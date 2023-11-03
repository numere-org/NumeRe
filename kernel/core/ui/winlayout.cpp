/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#include "winlayout.hpp"
#include "../../../externals/tinyxml2/tinyxml2.h"
#include "../io/styledtextfile.hpp"
#include "../../kernel.hpp"
#include "../procedure/includer.hpp"

#include <stack>
#include <memory>

std::string removeQuotationMarks(const std::string& sString);


/////////////////////////////////////////////////
/// \brief This static function parses a
/// numerical argument.
///
/// \param sCmd const std::string&
/// \param pos size_t
/// \return std::string
///
/////////////////////////////////////////////////
static std::string parseNumOpt(const std::string& sCmd, size_t pos)
{
    return getArgAtPos(sCmd, pos, ARGEXTRACT_PARSED | ARGEXTRACT_STRIPPED);
}


/////////////////////////////////////////////////
/// \brief This static function parses a string
/// option.
///
/// \param sCmd const std::string&
/// \param pos size_t
/// \return std::string
///
/////////////////////////////////////////////////
static std::string parseStringOpt(const std::string& sCmd, size_t pos)
{
    std::string arg = getArgAtPos(sCmd, pos, ARGEXTRACT_PARSED);
    StripSpaces(arg);

    if (arg.find(",") != std::string::npos && arg.find("\"") != std::string::npos)
        return arg;
    else if (arg.front() == '"' && arg.back() == '"')
        return arg.substr(1, arg.length()-2);

    return arg;
}


/////////////////////////////////////////////////
/// \brief This static function parses a event
/// argument.
///
/// \param sCmd const std::string&
/// \param pos size_t
/// \param sFolderName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string parseEventOpt(const std::string& sCmd, size_t pos, const std::string& sFolderName)
{
    std::string option = getArgAtPos(sCmd, pos);
    std::string sProcBase = NumeReKernel::getInstance()->getSettings().getProcPath();

    if (sFolderName.starts_with(sProcBase))
    {
        sProcBase = sFolderName.substr(sProcBase.length());
        replaceAll(sProcBase, "/", "~");

        while (sProcBase.front() == '~')
            sProcBase.erase(0, 1);

        if (sProcBase.length() && sProcBase.back() != '~')
            sProcBase += "~";
    }
    else
        sProcBase.clear();

    if (option.front() == '$' && option.substr(option.length()-2) == "()")
        option.erase(option.length()-2);

    if (option.starts_with("$this~"))
        option = "$" + sProcBase + option.substr(6);

    return option;
}


/////////////////////////////////////////////////
/// \brief This static function evaluates the
/// expression part of each window layout
/// command.
///
/// \param sExpr std::string&
/// \return void
///
/////////////////////////////////////////////////
static void evaluateExpression(std::string& sExpr)
{
    NumeReKernel* instance = NumeReKernel::getInstance();

    if (sExpr.find_first_not_of(' ') == std::string::npos)
        return;

    // Call functions
    if (!instance->getDefinitions().call(sExpr))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sExpr, "");

    // Evaluate strings
    if (instance->getStringParser().isStringExpression(sExpr))
    {
        std::string dummy;
        NumeRe::StringParser::StringParserRetVal _ret = instance->getStringParser().evalAndFormat(sExpr, dummy, true);

        if (_ret == NumeRe::StringParser::STRING_SUCCESS)
            return;
    }

    if (instance->getMemoryManager().containsTablesOrClusters(sExpr))
        getDataElements(sExpr, instance->getParser(), instance->getMemoryManager(), instance->getSettings());

    // Numerical evaluation
    instance->getParser().SetExpr(sExpr);

    int results;
    mu::value_type* v = instance->getParser().Eval(results);

    sExpr.clear();

    for (int i = 0; i < results; i++)
    {
        if (sExpr.length())
            sExpr += ",";

        sExpr += toString(v[i], 7);
    }
}


