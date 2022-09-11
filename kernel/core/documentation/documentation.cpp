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

#include <vector>
#include <map>
#include <fstream>
#include "documentation.hpp"
#include "../../kernel.hpp"
#include "../datamanagement/database.hpp"
#include "../ui/error.hpp"
#include "../utils/tools.hpp"
#include "../../syntax.hpp"



/////////////////////////////////////////////////
/// \brief Static helper function for
/// doc_ReplaceExprContentForHTML to determine
/// literal values in expressions.
///
/// \param sExpr const std::string&
/// \param nPos size_t
/// \param nLength size_t
/// \return bool
///
/////////////////////////////////////////////////
static bool isValue(const std::string& sExpr, size_t nPos, size_t nLength)
{
    return (!nPos || !isalpha(sExpr[nPos-1])) && (nPos+nLength == sExpr.length() || !isalpha(sExpr[nPos+nLength]));
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// doc_ReplaceExprContentForHTML to determine
/// operators in expressions.
///
/// \param sExpr const std::string&
/// \param nPos size_t
/// \param nLength size_t
/// \return bool
///
/////////////////////////////////////////////////
static bool isOperator(const std::string& sExpr, size_t nPos, size_t nLength)
{
    return true;
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// doc_ReplaceExprContentForHTML to determine
/// functions in expressions.
///
/// \param sExpr const std::string&
/// \param nPos size_t
/// \param nLength size_t
/// \return bool
///
/////////////////////////////////////////////////
static bool isFunction(const std::string& sExpr, size_t nPos, size_t nLength)
{
    return (!nPos || !isalpha(sExpr[nPos-1])) && sExpr[nPos+nLength] == '(';
}


/////////////////////////////////////////////////
/// \brief This function replaces tokens in
/// <expr>-tags to improve the readability of
/// mathematical code.
///
/// \param sExpr std::string&
/// \param _option Settings&
/// \return void
///
/////////////////////////////////////////////////
static void doc_ReplaceExprContentForHTML(std::string& sExpr, Settings& _option)
{
    // Get the mathstyle data base's contents
    static NumeRe::DataBase HTMLEntitiesDB("<>/docs/mathstyle.ndb");
    size_t nPos = 0;

    // Set the starting position
    if (sExpr.find("<exprblock>") != std::string::npos)
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
            if (sExpr.find("<exprblock>", i+12) != std::string::npos)
            {
                i = sExpr.find("<exprblock>", i+12) + 10;
                continue;
            }

            break;
        }

        // Match the tokens of the data base
        for (size_t n = 0; n < HTMLEntitiesDB.size(); n++)
        {
            if (sExpr.substr(i, HTMLEntitiesDB[n][0].length()) == HTMLEntitiesDB[n][0]
                && ((HTMLEntitiesDB[n][2] == "OP" && isOperator(sExpr, i, HTMLEntitiesDB[n][0].length()))
                    || (HTMLEntitiesDB[n][2] == "VAL" && isValue(sExpr, i, HTMLEntitiesDB[n][0].length()))
                    || (HTMLEntitiesDB[n][2] == "FCT" && isFunction(sExpr, i, HTMLEntitiesDB[n][0].length())))
                )
            {
                sExpr.replace(i, HTMLEntitiesDB[n][0].length(), HTMLEntitiesDB[n][1]);
                i += HTMLEntitiesDB[n][1].length()-1;
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


/////////////////////////////////////////////////
/// \brief Searches for defined XML tokens in the
/// passed string and replaces them with the
/// plain HTML counterpart.
///
/// \param sDocParagraph std::string&
/// \param generateFile bool
/// \param _option Settings&
/// \return void
///
/////////////////////////////////////////////////
static void doc_ReplaceTokensForHTML(std::string& sDocParagraph, bool generateFile, Settings& _option)
{
    for (unsigned int k = 0; k < sDocParagraph.length(); k++)
    {
        if (sDocParagraph.substr(k,2) == "\\$")
            sDocParagraph.erase(k,1);

        if (sDocParagraph.substr(k,3) == "\\\\n")
            sDocParagraph.erase(k,1);

        if (sDocParagraph.substr(k,2) == "  ")
            sDocParagraph.replace(k,1,"&nbsp;");

        if (sDocParagraph.substr(k,4) == "<em>" && sDocParagraph.find("</em>", k+4) != std::string::npos)
        {
            sDocParagraph.insert(k+4, "<strong>");
            sDocParagraph.insert(sDocParagraph.find("</em>", k+12), "</strong>");
        }

        if (sDocParagraph.substr(k,3) == "<h>" && sDocParagraph.find("</h>", k+3) != std::string::npos)
        {
            sDocParagraph.replace(k, 3, "<h4>");
            sDocParagraph.replace(sDocParagraph.find("</h>",k+4), 4, "</h4>");
        }

        if (sDocParagraph.substr(k,6) == "<expr>" && sDocParagraph.find("</expr>", k+6) != std::string::npos)
        {
            std::string sExpr = sDocParagraph.substr(k+6, sDocParagraph.find("</expr>", k+6)-k-6);
            doc_ReplaceExprContentForHTML(sExpr,_option);
            sDocParagraph.replace(k,
                                  sDocParagraph.find("</expr>",k+6)+7-k,
                                  "<span style=\"font-style:italic; font-family: palatino linotype; font-weight: bold;\">"+sExpr+"</span>");
        }

        if (sDocParagraph.substr(k,6) == "<code>" && sDocParagraph.find("</code>", k+6) != std::string::npos)
        {
            sDocParagraph.insert(k+6, "<span style=\"color:#00008B;background-color:#F2F2F2;\">");
            sDocParagraph.insert(sDocParagraph.find("</code>", k+6), "</span>");
            std::string sCode = sDocParagraph.substr(k+6, sDocParagraph.find("</code>", k+6)-k-6);

            for (unsigned int i = 0; i < sCode.length(); i++)
            {
                if (sCode.substr(i,2) == "\\n")
                    sCode.replace(i,2,"<br>");
            }

            k += sCode.length();
        }

        if (sDocParagraph.substr(k,5) == "<img " && sDocParagraph.find("/>", k+5) != std::string::npos)
        {
            std::string sImg = sDocParagraph.substr(k, sDocParagraph.find("/>", k+5)+2-k);

            if (sImg.find("src") != std::string::npos)
            {
                std::string sImgSrc = Documentation::getArgAtPos(sImg, sImg.find('=', sImg.find("src"))+1);
                sImgSrc = NumeReKernel::getInstance()->getFileSystem().ValidFileName(sImgSrc, ".png");
                sImg = "<img src=\"" + sImgSrc + "\" />";
                sImg = "<div align=\"center\">" + sImg + "</div>";
            }
            else
                sImg.clear();

            sDocParagraph.replace(k, sDocParagraph.find("/>", k+5)+2-k, sImg);
            k += sImg.length();
        }

        if (sDocParagraph.substr(k, 10) == "&PLOTPATH&")
            sDocParagraph.replace(k, 10, generateFile ? "&lt;plotpath&gt;" : replacePathSeparator(_option.getPlotPath()));

        if (sDocParagraph.substr(k, 10) == "&LOADPATH&")
            sDocParagraph.replace(k, 10, generateFile ? "&lt;loadpath&gt;" : replacePathSeparator(_option.getLoadPath()));

        if (sDocParagraph.substr(k, 10) == "&SAVEPATH&")
            sDocParagraph.replace(k, 10, generateFile ? "&lt;savepath&gt;" : replacePathSeparator(_option.getSavePath()));

        if (sDocParagraph.substr(k, 10) == "&PROCPATH&")
            sDocParagraph.replace(k, 10, generateFile ? "&lt;procpath&gt;" : replacePathSeparator(_option.getProcPath()));

        if (sDocParagraph.substr(k, 12) == "&SCRIPTPATH&")
            sDocParagraph.replace(k, 12, generateFile ? "&lt;scriptpath&gt;" : replacePathSeparator(_option.getScriptPath()));

        if (sDocParagraph.substr(k, 9) == "&EXEPATH&")
            sDocParagraph.replace(k, 9, generateFile ? "&lt;&gt;" : replacePathSeparator(_option.getExePath()));
    }
}


/////////////////////////////////////////////////
/// \brief This function shows the content of a
/// documentation article based upon the passed
/// topic. The content is displayed in terminal
/// or in an external window (depending on the
/// settings) or directly written to an HTML file
/// (depending on an additional parameter).
///
/// \param __sTopic const std::string&
/// \param _option Settings&
/// \return void
///
/////////////////////////////////////////////////
void doc_Help(const std::string& __sTopic, Settings& _option)
{
    std::string sTopic = toLowerCase(__sTopic);

    if (findParameter(sTopic, "html"))
        eraseToken(sTopic, "html", false);

    // --> Zunaechst einmal muessen wir anfuehrende oder abschliessende Leerzeichen entfernen <--
    StripSpaces(sTopic);

    if (!sTopic.length())
        sTopic = "brief";

    if (sTopic.front() == '-')
        sTopic.erase(0,1);

    StripSpaces(sTopic);

    std::vector<std::string> vDocArticle = _option.getHelpArticle(sTopic);

    if (vDocArticle[0] == "NO_ENTRY_FOUND") // Nix gefunden
    {
        make_hline();
        NumeReKernel::print(LineBreak(_lang.get("DOC_HELP_NO_ENTRY_FOUND", sTopic), _option));
        make_hline();
    }
    else //if (findParameter(__sTopic, "html") || _option.useExternalDocWindow()) // HTML-Export generieren
    {
        bool generateFile = (bool)findParameter(__sTopic, "html");
        std::string sHTML = doc_HelpAsHTML(sTopic, generateFile, _option);

        if (generateFile)
        {
            _option.declareFileType(".html");
            std::string sFilename = _option.ValidizeAndPrepareName("<>/docs/htmlexport/"+_option.getHelpArticleID(sTopic) + ".html",".html");
            std::ofstream fHTML;
            fHTML.open(sFilename);

            if (fHTML.fail())
                throw SyntaxError(SyntaxError::CANNOT_GENERATE_FILE, "", SyntaxError::invalid_position, sFilename);

            // content schreiben
            fHTML << sHTML;
            NumeReKernel::print(_lang.get("DOC_HELP_HTMLEXPORT", _option.getHelpArticleTitle(_option.getHelpIdxKey(sTopic)), sFilename));
        }
        else
            NumeReKernel::setDocumentation(sHTML);
    }
    /*else // Hilfeartikel anzeigen
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

            if (vDocArticle[i].find("<example ") != std::string::npos) // Beispiel-Tags
            {
                bool bVerb = false;

                if (vDocArticle[i].find("type=") && Documentation::getArgAtPos(vDocArticle[i], vDocArticle[i].find("type=")+5) == "verbatim")
                    bVerb = true;

                doc_ReplaceTokens(vDocArticle[i], _option);
                NumeReKernel::print(_lang.get("DOC_HELP_EXAMPLE", Documentation::getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5)));
                NumeReKernel::printPreFmt("|\n");

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != std::string::npos)
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
                           NumeReKernel::printPreFmt("||<- " + LineBreak(vDocArticle[j], _option, false, 5) + "\n");
                        else
                        {
                            NumeReKernel::printPreFmt("||-> " + LineBreak(vDocArticle[j], _option, false, 5) + "\n");

                            if (vDocArticle[j+1].find("</example>") == std::string::npos)
                                NumeReKernel::printPreFmt("||\n");
                        }
                    }
                    else
                        NumeReKernel::printPreFmt("|" + toSystemCodePage(vDocArticle[j])+"\n");
                }
            }
            else if (vDocArticle[i].find("<exprblock>") != std::string::npos) // EXPRBLOCK-Tags
            {
                if (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != std::string::npos)
                {
                    doc_ReplaceTokens(vDocArticle[i], _option);

                    while (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != std::string::npos)
                    {
                        std::string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<exprblock>")+11, vDocArticle[i].find("</exprblock>")-vDocArticle[i].find("<exprblock>")-11);

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

                    NumeReKernel::print(vDocArticle[i]);
                }
                else
                {
                    if (vDocArticle[i] != "<exprblock>")
                        NumeReKernel::print(vDocArticle[i].substr(0,vDocArticle[i].find("<exprblock>")));

                    NumeReKernel::printPreFmt("|\n");

                    for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                    {
                        if (vDocArticle[j].find("</exprblock>") != std::string::npos)
                        {
                            i = j;

                            if (i+1 < vDocArticle.size())
                                NumeReKernel::printPreFmt("|\n");

                            break;
                        }

                        doc_ReplaceTokens(vDocArticle[j], _option);

                        while (vDocArticle[j].find("\\t") != std::string::npos)
                            vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "    ");

                        NumeReKernel::printPreFmt("|     " + toSystemCodePage(vDocArticle[j]) + "\n");
                    }
                }
            }
            else if (vDocArticle[i].find("<codeblock>") != std::string::npos) // CODEBLOCK-Tags
            {
                if (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != std::string::npos)
                {
                    doc_ReplaceTokens(vDocArticle[i], _option);

                    while (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != std::string::npos)
                    {
                        std::string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<codeblock>")+11, vDocArticle[i].find("</codeblock>")-vDocArticle[i].find("<codeblock>")-11);

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

                    NumeReKernel::print(vDocArticle[i]);
                }
                else
                {
                    if (vDocArticle[i] != "<codeblock>")
                        NumeReKernel::print(vDocArticle[i].substr(0,vDocArticle[i].find("<codeblock>")));

                    NumeReKernel::printPreFmt("|\n");

                    for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                    {
                        if (vDocArticle[j].find("</codeblock>") != std::string::npos)
                        {
                            i = j;

                            if (i+1 < vDocArticle.size())
                                NumeReKernel::printPreFmt("|\n");

                            break;
                        }

                        doc_ReplaceTokens(vDocArticle[j], _option);

                        while (vDocArticle[j].find("\\t") != std::string::npos)
                            vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "    ");

                        NumeReKernel::printPreFmt("|     " + toSystemCodePage(vDocArticle[j])+"\n");
                    }
                }
            }
            else if (vDocArticle[i].find("<list>") != std::string::npos) // Standard-LIST-Tags
            {
                unsigned int nLengthMax = 0;

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</list>") != std::string::npos)
                    {
                        std::string sLine = "";
                        std::string sNode = "";
                        std::string sRemainingLine = "";
                        std::string sFinalLine = "";
                        int nIndent = 0;

                        for (unsigned int k = i+1; k < j; k++)
                        {
                            nIndent = 0;
                            sNode = Documentation::getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5);
                            sRemainingLine = vDocArticle[k].substr(vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+Documentation::getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2)+1, vDocArticle[k].find("</item>")-1-vDocArticle[k].find('>', vDocArticle[k].find("node=")+5+Documentation::getArgAtPos(vDocArticle[k], vDocArticle[k].find("node=")+5).length()+2));
                            sFinalLine = "";

                            if (vDocArticle[k].find("type=") != std::string::npos && Documentation::getArgAtPos(vDocArticle[k], vDocArticle[k].find("type=")+5) == "verbatim")
                            {
                                NumeReKernel::printPreFmt("|     " + sNode);
                                nIndent = sNode.length()+6;
                                sLine.append(nLengthMax+9-nIndent, ' ');
                                sLine += "- " + sRemainingLine;

                                if (sLine.find('~') == std::string::npos && sLine.find('%') == std::string::npos)
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
                                            sLine += "- " + sRemainingLine;
                                        else
                                            sLine += "  " + sRemainingLine;

                                        if (sLine.find('~') == std::string::npos && sLine.find('%') == std::string::npos)
                                            sLine = LineBreak(sLine, _option, true, nIndent, nLengthMax+11);
                                        else
                                            sLine = LineBreak(sLine, _option, false, nIndent, nLengthMax+11);

                                        sFinalLine += sLine.substr(0,sLine.find('\n'));

                                        if (sLine.find('\n') != std::string::npos)
                                            sFinalLine += '\n';

                                        sRemainingLine.erase(0,sLine.substr(nLengthMax+11, sLine.find('\n')-nLengthMax-11).length());

                                        if (sRemainingLine.front() == ' ')
                                            sRemainingLine.erase(0,1);
                                    }
                                }

                                sLine = "|     " + sNode;
                                sLine.append(nLengthMax+9-sLine.length()+countEscapeSymbols(sLine), ' ');

                                if (!sFinalLine.length())
                                    sLine += "- " + sRemainingLine;
                                else
                                    sLine += "  " + sRemainingLine;

                                if (sLine.find('~') == std::string::npos && sLine.find('%') == std::string::npos)
                                    sFinalLine += LineBreak(sLine, _option, true, nIndent, nLengthMax+11);
                                else
                                    sFinalLine += LineBreak(sLine, _option, false, nIndent, nLengthMax+11);

                                NumeReKernel::printPreFmt(sFinalLine + "\n");
                            }
                        }

                        i = j;
                        break;
                    }
                    else
                    {
                        doc_ReplaceTokens(vDocArticle[j], _option);

                        std::string sNode = Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5);

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
            else if (vDocArticle[i].find("<table") != std::string::npos) // TABLE-Tags
            {
                std::string sTable = vDocArticle[i].substr(vDocArticle[i].find("<table"));

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</table>") != std::string::npos)
                    {
                        sTable += vDocArticle[j].substr(0,vDocArticle[j].find("</table>")+8);
                        // Send the whole content to the table reader and render the obtained table on the screen.
                        std::vector<std::vector<std::string> > vTable = doc_readTokenTable(sTable, _option);
                        std::vector<size_t> vFieldSizes;

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
                        sTable += vDocArticle[j];
                }
            }
            else // Normaler Paragraph
            {
                doc_ReplaceTokens(vDocArticle[i], _option);

                if (vDocArticle[i].find('~') == std::string::npos && vDocArticle[i].find('%') == std::string::npos)
                    NumeReKernel::print(LineBreak(vDocArticle[i], _option));
                else
                    NumeReKernel::print(LineBreak(vDocArticle[i], _option, false));
            }
        }

        NumeReKernel::toggleTableStatus();
        make_hline();
    }*/
}


/////////////////////////////////////////////////
/// \brief This static function creates the CSS
/// string for the span element containing the
/// lexed symbol. The passed style is mapped to
/// the settings, so that the syntax colours are
/// somewhat similar to the ones selected by the
/// user.
///
/// \param style int
/// \param _option const Settings&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createCssString(int style, const Settings& _option)
{
    std::string sSettingsString;

    switch (style)
    {
    //case NumeReSyntax::SYNTAX_STD:
    //    sSettingsString = _option.getSetting(SETTING_S_ST_STANDARD).stringval();
    //    break;
    case NumeReSyntax::SYNTAX_COMMAND:
        sSettingsString = _option.getSetting(SETTING_S_ST_COMMAND).stringval();
        break;
    case NumeReSyntax::SYNTAX_OPERATOR:
        sSettingsString = _option.getSetting(SETTING_S_ST_OPERATOR).stringval();
        break;
    case NumeReSyntax::SYNTAX_SPECIALVAL:
        sSettingsString = _option.getSetting(SETTING_S_ST_SPECIALVAL).stringval();
        break;
    case NumeReSyntax::SYNTAX_NUMBER:
        sSettingsString = _option.getSetting(SETTING_S_ST_NUMBER).stringval();
        break;
    case NumeReSyntax::SYNTAX_OPTION:
        sSettingsString = _option.getSetting(SETTING_S_ST_OPTION).stringval();
        break;
    case NumeReSyntax::SYNTAX_PROCEDURE:
        sSettingsString = _option.getSetting(SETTING_S_ST_PROCEDURE).stringval();
        break;
    case NumeReSyntax::SYNTAX_STRING:
        sSettingsString = _option.getSetting(SETTING_S_ST_STRING).stringval();
        break;
    case NumeReSyntax::SYNTAX_METHODS:
        sSettingsString = _option.getSetting(SETTING_S_ST_METHODS).stringval();
        break;
    case NumeReSyntax::SYNTAX_FUNCTION:
        sSettingsString = _option.getSetting(SETTING_S_ST_FUNCTION).stringval();
        break;
    case NumeReSyntax::SYNTAX_CONSTANT:
        sSettingsString = _option.getSetting(SETTING_S_ST_CONSTANT).stringval();
        break;
    case NumeReSyntax::SYNTAX_COMMENT:
        sSettingsString = _option.getSetting(SETTING_S_ST_COMMENT).stringval();
        break;
    default:
        return "";
    }

    replaceAll(sSettingsString, ":", ",");
    std::string sCssString = "color: rgb(" + sSettingsString.substr(0, sSettingsString.find('-')) + ");";
    std::string sBackgroundColor = sSettingsString.substr(sSettingsString.find('-')+1,
                                                           sSettingsString.rfind('-')-sSettingsString.find('-')-1);

    sSettingsString.erase(0, sSettingsString.rfind('-')+1);

    if (sSettingsString[0] == '1') // bold
        sCssString += " font-weight: bold;";

    if (sSettingsString[1] == '1') // italic
        sCssString += " font-style: italic;";

    if (sSettingsString[2] == '1') // underline
        sCssString += " text-decoration: underline;";

    if (sSettingsString[3] == '0') // custom background
        sCssString += " background-color: rgb(" + sBackgroundColor + ");";

    return sCssString;
}


/////////////////////////////////////////////////
/// \brief This static function lexes the passed
/// code string usign a static instance of the
/// NumeReSyntax class.
///
/// \param sCodeString const std::string&
/// \param _option const Settings&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string applySyntaxHighlighting(const std::string& sCodeString, const Settings& _option)
{
    static NumeReSyntax _syntax(_option.getExePath() + "/", NumeReKernel::getInstance()->getPluginCommands());
    std::string sStyleBytes;
    std::string sStyledString;

    if (sCodeString.front() != '|')
        sStyleBytes = _syntax.highlightLine("|<- " + sCodeString + " ").substr(4);
    else
        sStyleBytes = _syntax.highlightLine(sCodeString + " ");

    size_t nLastPos = sStyleBytes.length();
    size_t nLastStatePosition = 0;

    for (size_t i = 0; i < nLastPos; i++)
    {
        if (sStyleBytes[i] != sStyleBytes[nLastStatePosition] || i+1 == nLastPos)
        {
            int style = sStyleBytes[nLastStatePosition] - '0';
            std::string textRange;

            if (i+1 == nLastPos)
                textRange = sCodeString.substr(nLastStatePosition, i+1-nLastStatePosition);
            else
                textRange = sCodeString.substr(nLastStatePosition, i-nLastStatePosition);

            if (textRange.find_first_not_of(" \r\t\n") == std::string::npos)
            {
                sStyledString += textRange;
                nLastStatePosition = i;
                continue;
            }

            if (style == NumeReSyntax::SYNTAX_OPERATOR)
            {
                replaceAll(textRange, "<", "&lt;");
                replaceAll(textRange, ">", "&gt;");
            }

            std::string sCssString = createCssString(style, _option);

            if (sCssString.length())
                sStyledString += "<span style=\"" + sCssString + "\">" + textRange + "</span>";
            else
                sStyledString += textRange;

            nLastStatePosition = i;
        }
    }


    return sStyledString;
}


/////////////////////////////////////////////////
/// \brief Returns the final HTML string
/// containing the already lexed and highlighted
/// code.
///
/// \param sCode std::string
/// \param verbatim bool
/// \param _option Settings&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getHighlightedCode(std::string sCode, bool verbatim, Settings& _option)
{
    if (!verbatim)
    {
        replaceAll(sCode, "\\n", " \\n ");
        replaceAll(sCode, "\\t", "\\t ");
        replaceAll(sCode, "&lt;", "<");
        replaceAll(sCode, "&gt;", ">");
        sCode = applySyntaxHighlighting(sCode, _option);
        replaceAll(sCode, "\\t ", "&nbsp;&nbsp;&nbsp;&nbsp;");

        if (sCode.substr(0, 4) == "|<- ")
            sCode.replace(1, 1, "&lt;");

        if (sCode.substr(0, 4) == "|-> ")
            sCode.replace(2, 1, "&gt;");
    }
    else
        replaceAll(sCode, "\\t", "&nbsp;&nbsp;&nbsp;&nbsp;");

    replaceAll(sCode, "\\n", "<br>\n");
    return sCode;
}


#define FILE_CODEBLOCK_START "<div class=\"sites-codeblock sites-codesnippet-block\"><CODE><span style=\"color:#00008B;\">\n"
#define FILE_CODEBLOCK_END "</span></CODE></div>\n"

#define VIEWER_CODEBLOCK_START "<center><table border=\"0\" cellspacing=\"0\" bgcolor=\"#F2F2F2\" width=\"94%\">\n<tbody><tr><td>\n<CODE><span style=\"color:#00008B;\">\n"
#define VIEWER_CODEBLOCK_END "\n</span></CODE></td></tr></tbody></table></center>\n"


/////////////////////////////////////////////////
/// \brief Returns the final HTML string
/// containing the lexed and highlighted code and
/// embeds that into the code block environment.
///
/// \param sCode std::string
/// \param generateFile bool
/// \param verbatim bool
/// \param _option Settings&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string formatCodeBlock(std::string sCode, bool generateFile, bool verbatim, Settings& _option)
{
    sCode = getHighlightedCode(sCode, verbatim, _option);
    doc_ReplaceTokensForHTML(sCode, generateFile, _option);

    if (generateFile)
        return FILE_CODEBLOCK_START + sCode + FILE_CODEBLOCK_END;

    return VIEWER_CODEBLOCK_START + sCode + VIEWER_CODEBLOCK_END;
}

/////////////////////////////////////////////////
/// \brief This function returns the
/// documentation article for the selected topic
/// as an HTML std::string. This std::string
/// either may be used to create a corresponding
/// file or it may be displayed in the
/// documentation viewer.
///
/// \param __sTopic const std::string&
/// \param generateFile bool
/// \param _option Settings&
/// \return std::string
///
/////////////////////////////////////////////////
std::string doc_HelpAsHTML(const std::string& __sTopic, bool generateFile, Settings& _option)
{
    std::string sTopic = __sTopic;
    StripSpaces(sTopic);

    // Get the article contents
    std::vector<std::string> vDocArticle = _option.getHelpArticle(sTopic);

    if (vDocArticle[0] == "NO_ENTRY_FOUND") // Nix gefunden
        return "";

    bool isIndex = (vDocArticle[0] == "Index");

    std::string sHTML;

    sHTML = "<!DOCTYPE html>\n<html>\n<head>\n";

    // Convert the XML-like structure of the documentation
    // article into a valid HTML DOM, which can be returned
    // as a single std::string
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
        if (vDocArticle[i].find("<example ") != std::string::npos) // Beispiel-Tags
        {
            sHTML += "<h4>"+ _lang.get("DOC_HELP_EXAMPLE_HEADLINE") +"</h4>\n";
            bool bVerb = false;
            bool bCodeBlock = false;
            bool bPlain = false;

            if (vDocArticle[i].find("type=") && Documentation::getArgAtPos(vDocArticle[i], vDocArticle[i].find("type=")+5) == "verbatim")
                bVerb = true;

            if (vDocArticle[i].find("type=") && Documentation::getArgAtPos(vDocArticle[i], vDocArticle[i].find("type=")+5) == "codeblock")
                bCodeBlock = true;

            if (vDocArticle[i].find("type=") && Documentation::getArgAtPos(vDocArticle[i], vDocArticle[i].find("type=")+5) == "plain")
                bPlain = true;

            std::string sDescription = Documentation::getArgAtPos(vDocArticle[i], vDocArticle[i].find("desc=")+5);

            doc_ReplaceTokensForHTML(sDescription, generateFile, _option);

            if (generateFile)
                sHTML += "<p>" + sDescription + "</p>\n";
            else
                sHTML += "<p>" + sDescription + "</p>\n";

            if (bCodeBlock || bPlain)
            {
                std::string sCodeContent;

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != std::string::npos)
                    {
                        i = j;
                        sHTML += formatCodeBlock(sCodeContent.substr(0, sCodeContent.length()-2), generateFile, bPlain, _option) + "\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\\n";
                }
            }
            else
            {
                std::string sExample;

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != std::string::npos)
                    {
                        i = j;
                        doc_ReplaceTokensForHTML(sExample, generateFile, _option);

                        if (generateFile)
                            sHTML += FILE_CODEBLOCK_START + sExample.substr(0, sExample.length()-5) + FILE_CODEBLOCK_END;
                        else
                            sHTML += VIEWER_CODEBLOCK_START + sExample.substr(0, sExample.length()-5) + VIEWER_CODEBLOCK_END;

                        break;
                    }

                    if (vDocArticle[j] == "[...]")
                    {
                        sExample += "[...]<br>\n";
                        continue;
                    }

                    if (!bVerb)
                    {
                        if (((i+1) % 2 && j % 2) || (!((i+1) % 2) && !(j % 2)))
                            sExample += "|&lt;- " + getHighlightedCode(vDocArticle[j], false, _option) + "<br>\n";
                        else
                        {
                            sExample += "|-&gt; " + vDocArticle[j] + "<br>\n";

                            if (vDocArticle[j+1].find("</example>") == std::string::npos)
                                sExample += "|<br>\n";
                        }
                    }
                    else
                        sExample += getHighlightedCode(vDocArticle[j], false, _option) + "<br>\n";
                }
            }

        }
        else if (vDocArticle[i].find("<exprblock>") != std::string::npos) // EXPRBLOCK-Tags
        {
            if (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != std::string::npos)
            {
                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
                doc_ReplaceExprContentForHTML(vDocArticle[i], _option);

                while (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<exprblock>")+11, vDocArticle[i].find("</exprblock>")-vDocArticle[i].find("<exprblock>")-11);

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
                    if (vDocArticle[j].find("</exprblock>") != std::string::npos)
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

                    while (vDocArticle[j].find("\\t") != std::string::npos)
                        vDocArticle[j].replace(vDocArticle[j].find("\\t"), 2, "&nbsp;&nbsp;&nbsp;&nbsp;");

                    sHTML += (vDocArticle[j]) + "<br>\n";
                }
            }
        }
        else if (vDocArticle[i].find("<codeblock>") != std::string::npos) // CODEBLOCK-Tags
        {
            if (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != std::string::npos)
            {
                while (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<codeblock>")+11,
                                                                   vDocArticle[i].find("</codeblock>")-vDocArticle[i].find("<codeblock>")-11);

                    vDocArticle[i].replace(vDocArticle[i].find("<codeblock>"),
                                           vDocArticle[i].find("</codeblock>")+12-vDocArticle[i].find("<codeblock>"),
                                           "</p>" + formatCodeBlock(sExprBlock, generateFile, false, _option) + "<p>");
                }

                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
                sHTML += "<p>" + (vDocArticle[i]) + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<codeblock>")
                    sHTML += "<p>" + (vDocArticle[i].substr(0, vDocArticle[i].find("<codeblock>"))) + "</p>\n";

                std::string sCodeContent;

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</codeblock>") != std::string::npos)
                    {
                        i = j;
                        sHTML += formatCodeBlock(sCodeContent.substr(0, sCodeContent.length()-2), generateFile, false, _option) + "\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\\n";
                }
            }
        }
        else if (vDocArticle[i].find("<verbatim>") != std::string::npos) // CODEBLOCK-Tags
        {
            if (vDocArticle[i].find("</verbatim>", vDocArticle[i].find("<verbatim>")) != std::string::npos)
            {
                while (vDocArticle[i].find("</verbatim>", vDocArticle[i].find("<verbatim>")) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<verbatim>")+10,
                                                                   vDocArticle[i].find("</verbatim>")-vDocArticle[i].find("<verbatim>")-10);

                    vDocArticle[i].replace(vDocArticle[i].find("<verbatim>"),
                                           vDocArticle[i].find("</verbatim>")+11-vDocArticle[i].find("<verbatim>"),
                                           "</p>" + formatCodeBlock(sExprBlock, generateFile, true, _option) + "<p>");
                }

                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
                sHTML += "<p>" + (vDocArticle[i]) + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<verbatim>")
                    sHTML += "<p>" + (vDocArticle[i].substr(0, vDocArticle[i].find("<verbatim>"))) + "</p>\n";

                std::string sCodeContent;

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</verbatim>") != std::string::npos)
                    {
                        i = j;
                        sHTML += formatCodeBlock(sCodeContent.substr(0, sCodeContent.length()-2), generateFile, true, _option) + "\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\\n";
                }
            }
        }
        else if (vDocArticle[i].find("<syntax>") != std::string::npos) // CODEBLOCK-Tags
        {
            if (vDocArticle[i].find("</syntax>", vDocArticle[i].find("<syntax>")) != std::string::npos)
            {
                while (vDocArticle[i].find("</syntax>", vDocArticle[i].find("<syntax>")) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(vDocArticle[i].find("<syntax>")+8,
                                                                   vDocArticle[i].find("</syntax>")-vDocArticle[i].find("<syntax>")-8);

                    vDocArticle[i].replace(vDocArticle[i].find("<syntax>"),
                                           vDocArticle[i].find("</syntax>")+9-vDocArticle[i].find("<syntax>"),
                                           "</p><h4>Syntax</h4>" + formatCodeBlock(sExprBlock, generateFile, false, _option)
                                                + "<h4>" + _lang.get("DOC_HELP_DESC_HEADLINE") + "</h4><p>");
                }

                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
                sHTML += "<p>" + vDocArticle[i] + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<syntax>")
                    sHTML += "<p>" + (vDocArticle[i].substr(0, vDocArticle[i].find("<syntax>"))) + "</p>\n";

                sHTML += "<h4>Syntax</h4>\n";
                std::string sCodeContent;

                for (unsigned int j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</syntax>") != std::string::npos)
                    {
                        i = j;
                        sHTML += formatCodeBlock(sCodeContent.substr(0, sCodeContent.length()-2), generateFile, false, _option)
                            + "<h4>" + _lang.get("DOC_HELP_DESC_HEADLINE") + "</h4>\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\\n";
                }
            }
        }
        else if (vDocArticle[i].find("<list") != std::string::npos) // Alle LIST-Tags (umgewandelt zu TABLE)
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
                if (vDocArticle[j].find("</list>") != std::string::npos)
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
                             + (Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5))
                             + "</span></code></td>\n"
                             + "      <td style=\"width:400px;height:19px\">"
                             + (vDocArticle[j].substr(vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)+1, vDocArticle[j].find("</item>")-1-vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)))
                             + "</td>\n";
                        sHTML += "    </tr>\n";
                    }
                    else
                    {
                        if (isIndex)
                        {
                            sHTML += "    <tr>\n      <td width=\"200\"><a href=\"nhlp://"+Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5)+"?frame=self\"><code><span style=\"color:#00008B;\">"
                                  + Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5)
                                  + "</span></code></a></td>\n      <td>"
                                  + vDocArticle[j].substr(vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)+1, vDocArticle[j].find("</item>")-1-vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2))
                                  + "</td>\n    </tr>\n";
                        }
                        else
                        {
                            sHTML += "    <tr>\n      <td width=\"200\"><code><span style=\"color:#00008B;\">"
                                  + Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5)
                                  + "</span></code></td>\n      <td>"
                                  + vDocArticle[j].substr(vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2)+1, vDocArticle[j].find("</item>")-1-vDocArticle[j].find('>', vDocArticle[j].find("node=")+5+Documentation::getArgAtPos(vDocArticle[j], vDocArticle[j].find("node=")+5).length()+2))
                                  + "</td>\n    </tr>\n";
                        }
                    }
                }
            }
        }
        else if (vDocArticle[i].find("<table") != std::string::npos) // Table-Tags
        {
            if (generateFile)
                sHTML += "<div align=\"center\"><table style=\"border-collapse:collapse; border-color:rgb(136,136,136);border-width:1px\" border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";
            else
                sHTML += "<div align=\"center\"><table border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";

            for (unsigned int j = i+1; j < vDocArticle.size(); j++)
            {
                if (vDocArticle[j].find("</table>") != std::string::npos)
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


/////////////////////////////////////////////////
/// \brief This function provides the logic for
/// searching for entries in the keywords
/// database.
///
/// \param sToLookFor const std::string&
/// \param _option Settings&
/// \return void
///
/////////////////////////////////////////////////
void doc_SearchFct(const std::string& sToLookFor, Settings& _option)
{
    static NumeRe::DataBase findDataBase;
    static std::vector<double> vWeighting({3.0, 2.0, 1.0});

    // Load the database if not done already
    if (!findDataBase.size())
    {
        findDataBase.addData("<>/docs/find.ndb");

        if (_option.useCustomLangFiles() && fileExists(_option.ValidFileName("<>/user/docs/find.ndb", ".ndb")))
            findDataBase.addData("<>/user/docs/find.ndb");
    }

    // Search for matches in the database
    std::map<double,std::vector<size_t>> mMatches = findDataBase.findRecordsUsingRelevance(sToLookFor, vWeighting);

    // If nothig has been found, report that to the user
    if (!mMatches.size())
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        NumeReKernel::print(_lang.get("DOC_SEARCHFCT_NO_RESULTS", sToLookFor));
        NumeReKernel::toggleTableStatus();
        make_hline();
        return;
    }

    double dMax = mMatches.rbegin()->first;
    size_t nCount = 0;

    // Format the search results accordingly
    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print(toSystemCodePage(toUpperCase(_lang.get("DOC_SEARCHFCT_TABLEHEAD"))));
    make_hline();

    for (auto iter = mMatches.rbegin(); iter != mMatches.rend(); ++iter)
    {
        for (size_t j = 0; j < iter->second.size(); j++)
        {
            NumeReKernel::printPreFmt("|->    [");

            if (intCast(iter->first / dMax * 100) != 100)
                NumeReKernel::printPreFmt(" ");

            NumeReKernel::printPreFmt(toString(intCast(iter->first / dMax * 100)) + "%]   ");

            if (findDataBase.getElement(iter->second[j], 0) == "NumeRe v $$$")
                NumeReKernel::printPreFmt("NumeRe v " + sVersion);
            else
                NumeReKernel::printPreFmt(toSystemCodePage(findDataBase.getElement(iter->second[j], 0)));

            NumeReKernel::printPreFmt(" -- ");

            if (findDataBase.getElement(iter->second[j], 0) == "NumeRe v $$$")
                NumeReKernel::printPreFmt(findDataBase.getElement(iter->second[j], 1).substr(0, findDataBase.getElement(iter->second[j], 1).find("$$$")) + replacePathSeparator(_option.getExePath()) + "\n");
            else
                NumeReKernel::printPreFmt(findDataBase.getElement(iter->second[j], 1) + "\n");

            nCount++;
        }
    }

    NumeReKernel::printPreFmt("|\n");
    NumeReKernel::print(toSystemCodePage(_lang.get("DOC_SEARCHFCT_RESULT", toString(nCount))));
    NumeReKernel::toggleTableStatus();
    make_hline();
}

