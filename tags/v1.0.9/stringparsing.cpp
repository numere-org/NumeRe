
# include "parser_functions.hpp"

extern value_type vAns;
extern Integration_Vars parser_iVars;
extern bool bSupressAnswer;
extern mglGraph _fontData;
extern Plugin _plugin;

// --> Verarbeitet String-Ausdruecke <--
int parser_StringParser(string& sLine, string& sCache, Datafile& _data, Parser& _parser, const Settings& _option, bool bSilent)
{
    string* sFinal;
    string sTemp = "";
    string sTemp_2 = "";
    string sObject = sCache;
    string sDummy = "";
    string sParams = "";
    unsigned int nStrings = 1;
    bool bNoQuotes = false;
    bool bPeek = false;
    bool bReturningLogicals = false;
    bool bKeepMaskedQuotes = false;
    vector<bool> vIsNoStringValue;

    sLine = " " + sLine + " ";

    if (_option.getbDebug())
        cerr << "|-> DEBUG: Parsing sLine = " + sLine << endl;
    if (matchParams(sLine, "noquotes")
        || matchParams(sLine, "nq")
        || matchParams(sLine, "peek")
        || matchParams(sLine, "print")
        || matchParams(sLine, "kmq"))
    {
        int nPos = (int)sLine.length();
        if (matchParams(sLine, "noquotes") < nPos && matchParams(sLine, "noquotes"))
            nPos = matchParams(sLine, "noquotes");
        if (matchParams(sLine, "nq") < nPos && matchParams(sLine, "nq"))
            nPos = matchParams(sLine, "nq");
        if (matchParams(sLine, "peek") < nPos && matchParams(sLine, "peek"))
            nPos = matchParams(sLine, "peek");
        if (matchParams(sLine, "kmq") < nPos && matchParams(sLine, "kmq"))
            nPos = matchParams(sLine, "kmq");
        if (matchParams(sLine, "print")  < nPos && matchParams(sLine, "print"))
            nPos = matchParams(sLine, "print");
        nPos = sLine.rfind('-', nPos);
        sParams = sLine.substr(nPos);
        sLine = sLine.substr(0, nPos);
    }

    if (matchParams(sParams, "print"))
    {
        bNoQuotes = true;
        bPeek = true;
    }
    if (matchParams(sParams, "noquotes"))
    {
        bNoQuotes = true;
    }
    if (matchParams(sParams, "nq"))
    {
        bNoQuotes = true;
    }
    if (matchParams(sParams, "kmq"))
    {
        bKeepMaskedQuotes = true;
    }
    if (matchParams(sParams, "peek"))
    {
        bPeek = true;
    }

    if (sLine.find("{") != string::npos
        && sLine.find('=') != string::npos
        && sLine.find("{") < sLine.find('=')
        && sLine.find('<') != sLine.find('=')-1
        && sLine.find('>') != sLine.find('=')-1
        && sLine.find('!') != sLine.find('=')-1
        && sLine[sLine.find('=')+1] != '=')
    {
        string sLeftSide = sLine.substr(0,sLine.find('=')+1);
        sLine.erase(0,sLine.find('=')+1);
        sLine += " -kmq";
        if (!parser_StringParser(sLine, sDummy, _data, _parser, _option, true))
            return 0;
        if (containsStrings(sLine))
        {
            sLine = sLeftSide + "{" + sLine + "}";
            parser_VectorToExpr(sLine, _option);
        }
        else
        {
            sLine = sLeftSide + sLine;
            if (!containsStrings(sLine) && !_data.containsStringVars(sLine))
                return -1;
        }
    }
    else if (sLine.find('{') != string::npos
        && sLine.find("string(") != string::npos
        && sLine.find('=') != string::npos
        && sLine.find("string(") < sLine.find('{')
        && sLine.find('=') > sLine.find("string(")
        && sLine.find('=') < sLine.find('{')
        && sLine[sLine.find('=')-1] != '<'
        && sLine[sLine.find('=')-1] != '>'
        && sLine[sLine.find('=')-1] != '!'
        && sLine[sLine.find('=')+1] != '='
        )
    {
        for (unsigned int i = 0; i < sLine.find('='); i++)
        {
            if (sLine[i] != ' ')
            {
                if (sLine.substr(i, 7) == "string(")
                {
                    i += getMatchingParenthesis(sLine.substr(i+6))+6;
                    if (sLine.find_first_not_of(' ', i) == sLine.find('='))
                    {
                        string sLeftSide = sLine.substr(0,sLine.find('=')+1);
                        sLine.erase(0,sLine.find('=')+1);
                        sLine += " -kmq";
                        if (!parser_StringParser(sLine, sDummy, _data, _parser, _option, true))
                            return 0;
                        if (containsStrings(sLine))
                        {
                            sLine = sLeftSide + "{" + sLine + "}";
                            parser_VectorToExpr(sLine, _option);
                        }
                        else
                        {
                            sLine = sLeftSide + sLine;
                            if (!containsStrings(sLine) && !_data.containsStringVars(sLine))
                                return -1;
                        }
                    }
                    break;
                }
                else
                {
                    parser_VectorToExpr(sLine, _option);
                    break;
                }
            }
        }
    }
    else if (sLine.find("{") != string::npos)
    {
        parser_VectorToExpr(sLine, _option);
    }

    /// CHANGED: Nur Rekursionen durchfuehren, wenn auch '=' in dem String gefunden wurde. Nur dann ist sie naemlich noetig.
    if (sLine.find(',') != string::npos && sLine.find('=') != string::npos)
    {
        string sStringObject = "";
        if (sLine.substr(sLine.find_first_not_of(' '), 7) == "string(")
        {
            unsigned int nPos = getMatchingParenthesis(sLine);
            nPos = sLine.find('=', nPos);
            if (nPos == string::npos)
                return 0;
            if (sLine[nPos+1] == '=')
                nPos++;
            sStringObject = sLine.substr(0, nPos + 1);
            sLine.erase(0,nPos+1);
        }
        StripSpaces(sLine);
        /*if (sLine != getLastArgument(sLine, false))
        {
            string sRecursion = "";
            string sParsed = "";
            while (sLine.length())
            {
                sRecursion = getLastArgument(sLine, true);
                if (sLine.length() && sRecursion.find('=') != string::npos)
                {
                    sRecursion += " -kmq";
                    if (!parser_StringParser(sRecursion, sDummy, _data, _parser, _option, true))
                        return 0;
                }
                if (sParsed.length())
                    sRecursion += ", ";
                sParsed = sRecursion + sParsed;
            }
            sLine = sParsed;
        }*/
        if (sLine != getNextArgument(sLine, false))
        {
            string sRecursion = "";
            string sParsed = "";
            while (sLine.length())
            {
                sRecursion = getNextArgument(sLine, true);
                if (sLine.length() || sRecursion.find('=') != string::npos)
                {
                    sRecursion += " -kmq";
                    if (!parser_StringParser(sRecursion, sDummy, _data, _parser, _option, true))
                        return 0;
                }
                if (sParsed.length())
                    sParsed += ", ";
                sParsed += sRecursion;
            }
            sLine = sParsed;
        }
        if (sStringObject.length())
            sLine = sStringObject + sLine;
    }
    //cerr << sLine << endl;

    unsigned int n_pos = 0;

    while (sLine.find("to_string(", n_pos) != string::npos)
    {
        n_pos = sLine.find("to_string(", n_pos);
        unsigned int nPos = n_pos + 9;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 11))))
        {
            string sToString = sLine.substr(n_pos+10, nPos-n_pos-10);
            if (containsStrings(sToString) || _data.containsStringVars(sToString))
            {
                sToString += " -nq kmq";
                if (!parser_StringParser(sToString, sDummy, _data, _parser, _option, true))
                    return 0;
                for (unsigned int i = 0; i < sToString.length(); i++)
                {
                    if (i && sToString[i] == '"' && sToString[i-1] != '\\')
                        sToString.insert(i,1,'\\');
                }
            }
            sLine = sLine.substr(0,n_pos) + "\"" + sToString + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }
    while (sLine.find("to_uppercase(", n_pos) != string::npos)
    {
        n_pos = sLine.find("to_uppercase(", n_pos);
        unsigned int nPos = n_pos + 12;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 14))))
        {
            string sToString = sLine.substr(n_pos+13, nPos-n_pos-13);
            if (containsStrings(sToString) || _data.containsStringVars(sToString))
            {
                sToString += " -nq kmq";
                if (!parser_StringParser(sToString, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            sLine = sLine.substr(0,n_pos) + "\"" + toUpperCase(sToString) + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }
    while (sLine.find("to_lowercase(", n_pos) != string::npos)
    {
        n_pos = sLine.find("to_lowercase(", n_pos);
        unsigned int nPos = n_pos + 12;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 14))))
        {
            string sToString = sLine.substr(n_pos+13, nPos-n_pos-13);
            if (containsStrings(sToString) || _data.containsStringVars(sToString))
            {
                sToString += " -nq kmq";
                if (!parser_StringParser(sToString, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            sLine = sLine.substr(0,n_pos) + "\"" + toLowerCase(sToString) + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("string_cast(", n_pos) != string::npos)
    {
        n_pos = sLine.find("string_cast(", n_pos);
        unsigned int nPos = n_pos + 11;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 13))))
        {
            string sToString = sLine.substr(n_pos+12, nPos-n_pos-12);
            if (sToString.find('"') != string::npos || sToString.find('#') != string::npos)
            {
                sToString += " -nq";
                if (!parser_StringParser(sToString, sDummy, _data, _parser, _option, true))
                    return 0;
                for (unsigned int i = 0; i < sToString.length(); i++)
                {
                    if (i && sToString[i] == '"' && sToString[i-1] != '\\')
                        sToString.insert(i,1,'\\');
                }
            }
            sLine = sLine.substr(0,n_pos) + "\"" + sToString + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("to_char(", n_pos) != string::npos)
    {
        n_pos = sLine.find("to_char(", n_pos);
        unsigned int nPos = n_pos + 7;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 9))))
        {
            string sToChar = sLine.substr(n_pos+8, nPos-n_pos-8);
            int nChars = 0;
            value_type* v = 0;
            if (containsStrings(sToChar) || _data.containsStringVars(sToChar))
            {
                sToChar += " -nq";
                if (!parser_StringParser(sToChar, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            // --> Moeglicherweise erscheint nun "{{". Dies muss ersetzt werden <--
            if (sToChar.find("{") != string::npos)
            {
                parser_VectorToExpr(sToChar, _option);
            }
            _parser.SetExpr(sToChar);
            v = _parser.Eval(nChars);
            sToChar = "";
            for (int i = 0; i < nChars; i++)
            {
                sToChar += (char)((int)v[i]);
            }
            sLine = sLine.substr(0,n_pos) + "\"" + sToChar + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("to_value(", n_pos) != string::npos)
    {
        n_pos = sLine.find("to_value(", n_pos);
        unsigned int nPos = n_pos + 8;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 10))))
        {
            string sToValue = sLine.substr(n_pos+9, nPos-n_pos-9) + " -nq";
            if (!parser_StringParser(sToValue, sDummy, _data, _parser, _option, true))
                return 0;
            else
                sLine = sLine.substr(0,n_pos) + sToValue + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("to_cmd(", n_pos) != string::npos)
    {
        n_pos = sLine.find("to_cmd(", n_pos) + 6;
        if (isInQuotes(sLine, n_pos))
            continue;
        unsigned int nParPos = getMatchingParenthesis(sLine.substr(n_pos));
        if (nParPos == string::npos)
            throw UNMATCHED_PARENTHESIS;
        if (!isInQuotes(sLine, nParPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sCmdString = sLine.substr(n_pos+1, nParPos-1);
            StripSpaces(sCmdString);
            if (containsStrings(sCmdString) || _data.containsStringVars(sCmdString))
            {
                sCmdString += " -nq";
                parser_StringParser(sCmdString, sDummy, _data, _parser, _option, true);
                sCache = "";
            }
            sLine = sLine.substr(0, n_pos-6) + sCmdString + sLine.substr(n_pos + nParPos+1);
            n_pos -= 5;
        }
    }

    n_pos = 0;
    while (sLine.find("is_string(", n_pos) != string::npos)
    {
        n_pos = sLine.find("is_string(", n_pos);
        unsigned int nPos = n_pos + 9;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true)
            && !isInQuotes(sLine, n_pos, true)
            && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 11)))
            && (containsStrings(sLine.substr(n_pos+10, nPos - n_pos-10))
                || _data.containsStringVars(sLine.substr(n_pos+10, nPos - n_pos-10)))/*sLine.substr(n_pos, nPos - n_pos).find('"') != string::npos
                || sLine.substr(n_pos, nPos - n_pos).find('#') != string::npos
                || sLine.substr(n_pos, nPos - n_pos).find("string(") != string::npos)*/)
            sLine = sLine.substr(0, n_pos) + "true" + sLine.substr(nPos+1);
        else if (isInQuotes(sLine, nPos)
            || isInQuotes(sLine, n_pos))
            n_pos++;
        else
            sLine = sLine.substr(0, n_pos) + "false" + sLine.substr(nPos+1);
    }

    n_pos = 0;
    while (sLine.find("findfile(", n_pos) != string::npos)
    {
        n_pos = sLine.find("findfile(", n_pos);
        unsigned int nPos = n_pos + 8;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 10))))
        {
            string sFile = sLine.substr(n_pos+9, nPos-n_pos-9) + " -nq";
            if (!parser_StringParser(sFile, sDummy, _data, _parser, _option, true))
                return 0;
            else
            {
                FileSystem _fSys;
                _fSys.setTokens(_option.getTokenPaths());
                _fSys.setPath(_option.getExePath(), false, _option.getExePath());
                if (sFile.rfind('.') != string::npos)
                    _fSys.declareFileType(sFile.substr(sFile.rfind('.')));
                sFile = _fSys.ValidFileName(sFile, sFile.substr(sFile.rfind('.')));
                sLine = sLine.substr(0,n_pos) + toString((bool)fileExists(sFile)) + sLine.substr(nPos+1);
            }
        }
    } /*findfile("FILENAME")*/

    n_pos = 0;
    //cerr << "sline = " << sLine << endl;

    if (sLine.find('=') != string::npos
        && sLine.find('=') != sLine.find("==")
        && sLine.find('=') != sLine.find("!=")+1
        && sLine.find('=') != sLine.find("<=")+1
        && sLine.find('=') != sLine.find(">=")+1
        && !sObject.length()
        && !isInQuotes(sLine, sLine.find('=')))
    {
        if (sLine.substr(0,sLine.find('=')).find("data(") != string::npos || _data.containsCacheElements(sLine.substr(0, sLine.find('='))))
        {
            sObject = sLine.substr(0,sLine.find('='));
            sLine.erase(0,sLine.find('=')+1);
        }
    }

    if (sLine.find('=') != string::npos
        && sLine.find('=') != sLine.find("==")
        && sLine.find('=') != sLine.find("!=")+1
        && sLine.find('=') != sLine.find("<=")+1
        && sLine.find('=') != sLine.find(">=")+1
        && !sObject.length()
        && !isInQuotes(sLine, sLine.find('=')))
        n_pos = sLine.find('=') + 1;

    //cerr << "sline = " << sLine << endl;
    //cerr << "n_pos = " << n_pos << endl;

    if (_data.containsStringVars(sLine.substr(n_pos)))
        _data.getStringValues(sLine, n_pos);
    //cerr << "replacedline = " << sLine << endl;
    while (sLine.find("string(", n_pos) != string::npos)
    {
        n_pos = sLine.find("string(", n_pos);
        unsigned int nPos = n_pos + 6;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sString = "";
            if (nPos-n_pos-7 > 0)
            {
                sString = sLine.substr(n_pos+7, nPos-n_pos-7);
                StripSpaces(sString);
            }
            if (sString.length())
            {
                if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                    return 0;
                if (sString.find(':') != string::npos)
                {
                    int i1 = 0, i2 = 0, nCol = 0;
                    string s1 = "(" + sString + ")", s2 = "", sCol = "";
                    parser_SplitArgs(s1, s2, ':', _option);
                    if (s2.find(',') != string::npos)
                        parser_SplitArgs(s2, sCol, ',', _option, true);
                    if (parser_ExprNotEmpty(s1))
                    {
                        _parser.SetExpr(s1);
                        i1 = (unsigned int)_parser.Eval()-1;
                    }
                    if (parser_ExprNotEmpty(sCol))
                    {
                        _parser.SetExpr(sCol);
                        nCol = (unsigned int)_parser.Eval()-1;
                    }
                    if (parser_ExprNotEmpty(s2))
                    {
                        _parser.SetExpr(s2);
                        i2 = (unsigned int)_parser.Eval();
                    }
                    else
                        i2 = _data.getStringElements(nCol);
                    parser_CheckIndices(i1, i2);
                    if (parser_CheckMultArgFunc(sLine.substr(0,n_pos), sLine.substr(nPos+1)))
                    {
                        string sLeft = sLine.substr(0, n_pos);
                        StripSpaces(sLeft);
                        if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "num(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + toString((int)(i2-i1)) + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "max(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.maxString(i1,i2,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "min(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.minString(i1,i2,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "sum(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.sumString(i1,i2,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                    }
                    else
                    {
                        if (_data.getStringElements(nCol))
                        {
                            sString = "";
                            for (int i = i1; i < i2; i++)
                            {
                                sString += "\"" +  _data.readString((unsigned int)i, nCol) + "\"";
                                if (i < i2-1)
                                    sString += ", ";
                            }
                            sLine = sLine.substr(0, n_pos) + "{"+sString+"}" + sLine.substr(nPos+1);
                        }
                        else
                            sLine = sLine.substr(0, n_pos) + "\"\"" + sLine.substr(nPos+1);
                    }
                }
                else
                {
                    unsigned int nIndex = string::npos, nCol = 0;
                    string s1 = "(" + sString + ")", sCol = "";
                    if (s1.find(',') != string::npos)
                        parser_SplitArgs(s1, sCol, ',', _option);
                    if (parser_ExprNotEmpty(sCol))
                    {
                        _parser.SetExpr(sCol);
                        nCol = (unsigned int)_parser.Eval()-1;
                    }
                    if (parser_ExprNotEmpty(s1))
                    {
                        _parser.SetExpr(s1);
                        nIndex = (unsigned int)_parser.Eval()-1;
                    }


                    if (parser_CheckMultArgFunc(sLine.substr(0,n_pos), sLine.substr(nPos+1)))
                    {
                        string sLeft = sLine.substr(0, n_pos);

                        StripSpaces(sLeft);
                        if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "num(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "1" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "max(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString(nIndex,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "min(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString(nIndex,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                        else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "sum(")
                            sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString(nIndex,nCol) + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                    }
                    else
                    {
                        _parser.SetExpr(sString);
                        sLine = sLine.substr(0,n_pos) + "\"" + _data.readString(nIndex,nCol) + "\"" + sLine.substr(nPos+1);
                    }
                }
            }
            else
            {
                //cerr << sLine.substr(0,n_pos) << "***" << sLine.substr(nPos+1) << endl;
                if (parser_CheckMultArgFunc(sLine.substr(0,n_pos), sLine.substr(nPos+1)))
                {
                    string sLeft = sLine.substr(0, n_pos);
                    StripSpaces(sLeft);
                    if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "num(")
                        sLine = sLeft.substr(0, sLeft.length()-4) + "1" + sLine.substr(sLine.find(')', nPos+1)+1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "max(")
                        sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString() + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "min(")
                        sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString() + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                    else if (sLeft.length() > 3 && sLeft.substr(sLeft.length()-4) == "sum(")
                        sLine = sLeft.substr(0, sLeft.length()-4) + "\"" + _data.readString() + "\"" + sLine.substr(sLine.find(')', nPos+1)+1);
                }
                else
                    sLine = sLine.substr(0,n_pos) + "\"" + _data.readString() + "\"" + sLine.substr(nPos+1);
            }
        }
        n_pos++;
        //cerr << sLine << endl;
    }

    n_pos = 0;
    while (sLine.find("split(", n_pos) != string::npos)
    {
        n_pos = sLine.find("split(", n_pos);
        unsigned int nPos = n_pos + 5;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 7))))
        {
            string sStrToSplit = sLine.substr(n_pos+6, nPos-n_pos-6);
            if (!parser_StringParser(sStrToSplit, sDummy, _data, _parser, _option, true))
                return 0;
            else
            {
                string sToSplit = getNextArgument(sStrToSplit, true) + " -nq";
                string sSepChar = "";
                if (getNextArgument(sStrToSplit, false).length())
                    sSepChar = getNextArgument(sStrToSplit, true) + " -nq";
                else
                    return 0;
                if (!parser_StringParser(sToSplit, sDummy, _data, _parser, _option, true))
                    return 0;
                if (!parser_StringParser(sSepChar, sDummy, _data, _parser, _option, true))
                    return 0;
                boost::char_separator<char> cSep(sSepChar.substr(0,1).c_str());
                tokenizer<char_separator<char> > tok(sToSplit, cSep);
                sStrToSplit = "";
                for (tokenizer<char_separator<char> >::iterator iter = tok.begin(); iter != tok.end(); ++iter)
                {
                    sStrToSplit += "\"";
                    sStrToSplit += *iter;
                    sStrToSplit += "\", ";
                }
                if (sStrToSplit.length())
                    sLine = sLine.substr(0,n_pos) + "{" + sStrToSplit.substr(0,sStrToSplit.length()-2) + "}" + sLine.substr(nPos+1);
                else
                    return 0;
            }
        }
        n_pos++;
        //cerr << sLine << endl;
    }

    n_pos = 0;
    while (sLine.find("strlen(", n_pos) != string::npos)
    {
        n_pos = sLine.find("strlen(", n_pos);
        unsigned int nPos = n_pos + 6;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sLength = sLine.substr(n_pos+7, nPos-n_pos-7) + " -nq";
            if (!parser_StringParser(sLength, sDummy, _data, _parser, _option, true))
                return 0;
            else
                sLine = sLine.substr(0,n_pos) + toString((int)sLength.length()) + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("ascii(", n_pos) != string::npos)
    {
        n_pos = sLine.find("ascii(", n_pos);
        unsigned int nPos = n_pos + 5;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 7))))
        {
            string sAscii = sLine.substr(n_pos+6, nPos-n_pos-6) + " -nq";
            if (!parser_StringParser(sAscii, sDummy, _data, _parser, _option, true))
                return 0;
            string sCodes = "";
            for (unsigned int i = 0; i < sAscii.length(); i++)
            {
                sCodes += toString((int)sAscii[i]) + ",";
            }
            if (sCodes.length())
                sLine = sLine.substr(0,n_pos) + "{" + sCodes.substr(0,sCodes.length()-1) + "}" + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos) + sLine.substr(nPos+1);
        }
        n_pos++;
    }//ascii(STRING)

    n_pos = 0;
    while (sLine.find("strfnd(", n_pos) != string::npos)
    {
        unsigned int nPosition = 0;
        n_pos = sLine.find("strfnd(", n_pos);
        unsigned int nPos = n_pos + 6;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sStrFind = sLine.substr(n_pos+7, nPos-n_pos-7);
            if (!parser_StringParser(sStrFind, sDummy, _data, _parser, _option, true))
                return 0;
            else
            {
                string sToFind = getNextArgument(sStrFind, true) + " -nq";
                string sToFindIn = getNextArgument(sStrFind, true) + " -nq";
                string sPos = "";
                if (getNextArgument(sStrFind, false).length())
                    sPos = getNextArgument(sStrFind, true);
                if (!parser_StringParser(sToFind, sDummy, _data, _parser, _option, true))
                    return 0;
                if (!parser_StringParser(sToFindIn, sDummy, _data, _parser, _option, true))
                    return 0;
                if (sPos.length() && (containsStrings(sPos) || _data.containsStringVars(sPos)))
                {
                    sPos += " -nq";
                    if (!parser_StringParser(sPos, sDummy, _data, _parser, _option, true))
                        return 0;
                }
                if (parser_ExprNotEmpty(sPos))
                {
                    _parser.SetExpr(sPos);
                    nPosition = (unsigned int)(_parser.Eval()-1);
                    if (nPosition == string::npos || nPosition > sToFindIn.length())
                        nPosition = 0;
                }

                sToFindIn = sToFindIn.substr(sToFindIn.find('"')+1, sToFindIn.rfind('"')-sToFindIn.find('"')-1);
                sLine = sLine.substr(0,n_pos) + toString((int)sToFindIn.find(sToFind, nPosition)+1) + sLine.substr(nPos+1);
            }
        }
        n_pos++;
    }//strfnd("string1", "string2", POS)

    n_pos = 0;
    while (sLine.find("strrfnd(", n_pos) != string::npos)
    {
        unsigned int nPosition = string::npos;
        n_pos = sLine.find("strrfnd(", n_pos);
        unsigned int nPos = n_pos + 7;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 9))))
        {
            string sStrFind = sLine.substr(n_pos+8, nPos-n_pos-8);
            if (!parser_StringParser(sStrFind, sDummy, _data, _parser, _option, true))
                return 0;
            else
            {
                string sToFind = getNextArgument(sStrFind, true) + " -nq";
                string sToFindIn = getNextArgument(sStrFind, true) + " -nq";
                string sPos = "";
                if (getNextArgument(sStrFind, false).length())
                    sPos = getNextArgument(sStrFind, true);
                if (!parser_StringParser(sToFind, sDummy, _data, _parser, _option, true))
                    return 0;
                if (!parser_StringParser(sToFindIn, sDummy, _data, _parser, _option, true))
                    return 0;
                if (sPos.length() && (containsStrings(sPos) || _data.containsStringVars(sPos)))
                {
                    sPos += " -nq";
                    if (!parser_StringParser(sPos, sDummy, _data, _parser, _option, true))
                        return 0;
                }
                if (parser_ExprNotEmpty(sPos))
                {
                    _parser.SetExpr(sPos);
                    nPosition = (unsigned int)(_parser.Eval()-1);
                    if (nPosition == string::npos)
                        nPosition = 0;
                }

                sToFindIn = sToFindIn.substr(sToFindIn.find('"')+1, sToFindIn.rfind('"')-sToFindIn.find('"')-1);
                sLine = sLine.substr(0,n_pos) + toString((int)sToFindIn.rfind(sToFind, nPosition)+1) + sLine.substr(nPos+1);
            }
        }
        n_pos++;
    }//strrfnd("string1", "string2", POS)

    n_pos = 0;
    while (sLine.find("findparam(", n_pos) != string::npos)
    {
        n_pos = sLine.find("findparam(", n_pos);
        unsigned int nPos = n_pos + 9;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 11))))
        {
            string sParamFind = sLine.substr(n_pos+10, nPos-n_pos-10) + " -kmq";
            if (!parser_StringParser(sParamFind, sDummy, _data, _parser, _option, true))
                return 0;
            else
            {
                string sParam = getNextArgument(sParamFind, true) + " -nq";
                string sString = getNextArgument(sParamFind, true) + " -nq";
                string sChar = "";
                if (getNextArgument(sParamFind, false).length())
                    sChar = getNextArgument(sParamFind, true) + " -nq";
                if (!parser_StringParser(sParam, sDummy, _data, _parser, _option, true))
                    return 0;
                if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                    return 0;
                if (sChar.length())
                {
                    if (!parser_StringParser(sChar, sDummy, _data, _parser, _option, true))
                        return 0;
                }
                if (sChar.length())
                {
                    if (matchParams(sString, sParam, sChar[0]))
                        sLine = sLine.substr(0,n_pos) + toString((int)matchParams(sString, sParam, sChar[0])+1) + sLine.substr(nPos+1);
                    else
                        sLine = sLine.substr(0,n_pos) + "0" + sLine.substr(nPos+1);

                }
                else
                {
                    if (matchParams(sString, sParam))
                        sLine = sLine.substr(0,n_pos) + toString((int)matchParams(sString, sParam)) + sLine.substr(nPos+1);
                    else
                        sLine = sLine.substr(0,n_pos) + "0" + sLine.substr(nPos+1);
                }
            }
        }
        n_pos++;
    }//findparam("param", "string","char")

    //cerr << "sLine = " << sLine << endl;
    n_pos = 0;
    while (sLine.find("substr(", n_pos) != string::npos)
    {
        n_pos = sLine.find("substr(", n_pos);
        unsigned int nPos = n_pos + 6;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sSubstr = sLine.substr(n_pos+7, nPos-n_pos-7);
            //cerr << "substr: " << sSubstr << endl;
            string sString = getNextArgument(sSubstr, true);
            string sIndex = getNextArgument(sSubstr, true);
            string sLength = "-1";
            if (getNextArgument(sSubstr, false).length())
                sLength = getNextArgument(sSubstr, true);
            unsigned int nIndex = 0;
            unsigned int nLength = 0;
            if (containsStrings(sLength) || _data.containsStringVars(sLength))
            {
                sLength += " -nq";
                if (!parser_StringParser(sLength, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            if (containsStrings(sIndex) || _data.containsStringVars(sIndex))
            {
                sIndex += " -nq";
                if (!parser_StringParser(sIndex, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            sString += " -nq";
            if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                return 0;
            _parser.SetExpr(sIndex);
            nIndex = (unsigned int)_parser.Eval()-1;
            if (nIndex == string::npos)
                nIndex = 0;
            _parser.SetExpr(sLength);
            nLength = (unsigned int)_parser.Eval();
            sLine = sLine.substr(0,n_pos) + "\"" + sString.substr(nIndex, nLength) + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }//substr("string", index, length)

    n_pos = 0;
    while (sLine.find("repeat(", n_pos) != string::npos)
    {
        n_pos = sLine.find("repeat(", n_pos);
        unsigned int nPos = n_pos + 6;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sReplace = sLine.substr(n_pos+7, nPos-n_pos-7);
            string sString = getNextArgument(sReplace, true);
            string sNumber = getNextArgument(sReplace, true);
            unsigned int nNumber = 0;
            if (containsStrings(sNumber) || _data.containsStringVars(sNumber))
            {
                sNumber += " -nq";
                if (!parser_StringParser(sNumber, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            sString += " -nq kmq";
            if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                return 0;
            _parser.SetExpr(sNumber);
            nNumber = (unsigned int)_parser.Eval();
            if (nNumber == string::npos)
                nNumber = 0;
            sReplace.clear();
            for (unsigned int i = 0; i < nNumber; i++)
                sReplace += sString;

            sLine = sLine.substr(0,n_pos) + "\"" + sReplace + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }//replace("string", index, length, "string")

    n_pos = 0;
    while (sLine.find("replace(", n_pos) != string::npos)
    {
        n_pos = sLine.find("replace(", n_pos);
        unsigned int nPos = n_pos + 7;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 9))))
        {
            string sReplace = sLine.substr(n_pos+8, nPos-n_pos-8);
            string sString = getNextArgument(sReplace, true);
            string sIndex = getNextArgument(sReplace, true);
            string sLength = getNextArgument(sReplace, true);
            sReplace = getNextArgument(sReplace, true);
            unsigned int nIndex = 0;
            unsigned int nLength = 0;
            if (containsStrings(sLength) || _data.containsStringVars(sLength))
            {
                sLength += " -nq";
                if (!parser_StringParser(sLength, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            if (containsStrings(sIndex) || _data.containsStringVars(sIndex))
            {
                sIndex += " -nq";
                if (!parser_StringParser(sIndex, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            sString += " -nq kmq";
            if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                return 0;
            sReplace += " -nq kmq";
            if (!parser_StringParser(sReplace, sDummy, _data, _parser, _option, true))
                return 0;
            _parser.SetExpr(sIndex);
            nIndex = (unsigned int)_parser.Eval()-1;
            if (nIndex == string::npos)
                nIndex = 0;
            _parser.SetExpr(sLength);
            nLength = (unsigned int)_parser.Eval();
            sLine = sLine.substr(0,n_pos) + "\"" + sString.replace(nIndex, nLength, sReplace) + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }//replace("string", index, length, "string")

    n_pos = 0;
    while (sLine.find("replaceall(", n_pos) != string::npos)
    {
        n_pos = sLine.find("replaceall(", n_pos);
        unsigned int nPos = n_pos + 10;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 12))))
        {
            unsigned int nPosition1 = 0;
            unsigned int nPosition2 = string::npos;
            string sReplace = sLine.substr(n_pos+11, nPos-n_pos-11);
            string sString = getNextArgument(sReplace, true);
            string sString2 = getNextArgument(sReplace, true);
            string sString3 = getNextArgument(sReplace, true);
            if (getNextArgument(sReplace, false).length())
            {
                string sPos1 = getNextArgument(sReplace, true);
                if (containsStrings(sPos1) || _data.containsStringVars(sPos1))
                {
                    sPos1 += " -nq";
                    if (!parser_StringParser(sPos1, sDummy, _data, _parser, _option, true))
                        return 0;
                }
                _parser.SetExpr(sPos1);
                nPosition1 = (unsigned int)_parser.Eval()-1;
                if (nPosition1 == string::npos)
                    nPosition1 = 0;
            }
            if (getNextArgument(sReplace, false).length())
            {
                string sPos2 = getNextArgument(sReplace, true);
                if (containsStrings(sPos2) || _data.containsStringVars(sPos2))
                {
                    sPos2 += " -nq";
                    if (!parser_StringParser(sPos2, sDummy, _data, _parser, _option, true))
                        return 0;
                }
                _parser.SetExpr(sPos2);
                nPosition2 = (unsigned int)_parser.Eval()-1;
            }
            sString += " -nq kmq";
            if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                return 0;
            sString2 += " -nq kmq";
            if (!parser_StringParser(sString2, sDummy, _data, _parser, _option, true))
                return 0;
            sString3 += " -nq kmq";
            if (!parser_StringParser(sString3, sDummy, _data, _parser, _option, true))
                return 0;
            if (nPosition2 == string::npos)
                nPosition2 = sString.length();
            for (unsigned int i = nPosition1; i < nPosition2; i++)
            {
                if (i == sString.length())
                    break;
                if (sString.substr(i,sString2.length()) == sString2)
                {
                    sString.replace(i,sString2.length(), sString3);
                    nPosition2 += sString3.length()-sString2.length();
                    i += sString3.length()-1;
                }
            }

            sLine = sLine.substr(0,n_pos) + "\"" + sString + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }//replaceall("string", "string", "string"[, pos, pos])

    n_pos = 0;
    while (sLine.find("char(", n_pos) != string::npos)
    {
        n_pos = sLine.find("char(", n_pos);
        unsigned int nPos = n_pos + 4;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 6))))
        {
            string sCharString = sLine.substr(n_pos+5, nPos-n_pos-5);
            string sString = getNextArgument(sCharString, true);
            string sIndex = getNextArgument(sCharString, true);
            unsigned int nIndex = 0;
            if (containsStrings(sIndex) || _data.containsStringVars(sIndex))
            {
                sIndex += " -nq";
                if (!parser_StringParser(sIndex, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            sString += " -nq";
            if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                return 0;
            _parser.SetExpr(sIndex);
            nIndex = (unsigned int)_parser.Eval()-1;
            if (nIndex == string::npos || nIndex >= sString.length())
                nIndex = 0;
            if (nIndex < sString.length())
                sLine = sLine.substr(0,n_pos) + "\"" + sString[nIndex] + "\"" + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos) + sLine.substr(nPos+1);
        }
        n_pos++;
    }//char("string", index)

    n_pos = 0;
    while (sLine.find("getopt(", n_pos) != string::npos)
    {
        n_pos = sLine.find("getopt(", n_pos);
        unsigned int nPos = n_pos + 6;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 8))))
        {
            string sGetOpt = sLine.substr(n_pos+7, nPos-n_pos-7);
            string sString = getNextArgument(sGetOpt, true);
            string sIndex = getNextArgument(sGetOpt, true);
            unsigned int nIndex = 0;
            if (containsStrings(sIndex) || _data.containsStringVars(sIndex))
            {
                sIndex += " -nq";
                if (!parser_StringParser(sIndex, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            sString += " -nq";
            if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                return 0;
            _parser.SetExpr(sIndex);
            sString += " ";
            nIndex = (unsigned int)_parser.Eval()-1;
            if (nIndex != string::npos)
            {
                sLine = sLine.substr(0,n_pos) + "\"" + getArgAtPos(sString, nIndex) + "\"" + sLine.substr(nPos+1);
            }
            else
                sLine = sLine.substr(0,n_pos) + "\"\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }//getopt("string", index)

    n_pos = 0;
    while (sLine.find("getfilelist(", n_pos) != string::npos)
    {
        n_pos = sLine.find("getfilelist(", n_pos);
        unsigned int nPos = n_pos + 11;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 13))))
        {
            string sFileScheme = "";
            string sFlags = sLine.substr(n_pos+12, nPos-n_pos-12);
            int nFlags = 0;
            sFileScheme = getNextArgument(sFlags, true);
            if (sFlags.length())
            {
                _parser.SetExpr(sFlags);
                nFlags = (int)_parser.Eval();
            }
            if (containsStrings(sFileScheme) || _data.containsStringVars(sFileScheme))
            {
                sFileScheme += " -nq";
                if (!parser_StringParser(sFileScheme, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            vector<string> vFileList = getFileList(sFileScheme, _option, nFlags);
            sFileScheme.clear();
            for (unsigned int i = 0; i < vFileList.size(); i++)
            {
                sFileScheme += "\"" + vFileList[i] + "\"";
                if (i < vFileList.size()-1)
                    sFileScheme += ", ";
            }
            if (!sFileScheme.length())
                sLine = sLine.substr(0,n_pos) + "\"\"" + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos) + "{" + sFileScheme+ "}" + sLine.substr(nPos+1);
        }
        n_pos++;
    }//getfilelist("path/with/wildcards")

    n_pos = 0;
    while (sLine.find("getfolderlist(", n_pos) != string::npos)
    {
        n_pos = sLine.find("getfolderlist(", n_pos);
        unsigned int nPos = n_pos + 13;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 15))))
        {
            string sFileScheme = "";
            string sFlags = sLine.substr(n_pos+14, nPos-n_pos-14);
            int nFlags = 0;
            sFileScheme = getNextArgument(sFlags, true);
            if (sFlags.length())
            {
                _parser.SetExpr(sFlags);
                nFlags = (int)_parser.Eval();
            }
            if (containsStrings(sFileScheme) || _data.containsStringVars(sFileScheme))
            {
                sFileScheme += " -nq";
                if (!parser_StringParser(sFileScheme, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            vector<string> vFileList = getFolderList(sFileScheme, _option, nFlags);
            sFileScheme.clear();
            for (unsigned int i = 0; i < vFileList.size(); i++)
            {
                sFileScheme += "\"" + vFileList[i] + "\"";
                if (i < vFileList.size()-1)
                    sFileScheme += ", ";
            }
            if (!sFileScheme.length())
                sLine = sLine.substr(0,n_pos) + "\"\"" + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos) + "{" + sFileScheme+ "}" + sLine.substr(nPos+1);
        }
        n_pos++;
    }//getfilelist("path/with/wildcards")

    n_pos = 0;
    while (sLine.find("getindices(", n_pos) != string::npos)
    {
        n_pos = sLine.find("getindices(", n_pos);
        unsigned int nPos = n_pos + 10;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 12))))
        {
            string _sObject = sLine.substr(n_pos+11, nPos-n_pos-11);
            if (!parser_StringParser(_sObject, sDummy, _data, _parser, _option, true))
                return 0;
            string sType = _sObject;
            int nType = 0;
            _sObject = getNextArgument(sType, true);
            if (!sType.length())
            {
                sType = "0";
            }
            if (_sObject[0] == '"')
                _sObject.erase(0,1);
            if (_sObject[_sObject.length()-1] == '"')
                _sObject.erase(_sObject.length()-1);
            StripSpaces(_sObject);
            if (containsStrings(sType))
            {
                sType += " -nq";
                parser_StringParser(sType, sDummy, _data, _parser, _option, true);
            }
            _parser.SetExpr(sType);
            nType = (int)_parser.Eval();
            if (nType < -1 || nType > 2)
                nType = 0;
            if (_sObject.find("string(") != string::npos)
            {
                vector<double> vIndices;
                if (_sObject.find(':', _sObject.find("string(")+7) != string::npos)
                {
                    string s[3];
                    s[0] = _sObject.substr(_sObject.find("string(")+6);
                    parser_SplitArgs(s[0], s[1], ':', _option);
                    StripSpaces(s[0]);
                    StripSpaces(s[1]);
                    if (s[1].find(',') != string::npos)
                        parser_SplitArgs(s[1],s[2],',',_option, true);
                    if (!s[0].length())
                        vIndices.push_back(1.0);
                    else
                    {
                        _parser.SetExpr(s[0]);
                        vIndices.push_back(_parser.Eval());
                    }
                    vIndices.push_back(-1.0);
                    if (!s[2].length())
                        vIndices.push_back(1.0);
                    else
                    {
                        _parser.SetExpr(s[2]);
                        vIndices.push_back(_parser.Eval());
                    }
                    if (!s[1].length() && nType == -1)
                        vIndices[1] = -1.0;
                    else if (!s[1].length() && nType > 0)
                        vIndices[1] = vIndices.front()+1;
                    else if (!s[1].length())
                    {
                        vIndices[1] = _data.getStringElements((unsigned int)vIndices.back()-1);
                        if (!vIndices[1])
                            vIndices[1] = vIndices[0];
                    }
                    else
                    {
                        _parser.SetExpr(s[1]);
                        vIndices[1] = _parser.Eval();
                    }
                }
                else
                {
                    string s1 = _sObject.substr(_sObject.find("string(")+7, getMatchingParenthesis(_sObject.substr(_sObject.find("string(")+6))-1), sCol = "";
                    if (s1.find(',') != string::npos)
                        parser_SplitArgs(s1, sCol, ',', _option, true);

                    vIndices.push_back(1.0);
                    if (parser_ExprNotEmpty(sCol))
                    {
                        _parser.SetExpr(sCol);
                        vIndices.push_back(_parser.Eval());
                    }
                    else
                        vIndices.push_back(1.0);
                    if (parser_ExprNotEmpty(s1))
                    {
                        _parser.SetExpr(s1);
                        vIndices[0] = _parser.Eval();
                    }
                    else
                        vIndices[0] = _data.getStringElements((unsigned int)vIndices.back()-1);
                }
                _parser.SetVectorVar("indices["+ replaceToVectorname(_sObject) +"]", vIndices);
                sLine = sLine.substr(0,n_pos) + "indices["+ replaceToVectorname(_sObject) +"]" + sLine.substr(nPos+1);
                n_pos++;
                continue;
            }
            if (_sObject.find("data(") == string::npos && !_data.containsCacheElements(_sObject))
            {
                sLine = sLine.substr(0,n_pos) + "nan" + sLine.substr(nPos+1);
                n_pos++;
                continue;
            }
            Indices _mIndex = parser_getIndices(_sObject, _parser, _data, _option);
            if (_mIndex.nI[0] == -1 || _mIndex.nJ[0] == -1)
            {
                sLine = sLine.substr(0,n_pos) + "nan" + sLine.substr(nPos+1);
                n_pos++;
                continue;
            }
            if (nType > -1)
            {
                if (_mIndex.nI[1] == -1)
                    _mIndex.nI[1] = _mIndex.nI[0];
                if (_mIndex.nJ[1] == -1)
                    _mIndex.nJ[1] = _mIndex.nJ[0];
                if (_mIndex.nI[1] == -2)
                {
                    if (nType == 2)
                        _mIndex.nI[1] = _mIndex.nI[0]+1;
                }
                if (_mIndex.nJ[1] == -2)
                {
                    if (nType == 1)
                        _mIndex.nJ[1] = _mIndex.nJ[0]+1;
                }
                if (_sObject.find("data(") != string::npos)
                {
                    if (_mIndex.nI[1] == -2)
                        _mIndex.nI[1] = _data.getLines("data", false)-1;

                    if (_mIndex.nJ[1] == -2)
                        _mIndex.nJ[1] = _data.getCols("data")-1;
                }
                else
                {
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (_sObject.find(iter->first+"(") != string::npos
                            && (!_sObject.find(iter->first+"(")
                                || (_sObject.find(iter->first+"(") && checkDelimiter(_sObject.substr(sObject.find(iter->first+"(")-1, (iter->first).length()+2)))))
                        {
                            if (_mIndex.nI[1] == -2)
                                _mIndex.nI[1] = _data.getLines(iter->first, false)-1;
                            if (_mIndex.nJ[1] == -2)
                                _mIndex.nJ[1] = _data.getCols(iter->first)-1;
                        }
                    }
                }
            }
            vector<double> vIndices;
            vIndices.push_back(_mIndex.nI[0]+1);
            vIndices.push_back(_mIndex.nI[1]+1);
            vIndices.push_back(_mIndex.nJ[0]+1);
            vIndices.push_back(_mIndex.nJ[1]+1);
            _parser.SetVectorVar("indices["+replaceToVectorname(_sObject)+ "]", vIndices);
            sLine = sLine.substr(0,n_pos) + "indices["+ replaceToVectorname(_sObject) +"]" + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("getmatchingparens(", n_pos) != string::npos)
    {
        n_pos = sLine.find("getmatchingparens(", n_pos);
        unsigned int nPos = n_pos + 17;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 19))))
        {
            string _sObject = sLine.substr(n_pos+18, nPos-n_pos-18);
            if (!parser_StringParser(_sObject, sDummy, _data, _parser, _option, true))
                return 0;
            if (_sObject[0] == '"')
                _sObject.erase(0,1);
            if (_sObject[_sObject.length()-1] == '"')
                _sObject.erase(_sObject.length()-1);
            sLine = sLine.substr(0,n_pos) + toString((int)(getMatchingParenthesis(_sObject)+1)) + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("is_data(", n_pos) != string::npos)
    {
        n_pos = sLine.find("is_data(", n_pos);
        unsigned int nPos = n_pos + 7;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 9))))
        {
            string sData = sLine.substr(n_pos+8,nPos-n_pos-8);
            if (containsStrings(sData))
            {
                if (!parser_StringParser(sData, sDummy, _data, _parser, _option, true))
                    return 0;
            }
            if (sData[0] == '"')
                sData.erase(0,1);
            if (sData[sData.length()-1] == '"')
                sData.erase(sData.length()-1);
            StripSpaces(sData);
            if (sData.substr(0,5) == "data(" || _data.isCacheElement(sData))
                sLine = sLine.substr(0,n_pos) + "true" + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos) + "false" + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("data(", n_pos) != string::npos)
    {
        n_pos = sLine.find("data(", n_pos);
        unsigned int nPos = n_pos + 4;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 6))))
        {
            if (parser_CheckMultArgFunc(sLine.substr(0,n_pos),sLine.substr(nPos+1)))
            {
                if (n_pos > 4 && sLine.substr(sLine.rfind('(',n_pos)-4,5) == "norm(")
                    n_pos -= 5;
                else
                    n_pos -= 4;
                nPos++;
            }
            string sData = sLine.substr(n_pos, nPos-n_pos+1);
            //cerr << sData << endl;
            parser_GetDataElement(sData, _parser, _data, _option);
            sData += " -kmq";
            if (!parser_StringParser(sData, sDummy, _data, _parser, _option, true))
                return 0;
            StripSpaces(sData);
            sLine = sLine.substr(0,n_pos) + sData + sLine.substr(nPos+1);
        }
        n_pos++;
    }
    n_pos = 0;
    while (n_pos < sLine.length() && _data.containsCacheElements(sLine.substr(n_pos)))
    {
        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
        {
            if (sLine.find(iter->first+"(", n_pos) != string::npos)
            {
                n_pos = sLine.find(iter->first+"(", n_pos);
                unsigned int nPos = n_pos + (iter->first).length();
                if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, nPos))
                    throw UNMATCHED_PARENTHESIS;
                nPos += getMatchingParenthesis(sLine.substr(nPos));
                if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, (iter->first).length()+2))))
                {
                    if (parser_CheckMultArgFunc(sLine.substr(0,n_pos),sLine.substr(nPos+1)))
                    {
                        if (n_pos > 4 && sLine.substr(sLine.rfind('(',n_pos)-4,5) == "norm(")
                            n_pos -= 5;
                        else
                            n_pos -= 4;
                        nPos++;
                    }
                    string sData = sLine.substr(n_pos, nPos-n_pos+1);
                    parser_GetDataElement(sData, _parser, _data, _option);
                    sData += " -kmq";
                    if (!parser_StringParser(sData, sDummy, _data, _parser, _option, true))
                        return 0;
                    StripSpaces(sData)
;                    sLine = sLine.substr(0,n_pos) + sData + sLine.substr(nPos+1);
                }
            }
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("valtostr(", n_pos) != string::npos)
    {
        n_pos = sLine.find("valtostr(", n_pos);
        unsigned int nPos = n_pos + 8;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, n_pos, true) && !isInQuotes(sLine, nPos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 10))))
        {
            string sToString = sLine.substr(n_pos+9, nPos-n_pos-9);
            string sExpr = getNextArgument(sToString, true);
            string sChar = "";
            unsigned int nCount = 0;
            if (sToString.length())
            {
                sChar = getNextArgument(sToString, true);
                if (containsStrings(sChar) || _data.containsStringVars(sChar))
                {
                    sChar += " -nq kmq";
                    if (!parser_StringParser(sChar, sDummy, _data, _parser, _option, true))
                        return 0;
                }
                string sCnt = getNextArgument(sToString, true);
                if (sCnt.length())
                {
                    _parser.SetExpr(sCnt);
                    nCount = (unsigned int)fabs(_parser.Eval());
                }
            }
            if (!containsStrings(sExpr) && !_data.containsStringVars(sExpr))
            {
                int nResults = 0;
                value_type* v = 0;
                _parser.SetExpr(sExpr);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                    sToString = "{";
                else
                    sToString.clear();
                string sElement = "";
                for (int n = 0; n < nResults; n++)
                {
                    if (fabs(rint(v[n])-v[n]) < 1e-14 && fabs(v[n]) >= 1.0)
                        sElement = toString((long long int)rint(v[n]));
                    else
                        sElement = toString(v[n], _option);
                    while (sElement.length() < nCount && sChar.length())
                        sElement.insert(0,sChar);
                    sToString += sElement;
                    if (nResults > 1)
                        sToString += ",";
                }
                if (nResults > 1)
                {
                    sToString.back() = '}';
                }
            }
            else
            {
                sExpr += " -nq";
                if (!parser_StringParser(sExpr, sDummy, _data, _parser, _option, true))
                    return 0;
                while (sExpr.length() < nCount && sChar.length())
                    sExpr.insert(0,sChar);
                sToString = sExpr;
            }

            sLine = sLine.substr(0,n_pos) + "\"" + sToString + "\"" + sLine.substr(nPos+1);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("num(", n_pos) != string::npos)
    {
        n_pos = sLine.find("num(", n_pos);
        unsigned int nPos = n_pos + 3;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 5))))
        {
            string sEnum = sLine.substr(n_pos+4, nPos-n_pos-4);
            if (!parser_StringParser(sEnum, sDummy, _data, _parser, _option, true))
                return 0;
            if (containsStrings(sEnum) || _data.containsStringVars(sEnum))
                sLine = sLine.substr(0,n_pos) + toString(parser_numStrings(sEnum)) + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos+4) + sEnum + sLine.substr(nPos);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("cnt(", n_pos) != string::npos)
    {
        n_pos = sLine.find("cnt(", n_pos);
        unsigned int nPos = n_pos + 3;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 5))))
        {
            string sEnum = sLine.substr(n_pos+4, nPos-n_pos-4);
            if (!parser_StringParser(sEnum, sDummy, _data, _parser, _option, true))
                return 0;
            if (containsStrings(sEnum) || _data.containsStringVars(sEnum))
                sLine = sLine.substr(0,n_pos) + toString(parser_countStrings(sEnum)) + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos+4) + sEnum + sLine.substr(nPos);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("min(", n_pos) != string::npos)
    {
        n_pos = sLine.find("min(", n_pos);
        unsigned int nPos = n_pos + 3;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 5))))
        {
            string sMin = sLine.substr(n_pos+4, nPos-n_pos-4);
            if (!parser_StringParser(sMin, sDummy, _data, _parser, _option, true))
                return 0;
            if (containsStrings(sMin) || _data.containsStringVars(sMin))
                sLine = sLine.substr(0,n_pos) + parser_getMinString(sMin) + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos+4) + sMin + sLine.substr(nPos);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("max(", n_pos) != string::npos)
    {
        n_pos = sLine.find("max(", n_pos);
        unsigned int nPos = n_pos + 3;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 5))))
        {
            string sMax = sLine.substr(n_pos+4, nPos-n_pos-4);
            if (!parser_StringParser(sMax, sDummy, _data, _parser, _option, true))
                return 0;
            if (containsStrings(sMax) || _data.containsStringVars(sMax))
                sLine = sLine.substr(0,n_pos) + parser_getMaxString(sMax) + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos+4) + sMax + sLine.substr(nPos);
        }
        n_pos++;
    }

    n_pos = 0;
    while (sLine.find("sum(", n_pos) != string::npos)
    {
        n_pos = sLine.find("sum(", n_pos);
        unsigned int nPos = n_pos + 3;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos)
            throw UNMATCHED_PARENTHESIS;
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos, true) && !isInQuotes(sLine, n_pos, true) && (!n_pos || checkDelimiter(sLine.substr(n_pos-1, 5))))
        {
            string sSum = sLine.substr(n_pos+4, nPos-n_pos-4);
            if (!parser_StringParser(sSum, sDummy, _data, _parser, _option, true))
                return 0;
            if (containsStrings(sSum) || _data.containsStringVars(sSum))
                sLine = sLine.substr(0,n_pos) + parser_getSumString(sSum) + sLine.substr(nPos+1);
            else
                sLine = sLine.substr(0,n_pos+4) + sSum + sLine.substr(nPos);
        }
        n_pos++;
    }

    //cerr << sLine << endl;
    if (!containsStrings(sLine) && !_data.containsStringVars(sLine))
    {
        if (sObject.length() && !sCache.length())
            sCache = sObject;
        if (sLine.find("string(") != string::npos || _data.containsStringVars(sLine))
            return 0;
        if (bKeepMaskedQuotes)
        {
            int nResults = 0;
            value_type* v = 0;
            _parser.SetExpr(sLine);
            v = _parser.Eval(nResults);
            vAns = v[0];
            if (sLine.find('=') != string::npos)
                sLine.erase(0,sLine.find('=')+1);
            StripSpaces(sLine);
        }
        return -1;
    }
    n_pos = 0;
    while (sLine.find('(', n_pos) != string::npos)
    {
        n_pos = sLine.find('(', n_pos);
        unsigned int nPos = n_pos;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && !isInQuotes(sLine, n_pos))
            throw UNMATCHED_PARENTHESIS;
        if (getMatchingParenthesis(sLine.substr(nPos)) == string::npos && isInQuotes(sLine, n_pos))
        {
            n_pos++;
            continue;
        }
        nPos += getMatchingParenthesis(sLine.substr(nPos));
        if (!isInQuotes(sLine, nPos)
            && !isInQuotes(sLine, n_pos)
            && (n_pos < 6 || ( n_pos >= 6 && sLine.substr(n_pos-6,6) != "string")))
        {
            string sString = sLine.substr(n_pos+1, nPos-n_pos-1);
            if (n_pos > 0 && !checkDelimiter(sLine.substr(n_pos-1, nPos-n_pos+2)))
                return 0;
            if (containsStrings(sString) || _data.containsStringVars(sString))
            {
                if (!parser_StringParser(sString, sDummy, _data, _parser, _option, true))
                    return 0;
                else
                    sLine = sLine.substr(0,n_pos) + sString + sLine.substr(nPos+1);
            }
        }
        n_pos++;
    }

    if (sLine.find("{") != string::npos)
    {
        n_pos = 0;
        if (sLine.find('=') != string::npos
            && sLine.find('=') != sLine.find("==")
            && sLine.find('=') != sLine.find("!=")+1
            && sLine.find('=') != sLine.find("<=")+1
            && sLine.find('=') != sLine.find(">=")+1
            && !sObject.length()
            && !isInQuotes(sLine, sLine.find('=')))
            n_pos = sLine.find('=') + 1;
        string sVectortemp = sLine.substr(0,n_pos);
        sLine.erase(0,n_pos);
        parser_VectorToExpr(sLine, _option);
        sLine = sVectortemp + sLine;
    }
    StripSpaces(sLine);
    if (!sLine.length())
        return 1;
    if (sLine.find('"') == string::npos && sLine.find('#') == string::npos)
    {
        if (sObject.length() && !sCache.length())
            sCache = sObject;
        if (sLine.find("string(") != string::npos || _data.containsStringVars(sLine))
            return 0;
        return -1;
    }

    if (bSupressAnswer)
        bSilent = true;

    if (sLine.find('=') != string::npos
        && sLine.find('=') != sLine.find("==")
        && sLine.find('=') != sLine.find("!=")+1
        && sLine.find('=') != sLine.find("<=")+1
        && sLine.find('=') != sLine.find(">=")+1
        && !sObject.length()
        && !isInQuotes(sLine, sLine.find('=')))
    {
        sObject = sLine.substr(0,sLine.find('='));
        sLine = sLine.substr(sLine.find('=')+1);
    }

    //cerr << sObject << endl;
    //cerr << "'" << sLine << "'" << endl;

    sTemp = sLine + " ";
    unsigned int nPos = 0;
    while (sTemp.find('#', nPos) != string::npos)
    {
        nPos = sTemp.find('#', nPos);
        //cerr << isInQuotes(sTemp, nPos) << " " << nPos << endl;
        if (!isInQuotes(sTemp, nPos, true))
        {
            string sPrefix = "";
            sTemp_2 += sTemp.substr(0,nPos);
            sTemp = sTemp.substr(nPos+1);
            if (_option.getbDebug())
                mu::console() << _T("|-> DEBUG: sTemp = ") << sTemp << endl;
            if (sTemp[0] == '~')
            {
                for (unsigned int i = 0; i < sTemp.length(); i++)
                {
                    if (sTemp[i] != '~')
                    {
                        sPrefix = sTemp.substr(0,i);
                        sTemp = sTemp.substr(i);
                        break;
                    }
                }
            }
            if (sTemp[0] == '-' || sTemp[0] == '+')
                n_pos = 1;
            else
                n_pos = 0;
            if (sTemp[0] == '(' || sTemp[0] == '{')
            {
                string sExpr = sTemp.substr(1,getMatchingParenthesis(sTemp)-1);
                if (containsStrings(sExpr))
                {
                    sExpr += " -nq";
                    if (!parser_StringParser(sExpr, sDummy, _data, _parser, _option, true))
                        return 0;
                    string sElement = "";
                    string sBlock = "";
                    while (getNextArgument(sExpr, false).length())
                    {
                        sElement = getNextArgument(sExpr, true);
                        while (sElement.length() < sPrefix.length()+1)
                            sElement.insert(0,1,'0');
                        sBlock += "\"" + sElement + "\"";
                        if (getNextArgument(sExpr, false).length())
                        {
                            sBlock += ",";
                            if (sBlock.front() != '{')
                                sBlock.insert(0,1,'{');
                        }
                    }
                    if (sBlock.front() == '{')
                        sBlock += "}";
                    sTemp_2 += sBlock;
                    if (parser_getDelimiterPos(sTemp.substr(n_pos)) < sTemp.length())
                        sTemp = sTemp.substr(parser_getDelimiterPos(sTemp.substr(n_pos)));
                    else
                        sTemp = "";
                    if (_option.getbDebug())
                        mu::console() << _T("|-> DEBUG: sTemp_2 = ") << sTemp_2 << endl;

                    nPos = 0;
                    continue;
                }
                _parser.SetExpr(sExpr);
                //_parser.SetExpr(sTemp.substr(1, getMatchingParenthesis(sTemp)-1));
            }
            else if (sTemp[0] == '"')
            {
                string sExpr = sTemp.substr(1, sTemp.find('"', 1)-1);
                while (sExpr.length() < sPrefix.length()+2)
                    sExpr.insert(0,1,'0');
                sTemp_2 += "\"" + sExpr + "\"";
                if (sTemp.find('"', 1) < sTemp.length()-1)
                    sTemp = sTemp.substr(sTemp.find('"', 1)+1);
                else
                    sTemp = "";
                continue;
            }
            else if (sTemp[0] == '<')
            {
                if (sTemp.find("<>") == 0
                    || sTemp.find("<this>") == 0
                    || sTemp.find("<wp>") == 0
                    || sTemp.find("<loadpath>") == 0
                    || sTemp.find("<savepath>") == 0
                    || sTemp.find("<plotpath>") == 0
                    || sTemp.find("<procpath>") == 0
                    || sTemp.find("<scriptpath>") == 0)
                {
                    if (sTemp.find("<>") == 0 || sTemp.find("<this>") == 0)
                        sTemp_2 += "\"" + replacePathSeparator(_option.getExePath()) + "\"";
                    else if (sTemp.find("<wp>") == 0)
                        sTemp_2 += "\"" + replacePathSeparator(_option.getWorkPath()) + "\"";
                    else if (sTemp.find("<loadpath>") == 0)
                        sTemp_2 += "\"" + replacePathSeparator(_option.getLoadPath()) + "\"";
                    else if (sTemp.find("<savepath>") == 0)
                        sTemp_2 += "\"" + replacePathSeparator(_option.getSavePath()) + "\"";
                    else if (sTemp.find("<plotpath>") == 0)
                        sTemp_2 += "\"" + replacePathSeparator(_option.getPlotOutputPath()) + "\"";
                    else if (sTemp.find("<procpath>") == 0)
                        sTemp_2 += "\"" + replacePathSeparator(_option.getProcsPath()) + "\"";
                    else
                        sTemp_2 += "\"" + replacePathSeparator(_option.getScriptPath()) + "\"";
                    sTemp = sTemp.substr(sTemp.find('>')+1);
                }
                else if (sTemp.find('>') != string::npos)
                {
                    sTemp_2 += "\"" + sTemp.substr(1, sTemp.find('>')-1) + "\"";
                    sTemp = sTemp.substr(sTemp.find('>')+1);
                }
                else
                {
                    throw STRING_ERROR;
                    /*sTemp_2 += sTemp.substr(0, sTemp.find(' ')) + "\"";
                    sTemp = sTemp.substr(sTemp.find(' ')+1);*/
                }
                continue;
            }
            else
                _parser.SetExpr(sTemp.substr(0, parser_getDelimiterPos(sTemp.substr(n_pos))));
            if (_option.getbDebug())
                mu::console() << _T("|-> DEBUG: parser_getDelimiterPos(sTemp) = ") << parser_getDelimiterPos(sTemp.substr(n_pos)) << endl;
            {
                int nResults = 0;
                value_type* v = 0;
                v = _parser.Eval(nResults);
                if (nResults > 1)
                    sTemp_2 += "{";
                string sElement = "";
                for (int n = 0; n < nResults; n++)
                {
                    if (fabs(rint(v[n])-v[n]) < 1e-14 && fabs(v[n]) >= 1.0)
                        sElement = toString((long long int)rint(v[n]));
                    else
                        sElement = toString(v[n], _option);
                    while (sElement.length() < sPrefix.length()+1)
                        sElement.insert(0,1,'0');
                    sTemp_2 += "\"" + sElement + "\"";
                    if (nResults > 1)
                        sTemp_2 += ",";
                }
                if (nResults > 1)
                    sTemp_2.back() = '}';
            }

            //sTemp_2 += "\"";
            if (parser_getDelimiterPos(sTemp.substr(n_pos)) < sTemp.length())
                sTemp = sTemp.substr(parser_getDelimiterPos(sTemp.substr(n_pos)));
            else
                sTemp = "";
            if (_option.getbDebug())
                mu::console() << _T("|-> DEBUG: sTemp_2 = ") << sTemp_2 << endl;

            nPos = 0;
        }
        else
            nPos++;
        //cerr << nPos << endl;
    }

    if (sTemp.length() && sTemp_2.length())
        sTemp_2 += sTemp;

    if (sTemp_2.find('{') != string::npos)
    {
        parser_VectorToExpr(sTemp_2, _option);
    }
    if (sTemp_2.length())
        sTemp = sTemp_2;
    else
        sTemp = sLine;


    if (_option.getbDebug())
        mu::console() << _T("|-> DEBUG: sTemp = ") << sTemp << endl;

    for (unsigned int i = 0; i < sTemp.length(); i++)
    {
        if (sTemp[i] == ',' && !isInQuotes(sTemp, i))
        {
            nStrings++;
        }
    }

    if (nStrings)
        sFinal = new string[nStrings];
    else
        return 0;

    if (_option.getbDebug())
        cerr << "|-> DEBUG: nStrings = " << nStrings << endl;

    for (unsigned int j = 0; j < nStrings; j++)
    {
        sFinal[j] = "";
    }
    if (sTemp_2.length())
        sLine = sTemp_2;

    for (unsigned int i = 0; i < nStrings; i++)
    {
        if (i+1 == nStrings)
        {
            sFinal[i] = sLine;
            break;
        }
        for (unsigned int j = 0; j < sLine.length(); j++)
        {
            if (sLine[j] == ',' && !isInQuotes(sLine, j))
            {
                sFinal[i] = sLine.substr(0,j);
                sLine = sLine.substr(j+1);
                break;
            }
        }
    }

    for (unsigned int n = 0; n < nStrings; n++)
    {
        //cerr << sFinal[n] << endl;
        StripSpaces(sFinal[n]);
        if (!sFinal[n].length())
            continue;
        // Strings verknüpfen
        if (sFinal[n].front() == '"' && sFinal[n].back() == '"')
        {
            for (unsigned int j = 0; j < sFinal[n].length(); j++)
            {
                if (sFinal[n][j] == '+' && !isInQuotes(sFinal[n], j))
                {
                    unsigned int k = j;
                    j = sFinal[n].rfind('"', j);
                    sFinal[n] = sFinal[n].substr(0,sFinal[n].rfind('"', k)) + sFinal[n].substr(sFinal[n].find('"', k)+1);
                }
            }
        }
        if (sFinal[n].find("&&") != string::npos
            || sFinal[n].find("||") != string::npos
            || sFinal[n].find("<=") != string::npos
            || sFinal[n].find(">=") != string::npos
            || sFinal[n].find("==") != string::npos
            || sFinal[n].find("!=") != string::npos
            || (sFinal[n].find('<') != string::npos
                && sFinal[n].find("<>") != sFinal[n].find('<')
                && sFinal[n].find("<this>") != sFinal[n].find('<')
                && sFinal[n].find("<wp>") != sFinal[n].find('<')
                && sFinal[n].find("<loadpath>") != sFinal[n].find('<')
                && sFinal[n].find("<savepath>") != sFinal[n].find('<')
                && sFinal[n].find("<plotpath>") != sFinal[n].find('<')
                && sFinal[n].find("<procpath>") != sFinal[n].find('<')
                && sFinal[n].find("<scriptpath>") != sFinal[n].find('<'))
            || (sFinal[n].find('>') != string::npos
                && sFinal[n].find("<>") != sFinal[n].find('>')-1
                && sFinal[n].find("<this>") != sFinal[n].find('>')-5
                && sFinal[n].find("<wp>") != sFinal[n].find('>')-5
                && sFinal[n].find("<loadpath>") != sFinal[n].find('>')-9
                && sFinal[n].find("<savepath>") != sFinal[n].find('>')-9
                && sFinal[n].find("<plotpath>") != sFinal[n].find('>')-9
                && sFinal[n].find("<procpath>") != sFinal[n].find('>')-9
                && sFinal[n].find("<scriptpath>") != sFinal[n].find('>')-11)
            )
        {
            //bReturningLogicals = true;
            if (_option.getbDebug())
                cerr << "|-> DEBUG: Final[n] = " << sFinal[n] << endl;
            sFinal[n] = parser_evalStringLogic(sFinal[n], bReturningLogicals);
            StripSpaces(sFinal[n]);

        }
        if (sFinal[n].front() != '"' && sFinal[n].back() != '"')
        {
            _parser.SetExpr(sFinal[n]);
            sFinal[n] = toString(_parser.Eval(), _option);
            vIsNoStringValue.push_back(true);
        }
        else
            vIsNoStringValue.push_back(false);
        if ((sFinal[n].front() == '"' && sFinal[n].back() != '"')
            || (sFinal[n].front() != '"' && sFinal[n].back() == '"'))
        {
            if (sFinal[n].front() == '"')
                sFinal[n].insert(0,1,'\\');
            for (unsigned int q = 1; q < sFinal[n].length(); q++)
            {
                if (sFinal[n][q] == '"' && sFinal[n][q-1] != '\\')
                    sFinal[n].insert(q,1,'\\');
            }
        }
        else
        {
            if (sFinal[n].front() == '"')
                sFinal[n] = sFinal[n].substr(1);
            if (sFinal[n].back() == '"')
                sFinal[n].pop_back();
        }
    }

    if (sObject.length())
    {
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sObject = " << sObject << endl;
        if (sObject.find("data(") != string::npos || _data.containsCacheElements(sObject))
        {
            string si = "";
            string sj = "";
            int nIndex[2] = {0,0};
            if (sObject.find("data(") != string::npos)
                si = sObject.substr(sObject.find("data(")+4);
            else
            {
                for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                {
                    if (sObject.find(iter->first+"(") != string::npos
                        && (!sObject.find(iter->first+"(")
                            || (sObject.find(iter->first+"(") && checkDelimiter(sObject.substr(sObject.find(iter->first+"(")-1, (iter->first).length()+2)))))
                    {
                        si = sObject.substr(sObject.find(iter->first+"(")+(iter->first).length());
                        break;
                    }
                }
                _data.setCacheStatus(true);
            }
            try
            {
                parser_SplitArgs(si, sj, ',', _option);
            }
            catch (...)
            {
                delete[] sFinal;
                throw;
            }
            if (si.find("#") != string::npos)
            {
                if (sj.find(":") != string::npos)
                {
                    si = sj.substr(0,sj.find(":"));
                    sj = sj.substr(sj.find(":")+1);
                }
                else
                    si = sj;
                if (parser_ExprNotEmpty(si))
                {
                    _parser.SetExpr(si);
                    nIndex[0] = (int)_parser.Eval();
                    nIndex[0]--;
                }
                if (parser_ExprNotEmpty(sj))
                {
                    _parser.SetExpr(sj);
                    nIndex[1] = (int)_parser.Eval();
                }
                if (sObject.find("data(") != string::npos)
                {
                    if (!nIndex[1])
                        nIndex[1] = _data.getCols("data");
                    parser_CheckIndices(nIndex[0], nIndex[1]);
                    for (int n = 0; n < (int)nStrings; n++)
                    {
                        if (!sFinal[n].length() || n+nIndex[0] == nIndex[1]+1 || n+nIndex[0] >= _data.getCols("data"))
                            break;
                        _data.setHeadLineElement(n+nIndex[0], "data", removeControlSymbols(sFinal[n]));
                    }

                }
                else
                {
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (sObject.find(iter->first+"(") != string::npos
                            && (!sObject.find(iter->first+"(")
                                || (sObject.find(iter->first+"(") && checkDelimiter(sObject.substr(sObject.find(iter->first+"(")-1, (iter->first).length()+2)))))
                        {
                            if (nIndex[1])
                                parser_CheckIndices(nIndex[0], nIndex[1]);

                                //nIndex[1] = _data.getCols(iter->first);
                            for (int n = 0; n < (int)nStrings; n++)
                            {
                                if (!sFinal[n].length() || (nIndex[1] && n+nIndex[0] == nIndex[1]+1))// || n+nIndex[0] >= _data.getCols(iter->first))
                                    break;
                                _data.setHeadLineElement(n+nIndex[0], iter->first, removeControlSymbols(sFinal[n]));
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                delete[] sFinal;
                throw CANNOT_CONTAIN_STRINGS;
            }
            _data.setCacheStatus(false);
        }
        else if (sObject.find("string(") != string::npos)
        {
            string si = "";
            string sj = "";
            string sCol = "";
            int nIndex[3] = {-2,-2,0};
            si = sObject.substr(sObject.find("string(")+6);
            si = si.substr(1,getMatchingParenthesis(si)-1);
            StripSpaces(si);
            //cerr << si << endl;
            if (si.length())
            {
                if (si.find(":") != string::npos)
                {
                    try
                    {
                        si = "(" + si + ")";
                        parser_SplitArgs(si, sj, ':', _option);
                        if (sj.find(',') != string::npos)
                        {
                            parser_SplitArgs(sj, sCol, ',', _option, true);
                        }
                        if (!parser_ExprNotEmpty(si))
                        {
                            si = "1";
                        }
                    }
                    catch (...)
                    {
                        delete[] sFinal;
                        throw;
                    }
                }
                else if (si.find(',') != string::npos)
                {
                    try
                    {
                        parser_SplitArgs(si,sCol, ',', _option, true);
                        sj = si;
                    }
                    catch (...)
                    {
                        delete[] sFinal;
                        throw;
                    }
                }
                else
                    sj = si;

                if (parser_ExprNotEmpty(sCol))
                {
                    _parser.SetExpr(sCol);
                    nIndex[2] = (unsigned int)_parser.Eval()-1;
                }
                if (parser_ExprNotEmpty(si))
                {
                    _parser.SetExpr(si);
                    nIndex[0] = (unsigned int)_parser.Eval()-1;
                }
                if (parser_ExprNotEmpty(sj))
                {
                    _parser.SetExpr(sj);
                    nIndex[1] = (unsigned int)_parser.Eval()-1;
                }
                if (nIndex[0] < 0 && nIndex[1] < 0)
                    nIndex[0] = _data.getStringElements(nIndex[2]);
                else if (nIndex[0] < 0)
                    nIndex[0] = 0;

                if (nIndex[1] >= 0)
                    parser_CheckIndices(nIndex[0], nIndex[1]);

                //cerr << nIndex[0] << " " << nIndex[1] << " " << nIndex[2] << endl;

                for (int n = 0; n < (int)nStrings; n++)
                {
                    if (n+nIndex[0] == nIndex[1]+1)
                        break;
                    _data.writeString(sFinal[n], n+nIndex[0], nIndex[2]);
                }
            }
            else
            {
                for (int n = 0; n < (int)nStrings; n++)
                {
                    /*if (!sFinal[n].length())
                        break;*/
                    _data.writeString(sFinal[n]);
                }
            }
        }
        else if (_data.containsStringVars(sObject))
        {
            StripSpaces(sObject);
            if (sObject.find(' ') != string::npos)
                return 0;
            try
            {
                _data.setStringValue(sObject, sFinal[0]);
            }
            catch (...)
            {
                delete[] sFinal;
                throw;
            }
        }
        else
        {
            StripSpaces(sObject);
            if (sObject.find(' ') != string::npos)
            {
                delete[] sFinal;
                return 0;
            }
            if (parser_GetVarAdress(sObject, _parser))
            {
                if (!vIsNoStringValue[0])
                {
                    delete[] sFinal;
                    return 0;
                }
            }
            if (vIsNoStringValue[0])
            {
                try
                {
                    int nResults = 0;
                    value_type* v = 0;
                    _parser.SetExpr(sObject + " = " + sFinal[0]);
                    v = _parser.Eval(nResults);
                    vAns = v[0];
                }
                catch (...)
                {
                    delete[] sFinal;
                    throw;
                }
            }
            else
            {
                try
                {
                    _data.setStringValue(sObject, sFinal[0]);
                }
                catch (...)
                {
                    delete[] sFinal;
                    throw;
                }
            }
        }
    }

    sLine = "";
    string sConsoleOut = "|-> ";
    /*if (!bSilent)
        mu::console() << _T("|-> ");*/
    for (unsigned int j = 0; j < nStrings; j++)
    {
        if (bKeepMaskedQuotes)
        {
            if (!bNoQuotes && !vIsNoStringValue[j])
                sLine += "\"" + sFinal[j] + "\"";
            else
                sLine += sFinal[j];
            if (j < nStrings - 1)
                sLine += ",";
            continue;
        }
        if (sFinal[j] != "\\n" && sFinal[j] != "\\t" && !bNoQuotes && !bReturningLogicals && !vIsNoStringValue[j])
        {
            sConsoleOut += "\"";
            sLine += "\"";
        }
        for (unsigned int k = 0; k < sFinal[j].length(); k++)
        {
            if (k+1 < sFinal[j].length()
                && sFinal[j][k] == '\\'
                && (sFinal[j][k+1] == 'n' || sFinal[j][k+1] == 't' || sFinal[j][k+1] == '"')
                && !(sFinal[j].substr(k+1,3) == "tau"
                    && ((checkDelimiter(sFinal[j].substr(k,5)) && sFinal[j].length() >= k+5) || (sFinal[j].length() == k+4)))
                && !(sFinal[j].substr(k+1,5) == "theta"
                    && ((checkDelimiter(sFinal[j].substr(k,7)) && sFinal[j].length() >= k+7) || (sFinal[j].length() == k+6)))
                && !(sFinal[j].substr(k+1,2) == "nu"
                    && ((checkDelimiter(sFinal[j].substr(k,4)) && sFinal[j].length() >= k+4) || (sFinal[j].length() == k+3)))
                && !(sFinal[j].substr(k+1,3) == "neq"
                    && ((checkDelimiter(sFinal[j].substr(k,5)) && sFinal[j].length() >= k+5) || (sFinal[j].length() == k+4)))
            )
            {//\not\neq\ni
                if (sFinal[j][k+1] == 'n')
                {
                    sConsoleOut += "$";
                    sLine += "\n";
                }
                else if (sFinal[j][k+1] == 't')
                {
                    sConsoleOut += "\t";
                    sLine += "\t";
                }
                else if (sFinal[j][k+1] == '"')
                {
                    sConsoleOut += "\"";
                    if (!bKeepMaskedQuotes)
                        sLine += "\"";
                    else
                        sLine += "\\\"";
                }
                k += 1;
            }
            else
            {
                sConsoleOut += sFinal[j][k];
                sLine += sFinal[j][k];
            }
        }
        if (sFinal[j] != "\\n" && sFinal[j] != "\\t" && !bNoQuotes && !bReturningLogicals && !vIsNoStringValue[j])
        {
            sConsoleOut += "\"";
            sLine += "\"";
        }
        if (j == nStrings - 1)
            break;
        /*sif (!sFinal[j+1].length())
            continue;*/
        if (sFinal[j] != "\\n" && sFinal[j+1] != "\\n" && sFinal[j] != "\\t" && sFinal[j] != "\\t")
        {
            if (sLine.find_last_not_of("\" ") != string::npos && sLine[sLine.find_last_not_of("\" ")] == '\n')
            {
                sLine += ", ";
                if (sConsoleOut.back() == '"')
                {
                    sConsoleOut[sConsoleOut.rfind('$')] = '"';
                    sConsoleOut.back() = '$';
                }
                continue;
            }
            sConsoleOut += ", ";
            sLine += ", ";
        }
    }
    if (bPeek)
        cerr << "\r                                                       \r";
    if ((!bSilent || bPeek) && !bReturningLogicals)
        cerr << LineBreak(sConsoleOut, _option, false) << endl;


    delete[] sFinal;
    sFinal = 0;
    if (bReturningLogicals)
        return -1;
    else
        return 1;
}

int parser_countStrings(const string& sCmd)
{
    if (!sCmd.length())
        return 0;
    if (sCmd.find(',') == string::npos)
        return 1;
    int nCount = 1;

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == ',' && !isInQuotes(sCmd, i))
            nCount++;
    }
    return nCount;
}

