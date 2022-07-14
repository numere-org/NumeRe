/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/



#include "wx/wx.h"
#include "../gui/terminal/terminal.hpp"
#include "core/datamanagement/dataops.hpp" // for make_stringmatrix()
#include "core/datamanagement/database.hpp"
#include "core/io/logger.hpp"

#define KERNEL_PRINT_SLEEP 2
#define TERMINAL_FORMAT_FIELD_LENOFFSET 16
#define DEFAULT_NUM_PRECISION 7
#define DEFAULT_MINMAX_PRECISION 5

extern const std::string sVersion;
/* --> STATUS: Versionsname des Programms; Aktuell "Ampere", danach "Angstroem". Ab 1.0 Namen mit "B",
 *     z.B.: Biot(1774), Boltzmann(1844), Becquerel(1852), Bragg(1862), Bohr(1885), Brillouin(1889),
 *     de Broglie(1892, Bose(1894), Bloch(1905), Bethe(1906)) <--
 * --> de Coulomb(1736), Carnot(1796), P.Curie(1859), M.Curie(1867), A.Compton(1892), Cherenkov(1904),
 *     Casimir(1909), Chandrasekhar(1910), Chamberlain(1920), Cabibbo(1935) <--
 */

Language _lang;
mglGraph _fontData;
extern value_type vAns;
extern DefaultVariables _defVars;
__time64_t tTimeZero = _time64(0);

// Initialization of the static member variables
NumeReKernel* NumeReKernel::kernelInstance = nullptr;
int* NumeReKernel::baseStackPosition = nullptr;
NumeReTerminal* NumeReKernel::m_parent = nullptr;
std::queue<NumeReTask> NumeReKernel::taskQueue;
int NumeReKernel::nLINE_LENGTH = 80;
bool NumeReKernel::bWritingTable = false;
int NumeReKernel::nOpenFileFlag = 0;
int NumeReKernel::nLastStatusVal = -1;
unsigned int NumeReKernel::nLastLineLength = 0;
bool NumeReKernel::modifiedSettings = false;
bool NumeReKernel::bCancelSignal = false;
NumeRe::Table NumeReKernel::table;
bool NumeReKernel::bSupressAnswer = false;
bool NumeReKernel::bGettingLine = false;
bool NumeReKernel::bErrorNotification = false;
ProcedureLibrary NumeReKernel::ProcLibrary;


/////////////////////////////////////////////////
/// \brief Constructor of the kernel.
/////////////////////////////////////////////////
NumeReKernel::NumeReKernel() : _option(), _memoryManager(), _parser(), _stringParser(_parser, _memoryManager, _option), _functions(false)
{
    sCommandLine.clear();
    sAnswer.clear();
    sPlotCompose.clear();
    kernelInstance = this;
    _ans = nullptr;
}


/////////////////////////////////////////////////
/// \brief Destructor of the kernel.
/////////////////////////////////////////////////
NumeReKernel::~NumeReKernel()
{
    CloseSession();
    kernelInstance = nullptr;
}


/////////////////////////////////////////////////
/// \brief Get the settings available in the
/// Settings class.
///
/// \return Settings
///
/////////////////////////////////////////////////
Settings NumeReKernel::getKernelSettings()
{
    return _option;
}


/////////////////////////////////////////////////
/// \brief Update the internal settings.
///
/// \param _settings const Settings&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::setKernelSettings(const Settings& _settings)
{
    _option.copySettings(_settings);
    _debugger.setActive(_settings.useDebugger());
}


/////////////////////////////////////////////////
/// \brief Saves the allocated memories by the
/// tables automatically.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::Autosave()
{
    if (!_memoryManager.getSaveStatus())
    {
        g_logger.info("Autosaving tables.");
        _memoryManager.saveToCacheFile();
    }
}


/////////////////////////////////////////////////
/// \brief This is the kernel "booting" function.
///
/// \param _parent wxTerm*
/// \param __sPath const std::string&
/// \param sPredefinedFunctions const std::string&
/// \return void
///
/// This function sets all parameters, functions
/// and constants used for the numerical parser.
/// loads possible available autosaves and
/// definition files for functions and plugins.
/////////////////////////////////////////////////
void NumeReKernel::StartUp(NumeReTerminal* _parent, const std::string& __sPath, const std::string& sPredefinedFunctions)
{
    if (_parent && m_parent == nullptr)
        m_parent = _parent;
    //Do some start-up stuff here

    std::string sTime = getTimeStamp(false);
    std::string sLogFile = "numere.log";
    std::string sPath = __sPath;

    // Set the functions provided by the syntax object in the parent class
    _functions.setPredefinedFuncs(sPredefinedFunctions);
    _memoryManager.setPredefinedFuncs(_functions.getPredefinedFuncs());
    _script.setPredefinedFuncs(sPredefinedFunctions);
    _procedure.setPredefinedFuncs(sPredefinedFunctions);

    // Make the path UNIX style
    while (sPath.find('\\') != std::string::npos)
        sPath[sPath.find('\\')] = '/';

    // Set the path in the settings object and load the settings
    // from the config file
    _option.getSetting(SETTING_S_EXEPATH).stringval() = sPath;
    _option.load(sPath);				// Lade Informationen aus einem ini-File

    // Initialize the log file
    if (_option.useLogFile())
    {
        reduceLogFilesize((sPath + "/" + sLogFile).c_str());
        g_logger.open(sPath + "/" + sLogFile);
    }
    else
        g_logger.setLoggingLevel(Logger::LVL_DISABLED);

    g_logger.info("Verifying file system.");
    // Set the path tokens for all relevant objects
    _fSys.setTokens(_option.getTokenPaths());
    _memoryManager.setTokens(_option.getTokenPaths());
    _out.setTokens(_option.getTokenPaths());
    _pData.setTokens(_option.getTokenPaths());
    _script.setTokens(_option.getTokenPaths());
    _functions.setTokens(_option.getTokenPaths());
    _procedure.setTokens(_option.getTokenPaths());
    _option.setTokens(_option.getTokenPaths());
    _lang.setTokens(_option.getTokenPaths());

    // Set the current line length
    nLINE_LENGTH = _option.getWindow();
    installing = false;
    refreshTree = false;

    // Set the default paths for all objects
    _out.setPath(_option.getSavePath(), true, sPath);
    _out.createRevisionsFolder();

    _memoryManager.setPath(_option.getLoadPath(), true, sPath);
    _memoryManager.createRevisionsFolder();
    _memoryManager.newCluster("ans").setDouble(0, NAN);

    _memoryManager.setSavePath(_option.getSavePath());
    _memoryManager.setbLoadEmptyCols(_option.loadEmptyCols());

    _pData.setPath(_option.getPlotPath(), true, sPath);
    _pData.createRevisionsFolder();

    _script.setPath(_option.getScriptPath(), true, sPath);
    _script.setPath(_option.getScriptPath() + "/packages", true, sPath);
    _script.setPath(_option.getScriptPath(), false, sPath);
    _script.createRevisionsFolder();

    _procedure.setPath(_option.getProcPath(), true, sPath);
    _procedure.createRevisionsFolder();

    // Create the default paths, if they are not present
    _option.setPath(_option.getExePath() + "/docs/plugins", true, sPath);
    _option.setPath(_option.getExePath() + "/docs", true, sPath);
    _option.setPath(_option.getExePath() + "/user/lang", true, sPath);
    _option.setPath(_option.getExePath() + "/user/docs", true, sPath);
    _option.setPath(_option.getSavePath() + "/docs", true, sPath);
    _functions.setPath(_option.getExePath(), false, sPath);
    _fSys.setPath(_option.getExePath(), false, sPath);
    g_logger.info("File system was verified.");

    // Load the documentation index file
    g_logger.info("Loading documentation index.");
    _option.createDocumentationIndex(_option.useCustomLangFiles());

    // Load the language strings
    g_logger.info("Loading kernel language files.");
    _lang.loadStrings(_option.useCustomLangFiles());

    std::string sCacheFile = _option.getExePath() + "/numere.cache";

    // Load the plugin informations
    if (fileExists(_procedure.getPluginInfoPath()))
    {
        g_logger.info("Loading plugins.");
        _procedure.loadPlugins();
        _memoryManager.setPluginCommands(_procedure.getPluginNames());
        _lang.addToLanguage(getPluginLanguageStrings());
    }

    // Load the function definitions
    if (_option.controlDefinitions() && fileExists(_option.getExePath() + "\\functions.def"))
    {
        g_logger.info("Loading custom function definitions.");
        _functions.load(_option, true);
    }

    // Load the binary plot font
    g_logger.info("Loading plotting font.");
    _fontData.LoadFont(_option.getDefaultPlotFont().c_str(), (_option.getExePath() + "\\fonts").c_str());

    // Load the autosave file
    if (fileExists(sCacheFile))
    {
        g_logger.info("Loading tables from last session.");
        _memoryManager.loadFromCacheFile();
    }

    // Declare the default variables
    _parser.DefineVar("ans", &vAns);        // Deklariere die spezielle Variable "ans", die stets, das letzte Ergebnis speichert und die vier Standardvariablen
    _parser.DefineVar(_defVars.sName[0], &_defVars.vValue[0][0]);
    _parser.DefineVar(_defVars.sName[1], &_defVars.vValue[1][0]);
    _parser.DefineVar(_defVars.sName[2], &_defVars.vValue[2][0]);
    _parser.DefineVar(_defVars.sName[3], &_defVars.vValue[3][0]);

    // Declare the table dimension variables
    _parser.DefineVar("nlines", &_memoryManager.tableLinesCount);
    _parser.DefineVar("nrows", &_memoryManager.tableLinesCount);
    _parser.DefineVar("ncols", &_memoryManager.tableColumnsCount);
    _parser.DefineVar("nlen", &_memoryManager.dClusterElementsCount);

    // --> VAR-FACTORY Deklarieren (Irgendwo muessen die ganzen Variablen-Werte ja auch gespeichert werden) <--
    g_logger.debug("Creating variable factory.");
    _parser.SetVarFactory(parser_AddVariable, &(_parser.m_lDataStorage));

    // Define the operators
    g_logger.debug("Defining operators.");
    defineOperators();

    // Define the constants
    g_logger.debug("Defining constants.");
    defineConst();

    // Define the functions
    g_logger.debug("Defining functions.");
    defineFunctions();

    g_logger.info("Kernel ready.");
}


/////////////////////////////////////////////////
/// \brief This member function declares all
/// numerical operators.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::defineOperators()
{
    // --> Syntax fuer die Umrechnungsfunktionen definieren und die zugehoerigen Funktionen deklarieren <--
    _parser.DefinePostfixOprt("'G", parser_Giga);
    _parser.DefinePostfixOprt("'M", parser_Mega);
    _parser.DefinePostfixOprt("'k", parser_Kilo);
    _parser.DefinePostfixOprt("'m", parser_Milli);
    _parser.DefinePostfixOprt("'mu", parser_Micro);
    _parser.DefinePostfixOprt("'n", parser_Nano);

    // --> Einheitenumrechnungen: Werden aufgerufen durch WERT'EINHEIT <--
    _parser.DefinePostfixOprt("'eV", parser_ElectronVolt);
    _parser.DefinePostfixOprt("'fm", parser_Fermi);
    _parser.DefinePostfixOprt("'A", parser_Angstroem);
    _parser.DefinePostfixOprt("'b", parser_Barn);
    _parser.DefinePostfixOprt("'Torr", parser_Torr);
    _parser.DefinePostfixOprt("'AU", parser_AstroUnit);
    _parser.DefinePostfixOprt("'ly", parser_Lightyear);
    _parser.DefinePostfixOprt("'pc", parser_Parsec);
    _parser.DefinePostfixOprt("'mile", parser_Mile);
    _parser.DefinePostfixOprt("'yd", parser_Yard);
    _parser.DefinePostfixOprt("'ft", parser_Foot);
    _parser.DefinePostfixOprt("'in", parser_Inch);
    _parser.DefinePostfixOprt("'cal", parser_Calorie);
    _parser.DefinePostfixOprt("'psi", parser_PSI);
    _parser.DefinePostfixOprt("'kn", parser_Knoten);
    _parser.DefinePostfixOprt("'l", parser_liter);
    _parser.DefinePostfixOprt("'kmh", parser_kmh);
    _parser.DefinePostfixOprt("'mph", parser_mph);
    _parser.DefinePostfixOprt("'TC", parser_Celsius);
    _parser.DefinePostfixOprt("'TF", parser_Fahrenheit);
    _parser.DefinePostfixOprt("'Ci", parser_Curie);
    _parser.DefinePostfixOprt("'Gs", parser_Gauss);
    _parser.DefinePostfixOprt("'Ps", parser_Poise);
    _parser.DefinePostfixOprt("'mol", parser_mol);
    _parser.DefinePostfixOprt("!", parser_Faculty);
    _parser.DefinePostfixOprt("!!", parser_doubleFaculty);
    _parser.DefinePostfixOprt("i", parser_imaginaryUnit);

    // --> Logisches NICHT <--
    _parser.DefineInfixOprt("!", parser_Not);
    _parser.DefineInfixOprt("+", parser_Ignore);

    // --> Operatoren <--
    _parser.DefineOprt("%", parser_Mod, prMUL_DIV, oaLEFT, true);
    _parser.DefineOprt("|||", parser_XOR, prLOGIC, oaLEFT, true);
    _parser.DefineOprt("|", parser_BinOR, prLOGIC, oaLEFT, true);
    _parser.DefineOprt("&", parser_BinAND, prLOGIC, oaLEFT, true);
}


