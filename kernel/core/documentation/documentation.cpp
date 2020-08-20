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


#include "documentation.hpp"
#include "../../kernel.hpp"

static bool isValue(const string& sExpr, size_t nPos, size_t nLength)
{
    return (!nPos || !isalpha(sExpr[nPos-1])) && (nPos+nLength == sExpr.length() || !isalpha(sExpr[nPos+nLength]));
}

static bool isOperator(const string& sExpr, size_t nPos, size_t nLength)
{
    return true;
}

static bool isFunction(const string& sExpr, size_t nPos, size_t nLength)
{
    return (!nPos || !isalpha(sExpr[nPos-1])) && sExpr[nPos+nLength] == '(';
}

void doc_Help(const string& __sTopic, Settings& _option)
{
    string sTopic = toLowerCase(__sTopic);
    if (findParameter(sTopic, "html"))
        eraseToken(sTopic, "html", false);
    vector<string> vDocArticle;
    // --> Zunaechst einmal muessen wir anfuehrende oder abschliessende Leerzeichen entfernen <--
    StripSpaces(sTopic);
    if (!sTopic.length())
    {
        sTopic = "brief";
    }
    if (sTopic.front() == '-')
        sTopic.erase(0,1);
    StripSpaces(sTopic);

    vDocArticle = _option.getHelpArticle(sTopic);

    if (vDocArticle[0] == "NO_ENTRY_FOUND") // Nix gefunden
    {
        make_hline();
        NumeReKernel::print(LineBreak(_lang.get("DOC_HELP_NO_ENTRY_FOUND", sTopic), _option));
        make_hline();
        return;
    }
    else if (findParameter(__sTopic, "html") || _option.getUseExternalViewer()) // HTML-Export generieren
    {
        ofstream fHTML;
        string sHTML;
        bool generateFile = (bool)findParameter(__sTopic, "html");
        FileSystem _fSys;
        _fSys.setTokens(_option.getTokenPaths());
        _fSys.setPath("docs/htmlexport", true, _option.getExePath());
        string sFilename = "<>/docs/htmlexport/"+_option.getHelpArtclID(sTopic) + ".html";
        _option.declareFileType(".html");
        sFilename = _option.ValidFileName(sFilename,".html");

        sHTML = doc_HelpAsHTML(sTopic, generateFile, _option);
        if (generateFile)
        {
            fHTML.open(sFilename.c_str());
            if (fHTML.fail())
            {
                //sErrorToken = sFilename;
                throw SyntaxError(SyntaxError::CANNOT_GENERATE_FILE, "", SyntaxError::invalid_position, sFilename);
            }
            // content schreiben
            fHTML << sHTML;
            NumeReKernel::print(LineBreak(_lang.get("DOC_HELP_HTMLEXPORT", _option.getHelpArticleTitle(_option.getHelpIdxKey(sTopic)), sFilename), _option));
        }
        else
            NumeReKernel::setDocumentation(sHTML);
        return;
    }
    else // Hilfeartikel anzeigen
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        for (unsigned int i = 0; i < vDocArticle.size(); i++)
        {
            if (!i)
            {
                NumeReKernel::print(toSystemCodePage(toUpperCase(_lang.get("DOC_HELP_HEADLINE", vDocArticle[i]))));
                make_hline();
                continue;
            }

            if (vDocArticle[i].find("<example ") != string::npos) // Beispiel-Tags
            {
                bool bVerb = false;
                if (vDocArticle[i].find("type=") && getArgAtPos(vDocArticle[i], vDocArticle[i].find("type=")+5) == "verbatim")
                    bVerb = true;

                doc_ReplaceTokens(vDocArticle[i], _option);
                if (getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5).find('~') == string::npos)
                    NumeReKernel::print(LineBreak(_lang.get("DOC_HELP_EXAMPLE", getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5)), _option));
                else
                    NumeReKernel::print(LineBreak(_lang.get("DOC_HELP_EXAMPLE", getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5)), _option, false));
                NumeReKernel::printPreFmt("|\n");
                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != string::npos)
                    {
                        i = j;
                        if (i+1 < vDocArticle.size())
                            NumeReKernel::printPreFmt("|\n");
                        break;
                    }
                    if (vDocArticle[j] == "[...]")
                    {
                        NumeReKernel::printPreFmt("|[...]\n");
                        continue;
                    }

                    doc_ReplaceTokens(vDocArticle[j], _option);

                    if (!bVerb)
                    {
                        if (((i+1) % 2 && j % 2) || (!((i+1) % 2) && !(j % 2)))
                        {
                           NumeReKernel::printPreFmt("||<- " + LineBreak(vDocArticle[j], _option, false, 5) + "\n");
                        }
                        else
                        {
                            NumeReKernel::printPreFmt("||-> " + LineBreak(vDocArticle[j], _option, false, 5) + "\n");
                            if (vDocArticle[j+1].find("</example>") == string::npos)
                                NumeReKernel::printPreFmt("||\n");
                                //cerr << "||" << endl;
                        }
                    }
                    else
                    {
                        NumeReKernel::printPreFmt("|" + toSystemCodePage(vDocArticle[j])+"\n");
                    }
                }
            }
            else if (vDocArticle[i].find("<exprblock>") != string::npos) // EXPRBLOCK-Tags
            {
                if (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != string::npos)
                {
                    doc_ReplaceTokens(vDocArticle[i], _option);
                    while (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != string::npos)
                    {
                        string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<exprblock>")+11, vDocArticle[i].find("</exprblock>")-vDocArticle[i].find("<exprblock>")-11);
                        for (unsigned int k = 0; k < sExprBlock.length(); k++)
                        {
                            if (!k && sExprBlock[k] == '$')
                                sExprBlock.insert(0,"\\");
                            if (sExprBlock[k] == '$' && sExprBlock[k-1] != '\\')
                                sExprBlock.insert(k,"\\");
                            if (sExprBlock.substr(k,2) == "\\n")
                                sExprBlock.replace(k,2,"$  ");
                            if (sExprBlock.substr(k,2) == "\\t")
                                sExprBlock.replace(k,2,"    ");
                        }
                        vDocArticle[i].replace(vDocArticle[i].find("<exprblock>"), vDocArticle[i].find("</exprblock>")+12-vDocArticle[i].find("<exprblock>"), "$$  " + sExprBlock + "$$");
                    }
                    if (vDocArticle[i].substr(vDocArticle[i].length()-2) == "$$")
                        vDocArticle[i].pop_back();
                    NumeReKernel::print(LineBreak(vDocArticle[i], _option, false));
                }
                else
                {
                    if (vDocArticle[i] != "<exprblock>")
                        NumeReKernel::print(LineBreak(vDocArticle[i].substr(0,vDocArticle[i].find("<exprblock>")), _option));
                    NumeReKernel::printPreFmt("|\n");
                    for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                    {
                        if (vDocArticle[j].find("</exprblock>") != string::npos)
                        {
                            i = j;
                            if (i+1 < vDocArticle.size())
                                NumeReKernel::printPreFmt("|\n");
                            break;
                        }

                        doc_ReplaceTokens(vDocArticle[j], _option);
                        while (vDocArticle[j].find("\\t") != string::npos)
                            vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "    ");

                        NumeReKernel::printPreFmt("|     " + toSystemCodePage(vDocArticle[j]) + "\n");
                    }
                }
            }
            else if (vDocArticle[i].find("<codeblock>") != string::npos) // CODEBLOCK-Tags
            {
                if (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != string::npos)
                {
                    doc_ReplaceTokens(vDocArticle[i], _option);
                    while (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != string::npos)
                    {
                        string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<codeblock>")+11, vDocArticle[i].find("</codeblock>")-vDocArticle[i].find("<codeblock>")-11);
                        for (unsigned int k = 0; k < sExprBlock.length(); k++)
                        {
                            if (!k && sExprBlock[k] == '$')
                                sExprBlock.insert(0,"\\");
                            if (sExprBlock[k] == '$' && sExprBlock[k-1] != '\\')
                                sExprBlock.insert(k,"\\");
                            if (sExprBlock.substr(k,2) == "\\n")
                                sExprBlock.replace(k,2,"$  ");
                            if (sExprBlock.substr(k,2) == "\\t")
                                sExprBlock.replace(k,2,"    ");
                        }
                        vDocArticle[i].replace(vDocArticle[i].find("<codeblock>"), vDocArticle[i].find("</codeblock>")+12-vDocArticle[i].find("<codeblock>"), "$$  " + sExprBlock + "$$");
                    }
                    if (vDocArticle[i].substr(vDocArticle[i].length()-2) == "$$")
                        vDocArticle[i].pop_back();
                    NumeReKernel::print(LineBreak(vDocArticle[i], _option, false));
                }
                else
                {
                    if (vDocArticle[i] != "<codeblock>")
                        NumeReKernel::print(LineBreak(vDocArticle[i].substr(0,vDocArticle[i].find("<codeblock>")), _option));
                    NumeReKernel::printPreFmt("|\n");
                    for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                    {
                        if (vDocArticle[j].find("</codeblock>") != string::npos)
                        {
                            i = j;
                            if (i+1 < vDocArticle.size())
                                NumeReKernel::printPreFmt("|\n");
                            break;
                        }

                        doc_ReplaceTokens(vDocArticle[j], _option);
                        while (vDocArticle[j].find("\\t") != string::npos)
                            vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "    ");

                        NumeReKernel::printPreFmt("|     " + toSystemCodePage(vDocArticle[j])+"\n");
                    }
                }
            }
            else if (vDocArticle[i].find("<list>") != string::npos) // Standard-LIST-Tags
            {
                unsigned int nLengthMax = 0;
                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</list>") != string::npos)
                    {
                        string sLine = "";
                        string sNode = "";
                        string sRemainingLine = "";
                        string sFinalLine = "";
                        int nIndent = 0;
                        for (unsigned int k = i+1; k < j; k++)
                        {
                            nIndent = 0;
                            sNode = getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5);
                            sRemainingLine = vDocArticle[k].substr(vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2)+1, vDocArticle[k].find("</item>")-1-vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2));
                            sFinalLine = "";

                            if (vDocArticle[k].find("type=") != string::npos && getArgAtPos(vDocArticle[k], vDocArticle[k].find("type=")+5) == "verbatim")
                            {
                                NumeReKernel::printPreFmt("|     " + sNode);
                                nIndent = sNode.length()+6;
                                sLine.append(nLengthMax+9-nIndent, ' ');
                                sLine += "- " + sRemainingLine;
                                if (sLine.find('~') == string::npos && sLine.find('%') == string::npos)
                                    NumeReKernel::printPreFmt(LineBreak(sLine, _option, true, nIndent, nLengthMax+11) + "\n");
                                else
                                    NumeReKernel::printPreFmt(LineBreak(sLine, _option, false, nIndent, nLengthMax+11) + "\n");
                            }
                            else
                            {
                                for (unsigned int n = 0; n < sNode.length(); n++)
                                {
                                    if (n && sNode[n] == '$' && sNode[n-1] != '\\')
                                    {
                                        sLine = "|     " + sNode.substr(0,n);
                                        sNode.erase(0,n+1);
                                        n = -1;
                                        sLine.append(nLengthMax+9-sLine.length()+countEscapeSymbols(sLine), ' ');
                                        if (!sFinalLine.length())
                                        {
                                            sLine += "- " + sRemainingLine;
                                        }
                                        else
                                            sLine += "  " + sRemainingLine;
                                        if (sLine.find('~') == string::npos && sLine.find('%') == string::npos)
                                            sLine = LineBreak(sLine, _option, true, nIndent, nLengthMax+11);
                                        else
                                            sLine = LineBreak(sLine, _option, false, nIndent, nLengthMax+11);
                                        sFinalLine += sLine.substr(0,sLine.find('\n'));
                                        if (sLine.find('\n') != string::npos)
                                            sFinalLine += '\n';
                                        sRemainingLine.erase(0,sLine.substr(nLengthMax+11, sLine.find('\n')-nLengthMax-11).length());
                                        if (sRemainingLine.front() == ' ')
                                            sRemainingLine.erase(0,1);
                                    }
                                }
                                sLine = "|     " + sNode;
                                sLine.append(nLengthMax+9-sLine.length()+countEscapeSymbols(sLine), ' ');
                                if (!sFinalLine.length())
                                {
                                    sLine += "- " + sRemainingLine;
                                }
                                else
                                    sLine += "  " + sRemainingLine;
                                if (sLine.find('~') == string::npos && sLine.find('%') == string::npos)
                                    sFinalLine += LineBreak(sLine, _option, true, nIndent, nLengthMax+11);
                                else
                                    sFinalLine += LineBreak(sLine, _option, false, nIndent, nLengthMax+11);
                                NumeReKernel::printPreFmt(sFinalLine + "\n");
                                /*sLine = "|     " + getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5);
                                sLine.append(nLengthMax+9-sLine.length()+countEscapeSymbols(sLine), ' ');*/
                            }
                        }
                        i = j;
                        break;
                    }
                    else
                    {
                        doc_ReplaceTokens(vDocArticle[j], _option);

                        string sNode = getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5);
                        for (unsigned int k = 0; k < sNode.length(); k++)
                        {
                            if (sNode[k] == '$' && k && sNode[k-1] != '\\')
                            {
                                if (nLengthMax < k-countEscapeSymbols(sNode.substr(0,k)))
                                    nLengthMax = k-countEscapeSymbols(sNode.substr(0,k));
                                sNode.erase(0,k+1);
                                k = -1;
                            }
                        }
                        if (nLengthMax < sNode.length()-countEscapeSymbols(sNode))
                            nLengthMax = sNode.length()-countEscapeSymbols(sNode);
                    }
                }
            }
            else if (vDocArticle[i].find("<list") != string::npos && vDocArticle[i].find("type=") != string::npos) // Modified-LIST-Tags
            {
                map<string,string> mListitems;
                string sType = getArgAtPos(vDocArticle[i], vDocArticle[i].find("type=")+5);
                unsigned int nLengthMax = 0;
                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</list>") != string::npos)
                    {
                        string sLine = "";
                        int nIndent = 0;
                        if (sType == "desc" || sType == "udesc")
                        {
                            for (unsigned int k = i+1; k < j; k++)
                            {
                                nIndent = 0;
                                if (vDocArticle[k].find("type=") != string::npos && getArgAtPos(vDocArticle[k], vDocArticle[k].find("type=")+5) == "verbatim")
                                {
                                    if (sType == "udesc")
                                        NumeReKernel::printPreFmt("|     " + toUpperCase(getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5)));
                                    else
                                        NumeReKernel::printPreFmt("|     " + getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5));
                                }
                                else
                                {
                                    if (sType == "udesc")
                                        sLine = "|     " + toUpperCase(getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5));
                                    else
                                        sLine = "|     " + getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5);
                                }
                                sLine += ":  " + vDocArticle[k].substr(vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2)+1, vDocArticle[k].find("</item>")-1-vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2));
                                if (sLine.find('~') == string::npos && sLine.find('%') == string::npos)
                                    NumeReKernel::printPreFmt(LineBreak(sLine, _option, true, nIndent, 10)+"\n");
                                else
                                    NumeReKernel::printPreFmt(LineBreak(sLine, _option, false, nIndent, 10)+"\n");
                            }
                            i = j;
                            break;
                        }
                        else if (sType == "folded")
                        {
                            for (unsigned int k = i+1; k < j; k++)
                            {
                                if (vDocArticle[k].find("type=") != string::npos && getArgAtPos(vDocArticle[k], vDocArticle[k].find("type=")+5) == "verbatim")
                                {
                                    NumeReKernel::printPreFmt("|     " + getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5));
                                    nIndent = getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+6;
                                    sLine.append(nLengthMax+9-nIndent, ' ');
                                }
                                else
                                {
                                    sLine = "|     " + getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5);
                                    sLine.append(nLengthMax+9-sLine.length()+countEscapeSymbols(sLine), ' ');
                                }
                                sLine += "- [...]";
                                NumeReKernel::printPreFmt(LineBreak(sLine, _option, false, nIndent, nLengthMax+11)+"\n");
                                mListitems[getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5)] = vDocArticle[k].substr(vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2)+1, vDocArticle[k].find("</item>")-1-vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2));
                            }
                            NumeReKernel::print(LineBreak("GEFALTETE LISTE: Stichpunkt eingeben, um Beschreibung anzuzeigen. \"0\" eingeben, um die Liste zu verlassen:", _option));
                            while (true)
                            {
                                NumeReKernel::printPreFmt("|LIST> ");;
                                NumeReKernel::getline(sLine);
                                StripSpaces(sLine);
                                if (sLine == "0")
                                {
                                    mListitems.clear();
                                    break;
                                }
                                if (mListitems.find(sLine) == mListitems.end())
                                {
                                    sLine.clear();
                                    continue;
                                }
                                auto iter = mListitems.find(sLine);
                                nIndent = (iter->first).length()+6;
                                sLine = iter->first;
                                sLine.append(nLengthMax+9-nIndent, ' ');
                                sLine += "- " + iter->second;
                                NumeReKernel::printPreFmt(LineBreak("|     " + sLine, _option, false, 0, nLengthMax+11)+"\n");
                            }
                            i = j;
                            break;
                        }
                    }
                    else
                    {
                        doc_ReplaceTokens(vDocArticle[j], _option);

                        if (nLengthMax < getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()-countEscapeSymbols(getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5)))
                            nLengthMax = getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()-countEscapeSymbols(getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5));
                    }
                }
            }
            else if (vDocArticle[i].find("<table") != string::npos) // TABLE-Tags
            {
                string sTable = vDocArticle[i].substr(vDocArticle[i].find("<table"));
                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</table>") != string::npos)
                    {
                        sTable += vDocArticle[j].substr(0,vDocArticle[j].find("</table>")+8);
                        // Send the whole content to the table reader and render the obtained table on the screen.
                        vector<vector<string> > vTable = doc_readTokenTable(sTable, _option);
                        vector<size_t> vFieldSizes;
                        for (size_t v = 0; v < vTable.size(); v++)
                        {
                            for (size_t w = 0; w < vTable[v].size(); w++)
                            {
                                if (vFieldSizes.size() < w+1)
                                    vFieldSizes.push_back(vTable[v][w].length());
                                else
                                {
                                    if (vFieldSizes[w] < vTable[v][w].length())
                                        vFieldSizes[w] = vTable[v][w].length();
                                }
                            }
                        }

                        for (size_t v = 0; v < vTable.size(); v++)
                        {
                            NumeReKernel::printPreFmt("|     ");
                            for (size_t w = 0; w < vTable[v].size(); w++)
                            {
                                NumeReKernel::printPreFmt(strlfill(vTable[v][w], vFieldSizes[w]+2));
                            }
                            NumeReKernel::printPreFmt("\n");
                        }

                        i = j;
                        break;
                    }
                    else
                    {
                        sTable += vDocArticle[j];
                        /*doc_ReplaceTokens(vDocArticle[j], _option);

                        string sNode = getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5);
                        for (unsigned int k = 0; k < sNode.length(); k++)
                        {
                            if (sNode[k] == '$' && k && sNode[k-1] != '\\')
                            {
                                if (nLengthMax < k-countEscapeSymbols(sNode.substr(0,k)))
                                    nLengthMax = k-countEscapeSymbols(sNode.substr(0,k));
                                sNode.erase(0,k+1);
                                k = -1;
                            }
                        }
                        if (nLengthMax < sNode.length()-countEscapeSymbols(sNode))
                            nLengthMax = sNode.length()-countEscapeSymbols(sNode);*/
                    }
                }
            }
            else // Normaler Paragraph
            {
                doc_ReplaceTokens(vDocArticle[i], _option);

                if (vDocArticle[i].find('~') == string::npos && vDocArticle[i].find('%') == string::npos)
                    NumeReKernel::print(LineBreak(vDocArticle[i], _option));
                else
                    NumeReKernel::print(LineBreak(vDocArticle[i], _option, false));
            }
        }
        NumeReKernel::toggleTableStatus();
        make_hline();
        return;
    }

    return;
}


