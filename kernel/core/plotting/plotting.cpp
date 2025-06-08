/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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

#include <wx/image.h>

#include "plotting.hpp"
#include "plotasset.hpp"
#include "../maths/parser_functions.hpp"
#include "../../kernel.hpp"


extern DefaultVariables _defVars;
extern mglGraph _fontData;


std::string removeQuotationMarks(const std::string&);



static bool isPlot1D(StringView sCommand)
{
    return sCommand == "plot" || sCommand == "graph";
}

static bool isPlot3D(StringView sCommand)
{
    return sCommand == "plot3d" || sCommand == "graph3d";
}

static bool isMesh2D(StringView sCommand)
{
    return sCommand == "implot"
        || (!sCommand.ends_with("3d")
            && (sCommand.starts_with("mesh")
                || sCommand.starts_with("surf")
                || sCommand.starts_with("dens")
                || sCommand.starts_with("cont")
                || sCommand.starts_with("grad")));
}

static bool isMesh3D(StringView sCommand)
{
    return sCommand.ends_with("3d")
        && (sCommand.starts_with("mesh")
            || sCommand.starts_with("surf")
            || sCommand.starts_with("dens")
            || sCommand.starts_with("cont")
            || sCommand.starts_with("grad"));
}

static bool isVect2D(StringView sCommand)
{
    return sCommand == "vect" || sCommand == "vector";
}

static bool isVect3D(StringView sCommand)
{
    return sCommand == "vect3d" || sCommand == "vector3d";
}

static bool isDraw(StringView sCommand)
{
    return sCommand == "draw" || sCommand == "draw3d";
}

static bool has3DView(StringView sCommand)
{
    return sCommand == "draw3d"
        || isPlot3D(sCommand)
        || isMesh3D(sCommand)
        || sCommand.starts_with("mesh") // "mesh3d" already handled and included
        || sCommand.starts_with("surf")
        || sCommand.starts_with("cont");
}


// These definitions are for easier understanding of the different ranges
#define APPR_ONE 0.9999999
#define APPR_TWO 1.9999999
#define STYLES_COUNT 20

using namespace std;

/////////////////////////////////////////////////
/// \brief Wrapper function for creating plots.
/// Will create an instance of the Plot class,
/// which will handle the plotting process.
///
/// \param sCmd std::string&
/// \param _data Datafile&
/// \param _parser mu::Parser&
/// \param _option Settings&
/// \param _functions Define&
/// \param _pData PlotData&
/// \return void
///
/////////////////////////////////////////////////
void createPlot(std::string& sCmd, MemoryManager& _data, mu::Parser& _parser, Settings& _option, FunctionDefinitionManager& _functions, PlotData& _pData)
{
    Plot graph(sCmd, _data, _parser, _option, _functions, _pData);

    // Only open graph viewer, if not explicitly deactivated
    if (_pData.getSettings(PlotData::LOG_OPENIMAGE) && !_pData.getSettings(PlotData::LOG_SILENTMODE))
    {
        GraphHelper* _graphHelper = graph.createGraphHelper();

        if (_pData.getTargetGUI()[0] != -1)
        {
            NumeRe::WindowInformation window = NumeReKernel::getInstance()->getWindowManager().getWindowInformation(_pData.getTargetGUI()[0]);

            if (!window.window && window.nStatus != NumeRe::STATUS_RUNNING)
            {
                delete _graphHelper;
                _pData.deleteData(true);
                throw SyntaxError(SyntaxError::INVALID_WINDOW_ID, sCmd, "streamto", "streamto");
            }

            if (!window.window->setItemGraph(_graphHelper, _pData.getTargetGUI()[1]))
            {
                delete _graphHelper;
                _pData.deleteData(true);
                throw SyntaxError(SyntaxError::INVALID_WINDOW_ITEM_ID, sCmd, "streamto", "streamto");
            }
        }
        else
            NumeReKernel::getInstance()->getWindowManager().createWindow(_graphHelper);
    }
    // --> Speicher wieder freigeben <--
    _pData.deleteData(true);
}


/////////////////////////////////////////////////
/// \brief This static function is a fix for the
/// MathGL bug, which connects points outside of
/// the data range.
///
/// \param _mData const mglData&
/// \return mglData
///
/////////////////////////////////////////////////
mglData duplicatePoints(const mglData& _mData)
{
    if (_mData.GetNy() > 1)
    {
        mglData _temp(_mData.GetNx()*2, _mData.GetNy());

        for (int j = 0; j < _mData.GetNy(); j++)
        {
            for (int i = 0; i < _mData.GetNx(); i++)
            {
                _temp.a[2*i + j*_temp.GetNx()] = _mData.a[i+j*_mData.GetNx()];
                _temp.a[2*i+1+j*_temp.GetNx()] = _mData.a[i+j*_mData.GetNx()];
            }
        }

        return _temp;
    }
    else
    {
        mglData _temp(_mData.GetNN()*2);

        for (int i = 0; i < _mData.GetNx(); i++)
        {
            _temp.a[2*i] = _mData.a[i];
            _temp.a[2*i+1] = _mData.a[i];
        }

        return _temp;
    }
}


/////////////////////////////////////////////////
/// \brief This static function uses wxWidgets
/// functionality to add TIFF exporting support
/// to MathGL.
///
/// \param _graph mglGraph*
/// \param sOutputName const string&
/// \return void
///
/////////////////////////////////////////////////
static void writeTiff(mglGraph* _graph, const string& sOutputName)
{
    const unsigned char* bb = _graph->GetRGB();
    int w = _graph->GetWidth();
    int h = _graph->GetHeight();
    unsigned char *tmp = (unsigned char*)malloc(3*w*h);
    memcpy(tmp, bb, 3*w*h);
    wxImage tiffimage(w, h, tmp);

    tiffimage.SaveFile(sOutputName, wxBITMAP_TYPE_TIF);
}


/////////////////////////////////////////////////
/// \brief Plotting object constructor. This
/// class uses "calculate-upon-construction",
/// which means that the constructor has to do
/// all the hard work.
///
/// \param sCmd string&
/// \param __data Datafile&
/// \param __parser Parser&
/// \param __option Settings&
/// \param __functions Define&
/// \param __pData PlotData&
///
/////////////////////////////////////////////////
Plot::Plot(string& sCmd, MemoryManager& __data, Parser& __parser, Settings& __option, FunctionDefinitionManager& __functions, PlotData& __pData)
    : _data(__data), _parser(__parser), _option(__option), _functions(__functions), _pData(__pData)
{
    _graph = new mglGraph(0);
    _graph->SetPenDelta(0.65);
    bOutputDesired = false;             // if a output directly into a file is desired

    _pInfo.sCommand = "";
    _pInfo.sPlotParams = "";
    _pInfo.b2D = false;
    _pInfo.b3D = false;
    _pInfo.b2DVect = false;
    _pInfo.b3DVect = false;
    _pInfo.bDraw = false;
    _pInfo.bDraw3D = false;
    _pInfo.nMaxPlotDim = 1;
    _pInfo.nStyleMax = STYLES_COUNT;                  // Gesamtzahl der Styles
    _pInfo.secranges.intervals.resize(2, Interval(NAN, NAN));

    bool bAnimateVar = false;
    sOutputName.clear();

    vector<string> vPlotCompose;
    size_t nMultiplots[2] = {0, 0};

    size_t nSubPlots = 0;
    size_t nSubPlotMap = 0;


    // Pre-analyze the contents of the passed plotting command. If it
    // contains the "plotcompose" command at start (this will be added
    // by the command handler), then this is either a plot composition
    // or a multiplot.
    //
    // If it is a plotcompose block, then we split it up into the single
    // command lines. If it is a multiplot, then we will add the starting
    // "subplot" command, if it is missing.
    if (findCommand(sCmd).sString == "plotcompose")
    {
        // Remove the "plotcompose" command
        sCmd.erase(findCommand(sCmd).nPos, 11);

        // Search for the multiplot parameter
        if (findParameter(sCmd, "multiplot", '='))
        {
            // Decode the lines and columns of the parameter
            std::string sArg = getArgAtPos(sCmd, findParameter(sCmd, "multiplot", '=') + 9);

            if (sArg.find("<<COMPOSE>>") != std::string::npos)
                sArg.erase(sArg.find("<<COMPOSE>>"));

            _parser.SetExpr(sArg);
            int nRes = 0;
            const mu::StackItem* v = _parser.Eval(nRes);

            // Only use the option value, if it contains two values
            if (nRes == 2)
            {
                nMultiplots[1] = (size_t)v[0].get().getAsScalarInt();
                nMultiplots[0] = (size_t)v[1].get().getAsScalarInt();
            }

            // Remove everything up to the first command in the
            // block
            sCmd.erase(0, sCmd.find("<<COMPOSE>>") + 11);

            // Add the first "subplot", if it missing
            if (findCommand(sCmd).sString != "subplot" && sCmd.find("subplot") != string::npos)
                sCmd.insert(0, "subplot <<COMPOSE>> ");
        }

        StripSpaces(sCmd);

        // Split the block and search for the maximal
        // plot dimension simultaneously
        while (sCmd.length())
        {
            // Search the command
            string __sCMD = findCommand(sCmd).sString;

            // Identify the plotting dimensions
            determinePlottingDimensions(__sCMD);

            // Split the command lines
            vPlotCompose.push_back(sCmd.substr(0, sCmd.find("<<COMPOSE>>")));
            sCmd.erase(0, sCmd.find("<<COMPOSE>>") + 11);
            StripSpaces(sCmd);
        }

        // Gather all parameters from all contained plotting commands
        for (size_t i = 0; i < vPlotCompose.size(); i++)
        {
            if (vPlotCompose[i].find("-set") != string::npos
                    && !isInQuotes(vPlotCompose[i], vPlotCompose[i].find("-set"))
                    && findCommand(vPlotCompose[i]).sString != "subplot")
                _pInfo.sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("-set"));
            else if (vPlotCompose[i].find("--") != string::npos
                     && !isInQuotes(vPlotCompose[i], vPlotCompose[i].find("--"))
                     && findCommand(vPlotCompose[i]).sString != "subplot")
                _pInfo.sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("--"));

            _pInfo.sPlotParams += " ";
        }

        // Apply the gathered parameters. If it is a regular plot composition,
        // then use the "GLOBAL" parameter flag, otherwise use the "SUPERGLOBAL" flag
        if (_pInfo.sPlotParams.length())
        {
            if (nMultiplots[0]) // if this is a multiplot layout, then we will only evaluate the SUPERGLOBAL parameters
                _pData.setParams(_pInfo.sPlotParams, PlotData::SUPERGLOBAL);
            else
                _pData.setGlobalComposeParams(_pInfo.sPlotParams);
        }

        // Clear the parameter set
        _pInfo.sPlotParams = "";
    }
    else
    {
        // Find the plotting command
        string __sCMD = findCommand(sCmd).sString;

        // Identify the maximal plotting dimension
        determinePlottingDimensions(__sCMD);

        // Use the current command as a single "plot composition"
        vPlotCompose.push_back(sCmd);
    }

    // String-Arrays fuer die endgueltigen Styles:
    _pInfo.sLineStyles.resize(_pInfo.nStyleMax);
    _pInfo.sContStyles.resize(_pInfo.nStyleMax);
    _pInfo.sPointStyles.resize(_pInfo.nStyleMax);
    _pInfo.sConPointStyles.resize(_pInfo.nStyleMax);

    // The following statement moves the output cursor to the first postion and
    // cleans the line to avoid overwriting
    if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
        NumeReKernel::printPreFmt("\r");

    size_t nPlotStart = 0;

    // Main loop of the plotting object. This will execute all plotting
    // commands in the current comamnd set clustered into subplots event
    // if only one plotting command is used
    while (nPlotStart < vPlotCompose.size())
    {
        // Only delete the contents of the plot data object
        // if the last command was no "subplot" command, otherwise
        // one would reset all global plotting parameters
        if (nPlotStart)
            _pData.deleteData();

        // If this is a multiplot layout then we need to evaluate the global options for every subplot,
        // because we omitted this set further up. We enter this for each "subplot" command.
        if (vPlotCompose.size() > 1 && nMultiplots[0])
        {
            // Reset the maximal plotting dimension
            _pInfo.nMaxPlotDim = 0;
            _pInfo.nStyle = 0;

            // Gather each plotting parameter until the next "subplot" command or until the end of the
            // whole block
            for (size_t i = nPlotStart; i < vPlotCompose.size(); i++)
            {
                // Leave the loop, if the current command equals "subplot"
                if (findCommand(vPlotCompose[i]).sString == "subplot")
                    break;

                if (vPlotCompose[i].find("-set") != string::npos
                    && !isInQuotes(vPlotCompose[i], vPlotCompose[i].find("-set")))
                    _pInfo.sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("-set"));
                else if (vPlotCompose[i].find("--") != string::npos
                         && !isInQuotes(vPlotCompose[i], vPlotCompose[i].find("--")))
                    _pInfo.sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("--"));

                // Find the maximal plotting dimension of the current subplot
                determinePlottingDimensions(findCommand(vPlotCompose[i]).sString);

                // Append a whitespace for safety reasons
                _pInfo.sPlotParams += " ";
            }

            // Apply the global parameters for this single subplot
            if (_pInfo.sPlotParams.length())
            {
                _pData.setParams(_pInfo.sPlotParams, PlotData::GLOBAL);
                _pInfo.sPlotParams.clear();
            }
        }

        // Create the actual subplot of this plot composition (if only
        // one plotting command is used, the return value will be equal to
        // the size of the plot composition vectore, which is 1)
        nPlotStart = createSubPlotSet(bAnimateVar, vPlotCompose, nPlotStart, nMultiplots, nSubPlots, nSubPlotMap);

        // Perform the reset between single subplots here
        if (nPlotStart < vPlotCompose.size())
        {
            _graph->SetFunc("", "", "");
            _graph->SetOrigin(NAN, NAN, NAN);
            _graph->Alpha(false);
            _graph->Light(false);
        }
    }

    // Save the created plot to the desired file, if the user specifies a file
    // name to write to.
    if (_pData.getSettings(PlotData::LOG_SILENTMODE) || !_pData.getSettings(PlotData::LOG_OPENIMAGE) || bOutputDesired)
    {
        // --> Speichern und Erfolgsmeldung <--
        if (!_pData.getAnimateSamples() || !bAnimateVar)
        {
            if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
                NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("PLOT_SAVING")) + " ... ");

            if (sOutputName.ends_with(".bps"))
            {
                sOutputName[sOutputName.length()-3] = 'e';
                _graph->WriteBPS(sOutputName.c_str());
            }
            else if (sOutputName.ends_with(".tif") || sOutputName.ends_with(".tiff"))
                writeTiff(_graph, sOutputName);
            else if (sOutputName.ends_with(".png"))
                _graph->WritePNG(sOutputName.c_str(), "", false);
            else
                _graph->WriteFrame(sOutputName.c_str());

            // --> TeX-Ausgabe gewaehlt? Dann werden mehrere Dateien erzeugt, die an den Zielort verschoben werden muessen <--
            if (sOutputName.ends_with(".tex"))
                writeTeXMain(sOutputName);

            if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
                NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
        }

        if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
            NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PLOT_SAVE_LOCATION", sOutputName), _option, 0) + "\n");
    }
}


/////////////////////////////////////////////////
/// \brief Plot class destructor.
/////////////////////////////////////////////////
Plot::~Plot()
{
    clearData();

    if (_graph)
        delete _graph;
}


/////////////////////////////////////////////////
/// \brief This static member function detects
/// plotting commands.
///
/// \param command StringView
/// \return bool
///
/////////////////////////////////////////////////
bool Plot::isPlottingCommand(StringView command)
{
    return command.starts_with("plot")
        || command == "subplot"
        || command == "implot"
        || command.starts_with("grad")
        || command.starts_with("graph")
        || command.starts_with("dens")
        || command.starts_with("draw")
        || command.starts_with("vect")
        || command.starts_with("cont")
        || command.starts_with("surf")
        || command.starts_with("mesh");
}


/////////////////////////////////////////////////
/// \brief This member function determines the
/// maximal plotting dimension of the passed
/// command.
///
/// \param sPlotCommand StringView
/// \return void
///
/////////////////////////////////////////////////
void Plot::determinePlottingDimensions(StringView sPlotCommand)
{
    if ((sPlotCommand.starts_with("mesh")
         || sPlotCommand.starts_with("surf")
         || sPlotCommand.starts_with("cont")
         || sPlotCommand.starts_with("vect")
         || sPlotCommand.starts_with("dens")
         || sPlotCommand.starts_with("draw")
         || sPlotCommand.starts_with("grad")
         || sPlotCommand.starts_with("plot"))
        && sPlotCommand.ends_with("3d"))
    {
        _pInfo.nMaxPlotDim = 3;
    }
    else if (sPlotCommand.starts_with("mesh")
             || sPlotCommand.starts_with("surf")
             || sPlotCommand.starts_with("cont"))
    {
        _pInfo.nMaxPlotDim = 3;
    }
    else if (sPlotCommand.starts_with("vect")
             || sPlotCommand.starts_with("dens")
             || sPlotCommand.starts_with("grad")
             || sPlotCommand == "implot")
    {
        if (_pInfo.nMaxPlotDim < 3)
            _pInfo.nMaxPlotDim = 2;
    }
}


/////////////////////////////////////////////////
/// \brief Resolves possible embedded definitions
/// in the draw function's components.
///
/// \param sArgument StringView
/// \param vDrawVector std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
static void resolveDrawVectorArgs(StringView sArgument, std::vector<std::string>& vDrawVector)
{
    sArgument.strip();

    if (sArgument.front() == '{' && sArgument.back() == '}')
    {
        // Resolve vector syntax
        EndlessVector<StringView> args = getAllArguments(sArgument.subview(1, sArgument.length()-2));

        for (StringView& arg : args)
        {
            arg.strip();

            // Remove obsolete surrounding parentheses
            while (arg.front() == '(' && getMatchingParenthesis(arg) == arg.length()-1)
            {
                arg.trim_front(1);
                arg.trim_back(1);
                arg.strip();
            }

            // Handle vector syntax
            if (arg.front() == '{' && arg.back() == '}')
                resolveDrawVectorArgs(arg, vDrawVector);
            else
                vDrawVector.push_back(arg.to_string());
        }
    }
    else
    {
        // Remove obsolete surrounding parentheses
        while (sArgument.front() == '(' && getMatchingParenthesis(sArgument) == sArgument.length()-1)
        {
            sArgument.trim_front(1);
            sArgument.trim_back(1);
            sArgument.strip();
        }

        // Handle vector syntax
        if (sArgument.front() == '{' && sArgument.back() == '}')
            resolveDrawVectorArgs(sArgument, vDrawVector);
        else
            vDrawVector.push_back(sArgument.to_string());
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of a single subplot, which may be a
/// single plot or part of a plot composition.
///
/// \param bAnimateVar bool&
/// \param vPlotCompose vector<string>&
/// \param nSubPlotStart size_t
/// \param nMultiplots[2] size_t
/// \param nSubPlots size_t&
/// \param nSubPlotMap size_t&
/// \return size_t
///
/////////////////////////////////////////////////
size_t Plot::createSubPlotSet(bool& bAnimateVar, vector<string>& vPlotCompose, size_t nSubPlotStart, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap)
{
    vector<string> vDrawVector;
    string sCmd;
    bool bNewSubPlot = nSubPlotStart != 0;

    dataRanges.intervals.resize(3);
    dataRanges.setNames({"x", "y", "z"});
    secDataRanges.intervals.resize(2);
    secDataRanges.setNames({"x", "y"});

    nLegends = 0;

    // This loop will iterate through the plotting commands from
    // the passed starting index until the end or until the command
    // "subplot" has been found
    for (size_t nPlotCompose = nSubPlotStart; nPlotCompose < vPlotCompose.size(); nPlotCompose++)
    {
        m_types.clear();
        sCmd = vPlotCompose[nPlotCompose];
        sCurrentExpr = sCmd;
        _pInfo.sPlotParams = "";

        // Clean the memory, if this is not the first
        // run through this loop
        if (nPlotCompose)
        {
            // Clear allocated memory
            if (m_manager.assets.size())
                m_manager.assets.clear();

            // Only delete the contents of the plot data object
            // if the last command was no "subplot" command, otherwise
            // one would reset all global plotting parameters
            //if (bNewSubPlot)
            //    _pData.deleteData();

            // Reset the plot info object
            _pInfo.b2D = false;
            _pInfo.b3D = false;
            _pInfo.b2DVect = false;
            _pInfo.b3DVect = false;
            _pInfo.bDraw3D = false;
            _pInfo.bDraw = false;
            vDrawVector.clear();

            // Reset the title to avoid double-prints
            //_pData.setTitle("");
        }

        // Find the current plot command
        _pInfo.sCommand = findCommand(sCmd).sString;
        size_t nOffset = findCommand(sCmd).nPos;

        // Extract the "function-and-data" section
        if (sCmd.find("-set") != string::npos && !isInQuotes(sCmd, sCmd.find("-set")))
            sFunc = sCmd.substr(nOffset + _pInfo.sCommand.length(), sCmd.find("-set") - _pInfo.sCommand.length() - nOffset);
        else if (sCmd.find("--") != string::npos && !isInQuotes(sCmd, sCmd.find("--")))
            sFunc = sCmd.substr(nOffset + _pInfo.sCommand.length(), sCmd.find("--") - _pInfo.sCommand.length() - nOffset);
        else
            sFunc = sCmd.substr(nOffset + _pInfo.sCommand.length());

        // --> Unnoetige Leerstellen entfernen <--
        StripSpaces(sFunc);

        // --> Ruf' ggf. den Prompt auf <--
        if (sFunc.find("??") != string::npos)
            sFunc = promptForUserInput(sFunc);

        // Get the plotting parameters for the current command
        if (sCmd.find("-set") != string::npos && !isInQuotes(sCmd, sCmd.find("-set")) && _pInfo.sCommand != "subplot")
            _pInfo.sPlotParams = sCmd.substr(sCmd.find("-set"));
        else if (sCmd.find("--") != string::npos && !isInQuotes(sCmd, sCmd.find("--")) && _pInfo.sCommand != "subplot")
            _pInfo.sPlotParams = sCmd.substr(sCmd.find("--"));

        // If the current command has plotting parameters,
        // evaluate them here. The contained strings are
        // also parsed at this location
        if (_pInfo.sPlotParams.length())
        {
            // Apply the parameters locally (for a plot composition) or globally, if
            // this is a single plot command
            if (vPlotCompose.size() > 1)
                _pData.setLocalComposeParams(_pInfo.sPlotParams);
            else
                _pData.setParams(_pInfo.sPlotParams);
        }

        if (!nPlotCompose)
        {
            // Apply the quality and image dimension settings to the overall result
            // image. This affects also a whole multiplot and is therefore done before
            // the first plot is rendered.
            applyPlotSizeAndQualitySettings();
        }

        // Ensure that the current command line contains a "function-and-data" section
        if (!sFunc.length() && vPlotCompose.size() > 1 && _pInfo.sCommand != "subplot")
            continue;
        else if (!sFunc.length() && _pInfo.sCommand != "subplot")
            throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, sCurrentExpr.length());

        setStyles();

        if (_pData.getSettings(PlotData::FLOAT_BARS) || _pData.getSettings(PlotData::FLOAT_HBARS))
            _graph->SetBarWidth(_pData.getSettings(PlotData::FLOAT_BARS)
                                ? _pData.getSettings(PlotData::FLOAT_BARS)
                                : _pData.getSettings(PlotData::FLOAT_HBARS));

        // Set the number of samples depending on the
        // current plotting command
        if (!_pData.getAnimateSamples()
                || (_pData.getAnimateSamples()
                    && _pInfo.sCommand.substr(0, 4) != "mesh"
                    && _pInfo.sCommand.substr(0, 4) != "surf"
                    && _pInfo.sCommand.substr(0, 4) != "grad"
                    && _pInfo.sCommand.substr(0, 4) != "cont"))
            _pInfo.nSamples = _pData.getSettings(PlotData::INT_SAMPLES);
        else if (_pData.getSettings(PlotData::INT_SAMPLES) > 1000
                 && (_pInfo.sCommand.substr(0, 4) == "mesh"
                     || _pInfo.sCommand.substr(0, 4) == "surf"
                     || _pInfo.sCommand.substr(0, 4) == "grad"
                     || _pInfo.sCommand.substr(0, 4) == "cont"))
            _pInfo.nSamples = 1000;
        else
            _pInfo.nSamples = _pData.getSettings(PlotData::INT_SAMPLES);

        if (_pInfo.nSamples > 151 && _pInfo.b3D)
            _pInfo.nSamples = 151;


        // Create a file name depending on the selected current plot
        // command, if the user did not provide a custom one
        filename(vPlotCompose.size(), nPlotCompose);

        // Get the target output file name and remove the surrounding
        // quotation marks
        if (!nPlotCompose)
        {
            sOutputName = _pData.getSettings(PlotData::STR_FILENAME);
            StripSpaces(sOutputName);

            if (sOutputName[0] == '"' && sOutputName[sOutputName.length() - 1] == '"')
                sOutputName = sOutputName.substr(1, sOutputName.length() - 2);

            if (!nMultiplots[0] && !nMultiplots[1])
                _graph->SubPlot(1, 1, 0, _pData.getSettings(PlotData::STR_PLOTBOUNDARIES).c_str());
        }

        // This section contains the logic, which will determine the position
        // of subplots in a multiplot layout. We'll only enter this section, if the
        // current command equals "subplot" and continue with the next plot
        // command afterwards
        if (findCommand(sCmd).sString == "subplot" && nMultiplots[0] && nMultiplots[1])
        {
            // This function closes the current graph (puts the legend)
            // and determines the position of the next subplot in the
            // plotting layout
            evaluateSubplot(sCmd, nMultiplots, nSubPlots, nSubPlotMap);
            nSubPlots++;

            // Return the position of the next plotting command
            return nPlotCompose+1;
        }
        else if (findCommand(sCmd).sString == "subplot")
            continue; // Ignore the "subplot" command, if we have no multiplot layout

        // Display the "Calculating data for SOME PLOT" message
        displayMessage(_pData.getAnimateSamples() && findVariableInExpression(sFunc, "t") != std::string::npos);

        // Apply the logic and the transformation for logarithmic
        // plotting axes
        if (_pData.getSettings(PlotData::INT_COORDS) == CARTESIAN
                && (nPlotCompose == nSubPlotStart)
                && (_pData.getLogscale(XRANGE) || _pData.getLogscale(YRANGE) || _pData.getLogscale(ZRANGE) || _pData.getLogscale(CRANGE)))
        {
            setLogScale((_pInfo.b2D || _pInfo.sCommand == "plot3d"));
        }

        // Prepare the legend strings
        if (!_pInfo.bDraw3D && !_pInfo.bDraw)
        {
            // Add the legends to the function-and-data section
            if (!addLegends(sFunc))
                return vPlotCompose.size(); // --> Bei Fehlern: Zurueck zur aufrufenden Funktion <--
        }

        // Replace the custom defined functions with their definition
        if (!_functions.call(sFunc))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCurrentExpr, SyntaxError::invalid_position);

        // Call the input prompt, if one of the function definition requires this
        if (sFunc.find("??") != string::npos)
            sFunc = promptForUserInput(sFunc);

        // Ensure that we set the variable "t" to a reasonable value to avoid
        // problems within the detection of indices
        if (_pData.getAnimateSamples() && findVariableInExpression(sFunc, "t") != std::string::npos)
        {
            // set t to the starting value
            _defVars.vValue[TCOORD][0] = _pData.getRanges()[TRANGE].front();  // Plotparameter: t
        }

        // Split the function-and-data section into functions and data sets,
        // evaluate the indices of the data sets and store the values of the
        // data sets into the mglData objects, which will be used further down
        // to display the data values in the plots
        std::vector<std::string> vDataPlots;

        if (!isDraw(_pInfo.sCommand))
            vDataPlots = separateFunctionsAndData();

        // Do we need to search for the animation parameter variable?
        if (_pData.getAnimateSamples())
        {
            for (std::string sDataPlots : vDataPlots)
            {
                if (findVariableInExpression(sDataPlots, "t") != std::string::npos)
                {
                    bAnimateVar = true;
                    // set t to the starting value
                    _defVars.vValue[TCOORD][0] = _pData.getRanges()[TRANGE].front();  // Plotparameter: t
                    break;
                }
            }
        }

        // Prepare the plotting memory for functions and datasets depending
        // on their number
        prepareMemory();

        // Reset the internal data ranges for each new subplot (otherwise
        // they'll be combined into an ever-growing interval)
        if (bNewSubPlot)
        {
            for (size_t i = 0; i < dataRanges.size(); i++)
            {
                dataRanges[i].reset(NAN, NAN);
            }
            for (size_t i = 0; i < secDataRanges.size(); i++)
            {
                secDataRanges[i].reset(NAN, NAN);
            }
        }

        // Get now the data values for the plot
        extractDataValues(vDataPlots);

        StripSpaces(sFunc);

        // Ensure that either functions or data plots are available
        if (!m_manager.assets.size() && !isDraw(_pInfo.sCommand))
            throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, sCurrentExpr.find(' '));

        // Ensure that the functions do not contain any strings, because strings
        // cannot be plotted
        if (containsStrings(sFunc) && !(_pInfo.bDraw3D || _pInfo.bDraw))
        {
            clearData();
            throw SyntaxError(SyntaxError::CANNOT_PLOT_STRINGS, sCurrentExpr, SyntaxError::invalid_position);
        }

        // Determine, whether the function string is empty. If it is and the current
        // plotting style is not a drawing style, then assign the function string to
        // the parser. Otherwise split the function string into the single functions.
        if (isNotEmptyExpression(sFunc) && !(_pInfo.bDraw3D || _pInfo.bDraw))
        {
            try
            {
                _parser.SetExpr(sFunc);
                _parser.Eval(_pInfo.nFunctions);

                // Search for the animation time variable "t"
                if (findVariableInExpression(sFunc, "t"))
                    bAnimateVar = true;

                // Check, whether the number of functions correspond to special
                // plotting styles
                if ((_pData.getSettings(PlotData::LOG_COLORMASK) || _pData.getSettings(PlotData::LOG_ALPHAMASK))
                    && (_pInfo.b2D || (_pInfo.sCommand == "plot3d" && _pData.getSettings(PlotData::INT_MARKS)))
                    && m_types.size() % 2)
                    throw SyntaxError(SyntaxError::NUMBER_OF_FUNCTIONS_NOT_MATCHING, sCurrentExpr, sCurrentExpr.find(' '));
            }
            catch (...)
            {
                clearData();
                throw;
            }
        }
        else if (isNotEmptyExpression(sFunc) && (_pInfo.bDraw3D || _pInfo.bDraw))
        {
            string sArgument;
            // Append a whitespace
            sFunc += " ";

            // Split the function string into the single functions
            // and store them in the drawing vector
            while (sFunc.length())
            {
                sArgument = getNextArgument(sFunc, true);
                StripSpaces(sArgument);

                if (!sArgument.length())
                    continue;

                // Try to detect the occurence of the animation variable
                if (findVariableInExpression(sArgument, "t") != string::npos)
                    bAnimateVar = true;

                resolveDrawVectorArgs(sArgument, vDrawVector);
            }

            // Set the function string to be empty
            sFunc.clear();
        }
        else
            sFunc.clear();

        // Calculate the default plotting ranges depending on the user
        // specification. If the user didn't specify
        // a set of ranges, we try to use the plotting ranges of the data
        // points and omit negative numbers, if the corresponding axis was
        // set to logarithmic scale.
        defaultRanges(nPlotCompose, bNewSubPlot);

        // Set the plot calculation variables (x, y and z) to their starting
        // points
        _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE].front();  // Plotvariable: x
        _defVars.vValue[YCOORD][0] = _pInfo.ranges[YRANGE].front();  // Plotvariable: y
        _defVars.vValue[ZCOORD][0] = _pInfo.ranges[ZRANGE].front();  // Plotvariable: z

        // For the "plot3d" style or the animated plot we need the time coordinate.
        // Set it to their corresponding starting value
        if (_pInfo.sCommand == "plot3d" || _pData.getAnimateSamples())
            _defVars.vValue[TCOORD][0] = _pInfo.ranges[TRANGE].front();  // Plotparameter: t

        // Now create the plot or the animation using the provided and
        // pre-calculated data. This function will also open and
        // close the GIF, if the animation directly saved to a file
        createPlotOrAnimation(nPlotCompose, vPlotCompose.size(), bNewSubPlot, bAnimateVar, vDrawVector, vDataPlots);

        bNewSubPlot = false;

        if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");
    }

    // Return the lenght of the plotcompostion array, which
    // will signal the calling function that we're finished
    // with plotting and that the control can be returned to
    // the kernel.
    return vPlotCompose.size();
}


/////////////////////////////////////////////////
/// \brief This member function determines output
/// size and quality on the plotting target and
/// the plotting parameters.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::applyPlotSizeAndQualitySettings()
{
    double factor = NAN;

    // Apply the quality and image dimension settings to the overall result
    // image. This affects also a whole multiplot and is therefore done before
    // the first plot is rendered.
    // If the user requests a specific size, we'll preferably using this
    if (_pData.getSettings(PlotData::INT_SIZE_X) > 0 && _pData.getSettings(PlotData::INT_SIZE_Y) > 0)
    {
        nHeight = _pData.getSettings(PlotData::INT_SIZE_Y);
        nWidth = _pData.getSettings(PlotData::INT_SIZE_X);
    }
    else if (_pData.getSettings(PlotData::LOG_SILENTMODE) || !_pData.getSettings(PlotData::LOG_OPENIMAGE))
    {
        // Switch between fullHD and the normal resolution
        if (_pData.getSettings(PlotData::INT_HIGHRESLEVEL) == 2)
            factor = 1920.0 * 1440.0;
        else
            factor = 1280.0 * 960.0;
    }
    else
    {
        // This section is for printing to the GraphViewer window
        // we use different resolutions here
        if (_pData.getAnimateSamples() && !_pData.getSettings(PlotData::STR_FILENAME).length())
            factor = 640.0 * 480.0; // Animation size (faster for rendering)
        else if (_pData.getSettings(PlotData::INT_HIGHRESLEVEL) == 2)
            factor = 1280.0 * 960.0; // Hires output
        else
            factor = 800.0 * 600.0; // Standard output
    }

    if (!isnan(factor))
    {
        factor = sqrt(factor / _pData.getSettings(PlotData::FLOAT_ASPECT));
        nHeight = rint(factor);
        nWidth = rint(_pData.getSettings(PlotData::FLOAT_ASPECT)*factor);
    }

    _graph->SetSize(nWidth, nHeight);

    // Copy the font and select the font size
    _graph->CopyFont(&_fontData);
    _graph->SetFontSizeCM(0.22 * (1.0 + _pData.getSettings(PlotData::FLOAT_TEXTSIZE)) / 6.0, 72);
    _graph->SetFlagAdv(1, MGL_FULL_CURV);
}