/////////////////////////////////////////////////
/// \brief This member function declares all
/// numerical constants.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::defineConst()
{
    // --> Eigene Konstanten <--
    _parser.DefineConst("_g", 9.80665);
    _parser.DefineConst("_c", 299792458);
    _parser.DefineConst("_elek_feldkonst", 8.854187817e-12);
    _parser.DefineConst("_n_avogadro", 6.02214129e23);
    _parser.DefineConst("_k_boltz", 1.3806488e-23);
    _parser.DefineConst("_elem_ladung", 1.602176565e-19);
    _parser.DefineConst("_h", 6.62606957e-34);
    _parser.DefineConst("_hbar", 1.054571726e-34);
    _parser.DefineConst("_m_elektron", 9.10938291e-31);
    _parser.DefineConst("_m_proton", 1.672621777e-27);
    _parser.DefineConst("_m_neutron", 1.674927351e-27);
    _parser.DefineConst("_m_muon", 1.883531475e-28);
    _parser.DefineConst("_m_tau", 3.16747e-27);
    _parser.DefineConst("_magn_feldkonst", 1.25663706144e-6);
    _parser.DefineConst("_m_erde", 5.9726e24);
    _parser.DefineConst("_m_sonne", 1.9885e30);
    _parser.DefineConst("_r_erde", 6.378137e6);
    _parser.DefineConst("_r_sonne", 6.9551e8);
    _parser.DefineConst("true", 1);
    _parser.DefineConst("_theta_weinberg", 0.49097621387892);
    _parser.DefineConst("false", 0);
    _parser.DefineConst("_2pi", 6.283185307179586476925286766559);
    _parser.DefineConst("_R", 8.3144622);
    _parser.DefineConst("_alpha_fs", 7.2973525698E-3);
    _parser.DefineConst("_mu_bohr", 9.27400968E-24);
    _parser.DefineConst("_mu_kern", 5.05078353E-27);
    _parser.DefineConst("_mu_e", -9.284764620e-24);
    _parser.DefineConst("_mu_n", -9.662365e-27);
    _parser.DefineConst("_mu_p", 1.4106067873e8);
    _parser.DefineConst("_m_amu", 1.660538921E-27);
    _parser.DefineConst("_r_bohr", 5.2917721092E-11);
    _parser.DefineConst("_G", 6.67384E-11);
    _parser.DefineConst("_coul_norm", 8987551787.99791145324707);
    _parser.DefineConst("_stefan_boltzmann", 5.670367e-8);
    _parser.DefineConst("_wien", 2.8977729e-3);
    _parser.DefineConst("_rydberg", 1.0973731568508e7);
    _parser.DefineConst("_hartree", 4.35974465e-18);
    _parser.DefineConst("_lande_e", -2.00231930436182);
    _parser.DefineConst("_gamma_e", 1.760859644e11);
    _parser.DefineConst("_gamma_n", 1.83247172e8);
    _parser.DefineConst("_gamma_p", 2.6752219e8);
    _parser.DefineConst("_feigenbaum_delta", 4.66920160910299067185);
    _parser.DefineConst("_feigenbaum_alpha", 2.50290787509589282228);
    _parser.DefineConst("nan", NAN);
    _parser.DefineConst("inf", INFINITY);
    _parser.DefineConst("void", NAN);
    _parser.DefineConst("I", mu::value_type(0.0, 1.0));
    _parser.DefineConst(errorTypeToString(TYPE_CUSTOMERROR), mu::value_type(TYPE_CUSTOMERROR, 0));
    _parser.DefineConst(errorTypeToString(TYPE_SYNTAXERROR), mu::value_type(TYPE_SYNTAXERROR, 0));
    _parser.DefineConst(errorTypeToString(TYPE_ASSERTIONERROR), mu::value_type(TYPE_ASSERTIONERROR, 0));
    _parser.DefineConst(errorTypeToString(TYPE_MATHERROR), mu::value_type(TYPE_MATHERROR, 0));
}


/////////////////////////////////////////////////
/// \brief This member function declares all
/// mathematical functions.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::defineFunctions()
{

    /////////////////////////////////////////////////////////////////////
    // NOTE:
    // If multi-argument functions are declared, think of whether
    // they can be use a column of data sets as their argument list.
    // If not, then they have to be excluded in the multi-argument
    // function search in the parser.
    /////////////////////////////////////////////////////////////////////

    _parser.DefineFun("faculty", parser_Faculty, false);                        // faculty(n)
    _parser.DefineFun("factorial", parser_Faculty, false);                      // factorial(n)
    _parser.DefineFun("dblfacul", parser_doubleFaculty, false);                 // dblfacul(n)
    _parser.DefineFun("dblfact", parser_doubleFaculty, false);                  // dblfact(n)
    _parser.DefineFun("binom", parser_Binom, false);                            // binom(Wert1,Wert2)
    _parser.DefineFun("num", parser_Num, true);                                 // num(a,b,c,...)
    _parser.DefineFun("cnt", parser_Cnt, true);                                 // num(a,b,c,...)
    _parser.DefineFun("std", parser_Std, false);                                // std(a,b,c,...)
    _parser.DefineFun("prd", parser_product, false);                            // prd(a,b,c,...)
    _parser.DefineFun("round", parser_round, false);                            // round(x,n)
    _parser.DefineFun("radian", parser_toRadian, true);                         // radian(alpha)
    _parser.DefineFun("degree", parser_toDegree, true);                         // degree(x)
    _parser.DefineFun("Y", parser_SphericalHarmonics, true);                    // Y(l,m,theta,phi)
    _parser.DefineFun("imY", parser_imSphericalHarmonics, true);                // imY(l,m,theta,phi)
    _parser.DefineFun("Z", parser_Zernike, true);                               // Z(n,m,rho,phi)
    _parser.DefineFun("sinc", parser_SinusCardinalis, true);                    // sinc(x)
    _parser.DefineFun("sbessel", parser_SphericalBessel, true);                 // sbessel(n,x)
    _parser.DefineFun("sneumann", parser_SphericalNeumann, true);               // sneumann(n,x)
    _parser.DefineFun("bessel", parser_RegularCylBessel, true);                 // bessel(n,x)
    _parser.DefineFun("neumann", parser_IrregularCylBessel, true);              // neumann(n,x)
    _parser.DefineFun("legendre", parser_LegendrePolynomial, true);             // legendre(n,x)
    _parser.DefineFun("legendre_a", parser_AssociatedLegendrePolynomial, true); // legendre_a(l,m,x)
    _parser.DefineFun("laguerre", parser_LaguerrePolynomial, true);             // laguerre(n,x)
    _parser.DefineFun("laguerre_a", parser_AssociatedLaguerrePolynomial, true); // laguerre_a(n,k,x)
    _parser.DefineFun("hermite", parser_HermitePolynomial, true);               // hermite(n,x)
    _parser.DefineFun("betheweizsaecker", parser_BetheWeizsaecker, true);       // betheweizsaecker(N,Z)
    _parser.DefineFun("heaviside", parser_Heaviside, true);                     // heaviside(x)
    _parser.DefineFun("phi", parser_phi, true);                                 // phi(x,y)
    _parser.DefineFun("theta", parser_theta, true);                             // theta(x,y,z)
    _parser.DefineFun("norm", parser_Norm, true);                               // norm(x,y,z,...)
    _parser.DefineFun("med", parser_Med, true);                                 // med(x,y,z,...)
    _parser.DefineFun("pct", parser_Pct, true);                                 // pct(x,y,z,...)
    _parser.DefineFun("and", parser_and, true);                                 // and(x,y,z,...)
    _parser.DefineFun("or", parser_or, true);                                   // or(x,y,z,...)
    _parser.DefineFun("xor", parser_xor, true);                                 // xor(x,y,z,...)
    _parser.DefineFun("minpos", parser_MinPos, true);                           // minpos(x,y,z,...)
    _parser.DefineFun("maxpos", parser_MaxPos, true);                           // maxpos(x,y,z,...)
    _parser.DefineFun("polynomial", parser_polynomial, true);                   // polynomial(x,a0,a1,a2,a3,...)
    _parser.DefineFun("perlin", parser_perlin, true);                           // perlin(x,y,z,seed,freq,oct,pers)
    _parser.DefineFun("rand", parser_Random, false);                            // rand(left,right)
    _parser.DefineFun("gauss", parser_gRandom, false);                          // gauss(mean,std)
    _parser.DefineFun("erf", parser_erf, false);                                // erf(x)
    _parser.DefineFun("erfc", parser_erfc, false);                              // erfc(x)
    _parser.DefineFun("gamma", parser_gamma, false);                            // gamma(x)
    _parser.DefineFun("cmp", parser_compare, false);                            // cmp(crit,a,b,c,...,type)
    _parser.DefineFun("is_string", parser_is_string, false);                    // is_string(EXPR)
    _parser.DefineFun("to_value", parser_Ignore, false);                        // to_value(STRING)
    _parser.DefineFun("time", parser_time, false);                              // time()
    _parser.DefineFun("clock", parser_clock, false);                            // clock()
    _parser.DefineFun("sleep", parser_sleep, false);                            // sleep(millisecnds)
    _parser.DefineFun("version", parser_numereversion, true);                   // version()
    _parser.DefineFun("date", parser_date, false);                              // date(TIME,TYPE)
    _parser.DefineFun("is_nan", parser_isnan, true);                            // is_nan(x)
    _parser.DefineFun("range", parser_interval, true);                          // range(x,left,right)
    _parser.DefineFun("Ai", parser_AiryA, true);                                // Ai(x)
    _parser.DefineFun("Bi", parser_AiryB, true);                                // Bi(x)
    _parser.DefineFun("ellipticF", parser_EllipticF, true);                     // ellipticF(x,k)
    _parser.DefineFun("ellipticE", parser_EllipticE, true);                     // ellipticE(x,k)
    _parser.DefineFun("ellipticPi", parser_EllipticP, true);                    // ellipticPi(x,n,k)
    _parser.DefineFun("ellipticD", parser_EllipticD, true);                     // ellipticD(x,n,k)
    _parser.DefineFun("cot", parser_cot, true);                                 // cot(x)
    _parser.DefineFun("floor", parser_floor, true);                             // floor(x)
    _parser.DefineFun("roof", parser_roof, true);                               // roof(x)
    _parser.DefineFun("rect", parser_rect, true);                               // rect(x,x0,x1)
    _parser.DefineFun("ivl", parser_ivl, true);                                 // ivl(x,x0,x1,lb,rb)
    _parser.DefineFun("student_t", parser_studentFactor, true);                 // student_t(number,confidence)
    _parser.DefineFun("gcd", parser_gcd, true);                                 // gcd(x,y)
    _parser.DefineFun("lcm", parser_lcm, true);                                 // lcm(x,y)
    _parser.DefineFun("beta", parser_beta, true);                               // beta(x,y)
    _parser.DefineFun("zeta", parser_zeta, true);                               // zeta(n)
    _parser.DefineFun("Cl2", parser_clausen, true);                             // Cl2(x)
    _parser.DefineFun("psi", parser_digamma, true);                             // psi(x)
    _parser.DefineFun("psi_n", parser_polygamma, true);                         // psi_n(n,x)
    _parser.DefineFun("Li2", parser_dilogarithm, true);                         // Li2(x)
    _parser.DefineFun("log_b", parser_log_b, true);                             // log_b(b,x)
    _parser.DefineFun("real", parser_real, true);                               // real(x)
    _parser.DefineFun("imag", parser_imag, true);                               // imag(x)
    _parser.DefineFun("to_rect", parser_polar2rect, true);                      // to_rect(x)
    _parser.DefineFun("to_polar", parser_rect2polar, true);                     // to_polar(x)
    _parser.DefineFun("conj", parser_conj, true);                               // conj(x)
    _parser.DefineFun("complex", parser_complex, true);                         // complex(re,im)

    /////////////////////////////////////////////////////////////////////
    // NOTE:
    // If multi-argument functions are declared, think of whether
    // they can be use a column of data sets as their argument list.
    // If not, then they have to be excluded in the multi-argument
    // function search in the parser.
    // Multi-args with a limited number of args (like perlin) have to be
    // excluded always.
    /////////////////////////////////////////////////////////////////////
}


/////////////////////////////////////////////////
/// \brief Starts the stack tracker, which will
/// prevent stack overflows.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::initializeStackTracker()
{
    // measure the current stack position
    int stackMeasureVar;
    baseStackPosition = &stackMeasureVar;
    g_logger.debug("Base stack address = " + toHexString((int)baseStackPosition));
}


/////////////////////////////////////////////////
/// \brief This member function prints the version
/// headline and the version information to the
/// console.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::printVersionInfo()
{
    bWritingTable = true;
    make_hline(80);
    printPreFmt("| ");
    displaySplash();
    printPreFmt("                                  |\n");
    printPreFmt("| Version: " + sVersion + strfill("Build: ", 79 - 22 - sVersion.length()) + AutoVersion::YEAR + "-" + AutoVersion::MONTH + "-" + AutoVersion::DATE + " |\n");
    printPreFmt("| Copyright (c) 2013-" + std::string(AutoVersion::YEAR) + toSystemCodePage(", Erik A. Hänel et al.") + strfill(toSystemCodePage(_lang.get("MAIN_ABOUT_NBR")), 79 - 48) + " |\n");
    make_hline(80);

    printPreFmt("|\n");

    if (_option.showGreeting() && fileExists(_option.getExePath() + "\\numere.ini"))
        printPreFmt(toSystemCodePage(getGreeting()) + "|\n");

    print(LineBreak(_lang.get("PARSER_INTRO"), _option));
    flush();
    bWritingTable = false;

}