// This function returns the documentation article
// for the selected topic as a HTML string. This
// string either may be used to create a corresponding
// file or it may be displayed in the documentation viewer
string doc_HelpAsHTML(const string& __sTopic, bool generateFile, Settings& _option)
{
    string sTopic = __sTopic;
    StripSpaces(sTopic);

    // Get the article contents
    vector<string> vDocArticle = _option.getHelpArticle(sTopic);

    if (vDocArticle[0] == "NO_ENTRY_FOUND") // Nix gefunden
        return "";

    bool isIndex = (vDocArticle[0] == "Index");

    string sHTML;

    sHTML = "<!DOCTYPE html>\n<html>\n<head>\n";

    // Convert the XML-like structure of the documentation
    // article into a valid HTML DOM, which can be returned
    // as a single string
    for (unsigned int i = 0; i < vDocArticle.size(); i++)
    {
        // If this is the first line, then create the header
        // tag section of the HTML file
        if (!i)
        {
            if (generateFile)
            {
                // Header fertigstellen
                sHTML += "<title>" + toUpperCase(_lang.get("DOC_HELP_HEADLINE", vDocArticle[i]))
                      + "</title>\n"
                      + "</head>\n\n"
                      + "<body>\n"
                      + "<!-- START COPYING HERE -->\n";
                sHTML += "<h4>" + _lang.get("DOC_HELP_DESC_HEADLINE") + "</h4>\n";
            }
            else
            {
                // Header fertigstellen
                sHTML += "<title>" + vDocArticle[i] + "</title>\n</head>\n\n<body>\n<h2>"+vDocArticle[i]+"</h2>\n";
            }

            continue;
        }

        // Expand the XML tags in the documentation article
        // into corresponding HTML tags, which will resemble
        // the intended style
        if (vDocArticle[i].find("<example ") != string::npos) // Beispiel-Tags
        {
            sHTML += "<h4>"+ _lang.get("DOC_HELP_EXAMPLE_HEADLINE") +"</h4>\n";
            bool bVerb = false;

            if (vDocArticle[i].find("type=") && getArgAtPos(vDocArticle[i], vDocArticle[i].find("type=")+5) == "verbatim")
                bVerb = true;

            string sDescription = getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5);

            doc_ReplaceTokensForHTML(sDescription, generateFile, _option);

            if (generateFile)
                sHTML += "<p>" + sDescription + "</p>\n<div style=\"margin-left:40px;\">\n<code><span style=\"color:#00008B;\">\n";
            else
                sHTML += "<p>" + sDescription + "</p>\n<div>\n<code><span style=\"color:#00008B;\">\n";

            for (unsigned int j = i+1; j < vDocArticle.size(); j++)
            {
                if (vDocArticle[j].find("</example>") != string::npos)
                {
                    i = j;
                    sHTML.erase(sHTML.length()-5);
                    sHTML += "\n</span></code>\n</div>\n";
                    break;
                }

                if (vDocArticle[j] == "[...]")
                {
                    sHTML += "[...]<br>\n";
                    continue;
                }

                doc_ReplaceTokensForHTML(vDocArticle[j], generateFile, _option);

                if (!bVerb)
                {
                    if (((i+1) % 2 && j % 2) || (!((i+1) % 2) && !(j % 2)))
                    {
                        sHTML += "|&lt;- " + (vDocArticle[j]) + "<br>\n";
                    }
                    else
                    {
                        sHTML += "|-&gt; " + (vDocArticle[j]) + "<br>\n";

                        if (vDocArticle[j+1].find("</example>") == string::npos)
                            sHTML += "|<br>\n";
                    }
                }
                else
                {
                    sHTML += (vDocArticle[j]) + "<br>\n";
                }
            }
        }
        else if (vDocArticle[i].find("<exprblock>") != string::npos) // EXPRBLOCK-Tags
        {
            if (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != string::npos)
            {
                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
                doc_ReplaceExprContentForHTML(vDocArticle[i], _option);

                while (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != string::npos)
                {
                    string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<exprblock>")+11, vDocArticle[i].find("</exprblock>")-vDocArticle[i].find("<exprblock>")-11);

                    for (unsigned int k = 0; k < sExprBlock.length(); k++)
                    {
                        if (sExprBlock.substr(k,2) == "\\n")
                            sExprBlock.replace(k,2,"<br>");

                        if (sExprBlock.substr(k,2) == "\\t")
                            sExprBlock.replace(k,2,"&nbsp;&nbsp;&nbsp;&nbsp;");
                    }

                    if (generateFile)
                        vDocArticle[i].replace(vDocArticle[i].find("<exprblock>"), vDocArticle[i].find("</exprblock>")+12-vDocArticle[i].find("<exprblock>"), "</p><div style=\"font-style: italic;margin-left: 40px\">" + sExprBlock + "</div><p>");
                    else
                        vDocArticle[i].replace(vDocArticle[i].find("<exprblock>"), vDocArticle[i].find("</exprblock>")+12-vDocArticle[i].find("<exprblock>"), "</p><blockquote><span style=\"font-style: italic; font-family: palatino linotype; font-size: 12pt; font-weight: bold;\">" + sExprBlock + "</span></blockquote><p>");
                }

                sHTML += "<p>" + (vDocArticle[i]) + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<exprblock>")
                    sHTML += "<p>" + (vDocArticle[i].substr(0, vDocArticle[i].find("<exprblock>"))) + "</p>\n";

                if (generateFile)
                    sHTML += "<div style=\"font-style: italic;margin-left: 40px\">\n";
                else
                    sHTML += "<blockquote><span style=\"font-style: italic; font-family: palatino linotype; font-size: 12pt; font-weight: bold;\">\n";

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</exprblock>") != string::npos)
                    {
                        i = j;

                        if (generateFile)
                            sHTML += "</div>\n";
                        else
                        {
                            sHTML.erase(sHTML.length()-5);
                            sHTML += "\n</span></blockquote>\n";
                        }

                        break;
                    }

                    doc_ReplaceTokensForHTML(vDocArticle[j], generateFile, _option);
                    doc_ReplaceExprContentForHTML(vDocArticle[j], _option);

                    while (vDocArticle[j].find("\\t") != string::npos)
                        vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "&nbsp;&nbsp;&nbsp;&nbsp;");

                    sHTML += (vDocArticle[j]) + "<br>\n";
                }
            }
        }
        else if (vDocArticle[i].find("<codeblock>") != string::npos) // CODEBLOCK-Tags
        {
            if (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != string::npos)
            {
                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);

                while (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != string::npos)
                {
                    string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<codeblock>")+11, vDocArticle[i].find("</codeblock>")-vDocArticle[i].find("<codeblock>")-11);

                    for (unsigned int k = 0; k < sExprBlock.length(); k++)
                    {
                        if (sExprBlock.substr(k,2) == "\\n")
                            sExprBlock.replace(k,2,"<br>");

                        if (sExprBlock.substr(k,2) == "\\t")
                            sExprBlock.replace(k,2,"&nbsp;&nbsp;&nbsp;&nbsp;");
                    }

                    if (generateFile)
                        vDocArticle[i].replace(vDocArticle[i].find("<codeblock>"), vDocArticle[i].find("</codeblock>")+12-vDocArticle[i].find("<codeblock>"), "</p><div class=\"sites-codeblock sites-codesnippet-block\"><code><span style=\"color:#00008B;\">" + sExprBlock + "</span></code></div><p>");
                    else
                        vDocArticle[i].replace(vDocArticle[i].find("<codeblock>"), vDocArticle[i].find("</codeblock>")+12-vDocArticle[i].find("<codeblock>"), "</p><blockquote><code><span style=\"color:#00008B;\">" + sExprBlock + "</span></code></blockquote><p>");
                }

                sHTML += "<p>" + (vDocArticle[i]) + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<codeblock>")
                    sHTML += "<p>" + (vDocArticle[i].substr(0, vDocArticle[i].find("<codeblock>"))) + "</p>\n";

                if (generateFile)
                    sHTML += "<div class=\"sites-codeblock sites-codesnippet-block\"><code><span style=\"color:#00008B;\">\n";
                else
                    sHTML += "<blockquote><code><span style=\"color:#00008B;\">\n";

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</codeblock>") != string::npos)
                    {
                        i = j;

                        if (generateFile)
                            sHTML += "</span></code></div>\n";
                        else
                        {
                            sHTML.erase(sHTML.length()-5);
                            sHTML += "\n</span></code></blockquote>\n";
                        }

                        break;
                    }

                    doc_ReplaceTokensForHTML(vDocArticle[j], generateFile, _option);

                    while (vDocArticle[j].find("\\t") != string::npos)
                        vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "&nbsp;&nbsp;&nbsp;&nbsp;");

                    sHTML += (vDocArticle[j]) + "<br>\n";
                }
            }
        }
        else if (vDocArticle[i].find("<list") != string::npos) // Alle LIST-Tags (umgewandelt zu TABLE)
        {
            if (generateFile)
            {
                sHTML += "<h4>"+ _lang.get("DOC_HELP_OPTIONS_HEADLINE") +"</h4>\n";
                sHTML += "<table style=\"border-collapse:collapse; border-color:rgb(136,136,136);border-width:1px\" border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";
            }
            else
            {
                sHTML += "<table border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";
            }

            for (unsigned int j = i+1; j < vDocArticle.size(); j++)
            {
                if (vDocArticle[j].find("</list>") != string::npos)
                {
                    sHTML += "  </tbody>\n</table>\n";

                    i = j;
                    break;
                }
                else
                {
                    doc_ReplaceTokensForHTML(vDocArticle[j], generateFile, _option);

                    if (generateFile)
                    {
                        sHTML += "    <tr>\n";
                        sHTML += "      <td style=\"width:200px;height:19px\"><code><span style=\"color:#00008B;\">"
                             + (getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5))
                             + "</span></code></td>\n"
                             + "      <td style=\"width:400px;height:19px\">"
                             + (vDocArticle[j].substr(vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)+1, vDocArticle[j].find("</item>")-1-vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)))
                             + "</td>\n";
                        sHTML += "    </tr>\n";
                    }
                    else
                    {
                        if (isIndex)
                        {
                            sHTML += "    <tr>\n      <td width=\"200\"><a href=\"nhlp://"+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5)+"?frame=self\"><code><span style=\"color:#00008B;\">"
                                  + getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5)
                                  + "</span></code></a></td>\n      <td>"
                                  + vDocArticle[j].substr(vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)+1, vDocArticle[j].find("</item>")-1-vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2))
                                  + "</td>\n    </tr>\n";
                        }
                        else
                        {
                            sHTML += "    <tr>\n      <td width=\"200\"><code><span style=\"color:#00008B;\">"
                                  + getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5)
                                  + "</span></code></td>\n      <td>"
                                  + vDocArticle[j].substr(vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)+1, vDocArticle[j].find("</item>")-1-vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2))
                                  + "</td>\n    </tr>\n";
                        }
                    }
                }
            }
        }
        else if (vDocArticle[i].find("<table") != string::npos) // Table-Tags
        {
            if (generateFile)
                sHTML += "<div align=\"center\"><table style=\"border-collapse:collapse; border-color:rgb(136,136,136);border-width:1px\" border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";
            else
                sHTML += "<div align=\"center\"><table border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";

            for (unsigned int j = i+1; j < vDocArticle.size(); j++)
            {
                if (vDocArticle[j].find("</table>") != string::npos)
                {
                    sHTML += "  </tbody>\n</table></div>\n";
                    i = j;
                    break;
                }
                else
                {
                    doc_ReplaceTokensForHTML(vDocArticle[j], generateFile, _option);
                    sHTML += vDocArticle[j] + "\n";
                }
            }
        }
        else // Normaler Paragraph
        {
            doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
            sHTML += "<p>" + vDocArticle[i] + "</p>";

            if (generateFile)
                sHTML += "\n";
        }
    }

    if (generateFile)
        sHTML += "<!-- END COPYING HERE -->\n</body>\n</html>\n";
    else
        sHTML += "</body>\n</html>\n";

    return sHTML;
}

