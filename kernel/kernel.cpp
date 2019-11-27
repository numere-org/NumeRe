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
#include "../gui/terminal/wxterm.h"
#include "core/datamanagement/dataops.hpp" // for make_stringmatrix()
#define KERNEL_PRINT_SLEEP 2

extern const string sVersion;
/* --> STATUS: Versionsname des Programms; Aktuell "Ampere", danach "Angstroem". Ab 1.0 Namen mit "B",
 *     z.B.: Biot(1774), Boltzmann(1844), Becquerel(1852), Bragg(1862), Bohr(1885), Brillouin(1889),
 *     de Broglie(1892, Bose(1894), Bloch(1905), Bethe(1906)) <--
 * --> de Coulomb(1736), Carnot(1796), P.Curie(1859), M.Curie(1867), A.Compton(1892), Cherenkov(1904),
 *     Casimir(1909), Chandrasekhar(1910), Chamberlain(1920), Cabibbo(1935) <--
 */

Language _lang;
mglGraph _fontData;
extern Plugin _plugin;
extern value_type vAns;
extern Integration_Vars parser_iVars;
time_t tTimeZero = time(0);

// Initialization of the static member variables
NumeReKernel* NumeReKernel::kernelInstance = nullptr;
int* NumeReKernel::baseStackPosition = nullptr;
wxTerm* NumeReKernel::m_parent = nullptr;
queue<NumeReTask> NumeReKernel::taskQueue;
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
ofstream NumeReKernel::oLogFile;
ProcedureLibrary NumeReKernel::ProcLibrary;

typedef BOOL (WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);


/////////////////////////////////////////////////
/// \brief This function returns true, if we're
/// currently running on Win x64.
///
/// \return bool
///
/////////////////////////////////////////////////
bool IsWow64()
{
	BOOL bIsWow64 = false;

	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.

	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
			GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			return false;
		}
	}
	return (bool)bIsWow64;
}


/////////////////////////////////////////////////
/// \brief Constructor of the kernel.
/////////////////////////////////////////////////
NumeReKernel::NumeReKernel() : _option(), _data(), _parser(), _stringParser(_parser, _data, _option)
{
	sCommandLine.clear();
	sAnswer.clear();
	sPlotCompose.clear();
	kernelInstance = this;
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
	return _option.sendSettings();
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
	_debugger.setActive(_settings.getUseDebugger());
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
	if (!_data.getSaveStatus())
		_data.saveToCacheFile();
	return;
}


