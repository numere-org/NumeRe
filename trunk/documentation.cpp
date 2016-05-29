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


void doc_Help(const string& __sTopic, Settings& _option)
{
    string sTopic = toLowerCase(__sTopic);
    if (matchParams(sTopic, "html"))
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
        cerr << LineBreak("|-> "+_lang.get("DOC_HELP_NO_ENTRY_FOUND", sTopic), _option) << endl;
        make_hline();
        return;
    }
    else if (matchParams(__sTopic, "html")) // HTML-Export generieren
    {
        ofstream fHTML;
        FileSystem _fSys;
        _fSys.setTokens(_option.getTokenPaths());
        _fSys.setPath("docs/htmlexport", true, _option.getExePath());
        string sFilename = "<>/docs/htmlexport/"+_option.getHelpArtclID(sTopic) + ".html";
        _option.declareFileType(".html");
        sFilename = _option.ValidFileName(sFilename,".html");

        fHTML.open(sFilename.c_str());
        if (fHTML.fail())
        {
            sErrorToken = sFilename;
            throw CANNOT_GENERATE_FILE;
        }
        // Header schreiben
        fHTML << "<!DOCTYPE html>" << endl
              << "<html>" << endl
              << "<head>" << endl;
        for (unsigned int i = 0; i < vDocArticle.size(); i++)
        {
            if (!i)
            {
                // Header fertigstellen
                fHTML << "<title>NUMERE-HILFE: " + toUpperCase(vDocArticle[i])
                      << "</title>" << endl
                      << "</head>" << endl << endl
                      << "<body>" << endl
                      << "<!-- START COPYING HERE -->" << endl;
                fHTML << "<h4>Beschreibung:</h4>" << endl;
                continue;
            }

            if (vDocArticle[i].find("<example ") != string::npos) // Beispiel-Tags
            {
                fHTML << "<h4>Beispiel</h4>" << endl;
                bool bVerb = false;
                if (vDocArticle[i].find("type=") && getArgAtPos(vDocArticle[i], vDocArticle[i].find("type=")+5) == "verbatim")
                    bVerb = true;

                doc_ReplaceTokensForHTML(vDocArticle[i], _option);
                fHTML << "<p>" << (getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5)) << "</p>" << endl;
                fHTML << "<div style=\"margin-left:40px;\"><code>" << endl;
                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != string::npos)
                    {
                        i = j;
                        fHTML << "</code></div>" << endl;
                        break;
                    }
                    if (vDocArticle[j] == "[...]")
                    {
                        fHTML << "[...]<br>" << endl;
                        continue;
                    }

                    doc_ReplaceTokensForHTML(vDocArticle[j], _option);

                    if (!bVerb)
                    {
                        if (((i+1) % 2 && j % 2) || (!((i+1) % 2) && !(j % 2)))
                        {
                            fHTML << "|&lt;- ";
                            fHTML << (vDocArticle[j]) << "<br>" << endl;
                        }
                        else
                        {
                            fHTML << "|-&gt; ";
                            fHTML << (vDocArticle[j]) << "<br>" << endl;
                            if (vDocArticle[j+1].find("</example>") == string::npos)
                                fHTML << "|<br>" << endl;
                        }
                    }
                    else
                    {
                        fHTML << (vDocArticle[j]) << "<br>" << endl;
                    }
                }
            }
            else if (vDocArticle[i].find("<exprblock>") != string::npos) // EXPRBLOCK-Tags
            {
                if (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != string::npos)
                {
                    doc_ReplaceTokensForHTML(vDocArticle[i], _option);
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
                        vDocArticle[i].replace(vDocArticle[i].find("<exprblock>"), vDocArticle[i].find("</exprblock>")+12-vDocArticle[i].find("<exprblock>"), "</p><div style=\"font-style: italic;margin-left: 40px\">" + sExprBlock + "</div><p>");
                    }

                    fHTML << "<p>" << (vDocArticle[i]) << "</p>" << endl;
                }
                else
                {
                    if (vDocArticle[i] != "<exprblock>")
                        fHTML << "<p>" << (vDocArticle[i].substr(0,vDocArticle[i].find("<exprblock>"))) << "</p>" << endl;
                    fHTML << "<div style=\"font-style: italic;margin-left: 40px\">" << endl;
                    for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                    {
                        if (vDocArticle[j].find("</exprblock>") != string::npos)
                        {
                            i = j;
                            fHTML << "</div>" << endl;
                            break;
                        }

                        doc_ReplaceTokensForHTML(vDocArticle[j], _option);
                        doc_ReplaceExprContentForHTML(vDocArticle[j], _option);

                        while (vDocArticle[j].find("\\t") != string::npos)
                            vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "&nbsp;&nbsp;&nbsp;&nbsp;");

                        fHTML << (vDocArticle[j]) << "<br>" << endl;
                    }
                }
            }
            else if (vDocArticle[i].find("<codeblock>") != string::npos) // CODEBLOCK-Tags
            {
                if (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != string::npos)
                {
                    doc_ReplaceTokensForHTML(vDocArticle[i], _option);
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
                        vDocArticle[i].replace(vDocArticle[i].find("<codeblock>"), vDocArticle[i].find("</codeblock>")+12-vDocArticle[i].find("<codeblock>"), "</p><div class=\"sites-codeblock sites-codesnippet-block\"><code>" + sExprBlock + "</code></div><p>");
                    }
                    fHTML << "<p>" << (vDocArticle[i]) << "</p>" << endl;
                }
                else
                {
                    if (vDocArticle[i] != "<codeblock>")
                        fHTML << "<p>" << (vDocArticle[i].substr(0,vDocArticle[i].find("<codeblock>"))) << "</p>" << endl;
                    fHTML << "<div class=\"sites-codeblock sites-codesnippet-block\"><code>" << endl;
                    for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                    {
                        if (vDocArticle[j].find("</codeblock>") != string::npos)
                        {
                            i = j;
                            fHTML << "</code></div>" << endl;
                            break;
                        }

                        doc_ReplaceTokensForHTML(vDocArticle[j], _option);
                        while (vDocArticle[j].find("\\t") != string::npos)
                            vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "&nbsp;&nbsp;&nbsp;&nbsp;");

                        fHTML << (vDocArticle[j]) << "<br>" << endl;
                    }
                }
            }
            else if (vDocArticle[i].find("<list") != string::npos) // Alle LIST-Tags (umgewandelt zu TABLE)
            {
                fHTML << "<h4>Optionen:</h4>" << endl;
                fHTML << "<table style=\"border-collapse:collapse; border-color:rgb(136,136,136);border-width:1px\" border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">" << endl << "  <tbody>" << endl;
                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</list>") != string::npos)
                    {
                        fHTML << "  </tbody>" << endl
                             << "</table>" << endl;
                        i = j;
                        break;
                    }
                    else
                    {
                        doc_ReplaceTokensForHTML(vDocArticle[j], _option);
                        fHTML << "    <tr>" << endl;
                        fHTML << "      <td style=\"width:200px;height:19px\"><code>"
                             << (getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5))
                             << "</code></td>" << endl
                             << "      <td style=\"width:400px;height:19px\">"
                             << (vDocArticle[j].substr(vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)+1, vDocArticle[j].find("</item>")-1-vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)))
                             << "</td>" << endl;
                        fHTML << "    </tr>" << endl;
                    }
                }
            }
            else // Normaler Paragraph
            {
                doc_ReplaceTokensForHTML(vDocArticle[i], _option);
                fHTML << "<p>" << (vDocArticle[i]) << "</p>" << endl;
            }
        }
        fHTML << "<!-- END COPYING HERE -->" << endl
              << "</body>" << endl
              << "</html>" << endl;
        fHTML.close();
        cerr << LineBreak("|-> "+_lang.get("DOC_HELP_HTMLEXPORT", _option.getHelpArticleTitle(_option.getHelpIdxKey(sTopic)), sFilename), _option) << endl;
        return;
    }
    else // Hilfeartikel anzeigen
    {
        make_hline();
        for (unsigned int i = 0; i < vDocArticle.size(); i++)
        {
            if (!i)
            {
                cerr << toSystemCodePage("|-> " + toUpperCase(_lang.get("DOC_HELP_HEADLINE", vDocArticle[i]))) << endl;
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
                    cerr << LineBreak("|-> " + _lang.get("DOC_HELP_EXAMPLE", getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5)), _option) << endl;
                else
                    cerr << LineBreak("|-> " + _lang.get("DOC_HELP_EXAMPLE", getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5)), _option, false) << endl;
                cerr << "|" << endl;
                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != string::npos)
                    {
                        i = j;
                        if (i+1 < vDocArticle.size())
                            cerr << "|" << endl;
                        break;
                    }
                    if (vDocArticle[j] == "[...]")
                    {
                        cerr << "|[...]" << endl;
                        continue;
                    }

                    doc_ReplaceTokens(vDocArticle[j], _option);

                    if (!bVerb)
                    {
                        if (((i+1) % 2 && j % 2) || (!((i+1) % 2) && !(j % 2)))
                        {
                            cerr << "||<- ";
                            cerr << LineBreak(vDocArticle[j], _option, false, 5) << endl;
                        }
                        else
                        {
                            cerr << "||-> ";
                            cerr << LineBreak(vDocArticle[j], _option, false, 5) << endl;
                            if (vDocArticle[j+1].find("</example>") == string::npos)
                                cerr << "||" << endl;
                        }
                    }
                    else
                    {
                        cerr << "|" << toSystemCodePage(vDocArticle[j]) << endl;
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
                    cerr << LineBreak("|-> " + vDocArticle[i], _option, false) << endl;
                }
                else
                {
                    if (vDocArticle[i] != "<exprblock>")
                        cerr << LineBreak("|-> " + vDocArticle[i].substr(0,vDocArticle[i].find("<exprblock>")), _option) << endl;
                    cerr << "|" << endl;
                    for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                    {
                        if (vDocArticle[j].find("</exprblock>") != string::npos)
                        {
                            i = j;
                            if (i+1 < vDocArticle.size())
                                cerr << "|" << endl;
                            break;
                        }

                        doc_ReplaceTokens(vDocArticle[j], _option);
                        while (vDocArticle[j].find("\\t") != string::npos)
                            vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "    ");

                        cerr << "|     " << toSystemCodePage(vDocArticle[j]) << endl;
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
                    cerr << LineBreak("|-> " + vDocArticle[i], _option, false) << endl;
                }
                else
                {
                    if (vDocArticle[i] != "<codeblock>")
                        cerr << LineBreak("|-> " + vDocArticle[i].substr(0,vDocArticle[i].find("<codeblock>")), _option) << endl;
                    cerr << "|" << endl;
                    for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                    {
                        if (vDocArticle[j].find("</codeblock>") != string::npos)
                        {
                            i = j;
                            if (i+1 < vDocArticle.size())
                                cerr << "|" << endl;
                            break;
                        }

                        doc_ReplaceTokens(vDocArticle[j], _option);
                        while (vDocArticle[j].find("\\t") != string::npos)
                            vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "    ");

                        cerr << "|     " << toSystemCodePage(vDocArticle[j]) << endl;
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
                                cerr << "|     " + sNode;
                                nIndent = sNode.length()+6;
                                sLine.append(nLengthMax+9-nIndent, ' ');
                                sLine += "- " + sRemainingLine;
                                if (sLine.find('~') == string::npos && sLine.find('%') == string::npos)
                                    cerr << LineBreak(sLine, _option, true, nIndent, nLengthMax+11) << endl;
                                else
                                    cerr << LineBreak(sLine, _option, false, nIndent, nLengthMax+11) << endl;
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
                                        if (sLine.find('\n') != string::npos);
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
                                cerr << sFinalLine << endl;
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
                                        cerr << "|     " + toUpperCase(getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5));
                                    else
                                        cerr << "|     " + getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5);
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
                                    cerr << LineBreak(sLine, _option, true, nIndent, 10) << endl;
                                else
                                    cerr << LineBreak(sLine, _option, false, nIndent, 10) << endl;
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
                                    cerr << "|     " + getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5);
                                    nIndent = getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+6;
                                    sLine.append(nLengthMax+9-nIndent, ' ');
                                }
                                else
                                {
                                    sLine = "|     " + getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5);
                                    sLine.append(nLengthMax+9-sLine.length()+countEscapeSymbols(sLine), ' ');
                                }
                                sLine += "- [...]";
                                cerr << LineBreak(sLine, _option, false, nIndent, nLengthMax+11) << endl;
                                mListitems[getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5)] = vDocArticle[k].substr(vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2)+1, vDocArticle[k].find("</item>")-1-vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2));
                            }
                            cerr << LineBreak("|-> GEFALTETE LISTE: Stichpunkt eingeben, um Beschreibung anzuzeigen. \"0\" eingeben, um die Liste zu verlassen:", _option) << endl;
                            while (true)
                            {
                                cerr << "|LIST> ";
                                getline(cin, sLine);
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
                                cerr << LineBreak("|     " + sLine, _option, false, 0, nLengthMax+11) << endl;
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
            else // Normaler Paragraph
            {
                doc_ReplaceTokens(vDocArticle[i], _option);

                if (vDocArticle[i].find('~') == string::npos && vDocArticle[i].find('%') == string::npos)
                    cerr << LineBreak("|-> " + vDocArticle[i], _option) << endl;
                else
                    cerr << LineBreak("|-> " + vDocArticle[i], _option, false) << endl;
            }
        }
        make_hline();
        return;
    }

    return;
}

