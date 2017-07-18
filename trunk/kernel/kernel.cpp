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
#include "../gui/wxterm.h"

extern const string sVersion;
/* --> STATUS: Versionsname des Programms; Aktuell "Ampere", danach "Angstroem". Ab 1.0 Namen mit "B",
 *     z.B.: Biot(1774), Boltzmann(1844), Becquerel(1852), Bragg(1862), Bohr(1885), Brillouin(1889),
 *     de Broglie(1892, Bose(1894), Bloch(1905), Bethe(1906)) <--
 */

Language _lang;
mglGraph _fontData;
extern Plugin _plugin;
extern value_type vAns;
extern Integration_Vars parser_iVars;
time_t tTimeZero = time(0);


wxTerm* NumeReKernel::m_parent = nullptr;
int NumeReKernel::nLINE_LENGTH = 80;
bool NumeReKernel::bWritingTable = false;
string NumeReKernel::sFileToEdit = "";
string NumeReKernel::sDocumentation = "";
unsigned int NumeReKernel::nLineToGoTo = 0;
int NumeReKernel::nOpenFileFlag = 0;
int NumeReKernel::nLastStatusVal = 0;
unsigned int NumeReKernel::nLastLineLength = 0;
bool NumeReKernel::modifiedSettings = false;
bool NumeReKernel::bCancelSignal = false;
stringmatrix NumeReKernel::sTable;
vector<string> NumeReKernel::vDebugInfos;
string NumeReKernel::sTableName = "";
Debugmessenger NumeReKernel::_messenger;
bool NumeReKernel::bSupressAnswer = false;
bool NumeReKernel::bGettingLine = false;
bool NumeReKernel::bErrorNotification = false;
size_t NumeReKernel::nScriptLine = 0;
string NumeReKernel::sScriptFileName = "";
ProcedureLibrary NumeReKernel::ProcLibrary;

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
bool IsWow64()
{
    BOOL bIsWow64 = false;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            return false;
        }
    }
    return (bool)bIsWow64;
}

NumeReKernel::NumeReKernel()
{
    sCommandLine.clear();
    sAnswer.clear();
    sPlotCompose.clear();
}

NumeReKernel::~NumeReKernel()
{
    CloseSession();
}

Settings NumeReKernel::getKernelSettings()
{
    return _option.sendSettings();
}

void NumeReKernel::setKernelSettings(const Settings& _settings)
{
    _option.copySettings(_settings);
}

void NumeReKernel::Autosave()
{
    if (!_data.getSaveStatus())
        _data.saveCache();
    return;
}