/////////////////////////////////////////////////
/// \brief This is the kernel "booting" function.
///
/// \param _parent wxTerm*
/// \param __sPath const string&
/// \param sPredefinedFunctions const string&
/// \return void
///
/// This function sets all parameters, functions
/// and constants used for the numerical parser.
/// loads possible available autosaves and
/// definition files for functions and plugins.
/////////////////////////////////////////////////
void NumeReKernel::StartUp(wxTerm* _parent, const string& __sPath, const string& sPredefinedFunctions)
{
	if (_parent && m_parent == nullptr)
		m_parent = _parent;
	//Do some start-up stuff here

	string sFile = ""; 			// String fuer den Dateinamen.
	string sTime = getTimeStamp(false);
	string sLogFile = "numere.log";
	string sPath = __sPath;

	// Set the functions provided by the syntax object in the parent class
	_functions.setPredefinedFuncs(sPredefinedFunctions);
	_data.setPredefinedFuncs(_functions.getPredefinedFuncs());
	_script.setPredefinedFuncs(sPredefinedFunctions);
	_procedure.setPredefinedFuncs(sPredefinedFunctions);

	// Get the version of the operating system
	OSVERSIONINFOA _osversioninfo;
	_osversioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&_osversioninfo);

	// Make the path UNIX style
	while (sPath.find('\\') != string::npos)
		sPath[sPath.find('\\')] = '/';

    // Set the path in the settings object and load the settings
    // from the config file
	_option.setExePath(sPath);
	_option.load(sPath);				// Lade Informationen aus einem ini-File

	// Initialize the log file
	if (_option.getbUseLogFile())
	{
		reduceLogFilesize((sPath + "/" + sLogFile).c_str());
		oLogFile.open((sPath + "/" + sLogFile).c_str(), ios_base::out | ios_base::app | ios_base::ate);
		if (oLogFile.fail())
			oLogFile.close();
	}

	// Add headline information to the log file
	if (oLogFile.is_open())
	{
		oLogFile << "--- NUMERE-SESSION-PROTOCOL: " << sTime << " ---" << endl;
		oLogFile << "--- NumeRe v " << sVersion
				 << " | Build " << AutoVersion::YEAR << "-" << AutoVersion::MONTH << "-" << AutoVersion::DATE
				 << " | OS: Windows v " << _osversioninfo.dwMajorVersion << "." << _osversioninfo.dwMinorVersion << "." << _osversioninfo.dwBuildNumber << " " << _osversioninfo.szCSDVersion << (IsWow64() ? " (64 Bit) ---" : " ---") << endl;
	}

	// Set the path tokens for all relevant objects
	_data.setTokens(_option.getTokenPaths());
	_out.setTokens(_option.getTokenPaths());
	_pData.setTokens(_option.getTokenPaths());
	_script.setTokens(_option.getTokenPaths());
	_functions.setTokens(_option.getTokenPaths());
	_procedure.setTokens(_option.getTokenPaths());
	_option.setTokens(_option.getTokenPaths());
	_lang.setTokens(_option.getTokenPaths());

	// Set the current line length
	nLINE_LENGTH = _option.getWindow();

	// Set the default paths for all objects
	_out.setPath(_option.getSavePath(), true, sPath);
	_out.createRevisionsFolder();

	_data.setPath(_option.getLoadPath(), true, sPath);
	_data.createRevisionsFolder();
	_data.newCluster("ans").setDouble(0, NAN);

	_data.setSavePath(_option.getSavePath());
	_data.setbLoadEmptyCols(_option.getbLoadEmptyCols());

	_pData.setPath(_option.getPlotOutputPath(), true, sPath);
	_pData.createRevisionsFolder();

	_script.setPath(_option.getScriptPath(), true, sPath);
	_script.createRevisionsFolder();

	_procedure.setPath(_option.getProcsPath(), true, sPath);
	_procedure.createRevisionsFolder();

	// Create the default paths, if they are not present
	_option.setPath(_option.getExePath() + "/docs/plugins", true, sPath);
	_option.setPath(_option.getExePath() + "/docs", true, sPath);
	_option.setPath(_option.getExePath() + "/user/lang", true, sPath);
	_option.setPath(_option.getExePath() + "/user/docs", true, sPath);
	_option.setPath(_option.getSavePath() + "/docs", true, sPath);
	_functions.setPath(_option.getExePath(), false, sPath);
	addToLog("> SYSTEM: File system was verified.");

	// Load the documentation index file
	_option.loadDocIndex(false);
	addToLog("> SYSTEM: Documentation index was loaded.");

	// Update the documentation index file
	if (fileExists(_option.getExePath() + "/update.hlpidx"))
	{
		_option.updateDocIndex();
		addToLog("> SYSTEM: Documentation index was updated.");
	}

	// Load custom language files
	if (_option.getUseCustomLanguageFiles())
	{
	    // Load the custom documentation index
		_option.loadDocIndex(_option.getUseCustomLanguageFiles());
		addToLog("> SYSTEM: User Documentation index was loaded.");
	}

	// Load the language strings
	_lang.loadStrings(_option.getUseCustomLanguageFiles());
	addToLog("> SYSTEM: Language files were loaded.");

	string sAutosave = _option.getSavePath() + "/cache.tmp";
	string sCacheFile = _option.getExePath() + "/numere.cache";

	// Load the plugin informations
	if (fileExists(_procedure.getPluginInfoPath()))
	{
		_procedure.loadPlugins();
		_plugin = _procedure;
		_data.setPluginCommands(_procedure.getPluginNames());
		addToLog("> SYSTEM: Plugin information was loaded.");
	}

	// Load the function definitions
	if (_option.getbDefineAutoLoad() && fileExists(_option.getExePath() + "\\functions.def"))
	{
		_functions.load(_option, true);
		addToLog("> SYSTEM: Function definitions were loaded.");
	}

	// Load the binary plot font
	_fontData.LoadFont(_option.getDefaultPlotFont().c_str(), (_option.getExePath() + "\\fonts").c_str());

	// Load the autosave file
	if (fileExists(sAutosave) || fileExists(sCacheFile))
	{
		if (fileExists(sAutosave))
		{
			_data.openAutosave(sAutosave, _option);
			_data.setSaveStatus(true);
			remove(sAutosave.c_str());
			_data.saveToCacheFile();
		}
		else
		{
			_data.loadFromCacheFile();
		}

		addToLog("> SYSTEM: Automatic backup was loaded.");
	}

    // Declare the default variables
	_parser.DefineVar("ans", &vAns);        // Deklariere die spezielle Variable "ans", die stets, das letzte Ergebnis speichert und die vier Standardvariablen
	_parser.DefineVar(parser_iVars.sName[0], &parser_iVars.vValue[0][0]);
	_parser.DefineVar(parser_iVars.sName[1], &parser_iVars.vValue[1][0]);
	_parser.DefineVar(parser_iVars.sName[2], &parser_iVars.vValue[2][0]);
	_parser.DefineVar(parser_iVars.sName[3], &parser_iVars.vValue[3][0]);

	// Declare the table dimension variables
	_parser.DefineVar("nlines", &_data.tableLinesCount);
	_parser.DefineVar("ncols", &_data.tableColumnsCount);
	_parser.DefineVar("nlen", &_data.dClusterElementsCount);

	// --> VAR-FACTORY Deklarieren (Irgendwo muessen die ganzen Variablen-Werte ja auch gespeichert werden) <--
	_parser.SetVarFactory(parser_AddVariable, &_parser);

	// Define the operators
	defineOperators();

	// Define the constants
	defineConst();

	// Define the functions
	defineFunctions();
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
	_parser.DefinePostfixOprt(_nrT("'G"), parser_Giga);
	_parser.DefinePostfixOprt(_nrT("'M"), parser_Mega);
	_parser.DefinePostfixOprt(_nrT("'k"), parser_Kilo);
	_parser.DefinePostfixOprt(_nrT("'m"), parser_Milli);
	_parser.DefinePostfixOprt(_nrT("'mu"), parser_Micro);
	//_parser.DefinePostfixOprt(_nrT("µ"), parser_Micro);
	_parser.DefinePostfixOprt(_nrT("'n"), parser_Nano);

	// --> Einheitenumrechnungen: Werden aufgerufen durch WERT'EINHEIT <--
	_parser.DefinePostfixOprt(_nrT("'eV"), parser_ElectronVolt);
	_parser.DefinePostfixOprt(_nrT("'fm"), parser_Fermi);
	_parser.DefinePostfixOprt(_nrT("'A"), parser_Angstroem);
	_parser.DefinePostfixOprt(_nrT("'b"), parser_Barn);
	_parser.DefinePostfixOprt(_nrT("'Torr"), parser_Torr);
	_parser.DefinePostfixOprt(_nrT("'AU"), parser_AstroUnit);
	_parser.DefinePostfixOprt(_nrT("'ly"), parser_Lightyear);
	_parser.DefinePostfixOprt(_nrT("'pc"), parser_Parsec);
	_parser.DefinePostfixOprt(_nrT("'mile"), parser_Mile);
	_parser.DefinePostfixOprt(_nrT("'yd"), parser_Yard);
	_parser.DefinePostfixOprt(_nrT("'ft"), parser_Foot);
	_parser.DefinePostfixOprt(_nrT("'in"), parser_Inch);
	_parser.DefinePostfixOprt(_nrT("'cal"), parser_Calorie);
	_parser.DefinePostfixOprt(_nrT("'psi"), parser_PSI);
	_parser.DefinePostfixOprt(_nrT("'kn"), parser_Knoten);
	_parser.DefinePostfixOprt(_nrT("'l"), parser_liter);
	_parser.DefinePostfixOprt(_nrT("'kmh"), parser_kmh);
	_parser.DefinePostfixOprt(_nrT("'mph"), parser_mph);
	_parser.DefinePostfixOprt(_nrT("'TC"), parser_Celsius);
	_parser.DefinePostfixOprt(_nrT("'TF"), parser_Fahrenheit);
	_parser.DefinePostfixOprt(_nrT("'Ci"), parser_Curie);
	_parser.DefinePostfixOprt(_nrT("'Gs"), parser_Gauss);
	_parser.DefinePostfixOprt(_nrT("'Ps"), parser_Poise);
	_parser.DefinePostfixOprt(_nrT("'mol"), parser_mol);
	_parser.DefinePostfixOprt("!", parser_Faculty);
	_parser.DefinePostfixOprt("!!", parser_doubleFaculty);

	// --> Logisches NICHT <--
	_parser.DefineInfixOprt(_nrT("!"), parser_Not);
	_parser.DefineInfixOprt(_nrT("+"), parser_Ignore);

	// --> Operatoren <--
	_parser.DefineOprt(_nrT("%"), parser_Mod, prMUL_DIV, oaLEFT, true);
	_parser.DefineOprt(_nrT("|||"), parser_XOR, prLOGIC, oaLEFT, true);
	_parser.DefineOprt(_nrT("|"), parser_BinOR, prLOGIC, oaLEFT, true);
	_parser.DefineOprt(_nrT("&"), parser_BinAND, prLOGIC, oaLEFT, true);
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
	_parser.DefineConst(_nrT("_g"), 9.80665);
	_parser.DefineConst(_nrT("_c"), 299792458);
	_parser.DefineConst(_nrT("_elek_feldkonst"), 8.854187817e-12);
	_parser.DefineConst(_nrT("_n_avogadro"), 6.02214129e23);
	_parser.DefineConst(_nrT("_k_boltz"), 1.3806488e-23);
	_parser.DefineConst(_nrT("_elem_ladung"), 1.602176565e-19);
	_parser.DefineConst(_nrT("_h"), 6.62606957e-34);
	_parser.DefineConst(_nrT("_hbar"), 1.054571726e-34);
	_parser.DefineConst(_nrT("_m_elektron"), 9.10938291e-31);
	_parser.DefineConst(_nrT("_m_proton"), 1.672621777e-27);
	_parser.DefineConst(_nrT("_m_neutron"), 1.674927351e-27);
	_parser.DefineConst(_nrT("_m_muon"), 1.883531475e-28);
	_parser.DefineConst(_nrT("_m_tau"), 3.16747e-27);
	_parser.DefineConst(_nrT("_magn_feldkonst"), 1.25663706144e-6);
	_parser.DefineConst(_nrT("_m_erde"), 5.9726e24);
	_parser.DefineConst(_nrT("_m_sonne"), 1.9885e30);
	_parser.DefineConst(_nrT("_r_erde"), 6.378137e6);
	_parser.DefineConst(_nrT("_r_sonne"), 6.9551e8);
	_parser.DefineConst(_nrT("true"), 1);
	_parser.DefineConst(_nrT("_theta_weinberg"), 0.49097621387892);
	_parser.DefineConst(_nrT("false"), 0);
	_parser.DefineConst(_nrT("_2pi"), 6.283185307179586476925286766559);
	_parser.DefineConst(_nrT("_R"), 8.3144622);
	_parser.DefineConst(_nrT("_alpha_fs"), 7.2973525698E-3);
	_parser.DefineConst(_nrT("_mu_bohr"), 9.27400968E-24);
	_parser.DefineConst(_nrT("_mu_kern"), 5.05078353E-27);
	_parser.DefineConst(_nrT("_mu_e"), -9.284764620e-24);
	_parser.DefineConst(_nrT("_mu_n"), -9.662365e-27);
	_parser.DefineConst(_nrT("_mu_p"), 1.4106067873e8);
	_parser.DefineConst(_nrT("_m_amu"), 1.660538921E-27);
	_parser.DefineConst(_nrT("_r_bohr"), 5.2917721092E-11);
	_parser.DefineConst(_nrT("_G"), 6.67384E-11);
	_parser.DefineConst(_nrT("_coul_norm"), 8987551787.99791145324707);
	_parser.DefineConst(_nrT("_stefan_boltzmann"), 5.670367e-8);
	_parser.DefineConst(_nrT("_wien"), 2.8977729e-3);
	_parser.DefineConst(_nrT("_rydberg"), 1.0973731568508e7);
	_parser.DefineConst(_nrT("_hartree"), 4.35974465e-18);
	_parser.DefineConst(_nrT("_lande_e"), -2.00231930436182);
	_parser.DefineConst(_nrT("_gamma_e"), 1.760859644e11);
	_parser.DefineConst(_nrT("_gamma_n"), 1.83247172e8);
	_parser.DefineConst(_nrT("_gamma_p"), 2.6752219e8);
	_parser.DefineConst(_nrT("_feigenbaum_delta"), 4.66920160910299067185);
	_parser.DefineConst(_nrT("_feigenbaum_alpha"), 2.50290787509589282228);
	_parser.DefineConst(_nrT("nan"), NAN);
	_parser.DefineConst(_nrT("inf"), INFINITY);
	_parser.DefineConst(_nrT("void"), NAN);
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
	// --> mathemat. Funktion deklarieren <--
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
	_parser.DefineFun("polynomial", parser_polynomial, true);                    // polynomial(x,a0,a1,a2,a3,...)
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
    // measure the current stack position
    int stackMeasureVar;
    baseStackPosition = &stackMeasureVar;

	bWritingTable = true;
	make_hline(80);
	printPreFmt("| ");
	displaySplash();
	printPreFmt("                                  |\n");
	printPreFmt("| Version: " + sVersion + strfill("Build: ", 79 - 22 - sVersion.length()) + AutoVersion::YEAR + "-" + AutoVersion::MONTH + "-" + AutoVersion::DATE + " |\n");
	printPreFmt("| Copyright (c) 2013-" + string(AutoVersion::YEAR) + toSystemCodePage(", Erik A. Hänel et al.") + strfill(toSystemCodePage(_lang.get("MAIN_ABOUT_NBR")), 79 - 48) + " |\n");
	make_hline(80);

	printPreFmt("|\n");

	if (_option.getbGreeting() && fileExists(_option.getExePath() + "\\numere.ini"))
		printPreFmt(toSystemCodePage(getGreeting()) + "|\n");

	print(LineBreak(_lang.get("PARSER_INTRO"), _option));;
	printPreFmt("|\n|<- ");
	flush();
	bWritingTable = false;

}


