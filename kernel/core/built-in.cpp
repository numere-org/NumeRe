/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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


#include "built-in.hpp"
#include "../kernel.hpp"
#include "io/fileops.hpp"
#include "datamanagement/dataops.hpp"

extern mglGraph _fontData;

static void BI_show_credits(Parser& _parser, Settings& _option);
static void BI_ListOptions(Settings& _option);
static bool BI_ListFiles(const string& sCmd, const Settings& _option);
static bool BI_ListDirectory(const string& sDir, const string& sParams, const Settings& _option);
static bool BI_newObject(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
static bool BI_editObject(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
static string BI_getVarList(const string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
static bool BI_executeCommand(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
static int swapCaches(string& sCmd, Datafile& _data, Parser& _parser, Settings& _option, Define& _functions);
static int renameCaches(string& sCmd, Datafile& _data, Parser& _parser, Settings& _option, Define& _functions);
static bool undefineFunctions(string sFunctionList, Define& _functions, const Settings& _option);
static int showDialog(string& sCmd);
static int showDataObject(string& sCmd);




/*
 * Built-In-Funktionen
 * -> Bieten die grundlegende Funktionalitaet dieses Frameworks
 */


// This function will simply display some credits for this application
static void BI_show_credits(Parser& _parser, Settings& _option)
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::printPreFmt("|-> ");
	BI_splash();
	NumeReKernel::printPreFmt("\n");
	make_hline();
	NumeReKernel::printPreFmt("|-> Version: " + sVersion);
	NumeReKernel::printPreFmt(" | " + _lang.get("BUILTIN_CREDITS_BUILD") + ": " + AutoVersion::YEAR + "-" + AutoVersion::MONTH + "-" + AutoVersion::DATE + "\n");
	NumeReKernel::print("Copyright (c) 2013-" + (AutoVersion::YEAR + toSystemCodePage(", Erik HÄNEL et al.")) );
	NumeReKernel::printPreFmt("|   <numere.developer@gmail.com>\n" );
	NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CREDITS_VERSIONINFO"), _option) );
	make_hline(-80);
	NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CREDITS_LICENCE_1"), _option) );
	NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CREDITS_LICENCE_2"), _option) );
	NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CREDITS_LICENCE_3"), _option) );
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}

// This function displays the intro text
void BI_splash()
{
	NumeReKernel::printPreFmt("NUMERE: FRAMEWORK FÜR NUMERISCHE RECHNUNGEN");
	return;
}

// This function is the main command handling function
// It will identify the comands in the string and handle them
// correspondingly
// In the current implementation, it will also perform some pre-
// evaluation steps. This has to be changed
int BI_CommandHandler(string& sCmd, Datafile& _data, Output& _out, Settings& _option, Parser& _parser, Define& _functions, PlotData& _pData, Script& _script, bool bParserActive)
{
	string sArgument = "";  // String fuer das evtl. uebergebene Argument
	StripSpaces(sCmd);
	sCmd += " ";
	string sCommand = findCommand(sCmd).sString;
	int nArgument = -1;     // Integer fuer das evtl. uebergebene Argument
	unsigned int nPos = string::npos;
	Indices _idx;
	map<string, long long int> mCaches = _data.getCacheList();
	mCaches["data"] = -1;
	const static string sPreferredCmds = ";clear;copy;smooth;retoque;retouch;resample;stats;save;showf;swap;hist;help;man;move;matop;mtrxop;random;remove;rename;append;reload;delete;datagrid;list;load;export;edit";
	const static string sPlotCommands = " plotcompose plot plot3d graph graph3d mesh meshgrid mesh3d meshgrid3d surf surface surf3d surface3d cont contour cont3d contour3d vect vector vect3d vector3d dens density dens3d density3d draw draw3d grad gradient grad3d gradient3d ";
	string sCacheCmd = "";

	for (auto iter = mCaches.begin(); iter != mCaches.end(); ++iter)
	{
		if (findCommand(sCmd, iter->first).sString == iter->first)
		{
			sCacheCmd = iter->first;
			break;
		}
	}
	if (sCacheCmd.length() && sPreferredCmds.find(";" + sCommand + ";") != string::npos) // Ist das fuehrende Kommando praeferiert?
		sCacheCmd.clear();

	if (_option.getbDebug())
	{
		NumeReKernel::print("DEBUG: sCmd = " + sCmd );
		NumeReKernel::print("DEBUG: sCacheCmd = " + sCacheCmd );
		NumeReKernel::print("DEBUG: sCommand = " + sCommand );
		NumeReKernel::print("DEBUG: findCommand(sCmd, \"data\").sString = " + findCommand(sCmd, "data").sString );
	}

	if (sCommand == "find")
	{
		if (sCmd.length() > 6 && sCmd.find("-") != string::npos)
			doc_SearchFct(sCmd.substr(sCmd.find('-', findCommand(sCmd).nPos) + 1), _option);
		else if (sCmd.length() > 6)
		{
			doc_SearchFct(sCmd.substr(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + sCommand.length())), _option);
		}
		else
		{
			NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CHECKKEYWORD_FIND_CANNOT_READ"), _option));
			//NumeReKernel::print("|-> Kann den Begriff nicht identifizieren!" );
			doc_Help("find", _option);
		}
		return 1;
	}
	else if ((findCommand(sCmd, "integrate").sString == "integrate" || findCommand(sCmd, "integrate").sString == "integrate2" || findCommand(sCmd, "integrate").sString == "integrate2d") && sCmd.substr(0, 4) != "help")
	{
		nPos = findCommand(sCmd, "integrate").nPos;
		vector<double> vIntegrate;
		if (nPos)
		{
			sArgument = sCmd;
			sCmd = extractCommandString(sCmd, findCommand(sCmd, "integrate"));
			sArgument.replace(nPos, sCmd.length(), "<<ANS>>");
		}
		else
			sArgument = "<<ANS>>";
		sCmd = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
		StripSpaces(sCmd);
		/*if (!matchParams(sCmd, "x", '='))
		    throw NO_INTEGRATION_RANGES;*/
		if (bParserActive &&
				((findCommand(sCmd, "integrate").sString.length() >= 10 && findCommand(sCmd, "integrate").sString.substr(0, 10) == "integrate2")
				 || (matchParams(sCmd, "x", '=') && matchParams(sCmd, "y", '='))))
		{
			vIntegrate = parser_Integrate_2(sCmd, _data, _parser, _option, _functions);
			sCmd = sArgument;
			sCmd.replace(sCmd.find("<<ANS>>"), 7, "_~integrate2[~_~]");
			_parser.SetVectorVar("_~integrate2[~_~]", vIntegrate);
			return 0;
		}
		else if (bParserActive)
		{
			vIntegrate = parser_Integrate(sCmd, _data, _parser, _option, _functions);
			sCmd = sArgument;
			sCmd.replace(sCmd.find("<<ANS>>"), 7, "_~integrate[~_~]");
			_parser.SetVectorVar("_~integrate[~_~]", vIntegrate);
			return 0;
		}
		else
		{
			doc_Help("integrate", _option);
			return 1;
		}
	}
	else if (findCommand(sCmd, "diff").sString == "diff" && sCommand != "help")
	{
		nPos = findCommand(sCmd, "diff").nPos;
		vector<double> vDiff;

		if (nPos)
		{
			sArgument = sCmd;
			sCmd = extractCommandString(sCmd, findCommand(sCmd, "diff"));
			sArgument.replace(nPos, sCmd.length(), "<<ANS>>");
		}
		else
			sArgument = "<<ANS>>";
		if (bParserActive && sCmd.length() > 5)
		{
			vDiff = parser_Diff(sCmd, _parser, _data, _option, _functions);
			sCmd = sArgument;
			/*sArgument = "{{";
			for (unsigned int i = 0; i < vDiff.size(); i++)
			{
			    sArgument += toCmdString(vDiff[i]);
			    if (i < vDiff.size()-1)
			        sArgument += ",";
			}
			sArgument += "}}";*/
			sCmd.replace(sCmd.find("<<ANS>>"), 7, "_~diff[~_~]");
			_parser.SetVectorVar("_~diff[~_~]", vDiff);
			return 0;
		}
		else
			doc_Help("diff", _option);
		return 1;
	}
	else if (findCommand(sCmd, "extrema").sString == "extrema" && sCommand != "help")
	{
		nPos = findCommand(sCmd, "extrema").nPos;
		if (nPos)
		{
			sArgument = sCmd;
			sCmd = extractCommandString(sCmd, findCommand(sCmd, "extrema"));
			sArgument.replace(nPos, sCmd.length(), "<<ans>>");
		}
		else
			sArgument = "<<ans>>";
		if (bParserActive && sCmd.length() > 8)
		{
			if (parser_findExtrema(sCmd, _data, _parser, _option, _functions))
			{
				if (sCmd[0] != '"')
				{
					sArgument.replace(sArgument.find("<<ans>>"), 7, sCmd);
					sCmd = sArgument;
				}
				return 0;
			}
			else
				doc_Help("extrema", _option);
			return 1;
		}
		else
			doc_Help("extrema", _option);
		return 1;
	}
	else if (findCommand(sCmd, "pulse").sString == "pulse" && sCommand != "help")
	{
		if (!parser_pulseAnalysis(sCmd, _parser, _data, _functions, _option))
		{
			doc_Help("pulse", _option);
			return 1;
		}
		return 0;
	}
	else if (findCommand(sCmd, "eval").sString == "eval" && sCommand != "help")
	{
		nPos = findCommand(sCmd, "eval").nPos;
		if (nPos)
		{
			sArgument = sCmd;
			sCmd = extractCommandString(sCmd, findCommand(sCmd, "eval"));
			sArgument.replace(nPos, sCmd.length(), "<<ans>>");
		}
		else
			sArgument = "<<ans>>";

		if (parser_evalPoints(sCmd, _data, _parser, _option, _functions))
		{
			if (sCmd[0] != '"')
			{
				sArgument.replace(sArgument.find("<<ans>>"), 7, sCmd);
				sCmd = sArgument;
			}
			return 0;
		}
		else
			doc_Help("eval", _option);
		return 1;
	}
	else if (findCommand(sCmd, "zeroes").sString == "zeroes" && sCommand != "help")
	{
		nPos = findCommand(sCmd, "zeroes").nPos;
		if (nPos)
		{
			sArgument = sCmd;
			sCmd = extractCommandString(sCmd, findCommand(sCmd, "zeroes"));
			sArgument.replace(nPos, sCmd.length(), "<<ans>>");
		}
		else
			sArgument = "<<ans>>";
		if (bParserActive && sCmd.length() > 7)
		{
			if (parser_findZeroes(sCmd, _data, _parser, _option, _functions))
			{
				if (sCmd[0] != '"')
				{
					sArgument.replace(sArgument.find("<<ans>>"), 7, sCmd);
					sCmd = sArgument;
				}
				return 0;
			}
			else
				doc_Help("zeroes", _option);
			return 1;
		}
		else
			doc_Help("zeroes", _option);
		return 1;
	}
	else if (findCommand(sCmd, "sort").sString == "sort" && sCommand != "help")
	{
		nPos = findCommand(sCmd, "sort").nPos;
		if (nPos)
		{
			sArgument = sCmd;
			sCmd = extractCommandString(sCmd, findCommand(sCmd, "sort"));
			sArgument.replace(nPos, sCmd.length(), "<<ans>>");
		}
		else
			sArgument = "<<ans>>";

		sortData(sCmd, _parser, _data, _functions, _option);

		if (sCmd.length())
		{
			sArgument.replace(sArgument.find("<<ans>>"), 7, sCmd);
			sCmd = sArgument;
			return 0;
		}
		return 1;
	}
	else if (findCommand(sCmd, "dialog").sString == "dialog" && sCommand != "help")
	{
        return showDialog(sCmd);
	}
	else if (sPlotCommands.find(" " + sCommand + " ") != string::npos)
	{
		if (sCmd.length() > sCommand.length() + 1)
		{

			if (sCommand == "graph")
				sCmd.replace(findCommand(sCmd).nPos, 5, "plot");
			if (sCommand == "graph3d")
				sCmd.replace(findCommand(sCmd).nPos, 7, "plot3d");
			if (sCmd.find("--") != string::npos || sCmd.find("-set") != string::npos)
			{
				string sCmdSubstr;
				if (sCmd.find("--") != string::npos)
					sCmdSubstr = sCmd.substr(4, sCmd.find("--") - 4);
				else
					sCmdSubstr = sCmd.substr(4, sCmd.find("-set") - 4);
				if (!isNotEmptyExpression(sCmdSubstr))
				{
					if (sCmd.find("--") != string::npos)
						_pData.setParams(sCmd.substr(sCmd.find("--")), _parser, _option);
					else
						_pData.setParams(sCmd.substr(sCmd.find("-set")), _parser, _option);
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_PLOTPARAMS")));
					//NumeReKernel::print("|-> Plotparameter aktualisiert." );
				}
				else
					parser_Plot(sCmd, _data, _parser, _option, _functions, _pData);
			}
			else
				parser_Plot(sCmd, _data, _parser, _option, _functions, _pData);

		}
		else
			doc_Help(sCommand, _option);
		return 1;
	}
	else if ((findCommand(sCmd, "fit").sString == "fit" || findCommand(sCmd, "fit").sString == "fitw") && sCommand != "help")
	{
		if (_data.isValid() || _data.isValidCache())
			parser_fit(sCmd, _parser, _data, _functions, _option);
		else
			doc_Help("fit", _option);
		return 1;
	}
	else if (sCommand == "fft")
	{
		parser_fft(sCmd, _parser, _data, _option);
		return 1;
	}
	else if (sCommand == "fwt")
	{
		parser_wavelet(sCmd, _parser, _data, _option);
		return 1;
	}
	else if (findCommand(sCmd, "get").sString == "get" && sCommand != "help")
	{
		nPos = findCommand(sCmd, "get").nPos;
		sCommand = extractCommandString(sCmd, findCommand(sCmd, "get"));
		if (matchParams(sCmd, "savepath"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + _option.getSavePath() + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + _option.getSavePath() + "\"");
				return 0;
			}
			NumeReKernel::print("SAVEPATH: \"" + _option.getSavePath() + "\"");
			return 1;
		}
		else if (matchParams(sCmd, "loadpath"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + _option.getLoadPath() + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + _option.getLoadPath() + "\"");
				return 0;
			}
			NumeReKernel::print("LOADPATH: \"" + _option.getLoadPath() + "\"");
			return 1;
		}
		else if (matchParams(sCmd, "workpath"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + _option.getWorkPath() + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + _option.getWorkPath() + "\"");
				return 0;
			}
			NumeReKernel::print("WORKPATH: \"" + _option.getWorkPath() + "\"");
			return 1;
		}
		else if (matchParams(sCmd, "viewer"))
		{
			if (_option.getViewerPath().length())
			{
				if (matchParams(sCmd, "asstr"))
				{
					if (_option.getViewerPath()[0] == '"' && _option.getViewerPath()[_option.getViewerPath().length() - 1] == '"')
					{
						if (!nPos)
							sCmd = _option.getViewerPath();
						else
							sCmd.replace(nPos, sCommand.length(), _option.getViewerPath());
					}
					else
					{
						if (!nPos)
							sCmd = "\"" + _option.getViewerPath() + "\"";
						else
							sCmd.replace(nPos, sCommand.length(), "\"" + _option.getViewerPath() + "\"");
					}
					return 0;
				}
				if (_option.getViewerPath()[0] == '"' && _option.getViewerPath()[_option.getViewerPath().length() - 1] == '"')
					NumeReKernel::print(LineBreak("IMAGEVIEWER: " + _option.getViewerPath(), _option));
				else
					NumeReKernel::print(LineBreak("|-> IMAGEVIEWER: \"" + _option.getViewerPath() + "\"", _option));
			}
			else
			{
				if (matchParams(sCmd, "asstr"))
				{
					if (!nPos)
						sCmd = "\"\"";
					else
						sCmd.replace(nPos, sCommand.length(), "\"\"");
					return 0;
				}
				else
					NumeReKernel::print("Kein Imageviewer deklariert!");
			}
			return 1;
		}
		else if (matchParams(sCmd, "editor"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (_option.getEditorPath()[0] == '"' && _option.getEditorPath()[_option.getEditorPath().length() - 1] == '"')
				{
					if (!nPos)
						sCmd = _option.getEditorPath();
					else
						sCmd.replace(nPos, sCommand.length(), _option.getEditorPath());
				}
				else
				{
					if (!nPos)
						sCmd = "\"" + _option.getEditorPath() + "\"";
					else
						sCmd.replace(nPos, sCommand.length(), "\"" + _option.getEditorPath() + "\"");
				}
				return 0;
			}
			if (_option.getEditorPath()[0] == '"' && _option.getEditorPath()[_option.getEditorPath().length() - 1] == '"')
				NumeReKernel::print(LineBreak("TEXTEDITOR: " + _option.getEditorPath(), _option));
			else
				NumeReKernel::print(LineBreak("TEXTEDITOR: \"" + _option.getEditorPath() + "\"", _option));
			return 1;
		}
		else if (matchParams(sCmd, "scriptpath"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + _option.getScriptPath() + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + _option.getScriptPath() + "\"");
				return 0;
			}
			NumeReKernel::print("SCRIPTPATH: \"" + _option.getScriptPath() + "\"");
			return 1;
		}
		else if (matchParams(sCmd, "procpath"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + _option.getProcsPath() + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + _option.getProcsPath() + "\"");
				return 0;
			}
			NumeReKernel::print("PROCPATH: \"" + _option.getProcsPath() + "\"");
			return 1;
		}
		else if (matchParams(sCmd, "plotfont"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + _option.getDefaultPlotFont() + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + _option.getDefaultPlotFont() + "\"");
				return 0;
			}
			NumeReKernel::print("PLOTFONT: \"" + _option.getDefaultPlotFont() + "\"");
			return 1;
		}
		else if (matchParams(sCmd, "precision"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getPrecision());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getPrecision()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getPrecision()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getPrecision()) + "\"");
				return 0;
			}
			NumeReKernel::print("PRECISION = " + toString(_option.getPrecision()));
			return 1;
		}
		else if (matchParams(sCmd, "faststart"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbFastStart());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbFastStart()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbFastStart()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbFastStart()) + "\"");
				return 0;
			}
			NumeReKernel::print("FASTSTART: " + toString(_option.getbFastStart()));
			return 1;
		}
		else if (matchParams(sCmd, "compact"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbCompact());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbCompact()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbCompact()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbCompact()) + "\"");
				return 0;
			}
			NumeReKernel::print("COMPACT-MODE: " + toString(_option.getbCompact()));
			return 1;
		}
		else if (matchParams(sCmd, "autosave"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getAutoSaveInterval());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getAutoSaveInterval()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getAutoSaveInterval()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getAutoSaveInterval()) + "\"");
				return 0;
			}
			NumeReKernel::print("AUTOSAVE-INTERVAL: " + toString(_option.getAutoSaveInterval()) + " [sec]");
			return 1;
		}
		else if (matchParams(sCmd, "plotparams"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = _pData.getParams(_option, true);
				else
					sCmd.replace(nPos, sCommand.length(), _pData.getParams(_option, true));
				return 0;
			}
			NumeReKernel::print(LineBreak("PLOTPARAMS: " + _pData.getParams(_option), _option, false));
			return 1;
		}
		else if (matchParams(sCmd, "varlist"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = BI_getVarList("vars -asstr", _parser, _data, _option);
				else
					sCmd.replace(nPos, sCommand.length(), BI_getVarList("vars -asstr", _parser, _data, _option));
				return 0;
			}
			NumeReKernel::print(LineBreak("VARS: " + BI_getVarList("vars", _parser, _data, _option), _option, false));
			return 1;
		}
		else if (matchParams(sCmd, "stringlist"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = BI_getVarList("strings -asstr", _parser, _data, _option);
				else
					sCmd.replace(nPos, sCommand.length(), BI_getVarList("strings -asstr", _parser, _data, _option));
				return 0;
			}
			NumeReKernel::print(LineBreak("STRINGS: " + BI_getVarList("strings", _parser, _data, _option), _option, false));
			return 1;
		}
		else if (matchParams(sCmd, "numlist"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = BI_getVarList("nums -asstr", _parser, _data, _option);
				else
					sCmd.replace(nPos, sCommand.length(), BI_getVarList("nums -asstr", _parser, _data, _option));
				return 0;
			}
			NumeReKernel::print(LineBreak("NUMS: " + BI_getVarList("nums", _parser, _data, _option), _option, false));
			return 1;
		}
		else if (matchParams(sCmd, "plotpath"))
		{
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + _option.getPlotOutputPath() + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + _option.getPlotOutputPath() + "\"");
				return 0;
			}
			NumeReKernel::print("PLOTPATH: \"" + _option.getPlotOutputPath() + "\"");
			return 1;
		}
		else if (matchParams(sCmd, "greeting"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbGreeting());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbGreeting()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbGreeting()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbGreeting()) + "\"");
				return 0;
			}
			NumeReKernel::print("GREETING: " + toString(_option.getbGreeting()));
			return 1;
		}
		else if (matchParams(sCmd, "hints"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbShowHints());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbShowHints()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbShowHints()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbGreeting()) + "\"");
				return 0;
			}
			NumeReKernel::print("HINTS: " + toString(_option.getbGreeting()));
			return 1;
		}
		else if (matchParams(sCmd, "useescinscripts"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbUseESCinScripts());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseESCinScripts()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbUseESCinScripts()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbUseESCinScripts()) + "\"");
				return 0;
			}
			NumeReKernel::print("USEESCINSCRIPTS: " + toString(_option.getbUseESCinScripts()));
			return 1;
		}
		else if (matchParams(sCmd, "usecustomlang"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getUseCustomLanguageFiles());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getUseCustomLanguageFiles()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getUseCustomLanguageFiles()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getUseCustomLanguageFiles()) + "\"");
				return 0;
			}
			NumeReKernel::print("USECUSTOMLANG: " + toString(_option.getUseCustomLanguageFiles()));
			return 1;
		}
		else if (matchParams(sCmd, "externaldocwindow"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getUseExternalViewer());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getUseExternalViewer()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getUseExternalViewer()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getUseExternalViewer()) + "\"");
				return 0;
			}
			NumeReKernel::print("EXTERNALDOCWINDOW: " + toString(_option.getUseExternalViewer()));
			return 1;
		}
		else if (matchParams(sCmd, "draftmode"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbUseDraftMode());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseDraftMode()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbUseDraftMode()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbUseDraftMode()) + "\"");
				return 0;
			}
			NumeReKernel::print("DRAFTMODE: " + toString(_option.getbUseDraftMode()));
			return 1;
		}
		else if (matchParams(sCmd, "extendedfileinfo"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbShowExtendedFileInfo());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbShowExtendedFileInfo()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbShowExtendedFileInfo()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbShowExtendedFileInfo()) + "\"");
				return 0;
			}
			NumeReKernel::print("EXTENDED FILEINFO: " + toString(_option.getbShowExtendedFileInfo()));
			return 1;
		}
		else if (matchParams(sCmd, "loademptycols"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbLoadEmptyCols());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbLoadEmptyCols()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbLoadEmptyCols()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbLoadEmptyCols()) + "\"");
				return 0;
			}
			NumeReKernel::print("LOAD EMPTY COLS: " + toString(_option.getbLoadEmptyCols()));
			return 1;
		}
		else if (matchParams(sCmd, "logfile"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbUseLogFile());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbUseLogFile()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbUseLogFile()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbUseLogFile()) + "\"");
				return 0;
			}
			NumeReKernel::print("EXTENDED FILEINFO: " + toString(_option.getbUseLogFile()));
			return 1;
		}
		else if (matchParams(sCmd, "defcontrol"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString(_option.getbDefineAutoLoad());
				else
					sCmd.replace(nPos, sCommand.length(), toString(_option.getbDefineAutoLoad()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString(_option.getbDefineAutoLoad()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString(_option.getbDefineAutoLoad()) + "\"");
				return 0;
			}
			NumeReKernel::print("DEFCONTROL: " + toString(_option.getbDefineAutoLoad()));
			return 1;
		}
		else if (matchParams(sCmd, "buffersize"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString((int)_option.getBuffer(1));
				else
					sCmd.replace(nPos, sCommand.length(), toString((int)_option.getBuffer(1)));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString((int)_option.getBuffer(1)) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString((int)_option.getBuffer(1)) + "\"");
				return 0;
			}
			NumeReKernel::print("BUFFERSIZE: " + _option.getBuffer(1) );
			return 1;
		}
		else if (matchParams(sCmd, "windowsize"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = "x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1);
                else
                    sCmd.replace(nPos, sCommand.length(), "{" + toString((int)_option.getWindow() + 1) + ", " + toString((int)_option.getWindow(1) + 1) + "}");
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1) + "\"");
				return 0;
			}
			NumeReKernel::print("WINDOWSIZE: x = " + toString((int)_option.getWindow() + 1) + ", y = " + toString((int)_option.getWindow(1) + 1) );
			return 1;
		}
		else if (matchParams(sCmd, "colortheme"))
		{
			if (matchParams(sCmd, "asval"))
			{
				if (!nPos)
					sCmd = toString((int)_option.getColorTheme());
				else
					sCmd.replace(nPos, sCommand.length(), toString((int)_option.getColorTheme()));
				return 0;
			}
			if (matchParams(sCmd, "asstr"))
			{
				if (!nPos)
					sCmd = "\"" + toString((int)_option.getColorTheme()) + "\"";
				else
					sCmd.replace(nPos, sCommand.length(), "\"" + toString((int)_option.getColorTheme()) + "\"");
				return 0;
			}
			NumeReKernel::print("COLORTHEME: " + _option.getColorTheme() );
			return 1;
		}
		else
		{
			doc_Help("get", _option);
			return 1;
		}
	}
	else if (sCommand.substr(0, 5) == "undef" || sCommand == "undefine")
	{
		nPos = findCommand(sCmd).nPos;
		if (sCmd.length() > 7)
		{
			undefineFunctions(sCmd.substr(sCmd.find(' ', nPos) + 1), _functions, _option);
		}
		else
			doc_Help("define", _option);
		return 1;
	}
	else if (findCommand(sCmd, "readline").sString == "readline" && sCommand != "help")
	{
		nPos = findCommand(sCmd, "readline").nPos;
		sCommand = extractCommandString(sCmd, findCommand(sCmd, "readline"));
		string sDefault = "";
		if (matchParams(sCmd, "msg", '='))
		{
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd, nPos);
			//addArgumentQuotes(sCmd, "msg");
			sCmd = sCmd.replace(nPos, sCommand.length(), BI_evalParamString(sCommand, _parser, _data, _option, _functions));
			sCommand = BI_evalParamString(sCommand, _parser, _data, _option, _functions);
		}
		if (matchParams(sCmd, "dflt", '='))
		{
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd, nPos);
			//addArgumentQuotes(sCmd, "dflt");
			sCmd = sCmd.replace(nPos, sCommand.length(), BI_evalParamString(sCommand, _parser, _data, _option, _functions));
			sCommand = BI_evalParamString(sCommand, _parser, _data, _option, _functions);
			sDefault = getArgAtPos(sCmd, matchParams(sCmd, "dflt", '=') + 4);
		}
		while (!sArgument.length())
		{
			string sLastLine = "";
			NumeReKernel::printPreFmt("|-> ");
			if (matchParams(sCmd, "msg", '='))
			{
				//unsigned int n_pos = matchParams(sCmd, "msg", '=') + 3;
				//NumeReKernel::print(toSystemCodePage(sCmd.substr(sCmd.find('"', n_pos)+1, sCmd.rfind('"')-sCmd.find('"', n_pos)-1));
				sLastLine = LineBreak(getArgAtPos(sCmd, matchParams(sCmd, "msg", '=') + 3), _option, false, 4);
				NumeReKernel::printPreFmt(sLastLine);
				if (sLastLine.find('\n') != string::npos)
					sLastLine.erase(0, sLastLine.rfind('\n'));
				if (sLastLine.substr(0, 4) == "|   " || sLastLine.substr(0, 4) == "|<- " || sLastLine.substr(0, 4) == "|-> ")
					sLastLine.erase(0, 4);
				StripSpaces(sLastLine);
			}
			NumeReKernel::getline(sArgument);
			if (sLastLine.length() && sArgument.find(sLastLine) != string::npos)
				sArgument.erase(0, sArgument.find(sLastLine) + sLastLine.length());
			StripSpaces(sArgument);
			if (!sArgument.length() && sDefault.length())
				sArgument = sDefault;
		}
		if (matchParams(sCmd, "asstr") && sArgument[0] != '"' && sArgument[sArgument.length() - 1] != '"')
			sCmd = sCmd.replace(nPos, sCommand.length(), "\"" + sArgument + "\"");
		else
			sCmd = sCmd.replace(nPos, sCommand.length(), sArgument);
		GetAsyncKeyState(VK_ESCAPE);
		return 0;
	}
	else if (findCommand(sCmd, "read").sString == "read" && sCommand != "help")
	{
		nPos = findCommand(sCmd, "read").nPos;
		sArgument = extractCommandString(sCmd, findCommand(sCmd, "read"));
		sCommand = sArgument;
		if (sArgument.length() > 5) //matchParams(sArgument, "file", '='))
		{
			readFromFile(sArgument, _parser, _data, _option);
			sCmd.replace(nPos, sCommand.length(), sArgument);
			return 0;
		}
		else
			doc_Help("read", _option);
		return 1;
	}
	else if (findCommand(sCmd, "data").sString == "data" && sCacheCmd == "data" && sCommand != "clear" && sCommand != "copy")
	{
	    // DEPRECATED: Declared at v1.1.2rc1
		NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));

		if (matchParams(sCmd, "clear"))
		{
			if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
				remove_data(_data, _option, true);
			else
				remove_data(_data, _option);
			return 1;
		}
		else if (matchParams(sCmd, "load") || matchParams(sCmd, "load", '='))
		{
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd);
			if (matchParams(sCmd, "load", '='))
				addArgumentQuotes(sCmd, "load");
			if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
			{
				if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
					_data.setbLoadEmptyColsInNextFile(true);
				if (matchParams(sCmd, "slices", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slices", '=') + 6) == "xz")
					nArgument = -1;
				else if (matchParams(sCmd, "slices", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slices", '=') + 6) == "yz")
					nArgument = -2;
				else
					nArgument = 0;
				if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
				{
					if (_data.isValid())
					{
						if (_option.getSystemPrintStatus())
							_data.removeData(false);
						else
							_data.removeData(true);
					}
					if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
					{
						if (sArgument.find('/') == string::npos)
							sArgument = "<loadpath>/" + sArgument;
						vector<string> vFilelist = getFileList(sArgument, _option);
						if (!vFilelist.size())
						{
							throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
						}
						string sPath = "<loadpath>/";
						if (sArgument.find('/') != string::npos)
							sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
						_data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
						Datafile _cache;
						_cache.setTokens(_option.getTokenPaths());
						_cache.setPath(_data.getPath(), false, _data.getProgramPath());
						for (unsigned int i = 1; i < vFilelist.size(); i++)
						{
							_cache.removeData(false);
							_cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
							_data.melt(_cache);
						}
						if (_data.isValid())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
						//NumeReKernel::print(LineBreak("|-> Alle Daten der " + toString((int)vFilelist.size())+ " Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
						return 1;
					}
					if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
					{
						if (matchParams(sCmd, "head", '='))
							nArgument = matchParams(sCmd, "head", '=') + 4;
						else
							nArgument = matchParams(sCmd, "h", '=') + 1;
						nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
						_data.openFile(sArgument, _option, false, true, nArgument);
					}
					else
					{
						_data.openFile(sArgument, _option, false, true, nArgument);
					}
					if (_data.isValid() && _option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
					//NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
				}
				else if (!_data.isValid())
				{
					if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
					{
						if (sArgument.find('/') == string::npos)
							sArgument = "<loadpath>/" + sArgument;
						//NumeReKernel::print(sArgument );
						vector<string> vFilelist = getFileList(sArgument, _option);
						if (!vFilelist.size())
						{
							throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
						}
						string sPath = "<loadpath>/";
						if (sArgument.find('/') != string::npos)
							sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
						_data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
						Datafile _cache;
						_cache.setTokens(_option.getTokenPaths());
						_cache.setPath(_data.getPath(), false, _data.getProgramPath());
						for (unsigned int i = 1; i < vFilelist.size(); i++)
						{
							_cache.removeData(false);
							_cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
							_data.melt(_cache);
						}
						if (_data.isValid())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
						//NumeReKernel::print(LineBreak("|-> Alle Daten der " +toString((int)vFilelist.size())+ " Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
						return 1;
					}
					if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
					{
						if (matchParams(sCmd, "head", '='))
							nArgument = matchParams(sCmd, "head", '=') + 4;
						else
							nArgument = matchParams(sCmd, "h", '=') + 1;
						nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
						_data.openFile(sArgument, _option, false, true, nArgument);
					}
					else
						_data.openFile(sArgument, _option, false, false, nArgument);
					if (_data.isValid() && _option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
					//NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
				}
				else
					load_data(_data, _option, _parser, sArgument);
			}
			else
				load_data(_data, _option, _parser);
			return 1;
		}
		else if (matchParams(sCmd, "paste") || matchParams(sCmd, "pasteload"))
		{
			_data.pasteLoad(_option);
			if (_data.isValid())
				NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_PASTE_SUCCESS", toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
			//NumeReKernel::print(LineBreak("|-> Die Daten wurden erfolgreich eingefügt: Der Datensatz besteht nun aus "+toString(_data.getLines("data"))+" Zeile(n) und "+toString(_data.getCols("data"))+" Spalte(n).", _option) );
			return 1;
		}
		else if (matchParams(sCmd, "reload") || matchParams(sCmd, "reload", '='))
		{
			if ((_data.getDataFileName("data") == "Merged Data" || _data.getDataFileName("data") == "Pasted Data") && !matchParams(sCmd, "reload", '='))
				//throw CANNOT_RELOAD_DATA;
				throw SyntaxError(SyntaxError::CANNOT_RELOAD_DATA, "", SyntaxError::invalid_position);
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd);
			if (matchParams(sCmd, "reload", '='))
				addArgumentQuotes(sCmd, "reload");
			if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
			{
				if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
					_data.setbLoadEmptyColsInNextFile(true);
				if (_data.isValid())
				{
					_data.removeData(false);
					if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
					{
						if (matchParams(sCmd, "head", '='))
							nArgument = matchParams(sCmd, "head", '=') + 4;
						else
							nArgument = matchParams(sCmd, "h", '=') + 1;
						nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
						_data.openFile(sArgument, _option, false, true, nArgument);
					}
					else
						_data.openFile(sArgument, _option);
					if (_data.isValid() && _option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_FILE_SUCCESS", _data.getDataFileName("data")), _option) );
					//NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich aktualisiert.", _option) );
				}
				else
					load_data(_data, _option, _parser, sArgument);
			}
			else if (_data.isValid())
			{
				if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
					_data.setbLoadEmptyColsInNextFile(true);
				sArgument = _data.getDataFileName("data");
				_data.removeData(false);
				_data.openFile(sArgument, _option, false, true);
				if (_data.isValid() && _option.getSystemPrintStatus())
					NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_SUCCESS"), _option) );
				//NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich aktualisiert.", _option) );
			}
			else
				load_data(_data, _option, _parser);
			return 1;
		}
		else if (matchParams(sCmd, "app") || matchParams(sCmd, "app", '='))
		{
			append_data(sCmd, _data, _option, _parser);
			return 1;
		}
		else if (matchParams(sCmd, "showf"))
		{
			show_data(_data, _out, _option, "data", _option.getPrecision(), true, false);
			return 1;
		}
		else if (matchParams(sCmd, "show"))
		{
			_out.setCompact(_option.getbCompact());
			show_data(_data, _out, _option, "data", _option.getPrecision(), true, false);
			return 1;
		}
		else if (sCmd.substr(0, 5) == "data(")
		{
			return 0;
		}
		else if (matchParams(sCmd, "stats"))
		{
			sArgument = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
			if (_data.isValid())
				plugin_statistics(sArgument, _data, _out, _option, false, true);
			else
				//throw NO_DATA_AVAILABLE;
				throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, sArgument, sArgument);
			return 1;
		}
		else if (matchParams(sCmd, "hist"))
		{
			sArgument = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
			if (_data.isValid())
				plugin_histogram(sArgument, _data, _data, _out, _option, _pData, false, true);
			else
				//throw NO_DATA_AVAILABLE;
				throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, sArgument, sArgument);
			return 1;
		}
		else if (matchParams(sCmd, "save") || matchParams(sCmd, "save", '='))
		{
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd);
			if (matchParams(sCmd, "save", '='))
				addArgumentQuotes(sCmd, "save");
			_data.setPrefix("data");
			if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
			{
				if (_data.saveFile("data", sArgument))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
					//NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
				}
				else
					//throw CANNOT_SAVE_FILE;
					throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
				//NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
			}
			else
			{
				sArgument = _data.getDataFileName("data");
				if (sArgument.find('\\') != string::npos)
					sArgument = sArgument.substr(sArgument.rfind('\\') + 1);
				if (sArgument.find('/') != string::npos)
					sArgument = sArgument.substr(sArgument.rfind('/') + 1);
				if (sArgument.substr(sArgument.rfind('.')) != ".ndat")
					sArgument = sArgument.substr(0, sArgument.rfind('.')) + ".ndat";
				if (_data.saveFile("data", "copy_of_" + sArgument))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
					//NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
				}
				else
					//throw CANNOT_SAVE_FILE;
					throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
				//NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
			}
			return 1;
		}
		else if (matchParams(sCmd, "sort", '=') || matchParams(sCmd, "sort"))
		{
			_data.sortElements(sCmd);
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SORT_SUCCESS"), _option) );
			return 1;
		}
		else if (matchParams(sCmd, "export") || matchParams(sCmd, "export", '='))
		{
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd);
			if (matchParams(sCmd, "export", '='))
				addArgumentQuotes(sCmd, "export");
			if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
			{
				_out.setFileName(sArgument);
				show_data(_data, _out, _option, "data", _option.getPrecision(), true, false, true, false);
			}
			else
				show_data(_data, _out, _option, "data", _option.getPrecision(), true, false, true);
			return 1;
		}
		else if ((matchParams(sCmd, "avg")
				  || matchParams(sCmd, "sum")
				  || matchParams(sCmd, "min")
				  || matchParams(sCmd, "max")
				  || matchParams(sCmd, "norm")
				  || matchParams(sCmd, "std")
				  || matchParams(sCmd, "prd")
				  || matchParams(sCmd, "num")
				  || matchParams(sCmd, "cnt")
				  || matchParams(sCmd, "and")
				  || matchParams(sCmd, "or")
				  || matchParams(sCmd, "xor")
				  || matchParams(sCmd, "med"))
				 && (matchParams(sCmd, "lines") || matchParams(sCmd, "cols")))
		{
			if (!_data.isValid())
				//throw NO_DATA_AVAILABLE;
				throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
			string sEvery = "";
			if (matchParams(sCmd, "every", '='))
			{
				value_type* v = 0;
				_parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "every", '=') + 5));
				v = _parser.Eval(nArgument);
				if (nArgument > 1)
				{
					sEvery = "every=" + toString((int)v[0]) + "," + toString((int)v[1]) + " ";
				}
				else
					sEvery = "every=" + toString((int)v[0]) + " ";
			}
			nPos = findCommand(sCmd, "data").nPos;
			sArgument = extractCommandString(sCmd, findCommand(sCmd, "data"));
			sCommand = sArgument;
			if (matchParams(sCmd, "grid"))
				sArgument = "grid";
			else
				sArgument.clear();
			if (matchParams(sCmd, "avg"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[avg_lines]");
					_parser.SetVectorVar("_~data[avg_lines]", _data.avg("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[avg_cols]");
					_parser.SetVectorVar("_~data[avg_cols]", _data.avg("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "sum"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[sum_lines]");
					_parser.SetVectorVar("_~data[sum_lines]", _data.sum("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[sum_cols]");
					_parser.SetVectorVar("_~data[sum_cols]", _data.sum("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "min"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[min_lines]");
					_parser.SetVectorVar("_~data[min_lines]", _data.min("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[min_cols]");
					_parser.SetVectorVar("_~data[min_cols]", _data.min("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "max"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[max_lines]");
					_parser.SetVectorVar("_~data[max_lines]", _data.max("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[max_cols]");
					_parser.SetVectorVar("_~data[max_cols]", _data.max("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "norm"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[norm_lines]");
					_parser.SetVectorVar("_~data[norm_lines]", _data.norm("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[norm_cols]");
					_parser.SetVectorVar("_~data[norm_cols]", _data.norm("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "std"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[std_lines]");
					_parser.SetVectorVar("_~data[std_lines]", _data.std("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[std_cols]");
					_parser.SetVectorVar("_~data[std_cols]", _data.std("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "prd"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[prd_lines]");
					_parser.SetVectorVar("_~data[prd_lines]", _data.prd("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[prd_cols]");
					_parser.SetVectorVar("_~data[prd_cols]", _data.prd("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "num"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[num_lines]");
					_parser.SetVectorVar("_~data[num_lines]", _data.num("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[num_cols]");
					_parser.SetVectorVar("_~data[num_cols]", _data.num("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "cnt"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[cnt_lines]");
					_parser.SetVectorVar("_~data[cnt_lines]", _data.cnt("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[cnt_cols]");
					_parser.SetVectorVar("_~data[cnt_cols]", _data.cnt("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "med"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[med_lines]");
					_parser.SetVectorVar("_~data[med_lines]", _data.med("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[med_cols]");
					_parser.SetVectorVar("_~data[med_cols]", _data.med("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "and"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[and_lines]");
					_parser.SetVectorVar("_~data[and_lines]", _data.and_func("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[and_cols]");
					_parser.SetVectorVar("_~data[and_cols]", _data.and_func("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "or"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[or_lines]");
					_parser.SetVectorVar("_~data[or_lines]", _data.or_func("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[or_cols]");
					_parser.SetVectorVar("_~data[or_cols]", _data.or_func("data", sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "xor"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[xor_lines]");
					_parser.SetVectorVar("_~data[xor_lines]", _data.xor_func("data", sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~data[xor_cols]");
					_parser.SetVectorVar("_~data[xor_cols]", _data.xor_func("data", sArgument + "cols" + sEvery));
				}
			}

			return 0;
		}
		else if ((matchParams(sCmd, "avg")
				  || matchParams(sCmd, "sum")
				  || matchParams(sCmd, "min")
				  || matchParams(sCmd, "max")
				  || matchParams(sCmd, "norm")
				  || matchParams(sCmd, "std")
				  || matchParams(sCmd, "prd")
				  || matchParams(sCmd, "num")
				  || matchParams(sCmd, "cnt")
				  || matchParams(sCmd, "and")
				  || matchParams(sCmd, "or")
				  || matchParams(sCmd, "xor")
				  || matchParams(sCmd, "med"))
				)
		{
			if (!_data.isValid())
				//throw NO_DATA_AVAILABLE;
				throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
			nPos = findCommand(sCmd, "data").nPos;
			sArgument = extractCommandString(sCmd, findCommand(sCmd, "data"));
			sCommand = sArgument;
			if (matchParams(sCmd, "grid") && _data.getCols("data") < 3)
				//throw TOO_FEW_COLS;
				throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, "data", "data");
			else if (matchParams(sCmd, "grid"))
				nArgument = 2;
			else
				nArgument = 0;
			if (matchParams(sCmd, "avg"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.avg("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "sum"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.sum("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "min"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.min("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "max"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.max("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "norm"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.norm("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "std"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.std("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "prd"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.prd("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "num"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.num("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "cnt"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.cnt("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "med"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.med("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "and"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.and_func("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "or"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.or_func("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));
			else if (matchParams(sCmd, "xor"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.xor_func("data", 0, _data.getLines("data", false)-1, nArgument, _data.getCols("data")-1)));

			return 0;
		}
		else if (sCommand == "data")
		{
			doc_Help("data", _option);
			return 1;
		}
		else
			return 0;

	}
	else if (sCommand == "new")
	{
		if (matchParams(sCmd, "dir", '=')
				|| matchParams(sCmd, "script", '=')
				|| matchParams(sCmd, "proc", '=')
				|| matchParams(sCmd, "file", '=')
				|| matchParams(sCmd, "plugin", '=')
				|| matchParams(sCmd, "cache", '=')
				|| sCmd.find("()", findCommand(sCmd).nPos + 3) != string::npos
				|| sCmd.find('$', findCommand(sCmd).nPos + 3) != string::npos)
		{
			_data.setUserdefinedFuncs(_functions.getDefinesName());
			if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
				sCmd = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
			if (!BI_newObject(sCmd, _parser, _data, _option))
				doc_Help("new", _option);
		}
		else
			doc_Help("new", _option);
		return 1;
	}
	else if (sCommand == "view")
	{
		if (sCmd.length() > 5)
		{
			if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
			{
				BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option);
				sArgument = "edit " + sArgument;
				BI_editObject(sArgument, _parser, _data, _option);
			}
			else
				BI_editObject(sCmd, _parser, _data, _option);
		}
		else
			doc_Help("edit", _option);
		return 1;
	}
	else if (sCommand == "taylor")
	{
		if (sCmd.length() > 7)
			parser_Taylor(sCmd, _parser, _option, _functions);
		else
			doc_Help("taylor", _option);
		return 1;
	}
	else if (sCommand == "quit")
	{
		if (matchParams(sCmd, "as"))
			BI_Autosave(_data, _out, _option);
		if (matchParams(sCmd, "i"))
			_data.setSaveStatus(true);
		return -1;
	}
	else if (sCommand == "firststart")
	{
	    // DEPRECATED: Declared at v1.1.2rc1
	    NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
		doc_FirstStart(_option);
		return 1;
	}
	else if (sCommand == "open")
	{
		if (sCmd.length() > 5)
		{
			if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
			{
				BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option);
				sArgument = "edit " + sArgument;
				BI_editObject(sArgument, _parser, _data, _option);
			}
			else
				BI_editObject(sCmd, _parser, _data, _option);
		}
		else
			doc_Help("edit", _option);
		return 1;
	}
	else if (sCommand == "odesolve")
	{
		if (sCmd.length() > 9)
		{
			Odesolver _solver(&_parser, &_data, &_functions, &_option);
			_solver.solve(sCmd);
		}
		else
			doc_Help("odesolver", _option);
		return 1;
	}
	else if (sCacheCmd.length() // BUGGY
			 && findCommand(sCmd, sCacheCmd).sString == sCacheCmd
			 && sCommand != "clear"
			 && sCommand != "copy"
			 && sCommand != "smooth"
			 && sCommand != "retoque"
			 && sCommand != "retouch"
			 && sCommand != "resample")
	{
	    // DEPRECATED: Declared at v1.1.2rc1
	    NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
		if (matchParams(sCmd, "showf"))
		{
			show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true);
			return 1;
		}
		else if (matchParams(sCmd, "show"))
		{
			_out.setCompact(_option.getbCompact());
			show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true);
			return 1;
		}
		else if (matchParams(sCmd, "clear"))
		{
			if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
				clear_cache(_data, _option, true);
			else
				clear_cache(_data, _option);
			return 1;
		}
		else if (matchParams(sCmd, "hist"))
		{
			sArgument = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
			if (_data.isValidCache())
				plugin_histogram(sArgument, _data, _data, _out, _option, _pData, true, false);
			else
				//throw NO_DATA_AVAILABLE;
				throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
			return 1;
		}
		else if (matchParams(sCmd, "stats"))
		{
			sArgument = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
			if (matchParams(sCmd, "save", '='))
			{
				if (sCmd[sCmd.find("save=") + 5] == '"' || sCmd[sCmd.find("save=") + 5] == '#')
				{
					if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
						sArgument = "";
				}
				else
					sArgument = sCmd.substr(sCmd.find("save=") + 5, sCmd.find(' ', sCmd.find("save=") + 5) - sCmd.find("save=") - 5);
			}

			if (_data.isValidCache())
				plugin_statistics(sArgument, _data, _out, _option, true, false);
			else
				//throw NO_DATA_AVAILABLE;
				throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
			return 1;
		}
		else if (matchParams(sCmd, "save") || matchParams(sCmd, "save", '='))
		{
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd);
			if (matchParams(sCmd, "save", '='))
				addArgumentQuotes(sCmd, "save");
			_data.setPrefix(sCommand);
			if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
			{
				_data.setCacheStatus(true);
				if (_data.saveFile(sCommand, sArgument))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
					//NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
				}
				else
				{
					_data.setCacheStatus(false);
					//throw CANNOT_SAVE_FILE;
					throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
					//NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
				}
				_data.setCacheStatus(false);
			}
			else
			{
				_data.setCacheStatus(true);
				if (_data.saveFile(sCommand, ""))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _data.getOutputFileName()), _option) );
					//NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _data.getOutputFileName() + "\" gespeichert.", _option) );
				}
				else
				{
					_data.setCacheStatus(false);
					//throw CANNOT_SAVE_FILE;
					throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
					//NumeReKernel::print(LineBreak("|-> FEHLER: Daten konnten nicht gespeichert werden!", _option) );
				}
				_data.setCacheStatus(false);
			}
			return 1;
		}
		else if (matchParams(sCmd, "sort") || matchParams(sCmd, "sort", '='))
		{
			_data.sortElements(sCmd);
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SORT_SUCCESS"), _option) );
			return 1;
		}
		else if (matchParams(sCmd, "export") || matchParams(sCmd, "export", '='))
		{
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd);
			if (matchParams(sCmd, "export", '='))
				addArgumentQuotes(sCmd, "export");
			if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
			{
				_out.setFileName(sArgument);
				show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true, true, false);
			}
			else
				show_data(_data, _out, _option, sCommand, _option.getPrecision(), false, true, true);
			return 1;
		}
		else if (matchParams(sCmd, "rename", '=')) //CACHE -rename=NEWNAME
		{
			if (_data.containsStringVars(sCmd) || containsStrings(sCmd))
				sCmd = BI_evalParamString(sCmd, _parser, _data, _option, _functions);

			sArgument = getArgAtPos(sCmd, matchParams(sCmd, "rename", '=') + 6);
			_data.renameCache(sCommand, sArgument);
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RENAME_CACHE", sArgument), _option) );
			//NumeReKernel::print(LineBreak("|-> Der Cache wurde erfolgreich zu \""+sArgument+"\" umbenannt.", _option) );
			return 1;
		}
		else if (matchParams(sCmd, "swap", '=')) //CACHE -swap=NEWCACHE
		{
			if (_data.containsStringVars(sCmd) || containsStrings(sCmd))
				sCmd = BI_evalParamString(sCmd, _parser, _data, _option, _functions);

			sArgument = getArgAtPos(sCmd, matchParams(sCmd, "swap", '=') + 4);
			_data.swapCaches(sCommand, sArgument);
			if (_option.getSystemPrintStatus())
				NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SWAP_CACHE", sCommand, sArgument), _option) );
			//NumeReKernel::print(LineBreak("|-> Der Inhalt von \""+sCommand+"\" wurde erfolgreich mit dem Inhalt von \""+sArgument+"\" getauscht.", _option) );
			return 1;
		}
		else if ((matchParams(sCmd, "avg")
				  || matchParams(sCmd, "sum")
				  || matchParams(sCmd, "min")
				  || matchParams(sCmd, "max")
				  || matchParams(sCmd, "norm")
				  || matchParams(sCmd, "std")
				  || matchParams(sCmd, "prd")
				  || matchParams(sCmd, "num")
				  || matchParams(sCmd, "cnt")
				  || matchParams(sCmd, "and")
				  || matchParams(sCmd, "or")
				  || matchParams(sCmd, "xor")
				  || matchParams(sCmd, "med"))
				 && (matchParams(sCmd, "lines") || matchParams(sCmd, "cols")))
		{
			if (!_data.isValidCache() || !_data.getCacheCols(sCacheCmd, false))
				//throw NO_CACHED_DATA;
				throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, sCacheCmd, sCacheCmd);
			string sEvery = "";
			if (matchParams(sCmd, "every", '='))
			{
				value_type* v = 0;
				_parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "every", '=') + 5));
				v = _parser.Eval(nArgument);
				if (nArgument > 1)
				{
					sEvery = "every=" + toString((int)v[0]) + "," + toString((int)v[1]) + " ";
				}
				else
					sEvery = "every=" + toString((int)v[0]) + " ";
			}
			nPos = findCommand(sCmd, sCacheCmd).nPos;
			sArgument = extractCommandString(sCmd, findCommand(sCmd, sCacheCmd));
			sCommand = sArgument;
			if (matchParams(sCmd, "grid"))
				sArgument = "grid";
			else
				sArgument.clear();

			if (matchParams(sCmd, "avg"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[avg_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[avg_lines]", _data.avg(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[avg_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[avg_cols]", _data.avg(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "sum"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[sum_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[sum_lines]", _data.sum(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[sum_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[sum_cols]", _data.sum(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "min"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[min_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[min_lines]", _data.min(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[min_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[min_cols]", _data.min(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "max"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[max_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[max_lines]", _data.max(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[max_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[max_cols]", _data.max(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "norm"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[norm_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[norm_lines]", _data.norm(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[norm_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[norm_cols]", _data.norm(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "std"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[std_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[std_lines]", _data.std(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[std_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[std_cols]", _data.std(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "prd"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[prd_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[prd_lines]", _data.prd(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[prd_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[prd_cols]", _data.prd(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "num"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[num_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[num_lines]", _data.num(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[num_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[num_cols]", _data.num(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "cnt"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[cnt_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[cnt_lines]", _data.cnt(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[cnt_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[cnt_cols]", _data.cnt(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "med"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[med_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[med_lines]", _data.med(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[med_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[med_cols]", _data.med(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "and"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[and_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[and_lines]", _data.and_func(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[and_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[and_cols]", _data.and_func(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "or"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[or_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[or_lines]", _data.or_func(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[or_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[or_cols]", _data.or_func(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}
			else if (matchParams(sCmd, "xor"))
			{
				if (matchParams(sCmd, "lines"))
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[xor_lines]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[xor_lines]", _data.med(sCacheCmd, sArgument + "lines" + sEvery));
				}
				else
				{
					sCmd.replace(nPos, sCommand.length(), "_~" + sCacheCmd + "[xor_cols]");
					_parser.SetVectorVar("_~" + sCacheCmd + "[xor_cols]", _data.med(sCacheCmd, sArgument + "cols" + sEvery));
				}
			}

			return 0;
		}
		else if ((matchParams(sCmd, "avg")
				  || matchParams(sCmd, "sum")
				  || matchParams(sCmd, "min")
				  || matchParams(sCmd, "max")
				  || matchParams(sCmd, "norm")
				  || matchParams(sCmd, "std")
				  || matchParams(sCmd, "prd")
				  || matchParams(sCmd, "num")
				  || matchParams(sCmd, "cnt")
				  || matchParams(sCmd, "and")
				  || matchParams(sCmd, "or")
				  || matchParams(sCmd, "xor")
				  || matchParams(sCmd, "med"))
				)
		{
			if (!_data.isValidCache() || !_data.getCacheCols(sCacheCmd, false))
				//throw NO_CACHED_DATA;
				throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, sCacheCmd, sCacheCmd);
			nPos = findCommand(sCmd, sCacheCmd).nPos;
			sArgument = extractCommandString(sCmd, findCommand(sCmd, sCacheCmd));
			sCommand = sArgument;
			if (matchParams(sCmd, "grid") && _data.getCacheCols(sCacheCmd, false) < 3)
				//throw TOO_FEW_COLS;
				throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, sCacheCmd, sCacheCmd);
			else if (matchParams(sCmd, "grid"))
				nArgument = 2;
			else
				nArgument = 0;
			if (matchParams(sCmd, "avg"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.avg(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "sum"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.sum(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "min"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.min(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "max"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.max(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "norm"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.norm(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "std"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.std(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "prd"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.prd(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "num"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.num(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "cnt"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.cnt(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "med"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.med(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "and"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.and_func(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "or"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.or_func(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));
			else if (matchParams(sCmd, "xor"))
				sCmd.replace(nPos, sCommand.length(), toCmdString(_data.xor_func(sCacheCmd, 0, _data.getLines(sCacheCmd, false)-1, nArgument, _data.getCols(sCacheCmd)-1)));

			return 0;
		}
		else if (sCommand == "cache")
		{
			doc_Help("cache", _option);
			return 1;
		}
		else
			return 0;
	}
	else if (sCommand[0] == 'c' || sCommand[0] == 'a' || sCommand[0] == 'i')
	{
		if (sCommand == "clear")
		{
			if (matchParams(sCmd, "data") || sCmd.find(" data()", findCommand(sCmd).nPos) != string::npos)
			{
				if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
					remove_data(_data, _option, true);
				else
					remove_data(_data, _option);
			}
			else if (_data.matchCache(sCmd).length() || _data.containsTablesOrClusters(sCmd.substr(findCommand(sCmd).nPos)))
			{
				if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
					clear_cache(_data, _option, true);
				else
					clear_cache(_data, _option);
			}
			else if (matchParams(sCmd, "string") || sCmd.find(" string()", findCommand(sCmd).nPos) != string::npos)
			{
				if (_data.clearStringElements())
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_SUCCESS"), _option) );
					//NumeReKernel::print(LineBreak("|-> Zeichenketten wurden erfolgreich entfernt.", _option) );
				}
				else
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_EMPTY"), _option) );
					//NumeReKernel::print(LineBreak("|-> Es wurden keine Zeichenketten gefunden.", _option) );
				}
				return 1;
			}
			else
			{
				//throw TABLE_DOESNT_EXIST;
				throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);
			}
			return 1;
		}
		else if (sCommand == "credits" || sCommand == "about" || sCommand == "info")
		{
			BI_show_credits(_parser, _option);
			return 1;
		}
		else if (sCommand.substr(0, 6) == "ifndef" || sCommand == "ifndefined")
		{
			if (sCmd.find(' ') != string::npos)
			{
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (matchParams(sCmd, "comment", '='))
					addArgumentQuotes(sCmd, "comment");

				sArgument = sCmd.substr(sCmd.find(' '));
				StripSpaces(sArgument);
				if (!_functions.isDefined(sArgument.substr(0, sArgument.find(":="))))
                {
					if (_functions.defineFunc(sArgument))
                        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
                    else
                        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));
                }
			}
			else
				doc_Help("ifndef", _option);
			return 1;
		}
		else if (sCommand == "install")
		{
			if (!_script.isOpen())
			{
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				_script.setInstallProcedures();
				if (containsStrings(sCmd))
					BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option);
				else
					sArgument = sCmd.substr(findCommand(sCmd).nPos + 8);
				StripSpaces(sArgument);
				_script.openScript(sArgument);
			}
			return 1;
		}
		else if (sCommand == "copy")
		{
			if (_data.containsTablesOrClusters(sCmd) || sCmd.find("data(", 5) != string::npos)
			{
				if (CopyData(sCmd, _parser, _data, _option))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_COPYDATA_SUCCESS"), _option) );
				}
				else
					throw SyntaxError(SyntaxError::CANNOT_COPY_DATA, sCmd, SyntaxError::invalid_position);
			}
			else if ((matchParams(sCmd, "target", '=') || matchParams(sCmd, "t", '=')) && sCmd.length() > 5)
			{
				if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
					nArgument = 1;
				else
					nArgument = 0;
				if (copyFile(sCmd, _parser, _data, _option))
				{
					if (_option.getSystemPrintStatus())
					{
						if (nArgument)
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_COPYFILE_ALL_SUCCESS", sCmd), _option) );
						else
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_COPYFILE_SUCCESS", sCmd), _option) );
					}
				}
				else
				{
					//sErrorToken = sCmd;
					//throw CANNOT_COPY_FILE;
					throw SyntaxError(SyntaxError::CANNOT_COPY_FILE, sCmd, SyntaxError::invalid_position, sCmd);
				}
			}

			return 1;
		}
		else if (sCommand == "append")
		{
			if (matchParams(sCmd, "data") || matchParams(sCmd, "data", '='))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
				sCmd.replace(sCmd.find("data"), 4, "app");
				append_data(sCmd, _data, _option, _parser);
			}
			else if (sCmd.length() > findCommand(sCmd).nPos + 7 && sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7) != string::npos)
			{
				NumeReKernel::printPreFmt("\r");
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (sCmd[sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7)] != '"' && sCmd.find("string(") == string::npos)
				{
					sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7), 1, '"');
					if (matchParams(sCmd, "slice")
							|| matchParams(sCmd, "keepdim")
							|| matchParams(sCmd, "complete")
							|| matchParams(sCmd, "ignore")
							|| matchParams(sCmd, "i")
							|| matchParams(sCmd, "head")
							|| matchParams(sCmd, "h")
							|| matchParams(sCmd, "all"))
					{
						nArgument = string::npos;
						while (sCmd.find_last_of('-', nArgument) != string::npos
								&& sCmd.find_last_of('-', nArgument) > sCmd.find_first_of(' ', sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7)))
							nArgument = sCmd.find_last_of('-', nArgument) - 1;
						nArgument = sCmd.find_last_not_of(' ', nArgument);
						sCmd.insert(nArgument + 1, 1, '"');
					}
					else
					{
						sCmd.insert(sCmd.find_last_not_of(' ') + 1, 1, '"');
					}
				}
				sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 7), "-app=");
				append_data(sCmd, _data, _option, _parser);
				return 1;
			}
		}
		else if (sCommand == "audio")
		{
			if (!parser_writeAudio(sCmd, _parser, _data, _functions, _option))
				//throw CANNOT_SAVE_FILE;
				throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, SyntaxError::invalid_position);
			else if (_option.getSystemPrintStatus())
				NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_AUDIO_SUCCESS"), _option) );
			//NumeReKernel::print(LineBreak("|-> Die Audiodatei wurde erfolgreich erzeugt.", _option) );
			return 1;
		}
		else if (sCommand == "imread")
		{
			readImage(sCmd, _parser, _data, _option);
			return 1;
		}

		return 0;
	}
	else if (sCommand[0] == 'w')
	{
		if (sCommand == "write")
		{
			if (sCmd.length() > 6 && matchParams(sCmd, "file", '='))
			{
				writeToFile(sCmd, _parser, _data, _option);
			}
			else
				doc_Help("write", _option);
			return 1;
		}
		else if (sCommand == "workpath")
		{
			if (sCmd.length() <= 8)
				return 0;
			if (sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 8) == string::npos)
				return 0;
			if (sCmd.find('"') == string::npos)
			{
				sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 8), 1, '"');
				StripSpaces(sCmd);
				sCmd += '"';
			}
			while (sCmd.find('\\') != string::npos)
				sCmd[sCmd.find('\\')] = '/';

			if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
			{
				return 1;
			}
			FileSystem _fSys;
			_fSys.setTokens(_option.getTokenPaths());
			_fSys.setPath(sArgument, true, _data.getProgramPath());
			_option.setWorkPath(_fSys.getPath());
			if (_option.getSystemPrintStatus())
				NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );
			//NumeReKernel::print("|-> Dateipfad erfolgreich aktualisiert." );
			return 1;
		}
		else if (sCommand == "warn")
        {
            if (sCmd.length() > 5)
            {
                if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
                {
                    sArgument = sCmd.substr(sCmd.find("warn")+5);
                    _parser.SetExpr(sArgument);
                    int nResults = 0;
                    value_type* v = _parser.Eval(nResults);
                    if (nResults > 1)
                    {
                        sArgument = "{";
                        for (int i = 0; i < nResults; i++)
                        {
                            sArgument += " " + toString(v[i], _option) + ",";
                        }
                        sArgument.pop_back();
                        sArgument += "}";
                    }
                    else
                        sArgument = toString(v[0], _option);
                }

                NumeReKernel::issueWarning(sArgument);
            }
            else
                doc_Help("warn", _option);

            return 1;
        }
		return 0;
	}
	else if (sCommand[0] == 's')
	{
		if (sCommand == "stats")
		{
			sArgument = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
			if (matchParams(sCmd, "data") && _data.isValid())
				plugin_statistics(sArgument, _data, _out, _option, false, true);
			else if (_data.matchCache(sCmd).length() && _data.isValidCache())
				plugin_statistics(sArgument, _data, _out, _option, true, false);
			else
			{
				for (auto iter = mCaches.begin(); iter != mCaches.end(); ++iter)
				{
					if (matchParams(sCmd, iter->first) && _data.isValidCache())
					{
						plugin_statistics(sArgument, _data, _out, _option, true, false);
						break;
					}
					else if (sCmd.find(iter->first + "(") != string::npos
							 && (!sCmd.find(iter->first + "(")
								 || (sCmd.find(iter->first + "(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
					{
						//NumeReKernel::print(sCmd );
						Datafile _cache;
						_cache.setCacheStatus(true);
						_idx = parser_getIndices(sCmd, _parser, _data, _option);
						if (sCmd.find(iter->first + "(") != string::npos && iter->second != -1)
							_data.setCacheStatus(true);
						if (!isValidIndexSet(_idx))
							throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

                        if (_idx.row.isOpenEnd())
                            _idx.row.setRange(0, _data.getLines(iter->first, false)-1);

                        if (_idx.col.isOpenEnd())
                            _idx.col.setRange(0, _data.getCols(iter->first, false)-1);

                        _cache.setCacheSize(_idx.row.size(), _idx.col.size(), "cache");

                        for (size_t i = 0; i < _idx.row.size(); i++)
                        {
                            for (size_t j = 0; j < _idx.col.size(); j++)
                            {
                                if (!i)
                                {
                                    _cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_idx.col[j], iter->first));
                                }
                                if (_data.isValidEntry(_idx.row[i], _idx.col[j], iter->first))
                                    _cache.writeToCache(i, j, "cache", _data.getElement(_idx.row[i], _idx.col[j], iter->first));
                            }
                        }

						if (_data.containsStringVars(sCmd))
							_data.getStringValues(sCmd);

						if (matchParams(sCmd, "export", '='))
							addArgumentQuotes(sCmd, "export");

						//NumeReKernel::print(sCmd );
						_data.setCacheStatus(false);
						sArgument = "stats -cache " + sCmd.substr(getMatchingParenthesis(sCmd.substr(sCmd.find('('))) + 1 + sCmd.find('('));
						sArgument = BI_evalParamString(sArgument, _parser, _data, _option, _functions);
						plugin_statistics(sArgument, _cache, _out, _option, true, false);
						return 1;
					}
				}
			}

			return 1;
		}
		else if (sCommand == "stfa")
		{
			if (!parser_stfa(sCmd, sArgument, _parser, _data, _functions, _option))
			{
				doc_Help("stfa", _option);
			}
			else
				NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_STFA_SUCCESS", sArgument), _option) );
			return 1;
		}
		else if (sCommand == "spline")
		{
			if (!parser_spline(sCmd, _parser, _data, _functions, _option))
			{
				doc_Help("spline", _option);
			}

			return 1;
		}
		else if (sCommand == "save")
		{
			if (matchParams(sCmd, "define"))
			{
				_functions.save(_option);
				return 1;
			}
			else if (matchParams(sCmd, "set") || matchParams(sCmd, "settings"))
			{
				_option.save(_option.getExePath());
				return 1;
			}
			else //if (matchParams(sCmd, "data") || matchParams(sCmd, "data", '='))
			{
				for (auto iter = mCaches.begin(); iter != mCaches.end(); ++iter)
				{
					if (sCmd.find(iter->first + "(") != string::npos
							&& (!sCmd.find(iter->first + "(")
								|| (sCmd.find(iter->first + "(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
					{
						//NumeReKernel::print(sCmd );
						Datafile _cache;
						_cache.setTokens(_option.getTokenPaths());
						_cache.setPath(_data.getPath(), false, _option.getExePath());
						_cache.setCacheStatus(true);
						if (sCmd.find("()") != string::npos)
							sCmd.replace(sCmd.find("()"), 2, "(:,:)");
						_idx = parser_getIndices(sCmd, _parser, _data, _option);

						if (sCmd.find(iter->first + "(") != string::npos && iter->second != -1)
							_data.setCacheStatus(true);
						if (!isValidIndexSet(_idx))
							throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, SyntaxError::invalid_position);

                        if (_idx.row.isOpenEnd())
                            _idx.row.setRange(0, _data.getLines(iter->first, false)-1);

                        if (_idx.col.isOpenEnd())
                            _idx.col.setRange(0, _data.getCols(iter->first, false)-1);

                        _cache.setCacheSize(_idx.row.size(), _idx.col.size(), "cache");

                        if (iter->first != "cache")
                            _cache.renameCache("cache", (iter->second == -1 ? "copy_of_" + (iter->first) : (iter->first)), true);

                        for (size_t i = 0; i < _idx.row.size(); i++)
                        {
                            for (size_t j = 0; j < _idx.col.size(); j++)
                            {
                                if (!i)
                                {
                                    _cache.setHeadLineElement(j, iter->second == -1 ? "copy_of_" + (iter->first) : (iter->first), _data.getHeadLineElement(_idx.col[j], iter->first));
                                }
                                if (_data.isValidEntry(_idx.row[i], _idx.col[j], iter->first))
                                    _cache.writeToCache(i, j, iter->second == -1 ? "copy_of_" + (iter->first) : (iter->first), _data.getElement(_idx.row[i], _idx.col[j], iter->first));
                            }
                        }

						if (_data.containsStringVars(sCmd))
							_data.getStringValues(sCmd);
						if (matchParams(sCmd, "file", '='))
							addArgumentQuotes(sCmd, "file");

						//NumeReKernel::print(sCmd );
						_data.setCacheStatus(false);
						if (containsStrings(sCmd) && BI_parseStringArgs(sCmd.substr(matchParams(sCmd, "file", '=')), sArgument, _parser, _data, _option))
						{
							//NumeReKernel::print(sArgument );
							_cache.setPrefix(sArgument);
							if (_cache.saveFile(iter->second == -1 ? "copy_of_" + (iter->first) : (iter->first), sArgument))
							{
								if (_option.getSystemPrintStatus())
									NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _cache.getOutputFileName()), _option) );
								//NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _cache.getOutputFileName() + "\" gespeichert.", _option) );
								return 1;
							}
							else
								//throw CANNOT_SAVE_FILE;
								throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, sArgument, sArgument);
						}
						else
							_cache.setPrefix(iter->second == -1 ? "copy_of_" + (iter->first) : (iter->first));
						if (_cache.saveFile(iter->second == -1 ? "copy_of_" + (iter->first) : (iter->first), ""))
						{
							if (_option.getSystemPrintStatus())
								NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SAVEDATA_SUCCESS", _cache.getOutputFileName()), _option) );
							//NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich nach \"" + _cache.getOutputFileName() + "\" gespeichert.", _option) );
						}
						else
							//throw CANNOT_SAVE_FILE;
							throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sCmd, SyntaxError::invalid_position);
						return 1;
					}
				}
				return 1;
			}

			doc_Help("save", _option);
			return 1;
		}
		else if (sCommand == "set")
		{
			if (_data.containsStringVars(sCmd))
				_data.getStringValues(sCmd);
			if (matchParams(sCmd, "savepath") || matchParams(sCmd, "savepath", '='))
			{
				if (matchParams(sCmd, "savepath", '='))
				{
					addArgumentQuotes(sCmd, "savepath");
				}
				while (sCmd.find('\\') != string::npos)
					sCmd[sCmd.find('\\')] = '/';
				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );
					//NumeReKernel::print("|-> Einen Pfad eingeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				_out.setPath(sArgument, true, _out.getProgramPath());
				_option.setSavePath(_out.getPath());
				_data.setSavePath(_option.getSavePath());
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );
				//NumeReKernel::print("|-> Dateipfad erfolgreich aktualisiert." );
				NumeReKernel::modifiedSettings = true;
				return 1;
			}
			else if (matchParams(sCmd, "loadpath") || matchParams(sCmd, "loadpath", '='))
			{
				if (matchParams(sCmd, "loadpath", '='))
				{
					addArgumentQuotes(sCmd, "loadpath");
				}
				while (sCmd.find('\\') != string::npos)
					sCmd[sCmd.find('\\')] = '/';

				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );
					//NumeReKernel::print("|-> Einen Pfad eingeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				_data.setPath(sArgument, true, _data.getProgramPath());
				_option.setLoadPath(_data.getPath());
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );
				//NumeReKernel::print("|-> Dateipfad erfolgreich aktualisiert." );
				NumeReKernel::modifiedSettings = true;
				return 1;
			}
			else if (matchParams(sCmd, "workpath") || matchParams(sCmd, "workpath", '='))
			{
				if (matchParams(sCmd, "workpath", '='))
				{
					addArgumentQuotes(sCmd, "workpath");
				}
				while (sCmd.find('\\') != string::npos)
					sCmd[sCmd.find('\\')] = '/';

				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );
					//NumeReKernel::print("|-> Einen Pfad eingeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				FileSystem _fSys;
				_fSys.setTokens(_option.getTokenPaths());
				_fSys.setPath(sArgument, true, _data.getProgramPath());
				_option.setWorkPath(_fSys.getPath());
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );
				//NumeReKernel::print("|-> Dateipfad erfolgreich aktualisiert." );
				NumeReKernel::modifiedSettings = true;
				return 1;
			}
			else if (matchParams(sCmd, "viewer") || matchParams(sCmd, "viewer", '='))
			{
				if (matchParams(sCmd, "viewer", '='))
					addArgumentQuotes(sCmd, "viewer");
				while (sCmd.find('\\') != string::npos)
					sCmd[sCmd.find('\\')] = '/';
				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );
					//NumeReKernel::print("|-> Einen Pfad eingeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				_option.setViewerPath(sArgument);
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage(  _lang.get("BUILTIN_CHECKKEYWORD_SET_PROGRAM", "Imageviewer")) );
				//NumeReKernel::print("|-> Imageviewer erfolgreich deklariert." );
				return 1;
			}
			else if (matchParams(sCmd, "editor") || matchParams(sCmd, "editor", '='))
			{
				if (matchParams(sCmd, "editor", '='))
					addArgumentQuotes(sCmd, "editor");
				while (sCmd.find('\\') != string::npos)
					sCmd[sCmd.find('\\')] = '/';
				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );
					//NumeReKernel::print("|-> Einen Pfad eingeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				_option.setEditorPath(sArgument);
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage(  _lang.get("BUILTIN_CHECKKEYWORD_SET_PROGRAM", "Texteditor")) );
				//NumeReKernel::print("|-> Texteditor erfolgreich deklariert." );
				return 1;
			}
			else if (matchParams(sCmd, "scriptpath") || matchParams(sCmd, "scriptpath", '='))
			{
				if (matchParams(sCmd, "scriptpath", '='))
				{
					addArgumentQuotes(sCmd, "scriptpath");
				}
				while (sCmd.find('\\') != string::npos)
					sCmd[sCmd.find('\\')] = '/';
				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );
					//NumeReKernel::print("|-> Einen Pfad eingeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				_script.setPath(sArgument, true, _script.getProgramPath());
				_option.setScriptPath(_script.getPath());
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );
				//NumeReKernel::print("|-> Dateipfad erfolgreich aktualisiert." );
				NumeReKernel::modifiedSettings = true;
				return 1;
			}
			else if (matchParams(sCmd, "plotpath") || matchParams(sCmd, "plotpath", '='))
			{
				if (matchParams(sCmd, "plotpath", '='))
					addArgumentQuotes(sCmd, "plotpath");
				while (sCmd.find('\\') != string::npos)
					sCmd[sCmd.find('\\')] = '/';
				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );
					//NumeReKernel::print("|-> Einen Pfad eingeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				_pData.setPath(sArgument, true, _pData.getProgramPath());
				_option.setPlotOutputPath(_pData.getPath());
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );
				//NumeReKernel::print("|-> Dateipfad erfolgreich aktualisiert." );
				NumeReKernel::modifiedSettings = true;
				return 1;
			}
			else if (matchParams(sCmd, "procpath") || matchParams(sCmd, "procpath", '='))
			{
				if (matchParams(sCmd, "procpath", '='))
					addArgumentQuotes(sCmd, "procpath");
				while (sCmd.find('\\') != string::npos)
					sCmd[sCmd.find('\\')] = '/';
				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_GIVEPATH") + ":") );
					//NumeReKernel::print("|-> Einen Pfad eingeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				_option.setProcPath(sArgument);
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_PATH")) );
				//NumeReKernel::print("|-> Dateipfad erfolgreich aktualisiert." );
				NumeReKernel::modifiedSettings = true;
				return 1;
			}
			else if (matchParams(sCmd, "plotfont") || matchParams(sCmd, "plotfont", '='))
			{
				if (matchParams(sCmd, "plotfont", '='))
					addArgumentQuotes(sCmd, "plotfont");
				if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_ENTER_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_DEFAULTFONT"))) );
					//NumeReKernel::print("|-> Standardschriftart angeben:" );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
					}
					while (!sArgument.length());
				}
				if (sArgument[0] == '"')
					sArgument.erase(0, 1);
				if (sArgument[sArgument.length() - 1] == '"')
					sArgument.erase(sArgument.length() - 1);
				_option.setDefaultPlotFont(sArgument);
				_fontData.LoadFont(_option.getDefaultPlotFont().c_str(), _option.getExePath().c_str());
				_pData.setFont(_option.getDefaultPlotFont());
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_DEFAULTFONT"))) );
				//NumeReKernel::print("|-> Standardschriftart wurde erfolgreich eingestellt." );
				return 1;
			}
			else if (matchParams(sCmd, "precision") || matchParams(sCmd, "precision", '='))
			{
				if (!parser_parseCmdArg(sCmd, "precision", _parser, nArgument) || (!nArgument || nArgument > 14))
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_ENTER_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_PRECISION")) + " (1-14)") );
					//NumeReKernel::print(toSystemCodePage("|-> Präzision eingeben: (1-14)") );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
						nArgument = StrToInt(sArgument);
					}
					while (!nArgument || nArgument > 14);

				}
				_option.setprecision(nArgument);
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_PRECISION"))) );
				//NumeReKernel::print("|-> Präzision wurde erfolgreich eingestellt." );
				return 1;
			}
			else if (matchParams(sCmd, "faststart") || matchParams(sCmd, "faststart", '='))
			{
				if (!parser_parseCmdArg(sCmd, "faststart", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbFastStart();
					/*NumeReKernel::print("|-> Schneller Start? (1 = ja, 0 = nein)" );
					do
					{
					    NumeReKernel::print("|" );
					    NumeReKernel::print("|<- ";
					    getline(cin, sArgument);
					    nArgument = StrToInt(sArgument);
					}
					while (nArgument != 0 && nArgument != 1);*/
				}
				_option.setbFastStart((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_PARSERTEST", _lang.get("COMMON_WITHOUT")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_PARSERTEST", _lang.get("COMMON_WITH")), _option) );
				}
				return 1;
			}
			else if (matchParams(sCmd, "draftmode") || matchParams(sCmd, "draftmode", '='))
			{
				if (!parser_parseCmdArg(sCmd, "draftmode", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbUseDraftMode();
				}
				_option.setbUseDraftMode((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DRAFTMODE"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DRAFTMODE"), _lang.get("COMMON_INACTIVE")), _option) );
					/*if (nArgument)
					    NumeReKernel::print(LineBreak("|-> Entwurfsmodus aktiviert.", _option) );
					else
					    NumeReKernel::print(LineBreak("|-> Entwurfsmodus deaktiviert.", _option) );*/
				}
				return 1;
			}
			else if (matchParams(sCmd, "extendedfileinfo") || matchParams(sCmd, "extendedfileinfo", '='))
			{
				if (!parser_parseCmdArg(sCmd, "extendedfileinfo", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbShowExtendedFileInfo();
				}
				_option.setbExtendedFileInfo((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_EXTENDEDINFO"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_EXTENDEDINFO"), _lang.get("COMMON_INACTIVE")), _option) );
					/*if (nArgument)
					    NumeReKernel::print(LineBreak("|-> Erweiterte Dateiinformationen aktiviert.", _option) );
					else
					    NumeReKernel::print(LineBreak("|-> Erweiterte Dateiinformationen deaktiviert.", _option) );*/
				}
				return 1;
			}
			else if (matchParams(sCmd, "loademptycols") || matchParams(sCmd, "loademptycols", '='))
			{
				if (!parser_parseCmdArg(sCmd, "loademptycols", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbLoadEmptyCols();
				}
				_option.setbLoadEmptyCols((bool)nArgument);
				_data.setbLoadEmptyCols((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_LOADEMPTYCOLS"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_LOADEMPTYCOLS"), _lang.get("COMMON_INACTIVE")), _option) );
					/*if (nArgument)
					    NumeReKernel::print(LineBreak("|-> Laden leerer Spalten aktiviert.", _option) );
					else
					    NumeReKernel::print(LineBreak("|-> Laden leerer Spalten deaktiviert.", _option) );*/
				}
				return 1;
			}
			else if (matchParams(sCmd, "logfile") || matchParams(sCmd, "logfile", '='))
			{
				if (!parser_parseCmdArg(sCmd, "logfile", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbUseLogFile();
				}
				_option.setbUseLogFile((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_LOGFILE"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_LOGFILE"), _lang.get("COMMON_INACTIVE")), _option) );
					/*if (nArgument)
					    NumeReKernel::print(LineBreak("|-> Protokollierung aktiviert.", _option) );
					else
					    NumeReKernel::print(LineBreak("|-> Protokollierung deaktiviert.", _option) );*/
					NumeReKernel::print(LineBreak("|   (" + _lang.get("BUILTIN_CHECKKEYWORD_SET_RESTART_REQUIRED") + ")", _option) );
					//NumeReKernel::print(LineBreak("|   (Einstellung wird zum nächsten Start aktiv)", _option) );
				}
				return 1;
			}
			else if (matchParams(sCmd, "mode") || matchParams(sCmd, "mode", '='))
			{
				if (matchParams(sCmd, "mode", '='))
					addArgumentQuotes(sCmd, "mode");
				BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option);
				if (sArgument.length() && sArgument == "debug")
				{
					if (_option.getUseDebugger())
					{
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DEBUGGER"), _lang.get("COMMON_INACTIVE")), _option) );
						//NumeReKernel::print(LineBreak("|-> Debugger wurde deaktiviert.", _option) );
						_option.setDebbuger(false);
						NumeReKernel::getInstance()->getDebugger().setActive(false);
					}
					else
					{
						_option.setDebbuger(true);
						NumeReKernel::getInstance()->getDebugger().setActive(true);
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DEBUGGER"), _lang.get("COMMON_ACTIVE")), _option) );
						//NumeReKernel::print(LineBreak("|-> Debugger wurde aktiviert.", _option) );
					}
					return 1;
				}
				else if (sArgument.length() && sArgument == "developer")
				{
					if (_option.getbDebug())
					{
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_DEVMODE_INACTIVE"), _option) );
						_option.setbDebug(false);
						_parser.EnableDebugDump(false, false);
					}
					else
					{
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_DEVMODE_ACTIVE"), _option) );
						sArgument = "";
						do
						{
							NumeReKernel::printPreFmt("|\n|<- ");
							NumeReKernel::getline(sArgument);
						}
						while (!sArgument.length());
						if (sArgument == AutoVersion::STATUS)
						{
							_option.setbDebug(true);
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_DEVMODE_SUCCESS"), _option) );
							_parser.EnableDebugDump(true, true);
						}
						else
						{
							NumeReKernel::print(toSystemCodePage( _lang.get("COMMON_CANCEL")) );
						}
					}
					return 1;
				}
				return 1;
			}
			else if (matchParams(sCmd, "compact") || matchParams(sCmd, "compact", '='))
			{
				if (!parser_parseCmdArg(sCmd, "compact", _parser, nArgument) || !(nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbCompact();
				}
				_option.setbCompact((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_COMPACT"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_COMPACT"), _lang.get("COMMON_INACTIVE")), _option) );
				}
				return 1;
			}
			else if (matchParams(sCmd, "greeting") || matchParams(sCmd, "greeting", '='))
			{
				if (!parser_parseCmdArg(sCmd, "greeting", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbGreeting();
				}
				_option.setbGreeting((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_GREETING"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_GREETING"), _lang.get("COMMON_INACTIVE")), _option) );
				}
				return 1;
			}
			else if (matchParams(sCmd, "hints") || matchParams(sCmd, "hints", '='))
			{
				if (!parser_parseCmdArg(sCmd, "hints", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbShowHints();
				}
				_option.setbShowHints((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_HINTS"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_HINTS"), _lang.get("COMMON_INACTIVE")), _option) );
				}
				return 1;
			}
			else if (matchParams(sCmd, "useescinscripts") || matchParams(sCmd, "useescinscripts", '='))
			{
				if (!parser_parseCmdArg(sCmd, "useescinscripts", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbUseESCinScripts();
				}
				_option.setbUseESCinScripts((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_ESC_IN_SCRIPTS"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_ESC_IN_SCRIPTS"), _lang.get("COMMON_INACTIVE")), _option) );
				}
				return 1;
			}
			else if (matchParams(sCmd, "usecustomlang") || matchParams(sCmd, "usecustomlang", '='))
			{
				if (!parser_parseCmdArg(sCmd, "usecustomlang", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getUseCustomLanguageFiles();
				}
				_option.setUserLangFiles((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_CUSTOM_LANG"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_CUSTOM_LANG"), _lang.get("COMMON_INACTIVE")), _option) );
				}
				return 1;
			}
			else if (matchParams(sCmd, "externaldocwindow") || matchParams(sCmd, "externaldocwindow", '='))
			{
				if (!parser_parseCmdArg(sCmd, "externaldocwindow", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getUseExternalViewer();
				}
				_option.setExternalDocViewer((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DOC_VIEWER"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DOC_VIEWER"), _lang.get("COMMON_INACTIVE")), _option) );
				}
				return 1;
			}
			else if (matchParams(sCmd, "defcontrol") || matchParams(sCmd, "defcontrol", '='))
			{
				if (!parser_parseCmdArg(sCmd, "defcontrol", _parser, nArgument) || (nArgument != 0 && nArgument != 1))
				{
					nArgument = !_option.getbDefineAutoLoad();
				}
				_option.setbDefineAutoLoad((bool)nArgument);
				if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DEFCONTROL"), _lang.get("COMMON_ACTIVE")), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SET_MODE", _lang.get("BUILTIN_CHECKKEYWORD_DEFCONTROL"), _lang.get("COMMON_INACTIVE")), _option) );
				}
				if (_option.getbDefineAutoLoad() && !_functions.getDefinedFunctions() && BI_FileExists(_option.getExePath() + "\\functions.def"))
					_functions.load(_option);
				return 1;
			}
			else if (matchParams(sCmd, "autosave") || matchParams(sCmd, "autosave", '='))
			{
				if (!parser_parseCmdArg(sCmd, "autosave", _parser, nArgument) && !nArgument)
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_AUTOSAVE") + "? [sec]") );
					//NumeReKernel::print(toSystemCodePage("|-> Intervall für automatische Speicherung? [sec]") );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
						nArgument = StrToInt(sArgument);
					}
					while (!nArgument);
				}
				_option.setAutoSaveInterval(nArgument);
				if (_option.getSystemPrintStatus())
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_AUTOSAVE"))) );
				//NumeReKernel::print("|-> Automatische Speicherung alle " + _option.getAutoSaveInterval() + " sec." );
				return 1;
			}
			else if (matchParams(sCmd, "buffersize") || matchParams(sCmd, "buffersize", '='))
			{
				if (!parser_parseCmdArg(sCmd, "buffersize", _parser, nArgument) || nArgument < 300)
				{
					NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_BUFFERSIZE") + "? (>= 300)") );
					//NumeReKernel::print(toSystemCodePage("|-> Buffergröße? (Größer oder gleich 300)") );
					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
						nArgument = StrToInt(sArgument);
					}
					while (nArgument < 300);
				}
				_option.setWindowBufferSize(0, (unsigned)nArgument);
				//if (ResizeConsole(_option))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_BUFFERSIZE"))) );
					//NumeReKernel::print("|-> Buffer erfolgreich aktualisiert." );
				}
				/*else
				    throw;*/
				NumeReKernel::modifiedSettings = true;
				return 1;
			}
			else if (matchParams(sCmd, "windowsize"))
			{
				if (matchParams(sCmd, "x", '='))
				{
					parser_parseCmdArg(sCmd, "x", _parser, nArgument);
					//nArgument = matchParams(sCmd, "x", '=')+1;
					_option.setWindowSize((unsigned)nArgument, 0);
					_option.setWindowBufferSize(_option.getWindow() + 1, 0);
					NumeReKernel::nLINE_LENGTH = _option.getWindow();
					//NumeReKernel::print(nArgument );

				}
				if (matchParams(sCmd, "y", '='))
				{
					parser_parseCmdArg(sCmd, "y", _parser, nArgument);
					//nArgument = matchParams(sCmd, "y", '=')+1;
					_option.setWindowSize(0, (unsigned)nArgument);
					if (_option.getWindow(1) + 1 > _option.getBuffer(1))
						_option.setWindowBufferSize(0, _option.getWindow(1) + 1);
					//NumeReKernel::print(nArgument );
				}
				//if (ResizeConsole(_option))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_CHANGE_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_WINDOWSIZE"))) );
					//NumeReKernel::print(LineBreak("|-> Fenstergröße erfolgreich aktualisiert.", _option) );
				}
				//else
				//    throw; //NumeReKernel::print(LineBreak("|-> Ein Fehler ist aufgetreten!", _option) );
				return 1;
			}
			else if (matchParams(sCmd, "save"))
			{
				_option.save(_option.getExePath());
				return 1;
			}
			else
			{
				doc_Help("set", _option);
				return 1;
			}
		}
		else if (sCommand == "start")
		{
			if (_script.isOpen())
				//throw CANNOT_CALL_SCRIPT_RECURSIVELY;
				throw SyntaxError(SyntaxError::CANNOT_CALL_SCRIPT_RECURSIVELY, sCmd, SyntaxError::invalid_position, sCommand);
			if (matchParams(sCmd, "script") || matchParams(sCmd, "script", '='))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (matchParams(sCmd, "install"))
					_script.setInstallProcedures();
				if (matchParams(sCmd, "script", '='))
					addArgumentQuotes(sCmd, "script");
				if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
					_script.openScript(sArgument);
				else
					_script.openScript();
			}
			else
			{
				if (!_script.isOpen())
				{
					if (_data.containsStringVars(sCmd))
						_data.getStringValues(sCmd);
					if (containsStrings(sCmd))
						BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option);
					else
						sArgument = sCmd.substr(findCommand(sCmd).nPos + 6);
					StripSpaces(sArgument);
					if (!sArgument.length())
					{
						if (_script.getScriptFileName().length())
							_script.openScript();
						else
						{
							//sErrorToken = "["+_lang.get("BUILTIN_CHECKKEYWORD_START_ERRORTOKEN")+"]";
							//throw SCRIPT_NOT_EXIST;
							throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sCmd, sArgument, "[" + _lang.get("BUILTIN_CHECKKEYWORD_START_ERRORTOKEN") + "]");
						}
						return 1;
					}
					_script.openScript(sArgument);
				}
			}
			return 1;
		}
		else if (sCommand == "script")
		{
			if (matchParams(sCmd, "load") || matchParams(sCmd, "load", '='))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
				if (!_script.isOpen())
				{
					if (_data.containsStringVars(sCmd))
						_data.getStringValues(sCmd);
					if (matchParams(sCmd, "load", '='))
						addArgumentQuotes(sCmd, "load");
					if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
					{
						do
						{
							NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_ENTER_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_SCRIPTNAME"))) );
							//NumeReKernel::print("|-> Dateiname des Scripts angeben:" );
							NumeReKernel::printPreFmt("|<- ");
							NumeReKernel::getline(sArgument);
						}
						while (!sArgument.length());
					}
					_script.setScriptFileName(sArgument);
					if (BI_FileExists(_script.getScriptFileName()))
					{
						if (_option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SCRIPTLOAD_SUCCESS", _script.getScriptFileName()), _option) );
						//NumeReKernel::print(LineBreak("|-> Script \"" + _script.getScriptFileName() + "\" wurde erfolgreich geladen!", _option) );
					}
					else
					{
						string sErrorToken = _script.getScriptFileName();
						sArgument = "";
						_script.setScriptFileName(sArgument);
						//throw SCRIPT_NOT_EXIST;
						throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sCmd, sErrorToken, sErrorToken);
					}
				}
				else
					//throw CANNOT_CALL_SCRIPT_RECURSIVELY;
					throw SyntaxError(SyntaxError::CANNOT_CALL_SCRIPT_RECURSIVELY, sCmd, SyntaxError::invalid_position, sCommand);
			}
			else if (matchParams(sCmd, "start") || matchParams(sCmd, "start", '='))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
				if (_script.isOpen())
					//throw CANNOT_CALL_SCRIPT_RECURSIVELY;
					throw SyntaxError(SyntaxError::CANNOT_CALL_SCRIPT_RECURSIVELY, sCmd, SyntaxError::invalid_position, sCommand);
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (matchParams(sCmd, "install"))
					_script.setInstallProcedures();
				if (matchParams(sCmd, "start", '='))
					addArgumentQuotes(sCmd, "start");
				if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
					_script.openScript(sArgument);
				else
					_script.openScript();
			}
			else
				doc_Help("script", _option);
			return 1;
		}
		else if (sCommand.substr(0, 4) == "show" || sCommand == "showf")
		{
            return showDataObject(sCmd);
		}
		else if (sCommand == "search")
		{
			if (sCmd.length() > 8 && sCmd.find("-") != string::npos)
				doc_SearchFct(sCmd.substr(sCmd.find("-") + 1), _option);
			else if (sCmd.length() > 8)
				doc_SearchFct(sCmd.substr(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + sCommand.length())), _option);
			else
			{
				NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_FIND_CANNOT_READ"), _option) );
				//NumeReKernel::print("|-> Kann den Begriff nicht identifizieren!" );
				doc_Help("find", _option);
			}
			return 1;
		}
		else if (sCommand == "smooth")
		{
			// smooth cache(i1:i2,j1:j2) -order=1
			if (matchParams(sCmd, "order", '='))
			{
				nArgument = matchParams(sCmd, "order", '=') + 5;
				if (_data.containsTablesOrClusters(sCmd.substr(nArgument)) || sCmd.substr(nArgument).find("data(") != string::npos)
				{
					sArgument = sCmd.substr(nArgument);
					getDataElements(sArgument, _parser, _data, _option);
					if (sArgument.find("{") != string::npos)
						parser_VectorToExpr(sArgument, _option);
					sCmd = sCmd.substr(0, nArgument) + sArgument;
				}
				_parser.SetExpr(getArgAtPos(sCmd, nArgument));
				nArgument = (int)_parser.Eval();
			}
			else
				nArgument = 1;
			if (!_data.containsTablesOrClusters(sCmd))
				return 1;
			for (auto iter = mCaches.begin(); iter != mCaches.end(); ++iter)
			{
				if (sCmd.find(iter->first + "(") != string::npos
						&& (!sCmd.find(iter->first + "(")
							|| (sCmd.find(iter->first + "(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first + "(") - 1, (iter->first).length() + 2))))
						&& iter->second >= 0)
				{
					sArgument = sCmd.substr(sCmd.find(iter->first + "("), (iter->first).length() + getMatchingParenthesis(sCmd.substr(sCmd.find(iter->first + "(") + (iter->first).length())) + 1);
					break;
				}
			}
			//NumeReKernel::print(sArgument );
			_idx = parser_getIndices(sArgument, _parser, _data, _option);

			if (!isValidIndexSet(_idx))
				throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, sArgument);

			if (_idx.row.isOpenEnd())
				_idx.row.setRange(0, _data.getLines(sArgument.substr(0, sArgument.find('(')), false)-1);

			if (_idx.col.isOpenEnd())
				_idx.col.setRange(0, _data.getCols(sArgument.substr(0, sArgument.find('(')))-1);

			if (matchParams(sCmd, "grid"))
			{
				if (_data.smooth(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, nArgument, Cache::GRID))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", "\"" + sArgument.substr(0, sArgument.find('(')) + "\""), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, sArgument, sArgument);
				}
			}
			else if (!matchParams(sCmd, "lines") && !matchParams(sCmd, "cols"))
			{
				if (_data.smooth(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, nArgument, Cache::ALL))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", "\"" + sArgument.substr(0, sArgument.find('(')) + "\""), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, sArgument, sArgument);
				}
			}
			else if (matchParams(sCmd, "lines"))
			{
				if (_data.smooth(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, nArgument, Cache::LINES))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", _lang.get("COMMON_LINES")), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, sArgument, sArgument);
				}
			}
			else if (matchParams(sCmd, "cols"))
			{
				if (_data.smooth(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, nArgument, Cache::COLS))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SMOOTH", _lang.get("COMMON_COLS")), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, sCmd, sArgument, sArgument);
				}
			}
			return 1;
		}
		else if (sCommand == "string" && sCmd.substr(0, 7) != "string(")
		{
			if (matchParams(sCmd, "clear"))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
				if (_data.clearStringElements())
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_SUCCESS"), _option) );
					//NumeReKernel::print(LineBreak("|-> Zeichenketten wurden erfolgreich entfernt.", _option) );
				}
				else
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_CLEARSTRINGS_EMPTY"), _option) );
					//NumeReKernel::print(LineBreak("|-> Es wurden keine Zeichenketten gefunden.", _option) );
				}
				return 1;
			}
		}
		else if (sCommand == "swap")
		{
			return swapCaches(sCmd, _data, _parser, _option, _functions);
		}

		return 0;
	}
	else if (sCommand[0] == 'h' || sCommand[0] == 'm')
	{
		if (sCommand.substr(0, 4) == "hist")
		{
			sArgument = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
			if (matchParams(sCmd, "data") && _data.isValid())
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRACATED"));
				plugin_histogram(sArgument, _data, _data, _out, _option, _pData, false, true);
			}
			else if (matchParams(sCmd, "cache") && _data.isValidCache())
            {
                // DEPRECATED: Declared at v1.1.2rc1
                NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
				plugin_histogram(sArgument, _data, _data, _out, _option, _pData, true, false);
            }
			else
			{
				for (auto iter = mCaches.begin(); iter != mCaches.end(); ++iter)
				{
					if (matchParams(sCmd, iter->first) && _data.isValidCache())
					{
						plugin_histogram(sArgument, _data, _data, _out, _option, _pData, true, false);
						break;
					}
					else if (sCmd.find(iter->first + "(") != string::npos
							 && (!sCmd.find(iter->first + "(")
								 || (sCmd.find(iter->first + "(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
					{
						//NumeReKernel::print(sCmd );
						Datafile _cache;
						_cache.setCacheStatus(true);
						_idx = parser_getIndices(sCmd, _parser, _data, _option);

						if (sCmd.find(iter->first + "(") != string::npos && iter->second != -1)
							_data.setCacheStatus(true);

						if (!isValidIndexSet(_idx))
							throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, iter->first + "(", iter->first);

                        if (_idx.row.isOpenEnd())
                            _idx.row.setRange(0, _data.getLines(iter->first, false)-1);

                        if (_idx.col.isOpenEnd())
                            _idx.col.setRange(0, _data.getCols(iter->first, false)-1);

                        _cache.setCacheSize(_idx.row.size(), _idx.col.size(), "cache");

                        for (unsigned int i = 0; i < _idx.row.size(); i++)
                        {
                            for (unsigned int j = 0; j < _idx.col.size(); j++)
                            {
                                if (!i)
                                {
                                    _cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_idx.col[j], iter->first));
                                }
                                if (_data.isValidEntry(_idx.row[i], _idx.col[j], iter->first))
                                    _cache.writeToCache(i, j, "cache", _data.getElement(_idx.row[i], _idx.col[j], iter->first));
                            }
                        }

						if (_data.containsStringVars(sCmd))
							_data.getStringValues(sCmd);
						if (matchParams(sCmd, "export", '='))
							addArgumentQuotes(sCmd, "export");

						//NumeReKernel::print(sCmd );
						_data.setCacheStatus(false);
						if (sCommand == "hist2d")
							sArgument = "hist2d -cache c=1:inf " + sCmd.substr(getMatchingParenthesis(sCmd.substr(sCmd.find('('))) + 1 + sCmd.find('('));
						else
							sArgument = "hist -cache c=1:inf " + sCmd.substr(getMatchingParenthesis(sCmd.substr(sCmd.find('('))) + 1 + sCmd.find('('));
						sArgument = BI_evalParamString(sArgument, _parser, _data, _option, _functions);
						plugin_histogram(sArgument, _cache, _data, _out, _option, _pData, true, false);
						break;
					}
				}
			}
			return 1;
		}
		else if (sCommand == "help" || sCommand == "man")
		{
			if (findCommand(sCmd).nPos + findCommand(sCmd).sString.length() < sCmd.length())
				doc_Help(sCmd.substr(findCommand(sCmd).nPos + findCommand(sCmd).sString.length()), _option);
			else
				doc_Help("brief", _option);
			/*if (sCmd.length() > 5 && sCmd.find("-") != string::npos)
			{
			    doc_Help(sCmd.substr(sCmd.find('-')+1), _option);
			}
			else if (sCmd.length() > 5)
			{
			    doc_Help(getArgAtPos(sCmd, findCommand(sCmd).nPos+sCommand.length()), _option);
			}
			else
			    doc_Help("brief", _option);*/
			return 1;
		}
		else if (sCommand == "move")
		{
			if (sCmd.length() > 5)
			{
				if (_data.containsTablesOrClusters(sCmd) && (matchParams(sCmd, "target", '=') || matchParams(sCmd, "t", '=')))
				{
					if (moveData(sCmd, _parser, _data, _option))
					{
						if (_option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_MOVEDATA_SUCCESS"), _option) );
					}
					else
						throw SyntaxError(SyntaxError::CANNOT_MOVE_DATA, sCmd, SyntaxError::invalid_position);
				}
				else
				{
					if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
						nArgument = 1;
					else
						nArgument = 0;
					if (moveFile(sCmd, _parser, _data, _option))
					{
						if (_option.getSystemPrintStatus())
						{
							if (nArgument)
								NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CHECKKEYWORD_MOVEFILE_ALL_SUCCESS", sCmd), _option) );
							else
								NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CHECKKEYWORD_MOVEFILE_SUCCESS", sCmd), _option) );
						}
					}
					else
					{
						//sErrorToken = sCmd;
						throw SyntaxError(SyntaxError::CANNOT_MOVE_FILE, sCmd, SyntaxError::invalid_position, sCmd);
					}
				}
				return 1;
			}
		}
		else if (sCommand == "hline")
		{
			if (matchParams(sCmd, "single"))
				make_hline(-2);
			else
				make_hline();
			return 1;
		}
		else if (sCommand == "matop" || sCommand == "mtrxop")
		{
			parser_matrixOperations(sCmd, _parser, _data, _functions, _option);
			return 1;
		}

		return 0;
	}
	else if (sCommand[0] == 'r')
	{
		if (sCommand == "random")
		{
			if (matchParams(sCmd, "o"))
			{
				plugin_random(sCmd, _data, _out, _option, true);
			}
			else
				plugin_random(sCmd, _data, _out, _option);
			return 1;
		}
		else if (sCommand.substr(0, 5) == "redef" || sCommand == "redefine")
		{
			if (sCmd.length() > sCommand.length() + 1)
			{
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);

				if (matchParams(sCmd, "comment", '='))
					addArgumentQuotes(sCmd, "comment");

				if (_functions.defineFunc(sCmd.substr(sCmd.find(' ') + 1), true))
					NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
                else
                    NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));
			}
			else
				doc_Help("define", _option);
			return 1;
		}
		else if (sCommand == "resample")
		{
			//NumeReKernel::print(sCommand );
			if (!_data.containsTablesOrClusters(sCmd))
				return 1;
			for (auto iter = mCaches.begin(); iter != mCaches.end(); ++iter)
			{
				if (sCmd.find(iter->first + "(") != string::npos
						&& (!sCmd.find(iter->first + "(")
							|| (sCmd.find(iter->first + "(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
				{
					sArgument = sCmd.substr(sCmd.find(iter->first + "("), (iter->first).length() + getMatchingParenthesis(sCmd.substr(sCmd.find(iter->first + "(") + (iter->first).length())) + 1);
					break;
				}
			}
			if (matchParams(sCmd, "samples", '='))
			{
				nArgument = matchParams(sCmd, "samples", '=') + 7;
				if (_data.containsTablesOrClusters(getArgAtPos(sCmd, nArgument)) || getArgAtPos(sCmd, nArgument).find("data(") != string::npos)
				{
					sArgument = getArgAtPos(sCmd, nArgument);
					//NumeReKernel::print("get data element (BI)" );
					getDataElements(sArgument, _parser, _data, _option);
					if (sArgument.find("{") != string::npos)
						parser_VectorToExpr(sArgument, _option);
					sCmd.replace(nArgument, getArgAtPos(sCmd, nArgument).length(), sArgument);
					//sCmd = sCmd.substr(0,nArgument) + sArgument;
				}
				_parser.SetExpr(getArgAtPos(sCmd, nArgument));
				nArgument = (int)_parser.Eval();
			}
			else
				nArgument = _data.getCacheLines(sArgument.substr(0, sArgument.find('(')), false);


			_idx = parser_getIndices(sArgument, _parser, _data, _option);

			if (!isValidIndexSet(_idx))
				throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, sArgument, sArgument);

			if (_idx.row.isOpenEnd())
				_idx.row.setRange(0, _data.getLines(sArgument.substr(0, sArgument.find('(')), false)-1);

			if (_idx.col.isOpenEnd())
				_idx.col.setRange(0, _data.getCols(sArgument.substr(0, sArgument.find('(')))-1);

			if (matchParams(sCmd, "grid"))
			{
				if (_data.resample(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, nArgument, Cache::GRID))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", "\"" + sArgument.substr(0, sArgument.find('(')) + "\""), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, sArgument.substr(0, sArgument.find('(')), sArgument.substr(0, sArgument.find('(') - 1));
				}
			}
			else if (!matchParams(sCmd, "lines") && !matchParams(sCmd, "cols"))
			{
				if (_data.resample(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, nArgument, Cache::ALL))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", "\"" + sArgument.substr(0, sArgument.find('(')) + "\""), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, sArgument.substr(0, sArgument.find('(')), sArgument.substr(0, sArgument.find('(') - 1));
				}
			}
			else if (matchParams(sCmd, "cols"))
			{
				if (_data.resample(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, nArgument, Cache::COLS))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", _lang.get("COMMON_COLS")), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, sArgument.substr(0, sArgument.find('(')), sArgument.substr(0, sArgument.find('(') - 1));
				}
			}
			else if (matchParams(sCmd, "lines"))
			{
				if (_data.resample(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, nArgument, Cache::LINES))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RESAMPLE", _lang.get("COMMON_LINES")), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, sCmd, sArgument.substr(0, sArgument.find('(')), sArgument.substr(0, sArgument.find('(') - 1));
				}
			}
			return 1;
		}
		else if (sCommand == "remove")
		{
			if (_data.containsTablesOrClusters(sCmd))
			{
				while (_data.containsTablesOrClusters(sCmd))
				{
					for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
					{
						if (sCmd.find(iter->first + "()") != string::npos && iter->first != "cache")
						{
							string sObj = iter->first;
							if (_data.deleteCache(iter->first))
							{
								if (sArgument.length())
									sArgument += ", ";
								sArgument += "\"" + sObj + "()\"";
								break;
							}
						}
					}
				}
				if (sArgument.length() && _option.getSystemPrintStatus())
					NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_REMOVECACHE", sArgument), _option) );
				//NumeReKernel::print(LineBreak("|-> Cache(s) " + sArgument + " wurde(n) erfolgreich entfernt.", _option) );
			}
			else if (sCmd.length() > 7)
			{
				if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
					nArgument = 1;
				else
					nArgument = 0;
				if (!removeFile(sCmd, _parser, _data, _option))
				{
					//sErrorToken = sCmd;
					throw SyntaxError(SyntaxError::CANNOT_REMOVE_FILE, sCmd, SyntaxError::invalid_position, sCmd);
					//NumeReKernel::print(LineBreak("|-> Die Datei \"" + sCmd + "\" konnte nicht gelöscht werden oder existiert nicht!", _option) );
				}
				else if (_option.getSystemPrintStatus())
				{
					if (nArgument)
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_REMOVE_ALL_FILE"), _option) );
					else
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_REMOVE_FILE"), _option) );
				}
			}
			else
			{
				throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);
				//NumeReKernel::print(LineBreak("|-> FEHLER: Keine zu löschende Datei angegeben!", _option) );
			}
			return 1;
		}
		else if (sCommand == "rename") //rename CACHE=NEWNAME
		{
			return renameCaches(sCmd, _data, _parser, _option, _functions);
		}
		else if (sCommand == "reload")
		{
			if (matchParams(sCmd, "data") || matchParams(sCmd, "data", '='))
			{
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (matchParams(sCmd, "data", '='))
					addArgumentQuotes(sCmd, "data");
				if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
						_data.setbLoadEmptyColsInNextFile(true);
					if (_data.isValid())
					{
						_data.removeData(false);
						if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
						{
							if (matchParams(sCmd, "head", '='))
								nArgument = matchParams(sCmd, "head", '=') + 4;
							else
								nArgument = matchParams(sCmd, "h", '=') + 1;
							nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
							_data.openFile(sArgument, _option, false, true, nArgument);
						}
						else
							_data.openFile(sArgument, _option);
						if (_data.isValid() && _option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_FILE_SUCCESS", _data.getDataFileName("data")), _option) );
						//NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich aktualisiert.", _option) );
					}
					else
						load_data(_data, _option, _parser, sArgument);
				}
				else if (_data.isValid())
				{
					if ((_data.getDataFileName("data") == "Merged Data" || _data.getDataFileName("data") == "Pasted Data") && !matchParams(sCmd, "data", '='))
						throw SyntaxError(SyntaxError::CANNOT_RELOAD_DATA, sCmd, SyntaxError::invalid_position);
					if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
						_data.setbLoadEmptyColsInNextFile(true);
					sArgument = _data.getDataFileName("data");
					_data.removeData(false);
					_data.openFile(sArgument, _option, false, true);
					if (_data.isValid() && _option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RELOAD_SUCCESS"), _option) );
					//NumeReKernel::print(LineBreak("|-> Daten wurden erfolgreich aktualisiert.", _option) );
				}
				else
					load_data(_data, _option, _parser);
			}
			return 1;
		}
		else if (sCommand == "retoque" || sCommand == "retouch")
		{
			if (!_data.containsTablesOrClusters(sCmd))
				return 1;
			for (auto iter = mCaches.begin(); iter != mCaches.end(); ++iter)
			{
				if (sCmd.find(iter->first + "(") != string::npos
						&& (!sCmd.find(iter->first + "(")
							|| (sCmd.find(iter->first + "(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first + "(") - 1, (iter->first).length() + 2))))
						&& iter->second >= 0)
				{
					sArgument = sCmd.substr(sCmd.find(iter->first + "("), (iter->first).length() + getMatchingParenthesis(sCmd.substr(sCmd.find(iter->first + "(") + (iter->first).length())) + 1);
					break;
				}
			}
			// DEPRECATED: Declared at v1.1.2rc1
			if (sCommand == "retoque")
                NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));

			_idx = parser_getIndices(sArgument, _parser, _data, _option);

			if (!isValidIndexSet(_idx))
				throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, sArgument, sArgument);

			if (_idx.row.isOpenEnd())
				_idx.row.setRange(0, _data.getLines(sArgument.substr(0, sArgument.find('(')), false)-1);

			if (_idx.col.isOpenEnd())
				_idx.col.setRange(0, _data.getCols(sArgument.substr(0, sArgument.find('(')))-1);

			if (matchParams(sCmd, "grid"))
			{
				if (_data.retoque(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, Cache::GRID))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", "\"" + sArgument.substr(0, sArgument.find('(')) + "\""), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, sArgument.substr(0, sArgument.find('(')), sArgument.substr(0, sArgument.find('(') - 1));
				}
			}
			else if (!matchParams(sCmd, "lines") && !matchParams(sCmd, "cols"))
			{
				if (_data.retoque(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, Cache::ALL))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", "\"" + sArgument.substr(0, sArgument.find('(')) + "\""), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, sArgument.substr(0, sArgument.find('(')), sArgument.substr(0, sArgument.find('(') - 1));
				}
			}
			else if (matchParams(sCmd, "lines"))
			{
				if (_data.retoque(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, Cache::LINES))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", _lang.get("COMMON_LINES")), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, sArgument.substr(0, sArgument.find('(')), sArgument.substr(0, sArgument.find('(') - 1));
				}
			}
			else if (matchParams(sCmd, "cols"))
			{
				if (_data.retoque(sArgument.substr(0, sArgument.find('(')), _idx.row, _idx.col, Cache::COLS))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RETOQUE", _lang.get("COMMON_COLS")), _option) );
				}
				else
				{
					throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, sArgument.substr(0, sArgument.find('(')), sArgument.substr(0, sArgument.find('(') - 1));
				}
			}
			return 1;
		}
		else if (sCommand == "regularize")
		{
			if (!parser_regularize(sCmd, _parser, _data, _functions, _option))
				throw SyntaxError(SyntaxError::CANNOT_RETOQUE_CACHE, sCmd, SyntaxError::invalid_position);
			else if (_option.getSystemPrintStatus())
				NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_REGULARIZE"), _option) );
			//NumeReKernel::print(LineBreak("|-> Der gewünschte Cache wurde erfolgreich regularisiert.", _option) );
			return 1;
		}


		return 0;
	}
	else if (sCommand[0] == 'd')
	{
		//NumeReKernel::print("define" );
		if (sCommand == "define")
		{
			if (sCmd.length() > 8)
			{
				_functions.setCacheList(_data.getCacheNames());
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (matchParams(sCmd, "comment", '='))
					addArgumentQuotes(sCmd, "comment");
				if (matchParams(sCmd, "save"))
				{
					_functions.save(_option);
					return 1;
				}
				else if (matchParams(sCmd, "load"))
				{
					if (BI_FileExists(_option.getExePath() + "\\functions.def"))
						_functions.load(_option);
					else
						NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_DEF_EMPTY")) );
					return 1;
				}
				else
				{
				    if (_functions.defineFunc(sCmd.substr(7)))
                        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
                    else
                        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));
				}
			}
			else
				doc_Help("define", _option);
			return 1;
		}
		else if (sCommand.substr(0, 3) == "del" || sCommand == "delete")
		{
			if (_data.containsTablesOrClusters(sCmd))
			{
				if (matchParams(sCmd, "ignore") || matchParams(sCmd, "i"))
                {
					if (deleteCacheEntry(sCmd, _parser, _data, _option))
					{
						if (_option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_DELETE_SUCCESS"), _option) );
					}
					else
						throw SyntaxError(SyntaxError::CANNOT_DELETE_ELEMENTS, sCmd, SyntaxError::invalid_position);
                }
				else
				{
					NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_DELETE_CONFIRM"), _option) );

					do
					{
						NumeReKernel::printPreFmt("|\n|<- ");
						NumeReKernel::getline(sArgument);
						StripSpaces(sArgument);
					}
					while (!sArgument.length());

					if (sArgument.substr(0, 1) == _lang.YES())
						deleteCacheEntry(sCmd, _parser, _data, _option);
					else
					{
						NumeReKernel::print(_lang.get("COMMON_CANCEL") );
						return 1;
					}
				}

				if (!_data.isValidCache())
				{
					sArgument = _option.getSavePath() + "/cache.tmp";

					if (BI_FileExists(sArgument))
					{
						remove(sArgument.c_str());
					}
				}
			}
			else if (sCmd.find("string()") != string::npos || sCmd.find("string(:)") != string::npos)
			{
				if (_data.removeStringElements(0))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_SUCCESS", "1"), _option) );
					//NumeReKernel::print(LineBreak("|-> Zeichenketten wurden erfolgreich entfernt.", _option) );
				}
				else
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_EMPTY", "1"), _option) );
					//NumeReKernel::print(LineBreak("|-> Es wurden keine Zeichenketten gefunden.", _option) );
				}
				return 1;
			}
			else if (sCmd.find(" string(", findCommand(sCmd).nPos) != string::npos)
			{
				_parser.SetExpr(sCmd.substr(sCmd.find(" string(", findCommand(sCmd).nPos) + 8, getMatchingParenthesis(sCmd.substr(sCmd.find(" string(", findCommand(sCmd).nPos) + 7)) - 1));
				nArgument = (int)_parser.Eval() - 1;
				if (_data.removeStringElements(nArgument))
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_SUCCESS", toString(nArgument + 1)), _option) );
					//NumeReKernel::print(LineBreak("|-> Zeichenketten wurden erfolgreich entfernt.", _option) );
				}
				else
				{
					if (_option.getSystemPrintStatus())
						NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_DELETESTRINGS_EMPTY", toString(nArgument + 1)), _option) );
					//NumeReKernel::print(LineBreak("|-> Es wurden keine Zeichenketten gefunden.", _option) );
				}
				return 1;

			}
			else
				doc_Help("cache", _option);
			return 1;
		}
		else if (sCommand == "datagrid")
		{
			sArgument = "grid";
			if (!parser_datagrid(sCmd, sArgument, _parser, _data, _functions, _option))
				doc_Help("datagrid", _option);
			else if (_option.getSystemPrintStatus())
			{
				NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_DATAGRID_SUCCESS", sArgument), _option) );
				//NumeReKernel::print(LineBreak("|-> Das Datengitter wurde erfolgreich in \"grid()\" erzeugt.", _option) );
			}
			return 1;
		}

		return 0;
	}
	else if (sCommand[0] == 'l')
	{
		if (sCommand == "list")
		{
			if (matchParams(sCmd, "files") || (matchParams(sCmd, "files", '=')))
			{
				BI_ListFiles(sCmd, _option);
				return 1;
			}
			else if (matchParams(sCmd, "var") && bParserActive)
			{
				parser_ListVar(_parser, _option, _data);
				return 1;
			}
			else if (matchParams(sCmd, "const") && bParserActive)
			{
				parser_ListConst(_parser, _option);
				return 1;
			}
			else if ((matchParams(sCmd, "func") || matchParams(sCmd, "func", '=')) && bParserActive)
			{
				if (matchParams(sCmd, "func", '='))
					sArgument = getArgAtPos(sCmd, matchParams(sCmd, "func", '=') + 4);
				else
				{
					parser_ListFunc(_option, "all");
					return 1;
				}
				if (sArgument == "num" || sArgument == "numerical")
					parser_ListFunc(_option, "num");
				else if (sArgument == "mat" || sArgument == "matrix" || sArgument == "vec" || sArgument == "vector")
					parser_ListFunc(_option, "mat");
				else if (sArgument == "string")
					parser_ListFunc(_option, "string");
				else if (sArgument == "trigonometric")
					parser_ListFunc(_option, "trigonometric");
				else if (sArgument == "hyperbolic")
					parser_ListFunc(_option, "hyperbolic");
				else if (sArgument == "logarithmic")
					parser_ListFunc(_option, "logarithmic");
				else if (sArgument == "polynomial")
					parser_ListFunc(_option, "polynomial");
				else if (sArgument == "stats" || sArgument == "statistical")
					parser_ListFunc(_option, "stats");
				else if (sArgument == "angular")
					parser_ListFunc(_option, "angular");
				else if (sArgument == "physics" || sArgument == "physical")
					parser_ListFunc(_option, "physics");
				else if (sArgument == "logic" || sArgument == "logical")
					parser_ListFunc(_option, "logic");
				else if (sArgument == "time")
					parser_ListFunc(_option, "time");
				else if (sArgument == "distrib")
					parser_ListFunc(_option, "distrib");
				else if (sArgument == "random")
					parser_ListFunc(_option, "random");
				else if (sArgument == "coords")
					parser_ListFunc(_option, "coords");
				else if (sArgument == "draw")
					parser_ListFunc(_option, "draw");
				else
					parser_ListFunc(_option, "all");
				return 1;
			}
			else if (matchParams(sCmd, "logic") && bParserActive)
			{
				parser_ListLogical(_option);
				return 1;
			}
			else if (matchParams(sCmd, "cmd") && bParserActive)
			{
				parser_ListCmd(_option);
				return 1;
			}
			else if (matchParams(sCmd, "define") && bParserActive)
			{
				parser_ListDefine(_functions, _option);
				return 1;
			}
			else if (matchParams(sCmd, "settings"))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
                NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
				BI_ListOptions(_option);
				return 1;
			}
			else if (matchParams(sCmd, "units"))
			{
				parser_ListUnits(_option);
				return 1;
			}
			else if (matchParams(sCmd, "plugins"))
			{
				parser_ListPlugins(_parser, _data, _option);
				return 1;
			}
			else
			{
				doc_Help("list", _option);
				return 1;
			}
		}
		else if (sCommand == "load")
		{
			if (matchParams(sCmd, "define"))
			{
				if (BI_FileExists("functions.def"))
					_functions.load(_option);
				else
					NumeReKernel::print( _lang.get("BUILTIN_CHECKKEYWORD_DEF_EMPTY") );
			}
			else if (matchParams(sCmd, "data") || matchParams(sCmd, "data", '='))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED"));
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (matchParams(sCmd, "data", '='))
					addArgumentQuotes(sCmd, "data");
				if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					if (matchParams(sCmd, "slice", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slice", '=') + 5) == "xz")
						nArgument = -1;
					else if (matchParams(sCmd, "slice", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slice", '=') + 5) == "yz")
						nArgument = -2;
					else
						nArgument = 0;
					if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
						_data.setbLoadEmptyColsInNextFile(true);
					if (matchParams(sCmd, "tocache") && !matchParams(sCmd, "all"))
					{
						Datafile _cache;
						_cache.setTokens(_option.getTokenPaths());
						_cache.setPath(_option.getLoadPath(), false, _option.getExePath());
						_cache.openFile(sArgument, _option, false, true, nArgument);
						sArgument = generateCacheName(sArgument, _option);
						if (!_data.isCacheElement(sArgument + "()"))
							_data.addCache(sArgument + "()", _option);
						nArgument = _data.getCols(sArgument, false);
						for (long long int i = 0; i < _cache.getLines("data", false); i++)
						{
							for (long long int j = 0; j < _cache.getCols("data", false); j++)
							{
								if (!i)
									_data.setHeadLineElement(j + nArgument, sArgument, _cache.getHeadLineElement(j, "data"));
								if (_cache.isValidEntry(i, j, "data"))
								{
									_data.writeToCache(i, j + nArgument, sArgument, _cache.getElement(i, j, "data"));
								}
							}
						}
						if (_data.isValidCache() && _data.getCols(sArgument, false))
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _cache.getDataFileName("data"), toString(_data.getLines(sArgument, false)), toString(_data.getCols(sArgument, false))), _option) );
						return 1;
					}
					else if (matchParams(sCmd, "tocache") && matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
					{
						if (sArgument.find('/') == string::npos)
							sArgument = "<loadpath>/" + sArgument;
						vector<string> vFilelist = getFileList(sArgument, _option);
						if (!vFilelist.size())
							throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
						string sPath = "<loadpath>/";
						if (sArgument.find('/') != string::npos)
							sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
						string sTarget = generateCacheName(sPath + vFilelist[0], _option);
						Datafile _cache;
						_cache.setTokens(_option.getTokenPaths());
						_cache.setPath(_data.getPath(), false, _data.getProgramPath());
						for (unsigned int i = 0; i < vFilelist.size(); i++)
						{
							_cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
							sTarget = generateCacheName(sPath + vFilelist[i], _option);
							if (!_data.isCacheElement(sTarget + "()"))
								_data.addCache(sTarget + "()", _option);
							nArgument = _data.getCols(sTarget, false);
							for (long long int i = 0; i < _cache.getLines("data", false); i++)
							{
								for (long long int j = 0; j < _cache.getCols("data", false); j++)
								{
									if (!i)
										_data.setHeadLineElement(j + nArgument, sTarget, _cache.getHeadLineElement(j, "data"));
									if (_cache.isValidEntry(i, j, "data"))
									{
										_data.writeToCache(i, j + nArgument, sTarget, _cache.getElement(i, j, "data"));
									}
								}
							}
							_cache.removeData(false);
							nArgument = -1;
						}
						if (_data.isValidCache())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_CACHES_SUCCESS", toString((int)vFilelist.size()), sArgument), _option) );
						//NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
						return 1;
					}
					if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
					{
						if (_data.isValid())
						{
							if (_option.getSystemPrintStatus())
								_data.removeData(false);
							else
								_data.removeData(true);
						}
						if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
						{
							if (sArgument.find('/') == string::npos)
								sArgument = "<loadpath>/" + sArgument;
							vector<string> vFilelist = getFileList(sArgument, _option);
							if (!vFilelist.size())
								throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
							string sPath = "<loadpath>/";
							if (sArgument.find('/') != string::npos)
								sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
							_data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
							Datafile _cache;
							_cache.setTokens(_option.getTokenPaths());
							_cache.setPath(_data.getPath(), false, _data.getProgramPath());
							for (unsigned int i = 1; i < vFilelist.size(); i++)
							{
								_cache.removeData(false);
								_cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
								_data.melt(_cache);
							}
							if (_data.isValid())
								NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
							//NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
							return 1;
						}

						if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
						{
							if (matchParams(sCmd, "head", '='))
								nArgument = matchParams(sCmd, "head", '=') + 4;
							else
								nArgument = matchParams(sCmd, "h", '=') + 1;
							nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
							_data.openFile(sArgument, _option, false, true, nArgument);
						}
						else
							_data.openFile(sArgument, _option, false, true, nArgument);
						if (_data.isValid() && _option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
						//NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );

					}
					else if (!_data.isValid())
					{
						if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
						{
							if (sArgument.find('/') == string::npos)
								sArgument = "<loadpath>/" + sArgument;
							vector<string> vFilelist = getFileList(sArgument, _option);
							if (!vFilelist.size())
								throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
							string sPath = "<loadpath>/";
							if (sArgument.find('/') != string::npos)
								sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
							_data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
							Datafile _cache;
							_cache.setTokens(_option.getTokenPaths());
							_cache.setPath(_data.getPath(), false, _data.getProgramPath());
							for (unsigned int i = 1; i < vFilelist.size(); i++)
							{
								_cache.removeData(false);
								_cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
								_data.melt(_cache);
							}
							if (_data.isValid())
								NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
							//NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
							return 1;
						}
						if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
						{
							if (matchParams(sCmd, "head", '='))
								nArgument = matchParams(sCmd, "head", '=') + 4;
							else
								nArgument = matchParams(sCmd, "h", '=') + 1;
							nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
							_data.openFile(sArgument, _option, false, true, nArgument);
						}
						else
							_data.openFile(sArgument, _option, false, false, nArgument);
						if (_data.isValid() && _option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
						//NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );

					}
					else
						load_data(_data, _option, _parser, sArgument);
				}
				else
					load_data(_data, _option, _parser);
			}
			else if (matchParams(sCmd, "script") || matchParams(sCmd, "script", '='))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
				if (!_script.isOpen())
				{
					if (_data.containsStringVars(sCmd))
						_data.getStringValues(sCmd);
					if (matchParams(sCmd, "script", '='))
						addArgumentQuotes(sCmd, "script");
					if (!BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
					{
						do
						{
							NumeReKernel::print(toSystemCodePage( _lang.get("BUILTIN_CHECKKEYWORD_SET_ENTER_VALUE", _lang.get("BUILTIN_CHECKKEYWORD_SCRIPTNAME"))) );
							//NumeReKernel::print("|-> Dateiname des Scripts angeben:" );
							NumeReKernel::printPreFmt("|<- ");
							NumeReKernel::getline(sArgument);
						}
						while (!sArgument.length());
					}
					_script.setScriptFileName(sArgument);
					if (BI_FileExists(_script.getScriptFileName()))
					{
						if (_option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SCRIPTLOAD_SUCCESS", _script.getScriptFileName()), _option) );
						//NumeReKernel::print(LineBreak("|-> Script \"" + _script.getScriptFileName() + "\" wurde erfolgreich geladen!", _option) );
					}
					else
					{
						string sErrorToken = _script.getScriptFileName();
						sArgument = "";
						_script.setScriptFileName(sArgument);
						throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sCmd, sErrorToken, sErrorToken);
					}
				}
				return 1;
			}
			else if (sCmd.length() > findCommand(sCmd).nPos + 5 && sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5) != string::npos)
			{
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (sCmd[sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5)] != '"' && sCmd.find("string(") == string::npos)
				{
					if (matchParams(sCmd, "slice")
							|| matchParams(sCmd, "keepdim")
							|| matchParams(sCmd, "complete")
							|| matchParams(sCmd, "ignore")
							|| matchParams(sCmd, "tocache")
							|| matchParams(sCmd, "i")
							|| matchParams(sCmd, "head")
							|| matchParams(sCmd, "h")
							|| matchParams(sCmd, "app")
							|| matchParams(sCmd, "all"))
					{
						sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5), 1, '"');
						nArgument = string::npos;
						while (sCmd.find_last_of('-', nArgument) != string::npos
								&& sCmd.find_last_of('-', nArgument) > sCmd.find_first_of(' ', sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5)))
							nArgument = sCmd.find_last_of('-', nArgument) - 1;
						nArgument = sCmd.find_last_not_of(' ', nArgument);
						sCmd.insert(nArgument + 1, 1, '"');
					}
					else
					{
						sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5), 1, '"');
						sCmd.insert(sCmd.find_last_not_of(' ') + 1, 1, '"');
					}
				}
				if (matchParams(sCmd, "app"))
				{
					sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 5), "-app=");
					append_data(sCmd, _data, _option, _parser);
					return 1;
				}
				if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					//NumeReKernel::print(sArgument );
					if (matchParams(sCmd, "slice", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slice", '=') + 5) == "xz")
						nArgument = -1;
					else if (matchParams(sCmd, "slice", '=') && getArgAtPos(sCmd, matchParams(sCmd, "slice", '=') + 5) == "yz")
						nArgument = -2;
					else
						nArgument = 0;
					if (matchParams(sCmd, "keepdim") || matchParams(sCmd, "complete"))
						_data.setbLoadEmptyColsInNextFile(true);
					if (matchParams(sCmd, "tocache") && !matchParams(sCmd, "all"))
					{
						Datafile _cache;
						_cache.setTokens(_option.getTokenPaths());
						_cache.setPath(_option.getLoadPath(), false, _option.getExePath());
						_cache.openFile(sArgument, _option, false, true, nArgument);
						sArgument = generateCacheName(sArgument, _option);
						if (!_data.isCacheElement(sArgument + "()"))
							_data.addCache(sArgument + "()", _option);
						nArgument = _data.getCols(sArgument, false);
						for (long long int i = 0; i < _cache.getLines("data", false); i++)
						{
							for (long long int j = 0; j < _cache.getCols("data", false); j++)
							{
								if (!i)
									_data.setHeadLineElement(j + nArgument, sArgument, _cache.getHeadLineElement(j, "data"));
								if (_cache.isValidEntry(i, j, "data"))
								{
									_data.writeToCache(i, j + nArgument, sArgument, _cache.getElement(i, j, "data"));
								}
							}
						}
						if (_data.isValidCache() && _data.getCols(sArgument, false))
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _cache.getDataFileName("data"), toString(_data.getLines(sArgument, false)), toString(_data.getCols(sArgument, false))), _option) );
						return 1;
					}
					else if (matchParams(sCmd, "tocache") && matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
					{
						if (sArgument.find('/') == string::npos)
							sArgument = "<loadpath>/" + sArgument;
						vector<string> vFilelist = getFileList(sArgument, _option);
						if (!vFilelist.size())
							throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
						string sPath = "<loadpath>/";
						if (sArgument.find('/') != string::npos)
							sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
						string sTarget = generateCacheName(sPath + vFilelist[0], _option);
						Datafile _cache;
						_cache.setTokens(_option.getTokenPaths());
						_cache.setPath(_data.getPath(), false, _data.getProgramPath());
						for (unsigned int i = 0; i < vFilelist.size(); i++)
						{
							_cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
							sTarget = generateCacheName(sPath + vFilelist[i], _option);
							if (!_data.isCacheElement(sTarget + "()"))
								_data.addCache(sTarget + "()", _option);
							nArgument = _data.getCols(sTarget, false);
							for (long long int i = 0; i < _cache.getLines("data", false); i++)
							{
								for (long long int j = 0; j < _cache.getCols("data", false); j++)
								{
									if (!i)
										_data.setHeadLineElement(j + nArgument, sTarget, _cache.getHeadLineElement(j, "data"));
									if (_cache.isValidEntry(i, j, "data"))
									{
										_data.writeToCache(i, j + nArgument, sTarget, _cache.getElement(i, j, "data"));
									}
								}
							}
							_cache.removeData(false);
							nArgument = -1;
						}
						if (_data.isValidCache())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_CACHES_SUCCESS", toString((int)vFilelist.size()), sArgument), _option) );
						//NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
						return 1;
					}
					if (matchParams(sCmd, "i") || matchParams(sCmd, "ignore"))
					{
						if (_data.isValid())
						{
							if (_option.getSystemPrintStatus())
								_data.removeData(false);
							else
								_data.removeData(true);
						}
						if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
						{
							if (sArgument.find('/') == string::npos)
								sArgument = "<loadpath>/" + sArgument;
							vector<string> vFilelist = getFileList(sArgument, _option);
							if (!vFilelist.size())
								throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
							string sPath = "<loadpath>/";
							if (sArgument.find('/') != string::npos)
								sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
							_data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
							Datafile _cache;
							_cache.setTokens(_option.getTokenPaths());
							_cache.setPath(_data.getPath(), false, _data.getProgramPath());
							for (unsigned int i = 1; i < vFilelist.size(); i++)
							{
								_cache.removeData(false);
								_cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
								_data.melt(_cache);
							}
							if (_data.isValid())
								NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
							//NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
							return 1;
						}

						if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
						{
							if (matchParams(sCmd, "head", '='))
								nArgument = matchParams(sCmd, "head", '=') + 4;
							else
								nArgument = matchParams(sCmd, "h", '=') + 1;
							nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
							_data.openFile(sArgument, _option, false, true, nArgument);
						}
						else
							_data.openFile(sArgument, _option, false, true, nArgument);
						if (_data.isValid() && _option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", false)), toString(_data.getCols("data", false))), _option) );
						//NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );

					}
					else if (!_data.isValid())
					{
						if (matchParams(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
						{
							if (sArgument.find('/') == string::npos)
								sArgument = "<loadpath>/" + sArgument;
							vector<string> vFilelist = getFileList(sArgument, _option);
							if (!vFilelist.size())
								throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, sArgument, sArgument);
							string sPath = "<loadpath>/";
							if (sArgument.find('/') != string::npos)
								sPath = sArgument.substr(0, sArgument.rfind('/') + 1);
							_data.openFile(sPath + vFilelist[0], _option, false, true, nArgument);
							Datafile _cache;
							_cache.setTokens(_option.getTokenPaths());
							_cache.setPath(_data.getPath(), false, _data.getProgramPath());
							for (unsigned int i = 1; i < vFilelist.size(); i++)
							{
								_cache.removeData(false);
								_cache.openFile(sPath + vFilelist[i], _option, false, true, nArgument);
								_data.melt(_cache);
							}
							if (_data.isValid())
								NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYOWRD_LOAD_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", false)), toString(_data.getCols("data", false))), _option) );
							//NumeReKernel::print(LineBreak("|-> Alle Daten der Dateien \"" + sArgument + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
							return 1;
						}
						if (matchParams(sCmd, "head", '=') || matchParams(sCmd, "h", '='))
						{
							if (matchParams(sCmd, "head", '='))
								nArgument = matchParams(sCmd, "head", '=') + 4;
							else
								nArgument = matchParams(sCmd, "h", '=') + 1;
							nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
							_data.openFile(sArgument, _option, false, true, nArgument);
						}
						else
							_data.openFile(sArgument, _option, false, false, nArgument);
						if (_data.isValid() && _option.getSystemPrintStatus())
							NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", false)), toString(_data.getCols("data", false))), _option) );
						//NumeReKernel::print(LineBreak("|-> Daten aus \"" + _data.getDataFileName("data") + "\" wurden erfolgreich in den Speicher geladen: der Datensatz besteht aus " + toString(_data.getLines("data", true)) + " Zeile(n) und " + toString(_data.getCols("data")) + " Spalte(n).", _option) );
					}
					else
						load_data(_data, _option, _parser, sArgument);
				}
				else
					load_data(_data, _option, _parser);
			}
			else
				doc_Help("load", _option);
			return 1;
		}

		return 0;
	}
	else if (sCommand[0] == 'e')
	{
		//NumeReKernel::print("export" );
		if (sCommand == "export")
		{
			size_t nPrecision = _option.getPrecision();
			if (matchParams(sCmd, "precision", '='))
			{
				_parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "precision", '=')));
				nPrecision = _parser.Eval();
				if (nPrecision < 0 || nPrecision > 14)
					nPrecision = _option.getPrecision();
			}
			if (matchParams(sCmd, "data") || matchParams(sCmd, "data", '='))
			{
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (matchParams(sCmd, "data", '='))
					addArgumentQuotes(sCmd, "data");
				if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					_out.setFileName(sArgument);
					show_data(_data, _out, _option, "data", nPrecision, true, false, true, false);
				}
				else
					show_data(_data, _out, _option, "data", nPrecision, true, false, true);
			}
			else if (_data.matchCache(sCmd).length() || _data.matchCache(sCmd, '=').length())
			{
				if (_data.containsStringVars(sCmd))
					_data.getStringValues(sCmd);
				if (_data.matchCache(sCmd, '=').length())
					addArgumentQuotes(sCmd, _data.matchCache(sCmd, '='));
				if (BI_parseStringArgs(sCmd, sArgument, _parser, _data, _option))
				{
					_out.setFileName(sArgument);
					show_data(_data, _out, _option, _data.matchCache(sCmd), nPrecision, false, true, true, false);
				}
				else
					show_data(_data, _out, _option, _data.matchCache(sCmd), nPrecision, false, true, true);
			}
			else //if (sCmd.find("cache(") != string::npos || sCmd.find("data(") != string::npos)
			{
				for (auto iter = mCaches.begin(); iter != mCaches.end(); ++iter)
				{
					if (sCmd.find(iter->first + "(") != string::npos
							&& (!sCmd.find(iter->first + "(")
								|| (sCmd.find(iter->first + "(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
					{
						//NumeReKernel::print(sCmd );
						Datafile _cache;
						_cache.setCacheStatus(true);
						if (sCmd.find("()") != string::npos)
							sCmd.replace(sCmd.find("()"), 2, "(:,:)");
						_idx = parser_getIndices(sCmd, _parser, _data, _option);

						if (sCmd.find(iter->first + "(") != string::npos && iter->second != -1)
							_data.setCacheStatus(true);

						if (!isValidIndexSet(_idx))
							throw SyntaxError(SyntaxError::CANNOT_EXPORT_DATA, sCmd, iter->first + "(", iter->first);

                        if (_idx.row.isOpenEnd())
                            _idx.row.setRange(0, _data.getLines(iter->first)-1);

                        if (_idx.col.isOpenEnd())
                            _idx.col.setRange(0, _data.getCols(iter->first)-1);

                        _cache.setCacheSize(_idx.row.size(), _idx.col.size(), "cache");

                        if (iter->first != "cache" && iter->first != "data")
                            _cache.renameCache("cache", iter->first, true);

                        if (iter->first == "data")
                            _cache.renameCache("cache", "copy_of_data", true);

                        for (unsigned int i = 0; i < _idx.row.size(); i++)
                        {
                            for (unsigned int j = 0; j < _idx.col.size(); j++)
                            {
                                if (!i)
                                {
                                    _cache.setHeadLineElement(j, (iter->first == "data" ? "copy_of_data" : iter->first), _data.getHeadLineElement(_idx.col[j], iter->first));
                                }
                                if (_data.isValidEntry(_idx.row[i], _idx.col[j], iter->first))
                                    _cache.writeToCache(i, j, (iter->first == "data" ? "copy_of_data" : iter->first), _data.getElement(_idx.row[i], _idx.col[j], iter->first));
                            }
                        }

						if (_data.containsStringVars(sCmd))
							_data.getStringValues(sCmd);

						if (matchParams(sCmd, "file", '='))
							addArgumentQuotes(sCmd, "file");

						//NumeReKernel::print(sCmd );
						_data.setCacheStatus(false);
						if (containsStrings(sCmd) && BI_parseStringArgs(sCmd.substr(matchParams(sCmd, "file", '=')), sArgument, _parser, _data, _option))
						{
							//NumeReKernel::print(sArgument );
							_out.setFileName(sArgument);
							show_data(_cache, _out, _option, (iter->first == "data" ? "copy_of_data" : iter->first), nPrecision, false, true, true, false);
						}
						else
						{
							show_data(_cache, _out, _option, (iter->first == "data" ? "copy_of_data" : iter->first), nPrecision, false, true, true);
						}
						return 1;
					}
				}
			}
			return 1;
		}
		else if (sCommand == "edit")
		{
			if (sCmd.length() > 5)
			{
				BI_editObject(sCmd, _parser, _data, _option);
			}
			else
				doc_Help("edit", _option);
			return 1;
		}
		else if (sCommand == "execute")
		{
			BI_executeCommand(sCmd, _parser, _data, _functions, _option);
			return 1;
		}
		return 0;
	}
	else if (sCommand[0] == 'p')
	{
		if (sCommand.substr(0, 5) == "paste")
		{
			if (matchParams(sCmd, "data"))
			{
			    // DEPRECATED: Declared at v1.1.2rc1
			    NumeReKernel::issueWarning(_lang.get("COMMON_COMMAND_DEPRECATED"));
				_data.pasteLoad(_option);
				if (_data.isValid())
					NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_PASTE_SUCCESS", toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option) );
				//NumeReKernel::print(LineBreak("|-> Die Daten wurden erfolgreich eingefügt: Der Datensatz besteht nun aus "+toString(_data.getLines("data"))+" Zeile(n) und "+toString(_data.getCols("data"))+" Spalte(n).", _option) );
				return 1;

			}
		}
		else if (sCommand == "progress" && sCmd.length() > 9)
		{
			value_type* vVals = 0;
			string sExpr;
			if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
				sCmd = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
			if (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos)
			{
				if (sCmd.find("-set") != string::npos)
					sArgument = sCmd.substr(sCmd.find("-set"));
				else
					sArgument = sCmd.substr(sCmd.find("--"));
				sCmd.erase(sCmd.find(sArgument));
				if (matchParams(sArgument, "first", '='))
				{
					sExpr = getArgAtPos(sArgument, matchParams(sArgument, "first", '=') + 5) + ",";
					//_parser.SetExpr(getArgAtPos(sArgument, matchParams(sArgument, "first", '=')+5));
					//sCmd.erase(matchParams(sCmd, "first", '=')-1, getArgAtPos(sCmd, matchParams(sCmd, "first", '=')+5).length()+6);
					//nVals[1] = int(_parser.Eval());
				}
				else
					sExpr = "1,";
				if (matchParams(sArgument, "last", '='))
				{
					sExpr += getArgAtPos(sArgument, matchParams(sArgument, "last", '=') + 4);
					//_parser.SetExpr(getArgAtPos(sArgument, matchParams(sArgument, "last", '=')+4));
					//sCmd.erase(matchParams(sCmd, "last", '=')-1, getArgAtPos(sCmd, matchParams(sCmd, "last", '=')+4).length()+5);
					//nVals[2] = int(_parser.Eval());
				}
				else
					sExpr += "100";
				if (matchParams(sArgument, "type", '='))
				{
					sArgument = getArgAtPos(sArgument, matchParams(sArgument, "type", '=') + 4);
					if (containsStrings(sArgument))
					{
						if (sArgument.front() != '"')
							sArgument = "\"" + sArgument + "\" -nq";
						string sDummy;
						parser_StringParser(sArgument, sDummy, _data, _parser, _option, true);
					}
				}
				else
					sArgument = "std";
			}
			else
			{
				sArgument = "std";
				sExpr = "1,100";
			}
			while (sCmd.length() && (sCmd[sCmd.length() - 1] == ' ' || sCmd[sCmd.length() - 1] == '-'))
				sCmd.pop_back();
			if (!sCmd.length())
				return 1;
			_parser.SetExpr(sCmd.substr(findCommand(sCmd).nPos + 8) + "," + sExpr);
			vVals = _parser.Eval(nArgument);
			make_progressBar((int)vVals[0], (int)vVals[1], (int)vVals[2], sArgument);
			return 1;
		}
		else if (sCommand == "print" && sCmd.length() > 6)
		{
			sArgument = sCmd.substr(findCommand(sCmd).nPos + 6) + " -print";
			sCmd.replace(findCommand(sCmd).nPos, string::npos, sArgument);
			return 0;
		}
		return 0;
	}
	return 0;
}

// This static function handles the swapping of the values
// of two caches
static int swapCaches(string& sCmd, Datafile& _data, Parser& _parser, Settings& _option, Define& _functions)
{
    string sArgument;

    // If the current command line contains strings
    // handle them here
    if (_data.containsStringVars(sCmd) || containsStrings(sCmd))
        sCmd = BI_evalParamString(sCmd, _parser, _data, _option, _functions);

    // Handle legacy and new syntax in these two cases
    if (_data.matchCache(sCmd, '=').length())
    {
        // Legacy syntax: swap -cache1=cache2
        //
        // Get the option value of the parameter "cache1"
        sArgument = getArgAtPos(sCmd, matchParams(sCmd, _data.matchCache(sCmd, '='), '=') + _data.matchCache(sCmd, '=').length());

        // Swap the caches
        _data.swapCaches(_data.matchCache(sCmd, '='), sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SWAP_CACHE", _data.matchCache(sCmd, '='), sArgument), _option) );
    }
    else if (sCmd.find("()") != string::npos && sCmd.find(',') != string::npos)
    {
        // New syntax: swap cache1(), cache2()
        //
        // Extract the first of the two arguments
        // (length of command = 4)
        sCmd.erase(0, 4);
        sArgument = getNextArgument(sCmd, true);

        if (!sCmd.length())
            return 1;

        // Remove parentheses, if available
        if (sArgument.find('(') != string::npos)
            sArgument.erase(sArgument.find('('));

        if (sCmd.find('(') != string::npos)
            sCmd.erase(sCmd.find('('));

        // Remove not necessary white spaces
        StripSpaces(sCmd);
        StripSpaces(sArgument);

        // Swap the caches
        _data.swapCaches(sCmd, sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_SWAP_CACHE", sCmd, sArgument), _option) );
    }

    return 1;
}

// This static function handles the renaming of a cache with
// a new name
static int renameCaches(string& sCmd, Datafile& _data, Parser& _parser, Settings& _option, Define& _functions)
{
    string sArgument;

    // If the current command line contains strings
    // handle them here
    if (_data.containsStringVars(sCmd) || containsStrings(sCmd))
        sCmd = BI_evalParamString(sCmd, _parser, _data, _option, _functions);

    // Handle legacy and new syntax in these two cases
    if (_data.matchCache(sCmd, '=').length())
    {
        // Legacy syntax: rename -cache1=cache2
        //
        // Get the option value of the parameter "cache1"
        sArgument = getArgAtPos(sCmd, matchParams(sCmd, _data.matchCache(sCmd, '='), '=') + _data.matchCache(sCmd, '=').length());

        // Rename the cache
        _data.renameCache(_data.matchCache(sCmd, '='), sArgument);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RENAME_CACHE", sArgument), _option) );
    }
    else if (sCmd.find("()") != string::npos && sCmd.find(',') != string::npos)
    {
        // New syntax: rename cache1(), cache2()
        //
        // Extract the first of the two arguments
        // (length of command = 6)
        sCmd.erase(0, 6);
        sArgument = getNextArgument(sCmd, true);

        if (!sCmd.length())
            return 1;

        // Remove parentheses, if available
        if (sArgument.find('(') != string::npos)
            sArgument.erase(sArgument.find('('));

        if (sCmd.find('(') != string::npos)
            sCmd.erase(sCmd.find('('));

        // Remove not necessary white spaces
        StripSpaces(sArgument);
        StripSpaces(sCmd);

        // Rename the cache
        _data.renameCache(sArgument, sCmd);

        if (_option.getSystemPrintStatus())
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_CHECKKEYWORD_RENAME_CACHE", sCmd), _option) );
    }

    return 1;
}

// This static function handles the undefinition process of
// custom defined functions
static bool undefineFunctions(string sFunctionList, Define& _functions, const Settings& _option)
{
    string sSuccessFulRemoved;

    // As long as the list of passed functions has a length,
    // undefine the current first argument of the list
    while (sFunctionList.length())
    {
        string sFunction = getNextArgument(sFunctionList, true);

        // Try to undefine the functions
        if (!_functions.undefineFunc(sFunction))
            NumeReKernel::issueWarning(_lang.get("BUILTIN_CHECKKEYWORD_UNDEF_FAIL", sFunction));
        else
            sSuccessFulRemoved += sFunction + ", ";
    }

    // Inform the user that (some) of the functions were undefined
    if (_option.getSystemPrintStatus() && sSuccessFulRemoved.length())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_UNDEF_SUCCESS", sSuccessFulRemoved.substr(0, sSuccessFulRemoved.length()-2)));

    return true;
}

// This static function handles the displaying of user interaction dialogs.
// This includes message boxes, file and directory pickers, text entries,
// list and selection dialogs
static int showDialog(string& sCmd)
{
    size_t position = findCommand(sCmd, "dialog").nPos;
    string sDialogSettings = sCmd.substr(position+7);
    string sMessage;
    string sTitle = "NumeRe: Window";
    string sExpression;
    int nControls = NumeRe::CTRL_NONE;
    NumeReKernel* kernel = NumeReKernel::getInstance();

    // If the current command line contains strings in the option values
    // handle them here
    if (kernel->getData().containsStringVars(sDialogSettings) || containsStrings(sDialogSettings))
        sDialogSettings = BI_evalParamString(sDialogSettings, kernel->getParser(), kernel->getData(), kernel->getSettings(), kernel->getDefinitions());

    // Extract the message for the user
    if (matchParams(sDialogSettings, "msg", '='))
        sMessage = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "msg", '=')+3);

    // Extract the window title
    if (matchParams(sDialogSettings, "title", '='))
        sTitle = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "title", '=')+5);

    // Extract the selected dialog type if available, otherwise
    // use the message box as default value
    if (matchParams(sDialogSettings, "type", '='))
    {
        string sType = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "type", '=')+4);

        if (sType == "filedialog")
            nControls = NumeRe::CTRL_FILEDIALOG;
        else if (sType == "dirdialog")
            nControls = NumeRe::CTRL_FOLDERDIALOG;
        else if (sType == "listdialog")
            nControls = NumeRe::CTRL_LISTDIALOG;
        else if (sType == "selectiondialog")
            nControls = NumeRe::CTRL_SELECTIONDIALOG;
        else if (sType == "messagebox")
            nControls = NumeRe::CTRL_MESSAGEBOX;
        else if (sType == "textentry")
            nControls = NumeRe::CTRL_TEXTENTRY;
    }
    else
        nControls = NumeRe::CTRL_MESSAGEBOX;

    // Extract the button information. The default values are
    // created by wxWidgets. We don't have to do that here
    if (matchParams(sDialogSettings, "buttons", '='))
    {
        string sButtons = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "buttons", '=')+7);

        if (sButtons == "ok")
            nControls |= NumeRe::CTRL_OKBUTTON;
        else if (sButtons == "okcancel")
            nControls |= NumeRe::CTRL_OKBUTTON | NumeRe::CTRL_CANCELBUTTON;
        else if (sButtons == "yesno")
            nControls |= NumeRe::CTRL_YESNOBUTTON;
    }

    // Extract the icon information. The default values are
    // created by wxWidgets. We don't have to do that here
    if (matchParams(sDialogSettings, "icon", '='))
    {
        string sIcon = getArgAtPos(sDialogSettings, matchParams(sDialogSettings, "icon", '=')+4);

        if (sIcon == "erroricon")
            nControls |= NumeRe::CTRL_ICONERROR;
        else if (sIcon == "warnicon")
            nControls |= NumeRe::CTRL_ICONWARNING;
        else if (sIcon == "infoicon")
            nControls |= NumeRe::CTRL_ICONINFORMATION;
        else if (sIcon == "questionicon")
            nControls |= NumeRe::CTRL_ICONQUESTION;
    }

    // Extract the default values for the dialog. First,
    // erase the appended parameter list
    if (sDialogSettings.find("-set") != string::npos)
        sDialogSettings.erase(sDialogSettings.find("-set"));
    else if (sDialogSettings.find("--") != string::npos)
        sDialogSettings.erase(sDialogSettings.find("--"));

    // Strip spaces and assign the value
    StripSpaces(sDialogSettings);
    sExpression = sDialogSettings;

    // Handle strings in the default value
    // expression. This will include also possible path
    // tokens
    if (kernel->getData().containsStringVars(sExpression) || containsStrings(sExpression))
    {
        string sDummy;
        parser_StringParser(sExpression, sDummy, kernel->getData(), kernel->getParser(), kernel->getSettings(), true);
    }

    // Ensure that default values are available, if the user
    // selected either a list or a selection dialog
    if ((nControls & NumeRe::CTRL_LISTDIALOG || nControls & NumeRe::CTRL_SELECTIONDIALOG) && (!sExpression.length() || sExpression == "\"\""))
    {
        throw SyntaxError(SyntaxError::NO_DEFAULTVALUE_FOR_DIALOG, sCmd, "dialog");
    }

    // Use the default expression as message for the message
    // box as a fallback solution
    if (nControls & NumeRe::CTRL_MESSAGEBOX && (!sMessage.length() || sMessage == "\"\""))
        sMessage = getNextArgument(sExpression, false);

    // Ensure that the message box has at least a message,
    // because the message box is the default value
    if (nControls & NumeRe::CTRL_MESSAGEBOX && (!sMessage.length() || sMessage == "\"\""))
    {
        throw SyntaxError(SyntaxError::NO_DEFAULTVALUE_FOR_DIALOG, sCmd, "dialog");
    }

    // Get the window manager, create the modal window and
    // wait until the user interacted with the dialog
    NumeRe::WindowManager& manager = kernel->getWindowManager();
    size_t winid = manager.createWindow(NumeRe::WINDOW_MODAL, NumeRe::WindowSettings(nControls, true, sMessage, sTitle, sExpression));
    NumeRe::WindowInformation wininfo = manager.getWindowInformationModal(winid);

    // Insert the return value as a string into the command
    // line and inform the command handler, that a value
    // has to be evaluated
    sCmd = sCmd.substr(0, position) + "\"" + replacePathSeparator(wininfo.sReturn) + "\"";

    return 0;
}