// Definierte Tokens durch Steuerzeichen ersetzen
void doc_ReplaceTokens(string& sDocParagraph, const Settings& _option)
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
void doc_ReplaceTokensForHTML(string& sDocParagraph, const Settings& _option)
{
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
        if (sDocParagraph.substr(k,6) == "<expr>" && sDocParagraph.find("</expr>", k+6) != string::npos)
        {
            string sExpr = sDocParagraph.substr(k+6, sDocParagraph.find("</expr>", k+6)-k-6);
            doc_ReplaceExprContentForHTML(sExpr,_option);
            sDocParagraph.replace(k, sDocParagraph.find("</expr>",k+6)+7-k, "<em>"+sExpr+"</em>");
        }
        if (sDocParagraph.substr(k,6) == "<code>" && sDocParagraph.find("</code>", k+6) != string::npos)
        {
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
        if (sDocParagraph.substr(k,10) == "&PLOTPATH&")
            sDocParagraph.replace(k,10,"&lt;plotpath&gt;");
        if (sDocParagraph.substr(k,10) == "&LOADPATH&")
            sDocParagraph.replace(k,10,"&lt;loadpath&gt;");
        if (sDocParagraph.substr(k,10) == "&SAVEPATH&")
            sDocParagraph.replace(k,10,"&lt;savepath&gt;");
        if (sDocParagraph.substr(k,10) == "&PROCPATH&")
            sDocParagraph.replace(k,10,"&lt;procpath&gt;");
        if (sDocParagraph.substr(k,12) == "&SCRIPTPATH&")
            sDocParagraph.replace(k,12,"&lt;scriptpath&gt;");
        if (sDocParagraph.substr(k,9) == "&EXEPATH&")
            sDocParagraph.replace(k,9,"&lt;&gt;");
    }
    return;
}

