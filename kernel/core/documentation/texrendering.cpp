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

    //replaceAll(sExpr, "{", "\\{", nPos);
    //replaceAll(sExpr, "}", "\\}", nPos);

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
                sExpr.replace(i, EntitiesDB[n][0].length(), EntitiesDB[n][3] + (EntitiesDB[n][1] == "OP" ? " " : ""));
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

        replaceAll(sExpr, "...", "\\ldots");
        replaceAll(sExpr, "&gt;", ">");
        replaceAll(sExpr, "&lt;", "<");
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

        if (sDocParagraph.substr(k, 8) == "&#x2713;")
            sDocParagraph.replace(k, 8, "\\usym{2713}");

        if (sDocParagraph[k] == '%')
        {
            sDocParagraph.insert(k, "\\");
            k++;
        }

        if (sDocParagraph.substr(k,4) == "<em>" && sDocParagraph.find("</em>", k+4) != std::string::npos)
        {
            sDocParagraph.replace(sDocParagraph.find("</em>", k+5), 5, "}}");
            sDocParagraph.replace(k,4, "\\emph{\\textbf{");
        }

        if (sDocParagraph.substr(k,3) == "<h>" && sDocParagraph.find("</h>", k+3) != std::string::npos)
        {
            sDocParagraph.replace(k, 3, "\\subsection{");
            sDocParagraph.replace(sDocParagraph.find("</h>", k+4), 4, "}");
        }

        if (sDocParagraph.substr(k,3) == "<a " && sDocParagraph.find("</a>", k+3) != std::string::npos)
        {
            std::string sAnchor = sDocParagraph.substr(k, sDocParagraph.find("</a>", k+3)+4-k);

            if (sAnchor.find("href") != std::string::npos)
            {
                std::string sAnchorRef = Documentation::getArgAtPos(sAnchor, sAnchor.find('=', sAnchor.find("href"))+1);
                std::string sAnchorText = sAnchor.substr(sAnchor.find('>')+1, sAnchor.find("</a>") - sAnchor.find('>')-1);

                doc_ReplaceTokensForTeX(sAnchorText);

                if (sAnchorRef.starts_with("nhlp://"))
                {
                    sAnchorRef = NumeReKernel::getInstance()->getSettings().getHelpIdxKey(sAnchorRef.substr(7, sAnchorRef.find('?')-7));

                    if (sAnchorText.starts_with("help "))
                        sAnchor = "\\nameref{help:" + sAnchorRef + "}";
                    else
                        sAnchor = sAnchorText + " {\\footnotesize [$\\Rightarrow$ \\nameref{help:" + sAnchorRef + "}]}";
                }
                else
                    sAnchor = "\\href{" + sAnchor + "}{" + sAnchorText + "}";
            }
            else
                sAnchor.clear();

            sDocParagraph.replace(k, sDocParagraph.find("</a>", k+3)+4-k, sAnchor);
            k += sAnchor.length();
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

            replaceAll(sCode, "&lt;", "<");
            replaceAll(sCode, "&gt;", ">");
            //replaceAll(sCode, "\\t", "\t");
            //replaceAll(sCode, "\\n", "\n");
            replaceAll(sCode, "&amp;", "&");

            sDocParagraph.replace(k, sDocParagraph.find("</code>", k+6)+7-k, "\\lstinline`" + sCode + "`");
            k += sCode.length() + 12;
        }

        if (sDocParagraph.substr(k,5) == "<img " && sDocParagraph.find("/>", k+5) != std::string::npos)
        {
            std::string sImg = sDocParagraph.substr(k, sDocParagraph.find("/>", k+5)+2-k);

            if (sImg.find("src") != std::string::npos)
            {
                std::string sImgSrc = Documentation::getArgAtPos(sImg, sImg.find('=', sImg.find("src"))+1);
                sImgSrc = NumeReKernel::getInstance()->getFileSystem().ValidFileName(sImgSrc, ".png");
                sImg = "\\begin{figure}[ht]\n\\centering\n\\includegraphics[width=0.6\\textwidth]{" + sImgSrc + "}\n\\end{figure}\n";
            }
            else
                sImg.clear();

            sDocParagraph.replace(k, sDocParagraph.find("/>", k+5)+2-k, sImg);
            k += sImg.length();
        }

        if (sDocParagraph.substr(k, 6) == "<item ")
            k = sDocParagraph.find("\">", k)+1;

        if (sDocParagraph.substr(k, 18) == "\\begin{lstlisting}")
            k = sDocParagraph.find("\\end{lstlisting}", k)+16;

        if (sDocParagraph.substr(k, 16) == "\\begin{verbatim}")
            k = sDocParagraph.find("\\end{verbatim}", k)+14;

        if (sDocParagraph[k] == '"')
        {
            if (k+1 == sDocParagraph.length()
                || isblank(sDocParagraph[k+1])
                || sDocParagraph[k+1] == ','
                || sDocParagraph[k+1] == '.'
                || sDocParagraph[k+1] == '-')
                sDocParagraph.replace(k, 1, "<<");
            else
                sDocParagraph.replace(k, 1, ">>");
        }

        if (sDocParagraph.substr(k, 10) == "&PLOTPATH&")
            sDocParagraph.replace(k, 10, "<plotpath>");

        if (sDocParagraph.substr(k, 10) == "&LOADPATH&")
            sDocParagraph.replace(k, 10, "<loadpath>");

        if (sDocParagraph.substr(k, 10) == "&SAVEPATH&")
            sDocParagraph.replace(k, 10, "<savepath>");

        if (sDocParagraph.substr(k, 10) == "&PROCPATH&")
            sDocParagraph.replace(k, 10, "<procpath>");

        if (sDocParagraph.substr(k, 12) == "&SCRIPTPATH&")
            sDocParagraph.replace(k, 12, "<scriptpath>");

        if (sDocParagraph.substr(k, 9) == "&EXEPATH&")
            sDocParagraph.replace(k, 9, "<>");
    }

    replaceAll(sDocParagraph, "&amp;", "\\&");
    replaceAll(sDocParagraph, "<br/>", "\\\\");
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
    //if (!verbatim)
    //{
    //    replaceAll(sCode, "\\n", " \\n ");
    //    //replaceAll(sCode, "&lt;", "<");
    //    //replaceAll(sCode, "&gt;", ">");
    //
    //    //if (sCode.starts_with("|<- "))
    //    //    sCode.replace(1, 1, "&lt;");
    //    //
    //    //if (sCode.starts_with("|-> "))
    //    //    sCode.replace(2, 1, "&gt;");
    //}

    replaceAll(sCode, "&lt;", "<");
    replaceAll(sCode, "&gt;", ">");
    replaceAll(sCode, "\\t", "\t");
    replaceAll(sCode, "\\n", "\n");
    replaceAll(sCode, "&amp;", "\\&");
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
    replaceAll(sCode, "\"", "&q&");
    replaceAll(sCode, "%", "&p&");
    doc_ReplaceTokensForTeX(sCode);
    replaceAll(sCode, "&q&", "\"");
    replaceAll(sCode, "&p&", "%");

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
            replaceAll(vDocArticle[j], "<br/>", "\\newline ");
            replaceAll(vDocArticle[j], "&quot;", "\"");
            replaceAll(vDocArticle[j], "&lt;", "<");
            replaceAll(vDocArticle[j], "&gt;", ">");
            replaceAll(vDocArticle[j], "&", "\\&");
            size_t pos = vDocArticle[j].find("node=")+5;
            std::string& sLine = vDocArticle[j];
            std::string sNode = Documentation::getArgAtPos(sLine, pos);
            size_t nodeLen = sNode.length();
            replaceAll(sNode, "\\newline ", "`\\newline\\lstinline`");
            replaceAll(sNode, "\t", " ");
            replaceAll(sNode, "\\&", "&");

            vList.push_back(std::make_pair(sNode,
                                           sLine.substr(sLine.find('>', pos+nodeLen+2)+1,
                                                        sLine.find("</item>")-1-sLine.find('>', pos+nodeLen+2))));
        }
    }

    return vList;
}