// Definierte Tokens durch Steuerzeichen ersetzen
void doc_ReplaceTokens(string& sDocParagraph, Settings& _option)
{
    for (unsigned int k = 0; k < sDocParagraph.length(); k++)
    {
        if (sDocParagraph.substr(k,6) == "<expr>" && sDocParagraph.find("</expr>", k+6) != string::npos)
        {
            string sExpr = sDocParagraph.substr(k+6, sDocParagraph.find("</expr>", k+6)-k-6);
            for (unsigned int i = 0; i < sExpr.length(); i++)
            {
                if (!i && sExpr[i] == '$')
                    sExpr.insert(0,"\\");
                if (sExpr[i] == '$' && sExpr[i-1] != '\\')
                    sExpr.insert(i,"\\");
                /*if (sExpr.substr(i,2) == "\\n")
                    sExpr.replace(i,2,"\n");*/
            }
            sDocParagraph.replace(k, sDocParagraph.find("</expr>",k+6)+7-k, sExpr);
        }
        if (sDocParagraph.substr(k,3) == "<a " && sDocParagraph.find("</a>", k+3) != string::npos)
        {
            string sExpr = sDocParagraph.substr(sDocParagraph.find('>', k+3)+1, sDocParagraph.find("</a>", k+3)-sDocParagraph.find('>', k+3)-1);
            sDocParagraph.replace(k, sDocParagraph.find("</a>",k)+4-k, sExpr);
        }
        if (sDocParagraph.substr(k,5) == "<img " && sDocParagraph.find("/>", k+5) != string::npos)
        {
            sDocParagraph.erase(k, sDocParagraph.find("/>",k)+5-k);
        }
        if (sDocParagraph.substr(k,6) == "<code>" && sDocParagraph.find("</code>", k+6) != string::npos)
        {
            string sCode = sDocParagraph.substr(k+6, sDocParagraph.find("</code>", k+6)-k-6);
            for (unsigned int i = 0; i < sCode.length(); i++)
            {
                if (sCode.substr(i,4) == "&gt;")
                    sCode.replace(i,4,">");
                if (sCode.substr(i,4) == "&lt;")
                    sCode.replace(i,4,"<");
                if (sCode.substr(i,6) == "&quot;")
                    sCode.replace(i,6,"\"");
                if (!i && sCode[i] == '$')
                    sCode.insert(0,"\\");
                if (sCode[i] == '$' && sCode[i-1] != '\\')
                    sCode.insert(i,"\\");
                /*if (sCode.substr(i,2) == "\\n")
                    sCode.replace(i,2,"\n");*/
            }
            sDocParagraph.replace(k, sDocParagraph.find("</code>",k+6)+7-k, "'"+sCode+"'");
            k += sCode.length();
        }
        if (sDocParagraph.substr(k,4) == "<em>" && sDocParagraph.find("</em>", k+4) != string::npos)
        {
            string sEmph = sDocParagraph.substr(k+4, sDocParagraph.find("</em>", k+4)-k-4);
            doc_ReplaceTokens(sEmph, _option);
            sDocParagraph.replace(k, sDocParagraph.find("</em>",k+4)+5-k, toUpperCase(sEmph));
            k += sEmph.length();
        }
        if (sDocParagraph.substr(k,3) == "<h>" && sDocParagraph.find("</h>", k+3) != string::npos)
        {
            string sEmph = sDocParagraph.substr(k+3, sDocParagraph.find("</h>", k+3)-k-3);
            doc_ReplaceTokens(sEmph, _option);
            sDocParagraph.replace(k, sDocParagraph.find("</h>",k+3)+4-k, toUpperCase(sEmph));
            k += sEmph.length();
        }
        if (sDocParagraph.substr(k,4) == "<br>")
            sDocParagraph.replace(k,4,"$");
        if (sDocParagraph.substr(k,4) == "&gt;")
            sDocParagraph.replace(k,4,">");
        if (sDocParagraph.substr(k,4) == "&lt;")
            sDocParagraph.replace(k,4,"<");
        if (sDocParagraph.substr(k,5) == "&amp;")
            sDocParagraph.replace(k,5,"&");
        if (sDocParagraph.substr(k,6) == "&quot;")
            sDocParagraph.replace(k,6,"\"");
        if (sDocParagraph.substr(k,10) == "&PLOTPATH&")
            sDocParagraph.replace(k,10,replacePathSeparator(_option.getPlotOutputPath()));
        if (sDocParagraph.substr(k,10) == "&LOADPATH&")
            sDocParagraph.replace(k,10,replacePathSeparator(_option.getLoadPath()));
        if (sDocParagraph.substr(k,10) == "&SAVEPATH&")
            sDocParagraph.replace(k,10,replacePathSeparator(_option.getSavePath()));
        if (sDocParagraph.substr(k,10) == "&PROCPATH&")
            sDocParagraph.replace(k,10,replacePathSeparator(_option.getProcsPath()));
        if (sDocParagraph.substr(k,12) == "&SCRIPTPATH&")
            sDocParagraph.replace(k,12,replacePathSeparator(_option.getScriptPath()));
        if (sDocParagraph.substr(k,9) == "&EXEPATH&")
            sDocParagraph.replace(k,9,replacePathSeparator(_option.getExePath()));
    }
    return;
}