/////////////////////////////////////////////////
/// \brief This is the main loop for the core of
/// NumeRe.
///
/// \param sCommand const std::string&
/// \return NumeReKernel::KernelStatus
///
/// This function is called by the terminal to
/// process the desired operations using the core
/// of this application. It features an inner
/// loop, which will process a whole script or a
/// semicolon-separated list of commands
/// automatically.
/////////////////////////////////////////////////
NumeReKernel::KernelStatus NumeReKernel::MainLoop(const std::string& sCommand)
{
    if (!m_parent)
        return NUMERE_ERROR;


    std::string sLine_Temp = "";     // Temporaerer String fuer die Eingabe
    std::string sCache;         // Zwischenspeicher fuer die Cache-Koordinaten
    std::string sKeep = "";          // Zwei '\' am Ende einer Zeile ermoeglichen es, dass die Eingabe auf mehrere Zeilen verteilt wird.
    std::string sCmdCache = "";      // Die vorherige Zeile wird hierin zwischengespeichert
    std::string sLine = "";          // The actual line
    std::string sCurrentCommand = "";// The current command
    mu::value_type* v = 0;          // Ergebnisarray
    int& nDebuggerCode = _procedure.getDebuggerCode();
    int nNum = 0;               // Zahl der Ergebnisse in value_type* v
    nLastStatusVal = -1;
    nLastLineLength = 0;
    installing = false;
    refreshTree = false;

    // Needed for some handler functions
    KernelStatus nReturnVal = NUMERE_ERROR;

    // add the passed command to the internal command line (append it, if it's non-empty)
    sCommandLine += sCommand;
    if (!sCommandLine.length())
        return NUMERE_PENDING;

    // clear whitespaces
    while (sCommandLine.front() == ' ')
        sCommandLine.erase(0, 1);
    if (!sCommandLine.length())
        return NUMERE_PENDING;

    // clear whitespaces
    while (sCommandLine.back() == ' ')
        sCommandLine.pop_back();

    // check for the double backslash at the end of the line
    if (sCommandLine.length() > 2 && sCommandLine.substr(sCommandLine.length() - 2, 2) == "\\\\")
    {
        sCommandLine.erase(sCommandLine.length() - 2);
        return NUMERE_PENDING;
    }

    // Pass the combined command line to the internal variable and clear the contents of the class
    // member variable (we don't want to repeat the tasks entered last time)
    sLine = sCommandLine;
    sCommandLine.clear();
    _parser.ClearVectorVars();
    // Remove all temporary clusters defined for
    // inlined procedures
    _memoryManager.removeTemporaryClusters();
    _assertionHandler.resetStats();
    bSupressAnswer = false;

    // set the procedure main path to the desired one. --> check, whether this is necessary here
    if (_procedure.getPath() != _option.getProcPath())
    {
        _procedure.setPath(_option.getProcPath(), true, _procedure.getProgramPath());
        _option.getSetting(SETTING_S_PROCPATH).stringval() = _procedure.getPath();
    }

    // Evaluate the passed commands or the contents of the script
    // This is the actual loop. It will evaluate at least once.
    do
    {
        bSupressAnswer = false;
        sCache.clear();
        _assertionHandler.reset();

        // Reset the parser variable map pointer
        if (_parser.mVarMapPntr)
            _parser.mVarMapPntr = 0;

        // Try-catch block to handle all the internal exceptions
        try
        {
            // Handle command line sources and validate the input
            if (!handleCommandLineSource(sLine, sCmdCache, sKeep))
                continue;

            // Search for the "assert" command decoration
            if (findCommand(sLine, "assert").sString == "assert" && !_procedure.getCurrentBlockDepth())
            {
                _assertionHandler.enable(sLine);
                sLine.erase(findCommand(sLine, "assert").nPos, 6);
                StripSpaces(sLine);
            }

            // Get the current command
            sCurrentCommand = findCommand(sLine).sString;

            // Get the tasks from the command cache or add
            // the current line to the command cache
            if (!getLineFromCommandCache(sLine, sCmdCache, sCurrentCommand))
                continue;

            // Eval debugger breakpoints from scripts
            if ((sLine.substr(0, 2) == "|>" || nDebuggerCode == DEBUGGER_STEP)
                    && _script.isValid()
                    && !_procedure.is_writing()
                    && !_procedure.getCurrentBlockDepth())
            {
                if (sLine.substr(0, 2) == "|>")
                    sLine.erase(0, 2);

                if (_option.useDebugger() && nDebuggerCode != DEBUGGER_LEAVE)
                {
                    nDebuggerCode = evalDebuggerBreakPoint(sLine);
                }
            }

            // Log the current line
            g_logger.cmdline(sLine);

            // Handle, whether the pressed the ESC key
            if (GetAsyncCancelState() && _script.isValid() && _script.isOpen())
            {
                if (_option.useEscInScripts())
                    throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
            }
            // Done explicitly twice to clear the key cache
            GetAsyncKeyState(VK_ESCAPE);

            // Get the current command
            sCurrentCommand = findCommand(sLine).sString;

            // Handle the compose block
            nReturnVal = NUMERE_ERROR;
            if (!handleComposeBlock(sLine, sCmdCache, sCurrentCommand, nReturnVal))
            {
                // returns false either when the loop shall return
                // or it shall continue
                if (nReturnVal)
                    return nReturnVal;
                continue;
            }

            // Evaluate the install info string
            if (_script.isValid() && _script.isOpen() && _script.installProcedures())
                installing = true;

            // Get the current command
            sCurrentCommand = findCommand(sLine).sString;

            // uninstall the plugin, if desired
            if (uninstallPlugin(sLine, sCurrentCommand))
                return NUMERE_DONE_KEYWORD;

            // Handle the writing of procedures
            nReturnVal = NUMERE_ERROR;
            if (!handleProcedureWrite(sLine, sCmdCache, sCurrentCommand, nReturnVal))
            {
                // returns false either when the loop shall return
                // or it shall continue
                if (nReturnVal)
                    return nReturnVal;
                continue;
            }

            // Handle the "to_cmd()" function
            handleToCmd(sLine, sCache, sCurrentCommand);

            // Handle procedure calls at this location
            // Will return false, if the command line was cleared completely
            if (!evaluateProcedureCalls(sLine))
                continue;

            // --> Gibt es "??"? Dann rufe die Prompt-Funktion auf <--
            if (!_procedure.getCurrentBlockDepth() && sLine.find("??") != std::string::npos && sCurrentCommand != "help")
                sLine = promptForUserInput(sLine);

            // Handle plugin commands
            // Will return false, if the command line was cleared completely
            if (!executePlugins(sLine))
                continue;

            // remove the "explicit" command, which may be used to suppress plugins
            if (findCommand(sLine, "explicit").sString == "explicit")
            {
                sLine.erase(findCommand(sLine, "explicit").nPos, 8);
                StripSpaces(sLine);
            }

            /* --> Die Keyword-Suche soll nur funktionieren, wenn keine Schleife eingegeben wird, oder wenn eine
             *     eine Schleife eingegeben wird, dann nur in den wenigen Spezialfaellen, die zum Nachschlagen
             *     eines Keywords noetig sind ("list", "help", "find", etc.) <--
             */
            if ((!_procedure.getCurrentBlockDepth() && !FlowCtrl::isFlowCtrlStatement(sCurrentCommand))
                    || sCurrentCommand == "help"
                    || sCurrentCommand == "man"
                    || sCurrentCommand == "quit"
                    || sCurrentCommand == "list"
                    || sCurrentCommand == "find"
                    || sCurrentCommand == "search")
            {
                //print("Debug: Keywords");
                switch (commandHandler(sLine))
                {
                    case NO_COMMAND:
                    case COMMAND_HAS_RETURNVALUE:
                        break; // Kein Keyword: Mit dem Parser auswerten
                    case COMMAND_PROCESSED:        // Keyword: Naechster Schleifendurchlauf!
                        if (!sCmdCache.length() && !(_script.isValid() && _script.isOpen()))
                        {
                            if (_script.wasLastCommand())
                            {
                                print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
                                _memoryManager.setPluginCommands(_procedure.getPluginNames());

                                checkInternalStates();
                            }

                            sCommandLine.clear();
                            bCancelSignal = false;
                            return NUMERE_DONE_KEYWORD;
                        }
                        else
                            continue;
                    case NUMERE_QUIT:
                        // --> Sind ungesicherte Daten im Cache? Dann moechte der Nutzer diese vielleicht speichern <--
                        if (!_memoryManager.getSaveStatus()) // MAIN_UNSAVED_CACHE
                        {
                            std::string c = "";
                            print(LineBreak(_lang.get("MAIN_UNSAVED_CACHE"), _option));
                            printPreFmt("|\n|<- ");
                            NumeReKernel::getline(c);
                            if (c == _lang.YES())
                            {
                                _memoryManager.saveToCacheFile(); // MAIN_CACHE_SAVED
                                print(LineBreak(_lang.get("MAIN_CACHE_SAVED"), _option));
                                Sleep(500);
                            }
                            else
                            {
                                _memoryManager.removeTablesFromMemory();
                            }
                        }
                        return NUMERE_QUIT;  // Keyword "quit"
                        //case  2: return 1;  // Keyword "mode"
                }
            }

            // Evaluate function calls (only outside the flow control blocks)
            if (!_procedure.getCurrentBlockDepth() && !FlowCtrl::isFlowCtrlStatement(sCurrentCommand))
            {
                if (!_functions.call(sLine))
                    throw SyntaxError(SyntaxError::FUNCTION_ERROR, sLine, SyntaxError::invalid_position);
            }

            // Handle procedure calls at this location
            // Will return false, if the command line was cleared completely
            if (!evaluateProcedureCalls(sLine))
                continue;

            // --> Nochmals ueberzaehlige Leerzeichen entfernen <--
            StripSpaces(sLine);
            if (!_procedure.getCurrentBlockDepth())
            {
                // this is obviously a time consuming task => to be investigated
                evalRecursiveExpressions(sLine);
            }

            // Handle the flow controls like "if", "while", "for"
            nReturnVal = NUMERE_ERROR;
            if (!handleFlowControls(sLine, sCmdCache, sCurrentCommand, nReturnVal))
            {
                // returns false either when the loop shall return
                // or it shall continue
                if (nReturnVal)
                    return nReturnVal;
                continue;
            }

            // --> Gibt es "??" ggf. nochmal? Dann rufe die Prompt-Funktion auf <--
            if (sLine.find("??") != std::string::npos)
                sLine = promptForUserInput(sLine);

			bool bWriteToCache = false;
			bool bWriteToCluster = false;

            // Get data elements for the current command line or determine,
            // if the target value of the current command line is a candidate
            // for a cluster
            if (!_stringParser.isStringExpression(sLine) && _memoryManager.containsTablesOrClusters(sLine))
            {
                sCache = getDataElements(sLine, _parser, _memoryManager, _option);

                if (sCache.length() && sCache.find('#') == std::string::npos)
                    bWriteToCache = true;
            }
            else if (isClusterCandidate(sLine, sCache))
                bWriteToCache = true;

            // Remove the definition operator
            while (sLine.find(":=") != std::string::npos)
            {
                sLine.erase(sLine.find(":="), 1);
            }

            // evaluate strings
            nReturnVal = NUMERE_ERROR;

            if (!evaluateStrings(sLine, sCache, sCmdCache, bWriteToCache, nReturnVal))
            {
                // returns false either when the loop shall return
                // or it shall continue
                if (nReturnVal)
                    return nReturnVal;

                continue;
            }

            // --> Wenn die Ergebnisse in den Cache geschrieben werden sollen, bestimme hier die entsprechenden Koordinaten <--
            Indices _idx;

            if (bWriteToCache)
            {
                // Get the indices from the corresponding function
                getIndices(sCache, _idx, _parser, _memoryManager, _option);

                if (sCache[sCache.find_first_of("({")] == '{')
                {
                    bWriteToCluster = true;
                }

                if (!isValidIndexSet(_idx))
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "", _idx.row.to_string() + "," + _idx.col.to_string());

                if (!bWriteToCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
                    throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");

                sCache.erase(sCache.find_first_of("({"));
                StripSpaces(sCache);
            }

            // --> Ausdruck an den Parser uebergeben und einmal auswerten <--
            if (!_parser.IsAlreadyParsed(sLine))
                _parser.SetExpr(sLine);

            // --> Jetzt weiss der Parser, wie viele Ergebnisse er berechnen muss <--
            v = _parser.Eval(nNum);
            _assertionHandler.checkAssertion(v, nNum);

            // Create the answer of the calculation and print it
            // to the command line, if not suppressed
            createCalculationAnswer(nNum, v, sCmdCache);

            if (bWriteToCache)
            {
                // Is it a cluster?
                if (bWriteToCluster)
                {
                    NumeRe::Cluster& cluster = _memoryManager.getCluster(sCache);
                    cluster.assignResults(_idx, nNum, v);
                }
                else
                    _memoryManager.writeToTable(_idx, sCache, v, nNum);
            }
        }
        // This section starts the error handling
        catch (mu::Parser::exception_type& e)
        {
            _option.enableSystemPrints(true);
            // --> Vernuenftig formatierte Fehlermeldungen <--
            unsigned int nErrorPos = e.GetPos();
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_MUP_HEAD")));
            make_hline();

            // --> Eigentliche Fehlermeldung <--
            print(LineBreak(e.GetMsg(), _option));
            print(LineBreak(_lang.get("ERR_EXPRESSION", maskProcedureSigns(e.GetExpr())), _option, true, 4, 15));

            /* --> Ausdruecke, die laenger als 63 Zeichen sind, passen nicht in die Zeile. Wir stellen nur die ersten
             *     60 Zeichen gefolgt von "..." dar <--
             */
            // --> Fehlerhaftes/Unerwartetes Objekt <--
            if (e.GetToken().length())
                print(toSystemCodePage(_lang.get("ERR_OBJECT", e.GetToken())));

            /* --> Position des Fehlers im Ausdruck: Wir stellen um den Fehler nur einen Ausschnitt
             *     des Ausdrucks in der Laenge von etwa 63 Zeichen dar und markieren die Fehlerposition
             *     durch ein darunter angezeigten Zirkumflex "^" <--
             */
            if (nErrorPos < e.GetExpr().length())
            {
                if (e.GetExpr().length() > 63 && nErrorPos > 31 && nErrorPos < e.GetExpr().length() - 32)
                {
                    printPreFmt("|   Position:  '..." + e.GetExpr().substr(nErrorPos - 29, 57) + "...'\n");
                    printPreFmt(pointToError(32));
                }
                else if (nErrorPos < 32)
                {
                    std::string sErrorExpr = "|   Position:  '";
                    if (e.GetExpr().length() > 63)
                        sErrorExpr += e.GetExpr().substr(0, 60) + "...'";
                    else
                        sErrorExpr += e.GetExpr() + "'";
                    printPreFmt(sErrorExpr + "\n");
                    printPreFmt(pointToError(nErrorPos + 1));
                }
                else if (nErrorPos > e.GetExpr().length() - 32)
                {
                    std::string sErrorExpr = "|   Position:  '";
                    if (e.GetExpr().length() > 63)
                    {
                        printPreFmt(sErrorExpr + "..." + e.GetExpr().substr(e.GetExpr().length() - 60) + "'\n");
                        printPreFmt(pointToError(65 - (e.GetExpr().length() - nErrorPos) - 2));
                    }
                    else
                    {
                        printPreFmt(sErrorExpr + e.GetExpr() + "'\n");
                        printPreFmt(pointToError(nErrorPos));
                    }
                }
            }

            resetAfterError(sCmdCache);

            make_hline();
            g_logger.error(toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.GetMsg());


            sendErrorNotification();
            return NUMERE_ERROR;
        }
        catch (const std::bad_alloc& e)
        {
            _option.enableSystemPrints(true);
            /* --> Das ist die schlimmste aller Exceptions: Fehler bei der Speicherallozierung.
             *     Leider gibt es bis dato keine Moeglichkeit, diesen wieder zu beheben, also bleibt
             *     vorerst nichts anderes uebrig, als NumeRe mit terminate() abzuschiessen <--
             */
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_STD_BA_HEAD")));
            make_hline();
            print(LineBreak(_lang.get("ERR_STD_BADALLOC", sVersion), _option));
            resetAfterError(sCmdCache);

            make_hline();
            g_logger.error("ERROR: CRITICAL ACCESS VIOLATION");
            sendErrorNotification();

            return NUMERE_ERROR;
        }
        catch (const std::exception& e)
        {
            _option.enableSystemPrints(true);
            // --> Alle anderen Standard-Exceptions <--
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_STD_INTERNAL_HEAD")));
            make_hline();
            print(LineBreak(std::string(e.what()), _option));
            print(LineBreak(_lang.get("ERR_STD_INTERNAL"), _option));

            resetAfterError(sCmdCache);

            g_logger.error(toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.what());
            make_hline();
            sendErrorNotification();

            return NUMERE_ERROR;
        }
        catch (SyntaxError& e)
        {
            _option.enableSystemPrints(true);
            sendErrorNotification();
            make_hline();
            if (e.errorcode == SyntaxError::PROCESS_ABORTED_BY_USER)
            {
                print(toUpperCase(_lang.get("ERR_PROCESS_CANCELLED_HEAD")));
                make_hline();
                print(LineBreak(_lang.get("ERR_NR_3200_0_PROCESS_ABORTED_BY_USER"), _option, false));
                //cerr << LineBreak("|-> Siehe auch \"help procedure\"", _option) << endl;

                g_logger.warning("Process was cancelled by user");
                // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
                if (_script.isValid() && _script.isOpen())
                {
                    print(LineBreak(_lang.get("ERR_SCRIPTABORT", toString((int)_script.getCurrentLine())), _option));
                    // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                    _script.close();
                }
            }
            else
            {
                print(toUpperCase(_lang.get("ERR_NR_HEAD")));
                make_hline();

                if (e.getToken().length() && (e.errorcode == SyntaxError::PROCEDURE_THROW || e.errorcode == SyntaxError::LOOP_THROW))
                {
                    print(LineBreak(e.getToken(), _option));
                    g_logger.error(toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.getToken());
                }
                else
                {
                    std::string sErrLine_0 = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3]));
                    std::string sErrLine_1 = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_1_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3]));
                    std::string sErrIDString = _lang.getKey("ERR_NR_" + toString((int)e.errorcode) + "_0_*");

                    if (sErrLine_0.substr(0, 7) == "ERR_NR_")
                    {
                        sErrLine_0 = _lang.get("ERR_GENERIC_0", toString((int)e.errorcode));
                        sErrLine_1 = _lang.get("ERR_GENERIC_1");
                        sErrIDString = "ERR_GENERIC";
                    }
                    print(LineBreak(sErrLine_0, _option));
                    print(LineBreak(sErrLine_1, _option));
                    if (e.getExpr().length())
                    {
                        print(LineBreak(_lang.get("ERR_EXPRESSION", maskProcedureSigns(e.getExpr())), _option, true, 4, 15));
                        if (e.getPosition() != SyntaxError::invalid_position)
                        {
                            /* --> Position des Fehlers im Ausdruck: Wir stellen um den Fehler nur einen Ausschnitt
                             *     des Ausdrucks in der Laenge von etwa 63 Zeichen dar und markieren die Fehlerposition
                             *     durch ein darunter angezeigten Zirkumflex "^" <--
                             */
                            if (e.getExpr().length() > 63 && e.getPosition() > 31 && e.getPosition() < e.getExpr().length() - 32)
                            {
                                printPreFmt("|   Position:  '..." + e.getExpr().substr(e.getPosition() - 29, 57) + "...'\n");
                                printPreFmt(pointToError(32));
                            }
                            else if (e.getPosition() < 32)
                            {
                                std::string sErrorExpr = "|   Position:  '";
                                if (e.getExpr().length() > 63)
                                    sErrorExpr += e.getExpr().substr(0, 60) + "...'";
                                else
                                    sErrorExpr += e.getExpr() + "'";
                                printPreFmt(sErrorExpr + "\n");
                                printPreFmt(pointToError(e.getPosition() + 1));
                            }
                            else if (e.getPosition() > e.getExpr().length() - 32)
                            {
                                std::string sErrorExpr = "|   Position:  '";
                                if (e.getExpr().length() > 63)
                                {
                                    printPreFmt(sErrorExpr + "..." + e.getExpr().substr(e.getExpr().length() - 60) + "'\n");
                                    printPreFmt(pointToError(65 - (e.getExpr().length() - e.getPosition()) - 2));
                                }
                                else
                                {
                                    printPreFmt(sErrorExpr + e.getExpr() + "'\n");
                                    printPreFmt(pointToError(e.getPosition()));
                                }
                            }
                        }
                    }

                    g_logger.error(toUpperCase(_lang.get("ERR_ERROR")) + ": " + sErrIDString);
                }
            }
            resetAfterError(sCmdCache);

            make_hline();
            sendErrorNotification();

            return NUMERE_ERROR;
        }
        catch (...)
        {
            /* --> Allgemeine Exception abfangen, die nicht durch mu::exception_type oder std::exception
             *     abgedeckt wird <--
             */
            _option.enableSystemPrints(true);
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_CATCHALL_HEAD")));
            make_hline();
            print(LineBreak(_lang.get("ERR_CATCHALL"), _option));

            resetAfterError(sCmdCache);
            make_hline();

            g_logger.error("ERROR: UNKNOWN EXCEPTION");
            sendErrorNotification();

            return NUMERE_ERROR;
        }

        _stringParser.removeTempStringVectorVars();

        if (_script.wasLastCommand())
        {
            print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
            _memoryManager.setPluginCommands(_procedure.getPluginNames());

            checkInternalStates();

            if (!sCmdCache.length())
            {
                bCancelSignal = false;
                return NUMERE_DONE_KEYWORD;
            }
        }
    }
    while ((_script.isValid() && _script.isOpen()) || sCmdCache.length());

    bCancelSignal = false;
    nDebuggerCode = 0;

    if (_script.wasLastCommand())
    {
        print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
        _memoryManager.setPluginCommands(_procedure.getPluginNames());

        checkInternalStates();

        return NUMERE_DONE_KEYWORD;
    }

    if (bSupressAnswer || !sLine.length())
        return NUMERE_DONE_KEYWORD;
    return NUMERE_DONE;

}