void NumeReKernel::StartUp(wxTerm* _parent)
{
    if (_parent && m_parent == nullptr)
        m_parent = _parent;
    //Do some start-up stuff here

    string sFile = ""; 			// String fuer den Dateinamen.
	string sScriptName = "";
	string sTime = getTimeStamp(false);
	string sLogFile = "numere.log";

    _data.setPredefinedFuncs(_functions.getPredefinedFuncs());
    //Sleep(50);
    //cerr << "Done.";

    //nextLoadMessage(50);
    //cerr << " -> Reading system's information ... ";
    char __cPath[1024];
    OSVERSIONINFOA _osversioninfo;
    _osversioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&_osversioninfo);
    GetModuleFileNameA(NULL, __cPath, 1024);
    string sPath = __cPath;

    sPath = sPath.substr(0,sPath.rfind("\\numere.exe"));


    while (sPath.find('\\') != string::npos)
        sPath[sPath.find('\\')] = '/';
    //Sleep(50);
    //cerr << "Done.";

    //nextLoadMessage(50);

    _option.setExePath(sPath);
 	_option.load(sPath);				// Lade Informationen aus einem ini-File

    if (_option.getbUseLogFile())
    {
        reduceLogFilesize((sPath+"/"+sLogFile).c_str());
        oLogFile.open((sPath+"/"+sLogFile).c_str(), ios_base::out | ios_base::app | ios_base::ate);
        if (oLogFile.fail())
            oLogFile.close();
    }
    if (oLogFile.is_open())
    {
        oLogFile << "--- NUMERE-SESSION-PROTOCOL: " << sTime << " ---" << endl;
        oLogFile << "--- NumeRe v " << sVersion
                 << " | Build " << AutoVersion::YEAR << "-" << AutoVersion::MONTH << "-" << AutoVersion::DATE
                 << " | OS: Windows v " << _osversioninfo.dwMajorVersion << "." << _osversioninfo.dwMinorVersion << "." << _osversioninfo.dwBuildNumber << " " << _osversioninfo.szCSDVersion << (IsWow64() ? " (64 Bit) ---" : " ---") << endl;
    }

 	//nextLoadMessage(50);
 	//cerr << " -> Setting global parameters ... ";
 	_data.setTokens(_option.getTokenPaths());
 	_out.setTokens(_option.getTokenPaths());
 	_pData.setTokens(_option.getTokenPaths());
 	_script.setTokens(_option.getTokenPaths());
 	_functions.setTokens(_option.getTokenPaths());
 	_procedure.setTokens(_option.getTokenPaths());
 	_option.setTokens(_option.getTokenPaths());
 	_lang.setTokens(_option.getTokenPaths());
	//ResizeConsole(_option);
    nLINE_LENGTH = _option.getWindow();
    //Sleep(50);
    //cerr << "Done.";

 	//nextLoadMessage(50);
 	//cerr << toSystemCodePage(" -> Verifying NumeRe file system ... ");
	_out.setPath(_option.getSavePath(), true, sPath);
	_data.setPath(_option.getLoadPath(), true, sPath);
	_data.setSavePath(_option.getSavePath());
	_data.setbLoadEmptyCols(_option.getbLoadEmptyCols());
	_pData.setPath(_option.getPlotOutputPath(), true, sPath);
	_script.setPath(_option.getScriptPath(), true, sPath);
	_procedure.setPath(_option.getProcsPath(), true, sPath);
	_option.setPath(_option.getExePath() + "/docs/plugins", true, sPath);
	_option.setPath(_option.getExePath() + "/docs", true, sPath);
	_option.setPath(_option.getExePath() + "/user/lang", true, sPath);
	_option.setPath(_option.getExePath() + "/user/docs", true, sPath);
	_functions.setPath(_option.getExePath(), false, sPath);
	//Sleep(50);
    //cerr << "Done.";
    if (oLogFile.is_open())
        oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: File system was verified." << endl;

    //nextLoadMessage(50);
    //cerr << toSystemCodePage(" -> Loading documentation index ... ");
    _option.loadDocIndex(false);
    //Sleep(50);
    //cerr << "Done.";
    if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Documentation index was loaded." << endl;

    if (BI_FileExists(_option.getExePath() + "/update.hlpidx"))
    {
        //nextLoadMessage(50);
        //cerr << toSystemCodePage(" -> Updating documentation index ... ");
        _option.updateDocIndex();
        //Sleep(50);
        //cerr << "Done.";
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Documentation index was updated." << endl;
    }
    if (_option.getUseCustomLanguageFiles())
    {
        //nextLoadMessage(50);
        //cerr << toSystemCodePage(" -> Loading user documentation index ... ");
        _option.loadDocIndex(_option.getUseCustomLanguageFiles());
        //Sleep(50);
        //cerr << "Done.";
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: User Documentation index was loaded." << endl;
    }
    //nextLoadMessage(50);
    //cerr << toSystemCodePage(" -> Loading language files ... ");
    _lang.loadStrings(_option.getUseCustomLanguageFiles());
    //Sleep(50);
    //cerr << "Done.";
    if (oLogFile.is_open())
        oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Language files were loaded." << endl;


    //if (_option.getbDebug())
    //    cerr << "PATH: " << __cPath << endl;

    // --> Hier wollen wir den Titel der Console aendern. Ist eine Windows-Funktion <--
    //SetConsTitle(_data, _option);

    string sAutosave = _option.getSavePath() + "/cache.tmp";
    string sCacheFile = _option.getExePath() + "/numere.cache";


	/**if (!_option.getbFastStart())
	{
        nextLoadMessage(50);
        cerr << toSystemCodePage(" -> " + _lang.get("MAIN_LOADING_PARSER_SELFTEST") + " ... ");
        Sleep(600);
        parser_SelfTest(_parser);   // Fuehre den Parser-Selbst-Test aus
        Sleep(650);				    // Warte 500 msec
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Parser self test was done." << endl;
    }*/
    //nextLoadMessage(50);
    //cerr << toSystemCodePage(" -> " + _lang.get("MAIN_LOADING_IOSTREAM") + " ... ");
    //Sleep(50);
    //cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
    if (BI_FileExists(_procedure.getPluginInfoPath()))
    {
        //nextLoadMessage(50);
        //cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_PLUGINS")+" ... ", _option);
        _procedure.loadPlugins();
        _plugin = _procedure;
        _data.setPluginCommands(_procedure.getPluginNames());
        //cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Plugin information was loaded." << endl;
    }
    if (_option.getbDefineAutoLoad() && BI_FileExists(_option.getExePath() + "\\functions.def"))
    {
        //nextLoadMessage(50);
        //cerr << " -> ";
        _functions.load(_option, true);
        //if (!_option.getbFastStart())
        //    Sleep(350);
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Function definitions were loaded." << endl;
    }

    //nextLoadMessage(50);
    //cerr << toSystemCodePage(" -> " + _lang.get("MAIN_LOADING_FONT", toUpperCase(_option.getDefaultPlotFont().substr(0,1))+_option.getDefaultPlotFont().substr(1)) + " ... ");
    //cerr << toSystemCodePage(" -> Lade Schriftsatz \""+toUpperCase(_option.getDefaultPlotFont().substr(0,1))+_option.getDefaultPlotFont().substr(1)+"\" für Graph ... ");
    _fontData.LoadFont(_option.getDefaultPlotFont().c_str(), (_option.getExePath()+ "\\fonts").c_str());
    //cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";

    //nextLoadMessage(50);
    //cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_AUTOSAVE_SEARCH")+" ... ", _option);
    //Sleep(50);
    if (BI_FileExists(sAutosave) || BI_FileExists(sCacheFile))
    {
        //cerr << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_FOUND"));
        if (BI_FileExists(sAutosave))
        {
            // --> Lade den letzten Cache, falls dieser existiert <--
            //nextLoadMessage(50);
            //cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_AUTOSAVE")+" ... ", _option);
            _data.openAutosave(sAutosave, _option);
            _data.setSaveStatus(true);
            remove(sAutosave.c_str());
            //cerr << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_TRANSLATING")+" ... ");
            if (_data.saveCache());
                /**cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
            else
            {
                cerr << endl << " -> " << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_ERROR_SAVING")) << endl;
                Sleep(50);
            }*/
        }
        else
        {
            //nextLoadMessage(50);
            //cerr << LineBreak(" -> "+_lang.get("MAIN_LOADING_AUTOSAVE")+" ... ", _option);
            if (_data.loadCache());
                /**cerr << toSystemCodePage(_lang.get("COMMON_DONE")) << ".";
            else
            {
                cerr << endl << " -> " << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_ERROR_LOADING")) << endl;
                Sleep(50);
            }*/
        }
        if (oLogFile.is_open())
            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Automatic backup was loaded." << endl;
    }
    /**else
        cerr << toSystemCodePage(_lang.get("MAIN_LOADING_AUTOSAVE_NOT_FOUND"));*/


	if (sScriptName.length())
	{
        _script.setScriptFileName(sScriptName);
        _script.setAutoStart(true);
    }

    _parser.DefineVar(_nrT("ans"), &vAns);        // Deklariere die spezielle Variable "ans", die stets, das letzte Ergebnis speichert und die vier Standardvariablen
    _parser.DefineVar(parser_iVars.sName[0], &parser_iVars.vValue[0][0]);
    _parser.DefineVar(parser_iVars.sName[1], &parser_iVars.vValue[1][0]);
    _parser.DefineVar(parser_iVars.sName[2], &parser_iVars.vValue[2][0]);
    _parser.DefineVar(parser_iVars.sName[3], &parser_iVars.vValue[3][0]);

    // --> Syntax fuer die Umrechnungsfunktionen definieren und die zugehoerigen Funktionen deklarieren <--
    _parser.DefinePostfixOprt(_nrT("'G"), parser_Giga);
    _parser.DefinePostfixOprt(_nrT("'M"), parser_Mega);
    _parser.DefinePostfixOprt(_nrT("'k"), parser_Kilo);
    _parser.DefinePostfixOprt(_nrT("'m"), parser_Milli);
    _parser.DefinePostfixOprt(_nrT("'mu"), parser_Micro);
    //_parser.DefinePostfixOprt(_nrT("µ"), parser_Micro);
    _parser.DefinePostfixOprt(_nrT("'n"), parser_Nano);
    _parser.DefinePostfixOprt(_nrT("~"), parser_Ignore);

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

    // --> Logisches NICHT <--
    _parser.DefineInfixOprt(_nrT("!"), parser_Not);

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
    _parser.DefineConst(_nrT("_m_amu"), 1.660538921E-27);
    _parser.DefineConst(_nrT("_r_bohr"), 5.2917721092E-11);
    _parser.DefineConst(_nrT("_G"), 6.67384E-11);
    _parser.DefineConst(_nrT("_coul_norm"), 8987551787.99791145324707);
    _parser.DefineConst(_nrT("nan"), NAN);
    _parser.DefineConst(_nrT("inf"), INFINITY);
    _parser.DefineConst(_nrT("void"), NAN);

    // --> Die Fakultaet und den Binomialkoeffzienten als mathemat. Funktion deklarieren <--
    _parser.DefineFun(_nrT("faculty"), parser_Faculty, false);                        // faculty(n)
    _parser.DefineFun(_nrT("dblfacul"), parser_doubleFaculty, false);                 // dblfacul(n)
    _parser.DefineFun(_nrT("binom"), parser_Binom, false);                            // binom(Wert1,Wert2)
    _parser.DefineFun(_nrT("num"), parser_Num, true);                                 // num(a,b,c,...)
    _parser.DefineFun(_nrT("cnt"), parser_Cnt, true);                                 // num(a,b,c,...)
    _parser.DefineFun(_nrT("std"), parser_Std, false);                                // std(a,b,c,...)
    _parser.DefineFun(_nrT("prd"), parser_product, false);                            // prd(a,b,c,...)
    _parser.DefineFun(_nrT("round"), parser_round, false);                            // round(x,n)
    _parser.DefineFun(_nrT("radian"), parser_toRadian, true);                         // radian(alpha)
    _parser.DefineFun(_nrT("degree"), parser_toDegree, true);                         // degree(x)
    _parser.DefineFun(_nrT("Y"), parser_SphericalHarmonics, true);                    // Y(l,m,theta,phi)
    _parser.DefineFun(_nrT("imY"), parser_imSphericalHarmonics, true);                // imY(l,m,theta,phi)
    _parser.DefineFun(_nrT("Z"), parser_Zernike, true);                               // Z(n,m,rho,phi)
    _parser.DefineFun(_nrT("sinc"), parser_SinusCardinalis, true);                    // sinc(x)
    _parser.DefineFun(_nrT("sbessel"), parser_SphericalBessel, true);                 // sbessel(n,x)
    _parser.DefineFun(_nrT("sneumann"), parser_SphericalNeumann, true);               // sneumann(n,x)
    _parser.DefineFun(_nrT("bessel"), parser_RegularCylBessel, true);                 // bessel(n,x)
    _parser.DefineFun(_nrT("neumann"), parser_IrregularCylBessel, true);              // neumann(n,x)
    _parser.DefineFun(_nrT("legendre"), parser_LegendrePolynomial, true);             // legendre(n,x)
    _parser.DefineFun(_nrT("legendre_a"), parser_AssociatedLegendrePolynomial, true); // legendre_a(l,m,x)
    _parser.DefineFun(_nrT("laguerre"), parser_LaguerrePolynomial, true);             // laguerre(n,x)
    _parser.DefineFun(_nrT("laguerre_a"), parser_AssociatedLaguerrePolynomial, true); // laguerre_a(n,k,x)
    _parser.DefineFun(_nrT("hermite"), parser_HermitePolynomial, true);               // hermite(n,x)
    _parser.DefineFun(_nrT("betheweizsaecker"), parser_BetheWeizsaecker, true);       // betheweizsaecker(N,Z)
    _parser.DefineFun(_nrT("heaviside"), parser_Heaviside, true);                     // heaviside(x)
    _parser.DefineFun(_nrT("phi"), parser_phi, true);                                 // phi(x,y)
    _parser.DefineFun(_nrT("theta"), parser_theta, true);                             // theta(x,y,z)
    _parser.DefineFun(_nrT("norm"), parser_Norm, true);                               // norm(x,y,z,...)
    _parser.DefineFun(_nrT("med"), parser_Med, true);                                 // med(x,y,z,...)
    _parser.DefineFun(_nrT("pct"), parser_Pct, true);                                 // pct(x,y,z,...)
    _parser.DefineFun(_nrT("and"), parser_and, true);                                 // and(x,y,z,...)
    _parser.DefineFun(_nrT("or"), parser_or, true);                                   // or(x,y,z,...)
    _parser.DefineFun(_nrT("rand"), parser_Random, false);                            // rand(left,right)
    _parser.DefineFun(_nrT("gauss"), parser_gRandom, false);                          // gauss(mean,std)
    _parser.DefineFun(_nrT("erf"), parser_erf, false);                                // erf(x)
    _parser.DefineFun(_nrT("erfc"), parser_erfc, false);                              // erfc(x)
    _parser.DefineFun(_nrT("gamma"), parser_gamma, false);                            // gamma(x)
    _parser.DefineFun(_nrT("cmp"), parser_compare, false);                            // cmp(crit,a,b,c,...,type)
    _parser.DefineFun(_nrT("is_string"), parser_is_string, false);                    // is_string(EXPR)
    _parser.DefineFun(_nrT("to_value"), parser_Ignore, false);                        // to_value(STRING)
    _parser.DefineFun(_nrT("time"), parser_time, false);                              // time()
    _parser.DefineFun(_nrT("version"), parser_numereversion, true);                   // version()
    _parser.DefineFun(_nrT("date"), parser_date, false);                              // date(TIME,TYPE)
    _parser.DefineFun(_nrT("is_nan"), parser_isnan, true);                            // is_nan(x)
    _parser.DefineFun(_nrT("range"), parser_interval, true);                          // range(x,left,right)
    _parser.DefineFun(_nrT("Ai"), parser_AiryA, true);                                // Ai(x)
    _parser.DefineFun(_nrT("Bi"), parser_AiryB, true);                                // Bi(x)
    _parser.DefineFun(_nrT("ellipticF"), parser_EllipticF, true);                     // ellipticF(x,k)
    _parser.DefineFun(_nrT("ellipticE"), parser_EllipticE, true);                     // ellipticE(x,k)
    _parser.DefineFun(_nrT("ellipticPi"), parser_EllipticP, true);                    // ellipticPi(x,n,k)
    _parser.DefineFun(_nrT("ellipticD"), parser_EllipticD, true);                     // ellipticD(x,n,k)
    _parser.DefineFun(_nrT("cot"), parser_cot, true);                                 // cot(x)
    _parser.DefineFun(_nrT("floor"), parser_floor, true);                             // floor(x)
    _parser.DefineFun(_nrT("roof"), parser_roof, true);                               // roof(x)
    _parser.DefineFun(_nrT("rect"), parser_rect, true);                               // rect(x,x0,x1)
    _parser.DefineFun(_nrT("student_t"), parser_studentFactor, true);                 // student_t(number,confidence)
    _parser.DefineFun(_nrT("gcd"), parser_gcd, true);                                 // gcd(x,y)
    _parser.DefineFun(_nrT("lcm"), parser_lcm, true);                                 // lcm(x,y)

    // --> Operatoren <--
    _parser.DefineOprt(_nrT("%"), parser_Mod, prMUL_DIV, oaLEFT, true);
    _parser.DefineOprt(_nrT("|||"), parser_XOR, prLOGIC, oaLEFT, true);
    _parser.DefineOprt(_nrT("|"), parser_BinOR, prLOGIC, oaLEFT, true);
    _parser.DefineOprt(_nrT("&"), parser_BinAND, prLOGIC, oaLEFT, true);

    // --> VAR-FACTORY Deklarieren (Irgendwo muessen die ganzen Variablen-Werte ja auch gespeichert werden) <--
    _parser.SetVarFactory(parser_AddVariable, &_parser);


}