// Definierte Tokens durch ggf. passende HTML-Tokens ersetzen
void doc_ReplaceTokensForHTML(string& sDocParagraph, bool generateFile, Settings& _option)
{
    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());

    for (unsigned int k = 0; k < sDocParagraph.length(); k++)
    {
        if (sDocParagraph.substr(k,2) == "\\$")
            sDocParagraph.erase(k,1);
        if (sDocParagraph.substr(k,3) == "\\\\n")
            sDocParagraph.erase(k,1);
        if (sDocParagraph.substr(k,2) == "  ")
            sDocParagraph.replace(k,1,"&nbsp;");
        if (sDocParagraph.substr(k,4) == "<em>" && sDocParagraph.find("</em>", k+4) != string::npos)
        {
            sDocParagraph.insert(k+4,"<strong>");
            sDocParagraph.insert(sDocParagraph.find("</em>",k+12),"</strong>");
        }
        if (sDocParagraph.substr(k,3) == "<h>" && sDocParagraph.find("</h>", k+3) != string::npos)
        {
            sDocParagraph.replace(k, 3, "<h4>");
            sDocParagraph.replace(sDocParagraph.find("</h>",k+4), 4, "</h4>");
        }
        if (sDocParagraph.substr(k,6) == "<expr>" && sDocParagraph.find("</expr>", k+6) != string::npos)
        {
            string sExpr = sDocParagraph.substr(k+6, sDocParagraph.find("</expr>", k+6)-k-6);
            doc_ReplaceExprContentForHTML(sExpr,_option);
            sDocParagraph.replace(k, sDocParagraph.find("</expr>",k+6)+7-k, "<span style=\"font-style:italic; font-family: palatino linotype; font-weight: bold;\">"+sExpr+"</span>");
        }
        if (sDocParagraph.substr(k,6) == "<code>" && sDocParagraph.find("</code>", k+6) != string::npos)
        {
            sDocParagraph.insert(k+6, "<span style=\"color:#00008B;\">");
            sDocParagraph.insert(sDocParagraph.find("</code>", k+6), "</span>");
            string sCode = sDocParagraph.substr(k+6, sDocParagraph.find("</code>", k+6)-k-6);
            for (unsigned int i = 0; i < sCode.length(); i++)
            {
                /*if (sCode.substr(i,6) == "&quot;")
                    sCode.replace(i,6,"\"");*/
                if (sCode.substr(i,2) == "\\n")
                    sCode.replace(i,2,"<br>");
            }
            //sDocParagraph.replace(k, sDocParagraph.find("</code>",k+6)+7-k, "'"+sCode+"'");
            k += sCode.length();
        }
        if (sDocParagraph.substr(k,5) == "<img " && sDocParagraph.find("/>", k+5) != string::npos)
        {
            string sImg = sDocParagraph.substr(k, sDocParagraph.find("/>", k+5)+2-k);
            if (sImg.find("src") != string::npos)
            {
                string sImgSrc = getArgAtPos(sImg, sImg.find('=', sImg.find("src"))+1);
                sImgSrc = _fSys.ValidFileName(sImgSrc, ".png");
                sImg = "<img src=\"" + sImgSrc + "\" />";
                sImg = "<div align=\"center\">" + sImg + "</div>";
            }
            else
                sImg.clear();

            sDocParagraph.replace(k, sDocParagraph.find("/>", k+5)+2-k, sImg);
            k += sImg.length();
        }
        if (sDocParagraph.substr(k, 10) == "&PLOTPATH&")
            sDocParagraph.replace(k, 10, generateFile ? "&lt;plotpath&gt;" : replacePathSeparator(_option.getPlotOutputPath()));
        if (sDocParagraph.substr(k, 10) == "&LOADPATH&")
            sDocParagraph.replace(k, 10, generateFile ? "&lt;loadpath&gt;" : replacePathSeparator(_option.getLoadPath()));
        if (sDocParagraph.substr(k, 10) == "&SAVEPATH&")
            sDocParagraph.replace(k, 10, generateFile ? "&lt;savepath&gt;" : replacePathSeparator(_option.getSavePath()));
        if (sDocParagraph.substr(k, 10) == "&PROCPATH&")
            sDocParagraph.replace(k, 10, generateFile ? "&lt;procpath&gt;" : replacePathSeparator(_option.getProcsPath()));
        if (sDocParagraph.substr(k, 12) == "&SCRIPTPATH&")
            sDocParagraph.replace(k, 12, generateFile ? "&lt;scriptpath&gt;" : replacePathSeparator(_option.getScriptPath()));
        if (sDocParagraph.substr(k, 9) == "&EXEPATH&")
            sDocParagraph.replace(k, 9, generateFile ? "&lt;&gt;" : replacePathSeparator(_option.getExePath()));
    }
    return;
}

