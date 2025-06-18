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
#include "core/datamanagement/database.hpp"
#include "core/io/logger.hpp"
#include "core/plotting/plotting.hpp"
#include "core/procedure/mangler.hpp"
#include "versioninformation.hpp"

#include "core/maths/functionimplementation.hpp"
#include "core/strings/functionimplementation.hpp"

#define KERNEL_PRINT_SLEEP 2
#define TERMINAL_FORMAT_FIELD_LENOFFSET 16
#define DEFAULT_NUM_PRECISION 7
#define DEFAULT_MINMAX_PRECISION 5

#define INT8_MIN (-128)
#define INT16_MIN (-32768)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN  (-9223372036854775807LL - 1)

#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT64_MAX 9223372036854775807LL

#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 0xffffffffU  /* 4294967295U */
#define UINT64_MAX 0xffffffffffffffffULL /* 18446744073709551615ULL */

Language _lang;
mglGraph _fontData;
extern mu::Variable vAns;
extern DefaultVariables _defVars;

// Initialization of the static member variables
NumeReKernel* NumeReKernel::kernelInstance = nullptr;
int* NumeReKernel::baseStackPosition = nullptr;
NumeReTerminal* NumeReKernel::m_parent = nullptr;
std::queue<NumeReTask> NumeReKernel::taskQueue;
int NumeReKernel::nLINE_LENGTH = 80;
bool NumeReKernel::bWritingTable = false;
int NumeReKernel::nOpenFileFlag = 0;
int NumeReKernel::nLastStatusVal = -1;
size_t NumeReKernel::nLastLineLength = 0;
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
NumeReKernel::NumeReKernel() : _option(), _memoryManager(), _parser(), _functions(false)
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

    synchronizePathSettings();
}