/////////////////////////////////////////////////
/// \brief This member function creates the plot
/// or animation selected by the plotting command.
///
/// \param nPlotCompose size_t
/// \param nPlotComposeSize size_t
/// \param bNewSubPlot bool
/// \param bAnimateVar bool
/// \param vDrawVector vector<string>&
/// \param sDataAxisBinds const string&
/// \return bool
///
/// It will also create the samples for the
/// animation, which may be target either
/// directly to a file (aka GIF) or to the
/// GraphViewer, which will finalize the
/// rendering step.
/////////////////////////////////////////////////
bool Plot::createPlotOrAnimation(size_t nPlotCompose, size_t nPlotComposeSize, bool bNewSubPlot, bool bAnimateVar, vector<string>& vDrawVector, const vector<string>& vDataPlots)
{
    mglData _mBackground;

    // If the animation is saved to a file, set the frame time
    // for the GIF image
    if (_pData.getAnimateSamples() && bOutputDesired)
        _graph->StartGIF(sOutputName.c_str(), 40); // 40msec = 2sec bei 50 Frames, d.h. 25 Bilder je Sekunde

    // Load the background image from the target file and apply the
    // black/white color scheme
    if (_pData.getSettings(PlotData::STR_BACKGROUND).length() && _pData.getSettings(PlotData::STR_BACKGROUNDCOLORSCHEME) != "<<REALISTIC>>")
    {
        if (_pData.getAnimateSamples() && _option.systemPrints())
            NumeReKernel::printPreFmt("|-> ");

        if (_option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_LOADING_BACKGROUND")) + " ... ");

        _mBackground.Import(_pData.getSettings(PlotData::STR_BACKGROUND).c_str(), "kw");

        if (_pData.getAnimateSamples() && _option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");
    }

    // This is the start of the actual plotting algorithm. The previous
    // lines were only preparations for this algorithm. We create either a
    // single plot or a set of frames for an animation
    for (int t_animate = 0; t_animate <= _pData.getAnimateSamples(); t_animate++)
    {
        // If it is an animation, then we're required to reset the plotting
        // variables for each frame. Additionally, we have to start a new
        // frame at this location.
        if (_pData.getAnimateSamples() && !_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && bAnimateVar)
        {
            NumeReKernel::printPreFmt("\r|-> " + toSystemCodePage(_lang.get("PLOT_RENDERING_FRAME", toString(t_animate + 1), toString(_pData.getAnimateSamples() + 1))) + " ... ");
            _pInfo.nStyle = 0;

            // Prepare a new frame
            _graph->NewFrame();

            // Reset the plotting variables (x, y and z) to their initial values
            // and increment the time variable one step further
            if (t_animate)
            {
                _defVars.vValue[TCOORD][0] = _pInfo.ranges[TRANGE](t_animate, _pData.getAnimateSamples());

                // Recalculate the data plots
                extractDataValues(vDataPlots);

                defaultRanges(nPlotCompose, true);

                _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE].front();
                _defVars.vValue[YCOORD][0] = _pInfo.ranges[YRANGE].front();
                _defVars.vValue[ZCOORD][0] = _pInfo.ranges[ZRANGE].front();
            }
        }

        double dt_max = _defVars.vValue[TCOORD][0].front().getNum().asF64();

        // Apply the title line to the graph
        if (_pData.getSettings(PlotData::STR_PLOTTITLE).length())
        {
            _graph->Title(fromSystemCodePage(_pData.getSettings(PlotData::STR_PLOTTITLE)).c_str(), "", -1.5);
            if (!_pData.getAnimateSamples())
                _pData.setTitle("");
        }

        // If the user requested an orthogonal projection, then
        // we activate this plotting mode at this location
        if (_pData.getSettings(PlotData::LOG_ORTHOPROJECT) && has3DView(_pInfo.sCommand))
        {
            _graph->Ternary(4);
            _graph->SetRotatedText(false);
        }

        // Rotate the graph to the desired plotting angles
        if (_pInfo.nMaxPlotDim > 2 && (!nPlotCompose || bNewSubPlot))
            _graph->Rotate(_pData.getRotateAngle(), _pData.getRotateAngle(1));

        // Calculate the function values for the set plotting ranges
        // and the eventually set time variable
        fillData(dt_max, t_animate);

        // Normalize the plotting results, if the current plotting
        // style is a vector field
        if (_pInfo.b2DVect || _pInfo.b3DVect)
            m_manager.normalize(t_animate);

        // Change the plotting ranges to fit to the calculated plot
        // data (only, if the user did not specify those ranges, too).
        fitPlotRanges(nPlotCompose, bNewSubPlot);

        // Pass the final ranges to the graph. Only do this, if this is
        // the first plot of a plot composition or a new subplot.
        if (!nPlotCompose || bNewSubPlot)
            passRangesToGraph();

        // Apply a color bar, if desired and supplied by the plotting style
        applyColorbar();

        // Apply the light effect, if desired and supplied by the plotting stype
        applyLighting();

        // Activate the perspective effect
        if ((!nPlotCompose || bNewSubPlot) && _pInfo.nMaxPlotDim > 2)
            _graph->Perspective(_pData.getSettings(PlotData::FLOAT_PERSPECTIVE));

        // Render the background image
        if (_pData.getSettings(PlotData::STR_BACKGROUND).length())
        {
            if (_pData.getSettings(PlotData::STR_BACKGROUNDCOLORSCHEME) != "<<REALISTIC>>")
            {
                _graph->SetRanges(_pInfo.ranges[XRANGE].min(), _pInfo.ranges[XRANGE].max(),
                                  _pInfo.ranges[YRANGE].min(), _pInfo.ranges[YRANGE].max(),
                                  _mBackground.Minimal(), _mBackground.Maximal());
                _graph->Dens(_mBackground, _pData.getSettings(PlotData::STR_BACKGROUNDCOLORSCHEME).c_str());
            }
            else
                _graph->Logo(_pData.getSettings(PlotData::STR_BACKGROUND).c_str());

            _graph->Rasterize();
            _graph->SetRanges(_pInfo.ranges[XRANGE].min(), _pInfo.ranges[XRANGE].max(),
                              _pInfo.ranges[YRANGE].min(), _pInfo.ranges[YRANGE].max(),
                              _pInfo.ranges[ZRANGE].min(), _pInfo.ranges[ZRANGE].max());
        }

        // This section will transform the calculated data points into the desired
        // plotting style. The most complex plots are the default plot, the "plot3d"
        // and the 2D-Plots (mesh, surf, etc.)
        if (isMesh2D(_pInfo.sCommand))        // 2D-Plot
            create2dPlot(nPlotCompose, nPlotComposeSize);
        else if (isPlot1D(_pInfo.sCommand))      // Standardplot
            createStdPlot(nPlotCompose, nPlotComposeSize);
        else if (isMesh3D(_pInfo.sCommand))   // 3D-Plot
            create3dPlot();
        else if (isVect3D(_pInfo.sCommand))   // 3D-Vektorplot
            create3dVect();
        else if (isVect2D(_pInfo.sCommand))   // 2D-Vektorplot
            create2dVect();
        else if (_pInfo.bDraw)
            create2dDrawing(vDrawVector);
        else if (_pInfo.bDraw3D)
            create3dDrawing(vDrawVector);
        else            // 3D-Trajektorie
            createStd3dPlot(nPlotCompose, nPlotComposeSize);

        // Finalize the GIF frame
        if (_pData.getAnimateSamples() && bAnimateVar)
            _graph->EndFrame();

        // If no animation was selected or the
        // animation variable ("t") is missing,
        // leave the loop at this position
        if (!bAnimateVar)
            break;
    }

    // Finalize the GIF completely
    if (_pData.getAnimateSamples() && bAnimateVar && bOutputDesired)
        _graph->CloseGIF();

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function wraps the
/// creation of all all two-dimensional plots
/// (e.g. surface or density plots).
///
/// \param nPlotCompose size_t
/// \param nPlotComposeSize size_t
/// \return void
///
/////////////////////////////////////////////////
void Plot::create2dPlot(size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    bool useImag = false;
    bool isComplexPlaneMode = _pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE;
    mglData _mData(_pInfo.nSamples, _pInfo.nSamples);
    mglData _mData2;
    mglData _mPlotAxes[2];
    mglData _mContVec(_pData.getSettings(PlotData::INT_CONTLINES));

    for (int nCont = 0; nCont < _pData.getSettings(PlotData::INT_CONTLINES); nCont++)
    {
        _mContVec.a[nCont] = _pInfo.ranges[ZRANGE](nCont, _pData.getSettings(PlotData::INT_CONTLINES)).real();
    }

    if (_pData.getSettings(PlotData::INT_CONTLINES) % 2)
        _mContVec.a[_pData.getSettings(PlotData::INT_CONTLINES)/2] = _pInfo.ranges[ZRANGE].middle();

    // Resize the matrices to fit in the created image
    m_manager.resize(nWidth, nHeight);

    // Apply curvilinear coordinates
    if (!_pData.getSettings(PlotData::LOG_PARAMETRIC))
        m_manager.applyCoordSys((CoordinateSystem)_pData.getSettings(PlotData::INT_COORDS),
                                (_pData.getSettings(PlotData::LOG_COLORMASK) || _pData.getSettings(PlotData::LOG_ALPHAMASK)) ? 2 : 1);

    for (int n = 0; n < (int)m_manager.assets.size(); n++)
    {
        int nDataOffset = 0;

        if (_pData.getSettings(PlotData::LOG_PARAMETRIC) && (int)m_manager.assets.size() > n+2)
        {
            _mPlotAxes[XCOORD].Link(m_manager.assets[n].data[0].first);
            _mPlotAxes[YCOORD].Link(m_manager.assets[n+1].data[0].first);
            nDataOffset = 2;
        }
        else
        {
            _mPlotAxes[XCOORD].Link(m_manager.assets[n].axes[XCOORD]);
            _mPlotAxes[YCOORD].Link(m_manager.assets[n].axes[YCOORD]);
        }

        StripSpaces(m_manager.assets[n+nDataOffset].legend);

        if (isComplexPlaneMode)
            _mData = m_manager.assets[n+nDataOffset].norm(0);
        else
            _mData.Link(useImag ? m_manager.assets[n+nDataOffset].data[0].second : m_manager.assets[n+nDataOffset].data[0].first);

        if ((_pData.getSettings(PlotData::LOG_COLORMASK) || _pData.getSettings(PlotData::LOG_ALPHAMASK))
            && n+nDataOffset+1 < (int)m_manager.assets.size())
            _mData2.Link(useImag ? m_manager.assets[n+nDataOffset+1].data[0].second : m_manager.assets[n+nDataOffset+1].data[0].first);
        else if (isComplexPlaneMode)
            _mData2 = m_manager.assets[n+nDataOffset].arg(0);
        else if (_pData.getSettings(PlotData::LOG_COLORMASK) || _pData.getSettings(PlotData::LOG_ALPHAMASK))
            _mData2.Create(_pInfo.nSamples, _pInfo.nSamples);

        // Create the actual 2D plot
        if (!plot2d(_mData, _mData2, _mPlotAxes, _mContVec))
        {
            clearData();
            return;
        }

        // Do only present legend entries, if there are more than two
        // data sets or more than two composed data sets to be displayed
        if ((int)m_manager.assets.size() > 2+nDataOffset
            || _pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM
            || ((int)m_manager.assets.size() >= 2+nDataOffset && !(_pData.getSettings(PlotData::LOG_COLORMASK) || _pData.getSettings(PlotData::LOG_ALPHAMASK))))
        {
            if (_pData.getSettings(PlotData::LOG_CONTLABELS) && _pInfo.sCommand.substr(0, 4) != "cont")
                _graph->Cont(_mPlotAxes[0], _mPlotAxes[1], _mData,
                             ("t" + _pInfo.sContStyles[_pInfo.nStyle]).c_str(), ("val " + toString(_pData.getSettings(PlotData::INT_CONTLINES))).c_str());
            else if (!_pData.getSettings(PlotData::LOG_CONTLABELS) && _pInfo.sCommand.substr(0, 4) != "cont")
                _graph->Cont(_mPlotAxes[0], _mPlotAxes[1], _mData,
                             _pInfo.sContStyles[_pInfo.nStyle].c_str(), ("val " + toString(_pData.getSettings(PlotData::INT_CONTLINES))).c_str());

            sConvLegends = m_manager.assets[n+nDataOffset].legend;
            getDataElements(sConvLegends, _parser, _data);
            _parser.SetExpr(sConvLegends);
            sConvLegends = _parser.Eval().printVals();

            if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM && sConvLegends.length())
                sConvLegends = useImag ? "Im(" + sConvLegends + ")" : "Re(" + sConvLegends + ")";

            sConvLegends = "\"" + sConvLegends + "\"";

            if (sConvLegends != "\"\"")
            {
                _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(),
                                  _pInfo.sContStyles[_pInfo.nStyle].c_str());
                nLegends++;
            }

            _pInfo.nStyle = _pInfo.nextStyle();
        }

        if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM)
        {
            if (!useImag)
            {
                useImag = true;
                n--;
            }
            else
            {
                useImag = false;
                n += nDataOffset;
            }
        }
        else
            n += nDataOffset;

        if ((_pData.getSettings(PlotData::LOG_COLORMASK) || _pData.getSettings(PlotData::LOG_ALPHAMASK))
            && n+1 < (int)m_manager.assets.size()
            && (useImag || _pData.getSettings(PlotData::INT_COMPLEXMODE) != CPLX_REIM))
            n++;
    }

    if (nLegends && !_pData.getSettings(PlotData::LOG_SCHEMATIC) && nPlotCompose + 1 == nPlotComposeSize)
        _graph->Legend(1.35, 1.2);
}