// This function replaces tokens in <expr>-tags to
// improve the readability of mathematical code
void doc_ReplaceExprContentForHTML(string& sExpr, Settings& _option)
{
    // Get the mathstyle data base's contents
    static vector<vector<string> > vHTMLEntities = getDataBase("<>/docs/mathstyle.ndb", _option);
    size_t nPos = 0;

    // Set the starting position
    if (sExpr.find("<exprblock>") != string::npos)
        nPos = sExpr.find("<exprblock>")+11;

    // Try to match the tokens to those in the
    // data base and replace them
    for (size_t i = nPos; i < sExpr.length(); i++)
    {
        // If this is the end of the current
        // expression block, try to find a new
        // one or abort the current loop
        if (sExpr.substr(i, 12) == "</exprblock>")
        {
            if (sExpr.find("<exprblock>", i+12) != string::npos)
            {
                i = sExpr.find("<exprblock>", i+12) + 10;
                continue;
            }

            break;
        }

        // Match the tokens of the data base
        for (size_t n = 0; n < vHTMLEntities.size(); n++)
        {
            if (sExpr.substr(i, vHTMLEntities[n][0].length()) == vHTMLEntities[n][0]
                && ((vHTMLEntities[n][2] == "OP" && isOperator(sExpr, i, vHTMLEntities[n][0].length()))
                    || (vHTMLEntities[n][2] == "VAL" && isValue(sExpr, i, vHTMLEntities[n][0].length()))
                    || (vHTMLEntities[n][2] == "FCT" && isFunction(sExpr, i, vHTMLEntities[n][0].length())))
                )
            {
                sExpr.replace(i, vHTMLEntities[n][0].length(), vHTMLEntities[n][1]);
                i += vHTMLEntities[n][1].length()-1;
            }
        }

        // Handle supscripts
        if (sExpr[i] == '^')
        {
            if (sExpr[i+1] == '(')
            {
                sExpr.replace(getMatchingParenthesis(sExpr.substr(i))+i, 1, "</sup>");
                sExpr.replace(i, 2, "<sup>");
            }
            else
            {
                sExpr.insert(i+2, "</sup>");
                sExpr.replace(i, 1, "<sup>");
            }

            i += 4;
            continue;
        }

        // Handle subscripts
        if (sExpr[i] == '_')
        {
            if (sExpr[i+1] == '(')
            {
                sExpr.replace(getMatchingParenthesis(sExpr.substr(i))+i, 1, "</sub >");
                sExpr.replace(i, 2, "<sub>");
            }
            else
            {
                sExpr.insert(i+2, "</sub>");
                sExpr.replace(i, 1, "<sub>");
            }

            i += 4;
            continue;
        }

        // Insert whitespaces after commas
        if (sExpr[i] == ',' && sExpr[i+1] != ' ')
            sExpr.insert(i+1, 1, ' ');

        // Special case: autodetect numerical
        // subscripts
        if (i < sExpr.length()-1 && isdigit(sExpr[i+1]) && isalpha(sExpr[i]))
        {
            if (i < sExpr.length()-2)
                sExpr.insert(i+2, "</sub>");
            else
                sExpr.append("</sub>");

            sExpr.insert(i+1,"<sub>");
            i += 12;
        }
    }
}

