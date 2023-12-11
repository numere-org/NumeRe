/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2023  Erik Haenel et al.

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

#include "texrendering.hpp"
#include "../datamanagement/database.hpp"
#include "../structures.hpp"
#include "../utils/tools.hpp"
#include "../../kernel.hpp"


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
/// \return void
///
/////////////////////////////////////////////////
static void doc_ReplaceExprContentForTeX(std::string& sExpr)
{
    // Get the mathstyle data base's contents
    static NumeRe::DataBase EntitiesDB("<>/docs/mathstyle.ndb");
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
        for (size_t n = 0; n < EntitiesDB.size(); n++)
        {
            if (sExpr.substr(i, EntitiesDB[n][0].length()) == EntitiesDB[n][0]
                && ((EntitiesDB[n][1] == "OP" && isOperator(sExpr, i, EntitiesDB[n][0].length()))
                    || (EntitiesDB[n][1] == "VAL" && isValue(sExpr, i, EntitiesDB[n][0].length()))
                    || (EntitiesDB[n][1] == "FCT" && isFunction(sExpr, i, EntitiesDB[n][0].length())))
                )
            {
                sExpr.replace(i, EntitiesDB[n][0].length(), EntitiesDB[n][3]);
                i += EntitiesDB[n][3].length()-1;
            }
        }

        // Handle supscripts
        if (sExpr[i] == '^')
        {
            if (sExpr[i+1] == '(')
            {
                sExpr[getMatchingParenthesis(StringView(sExpr, i))+i] = '}';
                sExpr[i+1] = '{';
            }

            continue;
        }

        // Handle subscripts
        if (sExpr[i] == '_')
        {
            if (sExpr[i+1] == '(')
            {
                sExpr[getMatchingParenthesis(StringView(sExpr, i))+i] = '}';
                sExpr[i+1] = '{';
            }

            continue;
        }

        // Special case: autodetect numerical
        // subscripts
        if (i < sExpr.length()-1 && isdigit(sExpr[i+1]) && isalpha(sExpr[i]))
        {
            if (i < sExpr.length()-2)
                sExpr.insert(i+2, "}");
            else
                sExpr.append("}");

            sExpr.insert(i+1,"_{");
            i += 2;
        }
    }
}


/////////////////////////////////////////////////
/// \brief Searches for defined XML tokens in the
/// passed string and replaces them with the
/// plain HTML counterpart.
///
/// \param sDocParagraph std::string&
/// \return void
///
/////////////////////////////////////////////////
static void doc_ReplaceTokensForTeX(std::string& sDocParagraph)
{
    for (size_t k = 0; k < sDocParagraph.length(); k++)
    {
        //if (sDocParagraph.substr(k,3) == "\\\\n")
        //    sDocParagraph.erase(k,1);

        //if (sDocParagraph.substr(k,2) == "  ")
        //    sDocParagraph.replace(k,1,"&nbsp;");

        if (sDocParagraph.substr(k,4) == "<em>" && sDocParagraph.find("</em>", k+4) != std::string::npos)
        {
            sDocParagraph.replace(k,4, "\\emph{");
            sDocParagraph.replace(sDocParagraph.find("</em>", k+12), 5, "}");
        }

        if (sDocParagraph.substr(k,3) == "<h>" && sDocParagraph.find("</h>", k+3) != std::string::npos)
        {
            sDocParagraph.replace(k, 3, "\\section{");
            sDocParagraph.replace(sDocParagraph.find("</h>", k+4), 4, "}");
        }

        if (sDocParagraph.substr(k,6) == "<expr>" && sDocParagraph.find("</expr>", k+6) != std::string::npos)
        {
            std::string sExpr = sDocParagraph.substr(k+6, sDocParagraph.find("</expr>", k+6)-k-6);
            doc_ReplaceExprContentForTeX(sExpr);
            sDocParagraph.replace(k,
                                  sDocParagraph.find("</expr>",k+6)+7-k,
                                  "$" + sExpr + "$");
        }

        if (sDocParagraph.substr(k,6) == "<code>" && sDocParagraph.find("</code>", k+6) != std::string::npos)
        {
            std::string sCode = sDocParagraph.substr(k+6, sDocParagraph.find("</code>", k+6)-k-6);

            replaceAll(sCode, "\\n", "\\\\");

            sDocParagraph.replace(k, sDocParagraph.find("</code>", k+6)+7-k, "\\lstinline`" + sCode + "`");
            k += sCode.length();
        }

        /*if (sDocParagraph.substr(k,5) == "<img " && sDocParagraph.find("/>", k+5) != std::string::npos)
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
        }*/

        if (sDocParagraph.substr(k, 10) == "&PLOTPATH&")
            sDocParagraph.replace(k, 10, "<plotpath>");

        if (sDocParagraph.substr(k, 10) == "&LOADPATH&")
            sDocParagraph.replace(k, 10, "<loadpath>");

        if (sDocParagraph.substr(k, 10) == "&SAVEPATH&")
            sDocParagraph.replace(k, 10, "<savepath>");

        if (sDocParagraph.substr(k, 10) == "&PROCPATH&")
            sDocParagraph.replace(k, 10, "<procpath>");

        if (sDocParagraph.substr(k, 12) == "&SCRIPTPATH&")
            sDocParagraph.replace(k, 12, "scriptpath>");

        if (sDocParagraph.substr(k, 9) == "&EXEPATH&")
            sDocParagraph.replace(k, 9, "<>");
    }
}