/////////////////////////////////////////////////
/// \brief This member function creates an actual
/// single two-dimensional plot based upon the
/// current plotting command.
///
/// \param _mData mglData&
/// \param _mData2 mglData&
/// \param _mAxisVals mglData*
/// \param _mContVec mglData&
/// \return bool
///
/////////////////////////////////////////////////
bool Plot::plot2d(mglData& _mData, mglData& _mData2, mglData* _mAxisVals, mglData& _mContVec)
{
    if (_pData.getSettings(PlotData::LOG_CUTBOX)
            && _pInfo.sCommand.substr(0, 4) != "cont"
            && _pInfo.sCommand.substr(0, 4) != "grad"
            && _pInfo.sCommand.substr(0, 4) != "dens"
            && _pInfo.sCommand != "implot")
        _graph->SetCutBox(CalcCutBox(_pData.getRotateAngle(1), 0, _pData.getSettings(PlotData::INT_COORDS)),
                          CalcCutBox(_pData.getRotateAngle(1), 1, _pData.getSettings(PlotData::INT_COORDS)));

    // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
    if (_pInfo.sCommand.substr(0, 4) == "mesh")
    {
        if (_pData.getSettings(PlotData::FLOAT_BARS))
            _graph->Boxs(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("#").c_str());
        else
            _graph->Mesh(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else if (_pInfo.sCommand.substr(0, 4) == "surf")
    {
        if (_pData.getSettings(PlotData::FLOAT_BARS))
            _graph->Boxs(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        else if (_pData.getSettings(PlotData::LOG_COLORMASK) || _pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE)
            _graph->SurfC(_mAxisVals[0], _mAxisVals[1], _mData, _mData2, _pData.getColorScheme().c_str());
        else if (_pData.getSettings(PlotData::LOG_ALPHAMASK))
            _graph->SurfA(_mAxisVals[0], _mAxisVals[1], _mData, _mData2, _pData.getColorScheme().c_str());
        else
            _graph->Surf(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else if (_pInfo.sCommand.substr(0, 4) == "cont")
    {
        if (_pData.getSettings(PlotData::LOG_CONTLABELS))
        {
            if (_pData.getSettings(PlotData::LOG_CONTFILLED))
                _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());

            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("t").c_str());
        }
        else if (_pData.getSettings(PlotData::LOG_CONTPROJ))
        {
            if (_pData.getSettings(PlotData::LOG_CONTFILLED))
            {
                _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
                _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
            }
            else
                _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
        }
        else if (_pData.getSettings(PlotData::LOG_CONTFILLED))
        {
            _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "k");
        }
        else
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else if (_pInfo.sCommand.substr(0, 4) == "grad")
    {
        if (_pData.getSettings(PlotData::INT_HIGHRESLEVEL) || !_option.isDraftMode())
        {
            if (_pData.getSettings(PlotData::LOG_CONTFILLED) && _pData.getSettings(PlotData::LOG_CONTPROJ))
                _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeMedium().c_str(), "value 10");
            else
                _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str(), "value 10");
        }
        else
        {
            if (_pData.getSettings(PlotData::LOG_CONTFILLED) && _pData.getSettings(PlotData::LOG_CONTPROJ))
                _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeMedium().c_str());
            else
                _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        }

        if (!(_pData.getSettings(PlotData::LOG_CONTFILLED) && _pData.getSettings(PlotData::LOG_CONTPROJ)))
            _graph->Dens(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeLight().c_str());
    }
    else if (_pInfo.sCommand.substr(0, 4) == "dens" || _pInfo.sCommand == "implot")
    {
        if (_pData.getSettings(PlotData::FLOAT_BARS))
            _graph->Tile(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        else if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE)
        {
            // Density plot from -PI to PI
            _graph->Dens(_mAxisVals[0], _mAxisVals[1], _mData2, _pData.getColorScheme().c_str());

            // Contour lines from ZRANGE-min to ZRANGE-max
            _graph->SetRange('c', _pInfo.ranges[ZRANGE].min(), _pInfo.ranges[ZRANGE].max());
            _graph->SetTicksVal('c', "");
            _graph->Colorbar(_pData.getSettings(PlotData::LOG_BOX) ? "kw^I" : "kw^");
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "kw");
        }
        else
            _graph->Dens(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else
        return false;

    if (_pData.getSettings(PlotData::LOG_CUTBOX)
            && _pInfo.sCommand.substr(0, 4) != "cont"
            && _pInfo.sCommand.substr(0, 4) != "grad"
            && _pInfo.sCommand.substr(0, 4) != "dens")
        _graph->SetCutBox(mglPoint(0), mglPoint(0));

    // --> Ggf. Konturlinien ergaenzen <--
    if (_pData.getSettings(PlotData::LOG_CONTPROJ) && _pInfo.sCommand.substr(0, 4) != "cont")
    {
        if (_pData.getSettings(PlotData::LOG_CONTFILLED) && _pInfo.sCommand.substr(0, 4) != "dens")
        {
            _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
        }
        else if (_pInfo.sCommand.substr(0, 4) == "dens" && _pData.getSettings(PlotData::LOG_CONTFILLED))
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
        else
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
    }

    if (_pData.getSettings(PlotData::LOG_CONTLABELS) && _pInfo.sCommand.substr(0, 4) != "cont" && _pInfo.nFunctions == 1)
    {
        _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, ("t" + _pInfo.sContStyles[_pInfo.nStyle]).c_str());

        if (_pData.getSettings(PlotData::LOG_CONTFILLED))
            _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Simple helper function for
/// Plot::createStdPlot.
///
/// \param _mData const mglData&
/// \param ivl const Interval&
/// \param secIvl const Interval&
/// \return mglData
///
/////////////////////////////////////////////////
static mglData scaleSecondaryToPrimaryInterval(const mglData& _mData, const Interval& ivl, const Interval& secIvl)
{
    return (_mData - secIvl.min()) * ivl.range() / secIvl.range() + ivl.min();
}


/////////////////////////////////////////////////
/// \brief This member function wraps the
/// creation of all one-dimensional plots
/// (e.g. line or point plots).
///
/// \param nPlotCompose size_t
/// \param nPlotComposeSize size_t
/// \return void
///
/////////////////////////////////////////////////
void Plot::createStdPlot(size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    bool useImag = false;
    bool isCplxPlaneMode = _pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE;

    mglData _mData;
    mglData _mPlotAxes;
    mglData _mData2[3];

    int nPrevDataLayers = 0;
    int nCurrentStyle = 0;

    // Apply curvilinear coordinates
    m_manager.applyCoordSys((CoordinateSystem)_pData.getSettings(PlotData::INT_COORDS));

    for (int n = 0; n < (int)m_manager.assets.size(); n++)
    {
        int nDataOffset = 0;

        if (_pData.getSettings(PlotData::LOG_PARAMETRIC) && n+1 < (int)m_manager.assets.size() && !isCplxPlaneMode)
        {
            _mPlotAxes.Link(m_manager.assets[n].data[0].first);
            nDataOffset = 1;
        }
        else
            _mPlotAxes.Link(isCplxPlaneMode ? m_manager.assets[n].data[0].first : m_manager.assets[n].axes[XCOORD]);

        StripSpaces(m_manager.assets[n+nDataOffset].legend);

        _mData.Link(useImag || isCplxPlaneMode ? m_manager.assets[n+nDataOffset].data[0].second : m_manager.assets[n+nDataOffset].data[0].first);

        if (_pData.getSettings(PlotData::LOG_REGION)
            && n+nDataOffset+1 < (int)m_manager.assets.size())
            _mData2[0].Link(useImag || isCplxPlaneMode ? m_manager.assets[n+nDataOffset+1].data[0].second : m_manager.assets[n+nDataOffset+1].data[0].first);
        else
            _mData2[0] = 0.0 * _mData;

        // Copy the data to the relevant memory
        if (m_manager.assets[n+nDataOffset].type == PT_FUNCTION)
        {
            if (m_manager.assets[n+nDataOffset].boundAxes.find('r') != std::string::npos)
                _mData = scaleSecondaryToPrimaryInterval(_mData, _pInfo.ranges[YRANGE], _pInfo.secranges[YRANGE]);

            if (_pData.getSettings(PlotData::LOG_REGION)
                && n+nDataOffset+1 < (int)m_manager.assets.size()
                && m_manager.assets[n+nDataOffset+1].boundAxes.find('r') != std::string::npos)
                _mData2[0] = scaleSecondaryToPrimaryInterval(_mData2[0], _pInfo.ranges[YRANGE], _pInfo.secranges[YRANGE]);
        }
        else
        {
            if (_pData.getSettings(PlotData::LOG_BOXPLOT))
            {
                // Create the boxplot axis (count of elements has to be one element larger)
                _mData2[0].Create(m_manager.assets[n+nDataOffset].getLayers()+1);

                for (size_t col = 0; col < m_manager.assets[n+nDataOffset].getLayers()+1; col++)
                {
                    // Create the axis by adding previous layers to shift
                    // the current box plots to the right
                    _mData2[0].a[col] = nPrevDataLayers + 0.5 + col;
                }

                nPrevDataLayers += m_manager.assets[n+nDataOffset].getLayers();
                _mData = m_manager.assets[n+nDataOffset].vectorsToMatrix();
                _mData.Transpose();
            }
            else if (_pData.getSettings(PlotData::LOG_OHLC) || _pData.getSettings(PlotData::LOG_CANDLESTICK))
            {
                _mData.Link(m_manager.assets[n+nDataOffset].data[0].first);
                _mData2[0].Link(m_manager.assets[n+nDataOffset].data[1].first);
                _mData2[1].Link(m_manager.assets[n+nDataOffset].data[2].first);
                _mData2[2].Link(m_manager.assets[n+nDataOffset].data[3].first);
            }
            else if (_pData.getSettings(PlotData::FLOAT_BARS)
                     || _pData.getSettings(PlotData::FLOAT_HBARS))
                _mData = m_manager.assets[n+nDataOffset].vectorsToMatrix();
            else
            {
                if (_pData.getSettings(PlotData::LOG_INTERPOLATE) && getNN(_mData) >= _pData.getSettings(PlotData::INT_SAMPLES))
                {
                    if (!_pData.getSettings(PlotData::LOG_REGION))
                    {
                        // If the current data dimension is higher than 2, then we
                        // expand it into an array of curves
                        if (m_manager.assets[n+nDataOffset].getLayers() > 1)
                            _mData = m_manager.assets[n+nDataOffset].vectorsToMatrix();

                        // Create an zero-error data set
                        _mData2[0] = 0.0 * _mData;
                    }
                }
                else if (_pData.getSettings(PlotData::LOG_YERROR)
                         || _pData.getSettings(PlotData::LOG_XERROR))
                {
                    size_t layer = 1;

                    if (_pData.getSettings(PlotData::LOG_XERROR) && m_manager.assets[n+nDataOffset].getLayers() > layer)
                    {
                        _mData2[XCOORD].Link(m_manager.assets[n+nDataOffset].data[layer].first);
                        layer++;
                    }
                    else
                        _mData2[XCOORD] = 0.0 * _mData;

                    if (_pData.getSettings(PlotData::LOG_YERROR) && m_manager.assets[n+nDataOffset].getLayers() > layer)
                        _mData2[YCOORD].Link(m_manager.assets[n+nDataOffset].data[layer].first);
                    else
                        _mData2[YCOORD] = 0.0 * _mData;
                }
                else if (m_manager.assets[n+nDataOffset].getLayers() > 1)
                    _mData = m_manager.assets[n+nDataOffset].vectorsToMatrix();
            }

            if (m_manager.assets[n+nDataOffset].boundAxes.find('t') != std::string::npos)
            {
                for (int i = 0; i < getNN(_mPlotAxes); i++)
                {
                    if (!_pInfo.secranges[XRANGE].isInside(_mPlotAxes.a[i]))
                        _mPlotAxes.a[i] = NAN;
                }

                _mPlotAxes = scaleSecondaryToPrimaryInterval(_mPlotAxes, _pInfo.ranges[XRANGE], _pInfo.secranges[XRANGE]);
            }
            else
            {
                for (int i = 0; i < getNN(_mPlotAxes); i++)
                {
                    if (!_pInfo.ranges[XRANGE].isInside(_mPlotAxes.a[i]))
                        _mPlotAxes.a[i] = NAN;
                }
            }

            if (m_manager.assets[n+nDataOffset].boundAxes.find('r') != std::string::npos)
                _mData = scaleSecondaryToPrimaryInterval(_mData, _pInfo.ranges[YRANGE], _pInfo.secranges[YRANGE]);

            if (_pData.getSettings(PlotData::LOG_REGION)
                && n+nDataOffset+1 < (int)m_manager.assets.size()
                && m_manager.assets[n+nDataOffset+1].boundAxes.find('r') != std::string::npos && getNN(_mData2[0]) > 1)
                _mData2[0] = scaleSecondaryToPrimaryInterval(_mData2[0], _pInfo.ranges[YRANGE], _pInfo.secranges[YRANGE]);

            if (_pData.getSettings(PlotData::LOG_YERROR) && m_manager.assets[n+nDataOffset].boundAxes.find('r') != std::string::npos)
                _mData2[1] = scaleSecondaryToPrimaryInterval(_mData2[1], _pInfo.ranges[YRANGE], _pInfo.secranges[YRANGE]);

            if (_pData.getSettings(PlotData::LOG_XERROR) && m_manager.assets[n+nDataOffset].boundAxes.find('t') != std::string::npos)
                _mData2[0] = scaleSecondaryToPrimaryInterval(_mData2[0], _pInfo.ranges[XRANGE], _pInfo.secranges[XRANGE]);
        }

        // Store the current style
        nCurrentStyle = _pInfo.nStyle;

        // Create the plot
        if (!plotstd(_mData, _mPlotAxes, _mData2, m_manager.assets[n+nDataOffset].type))
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            clearData();
            return;
        }

        // Increment the style counter
        _pInfo.nStyle = _pInfo.nextStyle();

        // Create the legend
        if (_pData.getSettings(PlotData::LOG_REGION) && getNN(_mData2[0]) > 1 && (int)m_manager.assets.size() > n+nDataOffset+1)
            sConvLegends = "\"" + removeQuotationMarks(m_manager.assets[n+nDataOffset].legend) + "\n"
                            + removeQuotationMarks(m_manager.assets[n+nDataOffset+1].legend) + "\"";
        else
            sConvLegends = m_manager.assets[n+nDataOffset].legend;

        getDataElements(sConvLegends, _parser, _data);
        _parser.SetExpr(sConvLegends);
        sConvLegends = _parser.Eval().printVals();

        // While the legend string is not empty
        while (sConvLegends.length())
        {
            // Get the next legend string
            string sLegend = sConvLegends.substr(0, sConvLegends.find('\n'));

            if (sConvLegends.find('\n') != string::npos)
                sConvLegends.erase(0, sConvLegends.find('\n')+1);
            else
                sConvLegends.clear();

            if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM && sLegend.length())
                sLegend = useImag ? "Im(" + sLegend + ")" : "Re(" + sLegend + ")";

            // Add the legend
            if (sLegend.length())
            {
                std::string sLegendStyle;

                if (m_manager.assets[n+nDataOffset].type == PT_FUNCTION)
                    sLegendStyle = getLegendStyle(_pInfo.sLineStyles[nCurrentStyle]);
                else
                {
                    if (_pData.getSettings(PlotData::LOG_BOXPLOT))
                        sLegendStyle = getLegendStyle(_pInfo.sLineStyles[nCurrentStyle]);
                    else if (!_pData.getSettings(PlotData::LOG_XERROR)
                             && !_pData.getSettings(PlotData::LOG_YERROR)
                             && !_pData.getSettings(PlotData::LOG_OHLC)
                             && !_pData.getSettings(PlotData::LOG_CANDLESTICK))
                    {
                        if ((_pData.getSettings(PlotData::LOG_INTERPOLATE) && m_manager.assets[n+nDataOffset].axes[0].nx >= _pInfo.nSamples)
                            || _pData.getSettings(PlotData::FLOAT_BARS)
                            || _pData.getSettings(PlotData::FLOAT_HBARS))
                            sLegendStyle = getLegendStyle(_pInfo.sLineStyles[nCurrentStyle]);
                        else if (_pData.getSettings(PlotData::LOG_CONNECTPOINTS)
                                 || (_pData.getSettings(PlotData::LOG_INTERPOLATE) && m_manager.assets[n+nDataOffset].axes[0].nx >= 0.9 * _pInfo.nSamples))
                            sLegendStyle = getLegendStyle(_pInfo.sConPointStyles[nCurrentStyle]);
                        else if (_pData.getSettings(PlotData::LOG_STEPPLOT))
                            sLegendStyle = getLegendStyle(_pInfo.sLineStyles[nCurrentStyle]);
                        else
                            sLegendStyle = getLegendStyle(_pInfo.sPointStyles[nCurrentStyle]);
                    }
                    else
                        sLegendStyle = getLegendStyle(_pInfo.sPointStyles[nCurrentStyle]);
                }

                nLegends++;
                _graph->AddLegend(fromSystemCodePage(replaceToTeX(sLegend)).c_str(), sLegendStyle.c_str());
            }

            if (nCurrentStyle == _pInfo.nStyleMax - 1)
                nCurrentStyle = 0;
            else
                nCurrentStyle++;
        }

        if (_pData.getSettings(PlotData::LOG_REGION)
            && n+nDataOffset+1 < (int)m_manager.assets.size()
            && (useImag || _pData.getSettings(PlotData::INT_COMPLEXMODE) != CPLX_REIM))
            n++;

        if ((getNN(_mData2[0])
             && _pData.getSettings(PlotData::LOG_REGION)
             && n+nDataOffset+1 < (int)m_manager.assets.size())
            || _pData.getSettings(PlotData::LOG_OHLC)
            || _pData.getSettings(PlotData::LOG_CANDLESTICK))
            _pInfo.nStyle = _pInfo.nextStyle();

        if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM)
        {
            if (!useImag)
            {
                useImag = true;
                n--;
            }
            else
            {
                useImag = false;
                n += nDataOffset;
            }
        }
        else
            n += nDataOffset;
    }

    double AutoRangeX = _pData.getLogscale(XRANGE) ? log10(_pInfo.ranges[XRANGE].range()) : _pInfo.ranges[XRANGE].range();
    double AutoRangeY = _pData.getLogscale(YRANGE) ? log10(_pInfo.ranges[YRANGE].range()) : _pInfo.ranges[YRANGE].range();

    for (const Line& line : _pData.getHLines())
    {
        if (!line.sDesc.length())
            continue;

        double xpos = _pInfo.ranges[XRANGE].min() + 0.03 * AutoRangeX;
        double ypos = line.dPos + 0.01 * AutoRangeY;

        _graph->Line(mglPoint(_pInfo.ranges[XRANGE].min(), line.dPos), mglPoint(_pInfo.ranges[XRANGE].max(), line.dPos), line.sStyle.c_str(), 100);
        _graph->Puts(xpos, ypos, fromSystemCodePage(line.sDesc).c_str(), ":kL");
    }

    for (const Line& line : _pData.getVLines())
    {
        if (!line.sDesc.length())
            continue;

        mglPoint textpos = mglPoint(line.dPos - 0.01 * AutoRangeX,
                                    _pInfo.ranges[YRANGE].min() + 0.05 * AutoRangeY);
        mglPoint textdir = mglPoint(line.dPos - 0.01 * AutoRangeX,
                                    _pInfo.ranges[YRANGE].max());

        _graph->Line(mglPoint(line.dPos, _pInfo.ranges[YRANGE].min()), mglPoint(line.dPos, _pInfo.ranges[YRANGE].max()), line.sStyle.c_str());
        _graph->Puts(textpos, textdir, fromSystemCodePage(line.sDesc).c_str(), ":kL");
    }

    if (nLegends && !_pData.getSettings(PlotData::LOG_SCHEMATIC) && nPlotCompose + 1 == nPlotComposeSize)
    {
        _graph->SetMarkSize(1.0);
        _graph->Legend(_pData.getSettings(PlotData::INT_LEGENDPOSITION));
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates an actual
/// single one-dimensional plot based upon the
/// current plotting command.
///
/// \param _mData mglData&
/// \param _mAxisVals mglData&
/// \param _mData2[3] mglData
/// \param nType const short
/// \return bool
///
/////////////////////////////////////////////////
bool Plot::plotstd(mglData& _mData, mglData& _mAxisVals, mglData _mData2[3], const short nType)
{
#warning NOTE (numere#3#08/15/21): Temporary fix for MathGL misbehaviour
    if (!_pData.getSettings(PlotData::LOG_BOXPLOT)
        && !_pData.getSettings(PlotData::LOG_YERROR)
        && !_pData.getSettings(PlotData::LOG_XERROR)
        && !_pData.getSettings(PlotData::LOG_OHLC)
        && !_pData.getSettings(PlotData::LOG_CANDLESTICK)
        && !_pData.getSettings(PlotData::FLOAT_BARS)
        && !_pData.getSettings(PlotData::FLOAT_HBARS)
        && !_pData.getSettings(PlotData::LOG_STEPPLOT)
        && !_pData.getSettings(PlotData::LOG_TABLE))
    {
        _mData = duplicatePoints(_mData);
        _mAxisVals = duplicatePoints(_mAxisVals);
        _mData2[0] = duplicatePoints(_mData2[0]);
        _mData2[1] = duplicatePoints(_mData2[1]);
        _mData2[2] = duplicatePoints(_mData2[1]);
    }

    int nNextStyle = _pInfo.nextStyle();
    std::string sAreaGradient = std::string("a") + _pData.getColors()[_pInfo.nStyle] + "{" + _pData.getColors()[_pInfo.nStyle] + "9}";

    if (isdigit(_pInfo.sLineStyles[_pInfo.nStyle].back()))
        _graph->SetMarkSize((_pInfo.sLineStyles[_pInfo.nStyle].back() - '0') * 0.5 + 0.5);

    if (nType == PT_FUNCTION)
    {
        if (_pData.getSettings(PlotData::LOG_REGION) && getNN(_mData2[0]) > 1) // Default region plot
        {
            _graph->Region(_mAxisVals, _mData, _mData2[0],
                           ("a{" + _pData.getColors().substr(_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(nNextStyle, 1) + "7}").c_str());
            _graph->Plot(_mAxisVals, _mData,
                         ("a" + _pInfo.sLineStyles[_pInfo.nStyle]).c_str());
            _graph->Plot(_mAxisVals, _mData2[0],
                         ("a" + _pInfo.sLineStyles[nNextStyle]).c_str());
        }
        else if (_pData.getSettings(PlotData::LOG_AREA) || _pData.getSettings(PlotData::LOG_REGION)) // Fallback for region with only single plot
            _graph->Area(_mAxisVals, _mData, sAreaGradient.c_str());
        else
            _graph->Plot(_mAxisVals, _mData, ("a" + _pInfo.sLineStyles[_pInfo.nStyle]).c_str());
    }
    else
    {
        if (_pData.getSettings(PlotData::LOG_BOXPLOT))
            _graph->BoxPlot(_mData2[0], _mData,
                            _pInfo.sLineStyles[_pInfo.nStyle].c_str());
        else if (_pData.getSettings(PlotData::LOG_OHLC))
            _graph->OHLC(_mAxisVals, _mData, _mData2[0], _mData2[1], _mData2[2],
                         (_pInfo.sLineStyles[_pInfo.nStyle]+_pInfo.sLineStyles[nNextStyle]).c_str());
        else if (_pData.getSettings(PlotData::LOG_CANDLESTICK))
            _graph->Candle(_mAxisVals, _mData2[2], _mData, _mData2[1], _mData2[0],
                           (_pData.getColors().substr(_pInfo.nStyle, 1)+_pData.getColors().substr(nNextStyle, 1)).c_str());
        else if (!_pData.getSettings(PlotData::LOG_XERROR) && !_pData.getSettings(PlotData::LOG_YERROR))
        {
            if (_pData.getSettings(PlotData::LOG_INTERPOLATE) && countValidElements(_mData) >= (size_t)_pInfo.nSamples)
            {
                if (_pData.getSettings(PlotData::FLOAT_BARS))
                    _graph->Bars(_mAxisVals, _mData,
                                 ("F" + composeColoursForBarChart(_mData.ny)).c_str());
                else if (_pData.getSettings(PlotData::LOG_REGION) && getNN(_mData2[0]) > 1)
                {
                    _graph->Region(_mAxisVals, _mData, _mData2[0],
                                   ("a{" + _pData.getColors().substr(_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(nNextStyle, 1) + "7}").c_str());
                    _graph->Plot(_mAxisVals, _mData,
                                 ("a" + _pInfo.sLineStyles[_pInfo.nStyle]).c_str());
                    _graph->Plot(_mAxisVals, _mData2[0],
                                 ("a" + _pInfo.sLineStyles[nNextStyle]).c_str());
                }
                else if (_pData.getSettings(PlotData::LOG_AREA) || _pData.getSettings(PlotData::LOG_REGION))
                    _graph->Area(_mAxisVals, _mData, sAreaGradient.c_str());
                else if (_pData.getSettings(PlotData::LOG_STEPPLOT))
                    _graph->Step(_mAxisVals, _mData,
                                 (_pInfo.sLineStyles[_pInfo.nStyle]).c_str());
                else
                    _graph->Plot(_mAxisVals, _mData,
                                 ("a" + expandStyleForCurveArray(_pInfo.sLineStyles[_pInfo.nStyle], _mData.ny > 1)).c_str());
            }
            else if (_pData.getSettings(PlotData::LOG_CONNECTPOINTS)
                     || (_pData.getSettings(PlotData::LOG_INTERPOLATE) && countValidElements(_mData) >= 0.9 * _pInfo.nSamples))
            {
                if (_pData.getSettings(PlotData::FLOAT_BARS))
                    _graph->Bars(_mAxisVals, _mData,
                                 ("F" + composeColoursForBarChart(_mData.ny)).c_str());
                else if (_pData.getSettings(PlotData::LOG_STEPPLOT))
                    _graph->Step(_mAxisVals, _mData,
                                 (_pInfo.sLineStyles[_pInfo.nStyle]).c_str());
                else if (_pData.getSettings(PlotData::LOG_AREA))
                    _graph->Area(_mAxisVals, _mData, sAreaGradient.c_str());
                else
                    _graph->Plot(_mAxisVals, _mData,
                                 ("a" + expandStyleForCurveArray(_pInfo.sConPointStyles[_pInfo.nStyle], _mData.ny > 1)).c_str());
            }
            else
            {

                if (_pData.getSettings(PlotData::FLOAT_BARS))
                    _graph->Bars(_mAxisVals, _mData,
                                 ("F" + composeColoursForBarChart(_mData.ny)).c_str());
                else if (_pData.getSettings(PlotData::FLOAT_HBARS))
                    _graph->Barh(_mAxisVals, _mData,
                                 ("F" + composeColoursForBarChart(_mData.ny)).c_str());
                else if (_pData.getSettings(PlotData::LOG_STEPPLOT))
                    _graph->Step(_mAxisVals, _mData,
                                 (_pInfo.sLineStyles[_pInfo.nStyle]).c_str());
                else if (_pData.getSettings(PlotData::LOG_AREA))
                    _graph->Stem(_mAxisVals, _mData,
                                 _pInfo.sConPointStyles[_pInfo.nStyle].c_str());
                else
                    _graph->Plot(_mAxisVals, _mData,
                                 ("a" + expandStyleForCurveArray(_pInfo.sPointStyles[_pInfo.nStyle], _mData.ny > 1)).c_str());
            }
        }
        else
            _graph->Error(_mAxisVals, _mData, _mData2[0], _mData2[1],
                          _pInfo.sPointStyles[_pInfo.nStyle].c_str());
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of all three-dimensional plots.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::create3dPlot()
{
    static const std::string slicesStyleChars[3] = {"x", "", "z"};

    if (sFunc.length())
    {
        mglData _mContVec(15);
        IntervalSet dataInterval = m_manager.assets[0].getDataIntervals();
        m_manager.weightedRange(ALLRANGES, dataInterval[0]);

        // Apply curvilinear coordinates
        m_manager.applyCoordSys((CoordinateSystem)_pData.getSettings(PlotData::INT_COORDS));

        if (!isnan(_pInfo.ranges[CRANGE].front()))
            dataInterval[0] = _pInfo.ranges[CRANGE];

        for (int nCont = 0; nCont < 15; nCont++)
        {
            _mContVec.a[nCont] = dataInterval[0](nCont, 15).real();
        }

        _mContVec.a[7] = dataInterval[0].middle();

        mglData& _mData = m_manager.assets[0].data[0].first;

        if (_pData.getSettings(PlotData::LOG_CUTBOX)
                && _pInfo.sCommand.substr(0, 4) != "cont"
                && _pInfo.sCommand.substr(0, 4) != "grad"
                && (_pInfo.sCommand.substr(0, 4) != "dens" || (_pInfo.sCommand.substr(0, 4) == "dens" && _pData.getSettings(PlotData::LOG_CLOUDPLOT))))
        {
            _graph->SetCutBox(CalcCutBox(_pData.getRotateAngle(1), 0, _pData.getSettings(PlotData::INT_COORDS), true),
                              CalcCutBox(_pData.getRotateAngle(1), 1, _pData.getSettings(PlotData::INT_COORDS), true));
        }

        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
        if (_pInfo.sCommand.substr(0, 4) == "mesh")
        {
            _graph->Surf3(m_manager.assets[0].axes[XCOORD],
                          m_manager.assets[0].axes[YCOORD],
                          m_manager.assets[0].axes[ZCOORD],
                          _mData,
                          _pData.getColorScheme("#").c_str(),
                          "value 11");

            if (m_manager.assets[0].isComplex(0))
                _graph->Surf3(m_manager.assets[0].axes[XCOORD],
                              m_manager.assets[0].axes[YCOORD],
                              m_manager.assets[0].axes[ZCOORD],
                              m_manager.assets[0].data[0].second,
                              _pData.getColorScheme("#").c_str(),
                              "value 11");
        }
        else if (_pInfo.sCommand.substr(0, 4) == "surf" && !_pData.getSettings(PlotData::LOG_ALPHA))
        {
            _graph->Surf3(m_manager.assets[0].axes[XCOORD],
                          m_manager.assets[0].axes[YCOORD],
                          m_manager.assets[0].axes[ZCOORD],
                          _mData,
                          _pData.getColorScheme().c_str(),
                          "value 11");

            if (m_manager.assets[0].isComplex(0))
                _graph->Surf3(m_manager.assets[0].axes[XCOORD],
                              m_manager.assets[0].axes[YCOORD],
                              m_manager.assets[0].axes[ZCOORD],
                              m_manager.assets[0].data[0].second,
                              _pData.getColorScheme().c_str(),
                              "value 11");
        }
        else if (_pInfo.sCommand.substr(0, 4) == "surf" && _pData.getSettings(PlotData::LOG_ALPHA))
        {
            _graph->Surf3A(m_manager.assets[0].axes[XCOORD],
                           m_manager.assets[0].axes[YCOORD],
                           m_manager.assets[0].axes[ZCOORD],
                           _mData,
                           _mData,
                           _pData.getColorScheme().c_str(),
                           "value 11");

            if (m_manager.assets[0].isComplex(0))
                _graph->Surf3A(m_manager.assets[0].axes[XCOORD],
                               m_manager.assets[0].axes[YCOORD],
                               m_manager.assets[0].axes[ZCOORD],
                               m_manager.assets[0].data[0].second,
                               m_manager.assets[0].data[0].second,
                               _pData.getColorScheme().c_str(),
                               "value 11");
        }
        else if (_pInfo.sCommand.substr(0, 4) == "cont")
        {
            if (_pData.getSettings(PlotData::LOG_CONTPROJ))
            {
                if (_pData.getSettings(PlotData::LOG_CONTFILLED))
                {
                    _graph->ContFX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                    _graph->ContFY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1),1));
                    _graph->ContFZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.ranges[ZRANGE].min());
                    _graph->ContX(_mContVec, _mData.Sum("x"), "k", getProjBackground(_pData.getRotateAngle(1)));
                    _graph->ContY(_mContVec, _mData.Sum("y"), "k", getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.ranges[ZRANGE].min());
                }
                else
                {
                    _graph->ContX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                    _graph->ContY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->ContZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.ranges[ZRANGE].min());
                }
            }
            else if (_pData.getSettings(PlotData::LOG_CONTFILLED))
            {
                for (int i = XCOORD; i <= ZCOORD; i++)
                {
                    for (unsigned short n = 0; n < _pData.getSlices(i); n++)
                    {
                        _graph->ContF3(_mContVec,
                                       m_manager.assets[0].axes[XCOORD],
                                       m_manager.assets[0].axes[YCOORD],
                                       m_manager.assets[0].axes[ZCOORD],
                                       _mData,
                                       _pData.getColorScheme(slicesStyleChars[i]).c_str(),
                                       (n + 1)*_pInfo.nSamples / (_pData.getSlices(i) + 1));
                        _graph->Cont3(_mContVec,
                                      m_manager.assets[0].axes[XCOORD],
                                      m_manager.assets[0].axes[YCOORD],
                                      m_manager.assets[0].axes[ZCOORD],
                                      _mData,
                                      ("k" + slicesStyleChars[i]).c_str(),
                                      (n + 1)*_pInfo.nSamples / (_pData.getSlices() + 1));
                    }
                }
            }
            else
            {
                for (int i = XCOORD; i <= ZCOORD; i++)
                {
                    for (unsigned short n = 0; n < _pData.getSlices(i); n++)
                        _graph->Cont3(_mContVec,
                                      m_manager.assets[0].axes[XCOORD],
                                      m_manager.assets[0].axes[YCOORD],
                                      m_manager.assets[0].axes[ZCOORD],
                                      _mData,
                                      _pData.getColorScheme(slicesStyleChars[i]).c_str(),
                                      (n + 1)*_pInfo.nSamples / (_pData.getSlices(i) + 1));
                }
            }
        }
        else if (_pInfo.sCommand.substr(0, 4) == "grad")
        {
            if (_pData.getSettings(PlotData::INT_HIGHRESLEVEL) || !_option.isDraftMode())
            {
                if (_pData.getSettings(PlotData::LOG_CONTFILLED) && _pData.getSettings(PlotData::LOG_CONTPROJ))
                    _graph->Grad(m_manager.assets[0].axes[XCOORD],
                                 m_manager.assets[0].axes[YCOORD],
                                 m_manager.assets[0].axes[ZCOORD],
                                 _mData,
                                 _pData.getColorSchemeMedium().c_str(),
                                 "value 10");
                else
                    _graph->Grad(m_manager.assets[0].axes[XCOORD],
                                 m_manager.assets[0].axes[YCOORD],
                                 m_manager.assets[0].axes[ZCOORD],
                                 _mData,
                                 _pData.getColorScheme().c_str(),
                                 "value 10");
            }
            else
            {
                if (_pData.getSettings(PlotData::LOG_CONTFILLED) && _pData.getSettings(PlotData::LOG_CONTPROJ))
                    _graph->Grad(m_manager.assets[0].axes[XCOORD],
                                 m_manager.assets[0].axes[YCOORD],
                                 m_manager.assets[0].axes[ZCOORD],
                                 _mData,
                                 _pData.getColorSchemeMedium().c_str());
                else
                    _graph->Grad(m_manager.assets[0].axes[XCOORD],
                                 m_manager.assets[0].axes[YCOORD],
                                 m_manager.assets[0].axes[ZCOORD],
                                 _mData,
                                 _pData.getColorScheme().c_str());
            }

            if (!(_pData.getSettings(PlotData::LOG_CONTPROJ)))
            {
                for (int i = XCOORD; i <= ZCOORD; i++)
                {
                    _graph->Dens3(m_manager.assets[0].axes[XCOORD],
                                  m_manager.assets[0].axes[YCOORD],
                                  m_manager.assets[0].axes[ZCOORD],
                                  _mData,
                                  _pData.getColorSchemeLight(slicesStyleChars[i]).c_str(),
                                  _pInfo.nSamples / 2);
                }
            }
        }
        else if (_pInfo.sCommand.substr(0, 4) == "dens")
        {
            if (!(_pData.getSettings(PlotData::LOG_CONTFILLED) && _pData.getSettings(PlotData::LOG_CONTPROJ))
                && !_pData.getSettings(PlotData::LOG_CLOUDPLOT))
            {
                for (int i = XCOORD; i <= ZCOORD; i++)
                {
                    for (unsigned short n = 0; n < _pData.getSlices(i); n++)
                        _graph->Dens3(m_manager.assets[0].axes[XCOORD],
                                      m_manager.assets[0].axes[YCOORD],
                                      m_manager.assets[0].axes[ZCOORD],
                                      _mData,
                                      _pData.getColorScheme(slicesStyleChars[i]).c_str(),
                                      (n + 1)*_pInfo.nSamples / (_pData.getSlices(i) + 1));
                }
            }
            else if (_pData.getSettings(PlotData::LOG_CLOUDPLOT) && !(_pData.getSettings(PlotData::LOG_CONTFILLED)
                                                                      && _pData.getSettings(PlotData::LOG_CONTPROJ)))
                _graph->Cloud(m_manager.assets[0].axes[XCOORD],
                              m_manager.assets[0].axes[YCOORD],
                              m_manager.assets[0].axes[ZCOORD],
                              _mData,
                              _pData.getColorScheme().c_str());
        }
        else
        {
            clearData();
            return;
        }

        if (_pData.getSettings(PlotData::LOG_CUTBOX)
            && _pInfo.sCommand.substr(0, 4) != "cont"
            && _pInfo.sCommand.substr(0, 4) != "grad"
            && (_pInfo.sCommand.substr(0, 4) != "dens" || (_pInfo.sCommand.substr(0, 4) == "dens" && _pData.getSettings(PlotData::LOG_CLOUDPLOT))))
            _graph->SetCutBox(mglPoint(0), mglPoint(0));

        // --> Ggf. Konturlinien ergaenzen <--
        if (_pData.getSettings(PlotData::LOG_CONTPROJ) && _pInfo.sCommand.substr(0, 4) != "cont")
        {
            if (_pData.getSettings(PlotData::LOG_CONTFILLED)
                && (_pInfo.sCommand.substr(0, 4) != "dens" && _pInfo.sCommand.substr(0, 4) != "grad"))
            {
                _graph->ContFX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                _graph->ContFY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                _graph->ContFZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.ranges[ZRANGE].min());
                _graph->ContX(_mContVec, _mData.Sum("x"), "k", getProjBackground(_pData.getRotateAngle(1)));
                _graph->ContY(_mContVec, _mData.Sum("y"), "k", getProjBackground(_pData.getRotateAngle(1), 1));
                _graph->ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.ranges[ZRANGE].min());
            }
            else if ((_pInfo.sCommand.substr(0, 4) == "dens" || _pInfo.sCommand.substr(0, 4) == "grad")
                     && _pData.getSettings(PlotData::LOG_CONTFILLED))
            {
                if (_pInfo.sCommand == "dens")
                {
                    _graph->DensX(_mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                    _graph->DensY(_mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->DensZ(_mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.ranges[ZRANGE].min());
                }
                else
                {
                    _graph->DensX(_mData.Sum("x"), _pData.getColorSchemeLight().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                    _graph->DensY(_mData.Sum("y"), _pData.getColorSchemeLight().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->DensZ(_mData.Sum("z"), _pData.getColorSchemeLight().c_str(), _pInfo.ranges[ZRANGE].min());
                }

                _graph->ContX(_mContVec, _mData.Sum("x"), "k", getProjBackground(_pData.getRotateAngle(1)));
                _graph->ContY(_mContVec, _mData.Sum("y"), "k", getProjBackground(_pData.getRotateAngle(1), 1));
                _graph->ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.ranges[ZRANGE].min());
            }
            else
            {
                _graph->ContX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                _graph->ContY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                _graph->ContZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.ranges[ZRANGE].min());
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of all three-dimensional vector
/// fields.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::create3dVect()
{
    if (sFunc.length())
    {
        if (_pData.getSettings(PlotData::LOG_CUTBOX))
        {
            _graph->SetCutBox(CalcCutBox(_pData.getRotateAngle(1),  0, _pData.getSettings(PlotData::INT_COORDS), true),
                              CalcCutBox(_pData.getRotateAngle(1), 1, _pData.getSettings(PlotData::INT_COORDS), true));
        }

        for (size_t i = 0; i < m_manager.assets.size(); i++)
        {
            mglData& _mData_x = m_manager.assets[i].data[XCOORD].first;
            mglData& _mData_y = m_manager.assets[i].data[YCOORD].first;
            mglData& _mData_z = m_manager.assets[i].data[ZCOORD].first;

            // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
            if (_pData.getSettings(PlotData::LOG_FLOW))
                _graph->Flow(_mData_x, _mData_y, _mData_z, _pData.getColorScheme("v").c_str());
            else if (_pData.getSettings(PlotData::LOG_PIPE))
                _graph->Pipe(_mData_x, _mData_y, _mData_z, _pData.getColorScheme().c_str());
            else if (_pData.getSettings(PlotData::LOG_FIXEDLENGTH))
                _graph->Vect(_mData_x, _mData_y, _mData_z, _pData.getColorScheme("f").c_str());
            else if (!_pData.getSettings(PlotData::LOG_FIXEDLENGTH))
                _graph->Vect(_mData_x, _mData_y, _mData_z, _pData.getColorScheme().c_str());
            else
            {
                // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
                clearData();
                return;
            }
        }

        if (_pData.getSettings(PlotData::LOG_CUTBOX))
            _graph->SetCutBox(mglPoint(0), mglPoint(0));
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of all two-dimensional vector fields.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::create2dVect()
{
    mglData* _mData_x = nullptr;
    mglData* _mData_y = nullptr;

    // Reduce the samples if they are larger than
    // half of the image pixel dimensions
    m_manager.resize(nWidth/15, nHeight/15);

    for (size_t i = 0; i < m_manager.assets.size(); i++)
    {
        if (!m_manager.assets[i].isComplex(0) && m_manager.assets[i].type == PT_FUNCTION)
        {
            _mData_x = &m_manager.assets[i].data[XCOORD].first;
            _mData_y = &m_manager.assets[i].data[YCOORD].first;
        }
        else
        {
            _mData_x = &m_manager.assets[i].data[XCOORD].first;
            _mData_y = &m_manager.assets[i].data[XCOORD].second;
        }

        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
        if (_pData.getSettings(PlotData::LOG_FLOW))
            _graph->Flow(*_mData_x, *_mData_y, _pData.getColorScheme("v").c_str());
        else if (_pData.getSettings(PlotData::LOG_PIPE))
            _graph->Pipe(*_mData_x, *_mData_y, _pData.getColorScheme().c_str());
        else if (_pData.getSettings(PlotData::LOG_FIXEDLENGTH))
            _graph->Vect(*_mData_x, *_mData_y, _pData.getColorScheme("f").c_str());
        else if (!_pData.getSettings(PlotData::LOG_FIXEDLENGTH))
            _graph->Vect(*_mData_x, *_mData_y, _pData.getColorScheme().c_str());
        else
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            clearData();
            return;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of th drawing of using Face() and
/// ensures the transparency of the drawing.
///
/// \param _graph mglGraph*
/// \param p1 const mglPoint&
/// \param p2 const mglPoint&
/// \param p3 const mglPoint&
/// \param p4 const mglPoint&
/// \param sStyle const std::string&
/// \return void
///
/////////////////////////////////////////////////
static void faceAdapted(mglGraph* _graph, const mglPoint& p1, const mglPoint& p2, const mglPoint& p3, const mglPoint& p4, const std::string& sStyle)
{
    if (sStyle.substr(0, 1) == "#")
    {
        if (size(sStyle) > 1)
        {
            _graph->Line(p4, p2, sStyle.substr(1, 1).c_str());
            _graph->Line(p2, p1, sStyle.substr(1, 1).c_str());
            _graph->Line(p1, p3, sStyle.substr(1, 1).c_str());
            _graph->Line(p3, p4, sStyle.substr(1, 1).c_str());
        }
        else
        {
            _graph->Line(p4, p2, sStyle.c_str());
            _graph->Line(p2, p1, sStyle.c_str());
            _graph->Line(p1, p3, sStyle.c_str());
            _graph->Line(p3, p4, sStyle.c_str());
        }

        return;
    }

    if (size(sStyle) == 2)
    {
        _graph->Face(p1, p2, p3, p4, sStyle.substr(1, 1).c_str());
        _graph->Line(p4, p2, "#");
        _graph->Line(p2, p1, "#");
        _graph->Line(p1, p3, "#");
        _graph->Line(p3, p4, "#");
    }
    else if (size(sStyle) > 2)
    {
        _graph->Face(p1, p2, p3, p4, sStyle.substr(1, 1).c_str());
        _graph->Line(p4, p2, sStyle.substr(2, 1).c_str());
        _graph->Line(p2, p1, sStyle.substr(2, 1).c_str());
        _graph->Line(p1, p3, sStyle.substr(2, 1).c_str());
        _graph->Line(p3, p4, sStyle.substr(2, 1).c_str());
    }
    else
        _graph->Face(p1, p2, p3, p4, sStyle.c_str());
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of all two-dimensional drawings.
///
/// \param vDrawVector vector<string>&
/// \return void
///
/////////////////////////////////////////////////
void Plot::create2dDrawing(vector<string>& vDrawVector)
{
    int nFunctions;
    string sStyle;
    string sTextString;
    StringView sDrawExpr;
    StringView sCurrentDrawingFunction;
    string sDummy;

    for (size_t v = 0; v < vDrawVector.size(); v++)
    {
        sStyle = "k";
        sTextString = "";
        sCurrentDrawingFunction = vDrawVector[v];

        if (sCurrentDrawingFunction.back() == ')')
            sDrawExpr = sCurrentDrawingFunction.subview(sCurrentDrawingFunction.find('(') + 1, sCurrentDrawingFunction.rfind(')') - sCurrentDrawingFunction.find('(') - 1);
        else
            sDrawExpr = sCurrentDrawingFunction.subview(sCurrentDrawingFunction.find('(') + 1);

        if (sDrawExpr.find('{') != string::npos)
        {
            sDummy = sDrawExpr.to_string();
            convertVectorToExpression(sDummy);
            sDrawExpr = sDummy;
        }

        _parser.SetExpr(sDrawExpr);
        const mu::StackItem* vRes = _parser.Eval(nFunctions);

        if (nFunctions > 2
            && vRes[nFunctions-2].get().getCommonType() == mu::TYPE_STRING
            && vRes[nFunctions-1].get().getCommonType() == mu::TYPE_STRING)
        {
            sTextString = vRes[nFunctions-2].get().printVals();
            sStyle = vRes[nFunctions-1].get().printVals();
            nFunctions -= 2;
        }
        else if (nFunctions > 1
                 && vRes[nFunctions-1].get().getCommonType() == mu::TYPE_STRING)
        {
            sStyle = vRes[nFunctions-1].get().printVals();
            nFunctions -= 1;
        }

        std::vector<double> vResults;

        for (int i = 0; i < nFunctions; i++)
        {
            vResults.push_back(vRes[i].get().front().getNum().asF64());
        }

        if (sCurrentDrawingFunction.starts_with("trace(") || sCurrentDrawingFunction.starts_with("line("))
        {
            if (nFunctions < 2)
                continue;

            if (nFunctions < 4)
                _graph->Line(mglPoint(), mglPoint(vResults[0], vResults[1]), sStyle.c_str());
            else
                _graph->Line(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("tracev(") || sCurrentDrawingFunction.starts_with("linev("))
        {
            if (nFunctions < 2)
                continue;

            if (nFunctions < 4)
                _graph->Line(mglPoint(), mglPoint(vResults[0], vResults[1]), sStyle.c_str());
            else
                _graph->Line(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("face(") || sCurrentDrawingFunction.starts_with("cuboid("))
        {
            if (nFunctions < 4)
                continue;

            if (nFunctions < 6)
            {
                mglPoint point1 = mglPoint(vResults[2] - vResults[3] + vResults[1], vResults[3] + vResults[2] - vResults[0]);
                mglPoint point2 = mglPoint(vResults[2], vResults[3]);
                mglPoint point3 = mglPoint(vResults[0] - vResults[3] + vResults[1], vResults[1] + vResults[2] - vResults[0]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
            else if (nFunctions < 8)
            {
                mglPoint point1 = mglPoint(vResults[4], vResults[5]);
                mglPoint point2 = mglPoint(vResults[2], vResults[3]);
                mglPoint point3 = mglPoint(vResults[0] + vResults[4] - vResults[2], vResults[1] + vResults[5] - vResults[3]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
            else
            {
                mglPoint point1 = mglPoint(vResults[4], vResults[5]);
                mglPoint point2 = mglPoint(vResults[2], vResults[3]);
                mglPoint point3 = mglPoint(vResults[6], vResults[7]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
        }
        else if (sCurrentDrawingFunction.starts_with("facev("))
        {
            if (nFunctions < 4)
                continue;
            if (nFunctions < 6)
            {
                mglPoint point1 = mglPoint(vResults[0] + vResults[2] - vResults[3], vResults[1] + vResults[3] + vResults[2]);
                mglPoint point2 = mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]);
                mglPoint point3 = mglPoint(vResults[0] - vResults[3], vResults[1] + vResults[2]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
            else if (nFunctions < 8)
            {
                mglPoint point1 = mglPoint(vResults[0] + vResults[4] + vResults[2], vResults[1] + vResults[3] + vResults[5]);
                mglPoint point2 = mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]);
                mglPoint point3 = mglPoint(vResults[0] + vResults[4], vResults[1] + vResults[5]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
            else
            {
                mglPoint point1 = mglPoint(vResults[0] + vResults[4], vResults[1] + vResults[5]);
                mglPoint point2 = mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]);
                mglPoint point3 = mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[7]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
        }
        else if (sCurrentDrawingFunction.starts_with("triangle("))
        {
            if (nFunctions < 4)
                continue;

            double c = hypot(vResults[2] - vResults[0], vResults[3] - vResults[1]) / 2.0 * sqrt(3) / hypot(vResults[2], vResults[3]);

            if (nFunctions < 6)
            {
                mglPoint point1 = mglPoint((-vResults[0] + vResults[2]) / 2.0 - c * vResults[3], (-vResults[1] + vResults[3]) / 2.0 + c * vResults[2]);
                mglPoint point2 = mglPoint(vResults[2], vResults[3]);
                mglPoint point3 = mglPoint((-vResults[0] + vResults[2]) / 2.0 - c * vResults[3], (-vResults[1] + vResults[3]) / 2.0 + c * vResults[2]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }

            else
            {
                mglPoint point1 = mglPoint(vResults[4], vResults[5]);
                mglPoint point2 = mglPoint(vResults[2], vResults[3]);
                mglPoint point3 = mglPoint(vResults[4], vResults[5]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }

        }
        else if (sCurrentDrawingFunction.starts_with("trianglev("))
        {
            if (nFunctions < 4)
                continue;

            double c = sqrt(3.0) / 2.0;

            if (nFunctions < 6)
            {
                mglPoint point1 = mglPoint((vResults[0] + 0.5 * vResults[2]) - c * vResults[3], (vResults[1] + 0.5 * vResults[3]) + c * vResults[2]);
                mglPoint point2 = mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]);
                mglPoint point3 = mglPoint((vResults[0] + 0.5 * vResults[2]) - c * vResults[3], (vResults[1] + 0.5 * vResults[3]) + c * vResults[2]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }

            else
            {
                mglPoint point1 = mglPoint(vResults[0] + vResults[4], vResults[1] + vResults[5]);
                mglPoint point2 = mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]);
                mglPoint point3 = mglPoint(vResults[0] + vResults[4], vResults[1] + vResults[5]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }

        }
        else if (sCurrentDrawingFunction.starts_with("sphere("))
        {
            if (nFunctions < 3)
                continue;

            _graph->Sphere(mglPoint(vResults[0], vResults[1]), vResults[2], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("drop("))
        {
            if (nFunctions < 5)
                continue;

            double dShift = 1;
            double dAspherity = 1;

            if (nFunctions >= 6)
                dShift = vResults[5];

            if (nFunctions >= 7)
                dAspherity = vResults[6];

            _graph->Drop(mglPoint(vResults[0], vResults[1]),
                         mglPoint(vResults[2], vResults[3]),
                         vResults[4],
                         sStyle.c_str(),
                         dShift,
                         dAspherity);
        }
        else if (sCurrentDrawingFunction.starts_with("circle("))
        {
            if (nFunctions < 3)
                continue;

            _graph->Circle(mglPoint(vResults[0], vResults[1]), vResults[2], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("arc("))
        {
            if (nFunctions < 5)
                continue;

            _graph->Arc(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("arcv("))
        {
            if (nFunctions < 5)
                continue;

            _graph->Arc(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2] + vResults[0], vResults[3] + vResults[1]), vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("point("))
        {
            if (nFunctions < 2)
                continue;

            _graph->Mark(mglPoint(vResults[0], vResults[1]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("curve("))
        {
            if (nFunctions < 8)
                continue;

            _graph->Curve(mglPoint(vResults[0], vResults[1]),
                          mglPoint(vResults[2], vResults[3]),
                          mglPoint(vResults[4], vResults[5]),
                          mglPoint(vResults[6], vResults[7]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("ellipse("))
        {
            if (nFunctions < 5)
                continue;

            _graph->Ellipse(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("ellipsev("))
        {
            if (nFunctions < 5)
                continue;

            _graph->Ellipse(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2] + vResults[0], vResults[3] + vResults[1]), vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("text("))
        {
            double dTextSize = -1;

            if (!sTextString.length())
            {
                sTextString = sStyle;
                sStyle = "k";
            }

            if (sStyle.find_first_of("0123456789") != std::string::npos)
                dTextSize = StrToDb(sStyle.substr(sStyle.find_first_of("0123456789"), 1));

            if (nFunctions >= 4)
                _graph->Puts(mglPoint(vResults[0], vResults[1]),
                             mglPoint(vResults[2], vResults[3]), sTextString.c_str(), sStyle.c_str(), dTextSize);
            else if (nFunctions >= 2)
                _graph->Puts(mglPoint(vResults[0], vResults[1]), sTextString.c_str(), sStyle.c_str(), dTextSize);
            else
                continue;
        }
        else if (sCurrentDrawingFunction.starts_with("polygon("))
        {
            if (nFunctions < 5 || vResults[4] < 3)
                continue;

            _graph->Polygon(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), (int)vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("polygonv("))
        {
            if (nFunctions < 5 || vResults[4] < 3)
                continue;

            _graph->Polygon(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2] + vResults[0], vResults[3] + vResults[1]), (int)vResults[4], sStyle.c_str());
        }
        else
            continue;
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of all three-dimensional drawings.
///
/// \param vDrawVector vector<string>&
/// \return void
///
/////////////////////////////////////////////////
void Plot::create3dDrawing(vector<string>& vDrawVector)
{
    int nFunctions;
    string sStyle;
    string sTextString;
    StringView sDrawExpr;
    StringView sCurrentDrawingFunction;
    string sDummy;

    for (size_t v = 0; v < vDrawVector.size(); v++)
    {
        sStyle = "k";
        sTextString = "";
        sCurrentDrawingFunction = vDrawVector[v];

        if (sCurrentDrawingFunction.back() == ')')
            sDrawExpr = sCurrentDrawingFunction.subview(sCurrentDrawingFunction.find('(') + 1, sCurrentDrawingFunction.rfind(')') - sCurrentDrawingFunction.find('(') - 1);
        else
            sDrawExpr = sCurrentDrawingFunction.subview(sCurrentDrawingFunction.find('(') + 1);

        if (sDrawExpr.find('{') != string::npos)
        {
            sDummy = sDrawExpr.to_string();
            convertVectorToExpression(sDummy);
            sDrawExpr = sDummy;
        }

        _parser.SetExpr(sDrawExpr);
        const mu::StackItem* vRes = _parser.Eval(nFunctions);

        if (nFunctions > 2
            && vRes[nFunctions-2].get().getCommonType() == mu::TYPE_STRING
            && vRes[nFunctions-1].get().getCommonType() == mu::TYPE_STRING)
        {
            sTextString = vRes[nFunctions-2].get().printVals();
            sStyle = vRes[nFunctions-1].get().printVals();
            nFunctions -= 2;
        }
        else if (nFunctions > 1
                 && vRes[nFunctions-1].get().getCommonType() == mu::TYPE_STRING)
        {
            sStyle = vRes[nFunctions-1].get().printVals();
            nFunctions -= 1;
        }

        std::vector<double> vResults;

        for (int i = 0; i < nFunctions; i++)
        {
            vResults.push_back(vRes[i].get().front().getNum().asF64());
        }

        if (sCurrentDrawingFunction.starts_with("trace(") || sCurrentDrawingFunction.starts_with("line("))
        {
            if (nFunctions < 3)
                continue;

            if (nFunctions < 6)
                _graph->Line(mglPoint(), mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
            else
                _graph->Line(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("tracev(") || sCurrentDrawingFunction.starts_with("linev("))
        {
            if (nFunctions < 3)
                continue;

            if (nFunctions < 6)
                _graph->Line(mglPoint(), mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
            else
                _graph->Line(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3] + vResults[0], vResults[4] + vResults[1], vResults[5] + vResults[2]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("face("))
        {
            if (nFunctions < 6)
                continue;

            if (nFunctions < 9)
                 {
                    mglPoint point1 = mglPoint(vResults[3] - vResults[4] + vResults[1], vResults[4] + vResults[3] - vResults[0], vResults[5]);
                    mglPoint point2 = mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[2] + vResults[5]);
                    mglPoint point3 = mglPoint(vResults[0] - vResults[4] + vResults[1], vResults[1] + vResults[3] - vResults[0], vResults[2]);
                    mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                    faceAdapted(_graph, point1, point2, point3, point4, sStyle);
                 }
            else if (nFunctions < 12)
                {
                    mglPoint point1 = mglPoint(vResults[6], vResults[7], vResults[8]);
                    mglPoint point2 = mglPoint(vResults[3], vResults[4], vResults[5]);
                    mglPoint point3 = mglPoint(vResults[0] + vResults[6] - vResults[3], vResults[1] + vResults[7] - vResults[4], vResults[2] + vResults[8] - vResults[5]);
                    mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                    faceAdapted(_graph, point1, point2, point3, point4, sStyle);
                 }
            else
                {
                    mglPoint point1 = mglPoint(vResults[6], vResults[7], vResults[8]);
                    mglPoint point2 = mglPoint(vResults[3], vResults[4], vResults[5]);
                    mglPoint point3 = mglPoint(vResults[9], vResults[10], vResults[11]);
                    mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                    faceAdapted(_graph, point1, point2, point3, point4, sStyle);
                 }

        }
        else if (sCurrentDrawingFunction.starts_with("facev("))
        {
            if (nFunctions < 6)
                continue;

            if (nFunctions < 9)
                {
                    mglPoint point1 = mglPoint(vResults[0] + vResults[3] - vResults[4], vResults[1] + vResults[4] + vResults[3], vResults[5] + vResults[2]);
                    mglPoint point2 = mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[5] + vResults[2]);
                    mglPoint point3 = mglPoint(vResults[0] - vResults[4], vResults[1] + vResults[3], vResults[2]);
                    mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                    faceAdapted(_graph, point1, point2, point3, point4, sStyle);
                 }

            else if (nFunctions < 12)
                {
                    mglPoint point1 = mglPoint(vResults[0] + vResults[6] + vResults[3], vResults[1] + vResults[7] + vResults[4], vResults[2] + vResults[8] + vResults[5]);
                    mglPoint point2 = mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[4], vResults[2] + vResults[5]);
                    mglPoint point3 = mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[7], vResults[2] + vResults[8]);
                    mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                    faceAdapted(_graph, point1, point2, point3, point4, sStyle);
                 }

            else
                {
                    mglPoint point1 = mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[7], vResults[2] + vResults[8]);
                    mglPoint point2 = mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[2] + vResults[5]);
                    mglPoint point3 = mglPoint(vResults[0] + vResults[9], vResults[1] + vResults[10], vResults[2] + vResults[11]);
                    mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                    faceAdapted(_graph, point1, point2, point3, point4, sStyle);
                 }
        }
        else if (sCurrentDrawingFunction.starts_with("triangle("))
        {
            if (nFunctions < 6)
                continue;

            double c = sqrt((vResults[3] - vResults[0]) * (vResults[3] - vResults[0])
                            + (vResults[4] - vResults[1]) * (vResults[4] - vResults[1])
                            + (vResults[5] - vResults[2]) * (vResults[5] - vResults[2])) / 2.0 * sqrt(3) / hypot(vResults[3], vResults[4]);

            if (nFunctions < 9)
            {
                mglPoint point1 = mglPoint((-vResults[0] + vResults[3]) / 2.0 - c * vResults[4], (-vResults[1] + vResults[4]) / 2.0 + c * vResults[3], (vResults[5] + vResults[2]) / 2.0);
                mglPoint point2 = mglPoint(vResults[3], vResults[4], vResults[5]);
                mglPoint point3 = mglPoint((-vResults[0] + vResults[3]) / 2.0 - c * vResults[4], (-vResults[1] + vResults[4]) / 2.0 + c * vResults[3], (vResults[5] + vResults[2]) / 2.0);
                mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }

            else
            {
                mglPoint point1 = mglPoint(vResults[6], vResults[7], vResults[8]);
                mglPoint point2 = mglPoint(vResults[3], vResults[4], vResults[5]);
                mglPoint point3 = mglPoint(vResults[6], vResults[7], vResults[8]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
        }
        else if (sCurrentDrawingFunction.starts_with("trianglev("))
        {
            if (nFunctions < 6)
                continue;

            double c = sqrt((vResults[3]) * (vResults[3])
                            + (vResults[4]) * (vResults[4])
                            + (vResults[5]) * (vResults[5])) / 2.0 * sqrt(3) / hypot(vResults[3], vResults[4]);

            if (nFunctions < 9)
            {
                mglPoint point1 = mglPoint((vResults[0] + 0.5 * vResults[3]) - c * vResults[4], (vResults[1] + 0.5 * vResults[4]) + c * vResults[3], (vResults[5] + 0.5 * vResults[2]));
                mglPoint point2 = mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[2] + vResults[5]);
                mglPoint point3 = mglPoint((vResults[0] + 0.5 * vResults[3]) - c * vResults[4], (vResults[1] + 0.5 * vResults[4]) + c * vResults[3], (vResults[5] + 0.5 * vResults[2]));
                mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
            else
            {
                mglPoint point1 = mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[7], vResults[2] + vResults[8]);
                mglPoint point2 = mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[2] + vResults[5]);
                mglPoint point3 = mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[7], vResults[2] + vResults[8]);
                mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]);
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
            }
        }
        else if (sCurrentDrawingFunction.starts_with("cuboid("))
        {
            if (nFunctions < 6)
                continue;

            mglPoint _mDx;
            mglPoint _mDy;
            mglPoint _mDz;

            if (nFunctions < 9)
            {
                _mDx = mglPoint(vResults[3], vResults[4], vResults[5]);
                _mDy = mglPoint(-vResults[4], vResults[3], vResults[5]);
                _mDz = mglPoint(vResults[4] * vResults[5] - vResults[3] * vResults[5],
                                -vResults[4] * vResults[5] - vResults[3] * vResults[5],
                                vResults[3] * vResults[3] + vResults[4] * vResults[4])
                       / sqrt(vResults[3] * vResults[3] + vResults[4] * vResults[4] + vResults[5] * vResults[5]);
            }
            else if (nFunctions < 12)
            {
                _mDx = mglPoint(vResults[3], vResults[4], vResults[5]);
                _mDy = mglPoint(vResults[6], vResults[7], vResults[8]);
                _mDz = mglPoint(vResults[4] * vResults[8] - vResults[7] * vResults[5],
                                vResults[6] * vResults[5] - vResults[3] * vResults[8],
                                vResults[3] * vResults[7] - vResults[6] * vResults[4]) * 2.0
                       / (sqrt(vResults[3] * vResults[3] + vResults[4] * vResults[4] + vResults[5] * vResults[5])
                          + sqrt(vResults[6] * vResults[6] + vResults[7] * vResults[7] + vResults[8] * vResults[8]));
            }
            else
            {
                _mDx = mglPoint(vResults[3], vResults[4], vResults[5]);
                _mDy = mglPoint(vResults[6], vResults[7], vResults[8]);
                _mDz = mglPoint(vResults[9], vResults[10], vResults[11]);
            }


                mglPoint point1 = mglPoint(vResults[0], vResults[1], vResults[2]);
                mglPoint point2 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx;
                mglPoint point3 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy;
                mglPoint point4 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx + _mDy;
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);

                point1 = mglPoint(vResults[0], vResults[1], vResults[2]);
                point2 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx;
                point3 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz;
                point4 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz + _mDx;
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);

                point1 = mglPoint(vResults[0], vResults[1], vResults[2]);
                point2 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy;
                point3 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz;
                point4 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy + _mDz;
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);

                point1 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz;
                point2 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx + _mDz;
                point3 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy + _mDz;
                point4 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx + _mDy + _mDz;
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);

                point1 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy;
                point2 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx + _mDy;
                point3 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz + _mDy;
                point4 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz + _mDx + _mDy;
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);

                point1 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx;
                point2 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy + _mDx;
                point3 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz + _mDx;
                point4 = mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy + _mDz + _mDx;
                faceAdapted(_graph, point1, point2, point3, point4, sStyle);
        }
        else if (sCurrentDrawingFunction.starts_with("sphere("))
        {
            if (nFunctions < 4)
                continue;

            _graph->Sphere(mglPoint(vResults[0], vResults[1], vResults[2]), vResults[3], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("cone("))
        {
            if (nFunctions < 7)
                continue;

            if (nFunctions >= 8)
                _graph->Cone(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], vResults[7], sStyle.c_str());
            else
                _graph->Cone(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], 0.0, sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("conev("))
        {
            if (nFunctions < 7)
                continue;

            if (nFunctions >= 8)
                _graph->Cone(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]), vResults[6], vResults[7], sStyle.c_str());
            else
                _graph->Cone(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]), vResults[6], 0.0, sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("drop("))
        {
            if (nFunctions < 7)
                continue;

            double dShift = 1;
            double dAspherity = 1;

            if (nFunctions >= 8)
                dShift = vResults[7];

            if (nFunctions >= 9)
                dAspherity = vResults[8];

            _graph->Drop(mglPoint(vResults[0], vResults[1], vResults[2]),
                         mglPoint(vResults[3], vResults[4], vResults[5]),
                         vResults[6],
                         sStyle.c_str(),
                         dShift,
                         dAspherity);
        }
        else if (sCurrentDrawingFunction.starts_with("circle("))
        {
            if (nFunctions < 4)
                continue;

            _graph->Circle(mglPoint(vResults[0], vResults[1], vResults[2]), vResults[3], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("arc("))
        {
            if (nFunctions < 7)
                continue;

            if (nFunctions < 9)
                _graph->Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], sStyle.c_str());
            else
                _graph->Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[3], vResults[4], vResults[5]),
                            mglPoint(vResults[6], vResults[7], vResults[8]),
                            vResults[9], sStyle.c_str());

        }
        else if (sCurrentDrawingFunction.starts_with("arcv("))
        {
            if (nFunctions < 7)
                continue;

            if (nFunctions < 9)
                _graph->Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]), vResults[6], sStyle.c_str());
            else
                _graph->Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[6], vResults[7], vResults[8]) + mglPoint(vResults[0], vResults[1], vResults[2]),
                            vResults[9], sStyle.c_str());

        }
        else if (sCurrentDrawingFunction.starts_with("point("))
        {
            if (nFunctions < 3)
                continue;

            _graph->Mark(mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("curve("))
        {
            if (nFunctions < 12)
                continue;

            _graph->Curve(mglPoint(vResults[0], vResults[1], vResults[2]),
                          mglPoint(vResults[3], vResults[4], vResults[5]),
                          mglPoint(vResults[6], vResults[7], vResults[8]),
                          mglPoint(vResults[9], vResults[10], vResults[11]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("ellipse("))
        {
            if (nFunctions < 7)
                continue;

            _graph->Ellipse(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("ellipsev("))
        {
            if (nFunctions < 7)
                continue;

            _graph->Ellipse(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]), vResults[6], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("text("))
        {
            double dTextSize = -1;

            if (!sTextString.length())
            {
                sTextString = sStyle;
                sStyle = "k";
            }

            if (sStyle.find_first_of("0123456789") != std::string::npos)
                dTextSize = StrToDb(sStyle.substr(sStyle.find_first_of("0123456789"), 1));

            if (nFunctions >= 6)
                _graph->Puts(mglPoint(vResults[0], vResults[1], vResults[2]),
                             mglPoint(vResults[3], vResults[4], vResults[5]), sTextString.c_str(), sStyle.c_str(), dTextSize);
            else if (nFunctions >= 3)
                _graph->Puts(mglPoint(vResults[0], vResults[1], vResults[2]), sTextString.c_str(), sStyle.c_str(), dTextSize);
            else
                continue;
        }
        else if (sCurrentDrawingFunction.starts_with("polygon("))
        {
            if (nFunctions < 7 || vResults[6] < 3)
                continue;

            _graph->Polygon(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), (int)vResults[6], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.starts_with("polygonv("))
        {
            if (nFunctions < 7 || vResults[6] < 3)
                continue;

            _graph->Polygon(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]), (int)vResults[6], sStyle.c_str());
        }
        else
            continue;
    }
}


/////////////////////////////////////////////////
/// \brief This member function wraps the
/// creation of all trajectory-like plots (plots
/// describing a series of points in 3D space).
///
/// \param nPlotCompose size_t
/// \param nPlotComposeSize size_t
/// \return void
///
/////////////////////////////////////////////////
void Plot::createStd3dPlot(size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    bool useImag = false;
    bool masked = (_pData.getSettings(PlotData::LOG_COLORMASK) || _pData.getSettings(PlotData::LOG_ALPHAMASK))
        && _pData.getSettings(PlotData::INT_MARKS);

    mglData _mData[3];
    mglData _mData2[3];

    if (_pData.getSettings(PlotData::LOG_CUTBOX))
        _graph->SetCutBox(CalcCutBox(_pData.getRotateAngle(1), 0, _pData.getSettings(PlotData::INT_COORDS), true),
                          CalcCutBox(_pData.getRotateAngle(1), 1, _pData.getSettings(PlotData::INT_COORDS), true));

    // Apply curvilinear coordinates
    m_manager.applyCoordSys((CoordinateSystem)_pData.getSettings(PlotData::INT_COORDS), masked ? 2 : 1);

    for (size_t n = 0; n < m_manager.assets.size(); n++)
    {
        int nDataOffset = 0;

        if (_pData.getSettings(PlotData::LOG_PARAMETRIC) && n+2 < m_manager.assets.size())
        {
            _mData[XCOORD].Link(useImag ? m_manager.assets[n].data[0].second : m_manager.assets[n].data[0].first);
            _mData[YCOORD].Link(useImag ? m_manager.assets[n+1].data[0].second : m_manager.assets[n+1].data[0].first);
            _mData[ZCOORD].Link(useImag ? m_manager.assets[n+2].data[0].second : m_manager.assets[n+2].data[0].first);
            nDataOffset = 2;
        }
        else
        {
            _mData[XCOORD].Link(useImag ? m_manager.assets[n].data[XCOORD].second : m_manager.assets[n].data[XCOORD].first);
            _mData[YCOORD].Link(useImag ? m_manager.assets[n].data[YCOORD].second : m_manager.assets[n].data[YCOORD].first);
            _mData[ZCOORD].Link(useImag ? m_manager.assets[n].data[ZCOORD].second : m_manager.assets[n].data[ZCOORD].first);
        }

        StripSpaces(m_manager.assets[n+nDataOffset].legend);

        if ((_pData.getSettings(PlotData::LOG_REGION) || masked)
            && n+nDataOffset+1 < m_manager.assets.size())
        {
            _mData2[XCOORD].Link(useImag ? m_manager.assets[n+nDataOffset+1].data[XCOORD].second : m_manager.assets[n+nDataOffset+1].data[XCOORD].first);
            _mData2[YCOORD].Link(useImag ? m_manager.assets[n+nDataOffset+1].data[YCOORD].second : m_manager.assets[n+nDataOffset+1].data[YCOORD].first);
            _mData2[ZCOORD].Link(useImag ? m_manager.assets[n+nDataOffset+1].data[ZCOORD].second : m_manager.assets[n+nDataOffset+1].data[ZCOORD].first);
        }
        else
        {
            for (int j = XCOORD; j <= ZCOORD; j++)
            {
                _mData2[j] = 0.0 * _mData[j];
            }
        }

        if (m_manager.assets[n+nDataOffset].type == PT_DATA)
        {
            for (int j = XRANGE; j <= ZRANGE; j++)
            {
                for (int i = 0; i < getNN(_mData[j]); i++)
                {
                    if (!_pInfo.ranges[j].isInside(_mData[j].a[i]))
                        _mData[j].a[i] = NAN;
                }
            }

            if (_pData.getSettings(PlotData::LOG_XERROR) && _pData.getSettings(PlotData::LOG_YERROR))
            {
                _mData2[XCOORD].Link(m_manager.assets[n+nDataOffset].data[XCOORD+3].first);
                _mData2[YCOORD].Link(m_manager.assets[n+nDataOffset].data[YCOORD+3].first);
                _mData2[ZCOORD].Link(m_manager.assets[n+nDataOffset].data[ZCOORD+3].first);
            }
        }

        // Create the actual 3D trajectory plot
        if (!plotstd3d(_mData, _mData2, m_manager.assets[n+nDataOffset].type))
        {
            clearData();
            return;
        }

        if (m_manager.assets[n+nDataOffset].type == PT_FUNCTION)
        {
            if (_pData.getSettings(PlotData::LOG_REGION) && n+nDataOffset+1 < m_manager.assets.size())
            {
                for (int k = 0; k < 2; k++)
                {
                    sConvLegends = m_manager.assets[n+nDataOffset].legend;
                    getDataElements(sConvLegends, _parser, _data);
                    _parser.SetExpr(sConvLegends);
                    sConvLegends = _parser.Eval().printVals();
                    sConvLegends = "\"" + sConvLegends + "\"";

                    for (size_t l = 0; l < sConvLegends.length(); l++)
                    {
                        if (sConvLegends[l] == '(')
                            l += getMatchingParenthesis(StringView(sConvLegends, l));

                        if (sConvLegends[l] == ',')
                        {
                            sConvLegends = "\"[" + sConvLegends.substr(1, sConvLegends.length() - 2) + "]\"";
                            break;
                        }
                    }

                    if (sConvLegends != "\"\"")
                    {
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(),
                                          _pInfo.sLineStyles[_pInfo.nStyle].c_str());
                        nLegends++;
                    }

                }
            }
            else
            {
                _parser.SetExpr(m_manager.assets[n+nDataOffset].legend);
                sConvLegends = _parser.Eval().printVals();

                if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM && sConvLegends.length())
                    sConvLegends = useImag ? "Im(" + sConvLegends + ")" : "Re(" + sConvLegends + ")";

                sConvLegends = "\"" + sConvLegends + "\"";

                if (sConvLegends != "\"\"")
                {
                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(),
                                      getLegendStyle(_pInfo.sLineStyles[_pInfo.nStyle]).c_str());
                    nLegends++;
                }

            }
        }
        else
        {
            sConvLegends = m_manager.assets[n+nDataOffset].legend;
            getDataElements(sConvLegends, _parser, _data);
            _parser.SetExpr(sConvLegends);
            sConvLegends = _parser.Eval().printVals();

            if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM && sConvLegends.length())
                sConvLegends = useImag ? "Im(" + sConvLegends + ")" : "Re(" + sConvLegends + ")";

            sConvLegends = "\"" + sConvLegends + "\"";

            if (sConvLegends != "\"\"")
            {
                nLegends++;
                std::string sLegendStyle;
                std::string sLegend = fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2)));

                if (!_pData.getSettings(PlotData::LOG_XERROR) && !_pData.getSettings(PlotData::LOG_YERROR))
                {
                    if ((_pData.getSettings(PlotData::LOG_INTERPOLATE) && countValidElements(_mData[XCOORD]) >= (size_t)_pInfo.nSamples)
                        || _pData.getSettings(PlotData::FLOAT_BARS))
                        sLegendStyle = getLegendStyle(_pInfo.sLineStyles[_pInfo.nStyle]);
                    else if (_pData.getSettings(PlotData::LOG_CONNECTPOINTS)
                             || (_pData.getSettings(PlotData::LOG_INTERPOLATE) && countValidElements(_mData[XCOORD]) >= 0.9 * _pInfo.nSamples))
                        sLegendStyle = getLegendStyle(_pInfo.sConPointStyles[_pInfo.nStyle]);
                    else if (_pData.getSettings(PlotData::LOG_STEPPLOT))
                        sLegendStyle = getLegendStyle(_pInfo.sLineStyles[_pInfo.nStyle]);
                    else
                        sLegendStyle = getLegendStyle(_pInfo.sPointStyles[_pInfo.nStyle]);
                }
                else
                    sLegendStyle = getLegendStyle(_pInfo.sPointStyles[_pInfo.nStyle]);

                _graph->AddLegend(sLegend.c_str(), sLegendStyle.c_str());
            }

        }

        _pInfo.nStyle = _pInfo.nextStyle();

        if ((_pData.getSettings(PlotData::LOG_REGION) || masked)
            && n+nDataOffset+1 < m_manager.assets.size()
            && (useImag || _pData.getSettings(PlotData::INT_COMPLEXMODE) != CPLX_REIM))
            n++;

        if (getNN(_mData2[0]) && _pData.getSettings(PlotData::LOG_REGION))
            _pInfo.nStyle = _pInfo.nextStyle();

        if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM)
        {
            if (!useImag)
            {
                useImag = true;
                n--;
            }
            else
            {
                useImag = false;
                n += nDataOffset;
            }
        }
        else
            n += nDataOffset;
    }

    if (_pData.getSettings(PlotData::LOG_CUTBOX))
        _graph->SetCutBox(mglPoint(0), mglPoint(0));

    if (nPlotCompose + 1 == nPlotComposeSize
        && !((_pData.getSettings(PlotData::INT_MARKS) || _pData.getSettings(PlotData::LOG_CRUST)) && _pInfo.sCommand.substr(0, 6) == "plot3d")
        && nLegends
        && !_pData.getSettings(PlotData::LOG_SCHEMATIC))
    {
        _graph->SetMarkSize(1.0);
        if (_pData.getRotateAngle() || _pData.getRotateAngle(1))
            _graph->Legend(1.35, 1.2);
        else
            _graph->Legend(_pData.getSettings(PlotData::INT_LEGENDPOSITION));
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates an actual
/// single three-dimensional trajectory.
///
/// \param _mData[3] mglData
/// \param _mData2[3] mglData
/// \param nType const short
/// \return bool
///
/////////////////////////////////////////////////
bool Plot::plotstd3d(mglData _mData[3], mglData _mData2[3], const short nType)
{
#warning NOTE (numere#3#08/15/21): Temporary fix for MathGL misbehaviour
    if (!_pData.getSettings(PlotData::LOG_BOXPLOT)
        && !_pData.getSettings(PlotData::LOG_YERROR)
        && !_pData.getSettings(PlotData::LOG_XERROR)
        && !_pData.getSettings(PlotData::FLOAT_BARS)
        && !_pData.getSettings(PlotData::FLOAT_HBARS)
        && !_pData.getSettings(PlotData::LOG_STEPPLOT))
    {
        for (int i = XCOORD; i <= ZCOORD; i++)
        {
            _mData[i] = duplicatePoints(_mData[i]);
            _mData2[i] = duplicatePoints(_mData2[i]);
        }
    }

    std::string sAreaGradient = std::string("a") + _pData.getColors()[_pInfo.nStyle] + "{" + _pData.getColors()[_pInfo.nStyle] + "9}";
    int nNextStyle = _pInfo.nextStyle();

    if (isdigit(_pInfo.sLineStyles[_pInfo.nStyle].back()))
        _graph->SetMarkSize((_pInfo.sLineStyles[_pInfo.nStyle].back() - '0')*0.5 + 0.5);

    if (nType == PT_FUNCTION)
    {
        if (!_pData.getSettings(PlotData::LOG_AREA)
            && !_pData.getSettings(PlotData::LOG_REGION))
            _graph->Plot(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD], ("a" + _pInfo.sLineStyles[_pInfo.nStyle]).c_str());
        else if (_pData.getSettings(PlotData::LOG_REGION)
                 && getNN(_mData2[0]))
        {
            _graph->Region(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD], _mData2[XCOORD], _mData2[YCOORD], _mData2[ZCOORD],
                           ("a{" + _pData.getColors().substr(_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(nNextStyle, 1) + "7}").c_str());
            _graph->Plot(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                         ("a" + _pInfo.sLineStyles[_pInfo.nStyle]).c_str());
            _graph->Plot(_mData2[XCOORD], _mData2[YCOORD], _mData2[ZCOORD],
                         ("a" + _pInfo.sLineStyles[nNextStyle]).c_str());
        }
        else
            _graph->Area(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD], sAreaGradient.c_str());
    }
    else
    {
        if (!_pData.getSettings(PlotData::LOG_XERROR) && !_pData.getSettings(PlotData::LOG_YERROR))
        {
            // --> Interpolate-Schalter. Siehe weiter oben fuer Details <--
            if (_pData.getSettings(PlotData::LOG_INTERPOLATE)
                && countValidElements(_mData[0]) >= (size_t)_pInfo.nSamples)
            {
                if (_pData.getSettings(PlotData::FLOAT_BARS))
                    _graph->Bars(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                 (_pInfo.sLineStyles[_pInfo.nStyle] + "^").c_str());
                else if (_pData.getSettings(PlotData::LOG_AREA) || _pData.getSettings(PlotData::LOG_REGION))
                    _graph->Area(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD], sAreaGradient.c_str());
                else
                    _graph->Plot(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                 ("a" + _pInfo.sLineStyles[_pInfo.nStyle]).c_str());
            }
            else if (_pData.getSettings(PlotData::LOG_CONNECTPOINTS)
                     || (_pData.getSettings(PlotData::LOG_INTERPOLATE) && countValidElements(_mData[0]) >= 0.9 * _pInfo.nSamples))
            {
                if (_pData.getSettings(PlotData::FLOAT_BARS))
                    _graph->Bars(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                 (_pInfo.sLineStyles[_pInfo.nStyle] + "^").c_str());
                else if (_pData.getSettings(PlotData::LOG_AREA))
                    _graph->Area(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD], sAreaGradient.c_str());
                else
                    _graph->Plot(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                 ("a" + _pInfo.sConPointStyles[_pInfo.nStyle]).c_str());
            }
            else
            {
                if (_pData.getSettings(PlotData::INT_MARKS))
                {
                    if (_pData.getSettings(PlotData::LOG_ALPHAMASK))
                        _graph->Dots(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD], _mData2[ZCOORD],
                                     _pData.getColorScheme(toString(_pData.getSettings(PlotData::INT_MARKS))).c_str());
                    else if (_pData.getSettings(PlotData::LOG_COLORMASK))
                        _graph->Dots(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD], _mData2[ZCOORD], _mData2[ZCOORD]*0,
                                     _pData.getColorScheme(toString(_pData.getSettings(PlotData::INT_MARKS))).c_str());
                    else
                        _graph->Dots(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                     _pData.getColorScheme(toString(_pData.getSettings(PlotData::INT_MARKS))).c_str());
                }
                else if (_pData.getSettings(PlotData::LOG_CRUST))
                    _graph->Crust(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                  _pData.getColorScheme().c_str());
                else if (_pData.getSettings(PlotData::FLOAT_BARS))
                    _graph->Bars(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                 (_pInfo.sLineStyles[_pInfo.nStyle] + "^").c_str());
                else if (_pData.getSettings(PlotData::LOG_STEPPLOT))
                    _graph->Step(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                 (_pInfo.sLineStyles[_pInfo.nStyle]).c_str());
                else if (_pData.getSettings(PlotData::LOG_AREA))
                    _graph->Stem(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                 _pInfo.sConPointStyles[_pInfo.nStyle].c_str());
                else
                    _graph->Plot(_mData[XCOORD], _mData[YCOORD], _mData[ZCOORD],
                                 ("a" + _pInfo.sPointStyles[_pInfo.nStyle]).c_str());
            }
        }
        else if (_pData.getSettings(PlotData::LOG_XERROR)
                 || _pData.getSettings(PlotData::LOG_YERROR))
        {
            for (int m = 0; m < _mData[0].nx; m++)
            {
                _graph->Error(mglPoint(_mData[XCOORD].a[m], _mData[YCOORD].a[m], _mData[ZCOORD].a[m]),
                              mglPoint(_mData2[XCOORD].a[m], _mData2[YCOORD].a[m], _mData2[ZCOORD].a[m]),
                              _pInfo.sPointStyles[_pInfo.nStyle].c_str());
            }
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function is a helper to
/// return the number of data points located in
/// the passed mglData object.
///
/// \param _mData const mglData&
/// \return long
///
/////////////////////////////////////////////////
long Plot::getNN(const mglData& _mData)
{
    return _mData.nx * _mData.ny * _mData.nz;
}


/////////////////////////////////////////////////
/// \brief This member function creates a default
/// plot file name based upon the currently used
/// plotting command.
///
/// \param nPlotComposeSize size_t
/// \param nPlotCompose size_t
/// \return void
///
/////////////////////////////////////////////////
void Plot::filename(size_t nPlotComposeSize, size_t nPlotCompose)
{
    // --> Ggf. waehlen eines Default-Dateinamens <--
    if (!_pData.getSettings(PlotData::STR_FILENAME).length() && !nPlotCompose)
    {
        string sExt = ".png";

        if (_pData.getAnimateSamples())
            sExt = ".gif";

        if (nPlotComposeSize > 1)
            _pData.setFileName("composition" + sExt);
        else if (_pInfo.sCommand == "plot3d")
            _pData.setFileName("plot3d" + sExt);
        else if (_pInfo.sCommand == "plot")
            _pData.setFileName("plot" + sExt);
        else if (_pInfo.sCommand == "meshgrid3d" || _pInfo.sCommand.substr(0, 6) == "mesh3d")
            _pData.setFileName("meshgrid3d" + sExt);
        else if (_pInfo.sCommand.substr(0, 4) == "mesh")
            _pData.setFileName("meshgrid" + sExt);
        else if (_pInfo.sCommand.substr(0, 6) == "surf3d" || _pInfo.sCommand == "surface3d")
            _pData.setFileName("surface3d" + sExt);
        else if (_pInfo.sCommand.substr(0, 4) == "surf")
            _pData.setFileName("surface" + sExt);
        else if (_pInfo.sCommand.substr(0, 6) == "cont3d" || _pInfo.sCommand == "contour3d")
            _pData.setFileName("contour3d" + sExt);
        else if (_pInfo.sCommand.substr(0.4) == "cont")
            _pData.setFileName("contour" + sExt);
        else if (_pInfo.sCommand.substr(0, 6) == "grad3d" || _pInfo.sCommand == "gradient3d")
            _pData.setFileName("gradient3d" + sExt);
        else if (_pInfo.sCommand.substr(0, 4) == "grad")
            _pData.setFileName("gradient" + sExt);
        else if (_pInfo.sCommand == "density3d" || _pInfo.sCommand.substr(0, 6) == "dens3d")
            _pData.setFileName("density3d" + sExt);
        else if (_pInfo.sCommand.substr(0, 4) == "dens")
            _pData.setFileName("density" + sExt);
        else if (_pInfo.sCommand.substr(0, 6) == "vect3d" || _pInfo.sCommand == "vector3d")
            _pData.setFileName("vectorfield3d" + sExt);
        else if (_pInfo.sCommand.substr(0, 4) == "vect")
            _pData.setFileName("vectorfield" + sExt);
        else if (_pInfo.sCommand.substr(0, 6) == "draw3d")
            _pData.setFileName("drawing3d" + sExt);
        else if (_pInfo.sCommand.substr(0, 4) == "draw")
            _pData.setFileName("drawing" + sExt);
        else if (_pInfo.sCommand == "implot")
            _pData.setFileName("image" + sExt);
        else
            _pData.setFileName("unknown_style" + sExt);
    }
    else if (_pData.getSettings(PlotData::STR_FILENAME).length() && !nPlotCompose)
        bOutputDesired = true;

    if (containsStrings(_pData.getSettings(PlotData::STR_FILENAME)) && !nPlotCompose)
    {
        std::string sTemp = _pData.getSettings(PlotData::STR_FILENAME);
        std::string sExtension = sTemp.substr(sTemp.find('.'));
        sTemp = sTemp.substr(0, sTemp.find('.'));

        if (sExtension[sExtension.length() - 1] == '"')
        {
            sTemp += "\"";
            sExtension = sExtension.substr(0, sExtension.length() - 1);
        }

        _parser.SetExpr(sTemp);
        sTemp = _parser.Eval().printVals();
        _pData.setFileName(sTemp + sExtension);
    }

    if (_pData.getAnimateSamples() && _pData.getSettings(PlotData::STR_FILENAME).substr(_pData.getSettings(PlotData::STR_FILENAME).rfind('.')) != ".gif" && !nPlotCompose)
        _pData.setFileName(_pData.getSettings(PlotData::STR_FILENAME).substr(0, _pData.getSettings(PlotData::STR_FILENAME).length() - 4) + ".gif");
}


/////////////////////////////////////////////////
/// \brief This member function prepares the line
/// and point styles based upon the currently
/// used parameters.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::setStyles()
{
    for (int i = 0; i < _pInfo.nStyleMax; i++)
    {
        _pInfo.sLineStyles[i] = _pData.getColors().substr(i, 1);
        _pInfo.sPointStyles[i] = _pData.getColors().substr(i, 1);
        _pInfo.sContStyles[i] = _pData.getColors().substr(i, 1);
        _pInfo.sConPointStyles[i] = _pData.getColors().substr(i, 1);
    }

    for (int i = 0; i < _pInfo.nStyleMax; i++)
    {
        _pInfo.sLineStyles[i] += _pData.getLineStyles()[i];

        if (_pData.getSettings(PlotData::LOG_DRAWPOINTS))
        {
            if (_pData.getSettings(PlotData::STR_POINTSTYLES)[2 * i] != ' ')
                _pInfo.sLineStyles[i] += _pData.getSettings(PlotData::STR_POINTSTYLES)[2 * i];

            _pInfo.sLineStyles[i] += _pData.getSettings(PlotData::STR_POINTSTYLES)[2 * i + 1];
        }

        if (_pData.getSettings(PlotData::LOG_YERROR) || _pData.getSettings(PlotData::LOG_XERROR))
        {
            if (_pData.getSettings(PlotData::STR_POINTSTYLES)[2 * i] != ' ')
                _pInfo.sPointStyles[i] += _pData.getSettings(PlotData::STR_POINTSTYLES)[2 * i];

            _pInfo.sPointStyles[i] += _pData.getSettings(PlotData::STR_POINTSTYLES)[2 * i + 1];
        }
        else
        {
            if (_pData.getSettings(PlotData::STR_POINTSTYLES)[2 * i] != ' ')
                _pInfo.sPointStyles[i] += " " + _pData.getSettings(PlotData::STR_POINTSTYLES).substr(2 * i, 1) + _pData.getSettings(PlotData::STR_POINTSTYLES).substr(2 * i + 1, 1);
            else
                _pInfo.sPointStyles[i] += " " + _pData.getSettings(PlotData::STR_POINTSTYLES).substr(2 * i + 1, 1);
        }

        if (_pData.getSettings(PlotData::STR_POINTSTYLES)[2 * i] != ' ')
            _pInfo.sConPointStyles[i] += _pData.getLineStyles().substr(i, 1) + _pData.getSettings(PlotData::STR_POINTSTYLES).substr(2 * i, 1) + _pData.getSettings(PlotData::STR_POINTSTYLES).substr(2 * i + 1, 1);
        else
            _pInfo.sConPointStyles[i] += _pData.getLineStyles().substr(i, 1) + _pData.getSettings(PlotData::STR_POINTSTYLES).substr(2 * i + 1, 1);

        _pInfo.sContStyles[i] += _pData.getLineStyles()[i];
        _pInfo.sLineStyles[i] += _pData.getSettings(PlotData::STR_LINESIZES)[i];
        _pInfo.sConPointStyles[i] += _pData.getSettings(PlotData::STR_LINESIZES)[i];
    }
}


/////////////////////////////////////////////////
/// \brief This protected member function expands
/// a style string into 9 style string with
/// different luminosities. This is used by curve
/// arrays.
///
/// \param sCurrentStyle const string&
/// \param expand bool
/// \return string
///
/////////////////////////////////////////////////
string Plot::expandStyleForCurveArray(const string& sCurrentStyle, bool expand)
{
    // Do nothing, if the string shall not be expanded
    if (!expand)
        return sCurrentStyle;

    string sExpandedStyle;

    // Expand the style string
    for (size_t i = 1; i <= 9; i++)
    {
        sExpandedStyle += "{" + sCurrentStyle.substr(0, 1) + toString(i) + "}" + sCurrentStyle.substr(1);
    }

    return sExpandedStyle;
}


/////////////////////////////////////////////////
/// \brief This member function evaluates the
/// subplot command in a plot composition and
/// determines, where to place the next plot and
/// if that is actually possible.
///
/// \param sCmd string&
/// \param nMultiplots[2] size_t
/// \param nSubPlots size_t&
/// \param nSubPlotMap size_t&
/// \return void
///
/////////////////////////////////////////////////
void Plot::evaluateSubplot(string& sCmd, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap)
{
    if (nLegends && !_pData.getSettings(PlotData::LOG_SCHEMATIC))
    {
        _graph->Legend(_pData.getSettings(PlotData::INT_LEGENDPOSITION));
        _graph->ClearLegend();
    }

    string sSubPlotIDX = sCmd.substr(findCommand(sCmd).nPos + 7);

    if (sSubPlotIDX.find("-set") != string::npos || sSubPlotIDX.find("--") != string::npos)
    {
        _pData.setGlobalComposeParams(sSubPlotIDX);

        if (sSubPlotIDX.find("-set") != string::npos)
            sSubPlotIDX.erase(sSubPlotIDX.find("-set"));
        else
            sSubPlotIDX.erase(sSubPlotIDX.find("--"));
    }

    StripSpaces(sSubPlotIDX);

    if (findParameter(sCmd, "cols", '=') || findParameter(sCmd, "lines", '=') || findParameter(sCmd, "rows", '='))
    {
        size_t nMultiLines = 1, nMultiCols = 1;

        if (findParameter(sCmd, "cols", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "cols", '=') + 4));
            nMultiCols = (size_t)_parser.Eval().getAsScalarInt();
        }

        if (findParameter(sCmd, "rows", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "rows", '=') + 4));
            nMultiLines = (size_t)_parser.Eval().getAsScalarInt();
        }
        else if (findParameter(sCmd, "lines", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "lines", '=') + 5));
            nMultiLines = (size_t)_parser.Eval().getAsScalarInt();
        }

        if (sSubPlotIDX.length())
        {
            if (!_functions.call(sSubPlotIDX))
                throw SyntaxError(SyntaxError::FUNCTION_ERROR, sSubPlotIDX, SyntaxError::invalid_position);

            if (_data.containsTablesOrClusters(sSubPlotIDX))
            {
                getDataElements(sSubPlotIDX, _parser, _data);
            }

            _parser.SetExpr(sSubPlotIDX);
            int nRes = 0;
            const mu::StackItem* v = _parser.Eval(nRes);

            if (nRes == 1)
            {
                mu::Array plotIdx = v[0].get();

                if (plotIdx.getAsScalarInt() < 1)
                    plotIdx = mu::Array(1);

                if ((size_t)plotIdx.getAsScalarInt() - 1 >= nMultiplots[0]*nMultiplots[1])
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, (size_t)(plotIdx.getAsScalarInt() - 1), nMultiCols, nMultiLines))
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                _graph->MultiPlot(nMultiplots[0], nMultiplots[1], plotIdx.getAsScalarInt() - 1, nMultiCols, nMultiLines,
                                  _pData.getSettings(PlotData::STR_PLOTBOUNDARIES).c_str());
            }   // cols, lines
            else
            {
                if ((size_t)(v[1].get().getAsScalarInt() - 1 + (v[0].get().getAsScalarInt() - 1)*nMultiplots[1]) >= nMultiplots[0]*nMultiplots[1])
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, (size_t)((v[1].get().getAsScalarInt() - 1) + (v[0].get().getAsScalarInt() - 1)*nMultiplots[0]),
                                         nMultiCols, nMultiLines))
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                _graph->MultiPlot(nMultiplots[0], nMultiplots[1], (int)((v[1].get().getAsScalarInt() - 1) + (v[0].get().getAsScalarInt() - 1)*nMultiplots[0]),
                                  nMultiCols, nMultiLines, _pData.getSettings(PlotData::STR_PLOTBOUNDARIES).c_str());
            }
        }
        else
        {
            if (nSubPlots >= nMultiplots[0]*nMultiplots[1])
                throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

            int nPlotPos = 1;

            for (size_t nSub = 0; nSub < nMultiplots[0]*nMultiplots[1]; nSub++)
            {
                if (nPlotPos & nSubPlotMap)
                    nPlotPos <<= 1;
                else
                {
                    if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, nSub, nMultiCols, nMultiLines))
                        throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                    _graph->MultiPlot(nMultiplots[0], nMultiplots[1], nSub, nMultiCols, nMultiLines,
                                      _pData.getSettings(PlotData::STR_PLOTBOUNDARIES).c_str());
                    break;
                }

                if (nSub == nMultiplots[0]*nMultiplots[1] - 1)
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
            }
        }
    }
    else
    {
        if (sSubPlotIDX.length())
        {
            if (!_functions.call(sSubPlotIDX))
                throw SyntaxError(SyntaxError::FUNCTION_ERROR, sSubPlotIDX, SyntaxError::invalid_position);

            if (_data.containsTablesOrClusters(sSubPlotIDX))
                getDataElements(sSubPlotIDX, _parser, _data);

            _parser.SetExpr(sSubPlotIDX);
            int nRes = 0;
            const mu::StackItem* v = _parser.Eval(nRes);

            if (nRes == 1)
            {
                mu::Array plotIdx = v[0].get();

                if (plotIdx.getAsScalarInt() < 1)
                    plotIdx = mu::Value(1);

                if ((size_t)plotIdx.getAsScalarInt() - 1 >= nMultiplots[0]*nMultiplots[1])
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                if ((size_t)plotIdx.getAsScalarInt() != 1)
                    nRes <<= (size_t)(plotIdx.getAsScalarInt() - 1);

                if (nRes & nSubPlotMap)
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                nSubPlotMap |= nRes;
                _graph->SubPlot(nMultiplots[0], nMultiplots[1], plotIdx.getAsScalarInt() - 1, _pData.getSettings(PlotData::STR_PLOTBOUNDARIES).c_str());
            }
            else
            {
                if ((size_t)(v[1].get().getAsScalarInt() - 1 + (v[0].get().getAsScalarInt() - 1)*nMultiplots[0]) >= nMultiplots[0]*nMultiplots[1])
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                nRes = 1;

                if ((size_t)(v[1].get().getAsScalarInt() + (v[0].get().getAsScalarInt() - 1)*nMultiplots[0]) != 1)
                    nRes <<= (size_t)((v[1].get().getAsScalarInt() - 1) + (v[0].get().getAsScalarInt() - 1) * nMultiplots[0]);

                if (nRes & nSubPlotMap)
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

                nSubPlotMap |= nRes;
                _graph->SubPlot(nMultiplots[0], nMultiplots[1], (int)((v[1].get().getAsScalarInt() - 1) + (v[0].get().getAsScalarInt() - 1)*nMultiplots[0]),
                                _pData.getSettings(PlotData::STR_PLOTBOUNDARIES).c_str());
            }
        }
        else
        {
            if (nSubPlots >= nMultiplots[0]*nMultiplots[1])
                throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);

            int nPlotPos = 1;

            for (size_t nSub = 0; nSub < nMultiplots[0]*nMultiplots[1]; nSub++)
            {
                if (nPlotPos & nSubPlotMap)
                    nPlotPos <<= 1;
                else
                {
                    nSubPlotMap |= nPlotPos;
                    _graph->SubPlot(nMultiplots[0], nMultiplots[1], nSub, _pData.getSettings(PlotData::STR_PLOTBOUNDARIES).c_str());
                    break;
                }

                if (nSub == nMultiplots[0]*nMultiplots[1] - 1)
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function writes a message
/// to the terminal indicating that the algorithm
/// is currently working.
///
/// \param bAnimateVar bool
/// \return void
///
/////////////////////////////////////////////////
void Plot::displayMessage(bool bAnimateVar)
{
    if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
        NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("PLOT_CALCULATING_DATA_FOR") + " "));

    if (_pInfo.sCommand == "surface3d"
            || _pInfo.sCommand.substr(0, 6) == "surf3d"
            || _pInfo.sCommand == "meshgrid3d"
            || _pInfo.sCommand.substr(0, 6) == "mesh3d"
            || _pInfo.sCommand == "contour3d"
            || _pInfo.sCommand.substr(0, 6) == "cont3d"
            || _pInfo.sCommand == "density3d"
            || _pInfo.sCommand.substr(0, 6) == "dens3d"
            || _pInfo.sCommand == "gradient3d"
            || _pInfo.sCommand.substr(0, 6) == "grad3d")
    {
        _pInfo.b3D = true;
        if (_pInfo.nSamples > 71)
        {
            if (_pData.getSettings(PlotData::INT_HIGHRESLEVEL) == 2 && _pInfo.nSamples > 151)
                _pInfo.nSamples = 151;
            else if ((_pData.getSettings(PlotData::INT_HIGHRESLEVEL) == 1 || !_option.isDraftMode()) && _pInfo.nSamples > 100)
                _pInfo.nSamples = std::min(121, _pInfo.nSamples);
            else
                _pInfo.nSamples = 71;
        }
        if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "surf")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_SURFACE")) + "-");
        else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "mesh")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_MESHGRID")) + "-");
        else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "cont")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_CONTOUR")) + "-");
        else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "dens")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_DENSITY")) + "-");
        else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "grad")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_GRADIENT")) + "-");
    }
    else if (_pInfo.sCommand.substr(0, 6) == "vect3d" || _pInfo.sCommand == "vector3d")
    {
        _pInfo.b3DVect = true;
        if (_pInfo.nSamples > 11)
            _pInfo.nSamples = 11;
        if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_VECTOR")) + "-");
        if (_pData.getSettings(PlotData::LOG_PIPE) || _pData.getSettings(PlotData::LOG_FLOW))
        {
            if (_pInfo.nSamples % 2)
                _pInfo.nSamples -= 1;
        }
    }
    else if (_pInfo.sCommand.substr(0, 4) == "vect")
    {
        _pInfo.b2DVect = true;
        if (_pInfo.nSamples > 21)
            _pInfo.nSamples = 21;
        if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_VECTOR")) + "-");
    }
    else if (_pInfo.sCommand.substr(0, 4) == "mesh"
             || _pInfo.sCommand.substr(0, 4) == "surf"
             || _pInfo.sCommand.substr(0, 4) == "cont"
             || _pInfo.sCommand.substr(0, 4) == "grad"
             || _pInfo.sCommand.substr(0, 4) == "dens")
    {
        _pInfo.b2D = true;
        if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "surf")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_SURFACE")) + "-");
        else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "mesh")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_MESHGRID")) + "-");
        else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "cont")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_CONTOUR")) + "-");
        else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "dens")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_DENSITY")) + "-");
        else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "grad")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_GRADIENT")) + "-");
    }
    else if (_pInfo.sCommand == "plot3d")
    {
        if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
            NumeReKernel::printPreFmt("3D-");
    }
    else if (_pInfo.sCommand == "draw")
    {
        _pInfo.bDraw = true;
    }
    else if (_pInfo.sCommand == "draw3d")
    {
        _pInfo.bDraw3D = true;
        if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
            NumeReKernel::printPreFmt("3D-");
    }
    else if (_pInfo.sCommand == "implot")
        _pInfo.b2D = true;

    if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && !bAnimateVar && !(_pInfo.bDraw3D || _pInfo.bDraw) && _pInfo.sCommand != "implot")
        NumeReKernel::printPreFmt("Plot ... ");
    else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && _pInfo.sCommand == "implot")
        NumeReKernel::printPreFmt("Image plot ... ");
    else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints() && !bAnimateVar)
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_DRAWING")) + " ... ");
    else if (!_pData.getSettings(PlotData::LOG_SILENTMODE) && _option.systemPrints())
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_ANIMATION")) + " ... \n");

}