static std::vector<std::vector<std::string>> doc_readTokenTable(const std::string& sTable)
{
    std::vector<std::vector<std::string> > vTable;
    std::vector<std::string> vLine;

    for (size_t i = 0; i < sTable.length(); i++) // <table> <tr> <td>
    {
        if (sTable.substr(i, 4) == "<td>")
        {
            for (size_t j = i+4; j < sTable.length(); j++)
            {
                if (sTable.substr(j, 5) == "</td>")
                {
                    vLine.push_back(sTable.substr(i+4, j-i-4));
                    doc_ReplaceTokensForTeX(vLine.back());
                    replaceAll(vLine.back(), "<br/>", " ");
                    i = j+4;
                    break;
                }
            }
        }

        if (sTable.substr(i, 5) == "</tr>")
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



std::string renderTeX(std::vector<std::string>&& vDocArticle, const std::string& sIndex, const Settings& _option)
{
    if (vDocArticle[0] == "NO_ENTRY_FOUND") // Nix gefunden
        return "";

    bool isIndex = (vDocArticle[0] == "Index");

    // Hack for "chi^2"
    replaceAll(vDocArticle[0], "^2", "$^2$");

    // create the header tag section of the TeX file
    std::string sTeX = "\\section{" + vDocArticle[0] + "}\\label{help:" + sIndex + "}\n";


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
            sTeX += "\\subsection{"+ _lang.get("DOC_HELP_EXAMPLE_HEADLINE") +"}\n";
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

            sTeX += sDescription + "\n";

            if (bCodeBlock || bPlain)
            {
                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</example>") != std::string::npos)
                    {
                        i = j;
                        sTeX += "\\begin{lstlisting}\n" + formatCodeBlock(sCodeContent, bPlain) + "\n\\end{lstlisting}\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\n";
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
                        replaceAll(sExample, "\"", "&q&");
                        replaceAll(sExample, "%", "&p&");
                        doc_ReplaceTokensForTeX(sExample);
                        replaceAll(sExample, "&q&", "\"");
                        replaceAll(sExample, "&p&", "%");
                        sTeX += "\\begin{lstlisting}\n" + sExample + "\\end{lstlisting}\n";

                        break;
                    }

                    if (vDocArticle[j] == "[...]")
                    {
                        sExample += "[...]\n";
                        continue;
                    }

                    if (!bVerb)
                    {
                        if (((i+1) % 2 && j % 2) || (!((i+1) % 2) && !(j % 2)))
                            sExample += "|<- " + getHighlightedCode(vDocArticle[j], false) + "\n";
                        else
                        {
                            sExample += "|-> " + vDocArticle[j] + "\n";

                            if (vDocArticle[j+1].find("</example>") == std::string::npos)
                                sExample += "|\n";
                        }
                    }
                    else
                        sExample += getHighlightedCode(vDocArticle[j], false) + "\n";
                }
            }
            sTeX += "\n";
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
                    vDocArticle[i].replace(pos, endpos+12-pos, "\\begin{gather*}\n" + sExprBlock + "\\end{gather*}\n");

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
                        sTeX += "\\begin{gather*}\n" + formatExprBlock(sExprBlock) + "\\end{gather*}\n";
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
                        sTeX += "\\begin{lstlisting}\n" + formatCodeBlock(sCodeContent, false) + "\\end{lstlisting}\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\n";
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
                        sTeX += "\\begin{verbatim}\n" + formatCodeBlock(sCodeContent, true) + "\\end{verbatim}\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\n";
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
                                           "\n\\subsection{Syntax}\n\\begin{lstlisting}\n" + formatCodeBlock(sExprBlock, false)
                                                + "\n\\end{lstlisting}\n\\subsection{" + _lang.get("DOC_HELP_DESC_HEADLINE") + "}\n");

                    pos = vDocArticle[i].find("<syntax>");
                }

                doc_ReplaceTokensForTeX(vDocArticle[i]);
                sTeX += vDocArticle[i] + "\n\n";
            }
            else
            {
                if (vDocArticle[i] != "<syntax>")
                    sTeX += vDocArticle[i].substr(0, vDocArticle[i].find("<syntax>")) + "\n";

                sTeX += "\\subsection{Syntax}\n\\begin{lstlisting}\n";
                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</syntax>") != std::string::npos)
                    {
                        i = j;
                        sTeX += formatCodeBlock(sCodeContent, false)
                            + "\\end{lstlisting}\n\\subsection{" + _lang.get("DOC_HELP_DESC_HEADLINE") + "}\n";
                        break;
                    }

                    sCodeContent += vDocArticle[j] + "\n";
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
                sTeX += "\\begin{small}\n\\centering\n\\begin{longtable}{p{0.35\\textwidth}p{0.6\\textwidth}}\n\\toprule\n";

                for (const auto& iter : vList)
                {
                    sTeX += "    \\lstinline`" + iter.first + "` & " + iter.second + "\\\\ \n";
                }

                sTeX += "\\bottomrule\n\\end{longtable}\n\\end{small}\n";
            }
        }
        else if (vDocArticle[i].find("<table") != std::string::npos) // TABLE-Tags
        {
            std::string sTable = vDocArticle[i].substr(vDocArticle[i].find("<table"));

            for (size_t j = i+1; j < vDocArticle.size(); j++)
            {
                if (vDocArticle[j].find("</table>") != std::string::npos)
                {
                    sTable += vDocArticle[j].substr(0, vDocArticle[j].find("</table>")+8);

                    // Send the whole content to the table reader and render the obtained table on the screen.
                    std::vector<std::vector<std::string>> vTable = doc_readTokenTable(sTable);
                    sTeX += "\\begin{small}\n\\centering\n\\begin{longtable}{"
                        + strfill("", vTable[0].size(), 'c') + "}\n\\toprule\n";

                    for (size_t v = 0; v < vTable.size(); v++)
                    {
                        for (size_t w = 0; w < vTable[v].size(); w++)
                        {
                            if (w)
                                sTeX += " & ";

                            sTeX += vTable[v][w];
                        }

                        sTeX += "\\\\ \n";
                    }

                    sTeX += "\\bottomrule\n\\end{longtable}\n\\end{small}\n";

                    i = j;
                    break;
                }
                else
                    sTable += vDocArticle[j];
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