/////////////////////////////////////////////////
/// \brief This member function synchronizes all
/// core elements' paths with those in the
/// settings object. Intended to be used after
/// settings have changed.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::synchronizePathSettings()
{
    const std::string sExePath = _option.getExePath();
    const std::string sTokens = _option.getTokenPaths();

    // Set paths
    _out.setPath(_option.getSavePath(), false, sExePath);
    _memoryManager.setSavePath(_option.getSavePath());
    _memoryManager.setPath(_option.getLoadPath(), false, sExePath);
    _script.setPath(_option.getScriptPath(), false, sExePath);
    _pData.setPath(_option.getPlotPath(), false, sExePath);
    _procedure.setPath(_option.getProcPath(), false, sExePath);

    // Set tokens
    _out.setTokens(sTokens);
    _memoryManager.setTokens(sTokens);
    _script.setTokens(sTokens);
    _pData.setTokens(sTokens);
    _procedure.setTokens(sTokens);
    _fSys.setTokens(sTokens);
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

    std::string sTime = getTimeStamp(false);
    std::string sLogFile = "numere.log";
    std::string sPath = __sPath;

    // Set the functions provided by the syntax object in the parent class
    _functions.setPredefinedFuncs(sPredefinedFunctions);
    _memoryManager.setPredefinedFuncs(_functions.getPredefinedFuncs());
    _script.setPredefinedFuncs(sPredefinedFunctions);
    _procedure.setPredefinedFuncs(sPredefinedFunctions);

    // Make the path UNIX style
    replaceAll(sPath, "\\", "/");

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
    refreshTree = false;

    // Set the default paths for all objects
    _out.setPath(_option.getSavePath(), true, sPath);
    _out.createRevisionsFolder();

    _memoryManager.setPath(_option.getLoadPath(), true, sPath);
    _memoryManager.createRevisionsFolder();
    _memoryManager.newCluster("ans");

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
        g_logger.info("Loading package definitions.");

        try
        {
            _procedure.loadPlugins();
            _memoryManager.setPluginCommands(_procedure.getPluginNames());
            _lang.addToLanguage(getPluginLanguageStrings());
        }
        catch (...)
        {
            g_logger.error("Could not find or load the plugin definition file.");
        }
    }

    // Load the function definitions
    if (_option.controlDefinitions() && fileExists(_option.getExePath() + "\\functions.def"))
    {
        g_logger.info("Loading custom function definitions.");

        try
        {
            _functions.load(_option, true);
        }
        catch (...)
        {
            g_logger.error("Could not find or load the function definition file.");
        }
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
    _parser.DefineVar("ans", &vAns);
    _parser.DefineVar(_defVars.sName[0], &_defVars.vValue[0][0]);
    _parser.DefineVar(_defVars.sName[1], &_defVars.vValue[1][0]);
    _parser.DefineVar(_defVars.sName[2], &_defVars.vValue[2][0]);
    _parser.DefineVar(_defVars.sName[3], &_defVars.vValue[3][0]);

    // Initialize the default variables to a reasonable
    // default value
    _defVars.vValue[0][0] = mu::Value(0.0);
    _defVars.vValue[1][0] = mu::Value(0.0);
    _defVars.vValue[2][0] = mu::Value(0.0);
    _defVars.vValue[3][0] = mu::Value(0.0);

    // Declare the table dimension variables
    _parser.DefineVar("nlines", &_memoryManager.tableLinesCount);
    _parser.DefineVar("nrows", &_memoryManager.tableLinesCount);
    _parser.DefineVar("ncols", &_memoryManager.tableColumnsCount);
    _parser.DefineVar("nlen", &_memoryManager.dClusterElementsCount);

    // Define the operators
    g_logger.debug("Defining operators.");
    defineOperators();

    // Define the constants
    g_logger.debug("Defining constants.");
    defineConst();

    // Define the functions
    g_logger.debug("Defining functions.");
    defineNumFunctions();
    defineStrFunctions();

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
    _parser.DefinePostfixOprt("'G", unit_Giga);
    _parser.DefinePostfixOprt("'M", unit_Mega);
    _parser.DefinePostfixOprt("'k", unit_Kilo);
    _parser.DefinePostfixOprt("'m", unit_Milli);
    _parser.DefinePostfixOprt("'mu", unit_Micro);
    _parser.DefinePostfixOprt("'n", unit_Nano);

    // --> Einheitenumrechnungen: Werden aufgerufen durch WERT'EINHEIT <--
    _parser.DefinePostfixOprt("'eV", unit_ElectronVolt);
    _parser.DefinePostfixOprt("'fm", unit_Fermi);
    _parser.DefinePostfixOprt("'A", unit_Angstroem);
    _parser.DefinePostfixOprt("'b", unit_Barn);
    _parser.DefinePostfixOprt("'Torr", unit_Torr);
    _parser.DefinePostfixOprt("'AU", unit_AstroUnit);
    _parser.DefinePostfixOprt("'ly", unit_Lightyear);
    _parser.DefinePostfixOprt("'pc", unit_Parsec);
    _parser.DefinePostfixOprt("'mile", unit_Mile);
    _parser.DefinePostfixOprt("'yd", unit_Yard);
    _parser.DefinePostfixOprt("'ft", unit_Foot);
    _parser.DefinePostfixOprt("'in", unit_Inch);
    _parser.DefinePostfixOprt("'cal", unit_Calorie);
    _parser.DefinePostfixOprt("'psi", unit_PSI);
    _parser.DefinePostfixOprt("'kn", unit_Knoten);
    _parser.DefinePostfixOprt("'l", unit_liter);
    _parser.DefinePostfixOprt("'kmh", unit_kmh);
    _parser.DefinePostfixOprt("'mph", unit_mph);
    _parser.DefinePostfixOprt("'TC", unit_Celsius);
    _parser.DefinePostfixOprt("'TF", unit_Fahrenheit);
    _parser.DefinePostfixOprt("'Ci", unit_Curie);
    _parser.DefinePostfixOprt("'Gs", unit_Gauss);
    _parser.DefinePostfixOprt("'Ps", unit_Poise);
    _parser.DefinePostfixOprt("'mol", unit_mol);
    _parser.DefinePostfixOprt("'y", cast_years);
    _parser.DefinePostfixOprt("'wk", cast_weeks);
    _parser.DefinePostfixOprt("'d", cast_days);
    _parser.DefinePostfixOprt("'h", cast_hours);
    _parser.DefinePostfixOprt("'min", cast_minutes);
    _parser.DefinePostfixOprt("'s", cast_seconds);
    _parser.DefinePostfixOprt("!", numfnc_Factorial);
    _parser.DefinePostfixOprt("!!", numfnc_doubleFactorial);
    _parser.DefinePostfixOprt("i", numfnc_imaginaryUnit);

    // --> Operatoren <--
    _parser.DefineOprt("%", oprt_Mod, prMUL_DIV, oaLEFT);
    _parser.DefineOprt("|||", oprt_XOR, prLOGIC, oaLEFT);
    _parser.DefineOprt("|", oprt_BinOR, prLOGIC, oaLEFT);
    _parser.DefineOprt("&", oprt_BinAND, prLOGIC, oaLEFT);
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
    _parser.DefineConst("_electric_const", 8.854187817e-12);
    _parser.DefineConst("_n_avogadro", 6.02214129e23);
    _parser.DefineConst("_k_boltz", 1.3806488e-23);
    _parser.DefineConst("_elem_ladung", 1.602176565e-19);
    _parser.DefineConst("_elem_charge", 1.602176565e-19);
    _parser.DefineConst("_h", 6.62606957e-34);
    _parser.DefineConst("_hbar", 1.054571726e-34);
    _parser.DefineConst("_m_elektron", 9.10938291e-31);
    _parser.DefineConst("_m_electron", 9.10938291e-31);
    _parser.DefineConst("_m_proton", 1.672621777e-27);
    _parser.DefineConst("_m_neutron", 1.674927351e-27);
    _parser.DefineConst("_m_muon", 1.883531475e-28);
    _parser.DefineConst("_m_tau", 3.16747e-27);
    _parser.DefineConst("_magn_feldkonst", 1.25663706144e-6);
    _parser.DefineConst("_magnetic_const", 1.25663706144e-6);
    _parser.DefineConst("_m_erde", 5.9726e24);
    _parser.DefineConst("_m_earth", 5.9726e24);
    _parser.DefineConst("_m_sonne", 1.9885e30);
    _parser.DefineConst("_m_sun", 1.9885e30);
    _parser.DefineConst("_r_erde", 6.378137e6);
    _parser.DefineConst("_r_earth", 6.378137e6);
    _parser.DefineConst("_r_sonne", 6.9551e8);
    _parser.DefineConst("_r_sun", 6.9551e8);
    _parser.DefineConst("true", true);
    _parser.DefineConst("_theta_weinberg", 0.49097621387892);
    _parser.DefineConst("false", false);
    _parser.DefineConst("_2pi", 6.283185307179586476925286766559);
    _parser.DefineConst("_R", 8.3144622);
    _parser.DefineConst("_alpha_fs", 7.2973525698E-3);
    _parser.DefineConst("_mu_bohr", 9.27400968E-24);
    _parser.DefineConst("_mu_kern", 5.05078353E-27);
    _parser.DefineConst("_mu_nuclear", 5.05078353E-27);
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
    _parser.DefineConst("_day_secs", 86400);
    _parser.DefineConst("_hour_secs", 3600 );
    _parser.DefineConst("_week_secs", 604800);
    _parser.DefineConst("_year_secs", 31557600);
    _parser.DefineConst("nan", NAN);
    _parser.DefineConst("inf", INFINITY);
    _parser.DefineConst("void", mu::Value());
    _parser.DefineConst("I", std::complex<double>(0.0, 1.0));
    _parser.DefineConst(errorTypeToString(TYPE_CUSTOMERROR), mu::Value(TYPE_CUSTOMERROR));
    _parser.DefineConst(errorTypeToString(TYPE_SYNTAXERROR), mu::Value(TYPE_SYNTAXERROR));
    _parser.DefineConst(errorTypeToString(TYPE_ASSERTIONERROR), mu::Value(TYPE_ASSERTIONERROR));
    _parser.DefineConst(errorTypeToString(TYPE_MATHERROR), mu::Value(TYPE_MATHERROR));
    _parser.DefineConst("ui8_max", mu::Numerical((uint8_t)UINT8_MAX));
    _parser.DefineConst("i8_max", mu::Numerical((int8_t)INT8_MAX));
    _parser.DefineConst("i8_min", mu::Numerical((int8_t)INT8_MIN));
    _parser.DefineConst("ui16_max", mu::Numerical((uint16_t)UINT16_MAX));
    _parser.DefineConst("i16_max", mu::Numerical((int16_t)INT16_MAX));
    _parser.DefineConst("i16_min", mu::Numerical((int16_t)INT16_MIN));
    _parser.DefineConst("ui32_max", UINT32_MAX);
    _parser.DefineConst("i32_max", INT32_MAX);
    _parser.DefineConst("i32_min", INT32_MIN);
    _parser.DefineConst("ui64_max", UINT64_MAX);
    _parser.DefineConst("i64_max", INT64_MAX);
    _parser.DefineConst("i64_min", INT64_MIN);
    _parser.DefineConst("f64_max", __DBL_MAX__);
    _parser.DefineConst("f64_min", __DBL_MIN__);
    _parser.DefineConst("f64_eps", __DBL_EPSILON__);
    _parser.DefineConst("f32_max", mu::Numerical(__FLT_MAX__));
    _parser.DefineConst("f32_min", mu::Numerical(__FLT_MIN__));
    _parser.DefineConst("f32_eps", mu::Numerical(__FLT_EPSILON__));
    _parser.DefineConst("cf64_max", std::complex<double>(__DBL_MAX__, __DBL_MAX__));
    _parser.DefineConst("cf64_min", std::complex<double>(__DBL_MIN__, __DBL_MIN__));
    _parser.DefineConst("cf64_eps", std::complex<double>(__DBL_EPSILON__, __DBL_EPSILON__));
    _parser.DefineConst("cf32_max", std::complex<float>(__FLT_MAX__, __FLT_MAX__));
    _parser.DefineConst("cf32_min", std::complex<float>(__FLT_MIN__, __FLT_MIN__));
    _parser.DefineConst("cf32_eps", std::complex<float>(__FLT_EPSILON__, __FLT_EPSILON__));
    _parser.DefineConst("loremipsum", "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed eiusmod tempor incidunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat. Quis aute iure reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint obcaecat cupiditat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
}


/////////////////////////////////////////////////
/// \brief This member function declares all
/// mathematical functions.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::defineNumFunctions()
{

    /////////////////////////////////////////////////////////////////////
    // NOTE:
    // If multi-argument functions are declared, think of whether
    // they can be use a column of data sets as their argument list.
    // If not, then they have to be excluded in the multi-argument
    // function search in the parser.
    /////////////////////////////////////////////////////////////////////

    _parser.DefineFun("faculty", numfnc_Factorial);                              // faculty(n)
    _parser.DefineFun("factorial", numfnc_Factorial);                            // factorial(n)
    _parser.DefineFun("dblfacul", numfnc_doubleFactorial);                       // dblfacul(n)
    _parser.DefineFun("dblfact", numfnc_doubleFactorial);                        // dblfact(n)
    _parser.DefineFun("binom", numfnc_Binom);                                    // binom(Wert1,Wert2)
    _parser.DefineFun("num", numfnc_Num);                                        // num(a,b,c,...)
    _parser.DefineFun("cnt", numfnc_Cnt);                                        // num(a,b,c,...)
    _parser.DefineFun("std", numfnc_Std);                                        // std(a,b,c,...)
    _parser.DefineFun("prd", numfnc_product);                                    // prd(a,b,c,...)
    _parser.DefineFun("round", numfnc_round);                                    // round(x,n)
    _parser.DefineFun("rint", numfnc_rint);                                      // rint(x)
    _parser.DefineFun("radian", numfnc_toRadian);                                // radian(alpha)
    _parser.DefineFun("degree", numfnc_toDegree);                                // degree(x)
    _parser.DefineFun("Y", numfnc_SphericalHarmonics);                           // Y(l,m,theta,phi)
    _parser.DefineFun("imY", numfnc_imSphericalHarmonics);                       // imY(l,m,theta,phi)
    _parser.DefineFun("Z", numfnc_Zernike);                                      // Z(n,m,rho,phi)
    _parser.DefineFun("sbessel", numfnc_SphericalBessel);                        // sbessel(n,x)
    _parser.DefineFun("sneumann", numfnc_SphericalNeumann);                      // sneumann(n,x)
    _parser.DefineFun("bessel", numfnc_RegularCylBessel);                        // bessel(n,x)
    _parser.DefineFun("neumann", numfnc_IrregularCylBessel);                     // neumann(n,x)
    _parser.DefineFun("legendre", numfnc_LegendrePolynomial);                    // legendre(n,x)
    _parser.DefineFun("legendre_a", numfnc_AssociatedLegendrePolynomial);        // legendre_a(l,m,x)
    _parser.DefineFun("laguerre", numfnc_LaguerrePolynomial);                    // laguerre(n,x)
    _parser.DefineFun("laguerre_a", numfnc_AssociatedLaguerrePolynomial);        // laguerre_a(n,k,x)
    _parser.DefineFun("hermite", numfnc_HermitePolynomial);                      // hermite(n,x)
    _parser.DefineFun("betheweizsaecker", numfnc_BetheWeizsaecker);              // betheweizsaecker(N,Z)
    _parser.DefineFun("heaviside", numfnc_Heaviside);                            // heaviside(x)
    _parser.DefineFun("phi", numfnc_phi);                                        // phi(x,y)
    _parser.DefineFun("theta", numfnc_theta);                                    // theta(x,y,z)
    _parser.DefineFun("norm", numfnc_Norm);                                      // norm(x,y,z,...)
    _parser.DefineFun("rms", numfnc_Rms);                                        // rms(x,y,z,...)
    _parser.DefineFun("stderr", numfnc_StdErr);                                  // stderr(x,y,z,...)
    _parser.DefineFun("skw", numfnc_Skew);                                       // skw(x,y,z,...)
    _parser.DefineFun("exc", numfnc_Exc);                                        // exc(x,y,z,...)
    _parser.DefineFun("med", numfnc_Med);                                        // med(x,y,z,...)
    _parser.DefineFun("pct", numfnc_Pct);                                        // pct(x,y,z,...)
    _parser.DefineFun("and", numfnc_and);                                        // and(x,y,z,...)
    _parser.DefineFun("or", numfnc_or);                                          // or(x,y,z,...)
    _parser.DefineFun("xor", numfnc_xor);                                        // xor(x,y,z,...)
    _parser.DefineFun("minpos", numfnc_MinPos);                                  // minpos(x,y,z,...)
    _parser.DefineFun("maxpos", numfnc_MaxPos);                                  // maxpos(x,y,z,...)
    _parser.DefineFun("polynomial", numfnc_polynomial);                          // polynomial(x,a0,a1,a2,a3,...)
    _parser.DefineFun("erf", numfnc_erf);                                        // erf(x)
    _parser.DefineFun("erfc", numfnc_erfc);                                      // erfc(x)
    _parser.DefineFun("gamma", numfnc_gamma);                                    // gamma(x)
    _parser.DefineFun("cmp", numfnc_compare);                                    // cmp(crit,a,b,c,...,type)
    _parser.DefineFun("is_string", numfnc_is_string);                            // is_string(EXPR)
    _parser.DefineFun("to_value", numfnc_Identity);                              // to_value(STRING)
    _parser.DefineFun("sleep", numfnc_sleep, false);                             // sleep(millisecnds)
    _parser.DefineFun("version", numfnc_numereversion);                          // version()
    _parser.DefineFun("getompthreads", numfnc_omp_threads);                      // getompthreads()
    _parser.DefineFun("getdpiscale", numfnc_pixelscale);                         // getdpiscale()
    _parser.DefineFun("is_nan", numfnc_isnan);                                   // is_nan(x)
    _parser.DefineFun("is_void", numfnc_isvoid);                                 // is_void(x)
    _parser.DefineFun("range", numfnc_interval);                                 // range(x,left,right)
    _parser.DefineFun("Ai", numfnc_AiryA);                                       // Ai(x)
    _parser.DefineFun("Bi", numfnc_AiryB);                                       // Bi(x)
    _parser.DefineFun("ellipticF", numfnc_EllipticF);                            // ellipticF(x,k)
    _parser.DefineFun("ellipticE", numfnc_EllipticE);                            // ellipticE(x,k)
    _parser.DefineFun("ellipticPi", numfnc_EllipticP);                           // ellipticPi(x,n,k)
    _parser.DefineFun("ellipticD", numfnc_EllipticD);                            // ellipticD(x,k)
    _parser.DefineFun("floor", numfnc_floor);                                    // floor(x)
    _parser.DefineFun("roof", numfnc_roof);                                      // roof(x)
    _parser.DefineFun("ceil", numfnc_roof);                                      // ceil(x)
    _parser.DefineFun("rect", numfnc_rect);                                      // rect(x,x0,x1)
    _parser.DefineFun("ivl", numfnc_ivl);                                        // ivl(x,x0,x1,lb,rb)
    _parser.DefineFun("student_t", numfnc_studentFactor);                        // student_t(number,confidence)
    _parser.DefineFun("gcd", numfnc_gcd);                                        // gcd(x,y)
    _parser.DefineFun("lcm", numfnc_lcm);                                        // lcm(x,y)
    _parser.DefineFun("beta", numfnc_beta);                                      // beta(x,y)
    _parser.DefineFun("zeta", numfnc_zeta);                                      // zeta(n)
    _parser.DefineFun("Cl2", numfnc_clausen);                                    // Cl2(x)
    _parser.DefineFun("psi", numfnc_digamma);                                    // psi(x)
    _parser.DefineFun("psi_n", numfnc_polygamma);                                // psi_n(n,x)
    _parser.DefineFun("Li2", numfnc_dilogarithm);                                // Li2(x)
    _parser.DefineFun("exp", numfnc_exp);                                        // exp(x)
    _parser.DefineFun("abs", numfnc_abs);                                        // abs(x)
    _parser.DefineFun("sqrt", numfnc_sqrt);                                      // sqrt(x)
    _parser.DefineFun("sign", numfnc_sign);                                      // sign(x)
    _parser.DefineFun("log2", numfnc_log2);                                      // log2(x)
    _parser.DefineFun("log10", numfnc_log10);                                    // log10(x)
    _parser.DefineFun("log", numfnc_log10);                                      // log(x)
    _parser.DefineFun("ln", numfnc_ln);                                          // ln(x)
    _parser.DefineFun("log_b", numfnc_log_b);                                    // log_b(b,x)
    _parser.DefineFun("real", numfnc_real);                                      // real(x)
    _parser.DefineFun("imag", numfnc_imag);                                      // imag(x)
    _parser.DefineFun("to_rect", numfnc_polar2rect);                             // to_rect(x)
    _parser.DefineFun("to_polar", numfnc_rect2polar);                            // to_polar(x)
    _parser.DefineFun("conj", numfnc_conj);                                      // conj(x)
    _parser.DefineFun("complex", numfnc_complex);                                // complex(re,im)
    _parser.DefineFun("logtoidx", numfnc_logtoidx);                              // logtoidx({x,y,z...})
    _parser.DefineFun("idxtolog", numfnc_idxtolog);                              // idxtolog({x,y,z...})
    _parser.DefineFun("getorder", numfnc_order);                                 // getorder({x,y,z...})
    _parser.DefineFun("is_equal", numfnc_is_equal);                              // is_equal({x,y,z...})
    _parser.DefineFun("is_ordered", numfnc_is_ordered);                          // is_ordered({x,y,z...})
    _parser.DefineFun("is_unique", numfnc_is_unique);                            // is_unique({x,y,z...})
    _parser.DefineFun("inv_pct", numfnc_pct_inv);                                // inv_pct({x,y,z...}, pct)
    _parser.DefineFun("complement", numfnc_complement);                          // complement({x,y,z...}, {a,b,c,...})
    _parser.DefineFun("union", numfnc_union);                                    // union({x,y,z...}, {a,b,c,...})
    _parser.DefineFun("intersection", numfnc_intersection);                      // intersection({x,y,z...}, {a,b,c,...})
    _parser.DefineFun("getoverlap", numfnc_getOverlap);                          // getoverlap({x1,x2}, {y1,y2})

    _parser.DefineFun("convertunit", unit_conversion, true, 1);                  // convertunit({x,y,..},{u1,u2,...}[,{m1,...}])

    _parser.DefineFun("sinc", numfnc_SinusCardinalis);                           // sinc(x)
    _parser.DefineFun("sin", numfnc_sin);                                        // sin(x)
    _parser.DefineFun("cos", numfnc_cos);                                        // cos(x)
    _parser.DefineFun("tan", numfnc_tan);                                        // tan(x)
    _parser.DefineFun("cot", numfnc_cot);                                        // cot(x)
    _parser.DefineFun("asin", numfnc_asin);                                      // asin(x)
    _parser.DefineFun("acos", numfnc_acos);                                      // acos(x)
    _parser.DefineFun("atan", numfnc_atan);                                      // atan(x)
    _parser.DefineFun("arcsin", numfnc_asin);                                    // arcsin(x)
    _parser.DefineFun("arccos", numfnc_acos);                                    // arccos(x)
    _parser.DefineFun("arctan", numfnc_atan);                                    // arctan(x)
    _parser.DefineFun("sinh", numfnc_sinh);                                      // sinh(x)
    _parser.DefineFun("cosh", numfnc_cosh);                                      // cosh(x)
    _parser.DefineFun("tanh", numfnc_tanh);                                      // tanh(x)
    _parser.DefineFun("asinh", numfnc_asinh);                                    // asinh(x)
    _parser.DefineFun("acosh", numfnc_acosh);                                    // acosh(x)
    _parser.DefineFun("atanh", numfnc_atanh);                                    // atanh(x)
    _parser.DefineFun("arsinh", numfnc_asinh);                                   // arsinh(x)
    _parser.DefineFun("arcosh", numfnc_acosh);                                   // arcosh(x)
    _parser.DefineFun("artanh", numfnc_atanh);                                   // artanh(x)
    _parser.DefineFun("sec", numfnc_sec);                                        // sec(x)
    _parser.DefineFun("csc", numfnc_csc);                                        // csc(x)
    _parser.DefineFun("asec", numfnc_asec);                                      // asec(x)
    _parser.DefineFun("acsc", numfnc_acsc);                                      // acsc(x)
    _parser.DefineFun("sech", numfnc_sech);                                      // sech(x)
    _parser.DefineFun("csch", numfnc_csch);                                      // csch(x)
    _parser.DefineFun("asech", numfnc_asech);                                    // asech(x)
    _parser.DefineFun("acsch", numfnc_acsch);                                    // acsch(x)

    _parser.DefineFun("time", timfnc_time, false);                               // time()
    _parser.DefineFun("clock", timfnc_clock, false);                             // clock()
    _parser.DefineFun("today", timfnc_today, false);                             // today()
    _parser.DefineFun("date", timfnc_date);                                      // date(TIME,TYPE)
    _parser.DefineFun("datetime", timfnc_datetime);                              // datetime(x)
    _parser.DefineFun("weeknum", timfnc_weeknum);                                // weeknum(tDate)
    _parser.DefineFun("as_date", timfnc_as_date, true, 2);                       // as_date(nYear, nMounth, nDay)
    _parser.DefineFun("as_time", timfnc_as_time, true, 4);                       // as_time(nHours, nMinutes, nSeconds, nMilli, nMicro)
    _parser.DefineFun("get_utc_offset", timfnc_get_utc_offset, false);           // get_utc_offset()
    _parser.DefineFun("is_leapyear", timfnc_is_leap_year);                       // is_leap_year(nDate)
    _parser.DefineFun("is_daylightsavingtime", timfnc_is_daylightsavingtime);    // is_daylightsavingtime(nDate)

    _parser.DefineFun("perlin", rndfnc_perlin, true, 6);                         // perlin(x,y,z,seed,freq,oct,pers)
    _parser.DefineFun("ridgedmulti", rndfnc_rigedmultifractal, true, 5);         // ridgedmulti(x,y,z,seed,freq,oct)
    _parser.DefineFun("billownoise", rndfnc_billow, true, 6);                    // billownoise(x,y,z,seed,freq,oct,pers)
    _parser.DefineFun("voronoinoise", rndfnc_voronoi, true, 6);                  // voronoinoise(x,y,z,seed,freq,displ,dist)
    _parser.DefineFun("rand", rndfnc_Random, false, 1);                          // rand(left,right,n)
    _parser.DefineFun("gauss", rndfnc_gRandom, false, 1);                        // gauss(mean,std,n)
    _parser.DefineFun("laplace_rd", rndfnc_laplace_rd, false, 1);                // laplace_rd(sigma,n)
    _parser.DefineFun("laplace_pdf", rndfnc_laplace_pdf);                        // laplace_pdf(x, sigma)
    _parser.DefineFun("laplace_cdf_p", rndfnc_laplace_cdf_p);                    // laplace_cdf_p(x, sigma)
    _parser.DefineFun("laplace_cdf_q", rndfnc_laplace_cdf_q);                    // laplace_cdf_q(x, sigma)
    _parser.DefineFun("laplace_inv_p", rndfnc_laplace_inv_p);                    // laplace_inv_p(p, sigma)
    _parser.DefineFun("laplace_inv_q", rndfnc_laplace_inv_q);                    // laplace_inv_q(q, sigma)
    _parser.DefineFun("cauchy_rd", rndfnc_cauchy_rd, false, 1);                  // cauchy_rd(a,n)
    _parser.DefineFun("cauchy_pdf", rndfnc_cauchy_pdf);                          // cauchy_pdf(x, a)
    _parser.DefineFun("cauchy_cdf_p", rndfnc_cauchy_cdf_p);                      // cauchy_cdf_p(x, a)
    _parser.DefineFun("cauchy_cdf_q", rndfnc_cauchy_cdf_q);                      // cauchy_cdf_q(x, a)
    _parser.DefineFun("cauchy_inv_p", rndfnc_cauchy_inv_p);                      // cauchy_inv_p(p, a)
    _parser.DefineFun("cauchy_inv_q", rndfnc_cauchy_inv_q);                      // cauchy_inv_q(q, a)
    _parser.DefineFun("rayleigh_rd", rndfnc_rayleigh_rd, false, 1);              // rayleigh_rd(sigma,n)
    _parser.DefineFun("rayleigh_pdf", rndfnc_rayleigh_pdf);                      // rayleigh_pdf(x, sigma)
    _parser.DefineFun("rayleigh_cdf_p", rndfnc_rayleigh_cdf_p);                  // rayleigh_cdf_p(x, sigma)
    _parser.DefineFun("rayleigh_cdf_q", rndfnc_rayleigh_cdf_q);                  // rayleigh_cdf_q(x, sigma)
    _parser.DefineFun("rayleigh_inv_p", rndfnc_rayleigh_inv_p);                  // rayleigh_inv_p(p, sigma)
    _parser.DefineFun("rayleigh_inv_q", rndfnc_rayleigh_inv_q);                  // rayleigh_inv_q(q, sigma)
    _parser.DefineFun("landau_rd", rndfnc_landau_rd, false, 1);                  // landau_rd(n)
    _parser.DefineFun("landau_pdf", rndfnc_landau_pdf);                          // landau_pdf(x)
    _parser.DefineFun("alpha_stable_rd", rndfnc_levyAlphaStable_rd, false, 1);   // levyAlphaStable_rd(c, alpha,n)
    _parser.DefineFun("fisher_f_rd", rndfnc_fisher_f_rd, false, 1);              // fisher_f_rd(nu1, nu2,n)
    _parser.DefineFun("fisher_f_pdf", rndfnc_fisher_f_pdf);                      // fisher_f_pdf(x, nu1, nu2)
    _parser.DefineFun("fisher_f_cdf_p", rndfnc_fisher_f_cdf_p);                  // fisher_f_cdf_p(x, nu1, nu2)
    _parser.DefineFun("fisher_f_cdf_q", rndfnc_fisher_f_cdf_q);                  // fisher_f_cdf_q(x, nu1, nu2)
    _parser.DefineFun("fisher_f_inv_p", rndfnc_fisher_f_inv_p);                  // fisher_f_inv_p(p, nu1, nu2)
    _parser.DefineFun("fisher_f_inv_q", rndfnc_fisher_f_inv_q);                  // fisher_f_inv_q(q, nu1, nu2)
    _parser.DefineFun("weibull_rd", rndfnc_weibull_rd, false, 1);                // weibull_rd(a, b,n)
    _parser.DefineFun("weibull_pdf", rndfnc_weibull_pdf);                        // weibull_pdf(x, a, b)
    _parser.DefineFun("weibull_cdf_p", rndfnc_weibull_cdf_p);                    // weibull_cdf_p(x, a, b)
    _parser.DefineFun("weibull_cdf_q", rndfnc_weibull_cdf_q);                    // weibull_cdf_q(x, a, b)
    _parser.DefineFun("weibull_inv_p", rndfnc_weibull_inv_p);                    // weibull_inv_p(p, a, b)
    _parser.DefineFun("weibull_inv_q", rndfnc_weibull_inv_q);                    // weibull_inv_q(q, a, b)
    _parser.DefineFun("student_t_rd", rndfnc_student_t_rd, false, 1);            // student_t_rd(nu,n)
    _parser.DefineFun("student_t_pdf", rndfnc_student_t_pdf);                    // student_t_pdf(x, nu)
    _parser.DefineFun("student_t_cdf_p", rndfnc_student_t_cdf_p);                // student_t_cdf_p(x, nu)
    _parser.DefineFun("student_t_cdf_q", rndfnc_student_t_cdf_q);                // student_t_cdf_q(x, nu)
    _parser.DefineFun("student_t_inv_p", rndfnc_student_t_inv_p);                // student_t_inv_p(p, nu)
    _parser.DefineFun("student_t_inv_q", rndfnc_student_t_inv_q);                // student_t_inv_q(q, nu)

    // Cast functions
    _parser.DefineFun("category", cast_category, true, 1);                       // category(str,val)
    _parser.DefineFun("seconds", cast_seconds);                                  // seconds(val)
    _parser.DefineFun("minutes", cast_minutes);                                  // minutes(val)
    _parser.DefineFun("hours", cast_hours);                                      // hours(val)
    _parser.DefineFun("days", cast_days);                                        // days(val)
    _parser.DefineFun("weeks", cast_weeks);                                      // weeks(val)
    _parser.DefineFun("years", cast_years);                                      // years(val)
    _parser.DefineFun("i8", cast_numerical<int8_t>);                             // i8(x)
    _parser.DefineFun("ui8", cast_numerical<uint8_t>);                           // ui8(x)
    _parser.DefineFun("i16", cast_numerical<int16_t>);                           // i16(x)
    _parser.DefineFun("ui16", cast_numerical<uint16_t>);                         // ui16(x)
    _parser.DefineFun("i32", cast_numerical<int32_t>);                           // i32(x)
    _parser.DefineFun("ui32", cast_numerical<uint32_t>);                         // ui32(x)
    _parser.DefineFun("i64", cast_numerical<int64_t>);                           // i64(x)
    _parser.DefineFun("ui64", cast_numerical<uint64_t>);                         // ui64(x)
    _parser.DefineFun("f32", cast_numerical<float>);                             // f32(x)
    _parser.DefineFun("f64", cast_numerical<double>);                            // f64(x)
    _parser.DefineFun("cf32", cast_numerical_cmplx<float>);                      // cf32(x)
    _parser.DefineFun("cf64", cast_numerical_cmplx<double>);                     // cf64(x)

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
/// \brief This member function declares all
/// string functions.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::defineStrFunctions()
{
    _parser.DefineFun("to_tex", strfnc_to_tex, true, 1);                             // to_tex(str,n)
    _parser.DefineFun("to_uppercase", strfnc_to_uppercase);                          // to_uppercase(str)
    _parser.DefineFun("to_lowercase", strfnc_to_lowercase);                          // to_lowercase(str)
    _parser.DefineFun("to_ansi", strfnc_utf8ToAnsi);                                 // to_ansi(str)
    _parser.DefineFun("to_utf8", strfnc_ansiToUtf8);                                 // to_utf8(str)
    _parser.DefineFun("getenvvar", strfnc_getenvvar);                                // getenvvar(str)
    _parser.DefineFun("getfileparts", strfnc_getFileParts);                          // getfileparts(str)
    _parser.DefineFun("getfilediff", strfnc_getFileDiffs, false);                    // getfilediff(str1,str2) <- file accesses cannot be optimized away
    _parser.DefineFun("getfilelist", strfnc_getfilelist, false, 1);                  // getfilelist(str,n)
    _parser.DefineFun("getfolderlist", strfnc_getfolderlist, false, 1);              // getfolderlist(str,n)
    _parser.DefineFun("strlen", strfnc_strlen);                                      // strlen(str)
    _parser.DefineFun("firstch", strfnc_firstch);                                    // firstch(str)
    _parser.DefineFun("lastch", strfnc_lastch);                                      // lastch(str)
    _parser.DefineFun("getmatchingparens", strfnc_getmatchingparens);                // getmatchingparens(str)
    _parser.DefineFun("ascii", strfnc_ascii);                                        // ascii(str)
    _parser.DefineFun("is_blank", strfnc_isblank);                                   // is_blank(str)
    _parser.DefineFun("is_alnum", strfnc_isalnum);                                   // is_alnum(str)
    _parser.DefineFun("is_alpha", strfnc_isalpha);                                   // is_alpha(str)
    _parser.DefineFun("is_cntrl", strfnc_iscntrl);                                   // is_cntrl(str)
    _parser.DefineFun("is_digit", strfnc_isdigit);                                   // is_digit(str)
    _parser.DefineFun("is_graph", strfnc_isgraph);                                   // is_graph(str)
    _parser.DefineFun("is_lower", strfnc_islower);                                   // is_lower(str)
    _parser.DefineFun("is_print", strfnc_isprint);                                   // is_print(str)
    _parser.DefineFun("is_punct", strfnc_ispunct);                                   // is_punct(str)
    _parser.DefineFun("is_space", strfnc_isspace);                                   // is_space(str)
    _parser.DefineFun("is_upper", strfnc_isupper);                                   // is_upper(str)
    _parser.DefineFun("is_xdigit", strfnc_isxdigit);                                 // is_xdigit(str)
    _parser.DefineFun("is_dirpath", strfnc_isdir);                                   // is_dirpath(str)
    _parser.DefineFun("is_filepath", strfnc_isfile);                                 // is_filepath(str)
    _parser.DefineFun("to_char", strfnc_to_char);                                    // to_char({x,y,z...})
    _parser.DefineFun("findfile", strfnc_findfile, false, 1);                        // findfile(str1,str2)
    _parser.DefineFun("split", strfnc_split, true, 1);                               // split(str1,str2,n)
    _parser.DefineFun("to_time", strfnc_to_time);                                    // to_time(str1,str2)
    _parser.DefineFun("strfnd", strfnc_strfnd, true, 1);                             // strfnd(str1,str2,n)
    _parser.DefineFun("strmatch", strfnc_strmatch, true, 1);                         // strmatch(str1,str2,n)
    _parser.DefineFun("strrfnd", strfnc_strrfnd, true, 1);                           // strrfnd(str1,str2,n)
    _parser.DefineFun("strrmatch", strfnc_strrmatch, true, 1);                       // strrmatch(str1,str2,n)
    _parser.DefineFun("str_not_match", strfnc_str_not_match, true, 1);               // str_not_match(str1,str2,n)
    _parser.DefineFun("str_not_rmatch", strfnc_str_not_rmatch, true, 1);             // str_not_rmatch(str1,str2,n)
    _parser.DefineFun("strfndall", strfnc_strfndall, true, 2);                       // strfndall(str1,str2,n,n)
    _parser.DefineFun("strmatchall", strfnc_strmatchall, true, 2);                   // strmatchall(str1,str2,n,n)
    _parser.DefineFun("findparam", strfnc_findparam, true, 1);                       // findparam(str1,str2,str3)
    _parser.DefineFun("substr", strfnc_substr, true, 1);                             // substr(str,p,l)
    _parser.DefineFun("repeat", strfnc_repeat);                                      // repeat(str,n)
    _parser.DefineFun("timeformat", strfnc_timeformat);                              // timeformat(str,time)
    _parser.DefineFun("weekday", strfnc_weekday, true, 1);                           // weekday(d,opts)
    _parser.DefineFun("char", strfnc_char);                                          // char(str,n)
    _parser.DefineFun("getopt", strfnc_getopt);                                      // getopt(str,n)
    _parser.DefineFun("replace", strfnc_replace);                                    // replace(str,n,m,str)
    _parser.DefineFun("textparse", strfnc_textparse, true, 2);                       // textparse(str,str,p1,p2)
    _parser.DefineFun("locate", strfnc_locate, true, 1);                             // locate({str},str,n)
    _parser.DefineFun("strunique", strfnc_strunique, true, 1);                       // strunique({str},n)
    _parser.DefineFun("strjoin", strfnc_strjoin, true, 2);                           // strjoin({str},str,n)
    _parser.DefineFun("getkeyval", strfnc_getkeyval, true, 2);                       // getkeyval({str},str,arg,n)
    _parser.DefineFun("findtoken", strfnc_findtoken, true, 1);                       // findtoken(str,str,str)
    _parser.DefineFun("replaceall", strfnc_replaceall, true, 2);                     // replaceall(str,str,str,n,m)
    _parser.DefineFun("strip", strfnc_strip, true, 1);                               // strip(str,str,str,n)
    _parser.DefineFun("regex", strfnc_regex, true, 2);                               // regex(str,str,p,l)
    _parser.DefineFun("basetodec", strfnc_basetodec);                                // basetodec(str,str)
    _parser.DefineFun("dectobase", strfnc_dectobase);                                // dectobase(str,val)
    _parser.DefineFun("justify", strfnc_justify, true, 1);                           // justify({str},n)
    _parser.DefineFun("getlasterror", strfnc_getlasterror, false);                   // getlasterror()
    _parser.DefineFun("geterrormsg", strfnc_geterrormessage);                        // geterrormsg(str)
    _parser.DefineFun("getversioninfo", strfnc_getversioninfo);                      // getversioninfo()
    _parser.DefineFun("getuilang", strfnc_getuilang);                                // getuilang()
    _parser.DefineFun("getuserinfo", strfnc_getuserinfo, false);                     // getuserinfo()
    _parser.DefineFun("uuid", strfnc_getuuid, false);                                // getuuid()
    _parser.DefineFun("getodbcdrivers", strfnc_getodbcdrivers, false);               // getuuid()
    _parser.DefineFun("getfileinfo", strfnc_getfileinfo, false);                     // getfileinfo(str)
    _parser.DefineFun("sha256", strfnc_sha256, false, 1);                            // sha256(str,n) <- can access a file
    _parser.DefineFun("encode_base_n", strfnc_encode_base_n, false, 2);              // encode_base_n(str,log,n) <- can access a file
    _parser.DefineFun("decode_base_n", strfnc_decode_base_n, true, 1);               // decode_base_n(str,n)
    _parser.DefineFun("startswith", strfnc_startswith);                              // startswith(str,str)
    _parser.DefineFun("endswith", strfnc_endswith);                                  // endswith(str,str)
    _parser.DefineFun("to_value", strfnc_to_value);                                  // to_value(str)
    _parser.DefineFun("to_string", strfnc_to_string);                                // to_string(val)
    _parser.DefineFun("getindices", strfnc_getindices, false, 1);                    // getindices(str,n) <- tables/clusters may change
    _parser.DefineFun("is_data", strfnc_is_data, false);                             // is_data(str) <- tables/clusters may change
    _parser.DefineFun("is_table", strfnc_is_table, false);                           // is_table(str) <- tables/clusters may change
    _parser.DefineFun("is_cluster", strfnc_is_cluster, false);                       // is_cluster(str) <- tables/clusters may change
    _parser.DefineFun("findcolumn", strfnc_findcolumn, false);                       // findcolumn(str,str) <- tables may change
    _parser.DefineFun("valtostr", strfnc_valtostr, true, 2);                         // valtostr(val,str,l)
    _parser.DefineFun("gettypeof", strfnc_gettypeof);                                // gettypeof(val)
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
    g_logger.debug("Base stack address = " + toHexString((size_t)baseStackPosition));
}


/////////////////////////////////////////////////
/// \brief This member function prints the version
/// headline and the version information to the
/// console.
///
/// \param shortInfo bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::printVersionInfo(bool shortInfo)
{
    bWritingTable = true;
    make_hline(80);
    std::string sAppName = toUpperCase(_lang.get("COMMON_APPNAME"));

    printPreFmt("| " + sAppName + strfill("|\n", 79 - sAppName.length()));
    printPreFmt("| Version: " + getVersion() + strfill("Build: ", 79 - 22 - getVersion().length())
                + printBuildDate() + " |\n");
    printPreFmt("| Copyright (c) 2013-" + getBuildYear() + toSystemCodePage(", Erik A. H\xe4nel et al.")
                + strfill(_lang.get("MAIN_ABOUT_NBR"), 79 - 48) + " |\n");
    make_hline(80);

    if (!shortInfo)
    {
        printPreFmt("|\n");

        if (_option.showGreeting())
            printPreFmt(toSystemCodePage(getGreeting()) + "|\n");

        print(LineBreak(_lang.get("PARSER_INTRO", getUserDisplayName(true)), _option));
    }

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
    std::string sLine = "";          // The actual line
    std::string sCurrentCommand = "";// The current command
    std::queue<std::string> emptyQueue;
    commandQueue.swap(emptyQueue);
    int& nDebuggerCode = _procedure.getDebuggerCode();
    nLastStatusVal = -1;
    nLastLineLength = 0;
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
    if (sCommandLine.ends_with("\\\\"))
    {
        sCommandLine.erase(sCommandLine.length() - 2);
        return NUMERE_PENDING;
    }

    // Check for comments
    size_t nQuotes = 0;

    for (size_t i = 0; i < sCommandLine.length(); i++)
    {
        if (sCommandLine[i] == '"' && (!i || sCommandLine[i-1] != '\\'))
            nQuotes++;

        if (nQuotes % 2)
            continue;

        if (sCommandLine.substr(i, 2) == "##")
        {
            sCommandLine.erase(i);
            break;
        }

        if (sCommandLine.substr(i, 2) == "#*")
        {
            for (size_t j = i+2; j < sCommandLine.length(); j++)
            {
                if (sCommandLine.substr(j, 2) == "*#")
                {
                    sCommandLine.erase(i, j+2-i);
                    break;
                }

                if (j+1 == sCommandLine.length())
                    return NUMERE_PENDING;
            }
        }
    }

    // Pass the combined command line to the internal variable and clear the contents of the class
    // member variable (we don't want to repeat the tasks entered last time)
    sLine = sCommandLine;
    sCommandLine.clear();
    _parser.ClearInternalVars();
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
        _parser.SetVarAliases(nullptr);

        // Try-catch block to handle all the internal exceptions
        try
        {
            // Handle command line sources and validate the input
            if (!handleCommandLineSource(sLine, sKeep))
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
            if (!getLineFromCommandCache(sLine, sCurrentCommand))
                continue;

            // Eval debugger breakpoints from scripts
            if ((sLine.starts_with("|>") || nDebuggerCode == DEBUGGER_STEP)
                && _script.isValid()
                && !_procedure.is_writing()
                && !_procedure.getCurrentBlockDepth())
            {
                Breakpoint bp(true);

                if (sLine.starts_with("|>"))
                {
                    sLine.erase(0, 2);
                    bp = _debugger.getBreakpointManager().getBreakpoint(_script.getScriptFileName(), _script.getCurrentLine()-1);
                }

                if (_option.useDebugger() && nDebuggerCode != DEBUGGER_LEAVE && bp.isActive(false))
                    nDebuggerCode = evalDebuggerBreakPoint(sLine);
            }

            // Log the current line and ensure that possible passwords
            // are hidden
            size_t pos = findParameter(sLine, "pwd", '=');

            if (pos)
            {
                std::string password = getArgAtPos(sLine, pos+3, ARGEXTRACT_NONE);
                std::string sHashed = sLine;
                replaceAll(sHashed, password.c_str(), "\"******\"", pos);
                g_logger.cmdline(sHashed);
            }
            else
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
            if (!handleComposeBlock(sLine, sCurrentCommand, nReturnVal))
            {
                // returns false either when the loop shall return
                // or it shall continue
                if (nReturnVal)
                    return nReturnVal;
                continue;
            }

            // Get the current command
            sCurrentCommand = findCommand(sLine).sString;

            // uninstall the plugin, if desired
            if (uninstallPlugin(sLine, sCurrentCommand))
                return NUMERE_DONE_KEYWORD;

            // Handle the writing of procedures
            nReturnVal = NUMERE_ERROR;
            if (!handleProcedureWrite(sLine, sCurrentCommand, nReturnVal))
            {
                // returns false either when the loop shall return
                // or it shall continue
                if (nReturnVal)
                    return nReturnVal;
                continue;
            }

            // Handle the "to_cmd()" function
            handleToCmd(sLine, sCache, sCurrentCommand);

            if (containsCastingFunctions(sLine))
            {
                evaluateCastingFunctions(sLine);
                sCurrentCommand = findCommand(sLine).sString;
            }

            // Handle procedure calls at this location
            // Will return false, if the command line was cleared completely
            // Do nothing, if the prefixed command is a "manual" command
            if (sCurrentCommand != "help" && sCurrentCommand != "edit" && sCurrentCommand != "new" && !evaluateProcedureCalls(sLine))
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
                        if (!commandQueue.size() && !(_script.isValid() && _script.isOpen()))
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
#warning TODO (numere#3#08/27/24): This cannot be deactivated until tables are integrated differently -> NEW ISSUE
                evalRecursiveExpressions(sLine);
            }

            // Handle the flow controls like "if", "while", "for"
            nReturnVal = NUMERE_ERROR;
            if (!handleFlowControls(sLine, sCurrentCommand, nReturnVal))
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
            if (_memoryManager.containsTablesOrClusters(sLine))
            {
                sCache = getDataElements(sLine, _parser, _memoryManager);

                if (sCache.length())
                    bWriteToCache = true;
            }
            else if (isClusterCandidate(sLine, sCache))
                bWriteToCache = true;

            // Remove the definition operator
            while (sLine.find(":=") != std::string::npos)
            {
                sLine.erase(sLine.find(":="), 1);
            }

            // --> Wenn die Ergebnisse in den Cache geschrieben werden sollen, bestimme hier die entsprechenden Koordinaten <--
            Indices _idx;

            if (bWriteToCache)
            {
                // Get the indices from the corresponding function
                getIndices(sCache, _idx, _parser, _memoryManager, true);

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

            int nNum;
            const mu::StackItem* v = _parser.Eval(nNum);
            _assertionHandler.checkAssertion(v, nNum);

            // Create the answer of the calculation and print it
            // to the command line, if not suppressed
            createCalculationAnswer(nNum, v);

            if (bWriteToCache)
            {
                // Is it a cluster?
                if (bWriteToCluster)
                {
                    NumeRe::Cluster& cluster = _memoryManager.getCluster(sCache);
                    cluster.assignResults(_idx, v[0].get());
                }
                else
                    _memoryManager.writeToTable(_idx, sCache, v[0].get());
            }
        }
        // This section starts the error handling
        catch (mu::Parser::exception_type& e)
        {
            _option.enableSystemPrints(true);
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_MUP_HEAD")));
            make_hline();
            std::string sMsg = e.GetMsg();
            printErrorMessage(sMsg.substr(0, sMsg.find('$')),
                              sMsg.find('$') != std::string::npos ? sMsg.substr(sMsg.find('$')+1) : "",
                              e.GetExpr(),
                              e.GetPos());
            make_hline();
            sendErrorNotification();

            g_logger.error(toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.GetMsg());
            resetAfterError();
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
            print(LineBreak(_lang.get("ERR_STD_BADALLOC", getVersion()), _option, false, 4));
            make_hline();
            sendErrorNotification();

            g_logger.error("ERROR: BAD ALLOC");
            resetAfterError();
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
            print(std::string(e.what()));
            print(_lang.get("ERR_STD_INTERNAL"));
            make_hline();
            sendErrorNotification();

            g_logger.error(toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.what());
            resetAfterError();
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
                print(LineBreak(_lang.get("ERR_NR_3200_0_PROCESS_ABORTED_BY_USER"), _option, false, 4));
                g_logger.warning("Process was cancelled by user");
            }
            else
            {
                print(toUpperCase(_lang.get("ERR_NR_HEAD")));
                make_hline();

                if (e.getToken().length() && (e.errorcode == SyntaxError::PROCEDURE_THROW || e.errorcode == SyntaxError::LOOP_THROW))
                {
                    print(e.getToken());

                    auto errorLoc = getErrorLocation();

                    if (errorLoc.second != SyntaxError::invalid_position)
                    {
                        print(_lang.get("DBG_FILE") + ": " + errorLoc.first + " @ " + toString(errorLoc.second+1));

                        if ((!_debugger.isActive() || _script.isOpen()) && _option.getSetting(SETTING_B_POINTTOERROR).active())
                            gotoLine(errorLoc.first, errorLoc.second);
                    }

                    g_logger.error(toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.getToken());
                }
                else
                {
                    std::string sErrLine_0 = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(),
                                                       toString(e.getIndices()[0]), toString(e.getIndices()[1]),
                                                       toString(e.getIndices()[2]), toString(e.getIndices()[3]));
                    std::string sErrLine_1 = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_1_*", e.getToken(),
                                                       toString(e.getIndices()[0]), toString(e.getIndices()[1]),
                                                       toString(e.getIndices()[2]), toString(e.getIndices()[3]));
                    std::string sErrIDString = _lang.getKey("ERR_NR_" + toString((int)e.errorcode) + "_0_*");

                    if (sErrLine_0.starts_with("ERR_NR_"))
                    {
                        sErrLine_0 = _lang.get("ERR_GENERIC_0", toString((int)e.errorcode));
                        sErrLine_1 = _lang.get("ERR_GENERIC_1");
                        sErrIDString = "ERR_GENERIC";
                    }

                    printErrorMessage(sErrLine_0, sErrLine_1, e.getExpr(), e.getPosition());
                    g_logger.error(toUpperCase(_lang.get("ERR_ERROR")) + ": " + sErrIDString);
                }
            }


            make_hline();
            sendErrorNotification();

            resetAfterError();
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
            print(_lang.get("ERR_CATCHALL"));
            make_hline();
            sendErrorNotification();

            g_logger.error("ERROR: UNKNOWN EXCEPTION");
            resetAfterError();
            return NUMERE_ERROR;
        }

        if (_script.wasLastCommand())
        {
            print(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()));
            _memoryManager.setPluginCommands(_procedure.getPluginNames());

            checkInternalStates();

            if (!commandQueue.size())
            {
                bCancelSignal = false;
                return NUMERE_DONE_KEYWORD;
            }
        }
    }
    while ((_script.isValid() && _script.isOpen()) || commandQueue.size());

    bCancelSignal = false;
    nDebuggerCode = 0;

    if (_script.wasLastCommand())
    {
        print(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()));
        _memoryManager.setPluginCommands(_procedure.getPluginNames());

        checkInternalStates();

        return NUMERE_DONE_KEYWORD;
    }

    if (bSupressAnswer || !sLine.length())
        return NUMERE_DONE_KEYWORD;

    return NUMERE_DONE;
}