/////////////////////////////////////////////////
/// \brief This static function parses a single
/// layout command into a usable XML element.
///
/// \param sLayoutCommand const std::string&
/// \param layoutElement tinyxml2::XMLElement*
/// \param sFolderName const std::string&
/// \return void
///
/////////////////////////////////////////////////
static void parseLayoutCommand(const std::string& sLayoutCommand, tinyxml2::XMLElement* layoutElement, const std::string& sFolderName)
{
    std::string sExpr = sLayoutCommand.substr(0, std::min(sLayoutCommand.find("-set"), sLayoutCommand.find("--")));

    replaceAll(sExpr, "<this>", sFolderName.c_str());

    evaluateExpression(sExpr);

    StripSpaces(sExpr);

    layoutElement->SetText(sExpr.c_str());

    if (findParameter(sLayoutCommand, "id", '='))
        layoutElement->SetAttribute("id", parseNumOpt(sLayoutCommand, findParameter(sLayoutCommand, "id", '=')+2).c_str());

    if (findParameter(sLayoutCommand, "relscl", '='))
        layoutElement->SetAttribute("prop", parseNumOpt(sLayoutCommand, findParameter(sLayoutCommand, "relscl", '=')+6).c_str());

    if (findParameter(sLayoutCommand, "color", '='))
        layoutElement->SetAttribute("color", parseNumOpt(sLayoutCommand, findParameter(sLayoutCommand, "color", '=')+5).c_str());

    if (findParameter(sLayoutCommand, "min", '='))
        layoutElement->SetAttribute("min", parseNumOpt(sLayoutCommand, findParameter(sLayoutCommand, "min", '=')+3).c_str());

    if (findParameter(sLayoutCommand, "max", '='))
        layoutElement->SetAttribute("max", parseNumOpt(sLayoutCommand, findParameter(sLayoutCommand, "max", '=')+3).c_str());

    if (findParameter(sLayoutCommand, "value", '='))
        layoutElement->SetAttribute("value", parseStringOpt(sLayoutCommand, findParameter(sLayoutCommand, "value", '=')+5).c_str());

    if (findParameter(sLayoutCommand, "label", '='))
        layoutElement->SetAttribute("label", parseStringOpt(sLayoutCommand, findParameter(sLayoutCommand, "label", '=')+5).c_str());

    if (findParameter(sLayoutCommand, "font", '='))
        layoutElement->SetAttribute("font", parseStringOpt(sLayoutCommand, findParameter(sLayoutCommand, "font", '=')+4).c_str());

    if (findParameter(sLayoutCommand, "align", '='))
        layoutElement->SetAttribute("align", parseStringOpt(sLayoutCommand, findParameter(sLayoutCommand, "align", '=')+5).c_str());

    if (findParameter(sLayoutCommand, "type", '='))
        layoutElement->SetAttribute("type", getArgAtPos(sLayoutCommand, findParameter(sLayoutCommand, "type", '=')+4).c_str());

    if (findParameter(sLayoutCommand, "size", '='))
        layoutElement->SetAttribute("size", parseNumOpt(sLayoutCommand, findParameter(sLayoutCommand, "size", '=')+4).c_str());

    if (findParameter(sLayoutCommand, "state", '='))
        layoutElement->SetAttribute("state", getArgAtPos(sLayoutCommand, findParameter(sLayoutCommand, "state", '=')+5).c_str());

    if (findParameter(sLayoutCommand, "onchange", '='))
        layoutElement->SetAttribute("onchange", parseEventOpt(sLayoutCommand,
                                                              findParameter(sLayoutCommand, "onchange", '=')+8, sFolderName).c_str());

    if (findParameter(sLayoutCommand, "onclick", '='))
        layoutElement->SetAttribute("onclick", parseEventOpt(sLayoutCommand,
                                                             findParameter(sLayoutCommand, "onclick", '=')+7, sFolderName).c_str());

    if (findParameter(sLayoutCommand, "onactivate", '='))
        layoutElement->SetAttribute("onactivate", parseEventOpt(sLayoutCommand,
                                                                findParameter(sLayoutCommand, "onactivate", '=')+10, sFolderName).c_str());
}