/////////////////////////////////////////////////
/// \brief This is the main loop for the core of
/// NumeRe.
///
/// \param sCommand const string&
/// \return NumeReKernel::KernelStatus
///
/// This function is called by the terminal to
/// process the desired operations using the core
/// of this application. It features an inner
/// loop, which will process a whole script or a
/// semicolon-separated list of commands
/// automatically.
/////////////////////////////////////////////////
NumeReKernel::KernelStatus NumeReKernel::MainLoop(const string& sCommand)
{
	if (!m_parent)
		return NUMERE_ERROR;

	// indices as target for cache writing actions
	Indices _idx;

	bool bWriteToCache = false; // TRUE, wenn das/die errechneten Ergebnisse in den Cache geschrieben werden sollen
	bool bWriteToCluster = false;

	string sLine_Temp = "";     // Temporaerer String fuer die Eingabe
	string sCache = "";         // Zwischenspeicher fuer die Cache-Koordinaten
	string sKeep = "";          // Zwei '\' am Ende einer Zeile ermoeglichen es, dass die Eingabe auf mehrere Zeilen verteilt wird.
	string sCmdCache = "";      // Die vorherige Zeile wird hierin zwischengespeichert
	string sLine = "";          // The actual line
	string sCurrentCommand = "";// The current command
	value_type* v = 0;          // Ergebnisarray
	int& nDebuggerCode = _procedure.getDebuggerCode();
	int nNum = 0;               // Zahl der Ergebnisse in value_type* v
	nLastStatusVal = -1;
	nLastLineLength = 0;

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
	bSupressAnswer = false;

	// set the procedure main path to the desired one. --> check, whether this is necessary here
	if (_procedure.getPath() != _option.getProcsPath())
	{
		_procedure.setPath(_option.getProcsPath(), true, _procedure.getProgramPath());
		_option.setProcPath(_procedure.getPath());
	}

	// Evaluate the passed commands or the contents of the script
	// This is the actual loop. It will evaluate at least once.
	do
	{
		bSupressAnswer = false;
		bWriteToCache = false;
		bWriteToCluster = false;
		sCache = "";

		// Reset the parser variable map pointer
		if (_parser.mVarMapPntr)
			_parser.mVarMapPntr = 0;

        // Try-catch block to handle all the internal exceptions
		try
		{
		    // Handle command line sources and validate the input
			if (!handleCommandLineSource(sLine, sCmdCache, sKeep))
                continue;

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
					&& !_procedure.getLoop())
			{
			    if (sLine.substr(0, 2) == "|>")
                    sLine.erase(0, 2);

				if (_option.getUseDebugger() && nDebuggerCode != DEBUGGER_LEAVE)
                {
					nDebuggerCode = evalDebuggerBreakPoint(sLine);
                }
			}

			// Log the current line
			addToLog(sLine);

			// Handle, whether the pressed the ESC key
			if (GetAsyncCancelState() && _script.isValid() && _script.isOpen())
			{
				if (_option.getbUseESCinScripts())
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
			if (_script.isValid() && _script.isOpen() && _script.installProcedures() && _script.getInstallInfoString().length())
			{
				if (findParameter(_script.getInstallInfoString(), "type", '='))
				{
					if (getArgAtPos(_script.getInstallInfoString(), findParameter(_script.getInstallInfoString(), "type", '=')).find("TYPE_PLUGIN") != string::npos)
					{
						_procedure.declareNewPlugin(_script.getInstallInfoString());
						_plugin = _procedure;
					}
				}
			}

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
			if (!_procedure.getLoop() && sLine.find("??") != string::npos && sCurrentCommand != "help")
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
			if (!_procedure.getLoop()
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
								_data.setPluginCommands(_procedure.getPluginNames());
							}
							sCommandLine.clear();
							bCancelSignal = false;
							return NUMERE_DONE_KEYWORD;
						}
						else
							continue;
					case NUMERE_QUIT:
						// --> Sind ungesicherte Daten im Cache? Dann moechte der Nutzer diese vielleicht speichern <--
						if (!_data.getSaveStatus()) // MAIN_UNSAVED_CACHE
						{
							string c = "";
							print(LineBreak(_lang.get("MAIN_UNSAVED_CACHE"), _option));
							printPreFmt("|\n|<- ");
							NumeReKernel::getline(c);
							if (c == _lang.YES())
							{
								_data.saveToCacheFile(); // MAIN_CACHE_SAVED
								print(LineBreak(_lang.get("MAIN_CACHE_SAVED"), _option));
								Sleep(500);
							}
							else
							{
								_data.clearCache();
							}
						}
						return NUMERE_QUIT;  // Keyword "quit"
						//case  2: return 1;  // Keyword "mode"
				}
			}

			// Evaluate function calls (only outside the flow control blocks)
			if (!_procedure.getLoop() && sCurrentCommand != "for" && sCurrentCommand != "if" && sCurrentCommand != "while" && sCurrentCommand != "switch")
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
			if (!_procedure.getLoop())
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
			if (sLine.find("??") != string::npos)
				sLine = promptForUserInput(sLine);

			// Get data elements for the current command line or determine,
			// if the target value of the current command line is a candidate
			// for a cluster
			if (!_stringParser.isStringExpression(sLine) && (sLine.find("data(") != string::npos || _data.containsTablesOrClusters(sLine)))
			{
				sCache = getDataElements(sLine, _parser, _data, _option);

				if (sCache.length() && sCache.find('#') == string::npos)
					bWriteToCache = true;
			}
			else if (isClusterCandidate(sLine, sCache))
                bWriteToCache = true;

			// Remove the definition operator
			while (sLine.find(":=") != string::npos)
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
			if (bWriteToCache)
			{
				// Get the indices from the corresponding function
				_idx = getIndices(sCache, _parser, _data, _option);

                if (sCache[sCache.find_first_of("({")] == '{')
                {
                    bWriteToCluster = true;
                }

				if (!isValidIndexSet(_idx))
					throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "");

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

			// Create the answer of the calculation and print it
			// to the command line, if not suppressed
			createCalculationAnswer(nNum, v, sCmdCache);

			if (bWriteToCache)
            {
                // Is it a cluster?
                if (bWriteToCluster)
                {
                    NumeRe::Cluster& cluster = _data.getCluster(sCache);
                    cluster.assignResults(_idx, nNum, v);
                }
                else
                    _data.writeToTable(_idx, sCache, v, nNum);
            }
		}
		// This section starts the error handling
		catch (mu::Parser::exception_type& e)
		{
			_option.setSystemPrintStatus(true);
			// --> Vernuenftig formatierte Fehlermeldungen <--
			unsigned int nErrorPos = (int)e.GetPos();
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
			if (e.GetExpr().length() > 63 && nErrorPos > 31 && nErrorPos < e.GetExpr().length() - 32)
			{
				printPreFmt("|   Position:  \"..." + e.GetExpr().substr(nErrorPos - 29, 57) + "...\"\n");
				printPreFmt(pointToError(32));
			}
			else if (nErrorPos < 32)
			{
				string sErrorExpr = "|   Position:  \"";
				if (e.GetExpr().length() > 63)
					sErrorExpr += e.GetExpr().substr(0, 60) + "...\"";
				else
					sErrorExpr += e.GetExpr() + "\"";
				printPreFmt(sErrorExpr + "\n");
				printPreFmt(pointToError(nErrorPos + 1));
			}
			else if (nErrorPos > e.GetExpr().length() - 32)
			{
				string sErrorExpr = "|   Position:  \"";
				if (e.GetExpr().length() > 63)
				{
					printPreFmt(sErrorExpr + "..." + e.GetExpr().substr(e.GetExpr().length() - 60) + "\"\n");
					printPreFmt(pointToError(65 - (e.GetExpr().length() - nErrorPos) - 2));
				}
				else
				{
					printPreFmt(sErrorExpr + e.GetExpr() + "\"\n");
					printPreFmt(pointToError(nErrorPos));
				}
			}

            resetAfterError(sCmdCache);

			make_hline();
			addToLog("> " + toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.GetMsg());


			sendErrorNotification();
			return NUMERE_ERROR;
		}
		catch (const std::bad_alloc& e)
		{
			_option.setSystemPrintStatus(true);
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
			addToLog("> ERROR: CRITICAL ACCESS VIOLATION");
			sendErrorNotification();

			return NUMERE_ERROR;
		}
		catch (const std::exception& e)
		{
			_option.setSystemPrintStatus(true);
			// --> Alle anderen Standard-Exceptions <--
			sendErrorNotification();
			make_hline();
			print(toUpperCase(_lang.get("ERR_STD_INTERNAL_HEAD")));
			make_hline();
			print(LineBreak(string(e.what()), _option));
			print(LineBreak(_lang.get("ERR_STD_INTERNAL"), _option));

            resetAfterError(sCmdCache);

			addToLog("> " + toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.what());
			make_hline();
			sendErrorNotification();

			return NUMERE_ERROR;
		}
		catch (SyntaxError& e)
		{
			_option.setSystemPrintStatus(true);
			sendErrorNotification();
			make_hline();
			if (e.errorcode == SyntaxError::PROCESS_ABORTED_BY_USER)
			{
				print(toUpperCase(_lang.get("ERR_PROCESS_CANCELLED_HEAD")));
				make_hline();
				print(LineBreak(_lang.get("ERR_NR_3200_0_PROCESS_ABORTED_BY_USER"), _option, false));
				//cerr << LineBreak("|-> Siehe auch \"help procedure\"", _option) << endl;

				addToLog("> NOTE: Process was cancelled by user");
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

					addToLog("> " + toUpperCase(_lang.get("ERR_ERROR")) + ": " + e.getToken());
				}
				else
				{
					string sErrLine_0 = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3]));
					string sErrLine_1 = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_1_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3]));
					string sErrIDString = _lang.getKey("ERR_NR_" + toString((int)e.errorcode) + "_0_*");

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
								printPreFmt("|   Position:  \"..." + e.getExpr().substr(e.getPosition() - 29, 57) + "...\"\n");
								printPreFmt(pointToError(32));
							}
							else if (e.getPosition() < 32)
							{
								string sErrorExpr = "|   Position:  \"";
								if (e.getExpr().length() > 63)
									sErrorExpr += e.getExpr().substr(0, 60) + "...\"";
								else
									sErrorExpr += e.getExpr() + "\"";
								printPreFmt(sErrorExpr + "\n");
								printPreFmt(pointToError(e.getPosition() + 1));
							}
							else if (e.getPosition() > e.getExpr().length() - 32)
							{
								string sErrorExpr = "|   Position:  \"";
								if (e.getExpr().length() > 63)
								{
									printPreFmt(sErrorExpr + "..." + e.getExpr().substr(e.getExpr().length() - 60) + "\"\n");
									printPreFmt(pointToError(65 - (e.getExpr().length() - e.getPosition()) - 2));
								}
								else
								{
									printPreFmt(sErrorExpr + e.getExpr() + "\"\n");
									printPreFmt(pointToError(e.getPosition()));
								}
							}
						}
					}

					addToLog("> " + toUpperCase(_lang.get("ERR_ERROR")) + ": " + sErrIDString);
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
			_option.setSystemPrintStatus(true);
			sendErrorNotification();
			make_hline();
			print(toUpperCase(_lang.get("ERR_CATCHALL_HEAD")));
			make_hline();
			print(LineBreak(_lang.get("ERR_CATCHALL"), _option));

			resetAfterError(sCmdCache);
			make_hline();

			addToLog("> ERROR: UNKNOWN EXCEPTION");
			sendErrorNotification();

			return NUMERE_ERROR;
		}

		_stringParser.removeTempStringVectorVars();

		if (_script.wasLastCommand())
		{
			print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
			_data.setPluginCommands(_procedure.getPluginNames());
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
		_data.setPluginCommands(_procedure.getPluginNames());
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
/// \param sLine string&
/// \param sCmdCache const string&
/// \param sKeep string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleCommandLineSource(string& sLine, const string& sCmdCache, string& sKeep)
{
    if (!sCmdCache.length())
    {
        if (_data.pausedOpening())
        {
            _data.openFromCmdLine(_option, "", true);
            if (_data.isValid())
            {
                print(LineBreak(_lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option, true, 4));
                addToLog("> SYSTEM: Data out of " + _data.getDataFileName("data") + " was successfully loaded.");
            }
        }

        if (_script.getAutoStart())
        {
            print(LineBreak(_lang.get("PARSER_STARTINGSCRIPT", _script.getScriptFileName()), _option, true, 4));
            addToLog("> SYSTEM: Starting Script " + _script.getScriptFileName());
            _script.openScript();
        }

        _data.setCacheStatus(false);

        // --> Wenn gerade ein Script aktiv ist, lese dessen naechste Zeile, sonst nehme eine Zeile von std::cin <--
        if (_script.isValid() && _script.isOpen())
        {
            sLine = _script.getNextScriptCommand();
        }
        else if (_option.readCmdCache().length())
        {
            addToLog("> SYSTEM: Processing command line parameters:");
            sLine = _option.readCmdCache(true);
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

        if (sLine.find("<helpindex>") != string::npos && sLine.find("</helpindex>") != string::npos)
        {
            _procedure.addHelpIndex(sLine.substr(0, sLine.find("<<>>")), getArgAtPos(sLine, sLine.find("id=") + 3));
            sLine.erase(0, sLine.find("<<>>") + 4);
            _option.addToDocIndex(sLine, _option.getUseCustomLanguageFiles());
            _plugin = _procedure;
            return false;
        }

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
                && (sLine.find('(') != string::npos || sLine.find('{') != string::npos))
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
/// \param sLine string&
/// \param sCmdCache string&
/// \param sCurrentCommand const string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::getLineFromCommandCache(string& sLine, string& sCmdCache, const string& sCurrentCommand)
{
    // Only do something if the command cache is not empty or the current line contains a semicolon
    if ((sCmdCache.length() || sLine.find(';') != string::npos) && !_procedure.is_writing() && sCurrentCommand != "procedure")
    {
        if (sCmdCache.length())
        {
            // The command cache is not empty
            // Get the next task from the command cache
            while (sCmdCache.front() == ';' || sCmdCache.front() == ' ')
                sCmdCache.erase(0, 1);
            if (!sCmdCache.length())
                return false;
            if (sCmdCache.find(';') != string::npos)
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
                    if (parens != string::npos)
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
/// \param sLine string&
/// \param sCmdCache const string&
/// \param sCurrentCommand const string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleComposeBlock(string& sLine, const string& sCmdCache, const string& sCurrentCommand, KernelStatus& nReturnVal)
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
                if (_procedure.getLoop())
                {
                    // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                    printPreFmt("|" + _procedure.getCurrentBlock());
                    if (_procedure.getCurrentBlock() == "IF")
                    {
                        if (_procedure.getLoop() > 1)
                            printPreFmt("---");
                        else
                            printPreFmt("-");
                    }
                    else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getLoop() > 1)
                        printPreFmt("-");
                    else
                    {
                        if (_procedure.getLoop() > 1)
                            printPreFmt("--");
                    }
                    printPreFmt(strfill("> ", 2 * _procedure.getLoop(), '-'));
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
            string sCommand = findCommand(sLine).sString;
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
                    if (_procedure.getLoop())
                    {
                        // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                        printPreFmt("|" + _procedure.getCurrentBlock());
                        if (_procedure.getCurrentBlock() == "IF")
                        {
                            if (_procedure.getLoop() > 1)
                                printPreFmt("---");
                            else
                                printPreFmt("-");
                        }
                        else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getLoop() > 1)
                            printPreFmt("-");
                        else
                        {
                            if (_procedure.getLoop() > 1)
                                printPreFmt("--");
                        }
                        printPreFmt(strfill("> ", 2 * _procedure.getLoop(), '-'));
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
/// \param sLine const string&
/// \param sCmdCache const string&
/// \param sCurrentCommand const string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleProcedureWrite(const string& sLine, const string& sCmdCache, const string& sCurrentCommand, KernelStatus& nReturnVal)
{
    if (_procedure.is_writing() || sCurrentCommand == "procedure")
    {
        if (!_procedure.writeProcedure(sLine))
            print(LineBreak(_lang.get("PARSER_CANNOTCREATEPROC"), _option));
        if (!(_script.isValid() && _script.isOpen()) && !sCmdCache.length())
        {
            if (_procedure.getLoop())
            {
                // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                printPreFmt("|" + _procedure.getCurrentBlock());
                if (_procedure.getCurrentBlock() == "IF")
                {
                    if (_procedure.getLoop() > 1)
                        printPreFmt("---");
                    else
                        printPreFmt("-");
                }
                else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getLoop() > 1)
                    printPreFmt("-");
                else
                {
                    if (_procedure.getLoop() > 1)
                        printPreFmt("--");
                }
                printPreFmt(strfill("> ", 2 * _procedure.getLoop(), '-'));
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
/// \param sLine const string&
/// \param sCurrentCommand const string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::uninstallPlugin(const string& sLine, const string& sCurrentCommand)
{
    if (sCurrentCommand == "uninstall")
    {
        // Ge the plugin name
        string sPlugin = fromSystemCodePage(getArgAtPos(sLine, findCommand(sLine).nPos + 9));

        // Remove the plugin and get the help index ID
        sPlugin = _procedure.deletePlugin(sPlugin);
        if (sPlugin.length())
        {
            _plugin = _procedure;
            if (sPlugin != "<<NO_HLP_ENTRY>>")
            {
                while (sPlugin.find(';') != string::npos)
                    sPlugin[sPlugin.find(';')] = ',';
                while (sPlugin.length())
                {
                    // Remove the reference from the help index
                    _option.removeFromDocIndex(getNextArgument(sPlugin, true), _option.getUseCustomLanguageFiles());
                }
            }
            print(LineBreak(_lang.get("PARSER_PLUGINDELETED"), _option));
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
/// \param sLine string&
/// \param sCache string&
/// \param sCurrentCommand string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::handleToCmd(string& sLine, string& sCache, string& sCurrentCommand)
{
    // Do only something, if "to_cmd()" is located
    if (sLine.find("to_cmd(") != string::npos && !_procedure.getLoop())
    {
        unsigned int nPos = 0;

        // Find all "to_cmd()"'s
        while (sLine.find("to_cmd(", nPos) != string::npos)
        {
            nPos = sLine.find("to_cmd(", nPos) + 6;
            if (isInQuotes(sLine, nPos))
                continue;
            unsigned int nParPos = getMatchingParenthesis(sLine.substr(nPos));
            if (nParPos == string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);
            string sCmdString = sLine.substr(nPos + 1, nParPos - 1);
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
/// \param sLine string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::evaluateProcedureCalls(string& sLine)
{
    // Only if there's a candidate for a procedure
    if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) != string::npos && !_procedure.getLoop())
    {
        unsigned int nPos = 0;
        int nProc = 0;

        // Find all procedures
        while (sLine.find('$', nPos) != string::npos && sLine.find('(', sLine.find('$', nPos)) != string::npos)
        {
            unsigned int nParPos = 0;
            nPos = sLine.find('$', nPos) + 1;

            // Get procedure name and argument list
            string __sName = sLine.substr(nPos, sLine.find('(', nPos) - nPos);
            string __sVarList = "";
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
                Returnvalue _rTemp = _procedure.execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script);
                if (!_procedure.getReturnType())
                    sLine = sLine.substr(0, nPos - 1) + sLine.substr(nParPos + 1);
                else
                {
                    _procedure.replaceReturnVal(sLine, _parser, _rTemp, nPos - 1, nParPos + 1, "_~PROC~[" + __sName + "~ROOT_" + toString(nProc) + "]");
                    nProc++;
                }
            }
            nPos += __sName.length() + __sVarList.length() + 1;
        }
        StripSpaces(sLine);
        if (!sLine.length())
            return false;
    }
    else if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) == string::npos)
    {
        // If there's a dollar sign without an opening parenthesis
        // ensure that it is enclosed with quotation marks
        size_t i = sLine.find('$');
        bool isnotinquotes = true;

        // Examine each occurence of a dollar sign
        while (isInQuotes(sLine, i))
        {
            if (sLine.find('$', i + 1) != string::npos)
            {
                i = sLine.find('$', i + 1);
            }
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
/// \param sLine string&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::executePlugins(string& sLine)
{
    // If there's a plugin command
    if (_procedure.isPluginCmd(sLine) && !_procedure.getLoop())
    {
        // Evaluate the command and store procedure name and argument list internally
        if (_procedure.evalPluginCmd(sLine))
        {
            _option.setSystemPrintStatus(false);

            // Call the relevant procedure
            Returnvalue _rTemp = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script);

            // Handle the return values
            if (_rTemp.isString() && sLine.find("<<RETURNVAL>>") != string::npos)
            {
                string sReturn = "{";
                for (unsigned int v = 0; v < _rTemp.vStringVal.size(); v++)
                    sReturn += _rTemp.vStringVal[v] + ",";
                sReturn.back() = '}';
                sLine.replace(sLine.find("<<RETURNVAL>>"), 13, sReturn);
            }
            else if (_rTemp.isNumeric() && sLine.find("<<RETURNVAL>>") != string::npos)
            {
                sLine.replace(sLine.find("<<RETURNVAL>>"), 13, "_~PLUGIN[" + _procedure.getPluginProcName() + "~ROOT]");
                _parser.SetVectorVar("_~PLUGIN[" + _procedure.getPluginProcName() + "~ROOT]", _rTemp.vNumVal);
            }
            _option.setSystemPrintStatus(true);
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
/// \param sLine string&
/// \param sCmdCache const string&
/// \param sCurrentCommand const string&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::handleFlowControls(string& sLine, const string& sCmdCache, const string& sCurrentCommand, KernelStatus& nReturnVal)
{
    if (_procedure.getLoop() || sCurrentCommand == "for" || sCurrentCommand == "if" || sCurrentCommand == "while" || sCurrentCommand == "switch")
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
            if (_procedure.getLoop())
            {
                toggleTableStatus();
                // --> Wenn in "_procedure" geschrieben wird und dabei kein Script ausgefuehrt wird, hebe dies entsprechend hervor <--
                printPreFmt("|" + _procedure.getCurrentBlock());

                if (_procedure.getCurrentBlock() == "IF")
                {
                    if (_procedure.getLoop() > 1)
                        printPreFmt("---");
                    else
                        printPreFmt("-");
                }
                else if (_procedure.getCurrentBlock() == "ELSE" && _procedure.getLoop() > 1)
                    printPreFmt("-");
                else
                {
                    if (_procedure.getLoop() > 1)
                        printPreFmt("--");
                }

                toggleTableStatus();
                printPreFmt(strfill("> ", 2 * _procedure.getLoop(), '-'));
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
                    _data.setPluginCommands(_procedure.getPluginNames());
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
            print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
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
/// \param sLine string&
/// \param sCache string&
/// \param sCmdCache const string&
/// \param bWriteToCache bool&
/// \param nReturnVal KernelStatus&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReKernel::evaluateStrings(string& sLine, string& sCache, const string& sCmdCache, bool& bWriteToCache, KernelStatus& nReturnVal)
{
    if (_stringParser.isStringExpression(sLine))
    {
        auto retVal = _stringParser.evalAndFormat(sLine, sCache);

        if (retVal == NumeRe::StringParser::STRING_SUCCESS)
        {
            if (!sCmdCache.length() && !(_script.isValid() && _script.isOpen()))
            {
                if (_script.wasLastCommand())
                {
                    print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
                    _data.setPluginCommands(_procedure.getPluginNames());
                }
                sCommandLine.clear();
                bCancelSignal = false;
                nReturnVal = NUMERE_DONE_KEYWORD;
                return false;
            }
            else
                return false;
        }

        if (sCache.length() && _data.containsTablesOrClusters(sCache) && !bWriteToCache)
            bWriteToCache = true;
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
/// \param sCmdCache const string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::createCalculationAnswer(int nNum, value_type* v, const string& sCmdCache)
{
    vAns = v[0];
    getAns().clear();
    getAns().setDoubleArray(nNum, v);

    if (!bSupressAnswer)
        printResult(formatResultOutput(nNum, v), sCmdCache, _script.isValid() && _script.isOpen());
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// reset the kernel variables after an error had
/// been handled.
///
/// \param sCmdCache string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::resetAfterError(string& sCmdCache)
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
/// \return string
///
/////////////////////////////////////////////////
string NumeReKernel::getGreeting()
{
	unsigned int nth_Greeting = 0;
	vector<string> vGreetings;

	// Get the greetings from the database file
	if (_option.getUseCustomLanguageFiles() && fileExists(_option.ValidFileName("<>/user/docs/greetings.ndb", ".ndb")))
		vGreetings = getDBFileContent("<>/user/docs/greetings.ndb", _option);
	else
		vGreetings = getDBFileContent("<>/docs/greetings.ndb", _option);

	string sLine;

	if (!vGreetings.size())
		return "|-> ERROR: GREETINGS FILE IS EMPTY.\n";

	// --> Einen Seed (aus der Zeit generiert) an die rand()-Funktion zuweisen <--
	srand(time(NULL));

	// --> Die aktuelle Begruessung erhalten wir als modulo(nGreetings)-Operation auf rand() <--
	nth_Greeting = (rand() % vGreetings.size());

	if (nth_Greeting >= vGreetings.size())
		nth_Greeting = vGreetings.size() - 1;

	// --> Gib die zufaellig ausgewaehlte Begruessung zurueck <--
	return "|-> \"" + vGreetings[nth_Greeting] + "\"\n";
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
		nLINE_LENGTH = nLength;
		_option.setWindowSize(nLength);
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
	if (!_data.getSaveStatus())
	{
		_data.saveToCacheFile();
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
	saveData();
	_data.clearCache();
	_data.removeData(false);

	// --> Konfiguration aus den Objekten zusammenfassen und anschliessend speichern <--
	_option.setSavePath(_out.getPath());
	_option.setLoadPath(_data.getPath());
	_option.setPlotOutputPath(_pData.getPath());
	_option.setScriptPath(_script.getPath());

	// Save the function definitions
	if (_option.getbDefineAutoLoad() && _functions.getDefinedFunctions())
	{
		_option.setSystemPrintStatus(false);
		_functions.save(_option);
		Sleep(100);
	}
	_option.save(_option.getExePath()); // MAIN_QUIT

	// Write an information to the log file
	if (oLogFile.is_open())
	{
		oLogFile << "--- NUMERE WAS TERMINATED SUCCESSFULLY ---" << endl << endl << endl;
		oLogFile.close();
	}

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
/// \return string
///
/////////////////////////////////////////////////
string NumeReKernel::ReadAnswer()
{
	string sAns = sAnswer;
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
/// \return map<string, string>
///
/////////////////////////////////////////////////
map<string, string> NumeReKernel::getPluginLanguageStrings()
{
	map<string, string> mPluginLangStrings;
	for (size_t i = 0; i < _procedure.getPluginCount(); i++)
	{
		string sDesc = _procedure.getPluginCommand(i) + "     - " + _procedure.getPluginDesc(i);
		while (sDesc.find("\\\"") != string::npos)
			sDesc.erase(sDesc.find("\\\""), 1);

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
/// \return map<string, string>
///
/////////////////////////////////////////////////
map<string, string> NumeReKernel::getFunctionLanguageStrings()
{
	map<string, string> mFunctionLangStrings;
	for (size_t i = 0; i < _functions.getDefinedFunctions(); i++)
	{
		string sDesc = _functions.getFunction(i) + "     ARG   - " + _functions.getComment(i);
		while (sDesc.find("\\\"") != string::npos)
			sDesc.erase(sDesc.find("\\\""), 1);

		mFunctionLangStrings["PARSERFUNCS_LISTFUNC_FUNC_" + toUpperCase(_functions.getFunction(i).substr(0, _functions.getFunction(i).rfind('('))) + "_[DEFINE]"] = sDesc;
	}
	return mFunctionLangStrings;
}


/////////////////////////////////////////////////
/// \brief This member function is used by the
/// syntax highlighter to hightlight the plugin
/// commands.
///
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> NumeReKernel::getPluginCommands()
{
	vector<string> vPluginCommands;

	for (size_t i = 0; i < _procedure.getPluginCount(); i++)
		vPluginCommands.push_back(_procedure.getPluginCommand(i));
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
/// \param sCommand const string&
/// \return string
///
/////////////////////////////////////////////////
string NumeReKernel::getDocumentation(const string& sCommand)
{
	return doc_HelpAsHTML(sCommand, false, _option);
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// documentation index as a string vector, which
/// can be used to fill the tree in the
/// documentation browser.
///
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> NumeReKernel::getDocIndex()
{
    return _option.getDocIndex();
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

    mu::varmap_type varmap = _parser.GetVar();
    map<string, string> stringmap = _stringParser.getStringVars();
    map<string, long long int> tablemap = _data.mCachesMap;
    const map<string, NumeRe::Cluster>& clustermap = _data.getClusterMap();
    string sCurrentLine;

    if (_data.isValid())
        tablemap["data"] = -1;
    if (_data.getStringElements())
        tablemap["string"] = -2;

    // Gather all (global) numerical variables
	for (auto iter = varmap.begin(); iter != varmap.end(); ++iter)
    {
        if ((iter->first).substr(0, 2) == "_~")
            continue;

        sCurrentLine = iter->first + "\t1 x 1\tdouble\t" + toString(*iter->second, 5) + "\t" + iter->first;
        vars.vVariables.push_back(sCurrentLine);
    }

    vars.nNumerics = vars.vVariables.size();

    // Gather all (global) string variables
	for (auto iter = stringmap.begin(); iter != stringmap.end(); ++iter)
    {
        if ((iter->first).substr(0, 2) == "_~")
            continue;

        sCurrentLine = iter->first + "\t1 x 1\tstring\t\"" + replaceControlCharacters(iter->second) + "\"\t" + iter->first;
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
            sCurrentLine = iter->first + "()\t" + toString(_data.getStringElements()) + " x " + toString(_data.getStringCols());
            sCurrentLine += "\tstring\t{\"" + replaceControlCharacters(_data.minString()) + "\", ..., \"" + replaceControlCharacters(_data.maxString()) + "\"}\tstring()";
        }
        else
        {
            sCurrentLine = iter->first + "()\t" + toString(_data.getLines(iter->first, false)) + " x " + toString(_data.getCols(iter->first, false));
            sCurrentLine += "\tdouble\t{" + toString(_data.min(iter->first, "")[0], 5) + ", ..., " + toString(_data.max(iter->first, "")[0], 5) + "}\t" + iter->first + "()";
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
        sCurrentLine += "\tcluster\t" + iter->second.getShortVectorRepresentation() + "\t" + iter->first + "{}";

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
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> NumeReKernel::getPathSettings() const
{
	vector<string> vPaths;
	vPaths.push_back(_option.getExePath()); //0
	vPaths.push_back(_option.getWorkPath()); //1
	vPaths.push_back(_option.getLoadPath()); //2
	vPaths.push_back(_option.getSavePath()); //3
	vPaths.push_back(_option.getScriptPath()); //4
	vPaths.push_back(_option.getProcsPath()); //5
	vPaths.push_back(_option.getPlotOutputPath()); //6

	return vPaths;
}


/////////////////////////////////////////////////
/// \brief This member function appends the
/// formatted string to the buffer and informs the
/// terminal that we have a new string to print.
///
/// \param sLine const string&
/// \param sCmdCache const string&
/// \param bScriptRunning bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::printResult(const string& sLine, const string& sCmdCache, bool bScriptRunning)
{
	if (!m_parent)
		return;

	if (sCmdCache.length() || bScriptRunning )
	{
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
	else
		sAnswer = sLine;
}


/////////////////////////////////////////////////
/// \brief This member function masks the dollar
/// signs in the strings to avoid that the line
/// breaking functions uses them as aliases for
/// line breaks.
///
/// \param sLine string
/// \return string
///
/////////////////////////////////////////////////
string NumeReKernel::maskProcedureSigns(string sLine)
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
/// \param __sLine const string&
/// \param printingEnabled bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::print(const string& __sLine, bool printingEnabled)
{
	if (!m_parent || !printingEnabled)
		return;
	else
	{
		string sLine = __sLine;

		if (bErrorNotification)
		{
			if (sLine.front() == '\r')
				sLine.insert(1, 1, (char)15);
			else
				sLine.insert(0, 1, (char)15);

			for (size_t i = 0; i < sLine.length(); i++)
			{
				if (sLine[i] == '\n' && i < sLine.length() - 2)
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
/// \param __sLine const string&
/// \param printingEnabled bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::printPreFmt(const string& __sLine, bool printingEnabled)
{
	if (!m_parent || !printingEnabled)
		return;
	else
	{
		string sLine = __sLine;

		if (bErrorNotification)
		{
			if (sLine.front() == '\r')
				sLine.insert(1, 1, (char)15);
			else
				sLine.insert(0, 1, (char)15);

			for (size_t i = 0; i < sLine.length(); i++)
			{
				if (sLine[i] == '\n' && i < sLine.length() - 2)
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
/// \return string
///
/////////////////////////////////////////////////
string NumeReKernel::formatResultOutput(int nNum, value_type* v)
{
    Settings& _option = getInstance()->getSettings();

    if (nNum > 1)
    {
        // More than one result
        //
        // How many fit into one line?
        int nLineBreak = numberOfNumbersPerLine();
        string sAns = "ans = {";

        // compose the result
        for (int i = 0; i < nNum; ++i)
        {
            sAns += strfill(toString(v[i], _option), _option.getPrecision() + 7);

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
        return "ans = " + toString(v[0], _option);
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
/// \param vStringResults const vector<string>&
/// \return string
///
/////////////////////////////////////////////////
string NumeReKernel::formatResultOutput(const vector<string>& vStringResults)
{
    Settings& _option = getInstance()->getSettings();

    if (vStringResults.size() > 1)
    {
        // More than one result
        //
        // How many fit into one line?
        size_t nLineBreak = numberOfNumbersPerLine();
        string sAns = "ans = {";
        size_t nNum = vStringResults.size();

        // compose the result
        for (size_t i = 0; i < nNum; ++i)
        {
            sAns += strfill(truncString(vStringResults[i], _option.getPrecision()+6), _option.getPrecision() + 7);

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
/// \param sWarningMessage string
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::issueWarning(string sWarningMessage)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);

        // Insert warning symbols, if linebreaks are contained in this message
        replaceAll(sWarningMessage, "\n", "\n|!> ");

        m_parent->m_sAnswer += "\r|!> " + _lang.get("COMMON_WARNING") + ": " + sWarningMessage + "\n";

        if (m_parent->m_KernelStatus < NumeReKernel::NUMERE_STATUSBAR_UPDATE || m_parent->m_KernelStatus == NumeReKernel::NUMERE_ANSWER_READ)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_ISSUE_WARNING;

    }

    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(KERNEL_PRINT_SLEEP);
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
	return (getInstance()->getSettings().getWindow() - 1 - 15) / (getInstance()->getSettings().getPrecision() + 9);
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
/// \brief This function displays a progress bar
/// constructed from characters in the terminal.
///
/// \param nStep int
/// \param nFirstStep int
/// \param nFinalStep int
/// \param sType const string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::progressBar(int nStep, int nFirstStep, int nFinalStep, const string& sType)
{
	int nStatusVal = 0;
	const int BARLENGTH = 40;
	const double BARDIVISOR = 2.5;

	if (abs(nFinalStep - nFirstStep) < 9999
        && abs((nStep - nFirstStep) / (double)(nFinalStep - nFirstStep) * BARLENGTH)
        > abs((nStep - 1 - nFirstStep) / (double)(nFinalStep - nFirstStep) * BARLENGTH))
	{
		nStatusVal = abs((int)((nStep - nFirstStep) / (double)(nFinalStep - nFirstStep) * BARLENGTH)) * BARDIVISOR;
	}
	else if (abs(nFinalStep - nFirstStep) >= 9999
        && abs((nStep - nFirstStep) / (double)(nFinalStep - nFirstStep) * 100)
        > abs((nStep - 1 - nFirstStep) / (double)(nFinalStep - nFirstStep) * 100))
	{
		nStatusVal = abs((int)((nStep - nFirstStep) / (double)(nFinalStep - nFirstStep) * 100));
	}

	if (nLastStatusVal >= 0 && nLastStatusVal == nStatusVal && (sType != "cancel" && sType != "bcancel"))
		return;

	toggleTableStatus();

	if (nLastLineLength > 0)
		printPreFmt("\r");

	if (sType == "std")
	{
		printPreFmt("\r|-> " + _lang.get("COMMON_EVALUATING") + " ... " + toString(nStatusVal) + " %");
		nLastLineLength = 14 + _lang.get("COMMON_EVALUATING").length();
	}
	else if (sType == "cancel")
	{
		printPreFmt("\r|-> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL"));
		nStep = nFinalStep;
	}
	else if (sType == "bar")
	{
		printPreFmt("\r|-> [" + strfill("#", (int)(nStatusVal / BARDIVISOR), '#') + strfill(" ", BARLENGTH - (int)(nStatusVal / BARDIVISOR)) + "] (" + toString(nStatusVal) + " %)");
		nLastLineLength = 14 + BARLENGTH;
	}
	else if (sType == "bcancel")
	{
		printPreFmt("\r|-> [" + strfill("#", (int)(nLastStatusVal / BARDIVISOR), '#') + strfill(" ", BARLENGTH - (int)(nLastStatusVal / BARDIVISOR)) + "] (--- %)");
		nFinalStep = nStep;
	}
	else
	{
		nLastLineLength = 1;
		printPreFmt("\r|");

		for (unsigned int i = 0; i < sType.length(); i++)
		{
			if (sType.substr(i, 5) == "<bar>")
			{
				printPreFmt("[" + strfill("#", (int)(nStatusVal / BARDIVISOR), '#') + strfill(" ", BARLENGTH - (int)(nStatusVal / BARDIVISOR)) + "]");
				i += 4;
				nLastLineLength += 2 + BARLENGTH;
				continue;
			}

			if (sType.substr(i, 5) == "<Bar>")
			{
				printPreFmt("[" + strfill("#", (int)(nStatusVal / BARDIVISOR), '#') + strfill("-", BARLENGTH - (int)(nStatusVal / BARDIVISOR), '-') + "]");
				i += 4;
				nLastLineLength += 2 + BARLENGTH;
				continue;
			}

			if (sType.substr(i, 5) == "<BAR>")
			{
				printPreFmt("[" + strfill("#", (int)(nStatusVal / BARDIVISOR), '#') + strfill("=", BARLENGTH - (int)(nStatusVal / BARDIVISOR), '=') + "]");
				i += 4;
				nLastLineLength += 2 + BARLENGTH;
				continue;
			}

			if (sType.substr(i, 5) == "<val>")
			{
				printPreFmt(toString(nStatusVal));
				i += 4;
				nLastLineLength += 3;
				continue;
			}

			if (sType.substr(i, 5) == "<Val>")
			{
				printPreFmt(strfill(toString(nStatusVal), 3));
				i += 4;
				nLastLineLength += 3;
				continue;
			}

			if (sType.substr(i, 5) == "<VAL>")
			{
				printPreFmt(strfill(toString(nStatusVal), 3, '0'));
				i += 4;
				nLastLineLength += 3;
				continue;
			}

			printPreFmt(sType.substr(i, 1));
			nLastLineLength++;
		}
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
/// \param sFile const string&
/// \param nLine unsigned int
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::gotoLine(const string& sFile, unsigned int nLine)
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
/// \param _sDocumentation const string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::setDocumentation(const string& _sDocumentation)
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
/// \brief This member function passes a table to
/// the GUI to be displayed in the table viewer.
/// It also allows to create a table editor window.
///
/// \param _table NumeRe::Table
/// \param __name string
/// \param openeditable bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::showTable(NumeRe::Table _table, string __name, bool openeditable)
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
/// \param _stringtable NumeRe::Container<string>
/// \param __name string
/// \param openeditable bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::showStringTable(NumeRe::Container<string> _stringtable, string __name, bool openeditable)
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
/// \param sTableName const string&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table NumeReKernel::getTable(const string& sTableName)
{
    string sSelectedTable = sTableName;

    if (sSelectedTable.find("()") != string::npos)
        sSelectedTable.erase(sSelectedTable.find("()"));

    if ((!_data.isTable(sSelectedTable) && sSelectedTable != "data") || !_data.getCols(sSelectedTable))
        return NumeRe::Table();

    return _data.extractTable(sSelectedTable);
}


/////////////////////////////////////////////////
/// \brief This member function creates the table
/// container for the string table or the clusters.
///
/// \param sStringTableName const string&
/// \return NumeRe::Container<string>
///
/////////////////////////////////////////////////
NumeRe::Container<string> NumeReKernel::getStringTable(const string& sStringTableName)
{
    if (sStringTableName == "string()")
    {
        // Create the container for the string table
        NumeRe::Container<string> stringTable(_data.getStringElements(), _data.getStringCols());

        for (size_t j = 0; j < _data.getStringCols(); j++)
        {
            for (size_t i = 0; i < _data.getStringElements(j); i++)
            {
                stringTable.set(i, j, "\"" + _data.readString(i, j) + "\"");
            }
        }

        return stringTable;
    }
    else if (_data.isCluster(sStringTableName))
    {
        // Create the container for the selected cluster
        NumeRe::Cluster& clust = _data.getCluster(sStringTableName.substr(0, sStringTableName.find("{}")));
        NumeRe::Container<string> stringTable(clust.size(), 1);

        for (size_t i = 0; i < clust.size(); i++)
        {
            if (clust.getType(i) == NumeRe::ClusterItem::ITEMTYPE_STRING)
                stringTable.set(i, 0, clust.getString(i));
            else
                stringTable.set(i, 0, toString(clust.getDouble(i), 5));
        }

        return stringTable;
    }

    return NumeRe::Container<string>();
}


/////////////////////////////////////////////////
/// \brief This member function passes the
/// debugging information to the GUI to be
/// displayed in the debugger window.
///
/// \param sTitle const string&
/// \param vStacktrace const vector<string>&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::showDebugEvent(const string& sTitle, const vector<string>& vStacktrace)
{
	if (!m_parent)
		return;
	else
	{
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
/// \param sCurrentCommand const string&
/// \return int
///
/////////////////////////////////////////////////
int NumeReKernel::evalDebuggerBreakPoint(const string& sCurrentCommand)
{
    if (!getInstance())
        return DEBUGGER_CONTINUE;

	mu::varmap_type varmap;
	string** sLocalVars = nullptr;
	double* dLocalVars = nullptr;
	size_t nLocalVarMapSize = 0;
	size_t nLocalVarMapSkip = 0;
	string** sLocalStrings = nullptr;
	size_t nLocalStringMapSize = 0;
	string** sLocalTables = nullptr;
	size_t nLocalTableMapSize = 0;
	string** sLocalClusters = nullptr;
	size_t nLocalClusterMapSize = 0;

	// Obtain references to the debugger and the parser
	NumeReDebugger& _debugger = getInstance()->getDebugger();
	Parser& _parser = getInstance()->getParser();

	// Get the numerical variable map
    varmap = _parser.GetVar();
    nLocalVarMapSize = varmap.size();
    sLocalVars = new string*[nLocalVarMapSize];
    dLocalVars = new double[nLocalVarMapSize];
    size_t i = 0;

    // Create the numerical variable set
    for (auto iter = varmap.begin(); iter != varmap.end(); ++iter)
    {
        sLocalVars[i + nLocalVarMapSkip] = new string[2];

        if ((iter->first).substr(0, 2) == "_~")
        {
            nLocalVarMapSkip++;
            continue;
        }

        sLocalVars[i][0] = iter->first;
        sLocalVars[i][1] = iter->first;
        dLocalVars[i] = *(iter->second);
        i++;
    }

    // Get the string variable map
    map<string, string> sStringMap = getInstance()->getStringParser().getStringVars();

    nLocalStringMapSize = sStringMap.size();

    // Create the string variable set
    if (nLocalStringMapSize)
    {
        sLocalStrings = new string*[nLocalStringMapSize];
        i = 0;

        for (auto iter = sStringMap.begin(); iter != sStringMap.end(); ++iter)
        {
            sLocalStrings[i] = new string[2];
            sLocalStrings[i][0] = iter->first;
            sLocalStrings[i][1] = iter->first;
            i++;
        }
    }

    // Get the table variable map
    map<string, long long int> tableMap = getInstance()->getData().mCachesMap;

    if (getInstance()->getData().isValid())
        tableMap["data"] = -1;

    if (getInstance()->getData().getStringElements())
        tableMap["string"] = -2;

    nLocalTableMapSize = tableMap.size();

    // Create the table variable set
    if (nLocalTableMapSize)
    {
        sLocalTables = new string*[nLocalTableMapSize];
        i = 0;

        for (auto iter = tableMap.begin(); iter != tableMap.end(); ++iter)
        {
            sLocalTables[i] = new string[2];
            sLocalTables[i][0] = iter->first;
            sLocalTables[i][1] = iter->first;
            i++;
        }
    }

    const map<string, NumeRe::Cluster>& clusterMap = getInstance()->getData().getClusterMap();

    nLocalClusterMapSize = clusterMap.size();

    // Create the cluster variable set
    if (nLocalClusterMapSize)
    {
        sLocalClusters = new string*[nLocalClusterMapSize];
        i = 0;

        for (auto iter = clusterMap.begin(); iter != clusterMap.end(); ++iter)
        {
            sLocalClusters[i] = new string[2];
            sLocalClusters[i][0] = iter->first + "{";
            sLocalClusters[i][1] = iter->first;
            i++;
        }
    }


    // Pass the created information to the debugger
	_debugger.gatherInformations(sLocalVars, nLocalVarMapSize - nLocalVarMapSkip, dLocalVars, sLocalStrings, nLocalStringMapSize, sLocalTables, nLocalTableMapSize, sLocalClusters, nLocalClusterMapSize, nullptr, 0, sCurrentCommand,
                                 getInstance()->getScript().getScriptFileName(), getInstance()->getScript().getCurrentLine() - 1);

    // Clean up memory
	if (sLocalVars)
	{
		for (size_t i = 0; i < nLocalVarMapSize; i++)
			delete[] sLocalVars[i];

		delete[] sLocalVars;
		delete[] dLocalVars;
	}

	if (sLocalStrings)
	{
		for (size_t i = 0; i < nLocalStringMapSize; i++)
			delete[] sLocalStrings[i];

		delete[] sLocalStrings;
	}

	if (sLocalTables)
	{
		for (size_t i = 0; i < nLocalTableMapSize; i++)
			delete[] sLocalTables[i];

		delete[] sLocalTables;
	}

	if (sLocalClusters)
	{
		for (size_t i = 0; i < nLocalClusterMapSize; i++)
			delete[] sLocalClusters[i];

		delete[] sLocalClusters;
	}

	// Show the breakpoint and wait for the
	// user interaction
	return _debugger.showBreakPoint();
}


/////////////////////////////////////////////////
/// \brief This member function appends the passed
/// string to the command log.
///
/// \param sLogMessage const string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::addToLog(const string& sLogMessage)
{
    if (oLogFile.is_open())
        oLogFile << toString(time(0) - tTimeZero, true) << "> " << sLogMessage << endl;
}


/////////////////////////////////////////////////
/// \brief This function is an implementation
/// replacing the std::getline() function.
///
/// \param sLine string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReKernel::getline(string& sLine)
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

	if (bCancel || GetAsyncKeyState(VK_ESCAPE))
		return true;

	return false;
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
		NumeReKernel::printPreFmt("\r" + strfill(string(1, '='), NumeReKernel::nLINE_LENGTH - 1, '=') + "\n");
	else if (nLength < -1)
		NumeReKernel::printPreFmt("\r" + strfill(string(1, '-'), NumeReKernel::nLINE_LENGTH - 1, '-') + "\n");
	else
		NumeReKernel::printPreFmt("\r" + strfill(string(1, '='), nLength, '=') + "\n");

	return;
}

