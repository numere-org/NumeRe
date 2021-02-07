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


#include "datafile.hpp"
#include "../../kernel.hpp"
#include "../ui/error.hpp"

#include <boost/tokenizer.hpp>

bool fileExists(const string&);
string wcstombs(const wstring&);
using namespace std;
using namespace boost;

size_t qSortDouble(double* dArray, size_t nlength);
string toString(int);
string toLowerCase(const string&);


int findParameter(const string& sCmd, const string& sParam, const char cFollowing);
string getArgAtPos(const string& sCmd, unsigned int nPos, int extraction);
void StripSpaces(string& sToStrip);
string getClipboardText();
string utf8parser(const string& sString);
int StrToInt(const string&);

/*
 * Realisierung der Datafile-Klasse
 */

// --> Standard-Konstruktor <--
PasteHandler::PasteHandler()
{
}


vector<string> PasteHandler::getPastedDataFromCmdLine(const Settings& _option, bool& bKeepEmptyTokens)
{
    vector<string> vPaste;
    string sLine;
    make_hline();
    NumeReKernel::print(LineBreak("NUMERE: "+toUpperCase(_lang.get("DATA_PASTE_HEADLINE")), _option));
    make_hline();
    NumeReKernel::printPreFmt(LineBreak("|-> "+_lang.get("DATA_PASTE_DESCRIPTION"), _option) + "\n|\n");
    while (true)
    {
        NumeReKernel::printPreFmt("|PASTE> ");
        NumeReKernel::getline(sLine);
        if (sLine == "endpaste")
            break;
        if (!isNumeric(sLine) && sLine.find(' ') != string::npos && sLine.find('\t') != string::npos)
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ' ')
                    sLine[i] = '_';
            }
            if (!bKeepEmptyTokens)
                bKeepEmptyTokens = true;
        }
        replaceTabSign(sLine);
        if (sLine.find_first_not_of(' ') == string::npos)
            continue;
        if (isNumeric(sLine) && sLine.find(',') != string::npos && sLine.find('.') == string::npos)
            replaceDecimalSign(sLine);
        else if (sLine.find(',') != string::npos && sLine.find(';') != string::npos)
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                    sLine[i] = '.';
                if (sLine[i] == ';')
                {
                    if (!bKeepEmptyTokens)
                        bKeepEmptyTokens = true;
                    sLine[i] = ' ';
                }
            }
        }
        else
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == ',')
                {
                    if (!bKeepEmptyTokens)
                        bKeepEmptyTokens = true;
                    sLine[i] = ' ';
                }
            }
        }
        vPaste.push_back(sLine);
    }
    make_hline();
    return vPaste;
}