/////////////////////////////////////////////////
/// \brief This static function parses a layout
/// script into a xml data container usable by
/// the GUI. Returns the name of the onopen event
/// handler, if any.
///
/// \param sLayoutScript std::string&
/// \param layout tinyxml2::XMLDocument*
/// \return std::string
///
/////////////////////////////////////////////////
static std::string parseLayoutScript(std::string& sLayoutScript, tinyxml2::XMLDocument* layout)
{
    // Ensure that the file name of the layout
    // script is valid
    sLayoutScript = NumeReKernel::getInstance()->getScript().ValidFileName(sLayoutScript, ".nlyt");

    // Load the layoutscript as a StyledTextFile
    StyledTextFile layoutScript(sLayoutScript);
    SymDefManager _symDefs;
    std::unique_ptr<Includer> _includer; // Pointer-based, because it might not be available each time

    std::string sFolderName = sLayoutScript.substr(0, sLayoutScript.rfind('/'));
    std::string sOnOpenEvent;

    // Nothing read?
    if (!layoutScript.getLinesCount())
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sLayoutScript);

    std::stack<tinyxml2::XMLElement*> currentGroup;
    tinyxml2::XMLElement* currentChild = nullptr;
    int i = 0;

    // Go through the layout script
    while (i < layoutScript.getLinesCount())
    {
        std::string line;

        // If the includer is open, get the line from
        // here
        if (_includer && _includer->is_open())
        {
            line = _includer->getNextLine();

            // Remove not necessary global statements
            if (findCommand(line).sString == "global")
                line.erase(0, findCommand(line).nPos+7);
        }
        else
        {
            // Free up the includer, if it is not necessary any more
            if (_includer)
                _includer.reset();

            // Get the current line without comments
            line = layoutScript.getStrippedLine(i);
            i++;

            // If this line contains an including syntax, create
            // a new includer to resolve that
            if (Includer::is_including_syntax(line) && !_includer)
            {
                _includer.reset(new Includer(line, sLayoutScript.substr(0, sLayoutScript.rfind('/'))));
                continue;
            }
        }

        // Resolve symbol declarations
        _symDefs.resolveSymbols(line);

        StripSpaces(line);

        if (line.length())
        {
            Match _mMatch = findCommand(line);

            // Create new symbol declarations
            if (_mMatch.sString == "declare")
            {
                _symDefs.createSymbol(line.substr(_mMatch.nPos+8));
                continue;
            }

            // Decode the commands
            if (_mMatch.sString == "layout")
            {
                // Start of the layout block
                currentGroup.push(layout->NewElement("layout"));
                layout->InsertFirstChild(currentGroup.top());
                replaceAll(line, "<this>", sFolderName.c_str());

                if (findParameter(line, "size", '='))
                    currentGroup.top()->SetAttribute("size", parseNumOpt(line, findParameter(line, "size", '=')+4).c_str());

                if (findParameter(line, "title", '='))
                    currentGroup.top()->SetAttribute("title", parseStringOpt(line, findParameter(line, "title", '=')+5).c_str());

                if (findParameter(line, "icon", '='))
                    currentGroup.top()->SetAttribute("icon", parseStringOpt(line, findParameter(line, "icon", '=')+4).c_str());

                if (findParameter(line, "color", '='))
                    currentGroup.top()->SetAttribute("color", parseNumOpt(line, findParameter(line, "color", '=')+5).c_str());

                if (findParameter(line, "statustext", '='))
                    currentGroup.top()->SetAttribute("statustext", parseStringOpt(line, findParameter(line, "statustext", '=')+10).c_str());

                if (findParameter(line, "onopen", '='))
                    sOnOpenEvent = parseEventOpt(line, findParameter(line, "onopen", '=')+6, sFolderName);
            }
            else if (_mMatch.sString == "endlayout")
                break;
            else if (_mMatch.sString == "group")
            {
                // Start a new group
                tinyxml2::XMLElement* newgroup = layout->NewElement("group");
                currentGroup.top()->InsertEndChild(newgroup);
                currentGroup.push(newgroup);

                if (findParameter(line, "label", '='))
                    newgroup->SetAttribute("label", parseStringOpt(line, findParameter(line, "label", '=')+5).c_str());

                if (findParameter(line, "type", '='))
                    newgroup->SetAttribute("type", getArgAtPos(line, findParameter(line, "type", '=')+4).c_str());

                if (findParameter(line, "style", '='))
                    newgroup->SetAttribute("style", getArgAtPos(line, findParameter(line, "style", '=')+5).c_str());

                if (findParameter(line, "expand"))
                    newgroup->SetAttribute("expand", "true");

            }
            else if (_mMatch.sString == "endgroup")
            {
                currentGroup.pop();

                if (currentGroup.empty())
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sLayoutScript);
            }
            else if (_mMatch.sString == "prop" || _mMatch.sString == "var" || _mMatch.sString == "str")
            {
                if (currentGroup.empty())
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sLayoutScript, "");

                // Create a the var element
                currentChild = layout->NewElement("prop");
                currentGroup.top()->InsertEndChild(currentChild);
                std::string sLayoutCommand = line.substr(_mMatch.nPos+_mMatch.sString.length());
                std::string sExpr = sLayoutCommand.substr(0, std::min(sLayoutCommand.find("-set"), sLayoutCommand.find("--")));
                replaceAll(sExpr, "<this>", sFolderName.c_str());
                StripSpaces(sExpr);
                currentChild->SetText(sExpr.c_str());
            }
            else
            {
                // All other commands
                if (currentGroup.empty())
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sLayoutScript, "");

                // Create a new element
                currentChild = layout->NewElement(_mMatch.sString.c_str());
                currentGroup.top()->InsertEndChild(currentChild);

                // Parse the parameters and the
                // command expression and insert
                // it
                parseLayoutCommand(line.substr(_mMatch.nPos+_mMatch.sString.length()), currentChild, sFolderName);
            }
        }
    }

    // Nothing usable?
    if (!layout->FirstChild())
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sLayoutScript);

    return sOnOpenEvent;
}