/////////////////////////////////////////////////
/// \brief This private member function will
/// handle the command line input source and
/// validate it, before the core will evaluate it.
///
/// \param sLine std::string&
/// \param sCmdCache const std::string&
/// \param sKeep std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleCommandLineSource(std::string& sLine, const std::string& sCmdCache, std::string& sKeep)
{
    if (!sCmdCache.length())
    {
        // --> Wenn gerade ein Script aktiv ist, lese dessen naechste Zeile, sonst nehme eine Zeile von std::cin <--
        if (_script.isValid() && _script.isOpen())
        {
            sLine = _script.getNextScriptCommand();
        }

        // --> Leerzeichen und Tabulatoren entfernen <--
        StripSpaces(sLine);
        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            if (sLine[i] == '\t')
                sLine[i] = ' ';
        }

        // --> Keine Laenge? Ignorieren! <--
        if (!sLine.length() || sLine[0] == '@')
            return false;

        // --> Kommando "global" entfernen <--
        if (findCommand(sLine).sString == "global")
        {
            sLine = sLine.substr(findCommand(sLine).nPos + 6);
            StripSpaces(sLine);
        }

        // --> Wenn die Laenge groesser als 2 ist, koennen '\' am Ende sein <--
        if (sLine.length() > 2)
        {
            if (sLine.substr(sLine.length() - 2, 2) == "\\\\")
            {
                // --> Ergaenze die Eingabe zu sKeep und beginne einen neuen Schleifendurchlauf <--
                sKeep += sLine.substr(0, sLine.length() - 2);
                return false;
            }
        }

        /* --> Steht etwas in sKeep? Ergaenze die aktuelle Eingabe, weise dies
         *     wiederum an sLine zu und loesche den Inhalt von sKeep <--
         */
        if (sKeep.length())
        {
            sKeep += sLine;
            sLine = sKeep;
            sKeep = "";
        }

        // Ensure that the number of parentheses is matching
        if (findCommand(sLine).sString != "help"
                && findCommand(sLine).sString != "find"
                && findCommand(sLine).sString != "search"
                && (sLine.find('(') != std::string::npos || sLine.find('{') != std::string::npos))
        {
            if (!validateParenthesisNumber(sLine))
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, sLine.find('('));
        }
    }
    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function returns
/// the current command line from the command
/// cache (i.e. the list of commands separated by
/// semicolons).
///
/// \param sLine std::string&
/// \param sCmdCache std::string&
/// \param sCurrentCommand const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::getLineFromCommandCache(std::string& sLine, std::string& sCmdCache, const std::string& sCurrentCommand)
{
    // Only do something if the command cache is not empty or the current line contains a semicolon
    if ((sCmdCache.length() || sLine.find(';') != std::string::npos) && !_procedure.is_writing() && sCurrentCommand != "procedure")
    {
        if (sCmdCache.length())
        {
            // The command cache is not empty
            // Get the next task from the command cache
            while (sCmdCache.front() == ';' || sCmdCache.front() == ' ')
                sCmdCache.erase(0, 1);
            if (!sCmdCache.length())
                return false;
            if (sCmdCache.find(';') != std::string::npos)
            {
                // More than one task available
                for (unsigned int i = 0; i < sCmdCache.length(); i++)
                {
                    if (sCmdCache[i] == ';' && !isInQuotes(sCmdCache, i))
                    {
                        bSupressAnswer = true;
                        sLine = sCmdCache.substr(0, i);
                        sCmdCache.erase(0, i + 1);
                        break;
                    }
                    if (i == sCmdCache.length() - 1)
                    {
                        sLine = sCmdCache;
                        sCmdCache.clear();
                        break;
                    }
                }
            }
            else
            {
                // Only one task available
                sLine = sCmdCache;
                sCmdCache.clear();
            }
        }
        else if (sLine.find(';') == sLine.length() - 1)
        {
            // Only remove the trailing semicolon -> used to suppress the output of the current line
            bSupressAnswer = true;
            sLine.pop_back();
        }
        else
        {
            // Use only the first task from the command line and cache the remaining
            // part in the command cache
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == '(' || sLine[i] == '[' || sLine[i] == '{')
                {
                    size_t parens = getMatchingParenthesis(sLine.substr(i));
                    if (parens != std::string::npos)
                        i += parens;
                }
                if (sLine[i] == ';' && !isInQuotes(sLine, i))
                {
                    if (i != sLine.length() - 1)
                        sCmdCache = sLine.substr(i + 1);
                    sLine.erase(i);
                    bSupressAnswer = true;
                }
                if (i == sLine.length() - 1)
                {
                    break;
                }
            }
        }
    }
    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// handle the compose block (i.e. store the
/// different commands and return them, once the
/// block is finished).
///
/// \param sLine std::string&
/// \param sCmdCache const std::string&
/// \param sCurrentCommand const std::string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleComposeBlock(std::string& sLine, const std::string& sCmdCache, const std::string& sCurrentCommand, KernelStatus& nReturnVal)
{
    // Only do something, if the current command contains
    // a compose block syntax element
    if ((sCurrentCommand == "compose"
            || sCurrentCommand == "endcompose"
            || sPlotCompose.length())
            && !_procedure.is_writing()
            && sCurrentCommand != "quit"
            && sCurrentCommand != "help")
    {
        if (!sPlotCompose.length() && findCommand(sLine).sString == "compose")
        {
            // Create a new compose block
            sPlotCompose = "plotcompose ";

            // Add the multiplot layout, if needed
            if (findParameter(sLine, "multiplot", '='))
            {
                sPlotCompose += "-multiplot=" + getArgAtPos(sLine, findParameter(sLine, "multiplot", '=') + 9) + " <<COMPOSE>> ";
            }

            // If the block wasn't started from a script, then ask the user
            // to complete it
            if (!(_script.isValid() && _script.isOpen()) && !sCmdCache.length())
            {
                if (_procedure.getCurrentBlockDepth())
                {
                    // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                    printPreFmt("|" + _procedure.getCurrentBlock());
                    if (_procedure.getCurrentBlock() == "IF")
                    {
                        if (_procedure.getCurrentBlockDepth() > 1)
                            printPreFmt("---");
                        else
                            printPreFmt("-");
                    }
                    else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getCurrentBlockDepth() > 1)
                        printPreFmt("-");
                    else
                    {
                        if (_procedure.getCurrentBlockDepth() > 1)
                            printPreFmt("--");
                    }
                    printPreFmt(strfill("> ", 2 * _procedure.getCurrentBlockDepth(), '-'));
                }
                else if (_procedure.is_writing())
                {
                    printPreFmt("|PROC> ");
                }
                else if (!_procedure.is_writing() && sPlotCompose.length())
                {
                    printPreFmt("|COMP> ");
                }

                // Waiting for input
                nReturnVal = NUMERE_PENDING_SPECIAL;
                return false;
            }
            return false;
        }
        else if (findCommand(sLine).sString == "abort")
        {
            // Abort the compose block
            sPlotCompose = "";
            print(LineBreak(_lang.get("PARSER_ABORTED"), _option));
            return false;
        }
        else if (findCommand(sLine).sString != "endcompose")
        {
            // add a new plotting command
            std::string sCommand = findCommand(sLine).sString;
            if (sCommand.substr(0, 4) == "plot"
                    || sCommand.substr(0, 7) == "subplot"
                    || sCommand.substr(0, 4) == "grad"
                    || sCommand.substr(0, 4) == "dens"
                    || sCommand.substr(0, 4) == "draw"
                    || sCommand.substr(0, 4) == "vect"
                    || sCommand.substr(0, 4) == "cont"
                    || sCommand.substr(0, 4) == "surf"
                    || sCommand.substr(0, 4) == "mesh")
            {
                sPlotCompose += sLine + " <<COMPOSE>> ";
                // If the block wasn't started from a script, then ask the user
                // to complete it
                if (!(_script.isValid() && _script.isOpen()) && !sCmdCache.length())
                {
                    if (_procedure.getCurrentBlockDepth())
                    {
                        // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                        printPreFmt("|" + _procedure.getCurrentBlock());
                        if (_procedure.getCurrentBlock() == "IF")
                        {
                            if (_procedure.getCurrentBlockDepth() > 1)
                                printPreFmt("---");
                            else
                                printPreFmt("-");
                        }
                        else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getCurrentBlockDepth() > 1)
                            printPreFmt("-");
                        else
                        {
                            if (_procedure.getCurrentBlockDepth() > 1)
                                printPreFmt("--");
                        }
                        printPreFmt(strfill("> ", 2 * _procedure.getCurrentBlockDepth(), '-'));
                    }
                    else if (_procedure.is_writing())
                    {
                        printPreFmt("|PROC> ");
                    }
                    else if (!_procedure.is_writing() && sPlotCompose.length())
                    {
                        printPreFmt("|COMP> ");
                    }

                    // Waiting for input
                    nReturnVal = NUMERE_PENDING_SPECIAL;
                    return false;
                }
            }
            return false;
        }
        else
        {
            // End the current compose block and clear the cache
            // "endcompose" is not needed here
            sLine = sPlotCompose;
            sPlotCompose = "";
        }
    }
    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// handle the writing of procedure lines to the