/////////////////////////////////////////////////
/// \brief This member function separates the
/// functions from the data plots and returns a
/// vector containing the definitions of the data
/// plots.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> Plot::separateFunctionsAndData()
{
    string sFuncTemp = sFunc;
    string sToken = "";

    while (sFuncTemp.length())
    {
        sToken = getNextArgument(sFuncTemp, true);
        StripSpaces(sToken);

        if (_data.containsTablesOrClusters(sToken))
        {
            std::string sErrTok = sToken.substr(0, sToken.find_first_of("#\"", getMatchingParenthesis(sToken)));
            StripSpaces(sErrTok);

            if (_data.containsTablesOrClusters(sToken.substr(0, sToken.find_first_of("({") + 1))
                && !_data.isTable(sToken.substr(0, sToken.find_first_of("({")))
                && !_data.isCluster(sToken.substr(0, sToken.find_first_of("({"))))
                throw SyntaxError(SyntaxError::DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING, sCurrentExpr, sErrTok, sToken);

            string sSubstr = sToken.substr(getMatchingParenthesis(StringView(sToken, sToken.find_first_of("({"))) + sToken.find_first_of("({") + 1);

            if (sSubstr[sSubstr.find_first_not_of(' ')] != '"' && sSubstr[sSubstr.find_first_not_of(' ')] != '#')
                throw SyntaxError(SyntaxError::DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING, sCurrentExpr, sErrTok, sToken);
        }
    }

    // --> Zerlegen von sFunc in Funktionenteile und Datenplots <--
    sFuncTemp = sFunc;
    sFunc.clear();

    std::vector<std::string> vDataPlots;

    while (sFuncTemp.length())
    {
        sToken = getNextArgument(sFuncTemp, true);
        size_t nPos = sToken.find_first_of("#\"");

        if (sToken.find_first_of("([{") < nPos)
            nPos = sToken.find_first_of("#\"", getMatchingParenthesis(sToken));

        // Ensure we don't have a string expression right here
        //if (!nPos || NumeReKernel::getInstance()->getStringParser().isStringExpression(sToken.substr(0, nPos)))
        //    throw SyntaxError(SyntaxError::CANNOT_PLOT_STRINGS, sCurrentExpr, sToken, sToken);

        if (_data.containsTablesOrClusters(sToken))
        {
            m_types.push_back(PT_DATA);
            m_manager.assets.push_back(PlotAsset());
            m_manager.assets.back().boundAxes = _pData.getAxisbind(m_manager.assets.size()-1);
            m_manager.assets.back().legend = sToken.substr(nPos);

            vDataPlots.push_back(sToken.substr(0, nPos));
        }
        else
        {
            m_types.push_back(PT_FUNCTION);
            m_manager.assets.push_back(PlotAsset());
            m_manager.assets.back().boundAxes = _pData.getAxisbind(m_manager.assets.size()-1);
            m_manager.assets.back().legend = sToken.substr(nPos);

            sFunc += "," + sToken.substr(0, nPos);
        }
    }

    sFunc.erase(0, 1);

    if (!_pInfo.b2DVect && !_pInfo.b3DVect && !_pInfo.b3D && !_pInfo.bDraw && !_pInfo.bDraw3D)
        createDataLegends();

    return vDataPlots;
}