void NumeReKernel::printVersionInfo()
{
    bWritingTable = true;
    make_hline(80);
    printPreFmt("| ");
    BI_splash();
    printPreFmt("                                  |\n");
	printPreFmt("| Version: " + sVersion + strfill("Build: ", 79-22-sVersion.length()) + AutoVersion::YEAR + "-" + AutoVersion::MONTH + "-" + AutoVersion::DATE + " |\n");
	printPreFmt("| Copyright (c) 2013-" + string(AutoVersion::YEAR) + toSystemCodePage(", Erik A. Hänel et al.") + strfill(toSystemCodePage(_lang.get("MAIN_ABOUT_NBR")), 79-48) + " |\n"); //toSystemCodePage("Über: siehe \"about\" |") << endl; //MAIN_ABOUT

	//printPreFmt("|-> Copyright (c) 2013-" + string(AutoVersion::YEAR) + toSystemCodePage(", Erik A. Hänel et al.") + strfill(toSystemCodePage(_lang.get("MAIN_ABOUT_NBR")), _option.getWindow()-50) + "\n"); //toSystemCodePage("Über: siehe \"about\" |") << endl; //MAIN_ABOUT
	//printPreFmt("|   Version: " + sVersion + strfill("Build: ", _option.getWindow()-24-sVersion.length()) + AutoVersion::YEAR + "-" + AutoVersion::MONTH + "-" + AutoVersion::DATE + "\n");
	make_hline(80);

	printPreFmt("|\n");
	//cerr << "|" << endl;

	if (_option.getbGreeting() && BI_FileExists(_option.getExePath()+"\\numere.ini"))
	{
        printPreFmt(toSystemCodePage(BI_Greeting(_option)) + "|\n"); /// Das hat einen Bug markiert: zwei mal "\n" bricht die Auswertung ab.
        //cerr << toSystemCodePage(BI_Greeting(_option));
        //cerr << "|" << endl;
    }
    print(LineBreak(_lang.get("PARSER_INTRO"), _option));;
    printPreFmt("|\n|<- ");
    flush();
    bWritingTable = false;

}