/// corresponding file.
///
/// \param sLine const std::string&
/// \param sCmdCache const std::string&
/// \param sCurrentCommand const std::string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleProcedureWrite(const std::string& sLine, const std::string& sCmdCache, const std::string& sCurrentCommand, KernelStatus& nReturnVal)
{
    if (_procedure.is_writing() || sCurrentCommand == "procedure")
    {
        if (!_procedure.writeProcedure(sLine))
            print(LineBreak(_lang.get("PARSER_CANNOTCREATEPROC"), _option));
        if (!(_script.isValid() && _script.isOpen()) && !sCmdCache.length())
        {
            if (_procedure.getCurrentBlockDepth())
            {
                // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                printPreFmt("|" + _procedure.getCurrentBlock());
                if (_procedure.getCurrentBlock() == "IF")
                {
                    if (_procedure.getCurrentBlockDepth() > 1)
                        printPreFmt("---");
                    else
                        printPreFmt("-");
                }
                else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getCurrentBlockDepth() > 1)
                    printPreFmt("-");
                else
                {
                    if (_procedure.getCurrentBlockDepth() > 1)
                        printPreFmt("--");
                }
                printPreFmt(strfill("> ", 2 * _procedure.getCurrentBlockDepth(), '-'));
            }
            else if (_procedure.is_writing())
            {
                printPreFmt("|PROC> ");
            }
            else if (!_procedure.is_writing() && sPlotCompose.length())
            {
                printPreFmt("|COMP> ");
            }
            nReturnVal = NUMERE_PENDING_SPECIAL;
            return false;
        }
        return false;
    }
    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function uninstalls