/////////////////////////////////////////////////
/// \brief Creates the internal mglData objects
/// and fills them with the data values from the
/// MemoryManager class instance in the kernel.
///
/// \param vDataPlots const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void Plot::extractDataValues(const std::vector<std::string>& vDataPlots)
{
    size_t typeCounter = 0;
    size_t datIvlID = PlotAssetManager::REAL;
    size_t datIvlID3D = PlotAssetManager::REAL;

    if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM)
    {
        datIvlID = PlotAssetManager::REIM;
        datIvlID3D = PlotAssetManager::REIM;
    }
    else if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE)
    {
        datIvlID = PlotAssetManager::IMAG;
        datIvlID3D = PlotAssetManager::ABSREIM;
    }

    bool isTimeAxis[CRANGE+1] = {false, false, false, false};

    // Now extract the index informations of each referred
    // data object and copy its contents to the prepared
    // mglData objects
    for (size_t i = 0; i < vDataPlots.size(); i++)
    {
        size_t nParPos = vDataPlots[i].find_first_of("({");
        DataAccessParser _accessParser;

        while (m_types[typeCounter] != PT_DATA && typeCounter+1 < m_types.size())
            typeCounter++;

        if (nParPos == string::npos || m_types[typeCounter] != PT_DATA)
            throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCurrentExpr, vDataPlots[i], vDataPlots[i]);

        // Get the indices for the current data plot object optimized
        // for the plotting algorithm
        _accessParser = getAccessParserForPlotAndFit(vDataPlots[i]);

        Indices& _idx = _accessParser.getIndices();
        std::string& sDataTable = _accessParser.getDataObject();
        bool openEnd = _idx.col.isOpenEnd();

        // fill the open end indices
        _accessParser.evalIndices();

        if (!_accessParser.isCluster())
        {
            if (_idx.row.last() >= _data.getLines(sDataTable, false))
                _idx.row.setRange(0, _data.getLines(sDataTable, false)-1);

            if (_idx.col.last() >= _data.getCols(sDataTable) && _pInfo.sCommand != "plot3d")
                _idx.col.setRange(0, _data.getCols(sDataTable)-1);
        }
        else
        {
            if (_idx.row.last() >= (int)_data.getCluster(sDataTable).size())
                _idx.row.setRange(0, _data.getCluster(sDataTable).size()-1);
        }

        // Validize the indices depending on whether a cluster or
        // a table is used for the current data access
        if (_accessParser.isCluster())
        {
            if (_idx.row.front() >= (int)_data.getCluster(sDataTable).size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCurrentExpr, sDataTable, _idx.row.to_string() + ", " + _idx.col.to_string());
        }
        else
        {
            if (_idx.row.front() >= _data.getLines(sDataTable, false)
                || (_idx.col.front() >= _data.getCols(sDataTable) && _pInfo.sCommand != "plot3d"))
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCurrentExpr, sDataTable, _idx.row.to_string() + ", " + _idx.col.to_string());
        }

        if (isPlot1D(_pInfo.sCommand))
        {
            size_t datarows = 1;

            if (_pData.getSettings(PlotData::LOG_XERROR) && _pData.getSettings(PlotData::LOG_YERROR))
                datarows = 3;
            else if (_pData.getSettings(PlotData::LOG_XERROR) || _pData.getSettings(PlotData::LOG_YERROR))
                datarows = 2;
            else if (_pData.getSettings(PlotData::LOG_OHLC) || _pData.getSettings(PlotData::LOG_CANDLESTICK))
                datarows = 4;
            else if (_idx.col.numberOfNodes() >= 2)
                datarows = _idx.col.numberOfNodes() - 1*!_pData.getSettings(PlotData::LOG_BOXPLOT);

            m_manager.assets[typeCounter].create1DPlot(PT_DATA, _idx.row.size(), datarows);

            if (m_manager.assets[typeCounter].type == PT_NONE)
                throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, _accessParser.getDataObject(), _accessParser.getDataObject());

            // A vector index was used. Insert a column index
            // if the current plot is a boxplot or an axis coordinate
            // is missing
            if (_pData.getSettings(PlotData::LOG_BOXPLOT)
                || _idx.col.numberOfNodes() == 1
                || (_idx.col.size() < 5 && (_pData.getSettings(PlotData::LOG_OHLC) || _pData.getSettings(PlotData::LOG_CANDLESTICK))))
                _idx.col.prepend(std::vector<int>({-1}));

            if (_idx.col.front() == VectorIndex::INVALID)
            {
                for (size_t n = 0; n < _idx.row.size(); n++)
                    m_manager.assets[typeCounter].writeAxis(n+1.0, n, XCOORD);
            }
            else
            {
                mu::Array vAxis = getDataFromObject(_accessParser.getDataObject(),
                                                    _idx.row, _idx.col.front(),
                                                    _accessParser.isCluster());

                if (vAxis.getCommonType() == mu::TYPE_NUMERICAL && vAxis.getCommonNumericalType() == mu::DATETIME)
                    isTimeAxis[XRANGE] = true;

                for (size_t n = 0; n < vAxis.size(); n++)
                {
                    m_manager.assets[typeCounter].writeAxis(vAxis[n].getNum().asF64(), n, XCOORD);
                }
            }

            // Fill the mglData objects with the data from the
            // referenced data object
            if (datarows == 1 && _idx.col.numberOfNodes() == 2 && !openEnd)
            {
                mu::Array vVals = getDataFromObject(_accessParser.getDataObject(),
                                                    _idx.row, _idx.col.last(),
                                                    _accessParser.isCluster());

                if (vVals.getCommonType() == mu::TYPE_NUMERICAL && vVals.getCommonNumericalType() == mu::DATETIME)
                    isTimeAxis[YRANGE] = true;

                for (size_t n = 0; n < vVals.size(); n++)
                {
                    m_manager.assets[typeCounter].writeData(vVals[n].getNum().asCF64(), 0, n);
                }
            }
            else
            {
                for (size_t q = 0; q < datarows; q++)
                {
                    if (q >= _idx.col.size())
                    {
                        // Write zeroes for errorbars or similar
                        for (size_t n = 0; n < _idx.row.size(); n++)
                            m_manager.assets[typeCounter].writeData(0.0, q, n);
                    }
                    else if (_idx.col[q+1] == VectorIndex::INVALID)
                    {
                        for (size_t n = 0; n < _idx.row.size(); n++)
                            m_manager.assets[typeCounter].writeData(n+1.0, q, n);
                    }
                    else
                    {
                        mu::Array vVals = getDataFromObject(_accessParser.getDataObject(),
                                                            _idx.row, _idx.col[q+1],
                                                            _accessParser.isCluster());

                        if (vVals.getCommonType() == mu::TYPE_NUMERICAL && vVals.getCommonNumericalType() == mu::DATETIME)
                            isTimeAxis[YRANGE] = true;

                        for (size_t n = 0; n < vVals.size(); n++)
                        {
                            m_manager.assets[typeCounter].writeData(vVals[n].getNum().asCF64(), q, n);
                        }
                    }
                }
            }

            bool isHbar = _pData.getSettings(PlotData::FLOAT_HBARS) != 0.0;
            bool isMultiDataSet = _pData.getSettings(PlotData::FLOAT_HBARS) != 0.0
                || _pData.getSettings(PlotData::FLOAT_BARS) != 0.0
                || _pData.getSettings(PlotData::LOG_BOXPLOT)
                || _pData.getSettings(PlotData::LOG_OHLC)
                || _pData.getSettings(PlotData::LOG_CANDLESTICK)
                || !(_pData.getSettings(PlotData::LOG_XERROR) || _pData.getSettings(PlotData::LOG_YERROR));
            bool isStackedBars = _pData.getSettings(PlotData::LOG_STACKEDBARS)
                && (isHbar || _pData.getSettings(PlotData::FLOAT_BARS) != 0.0);


            // Calculate the data ranges
            if (_pData.getSettings(PlotData::LOG_PARAMETRIC) && typeCounter < (m_manager.assets.size() / 2) * 2)
            {
                Interval range = m_manager.assets[typeCounter].getDataIntervals(0)[datIvlID];

                for (size_t layer = 1; layer < std::max((size_t)1u, isMultiDataSet*m_manager.assets[typeCounter].getLayers()); layer++)
                {
                    range = range.combine(m_manager.assets[typeCounter].getDataIntervals(layer)[datIvlID]);
                }

                if (!(typeCounter % 2) && m_manager.assets[typeCounter].boundAxes.find('t') != std::string::npos)
                    secDataRanges[XRANGE+isHbar] = secDataRanges[XRANGE+isHbar].combine(range);

                dataRanges[(typeCounter+isHbar) % 2] = dataRanges[(typeCounter+isHbar) % 2].combine(range);
            }
            else
            {
                Interval axisrange = m_manager.assets[typeCounter].getAxisInterval(XCOORD);

                if (m_manager.assets[typeCounter].boundAxes.find('t') != std::string::npos)
                    secDataRanges[XRANGE+isHbar] = secDataRanges[XRANGE+isHbar].combine(axisrange);
                else
                    dataRanges[XRANGE+isHbar] = dataRanges[XRANGE+isHbar].combine(axisrange);

                if (isStackedBars)
                {
                    // For stacked bars, we first have to add all data up to get the correct
                    // interval for this data
                    mglData stacked = m_manager.assets[typeCounter].data.front().first;
                    Interval stackedInterval(stacked.Minimal(), stacked.Maximal());

                    for (size_t layer = 1; layer < m_manager.assets[typeCounter].getLayers(); layer++)
                    {
                        stacked += m_manager.assets[typeCounter].data[layer].first;
                        // Due to the fall-like representation, intermediate minimal and maximal
                        // values still count for the total minimal and maximal interval,
                        // therefore we combine all intermediate steps
                        stackedInterval = stackedInterval.combine(Interval(stacked.Minimal(),
                                                                           stacked.Maximal()));
                    }

                    if (m_manager.assets[typeCounter].boundAxes.find('r') != std::string::npos)
                        secDataRanges[YRANGE-isHbar] = secDataRanges[YRANGE-isHbar].combine(stackedInterval);
                    else
                        dataRanges[YRANGE-isHbar] = dataRanges[YRANGE-isHbar].combine(stackedInterval);
                }
                else
                {
                    for (size_t layer = 0; layer < std::max((size_t)1u, isMultiDataSet*m_manager.assets[typeCounter].getLayers()); layer++)
                    {
                        IntervalSet datIvl = m_manager.assets[typeCounter].getDataIntervals(layer);

                        if (m_manager.assets[typeCounter].boundAxes.find('r') != std::string::npos)
                            secDataRanges[YRANGE-isHbar] = secDataRanges[YRANGE-isHbar].combine(datIvl[datIvlID]);
                        else
                            dataRanges[YRANGE-isHbar] = dataRanges[YRANGE-isHbar].combine(datIvl[datIvlID]);
                    }
                }
            }
        }
        else if (isPlot3D(_pInfo.sCommand))
        {
            size_t datarows = 1;

            if (_pData.getSettings(PlotData::LOG_XERROR) && _pData.getSettings(PlotData::LOG_YERROR))
                datarows = 2;
            else if (_idx.col.numberOfNodes() > 2)
                datarows = _idx.col.numberOfNodes() - 1;

            m_manager.assets[typeCounter].create3DPlot(PT_DATA, _idx.row.size(), datarows);
            m_manager.assets[typeCounter].boundAxes = "lb";

            if (m_manager.assets[typeCounter].type == PT_NONE)
                throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, _accessParser.getDataObject(), _accessParser.getDataObject());

            for (size_t q = 0; q < 3*datarows; q++)
            {
                // If the column is invalid or completely empty, then
                // use zeros to fill it
                if (_idx.col[q] == VectorIndex::INVALID
                    || !_data.getColElements(_idx.col.subidx(q, 1), _accessParser.getDataObject()))
                {
                    for (size_t t = 0; t < _idx.row.size(); t++)
                        m_manager.assets[typeCounter].writeData(0.0, q, t);
                }
                else
                {
                    mu::Array vVals = getDataFromObject(_accessParser.getDataObject(),
                                                        _idx.row, _idx.col[q],
                                                        _accessParser.isCluster());

                    if (vVals.getCommonType() == mu::TYPE_NUMERICAL
                        && vVals.getCommonNumericalType() == mu::DATETIME
                        && q <= ZRANGE)
                        isTimeAxis[q] = true;

                    for (size_t t = 0; t < vVals.size(); t++)
                    {
                        m_manager.assets[typeCounter].writeData(vVals[t].getNum().asCF64(), q, t);
                    }
                }
            }

            // Calculate the data ranges
            if (_pData.getSettings(PlotData::LOG_PARAMETRIC) && typeCounter < (m_manager.assets.size() / 3) * 3)
                dataRanges[typeCounter%3] = dataRanges[typeCounter%3].combine(m_manager.assets[typeCounter].getDataIntervals(0)[datIvlID3D]);
            else
            {
                for (size_t c = XCOORD; c <= ZCOORD; c++)
                    dataRanges[c] = dataRanges[c].combine(m_manager.assets[typeCounter].getDataIntervals(c)[datIvlID]);
            }
        }
        else if (isMesh2D(_pInfo.sCommand))
        {
            bool isBars = _pInfo.sCommand == "dens" && _pData.getSettings(PlotData::FLOAT_BARS) > 0;
            std::vector<size_t> samples = _accessParser.getDataGridDimensions();

            // Density plots with bars enabled need
            // one additional element per axis
            if (isBars)
            {
                samples.front()++;
                samples.back()++;
            }

            m_manager.assets[typeCounter].create2DMesh(PT_DATA, samples, 1);
            m_manager.assets[typeCounter].boundAxes = "lb";

            if (m_manager.assets[typeCounter].type == PT_NONE)
                throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, _accessParser.getDataObject(), _accessParser.getDataObject());

            // Write the axes (do not write the additional value for
            // the bar-density mixture)
            for (size_t axis = 0; axis < samples.size(); axis++)
            {
                mu::Array vAxis = getDataFromObject(_accessParser.getDataObject(),
                                                    _idx.row, _idx.col[axis],
                                                    _accessParser.isCluster());

                if (vAxis.getCommonType() == mu::TYPE_NUMERICAL
                    && vAxis.getCommonNumericalType() == mu::DATETIME
                    && axis <= YRANGE)
                    isTimeAxis[axis] = true;

                for (size_t m = 0; m < std::min(samples[axis]-isBars, vAxis.size()); m++)
                {
                    m_manager.assets[typeCounter].writeAxis(vAxis[m].getNum().asF64(), m, (PlotCoords)axis);
                }
            }

            // For bars and density plots, we simply repeat the last
            // axis step distance to add another axis value
            if (isBars)
            {
                double diff = m_manager.assets[typeCounter].axes[XCOORD].a[samples.front()-2]
                    - m_manager.assets[typeCounter].axes[XCOORD].a[samples.front()-3];
                m_manager.assets[typeCounter].axes[XCOORD].a[samples.front()-1] = diff
                    + m_manager.assets[typeCounter].axes[XCOORD].a[samples.front()-2];

                diff = m_manager.assets[typeCounter].axes[YCOORD].a[samples.back()-2]
                    - m_manager.assets[typeCounter].axes[YCOORD].a[samples.back()-3];
                m_manager.assets[typeCounter].axes[YCOORD].a[samples.back()-1] = diff
                    + m_manager.assets[typeCounter].axes[YCOORD].a[samples.back()-2];
            }

            // Write the meshgrid data
            #pragma omp parallel for
            for (size_t y = 0; y < samples[1]-isBars; y++)
            {
                mu::Array vVals = getDataFromObject(_accessParser.getDataObject(),
                                                    _idx.row, _idx.col[y+2],
                                                    _accessParser.isCluster());

                if (vVals.getCommonType() == mu::TYPE_NUMERICAL
                    && vVals.getCommonNumericalType() == mu::DATETIME
                    && !isTimeAxis[ZRANGE])
                {
                    #pragma omp critical
                    {
                        isTimeAxis[ZRANGE] = true;
                        isTimeAxis[CRANGE] = true;
                    }
                }

                for (size_t x = 0; x < std::min(samples[0]-isBars, vVals.size()); x++)
                {
                    m_manager.assets[typeCounter].writeData(vVals[x].as_cmplx(), 0, x, y);
                }
            }

            // Calculate the data ranges
            if (_pData.getSettings(PlotData::LOG_PARAMETRIC) && typeCounter < (m_manager.assets.size() / 3) * 3)
                dataRanges[typeCounter%3] = dataRanges[typeCounter%3].combine(m_manager.assets[typeCounter].getDataIntervals(0)[datIvlID3D]);
            else
            {
                dataRanges[XRANGE] = dataRanges[XRANGE].combine(m_manager.assets[typeCounter].getAxisInterval(XCOORD));
                dataRanges[YRANGE] = dataRanges[YRANGE].combine(m_manager.assets[typeCounter].getAxisInterval(YCOORD));

                if (_pInfo.sCommand == "implot")
                    dataRanges[ZRANGE] = dataRanges[ZRANGE].combine(Interval(0.0, 255.0));
                else
                    dataRanges[ZRANGE] = dataRanges[ZRANGE].combine(m_manager.assets[typeCounter].getDataIntervals(0)[datIvlID3D]);
            }
        }
        else if (isVect2D(_pInfo.sCommand))
        {

            std::vector<size_t> samples = _accessParser.getDataGridDimensions();

            m_manager.assets[typeCounter].create2DMesh(PT_DATA, samples, 1);
            m_manager.assets[typeCounter].boundAxes = "lb";

            if (m_manager.assets[typeCounter].type == PT_NONE)
                throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, _accessParser.getDataObject(), _accessParser.getDataObject());

            // Write the axes
            for (size_t axis = 0; axis < samples.size(); axis++)
            {
                mu::Array vAxis = getDataFromObject(_accessParser.getDataObject(),
                                                    _idx.row, _idx.col[axis],
                                                    _accessParser.isCluster());

                if (vAxis.getCommonType() == mu::TYPE_NUMERICAL && vAxis.getCommonNumericalType() == mu::DATETIME)
                    isTimeAxis[axis] = true;

                for (size_t m = 0; m < std::min(samples[axis], vAxis.size()); m++)
                {
                    m_manager.assets[typeCounter].writeAxis(vAxis[m].getNum().asF64(), m, (PlotCoords)axis);
                }
            }

            // Write the complex vector data
            #pragma omp parallel for
            for (size_t y = 0; y < samples[1]; y++)
            {
                mu::Array vVals = getDataFromObject(_accessParser.getDataObject(),
                                                    _idx.row, _idx.col[y+2],
                                                    _accessParser.isCluster());

                for (size_t x = 0; x < std::min(samples[0], vVals.size()); x++)
                {
                    m_manager.assets[typeCounter].writeData(vVals[x].as_cmplx(), 0, x, y);
                }
            }

            // Calculate the data ranges
            //if (_pData.getSettings(PlotData::LOG_PARAMETRIC) && typeCounter < (m_manager.assets.size() / 3) * 3)
            //    dataRanges[typeCounter%3] = dataRanges[typeCounter%3].combine(m_manager.assets[typeCounter].getDataIntervals(0)[datIvlID3D]);
            //else
            //{
                dataRanges[XRANGE] = dataRanges[XRANGE].combine(m_manager.assets[typeCounter].getAxisInterval(XCOORD));
                dataRanges[YRANGE] = dataRanges[YRANGE].combine(m_manager.assets[typeCounter].getAxisInterval(YCOORD));
                dataRanges[ZRANGE] = dataRanges[ZRANGE].combine(m_manager.assets[typeCounter].getDataIntervals(0)[datIvlID3D]);
            //}
        }
        else
            throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, _accessParser.getDataObject(), _accessParser.getDataObject());

        typeCounter++;
    }

    // Ensure that we have at least minimal axis intervals
    if (dataRanges[XRANGE].range() == 0.0 && vDataPlots.size())
        dataRanges[XRANGE].expand(0.1);

    if (dataRanges[YRANGE].range() == 0.0 && vDataPlots.size() && !isPlot1D(_pInfo.sCommand))
        dataRanges[YRANGE].expand(0.1);

    // Auto-apply time axes
    for (size_t i = 0; i <= CRANGE; i++)
    {
        if (isTimeAxis[i] && !_pData.getTimeAxis(i).use && dataRanges[i].max() > 0.0)
        {
            TimeAxis a;

            // Detect the optimal axis ticks based upon
            // the corresponding axis range
            double range = dataRanges[i].range();

#warning TODO (numere#6#09/16/24): We might want to think about an improved axis stepping for a time axis
            if (range < 3600)
                a.activate("mm:ss");
            else if (range < 24*3600)
                a.activate("hh:mm");
            else if (range < 5*30*24*3600)
                a.activate("MM-DD");
            else if (range < 365*24*3600)
                a.activate("YYYY-MM-DD");
            else if (range < 3*365*24*3600)
                a.activate("YYYY-MM");
            else
                a.activate("YYYY");

            _pData.setTimeAxis(i, a);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// legend strings for the data plots.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::createDataLegends()
{
    // Examine all data labels
    for (size_t i = 0; i < m_manager.assets.size(); i++)
    {
        if (m_types[i] != PT_DATA)
            continue;

        // Extraxt the current label and
        // remove the surrounding quotation marks
        std::string sTemp = toInternalString(m_manager.assets[i].legend);

        // Try to find a data object in the current label
        if (_data.containsTables(sTemp)
                && (sTemp.find(',') != std::string::npos || sTemp.substr(sTemp.find('('), 2) == "()")
                && sTemp.find(')') != std::string::npos)
        {
            // Ensure that the referenced data object contains valid data
            if (_data.containsTablesOrClusters(sTemp) && !_data.isValid())
                throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCurrentExpr, sTemp, sTemp);

            // Strip all spaces and extract the table name
            StripSpaces(sTemp);
            DataAccessParser _access = getAccessParserForPlotAndFit(sTemp);
            //_access.evalIndices();
            std::string sTableName = _access.getDataObject();
            const VectorIndex& vCols = _access.getIndices().col;

            if (vCols.numberOfNodes() == 1)
                sTemp = toExternalString(_data.getTopHeadLineElement(vCols.front(), sTableName));
            else if (vCols.numberOfNodes() == 2)
            {
                // First and second index value available
                if (_pInfo.sCommand != "plot3d")
                {
                    // Standard plot
                    if (!(_pData.getSettings(PlotData::LOG_YERROR) || _pData.getSettings(PlotData::LOG_XERROR)))
                    {
                        // Handle here barcharts
                        if (_pData.getSettings(PlotData::FLOAT_BARS) || _pData.getSettings(PlotData::FLOAT_HBARS))
                        {
                            sTemp.clear();
                            vCols.setOpenEndIndex(_data.getCols(sTableName)-1);

                            // Don't use the first one
                            for (size_t i = 0; i < vCols.size(); i++)
                            {
                                if (sTemp.length())
                                    sTemp += "\n";

                                sTemp += _data.getTopHeadLineElement(vCols[i], sTableName);
                            }

                            sTemp = toExternalString(sTemp);
                        }
                        else
                        {
                            if (vCols.isOpenEnd())
                                sTemp = toExternalString(_data.getTopHeadLineElement(vCols[1], sTableName) + " vs. "
                                                         + _data.getTopHeadLineElement(vCols[0], sTableName));
                            else
                                sTemp = toExternalString(_data.getTopHeadLineElement(vCols.last(), sTableName) + " vs. "
                                                         + _data.getTopHeadLineElement(vCols.front(), sTableName));
                        }
                    }
                    else
                    {
                        sTemp = toExternalString(_data.getTopHeadLineElement(vCols[1], sTableName) + " vs. "
                                                 + _data.getTopHeadLineElement(vCols[0], sTableName));
                    }
                }
                else
                {
                    // three-dimensional plot
                    sTemp = toExternalString(_data.getTopHeadLineElement(vCols[0], sTableName) + ", "
                                             + _data.getTopHeadLineElement(vCols[1], sTableName) + ", "
                                             + _data.getTopHeadLineElement(vCols[2], sTableName));
                }
            }
            else if (vCols.numberOfNodes() == 3)
            {
                if (_pInfo.sCommand != "plot3d")
                {
                    // Standard plot
                    if (!(_pData.getSettings(PlotData::LOG_YERROR) || _pData.getSettings(PlotData::LOG_XERROR)))
                    {
                        // Handle here barcharts
                        if (_pData.getSettings(PlotData::FLOAT_BARS) || _pData.getSettings(PlotData::FLOAT_HBARS))
                        {
                            sTemp.clear();

                            // Don't use the first one
                            for (size_t i = 0; i < vCols.size(); i++)
                            {
                                if (sTemp.length())
                                    sTemp += "\n";

                                sTemp += _data.getTopHeadLineElement(vCols[i], sTableName);
                            }

                            sTemp = toExternalString(sTemp);
                        }
                        else
                            sTemp = toExternalString(_data.getTopHeadLineElement(vCols[1], sTableName) + " vs. "
                                                     + _data.getTopHeadLineElement(vCols[0], sTableName));
                    }
                    else
                    {
                        sTemp = toExternalString(_data.getTopHeadLineElement(vCols[1], sTableName) + " vs. "
                                                 + _data.getTopHeadLineElement(vCols[0], sTableName));
                    }
                }
                else
                {
                    // three-dimensional plot
                    std::vector<int> vNodes = vCols.getVector();
                    sTemp = toExternalString(_data.getTopHeadLineElement(vNodes[0], sTableName) + ", "
                                             + _data.getTopHeadLineElement(vNodes[1], sTableName) + ", "
                                             + _data.getTopHeadLineElement(vNodes[2], sTableName));
                }
            }
            else
                sTemp = toExternalString(sTemp);

            // Prepend backslashes before opening and closing
            // braces
            for (size_t i = 0; i < sTemp.size(); i++)
            {
                if (sTemp[i] == '{' || sTemp[i] == '}')
                {
                    sTemp.insert(i, 1, '\\');
                    i++;
                }
            }

            // Replace the data expression with the parsed headlines
            m_manager.assets[i].legend = sTemp;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function counts the valid
/// data points in the passed mglData object.
///
/// \param _mData const mglData&
/// \return size_t
///
/////////////////////////////////////////////////
size_t Plot::countValidElements(const mglData& _mData)
{
    size_t nElements = 0;

    // Go through the array
    for (int i = 0; i < _mData.nx; i++)
    {
        // count only non-nans
        if (!isnan(_mData.a[i]))
            nElements++;
    }

    // return the number of elements
    return nElements;
}


/////////////////////////////////////////////////
/// \brief This member function prepares the
/// memory array in the PlotData class instance
/// to fit the desired plotting style.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::prepareMemory()
{
    if (isPlot1D(_pInfo.sCommand))
    {
        for (size_t i = 0; i < m_types.size(); i++)
        {
            if (m_types[i] == PT_FUNCTION)
                m_manager.assets[i].create1DPlot(PT_FUNCTION, _pInfo.nSamples);
        }
    }
    else if (isPlot3D(_pInfo.sCommand))
    {
        for (size_t i = 0; i < m_types.size(); i++)
        {
            if (m_types[i] == PT_FUNCTION)
                m_manager.assets[i].create3DPlot(PT_FUNCTION, _pInfo.nSamples);
        }
    }
    else if (isMesh2D(_pInfo.sCommand))
    {
        for (size_t i = 0; i < m_types.size(); i++)
        {
            if (m_types[i] == PT_FUNCTION)
                m_manager.assets[i].create2DMesh(PT_FUNCTION, std::vector<size_t>(2, _pInfo.nSamples));
        }
    }
    else if (isMesh3D(_pInfo.sCommand))
    {
        for (size_t i = 0; i < m_types.size(); i++)
        {
            if (m_types[i] == PT_FUNCTION)
                m_manager.assets[i].create3DMesh(PT_FUNCTION, std::vector<size_t>(3, _pInfo.nSamples));
        }
    }
    else if (isVect2D(_pInfo.sCommand))
    {
        for (size_t i = 0; i < m_types.size(); i++)
        {
            if (m_types[i] == PT_FUNCTION)
                m_manager.assets[i].create2DVect(PT_FUNCTION, std::vector<size_t>(2, _pInfo.nSamples));
        }
    }
    else if (isVect3D(_pInfo.sCommand))
    {
        for (size_t i = 0; i < m_types.size(); i++)
        {
            if (m_types[i] == PT_FUNCTION)
                m_manager.assets[i].create3DVect(PT_FUNCTION, std::vector<size_t>(3, _pInfo.nSamples));
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function applies the
/// default ranges to the plotting ranges.
///
/// \param nPlotCompose size_t
/// \param bNewSubPlot bool
/// \return void
///
/// Default ranges are used, whenever the user
/// does not supply custom ones. If the current
/// plot contains data sets, then those ranges
/// are used as default ranges (if the user did
/// not supply custom ones).
/////////////////////////////////////////////////
void Plot::defaultRanges(size_t nPlotCompose, bool bNewSubPlot)
{
    if (!nPlotCompose || bNewSubPlot)
    {
        _pInfo.ranges = _pData.getRanges();

        // --> Standard-Ranges zuweisen: wenn weniger als i+1 Ranges gegeben sind und Datenranges vorhanden sind, verwende die Datenranges <--
        for (int i = XCOORD; i <= ZCOORD; i++)
        {
            if (_pInfo.bDraw3D || _pInfo.bDraw)///Temporary
                continue;

            if (m_manager.hasDataPlots() && (_pData.getGivenRanges() < i + 1 || !_pData.getRangeSetting(i)))
            {
                if ((isinf(dataRanges[i].front()) || isnan(dataRanges[i].front()))
                    && (isinf(secDataRanges[i].front()) || isnan(secDataRanges[i].front()))
                    && (unsigned)i < std::max(2u, _pInfo.nMaxPlotDim))
                {
                    clearData();
                    throw SyntaxError(SyntaxError::PLOTDATA_IS_NAN, sCurrentExpr, sCurrentExpr.find(' ')+1);
                }
                else if (!(isinf(dataRanges[i].front()) || isnan(dataRanges[i].front())))
                    _pInfo.ranges[i] = dataRanges[i];

                if (i < 2 && !isinf(secDataRanges[i].front()) && !isnan(secDataRanges[i].front()))
                    _pInfo.secranges[i] = secDataRanges[i];
            }
            else if (i)
                _pInfo.secranges[i] = secDataRanges[i];

            if (!isnan(_pData.getAddAxis(i).ivl.min()))
                _pInfo.secranges[i].reset(_pData.getAddAxis(i).ivl.min(), _pData.getAddAxis(i).ivl.max());
        }

        // --> Spezialfall: Wenn nur eine Range gegeben ist, verwende im 3D-Fall diese fuer alle drei benoetigten Ranges <--
        if (_pData.getGivenRanges() == 1 && (_pInfo.b3D || _pInfo.b3DVect))
        {
            _pInfo.ranges[YRANGE].reset(_pInfo.ranges[XRANGE].front(), _pInfo.ranges[XRANGE].back());
            _pInfo.ranges[ZRANGE].reset(_pInfo.ranges[XRANGE].front(), _pInfo.ranges[XRANGE].back());
        }
    }
    // --> Sonderkoordinatensaetze und dazu anzugleichende Ranges. Noch nicht korrekt implementiert <--
    if (_pData.getSettings(PlotData::INT_COORDS) == CARTESIAN)
    {
        /* --> Im Falle logarithmischer Plots muessen die Darstellungsintervalle angepasst werden. Falls
         *     die Intervalle zu Teilen im Negativen liegen, versuchen wir trotzdem etwas sinnvolles
         *     daraus zu machen. <--
         */
        for (size_t i = XRANGE; i <= ZRANGE; i++)
        {
            if (_pData.getLogscale(i) && (i != ZRANGE || (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect)))
            {
                if (_pInfo.ranges[i].min() <= 0 && _pInfo.ranges[i].max() > 0)
                    _pInfo.ranges[i].reset(_pInfo.ranges[i].max() * 1e-3, _pInfo.ranges[i].max());
                else if (_pInfo.ranges[i].min() < 0 && _pInfo.ranges[i].max() <= 0)
                {
                    clearData();
                    throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCurrentExpr, SyntaxError::invalid_position);
                }
            }

        }
    }
    else if (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN)
    {
        // --> Im Falle polarer oder sphaerischer Koordinaten muessen die Darstellungsintervalle angepasst werden <--
        if (_pData.getSettings(PlotData::INT_COORDS) == POLAR_PZ
            || _pData.getSettings(PlotData::INT_COORDS) == POLAR_RP
            || _pData.getSettings(PlotData::INT_COORDS) == POLAR_RZ)
        {
            if (_pInfo.sCommand.find("3d") == string::npos && !_pInfo.b2DVect)
            {
                int nRCoord = ZCOORD;
                int nPhiCoord = XCOORD;

                if (_pData.getSettings(PlotData::INT_COORDS) == POLAR_RP)
                {
                    nRCoord = XCOORD;
                    nPhiCoord = YRANGE;
                }
                else if (_pData.getSettings(PlotData::INT_COORDS) == POLAR_RZ && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect))
                {
                    nRCoord = XCOORD;
                    nPhiCoord = ZCOORD;
                }
                else if (!(_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect))
                    nRCoord = YRANGE;

                if (!_pData.getRangeSetting(nRCoord))
                    _pInfo.ranges[nRCoord].reset(0.0, _pInfo.ranges[nRCoord].max());

                if (!_pData.getRangeSetting(nPhiCoord))
                    _pInfo.ranges[nPhiCoord].reset(0.0, 2.0 * M_PI);
            }
            else
            {
                _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());

                if (!_pData.getRangeSetting(YRANGE))
                    _pInfo.ranges[YRANGE].reset(0.0, 2.0 * M_PI);
            }
        }
        else if (_pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_PT
                 || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RP
                 || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RT)
        {
            if (_pInfo.sCommand.find("3d") == string::npos)
            {
                int nRCoord = ZCOORD;
                int nPhiCoord = XCOORD;
                int nThetaCoord = YCOORD;

                if (_pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RP)
                {
                    nRCoord = XCOORD;
                    nPhiCoord = YCOORD;
                    nThetaCoord = ZCOORD;
                }
                else if (_pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RT && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect))
                {
                    nRCoord = XCOORD;
                    nPhiCoord = ZCOORD;
                    nThetaCoord = YCOORD;
                }
                else if (!(_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect))
                {
                    nRCoord = YCOORD;
                    nThetaCoord = ZCOORD;
                }

                if (!_pData.getRangeSetting(nRCoord))
                    _pInfo.ranges[nRCoord].reset(0.0, _pInfo.ranges[nRCoord].max());

                if (!_pData.getRangeSetting(nPhiCoord))
                    _pInfo.ranges[nPhiCoord].reset(0.0, 2.0 * M_PI);

                if (!_pData.getRangeSetting(nThetaCoord))
                    _pInfo.ranges[nThetaCoord].reset(0.0, M_PI);
            }
            else
            {
                _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());

                if (!_pData.getRangeSetting(YRANGE))
                    _pInfo.ranges[YRANGE].reset(0.0, 2.0 * M_PI);

                if (!_pData.getRangeSetting(ZCOORD))
                    _pInfo.ranges[ZRANGE].reset(0.0, M_PI);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// plotting data points from usual expressions.
///
/// \param dt_max double
/// \param t_animate int
/// \return void
///
/////////////////////////////////////////////////
void Plot::fillData(double dt_max, int t_animate)
{
    const mu::StackItem* vResults = nullptr;

    if (!sFunc.length())
        return;

    std::vector<size_t> vFuncMap;

    for (size_t i = 0; i < m_types.size(); i++)
    {
        if (m_types[i] == PT_FUNCTION)
            vFuncMap.push_back(i);
    }

    if (isPlot1D(_pInfo.sCommand))
    {
        if (sFunc.find('{') != string::npos && !_pInfo.bDraw3D && !_pInfo.bDraw)
            convertVectorToExpression(sFunc);

        // Necessary, because of evaluatio of the legend entries
        _parser.SetExpr(sFunc);

        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getLogscale(XRANGE))
                    _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE].log(x, _pInfo.nSamples);
                else
                    _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE](x, _pInfo.nSamples);
            }

            vResults = _parser.Eval(_pInfo.nFunctions);

            if ((size_t)_pInfo.nFunctions != vFuncMap.size())
                throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, sCurrentExpr.find(' ')+1);

            for (int i = 0; i < _pInfo.nFunctions; i++)
            {
                m_manager.assets[vFuncMap[i]].writeAxis(_defVars.vValue[XCOORD][0].front().getNum().asF64(), x, XCOORD);
                m_manager.assets[vFuncMap[i]].writeData(vResults[i].get().front().getNum().asCF64(), 0, x);
            }
        }
    }
    else if (isPlot3D(_pInfo.sCommand))
    {
        EndlessVector<std::string> expressions = getAllArguments(sFunc);

        for (size_t k = 0; k < vFuncMap.size(); k++)
        {
            _defVars.vValue[XCOORD][0] = mu::Value(0.0);
            _defVars.vValue[YCOORD][0] = mu::Value(0.0);
            _defVars.vValue[ZCOORD][0] = mu::Value(0.0);

            if (expressions[k].find('{') != string::npos && !_pInfo.bDraw3D && !_pInfo.bDraw)
                convertVectorToExpression(expressions[k]);

            _parser.SetExpr(expressions[k]);
            vResults = _parser.Eval(_pInfo.nFunctions);

            _defVars.vValue[TCOORD][0] = _pInfo.ranges[TRANGE].front();
            _defVars.vValue[XCOORD][0] = vResults[XCOORD].get();

            if (YCOORD < _pInfo.nFunctions)
                _defVars.vValue[YCOORD][0] = vResults[YCOORD].get();

            if (ZCOORD < _pInfo.nFunctions)
                _defVars.vValue[ZCOORD][0] = vResults[ZCOORD].get();

            int nRenderSamples = _pInfo.nSamples;

            for (int t = 0; t < nRenderSamples; t++)
            {
                if (t != 0)
                {
                    if (_pData.getAnimateSamples())
                    {
                        double dSamples = 1.0;

                        if ((t_animate * 100.0) / _pData.getAnimateSamples() <= 25.0)
                            dSamples = (double)(_pInfo.nSamples - 1) * 0.25;
                        else if ((t_animate * 100.0) / _pData.getAnimateSamples() <= 50.0)
                            dSamples = (double)(_pInfo.nSamples - 1) * 0.5;
                        else if ((t_animate * 100.0) / _pData.getAnimateSamples() <= 75.0)
                            dSamples = (double)(_pInfo.nSamples - 1) * 0.75;
                        else
                            dSamples = (double)(_pInfo.nSamples - 1);

                        nRenderSamples = (int)dSamples + 1;
                        _defVars.vValue[TCOORD][0] += (mu::Value(dt_max) - _pInfo.ranges[TRANGE].front()) / mu::Value(dSamples);
                    }
                    else
                        _defVars.vValue[TCOORD][0] = mu::Value(_pInfo.ranges[TRANGE](t, _pInfo.nSamples));

                    _defVars.vValue[XCOORD][0] = mu::Value(m_manager.assets[vFuncMap[k]].data[XCOORD].first.a[t-1]);
                    _defVars.vValue[YCOORD][0] = mu::Value(m_manager.assets[vFuncMap[k]].data[YCOORD].first.a[t-1]);
                    _defVars.vValue[ZCOORD][0] = mu::Value(m_manager.assets[vFuncMap[k]].data[ZCOORD].first.a[t-1]);
                }

                // --> Wir werten alle Koordinatenfunktionen zugleich aus und verteilen sie auf die einzelnen Parameterkurven <--
                vResults = _parser.Eval(_pInfo.nFunctions);

                for (int i = XCOORD; i <= ZCOORD; i++)
                {
                    if (i >= _pInfo.nFunctions)
                        m_manager.assets[vFuncMap[k]].writeData(0.0, i, t);
                    else
                        m_manager.assets[vFuncMap[k]].writeData(vResults[i].get().front().getNum().asCF64(), i, t);
                }
            }

            for (int t = nRenderSamples; t < _pInfo.nSamples; t++)
            {
                for (int i = XCOORD; i <= ZCOORD; i++)
                    m_manager.assets[vFuncMap[k]].writeData(NAN, i, t);
            }
        }

        _defVars.vValue[TCOORD][0] = mu::Value(dt_max);
    }
    else if (isMesh2D(_pInfo.sCommand))
    {
        if (sFunc.find('{') != string::npos && !_pInfo.bDraw3D && !_pInfo.bDraw)
            convertVectorToExpression(sFunc);

        // Necessary, because of evaluatio of the legend entries
        _parser.SetExpr(sFunc);

        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getLogscale(XRANGE))
                    _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE].log(x, _pInfo.nSamples);
                else
                    _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE](x, _pInfo.nSamples);
            }

            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (_pData.getLogscale(YRANGE))
                    _defVars.vValue[YCOORD][0] = _pInfo.ranges[YRANGE].log(y, _pInfo.nSamples);
                else
                    _defVars.vValue[YCOORD][0] = _pInfo.ranges[YRANGE](y, _pInfo.nSamples);

                vResults = _parser.Eval(_pInfo.nFunctions);

                if ((size_t)_pInfo.nFunctions != vFuncMap.size())
                    throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, sCurrentExpr.find(' ')+1);

                for (size_t i = 0; i < vFuncMap.size(); i++)
                {
                    m_manager.assets[vFuncMap[i]].writeAxis(_defVars.vValue[XCOORD][0].front().getNum().asF64(), x, XCOORD);
                    m_manager.assets[vFuncMap[i]].writeAxis(_defVars.vValue[YCOORD][0].front().getNum().asF64(), y, YCOORD);
                    m_manager.assets[vFuncMap[i]].writeData(vResults[i].get().front().getNum().asCF64(), 0, x, y);
                }
            }
        }
    }
    else if (isMesh3D(_pInfo.sCommand))
    {
        if (sFunc.find('{') != string::npos && !_pInfo.bDraw3D && !_pInfo.bDraw)
            convertVectorToExpression(sFunc);

        // Necessary, because of evaluatio of the legend entries
        _parser.SetExpr(sFunc);

        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getLogscale(XRANGE))
                    _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE].log(x, _pInfo.nSamples);
                else
                    _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE](x, _pInfo.nSamples);
            }

            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (_pData.getLogscale(YRANGE))
                    _defVars.vValue[YCOORD][0] = _pInfo.ranges[YRANGE].log(y, _pInfo.nSamples);
                else
                    _defVars.vValue[YCOORD][0] = _pInfo.ranges[YRANGE](y, _pInfo.nSamples);

                for (int z = 0; z < _pInfo.nSamples; z++)
                {
                    if (_pData.getLogscale(ZRANGE))
                        _defVars.vValue[ZCOORD][0] = _pInfo.ranges[ZRANGE].log(z, _pInfo.nSamples);
                    else
                        _defVars.vValue[ZCOORD][0] = _pInfo.ranges[ZRANGE](z, _pInfo.nSamples);

                    vResults = _parser.Eval(_pInfo.nFunctions);

                    if ((size_t)_pInfo.nFunctions != vFuncMap.size())
                        throw SyntaxError(SyntaxError::PLOT_ERROR, sCurrentExpr, sCurrentExpr.find(' ')+1);

                    for (size_t i = 0; i < vFuncMap.size(); i++)
                    {
                        m_manager.assets[vFuncMap[i]].writeAxis(_defVars.vValue[XCOORD][0].front().getNum().asF64(), x, XCOORD);
                        m_manager.assets[vFuncMap[i]].writeAxis(_defVars.vValue[YCOORD][0].front().getNum().asF64(), y, YCOORD);
                        m_manager.assets[vFuncMap[i]].writeAxis(_defVars.vValue[ZCOORD][0].front().getNum().asF64(), z, ZCOORD);
                        m_manager.assets[vFuncMap[i]].writeData(vResults[i].get().front().getNum().asCF64(), 0, x, y, z);
                    }
                }
            }
        }
    }
    else if (isVect2D(_pInfo.sCommand))
    {
        EndlessVector<std::string> expressions = getAllArguments(sFunc);

        for (size_t k = 0; k < vFuncMap.size(); k++)
        {
            if (expressions[k].find('{') != string::npos && !_pInfo.bDraw3D && !_pInfo.bDraw)
                convertVectorToExpression(expressions[k]);

            _parser.SetExpr(expressions[k]);

            for (int x = 0; x < _pInfo.nSamples; x++)
            {
                _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE](x, _pInfo.nSamples);

                for (int y = 0; y < _pInfo.nSamples; y++)
                {
                    _defVars.vValue[YCOORD][0] = _pInfo.ranges[YRANGE](y, _pInfo.nSamples);
                    vResults = _parser.Eval(_pInfo.nFunctions);

                    for (int i = 0; i < 2; i++)
                    {
                        m_manager.assets[vFuncMap[k]].writeAxis(_defVars.vValue[XCOORD][0].front().getNum().asF64(), x, XCOORD);
                        m_manager.assets[vFuncMap[k]].writeAxis(_defVars.vValue[YCOORD][0].front().getNum().asF64(), y, YCOORD);

                        if (_pInfo.nFunctions <= i) // Always fill missing dimensions with zero
                            m_manager.assets[vFuncMap[k]].writeData(0.0, i, x, y);
                        else
                            m_manager.assets[vFuncMap[k]].writeData(vResults[i].get().front().getNum().asCF64(), i, x, y);
                    }
                }
            }
        }
    }
    else if (isVect3D(_pInfo.sCommand))
    {
        EndlessVector<std::string> expressions = getAllArguments(sFunc);

        for (size_t k = 0; k < vFuncMap.size(); k++)
        {
            if (expressions[k].find('{') != string::npos && !_pInfo.bDraw3D && !_pInfo.bDraw)
                convertVectorToExpression(expressions[k]);

            _parser.SetExpr(expressions[k]);

            for (int x = 0; x < _pInfo.nSamples; x++)
            {
                _defVars.vValue[XCOORD][0] = _pInfo.ranges[XRANGE](x, _pInfo.nSamples);

                for (int y = 0; y < _pInfo.nSamples; y++)
                {
                    _defVars.vValue[YCOORD][0] = _pInfo.ranges[YRANGE](y, _pInfo.nSamples);

                    for (int z = 0; z < _pInfo.nSamples; z++)
                    {
                        _defVars.vValue[ZCOORD][0] = _pInfo.ranges[ZRANGE](z, _pInfo.nSamples);
                        vResults = _parser.Eval(_pInfo.nFunctions);

                        for (int i = 0; i < 3; i++)
                        {
                            m_manager.assets[vFuncMap[k]].writeAxis(_defVars.vValue[XCOORD][0].front().getNum().asF64(), x, XCOORD);
                            m_manager.assets[vFuncMap[k]].writeAxis(_defVars.vValue[YCOORD][0].front().getNum().asF64(), y, YCOORD);
                            m_manager.assets[vFuncMap[k]].writeAxis(_defVars.vValue[ZCOORD][0].front().getNum().asF64(), z, ZCOORD);

                            if (_pInfo.nFunctions <= i) // Always fill missing dimensions with zero
                                m_manager.assets[vFuncMap[k]].writeData(0.0, i, x, y, z);
                            else
                                m_manager.assets[vFuncMap[k]].writeData(vResults[i].get().front().getNum().asCF64(), i, x, y, z);
                        }
                    }
                }
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function fits the
/// remaining plotting ranges for the calculated
/// interval of data points.
///
/// \param nPlotCompose size_t
/// \param bNewSubPlot bool
/// \return void
///
/// E.g. if a usual plot is calculated and the
/// user did only supply the x range, then this
/// function will calculate the corresponding y
/// range (and extend it to about 10%).
/////////////////////////////////////////////////
void Plot::fitPlotRanges(size_t nPlotCompose, bool bNewSubPlot)
{
    /* --> Darstellungsintervalle anpassen: Wenn nicht alle vorgegeben sind, sollten die fehlenden
     *     passend berechnet werden. Damit aber kein Punkt auf dem oberen oder dem unteren Rahmen
     *     liegt, vergroessern wir das Intervall um 5% nach oben und 5% nach unten <--
     * --> Fuer Vektor- und 3D-Plots ist das allerdings recht sinnlos <--
     */
    if (sFunc.length())
    {
        IntervalSet functionIntervals = m_manager.getFunctionIntervals();

        if (isnan(functionIntervals[0].front())
            || isnan(functionIntervals[0].back())
            || isinf(functionIntervals[0].front())
            || isinf(functionIntervals[0].back()))
        {
            clearData();
            throw SyntaxError(SyntaxError::PLOTDATA_IS_NAN, sCurrentExpr, sCurrentExpr.find(' ')+1);
        }
    }

    bool isCmplxPlane = _pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE;
    size_t ivlID = PlotAssetManager::REAL;
    size_t ivlID3D = PlotAssetManager::REAL;

    if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_REIM)
    {
        ivlID = PlotAssetManager::REIM;
        ivlID3D = PlotAssetManager::REIM;
    }
    else if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE)
    {
        ivlID = PlotAssetManager::IMAG;
        ivlID3D = PlotAssetManager::ABSREIM;
    }

    if ((isPlot1D(_pInfo.sCommand) || isPlot3D(_pInfo.sCommand) || isMesh2D(_pInfo.sCommand))
        && (!nPlotCompose || bNewSubPlot))
    {
        if (isPlot1D(_pInfo.sCommand) && _pData.getGivenRanges() < 2)
        {
            if (sFunc.length())
            {
                IntervalSet funcIntLeft = m_manager.getFunctionIntervals(ONLYLEFT);
                IntervalSet funcIntRight = m_manager.getFunctionIntervals(ONLYRIGHT);

                m_manager.weightedRange(ONLYLEFT, funcIntLeft[ivlID]);
                m_manager.weightedRange(ONLYRIGHT, funcIntRight[ivlID]);

                if (m_manager.hasDataPlots())
                {
                    // It might happen that all data plots are assigned to the secondary axis
                    if (!isnan(dataRanges[YRANGE].front()))
                        _pInfo.ranges[YRANGE] = _pInfo.ranges[YRANGE].combine(funcIntLeft[ivlID]);
                    else
                        _pInfo.ranges[YRANGE] = funcIntLeft[ivlID];

                    _pInfo.secranges[YRANGE] = _pInfo.secranges[YRANGE].combine(funcIntRight[ivlID]);

                    if (isCmplxPlane)
                        _pInfo.ranges[XRANGE] = m_manager.getDataIntervals()[PlotAssetManager::REAL].combine(funcIntLeft[PlotAssetManager::REAL]);
                }
                else
                {
                    _pInfo.ranges[YRANGE] = funcIntLeft[ivlID];
                    _pInfo.secranges[YRANGE] = funcIntRight[ivlID];

                    if (!isnan(_pData.getAddAxis(YCOORD).ivl.min()))
                        _pInfo.secranges[YRANGE].reset(_pData.getAddAxis(YCOORD).ivl.min(), _pData.getAddAxis(YCOORD).ivl.max());

                    if (isCmplxPlane)
                        _pInfo.ranges[XRANGE] = funcIntLeft[PlotAssetManager::REAL];
                }

                if ((isnan(funcIntLeft[ivlID].front()) || isnan(_pInfo.ranges[YRANGE].front())) && isnan(dataRanges[YRANGE].front()))
                    _pInfo.ranges[YRANGE].reset(_pInfo.secranges[YRANGE].min(), _pInfo.secranges[YRANGE].max());
            }

            // Adapt ranges for barcharts to ensure that the zero is part of the interval
            if (_pData.getSettings(PlotData::FLOAT_HBARS)
                && _pInfo.ranges[XRANGE].min()*_pInfo.ranges[XRANGE].max() >= 0
                && !_pData.getRangeSetting(0))
            {
                if (std::abs(_pInfo.ranges[XRANGE].min()) < std::abs(_pInfo.ranges[XRANGE].max()))
                    _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                else
                    _pInfo.ranges[XRANGE].reset(_pInfo.ranges[XRANGE].min(), 0.0);
            }

            if (_pData.getSettings(PlotData::FLOAT_BARS)
                && _pInfo.ranges[YRANGE].min()*_pInfo.ranges[YRANGE].max() >= 0)
            {
                if (std::abs(_pInfo.ranges[YRANGE].min()) < std::abs(_pInfo.ranges[YRANGE].max()))
                    _pInfo.ranges[YRANGE].reset(0.0, _pInfo.ranges[YRANGE].max()*1.05);
                else
                    _pInfo.ranges[YRANGE].reset(_pInfo.ranges[YRANGE].min()*1.05, 0.0);
            }
            else
                _pInfo.ranges[YRANGE].expand(1.1, (_pData.getLogscale(YRANGE) || _pData.getSettings(PlotData::INT_COORDS) != CARTESIAN
                                                   ? 0.0 : -INFINITY));

            if (isCmplxPlane)
                _pInfo.ranges[XRANGE].expand(1.1);
        }
        else if (isPlot1D(_pInfo.sCommand) && _pData.getGivenRanges() >= 2 && isCmplxPlane)
        {
            if (sFunc.length())
            {
                IntervalSet funcInt = m_manager.getFunctionIntervals(ALLRANGES);

                m_manager.weightedRange(ALLRANGES, funcInt[ivlID]);

                if (m_manager.hasDataPlots())
                {
                    if (_pData.getRangeSetting(YRANGE))
                        _pInfo.ranges[XRANGE] = _pData.getRanges()[YRANGE].combine(m_manager.getDataIntervals()[PlotAssetManager::REAL].combine(funcInt[PlotAssetManager::REAL]));
                    else
                        _pInfo.ranges[XRANGE] = m_manager.getDataIntervals()[PlotAssetManager::REAL].combine(funcInt[PlotAssetManager::REAL]);

                    if (_pData.getRangeSetting(ZRANGE))
                        _pInfo.ranges[YRANGE] = _pData.getRanges()[ZRANGE].combine(m_manager.getDataIntervals()[PlotAssetManager::IMAG].combine(funcInt[PlotAssetManager::IMAG]));
                    else
                        _pInfo.ranges[YRANGE] = m_manager.getDataIntervals()[PlotAssetManager::IMAG].combine(funcInt[PlotAssetManager::IMAG]);
                }
                else
                {
                    if (_pData.getRangeSetting(YRANGE))
                        _pInfo.ranges[XRANGE] = _pData.getRanges()[YRANGE].combine(funcInt[PlotAssetManager::REAL]);
                    else
                        _pInfo.ranges[XRANGE] = funcInt[PlotAssetManager::REAL];

                    if (_pData.getRangeSetting(ZRANGE))
                        _pInfo.ranges[YRANGE] = _pData.getRanges()[ZRANGE].combine(funcInt[PlotAssetManager::IMAG]);
                    else
                        _pInfo.ranges[YRANGE] = funcInt[PlotAssetManager::IMAG];
                }
            }

            if (!_pData.getRangeSetting(ZRANGE))
                _pInfo.ranges[YRANGE].expand(1.1, (_pData.getLogscale(YRANGE) || _pData.getSettings(PlotData::INT_COORDS) != CARTESIAN
                                                   ? 0.0 : -INFINITY));

            if (!_pData.getRangeSetting(YRANGE))
                _pInfo.ranges[XRANGE].expand(1.1);
        }
        else if (isPlot3D(_pInfo.sCommand) && _pData.getGivenRanges() < 3)
        {
            if (sFunc.length())
            {
                for (int i = XRANGE; i <= ZRANGE; i++)
                {
                    IntervalSet coordIntervals = m_manager.getFunctionIntervals(i);

                    m_manager.weightedRange(i, coordIntervals[ivlID]);

                    if (_pData.getGivenRanges() >= i + 1 && _pData.getRangeSetting(i))
                        continue;

                    if (m_manager.hasDataPlots())
                    {
                        IntervalSet dataCoordIntervals = m_manager.getDataIntervals(i);
                        _pInfo.ranges[i] = dataCoordIntervals[ivlID].combine(coordIntervals[ivlID]);
                    }
                    else
                        _pInfo.ranges[i] = coordIntervals[ivlID];

                    _pInfo.ranges[i].expand(1.1, (_pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_PT
                                                  || (_pData.getSettings(PlotData::INT_COORDS) == POLAR_PZ && i < 2)
                                                  || (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN && !i) ? 0.0 : -INFINITY));
                }

                if (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN && _pInfo.ranges[YRANGE].min() != 0.0)
                    _pInfo.ranges[YRANGE].reset(0.0, _pInfo.ranges[YRANGE].max());

                if (_pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_PT && _pInfo.ranges[ZRANGE].min() != 0.0)
                    _pInfo.ranges[ZRANGE].reset(0.0, _pInfo.ranges[ZRANGE].max());
            }
        }
        else if (isMesh2D(_pInfo.sCommand) && _pData.getGivenRanges() < 3 && _pInfo.sCommand != "implot")
        {
            if (sFunc.length())
            {
                IntervalSet funcIntervals = m_manager.getFunctionIntervals();
                m_manager.weightedRange(ALLRANGES, funcIntervals[ivlID3D]);
                _pInfo.ranges[ZRANGE] = _pInfo.ranges[ZRANGE].combine(funcIntervals[ivlID3D]);

                if (!m_manager.hasDataPlots())
                    _pInfo.ranges[ZRANGE] = funcIntervals[ivlID3D];
            }

            _pInfo.ranges[ZRANGE].expand(1.1, (_pData.getLogscale(ZRANGE)
                                               || (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN && _pData.getSettings(PlotData::INT_COORDS) != POLAR_RP) ? 0.0 : -INFINITY));
        }

        if (isPlot1D(_pInfo.sCommand))
        {
            if (std::isnan(_pData.getAddAxis(YCOORD).ivl.min()))
                _pInfo.secranges[YRANGE].expand(1.1, (_pData.getLogscale(YRANGE) || _pData.getSettings(PlotData::INT_COORDS) != CARTESIAN ? 0.0 : -INFINITY));

            for (int i = XRANGE; i <= YRANGE; i++)
                _pData.setAddAxis(i, _pInfo.secranges[i]);
        }
    }
    else if (_pInfo.b2DVect && (!nPlotCompose || bNewSubPlot) && !(_pInfo.bDraw3D || _pInfo.bDraw))
    {
        if (sFunc.length())
        {
            if (_pData.getGivenRanges() < 3)
            {
                IntervalSet funcIntervals = m_manager.getFunctionIntervals();
                m_manager.weightedRange(ALLRANGES, funcIntervals[0]);
                _pInfo.ranges[ZRANGE] = funcIntervals[0];
            }

            if (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN)
            {
                _pInfo.ranges[YRANGE].reset(0.0, _pInfo.ranges[YRANGE].max());
                _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
            }
        }
    }

    if (_pData.getLogscale(XRANGE) || _pData.getLogscale(YRANGE) || _pData.getLogscale(ZRANGE))
    {
        for (size_t i = XRANGE; i <= ZRANGE; i++)
        {
            if (_pData.getLogscale(i) && (i != ZRANGE || (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect)))
            {
                if ((_pInfo.ranges[i].min() <= 0 && _pInfo.ranges[i].max() <= 0) || _pData.getAxisScale(i) <= 0.0)
                {
                    clearData();
                    throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCurrentExpr, SyntaxError::invalid_position);
                }
                else if (_pInfo.ranges[i].min() <= 0)
                    _pInfo.ranges[i].reset(_pInfo.ranges[i].max() * 1e-3, _pInfo.ranges[i].max());
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Clearing member function for
/// allocated memory on the heap.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::clearData()
{
    m_manager.assets.clear();
}


/////////////////////////////////////////////////
/// \brief Apply the needed interval modification
/// to switch to an optical "nice" and well
/// readable interval.
///
/// \param ivl Interval&
/// \param isLogarithmic bool
/// \return void
///
/////////////////////////////////////////////////
static void applyNiceAxis(Interval& ivl, bool isLogarithmic)
{
    if (isnan(ivl.front()))
        return;

    double absmax = std::max(std::abs(ivl.min()), std::abs(ivl.max()));
    double dScale = intPower(10.0, std::ceil(std::log10(absmax)));
    double dMin = ivl.min() / dScale;
    double dMax = ivl.max() / dScale;

    // This factor determines the number of "ticks" we'll
    // allow in the current interval. This default to 2
    double factor = 2.0; // 0 - 5 - 10

    // Switch to a different factor based upon the current range
    // of the interval
    if (ivl.range() < 0.2 * dScale)
        factor = 20.0; // 0 - 0.5 - 1 - ... - 10
    else if (ivl.range() < 0.5 * dScale)
        factor = 10.0; // 0 - 1 - 2 - ... - 10
    else if (ivl.range() < 0.8 * dScale)
        factor = 5.0; // 0 - 2 - 4 - ... - 10

    // <1.5 -> 1
    // <3 -> 2
    // <7 -> 5
    // <10 -> 10

    // Calculate the new minimal and maximal interval boundaries
    dMin = std::floor(dMin * factor) / factor * dScale;
    dMax = std::ceil(dMax * factor) / factor * dScale;

    // Logarithmic axes cannot be negative. We'll use the previously
    // selected lower boundary
    if (isLogarithmic && dMin <= 0.0)
        dMin = ivl.min();

    ivl.reset(dMin, dMax);
}


/////////////////////////////////////////////////
/// \brief This member function informs the
/// mglGraph object about the new plotting
/// interval ranges.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::passRangesToGraph()
{
    if (_pData.getSettings(PlotData::LOG_BOXPLOT) && !_pData.getRangeSetting() && m_manager.hasDataPlots())
    {
        size_t nLayers = 0;

        // Create the x axis by summing up all data layers
        for (size_t i = 0; i < m_manager.assets.size(); i++)
            nLayers += m_manager.assets[i].getLayers();

        _pInfo.ranges[XRANGE].reset(0, nLayers+1);
    }

    // Adapt ranges to fit in time scale
    for (size_t i = XRANGE; i <= CRANGE; i++)
    {
        if (_pData.getTimeAxis(i).use)
            _pInfo.ranges[i].reset(std::max(0.0, _pInfo.ranges[i].min()), std::max(0.0, _pInfo.ranges[i].max()));
    }

    // INSERT HERE AXIS LOGIC
    if (_pData.getSettings(PlotData::INT_AXIS) == AXIS_NICE)
    {
        for (size_t i = YRANGE; i <= ZRANGE; i++)
        {
            if (!_pData.getRangeSetting(i))
                applyNiceAxis(_pInfo.ranges[i], _pData.getLogscale(i));
        }
    }
    else if (_pData.getSettings(PlotData::INT_AXIS) == AXIS_EQUAL)
    {
        double range = _pInfo.ranges[XRANGE].range();
        double aspect = _pData.getSettings(PlotData::FLOAT_ASPECT);

        for (size_t i = YRANGE; i <= ZRANGE; i++)
        {
            if (!_pData.getRangeSetting(i) && !isnan(_pInfo.ranges[i].front()))
            {
                double newRange = range / aspect - _pInfo.ranges[i].range();
                _pInfo.ranges[i].reset(_pInfo.ranges[i].min() - 0.5*newRange,
                                       _pInfo.ranges[i].max() + 0.5*newRange);
            }
        }
    }

    if (_pData.getInvertion(XCOORD))
        _graph->SetRange('x', _pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE), _pInfo.ranges[XRANGE].min() / _pData.getAxisScale(XRANGE));
    else
        _graph->SetRange('x', _pInfo.ranges[XRANGE].min() / _pData.getAxisScale(XRANGE), _pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE));

    if (_pData.getInvertion(YCOORD))
        _graph->SetRange('y', _pInfo.ranges[YRANGE].max() / _pData.getAxisScale(YRANGE), _pInfo.ranges[YRANGE].min() / _pData.getAxisScale(YRANGE));
    else
        _graph->SetRange('y', _pInfo.ranges[YRANGE].min() / _pData.getAxisScale(YRANGE), _pInfo.ranges[YRANGE].max() / _pData.getAxisScale(YRANGE));

    if (_pData.getInvertion(ZCOORD))
        _graph->SetRange('z', _pInfo.ranges[ZRANGE].max() / _pData.getAxisScale(ZRANGE), _pInfo.ranges[ZRANGE].min() / _pData.getAxisScale(ZRANGE));
    else
        _graph->SetRange('z', _pInfo.ranges[ZRANGE].min() / _pData.getAxisScale(ZRANGE), _pInfo.ranges[ZRANGE].max() / _pData.getAxisScale(ZRANGE));

    if ((_pData.getSettings(PlotData::FLOAT_BARS) || _pData.getSettings(PlotData::FLOAT_HBARS))
        && m_manager.hasDataPlots()
        && !_pInfo.b2D)
    {
        bool isHbar = _pData.getSettings(PlotData::FLOAT_HBARS);
        int nMinbars = -1;

        for (size_t k = 0; k < m_manager.assets.size(); k++)
        {
            if (m_manager.assets[k].type == PT_DATA
                && (nMinbars == -1 || nMinbars > m_manager.assets[k].axes[XCOORD].nx))
                nMinbars = m_manager.assets[k].axes[XCOORD].nx;
        }

        if (nMinbars < 2)
            nMinbars = 2;

        double dBarWidth = _pInfo.ranges[XRANGE+isHbar].range() / (2.0 * (nMinbars - 1.0));

        if (_pData.getInvertion(XRANGE+isHbar))
            _graph->SetRange('x'+isHbar,
                             (_pInfo.ranges[XRANGE+isHbar].max()-dBarWidth) / _pData.getAxisScale(XRANGE+isHbar),
                             (_pInfo.ranges[XRANGE+isHbar].min()+dBarWidth) / _pData.getAxisScale(XRANGE+isHbar));
        else
            _graph->SetRange('x'+isHbar,
                             (_pInfo.ranges[XRANGE+isHbar].min()-dBarWidth) / _pData.getAxisScale(XRANGE+isHbar),
                             (_pInfo.ranges[XRANGE+isHbar].max()+dBarWidth) / _pData.getAxisScale(XRANGE+isHbar));

        if (_pInfo.sCommand == "plot3d")
        {
            dBarWidth = _pInfo.ranges[YRANGE].range() / (2.0 * (nMinbars - 1.0));

            if (_pData.getInvertion(YRANGE))
                _graph->SetRange('y',
                             (_pInfo.ranges[YRANGE].max() - dBarWidth) / _pData.getAxisScale(YRANGE),
                             (_pInfo.ranges[YRANGE].min() + dBarWidth) / _pData.getAxisScale(YRANGE));
            else
                _graph->SetRange('y',
                             (_pInfo.ranges[YRANGE].min() - dBarWidth) / _pData.getAxisScale(YRANGE),
                             (_pInfo.ranges[YRANGE].max() + dBarWidth) / _pData.getAxisScale(YRANGE));
        }
    }

    if (!isnan(_pInfo.ranges[CRANGE].front()) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
    {
        if (_pData.getLogscale(CRANGE) && ((_pInfo.ranges[CRANGE].min() <= 0.0 && _pInfo.ranges[CRANGE].max() <= 0.0) || _pData.getAxisScale(CRANGE) <= 0.0))
        {
            clearData();
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCurrentExpr, SyntaxError::invalid_position);
        }
        else if (_pData.getLogscale(CRANGE) && _pInfo.ranges[CRANGE].min() <= 0.0)
        {
            _graph->SetRange('c', _pInfo.ranges[CRANGE].max() * 1e-3 / _pData.getAxisScale(CRANGE),
                                  _pInfo.ranges[CRANGE].max() / _pData.getAxisScale(CRANGE));
            _pInfo.ranges[CRANGE].reset(_pInfo.ranges[CRANGE].max()*1e-3, _pInfo.ranges[CRANGE].max());
        }
        else if (_pData.getInvertion(CRANGE))
            _graph->SetRange('c', _pInfo.ranges[CRANGE].max() / _pData.getAxisScale(CRANGE), _pInfo.ranges[CRANGE].min() / _pData.getAxisScale(CRANGE));
        else
            _graph->SetRange('c', _pInfo.ranges[CRANGE].min() / _pData.getAxisScale(CRANGE), _pInfo.ranges[CRANGE].max() / _pData.getAxisScale(CRANGE));
    }
    else if (m_manager.hasDataPlots())
    {
        double dColorMin = dataRanges[ZRANGE].min() / _pData.getAxisScale(CRANGE);
        double dColorMax = dataRanges[ZRANGE].max() / _pData.getAxisScale(CRANGE);
        IntervalSet zfuncIntervals = m_manager.getFunctionIntervals(ZCOORD);

        m_manager.weightedRange(ZCOORD, zfuncIntervals[0]);
        double dMin = zfuncIntervals[0].min();
        double dMax = zfuncIntervals[0].max();

        if (dMax > dColorMax && sFunc.length() && _pInfo.sCommand == "plot3d")
            dColorMax = dMax / _pData.getAxisScale(CRANGE);

        if (dMin < dColorMin && sFunc.length() && _pInfo.sCommand == "plot3d")
            dColorMin = dMin / _pData.getAxisScale(CRANGE);

        if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0 && dColorMax <= 0.0)
        {
            clearData();
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCurrentExpr, SyntaxError::invalid_position);
        }
        else if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0)
        {
            dColorMin = dColorMax * 1e-3;
            _graph->SetRange('c', dColorMin, dColorMax + 0.05 * (dColorMax - dColorMin));
            _pInfo.ranges[CRANGE].reset(dColorMin, dColorMax + 0.05 * (dColorMax - dColorMin));
        }
        else if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            _pInfo.ranges[CRANGE].reset(dColorMin*0.95, dColorMax*1.05);
            _graph->SetRange('c', _pInfo.ranges[CRANGE].min(), _pInfo.ranges[CRANGE].max());
        }
        else if (_pInfo.sCommand == "implot")
        {
            _graph->SetRange('c', 0, 255.0);
            _pInfo.ranges[CRANGE].reset(0.0, 255.0);
        }
        else if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE)
        {
            _pInfo.ranges[CRANGE].reset(-M_PI, M_PI);
            _graph->SetRange('c', -M_PI, M_PI);
        }
        else
        {
            _pInfo.ranges[CRANGE].reset(dColorMin, dColorMax);
            _pInfo.ranges[CRANGE].expand(1.1);
            _graph->SetRange('c', _pInfo.ranges[CRANGE].min(), _pInfo.ranges[CRANGE].max());
        }
    }
    else
    {
        if ((_pInfo.b2DVect || _pInfo.b3DVect) && (_pData.getSettings(PlotData::LOG_FLOW) || _pData.getSettings(PlotData::LOG_PIPE)))
        {
            if (_pData.getLogscale(CRANGE))
                _graph->SetRange('c', 1e-3, 1.05);
            else
                _graph->SetRange('c', -1.05, 1.05);
        }
        else if (_pInfo.b2DVect || _pInfo.b3DVect)
        {
            if (_pData.getLogscale(CRANGE))
                _graph->SetRange('c', 1e-3, 1.05);
            else
                _graph->SetRange('c', -0.05, 1.05);
        }
        else
        {
            IntervalSet funcIntervals = m_manager.getFunctionIntervals();
            m_manager.weightedRange(ALLRANGES, funcIntervals[0]);
            double dMin = funcIntervals[0].min();
            double dMax = funcIntervals[0].max();

            if (_pData.getLogscale(CRANGE)
                && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect)
                && ((dMin <= 0.0 && dMax) || _pData.getAxisScale(3) <= 0.0))
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCurrentExpr, SyntaxError::invalid_position);
            }
            else if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dMin <= 0.0)
            {
                _pInfo.ranges[CRANGE].reset(dMax * 1e-3 / _pData.getAxisScale(CRANGE),
                                            (dMax + 0.05 * (dMax - dMin)) / _pData.getAxisScale(CRANGE));
            }
            else if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
            {
                _pInfo.ranges[CRANGE].reset(dMin * 0.95 / _pData.getAxisScale(CRANGE),
                                            dMax * 1.05 / _pData.getAxisScale(CRANGE));
            }
            else if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE)
                _pInfo.ranges[CRANGE].reset(-M_PI, M_PI);
            else
            {
                _pInfo.ranges[CRANGE].reset((dMin - 0.05 * (dMax - dMin)) / _pData.getAxisScale(CRANGE),
                                            (dMax + 0.05 * (dMax - dMin)) / _pData.getAxisScale(CRANGE));
            }
        }

        if (_pData.getSettings(PlotData::INT_AXIS) == AXIS_NICE
            && _pData.getSettings(PlotData::INT_COMPLEXMODE) != CPLX_PLANE)
            applyNiceAxis(_pInfo.ranges[CRANGE], _pData.getLogscale(CRANGE));

        _graph->SetRange('c', _pInfo.ranges[CRANGE].min(), _pInfo.ranges[CRANGE].max());
    }
    // --> Andere Parameter setzen (die i. A. von den bestimmten Ranges abghaengen) <--
    // --> Gitter-, Koordinaten- und Achsenbeschriftungen <--
    CoordSettings();

    if (_pData.getAxisScale(XRANGE) != 1.0
        || _pData.getAxisScale(YRANGE) != 1.0
        || _pData.getAxisScale(ZRANGE) != 1.0
        || _pData.getAxisScale(CRANGE) != 1.0)
    {
        if (_pData.getInvertion(XRANGE))
            _graph->SetRange('x', _pInfo.ranges[XRANGE].max(), _pInfo.ranges[XRANGE].min());
        else
            _graph->SetRange('x', _pInfo.ranges[XRANGE].min(), _pInfo.ranges[XRANGE].max());

        if (_pData.getInvertion(YRANGE))
            _graph->SetRange('y', _pInfo.ranges[YRANGE].max(), _pInfo.ranges[YRANGE].min());
        else
            _graph->SetRange('y', _pInfo.ranges[YRANGE].min(), _pInfo.ranges[YRANGE].max());

        if (_pData.getInvertion(ZRANGE))
            _graph->SetRange('z', _pInfo.ranges[ZRANGE].max(), _pInfo.ranges[ZRANGE].min());
        else
            _graph->SetRange('z', _pInfo.ranges[ZRANGE].min(), _pInfo.ranges[ZRANGE].max());

        if ((_pData.getSettings(PlotData::FLOAT_BARS) || _pData.getSettings(PlotData::FLOAT_HBARS))
            && m_manager.hasDataPlots()
            && !_pInfo.b2D)
        {
            bool isHbar = _pData.getSettings(PlotData::FLOAT_HBARS);
            int nMinbars = -1;

            for (size_t k = 0; k < m_manager.assets.size(); k++)
            {
                if (m_manager.assets[k].type == PT_DATA
                    && (nMinbars == -1 || nMinbars > m_manager.assets[k].axes[XCOORD].nx))
                    nMinbars = m_manager.assets[k].axes[XCOORD].nx;
            }

            if (nMinbars < 2)
                nMinbars = 2;

            double dBarWidth = _pInfo.ranges[XRANGE+isHbar].range() / (2.0 * (nMinbars - 1.0));

            if (_pData.getInvertion(XRANGE+isHbar))
                _graph->SetRange('x'+isHbar,
                                 _pInfo.ranges[XRANGE+isHbar].max() - dBarWidth,
                                 _pInfo.ranges[XRANGE+isHbar].min() + dBarWidth);
            else
                _graph->SetRange('x'+isHbar,
                                 _pInfo.ranges[XRANGE+isHbar].min() - dBarWidth,
                                 _pInfo.ranges[XRANGE+isHbar].max() + dBarWidth);

            if (_pInfo.sCommand == "plot3d")
            {
                dBarWidth = _pInfo.ranges[YRANGE].range() / (2.0 * (nMinbars - 1.0));

                if (_pData.getInvertion(YRANGE))
                    _graph->SetRange('y',
                                 _pInfo.ranges[YRANGE].max() - dBarWidth,
                                 _pInfo.ranges[YRANGE].min() + dBarWidth);
                else
                    _graph->SetRange('y',
                                 _pInfo.ranges[YRANGE].min() - dBarWidth,
                                 _pInfo.ranges[YRANGE].max() + dBarWidth);
            }
        }

        if (!isnan(_pInfo.ranges[CRANGE].front()) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            if (_pData.getLogscale(CRANGE) && _pData.getRanges()[CRANGE].min() <= 0.0)
                _pInfo.ranges[CRANGE].reset(_pData.getRanges()[CRANGE].max()*1e-3, _pData.getRanges()[CRANGE].max());
            else
                _pInfo.ranges[CRANGE] = _pData.getRanges()[CRANGE];

            if (_pData.getInvertion(CRANGE))
                _graph->SetRange('c', _pInfo.ranges[CRANGE].max(), _pInfo.ranges[CRANGE].min());
            else
                _graph->SetRange('c', _pInfo.ranges[CRANGE].min(), _pInfo.ranges[CRANGE].max());
        }
        else if (m_manager.hasDataPlots())
        {
            double dColorMin = dataRanges[ZRANGE].min();
            double dColorMax = dataRanges[ZRANGE].max();
            IntervalSet zfuncIntervals = m_manager.getFunctionIntervals(ZCOORD);
            m_manager.weightedRange(ZCOORD, zfuncIntervals[0]);

            double dMin = zfuncIntervals[0].min();
            double dMax = zfuncIntervals[0].max();

            if (dMax > dColorMax && sFunc.length() && _pInfo.sCommand == "plot3d")
                dColorMax = dMax;
            if (dMin < dColorMin && sFunc.length() && _pInfo.sCommand == "plot3d")
                dColorMin = dMin;

            if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0 && dColorMax <= 0.0)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCurrentExpr, SyntaxError::invalid_position);
            }
            else if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0)
            {
                dColorMin = dColorMax * 1e-3;
                _pInfo.ranges[CRANGE].reset(dColorMin, dColorMax + 0.05* (dColorMax - dColorMin));
            }
            else if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
                _pInfo.ranges[CRANGE].reset(dColorMin * 0.05, dColorMax * 1.05);
            else
            {
                _pInfo.ranges[CRANGE].reset(dColorMin, dColorMax);
                _pInfo.ranges[CRANGE].expand(1.1);
            }

            _graph->SetRange('c', _pInfo.ranges[CRANGE].min(), _pInfo.ranges[CRANGE].max());
        }
        else
        {
            IntervalSet funcIntervals = m_manager.getFunctionIntervals();
            m_manager.weightedRange(ALLRANGES, funcIntervals[0]);

            double dMin = funcIntervals[0].min();
            double dMax = funcIntervals[0].max();

            if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dMin <= 0.0 && dMax)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCurrentExpr, SyntaxError::invalid_position);
            }
            else if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dMin <= 0.0)
                _pInfo.ranges[CRANGE].reset(dMax*1e-3, dMax + 0.05 * (dMax - dMin));
            else if (_pData.getLogscale(CRANGE) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
                _pInfo.ranges[CRANGE].reset(dMin * 0.95, dMax * 1.05);
            else
            {
                _pInfo.ranges[CRANGE].reset(dMin, dMax);
                _pInfo.ranges[CRANGE].expand(1.1);
            }

            _graph->SetRange('c', _pInfo.ranges[CRANGE].min(), _pInfo.ranges[CRANGE].max());
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function applies the
/// colour bar to the plot.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::applyColorbar()
{
    if (_pData.getSettings(PlotData::LOG_COLORBAR) && (_pInfo.sCommand.substr(0, 4) == "grad" || _pInfo.sCommand.substr(0, 4) == "dens" || _pInfo.sCommand == "implot") && !_pInfo.b3D && !_pData.getSettings(PlotData::LOG_SCHEMATIC))
    {
        if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE)
        {
            _graph->SetRange('x', _pInfo.ranges[CRANGE].min()+1e-3, _pInfo.ranges[CRANGE].max()-1e-3);
            _graph->SetTicksVal('c', "--\\pi\n--\\pi/2\n0\n\\pi/2\n\\pi");
            _graph->SetRange('x', _pInfo.ranges[XRANGE].min(), _pInfo.ranges[XRANGE].max());
        }

        if (_pData.getAxisScale(CRANGE) != 1.0)
            _graph->SetRange('c', _pInfo.ranges[CRANGE].min() / _pData.getAxisScale(CRANGE), _pInfo.ranges[CRANGE].max() / _pData.getAxisScale(CRANGE));

        // --> In diesem Fall haetten wir gerne eine Colorbar fuer den Farbwert <--
        if (_pData.getSettings(PlotData::LOG_BOX))
        {
            if (!(_pData.getSettings(PlotData::LOG_CONTPROJ) && _pData.getSettings(PlotData::LOG_CONTFILLED)) && _pInfo.sCommand.substr(0, 4) != "dens" && _pInfo.sCommand != "implot")
                _graph->Colorbar(_pData.getColorSchemeLight("I>").c_str());
            else
                _graph->Colorbar(_pData.getColorScheme("I>").c_str());
        }
        else
        {
            if (!(_pData.getSettings(PlotData::LOG_CONTPROJ) && _pData.getSettings(PlotData::LOG_CONTFILLED)) && _pInfo.sCommand.substr(0, 4) != "dens" && _pInfo.sCommand != "implot")
                _graph->Colorbar(_pData.getColorSchemeLight(">").c_str());
            else
                _graph->Colorbar(_pData.getColorScheme().c_str());
        }

        if (_pData.getAxisScale(CRANGE) != 1.0)
            _graph->SetRange('c', _pInfo.ranges[CRANGE].min(), _pInfo.ranges[CRANGE].max());
    }
    else if (_pData.getSettings(PlotData::LOG_COLORBAR) && !_pData.getSettings(PlotData::LOG_SCHEMATIC)
             && (_pInfo.sCommand.substr(0, 4) == "mesh"
                 || _pInfo.sCommand.substr(0, 4) == "surf"
                 || _pInfo.sCommand.substr(0, 4) == "cont"
                 || _pInfo.sCommand.substr(0, 4) == "dens"
                 || _pInfo.sCommand.substr(0, 4) == "grad"
                 || (_pInfo.sCommand.substr(0, 6) == "plot3d" && (_pData.getSettings(PlotData::INT_MARKS) || _pData.getSettings(PlotData::LOG_CRUST)))
                )
            )
    {
        if (_pData.getSettings(PlotData::INT_COMPLEXMODE) == CPLX_PLANE)
        {
            _graph->SetRange('x', _pInfo.ranges[CRANGE].min()+1e-3, _pInfo.ranges[CRANGE].max()-1e-3);
            _graph->SetTicksVal('c', "--\\pi\n--\\pi/2\n0\n\\pi/2\n\\pi");
            _graph->SetRange('x', _pInfo.ranges[XRANGE].min(), _pInfo.ranges[XRANGE].max());
        }

        if (_pData.getAxisScale(CRANGE) != 1.0)
            _graph->SetRange('c', _pInfo.ranges[CRANGE].min() / _pData.getAxisScale(CRANGE), _pInfo.ranges[CRANGE].max() / _pData.getAxisScale(CRANGE));

        _graph->Colorbar(_pData.getColorScheme().c_str());

        if (_pData.getAxisScale(CRANGE) != 1.0)
            _graph->SetRange('c', _pInfo.ranges[CRANGE].min(), _pInfo.ranges[CRANGE].max());
    }
}