/// --> Main Loop <--
NumeReKernel::KernelStatus NumeReKernel::MainLoop(const string& sCommand)
{
    if (!m_parent)
        return NUMERE_ERROR;


    Indices _idx;

    bool bWriteToCache = false; // TRUE, wenn das/die errechneten Ergebnisse in den Cache geschrieben werden sollen

    string sLine_Temp = "";     // Temporaerer String fuer die Eingabe
    string sCache = "";         // Zwischenspeicher fuer die Cache-Koordinaten
    string sKeep = "";          // Zwei '\' am Ende einer Zeile ermoeglichen es, dass die Eingabe auf mehrere Zeilen verteilt wird.
                                // Die vorherige Zeile wird hierin zwischengespeichert
    string sCmdCache = "";
    string sLine = "";
    value_type* v = 0;          // Ergebnisarray
    int nNum = 0;               // Zahl der Ergebnisse in value_type* v

//print("DEBUG: "+sCommand);
    sCommandLine += sCommand;
    if (!sCommandLine.length())
        return NUMERE_PENDING;
//print("DEBUG: "+sCommandLine);
    while (sCommandLine.front() == ' ')
        sCommandLine.erase(0,1);
    if (!sCommandLine.length())
        return NUMERE_PENDING;

//print("DEBUG: "+sCommandLine);

    while (sCommandLine.back() == ' ')
        sCommandLine.pop_back();

//print("DEBUG: "+sCommandLine);
    if (sCommandLine.length() > 2 && sCommandLine.substr(sCommandLine.length()-2,2) == "\\\\")
    {
        sCommandLine.erase(sCommandLine.length()-2);
        return NUMERE_PENDING;
    }

    sLine = sCommandLine;
    sCommandLine.clear();
    _parser.ClearVectorVars();
    bSupressAnswer = false;


    if (_procedure.getPath() != _option.getProcsPath())
    {
        _procedure.setPath(_option.getProcsPath(), true, _procedure.getProgramPath());
        _option.setProcPath(_procedure.getPath());
    }
    // --> "try {...} catch() {...}" verwenden wir, um Parser-Exceptions abzufangen und korrekt auszuwerten <--
    do
    {
        bSupressAnswer = false;
        bWriteToCache = false;
        sCache = "";

        if (_parser.mVarMapPntr)
            _parser.mVarMapPntr = 0;
        //print("DEBUG: "+sLine);
        try
        {
            if (!sCmdCache.length())
            {
                if (_data.pausedOpening())
                {
                    _data.openFromCmdLine(_option, "", true);
                    if (_data.isValid())
                    {
                        print(LineBreak(_lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option, true, 4));
                        if (oLogFile.is_open())
                            oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Data out of " <<_data.getDataFileName("data") << " was successfully loaded." << endl;
                    }
                }
                if (_script.getAutoStart())
                {
                    print(LineBreak(_lang.get("PARSER_STARTINGSCRIPT", _script.getScriptFileName()), _option, true, 4));
                    if (oLogFile.is_open())
                        oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Starting Script " << _script.getScriptFileName() << endl;
                    _script.openScript();
                }

                _data.setCacheStatus(false);

                // --> Wenn gerade ein Script aktiv ist, lese dessen naechste Zeile, sonst nehme eine Zeile von std::cin <--
                if (_script.isValid() && _script.isOpen())
                {
                    sLine = _script.getNextScriptCommand();
                    sScriptFileName = _script.getScriptFileName();
                    nScriptLine = _script.getCurrentLine();
                }
                else if (_option.readCmdCache().length())
                {
                    if (oLogFile.is_open())
                        oLogFile << toString(time(0)-tTimeZero, true) << "> SYSTEM: Processing command line parameters:" << endl;
                    sLine = _option.readCmdCache(true);
                }
                // --> Leerzeichen und Tabulatoren entfernen <--
                StripSpaces(sLine);
                for (unsigned int i = 0; i < sLine.length(); i++)
                {
                    if (sLine[i] == '\t')
                        sLine[i] = ' ';
                }
                if (findCommand(sLine).sString != "help"
                    && findCommand(sLine).sString != "find"
                    && findCommand(sLine).sString != "search"
                    && (sLine.find('(') != string::npos || sLine.find('{') != string::npos))
                {
                    if (!validateParenthesisNumber(sLine))
                        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, sLine.find('('));
                }

                // --> Keine Laenge? Ignorieren! <--
                if (!sLine.length() || sLine[0] == '@')
                    continue;
                if (sLine.find("<helpindex>") != string::npos && sLine.find("</helpindex>") != string::npos)
                {
                    _procedure.addHelpIndex(sLine.substr(0,sLine.find("<<>>")),getArgAtPos(sLine, sLine.find("id=")+3));
                    sLine.erase(0,sLine.find("<<>>")+4);
                    _option.addToDocIndex(sLine, _option.getUseCustomLanguageFiles());
                    _plugin = _procedure;
                    continue;
                }

                // --> Kommando "global" entfernen <--
                if (findCommand(sLine).sString == "global")
                {
                    sLine = sLine.substr(findCommand(sLine).nPos+6);
                    StripSpaces(sLine);
                }
                // --> Wenn die Laenge groesser als 2 ist, koennen '\' am Ende sein <--
                if (sLine.length() > 2)
                {
                    if (sLine.substr(sLine.length()-2,2) == "\\\\")
                    {
                        // --> Ergaenze die Eingabe zu sKeep und beginne einen neuen Schleifendurchlauf <--
                        sKeep += sLine.substr(0,sLine.length()-2);
                        continue;
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

            }
            if ((sCmdCache.length() || sLine.find(';') != string::npos) && !_procedure.is_writing() && findCommand(sLine).sString != "procedure")
            {
                if (sCmdCache.length())
                {
                    while (sCmdCache.front() == ';' || sCmdCache.front() == ' ')
                        sCmdCache.erase(0,1);
                    if (!sCmdCache.length())
                        continue;
                    if (sCmdCache.find(';') != string::npos)
                    {
                        for (unsigned int i = 0; i < sCmdCache.length(); i++)
                        {
                            if (sCmdCache[i] == ';' && !isInQuotes(sCmdCache, i))
                            {
                                bSupressAnswer = true;
                                sLine = sCmdCache.substr(0,i);
                                sCmdCache.erase(0,i+1);
                                break;
                            }
                            if (i == sCmdCache.length()-1)
                            {
                                sLine = sCmdCache;
                                sCmdCache.clear();
                                break;
                            }
                        }
                    }
                    else
                    {
                        sLine = sCmdCache;
                        sCmdCache.clear();
                    }
                }
                else if (sLine.find(';') == sLine.length()-1)
                {
                    bSupressAnswer = true;
                    sLine.pop_back();
                }
                else
                {
                    for (unsigned int i = 0; i < sLine.length(); i++)
                    {
                        if (sLine[i] == ';' && !isInQuotes(sLine, i))
                        {
                            if (i != sLine.length()-1)
                                sCmdCache = sLine.substr(i+1);
                            sLine.erase(i);
                            bSupressAnswer = true;
                        }
                        if (i == sLine.length()-1)
                        {
                            break;
                        }
                    }
                }
            }

            // Eval debugger breakpoints from scripts
            if (sLine.substr(0,2) == "|>"
                && _script.isValid()
                && !_procedure.is_writing()
                && !_procedure.getLoop())
            {
                sLine.erase(0,2);
                if (_option.getUseDebugger())
                    evalDebuggerBreakPoint(_option, _data.getStringVars(), sLine, &_parser);
            }

            if (oLogFile.is_open())
                oLogFile << toString(time(0) - tTimeZero, true) << "> " << sLine << endl;
            if (GetAsyncCancelState() && _script.isValid() && _script.isOpen())
            {
                if (_option.getbUseESCinScripts())
                    throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
            }
            GetAsyncKeyState(VK_ESCAPE);
            if ((findCommand(sLine).sString == "compose"
                    || findCommand(sLine).sString == "endcompose"
                    || sPlotCompose.length())
                && !_procedure.is_writing()
                && findCommand(sLine).sString != "quit"
                && findCommand(sLine).sString != "help")
            {
                if (!sPlotCompose.length() && findCommand(sLine).sString == "compose")
                {
                    sPlotCompose = "plotcompose ";
                    if (matchParams(sLine, "multiplot", '='))
                    {
                        sPlotCompose += "-multiplot=" + getArgAtPos(sLine, matchParams(sLine, "multiplot",'=')+9) + " <<COMPOSE>> ";
                    }
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
                            printPreFmt(strfill("> ", 2*_procedure.getLoop(), '-'));
                        }
                        else if (_procedure.is_writing())
                        {
                            printPreFmt("|PROC> ");
                        }
                        else if (!_procedure.is_writing() && sPlotCompose.length())
                        {
                            printPreFmt("|COMP> ");
                        }
                        return NUMERE_PENDING_SPECIAL;
                    }
                    continue;
                }
                else if (findCommand(sLine).sString == "abort")
                {
                    sPlotCompose = "";
                    print(LineBreak(_lang.get("PARSER_ABORTED"), _option));
                    continue;
                }
                else if (findCommand(sLine).sString != "endcompose")
                {
                    string sCommand = findCommand(sLine).sString;
                    if (sCommand.substr(0,4) == "plot"
                        || sCommand.substr(0,7) == "subplot"
                        || sCommand.substr(0,4) == "grad"
                        || sCommand.substr(0,4) == "dens"
                        || sCommand.substr(0,4) == "draw"
                        || sCommand.substr(0,4) == "vect"
                        || sCommand.substr(0,4) == "cont"
                        || sCommand.substr(0,4) == "surf"
                        || sCommand.substr(0,4) == "mesh")
                    {
                        sPlotCompose += sLine + " <<COMPOSE>> ";
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
                                printPreFmt(strfill("> ", 2*_procedure.getLoop(), '-'));
                            }
                            else if (_procedure.is_writing())
                            {
                                printPreFmt("|PROC> ");
                            }
                            else if (!_procedure.is_writing() && sPlotCompose.length())
                            {
                                printPreFmt("|COMP> ");
                            }
                            return NUMERE_PENDING_SPECIAL;
                        }
                    }
                    continue;
                }
                else
                {
                    sLine = sPlotCompose;
                    sPlotCompose = "";
                }
            }

            if (_script.isValid() && _script.isOpen() && _script.installProcedures() && _script.getInstallInfoString().length())
            {
                if (matchParams(_script.getInstallInfoString(), "type", '='))
                {
                    if (getArgAtPos(_script.getInstallInfoString(), matchParams(_script.getInstallInfoString(), "type", '=')).find("TYPE_PLUGIN") != string::npos)
                    {
                        _procedure.declareNewPlugin(_script.getInstallInfoString());
                        _plugin = _procedure;
                    }
                }
            }
            if (findCommand(sLine).sString == "uninstall")
            {
                string sPlugin = fromSystemCodePage(getArgAtPos(sLine, findCommand(sLine).nPos+9));
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
                            _option.removeFromDocIndex(getNextArgument(sPlugin, true), _option.getUseCustomLanguageFiles());
                        }
                    }
                    print(LineBreak(_lang.get("PARSER_PLUGINDELETED"), _option));
                }
                else
                    print(LineBreak(_lang.get("PARSER_PLUGINNOTFOUND"), _option));
                return NUMERE_DONE_KEYWORD;
            }

            if (_procedure.is_writing() || findCommand(sLine).sString == "procedure")
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
                        printPreFmt(strfill("> ", 2*_procedure.getLoop(), '-'));
                    }
                    else if (_procedure.is_writing())
                    {
                        printPreFmt("|PROC> ");
                    }
                    else if (!_procedure.is_writing() && sPlotCompose.length())
                    {
                        printPreFmt("|COMP> ");
                    }
                    return NUMERE_PENDING_SPECIAL;
                }
                continue;
            }

            if (sLine.find("to_cmd(") != string::npos && !_procedure.getLoop())
            {
                unsigned int nPos = 0;
                while (sLine.find("to_cmd(", nPos) != string::npos)
                {
                    nPos = sLine.find("to_cmd(", nPos) + 6;
                    if (isInQuotes(sLine, nPos))
                        continue;
                    unsigned int nParPos = getMatchingParenthesis(sLine.substr(nPos));
                    if (nParPos == string::npos)
                        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);
                    string sCmdString = sLine.substr(nPos+1, nParPos-1);
                    StripSpaces(sCmdString);
                    if (containsStrings(sCmdString) || _data.containsStringVars(sCmdString))
                    {
                        sCmdString += " -nq";
                        parser_StringParser(sCmdString, sCache, _data, _parser, _option, true);
                        sCache = "";
                    }
                    sLine = sLine.substr(0, nPos-6) + sCmdString + sLine.substr(nPos + nParPos+1);
                    nPos -= 5;
                }
            }
            // --> Prozeduren abarbeiten <--
            if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) != string::npos && !_procedure.getLoop())
            {
                unsigned int nPos = 0;
                int nProc = 0;
                while (sLine.find('$', nPos) != string::npos && sLine.find('(', sLine.find('$', nPos)) != string::npos)
                {
                    unsigned int nParPos = 0;
                    nPos = sLine.find('$', nPos) + 1;
                    string __sName = sLine.substr(nPos, sLine.find('(', nPos)-nPos);
                    string __sVarList = "";
                    if (sLine[nPos] == '\'')
                    {
                        __sName = sLine.substr(nPos+1, sLine.find('\'', nPos+1)-nPos-1);
                        nParPos = sLine.find('(', nPos+1+__sName.length());
                    }
                    else
                        nParPos = sLine.find('(', nPos);
                    __sVarList = sLine.substr(nParPos);
                    nParPos += getMatchingParenthesis(sLine.substr(nParPos));
                    __sVarList = __sVarList.substr(1,getMatchingParenthesis(__sVarList)-1);

                    if (!isInQuotes(sLine, nPos, true))
                    {
                        Returnvalue _rTemp = _procedure.execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script);
                        if (!_procedure.getReturnType())
                            sLine = sLine.substr(0, nPos-1) + sLine.substr(nParPos+1);
                        else
                        {
                            _procedure.replaceReturnVal(sLine, _parser, _rTemp, nPos-1, nParPos+1, "PROC~["+__sName+"~ROOT_"+toString(nProc)+"]");
                            nProc++;
                        }
                    }
                    nPos += __sName.length() + __sVarList.length()+1;
                }
                StripSpaces(sLine);
                if (!sLine.length())
                    continue;
            }

            // --> Gibt es "??"? Dann rufe die Prompt-Funktion auf <--
            if (!_procedure.getLoop() && sLine.find("??") != string::npos && sLine.substr(0,4) != "help")
                sLine = parser_Prompt(sLine);

            if (_procedure.isPluginCmd(sLine) && !_procedure.getLoop())
            {
                if (_procedure.evalPluginCmd(sLine))
                {
                    _option.setSystemPrintStatus(false);
                    Returnvalue _rTemp = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script);
                    if (_rTemp.vStringVal.size() && sLine.find("<<RETURNVAL>>") != string::npos)
                    {
                        string sReturn = "{";
                        for (unsigned int v = 0; v < _rTemp.vStringVal.size(); v++)
                            sReturn += _rTemp.vStringVal[v]+",";
                        sReturn.back() = '}';
                        sLine.replace(sLine.find("<<RETURNVAL>>"), 13, sReturn);
                    }
                    else if (!_rTemp.vStringVal.size() && sLine.find("<<RETURNVAL>>") != string::npos)
                    {
                        sLine.replace(sLine.find("<<RETURNVAL>>"), 13, "_~PLUGIN["+_procedure.getPluginProcName()+"~ROOT]");
                        vAns = _rTemp.vNumVal[0];
                        _parser.SetVectorVar("_~PLUGIN["+_procedure.getPluginProcName()+"~ROOT]", _rTemp.vNumVal);
                    }
                    _option.setSystemPrintStatus(true);
                    if (!sLine.length())
                        continue;
                }
                else
                {
                    continue;
                }
            }
            if (findCommand(sLine, "explicit").sString == "explicit")
            {
                sLine.erase(findCommand(sLine, "explicit").nPos,8);
                StripSpaces(sLine);
            }
            /* --> Die Keyword-Suche soll nur funktionieren, wenn keine Schleife eingegeben wird, oder wenn eine
             *     eine Schleife eingegeben wird, dann nur in den wenigen Spezialfaellen, die zum Nachschlagen
             *     eines Keywords noetig sind ("list", "help", "find", etc.) <--
             */
            if (!_procedure.getLoop()
                || sLine.substr(0,4) == "help"
                || sLine.substr(0,3) == "man"
                || sLine.substr(0,4) == "quit"
                || sLine.substr(0,4) == "list"
                || sLine.substr(0,4) == "find"
                || sLine.substr(0,6) == "search")
            {
                //print("Debug: Keywords");
                switch (BI_CheckKeyword(sLine, _data, _out, _option, _parser, _functions, _pData, _script, true))
                {
                    case  0: break; // Kein Keyword: Mit dem Parser auswerten
                    case  1:        // Keyword: Naechster Schleifendurchlauf!
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
                    case -1:
                        // --> Sind ungesicherte Daten im Cache? Dann moechte der Nutzer diese vielleicht speichern <--
                        if (!_data.getSaveStatus()) // MAIN_UNSAVED_CACHE
                        {
                            string c = "";
                            print(LineBreak(_lang.get("MAIN_UNSAVED_CACHE"), _option));
                            printPreFmt("|\n|<- ");
                            NumeReKernel::getline(c);
                            if (c == _lang.YES())
                            {
                                _data.saveCache(); // MAIN_CACHE_SAVED
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
            // --> Wenn die call()-Methode FALSE zurueckgibt, ist etwas schief gelaufen! <--
            if (!_functions.call(sLine, _option))
                throw SyntaxError(SyntaxError::FUNCTION_ERROR, sLine, SyntaxError::invalid_position);
            // --> Prozeduren abarbeiten <--
            if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) != string::npos && !_procedure.getLoop())
            {
                unsigned int nPos = 0;
                int nProc = 0;
                while (sLine.find('$', nPos) != string::npos && sLine.find('(', sLine.find('$', nPos)) != string::npos)
                {
                    unsigned int nParPos = 0;
                    nPos = sLine.find('$', nPos) + 1;
                    string __sName = sLine.substr(nPos, sLine.find('(', nPos)-nPos);
                    nParPos = sLine.find('(', nPos);
                    nParPos += getMatchingParenthesis(sLine.substr(nParPos));
                    string __sVarList = sLine.substr(sLine.find('(',nPos));
                    __sVarList = __sVarList.substr(+1,getMatchingParenthesis(__sVarList)-1);

                    if (!isInQuotes(sLine, nPos))
                    {
                        Returnvalue _rTemp = _procedure.execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script);
                        if (!_procedure.getReturnType())
                            sLine = sLine.substr(0, nPos-1) + sLine.substr(nParPos+1);
                        else
                        {
                            _procedure.replaceReturnVal(sLine, _parser, _rTemp, nPos-1, nParPos+1, "PROC~["+__sName+"~ROOT_"+toString(nProc)+"]");
                            nProc++;
                        }
                    }
                    nPos += __sName.length() + __sVarList.length()+1;

                }
                StripSpaces(sLine);
                if (!sLine.length())
                    continue;
            }
            else if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) == string::npos)
            {
                size_t i = sLine.find('$');
                bool isnotinquotes = true;
                while (isInQuotes(sLine, i))
                {
                    if (sLine.find('$', i+1) != string::npos)
                    {
                        i = sLine.find('$', i+1);
                    }
                    else
                    {
                        isnotinquotes = false;
                        break;
                    }
                }
                if (isnotinquotes)
                {
                    sLine = "";
                    continue;
                }
            }
            // --> Nochmals ueberzaehlige Leerzeichen entfernen <--
            StripSpaces(sLine);
            if (!_procedure.getLoop())
            {
                // this is obviously a time consuming task => to be investigated
                evalRecursiveExpressions(sLine);
            }
            /*if (_option.getbDebug())
                cerr << "|-> DEBUG: sLine = " << sLine << endl;*/

            // --> Befinden wir uns in einem Loop? Dann ist nLoop > -1! <--
            if (_procedure.getLoop() || sLine.substr(0,3) == "for" || sLine.substr(0,2) == "if" || sLine.substr(0,5) == "while")
            {
                // --> Die Zeile in den Ausdrucksspeicher schreiben, damit sie spaeter wiederholt aufgerufen werden kann <--
                _procedure.setCommand(sLine, _parser, _data, _functions, _option, _out, _pData, _script);
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
                        printPreFmt(strfill("> ", 2*_procedure.getLoop(), '-'));
                    }
                    else if (_procedure.is_writing())
                    {
                        printPreFmt("|PROC> ");
                    }
                    else if (!_procedure.is_writing() && sPlotCompose.length() )
                    {
                        printPreFmt("|COMP> ");
                    }
                    else
                    {
                        if (_script.wasLastCommand())
                        {
                            print(LineBreak(_lang.get("PARSER_SCRIPT_FINISHED", _script.getScriptFileName()), _option, true, 4));
                            _data.setPluginCommands(_procedure.getPluginNames());
                        }
                        bCancelSignal = false;
                        return NUMERE_DONE_KEYWORD;
                    }
                    return NUMERE_PENDING_SPECIAL;
                }
                continue;
            }
            // --> Gibt es "??" ggf. nochmal? Dann rufe die Prompt-Funktion auf <--
            if (sLine.find("??") != string::npos)
                sLine = parser_Prompt(sLine);
            // --> Gibt es "data(" oder "cache("? Dann rufe die GetDataElement-Methode auf <--
            if (!containsStrings(sLine)
                && !_data.containsStringVars(sLine)
                && (sLine.find("data(") != string::npos || _data.containsCacheElements(sLine)))
            {
                //cerr << "get data element (parser)" << endl;
                sCache = parser_GetDataElement(sLine, _parser, _data, _option);
                if (sCache.length() && sCache.find('#') == string::npos)
                    bWriteToCache = true;
            }
            // --> Workaround fuer den x = x+1-Bug: In diesem Fall sollte die Eingabe x := x+1 lauten und wird hier weiterverarbeitet <--
            while (sLine.find(":=") != string::npos)
            {
                sLine.erase(sLine.find(":="),1);
            }
            // --> String-Syntax ("String" oder #VAR)? String-Parser aufrufen und mit dem naechsten Schleifendurchlauf fortfahren <--
            if (containsStrings(sLine) || _data.containsStringVars(sLine))
            {
                int nReturn = parser_StringParser(sLine, sCache, _data, _parser, _option);
                if (nReturn)
                {
                    if (nReturn == 1)
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
                            return NUMERE_DONE_KEYWORD;
                        }
                        else
                            continue;
                    }
                    if (sCache.length() && _data.containsCacheElements(sCache) && !bWriteToCache)
                        bWriteToCache = true;
                }
                else
                {
                    throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
                }
            }

            // --> Moeglicherweise erscheint nun "{{". Dies muss ersetzt werden <--
            if (sLine.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
            {
                parser_VectorToExpr(sLine, _option);
            }


            // --> Wenn die Ergebnisse in den Cache geschrieben werden sollen, bestimme hier die entsprechenden Koordinaten <--
            if (bWriteToCache)
            {
                // Get the indices from the corresponding function
                _idx = parser_getIndices(sCache, _parser, _data, _option);

                if ((_idx.nI[0] < 0 || _idx.nJ[0] < 0) && !_idx.vI.size() && !_idx.vJ.size())
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "");
                if ((_idx.nI[1] == -2 && _idx.nJ[1] == -2))
                    throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");

                if (_idx.nI[1] == -1)
                    _idx.nI[1] = _idx.nI[0];
                if (_idx.nJ[1] == -1)
                    _idx.nJ[1] = _idx.nJ[0];
                sCache.erase(sCache.find('('));
                StripSpaces(sCache);
            }

            // --> Ausdruck an den Parser uebergeben und einmal auswerten <--
            if (sLine + " " != _parser.GetExpr())
                _parser.SetExpr(sLine);


            // --> Jetzt weiss der Parser, wie viele Ergebnisse er berechnen muss <--
            v = _parser.Eval(nNum);
            if (nNum > 1)
            {
                //value_type *v = _parser.Eval(nNum);
                vAns = v[0];
                if (!bSupressAnswer)
                {
                    int nLineBreak = parser_LineBreak(_option);
                    string sAns = "ans = {";
                    for (int i = 0; i < nNum; ++i)
                    {
                        sAns += strfill(toString(v[i], _option), _option.getPrecision()+7);
                        if (i < nNum-1)
                            sAns += ", ";
                        if (nNum + 1 > nLineBreak && !((i+1) % nLineBreak) && i < nNum-1)
                            sAns += "...\n|          ";
                    }
                    sAns += "}";
                    printResult(sAns, sCmdCache, _script.isValid() && _script.isOpen());
                }
            }
            else
            {
                vAns = v[0];
                if (!bSupressAnswer)
                    printResult("ans = "+ toString(vAns, _option), sCmdCache, _script.isOpen() && _script.isValid());
            }
            if (bWriteToCache)
                _data.writeToCache(_idx, sCache, v, nNum);
        }
        catch (mu::Parser::exception_type &e)
        {
            _option.setSystemPrintStatus(true);
            // --> Vernuenftig formatierte Fehlermeldungen <--
            unsigned int nErrorPos = (int)e.GetPos();
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_MUP_HEAD")));
            make_hline();
            showDebugError(_lang.get("ERR_MUP_HEAD_DBG"));

            // --> Eigentliche Fehlermeldung <--
            print(LineBreak(e.GetMsg(), _option));
            print(LineBreak(_lang.get("ERR_EXPRESSION", maskProcedureSigns(e.GetExpr())), _option, true, 4, 15));
            //cerr << "|   Ausdruck:  " << LineBreak("\"" + e.GetExpr() + "\"", _option, true, 15, 15) << endl;

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
            if (e.GetExpr().length() > 63 && nErrorPos > 31 && nErrorPos < e.GetExpr().length()-32)
            {
                printPreFmt("|   Position:  \"..."+e.GetExpr().substr(nErrorPos-29,57) + "...\"\n");
                printPreFmt(pointToError(32));
            }
            else if (nErrorPos < 32)
            {
                string sErrorExpr = "|   Position:  \"";
                if (e.GetExpr().length() > 63)
                    sErrorExpr += e.GetExpr().substr(0,60) + "...\"";
                else
                    sErrorExpr += e.GetExpr() + "\"";
                printPreFmt(sErrorExpr+"\n");
                printPreFmt(pointToError(nErrorPos+1));
            }
            else if (nErrorPos > e.GetExpr().length()-32)
            {
                string sErrorExpr = "|   Position:  \"";
                if (e.GetExpr().length() > 63)
                {
                    printPreFmt(sErrorExpr + "..." + e.GetExpr().substr(e.GetExpr().length()-60) + "\"\n");
                    printPreFmt(pointToError(65-(e.GetExpr().length()-nErrorPos)-2));
                }
                else
                {
                    printPreFmt(sErrorExpr + e.GetExpr() + "\"\n");
                    printPreFmt(pointToError(nErrorPos));
                }
            }

            // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
            if (_script.isValid() && _script.isOpen())
            {
                print(LineBreak(_lang.get("ERR_SCRIPTCATCH", toString((int)_script.getCurrentLine())), _option));
                // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                _script.close();
            }

            make_hline();

            // --> Alle Variablen zuerst zuruecksetzen! <--
            _procedure.reset();
            _pData.setFileName("");
            if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> " << toUpperCase(_lang.get("ERR_ERROR")) << ": " << e.GetMsg() << endl;
            if (sCmdCache.length())
                sCmdCache.clear();
            _parser.DeactivateLoopMode();
            sCommandLine.clear();
            bCancelSignal = false;
            sendErrorNotification();
            return NUMERE_ERROR;
        }
        catch (const std::bad_alloc &e)
        {
            _option.setSystemPrintStatus(true);
            /* --> Das ist die schlimmste aller Exceptions: Fehler bei der Speicherallozierung.
             *     Leider gibt es bis dato keine Moeglichkeit, diesen wieder zu beheben, also bleibt
             *     vorerst nichts anderes uebrig, als NumeRe mit terminate() abzuschiessen <--
             */
            ///cerr << endl;
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_STD_BA_HEAD")));
            make_hline();
            print(LineBreak(_lang.get("ERR_STD_BADALLOC", sVersion), _option));
            make_hline();
            if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> ERROR: CRITICAL ACCESS VIOLATION" << endl;
            /**for (int i = 4; i > 0; i--)
            {
                cerr << "\r|-> TERMINATING IN " << i << " sec ...";
                Sleep(1000);
            }*/
            if (sCmdCache.length())
                sCmdCache.clear();
            sCommandLine.clear();
            bCancelSignal = false;
            sendErrorNotification();
            return NUMERE_ERROR;
        }
        catch (const std::exception &e)
        {
            _option.setSystemPrintStatus(true);
            // --> Alle anderen Standard-Exceptions <--
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_STD_INTERNAL_HEAD")));
            make_hline();
            showDebugError(_lang.get("ERR_STD_INTERNAL_HEAD_DBG"));
            print(LineBreak(string(e.what()), _option));
            print(LineBreak(_lang.get("ERR_STD_INTERNAL"), _option));

            // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
            if (_script.isValid() && _script.isOpen())
            {
                print(LineBreak(_lang.get("ERR_SCRIPTCATCH", toString((int)_script.getCurrentLine())), _option));
                // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                _script.close();
            }
            _pData.setFileName("");
            make_hline();
            if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> " << toUpperCase(_lang.get("ERR_ERROR")) << ": " << e.what() << endl;
            if (sCmdCache.length())
                sCmdCache.clear();
            _parser.DeactivateLoopMode();
            sCommandLine.clear();
            bCancelSignal = false;
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
                if (oLogFile.is_open())
                    oLogFile << toString(time(0) - tTimeZero, true) << "> NOTE: Process was cancelled by user" << endl;
                // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
                if (_script.isValid() && _script.isOpen())
                {
                    print(LineBreak(_lang.get("ERR_SCRIPTABORT", toString((int)_script.getCurrentLine())), _option));
                    // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                    _script.close();
                }
                if (_option.getUseDebugger())
                    _option._debug.reset();
            }
            else
            {
                print(toUpperCase(_lang.get("ERR_NR_HEAD")));
                make_hline();
                showDebugError(_lang.get("ERR_NR_HEAD_DBG"));

                if (e.getToken().length() && (e.errorcode == SyntaxError::PROCEDURE_THROW || e.errorcode == SyntaxError::LOOP_THROW))
                {
                    print(LineBreak(e.getToken(), _option));
                    if (oLogFile.is_open())
                        oLogFile << toString(time(0)-tTimeZero, true) << "> " << toUpperCase(_lang.get("ERR_ERROR")) << ": " << e.getToken() << endl;
                }
                else
                {
                    string sErrLine_0 = _lang.get("ERR_NR_"+toString((int)e.errorcode)+"_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3]));
                    string sErrLine_1 = _lang.get("ERR_NR_"+toString((int)e.errorcode)+"_1_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3]));
                    string sErrIDString = _lang.getKey("ERR_NR_"+toString((int)e.errorcode)+"_0_*");

                    if (sErrLine_0.substr(0,7) == "ERR_NR_")
                    {
                        sErrLine_0 = _lang.get("ERR_GENERIC_0");
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
                            if (e.getExpr().length() > 63 && e.getPosition() > 31 && e.getPosition() < e.getExpr().length()-32)
                            {
                                printPreFmt("|   Position:  \"..."+e.getExpr().substr(e.getPosition()-29,57) + "...\"\n");
                                printPreFmt(pointToError(32));
                            }
                            else if (e.getPosition() < 32)
                            {
                                string sErrorExpr = "|   Position:  \"";
                                if (e.getExpr().length() > 63)
                                    sErrorExpr += e.getExpr().substr(0,60) + "...\"";
                                else
                                    sErrorExpr += e.getExpr() + "\"";
                                printPreFmt(sErrorExpr+"\n");
                                printPreFmt(pointToError(e.getPosition()+1));
                            }
                            else if (e.getPosition() > e.getExpr().length()-32)
                            {
                                string sErrorExpr = "|   Position:  \"";
                                if (e.getExpr().length() > 63)
                                {
                                    printPreFmt(sErrorExpr + "..." + e.getExpr().substr(e.getExpr().length()-60) + "\"\n");
                                    printPreFmt(pointToError(65-(e.getExpr().length()-e.getPosition())-2));
                                }
                                else
                                {
                                    printPreFmt(sErrorExpr + e.getExpr() + "\"\n");
                                    printPreFmt(pointToError(e.getPosition()));
                                }
                            }
                        }
                    }
                    if (oLogFile.is_open())
                        oLogFile << toString(time(0)-tTimeZero, true) << "> " << toUpperCase(_lang.get("ERR_ERROR")) << ": " << sErrIDString << endl;
                }
                // --> Wenn ein Script ausgefuehrt wird, lesen wir den Index der letzten eingelesenen Zeile und geben diesen hier aus <--
                if (_script.isValid() && _script.isOpen())
                {
                    print(LineBreak(_lang.get("ERR_SCRIPTCATCH", toString((int)_script.getCurrentLine())), _option));
                    // --> Script beenden! Mit einem Fehler ist es unsinnig weiterzurechnen <--
                    _script.close();
                }
            }
            _pData.setFileName("");
            make_hline();
            //sErrorToken = "";
            //nErrorIndices[0] = -1;
            //nErrorIndices[1] = -1;
            if (sCmdCache.length())
                sCmdCache.clear();
            _parser.DeactivateLoopMode();
            sCommandLine.clear();
            bCancelSignal = false;
            sendErrorNotification();
            return NUMERE_ERROR;
        }
        catch (...)
        {
            /* --> Allgemeine Exception abfangen, die nicht durch mu::exception_type oder std::exception
             *     abgedeckt wird <--
             */
            sendErrorNotification();
            make_hline();
            print(toUpperCase(_lang.get("ERR_CATCHALL_HEAD")));
            make_hline();
            print(LineBreak(_lang.get("ERR_CATCHALL"), _option));
            make_hline();
            if (oLogFile.is_open())
                oLogFile << toString(time(0)-tTimeZero, true) << "> ERROR: UNKNOWN EXCEPTION" << endl;
            _pData.setFileName("");
            //cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (sCmdCache.length())
                sCmdCache.clear();
            _parser.DeactivateLoopMode();
            sCommandLine.clear();
            bCancelSignal = false;
            sendErrorNotification();
            return NUMERE_ERROR;
        }
        //print(toString(vAns,7));
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