/////////////////////////////////////////////////
/// \brief This static function returns the item
/// id from the user command string. It is also
/// handled if the user erroneously uses "id"
/// instead "item".
///
/// \param sCmd const std::string&
/// \return int
///
/////////////////////////////////////////////////
static int getItemId(const std::string& sCmd)
{
    if (findParameter(sCmd, "item", '='))
        return StrToInt(getArgAtPos(sCmd, findParameter(sCmd, "item", '=')+4, ARGEXTRACT_PARSED | ARGEXTRACT_ASINT | ARGEXTRACT_STRIPPED));
    else if (findParameter(sCmd, "id", '='))
        return StrToInt(getArgAtPos(sCmd, findParameter(sCmd, "id", '=')+2, ARGEXTRACT_PARSED | ARGEXTRACT_ASINT | ARGEXTRACT_STRIPPED));

    return -1;
}


/////////////////////////////////////////////////
/// \brief This static function returns the
/// window information describing the window with
/// the selected ID.
///
/// \param sExpr const std::string&
/// \return NumeRe::WindowInformation
///
/////////////////////////////////////////////////
static NumeRe::WindowInformation getWindow(const std::string& sExpr)
{
    std::string sCurExpr = sExpr;

    if (NumeReKernel::getInstance()->getMemoryManager().containsTablesOrClusters(sCurExpr))
        getDataElements(sCurExpr, NumeReKernel::getInstance()->getParser(), NumeReKernel::getInstance()->getMemoryManager(), NumeReKernel::getInstance()->getSettings());

    NumeReKernel::getInstance()->getParser().SetExpr(sCurExpr);
    int windowID = intCast(NumeReKernel::getInstance()->getParser().Eval());

    return NumeReKernel::getInstance()->getWindowManager().getWindowInformation(windowID);
}