/////////////////////////////////////////////////
/// \brief Returns the final HTML string
/// containing the already lexed and highlighted
/// code.
///
/// \param sCode std::string
/// \param verbatim bool

/// \return std::string
///
/////////////////////////////////////////////////
static std::string getHighlightedCode(std::string sCode, bool verbatim)
{
    if (!verbatim)
    {
        replaceAll(sCode, "\\n", " \\n ");
        replaceAll(sCode, "\\t", "\t");
        replaceAll(sCode, "&lt;", "<");
        replaceAll(sCode, "&gt;", ">");

        //if (sCode.starts_with("|<- "))
        //    sCode.replace(1, 1, "&lt;");
        //
        //if (sCode.starts_with("|-> "))
        //    sCode.replace(2, 1, "&gt;");
    }
    else
        replaceAll(sCode, "\\t", "\t");

    replaceAll(sCode, "\\n", "\\\\\n");
    return sCode;
}


/////////////////////////////////////////////////
/// \brief Returns the final HTML string
/// containing the lexed and highlighted code and
/// embeds that into the code block environment.
///
/// \param sCode std::string
/// \param verbatim bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string formatCodeBlock(std::string sCode, bool verbatim)
{
    sCode = getHighlightedCode(sCode, verbatim);
    doc_ReplaceTokensForTeX(sCode);

    return sCode;
}


static std::string formatExprBlock(std::string sExpr)
{
    replaceAll(sExpr, "\\t", "\t");

    return sExpr;
}


/////////////////////////////////////////////////
/// \brief Parses a list into a two-column
/// structure, which can be converted into a HTML
/// table.
///
/// \param vDocArticle std::vector<std::string>&
/// \param i size_t&
/// \return std::vector<std::pair<std::string, std::string>>
///
/////////////////////////////////////////////////
static std::vector<std::pair<std::string, std::string>> parseList(std::vector<std::string>& vDocArticle, size_t& i)
{
    std::vector<std::pair<std::string, std::string>> vList;

    for (size_t j = i+1; j < vDocArticle.size(); j++)
    {
        if (vDocArticle[j].find("</list>") != std::string::npos)
        {
            i = j;
            break;
        }
        else
        {
            doc_ReplaceTokensForTeX(vDocArticle[j]);
            size_t pos = vDocArticle[j].find("node=")+5;
            std::string& sLine = vDocArticle[j];
            std::string sNode = Documentation::getArgAtPos(sLine, pos);

            vList.push_back(std::make_pair(sNode,
                                           sLine.substr(sLine.find('>', pos+sNode.length()+2)+1,
                                                        sLine.find("</item>")-1-sLine.find('>', pos+sNode.length()+2))));
        }
    }

    return vList;
}