void NumeReKernel::updateLineLenght(int nLength)
{
    if (nLength > 0)
    {
        nLINE_LENGTH = nLength;
        _option.setWindowSize(nLength);
    }
}

void NumeReKernel::saveData()
{
    if (!_data.getSaveStatus()) // MAIN_UNSAVED_CACHE
    {
        _data.saveCache(); // MAIN_CACHE_SAVED
        print(LineBreak(_lang.get("MAIN_CACHE_SAVED"), _option));
        Sleep(500);
    }

}

void NumeReKernel::CloseSession()
{
    saveData();
    _data.clearCache();
    _data.removeData(false); // MAIN_BYE

    // --> Konfiguration aus den Objekten zusammenfassen und anschliessend speichern <--
	_option.setSavePath(_out.getPath());
	_option.setLoadPath(_data.getPath());
	_option.setPlotOutputPath(_pData.getPath());
	_option.setScriptPath(_script.getPath());
	if (_option.getbDefineAutoLoad() && _functions.getDefinedFunctions())
	{
        _option.setSystemPrintStatus(false);
        _functions.save(_option);
        Sleep(100);
	}
	_option.save(_option.getExePath()); // MAIN_QUIT
	if (oLogFile.is_open())
	{
        oLogFile << "--- NUMERE WAS TERMINATED SUCCESSFULLY ---" << endl << endl << endl;
        oLogFile.close();
	}


    //Do some clean-up stuff here
    sCommandLine.clear();
    sAnswer.clear();
    m_parent = nullptr;
}