/////////////////////////////////////////////////
/// \brief This static function handles property
/// reads from windows.
///
/// \param cmdParser CommandLineParser&
/// \param sExpr const std::string&
/// \return void
///
/////////////////////////////////////////////////
static void getParametersFromWindow(CommandLineParser& cmdParser, const std::string& sExpr)
{
    const std::string& sParList = cmdParser.getParameterList();

    // Get value of window item
    int itemID = getItemId(sParList);
    NumeRe::WindowInformation winInfo = getWindow(sExpr);

    // If the window does not exist, the pointer
    // is a nullptr type
    if (!winInfo.window || winInfo.nStatus != NumeRe::STATUS_RUNNING)
        throw SyntaxError(SyntaxError::INVALID_WINDOW_ID, cmdParser.getCommandLine(), sExpr);

    if (findParameter(sParList, "value"))
    {
        if (findParameter(sParList, "prop", '='))
        {
            std::string varname = getArgAtPos(sParList, findParameter(sParList, "prop", '=')+4);
            cmdParser.setReturnValue(winInfo.window->getPropValue(varname));
        }
        else
        {
            NumeRe::WinItemValue val = winInfo.window->getItemValue(itemID);

            if (val.type != "tablegrid")
                cmdParser.setReturnValue(val.stringValue);
            else
            {
                MemoryManager& _memManager = NumeReKernel::getInstance()->getMemoryManager();
                Indices _idx;
                std::string sTarget = cmdParser.getTargetTable(_idx, "valtable");

                cmdParser.setReturnValue("\"" + sTarget + "()\"");
                _memManager.importTable(val.tableValue, sTarget, _idx.row, _idx.col);
            }
        }
    }
    else if (findParameter(sParList, "label"))
        cmdParser.setReturnValue(winInfo.window->getItemLabel(itemID));
    else if (findParameter(sParList, "state"))
        cmdParser.setReturnValue("\"" + winInfo.window->getItemState(itemID) + "\"");
    else if (findParameter(sParList, "color"))
        cmdParser.setReturnValue(winInfo.window->getItemColor(itemID));
    else if (findParameter(sParList, "selection"))
        cmdParser.setReturnValue(winInfo.window->getItemSelection(itemID));
    else if (findParameter(sParList, "statustext"))
        cmdParser.setReturnValue(winInfo.window->getStatusText());
    else if (findParameter(sParList, "dialogresult"))
        cmdParser.setReturnValue(winInfo.window->dialog());
}


/////////////////////////////////////////////////
/// \brief This static function handles property
/// writes in windows.
///
/// \param cmdParser CommandLineParser&
/// \param sExpr const std::string&
/// \return void
///
/////////////////////////////////////////////////
static void setParametersInWindow(CommandLineParser& cmdParser, const std::string& sExpr)
{
    const std::string& sParList = cmdParser.getParameterList();

    // Change value of window item
    int itemID = getItemId(sParList);
    NumeRe::WindowInformation winInfo = getWindow(sExpr);

    // If the window does not exist, the pointer
    // is a nullptr type
    if (!winInfo.window || winInfo.nStatus != NumeRe::STATUS_RUNNING)
        throw SyntaxError(SyntaxError::INVALID_WINDOW_ID, cmdParser.getCommandLine(), sExpr);

    // Get the new value
    if (findParameter(sParList, "value", '='))
    {
        if (findParameter(sParList, "prop", '='))
        {
            std::string varname = getArgAtPos(sParList, findParameter(sParList, "prop", '=')+4);
            cmdParser.setReturnValue(toString(winInfo.window->setPropValue(getArgAtPos(sParList, findParameter(sParList, "value", '=')+5, ARGEXTRACT_PARSED), varname)));
        }
        else
        {
            std::string sValue = getArgAtPos(sParList, findParameter(sParList, "value", '=')+5);
            MemoryManager& _memManager = NumeReKernel::getInstance()->getMemoryManager();
            NumeRe::WinItemValue value;

            if (_memManager.containsTables(sValue))
            {
                DataAccessParser _access(sValue);
                value.tableValue = _memManager.extractTable(_access.getDataObject(), _access.getIndices().row, _access.getIndices().col);
                value.stringValue = _access.getDataObject() + "()";
            }
            else
                value.stringValue = cmdParser.getParameterValueAsString("value", "");

            cmdParser.setReturnValue(toString(winInfo.window->setItemValue(value, itemID)));
        }
    }
    else if (findParameter(sParList, "label", '='))
    {
        std::string sLabel = cmdParser.getParameterValueAsString("label", "");
        cmdParser.setReturnValue(toString(winInfo.window->setItemLabel(sLabel, itemID)));
    }
    else if (findParameter(sParList, "state", '='))
    {
        std::string sState = getArgAtPos(sParList, findParameter(sParList, "state", '=')+5);
        cmdParser.setReturnValue(toString(winInfo.window->setItemState(sState, itemID)));
    }
    else if (findParameter(sParList, "display", '='))
    {
        std::string sDisplay = getArgAtPos(sParList, findParameter(sParList, "display", '=')+7);
        cmdParser.setReturnValue(toString(winInfo.window->setDisplay(sDisplay)));
    }
    else if (findParameter(sParList, "color", '='))
    {
        std::string sColor = parseNumOpt(sParList, findParameter(sParList, "color", '=')+5);
        cmdParser.setReturnValue(toString(winInfo.window->setItemColor(sColor, itemID)));
    }
    else if (findParameter(sParList, "selection", '='))
    {
        std::vector<mu::value_type> sel = cmdParser.getParameterValueAsNumericalValue("selection");
        int sel1 = 1, sel2 = 0;

        if (sel.size() > 0)
            sel1 = intCast(sel[0]);

        if (sel.size() > 1)
            sel2 = intCast(sel[1]);

        cmdParser.setReturnValue(toString(winInfo.window->setItemSelection(sel1, sel2, itemID)));
    }
    else if (findParameter(sParList, "focus"))
    {
        cmdParser.setReturnValue(toString(winInfo.window->setItemFocus(itemID)));
    }
    else if (findParameter(sParList, "statustext", '='))
    {
        std::string sStatusText = cmdParser.getParameterValueAsString("statustext", "");
        cmdParser.setReturnValue(toString(winInfo.window->setStatusText(sStatusText)));
    }
}