std::string renderTeX(std::vector<std::string>&& vDocArticle, const Settings& _option)
{
    if (vDocArticle[0] == "NO_ENTRY_FOUND") // Nix gefunden
        return "";

    bool isIndex = (vDocArticle[0] == "Index");

    // create the header tag section of the TeX file
    std::string sTeX = "\\chapter{" + vDocArticle[0] + "}\n";


    // Convert the XML-like structure of the documentation
    // article into a valid TeX file, which can be returned
    // as a single std::string
    for (size_t i = 1; i < vDocArticle.size(); i++)
    {
        // Expand the XML tags in the documentation article
        // into corresponding HTML tags, which will resemble
        // the intended style
        if (vDocArticle[i].find("<example ") != std::string::npos) // Beispiel-Tags
        {
            sTeX += "<h4>"+ _lang.get("DOC_HELP_EXAMPLE_HEADLINE") +"</h4>\n";
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

            doc_ReplaceTokensForTeX(sDescription);

            sTeX += "<p>" + sDescription + "</p>\n";

            if (bCodeBlock || bPlain)
            {
                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != std::string::npos)
                    {
                        i = j;
                        sTeX += formatCodeBlock(sCodeContent.substr(0, sCodeContent.length()-2), bPlain) + "\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\\n";
                }
            }
            else
            {
                std::string sExample;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != std::string::npos)
                    {
                        i = j;
                        doc_ReplaceTokensForTeX(sExample);

                       // if (generateFile)
                       //     sTeX += FILE_CODEBLOCK_START + sExample.substr(0, sExample.length()-5) + FILE_CODEBLOCK_END;
                       // else
                       //     sTeX += VIEWER_CODEBLOCK_START + sExample.substr(0, sExample.length()-5) + VIEWER_CODEBLOCK_END;

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
                            sExample += "|&lt;- " + getHighlightedCode(vDocArticle[j], false) + "<br>\n";
                        else
                        {
                            sExample += "|-&gt; " + vDocArticle[j] + "<br>\n";

                            if (vDocArticle[j+1].find("</example>") == std::string::npos)
                                sExample += "|<br>\n";
                        }
                    }
                    else
                        sExample += getHighlightedCode(vDocArticle[j], false) + "<br>\n";
                }
            }

        }
        else if (vDocArticle[i].find("<exprblock>") != std::string::npos) // EXPRBLOCK-Tags
        {
            if (vDocArticle[i].find("</exprblock>", vDocArticle[i].find("<exprblock>")) != std::string::npos)
            {
                doc_ReplaceTokensForTeX(vDocArticle[i]);
                doc_ReplaceExprContentForTeX(vDocArticle[i]);
                size_t pos = vDocArticle[i].find("<exprblock>");
                size_t endpos;

                while (pos != std::string::npos && (endpos = vDocArticle[i].find("</exprblock>", pos)) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(pos+11, endpos-pos-11);
                    replaceAll(sExprBlock, "\\n", "\\\\\n");
                    vDocArticle[i].replace(pos, endpos+12-pos, "\\[" + sExprBlock + "\\]");

                    pos = vDocArticle[i].find("<exprblock>");
                }

                sTeX += vDocArticle[i] + "\n\n";
            }
            else
            {
                if (vDocArticle[i] != "<exprblock>")
                    sTeX += vDocArticle[i].substr(0, vDocArticle[i].find("<exprblock>")) + "\n";

                std::string sExprBlock;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</exprblock>") != std::string::npos)
                    {
                        i = j;
                        sTeX += "\\[\n" + formatExprBlock(sExprBlock) + "\n\\]\n";
                        break;
                    }

                    doc_ReplaceTokensForTeX(vDocArticle[j]);
                    doc_ReplaceExprContentForTeX(vDocArticle[j]);

                    sExprBlock += vDocArticle[j] + "\\\\\n";
                }
            }
        }
        else if (vDocArticle[i].find("<codeblock>") != std::string::npos) // CODEBLOCK-Tags
        {
            if (vDocArticle[i].find("</codeblock>", vDocArticle[i].find("<codeblock>")) != std::string::npos)
            {
                size_t pos = vDocArticle[i].find("<codeblock>");
                size_t endpos;

                while (pos != std::string::npos && (endpos = vDocArticle[i].find("</codeblock>", pos)) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(pos+11, endpos-pos-11);

                    vDocArticle[i].replace(pos, endpos+12-pos,
                                           "\\begin{lstlisting}\n" + formatCodeBlock(sExprBlock, false) + "\n\\end{lstlisting}\n");

                    pos = vDocArticle[i].find("<codeblock>");
                }

                doc_ReplaceTokensForTeX(vDocArticle[i]);
                sTeX += vDocArticle[i] + "\n\n";
            }
            else
            {
                if (vDocArticle[i] != "<codeblock>")
                    sTeX += vDocArticle[i].substr(0, vDocArticle[i].find("<codeblock>")) + "\n";

                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</codeblock>") != std::string::npos)
                    {
                        i = j;
                        sTeX += "\\begin{lstlisting}\n" + formatCodeBlock(sCodeContent.substr(0, sCodeContent.length()-2), false) + "\n\\end{lstlisting}\n";
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
                size_t pos = vDocArticle[i].find("<verbatim>");
                size_t endpos;

                while (pos != std::string::npos && (endpos = vDocArticle[i].find("</verbatim>", pos)) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(pos+10, endpos-pos-10);

                    vDocArticle[i].replace(pos, endpos+11-pos,
                                           "\\begin{verbatim}\n" + formatCodeBlock(sExprBlock, true) + "\n\\end{verbatim}\n");

                    pos = vDocArticle[i].find("<verbatim>");
                }

                doc_ReplaceTokensForTeX(vDocArticle[i]);
                sTeX += vDocArticle[i] + "\n\n";
            }
            else
            {
                if (vDocArticle[i] != "<verbatim>")
                    sTeX += vDocArticle[i].substr(0, vDocArticle[i].find("<verbatim>")) + "\n";

                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</verbatim>") != std::string::npos)
                    {
                        i = j;
                        sTeX += "\\begin{verbatim}\n" + formatCodeBlock(sCodeContent.substr(0, sCodeContent.length()-2), true) + "\n\\end{verbatim}\n";
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
                size_t pos = vDocArticle[i].find("<syntax>");
                size_t endpos;

                while (pos != std::string::npos && (endpos = vDocArticle[i].find("</syntax>", pos)) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(pos+8, endpos-pos-8);

                    vDocArticle[i].replace(pos, endpos+9-pos,
                                           "\n\\section{Syntax}\n\\begin{lstlisting}\n" + formatCodeBlock(sExprBlock, false)
                                                + "\n\\end{lstlisting}\n\\section{" + _lang.get("DOC_HELP_DESC_HEADLINE") + "}\n");

                    pos = vDocArticle[i].find("<syntax>");
                }

                doc_ReplaceTokensForTeX(vDocArticle[i]);
                sTeX += vDocArticle[i] + "\n\n";
            }
            else
            {
                if (vDocArticle[i] != "<syntax>")
                    sTeX += vDocArticle[i].substr(0, vDocArticle[i].find("<syntax>")) + "\n";

                sTeX += "\n\\section{Syntax}\n\\begin{lstlisting}\n";
                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</syntax>") != std::string::npos)
                    {
                        i = j;
                        sTeX += formatCodeBlock(sCodeContent.substr(0, sCodeContent.length()-2), false)
                            + "\n\\end{lstlisting}\n\\section{" + _lang.get("DOC_HELP_DESC_HEADLINE") + "}\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\\n";
                }
            }
        }
        else if (vDocArticle[i].find("<list") != std::string::npos) // Alle LIST-Tags (umgewandelt zu TABLE)
        {
            std::vector<std::pair<std::string, std::string>> vList = parseList(vDocArticle, i);
            bool isUList = true;

            for (const auto& iter : vList)
            {
                if (iter.first != "*")
                {
                    isUList = false;
                    break;
                }
            }

            if (isUList)
            {
                sTeX += "\\begin{itemize}\n";

                for (const auto& iter : vList)
                {
                    sTeX += "  \\item " + iter.second + "\n";
                }

                sTeX += "\\end{itemize}\n";
            }
            else
            {
                sTeX += "<table border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";

                for (const auto& iter : vList)
                {
                    if (isIndex)
                    {
                        sTeX += "    <tr>\n      <td width=\"200\"><a href=\"nhlp://"
                              + iter.first + "?frame=self\"><code><span style=\"color:#00008B;\">"
                              + iter.first + "</span></code></a></td>\n      <td>" + iter.second + "</td>\n    </tr>\n";
                    }
                    else
                    {
                        sTeX += "    <tr>\n      <td width=\"200\"><code><span style=\"color:#00008B;\">"
                                  + iter.first + "</span></code></td>\n      <td>" + iter.second + "</td>\n    </tr>\n";
                    }
                }

                sTeX += "  </tbody>\n</table>\n";
            }
        }
        else if (vDocArticle[i].find("<table") != std::string::npos) // Table-Tags
        {
            sTeX += "<div align=\"center\"><table border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";

            for (size_t j = i+1; j < vDocArticle.size(); j++)
            {
                if (vDocArticle[j].find("</table>") != std::string::npos)
                {
                    sTeX += "  </tbody>\n</table></div>\n";
                    i = j;
                    break;
                }
                else
                {
                    doc_ReplaceTokensForTeX(vDocArticle[j]);
                    sTeX += vDocArticle[j] + "\n";
                }
            }
        }
        else // Normaler Paragraph
        {
            doc_ReplaceTokensForTeX(vDocArticle[i]);
            sTeX += vDocArticle[i] + "\n\n";
        }
    }

    return sTeX;
}