// This static function handles the displaying of tables and clusters
// in the table viewer. Editing of tables is not supplied by this function.
static int showDataObject(string& sCmd)
{
    // Get references to the main objects
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    // Handle the compact mode (probably not needed any more)
    if (sCmd.substr(0, 5) == "showf")
    {
        _out.setCompact(false);
    }
    else
    {
        _out.setCompact(_option.getbCompact());
    }

    // Determine the correct data object
    if (matchParams(sCmd, "data") || sCmd.find(" data()") != string::npos)
    {
        // data as object, passed as parameter
        show_data(_data, _out, _option, "data", _option.getPrecision(), true, false);
    }
    else if (_data.matchCache(sCmd).length())
    {
        // a cache as object, passed as parameter
        show_data(_data, _out, _option, _data.matchCache(sCmd), _option.getPrecision(), false, true);
    }
    else
    {
        DataAccessParser _accessParser(sCmd);

        if (_accessParser.getDataObject().length())
        {
            if (_accessParser.isCluster())
            {
                NumeRe::Cluster& cluster = _data.getCluster(_accessParser.getDataObject());

                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, cluster.size()-1);

                // Create the target container
                NumeRe::Container<string> _stringTable(_accessParser.getIndices().row.size(), 1);

                // Copy the data to the new container
                for (size_t i = 0; i < _accessParser.getIndices().row.size(); i++)
                {
                    if (cluster.getType(i) == NumeRe::ClusterItem::ITEMTYPE_STRING)
                        _stringTable.set(i, 0, cluster.getString(_accessParser.getIndices().row[i]));
                    else
                        _stringTable.set(i, 0, toString(cluster.getDouble(_accessParser.getIndices().row[i]), 5));
                }

                // Redirect control
                NumeReKernel::showStringTable(_stringTable, _accessParser.getDataObject() + "{}");

                return 1;
            }
            else if (_accessParser.getDataObject() == "string")
            {
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getStringElements()-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.setRange(0, _data.getStringCols()-1);

                // Create the target container
                NumeRe::Container<string> _stringTable(_accessParser.getIndices().row.size(), _accessParser.getIndices().col.size());

                // Copy the data to the new container and add surrounding
                // quotation marks
                for (size_t j = 0; j < _accessParser.getIndices().col.size(); j++)
                {
                    for (size_t i = 0; i < _accessParser.getIndices().row.size(); i++)
                    {
                        if (_data.getStringElements(_accessParser.getIndices().col[j]) <= _accessParser.getIndices().row[i])
                            break;

                        _stringTable.set(i, j, "\"" + _data.readString(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j]) + "\"");
                    }
                }

                // Redirect control
                NumeReKernel::showStringTable(_stringTable, "string()");
            }
            else
            {
                Datafile _cache;

                // Validize the obtained index sets
                if (!isValidIndexSet(_accessParser.getIndices()))
                    throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, _accessParser.getDataObject() + "(", _accessParser.getDataObject() + "()");

                // Copy the target data to a new table
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);

                _cache.setCacheSize(_accessParser.getIndices().row.size(), _accessParser.getIndices().col.size(), "cache");
                _cache.renameCache("cache", "*" + _accessParser.getDataObject(), true);

                for (unsigned int i = 0; i < _accessParser.getIndices().row.size(); i++)
                {
                    for (unsigned int j = 0; j < _accessParser.getIndices().col.size(); j++)
                    {
                        if (!i)
                        {
                            _cache.setHeadLineElement(j, "*" + _accessParser.getDataObject(), _data.getHeadLineElement(_accessParser.getIndices().col[j], _accessParser.getDataObject()));
                        }

                        if (_data.isValidEntry(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()))
                            _cache.writeToCache(i, j, "*" + _accessParser.getDataObject(), _data.getElement(_accessParser.getIndices().row[i], _accessParser.getIndices().col[j], _accessParser.getDataObject()));
                    }
                }

                if (_data.containsStringVars(sCmd))
                    _data.getStringValues(sCmd);

                // Redirect the control
                show_data(_cache, _out, _option, "*" + _accessParser.getDataObject(), _option.getPrecision(), false, true);
                return 1;
            }
        }
        else
        {
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);
        }

    }

    return 1;
}