string NumeReKernel::ReadAnswer()
{
    string sAns = sAnswer;
    sAnswer.clear();
    return sAns;
}

map<string,string> NumeReKernel::getPluginLanguageStrings()
{
    map<string,string> mPluginLangStrings;
    for (size_t i = 0; i < _procedure.getPluginCount(); i++)
    {
        string sDesc = _procedure.getPluginCommand(i)+ "     - " + _procedure.getPluginDesc(i);
        while (sDesc.find("\\\"") != string::npos)
            sDesc.erase(sDesc.find("\\\""), 1);

        mPluginLangStrings["PARSERFUNCS_LISTCMD_CMD_"+toUpperCase(_procedure.getPluginCommand(i))+"_[PLUGINS]"] = sDesc;
    }
    return mPluginLangStrings;
}

map<string,string> NumeReKernel::getFunctionLanguageStrings()
{
    map<string,string> mFunctionLangStrings;
    for (size_t i = 0; i < _functions.getDefinedFunctions(); i++)
    {
        string sDesc = _functions.getFunction(i)+ "     ARG   - " + _functions.getComment(i);
        while (sDesc.find("\\\"") != string::npos)
            sDesc.erase(sDesc.find("\\\""), 1);

        mFunctionLangStrings["PARSERFUNCS_LISTFUNC_FUNC_"+toUpperCase(_functions.getFunction(i).substr(0,_functions.getFunction(i).rfind('(')))+"_[DEFINE]"] = sDesc;
    }
    return mFunctionLangStrings;
}