/////////////////////////////////////////////////
/// \brief This function is the actual
/// implementation of the \c window command.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void windowCommand(CommandLineParser& cmdParser)
{
    NumeRe::WindowManager& winManager = NumeReKernel::getInstance()->getWindowManager();

    // Find the expression part
    std::string sExpr = cmdParser.getExpr();
    std::string sParList = cmdParser.getParameterList();

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sExpr))
    {
        std::string dummy;
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sExpr, dummy, true);
    }

    StripSpaces(sExpr);
    sExpr = removeQuotationMarks(sExpr);

    // Determine, what the user wants to do
    if (findParameter(sParList, "getitems", '='))
    {
        // get IDs of all selected items
        std::string sItemType = getArgAtPos(sParList, findParameter(sParList, "getitems", '=')+8);
        NumeRe::WindowInformation winInfo = getWindow(sExpr);
        Parser& _parser = NumeReKernel::getInstance()->getParser();

        // If the window does not exist, the pointer
        // is a nullptr type
        if (winInfo.window && winInfo.nStatus == NumeRe::STATUS_RUNNING)
        {
            if (sItemType == "prop")
            {
                cmdParser.setReturnValue("{" + winInfo.window->getProperties() + "}");
            }
            else
            {
                std::vector<int> vItems = winInfo.window->getWindowItems(sItemType);

                if (!vItems.size())
                    cmdParser.setReturnValue("nan");
                else
                {
                    std::vector<mu::value_type> vRes;

                    // Convert the ints to doubles
                    for (auto items : vItems)
                        vRes.push_back(items);

                    cmdParser.setReturnValue(_parser.CreateTempVectorVar(vRes));
                }
            }
        }
        else
            throw SyntaxError(SyntaxError::INVALID_WINDOW_ID, cmdParser.getCommandLine(), cmdParser.getExpr());
    }
    else if (findParameter(sParList, "get"))
        getParametersFromWindow(cmdParser, sExpr);
    else if (findParameter(sParList, "set"))
        setParametersInWindow(cmdParser, sExpr);
    else if (findParameter(sParList, "close"))
    {
        // Close window
        NumeRe::WindowInformation winInfo = getWindow(sExpr);

        // If the window does not exist, the pointer
        // is a nullptr type
        if (winInfo.window && winInfo.nStatus == NumeRe::STATUS_RUNNING)
            cmdParser.setReturnValue(toString(winInfo.window->closeWindow()));
        else
            cmdParser.setReturnValue("false");
    }
    else
    {
        // Create new window
        tinyxml2::XMLDocument* layout = new tinyxml2::XMLDocument;
        std::string sOnOpenEvent;

        // parse layout
        try
        {
            sOnOpenEvent = parseLayoutScript(sExpr, layout);
        }
        catch (...)
        {
            delete layout;
            throw;
        }

        // Create the window and return the ID
        int id = winManager.createWindow(layout);

        size_t millisecs = 0;

        while (millisecs < 3000)
        {
            Sleep(100);
            millisecs += 100;

            if (winManager.getWindowInformation(id).window->creationFinished())
                break;
        }

        cmdParser.setReturnValue(toString(id));

        // Create the string for the onopen event
        if (sOnOpenEvent.length())
            cmdParser.setReturnValue(" " + sOnOpenEvent + "(" + toString(id) + ", -1, {\"event\",\"onopen\",\"object\",\"window\",\"value\",nan,\"state\",\"enabled\"})");
    }
}