// This function performs the autosave at the application termination
void BI_Autosave(Datafile& _data, Output& _out, Settings& _option)
{
    // Only do something, if there's unsaved and valid data
	if (_data.isValidCache() && !_data.getSaveStatus())
	{
	    // Inform the user
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt(toSystemCodePage(  _lang.get("BUILTIN_AUTOSAVE") + " ... "));

		// Try to save the cache
		if (_data.saveCache())
		{
			if (_option.getSystemPrintStatus())
				NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS") + ".") );
		}
		else
		{
			if (_option.getSystemPrintStatus())
				NumeReKernel::printPreFmt("\n");
			throw SyntaxError(SyntaxError::CANNOT_SAVE_CACHE, "", SyntaxError::invalid_position);
		}
	}
	return;
}

// This is a wrapper for the corresponding function from tools.cpp
bool BI_FileExists(const string& sFilename)
{
	return fileExists(sFilename);
}

// This function lists all internal (kernel) settings
static void BI_ListOptions(Settings& _option)
{
	make_hline();
	NumeReKernel::print("NUMERE: " + toUpperCase(_lang.get("BUILTIN_LISTOPT_SETTINGS")) );
	make_hline();
	NumeReKernel::print(  toSystemCodePage(_lang.get("BUILTIN_LISTOPT_1")) + "\n|" );

	// List the path settings
	NumeReKernel::printPreFmt(sectionHeadline(_lang.get("BUILTIN_LISTOPT_2")));
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_3", _option.getSavePath()), _option, true, 0, 25) + "\n" );
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_4", _option.getLoadPath()), _option, true, 0, 25) + "\n" );
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_5", _option.getScriptPath()), _option, true, 0, 25) + "\n" );
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_6", _option.getProcsPath()), _option, true, 0, 25) + "\n" );
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_7", _option.getPlotOutputPath()), _option, true, 0, 25) + "\n" );
	if (_option.getViewerPath().length())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_8", _option.getViewerPath()), _option, true, 0, 25) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_8", _lang.get("BUILTIN_LISTOPT_NOVIEWER")), _option, true, 0, 25) + "\n");
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_9", _option.getEditorPath()), _option, true, 0, 25) + "\n");
	NumeReKernel::printPreFmt("|\n" );

	// List all other settings
	NumeReKernel::printPreFmt(sectionHeadline(_lang.get("BUILTIN_LISTOPT_10")));

	// Autosaveintervall
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_11", toString(_option.getAutoSaveInterval())), _option) + "\n");

	// Greeting
	if (_option.getbGreeting())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_12", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_12", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Buffer
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_13", toString(_option.getBuffer(1))), _option) + "\n");

	// Draftmode
	if (_option.getbUseDraftMode())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_15", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_15", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Extendedfileinfo
	if (_option.getbShowExtendedFileInfo())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_16", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_16", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// ESC in Scripts
	if (_option.getbUseESCinScripts())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_17", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_17", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Defcontrol
	if (_option.getbDefineAutoLoad())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_19", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_19", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Compact table view in the terminal
	if (_option.getbCompact())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_20", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_20", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Loading empty columns
	if (_option.getbLoadEmptyCols())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_21", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_21", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Precision
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_22", toString(_option.getPrecision())), _option) + "\n");

	// Create a logfile of the terminal inputs
	if (_option.getbUseLogFile())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_23", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_23", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Default Plotfont
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_25", _option.getDefaultPlotFont()), _option) + "\n");

	// Display Hints
	if (_option.getbShowHints())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_26", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_26", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	// Use UserLangFiles
	if (_option.getUseCustomLanguageFiles())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_27", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_27", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	//  Use the ExternalDocViewer
	if (_option.getUseExternalViewer())
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_28", toUpperCase(_lang.get("COMMON_ACTIVE"))), _option) + "\n");
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("BUILTIN_LISTOPT_28", toUpperCase(_lang.get("COMMON_INACTIVE"))), _option) + "\n");

	NumeReKernel::printPreFmt("|\n" );
	NumeReKernel::print(LineBreak( _lang.get("BUILTIN_LISTOPT_FOOTNOTE"), _option) );
	make_hline();
	return;
}