vector<string> NumeReKernel::getPluginCommands()
{
    vector<string> vPluginCommands;

    for (size_t i = 0; i < _procedure.getPluginCount(); i++)
        vPluginCommands.push_back(_procedure.getPluginCommand(i));
    return vPluginCommands;
}

string NumeReKernel::ReadFileName()
{
    string sFile = sFileToEdit;
    sFileToEdit.clear();
    return sFile;
}

unsigned int NumeReKernel::ReadLineNumber()
{
    unsigned int nLine = nLineToGoTo;
    nLineToGoTo = 0;
    return nLine;
}

int NumeReKernel::ReadOpenFileFlag()
{
    int nFlag = nOpenFileFlag;
    nOpenFileFlag = 0;
    return nFlag;
}

string NumeReKernel::ReadDoc()
{
    string Doc = sDocumentation;
    sDocumentation.clear();
    return Doc;
}

string NumeReKernel::getDocumentation(const string& sCommand)
{
    return doc_HelpAsHTML(sCommand, false, _option);
}

bool NumeReKernel::SettingsModified()
{
    bool modified = modifiedSettings;
    modifiedSettings = false;
    return modified;
}

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
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_CALC_UPDATE;
        }
        wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
        Sleep(5);
    }
    else
        sAnswer = sLine;
}

string NumeReKernel::maskProcedureSigns(string sLine)
{
    for (size_t i = 0; i < sLine.length(); i++)
    {
        if (sLine[i] == '$' && (!i || sLine[i-1] != '\\'))
            sLine.insert(i,1, '\\');
    }
    return sLine;
}


// This is the virtual cout function. The port from the kernel of course needs some tweaking
void NumeReKernel::print(const string& __sLine)
{
    if (!m_parent)
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
        }
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        m_parent->m_sAnswer += "|-> " + sLine + "\n";
        if (m_parent->m_KernelStatus != NumeReKernel::NUMERE_PRINTLINE && m_parent->m_KernelStatus != NumeReKernel::NUMERE_CALC_UPDATE)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_PRINTLINE;
    }
    if (bWritingTable)
        return;
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(5);
}

// This is the virtual cout function. The port from the kernel of course needs some tweaking
void NumeReKernel::printPreFmt(const string& __sLine)
{
    if (!m_parent)
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
        }

        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        m_parent->m_sAnswer += sLine;
        if (m_parent->m_KernelStatus != NumeReKernel::NUMERE_PRINTLINE_PREFMT)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_PRINTLINE_PREFMT;
    }
    if (bWritingTable)
        return;
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(5);
}


void NumeReKernel::sendErrorNotification()
{
    bErrorNotification = !bErrorNotification;
}

// This shall replace the corresponding function from "tools.hpp"