/// a previously installed plugin.
///
/// \param sLine const std::string&
/// \param sCurrentCommand const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::uninstallPlugin(const std::string& sLine, const std::string& sCurrentCommand)
{
    if (sCurrentCommand == "uninstall")
    {
        // Ge the plugin name
        std::string sPlugin = fromSystemCodePage(getArgAtPos(sLine, findCommand(sLine).nPos + 9));

        // Remove the plugin and get the help index ID
        sPlugin = _procedure.deletePackage(sPlugin);

        if (sPlugin.length())
        {
            if (sPlugin != "<<NO_HLP_ENTRY>>")
            {
                while (sPlugin.find(';') != std::string::npos)
                    sPlugin[sPlugin.find(';')] = ',';

                while (sPlugin.length())
                {
                    // Remove the reference from the help index
                    _option.removeFromDocIndex(getNextArgument(sPlugin, true));
                }
            }

            print(LineBreak(_lang.get("PARSER_PLUGINDELETED"), _option));
            installationDone();
        }
        else
            print(LineBreak(_lang.get("PARSER_PLUGINNOTFOUND"), _option));

        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This private member function handles
/// the "to_cmd()" function, if it is part of
/// the current command line.
///
/// \param sLine std::string&
/// \param sCache std::string&
/// \param sCurrentCommand std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::handleToCmd(std::string& sLine, std::string& sCache, std::string& sCurrentCommand)
{
    // Do only something, if "to_cmd()" is located
    if (sLine.find("to_cmd(") != std::string::npos && !_procedure.getCurrentBlockDepth())
    {
        unsigned int nPos = 0;

        // Find all "to_cmd()"'s
        while (sLine.find("to_cmd(", nPos) != std::string::npos)
        {
            nPos = sLine.find("to_cmd(", nPos) + 6;
            if (isInQuotes(sLine, nPos))
                continue;
            unsigned int nParPos = getMatchingParenthesis(sLine.substr(nPos));
            if (nParPos == std::string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);
            std::string sCmdString = sLine.substr(nPos + 1, nParPos - 1);
            StripSpaces(sCmdString);

            // Evaluate the string part
            if (_stringParser.isStringExpression(sCmdString))
            {
                sCmdString += " -nq";
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmdString, sCache, true);
                sCache = "";
            }
            sLine = sLine.substr(0, nPos - 6) + sCmdString + sLine.substr(nPos + nParPos + 1);
            nPos -= 5;
        }

        // Get the current command
        sCurrentCommand = findCommand(sLine).sString;
    }
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// evaluate any calls to procedurs and replace
/// their results in the current command line.
///
/// \param sLine std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::evaluateProcedureCalls(std::string& sLine)
{
    // Only if there's a candidate for a procedure
    if (sLine.find('$') != std::string::npos && sLine.find('(', sLine.find('$')) != std::string::npos && !_procedure.getCurrentBlockDepth())
    {
        unsigned int nPos = 0;
        int nProc = 0;

        // Find all procedures
        while (sLine.find('$', nPos) != std::string::npos && sLine.find('(', sLine.find('$', nPos)) != std::string::npos)
        {
            unsigned int nParPos = 0;
            nPos = sLine.find('$', nPos) + 1;

            // Get procedure name and argument list
            std::string __sName = sLine.substr(nPos, sLine.find('(', nPos) - nPos);
            std::string __sVarList = "";

            if (sLine[nPos] == '\'')
            {
                // This is an explicit file name
                __sName = sLine.substr(nPos + 1, sLine.find('\'', nPos + 1) - nPos - 1);
                nParPos = sLine.find('(', nPos + 1 + __sName.length());
            }
            else
                nParPos = sLine.find('(', nPos);

            __sVarList = sLine.substr(nParPos);
            nParPos += getMatchingParenthesis(sLine.substr(nParPos));
            __sVarList = __sVarList.substr(1, getMatchingParenthesis(__sVarList) - 1);

            // Ensure that the procedure is not part of quotation marks
            if (!isInQuotes(sLine, nPos, true))
            {
                // Execute the current procedure
                Returnvalue _rTemp = _procedure.execute(__sName, __sVarList, _parser, _functions, _memoryManager, _option, _out, _pData, _script);
                if (!_procedure.getReturnType())
                    sLine = sLine.substr(0, nPos - 1) + sLine.substr(nParPos + 1);
                else
                {
                    _procedure.replaceReturnVal(sLine, _parser, _rTemp, nPos - 1, nParPos + 1,
                                                "_~PROC~[" + _procedure.mangleName(__sName) + "~ROOT_" + toString(nProc) + "]");
                    nProc++;
                }
            }

            nPos += __sName.length() + __sVarList.length() + 1;
        }

        StripSpaces(sLine);

        if (!sLine.length())
            return false;
    }
    else if (sLine.find('$') != std::string::npos && sLine.find('(', sLine.find('$')) == std::string::npos)
    {
        // If there's a dollar sign without an opening parenthesis
        // ensure that it is enclosed with quotation marks
        size_t i = sLine.find('$');

        if (findCommand(sLine).sString == "new")
            return true;

        bool isnotinquotes = true;

        // Examine each occurence of a dollar sign
        while (isInQuotes(sLine, i))
        {
            if (sLine.find('$', i + 1) != std::string::npos)
                i = sLine.find('$', i + 1);
            else
            {
                isnotinquotes = false;
                break;
            }
        }

        // If there's one, which is not in quotation marks
        // clear the line and return false
        if (isnotinquotes)
        {
            sLine = "";
            return false;
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// execute any call to a plugin.
///
/// \param sLine std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::executePlugins(std::string& sLine)
{
    // If there's a plugin command
    if (_procedure.isPluginCmd(sLine) && !_procedure.getCurrentBlockDepth())
    {
        // Evaluate the command and store procedure name and argument list internally
        if (_procedure.evalPluginCmd(sLine))
        {
            _option.enableSystemPrints(false);

            // Call the relevant procedure
            Returnvalue _rTemp = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _memoryManager, _option, _out, _pData, _script);

            // Handle the return values
            if (_rTemp.isString() && sLine.find("<<RETURNVAL>>") != std::string::npos)
            {
                std::string sReturn = "{";
                for (unsigned int v = 0; v < _rTemp.vStringVal.size(); v++)
                    sReturn += _rTemp.vStringVal[v] + ",";
                sReturn.back() = '}';
                sLine.replace(sLine.find("<<RETURNVAL>>"), 13, sReturn);
            }
            else if (_rTemp.sReturnedTable.length())
            {
                std::string sTargetTable = sLine.substr(0, sLine.find("<<RETURNVAL>>"));

                if (sTargetTable.find('=') != std::string::npos)
                    sTargetTable.erase(sTargetTable.find_last_not_of(" ="));

                StripSpaces(sTargetTable);

                if (sTargetTable.find('(') != std::string::npos
                    && sTargetTable.substr(sTargetTable.find('(')) == "()"
                    && sLine.substr(sLine.find("<<RETURNVAL>>")+13).find_first_not_of(" ;") == std::string::npos)
                {
                    sTargetTable.erase(sTargetTable.find('('));

                    // Copy efficient move operations
                    if (_rTemp.delayDelete)
                    {
                        if (!_memoryManager.isTable(sTargetTable))
                            _memoryManager.renameTable(_rTemp.sReturnedTable, sTargetTable, true);
                        else
                        {
                            _memoryManager.swapTables(sTargetTable, _rTemp.sReturnedTable);
                            _memoryManager.deleteTable(_rTemp.sReturnedTable);
                        }
                    }
                    else
                        _memoryManager.copyTable(_rTemp.sReturnedTable, sTargetTable);

                    sLine = _parser.CreateTempVectorVar(std::vector<mu::value_type>({_memoryManager.getLines(sTargetTable),
                                                                                     _memoryManager.getCols(sTargetTable)}))
                            + sLine.substr(sLine.find("<<RETURNVAL>>")+13);
                }
                else
                {
                    sLine.replace(sLine.find("<<RETURNVAL>>"), 13, _memoryManager.isEmpty(_rTemp.sReturnedTable) ? "false" : "true");

                    if (_rTemp.delayDelete)
                        _memoryManager.deleteTable(_rTemp.sReturnedTable);
                }
            }
            else if (_rTemp.isNumeric() && sLine.find("<<RETURNVAL>>") != std::string::npos)
            {
                sLine.replace(sLine.find("<<RETURNVAL>>"), 13, "_~PLUGIN[" + _procedure.getPluginProcName() + "~ROOT]");
                _parser.SetVectorVar("_~PLUGIN[" + _procedure.getPluginProcName() + "~ROOT]", _rTemp.vNumVal);
            }
            _option.enableSystemPrints(true);
            if (!sLine.length())
                return false;
        }
        else
        {
            return false;
        }
    }
    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// handle used flow controls.
///
/// \param sLine std::string&
/// \param sCmdCache const std::string&
/// \param sCurrentCommand const std::string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleFlowControls(std::string& sLine, const std::string& sCmdCache, const std::string& sCurrentCommand, KernelStatus& nReturnVal)
{
    if (_procedure.getCurrentBlockDepth() || FlowCtrl::isFlowCtrlStatement(sCurrentCommand) )
    {
        if (bSupressAnswer)
            sLine += ";";
        // --> Die Zeile in den Ausdrucksspeicher schreiben, damit sie spaeter wiederholt aufgerufen werden kann <--
        _procedure.setCommand(sLine, (_script.isValid() && _script.isOpen()) ? _script.getCurrentLine()-1 : 0);
        /* --> So lange wir im Loop sind und nicht endfor aufgerufen wurde, braucht die Zeile nicht an den Parser
         *     weitergegeben werden. Wir ignorieren daher den Rest dieser for(;;)-Schleife <--
         */
        if (!(_script.isValid() && _script.isOpen()) && !sCmdCache.length())
        {
            if (_procedure.getCurrentBlockDepth())
            {
                toggleTableStatus();
                // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                printPreFmt("|" + _procedure.getCurrentBlock());

                if (_procedure.getCurrentBlock() == "IF")
                {
                    if (_procedure.getCurrentBlockDepth() > 1)
                        printPreFmt("---");
                    else
                        printPreFmt("-");
                }
                else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getCurrentBlockDepth() > 1)
                    printPreFmt("-");
                else
                {
                    if (_procedure.getCurrentBlockDepth() > 1)
                        printPreFmt("--");
                }

                toggleTableStatus();
                printPreFmt(strfill("> ", 2 * _procedure.getCurrentBlockDepth(), '-'));
            }
            else if (_procedure.is_writing())
                printPreFmt("|PROC> ");
            else if (!_procedure.is_writing() && sPlotCompose.length() )
                printPreFmt("|COMP> ");
            else
            {
                if (_script.wasLastCommand())
                {
                    print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
                    _memoryManager.setPluginCommands(_procedure.getPluginNames());

                    checkInternalStates();
                }

                bCancelSignal = false;
                nReturnVal = NUMERE_DONE_KEYWORD;
                return false;
            }

            nReturnVal = NUMERE_PENDING_SPECIAL;
            return false;
        }
        else
        {
            // A return command occured in an evaluated flow
            // control block. If the block was created by a
            // script, close the script now.
            if (_procedure.getReturnSignal())
            {
                _script.returnCommand();
                print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));

                checkInternalStates();

                nReturnVal = NUMERE_DONE_KEYWORD;
            }
        }

        return false;
    }
    else if (sCurrentCommand == "return")
    {
        // The current command is return. If the command
        // has been read from a script, close that script
        // now
        if (_script.isOpen())
        {
            _script.returnCommand();

            // Only signal finishing if the script was not
            // already re-opened due to chained installations.
            if (!_script.isOpen())
            {
                print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
                checkInternalStates();
            }
            else
                return true;
        }

        nReturnVal = NUMERE_DONE_KEYWORD;
        return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function redirects
/// the processing of strings to the string parser.
///
/// \param sLine std::string&
/// \param sCache std::string&
/// \param sCmdCache const std::string&
/// \param bWriteToCache bool&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::evaluateStrings(std::string& sLine, std::string& sCache, const std::string& sCmdCache, bool& bWriteToCache, KernelStatus& nReturnVal)
{
    if (_stringParser.isStringExpression(sLine) || _stringParser.isStringExpression(sCache))
    {
        auto retVal = _stringParser.evalAndFormat(sLine, sCache, false, true);

        if (retVal == NumeRe::StringParser::STRING_SUCCESS)
        {
            if (!sCmdCache.length() && !(_script.isValid() && _script.isOpen()))
            {
                if (_script.wasLastCommand())
                {
                    print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
                    _memoryManager.setPluginCommands(_procedure.getPluginNames());
                    checkInternalStates();
                }

                sCommandLine.clear();
                bCancelSignal = false;
                nReturnVal = NUMERE_DONE_KEYWORD;
                return false;
            }
            else
                return false;
        }

#warning NOTE (erik.haenel#3#): This is changed due to double writes in combination with c{nlen+1} = VAL
        //if (sCache.length() && _memoryManager.containsTablesOrClusters(sCache) && !bWriteToCache)
        //    bWriteToCache = true;

        if (sCache.length())
        {
            bWriteToCache = false;
            sCache.clear();
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// create the answer line for the parser which is
/// then passed to NumeReKernel::printResult().
///
/// \param nNum int
/// \param v value_type*
/// \param sCmdCache const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::createCalculationAnswer(int nNum, value_type* v, const std::string& sCmdCache)
{
    vAns = v[0];
    getAns().setDoubleArray(nNum, v);

    if (!bSupressAnswer)
        printResult(formatResultOutput(nNum, v), sCmdCache, _script.isValid() && _script.isOpen());
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// reset the kernel variables after an error had
/// been handled.
///
/// \param sCmdCache std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::resetAfterError(std::string& sCmdCache)
{
    _pData.setFileName("");
    if (sCmdCache.length())
        sCmdCache.clear();
    _parser.DeactivateLoopMode();
    sCommandLine.clear();
    bCancelSignal = false;

    // If script is still open, close it
    if (_script.isOpen() && _script.isValid())
    {
        print(LineBreak(_lang.get("ERR_SCRIPTCATCH", toString((int)_script.getCurrentLine())), _option));
        _script.close();
    }

    // Reset the debugger, if not already done
    _debugger.finalize();
    _procedure.reset();
    _stringParser.removeTempStringVectorVars();
}


/////////////////////////////////////////////////
/// \brief Returns a random greeting string,
/// which may be printed to the terminal later.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReKernel::getGreeting()
{
    NumeRe::DataBase greetingsDB("<>/docs/greetings.ndb");

    // Get the greetings from the database file
    if (_option.useCustomLangFiles() && fileExists(_option.ValidFileName("<>/user/docs/greetings.ndb", ".ndb")))
        greetingsDB.addData("<>/user/docs/greetings.ndb");

    if (!greetingsDB.size())
        return "|-> ERROR: GREETINGS FILE IS EMPTY.\n";

    // return a random greeting
    return "|-> \"" + greetingsDB.getElement(greetingsDB.randomRecord(), 0) + "\"\n";
}


/////////////////////////////////////////////////
/// \brief Evaluates the internal states,
/// performs necessary actions and resets them.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::checkInternalStates()
{
    if (installing)
    {
        installing = false;
        installationDone();
    }

    if (refreshTree)
    {
        refreshTree = false;
        _lang.addToLanguage(getPluginLanguageStrings());

        if (!m_parent)
            return;
        else
        {
            wxCriticalSectionLocker lock(m_parent->m_kernelCS);

            // create the task
            NumeReTask task;
            task.taskType = NUMERE_REFRESH_FUNCTIONTREE;

            taskQueue.push(task);

            m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
        }

        wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    }
}


/////////////////////////////////////////////////
/// \brief This member function is used to update
/// the internal terminal line length information
/// after the terminal had been resized.
///
/// \param nLength int
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::updateLineLenght(int nLength)
{
    if (nLength > 0)
    {
        g_logger.debug("Changing line length.");
        nLINE_LENGTH = nLength;
        _option.getSetting(SETTING_V_WINDOW_X).value() = nLength;
    }
}


/////////////////////////////////////////////////
/// \brief This member function performs the
/// autosaving of tables at application shutdown.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::saveData()
{
    if (!_memoryManager.getSaveStatus())
    {
        g_logger.info("Saving latest table changes.");
        _memoryManager.saveToCacheFile();
        print(LineBreak(_lang.get("MAIN_CACHE_SAVED"), _option));
        Sleep(500);
    }

}


/////////////////////////////////////////////////
/// \brief This member function shuts the kernel
/// down and terminates the kernel instance. Every
/// call to NumeReKernel::getInstance() after a
/// call to this function will produce a
/// segmentation fault.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::CloseSession()
{
    // Does not to be carried out twice
    if (!m_parent)
        return;

    saveData();
    _memoryManager.removeTablesFromMemory();

    // --> Konfiguration aus den Objekten zusammenfassen und anschliessend speichern <--
    _option.getSetting(SETTING_S_SAVEPATH).stringval() = _out.getPath();
    _option.getSetting(SETTING_S_LOADPATH).stringval() = _memoryManager.getPath();
    _option.getSetting(SETTING_S_PLOTPATH).stringval() = _pData.getPath();
    _option.getSetting(SETTING_S_SCRIPTPATH).stringval() = _script.getPath();

    // Save the function definitions
    if (_option.controlDefinitions() && _functions.getDefinedFunctions())
    {
        g_logger.info("Saving function definitions.");
        _functions.save(_option);
        Sleep(100);
    }

    g_logger.info("Saving options.");
    _option.save(_option.getExePath()); // MAIN_QUIT

    // Do some clean-up stuff here
    sCommandLine.clear();
    sAnswer.clear();
    m_parent = nullptr;
}


/////////////////////////////////////////////////
/// \brief This member function is a simple
/// wrapper to read the kernel answer and reset
/// it automatically.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReKernel::ReadAnswer()
{
    std::string sAns = sAnswer;
    sAnswer.clear();
    return sAns;
}


/////////////////////////////////////////////////
/// \brief This function displays the intro text.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::displaySplash()
{
    printPreFmt("NUMERE: FRAMEWORK FÜR NUMERISCHE RECHNUNGEN");
    return;
}


/////////////////////////////////////////////////
/// \brief This member function returns a map of
/// language strings for the installed plugins,
/// which will be used by the global language
/// object in the editor and the symbols browser.
///
/// \return std::map<std::string, std::string>
///
/////////////////////////////////////////////////
std::map<std::string, std::string> NumeReKernel::getPluginLanguageStrings()
{
    std::map<std::string, std::string> mPluginLangStrings;
    for (size_t i = 0; i < _procedure.getPackageCount(); i++)
    {
        if (!_procedure.getPluginCommand(i).length())
            continue;

        std::string sDesc = _procedure.getPluginCommandSignature(i) + "     - " + _procedure.getPackageDescription(i);
        replaceAll(sDesc, "\\\"", "\"");
        mPluginLangStrings["PARSERFUNCS_LISTCMD_CMD_" + toUpperCase(_procedure.getPluginCommand(i)) + "_[PLUGINS]"] = sDesc;
    }
    return mPluginLangStrings;
}


/////////////////////////////////////////////////
/// \brief This member function returns a map of
/// language strings for the declared functions,
/// which will be used by the global language
/// object in the editor and the symbols browser.
///
/// \return std::map<std::string, std::string>
///
/////////////////////////////////////////////////
std::map<std::string, std::string> NumeReKernel::getFunctionLanguageStrings()
{
    std::map<std::string, std::string> mFunctionLangStrings;
    for (size_t i = 0; i < _functions.getDefinedFunctions(); i++)
    {
        std::string sDesc = _functions.getFunctionSignature(i) + "     ARG   - " + _functions.getComment(i);
        while (sDesc.find("\\\"") != std::string::npos)
            sDesc.erase(sDesc.find("\\\""), 1);

        mFunctionLangStrings["PARSERFUNCS_LISTFUNC_FUNC_" + toUpperCase(_functions.getFunctionSignature(i).substr(0, _functions.getFunctionSignature(i).rfind('('))) + "_[DEFINE]"] = sDesc;
    }
    return mFunctionLangStrings;
}


/////////////////////////////////////////////////
/// \brief This member function is used by the
/// syntax highlighter to hightlight the plugin
/// commands.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReKernel::getPluginCommands()
{
    std::vector<std::string> vPluginCommands;

    for (size_t i = 0; i < _procedure.getPackageCount(); i++)
    {
        if (_procedure.getPluginCommand(i).length())
            vPluginCommands.push_back(_procedure.getPluginCommand(i));
    }

    return vPluginCommands;
}


/////////////////////////////////////////////////
/// \brief This member function returns the mode,
/// how a file shall be opened in the editor, when
/// called by the kernel-
///
/// \return int
///
/////////////////////////////////////////////////
int NumeReKernel::ReadOpenFileFlag()
{
    int nFlag = nOpenFileFlag;
    nOpenFileFlag = 0;
    return nFlag;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// documentation for the passed command string
/// as HTML string prepared for the help browser.
///
/// \param sCommand const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReKernel::getDocumentation(const std::string& sCommand)
{
    return doc_HelpAsHTML(sCommand, false, _option);
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// documentation index as a string vector, which
/// can be used to fill the tree in the
/// documentation browser.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReKernel::getDocIndex()
{
    return _option.getDocIndex();
}


/////////////////////////////////////////////////
/// \brief Formats the byte size using KB or MB
/// as units.
///
/// \param bytes double
/// \return std::string
///
/////////////////////////////////////////////////
static std::string formatByteSize(double bytes)
{
    if (bytes > 1024.0)
    {
        bytes /= 1024.0;

        if (bytes > 1024.0)
            return toString(bytes / 1024.0, 5) + " MBytes";

        return toString(bytes, 5) + " KBytes";
    }

    return toString(bytes, 5) + " Bytes";
}


/////////////////////////////////////////////////
/// \brief This member function returns a structure
/// containing all currently declared variables,
/// which can be displayed in the variable viewer.
///
/// \return NumeReVariables
///
/////////////////////////////////////////////////
NumeReVariables NumeReKernel::getVariableList()
{
    NumeReVariables vars;
    constexpr size_t MAXSTRINGLENGTH = 1024;

    mu::varmap_type varmap = _parser.GetVar();
    const std::map<std::string, std::string>& stringmap = _stringParser.getStringVars();
    std::map<std::string, std::pair<size_t, size_t>> tablemap = _memoryManager.getTableMap();
    const std::map<std::string, NumeRe::Cluster>& clustermap = _memoryManager.getClusterMap();
    std::string sCurrentLine;

    if (_memoryManager.getStringElements())
        tablemap["string"] = std::pair<size_t,size_t>(-1, -1);

    // Gather all (global) numerical variables
    for (auto iter = varmap.begin(); iter != varmap.end(); ++iter)
    {
        if ((iter->first).substr(0, 2) == "_~")
            continue;

        if ((*iter->second).imag() && !(isnan((*iter->second).real()) && isnan((*iter->second).imag())))
            sCurrentLine = iter->first + "\t1 x 1\tcomplex\t" + toString(*iter->second, DEFAULT_NUM_PRECISION*2) + "\t" + iter->first + "\t" + formatByteSize(16);
        else
            sCurrentLine = iter->first + "\t1 x 1\tdouble\t" + toString(*iter->second, DEFAULT_NUM_PRECISION) + "\t" + iter->first + "\t" + formatByteSize(8);

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nNumerics = vars.vVariables.size();

    // Gather all (global) string variables
    for (auto iter = stringmap.begin(); iter != stringmap.end(); ++iter)
    {
        if ((iter->first).substr(0, 2) == "_~")
            continue;

        sCurrentLine = iter->first
            + "\t1 x 1\tstring\t\""
            + replaceControlCharacters(ellipsize(iter->second, MAXSTRINGLENGTH)) + "\"\t"
            + iter->first + "\t" + formatByteSize(iter->second.length());
        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nStrings = vars.vVariables.size() - vars.nNumerics;

    // Gather all (global) tables
    for (auto iter = tablemap.begin(); iter != tablemap.end(); ++iter)
    {
        if ((iter->first).substr(0, 2) == "_~")
            continue;

        if (iter->first == "string")
        {
            sCurrentLine = iter->first + "()\t" + toString(_memoryManager.getStringElements()) + " x " + toString(_memoryManager.getStringCols());
            sCurrentLine += "\tstring\t{\"" + replaceControlCharacters(_memoryManager.minString()) + "\", ..., \"" + replaceControlCharacters(_memoryManager.maxString()) + "\"}\tstring()\t" + formatByteSize(_memoryManager.getStringSize());
        }
        else
        {
            sCurrentLine = iter->first + "()\t" + toString(_memoryManager.getLines(iter->first, false)) + " x " + toString(_memoryManager.getCols(iter->first, false));
            sCurrentLine += "\ttable\t{" + toString(_memoryManager.min(iter->first, "")[0], DEFAULT_MINMAX_PRECISION) + ", ..., " + toString(_memoryManager.max(iter->first, "")[0], DEFAULT_MINMAX_PRECISION) + "}\t" + iter->first + "()\t" + formatByteSize(_memoryManager.getBytes(iter->first));
        }

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nTables = vars.vVariables.size() - vars.nNumerics - vars.nStrings;

    // Gather all (global) clusters
    for (auto iter = clustermap.begin(); iter != clustermap.end(); ++iter)
    {
        if ((iter->first).substr(0, 2) == "_~")
            continue;

        sCurrentLine = iter->first + "{}\t" + toString(iter->second.size()) + " x 1";
        sCurrentLine += "\tcluster\t" + replaceControlCharacters(iter->second.getShortVectorRepresentation())
            + "\t" + iter->first + "{}\t" + formatByteSize(iter->second.getBytes());

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nClusters = vars.vVariables.size() - vars.nNumerics - vars.nStrings - vars.nTables;

    return vars;
}


/////////////////////////////////////////////////
/// \brief Returns \c true, if the user changed
/// any internal settings using the \c set
/// command.
///
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::SettingsModified()
{
    bool modified = modifiedSettings;
    modifiedSettings = false;
    return modified;
}


/////////////////////////////////////////////////
/// \brief This member function returns a vector
/// containing all currently declared paths in a
/// distinct order.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReKernel::getPathSettings() const
{
    std::vector<std::string> vPaths;
    vPaths.push_back(_option.getExePath()); //0
    vPaths.push_back(_option.getWorkPath()); //1
    vPaths.push_back(_option.getLoadPath()); //2
    vPaths.push_back(_option.getSavePath()); //3
    vPaths.push_back(_option.getScriptPath()); //4
    vPaths.push_back(_option.getProcPath()); //5
    vPaths.push_back(_option.getPlotPath()); //6

    return vPaths;
}


/////////////////////////////////////////////////
/// \brief Returns a vector containing the names
/// and the version info of each installed plugin.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReKernel::getInstalledPackages() const
{
    std::vector<std::string> vPackages;

    for (size_t i = 0; i < _procedure.getPackageCount(); i++)
    {
        vPackages.push_back(_procedure.getPackageName(i) + "\t" + _procedure.getPackageVersion(i));
    }

    return vPackages;
}


/////////////////////////////////////////////////
/// \brief Returns the menu map used to construct
/// the package menu.
///
/// \return std::map<std::string, std::string>
///
/////////////////////////////////////////////////
std::map<std::string, std::string> NumeReKernel::getMenuMap() const
{
    return _procedure.getMenuMap();
}


/////////////////////////////////////////////////
/// \brief This member function appends the
/// formatted string to the buffer and informs the
/// terminal that we have a new string to print.
///
/// \param sLine const std::string&
/// \param sCmdCache const std::string&
/// \param bScriptRunning bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::printResult(const std::string& sLine, const std::string& sCmdCache, bool bScriptRunning)
{
    if (!m_parent)
        return;

    //if (sCmdCache.length() || bScriptRunning )
    //{
    if (bSupressAnswer)
        return;

    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        m_parent->m_sAnswer += "|-> " + sLine + "\n";

        if (m_parent->m_KernelStatus < NumeReKernel::NUMERE_STATUSBAR_UPDATE || m_parent->m_KernelStatus == NumeReKernel::NUMERE_ANSWER_READ)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_CALC_UPDATE;
    }

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(5);
    //}
    //else
    //    sAnswer = sLine;
}


/////////////////////////////////////////////////
/// \brief This member function masks the dollar
/// signs in the strings to avoid that the line
/// breaking functions uses them as aliases for
/// line breaks.
///
/// \param sLine std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReKernel::maskProcedureSigns(std::string sLine)
{
    for (size_t i = 0; i < sLine.length(); i++)
    {
        if (sLine[i] == '$' && (!i || sLine[i - 1] != '\\'))
            sLine.insert(i, 1, '\\');
    }

    return sLine;
}


/////////////////////////////////////////////////
/// \brief This member function appends the
/// passed string as a new output line to the
/// buffer and informs the terminal that we have
/// a new string to print.
///
/// \param __sLine const std::string&
/// \param printingEnabled bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::print(const std::string& __sLine, bool printingEnabled)
{
    if (!m_parent || !printingEnabled)
        return;
    else
    {
        std::string sLine = __sLine;

        if (bErrorNotification)
        {
            if (sLine.front() == '\r')
                sLine.insert(1, 1, (char)15);
            else
                sLine.insert(0, 1, (char)15);

            for (size_t i = 0; i < sLine.length(); i++)
            {
                if (i < sLine.length() - 2 && sLine[i] == '\n')
                    sLine.insert(i + 1, 1, (char)15);
            }
        }

        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        m_parent->m_sAnswer += "|-> " + sLine + "\n";

        if (m_parent->m_KernelStatus < NumeReKernel::NUMERE_STATUSBAR_UPDATE || m_parent->m_KernelStatus == NumeReKernel::NUMERE_ANSWER_READ)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_PRINTLINE;
    }

    if (bWritingTable)
        return;

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(KERNEL_PRINT_SLEEP);
}


/////////////////////////////////////////////////
/// \brief  This member function appends the pre-
/// formatted string to the buffer and informs the
/// terminal that we have a new string to print.
///
/// \param __sLine const std::string&
/// \param printingEnabled bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::printPreFmt(const std::string& __sLine, bool printingEnabled)
{
    if (!m_parent || !printingEnabled)
        return;
    else
    {
        std::string sLine = __sLine;

        if (bErrorNotification)
        {
            if (sLine.front() == '\r')
                sLine.insert(1, 1, (char)15);
            else
                sLine.insert(0, 1, (char)15);

            for (size_t i = 0; i < sLine.length(); i++)
            {
                if (i < sLine.length() - 2 && sLine[i] == '\n')
                    sLine.insert(i + 1, 1, (char)15);
            }
        }

        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        m_parent->m_sAnswer += sLine;

        if (m_parent->m_KernelStatus < NumeReKernel::NUMERE_STATUSBAR_UPDATE || m_parent->m_KernelStatus == NumeReKernel::NUMERE_ANSWER_READ)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_PRINTLINE_PREFMT;
    }

    if (bWritingTable)
        return;

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(KERNEL_PRINT_SLEEP);
}


/////////////////////////////////////////////////
/// \brief This static function is used to format
/// the result output in the terminal for
/// numerical-only results.
///
/// \param nNum int
/// \param v value_type*
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReKernel::formatResultOutput(int nNum, value_type* v)
{
    Settings& _option = getInstance()->getSettings();

    if (nNum > 1)
    {
        // More than one result
        //
        // How many fit into one line?
        int nLineBreak = numberOfNumbersPerLine();
        std::string sAns = "ans = {";

        // compose the result
        for (int i = 0; i < nNum; ++i)
        {
            sAns += strfill(toString(v[i], _option.getPrecision()), _option.getPrecision() + TERMINAL_FORMAT_FIELD_LENOFFSET);

            if (i < nNum - 1)
                sAns += ", ";

            if (nNum + 1 > nLineBreak && !((i + 1) % nLineBreak) && i < nNum - 1)
                sAns += "...\n|          ";
        }

        sAns += "}";

        // return the composed result
        return sAns;
    }
    else
    {
        // Only one result
        // return the answer
        return "ans = " + toString(v[0], _option.getPrecision());
    }

    // fallback
    return "";
}


/////////////////////////////////////////////////
/// \brief This static function is used to format
/// the result output in the terminal for string
/// and numerical results converted to a string
/// vector.
///
/// \param vStringResults const std::vector<std::string>&
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReKernel::formatResultOutput(const std::vector<std::string>& vStringResults)
{
    const Settings& _option = getInstance()->getSettings();

    if (vStringResults.size() > 1)
    {
        // More than one result
        //
        // How many fit into one line?
        size_t nLineBreak = numberOfNumbersPerLine();
        std::string sAns = "ans = {";
        size_t nNum = vStringResults.size();

        // compose the result
        for (size_t i = 0; i < nNum; ++i)
        {
            sAns += strfill(truncString(vStringResults[i], _option.getPrecision()+TERMINAL_FORMAT_FIELD_LENOFFSET-1), _option.getPrecision() + TERMINAL_FORMAT_FIELD_LENOFFSET);

            if (i < nNum - 1)
                sAns += ", ";

            if (nNum + 1 > nLineBreak && !((i + 1) % nLineBreak) && i < nNum - 1)
                sAns += "...\n|          ";
        }

        sAns += "}";

        // return the composed result
        return sAns;
    }
    else
    {
        // Only one result
        // return the answer
        return "ans = " + vStringResults.front();
    }

    // fallback
    return "";
}


/////////////////////////////////////////////////
/// \brief This static function may be used to
/// issue a warning to the user. The warning will
/// be printed by the terminal in a separate
/// colour.
///
/// \param sWarningMessage std::string
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::issueWarning(std::string sWarningMessage)
{
    if (!m_parent)
        return;
    else
    {
        g_logger.warning(sWarningMessage);
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // Insert warning symbols, if linebreaks are contained in this message
        replaceAll(sWarningMessage, "\n", "\n|!> ");

        m_parent->m_sAnswer += "\r|!> " + _lang.get("COMMON_WARNING") + ": " + sWarningMessage + "\n";

        if (m_parent->m_KernelStatus < NumeReKernel::NUMERE_STATUSBAR_UPDATE || m_parent->m_KernelStatus == NumeReKernel::NUMERE_ANSWER_READ)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_ISSUE_WARNING;

    }

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10*KERNEL_PRINT_SLEEP);
}


/////////////////////////////////////////////////
/// \brief This static function may be used to
/// print a test failure message in the terminal.
///
/// \param sFailMessage std::string
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::failMessage(std::string sFailMessage)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // Insert warning symbols, if linebreaks are contained in this message
        replaceAll(sFailMessage, "\n", "\n|!> ");
        replaceAll(sFailMessage, "$", " ");

        m_parent->m_sAnswer += "\r|!> " + sFailMessage + "\n";

        if (m_parent->m_KernelStatus < NumeReKernel::NUMERE_STATUSBAR_UPDATE || m_parent->m_KernelStatus == NumeReKernel::NUMERE_ANSWER_READ)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_ISSUE_WARNING;

    }

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10*KERNEL_PRINT_SLEEP);
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// count of numbers fitting into a single
/// terminal line depending on its line length.
///
/// \return int
///
/////////////////////////////////////////////////
int NumeReKernel::numberOfNumbersPerLine()
{
    /* --> Wir berechnen die Anzahl an Zahlen, die in eine Zeile passen, automatisch <--
     * Links: 11 Zeichen bis [; rechts: vier Zeichen mit EOL;
     * Fuer jede Zahl: 1 Vorzeichen, 1 Dezimalpunkt, 5 Exponentenstellen, Praezision Ziffern, 1 Komma und 1 Leerstelle
     */
    return (getInstance()->getSettings().getWindow() - 1 - 15) / (getInstance()->getSettings().getPrecision() + TERMINAL_FORMAT_FIELD_LENOFFSET+2);
}


/////////////////////////////////////////////////
/// \brief This member function is used to toggle
/// the error notification status. The error
/// notification is used to style text as error
/// messages.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::sendErrorNotification()
{
    bErrorNotification = !bErrorNotification;
}


/////////////////////////////////////////////////
/// \brief Simple static helper function to
/// render the actual progress bar.
///
/// \param nValue int
/// \param nLength size_t
/// \param sProgressChar const std::string&
/// \param sWhitespaceChar const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string drawBar(int nValue, size_t nLength, const std::string& sProgressChar, const std::string& sWhitespaceChar)
{
    return "[" + strfill(sProgressChar, nValue, sProgressChar[0]) + strfill(sWhitespaceChar, nLength - nValue, sWhitespaceChar[0]) + "]";
}


/////////////////////////////////////////////////
/// \brief This function displays a progress bar
/// constructed from characters in the terminal.
///
/// \param nStep int
/// \param nFirstStep int
/// \param nFinalStep int
/// \param sType const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::progressBar(int nStep, int nFirstStep, int nFinalStep, const std::string& sType)
{
    int nStatusVal = 0;
    const int BARLENGTH = 40;
    const double BARDIVISOR = 2.5;
    int nDistance = std::abs(nFinalStep - nFirstStep);
    double dRatio = std::abs(nStep - nFirstStep) / (double)nDistance; // 0-1

    // Calculate the current status val in percent depending
    // on the number of elements
    if (nDistance < 9999
        && dRatio * BARLENGTH > std::abs((nStep - 1 - nFirstStep) / (double)nDistance * BARLENGTH))
        nStatusVal = intCast(dRatio * BARLENGTH) * BARDIVISOR;
    else if (nDistance >= 9999
        && dRatio * 100 > std::abs((nStep - 1 - nFirstStep) / (double)nDistance * 100))
        nStatusVal = intCast(dRatio * 100);

    // Do not update unless explicit necessary
    if (nLastStatusVal >= 0 && nLastStatusVal == nStatusVal && (sType != "cancel" && sType != "bcancel"))
        return;

    toggleTableStatus();

    // Show the progress depending on the selected type
    if (sType == "std")
    {
        // Only value
        printPreFmt("\r|-> " + _lang.get("COMMON_EVALUATING") + " ... " + toString(nStatusVal) + " %");
        nLastLineLength = 14 + _lang.get("COMMON_EVALUATING").length();
    }
    else if (sType == "cancel")
    {
        // Cancelation of only value
        printPreFmt("\r|-> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL"));
        nStep = nFinalStep;
    }
    else if (sType == "bar")
    {
        // Bar with value
        printPreFmt("\r|-> " + drawBar(intCast(100*dRatio/BARDIVISOR), BARLENGTH, "#", " ") + " (" + toString(nStatusVal) + " %)");
        nLastLineLength = 14 + BARLENGTH;
    }
    else if (sType == "bcancel")
    {
        // Cancelation of bar with value
        printPreFmt("\r|-> " + drawBar(intCast(nLastStatusVal/BARDIVISOR), BARLENGTH, "#", " ") + " (--- %)");
        nFinalStep = nStep;
    }
    else
    {
        nLastLineLength = 1;

        // Create a buffer
        std::string sLine = "\r|";

        // This is the custom part, where special tokens define
        // values or bars in different shapes and the remaining
        // string is printed directly
        for (unsigned int i = 0; i < sType.length(); i++)
        {
            // Standard bar
            if (sType.substr(i, 5) == "<bar>")
            {
                sLine += drawBar(intCast(100*dRatio/BARDIVISOR), BARLENGTH, "#", " ");
                i += 4;
                nLastLineLength += 2 + BARLENGTH;
                continue;
            }

            // Bar with minus sign as filler
            if (sType.substr(i, 5) == "<Bar>")
            {
                sLine += drawBar(intCast(100*dRatio/BARDIVISOR), BARLENGTH, "#", "-");
                i += 4;
                nLastLineLength += 2 + BARLENGTH;
                continue;
            }

            // Bar with an equal sign as filler
            if (sType.substr(i, 5) == "<BAR>")
            {
                sLine += drawBar(intCast(100*dRatio/BARDIVISOR), BARLENGTH, "#", "=");
                i += 4;
                nLastLineLength += 2 + BARLENGTH;
                continue;
            }

            // Only value
            if (sType.substr(i, 5) == "<val>")
            {
                sLine += toString(nStatusVal);
                i += 4;
                nLastLineLength += 3;
                continue;
            }

            // Value with whitespace as filler
            if (sType.substr(i, 5) == "<Val>")
            {
                sLine += strfill(toString(nStatusVal), 3);
                i += 4;
                nLastLineLength += 3;
                continue;
            }

            // Value with zeroes as filler
            if (sType.substr(i, 5) == "<VAL>")
            {
                sLine += strfill(toString(nStatusVal), 3, '0');
                i += 4;
                nLastLineLength += 3;
                continue;
            }

            // Append the remaining part
            sLine += sType[i];
            nLastLineLength++;
        }

        // Print the line buffer
        printPreFmt(sLine);
    }

    if (nLastStatusVal == 0 || nLastStatusVal != nStatusVal)
        nLastStatusVal = nStatusVal;

    if (nFinalStep == nStep)
    {
        printPreFmt("\n");
        nLastStatusVal = 0;
        nLastLineLength = 0;
    }

    flush();
    toggleTableStatus();
}


/////////////////////////////////////////////////
/// \brief This member function handles opening
/// files and jumping to lines as requested by the
/// kernel.
///
/// \param sFile const std::string&
/// \param nLine unsigned int
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::gotoLine(const std::string& sFile, unsigned int nLine)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // Create the task
        NumeReTask task;
        task.sString = sFile;
        task.nLine = nLine;
        task.taskType = NUMERE_EDIT_FILE;

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10);
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// display of a documentation window as requested
/// by the kernel.
///
/// \param _sDocumentation const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::setDocumentation(const std::string& _sDocumentation)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // Create the task
        NumeReTask task;
        task.sString = _sDocumentation;
        task.taskType = NUMERE_OPEN_DOC;

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10);
}


/////////////////////////////////////////////////
/// \brief Notify the GUI that the installation
/// was processed.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::installationDone()
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // Create the task
        NumeReTask task;
        task.taskType = NUMERE_INSTALLATION_DONE;

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10);
}


/////////////////////////////////////////////////
/// \brief This member function passes a table to
/// the GUI to be displayed in the table viewer.
/// It also allows to create a table editor window.
///
/// \param _table NumeRe::Table
/// \param __name std::string
/// \param openeditable bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::showTable(NumeRe::Table _table, std::string __name, bool openeditable)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // Create the task
        NumeReTask task;
        task.sString = __name + "()";
        task.table = _table;

        // Use the corresponding task type
        if (openeditable)
            task.taskType = NUMERE_EDIT_TABLE;
        else
            task.taskType = NUMERE_SHOW_TABLE;

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10);
}


/////////////////////////////////////////////////
/// \brief This member function passes a string
/// table to the GUI to be displayed in the table
/// viewer.
///
/// \param _stringtable NumeRe::Container<std::string>
/// \param __name std::string
/// \param openeditable bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::showStringTable(NumeRe::Container<std::string> _stringtable, std::string __name, bool openeditable)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // Create the task
        NumeReTask task;
        task.sString = __name;
        task.stringTable = _stringtable;

        // Use the corresponding task type
        if (openeditable)
            task.taskType = NUMERE_EDIT_TABLE;
        else
            task.taskType = NUMERE_SHOW_STRING_TABLE;

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10);
}