// This function returns the string argument for a single parameter in the command line.
// It will also parse it directly, which means that it won't contain further string operations.
bool BI_parseStringArgs(const string& sCmd, string& sArgument, Parser& _parser, Datafile& _data, Settings& _option)
{
    // Don't do anything, if no string is found in this expression
	if (!containsStrings(sCmd) && !_data.containsStringVars(sCmd))
		return false;
	string sTemp = sCmd;

    // Get the contents of the contained data tables
	if (sTemp.find("data(") != string::npos || _data.containsTablesOrClusters(sTemp))
	{
		getDataElements(sTemp, _parser, _data, _option);
	}

	//
	for (unsigned int i = 0; i < sTemp.length(); i++)
	{
	    // Jump over this parenthesis, if its contents don't contain
	    // strings or string variables
		if (sTemp[i] == '('
				&& !containsStrings(sTemp.substr(i, getMatchingParenthesis(sTemp.substr(i))))
				&& !containsStrings(sTemp.substr(0, i))
				&& !_data.containsStringVars(sTemp.substr(i, getMatchingParenthesis(sTemp.substr(i))))
				&& !_data.containsStringVars(sTemp.substr(0, i)))
		{
			i += getMatchingParenthesis(sTemp.substr(i));
		}

		// Evaluate parameter starts, i.e. the minus sign of the command line
		if (sTemp[i] == '-'
				&& !containsStrings(sTemp.substr(0, i))
				&& !_data.containsStringVars(sTemp.substr(0, i)))
		{
		    // No string left of the minus sign, erase this part
		    // and break the loop
			sTemp.erase(0, i);
			break;
		}
		else if (sTemp[i] == '-'
				 && (containsStrings(sTemp.substr(0, i))
					 || _data.containsStringVars(sTemp.substr(0, i))))
		{
		    // There are strings left of the minus sign
		    // Find now the last string element in this part of the expression
			for (int j = (int)i; j >= 0; j--)
			{
			    // Find the start of this function or data element, which
			    // ends at this character
				if (sTemp[j] == '(' && j && (isalnum(sTemp[j - 1]) || sTemp[j - 1] == '_'))
				{
					while (j && (isalnum(sTemp[j - 1]) || sTemp[j - 1] == '_'))
					{
						j--;
					}
				}

				// This is now the location, where all string-related stuff is
				// to the right and everything else is to the left
				if ((!containsStrings(sTemp.substr(0, j)) && containsStrings(sTemp.substr(j, i - j)))
						|| (!_data.containsStringVars(sTemp.substr(0, j)) && _data.containsStringVars(sTemp.substr(j, i - j))))
				{
				    // Erase the left part and break the loop
					sTemp.erase(0, j);
					break;
				}
			}
			break;
		}
	}

	// If there are no strings, sTemp will be empty
	if (!sTemp.length())
		return false;

    // Get the string variable values
	if (_data.containsStringVars(sTemp))
		_data.getStringValues(sTemp);

    // Get now the string argument, which may contain pure
    // strings, string functions and concatenations
	if (!getStringArgument(sTemp, sArgument))
		return false;

    // If there are path tokens in the string part, ensure that
    // they are valid. Additionally, replace the "<this>" path token
	if (sArgument.find('<') != string::npos && sArgument.find('>', sArgument.find('<')) != string::npos)
	{
		for (unsigned int i = 0; i < sArgument.length(); i++)
		{
			if (sArgument.find('<', i) == string::npos)
				break;
			if (sArgument[i] == '<' && sArgument.find('>', i) != string::npos)
			{
				string sToken = sArgument.substr(i, sArgument.find('>', i) + 1 - i);
				if (sToken == "<this>")
					sToken = _option.getExePath();
				if (sToken.find('/') == string::npos)
				{
				    // Is the token valid?
					if (_option.getTokenPaths().find(sToken) == string::npos)
					{
						throw SyntaxError(SyntaxError::UNKNOWN_PATH_TOKEN, sCmd, sToken, sToken);
					}
				}
				i = sArgument.find('>', i);
			}
		}
	}

	// Clear the temporary variable
	sTemp.clear();

	// Parse the string expression
	if (!parser_StringParser(sArgument, sTemp, _data, _parser, _option, true))
		return false;
	else
	{
		sArgument = sArgument.substr(1, sArgument.length() - 2);
		return true;
	}
}