void doc_ReplaceExprContentForHTML(string& sExpr, const Settings& _option)
{
    static const unsigned int nEntities = 10;
    static const string sHTMLEntities[nEntities][2] =
        {
            { "_2pi",   "2&pi;"},
            {  "_pi",    "&pi;"},
            {   "PI",    "&pi;"},
            {   "pi",    "&pi;"},
            {  "chi",   "&chi;"},
            {  "phi",   "&phi;"},
            {  "Phi",   "&Phi;"},
            {  "rho",   "&rho;"},
            {"theta", "&theta;"},
            {"delta", "&delta;"},
        };
    unsigned int nPos = 0;
    if (sExpr.find("<exprblock>") != string::npos)
        nPos = sExpr.find("<exprblock>")+11;
    for (unsigned int i = nPos; i < sExpr.length(); i++)
    {
        if (sExpr.substr(i,12) == "</exprblock>")
        {
            if (sExpr.find("<exprblock>", i+12) != string::npos)
            {
                i = sExpr.find("<exprblock>",i+12)+10;
                continue;
            }
            break;
        }
        if (sExpr.substr(i,2) == "<=")
        {
            sExpr.replace(i,2,"&le;");
            i += 3;
        }
        if (sExpr.substr(i,5) == "&lt;=")
        {
            sExpr.replace(i,5,"&le;");
            i += 3;
        }
        if (sExpr.substr(i,2) == ">=")
        {
            sExpr.replace(i,2,"&ge;");
            i += 3;
        }
        if (sExpr.substr(i,5) == "&gt;=")
        {
            sExpr.replace(i,5,"&ge;");
            i += 3;
        }
        for (unsigned int n = 0; n < nEntities; n++)
        {
            if (sExpr.substr(i,sHTMLEntities[n][0].length()) == sHTMLEntities[n][0]
                && (!i
                    || !isalpha(sExpr[i-1]))
                && (i+sHTMLEntities[n][0].length() == sExpr.length()
                    || !isalpha(sExpr[i+sHTMLEntities[n][0].length()]))
                )
            {
                sExpr.replace(i,sHTMLEntities[n][0].length(),sHTMLEntities[n][1]);
                i += sHTMLEntities[n][1].length()-1;
            }
        }

        /*if (sExpr.substr(i,3) == "chi" && (!i || !isalpha(sExpr[i-1])) && (i + 3 == sExpr.length() || !isalpha(sExpr[i+3])))
        {
            sExpr.replace(i,3,"&chi;");
            i += 4;
        }
        if (sExpr.substr(i,3) == "Phi" && (!i || !isalpha(sExpr[i-1])) && (i + 3 == sExpr.length() || !isalpha(sExpr[i+3])))
        {
            sExpr.replace(i,3,"&Phi;");
            i += 4;
        }
        if (sExpr.substr(i,3) == "phi" && (!i || !isalpha(sExpr[i-1])) && (i + 3 == sExpr.length() || !isalpha(sExpr[i+3])))
        {
            sExpr.replace(i,3,"&phi;");
            i += 4;
        }
        if (sExpr.substr(i,3) == "rho" && (!i || !isalpha(sExpr[i-1])) && (i + 3 == sExpr.length() || !isalpha(sExpr[i+3])))
        {
            sExpr.replace(i,3,"&rho;");
            i += 4;
        }
        if (sExpr.substr(i,5) == "theta" && (!i || !isalpha(sExpr[i-1])) && (i + 5 == sExpr.length() || !isalpha(sExpr[i+5])))
        {
            sExpr.replace(i,5,"&theta;");
            i += 7;
        }
        if (sExpr.substr(i,5) == "delta" && (!i || !isalpha(sExpr[i-1])) && (i + 5 == sExpr.length() || !isalpha(sExpr[i+5])))
        {
            sExpr.replace(i,5,"&delta;");
            i += 7;
        }*/
        if (sExpr[i] == '^')
        {
            sExpr.insert(i+2,"</sup>");
            sExpr.replace(i,1,"<sup>");
        }
        if (sExpr[i] == '_')
        {
            sExpr.insert(i+2,"</sub>");
            sExpr.replace(i,1,"<sub>");
            i += 7;
        }
        if (i < sExpr.length()-1 && isdigit(sExpr[i+1]) && isalpha(sExpr[i]))
        {
            if (i < sExpr.length()-2)
                sExpr.insert(i+2, "</sub>");
            else
                sExpr.append("</sub>");
            sExpr.insert(i+1,"<sub>");
            i += 7;
        }
        if (sExpr.substr(i,2) == "\\n")
            sExpr.replace(i,2,"<br>");
    }
}