int parser_numStrings(const string& sCmd)
{
    if (!sCmd.length() || sCmd == "\"\"")
        return 0;
    if (sCmd != "\"\"" && sCmd.find(',') == string::npos)
        return 1;
    int nCount = 0;
    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if ((sCmd[i] == ',' && !isInQuotes(sCmd, i)) || i+1 == sCmd.length())
        {
            for (int j = i; j >= 0; j--)
            {
                if (j > 0 && sCmd[j] == '"' && sCmd[j-1] == '"')
                    break;
                else if (j > 0 && sCmd[j] == '"' && sCmd[j-1] != '"')
                {
                    nCount++;
                    break;
                }
            }
        }
    }
    return nCount;
}

string parser_getMinString(const string& sCmd)
{
    if (!sCmd.length())
        return "";
    if (sCmd.find(',') == string::npos)
        return sCmd;
    string* sStrings;
    unsigned int nCount = 1;

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == ',' && !isInQuotes(sCmd, i))
            nCount++;
    }
    sStrings = new string[nCount];

    string sLine = sCmd;
    string sMin = "";
    for (unsigned int i = 0; i < nCount; i++)
    {
        if (i+1 == nCount)
        {
            sStrings[i] = sLine;
            StripSpaces(sStrings[i]);
            sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
            break;
        }
        for (unsigned int j = 0; j < sLine.length(); j++)
        {
            if (sLine[j] == ',' && !isInQuotes(sLine, j))
            {
                sStrings[i] = sLine.substr(0,j);
                StripSpaces(sStrings[i]);
                sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
                sLine = sLine.substr(j+1);
                break;
            }
        }
    }

    sMin = sStrings[0];
    for (unsigned int i = 1; i < nCount; i++)
    {
        if (sMin > sStrings[i])
            sMin = sStrings[i];
    }
    sMin = "\"" + sMin + "\"";
    delete[] sStrings;
    return sMin;
}