vector<vector<string> > doc_readTokenTable(const string& sTable, Settings& _option)
{
    vector<vector<string> > vTable;
    vector<string> vLine;
    for (size_t i = 0; i < sTable.length(); i++) // <table> <tr> <td>
    {
        if (sTable.substr(i,4) == "<td>")
        {
            for (size_t j = i+4; j < sTable.length(); j++)
            {
                if (sTable.substr(j,5) == "</td>")
                {
                    vLine.push_back(sTable.substr(i+4, j-i-4));
                    doc_ReplaceTokens(vLine[vLine.size()-1], _option);
                    i = j+4;
                    break;
                }
            }
        }
        if (sTable.substr(i,5) == "</tr>")
        {
            vTable.push_back(vLine);
            vLine.clear();
            i += 4;
        }
        if (sTable.substr(i,8) == "</table>")
        {
            break;
        }
    }
    return vTable;
}

void doc_SearchFct(const string& sToLookFor, Settings& _option)
{
    bool bResult = false;
    string* sMultiTopics;
    string sToLookFor_cp = toLowerCase(sToLookFor);
    string sUsedIdxKeys = ";";
    static vector<vector<string> > vTopics;
    if (!vTopics.size() && _option.getUseCustomLanguageFiles() && fileExists(_option.ValidFileName("<>/user/docs/find.ndb", ".ndb")))
    {
        vector<vector<string> > vStdTopics = getDataBase("<>/docs/find.ndb", _option);
        vector<vector<string> > vLangTopics = getDataBase("<>/user/docs/find.ndb", _option);
        map<string,int> mTopics;
        for (unsigned int i = 0; i < vStdTopics.size(); i++)
            mTopics[toLowerCase(vStdTopics[i][0])] = i+1;
        for (unsigned int i = 0; i < vLangTopics.size(); i++)
            mTopics[toLowerCase(vLangTopics[i][0])] = -i-1;
        for (auto iter = mTopics.begin(); iter != mTopics.end(); ++iter)
        {
            if (iter->second > 0)
                vTopics.push_back(vStdTopics[(iter->second-1)]);
            else
                vTopics.push_back(vLangTopics[abs(iter->second+1)]);
        }
    }
    else if (!vTopics.size())
        vTopics = getDataBase("<>/docs/find.ndb", _option);
    if (!vTopics.size())
    {
        make_hline();
        NumeReKernel::print(LineBreak(_lang.get("DOC_SEARCHFCT_DB_ERROR"), _option));
        make_hline();
        return;
    }

    sToLookFor_cp = fromSystemCodePage(sToLookFor_cp);
    int nMultiTopics = 0;
    int nMatches[vTopics.size()][2];// save export lösung
    int nMax[2] = {0,0};
    int nCount = 0;

    StripSpaces(sToLookFor_cp);
    if (sToLookFor_cp == "help")
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        NumeReKernel::print(toSystemCodePage(toUpperCase(_lang.get("DOC_SEARCHFCT_TABLEHEAD"))));
        make_hline();
        for (unsigned int i = 0; i < vTopics.size(); i++)
        {
            if (vTopics[i][0].substr(0,4) == "help")
            {
                NumeReKernel::print("   [100%]   " + vTopics[i][0] + " -- " + LineBreak(vTopics[i][1], _option, true, 20+vTopics[i][0].length(), 20));
                break;
            }
        }
        NumeReKernel::printPreFmt("|\n");
        NumeReKernel::print(toSystemCodePage(_lang.get("DOC_SEARCHFCT_RESULT", "1")));
        NumeReKernel::toggleTableStatus();
        make_hline();
        return;
    }


    if (sToLookFor_cp.find(" ") != string::npos && sToLookFor_cp[0] != '"' && sToLookFor_cp[sToLookFor_cp.length()-1] != '"')
    {
        string sTemp = sToLookFor_cp;
        do
        {
            sTemp = sTemp.substr(sTemp.find(" ")+1);
            nMultiTopics++;
        }
        while (sTemp.find(" ") != string::npos);
        nMultiTopics++;
    }
    else if (sToLookFor_cp[0] == '"' && sToLookFor_cp[sToLookFor_cp.length()-1] == '"' && sToLookFor_cp.length() > 1)
    {
        nMultiTopics = 1;
        sToLookFor_cp.erase(0,1);
        sToLookFor_cp.erase(sToLookFor_cp.length()-1);
    }
    else
    {
        nMultiTopics = 1;
    }

    sMultiTopics = new string[nMultiTopics];

    if (nMultiTopics > 1)
    {
        sMultiTopics[0] = sToLookFor_cp;
        for (int k = 0; k < nMultiTopics-1; k++)
        {
            sMultiTopics[k+1] = sMultiTopics[k].substr(sMultiTopics[k].find(" ")+1);
            sMultiTopics[k] = sMultiTopics[k].substr(0,sMultiTopics[k].find(" "));
        }

        for (int k = 0; k < nMultiTopics; k++)
        {
            if (sMultiTopics[k][0] == ' ')
                sMultiTopics[k] = sMultiTopics[k].substr(1);
            if (sMultiTopics[k][sMultiTopics[k].length()-1] == ' ')
                sMultiTopics[k] = sMultiTopics[k].substr(0,sMultiTopics[k].length()-1);
        }
    }
    else
    {
        sMultiTopics[0] = sToLookFor_cp;
    }

    // --> Treffer finden, zaehlen und merken <--
    for (unsigned int i = 0; i < vTopics.size(); i++)
    {
        nMatches[i][0] = 0;
        nMatches[i][1] = 0;
        for (int k = 0; k < nMultiTopics; k++)
        {
            if (vTopics[i].size() < 3)
            {
                //cerr << vTopics[i][0] << endl << vTopics[i].size() << endl;
                if (sMultiTopics)
                    delete[] sMultiTopics;
                make_hline();
                NumeReKernel::print(LineBreak(_lang.get("DOC_SEARCHFCT_DB_ERROR"), _option));
                make_hline();
                return;
            }
            for (unsigned int j = 0; j < vTopics[i].size(); j++)
            {
                if (toLowerCase(vTopics[i][j]).find(sMultiTopics[k]) != string::npos && sMultiTopics[k].length() && sMultiTopics[k] != " ")
                {
                    nMatches[i][0]++;
                    nMatches[i][1] += vTopics[i].size() - j;
                        if (!bResult)
                        bResult = true;
                    break;
                }
            }
        }
        if (nMatches[i][0] > nMax[0])
            nMax[0] = nMatches[i][0];
        if (nMatches[i][1] > nMax[1])
            nMax[1] = nMatches[i][1];
    }
    // --> Nach Relevanz sortiert ausgeben <--
    if (bResult)
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        NumeReKernel::print(toSystemCodePage(toUpperCase(_lang.get("DOC_SEARCHFCT_TABLEHEAD"))));
        make_hline();

        if (nMax[0] % nMultiTopics && nMultiTopics > 1)
        {
            nMax[1] = nMax[1] / (double)nMax[0] * ( nMultiTopics - (nMax[0] % nMultiTopics) ) + nMax[1];
        }

        for (int k = nMultiTopics * vTopics[0].size(); k > 0; k--)
        {
            for (unsigned int i = 0; i < vTopics.size(); i++)
            {
                if (nMatches[i][1] == k)
                {
                    NumeReKernel::printPreFmt("|->    [");
                    if ((int)(nMatches[i][1] / (double)(nMax[1]) * 100) != 100)
                        NumeReKernel::printPreFmt(" ");
                    NumeReKernel::printPreFmt(toString((int)(nMatches[i][1] / (double)(nMax[1]) * 100)) + "%]   ");
                    if (vTopics[i][0] == "NumeRe v $$$")
                        NumeReKernel::printPreFmt("NumeRe v " + sVersion);
                    else
                        NumeReKernel::printPreFmt(toSystemCodePage(vTopics[i][0]));
                    NumeReKernel::printPreFmt(" -- ");
                    if (vTopics[i][0] == "NumeRe v $$$")
                        NumeReKernel::printPreFmt(LineBreak(vTopics[i][1].substr(0, vTopics[i][1].find("$$$")) + replacePathSeparator(_option.getExePath()), _option, true, 29+sVersion.length(), 20) + "\n");
                    else
                        NumeReKernel::printPreFmt(LineBreak(vTopics[i][1], _option, true, 20+vTopics[i][0].length(), 20) + "\n");
                    ///cerr << endl;
                    nCount++;
                }
            }
        }
        for (int i = 0; i < nMultiTopics; i++)
        {
            if (_option.getHelpIdxKey(sMultiTopics[i]) != "<<NONE>>")
            {
                if (sUsedIdxKeys.find(";" + _option.getHelpIdxKey(sMultiTopics[i]) + ";") != string::npos)
                    continue;
                NumeReKernel::print(LineBreak("   [HELP]   " + _option.getHelpIdxKey(sMultiTopics[i]) + " -- " + _option.getHelpArticleTitle(_option.getHelpIdxKey(sMultiTopics[i])), _option));
                sUsedIdxKeys += _option.getHelpIdxKey(sMultiTopics[i])+ ";";
                nCount++;
            }
        }
        NumeReKernel::printPreFmt("|\n");
        NumeReKernel::print(toSystemCodePage(_lang.get("DOC_SEARCHFCT_RESULT", toString(nCount))));
        NumeReKernel::toggleTableStatus();
        make_hline();
    }
    else
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        for (int i = 0; i < nMultiTopics; i++)
        {
            if (_option.getHelpIdxKey(sMultiTopics[i]) != "<<NONE>>")
            {
                if (sUsedIdxKeys.find(";" + _option.getHelpIdxKey(sMultiTopics[i]) + ";") != string::npos)
                    continue;
                if (!nCount)
                {
                    NumeReKernel::print(toSystemCodePage(toUpperCase(_lang.get("DOC_SEARCHFCT_TABLEHEAD"))));
                    make_hline();
                }
                NumeReKernel::print(LineBreak("   [HELP]   " + _option.getHelpIdxKey(sMultiTopics[i]) + " -- " + _option.getHelpArticleTitle(_option.getHelpIdxKey(sMultiTopics[i])), _option));
                sUsedIdxKeys += _option.getHelpIdxKey(sMultiTopics[i]) + ";";
                nCount++;
            }
        }
        if (!nCount)
            NumeReKernel::print(LineBreak(_lang.get("DOC_SEARCHFCT_NO_RESULTS", sToLookFor_cp), _option));
        else
        {
            NumeReKernel::printPreFmt("|\n");
            NumeReKernel::print(toSystemCodePage(_lang.get("DOC_SEARCHFCT_RESULT", toString(nCount))));
        }
        NumeReKernel::toggleTableStatus();
        make_hline();
    }
    delete[] sMultiTopics;
    sMultiTopics = 0;
    return;
}