// Returns a random greeting string, which may be printed to the terminal later
string BI_Greeting(Settings& _option)
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

// This function evaluates a passed parameter string, so that the values of the parameters
// are only values and no expressions any more
string BI_evalParamString(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option, Define& _functions)
{
	string sReturn = sCmd;
	string sTemp = "";
	string sDummy = "";
	unsigned int nPos = 0;
	unsigned int nLength = 0;
	vector<double> vInterval;

	// Add a whitespace character at the end
	if (sReturn.back() != ' ')
		sReturn += " ";

    // Try to detect the interval syntax
	if (sReturn.find('-') != string::npos
			&& (sReturn.find('[') != string::npos
				|| matchParams(sReturn, "x", '=')
				|| matchParams(sReturn, "y", '=')
				|| matchParams(sReturn, "z", '=')))
	{
	    // Get the parameter part of the string and remove
	    // the parameter string part from the original expression
		if (sReturn.find("-set") != string::npos)
		{
			sTemp = sReturn.substr(sReturn.find("-set"));
			sReturn.erase(sReturn.find("-set"));
		}
		else if (sReturn.find("--") != string::npos)
		{
			sTemp = sReturn.substr(sReturn.find("--"));
			sReturn.erase(sReturn.find("--"));
		}
		else
		{
			sTemp = sReturn.substr(sReturn.find('-'));
			sReturn.erase(sReturn.find("-"));
		}

		// Parse the interval syntax
		vInterval = parser_IntervalReader(sTemp, _parser, _data, _functions, _option, true);

		// Append the remaining part of the parameter string to the expression
		sReturn += sTemp;
	}

	// Get the string var values, if any
	if (_data.containsStringVars(sReturn))
		_data.getStringValues(sReturn);

	// Repeat as long as an equal sign is found after
	// the current position in the command line
	while (sReturn.find('=', nPos) != string::npos)
	{
	    // Get the position after the equal sign
		nPos = sReturn.find('=', nPos) + 1;

		// Ignore equal signs in strings
		if (isInQuotes(sReturn, nPos))
        {
            nPos++;
            continue;
        }

		// jump over whitespaces
		while (nPos < sReturn.length() - 1 && sReturn[nPos] == ' ')
			nPos++;

        // Parse the parameter values into evaluated values for the commands
		if (containsStrings(sReturn.substr(nPos, sReturn.find(' ', nPos) - nPos)) || _data.containsStringVars(sReturn.substr(nPos, sReturn.find(' ', nPos) - nPos)))
		{
		    // This is a string value
			if (!getStringArgument(sReturn.substr(nPos - 1), sTemp)) // mit "=" uebergeben => fixes getStringArgument issues
				return "";

			// Get the current length of the string
			nLength = sTemp.length();
			sTemp += " -kmq";

			// Parse the string
			if (!parser_StringParser(sTemp, sDummy, _data, _parser, _option, true))
				return "";

            // Replace the parsed string
			sReturn.replace(nPos, nLength, sTemp);
		}
		else if ((nPos > 5 && sReturn.substr(nPos - 5, 5) == "save=")
				 || (nPos > 7 && sReturn.substr(nPos - 7, 7) == "export="))
		{
		    // This is a path value without quotation marks
		    // (otherwise it would be catched by the previous block)
			sTemp = sReturn.substr(nPos, sReturn.find(' ', nPos) - nPos);
			nLength = sTemp.length();

			// Add quotation marks and replace the prvious path definition
			sTemp = "\"" + sTemp + "\"";
			sReturn.replace(nPos, nLength, sTemp);
		}
		else if ((nPos > 8 && sReturn.substr(nPos - 8, 8) == "tocache=")
				 || (nPos > 5 && sReturn.substr(nPos - 5, 5) == "type=")
                 || (nPos > 5 && sReturn.substr(nPos - 5, 5) == "icon=")
                 || (nPos > 8 && sReturn.substr(nPos - 8, 8) == "buttons="))
		{
		    // do nothing here
			nPos++;
		}
		else
		{
		    // All other cases, i.e. numerical values
		    // evaluate the value correspondingly

		    // Get the value and its length
			sTemp = sReturn.substr(nPos, sReturn.find(' ', nPos) - nPos);
			nLength = sTemp.length();

			// Call functions
			if (!_functions.call(sTemp))
				return "";

            // Get data elements
			if (sTemp.find("data(") != string::npos || _data.containsTablesOrClusters(sTemp))
				getDataElements(sTemp, _parser, _data, _option);

            int nResult = 0;
            value_type* v = nullptr;

            // If the string contains a colon operator,
            // replace it with a comma
            if (sTemp.find(':') != string::npos)
            {
				string sTemp_2 = "";
				sTemp = "(" + sTemp + ")";
				parser_SplitArgs(sTemp, sTemp_2, ':', _option, false);

                sTemp += ", " + sTemp_2;
            }

            // Set the expression and evaluate it numerically
            _parser.SetExpr(sTemp);
            v = _parser.Eval(nResult);

            // Clear the temporary variable
            sTemp.clear();

            // convert the doubles into strings and remove the trailing comma
            for (int i = 0; i < nResult; i++)
            {
                sTemp += toString(v[i], _option) + ":";
            }
            sTemp.pop_back();

            // Replace the string
			sReturn.replace(nPos, nLength, sTemp);
		}

	}

	// Convert the calculated intervals into their string definitions
	if (vInterval.size())
	{
	    // x interval
		if (vInterval.size() >= 2)
		{
			if (!isnan(vInterval[0]) && !isnan(vInterval[1]))
				sReturn += " -x=" + toString(vInterval[0], 7) + ":" + toString(vInterval[1], 7);
		}

		// y interval
		if (vInterval.size() >= 4)
		{
			if (!isnan(vInterval[2]) && !isnan(vInterval[3]))
				sReturn += " -y=" + toString(vInterval[2], 7) + ":" + toString(vInterval[3], 7);
		}

		// z interval
		if (vInterval.size() >= 6)
		{
			if (!isnan(vInterval[4]) && !isnan(vInterval[5]))
				sReturn += " -z=" + toString(vInterval[4], 7) + ":" + toString(vInterval[5], 7);
		}
	}

	return sReturn;
}