/////////////////////////////////////////////////
/// \brief Converts a full-qualified procedure
/// name into the corresponding file name.
///
/// \param sProc std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getProcedureFileName(std::string sProc)
{
    // Replace all tilde characters in the current path
    // string. Consider the special namespace "main", which
    // is a reference to the toplevel procedure folder
    for (size_t i = 0; i < sProc.length(); i++)
    {
        if (sProc[i] == '~')
        {
            if (sProc.length() > 5 && i >= 4 && sProc.substr(i - 4, 5) == "main~")
                sProc = sProc.substr(0, i - 4) + sProc.substr(i + 1);
            else
                sProc[i] = '/';
        }
    }

    // Create a valid file name from the procedure name
    sProc = NumeReKernel::getInstance()->getProcedureInterpreter().ValidFileName(sProc, ".nprc");

    return sProc;
}


/////////////////////////////////////////////////
/// \brief Examines a window layout file and
/// searches for all event handler procedures.
/// Returns their corresponding filenames as a
/// vector. Might contain duplicates.
///
/// \param sLayoutFile const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> getEventProcedures(const std::string& sLayoutFile)
{
    StyledTextFile layoutFile(sLayoutFile);
    std::string sFolderName = sLayoutFile.substr(0, sLayoutFile.rfind('/'));
    std::vector<std::string> vProcedures;

    for (size_t i = 0; i < (size_t)layoutFile.getLinesCount(); i++)
    {
        std::string sLine = layoutFile.getStrippedLine(i);

        if (findParameter(sLine, "onopen", '='))
        {
            std::string sEvent = parseEventOpt(sLine, findParameter(sLine, "onopen", '=')+6, sFolderName);

            if (sEvent.front() == '$')
                vProcedures.push_back(getProcedureFileName(sEvent.substr(1)));
        }

        if (findParameter(sLine, "onclick", '='))
        {
            std::string sEvent = parseEventOpt(sLine, findParameter(sLine, "onclick", '=')+7, sFolderName);

            if (sEvent.front() == '$')
                vProcedures.push_back(getProcedureFileName(sEvent.substr(1)));
        }

        if (findParameter(sLine, "onchange", '='))
        {
            std::string sEvent = parseEventOpt(sLine, findParameter(sLine, "onchange", '=')+8, sFolderName);

            if (sEvent.front() == '$')
                vProcedures.push_back(getProcedureFileName(sEvent.substr(1)));
        }
    }

    return vProcedures;
}