/////////////////////////////////////////////////
/// \brief This member function applies the light
/// effects to two- or three-dimensional plots.
/// The orientation of the light sources depend
/// on the dimensionality of the plot.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::applyLighting()
{
    if (_pInfo.sCommand.substr(0, 4) == "mesh"
        || _pInfo.sCommand.substr(0, 4) == "surf"
        || _pInfo.sCommand.substr(0, 4) == "cont"
        || _pInfo.b3D
        || _pInfo.b3DVect
        || _pData.getSettings(PlotData::LOG_PIPE)
        || _pInfo.bDraw3D
        || _pInfo.bDraw
        || (_pInfo.sCommand == "plot3d" && (_pData.getSettings(PlotData::LOG_CRUST) || _pData.getSettings(PlotData::INT_MARKS))))
    {
        // --> Licht- und Transparenz-Effekte <--
        if (_pInfo.sCommand.substr(0, 4) == "surf"
            || (_pInfo.sCommand.substr(0, 4) == "dens" && _pInfo.b3D && !_pData.getSettings(PlotData::LOG_CONTPROJ))
            || (_pInfo.sCommand.substr(0, 4) == "cont" && _pInfo.b3D && _pData.getSettings(PlotData::LOG_CONTFILLED) && !_pData.getSettings(PlotData::LOG_CONTPROJ))
            || _pInfo.sCommand == "plot3d"
            || _pInfo.bDraw3D
            || _pInfo.bDraw)
        {
            _graph->Alpha(_pData.getSettings(PlotData::LOG_ALPHA));
            _graph->SetAlphaDef(_pData.getSettings(PlotData::FLOAT_ALPHAVAL));
        }

        if (_pData.getSettings(PlotData::LOG_ALPHAMASK)
            && (_pInfo.sCommand.substr(0, 4) == "surf" || _pInfo.sCommand == "plot3d")
            && !_pData.getSettings(PlotData::LOG_ALPHA))
        {
            _graph->Alpha(true);
            _graph->SetAlphaDef(_pData.getSettings(PlotData::FLOAT_ALPHAVAL));
        }

        if (_pData.getSettings(PlotData::INT_LIGHTING))
        {
            _graph->Light(true);

            if (!_pData.getSettings(PlotData::LOG_PIPE) && !_pInfo.bDraw)
            {
                if (_pData.getSettings(PlotData::INT_LIGHTING) == 1)
                {
                    _graph->AddLight(0, mglPoint(0, 0, 1), 'w', 0.35);
                    _graph->AddLight(1, mglPoint(5, 30, 5), 'w', 0.15);
                    _graph->AddLight(2, mglPoint(5, -30, 5), 'w', 0.06);
                }
                else
                {
                    _graph->AddLight(0, mglPoint(0, 0, 1), 'w', 0.25);
                    _graph->AddLight(1, mglPoint(5, 30, 5), 'w', 0.1);
                    _graph->AddLight(2, mglPoint(5, -30, 5), 'w', 0.04);
                }
            }
            else
            {
                if (_pData.getSettings(PlotData::INT_LIGHTING) == 1)
                {
                    _graph->AddLight(1, mglPoint(-5, 5, 1), 'w', 0.4);
                    _graph->AddLight(2, mglPoint(-3, 3, 9), 'w', 0.1);
                    _graph->AddLight(3, mglPoint(5, -5, 1), 'w', 0.1);
                }
                else
                {
                    _graph->AddLight(1, mglPoint(-5, 5, 1), 'w', 0.2);
                    _graph->AddLight(2, mglPoint(-3, 3, 9), 'w', 0.05);
                    _graph->AddLight(3, mglPoint(5, -5, 1), 'w', 0.05);
                }

                _graph->Light(0, false);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// complex logscale logic applied to the
/// coordinate system of the plot.
///
/// \param bzLogscale bool
/// \return void
///
/////////////////////////////////////////////////
void Plot::setLogScale(bool bzLogscale)
{
    // --> Logarithmische Skalierung; ein bisschen Fummelei <--
    if (_pData.getSettings(PlotData::INT_COORDS) == CARTESIAN)
    {
        if ((_pData.getLogscale(XRANGE) || _pData.getLogscale(YRANGE) || _pData.getLogscale(ZRANGE)) || _pData.getLogscale(CRANGE))
            _graph->SetRanges(0.1, 10.0, 0.1, 10.0, 0.1, 10.0);

        _graph->SetFunc(_pData.getLogscale(XRANGE) ? "lg(x)" : "",
                        _pData.getLogscale(YRANGE) ? "lg(y)" : "",
                        _pData.getLogscale(ZRANGE) && bzLogscale ? "lg(z)" : "",
                        _pData.getLogscale(CRANGE) ? "lg(c)" : "");
    }
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// light source depending on the orientation of
/// the graph. It is not used.
///
/// \param dPhi double
/// \param dTheta double
/// \param nId int
/// \param cColor char
/// \param dBrightness double
/// \return void
///
/////////////////////////////////////////////////
void Plot::directionalLight(double dPhi, double dTheta, int nId, char cColor, double dBrightness)
{
    mglPoint _mPoint(0.0, 0.0, 0.0);
    mglPoint _mDirection(_pInfo.ranges[XRANGE].middle(), _pInfo.ranges[YRANGE].middle(), _pInfo.ranges[ZRANGE].middle());
    double dNorm = hypot(_pInfo.ranges[XRANGE].range() / 2.0, _pInfo.ranges[YRANGE].range() / 2.0);

    _mPoint.x = dNorm * cos(dPhi / 180.0 * M_PI + M_PI_4) * sin(dTheta / 180.0 * M_PI);
    _mPoint.y = dNorm * sin(dPhi / 180.0 * M_PI + M_PI_4) * sin(dTheta / 180.0 * M_PI);
    _mPoint.z = _pInfo.ranges[ZRANGE].max();

    _mPoint += _mDirection;

    if (_mPoint.x > _pInfo.ranges[XRANGE].max())
        _mPoint.x = _pInfo.ranges[XRANGE].max();

    if (_mPoint.x < _pInfo.ranges[XRANGE].min())
        _mPoint.x = _pInfo.ranges[XRANGE].min();

    if (_mPoint.y > _pInfo.ranges[YRANGE].max())
        _mPoint.y = _pInfo.ranges[YRANGE].max();

    if (_mPoint.y < _pInfo.ranges[YRANGE].min())
        _mPoint.y = _pInfo.ranges[YRANGE].min();

    _mDirection = _mDirection - _mPoint;

    _graph->AddLight(nId, _mPoint, _mDirection, cColor, dBrightness);
    return;
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// two coordinates in space, which describe the
/// cutting box. The cutting box depends on the
/// rotation of the graph.
///
/// \param dPhi double
/// \param nEdge int
/// \param nCoords int
/// \param b3D bool
/// \return mglPoint
///
/////////////////////////////////////////////////
mglPoint Plot::CalcCutBox(double dPhi, int nEdge, int nCoords, bool b3D)
{
    int z = ZCOORD;
    int r = XCOORD;

    // Determine the coordinates, which correspond to X and Y
    if (!b3D)
    {
        switch (nCoords)
        {
            case POLAR_PZ:
                z = YCOORD;
                r = ZCOORD;
                break;
            case POLAR_RP:
                z = ZCOORD;
                r = XCOORD;
                break;
            case POLAR_RZ:
                z = YCOORD;
                r = XCOORD;
                break;
            case SPHERICAL_PT:
                r = ZCOORD;
                break;
            case SPHERICAL_RP:
                r = XCOORD;
                break;
            case SPHERICAL_RT:
                r = XCOORD;
                break;
        }
    }

    // The cutting box depends on the rotation of the graph
    // and is oriented in a way that the cutted range is
    // always presented to the viewer.
    if (dPhi >= 0.0 && dPhi < 90.0)
    {
        if (!nEdge)
        {
            if (nCoords == CARTESIAN)
                return mglPoint(_pInfo.ranges[XRANGE].middle(),
                                _pInfo.ranges[YRANGE].min() - 0.1 * fabs(_pInfo.ranges[YRANGE].min()),
                                _pInfo.ranges[ZRANGE].min() - 0.1 * fabs(_pInfo.ranges[ZRANGE].min()));
            else if (nCoords == POLAR_PZ || nCoords == POLAR_RP || nCoords == POLAR_RZ)
                return createMglPoint(nCoords, 0.0, 1.5 * M_PI, _pInfo.ranges[z].min() - 0.1 * fabs(_pInfo.ranges[z].min()));
            else if (nCoords == SPHERICAL_PT || nCoords == SPHERICAL_RP || nCoords == SPHERICAL_RT)
                return createMglPoint(nCoords, 0.0, 1.5 * M_PI, 0.0, b3D);
        }
        else
        {
            if (nCoords == CARTESIAN)
                return mglPoint(_pInfo.ranges[XRANGE].max() + 0.1 * fabs(_pInfo.ranges[XRANGE].max()),
                                _pInfo.ranges[YRANGE].middle(),
                                _pInfo.ranges[ZRANGE].max() + 0.1 * fabs(_pInfo.ranges[ZRANGE].max()));
            else if (nCoords == POLAR_PZ || nCoords == POLAR_RP || nCoords == POLAR_RZ)
                return createMglPoint(nCoords, 1.1 * _pInfo.ranges[r].max(), 2.0 * M_PI, _pInfo.ranges[z].max() + 0.1 * fabs(_pInfo.ranges[z].max()), b3D);
            else if (nCoords == SPHERICAL_PT || nCoords == SPHERICAL_RP || nCoords == SPHERICAL_RT)
                return createMglPoint(nCoords, 1.1 * _pInfo.ranges[r].max(), 2.0 * M_PI, M_PI, b3D);
        }
    }
    else if (dPhi >= 90.0 && dPhi < 180.0)
    {
        if (!nEdge)
        {
            if (nCoords == CARTESIAN)
                return mglPoint(_pInfo.ranges[XRANGE].middle(),
                                _pInfo.ranges[YRANGE].middle(),
                                _pInfo.ranges[ZRANGE].min() - 0.1 * fabs(_pInfo.ranges[ZRANGE].min()));
            else if (nCoords == POLAR_PZ || nCoords == POLAR_RP || nCoords == POLAR_RZ)
                return createMglPoint(nCoords, 0.0, 0.0, _pInfo.ranges[z].min() - 0.1 * fabs(_pInfo.ranges[z].min()), b3D);
            else if (nCoords == SPHERICAL_PT || nCoords == SPHERICAL_RP || nCoords == SPHERICAL_RT)
                return createMglPoint(nCoords, 0.0, 0.0, 0.0, b3D);
        }
        else
        {
            if (nCoords == CARTESIAN)
                return mglPoint(_pInfo.ranges[XRANGE].max() + 0.1 * fabs(_pInfo.ranges[XRANGE].max()),
                                _pInfo.ranges[YRANGE].max() + 0.1 * fabs(_pInfo.ranges[YRANGE].max()),
                                _pInfo.ranges[ZRANGE].max() + 0.1 * fabs(_pInfo.ranges[ZRANGE].max()));
            else if (nCoords == POLAR_PZ || nCoords == POLAR_RP || nCoords == POLAR_RZ)
                return createMglPoint(nCoords, _pInfo.ranges[r].max() * 1.1, 0.5 * M_PI, _pInfo.ranges[z].max() + 0.1 * fabs(_pInfo.ranges[z].max()), b3D);
            else if (nCoords == SPHERICAL_PT || nCoords == SPHERICAL_RP || nCoords == SPHERICAL_RT)
                return createMglPoint(nCoords, 1.1 * _pInfo.ranges[r].max(), 0.5 * M_PI, M_PI, b3D);
        }
    }
    else if (dPhi >= 180.0 && dPhi < 270.0)
    {
        if (!nEdge)
        {
            if (nCoords == CARTESIAN)
                return mglPoint(_pInfo.ranges[XRANGE].min() - 0.1 * fabs(_pInfo.ranges[XRANGE].min()),
                                _pInfo.ranges[YRANGE].middle(),
                                _pInfo.ranges[ZRANGE].min() - 0.1 * fabs(_pInfo.ranges[ZRANGE].min()));
            else if (nCoords == POLAR_PZ || nCoords == POLAR_RP || nCoords == POLAR_RZ)
                return createMglPoint(nCoords, 0.0, 0.5 * M_PI, _pInfo.ranges[z].min() - 0.1 * fabs(_pInfo.ranges[z].min()), b3D);
            else if (nCoords == SPHERICAL_PT || nCoords == SPHERICAL_RP || nCoords == SPHERICAL_RT)
                return createMglPoint(nCoords, 0.0, 0.5 * M_PI, 0.0, b3D);
        }
        else
        {
            if (nCoords == CARTESIAN)
                return mglPoint(_pInfo.ranges[XRANGE].middle(),
                                _pInfo.ranges[YRANGE].max() + 0.1 * fabs(_pInfo.ranges[YRANGE].max()),
                                _pInfo.ranges[ZRANGE].max() + 0.1 * fabs(_pInfo.ranges[ZRANGE].max()));
            else if (nCoords == POLAR_PZ || nCoords == POLAR_RP || nCoords == POLAR_RZ)
                return createMglPoint(nCoords, 1.1 * _pInfo.ranges[r].max(), M_PI, _pInfo.ranges[z].max() + 0.1 * fabs(_pInfo.ranges[z].max()), b3D);
            else if (nCoords == SPHERICAL_PT || nCoords == SPHERICAL_RP || nCoords == SPHERICAL_RT)
                return createMglPoint(nCoords, 1.1 * _pInfo.ranges[r].max(), M_PI, M_PI, b3D);
        }
    }
    else
    {
        if (!nEdge)
        {
            if (nCoords == CARTESIAN)
                return mglPoint(_pInfo.ranges[XRANGE].min() - 0.1 * fabs(_pInfo.ranges[XRANGE].min()),
                                _pInfo.ranges[YRANGE].min() - 0.1 * fabs(_pInfo.ranges[YRANGE].min()),
                                _pInfo.ranges[ZRANGE].min() - 0.1 * fabs(_pInfo.ranges[ZRANGE].min()));
            else if (nCoords == POLAR_PZ || nCoords == POLAR_RP || nCoords == POLAR_RZ)
                return createMglPoint(nCoords, 0.0, M_PI, _pInfo.ranges[z].min() - 0.1 * fabs(_pInfo.ranges[z].min()), b3D);
            else if (nCoords == SPHERICAL_PT || nCoords == SPHERICAL_RP || nCoords == SPHERICAL_RT)
                return createMglPoint(nCoords, 0.0, M_PI, 0.0, b3D);
        }
        else
        {
            if (nCoords == CARTESIAN)
                return mglPoint(_pInfo.ranges[XRANGE].middle(),
                                _pInfo.ranges[YRANGE].middle(),
                                _pInfo.ranges[ZRANGE].max() + 0.1 * fabs(_pInfo.ranges[ZRANGE].max()));
            else if (nCoords == POLAR_PZ || nCoords == POLAR_RP || nCoords == POLAR_RZ)
                return createMglPoint(nCoords, 1.1 * _pInfo.ranges[r].max(), 1.5 * M_PI, _pInfo.ranges[z].max() + 0.1 * fabs(_pInfo.ranges[z].max()), b3D);
            else if (nCoords == SPHERICAL_PT || nCoords == SPHERICAL_RP || nCoords == SPHERICAL_RT)
                return createMglPoint(nCoords, 1.1 * _pInfo.ranges[r].max(), 1.5 * M_PI, M_PI, b3D);
        }
    }

    return mglPoint(0);
}


/////////////////////////////////////////////////
/// \brief This member function creates a
/// mglPoint instance depending on the chosen
/// coordinate system.
///
/// \param nCoords int
/// \param r double
/// \param phi double
/// \param theta double
/// \param b3D bool
/// \return mglPoint
///
/////////////////////////////////////////////////
mglPoint Plot::createMglPoint(int nCoords, double r, double phi, double theta, bool b3D)
{
    if (b3D)
        return mglPoint(r, phi, theta);

    switch (nCoords)
    {
        case POLAR_PZ:
            return mglPoint(phi, theta, r);
        case POLAR_RP:
            return mglPoint(r, phi, theta);
        case POLAR_RZ:
            return mglPoint(r, theta, phi);
        case SPHERICAL_PT:
            return mglPoint(phi, theta, r);
        case SPHERICAL_RP:
            return mglPoint(r, phi, theta);
        case SPHERICAL_RT:
            return mglPoint(r, theta, phi);
        default:
            return mglPoint(r, phi, theta);
    }

    return mglPoint();
}


/////////////////////////////////////////////////
/// \brief This member function determines the
/// positions for the projected density and
/// contour maps, which depend on the rotation of
/// the graph and are chosen in a way that they
/// are always in the background of the graph.
///
/// \param dPhi double
/// \param nEdge int
/// \return double
///
/////////////////////////////////////////////////
double Plot::getProjBackground(double dPhi, int nEdge)
{
    if (dPhi >= 0.0 && dPhi < 90.0)
    {
        if (!nEdge)
            return _pInfo.ranges[XRANGE].min();
        else
            return _pInfo.ranges[YRANGE].max();
    }
    else if (dPhi >= 90.0 && dPhi < 180.0)
    {
        if (!nEdge)
            return _pInfo.ranges[XRANGE].min();
        else
            return _pInfo.ranges[YRANGE].min();
    }
    else if (dPhi >= 180.0 && dPhi < 270.0)
    {
        if (!nEdge)
            return _pInfo.ranges[XRANGE].max();
        else
            return _pInfo.ranges[YRANGE].min();
    }
    else
    {
        if (!nEdge)
            return _pInfo.ranges[XRANGE].max();
        else
            return _pInfo.ranges[YRANGE].max();
    }

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief This member function prepares the
/// legend strings to fit the selected legend
/// style (only colours or only styles or both).
///
/// \param sLegend const string&
/// \return string
///
/////////////////////////////////////////////////
string Plot::getLegendStyle(const string& sLegend)
{
    if (_pData.getSettings(PlotData::INT_LEGENDSTYLE))
    {
        if (_pData.getSettings(PlotData::INT_LEGENDSTYLE) == 1)
            return sLegend.substr(0, 1) + "-";
        else if (_pData.getSettings(PlotData::INT_LEGENDSTYLE) == 2)
            return "k" + sLegend.substr(1);
        else
            return sLegend;
    }
    else
        return sLegend;
}


/////////////////////////////////////////////////
/// \brief This member function applies a
/// floating point modulo operator on the data
/// set.
///
/// \param _mData const mglData&
/// \param dDenominator double
/// \return mglData
///
/////////////////////////////////////////////////
mglData Plot::fmod(const mglData& _mData, double dDenominator)
{
    if (!getNN(_mData))
        return _mData;

    mglData _mReturn(_mData.nx, _mData.ny, _mData.nz);

    for (long int i = 0; i < getNN(_mData); i++)
    {
        _mReturn.a[i] = ::fmod(_mData.a[i], dDenominator);
        // Spezialfall fuer krummlinige Koordinaten: wandle negative Winkel in positive um
        if (_mReturn.a[i] < 0)
            _mReturn.a[i] += dDenominator;
    }

    return _mReturn;
}


/////////////////////////////////////////////////
/// \brief Static helper function to determine,
/// whether a box is needed due to secondary
/// axes.
///
/// \param _pData const PlotData&
/// \param _pInfo const PlotInfo&
/// \return bool
///
/////////////////////////////////////////////////
static bool hasSecAxisBox(const PlotData& _pData, const PlotInfo& _pInfo)
{
    return isPlot1D(_pInfo.sCommand)
        && _pData.getSettings(PlotData::INT_COORDS) == CARTESIAN
        && !_pData.getSettings(PlotData::LOG_SCHEMATIC)
        && (!isnan(_pData.getAddAxis(XCOORD).ivl.front()) || !isnan(_pData.getAddAxis(YCOORD).ivl.front()));
}


/////////////////////////////////////////////////
/// \brief This member function applies axes and
/// axis labels to the graph.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::CoordSettings()
{
    if (_pData.getSettings(PlotData::INT_AXIS) != AXIS_NONE)
    {
        for (int i = XRANGE; i <= CRANGE; i++)
        {
            if (_pData.getTickTemplate(i).length())
            {
                if (i < 3)
                    _graph->SetTickTempl('x' + i, _pData.getTickTemplate(i).c_str());
                else
                    _graph->SetTickTempl('c', _pData.getTickTemplate(i).c_str());
            }
            else if (_pInfo.ranges[i].max() / _pData.getAxisScale(i) < 1e-2
                     && _pInfo.ranges[i].max() / _pData.getAxisScale(i) >= 1e-3)
            {
                if (i < 3)
                    _graph->SetTickTempl('x' + i, "%g");
                else
                    _graph->SetTickTempl('c', "%g");
            }

            if (_pData.getCustomTick(i).length())
            {
                int nCount = 1;
                mglData _mAxisRange;

                if (i < 3)
                {
                    for (size_t n = 0; n < _pData.getCustomTick(i).length(); n++)
                    {
                        if (_pData.getCustomTick(i)[n] == '\n')
                            nCount++;
                    }

                    _mAxisRange.Create(nCount);

                    // Ranges fuer customn ticks anpassen
                    if (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN)
                    {
                        if (!(_pInfo.b2D || _pInfo.b3D || _pInfo.sCommand == "plot3d" || _pInfo.b3DVect || _pInfo.b2DVect))
                        {
                            _pInfo.ranges[XRANGE].reset(0.0, 2.0);
                            _pInfo.ranges[YRANGE].reset(0.0, _pInfo.ranges[YRANGE].max());
                        }
                        else if (_pInfo.sCommand.find("3d") != string::npos)
                        {
                            _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                            _pInfo.ranges[YRANGE].reset(0.0, 2.0);

                            if (_pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_PT
                                || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RP
                                || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RT)
                                _pInfo.ranges[ZRANGE].reset(0.0, 1.0);
                        }
                        else if (_pInfo.b2DVect)
                        {
                            _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                            _pInfo.ranges[YRANGE].reset(0.0, 2.0);
                        }
                        else
                        {
                            _pInfo.ranges[XRANGE].reset(0.0, 2.0);
                            _pInfo.ranges[ZRANGE].reset(0.0, _pInfo.ranges[ZRANGE].max());

                            if (_pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_PT
                                || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RP
                                || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RT)
                                _pInfo.ranges[YRANGE].reset(0.0, 1.0);
                        }
                    }

                    if (nCount == 1)
                        _mAxisRange.a[0] = _pInfo.ranges[i].middle();
                    else
                    {
                        for (int n = 0; n < nCount; n++)
                        {
                            _mAxisRange.a[n] = _pInfo.ranges[i](n, nCount).real();
                        }
                    }

                    _graph->SetTicksVal('x' + i, _mAxisRange, fromSystemCodePage(_pData.getCustomTick(i)).c_str());
                }
                else
                {
                    for (size_t n = 0; n < _pData.getCustomTick(i).length(); n++)
                    {
                        if (_pData.getCustomTick(i)[n] == '\n')
                            nCount++;
                    }

                    _mAxisRange.Create(nCount);

                    if (nCount == 1)
                        _mAxisRange.a[0] = _pInfo.ranges[i].middle();
                    else
                    {
                        for (int n = 0; n < nCount; n++)
                        {
                            _mAxisRange.a[n] = _pInfo.ranges[i](n, nCount).real();
                        }
                    }

                    _graph->SetTicksVal('c', _mAxisRange, fromSystemCodePage(_pData.getCustomTick(i)).c_str());
                }
            }

            if (_pData.getTimeAxis(i).use)
            {
                if (i < 3)
                    _graph->SetTicksTime('x' + i, 0, _pData.getTimeAxis(i).sTimeFormat.c_str());
                else
                    _graph->SetTicksTime('c', 0, _pData.getTimeAxis(i).sTimeFormat.c_str());
            }
        }

        if (_pData.getSettings(PlotData::LOG_BOX)
            || hasSecAxisBox(_pData, _pInfo)
            || _pData.getSettings(PlotData::INT_COORDS) != CARTESIAN)
        {
            if (isPlot1D(_pInfo.sCommand)) // standard plot
            {
                if (_pData.getSettings(PlotData::INT_COORDS) == CARTESIAN)
                {
                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    {
                        if (!isnan(_pData.getAddAxis(XCOORD).ivl.front()) || !isnan(_pData.getAddAxis(YCOORD).ivl.front()))
                        {
                            Axis _axis = _pData.getAddAxis(XCOORD);
                            double dAspect = _pData.getSettings(PlotData::FLOAT_ASPECT);

                            if (_axis.sLabel.length())
                            {
                                _graph->SetRanges(_axis.ivl.min(), _axis.ivl.max(),
                                                  _pInfo.ranges[YRANGE].min(), _pInfo.ranges[YRANGE].max());
                                _graph->SetOrigin(_axis.ivl.max(), _pInfo.ranges[YRANGE].max() + _pInfo.ranges[YRANGE].range()*0.05);
                                _graph->Axis("x", _axis.sStyle.c_str());
                                _graph->Label('x', fromSystemCodePage("#" + _axis.sStyle + "{" + _axis.sLabel + "}").c_str(), 0);
                            }

                            _axis = _pData.getAddAxis(YCOORD);

                            if (_axis.sLabel.length())
                            {
                                _graph->SetRanges(_pInfo.ranges[XRANGE].min(), _pInfo.ranges[XRANGE].max(),
                                                  _axis.ivl.min(), _axis.ivl.max());
                                _graph->SetOrigin(_pInfo.ranges[XRANGE].max() + _pInfo.ranges[XRANGE].range()*0.05 / dAspect, _axis.ivl.max());

                                _graph->Axis("y", _axis.sStyle.c_str());
                                _graph->Label('y', fromSystemCodePage("#" + _axis.sStyle + "{" + _axis.sLabel + "}").c_str(), 0);
                            }

                            _graph->SetRanges(_pInfo.ranges[XRANGE].min(), _pInfo.ranges[XRANGE].max(),
                                              _pInfo.ranges[YRANGE].min(), _pInfo.ranges[YRANGE].max(),
                                              _pInfo.ranges[ZRANGE].min(), _pInfo.ranges[ZRANGE].max());
                            _graph->SetOrigin(_pInfo.ranges[XRANGE].min(), _pInfo.ranges[YRANGE].min());
                        }

                        _graph->Axis("xy");
                    }
                }
                else
                {
                    if (_pData.getSettings(PlotData::INT_COORDS) == POLAR_RP
                        || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RP)
                    {
                        _graph->SetOrigin(_pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE), 0.0);
                        _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(YCOORD)).c_str(),
                                        CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(YCOORD)).c_str());
                        _graph->SetRange('y', 0.0, APPR_TWO / _pData.getAxisScale(YCOORD));
                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE));
                    }
                    else if (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN)
                    {
                        _graph->SetOrigin(0.0, _pInfo.ranges[YCOORD].max() / _pData.getAxisScale(YCOORD));
                        _graph->SetFunc(CoordFunc("y*cos(pi*x*$PS$)", _pData.getAxisScale(XRANGE)).c_str(),
                                        CoordFunc("y*sin(pi*x*$PS$)", _pData.getAxisScale(XRANGE)).c_str());
                        _graph->SetRange('x', 0.0, APPR_TWO / _pData.getAxisScale(XRANGE));
                        _graph->SetRange('y', 0.0, _pInfo.ranges[YCOORD].max() / _pData.getAxisScale(YCOORD));
                    }

                    applyGrid();

                    if (_pData.getSettings(PlotData::INT_COORDS) == POLAR_RP
                        || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RP)
                    {
                        _graph->SetFunc("x*cos(y)", "x*sin(y)");
                        _graph->SetRange('y', 0.0, 2.0 * M_PI);
                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max());
                    }
                    else if (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN)
                    {
                        _graph->SetFunc("y*cos(x)", "y*sin(x)");
                        _graph->SetRange('x', 0.0, 2.0 * M_PI);
                        _graph->SetRange('y', 0.0, _pInfo.ranges[YRANGE].max());
                    }

                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC)
                        || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                        || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                        || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph->Label('x', fromSystemCodePage(_pData.getAxisLabel(XCOORD)).c_str(), 0.25);

                        if (_pData.getSettings(PlotData::INT_COORDS) == POLAR_RP
                            || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RP)
                            _graph->Label('y', fromSystemCodePage(_pData.getAxisLabel(YCOORD)).c_str(), 0.0);
                        else
                            _graph->Label('y', fromSystemCodePage(_pData.getAxisLabel(ZCOORD)).c_str(), 0.0);
                    }

                    if (_pData.getSettings(PlotData::FLOAT_BARS) || _pData.getSettings(PlotData::LOG_AREA))
                        _graph->SetOrigin(0.0, 0.0);

                    _pInfo.ranges[XRANGE].reset(0.0, 2.0*M_PI);
                    _pInfo.ranges[YRANGE].reset(0.0, _pInfo.ranges[YRANGE].max());
                }
            }
            else if (isMesh3D(_pInfo.sCommand) || isVect3D(_pInfo.sCommand) || isPlot3D(_pInfo.sCommand)) // 3d-plots and plot3d and vect3d
            {
                if (_pData.getSettings(PlotData::INT_COORDS) == POLAR_PZ
                    || _pData.getSettings(PlotData::INT_COORDS) == POLAR_RP
                    || _pData.getSettings(PlotData::INT_COORDS) == POLAR_RZ)
                {
                    _graph->SetOrigin(_pInfo.ranges[XRANGE].max() / _pData.getAxisScale(), 0.0, _pInfo.ranges[ZRANGE].min() / _pData.getAxisScale(2));
                    _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(),
                                    CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(), "z");
                    _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max() / _pData.getAxisScale());
                    _graph->SetRange('y', 0.0, 1.9999999 / _pData.getAxisScale(1));
                    _graph->SetRange('z', _pInfo.ranges[ZRANGE].min() / _pData.getAxisScale(2),
                                          _pInfo.ranges[ZRANGE].max() / _pData.getAxisScale(2));

                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                        _graph->Axis();
                    else
                    {
                        _graph->SetTickLen(1e-20);
                        _graph->Axis("_");
                    }

                    _graph->Box();

                    if (_pInfo.b3DVect && _pData.getSettings(PlotData::INT_GRID))
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    else if (_pData.getSettings(PlotData::INT_GRID) == 1)
                        _graph->Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getSettings(PlotData::INT_GRID) == 2)
                    {
                        _graph->Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }

                    _graph->SetFunc("x*cos(y)", "x*sin(y)", "z");
                    _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max());
                    _graph->SetRange('y', 0.0, 2.0 * M_PI);
                    _graph->SetRange('z', _pInfo.ranges[ZRANGE].min(), _pInfo.ranges[ZRANGE].max());

                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC)
                        || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                        || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                        || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph->Label('x', fromSystemCodePage(_pData.getAxisLabel(ZCOORD)).c_str(), -0.5);
                        _graph->Label('y', fromSystemCodePage(_pData.getAxisLabel(XCOORD)).c_str(), (_pData.getRotateAngle(1) - 225.0) / 180.0);
                        _graph->Label('z', fromSystemCodePage(_pData.getAxisLabel(YCOORD)).c_str(), 0.0);
                    }

                    if (_pData.getSettings(PlotData::FLOAT_BARS) || _pData.getSettings(PlotData::LOG_AREA))
                        _graph->SetOrigin(0.0, 0.0, _pInfo.ranges[ZRANGE].min());

                    _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                    _pInfo.ranges[YRANGE].reset(0.0, 2.0*M_PI);

                }
                else if (_pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_PT
                         || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RP
                         || _pData.getSettings(PlotData::INT_COORDS) == SPHERICAL_RT)
                {
                    _graph->SetOrigin(_pInfo.ranges[XRANGE].max() / _pData.getAxisScale(), 0.0, 0.5 / _pData.getAxisScale(2));
                    _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(1), _pData.getAxisScale(2)).c_str(),
                                    CoordFunc("x*sin(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(1), _pData.getAxisScale(2)).c_str(),
                                    CoordFunc("x*cos(pi*z*$PS$)", 1.0, _pData.getAxisScale(2)).c_str());
                    _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max() / _pData.getAxisScale());
                    _graph->SetRange('y', 0.0, 1.9999999 / _pData.getAxisScale(1));
                    _graph->SetRange('z', 0.0, 0.9999999 / _pData.getAxisScale(2));

                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                        _graph->Axis();
                    else
                    {
                        _graph->SetTickLen(1e-20);
                        _graph->Axis("_");
                    }

                    _graph->Box();

                    if (_pInfo.b3DVect && _pData.getSettings(PlotData::INT_GRID))
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    else if (_pData.getSettings(PlotData::INT_GRID) == 1)
                        _graph->Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getSettings(PlotData::INT_GRID) == 2)
                    {
                        _graph->Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }

                    _graph->SetFunc("x*cos(y)*sin(z)", "x*sin(y)*sin(z)", "x*cos(z)");
                    _graph->SetOrigin(_pInfo.ranges[XRANGE].max(), 0.0, 0.5 * M_PI);
                    _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max());
                    _graph->SetRange('y', 0.0, 2.0 * M_PI);
                    _graph->SetRange('z', 0.0, 1.0 * M_PI);

                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC)
                        || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                        || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                        || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph->Label('x', fromSystemCodePage(_pData.getAxisLabel(ZCOORD)).c_str(), -0.4);
                        _graph->Label('y', fromSystemCodePage(_pData.getAxisLabel(XCOORD)).c_str(), (_pData.getRotateAngle(1) - 225.0) / 180.0);
                        _graph->Label('z', fromSystemCodePage(_pData.getAxisLabel(YCOORD)).c_str(), -0.9); // -0.4
                    }

                    if (_pData.getSettings(PlotData::FLOAT_BARS) || _pData.getSettings(PlotData::LOG_AREA))
                        _graph->SetOrigin(0.0, 0.0, 0.5 * M_PI);

                    _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                    _pInfo.ranges[YRANGE].reset(0.0, 2.0*M_PI);
                    _pInfo.ranges[ZRANGE].reset(0.0, M_PI);
                }
                else
                {
                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                        _graph->Axis("xyz");
                }
            }
            else if (isVect2D(_pInfo.sCommand)) // vect
            {
                if (_pData.getSettings(PlotData::INT_COORDS) != CARTESIAN)
                {
                    _graph->SetOrigin(_pInfo.ranges[XRANGE].max() / _pData.getAxisScale(), 0.0);
                    _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(),
                                    CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(1)).c_str());
                    _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max() / _pData.getAxisScale());
                    _graph->SetRange('y', 0.0, 1.9999999 / _pData.getAxisScale(1));

                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                        _graph->Axis("xy");
                    else
                    {
                        _graph->SetTickLen(1e-20);
                        _graph->Axis("xy_");
                    }

                    _graph->Box();

                    if (_pData.getSettings(PlotData::INT_GRID) == 1)
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());

                    _graph->SetFunc("x*cos(y)", "x*sin(y)");
                    _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max());
                    _graph->SetRange('y', 0.0, 2.0 * M_PI);

                    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC)
                            || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                            || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                            || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph->Label('x', fromSystemCodePage(_pData.getAxisLabel(ZCOORD)).c_str(), 0.0);
                        _graph->Label('y', fromSystemCodePage(_pData.getAxisLabel(XCOORD)).c_str(), 0.25);
                    }

                    _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                    _pInfo.ranges[YRANGE].reset(0.0, 2.0*M_PI);
                }
                else if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("xy");
            }
            else // 2d plots
            {
                switch (_pData.getSettings(PlotData::INT_COORDS))
                {
                    case POLAR_PZ:
                    {
                        _graph->SetOrigin(0.0,
                                          _pInfo.ranges[YRANGE].min() / _pData.getAxisScale(YRANGE),
                                          _pInfo.ranges[ZRANGE].max() / _pData.getAxisScale(ZRANGE));
                        _graph->SetFunc(CoordFunc("z*cos(pi*x*$PS$)", _pData.getAxisScale(XRANGE)).c_str(),
                                        CoordFunc("z*sin(pi*x*$PS$)", _pData.getAxisScale(XRANGE)).c_str(), "y");

                        _graph->SetRange('x', 0.0, APPR_TWO / _pData.getAxisScale(XRANGE));
                        _graph->SetRange('y', _pInfo.ranges[YRANGE].min() / _pData.getAxisScale(YRANGE),
                                              _pInfo.ranges[YRANGE].max() / _pData.getAxisScale(YRANGE));
                        _graph->SetRange('z', 0.0, _pInfo.ranges[ZRANGE].max() / _pData.getAxisScale(ZRANGE));

                        applyGrid();

                        _graph->SetFunc("z*cos(x)", "z*sin(x)", "y");

                        _graph->SetRange('x', 0.0, 2.0 * M_PI);
                        _graph->SetRange('y', _pInfo.ranges[YRANGE].min(), _pInfo.ranges[YRANGE].max());
                        _graph->SetRange('z', 0.0, _pInfo.ranges[ZRANGE].max());

                        _pInfo.ranges[XRANGE].reset(0.0, 2.0*M_PI);
                        _pInfo.ranges[ZRANGE].reset(0.0, _pInfo.ranges[ZRANGE].max());
                        break;
                    }
                    case POLAR_RP:
                    {
                        _graph->SetOrigin(_pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE),
                                          0.0,
                                          _pInfo.ranges[ZRANGE].min() / _pData.getAxisScale(ZRANGE));
                        _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(YRANGE)).c_str(),
                                        CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(YRANGE)).c_str(), "z");

                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE));
                        _graph->SetRange('y', 0.0, APPR_TWO / _pData.getAxisScale(YRANGE));
                        _graph->SetRange('z', _pInfo.ranges[ZRANGE].min() / _pData.getAxisScale(ZRANGE),
                                              _pInfo.ranges[ZRANGE].max() / _pData.getAxisScale(ZRANGE));

                        applyGrid();

                        _graph->SetFunc("x*cos(y)", "x*sin(y)", "z");

                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max());
                        _graph->SetRange('y', 0.0, 2.0 * M_PI);
                        _graph->SetRange('z', _pInfo.ranges[ZRANGE].min(), _pInfo.ranges[ZRANGE].max());

                        _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                        _pInfo.ranges[YRANGE].reset(0.0, 2.0 * M_PI);
                        break;
                    }
                    case POLAR_RZ:
                    {
                        _graph->SetOrigin(_pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE),
                                          _pInfo.ranges[YRANGE].min() / _pData.getAxisScale(YRANGE),
                                          0.0);
                        _graph->SetFunc(CoordFunc("x*cos(pi*z*$PS$)", _pData.getAxisScale(ZRANGE)).c_str(),
                                        CoordFunc("x*sin(pi*z*$PS$)", _pData.getAxisScale(ZRANGE)).c_str(), "y");

                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE));
                        _graph->SetRange('y', _pInfo.ranges[YRANGE].min() / _pData.getAxisScale(YRANGE),
                                              _pInfo.ranges[YRANGE].max() / _pData.getAxisScale(YRANGE));
                        _graph->SetRange('z', 0.0, APPR_TWO / _pData.getAxisScale(ZRANGE));

                        applyGrid();

                        _graph->SetFunc("x*cos(z)", "x*sin(z)", "y");

                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max());
                        _graph->SetRange('y', _pInfo.ranges[YRANGE].min(), _pInfo.ranges[YRANGE].max());
                        _graph->SetRange('z', 0.0, 2.0 * M_PI);

                        _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                        _pInfo.ranges[ZRANGE].reset(0.0, 2.0*M_PI);
                        break;
                    }
                    case SPHERICAL_PT:
                    {
                        _graph->SetOrigin(0.0,
                                          0.5 / _pData.getAxisScale(YRANGE),
                                          _pInfo.ranges[ZRANGE].max() / _pData.getAxisScale(ZRANGE));
                        _graph->SetFunc(CoordFunc("z*cos(pi*x*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(XRANGE), _pData.getAxisScale(YRANGE)).c_str(),
                                        CoordFunc("z*sin(pi*x*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(XRANGE), _pData.getAxisScale(YRANGE)).c_str(),
                                        CoordFunc("z*cos(pi*y*$TS$)", 1.0, _pData.getAxisScale(YRANGE)).c_str());

                        _graph->SetRange('x', 0.0, APPR_TWO / _pData.getAxisScale(XRANGE));
                        _graph->SetRange('y', 0.0, APPR_ONE / _pData.getAxisScale(YRANGE));
                        _graph->SetRange('z', 0.0, _pInfo.ranges[ZRANGE].max() / _pData.getAxisScale(ZRANGE));

                        applyGrid();

                        _graph->SetFunc("z*cos(x)*sin(y)", "z*sin(x)*sin(y)", "z*cos(y)");
                        _graph->SetOrigin(0.0, 0.5 * M_PI, _pInfo.ranges[ZRANGE].max());
                        _graph->SetRange('x', 0.0, 2.0 * M_PI);
                        _graph->SetRange('y', 0.0, 1.0 * M_PI);
                        _graph->SetRange('z', 0.0, _pInfo.ranges[ZRANGE].max());

                        _pInfo.ranges[XRANGE].reset(0.0, 2.0*M_PI);
                        _pInfo.ranges[YRANGE].reset(0.0, M_PI);
                        _pInfo.ranges[ZRANGE].reset(0.0, _pInfo.ranges[ZRANGE].max());
                        break;
                    }
                    case SPHERICAL_RP:
                    {
                        _graph->SetOrigin(_pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE),
                                          0.0,
                                          0.5 / _pData.getAxisScale(ZRANGE));
                        _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(YRANGE), _pData.getAxisScale(ZRANGE)).c_str(),
                                        CoordFunc("x*sin(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(YRANGE), _pData.getAxisScale(ZRANGE)).c_str(),
                                        CoordFunc("x*cos(pi*z*$TS$)", 1.0, _pData.getAxisScale(ZRANGE)).c_str());

                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE));
                        _graph->SetRange('y', 0.0, APPR_TWO / _pData.getAxisScale(YRANGE));
                        _graph->SetRange('z', 0.0, APPR_ONE / _pData.getAxisScale(ZRANGE));

                        applyGrid();

                        _graph->SetFunc("x*cos(y)*sin(z)", "x*sin(y)*sin(z)", "x*cos(z)");
                        _graph->SetOrigin(_pInfo.ranges[XRANGE].max(), 0.0, 0.5 * M_PI);
                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max());
                        _graph->SetRange('y', 0.0, 2.0 * M_PI);
                        _graph->SetRange('z', 0.0, 1.0 * M_PI);

                        _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                        _pInfo.ranges[YRANGE].reset(0.0, 2.0 * M_PI);
                        _pInfo.ranges[ZRANGE].reset(0.0, M_PI);
                        break;
                    }
                    case SPHERICAL_RT:
                    {
                        _graph->SetOrigin(_pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE),
                                          0.5 / _pData.getAxisScale(YRANGE),
                                          0.0);
                        _graph->SetFunc(CoordFunc("x*cos(pi*z*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(ZRANGE), _pData.getAxisScale(YRANGE)).c_str(),
                                        CoordFunc("x*sin(pi*z*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(ZRANGE), _pData.getAxisScale(YRANGE)).c_str(),
                                        CoordFunc("x*cos(pi*y*$TS$)", 1.0, _pData.getAxisScale(YRANGE)).c_str());

                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max() / _pData.getAxisScale(XRANGE));
                        _graph->SetRange('y', 0.0, APPR_ONE / _pData.getAxisScale(YRANGE));
                        _graph->SetRange('z', 0.0, APPR_TWO / _pData.getAxisScale(ZRANGE));

                        applyGrid();

                        _graph->SetFunc("x*cos(z)*sin(y)", "x*sin(z)*sin(y)", "x*cos(y)");
                        _graph->SetOrigin(_pInfo.ranges[XRANGE].max(), 0.5 * M_PI, 0.0);
                        _graph->SetRange('x', 0.0, _pInfo.ranges[XRANGE].max());
                        _graph->SetRange('y', 0.0, 1.0 * M_PI);
                        _graph->SetRange('z', 0.0, 2.0 * M_PI);

                        _pInfo.ranges[XRANGE].reset(0.0, _pInfo.ranges[XRANGE].max());
                        _pInfo.ranges[YRANGE].reset(0.0, M_PI);
                        _pInfo.ranges[ZRANGE].reset(0.0, 2.0*M_PI);
                        break;
                    }
                    default:
                    {
                        if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                        {
                            // Change this settings only, if a density plot is requested
                            if (_pInfo.sCommand == "dens" || _pInfo.sCommand == "density")
                            {
                                _graph->SetOrigin(_pInfo.ranges[XRANGE].min(),
                                                  _pInfo.ranges[YRANGE].min(),
                                                  _pInfo.ranges[ZRANGE].max());
                            }

                            _graph->Axis();
                        }
                    }
                }

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC)
                    || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                    || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                    || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                {
                    _graph->Label('x', fromSystemCodePage(_pData.getAxisLabel(XCOORD)).c_str(), getLabelPosition(XCOORD));
                    _graph->Label('y', fromSystemCodePage(_pData.getAxisLabel(YCOORD)).c_str(), getLabelPosition(YCOORD));
                    _graph->Label('z', fromSystemCodePage(_pData.getAxisLabel(ZCOORD)).c_str(), getLabelPosition(ZCOORD));
                }
            }
        }
        else if (isnan(_pData.getOrigin(XCOORD)) && isnan(_pData.getOrigin(YCOORD)) && isnan(_pData.getOrigin(ZCOORD)))
        {
            if (_pInfo.ranges[XRANGE].isInside(0.0)
                && _pInfo.ranges[YRANGE].isInside(0.0)
                && _pInfo.ranges[ZRANGE].isInside(0.0)
                && _pInfo.nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (_pInfo.b2D && sCommand != "dens"))
               )
            {
                _graph->SetOrigin(0.0, 0.0, 0.0);

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("AKDTVISO");
                else
                {
                    _graph->SetTickLen(1e-20);
                    _graph->SetTicks('x', -5, 1);
                    _graph->SetTicks('y', -5, 1);
                    _graph->SetTicks('z', -5, 1);
                    _graph->Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.ranges[XRANGE].isInside(0.0)
                     && _pInfo.ranges[YRANGE].isInside(0.0)
                     && _pInfo.nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(_pInfo.b2D && sCommand != "dens"))
                    )
            {
                _graph->SetOrigin(0.0,
                                  0.0,
                                  _pInfo.sCommand == "dens" || _pInfo.sCommand == "density" ? _pInfo.ranges[ZRANGE].max() : 0.0);

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("AKDTVISO");
                else
                {
                    _graph->SetTickLen(1e-20);
                    _graph->SetTicks('x', -5, 1);
                    _graph->SetTicks('y', -5, 1);
                    _graph->Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.nMaxPlotDim == 3)
            {
                // Set the origin to zero where possible
                _graph->SetOrigin(_pInfo.ranges[XRANGE].isInside(0.0) ? 0.0 : _pInfo.ranges[XRANGE].min(),
                                  _pInfo.ranges[YRANGE].isInside(0.0) ? 0.0 : _pInfo.ranges[YRANGE].min(),
                                  _pInfo.ranges[ZRANGE].isInside(0.0) ? 0.0 : _pInfo.ranges[ZRANGE].min());

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("AKDTVISO");
                else
                {
                    _graph->SetTickLen(1e-20);
                    _graph->SetTicks('x', -5, 1);
                    _graph->SetTicks('y', -5, 1);
                    _graph->SetTicks('z', -5, 1);
                    _graph->Axis("AKDTVISO_");
                }

            }
            else if (_pInfo.nMaxPlotDim <= 2)
            {
                bool isDens = _pInfo.sCommand == "dens" || _pInfo.sCommand == "density";

                // Set the origin to zero where possible
                _graph->SetOrigin(_pInfo.ranges[XRANGE].isInside(0.0) ? 0.0 : _pInfo.ranges[XRANGE].min(),
                                  _pInfo.ranges[YRANGE].isInside(0.0) ? 0.0 : _pInfo.ranges[YRANGE].min(),
                                  isDens ? _pInfo.ranges[ZRANGE].max() : NAN);

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("AKDTVISO");
                else
                {
                    _graph->SetTickLen(1e-20);
                    _graph->SetTicks('x', -5, 1);
                    _graph->SetTicks('y', -5, 1);
                    _graph->Axis("AKDTVISO_");
                }
            }
        }
        else if (_pData.getOrigin(XCOORD) != 0.0
                 || _pData.getOrigin(YCOORD) != 0.0
                 || _pData.getOrigin(ZCOORD) != 0.0)
        {
            if (_pInfo.ranges[XRANGE].isInside(_pData.getOrigin(XCOORD))
                && _pInfo.ranges[YRANGE].isInside(_pData.getOrigin(YCOORD))
                && _pInfo.ranges[ZRANGE].isInside(_pData.getOrigin(ZCOORD))
                && _pInfo.nMaxPlotDim > 2)
            {
                _graph->SetOrigin(_pData.getOrigin(XCOORD),
                                  _pData.getOrigin(YCOORD),
                                  _pData.getOrigin(ZCOORD));

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("AKDTVISO");
                else
                {
                    _graph->SetTickLen(1e-20);
                    _graph->SetTicks('x', -5, 1);
                    _graph->SetTicks('y', -5, 1);
                    _graph->SetTicks('z', -5, 1);
                    _graph->Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.ranges[XRANGE].isInside(_pData.getOrigin(XCOORD))
                     && _pInfo.ranges[YRANGE].isInside(_pData.getOrigin(YCOORD))
                     && _pInfo.nMaxPlotDim <= 2)
            {
                _graph->SetOrigin(_pData.getOrigin(XCOORD),
                                  _pData.getOrigin(YCOORD));

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("AKDTVISO");
                else
                {
                    _graph->SetTickLen(1e-20);
                    _graph->SetTicks('x', -5, 1);
                    _graph->SetTicks('y', -5, 1);
                    _graph->Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.nMaxPlotDim == 3)
            {
                double or_x = _pData.getOrigin(XCOORD);
                double or_y = _pData.getOrigin(YCOORD);
                double or_z = _pData.getOrigin(ZCOORD);

                // Set the origin to the desired point where possible
                _graph->SetOrigin(_pInfo.ranges[XRANGE].isInside(or_x) ? or_x : _pInfo.ranges[XRANGE].min(),
                                  _pInfo.ranges[YRANGE].isInside(or_y) ? or_y : _pInfo.ranges[YRANGE].min(),
                                  _pInfo.ranges[ZRANGE].isInside(or_z) ? or_z : _pInfo.ranges[ZRANGE].min());

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("AKDTVISO");
                else
                {
                    _graph->SetTickLen(1e-20);
                    _graph->SetTicks('x', -5, 1);
                    _graph->SetTicks('y', -5, 1);
                    _graph->SetTicks('z', -5, 1);
                    _graph->Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.nMaxPlotDim <= 2)
            {
                double or_x = _pData.getOrigin(XCOORD);
                double or_y = _pData.getOrigin(YCOORD);

                // Set the origin to the desired point where possible
                _graph->SetOrigin(_pInfo.ranges[XRANGE].isInside(or_x) ? or_x : _pInfo.ranges[XRANGE].min(),
                                  _pInfo.ranges[YRANGE].isInside(or_y) ? or_y : _pInfo.ranges[YRANGE].min());

                if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                    _graph->Axis("AKDTVISO");
                else
                {
                    _graph->SetTickLen(1e-20);
                    _graph->SetTicks('x', -5, 1);
                    _graph->SetTicks('y', -5, 1);
                    _graph->Axis("AKDTVISO_");
                }
            }
        }
        else if (_pInfo.ranges[XRANGE].isInside(0.0)
                 && _pInfo.ranges[YRANGE].isInside(0.0)
                 && _pInfo.ranges[ZRANGE].isInside(0.0)
                 && _pInfo.nMaxPlotDim > 2)
        {
            _graph->SetOrigin(0.0, 0.0, 0.0);

            if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                _graph->Axis("AKDTVISO");
            else
            {
                _graph->SetTickLen(1e-20);
                _graph->SetTicks('x', -5, 1);
                _graph->SetTicks('y', -5, 1);
                _graph->SetTicks('z', -5, 1);
                _graph->Axis("AKDTVISO_");
            }
        }
        else if (_pInfo.ranges[XRANGE].isInside(0.0)
                 && _pInfo.ranges[YRANGE].isInside(0.0)
                 && _pInfo.nMaxPlotDim <= 2)
        {
            _graph->SetOrigin(0.0,
                              0.0,
                              _pInfo.sCommand == "dens" || _pInfo.sCommand == "density" ? _pInfo.ranges[ZRANGE].max() : 0.0);

            if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                _graph->Axis("AKDTVISO");
            else
            {
                _graph->SetTickLen(1e-20);
                _graph->SetTicks('x', -5, 1);
                _graph->SetTicks('y', -5, 1);
                _graph->Axis("AKDTVISO_");
            }
        }
        else if (_pInfo.nMaxPlotDim > 2)
        {
            _graph->SetOrigin(_pInfo.ranges[XRANGE].min(),
                              _pInfo.ranges[YRANGE].min(),
                              _pInfo.ranges[ZRANGE].min());

            if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                _graph->Axis("AKDTVISO");
            else
            {
                _graph->SetTickLen(1e-20);
                _graph->SetTicks('x', -5, 1);
                _graph->SetTicks('y', -5, 1);
                _graph->SetTicks('z', -5, 1);
                _graph->Axis("AKDTVISO_");
            }
        }
        else
        {
            _graph->SetOrigin(_pInfo.ranges[XRANGE].min(),
                              _pInfo.ranges[YRANGE].min(),
                              _pInfo.sCommand == "dens" || _pInfo.sCommand == "density" ? _pInfo.ranges[ZRANGE].max() : _pInfo.ranges[ZRANGE].min());

            if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                _graph->Axis("AKDTVISO");
            else
            {
                _graph->SetTickLen(1e-20);
                _graph->SetTicks('x', -5, 1);
                _graph->SetTicks('y', -5, 1);
                _graph->SetTicks('z', -5, 1);
                _graph->Axis("AKDTVISO_");
            }
        }
    }

    if (_pData.getSettings(PlotData::INT_COORDS) == CARTESIAN)
    {
        if (_pData.getSettings(PlotData::INT_GRID) && !_pInfo.b2DVect && !_pInfo.b3DVect) // Standard-Grid
        {
            if (_pData.getSettings(PlotData::INT_GRID) == 1)
                _graph->Grid("xyzt", _pData.getGridStyle().c_str());
            else if (_pData.getSettings(PlotData::INT_GRID) == 2)
            {
                _graph->Grid("xyzt!", _pData.getGridStyle().c_str());
                _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
            }
        }
        else if (_pData.getSettings(PlotData::INT_GRID)) // Vektor-Grid
            _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());

        if (_pData.getSettings(PlotData::LOG_BOX) || hasSecAxisBox(_pData, _pInfo))
        {
            if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
                _graph->Box();
            else
                _graph->Box("k", false);
        }

        // --> Achsen beschriften <--
        if (_pData.getSettings(PlotData::INT_AXIS) != AXIS_NONE
            && (!_pData.getSettings(PlotData::LOG_SCHEMATIC)
                || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                || findParameter(_pInfo.sPlotParams, "zlabel", '=')))
        {
            _graph->Label('x', fromSystemCodePage(_pData.getAxisLabel(XCOORD)).c_str(), getLabelPosition(XCOORD));
            _graph->Label('y', fromSystemCodePage(_pData.getAxisLabel(YCOORD)).c_str(), getLabelPosition(YCOORD));
            _graph->Label('z', fromSystemCodePage(_pData.getAxisLabel(ZCOORD)).c_str(), getLabelPosition(ZCOORD));
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function determines the
/// position of the axis label, which depends on
/// whether a box surrounds the plot and the
/// selected coordinate system.
///
/// \param nCoord int
/// \return double
///
/////////////////////////////////////////////////
double Plot::getLabelPosition(int nCoord)
{
    if (_pData.getSettings(PlotData::INT_COORDS) == CARTESIAN)
    {
        if (_pData.getSettings(PlotData::LOG_BOX) || hasSecAxisBox(_pData, _pInfo))
            return 0.0;
        else
            return 1.1;
    }
    else
    {
        const int RCOORD = XCOORD;
        const int PHICOORD = YCOORD;
        const int THETACOORD = ZCOORD;
        int nCoordMap[] = {RCOORD, PHICOORD, ZCOORD};

        double dCoordPos[] = {-0.5, (_pData.getRotateAngle(1) - 225.0) / 180.0, 0.0};

        if (_pData.getSettings(PlotData::INT_COORDS) >= SPHERICAL_PT)
        {
            dCoordPos[RCOORD] = -0.4;
            dCoordPos[THETACOORD] = -0.9;
        }

        switch (_pData.getSettings(PlotData::INT_COORDS))
        {
            case POLAR_PZ:
                nCoordMap[XCOORD] = PHICOORD;
                nCoordMap[YCOORD] = ZCOORD;
                nCoordMap[ZCOORD] = RCOORD;
                break;
            case POLAR_RP:
                nCoordMap[XCOORD] = RCOORD;
                nCoordMap[YCOORD] = PHICOORD;
                nCoordMap[ZCOORD] = ZCOORD;
                break;
            case POLAR_RZ:
                nCoordMap[XCOORD] = RCOORD;
                nCoordMap[YCOORD] = ZCOORD;
                nCoordMap[ZCOORD] = PHICOORD;
                break;
            case SPHERICAL_PT:
                nCoordMap[XCOORD] = PHICOORD;
                nCoordMap[YCOORD] = THETACOORD;
                nCoordMap[ZCOORD] = RCOORD;
                break;
            case SPHERICAL_RP:
                nCoordMap[XCOORD] = RCOORD;
                nCoordMap[YCOORD] = PHICOORD;
                nCoordMap[ZCOORD] = THETACOORD;
                break;
            case SPHERICAL_RT:
                nCoordMap[XCOORD] = RCOORD;
                nCoordMap[YCOORD] = THETACOORD;
                nCoordMap[ZCOORD] = PHICOORD;
                break;
            default:
                return 0.0;
        }

        return dCoordPos[nCoordMap[nCoord]];
    }
}


/////////////////////////////////////////////////
/// \brief This member function applies the grid
/// with the selected styles to the plot.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::applyGrid()
{
    if (!_pData.getSettings(PlotData::LOG_SCHEMATIC))
        _graph->Axis(); //U
    else
    {
        _graph->SetTickLen(1e-20);
        _graph->Axis("_");
    }

    if (_pData.getSettings(PlotData::LOG_BOX)
        || hasSecAxisBox(_pData, _pInfo)
        || _pData.getSettings(PlotData::INT_COORDS) != CARTESIAN)
        _graph->Box();

    if (_pData.getSettings(PlotData::INT_GRID) == 1)
        _graph->Grid("xyzt", _pData.getGridStyle().c_str());
    else if (_pData.getSettings(PlotData::INT_GRID) == 2)
    {
        _graph->Grid("xyzt!", _pData.getGridStyle().c_str());
        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
    }
}


/////////////////////////////////////////////////
/// \brief This member function is used to
/// create a coordinate function for the
/// coordinate system, which considers the
/// desired axis scale.
///
/// \param sFuncDef const std::string&
/// \param dPhiScale double
/// \param dThetaScale double
/// \return string
///
/////////////////////////////////////////////////
string Plot::CoordFunc(const std::string& sFuncDef, double dPhiScale, double dThetaScale)
{
    string sParsedFunction = sFuncDef;

    if (dPhiScale == 1.0)
    {
        while (sParsedFunction.find("$PS$") != string::npos)
            sParsedFunction.replace(sParsedFunction.find("$PS$"), 4, "1");
    }
    else
    {
        while (sParsedFunction.find("$PS$") != string::npos)
            sParsedFunction.replace(sParsedFunction.find("$PS$"), 4, "(" + toString(dPhiScale, 5) + ")");
    }

    if (dThetaScale == 1.0)
    {
        while (sParsedFunction.find("$TS$") != string::npos)
            sParsedFunction.replace(sParsedFunction.find("$TS$"), 4, "1");
    }
    else
    {
        while (sParsedFunction.find("$TS$") != string::npos)
            sParsedFunction.replace(sParsedFunction.find("$TS$"), 4, "(" + toString(dThetaScale, 5) + ")");
    }

    return sParsedFunction;
}


/////////////////////////////////////////////////
/// \brief This member function creates a special
/// colour string, which is used for bar charts,
/// which might contain more than one data set.
///
/// \param nNum long int
/// \return string
///
/////////////////////////////////////////////////
string Plot::composeColoursForBarChart(long int nNum)
{
    string sColours;

    if (_pData.getSettings(PlotData::LOG_STACKEDBARS))
        sColours = "a";

    for (int i = 0; i < nNum; i++)
    {
        sColours += _pData.getColors()[_pInfo.nStyle];

        if (i + 1 < nNum)
            _pInfo.nStyle = _pInfo.nextStyle();
    }

    return sColours;
}


/////////////////////////////////////////////////
/// \brief This member function checks, whether
/// the selected subplot position is still empty.
///
/// \param nMultiPlot[2] size_t
/// \param nSubPlotMap size_t&
/// \param nPlotPos size_t
/// \param nCols size_t
/// \param nLines size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Plot::checkMultiPlotArray(size_t nMultiPlot[2], size_t& nSubPlotMap, size_t nPlotPos, size_t nCols, size_t nLines)
{
    // cols, lines
    if (nPlotPos + nCols - 1 >= (nPlotPos / nMultiPlot[0] + 1)*nMultiPlot[0])
        return false;

    if (nPlotPos / nMultiPlot[0] + nLines - 1 >= nMultiPlot[1])
        return false;

    size_t nCol0, nLine0, pos;
    nCol0 = nPlotPos % nMultiPlot[0];
    nLine0 = nPlotPos / nMultiPlot[0];

    for (size_t i = 0; i < nLines; i++)
    {
        for (size_t j = 0; j < nCols; j++)
        {
            pos = 1;
            pos <<= ((nCol0 + j) + nMultiPlot[0] * (nLine0 + i));

            if (pos & nSubPlotMap)
                return false;
        }
    }

    for (size_t i = 0; i < nLines; i++)
    {
        for (size_t j = 0; j < nCols; j++)
        {
            pos = 1;
            pos <<= ((nCol0 + j) + nMultiPlot[0] * (nLine0 + i));
            nSubPlotMap |= pos;
        }
    }

    return true;
}