void doc_FirstStart(const Settings& _option)
{
    string sInput = "";
    vector<string> vPageContents;
    NumeReKernel::toggleTableStatus();
    make_hline();

    for (int i = 1; i <= 7; i++)
    {
        NumeReKernel::print(toSystemCodePage(toUpperCase(_lang.get("DOC_FIRSTSTART_HEADLINE_PREFIX", _lang.get("DOC_FIRSTSTART_PAGE_"+toString(i)+"_HEAD"), toString(i), "7"))));
        make_hline();
        vPageContents = _lang.getList("DOC_FIRSTSTART_PAGE_"+toString(i)+"_LINE_*");
        for (unsigned int j = 0; j < vPageContents.size(); j++)
        {
            while (vPageContents[j].find("%%1%%") != string::npos)
            {
                vPageContents[j].replace(vPageContents[j].find("%%1%%"),5,sVersion);
            }
            NumeReKernel::print(LineBreak(vPageContents[j], _option));
        }
        vPageContents.clear();
        NumeReKernel::toggleTableStatus();
        NumeReKernel::printPreFmt("|   (" + toSystemCodePage(toUpperCase(_lang.get("DOC_FIRSTSTART_NEXTPAGE"))) + ") ");
        NumeReKernel::getline(sInput);
        StripSpaces(sInput);
        if (sInput == "0")
        {
            make_hline();
            return;
        }
        else
            sInput = "";
        NumeReKernel::toggleTableStatus();
        make_hline();
    }
    if (NumeReKernel::bWritingTable)
        NumeReKernel::toggleTableStatus();
    return;
}

