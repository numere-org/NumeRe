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
#include "../ui/calltipprovider.hpp"
#include "../utils/tools.hpp"

#include "htmlrendering.hpp"
#include "texrendering.hpp"

static std::string doc_HelpAsTeX(const std::string& __sTopic, Settings& _option);


/////////////////////////////////////////////////
/// \brief Splits an itemized list into the
/// external NHLP xml-ish file structure.
///
/// \param sDefinition const std::string&
/// \param vDoc std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
static void doc_splitDocumentation(const std::string& sDefinition, std::vector<std::string>& vDoc)
{
    bool isInList = false;
    std::vector<std::string> vSplitted = split(sDefinition, '\n');

    for (size_t i = 0; i < vSplitted.size(); i++)
    {
        if (vSplitted[i][4] != '-')
        {
            if (isInList)
            {
                vDoc.push_back("</list>");
                isInList = false;
            }

            vDoc.push_back(vSplitted[i].substr(4));
        }
        else
        {
            if (!isInList)
            {
                vDoc.push_back("<list>");
                isInList = true;
            }

            std::string sNode = "*";
            std::string sText;

            if (vSplitted[i].find(": ") != std::string::npos)
            {
                sText = vSplitted[i].substr(vSplitted[i].find(": ")+2);
                sNode = vSplitted[i].substr(5, vSplitted[i].find(": ")-5);
                replaceAll(sNode, "\"", "");
            }
            else
                sText = vSplitted[i].substr(5);

            StripSpaces(sNode);
            StripSpaces(sText);

            vDoc.push_back("<item node=\"" + sNode + "\">" + sText + "</item>");
        }
    }

    if (isInList)
        vDoc.push_back("</list>");
}


/////////////////////////////////////////////////
/// \brief Extract the argument list of the
/// passed function definition.
///
/// \param sDefinition std::string
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
static std::vector<std::string> getArgumentList(std::string sDefinition)
{
    size_t pos = sDefinition.find('(');

    if (pos == std::string::npos)
        return std::vector<std::string>();

    if (sDefinition.find("().") != std::string::npos)
        return getArgumentList(sDefinition.substr(sDefinition.find("().")+3));

    sDefinition.erase(0, pos+1);
    sDefinition.erase(sDefinition.rfind(')'));

    return getAllArguments(sDefinition);
}


/////////////////////////////////////////////////
/// \brief Replaces the occurences of a single
/// argument in a single string with their code
/// highlighted variant.
///
/// \param sArgument const std::string&
/// \param sString std::string&
/// \param nPos size_t
/// \return void
///
/////////////////////////////////////////////////
static void replaceArgumentOccurences(const std::string& sArgument, std::string& sString, size_t nPos)
{
    // Search for the next occurence of the variable
    while ((nPos = sString.find(sArgument, nPos)) != std::string::npos)
    {
        // check, whether the found match is an actual variable
        if (StringView(sString).is_delimited_sequence(nPos, sArgument.length()))
        {
            // replace VAR with <code>VAR</code> and increment the
            // position index by the variable length + 13
            sString.replace(nPos, sArgument.length(), "<code>" + sArgument + "</code>");
            nPos += sArgument.length() + 13;
        }
        else
            nPos += sArgument.length();
    }
}


/////////////////////////////////////////////////
/// \brief Apply a code style to the arguments of
/// the function used within the description.
///
/// \param vDoc std::vector<std::string>&
/// \param sDefinition const std::string&
/// \return void
///
/////////////////////////////////////////////////
static void applyCodeHighlighting(std::vector<std::string>& vDoc, const std::string& sDefinition)
{
    std::vector<std::string> vArgs = getArgumentList(sDefinition);

    for (std::string arg : vArgs)
    {
        // clean arg, if necessary
        if (arg.find_first_of(" =&") != std::string::npos)
            arg.erase(arg.find_first_of(" =&"));

        if (arg.front() == '{' && arg.back() == '}')
            arg = arg.substr(1, arg.length()-2);

        // We start after the closing </syntax>
        for (size_t i = 4; i < vDoc.size(); i++)
        {
            size_t startPos = 0;

            if (vDoc[i].starts_with("<item "))
                startPos = vDoc[i].find("\">")+2;

            replaceArgumentOccurences(arg, vDoc[i], startPos);
            replaceArgumentOccurences("_" + arg, vDoc[i], startPos);
        }
    }
}