/////////////////////////////////////////////////
/// \brief This member function passes a window
/// object to the user interface, which will then
/// converted into a real window.
///
/// \param window const NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::showWindow(const NumeRe::Window& window)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // create the task
        NumeReTask task;
        task.window = window;
        task.taskType = NUMERE_SHOW_WINDOW;

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
    }

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10);
}


/////////////////////////////////////////////////
/// \brief This member function is used by the
/// kernel to be notified when the user finished
/// the table edit process and the table may be
/// updated internally.
///
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table NumeReKernel::getTable()
{
    if (!m_parent)
        return NumeRe::Table();

    bool bHasTable = false;
    bool bWasCanceled = false;

    do
    {
        Sleep(100);
        {
            wxCriticalSectionLocker lock(m_parent->m_kernelCS);
            bHasTable = m_parent->m_bTableEditAvailable;
            bWasCanceled = m_parent->m_bTableEditCanceled;
            m_parent->m_bTableEditAvailable = false;
            m_parent->m_bTableEditCanceled = false;
        }
    }
    while (!bHasTable && !bWasCanceled);

    if (bWasCanceled)
        return NumeRe::Table();

    return table;
}


/////////////////////////////////////////////////
/// \brief This member function creates the table
/// container for the selected numerical table.
///
/// \param sTableName const std::string&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table NumeReKernel::getTable(const std::string& sTableName)
{
    std::string sSelectedTable = sTableName;

    if (sSelectedTable.find("()") != std::string::npos)
        sSelectedTable.erase(sSelectedTable.find("()"));

    if (!_memoryManager.isTable(sSelectedTable) || !_memoryManager.getCols(sSelectedTable))
        return NumeRe::Table();

    return _memoryManager.extractTable(sSelectedTable);
}