/////////////////////////////////////////////////
/// \brief This function is the actual
/// implementation of the \c dialog command.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void dialogCommand(CommandLineParser& cmdParser)
{
    const std::string& sParList = cmdParser.getParameterList();
    std::string sMessage;
    std::string sTitle = "NumeRe: Window";
    std::string sExpression;
    int nControls = NumeRe::CTRL_NONE;
    NumeReKernel* kernel = NumeReKernel::getInstance();

    // Extract the message for the user
    if (findParameter(sParList, "msg", '='))
        sMessage = cmdParser.getParameterValueAsString("msg", "");

    // Extract the window title
    if (findParameter(sParList, "title", '='))
        sTitle = cmdParser.getParameterValueAsString("title", sTitle);

    // Extract the selected dialog type if available, otherwise
    // use the message box as default value
    if (findParameter(sParList, "type", '='))
    {
        std::string sType = getArgAtPos(sParList, findParameter(sParList, "type", '=')+4);

        if (sType == "filedialog")
            nControls = NumeRe::CTRL_FILEDIALOG;
        else if (sType == "dirdialog")
            nControls = NumeRe::CTRL_FOLDERDIALOG;
        else if (sType == "listdialog")
            nControls = NumeRe::CTRL_LISTDIALOG;
        else if (sType == "selectiondialog")
            nControls = NumeRe::CTRL_SELECTIONDIALOG;
        else if (sType == "messagebox")
            nControls = NumeRe::CTRL_MESSAGEBOX;
        else if (sType == "textentry")
            nControls = NumeRe::CTRL_TEXTENTRY;
        else if (sType == "listeditor")
            nControls = NumeRe::CTRL_LISTEDITDIALOG;
    }
    else
        nControls = NumeRe::CTRL_MESSAGEBOX;

    // Extract the button information. The default values are
    // created by wxWidgets. We don't have to do that here
    if (findParameter(sParList, "buttons", '='))
    {
        std::string sButtons = getArgAtPos(sParList, findParameter(sParList, "buttons", '=')+7);

        if (sButtons == "ok")
            nControls |= NumeRe::CTRL_OKBUTTON;
        else if (sButtons == "okcancel")
            nControls |= NumeRe::CTRL_OKBUTTON | NumeRe::CTRL_CANCELBUTTON;
        else if (sButtons == "yesno")
            nControls |= NumeRe::CTRL_YESNOBUTTON;
    }

    // Extract the icon information. The default values are
    // created by wxWidgets. We don't have to do that here
    if (findParameter(sParList, "icon", '='))
    {
        std::string sIcon = getArgAtPos(sParList, findParameter(sParList, "icon", '=')+4);

        if (sIcon == "erroricon")
            nControls |= NumeRe::CTRL_ICONERROR;
        else if (sIcon == "warnicon")
            nControls |= NumeRe::CTRL_ICONWARNING;
        else if (sIcon == "infoicon")
            nControls |= NumeRe::CTRL_ICONINFORMATION;
        else if (sIcon == "questionicon")
            nControls |= NumeRe::CTRL_ICONQUESTION;
    }

    // Strip spaces and assign the value
    sExpression = cmdParser.getExpr();

    // Resolve table accesses
    if (kernel->getMemoryManager().containsTablesOrClusters(sExpression))
        getDataElements(sExpression, kernel->getParser(), kernel->getMemoryManager(), kernel->getSettings());

    // Handle strings in the default value
    // expression. This will include also possible path
    // tokens
    if (kernel->getStringParser().isStringExpression(sExpression))
    {
        std::string sDummy;
        kernel->getStringParser().evalAndFormat(sExpression, sDummy, true);
        sExpression = kernel->getAns().serialize();
    }

    // Ensure that default values are available, if the user
    // selected either a list or a selection dialog
    if ((nControls & NumeRe::CTRL_LISTDIALOG || nControls & NumeRe::CTRL_SELECTIONDIALOG) && (!sExpression.length() || sExpression == "\"\""))
    {
        throw SyntaxError(SyntaxError::NO_DEFAULTVALUE_FOR_DIALOG, cmdParser.getCommandLine(), "dialog");
    }

    // Use the default expression as message for the message
    // box as a fallback solution
    if (nControls & NumeRe::CTRL_MESSAGEBOX && (!sMessage.length() || sMessage == "\"\""))
        sMessage = getNextArgument(sExpression, false);

    // Ensure that the message box has at least a message,
    // because the message box is the default value
    if (nControls & NumeRe::CTRL_MESSAGEBOX && (!sMessage.length() || sMessage == "\"\""))
    {
        throw SyntaxError(SyntaxError::NO_DEFAULTVALUE_FOR_DIALOG, cmdParser.getCommandLine(), "dialog");
    }

    // Ensure that the path for the file and the directory
    // dialog is a valid path and replace all placeholders
    if ((nControls & NumeRe::CTRL_FILEDIALOG || nControls & NumeRe::CTRL_FOLDERDIALOG) && sExpression.length() && sExpression != "\"\"")
    {
        std::string sTemp = sExpression;

        sExpression = "\"" + kernel->getMemoryManager().ValidFolderName(removeQuotationMarks(getNextArgument(sTemp, true))) + "\"";

        if (sTemp.length())
            sExpression += "," + sTemp;
    }

    // Get the window manager, create the modal window and
    // wait until the user interacted with the dialog
    NumeRe::WindowManager& manager = kernel->getWindowManager();
    size_t winid = manager.createWindow(NumeRe::WINDOW_MODAL, NumeRe::WindowSettings(nControls, true, sMessage, sTitle, sExpression));
    NumeRe::WindowInformation wininfo = manager.getWindowInformationModal(winid);

    // Insert the return value as a string into the command
    // line and inform the command handler, that a value
    // has to be evaluated
    if (wininfo.sReturn.find("\\\"") == std::string::npos)
        cmdParser.setReturnValue("\"" + replacePathSeparator(wininfo.sReturn) + "\"");
    else
        cmdParser.setReturnValue("\"" + wininfo.sReturn + "\"");
}