void NumeReKernel::statusBar(int nStep, int nFirstStep, int nFinalStep, const string& sType)
{
    //cerr << nStep << endl << nFirstStep << endl << nFinalStep << endl << sType << endl;
    int nStatusVal = 0;
    if (abs(nFinalStep-nFirstStep) < 9999
        && abs((nStep-nFirstStep) / (double)(nFinalStep-nFirstStep) * 20)
            > abs((nStep-1-nFirstStep) / (double)(nFinalStep-nFirstStep) * 20))
    {
        nStatusVal = abs((int)((nStep-nFirstStep) / (double)(nFinalStep-nFirstStep) * 20)) * 5;
    }
    else if (abs(nFinalStep-nFirstStep) >= 9999
        && abs((nStep-nFirstStep) / (double)(nFinalStep-nFirstStep) * 100)
            > abs((nStep-1-nFirstStep) / (double)(nFinalStep-nFirstStep) * 100))
    {
        nStatusVal = abs((int)((nStep-nFirstStep) / (double)(nFinalStep-nFirstStep) * 100));
    }
    if (nLastStatusVal >= 0 && nLastStatusVal == nStatusVal && (sType != "cancel" && sType != "bcancel"))
        return;
    toggleTableStatus();
    if (nLastLineLength > 0)
        printPreFmt("\r" + strfill(" ", nLastLineLength));
    if (sType == "std")
    {
        printPreFmt("\r|-> " + _lang.get("COMMON_EVALUATING") + " ... " + toString(nStatusVal) + " %");
        nLastLineLength = 14+_lang.get("COMMON_EVALUATING").length();
    }
    else if (sType == "cancel")
    {
        printPreFmt("\r|-> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL"));
        nStep = nFinalStep;
    }
    else if (sType == "bar")
    {
        printPreFmt("\r|-> [" + strfill("#", (int)(nStatusVal/5.0), '#') + strfill(" ", 20-(int)(nStatusVal/5.0)) + "] (" + toString(nStatusVal) + " %)");
        nLastLineLength = 34;
    }
    else if (sType == "bcancel")
    {
        printPreFmt("\r|-> [" + strfill("#", (int)(nLastStatusVal/5.0), '#') + strfill(" ", 20-(int)(nLastStatusVal/5.0)) + "] (--- %)");
        nFinalStep = nStep;
    }
    else
    {
        nLastLineLength = 1;
        printPreFmt("\r|");
        for (unsigned int i = 0; i < sType.length(); i++)
        {
            if (sType.substr(i,5) == "<bar>")
            {
                printPreFmt("[" + strfill("#", (int)(nStatusVal/5.0), '#') + strfill(" ", 20-(int)(nStatusVal/5.0)) + "]");
                i += 4;
                nLastLineLength += 22;
                continue;
            }
            if (sType.substr(i,5) == "<Bar>")
            {
                printPreFmt("[" + strfill("#", (int)(nStatusVal/5.0), '#') + strfill("-", 20-(int)(nStatusVal/5.0), '-') + "]");
                i += 4;
                nLastLineLength += 22;
                continue;
            }
            if (sType.substr(i,5) == "<BAR>")
            {
                printPreFmt("[" + strfill("#", (int)(nStatusVal/5.0), '#') + strfill("=", 20-(int)(nStatusVal/5.0), '=') + "]");
                i += 4;
                nLastLineLength += 22;
                continue;
            }
            if (sType.substr(i,5) == "<val>")
            {
                printPreFmt(toString(nStatusVal));
                i += 4;
                nLastLineLength += 3;
                continue;
            }
            if (sType.substr(i,5) == "<Val>")
            {
                printPreFmt(strfill(toString(nStatusVal), 3));
                i += 4;
                nLastLineLength += 3;
                continue;
            }
            if (sType.substr(i,5) == "<VAL>")
            {
                printPreFmt(strfill(toString(nStatusVal), 3, '0'));
                i += 4;
                nLastLineLength += 3;
                continue;
            }
            printPreFmt(sType.substr(i,1));
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
    return;
}

void NumeReKernel::gotoLine(const string& sFile, unsigned int nLine)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        sFileToEdit = sFile;
        if (nLine)
            nLineToGoTo = nLine-1;
        m_parent->m_KernelStatus = NUMERE_EDIT_FILE;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(100);
}

void NumeReKernel::setDocumentation(const string& _sDocumentation)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        sDocumentation = _sDocumentation;
        m_parent->m_KernelStatus = NUMERE_OPEN_DOC;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(100);
}

void NumeReKernel::showTable(string** __stable, size_t cols, size_t lines, string __name, bool openeditable)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        sTable.clear();
        for (size_t i = 0; i < lines; i++)
        {
            sTable.push_back(vector<string>(cols, ""));
            for (size_t j = 0; j < cols; j++)
            {
                sTable[i][j] = __stable[i][j];
            }
        }
        sTableName = __name;
        if (openeditable)
            m_parent->m_KernelStatus = NUMERE_EDIT_TABLE;
        else
            m_parent->m_KernelStatus = NUMERE_SHOW_TABLE;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(100);
}

stringmatrix NumeReKernel::getTable()
{
    if (!m_parent)
        return stringmatrix();
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
        return stringmatrix();
    return sTable;
}

void NumeReKernel::showDebugError(const string& sTitle)
{
    if (_option.getUseDebugger() && _option._debug.validDebuggingInformations())
    {
        showDebugEvent(sTitle, _option._debug.getModuleInformations(), _option._debug.getStackTrace(), _option._debug.getNumVars(), _option._debug.getStringVars());
        gotoLine(_option._debug.getErrorModule(), _option._debug.getLineNumber());
    }
    _option._debug.reset();
}

void NumeReKernel::showDebugEvent(const string& sTitle, const vector<string>& vModule, const vector<string>& vStacktrace, const vector<string>& vNumVars, const vector<string>& vStringVars)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        vDebugInfos.clear();

        // note the size of the fields
        vDebugInfos.push_back(toString(vModule.size())+";"+toString(vStacktrace.size())+";"+toString(vNumVars.size())+";");
        vDebugInfos.push_back(sTitle);

        vDebugInfos.insert(vDebugInfos.end(), vModule.begin(), vModule.end());
        vDebugInfos.insert(vDebugInfos.end(), vStacktrace.begin(), vStacktrace.end());
        if (vNumVars.size())
            vDebugInfos.insert(vDebugInfos.end(), vNumVars.begin(), vNumVars.end());
        if (vStringVars.size())
            vDebugInfos.insert(vDebugInfos.end(), vStringVars.begin(), vStringVars.end());

        m_parent->m_KernelStatus = NUMERE_DEBUG_EVENT;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(100);
}

void NumeReKernel::waitForContinue()
{
    if (!m_parent)
        return;
    bool bContinue = false;
    do
    {
        Sleep(100);
        {
            wxCriticalSectionLocker lock(m_parent->m_kernelCS);
            bContinue = m_parent->m_bContinueDebug;
            m_parent->m_bContinueDebug = false;
        }
    }
    while (!bContinue);

    return;
}

void NumeReKernel::evalDebuggerBreakPoint(Settings& _option, const map<string,string>& sStringMap, const string& sCurrentCommand, Parser* _parser)
{
    mu::varmap_type varmap;
    string** sLocalVars = nullptr;
    double* dLocalVars = nullptr;
    size_t nLocalVarMapSize = 0;
    size_t nLocalVarMapSkip = 0;
    string** sLocalStrings = nullptr;
    size_t nLocalStringMapSize = 0;
    if (_parser)
    {
        varmap = _parser->GetVar();
        nLocalVarMapSize = varmap.size();
        sLocalVars = new string*[nLocalVarMapSize];
        dLocalVars = new double[nLocalVarMapSize];
        size_t i = 0;
        for (auto iter = varmap.begin(); iter != varmap.end(); ++iter)
        {
            sLocalVars[i+nLocalVarMapSkip] = new string[2];
            if ((iter->first).substr(0,2) == "_~")
            {
                nLocalVarMapSkip++;
                continue;
            }
            sLocalVars[i][0] = iter->first;
            sLocalVars[i][1] = iter->first;
            dLocalVars[i] = *(iter->second);
            i++;
        }

        nLocalStringMapSize = sStringMap.size();
        if (nLocalStringMapSize)
        {
            sLocalStrings = new string*[nLocalStringMapSize];
            i = 0;
            for (auto iter = sStringMap.begin(); iter != sStringMap.end(); ++iter)
            {
                sLocalStrings[i] = new string[2];
                sLocalStrings[i][0] = iter->first;
                sLocalStrings[i][1] = iter->first;
            }
        }
    }
    _option._debug.gatherInformations(sLocalVars, nLocalVarMapSize-nLocalVarMapSkip, dLocalVars, sLocalStrings, nLocalStringMapSize, sStringMap, sCurrentCommand, sScriptFileName, nScriptLine);
    showDebugEvent(_lang.get("DBG_HEADLINE"), _option._debug.getModuleInformations(), _option._debug.getStackTrace(), _option._debug.getNumVars(), _option._debug.getStringVars());
    gotoLine(_option._debug.getErrorModule(), _option._debug.getLineNumber());
    _option._debug.resetBP();
    if (sLocalVars)
    {
        delete[] dLocalVars;
        for (size_t i = 0; i < nLocalVarMapSize; i++)
            delete[] sLocalVars[i];
        delete[] sLocalVars;
    }
    if (sLocalStrings)
    {
        for (size_t i = 0; i < nLocalStringMapSize; i++)
            delete[] sLocalStrings[i];
        delete[] sLocalStrings;
    }
    waitForContinue();
}

void NumeReKernel::getline(string& sLine)
{
    if (!m_parent)
        return;
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        bGettingLine = true;
    }
    bool bgotline = false;
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
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        bGettingLine = false;
    }

    StripSpaces(sLine);
}

void NumeReKernel::setFileName(const string& sFileName)
{
    sFileToEdit = sFileName;
}

void make_hline(int nLength)
{
    if (nLength == -1)
    {
        NumeReKernel::printPreFmt("\r"+strfill(string(1,'='), NumeReKernel::nLINE_LENGTH-1, '=') + "\n");
	}
	else if (nLength < -1)
	{
        NumeReKernel::printPreFmt("\r"+strfill(string(1,'-'), NumeReKernel::nLINE_LENGTH-1, '-') + "\n");
	}
	else
	{
        NumeReKernel::printPreFmt("\r"+strfill(string(1,'='), nLength, '=') + "\n");
	}
	return;
}

void NumeReKernel::toggleTableStatus()
{
    bWritingTable = !bWritingTable;
}

void NumeReKernel::flush()
{
    if (!m_parent)
        return;
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(1);
}

bool NumeReKernel::GetAsyncCancelState()
{
    bool bCancel = bCancelSignal;
    bCancelSignal = false;
    if (bCancel || GetAsyncKeyState(VK_ESCAPE))
        return true;
    return false;
}