/////////////////////////////////////////////////
/// \brief This member function creates the table
/// container for the string table or the clusters.
///
/// \param sStringTableName const std::string&
/// \return NumeRe::Container<std::string>
///
/////////////////////////////////////////////////
NumeRe::Container<std::string> NumeReKernel::getStringTable(const std::string& sStringTableName)
{
    if (sStringTableName == "string()")
    {
        // Create the container for the string table
        NumeRe::Container<std::string> stringTable(_memoryManager.getStringElements(), _memoryManager.getStringCols());

        for (size_t j = 0; j < _memoryManager.getStringCols(); j++)
        {
            for (size_t i = 0; i < _memoryManager.getStringElements(j); i++)
            {
                stringTable.set(i, j, "\"" + _memoryManager.readString(i, j) + "\"");
            }
        }

        return stringTable;
    }
    else if (_memoryManager.isCluster(sStringTableName))
    {
        // Create the container for the selected cluster
        NumeRe::Cluster& clust = _memoryManager.getCluster(sStringTableName.substr(0, sStringTableName.find("{}")));
        NumeRe::Container<std::string> stringTable(clust.size(), 1);

        for (size_t i = 0; i < clust.size(); i++)
        {
            if (clust.getType(i) == NumeRe::ClusterItem::ITEMTYPE_STRING)
                stringTable.set(i, 0, clust.getString(i));
            else
                stringTable.set(i, 0, toString(clust.getDouble(i), 5));
        }

        return stringTable;
    }

    return NumeRe::Container<std::string>();
}


/////////////////////////////////////////////////
/// \brief This member function passes the
/// debugging information to the GUI to be
/// displayed in the debugger window.
///
/// \param sTitle const std::string&
/// \param vStacktrace const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::showDebugEvent(const std::string& sTitle, const std::vector<std::string>& vStacktrace)
{
    if (!m_parent)
        return;
    else
    {
        g_logger.debug("Debugger event.");
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        NumeReTask task;
        task.taskType = NUMERE_DEBUG_EVENT;

        // note the size of the fields
        task.vDebugEvent.push_back(sTitle);
        task.vDebugEvent.insert(task.vDebugEvent.end(), vStacktrace.begin(), vStacktrace.end());

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
        m_parent->m_nDebuggerCode = 0;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10);
}


/////////////////////////////////////////////////
/// \brief This static function waits until the
/// user sends a continuation command via the
/// debugger and returns the corresponding
/// debugger code.
///
/// \return int
///
/////////////////////////////////////////////////
int NumeReKernel::waitForContinue()
{
    if (!m_parent)
        return DEBUGGER_CONTINUE;

    int nDebuggerCode = 0;
    g_logger.debug("Waiting for continue.");

    // Periodically check for an updated debugger code
    do
    {
        Sleep(100);
        {
            wxCriticalSectionLocker lock(m_parent->m_kernelCS);
            nDebuggerCode = m_parent->m_nDebuggerCode;
            m_parent->m_nDebuggerCode = 0;
        }
    }
    while (!nDebuggerCode);

    // Return the obtained debugger code
    return nDebuggerCode;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of the debugger information for a
/// script debugger breakpoint and returns the
/// obtained debugger code.
///
/// \param sCurrentCommand const std::string&
/// \return int
///
/////////////////////////////////////////////////
int NumeReKernel::evalDebuggerBreakPoint(const std::string& sCurrentCommand)
{
    if (!getInstance())
        return DEBUGGER_CONTINUE;

    std::map<std::string, std::pair<std::string, mu::value_type*>> mLocalVars;
    std::map<std::string, std::pair<std::string, std::string>> mLocalStrings;
    std::map<std::string, std::string> mLocalTables;
    std::map<std::string, std::string> mLocalClusters;
    std::map<std::string, std::string> mArguments;

    // Obtain references to the debugger and the parser
    NumeReDebugger& _debugger = getInstance()->getDebugger();
    const Parser& _parser = getInstance()->getParser();

    // Get the numerical variable map
    const mu::varmap_type& varmap = _parser.GetVar();

    for (auto iter : varmap)
        mLocalVars[iter.first] = std::make_pair(iter.first, iter.second);

    // Get the string variable map
    const std::map<std::string, std::string>& sStringMap = getInstance()->getStringParser().getStringVars();

    for (const auto& iter : sStringMap)
        mLocalStrings[iter.first] = std::make_pair(iter.first, iter.second);

    // Get the table variable map
    std::map<std::string, std::pair<size_t,size_t>> tableMap = getInstance()->getMemoryManager().getTableMap();

    if (getInstance()->getMemoryManager().getStringElements())
        tableMap["string"] = std::pair<size_t, size_t>(-1, -1);

    for (const auto& iter : tableMap)
        mLocalTables[iter.first] = iter.first;

    // Get the cluster mao
    const std::map<std::string, NumeRe::Cluster>& clusterMap = getInstance()->getMemoryManager().getClusterMap();

    for (const auto& iter : clusterMap)
        mLocalClusters[iter.first] = iter.first;


    // Pass the created information to the debugger
    _debugger.gatherInformations(mLocalVars, mLocalStrings, mLocalTables, mLocalClusters, mArguments,
                                 sCurrentCommand, getInstance()->getScript().getScriptFileName(), getInstance()->getScript().getCurrentLine()-1);

    // Show the breakpoint and wait for the
    // user interaction
    return _debugger.showBreakPoint();
}


/////////////////////////////////////////////////
/// \brief Clear the terminal
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::clcTerminal()
{
    if (!m_parent)
        return;
    else
    {
        g_logger.debug("Clearing terminal.");
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // create the task
        NumeReTask task;
        task.taskType = NUMERE_CLC_TERMINAL;

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
    }

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10*KERNEL_PRINT_SLEEP);
}

/////////////////////////////////////////////////
/// \brief This member function informs the GUI
/// to reload the contents of the function tree
/// as soon as possible.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::refreshFunctionTree()
{
    g_logger.debug("Refreshing function tree.");
    getInstance()->refreshTree = true;
}


/////////////////////////////////////////////////
/// \brief This member function informs the GUI
/// to close all windows of the selected type.
/// Use 0 or WT_ALL to close all closable windows
/// at once.
///
/// \param type int
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::closeWindows(int type)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // create the task
        NumeReTask task;
        task.taskType = NUMERE_CLOSE_WINDOWS;
        task.nLine = type;

        taskQueue.push(task);

        m_parent->m_KernelStatus = NUMERE_QUEUED_COMMAND;
    }

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(KERNEL_PRINT_SLEEP);
}


/////////////////////////////////////////////////
/// \brief This function is an implementation
/// replacing the std::getline() function.
///
/// \param sLine std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::getline(std::string& sLine)
{
    if (!m_parent)
        return;

    // Inform the terminal that we'd like to get
    // a textual input from the user through the
    // terminal and that this is not to be noted
    // in the history.
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        bGettingLine = true;
    }

    bool bgotline = false;

    // Check regularily, if the user provided
    // some input
    do
    {
        Sleep(100);
        {
            wxCriticalSectionLocker lock(m_parent->m_kernelCS);
            bgotline = m_parent->m_bCommandAvailable;
            sLine = m_parent->m_sCommandLine;
            m_parent->m_bCommandAvailable = false;
            m_parent->m_sCommandLine.clear();
        }
    }
    while (!bgotline);

    // Inform the terminal that we're finished with
    // getline()
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        bGettingLine = false;
    }

    StripSpaces(sLine);
}


/////////////////////////////////////////////////
/// \brief Toggles the table writing status, which
/// will reduce the number or events send to the
/// terminal.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::toggleTableStatus()
{
    bWritingTable = !bWritingTable;
}


/////////////////////////////////////////////////
/// \brief Inform the terminal to write the
/// current output buffer as soon as possible.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::flush()
{
    if (!m_parent)
        return;

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(1);
}


/////////////////////////////////////////////////
/// \brief This function is used by the kernel to
/// get informed, when the user pressed ESC or
/// used other means of aborting the current
/// calculation process.
///
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::GetAsyncCancelState()
{
    bool bCancel = bCancelSignal;
    bCancelSignal = false;

    return bCancel;
}


/////////////////////////////////////////////////
/// \brief This function prints a horizontal line
/// to the terminal using either minus or equal
/// signs.
///
/// \param nLength int
/// \return void
///
/////////////////////////////////////////////////
void make_hline(int nLength)
{
    if (nLength == -1)
        NumeReKernel::printPreFmt("\r" + strfill(std::string(1, '='), NumeReKernel::nLINE_LENGTH - 1, '=') + "\n");
    else if (nLength < -1)
        NumeReKernel::printPreFmt("\r" + strfill(std::string(1, '-'), NumeReKernel::nLINE_LENGTH - 1, '-') + "\n");
    else
        NumeReKernel::printPreFmt("\r" + strfill(std::string(1, '='), nLength, '=') + "\n");

    return;
}