// --> Lese den Inhalt eines Tabellenpastes <--
Memory* PasteHandler::pasteLoad(const Settings& _option)
{
    vector<string> vPaste;
    string sLine = "";
    string sClipboard = getClipboardText();
    boost::char_separator<char> sep(" ");
    long long int nSkip = 0;
    long long int nColsTemp = 0;
    bool bKeepEmptyTokens = false;
    bool bReadClipboard = (bool)(sClipboard.length());
    long long int nLines = 0;
    long long int nCols = 0;

    //cerr << sClipboard << endl;

    if (!sClipboard.length())
    {
        vPaste = getPastedDataFromCmdLine(_option, bKeepEmptyTokens);
    }
    else
    {
        while (true)
        {
            if (!sClipboard.length() || sClipboard == "\n")
                break;
            sLine = sClipboard.substr(0, sClipboard.find('\n'));
            if (sLine.back() == (char)13) // CR entfernen
                sLine.pop_back();
            //cerr << sLine << " " << (int)sLine.back() << endl;
            if (sClipboard.find('\n') != string::npos)
                sClipboard.erase(0, sClipboard.find('\n')+1);
            else
                sClipboard.clear();
            //StripSpaces(sLine);
            if (!isNumeric(sLine) && sLine.find(' ') != string::npos && sLine.find('\t') != string::npos)
            {
                for (unsigned int i = 0; i < sLine.length(); i++)
                {
                    if (sLine[i] == ' ')
                        sLine[i] = '_';
                }
                if (!bKeepEmptyTokens)
                    bKeepEmptyTokens = true;
            }
            replaceTabSign(sLine);
            if (sLine.find_first_not_of(' ') == string::npos)
                continue;
            if (isNumeric(sLine) && sLine.find(',') != string::npos && sLine.find('.') == string::npos)
                replaceDecimalSign(sLine);
            else if (sLine.find(',') != string::npos && sLine.find(';') != string::npos)
            {
                for (unsigned int i = 0; i < sLine.length(); i++)
                {
                    if (sLine[i] == ',')
                        sLine[i] = '.';
                    if (sLine[i] == ';')
                    {
                        if (!bKeepEmptyTokens)
                            bKeepEmptyTokens = true;
                        sLine[i] = ' ';
                    }
                }
            }
            else
            {
                for (unsigned int i = 0; i < sLine.length(); i++)
                {
                    if (sLine[i] == ',')
                    {
                        if (!bKeepEmptyTokens)
                            bKeepEmptyTokens = true;
                        sLine[i] = ' ';
                    }
                }
            }
            vPaste.push_back(sLine);
        }
        if (!vPaste.size())
        {
            bReadClipboard = false;
            bKeepEmptyTokens = false;
            vPaste = getPastedDataFromCmdLine(_option, bKeepEmptyTokens);
        }
    }
    //cerr << vPaste.size() << endl;
    if (!vPaste.size())
        return nullptr;

    nLines = vPaste.size();
    //cerr << nLines << endl;
    for (unsigned int i = 0; i < vPaste.size(); i++)
    {
        if (!isNumeric(vPaste[i]))
        {
            nLines--;
            nSkip++;
            if (nLines > i+1 && vPaste[i+1].find(' ') == string::npos && vPaste[i].find(' ') != string::npos)
            {
                for (unsigned int j = 0; j < vPaste[i].size(); j++)
                {
                    if (vPaste[i][j] == ' ')
                        vPaste[i][j] = '_';
                }
            }
        }
        else
            break;
    }
    //cerr << nLines << endl;
    if (!nLines && !bReadClipboard)
    {
        NumeReKernel::print(LineBreak(_lang.get("DATA_COULD_NOT_IDENTIFY_PASTED_CONTENT"), _option));
        return nullptr;
    }
    else if (bReadClipboard && !nLines)
    {
        bKeepEmptyTokens = false;
        vPaste = getPastedDataFromCmdLine(_option, bKeepEmptyTokens);
        if (!vPaste.size())
            return nullptr;
        nLines = vPaste.size();
        nSkip = 0;
        //cerr << nLines << endl;
        for (unsigned int i = 0; i < vPaste.size(); i++)
        {
            if (!isNumeric(vPaste[i]))
            {
                nLines--;
                nSkip++;
                if (nLines > i+1 && vPaste[i+1].find(' ') == string::npos && vPaste[i].find(' ') != string::npos)
                {
                    for (unsigned int j = 0; j < vPaste[i].size(); j++)
                    {
                        if (vPaste[i][j] == ' ')
                            vPaste[i][j] = '_';
                    }
                }
            }
            else
                break;
        }
        if (!nLines)
        {
            NumeReKernel::print(LineBreak(_lang.get("DATA_COULD_NOT_IDENTIFY_PASTED_CONTENT"), _option));
            return nullptr;
        }
    }

    if (bKeepEmptyTokens)
    {
        sep = char_separator<char>(" ","", boost::keep_empty_tokens);
    }
    for (unsigned int i = 0; i < vPaste.size(); i++)
    {
        nColsTemp = 0;
        stripTrailingSpaces(vPaste[i]);
        //cerr << vPaste[i] << endl;
        tokenizer<char_separator<char> > tok(vPaste[i],sep);
        for (auto iter = tok.begin(); iter != tok.end(); ++iter)
            nColsTemp++;
        if (nColsTemp > nCols)
            nCols = nColsTemp;
    }

    Memory* _mem = new Memory(nLines, nCols);

    for (unsigned int i = 0; i < vPaste.size(); i++)
    {
        tokenizer<char_separator<char> > tok(vPaste[i], sep);
        long long int j = 0;
        for (auto iter = tok.begin(); iter != tok.end(); ++iter)
        {
            sLine = *iter;
            if (sLine[sLine.length()-1] == '%')
                sLine.erase(sLine.length()-1);
            if (isNumeric(sLine) && sLine != "NAN" && sLine != "NaN" && sLine != "nan")
            {
                if (i < nSkip && nSkip)
                {
                    if (sLine.length())
                    {
                        if (!i)
                            _mem->setHeadLineElement(j, sLine);
                        else
                            _mem->setHeadLineElement(j, _mem->getHeadLineElement(j) + "\\n" + sLine);
                    }
                }
                else
                {
                    _mem->writeData(i-nSkip, j, StrToDb(sLine));
                }
            }
            else if (i < nSkip && nSkip)
            {
                if (sLine.length())
                {
                    if (!i || _mem->getHeadLineElement(j) == _lang.get("COMMON_COL") + "_" + toString(j+1))
                        _mem->setHeadLineElement(j, sLine);
                    else
                        _mem->setHeadLineElement(j, _mem->getHeadLineElement(j) + "\\n" + sLine);
                }
            }
            else
            {
                _mem->writeData(i-nSkip, j, NAN);
            }
            j++;
            if (j == nCols)
                break;
        }
    }

    return _mem;
}



bool PasteHandler::isNumeric(const string& _sString)
{
    if (!_sString.length())
        return false;
    for (unsigned int i = 0; i < _sString.length(); i++)
    {
        if ((_sString[i] >= '0' && _sString[i] <= '9')
            || _sString[i] == 'e'
            || _sString[i] == 'E'
            || _sString[i] == '-'
            || _sString[i] == '+'
            || _sString[i] == '.'
            || _sString[i] == ','
            || _sString[i] == '\t'
            || _sString[i] == '%'
            || _sString[i] == ' ')
            continue;
        else if (_sString.substr(i, 3) == "nan"
            || _sString.substr(i, 3) == "NaN"
            || _sString.substr(i, 3) == "NAN"
            || _sString.substr(i, 3) == "inf"
            || _sString.substr(i, 3) == "INF"
            )
        {
            i += 2;
            continue;
        }
        else
        {
            //cerr << _sString[i] << endl;
            return false;
        }
    }
    return true;
}