void doc_TipOfTheDay(Settings& _option)
{
    vector<string> vTipList;
    if (_option.getUseCustomLanguageFiles() && fileExists(_option.ValidFileName("<>/user/docs/hints.ndb", ".ndb")))
        vTipList = getDBFileContent("<>/user/docs/hints.ndb", _option);
    else
        vTipList = getDBFileContent("<>/docs/hints.ndb", _option);
    unsigned int nth_tip = 0;
    // --> Einen Seed (aus der Zeit generiert) an die rand()-Funktion zuweisen <--
    srand(time(NULL));

    if (!vTipList.size())
        return;
    // --> Die aktuelle Begruessung erhalten wir als modulo(nGreetings)-Operation auf rand() <--
    nth_tip = (rand() % vTipList.size());
    if (nth_tip >= vTipList.size())
        nth_tip = vTipList.size()-1;

    NumeReKernel::toggleTableStatus();
    NumeReKernel::printPreFmt("|\n");
    //cerr << "|" << endl;
    make_hline();
    //cerr << "|-> NUMERE: SCHON GEWUSST?  [Nr. "<< nth_tip+1 << "/" << vTipList.size() << "]" << endl;
    NumeReKernel::print(toSystemCodePage(_lang.get("DOC_TIPOFTHEDAY_HEADLINE", toString(nth_tip+1), toString((int)vTipList.size()))));
    make_hline();
    NumeReKernel::print(LineBreak(vTipList[nth_tip], _option));
    //cerr << "|" << endl << LineBreak("|-> Diese Hinweise können mit \"set -hints=false\" deaktiviert werden.", _option) << endl;
    NumeReKernel::toggleTableStatus();
    make_hline();

    return;
}