void doc_SearchFct(const string& sToLookFor, Settings& _option)
{
    bool bResult = false;
    string* sMultiTopics;
    string sToLookFor_cp = toLowerCase(sToLookFor);
    string sUsedIdxKeys = ";";
    static vector<vector<string> > vTopics;
    if (!vTopics.size() && fileExists(_option.ValidFileName("<>/user/docs/find.ndb", ".ndb")))
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
        cerr << LineBreak("|-> "+_lang.get("DOC_SEARCHFCT_DB_ERROR"), _option) << endl;
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
        make_hline();
        cerr << "|-> " << toSystemCodePage(toUpperCase(_lang.get("DOC_SEARCHFCT_TABLEHEAD"))) << endl;
        make_hline();
        for (unsigned int i = 0; i < vTopics.size(); i++)
        {
            if (vTopics[i][0].substr(0,4) == "help")
            {
                cerr << "|->    [100%]   " << vTopics[i][0] << " -- " << LineBreak(vTopics[i][1], _option, true, 20+vTopics[i][0].length(), 20) << endl;
                break;
            }
        }
        cerr << "|" << endl;
        cerr << "|-> " << toSystemCodePage(_lang.get("DOC_SEARCHFCT_RESULT", "1")) << endl;
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
                if (sMultiTopics)
                    delete[] sMultiTopics;
                make_hline();
                cerr << LineBreak("|-> "+_lang.get("DOC_SEARCHFCT_DB_ERROR"), _option) << endl;
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
        make_hline();
        cerr << "|-> " << toSystemCodePage(toUpperCase(_lang.get("DOC_SEARCHFCT_TABLEHEAD"))) << endl;
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
                    cerr << "|->    [";
                    if ((int)(nMatches[i][1] / (double)(nMax[1]) * 100) != 100)
                        cerr << " ";
                    cerr << (int)(nMatches[i][1] / (double)(nMax[1]) * 100)
                         << "%]   ";
                    if (vTopics[i][0] == "NumeRe v $$$")
                        cerr << "NumeRe v " << sVersion;
                    else
                        cerr << toSystemCodePage(vTopics[i][0]);
                    cerr << " -- ";
                    if (vTopics[i][0] == "NumeRe v $$$")
                        cerr << LineBreak(vTopics[i][1].substr(0, vTopics[i][1].find("$$$")) + replacePathSeparator(_option.getExePath()), _option, true, 29+sVersion.length(), 20);
                    else
                        cerr << LineBreak(vTopics[i][1], _option, true, 20+vTopics[i][0].length(), 20);
                    cerr << endl;
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
                cerr << LineBreak("|->    [HELP]   " + _option.getHelpIdxKey(sMultiTopics[i]) + " -- " + _option.getHelpArticleTitle(_option.getHelpIdxKey(sMultiTopics[i])), _option) << endl;
                sUsedIdxKeys += _option.getHelpIdxKey(sMultiTopics[i])+ ";";
                nCount++;
            }
        }
        cerr << "|" << endl;
        cerr << "|-> " << toSystemCodePage(_lang.get("DOC_SEARCHFCT_RESULT", toString(nCount))) << endl;
        make_hline();
    }
    else
    {
        make_hline();
        for (int i = 0; i < nMultiTopics; i++)
        {
            if (_option.getHelpIdxKey(sMultiTopics[i]) != "<<NONE>>")
            {
                if (sUsedIdxKeys.find(";" + _option.getHelpIdxKey(sMultiTopics[i]) + ";") != string::npos)
                    continue;
                if (!nCount)
                {
                    cerr << "|-> " << toSystemCodePage(toUpperCase(_lang.get("DOC_SEARCHFCT_TABLEHEAD"))) << endl;
                    make_hline();
                }
                cerr << LineBreak("|->    [HELP]   " + _option.getHelpIdxKey(sMultiTopics[i]) + " -- " + _option.getHelpArticleTitle(_option.getHelpIdxKey(sMultiTopics[i])), _option) << endl;
                sUsedIdxKeys += _option.getHelpIdxKey(sMultiTopics[i]) + ";";
                nCount++;
            }
        }
        if (!nCount)
            cerr << LineBreak("|-> "+_lang.get("DOC_SEARCHFCT_NO_RESULTS", sToLookFor_cp), _option) << endl;
        else
        {
            cerr << "|" << endl;
            cerr << "|-> " << toSystemCodePage(_lang.get("DOC_SEARCHFCT_RESULT", toString(nCount))) << endl;
        }
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
    make_hline();

    for (int i = 1; i <= 7; i++)
    {
        cerr << toSystemCodePage("|-> "+toUpperCase(_lang.get("DOC_FIRSTSTART_HEADLINE_PREFIX", _lang.get("DOC_FIRSTSTART_PAGE_"+toString(i)+"_HEAD"), toString(i), "7"))) << endl;
        make_hline();
        vPageContents = _lang.getList("DOC_FIRSTSTART_PAGE_"+toString(i)+"_LINE_*");
        for (unsigned int j = 0; j < vPageContents.size(); j++)
        {
            while (vPageContents[j].find("%%1%%") != string::npos)
            {
                vPageContents[j].replace(vPageContents[j].find("%%1%%"),5,sVersion);
            }
            cerr << LineBreak("|-> "+vPageContents[j], _option) << endl;
        }
        vPageContents.clear();
        cerr << "|   (" << toSystemCodePage(toUpperCase(_lang.get("DOC_FIRSTSTART_NEXTPAGE"))) << ") ";
        getline(cin, sInput);
        StripSpaces(sInput);
        if (sInput == "0")
        {
            make_hline();
            return;
        }
        else
            sInput = "";
        make_hline();
    }

    /*cerr << "|-> NUMERE: ERSTER START [EINSTIEG -- SEITE 1/7]" << endl;
    make_hline();
    cerr << LineBreak("|-> HALLO UND WILLKOMMEN!$Ich bin NumeRe v " + sVersion + ", Framework für Numerische Rechnungen, und freue mich, dich zum ersten Mal zu begrüßen!", _option) << endl;
    cerr << LineBreak("|-> Ich bin ausgelegt als eine Tabellenkalkulation, die rein auf der Konsole basiert, allerdings bin ich für die (Natur-)Wissenschaftliche Arbeit optimiert. So verfüge ich über die Möglichkeit, durch Scripte vordefinierte Abläufe zu automatisieren, graphische Plots in vielerlei Varianten zu erzeugen, oder einige andere Dinge.", _option) << endl;
    cerr << LineBreak("|-> Diese Einführung werde ich nur dieses eine Mal automatisch anzeigen. Falls du sie später noch mal sehen willst, gebe einfach \"firststart\" in die Konsole ein. Außerdem gibt es die Möglichkeit, zu allen Themen eine Hilfe zu erhalten: gib einfach \"help THEMA\" ein, um meine Hilfe zum THEMA aufzurufen, oder lass' THEMA weg, um die Hilfeübersicht anzuzeigen. (Beispiel: \"help data\")", _option) << endl;
    cerr << LineBreak("|-> Solltest du einmal ein Kommando oder eine Funktion nicht finden können, kannst du meine Stichwortsuche verwenden: \"find BEGRIFFE\". Wie durch BEGRIFFE angedeutet, kannst du hier auch mehrere Begriffe angeben, die durch Leerzeichen getrennt sein müssen. (Beispiel: \"find funktionen definieren\")", _option) << endl;
    cerr << LineBreak("|-> HINWEIS: Dies ist nur eine knappe Einführung, in der für alle Feinheiten meiner Syntax kein Platz ist. Sieh dir einfach die Hilfeartikel an, die ich im Folgenden angeben werde, falls du irgendwo nicht weiterkommst, oder schau in die NumeRe-Onlinereferenz: <https://sites.google.com/site/numereframework/onlinereferenz>", _option) << endl;
    cerr << LineBreak("|-> TIPP: Da dies mein erster Start auf deinem Rechner zu sein scheint, wirst du eben beobachtet haben, wie ich einen ausführlichen Test durchgeführt habe. Wenn du nicht möchtest, dass ich das nochmal mache, gib nach dieser Einführung (oder auch später) \"set -faststart=1\" ein.", _option) << endl;
    cerr << "|   (ENTER ZUM FORTFAHREN / 0+ENTER ZUM ABBRECHEN) ";
    getline(cin, sInput);
    StripSpaces(sInput);
    if (sInput == "0")
    {
        make_hline();
        return;
    }
    else
        sInput = "";
    make_hline();
    cerr << "|-> NUMERE: ERSTER START [EIN- & AUSGABE -- SEITE 2/7]" << endl;
    make_hline();
    cerr << LineBreak("|-> Als Framework für Numerische Rechnungen kann ich natürlich vor allem eines: eingegebene Ausdrücke numerisch auswerten. Dazu kannst du den Ausdruck, den ich dir auswerten soll, im Prinzip eingeben, wie du ihn in einen beliebigen Taschenrechner eintippen würdest. Der Unterschied liegt lediglich darin, dass ich mit Variablen umgehen kann und Leerstellen natürlich bedeutungslos sind. (Beispiel: \"5*23*x + 2*x^2 - 15*cos(2)\")", _option) << endl;
    cerr << LineBreak("|-> Diese Variablen musst du nicht zuvor deklarieren. Ich erkenne auch Variablen, die mit einem Ausdruck eingegeben werden, als solche und werde sie entsprechend in meinem Speicher ablegen. Du brauchst dir also darüber keine Sorgen machen. (Allerdings solltest du beachten, dass neue Variablen stets den Wert 0 haben.)", _option) << endl;
    cerr << LineBreak("|-> Deine Variablen können aus Buchstaben, Unterstrichen und Zahlen bestehen, jedoch dürfen sie niemals mit einer Ziffer beginnen. (Z.B.: \"x\", \"x0\", \"_var_Wert\", ...)", _option) << endl;
    cerr << LineBreak("|-> Den numerischen Wert, den ich ausrechne, werde ich in der nächsten Zeile als \"ans = WERT\" ausgeben. Dabei werde ich diesen Wert eben auch der speziellen Variable \"ans\" zuweisen, so dass du beim nächsten Ausdruck damit weiterrechnen kannst.", _option) << endl;
    cerr << LineBreak("|-> Ich kann auch numerisch Differenzieren und Integrieren, sowie Extrema und Nullstellen suchen. Die dazu nötigen Kommandos sowie eine komplette Liste aller anderen findest du unter \"list -cmd\".", _option) << endl;
    cerr << LineBreak("|-> TIPP: Weiterführende Infos findest du unter \"help expression\", \"help var\", \"help integrate\", \"help diff\", \"help extrema\" und \"help zeroes\"", _option) << endl;
    cerr << "|   (ENTER ZUM FORTFAHREN / 0+ENTER ZUM ABBRECHEN) ";
    getline(cin, sInput);
    StripSpaces(sInput);
    if (sInput == "0")
    {
        make_hline();
        return;
    }
    else
        sInput = "";
    make_hline();
    cerr << "|-> NUMERE: ERSTER START [NUMERE EINRICHTEN & VERBESSERN -- SEITE 3/7]" << endl;
    make_hline();
    cerr << LineBreak("|-> Zwar wurde ich von meinem Entwickler schon mit vernünftigen Standardeinstellungen versehen, doch möglicherweise hast du einen anderen Workflow und möchtest deswegen etwas an meinen Einstellungen ändern. Dazu gibt es das Kommando \"set\":$set -EINSTELLUNG=WERT$(Manche Einstellungen haben nur die Werte 1/0, andere explizit numerische Werte und einige auch Zeichenketten. Die Bezeichnungen der Einstellungswerte findest du im entsprechenden Hilfeartikel.)", _option) << endl;
    cerr << LineBreak("|-> Um meine Einstellungen zu lesen, verwendest du das Kommando \"get\" (get -EINSTELLUNG) und um sie in einer kompakten Liste anzeigen zu lassen \"list -settings\".", _option) << endl;
    cerr << LineBreak("|-> Vielleicht findest du, dass bei mir etwas nicht so ganz stimmt, oder dass mir noch was ganz wichtiges fehlt. Sollte dem so sein, dann freut sich mein Entwickler über deine Nachricht. Schreibe ihm entweder per Mail <numere.developer@gmail.com>, oder trage deinen Wunsch/Fehler im Bug- und Requesttracker ein: <https://sites.google.com/site/numereframework/to-dos>", _option) << endl;
    cerr << LineBreak("|-> TIPP: Weiterführende Infos findest du unter \"help set\", \"help get\" und \"about\"", _option) << endl;
    cerr << "|   (ENTER ZUM FORTFAHREN / 0+ENTER ZUM ABBRECHEN) ";
    getline(cin, sInput);
    StripSpaces(sInput);
    if (sInput == "0")
    {
        make_hline();
        return;
    }
    else
        sInput = "";
    make_hline();
    cerr << "|-> NUMERE: ERSTER START [DATENFILES -- SEITE 4/7]" << endl;
    make_hline();
    cerr << LineBreak("|-> Ich hatte eben einige Features angedeutet, die ich dir in den folgenden Zeilen vorstellen werde. Beginnen wir dabei zunächst mit der Arbeit mit Datensätzen:", _option) << endl;
    cerr << LineBreak("|-> Ich fasse jeden Datensatz als eine Tabelle auf. Tabellen als Textdatei als *.dat oder *.txt kannst du mittels des Kommandos \"load DATEI\", wobei DATEI der Dateiname des Datensatzes ist, in meinen Arbeitsspeicher laden. Kommentare müssen dabei durch ein \"#\" am Anfang der Zeile auskommentiert werden, Dezimaltrennzeichen kann der Punkt oder das Komma (oder beides) sein und die einzelnen Spalten müssen durch mindestens ein Leerzeichen oder einen Tabulator getrennt sein. (Beispiel: \"load samples/data\")", _option) << endl;
    cerr << LineBreak("|-> Ich kann des Weiteren auch die Tabelle aus einer CASSY(R)-LABX-Datei extrahieren, sie auswerten und ggf. in eine *.dat-Datei exportieren. Außerdem verstehe ich Comma Separated Value-Datafiles (*.csv), JCAMP-DX-Files (*.dx) und IGOR Binary Waves (*.ibw). Das zusätzliche \"NumeRe-Datafile\"-Format (*.ndat) verwende ich standardmäßig zum Speichern, allerdings handelt es sich dabei um ein binäres Format, das nur von mir gelesen werden kann. Ich kann die Daten aber auch in eine Text- oder eine TeXdatei exportieren, wenn das gewünscht ist.", _option) << endl;
    cerr << LineBreak("|-> Die geladenen Daten kann ich rasch analysieren. Wenn du \"stats data()\" eingibst, berechne ich dir die Statistiken der Datensätze, und bei \"hist data()\" berechne ich dir ein Histogramm der Daten. Tiefergehendere oder ausführlichere Auswertungen kann ich dir machen, wenn du die dazu nötigen Gleichungen direkt eingibst, wobei die Daten in meinem Arbeitsspeicher durch \"data(ZEILE,SPALTE)\" repräsentiert sind. (Beispiel: \"7*exp(data(12,3)^2)\")", _option) << endl;
    cerr << LineBreak("|-> TIPP: Weiterführende Infos findest du unter \"help data\", \"help load\", \"help stats\" und \"help hist\"", _option) << endl;
    cerr << "|   (ENTER ZUM FORTFAHREN / 0+ENTER ZUM ABBRECHEN) ";
    getline(cin, sInput);
    StripSpaces(sInput);
    if (sInput == "0")
    {
        make_hline();
        return;
    }
    else
        sInput = "";
    make_hline();
    cerr << "|-> NUMERE: ERSTER START [PLOTTEN -- SEITE 5/7]" << endl;
    make_hline();
    cerr << LineBreak("|-> Daneben verfüge ich über einen vielseitigen Plotalgorithmus, der dir eine graphische Darstellung von Funktionen oder Datensätzen in vielen Varianten ermöglicht.", _option) << endl;
    cerr << LineBreak("|-> Einfache 1D-Plots (z.B. von f(x) = sin(x)) mache ich dir durch die Eingabe von \"plot FUNKTION -set OPTIONEN\". Dabei ist FUNKTION durch den Funktionsausdruck und OPTIONEN durch die gewünschten Plotoptionen zu ersetzen. Statt eines Funktionsausdrucks kann hierbei auch ein Datensatz angegeben werden. (Beispiel: \"plot sin(x) -set [-_pi:_pi]\" oder \"plot data(:,1:3) -set yerrorbars\")", _option) << endl;
    cerr << LineBreak("|-> 2D-Plots von Funktionen der Form z = f(x,y) bekommst du durch \"mesh\", \"surf\", \"dens\" oder \"cont\", je nach gewünschter Darstellungsmethode. (z.B. \"surf exp(-norm(x,y)^2/5) -set [-5:5,-5:5] light\")", _option) << endl;
    cerr << LineBreak("|-> 3D-Plots von Trajektorien oder Skalarfeldern (Phi = Phi(x,y,z)) erzeuge ich durch das zusätzliche Anhängen von \"3d\" an ein Kommando (z.B. \"surf3d\").", _option) << endl;
    cerr << LineBreak("|-> Vektorfelder kann ich dir durch \"vect\" bzw. \"vect3d\" darstellen. (Beispiel: \"vect -y, x -set [-5:5,-5:5]\")", _option) << endl;
    cerr << LineBreak("|-> Meine Plots haben verschiedene Qualitätsstufen: standardmäßig mache ich alle Plots in der mittleren Stufe, da dies Rechenzeit spart. Willst du eine hohe Qualität, so gib die Option \"hires\" an. Du kannst auch den Entwurfsmodus verwenden. Gib dafür \"set -draftmode=0\" ein.", _option) << endl;
    cerr << LineBreak("|-> TIPP: Weiterführende Infos findest du unter \"help plot\", \"help plotoptions\", \"help plot3d\", \"help mesh\", \"help mesh3d\", \"help vect\" und \"help vect3d\"", _option) << endl;
    cerr << "|   (ENTER ZUM FORTFAHREN / 0+ENTER ZUM ABBRECHEN) ";
    getline(cin, sInput);
    StripSpaces(sInput);
    if (sInput == "0")
    {
        make_hline();
        return;
    }
    else
        sInput = "";
    make_hline();
    cerr << "|-> NUMERE: ERSTER START [AUTOMATISIEREN, SCRIPTE & PROZEDUREN -- SEITE 6/7]" << endl;
    make_hline();
    cerr << LineBreak("|-> Wie ich zuvor erwähnte, kann ich vordefinierte Abläufe automatisieren. Dazu stehen dir einfache Programmierfunktionen zur Verfügung, die du während der Laufzeit direkt eingeben, oder aber in ein sogenanntes \"NumeRe-Script\" (*.nscr) auslagern kannst.", _option) << endl;
    cerr << LineBreak("|-> Als Funktionen stehen dir die Zählschleife (FOR-Schleife), die bedingte Schleife (WHILE-Schleife) und die bedingte Verzweigung (IF-Verzweigung) zur Verfügung, die im Wesentlichen über dieselben Fähigkeiten wie die entsprechenden Varianten aus C/C++ verfügen.", _option) << endl;
    cerr << LineBreak("|-> Scripte kannst du durch das Kommando \"script -start=SCRIPT\", wobei SCRIPT durch den Dateinamen des Scripts zu ersetzen ist, starten. Wenn ein Script gestartet wurde, werde ich alle Ausdrücke zeilenweise abarbeiten, die ich in deinem Script finde. In meinem Unterverzeichnis \"samples\" sollten ein paar Beispielscripte zu finden sein, sofern du sie bei Installation nicht abgewählt hast. Teste einfach mal \"script -start=samples/sample\"", _option) << endl;
    cerr << LineBreak("|-> Die größtmögliche Flexibilität bietet dir mein integrierter Interpreter, der in der Lage ist, komplexe Automatismen, \"NumeRe-Prozeduren\" (*.nprc) genannt, auszuführen. Diese Prozeduren kannst du verwenden, um deine eigenen Unterprogramme in meinem Framework zu entwickeln. Auch wenn die Vielfalt der Funktionen dieser Prozeduren vielleicht noch eingeschränkt erscheinen mag, so wird sie doch stetig erweitert.", _option) << endl;
    cerr << LineBreak("|-> TIPP: Weiterführende Infos findest du unter \"help for\", \"help while\", \"help if\", \"help script\" und \"help procedure\"", _option) << endl;
    cerr << "|   (ENTER ZUM FORTFAHREN / 0+ENTER ZUM ABBRECHEN) ";
    getline(cin, sInput);
    StripSpaces(sInput);
    if (sInput == "0")
    {
        make_hline();
        return;
    }
    else
        sInput = "";
    make_hline();
    cerr << "|-> NUMERE: ERSTER START [FUNKTIONEN -- SEITE 7/7]" << endl;
    make_hline();
    cerr << LineBreak("|-> Außerdem besitze ich einen großen Satz vordefinierter Funktionen, Konstanten und Einheitenumrechnungen. Ich kann sie dir auflisten, wenn du \"list -func\" für die Funktionen, \"list -const\" für die Konstanten und \"list -units\" für die Einheitenumrechnungen eingibst.", _option) << endl;
    cerr << LineBreak("|-> Zusätzlich kannst du auch noch eigene Funktionen definieren, mit denen du dann genauso wie mit meinen vordefinierten umgehen kannst. Dazu musst du mir aber erklären, was deine Funktion können soll. Die dazu nötige Definition machst du dabei durch das Schema \"define FUNKTIONSNAME(ARGUMENTE) := FUNKTIONSAUSDRUCK\", z.B. durch \"define f(x,y) := cos(x)+sin(y)\"", _option) << endl;
    cerr << LineBreak("|-> TIPP: Weiterführende Infos erhältst du unter \"help func\" und \"help define\"", _option) << endl;
    cerr << "|   (ENTER ZUM FORTFAHREN) ";
    getline(cin, sInput);*/
    //make_hline();
    return;
}

