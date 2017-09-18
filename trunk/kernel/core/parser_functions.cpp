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


#include "parser_functions.hpp"
#include "../kernel.hpp"

value_type vAns;
Integration_Vars parser_iVars;
//extern bool bSupressAnswer;
extern mglGraph _fontData;
Plugin _plugin;

const string sParserVersion = "1.0.2";
void parser_ReplaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement);
void parser_ReplaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, Datafile& _data, Parser& _parser, const Settings& _option);


void printUnits(const string& sUnit, const string& sDesc, const string& sDim, const string& sValues, unsigned int nWindowsize)
{
    NumeReKernel::printPreFmt("|     " +strlfill(sUnit, 11) /*std::left*/ +strlfill(sDesc, (nWindowsize-17)/3+(nWindowsize+1)%3) + strlfill(sDim, (nWindowsize-35)/3)+"="+strfill(sValues, (nWindowsize-2)/3)+"\n");
                     ///<< std::setfill(' ') << std::setw((nWindowsize-17)/3+(nWindowsize+1)%3) << std::left << sDesc
                     ///<< std::setfill(' ') << std::setw((nWindowsize-35)/3) << std::left << sDim << "="
                     ///<< std::setfill(' ') << std::setw((nWindowsize-2)/3) << std::right << sValues) << endl;
    return;
}


// --> Pruefen, ob eine Variable (string_type sVar) in einem Ausdruck enthalten ist <--
bool parser_CheckVarOccurence(Parser& _parser, const string_type& sVar)
{
    bool bOccurs = false;

    // --> Auswerte-Methode einmal aufrufen, um den Ausdruck in Bytecode umzuwandeln <--
    _parser.Eval();

    // --> Falls der Ausdruck gar nicht existiert, koennen wir gleich FALSE zurueckgeben <--
    if (!_parser.GetExpr().length())
        return false;

    // --> Generiere eine varmap mit den verwendeten Variablen <--
    varmap_type variables = _parser.GetUsedVar();
    if (!variables.size())
        return false;   // Wenn keine Eintraege in der varmap enthalten sind, kann auch keine Variable vorhanden sein
    else
    {
        // --> Vergleiche alle Eintraege in der varmap mit dem zu findenden Variablen-string <--
        varmap_type::const_iterator item = variables.begin();
        for (; item != variables.end(); ++item)
        {
            if (item->first == sVar)
            {
                bOccurs = true;
                break;
            }
        }
    }
    return bOccurs;
}

// --> Integrations-Funktion in einer Dimension <--
vector<double> parser_Integrate(const string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{

    string sParams = "";        // Parameter-string
    string sInt_Line[4];        // Array, in das alle Eingaben gespeichert werden
    string sLabel = "";
    value_type* v = 0;
    int nResults = 0;
    vector<double> vResult;   // Ausgabe-Wert
    vector<double> fx_n[3]; // Werte an der Stelle n und n+1
    bool bNoIntVar = false;     // Boolean: TRUE, wenn die Funktion eine Konstante der Integration ist
    bool bLargeInterval = false;    // Boolean: TRUE, wenn ueber ein grosses Intervall integriert werden soll
    //bool bDoRoundResults = true;
    bool bReturnFunctionPoints = false;
    bool bCalcXvals = false;
    int nSign = 1;              // Vorzeichen, falls die Integrationsgrenzen getauscht werden muessen
    unsigned int nMethod = 1;    // 1 = trapezoidal, 2 = simpson

    sInt_Line[2] = "1e-3";
    parser_iVars.vValue[0][3] = 1e-3;
    // --> Deklarieren der Integrations-Variablen "x" <--
    //_parser.DefineVar(parser_iVars.sName[0], &parser_iVars.vValue[0][0]);

    if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
    {
        //sErrorToken = "integrate";
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "integrate");
    }

    if (_option.getSystemPrintStatus())
        NumeReKernel::printPreFmt("                                              \r");
    // --> Zunaechst pruefen wir den String sCmd auf Parameter und Funktion <--
    if (sCmd.find("-set") != string::npos)
    {
        sParams = sCmd.substr(sCmd.find("-set"));
        sInt_Line[3] = sCmd.substr(9, sCmd.find("-set")-9);
    }
    else if (sCmd.find("--") != string::npos)
    {
        sParams = sCmd.substr(sCmd.find("--"));
        sInt_Line[3] = sCmd.substr(9, sCmd.find("--")-9);
    }
    else if (sCmd.length() > 9)
        sInt_Line[3] = sCmd.substr(9);
    StripSpaces(sInt_Line[3]);
    if (!sInt_Line[3].length())
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
    if (sInt_Line[3].length() && sInt_Line[3].find("??") != string::npos)
        sInt_Line[3] = parser_Prompt(sInt_Line[3]);
    StripSpaces(sInt_Line[3]);
    if ((sInt_Line[3].substr(0,5) == "data(" || _data.isCacheElement(sInt_Line[3]))
        && getMatchingParenthesis(sInt_Line[3]) != string::npos
        && sInt_Line[3].find_first_not_of(' ', getMatchingParenthesis(sInt_Line[3])+1) == string::npos) // xvals
    {
        if (sParams.length() && matchParams(sParams, "x", '='))
        {
            sInt_Line[0] = getArgAtPos(sParams, matchParams(sParams, "x", '=')+1);
            if (sInt_Line[0].find(':') != string::npos)
                sInt_Line[0].replace(sInt_Line[0].find(':'),1,",");
            _parser.SetExpr(sInt_Line[0]);
            v = _parser.Eval(nResults);
            if (nResults > 1)
                parser_iVars.vValue[0][2] = v[1];
            parser_iVars.vValue[0][1] = v[0];
        }
        if (sParams.length() && matchParams(sParams, "points"))
            bReturnFunctionPoints = true;
        if (sParams.length() && matchParams(sParams, "xvals"))
            bCalcXvals = true;
        string sDatatable = sInt_Line[3].substr(0,sInt_Line[3].find('('));
        Indices _idx = parser_getIndices(sInt_Line[3], _parser, _data, _option);
        if (_idx.vI.size())
        {
            if (_idx.vI.size() == 1 || _idx.vJ.size() == 1)
                vResult.push_back(_data.sum(sDatatable, _idx.vI, _idx.vJ));
            else
            {
                Datafile _cache;
                for (unsigned int i = 0; i < _idx.vI.size(); i++)
                {
                    _cache.writeToCache(i,0,"cache",_data.getElement(_idx.vI[i], _idx.vJ[0], sDatatable));
                    _cache.writeToCache(i,1,"cache",_data.getElement(_idx.vI[i], _idx.vJ[1], sDatatable));
                }
                _cache.sortElements("cache -sort c=1[2]");
                double dResult = 0.0;
                long long int j = 1;
                for (long long int i = 0; i < _cache.getLines("cache",false)-1; i++)//nan-suche
                {
                    j = 1;
                    if (!_cache.isValidEntry(i,1,"cache"))
                        continue;
                    while (!_cache.isValidEntry(i+j,1,"cache"))
                        j++;
                    if (!_cache.isValidEntry(i+j,0,"cache"))
                        break;
                    if (sInt_Line[0].length() && parser_iVars.vValue[0][1] > _cache.getElement(i,0,"cache"))
                        continue;
                    if (sInt_Line[0].length() && parser_iVars.vValue[0][2] < _cache.getElement(i+j,0,"cache"))
                        break;

                    if (!bReturnFunctionPoints && !bCalcXvals)
                        dResult += (_cache.getElement(i,1,"cache")+_cache.getElement(i+j,1,"cache"))/2.0*(_cache.getElement(i+j,0,"cache")-_cache.getElement(i,0,"cache"));
                    else if (bReturnFunctionPoints && !bCalcXvals)
                    {
                        if (vResult.size())
                            vResult.push_back((_cache.getElement(i,1,"cache")+_cache.getElement(i+j,1,"cache"))/2.0*(_cache.getElement(i+j,0,"cache")-_cache.getElement(i,0,"cache"))+vResult.back());
                        else
                            vResult.push_back((_cache.getElement(i,1,"cache")+_cache.getElement(i+j,1,"cache"))/2.0*(_cache.getElement(i+j,0,"cache")-_cache.getElement(i,0,"cache")));
                    }
                    else
                    {
                        //vResult.push_back((_cache.getElement(i+j,0,"cache")+_cache.getElement(i,0,"cache"))/2.0);
                        vResult.push_back(_cache.getElement(i+j,0,"cache"));
                    }
                }
                if (!bReturnFunctionPoints && !bCalcXvals)
                    vResult.push_back(dResult);
            }
        }
        else
        {
            if (_idx.nI[1] == -1 || _idx.nJ[1] == -1)
            {
                parser_evalIndices(sDatatable, _idx, _data);
                vResult.push_back(_data.sum(sDatatable,_idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]));
            }
            else
            {
                parser_evalIndices(sDatatable, _idx, _data);
                Datafile _cache;
                for (long long int i = _idx.nI[0]; i < _idx.nI[1]; i++)
                {
                    _cache.writeToCache(i-_idx.nI[0],0,"cache",_data.getElement(i, _idx.nJ[0], sDatatable));
                    _cache.writeToCache(i-_idx.nI[0],1,"cache",_data.getElement(i, _idx.nJ[1]-1, sDatatable));
                }
                _cache.sortElements("cache -sort c=1[2]");
                double dResult = 0.0;
                long long int j = 1;
                for (long long int i = 0; i < _cache.getLines("cache",false)-1; i++)//nan-suche
                {
                    j = 1;
                    if (!_cache.isValidEntry(i,1,"cache"))
                        continue;
                    while (!_cache.isValidEntry(i+j,1,"cache"))
                        j++;
                    if (!_cache.isValidEntry(i+j,0,"cache"))
                        break;
                    if (sInt_Line[0].length() && parser_iVars.vValue[0][1] > _cache.getElement(i,0,"cache"))
                        continue;
                    if (sInt_Line[0].length() && parser_iVars.vValue[0][2] < _cache.getElement(i+j,0,"cache"))
                        break;

                    if (!bReturnFunctionPoints && !bCalcXvals)
                        dResult += (_cache.getElement(i,1,"cache")+_cache.getElement(i+j,1,"cache"))/2.0*(_cache.getElement(i+j,0,"cache")-_cache.getElement(i,0,"cache"));
                    else if (bReturnFunctionPoints && !bCalcXvals)
                    {
                        if (vResult.size())
                            vResult.push_back((_cache.getElement(i,1,"cache")+_cache.getElement(i+j,1,"cache"))/2.0*(_cache.getElement(i+j,0,"cache")-_cache.getElement(i,0,"cache"))+vResult.back());
                        else
                            vResult.push_back((_cache.getElement(i,1,"cache")+_cache.getElement(i+j,1,"cache"))/2.0*(_cache.getElement(i+j,0,"cache")-_cache.getElement(i,0,"cache")));
                    }
                    else
                    {
                        //vResult.push_back((_cache.getElement(i+j,0,"cache")+_cache.getElement(i,0,"cache"))/2.0);
                        vResult.push_back(_cache.getElement(i+j,0,"cache"));
                    }
                }
                if (!bReturnFunctionPoints)
                    vResult.push_back(dResult);
            }
        }
        return vResult;
    }
    if (sInt_Line[3].find("{") != string::npos)
        parser_VectorToExpr(sInt_Line[3], _option);
    sLabel = sInt_Line[3];
    if (sInt_Line[3].length() && !_functions.call(sInt_Line[3], _option))
    {
        sInt_Line[3] = "";
        sLabel = "";
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
    }
    if (sParams.length())
    {
        int nPos = 0;
        if (matchParams(sParams, "precision", '='))
        {
            nPos = matchParams(sParams, "precision", '=')+9;
            sInt_Line[2] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[2]);
            if (parser_ExprNotEmpty(sInt_Line[2]))
            {
                _parser.SetExpr(sInt_Line[2]);
                parser_iVars.vValue[0][3] = _parser.Eval();
                if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                {
                    vResult.push_back(NAN);
                    return vResult;
                }
                if (!parser_iVars.vValue[0][3])
                    sInt_Line[2] = "";
            }
        }
        if (matchParams(sParams, "p", '='))
        {
            nPos = matchParams(sParams, "p", '=')+1;
            sInt_Line[2] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[2]);
            if (parser_ExprNotEmpty(sInt_Line[2]))
            {
                _parser.SetExpr(sInt_Line[2]);
                parser_iVars.vValue[0][3] = _parser.Eval();
                if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                {
                    vResult.push_back(NAN);
                    return vResult;
                }
                if (!parser_iVars.vValue[0][3])
                    sInt_Line[2] = "";
            }
        }
        if (matchParams(sParams, "eps", '='))
        {
            nPos = matchParams(sParams, "eps", '=')+3;
            sInt_Line[2] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[2]);
            if (parser_ExprNotEmpty(sInt_Line[2]))
            {
                _parser.SetExpr(sInt_Line[2]);
                parser_iVars.vValue[0][3] = _parser.Eval();
                if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                {
                    vResult.push_back(NAN);
                    return vResult;
                }
                if (!parser_iVars.vValue[0][3])
                    sInt_Line[2] = "";
            }
        }
        if (matchParams(sParams, "x", '='))
        {
            nPos = matchParams(sParams, "x", '=')+1;
            sInt_Line[0] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[0]);
            if (sInt_Line[0].find(':') != string::npos)
            {
                sInt_Line[0] = "(" + sInt_Line[0] + ")";
                parser_SplitArgs(sInt_Line[0], sInt_Line[1], ':', _option);
                StripSpaces(sInt_Line[0]);
                StripSpaces(sInt_Line[1]);
                if (parser_ExprNotEmpty(sInt_Line[0]))
                {
                    _parser.SetExpr(sInt_Line[0]);
                    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
                    {
                        sInt_Line[0] = "";
                    }
                    else
                    {
                        parser_iVars.vValue[0][1] = _parser.Eval();
                        if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                        {
                            vResult.push_back(NAN);
                            return vResult;
                        }
                    }
                }
                if (parser_ExprNotEmpty(sInt_Line[1]))
                {
                    _parser.SetExpr(sInt_Line[1]);
                    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
                        sInt_Line[1] = "";
                    else
                    {
                        parser_iVars.vValue[0][2] = _parser.Eval();
                        if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                        {
                            vResult.push_back(NAN);
                            return vResult;
                        }
                    }
                }
                if (sInt_Line[0].length() && sInt_Line[1].length() && parser_iVars.vValue[0][1] == parser_iVars.vValue[0][2])
                    throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
                if (!sInt_Line[0].length() || !sInt_Line[1].length())
                    throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
            }
            else
                throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
        }
        if (matchParams(sParams, "method", '='))
        {
            nPos = matchParams(sParams, "method", '=')+6;
            if (getArgAtPos(sParams, nPos) == "trapezoidal")
                nMethod = 1;
            if (getArgAtPos(sParams, nPos) == "simpson")
                nMethod = 2;
        }
        if (matchParams(sParams, "m", '='))
        {
            nPos = matchParams(sParams, "m", '=')+1;
            if (getArgAtPos(sParams, nPos) == "trapezoidal")
                nMethod = 1;
            if (getArgAtPos(sParams, nPos) == "simpson")
                nMethod = 2;
        }
        if (matchParams(sParams, "steps", '='))
        {
            sInt_Line[2] = getArgAtPos(sParams, matchParams(sParams, "steps", '=')+5);
            _parser.SetExpr(sInt_Line[2]);
            parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1])/_parser.Eval();
        }
        if (matchParams(sParams, "s", '='))
        {
            sInt_Line[2] = getArgAtPos(sParams, matchParams(sParams, "s", '=')+1);
            _parser.SetExpr(sInt_Line[2]);
            parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1])/_parser.Eval();
        }
        /*if (matchParams(sParams, "noround") || matchParams(sParams, "nr"))
            bDoRoundResults = false;*/
        if (matchParams(sParams, "points"))
            bReturnFunctionPoints = true;
        if (matchParams(sParams, "xvals"))
            bCalcXvals = true;
    }

    if (!sInt_Line[3].length())
    {
        // --> Einlesen der zu integrierenden Funktion <--
        do
        {
            do
            {

                NumeReKernel::printPreFmt("|INTEGRATE> f(" + parser_iVars.sName[0] + ") = ");
                NumeReKernel::getline(sInt_Line[3]);
            }
            while (!sInt_Line[3].length()); // Wiederhole so lange, bis eine Eingabe getaetigt wurde
            sLabel = sInt_Line[3];
            // --> Handelt es sich um eine definierte Funktion? <--
        }
        while (!_functions.call(sInt_Line[3], _option));

        if (sInt_Line[3].find("??") != string::npos)
            sInt_Line[3] = parser_Prompt(sInt_Line[3]);
    }
    // --> Preufen, ob die Variable "x" in dem String vorkommt <--
    _parser.SetExpr(sInt_Line[3]);
    if (!parser_CheckVarOccurence(_parser,parser_iVars.sName[0]))
        bNoIntVar = true;       // Nein? Dann setzen wir den Bool auf TRUE und sparen uns viel Rechnung
    _parser.Eval(nResults);
    vResult.resize(nResults);
    for (int i = 0; i < 3; i++)
        fx_n[i].resize(nResults);
    for (int i = 0; i < nResults; i++)
    {
        vResult[i] = 0.0;
        for (int j = 0; j < 3; j++)
            fx_n[j][i] = 0.0;
    }

    // --> Integrationsgrenzen einlesen: Diese koennen entweder einzeln oder in der Form a:b eingegeben werden <--
    if (!sInt_Line[0].length())
    {
        do
        {
            NumeReKernel::printPreFmt("|INTEGRATE> von " + parser_iVars.sName[0] + " = ");
            NumeReKernel::getline(sInt_Line[0]);
            if (sInt_Line[0].find('=') != string::npos)
                sInt_Line[0].erase(0,sInt_Line[0].find('=')+1);

            if (sInt_Line[0].length())
            {
                // --> Pruefen, ob die Grenzen in der Form a:b eingegeben wurden <--
                if (sInt_Line[0].find(':') != string::npos && sInt_Line[0].find(':') != sInt_Line[0].length() - 1 && sInt_Line[0].find(':'))
                {
                    // --> Ja? Dann teile den String an den beiden Punkten ":" in zwei Strings <--
                    sInt_Line[0] = "(" + sInt_Line[0] + ")";
                    parser_SplitArgs(sInt_Line[0], sInt_Line[1], ':', _option);
                    StripSpaces(sInt_Line[0]);
                    StripSpaces(sInt_Line[1]);
                    // --> Strings an den Parser schicken und auswerten <--
                    _parser.SetExpr(sInt_Line[0]);
                    if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
                        parser_iVars.vValue[0][1] = _parser.Eval();
                    else
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_BOUNDARYDEPENDENCE", parser_iVars.sName[0]), _option, true, 0, 12)+"\n");
                        sInt_Line[0] = "";
                        sInt_Line[1] = "";
                    }
                    _parser.SetExpr(sInt_Line[1]);
                    if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
                        parser_iVars.vValue[0][2] = _parser.Eval();
                    else
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_BOUNDARYDEPENDENCE", parser_iVars.sName[0]), _option, true, 0, 12)+"\n");
                        sInt_Line[0] = "";
                        sInt_Line[1] = "";
                    }
                }
                else if(!sInt_Line[0].find(':') || (sInt_Line[0].find(':') == sInt_Line[0].length() - 1 && sInt_Line[0].length() > 1))
                {
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_BOUNDARYINVALID"), _option, true, 0, 12)+"\n");
                    sInt_Line[0] = "";
                }
                else
                {
                    _parser.SetExpr(sInt_Line[0]);

                    // --> Pruefen, ob "x" in den/der Grenze(n) vorkommt. Das koennen wir naemlich nicht zulassen <--
                    if (parser_CheckVarOccurence(_parser,parser_iVars.sName[0]))
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_BOUNDARYDEPENDENCE", parser_iVars.sName[0]), _option, true, 0, 12));
                        sInt_Line[0] = "";
                    }
                    else
                        parser_iVars.vValue[0][1] = _parser.Eval();
                }
            }
            // --> Wiederhole so lange, wie du "x" in dem String findest, oder der String empty ist <--
        }
        while (!sInt_Line[0].length()); // So lange der string empty ist
    }
    if (!sInt_Line[1].length())
    {
        // --> Obere Grenze einlesen <--
        NumeReKernel::printPreFmt("|INTEGRATE> bis " + parser_iVars.sName[0] + " = ");
        do
        {
            NumeReKernel::getline(sInt_Line[1]);
            if (sInt_Line[1].find('=') != string::npos)
                sInt_Line[1].erase(0,sInt_Line[1].find('=')+1);

            if (sInt_Line[1].length())
            {
                _parser.SetExpr(sInt_Line[1]);
                // --> Erneut pruefen, ob "x" in dem String vorkommt <--
                if (parser_CheckVarOccurence(_parser,parser_iVars.sName[0]))
                {
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_BOUNDARYDEPENDENCE", parser_iVars.sName[0]), _option, true, 0, 12)+"\n");
                    sInt_Line[1] = "";
                    NumeReKernel::printPreFmt("|INTEGRATE> bis " + parser_iVars.sName[0] + " = ");
                }
            }
        }
        while (!sInt_Line[1].length()); // So lange auswerten, wie der String empty ist

        // --> String auswerten <--
        parser_iVars.vValue[0][2] = _parser.Eval();
    }

    // --> Pruefen, ob die obere Grenze ggf. kleiner als die untere ist <--
    if (parser_iVars.vValue[0][2] < parser_iVars.vValue[0][1])
    {
        // --> Ja? Dann tauschen wir sie fuer die Berechnung einfach aus <--
        value_type vTemp = parser_iVars.vValue[0][1];
        parser_iVars.vValue[0][1] = parser_iVars.vValue[0][2];
        parser_iVars.vValue[0][2] = vTemp;
        nSign *= -1; // Beachten wir das Tauschen der Grenzen durch ein zusaetzliches Vorzeichen
    }

    // --> Schwerere Loesung: numerisch Integrieren ... <--
    if (!bNoIntVar || bReturnFunctionPoints || bCalcXvals)
    {
        if (sInt_Line[2].length() && parser_iVars.vValue[0][3] > parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1])
            sInt_Line[2] = "";
        if (!sInt_Line[2].length())
        {
            do
            {
                // --> Praezision einlesen: die darf vor allem nicht 0 sein <--
                do
                {
                    NumeReKernel::printPreFmt("|INTEGRATE> Praezision d" + parser_iVars.sName[0] + " = ");
                    NumeReKernel::getline(sInt_Line[2]);
                    if (sInt_Line[2] == "0")
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_PRECISIONGREATERZERO"), _option, true, 0, 12) + "\n");
                    }
                }
                while (!sInt_Line[2].length() || sInt_Line[2] == "0"); // Wiederhole so lange String empty oder identisch 0

                // --> An den Parser schicken und auswerten <--
                _parser.SetExpr(sInt_Line[2]);
                parser_iVars.vValue[0][3] = _parser.Eval();
                // --> Sicherheitshalber noch mal pruefen, falls der Ausdruck in der Auswertung 0 ist <--
                if (!parser_iVars.vValue[0][3])
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_PRECISIONGREATERZERO"), _option, true, 0, 12) + "\n");
                if (parser_iVars.vValue[0][3] > (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]))
                {
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_PRECISIONGREATERINTERVAL"), _option, true, 0, 12) + "\n");
                }
            }
            while (!parser_iVars.vValue[0][3] || parser_iVars.vValue[0][3] > (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1])); // Wiederhole so lange, wie die Praezision identisch 0 ist
        }
        // --> Pruefen, ob die Praezision ggf. kleiner 0 ist. Dann einfach mit -1 multiplizieren <--
        if (parser_iVars.vValue[0][3] < 0)
        {
            parser_iVars.vValue[0][3] *= -1;
        }

        if (bCalcXvals)
        {
            parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1];//+parser_iVars.vValue[0][2]/2.0;
            vResult[0] = parser_iVars.vValue[0][0];
            while (parser_iVars.vValue[0][0] + parser_iVars.vValue[0][3] < parser_iVars.vValue[0][2])
            {
                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3];
                vResult.push_back(parser_iVars.vValue[0][0]);
            }
            return vResult;
        }
        // --> Zu integrierende Funktion an den Parser schicken <--
        _parser.SetExpr(sInt_Line[3]);

        // --> Ist es (datenmaessig) ein recht grosses Intervall? <--
        if ((parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) / parser_iVars.vValue[0][3] >= 9.9e6)
            bLargeInterval = true;
        if ((parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) / parser_iVars.vValue[0][3] > 1e10)
            throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, sCmd, SyntaxError::invalid_position);
        /*if (_option.getSystemPrintStatus())
            cerr << "|INTEGRATE> Werte aus ... 0 %";*/

        // -->  Integrations-Variable auf die linke Grenze setzen <--
        parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1];

        // --> Erste Stuetzstelle auswerten <--
        v = _parser.Eval(nResults);
        for (int i = 0; i < nResults; i++)
            fx_n[0][i] = v[i];

        // --> Eigentliche numerische Integration: Jedes Mal pruefen, ob die Integrationsvariable noch kleiner als die rechte Grenze ist <--
        while (parser_iVars.vValue[0][0] + parser_iVars.vValue[0][3] < parser_iVars.vValue[0][2] + parser_iVars.vValue[0][3]*1e-1)
        {
            if (nMethod == 1)
            {
                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]; // x + dx
                v = _parser.Eval(nResults);    // n+1-te Stuetzstelle auswerten
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[1][i] = v[i];    // n+1-te Stuetzstelle auswerten
                    if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[1][i]))
                        fx_n[1][i] = 0.0;
                }
            }
            else if (nMethod == 2)
            {
                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]/2.0;
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[1][i] = v[i];
                    if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[1][i]))
                        fx_n[1][i] = 0.0;
                }
                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]/2.0;
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[2][i] = v[i];
                    if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[2][i]))
                        fx_n[2][i] = 0.0;
                }
            }
            if (nMethod == 1)
            {
                if (!bReturnFunctionPoints)
                {
                    for (int i = 0; i < nResults; i++)
                    {
                        vResult[i] += parser_iVars.vValue[0][3] * (fx_n[0][i] + fx_n[1][i]) * 0.5; // Durch ein Trapez annaehern!
                        //cerr << vResult[i] << endl;
                    }
                }
                else
                {
                    if (vResult.size())
                        vResult.push_back(parser_iVars.vValue[0][3] * (fx_n[0][0] + fx_n[1][0]) * 0.5 + vResult.back());
                    else
                        vResult.push_back(parser_iVars.vValue[0][3] * (fx_n[0][0] + fx_n[1][0]) * 0.5);
                }
            }
            else if (nMethod == 2)
            {
                if (!bReturnFunctionPoints)
                {
                    for (int i = 0; i < nResults; i++)
                        vResult[i] += parser_iVars.vValue[0][3]/6.0 * (fx_n[0][i] + 4.0*fx_n[1][i] + fx_n[2][i]); // b-a/6*(f(a)+4f(a+b/2)+f(b))
                }
                else
                {
                    if (vResult.size())
                        vResult.push_back(parser_iVars.vValue[0][3]/6.0 * (fx_n[0][0] + 4.0*fx_n[1][0] + fx_n[2][0]) + vResult.back());
                    else
                        vResult.push_back(parser_iVars.vValue[0][3]/6.0 * (fx_n[0][0] + 4.0*fx_n[1][0] + fx_n[2][0]));
                }
            }
            if (nMethod == 1)
            {
                for (int i = 0; i < nResults; i++)
                    fx_n[0][i] = fx_n[1][i];              // Wert der n+1-ten Stuetzstelle an die n-te Stuetzstelle zuweisen
            }
            else if (nMethod == 2)
            {
                for (int i = 0; i < nResults; i++)
                    fx_n[0][i] = fx_n[2][i];
            }
            if (_option.getSystemPrintStatus() && bLargeInterval)
            {
                if (!bLargeInterval)
                {
                    if ((int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) * 20) > (int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][3]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) * 20))
                    {
                        NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 20) * 5) + " %");
                    }
                }
                else
                {
                    if ((int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) * 100) > (int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][3]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) * 100))
                    {
                        NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 100)) + " %");
                    }
                }
                if (NumeReKernel::GetAsyncCancelState())//GetAsyncKeyState(VK_ESCAPE))
                {
                    NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL") + ".\n");
                    throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
                }
            }
        }

        // --> Ergebnis sinnvoll runden! <--
        /*if (bDoRoundResults)
        {
            for (unsigned int i = 0; i < vResult.size(); i++)
            {
                double dExponent = -1.0*floor(log10(abs(vResult[i])));
                if (isnan(dExponent) || isinf(dExponent))
                    continue;
                vResult[i] = vResult[i]*pow(10.0,dExponent) / (parser_iVars.vValue[0][3]);
                vResult[i] = std::round(vResult[i]);
                vResult[i] = nSign * vResult[i] * (parser_iVars.vValue[0][3]) / pow(10.0,dExponent);
            }
        }*/
    }
    else
    {
        // --> Einfache Loesung: Konstante Integrieren <--
        string sTemp = sInt_Line[3];
        sInt_Line[3].erase();
        while (sTemp.length())
            sInt_Line[3] += getNextArgument(sTemp, true) + "*" + parser_iVars.sName[0] + ",";
        sInt_Line[3].erase(sInt_Line[3].length()-1,1);
        //sInt_Line[3] = sInt_Line[3] + "*" + parser_iVars.sName[0]; // Die analytische Loesung ist simpel: const * x
        /*if (_option.getSystemPrintStatus())
        {
            cerr << "|INTEGRATE>" << LineBreak(" Analytische Loesung: F(" + parser_iVars.sName[0] + ") = " + sInt_Line[3], _option, true, 12, 12) << endl;
            cerr << "|INTEGRATE> Werte aus ...";
        }*/
        // --> Neuen Ausdruck an den Parser schicken und Integral gemaess dem Hauptsatz berechnen: F(b) - F(a) <--
        _parser.SetExpr(sInt_Line[3]);
        parser_iVars.vValue[0][0] = parser_iVars.vValue[0][2];
        v = _parser.Eval(nResults);
        for (int i = 0; i < nResults; i++)
            vResult[i] = v[i];
        parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1];

        v = _parser.Eval(nResults);
        for (int i = 0; i < nResults; i++)
            vResult[i] -= v[i];
    }

    // --> Ausgabe des Ergebnisses <--
    if (_option.getSystemPrintStatus() && bLargeInterval)
    {
        //cerr << std::setprecision(_option.getPrecision());
        NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 100 %: " + _lang.get("COMMON_SUCCESS") + "!\n");
        /*cerr << "|INTEGRATE>";
        if (bNoIntVar)
            cerr << LineBreak(" F(" + parser_iVars.sName[0] + ") = " + sInt_Line[3] + " von " + parser_iVars.sName[0] + "=" + sInt_Line[0] + " bis " + sInt_Line[1] + ": Erfolg!", _option, true, 12, 12) << endl;
        else
            cerr << LineBreak(" Integral \"" + sLabel + "\" von " + parser_iVars.sName[0] + "=" + sInt_Line[0] + " bis " + sInt_Line[1] + ": Erfolg!", _option, true, 12, 12) << endl;*/
    }
    // --> Weise das Ergebnis noch an die Variable "ans" zu <--
    /*if (nResults > 1 && !bSupressAnswer)
    {
        //cerr << std::setprecision(_option.getPrecision());
        int nLineBreak = parser_LineBreak(_option);
        cerr << "|-> ans = [";
        for (int i = 0; i < nResults; ++i)
        {
            cerr << std::setfill(' ') << std::setw(_option.getPrecision()+7) << std::setprecision(_option.getPrecision()) << vResult[i];
            if (i < nResults-1)
                cerr << ", ";
            if (nResults + 1 > nLineBreak && !((i+1) % nLineBreak) && i < nResults-1)
                cerr << "...\n|          ";
        }
        cerr << "]" << endl;
    }*/
    return vResult;
}

// --> Integrationsfunktion in 2D <--
vector<double> parser_Integrate_2(const string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
    string __sCmd = findCommand(sCmd).sString;
    string sLabel = "";
    string sParams = "";            // Parameter-string
    string sInt_Line[2][3];         // string-Array fuer die Integralgrenzen
    string sInt_Fct;                // string fuer die zu integrierende Funktion
    value_type* v = 0;
    int nResults = 0;
    vector<double> vResult[3];      // value_type-Array, wobei vResult[0] das eigentliche Ergebnis speichert
                                    // und vResult[1] fuer die Zwischenergebnisse des inneren Integrals ist
    vector<double> fx_n[2][3];          // value_type-Array fuer die jeweiligen Stuetzstellen im inneren und aeusseren Integral
    bool bIntVar[2] = {true, true}; // bool-Array, das speichert, ob und welche Integrationsvariablen in sInt_Fct enthalten sind
    bool bRenewBorder = false;      // bool, der speichert, ob die Integralgrenzen von x oder y abhaengen
    bool bLargeArray = false;       // bool, der TRUE fuer viele Datenpunkte ist
    //bool bDoRoundResults = true;
    int nSign = 1;                  // Vorzeichen-Integer
    unsigned int nMethod = 1;       // trapezoidal = 1, simpson = 2

    sInt_Line[0][2] = "1e-3";
    parser_iVars.vValue[0][3] = 1e-3;
    parser_iVars.vValue[1][3] = 1e-3;

    // --> Deklarieren wir zunaechst die Variablen "x" und "y" fuer den Parser und verknuepfen sie mit C++-Variablen <--
    //_parser.DefineVar(parser_iVars.sName[0], &parser_iVars.vValue[0][0]);
    //_parser.DefineVar(parser_iVars.sName[1], &parser_iVars.vValue[1][0]);


    if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
    {
        //sErrorToken = "integrate";
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "integrate");
    }
    if (_option.getSystemPrintStatus())
        NumeReKernel::printPreFmt("                                              \r");
    // --> Zunaechst pruefen wir den String sCmd auf Parameter und Funktion <--
    if (sCmd.find("-set") != string::npos)
    {
        sParams = sCmd.substr(sCmd.find("-set"));
        sInt_Fct = sCmd.substr(__sCmd.length(), sCmd.find("-set")-__sCmd.length());
    }
    else if (sCmd.find("--") != string::npos)
    {
        sParams = sCmd.substr(sCmd.find("--"));
        sInt_Fct = sCmd.substr(__sCmd.length(), sCmd.find("--")-__sCmd.length());
    }
    else if (sCmd.length() > __sCmd.length())
        sInt_Fct = sCmd.substr(__sCmd.length());
    StripSpaces(sInt_Fct);
    if (!sInt_Fct.length())
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
    if (sInt_Fct.length() && sInt_Fct.find("??") != string::npos)
        sInt_Fct = parser_Prompt(sInt_Fct);
    if (sInt_Fct.find("{") != string::npos)
        parser_VectorToExpr(sInt_Fct, _option);
    sLabel = sInt_Fct;
    if (sInt_Fct.length() && !_functions.call(sInt_Fct, _option))
    {
        sInt_Fct = "";
        sLabel = "";
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
    }
    if (sParams.length())
    {
        int nPos = 0;
        if (matchParams(sParams, "precision", '='))
        {
            nPos = matchParams(sParams, "precision", '=')+9;
            sInt_Line[0][2] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[0][2]);
            if (parser_ExprNotEmpty(sInt_Line[0][2]))
            {
                _parser.SetExpr(sInt_Line[0][2]);
                parser_iVars.vValue[0][3] = _parser.Eval();
                if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                {
                    vResult[0].push_back(NAN);
                    return vResult[0];
                }
                if (!parser_iVars.vValue[0][3])
                    sInt_Line[0][2] = "";
                else
                    parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
            }
        }
        if (matchParams(sParams, "p", '='))
        {
            nPos = matchParams(sParams, "p", '=')+1;
            sInt_Line[0][2] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[0][2]);
            if (parser_ExprNotEmpty(sInt_Line[0][2]))
            {
                _parser.SetExpr(sInt_Line[0][2]);
                parser_iVars.vValue[0][3] = _parser.Eval();
                if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                {
                    vResult[0].push_back(NAN);
                    return vResult[0];
                }
                if (!parser_iVars.vValue[0][3])
                    sInt_Line[0][2] = "";
                else
                    parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
            }
        }
        if (matchParams(sParams, "eps", '='))
        {
            nPos = matchParams(sParams, "eps", '=')+3;
            sInt_Line[0][2] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[0][2]);
            if (parser_ExprNotEmpty(sInt_Line[0][2]))
            {
                _parser.SetExpr(sInt_Line[0][2]);
                parser_iVars.vValue[0][3] = _parser.Eval();
                if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                {
                    vResult[0].push_back(NAN);
                    return vResult[0];
                }
                if (!parser_iVars.vValue[0][3])
                    sInt_Line[0][2] = "";
                else
                    parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
            }
        }
        if (matchParams(sParams, "x", '='))
        {
            nPos = matchParams(sParams, "x", '=')+1;
            sInt_Line[0][0] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[0][0]);
            if (sInt_Line[0][0].find(':') != string::npos)
            {
                sInt_Line[0][0] = "(" + sInt_Line[0][0] + ")";
                parser_SplitArgs(sInt_Line[0][0], sInt_Line[0][1], ':', _option);
                StripSpaces(sInt_Line[0][0]);
                StripSpaces(sInt_Line[0][1]);
                if (parser_ExprNotEmpty(sInt_Line[0][0]))
                {
                    _parser.SetExpr(sInt_Line[0][0]);
                    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]) || parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                    {
                        sInt_Line[0][0] = "";
                    }
                    else
                    {
                        parser_iVars.vValue[0][1] = _parser.Eval();
                        if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                        {
                            vResult[0].push_back(NAN);
                            return vResult[0];
                        }
                    }
                }
                if (parser_ExprNotEmpty(sInt_Line[0][1]))
                {
                    _parser.SetExpr(sInt_Line[0][1]);
                    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]) || parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                        sInt_Line[0][1] = "";
                    else
                    {
                        parser_iVars.vValue[0][2] = _parser.Eval();
                        if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                        {
                            vResult[0].push_back(NAN);
                            return vResult[0];
                        }
                    }
                }
                if (sInt_Line[0][0].length() && sInt_Line[0][1].length() && parser_iVars.vValue[0][1] == parser_iVars.vValue[0][2])
                    throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
                if (!sInt_Line[0][0].length() || !sInt_Line[0][1].length())
                    throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
            }
            else
                throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
        }
        if (matchParams(sParams, "y", '='))
        {
            nPos = matchParams(sParams, "y", '=')+1;
            sInt_Line[1][0] = getArgAtPos(sParams, nPos);
            StripSpaces(sInt_Line[1][0]);
            if (sInt_Line[1][0].find(':') != string::npos)
            {
                sInt_Line[1][0] = "(" + sInt_Line[1][0] + ")";
                parser_SplitArgs(sInt_Line[1][0], sInt_Line[1][1], ':', _option);
                StripSpaces(sInt_Line[1][0]);
                StripSpaces(sInt_Line[1][1]);
                if (parser_ExprNotEmpty(sInt_Line[1][0]))
                {
                    _parser.SetExpr(sInt_Line[1][0]);
                    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                    {
                        sInt_Line[1][0] = "";
                    }
                    else
                    {
                        parser_iVars.vValue[1][1] = _parser.Eval();
                        if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                        {
                            vResult[0].push_back(NAN);
                            return vResult[0];
                        }
                    }
                }
                if (parser_ExprNotEmpty(sInt_Line[1][1]))
                {
                    _parser.SetExpr(sInt_Line[1][1]);
                    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                        sInt_Line[1][1] = "";
                    else
                    {
                        parser_iVars.vValue[1][2] = _parser.Eval();
                        if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
                        {
                            vResult[0].push_back(NAN);
                            return vResult[0];
                        }
                    }
                }
                if (sInt_Line[1][0].length() && sInt_Line[1][1].length() && parser_iVars.vValue[1][1] == parser_iVars.vValue[1][2])
                    throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
                if (!sInt_Line[1][0].length() || !sInt_Line[1][1].length())
                    throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
            }
            else
                throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
        }
        if (matchParams(sParams, "method", '='))
        {
            nPos = matchParams(sParams, "method", '=')+6;
            if (getArgAtPos(sParams, nPos) == "trapezoidal")
                nMethod = 1;
            if (getArgAtPos(sParams, nPos) == "simpson")
                nMethod = 2;
        }
        if (matchParams(sParams, "m", '='))
        {
            nPos = matchParams(sParams, "m", '=')+1;
            if (getArgAtPos(sParams, nPos) == "trapezoidal")
                nMethod = 1;
            if (getArgAtPos(sParams, nPos) == "simpson")
                nMethod = 2;
        }
        if (matchParams(sParams, "steps", '='))
        {
            sInt_Line[0][2] = getArgAtPos(sParams, matchParams(sParams, "steps", '=')+5);
            _parser.SetExpr(sInt_Line[0][2]);
            parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1])/_parser.Eval();
            parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
        }
        if (matchParams(sParams, "s", '='))
        {
            sInt_Line[0][2] = getArgAtPos(sParams, matchParams(sParams, "s", '=')+1);
            _parser.SetExpr(sInt_Line[0][2]);
            parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1])/_parser.Eval();
            parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
        }
        /*if (matchParams(sParams, "noround") || matchParams(sParams, "nr"))
            bDoRoundResults = false;*/
    }


    if (!sInt_Fct.length())
    {
        // --> Einlesen der Funktion f(x,y): do-while, um auf jeden Fall eine nicht-leere Funktion zu integrieren <--
        do
        {
            do
            {
                NumeReKernel::printPreFmt("|INTEGRATE> f(" + parser_iVars.sName[0] + "," + parser_iVars.sName[1] + ") = ");
                NumeReKernel::getline(sInt_Fct);
            }
            while (!sInt_Fct.length()); // So lange, wie der string empty ist
            sLabel = sInt_Fct;
        }
        while (!_functions.call(sInt_Fct, _option));
    }
    if (sInt_Fct.find("??") != string::npos)
        sInt_Fct = parser_Prompt(sInt_Fct);

    // --> Pruefen wir sofort, ob "x" oder "y" in der Funktion enthalten sind und setzen den bool entsprechend <--
    _parser.SetExpr(sInt_Fct);
    if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
        bIntVar[0] = false;
    if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
        bIntVar[1] = false;

    _parser.Eval(nResults);
    for (int i = 0; i < 3; i++)
    {
        vResult[i].resize(nResults);
        fx_n[0][i].resize(nResults);
        fx_n[1][i].resize(nResults);
    }

    for (int i = 0; i < nResults; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            vResult[j][i] = 0.0;
            fx_n[0][j][i] = 0.0;
            fx_n[1][j][i] = 0.0;
        }
    }

    /* --> Einlesen der Grenzen: wie im 1D-Fall koennen die auch im Schema "x_0..x_1" eingegeben werden
     *     do-while, um auf jeden Fall eine nicht-leere Grenze zu haben <--
     *
     * --> Eine Eingabe von "x" oder "y" kann hier ebenfalls nicht zugelassen werden <--
     */
    if (!sInt_Line[0][0].length())
    {
        do
        {
            NumeReKernel::printPreFmt("|INTEGRATE> von " + parser_iVars.sName[0] + " = ");
            NumeReKernel::getline(sInt_Line[0][0]);
            if (sInt_Line[0][0].find('=') != string::npos)
            {
                sInt_Line[0][0].erase(0, sInt_Line[0][0].find('=')+1);
            }
            if (sInt_Line[0][0].length())
            {
                if (sInt_Line[0][0].find(':') != string::npos && sInt_Line[0][0].find(':') != sInt_Line[0][0].length() - 1 && sInt_Line[0][0].find(':'))
                {
                    sInt_Line[0][0] = "(" + sInt_Line[0][0] + ")";
                    parser_SplitArgs(sInt_Line[0][0], sInt_Line[0][1], ':', _option);
                    StripSpaces(sInt_Line[0][0]);
                    StripSpaces(sInt_Line[0][1]);

                    _parser.SetExpr(sInt_Line[0][0]);
                    if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[0]) && !parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                        parser_iVars.vValue[0][1] = _parser.Eval();
                    else
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_BOUNDARYDEPENDENCE", parser_iVars.sName[0], parser_iVars.sName[1]), _option, true, 0, 12)+ "\n");
                        sInt_Line[0][0] = "";
                        sInt_Line[0][1] = "";
                    }
                    _parser.SetExpr(sInt_Line[0][1]);
                    if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[0]) && !parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                        parser_iVars.vValue[0][2] = _parser.Eval();
                    else
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_BOUNDARYDEPENDENCE", parser_iVars.sName[0], parser_iVars.sName[1]), _option, true, 0, 12)+ "\n");
                        sInt_Line[0][0] = "";
                        sInt_Line[0][1] = "";
                    }
                }
                else if (!sInt_Line[0][0].find(':') || (sInt_Line[0][0].find(':') == sInt_Line[0][0].length() - 1 && sInt_Line[0][0].length() > 1))
                {
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTERGRATE_BOUNDARYINVALID"), _option, true, 0, 12)+"\n");
                    sInt_Line[0][0] = "";
                }
                else
                {
                    _parser.SetExpr(sInt_Line[0][0]);
                    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]) || parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_BOUNDARYDEPENDENCE", parser_iVars.sName[0], parser_iVars.sName[1]), _option, true, 0, 12) +"\n");
                        sInt_Line[0][0] = "";
                    }
                }
            }
        }
        while (!sInt_Line[0][0].length());
    }
    // --> Pruefen, ob ".." in dem string_type enthalten ist und ggf. entsprechende Teilung des string_types <--
    if (!sInt_Line[0][1].length())
    {
        // --> Falls die Grenzen nicht im Schema "x_0..x_1" eingegeben wurden, werte die Grenze aus und frage die obere ab <--
        parser_iVars.vValue[0][1] = _parser.Eval();

        // --> Zweite Grenze ebenfalls mit do-while abfragen <--
        do
        {
            NumeReKernel::printPreFmt("|INTEGRATE> bis " + parser_iVars.sName[0] + " = ");
            NumeReKernel::getline(sInt_Line[0][1]);
            if (sInt_Line[0][1].find('=') != string::npos)
                sInt_Line[0][1].erase(0, sInt_Line[0][1].find('=')+1);
            if (sInt_Line[0][1].length())
            {
                _parser.SetExpr(sInt_Line[0][1]);
                if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]) || parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                {
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_BOUNDARYDEPENDENCE", parser_iVars.sName[0], parser_iVars.sName[1]), _option, true, 0, 12)+"\n");
                    sInt_Line[0][1] = "";
                }
            }
        }
        while (!sInt_Line[0][1].length());

        parser_iVars.vValue[0][2] = _parser.Eval();
    }

    // --> Lese nun die y-Grenzen ein: Vorgehen wie oben <--
    if (!sInt_Line[1][0].length())
    {
        do
        {
            NumeReKernel::printPreFmt("|INTEGRATE> von " + parser_iVars.sName[1] + " = ");
            NumeReKernel::getline(sInt_Line[1][0]);
            if (sInt_Line[1][0].find('=') != string::npos)
                sInt_Line[1][0].erase(0,sInt_Line[1][0].find('=')+1);

            if (sInt_Line[1][0].length())
            {
                if (sInt_Line[1][0].find(':') != string::npos && sInt_Line[1][0].find(':') != sInt_Line[1][0].length() - 1 && sInt_Line[1][0].find(':'))
                {
                    sInt_Line[1][0] = "(" + sInt_Line[1][0] + ")";
                    parser_SplitArgs(sInt_Line[1][0], sInt_Line[1][1], ':', _option);
                    StripSpaces(sInt_Line[1][0]);
                    StripSpaces(sInt_Line[1][1]);
                    _parser.SetExpr(sInt_Line[1][0]);
                    if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                        parser_iVars.vValue[1][1] = _parser.Eval();
                    else
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_BOUNDARYSELFDEPENDENCE", parser_iVars.sName[1]), _option, true, 0, 12)+"\n");
                        sInt_Line[1][0] = "";
                        sInt_Line[1][1] = "";
                    }
                    _parser.SetExpr(sInt_Line[1][1]);
                    if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                        parser_iVars.vValue[1][2] = _parser.Eval();
                    else
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_BOUNDARYSELFDEPENDENCE", parser_iVars.sName[1]), _option, true, 0, 12)+"\n");
                        sInt_Line[1][0] = "";
                        sInt_Line[1][1] = "";
                    }
                }
                else if (!sInt_Line[1][0].find(':') || (sInt_Line[1][0].find(':') == sInt_Line[1][0].length() - 1 && sInt_Line[1][0].length() > 1))
                {
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE_BOUNDARYINVALID"), _option, true, 0, 12)+"\n");
                    sInt_Line[1][0] = "";
                }
                else
                {
                    _parser.SetExpr(sInt_Line[1][0]);
                    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                    {
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_BOUNDARYSELFDEPENDENCE", parser_iVars.sName[1]), _option, true, 0, 12)+"\n");
                        sInt_Line[1][0] = "";
                    }
                }
            }
        }
        while (!sInt_Line[1][0].length());
    }
    if (!sInt_Line[1][1].length())
    {
        parser_iVars.vValue[1][1] = _parser.Eval();

        do
        {
            NumeReKernel::printPreFmt("|INTEGRATE> bis " + parser_iVars.sName[1] + " = ");
            NumeReKernel::getline(sInt_Line[1][1]);
            if (sInt_Line[1][1].find('=') != string::npos)
                sInt_Line[1][1].erase(0, sInt_Line[1][1].find('=')+1);

            if (sInt_Line[1][1].length())
            {
                _parser.SetExpr(sInt_Line[1][1]);
                if (parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
                {
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_BOUNDARYSELFDEPENDENCE", parser_iVars.sName[1]), _option, true, 0, 12)+"\n");
                    sInt_Line[1][1] = "";
                }
            }
        }
        while (!sInt_Line[1][1].length());

        parser_iVars.vValue[1][2] = _parser.Eval();
    }

    // --> Sind die x-Integrationsgrenzen ggf. vertauscht? Umdrehen und durch ein Vorzeichen speichern <--
    if (parser_iVars.vValue[0][1] > parser_iVars.vValue[0][2])
    {
        value_type vTemp = parser_iVars.vValue[0][1];
        parser_iVars.vValue[0][1] = parser_iVars.vValue[0][2];
        parser_iVars.vValue[0][2] = vTemp;
        nSign *= -1;
    }

    /* --> Dasselbe fuer die y-Grenzen. Hier sollten wir auch die Strings tauschen, da diese
     *     ggf. nochmals ausgewertet werden muessen <--
     */
    if (parser_iVars.vValue[1][1] > parser_iVars.vValue[1][2])
    {
        value_type vTemp = parser_iVars.vValue[1][1];
        string_type sTemp = sInt_Line[1][0];
        parser_iVars.vValue[1][1] = parser_iVars.vValue[1][2];
        sInt_Line[1][0] = sInt_Line[1][1];
        parser_iVars.vValue[1][2] = vTemp;
        sInt_Line[1][1] = sTemp;
        nSign *= -1;
    }

    // --> Pruefen, ob in den inneren Grenzen ggf "x" enthalten ist <--
    _parser.SetExpr(sInt_Line[1][0] + " + " + sInt_Line[1][1]);
    if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
        bRenewBorder = true;    // Ja? Setzen wir den bool entsprechend

    // --> Okay. Ist wenigstens eine Integrationsvariable in f(x,y) enthalten? <--
    if (bIntVar[0] || bIntVar[1])
    {
        /* --> Ja? Dann brauchen wir auch die Praezision. Komplizierter, da wir zwei do-while's brauchen. Zunaechst
         *     pruefen wir direkt die Eingabe (vorhanden und nicht == "0"), in der aeusseren Schleife weisen wir den
         *     String an den Parser und pruefen dessen Ergebnis. Dies sollte auch nicht == 0 sein <--
         * --> Neue Option: direkte Uebergabe als Command-String. Hierbei muessen wir aber auch kontrollieren, dass die
         *     Praezision nicht groesser als das Integrationsintervall ist. <--
         */
        if (sInt_Line[0][2].length() && (parser_iVars.vValue[0][3] > parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]
                                            || parser_iVars.vValue[0][3] > parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]))
            sInt_Line[0][2] = "";

        if (!sInt_Line[0][2].length())
        {
            do
            {
                do
                {
                    NumeReKernel::printPreFmt("|INTEGRATE> Praezision d" + parser_iVars.sName[0] + ", d" + parser_iVars.sName[1] + " = ");
                    NumeReKernel::getline(sInt_Line[0][2]);
                    if (sInt_Line[0][2] == "0")
                        NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE_PRECISIONGREATERZERO"), _option, true, 0, 12)+"\n");
                }
                while(!sInt_Line[0][2].length() || sInt_Line[0][2] == "0");
                _parser.SetExpr(sInt_Line[0][2]);
                parser_iVars.vValue[0][3] = _parser.Eval();
                if (!parser_iVars.vValue[0][3])
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE_PRECISIONGREATER_ZERO"), _option, true, 0, 12) +"\n");
                if (parser_iVars.vValue[0][3] > (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1])
                    || parser_iVars.vValue[0][3] > (parser_iVars.vValue[1][2]-parser_iVars.vValue[1][1]))
                {
                    NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE_PRECISIONGREATERINTERVAL"), _option, true, 0, 12) + "\n");
                }
            }
            while(!parser_iVars.vValue[0][3] || parser_iVars.vValue[0][3] > (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1])
                    || parser_iVars.vValue[0][3] > (parser_iVars.vValue[1][2]-parser_iVars.vValue[1][1]));
        }
        // --> Ist die Praezision vielleicht kleiner 0? Das koennen wir auch nicht zulassen ... <--
        if (parser_iVars.vValue[0][3] < 0)
            parser_iVars.vValue[0][3] *= -1;

        /* --> Legacy: womoeglich sollen einmal unterschiedliche Praezisionen fuer "x" und "y"
         *     moeglich sein. Inzwischen weisen wir hier einfach mal die Praezision von "x" an
         *     die fuer "y" zu. <--
         */
        parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];

        // --> Haengt die Funktion nur von "y" ab und die Grenzen nicht von "x"? <--
        if ((bIntVar[1] && !bIntVar[0]) && !bRenewBorder)
        {
            /* --> Ja? Dann sind wir frech und tauschen einfach alles aus, da eine Integration nur
             *     ueber "x" deutlich schneller ist. <--
             *
             * --> Dazu muessen wir in der Funktion "y" durch "x" ersetzen, die Werte der Grenzen und
             *     die ihre string_types tauschen. <--
             */
            NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> "+_lang.get("PARSERFUNCS_INTEGRATE2_SWAPVARS", parser_iVars.sName[0], parser_iVars.sName[1]) + " ...", _option, false, 0, 12)+"\n");
            // --> Leerzeichen als "virtuelle Delimiter" hinzufuegen <--
            string_type sTempFct = " " + sInt_Fct + " ";
            sInt_Fct = "";
            do
            {
                /* --> Pruefen wir, ob die Umgebung der gefundenen Variable "y" zu den "Delimitern" gehoert. Anderenfalls
                 *     koennte es sich ja auch um einen Variablennamen handeln. <--
                 */
                if(checkDelimiter(sTempFct.substr(sTempFct.find(parser_iVars.sName[1])-1,parser_iVars.sName[1].length()+2)))
                {
                    int nToReplace = sTempFct.find(parser_iVars.sName[1]);
                    sTempFct.replace(nToReplace, parser_iVars.sName[0].length(), parser_iVars.sName[0]);
                    sInt_Fct += sTempFct.substr(0,nToReplace+2);
                    sTempFct = sTempFct.substr(nToReplace+2);
                }
                else
                {
                    /* --> Selbst wenn die gefunde Stelle sich nicht als Variable "y" erwiesen hat, muessen wir den Substring
                     *     an die Variable sInt_Fct zuweisen, da wir anderenfalls in einen Loop laufen <--
                     */
                    sInt_Fct += sTempFct.substr(0,sTempFct.find(parser_iVars.sName[1]) + 2);
                    sTempFct = sTempFct.substr(sTempFct.find(parser_iVars.sName[1]) + 2);
                }
                // --> Weisen wir den restlichen String an den Parser zu <--
                if (sTempFct.length())
                    _parser.SetExpr(sTempFct);
                else // Anderenfalls koennen wir auch abbrechen; der gesamte String wurde kontrolliert
                    break;
            }
            while (parser_CheckVarOccurence(_parser, parser_iVars.sName[1])); // so lange im restlichen String noch Variablen gefunden werden

            // --> Das Ende des Strings ggf. noch anfuegen <--
            if (sTempFct.length())
                sInt_Fct += sTempFct;
            // --> Ueberzaehlige Leerzeichen entfernen <--
            StripSpaces(sInt_Fct);

            // --> Strings tauschen <--
            string_type sTemp = sInt_Line[0][0];
            sInt_Line[0][0] = sInt_Line[1][0];
            sInt_Line[1][0] = sTemp;
            sTemp = sInt_Line[0][1];
            sInt_Line[0][1] = sInt_Line[1][1];
            sInt_Line[1][1] = sTemp;

            // --> Werte tauschen <---
            value_type vTemp = parser_iVars.vValue[0][1];
            parser_iVars.vValue[0][1] = parser_iVars.vValue[1][1];
            parser_iVars.vValue[1][1] = vTemp;
            vTemp = parser_iVars.vValue[0][2];
            parser_iVars.vValue[0][2] = parser_iVars.vValue[1][2];
            parser_iVars.vValue[1][2] = vTemp;
            bIntVar[0] = true;
            bIntVar[1] = false;
            NumeReKernel::printPreFmt("|INTEGRATE> " + _lang.get("COMMON_SUCCESS") + "!\n");
        }
        // --> Uebergeben wir nun die Integrations-Funktion an den Parser <--
        _parser.SetExpr(sInt_Fct);

        if (((parser_iVars.vValue[0][1]-parser_iVars.vValue[0][0])*(parser_iVars.vValue[1][1]-parser_iVars.vValue[1][0]) / parser_iVars.vValue[0][3] >= 1e3 && bIntVar[0] && bIntVar[1])
            || ((parser_iVars.vValue[0][1]-parser_iVars.vValue[0][0])*(parser_iVars.vValue[1][1]-parser_iVars.vValue[1][0]) / parser_iVars.vValue[0][3] >= 9.9e6 && (bIntVar[0] || bIntVar[1])))
            bLargeArray = true;
        if (((parser_iVars.vValue[0][1]-parser_iVars.vValue[0][0])*(parser_iVars.vValue[1][1]-parser_iVars.vValue[1][0]) / parser_iVars.vValue[0][3] > 1e10 && bIntVar[0] && bIntVar[1])
            || ((parser_iVars.vValue[0][1]-parser_iVars.vValue[0][0])*(parser_iVars.vValue[1][1]-parser_iVars.vValue[1][0]) / parser_iVars.vValue[0][3] > 1e10 && (bIntVar[0] || bIntVar[1])))
            throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, sCmd, SyntaxError::invalid_position);
        // --> Kleine Info an den Benutzer, dass der Code arbeitet <--

        if (_option.getSystemPrintStatus())
            NumeReKernel::printPreFmt("|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 0 %");

        // --> Setzen wir "x" und "y" auf ihre Startwerte <--
        parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1]; // x = x_0
        parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1]; // y = y_0

        // --> Fall: "x" und "y" enthalten. Sehr umstaendlich und aufwaendig zu rechnen <--
        if (bIntVar[0] && bIntVar[1])
        {
            // --> Werte mit den Startwerten die erste Stuetzstelle fuer die y-Integration aus <--
            v = _parser.Eval(nResults);
            for (int i = 0; i < nResults; i++)
                fx_n[1][0][i] = v[i];

            /* --> Berechne das erste y-Integral fuer die erste Stuetzstelle fuer x
             *     Die Schleife laeuft so lange wie y < y_1 <--
             */
            while (parser_iVars.vValue[1][0] + parser_iVars.vValue[1][3] < parser_iVars.vValue[1][2] + parser_iVars.vValue[1][3] * 1e-1)
            {
                if (nMethod == 1)
                {
                    parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]; // y + dy
                    v = _parser.Eval(nResults); // Werte stelle n+1 aus
                    for (int i = 0; i < nResults; i++)
                    {
                        fx_n[1][1][i] = v[i]; // Werte stelle n+1 aus
                        if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
                            fx_n[1][1][i] = 0.0;
                        vResult[1][i] += parser_iVars.vValue[1][3] * (fx_n[1][0][i] + fx_n[1][1][i]) * 0.5; // Berechne das Trapez zu y
                        fx_n[1][0][i] = fx_n[1][1][i];  // Weise Wert an Stelle n+1 an Stelle n zu
                    }
                }
                else if (nMethod == 2)
                {
                    parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]/2.0;
                    v = _parser.Eval(nResults);
                    for (int i = 0; i < nResults; i++)
                        fx_n[1][1][i] = v[i];
                    parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]/2.0;
                    v = _parser.Eval(nResults);
                    for (int i = 0; i < nResults; i++)
                    {
                        fx_n[1][2][i] = v[i];
                        if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
                            fx_n[1][1][i] = 0.0;
                        if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][2][i]))
                            fx_n[1][2][i] = 0.0;
                        vResult[1][i] = parser_iVars.vValue[1][2]/6*(fx_n[1][0][i] + 4.0*fx_n[1][1][i] + fx_n[1][2][i]);
                        fx_n[1][0][i] = fx_n[1][2][i];
                    }
                }
            }
            for (int i = 0; i < nResults; i++)
                fx_n[0][0][i] = vResult[1][i]; // Weise ersten Stelle fuer x zu
        }
        else
        {
            // --> Hier ist nur "x" oder nur "y" enthalten. Wir koennen uns das erste Integral sparen <--
            v = _parser.Eval(nResults);
            for (int i = 0; i < nResults; i++)
                fx_n[0][0][i] = v[i];
        }

        /* --> Das eigentliche, numerische Integral. Es handelt sich um nichts weiter als viele
         *     while()-Schleifendurchlaeufe.
         *     Die aeussere Schleife laeuft so lange x < x_1 ist. <--
         */
        while (parser_iVars.vValue[0][0] + parser_iVars.vValue[0][3] < parser_iVars.vValue[0][2] + parser_iVars.vValue[0][3] * 1e-1)
        {
            if (nMethod == 1)
            {
                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]; // x + dx
                // --> Preufen wir, ob die Grenzen ggf. von "x" abhaengen <--
                if (bRenewBorder)
                {
                    /* --> Ja? Dann muessen wir jedes Mal diese Grenzen neu auswerten (Sollte man in Zukunft
                     *     noch intelligenter loesen) <--
                     */
                    _parser.SetExpr(sInt_Line[1][0]);
                    parser_iVars.vValue[1][1] = _parser.Eval();
                    _parser.SetExpr(sInt_Line[1][1]);
                    parser_iVars.vValue[1][2] = _parser.Eval();
                    _parser.SetExpr(sInt_Fct);
                }

                // --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
                // --> Werten wir sofort die erste y-Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                    fx_n[1][0][i] = v[i];

                // --> Setzen wir die vResult-Variable fuer die innere Schleife auf 0 <--
                for (int i = 0; i < nResults; i++)
                    vResult[1][i] = 0.0;

                // --> Ist eigentlich sowohl "x" als auch "y" in f(x,y) (oder ggf. nur "y"?) vorhanden? <--
                if (bIntVar[1] && (!bIntVar[0] || bIntVar[0]))
                {
                    // --> Ja? Dann muessen wir wohl diese Integration muehsam ausrechnen <--
                    while (parser_iVars.vValue[1][0] + parser_iVars.vValue[1][3] < parser_iVars.vValue[1][2] + parser_iVars.vValue[1][3] * 1e-1) // so lange y < y_1
                    {
                        parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]; // y + dy
                        v = _parser.Eval(nResults); // Werte stelle n+1 aus
                        for (int i = 0; i < nResults; i++)
                        {
                            fx_n[1][1][i] = v[i]; // Werte stelle n+1 aus
                            if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
                                fx_n[1][0][i] = 0.0;
                            vResult[1][i] += parser_iVars.vValue[1][3] * (fx_n[1][0][i] + fx_n[1][1][i]) * 0.5; // Berechne das Trapez zu y
                            fx_n[1][0][i] = fx_n[1][1][i];  // Weise Wert an Stelle n+1 an Stelle n zu
                        }
                    }
                }
                else if (bIntVar[0] && !bIntVar[1])
                {
                    /* --> Nein? Dann koennen wir das gesamte y-Integral durch ein Trapez berechnen. Dazu
                     *     setzen wir die Variable "y" auf den Wert der oberen Grenze und werten das Ergebnis
                     *     fuer die obere Stuetzstelle aus. Anschliessend berechnen wir mit diesen beiden Stuetz-
                     *     stellen und der Breite des (aktuellen) Integrationsintervalls die Flaeche des um-
                     *     schlossenen Trapezes <--
                     */
                    parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
                    v = _parser.Eval(nResults);
                    for (int i = 0; i < nResults; i++)
                    {
                        fx_n[1][1][i] = v[i];
                        vResult[1][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) * (fx_n[1][0][i] + fx_n[1][1][i]) * 0.5;
                        fx_n[1][0][i] = fx_n[1][1][i];
                    }
                }
                // --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[0][1][i] = vResult[1][i];
                    if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[0][1][i]))
                        fx_n[0][1][i] = 0.0;
                    vResult[0][i] += parser_iVars.vValue[0][3] * (fx_n[0][0][i] + fx_n[0][1][i]) * 0.5; // Berechne das Trapez zu x
                    fx_n[0][0][i] = fx_n[0][1][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
                }
            }
            else if (nMethod == 2)
            {
                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]/2.0; // x + dx
                // --> Preufen wir, ob die Grenzen ggf. von "x" abhaengen <--
                if (bRenewBorder)
                {
                    /* --> Ja? Dann muessen wir jedes Mal diese Grenzen neu auswerten (Sollte man in Zukunft
                     *     noch intelligenter loesen) <--
                     */
                    _parser.SetExpr(sInt_Line[1][0]);
                    parser_iVars.vValue[1][1] = _parser.Eval();
                    _parser.SetExpr(sInt_Line[1][1]);
                    parser_iVars.vValue[1][2] = _parser.Eval();
                    _parser.SetExpr(sInt_Fct);
                }

                // --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
                // --> Werten wir sofort die erste y-Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                    fx_n[1][0][i] = v[i];

                // --> Setzen wir die vResult-Variable fuer die innere Schleife auf 0 <--
                for (int i = 0; i < nResults; i++)
                    vResult[1][i] = 0.0;

                // --> Ist eigentlich sowohl "x" als auch "y" in f(x,y) (oder ggf. nur "y"?) vorhanden? <--
                if (bIntVar[1] && (!bIntVar[0] || bIntVar[0]))
                {
                    // --> Ja? Dann muessen wir wohl diese Inegration muehsam ausrechnen <--
                    while (parser_iVars.vValue[1][0] + parser_iVars.vValue[1][3] < parser_iVars.vValue[1][2] + parser_iVars.vValue[1][3] * 1e-1) // so lange y < y_1
                    {
                        parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]/2.0; // y + dy
                        v = _parser.Eval(nResults); // Werte stelle n+1 aus
                        for (int i = 0; i < nResults; i++)
                        {
                            fx_n[1][1][i] = v[i]; // Werte stelle n+1 aus
                            if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
                                fx_n[1][1][i] = 0.0;
                        }
                        parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]/2.0; // y + dy
                        v = _parser.Eval(nResults); // Werte stelle n+1 aus
                        for (int i = 0; i < nResults; i++)
                        {
                            fx_n[1][2][i] = v[i]; // Werte stelle n+1 aus
                            if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][2][i]))
                                fx_n[1][2][i] = 0.0;
                            vResult[1][i] += parser_iVars.vValue[1][3]/6.0 * (fx_n[1][0][i] + 4.0*fx_n[1][1][i]+fx_n[1][2][i]); // Berechne das Trapez zu y
                            fx_n[1][0][i] = fx_n[1][2][i];  // Weise Wert an Stelle n+1 an Stelle n zu
                        }
                    }
                }
                else if (bIntVar[0] && !bIntVar[1])
                {
                    /* --> Nein? Dann koennen wir das gesamte y-Integral durch ein Trapez berechnen. Dazu
                     *     setzen wir die Variable "y" auf den Wert der oberen Grenze und werten das Ergebnis
                     *     fuer die obere Stuetzstelle aus. Anschliessend berechnen wir mit diesen beiden Stuetz-
                     *     stellen und der Breite des (aktuellen) Integrationsintervalls die Flaeche des um-
                     *     schlossenen Trapezes <--
                     */
                    parser_iVars.vValue[1][0] = (parser_iVars.vValue[1][1] + parser_iVars.vValue[1][2])/2.0;
                    v = _parser.Eval(nResults);
                    for (int i = 0; i < nResults; i++)
                        fx_n[1][1][i] = v[i];
                    parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
                    v = _parser.Eval(nResults);
                    for (int i = 0; i < nResults; i++)
                    {
                        fx_n[1][2][i] = v[i];
                        vResult[1][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1])/6.0 * (fx_n[1][0][i] + 4.0*fx_n[1][1][i] + fx_n[1][2][i]);
                    }
                }
                // --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[0][1][i] = vResult[1][i];
                    if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[0][1][i]))
                        fx_n[0][1][i] = 0.0;
                }

                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]/2.0; // x + dx
                // --> Preufen wir, ob die Grenzen ggf. von "x" abhaengen <--
                if (bRenewBorder)
                {
                    /* --> Ja? Dann muessen wir jedes Mal diese Grenzen neu auswerten (Sollte man in Zukunft
                     *     noch intelligenter loesen) <--
                     */
                    _parser.SetExpr(sInt_Line[1][0]);
                    parser_iVars.vValue[1][1] = _parser.Eval();
                    _parser.SetExpr(sInt_Line[1][1]);
                    parser_iVars.vValue[1][2] = _parser.Eval();
                    _parser.SetExpr(sInt_Fct);
                }

                // --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
                // --> Setzen wir die vResult-Variable fuer die innere Schleife auf 0 <--
                for (int i = 0; i < nResults; i++)
                    vResult[2][i] = 0.0;

                // --> Ist eigentlich sowohl "x" als auch "y" in f(x,y) (oder ggf. nur "y"?) vorhanden? <--
                if (bIntVar[1] && (!bIntVar[0] || bIntVar[0]))
                {
                    // --> Ja? Dann muessen wir wohl diese Inegration muehsam ausrechnen <--
                    while (parser_iVars.vValue[1][0] + parser_iVars.vValue[1][3] < parser_iVars.vValue[1][2] + parser_iVars.vValue[1][3] * 1e-1) // so lange y < y_1
                    {
                        parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]/2.0; // y + dy
                        v = _parser.Eval(nResults); // Werte stelle n+1 aus
                        for (int i = 0; i < nResults; i++)
                        {
                            fx_n[1][1][i] = v[i]; // Werte stelle n+1 aus
                            if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
                                fx_n[1][1][i] = 0.0;
                        }
                        parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]/2.0; // y + dy
                        v = _parser.Eval(nResults); // Werte stelle n+1 aus
                        for (int i = 0; i < nResults; i++)
                        {
                            fx_n[1][2][i] = v[i]; // Werte stelle n+1 aus
                            if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][2][i]))
                                fx_n[1][2][i] = 0.0;
                            vResult[2][i] += parser_iVars.vValue[1][3]/6.0 * (fx_n[1][0][i] + 4.0*fx_n[1][1][i] + fx_n[1][2][i]); // Berechne das Trapez zu y
                            fx_n[1][0][i] = fx_n[1][2][i];  // Weise Wert an Stelle n+1 an Stelle n zu
                        }
                    }
                }
                else if (bIntVar[0] && !bIntVar[1])
                {
                    /* --> Nein? Dann koennen wir das gesamte y-Integral durch ein Trapez berechnen. Dazu
                     *     setzen wir die Variable "y" auf den Wert der oberen Grenze und werten das Ergebnis
                     *     fuer die obere Stuetzstelle aus. Anschliessend berechnen wir mit diesen beiden Stuetz-
                     *     stellen und der Breite des (aktuellen) Integrationsintervalls die Flaeche des um-
                     *     schlossenen Trapezes <--
                     */
                    parser_iVars.vValue[1][0] = (parser_iVars.vValue[1][1] + parser_iVars.vValue[1][2])/2.0;
                    v = _parser.Eval(nResults);
                    for (int i = 0; i < nResults; i++)
                        fx_n[1][1][i] = v[i];
                    parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
                    v = _parser.Eval(nResults);
                    for (int i = 0; i < nResults; i++)
                    {
                        fx_n[1][2][i] = v[i];
                        vResult[2][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1])/6.0 * (fx_n[1][0][i] + 4.0*fx_n[1][1][i] + fx_n[1][2][i]);
                    }
                }
                // --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[0][2][i] = vResult[2][i];
                    if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[0][2][i]))
                        fx_n[0][2][i] = 0.0;
                    vResult[0][i] += parser_iVars.vValue[0][3]/6.0 * (fx_n[0][0][i] + 4.0*fx_n[0][1][i] + fx_n[0][2][i]); // Berechne das Trapez zu x
                    fx_n[0][0][i] = fx_n[0][2][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
                }
            }
            if (_option.getSystemPrintStatus())
            {
                if (!bLargeArray)
                {
                    if ((int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) * 20) > (int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][3]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) * 20))
                    {
                        NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 20) * 5) + " %");
                    }
                }
                else
                {
                    if ((int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) * 100) > (int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][3]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2]-parser_iVars.vValue[0][1]) * 100))
                    {
                        NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((parser_iVars.vValue[0][0]-parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 100)) + " %");
                    }
                }
                if (NumeReKernel::GetAsyncCancelState())//GetAsyncKeyState(VK_ESCAPE))
                {
                    NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL") + "!\n");
                    throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
                }
            }
        }

        // --> Ergebnis sinnvoll runden! <--
        /*if (bDoRoundResults)
        {
            for (unsigned int i = 0; i < vResult[0].size(); i++)
            {
                double dExponent = -1.0*floor(log10(abs(vResult[0][i])));
                if (isnan(dExponent) || isinf(dExponent))
                    continue;
                vResult[0][i] = vResult[0][i] * pow(10.0, dExponent) / (parser_iVars.vValue[0][3] * parser_iVars.vValue[0][3]);
                vResult[0][i] = std::round(vResult[0][i]);
                vResult[0][i] = vResult[0][i] * (parser_iVars.vValue[0][3] * parser_iVars.vValue[0][3]) / pow(10.0, dExponent);
            }
        }*/
        if (_option.getSystemPrintStatus())
            NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 100 %");
    }
    else if (!bRenewBorder)
    {
        // --> Okay: hier ist weder "x" noch "y" in f(x,y) enthalten, noch haengen die Grenzen von "x" ab <--
        string sTemp = sInt_Fct;

        string sInt_Fct_2 = "";
        while (sTemp.length())
            sInt_Fct_2 += getNextArgument(sTemp, true) + "*" + parser_iVars.sName[0] + "*" + parser_iVars.sName[1] + ",";

        sInt_Fct_2.erase(sInt_Fct_2.length()-1,1);
        //string_type sInt_Fct_2 = sInt_Fct + "*" + parser_iVars.sName[0] + "*" + parser_iVars.sName[1];
        if (_option.getSystemPrintStatus())
        {
            NumeReKernel::printPreFmt("|INTEGRATE>" + LineBreak(" " + _lang.get("PARSERFUNCS_INTEGRATE_ANALYTICAL") + ": F(" + parser_iVars.sName[0] + "," + parser_iVars.sName[1] + ") = " + sInt_Fct_2,_option, true, 12, 12)+"\n");
            NumeReKernel::printPreFmt("|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... ");
        }
        // --> Schnelle Loesung: Konstante x Flaeche, die vom Integral umschlossen wird <--
        parser_iVars.vValue[0][0] = parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1];
        parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1];
        _parser.SetExpr(sInt_Fct_2);
        v = _parser.Eval(nResults);
        for (int i = 0; i < nResults; i++)
            vResult[0][i] = v[i];
    }
    else
    {
        /* --> Doofer Fall: zwar eine Funktion, die weder von "x" noch von "y" abhaengt,
         *     dafuer aber erfordert, dass die Grenzen des Integrals jedes Mal aktualisiert
         *     werden. <--
         */
        if (_option.getSystemPrintStatus())
            NumeReKernel::printPreFmt("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_CONSTANT") + " ... ");
        // --> Waehle willkuerliche Praezision von 1e-4 <--
        parser_iVars.vValue[0][3] = 1e-4;
        parser_iVars.vValue[1][3] = 1e-4;
        // --> Setze "x" und "y" auf ihre unteren Grenzen <--
        parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1];
        parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
        // --> Werte erste x-Stuetzstelle aus <--
        v = _parser.Eval(nResults);
        for (int i = 0; i < nResults; i++)
            fx_n[0][0][i] = v[i];

        /* --> Berechne das eigentliche Integral. Unterscheidet sich nur begrenzt von dem oberen,
         *     ausfuehrlichen Fall, ausser dass die innere Schleife aufgrund des Fehlens der Inte-
         *     grationsvariablen "y" vollstaendig wegfaellt <--
         */
        while (parser_iVars.vValue[0][0] + 1e-4 < parser_iVars.vValue[0][2] + 1e-5)
        {
            if (nMethod == 1)
            {
                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]; // x + dx

                // --> Erneuere die Werte der x- und y-Grenzen <--
                _parser.SetExpr(sInt_Line[1][0]);
                parser_iVars.vValue[1][1] = _parser.Eval();
                _parser.SetExpr(sInt_Line[1][1]);
                parser_iVars.vValue[1][2] = _parser.Eval();
                // --> Weise dem Parser wieder die Funktion f(x,y) zu <--
                _parser.SetExpr(sInt_Fct);
                // --> Setze "y" wieder auf die untere Grenze <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
                // --> Setze den Speicher fuer die "innere" Integration auf 0 <--
                for (int i = 0; i < nResults; i++)
                    vResult[1][i] = 0.0;

                // --> Werte erste y-Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                    fx_n[1][0][i] = v[i];
                // --> Setze "y" auf die obere Grenze <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
                // --> Werte die zweite Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[1][1][i] = v[i];
                    // --> Berechne das y-Trapez <--
                    vResult[1][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) * (fx_n[1][0][i] + fx_n[1][1][i]) * 0.5;

                    // --> Weise das y-Ergebnis der zweiten x-Stuetzstelle zu <--
                    fx_n[0][1][i] = vResult[1][i];
                    vResult[0][i] += parser_iVars.vValue[0][3] * (fx_n[0][0][i] + fx_n[0][1][i]) * 0.5; // Berechne das Trapez zu x
                    fx_n[0][0][i] = fx_n[0][1][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
                }
            }
            else if (nMethod == 2)
            {
                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]/2.0; // x + dx

                // --> Erneuere die Werte der x- und y-Grenzen <--
                _parser.SetExpr(sInt_Line[1][0]);
                parser_iVars.vValue[1][1] = _parser.Eval();
                _parser.SetExpr(sInt_Line[1][1]);
                parser_iVars.vValue[1][2] = _parser.Eval();
                // --> Weise dem Parser wieder die Funktion f(x,y) zu <--
                _parser.SetExpr(sInt_Fct);
                // --> Setze "y" wieder auf die untere Grenze <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
                // --> Setze den Speicher fuer die "innere" Integration auf 0 <--
                for (int i = 0; i < nResults; i++)
                    vResult[1][i] = 0.0;

                // --> Werte erste y-Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                    fx_n[1][0][i] = v[i];
                // --> Setze "y" auf die obere Grenze <--
                parser_iVars.vValue[1][0] = (parser_iVars.vValue[1][1] + parser_iVars.vValue[1][2])/2.0;
                // --> Werte die zweite Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                    fx_n[1][1][i] = v[i];
                // --> Setze "y" auf die obere Grenze <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
                // --> Werte die zweite Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[1][2][i] = v[i];
                    // --> Berechne das y-Trapez <--
                    vResult[1][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1])/6.0 * (fx_n[1][0][i] + 4.0*fx_n[1][1][i] + fx_n[1][2][i]);

                    // --> Weise das y-Ergebnis der zweiten x-Stuetzstelle zu <--
                    fx_n[0][1][i] = vResult[1][i];
                }

                parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]/2.0; // x + dx

                // --> Erneuere die Werte der x- und y-Grenzen <--
                _parser.SetExpr(sInt_Line[1][0]);
                parser_iVars.vValue[1][1] = _parser.Eval();
                _parser.SetExpr(sInt_Line[1][1]);
                parser_iVars.vValue[1][2] = _parser.Eval();
                // --> Weise dem Parser wieder die Funktion f(x,y) zu <--
                _parser.SetExpr(sInt_Fct);
                // --> Setze "y" wieder auf die untere Grenze <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
                // --> Setze den Speicher fuer die "innere" Integration auf 0 <--
                for (int i = 0; i < nResults; i++)
                    vResult[2][i] = 0.0;

                // --> Werte erste y-Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                    fx_n[1][0][i] = v[i];
                // --> Setze "y" auf die obere Grenze <--
                parser_iVars.vValue[1][0] = (parser_iVars.vValue[1][1] + parser_iVars.vValue[1][2])/2.0;
                // --> Werte die zweite Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                    fx_n[1][1][i] = v[i];
                // --> Setze "y" auf die obere Grenze <--
                parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
                // --> Werte die zweite Stuetzstelle aus <--
                v = _parser.Eval(nResults);
                for (int i = 0; i < nResults; i++)
                {
                    fx_n[1][2][i] = v[i];
                    // --> Berechne das y-Trapez <--
                    vResult[2][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1])/6.0 * (fx_n[1][0][i] + 4.0*fx_n[1][1][i] + fx_n[1][2][i]);

                    // --> Weise das y-Ergebnis der zweiten x-Stuetzstelle zu <--
                    fx_n[0][2][i] = vResult[2][i];
                    vResult[0][i] += parser_iVars.vValue[0][3]/6.0 * (fx_n[0][0][i] + 4.0*fx_n[0][1][i]+fx_n[0][2][i]); // Berechne das Trapez zu x
                    fx_n[0][0][i] = fx_n[0][2][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
                }
            }
        }

        // --> Ergebnis sinnvoll runden! <--
        /*if (bDoRoundResults)
        {
            for (unsigned int i = 0; i < vResult[0].size(); i++)
            {
                double dExponent = -1.0*floor(log10(abs(vResult[0][i])));
                if (isinf(dExponent) || isnan(dExponent))
                    continue;
                vResult[0][i] = vResult[0][i] * pow(10.0, dExponent) / (parser_iVars.vValue[0][3] * parser_iVars.vValue[0][3]);
                vResult[0][i] = std::round(vResult[0][i]);
                vResult[0][i] = vResult[0][i] * (parser_iVars.vValue[0][3] * parser_iVars.vValue[0][3]) / pow(10.0, dExponent);
            }
        }*/
    }

    // --> Falls die Grenzen irgendwo getauscht worden sind, wird dem hier Rechnung getragen <--
    for (int i = 0; i < nResults; i++)
        vResult[0][i] *= nSign;

    // --> FERTIG! Teilen wir dies dem Benutzer mit <--
    if (_option.getSystemPrintStatus())
    {
        //cerr << std::setprecision(_option.getPrecision());
        NumeReKernel::printPreFmt(": " + _lang.get("COMMON_SUCCESS") + "!\n");

        // --> Noch eine abschliessende Ausgabe des Ergebnisses <--
        /*if (bIntVar[0] || bIntVar[1])
            cerr << LineBreak(" Integral \"" + sLabel + "\" von [" + parser_iVars.sName[0]+","+parser_iVars.sName[1]+"]=["+sInt_Line[0][0]+","+sInt_Line[1][0]+"] bis ["+sInt_Line[0][1]+","+sInt_Line[1][1]+"]: Erfolg!", _option, true, 12, 12) << endl;
        else if (!bRenewBorder)
            cerr << LineBreak(" F(" + parser_iVars.sName[0] + "," + parser_iVars.sName[1] + ") = " + sInt_Fct + " von [" + parser_iVars.sName[0]+","+parser_iVars.sName[1]+"]=["+sInt_Line[0][0]+","+sInt_Line[1][0]+"] bis ["+sInt_Line[0][1]+","+sInt_Line[1][1]+"]: Erfolg!", _option, true, 12, 12) << endl;
        else
            cerr << LineBreak(" Integral \"" + sLabel + "\" von [" + parser_iVars.sName[0]+","+parser_iVars.sName[1]+"]=["+sInt_Line[0][0]+","+sInt_Line[1][0]+"] bis ["+sInt_Line[0][1]+","+sInt_Line[1][1]+"]: Erfolg!", _option, true, 12, 12) << endl;*/
    }

    /*if (nResults > 1 && !bSupressAnswer)
    {
        //cerr << std::setprecision(_option.getPrecision());
        int nLineBreak = parser_LineBreak(_option);
        cerr << "|-> ans = [";
        for (int i = 0; i < nResults; ++i)
        {
            cerr << std::setfill(' ') << std::setw(_option.getPrecision()+7) << std::setprecision(_option.getPrecision()) << vResult[0][i];
            if (i < nResults-1)
                cerr << ", ";
            if (nResults + 1 > nLineBreak && !((i+1) % nLineBreak) && i < nResults-1)
                cerr << "...\n|          ";
        }
        cerr << "]" << endl;
    }*/

    // --> Weisen wir "ans" noch das Ergebnis der Integration zu <--
    //vAns = vResult[0][0];
    // --> Fertig! Zurueck zur aufrufenden Funkton! <--
    return vResult[0];
}

// --> Numerische Differenzierung <--
vector<double> parser_Diff(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option, Define& _functions)
{
    string sExpr = sCmd.substr(findCommand(sCmd).sString.length()+findCommand(sCmd).nPos);
    string sEps = "";
    string sVar = "";
    //string sInterval = "";
    string sPos = "";
    double dEps = 0.0;
    double dPos = 0.0;
    double* dVar = 0;
    value_type* v = 0;
    int nResults = 0;
    int nSamples = 100;
    vector<double> vInterval;
    vector<double> vResult;

    if (containsStrings(sExpr) || _data.containsStringVars(sExpr))
    {
        //sErrorToken = "diff";
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "diff");
    }

    if (sExpr.find("-set") != string::npos)
        sExpr.erase(sExpr.find("-set"));
        //sExpr = sCmd.substr(findCommand(sCmd).sString.length(), sCmd.find("-set")-findCommand(sCmd).sString.length());
    else if (sExpr.find("--") != string::npos)
        sExpr.erase(sExpr.find("--"));
        //sExpr = sCmd.substr(findCommand(sCmd).sString.length(), sCmd.find("--")-findCommand(sCmd).sString.length());

    if (!_functions.call(sExpr, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sExpr, sExpr);
    StripSpaces(sExpr);

    if ((sExpr.find("data(") == string::npos && !_data.containsCacheElements(sExpr))
        && (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos))
    {
        /*if (!_functions.call(sExpr, _option))
        {
            throw FUNCTION_ERROR;
        }
        StripSpaces(sExpr);*/

        if (sCmd.find("-set") != string::npos)
            sVar = sCmd.substr(sCmd.find("-set"));
        else
            sVar = sCmd.substr(sCmd.find("--"));
        if (!_functions.call(sVar, _option))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sVar, sVar);
        StripSpaces(sVar);

        if (matchParams(sVar, "eps", '='))
        {

            sEps = getArgAtPos(sVar, matchParams(sVar, "eps", '=')+3);
            sVar += " ";
            sVar = sVar.substr(0,matchParams(sVar, "eps", '=')) + sVar.substr(sVar.find(' ', matchParams(sVar, "eps", '=')+3));

            if (parser_ExprNotEmpty(sEps))
            {
                _parser.SetExpr(sEps);
                dEps = _parser.Eval();
            }
            if (isinf(dEps) || isnan(dEps))
                dEps = 0.0;
            if (dEps < 0)
                dEps *= -1;
        }

        if (matchParams(sVar, "samples", '='))
        {

            _parser.SetExpr(getArgAtPos(sVar, matchParams(sVar, "samples", '=')+7));
            nSamples = (int)_parser.Eval();
            sVar += " ";
            sVar = sVar.substr(0,matchParams(sVar, "samples", '=')) + sVar.substr(sVar.find(' ', matchParams(sVar, "samples", '=')+7));
            if (nSamples <= 0)
                nSamples = 100;
        }

        if (sVar.find('=') != string::npos ||
            (sVar.find('[') != string::npos
                && sVar.find(']', sVar.find('[')) != string::npos
                && sVar.find(':', sVar.find('[')) != string::npos))
        {

            if (sVar.substr(0,2) == "--")
                sVar = sVar.substr(2);
            else if (sVar.substr(0,4) == "-set")
                sVar = sVar.substr(4);
            if (sVar.find('[') != string::npos
                && sVar.find(']', sVar.find('[')) != string::npos
                && sVar.find(':', sVar.find('[')) != string::npos)
            {
                sPos = sVar.substr(sVar.find('[')+1, getMatchingParenthesis(sVar.substr(sVar.find('[')))-1);
                sVar = "x";
                StripSpaces(sPos);
                if (sPos == ":")
                    sPos = "-10:10";
            }
            else
            {
                int nPos = sVar.find('=');
                sPos = sVar.substr(nPos+1, sVar.find(' ', nPos)-nPos-1);
                sVar = " " + sVar.substr(0,nPos);
                sVar = sVar.substr(sVar.rfind(' '));
                StripSpaces(sVar);
            }
            if (parser_ExprNotEmpty(sPos))
            {
                if (_data.containsCacheElements(sPos) || sPos.find("data(") != string::npos)
                {
                    parser_GetDataElement(sPos, _parser, _data, _option);
                    /*if (sPos.find("{{") != string::npos && (containsStrings(sPos) || _data.containsStringVars(sPos)))
                        parser_VectorToExpr(sPos, _option);*/
                }
                if (sPos.find(':') != string::npos)
                    sPos.replace(sPos.find(':'),1,",");
                //cerr << sPos << endl;
                _parser.SetExpr(sPos);
                v = _parser.Eval(nResults);
                if (isinf(v[0]) || isnan(v[0]))
                {
                    vResult.push_back(NAN);
                    return vResult;
                }
                for (int i = 0; i < nResults; i++)
                {
                    vInterval.push_back(v[i]);
                }
            }
            //cerr << sExpr << endl;
            _parser.SetExpr(sExpr);
            _parser.Eval(nResults);

            dVar = parser_GetVarAdress(sVar, _parser);
            if (!dVar)
            {
                throw SyntaxError(SyntaxError::DIFF_VAR_NOT_FOUND, sCmd, sVar, sVar);
            }

        }

        if (!dVar)
        {
            throw SyntaxError(SyntaxError::NO_DIFF_VAR, sCmd, SyntaxError::invalid_position);
        }

        if (!dEps)
            dEps = 1e-7;
        string sCompl_Expr = sExpr;
        if (vInterval.size() == 1 || vInterval.size() > 2)
        {
            if (sCompl_Expr.find("{") != string::npos)
                parser_VectorToExpr(sCompl_Expr, _option);
            while (sCompl_Expr.length())
            {
                sExpr = getNextArgument(sCompl_Expr, true);
                _parser.SetExpr(sExpr);
                for (unsigned int i = 0; i < vInterval.size(); i++)
                {
                    dPos = vInterval[i];
                    vResult.push_back(_parser.Diff(dVar, dPos, dEps));
                }
            }
        }
        else
        {
            if (sCompl_Expr.find("{") != string::npos)
                parser_VectorToExpr(sCompl_Expr, _option);
            while (sCompl_Expr.length())
            {
                sExpr = getNextArgument(sCompl_Expr, true);
                _parser.SetExpr(sExpr);
                for (int i = 0; i < nSamples; i++)
                {
                    dPos = vInterval[0] + (vInterval[1]-vInterval[0])/(double)(nSamples-1)*(double)i;
                    vResult.push_back(_parser.Diff(dVar, dPos, dEps));
                }
            }
        }
    }
    else if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
    {
        /*sExpr = sCmd.substr(findCommand(sCmd).nPos+findCommand(sCmd).sString.length());
        StripSpaces(sExpr);
        if (!_functions.call(sExpr, _option))
            throw FUNCTION_ERROR;*/
        Indices _idx = parser_getIndices(sExpr, _parser, _data, _option);
        sExpr.erase(sExpr.find('('));
        //cerr << sExpr << endl;
        if (((_idx.nI[0] == -1 || _idx.nI[1] == -1) && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

        if (!_idx.vI.size())
        {
            if (_idx.nI[1] == -2)
                _idx.nI[1] = _data.getLines(sExpr, false);
            if (_idx.nJ[1] == -2)
                _idx.nJ[1] = _idx.nJ[0]+1;

            if (_idx.nJ[1] == -1)
            {
                for (long long int i = _idx.nI[0]; i < _idx.nI[1]-1; i++)
                {
                    if (_data.isValidEntry(i,_idx.nJ[0], sExpr)
                        && _data.isValidEntry(i+1,_idx.nJ[0], sExpr))
                        vResult.push_back(_data.getElement(i+1, _idx.nJ[0], sExpr)-_data.getElement(i, _idx.nJ[0], sExpr));
                    else
                        vResult.push_back(NAN);
                }
            }
            else
            {
                Datafile _cache;
                for (long long int i = _idx.nI[0]; i < _idx.nI[1]; i++)
                {
                    _cache.writeToCache(i-_idx.nI[0], 0, "cache", _data.getElement(i,_idx.nJ[0], sExpr));
                    _cache.writeToCache(i-_idx.nI[0], 1, "cache", _data.getElement(i,_idx.nJ[1], sExpr));
                }
                //cerr << _cache.getLines("cache", false) << "  " << _cache.getCols("cache") << endl;
                _cache.sortElements("cache -sort c=1[2]");
                if (matchParams(sCmd, "xvals"))
                {
                    for (long long int i = 0; i < _cache.getLines("cache", false)-1; i++)
                    {
                        if (_cache.isValidEntry(i, 0, "cache")
                            && _cache.isValidEntry(i+1, 0, "cache")
                            && _cache.isValidEntry(i, 1, "cache")
                            && _cache.isValidEntry(i+1, 1, "cache"))
                            vResult.push_back((_cache.getElement(i+1, 0, "cache")+_cache.getElement(i, 0, "cache"))/2);
                        else
                            vResult.push_back(NAN);
                    }
                }
                else
                {
                    for (long long int i = 0; i < _cache.getLines("cache", false)-1; i++)
                    {
                        if (_cache.isValidEntry(i, 0, "cache")
                            && _cache.isValidEntry(i+1, 0, "cache")
                            && _cache.isValidEntry(i, 1, "cache")
                            && _cache.isValidEntry(i+1, 1, "cache"))
                            vResult.push_back((_cache.getElement(i+1, 1, "cache")-_cache.getElement(i, 1, "cache"))
                                / (_cache.getElement(i+1, 0, "cache")-_cache.getElement(i, 0, "cache")));
                        else
                            vResult.push_back(NAN);
                    }
                }
            }
        }
        else
        {
            if (_idx.vJ.size() == 1)
            {
                for (long long int i = 0; i < _idx.vI.size()-1; i++)
                {
                    if (_data.isValidEntry(_idx.vI[i],_idx.vJ[0], sExpr)
                        && _data.isValidEntry(_idx.vI[i+1],_idx.vJ[0], sExpr))
                        vResult.push_back(_data.getElement(_idx.vI[i+1], _idx.vJ[0], sExpr)-_data.getElement(_idx.vI[i], _idx.vJ[0], sExpr));
                    else
                        vResult.push_back(NAN);
                }
            }
            else
            {
                Datafile _cache;
                for (long long int i = 0; i < _idx.vI.size(); i++)
                {
                    _cache.writeToCache(i, 0, "cache", _data.getElement(_idx.vI[i],_idx.vJ[0], sExpr));
                    _cache.writeToCache(i, 1, "cache", _data.getElement(_idx.vI[i],_idx.vJ[1], sExpr));
                }
                //cerr << _cache.getLines("cache", false) << "  " << _cache.getCols("cache") << endl;
                _cache.sortElements("cache -sort c=1[2]");
                if (matchParams(sCmd, "xvals"))
                {
                    for (long long int i = 0; i < _cache.getLines("cache", false)-1; i++)
                    {
                        if (_cache.isValidEntry(i, 0, "cache")
                            && _cache.isValidEntry(i+1, 0, "cache")
                            && _cache.isValidEntry(i, 1, "cache")
                            && _cache.isValidEntry(i+1, 1, "cache"))
                            vResult.push_back((_cache.getElement(i+1, 0, "cache")+_cache.getElement(i, 0, "cache"))/2);
                        else
                            vResult.push_back(NAN);
                    }
                }
                else
                {
                    for (long long int i = 0; i < _cache.getLines("cache", false)-1; i++)
                    {
                        if (_cache.isValidEntry(i, 0, "cache")
                            && _cache.isValidEntry(i+1, 0, "cache")
                            && _cache.isValidEntry(i, 1, "cache")
                            && _cache.isValidEntry(i+1, 1, "cache"))
                            vResult.push_back((_cache.getElement(i+1, 1, "cache")-_cache.getElement(i, 1, "cache"))
                                / (_cache.getElement(i+1, 0, "cache")-_cache.getElement(i, 0, "cache")));
                        else
                            vResult.push_back(NAN);
                    }
                }
            }
        }

    }
    else
    {
        throw SyntaxError(SyntaxError::NO_DIFF_OPTIONS, sCmd, SyntaxError::invalid_position);
    }
    return vResult;
}

// --> Listet alle vorhandenen mathematischen Funktionen <--
void parser_ListFunc(const Settings& _option, const string& sType) //PRSRFUNC_LISTFUNC_[TYPES]_*
{
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::printPreFmt("|-> NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTFUNC_HEADLINE")));
    if (sType != "all")
    {
        NumeReKernel::printPreFmt("  [" + toUpperCase(_lang.get("PARSERFUNCS_LISTFUNC_TYPE_"+toUpperCase(sType))) + "]");
    }
    NumeReKernel::printPreFmt("\n");
    make_hline();
    NumeReKernel::printPreFmt(LineBreak("|   "+_lang.get("PARSERFUNCS_LISTFUNC_TABLEHEAD"), _option, false, 0, 28)+"\n|\n");
    vector<string> vFuncs;
    if (sType == "all")
        vFuncs = _lang.getList("PARSERFUNCS_LISTFUNC_FUNC_*");
    else
        vFuncs = _lang.getList("PARSERFUNCS_LISTFUNC_FUNC_*_["+toUpperCase(sType)+"]");

    for (unsigned int i = 0; i < vFuncs.size(); i++)
    {
        NumeReKernel::printPreFmt(LineBreak("|   " + vFuncs[i], _option, false, 0, 41)+"\n");
    }
    NumeReKernel::printPreFmt("|\n");
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTFUNC_FOOTNOTE1"), _option));
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTFUNC_FOOTNOTE2"), _option));
    NumeReKernel::toggleTableStatus();
    make_hline();
    return;
}

// --> Listet alle selbst definierten Funktionen <--
void parser_ListDefine(const Define& _functions, const Settings& _option)
{
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTDEFINE_HEADLINE")));
    make_hline();
    if (!_functions.getDefinedFunctions())
    {
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTDEFINE_EMPTY")));
    }
    else
    {
        for (unsigned int i = 0; i < _functions.getDefinedFunctions(); i++)
        {
            NumeReKernel::printPreFmt(sectionHeadline(_functions.getFunction(i).substr(0,_functions.getFunction(i).rfind('('))));
            ///cerr << "|   "  << std::setfill((char)196) << std::setw(_option.getWindow()-4) << std::left << toUpperCase(_functions.getFunction(i).substr(0,_functions.getFunction(i).rfind('(')))+": " << endl;
            if (_functions.getComment(i).length())
            {
                NumeReKernel::printPreFmt(LineBreak("|       "+_lang.get("PARSERFUNCS_LISTDEFINE_DESCRIPTION",_functions.getComment(i)), _option, true, 0, 25)+"\n");//10
            }
            NumeReKernel::printPreFmt(LineBreak("|       "+_lang.get("PARSERFUNCS_LISTDEFINE_DEFINITION", _functions.getFunction(i), _functions.getImplemention(i)), _option, false, 0, 29)+"\n");//14
            /*if (i < _functions.getDefinedFunctions()-1)
                cerr << "|" << endl;*/
        }
        NumeReKernel::printPreFmt("|   -- " + toString((int)_functions.getDefinedFunctions()) + " " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTDEFINE_FUNCTIONS"))  + " --\n");
    }
    NumeReKernel::toggleTableStatus();
    make_hline();
    return;
}

// --> Listet alle Logik-Ausdruecke <--
void parser_ListLogical(const Settings& _option)
{
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTLOGICAL_HEADLINE"))));
    make_hline();
    NumeReKernel::printPreFmt(toSystemCodePage("|   "+_lang.get("PARSERFUNCS_LISTLOGICAL_TABLEHEAD"))+"\n|\n");

    vector<string> vLogicals = _lang.getList("PARSERFUNCS_LISTLOGICAL_ITEM*");
    for (unsigned int i = 0; i < vLogicals.size(); i++)
        NumeReKernel::printPreFmt(toSystemCodePage("|   " + vLogicals[i])+"\n");
    NumeReKernel::printPreFmt(toSystemCodePage("|\n"));
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTLOGICAL_FOOTNOTE1"), _option));
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTLOGICAL_FOOTNOTE2"), _option));
    NumeReKernel::toggleTableStatus();
    make_hline();
    return;
}

// --> Listet alle zuvor deklarierten Variablen und ihre Werte <--
void parser_ListVar(mu::ParserBase& _parser, const Settings& _option, const Datafile& _data)
{
    // Query the used variables (must be done after calc)
    int nDataSetNum = 1;
    mu::varmap_type variables = _parser.GetVar();
    map<string,string> StringMap = _data.getStringVars();
    map<string,int> VarMap;
    map<string,long long int> CacheMap = _data.getCacheList();

    for (auto iter = variables.begin(); iter != variables.end(); ++iter)
    {
        VarMap[iter->first] = 0;
    }
    for (auto iter = StringMap.begin(); iter != StringMap.end(); ++iter)
    {
        VarMap[iter->first] = 1;
    }

    //string_type sExprTemp = _parser.GetExpr();
    int nBytesSum = 0;
    string sDataSize = toString(_data.getLines("data",false)) + " x " + toString(_data.getCols("data"));
    string sStringSize = toString((int)_data.getStringElements()) + " x " + toString((int)_data.getStringCols());
    if (!VarMap.size())
    {
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_EMPTY")));
        return;
    }
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print("NUMERE: " + toUpperCase(toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_HEADLINE"))));
    make_hline();

    for (auto iter = CacheMap.begin(); iter != CacheMap.end(); ++iter)
    {
        string sCacheSize = toString(_data.getCacheLines(iter->first, false)) + " x " + toString(_data.getCacheCols(iter->first, false));
        NumeReKernel::printPreFmt("|   " + iter->first+"()" + strfill("Dim:", (_option.getWindow(0)-32)/2-(iter->first).length() + _option.getWindow(0)%2) + strfill(sCacheSize, (_option.getWindow(0)-50)/2)+strfill("[double x double]", 19));
        ///cerr << "|   " << iter->first << "()" << std::setfill(' ') << std::setw((_option.getWindow(0)-32)/2-(iter->first).length() + _option.getWindow(0)%2) << "Dim:" //24
        ///     <<  std::setfill(' ') << std::setw((_option.getWindow(0)-50)/2) << sCacheSize << std::setw(19) << "[double x double]"; //15
        if (_data.getSize(iter->second) >= 1024*1024)
            NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second)/(1024.0*1024.0), 4), 9)+ " MBytes\n");
            ///cerr << std::setprecision(4) << std::setw(9) << _data.getSize(iter->second)/(1024.0*1024.0) << " MBytes";
        else if (_data.getSize(iter->second) >= 1024)
            NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second)/(1024.0), 4), 9)+ " KBytes\n");
            ///cerr << std::setprecision(4) << std::setw(9) << _data.getSize(iter->second)/1024.0 << " KBytes";
        else
            NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second)), 9)+ "  Bytes\n");
            ///cerr << std::setw(9) << _data.getSize(iter->second) << "  Bytes";
        //cerr << endl;
        nBytesSum += _data.getSize(iter->second);
    }
    NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0)-4, '-')+"\n");
    ///cerr << "|   " << std::setfill((char)196) << std::setw(_option.getWindow(0)-4) << (char)196 << endl;

    if (_data.isValid())
    {
        NumeReKernel::printPreFmt("|   data()" + strfill("Dim:", (_option.getWindow(0)-32)/2-4 + _option.getWindow(0)%2) + strfill(sDataSize, (_option.getWindow(0)-50)/2)+strfill("[double x double]", 19));
        ///cerr << "|   data()" << std::setfill(' ') << std::setw((_option.getWindow(0)-32)/2-4 + _option.getWindow(0)%2) << "Dim:" << std::setfill(' ') << std::setw((_option.getWindow(0)-50)/2) << sDataSize << std::setw(19) << "[double x double]";
        if (_data.getDataSize() >= 1024*1024)
            NumeReKernel::printPreFmt(strfill(toString(_data.getDataSize()/(1024.0*1024.0), 4), 9)+ " MBytes\n");
            ///cerr << std::setprecision(4) << std::setw(9) << _data.getDataSize()/(1024.0*1024.0) << " MBytes";
        else if (_data.getDataSize() >= 1024)
            NumeReKernel::printPreFmt(strfill(toString(_data.getDataSize()/(1024.0), 4), 9)+ " KBytes\n");
            ///cerr << std::setprecision(4) << std::setw(9) << _data.getDataSize()/1024.0 << " KBytes";
        else
            NumeReKernel::printPreFmt(strfill(toString(_data.getDataSize()), 9)+ "  Bytes\n");
            ///cerr << std::setw(9) << _data.getDataSize() << "  Bytes";
        //cerr << endl;
        nBytesSum += _data.getDataSize();

        NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0)-4, '-')+"\n");
        ///cerr << "|   " << std::setfill((char)196) << std::setw(_option.getWindow(0)-4) << (char)196 << endl;
    }
    if (_data.getStringElements())
    {
        NumeReKernel::printPreFmt("|   string()" + strfill("Dim:", (_option.getWindow(0)-32)/2-6 + _option.getWindow(0)%2) + strfill(sStringSize, (_option.getWindow(0)-50)/2)+strfill("[string x string]", 19));
        ///cerr << "|   string()" << std::setfill(' ') << std::setw((_option.getWindow(0)-32)/2-6 + _option.getWindow(0)%2) << "Dim:" << std::setfill(' ') << std::setw((_option.getWindow(0)-50)/2) << sStringSize << std::setw(19) << "[string x string]";
        if (_data.getStringSize() >= 1024*1024)
            NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize()/(1024.0*1024.0), 4), 9)+ " MBytes\n");
            ///cerr << std::setprecision(4) << std::setw(9) << _data.getStringSize()/(1024.0*1024.0) << " MBytes";
        else if (_data.getStringSize() >= 1024)
            NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize()/(1024.0), 4), 9)+ " KBytes\n");
            ///cerr << std::setprecision(4) << std::setw(9) << _data.getStringSize()/1024.0 << " KBytes";
        else
            NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize()), 9)+ "  Bytes\n");
            ///cerr << std::setw(9) << _data.getStringSize() << "  Bytes";
        //cerr << endl;
        nBytesSum += _data.getStringSize();

        NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0)-4, '-')+"\n");
        ///cerr << "|   " << std::setfill((char)196) << std::setw(_option.getWindow(0)-4) << (char)196 << endl;
    }

    for (auto item = VarMap.begin(); item != VarMap.end(); ++item)
    {
        if (item->second)
        {
            NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow(0)-20)/2+1-_option.getPrecision()-(item->first).length() + _option.getWindow(0)%2));
            ///cerr << "|   " << item->first;
            ///cerr << std::setfill(' ') << std::setw((_option.getWindow(0)-20)/2+1-_option.getPrecision()-(item->first).length() + _option.getWindow(0)%2) << " = ";
            if (StringMap[item->first].length() > (unsigned int)_option.getPrecision()+(_option.getWindow(0)-60)/2-4)
                NumeReKernel::printPreFmt(strfill("\""+StringMap[item->first].substr(0,_option.getPrecision()+(_option.getWindow(0)-60)/2-7)+"...\"", (_option.getWindow(0)-60)/2+_option.getPrecision()));
                ///cerr << std::setw((_option.getWindow(0)-60)/2+_option.getPrecision()) << "\""+StringMap[item->first].substr(0,_option.getPrecision()+(_option.getWindow(0)-60)/2-7)+"...\"";
            else
                NumeReKernel::printPreFmt(strfill("\""+StringMap[item->first]+"\"", (_option.getWindow(0)-60)/2+_option.getPrecision()));
                //cerr << std::setw((_option.getWindow(0)-60)/2+_option.getPrecision()) << "\""+StringMap[item->first]+"\"";
            NumeReKernel::printPreFmt(strfill("[string]", 19) + strfill(toString((int)StringMap[item->first].size()), 9)+"  Bytes\n");
            ///cerr << std::setw(19) << "[string]";
            ///cerr << std::setw(9) << StringMap[item->first].size() << "  Bytes" << endl;
            nBytesSum += StringMap[item->first].size();
        }
        else
        {
            //_parser.SetExpr(item->first);
            NumeReKernel::printPreFmt("|   "+item->first + strfill(" = ", (_option.getWindow(0)-20)/2+1-_option.getPrecision()-(item->first).length() + _option.getWindow(0)%2) + strfill(toString(*variables[item->first], _option), (_option.getWindow(0)-60)/2+ _option.getPrecision()) + strfill("[double]", 19)+strfill("8", 9)+"  Bytes\n");
            ///cerr << std::setprecision(_option.getPrecision());
            ///cerr << "|   " << item->first;
            ///cerr << std::setfill(' ') << std::setw((_option.getWindow(0)-20)/2+1-_option.getPrecision()-(item->first).length() + _option.getWindow(0)%2) << " = ";
            ///cerr << std::setw((_option.getWindow(0)-60)/2+ _option.getPrecision()) << *variables[item->first];
            ///cerr << std::setw(19) << "[double]";
            ///cerr << std::setw(9) << sizeof(double) << "  Bytes" << endl;
            nBytesSum += sizeof(double);
        }
    }

    NumeReKernel::printPreFmt("|   -- " + toString((int)VarMap.size()) + " " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_VARS_AND")) + " ");
    if (_data.isValid() || _data.isValidCache() || _data.getStringElements())
    {
        if (_data.isValid() && _data.isValidCache() && _data.getStringElements())
        {
            NumeReKernel::printPreFmt(toString(2+CacheMap.size()));
            nDataSetNum = CacheMap.size()+2;
        }
        else if ((_data.isValid() && _data.isValidCache())
            || (_data.isValidCache() && _data.getStringElements()))
        {
            NumeReKernel::printPreFmt(toString(1+CacheMap.size()));
            nDataSetNum = CacheMap.size()+1;
        }
        else if (_data.isValid() && _data.getStringElements())
        {
            NumeReKernel::printPreFmt("2");
            nDataSetNum = 2;
        }
        else if (_data.isValidCache())
        {
            NumeReKernel::printPreFmt(toString((int)CacheMap.size()));
            nDataSetNum = CacheMap.size();
        }
        else
            NumeReKernel::printPreFmt("1");
    }
    else
        NumeReKernel::printPreFmt("0");
    NumeReKernel::printPreFmt(" " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_DATATABLES")) + " --");
    if (VarMap.size() > 9 && nDataSetNum > 9)
        NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0)-32-_lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length()-_lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
        ///cerr << std::setfill(' ') << std::setw(_option.getWindow(0)-32-_lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length()-_lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length()) << "Total: ";
    else if (VarMap.size() > 9 || nDataSetNum > 9)
        NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0)-31-_lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length()-_lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
        ///cerr << std::setfill(' ') << std::setw(_option.getWindow(0)-31-_lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length()-_lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length()) << "Total: ";
    else
        NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0)-30-_lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length()-_lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
        ///cerr << std::setfill(' ') << std::setw(_option.getWindow(0)-30-_lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length()-_lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length()) << "Total: ";
    if (nBytesSum >= 1024*1024)
        NumeReKernel::printPreFmt(strfill(toString(nBytesSum/(1024.0*1024.0), 4), 8) + " MBytes\n");
        ///cerr << std::setprecision(4) << std::setw(8) << nBytesSum/(1024.0*1024.0) << " MBytes";
    else if (nBytesSum >= 1024)
        NumeReKernel::printPreFmt(strfill(toString(nBytesSum/(1024.0), 4), 8) + " KBytes\n");
        ///cerr << std::setprecision(4) << std::setw(8) << nBytesSum/1024.0 << " KBytes";
    else
        NumeReKernel::printPreFmt(strfill(toString(nBytesSum), 8) + "  Bytes\n");
        ///cerr << std::setw(8) << nBytesSum << "  Bytes";
    //NumeReKernel::printPreFmt("\n");
    NumeReKernel::toggleTableStatus();
    make_hline();
    /*if(sExprTemp.length() != 0)
        _parser.SetExpr(sExprTemp);*/
    return;
}

// --> Listet alle deklarierten Konstanten <--
void parser_ListConst(const mu::ParserBase& _parser, const Settings& _option)
{
    const int nUnits = 20;
    string sUnits[nUnits] = {
        "_G[m^3/(kg s^2)]",
        "_R[J/(mol K)]",
        "_coul_norm[V m/(A s)]",
        "_c[m/s]",
        "_elek[A s/(V m)]",
        "_elem[A s]",
        "_gamma[1/(T s)]",
        "_g[m/s^2]",
        "_hartree[J]",
        "_h[J s]",
        "_k[J/K]",
        "_m_[kg]",
        "_magn[V s/(A m)]",
        "_mu_[J/T]",
        "_n[1/mol]",
        "_rydberg[1/m]",
        "_r[m]",
        "_stefan[J/(m^2 s K^4)]",
        "_wien[m K]",
        "_[---]"};
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTCONST_HEADLINE"))));
    make_hline();

    mu::valmap_type cmap = _parser.GetConst();
    if (!cmap.size())
    {
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTCONST_EMPTY")));
    }
    else
    {
        valmap_type::const_iterator item = cmap.begin();
        for (; item!=cmap.end(); ++item)
        {
            if (item->first[0] != '_')
                continue;
            NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow()-10)/2+2-_option.getPrecision()-(item->first).length() + _option.getWindow()%2) + strfill(toString(item->second, _option), _option.getPrecision()+(_option.getWindow()-50)/2));
            ///cerr << std::setprecision(_option.getPrecision());
            ///cerr << "|   " << item->first;
            ///cerr << std::setfill(' ') << std::setw((_option.getWindow()-10)/2+2-_option.getPrecision()-(item->first).length() + _option.getWindow()%2) << " = ";
            ///cerr << std::setw(_option.getPrecision()+(_option.getWindow()-50)/2) << item->second;
            ///cerr << std::setw(24);
            for (int i = 0; i < nUnits; i++)
            {
                if (sUnits[i].substr(0,sUnits[i].find('[')) == (item->first).substr(0,sUnits[i].find('[')))
                {
                    NumeReKernel::printPreFmt(strfill(sUnits[i].substr(sUnits[i].find('[')), 24) + "\n");
                    ///cerr << sUnits[i].substr(sUnits[i].find('['));
                    break;
                }
            }
            ///cerr << endl;
        }
        NumeReKernel::printPreFmt("|\n");
        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCONST_FOOTNOTE1"), _option));
        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCONST_FOOTNOTE2"), _option));
    }
    NumeReKernel::toggleTableStatus();
    make_hline();
    return;
}

// --> Listet alle im letzten Ausdruck verwendeten Variablen und ihre Werte <--
void parser_ListExprVar(mu::ParserBase& _parser, const Settings& _option, const Datafile& _data)
{
    string_type sExpr = _parser.GetExpr();
    //string sCacheSize = "Dimension: " + toString(_data.getCacheLines(false)) + " x " + toString(_data.getCacheCols(false));
    if (sExpr.length()==0)
    {
        cerr << toSystemCodePage("|-> " + _lang.get("PARSERFUNCS_LISTEXPRVAR_EMPTY")) << endl;
        return;
    }

    // Query the used variables (must be done after calc)
    make_hline();
    cerr << "|-> NUMERE: " << toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTEXPRVAR_HEADLINE"))) << endl;
    make_hline();
    cerr << LineBreak("|   " + _lang.get("PARSERFUNCS_LISTEXPRVAR_EXPR", _parser.GetExpr()), _option, true, 0, 14) << endl;

    varmap_type variables = _parser.GetUsedVar();
    if (!variables.size())
    {
        cerr << "|" << endl
             << toSystemCodePage("|-> "+_lang.get("PARSERFUNCS_LISTEXPRVAR_NOVARS")) << endl;
    }
    else
    {
        mu::varmap_type::const_iterator item = variables.begin();
        /*if (_parser.GetExpr().find("cache(") != string::npos)
        {
            cerr << "|   cache" << std::setfill(' ') << std::setw(36) << sCacheSize << std::setw(19) << "[double x double]";
            if (_data.getSize() >= 1024*1024)
                cerr << std::setprecision(6) << std::setw(9) << _data.getSize()/(1024.0*1024.0) << " MBytes";
            else if (_data.getSize() >= 1024)
                cerr << std::setprecision(6) << std::setw(9) << _data.getSize()/1024.0 << " KBytes";
            else
                cerr << std::setw(9) << _data.getSize() << "  Bytes";
            cerr << endl;
        }*/


        for (; item!=variables.end(); ++item)
        {
            _parser.SetExpr(item->first);
            cerr << std::setprecision(_option.getPrecision());
            cerr << "|   " << item->first;
            cerr << std::setfill(' ') << std::setw((_option.getWindow()-20)/2+1-_option.getPrecision()-(item->first).length() + _option.getWindow()%2) << " = ";
            cerr << std::setw(_option.getPrecision()+(_option.getWindow()-60)/2) << _parser.Eval();
            cerr << std::setw(19) << "[double]";
            cerr << std::setw(9) << sizeof(double) << "  Bytes" << endl;
        }
        cerr << "|   -- " << _lang.get("PARSERFUNCS_LISTEXPRVAR_FOOTNOTE", toString((int)variables.size())) << " --" << endl;

    }
    _parser.SetExpr(sExpr);
    make_hline();
    return;
}

// --> Listet alle erweiterten Kommandos <--
void parser_ListCmd(const Settings& _option)
{
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTCMD_HEADLINE")))); //PRSRFUNC_LISTCMD_*
    make_hline();
    NumeReKernel::printPreFmt(LineBreak("|   " +_lang.get("PARSERFUNCS_LISTCMD_TABLEHEAD"), _option, 0)+"\n|\n");
    ///cerr << "|" << endl;
    vector<string> vCMDList = _lang.getList("PARSERFUNCS_LISTCMD_CMD_*");

    for (unsigned int i = 0; i < vCMDList.size(); i++)
    {
        NumeReKernel::printPreFmt(LineBreak("|   "+vCMDList[i], _option, false, 0, 34)+"\n");
    }
    NumeReKernel::printPreFmt("|\n");
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCMD_FOOTNOTE1"), _option));
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCMD_FOOTNOTE2"), _option));
    NumeReKernel::toggleTableStatus();
    make_hline();
}

// --> Listet alle Einheitenumrechnungen <--
void parser_ListUnits(const Settings& _option) //PRSRFUNC_LISTUNITS_*
{
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTUNITS_HEADLINE")))); //(_option.getWindow()-x)/3
    make_hline(); // 11       21  x=17             15   x=35      1               x=2      26
    //cerr << "|     Symbol     Bezeichnung          Dimension              Umrechnung  Einheit" << endl;
    //cerr << "|     Symbol     " << std::setfill(' ') << std::setw((_option.getWindow()-17)/3 + (_option.getWindow()+1)%3) << std::left << "Bezeichnung"
    //                            << std::setfill(' ') << std::setw((_option.getWindow()-35)/3+1) << std::left << "Dimension"
    //                            << std::setfill(' ') << std::setw((_option.getWindow()-2)/3) << std::right << "Umrechnung  Einheit" << endl;
    printUnits(_lang.get("PARSERFUNCS_LISTUNITS_SYMBOL"), _lang.get("PARSERFUNCS_LISTUNITS_DESCRIPTION"), _lang.get("PARSERFUNCS_LISTUNITS_DIMENSION"), _lang.get("PARSERFUNCS_LISTUNITS_UNIT"), _option.getWindow());
    NumeReKernel::printPreFmt("|\n");
    //cerr << "|" << endl;
    printUnits("1'A",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ANGSTROEM"),        "L",           "1e-10      [m]", _option.getWindow());
    printUnits("1'AU",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ASTRO_UNIT"),       "L",           "1.4959787e11      [m]", _option.getWindow());
    printUnits("1'b",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_BARN"),             "L^2",         "1e-28    [m^2]", _option.getWindow());
    printUnits("1'cal", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CALORY"),           "M L^2 / T^2", "4.1868      [J]", _option.getWindow());
    printUnits("1'Ci",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CURIE"),            "1 / T",       "3.7e10     [Bq]", _option.getWindow());
    printUnits("1'eV",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ELECTRONVOLT"),     "M L^2 / T^2", "1.60217657e-19      [J]", _option.getWindow());
    printUnits("1'fm",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FERMI"),            "L",           "1e-15      [m]", _option.getWindow());
    printUnits("1'ft",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FOOT"),             "L",           "0.3048      [m]", _option.getWindow());
    printUnits("1'Gs",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_GAUSS"),            "M / (T^2 I)", "1e-4      [T]", _option.getWindow());
    printUnits("1'in",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_INCH"),             "L",           "0.0254      [m]", _option.getWindow());
    printUnits("1'kmh", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_VELOCITY"),         "L / T",       "0.2777777...    [m/s]", _option.getWindow());
    printUnits("1'kn",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_KNOTS"),            "L / T",       "0.5144444...    [m/s]", _option.getWindow());
    printUnits("1'l",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_LITERS"),           "L^3",         "1e-3    [m^3]", _option.getWindow());
    printUnits("1'ly",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_LIGHTYEAR"),        "L",           "9.4607305e15      [m]", _option.getWindow());
    printUnits("1'mile",_lang.get("PARSERFUNCS_LISTUNITS_UNIT_MILE"),             "L",           "1609.344      [m]", _option.getWindow());
    printUnits("1'mol", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_MOL"),              "N",           "6.022140857e23      ---", _option.getWindow());
    printUnits("1'mph", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_VELOCITY"),         "L / T",       "0.44703722    [m/s]", _option.getWindow());
    printUnits("1'Ps",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_POISE"),            "M / (L T)",   "0.1   [Pa s]", _option.getWindow());
    printUnits("1'pc",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_PARSEC"),           "L",           "3.0856776e16      [m]", _option.getWindow());
    printUnits("1'psi", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_PSI"),              "M / (L T^2)", "6894.7573     [Pa]", _option.getWindow());
    printUnits("1'TC",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CELSIUS"),          "Theta",       "274.15      [K]", _option.getWindow());
    printUnits("1'TF",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FAHRENHEIT"),       "Theta",       "255.92778      [K]", _option.getWindow());
    printUnits("1'Torr",_lang.get("PARSERFUNCS_LISTUNITS_UNIT_TORR"),             "M / (L T^2)", "133.322     [Pa]", _option.getWindow());
    printUnits("1'yd",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_YARD"),             "L",           "0.9144      [m]", _option.getWindow());
    NumeReKernel::printPreFmt("|\n");
    printUnits("1'G",   "(giga)",             "---",           "1e9      ---", _option.getWindow());
    printUnits("1'M",   "(mega)",             "---",           "1e6      ---", _option.getWindow());
    printUnits("1'k",   "(kilo)",             "---",           "1e3      ---", _option.getWindow());
    printUnits("1'm",   "(milli)",            "---",           "1e-3      ---", _option.getWindow());
    printUnits("1'mu",  "(micro)",            "---",           "1e-6      ---", _option.getWindow());
    printUnits("1'n",   "(nano)",             "---",           "1e-9      ---", _option.getWindow());

    NumeReKernel::printPreFmt("|\n");
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTUNITS_FOOTNOTE"), _option));
    NumeReKernel::toggleTableStatus();
    make_hline();

    return;
}

// --> Listet alle vorhandenen Plugins <--
void parser_ListPlugins(Parser& _parser, Datafile& _data, const Settings& _option)
{
    string sDummy = "";
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print(toSystemCodePage("NUMERE: "+toUpperCase(_lang.get("PARSERFUNCS_LISTPLUGINS_HEADLINE"))));
    make_hline();
    if (!_plugin.getPluginCount())
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTPLUGINS_EMPTY")));
    else
    {
        NumeReKernel::printPreFmt(LineBreak("|   "+_lang.get("PARSERFUNCS_LISTPLUGINS_TABLEHEAD"), _option, 0) + "\n");
        NumeReKernel::printPreFmt("|\n");
        for (unsigned int i = 0; i < _plugin.getPluginCount(); i++)
        {
            string sLine = "|   ";
            if (_plugin.getPluginCommand(i).length() > 18)
                sLine += _plugin.getPluginCommand(i).substr(0,15) + "...";
            else
                sLine += _plugin.getPluginCommand(i);
            sLine.append(23-sLine.length(), ' ');

            sLine += _lang.get("PARSERFUNCS_LISTPLUGINS_PLUGININFO", _plugin.getPluginName(i), _plugin.getPluginVersion(i), _plugin.getPluginAuthor(i));
            if (_plugin.getPluginDesc(i).length())
            {
                sLine += "$" + _plugin.getPluginDesc(i);
            }
            sLine = '"' + sLine + "\" -nq";
            if (!parser_StringParser(sLine, sDummy, _data, _parser, _option, true))
            {
                NumeReKernel::toggleTableStatus();
                throw SyntaxError(SyntaxError::STRING_ERROR, "", SyntaxError::invalid_position);
            }
            NumeReKernel::printPreFmt(LineBreak(sLine, _option, true, 0, 25) + "\n");
        }
    }
    NumeReKernel::toggleTableStatus();
    make_hline();
    return;
}

// --> Ein kleines Splash (Logo) <--
void parser_splash(Parser& _parser)
{
    mu::console() << _nrT("|-> RECHNER-MODUS (v ") << sParserVersion << _nrT(")\n");
    mu::console() << _nrT("|   =======================\n");
    mu::console() << _nrT("|-> Basiert auf der muParser-Library v ") << _parser.GetVersion(pviBRIEF) << _nrT("\n|\n");
    return;
}

/* --> Diese Funktion durchsucht einen gegebenen String sLine nach den Elementen "data(" oder "cache(" und erstetzt diese
 *     entsprechend der Syntax durch Elemente (oder Vektoren) aus dem Datenfile oder dem Cache. Falls des Weiteren auch
 *     noch Werte in den Cache geschrieben werden sollen (das Datenfile ist READ-ONLY), wird dies von dieser Funktion
 *     ebenfalls determiniert. <--
 * --> Um die ggf. ersetzten Vektoren weiterverwenden zu koennen, muss die Funktion parser_VectorToExpr() auf den String
 *     sLine angewendet werden. <--
 */
string parser_GetDataElement(string& sLine, Parser& _parser, Datafile& _data, const Settings& _option, bool bReplaceNANs)
{
    string sCache = "";             // Rueckgabe-string: Ggf. der linke Teil der Gleichung, falls es sich um eine Zuweisung handelt
    string sLine_Temp = "";         // temporaerer string, da wir die string-Referenz nicht unnoetig veraendern wollen
    unsigned int eq_pos = string::npos;                // int zum Zwischenspeichern der Position des "="

    int nParenthesis = 0;

    // Evaluate possible cached equations
    if (_parser.HasCachedAccess() && !_parser.IsCompiling())
    {
        mu::CachedDataAccess _access;
        Indices _idx;
        for (size_t i = 0; i < _parser.HasCachedAccess(); i++)
        {
            _access = _parser.GetCachedAccess(i);
            _idx = parser_getIndices(_access.sAccessEquation, _parser, _data, _option);
            if ((_idx.nI[0] == -1 || _idx.nJ[0] == -1) && !_idx.vI.size() && !_idx.vJ.size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position);
            if (_idx.nI[1] == -2 && _idx.nJ[1] == -2)
                throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);
            if (_idx.nI[1] == -1)
                _idx.nI[1] = _idx.nI[0];
            if (_idx.nJ[1] == -1)
                _idx.nJ[1] = _idx.nJ[0];
            if (_idx.nI[1] == -2)
                _idx.nI[1] = _data.getLines(_access.sCacheName, false)-1;
            if (_idx.nJ[1] == -2)
                _idx.nJ[1] = _data.getCols(_access.sCacheName, false)-1;

            vector<long long int> vLine;
            vector<long long int> vCol;

            if (_idx.vI.size() && _idx.vJ.size())
            {
                vLine = _idx.vI;
                vCol = _idx.vJ;
            }
            else
            {
                for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                    vLine.push_back(i);
                for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                    vCol.push_back(j);
            }

            //_parser.SetVectorVar(_access.sVectorName, _data.getElement(vLine, vCol, _access.sCacheName));
            _data.copyElementsInto(_parser.GetVectorVar(_access.sVectorName), vLine, vCol, _access.sCacheName);
            _parser.UpdateVectorVar(_access.sVectorName);
        }
        sLine = _parser.GetCachedEquation();
        sCache = _parser.GetCachedTarget();
        return sCache;
    }


    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        if (sLine[i] == '(' && !isInQuotes(sLine, i, true))
            nParenthesis++;
        if (sLine[i] == ')' && !isInQuotes(sLine, i, true))
            nParenthesis--;
    }
    if (nParenthesis)
        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, SyntaxError::invalid_position);

    // --> Findest du "data("? <--
    if (sLine.find("data(") != string::npos)
    {
        // --> Sind ueberhaupt Daten vorhanden? <--
        if (!_data.isValid() && (!sLine.find("data(") || checkDelimiter(sLine.substr(sLine.find("data(")-1,6))))
        {
            /* --> Nein? Mitteilen, BOOLEAN setzen (der die gesamte, weitere Auswertung abbricht)
             *     und zurueck zur aufrufenden Funktion <--
             */
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sLine, SyntaxError::invalid_position);
        }
        // --> Ist rechts von "data(" noch ein "=" und gehoert das nicht zu einem Logik-Ausdruck? <--
        eq_pos = sLine.find("=", sLine.find("data(")+5);
        if (eq_pos != string::npos
            && sLine[eq_pos+1] != '='
            && sLine[eq_pos-1] != '<'
            && sLine[eq_pos-1] != '>'
            && sLine[eq_pos-1] != '!'
            && !parser_CheckMultArgFunc(sLine.substr(0,sLine.find("data(")), sLine.substr(sLine.find(")",sLine.find("data(")+1)))
            )
        {
            if (sLine.substr(sLine.find("data(")+5,sLine.find(",", sLine.find("data(")+5)-sLine.find("data(")-5).find("#") != string::npos)
            {
                sCache = sLine.substr(0,eq_pos);
                sLine = sLine.substr(eq_pos+1);
            }
            else
            {
                // --> Ja? Dann brechen wir ebenfalls ab, da wir nicht in data() schreiben wollen <--
                throw SyntaxError(SyntaxError::READ_ONLY_DATA, sLine, SyntaxError::invalid_position);
            }
        }
        /* --> Diese Schleife ersetzt nacheinander alle Stellen, in denen "data(" auftritt, durch "Vektoren", die
         *     in einer anderen Funktion weiterverarbeitet werden koennen. Eine aehnliche Schleife findet sich
         *     auch fuer "cache(" etwas weiter unten. <--
         * --> Wenn diese Schleife abgearbeitet ist, wird natuerlich auch noch geprueft, ob auch "cache(" gefunden
         *     wird und ggf. die Schleife fuer den Cache gestartet. <--
         */
        if (sLine.find("data(") != string::npos)
            parser_ReplaceEntities(sLine, "data(", _data, _parser, _option, bReplaceNANs);
    }

    /* --> Jetzt folgt der ganze Spass fuer "cache(". Hier ist relativ viel aehnlich, allerdings gibt es
     *     noch den Fall, dass "cache(" links des "=" auftauchen darf, da es sich dabei um eine Zuweisung
     *     eines (oder mehrerer) Wert(e) an den Cache handelt. <--
     */
    if (_data.containsCacheElements(sLine))
    {
        // --> Ist links vom ersten "cache(" ein "=" oder ueberhaupt ein "=" im gesamten Ausdruck? <--
        eq_pos = sLine.find("=");
        if (eq_pos == string::npos              // gar kein "="?
            || !_data.containsCacheElements(sLine.substr(0,eq_pos))    // nur links von "cache("?
            || (_data.containsCacheElements(sLine.substr(0,eq_pos))   // wenn rechts von "cache(", dann nur Logikausdruecke...
                && (sLine[eq_pos+1] == '='
                    || sLine[eq_pos-1] == '<'
                    || sLine[eq_pos-1] == '>'
                    || sLine[eq_pos-1] == '!'
                    )
                )
            )
        {
            // --> Cache-Lese-Status aktivieren! <--
            _data.setCacheStatus(true);

            try
            {
                //cerr << _data.getCacheCount() << endl;
                //map<string,long long int> mCachesMap = _data.getCacheList();
                for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); iter++)
                {
                    //cerr << (iter->first)+"(" << endl;
                    if (sLine.find((iter->first)+"(") != string::npos)
                        parser_ReplaceEntities(sLine, (iter->first)+"(", _data, _parser, _option, bReplaceNANs);
                }
            }
            catch (...)
            {
                _data.setCacheStatus(false);
                throw;
            }
            // --> Cache-Lese-Status deaktivieren <--
            _data.setCacheStatus(false);
        }
        else
        {
            /* --> Nein? Dann ist das eine Zuweisung. Wird komplizierter zu loesen. Außerdem kann dann rechts von
             *     "=" immer noch "cache(" auftreten. <--
             * --> Suchen wir zuerst mal nach der Position des "=" und speichern diese in eq_pos <--
             */
            /// !Achtung! Logikausdruecke abfangen!
            //eq_pos = sLine.find("=");
            // --> Teilen wir nun sLine an "=": Der Teillinks in sCache, der Teil rechts in sLine_Temp <--
            sCache = sLine.substr(0,eq_pos);
            StripSpaces(sCache);
            while (sCache[0] == '(')
                sCache.erase(0,1);
            // --> Gibt's innerhalb von "cache()" nochmal einen Ausdruck "cache("? <--
            if (_data.containsCacheElements(sCache.substr(sCache.find('(')+1)))
            {
                /*if (!_data.isValidCache())
                {
                    throw NO_CACHED_DATA;
                }*/
                _data.setCacheStatus(true);

                sLine_Temp = sCache.substr(sCache.find('(')+1);
                if (_option.getbDebug())
                    mu::console() << _nrT("|-> DEBUG: sLine_Temp = ") << sLine_Temp << endl;
                for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                {
                    if (sLine_Temp.find(iter->first+"(") != string::npos)
                        parser_ReplaceEntities(sLine_Temp, iter->first+"(", _data, _parser, _option, bReplaceNANs);
                }
                sCache = sCache.substr(0,sCache.find('(')+1) + sLine_Temp;
                if (_option.getbDebug())
                    mu::console() << _nrT("|-> DEBUG: sCache = ") << sCache << endl;
                _data.setCacheStatus(false);
            }
            sLine_Temp = sLine.substr(eq_pos+1);

            // --> Gibt es rechts von "=" nochmals "cache("? <--
            if (_data.containsCacheElements(sLine_Temp))
            {
                /* --> Ja? Geht eigentlich trotzdem wie oben, mit Ausnahme, dass ueberall wo "sLine" aufgetreten ist,
                 *     nun "sLine_Temp" auftritt <--
                 */
                if (_option.getbDebug())
                    mu::console() << _nrT("|-> DEBUG: sLine_Temp.find(...) = ")
                                  << sLine_Temp.find("cache(") << endl;
                /*if (!_data.isValidCache())
                {
                    throw NO_CACHED_DATA;
                }*/

                _data.setCacheStatus(true);

                try
                {
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (sLine_Temp.find(iter->first+"(") != string::npos)
                            parser_ReplaceEntities(sLine_Temp, iter->first+"(", _data, _parser, _option, bReplaceNANs);
                    }
                }
                catch (...)
                {
                    _data.setCacheStatus(false);
                    throw;
                }
                _data.setCacheStatus(false);
            }
            // --> sLine_Temp an sLine zuweisen <--
            sLine = sLine_Temp;
        }
    }

    return sCache;
}

// --> Diese Funktion ersetzt Vektor-Ausdruecke durch vielfach-Ausdruecke <--
void parser_VectorToExpr(string& sLine, const Settings& _option)
{
    //string* sFinalVector;
    string sVectors[32];
    string sScalars[33];
    //string ** sVec_Sep;
    string sTemp = sLine;
    string sInterVector = "";
    string sExprParts[3] = {"", "", ""};
    string sDelim = "+-*/^&|!%";
    int nDim = 0;
    int nDim_vec = 0;
    int nCount = 0;
    int nScalars = 0;
    unsigned int nPos = 0;
    unsigned int nPos_2 = 0;
    size_t nQuotes = 0;
    bool bIsStringExpression = containsStrings(sLine);

    for (int i = 0; i < 32; i++)
    {
        sVectors[i] = "";
        sScalars[i] = "";
    }
    sScalars[32] = "";
    if (_option.getbDebug())
        cerr << "|-> DEBUG: sLine = " << sTemp.substr(0,100) << endl;
    for (nPos_2 = 0; nPos_2 < sTemp.length(); nPos_2++)
    //do
    {
        if (sTemp[nPos_2] == '"')
        {
            if (!nPos_2 || (nPos_2 && sTemp[nPos_2-1] != '\\'))
                nQuotes++;
        }
        if ((nQuotes % 2) || sTemp[nPos_2] != '{')
            //nPos_2 = sTemp.find("{", nPos);
            continue;
        if (isToStringArg(sTemp, nPos_2))
            continue;


        /*if (isInQuotes(sTemp, nPos_2, false) || isToStringArg(sTemp, nPos_2))
        {
            nPos++;
            continue;
        }*/
        //nPos_2 = sTemp.find("{{", nPos);
        if (isMultiValue(sTemp.substr(nPos, nPos_2-nPos), true))
        {
            sInterVector = sTemp.substr(nPos, nPos_2-nPos);
            /*if (_option.getbDebug())
                cerr << "|" << endl << "|-> DEBUG: sInterVector = " << sInterVector << endl;*/
            int nParenthesis = 0;
            for (unsigned int i = 0; i < sInterVector.length(); i++)
            {
                /*if (_option.getbDebug())
                    cerr << "|-> DEBUG: sInterVector[" << i << "] = " << sInterVector[i] << ", nParenthesis = " << nParenthesis << endl;*/
                if (sInterVector[i] == '(')
                    nParenthesis++;
                if (sInterVector[i] == ')')
                    nParenthesis--;
                if (sInterVector[i] == ',' && nParenthesis <= 0)
                {
                    if (!nParenthesis)
                    {
                        sExprParts[0] = sInterVector.substr(0,i);
                        sExprParts[2] = sInterVector.substr(i+1);
                        break;
                    }
                    else
                    {
                        for (int j = sTemp.rfind("{", nPos); j >= 0; j--)
                        {
                            if (sTemp[j] == '(')
                                nParenthesis++;
                            else if (sTemp[j] == ')')
                                nParenthesis--;
                            if (!nParenthesis)
                                break;
                        }
                        if (!nParenthesis)
                        {
                            sExprParts[0] = sInterVector.substr(0,i);
                            sExprParts[2] = sInterVector.substr(i+1);
                            break;
                        }
                        else
                        {
                            sLine = "";
                            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, SyntaxError::invalid_position);
                        }
                    }
                }
            }
            if (_option.getbDebug())
                cerr << "|-> DEBUG: sExprParts[0] = " << sExprParts[0] << "; sExprParts[2] = " << sExprParts[2] << endl;
            while (isMultiValue(sExprParts[2]))
            {
                //sExprParts[2] = "(" + sExprParts[2] + ")";
                parser_SplitArgs(sExprParts[2], sExprParts[1], ',', _option, true);
                sExprParts[2] = sExprParts[1];
            }
            sExprParts[1] = sInterVector.substr(sExprParts[0].length(),sInterVector.length()-sExprParts[0].length()-sExprParts[2].length());
            sExprParts[0] = sTemp.substr(0,nPos) + sExprParts[0];
            sExprParts[2] = sExprParts[2] + sTemp.substr(nPos_2);
            if (_option.getbDebug())
            {
                cerr << "|-> DEBUG: ";
                for (int i = 0; i < 3; i++)
                {
                    cerr << "sExprParts[" << i << "] = " << sExprParts[i] << "; ";
                }
                cerr << endl;
            }

            if (sExprParts[0].find("{") != string::npos)
                parser_VectorToExpr(sExprParts[0], _option);
            if (sExprParts[2].find("{") != string::npos)
                parser_VectorToExpr(sExprParts[2], _option);
            sLine = sExprParts[0] + sExprParts[1] + sExprParts[2];
            return;
        }
        nPos = sTemp.find("}", nPos);
        if (nPos == string::npos)
            break;
        if (sTemp[nPos+1] =='}')
            nPos += 2;
        else
            nPos++;
    }

    nPos = 0;
    nPos_2 = 0;
    nQuotes = 0;

    //cerr << sTemp.substr(0,100) << endl;

    for (nPos = 0; nPos < sTemp.length(); nPos++)
    //do
    {
        if (sTemp[nPos] == '"')
        {
            if (!nPos || (nPos && sTemp[nPos-1] != '\\'))
                nQuotes++;
        }
        if (nCount == 31)
        {
            throw SyntaxError(SyntaxError::TOO_MANY_VECTORS, sLine, SyntaxError::invalid_position);
        }
        if (sTemp[nPos] != '{' || (nQuotes % 2))
            continue;
        if (isToStringArg(sTemp, nPos))
            continue;
        /*nPos = sTemp.find('{', nPos);
        if (isInQuotes(sTemp, nPos, false) || isToStringArg(sTemp, nPos))
        {
            nPos++;
            continue;
        }*/
        nDim_vec = 0;
        if (getMatchingParenthesis(sTemp.substr(nPos)) == string::npos)
            throw SyntaxError(SyntaxError::INCOMPLETE_VECTOR_SYNTAX, sLine, SyntaxError::invalid_position);
        sVectors[nCount] = sTemp.substr(nPos+1, getMatchingParenthesis(sTemp.substr(nPos))-1);
        //sVectors[nCount] = sTemp.substr(sTemp.find("{", nPos)+1, getMatchingParenthesis(sTemp.substr(sTemp.find('{', nPos)))-1);
        if (sTemp.find('{', nPos) != 0)
            sScalars[nScalars] += sTemp.substr(0, sTemp.find('{', nPos));
        if (sVectors[nCount][0] == '{' && parser_CheckMultArgFunc(sScalars[nScalars], sTemp.substr(sTemp.find('}',nPos)+1)))
        {
            sVectors[nCount].erase(0,1);
            if (sVectors[nCount].back() == '}')
                sVectors[nCount].pop_back();
            sScalars[nScalars] += sVectors[nCount];
            sTemp = sTemp.substr(sTemp.find("}}")+2);
            continue;
        }
        else if (parser_CheckMultArgFunc(sScalars[nScalars], sTemp.substr(sTemp.find('}',nPos)+1)))
        {
            sScalars[nScalars] += sVectors[nCount];
            sTemp = sTemp.substr(sTemp.find("}", nPos)+1);
            continue;
        }
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sVectors[" << nCount << "] = " << sVectors[nCount].substr(0,100) << ", sScalars[" << nScalars << "] = " << sScalars[nScalars].substr(0,100) << endl;
        if (sVectors[nCount][0] == '{')
        {
            sVectors[nCount].erase(0,1);
            if (sVectors[nCount].back() == '}')
                sVectors[nCount].pop_back();
            sTemp.erase(0,sTemp.find('}', nPos)+2);
        }
        else
            sTemp.erase(0,sTemp.find('}', nPos)+1);
        nPos = 0;
        if (sVectors[nCount].length())
        {
            string sTempCopy = sVectors[nCount];
            while (sTempCopy.length())
            {
                if (getNextArgument(sTempCopy, true).length())
                    nDim_vec++;
            }
        }
        if (nDim_vec > nDim)
            nDim = nDim_vec;
        nCount++;
        nScalars++;
        if (!sTemp.length())
            break;
    }
    //while (sTemp.find('{', nPos) != string::npos);
    if (sTemp.length())
    {
        sScalars[nScalars] += sTemp;
        nScalars++;
    }

    sTemp.clear();
    sLine.clear();
    if (!nDim)
    {
        for (int i = 0; i < nScalars; i++)
            sLine += sScalars[i];
    }
    else
    {
        for (int i = 0; i < nDim; i++)
        {
            for (int j = 0; j < nCount; j++)
            {
                sLine += sScalars[j];
                sTemp.clear();
                if (sVectors[j].length())
                    sTemp = getNextArgument(sVectors[j], true);
                else
                {
                    sTemp = parser_AddVectorComponent(sVectors[j], sScalars[j], sScalars[j+1], bIsStringExpression);
                }

                if (!bIsStringExpression)
                {
                    for (unsigned int n = 0; n < sDelim.length(); n++)
                    {
                        if (sTemp.find(sDelim[n]) != string::npos)
                        {
                            sTemp = "(" + sTemp + ")";
                            break;
                        }
                    }
                }
                sLine += sTemp;
            }
            if (nScalars > nCount)
                sLine += sScalars[nScalars-1];
            if (i < nDim-1)
                sLine += ",";
        }
    }
    if (_option.getbDebug())
        cerr << "|-> DEBUG: Returning sLine = " << sLine.substr(0,80) << endl;
    return;
}

// --> Diese Funktion ergaenzt Vektorkomponenten entsprechend einer Heuristik <--
string parser_AddVectorComponent(const string& sVectorComponent, const string& sLeft, const string& sRight, bool bAddStrings)
{
    bool bOneLeft = false;
    bool bOneRight = false;
    if (sVectorComponent.length())
    {
        return sVectorComponent;
    }
    else if (bAddStrings)
        return "\"\"";
    else if (!sLeft.length() && !sRight.length())
    {
        return "0";
    }
    else
    {
        for (int i = sLeft.length()-1; i >= 0; i--)
        {
            if (sLeft[i] != ' ')
            {
                if (sLeft[i] == '(')
                {
                    for (int j = i-1; j >= 0; j--)
                    {
                        if (sLeft[j] == '(')
                        {
                            bOneLeft = true;
                            break;
                        }
                        if (sLeft[j] == '/')
                            return "1";
                    }
                }
                else if (sLeft[i] == '/')
                    return "1";
                break;
            }
        }
        for (unsigned int i = 0; i < sRight.length(); i++)
        {
            if (sRight[i] != ' ')
            {
                if (sRight[i] == ')')
                {
                    for (unsigned int j = i+1; j < sRight.length(); j++)
                    {
                        if (sRight[j] == ')')
                        {
                            bOneRight = true;
                            break;
                        }
                    }
                }
                break;
            }
        }
        if ((bOneLeft && bOneRight))
            return "1";
    }
    return "0";
}

// --> Prueft, ob ein Ausdruck Nicht-Leer ist (also auch, dass er nicht nur aus Leerzeichen besteht) <--
bool parser_ExprNotEmpty(const string& sExpr)
{
    if (!sExpr.length())
        return false;
    else
    {
        for (unsigned int i = 0; i < sExpr.length(); i++)
        {
            if (sExpr[i] != ' ')
                return true;
        }
        return false;
    }
}

/* --> Diese Funktion prueft, ob das Argument, dass sich zwischen sLeft und sRight befindet, in einer
 *     Multi-Argument-Funktion steht <--
 */
bool parser_CheckMultArgFunc(const string& sLeft, const string& sRight)
{
    int nPos = 0;
    string sFunc = "";
    bool bCMP = false;

    for (unsigned int i = 0; i < sRight.length(); i++)
    {
        if (sRight[i] != ' ')
        {
            if (sRight[i] == ')')
                break;
            else if (sRight[i] == ',')
            {
                if (/*sRight.find(',', i+1) != string::npos &&*/ sRight.find(')', i+1) != string::npos
                    /*&& sRight.find(',', i+1) < sRight.find(')', i+1)*/)
                    bCMP = true;
                else
                    return false;
                break;
            }
        }
    }
    for (int i = sLeft.length()-1; i >= 0; i--)
    {
        if (sLeft[i] != ' ')
        {
            if (sLeft[i] != '(')
                return false;
            nPos = i;
            break;
        }
    }
    if (nPos == 2)
    {
        sFunc = sLeft.substr(nPos - 2,2);
        if (sFunc == "or" && !bCMP)
            return true;
        return false;
    }
    else if (nPos >= 3)
    {
        sFunc = sLeft.substr(nPos - 3,3);
        if (sFunc == "max" && !bCMP)
            return true;
        else if (sFunc == "min" && !bCMP)
            return true;
        else if (sFunc == "sum" && !bCMP)
            return true;
        else if (sFunc == "avg" && !bCMP)
            return true;
        else if (sFunc == "num" && !bCMP)
            return true;
        else if (sFunc == "cnt" && !bCMP)
            return true;
        else if (sFunc == "med" && !bCMP)
            return true;
        else if (sFunc == "pct" && bCMP)
            return true;
        else if (sFunc == "std" && !bCMP)
            return true;
        else if (sFunc == "prd" && !bCMP)
            return true;
        else if (sFunc == "and" && !bCMP)
            return true;
        else if (sFunc.substr(1) == "or" && !bCMP)
            return true;
        else if (sFunc == "cmp" && bCMP)
        {
            //cerr << "cmp()" << endl;
            return true;
        }
        else if (sFunc == "orm" && !bCMP)
        {
            if (nPos > 3 && sLeft.substr(nPos - 4, 4) == "norm")
                return true;
            else
                return false;
        }
        else
            return false;
    }
    else
        return false;
}

/* --> Diese Funktion ersetzt in einem gegebenen String sLine alle Entities (sEntity) von "data(" oder "cache(" und bricht
 *     ab, sobald ein Fehler auftritt. Der Fehler wird in der Referenz von bSegmentationFault gespeichert und kann in
 *     in der aufrufenden Funktion weiterverarbeitet werden <--
 */
void parser_ReplaceEntities(string& sLine, const string& sEntity, Datafile& _data, Parser& _parser, const Settings& _option, bool bReplaceNANs)
{
    Indices _idx;
    string sEntityOccurence = "";
    string sEntityName = sEntity.substr(0, sEntity.find('('));
    ///string sOprtChrs = "+-*/^&|!=?%";
    unsigned int nPos = 0;
    bool bWriteStrings = false;
    bool bWriteFileName = false;
    vector<double> vEntityContents;
    string sEntityReplacement = "";
    string sEntityStringReplacement = "";


    /* --> Diese Schleife ersetzt nacheinander alle Stellen, in denen sEntity auftritt, durch "Vektoren", die
     *     in einer anderen Funktion weiterverarbeitet werden koennen. <--
     */
    do
    {
        /* --> Zunaechst muessen wir herausfinden welche(s) Datenelement(e) wir aus der Datafile-Tabelle
         *     extrahieren sollen. Dies wird durch die Syntax data(i,j) bestimmt, wobei i der Zeilen- und
         *     j der Spaltenindex ist. i und j koennen mittels der Syntax "i_0:i_1" bzw. "j_0:j_1" einen
         *     Bereich bestimmen, allerdings (noch) keine Matrix. (Also entweder nur i oder j) <--
         * --> Speichere zunaechst den Teil des Strings nach "data(" in si_pos[0] <--
         */
        sEntityOccurence = sLine.substr(sLine.find(sEntity, nPos));
        nPos = sLine.find(sEntity, nPos);
        if (nPos && !checkDelimiter(sLine.substr(nPos-1, sEntity.length()+1)))
        {
            nPos++;
            continue;
        }
        //sEntityOccurence = sEntityOccurence.substr(0,getMatchingParenthesis(sEntityOccurence.substr(sEntityOccurence.find('('))) + sEntityOccurence.find('(')+1);
        sEntityOccurence = sEntityOccurence.substr(0,getMatchingParenthesis(sEntityOccurence)+1);
        vEntityContents.clear();
        sEntityReplacement.clear();
        sEntityStringReplacement.clear();

        // Reading the indices happens in this function
        _idx = parser_getIndices(sEntityOccurence, _parser, _data, _option);

        // evaluate the indices regarding the possible combinations
        if ((_idx.nI[0] == -1 || _idx.nJ[0] == -1) && !_idx.vI.size() && !_idx.vJ.size())
            throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position);
        if (_idx.nI[1] == -2 && _idx.nJ[1] == -2)
            throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);
        if (_idx.nI[1] == -1)
            _idx.nI[1] = _idx.nI[0];
        if (_idx.nJ[1] == -1)
            _idx.nJ[1] = _idx.nJ[0];
        if (_idx.nI[1] == -2)
            _idx.nI[1] = _data.getLines(sEntityName, false)-1;
        if (_idx.nJ[1] == -2)
            _idx.nJ[1] = _data.getCols(sEntityName, false)-1;
        if (_idx.nI[0] == -3)
            bWriteStrings = true;
        if (_idx.nJ[0] == -3)
            bWriteFileName = true;

        if (bWriteFileName)
        {
            sEntityStringReplacement = "\"" + _data.getDataFileName(sEntityName) + "\"";
        }
        else if (bWriteStrings)
        {
            if (_idx.vJ.size())
            {
                for (size_t j = 0; j < _idx.vJ.size(); j++)
                {
                    sEntityStringReplacement += "\"" + _data.getHeadLineElement(_idx.vJ[j], sEntityName) + "\", ";
                }
            }
            else
            {
                for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                {
                    sEntityStringReplacement += "\"" + _data.getHeadLineElement(j, sEntityName) + "\", ";
                }
            }
            if (sEntityStringReplacement.length())
                sEntityStringReplacement.erase(sEntityStringReplacement.rfind(','));
        }

        // create a vector containing the data
        if (_idx.vI.size() && _idx.vJ.size())
        {
            vEntityContents = _data.getElement(_idx.vI, _idx.vJ, sEntityName);
        }
        else
        {
            parser_CheckIndices(_idx.nI[0], _idx.nI[1]);
            parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);
            for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
            {
                for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                {
                    vEntityContents.push_back(_data.getElement(i, j, sEntityName));
                }
            }
        }

        // replace the occurences
        if (bWriteStrings)
        {
            _parser.DisableAccessCaching();
            parser_ReplaceEntityStringOccurence(sLine, sEntityOccurence, sEntityStringReplacement);
        }
        else
        {
            sEntityReplacement = replaceToVectorname(sEntityOccurence);
            _parser.SetVectorVar(sEntityReplacement, vEntityContents);
            mu::CachedDataAccess _access = {sEntityName + "(" + _idx.sCompiledAccessEquation + ")", sEntityReplacement, sEntityName};
            _parser.CacheCurrentAccess(_access);

            parser_ReplaceEntityOccurence(sLine, sEntityOccurence, sEntityName, sEntityReplacement, _idx, _data, _parser, _option);
        }
    }
    while (sLine.find(sEntity, nPos) != string::npos);

    return;
}

void parser_ReplaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement)
{
    size_t nPos = 0;
    while ((nPos = sLine.find(sEntityOccurence)) != string::npos)
    {
        sLine.replace(nPos, sEntityOccurence.length(), sEntityStringReplacement);
    }
}

void parser_ReplaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, Datafile& _data, Parser& _parser, const Settings& _option)
{
    sLine = " " + sLine + " ";

    vector<long long int> vLine;
    vector<long long int> vCol;
    size_t nPos = 0;

    if (_idx.vI.size() && _idx.vJ.size())
    {
        vLine = _idx.vI;
        vCol = _idx.vJ;
    }
    else
    {
        for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
            vLine.push_back(i);
        for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
            vCol.push_back(j);
    }

    while ((nPos = sLine.find(sEntityOccurence)) != string::npos)
    {
        string sLeft = sLine.substr(0, nPos);
        StripSpaces(sLeft);
        if (sLeft.length() < 3 || sLeft.back() != '(')
        {
            sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
            continue;
        }
        else if (sLeft.length() == 3)
        {
            if (sLeft.substr(sLeft.length()-3) == "or(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("or(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.or_func(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else
                sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
        }
        else if (sLeft.length() >= 4)
        {
            if (sLeft.substr(sLeft.length()-4) == "std(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("std(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.std(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "avg(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("avg(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.avg(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "max(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("max(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.max(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "min(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("min(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.min(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "prd(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("prd(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.prd(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "sum(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("sum(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.sum(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "num(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("num(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.num(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "and(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("and(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.and_func(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-3) == "or(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("or(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.or_func(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "cnt(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("cnt(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.cnt(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "med(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("med(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.med(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.length() >= 5 && sLeft.substr(sLeft.length()-5) == "norm(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("norm(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.norm(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "cmp(")
            {
                _parser.DisableAccessCaching();
                double dRef = 0.0;
                int nType = 0;
                string sArg = "";
                sLeft = sLine.substr(sLeft.length()+1, getMatchingParenthesis(sLine.substr(sLeft.length()-1))-2);
                sArg = getNextArgument(sLeft, true);
                sArg = getNextArgument(sLeft, true);
                if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
                    parser_GetDataElement(sArg, _parser, _data, _option);
                _parser.SetExpr(sArg);
                dRef = (double)_parser.Eval();
                sArg = getNextArgument(sLeft, true);
                if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
                    parser_GetDataElement(sArg, _parser, _data, _option);
                _parser.SetExpr(sArg);
                nType = (int)_parser.Eval();
                sLine = sLine.replace(sLine.rfind("cmp(", sLine.find(sEntityOccurence)),
                    getMatchingParenthesis(sLine.substr(sLine.rfind("cmp(", sLine.find(sEntityOccurence))+3))+4,
                    toCmdString(_data.cmp(sEntityName, vLine, vCol, dRef, nType)));
            }
            else if (sLeft.substr(sLeft.length()-4) == "pct(")
            {
                _parser.DisableAccessCaching();
                double dPct = 0.5;
                string sArg = "";
                sLeft = sLine.substr(sLeft.length()+1, getMatchingParenthesis(sLine.substr(sLeft.length()-1))-2);
                sArg = getNextArgument(sLeft, true);
                sArg = getNextArgument(sLeft, true);
                if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
                    parser_GetDataElement(sArg, _parser, _data, _option);
                _parser.SetExpr(sArg);
                dPct = _parser.Eval();
                sLine = sLine.replace(sLine.rfind("pct(", sLine.find(sEntityOccurence)),
                    getMatchingParenthesis(sLine.substr(sLine.rfind("pct(", sLine.find(sEntityOccurence))+3))+4,
                    toCmdString(_data.pct(sEntityName, vLine, vCol, dPct)));
            }
            else
                sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
        }
    }
}
/* --> Diese Funktion teilt den String sToSplit am char cSep auf, wobei oeffnende und schliessende
 *     Klammern beruecksichtigt werden <--
 */
int parser_SplitArgs(string& sToSplit, string& sSecArg, const char& cSep, const Settings& _option, bool bIgnoreSurroundingParenthesis)
{
    int nFinalParenthesis = 0;
    int nParenthesis = 0;
    int nV_Parenthesis = 0;
    int nSep = -1;

    StripSpaces(sToSplit);

    if (!bIgnoreSurroundingParenthesis)
    {
        // --> Suchen wir nach der schliessenden Klammer <--
        for (unsigned int i = 0; i < sToSplit.length(); i++)
        {
            if (sToSplit[i] == '(')
                nParenthesis++;
            if (sToSplit[i] == ')')
                nParenthesis--;
            if (!nParenthesis)
            {
                nFinalParenthesis = i;
                break;
            }
        }

        if (nParenthesis)
        {
            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sToSplit, SyntaxError::invalid_position);
        }
    }
    else
    {
        sToSplit = "(" + sToSplit + ")";
        nFinalParenthesis = sToSplit.length()-1;
    }
    // --> Trennen wir den Rest und die umschliessenden Klammern des Strings ab <--
    sToSplit = sToSplit.substr(1,nFinalParenthesis-1);

    // --> Suchen wir nach dem char cSep <--
    for (unsigned int i = 0; i < sToSplit.length(); i++)
    {
        if (sToSplit[i] == '(')
            nParenthesis++;
        if (sToSplit[i] == ')')
            nParenthesis--;
        if (sToSplit[i] == '{')
        {
            nV_Parenthesis++;
        }
        if (sToSplit[i] == '}')
        {
            nV_Parenthesis--;
        }
        if (sToSplit[i] == cSep && !nParenthesis && !nV_Parenthesis)
        {
            nSep = i;
            break;
        }
    }

    if (nSep == -1)
    {
        throw SyntaxError(SyntaxError::SEPARATOR_NOT_FOUND, sToSplit, SyntaxError::invalid_position);
    }

    // --> Teilen wir nun den string sToSplit in sSecArg und sToSplit auf <--
    sSecArg = sToSplit.substr(nSep+1);
    sToSplit = sToSplit.substr(0,nSep);
    return nFinalParenthesis;
}

// --> Gibt die Anzahl an Gleitkommazahlen zurueck, die in eine Zeile mit der aktuellen Zeilenlaenge passen <--
int parser_LineBreak(const Settings& _option)
{
    /* --> Wir berechnen die Anzahl an Zahlen, die in eine Zeile passen, automatisch <--
     * Links: 11 Zeichen bis [; rechts: vier Zeichen mit EOL;
     * Fuer jede Zahl: 1 Vorzeichen, 1 Dezimalpunkt, 5 Exponentenstellen, Praezision Ziffern, 1 Komma und 1 Leerstelle
     */
    return (_option.getWindow()-1-15) / (_option.getPrecision()+9);

}

// --> Prueft, ob der zweite Index groesser als der erste ist und vertauscht sie ggf. <--
void parser_CheckIndices(int& nIndex_1, int& nIndex_2)
{
    if (nIndex_1 < 0)
        nIndex_1 = 0;
    if (nIndex_2 < nIndex_1)
    {
        int nTemp = nIndex_1;
        nIndex_1 = nIndex_2,
        nIndex_2 = nTemp;
        if (nIndex_1 < 0)
            nIndex_1 = 0;
    }
    return;
}

// --> Prueft, ob der zweite Index groesser als der erste ist und vertauscht sie ggf. <--
void parser_CheckIndices(long long int& nIndex_1, long long int& nIndex_2)
{
    if (nIndex_1 < 0)
        nIndex_1 = 0;
    if (nIndex_2 < nIndex_1)
    {
        long long int nTemp = nIndex_1;
        nIndex_1 = nIndex_2,
        nIndex_2 = nTemp;
        if (nIndex_1 < 0)
            nIndex_1 = 0;
    }
    return;
}


// --> Gibt die Position des naechsten Delimiters zurueck <--
unsigned int parser_getDelimiterPos(const string& sLine)
{
    string sDelimiter = "+-*/ =^&|!<>,\n";
    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        if (sLine[i] == '(' || sLine[i] == '{')
            i += getMatchingParenthesis(sLine.substr(i));
        for (unsigned int j = 0; j < sDelimiter.length(); j++)
        {
            if (sLine[i] == sDelimiter[j])
            {
                return i;
            }
        }
    }
    return -1;
}

// --> Diese Funktion ersetzt den Prompt ("??[default]") durch eine Eingabeaufforderung <--
string parser_Prompt(const string& __sCommand)
{
    string sReturn = "";                // Variable fuer Rueckgabe-String
    string sInput = "";                 // Variable fuer die erwartete Eingabe
    bool bHasDefaultValue = false;      // Boolean; TRUE, wenn der String einen Default-Value hat
    unsigned int nPos = 0;                       // Index-Variable

    if (__sCommand.find("??") == string::npos)    // Wenn's "??" gar nicht gibt, koennen wir sofort zurueck
        return __sCommand;
    sReturn = __sCommand;               // Kopieren wir den Uebergebenen String in sReturn

    // --> do...while-Schleife, so lange "??" im String gefunden wird <--
    do
    {
        /* --> Fuer jeden "??" muessen wir eine Eingabe abfragen, daher muessen
         *     wir zuerst alle Variablen zuruecksetzen <--
         */
        sInput = "";
        bHasDefaultValue = false;

        // --> Speichern der naechsten Position von "??" in nPos <--
        nPos = sReturn.find("??");

        // --> Pruefen wir, ob es die Default-Value-Klammer ("??[DEFAULT]") gibt <--
        if (sReturn.find("[", nPos) != string::npos)
        {
            // --> Es gibt drei moegliche Faelle, wie eine eckige Klammer auftreten kann <--
            if (sReturn.find("??", nPos+2) != string::npos && sReturn.find("[", nPos) < sReturn.find("??", nPos+2))
                bHasDefaultValue = true;
            else if (sReturn.find("??", nPos+2) == string::npos)
                bHasDefaultValue = true;
            else
                bHasDefaultValue = false;
        }

        /* --> Eingabe in einer do...while abfragen. Wenn ein Defaultwert vorhanden ist,
         *     braucht diese Schleife nicht loopen, auch wenn nichts eingegeben wird <--
         */
        do
        {
            string sComp = sReturn.substr(0,nPos);
            // --> Zur Orientierung geben wir den Teil des Strings vor "??" aus <--
            NumeReKernel::printPreFmt("|-\?\?> " + sComp);
            /*if (sReturn[nPos-1] != ' ')
                cerr << " ";*/
            NumeReKernel::getline(sInput);
            StripSpaces(sComp);
            if (sComp.length() && sInput.find(sComp) != string::npos)
                sInput.erase(0,sInput.find(sComp)+sComp.length());
            StripSpaces(sInput);
            //getline(cin, sInput);
        }
        while (!bHasDefaultValue && !sInput.length());

        // --> Eingabe in den String einsetzen <--
        if (bHasDefaultValue && !sInput.length())
        {
            sReturn = sReturn.substr(0, nPos) + sReturn.substr(sReturn.find("[", nPos)+1, sReturn.find("]", nPos)-sReturn.find("[", nPos)-1) + sReturn.substr(sReturn.find("]", nPos)+1);
        }
        else if (bHasDefaultValue && sInput.length())
        {
            sReturn = sReturn.substr(0, nPos) + sInput + sReturn.substr(sReturn.find("]", nPos)+1);
        }
        else
        {
            sReturn = sReturn.substr(0, nPos) + sInput + sReturn.substr(nPos+2);
        }
    }
    while (sReturn.find("??") != string::npos);

    GetAsyncKeyState(VK_ESCAPE);
    //NumeReKernel::GetAsyncCancelState();
    // --> Jetzt enthaelt der String sReturn "??" an keiner Stelle mehr und kann zurueckgegeben werden <--
    return sReturn;
}

// --> Diese Funktion gibt die Adresse einer bekannten Variable zurueck <--
double* parser_GetVarAdress(const string& sVarName, Parser& _parser)
{
    double* VarAdress = 0;
    mu::varmap_type Vars = _parser.GetVar();
    mu::varmap_type::const_iterator item = Vars.begin();

    for (; item != Vars.end(); ++item)
    {
        if (item->first == sVarName)
        {
            VarAdress = item->second;
            break;
        }
    }

    return VarAdress;
}

bool parser_findExtrema(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
    unsigned int nSamples = 21;
    int nOrder = 5;
    double dVal[2];
    double dLeft = 0.0;
    double dRight = 0.0;
    int nMode = 0;
    double* dVar = 0;
    string sExpr = "";
    string sParams = "";
    string sInterval = "";
    string sVar = "";

    if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
    {
        //sErrorToken = "extrema";
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "extrema");
    }

    if (sCmd.find("-set") != string::npos)
    {
        sExpr = sCmd.substr(0,sCmd.find("-set"));
        sParams = sCmd.substr(sCmd.find("-set"));
    }
    else if (sCmd.find("--") != string::npos)
    {
        sExpr = sCmd.substr(0,sCmd.find("--"));
        sParams = sCmd.substr(sCmd.find("--"));
    }
    else if (sCmd.find("data(") == string::npos && !_data.containsCacheElements(sCmd))
        throw SyntaxError(SyntaxError::NO_EXTREMA_OPTIONS, sCmd, SyntaxError::invalid_position);
    else
        sExpr = sCmd;

    StripSpaces(sExpr);
    sExpr = sExpr.substr(findCommand(sExpr).sString.length());

    if (!parser_ExprNotEmpty(sExpr) || !_functions.call(sExpr, _option))
        return false;
    if (!_functions.call(sParams, _option))
        return false;

    StripSpaces(sParams);

    if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
    {
        parser_GetDataElement(sExpr, _parser, _data, _option, false);
    }

    if (sParams.find("data(") != string::npos || _data.containsCacheElements(sParams))
    {
        parser_GetDataElement(sParams, _parser, _data, _option, false);
    }

    if (matchParams(sParams, "min"))
        nMode = -1;
    if (matchParams(sParams, "max"))
        nMode = 1;
    if (matchParams(sParams, "samples", '='))
    {
        _parser.SetExpr(getArgAtPos(sParams, matchParams(sParams, "samples", '=')+7));
        nSamples = (unsigned int)_parser.Eval();
        if (nSamples < 21)
            nSamples = 21;
        sParams.erase(matchParams(sParams, "samples", '=')-1,8);
    }
    if (matchParams(sParams, "points", '='))
    {
        _parser.SetExpr(getArgAtPos(sParams, matchParams(sParams, "points", '=')+6));
        nOrder = (int)_parser.Eval();
        if (nOrder <= 3)
            nOrder = 3;
        sParams.erase(matchParams(sParams, "points", '=')-1,7);
    }

    if (sParams.find('=') != string::npos
        || (sParams.find('[') != string::npos
            && sParams.find(']', sParams.find('['))
            && sParams.find(':', sParams.find('['))))
    {
        if (sParams.substr(0,2) == "--")
            sParams = sParams.substr(2);
        else if (sParams.substr(0,4) == "-set")
            sParams = sParams.substr(4);

        value_type* v = 0;
        Datafile _cache;
        _cache.setCacheStatus(true);
        int nResults = 0;
        if (sParams.find('=') != string::npos)
        {
            int nPos = sParams.find('=');
            sInterval = getArgAtPos(sParams, nPos+1);
            if (sInterval.front() == '[' && sInterval.back() == ']')
            {
                sInterval.pop_back();
                sInterval.erase(0,1);
            }
            sVar = " " + sParams.substr(0,nPos);
            sVar = sVar.substr(sVar.rfind(' '));
            StripSpaces(sVar);
        }
        else
        {
            sVar = "x";
            sInterval = sParams.substr(sParams.find('[')+1, getMatchingParenthesis(sParams.substr(sParams.find('[')))-1);
            StripSpaces(sInterval);
            if (sInterval == ":")
                sInterval = "-10:10";
        }
        _parser.SetExpr(sExpr);
        v = _parser.Eval(nResults);
        if (nResults > 1)
        {
            vector<double> vResults;
            int nResults_x = 0;
            for (int i = 0; i < nResults; i++)
            {
                _cache.writeToCache(i, 1, "cache", v[i]);
            }
            _parser.SetExpr(sInterval);
            v = _parser.Eval(nResults_x);
            if (nResults_x > 1)
            {
                for (int i = 0; i < nResults; i++)
                {
                    if (i >= nResults_x)
                    {
                        _cache.writeToCache(i, 0, "cache", 0.0);
                    }
                    else
                    {
                        _cache.writeToCache(i, 0, "cache", v[i]);
                    }
                }
            }
            else
                return false;
            //cerr << nResults << " " << nResults_x << " " << _cache.getLines("cache", false) << endl;
            sCmd = "cache -sort cols=1[2]";
            _cache.sortElements(sCmd);

            double dMedian = 0.0, dExtremum = 0.0;
            double* data = new double[nOrder];
            int nDir = 0;
            int nanShift = 0;
            if (nOrder >= nResults/3)
                nOrder = nResults/3;
            if (nOrder < 3)
            {
                vResults.push_back(NAN);
                return false;
            }
            for (int i = 0; i+nanShift < _cache.getLines("cache", true); i++)
            {
                if (i == nOrder)
                    break;
                while (isnan(_cache.getElement(i+nanShift, 1, "cache")) && i+nanShift < _cache.getLines("cache", true)-1)
                    nanShift++;
                data[i] = _cache.getElement(i+nanShift, 1, "cache");
            }
            gsl_sort(data, 1, nOrder);
            dExtremum = gsl_stats_median_from_sorted_data(data, 1, nOrder);
            //cerr << dExtremum << endl;
            //for (int i = 1; i < nResults-1; i++)
            for (int i = nOrder; i+nanShift < _cache.getLines("cache", false)-nOrder; i++)
            {
                int currNanShift = 0;
                dMedian = 0.0;
                for (int j = i; j < i+nOrder; j++)
                {
                    while (isnan(_cache.getElement(j+nanShift+currNanShift, 1, "cache")) && j+nanShift+currNanShift < _cache.getLines("cache", true)-1)
                        currNanShift++;
                    data[j-i] = _cache.getElement(j+nanShift+currNanShift, 1, "cache");
                }
                gsl_sort(data, 1, nOrder);
                dMedian = gsl_stats_median_from_sorted_data(data, 1, nOrder);
                //cerr << dMedian << endl;
                if (!nDir)
                {
                    if (dMedian > dExtremum)
                    {
                        nDir = 1;
                    }
                    else if (dMedian < dExtremum)
                    {
                        nDir = -1;
                    }
                    dExtremum = dMedian;
                }
                else
                {
                    if (nDir == 1)
                    {
                        if (dMedian < dExtremum)
                        {
                            if (!nMode || nMode == 1)
                            {
                                int nExtremum = i;
                                double dExtremum = _cache.getElement(i+nanShift, 1, "cache");
                                for (long long int k = i+nanShift; k >= 0; k--)
                                {
                                    if (k == i-nOrder)
                                        break;
                                    if (_cache.getElement(k, 1, "cache") > dExtremum)
                                    {
                                        nExtremum = k;
                                        dExtremum = _cache.getElement(k, 1, "cache");
                                    }
                                }
                                vResults.push_back(_cache.getElement(nExtremum, 0, "cache"));
                                i = nExtremum+nOrder;
                            }
                            nDir = 0;
                        }
                        dExtremum = dMedian;
                    }
                    else
                    {
                        if (dMedian > dExtremum)
                        {
                            if (!nMode || nMode == -1)
                            {
                                int nExtremum = i+nanShift;
                                double dExtremum = _cache.getElement(i, 1, "cache");
                                for (long long int k = i+nanShift; k >= 0; k--)
                                {
                                    if (k == i-nOrder)
                                        break;
                                    if (_cache.getElement(k, 1, "cache") < dExtremum)
                                    {
                                        nExtremum = k;
                                        dExtremum = _cache.getElement(k, 1, "cache");
                                    }
                                }
                                vResults.push_back(_cache.getElement(nExtremum, 0, "cache"));
                                i = nExtremum+nOrder;
                            }
                            nDir = 0;
                        }
                        dExtremum = dMedian;
                    }
                }
                nanShift += currNanShift;
            }
            if (!vResults.size())
                vResults.push_back(NAN);
            delete[] data;
            sCmd = "extrema[~_~]";
            _parser.SetVectorVar("extrema[~_~]", vResults);
            return true;
        }
        else
        {
            if (!parser_CheckVarOccurence(_parser, sVar))
            {
                sCmd = toSystemCodePage("\"Bezüglich der Variablen " + sVar + " ist der Ausdruck konstant und besitzt keine Extrema!\"");
                return true;
            }
            dVar = parser_GetVarAdress(sVar, _parser);
            if (!dVar)
            {
                throw SyntaxError(SyntaxError::EXTREMA_VAR_NOT_FOUND, sCmd, sVar, sVar);
            }
            if (sInterval.find(':') == string::npos || sInterval.length() < 3)
                return false;
            if (parser_ExprNotEmpty(sInterval.substr(0,sInterval.find(':'))))
            {
                _parser.SetExpr(sInterval.substr(0,sInterval.find(':')));
                dLeft = _parser.Eval();
                if (isinf(dLeft) || isnan(dLeft))
                {
                    sCmd = "nan";
                    return false;
                }
            }
            else
                return false;
            if (parser_ExprNotEmpty(sInterval.substr(sInterval.find(':')+1)))
            {
                _parser.SetExpr(sInterval.substr(sInterval.find(':')+1));
                dRight = _parser.Eval();
                if (isinf(dRight) || isnan(dRight))
                {
                    sCmd = "nan";
                    return false;
                }
            }
            else
                return false;
            if (dRight < dLeft)
            {
                double Temp = dRight;
                dRight = dLeft;
                dLeft = Temp;
            }
        }
    }
    else if (sCmd.find("data(") != string::npos || _data.containsCacheElements(sCmd))
    {
        value_type* v;
        int nResults = 0;
        _parser.SetExpr(sExpr);
        v = _parser.Eval(nResults);
        if (nResults > 1)
        {
            if (nOrder >= nResults/3)
                nOrder = nResults/3;

            double dMedian = 0.0, dExtremum = 0.0;
            double* data = 0;
            data = new double[nOrder];
            int nDir = 0;
            int nanShift = 0;
            vector<double> vResults;
            if (nOrder < 3)
            {
                vResults.push_back(NAN);
                return false;
            }
            for (int i = 0; i+nanShift < nResults; i++)
            {
                if (i == nOrder)
                    break;
                while (isnan(v[i+nanShift]) && i+nanShift < nResults-1)
                    nanShift++;
                data[i] = v[i+nanShift];
            }
            gsl_sort(data, 1, nOrder);
            dExtremum = gsl_stats_median_from_sorted_data(data, 1, nOrder);
            //cerr << dExtremum << endl;
            //for (int i = 1; i < nResults-1; i++)
            for (int i = nOrder; i+nanShift < nResults-nOrder; i++)
            {
                int currNanShift = 0;
                dMedian = 0.0;
                for (int j = i; j < i+nOrder; j++)
                {
                    while (isnan(v[j+nanShift+currNanShift]) && j+nanShift+currNanShift < nResults-1)
                        currNanShift++;
                    data[j-i] = v[j+nanShift+currNanShift];
                }
                gsl_sort(data, 1, nOrder);
                dMedian = gsl_stats_median_from_sorted_data(data, 1, nOrder);
                //cerr << dMedian << endl;
                if (!nDir)
                {
                    if (dMedian > dExtremum)
                    {
                        nDir = 1;
                    }
                    else if (dMedian < dExtremum)
                    {
                        nDir = -1;
                    }
                    dExtremum = dMedian;
                }
                else
                {
                    if (nDir == 1)
                    {
                        if (dMedian < dExtremum)
                        {
                            if (!nMode || nMode == 1)
                            {
                                int nExtremum = i+nanShift;
                                double dExtremum = v[i+nanShift];
                                for (long long int k = i+nanShift; k >= 0; k--)
                                {
                                    if (k == i-nOrder)
                                        break;
                                    if (v[k] > dExtremum)
                                    {
                                        nExtremum = k;
                                        dExtremum = v[k];
                                    }
                                }
                                vResults.push_back(nExtremum+1);
                                //cerr << i-nExtremum << endl;
                                i = nExtremum + nOrder;
                            }
                            nDir = 0;
                        }
                        dExtremum = dMedian;
                    }
                    else
                    {
                        if (dMedian > dExtremum)
                        {
                            if (!nMode || nMode == -1)
                            {
                                int nExtremum = i+nanShift;
                                double dExtremum = v[i+nanShift];
                                for (long long int k = i+nanShift; k >= 0; k--)
                                {
                                    if (k == i-nOrder)
                                        break;
                                    if (v[k] < dExtremum)
                                    {
                                        nExtremum = k;
                                        dExtremum = v[k];
                                    }
                                }
                                vResults.push_back(nExtremum+1);
                                //cerr << i-nExtremum << endl;
                                i = nExtremum + nOrder;
                            }
                            nDir = 0;
                        }
                        dExtremum = dMedian;
                    }
                }
                nanShift += currNanShift;
            }
            if (data)
                delete[] data;
            if (!vResults.size())
                vResults.push_back(NAN);
            sCmd = "extrema[~_~]";
            _parser.SetVectorVar("extrema[~_~]", vResults);
            return true;
        }
        else
            throw SyntaxError(SyntaxError::NO_EXTREMA_VAR, sCmd, SyntaxError::invalid_position);
    }
    else
        throw SyntaxError(SyntaxError::NO_EXTREMA_VAR, sCmd, SyntaxError::invalid_position);

    if ((int)(dRight-dLeft))
    {
        nSamples = (nSamples-1)*(int)(dRight - dLeft) + 1;
    }
    if (nSamples > 10001)
        nSamples = 10001;

    _parser.SetExpr(sExpr);
    _parser.Eval();
    sCmd = "";
    vector<double> vResults;
    dVal[0] = _parser.Diff(dVar, dLeft,1e-7);
    for (unsigned int i = 1; i < nSamples; i++)
    {
        dVal[1] = _parser.Diff(dVar, dLeft+i*(dRight-dLeft)/(double)(nSamples-1),1e-7);
        if (dVal[0]*dVal[1] < 0)
        {
            if (!nMode
                || (nMode == 1 && (dVal[0] > 0 && dVal[1] < 0))
                || (nMode == -1 && (dVal[0] < 0 && dVal[1] > 0)))
            {
                vResults.push_back(parser_LocalizeExtremum(sExpr, dVar, _parser, _option, dLeft+(i-1)*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1)));
                /*if (sCmd.length())
                    sCmd += ", ";
                sCmd += toCmdString(parser_LocalizeMin(sExpr, dVar, _parser, _option, dLeft+(i-1)*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1)));*/
            }
        }
        else if (dVal[0]*dVal[1] == 0.0)
        {
            if (!nMode
                || (nMode == 1 && (dVal[0] > 0 || dVal[1] < 0))
                || (nMode == -1 && (dVal[0] < 0 || dVal[1] > 0)))
            {
                int nTemp = i-1;
                if (dVal[0] != 0.0)
                {
                    while (dVal[0]*dVal[1] == 0.0 && i+1 < nSamples)
                    {
                        i++;
                        dVal[1] = _parser.Diff(dVar, dLeft+i*(dRight-dLeft)/(double)(nSamples-1),1e-7);
                    }
                }
                else
                {
                    while (dVal[1] == 0.0 && i+1 < nSamples)
                    {
                        i++;
                        dVal[1] = _parser.Diff(dVar, dLeft+i*(dRight-dLeft)/(double)(nSamples-1), 1e-7);
                    }
                }
                vResults.push_back(parser_LocalizeExtremum(sExpr, dVar, _parser, _option, dLeft+nTemp*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1)));
                /*if (sCmd.length())
                    sCmd += ", ";
                sCmd += toCmdString(parser_LocalizeMin(sExpr, dVar, _parser, _option, dLeft+nTemp*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1)));*/
            }
        }
        dVal[0] = dVal[1];
    }

    if (!sCmd.length() && !vResults.size())
    {
        dVal[0] = _parser.Diff(dVar, dLeft);
        dVal[1] = _parser.Diff(dVar, dRight);
        if (dVal[0]
            && (!nMode
                || (dVal[0] < 0 && nMode == 1)
                || (dVal[0] > 0 && nMode == -1)))
            sCmd = toString(dLeft, _option);
        if (dVal[1]
            && (!nMode
                || (dVal[1] < 0 && nMode == -1)
                || (dVal[1] > 0 && nMode == 1)))
        {
            if (sCmd.length())
                sCmd += ", ";
            sCmd += toString(dRight, _option);
        }
        if (!dVal[0] && ! dVal[1])
            sCmd = "nan";//"\"Kein Extremum gefunden!\"";
    }
    else
    {
        sCmd = "extrema[~_~]";
        _parser.SetVectorVar("extrema[~_~]", vResults);
    }
    return true;
}

bool parser_findZeroes(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
    unsigned int nSamples = 21;
    double dVal[2];
    double dLeft = 0.0;
    double dRight = 0.0;
    int nMode = 0;
    double* dVar = 0;
    double dTemp = 0.0;
    string sExpr = "";
    string sParams = "";
    string sInterval = "";
    string sVar = "";

    if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
    {
        //sErrorToken = "zeroes";
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "zeroes");
    }

    if (sCmd.find("-set") != string::npos)
    {
        sExpr = sCmd.substr(0,sCmd.find("-set"));
        sParams = sCmd.substr(sCmd.find("-set"));
    }
    else if (sCmd.find("--") != string::npos)
    {
        sExpr = sCmd.substr(0,sCmd.find("--"));
        sParams = sCmd.substr(sCmd.find("--"));
    }
    else if (sCmd.find("data(") == string::npos && !_data.containsCacheElements(sCmd))
        throw SyntaxError(SyntaxError::NO_ZEROES_OPTIONS, sCmd, SyntaxError::invalid_position);
    else
        sExpr = sCmd;

    StripSpaces(sExpr);
    sExpr = sExpr.substr(findCommand(sExpr).sString.length());

    if (!parser_ExprNotEmpty(sExpr) || !_functions.call(sExpr, _option))
        return false;
    if (!_functions.call(sParams, _option))
        return false;

    StripSpaces(sParams);

    if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
    {
        parser_GetDataElement(sExpr, _parser, _data, _option, false);
    }

    if (sParams.find("data(") != string::npos || _data.containsCacheElements(sParams))
    {
        parser_GetDataElement(sParams, _parser, _data, _option, false);
    }

    if (matchParams(sParams, "min") || matchParams(sParams, "down"))
        nMode = -1;
    if (matchParams(sParams, "max") || matchParams(sParams, "up"))
        nMode = 1;
    if (matchParams(sParams, "samples", '='))
    {
        _parser.SetExpr(getArgAtPos(sParams, matchParams(sParams, "samples", '=')+7));
        nSamples = (int)_parser.Eval();
        if (nSamples < 21)
            nSamples = 21;
        sParams.erase(matchParams(sParams, "samples", '=')-1,8);
    }

    if (sParams.find('=') != string::npos
        || (sParams.find('[') != string::npos
            && sParams.find(']', sParams.find('['))
            && sParams.find(':', sParams.find('['))))
    {
        if (sParams.substr(0,2) == "--")
            sParams = sParams.substr(2);
        else if (sParams.substr(0,4) == "-set")
            sParams = sParams.substr(4);

        value_type* v = 0;
        Datafile _cache;
        _cache.setCacheStatus(true);
        int nResults = 0;
        if (sParams.find('=') != string::npos)
        {
            int nPos = sParams.find('=');
            sInterval = getArgAtPos(sParams, nPos+1);
            if (sInterval.front() == '[' && sInterval.back() == ']')
            {
                sInterval.pop_back();
                sInterval.erase(0,1);
            }
            sVar = " " + sParams.substr(0,nPos);
            sVar = sVar.substr(sVar.rfind(' '));
            StripSpaces(sVar);
        }
        else
        {
            sVar = "x";
            sInterval = sParams.substr(sParams.find('[')+1, getMatchingParenthesis(sParams.substr(sParams.find('[')))-1);
            StripSpaces(sInterval);
            if (sInterval == ":")
                sInterval = "-10:10";
        }
        _parser.SetExpr(sExpr);
        v = _parser.Eval(nResults);
        if (nResults > 1)
        {
            vector<double> vResults;
            int nResults_x = 0;
            for (int i = 0; i < nResults; i++)
            {
                _cache.writeToCache(i, 1, "cache", v[i]);
            }
            _parser.SetExpr(sInterval);
            v = _parser.Eval(nResults_x);
            if (nResults_x > 1)
            {
                for (int i = 0; i < nResults; i++)
                {
                    if (i >= nResults_x)
                    {
                        _cache.writeToCache(i, 0, "cache", 0.0);
                    }
                    else
                    {
                        _cache.writeToCache(i, 0, "cache", v[i]);
                    }
                }
            }
            else
                return false;
            //cerr << nResults << " " << nResults_x << " " << _cache.getLines("cache", false) << endl;
            sCmd = "cache -sort cols=1[2]";
            _cache.sortElements(sCmd);

            for (long long int i = 1; i < _cache.getLines("cache", false); i++)
            {
                if (isnan(_cache.getElement(i-1,1,"cache")))
                    continue;
                if (!nMode && _cache.getElement(i,1,"cache")*_cache.getElement(i-1,1,"cache") <= 0.0)
                {
                    if (_cache.getElement(i,1,"cache") == 0.0)
                    {
                        vResults.push_back(_cache.getElement(i,0,"cache"));
                        i++;
                    }
                    else if (_cache.getElement(i-1,1,"cache") == 0.0)
                        vResults.push_back(_cache.getElement(i-1,0,"cache"));
                    else if (_cache.getElement(i,1,"cache")*_cache.getElement(i-1,1,"cache") < 0.0)
                        vResults.push_back(Linearize(_cache.getElement(i-1,0,"cache"), _cache.getElement(i-1,1,"cache"), _cache.getElement(i,0,"cache"), _cache.getElement(i,1,"cache")));
                }
                else if (nMode && _cache.getElement(i,1,"cache")*_cache.getElement(i-1,1,"cache") <= 0.0)
                {
                    if (_cache.getElement(i,1,"cache") == 0.0 && _cache.getElement(i-1,1,"cache") == 0.0)
                    {
                        for (long long int j = i+1; j < _cache.getLines("cache", false); j++)
                        {
                            if (nMode * _cache.getElement(j,1,"cache") > 0.0)
                            {
                                for (long long int k = i-1; k <= j; k++)
                                    vResults.push_back(_cache.getElement(k,0,"cache"));
                                break;
                            }
                            else if (nMode * _cache.getElement(j,1,"cache") < 0.0)
                                break;
                            if (j+1 == _cache.getLines("cache", false) && i > 1 && nMode*_data.getElement(i-2,1,"cache") < 0.0)
                            {
                                for (long long int k = i-1; k <= j; k++)
                                    vResults.push_back(_cache.getElement(k,0,"cache"));
                                break;
                            }
                        }
                        continue;
                    }
                    else if (_cache.getElement(i,1,"cache") == 0.0 && nMode * _cache.getElement(i-1,1,"cache") < 0.0)
                        vResults.push_back(_cache.getElement(i,0,"cache"));
                    else if (_cache.getElement(i-1,1,"cache") == 0.0 && nMode * _cache.getElement(i,1,"cache") > 0.0)
                        vResults.push_back(_cache.getElement(i-1,0,"cache"));
                    else if (_cache.getElement(i,1,"cache")*_cache.getElement(i-1,1,"cache") < 0.0 && nMode*_cache.getElement(i-1,1,"cache") < 0.0)
                        vResults.push_back(Linearize(_cache.getElement(i-1,0,"cache"), _cache.getElement(i-1,1,"cache"), _cache.getElement(i,0,"cache"), _cache.getElement(i,1,"cache")));
                }
            }
            if (!vResults.size())
                vResults.push_back(NAN);
            sCmd = "zeroes[~_~]";
            _parser.SetVectorVar("zeroes[~_~]", vResults);
            return true;
        }
        else
        {
            if (!parser_CheckVarOccurence(_parser, sVar))
            {
                if (!_parser.Eval())
                    sCmd = "\"Der Ausdruck ist auf dem gesamten Intervall identisch Null!\"";
                else
                    sCmd = toSystemCodePage("\"Bezüglich der Variablen " + sVar + " ist der Ausdruck konstant und besitzt keine Nullstellen!\"");
                return true;
            }
            dVar = parser_GetVarAdress(sVar, _parser);
            if (!dVar)
            {
                throw SyntaxError(SyntaxError::ZEROES_VAR_NOT_FOUND, sCmd, sVar, sVar);
            }
            if (sInterval.find(':') == string::npos || sInterval.length() < 3)
                return false;
            if (parser_ExprNotEmpty(sInterval.substr(0,sInterval.find(':'))))
            {
                _parser.SetExpr(sInterval.substr(0,sInterval.find(':')));
                dLeft = _parser.Eval();
                if (isinf(dLeft) || isnan(dLeft))
                {
                    sCmd = "nan";
                    return false;
                }
            }
            else
                return false;
            if (parser_ExprNotEmpty(sInterval.substr(sInterval.find(':')+1)))
            {
                _parser.SetExpr(sInterval.substr(sInterval.find(':')+1));
                dRight = _parser.Eval();
                if (isinf(dRight) || isnan(dRight))
                {
                    sCmd = "nan";
                    return false;
                }
            }
            else
                return false;
            if (dRight < dLeft)
            {
                double Temp = dRight;
                dRight = dLeft;
                dLeft = Temp;
            }
        }
    }
    else if (sCmd.find("data(") != string::npos || _data.containsCacheElements(sCmd))
    {
        value_type* v;
        int nResults = 0;
        _parser.SetExpr(sExpr);
        v = _parser.Eval(nResults);
        if (nResults > 1)
        {
            vector<double> vResults;
            for (int i = 1; i < nResults; i++)
            {
                if (isnan(v[i-1]))
                    continue;
                if (!nMode && v[i]*v[i-1] <= 0.0)
                {
                    if (v[i] == 0.0)
                    {
                        vResults.push_back((double)i+1);
                        i++;
                    }
                    else if (v[i-1] == 0.0)
                        vResults.push_back((double)i);
                    else if (fabs(v[i]) <= fabs(v[i-1]))
                        vResults.push_back((double)i+1);
                    else
                        vResults.push_back((double)i);
                }
                else if (nMode && v[i]*v[i-1] <= 0.0)
                {
                    if (v[i] == 0.0 && v[i-1] == 0.0)
                    {
                        for (int j = i+1; j < nResults; j++)
                        {
                            if (nMode * v[j] > 0.0)
                            {
                                for (int k = i-1; k <= j; k++)
                                    vResults.push_back(k);
                                break;
                            }
                            else if (nMode * v[j] < 0.0)
                                break;
                            if (j+1 == nResults && i > 2 && nMode * v[i-2] < 0.0)
                            {
                                for (int k = i-1; k <= j; k++)
                                    vResults.push_back(k);
                                break;
                            }
                        }
                        continue;
                    }
                    else if (v[i] == 0.0 && nMode*v[i-1] < 0.0)
                        vResults.push_back((double)i+1);
                    else if (v[i-1] == 0.0 && nMode*v[i] > 0.0)
                        vResults.push_back((double)i);
                    else if (fabs(v[i]) <= fabs(v[i-1]) && nMode*v[i-1] < 0.0)
                        vResults.push_back((double)i+1);
                    else if (nMode*v[i-1] < 0.0)
                        vResults.push_back((double)i);
                }
            }
            if (!vResults.size())
                vResults.push_back(NAN);
            sCmd = "zeroes[~_~]";
            _parser.SetVectorVar("zeroes[~_~]", vResults);
            return true;
        }
        else
            throw SyntaxError(SyntaxError::NO_ZEROES_VAR, sCmd, SyntaxError::invalid_position);
    }
    else
        throw SyntaxError(SyntaxError::NO_ZEROES_VAR, sCmd, SyntaxError::invalid_position);

    if ((int)(dRight-dLeft))
    {
        nSamples = (nSamples-1)*(int)(dRight - dLeft) + 1;
    }
    if (nSamples > 10001)
        nSamples = 10001;

    _parser.SetExpr(sExpr);
    _parser.Eval();
    sCmd = "";
    dTemp = *dVar;

    *dVar = dLeft;
    vector<double> vResults;
    dVal[0] = _parser.Eval();
    if (dVal[0] != 0.0 && fabs(dVal[0]) < 1e-10)
    {
        *dVar = dLeft - 1e-10;
        dVal[1] = _parser.Eval();
        if (dVal[0]*dVal[1] < 0 && (nMode*dVal[0] <= 0.0))
        {
            vResults.push_back(parser_LocalizeExtremum(sExpr, dVar, _parser, _option, dLeft-1e-10, dLeft));
        }
    }
    for (unsigned int i = 1; i < nSamples; i++)
    {
        *dVar = dLeft + i*(dRight-dLeft)/(double)(nSamples-1);
        dVal[1] = _parser.Eval();
        if (dVal[0]*dVal[1] < 0)
        {
            if (!nMode
                || (nMode == -1 && (dVal[0] > 0 && dVal[1] < 0))
                || (nMode == 1 && (dVal[0] < 0 && dVal[1] > 0)))
            {
                vResults.push_back((parser_LocalizeZero(sExpr, dVar, _parser, _option, dLeft+(i-1)*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1))));
            }
        }
        else if (dVal[0]*dVal[1] == 0.0)
        {
            if (!nMode
                || (nMode == -1 && (dVal[0] > 0 || dVal[1] < 0))
                || (nMode == 1 && (dVal[0] < 0 || dVal[1] > 0)))
            {
                int nTemp = i-1;
                if (dVal[0] != 0.0)
                {
                    while (dVal[0]*dVal[1] == 0.0 && i+1 < nSamples)
                    {
                        i++;
                        *dVar = dLeft+i*(dRight-dLeft)/(double)(nSamples-1);
                        dVal[1] = _parser.Eval();
                    }
                }
                else
                {
                    while (dVal[1] == 0.0 && i+1 < nSamples)
                    {
                        i++;
                        *dVar = dLeft+i*(dRight-dLeft)/(double)(nSamples-1);
                        dVal[1] = _parser.Eval();
                    }
                }
                vResults.push_back(parser_LocalizeZero(sExpr, dVar, _parser, _option, dLeft+nTemp*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1)));
            }
        }
        dVal[0] = dVal[1];
    }
    if (dVal[0] != 0.0 && fabs(dVal[0]) < 1e-10)
    {
        *dVar = dRight+1e-10;
        dVal[1] = _parser.Eval();
        if (dVal[0]*dVal[1] < 0 && nMode*dVal[0] <= 0.0)
        {
            vResults.push_back(parser_LocalizeZero(sExpr, dVar, _parser, _option, dRight, dRight+1e-10));
        }
    }

    *dVar = dTemp;

    if (!sCmd.length() && !vResults.size())
    {
        sCmd = "nan";//"\"Keine Nullstelle gefunden!\"";
    }
    else
    {
        sCmd = "zeroes[~_~]";
        _parser.SetVectorVar("zeroes[~_~]", vResults);
        //sCmd = "{{" + sCmd + "}}";
    }
    return true;
}

double parser_LocalizeExtremum(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps, int nRecursion)
{
    const unsigned int nSamples = 101;
    double dVal[2];

    if (_parser.GetExpr() != sCmd)
    {
        _parser.SetExpr(sCmd);
        _parser.Eval();
    }

    dVal[0] = _parser.Diff(dVarAdress, dLeft, 1e-7);
    for (unsigned int i = 1; i < nSamples; i++)
    {
        dVal[1] = _parser.Diff(dVarAdress, dLeft + i*(dRight-dLeft)/(double)(nSamples-1), 1e-7);
        if (dVal[0]*dVal[1] < 0)
        {
            if ((dRight - dLeft)/(double)(nSamples-1) <= dEps || fabs(log(dEps))+1 < nRecursion*2)
            {
                return dLeft + (i-1)*(dRight-dLeft)/(double)(nSamples-1) + Linearize(0.0, dVal[0], (dRight-dLeft)/(double)(nSamples-1), dVal[1]);
                //return dLeft + (i+0.5)*(dRight - dLeft)/(double)(nSamples-1);
            }
            else
                return parser_LocalizeExtremum(sCmd, dVarAdress, _parser, _option, dLeft+(i-1)*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1), dEps, nRecursion+1);
        }
        else if (dVal[0]*dVal[1] == 0.0)
        {
            int nTemp = i-1;
            if (dVal[0] != 0.0)
            {
                while (dVal[0]*dVal[1] == 0.0 && i+1 < nSamples)
                {
                    i++;
                    dVal[1] = _parser.Diff(dVarAdress, dLeft + i*(dRight-dLeft)/(double)(nSamples-1), 1e-7);
                }
            }
            else
            {
                while (dVal[1] == 0.0 && i+1 < nSamples)
                {
                    i++;
                    dVal[1] = _parser.Diff(dVarAdress, dLeft + i*(dRight-dLeft)/(double)(nSamples-1),1e-7);
                }
            }
            if ((i-nTemp)*(dRight - dLeft)/(double)(nSamples-1) <= dEps || (!nTemp && i+1 == nSamples) || fabs(log(dEps))+1 < nRecursion*2)
            {
                return dLeft + nTemp*(dRight-dLeft)/(double)(nSamples-1) + Linearize(0.0, dVal[0], (i-nTemp)*(dRight-dLeft)/(double)(nSamples-1), dVal[1]);
                //return dLeft + (i+nTemp)*(dRight-dLeft)/(double)(nSamples-1)/2.0;
            }
            else
                return parser_LocalizeExtremum(sCmd, dVarAdress, _parser, _option, dLeft + nTemp*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1), dEps, nRecursion+1);
        }
        dVal[0] = dVal[1];
    }

    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval();
    *dVarAdress = dRight;
    dVal[1] = _parser.Eval();
    return Linearize(dLeft, dVal[0], dRight, dVal[1]);
}

double parser_LocalizeZero(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps, int nRecursion)
{
    const unsigned int nSamples = 101;
    double dVal[2];

    if (_parser.GetExpr() != sCmd)
    {
        _parser.SetExpr(sCmd);
        _parser.Eval();
    }

    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval();
    for (unsigned int i = 1; i < nSamples; i++)
    {
        *dVarAdress = dLeft + i*(dRight-dLeft)/(double)(nSamples-1);
        dVal[1] = _parser.Eval();
        if (dVal[0]*dVal[1] < 0)
        {
            if ((dRight - dLeft)/(double)(nSamples-1) <= dEps || fabs(log(dEps))+1 < nRecursion*2)
            {
                return dLeft + (i-1)*(dRight-dLeft)/(double)(nSamples-1) + Linearize(0.0, dVal[0], (dRight-dLeft)/(double)(nSamples-1), dVal[1]);
            }
            else
                return parser_LocalizeZero(sCmd, dVarAdress, _parser, _option, dLeft+(i-1)*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1), dEps, nRecursion+1);
        }
        else if (dVal[0]*dVal[1] == 0.0)
        {
            int nTemp = i-1;
            if (dVal[0] != 0.0)
            {
                while (dVal[0]*dVal[1] == 0.0 && i+1 < nSamples)
                {
                    i++;
                    *dVarAdress = dLeft + i*(dRight-dLeft)/(double)(nSamples-1);
                    dVal[1] = _parser.Eval();
                }
            }
            else
            {
                while (dVal[1] == 0.0 && i+1 < nSamples)
                {
                    i++;
                    *dVarAdress = dLeft + i*(dRight-dLeft)/(double)(nSamples-1);
                    dVal[1] = _parser.Eval();
                }
            }
            if ((i-nTemp)*(dRight - dLeft)/(double)(nSamples-1) <= dEps || (!nTemp && i+1 == nSamples) || fabs(log(dEps))+1 < nRecursion*2)
            {
                return dLeft + nTemp*(dRight-dLeft)/(double)(nSamples-1) + Linearize(0.0, dVal[0], (i-nTemp)*(dRight-dLeft)/(double)(nSamples-1), dVal[1]);
            }
            else
                return parser_LocalizeZero(sCmd, dVarAdress, _parser, _option, dLeft + nTemp*(dRight-dLeft)/(double)(nSamples-1), dLeft+i*(dRight-dLeft)/(double)(nSamples-1), dEps, nRecursion+1);
        }
        dVal[0] = dVal[1];
    }

    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval();
    *dVarAdress = dRight;
    dVal[1] = _parser.Eval();
    return Linearize(dLeft, dVal[0], dRight, dVal[1]);
}

// --> taylor FUNCTION -set VAR=WERT n=ORDNUNG unique <--
void parser_Taylor(string& sCmd, Parser& _parser, const Settings& _option, Define& _functions)
{
    string sParams = "";
    string sVarName = "";
    string sExpr = "";
    string sExpr_cpy = "";
    string sArg = "";
    string sTaylor = "Taylor";
    string sPolynom = "";
    bool bUseUniqueName = false;
    unsigned int nth_taylor = 6;
    unsigned int nSamples = 0;
    unsigned int nMiddle = 0;
    double* dVar = 0;
    double dVarValue = 0.0;
    long double** dDiffValues = 0;

    if (containsStrings(sCmd))
    {
        //sErrorToken = "taylor";
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "taylor");
    }

    if (sCmd.find("-set") != string::npos)
    {
        sParams = sCmd.substr(sCmd.find("-set"));
    }
    else if (sCmd.find("--") != string::npos)
    {
        sParams = sCmd.substr(sCmd.find("--"));
    }
    else
    {
        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_TAYLOR_MISSINGPARAMS"), _option));
        return;
    }

    if (matchParams(sParams, "n", '='))
    {
        _parser.SetExpr(sParams.substr(matchParams(sParams, "n", '=')+1, sParams.find(' ', matchParams(sParams, "n", '=')+1)-matchParams(sParams, "n", '=')-1));
        nth_taylor = (unsigned int)_parser.Eval();
        if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
            nth_taylor = 6;
        sParams = sParams.substr(0,matchParams(sParams, "n", '=')-1) + sParams.substr(matchParams(sParams, "n", '=')-1+_parser.GetExpr().length());
    }
    if (matchParams(sParams, "unique") || matchParams(sParams, "u"))
        bUseUniqueName = true;
    if (sParams.find('=') == string::npos)
        return;
    else
    {
        if (sParams.substr(0,2) == "-s")
            sParams = sParams.substr(4);
        else
            sParams = sParams.substr(2);
        sVarName = sParams.substr(0,sParams.find('='));
        StripSpaces(sVarName);

        _parser.SetExpr(sParams.substr(sParams.find('=')+1,sParams.find(' ', sParams.find('='))-sParams.find('=')-1));
        dVarValue = _parser.Eval();
        if (isinf(dVarValue) || isnan(dVarValue))
        {
            sCmd = "nan";
            return;
        }
        if (!dVarValue)
            sArg = "*x^";
        else if (dVarValue < 0)
            sArg = "*(x+" + toString(-dVarValue, _option.getPrecision()) + ")^";
        else
            sArg = "*(x-" + toString(dVarValue, _option.getPrecision()) + ")^";
    }
    sExpr = sCmd.substr(sCmd.find(' ')+1);
    if (sExpr.find("-set") != string::npos)
        sExpr = sExpr.substr(0, sExpr.find("-set"));
    else
        sExpr = sExpr.substr(0, sExpr.find("--"));

    StripSpaces(sExpr);
    sExpr_cpy = sExpr;
    if (bUseUniqueName)
        sTaylor += toString((int)nth_taylor) + "_" + sExpr;
    if (!_functions.call(sExpr, _option))
        return;
    StripSpaces(sExpr);
    _parser.SetExpr(sExpr);
    if (!parser_CheckVarOccurence(_parser, sVarName))
    {
        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_TAYLOR_CONSTEXPR", sVarName), _option));
        return;
    }
    if (sVarName.length())
        dVar = parser_GetVarAdress(sVarName, _parser);
    if (!dVar)
        return;

    if (bUseUniqueName)
    {
        for (unsigned int i = 0; i < sTaylor.length(); i++)
        {
            if (sTaylor[i] == ' '
                || sTaylor[i] == ','
                || sTaylor[i] == ';'
                || sTaylor[i] == '-'
                || sTaylor[i] == '*'
                || sTaylor[i] == '/'
                || sTaylor[i] == '%'
                || sTaylor[i] == '^'
                || sTaylor[i] == '!'
                || sTaylor[i] == '<'
                || sTaylor[i] == '>'
                || sTaylor[i] == '&'
                || sTaylor[i] == '|'
                || sTaylor[i] == '?'
                || sTaylor[i] == ':'
                || sTaylor[i] == '='
                || sTaylor[i] == '+'
                || sTaylor[i] == '['
                || sTaylor[i] == ']'
                || sTaylor[i] == '{'
                || sTaylor[i] == '}'
                || sTaylor[i] == '('
                || sTaylor[i] == ')')
            {
                sTaylor.erase(i,1);
                i--;
            }
        }
    }

    sTaylor += "(x) := ";

    if (!nth_taylor)
    {
        *dVar = dVarValue;
        sTaylor += toString(_parser.Eval(), _option);
    }
    else if (nth_taylor == 1)
    {
        *dVar = dVarValue;
        if (toString(_parser.Eval(), _option) != "0")
            sPolynom = toString(_parser.Eval(),_option);
        if (toString(_parser.Diff(dVar, dVarValue, 1e-7), _option) == "0")
        {
            if (!sPolynom.length())
                sPolynom = "0";
        }
        else if (_parser.Diff(dVar, dVarValue) < 0)
            sPolynom += " - " + toString(-_parser.Diff(dVar, dVarValue, 1e-7), _option);
        else if (sPolynom.length())
            sPolynom += " + " + toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);
        else
            sPolynom = toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);
        if (toString(_parser.Diff(dVar, dVarValue, 1e-7), _option) != "0")
            sPolynom += sArg.substr(0,sArg.length()-1);
        sTaylor += sPolynom;
    }
    else
    {
        *dVar = dVarValue;
        if (toString(_parser.Eval(), _option) != "0")
            sPolynom = toString(_parser.Eval(),_option);
        if (toString(_parser.Diff(dVar, dVarValue, 1e-7), _option) != "0")
        {
            if (_parser.Diff(dVar, dVarValue, 1e-7) < 0)
                sPolynom += " - " + toString(-_parser.Diff(dVar, dVarValue, 1e-7), _option);
            else if (sPolynom.length())
                sPolynom += " + " + toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);
            else
                sPolynom = toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);
            sPolynom += sArg.substr(0, sArg.length()-1);
        }
        nSamples = 4*nth_taylor+1;
        nMiddle = 2*nth_taylor;
        dDiffValues = new long double*[nSamples];
        for (unsigned int i = 0; i < nSamples; i++)
        {
            dDiffValues[i] = new long double[2];
        }

        for (unsigned int i = 0; i < nSamples; i++)
        {
            dDiffValues[i][0] = dVarValue + ((double)i-(double)nMiddle)*1e-1;
        }

        for (unsigned int i = 0; i < nSamples; i++)
        {
            dDiffValues[i][1] = _parser.Diff(dVar, dDiffValues[i][0], 1e-7);
           // cerr << std::setprecision(14) << dDiffValues[i][1] << ", ";
        }
        //cerr << endl;

        for (unsigned int j = 1; j < nth_taylor; j++)
        {
            //cerr << j+1 << endl;
            for (unsigned int i = nMiddle; i < nSamples-j; i++)
            {
                if (i == nMiddle)
                {
                    double dRight = (dDiffValues[nMiddle+1][1] - dDiffValues[nMiddle][1]) / ((1.0+(j-1)*0.5) * 1e-1);
                    double dLeft = (dDiffValues[nMiddle][1] - dDiffValues[nMiddle-1][1]) / ((1.0+(j-1)*0.5) * 1e-1);
                    dDiffValues[nMiddle][1] = (dLeft + dRight) / 2.0;
                }
                else
                {
                    dDiffValues[i][1] = (dDiffValues[i+1][1] - dDiffValues[i][1]) / (1e-1);
                    dDiffValues[(int)nSamples-(int)i-1][1] = (dDiffValues[(int)nSamples-(int)i-1][1] - dDiffValues[(int)nSamples-(int)i-2][1]) / (1e-1);
                }
            }
            /*for (unsigned int i = j; i < nSamples-j; i++)
                cerr << std::setprecision(14) << dDiffValues[i][1] << ", ";
            cerr << endl;*/
            if (toString((double)dDiffValues[nMiddle][1], _option) == "0")
                continue;
            else if (dDiffValues[nMiddle][1] < 0)
                sPolynom += " - " + toString(-(double)dDiffValues[nMiddle][1]/int_faculty((int)j+1), _option);// + "/" + toString(int_faculty((int)j+1));
            else if (sPolynom.length())
                sPolynom += " + " + toString((double)dDiffValues[nMiddle][1]/int_faculty((int)j+1), _option);// + "/" + toString(int_faculty((int)j+1));
            else
                sPolynom = toString((double)dDiffValues[nMiddle][1]/int_faculty((int)j+1), _option);// + "/" + toString(int_faculty((int)j+1));
            sPolynom += sArg + toString((int)j+1);
        }

        if (!sPolynom.length())
            sTaylor += "0";
        else
            sTaylor += sPolynom;
        for (unsigned int i = 0; i < nSamples; i++)
        {
            delete[] dDiffValues[i];
        }
        delete[] dDiffValues;
        dDiffValues = 0;
    }
    if (_option.getSystemPrintStatus())
        NumeReKernel::print(LineBreak(sTaylor, _option, true, 0, 8));
    sTaylor += _lang.get("PARSERFUNCS_TAYLOR_DEFINESTRING", sExpr_cpy, sVarName, toString(dVarValue, 4), toString((int)nth_taylor));
    //sTaylor += " -set comment=\"Taylorentwicklung des Ausdrucks '" + sExpr_cpy + "' an der Stelle " + sVarName + "=" + toString(dVarValue, 4) + " bis zur Ordnung " + toString((int)nth_taylor) + "\"";

    if (_functions.isDefined(sTaylor.substr(0,sTaylor.find(":="))))
        _functions.defineFunc(sTaylor, _parser, _option, true);
    else
        _functions.defineFunc(sTaylor, _parser, _option);
    return;
}

int int_faculty(int nNumber)
{
    if (nNumber < 0)
        nNumber *= -1;
    if (nNumber == 0)
        return 1;
    for (int i = nNumber-1; i > 0; i--)
    {
        nNumber *= i;
    }
    return nNumber;
}

/*
 * --> Gibt DATENELEMENT-Indices als Ints in einem Indices-Struct zurueck <--
 * --> Index = -1, falls der Index nicht gefunden wurde/kein DATENELEMENT uebergeben wurde <--
 * --> Index = -2, falls der Index den gesamten Bereich erlaubt <--
 * --> Index = -3, falls der Index eine Stringreferenz ist <--
 * --> Gibt alle angegeben Indices-1 zurueck <--
 */
Indices parser_getIndices(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    Indices _idx;
    string sI[2] = {"<<NONE>>", "<<NONE>>"};
    string sJ[2] = {"<<NONE>>", "<<NONE>>"};
    string sArgument = "";
    unsigned int nPos = 0;
    int nParenthesis = 0;
    value_type* v = 0;
    int nResults = 0;
    string sIndexExpressions;
    vector<int> vIndexNumbers;
    for (int i = 0; i < 2; i++)
    {
        _idx.nI[i] = -1;
        _idx.nJ[i] = -1;
    }
    //cerr << sCmd << endl;
    if (sCmd.find('(') == string::npos)
        return _idx;
    nPos = sCmd.find('(');
    for (unsigned int n = nPos; n < sCmd.length(); n++)
    {
        if (sCmd[n] == '(')
            nParenthesis++;
        if (sCmd[n] == ')')
            nParenthesis--;
        if (!nParenthesis)
        {
            sArgument = sCmd.substr(nPos+1, n-nPos-1);
            break;
        }
    }
    StripSpaces(sArgument);
    if (sArgument.find("data(") != string::npos || _data.containsCacheElements(sArgument))
        parser_GetDataElement(sArgument, _parser, _data, _option);
    // --> Kurzschreibweise!
    if (!sArgument.length())
    {
        _idx.nI[0] = 0;
        _idx.nJ[0] = 0;
        _idx.nI[1] = -2;
        _idx.nJ[1] = -2;
        return _idx;
    }
    _idx.sCompiledAccessEquation = sArgument;
    //cerr << sArgument << endl;
    if (sArgument.find(',') != string::npos)
    {
        nParenthesis = 0;
        nPos = 0;
        for (unsigned int n = 0; n < sArgument.length(); n++)
        {
            if (sArgument[n] == '(' || sArgument[n] == '{')
                nParenthesis++;
            if (sArgument[n] == ')' || sArgument[n] == '}')
                nParenthesis--;
            if (sArgument[n] == ':' && !nParenthesis)
            {
                if (!nPos)
                {
                    if (!n)
                        sI[0] = "<<EMPTY>>";
                    else
                        sI[0] = sArgument.substr(0, n);
                }
                else if (n == nPos)
                {
                    sJ[0] = "<<EMPTY>>";
                }
                else
                {
                    sJ[0] = sArgument.substr(nPos, n-nPos);
                }
                nPos = n+1;
            }
            if (sArgument[n] == ',' && !nParenthesis)
            {
                if (!nPos)
                {
                    if (!n)
                        sI[0] = "<<EMPTY>>";
                    else
                        sI[0] = sArgument.substr(0, n);
                }
                else
                {
                    if (n == nPos)
                        sI[1] = "<<EMPTY>>";
                    else
                        sI[1] = sArgument.substr(nPos, n - nPos);
                }
                nPos = n+1;
            }
        }
        if (sJ[0] == "<<NONE>>")
        {
            if (nPos < sArgument.length())
                sJ[0] = sArgument.substr(nPos);
            else
                sJ[0] = "<<EMPTY>>";
        }
        else if (nPos < sArgument.length())
            sJ[1] = sArgument.substr(nPos);
        else
            sJ[1] = "<<EMPTY>>";

        // --> Vektor prüfen <--
        if (sI[0] != "<<NONE>>" && sI[1] == "<<NONE>>")
        {
            StripSpaces(sI[0]);
            if (sI[0] == "#")
                _idx.nI[0] = -3;
            else
            {
                _parser.SetExpr(sI[0]);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                {
                    for (int n = 0; n < nResults; n++)
                    {
                        if (!isnan(v[n]) && !isinf(v[n]))
                            _idx.vI.push_back((int)v[n]-1);
                    }
                }
                else
                    _idx.nI[0] = (int)v[0]-1;
            }
        }
        if (sJ[0] != "<<NONE>>" && sJ[1] == "<<NONE>>")
        {
            StripSpaces(sJ[0]);
            if (sJ[0] == "#")
                _idx.nJ[0] = -3;
            else
            {
                _parser.SetExpr(sJ[0]);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                {
                    for (int n = 0; n < nResults; n++)
                    {
                        if (!isnan(v[n]) && !isinf(v[n]))
                            _idx.vJ.push_back((int)v[n]-1);
                    }
                }
                else
                    _idx.nJ[0] = (int)v[0]-1;
            }
        }

        for (int n = 0; n < 2; n++)
        {
            //cerr << sI[n] << endl;
            //cerr << sJ[n] << endl;
            if (sI[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.nI[n] = -2;
                else
                    _idx.nI[0] = 0;
            }
            else if (sI[n] != "<<NONE>>")
            {
                if (!_idx.vI.size() && _idx.nI[0] != -3)
                {
                    if (sIndexExpressions.length())
                        sIndexExpressions += ",";
                    sIndexExpressions += sI[n];
                    vIndexNumbers.push_back((n+1));
                }
            }
            if (sJ[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.nJ[n] = -2;
                else
                    _idx.nJ[0] = 0;
            }
            else if (sJ[n] != "<<NONE>>")
            {
                if (!_idx.vJ.size() && _idx.nJ[0] != -3)
                {
                    if (sIndexExpressions.length())
                        sIndexExpressions += ",";
                    sIndexExpressions += sJ[n];
                    vIndexNumbers.push_back(-(n+1));
                }
            }
        }
        if (sIndexExpressions.length())
        {
            _parser.SetExpr(sIndexExpressions);
            v = _parser.Eval(nResults);
            if ((size_t)nResults != vIndexNumbers.size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
            for (int i = 0; i < nResults; i++)
            {
                if (isnan(v[i]) || isinf(v[i]) || v[i] <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
                if (vIndexNumbers[i] > 0)
                    _idx.nI[vIndexNumbers[i]-1] = (int)v[i]-1;
                else
                    _idx.nJ[abs(vIndexNumbers[i])-1] = (int)v[i]-1;
            }
        }
        if (_idx.vI.size() || _idx.vJ.size())
        {
            string sCache = sCmd.substr(0,sCmd.find('('));
            if (!sCache.length())
                throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);
            if (!sCache.find("data(") && !_data.isCacheElement(sCache))
                throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);
            if (sCache.find(' ') != string::npos)
                sCache.erase(0,sCache.rfind(' ')+1);
            if (!_idx.vI.size())
            {
                if (_idx.nI[0] == -1)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
                if (_idx.nI[1] == -2 && _idx.nI[0] != -3)
                {
                    for (long long int i = _idx.nI[0]; i < _data.getLines(sCache, false); i++)
                        _idx.vI.push_back(i);
                }
                else if (_idx.nI[1] == -1)
                    _idx.vI.push_back(_idx.nI[0]);
                else if (_idx.nI[0] != -3)
                {
                    for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                        _idx.vI.push_back(i);
                }
            }
            if (!_idx.vJ.size())
            {
                if (_idx.nJ[0] == -1)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
                if (_idx.nJ[1] == -2 && _idx.nJ[0] != -3)
                {
                    for (long long int j = _idx.nJ[0]; j < _data.getCols(sCache); j++)
                        _idx.vJ.push_back(j);
                }
                else if (_idx.nJ[1] == -1)
                    _idx.vJ.push_back(_idx.nJ[0]);
                else if (_idx.nJ[0] != -3)
                {
                    for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                        _idx.vJ.push_back(j);
                }
            }
        }
    }
    return _idx;
}

string parser_evalStringLogic(string sLine, bool& bReturningLogicals)
{
    if (!sLine.length())
        return "false";
    if (sLine.find('"') == string::npos)
    {
        bReturningLogicals = true;
        return sLine;
    }

    sLine += " ";
    //cerr << sLine << endl;
    unsigned int nPos = 0;
    if (sLine.find('(') != string::npos)
    {
        while (sLine.find('(', nPos) != string::npos)
        {
            nPos = sLine.find('(', nPos) + 1;
            if (!isInQuotes(sLine, nPos-1))
            {
                sLine = sLine.substr(0,nPos-1) + parser_evalStringLogic(sLine.substr(nPos, getMatchingParenthesis(sLine.substr(nPos-1))-1), bReturningLogicals) + sLine.substr(getMatchingParenthesis(sLine.substr(nPos-1))+nPos);
                //cerr << sLine << endl;
                nPos = 0;
            }
        }
    }
    if (sLine.find("&&") != string::npos)
    {
        nPos = 0;
        while (sLine.find("&&", nPos) != string::npos)
        {
            nPos = sLine.find("&&", nPos)+2;
            if (!isInQuotes(sLine, nPos-2))
            {
                string sLeft = parser_evalStringLogic(sLine.substr(0,nPos-2), bReturningLogicals);
                string sRight = parser_evalStringLogic(sLine.substr(nPos), bReturningLogicals);
                StripSpaces(sLeft);
                StripSpaces(sRight);
                if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
                    sLeft = sLeft.substr(1,sLeft.length()-2);
                if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
                    sRight = sRight.substr(1,sRight.length()-2);
                bReturningLogicals = true;
                if (sLeft == "true" && sRight == "true")
                    return "true";
                else
                    return "false";
            }
        }
    }
    if (sLine.find("|||") != string::npos)
    {
        nPos = 0;
        while (sLine.find("|||", nPos) != string::npos)
        {
            nPos = sLine.find("|||", nPos)+3;
            if (!isInQuotes(sLine, nPos-3))
            {
                string sLeft = parser_evalStringLogic(sLine.substr(0,nPos-3), bReturningLogicals);
                string sRight = parser_evalStringLogic(sLine.substr(nPos), bReturningLogicals);
                StripSpaces(sLeft);
                StripSpaces(sRight);
                if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
                    sLeft = sLeft.substr(1,sLeft.length()-2);
                if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
                    sRight = sRight.substr(1,sRight.length()-2);
                bReturningLogicals = true;
                if ((sLeft == "true" || sRight == "true") && sLeft != sRight)
                    return "true";
                else
                    return "false";
            }
        }
    }
    if (sLine.find("||") != string::npos)
    {
        nPos = 0;
        while (sLine.find("||", nPos) != string::npos)
        {
            nPos = sLine.find("||", nPos)+2;
            if (!isInQuotes(sLine, nPos-2))
            {
                string sLeft = parser_evalStringLogic(sLine.substr(0,nPos-2), bReturningLogicals);
                string sRight = parser_evalStringLogic(sLine.substr(nPos), bReturningLogicals);
                StripSpaces(sLeft);
                StripSpaces(sRight);
                if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
                    sLeft = sLeft.substr(1,sLeft.length()-2);
                if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
                    sRight = sRight.substr(1,sRight.length()-2);
                bReturningLogicals = true;
                if (sLeft == "true" || sRight == "true")
                    return "true";
                else
                    return "false";
            }
        }
    }
    if (sLine.find("==") != string::npos && !isInQuotes(sLine, sLine.find("==")))
    {
        string sLeft = sLine.substr(0,sLine.find("=="));
        string sRight = sLine.substr(sLine.find("==")+2);
        StripSpaces(sLeft);
        StripSpaces(sRight);
        if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
            sLeft = sLeft.substr(1,sLeft.length()-2);
        if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
            sRight = sRight.substr(1,sRight.length()-2);
        bReturningLogicals = true;
        if (sLeft == sRight)
            return "true";
        else
            return "false";
    }
    else if (sLine.find("!=") != string::npos && !isInQuotes(sLine, sLine.find("!=")))
    {
        string sLeft = sLine.substr(0,sLine.find("!="));
        string sRight = sLine.substr(sLine.find("!=")+2);
        StripSpaces(sLeft);
        StripSpaces(sRight);
        if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
            sLeft = sLeft.substr(1,sLeft.length()-2);
        if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
            sRight = sRight.substr(1,sRight.length()-2);
        bReturningLogicals = true;
        if (sLeft != sRight)
            return "true";
        else
            return "false";
    }
    else if (sLine.find("<=") != string::npos && !isInQuotes(sLine, sLine.find("<=")))
    {
        string sLeft = sLine.substr(0,sLine.find("<="));
        string sRight = sLine.substr(sLine.find("<=")+2);
        StripSpaces(sLeft);
        StripSpaces(sRight);
        if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
            sLeft = sLeft.substr(1,sLeft.length()-2);
        if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
            sRight = sRight.substr(1,sRight.length()-2);
        bReturningLogicals = true;
        if (sLeft <= sRight)
            return "true";
        else
            return "false";
    }
    else if (sLine.find(">=") != string::npos && !isInQuotes(sLine, sLine.find(">=")))
    {
        string sLeft = sLine.substr(0,sLine.find(">="));
        string sRight = sLine.substr(sLine.find(">=")+2);
        StripSpaces(sLeft);
        StripSpaces(sRight);
        if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
            sLeft = sLeft.substr(1,sLeft.length()-2);
        if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
            sRight = sRight.substr(1,sRight.length()-2);
        bReturningLogicals = true;
        if (sLeft >= sRight)
            return "true";
        else
            return "false";
    }
    else if (sLine.find('<') != string::npos && !isInQuotes(sLine, sLine.find('<')))
    {
        string sLeft = sLine.substr(0,sLine.find('<'));
        string sRight = sLine.substr(sLine.find('<')+1);
        StripSpaces(sLeft);
        StripSpaces(sRight);
        if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
            sLeft = sLeft.substr(1,sLeft.length()-2);
        if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
            sRight = sRight.substr(1,sRight.length()-2);
        bReturningLogicals = true;
        if (sLeft < sRight)
            return "true";
        else
            return "false";
    }
    else if (sLine.find('>') != string::npos && !isInQuotes(sLine, sLine.find('>')))
    {
        string sLeft = sLine.substr(0,sLine.find('>'));
        string sRight = sLine.substr(sLine.find('>')+1);
        StripSpaces(sLeft);
        StripSpaces(sRight);
        if (sLeft[0] == '"' && sLeft[sLeft.length()-1] == '"')
            sLeft = sLeft.substr(1,sLeft.length()-2);
        if (sRight[0] == '"' && sRight[sRight.length()-1] == '"')
            sRight = sRight.substr(1,sRight.length()-2);
        bReturningLogicals = true;
        if (sLeft > sRight)
            return "true";
        else
            return "false";
    }
    StripSpaces(sLine);
    return sLine;
}

bool parser_parseCmdArg(const string& sCmd, const string& sParam, Parser& _parser, int& nArgument)
{
    if (!sCmd.length() || !sParam.length())
        return false;

    unsigned int nPos = 0;
    if (matchParams(sCmd, sParam) || matchParams(sCmd, sParam, '='))
    {
        if (matchParams(sCmd, sParam))
        {
            nPos = matchParams(sCmd, sParam) + sParam.length();
        }
        else
        {
            nPos = matchParams(sCmd, sParam, '=') + sParam.length();
        }
        while (sCmd[nPos] == ' ' && nPos < sCmd.length()-1)
            nPos++;
        if (sCmd[nPos] == ' ' || nPos >= sCmd.length()-1)
            return false;

        string sArg = sCmd.substr(nPos);
        if (sArg[0] == '(')
            sArg = sArg.substr(1, getMatchingParenthesis(sArg)-1);
        else
            sArg = sArg.substr(0, sArg.find(' '));
        _parser.SetExpr(sArg);
        if (isnan(_parser.Eval()) || isinf(_parser.Eval()))
            return false;
        nArgument = (int)_parser.Eval();
        return true;
    }
    return false;
}

bool parser_fit(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    vector<double> vx;
    vector<double> vy;
    vector<double> vy_w;
    vector<double> vTempZ;
    vector<vector<double> > vz;
    vector<vector<double> > vz_w;

    vector<double> vInitialVals;

    ofstream oFitLog;
    ofstream oTeXExport;
    string sFitLog = "<savepath>/numerefit.log";
    sFitLog = _data.ValidFileName(sFitLog, ".log");
    unsigned int nDim = 1;
    unsigned int nFitVars = 0;
    bool bUseErrors = false;
    bool bSaveErrors = false;
    double dChisq = 0.0;
    double dNormChisq = 0.0;
    bool bRestrictXVals = false;
    bool bRestrictYVals = false;
    bool bMaskDialog = false;
    bool bNoParams = false;
    bool b1DChiMap = false;
    bool bTeXExport = false;
    string sTeXExportFile = "<savepath>/fit.tex";
    double dMin = NAN;
    double dMax = NAN;
    double dMinY = NAN;
    double dMaxY = NAN;
    double dPrecision = 1e-4;
    int nMaxIterations = 500;

    double dErrorPercentageSum = 0.0;
    vector<double> vInterVal;

    Indices _idx;

    if (findCommand(sCmd, "fit").sString == "fitw")
        bUseErrors = true;

    if (sCmd.find("data(") == string::npos && !_data.containsCacheElements(sCmd))
        throw SyntaxError(SyntaxError::NO_DATA_FOR_FIT, sCmd, SyntaxError::invalid_position);
    //string sBadFunctions = "ascii(),avg(),betheweizsaecker(),binom(),char(),cmp(),date(),dblfacul(),faculty(),findfile(),findparam(),gauss(),gcd(),getopt(),heaviside(),hermite(),is_nan(),is_string(),laguerre(),laguerre_a(),lcm(),legendre(),legendre_a(),max(),min(),norm(),num(),cnt(),pct(),phi(),prd(),rand(),range(),replace(),replaceall(),rint(),round(),sbessel(),sneumann(),split(),std(),strfnd(),string_cast(),strrfnd(),strlen(),student_t(),substr(),sum(),theta(),time(),to_char(),to_cmd(),to_string(),to_value(),Y()";
    string sBadFunctions = "ascii(),char(),findfile(),findparam(),gauss(),getopt(),is_string(),rand(),replace(),replaceall(),split(),strfnd(),string_cast(),strrfnd(),strlen(),time(),to_char(),to_cmd(),to_string(),to_value()";
    string sFitFunction = sCmd;
    string sParams = "";
    string sFuncDisplay = "";
    string sFunctionDefString = "";
    string sFittedFunction = "";
    string sRestrictions = "";
    string sChiMap = "";
    string sChiMap_Vars[2] = {"",""};

    mu::varmap_type varMap;
    mu::varmap_type paramsMap;

    if (matchParams(sCmd, "chimap", '='))
    {
        sChiMap = getArgAtPos(sCmd, matchParams(sCmd, "chimap", '=')+6);
        eraseToken(sCmd, "chimap", true);

        if (sChiMap.length())
        {
            if (sChiMap.substr(0,sChiMap.find('(')) == "data")
                throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);
            _idx = parser_getIndices(sChiMap, _parser, _data, _option);
            if ((_idx.nI[0] == -1 || _idx.nJ[0] == -1) && (!_idx.vI.size() && !_idx.vJ.size()))
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
            if (_idx.vJ.size() && _idx.vJ.size() < 2)
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
            parser_evalIndices(sChiMap, _idx, _data);
            sChiMap.erase(sChiMap.find('('));
            if (!_idx.vJ.size())
            {
                if (_idx.nJ[1] < _idx.nJ[0])
                {
                    sChiMap_Vars[0] = _data.getHeadLineElement(_idx.nJ[0], sChiMap);
                    sChiMap_Vars[1] = _data.getHeadLineElement(_idx.nJ[0]-1, sChiMap);
                }
                else
                {
                    sChiMap_Vars[0] = _data.getHeadLineElement(_idx.nJ[0], sChiMap);
                    sChiMap_Vars[1] = _data.getHeadLineElement(_idx.nJ[0]+1, sChiMap);
                }
            }
            else
            {
                sChiMap_Vars[0] = _data.getHeadLineElement(_idx.vJ[0], sChiMap);
                sChiMap_Vars[1] = _data.getHeadLineElement(_idx.vJ[1], sChiMap);
            }
        }
    }
    if (matchParams(sCmd, "export", '='))
    {
        bTeXExport = true;
        sTeXExportFile = getArgAtPos(sCmd, matchParams(sCmd, "export", '=')+6);
        eraseToken(sCmd, "export", true);
    }
    else if (matchParams(sCmd, "export"))
    {
        bTeXExport = true;
        eraseToken(sCmd, "export", false);
    }

    if (bTeXExport)
    {
        sTeXExportFile = _data.ValidFileName(sTeXExportFile, ".tex");
        if (sTeXExportFile.substr(sTeXExportFile.rfind('.')) != ".tex")
            sTeXExportFile.replace(sTeXExportFile.rfind('.'), string::npos, ".tex");
    }

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == '(')
            i += getMatchingParenthesis(sCmd.substr(i));
        if (sCmd[i] == '-')
        {
            sCmd.erase(0,i);
            break;
        }
    }
    vInterVal = parser_IntervalReader(sCmd, _parser, _data, _functions, _option, true);
    //cerr << sCmd << endl;
    if (vInterVal.size())
    {
        if (vInterVal.size() >= 4)
        {
            dMin = vInterVal[0];
            dMax = vInterVal[1];
            dMinY = vInterVal[2];
            dMaxY = vInterVal[3];
            if (!isnan(dMin) || !isnan(dMax))
                bRestrictXVals = true;
            if (!isnan(dMinY) || !isnan(dMaxY))
                bRestrictYVals = true;
        }
        else if (vInterVal.size() == 2)
        {
            dMin = vInterVal[0];
            dMax = vInterVal[1];
            if (!isnan(dMin) || !isnan(dMax))
                bRestrictXVals = true;
        }
    }
    //cerr << dMin << " " << dMax << endl;
    for (unsigned int i = 0; i < sFitFunction.length(); i++)
    {
        if (sFitFunction[i] == '(')
            i += getMatchingParenthesis(sFitFunction.substr(i));
        if (sFitFunction[i] == '-')
        {
            sFitFunction.replace(i, string::npos, sCmd.substr(sCmd.find('-')));
            break;
        }
    }
    sCmd = sFitFunction;
    //cerr << sFitFunction << endl;
    //sFitFunction.replace(sFitFunction.find('-'), string::npos, sCmd.substr(sCmd.find('-')));

    if (matchParams(sFitFunction, "saverr"))
    {
        bSaveErrors = true;
        sFitFunction.erase(matchParams(sFitFunction, "saverr")-1, 6);
        sCmd.erase(matchParams(sCmd, "saverr")-1, 6);
    }
    if (matchParams(sFitFunction, "saveer"))
    {
        bSaveErrors = true;
        sFitFunction.erase(matchParams(sFitFunction, "saveer")-1, 6);
        sCmd.erase(matchParams(sCmd, "saveer")-1, 6);
    }
    if (matchParams(sFitFunction, "mask"))
    {
        bMaskDialog = true;
        sFitFunction.erase(matchParams(sFitFunction, "mask")-1, 6);
        sCmd.erase(matchParams(sCmd, "mask")-1, 6);
    }
    if (!matchParams(sFitFunction, "with", '='))
        throw SyntaxError(SyntaxError::NO_FUNCTION_FOR_FIT, sCmd, SyntaxError::invalid_position);
    if (matchParams(sFitFunction, "tol", '='))
    {
        _parser.SetExpr(getArgAtPos(sFitFunction, matchParams(sFitFunction, "tol", '=')+3));
        eraseToken(sCmd, "tol", true);
        eraseToken(sFitFunction, "tol", true);
        dPrecision = fabs(_parser.Eval());
        if (isnan(dPrecision) || isinf(dPrecision) || dPrecision == 0)
            dPrecision = 1e-4;
    }
    if (matchParams(sFitFunction, "iter", '='))
    {
        _parser.SetExpr(getArgAtPos(sFitFunction, matchParams(sFitFunction, "iter", '=')+4));
        eraseToken(sCmd, "iter", true);
        eraseToken(sFitFunction, "iter", true);
        nMaxIterations = abs(rint(_parser.Eval()));
        if (!nMaxIterations)
            nMaxIterations = 500;
    }
    if (matchParams(sFitFunction, "restrict", '='))
    {
        sRestrictions = getArgAtPos(sFitFunction, matchParams(sFitFunction, "restrict", '=')+8);
        eraseToken(sCmd, "restrict", true);
        eraseToken(sFitFunction, "restrict", true);
        if (sRestrictions.length() && sRestrictions.front() == '[' && sRestrictions.back() == ']')
        {
            sRestrictions.erase(0,1);
            sRestrictions.pop_back();
        }
        StripSpaces(sRestrictions);
        if (sRestrictions.length())
        {
            if (sRestrictions.front() == ',')
                sRestrictions.erase(0,1);
            if (sRestrictions.back() == ',')
                sRestrictions.pop_back();
            _parser.SetExpr(sRestrictions);
            _parser.Eval();
        }
    }
    if (!matchParams(sFitFunction, "params", '='))
    {
        //throw NO_PARAMS_FOR_FIT;
        bNoParams = true;
        sFitFunction = sFitFunction.substr(matchParams(sFitFunction, "with", '=')+4);
        sCmd.erase(matchParams(sCmd, "with", '=')-1);
    }
    else if (matchParams(sFitFunction, "with", '=') < matchParams(sFitFunction, "params", '='))
    {
        sParams = sFitFunction.substr(matchParams(sFitFunction, "params", '=')+6);
        sFitFunction = sFitFunction.substr(matchParams(sFitFunction, "with", '=')+4, matchParams(sFitFunction, "params", '=')-1-matchParams(sFitFunction, "with", '=')-4);
        sCmd = sCmd.substr(0,matchParams(sCmd, "with", '=')-1);
    }
    else
    {
        sParams = sFitFunction.substr(matchParams(sFitFunction, "params", '=')+6, matchParams(sFitFunction, "with", '=')-1-matchParams(sFitFunction, "params", '=')-6);
        sFitFunction = sFitFunction.substr(matchParams(sFitFunction, "with", '=')+4);
        sCmd = sCmd.substr(0,matchParams(sCmd, "params", '=')-1);
    }
    if (sParams.find('[') != string::npos)
        sParams = sParams.substr(sParams.find('[')+1);
    if (sParams.find(']') != string::npos)
        sParams = sParams.substr(0,sParams.find(']'));
    StripSpaces(sFitFunction);
    if (sFitFunction[sFitFunction.length()-1] == '-')
    {
        sFitFunction[sFitFunction.length()-1] = ' ';
        StripSpaces(sFitFunction);
    }
    if (!bNoParams)
    {
        StripSpaces(sParams);
        if (sParams[sParams.length()-1] == '-')
        {
            sParams[sParams.length()-1] = ' ';
            StripSpaces(sParams);
        }
        if (!_functions.call(sParams, _option))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sParams, sParams);
        if (sParams.find("data(") != string::npos || _data.containsCacheElements(sParams))
        {
            parser_GetDataElement(sParams, _parser, _data, _option);
        }
        if (sParams.find("{") != string::npos && (containsStrings(sParams) || _data.containsStringVars(sParams)))
            parser_VectorToExpr(sParams, _option);
    }
    StripSpaces(sCmd);

    if (!_functions.call(sFitFunction, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sFitFunction, sFitFunction);

    if (sFitFunction.find("data(") != string::npos || _data.containsCacheElements(sFitFunction))
    {
        parser_GetDataElement(sFitFunction, _parser, _data, _option);
    }
    if (sFitFunction.find("{") != string::npos)
        parser_VectorToExpr(sFitFunction, _option);

    unsigned int nPos = 0;
    while (sBadFunctions.find(',', nPos) != string::npos)
    {
        if (sFitFunction.find(sBadFunctions.substr(nPos, sBadFunctions.find(',', nPos)-nPos-1)) != string::npos)
        {
            //sErrorToken = sBadFunctions.substr(nPos, sBadFunctions.find(',', nPos)-nPos);
            throw SyntaxError(SyntaxError::FUNCTION_CANNOT_BE_FITTED, sCmd, SyntaxError::invalid_position, sBadFunctions.substr(nPos, sBadFunctions.find(',', nPos)-nPos));
        }
        else
            nPos = sBadFunctions.find(',', nPos)+1;
        if (nPos >= sBadFunctions.length())
            break;
    }
    nPos = 0;

    sFitFunction = " " + sFitFunction + " ";
    _parser.SetExpr(sFitFunction);
    _parser.Eval();
    varMap = _parser.GetUsedVar();
    if (bNoParams)
    {
        paramsMap = varMap;
        if (paramsMap.find("x") != paramsMap.end())
            paramsMap.erase(paramsMap.find("x"));
        if (paramsMap.find("y") != paramsMap.end())
            paramsMap.erase(paramsMap.find("y"));
        if (!paramsMap.size())
            throw SyntaxError(SyntaxError::NO_PARAMS_FOR_FIT, sCmd, SyntaxError::invalid_position);
    }
    else
    {
        _parser.SetExpr(sParams);
        _parser.Eval();
        // Falls noch andere Variablen zum Initialisieren verwendet werden, werden die hier entfernt
        if (sParams.find('=') != string::npos)
        {
            for (unsigned int i = 0; i < sParams.length(); i++)
            {
                if (sParams[i] == '=')
                {
                    for (unsigned int j = i; j < sParams.length(); j++)
                    {
                        if (sParams[j] == '(')
                            j += getMatchingParenthesis(sParams.substr(j));
                        if (sParams[j] == ',')
                        {
                            sParams.erase(i,j-i);
                            break;
                        }
                        if (j == sParams.length()-1)
                            sParams.erase(i);
                    }
                }
            }
            _parser.SetExpr(sParams);
            _parser.Eval();
        }
        paramsMap = _parser.GetUsedVar();
    }
    //_fitParams.Create(paramsMap.size());


    mu::varmap_type::const_iterator pItem = paramsMap.begin();
    mu::varmap_type::const_iterator vItem = varMap.begin();
    sParams = "";
    if (varMap.find("x") != varMap.end())
        nFitVars+=1;
    if (varMap.find("y") != varMap.end())
        nFitVars+=2;
    if (varMap.find("z") != varMap.end())
        nFitVars+=4;

    if (sChiMap.length())
    {
        if (sChiMap_Vars[0] == "x" || sChiMap_Vars[1] == "x")
        {
            //sErrorToken = "x";
            throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, "x");
        }
        if (sChiMap_Vars[0] == "y" || sChiMap_Vars[1] == "y")
        {
            //sErrorToken = "y";
            throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, "y");
        }
        if (varMap.find(sChiMap_Vars[0]) == varMap.end())
        {
            //sErrorToken = sChiMap_Vars[0];
            throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, sChiMap_Vars[0]);
        }
        if (varMap.find(sChiMap_Vars[1]) == varMap.end())
        {
            b1DChiMap = true;
        }
    }

    if (!nFitVars || !(nFitVars & 1))
    {
        //sErrorToken = "x";
        throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, "x");
    }

    pItem = paramsMap.begin();
    for (; pItem != paramsMap.end(); ++pItem)
    {
        if (pItem->first == "x" || pItem->first == "y" || pItem->first == "z")
        {
            //sErrorToken = pItem->first;
            throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, pItem->first);
        }

        bool bParamFound = false;
        vItem = varMap.begin();
        for (; vItem != varMap.end(); ++vItem)
        {
            if (vItem->first == pItem->first)
            {
                bParamFound = true;
                break;
            }
        }
        if (!bParamFound)
        {
            //sErrorToken = pItem->first;
            throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, pItem->first);
        }
    }
    if (sChiMap.length())
    {
        paramsMap.erase(sChiMap_Vars[0]);
        if (!b1DChiMap)
            paramsMap.erase(sChiMap_Vars[1]);
    }
    if (!paramsMap.size())
        throw SyntaxError(SyntaxError::NO_PARAMS_FOR_FIT, sCmd, SyntaxError::invalid_position);

    sFuncDisplay = sFitFunction;
    StripSpaces(sFuncDisplay);
    pItem = paramsMap.begin();

    if (_option.getbDebug())
        cerr << "|-> DEBUG: sFitFunction = " << sFitFunction << endl;
    sCmd.erase(0,findCommand(sCmd).nPos+findCommand(sCmd).sString.length());
    StripSpaces(sCmd);

    string si_pos[2] = {"", ""};                    // String-Array fuer die Zeilen-Position: muss fuer alle Spalten identisch sein!
    string sj_pos[6] = {"", "", "", "", "", ""};    // String-Array fuer die Spalten: kann bis zu sechs beliebige Werte haben
    string sDataTable = "data";
    int i_pos[2] = {0, 0};                          // Int-Array fuer den Wert der Zeilen-Positionen
    int j_pos[6] = {0, 0, 0, 0, 0, 0};              // Int-Array fuer den Wert der Spalten-Positionen
    int nMatch = 0;                                 // Int fuer die Position des aktuellen find-Treffers eines Daten-Objekts
    vector<long long int> vLine;
    vector<long long int> vCol;
    value_type* v = 0;
    int nResults = 0;

    // --> Ist da "cache" drin? Aktivieren wir den Cache-Status <--
    if (_data.containsCacheElements(sCmd) && sCmd.substr(0,5) != "data(")
    {
        if (_data.isValidCache())
            _data.setCacheStatus(true);
        else
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);

        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
        {
            if (sCmd.find(iter->first+"(") != string::npos
                && (!sCmd.find(iter->first+"(")
                    || (sCmd.find(iter->first+"(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first+"(")-1, (iter->first).length()+2)))))
            {
                sDataTable = iter->first;
                break;
            }
        }
    }
    else if (!_data.isValid())
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
    // --> Klammer und schliessende Klammer finden und in einen anderen String schreiben <--
    nMatch = sCmd.find('(');
    si_pos[0] = sCmd.substr(nMatch, getMatchingParenthesis(sCmd.substr(nMatch))+1);
    if (si_pos[0] == "()" || si_pos[0][si_pos[0].find_first_not_of(' ',1)] == ')')
        si_pos[0] = "(:,:)";
    if (si_pos[0].find("data(") != string::npos || _data.containsCacheElements(si_pos[0]))
    {
        parser_GetDataElement(si_pos[0], _parser, _data, _option);
    }
    if (_option.getbDebug())
        cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << endl;

    // --> Rausgeschnittenen String am Komma ',' in zwei Teile teilen <--
    try
    {
        parser_SplitArgs(si_pos[0], sj_pos[0], ',', _option);
    }
    catch (...)
    {
        //delete[] _mDataPlots;
        //delete[] nDataDim;
        throw;
    }
    if (_option.getbDebug())
        cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", sj_pos[0] = " << sj_pos[0] << endl;

    // --> Gibt's einen Doppelpunkt? Dann teilen wir daran auch noch mal <--
    if (si_pos[0].find(':') != string::npos)
    {
        si_pos[0] = "( " + si_pos[0] + " )";
        try
        {
            parser_SplitArgs(si_pos[0], si_pos[1], ':', _option);
        }
        catch (...)
        {
            //delete[] _mDataPlots;
            //delete[] nDataDim;
            throw;
        }
        if (!parser_ExprNotEmpty(si_pos[1]))
            si_pos[1] = "inf";
    }
    else
        si_pos[1] = "";

    if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", si_pos[1] = " << si_pos[1] << endl;
    }

    // --> Auswerten mit dem Parser <--
    if (parser_ExprNotEmpty(si_pos[0]))
    {
        _parser.SetExpr(si_pos[0]);
        v = _parser.Eval(nResults);
        if (nResults > 1)
        {
            for (int n = 0; n < nResults; n++)
                vLine.push_back((int)v[n]-1);
        }
        else
            i_pos[0] = (int)v[0] - 1;
    }
    else
        i_pos[0] = 0;
    if (si_pos[1] == "inf")
    {
        i_pos[1] = _data.getLines(sDataTable, false);
    }
    else if (parser_ExprNotEmpty(si_pos[1]))
    {
        _parser.SetExpr(si_pos[1]);
        i_pos[1] = (int)_parser.Eval();
    }
    else if (!vLine.size())
        i_pos[1] = i_pos[0]+1;
    // --> Pruefen, ob die Reihenfolge der Indices sinnvoll ist <--
    parser_CheckIndices(i_pos[0], i_pos[1]);

    if (_option.getbDebug())
        cerr << "|-> DEBUG: i_pos[0] = " << i_pos[0] << ", i_pos[1] = " << i_pos[1] << ", vLine.size() = " << vLine.size() << endl;

    if (!parser_ExprNotEmpty(sj_pos[0]))
        sj_pos[0] = "0";

    /* --> Jetzt fuer die Spalten: Fummelig. Man soll bis zu 6 Spalten angeben koennen und
     *     das Programm sollte trotzdem einen Sinn darin finden <--
     */
    int j = 0;
    try
    {
        while (sj_pos[j].find(':') != string::npos && j < 5)
        {
            sj_pos[j] = "( " + sj_pos[j] + " )";
            // --> String am naechsten ':' teilen <--
            parser_SplitArgs(sj_pos[j], sj_pos[j+1], ':', _option);
            // --> Spezialfaelle beachten: ':' ohne linke bzw. rechte Grenze <--
            if (!parser_ExprNotEmpty(sj_pos[j]))
                sj_pos[j] = "1";
            j++;
            if (!parser_ExprNotEmpty(sj_pos[j]))
                sj_pos[j] = "inf";
        }
    }
    catch (...)
    {
        //delete[] _mDataPlots;
        //delete[] nDataDim;
        throw;
    }
    // --> Alle nicht-beschriebenen Grenzen-Strings auf "" setzen <--
    for (int k = j+1; k < 6; k++)
        sj_pos[k] = "";

    // --> Grenzen-Strings moeglichst sinnvoll auswerten <--
    for (int k = 0; k <= j; k++)
    {
        // --> "inf" bedeutet "infinity". Ergo: die letztmoegliche Spalte <--
        if (sj_pos[k] == "inf")
        {
            j_pos[k] = _data.getCols(sDataTable)-1;
            break;
        }
        else if (parser_ExprNotEmpty(sj_pos[k]))
        {
            if (j == 0)
            {
                _parser.SetExpr(sj_pos[0]);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                {
                    for (int n = 0; n < nResults; n++)
                    {
                        if (n >= 6)
                            break;
                        vCol.push_back((int)v[n]-1);
                        j_pos[n] = (int)v[n]-1;
                        j = n;
                    }
                    break;
                }
                else
                    j_pos[0] = (int)v[0] - 1;
            }
            else
            {
                // --> Hat einen Wert: Kann man auch auswerten <--
                _parser.SetExpr(sj_pos[k]);
                j_pos[k] = (int)_parser.Eval() - 1;
            }
        }
        else if (!k)
        {
            // --> erstes Element pro Forma auf 0 setzen <--
            j_pos[k] = 0;
        }
        else // "data(2:4::7) = Spalten 2-4,5-7"
        {
            // --> Spezialfall. Verwendet vermutlich niemand <--
            j_pos[k] = j_pos[k]+1;
        }
    }
    if (_option.getbDebug())
        cerr << "|-> DEBUG: j_pos[0] = " << j_pos[0] << ", j_pos[1] = " << j_pos[1] << ", vCol.size() = " << vCol.size() << endl;
    if (i_pos[1] > _data.getLines(sDataTable, false))
        i_pos[1] = _data.getLines(sDataTable, false);
    if (j_pos[1] > _data.getCols(sDataTable)-1)
        j_pos[1] = _data.getCols(sDataTable)-1;
    if (!vLine.size() && !vCol.size() && (j_pos[0] < 0
        || j_pos[1] < 0
        || i_pos[0] > _data.getLines(sDataTable, false)
        || i_pos[1] > _data.getLines(sDataTable, false)
        || j_pos[0] > _data.getCols(sDataTable)-1
        || j_pos[1] > _data.getCols(sDataTable)-1))
    {
        /*delete[] _mDataPlots;
        delete[] nDataDim;*/
        throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
    }
    if (_option.getbDebug())
        cerr << "|-> DEBUG: j_pos[0] = " << j_pos[0] << ", j_pos[1] = " << j_pos[1] << endl;

    // --> Jetzt wissen wir die Spalten: Suchen wir im Falle von si_pos[1] == inf nach der laengsten <--
    if (si_pos[1] == "inf")
    {
        int nAppendedZeroes = _data.getAppendedZeroes(j_pos[0], sDataTable);
        for (int k = 1; k <= j; k++)
        {
            if (nAppendedZeroes > _data.getAppendedZeroes(j_pos[k], sDataTable))
                nAppendedZeroes = _data.getAppendedZeroes(j_pos[k], sDataTable);
        }
        if (nAppendedZeroes < i_pos[1])
            i_pos[1] = _data.getLines(sDataTable, true) - nAppendedZeroes;
        if (_option.getbDebug())
            cerr << "|-> DEBUG: i_pos[1] = " << i_pos[1] << endl;
    }


    /* --> Bestimmen wir die "Dimension" des zu fittenden Datensatzes. Dabei ist es auch
     *     von Bedeutung, ob Fehlerwerte verwendet werden sollen <--
     */
    nDim = 0;
    if (j == 0 && bUseErrors && vCol.size() < 3)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
    if (j == 0 && !vCol.size())
        nDim = 2;
    else if (j == 0)
        nDim = vCol.size();
    else if (j == 1)
    {
        if (!bUseErrors)
        {
            if (!(nFitVars & 2))
            {
                nDim = 2;
                if (abs(j_pos[1] - j_pos[0]) < 1)
                    throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
            }
            else
            {
                nDim = 3;
                if (abs(j_pos[1] - j_pos[0]) < abs(i_pos[1] - i_pos[0])+1)
                    throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
            }
        }
        else
        {
            if (!(nFitVars & 2))
            {
                nDim = 4;
                if (abs(j_pos[1]-j_pos[0]) < 2)
                    throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
            }
            else
            {
                nDim = 5;
                if (abs(j_pos[1]-j_pos[0]) < 2*abs(i_pos[1]-i_pos[0])+1)
                    throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
            }
        }
    }
    else
    {
        nDim = j+1;
    }

    parser_CheckIndices(i_pos[0], i_pos[1]);
    // Groesse der Datensaetze bestimmen:
    if (vLine.size() && !vCol.size())
    {
        vCol.push_back(j_pos[0]);
        if (j == 1)
        {
            if (nDim == 2)
            {
                if (sj_pos[1] == "inf")
                    vCol.push_back(j_pos[0]+1);
                else
                    vCol.push_back(j_pos[1]);
            }
            else
            {
                if (j_pos[0] < j_pos[1] || sj_pos[1] == "inf")
                {
                    for (unsigned int n = 1; n < nDim; n++)
                        vCol.push_back(j_pos[0]+n);
                }
                else if (j_pos[0] < j_pos[1])
                {
                    for (unsigned int n = 1; n < nDim; n++)
                        vCol.push_back(j_pos[0]-n);
                }
            }
        }
        else
        {
            for (int n = 1; n <= j; n++)
                vCol.push_back(j_pos[n]);
        }
    }


    if (isnan(dMin))
    {
        if (!vLine.size())
            dMin = _data.min(sDataTable, i_pos[0], i_pos[1], j_pos[0]);
        else
            dMin = _data.min(sDataTable, vLine, vector<long long int>(1,vCol[0]));
    }
    if (isnan(dMax))
    {
        if (!vLine.size())
            dMax = _data.max(sDataTable, i_pos[0], i_pos[1], j_pos[0]);
        else
            dMax = _data.max(sDataTable, vLine, vector<long long int>(1,vCol[0]));
    }
    if (dMax < dMin)
    {
        double dTemp = dMax;
        dMax = dMin;
        dMin = dTemp;
    }

    if (nFitVars & 2)
    {
        if (isnan(dMinY))
        {
            if (!vLine.size())
            {
                if (j == 1 && j_pos[1] > j_pos[0])
                    dMinY = _data.min(sDataTable, i_pos[0], i_pos[1], j_pos[0]+1);
                else if (j == 1)
                    dMinY = _data.min(sDataTable, i_pos[0], i_pos[1], j_pos[0]-1);
                else
                    dMinY = _data.min(sDataTable, i_pos[0], i_pos[1], j_pos[1]);
            }
            else
            {
                dMinY = _data.min(sDataTable, vLine, vector<long long int>(1,vCol[1]));
            }
        }
        if (isnan(dMaxY))
        {
            if (!vLine.size())
            {
                if (j == 1 && j_pos[1] > j_pos[0])
                    dMaxY = _data.max(sDataTable, i_pos[0], i_pos[1], j_pos[0]+1);
                else if (j == 1)
                    dMaxY = _data.max(sDataTable, i_pos[0], i_pos[1], j_pos[1]-1);
                else
                    dMaxY = _data.max(sDataTable, i_pos[0], i_pos[1], j_pos[1]);
            }
            else
                dMaxY = _data.max(sDataTable, vLine, vector<long long int>(1,vCol[1]));
        }
        if (dMaxY < dMinY)
        {
            double dTemp = dMaxY;
            dMaxY = dMinY;
            dMinY = dTemp;
        }
    }

    if (nDim == 2)
    {

        if (!vLine.size())
        {
            for (int i = i_pos[0]; i < i_pos[1]; i++)
            {
                /*if (i-i_pos[0]-nSkip == nSize)
                    break;*/

                if (!j)
                {
                    if (_data.isValidEntry(i,j_pos[0], sDataTable))
                    {
                        vx.push_back(i+1);
                        vy.push_back(_data.getElement(i,j_pos[0], sDataTable));
                        /*_fitDataX.a[i-i_pos[0]-nSkip] = i+1;
                        _fitDataY.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_po-s[0], sDataTable);*/
                    }
                    /*else
                        nSkip++;*/
                }
                else
                {
                    if (_data.isValidEntry(i,j_pos[0], sDataTable) && _data.isValidEntry(i,j_pos[1], sDataTable) && sj_pos[1] != "inf")
                    {
                        if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i,j_pos[0], sDataTable) < dMin || _data.getElement(i,j_pos[0], sDataTable) > dMax))
                        {
                            //nSkip++;
                            continue;
                        }
                        vx.push_back(_data.getElement(i,j_pos[0], sDataTable));
                        vy.push_back(_data.getElement(i,j_pos[1], sDataTable));
                        /*_fitDataX.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[0], sDataTable);
                        _fitDataY.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[1], sDataTable);*/
                    }
                    else if (_data.isValidEntry(i,j_pos[0], sDataTable) && _data.isValidEntry(i,j_pos[0]+1, sDataTable) && sj_pos[1] == "inf")
                    {
                        if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i,j_pos[0], sDataTable) < dMin || _data.getElement(i,j_pos[0], sDataTable) > dMax))
                        {
                            //nSkip++;
                            continue;
                        }
                        vx.push_back(_data.getElement(i,j_pos[0], sDataTable));
                        vy.push_back(_data.getElement(i,j_pos[0]+1, sDataTable));
                        /*_fitDataX.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[0], sDataTable);
                        _fitDataY.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[1], sDataTable);*/
                    }
                    /*else
                        nSkip++;*/
                }
            }
        }
        else
        {
            //cerr << vLine.size() << " " << vCol.size() << endl;
            for (unsigned int i = 0; i < vLine.size(); i++)
            {
                /*if (i - nSkip == (unsigned int)nSize)
                    break;*/
                if (!j)
                {
                    if (_data.isValidEntry(vLine[i], vCol[0], sDataTable))
                    {
                        vx.push_back(vLine[i]+1);
                        vy.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
                        /*_fitDataX.a[i-nSkip] = vLine[i]+1;
                        _fitDataY.a[i-nSkip] = _data.getElement(vLine[i], vCol[0], sDataTable);*/
                    }
                    /*else
                        nSkip++;*/
                }
                else
                {
                    if (_data.isValidEntry(vLine[i], vCol[0], sDataTable) && _data.isValidEntry(vLine[i], vCol[1], sDataTable))
                    {
                        if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax))
                        {
                            //nSkip++;
                            continue;
                        }
                        vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
                        vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));
                        /*_fitDataX.a[i-nSkip] = _data.getElement(vLine[i], vCol[0], sDataTable);
                        _fitDataY.a[i-nSkip] = _data.getElement(vLine[i], vCol[1], sDataTable);*/
                        //cerr << _data.getElement(vLine[i], vCol[0], sDataTable) << ", " << _data.getElement(vLine[i], vCol[1], sDataTable) << endl;
                    }
                    /*else
                        nSkip++;*/
                }
            }
        }
        if (paramsMap.size() > vx.size())
            throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
        /*if (!vLine.size())
        {
            if ((int)paramsMap.size() > _data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0])-nSkip)
                throw OVERFITTING_ERROR;
        }
        else
        {
            if ((int)paramsMap.size() > _data.num(sDataTable, vLine, vector<long long int>(1,vCol[0]))-nSkip)
                throw OVERFITTING_ERROR;
        }*/
    }
    else if (nDim == 4)
    {
        if (!vLine.size())
        {
            int nErrorCols = 2;
            if (j == 1)
            {
                if (abs(j_pos[1]-j_pos[0]) == 2)
                    nErrorCols = 1;
            }
            else if (j == 3)
                nErrorCols = 2;
            for (int i = i_pos[0]; i < i_pos[1]; i++)
            {
                /*if (i-i_pos[0]-nSkip == nSize)
                    break;*/
                if (j == 1)
                {
                    if ((_data.isValidEntry(i, j_pos[0], sDataTable) && _data.isValidEntry(i, j_pos[0]+1, sDataTable) && j_pos[0] < j_pos[1])
                        || (_data.isValidEntry(i, j_pos[1], sDataTable) && _data.isValidEntry(i, j_pos[1]-1, sDataTable) && j_pos[1] < j_pos[0]))
                    {
                        if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i,j_pos[0], sDataTable) > dMax))
                        {
                            //nSkip++;
                            continue;
                        }
                        if (j_pos[0] < j_pos[1])
                        {
                            vx.push_back(_data.getElement(i,j_pos[0], sDataTable));
                            vy.push_back(_data.getElement(i,j_pos[0]+1, sDataTable));
                            /*_fitDataX.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[0], sDataTable);
                            _fitDataY.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[0]+1, sDataTable);*/
                            if (nErrorCols == 1)
                            {
                                if (_data.isValidEntry(i,j_pos[0]+2, sDataTable))
                                    vy_w.push_back(fabs(_data.getElement(i,j_pos[0]+2, sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[0]+2, sDataTable));
                                else
                                    vy_w.push_back(0.0);  //_fitErrors.a[i-i_pos[0]-nSkip] = 0.0;
                            }
                            else
                            {
                                if (_data.isValidEntry(i,j_pos[0]+2, sDataTable) && _data.isValidEntry(i,j_pos[0]+3, sDataTable) && (_data.getElement(i,j_pos[0]+2, sDataTable) && _data.getElement(i,j_pos[0]+3, sDataTable)))
                                    vy_w.push_back(sqrt(fabs(_data.getElement(i,j_pos[0]+2, sDataTable)) * fabs(_data.getElement(i,j_pos[0]+3, sDataTable))));  //_fitErrors.a[i-i_pos[0]-nSkip] = sqrt(fabs(_data.getElement(i,j_pos[0]+2, sDataTable)) * fabs(_data.getElement(i,j_pos[0]+3, sDataTable)));
                                else if (_data.isValidEntry(i,j_pos[0]+2, sDataTable) && _data.getElement(i, j_pos[0]+2, sDataTable))
                                    vy_w.push_back(fabs(_data.getElement(i,j_pos[0]+2, sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[0]+2, sDataTable));
                                else if (_data.isValidEntry(i,j_pos[0]+3, sDataTable) && _data.getElement(i, j_pos[0]+3, sDataTable))
                                    vy_w.push_back(fabs(_data.getElement(i,j_pos[0]+3, sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[0]+3, sDataTable));
                                else
                                    vy_w.push_back(0.0);  //_fitErrors.a[i-i_pos[0]-nSkip] = 0.0;
                            }
                        }
                        else
                        {
                            vx.push_back(_data.getElement(i,j_pos[0], sDataTable));
                            vy.push_back(_data.getElement(i,j_pos[0]-1, sDataTable));
                            /*_fitDataX.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[0], sDataTable);
                            _fitDataY.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[0]-1, sDataTable);*/
                            if (nErrorCols == 1)
                            {
                                if (_data.isValidEntry(i,j_pos[0]-2, sDataTable))
                                    vy_w.push_back(fabs(_data.getElement(i,j_pos[0]-2, sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[0]-2, sDataTable));
                                else
                                    vy_w.push_back(0.0);  //_fitErrors.a[i-i_pos[0]+nSkip] = 0.0;
                            }
                            else
                            {
                                if (_data.isValidEntry(i,j_pos[0]-2, sDataTable) && _data.isValidEntry(i,j_pos[0]-3, sDataTable) && (_data.getElement(i,j_pos[0]-2, sDataTable) && _data.getElement(i,j_pos[0]-3, sDataTable)))
                                    vy_w.push_back(sqrt(fabs(_data.getElement(i,j_pos[0]-2, sDataTable)) * fabs(_data.getElement(i,j_pos[0]-3, sDataTable))));  //_fitErrors.a[i-i_pos[0]-nSkip] = sqrt(fabs(_data.getElement(i,j_pos[0]-2, sDataTable)) * fabs(_data.getElement(i,j_pos[0]-3, sDataTable)));
                                else if (_data.isValidEntry(i,j_pos[0]-2, sDataTable) && _data.getElement(i, j_pos[0]-2, sDataTable))
                                    vy_w.push_back(fabs(_data.getElement(i,j_pos[0]-2, sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[0]-2, sDataTable));
                                else if (_data.isValidEntry(i,j_pos[0]-3, sDataTable) && _data.getElement(i, j_pos[0]-3, sDataTable))
                                    vy_w.push_back(fabs(_data.getElement(i,j_pos[0]-3, sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[0]-3, sDataTable));
                                else
                                    vy_w.push_back(0.0);  //_fitErrors.a[i-i_pos[0]-nSkip] = 0.0;
                            }
                        }
                    }
                    /*else
                        nSkip++;*/
                }
                else
                {
                    if (_data.isValidEntry(i, j_pos[0], sDataTable) && _data.isValidEntry(i, j_pos[1], sDataTable))
                    {
                        if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i,j_pos[0], sDataTable) > dMax))
                        {
                            //nSkip++;
                            continue;
                        }
                        vx.push_back(_data.getElement(i,j_pos[0], sDataTable));
                        vy.push_back(_data.getElement(i,j_pos[1], sDataTable));
                        /*_fitDataX.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[0], sDataTable);
                        _fitDataY.a[i-i_pos[0]-nSkip] = _data.getElement(i,j_pos[1], sDataTable);*/
                        if (_data.isValidEntry(i, j_pos[2], sDataTable) && _data.isValidEntry(i, j_pos[3], sDataTable) && (_data.getElement(i, j_pos[2], sDataTable) || _data.getElement(i, j_pos[3], sDataTable)))
                            vy_w.push_back(sqrt(fabs(_data.getElement(i,j_pos[2], sDataTable)) * fabs(_data.getElement(i,j_pos[3], sDataTable))));  //_fitErrors.a[i-i_pos[0]-nSkip] = sqrt(fabs(_data.getElement(i,j_pos[2], sDataTable)) * fabs(_data.getElement(i,j_pos[3], sDataTable)));
                        if (_data.isValidEntry(i, j_pos[2], sDataTable) && _data.getElement(i, j_pos[2], sDataTable))
                            vy_w.push_back(fabs(_data.getElement(i,j_pos[2], sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[2], sDataTable));
                        if (_data.isValidEntry(i, j_pos[3], sDataTable) && _data.getElement(i,j_pos[3], sDataTable))
                            vy_w.push_back(fabs(_data.getElement(i,j_pos[3], sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[3], sDataTable));
                        else
                            vy_w.push_back(0.0);  //_fitErrors.a[i-i_pos[0]-nSkip] = 0.0;
                    }
                    /*else
                        nSkip++;*/
                }
            }
            if (paramsMap.size() > vx.size())//_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0])-nSkip)
                throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
        }
        else
        {
            /*if (_data.num(sDataTable, vLine, vector<long long int>(1,vCol[1])) < nSize)
                nSize = _data.num(sDataTable, vLine, vector<long long int>(1,vCol[1]));

            _fitDataX.Create(nSize);
            _fitDataY.Create(nSize);
            _fitErrors.Create(nSize);
            int nSkip = 0;*/
            int nErrorCols = 2;
            if (j == 1)
            {
                if (abs(vCol[1]-vCol[0]) == 2)
                    nErrorCols = 1;
            }
            else if (j == 3)
                nErrorCols = 2;
            for (unsigned int i = 0; i < vLine.size(); i++)
            {
                /*if (i-nSkip == (unsigned int)nSize)
                    break;*/
                if (j == 1)
                {
                    if (_data.isValidEntry(vLine[i], vCol[0], sDataTable) && _data.isValidEntry(vLine[i], vCol[1], sDataTable))
                    {
                        if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax))
                        {
                            //nSkip++;
                            continue;
                        }

                        vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
                        vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));
                        /*_fitDataX.a[i-nSkip] = _data.getElement(vLine[i], vCol[0], sDataTable);
                        _fitDataY.a[i-nSkip] = _data.getElement(vLine[i], vCol[1], sDataTable);*/
                        if (nErrorCols == 1)
                        {
                            if (_data.isValidEntry(vLine[i], vCol[2], sDataTable))
                                vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)));  //_fitErrors.a[i-nSkip] = fabs(_data.getElement(vLine[i], vCol[2], sDataTable));
                            else
                                vy_w.push_back(0.0);  //_fitErrors.a[i-nSkip] = 0.0;
                        }
                        else
                        {
                            if (_data.isValidEntry(vLine[i], vCol[2], sDataTable) && _data.isValidEntry(vLine[i], vCol[3], sDataTable) && (_data.getElement(vLine[i], vCol[2], sDataTable) && _data.getElement(vLine[i], vCol[3], sDataTable)))
                                vy_w.push_back(sqrt(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)) * fabs(_data.getElement(vLine[i], vCol[3], sDataTable))));  //_fitErrors.a[i-nSkip] = sqrt(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)) * fabs(_data.getElement(vLine[i], vCol[3], sDataTable)));
                            else if (_data.isValidEntry(vLine[i], vCol[2], sDataTable) && _data.getElement(vLine[i], vCol[2], sDataTable))
                                vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)));  //_fitErrors.a[i-nSkip] = fabs(_data.getElement(vLine[i], vCol[2], sDataTable));
                            else if (_data.isValidEntry(vLine[i], vCol[3], sDataTable) && _data.getElement(vLine[i], vCol[3], sDataTable))
                                vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[3], sDataTable)));  //_fitErrors.a[i-nSkip] = fabs(_data.getElement(vLine[i], vCol[3], sDataTable));
                            else
                                vy_w.push_back(0.0);  //_fitErrors.a[i-nSkip] = 0.0;
                        }
                    }
                    /*else
                        nSkip++;*/
                }
                else
                {
                    if (_data.isValidEntry(vLine[i], vCol[0], sDataTable) && _data.isValidEntry(vLine[i], vCol[1], sDataTable))
                    {
                        if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax))
                        {
                            //nSkip++;
                            continue;
                        }
                        vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
                        vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));
                        /*_fitDataX.a[i-nSkip] = _data.getElement(vLine[i], vCol[0], sDataTable);
                        _fitDataY.a[i-nSkip] = _data.getElement(vLine[i], vCol[1], sDataTable);*/
                        if (_data.isValidEntry(vLine[i], vCol[2], sDataTable) && _data.isValidEntry(vLine[i], vCol[3], sDataTable) && (_data.getElement(vLine[i], vCol[2], sDataTable) && _data.getElement(vLine[i], vCol[3], sDataTable)))
                            vy_w.push_back(sqrt(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)) * fabs(_data.getElement(i,vCol[3], sDataTable))));  //_fitErrors.a[i-nSkip] = sqrt(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)) * fabs(_data.getElement(i,vCol[3], sDataTable)));
                        else if (_data.isValidEntry(vLine[i], vCol[2], sDataTable) && _data.getElement(vLine[i], vCol[2], sDataTable))
                            vy_w.push_back(fabs(_data.getElement(vLine[i],vCol[2], sDataTable)));  //_fitErrors.a[i-nSkip] = fabs(_data.getElement(vLine[i],vCol[2], sDataTable));
                        else if (_data.isValidEntry(vLine[i], vCol[3], sDataTable) && _data.getElement(vLine[i],vCol[3], sDataTable))
                            vy_w.push_back(fabs(_data.getElement(vLine[i],vCol[3], sDataTable)));  //_fitErrors.a[i-nSkip] = fabs(_data.getElement(vLine[i],vCol[3], sDataTable));
                        else
                            vy_w.push_back(0.0);  //_fitErrors.a[i-nSkip] = 0.0;
                    }
                    /*else
                        nSkip++;*/
                }
            }
            if (paramsMap.size() > vx.size())//_data.num(sDataTable, vLine, vector<long long int>(1,vCol[0]))-nSkip)
                throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
        }

    }
    else if ((nFitVars & 2))
    {
        if (!vLine.size())
        {
            for (long long int i = i_pos[0]; i < i_pos[1]; i++)
            {
                /*if (i-i_pos[0]-nRowSkip == nSize || i-i_pos[0] - nColSkip == nSize)
                    break;*/
                if (j == 1 && j_pos[1] > j_pos[0])
                {
                    if (!_data.isValidEntry(i,j_pos[0]+1, sDataTable) || _data.getElement(i,j_pos[0]+1, sDataTable) < dMinY || _data.getElement(i,j_pos[0]+1, sDataTable) > dMaxY)
                    {
                        //continue;
                        //nColSkip++;
                    }
                    else
                        vy.push_back(_data.getElement(i,j_pos[0]+1, sDataTable));  //_fitDataY.a[i-i_pos[0]-nColSkip] = _data.getElement(i,j_pos[0]+1, sDataTable);
                }
                else if (j == 1)
                {
                    if (!_data.isValidEntry(i,j_pos[0]-1, sDataTable) || _data.getElement(i,j_pos[0]-1, sDataTable) < dMinY || _data.getElement(i,j_pos[0]-1, sDataTable) > dMaxY)
                    {
                        //continue;
                        //nColSkip++;
                    }
                    else
                        vy.push_back(_data.getElement(i,j_pos[0]-1, sDataTable));  //_fitDataY.a[i-i_pos[0]-nColSkip] = _data.getElement(i,j_pos[0]-1, sDataTable);
                }
                else
                {
                    if (!_data.isValidEntry(i,j_pos[1], sDataTable) || _data.getElement(i,j_pos[1], sDataTable) < dMinY || _data.getElement(i,j_pos[1], sDataTable) > dMaxY)
                    {
                        //continue;
                        //nColSkip++;
                    }
                    else
                        vy.push_back(_data.getElement(i,j_pos[1], sDataTable));  //_fitDataY.a[i-i_pos[0]-nColSkip] = _data.getElement(i,j_pos[1], sDataTable);
                }
                if (!_data.isValidEntry(i,j_pos[0], sDataTable) || _data.getElement(i,j_pos[0], sDataTable) < dMin || _data.getElement(i,j_pos[0], sDataTable) > dMax)
                {
                    //nRowSkip++;
                    continue;
                }
                else
                    vx.push_back(_data.getElement(i,j_pos[0], sDataTable));  //_fitDataX.a[i-i_pos[0]-nRowSkip] = _data.getElement(i,j_pos[0], sDataTable);

                if (j == 1 && j_pos[1] > j_pos[0])
                {
                    //long long int nSkip = 0;
                    for (long long int k = j_pos[0]+2; k < j_pos[0]+i_pos[1]-i_pos[0]+2; k++)
                    {
                        if (!_data.isValidEntry(k-j_pos[0]-2+i_pos[0],j_pos[0]+1, sDataTable) || _data.getElement(k-j_pos[0]-2+i_pos[0], j_pos[0]+1, sDataTable) < dMinY || _data.getElement(k-j_pos[0]-2+i_pos[0],j_pos[0]+1, sDataTable) > dMaxY)
                        {
                            continue;
                            //nSkip++;
                        }
                        else
                        {
                            vTempZ.push_back(_data.getElement(i,k, sDataTable));
                            //_fitDataZ.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k, sDataTable);
                            if (bUseErrors && _data.isValidEntry(i,k+i_pos[1]-i_pos[0], sDataTable))
                                vy_w.push_back(_data.getElement(i,k+i_pos[1]-i_pos[0], sDataTable));  //_fitErrors.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k+i_pos[1]-i_pos[0], sDataTable);
                            else if (bUseErrors)
                                vy_w.push_back(0.0);  //_fitErrors.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = 0.0;
                        }
                    }
                }
                else if (j == 1)
                {
                    //long long int nSkip = 0;
                    for (long long int k = j_pos[0]-2; k > j_pos[0]-i_pos[1]+i_pos[0]-2; k--)
                    {
                        if (k < 0)
                            break;
                        if (!_data.isValidEntry(i_pos[0]-(k-j_pos[0]+2),j_pos[0]-1, sDataTable) || _data.getElement(i_pos[0]-(k-j_pos[0]+2),j_pos[0]-1, sDataTable) < dMinY || _data.getElement(i_pos[0]-(k-j_pos[0]+2),j_pos[0]-1, sDataTable) > dMaxY)
                        {
                            continue;
                            //nSkip++;
                        }
                        else
                        {
                            vTempZ.push_back(_data.getElement(i,k, sDataTable));
                            //_fitDataZ.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k, sDataTable);
                            if (bUseErrors && k-i_pos[1]+i_pos[0] >= 0 && _data.isValidEntry(i,k-i_pos[1]+i_pos[0], sDataTable))
                                vy_w.push_back(_data.getElement(i,k-i_pos[1]+i_pos[0], sDataTable));  //_fitErrors.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k-i_pos[1]+i_pos[0], sDataTable);
                            else if (bUseErrors)
                                vy_w.push_back(0.0);  //_fitErrors.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = 0.0;
                        }
                    }
                }
                else
                {
                    //long long int nSkip = 0;
                    for (long long int k = j_pos[2]; k < j_pos[2]+i_pos[1]-i_pos[0]; k++)
                    {
                        if (j > 2 && k == j_pos[3])
                            break;
                        if (!_data.isValidEntry(k-j_pos[2]+i_pos[0],j_pos[1], sDataTable) || _data.getElement(k-j_pos[2]+i_pos[0],j_pos[1], sDataTable) < dMinY || _data.getElement(k-j_pos[2]+i_pos[0],j_pos[1], sDataTable) > dMaxY)
                        {
                            continue;
                            //nSkip++;
                        }
                        else
                        {
                            vTempZ.push_back(_data.getElement(i,k, sDataTable));
                            //_fitDataZ.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k, sDataTable);
                            if (bUseErrors && j > 2 && _data.isValidEntry(i,k+j_pos[3], sDataTable))
                                vy_w.push_back(_data.getElement(i,k+j_pos[3], sDataTable));  //_fitErrors.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k+j_pos[3], sDataTable);
                            else if (bUseErrors && _data.isValidEntry(i,k+i_pos[1]-i_pos[0], sDataTable))
                                vy_w.push_back(_data.getElement(i,k+i_pos[1]-i_pos[0], sDataTable));  //_fitErrors.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k+i_pos[1]-i_pos[0], sDataTable);
                            else if (bUseErrors)
                                vy_w.push_back(0.0);  //_fitErrors.a[(i-i_pos[0]-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = 0.0;
                        }
                    }
                }
                vz.push_back(vTempZ);
                vTempZ.clear();
                if (vy_w.size() && bUseErrors)
                {
                    vz_w.push_back(vy_w);
                    vy_w.clear();
                }

            }
            if (paramsMap.size() > vz.size()//_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0])-nRowSkip
                || paramsMap.size() > vz[0].size())//_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0])-nColSkip)
                throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
        }
        else
        {
            for (long long int i = 0; i < vLine.size(); i++)
            {
                /*if (i-nRowSkip == nSize || i - nColSkip == nSize)
                    break;*/

                if (!_data.isValidEntry(vLine[i], vCol[1], sDataTable) || _data.getElement(vLine[i], vCol[1], sDataTable) < dMinY || _data.getElement(vLine[i], vCol[1], sDataTable) > dMaxY)
                {
                    //continue;
                    //nColSkip++;
                }
                else
                    vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));  //_fitDataY.a[i-nColSkip] = _data.getElement(vLine[i], vCol[1], sDataTable);

                if (!_data.isValidEntry(vLine[i], vCol[0], sDataTable) || _data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax)
                {
                    //nRowSkip++;
                    continue;
                }
                else
                    vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable)); //_fitDataX.a[i-nRowSkip] = _data.getElement(vLine[i], vCol[0], sDataTable);

                if (j == 1 && j_pos[1] > j_pos[0])
                {
                    //long long int nSkip = 0;
                    for (long long int k = j_pos[0]+2; k < j_pos[0]+i_pos[1]-i_pos[0]+2; k++)
                    {
                        if (!_data.isValidEntry(k-j_pos[0]-2+i_pos[0],j_pos[0]+1, sDataTable) || _data.getElement(k-j_pos[0]-2+i_pos[0],j_pos[0]+1, sDataTable) < dMinY || _data.getElement(k-j_pos[0]-2+i_pos[0],j_pos[0]+1, sDataTable) > dMaxY)
                        {
                            continue;
                            //nSkip++;
                        }
                        else
                        {
                            vTempZ.push_back(_data.getElement(i,k, sDataTable));
                            //_fitDataZ.a[(i-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k, sDataTable);
                            if (bUseErrors && _data.isValidEntry(i,k+i_pos[1]-i_pos[0], sDataTable))
                                vy_w.push_back(_data.getElement(i,k+i_pos[1]-i_pos[0], sDataTable));  //_fitErrors.a[(i-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k+i_pos[1]-i_pos[0], sDataTable);
                            else if (bUseErrors)
                                vy_w.push_back(0.0);  //_fitErrors.a[(i-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = 0.0;
                        }
                    }
                }
                else if (j == 1)
                {
                    //long long int nSkip = 0;
                    for (long long int k = j_pos[0]-2; k > j_pos[0]-i_pos[1]+i_pos[0]-2; k--)
                    {
                        if (k < 0)
                            break;
                        if (!_data.isValidEntry(i_pos[0]-(k-j_pos[0]+2),j_pos[0]-1, sDataTable) || _data.getElement(i_pos[0]-(k-j_pos[0]+2),j_pos[0]-1, sDataTable) < dMinY || _data.getElement(i_pos[0]-(k-j_pos[0]+2),j_pos[0]-1, sDataTable) > dMaxY)
                        {
                            continue;
                            //nSkip++;
                        }
                        else
                        {
                            vTempZ.push_back(_data.getElement(i,k, sDataTable));
                            //_fitDataZ.a[(i-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k, sDataTable);
                            if (bUseErrors && k-i_pos[1]+i_pos[0] >= 0 && _data.isValidEntry(i,k-i_pos[1]+i_pos[0], sDataTable))
                                vy_w.push_back(_data.getElement(i,k-i_pos[1]+i_pos[0], sDataTable));  //_fitErrors.a[(i-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = _data.getElement(i,k-i_pos[1]+i_pos[0], sDataTable);
                            else if (bUseErrors)
                                vy_w.push_back(0.0);  //_fitErrors.a[(i-nRowSkip) + (k-j_pos[0]-2-nSkip)*nSize] = 0.0;
                        }
                    }
                }
                else
                {
                    //long long int nSkip = 0;
                    for (long long int k = vCol[2]; k < vCol.size(); k++)
                    {
                        if (j > 2 && k == vLine.size()+2)
                            break;
                        if (!_data.isValidEntry(vLine[k], vCol[1], sDataTable)
                            || _data.getElement(vLine[k], vCol[1], sDataTable) < dMinY
                            || _data.getElement(vLine[k], vCol[1], sDataTable) > dMaxY)
                        {
                            continue;
                            //nSkip++;
                        }
                        else
                        {
                            vTempZ.push_back(_data.getElement(vLine[i], vCol[k], sDataTable));
                        }
                    }
                }
                vz.push_back(vTempZ);
                vTempZ.clear();
                if (vy_w.size() && bUseErrors)
                {
                    vz_w.push_back(vy_w);
                    vy_w.clear();
                }
            }
            if (paramsMap.size() > vz.size()//_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0])-nRowSkip
                || paramsMap.size() > vz[0].size())//_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0])-nColSkip)
                throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
        }
    }
    else
    {
        if (!vLine.size())
        {
            for (int i = i_pos[0]; i < i_pos[1]; i++)
            {
                if (_data.isValidEntry(i, j_pos[0], sDataTable) && _data.isValidEntry(i, j_pos[1], sDataTable))
                {
                    if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i,j_pos[0], sDataTable) > dMax))
                    {
                        //nSkip++;
                        continue;
                    }
                    vx.push_back(_data.getElement(i,j_pos[0], sDataTable));
                    vy.push_back(_data.getElement(i,j_pos[1], sDataTable));

                    if (_data.isValidEntry(i,j_pos[2], sDataTable))
                        vy_w.push_back(fabs(_data.getElement(i,j_pos[2], sDataTable)));  //_fitErrors.a[i-i_pos[0]-nSkip] = fabs(_data.getElement(i,j_pos[2], sDataTable));
                    else
                        vy_w.push_back(0.0);  //_fitErrors.a[i-i_pos[0]-nSkip] = 0.0;
                }
                /*else
                    nSkip++;*/
            }
            if (paramsMap.size() > vy.size())//_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0])-nSkip)
                throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
        }
        else
        {
            for (unsigned int i = 0; i < vLine.size(); i++)
            {
                /*if (i-nSkip == (unsigned int)nSize)
                    break;*/
                if (_data.isValidEntry(vLine[i], vCol[0], sDataTable) && _data.isValidEntry(vLine[i], vCol[1], sDataTable))
                {
                    if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax))
                    {
                        //nSkip++;
                        continue;
                    }
                    vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
                    vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));
                    /*_fitDataX.a[i-nSkip] = _data.getElement(vLine[i], vCol[0], sDataTable);
                    _fitDataY.a[i-nSkip] = _data.getElement(vLine[i], vCol[1], sDataTable);*/
                    if (_data.isValidEntry(vLine[i], vCol[2], sDataTable))
                        vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)));  //_fitErrors.a[i-nSkip] = fabs(_data.getElement(vLine[i], vCol[2], sDataTable));
                    else
                        vy_w.push_back(0.0);  //_fitErrors.a[i-nSkip] = 0.0;
                }
                /*else
                    nSkip++;*/
            }
            if (paramsMap.size() > vy.size())//_data.num(sDataTable, vLine, vector<long long int>(1,vCol[0]))-nSkip)
                throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
        }
    }
    //cerr << nSize << endl;

    if (paramsMap.size() > vx.size())//nSize)
        throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);

    // Überzählige Klammern (durch Fit(x)) entfernen
    while (sFuncDisplay.front() == '(')
    {
        if (getMatchingParenthesis(sFuncDisplay) == sFuncDisplay.length()-1 && getMatchingParenthesis(sFuncDisplay) != string::npos)
        {
            sFuncDisplay.erase(0,1);
            sFuncDisplay.pop_back();
            StripSpaces(sFuncDisplay);
        }
        else
            break;
    }
    StripSpaces(sFitFunction);
    while (sFitFunction.front() == '(')
    {
        if (getMatchingParenthesis(sFitFunction) == sFitFunction.length()-1 && getMatchingParenthesis(sFitFunction) != string::npos)
        {
            sFitFunction.erase(0,1);
            sFitFunction.pop_back();
            StripSpaces(sFitFunction);
        }
        else
            break;
    }

    if (_option.getSystemPrintStatus())
        NumeReKernel::printPreFmt(LineBreak("|-> "+_lang.get("PARSERFUNCS_FIT_FITTING", sFuncDisplay)+" ", _option, 0));

    for (auto iter = paramsMap.begin(); iter != paramsMap.end(); ++iter)
    {
        vInitialVals.push_back(*(iter->second));
    }
    if (sChiMap.length())
    {
        Fitcontroller _fControl(&_parser);

        if (!_idx.vI.size())
        {
            for (long long int i = _idx.nI[0]; i < _idx.nI[1]; i++)
            {
                for (long long int j = _idx.nI[0]; j <= (_idx.nI[1]-1)*(!b1DChiMap)+_idx.nI[0]*(b1DChiMap); j++)
                {
                    auto iter = paramsMap.begin();
                    for (unsigned int n = 0; n < vInitialVals.size(); n++)
                    {
                        *(iter->second) = vInitialVals[n];
                        ++iter;
                    }
                    if (!_data.isValidEntry(i, _idx.nJ[0], sChiMap))
                        continue;
                    *(varMap.at(sChiMap_Vars[0])) = _data.getElement(i, _idx.nJ[0], sChiMap);
                    if (!b1DChiMap && _idx.nJ[0] < _idx.nJ[1])
                    {
                        if (!_data.isValidEntry(i, _idx.nJ[0]+1, sChiMap))
                            continue;
                        *(varMap.at(sChiMap_Vars[1])) = _data.getElement(j, _idx.nJ[0]+1, sChiMap);
                    }
                    else if (!b1DChiMap)
                    {
                        if (!_data.isValidEntry(i, _idx.nJ[0]-1, sChiMap))
                            continue;
                        *(varMap.at(sChiMap_Vars[1])) = _data.getElement(j, _idx.nJ[0]-1, sChiMap);
                    }
                    if (nDim >= 2 && nFitVars == 1)
                    {
                        if (!bUseErrors)
                        {
                            if (!_fControl.fit(vx, vy, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
                            {
                                if (_option.getSystemPrintStatus())
                                    NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                                return false;
                            }
                            sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                        }
                        else
                        {
                            if (!_fControl.fit(vx, vy, vy_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
                            {
                                if (_option.getSystemPrintStatus())
                                    NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                                return false;
                            }
                            sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                        }
                    }
                    else if (nDim == 3)
                    {
                        if (!_fControl.fit(vx, vy, vz, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
                        {
                            if (_option.getSystemPrintStatus())
                                NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                            return false;
                        }
                        sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                    }
                    else if (nDim == 5)
                    {
                        if (!_fControl.fit(vx, vy, vz, vz_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
                        {
                            if (_option.getSystemPrintStatus())
                                NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                            return false;
                        }
                        sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                    }
                    //if (_idx.nJ[0]+1+(!b1DChiMap)*(j+1) >= _idx.nJ[1])
                    // break;
                    if (_idx.nJ[0] < _idx.nJ[1])
                    {
                        _data.writeToCache(i, _idx.nJ[0]+1+(!b1DChiMap)*(j-_idx.nI[0]+1), sChiMap, _fControl.getFitChi());
                        if (i == _idx.nI[0] && !b1DChiMap)
                            _data.setHeadLineElement(_idx.nJ[0]+1+(!b1DChiMap)*(j-_idx.nI[0]+1), sChiMap, "chi^2["+toString(j-_idx.nI[0]+1)+"]");
                        else if (i == _idx.nI[0])
                            _data.setHeadLineElement(_idx.nJ[0]+1+(!b1DChiMap)*(j-_idx.nI[0]+1), sChiMap, "chi^2");
                    }
                    else
                    {
                        _data.writeToCache(i, _idx.nJ[0]-1-(!b1DChiMap)*(j-_idx.nI[0]+1), sChiMap, _fControl.getFitChi());
                        if (i == _idx.nI[0] && !b1DChiMap)
                            _data.setHeadLineElement(_idx.nJ[0]-1-(!b1DChiMap)*(j-_idx.nI[0]+1), sChiMap, "chi^2["+toString(j-_idx.nI[0]+1)+"]");
                        else if (i == _idx.nI[0])
                            _data.setHeadLineElement(_idx.nJ[0]-1-(!b1DChiMap)*(j-_idx.nI[0]+1), sChiMap, "chi^2");
                    }
                }
            }
        }
        else
        {
            for (long long int i = 0; i < _idx.vI.size(); i++)
            {
                for (long long int j = 0; j <= (_idx.vI.size()-1)*(!b1DChiMap); j++)
                {
                    auto iter = paramsMap.begin();
                    for (unsigned int n = 0; n < vInitialVals.size(); n++)
                    {
                        *(iter->second) = vInitialVals[n];
                        ++iter;
                    }
                    if (!_data.isValidEntry(_idx.vI[i], _idx.vJ[0], sChiMap))
                        continue;
                    *(varMap.at(sChiMap_Vars[0])) = _data.getElement(_idx.vI[i], _idx.vJ[0], sChiMap);
                    if (!b1DChiMap)
                    {
                        if (!_data.isValidEntry(_idx.vI[j], _idx.vJ[1], sChiMap))
                            continue;
                        *(varMap.at(sChiMap_Vars[1])) = _data.getElement(_idx.vI[j], _idx.vJ[1], sChiMap);
                    }
                    if (nDim >= 2 && nFitVars == 1)
                    {
                        if (!bUseErrors)
                        {
                            if (!_fControl.fit(vx, vy, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
                            {
                                if (_option.getSystemPrintStatus())
                                    NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                                return false;
                            }
                            sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                        }
                        else
                        {
                            if (!_fControl.fit(vx, vy, vy_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
                            {
                                if (_option.getSystemPrintStatus())
                                    NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                                return false;
                            }
                            sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                        }
                    }
                    else if (nDim == 3)
                    {
                        if (!_fControl.fit(vx, vy, vz, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
                        {
                            if (_option.getSystemPrintStatus())
                                NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                            return false;
                        }
                        sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                    }
                    else if (nDim == 5)
                    {
                        if (!_fControl.fit(vx, vy, vz, vz_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
                        {
                            if (_option.getSystemPrintStatus())
                                NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                            return false;
                        }
                        sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                    }
                    //if (_idx.nJ[0]+1+(!b1DChiMap)*(j+1) >= _idx.nJ[1])
                    // break;
                    _data.writeToCache(_idx.vI[i], _idx.vJ[1+(!b1DChiMap)*(j+1)], sChiMap, _fControl.getFitChi());
                    if (!i && !b1DChiMap)
                        _data.setHeadLineElement(_idx.vJ[1+(!b1DChiMap)*(j+1)], sChiMap, "chi^2["+toString(j+1)+"]");
                    else if (!i)
                        _data.setHeadLineElement(_idx.vJ[1+(!b1DChiMap)*(j+1)], sChiMap, "chi^2");

                }
            }
        }
        auto iter = paramsMap.begin();
        for (unsigned int n = 0; n < vInitialVals.size(); n++)
        {
            *(iter->second) = vInitialVals[n];
            ++iter;
        }
        if (_option.getSystemPrintStatus())
        {
            NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
            NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_CHIMAPLOCATION", sChiMap), _option));
            //cerr << LineBreak("|-> Die chi^2-Map wurde erfolgreich in " + sChiMap + "() angelegt.", _option) << endl;
        }
        if (!_functions.isDefined(sFunctionDefString))
            _functions.defineFunc(sFunctionDefString, _parser, _option);
        else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) != sFunctionDefString)
            _functions.defineFunc(sFunctionDefString, _parser, _option, true);

        return true;
    }

    Fitcontroller _fControl(&_parser);

    if (nDim >= 2 && nFitVars == 1)
    {
        if (!bUseErrors)
        {
            if (!_fControl.fit(vx, vy, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                return false;
            }
            //_graph.Fit(_fitDataX, _fitDataY, sFitFunction.c_str(), sParams.c_str(), _fitParams);
            sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
            /*if (!_functions.isDefined("Fit"))
                _functions.defineFunc("Fit(x) := "+sFuncDisplay + " -set comment=\"Angepasste Funktion\"", _parser, _option);
            else
                _functions.defineFunc("Fit(x) := "+sFuncDisplay + " -set comment=\"Angepasste Funktion\"", _parser, _option, true);*/
        }
        else
        {
            if (!_fControl.fit(vx, vy, vy_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
                return false;
            }
            //_graph.FitS(_fitDataX, _fitDataY, _fitErrors, sFitFunction.c_str(), sParams.c_str(), _fitParams);
            sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
            /*if (!_functions.isDefined("Fitw"))
                _functions.defineFunc("Fitw(x) := "+sFuncDisplay + " -set comment=\"Angepasste Funktion\"", _parser, _option);
            else
                _functions.defineFunc("Fitw(x) := "+sFuncDisplay + " -set comment=\"Angepasste Funktion\"", _parser, _option, true);*/

        }
    }
    else if (nDim == 3)
    {
        if (!_fControl.fit(vx, vy, vz, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
            return false;
        }
        //_graph.Fit(_fitDataX, _fitDataY, _fitDataZ, sFitFunction.c_str(), sParams.c_str(), _fitParams);
        sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
        /*if (!_functions.isDefined("Fit"))
            _functions.defineFunc("Fit(x,y) := "+sFuncDisplay + " -set comment=\"Angepasste Funktion\"", _parser, _option);
        else
            _functions.defineFunc("Fit(x,y) := "+sFuncDisplay + " -set comment=\"Angepasste Funktion\"", _parser, _option, true);*/
    }
    else if (nDim == 5)
    {
        if (!_fControl.fit(vx, vy, vz, vz_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
        {
            if (_option.getSystemPrintStatus())
                NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
            return false;
        }
        //_graph.Fit(_fitDataX, _fitDataY, _fitDataZ, _fitErrors, sFitFunction.c_str(), sParams.c_str(), _fitParams);
        sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
        /*if (!_functions.isDefined("Fitw"))
            _functions.defineFunc("Fitw(x,y) := "+sFuncDisplay + " -set comment=\"Angepasste Funktion\"", _parser, _option);
        else
            _functions.defineFunc("Fitw(x,y) := "+sFuncDisplay + " -set comment=\"Angepasste Funktion\"", _parser, _option, true);*/
    }

    vz_w = _fControl.getCovarianceMatrix();
    dChisq = _fControl.getFitChi();

    dNormChisq = dChisq;

    unsigned int nSize = ((vz.size()) ? (vz.size()*vz[0].size()) : vx.size());
    if (!bUseErrors && !(nFitVars & 2))
    {
        for (unsigned int i = 0; i < vz_w.size(); i++)
        {
            for (unsigned int j = 0; j < vz_w[0].size(); j++)
            {
                vz_w[i][j] *= dChisq / (nSize - paramsMap.size());
            }
        }
    }//_fitParamErrors *= dChisq / (nSize - _fitParams.GetNx());
    else if (!bUseErrors)
    {
        for (unsigned int i = 0; i < vz_w.size(); i++)
        {
            for (unsigned int j = 0; j < vz_w[0].size(); j++)
            {
                vz_w[i][j] *= dChisq / (nSize*nSize - paramsMap.size());
            }
        }
    }//    _fitParamErrors *= dChisq / (nSize*nSize - _fitParams.GetNx());

    if (!bMaskDialog && _option.getSystemPrintStatus())
        reduceLogFilesize(sFitLog);
    sFittedFunction = _fControl.getFitFunction(); //_graph.GetFit();
    oFitLog.open(sFitLog.c_str(), ios_base::ate | ios_base::app);
    if (oFitLog.fail())
    {
        oFitLog.close();
        _data.setCacheStatus(false);
        NumeReKernel::printPreFmt("\n");
        throw SyntaxError(SyntaxError::CANNOT_OPEN_FITLOG, sCmd, SyntaxError::invalid_position);
    }
    if (bTeXExport)
    {
        oTeXExport.open(sTeXExportFile.c_str(), ios_base::trunc);
        if (oTeXExport.fail())
        {
            oTeXExport.close();
            _data.setCacheStatus(false);
            NumeReKernel::printPreFmt("\n");
            //sErrorToken = sTeXExportFile;
            throw SyntaxError(SyntaxError::CANNOT_OPEN_TARGET, sCmd, SyntaxError::invalid_position, sTeXExportFile);
        }
    }
    ///FITLOG
    oFitLog << std::setw(76) << std::setfill('=') << '=' << endl;
    oFitLog << toUpperCase(_lang.get("PARSERFUNCS_FIT_HEADLINE")) <<": " << getTimeStamp(false) << endl;
    oFitLog << std::setw(76) << std::setfill('=') << '=' << endl;
    oFitLog << (_lang.get("PARSERFUNCS_FIT_FUNCTION", sFuncDisplay)) << endl;
    oFitLog << (_lang.get("PARSERFUNCS_FIT_FITTED_FUNC", sFittedFunction)) << endl;
    oFitLog << (_lang.get("PARSERFUNCS_FIT_DATASET")) << " ";
    if (nDim == 2)
    {
        oFitLog << j_pos[0]+1;
        if (j)
        {
            oFitLog << ", " << j_pos[1]+1;
        }
    }
    else if (nDim == 4)
    {
        int nErrorCols = 2;
        if (j == 1)
        {
            if (abs(j_pos[1]-j_pos[0]) == 3)
                nErrorCols = 1;
        }
        else if (j == 3)
            nErrorCols = 2;

        if (j == 1)
        {
            if (j_pos[0] < j_pos[1])
            {
                oFitLog << j_pos[0]+1 << ", " << j_pos[0]+2 << ", " << j_pos[0]+3;
                if (nErrorCols == 2)
                    oFitLog << ", " << j_pos[0]+4;
            }
            else
            {
                oFitLog << j_pos[0]+1 << ", " << j_pos[0] << ", " << j_pos[0]-1;
                if (nErrorCols == 2)
                    oFitLog << ", " << j_pos[0]-2;
            }
        }
        else
        {
            oFitLog << j_pos[0]+1 << ", " << j_pos[1]+1 << ", " << j_pos[2]+1 << ", " << j_pos[3]+1;
        }
    }
    else if ((nFitVars & 2))
    {
        if (j == 1 && j_pos[1] > j_pos[0])
        {
            oFitLog << j_pos[0]+1 << ", " << j_pos[0]+2 << ", " << j_pos[0]+3 << "-" << j_pos[0]+2+i_pos[1]-i_pos[0];
            if (bUseErrors)
                oFitLog << ", " << j_pos[2]+3+i_pos[1]-i_pos[0] << "-" << j_pos[0]+2+2*(i_pos[1]-i_pos[0]);
        }
        else if (j == 1)
        {
            oFitLog << j_pos[0]+1 << ", " << j_pos[0] << ", " << j_pos[0]-1 << "-" << j_pos[0]-2-i_pos[1]+i_pos[0];
            if (bUseErrors)
                oFitLog << ", " << j_pos[2]-3-i_pos[1]+i_pos[0] << "-" << j_pos[0]-2-2*(i_pos[1]-i_pos[0]);
        }
        else
        {
            oFitLog << j_pos[0]+1 << ", " << j_pos[1]+1 << ", " << j_pos[2]+1 << "-" << j_pos[2]+i_pos[1]-i_pos[0];
            if (bUseErrors)
            {
                if (j > 2)
                    oFitLog << ", " << j_pos[3]+1 << "-" << j_pos[3]+(i_pos[1]-i_pos[0]);
                else
                    oFitLog << ", " << j_pos[2]+i_pos[1]-i_pos[0]+1 << "-" << j_pos[0]+2*(i_pos[1]-i_pos[0]);
            }
        }
    }
    else
    {
        for (int k = 0; k < (int)nDim; k++)
        {
            oFitLog << j_pos[k]+1;
            if (k+1 < (int)nDim)
                oFitLog << ", ";
        }
    }
    oFitLog << " " << _lang.get("PARSERFUNCS_FIT_FROM") << " " << _data.getDataFileName(sDataTable) << endl;
    if (bUseErrors)
        oFitLog << (_lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString((int)nSize))) << endl;
    else
        oFitLog << (_lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString((int)nSize))) << endl;
    if (bRestrictXVals)
        oFitLog << (_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(dMin, 5), toString(dMax, 5))) << endl;
    if (bRestrictYVals)
        oFitLog << (_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(dMinY, 5), toString(dMaxY, 5))) << endl;
    if (sRestrictions.length())
        oFitLog << _lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", sRestrictions) << endl;
    oFitLog << _lang.get("PARSERFUNCS_FIT_FREEDOMS", toString((int)nSize - paramsMap.size())) << endl;
    oFitLog << _lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(dPrecision, 5), toString(nMaxIterations)) << endl;
    oFitLog << _lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations())) << endl;
    if (nSize != paramsMap.size() /*_fitParams.GetNx()*/ && !(nFitVars & 2))
    {
        oFitLog << _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7)) << endl;
        oFitLog << _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7)) << endl;
        oFitLog << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
    }
    else if (nFitVars & 2 && nSize != paramsMap.size() /*_fitParams.GetNx()*/)
    {
        oFitLog << _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7)) << endl;
        oFitLog << _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7)) << endl;
        oFitLog << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
    }
    //oFitLog << "Normierte Varianz der Residuen:         " << dNormChisq / (double)(nSize - _fitParams.GetNx()) << endl;
    oFitLog << endl;
    if (bUseErrors)
        oFitLog << _lang.get("PARSERFUNCS_FIT_LOG_TABLEHEAD1") << endl;
    else
        oFitLog << _lang.get("PARSERFUNCS_FIT_LOG_TABLEHEAD2") << endl;
    oFitLog << std::setw(76) << std::setfill('-') << '-' << endl;

    ///TEXEXPORT
    if (bTeXExport)
    {
        oTeXExport << "%\n% " << _lang.get("OUTPUT_PRINTLEGAL_TEX") << "\n%" << endl;
        oTeXExport << "\\section{" << _lang.get("PARSERFUNCS_FIT_HEADLINE") <<": " << getTimeStamp(false)  << "}" << endl;
        oTeXExport << "\\begin{itemize}" << endl;
        oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_FUNCTION", "$" + replaceToTeX(sFuncDisplay, true) + "$")) << endl;
        oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_FITTED_FUNC", "$" + replaceToTeX(sFittedFunction, true) + "$")) << endl;
        //oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_DATASET")) << " ";
        /*if (nDim == 2)
        {
            oFitLog << j_pos[0]+1;
            if (j)
            {
                oFitLog << ", " << j_pos[1]+1;
            }
        }
        else if (nDim == 4)
        {
            int nErrorCols = 2;
            if (j == 1)
            {
                if (abs(j_pos[1]-j_pos[0]) == 3)
                    nErrorCols = 1;
            }
            else if (j == 3)
                nErrorCols = 2;

            if (j == 1)
            {
                if (j_pos[0] < j_pos[1])
                {
                    oFitLog << j_pos[0]+1 << ", " << j_pos[0]+2 << ", " << j_pos[0]+3;
                    if (nErrorCols == 2)
                        oFitLog << ", " << j_pos[0]+4;
                }
                else
                {
                    oFitLog << j_pos[0]+1 << ", " << j_pos[0] << ", " << j_pos[0]-1;
                    if (nErrorCols == 2)
                        oFitLog << ", " << j_pos[0]-2;
                }
            }
            else
            {
                oFitLog << j_pos[0]+1 << ", " << j_pos[1]+1 << ", " << j_pos[2]+1 << ", " << j_pos[3]+1;
            }
        }
        else if ((nFitVars & 2))
        {
            if (j == 1 && j_pos[1] > j_pos[0])
            {
                oFitLog << j_pos[0]+1 << ", " << j_pos[0]+2 << ", " << j_pos[0]+3 << "-" << j_pos[0]+2+i_pos[1]-i_pos[0];
                if (bUseErrors)
                    oFitLog << ", " << j_pos[2]+3+i_pos[1]-i_pos[0] << "-" << j_pos[0]+2+2*(i_pos[1]-i_pos[0]);
            }
            else if (j == 1)
            {
                oFitLog << j_pos[0]+1 << ", " << j_pos[0] << ", " << j_pos[0]-1 << "-" << j_pos[0]-2-i_pos[1]+i_pos[0];
                if (bUseErrors)
                    oFitLog << ", " << j_pos[2]-3-i_pos[1]+i_pos[0] << "-" << j_pos[0]-2-2*(i_pos[1]-i_pos[0]);
            }
            else
            {
                oFitLog << j_pos[0]+1 << ", " << j_pos[1]+1 << ", " << j_pos[2]+1 << "-" << j_pos[2]+i_pos[1]-i_pos[0];
                if (bUseErrors)
                {
                    if (j > 2)
                        oFitLog << ", " << j_pos[3]+1 << "-" << j_pos[3]+(i_pos[1]-i_pos[0]);
                    else
                        oFitLog << ", " << j_pos[2]+i_pos[1]-i_pos[0]+1 << "-" << j_pos[0]+2*(i_pos[1]-i_pos[0]);
                }
            }
        }
        else
        {
            for (int k = 0; k < (int)nDim; k++)
            {
                oFitLog << j_pos[k]+1;
                if (k+1 < (int)nDim)
                    oFitLog << ", ";
            }
        }
        oFitLog << " " << _lang.get("PARSERFUNCS_FIT_FROM") << " " << _data.getDataFileName(sDataTable) << endl;*/
        if (bUseErrors)
            oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString((int)nSize))) << endl;
        else
            oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString((int)nSize))) << endl;
        if (bRestrictXVals)
            oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(dMin, 5), toString(dMax, 5))) << endl;
        if (bRestrictYVals)
            oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(dMinY, 5), toString(dMaxY, 5))) << endl;
        if (sRestrictions.length())
            oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", "$" + replaceToTeX(sRestrictions, true) + "$") << endl;
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_FREEDOMS", toString((int)nSize - paramsMap.size())) << endl;
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(dPrecision, 5), toString(nMaxIterations)) << endl;
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations())) << endl;
        if (nSize != paramsMap.size() /*_fitParams.GetNx()*/ && !(nFitVars & 2))
        {
            string sChiReplace = _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7));
            sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
            oTeXExport << "\t\\item " << sChiReplace << endl;
            sChiReplace = _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7));
            sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
            oTeXExport << "\t\\item " << sChiReplace << endl;
            oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
        }
        else if (nFitVars & 2 && nSize != paramsMap.size() /*_fitParams.GetNx()*/)
        {
            string sChiReplace = _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7));
            sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
            oTeXExport << "\t\\item " << sChiReplace << endl;
            sChiReplace = _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7));
            sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
            oTeXExport << "\t\\item " << sChiReplace << endl;
            oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
        }
        //oFitLog << "Normierte Varianz der Residuen:         " << dNormChisq / (double)(nSize - _fitParams.GetNx()) << endl;
        oTeXExport << "\\end{itemize}" << endl << "\\begin{table}[htb]" << endl << "\t\\centering\n\t\\begin{tabular}{cccc}" << endl << "\t\t\\toprule" << endl;
        if (bUseErrors)
            oTeXExport << "\t\t" << _lang.get("PARSERFUNCS_FIT_PARAM") << " & "
                       << _lang.get("PARSERFUNCS_FIT_INITIAL") << " & "
                       << _lang.get("PARSERFUNCS_FIT_FITTED") << " & "
                       << _lang.get("PARSERFUNCS_FIT_PARAM_DEV") << "\\\\" << endl;
        else
            oTeXExport << "\t\t" << _lang.get("PARSERFUNCS_FIT_PARAM") << " & "
                       << _lang.get("PARSERFUNCS_FIT_INITIAL") << " & "
                       << _lang.get("PARSERFUNCS_FIT_FITTED") << " & "
                       << _lang.get("PARSERFUNCS_FIT_ASYMPTOTIC_ERROR") << "\\\\" << endl;
        oTeXExport << "\t\t\\midrule" << endl;
    }
    _data.setCacheStatus(false);


    if (_option.getSystemPrintStatus())
        NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");

    if (_option.getSystemPrintStatus() && !bMaskDialog)
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        NumeReKernel::print(toSystemCodePage("NUMERE: "+toUpperCase(_lang.get("PARSERFUNCS_FIT_HEADLINE"))));
        make_hline();
        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_FUNCTION", sFittedFunction), _option, true));
        if (bUseErrors)
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString((int)nSize))));
        else
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString((int)nSize))));
        //cerr << "|-> Datenpunkte:                            " << nSize << (bUseErrors ? " mit " : " ohne ") << "Gewichtungsfaktoren" << endl;
        if (bRestrictXVals)
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(dMin, 5), toString(dMax, 5))));
        if (bRestrictYVals)
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(dMinY, 5), toString(dMaxY, 5))));
        if (sRestrictions.length())
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", sRestrictions)));
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_FREEDOMS", toString((int)nSize - paramsMap.size()))));
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(dPrecision, 5), toString(nMaxIterations))));
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations()))));
        if (nSize != paramsMap.size() && !(nFitVars & 2))
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7))));
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7))));
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7))));
        }
        else if (nSize != paramsMap.size() && (nFitVars & 2))
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7))));
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7))));
            NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7))));
        }
        NumeReKernel::printPreFmt("|\n");

        if (bUseErrors)
        {
            NumeReKernel::print(_lang.get("PARSERFUNCS_FIT_PARAM")
                + strfill(_lang.get("PARSERFUNCS_FIT_INITIAL"), (_option.getWindow()-32)/2+_option.getWindow()%2-5+9-_lang.get("PARSERFUNCS_FIT_PARAM").length())
                + strfill(_lang.get("PARSERFUNCS_FIT_FITTED"), (_option.getWindow()-50)/2)
                + strfill(_lang.get("PARSERFUNCS_FIT_PARAM_DEV"), 33));
            /**cerr << "|-> "
                 << _lang.get("PARSERFUNCS_FIT_PARAM")
                 << std::setw((_option.getWindow()-32)/2+_option.getWindow()%2-5+9-_lang.get("PARSERFUNCS_FIT_PARAM").length()) << std::setfill(' ')
                 << _lang.get("PARSERFUNCS_FIT_INITIAL")
                 << std::setw((_option.getWindow()-50)/2) << std::setfill(' ')
                 << _lang.get("PARSERFUNCS_FIT_FITTED")
                 << std::setw(33) << std::setfill(' ')
                 << _lang.get("PARSERFUNCS_FIT_PARAM_DEV") << endl;*/
            //cerr << "|-> Parameter" << std::setw((_option.getWindow()-32)/2+_option.getWindow()%2-5) << std::setfill(' ') << "Initialwert" << std::setw((_option.getWindow()-50)/2) << std::setfill(' ') << "Anpassung" << "    berechnete Standardabweichung" << endl;
        }
        else
        {
            NumeReKernel::print(_lang.get("PARSERFUNCS_FIT_PARAM")
                + strfill(_lang.get("PARSERFUNCS_FIT_INITIAL"), (_option.getWindow()-32)/2+_option.getWindow()%2-5+9-_lang.get("PARSERFUNCS_FIT_PARAM").length())
                + strfill(_lang.get("PARSERFUNCS_FIT_FITTED"), (_option.getWindow()-50)/2)
                + strfill(_lang.get("PARSERFUNCS_FIT_ASYMPTOTIC_ERROR"), 33));
            /**cerr << "|-> "
                 << _lang.get("PARSERFUNCS_FIT_PARAM")
                 << std::setw((_option.getWindow()-32)/2+_option.getWindow()%2-5+9-_lang.get("PARSERFUNCS_FIT_PARAM").length()) << std::setfill(' ')
                 << _lang.get("PARSERFUNCS_FIT_INITIAL")
                 << std::setw((_option.getWindow()-50)/2) << std::setfill(' ')
                 << _lang.get("PARSERFUNCS_FIT_FITTED")
                 << std::setw(33) << std::setfill(' ')
                 << _lang.get("PARSERFUNCS_FIT_ASYMPTOTIC_ERROR") << endl;*/
            //cerr << "|-> Parameter" << std::setw((_option.getWindow()-32)/2+_option.getWindow()%2-5) << std::setfill(' ') << "Initialwert" << std::setw((_option.getWindow()-50)/2) << std::setfill(' ') << "Anpassung" << "    Asymptotischer Standardfehler" << endl;
        }
        NumeReKernel::printPreFmt("|   "+strfill("-", _option.getWindow()-4, '-') + "\n");
    }
    pItem = paramsMap.begin();
    string sErrors = "";
    string sPMSign = " ";
    sPMSign[0] = (char)177;
    ///sPMSign[0] = (char)241;

    for (unsigned int n = 0; n < paramsMap.size() /*_fitParams.GetNx()*/; n++)
    {
        if (pItem == paramsMap.end())
            break;
        oFitLog << pItem->first << "    ";
        oFitLog << std::setprecision(_option.getPrecision()) << std::setw(24-pItem->first.length()) << std::setfill(' ') << vInitialVals[n]; //*(pItem->second);
        oFitLog << std::setprecision(_option.getPrecision()) << std::setw(15) << std::setfill(' ') << *(pItem->second); //_fitParams.a[n];
        oFitLog << std::setprecision(_option.getPrecision()) << std::setw(16) << std::setfill(' ') << "± " + toString(sqrt(abs(vz_w[n][n])), 5);
        if (vz_w[n][n])
        {
            oFitLog << " " << std::setw(16) << std::setfill(' ') << "(" + toString(abs(sqrt(abs(vz_w[n][n]/(*(pItem->second)))) /*_fitParamErrors.a[n*(_fitParamErrors.GetNx()+1)]))/_fitParams.a[n]*/ *100.0), 4) + "%)" << endl;
            dErrorPercentageSum += abs(sqrt(abs(vz_w[n][n]/(*(pItem->second)))) /*_fitParamErrors.a[n*(_fitParamErrors.GetNx()+1)]))/_fitParams.a[n]*/ *100.0);
        }
        else
            oFitLog << endl;

        if (bTeXExport)
        {
            oTeXExport << "\t\t$" <<  replaceToTeX(pItem->first, true) << "$ & $"
                       << vInitialVals[n] << "$ & $"
                       << *(pItem->second) << "$ & $\\pm"
                       << sqrt(abs(vz_w[n][n]));
            if (vz_w[n][n])
            {
                oTeXExport << " \\quad (" + toString(abs(sqrt(abs(vz_w[n][n]/(*(pItem->second))))*100.0), 4) + "\\%)$\\\\" << endl;
            }
            else
                oTeXExport << "$\\\\" << endl;
        }
        if (_option.getSystemPrintStatus() && !bMaskDialog)
        {
            NumeReKernel::printPreFmt("|   " + pItem->first + "    "
                + strfill(toString(vInitialVals[n], _option), (_option.getWindow()-32)/2+_option.getWindow()%2-pItem->first.length())
                + strfill(toString(*(pItem->second), _option), (_option.getWindow()-50)/2)
                + strfill(sPMSign + " " + toString(sqrt(abs(vz_w[n][n])), 5), 16));
            ///cerr << "|   " << pItem->first << "    ";
            ///cerr << std::setprecision(_option.getPrecision()) << std::setw((_option.getWindow()-32)/2+_option.getWindow()%2-pItem->first.length()) << std::setfill(' ') << vInitialVals[n]; //*(pItem->second);
            ///cerr << std::setprecision(_option.getPrecision()) << std::setw((_option.getWindow()-50)/2) << std::setfill(' ') << *(pItem->second); //_fitParams.a[n];
            ///cerr << std::setprecision(_option.getPrecision()) << std::setw(16) << std::setfill(' ') << sPMSign + " " + toString(sqrt(abs(vz_w[n][n])), 5);
            if (vz_w[n][n])
                NumeReKernel::printPreFmt(" " + strfill("(" + toString(abs(sqrt(abs(vz_w[n][n]/(*(pItem->second)))) *100.0), 4) + "%)", 16) + "\n");
                ///cerr << " " << std::setw(16) << std::setfill(' ') << "(" + toString(abs(sqrt(abs(vz_w[n][n]/(*(pItem->second)))) *100.0), 4) + "%)" << endl;
            else
                NumeReKernel::printPreFmt("\n");
                ///cerr << endl;
        }
        if (bSaveErrors)
        {
            sErrors += pItem->first + "_error = " + toCmdString(sqrt(abs(vz_w[n][n]))) + ",";
        }
        //*(pItem->second) = _fitParams.a[n];
        ++pItem;
    }
    dErrorPercentageSum /= (double)paramsMap.size(); //_fitParams.GetNx();
    if (bSaveErrors)
    {
        sErrors[sErrors.length()-1] = ' ';
        _parser.SetExpr(sErrors);
        _parser.Eval();
    }
    _parser.SetExpr("chi = "+toCmdString(sqrt(dChisq)));
    _parser.Eval();
    oFitLog << std::setw(76) << std::setfill('-') << '-' << endl;
    if (bTeXExport)
    {
        oTeXExport << "\t\t\\bottomrule" << endl << "\t\\end{tabular}" << endl << "\\end{table}" << endl;
    }
    if (_option.getSystemPrintStatus() && !bMaskDialog)
        NumeReKernel::printPreFmt("|   "+strfill("-", _option.getWindow()-4, '-') + "\n");
        ///cerr << "|   " << std::setw(_option.getWindow()-4) << std::setfill((char)196) << (char)196 << endl;
    if (paramsMap.size() > 1 && paramsMap.size() != nSize) //(_fitParams.nx > 1 && _fitParams.nx != nSize)
    {
        oFitLog << endl;
        oFitLog << _lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD") << ":" << endl;
        oFitLog << endl;
        for (unsigned int n = 0; n < paramsMap.size() /*_fitParams.GetNx()*/; n++)
        {
            if (!n)
                oFitLog << '/';
            else if (n+1 == paramsMap.size() /*_fitParams.GetNx()*/)
                oFitLog << '\\';
            else
                oFitLog << '|';
            for (unsigned int k = 0; k < paramsMap.size() /*_fitParams.GetNx()*/; k++)
            {
                oFitLog << " " << std::setprecision(3) << std::setw(10) << std::setfill(' ') << vz_w[n][k] / sqrt(fabs(vz_w[n][n]*vz_w[k][k])); //_fitParamErrors.a[n + k*_fitParamErrors.GetNx()] / sqrt(fabs(_fitParamErrors.a[n*(_fitParamErrors.GetNx()+1)]*_fitParamErrors.a[k*(_fitParamErrors.GetNx()+1)]));
            }
            if (!n)
                oFitLog << " \\";
            else if (n+1 == paramsMap.size() /*_fitParams.GetNx()*/)
                oFitLog << " /";
            else
                oFitLog << " |";
            oFitLog << endl;
        }

        if (bTeXExport)
        {
            oTeXExport << endl << "\\subsection{" << _lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD") << "}" << endl;
            oTeXExport << "\\[" << endl << "\t\\begin{pmatrix}" << endl;
            for (unsigned int n = 0; n < paramsMap.size(); n++)
            {
                oTeXExport << "\t\t";
                for (unsigned int k = 0; k < paramsMap.size(); k++)
                {
                    oTeXExport << vz_w[n][k] / sqrt(fabs(vz_w[n][n]*vz_w[k][k]));
                    if (k+1 < paramsMap.size())
                        oTeXExport << " & ";
                }
                oTeXExport << "\\\\" << endl;
            }
            oTeXExport << "\t\\end{pmatrix}" << endl << "\\]" << endl;
        }

        if (_option.getSystemPrintStatus() && !bMaskDialog)
        {
            NumeReKernel::printPreFmt("|\n|-> "+toSystemCodePage(_lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD"))+":\n|\n");
            /**cerr << "|" << endl;
            cerr << "|-> " << toSystemCodePage(_lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD")) << ":" << endl;
            cerr << "|" << endl;*/
            for (unsigned int n = 0; n < paramsMap.size() /*_fitParams.GetNx()*/; n++)
            {
                NumeReKernel::printPreFmt("|   ");
                ///cerr << "|   ";
                if (!n)
                    NumeReKernel::printPreFmt("/");
                    ///cerr << '/';
                else if (n+1 == paramsMap.size() /*_fitParams.GetNx()*/)
                    NumeReKernel::printPreFmt("\\");
                    ///cerr << '\\';
                else
                    NumeReKernel::printPreFmt("|");
                    ///cerr << '|';
                for (unsigned int k = 0; k < paramsMap.size() /*_fitParams.GetNx()*/; k++)
                {
                    NumeReKernel::printPreFmt(" " + strfill(toString(vz_w[n][k] / sqrt(fabs(vz_w[n][n] * vz_w[k][k])), 3), 10));
                    ///cerr << " " << std::setprecision(3) << std::setw(10) << std::setfill(' ') << vz_w[n][k] / sqrt(fabs(vz_w[n][n] * vz_w[k][k]));
                    //_fitParamErrors.a[n + k*_fitParamErrors.GetNx()] / sqrt(fabs(_fitParamErrors.a[n*(_fitParamErrors.GetNx()+1)]*_fitParamErrors.a[k*(_fitParamErrors.GetNx()+1)]));
                }
                if (!n)
                    NumeReKernel::printPreFmt(" \\\n");
                    ///cerr << " \\";
                else if (n+1 == paramsMap.size() /*_fitParams.GetNx()*/)
                    NumeReKernel::printPreFmt(" /\n");
                    ///cerr << " /";
                else
                    NumeReKernel::printPreFmt(" |\n");
                    ///cerr << " |";
                ///cerr << endl;
            }
        }
    }
    if (nFitVars & 2)
        nSize *= nSize;
    dNormChisq /= (double)(nSize - paramsMap.size() /*_fitParams.GetNx()*/);
    if (nFitVars & 2)
        dNormChisq = sqrt(dNormChisq);
    ///FITLOG
    oFitLog << endl;
    oFitLog << _lang.get("PARSERFUNCS_FIT_ANALYSIS") <<":" << endl;
    if (_fControl.getIterations() == nMaxIterations)
    {
        oFitLog << LineBreak(_lang.get("PARSERFUNCS_FIT_MAXITER_REACHED"), _option) << endl;
    }
    else
    {
        if (nSize != paramsMap.size() /*_fitParams.GetNx()*/)
        {
            if (bUseErrors)
            {
                if (log10(dNormChisq) > -1.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 50.0)
                    oFitLog << _lang.get("PARSERFUNCS_FIT_GOOD_W_ERROR") << endl;
                else if (log10(dNormChisq) <= -1.0 && dErrorPercentageSum < 20.0)
                    oFitLog << _lang.get("PARSERFUNCS_FIT_BETTER_W_ERROR") << endl;
                else if (log10(dNormChisq) >= 0.5 && log10(dNormChisq) < 1.5 && dErrorPercentageSum < 100.0)
                    oFitLog << _lang.get("PARSERFUNCS_FIT_NOT_GOOD_W_ERROR") << endl;
                else
                    oFitLog << _lang.get("PARSERFUNCS_FIT_BAD_W_ERROR") << endl;
            }
            else
            {
                if (log10(dNormChisq) < -3.0 && dErrorPercentageSum < 20.0)
                    oFitLog << _lang.get("PARSERFUNCS_FIT_GOOD_WO_ERROR") << endl;
                else if (log10(dNormChisq) < 0.0 && dErrorPercentageSum < 50.0)
                    oFitLog << _lang.get("PARSERFUNCS_FIT_IMPROVABLE_WO_ERROR") << endl;
                else if (log10(dNormChisq) >= 0.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 100.0)
                    oFitLog << _lang.get("PARSERFUNCS_FIT_NOT_GOOD_WO_ERROR") << endl;
                else
                    oFitLog << _lang.get("PARSERFUNCS_FIT_BAD_WO_ERROR") << endl;
            }
        }
        else
        {
            oFitLog << _lang.get("PARSERFUNCS_FIT_OVERFITTING") << endl;
        }
    }
    ///TEXEXPORT
    oTeXExport << endl;
    oTeXExport << "\\subsection{" << _lang.get("PARSERFUNCS_FIT_ANALYSIS") << "}" << endl;
    if (_fControl.getIterations() == nMaxIterations)
    {
        oTeXExport << LineBreak(_lang.get("PARSERFUNCS_FIT_MAXITER_REACHED"), _option) << endl;
    }
    else
    {
        if (nSize != paramsMap.size() /*_fitParams.GetNx()*/)
        {
            if (bUseErrors)
            {
                if (log10(dNormChisq) > -1.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 50.0)
                    oTeXExport << _lang.get("PARSERFUNCS_FIT_GOOD_W_ERROR") << endl;
                else if (log10(dNormChisq) <= -1.0 && dErrorPercentageSum < 20.0)
                    oTeXExport << _lang.get("PARSERFUNCS_FIT_BETTER_W_ERROR") << endl;
                else if (log10(dNormChisq) >= 0.5 && log10(dNormChisq) < 1.5 && dErrorPercentageSum < 100.0)
                    oTeXExport << _lang.get("PARSERFUNCS_FIT_NOT_GOOD_W_ERROR") << endl;
                else
                    oTeXExport << _lang.get("PARSERFUNCS_FIT_BAD_W_ERROR") << endl;
            }
            else
            {
                if (log10(dNormChisq) < -3.0 && dErrorPercentageSum < 20.0)
                    oTeXExport << _lang.get("PARSERFUNCS_FIT_GOOD_WO_ERROR") << endl;
                else if (log10(dNormChisq) < 0.0 && dErrorPercentageSum < 50.0)
                    oTeXExport << _lang.get("PARSERFUNCS_FIT_IMPROVABLE_WO_ERROR") << endl;
                else if (log10(dNormChisq) >= 0.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 100.0)
                    oTeXExport << _lang.get("PARSERFUNCS_FIT_NOT_GOOD_WO_ERROR") << endl;
                else
                    oTeXExport << _lang.get("PARSERFUNCS_FIT_BAD_WO_ERROR") << endl;
            }
        }
        else
        {
            oTeXExport << _lang.get("PARSERFUNCS_FIT_OVERFITTING") << endl;
        }
    }

    if (_option.getSystemPrintStatus() && !bMaskDialog)
    {
        NumeReKernel::printPreFmt("|\n|-> "+_lang.get("PARSERFUNCS_FIT_ANALYSIS") + ":\n");
        ///cerr << "|" << endl;
        ///cerr << "|-> " << _lang.get("PARSERFUNCS_FIT_ANALYSIS") << ":" << endl;
        if (_fControl.getIterations() == nMaxIterations)
        {
            NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_MAXITER_REACHED"), _option));
        }
        else
        {
            if (nSize != paramsMap.size() /*_fitParams.GetNx()*/)
            {
                if (bUseErrors)
                {
                    if (log10(dNormChisq) > -1.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 50.0)
                        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_GOOD_W_ERROR"), _option));
                    else if (log10(dNormChisq) <= -1.0 && dErrorPercentageSum < 20.0)
                        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_BETTER_W_ERROR"), _option));
                    else if (log10(dNormChisq) >= 0.5 && log10(dNormChisq) < 1.5 && dErrorPercentageSum < 100.0)
                        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_NOT_GOOD_W_ERROR"), _option));
                    else
                        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_BAD_W_ERROR"), _option));
                }
                else
                {
                    if (log10(dNormChisq) < -3.0 && dErrorPercentageSum < 20.0)
                        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_GOOD_WO_ERROR"), _option));
                    else if (log10(dNormChisq) < 0.0 && dErrorPercentageSum < 50.0)
                        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_IMPROVABLE_WO_ERROR"), _option));
                    else if (log10(dNormChisq) >= 0.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 100.0)
                        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_NOT_GOOD_WO_ERROR"), _option));
                    else
                        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_BAD_WO_ERROR"), _option));
                }
            }
            else
            {
                NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_OVERFITTING"), _option));
            }
        }
        NumeReKernel::toggleTableStatus();
        make_hline();
    }
    if (!_functions.isDefined(sFunctionDefString))
        _functions.defineFunc(sFunctionDefString, _parser, _option);
    else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) != sFunctionDefString)
        _functions.defineFunc(sFunctionDefString, _parser, _option, true);
    oFitLog.close();
    return true;
}

// fft data(:,:) -set inverse complex
bool parser_fft(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    mglDataC _fftData;
    //Indices _idx;
    long long int nCols = 0;
    int nDim = 0;
    int nSize = 0;
    int nSkip = 0;
    double dNyquistFrequency = 1.0;
    double dTimeInterval = 0.0;
    double dPhaseOffset = 0.0;
    bool bInverseTrafo = false;
    bool bComplex = false;

    if (matchParams(sCmd, "inverse"))
        bInverseTrafo = true;
    if (matchParams(sCmd, "complex"))
        bComplex = true;

    if (matchParams(sCmd, "inverse") || matchParams(sCmd, "complex"))
    {
        for (unsigned int i = 0; i < sCmd.length(); i++)
        {
            if (sCmd[i] == '(')
                i += getMatchingParenthesis(sCmd.substr(i));
            if (sCmd[i] == '-')
            {
                sCmd.erase(i);
                break;
            }
        }
    }

    sCmd = sCmd.substr(sCmd.find(' ', sCmd.find("fft")));
    StripSpaces(sCmd);

    string si_pos[2] = {"", ""};                    // String-Array fuer die Zeilen-Position: muss fuer alle Spalten identisch sein!
    string sj_pos[3] = {"", "", ""};                // String-Array fuer die Spalten: kann bis zu drei beliebige Werte haben
    string sDatatable = "data";
    string sTargetTable = "cache";
    int i_pos[2] = {0, 0};                          // Int-Array fuer den Wert der Zeilen-Positionen
    int j_pos[3] = {0, 0, 0};                       // Int-Array fuer den Wert der Spalten-Positionen
    int nMatch = 0;                                 // Int fuer die Position des aktuellen find-Treffers eines Daten-Objekts
    vector<long long int> vLine;
    vector<long long int> vCol;
    value_type* v = 0;
    int nResults = 0;

    // --> Ist da "cache" drin? Aktivieren wir den Cache-Status <--
    if (_data.containsCacheElements(sCmd) && sCmd.substr(0,5) != "data(")
    {
        if (_data.isValidCache())
            _data.setCacheStatus(true);
        else
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);
        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
        {
            if (sCmd.find(iter->first+"(") != string::npos
                && (!sCmd.find(iter->first+"(")
                    || (sCmd.find(iter->first+"(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first+"(")-1, (iter->first).length()+2)))))
            {
                sDatatable = iter->first;
                break;
            }
        }
    }
    else if (!_data.isValid())
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
    // --> Klammer und schliessende Klammer finden und in einen anderen String schreiben <--
    nMatch = sCmd.find('(');
    si_pos[0] = sCmd.substr(nMatch, getMatchingParenthesis(sCmd.substr(nMatch))+1);
    if (si_pos[0] == "()" || si_pos[0][si_pos[0].find_first_not_of(' ',1)] == ')')
        si_pos[0] = "(:,:)";
    if (si_pos[0].find("data(") != string::npos || _data.containsCacheElements(si_pos[0]))
    {
        parser_GetDataElement(si_pos[0],  _parser, _data, _option);
    }

    if (_option.getbDebug())
        cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << endl;

    // --> Rausgeschnittenen String am Komma ',' in zwei Teile teilen <--
    try
    {
        parser_SplitArgs(si_pos[0], sj_pos[0], ',', _option);
    }
    catch (...)
    {
        //delete[] _mDataPlots;
        //delete[] nDataDim;
        throw;
    }
    if (_option.getbDebug())
        cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", sj_pos[0] = " << sj_pos[0] << endl;

    // --> Gibt's einen Doppelpunkt? Dann teilen wir daran auch noch mal <--
    if (si_pos[0].find(':') != string::npos)
    {
        si_pos[0] = "( " + si_pos[0] + " )";
        try
        {
            parser_SplitArgs(si_pos[0], si_pos[1], ':', _option);
        }
        catch (...)
        {
            //delete[] _mDataPlots;
            //delete[] nDataDim;
            throw;
        }
        if (!parser_ExprNotEmpty(si_pos[1]))
            si_pos[1] = "inf";
    }
    else
        si_pos[1] = "";

    if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", si_pos[1] = " << si_pos[1] << endl;
    }

    // --> Auswerten mit dem Parser <--
    if (parser_ExprNotEmpty(si_pos[0]))
    {
        _parser.SetExpr(si_pos[0]);
        v = _parser.Eval(nResults);
        if (nResults > 1)
        {
            for (int n = 0; n < nResults; n++)
                vLine.push_back((int)v[n]-1);
        }
        else
            i_pos[0] = (int)v[0] - 1;
    }
    else
        i_pos[0] = 0;
    if (si_pos[1] == "inf")
    {
        i_pos[1] = _data.getLines(sDatatable, false);
    }
    else if (parser_ExprNotEmpty(si_pos[1]))
    {
        _parser.SetExpr(si_pos[1]);
        i_pos[1] = (int)_parser.Eval() - 1;
    }
    else if (!vLine.size())
        i_pos[1] = i_pos[0]+1;
    // --> Pruefen, ob die Reihenfolge der Indices sinnvoll ist <--
    parser_CheckIndices(i_pos[0], i_pos[1]);

    if (_option.getbDebug())
        cerr << "|-> DEBUG: i_pos[0] = " << i_pos[0] << ", i_pos[1] = " << i_pos[1] << ", vLine.size() = " << vLine.size() << endl;

    if (!parser_ExprNotEmpty(sj_pos[0]))
        sj_pos[0] = "0";

    /* --> Jetzt fuer die Spalten: Fummelig. Man soll bis zu 6 Spalten angeben koennen und
     *     das Programm sollte trotzdem einen Sinn darin finden <--
     */
    int j = 0;
    try
    {
        while (sj_pos[j].find(':') != string::npos && j < 2)
        {
            sj_pos[j] = "( " + sj_pos[j] + " )";
            // --> String am naechsten ':' teilen <--
            parser_SplitArgs(sj_pos[j], sj_pos[j+1], ':', _option);
            // --> Spezialfaelle beachten: ':' ohne linke bzw. rechte Grenze <--
            if (!parser_ExprNotEmpty(sj_pos[j]))
                sj_pos[j] = "1";
            j++;
            if (!parser_ExprNotEmpty(sj_pos[j]))
                sj_pos[j] = "inf";
        }
    }
    catch (...)
    {
        //delete[] _mDataPlots;
        //delete[] nDataDim;
        throw;
    }
    // --> Alle nicht-beschriebenen Grenzen-Strings auf "" setzen <--
    for (int k = j+1; k < 3; k++)
        sj_pos[k] = "";

    // --> Grenzen-Strings moeglichst sinnvoll auswerten <--
    for (int k = 0; k <= j; k++)
    {
        // --> "inf" bedeutet "infinity". Ergo: die letztmoegliche Spalte <--
        if (sj_pos[k] == "inf")
        {
            j_pos[k] = _data.getCols(sDatatable);
            break;
        }
        else if (parser_ExprNotEmpty(sj_pos[k]))
        {
            if (j == 0)
            {
                _parser.SetExpr(sj_pos[0]);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                {
                    for (int n = 0; n < nResults; n++)
                    {
                        if (n >= 6)
                            break;
                        vCol.push_back((int)v[n]-1);
                        j_pos[n] = (int)v[n]-1;
                        j = n;
                    }
                    break;
                }
                else
                    j_pos[0] = (int)v[0] - 1;
            }
            else
            {
                // --> Hat einen Wert: Kann man auch auswerten <--
                _parser.SetExpr(sj_pos[k]);
                j_pos[k] = (int)_parser.Eval() - 1;
            }
        }
        else if (!k)
        {
            // --> erstes Element pro Forma auf 0 setzen <--
            j_pos[k] = 0;
        }
        else // "data(2:4::7) = Spalten 2-4,5-7"
        {
            // --> Spezialfall. Verwendet vermutlich niemand <--
            j_pos[k] = j_pos[k]+1;
        }
    }
    if (_option.getbDebug())
        cerr << "|-> DEBUG: j_pos[0] = " << j_pos[0] << ", j_pos[1] = " << j_pos[1] << ", vCol.size() = " << vCol.size() << endl;
    if (i_pos[1] > _data.getLines(sDatatable, false))
        i_pos[1] = _data.getLines(sDatatable, false);
    if (j_pos[1] > _data.getCols(sDatatable)-1)
        j_pos[1] = _data.getCols(sDatatable)-1;
    if (!vLine.size() && !vCol.size() && (j_pos[0] < 0
        || j_pos[1] < 0
        || i_pos[0] > _data.getLines(sDatatable, false)
        || i_pos[1] > _data.getLines(sDatatable, false)
        || j_pos[0] > _data.getCols(sDatatable)-1
        || j_pos[1] > _data.getCols(sDatatable)-1))
    {
        /*delete[] _mDataPlots;
        delete[] nDataDim;*/
        throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
    }

    // --> Jetzt wissen wir die Spalten: Suchen wir im Falle von si_pos[1] == inf nach der laengsten <--
    if (si_pos[1] == "inf")
    {
        int nAppendedZeroes = _data.getAppendedZeroes(j_pos[0], sDatatable);
        for (int k = 1; k <= j; k++)
        {
            if (nAppendedZeroes > _data.getAppendedZeroes(j_pos[k], sDatatable))
                nAppendedZeroes = _data.getAppendedZeroes(j_pos[k], sDatatable);
        }
        if (nAppendedZeroes < i_pos[1])
            i_pos[1] = _data.getLines(sDatatable, true) - nAppendedZeroes;
        if (_option.getbDebug())
            cerr << "|-> DEBUG: i_pos[1] = " << i_pos[1] << endl;
    }


    /* --> Bestimmen wir die "Dimension" des zu fittenden Datensatzes. Dabei ist es auch
     *     von Bedeutung, ob Fehlerwerte verwendet werden sollen <--
     */
    nDim = 0;
    if (j == 0 && vCol.size() < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
    else if (j == 0)
        nDim = vCol.size();
    else
    {
        nDim = j+1;
    }

    if (vLine.size() && !vCol.size())
    {
        for (int n = 0; n < nDim; n++)
            vCol.push_back(j_pos[n]);
    }

    parser_CheckIndices(i_pos[0], i_pos[1]);
    // Groesse der Datensaetze bestimmen:
    if (!vLine.size())
        nSize = _data.num(sDatatable, i_pos[0], i_pos[1], j_pos[0]);
    else
        nSize = _data.num(sDatatable, vLine, vector<long long int>(1,vCol[0]));
    //cerr << nSize << endl;

    if (abs(i_pos[0]-i_pos[1]) <= 1 && vLine.size() <= 1)
        throw SyntaxError(SyntaxError::TOO_FEW_LINES, sCmd, SyntaxError::invalid_position);

    _fftData.Create(nSize);
    if (!vLine.size())
    {
        dNyquistFrequency = nSize / (_data.max(sDatatable, i_pos[0], i_pos[1], j_pos[0]) - _data.min(sDatatable, i_pos[0], i_pos[1], j_pos[0])) / 2.0;
        dTimeInterval = (nSize-1) / _data.max(sDatatable, i_pos[0], i_pos[1], j_pos[0]);
    }
    else
    {
        dNyquistFrequency = nSize / (_data.max(sDatatable, vLine, vector<long long int>(1,vCol[0])) - _data.min(sDatatable, vLine, vector<long long int>(1,vCol[0]))) / 2.0;
        dTimeInterval = (nSize-1) / _data.max(sDatatable, vLine, vector<long long int>(1,vCol[0]));
    }

    if (_option.getSystemPrintStatus())
    {
        if (!bInverseTrafo && nDim == 2)
            NumeReKernel::printPreFmt(LineBreak("|-> "+_lang.get("PARSERFUNCS_FFT_FOURIERTRANSFORMING", toString(j_pos[0]+1), toString(j_pos[1]+1), toString(dNyquistFrequency,6)) + " ", _option, 0));
            //cerr << LineBreak("|-> Fourier-transformiere Spalten " + toString(j_pos[0]+1) + " und " + toString(j_pos[1]+1) + ":$Nyquist-Grenzfrequenz ist " + toString(dNyquistFrequency,6) + " Hz ... ", _option);
        else if (!bInverseTrafo)
            NumeReKernel::printPreFmt(LineBreak("|-> "+_lang.get("PARSERFUNCS_FFT_FOURIERTRANSFORMING", toString(j_pos[0]+1)+", "+toString(j_pos[1]+1), toString(j_pos[2]+1), toString(dNyquistFrequency,6)) + " ", _option,0));
            //cerr << LineBreak("|-> Fourier-transformiere Spalten " + toString(j_pos[0]+1) + ", " + toString(j_pos[1]+1) + " und " + toString(j_pos[2]+1) + ":$Nyquist-Grenzfrequenz ist " + toString(dNyquistFrequency,6) + " Hz ... ", _option);
        else if (bInverseTrafo && nDim == 2)
            NumeReKernel::printPreFmt(LineBreak("|-> "+_lang.get("PARSERFUNCS_FFT_INVERSE_FOURIERTRANSFORMING", toString(j_pos[0]+1), toString(j_pos[1]+1), toString(dNyquistFrequency,6)) + " ", _option,0));
            //cerr << LineBreak("|-> Invers-Fourier-transformiere Spalten " + toString(j_pos[0]+1) + " und " + toString(j_pos[1]+1) + ":$Ergebnis-Zeitintervall ist " + toString(dTimeInterval,6) + " s ... ", _option);
        else
            NumeReKernel::printPreFmt(LineBreak("|-> "+_lang.get("PARSERFUNCS_FFT_INVERSE_FOURIERTRANSFORMING", toString(j_pos[0]+1)+", "+toString(j_pos[1]+1), toString(j_pos[2]+1), toString(dNyquistFrequency,6)) + " ", _option,0));
            //cerr << LineBreak("|-> Invers-Fourier-transformiere Spalten " + toString(j_pos[0]+1) + ", " + toString(j_pos[1]+1) + " und " + toString(j_pos[2]+1) + ":$Ergebnis-Zeitintervall ist " + toString(dTimeInterval,6) + " s ... ", _option);
    }

    if (!vLine.size())
    {
        for (int i = 0; i < abs(i_pos[0]-i_pos[1]); i++)
        {
            if (i-nSkip == nSize)
                break;
            if (nDim == 2)
            {
                if (_data.isValidEntry(i+i_pos[0], j_pos[1], sDatatable))
                    _fftData.a[i-nSkip] = dual(_data.getElement(i+i_pos[0], j_pos[1], sDatatable),0.0);
                else
                    nSkip++;
            }
            else if (bComplex && nDim == 3)
            {
                if (_data.isValidEntry(i+i_pos[0], j_pos[1], sDatatable) && _data.isValidEntry(i+i_pos[0], j_pos[2], sDatatable))
                    _fftData.a[i-nSkip] = dual(_data.getElement(i+i_pos[0], j_pos[1], sDatatable),_data.getElement(i+i_pos[0], j_pos[2], sDatatable));
                else
                    nSkip++;
            }
            else if (!bComplex && nDim == 3)
            {
                if (_data.isValidEntry(i+i_pos[0], j_pos[1], sDatatable) && _data.isValidEntry(i+i_pos[0], j_pos[2], sDatatable))
                {
                    _fftData.a[i-nSkip] = dual(_data.getElement(i+i_pos[0], j_pos[1], sDatatable)*cos(_data.getElement(i+i_pos[0], j_pos[2], sDatatable)),
                                        _data.getElement(i+i_pos[0], j_pos[1], sDatatable)*sin(_data.getElement(i+i_pos[0], j_pos[2], sDatatable)));
                }
                else
                    nSkip++;
            }
        }
    }
    else
    {
        for (unsigned int i = 0; i < vLine.size(); i++)
        {
            if (i-nSkip == (unsigned)nSize)
                break;
            if (nDim == 2)
            {
                if (_data.isValidEntry(vLine[i], vCol[1], sDatatable))
                    _fftData.a[i-nSkip] = dual(_data.getElement(vLine[i], vCol[1], sDatatable), 0.0);
                else
                    nSkip++;
            }
            else if (bComplex && nDim == 3)
            {
                if (_data.isValidEntry(vLine[i], vCol[1], sDatatable) && _data.isValidEntry(vLine[i], vCol[2], sDatatable))
                    _fftData.a[i-nSkip] = dual(_data.getElement(vLine[i], vCol[1], sDatatable),_data.getElement(vLine[i], vCol[2], sDatatable));
                else
                    nSkip++;
            }
            else if (!bComplex && nDim == 3)
            {
                if (_data.isValidEntry(vLine[i], vCol[1], sDatatable) && _data.isValidEntry(vLine[i], vCol[2], sDatatable))
                {
                    _fftData.a[i-nSkip] = dual(_data.getElement(vLine[i], vCol[1], sDatatable)*cos(_data.getElement(vLine[i], vCol[2], sDatatable)),
                                        _data.getElement(vLine[i], vCol[1], sDatatable)*sin(_data.getElement(vLine[i], vCol[2], sDatatable)));
                }
                else
                    nSkip++;
            }
        }
    }
    try
    {
        if (!bInverseTrafo)
        {
            _fftData.FFT("x");
            _fftData.a[0] /= dual((double)nSize, 0.0);
            _fftData.a[(int)round(_fftData.GetNx()/2.0)] /= dual(2.0, 0.0);
            for (long long int i = 1; i < _fftData.GetNx(); i++)
                _fftData.a[i] /= dual((double)nSize/2.0, 0.0);
        }
        else
        {
            _fftData.a[0] *= dual(2.0,0.0);
            _fftData.a[_fftData.GetNx()-1] *= dual(2.0,0.0);
            for (long long int i = 0; i < _fftData.GetNx(); i++)
                _fftData.a[i] *= dual((double)(_fftData.GetNx()-1),0.0);
            _fftData.FFT("ix");
        }
    }
    catch (...)
    {
        throw;
    }
    if (sDatatable != "data")
        sTargetTable = sDatatable;
    nCols = _data.getCacheCols(sTargetTable, false)+1;

    if (!bInverseTrafo)
    {
        for (long long int i = 0; i < (int)round(_fftData.GetNx()/2.0)+1; i++)
        {
            _data.writeToCache(i, nCols-1, sTargetTable, 2.0*(double)(i)*dNyquistFrequency/(double)(_fftData.GetNx()));
            if (!bComplex)
            {
                _data.writeToCache(i, nCols, sTargetTable, hypot(_fftData.a[i].real(),_fftData.a[i].imag()));
                //if (i > 2 && (2.0*atan2(_fftData.a[i].imag(), _fftData.a[i].real()) > M_PI && 2.0*atan2(_fftData.a[i-1].imag(), _fftData.a[i-1].real()) < -M_PI)
                if (i > 2 && (fabs(atan2(_fftData.a[i].imag(), _fftData.a[i].real())-atan2(_fftData.a[i-1].imag(),_fftData.a[i-1].real())) >= M_PI)
                    && ((atan2(_fftData.a[i].imag(), _fftData.a[i].real())-atan2(_fftData.a[i-1].imag(),_fftData.a[i-1].real()))*(atan2(_fftData.a[i-1].imag(), _fftData.a[i-1].real())-atan2(_fftData.a[i-2].imag(),_fftData.a[i-2].real())) < 0))
                {
                    if (atan2(_fftData.a[i-1].imag(), _fftData.a[i-1].real())-atan2(_fftData.a[i-2].imag(),_fftData.a[i-2].real()) < 0.0)
                        dPhaseOffset -= 2*M_PI;
                    else if (atan2(_fftData.a[i-1].imag(), _fftData.a[i-1].real())-atan2(_fftData.a[i-2].imag(),_fftData.a[i-2].real()) > 0.0)
                        dPhaseOffset += 2*M_PI;
                }
                _data.writeToCache(i, nCols+1, sTargetTable, atan2(_fftData.a[i].imag(), _fftData.a[i].real())+dPhaseOffset);
            }
            else
            {
                _data.writeToCache(i, nCols, sTargetTable, _fftData.a[i].real());
                _data.writeToCache(i, nCols+1, sTargetTable, _fftData.a[i].imag());
            }
        }

        _data.setCacheStatus(true);
        _data.setHeadLineElement(nCols-1, sTargetTable, _lang.get("COMMON_FREQUENCY")+"_[Hz]");
        if (!bComplex)
        {
            _data.setHeadLineElement(nCols, sTargetTable, _lang.get("COMMON_AMPLITUDE"));
            _data.setHeadLineElement(nCols+1, sTargetTable, _lang.get("COMMON_PHASE")+"_[rad]");
        }
        else
        {
            _data.setHeadLineElement(nCols, sTargetTable, "Re("+_lang.get("COMMON_AMPLITUDE")+")");
            _data.setHeadLineElement(nCols+1, sTargetTable, "Im("+_lang.get("COMMON_AMPLITUDE")+")");
        }
    }
    else
    {
        for (long long int i = 0; i < _fftData.GetNx(); i++)
        {
            _data.writeToCache(i, nCols-1, sTargetTable, (double)(i)*dTimeInterval/(double)(_fftData.GetNx()-1));
            _data.writeToCache(i, nCols, sTargetTable, _fftData.a[i].real());
            _data.writeToCache(i, nCols+1, sTargetTable, _fftData.a[i].imag());
        }

        _data.setCacheStatus(true);
        _data.setHeadLineElement(nCols-1, sTargetTable, _lang.get("COMMON_TIME")+"_[s]");
        _data.setHeadLineElement(nCols, sTargetTable, "Re("+_lang.get("COMMON_SIGNAL")+")");
        _data.setHeadLineElement(nCols+1, sTargetTable, "Im("+_lang.get("COMMON_SIGNAL")+")");
    }
    if (_option.getSystemPrintStatus())
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

    _data.setCacheStatus(false);
    return true;
}

bool parser_evalPoints(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
    unsigned int nSamples = 100;
    //double dVal[2];
    double dLeft = 0.0;
    double dRight = 0.0;
    //int nMode = 0;
    double* dVar = 0;
    double dTemp = 0.0;
    string sExpr = "";
    string sParams = "";
    string sInterval = "";
    string sVar = "";
    bool bLogarithmic = false;
    if (sCmd.find("-set") != string::npos)
    {
        sExpr = sCmd.substr(0,sCmd.find("-set"));
        sParams = sCmd.substr(sCmd.find("-set"));
    }
    else if (sCmd.find("--") != string::npos)
    {
        sExpr = sCmd.substr(0,sCmd.find("--"));
        sParams = sCmd.substr(sCmd.find("--"));
    }
    else
        sExpr = sCmd;

    StripSpaces(sExpr);
    sExpr = sExpr.substr(findCommand(sExpr).sString.length());

    if (parser_ExprNotEmpty(sExpr))
    {
        if (!_functions.call(sExpr, _option))
            return false;
    }
    if (parser_ExprNotEmpty(sParams))
    {
        if (!_functions.call(sParams, _option))
            return false;
    }
    StripSpaces(sParams);

    if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
    {

        parser_GetDataElement(sExpr, _parser, _data, _option);

        if (sExpr.find("{") != string::npos)
            parser_VectorToExpr(sExpr, _option);
    }

    if (sParams.find("data(") != string::npos || _data.containsCacheElements(sParams))
    {
        parser_GetDataElement(sParams, _parser, _data, _option);

        if (sParams.find("{") != string::npos && (containsStrings(sParams) || _data.containsStringVars(sParams)))
            parser_VectorToExpr(sParams, _option);
    }

    if (matchParams(sParams, "samples", '='))
    {
        sParams += " ";
        if (parser_ExprNotEmpty(getArgAtPos(sParams, matchParams(sParams, "samples", '=')+7)))
        {
            _parser.SetExpr(getArgAtPos(sParams, matchParams(sParams, "samples", '=')+7));
            nSamples = (unsigned int)_parser.Eval();
        }
        sParams.erase(matchParams(sParams, "samples", '=')-1, 8);
    }
    if (matchParams(sParams, "logscale"))
    {
        bLogarithmic = true;
        sParams.erase(matchParams(sParams, "logscale")-1, 8);
    }

    if (sParams.find('=') != string::npos
        || (sParams.find('[') != string::npos
            && sParams.find(']', sParams.find('['))
            && sParams.find(':', sParams.find('['))))
    {
        if (sParams.substr(0,2) == "--")
            sParams = sParams.substr(2);
        else if (sParams.substr(0,4) == "-set")
            sParams = sParams.substr(4);

        //value_type* v = 0;
        //Datafile _cache;
        //_cache.setCacheStatus(true);
        //int nResults = 0;
        if (sParams.find('=') != string::npos)
        {
            int nPos = sParams.find('=');
            sInterval = getArgAtPos(sParams, nPos+1);
            if (sInterval.front() == '[' && sInterval.back() == ']')
            {
                sInterval.pop_back();
                sInterval.erase(0,1);
            }
            sVar = " " + sParams.substr(0,nPos);
            sVar = sVar.substr(sVar.rfind(' '));
            StripSpaces(sVar);
        }
        else
        {
            sVar = "x";
            sInterval = sParams.substr(sParams.find('[')+1, getMatchingParenthesis(sParams.substr(sParams.find('[')))-1);
            StripSpaces(sInterval);
            if (sInterval == ":")
                sInterval = "-10:10";
        }

        if (parser_ExprNotEmpty(sExpr))
        {
            _parser.SetExpr(sExpr);
        }
        else
            _parser.SetExpr(sVar);
        _parser.Eval();
        /*if (!parser_CheckVarOccurence(_parser, sVar))
        {
            if (!_parser.Eval())
                sCmd = "\"Der Ausdruck ist auf dem gesamten Intervall identisch Null!\"";
            else
                sCmd = toSystemCodePage("\"Bezüglich der Variablen " + sVar + " ist der Ausdruck konstant und besitzt keine Nullstellen!\"");
            return true;
        }*/
        dVar = parser_GetVarAdress(sVar, _parser);
        if (!dVar)
        {
            throw SyntaxError(SyntaxError::EVAL_VAR_NOT_FOUND, sCmd, sVar, sVar);
        }
        if (sInterval.find(':') == string::npos || sInterval.length() < 3)
            return false;
        if (parser_ExprNotEmpty(sInterval.substr(0,sInterval.find(':'))))
        {
            _parser.SetExpr(sInterval.substr(0,sInterval.find(':')));
            dLeft = _parser.Eval();
            if (isinf(dLeft) || isnan(dLeft))
            {
                sCmd = "nan";
                return false;
            }
        }
        else
            return false;
        if (parser_ExprNotEmpty(sInterval.substr(sInterval.find(':')+1)))
        {
            _parser.SetExpr(sInterval.substr(sInterval.find(':')+1));
            dRight = _parser.Eval();
            if (isinf(dRight) || isnan(dRight))
            {
                sCmd = "nan";
                return false;
            }
        }
        else
            return false;
        /*if (dRight < dLeft)
        {
            double Temp = dRight;
            dRight = dLeft;
            dLeft = Temp;
        }*/
        if (bLogarithmic && (dLeft <= 0.0 || dRight <= 0.0))
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
    }

    if (parser_ExprNotEmpty(sExpr))
        _parser.SetExpr(sExpr);
    else if (dVar)
        _parser.SetExpr(sVar);
    else
        _parser.SetExpr("0");
    _parser.Eval();
    sCmd = "";
    vector<double> vResults;
    if (dVar)
    {
        dTemp = *dVar;

        *dVar = dLeft;

        //cerr << _parser.Eval() << endl;
        vResults.push_back(_parser.Eval());
        /*sCmd += toCmdString(_parser.Eval());
        if (nSamples > 1)
            sCmd += ",";*/

        for (unsigned int i = 1; i < nSamples; i++)
        {
            if (bLogarithmic)
                *dVar = pow(10.0, log10(dLeft) + i*(log10(dRight)-log10(dLeft))/(double)(nSamples-1));
            else
                *dVar = dLeft + i*(dRight-dLeft)/(double)(nSamples-1);
            /*if (i < 10)
                cerr << _parser.Eval() << endl;*/
            vResults.push_back(_parser.Eval());
            /*sCmd += toCmdString(_parser.Eval());
            if (i < nSamples-1)
                sCmd += ",";*/
        }
        *dVar = dTemp;
    }
    else
    {

        for (unsigned int i = 0; i < nSamples; i++)
        {
            vResults.push_back(_parser.Eval());
            /*sCmd += toCmdString(_parser.Eval());
            if (i < nSamples-1)
                sCmd += ",";*/
        }
    }
    sCmd = "evalpnts[~_~]";
    _parser.SetVectorVar("evalpnts[~_~]", vResults);
    //sCmd = "{{" + sCmd + "}}";

    return true;
}

// datagrid -x=x0:x1 y=y0:y1 z=func(x,y) samples=100
// datagrid -x=data(:,1) y=data(:,2) z=data(:,3)
// datagrid -x=data(2:,1) y=data(1,2:) z=data(2:,2:)
// datagrid EXPR -set [x0:x1, y0:y1] PARAMS
bool parser_datagrid(string& sCmd, string& sTargetCache, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    unsigned int nSamples = 100;
    string sXVals = "";
    string sYVals = "";
    string sZVals = "";

    Indices _idx;

    bool bTranspose = false;

    vector<double> vXVals;
    vector<double> vYVals;
    vector<vector<double> > vZVals;


    if (sCmd.find("data(") != string::npos && !_data.isValid())
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
    if (_data.containsCacheElements(sCmd) && !_data.isValidCache())
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);


    if (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos)
    {
        sZVals = sCmd.substr(findCommand(sCmd).sString.length()+findCommand(sCmd).nPos);
        if (sCmd.find("-set") != string::npos)
        {
            sCmd.erase(0,sCmd.find("-set"));
            sZVals.erase(sZVals.find("-set"));
        }
        else
        {
            sCmd.erase(0,sCmd.find("--"));
            sZVals.erase(sZVals.find("--"));
        }
        StripSpaces(sZVals);
    }
    if (sCmd.find('[') != string::npos && sCmd.find(']', sCmd.find('[')) != string::npos)
    {
        sXVals = sCmd.substr(sCmd.find('[')+1, sCmd.find(']', sCmd.find('[')) - sCmd.find('[')-1);
        StripSpaces(sXVals);
        if (sXVals.find(',') != string::npos)
        {
            sXVals = "(" + sXVals + ")";
            try
            {
                parser_SplitArgs(sXVals, sYVals, ',', _option);
            }
            catch (...)
            {
                sXVals.pop_back();
                sXVals.erase(0,1);
            }
            StripSpaces(sXVals);
            StripSpaces(sYVals);
        }
        if (sXVals == ":")
            sXVals = "-10:10";
        if (sYVals == ":")
            sYVals = "-10:10";
    }
    if ((!matchParams(sCmd, "x", '=') && !sXVals.length())
        || (!matchParams(sCmd, "y", '=') && !sYVals.length())
        || (!matchParams(sCmd, "z", '=') && !sZVals.length()))
    {
        //sErrorToken = "datagrid";
        throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, SyntaxError::invalid_position, "datagrid");
    }

    if (matchParams(sCmd, "samples", '='))
    {
        _parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "samples", '=')+7));
        nSamples = (unsigned int)_parser.Eval();
        if (nSamples < 2)
            throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);
        sCmd.erase(sCmd.find(getArgAtPos(sCmd, matchParams(sCmd, "samples", '=')+7), matchParams(sCmd, "samples", '=')-1),getArgAtPos(sCmd, matchParams(sCmd, "samples", '=')+7).length());
        sCmd.erase(matchParams(sCmd, "samples", '=')-1, 8);
    }
    if (matchParams(sCmd, "target", '='))
    {
        sTargetCache = getArgAtPos(sCmd, matchParams(sCmd, "target", '=')+6);
        _idx = parser_getIndices(sTargetCache, _parser, _data, _option);
        sTargetCache.erase(sTargetCache.find('('));
        if (sTargetCache == "data")
            throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, sTargetCache);

        if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
        sCmd.erase(sCmd.find(getArgAtPos(sCmd, matchParams(sCmd, "target", '=')+6), matchParams(sCmd, "target", '=')-1),getArgAtPos(sCmd, matchParams(sCmd, "target", '=')+6).length());
        sCmd.erase(matchParams(sCmd, "target", '=')-1, 7);
    }
    else
    {
        _idx.nI[0] = 0;
        _idx.nI[1] = -2;
        _idx.nJ[0] = 0;
        if (_data.isCacheElement("grid()"))
            _idx.nJ[0] += _data.getCols("grid", false);
        _idx.nJ[1] = -2;
    }
    if (matchParams(sCmd, "transpose"))
    {
        bTranspose = true;
        sCmd.erase(matchParams(sCmd, "transpose")-1, 9);
    }
    if (!sXVals.length())
    {
        sXVals = getArgAtPos(sCmd, matchParams(sCmd, "x", '=')+1);
        sCmd.erase(sCmd.find(getArgAtPos(sCmd, matchParams(sCmd, "x", '=')+1), matchParams(sCmd, "x", '=')-1),getArgAtPos(sCmd, matchParams(sCmd, "x", '=')+1).length());
        sCmd.erase(matchParams(sCmd, "x", '=')-1, 2);
    }
    if (!sYVals.length())
    {
        sYVals = getArgAtPos(sCmd, matchParams(sCmd, "y", '=')+1);
        sCmd.erase(sCmd.find(getArgAtPos(sCmd, matchParams(sCmd, "y", '=')+1), matchParams(sCmd, "y", '=')-1),getArgAtPos(sCmd, matchParams(sCmd, "y", '=')+1).length());
        sCmd.erase(matchParams(sCmd, "y", '=')-1, 2);
    }
    if (!sZVals.length())
    {
        while (sCmd[sCmd.length()-1] == ' ' || sCmd[sCmd.length()-1] == '=' || sCmd[sCmd.length()-1] == '-')
            sCmd.erase(sCmd.length()-1);
        sZVals = getArgAtPos(sCmd, matchParams(sCmd, "z", '=')+1);
    }
    if (!_functions.call(sZVals, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sZVals, sZVals);

    if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: sXVals = " << sXVals << endl;
        cerr << "|-> DEBUG: sYVals = " << sYVals << endl;
        cerr << "|-> DEBUG: sZVals = " << sZVals << endl;
    }

    ///>> X-Vector
    if ((sXVals.find("data(") != string::npos || _data.containsCacheElements(sXVals)) && sXVals.find(':', getMatchingParenthesis(sXVals.substr(sXVals.find('(')))+sXVals.find('(')) == string::npos)
    {
        Indices _idx = parser_getIndices(sXVals, _parser, _data, _option);
        string sDatatable = "data";
        if (_data.containsCacheElements(sXVals))
        {
            _data.setCacheStatus(true);
            for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
            {
                if (sXVals.find(iter->first+"(") != string::npos
                    && (!sXVals.find(iter->first+"(")
                        || (sXVals.find(iter->first+"(") && checkDelimiter(sXVals.substr(sXVals.find(iter->first+"(")-1, (iter->first).length()+2)))))
                {
                    sDatatable = iter->first;
                    break;
                }
            }
        }
        if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
        if (!_idx.vI.size())
        {
            if (_idx.nI[1] == -1)
                _idx.nI[1] = _idx.nI[0];
            if (_idx.nJ[1] == -1)
                _idx.nJ[1] = _idx.nJ[0];
            if (_idx.nJ[1] == -2)
                _idx.nJ[1] = _data.getCols(sDatatable)-1;
            if (_idx.nI[1] == -2 && _idx.nJ[1] != _idx.nJ[0])
                throw SyntaxError(SyntaxError::NO_MATRIX, sCmd, SyntaxError::invalid_position);
            if (_idx.nI[1] == -2)
                _idx.nI[1] = _data.getLines(sDatatable, true) - _data.getAppendedZeroes(_idx.nJ[0], sDatatable)-1;

            parser_CheckIndices(_idx.nI[0], _idx.nI[1]);
            parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

            if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
            {
                for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                {
                    for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                    {
                        //if (_data.isValidEntry(i,j))
                            vXVals.push_back(_data.getElement(i,j, sDatatable));
                    }
                }
            }
            else
            {
                double dMin = _data.min(sDatatable, _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]);
                double dMax = _data.max(sDatatable, _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]);

                for (unsigned int i = 0; i < nSamples; i++)
                    vXVals.push_back((dMax-dMin)/double(nSamples-1)*i+dMin);
            }
        }
        else
        {
            if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
            {
                vXVals = _data.getElement(_idx.vI,_idx.vJ,sDatatable);
            }
            else
            {
                double dMin = _data.min(sDatatable, _idx.vI, _idx.vJ);
                double dMax = _data.max(sDatatable, _idx.vI, _idx.vJ);

                for (unsigned int i = 0; i < nSamples; i++)
                    vXVals.push_back((dMax-dMin)/double(nSamples-1)*i+dMin);
            }
        }
        _data.setCacheStatus(false);
    }
    else if (sXVals.find(':') != string::npos)
    {
        if (sXVals.find("data(") != string::npos || _data.containsCacheElements(sXVals))
        {
            parser_GetDataElement(sXVals, _parser, _data, _option);
        }
        if (sXVals.find("{") != string::npos)
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
        sXVals.replace(sXVals.find(':'),1,",");
        _parser.SetExpr(sXVals);

        double* dResult = 0;
        int nNumResults = 0;
        dResult = _parser.Eval(nNumResults);
        if (nNumResults < 2)
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

        if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
        {
            Indices _idx = parser_getIndices(sZVals, _parser, _data, _option);
            string sZDatatable = "data";
            if (_data.containsCacheElements(sZVals))
            {
                _data.setCacheStatus(true);
                for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                {
                    if (sZVals.find(iter->first+"(") != string::npos
                        && (!sZVals.find(iter->first+"(")
                            || (sZVals.find(iter->first+"(") && checkDelimiter(sZVals.substr(sZVals.find(iter->first+"(")-1, (iter->first).length()+2)))))
                    {
                        sZDatatable = iter->first;
                        break;
                    }
                }
            }
            if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
            if (!_idx.vI.size())
            {
                if (_idx.nI[1] == -1)
                    _idx.nI[1] = _idx.nI[0];
                if (_idx.nJ[1] == -1)
                    _idx.nJ[1] = _idx.nJ[0];
                if (_idx.nJ[1] == -2)
                    _idx.nJ[1] = _data.getCols(sZDatatable)-1;

                parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

                if (_idx.nI[1] == -2)
                {
                    _idx.nI[1] = _data.getLines(sZDatatable, true)-_data.getAppendedZeroes(_idx.nJ[0], sZDatatable)-1;
                    for (long long int j = _idx.nJ[0]+1; j <= _idx.nJ[1]; j++)
                    {
                        if (_data.getLines(sZDatatable, true)-_data.getAppendedZeroes(j, sZDatatable)-1 > _idx.nI[1])
                            _idx.nI[1] = _data.getLines(sZDatatable, true)-_data.getAppendedZeroes(j, sZDatatable)-1;
                    }
                }

                parser_CheckIndices(_idx.nI[0], _idx.nI[1]);

                nSamples = _idx.nI[1] - _idx.nI[0] + 1;
                if (nSamples < 2)
                    nSamples = _idx.nJ[1] - _idx.nJ[0] + 1;
                if (nSamples < 2)
                    throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);
            }
            else
            {
                nSamples = _idx.vI.size();
                if (nSamples < 2)
                    nSamples = _idx.vJ.size();
                if (nSamples < 2)
                    throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);
            }
            _data.setCacheStatus(false);
        }

        for (unsigned int i = 0; i < nSamples; i++)
        {
            vXVals.push_back(dResult[0] + (dResult[1] - dResult[0])/double(nSamples-1)*i);
        }
    }
    else
        throw SyntaxError(SyntaxError::SEPARATOR_NOT_FOUND, sCmd, SyntaxError::invalid_position);

    if (_option.getbDebug())
        cerr << "|-> DEBUG: vXVals.size() = " << vXVals.size() << endl;

    ///>> Y-Vector
    if ((sYVals.find("data(") != string::npos || _data.containsCacheElements(sYVals)) && sYVals.find(':', getMatchingParenthesis(sYVals.substr(sXVals.find('(')))+sYVals.find('(')) == string::npos)
    {
        Indices _idx = parser_getIndices(sYVals, _parser, _data, _option);
        string sDatatable = "data";
        if (_data.containsCacheElements(sYVals))
        {
            _data.setCacheStatus(true);
            for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
            {
                if (sYVals.find(iter->first+"(") != string::npos
                    && (!sYVals.find(iter->first+"(")
                        || (sYVals.find(iter->first+"(") && checkDelimiter(sYVals.substr(sYVals.find(iter->first+"(")-1, (iter->first).length()+2)))))
                {
                    sDatatable = iter->first;
                    break;
                }
            }
        }

        if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
        if (!_idx.vI.size())
        {
            if (_idx.nI[1] == -1)
                _idx.nI[1] = _idx.nI[0];
            if (_idx.nJ[1] == -1)
                _idx.nJ[1] = _idx.nJ[0];
            if (_idx.nJ[1] == -2)
                _idx.nJ[1] = _data.getCols(sDatatable)-1;
            if (_idx.nI[1] == -2 && _idx.nJ[1] != _idx.nJ[0])
                throw SyntaxError(SyntaxError::NO_MATRIX, sCmd, SyntaxError::invalid_position);
            if (_idx.nI[1] == -2)
                _idx.nI[1] = _data.getLines(sDatatable, true) - _data.getAppendedZeroes(_idx.nJ[0], sDatatable)-1;

            parser_CheckIndices(_idx.nI[0], _idx.nI[1]);
            parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

            if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
            {
                for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                {
                    for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                    {
                        //if (_data.isValidEntry(i,j))
                            vYVals.push_back(_data.getElement(i,j, sDatatable));
                    }
                }
            }
            else
            {
                double dMin = _data.min(sDatatable, _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]);
                double dMax = _data.max(sDatatable, _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]);

                for (unsigned int i = 0; i < nSamples; i++)
                    vYVals.push_back((dMax-dMin)/double(nSamples-1)*i+dMin);
            }
        }
        else
        {
            if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
            {
                vYVals = _data.getElement(_idx.vI, _idx.vJ, sDatatable);
            }
            else
            {
                double dMin = _data.min(sDatatable, _idx.vI, _idx.vJ);
                double dMax = _data.max(sDatatable, _idx.vI, _idx.vJ);

                for (unsigned int i = 0; i < nSamples; i++)
                    vYVals.push_back((dMax-dMin)/double(nSamples-1)*i+dMin);
            }
        }
        _data.setCacheStatus(false);
    }
    else if (sYVals.find(':') != string::npos)
    {
        if (sYVals.find("data(") != string::npos || _data.containsCacheElements(sYVals))
        {
            parser_GetDataElement(sYVals, _parser, _data, _option);
        }
        if (sYVals.find("{") != string::npos)
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
        sYVals.replace(sYVals.find(':'),1,",");
        _parser.SetExpr(sYVals);

        double* dResult = 0;
        int nNumResults = 0;
        dResult = _parser.Eval(nNumResults);
        if (nNumResults < 2)
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

        if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
        {
            Indices _idx = parser_getIndices(sZVals, _parser, _data, _option);
            string szDatatable = "data";
            if (_data.containsCacheElements(sZVals))
            {
                _data.setCacheStatus(true);
                for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                {
                    if (sZVals.find(iter->first+"(") != string::npos
                        && (!sZVals.find(iter->first+"(")
                            || (sZVals.find(iter->first+"(") && checkDelimiter(sZVals.substr(sZVals.find(iter->first+"(")-1, (iter->first).length()+2)))))
                    {
                        szDatatable = iter->first;
                        break;
                    }
                }
            }

            if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
            if (!_idx.vI.size())
            {
                if (_idx.nI[1] == -1)
                    _idx.nI[1] = _idx.nI[0];
                if (_idx.nJ[1] == -1)
                    _idx.nJ[1] = _idx.nJ[0];
                if (_idx.nJ[1] == -2)
                    _idx.nJ[1] = _data.getCols(szDatatable)-1;
                if (_idx.nI[1] == -2)
                {
                    _idx.nI[1] = _data.getLines(szDatatable, true)-_data.getAppendedZeroes(_idx.nJ[0], szDatatable)-1;
                    for (long long int j = _idx.nJ[0]+1; j <= _idx.nJ[1]; j++)
                    {
                        if (_data.getLines(szDatatable, true)-_data.getAppendedZeroes(j, szDatatable)-1 > _idx.nI[1])
                            _idx.nI[1] = _data.getLines(szDatatable, true)-_data.getAppendedZeroes(j, szDatatable)-1;
                    }
                }

                parser_CheckIndices(_idx.nI[0], _idx.nI[1]);
                parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

                nSamples = _idx.nJ[1] - _idx.nJ[0] + 1;
                if (nSamples < 2)
                    nSamples = _idx.nI[1] - _idx.nI[0] + 1;
                if (nSamples < 2)
                    throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);
            }
            else
            {
                nSamples = _idx.vJ.size();
                if (nSamples < 2)
                    nSamples = _idx.vI.size();
                if (nSamples < 2)
                    throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);
            }
            _data.setCacheStatus(false);
        }

        for (unsigned int i = 0; i < nSamples; i++)
        {
            vYVals.push_back(dResult[0] + (dResult[1] - dResult[0])/double(nSamples-1)*i);
        }
    }
    else
        throw SyntaxError(SyntaxError::SEPARATOR_NOT_FOUND, sCmd, SyntaxError::invalid_position);
    if (_option.getbDebug())
        cerr << "|-> DEBUG: vYVals.size() = " << vYVals.size() << endl;

    ///>> Z-Matrix
    if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
    {
        Indices _idx = parser_getIndices(sZVals, _parser, _data, _option);
        string szDatatable = "data";
        if (_data.containsCacheElements(sZVals))
        {
            _data.setCacheStatus(true);
            for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
            {
                if (sZVals.find(iter->first+"(") != string::npos
                    && (!sZVals.find(iter->first+"(")
                        || (sZVals.find(iter->first+"(") && checkDelimiter(sZVals.substr(sZVals.find(iter->first+"(")-1, (iter->first).length()+2)))))
                {
                    szDatatable = iter->first;
                    break;
                }
            }
        }

        if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
        if (!_idx.vI.size())
        {
            if (_idx.nI[1] == -1)
                _idx.nI[1] = _idx.nI[0];
            if (_idx.nJ[1] == -1)
                _idx.nJ[1] = _idx.nJ[0];
            if (_idx.nJ[1] == -2)
                _idx.nJ[1] = _data.getCols(szDatatable)-1;

            parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

            if (_idx.nI[1] == -2)
            {
                _idx.nI[1] = _data.getLines(szDatatable, true)-_data.getAppendedZeroes(_idx.nJ[0], szDatatable)-1;
                for (long long int j = _idx.nJ[0]+1; j <= _idx.nJ[1]; j++)
                {
                    if (_data.getLines(szDatatable, true)-_data.getAppendedZeroes(j, szDatatable)-1 > _idx.nI[1])
                        _idx.nI[1] = _data.getLines(szDatatable, true)-_data.getAppendedZeroes(j, szDatatable)-1;
                }
            }

            parser_CheckIndices(_idx.nI[0], _idx.nI[1]);

            vector<double> vVector;
            if (!bTranspose)
            {
                for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                {
                    for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                    {
                        vVector.push_back(_data.getElement(i,j, szDatatable));
                    }
                    vZVals.push_back(vVector);
                    vVector.clear();
                }
            }
            else
            {
                for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                {
                    for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                    {
                        vVector.push_back(_data.getElement(i,j, szDatatable));
                    }
                    vZVals.push_back(vVector);
                    vVector.clear();
                }
            }
            if (_option.getbDebug())
                cerr << "|-> DEBUG: vZVals.size() = " << vZVals.size() << endl;

            if (!vZVals.size() || (vZVals.size() == 1 && vZVals[0].size() == 1))
                throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);

            if (vZVals.size() == 1 || vZVals[0].size() == 1)
            {
                mglData _mData[4];
                mglGraph _graph;
                _mData[0].Create(nSamples, nSamples);
                _mData[1].Create(vXVals.size());
                _mData[2].Create(vYVals.size());
                if (vZVals.size() != 1)
                    _mData[3].Create(vZVals.size());
                else
                    _mData[3].Create(vZVals[0].size());
                for (unsigned int i = 0; i < vXVals.size(); i++)
                    _mData[1].a[i] = vXVals[i];
                for (unsigned int i = 0; i < vYVals.size(); i++)
                    _mData[2].a[i] = vYVals[i];
                if (vZVals.size() != 1)
                {
                    for (unsigned int i = 0; i < vZVals.size(); i++)
                        _mData[3].a[i] = vZVals[i][0];
                }
                else
                {
                    for (unsigned int i = 0; i < vZVals[0].size(); i++)
                        _mData[3].a[i] = vZVals[0][i];
                }

                //cerr << _mData[3].Minimal() << endl;
                //cerr << _mData[3].Maximal() << endl;

                _graph.SetRanges(_mData[1], _mData[2], _mData[3]);
                _graph.DataGrid(_mData[0], _mData[1], _mData[2], _mData[3]);

                vXVals.clear();
                vYVals.clear();
                vZVals.clear();

                for (unsigned int i = 0; i < nSamples; i++)
                {
                    vXVals.push_back(_mData[1].Minimal()+(_mData[1].Maximal()-_mData[1].Minimal())/(double)(nSamples-1)*i);
                    vYVals.push_back(_mData[2].Minimal()+(_mData[2].Maximal()-_mData[2].Minimal())/(double)(nSamples-1)*i);
                }

                for (unsigned int i = 0; i < nSamples; i++)
                {
                    for (unsigned int j = 0; j < nSamples; j++)
                    {
                        vVector.push_back(_mData[0].a[i+nSamples*j]);
                    }
                    vZVals.push_back(vVector);
                    vVector.clear();
                }
            }
        }
        else
        {
            vector<double> vVector;
            if (!bTranspose)
            {
                for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                {
                    vVector = _data.getElement(vector<long long int>(1,_idx.vI[i]), _idx.vJ, szDatatable);
                    vZVals.push_back(vVector);
                    vVector.clear();
                }
            }
            else
            {
                for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                {
                    vVector = _data.getElement(_idx.vI, vector<long long int>(1,_idx.vJ[j]), szDatatable);
                    vZVals.push_back(vVector);
                    vVector.clear();
                }
            }
            if (_option.getbDebug())
                cerr << "|-> DEBUG: vZVals.size() = " << vZVals.size() << endl;

            if (!vZVals.size() || (vZVals.size() == 1 && vZVals[0].size() == 1))
                throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);

            if (vZVals.size() == 1 || vZVals[0].size() == 1)
            {
                mglData _mData[4];
                mglGraph _graph;
                _mData[0].Create(nSamples, nSamples);
                _mData[1].Create(vXVals.size());
                _mData[2].Create(vYVals.size());
                if (vZVals.size() != 1)
                    _mData[3].Create(vZVals.size());
                else
                    _mData[3].Create(vZVals[0].size());
                for (unsigned int i = 0; i < vXVals.size(); i++)
                    _mData[1].a[i] = vXVals[i];
                for (unsigned int i = 0; i < vYVals.size(); i++)
                    _mData[2].a[i] = vYVals[i];
                if (vZVals.size() != 1)
                {
                    for (unsigned int i = 0; i < vZVals.size(); i++)
                        _mData[3].a[i] = vZVals[i][0];
                }
                else
                {
                    for (unsigned int i = 0; i < vZVals[0].size(); i++)
                        _mData[3].a[i] = vZVals[0][i];
                }

                //cerr << _mData[3].Minimal() << endl;
                //cerr << _mData[3].Maximal() << endl;

                _graph.SetRanges(_mData[1], _mData[2], _mData[3]);
                _graph.DataGrid(_mData[0], _mData[1], _mData[2], _mData[3]);

                vXVals.clear();
                vYVals.clear();
                vZVals.clear();

                for (unsigned int i = 0; i < nSamples; i++)
                {
                    vXVals.push_back(_mData[1].Minimal()+(_mData[1].Maximal()-_mData[1].Minimal())/(double)(nSamples-1)*i);
                    vYVals.push_back(_mData[2].Minimal()+(_mData[2].Maximal()-_mData[2].Minimal())/(double)(nSamples-1)*i);
                }

                for (unsigned int i = 0; i < nSamples; i++)
                {
                    for (unsigned int j = 0; j < nSamples; j++)
                    {
                        vVector.push_back(_mData[0].a[i+nSamples*j]);
                    }
                    vZVals.push_back(vVector);
                    vVector.clear();
                }
            }
        }
        _data.setCacheStatus(false);
    }
    else
    {
        _parser.SetExpr(sZVals);

        vector<double> vVector;
        for (unsigned int x = 0; x < vXVals.size(); x++)
        {
            parser_iVars.vValue[0][0] = vXVals[x];
            for (unsigned int y = 0; y < vYVals.size(); y++)
            {
                parser_iVars.vValue[1][0] = vYVals[y];
                vVector.push_back(_parser.Eval());
            }
            vZVals.push_back(vVector);
            vVector.clear();
        }
    }

    if (_idx.nI[1] == -2 || _idx.nI[1] == -1)
        _idx.nI[1] = _idx.nI[0] + vXVals.size();
    if (_idx.nJ[1] == -2 || _idx.nJ[1] == -1)
        _idx.nJ[1] = _idx.nJ[0] + vYVals.size()+2;

    if (!_data.isCacheElement(sTargetCache))
        _data.addCache(sTargetCache, _option);
    _data.setCacheStatus(true);
    //long long int nFirstCol = _data.getCacheCols(sTargetCache, false);
    for (unsigned int i = 0; i < vXVals.size(); i++)
        _data.writeToCache(i, _idx.nJ[0], sTargetCache, vXVals[i]);
    _data.setHeadLineElement(_idx.nJ[0], sTargetCache, "x");
    //nFirstCol++;
    for (unsigned int i = 0; i < vYVals.size(); i++)
        _data.writeToCache(i, _idx.nJ[0]+1, sTargetCache, vYVals[i]);
    _data.setHeadLineElement(_idx.nJ[0]+1, sTargetCache, "y");
    //nFirstCol++;

    for (unsigned int i = 0; i < vZVals.size(); i++)
    {
        if (i+_idx.nI[0] >= _idx.nI[1])
            break;
        for (unsigned int j = 0; j < vZVals[i].size(); j++)
        {
            if (j+2+_idx.nJ[0] >= _idx.nJ[1])
                break;
            _data.writeToCache(_idx.nI[0]+i, _idx.nJ[0]+2+j, sTargetCache, vZVals[i][j]);
            if (!i)
                _data.setHeadLineElement(_idx.nJ[0]+2+j, sTargetCache, "z["+toString((int)j+1)+"]");
        }
    }
    _data.setCacheStatus(false);

    return true;
}


bool parser_evalIndices(const string& sCache, Indices& _idx, Datafile& _data)
{
    if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
        return false;
    if (_idx.nI[1] == -1)
        _idx.nI[1] = _idx.nI[0];
    if (_idx.nJ[1] == -1)
        _idx.nJ[1] = _idx.nJ[0];
    if (_idx.nI[1] == -2)
        _idx.nI[1] = _data.getLines(sCache.substr(0,sCache.find('(')), false);
    else
        _idx.nI[1]++;
    if (_idx.nJ[1] == -2)
        _idx.nJ[1] = _data.getCols(sCache.substr(0,sCache.find('(')));
    else
        _idx.nJ[1]++;
    return true;
}


vector<double> parser_IntervalReader(string& sExpr, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option, bool bEraseInterval)
{
    vector<double> vInterval;
    string sInterval[2] = {"",""};

    if (!_functions.call(sExpr, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sExpr, SyntaxError::invalid_position);

    if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
        parser_GetDataElement(sExpr, _parser, _data, _option);

    //cerr << sExpr << endl;
    if (matchParams(sExpr, "x", '='))
    {
        sInterval[0] = getArgAtPos(sExpr, matchParams(sExpr, "x", '=')+1);
        if (bEraseInterval)
        {
            sExpr.erase(sExpr.find(sInterval[0]), sInterval[0].length());
            sExpr.erase(sExpr.rfind('x',matchParams(sExpr,"x", '=')), matchParams(sExpr, "x", '=')+1-sExpr.rfind('x',matchParams(sExpr,"x", '=')));
        }
        if (sInterval[0].find(':') != string::npos)
            parser_SplitArgs(sInterval[0], sInterval[1], ':', _option, true);
        //cerr << sInterval[0] << "   " << sInterval[1] << endl;
        if (parser_ExprNotEmpty(sInterval[0]))
        {
            _parser.SetExpr(sInterval[0]);
            vInterval.push_back(_parser.Eval());
        }
        else
            vInterval.push_back(NAN);
        if (parser_ExprNotEmpty(sInterval[1]))
        {
            _parser.SetExpr(sInterval[1]);
            vInterval.push_back(_parser.Eval());
        }
        else
            vInterval.push_back(NAN);
    }
    if (matchParams(sExpr, "y", '='))
    {
        sInterval[0] = getArgAtPos(sExpr, matchParams(sExpr, "y", '=')+1);
        if (bEraseInterval)
        {
            sExpr.erase(sExpr.find(sInterval[0]), sInterval[0].length());
            sExpr.erase(sExpr.rfind('y',matchParams(sExpr,"y", '=')), matchParams(sExpr, "y", '=')+1-sExpr.rfind('y',matchParams(sExpr,"y", '=')));
        }
        if (sInterval[0].find(':') != string::npos)
            parser_SplitArgs(sInterval[0], sInterval[1], ':', _option, true);
        while (vInterval.size() < 2)
        {
            vInterval.push_back(NAN);
        }
        if (parser_ExprNotEmpty(sInterval[0]))
        {
            _parser.SetExpr(sInterval[0]);
            vInterval.push_back(_parser.Eval());
        }
        else
            vInterval.push_back(NAN);
        if (parser_ExprNotEmpty(sInterval[1]))
        {
            _parser.SetExpr(sInterval[1]);
            vInterval.push_back(_parser.Eval());
        }
        else
            vInterval.push_back(NAN);
    }
    if (matchParams(sExpr, "z", '='))
    {
        sInterval[0] = getArgAtPos(sExpr, matchParams(sExpr, "z", '=')+1);
        if (bEraseInterval)
        {
            sExpr.erase(sExpr.find(sInterval[0]), sInterval[0].length());
            sExpr.erase(sExpr.rfind('z',matchParams(sExpr,"z", '=')), matchParams(sExpr, "z", '=')+1-sExpr.rfind('z',matchParams(sExpr,"z", '=')));
        }
        if (sInterval[0].find(':') != string::npos)
            parser_SplitArgs(sInterval[0], sInterval[1], ':', _option, true);
        while (vInterval.size() < 4)
            vInterval.push_back(NAN);
        if (parser_ExprNotEmpty(sInterval[0]))
        {
            _parser.SetExpr(sInterval[0]);
            vInterval.push_back(_parser.Eval());
        }
        else
            vInterval.push_back(NAN);
        if (parser_ExprNotEmpty(sInterval[1]))
        {
            _parser.SetExpr(sInterval[1]);
            vInterval.push_back(_parser.Eval());
        }
        else
            vInterval.push_back(NAN);
    }
    if (sExpr.find('[') != string::npos
        && sExpr.find(']', sExpr.find('[')) != string::npos
        && sExpr.find(':', sExpr.find('[')) != string::npos)
    {
        unsigned int nPos = 0;

        do
        {
            nPos = sExpr.find('[', nPos);
            if (nPos == string::npos || sExpr.find(']',nPos) == string::npos)
                break;
            nPos++;
        }
        while (isInQuotes(sExpr, nPos) || sExpr.substr(nPos, sExpr.find(']')-nPos).find(':') == string::npos);

        if (nPos != string::npos && sExpr.find(']', nPos) != string::npos)
        {
            string sRanges[3];
            sRanges[0] = sExpr.substr(nPos, sExpr.find(']', nPos) - nPos);
            //cerr << sRanges[0] << endl;
            if (bEraseInterval)
                sExpr.erase(nPos-1, sExpr.find(']', nPos)-nPos+2);
            while (sRanges[0].find(',') != string::npos)
            {
                sRanges[0] = "(" + sRanges[0] + ")";
                parser_SplitArgs(sRanges[0], sRanges[2], ',', _option, false);
                if (sRanges[0].find(':') == string::npos)
                {
                    sRanges[0] = sRanges[2];
                    continue;
                }
                sRanges[0] = "(" + sRanges[0] + ")";
                parser_SplitArgs(sRanges[0], sRanges[1], ':', _option, false);
                if (parser_ExprNotEmpty(sRanges[0]))
                {
                    _parser.SetExpr(sRanges[0]);
                    vInterval.push_back(_parser.Eval());
                }
                else
                    vInterval.push_back(NAN);
                if (parser_ExprNotEmpty(sRanges[1]))
                {
                    _parser.SetExpr(sRanges[1]);
                    vInterval.push_back(_parser.Eval());
                }
                else
                    vInterval.push_back(NAN);
                sRanges[0] = sRanges[2];
            }
            if (sRanges[0].find(':') != string::npos)
            {
                sRanges[0] = "(" + sRanges[0] + ")";
                parser_SplitArgs(sRanges[0], sRanges[1], ':', _option, false);
                if (parser_ExprNotEmpty(sRanges[0]))
                {
                    _parser.SetExpr(sRanges[0]);
                    vInterval.push_back(_parser.Eval());
                }
                else
                    vInterval.push_back(NAN);
                if (parser_ExprNotEmpty(sRanges[1]))
                {
                    _parser.SetExpr(sRanges[1]);
                    vInterval.push_back(_parser.Eval());
                }
                else
                    vInterval.push_back(NAN);
            }
        }
    }
    return vInterval;
}

// audio data() -samples=SAMPLES file=FILENAME
bool parser_writeAudio(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    using namespace little_endian_io;

    ofstream fAudio;
    string sAudioFileName = "<savepath>/audiofile.wav";
    string sDataset = "";
    int nSamples = 44100;
    int nChannels = 1;
    int nBPS = 16;
    unsigned int nDataChunkPos = 0;
    unsigned int nFileSize = 0;
    const double dValMax = 32760.0;
    double dMax = 0.0;
    Indices _idx;
    Matrix _mDataSet;
    //_option.declareFileType(".wav");
    sCmd.erase(0,findCommand(sCmd).nPos + findCommand(sCmd).sString.length()); // Kommando entfernen

    // Strings parsen
    if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
    {
        string sDummy = "";
        if (!parser_StringParser(sCmd, sDummy, _data, _parser, _option, true))
            throw SyntaxError(SyntaxError::STRING_ERROR, sCmd, SyntaxError::invalid_position);
    }
    // Funktionen aufrufen
    if (!_functions.call(sCmd, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

    // Samples lesen
    if (matchParams(sCmd, "samples", '='))
    {
        string sSamples = getArgAtPos(sCmd, matchParams(sCmd, "samples",'=')+7);
        if (sSamples.find("data(") != string::npos || _data.containsCacheElements(sSamples))
        {
            parser_GetDataElement(sSamples, _parser, _data, _option);
        }
        _parser.SetExpr(sSamples);
        if (!isnan(_parser.Eval()) && !isinf(_parser.Eval()) && _parser.Eval() >= 1);
        nSamples = (int)_parser.Eval();
    }

    // Dateiname lesen
    if (matchParams(sCmd, "file", '='))
        sAudioFileName = getArgAtPos(sCmd, matchParams(sCmd, "file", '=')+4);
    if (sAudioFileName.find('/') == string::npos && sAudioFileName.find('\\') == string::npos)
        sAudioFileName.insert(0,"<savepath>/");
    // Dateiname pruefen
    sAudioFileName = _data.ValidFileName(sAudioFileName, ".wav");
    //cerr << sAudioFileName << endl;


    // Indices lesen
    _idx = parser_getIndices(sCmd, _parser, _data, _option);
    sDataset = sCmd.substr(0,sCmd.find('('));
    StripSpaces(sDataset);
    if (_idx.vI.size() || _idx.vJ.size())
    {
        if (_idx.vJ.size() > 2)
            return false;
        if (fabs(_data.max(sDataset, _idx.vI, _idx.vJ)) > fabs(_data.min(sDataset, _idx.vI, _idx.vJ)))
            dMax = fabs(_data.max(sDataset, _idx.vI, _idx.vJ));
        else
            dMax = fabs(_data.min(sDataset, _idx.vI, _idx.vJ));
        _mDataSet.push_back(_data.getElement(_idx.vI, vector<long long int>(_idx.vJ[0]), sDataset));
        if (_idx.vJ.size() == 2)
            _mDataSet.push_back(_data.getElement(_idx.vI, vector<long long int>(_idx.vJ[1]), sDataset));
        _mDataSet = parser_transposeMatrix(_mDataSet);
    }
    else
    {
        if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
            return false;
        if (_idx.nI[1] == -1)
            _idx.nI[1] = _idx.nI[0];
        else if (_idx.nI[1] == -2)
            _idx.nI[1] = _data.getLines(sDataset,false)-1;
        if (_idx.nJ[1] == -1)
            _idx.nJ[1] = _idx.nJ[0];
        else if (_idx.nJ[1] == -2)
        {
            _idx.nJ[1] = _idx.nJ[0]+1;
        }
        if (_data.getCols(sDataset, false) <= _idx.nJ[1])
            _idx.nJ[1] = _idx.nJ[0];
        _mDataSet = parser_ZeroesMatrix(_idx.nI[1]-_idx.nI[0]+1,(_idx.nJ[1] != _idx.nJ[0] ? 2 : 1));
        double dMaxCol[2] = {0.0,0.0};
        if (_idx.nJ[1] != _idx.nJ[0])
        {
            if (fabs(_data.max(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[1], -1)) > fabs(_data.min(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[1], -1)))
                dMaxCol[1] = fabs(_data.max(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[1], -1));
            else
                dMaxCol[1] = fabs(_data.min(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[1], -1));
            for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                _mDataSet[i-_idx.nI[0]][1] = _data.getElement(i, _idx.nJ[1], sDataset);
        }
        if (fabs(_data.max(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[0], -1)) > fabs(_data.min(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[0], -1)))
            dMaxCol[1] = fabs(_data.max(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[0], -1));
        else
            dMaxCol[1] = fabs(_data.min(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[0], -1));
        for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
            _mDataSet[i-_idx.nI[0]][0] = _data.getElement(i, _idx.nJ[0], sDataset);

        if (dMaxCol[0] > dMaxCol[1])
            dMax = dMaxCol[0];
        else
            dMax = dMaxCol[1];

    }

    for (unsigned int i = 0; i < _mDataSet.size(); i++)
    {
        for (unsigned int j = 0; j < _mDataSet[0].size(); j++)
        {
            _mDataSet[i][j] = _mDataSet[i][j] / dMax * dValMax;
        }
    }

    nChannels = _mDataSet[0].size();

    // Datenstream oeffnen
    fAudio.open(sAudioFileName.c_str(), ios::binary);

    if (fAudio.fail())
        return false;

    // Wave Header
    fAudio << "RIFF----WAVEfmt ";
    write_word(fAudio, 16, 4);
    write_word(fAudio, 1, 2);
    write_word(fAudio, nChannels, 2);
    write_word(fAudio, nSamples, 4);
    write_word(fAudio, (nSamples*nBPS*nChannels)/8, 4);
    write_word(fAudio, 2*nChannels, 2);
    write_word(fAudio, nBPS, 2);

    nDataChunkPos = fAudio.tellp();
    fAudio << "data----";
    // Audio-Daten schreiben
    for (unsigned int i = 0; i < _mDataSet.size(); i++)
    {
        for (unsigned int j = 0; j < _mDataSet[0].size(); j++)
        {
            write_word(fAudio, (int)_mDataSet[i][j], 2);
        }
    }
    // Chunk sizes nachtraeglich einfuegen
    nFileSize = fAudio.tellp();
    fAudio.seekp(nDataChunkPos+4);
    write_word(fAudio, nFileSize-nDataChunkPos+8,4);
    fAudio.seekp(4);
    write_word(fAudio, nFileSize-8,4);
    fAudio.close();
    return true;
}

bool parser_regularize(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    int nSamples = 100;
    string sDataset = "";
    string sColHeaders[2] = {"",""};
    Indices _idx;
    mglData _x, _v;
    double dXmin, dXmax;
    //_option.declareFileType(".wav");
    sCmd.erase(0,findCommand(sCmd).nPos + findCommand(sCmd).sString.length()); // Kommando entfernen

    // Strings parsen
    if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
    {
        string sDummy = "";
        if (!parser_StringParser(sCmd, sDummy, _data, _parser, _option, true))
            throw SyntaxError(SyntaxError::STRING_ERROR, sCmd, SyntaxError::invalid_position);
    }
    // Funktionen aufrufen
    if (!_functions.call(sCmd, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

    // Samples lesen
    if (matchParams(sCmd, "samples", '='))
    {
        string sSamples = getArgAtPos(sCmd, matchParams(sCmd, "samples",'=')+7);
        if (sSamples.find("data(") != string::npos || _data.containsCacheElements(sSamples))
        {
            parser_GetDataElement(sSamples, _parser, _data, _option);
        }
        _parser.SetExpr(sSamples);
        if (!isnan(_parser.Eval()) && !isinf(_parser.Eval()) && _parser.Eval() >= 1);
        nSamples = (int)_parser.Eval();
    }

    // Indices lesen
    _idx = parser_getIndices(sCmd, _parser, _data, _option);
    sDataset = sCmd.substr(0,sCmd.find('('));
    StripSpaces(sDataset);
    if (_idx.vI.size() || _idx.vJ.size())
    {
        if (_idx.vJ.size() != 2)
            return false;
        _x.Create(_data.cnt(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0])));
        _v.Create(_data.cnt(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0])));
        dXmin = _data.min(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0]));
        dXmax = _data.max(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0]));
        sColHeaders[0] = _data.getHeadLineElement(_idx.vJ[0], sDataset) + "\n(regularisiert)";
        sColHeaders[1] = _data.getHeadLineElement(_idx.vJ[1], sDataset) + "\n(regularisiert)";
        for (long long int i = 0; i < _idx.vI.size(); i++)
        {
            _x.a[i] = _data.getElement(_idx.vI[i], _idx.vJ[0], sDataset);
            _v.a[i] = _data.getElement(_idx.vI[i], _idx.vJ[1], sDataset);
        }
    }
    else
    {
        if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
            return false;
        if (_idx.nI[1] == -1)
            _idx.nI[1] = _idx.nI[0];
        else if (_idx.nI[1] == -2)
            _idx.nI[1] = _data.getLines(sDataset,false)-1;
        if (_idx.nJ[1] == -1)
            _idx.nJ[1] = _idx.nJ[0];
        else if (_idx.nJ[1] == -2)
        {
            _idx.nJ[1] = _idx.nJ[0]+1;
        }
        if (_data.getCols(sDataset, false) <= _idx.nJ[1])
            return false;
        _v.Create(_data.cnt(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]));
        _x.Create(_data.cnt(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]));
        for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
        {
            _v.a[i-_idx.nI[0]] = _data.getElement(i, _idx.nJ[1], sDataset);
        }
        dXmin = _data.min(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]);
        dXmax = _data.max(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]);
        sColHeaders[0] = _data.getHeadLineElement(_idx.nJ[0], sDataset) + "\\n(regularisiert)";
        sColHeaders[1] = _data.getHeadLineElement(_idx.nJ[1], sDataset) + "\\n(regularisiert)";
        for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
        {
            _x.a[i-_idx.nI[0]] = _data.getElement(i, _idx.nJ[0], sDataset);
        }
    }

    /*cerr << _v.nx << " " << _v.Minimal() << " " << _v.Maximal() << endl;
    cerr << _x.nx << " " << _x.Minimal() << " " << _x.Maximal() << endl;
    cerr << dXmin << " " << dXmax << endl;*/

    if (_x.nx != _v.GetNx())
        return false;
    if (!matchParams(sCmd, "samples", '='))
        nSamples = _x.GetNx();
    mglData _regularized(nSamples);
    _regularized.Refill(_x, _v, dXmin, dXmax); //wohin damit?

    long long int nLastCol = _data.getCols(sDataset, false);
    for (long long int i = 0; i < nSamples; i++)
    {
        _data.writeToCache(i, nLastCol, sDataset, dXmin + i*(dXmax-dXmin)/(nSamples-1));
        _data.writeToCache(i, nLastCol+1, sDataset, _regularized.a[i]);
    }
    _data.setHeadLineElement(nLastCol, sDataset, sColHeaders[0]);
    _data.setHeadLineElement(nLastCol+1, sDataset, sColHeaders[1]);
    return true;
}

bool parser_pulseAnalysis(string& _sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    string sDataset = "";
    Indices _idx;
    mglData _v;
    vector<double> vPulseProperties;
    double dXmin = NAN, dXmax = NAN;
    double dSampleSize = NAN;
    string sCmd = _sCmd.substr(findCommand(_sCmd, "pulse").nPos+5);
    //_option.declareFileType(".wav");
    //sCmd.erase(0,findCommand(sCmd).nPos + findCommand(sCmd).sString.length()); // Kommando entfernen

    // Strings parsen
    if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
    {
        string sDummy = "";
        if (!parser_StringParser(sCmd, sDummy, _data, _parser, _option, true))
            throw SyntaxError(SyntaxError::STRING_ERROR, _sCmd, SyntaxError::invalid_position);
    }
    // Funktionen aufrufen
    if (!_functions.call(sCmd, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, _sCmd, SyntaxError::invalid_position);


    // Indices lesen
    //cerr << sCmd << endl;
    _idx = parser_getIndices(sCmd, _parser, _data, _option);
    sDataset = sCmd;
    sDataset.erase(sDataset.find('('));
    StripSpaces(sDataset);
    //cerr << sDataset << endl;
    if (_idx.vI.size() || _idx.vJ.size())
    {
        if (_idx.vJ.size() != 2)
            return false;
        //_x.Create(_data.cnt(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0])));
        _v.Create(_data.cnt(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0])));
        dXmin = _data.min(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0]));
        dXmax = _data.max(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0]));
        for (long long int i = 0; i < _idx.vI.size(); i++)
        {
            //_x.a[i] = _data.getElement(_idx.vI[i], _idx.vJ[0], sDataset);
            _v.a[i] = _data.getElement(_idx.vI[i], _idx.vJ[1], sDataset);
        }
    }
    else
    {
        if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
            return false;
        if (_idx.nI[1] == -1)
            _idx.nI[1] = _idx.nI[0];
        else if (_idx.nI[1] == -2)
            _idx.nI[1] = _data.getLines(sDataset,false)-1;
        if (_idx.nJ[1] == -1)
            _idx.nJ[1] = _idx.nJ[0];
        else if (_idx.nJ[1] == -2)
        {
            _idx.nJ[1] = _idx.nJ[0]+1;
        }
        if (_data.getCols(sDataset, false) <= _idx.nJ[1])
            return false;
        _v.Create(_data.cnt(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]));
        //_x.Create(_data.cnt(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]));
        for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
        {
            _v.a[i-_idx.nI[0]] = _data.getElement(i, _idx.nJ[1], sDataset);
        }
        dXmin = _data.min(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]);
        dXmax = _data.max(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]);
        /*for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
        {
            _x.a[i-_idx.nI[0]] = _data.getElement(i, _idx.nJ[0], sDataset);
        }*/
    }

    /*cerr << _v.nx << " " << _v.Minimal() << " " << _v.Maximal() << endl;
    cerr << _x.nx << " " << _x.Minimal() << " " << _x.Maximal() << endl;
    cerr << dXmin << " " << dXmax << endl;*/

    dSampleSize = (dXmax-dXmin)/((double)_v.GetNx()-1.0);
    mglData _pulse(_v.Pulse('x'));
    if (_pulse.nx >= 5)
    {
        vPulseProperties.push_back(_pulse[0]); // max Amp
        vPulseProperties.push_back(_pulse[1]*dSampleSize+dXmin); // pos max Amp
        vPulseProperties.push_back(2.0*_pulse[2]*dSampleSize); // FWHM
        vPulseProperties.push_back(2.0*_pulse[3]*dSampleSize); // Width near max
        vPulseProperties.push_back(_pulse[4]*dSampleSize); // Energy (Integral pulse^2)
    }
    else
    {
        vPulseProperties.push_back(NAN);
        _sCmd.replace(findCommand(_sCmd, "pulse").nPos, string::npos, "pulse[~_~]");
        _parser.SetVectorVar("pulse[~_~]", vPulseProperties);

        return true;
    }
    // Ausgabe
    if (_option.getSystemPrintStatus())
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        NumeReKernel::print(LineBreak("NUMERE: "+toUpperCase(_lang.get("PARSERFUNCS_PULSE_HEADLINE")), _option));
        make_hline();
        for (unsigned int i = 0; i < vPulseProperties.size(); i++)
        {
            NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PARSERFUNCS_PULSE_TABLE_" + toString((int)i+1) + "_*", toString(vPulseProperties[i], _option)), _option, 0)+"\n");
        }
        NumeReKernel::toggleTableStatus();
        make_hline();
    }

    _sCmd.replace(findCommand(_sCmd, "pulse").nPos, string::npos, "pulse[~_~]");
    _parser.SetVectorVar("pulse[~_~]", vPulseProperties);

    return true;
}

bool parser_stfa(string& sCmd, string& sTargetCache, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    string sDataset = "";
    Indices _idx, _target;
    mglData _real, _imag, _result;
    int nSamples = 0;

    double dXmin = NAN, dXmax = NAN;
    double dFmin = 0.0, dFmax = 1.0;
    double dSampleSize = NAN;
    sCmd.erase(0, findCommand(sCmd).nPos+4);

    // Strings parsen
    if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
    {
        string sDummy = "";
        if (!parser_StringParser(sCmd, sDummy, _data, _parser, _option, true))
            throw SyntaxError(SyntaxError::STRING_ERROR, sCmd, SyntaxError::invalid_position);
    }
    // Funktionen aufrufen
    if (!_functions.call(sCmd, _option))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

    if (matchParams(sCmd, "samples", '='))
    {
        _parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "samples", '=')+7));
        nSamples = _parser.Eval();
        if (nSamples < 0)
            nSamples = 0;
    }
    if (matchParams(sCmd, "target", '='))
    {
        sTargetCache = getArgAtPos(sCmd, matchParams(sCmd, "target", '=')+6);
        _target = parser_getIndices(sTargetCache, _parser, _data, _option);
        sTargetCache.erase(sTargetCache.find('('));
        if (sTargetCache == "data")
            throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);

        if (_target.nI[0] == -1 || _target.nJ[0] == -1)
            throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
    }
    else
    {
        _target.nI[0] = 0;
        _target.nI[1] = -2;
        _target.nJ[0] = 0;
        if (_data.isCacheElement("stfdat()"))
            _target.nJ[0] += _data.getCols("stfdat", false);
        sTargetCache = "stfdat";
        _target.nJ[1] = -2;
    }


    // Indices lesen
    //cerr << sCmd << endl;
    _idx = parser_getIndices(sCmd, _parser, _data, _option);
    sDataset = sCmd;
    sDataset.erase(sDataset.find('('));
    StripSpaces(sDataset);
    //cerr << sDataset << endl;

    if (_idx.vI.size() || _idx.vJ.size())
    {
        if (_idx.vJ.size() != 2)
            return false;
        //_x.Create(_data.cnt(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0])));
        _real.Create(_data.cnt(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0])));
        _imag.Create(_data.cnt(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0])));
        dXmin = _data.min(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0]));
        dXmax = _data.max(sDataset, _idx.vI, vector<long long int>(_idx.vJ[0]));
        for (long long int i = 0; i < _idx.vI.size(); i++)
        {
            //_x.a[i] = _data.getElement(_idx.vI[i], _idx.vJ[0], sDataset);
            _real.a[i] = _data.getElement(_idx.vI[i], _idx.vJ[1], sDataset);
        }
        sDataset = _data.getHeadLineElement(_idx.vJ[0], sDataset);
    }
    else
    {
        if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
            return false;
        if (_idx.nI[1] == -1)
            _idx.nI[1] = _idx.nI[0];
        else if (_idx.nI[1] == -2)
            _idx.nI[1] = _data.getLines(sDataset,false)-1;
        if (_idx.nJ[1] == -1)
            _idx.nJ[1] = _idx.nJ[0];
        else if (_idx.nJ[1] == -2)
        {
            _idx.nJ[1] = _idx.nJ[0]+1;
        }
        if (_data.getCols(sDataset, false) <= _idx.nJ[1])
            return false;
        _real.Create(_data.cnt(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]));
        _imag.Create(_data.cnt(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]));
        //_x.Create(_data.cnt(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]));
        for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
        {
            _real.a[i-_idx.nI[0]] = _data.getElement(i, _idx.nJ[1], sDataset);
        }
        dXmin = _data.min(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]);
        dXmax = _data.max(sDataset, _idx.nI[0], _idx.nI[1]+1, _idx.nJ[0]);
        /*for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
        {
            _x.a[i-_idx.nI[0]] = _data.getElement(i, _idx.nJ[0], sDataset);
        }*/
        sDataset = _data.getHeadLineElement(_idx.nJ[0], sDataset);
    }
    if (!nSamples || nSamples > _real.GetNx())
    {
        nSamples = _real.GetNx()/32;
    }

    // Tatsaechliche STFA
    _result = mglSTFA(_real, _imag, nSamples);

    dSampleSize = (dXmax-dXmin)/((double)_result.GetNx()-1.0);

    // Nyquist: _real.GetNx()/(dXmax-dXmin)/2.0
    dFmax = _real.GetNx()/(dXmax-dXmin)/2.0;

    // Zielcache befuellen entsprechend der Fourier-Algorithmik

    if (_target.nI[1] == -2 || _target.nI[1] == -1)
        _target.nI[1] = _target.nI[0] + _result.GetNx();//?
    if (_target.nJ[1] == -2 || _target.nJ[1] == -1)
        _target.nJ[1] = _target.nJ[0] + _result.GetNy()+2;//?

    //cerr << _result.nx << endl;
    //cerr << _result.GetNy() << endl;

    if (!_data.isCacheElement(sTargetCache))
        _data.addCache(sTargetCache, _option);
    _data.setCacheStatus(true);
    //long long int nFirstCol = _data.getCacheCols(sTargetCache, false);

    // UPDATE DATA ELEMENTS
    for (int i = 0; i < _result.GetNx(); i++)
        _data.writeToCache(i, _target.nJ[0], sTargetCache, dXmin + i*dSampleSize);
    _data.setHeadLineElement(_target.nJ[0], sTargetCache, sDataset);
    //nFirstCol++;
    dSampleSize = 2*(dFmax-dFmin) / ((double)_result.GetNy()-1.0);
    for (int i = 0; i < _result.GetNy()/2; i++)
        _data.writeToCache(i, _target.nJ[0]+1, sTargetCache, dFmin + i*dSampleSize); // Fourier f Hier ist was falsch
    _data.setHeadLineElement(_target.nJ[0]+1, sTargetCache, "f [Hz]");
    //nFirstCol++;

    for (int i = 0; i < _result.GetNx(); i++)
    {
        if (i+_target.nI[0] >= _target.nI[1])
            break;
        for (int j = 0; j < _result.GetNy()/2; j++)
        {
            if (j+2+_target.nJ[0] >= _target.nJ[1])
                break;
            _data.writeToCache(_target.nI[0]+i, _target.nJ[0]+2+j, sTargetCache, _result[i+(j+_result.GetNy()/2)*_result.GetNx()]);
            if (!i)
                _data.setHeadLineElement(_target.nJ[0]+2+j, sTargetCache, "A["+toString((int)j+1)+"]");
        }
    }
    _data.setCacheStatus(false);

    return true;
}