/////////////////////////////////////////////////
/// \brief Tries to get the function
/// documentation strings from the language file
/// by the use of a CallTipProvider instance.
///
/// \param sToken std::string
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
static std::vector<std::string> doc_findFunctionDocumentation(std::string sToken)
{
    static NumeRe::CallTipProvider tipProvider(std::string::npos, false);

    if (sToken.find('(') != std::string::npos)
        sToken.erase(sToken.find('('));

    std::vector<std::string> vDoc;
    NumeRe::CallTip _ctip = tipProvider.getFunction(sToken);

    if (_ctip.sDefinition.length())
    {
        vDoc.push_back(sToken + "(...)");
        vDoc.push_back("<syntax>");
        vDoc.push_back(_ctip.sDefinition);
        vDoc.push_back("</syntax>");

        doc_splitDocumentation(_ctip.sDocumentation, vDoc);
        applyCodeHighlighting(vDoc, _ctip.sDefinition);
        return vDoc;
    }

    _ctip = tipProvider.getProcedure(sToken);

    if (_ctip.sDefinition.length())
    {
        vDoc.push_back(sToken + "(...)");
        vDoc.push_back("<syntax>");
        vDoc.push_back(_ctip.sDefinition);
        vDoc.push_back("</syntax>");

        doc_splitDocumentation(_ctip.sDocumentation, vDoc);
        applyCodeHighlighting(vDoc, _ctip.sDefinition);
        return vDoc;
    }

    _ctip = tipProvider.getMethod(sToken);

    if (_ctip.sDefinition.length())
    {
        sToken = _ctip.sDefinition;

        if (sToken.find('(', sToken.find('.')) != std::string::npos)
            sToken.replace(sToken.find('(', sToken.find('.')), std::string::npos, "(...)");
        else
            sToken.erase(sToken.find(' '));

        vDoc.push_back(sToken);
        vDoc.push_back("<syntax>");
        vDoc.push_back(_ctip.sDefinition);
        vDoc.push_back("</syntax>");

        doc_splitDocumentation(_ctip.sDocumentation, vDoc);
        applyCodeHighlighting(vDoc, _ctip.sDefinition);
        return vDoc;
    }

    return vDoc;
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
    std::string sTopic = __sTopic;

    if (findParameter(sTopic, "html"))
        eraseToken(sTopic, "html", false);

    if (findParameter(sTopic, "tex"))
        eraseToken(sTopic, "tex", false);

    // --> Zunaechst einmal muessen wir anfuehrende oder abschliessende Leerzeichen entfernen <--
    StripSpaces(sTopic);

    if (!sTopic.length())
        sTopic = "brief";

    if (sTopic.front() == '-')
        sTopic.erase(0,1);

    if (sTopic.back() == '-')
        sTopic.pop_back();

    StripSpaces(sTopic);

    if (sTopic.length() > 2 && sTopic.front() == '"' && sTopic.back() == '"')
        sTopic = toInternalString(sTopic);

    // Check for function documentation first
    if (doc_findFunctionDocumentation(sTopic).size())
    {
        bool generateFile = (bool)findParameter(__sTopic, "html");
        std::string sHTML = doc_HelpAsHTML(sTopic, generateFile, _option);
        NumeReKernel::setDocumentation(sHTML);
        return;
    }

    std::vector<std::string> vDocArticle = _option.getHelpArticle(sTopic);

    if (vDocArticle[0] == "NO_ENTRY_FOUND") // Nix gefunden
    {
        make_hline();
        NumeReKernel::print(LineBreak(_lang.get("DOC_HELP_NO_ENTRY_FOUND", sTopic), _option));
        make_hline();
    }
    else if (findParameter(__sTopic, "tex"))
    {
        std::string sTeX = doc_HelpAsTeX(sTopic, _option);

        _option.declareFileType(".tex");
        std::string sFilename = _option.ValidizeAndPrepareName("<>/docs/texexport/"+_option.getHelpArticleID(sTopic) + ".tex",".tex");
        std::ofstream fTeX;
        fTeX.open(sFilename);

        if (fTeX.fail())
            throw SyntaxError(SyntaxError::CANNOT_GENERATE_FILE, "", SyntaxError::invalid_position, sFilename);

        // content schreiben
        fTeX << ansiToUtf8(sTeX);

        if (_option.systemPrints())
            NumeReKernel::print(_lang.get("DOC_HELP_HTMLEXPORT", _option.getHelpArticleTitle(_option.getHelpIdxKey(sTopic)), sFilename));
    }
    else
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

            if (_option.systemPrints())
                NumeReKernel::print(_lang.get("DOC_HELP_HTMLEXPORT", _option.getHelpArticleTitle(_option.getHelpIdxKey(sTopic)), sFilename));
        }
        else
            NumeReKernel::setDocumentation(sHTML);
    }

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
    std::vector<std::string> vDocArticle = doc_findFunctionDocumentation(sTopic);

    if (!vDocArticle.size())
        vDocArticle = _option.getHelpArticle(toLowerCase(sTopic));

    return renderHTML(std::move(vDocArticle), generateFile, _option);
}


/////////////////////////////////////////////////
/// \brief This function returns the
/// documentation article for the selected topic
/// as a TeX std::string. This std::string
/// may be used to create a corresponding
/// file.
///
/// \param __sTopic const std::string&
/// \param _option Settings&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string doc_HelpAsTeX(const std::string& __sTopic, Settings& _option)
{
    std::string sTopic = __sTopic;
    StripSpaces(sTopic);
    std::string sIndex;

    // Get the article contents
    std::vector<std::string> vDocArticle = doc_findFunctionDocumentation(sTopic);

    if (!vDocArticle.size())
    {
        vDocArticle = _option.getHelpArticle(toLowerCase(sTopic));
        sIndex = _option.getHelpIdxKey(toLowerCase(sTopic));
    }

    return renderTeX(std::move(vDocArticle), sIndex, _option);
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