void doc_TipOfTheDay(Settings& _option)
{
    vector<string> vTipList = getDBFileContent("<>/docs/hints.ndb", _option);
    unsigned int nth_tip = 0;
    // --> Einen Seed (aus der Zeit generiert) an die rand()-Funktion zuweisen <--
    srand(time(NULL));

    if (!vTipList.size())
        return;
    // --> Die aktuelle Begruessung erhalten wir als modulo(nGreetings)-Operation auf rand() <--
    nth_tip = (rand() % vTipList.size());
    if (nth_tip >= vTipList.size())
        nth_tip = vTipList.size()-1;

    /*cerr << "|" << endl;
    make_hline();*/
    //cerr << "|-> NUMERE: SCHON GEWUSST?  [Nr. "<< nth_tip+1 << "/" << vTipList.size() << "]" << endl;
    cerr << toSystemCodePage("|-> "+_lang.get("DOC_TIPOFTHEDAY_HEADLINE", toString(nth_tip+1), toString((int)vTipList.size()))) << endl;
    make_hline();
    cerr << LineBreak("|-> " + vTipList[nth_tip], _option) << endl;
    //cerr << "|" << endl << LineBreak("|-> Diese Hinweise können mit \"set -hints=false\" deaktiviert werden.", _option) << endl;
    make_hline();

    return;
}