/////////////////////////////////////////////////
/// \brief This function points to the error
/// indicated by nPos. It draws three circumflexes
/// below the error location.
///
/// \param nPos size_t
/// \return std::string
///
/////////////////////////////////////////////////
static std::string pointToError(size_t nPos)
{
    std::string sErrorPointer = "|       ";
    sErrorPointer += strfill("^^^", nPos+14) + "\n";
    return sErrorPointer;
}


/////////////////////////////////////////////////
/// \brief Formats and prints the error message
/// for parser errors and syntax errors and
/// points to the error location. Jumps also to
/// the file and line, if activated.
///
/// \param errMsg const std::string&
/// \param errDesc const std::string&
/// \param expr std::string
/// \param pos size_t
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::printErrorMessage(const std::string& errMsg, const std::string& errDesc, std::string expr, size_t pos)
{
    // Get the file position if available
    auto errorLoc = getErrorLocation();

    // Print the message first
    print(LineBreak(Mangler::demangleExpression(errMsg), _option, true, 4));

    // Apply the empty line if necessary
    if (errorLoc.second != SyntaxError::invalid_position || expr.length())
        printPreFmt("|\n");

    if (errorLoc.second != SyntaxError::invalid_position)
    {
        printPreFmt("|       " + _lang.get("ERR_MODULE", errorLoc.first + " @ Line " + toString(errorLoc.second+1)) + "\n");

        if ((!_debugger.isActive() || _script.isOpen()) && _option.getSetting(SETTING_B_POINTTOERROR).active())
            gotoLine(errorLoc.first, errorLoc.second);
    }

    if (expr.length())
    {
        if (pos != SyntaxError::invalid_position)
            expr = Mangler::demangleExpressionWithPosition(expr, pos);
        else
            expr = Mangler::demangleExpression(expr);

        printPreFmt("|       " + LineBreak(_lang.get("ERR_EXPRESSION", maskProcedureSigns(expr)),
                                           _option, true, 8, 20) + "\n");

        if (pos != SyntaxError::invalid_position)
        {
            std::string sErrorExpr = "|       ";

            if (expr.length() > 63 && pos > 31 && pos < expr.length() - 32)
            {
                sErrorExpr += _lang.get("ERR_POSITION", "..." + expr.substr(pos - 29, 57) + "...");
                pos = 32;
            }
            else if (pos < 32)
            {
                if (expr.length() > 63)
                    sErrorExpr += _lang.get("ERR_POSITION", expr.substr(0, 60) + "...");
                else
                    sErrorExpr += _lang.get("ERR_POSITION", expr);

                pos++;
            }
            else if (pos > expr.length() - 32)
            {
                if (expr.length() > 63)
                {
                    sErrorExpr += _lang.get("ERR_POSITION", "..." + expr.substr(expr.length() - 60));
                    pos = 65 - (expr.length() - pos) - 2;
                }
                else
                    sErrorExpr += _lang.get("ERR_POSITION", expr);
            }

            printPreFmt(sErrorExpr + "\n");
            printPreFmt(pointToError(pos));
        }
        else
            printPreFmt("|\n");
    }

    if (errDesc.length())
        print(LineBreak(errDesc, _option, true, 4));
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// handle the command line input source and
/// validate it, before the core will evaluate it.
///
/// \param sLine std::string&
/// \param sKeep std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleCommandLineSource(std::string& sLine, std::string& sKeep)
{
    if (!commandQueue.size())
    {
        // --> Wenn gerade ein Script aktiv ist, lese dessen naechste Zeile, sonst nehme eine Zeile von std::cin <--
        if (_script.isValid() && _script.isOpen())
        {
            sLine = _script.getNextScriptCommand();
        }

        // --> Leerzeichen und Tabulatoren entfernen <--
        StripSpaces(sLine);
        for (size_t i = 0; i < sLine.length(); i++)
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
            if (sLine.ends_with("\\\\"))
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
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, sLine.find_first_of("({[]})"));
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
/// \param sCurrentCommand const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::getLineFromCommandCache(std::string& sLine, const std::string& sCurrentCommand)
{
    // Only do something if the command cache is not empty or the current line contains a semicolon
    if ((commandQueue.size() || sLine.find(';') != std::string::npos)
        && !_procedure.is_writing() && sCurrentCommand != "procedure")
    {
        if (commandQueue.size())
        {
            // The command cache is not empty
            // Get the next task from the command cache
            sLine = commandQueue.front();

            if (sLine.back() == ';')
            {
                sLine.pop_back();
                bSupressAnswer = true;
            }

            commandQueue.pop();
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
            EndlessVector<std::string> expressions = getAllSemiColonSeparatedTokens(sLine);

            for (const auto& expr : expressions)
            {
                commandQueue.push(expr + ";");
            }

            if (sLine.back() != ';')
                commandQueue.back().pop_back();

            sLine = commandQueue.front();

            if (sLine.back() == ';')
            {
                sLine.pop_back();
                bSupressAnswer = true;
            }

            commandQueue.pop();
        }
    }

    return sLine.length() != 0;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// handle the compose block (i.e. store the
/// different commands and return them, once the
/// block is finished).
///
/// \param sLine std::string&
/// \param sCurrentCommand const std::string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleComposeBlock(std::string& sLine, const std::string& sCurrentCommand, KernelStatus& nReturnVal)
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
            if (!(_script.isValid() && _script.isOpen()) && !commandQueue.size())
            {
                if (_procedure.getCurrentBlockDepth())
                {
                    // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                    printPreFmt("|" + FlowCtrl::getBlockName(_procedure.getCurrentBlock()));
                    if (_procedure.getCurrentBlock() == FlowCtrl::FCB_IF)
                    {
                        if (_procedure.getCurrentBlockDepth() > 1)
                            printPreFmt("---");
                        else
                            printPreFmt("-");
                    }
                    else if (_procedure.getCurrentBlock() == FlowCtrl::FCB_ELSE && _procedure.getCurrentBlockDepth() > 1)
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

            if (Plot::isPlottingCommand(sCommand))
            {
                sPlotCompose += sLine + " <<COMPOSE>> ";
                // If the block wasn't started from a script, then ask the user
                // to complete it
                if (!(_script.isValid() && _script.isOpen()) && !commandQueue.size())
                {
                    if (_procedure.getCurrentBlockDepth())
                    {
                        // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                        printPreFmt("|" + FlowCtrl::getBlockName(_procedure.getCurrentBlock()));
                        if (_procedure.getCurrentBlock() == FlowCtrl::FCB_IF)
                        {
                            if (_procedure.getCurrentBlockDepth() > 1)
                                printPreFmt("---");
                            else
                                printPreFmt("-");
                        }
                        else if (_procedure.getCurrentBlock() == FlowCtrl::FCB_ELSE && _procedure.getCurrentBlockDepth() > 1)
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
/// \param sCurrentCommand const std::string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleProcedureWrite(const std::string& sLine, const std::string& sCurrentCommand, KernelStatus& nReturnVal)
{
    if (_procedure.is_writing() || sCurrentCommand == "procedure")
    {
        if (!_procedure.writeProcedure(sLine))
            print(LineBreak(_lang.get("PARSER_CANNOTCREATEPROC"), _option));

        if (!(_script.isValid() && _script.isOpen()) && !commandQueue.size())
        {
            if (_procedure.getCurrentBlockDepth())
            {
                // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                printPreFmt("|" + FlowCtrl::getBlockName(_procedure.getCurrentBlock()));
                if (_procedure.getCurrentBlock() == FlowCtrl::FCB_IF)
                {
                    if (_procedure.getCurrentBlockDepth() > 1)
                        printPreFmt("---");
                    else
                        printPreFmt("-");
                }
                else if (_procedure.getCurrentBlock() == FlowCtrl::FCB_ELSE && _procedure.getCurrentBlockDepth() > 1)
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
                replaceAll(sPlugin, ";", ",");

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
        size_t nPos = 0;

        // Find all "to_cmd()"'s
        while (sLine.find("to_cmd(", nPos) != std::string::npos)
        {
            nPos = sLine.find("to_cmd(", nPos) + 6;

            if (isInQuotes(sLine, nPos))
                continue;

            size_t nParPos = getMatchingParenthesis(StringView(sLine, nPos));

            if (nParPos == std::string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

            std::string sCmdString = sLine.substr(nPos + 1, nParPos - 1);
            StripSpaces(sCmdString);

            // Evaluate the string part
            _parser.SetExpr(sCmdString);
            sCmdString = _parser.Eval().printVals();
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
        size_t nPos = 0;
        int nProc = 0;

        // Find all procedures
        while (sLine.find('$', nPos) != std::string::npos && sLine.find('(', sLine.find('$', nPos)) != std::string::npos)
        {
            size_t nParPos = 0;
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
            nParPos += getMatchingParenthesis(StringView(sLine, nParPos));
            __sVarList = __sVarList.substr(1, getMatchingParenthesis(__sVarList) - 1);

            // Ensure that the procedure is not part of quotation marks
            if (!isInQuotes(sLine, nPos, true))
            {
                // Execute the current procedure
                Returnvalue _rTemp = _procedure.execute(__sName, __sVarList, _parser, _functions, _memoryManager, _option, _pData, _script);
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
            Returnvalue _rTemp = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _memoryManager, _option, _pData, _script);

            // Handle the return values
            if (_rTemp.sReturnedTable.length())
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

                    sLine = _parser.CreateTempVar(std::vector<std::complex<double>>({_memoryManager.getLines(sTargetTable),
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
            else if (sLine.find("<<RETURNVAL>>") != std::string::npos)
            {
                std::string sVarName = "_~PLUGIN[" + _procedure.getPluginProcName() + "~ROOT]";
                sLine.replace(sLine.find("<<RETURNVAL>>"), 13, sVarName);
#warning FIXME (numere#4#08/05/24): Figure out, how to correctly handle multiple return values (instead of using .front()) -> NEW ISSUE
                _parser.SetInternalVar(sVarName, _rTemp.valArray.front());
            }

            _option.enableSystemPrints(true);

            if (!sLine.length())
                return false;
        }
        else
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// handle used flow controls.
///
/// \param sLine std::string&
/// \param sCurrentCommand const std::string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleFlowControls(std::string& sLine, const std::string& sCurrentCommand, KernelStatus& nReturnVal)
{
    if (_procedure.getCurrentBlockDepth() || FlowCtrl::isFlowCtrlStatement(sCurrentCommand) )
    {
        if (bSupressAnswer)
            sLine += ";";
        // --> Die Zeile in den Ausdrucksspeicher schreiben, damit sie spaeter wiederholt aufgerufen werden kann <--
        _procedure.addToControlFlowBlock(sLine, (_script.isValid() && _script.isOpen()) ? _script.getCurrentLine()-1 : 0);
        /* --> So lange wir im Loop sind und nicht endfor aufgerufen wurde, braucht die Zeile nicht an den Parser
         *     weitergegeben werden. Wir ignorieren daher den Rest dieser for(;;)-Schleife <--
         */
        if (!(_script.isValid() && _script.isOpen()) && !commandQueue.size())
        {
            if (_procedure.getCurrentBlockDepth())
            {
                toggleTableStatus();
                // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                printPreFmt("|" + FlowCtrl::getBlockName(_procedure.getCurrentBlock()));

                if (_procedure.getCurrentBlock() == FlowCtrl::FCB_IF)
                {
                    if (_procedure.getCurrentBlockDepth() > 1)
                        printPreFmt("---");
                    else
                        printPreFmt("-");
                }
                else if (_procedure.getCurrentBlock() == FlowCtrl::FCB_ELSE && _procedure.getCurrentBlockDepth() > 1)
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
/// \brief This private member function will
/// create the answer line for the parser which is
/// then passed to NumeReKernel::printResult().
///
/// \param nNum int
/// \param v const mu::StackItem*
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::createCalculationAnswer(int nNum, const mu::StackItem* v)
{
    vAns.overwrite(v[0].get());
    getAns().setValueArray(v[0].get());

    if (!bSupressAnswer)
        printResult(formatResultOutput(nNum, v), _script.isValid() && _script.isOpen());
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// reset the kernel variables after an error had
/// been handled.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::resetAfterError()
{
    _pData.setFileName("");

    if (commandQueue.size())
    {
        std::queue<std::string> emptyQueue;
        commandQueue.swap(emptyQueue);
    }
    _parser.DeactivateLoopMode();
    sCommandLine.clear();
    bCancelSignal = false;

    // If script is still open, close it
    if (_script.isOpen() && _script.isValid())
        _script.close();

    // Reset the debugger, if not already done
    _debugger.finalize();
    _procedure.reset();
}


/////////////////////////////////////////////////
/// \brief This member function returns the file
/// and the line position, if any.
///
/// \return std::pair<std::string, size_t>
///
/////////////////////////////////////////////////
std::pair<std::string, size_t> NumeReKernel::getErrorLocation()
{
    // Find it in a script
    if (_script.isOpen() && _script.isValid())
        return std::make_pair(_debugger.getErrorModule().length()
                              ? _debugger.getErrorModule() : _script.getScriptFileName(),
                              _debugger.getLineNumber() != SyntaxError::invalid_position
                              ? _debugger.getLineNumber() : _script.getCurrentLine()-1);

    // Find it in a procedure
    if (_debugger.getErrorModule().length())
        return std::make_pair(_debugger.getErrorModule(), _debugger.getLineNumber());

    return std::make_pair(std::string(""), SyntaxError::invalid_position);
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
    printPreFmt(toUpperCase(_lang.get("COMMON_APPNAME")));
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
            return toString(bytes / 1024.0, 5) + " MByte";

        return toString(bytes, 5) + " KByte";
    }

    return toString(bytes, 5) + " Byte";
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
    constexpr size_t MAXSTRINGLENGTH = 512;

    mu::varmap_type varmap = _parser.GetVar();
    std::map<std::string, std::pair<size_t, size_t>> tablemap = _memoryManager.getTableMap();
    const std::map<std::string, NumeRe::Cluster>& clustermap = _memoryManager.getClusterMap();
    std::string sCurrentLine;

    // Gather all (global) numerical variables
    for (auto iter = varmap.begin(); iter != varmap.end(); ++iter)
    {
        if ((iter->first).starts_with("_~")
            || iter->first == "ans"
            || iter->second->getCommonType() == mu::TYPE_STRING
            || isDimensionVar(iter->first))
            continue;

        sCurrentLine = iter->first + "\t" + iter->second->printDims() + "\t" + iter->second->getCommonTypeAsString() + "\t"
            + iter->second->printOverview(DEFAULT_NUM_PRECISION, MAXSTRINGLENGTH) + "\t"
            + iter->first + "\t" + formatByteSize(iter->second->getBytes());

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nNumerics = vars.vVariables.size();

    // Gather all (global) string variables
    for (auto iter = varmap.begin(); iter != varmap.end(); ++iter)
    {
        if ((iter->first).starts_with("_~")
            || iter->first == "ans"
            || iter->second->getCommonType() != mu::TYPE_STRING
            || isDimensionVar(iter->first))
            continue;

        sCurrentLine = iter->first + "\t" + iter->second->printDims() + "\t" + iter->second->getCommonTypeAsString() + "\t"
            + iter->second->printOverview(DEFAULT_NUM_PRECISION, MAXSTRINGLENGTH) + "\t"
            + iter->first + "\t" + formatByteSize(iter->second->getBytes());

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nStrings = vars.vVariables.size() - vars.nNumerics;

    // Gather all (global) tables
    for (auto iter = tablemap.begin(); iter != tablemap.end(); ++iter)
    {
        if ((iter->first).starts_with("_~"))
            continue;

        sCurrentLine = iter->first + "()\t" + toString(_memoryManager.getLines(iter->first, false)) + " x "
            + toString(_memoryManager.getCols(iter->first, false));

        std::pair<double,double> stats = _memoryManager.getTable(iter->first)->minmax();

        sCurrentLine += "\ttable\t{" + toString(stats.first, DEFAULT_MINMAX_PRECISION) + ", ..., "
            + toString(stats.second, DEFAULT_MINMAX_PRECISION) + "}\t" + iter->first + "()\t"
            + formatByteSize(_memoryManager.getBytes(iter->first));

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nTables = vars.vVariables.size() - vars.nNumerics - vars.nStrings;

    // Gather all (global) clusters
    for (auto iter = clustermap.begin(); iter != clustermap.end(); ++iter)
    {
        if ((iter->first).starts_with("_~"))
            continue;

        sCurrentLine = iter->first + "{}\t" + toString(iter->second.size()) + " x 1";
        sCurrentLine += "\tcluster\t" + iter->second.printOverview(DEFAULT_NUM_PRECISION, MAXSTRINGLENGTH, 5, true)
            + "\t" + iter->first + "{}\t" + formatByteSize(iter->second.getBytes());

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nClusters = vars.vVariables.size() - vars.nNumerics - vars.nStrings - vars.nTables;

    return vars;
}


/////////////////////////////////////////////////
/// \brief This member function returns a structure
/// containing all currently declared variables,
/// which can be used for autocompletion purposes.
///
/// \return NumeReVariables
///
/////////////////////////////////////////////////
NumeReVariables NumeReKernel::getVariableListForAutocompletion()
{
    NumeReVariables vars;

    mu::varmap_type varmap = _parser.GetVar();
    std::map<std::string, std::pair<size_t, size_t>> tablemap = _memoryManager.getTableMap();
    const std::map<std::string, NumeRe::Cluster>& clustermap = _memoryManager.getClusterMap();
    std::string sCurrentLine;

    // Gather all (global) numerical variables
    for (auto iter = varmap.begin(); iter != varmap.end(); ++iter)
    {
        if ((iter->first).starts_with("_~")
            || iter->first == "ans"
            || iter->second->getCommonType() == mu::TYPE_STRING
            || isDimensionVar(iter->first))
            continue;

        sCurrentLine = iter->first + "\t" + iter->second->printDims() + "\t" + iter->second->getCommonTypeAsString();

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nNumerics = vars.vVariables.size();

    // Gather all (global) string variables
    for (auto iter = varmap.begin(); iter != varmap.end(); ++iter)
    {
        if ((iter->first).starts_with("_~")
            || iter->first == "ans"
            || iter->second->getCommonType() != mu::TYPE_STRING
            || isDimensionVar(iter->first))
            continue;

        sCurrentLine = iter->first + "\t" + iter->second->printDims() + "\t" + iter->second->getCommonTypeAsString();

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nStrings = vars.vVariables.size() - vars.nNumerics;

    // Gather all (global) tables
    for (auto iter = tablemap.begin(); iter != tablemap.end(); ++iter)
    {
        if ((iter->first).starts_with("_~"))
            continue;

        sCurrentLine = iter->first + "()\t" + toString(_memoryManager.getLines(iter->first, false)) + " x "
            + toString(_memoryManager.getCols(iter->first, false)) + "\ttable";

        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nTables = vars.vVariables.size() - vars.nNumerics - vars.nStrings;

    // Gather all (global) clusters
    for (auto iter = clustermap.begin(); iter != clustermap.end(); ++iter)
    {
        if ((iter->first).starts_with("_~"))
            continue;

        sCurrentLine = iter->first + "{}\t" + toString(iter->second.size()) + " x 1\tcluster";

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
/// \return const std::vector<Package>&
///
/////////////////////////////////////////////////
const std::vector<Package>& NumeReKernel::getInstalledPackages() const
{
    return _procedure.getPackages();
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
/// \param bScriptRunning bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::printResult(const std::string& sLine, bool bScriptRunning)
{
    if (!m_parent)
        return;

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
/// \param v const mu::StackItem*
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReKernel::formatResultOutput(int nNum, const mu::StackItem* v)
{
    size_t prec = getInstance()->getSettings().getPrecision();
    std::string sAns;

    for (int n = 0; n < nNum; n++)
    {
        if (v[n].get().size() > 1)
        {
            // More than one result
            //
            // How many fit into one line?
            size_t nLineBreak = numberOfNumbersPerLine();

            if (n)
                sAns += "\n|-> ans = {";
            else
                sAns = "ans = {";

            // compose the result
            for (size_t i = 0; i < v[n].get().size(); ++i)
            {
                sAns += strfill(v[n].get()[i].printEmbedded(prec, prec+TERMINAL_FORMAT_FIELD_LENOFFSET, true),
                                prec + TERMINAL_FORMAT_FIELD_LENOFFSET);

                if (i < v[n].get().size() - 1)
                    sAns += ", ";

                if (v[n].get().size() + 1 > nLineBreak && !((i + 1) % nLineBreak) && i < v[n].get().size() - 1)
                    sAns += "...\n|          ";
            }

            sAns += "}";
        }
        else
        {
            // Only one result
            if (n)
                sAns += "\n|-> ans = " + v[n].get().print(prec, 0);
            else
                sAns = "ans = " + v[n].get().print(prec, 0);
        }
    }

    // fallback
    return sAns;
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
    ensureMainWindowVisible();
}


/////////////////////////////////////////////////
/// \brief Ensures that the main window is
/// visible by deactivating possible hide flags.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::ensureMainWindowVisible()
{
    if (!_option.getSetting(SETTING_B_WINDOWSHOWN).active())
    {
        _option.getSetting(SETTING_B_WINDOWSHOWN).active() = true;
        modifiedSettings = true;
    }
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
        for (size_t i = 0; i < sType.length(); i++)
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
/// \param nLine size_t
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::gotoLine(const std::string& sFile, size_t nLine)
{
    if (!m_parent)
        return;
    else
    {
        NumeReKernel::getInstance()->ensureMainWindowVisible();
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
    g_logger.info("Requested table: " + sTableName);

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
    g_logger.info("Requested variable: " + sStringTableName);
    constexpr size_t MAXSTRINGLENGTH = 1024;

    if (_memoryManager.isCluster(sStringTableName))
    {
        // Create the container for the selected cluster
        NumeRe::Cluster& clust = _memoryManager.getCluster(sStringTableName.substr(0, sStringTableName.find("{}")));

        if (sStringTableName.find(".sel(") == std::string::npos)
        {
            NumeRe::Container<std::string> stringTable(clust.size(), 1);

            for (size_t i = 0; i < clust.size(); i++)
            {
                const mu::Value& val = clust.get(i);

                if (val.getType() == mu::TYPE_STRING)
                    stringTable.set(i, 0, val.print(0, MAXSTRINGLENGTH, true));
                else
                    stringTable.set(i, 0, !val.isValid() ? "---" : val.print(5));
            }

            return stringTable;
        }

        size_t p = 0;
        mu::Array selected = clust;

        while ((p = sStringTableName.find(".sel(", p)) != std::string::npos)
        {
            std::string element = sStringTableName.substr(p+5, getMatchingParenthesis(StringView(sStringTableName, p+4))-1);
            size_t el = StrToInt(element)-1;

            if (el >= selected.size())
                return NumeRe::Container<std::string>();

            selected = selected.get(el);
            p++;
        }

        NumeRe::Container<std::string> stringTable(selected.size(), 1);

        for (size_t i = 0; i < selected.size(); i++)
        {
            const mu::Value& val = selected.get(i);

            if (val.getType() == mu::TYPE_STRING)
                stringTable.set(i, 0, val.print(0, MAXSTRINGLENGTH, true));
            else
                stringTable.set(i, 0, !val.isValid() ? "---" : val.print(5));
        }

        return stringTable;
    }
    else if (_parser.GetVar().find(sStringTableName) != _parser.GetVar().end())
    {
        auto iter = _parser.GetVar().find(sStringTableName);
        const mu::Array& arr = *(iter->second);

        NumeRe::Container<std::string> stringTable(arr.size(), 1);

        for (size_t i = 0; i < arr.size(); i++)
        {
            const mu::Value& val = arr.get(i);

            if (val.getType() == mu::TYPE_STRING)
                stringTable.set(i, 0, val.print(0, MAXSTRINGLENGTH, true));
            else
                stringTable.set(i, 0, !val.isValid() ? "---" : val.print(5));
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
        NumeReKernel::getInstance()->ensureMainWindowVisible();
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

    std::map<std::string, std::pair<std::string, mu::Variable*>> mLocalVars;
    std::map<std::string, std::string> mLocalTables;
    std::map<std::string, std::string> mLocalClusters;
    std::map<std::string, std::string> mArguments;

    // Obtain references to the debugger and the parser
    NumeReDebugger& _debugger = getInstance()->getDebugger();
    const Parser& _parser = getInstance()->getParser();

    // Get the numerical variable map
    const mu::varmap_type& varmap = _parser.GetVar();

    for (auto iter : varmap)
    {
        if (!iter.first.starts_with("_~")
            && iter.first != "ans"
            && !isDimensionVar(iter.first))
            mLocalVars[iter.first] = std::make_pair(iter.first, iter.second);
    }

    // Get the table variable map
    std::map<std::string, std::pair<size_t,size_t>> tableMap = getInstance()->getMemoryManager().getTableMap();

    for (const auto& iter : tableMap)
        mLocalTables[iter.first] = iter.first;

    // Get the cluster mao
    const std::map<std::string, NumeRe::Cluster>& clusterMap = getInstance()->getMemoryManager().getClusterMap();

    for (const auto& iter : clusterMap)
    {
        if (!iter.first.starts_with("_~"))
            mLocalClusters[iter.first] = iter.first;
    }


    // Pass the created information to the debugger
    _debugger.gatherInformations(mLocalVars, mLocalTables, mLocalClusters, mArguments,
                                 sCurrentCommand, getInstance()->getScript().getScriptFileName(),
                                 getInstance()->getScript().getCurrentLine()-1);

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
    bool b = bCancelSignal;
    bCancelSignal = false;
    return b;
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


