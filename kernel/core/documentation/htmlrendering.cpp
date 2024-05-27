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

#include "htmlrendering.hpp"
#include "../datamanagement/database.hpp"
#include "../structures.hpp"
#include "../utils/tools.hpp"
#include "../../kernel.hpp"
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
/// \return void
///
/////////////////////////////////////////////////
static void doc_ReplaceExprContentForHTML(std::string& sExpr)
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
                sExpr.replace(i, EntitiesDB[n][0].length(), EntitiesDB[n][2]);
                i += EntitiesDB[n][2].length()-1;
            }
        }

        // Handle supscripts
        if (sExpr[i] == '^')
        {
            if (sExpr[i+1] == '(')
            {
                sExpr.replace(getMatchingParenthesis(StringView(sExpr, i))+i, 1, "</sup>");
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
                sExpr.replace(getMatchingParenthesis(StringView(sExpr, i))+i, 1, "</sub>");
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
/// \param _option const Settings&
/// \return void
///
/////////////////////////////////////////////////
static void doc_ReplaceTokensForHTML(std::string& sDocParagraph, bool generateFile, const Settings& _option)
{
    for (size_t k = 0; k < sDocParagraph.length(); k++)
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
            doc_ReplaceExprContentForHTML(sExpr);
            sDocParagraph.replace(k,
                                  sDocParagraph.find("</expr>",k+6)+7-k,
                                  "<span style=\"font-style:italic; font-family: palatino linotype; font-weight: bold;\">"+sExpr+"</span>");
        }

        if (sDocParagraph.substr(k,6) == "<code>" && sDocParagraph.find("</code>", k+6) != std::string::npos)
        {
            sDocParagraph.insert(k+6, "<span style=\"color:#00008B;background-color:#F2F2F2;\">");
            sDocParagraph.insert(sDocParagraph.find("</code>", k+6), "</span>");
            std::string sCode = sDocParagraph.substr(k+6, sDocParagraph.find("</code>", k+6)-k-6);

            for (size_t i = 0; i < sCode.length(); i++)
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


#define FILE_CODEBLOCK_START "<div class=\"sites-codeblock sites-codesnippet-block\"><CODE><span style=\"color:#00008B;\">\n"
#define FILE_CODEBLOCK_END "</span></CODE></div>\n"

#define VIEWER_CODEBLOCK_START "<center><table border=\"0\" cellspacing=\"0\" bgcolor=\"#F2F2F2\" width=\"94%\">\n<tbody><tr><td>\n<CODE><span style=\"color:#00008B;\">\n"
#define VIEWER_CODEBLOCK_END "\n</span></CODE></td></tr></tbody></table></center>\n"


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
/// \param _option const Settings&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getHighlightedCode(std::string sCode, bool verbatim, const Settings& _option)
{
    if (!verbatim)
    {
        replaceAll(sCode, "\\n", " \\n ");
        replaceAll(sCode, "\\t", "\\t ");
        replaceAll(sCode, "&lt;", "<");
        replaceAll(sCode, "&gt;", ">");
        sCode = applySyntaxHighlighting(sCode, _option);
        replaceAll(sCode, "\\t ", "&nbsp;&nbsp;&nbsp;&nbsp;");

        if (sCode.starts_with("|<- "))
            sCode.replace(1, 1, "&lt;");

        if (sCode.starts_with("|-> "))
            sCode.replace(2, 1, "&gt;");
    }
    else
        replaceAll(sCode, "\\t", "&nbsp;&nbsp;&nbsp;&nbsp;");

    replaceAll(sCode, "\\n", "<br>\n");
    return sCode;
}


/////////////////////////////////////////////////
/// \brief Returns the final HTML string
/// containing the lexed and highlighted code and
/// embeds that into the code block environment.
///
/// \param sCode std::string
/// \param generateFile bool
/// \param verbatim bool
/// \param _option const Settings&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string formatCodeBlock(std::string sCode, bool generateFile, bool verbatim, const Settings& _option)
{
    sCode = getHighlightedCode(sCode, verbatim, _option);
    doc_ReplaceTokensForHTML(sCode, generateFile, _option);

    if (generateFile)
        return FILE_CODEBLOCK_START + sCode + FILE_CODEBLOCK_END;

    return VIEWER_CODEBLOCK_START + sCode + VIEWER_CODEBLOCK_END;
}


/////////////////////////////////////////////////
/// \brief Returns the final HTML string for an
/// expression block.
///
/// \param sExpr std::string
/// \param generateFile bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string formatExprBlock(std::string sExpr, bool generateFile)
{
    replaceAll(sExpr, "\\t", "&nbsp;&nbsp;&nbsp;&nbsp;");

    if (generateFile)
        return "<div style=\"font-style: italic;margin-left: 40px\">" + sExpr + "</div>";

    return "<blockquote><span style=\"font-style: italic; font-family: palatino linotype; font-size: 12pt; font-weight: bold;\">" + sExpr + "</span></blockquote>";
}


/////////////////////////////////////////////////
/// \brief Parses a list into a two-column
/// structure, which can be converted into a HTML
/// table.
///
/// \param vDocArticle std::vector<std::string>&
/// \param i size_t&
/// \param generateFile bool
/// \param _option const Settings&
/// \return std::vector<std::pair<std::string, std::string>>
///
/////////////////////////////////////////////////
static std::vector<std::pair<std::string, std::string>> parseList(std::vector<std::string>& vDocArticle, size_t& i, bool generateFile, const Settings& _option)
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
            doc_ReplaceTokensForHTML(vDocArticle[j], generateFile, _option);
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


/////////////////////////////////////////////////
/// \brief Render the XML structure as HTML.
///
/// \param vDocArticle std::vector<std::string>&&
/// \param generateFile bool
/// \param _option const Settings&
/// \return std::string
///
/////////////////////////////////////////////////
std::string renderHTML(std::vector<std::string>&& vDocArticle, bool generateFile, const Settings& _option)
{
    if (vDocArticle[0] == "NO_ENTRY_FOUND") // Nix gefunden
        return "";

    bool isIndex = (vDocArticle[0] == "Index");

    std::string sHTML;

    sHTML = "<!DOCTYPE html>\n<html>\n<head>\n";

    // create the header tag section of the HTML file
    if (generateFile)
    {
        // Header fertigstellen
        sHTML += "<title>" + toUpperCase(_lang.get("DOC_HELP_HEADLINE", vDocArticle[0]))
              + "</title>\n"
              + "</head>\n\n"
              + "<body>\n"
              + "<!-- START COPYING HERE -->\n";
        sHTML += "<h4>" + _lang.get("DOC_HELP_DESC_HEADLINE") + "</h4>\n";
    }
    else
    {
        // Header fertigstellen
        sHTML += "<title>" + vDocArticle[0] + "</title>\n</head>\n\n<body>\n<h2>"+vDocArticle[0]+"</h2>\n";
    }

    // Convert the XML-like structure of the documentation
    // article into a valid HTML DOM, which can be returned
    // as a single std::string
    for (size_t i = 1; i < vDocArticle.size(); i++)
    {
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

            sHTML += "<p>" + sDescription + "</p>\n";

            if (bCodeBlock || bPlain)
            {
                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
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

                for (size_t j = i+1; j < vDocArticle.size(); j++)
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
                doc_ReplaceExprContentForHTML(vDocArticle[i]);
                size_t pos = vDocArticle[i].find("<exprblock>");
                size_t endpos;

                while (pos != std::string::npos && (endpos = vDocArticle[i].find("</exprblock>", pos)) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(pos+11, endpos-pos-11);
                    replaceAll(sExprBlock, "\\n", "<br>");
                    vDocArticle[i].replace(pos, endpos+12-pos, "</p>" + formatExprBlock(sExprBlock, generateFile) + "<p>");

                    pos = vDocArticle[i].find("<exprblock>");
                }

                sHTML += "<p>" + vDocArticle[i] + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<exprblock>")
                    sHTML += "<p>" + vDocArticle[i].substr(0, vDocArticle[i].find("<exprblock>")) + "</p>\n";

                std::string sExprBlock;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
                {
                    if (vDocArticle[j].find("</exprblock>") != std::string::npos)
                    {
                        i = j;

                        if (!generateFile)
                            sExprBlock.erase(sExprBlock.length()-5);

                        sHTML += formatExprBlock(sExprBlock, generateFile) + "\n";
                        break;
                    }

                    doc_ReplaceTokensForHTML(vDocArticle[j], generateFile, _option);
                    doc_ReplaceExprContentForHTML(vDocArticle[j]);

                    sExprBlock += vDocArticle[j] + "<br>\n";
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
                                           "</p>" + formatCodeBlock(sExprBlock, generateFile, false, _option) + "<p>");

                    pos = vDocArticle[i].find("<codeblock>");
                }

                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
                sHTML += "<p>" + vDocArticle[i] + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<codeblock>")
                    sHTML += "<p>" + vDocArticle[i].substr(0, vDocArticle[i].find("<codeblock>")) + "</p>\n";

                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
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
                size_t pos = vDocArticle[i].find("<verbatim>");
                size_t endpos;

                while (pos != std::string::npos && (endpos = vDocArticle[i].find("</verbatim>", pos)) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(pos+10, endpos-pos-10);

                    vDocArticle[i].replace(pos, endpos+11-pos,
                                           "</p>" + formatCodeBlock(sExprBlock, generateFile, true, _option) + "<p>");

                    pos = vDocArticle[i].find("<verbatim>");
                }

                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
                sHTML += "<p>" + vDocArticle[i] + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<verbatim>")
                    sHTML += "<p>" + vDocArticle[i].substr(0, vDocArticle[i].find("<verbatim>")) + "</p>\n";

                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
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
                size_t pos = vDocArticle[i].find("<syntax>");
                size_t endpos;

                while (pos != std::string::npos && (endpos = vDocArticle[i].find("</syntax>", pos)) != std::string::npos)
                {
                    std::string sExprBlock = vDocArticle[i].substr(pos+8, endpos-pos-8);

                    vDocArticle[i].replace(pos, endpos+9-pos,
                                           "</p><h4>Syntax</h4>" + formatCodeBlock(sExprBlock, generateFile, false, _option)
                                                + "<h4>" + _lang.get("DOC_HELP_DESC_HEADLINE") + "</h4><p>");

                    pos = vDocArticle[i].find("<syntax>");
                }

                doc_ReplaceTokensForHTML(vDocArticle[i], generateFile, _option);
                sHTML += "<p>" + vDocArticle[i] + "</p>\n";
            }
            else
            {
                if (vDocArticle[i] != "<syntax>")
                    sHTML += "<p>" + vDocArticle[i].substr(0, vDocArticle[i].find("<syntax>")) + "</p>\n";

                sHTML += "<h4>Syntax</h4>\n";
                std::string sCodeContent;

                for (size_t j = i+1; j < vDocArticle.size(); j++)
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
            bool isEnum = vDocArticle[i].find("type=\"enum\"", vDocArticle[i].find("<list")) != std::string::npos;
            bool isUList = true;
            std::vector<std::pair<std::string, std::string>> vList = parseList(vDocArticle, i, generateFile, _option);

            for (const auto& iter : vList)
            {
                if (iter.first != "*")
                {
                    isUList = false;
                    break;
                }
            }

            if (isEnum)
            {
                sHTML += "<ol>\n";

                for (const auto& iter : vList)
                {
                    sHTML += "  <li>" + iter.second + "</li>\n";
                }

                sHTML += "</ol>\n";
            }
            else if (isUList)
            {
                sHTML += "<ul>\n";

                for (const auto& iter : vList)
                {
                    sHTML += "  <li>" + iter.second + "</li>\n";
                }

                sHTML += "</ul>\n";
            }
            else
            {
                if (generateFile)
                {
                    sHTML += "<h4>"+ _lang.get("DOC_HELP_OPTIONS_HEADLINE") +"</h4>\n";
                    sHTML += "<table style=\"border-collapse:collapse; border-color:rgb(136,136,136);border-width:1px\" border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";
                }
                else
                    sHTML += "<table border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";

                for (const auto& iter : vList)
                {
                    if (generateFile)
                    {
                        sHTML += "    <tr>\n      <td style=\"width:200px;height:19px\"><code><span style=\"color:#00008B;\">"
                              + iter.first + "</span></code></td>\n"
                              + "      <td style=\"width:400px;height:19px\">" + iter.second + "</td>\n    </tr>\n";
                    }
                    else if (isIndex)
                    {
                        sHTML += "    <tr>\n      <td width=\"200\"><a href=\"nhlp://"
                              + iter.first + "?frame=self\"><code><span style=\"color:#00008B;\">"
                              + iter.first + "</span></code></a></td>\n      <td>" + iter.second + "</td>\n    </tr>\n";
                    }
                    else
                    {
                        sHTML += "    <tr>\n      <td width=\"200\"><code><span style=\"color:#00008B;\">"
                                  + iter.first + "</span></code></td>\n      <td>" + iter.second + "</td>\n    </tr>\n";
                    }
                }

                sHTML += "  </tbody>\n</table>\n";
            }
        }
        else if (vDocArticle[i].find("<table") != std::string::npos) // Table-Tags
        {
            if (generateFile)
                sHTML += "<div align=\"center\"><table style=\"border-collapse:collapse; border-color:rgb(136,136,136);border-width:1px\" border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";
            else
                sHTML += "<div align=\"center\"><table border=\"1\" bordercolor=\"#888\" cellspacing=\"0\">\n  <tbody>\n";

            for (size_t j = i+1; j < vDocArticle.size(); j++)
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