// This function handles the displaying of the contents of the selected folders
static bool BI_ListFiles(const string& sCmd, const Settings& _option)
{
	string sConnect = "";
	string sSpecified = "";
	string __sCmd = sCmd + " ";
	string sPattern = "";
	unsigned int nFirstColLength = _option.getWindow() / 2 - 6;
	bool bFreePath = false;
	if (matchParams(__sCmd, "pattern", '=') || matchParams(__sCmd, "p", '='))
	{
		int nPos = 0;
		if (matchParams(__sCmd, "pattern", '='))
			nPos = matchParams(__sCmd, "pattern", '=') + 7;
		else
			nPos = matchParams(__sCmd, "p", '=') + 1;
		sPattern = getArgAtPos(__sCmd, nPos);
		StripSpaces(sPattern);
		if (sPattern.length())
			sPattern = _lang.get("BUILTIN_LISTFILES_FILTEREDFOR", sPattern);
	}

	make_hline();
	sConnect = "NUMERE: " + toUpperCase(_lang.get("BUILTIN_LISTFILES_EXPLORER"));
	if (sConnect.length() > nFirstColLength + 6)
	{
		sConnect += "    ";
	}
	else
		sConnect.append(nFirstColLength + 6 - sConnect.length(), ' ');
	NumeReKernel::print(LineBreak(sConnect + sPattern, _option, true, 0, sConnect.length()) );
	make_hline();

	if (matchParams(__sCmd, "files", '='))
	{
		int nPos = matchParams(__sCmd, "files", '=') + 5;
		sSpecified = getArgAtPos(__sCmd, nPos);

		StripSpaces(sSpecified);
		if (sSpecified[0] == '<' && sSpecified[sSpecified.length() - 1] == '>' && sSpecified != "<>" && sSpecified != "<this>")
		{
			sSpecified = sSpecified.substr(1, sSpecified.length() - 2);
			sSpecified = toLowerCase(sSpecified);
			if (sSpecified != "loadpath"
					&& sSpecified != "savepath"
					&& sSpecified != "plotpath"
					&& sSpecified != "scriptpath"
					&& sSpecified != "procpath"
					&& sSpecified != "wp")
				sSpecified = "";
		}
		else
		{
			bFreePath = true;
		}
	}

	if (!bFreePath)
	{
		if (!sSpecified.length() || sSpecified == "loadpath")
		{
			sConnect = _option.getLoadPath() + "  ";
			if (sConnect.length() > nFirstColLength)
			{
				sConnect += "$";
				sConnect.append(nFirstColLength, '-');
			}
			else
				sConnect.append(nFirstColLength - sConnect.length(), '-');
			sConnect += "  <" + toUpperCase(_lang.get("BUILTIN_LISTFILES_LOADPATH")) + ">  ";
			if (sConnect.find('$') != string::npos)
			{
				sConnect.append(_option.getWindow() - 4 - sConnect.length() + sConnect.rfind('$'), '-');
			}
			else
				sConnect.append(_option.getWindow() - 4 - sConnect.length(), '-');
			NumeReKernel::print(LineBreak( sConnect, _option) );
			if (!BI_ListDirectory("LOADPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}
		if (!sSpecified.length() || sSpecified == "savepath")
		{
			sConnect = _option.getSavePath() + "  ";
			if (sConnect.length() > nFirstColLength)
			{
				sConnect += "$";
				sConnect.append(nFirstColLength, '-');
			}
			else
				sConnect.append(nFirstColLength - sConnect.length(), '-');
			sConnect += "  <" + toUpperCase(_lang.get("BUILTIN_LISTFILES_SAVEPATH")) + ">  ";
			if (sConnect.find('$') != string::npos)
			{
				sConnect.append(_option.getWindow() - 4 - sConnect.length() + sConnect.rfind('$'), '-');
			}
			else
				sConnect.append(_option.getWindow() - 4 - sConnect.length(), '-');
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );
			NumeReKernel::print(LineBreak( sConnect, _option) );
			if (!BI_ListDirectory("SAVEPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}
		if (!sSpecified.length() || sSpecified == "scriptpath")
		{
			sConnect = _option.getScriptPath() + "  ";
			if (sConnect.length() > nFirstColLength)
			{
				sConnect += "$";
				sConnect.append(nFirstColLength, '-');
			}
			else
				sConnect.append(nFirstColLength - sConnect.length(), '-');
			sConnect += "  <" + toUpperCase(_lang.get("BUILTIN_LISTFILES_SCRIPTPATH")) + ">  ";
			if (sConnect.find('$') != string::npos)
			{
				sConnect.append(_option.getWindow() - 4 - sConnect.length() + sConnect.rfind('$'), '-');
			}
			else
				sConnect.append(_option.getWindow() - 4 - sConnect.length(), '-');
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );
			NumeReKernel::print(LineBreak( sConnect, _option) );
			if (!BI_ListDirectory("SCRIPTPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}
		if (!sSpecified.length() || sSpecified == "procpath")
		{
			sConnect = _option.getProcsPath() + "  ";
			if (sConnect.length() > nFirstColLength)
			{
				sConnect += "$";
				sConnect.append(nFirstColLength, '-');
			}
			else
				sConnect.append(nFirstColLength - sConnect.length(), '-');
			sConnect += "  <" + toUpperCase(_lang.get("BUILTIN_LISTFILES_PROCPATH")) + ">  ";
			if (sConnect.find('$') != string::npos)
			{
				sConnect.append(_option.getWindow() - 4 - sConnect.length() + sConnect.rfind('$'), '-');
			}
			else
				sConnect.append(_option.getWindow() - 4 - sConnect.length(), '-');
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );
			NumeReKernel::print(LineBreak( sConnect, _option) );
			if (!BI_ListDirectory("PROCPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}
		if (!sSpecified.length() || sSpecified == "plotpath")
		{
			sConnect = _option.getPlotOutputPath() + "  ";
			if (sConnect.length() > nFirstColLength)
			{
				sConnect += "$";
				sConnect.append(nFirstColLength, '-');
			}
			else
				sConnect.append(nFirstColLength - sConnect.length(), '-');
			sConnect += "  <" + toUpperCase(_lang.get("BUILTIN_LISTFILES_PLOTPATH")) + ">  ";
			if (sConnect.find('$') != string::npos)
			{
				sConnect.append(_option.getWindow() - 4 - sConnect.length() + sConnect.rfind('$'), '-');
			}
			else
				sConnect.append(_option.getWindow() - 4 - sConnect.length(), '-');
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );
			NumeReKernel::print(LineBreak( sConnect, _option) );
			if (!BI_ListDirectory("PLOTPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}
		if (sSpecified == "wp")
		{
			sConnect = _option.getWorkPath() + "  ";
			if (sConnect.length() > nFirstColLength)
			{
				sConnect += "$";
				sConnect.append(nFirstColLength, '-');
			}
			else
				sConnect.append(nFirstColLength - sConnect.length(), '-');
			sConnect += "  <" + toUpperCase(_lang.get("BUILTIN_LISTFILES_WORKPATH")) + ">  ";
			if (sConnect.find('$') != string::npos)
			{
				sConnect.append(_option.getWindow() - 4 - sConnect.length() + sConnect.rfind('$'), '-');
			}
			else
				sConnect.append(_option.getWindow() - 4 - sConnect.length(), '-');
			if (!sSpecified.length())
				NumeReKernel::printPreFmt("|\n" );
			NumeReKernel::print(LineBreak( sConnect, _option) );
			if (!BI_ListDirectory("WORKPATH", __sCmd, _option))
				NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
		}
	}
	else
	{
		sSpecified = fromSystemCodePage(sSpecified);
		if (sSpecified == "<>" || sSpecified == "<this>")
			sConnect = _option.getExePath() + "  ";
		else
			sConnect = sSpecified + "  ";
		if (sConnect.length() > nFirstColLength)
		{
			sConnect += "$";
			sConnect.append(nFirstColLength, '-');
		}
		else
			sConnect.append(nFirstColLength - sConnect.length(), '-');
		if (sSpecified == "<>" || sSpecified == "<this>")
			sConnect += "  <" + toUpperCase(_lang.get("BUILTIN_LISTFILES_ROOTPATH")) + ">  ";
		else
			sConnect += "  <" + toUpperCase(_lang.get("BUILTIN_LISTFILES_CUSTOMPATH")) + ">  ";
		if (sConnect.find('$') != string::npos)
		{
			sConnect.append(_option.getWindow() - 4 - sConnect.length() + sConnect.rfind('$'), '-');
		}
		else
			sConnect.append(_option.getWindow() - 4 - sConnect.length(), '-');
		NumeReKernel::print(LineBreak( sConnect, _option) );
		if (!BI_ListDirectory(sSpecified, __sCmd, _option))
			NumeReKernel::printPreFmt(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NOFILES") + " --", _option) + "\n");
	}
	make_hline();
	return true;
}

// This function displays the contents of a single directory
static bool BI_ListDirectory(const string& sDir, const string& sParams, const Settings& _option)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	LARGE_INTEGER Filesize;
	double dFilesize = 0.0;
	double dFilesizeTotal = 0.0;
	string sConnect = "";
	string sPattern = "*";
	string sFilesize = " Bytes";
	string sFileName = "";
	string sDirectory = "";
	int nLength = 0;
	int nCount[2] = {0, 0};
	unsigned int nFirstColLength = _option.getWindow() / 2 - 6;
	bool bOnlyDir = false;

	if (matchParams(sParams, "dir"))
		bOnlyDir = true;

	if (matchParams(sParams, "pattern", '=') || matchParams(sParams, "p", '='))
	{
		int nPos = 0;
		if (matchParams(sParams, "pattern", '='))
			nPos = matchParams(sParams, "pattern", '=') + 7;
		else
			nPos = matchParams(sParams, "p", '=') + 1;
		sPattern = getArgAtPos(sParams, nPos);
		StripSpaces(sPattern);
		if (!sPattern.length())
			sPattern = "*";
	}

	for (int n = 0; n < 2; n++)
	{
		if (bOnlyDir && n)
			break;
		if (sDir == "LOADPATH")
		{
			hFind = FindFirstFile((_option.getLoadPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getLoadPath();
		}
		else if (sDir == "SAVEPATH")
		{
			hFind = FindFirstFile((_option.getSavePath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getSavePath();
		}
		else if (sDir == "PLOTPATH")
		{
			hFind = FindFirstFile((_option.getPlotOutputPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getPlotOutputPath();
		}
		else if (sDir == "SCRIPTPATH")
		{
			hFind = FindFirstFile((_option.getScriptPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getScriptPath();
		}
		else if (sDir == "PROCPATH")
		{
			hFind = FindFirstFile((_option.getProcsPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getProcsPath();
		}
		else if (sDir == "WORKPATH")
		{
			hFind = FindFirstFile((_option.getWorkPath() + "\\" + sPattern).c_str(), &FindFileData);
			sDirectory = _option.getWorkPath();
		}
		else
		{
			if (sDir[0] == '.')
			{
				hFind = FindFirstFile((_option.getExePath() + "\\" + sDir + "\\" + sPattern).c_str(), &FindFileData);
				sDirectory = _option.getExePath() + "/" + sDir;
			}
			else if (sDir[0] == '<')
			{
				if (sDir.substr(0, 10) == "<loadpath>")
				{
					hFind = FindFirstFile((_option.getLoadPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getLoadPath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 10) == "<savepath>")
				{
					hFind = FindFirstFile((_option.getSavePath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getSavePath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 12) == "<scriptpath>")
				{
					hFind = FindFirstFile((_option.getScriptPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getScriptPath() + sDir.substr(12);
				}
				else if (sDir.substr(0, 10) == "<plotpath>")
				{
					hFind = FindFirstFile((_option.getPlotOutputPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getPlotOutputPath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 10) == "<procpath>")
				{
					hFind = FindFirstFile((_option.getProcsPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getProcsPath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 4) == "<wp>")
				{
					hFind = FindFirstFile((_option.getWorkPath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getWorkPath() + sDir.substr(10);
				}
				else if (sDir.substr(0, 2) == "<>" || sDir.substr(0, 6) == "<this>")
				{
					hFind = FindFirstFile((_option.getExePath() + "\\" + sDir.substr(sDir.find('>') + 1) + "\\" + sPattern).c_str(), &FindFileData);
					sDirectory = _option.getExePath() + sDir.substr(sDir.find('>') + 1);
				}
			}
			else
			{
				hFind = FindFirstFile((sDir + "\\" + sPattern).c_str(), &FindFileData);
				sDirectory = sDir;
			}
		}
		if (hFind == INVALID_HANDLE_VALUE)
			return false;

		do
		{
			sFilesize = " Bytes";
			sConnect = "|   ";
			sConnect += FindFileData.cFileName;
			sFileName = sDirectory + "/" + FindFileData.cFileName;
			if (sConnect.length() + 3 > nFirstColLength) //31
				sConnect = sConnect.substr(0, nFirstColLength - 14) + "..." + sConnect.substr(sConnect.length() - 8); //20
			nLength = sConnect.length();
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (n)
					continue;
				if (sConnect.substr(sConnect.length() - 2) == ".." || sConnect.substr(sConnect.length() - 1) == ".")
					continue;
				nCount[1]++;
				sConnect += "  (...)";
				sConnect.append(nFirstColLength - 1 - nLength, ' ');
				sConnect += "<" + _lang.get("BUILTIN_LISTFILES_CUSTOMPATH") + ">";
			}
			else if (!bOnlyDir && n)
			{
				nCount[0]++;
				Filesize.LowPart = FindFileData.nFileSizeLow;
				Filesize.HighPart = FindFileData.nFileSizeHigh;
				string sExt = "";
				if (sConnect.find('.') != string::npos)
					sExt = toLowerCase(sConnect.substr(sConnect.rfind('.'), sConnect.find(' ', sConnect.rfind('.')) - sConnect.rfind('.')));
				sConnect.append(nFirstColLength + 7 - nLength, ' ');
				if (!sExt.length())
					sConnect += _lang.get("COMMON_FILETYPE_NOEXT");
				else if (sExt == ".dat")
					sConnect += _lang.get("COMMON_FILETYPE_DAT");
				else if (sExt == ".nscr")
					sConnect += _lang.get("COMMON_FILETYPE_NSCR");
				else if (sExt == ".nhlp")
					sConnect += _lang.get("COMMON_FILETYPE_NHLP");
				else if (sExt == ".nlng")
					sConnect += _lang.get("COMMON_FILETYPE_NLNG");
				else if (sExt == ".hlpidx")
					sConnect += _lang.get("COMMON_FILETYPE_HLPIDX");
				else if (sExt == ".labx")
					sConnect += _lang.get("COMMON_FILETYPE_LABX");
				else if (sExt == ".jdx" || sExt == ".dx" || sExt == ".jcm")
					sConnect += _lang.get("COMMON_FILETYPE_JDX");
				else if (sExt == ".ibw")
					sConnect += _lang.get("COMMON_FILETYPE_IBW");
				else if (sExt == ".png")
					sConnect += _lang.get("COMMON_FILETYPE_PNG");
				else if (sExt == ".tex")
					sConnect += _lang.get("COMMON_FILETYPE_TEX");
				else if (sExt == ".eps")
					sConnect += _lang.get("COMMON_FILETYPE_EPS");
				else if (sExt == ".gif")
					sConnect += _lang.get("COMMON_FILETYPE_GIF");
				else if (sExt == ".svg")
					sConnect += _lang.get("COMMON_FILETYPE_SVG");
				else if (sExt == ".zip")
					sConnect += _lang.get("COMMON_FILETYPE_ZIP");
				else if (sExt == ".dll")
					sConnect += _lang.get("COMMON_FILETYPE_DLL");
				else if (sExt == ".exe")
					sConnect += _lang.get("COMMON_FILETYPE_EXE");
				else if (sExt == ".ini")
					sConnect += _lang.get("COMMON_FILETYPE_INI");
				else if (sExt == ".txt")
					sConnect += _lang.get("COMMON_FILETYPE_TXT");
				else if (sExt == ".def")
					sConnect += _lang.get("COMMON_FILETYPE_DEF");
				else if (sExt == ".csv")
					sConnect += _lang.get("COMMON_FILETYPE_CSV");
				else if (sExt == ".back")
					sConnect += _lang.get("COMMON_FILETYPE_BACK");
				else if (sExt == ".cache")
					sConnect += _lang.get("COMMON_FILETYPE_CACHE");
				else if (sExt == ".ndat")
					sConnect += _lang.get("COMMON_FILETYPE_NDAT");
				else if (sExt == ".nprc")
					sConnect += _lang.get("COMMON_FILETYPE_NPRC");
				else if (sExt == ".ndb")
					sConnect += _lang.get("COMMON_FILETYPE_NDB");
				else if (sExt == ".log")
					sConnect += _lang.get("COMMON_FILETYPE_LOG");
				else if (sExt == ".vfm")
					sConnect += _lang.get("COMMON_FILETYPE_VFM");
				else if (sExt == ".plugins")
					sConnect += _lang.get("COMMON_FILETYPE_PLUGINS");
				else if (sExt == ".ods")
					sConnect += _lang.get("COMMON_FILETYPE_ODS");
				else if (sExt == ".xls")
					sConnect += _lang.get("COMMON_FILETYPE_XLS");
				else if (sExt == ".xlsx")
					sConnect += _lang.get("COMMON_FILETYPE_XLSX");
				else if (sExt == ".wave" || sExt == ".wav")
					sConnect += _lang.get("COMMON_FILETYPE_WAV");
				else
					sConnect += toUpperCase(sConnect.substr(sConnect.rfind('.') + 1, sConnect.find(' ', sConnect.rfind('.')) - sConnect.rfind('.') - 1)) + "-" + _lang.get("COMMON_FILETYPE_NOEXT");

				dFilesize = (double)Filesize.QuadPart;
				dFilesizeTotal += dFilesize;
				if (dFilesize / 1000.0 >= 1)
				{
					dFilesize /= 1024.0;
					sFilesize = "KBytes";
					if (dFilesize / 1000.0 >= 1)
					{
						dFilesize /= 1024.0;
						sFilesize = "MBytes";
						if (dFilesize / 1000.0 >= 1)
						{
							dFilesize /= 1024.0;
							sFilesize = "GBytes";
						}
					}
				}
				sFilesize = toString(dFilesize, 3) + " " + sFilesize;
				sConnect.append(_option.getWindow() - sConnect.length() - sFilesize.length(), ' ');
				sConnect += sFilesize;
				if (sExt == ".ndat" && _option.getbShowExtendedFileInfo())
				{
					sConnect += "$     ";
					sConnect += getFileInfo(sFileName);
				}
			}
			else
				continue;
			/*if (sConnect.find('$') != string::npos)
			    sConnect.replace(sConnect.find('$'),1,"\\$");*/
			NumeReKernel::printPreFmt(LineBreak(sConnect, _option, false) + "\n");
		}
		while (FindNextFile(hFind, &FindFileData) != 0);
	}
	FindClose(hFind);
	if (nCount[0])
	{
		sFilesize = " Bytes";
		if (dFilesizeTotal / 1000.0 >= 1)
		{
			dFilesizeTotal /= 1024.0;
			sFilesize = "KBytes";
			if (dFilesizeTotal / 1000.0 >= 1)
			{
				dFilesizeTotal /= 1024.0;
				sFilesize = "MBytes";
				if (dFilesizeTotal / 1000.0 >= 1)
				{
					dFilesizeTotal /= 1024.0;
					sFilesize = "GBytes";
				}
			}
		}
		sFilesize = "Total: " + toString(dFilesizeTotal, 3) + " " + sFilesize;
	}
	else
		sFilesize = "";
	string sSummary = "-- " + _lang.get("BUILTIN_LISTFILES_SUMMARY", toString(nCount[0]), toString(nCount[1])) + " --";
	sSummary.append(_option.getWindow() - sSummary.length() - 4 - sFilesize.length(), ' ');
	sSummary += sFilesize;
	if (bOnlyDir)
	{
		if (nCount[1])
			NumeReKernel::print(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_DIR_SUMMARY", toString(nCount[1])) + " --", _option) );
		else
			NumeReKernel::print(LineBreak("|   -- " + _lang.get("BUILTIN_LISTFILES_NODIRS") + " --", _option) );
	}
	else
		NumeReKernel::printPreFmt(LineBreak("|   " + sSummary, _option) + "\n");
	return true;
}

// This function creates new objects: files, directories, procedures and caches
static bool BI_newObject(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
	int nType = 0;
	string sObject = "";
	vector<string> vTokens;
	FileSystem _fSys;
	_fSys.setTokens(_option.getTokenPaths());
	if (_data.containsStringVars(sCmd))
		_data.getStringValues(sCmd);
	if (matchParams(sCmd, "dir", '='))
	{
		nType = 1;
		addArgumentQuotes(sCmd, "dir");
	}
	else if (matchParams(sCmd, "script", '='))
	{
		nType = 2;
		addArgumentQuotes(sCmd, "script");
	}
	else if (matchParams(sCmd, "proc", '='))
	{
		nType = 3;
		addArgumentQuotes(sCmd, "proc");
	}
	else if (matchParams(sCmd, "file", '='))
	{
		nType = 4;
		addArgumentQuotes(sCmd, "file");
	}
	else if (matchParams(sCmd, "plugin", '='))
	{
		nType = 5;
		addArgumentQuotes(sCmd, "plugin");
	}
	else if (matchParams(sCmd, "cache", '='))
	{
		string sReturnVal = "";
		if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
		{
			if (!BI_parseStringArgs(sCmd, sObject, _parser, _data, _option))
				return false;
		}
		else
			sObject = sCmd.substr(matchParams(sCmd, "cache", '=') + 5);
		StripSpaces(sObject);
		//NumeReKernel::print(getNextArgument(sObject, false) );
		if (matchParams(sObject, "free"))
			eraseToken(sObject, "free", false);
		if (sObject.rfind('-') != string::npos)
			sObject.erase(sObject.rfind('-'));
		if (!sObject.length() || !getNextArgument(sObject, false).length())
			return false;
		while (sObject.length() && getNextArgument(sObject, false).length())
		{
			if (_data.isCacheElement(getNextArgument(sObject, false)))
			{
				if (matchParams(sCmd, "free"))
				{
					string sTemp = getNextArgument(sObject, false);
					sTemp.erase(sTemp.find('('));
					_data.deleteBulk(sTemp, 0, _data.getLines(sTemp) - 1, 0, _data.getCols(sTemp) - 1);
					if (sReturnVal.length())
						sReturnVal += ", ";
					sReturnVal += "\"" + getNextArgument(sObject, false) + "\"";
				}
				getNextArgument(sObject, true);
				continue;
			}
			if (_data.addCache(getNextArgument(sObject, false), _option))
			{
				if (sReturnVal.length())
					sReturnVal += ", ";
				sReturnVal += "\"" + getNextArgument(sObject, true) + "\"";
				continue;
			}
			else
				return false;
		}
		if (sReturnVal.length() && _option.getSystemPrintStatus())
		{
			if (matchParams(sCmd, "free"))
				NumeReKernel::print(LineBreak(  _lang.get("BUILTIN_NEW_FREE_CACHES", sReturnVal), _option) );
			else
				NumeReKernel::print(LineBreak(  _lang.get("BUILTIN_NEW_CACHES", sReturnVal), _option) );
		}
		return true;
	}
	else if (sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 3) != string::npos)
	{
		if (sCmd[sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 3)] == '$')
		{
			nType = 3;
			sCmd.insert(sCmd.find_first_not_of(' ', findCommand(sCmd).nPos + 3), "-proc=");
			addArgumentQuotes(sCmd, "proc");
		}
		else if (sCmd.find("()", findCommand(sCmd).nPos + 3) != string::npos)
		{
			string sReturnVal = "";
			if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
			{
				if (!BI_parseStringArgs(sCmd, sObject, _parser, _data, _option))
					return false;
			}
			else
				sObject = sCmd.substr(findCommand(sCmd).nPos + 3);
			StripSpaces(sObject);
			if (matchParams(sObject, "free"))
				eraseToken(sObject, "free", false);
			if (sObject.rfind('-') != string::npos)
				sObject.erase(sObject.rfind('-'));
			//NumeReKernel::print(getNextArgument(sObject, false) );
			if (!sObject.length() || !getNextArgument(sObject, false).length())
				return false;
			while (sObject.length() && getNextArgument(sObject, false).length())
			{
				if (_data.isCacheElement(getNextArgument(sObject, false)))
				{
					if (matchParams(sCmd, "free"))
					{
						string sTemp = getNextArgument(sObject, false);
						sTemp.erase(sTemp.find('('));
						_data.deleteBulk(sTemp, 0, _data.getLines(sTemp) - 1, 0, _data.getCols(sTemp) - 1);
						if (sReturnVal.length())
							sReturnVal += ", ";
						sReturnVal += "\"" + getNextArgument(sObject, false) + "\"";
					}
					getNextArgument(sObject, true);
					continue;
				}
				if (_data.addCache(getNextArgument(sObject, false), _option))
				{
					if (sReturnVal.length())
						sReturnVal += ", ";
					sReturnVal += "\"" + getNextArgument(sObject, true) + "\"";
					continue;
				}
				else
					return false;
			}
			if (sReturnVal.length() && _option.getSystemPrintStatus())
			{
				if (matchParams(sCmd, "free"))
					NumeReKernel::print(LineBreak(  _lang.get("BUILTIN_NEW_FREE_CACHES", sReturnVal), _option) );
				else
					NumeReKernel::print(LineBreak(  _lang.get("BUILTIN_NEW_CACHES", sReturnVal), _option) );
			}
			return true;
		}
	}
	if (!nType)
		return false;
	BI_parseStringArgs(sCmd, sObject, _parser, _data, _option);
	StripSpaces(sObject);
	if (!sObject.length())
		throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);
	if (_option.getbDebug())
		NumeReKernel::print("DEBUG: sObject = " + sObject );

	if (nType == 1)
	{
		int nReturn = _fSys.setPath(sObject, true, _option.getExePath());
		if (nReturn == 1 && _option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_FOLDERCREATED", sObject), _option) );
	}
	else if (nType == 2)
	{
		if (sObject.find('/') != string::npos || sObject.find('\\') != string::npos)
		{
			string sPath = sObject;
			for (unsigned int i = sPath.length() - 1; i >= 0; i--)
			{
				if (sPath[i] == '\\' || sPath[i] == '/')
				{
					sPath = sPath.substr(0, i);
					break;
				}
			}
			_fSys.setPath(sPath, true, _option.getScriptPath());
		}
		else
			_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());
		if (sObject.find('\\') == string::npos && sObject.find('/') == string::npos)
			sObject = "<scriptpath>/" + sObject;
		sObject = _fSys.ValidFileName(sObject, ".nscr");
		vTokens.push_back(sObject.substr(sObject.rfind('/') + 1, sObject.rfind('.') - sObject.rfind('/') - 1));
		vTokens.push_back(getTimeStamp(false));
		if (fileExists(_option.ValidFileName("<>/user/lang/tmpl_script.nlng", ".nlng")))
		{
			if (!generateTemplate(sObject, "<>/user/lang/tmpl_script.nlng", vTokens, _option))
			{
				//sErrorToken = sObject;
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, sObject, sObject);
			}
		}
		else
		{
			if (!generateTemplate(sObject, "<>/lang/tmpl_script.nlng", vTokens, _option))
			{
				//sErrorToken = sObject;
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, sObject, sObject);
			}
		}
		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_SCRIPTCREATED", sObject), _option) );
	}
	else if (nType == 3)
	{
		if (sObject.find('/') != string::npos || sObject.find('\\') != string::npos || sObject.find('~') != string::npos)
		{
			string sPath = sObject;
			for (unsigned int i = sPath.length() - 1; i >= 0; i--)
			{
				if (sPath[i] == '\\' || sPath[i] == '/' || sPath[i] == '~')
				{
					sPath = sPath.substr(0, i);
					break;
				}
			}
			while (sPath.find('~') != string::npos)
				sPath[sPath.find('~')] = '/';
			while (sPath.find('$') != string::npos)
				sPath.erase(sPath.find('$'), 1);
//            NumeReKernel::print(sPath );
			_fSys.setPath(sPath, true, _option.getProcsPath());
		}
		else
			_fSys.setPath(_option.getProcsPath(), false, _option.getExePath());
		string sProcedure = sObject;
		if (sProcedure.find('$') != string::npos)
		{
			sProcedure = sProcedure.substr(sProcedure.rfind('$'));
			if (sProcedure.find('~') != string::npos)
				sProcedure.erase(1, sProcedure.rfind('~'));
		}
		else
		{
			if (sProcedure.find('~') != string::npos)
				sProcedure = sProcedure.substr(sProcedure.rfind('~') + 1);
			if (sProcedure.find('\\') != string::npos)
				sProcedure = sProcedure.substr(sProcedure.rfind('\\') + 1);
			if (sProcedure.find('/') != string::npos)
				sProcedure = sProcedure.substr(sProcedure.rfind('/') + 1);
			StripSpaces(sProcedure);
			sProcedure = "$" + sProcedure;
		}
		if (sProcedure.find('.') != string::npos)
			sProcedure = sProcedure.substr(0, sProcedure.rfind('.'));

		if (sObject.find('\\') == string::npos && sObject.find('/') == string::npos)
			sObject = "<procpath>/" + sObject;
		while (sObject.find('~') != string::npos)
			sObject[sObject.find('~')] = '/';
		while (sObject.find('$') != string::npos)
			sObject.erase(sObject.find('$'), 1);
		sObject = _fSys.ValidFileName(sObject, ".nprc");

		ofstream fProcedure;
		fProcedure.open(sObject.c_str());
		if (fProcedure.fail())
		{
			//sErrorToken = sObject;
			throw SyntaxError(SyntaxError::CANNOT_GENERATE_PROCEDURE, sCmd, sObject, sObject);
		}
		unsigned int nLength = _lang.get("COMMON_PROCEDURE").length();
		fProcedure << "#*********" << std::setfill('*') << std::setw(nLength + 2) << "***" << std::setfill('*') << std::setw(max(21u, sProcedure.length() + 2)) << "*" << endl;
		fProcedure << " * NUMERE-" << toUpperCase(_lang.get("COMMON_PROCEDURE")) << ": " << sProcedure << "()" << endl;
		fProcedure << " * =======" << std::setfill('=') << std::setw(nLength + 2) << "===" << std::setfill('=') << std::setw(max(21u, sProcedure.length() + 2)) << "=" << endl;
		fProcedure << " * " << _lang.get("PROC_ADDED_DATE") << ": " << getTimeStamp(false) << " *#" << endl;
		fProcedure << endl;
		fProcedure << "procedure " << sProcedure << "()" << endl;
		fProcedure << "\t## " << _lang.get("BUILTIN_NEW_ENTERYOURCODE") << endl;
		fProcedure << "\treturn true" << endl;
		fProcedure << "endprocedure" << endl;
		fProcedure << endl;
		fProcedure << "#* " << _lang.get("PROC_END_OF_PROCEDURE") << endl;
		fProcedure << " * " << _lang.get("PROC_FOOTER") << endl;
		fProcedure << " * https://sites.google.com/site/numereframework/" << endl;
		fProcedure << " **" << std::setfill('*') << std::setw(_lang.get("PROC_FOOTER").length() + 1) << "#" << endl;


		fProcedure.close();

		/*vTokens.push_back(sProcedure);
		vTokens.push_back(getTimeStamp(false));
		if (!BI_generateTemplate(sObject, "<>/lang/tmpl_proc.nlng", vTokens, _option))
		{
		    sErrorToken = sObject;
		    throw CANNOT_GENERATE_PROCEDURE;
		}*/

		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_PROCCREATED", sObject), _option) );
	}
	else if (nType == 4)
	{
		if (sObject.find('/') != string::npos || sObject.find('\\') != string::npos)
		{
			string sPath = sObject;
			for (unsigned int i = sPath.length() - 1; i >= 0; i--)
			{
				if (sPath[i] == '\\' || sPath[i] == '/')
				{
					sPath = sPath.substr(0, i);
					break;
				}
			}
			_fSys.setPath(sPath, true, _option.getExePath());
		}
		else
			_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());
		if (sObject.find('\\') == string::npos && sObject.find('/') == string::npos)
			sObject = "<>/" + sObject;
		sObject = _fSys.ValidFileName(sObject, ".txt");

		if (sObject.substr(sObject.rfind('.')) == ".nprc"
				|| sObject.substr(sObject.rfind('.')) == ".nscr"
				|| sObject.substr(sObject.rfind('.')) == ".ndat")
			sObject.replace(sObject.rfind('.'), 5, ".txt");
		vTokens.push_back(sObject.substr(sObject.rfind('/') + 1, sObject.rfind('.') - sObject.rfind('/') - 1));
		vTokens.push_back(getTimeStamp(false));
		if (fileExists(_option.ValidFileName("<>/user/lang/tmpl_file.nlng", ".nlng")))
		{
			if (!generateTemplate(sObject, "<>/user/lang/tmpl_file.nlng", vTokens, _option))
			{
				//sErrorToken = sObject;
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_FILE, sCmd, SyntaxError::invalid_position, sObject);
			}
		}
		else
		{
			if (!generateTemplate(sObject, "<>/lang/tmpl_file.nlng", vTokens, _option))
			{
				//sErrorToken = sObject;
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_FILE, sCmd, SyntaxError::invalid_position, sObject);
			}
		}
		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_FILECREATED", sObject), _option) );
	}
	else if (nType == 5)
	{
		if (sObject.find('/') != string::npos || sObject.find('\\') != string::npos)
		{
			string sPath = sObject;
			for (unsigned int i = sPath.length() - 1; i >= 0; i--)
			{
				if (sPath[i] == '\\' || sPath[i] == '/')
				{
					sPath = sPath.substr(0, i);
					break;
				}
			}
			_fSys.setPath(sPath, true, _option.getScriptPath());
		}
		else
			_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());
		if (sObject.find('\\') == string::npos && sObject.find('/') == string::npos)
			sObject = "<scriptpath>/" + sObject;
		sObject = _fSys.ValidFileName(sObject, ".nscr");
		if (sObject.substr(sObject.rfind('/') + 1, 5) != "plgn_")
			sObject.insert(sObject.rfind('/') + 1, "plgn_");
		while (sObject.find(' ', sObject.rfind('/')) != string::npos)
			sObject.erase(sObject.find(' ', sObject.rfind('/')), 1);

		string sPluginName = sObject.substr(sObject.rfind("plgn_") + 5, sObject.rfind('.') - sObject.rfind("plgn_") - 5);
		vTokens.push_back(sPluginName);
		vTokens.push_back(getTimeStamp(false));
		if (fileExists(_option.ValidFileName("<>/user/lang/tmpl_plugin.nlng", ".nlng")))
		{
			if (!generateTemplate(sObject, "<>/user/lang/tmpl_plugin.nlng", vTokens, _option))
			{
				//sErrorToken = sObject;
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, SyntaxError::invalid_position, sObject);
			}
		}
		else
		{
			if (!generateTemplate(sObject, "<>/lang/tmpl_plugin.nlng", vTokens, _option))
			{
				//sErrorToken = sObject;
				throw SyntaxError(SyntaxError::CANNOT_GENERATE_SCRIPT, sCmd, SyntaxError::invalid_position, sObject);
			}
		}
		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak( _lang.get("BUILTIN_NEW_PLUGINCREATED", sPluginName, sObject), _option) );
	}
	return true;
}

// This function opens the object to edit its contents
static bool BI_editObject(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
	int nType = 0;
	int nFileOpenFlag = 0;
	//NumeReKernel::print(sCmd );

	if (matchParams(sCmd, "norefresh"))
	{
		nFileOpenFlag = 1;
	}
	if (matchParams(sCmd, "refresh"))
	{
		nFileOpenFlag = 2 | 4;
	}
	string sObject;
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		BI_parseStringArgs(sCmd, sObject, _parser, _data, _option);
	}
	else
	{
		sObject = sCmd.substr(findCommand(sCmd).sString.length());
		// remove flags from object
		if (nFileOpenFlag)
		{
			sObject.erase(sObject.rfind('-'));
		}
	}

	StripSpaces(sObject);
	FileSystem _fSys;
	_fSys.setTokens(_option.getTokenPaths());
	if (sObject.find('.') != string::npos)
		_fSys.declareFileType(sObject.substr(sObject.rfind('.')));

	if (!sObject.length())
		throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);
	if (_option.getbDebug())
		NumeReKernel::print("DEBUG: sObject = " + sObject );
	if (sObject[0] == '$'  && sObject[1] != '\'')
	{
		sObject = "<procpath>/" + sObject.substr(1);
	}
	else if (sObject[0] == '$')
	{
		sObject.erase(0, 1);
	}
	while (sObject.find('~') != string::npos)
		sObject[sObject.find('~')] = '/';
	while (sObject.find('$') != string::npos)
		sObject.erase(sObject.find('$'), 1);
	if (sObject[0] == '\'' && sObject[sObject.length() - 1] == '\'')
		sObject = sObject.substr(1, sObject.length() - 2);


	if (sObject.find("<loadpath>") != string::npos || sObject.find(_option.getLoadPath()) != string::npos)
	{
		_fSys.setPath(_option.getLoadPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".dat");
	}
	else if (sObject.find("<savepath>") != string::npos || sObject.find(_option.getSavePath()) != string::npos)
	{
		_fSys.setPath(_option.getSavePath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".dat");
	}
	else if (sObject.find("<scriptpath>") != string::npos || sObject.find(_option.getScriptPath()) != string::npos)
	{
		_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".nscr");
	}
	else if (sObject.find("<plotpath>") != string::npos || sObject.find(_option.getPlotOutputPath()) != string::npos)
	{
		_fSys.setPath(_option.getPlotOutputPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".png");
	}
	else if (sObject.find("<procpath>") != string::npos || sObject.find(_option.getProcsPath()) != string::npos)
	{
		_fSys.setPath(_option.getProcsPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".nprc");
	}
	else if (sObject.find("<wp>") != string::npos || sObject.find(_option.getWorkPath()) != string::npos)
	{
		_fSys.setPath(_option.getWorkPath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".nprc");
	}
	else if (sObject.find("<>") != string::npos || sObject.find("<this>") != string::npos || sObject.find(_option.getExePath()) != string::npos)
	{
		/*if (sObject[sObject.length()-1] != '*' && sObject[sObject.length()-1] != '/' && sObject.find('.') == string::npos)
		    sObject += "*";*/
		_fSys.setPath(_option.getExePath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".dat");
	}
	else if (!_data.containsTablesOrClusters(sObject))
	{
		if (sObject.find('.') == string::npos && (sObject.find('/') != string::npos || sObject.find('\\') != string::npos))
		{
			ShellExecute(NULL, NULL, sObject.c_str(), NULL, NULL, SW_SHOWNORMAL);
			return true;
		}
		if (sObject[sObject.length() - 1] != '*' && sObject.find('.') == string::npos)
			sObject += "*";
		if (sObject.find('.') != string::npos)
		{
			if (sObject.substr(sObject.rfind('.')) == ".dat" || sObject.substr(sObject.rfind('.')) == ".txt")
			{
				_fSys.setPath(_option.getLoadPath(), false, _option.getExePath());
				string sTemporaryObjectName = _fSys.ValidFileName(sObject, ".dat");
				if (!BI_FileExists(sTemporaryObjectName))
					_fSys.setPath(_option.getSavePath(), false, _option.getExePath());
			}
			else if (sObject.substr(sObject.rfind('.')) == ".nscr")
				_fSys.setPath(_option.getScriptPath(), false, _option.getExePath());
			else if (sObject.substr(sObject.rfind('.')) == ".nprc")
				_fSys.setPath(_option.getProcsPath(), false, _option.getExePath());
			else if (sObject.substr(sObject.rfind('.')) == ".png"
					 || sObject.substr(sObject.rfind('.')) == ".gif"
					 || sObject.substr(sObject.rfind('.')) == ".svg"
					 || sObject.substr(sObject.rfind('.')) == ".eps")
				_fSys.setPath(_option.getPlotOutputPath(), false, _option.getExePath());
			else if (sObject.substr(sObject.rfind('.')) == ".tex")
			{
				_fSys.setPath(_option.getPlotOutputPath(), false, _option.getExePath());
				string sTemporaryObjectName = _fSys.ValidFileName(sObject, ".tex");
				if (!BI_FileExists(sTemporaryObjectName))
					_fSys.setPath(_option.getSavePath(), false, _option.getExePath());
			}
			else if (sObject.substr(sObject.rfind('.')) == ".nhlp")
			{
				_fSys.setPath(_option.getExePath() + "/docs", false, _option.getExePath());
			}
			else
				_fSys.setPath(_option.getExePath(), false, _option.getExePath());
		}
		else
			_fSys.setPath(_option.getExePath(), false, _option.getExePath());
		sObject = _fSys.ValidFileName(sObject, ".dat");
	}
	if (_option.getbDebug())
		NumeReKernel::print("DEBUG: sObject = " + sObject );
	if (!_data.containsTablesOrClusters(sObject) && sObject.find('.') == string::npos && (sObject.find('/') != string::npos || sObject.find('\\') != string::npos))
	{
		ShellExecute(NULL, NULL, sObject.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return true;
	}
	if (_data.containsTablesOrClusters(sObject))
	{
		StripSpaces(sObject);
		string sTableName = sObject.substr(0, sObject.find('('));

		NumeReKernel::showTable(_data.extractTable(sTableName), sTableName, true);
		NumeReKernel::printPreFmt("|-> " + _lang.get("BUILTIN_WAITINGFOREDIT") + " ... ");

		NumeRe::Table _table = NumeReKernel::getTable();
		NumeReKernel::printPreFmt(_lang.get("COMMON_DONE") + ".\n");

		if (_table.isEmpty())
            return true;

        _data.importTable(_table, sTableName);
        return true;
	}
	if (!BI_FileExists(sObject) || sObject.find('.') == string::npos)
	{
		sObject.erase(sObject.rfind('.'));
		if (sObject.find('*') != string::npos)
			sObject.erase(sObject.rfind('*'));
		if ((int)ShellExecute(NULL, NULL, sObject.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32)
			return true;
		throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sCmd, SyntaxError::invalid_position, sObject);
	}

	if (sObject.substr(sObject.rfind('.')) == ".dat"
			|| sObject.substr(sObject.rfind('.')) == ".txt"
			|| sObject.substr(sObject.rfind('.')) == ".tex"
			|| sObject.substr(sObject.rfind('.')) == ".csv"
			|| sObject.substr(sObject.rfind('.')) == ".labx"
			|| sObject.substr(sObject.rfind('.')) == ".jdx"
			|| sObject.substr(sObject.rfind('.')) == ".jcm"
			|| sObject.substr(sObject.rfind('.')) == ".dx"
			|| sObject.substr(sObject.rfind('.')) == ".nscr"
			|| sObject.substr(sObject.rfind('.')) == ".nprc"
			|| sObject.substr(sObject.rfind('.')) == ".nhlp"
			|| sObject.substr(sObject.rfind('.')) == ".png"
			|| sObject.substr(sObject.rfind('.')) == ".gif"
			|| sObject.substr(sObject.rfind('.')) == ".m"
			|| sObject.substr(sObject.rfind('.')) == ".cpp"
			|| sObject.substr(sObject.rfind('.')) == ".cxx"
			|| sObject.substr(sObject.rfind('.')) == ".c"
			|| sObject.substr(sObject.rfind('.')) == ".hpp"
			|| sObject.substr(sObject.rfind('.')) == ".hxx"
			|| sObject.substr(sObject.rfind('.')) == ".h"
			|| sObject.substr(sObject.rfind('.')) == ".log")
		nType = 1;
	else if (sObject.substr(sObject.rfind('.')) == ".svg"
			 || sObject.substr(sObject.rfind('.')) == ".eps")
		nType = 2;
	if (!nType)
	{
		//sErrorToken = sObject;
		throw SyntaxError(SyntaxError::CANNOT_EDIT_FILE_TYPE, sCmd, SyntaxError::invalid_position, sObject);
	}

	if (nType == 1)
	{
		NumeReKernel::nOpenFileFlag = nFileOpenFlag;
		NumeReKernel::gotoLine(sObject);
		//NumeReKernel::setFileName(sObject);
		//openExternally(sObject, _option.getEditorPath(), _option.getExePath());
	}
	else if (nType == 2)
	{
		openExternally(sObject, _option.getViewerPath(), _option.getExePath());
	}

	return true;
}

// This function returns a list of the current defined variables
static string BI_getVarList(const string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
	mu::varmap_type mNumVars = _parser.GetVar();
	map<string, string> mStringVars = _data.getStringVars();
	map<string, int> mVars;

	string sSep = ", ";
	string sReturn = "";

	for (auto iter = mNumVars.begin(); iter != mNumVars.end(); ++iter)
	{
		mVars[iter->first] = 0;
	}
	for (auto iter = mStringVars.begin(); iter != mStringVars.end(); ++iter)
	{
		mVars[iter->first] = 1;
	}

	if (matchParams(sCmd, "asstr"))
	{
		sSep = "\", \"";
		sReturn = "\"";
	}

	if (findCommand(sCmd).sString == "vars")
	{
		for (auto iter = mVars.begin(); iter != mVars.end(); ++iter)
		{
			sReturn += iter->first + " = ";
			if (iter->second)
			{
				if (matchParams(sCmd, "asstr"))
				{
					sReturn += "\\\"" + mStringVars[iter->first] + "\\\"";
				}
				else
				{
					sReturn += "\"" + mStringVars[iter->first] + "\"";
				}
			}
			else
			{
				sReturn += toString(*mNumVars[iter->first], _option);
			}
			sReturn += sSep;
		}
	}
	if (findCommand(sCmd).sString == "strings")
	{
		for (auto iter = mStringVars.begin(); iter != mStringVars.end(); ++iter)
		{
			sReturn += iter->first + " = ";
			if (matchParams(sCmd, "asstr"))
			{
				sReturn += "\\\"" + iter->second + "\\\"";
			}
			else
			{
				sReturn += "\"" + iter->second + "\"";
			}
			sReturn += sSep;
		}
		if (sReturn == "\"")
			return "\"\"";
	}
	if (findCommand(sCmd).sString == "nums")
	{
		for (auto iter = mNumVars.begin(); iter != mNumVars.end(); ++iter)
		{
			sReturn += iter->first + " = ";
			sReturn += toString(*iter->second, _option);
			sReturn += sSep;
		}
	}

	if (matchParams(sCmd, "asstr") && sReturn.length() > 2)
		sReturn.erase(sReturn.length() - 3);
	else if (!matchParams(sCmd, "asstr") && sReturn.length() > 1)
		sReturn.erase(sReturn.length() - 2);
	return sReturn;
}

// execute "C:\Program Files (x86)\Notepad++\notepad++.exe" -set params="Path/to/file.txt"
static bool BI_executeCommand(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	if (!_option.getUseExecuteCommand())
		throw SyntaxError(SyntaxError::EXECUTE_COMMAND_DISABLED, sCmd, "execute");

	sCmd = BI_evalParamString(sCmd, _parser, _data, _option, _functions);
	FileSystem _fSys;
	_fSys.setTokens(_option.getTokenPaths());
	_fSys.setPath(_option.getExePath(), false, _option.getExePath());
	_fSys.declareFileType(".exe");
	string sParams = "";
	string sWorkpath = "";
	string sObject = "";
	int nRetVal = 0;
	bool bWaitForTermination = false;

	if (matchParams(sCmd, "params", '='))
	{
		sParams = "\"" + getArgAtPos(sCmd, matchParams(sCmd, "params", '=') + 6) + "\"";
	}
	if (matchParams(sCmd, "wp", '='))
	{
		sWorkpath = "\"" + getArgAtPos(sCmd, matchParams(sCmd, "wp", '=') + 2) + "\"";
	}
	if (matchParams(sCmd, "wait"))
		bWaitForTermination = true;

	sObject = sCmd.substr(findCommand(sCmd).sString.length());
	if (sParams.length() || bWaitForTermination || sWorkpath.length())
	{
		if (sCmd.find("-set") != string::npos && sObject.find("-set") != string::npos && !isInQuotes(sCmd, sCmd.find("-set")))
			sObject.erase(sObject.find("-set"));
		else if (sCmd.find("--") != string::npos && sObject.find("--") != string::npos && !isInQuotes(sCmd, sCmd.find("--")))
			sObject.erase(sObject.find("--"));
		else
			throw SyntaxError(SyntaxError::EXECUTE_COMMAND_UNSUCCESSFUL, sCmd, "execute"); // throw an unsuccessful, if the parameters are not clearly identified
	}

	if (containsStrings(sObject) || _data.containsStringVars(sObject))
	{
		string sDummy = "";
		parser_StringParser(sObject, sDummy, _data, _parser, _option, true);
	}
	if (containsStrings(sParams) || _data.containsStringVars(sParams))
	{
		string sDummy = "";
		sParams += " -nq";
		parser_StringParser(sParams, sDummy, _data, _parser, _option, true);
	}
	if (containsStrings(sWorkpath) || _data.containsStringVars(sWorkpath))
	{
		string sDummy = "";
		sWorkpath += " -nq";
		parser_StringParser(sWorkpath, sDummy, _data, _parser, _option, true);
	}

	if (sObject.find('<') != string::npos && sObject.find('>', sObject.find('<') + 1) != string::npos)
		sObject = _fSys.ValidFileName(sObject, ".exe");
	if (sParams.find('<') != string::npos && sParams.find('>', sParams.find('<') + 1) != string::npos)
	{
		if (sParams.front() == '"')
			sParams = "\"" + _fSys.ValidFileName(sParams.substr(1));
		else
			sParams = _fSys.ValidFileName(sParams);

	}
	if (sWorkpath.find('<') != string::npos && sWorkpath.find('>', sWorkpath.find('<') + 1) != string::npos)
	{
		if (sWorkpath.front() == '"')
			sWorkpath = "\"" + _fSys.ValidFileName(sWorkpath.substr(1));
		else
			sWorkpath = _fSys.ValidFileName(sWorkpath);
		if (sWorkpath.rfind(".dat") != string::npos)
			sWorkpath.erase(sWorkpath.rfind(".dat"), 4);
	}
	StripSpaces(sObject);

	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = sObject.c_str();
	ShExecInfo.lpParameters = sParams.c_str();
	ShExecInfo.lpDirectory = sWorkpath.c_str();
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;

	nRetVal = ShellExecuteEx(&ShExecInfo);

	if (!nRetVal)
		throw SyntaxError(SyntaxError::EXECUTE_COMMAND_UNSUCCESSFUL, sCmd, "execute");

	if (bWaitForTermination)
	{
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt("|-> " + _lang.get("COMMON_EVALUATING") + " ... ");
		while (bWaitForTermination)
		{
			// wait 1sec and check, whether the user pressed the ESC key
			if (WaitForSingleObject(ShExecInfo.hProcess, 1000) == WAIT_OBJECT_0)
				break;
			if (NumeReKernel::GetAsyncCancelState())
			{
				if (_option.getSystemPrintStatus())
					NumeReKernel::printPreFmt(_lang.get("COMMON_CANCEL") + "\n");
				throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
			}
		}
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt(_lang.get("COMMON_DONE") + ".\n");
	}
	return true;
}

/*
 * Das waren alle Built-In-Funktionen
 */