string parser_getMaxString(const string& sCmd)
{
    if (!sCmd.length())
        return "";
    if (sCmd.find(',') == string::npos)
        return sCmd;
    string* sStrings;
    unsigned int nCount = 1;

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == ',' && !isInQuotes(sCmd, i))
            nCount++;
    }
    sStrings = new string[nCount];

    string sLine = sCmd;
    string sMax = "";
    for (unsigned int i = 0; i < nCount; i++)
    {
        if (i+1 == nCount)
        {
            sStrings[i] = sLine;
            StripSpaces(sStrings[i]);
            sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
            break;
        }
        for (unsigned int j = 0; j < sLine.length(); j++)
        {
            if (sLine[j] == ',' && !isInQuotes(sLine, j))
            {
                sStrings[i] = sLine.substr(0,j);
                StripSpaces(sStrings[i]);
                sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
                sLine = sLine.substr(j+1);
                break;
            }
        }
    }

    sMax = sStrings[0];
    for (unsigned int i = 1; i < nCount; i++)
    {
        if (sMax < sStrings[i])
            sMax = sStrings[i];
    }
    sMax = "\"" + sMax + "\"";
    delete[] sStrings;
    return sMax;
}

string parser_getSumString(const string& sCmd)
{
    if (!sCmd.length())
        return "";
    if (sCmd.find(',') == string::npos)
        return sCmd;
    string* sStrings;
    unsigned int nCount = 1;

    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == ',' && !isInQuotes(sCmd, i))
            nCount++;
    }
    sStrings = new string[nCount];

    string sLine = sCmd;
    string sSum = "";
    for (unsigned int i = 0; i < nCount; i++)
    {
        if (i+1 == nCount)
        {
            sStrings[i] = sLine;
            StripSpaces(sStrings[i]);
            sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
            break;
        }
        for (unsigned int j = 0; j < sLine.length(); j++)
        {
            if (sLine[j] == ',' && !isInQuotes(sLine, j))
            {
                sStrings[i] = sLine.substr(0,j);
                StripSpaces(sStrings[i]);
                sStrings[i] = sStrings[i].substr(1, sStrings[i].length()-2);
                sLine = sLine.substr(j+1);
                break;
            }
        }
    }

    for (unsigned int i = 0; i < nCount; i++)
    {
        sSum += sStrings[i];
    }
    sSum = "\"" + sSum + "\"";
    delete[] sStrings;
    return sSum;
}


