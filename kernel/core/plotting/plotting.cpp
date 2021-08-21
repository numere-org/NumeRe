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
#include "../maths/parser_functions.hpp"
#include "../../kernel.hpp"


extern DefaultVariables _defVars;
extern mglGraph _fontData;


// These definitions are for easier understanding of the different ranges
#define XCOORD 0
#define YCOORD 1
#define ZCOORD 2
#define TCOORD 3
#define APPR_ONE 0.9999999
#define APPR_TWO 1.9999999
#define STYLES_COUNT 20

/////////////////////////////////////////////////
/// \brief Wrapper function for creating plots.
/// Will create an instance of the Plot class,
/// which will handle the plotting process.
///
/// \param sCmd string&
/// \param _data Datafile&
/// \param _parser Parser&
/// \param _option Settings&
/// \param _functions Define&
/// \param _pData PlotData&
/// \return void
///
/////////////////////////////////////////////////
void createPlot(string& sCmd, MemoryManager& _data, Parser& _parser, Settings& _option, FunctionDefinitionManager& _functions, PlotData& _pData)
{
    Plot graph(sCmd, _data, _parser, _option, _functions, _pData);

    // Only open graph viewer, if not explicitly deactivated
    if (_pData.getOpenImage() && !_pData.getSilentMode())
    {
        GraphHelper* _graphHelper = graph.createGraphHelper();

        if (_pData.getTargetGUI()[0] != -1)
        {
            NumeRe::WindowInformation window = NumeReKernel::getInstance()->getWindowManager().getWindowInformation(_pData.getTargetGUI()[0]);

            if (!window.window && window.nStatus != NumeRe::STATUS_RUNNING)
            {
                delete _graphHelper;
                _pData.deleteData(true);
                throw SyntaxError(SyntaxError::INVALID_WINDOW_ID, sCmd, "streamto");
            }

            if (!window.window->setItemGraph(_graphHelper, _pData.getTargetGUI()[1]))
            {
                delete _graphHelper;
                _pData.deleteData(true);
                throw SyntaxError(SyntaxError::INVALID_WINDOW_ITEM_ID, sCmd, "streamto");
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
static mglData duplicatePoints(const mglData& _mData)
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
/// \brief This static function is a fix for the
/// MathGL bug to connect points, which are out
/// of data range. This fix is used in curved
/// coordinates case, where the calculated
/// coordinate is r or rho.
///
/// \param _mData mglData&
/// \return void
///
/////////////////////////////////////////////////
static void removeNegativeValues(mglData& _mData)
{
    for (int i = 0; i < _mData.GetNN(); i++)
        _mData.a[i] = _mData.a[i] >= 0.0 ? _mData.a[i] : NAN;
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
    sFunc = "";                      // string mit allen Funktionen
    sLabels = "";                    // string mit den Namen aller Funktionen (Fuer die Legende)
    sDataLabels = "";                // string mit den Datenpunkt-Namen (Fuer die Legende)

    mglData _mBackground;                   // mglData-Objekt fuer ein evtl. Hintergrundbild
    _graph = new mglGraph();
    bOutputDesired = false;             // if a output directly into a file is desired

    _pInfo.sCommand = "";
    _pInfo.sPlotParams = "";
    _pInfo.dColorRanges[0] = 0.0;
    _pInfo.dColorRanges[1] = 1.0;
    _pInfo.b2D = false;
    _pInfo.b3D = false;
    _pInfo.b2DVect = false;
    _pInfo.b3DVect = false;
    _pInfo.bDraw = false;
    _pInfo.bDraw3D = false;
    _pInfo.nMaxPlotDim = 1;
    _pInfo.nStyleMax = STYLES_COUNT;                  // Gesamtzahl der Styles

    bool bAnimateVar = false;
    string sOutputName = "";

    vector<string> vPlotCompose;
    unsigned int nMultiplots[2] = {0, 0};

    unsigned int nSubPlots = 0;
    unsigned int nSubPlotMap = 0;


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
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "multiplot", '=') + 9));
            int nRes = 0;
            value_type* v = _parser.Eval(nRes);

            // Only use the option value, if it contains two values
            if (nRes == 2)
            {
                nMultiplots[1] = (unsigned int)intCast(v[0]);
                nMultiplots[0] = (unsigned int)intCast(v[1]);
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
        for (unsigned int i = 0; i < vPlotCompose.size(); i++)
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
            evaluatePlotParamString();
            if (nMultiplots[0]) // if this is a multiplot layout, then we will only evaluate the SUPERGLOBAL parameters
                _pData.setParams(_pInfo.sPlotParams, _parser, _option, PlotData::SUPERGLOBAL);
            else
                _pData.setGlobalComposeParams(_pInfo.sPlotParams, _parser, _option);
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
    _pInfo.sLineStyles = new string[_pInfo.nStyleMax];
    _pInfo.sContStyles = new string[_pInfo.nStyleMax];
    _pInfo.sPointStyles = new string[_pInfo.nStyleMax];
    _pInfo.sConPointStyles = new string[_pInfo.nStyleMax];

    // The following statement moves the output cursor to the first postion and
    // cleans the line to avoid overwriting
    if (!_pData.getSilentMode() && _option.systemPrints())
        NumeReKernel::printPreFmt("\r");

    size_t nPlotStart = 0;

    // Main loop of the plotting object. This will execute all plotting
    // commands in the current comamnd set clustered into subplots event
    // if only one plotting command is used
    while (nPlotStart < vPlotCompose.size())
    {
        // If this is a multiplot layout then we need to evaluate the global options for every subplot,
        // because we omitted this set further up. We enter this for each "subplot" command.
        if (vPlotCompose.size() > 1
                && nMultiplots[0])
        {
            // Reset the maximal plotting dimension
            _pInfo.nMaxPlotDim = 0;

            // Gather each plotting parameter until the next "subplot" command or until the end of the
            // whole block
            for (unsigned int i = nPlotStart; i < vPlotCompose.size(); i++)
            {
                if (vPlotCompose[i].find("-set") != string::npos
                        && !isInQuotes(vPlotCompose[i], vPlotCompose[i].find("-set"))
                        && findCommand(vPlotCompose[i]).sString != "subplot")
                    _pInfo.sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("-set"));
                else if (vPlotCompose[i].find("--") != string::npos
                         && !isInQuotes(vPlotCompose[i], vPlotCompose[i].find("--"))
                         && findCommand(vPlotCompose[i]).sString != "subplot")
                    _pInfo.sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("--"));

                // Leave the loop, if the current command equals "subplot"
                if (findCommand(vPlotCompose[i]).sString == "subplot")
                    break;

                // Find the maximal plotting dimension of the current subplot
                determinePlottingDimensions(findCommand(vPlotCompose[i]).sString);

                // Append a whitespace for safety reasons
                _pInfo.sPlotParams += " ";
            }

            // Apply the global parameters for this single subplot
            if (_pInfo.sPlotParams.length())
            {
                evaluatePlotParamString();
                _pData.setParams(_pInfo.sPlotParams, _parser, _option, PlotData::GLOBAL);
                _pInfo.sPlotParams.clear();
            }
        }

        // Create the actual subplot of this plot composition (if only
        // one plotting command is used, the return value will be equal to
        // the size of the plot composition vectore, which is 1)
        nPlotStart = createSubPlotSet(sOutputName, bAnimateVar, vPlotCompose, nPlotStart, nMultiplots, nSubPlots, nSubPlotMap);

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
    if (_pData.getSilentMode() || !_pData.getOpenImage() || bOutputDesired)
    {
        // --> Speichern und Erfolgsmeldung <--
        if (!_pData.getAnimateSamples() || !bAnimateVar)
        {
            if (!_pData.getSilentMode() && _option.systemPrints())
                NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("PLOT_SAVING")) + " ... ");

            if (sOutputName.substr(sOutputName.length()-4) == ".bps")
            {
                sOutputName[sOutputName.length()-3] = 'e';
                _graph->WriteBPS(sOutputName.c_str());
            }
            else if (sOutputName.substr(sOutputName.length()-4) == ".tif" || sOutputName.substr(sOutputName.length()-5) == ".tiff")
                writeTiff(_graph, sOutputName);
            else
                _graph->WriteFrame(sOutputName.c_str());

            // --> TeX-Ausgabe gewaehlt? Dann werden mehrere Dateien erzeugt, die an den Zielort verschoben werden muessen <--
            if (sOutputName.substr(sOutputName.length() - 4, 4) == ".tex")
                writeTeXMain(sOutputName);

            if (!_pData.getSilentMode() && _option.systemPrints())
                NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
        }

        if (!_pData.getSilentMode() && _option.systemPrints())
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
/// \brief This member function determines the
/// maximal plotting dimension of the passed
/// command.
///
/// \param sPlotCommand const string&
/// \return void
///
/////////////////////////////////////////////////
void Plot::determinePlottingDimensions(const string& sPlotCommand)
{
    if ((sPlotCommand.substr(0, 4) == "mesh"
            || sPlotCommand.substr(0, 4) == "surf"
            || sPlotCommand.substr(0, 4) == "cont"
            || sPlotCommand.substr(0, 4) == "vect"
            || sPlotCommand.substr(0, 4) == "dens"
            || sPlotCommand.substr(0, 4) == "draw"
            || sPlotCommand.substr(0, 4) == "grad"
            || sPlotCommand.substr(0, 4) == "plot")
            && sPlotCommand.find("3d") != string::npos)
    {
        _pInfo.nMaxPlotDim = 3;
    }
    else if (sPlotCommand.substr(0, 4) == "mesh" || sPlotCommand.substr(0, 4) == "surf" || sPlotCommand.substr(0, 4) == "cont")
    {
        _pInfo.nMaxPlotDim = 3;
    }
    else if (sPlotCommand.substr(0, 4) == "vect" || sPlotCommand.substr(0, 4) == "dens" || sPlotCommand.substr(0, 4) == "grad" || sPlotCommand == "implot")
    {
        if (_pInfo.nMaxPlotDim < 3)
            _pInfo.nMaxPlotDim = 2;
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// creation of a single subplot, which may be a
/// single plot or part of a plot composition.
///
/// \param sOutputName string&
/// \param bAnimateVar bool&
/// \param vPlotCompose vector<string>&
/// \param nSubPlotStart size_t
/// \param nMultiplots[2] size_t
/// \param nSubPlots size_t&
/// \param nSubPlotMap size_t&
/// \return size_t
///
/////////////////////////////////////////////////
size_t Plot::createSubPlotSet(string& sOutputName, bool& bAnimateVar, vector<string>& vPlotCompose, size_t nSubPlotStart, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap)
{
    vector<short> vType;
    vector<string> vDrawVector;
    string sCmd;
    string sAxisBinds;
    string sDataAxisBinds;
    bool bNewSubPlot = nSubPlotStart != 0;
    const short TYPE_FUNC = 1;

    double dDataRanges[3][2];
    double dSecDataRanges[2][2] = {{NAN, NAN}, {NAN, NAN}};

    size_t nLegends = 0;

    int nFunctions = 0;
    _pInfo.nFunctions = &nFunctions;

    int nStyle = 0;
    _pInfo.nStyle = &nStyle;

    // This loop will iterate through the plotting commands from
    // the passed starting index until the end or until the command
    // "subplot" has been found
    for (unsigned int nPlotCompose = nSubPlotStart; nPlotCompose < vPlotCompose.size(); nPlotCompose++)
    {
        vType.clear();
        sCmd = vPlotCompose[nPlotCompose];
        _pInfo.sPlotParams = "";

        // Clean the memory, if this is not the first
        // run through this loop
        if (nPlotCompose)
        {
            // Clear allocated memory
            if (v_mDataPlots.size())
            {
                v_mDataPlots.clear();
                sDataLabels = "";
            }

            // Only delete the contents of the plot data object
            // if the last command was no "subplot" command, otherwise
            // one would reset all global plotting parameters
            if (!bNewSubPlot)
                _pData.deleteData();

            // Reset the plot info object
            _pInfo.b2D = false;
            _pInfo.b3D = false;
            _pInfo.b2DVect = false;
            _pInfo.b3DVect = false;
            _pInfo.bDraw3D = false;
            _pInfo.bDraw = false;
            vDrawVector.clear();
            sLabels = "";
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
            evaluatePlotParamString();

            // Apply the parameters locally (for a plot composition) or globally, if
            // this is a single plot command
            if (vPlotCompose.size() > 1)
                _pData.setLocalComposeParams(_pInfo.sPlotParams, _parser, _option);
            else
                _pData.setParams(_pInfo.sPlotParams, _parser, _option);
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
            throw SyntaxError(SyntaxError::PLOT_ERROR, sCmd, SyntaxError::invalid_position);

        setStyles();

        if (_pData.getBars() || _pData.getHBars())
            _graph->SetBarWidth(_pData.getBars() ? _pData.getBars() : _pData.getHBars());

        // Set the number of samples depending on the
        // current plotting command
        if (!_pData.getAnimateSamples()
                || (_pData.getAnimateSamples()
                    && _pInfo.sCommand.substr(0, 4) != "mesh"
                    && _pInfo.sCommand.substr(0, 4) != "surf"
                    && _pInfo.sCommand.substr(0, 4) != "grad"
                    && _pInfo.sCommand.substr(0, 4) != "cont"))
            _pInfo.nSamples = _pData.getSamples();
        else if (_pData.getSamples() > 1000
                 && (_pInfo.sCommand.substr(0, 4) == "mesh"
                     || _pInfo.sCommand.substr(0, 4) == "surf"
                     || _pInfo.sCommand.substr(0, 4) == "grad"
                     || _pInfo.sCommand.substr(0, 4) == "cont"))
            _pInfo.nSamples = 1000;
        else
            _pInfo.nSamples = _pData.getSamples();

        if (_pInfo.nSamples > 151 && _pInfo.b3D)
            _pInfo.nSamples = 151;


        // Create a file name depending on the selected current plot
        // command, if the user did not provide a custom one
        filename(vPlotCompose.size(), nPlotCompose);

        // Get the target output file name and remove the surrounding
        // quotation marks
        if (!nPlotCompose)
        {
            sOutputName = _pData.getFileName();
            StripSpaces(sOutputName);
            if (sOutputName[0] == '"' && sOutputName[sOutputName.length() - 1] == '"')
                sOutputName = sOutputName.substr(1, sOutputName.length() - 2);
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
            evaluateSubplot(nLegends, sCmd, nMultiplots, nSubPlots, nSubPlotMap);
            nSubPlots++;

            // Return the position of the next plotting command
            return nPlotCompose+1;
        }
        else if (findCommand(sCmd).sString == "subplot")
            continue; // Ignore the "subplot" command, if we have no multiplot layout

        // Display the "Calculating data for SOME PLOT" message
        displayMessage(_pData.getAnimateSamples() && findVariableInExpression(sFunc, "t") != string::npos);

        // Apply the logic and the transformation for logarithmic
        // plotting axes
        if (_pData.getCoords() == PlotData::CARTESIAN
                && (nPlotCompose == nSubPlotStart)
                && (_pData.getxLogscale() || _pData.getyLogscale() || _pData.getzLogscale() || _pData.getcLogscale()))
        {
            setLogScale((_pInfo.b2D || _pInfo.sCommand == "plot3d"));
        }

        // Prepare the legend strings
        if (!_pInfo.bDraw3D && !_pInfo.bDraw)
        {
            // Obtain the values of string variables, which are probably used
            // as part of the legend strings
            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sFunc))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sFunc);

            // Add the legends to the function-and-data section
            if (!addLegends(sFunc))
                return vPlotCompose.size(); // --> Bei Fehlern: Zurueck zur aufrufenden Funktion <--
        }

        // Replace the custom defined functions with their definition
        if (!_functions.call(sFunc))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

        // Call the input prompt, if one of the function definition requires this
        if (sFunc.find("??") != string::npos)
            sFunc = promptForUserInput(sFunc);

        // Split the function-and-data section into functions and data sets,
        // evaluate the indices of the data sets and store the values of the
        // data sets into the mglData objects, which will be used further down
        // to display the data values in the plots
        std::vector<std::string> vDataPlots = evaluateDataPlots(vType, sAxisBinds, sDataAxisBinds);

        // Do we need to search for the animation parameter variable?
        if (_pData.getAnimateSamples())
        {
            for (std::string sDataPlots : vDataPlots)
            {
                if (findVariableInExpression(sDataPlots, "t") != std::string::npos)
                {
                    bAnimateVar = true;
                    // set t to the starting value
                    _defVars.vValue[TCOORD][0] = _pData.gettBoundary();  // Plotparameter: t
                    break;
                }
            }
        }

        // Get now the data values for the plot
        extractDataValues(vDataPlots, sDataAxisBinds, dDataRanges, dSecDataRanges);

        StripSpaces(sFunc);

        // Ensure that either functions or data plots are available
        if (!sFunc.length() && !v_mDataPlots.size())
            throw SyntaxError(SyntaxError::PLOT_ERROR, sCmd, SyntaxError::invalid_position);

        // Separate the functions from the legend strings, because the functions
        // can be evaluated in parallel and the legend strings will be handled
        // different.
        separateLegends();

        // Legacy vector to multi-expression conversion. Is quite probable not
        // needed any more
        if (sFunc.find("{") != string::npos && !_pInfo.bDraw3D && !_pInfo.bDraw)
            convertVectorToExpression(sFunc, _option);

        // Ensure that the functions do not contain any strings, because strings
        // cannot be plotted
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sFunc) && !(_pInfo.bDraw3D || _pInfo.bDraw))
        {
            clearData();
            throw SyntaxError(SyntaxError::CANNOT_PLOT_STRINGS, sCmd, SyntaxError::invalid_position);
        }

        // Determine, whether the function string is empty. If it is and the current
        // plotting style is not a drawing style, then assign the function string to
        // the parser. Otherwise split the function string into the single functions.
        if (isNotEmptyExpression(sFunc) && !(_pInfo.bDraw3D || _pInfo.bDraw))
        {
            try
            {
                _parser.SetExpr(sFunc);
                _parser.Eval(nFunctions);

                // Search for the animation time variable "t"
                if (findVariableInExpression(sFunc, "t"))
                    bAnimateVar = true;

                // Check, whether the number of functions correspond to special
                // plotting styles
                if ((_pData.getColorMask() || _pData.getAlphaMask()) && _pInfo.b2D && (nFunctions + v_mDataPlots.size()) % 2)
                    throw SyntaxError(SyntaxError::NUMBER_OF_FUNCTIONS_NOT_MATCHING, sCmd, SyntaxError::invalid_position);
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

                vDrawVector.push_back(sArgument);
            }

            // Set the function string to be empty
            sFunc = "<<empty>>";
        }
        else
            sFunc = "<<empty>>";

        // Fill the type vector with the number of functions and
        // their type. Additionally, gather the axis binds and set
        // the combined axis bind settings in the plotdata object
        if (nFunctions && !vType.size())
        {
            vType.assign((unsigned int)nFunctions, TYPE_FUNC);

            for (int i = 0; i < nFunctions; i++)
            {
                sAxisBinds += _pData.getAxisbind(i);
            }
        }

        _pData.setFunctionAxisbind(sAxisBinds);

        // Calculate the default plotting ranges depending on the user
        // specification. If the user didn't specify
        // a set of ranges, we try to use the plotting ranges of the data
        // points and omit negative numbers, if the corresponding axis was
        // set to logarithmic scale.
        defaultRanges(dDataRanges, dSecDataRanges, nPlotCompose, bNewSubPlot);

        // Set the plot calculation variables (x, y and z) to their starting
        // points
        _defVars.vValue[XCOORD][0] = _pInfo.dRanges[XCOORD][0];  // Plotvariable: x
        _defVars.vValue[YCOORD][0] = _pInfo.dRanges[YCOORD][0];  // Plotvariable: y
        _defVars.vValue[ZCOORD][0] = _pInfo.dRanges[ZCOORD][0];  // Plotvariable: z

        // For the "plot3d" style or the animated plot we need the time coordinate.
        // Set it to their corresponding starting value
        if (_pInfo.sCommand == "plot3d" || _pData.getAnimateSamples())
            _defVars.vValue[TCOORD][0] = _pData.gettBoundary();  // Plotparameter: t

        // Prepare the plotting memory for the functions depending
        // on their number
        prepareMemory(nFunctions);

        // Now create the plot or the animation using the provided and
        // pre-calculated data. This function will also open and
        // close the GIF, if the animation directly saved to a file
        createPlotOrAnimation(nStyle, nPlotCompose, vPlotCompose.size(), nLegends, bNewSubPlot, bAnimateVar, vDrawVector, vDataPlots, vType, nFunctions, dDataRanges, dSecDataRanges, sDataAxisBinds, sOutputName);

        bNewSubPlot = false;

        if (!_pData.getSilentMode() && _option.systemPrints())
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
    // Apply the quality and image dimension settings to the overall result
    // image. This affects also a whole multiplot and is therefore done before
    // the first plot is rendered.
    if (_pData.getSilentMode() || !_pData.getOpenImage())
    {
        // Switch between fullHD and the normal resolution
        if (_pData.getHighRes() == 2)
        {
            double dHeight = sqrt(1920.0 * 1440.0 / _pData.getAspect());
            _graph->SetSize((int)lrint(_pData.getAspect()*dHeight), (int)lrint(dHeight));
        }
        else
        {
            double dHeight = sqrt(1280.0 * 960.0 / _pData.getAspect());
            _graph->SetSize((int)lrint(_pData.getAspect()*dHeight), (int)lrint(dHeight));
        }
    }
    else
    {
        // This section is for printing to the GraphViewer window
        // we use different resolutions here
        if (_pData.getAnimateSamples() && !_pData.getFileName().length())
        {
            // Animation size (faster for rendering)
            double dHeight = sqrt(640.0 * 480.0 / _pData.getAspect());
            _graph->SetSize((int)lrint(_pData.getAspect()*dHeight), (int)lrint(dHeight));
        }
        else if (_pData.getHighRes() == 2)
        {
            // Hires output
            double dHeight = sqrt(1280.0 * 960.0 / _pData.getAspect());
            _graph->SetSize((int)lrint(_pData.getAspect()*dHeight), (int)lrint(dHeight));
        }
        else
        {
            // Standard output
            double dHeight = sqrt(800.0 * 600.0 / _pData.getAspect());
            _graph->SetSize((int)lrint(_pData.getAspect()*dHeight), (int)lrint(dHeight));
        }
    }

    // Copy the font and select the font size
    _graph->CopyFont(&_fontData);
    //_graph->SetFontSizePT(8 * ((double)(1 + _pData.getTextSize()) / 6.0), 72);
    _graph->SetFontSizeCM(0.22 * (1.0 + _pData.getTextSize()) / 6.0, 72);
    _graph->SetFlagAdv(1, MGL_FULL_CURV);
    //_graph->SubPlot(1,1,0, "");
    //_graph->SubPlot(1,1,0, "");

}


/////////////////////////////////////////////////
/// \brief This member function creates the plot
/// or animation selected by the plotting command.
///
/// \param nStyle int&
/// \param nPlotCompose size_t
/// \param nPlotComposeSize size_t
/// \param nLegends size_t&
/// \param bNewSubPlot bool
/// \param bAnimateVar bool
/// \param vDrawVector vector<string>&
/// \param vType vector<short>&
/// \param nFunctions int
/// \param dDataRanges[3][2] double
/// \param sDataAxisBinds const string&
/// \param sOutputName const string&
/// \return bool
///
/// It will also create the samples for the
/// animation, which may be target either
/// directly to a file (aka GIF) or to the
/// GraphViewer, which will finalize the
/// rendering step.
/////////////////////////////////////////////////
bool Plot::createPlotOrAnimation(int& nStyle, size_t nPlotCompose, size_t nPlotComposeSize, size_t& nLegends, bool bNewSubPlot, bool bAnimateVar, vector<string>& vDrawVector, const vector<string>& vDataPlots, vector<short>& vType, int nFunctions, double dDataRanges[3][2], double dSecDataRanges[2][2], const string& sDataAxisBinds, const string& sOutputName)
{
    mglData _mBackground;
    value_type* vResults = nullptr;

    // If the animation is saved to a file, set the frame time
    // for the GIF image
    if (_pData.getAnimateSamples() && bOutputDesired)
        _graph->StartGIF(sOutputName.c_str(), 40); // 40msec = 2sec bei 50 Frames, d.h. 25 Bilder je Sekunde

    // Load the background image from the target file and apply the
    // black/white color scheme
    if (_pData.getBackground().length() && _pData.getBGColorScheme() != "<<REALISTIC>>")
    {
        if (_pData.getAnimateSamples() && _option.systemPrints())
            NumeReKernel::printPreFmt("|-> ");

        if (_option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_LOADING_BACKGROUND")) + " ... ");

        _mBackground.Import(_pData.getBackground().c_str(), "kw");

        if (_pData.getAnimateSamples() && _option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");
    }

    // Prepare a label cache for the function and data labels,
    // which will be modified during the plot
    string sLabelsCache[2];
    sLabelsCache[0] = sLabels;
    sLabelsCache[1] = sDataLabels;

    // This is the start of the actual plotting algorithm. The previous
    // lines were only preparations for this algorithm. We create either a
    // single plot or a set of frames for an animation
    for (int t_animate = 0; t_animate <= _pData.getAnimateSamples(); t_animate++)
    {
        // If it is an animation, then we're required to reset the plotting
        // variables for each frame. Additionally, we have to start a new
        // frame at this location.
        if (_pData.getAnimateSamples() && !_pData.getSilentMode() && _option.systemPrints() && bAnimateVar)
        {
            NumeReKernel::printPreFmt("\r|-> " + toSystemCodePage(_lang.get("PLOT_RENDERING_FRAME", toString(t_animate + 1), toString(_pData.getAnimateSamples() + 1))) + " ... ");
            nStyle = 0;

            // Prepare a new frame
            _graph->NewFrame();

            // Reset the plotting variables (x, y and z) to their initial values
            // and increment the time variable one step further
            if (t_animate)
            {
                _defVars.vValue[TCOORD][0] += (_pData.gettBoundary(1) - _pData.gettBoundary()) / (double)_pData.getAnimateSamples();
                sLabels = sLabelsCache[0];
                sDataLabels = sLabelsCache[1];

                // Recalculate the data plots
                extractDataValues(vDataPlots, sDataAxisBinds, dDataRanges, dSecDataRanges);

                defaultRanges(dDataRanges, dSecDataRanges, nPlotCompose, true);

                _defVars.vValue[XCOORD][0] = _pInfo.dRanges[XCOORD][0];
                _defVars.vValue[YCOORD][0] = _pInfo.dRanges[YCOORD][0];
                _defVars.vValue[ZCOORD][0] = _pInfo.dRanges[ZCOORD][0];
            }
        }

        double dt_max = _defVars.vValue[TCOORD][0].real();

        // Apply the title line to the graph
        if (_pData.getTitle().length())
            _graph->Title(fromSystemCodePage(_pData.getTitle()).c_str(), "", -1.5);

        // If the user requested an orthogonal projection, then
        // we activate this plotting mode at this location
        if (_pData.getOrthoProject()
                && (_pInfo.b3D
                    || (_pInfo.b2D && _pInfo.sCommand.substr(0, 4) != "grad" && _pInfo.sCommand.substr(0, 4) != "dens")
                    || _pInfo.b3DVect
                    || _pInfo.sCommand == "plot3d")
           )
        {
            _graph->Ternary(4);
            _graph->SetRotatedText(false);
        }

        // Rotate the graph to the desired plotting angles
        if (_pInfo.nMaxPlotDim > 2 && (!nPlotCompose || bNewSubPlot))
            _graph->Rotate(_pData.getRotateAngle(), _pData.getRotateAngle(1));

        // Calculate the function values for the set plotting ranges
        // and the eventually set time variable
        nFunctions = fillData(vResults, dt_max, t_animate, nFunctions);

        // Normalize the plotting results, if the current plotting
        // style is a vector field
        if (_pInfo.b2DVect)
            _pData.normalize(2, t_animate);
        else if (_pInfo.b3DVect)
            _pData.normalize(3, t_animate);

        // Change the plotting ranges to fit to the calculated plot
        // data (only, if the user did not specify those ranges, too).
        fitPlotRanges(dDataRanges, nPlotCompose, bNewSubPlot);

        // Pass the final ranges to the graph. Only do this, if this is
        // the first plot of a plot composition or a new subplot.
        if (!nPlotCompose || bNewSubPlot)
            passRangesToGraph(dDataRanges);

        // Apply a color bar, if desired and supplied by the plotting style
        applyColorbar();

        // Apply the light effect, if desired and supplied by the plotting stype
        applyLighting();

        // Activate the perspective effect
        if ((!nPlotCompose || bNewSubPlot) && _pInfo.nMaxPlotDim > 2)
            _graph->Perspective(_pData.getPerspective());

        // Render the background image
        if (_pData.getBackground().length())
        {
            if (_pData.getBGColorScheme() != "<<REALISTIC>>")
            {
                _graph->SetRanges(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[XCOORD][1], _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[YCOORD][1], _mBackground.Minimal(), _mBackground.Maximal());
                _graph->Dens(_mBackground, _pData.getBGColorScheme().c_str());
            }
            else
                _graph->Logo(_pData.getBackground().c_str());

            _graph->Rasterize();
            _graph->SetRanges(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[XCOORD][1], _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[YCOORD][1], _pInfo.dRanges[ZCOORD][0], _pInfo.dRanges[ZCOORD][1]);
        }

        // This section will transform the calculated data points into the desired
        // plotting style. The most complex plots are the default plot, the "plot3d"
        // and the 2D-Plots (mesh, surf, etc.)
        if (_pInfo.b2D || _pInfo.sCommand == "implot")        // 2D-Plot
            create2dPlot(vType, nStyle, nLegends, nFunctions, nPlotCompose, nPlotComposeSize);
        else if (_pInfo.sCommand != "plot3d" && !_pInfo.b3D && !_pInfo.b3DVect && !_pInfo.b2DVect && !_pInfo.bDraw3D && !_pInfo.bDraw)      // Standardplot
            createStdPlot(vType, nStyle, nLegends, nFunctions, nPlotCompose, nPlotComposeSize);
        else if (_pInfo.b3D)   // 3D-Plot
            create3dPlot();
        else if (_pInfo.b3DVect)   // 3D-Vektorplot
            create3dVect();
        else if (_pInfo.b2DVect)   // 2D-Vektorplot
            create2dVect();
        else if (_pInfo.bDraw)
            create2dDrawing(vDrawVector, nFunctions);
        else if (_pInfo.bDraw3D)
            create3dDrawing(vDrawVector, nFunctions);
        else            // 3D-Trajektorie
            createStd3dPlot(vType, nStyle, nLegends, nFunctions, nPlotCompose, nPlotComposeSize);

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
/// \param vType vector<short>&
/// \param nStyle int&
/// \param nLegends size_t&
/// \param nFunctions int
/// \param nPlotCompose size_t
/// \param nPlotComposeSize size_t
/// \return void
///
/////////////////////////////////////////////////
void Plot::create2dPlot(vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    const short TYPE_FUNC = 1;
    const short TYPE_DATA = -1;
    mglData _mData(_pInfo.nSamples, _pInfo.nSamples);
    mglData _mMaskData;
    mglData _mPlotAxes[2];
    if (_pData.getColorMask() || _pData.getAlphaMask())
        _mMaskData.Create(_pInfo.nSamples, _pInfo.nSamples);

    mglData _mContVec(_pData.getNumContLines());
    for (size_t nCont = 0; nCont < _pData.getNumContLines(); nCont++)
    {
        _mContVec.a[nCont] = nCont * (_pInfo.dRanges[ZCOORD][1] - _pInfo.dRanges[ZCOORD][0]) / ((double)_pData.getNumContLines()-1) + _pInfo.dRanges[ZCOORD][0];
    }
    if (_pData.getNumContLines() % 2)
        _mContVec.a[_pData.getNumContLines()/2] = (_pInfo.dRanges[ZCOORD][1] - _pInfo.dRanges[ZCOORD][0]) / 2.0 + _pInfo.dRanges[ZCOORD][0];

    int nPos[2] = {0, 0};
    int nTypeCounter[2] = {0, 0};

    for (unsigned int nType = 0; nType < vType.size(); nType++)
    {
        if (vType[nType] == TYPE_FUNC)
        {
            StripSpaces(sLabels);
            _mData.Create(_pInfo.nSamples, _pInfo.nSamples);
            for (long int i = 0; i < _pInfo.nSamples; i++)
            {
                for (long int j = 0; j < _pInfo.nSamples; j++)
                {
                    // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                    _mData.a[i + _pInfo.nSamples * j] = _pData.getData(i, j, nTypeCounter[0]);
                    if ((_pData.getColorMask() || _pData.getAlphaMask()) && (vType.size() <= nType + 1 || (vType.size() > nType + 1 && vType[nType + 1] == TYPE_FUNC)))
                        _mMaskData.a[i + _pInfo.nSamples * j] = _pData.getData(i, j, nTypeCounter[0] + 1);
                }
            }
            if (_pData.getCoords() == PlotData::POLAR_RZ || _pData.getCoords() == PlotData::SPHERICAL_RT)
            {
                _mData = fmod(_mData, 2.0 * M_PI);
                _mMaskData = fmod(_mMaskData, 2.0 * M_PI);
            }
            else if (_pData.getCoords() == PlotData::SPHERICAL_RP)
            {
                _mData = fmod(_mData, M_PI);
                _mMaskData = fmod(_mMaskData, M_PI);
            }
            _mPlotAxes[0] = _mAxisVals[0];
            _mPlotAxes[1] = _mAxisVals[1];
            if ((_pData.getColorMask() || _pData.getAlphaMask()) && vType.size() > nType + 1 && vType[nType + 1] == TYPE_DATA)
            {
                _mMaskData = v_mDataPlots[nTypeCounter[1]][2];
                nTypeCounter[1]++;
            }
            else if ((_pData.getColorMask() || _pData.getAlphaMask()) && (vType.size() > nType + 1 && vType[nType + 1] == TYPE_FUNC))
                nTypeCounter[0]++;
        }
        else
        {
            StripSpaces(sDataLabels);
            if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
                v_mDataPlots[nTypeCounter[1]][0] = fmod(v_mDataPlots[nTypeCounter[1]][0], 2.0 * M_PI);
            if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                v_mDataPlots[nTypeCounter[1]][1] = fmod(v_mDataPlots[nTypeCounter[1]][1], 2.0 * M_PI);
            if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RT)
                v_mDataPlots[nTypeCounter[1]][1] = fmod(v_mDataPlots[nTypeCounter[1]][1], 1.0 * M_PI);

            _mData = v_mDataPlots[nTypeCounter[1]][2];
            _mPlotAxes[0] = v_mDataPlots[nTypeCounter[1]][0];
            _mPlotAxes[1] = v_mDataPlots[nTypeCounter[1]][1];
            if ((_pData.getColorMask() || _pData.getAlphaMask()) && vType.size() > nType + 1 && vType[nType + 1] == TYPE_DATA)
            {
                _mMaskData = v_mDataPlots[nTypeCounter[1] + 1][2];
                nTypeCounter[1]++;
            }
            else if ((_pData.getColorMask() || _pData.getAlphaMask()) && (vType.size() <= nType + 1 || (vType.size() > nType + 1 && vType[nType + 1] == TYPE_FUNC)))
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                {
                    for (long int j = 0; j < _pInfo.nSamples; j++)
                    {
                        // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                        _mData.a[i + _pInfo.nSamples * j] = _pData.getData(i, j, nTypeCounter[0]);
                        if ((_pData.getColorMask() || _pData.getAlphaMask()) && (vType.size() <= nType + 1 || (vType.size() > nType + 1 && vType[nType + 1] == TYPE_FUNC)))
                            _mMaskData.a[i + _pInfo.nSamples * j] = _pData.getData(i, j, nTypeCounter[0] + 1);
                    }
                }
                nTypeCounter[0]++;
            }
        }
        if (!plot2d(_mData, _mMaskData, _mPlotAxes, _mContVec))
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            clearData();
            return;
        }
        if (vType[nType] == TYPE_FUNC)
        {
            if ((nFunctions > 1 && !(_pData.getColorMask() || _pData.getAlphaMask()))
                    || (nFunctions > 2 && (_pData.getColorMask() || _pData.getAlphaMask()))
                    || (nFunctions && v_mDataPlots.size()))
            {
                if (_pData.getContLabels() && _pInfo.sCommand.substr(0, 4) != "cont")
                {
                    _graph->Cont(_mAxisVals[0], _mAxisVals[1], _mData, ("t" + _pInfo.sContStyles[nStyle]).c_str(), ("val " + toString(_pData.getNumContLines())).c_str());
                }
                else if (!_pData.getContLabels() && _pInfo.sCommand.substr(0, 4) != "cont")
                    _graph->Cont(_mAxisVals[0], _mAxisVals[1], _mData, _pInfo.sContStyles[nStyle].c_str(), ("val " + toString(_pData.getNumContLines())).c_str());
                nPos[0] = sLabels.find(';');
                sConvLegends = sLabels.substr(0, nPos[0]);
                // --> Der String-Parser wertet Ausdruecke wie "Var " + #var aus <--
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sConvLegends, sDummy, true);
                sLabels = sLabels.substr(nPos[0] + 1);
                if (sConvLegends != "\"\"")
                {
                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), _pInfo.sContStyles[nStyle].c_str());
                    nLegends++;
                }
                if (nStyle == _pInfo.nStyleMax - 1)
                    nStyle = 0;
                else
                    nStyle++;
            }
            nTypeCounter[0]++;
        }
        else
        {
            if ((v_mDataPlots.size() > 1 && !(_pData.getColorMask() || _pData.getAlphaMask()))
                    || (v_mDataPlots.size() > 2 && (_pData.getColorMask() || _pData.getAlphaMask()))
                    || (nFunctions && v_mDataPlots.size()))
            {
                if (_pData.getContLabels() && _pInfo.sCommand.substr(0, 4) != "cont")
                {
                    _graph->Cont(v_mDataPlots[nTypeCounter[1]][2], ("t" + _pInfo.sContStyles[nStyle]).c_str(), ("val " + toString(_pData.getNumContLines())).c_str());
                }
                else if (!_pData.getContLabels() && _pInfo.sCommand.substr(0, 4) != "cont")
                    _graph->Cont(v_mDataPlots[nTypeCounter[1]][2], _pInfo.sContStyles[nStyle].c_str(), ("val " + toString(_pData.getNumContLines())).c_str());
                nPos[1] = sDataLabels.find(';');
                sConvLegends = sDataLabels.substr(0, nPos[1]);
                // --> Der String-Parser wertet Ausdruecke wie "Var " + #var aus <--
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sConvLegends, sDummy, true);
                sDataLabels = sDataLabels.substr(nPos[1] + 1);
                if (sConvLegends != "\"\"")
                {
                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), _pInfo.sContStyles[nStyle].c_str());
                    nLegends++;
                }
                if (nStyle == _pInfo.nStyleMax - 1)
                    nStyle = 0;
                else
                    nStyle++;
            }
            nTypeCounter[1]++;
        }
        if ((_pData.getColorMask() || _pData.getAlphaMask()) && vType.size() > nType + 1)
            nType++;
    }
    // --> Position der Legende etwas aendern <--
    if (nFunctions > 1 && nLegends && !_pData.getSchematic() && nPlotCompose + 1 == nPlotComposeSize)
    {
        _graph->Legend(1.35, 1.2);
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates an actual
/// single two-dimensional plot based upon the
/// current plotting command.
///
/// \param _mData mglData&
/// \param _mMaskData mglData&
/// \param _mAxisVals mglData*
/// \param _mContVec mglData&
/// \return bool
///
/////////////////////////////////////////////////
bool Plot::plot2d(mglData& _mData, mglData& _mMaskData, mglData* _mAxisVals, mglData& _mContVec)
{
    /*if (_option.getbDebug())
        cerr << "|-> DEBUG: generating 2D-Plot..." << endl;*/

    if (_pData.getCutBox()
            && _pInfo.sCommand.substr(0, 4) != "cont"
            && _pInfo.sCommand.substr(0, 4) != "grad"
            && _pInfo.sCommand.substr(0, 4) != "dens"
            && _pInfo.sCommand != "implot")
        _graph->SetCutBox(CalcCutBox(_pData.getRotateAngle(1), 0, _pData.getCoords()), CalcCutBox(_pData.getRotateAngle(1), 1, _pData.getCoords()));

    // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
    if (_pInfo.sCommand.substr(0, 4) == "mesh")
    {
        if (_pData.getBars())
            _graph->Boxs(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("#").c_str());
        else
            _graph->Mesh(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else if (_pInfo.sCommand.substr(0, 4) == "surf")
    {
        if (_pData.getBars())
            _graph->Boxs(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        else if (_pData.getColorMask())
            _graph->SurfC(_mAxisVals[0], _mAxisVals[1], _mData, _mMaskData, _pData.getColorScheme().c_str());
        else if (_pData.getAlphaMask())
            _graph->SurfA(_mAxisVals[0], _mAxisVals[1], _mData, _mMaskData, _pData.getColorScheme().c_str());
        else
            _graph->Surf(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        //cerr << "\"" << _pData.getColorScheme().c_str() << "\" " << strlen(_pData.getColorScheme().c_str()) << endl;
    }
    else if (_pInfo.sCommand.substr(0, 4) == "cont")
    {
        if (_pData.getContLabels())
        {
            if (_pData.getContFilled())
                _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("t").c_str());
        }
        else if (_pData.getContProj())
        {
            if (_pData.getContFilled())
            {
                _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
                _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
            }
            else
                _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
        }
        else if (_pData.getContFilled())
        {
            _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "k");
        }
        else
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else if (_pInfo.sCommand.substr(0, 4) == "grad")
    {
        if (_pData.getHighRes() || !_option.isDraftMode())
        {
            if (_pData.getContFilled() && _pData.getContProj())
                _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeMedium().c_str(), "value 10");
            else
                _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str(), "value 10");
        }
        else
        {
            if (_pData.getContFilled() && _pData.getContProj())
                _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeMedium().c_str());
            else
                _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        }
        if (!(_pData.getContFilled() && _pData.getContProj()))
            _graph->Dens(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeLight().c_str());
    }
    else if (_pInfo.sCommand.substr(0, 4) == "dens" || _pInfo.sCommand == "implot")
    {
        if (_pData.getBars())
            _graph->Tile(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        else
            _graph->Dens(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else
        return false;

    if (_pData.getCutBox()
            && _pInfo.sCommand.substr(0, 4) != "cont"
            && _pInfo.sCommand.substr(0, 4) != "grad"
            && _pInfo.sCommand.substr(0, 4) != "dens")
        _graph->SetCutBox(mglPoint(0), mglPoint(0));
    // --> Ggf. Konturlinien ergaenzen <--
    if (_pData.getContProj() && _pInfo.sCommand.substr(0, 4) != "cont")
    {
        if (_pData.getContFilled() && _pInfo.sCommand.substr(0, 4) != "dens")
        {
            _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
        }
        else if (_pInfo.sCommand.substr(0, 4) == "dens" && _pData.getContFilled())
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
        else
            _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
    }
    if (_pData.getContLabels() && _pInfo.sCommand.substr(0, 4) != "cont" && *_pInfo.nFunctions == 1)
    {
        _graph->Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, ("t" + _pInfo.sContStyles[*_pInfo.nStyle]).c_str());
        if (_pData.getContFilled())
            _graph->ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function wraps the
/// creation of all one-dimensional plots
/// (e.g. line or point plots).
///
/// \param vType vector<short>&
/// \param nStyle int&
/// \param nLegends size_t&
/// \param nFunctions int
/// \param nPlotCompose size_t
/// \param nPlotComposeSize size_t
/// \return void
///
/////////////////////////////////////////////////
void Plot::createStdPlot(vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    const short TYPE_FUNC = 1;
    const short TYPE_DATA = -1;

    mglData _mData(_pInfo.nSamples);
    mglData _mPlotAxes;
    mglData _mData2[2];


    int nPos[2] = {0, 0};
    int nTypeCounter[2] = {0, 0};
    int nLastDataCounter = 0;
    int nCurrentStyle = 0;

    for (unsigned int nType = 0; nType < vType.size(); nType++)
    {
        // Copy the data to the relevant memory
        if (vType[nType] == TYPE_FUNC)
        {
            for (int i = 0; i < 2; i++)
                _mData2[i].Create(_pInfo.nSamples);
            _mData.Create(_pInfo.nSamples);

            StripSpaces(sLabels);

            for (long int i = 0; i < _pInfo.nSamples; i++)
            {
                _mData.a[i] = _pData.getData(i, nTypeCounter[0]);
            }
            if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                _mData = fmod(_mData, 2.0 * M_PI);
            _mPlotAxes = _mAxisVals[0];

            if (_pData.getRegion() && vType.size() > nType + 1 && vType[nType + 1] == TYPE_DATA)
            {
                _mData2[0] = v_mDataPlots[nTypeCounter[1]][1];
                nTypeCounter[1]++;
            }
            else if (_pData.getRegion() && vType.size() > nType + 1 && vType[nType + 1] == TYPE_FUNC)
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                    _mData2[0].a[i] = _pData.getData(i, nTypeCounter[0] + 1);
                nTypeCounter[0]++;
            }
            else
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                    _mData2[0].a[i] = 0.0;
            }
            if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                _mData2[0] = fmod(_mData2[0], 2.0 * M_PI);
            if (_pData.getAxisbind(nType)[0] == 'r')
            {
                _mData = (_mData - _pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (_pInfo.dSecAxisRanges[1][1] - _pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[YCOORD][0];
            }
            if (_pData.getRegion() && _pData.getAxisbind(nType + 1)[0] == 'r')
            {
                _mData2[0] = (_mData2[0] - _pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (_pInfo.dSecAxisRanges[1][1] - _pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[YCOORD][0];
            }
        }
        else
        {
            StripSpaces(sDataLabels);
            // Fallback for all bended coordinates, which are not covered by the second case
            if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RT || _pData.getCoords() == PlotData::POLAR_RZ)
                v_mDataPlots[nTypeCounter[1]][0] = fmod(v_mDataPlots[nTypeCounter[1]][0], 2.0 * M_PI);

            if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                v_mDataPlots[nTypeCounter[1]][1] = fmod(v_mDataPlots[nTypeCounter[1]][1], 2.0 * M_PI);


            _mData = v_mDataPlots[nTypeCounter[1]][1];
            _mPlotAxes = v_mDataPlots[nTypeCounter[1]][0];

            if (_pData.getBoxplot())
            {
                _mData2[0].Create(v_mDataPlots[nTypeCounter[1]].size());
                for (size_t col = 0; col < v_mDataPlots[nTypeCounter[1]].size(); col++)
                {
                    _mData2[0].a[col] = nTypeCounter[1] + nLastDataCounter + 0.5 + col;
                }
                nLastDataCounter += v_mDataPlots[nTypeCounter[1]].size() - 2; // it's always incremented by 1

                long int maxdim = v_mDataPlots[nTypeCounter[1]][1].nx;
                for (size_t data = 2; data < v_mDataPlots[nTypeCounter[1]].size(); data++)
                {
                    if (maxdim < v_mDataPlots[nTypeCounter[1]][data].nx)
                        maxdim = v_mDataPlots[nTypeCounter[1]][data].nx;
                }
                _mData.Create(maxdim, v_mDataPlots[nTypeCounter[1]].size() - 1);
                for (size_t data = 0; data < v_mDataPlots[nTypeCounter[1]].size() - 1; data++)
                {
                    for (int col = 0; col < v_mDataPlots[nTypeCounter[1]][data + 1].nx; col++)
                    {
                        _mData.a[col + data * maxdim] = v_mDataPlots[nTypeCounter[1]][data + 1].a[col];
                    }
                }
                _mData.Transpose();
            }
            else if (_pData.getBars() || _pData.getHBars())
            {
                long int maxdim = v_mDataPlots[nTypeCounter[1]][1].nx;
                for (size_t data = 2; data < v_mDataPlots[nTypeCounter[1]].size(); data++)
                {
                    if (maxdim < v_mDataPlots[nTypeCounter[1]][data].nx)
                        maxdim = v_mDataPlots[nTypeCounter[1]][data].nx;
                }
                _mData.Create(maxdim, v_mDataPlots[nTypeCounter[1]].size() - 1);
                for (size_t data = 0; data < v_mDataPlots[nTypeCounter[1]].size() - 1; data++)
                {
                    for (int col = 0; col < v_mDataPlots[nTypeCounter[1]][data + 1].nx; col++)
                    {
                        _mData.a[col + data * maxdim] = v_mDataPlots[nTypeCounter[1]][data + 1].a[col];
                    }
                }
            }
            else
            {
                if (_pData.getInterpolate() && getNN(_mData) >= _pData.getSamples())
                {
                    if (_pData.getRegion() && vType.size() > nType + 1 && vType[nType + 1] == TYPE_DATA)
                    {
                        _mData2[0] = v_mDataPlots[nTypeCounter[1] + 1][1];
                        nTypeCounter[1]++;
                    }
                    else if (_pData.getRegion() && vType.size() > nType + 1 && vType[nType + 1] == TYPE_FUNC)
                    {
                        for (long int i = 0; i < _pInfo.nSamples; i++)
                            _mData2[0].a[i] = _pData.getData(i, nTypeCounter[0]);
                        nTypeCounter[0]++;
                    }
                    else
                    {
                        // If the current data dimension is higher than 2, then we
                        // expand it into an array of curves
                        if (v_mDataPlots[nTypeCounter[1]].size() >= 3)
                        {
                            long int maxdim = v_mDataPlots[nTypeCounter[1]][1].nx;

                            // Find the largest x-set
                            for (size_t data = 2; data < v_mDataPlots[nTypeCounter[1]].size(); data++)
                            {
                                if (maxdim < v_mDataPlots[nTypeCounter[1]][data].nx)
                                    maxdim = v_mDataPlots[nTypeCounter[1]][data].nx;
                            }

                            // Create the target array and copy the data
                            _mData.Create(maxdim, v_mDataPlots[nTypeCounter[1]].size() - 1);

                            for (size_t data = 0; data < v_mDataPlots[nTypeCounter[1]].size() - 1; data++)
                            {
                                for (int col = 0; col < v_mDataPlots[nTypeCounter[1]][data + 1].nx; col++)
                                {
                                    _mData.a[col + data * maxdim] = v_mDataPlots[nTypeCounter[1]][data + 1].a[col];
                                }
                            }
                        }

                        // Create an zero-error data set
                        _mData2[0].Create(getNN(_mData));
                        for (long int i = 0; i < getNN(_mData); i++)
                            _mData2[0].a[i] = 0.0;
                    }
                }
                else if (_pData.getxError() && _pData.getyError() && v_mDataPlots[nTypeCounter[1]].size() >= 4)
                {
                    _mData2[0] = v_mDataPlots[nTypeCounter[1]][2];
                    _mData2[1] = v_mDataPlots[nTypeCounter[1]][3];
                }
                else if ((_pData.getyError() || _pData.getxError()) && v_mDataPlots[nTypeCounter[1]].size() >= 3)
                {
                    _mData2[0].Create(getNN(_mData));
                    _mData2[1].Create(getNN(_mData));
                    if (_pData.getyError() && !_pData.getxError())
                    {
                        _mData2[1] = v_mDataPlots[nTypeCounter[1]][2];
                        for (long int i = 0; i < getNN(_mData); i++)
                            _mData2[0].a[i] = 0.0;
                    }
                    else if (_pData.getyError() && _pData.getxError())
                    {
                        _mData2[0] = v_mDataPlots[nTypeCounter[1]][2];
                        _mData2[1] = v_mDataPlots[nTypeCounter[1]][2];
                    }
                    else
                    {
                        _mData2[0] = v_mDataPlots[nTypeCounter[1]][2];
                        for (long int i = 0; i < getNN(_mData); i++)
                            _mData2[1].a[i] = 0.0;
                    }
                }
                else if (v_mDataPlots[nTypeCounter[1]].size() >= 3)
                {
                    long int maxdim = v_mDataPlots[nTypeCounter[1]][1].nx;

                    // Find the largest x-set
                    for (size_t data = 2; data < v_mDataPlots[nTypeCounter[1]].size(); data++)
                    {
                        if (maxdim < v_mDataPlots[nTypeCounter[1]][data].nx)
                            maxdim = v_mDataPlots[nTypeCounter[1]][data].nx;
                    }

                    // Create the target array and copy the data
                    _mData.Create(maxdim, v_mDataPlots[nTypeCounter[1]].size() - 1);

                    for (size_t data = 0; data < v_mDataPlots[nTypeCounter[1]].size() - 1; data++)
                    {
                        for (int col = 0; col < v_mDataPlots[nTypeCounter[1]][data + 1].nx; col++)
                        {
                            _mData.a[col + data * maxdim] = v_mDataPlots[nTypeCounter[1]][data + 1].a[col];
                        }
                    }
                }
            }

            if (_pData.getAxisbind(nType)[1] == 't')
            {
                for (int i = 0; i < getNN(_mPlotAxes); i++)
                {
                    if (_mPlotAxes.a[i] < _pInfo.dSecAxisRanges[0][0] || _mPlotAxes.a[i] > _pInfo.dSecAxisRanges[0][1])
                        _mPlotAxes.a[i] = NAN;
                }
                _mPlotAxes = (_mPlotAxes - _pInfo.dSecAxisRanges[0][0]) * (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) / (_pInfo.dSecAxisRanges[0][1] - _pInfo.dSecAxisRanges[0][0]) + _pInfo.dRanges[XCOORD][0];
            }
            else
            {
                for (int i = 0; i < getNN(_mPlotAxes); i++)
                {
                    if (_mPlotAxes.a[i] < _pInfo.dRanges[XCOORD][0] || _mPlotAxes.a[i] > _pInfo.dRanges[XCOORD][1])
                        _mPlotAxes.a[i] = NAN;
                }
            }

            if (_pData.getAxisbind(nType)[0] == 'r')
            {
                _mData = (_mData - _pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (_pInfo.dSecAxisRanges[1][1] - _pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[YCOORD][0];
            }

            if (_pData.getRegion() && _pData.getAxisbind(nType + 1)[0] == 'r' && getNN(_mData2[0]) > 1)
            {
                _mData2[0] = (_mData2[0] - _pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (_pInfo.dSecAxisRanges[1][1] - _pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[YCOORD][0];
            }

            if (_pData.getyError() && _pData.getAxisbind(nType)[0] == 'r')
            {
                _mData2[1] = (_mData2[1] - _pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (_pInfo.dSecAxisRanges[1][1] - _pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[YCOORD][0];
            }

            if (_pData.getxError() && _pData.getAxisbind(nType)[1] == 't')
            {
                _mData2[0] = (_mData2[0] - _pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) / (_pInfo.dSecAxisRanges[0][1] - _pInfo.dSecAxisRanges[0][0]) + _pInfo.dRanges[XCOORD][0];
            }

        }

        // Store the current style
        nCurrentStyle = nStyle;

        // Create the plot
        if (!plotstd(_mData, _mPlotAxes, _mData2, vType[nType]))
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            clearData();
            return;
        }

        // Increment the style counter
        if (nStyle == _pInfo.nStyleMax - 1)
            nStyle = 0;
        else
            nStyle++;

        // Create the legend
        if (vType[nType] == TYPE_FUNC)
        {
            if (_pData.getRegion() && vType.size() > nType + 1)
            {
                for (int k = 0; k < 2; k++)
                {
                    nPos[0] = sLabels.find(';');
                    sConvLegends = sLabels.substr(0, nPos[0]) + " -nq";
                    sLabels = sLabels.substr(nPos[0] + 1);

                    // Apply the string parser
                    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sConvLegends, sDummy, true);

                    // Add new surrounding quotation marks
                    sConvLegends = "\"" + sConvLegends + "\"";

                    // Add the legend
                    if (sConvLegends != "\"\"")
                    {
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), _pInfo.sLineStyles[nCurrentStyle].c_str());
                        nLegends++;
                    }

                    if (nCurrentStyle == _pInfo.nStyleMax - 1)
                        nCurrentStyle = 0;
                    else
                        nCurrentStyle++;
                }
            }
            else
            {
                nPos[0] = sLabels.find(';');
                sConvLegends = sLabels.substr(0, nPos[0]) + " -nq";
                sLabels = sLabels.substr(nPos[0] + 1);

                // Apply the string parser
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sConvLegends, sDummy, true);

                // While the legend string is not empty
                while (sConvLegends.length())
                {
                    // Get the next legend string
                    string sLegend = sConvLegends.substr(0, sConvLegends.find('\n'));
                    if (sConvLegends.find('\n') != string::npos)
                        sConvLegends.erase(0, sConvLegends.find('\n')+1);
                    else
                        sConvLegends.clear();

                    // Add new surrounding quotation marks
                    sLegend = "\"" + sLegend + "\"";

                    // Add the legend
                    if (sLegend != "\"\"")
                    {
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sLegend.substr(1, sLegend.length() - 2))).c_str(), getLegendStyle(_pInfo.sLineStyles[nCurrentStyle]).c_str());
                        nLegends++;
                    }
                    if (nCurrentStyle == _pInfo.nStyleMax - 1)
                        nCurrentStyle = 0;
                    else
                        nCurrentStyle++;
                }
            }
            nTypeCounter[0]++;
        }
        else
        {
            nPos[1] = sDataLabels.find(';');
            sConvLegends = sDataLabels.substr(0, nPos[1]) + " -nq";
            sDataLabels = sDataLabels.substr(nPos[1] + 1);

            // Apply the string parser
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sConvLegends, sDummy, true);

            // While the legend string is not empty
            while (sConvLegends.length())
            {
                // Get the next legend string
                string sLegend = sConvLegends.substr(0, sConvLegends.find('\n'));
                if (sConvLegends.find('\n') != string::npos)
                    sConvLegends.erase(0, sConvLegends.find('\n')+1);
                else
                    sConvLegends.clear();

                // Add new surrounding quotation marks
                sLegend = "\"" + sLegend + "\"";

                // Add the legend
                if (sLegend != "\"\"")
                {
                    nLegends++;
                    if (_pData.getBoxplot())
                    {
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sLegend.substr(1, sLegend.length() - 2))).c_str(), getLegendStyle(_pInfo.sLineStyles[nCurrentStyle]).c_str());
                    }
                    else if (!_pData.getxError() && !_pData.getyError())
                    {
                        if ((_pData.getInterpolate() && v_mDataPlots[nTypeCounter[1]][0].nx >= _pInfo.nSamples) || _pData.getBars() || _pData.getHBars())
                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sLegend.substr(1, sLegend.length() - 2))).c_str(), getLegendStyle(_pInfo.sLineStyles[nCurrentStyle]).c_str());
                        else if (_pData.getConnectPoints() || (_pData.getInterpolate() && v_mDataPlots[nTypeCounter[1]][0].nx >= 0.9 * _pInfo.nSamples))
                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sLegend.substr(1, sLegend.length() - 2))).c_str(), getLegendStyle(_pInfo.sConPointStyles[nCurrentStyle]).c_str());
                        else if (_pData.getStepplot())
                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sLegend.substr(1, sLegend.length() - 2))).c_str(), getLegendStyle(_pInfo.sLineStyles[nCurrentStyle]).c_str());
                        else
                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sLegend.substr(1, sLegend.length() - 2))).c_str(), getLegendStyle(_pInfo.sPointStyles[nCurrentStyle]).c_str());
                    }
                    else
                    {
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sLegend.substr(1, sLegend.length() - 2))).c_str(), getLegendStyle(_pInfo.sPointStyles[nCurrentStyle]).c_str());
                    }
                }
                if (nCurrentStyle == _pInfo.nStyleMax - 1)
                    nCurrentStyle = 0;
                else
                    nCurrentStyle++;
            }

            nTypeCounter[1]++;
        }
        if (_pData.getRegion() && vType.size() > nType + 1 && getNN(_mData2[0]) > 1)
            nType++;
    }


    for (unsigned int i = 0; i < _pData.getHLinesSize(); i++)
    {
        if (_pData.getHLines(i).sDesc.length())
        {
            _graph->Line(mglPoint(_pInfo.dRanges[XCOORD][0], _pData.getHLines(i).dPos), mglPoint(_pInfo.dRanges[XCOORD][1], _pData.getHLines(i).dPos), _pData.getHLines(i).sStyle.c_str(), 100);
            _graph->Puts(_pData.getxLogscale() ? _pInfo.dRanges[XCOORD][0] + 0.03 * log10(fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1])) : _pInfo.dRanges[XCOORD][0] + 0.03 * fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]),
                         _pData.getyLogscale() ? _pData.getHLines(i).dPos + 0.01 * log10(fabs(_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0])) : _pData.getHLines(i).dPos + 0.01 * fabs(_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]),
                         fromSystemCodePage(_pData.getHLines(i).sDesc).c_str(), ":kL");
        }
    }

    for (unsigned int i = 0; i < _pData.getVLinesSize(); i++)
    {
        if (_pData.getVLines(i).sDesc.length())
        {
            _graph->Line(mglPoint(_pData.getVLines(i).dPos, _pInfo.dRanges[YCOORD][0]), mglPoint(_pData.getVLines(i).dPos, _pInfo.dRanges[YCOORD][1]), _pData.getVLines(i).sStyle.c_str());
            _graph->Puts(mglPoint(_pData.getxLogscale() ? _pData.getVLines(i).dPos - 0.01 * log10(fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1])) : _pData.getVLines(i).dPos - 0.01 * fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]),
                                  _pData.getyLogscale() ? _pInfo.dRanges[YCOORD][0] + 0.05 * log10(fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1])) : _pInfo.dRanges[YCOORD][0] + 0.05 * fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1])),
                         mglPoint(_pData.getxLogscale() ? _pData.getVLines(i).dPos - 0.01 * log10(fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1])) : _pData.getVLines(i).dPos - 0.01 * fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]),
                                  _pInfo.dRanges[YCOORD][1]),
                         fromSystemCodePage(_pData.getVLines(i).sDesc).c_str(), ":kL");
        }
    }

    if (nLegends && !_pData.getSchematic() && nPlotCompose + 1 == nPlotComposeSize)
        _graph->Legend(_pData.getLegendPosition());
}


/////////////////////////////////////////////////
/// \brief This member function creates an actual
/// single one-dimensional plot based upon the
/// current plotting command.
///
/// \param _mData mglData&
/// \param _mAxisVals mglData&
/// \param _mData2[2] mglData
/// \param nType const short
/// \return bool
///
/////////////////////////////////////////////////
bool Plot::plotstd(mglData& _mData, mglData& _mAxisVals, mglData _mData2[2], const short nType)
{
#warning NOTE (numere#3#08/15/21): Temporary fix for MathGL misbehaviour
    if (!_pData.getBoxplot() && !_pData.getyError() && !_pData.getxError() && !_pData.getBars() && !_pData.getHBars() && !_pData.getStepplot())
    {
        _mData = duplicatePoints(_mData);
        _mAxisVals = duplicatePoints(_mAxisVals);
        _mData2[0] = duplicatePoints(_mData2[0]);
        _mData2[1] = duplicatePoints(_mData2[1]);

        if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
        {
            removeNegativeValues(_mData);
            removeNegativeValues(_mData2[0]);
        }
    }

    if (nType == 1)
    {
        if (!_pData.getArea() && !_pData.getRegion())
            _graph->Plot(_mAxisVals, _mData, ("a" + _pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
        else if (_pData.getRegion() && getNN(_mData2[0]) > 1)
        {
            if (*_pInfo.nStyle == _pInfo.nStyleMax - 1)
                _graph->Region(_mAxisVals, _mData, _mData2[0], ("a{" + _pData.getColors().substr(*_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(0, 1) + "7}").c_str());
            else
                _graph->Region(_mAxisVals, _mData, _mData2[0], ("a{" + _pData.getColors().substr(*_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(*_pInfo.nStyle + 1, 1) + "7}").c_str());
            _graph->Plot(_mAxisVals, _mData, ("a" + _pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
            if (*_pInfo.nStyle == _pInfo.nStyleMax - 1)
                _graph->Plot(_mAxisVals, _mData2[0], ("a" + _pInfo.sLineStyles[0]).c_str());
            else
                _graph->Plot(_mAxisVals, _mData2[0], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle + 1]).c_str());
        }
        else if (_pData.getArea() || _pData.getRegion())
            _graph->Area(_mAxisVals, _mData, ("a" + _pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
    }
    else
    {
        if (_pData.getBoxplot())
        {
            _graph->BoxPlot(_mData2[0], _mData, _pInfo.sLineStyles[*_pInfo.nStyle].c_str());
        }
        else if (!_pData.getxError() && !_pData.getyError())
        {
            if (_pData.getInterpolate() && countValidElements(_mData) >= (size_t)_pInfo.nSamples)
            {
                if (!_pData.getArea() && !_pData.getBars() && !_pData.getRegion() && !_pData.getStepplot())
                    _graph->Plot(_mAxisVals, _mData, ("a" + expandStyleForCurveArray(_pInfo.sLineStyles[*_pInfo.nStyle], _mData.ny > 1)).c_str());
                else if (_pData.getBars() && !_pData.getArea() && !_pData.getRegion() && !_pData.getStepplot())
                    _graph->Bars(_mAxisVals, _mData, (composeColoursForBarChart(_mData.ny) + "^").c_str());
                else if (_pData.getRegion() && getNN(_mData2[0]) > 1)
                {
                    if (*_pInfo.nStyle == _pInfo.nStyleMax - 1)
                        _graph->Region(_mAxisVals, _mData, _mData2[0], ("a{" + _pData.getColors().substr(*_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(0, 1) + "7}").c_str());
                    else
                        _graph->Region(_mAxisVals, _mData, _mData2[0], ("a{" + _pData.getColors().substr(*_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(*_pInfo.nStyle + 1, 1) + "7}").c_str());
                    _graph->Plot(_mAxisVals, _mData, ("a" + _pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
                    if (*_pInfo.nStyle == _pInfo.nStyleMax - 1)
                        _graph->Plot(_mAxisVals, _mData2[0], ("a" + _pInfo.sLineStyles[0]).c_str());
                    else
                        _graph->Plot(_mAxisVals, _mData2[0], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle + 1]).c_str());
                }
                else if (_pData.getArea() || _pData.getRegion())
                    _graph->Area(_mAxisVals, _mData, ("a" + _pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
                else if (!_pData.getBars() && !_pData.getArea() && !_pData.getHBars() && _pData.getStepplot())
                    _graph->Step(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
            }
            else if (_pData.getConnectPoints() || (_pData.getInterpolate() && countValidElements(_mData) >= 0.9 * _pInfo.nSamples))
            {
                if (!_pData.getArea() && !_pData.getBars() && !_pData.getStepplot())
                    _graph->Plot(_mAxisVals, _mData, ("a" + expandStyleForCurveArray(_pInfo.sConPointStyles[*_pInfo.nStyle], _mData.ny > 1)).c_str());
                else if (_pData.getBars() && !_pData.getArea())
                    _graph->Bars(_mAxisVals, _mData, (composeColoursForBarChart(_mData.ny) + "^").c_str());
                else if (!_pData.getBars() && !_pData.getArea() && !_pData.getHBars() && _pData.getStepplot())
                    _graph->Step(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
                else
                    _graph->Area(_mAxisVals, _mData, ("a" + _pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
            }
            else
            {
                if (!_pData.getArea() && !_pData.getBars() && !_pData.getHBars() && !_pData.getStepplot())
                    _graph->Plot(_mAxisVals, _mData, ("a" + expandStyleForCurveArray(_pInfo.sPointStyles[*_pInfo.nStyle], _mData.ny > 1)).c_str());
                else if (_pData.getBars() && !_pData.getArea() && !_pData.getHBars() && !_pData.getStepplot())
                    _graph->Bars(_mAxisVals, _mData, (composeColoursForBarChart(_mData.ny) + "^").c_str());
                else if (!_pData.getBars() && !_pData.getArea() && _pData.getHBars() && !_pData.getStepplot())
                    _graph->Barh(_mAxisVals, _mData, (composeColoursForBarChart(_mData.ny) + "^").c_str());
                else if (!_pData.getBars() && !_pData.getArea() && !_pData.getHBars() && _pData.getStepplot())
                    _graph->Step(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
                else
                    _graph->Stem(_mAxisVals, _mData, _pInfo.sConPointStyles[*_pInfo.nStyle].c_str());
            }
        }
        else if (_pData.getxError() || _pData.getyError())
        {
            _graph->Error(_mAxisVals, _mData, _mData2[0], _mData2[1], _pInfo.sPointStyles[*_pInfo.nStyle].c_str());
        }
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
    if (sFunc != "<<empty>>")
    {
        mglData _mData(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);
        mglData _mContVec(15);
        double dMin = _pData.getMin();
        double dMax = _pData.getMax();

        weightedRange(PlotData::ALLRANGES, dMin, dMax);

        if (!isnan(_pData.getColorRange()))
        {
            dMin = _pData.getColorRange();
            dMax = _pData.getColorRange(1);
        }
        for (int nCont = 0; nCont < 15; nCont++)
        {
            _mContVec.a[nCont] = nCont * (dMax - dMin) / 14.0 + dMin;
            //cerr << _mContVec.a[nCont];
        }
        //cerr << endl;
        _mContVec.a[7] = (dMax - dMin) / 2.0 + dMin;

        //int nPos = 0;
        StripSpaces(sLabels);

        for (long int i = 0; i < _pInfo.nSamples; i++)
        {
            for (long int j = 0; j < _pInfo.nSamples; j++)
            {
                for (long int k = 0; k < _pInfo.nSamples; k++)
                {
                    // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                    _mData.a[i + _pInfo.nSamples * j + _pInfo.nSamples * _pInfo.nSamples * k] = _pData.getData(i, j, k);
                }
            }
        }

        if (_pData.getCutBox()
                && _pInfo.sCommand.substr(0, 4) != "cont"
                && _pInfo.sCommand.substr(0, 4) != "grad"
                && (_pInfo.sCommand.substr(0, 4) != "dens" || (_pInfo.sCommand.substr(0, 4) == "dens" && _pData.getCloudPlot())))
        {
            _graph->SetCutBox(CalcCutBox(_pData.getRotateAngle(1), 0, _pData.getCoords(), true), CalcCutBox(_pData.getRotateAngle(1), 1, _pData.getCoords(), true));
        }
        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
        if (_pInfo.sCommand.substr(0, 4) == "mesh")
            _graph->Surf3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("#").c_str(), "value 11");
        else if (_pInfo.sCommand.substr(0, 4) == "surf" && !_pData.getTransparency())
        {
            _graph->Surf3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), "value 11");
        }
        else if (_pInfo.sCommand.substr(0, 4) == "surf" && _pData.getTransparency())
            _graph->Surf3A(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _mData, _pData.getColorScheme().c_str(), "value 11");
        else if (_pInfo.sCommand.substr(0, 4) == "cont")
        {
            if (_pData.getContProj())
            {
                if (_pData.getContFilled())
                {
                    _graph->ContFX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                    _graph->ContFY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->ContFZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[ZCOORD][0]);
                    _graph->ContX(_mContVec, _mData.Sum("x"), "k", getProjBackground(_pData.getRotateAngle(1)));
                    _graph->ContY(_mContVec, _mData.Sum("y"), "k", getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.dRanges[ZCOORD][0]);
                }
                else
                {
                    _graph->ContX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                    _graph->ContY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->ContZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[ZCOORD][0]);
                }
            }
            else if (_pData.getContFilled())
            {
                for (unsigned short n = 0; n < _pData.getSlices(); n++)
                {
                    _graph->ContF3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("x").c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices() + 1));
                    _graph->Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, "kx", (n + 1)*_pInfo.nSamples / (_pData.getSlices() + 1));
                }
                for (unsigned short n = 0; n < _pData.getSlices(1); n++)
                {
                    _graph->ContF3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices(1) + 1));
                    _graph->Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, "k", (n + 1)*_pInfo.nSamples / (_pData.getSlices(1) + 1));
                }
                for (unsigned short n = 0; n < _pData.getSlices(2); n++)
                {
                    _graph->ContF3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("z").c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices(2) + 1));
                    _graph->Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, "kz", (n + 1)*_pInfo.nSamples / (_pData.getSlices(2) + 1));
                }
            }
            else
            {
                for (unsigned short n = 0; n < _pData.getSlices(); n++)
                    _graph->Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("x").c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices() + 1));
                for (unsigned short n = 0; n < _pData.getSlices(1); n++)
                    _graph->Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices(1) + 1));
                for (unsigned short n = 0; n < _pData.getSlices(2); n++)
                    _graph->Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("z").c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices(2) + 1));
            }
        }
        else if (_pInfo.sCommand.substr(0, 4) == "grad")
        {
            if (_pData.getHighRes() || !_option.isDraftMode())
            {
                if (_pData.getContFilled() && _pData.getContProj())
                    _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeMedium().c_str(), "value 10");
                else
                    _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), "value 10");
            }
            else
            {
                if (_pData.getContFilled() && _pData.getContProj())
                    _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeMedium().c_str());
                else
                    _graph->Grad(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str());
            }
            if (!(_pData.getContProj()))
            {
                _graph->Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeLight("x").c_str(), _pInfo.nSamples / 2);
                _graph->Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeLight().c_str(), _pInfo.nSamples / 2);
                _graph->Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeLight("z").c_str(), _pInfo.nSamples / 2);
            }
        }
        else if (_pInfo.sCommand.substr(0, 4) == "dens")
        {
            if (!(_pData.getContFilled() && _pData.getContProj()) && !_pData.getCloudPlot())
            {
                for (unsigned short n = 0; n < _pData.getSlices(); n++)
                    _graph->Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("x").c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices() + 1));
                for (unsigned short n = 0; n < _pData.getSlices(1); n++)
                    _graph->Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices(1) + 1));
                for (unsigned short n = 0; n < _pData.getSlices(2); n++)
                    _graph->Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("z").c_str(), (n + 1)*_pInfo.nSamples / (_pData.getSlices(2) + 1));
            }
            else if (_pData.getCloudPlot() && !(_pData.getContFilled() && _pData.getContProj()))
                _graph->Cloud(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str());
        }
        else
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            clearData();
            return;
        }

        if (_pData.getCutBox()
                && _pInfo.sCommand.substr(0, 4) != "cont"
                && _pInfo.sCommand.substr(0, 4) != "grad"
                && (_pInfo.sCommand.substr(0, 4) != "dens" || (_pInfo.sCommand.substr(0, 4) == "dens" && _pData.getCloudPlot())))
        {
            _graph->SetCutBox(mglPoint(0), mglPoint(0));
        }

        // --> Ggf. Konturlinien ergaenzen <--
        if (_pData.getContProj() && _pInfo.sCommand.substr(0, 4) != "cont")
        {
            if (_pData.getContFilled() && (_pInfo.sCommand.substr(0, 4) != "dens" && _pInfo.sCommand.substr(0, 4) != "grad"))
            {
                _graph->ContFX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                _graph->ContFY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                _graph->ContFZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[ZCOORD][0]);
                _graph->ContX(_mContVec, _mData.Sum("x"), "k", getProjBackground(_pData.getRotateAngle(1)));
                _graph->ContY(_mContVec, _mData.Sum("y"), "k", getProjBackground(_pData.getRotateAngle(1), 1));
                _graph->ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.dRanges[ZCOORD][0]);
            }
            else if ((_pInfo.sCommand.substr(0, 4) == "dens" || _pInfo.sCommand.substr(0, 4) == "grad") && _pData.getContFilled())
            {
                if (_pInfo.sCommand == "dens")
                {
                    _graph->DensX(_mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                    _graph->DensY(_mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->DensZ(_mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[ZCOORD][0]);
                }
                else
                {
                    _graph->DensX(_mData.Sum("x"), _pData.getColorSchemeLight().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                    _graph->DensY(_mData.Sum("y"), _pData.getColorSchemeLight().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                    _graph->DensZ(_mData.Sum("z"), _pData.getColorSchemeLight().c_str(), _pInfo.dRanges[ZCOORD][0]);
                }
                _graph->ContX(_mContVec, _mData.Sum("x"), "k", getProjBackground(_pData.getRotateAngle(1)));
                _graph->ContY(_mContVec, _mData.Sum("y"), "k", getProjBackground(_pData.getRotateAngle(1), 1));
                _graph->ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.dRanges[ZCOORD][0]);
            }
            else
            {
                _graph->ContX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1)));
                _graph->ContY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), getProjBackground(_pData.getRotateAngle(1), 1));
                _graph->ContZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[ZCOORD][0]);
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
    if (sFunc != "<<empty>>")
    {
        mglData _mData_x(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);
        mglData _mData_y(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);
        mglData _mData_z(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);

        StripSpaces(sLabels);

        for (long int i = 0; i < _pInfo.nSamples; i++)
        {
            for (long int j = 0; j < _pInfo.nSamples; j++)
            {
                for (long int k = 0; k < _pInfo.nSamples; k++)
                {
                    // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                    _mData_x.a[i + _pInfo.nSamples * j + _pInfo.nSamples * _pInfo.nSamples * k] = _pData.getData(i, j, 3 * k);
                    _mData_y.a[i + _pInfo.nSamples * j + _pInfo.nSamples * _pInfo.nSamples * k] = _pData.getData(i, j, 3 * k + 1);
                    _mData_z.a[i + _pInfo.nSamples * j + _pInfo.nSamples * _pInfo.nSamples * k] = _pData.getData(i, j, 3 * k + 2);
                }
            }
        }

        if (_pData.getCutBox())
        {
            _graph->SetCutBox(CalcCutBox(_pData.getRotateAngle(1),  0, _pData.getCoords(), true), CalcCutBox(_pData.getRotateAngle(1), 1, _pData.getCoords(), true));
        }
        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
        if (_pData.getFlow())
            _graph->Flow(_mData_x, _mData_y, _mData_z, _pData.getColorScheme("v").c_str());
        else if (_pData.getPipe())
            _graph->Pipe(_mData_x, _mData_y, _mData_z, _pData.getColorScheme().c_str());
        else if (_pData.getFixedLength())
            _graph->Vect(_mData_x, _mData_y, _mData_z, _pData.getColorScheme("f").c_str());
        else if (!_pData.getFixedLength())
            _graph->Vect(_mData_x, _mData_y, _mData_z, _pData.getColorScheme().c_str());
        else
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            clearData();
            return;
        }

        if (_pData.getCutBox())
        {
            _graph->SetCutBox(mglPoint(0), mglPoint(0));
        }
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
    if (sFunc != "<<empty>>")
    {
        mglData _mData_x(_pInfo.nSamples, _pInfo.nSamples);
        mglData _mData_y(_pInfo.nSamples, _pInfo.nSamples);

        StripSpaces(sLabels);

        for (long int i = 0; i < _pInfo.nSamples; i++)
        {
            for (long int j = 0; j < _pInfo.nSamples; j++)
            {
                // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                _mData_x.a[i + _pInfo.nSamples * j] = _pData.getData(i, j, 0);
                _mData_y.a[i + _pInfo.nSamples * j] = _pData.getData(i, j, 1);
            }
        }

        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
        if (_pData.getFlow())
            _graph->Flow(_mData_x, _mData_y, _pData.getColorScheme("v").c_str());
        else if (_pData.getPipe())
            _graph->Pipe(_mData_x, _mData_y, _pData.getColorScheme().c_str());
        else if (_pData.getFixedLength())
            _graph->Vect(_mData_x, _mData_y, _pData.getColorScheme("f").c_str());
        else if (!_pData.getFixedLength())
            _graph->Vect(_mData_x, _mData_y, _pData.getColorScheme().c_str());
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
/// creation of all two-dimensional drawings.
///
/// \param vDrawVector vector<string>&

/// \param nFunctions int&
/// \return void
///
/////////////////////////////////////////////////
void Plot::create2dDrawing(vector<string>& vDrawVector, int& nFunctions)
{
    string sStyle;
    string sTextString;
    string sDrawExpr;
    string sCurrentDrawingFunction;
    string sDummy;

    for (unsigned int v = 0; v < vDrawVector.size(); v++)
    {
        sStyle = "k";
        sTextString = "";
        sDrawExpr = "";
        sCurrentDrawingFunction = vDrawVector[v];
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentDrawingFunction))
        {
            for (int n = (int)sCurrentDrawingFunction.length() - 1; n >= 0; n--)
            {
                if (sCurrentDrawingFunction[n] == ',' && !isInQuotes(sCurrentDrawingFunction, (unsigned)n, true))
                {
                    sStyle = sCurrentDrawingFunction.substr(n + 1);
                    sCurrentDrawingFunction.erase(n);

                    break;
                }
            }
            sStyle = sStyle.substr(0, sStyle.rfind(')')) + " -nq";
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sStyle, sDummy, true);
        }
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentDrawingFunction))
        {
            for (int n = (int)sCurrentDrawingFunction.length() - 1; n >= 0; n--)
            {
                if (sCurrentDrawingFunction[n] == ',' && !isInQuotes(sCurrentDrawingFunction, (unsigned)n, true))
                {
                    sTextString = sCurrentDrawingFunction.substr(n + 1);
                    sCurrentDrawingFunction.erase(n);

                    break;
                }
            }
            sTextString += " -nq";
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sTextString, sDummy, true);
        }
        if (sCurrentDrawingFunction.back() == ')')
            sDrawExpr = sCurrentDrawingFunction.substr(sCurrentDrawingFunction.find('(') + 1, sCurrentDrawingFunction.rfind(')') - sCurrentDrawingFunction.find('(') - 1);
        else
            sDrawExpr = sCurrentDrawingFunction.substr(sCurrentDrawingFunction.find('(') + 1);
        if (sDrawExpr.find('{') != string::npos)
            convertVectorToExpression(sDrawExpr, _option);
        _parser.SetExpr(sDrawExpr);
        mu::value_type* vRes = _parser.Eval(nFunctions);
        std::vector<double> vResults = real({vRes, vRes+nFunctions});

        if (sCurrentDrawingFunction.substr(0, 6) == "trace(" || sCurrentDrawingFunction.substr(0, 5) == "line(")
        {
            if (nFunctions < 2)
                continue;
            if (nFunctions < 4)
                _graph->Line(mglPoint(), mglPoint(vResults[0], vResults[1]), sStyle.c_str());
            else
                _graph->Line(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 7) == "tracev(" || sCurrentDrawingFunction.substr(0, 6) == "linev(")
        {
            if (nFunctions < 2)
                continue;
            if (nFunctions < 4)
                _graph->Line(mglPoint(), mglPoint(vResults[0], vResults[1]), sStyle.c_str());
            else
                _graph->Line(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 5) == "face(" || sCurrentDrawingFunction.substr(0, 7) == "cuboid(")
        {
            if (nFunctions < 4)
                continue;
            if (nFunctions < 6)
                _graph->Face(mglPoint(vResults[2] - vResults[3] + vResults[1], vResults[3] + vResults[2] - vResults[0]),
                             mglPoint(vResults[2], vResults[3]),
                             mglPoint(vResults[0] - vResults[3] + vResults[1], vResults[1] + vResults[2] - vResults[0]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
            else if (nFunctions < 8)
                _graph->Face(mglPoint(vResults[4], vResults[5]),
                             mglPoint(vResults[2], vResults[3]),
                             mglPoint(vResults[0] + vResults[4] - vResults[2], vResults[1] + vResults[5] - vResults[3]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
            else
                _graph->Face(mglPoint(vResults[4], vResults[5]),
                             mglPoint(vResults[2], vResults[3]),
                             mglPoint(vResults[6], vResults[7]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 6) == "facev(")
        {
            if (nFunctions < 4)
                continue;
            if (nFunctions < 6)
                _graph->Face(mglPoint(vResults[0] + vResults[2] - vResults[3], vResults[1] + vResults[3] + vResults[2]),
                             mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]),
                             mglPoint(vResults[0] - vResults[3], vResults[1] + vResults[2]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
            else if (nFunctions < 8)
                _graph->Face(mglPoint(vResults[0] + vResults[4] + vResults[2], vResults[1] + vResults[3] + vResults[5]),
                             mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]),
                             mglPoint(vResults[0] + vResults[4], vResults[1] + vResults[5]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
            else
                _graph->Face(mglPoint(vResults[0] + vResults[4], vResults[1] + vResults[5]),
                             mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]),
                             mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[7]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 9) == "triangle(")
        {
            if (nFunctions < 4)
                continue;
            double c = hypot(vResults[2] - vResults[0], vResults[3] - vResults[1]) / 2.0 * sqrt(3) / hypot(vResults[2], vResults[3]);
            if (nFunctions < 6)
                _graph->Face(mglPoint((-vResults[0] + vResults[2]) / 2.0 - c * vResults[3], (-vResults[1] + vResults[3]) / 2.0 + c * vResults[2]),
                             mglPoint(vResults[2], vResults[3]),
                             mglPoint((-vResults[0] + vResults[2]) / 2.0 - c * vResults[3], (-vResults[1] + vResults[3]) / 2.0 + c * vResults[2]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
            else
                _graph->Face(mglPoint(vResults[4], vResults[5]),
                             mglPoint(vResults[2], vResults[3]),
                             mglPoint(vResults[4], vResults[5]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 10) == "trianglev(")
        {
            if (nFunctions < 4)
                continue;
            double c = sqrt(3.0) / 2.0;
            if (nFunctions < 6)
                _graph->Face(mglPoint((vResults[0] + 0.5 * vResults[2]) - c * vResults[3], (vResults[1] + 0.5 * vResults[3]) + c * vResults[2]),
                             mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]),
                             mglPoint((vResults[0] + 0.5 * vResults[2]) - c * vResults[3], (vResults[1] + 0.5 * vResults[3]) + c * vResults[2]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
            else
                _graph->Face(mglPoint(vResults[0] + vResults[4], vResults[1] + vResults[5]),
                             mglPoint(vResults[0] + vResults[2], vResults[1] + vResults[3]),
                             mglPoint(vResults[0] + vResults[4], vResults[1] + vResults[5]),
                             mglPoint(vResults[0], vResults[1]),
                             sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 7) == "sphere(")
        {
            if (nFunctions < 3)
                continue;
            _graph->Sphere(mglPoint(vResults[0], vResults[1]), vResults[2], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 5) == "drop(")
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
        else if (sCurrentDrawingFunction.substr(0, 7) == "circle(")
        {
            if (nFunctions < 3)
                continue;
            _graph->Circle(mglPoint(vResults[0], vResults[1]), vResults[2], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 4) == "arc(")
        {
            if (nFunctions < 5)
                continue;
            _graph->Arc(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 5) == "arcv(")
        {
            if (nFunctions < 5)
                continue;
            _graph->Arc(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2] + vResults[0], vResults[3] + vResults[1]), vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 6) == "point(")
        {
            if (nFunctions < 2)
                continue;
            _graph->Mark(mglPoint(vResults[0], vResults[1]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 6) == "curve(")
        {
            if (nFunctions < 8)
                continue;
            _graph->Curve(mglPoint(vResults[0], vResults[1]),
                          mglPoint(vResults[2], vResults[3]),
                          mglPoint(vResults[4], vResults[5]),
                          mglPoint(vResults[6], vResults[7]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 8) == "ellipse(")
        {
            if (nFunctions < 5)
                continue;
            _graph->Ellipse(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 9) == "ellipsev(")
        {
            if (nFunctions < 5)
                continue;
            _graph->Ellipse(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2] + vResults[0], vResults[3] + vResults[1]), vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 5) == "text(")
        {
            if (!sTextString.length())
            {
                sTextString = sStyle;
                sStyle = "k";
            }
            if (nFunctions >= 4)
                _graph->Puts(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), sTextString.c_str(), sStyle.c_str());
            else if (nFunctions >= 2)
                _graph->Puts(mglPoint(vResults[0], vResults[1]), sTextString.c_str(), sStyle.c_str());
            else
                continue;
        }
        else if (sCurrentDrawingFunction.substr(0, 8) == "polygon(")
        {
            if (nFunctions < 5 || vResults[4] < 3)
                continue;
            _graph->Polygon(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), (int)vResults[4], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 9) == "polygonv(")
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

/// \param nFunctions int&
/// \return void
///
/////////////////////////////////////////////////
void Plot::create3dDrawing(vector<string>& vDrawVector, int& nFunctions)
{
    string sStyle;
    string sTextString;
    string sDrawExpr;
    string sCurrentDrawingFunction;
    string sDummy;

    for (unsigned int v = 0; v < vDrawVector.size(); v++)
    {
        sStyle = "k";
        sTextString = "";
        sDrawExpr = "";
        sCurrentDrawingFunction = vDrawVector[v];
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentDrawingFunction))
        {
            for (int n = (int)sCurrentDrawingFunction.length() - 1; n >= 0; n--)
            {
                if (sCurrentDrawingFunction[n] == ',' && !isInQuotes(sCurrentDrawingFunction, (unsigned)n, true))
                {
                    sStyle = sCurrentDrawingFunction.substr(n + 1);
                    sCurrentDrawingFunction.erase(n);

                    break;
                }
            }
            sStyle = sStyle.substr(0, sStyle.rfind(')')) + " -nq";
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sStyle, sDummy, true);
        }
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentDrawingFunction))
        {
            for (int n = (int)sCurrentDrawingFunction.length() - 1; n >= 0; n--)
            {
                if (sCurrentDrawingFunction[n] == ',' && !isInQuotes(sCurrentDrawingFunction, (unsigned)n, true))
                {
                    sTextString = sCurrentDrawingFunction.substr(n + 1);
                    sCurrentDrawingFunction.erase(n);

                    break;
                }
            }
            sTextString += " -nq";
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sTextString, sDummy, true);
        }
        if (sCurrentDrawingFunction.back() == ')')
            sDrawExpr = sCurrentDrawingFunction.substr(sCurrentDrawingFunction.find('(') + 1, sCurrentDrawingFunction.rfind(')') - sCurrentDrawingFunction.find('(') - 1);
        else
            sDrawExpr = sCurrentDrawingFunction.substr(sCurrentDrawingFunction.find('(') + 1);
        if (sDrawExpr.find('{') != string::npos)
            convertVectorToExpression(sDrawExpr, _option);
        _parser.SetExpr(sDrawExpr);
        mu::value_type* vRes = _parser.Eval(nFunctions);
        std::vector<double> vResults = real({vRes, vRes+nFunctions});
        if (sCurrentDrawingFunction.substr(0, 6) == "trace(" || sCurrentDrawingFunction.substr(0, 5) == "line(")
        {
            if (nFunctions < 3)
                continue;
            if (nFunctions < 6)
                _graph->Line(mglPoint(), mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
            else
                _graph->Line(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 7) == "tracev(" || sCurrentDrawingFunction.substr(0, 6) == "linev(")
        {
            if (nFunctions < 3)
                continue;
            if (nFunctions < 6)
                _graph->Line(mglPoint(), mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
            else
                _graph->Line(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3] + vResults[0], vResults[4] + vResults[1], vResults[5] + vResults[2]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 5) == "face(")
        {
            if (nFunctions < 6)
                continue;
            if (nFunctions < 9)
                _graph->Face(mglPoint(vResults[3] - vResults[4] + vResults[1], vResults[4] + vResults[3] - vResults[0], vResults[5]),
                             mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[2] + vResults[5]),
                             mglPoint(vResults[0] - vResults[4] + vResults[1], vResults[1] + vResults[3] - vResults[0], vResults[2]),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
            else if (nFunctions < 12)
                _graph->Face(mglPoint(vResults[6], vResults[7], vResults[8]),
                             mglPoint(vResults[3], vResults[4], vResults[5]),
                             mglPoint(vResults[0] + vResults[6] - vResults[3], vResults[1] + vResults[7] - vResults[4], vResults[2] + vResults[8] - vResults[5]),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
            else
                _graph->Face(mglPoint(vResults[6], vResults[7], vResults[8]),
                             mglPoint(vResults[3], vResults[4], vResults[5]),
                             mglPoint(vResults[9], vResults[10], vResults[11]),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 6) == "facev(")
        {
            if (nFunctions < 6)
                continue;
            if (nFunctions < 9)
                _graph->Face(mglPoint(vResults[0] + vResults[3] - vResults[4], vResults[1] + vResults[4] + vResults[3], vResults[5] + vResults[2]),
                             mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[5] + vResults[2]),
                             mglPoint(vResults[0] - vResults[4], vResults[1] + vResults[3], vResults[2]),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
            else if (nFunctions < 12)
                _graph->Face(mglPoint(vResults[0] + vResults[6] + vResults[3], vResults[1] + vResults[7] + vResults[4], vResults[2] + vResults[8] + vResults[5]),
                             mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[4], vResults[2] + vResults[5]),
                             mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[7], vResults[2] + vResults[8]),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
            else
                _graph->Face(mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[7], vResults[2] + vResults[8]),
                             mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[2] + vResults[5]),
                             mglPoint(vResults[0] + vResults[9], vResults[1] + vResults[10], vResults[2] + vResults[11]),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 9) == "triangle(")
        {
            if (nFunctions < 6)
                continue;
            double c = sqrt((vResults[3] - vResults[0]) * (vResults[3] - vResults[0])
                            + (vResults[4] - vResults[1]) * (vResults[4] - vResults[1])
                            + (vResults[5] - vResults[2]) * (vResults[5] - vResults[2])) / 2.0 * sqrt(3) / hypot(vResults[3], vResults[4]);
            if (nFunctions < 9)
                _graph->Face(mglPoint((-vResults[0] + vResults[3]) / 2.0 - c * vResults[4], (-vResults[1] + vResults[4]) / 2.0 + c * vResults[3], (vResults[5] + vResults[2]) / 2.0),
                             mglPoint(vResults[3], vResults[4], vResults[5]),
                             mglPoint((-vResults[0] + vResults[3]) / 2.0 - c * vResults[4], (-vResults[1] + vResults[4]) / 2.0 + c * vResults[3], (vResults[5] + vResults[2]) / 2.0),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
            else
                _graph->Face(mglPoint(vResults[6], vResults[7], vResults[8]),
                             mglPoint(vResults[3], vResults[4], vResults[5]),
                             mglPoint(vResults[6], vResults[7], vResults[8]),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 10) == "trianglev(")
        {
            if (nFunctions < 6)
                continue;
            double c = sqrt((vResults[3]) * (vResults[3])
                            + (vResults[4]) * (vResults[4])
                            + (vResults[5]) * (vResults[5])) / 2.0 * sqrt(3) / hypot(vResults[3], vResults[4]);
            if (nFunctions < 9)
                _graph->Face(mglPoint((vResults[0] + 0.5 * vResults[3]) - c * vResults[4], (vResults[1] + 0.5 * vResults[4]) + c * vResults[3], (vResults[5] + 0.5 * vResults[2])),
                             mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[2] + vResults[5]),
                             mglPoint((vResults[0] + 0.5 * vResults[3]) - c * vResults[4], (vResults[1] + 0.5 * vResults[4]) + c * vResults[3], (vResults[5] + 0.5 * vResults[2])),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
            else
                _graph->Face(mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[7], vResults[2] + vResults[8]),
                             mglPoint(vResults[0] + vResults[3], vResults[1] + vResults[4], vResults[2] + vResults[5]),
                             mglPoint(vResults[0] + vResults[6], vResults[1] + vResults[7], vResults[2] + vResults[8]),
                             mglPoint(vResults[0], vResults[1], vResults[2]),
                             sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 7) == "cuboid(")
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

            _graph->Face(mglPoint(vResults[0], vResults[1], vResults[2]),
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx + _mDy,
                         sStyle.c_str());
            _graph->Face(mglPoint(vResults[0], vResults[1], vResults[2]),
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz + _mDx,
                         sStyle.c_str());
            _graph->Face(mglPoint(vResults[0], vResults[1], vResults[2]),
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy + _mDz,
                         sStyle.c_str());
            _graph->Face(mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx + _mDz,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy + _mDz,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx + _mDy + _mDz,
                         sStyle.c_str());
            _graph->Face(mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx + _mDy,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz + _mDy,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz + _mDx + _mDy,
                         sStyle.c_str());
            _graph->Face(mglPoint(vResults[0], vResults[1], vResults[2]) + _mDx,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy + _mDx,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDz + _mDx,
                         mglPoint(vResults[0], vResults[1], vResults[2]) + _mDy + _mDz + _mDx,
                         sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 7) == "sphere(")
        {
            if (nFunctions < 4)
                continue;
            _graph->Sphere(mglPoint(vResults[0], vResults[1], vResults[2]), vResults[3], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 5) == "cone(")
        {
            if (nFunctions < 7)
                continue;
            if (nFunctions >= 8)
                _graph->Cone(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], vResults[7], sStyle.c_str());
            else
                _graph->Cone(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], 0.0, sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 6) == "conev(")
        {
            if (nFunctions < 7)
                continue;
            if (nFunctions >= 8)
                _graph->Cone(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]), vResults[6], vResults[7], sStyle.c_str());
            else
                _graph->Cone(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]), vResults[6], 0.0, sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 5) == "drop(")
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
        else if (sCurrentDrawingFunction.substr(0, 7) == "circle(")
        {
            if (nFunctions < 4)
                continue;
            _graph->Circle(mglPoint(vResults[0], vResults[1], vResults[2]), vResults[3], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 4) == "arc(")
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
        else if (sCurrentDrawingFunction.substr(0, 5) == "arcv(")
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
        else if (sCurrentDrawingFunction.substr(0, 6) == "point(")
        {
            if (nFunctions < 3)
                continue;
            _graph->Mark(mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 6) == "curve(")
        {
            if (nFunctions < 12)
                continue;
            _graph->Curve(mglPoint(vResults[0], vResults[1], vResults[2]),
                          mglPoint(vResults[3], vResults[4], vResults[5]),
                          mglPoint(vResults[6], vResults[7], vResults[8]),
                          mglPoint(vResults[9], vResults[10], vResults[11]), sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 8) == "ellipse(")
        {
            if (nFunctions < 7)
                continue;
            _graph->Ellipse(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 9) == "ellipsev(")
        {
            if (nFunctions < 7)
                continue;
            _graph->Ellipse(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]) + mglPoint(vResults[0], vResults[1], vResults[2]), vResults[6], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 5) == "text(")
        {
            if (!sTextString.length())
            {
                sTextString = sStyle;
                sStyle = "k";
            }
            if (nFunctions >= 6)
                _graph->Puts(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), sTextString.c_str(), sStyle.c_str());
            else if (nFunctions >= 3)
                _graph->Puts(mglPoint(vResults[0], vResults[1], vResults[2]), sTextString.c_str(), sStyle.c_str());
            else
                continue;
        }
        else if (sCurrentDrawingFunction.substr(0, 8) == "polygon(")
        {
            if (nFunctions < 7 || vResults[6] < 3)
                continue;
            _graph->Polygon(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), (int)vResults[6], sStyle.c_str());
        }
        else if (sCurrentDrawingFunction.substr(0, 9) == "polygonv(")
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
/// \param vType vector<short>&
/// \param nStyle int&
/// \param nLegends size_t&
/// \param nFunctions int
/// \param nPlotCompose size_t
/// \param nPlotComposeSize size_t
/// \return void
///
/////////////////////////////////////////////////
void Plot::createStd3dPlot(vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    const short TYPE_FUNC = 1;
    const short TYPE_DATA = -1;

    mglData _mData[3] = {mglData(_pInfo.nSamples), mglData(_pInfo.nSamples), mglData(_pInfo.nSamples)};
    mglData _mData2[3] = {mglData(_pInfo.nSamples), mglData(_pInfo.nSamples), mglData(_pInfo.nSamples)};
    int nLabels = 0;
    int nPos[2] = {0, 0};
    int nTypeCounter[2] = {0, 0};
    unsigned int nCurrentType;
    while (sLabels.find(';', nPos[0]) != string::npos)
    {
        nPos[0] = sLabels.find(';', nPos[0]) + 1;
        nLabels++;
        if (nPos[0] >= (int)sLabels.length())
            break;
    }

    nPos[0] = 0;
    if (nLabels > _pData.getLayers())
    {
        for (int i = 0; i < _pData.getLayers(); i++)
        {
            for (int j = 0; j < 2; j++)
            {
                if (nLabels == 1)
                    break;
                nPos[0] = sLabels.find(';', nPos[0]);
                sLabels = sLabels.substr(0, nPos[0] - 1) + ", " + sLabels.substr(nPos[0] + 2);
                nLabels--;
            }
            nPos[0] = sLabels.find(';', nPos[0]) + 1;
            nLabels--;
        }
        nPos[0] = 0;
    }
    if (_option.isDeveloperMode())
        cerr << LineBreak("|-> DEBUG: sLabels = " + sLabels, _option) << endl;


    if (_pData.getCutBox())
        _graph->SetCutBox(CalcCutBox(_pData.getRotateAngle(1), 0, _pData.getCoords(), true), CalcCutBox(_pData.getRotateAngle(1), 1, _pData.getCoords(), true));


    for (unsigned int nType = 0; nType < vType.size(); nType++)
    {
        nCurrentType = nType;
        if (vType[nType] == TYPE_FUNC)
        {
            StripSpaces(sLabels);
            for (int i = 0; i < 3; i++)
            {
                _mData[i].Create(_pInfo.nSamples);
                _mData2[i].Create(_pInfo.nSamples);
            }

            for (long int i = 0; i < _pInfo.nSamples; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    _mData[j].a[i] = _pData.getData(i, j, nTypeCounter[0]);
                }
            }

            if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
                _mData[1] = fmod(_mData[1], 2.0 * M_PI);
            if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                _mData[2] = fmod(_mData[2], 2.0 * M_PI);
            if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RT)
                _mData[2] = fmod(_mData[2], 1.0 * M_PI);
            if (_pData.getRegion() && vType.size() > nType + 3 && vType[nType + 3] == TYPE_DATA)
            {
                _mData2[0] = v_mDataPlots[nTypeCounter[1]][0];
                _mData2[1] = v_mDataPlots[nTypeCounter[1]][1];
                _mData2[2] = v_mDataPlots[nTypeCounter[1]][2];
                if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
                    _mData2[1] = fmod(_mData2[1], 2.0 * M_PI);
                if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                    _mData2[2] = fmod(_mData2[2], 2.0 * M_PI);
                if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RT)
                    _mData2[2] = fmod(_mData2[2], 1.0 * M_PI);
                nTypeCounter[1]++;
                nType += 3;
            }
            else if (_pData.getRegion() && vType.size() > nType + 3 && vType[nType + 3] == TYPE_FUNC)
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        _mData2[j].a[i] = _pData.getData(i, j, nTypeCounter[0] + 1);
                    }
                }
                if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
                    _mData2[1] = fmod(_mData2[1], 2.0 * M_PI);
                if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                    _mData2[2] = fmod(_mData2[2], 2.0 * M_PI);
                if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RT)
                    _mData2[2] = fmod(_mData2[2], 1.0 * M_PI);

                nTypeCounter[0]++;
                nType++;
            }
            else
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        _mData2[j].a[i] = 0.0;
                    }
                }
            }
        }
        else
        {
            if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
                v_mDataPlots[nTypeCounter[1]][1] = fmod(v_mDataPlots[nTypeCounter[1]][1], 2.0 * M_PI);
            if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                v_mDataPlots[nTypeCounter[1]][2] = fmod(v_mDataPlots[nTypeCounter[1]][2], 2.0 * M_PI);
            if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RT)
                v_mDataPlots[nTypeCounter[1]][2] = fmod(v_mDataPlots[nTypeCounter[1]][2], 1.0 * M_PI);

            StripSpaces(sDataLabels);
            if (_pData.getxError() || _pData.getyError())
            {
                for (long int i = 0; i < v_mDataPlots[nTypeCounter[1]][0].nx; i++)
                {
                    if (v_mDataPlots[nTypeCounter[1]][0].a[i] < _pInfo.dRanges[XCOORD][0] || v_mDataPlots[nTypeCounter[1]][0].a[i] > _pInfo.dRanges[XCOORD][1]
                            || v_mDataPlots[nTypeCounter[1]][1].a[i] < _pInfo.dRanges[YCOORD][0] || v_mDataPlots[nTypeCounter[1]][1].a[i] > _pInfo.dRanges[YCOORD][1]
                            || v_mDataPlots[nTypeCounter[1]][1].a[i] < _pInfo.dRanges[ZCOORD][0] || v_mDataPlots[nTypeCounter[1]][1].a[i] > _pInfo.dRanges[ZCOORD][1])
                    {
                        v_mDataPlots[nTypeCounter[1]][0].a[i] = NAN;
                        v_mDataPlots[nTypeCounter[1]][1].a[i] = NAN;
                        v_mDataPlots[nTypeCounter[1]][2].a[i] = NAN;
                    }
                }
            }

            _mData[0] = v_mDataPlots[nTypeCounter[1]][0];
            _mData[1] = v_mDataPlots[nTypeCounter[1]][1];
            _mData[2] = v_mDataPlots[nTypeCounter[1]][2];

            for (int j = 0; j < 3; j++)
            {
                for (int i = 0; i < getNN(_mData[j]); i++)
                {
                    if (_mData[j].a[i] < _pInfo.dRanges[j][0] || _mData[j].a[i] > _pInfo.dRanges[j][1])
                        _mData[j].a[i] = NAN;
                }
            }

            if (_pData.getRegion() && vType.size() > nType + 1 && vType[nType + 1] == TYPE_DATA)
            {
                _mData2[0] = v_mDataPlots[nTypeCounter[1] + 1][0];
                _mData2[1] = v_mDataPlots[nTypeCounter[1] + 1][1];
                _mData2[2] = v_mDataPlots[nTypeCounter[1] + 1][2];
                if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
                    _mData2[1] = fmod(_mData2[1], 2.0 * M_PI);
                if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                    _mData2[2] = fmod(_mData2[2], 2.0 * M_PI);
                if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RT)
                    _mData2[2] = fmod(_mData2[2], 1.0 * M_PI);
                nTypeCounter[1]++;
                nType++;
            }
            else if (_pData.getRegion() && vType.size() > nType + 1 && vType[nType + 1] == TYPE_FUNC)
            {
                for (int j = 0; j < 3; j++)
                    _mData2[j].Create(_pInfo.nSamples);
                for (long int i = 0; i < _pInfo.nSamples; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        _mData2[j].a[i] = _pData.getData(i, j, nTypeCounter[0]);
                    }
                }
                if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
                    _mData2[1] = fmod(_mData2[1], 2.0 * M_PI);
                if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                    _mData2[2] = fmod(_mData2[2], 2.0 * M_PI);
                if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RT)
                    _mData2[2] = fmod(_mData2[2], 1.0 * M_PI);
                nTypeCounter[0]++;
                nType += 3;
            }
            else if (_pData.getRegion())
            {
                for (int j = 0; j < 3; j++)
                    _mData2[j].Create(getNN(_mData[0]));
                for (long int i = 0; i < getNN(_mData[0]); i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        _mData2[j].a[i] = 0.0;
                    }
                }
            }

            if (_pData.getxError() && _pData.getyError())
            {
                _mData2[0] = v_mDataPlots[nTypeCounter[1]][3];
                _mData2[1] = v_mDataPlots[nTypeCounter[1]][4];
                _mData2[2] = v_mDataPlots[nTypeCounter[1]][5];
            }
        }
        if (!plotstd3d(_mData, _mData2, vType[nCurrentType]))
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            clearData();
            return;
        }
        if (vType[nCurrentType] == TYPE_FUNC)
        {
            if (_pData.getRegion() && vType.size() > nCurrentType + 1)
            {
                for (int k = 0; k < 2; k++)
                {
                    nPos[0] = sLabels.find(';');
                    sConvLegends = sLabels.substr(0, nPos[0]) + " -nq";
                    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sConvLegends, sDummy, true);
                    sConvLegends = "\"" + sConvLegends + "\"";
                    for (unsigned int l = 0; l < sConvLegends.length(); l++)
                    {
                        if (sConvLegends[l] == '(')
                            l += getMatchingParenthesis(sConvLegends.substr(l));
                        if (sConvLegends[l] == ',')
                        {
                            sConvLegends = "\"[" + sConvLegends.substr(1, sConvLegends.length() - 2) + "]\"";
                            break;
                        }
                    }
                    sLabels = sLabels.substr(nPos[0] + 1);
                    if (sConvLegends != "\"\"")
                    {
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), _pInfo.sLineStyles[nStyle].c_str());
                        nLegends++;
                    }

                    if (nStyle == _pInfo.nStyleMax - 1)
                        nStyle = 0;
                    else
                        nStyle++;
                }
            }
            else
            {
                nPos[0] = sLabels.find(';');
                sConvLegends = sLabels.substr(0, nPos[0]) + " -nq";
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sConvLegends, sDummy, true);
                sConvLegends = "\"" + sConvLegends + "\"";
                sLabels = sLabels.substr(nPos[0] + 1);
                if (sConvLegends != "\"\"")
                {
                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), getLegendStyle(_pInfo.sLineStyles[nStyle]).c_str());
                    nLegends++;
                }
                if (nStyle == _pInfo.nStyleMax - 1)
                    nStyle = 0;
                else
                    nStyle++;
            }
            nTypeCounter[0]++;
            nType += 2;
        }
        else
        {
            nPos[1] = sDataLabels.find(';');
            sConvLegends = sDataLabels.substr(0, nPos[1]);
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sConvLegends, sDummy, true);
            sDataLabels = sDataLabels.substr(nPos[1] + 1);
            if (sConvLegends != "\"\"")
            {
                nLegends++;
                if (!_pData.getxError() && !_pData.getyError())
                {
                    if ((_pData.getInterpolate() && v_mDataPlots[nTypeCounter[1]][0].nx >= _pInfo.nSamples) || _pData.getBars())
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), getLegendStyle(_pInfo.sLineStyles[nStyle]).c_str());
                    else if (_pData.getConnectPoints() || (_pData.getInterpolate() && v_mDataPlots[nTypeCounter[1]][0].nx >= 0.9 * _pInfo.nSamples))
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), getLegendStyle(_pInfo.sConPointStyles[nStyle]).c_str());
                    else if (_pData.getStepplot())
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), getLegendStyle(_pInfo.sLineStyles[nStyle]).c_str());
                    else
                        _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), getLegendStyle(_pInfo.sPointStyles[nStyle]).c_str());
                }
                else
                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1, sConvLegends.length() - 2))).c_str(), getLegendStyle(_pInfo.sPointStyles[nStyle]).c_str());
            }
            if (nStyle == _pInfo.nStyleMax - 1)
                nStyle = 0;
            else
                nStyle++;

            nTypeCounter[1]++;
        }
    }


    if (_pData.getCutBox())
        _graph->SetCutBox(mglPoint(0), mglPoint(0));
    if (!((_pData.getMarks() || _pData.getCrust()) && _pInfo.sCommand.substr(0, 6) == "plot3d") && nLegends && !_pData.getSchematic() && nPlotCompose + 1 == nPlotComposeSize)
    {
        if (_pData.getRotateAngle() || _pData.getRotateAngle(1))
            _graph->Legend(1.35, 1.2);
        else
            _graph->Legend(_pData.getLegendPosition());
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
    if (!_pData.getBoxplot() && !_pData.getyError() && !_pData.getxError() && !_pData.getBars() && !_pData.getHBars() && !_pData.getStepplot())
    {
        for (int i = 0; i < 3; i++)
        {
            _mData[i] = duplicatePoints(_mData[i]);
            _mData2[i] = duplicatePoints(_mData2[i]);
        }

        if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::SPHERICAL_PT)
        {
            removeNegativeValues(_mData[2]);
            removeNegativeValues(_mData2[2]);
        }
    }

    if (nType == 1)
    {
        if (!_pData.getArea() && !_pData.getRegion())
            _graph->Plot(_mData[0], _mData[1], _mData[2], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
        else if (_pData.getRegion() && getNN(_mData2[0]))
        {
            if (*_pInfo.nStyle == _pInfo.nStyleMax - 1)
                _graph->Region(_mData[0], _mData[1], _mData[2], _mData2[0], _mData2[1], _mData2[2], ("a{" + _pData.getColors().substr(*_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(0, 1) + "7}").c_str());
            else
                _graph->Region(_mData[0], _mData[1], _mData[2], _mData2[0], _mData2[1], _mData2[2], ("a{" + _pData.getColors().substr(*_pInfo.nStyle, 1) + "7}{" + _pData.getColors().substr(*_pInfo.nStyle + 1, 1) + "7}").c_str());
            _graph->Plot(_mData[0], _mData[1], _mData[2], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
            if (*_pInfo.nStyle == _pInfo.nStyleMax - 1)
                _graph->Plot(_mData2[0], _mData2[1], _mData2[2], ("a" + _pInfo.sLineStyles[0]).c_str());
            else
                _graph->Plot(_mData2[0], _mData2[1], _mData2[2], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle + 1]).c_str());
        }
        else
            _graph->Area(_mData[0], _mData[1], _mData[2], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
    }
    else
    {
        if (!_pData.getxError() && !_pData.getyError())
        {
            // --> Interpolate-Schalter. Siehe weiter oben fuer Details <--
            if (_pData.getInterpolate() && countValidElements(_mData[0]) >= (size_t)_pInfo.nSamples)
            {
                if (!_pData.getArea() && !_pData.getBars() && !_pData.getRegion())
                    _graph->Plot(_mData[0], _mData[1], _mData[2], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
                else if (_pData.getBars() && !_pData.getArea() && !_pData.getRegion())
                    _graph->Bars(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle] + "^").c_str());
                    else
                    _graph->Area(_mData[0], _mData[1], _mData[2], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
            }
            else if (_pData.getConnectPoints() || (_pData.getInterpolate() && countValidElements(_mData[0]) >= 0.9 * _pInfo.nSamples))
            {
                if (!_pData.getArea() && !_pData.getBars())
                    _graph->Plot(_mData[0], _mData[1], _mData[2], ("a" + _pInfo.sConPointStyles[*_pInfo.nStyle]).c_str());
                else if (_pData.getBars() && !_pData.getArea())
                    _graph->Bars(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle] + "^").c_str());
                else
                    _graph->Area(_mData[0], _mData[1], _mData[2], ("a" + _pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
            }
            else
            {
                if (!_pData.getArea() && !_pData.getMarks() && !_pData.getBars() && !_pData.getStepplot() && !_pData.getCrust())
                    _graph->Plot(_mData[0], _mData[1], _mData[2], ("a" + _pInfo.sPointStyles[*_pInfo.nStyle]).c_str());
                else if (_pData.getMarks() && !_pData.getCrust() && !_pData.getBars() && !_pData.getArea() && !_pData.getStepplot())
                    _graph->Dots(_mData[0], _mData[1], _mData[2], _pData.getColorScheme(toString(_pData.getMarks())).c_str());
                else if (_pData.getCrust() && !_pData.getMarks() && !_pData.getBars() && !_pData.getArea() && !_pData.getStepplot())
                    _graph->Crust(_mData[0], _mData[1], _mData[2], _pData.getColorScheme().c_str());
                else if (_pData.getBars() && !_pData.getArea() && !_pData.getMarks() && !_pData.getStepplot() && !_pData.getCrust())
                    _graph->Bars(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle] + "^").c_str());
                else if (!_pData.getBars() && !_pData.getArea() && !_pData.getMarks() && _pData.getStepplot() && !_pData.getCrust())
                    _graph->Step(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
                else
                    _graph->Stem(_mData[0], _mData[1], _mData[2], _pInfo.sConPointStyles[*_pInfo.nStyle].c_str());
            }
        }
        else if (_pData.getxError() || _pData.getyError())
        {
            for (int m = 0; m < _mData[0].nx; m++)
            {
                _graph->Error(mglPoint(_mData[0].a[m], _mData[1].a[m], _mData[2].a[m]), mglPoint(_mData2[0].a[m], _mData2[1].a[m], _mData2[2].a[m]), _pInfo.sPointStyles[*_pInfo.nStyle].c_str());
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
/// \brief This member function evaluates the
/// current plot parameter string if it contains
/// actual string expressions.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::evaluatePlotParamString()
{
    string sDummy;
    if (_pInfo.sPlotParams.find("??") != string::npos)
        _pInfo.sPlotParams = promptForUserInput(_pInfo.sPlotParams);

    if (!_functions.call(_pInfo.sPlotParams))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, _pInfo.sPlotParams, SyntaxError::invalid_position);

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(_pInfo.sPlotParams) && _pInfo.sPlotParams.find('=') != string::npos)
    {
        unsigned int nPos = 0;

        if (NumeReKernel::getInstance()->getStringParser().containsStringVars(_pInfo.sPlotParams))
            NumeReKernel::getInstance()->getStringParser().getStringValues(_pInfo.sPlotParams);

        // Search for all option values in the current string
        while ((nPos = _pInfo.sPlotParams.find('=', nPos)) != string::npos)
        {
            nPos++;

            if (nPos >= _pInfo.sPlotParams.length())
                break;

            if (isInQuotes(_pInfo.sPlotParams, nPos))
                continue;

            // Find the actual value (jump over white spaces)
            while (_pInfo.sPlotParams[nPos] == ' ')
                nPos++;

            string sOptionValue = extractStringToken(_pInfo.sPlotParams, nPos);

            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sOptionValue))
            {
                size_t nLength = sOptionValue.length();
                string sParsedString;

                // Remove surrounding parentheses
                if (sOptionValue.front() == '(' && sOptionValue.back() == ')')
                {
                    sOptionValue.erase(0, 1);
                    sOptionValue.pop_back();
                }

                // Parse the current option value and consider
                // vector braces
                while (sOptionValue.length())
                {
                    string sCurrentString = getNextArgument(sOptionValue, true);
                    bool bVector = sCurrentString.find('{') != string::npos;

                    if (containsStrings(sCurrentString))
                        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCurrentString, sDummy, true);

                    if (bVector && sCurrentString.find('{') == string::npos)
                        sCurrentString = "{" + sCurrentString + "}";

                    if (sParsedString.length())
                        sParsedString += "," + sCurrentString;
                    else
                        sParsedString = sCurrentString;
                }

                if (_pInfo.sPlotParams[nPos] == '(' && sParsedString.front() != '(')
                    sParsedString = "(" + sParsedString + ")";

                // Replace the parsed string
                _pInfo.sPlotParams.replace(nPos, nLength, sParsedString);
            }
        }
    }
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
    if (!_pData.getFileName().length() && !nPlotCompose)
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
    else if (_pData.getFileName().length() && !nPlotCompose)
        bOutputDesired = true;

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(_pData.getFileName()) && !nPlotCompose)
    {
        string sTemp = _pData.getFileName();
        string sTemp_2 = "";
        string sExtension = sTemp.substr(sTemp.find('.'));
        sTemp = sTemp.substr(0, sTemp.find('.'));

        if (sExtension[sExtension.length() - 1] == '"')
        {
            sTemp += "\"";
            sExtension = sExtension.substr(0, sExtension.length() - 1);
        }

        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sTemp, sTemp_2, true);
        _pData.setFileName(sTemp.substr(1, sTemp.length() - 2) + sExtension);
    }

    if (_pData.getAnimateSamples() && _pData.getFileName().substr(_pData.getFileName().rfind('.')) != ".gif" && !nPlotCompose)
        _pData.setFileName(_pData.getFileName().substr(0, _pData.getFileName().length() - 4) + ".gif");
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

        if (_pData.getDrawPoints())
        {
            if (_pData.getPointStyles()[2 * i] != ' ')
                _pInfo.sLineStyles[i] += _pData.getPointStyles()[2 * i];

            _pInfo.sLineStyles[i] += _pData.getPointStyles()[2 * i + 1];
        }

        if (_pData.getyError() || _pData.getxError())
        {
            if (_pData.getPointStyles()[2 * i] != ' ')
                _pInfo.sPointStyles[i] += _pData.getPointStyles()[2 * i];

            _pInfo.sPointStyles[i] += _pData.getPointStyles()[2 * i + 1];
        }
        else
        {
            if (_pData.getPointStyles()[2 * i] != ' ')
                _pInfo.sPointStyles[i] += " " + _pData.getPointStyles().substr(2 * i, 1) + _pData.getPointStyles().substr(2 * i + 1, 1);
            else
                _pInfo.sPointStyles[i] += " " + _pData.getPointStyles().substr(2 * i + 1, 1);
        }

        if (_pData.getPointStyles()[2 * i] != ' ')
            _pInfo.sConPointStyles[i] += _pData.getLineStyles().substr(i, 1) + _pData.getPointStyles().substr(2 * i, 1) + _pData.getPointStyles().substr(2 * i + 1, 1);
        else
            _pInfo.sConPointStyles[i] += _pData.getLineStyles().substr(i, 1) + _pData.getPointStyles().substr(2 * i + 1, 1);

        _pInfo.sContStyles[i] += _pData.getLineStyles()[i];
        _pInfo.sLineStyles[i] += _pData.getLineSizes()[i];
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
/// \param nLegends size_t&
/// \param sCmd string&
/// \param nMultiplots[2] size_t
/// \param nSubPlots size_t&
/// \param nSubPlotMap size_t&
/// \return void
///
/////////////////////////////////////////////////
void Plot::evaluateSubplot(size_t& nLegends, string& sCmd, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap)
{
    if (nLegends && !_pData.getSchematic())
    {
        _graph->Legend(_pData.getLegendPosition());
        _graph->ClearLegend();
    }

    string sSubPlotIDX = sCmd.substr(findCommand(sCmd).nPos + 7);
    if (sSubPlotIDX.find("-set") != string::npos || sSubPlotIDX.find("--") != string::npos)
    {
        if (sSubPlotIDX.find("-set") != string::npos)
            sSubPlotIDX.erase(sSubPlotIDX.find("-set"));
        else
            sSubPlotIDX.erase(sSubPlotIDX.find("--"));
    }
    StripSpaces(sSubPlotIDX);
    if (findParameter(sCmd, "cols", '=') || findParameter(sCmd, "lines", '='))
    {
        unsigned int nMultiLines = 1, nMultiCols = 1;

        if (findParameter(sCmd, "cols", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "cols", '=') + 4));
            nMultiCols = (unsigned int)intCast(_parser.Eval());
        }
        if (findParameter(sCmd, "lines", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "lines", '=') + 5));
            nMultiLines = (unsigned int)intCast(_parser.Eval());
        }
        if (sSubPlotIDX.length())
        {
            if (!_functions.call(sSubPlotIDX))
                throw SyntaxError(SyntaxError::FUNCTION_ERROR, sSubPlotIDX, SyntaxError::invalid_position);
            if (_data.containsTablesOrClusters(sSubPlotIDX))
            {
                getDataElements(sSubPlotIDX, _parser, _data, _option);
            }
            _parser.SetExpr(sSubPlotIDX);
            int nRes = 0;
            value_type* v = _parser.Eval(nRes);
            if (nRes == 1)
            {
                if (intCast(v[0]) < 1)
                    v[0] = 1;
                if ((unsigned int)intCast(v[0]) - 1 >= nMultiplots[0]*nMultiplots[1])
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, (unsigned int)(intCast(v[0]) - 1), nMultiCols, nMultiLines))
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                _graph->MultiPlot(nMultiplots[0], nMultiplots[1], (unsigned int)intCast(v[0]) - 1, nMultiCols, nMultiLines);
            }   // cols, lines
            else
            {
                if ((unsigned int)(intCast(v[1]) - 1 + (intCast(v[0]) - 1)*nMultiplots[1]) >= nMultiplots[0]*nMultiplots[1])
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, (unsigned int)((intCast(v[1]) - 1) + (intCast(v[0]) - 1)*nMultiplots[0]), nMultiCols, nMultiLines))
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                _graph->MultiPlot(nMultiplots[0], nMultiplots[1], (unsigned int)((intCast(v[1]) - 1) + (intCast(v[0]) - 1)*nMultiplots[0]), nMultiCols, nMultiLines);
            }
        }
        else
        {
            if (nSubPlots >= nMultiplots[0]*nMultiplots[1])
                throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
            int nPlotPos = 1;
            for (unsigned int nSub = 0; nSub < nMultiplots[0]*nMultiplots[1]; nSub++)
            {
                if (nPlotPos & nSubPlotMap)
                    nPlotPos <<= 1;
                else
                {
                    if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, nSub, nMultiCols, nMultiLines))
                        throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                    _graph->MultiPlot(nMultiplots[0], nMultiplots[1], nSub, nMultiCols, nMultiLines);
                    break;
                }
                if (nSub == nMultiplots[0]*nMultiplots[1] - 1)
                {
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                }
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
            {
                getDataElements(sSubPlotIDX, _parser, _data, _option);
            }
            _parser.SetExpr(sSubPlotIDX);
            int nRes = 0;
            value_type* v = _parser.Eval(nRes);
            if (nRes == 1)
            {
                if (intCast(v[0]) < 1)
                    v[0] = 1;
                if ((unsigned int)intCast(v[0]) - 1 >= nMultiplots[0]*nMultiplots[1])
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                if ((unsigned int)intCast(v[0]) != 1)
                    nRes <<= (unsigned int)(intCast(v[0]) - 1);
                if (nRes & nSubPlotMap)
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                nSubPlotMap |= nRes;
                _graph->SubPlot(nMultiplots[0], nMultiplots[1], (unsigned int)intCast(v[0]) - 1);
            }
            else
            {
                if ((unsigned int)(intCast(v[1]) - 1 + (intCast(v[0]) - 1)*nMultiplots[0]) >= nMultiplots[0]*nMultiplots[1])
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                nRes = 1;
                if ((unsigned int)(intCast(v[1]) + (intCast(v[0]) - 1)*nMultiplots[0]) != 1)
                    nRes <<= (unsigned int)((intCast(v[1]) - 1) + (intCast(v[0]) - 1) * nMultiplots[0]);
                if (nRes & nSubPlotMap)
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                nSubPlotMap |= nRes;
                _graph->SubPlot(nMultiplots[0], nMultiplots[1], (unsigned int)((intCast(v[1]) - 1) + (intCast(v[0]) - 1)*nMultiplots[0]));
            }
        }
        else
        {
            if (nSubPlots >= nMultiplots[0]*nMultiplots[1])
                throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
            int nPlotPos = 1;
            for (unsigned int nSub = 0; nSub < nMultiplots[0]*nMultiplots[1]; nSub++)
            {
                if (nPlotPos & nSubPlotMap)
                    nPlotPos <<= 1;
                else
                {
                    nSubPlotMap |= nPlotPos;
                    _graph->SubPlot(nMultiplots[0], nMultiplots[1], nSub);
                    break;
                }
                if (nSub == nMultiplots[0]*nMultiplots[1] - 1)
                {
                    throw SyntaxError(SyntaxError::INVALID_SUBPLOT_INDEX, "", SyntaxError::invalid_position);
                }
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
    if (!_pData.getSilentMode() && _option.systemPrints())
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
        if (_pInfo.nSamples > 51)
        {
            if (_pData.getHighRes() == 2 && _pInfo.nSamples > 151)
                _pInfo.nSamples = 151;
            else if ((_pData.getHighRes() == 1 || !_option.isDraftMode()) && _pInfo.nSamples > 151)
                _pInfo.nSamples = 151;
            else
                _pInfo.nSamples = 51;
        }
        if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "surf")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_SURFACE")) + "-");
        else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "mesh")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_MESHGRID")) + "-");
        else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "cont")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_CONTOUR")) + "-");
        else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "dens")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_DENSITY")) + "-");
        else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "grad")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_GRADIENT")) + "-");
    }
    else if (_pInfo.sCommand.substr(0, 6) == "vect3d" || _pInfo.sCommand == "vector3d")
    {
        _pInfo.b3DVect = true;
        if (_pInfo.nSamples > 11)
            _pInfo.nSamples = 11;
        if (!_pData.getSilentMode() && _option.systemPrints())
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_VECTOR")) + "-");
        if (_pData.getPipe() || _pData.getFlow())
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
        if (!_pData.getSilentMode() && _option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_VECTOR")) + "-");
    }
    else if (_pInfo.sCommand.substr(0, 4) == "mesh"
             || _pInfo.sCommand.substr(0, 4) == "surf"
             || _pInfo.sCommand.substr(0, 4) == "cont"
             || _pInfo.sCommand.substr(0, 4) == "grad"
             || _pInfo.sCommand.substr(0, 4) == "dens")
    {
        _pInfo.b2D = true;
        if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "surf")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_SURFACE")) + "-");
        else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "mesh")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_MESHGRID")) + "-");
        else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "cont")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_CONTOUR")) + "-");
        else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "dens")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_DENSITY")) + "-");
        else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand.substr(0, 4) == "grad")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_GRADIENT")) + "-");
    }
    else if (_pInfo.sCommand == "plot3d")
    {
        if (!_pData.getSilentMode() && _option.systemPrints())
            NumeReKernel::printPreFmt("3D-");
    }
    else if (_pInfo.sCommand == "draw")
    {
        _pInfo.bDraw = true;
    }
    else if (_pInfo.sCommand == "draw3d")
    {
        _pInfo.bDraw3D = true;
        if (!_pData.getSilentMode() && _option.systemPrints())
            NumeReKernel::printPreFmt("3D-");
    }
    else if (_pInfo.sCommand == "implot")
        _pInfo.b2D = true;

    if (!_pData.getSilentMode() && _option.systemPrints() && !bAnimateVar && !(_pInfo.bDraw3D || _pInfo.bDraw) && _pInfo.sCommand != "implot")
        NumeReKernel::printPreFmt("Plot ... ");
    else if (!_pData.getSilentMode() && _option.systemPrints() && _pInfo.sCommand == "implot")
        NumeReKernel::printPreFmt("Image plot ... ");
    else if (!_pData.getSilentMode() && _option.systemPrints() && !bAnimateVar)
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_DRAWING")) + " ... ");
    else if (!_pData.getSilentMode() && _option.systemPrints())
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_ANIMATION")) + " ... \n");

}


/////////////////////////////////////////////////
/// \brief This member function separates the
/// functions from the data plots and returns a
/// vector containing the definitions of the data
/// plots.
///
/// \param vType vector<short>&
/// \param sAxisBinds string&
/// \param sDataAxisBinds string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> Plot::evaluateDataPlots(vector<short>& vType, string& sAxisBinds, string& sDataAxisBinds)
{
    const short TYPE_DATA = -1;
    const short TYPE_FUNC = 1;

    if (!_data.containsTablesOrClusters(sFunc))
        return std::vector<std::string>();

    string sFuncTemp = sFunc;
    string sToken = "";

    while (sFuncTemp.length())
    {
        sToken = getNextArgument(sFuncTemp, true);
        StripSpaces(sToken);

        if (_data.containsTablesOrClusters(sToken))
        {
            if (_data.containsTablesOrClusters(sToken.substr(0, sToken.find_first_of("({") + 1))
                && !_data.isTable(sToken.substr(0, sToken.find_first_of("({")))
                && !_data.isCluster(sToken.substr(0, sToken.find_first_of("({"))))
                throw SyntaxError(SyntaxError::DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING, "", SyntaxError::invalid_position, sToken);

            string sSubstr = sToken.substr(getMatchingParenthesis(sToken.substr(sToken.find_first_of("({"))) + sToken.find_first_of("({") + 1);

            if (sSubstr[sSubstr.find_first_not_of(' ')] != '"' && sSubstr[sSubstr.find_first_not_of(' ')] != '#')
                throw SyntaxError(SyntaxError::DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING, "", SyntaxError::invalid_position, sToken);
        }
    }

    // --> Zerlegen von sFunc in Funktionenteile und Datenplots <--
    sFuncTemp = sFunc;
    sFunc.clear();

    std::vector<std::string> vDataPlots;

    while (sFuncTemp.length())
    {
        sToken = getNextArgument(sFuncTemp, true);

        if (_data.containsTablesOrClusters(sToken))
        {
            vDataPlots.push_back(sToken);
            vType.push_back(TYPE_DATA);
            sDataAxisBinds += _pData.getAxisbind((sDataAxisBinds.length() + sAxisBinds.length()) / 2);
        }
        else
        {
            sFunc += "," + sToken;
            _functions.call(sToken);
            StripSpaces(sToken);

            if (sToken.front() == '{')
                sToken.front() = ' ';

            if (sToken.back() == '}')
                sToken.back() = ' ';

            while (getNextArgument(sToken, true).length())
                vType.push_back(TYPE_FUNC);

            sAxisBinds += _pData.getAxisbind((sDataAxisBinds.length() + sAxisBinds.length()) / 2);
        }
    }

    sFunc.erase(0, 1);

    if (!_pInfo.b2DVect && !_pInfo.b3DVect && !_pInfo.b3D && !_pInfo.bDraw && !_pInfo.bDraw3D)
    {
        // --> Allozieren wir Speicher fuer die gesamten Datenarrays <--
        v_mDataPlots.resize(vDataPlots.size());

        // --> Nutzen wir den Vorteil von ";" als Trennzeichen <--
        size_t nPos = 0;

        // --> Trennen wir die Legenden der Datensaetze von den eigentlichen Datensaetzen ab <--
        for (size_t i = 0; i < vDataPlots.size(); i++)
        {
            if (vDataPlots[i].find_first_of("#\"") != string::npos)
                nPos = vDataPlots[i].find_first_of("#\"");

            sDataLabels += vDataPlots[i].substr(nPos) + ";";
            vDataPlots[i].erase(nPos);
        }

        createDataLegends();
    }

    return vDataPlots;
}


/////////////////////////////////////////////////
/// \brief Creates the internal mglData objects
/// and fills them with the data values from the
/// MemoryManager class instance in the kernel.
///
/// \param vDataPlots const std::vector<std::string>&
/// \param sDataAxisBinds const std::string&
/// \param dDataRanges[3][2] double
/// \param dSecDataRanges[2][2] double
/// \return void
///
/////////////////////////////////////////////////
void Plot::extractDataValues(const std::vector<std::string>& vDataPlots, const std::string& sDataAxisBinds, double dDataRanges[3][2], double dSecDataRanges[2][2])
{
    // Now extract the index informations of each referred
    // data object and copy its contents to the prepared
    // mglData objects
    for (size_t i = 0; i < vDataPlots.size(); i++)
    {
        size_t nParPos = vDataPlots[i].find_first_of("({");
        DataAccessParser _accessParser;

        if (nParPos == string::npos)
            throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, vDataPlots[i] , vDataPlots[i]);

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
                throw SyntaxError(SyntaxError::INVALID_INDEX, sDataTable, SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());
        }
        else
        {
            if (_idx.row.front() >= _data.getLines(sDataTable, false)
                || (_idx.col.front() >= _data.getCols(sDataTable) && _pInfo.sCommand != "plot3d"))
                throw SyntaxError(SyntaxError::INVALID_INDEX, sDataTable, SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());
        }

        /* --> Bestimmen wir die "Dimension" des zu plottenden Datensatzes. Dabei ist es auch
         *     von Bedeutung, ob Fehlerbalken anzuzeigen sind <--
         */

        switch (_idx.col.numberOfNodes())
        {
            case 1:
                if (!_pData.getxError() && !_pData.getyError())
                    v_mDataPlots[i].resize(2);
                else if (_pData.getxError() && _pData.getyError())
                    v_mDataPlots[i].resize(4);
                else if (_pData.getxError() || _pData.getyError())
                    v_mDataPlots[i].resize(3);

                break;
            case 2:
                if (_pInfo.sCommand == "plot" && !_pData.getxError() && !_pData.getyError())
                    v_mDataPlots[i].resize(2);
                else if (_pInfo.sCommand == "plot" && _pData.getxError() && _pData.getyError())
                    v_mDataPlots[i].resize(4);
                else if (_pInfo.sCommand == "plot" && (_pData.getxError() || _pData.getyError()))
                    v_mDataPlots[i].resize(3);
                else if (_pInfo.sCommand == "plot3d" && !_pData.getxError() && !_pData.getyError())
                    v_mDataPlots[i].resize(3);
                else if (_pInfo.sCommand == "plot3d" && _pData.getxError() && _pData.getyError())
                    v_mDataPlots[i].resize(6);
                else if (_pInfo.b2D)
                    v_mDataPlots[i].resize(3);

                if ((_pData.getBoxplot() || _pData.getBars() || _pData.getHBars()) && openEnd)
                    v_mDataPlots[i].resize(_data.getCols(sDataTable, false) + _pData.getBoxplot() - _idx.col.front());

                break;
            default:
                if (!(_pData.getBoxplot() || _pData.getBars() || _pData.getHBars() || _pInfo.b2D))
                {
                    // This section was commented to allow arrays of curves
                    // to be displayed in the plot
                    /*if (nColumns > 6)
                        nDataDim[i] = 6;
                    else
                        */v_mDataPlots[i].resize(_idx.col.numberOfNodes());

                    /*if (_idx.col.size() > 6)
                    {
                        vector<long long int> vJ = _idx.col.getVector();
                        _idx.col = vector<long long int>(vJ.begin() + 6, vJ.end());
                    }*/
                }
                else if (_pInfo.b2D)
                    v_mDataPlots[i].resize(3);
                else
                {
                    if (_pData.getBars() || _pData.getHBars())
                        v_mDataPlots[i].resize(_idx.col.numberOfNodes());
                    else
                        v_mDataPlots[i].resize(_idx.col.numberOfNodes() + 1);
                }
        }

        if (!v_mDataPlots[i].size())
            throw SyntaxError(SyntaxError::PLOT_ERROR, sDataTable, SyntaxError::invalid_position);

        // Prepare and fill the mglData objects for the current
        // data access depending on whether single indices or an
        // index vector is used
        getValuesFromData(_accessParser, i, sDataAxisBinds, dDataRanges, dSecDataRanges, openEnd);
    }
}


/////////////////////////////////////////////////
/// \brief Fills the created mglData objects with
/// the data values from the MemoryManager.
///
/// \param _accessParser DataAccessParser&
/// \param i size_t
/// \param sDataAxisBinds const std::string&
/// \param dDataRanges[3][2] double
/// \param dSecDataRanges[2][2] double
/// \param openEnd bool
/// \return void
///
/////////////////////////////////////////////////
void Plot::getValuesFromData(DataAccessParser& _accessParser, size_t i, const std::string& sDataAxisBinds, double dDataRanges[3][2], double dSecDataRanges[2][2], bool openEnd)
{
    Indices& _idx = _accessParser.getIndices();

    if (_idx.col.numberOfNodes() > 2 && !_pInfo.b2D)
    {
        // A vector index was used. Insert a column index
        // if the current plot is a boxplot
        if (_pData.getBoxplot())
        {
            vector<int> vJ = _idx.col.getVector();
            vJ.insert(vJ.begin(), -1);
            _idx.col = vJ;
        }

        // Fill the mglData objects with the data from the
        // referenced data object
        for (size_t q = 0; q < v_mDataPlots[i].size(); q++)
        {
            v_mDataPlots[i][q].Create(_idx.row.size());

            if (_idx.col[q] == -1)
            {
                for (size_t n = 0; n < _idx.row.size(); n++)
                    v_mDataPlots[i][q].a[n] = _idx.row[n] + 1;
            }
            else
            {
                for (size_t n = 0; n < _idx.row.size(); n++)
                {
                    v_mDataPlots[i][q].a[n] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[n], _idx.col[q], _accessParser.isCluster()));

                    if (!_accessParser.isCluster() && _pInfo.sCommand == "plot3d" && q < 3 && !(_data.getLines(_accessParser.getDataObject(), true) - _data.getAppendedZeroes(_idx.col[q], _accessParser.getDataObject()) - _idx.row[0]))
                        v_mDataPlots[i][q].a[n] = 0.0;
                }
            }
        }

        // Calculated the data ranges
        for (int l = 0; l < (int)_idx.row.size(); l++)
        {
            calculateDataRanges(sDataAxisBinds, dDataRanges, dSecDataRanges, i, l, _idx.row, _idx.col.numberOfNodes());
        }
    }
    else
    {
        // Create the mglData object for the current data
        // access using single indices
        for (size_t q = 0; q < v_mDataPlots[i].size(); q++)
        {
            if (_pInfo.b2D && q == 2)///TEMPORARY
                v_mDataPlots[i][q].Create(_idx.row.size(), _idx.row.size());
            else
                v_mDataPlots[i][q].Create(_idx.row.size());
        }

        long long int nDataCols = _data.getCols(_accessParser.getDataObject());

        size_t nNum = 0;

        // Needed for data grids
        if (!_accessParser.isCluster())
            nNum = _data.num(_accessParser.getDataObject(), _idx.row, VectorIndex(_idx.col.front()+1)).real();

        // Fill the mglData objects using the single indices
        // and the referenced data object
        for (size_t l = 0; l < _idx.row.size(); l++)
        {
            // Fill the objects depending on the selected columns
            if (_idx.col.numberOfNodes() == 1 && (_accessParser.isCluster() || nDataCols >= _idx.col.front()))
            {
                // These are only y-values. Add index values
                // as x column
                v_mDataPlots[i][0].a[l] = _idx.row[l] + 1;

                v_mDataPlots[i][1].a[l] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col.front(), _accessParser.isCluster()));

                // Create pseudo error values in this case
                if (_pData.getxError() && _pData.getyError())
                    v_mDataPlots[i][3].a[l] = 0.0;
                else if (_pData.getyError() || _pData.getxError())
                    v_mDataPlots[i][2].a[l] = 0.0;
            }
            else if (!_accessParser.isCluster() && nDataCols < _idx.col.front() && _pInfo.sCommand != "plot3d")
            {
                // Catch column index errors
                throw SyntaxError(SyntaxError::TOO_FEW_COLS, _accessParser.getDataObject(), SyntaxError::invalid_position);
            }
            else if (!_accessParser.isCluster() && v_mDataPlots[i].size() == 2 && _idx.col.numberOfNodes() == 2)
            {
                // These are xy data values
                if ((nDataCols >= _idx.col.front()+1 && (_idx.col.isOrdered() || openEnd))
                    || (nDataCols >= _idx.col.back()+1 && !_idx.col.isOrdered()))
                {
                    // These data columns are in correct order
                    v_mDataPlots[i][0].a[l] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col.front(), _accessParser.isCluster()));

                    if (!openEnd)
                        v_mDataPlots[i][1].a[l] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col.last(), _accessParser.isCluster()));
                    else
                        v_mDataPlots[i][1].a[l] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col.front() + 1, _accessParser.isCluster()));
                }
                else if ((nDataCols >= _idx.col.front() && (_idx.col.isOrdered() || openEnd))
                         || (nDataCols >= _idx.col.back() && !_idx.col.isOrdered()))
                {
                    // These data columns miss a x-column
                    v_mDataPlots[i][0].a[l] = _idx.row[l] + 1;

                    if (_idx.col.isOrdered() || openEnd)
                        v_mDataPlots[i][1].a[l] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col.front(), _accessParser.isCluster()));
                    else if (!_idx.col.isOrdered())
                        v_mDataPlots[i][1].a[l] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col.last(), _accessParser.isCluster()));
                    else
                        v_mDataPlots[i][1].a[l] = NAN;
                }
                else
                {
                    // Catch column index errors
                    throw SyntaxError(SyntaxError::TOO_FEW_COLS, _accessParser.getDataObject(), SyntaxError::invalid_position);
                }
            }
            else if (!_accessParser.isCluster() && v_mDataPlots[i].size() >= 3 && (_idx.col.numberOfNodes() == 2 || _pInfo.b2D))
            {
                // These are xyz data values
                // These columns are in correct order
                for (size_t q = 0; q < v_mDataPlots[i].size() - (_pData.getBoxplot()); q++)
                {
                    // Use the data either as data grid or as xyz
                    // tuple values
                    if (_pInfo.b2D && q == 2)
                    {
                        for (size_t k = 0; k < _idx.row.size(); k++)
                        {
                            if (nNum <= k)
                            {
                                v_mDataPlots[i][q].a[l + (_idx.row.size())*k] = NAN;
                                continue;
                            }

                            v_mDataPlots[i][q].a[l + (_idx.row.size())*k] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col[q+k], _accessParser.isCluster()));
                        }
                    }
                    else
                    {
                        // --> Vorwaerts zaehlen <--
                        if (!q && (_pData.getBoxplot()))
                            v_mDataPlots[i][q] = _idx.row[l]+1;

                        if (nDataCols > _idx.col[q])
                            v_mDataPlots[i][q + (_pData.getBoxplot())].a[l] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col[q], _accessParser.isCluster()));
                        else if (q == 2 && _pInfo.sCommand == "plot3d"
                                 && !(_data.getLines(_accessParser.getDataObject(), true) - _data.getAppendedZeroes(_idx.col[q], _accessParser.getDataObject()) - _idx.row.front()))
                            v_mDataPlots[i][q].a[l] = 0.0;
                        else
                            v_mDataPlots[i][q + (_pData.getBoxplot())].a[l] = NAN;
                    }
                }
            }
            else if (!_accessParser.isCluster())
            {
                // These are columns in arbitrary order and an
                // arbitrary number of columns
                if ((_pData.getBoxplot() || _pData.getBars() || _pData.getHBars()) && !_pInfo.b2D)
                {
                    bool bBoxplot = _pData.getBoxplot() && !_pData.getBars() && !_pData.getHBars();

                    for (size_t k = 0; k < min(v_mDataPlots[i].size(), _idx.col.size()); k++)
                    {
                        if (bBoxplot && !k)
                            v_mDataPlots[i][0] = _idx.row[l] + 1;

                        if (nDataCols > _idx.col[k])
                            v_mDataPlots[i][k + bBoxplot].a[l] = validize(getDataFromObject(_accessParser.getDataObject(), _idx.row[l], _idx.col[k], _accessParser.isCluster()));
                        else if (_pInfo.sCommand == "plot3d" && k < 3
                                 && !(_data.getLines(_accessParser.getDataObject(), true) - _data.getAppendedZeroes(_idx.col[k], _accessParser.getDataObject()) - _idx.row.front()))
                            v_mDataPlots[i][k].a[l] = 0.0;
                        else
                            v_mDataPlots[i][k + bBoxplot].a[l] = NAN;
                    }
                }
            }

            // --> Berechnen der DataRanges <--
            calculateDataRanges(sDataAxisBinds, dDataRanges, dSecDataRanges, i, l, _idx.row);
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
    // --> Ersetzen von "data()" bzw. "cache()" durch die Spaltentitel <--
    size_t n_dpos = 0;

    // Examine all data labels
    while (sDataLabels.find(';', n_dpos) != string::npos)
    {
        // Extraxt the current label
        string sTemp = sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos) - n_dpos);

        // Try to find a data object in the current label
        if (_data.containsTables(sTemp.substr(1, sTemp.length()-2))
                && (sTemp.find(',') != string::npos || sTemp.substr(sTemp.find('('), 2) == "()")
                && sTemp.find(')') != string::npos)
        {
            // Ensure that the referenced data object contains valid data
            if (_data.containsTablesOrClusters(sTemp.substr(1, sTemp.length()-2)) && !_data.isValid())
            {
                throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sDataLabels, SyntaxError::invalid_position);
            }

            // Strip all spaces and extract the table name
            StripSpaces(sTemp);
            string sTableName = sTemp.substr(0, sTemp.find('('));

            // Clear quotation marks and semicolons at the beginning of the table name
            while (sTableName[0] == ';' || sTableName[0] == '"')
                sTableName.erase(0, 1);

            // Ensure that the parentheses are matching each other
            if (getMatchingParenthesis(sTemp.substr(sTemp.find('('))) == string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sTemp, sTemp.find('('));

            // Extract the argument of the data object
            string sArgs = sTemp.substr(sTemp.find('('), getMatchingParenthesis(sTemp.substr(sTemp.find('('))) + 1);

            // Update the data dimension variables
            _data.updateDimensionVariables(sTableName);

            // Expand empty parentheses
            if (sArgs == "()")
                sArgs = ":,:";
            else
                sArgs = sArgs.substr(1, sArgs.length()-2);

            string sArg_1 = sArgs;
            string sArg_2 = "<<empty>>";
            string sArg_3 = "<<empty>>";

            // Get the second dimension of the argument parentheses
            getNextArgument(sArg_1, true);
            StripSpaces(sArg_1);

            // If the second dimension contains one or more colons,
            // extract the individual columns here
            if (sArg_1 == ":")
            {
                sArg_1 = "";
                sArg_2 = "";
            }
            else if (sArg_1.find(':') != string::npos)
            {
                auto indices = getAllIndices(sArg_1);
                sArg_1 = indices[0];

                if (indices.size() > 1)
                {
                    sArg_2 = indices[1];

                    if (indices.size() > 2)
                        sArg_3 = indices[2];
                }
                else if (_data.containsTablesOrClusters(sArg_1))
                {
                    getDataElements(sArg_1, _parser, _data, _option);
                }
            }

            // Strip surrounding whitespaces
            StripSpaces(sArg_1);

            // Handle special cases
            if (!sArg_1.length() && !sArg_2.length() && sArg_3 == "<<empty>>")
            {
                sArg_1 = "1";
            }
            else if (!sArg_1.length())
            {
                n_dpos = sDataLabels.find(';', n_dpos) + 1;
                continue;
            }

            // Parse the single arguments to extract the corresponding
            // headline elements
            if (sArg_2 == "<<empty>>" && sArg_3 == "<<empty>>" && _pInfo.sCommand != "plot3d")
            {

                // Only one index or an index vector
                sTemp = "\"" + constructDataLegendElement(sArg_1, sTableName) + "\"";
            }
            else if (sArg_2.length())
            {
                // First and second index value available
                if (_pInfo.sCommand != "plot3d")
                {
                    // Standard plot
                    if (!(_pData.getyError() || _pData.getxError()) && sArg_3 == "<<empty>>")
                    {
                        // Handle here barcharts
                        if (_pData.getBars() || _pData.getHBars())
                        {
                            double dArg_1, dArg_2;
                            _parser.SetExpr(sArg_1);
                            dArg_1 = _parser.Eval().real();
                            _parser.SetExpr(sArg_2);
                            dArg_2 = _parser.Eval().real();
                            sTemp = "\"";

                            // Don't use the first one
                            for (int i = intCast(dArg_1); i < intCast(dArg_2); i++)
                            {
                                sTemp += _data.getTopHeadLineElement(i, sTableName) + "\n";
                            }
                            sTemp.pop_back();
                            sTemp += "\"";
                        }
                        else
                        {
                            sTemp = "\"" + constructDataLegendElement(sArg_2, sTableName) + " vs. " + constructDataLegendElement(sArg_1, sTableName) + "\"";
                        }
                    }
                    else if (sArg_3 != "<<empty>>")
                    {
                        sTemp = "\"" + constructDataLegendElement(sArg_2, sTableName) + " vs. " + constructDataLegendElement(sArg_1, sTableName) + "\"";
                    }
                    else
                    {
                        double dArg_1, dArg_2;
                        _parser.SetExpr(sArg_1);
                        dArg_1 = _parser.Eval().real();
                        _parser.SetExpr(sArg_2);
                        dArg_2 = _parser.Eval().real();

                        if (dArg_1 < dArg_2)
                            sTemp = "\"" + _data.getTopHeadLineElement((int)dArg_1, sTableName) + " vs. " + _data.getTopHeadLineElement((int)dArg_1 - 1, sTableName) + "\"";
                        else
                            sTemp = "\"" + _data.getTopHeadLineElement((int)dArg_2 - 1, sTableName) + " vs. " + _data.getTopHeadLineElement((int)dArg_2, sTableName) + "\"";
                    }
                }
                else if (sArg_3 == "<<empty>>" || !sArg_3.length())
                {
                    // three-dimensional plot
                    double dArg_1, dArg_2;
                    _parser.SetExpr(sArg_1);
                    dArg_1 = _parser.Eval().real();
                    _parser.SetExpr(sArg_2);
                    dArg_2 = _parser.Eval().real();

                    if (dArg_1 < dArg_2)
                        sTemp = "\"" + _data.getTopHeadLineElement((int)dArg_1 - 1, sTableName) + ", "
                            + _data.getTopHeadLineElement((int)dArg_2 - 2, sTableName) + ", "
                            + _data.getTopHeadLineElement((int)dArg_2 - 1, sTableName) + "\"";
                    else
                        sTemp = "\"" + _data.getTopHeadLineElement((int)dArg_2 - 1, sTableName) + ", "
                            + _data.getTopHeadLineElement((int)dArg_1 - 2, sTableName) + ", "
                            + _data.getTopHeadLineElement((int)dArg_1 - 1, sTableName) + "\"";
                }
                else if (sArg_3.length())
                {
                    // Three dimensional plot
                    sTemp = "\"" + constructDataLegendElement(sArg_1, sTableName) + ", "
                            + constructDataLegendElement(sArg_2, sTableName) + ", "
                            + constructDataLegendElement(sArg_3, sTableName) + "\"";
                }
            }
            else if (!sArg_2.length())
            {
                // second index open end
                if (_pInfo.sCommand != "plot3d")
                {
                    // Handle here barcharts
                    if (_pData.getBars() || _pData.getHBars())
                    {
                        double dArg_1;
                        _parser.SetExpr(sArg_1);
                        dArg_1 = _parser.Eval().real();

                        sTemp = "\"";

                        // Don't use the first one
                        for (int i = intCast(dArg_1); i < _data.getCols(sTableName, false); i++)
                        {
                            sTemp += _data.getTopHeadLineElement(i, sTableName) + "\n";
                        }
                        sTemp.pop_back();
                        sTemp += "\"";
                    }
                    else
                    {
                        _parser.SetExpr(sArg_1);
                        sTemp = "\"" + _data.getTopHeadLineElement(intCast(_parser.Eval()), sTableName) + " vs. " + _data.getTopHeadLineElement(intCast(_parser.Eval()) - 1, sTableName) + "\"";
                    }
                }
                else if (sArg_3 == "<<empty>>" || !sArg_3.length())
                {
                    _parser.SetExpr(sArg_1);
                    sTemp = "\"" + _data.getTopHeadLineElement(intCast(_parser.Eval()) - 1, sTableName) + ", "
                            + _data.getTopHeadLineElement(intCast(_parser.Eval()), sTableName) + ", "
                            + _data.getTopHeadLineElement(intCast(_parser.Eval()) + 1, sTableName) + "\"";
                }
                else if (sArg_3.length())
                {
                    _parser.SetExpr(sArg_1);
                    sTemp = "\"" + _data.getTopHeadLineElement(intCast(_parser.Eval()) - 1, sTableName) + ", " + _data.getTopHeadLineElement(intCast(_parser.Eval()), sTableName) + ", ";
                    sTemp += constructDataLegendElement(sArg_3, sTableName) + "\"";
                }
            }
            else
            {
                n_dpos = sDataLabels.find(';', n_dpos) + 1;
                continue;
            }

            // Replace the data expression with the parsed headlines
            sDataLabels = sDataLabels.substr(0, n_dpos) + sTemp + sDataLabels.substr(sDataLabels.find(';', n_dpos));

        }
        n_dpos = sDataLabels.find(';', n_dpos) + 1;

    }

    // Prepend backslashes before opening and closing
    // braces
    for (size_t i = 0; i < sDataLabels.size(); i++)
    {
        if (sDataLabels[i] == '{' || sDataLabels[i] == '}')
        {
            sDataLabels.insert(i, 1, '\\');
            i++;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function is used to
/// construct special legend elements.
///
/// \param sColumnIndices string&
/// \param sTableName const string&
/// \return string
///
/////////////////////////////////////////////////
string Plot::constructDataLegendElement(string& sColumnIndices, const string& sTableName)
{
    if (NumeReKernel::getInstance()->getMemoryManager().containsTablesOrClusters(sColumnIndices))
        getDataElements(sColumnIndices, _parser, NumeReKernel::getInstance()->getMemoryManager(), NumeReKernel::getInstance()->getSettings());

    value_type* v = nullptr;
    int nResults = 0;

    // Set the expression and evaluate it
    _parser.SetExpr(sColumnIndices);
    v = _parser.Eval(nResults);

    // If only one value, simply return the corresponding head line
    if (nResults == 1)
        return _data.getTopHeadLineElement(intCast(v[0]) - 1, sTableName);

    string sFirst = "[";
    string sLast = "]";
    char cSep = ',';
    int nStart = 0;

    // Barcharts and boxplots will need other legend strings
    if (_pInfo.sCommand == "plot" && (_pData.getBars() || _pData.getBoxplot() || _pData.getHBars()))
    {
        sFirst.clear();
        sLast.clear();
        cSep = '\n';
        nStart  = 1;
    }

    string sLegend = sFirst;

    // combine the legend strings
    for (int i = nStart; i < nResults; i++)
    {
        sLegend += _data.getTopHeadLineElement(intCast(v[i]) - 1, sTableName);
        if (i + 1 < nResults)
            sLegend += cSep;
    }
    return sLegend + sLast;
}


/////////////////////////////////////////////////
/// \brief This member function is used to
/// calculate the data ranges. It is called
/// during filling the data objects.
///
/// \param sDataAxisBinds const string&
/// \param dDataRanges[3][2] double
/// \param dSecDataRanges[2][2] double
/// \param i int
/// \param l int
/// \param _vLine const VectorIndex&
/// \param numberofColNodes size_t
/// \return void
///
/////////////////////////////////////////////////
void Plot::calculateDataRanges(const string& sDataAxisBinds, double dDataRanges[3][2], double dSecDataRanges[2][2], int i, int l, const VectorIndex& _vLine, size_t numberofColNodes)
{
    // Calculate the ranges for all three spatial directions
    for (int q = 0; q < 3; q++)
    {
        // Ignore the third direction, if it is not used
        // for the current plot
        if (q == 2 && v_mDataPlots[i].size() < 3)
        {
            dDataRanges[2][0] = 0;
            dDataRanges[2][1] = 0;
            break;
        }

        // Use the first row of the first data set to
        // initialize the data ranges
        if (l == 0 && i == 0)
        {
            // Ignore data ranges, which are resulting from
            // values, which are already excluded by the user
            if (q && _pData.getRangeSetting(q - 1))
            {
                if (_pData.getRanges(q - 1, 0) > v_mDataPlots[i][q - 1].a[l] || _pData.getRanges(q - 1, 1) < v_mDataPlots[i][q - 1].a[l])
                {
                    if (_pInfo.sCommand != "plot")
                    {
                        dDataRanges[q][0] = NAN;
                        dDataRanges[q][1] = NAN;
                    }
                    else
                    {
                        if (q < 2)
                        {
                            dSecDataRanges[q][0] = NAN;
                            dSecDataRanges[q][1] = NAN;
                        }
                        dDataRanges[q][0] = NAN;
                        dDataRanges[q][1] = NAN;
                    }
                    continue;
                }
            }
            // Use the first value as a base value
            if (_pInfo.sCommand != "plot" || q >= 2)
            {
                // Straightforward solution for three-dimensional plots
                dDataRanges[q][0] = v_mDataPlots[i][q].a[l];
                dDataRanges[q][1] = v_mDataPlots[i][q].a[l];
            }
            else if ((_pData.getBoxplot() || _pData.getBars() || (numberofColNodes > 2 && !_pData.getHBars())) && q == 1)
            {
                // Handle boxplots (not only the first value but the whole table)
                for (size_t dim = 1; dim < v_mDataPlots[i].size(); dim++)
                {
                    if (sDataAxisBinds[2 * i] == 'l' || sDataAxisBinds[2 * i] == 'b')
                    {
                        dDataRanges[1][0] = v_mDataPlots[i][dim].a[l];
                        dDataRanges[1][1] = v_mDataPlots[i][dim].a[l];
                    }
                    else
                    {
                        dSecDataRanges[1][0] = v_mDataPlots[i][dim].a[l];
                        dSecDataRanges[1][1] = v_mDataPlots[i][dim].a[l];
                    }
                }
            }
            else if (_pData.getHBars())
            {
                // Handle bars (not only the first value but the whole table)
                for (size_t dim = 0; dim < v_mDataPlots[i].size(); dim++)
                {
                    if (sDataAxisBinds[2 * i + !q] == 'l' || sDataAxisBinds[2 * i + !q] == 'b')
                    {
                        dDataRanges[0 + !dim][0] = v_mDataPlots[i][dim].a[l];
                        dDataRanges[0 + !dim][1] = v_mDataPlots[i][dim].a[l];
                    }
                    else
                    {
                        dSecDataRanges[0 + !dim][0] = v_mDataPlots[i][dim].a[l];
                        dSecDataRanges[0 + !dim][1] = v_mDataPlots[i][dim].a[l];
                    }
                }
            }
            else
            {
                // Handle usual plots with the additional axes
                if (sDataAxisBinds[2 * i + !q] == 'l' || sDataAxisBinds[2 * i + !q] == 'b')
                {
                    dDataRanges[q][0] = v_mDataPlots[i][q].a[l];
                    dDataRanges[q][1] = v_mDataPlots[i][q].a[l];
                    dSecDataRanges[q][0] = NAN;
                    dSecDataRanges[q][1] = NAN;
                }
                else
                {
                    dSecDataRanges[q][0] = v_mDataPlots[i][q].a[l];
                    dSecDataRanges[q][1] = v_mDataPlots[i][q].a[l];
                    dDataRanges[q][0] = NAN;
                    dDataRanges[q][1] = NAN;
                }
            }
        }
        else
        {
            // All other rows are used to compare them with the already setted value
            //
            // Three dimensional plot:
            if (_pInfo.b2D && q == 2 && _pInfo.sCommand != "implot")
            {
                // Ignore data ranges, which are resulting from
                // values, which are already excluded by the user
                if (_pData.getRangeSetting())
                {
                    if (_pData.getRanges(0, 0) > v_mDataPlots[i][0].a[l] || _pData.getRanges(0, 1) < v_mDataPlots[i][0].a[l])
                        continue;
                }
                if (_pData.getRangeSetting(1))
                {
                    if (_pData.getRanges(1, 0) > v_mDataPlots[i][1].a[l] || _pData.getRanges(1, 1) < v_mDataPlots[i][1].a[l])
                        continue;
                }

                // Calculate the data ranges for three dimensional plots
                for (size_t k = 0; k < _vLine.size(); k++)
                {
                    if (dDataRanges[q][0] > v_mDataPlots[i][q].a[l + _vLine.size() * k] || isnan(dDataRanges[q][0]))
                        dDataRanges[q][0] = v_mDataPlots[i][q].a[l + _vLine.size() * k];

                    if (dDataRanges[q][1] < v_mDataPlots[i][q].a[l + _vLine.size() * k] || isnan(dDataRanges[q][1]))
                        dDataRanges[q][1] = v_mDataPlots[i][q].a[l + _vLine.size() * k];
                }
            }
            else if (q == 2 && _pInfo.sCommand == "implot")
            {
                dDataRanges[q][0] = 0;
                dDataRanges[q][1] = 255;
            }
            else
            {
                // Ignore data ranges, which are resulting from
                // values, which are already excluded by the user
                if (q && _pData.getRangeSetting())
                {
                    if (_pData.getRanges(0, 0) > v_mDataPlots[i][0].a[l] || _pData.getRanges(0, 1) < v_mDataPlots[i][0].a[l])
                        continue;
                }

                // Calculate the data ranges for all other plot types
                if (_pInfo.sCommand != "plot" || q >= 2)
                {
                    if (dDataRanges[q][0] > v_mDataPlots[i][q].a[l] || isnan(dDataRanges[q][0]))
                        dDataRanges[q][0] = v_mDataPlots[i][q].a[l];
                    if (dDataRanges[q][1] < v_mDataPlots[i][q].a[l] || isnan(dDataRanges[q][1]))
                        dDataRanges[q][1] = v_mDataPlots[i][q].a[l];
                }
                else if ((_pData.getBoxplot() || _pData.getBars() || (numberofColNodes > 2 && !_pData.getHBars())) && q == 1)
                {
                    // Handle boxplots (evalute the row of the whole table)
                    for (size_t dim = 1; dim < v_mDataPlots[i].size(); dim++)
                    {
                        if (sDataAxisBinds[2 * i] == 'l' || sDataAxisBinds[2 * i] == 'b')
                        {
                            if (dDataRanges[1][0] > v_mDataPlots[i][dim].a[l] || isnan(dDataRanges[1][0]))
                                dDataRanges[1][0] = v_mDataPlots[i][dim].a[l];
                            if (dDataRanges[1][1] < v_mDataPlots[i][dim].a[l] || isnan(dDataRanges[1][1]))
                                dDataRanges[1][1] = v_mDataPlots[i][dim].a[l];
                        }
                        else
                        {
                            if (dSecDataRanges[1][0] > v_mDataPlots[i][dim].a[l] || isnan(dSecDataRanges[1][0]))
                                dSecDataRanges[1][0] = v_mDataPlots[i][dim].a[l];
                            if (dSecDataRanges[1][1] < v_mDataPlots[i][dim].a[l] || isnan(dSecDataRanges[1][1]))
                                dSecDataRanges[1][1] = v_mDataPlots[i][dim].a[l];
                        }
                    }
                }
                else if (_pData.getHBars())
                {
                    // Handle barplots (evalute the row of the whole table)
                    for (size_t dim = 0; dim < v_mDataPlots[i].size(); dim++)
                    {
                        if (sDataAxisBinds[2 * i + !q] == 'l' || sDataAxisBinds[2 * i + !q] == 'b')
                        {
                            if (dDataRanges[0 + !dim][0] > v_mDataPlots[i][dim].a[l] || isnan(dDataRanges[0 + !dim][0]))
                                dDataRanges[0 + !dim][0] = v_mDataPlots[i][dim].a[l];
                            if (dDataRanges[0 + !dim][1] < v_mDataPlots[i][dim].a[l] || isnan(dDataRanges[0 + !dim][1]))
                                dDataRanges[0 + !dim][1] = v_mDataPlots[i][dim].a[l];
                        }
                        else
                        {
                            if (dSecDataRanges[0 + !dim][0] > v_mDataPlots[i][dim].a[l] || isnan(dSecDataRanges[0 + !dim][0]))
                                dSecDataRanges[0 + !dim][0] = v_mDataPlots[i][dim].a[l];
                            if (dSecDataRanges[0 + !dim][1] < v_mDataPlots[i][dim].a[l] || isnan(dSecDataRanges[0 + !dim][1]))
                                dSecDataRanges[0 + !dim][1] = v_mDataPlots[i][dim].a[l];
                        }
                    }
                }
                else
                {
                    // Handle usual plots
                    if (sDataAxisBinds[2 * i + !q] == 'l' || sDataAxisBinds[2 * i + !q] == 'b')
                    {
                        if (dDataRanges[q][0] > v_mDataPlots[i][q].a[l] || isnan(dDataRanges[q][0]))
                            dDataRanges[q][0] = v_mDataPlots[i][q].a[l];
                        if (dDataRanges[q][1] < v_mDataPlots[i][q].a[l] || isnan(dDataRanges[q][1]))
                            dDataRanges[q][1] = v_mDataPlots[i][q].a[l];
                    }
                    else
                    {
                        if (dSecDataRanges[q][0] > v_mDataPlots[i][q].a[l] || isnan(dSecDataRanges[q][0]))
                            dSecDataRanges[q][0] = v_mDataPlots[i][q].a[l];
                        if (dSecDataRanges[q][1] < v_mDataPlots[i][q].a[l] || isnan(dSecDataRanges[q][1]))
                            dSecDataRanges[q][1] = v_mDataPlots[i][q].a[l];
                    }
                }
            }
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
/// \param nFunctions int
/// \return void
///
/////////////////////////////////////////////////
void Plot::prepareMemory(int nFunctions)
{
    if (!_pInfo.b2D && sFunc != "<<empty>>" && _pInfo.sCommand != "plot3d" && !_pInfo.b3D && !_pInfo.b3DVect && !_pInfo.b2DVect)
        _pData.setDim(_pInfo.nSamples, nFunctions);    // _pInfo.nSamples * nFunctions (Standardplot)
    else if (sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
        nFunctions % 3 ? _pData.setDim(_pInfo.nSamples, 3, nFunctions / 3 + 1) : _pData.setDim(_pInfo.nSamples, 3, nFunctions / 3); // _pInfo.nSamples * 3 * _pInfo.nSamples/3 (3D-Trajektorie)
    else if (sFunc != "<<empty>>" && _pInfo.b3D)
        _pData.setDim(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);    // _pInfo.nSamples * _pInfo.nSamples * _pInfo.nSamples (3D-Plot)
    else if (sFunc != "<<empty>>" && _pInfo.b3DVect)
        _pData.setDim(_pInfo.nSamples, _pInfo.nSamples, 3 * _pInfo.nSamples); // _pInfo.nSamples * _pInfo.nSamples * 3*_pInfo.nSamples (3D-Vektorplot)
    else if (sFunc != "<<empty>>" && _pInfo.b2DVect)
        _pData.setDim(_pInfo.nSamples, _pInfo.nSamples, 2 * _pInfo.nSamples); // _pInfo.nSamples * _pInfo.nSamples * 2*_pInfo.nSamples (2D-Vektorplot)
    else if (sFunc != "<<empty>>")
        _pData.setDim(_pInfo.nSamples, _pInfo.nSamples, nFunctions);  // _pInfo.nSamples * _pInfo.nSamples * nFunctions (2D-Plot)

    _mAxisVals[0].Create(_pInfo.nSamples);
    if (_pInfo.b2D || _pInfo.b3D)
    {
        _mAxisVals[1].Create(_pInfo.nSamples);
        if (_pInfo.b3D)
            _mAxisVals[2].Create(_pInfo.nSamples);
    }
}


/////////////////////////////////////////////////
/// \brief This member function separates the
/// legend strings from the actual functions.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::separateLegends()
{
    // Do only something, if the function has
    // a length and we're not in drawing mode
    if (sFunc.length() && !_pInfo.bDraw3D && !_pInfo.bDraw)
    {
        unsigned int n_pos = 0;
        unsigned int n_pos_2 = 0;

        // As long as quotation marks are part
        // of the function
        do
        {
            // Find the first of quotation mark
            // or variable to string parser
            if (sFunc.find('#') != string::npos && sFunc.find('#') < sFunc.find('"'))
                n_pos = sFunc.find('#');
            else
                n_pos = sFunc.find('"');

            // Find the next comma
            n_pos_2 = sFunc.find(',', n_pos);

            if (n_pos_2 == string::npos)
                n_pos_2 = sFunc.length();

            // Find the next comma, which is not part
            // of the string
            while (sFunc.find(',', n_pos_2 + 1) != string::npos && isInQuotes(sFunc, n_pos_2))
            {
                n_pos_2 = sFunc.find(',', n_pos_2 + 1);
            }

            // Use the remaining string, if the next comma
            // does not exist
            if (n_pos_2 == string::npos || isInQuotes(sFunc, n_pos_2))
                n_pos_2 = sFunc.length();

            // Separate labels and function
            sLabels += sFunc.substr(n_pos, n_pos_2 - n_pos) + ";";
            sFunc = sFunc.substr(0, n_pos) + (n_pos_2 < sFunc.length() ? sFunc.substr(n_pos_2) : "");
        }
        while (sFunc.find('"') != string::npos || sFunc.find('#') != string::npos);

        // Remove obsolete whitespaces
        StripSpaces(sLabels);
    }
}


/////////////////////////////////////////////////
/// \brief This member function applies the
/// default ranges to the plotting ranges.
///
/// \param dDataRanges[3][2] double
/// \param dSecDataRanges[2][2] double
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
void Plot::defaultRanges(double dDataRanges[3][2], double dSecDataRanges[2][2], size_t nPlotCompose, bool bNewSubPlot)
{
    if (!nPlotCompose || bNewSubPlot)
    {
        // --> Standard-Ranges zuweisen: wenn weniger als i+1 Ranges gegeben sind und Datenranges vorhanden sind, verwende die Datenranges <--
        for (int i = XCOORD; i <= ZCOORD; i++)
        {
            if (_pInfo.bDraw3D || _pInfo.bDraw)///Temporary
            {
                _pInfo.dRanges[i][0] = _pData.getRanges(i);
                _pInfo.dRanges[i][1] = _pData.getRanges(i, 1);
                continue;
            }
            if (v_mDataPlots.size() && (_pData.getGivenRanges() < i + 1 || !_pData.getRangeSetting(i)))
            {
                if ((isinf(dDataRanges[i][0]) || isnan(dDataRanges[i][0])) && (unsigned)i < _pInfo.nMaxPlotDim)
                {
                    clearData();
                    throw SyntaxError(SyntaxError::PLOTDATA_IS_NAN, "", SyntaxError::invalid_position);
                }
                else if (!(isinf(dDataRanges[i][0]) || isnan(dDataRanges[i][0])))
                    _pInfo.dRanges[i][0] = dDataRanges[i][0];
                else
                    _pInfo.dRanges[i][0] = -10.0;
                if (i < 2 && !isinf(dSecDataRanges[i][0]) && !isnan(dSecDataRanges[i][0]))
                    _pInfo.dSecAxisRanges[i][0] = dSecDataRanges[i][0];
                else if (i < 2)
                    _pInfo.dSecAxisRanges[i][0] = NAN;
            }
            else
            {
                _pInfo.dRanges[i][0] = _pData.getRanges(i);
                if (!i)//i < 2)
                    _pInfo.dSecAxisRanges[i][0] = NAN;
                else
                    _pInfo.dSecAxisRanges[i][0] = dSecDataRanges[i][0];
            }
            if (v_mDataPlots.size() && (_pData.getGivenRanges() < i + 1 || !_pData.getRangeSetting(i)))
            {
                if ((isinf(dDataRanges[i][1]) || isnan(dDataRanges[i][1])) && (unsigned)i < _pInfo.nMaxPlotDim)
                {
                    clearData();
                    throw SyntaxError(SyntaxError::PLOTDATA_IS_NAN, "", SyntaxError::invalid_position);
                }
                else if (!(isinf(dDataRanges[i][1]) || isnan(dDataRanges[i][1])))
                    _pInfo.dRanges[i][1] = dDataRanges[i][1];
                else
                    _pInfo.dRanges[i][1] = 10.0;
                if (i < 2 && !isinf(dSecDataRanges[i][1]) && !isnan(dSecDataRanges[i][1]))
                    _pInfo.dSecAxisRanges[i][1] = dSecDataRanges[i][1];
                else if (i < 2)
                    _pInfo.dSecAxisRanges[i][1] = NAN;
            }
            else
            {
                _pInfo.dRanges[i][1] = _pData.getRanges(i, 1);
                if (!i) //i < 2)
                    _pInfo.dSecAxisRanges[i][1] = NAN;
                else
                    _pInfo.dSecAxisRanges[i][1] = dSecDataRanges[i][1];
            }
            if (!isnan(_pData.getAddAxis(i).dMin))
            {
                _pInfo.dSecAxisRanges[i][0] = _pData.getAddAxis(i).dMin;
                _pInfo.dSecAxisRanges[i][1] = _pData.getAddAxis(i).dMax;
            }
        }

        // --> Spezialfall: Wenn nur eine Range gegeben ist, verwende im 3D-Fall diese fuer alle drei benoetigten Ranges <--
        if (_pData.getGivenRanges() == 1 && (_pInfo.b3D || _pInfo.b3DVect))
        {
            for (int i = 1; i < 3; i++)
            {
                _pInfo.dRanges[i][0] = _pInfo.dRanges[XCOORD][0];
                _pInfo.dRanges[i][1] = _pInfo.dRanges[XCOORD][1];
            }
        }
    }
    // --> Sonderkoordinatensaetze und dazu anzugleichende Ranges. Noch nicht korrekt implementiert <--
    if (_pData.getCoords() == PlotData::CARTESIAN)
    {
        /* --> Im Falle logarithmischer Plots muessen die Darstellungsintervalle angepasst werden. Falls
         *     die Intervalle zu Teilen im Negativen liegen, versuchen wir trotzdem etwas sinnvolles
         *     daraus zu machen. <--
         */
        if (_pData.getxLogscale())
        {
            if (_pInfo.dRanges[XCOORD][0] <= 0 && _pInfo.dRanges[XCOORD][1] > 0)
            {
                _pInfo.dRanges[XCOORD][0] = _pInfo.dRanges[XCOORD][1] / 1e3;
            }
            else if (_pInfo.dRanges[XCOORD][0] < 0 && _pInfo.dRanges[XCOORD][1] <= 0)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
        }

        if (_pData.getyLogscale())
        {
            if (_pInfo.dRanges[YCOORD][0] <= 0 && _pInfo.dRanges[YCOORD][1] > 0)
            {
                _pInfo.dRanges[YCOORD][0] = _pInfo.dRanges[YCOORD][1] / 1e3;
            }
            else if (_pInfo.dRanges[YCOORD][0] < 0 && _pInfo.dRanges[YCOORD][1] <= 0)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
        }

        if (_pData.getzLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            if (_pInfo.dRanges[ZCOORD][0] <= 0 && _pInfo.dRanges[ZCOORD][1] > 0)
            {
                _pInfo.dRanges[ZCOORD][0] = _pInfo.dRanges[ZCOORD][1] / 1e3;
            }
            else if (_pInfo.dRanges[ZCOORD][0] < 0 && _pInfo.dRanges[ZCOORD][1] <= 0)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
        }
    }
    else if (_pData.getCoords() != PlotData::CARTESIAN)
    {
        // --> Im Falle polarer oder sphaerischer Koordinaten muessen die Darstellungsintervalle angepasst werden <--
        if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::POLAR_RZ)
        {
            if (_pInfo.sCommand.find("3d") == string::npos && !_pInfo.b2DVect)
            {
                int nRCoord = ZCOORD;
                int nPhiCoord = XCOORD;
                if (_pData.getCoords() == PlotData::POLAR_RP)
                {
                    nRCoord = XCOORD;
                    nPhiCoord = YCOORD;
                }
                else if (_pData.getCoords() == PlotData::POLAR_RZ && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect))
                {
                    nRCoord = XCOORD;
                    nPhiCoord = ZCOORD;
                }
                else if (!(_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect))
                    nRCoord = YCOORD;
                if (!_pData.getRangeSetting(nRCoord))
                {
                    _pInfo.dRanges[nRCoord][0] = 0.0;
                }
                if (!_pData.getRangeSetting(nPhiCoord))
                {
                    _pInfo.dRanges[nPhiCoord][0] = 0.0;
                    _pInfo.dRanges[nPhiCoord][1] = 2 * M_PI;
                }
            }
            else
            {
                _pInfo.dRanges[XCOORD][0] = 0.0;
                if (!_pData.getRangeSetting(YCOORD))
                {
                    _pInfo.dRanges[YCOORD][0] = 0.0;
                    _pInfo.dRanges[YCOORD][1] = 2 * M_PI;
                }
            }
        }
        else if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RP || _pData.getCoords() == PlotData::SPHERICAL_RT)
        {
            if (_pInfo.sCommand.find("3d") == string::npos)
            {
                int nRCoord = ZCOORD;
                int nPhiCoord = XCOORD;
                int nThetaCoord = YCOORD;
                if (_pData.getCoords() == PlotData::SPHERICAL_RP)
                {
                    nRCoord = XCOORD;
                    nPhiCoord = YCOORD;
                    nThetaCoord = ZCOORD;
                }
                else if (_pData.getCoords() == PlotData::SPHERICAL_RT && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect))
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
                {
                    _pInfo.dRanges[nRCoord][0] = 0.0;
                }
                if (!_pData.getRangeSetting(nPhiCoord))
                {
                    _pInfo.dRanges[nPhiCoord][0] = 0.0;
                    _pInfo.dRanges[nPhiCoord][1] = 2 * M_PI;
                }
                if (!_pData.getRangeSetting(nThetaCoord))
                {
                    _pInfo.dRanges[nThetaCoord][0] = 0.0;
                    _pInfo.dRanges[nThetaCoord][1] = M_PI;
                }
            }
            else
            {
                _pInfo.dRanges[XCOORD][0] = 0.0;
                if (!_pData.getRangeSetting(YCOORD))
                {
                    _pInfo.dRanges[YCOORD][0] = 0.0;
                    _pInfo.dRanges[YCOORD][1] = 2 * M_PI;
                }
                if (!_pData.getRangeSetting(ZCOORD))
                {
                    _pInfo.dRanges[ZCOORD][0] = 0.0;
                    _pInfo.dRanges[ZCOORD][1] = M_PI;
                }
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// plotting data points from usual expressions.
///
/// \param vResults value_type*
/// \param dt_max double
/// \param t_animate int
/// \param nFunctions int
/// \return int
///
/////////////////////////////////////////////////
int Plot::fillData(value_type* vResults, double dt_max, int t_animate, int nFunctions)
{
    // --> Plot-Speicher mit den berechneten Daten fuellen <--
    if (!_pInfo.b2D && sFunc != "<<empty>>" && _pInfo.sCommand != "plot3d" && !_pInfo.b3D && !_pInfo.b3DVect && !_pInfo.b2DVect)
    {
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getxLogscale())
                    _defVars.vValue[XCOORD][0] = pow(10.0, log10(_pInfo.dRanges[XCOORD][0]) + (log10(_pInfo.dRanges[XCOORD][1]) - log10(_pInfo.dRanges[XCOORD][0])) / (double)(_pInfo.nSamples - 1) * (double)x);
                else
                    _defVars.vValue[XCOORD][0] += (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) / (double)(_pInfo.nSamples - 1);
            }
            _mAxisVals[0].a[x] = _defVars.vValue[XCOORD][0].real();
            vResults = _parser.Eval(nFunctions);
            for (int i = 0; i < nFunctions; i++)
            {
                if (isinf(vResults[i]) || isnan(vResults[i]))
                    _pData.setData(x, i, NAN);
                else
                    _pData.setData(x, i, vResults[i].real());
            }
        }
    }
    else if (sFunc != "<<empty>>" && _pInfo.b3D)
    {
        // --> Wie oben, aber 3D-Fall <--
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getxLogscale())
                    _defVars.vValue[XCOORD][0] = pow(10.0, log10(_pInfo.dRanges[XCOORD][0]) + (log10(_pInfo.dRanges[XCOORD][1]) - log10(_pInfo.dRanges[XCOORD][0])) / (double)(_pInfo.nSamples - 1) * (double)x);
                else
                    _defVars.vValue[XCOORD][0] += (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) / (double)(_pInfo.nSamples - 1);
            }
            _defVars.vValue[YCOORD][0] = _pInfo.dRanges[YCOORD][0];
            _mAxisVals[0].a[x] = _defVars.vValue[XCOORD][0].real();
            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (y != 0)
                {
                    if (_pData.getyLogscale())
                        _defVars.vValue[YCOORD][0] = pow(10.0, log10(_pInfo.dRanges[YCOORD][0]) + (log10(_pInfo.dRanges[YCOORD][1]) - log10(_pInfo.dRanges[YCOORD][0])) / (double)(_pInfo.nSamples - 1) * (double)y);
                    else
                        _defVars.vValue[YCOORD][0] += (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (double)(_pInfo.nSamples - 1);
                }
                _defVars.vValue[ZCOORD][0] = _pInfo.dRanges[ZCOORD][0];
                if (!x)
                    _mAxisVals[1].a[y] = _defVars.vValue[YCOORD][0].real();
                for (int z = 0; z < _pInfo.nSamples; z++)
                {
                    if (z != 0)
                    {
                        if (_pData.getyLogscale())
                            _defVars.vValue[ZCOORD][0] = pow(10.0, log10(_pInfo.dRanges[ZCOORD][0]) + (log10(_pInfo.dRanges[ZCOORD][1]) - log10(_pInfo.dRanges[ZCOORD][0])) / (double)(_pInfo.nSamples - 1) * (double)y);
                        else
                            _defVars.vValue[ZCOORD][0] += (_pInfo.dRanges[ZCOORD][1] - _pInfo.dRanges[ZCOORD][0]) / (double)(_pInfo.nSamples - 1);
                    }
                    if (!x && !y)
                        _mAxisVals[2].a[z] = _defVars.vValue[ZCOORD][0].real();
                    double dResult = _parser.Eval().real();
                    //vResults = &_parser.Eval();

                    if (isnan(dResult) || isinf(dResult))
                        _pData.setData(x, y, NAN, z);
                    else
                        _pData.setData(x, y, dResult, z);
                }
            }
        }

    }
    else if (sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
    {
        // --> Parametrische Kurven berechnen <--
        for (int k = 0; k < _pData.getLayers(); k++)
        {
            _defVars.vValue[XCOORD][0] = 0.0;
            _defVars.vValue[YCOORD][0] = 0.0;
            _defVars.vValue[ZCOORD][0] = 0.0;
            _defVars.vValue[TCOORD][0] = _pData.gettBoundary();
            vResults = _parser.Eval(nFunctions);
            _defVars.vValue[XCOORD][0] = vResults[3 * k];

            if (3 * k + 1 < nFunctions)
                _defVars.vValue[YCOORD][0] = vResults[3 * k + 1];

            if (3 * k + 2 < nFunctions)
                _defVars.vValue[ZCOORD][0] = vResults[3 + k + 2];

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
                        _defVars.vValue[TCOORD][0] += (dt_max - _pData.gettBoundary()) / dSamples;
                    }
                    else
                        _defVars.vValue[TCOORD][0] += (_pData.gettBoundary(1) - _pData.gettBoundary()) / (double)(_pInfo.nSamples - 1);

                    _defVars.vValue[XCOORD][0] = _pData.getData(t - 1, 0, k);
                    _defVars.vValue[YCOORD][0] = _pData.getData(t - 1, 1, k);
                    _defVars.vValue[ZCOORD][0] = _pData.getData(t - 1, 2, k);
                }

                // --> Wir werten alle Koordinatenfunktionen zugleich aus und verteilen sie auf die einzelnen Parameterkurven <--
                vResults = _parser.Eval(nFunctions);

                for (int i = 0; i < 3; i++)
                {
                    if (i + 3 * k >= nFunctions)
                        _pData.setData(t, i, 0.0, k);
                        //break;

                    if (isinf(vResults[i + 3 * k]) || isnan(vResults[i + 3 * k]))
                    {
                        if (!t)
                            _pData.setData(t, i, NAN, k);
                        else
                            _pData.setData(t, i, NAN, k);
                    }
                    else
                        _pData.setData(t, i, vResults[i + 3 * k].real(), k);
                }
            }

            for (int t = nRenderSamples; t < _pInfo.nSamples; t++)
            {
                for (int i = 0; i < 3; i++)
                    _pData.setData(t, i, NAN, k);
            }
        }

        _defVars.vValue[TCOORD][0] = dt_max;
    }
    else if (sFunc != "<<empty>>" && _pInfo.b3DVect)
    {
        // --> Vektorfeld (3D= <--
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                _defVars.vValue[XCOORD][0] += (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) / (double)(_pInfo.nSamples - 1);
            }
            _defVars.vValue[YCOORD][0] = _pInfo.dRanges[YCOORD][0];
            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (y != 0)
                {
                    _defVars.vValue[YCOORD][0] += (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (double)(_pInfo.nSamples - 1);
                }
                _defVars.vValue[ZCOORD][0] = _pInfo.dRanges[ZCOORD][0];
                for (int z = 0; z < _pInfo.nSamples; z++)
                {
                    if (z != 0)
                    {
                        _defVars.vValue[ZCOORD][0] += (_pInfo.dRanges[ZCOORD][1] - _pInfo.dRanges[ZCOORD][0]) / (double)(_pInfo.nSamples - 1);
                    }
                    vResults = _parser.Eval(nFunctions);

                    for (int i = 0; i < nFunctions; i++)
                    {
                        if (i > 2)
                            break;
                        if (isnan(vResults[i]) || isinf(vResults[i]))
                        {
                            if (!x || !y)
                                _pData.setData(x, y, NAN, 3 * z + i);
                            else
                                _pData.setData(x, y, NAN, 3 * z + i);
                        }
                        else
                            _pData.setData(x, y, vResults[i].real(), 3 * z + i);
                    }
                }
            }
        }
    }
    else if (sFunc != "<<empty>>" && _pInfo.b2DVect)
    {
        // --> Wie oben, aber 2D-Fall <--
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                _defVars.vValue[XCOORD][0] += (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) / (double)(_pInfo.nSamples - 1);
            }
            _defVars.vValue[YCOORD][0] = _pInfo.dRanges[YCOORD][0];
            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (y != 0)
                {
                    _defVars.vValue[YCOORD][0] += (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (double)(_pInfo.nSamples - 1);
                }
                vResults = _parser.Eval(nFunctions);

                for (int i = 0; i < nFunctions; i++)
                {
                    if (i > 1)
                        break;
                    if (isnan(vResults[i]) || isinf(vResults[i]))
                    {
                        if (!x || !y)
                            _pData.setData(x, y, NAN, i);
                        else
                            _pData.setData(x, y, NAN, i);
                    }
                    else
                        _pData.setData(x, y, vResults[i].real(), i);
                }
            }
        }
    }
    else if (sFunc != "<<empty>>")
    {
        //mglData* test = _mAxisVals;
        // --> Wie oben, aber 2D-Fall <--
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getxLogscale())
                    _defVars.vValue[XCOORD][0] = pow(10.0, log10(_pInfo.dRanges[XCOORD][0]) + (log10(_pInfo.dRanges[XCOORD][1]) - log10(_pInfo.dRanges[XCOORD][0])) / (double)(_pInfo.nSamples - 1) * (double)x);
                else
                    _defVars.vValue[XCOORD][0] += (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) / (double)(_pInfo.nSamples - 1);
            }
            _defVars.vValue[YCOORD][0] = _pInfo.dRanges[YCOORD][0];
            _mAxisVals[0].a[x] = _defVars.vValue[XCOORD][0].real();
            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (y != 0)
                {
                    if (_pData.getyLogscale())
                        _defVars.vValue[YCOORD][0] = pow(10.0, log10(_pInfo.dRanges[YCOORD][0]) + (log10(_pInfo.dRanges[YCOORD][1]) - log10(_pInfo.dRanges[YCOORD][0])) / (double)(_pInfo.nSamples - 1) * (double)y);
                    else
                        _defVars.vValue[YCOORD][0] += (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / (double)(_pInfo.nSamples - 1);
                }
                if (!x)
                    _mAxisVals[1].a[y] = _defVars.vValue[YCOORD][0].real();
                vResults = _parser.Eval(nFunctions);
                for (int i = 0; i < nFunctions; i++)
                {
                    if (isnan(vResults[i]) || isinf(vResults[i]))
                    {
                        if (!x || !y)
                            _pData.setData(x, y, NAN, i);
                        else
                            _pData.setData(x, y, NAN, i);
                    }
                    else
                        _pData.setData(x, y, vResults[i].real(), i);
                }
            }
        }
    }

    return nFunctions;
}


/////////////////////////////////////////////////
/// \brief This member function fits the
/// remaining plotting ranges for the calculated
/// interval of data points.
///
/// \param dDataRanges[3][2] double
/// \param nPlotCompose size_t
/// \param bNewSubPlot bool
/// \return void
///
/// E.g. if a usual plot is calculated and the
/// user did only supply the x range, then this
/// function will calculate the corresponding y
/// range (and extend it to about 10%).
/////////////////////////////////////////////////
void Plot::fitPlotRanges(double dDataRanges[3][2], size_t nPlotCompose, bool bNewSubPlot)
{
    /* --> Darstellungsintervalle anpassen: Wenn nicht alle vorgegeben sind, sollten die fehlenden
     *     passend berechnet werden. Damit aber kein Punkt auf dem oberen oder dem unteren Rahmen
     *     liegt, vergroessern wir das Intervall um 5% nach oben und 5% nach unten <--
     * --> Fuer Vektor- und 3D-Plots ist das allerdings recht sinnlos <--
     */
    if (sFunc != "<<empty>>")
    {
        if (isnan(_pData.getMin()) || isnan(_pData.getMax()) || isinf(_pData.getMin()) || isinf(_pData.getMax()))
        {
            clearData();
            throw SyntaxError(SyntaxError::PLOTDATA_IS_NAN, "", SyntaxError::invalid_position);
        }
    }

    if (!(_pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect) && (!nPlotCompose || bNewSubPlot) && !(_pInfo.bDraw3D || _pInfo.bDraw))
    {
        if (!_pInfo.b2D && _pData.getGivenRanges() < 2 && _pInfo.sCommand != "plot3d")
        {
            if (sFunc != "<<empty>>")
            {
                double dMinl = _pData.getMin(PlotData::ONLYLEFT);
                double dMaxl = _pData.getMax(PlotData::ONLYLEFT);
                double dMinr = _pData.getMin(PlotData::ONLYRIGHT);
                double dMaxr = _pData.getMax(PlotData::ONLYRIGHT);

                weightedRange(PlotData::ONLYLEFT, dMinl, dMaxl);
                weightedRange(PlotData::ONLYRIGHT, dMaxl, dMaxr);

                if (v_mDataPlots.size() && dMinl < _pInfo.dRanges[YCOORD][0])
                    _pInfo.dRanges[YCOORD][0] = dMinl;
                if (v_mDataPlots.size() && dMaxl > _pInfo.dRanges[YCOORD][1])
                    _pInfo.dRanges[YCOORD][1] = dMaxl;
                if (!v_mDataPlots.size())
                {
                    _pInfo.dRanges[YCOORD][0] = dMinl;
                    _pInfo.dRanges[YCOORD][1] = dMaxl;
                }
                if (v_mDataPlots.size() && (dMinr < _pInfo.dSecAxisRanges[1][0] || isnan(_pInfo.dSecAxisRanges[1][0])))
                    _pInfo.dSecAxisRanges[1][0] = dMinr;
                if (v_mDataPlots.size() && (dMaxr > _pInfo.dSecAxisRanges[1][1] || isnan(_pInfo.dSecAxisRanges[1][1])))
                    _pInfo.dSecAxisRanges[1][1] = dMaxr;
                if (!v_mDataPlots.size())
                {
                    _pInfo.dSecAxisRanges[1][0] = dMinr;
                    _pInfo.dSecAxisRanges[1][1] = dMaxr;
                    if (!isnan(_pData.getAddAxis(1).dMin))
                    {
                        _pInfo.dSecAxisRanges[1][0] = _pData.getAddAxis(1).dMin;
                        _pInfo.dSecAxisRanges[1][1] = _pData.getAddAxis(1).dMax;
                    }
                }
                if ((isnan(dMinl) || isnan(_pInfo.dRanges[YCOORD][0])) && isnan(dDataRanges[1][0]))
                {
                    _pInfo.dRanges[YCOORD][0] = _pInfo.dSecAxisRanges[1][0];
                    _pInfo.dRanges[YCOORD][0] = _pInfo.dSecAxisRanges[1][1];
                }
            }
            double dInt = fabs(_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]);
            if (dInt == 0.0 || (dInt < 1e-4 * _pInfo.dRanges[YCOORD][0]))
                dInt = fabs(_pInfo.dRanges[YCOORD][1]);
            _pInfo.dRanges[YCOORD][0] -= dInt / 20.0;
            if (_pInfo.dRanges[YCOORD][0] <= 0 && _pData.getyLogscale())
                _pInfo.dRanges[YCOORD][0] += dInt / 20.0;
            if (_pInfo.dRanges[YCOORD][0] < 0.0 && (_pData.getCoords() != PlotData::CARTESIAN))
                _pInfo.dRanges[YCOORD][0] = 0.0;
            _pInfo.dRanges[YCOORD][1] += dInt / 20.0;
            /*dInt = fabs(_pInfo.dSecAxisRanges[1][1] - _pInfo.dSecAxisRanges[1][0]);
            if (!isnan(dInt) && isnan(_pData.getAddAxis(1).dMin))
            {
                if (dInt == 0.0 || (dInt < 1e-4 * _pInfo.dSecAxisRanges[1][0]))
                    dInt = fabs(_pInfo.dSecAxisRanges[1][1]);
                _pInfo.dSecAxisRanges[1][0] -= dInt / 20.0;
                if (_pInfo.dSecAxisRanges[1][0] <= 0 && _pData.getyLogscale())
                    _pInfo.dSecAxisRanges[1][0] += dInt / 20.0;
                if (_pInfo.dSecAxisRanges[1][0] < 0.0 && (_pData.getCoords() != PlotData::CARTESIAN))
                    _pInfo.dSecAxisRanges[1][0] = 0.0;
                _pInfo.dSecAxisRanges[1][1] += dInt / 20.0;
            }
            for (int i = 0; i < 2; i++)
                _pData.setAddAxis(i, _pInfo.dSecAxisRanges[i][0], _pInfo.dSecAxisRanges[i][1]);*/
        }
        else if (_pInfo.sCommand == "plot3d" && _pData.getGivenRanges() < 3)
        {
            if (sFunc != "<<empty>>")
            {
                double dMin, dMax;
                for (int i = 0; i < 3; i++)
                {
                    dMin = _pData.getMin(i);
                    dMax = _pData.getMax(i);

                    weightedRange(i, dMin, dMax);

                    if (_pData.getGivenRanges() >= i + 1 && _pData.getRangeSetting(i))
                        continue;
                    if (v_mDataPlots.size() && dMin < _pInfo.dRanges[i][0])
                        _pInfo.dRanges[i][0] = dMin;
                    if (v_mDataPlots.size() && dMax > _pInfo.dRanges[i][1])
                        _pInfo.dRanges[i][1] = dMax;
                    if (!v_mDataPlots.size())
                    {
                        _pInfo.dRanges[i][0] = dMin;
                        _pInfo.dRanges[i][1] = dMax;
                    }
                    double dInt = fabs(_pInfo.dRanges[i][1] - _pInfo.dRanges[i][0]);
                    if (dInt == 0.0 || dInt < 1e-4 * _pInfo.dRanges[i][0])
                        dInt = fabs(_pInfo.dRanges[i][1]);
                    _pInfo.dRanges[i][0] -= dInt / 20.0;
                    _pInfo.dRanges[i][1] += dInt / 20.0;
                    if ((_pInfo.dRanges[i][0] < 0.0 && (_pData.getCoords() == PlotData::SPHERICAL_PT || (_pData.getCoords() == PlotData::POLAR_PZ && i < 2)))
                            || (_pInfo.dRanges[i][0] && _pData.getCoords() != PlotData::CARTESIAN && !i))
                        _pInfo.dRanges[i][0] = 0.0;
                }
                if (_pData.getCoords() != PlotData::CARTESIAN && _pInfo.dRanges[YCOORD][0] != 0.0)
                    _pInfo.dRanges[YCOORD][0] = 0.0;
                if (_pData.getCoords() == PlotData::SPHERICAL_PT && _pInfo.dRanges[ZCOORD][0] != 0.0)
                    _pInfo.dRanges[ZCOORD][0] = 0.0;
            }
        }
        else if (_pData.getGivenRanges() < 3 && _pInfo.sCommand != "implot")
        {
            if (sFunc != "<<empty>>")
            {
                double dMin = _pData.getMin();
                double dMax = _pData.getMax();

                weightedRange(PlotData::ALLRANGES, dMin, dMax);

                if (v_mDataPlots.size() && dMin < _pInfo.dRanges[ZCOORD][0])
                    _pInfo.dRanges[ZCOORD][0] = dMin;
                if (v_mDataPlots.size() && dMax > _pInfo.dRanges[ZCOORD][1])
                    _pInfo.dRanges[ZCOORD][1] = dMax;
                if (!v_mDataPlots.size())
                {
                    _pInfo.dRanges[ZCOORD][0] = dMin;
                    _pInfo.dRanges[ZCOORD][1] = dMax;
                }
            }
            double dInt = fabs(_pInfo.dRanges[ZCOORD][1] - _pInfo.dRanges[ZCOORD][0]);
            if (dInt == 0.0 || dInt < 1e-4 * _pInfo.dRanges[ZCOORD][0])
                dInt = fabs(_pInfo.dRanges[ZCOORD][1]);
            _pInfo.dRanges[ZCOORD][0] -= dInt / 20.0;
            if (_pInfo.dRanges[ZCOORD][0] <= 0 && _pData.getzLogscale())
                _pInfo.dRanges[ZCOORD][0] += dInt / 20.0;
            if (_pInfo.dRanges[ZCOORD][0] < 0.0 && (_pData.getCoords() != PlotData::CARTESIAN && _pData.getCoords() != PlotData::POLAR_RP))
                _pInfo.dRanges[ZCOORD][0] = 0.0;
            _pInfo.dRanges[ZCOORD][1] += dInt / 20.0;
        }

        if (!_pInfo.b2D && _pInfo.sCommand != "plot3d")
        {
            double dInt = fabs(_pInfo.dSecAxisRanges[1][1] - _pInfo.dSecAxisRanges[1][0]);
            if (!isnan(dInt) && isnan(_pData.getAddAxis(1).dMin))
            {
                if (dInt == 0.0 || (dInt < 1e-4 * _pInfo.dSecAxisRanges[1][0]))
                    dInt = fabs(_pInfo.dSecAxisRanges[1][1]);
                _pInfo.dSecAxisRanges[1][0] -= dInt / 20.0;
                if (_pInfo.dSecAxisRanges[1][0] <= 0 && _pData.getyLogscale())
                    _pInfo.dSecAxisRanges[1][0] += dInt / 20.0;
                if (_pInfo.dSecAxisRanges[1][0] < 0.0 && (_pData.getCoords() != PlotData::CARTESIAN))
                    _pInfo.dSecAxisRanges[1][0] = 0.0;
                _pInfo.dSecAxisRanges[1][1] += dInt / 20.0;
            }
            for (int i = 0; i < 2; i++)
                _pData.setAddAxis(i, _pInfo.dSecAxisRanges[i][0], _pInfo.dSecAxisRanges[i][1]);
        }
    }
    else if (_pInfo.b2DVect && (!nPlotCompose || bNewSubPlot) && !(_pInfo.bDraw3D || _pInfo.bDraw))
    {
        if (sFunc != "<<empty>>")
        {
            if (_pData.getGivenRanges() < 3)
            {
                double dMin = _pData.getMin();
                double dMax = _pData.getMax();

                weightedRange(PlotData::ALLRANGES, dMin, dMax);

                _pInfo.dRanges[ZCOORD][0] = dMin;
                _pInfo.dRanges[ZCOORD][1] = dMax;
            }
            if (_pData.getCoords() != PlotData::CARTESIAN)
            {
                _pInfo.dRanges[YCOORD][0] = 0.0;
                _pInfo.dRanges[XCOORD][0] = 0.0;
            }
        }
    }

    if (_pData.getxLogscale() || _pData.getyLogscale() || _pData.getzLogscale())
    {
        if (_pData.getxLogscale())
        {
            if ((_pInfo.dRanges[XCOORD][0] <= 0 && _pInfo.dRanges[XCOORD][1] <= 0) || _pData.getAxisScale() <= 0.0)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
            else if (_pInfo.dRanges[XCOORD][0] <= 0)
                _pInfo.dRanges[XCOORD][0] = _pInfo.dRanges[XCOORD][1] * 1e-3;
        }
        if (_pData.getyLogscale())
        {
            if ((_pInfo.dRanges[YCOORD][0] <= 0 && _pInfo.dRanges[YCOORD][1] <= 0) || _pData.getAxisScale(1) <= 0.0)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
            else if (_pInfo.dRanges[YCOORD][0] <= 0)
                _pInfo.dRanges[YCOORD][0] = _pInfo.dRanges[YCOORD][1] * 1e-3;
        }
        if (_pData.getzLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            if ((_pInfo.dRanges[ZCOORD][0] <= 0 && _pInfo.dRanges[ZCOORD][1] <= 0) || _pData.getAxisScale(2) <= 0.0)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
            else if (_pInfo.dRanges[ZCOORD][0] <= 0)
                _pInfo.dRanges[ZCOORD][0] = _pInfo.dRanges[ZCOORD][1] * 1e-3;
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
    v_mDataPlots.clear();
}


/////////////////////////////////////////////////
/// \brief This member function informs the
/// mglGraph object about the new plotting
/// interval ranges.
///
/// \param dDataRanges[3][2] double
/// \return void
///
/////////////////////////////////////////////////
void Plot::passRangesToGraph(double dDataRanges[3][2])
{
    if (_pData.getBoxplot() && !_pData.getRangeSetting() && v_mDataPlots.size())
    {
        _pInfo.dRanges[XCOORD][0] = 0;
        _pInfo.dRanges[XCOORD][1] = 1;
        for (size_t i = 0; i < v_mDataPlots.size(); i++)
            _pInfo.dRanges[XCOORD][1] += v_mDataPlots[i].size() - 1;
    }
    if (_pData.getInvertion())
        _graph->SetRange('x', _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(), _pInfo.dRanges[XCOORD][0] / _pData.getAxisScale());
    else
        _graph->SetRange('x', _pInfo.dRanges[XCOORD][0] / _pData.getAxisScale(), _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale());
    if (_pData.getInvertion(1))
        _graph->SetRange('y', _pInfo.dRanges[YCOORD][1] / _pData.getAxisScale(1), _pInfo.dRanges[YCOORD][0] / _pData.getAxisScale(1));
    else
        _graph->SetRange('y', _pInfo.dRanges[YCOORD][0] / _pData.getAxisScale(1), _pInfo.dRanges[YCOORD][1] / _pData.getAxisScale(1));
    if (_pData.getInvertion(2))
        _graph->SetRange('z', _pInfo.dRanges[ZCOORD][1] / _pData.getAxisScale(2), _pInfo.dRanges[ZCOORD][0] / _pData.getAxisScale(2));
    else
        _graph->SetRange('z', _pInfo.dRanges[ZCOORD][0] / _pData.getAxisScale(2), _pInfo.dRanges[ZCOORD][1] / _pData.getAxisScale(2));

    if (_pData.getBars() && v_mDataPlots.size() && !_pInfo.b2D)
    {
        int nMinbars = -1;
        for (size_t k = 0; k < v_mDataPlots.size(); k++)
        {
            if (nMinbars == -1 || nMinbars > v_mDataPlots[k][0].nx)
                nMinbars = v_mDataPlots[k][0].nx;
        }
        if (nMinbars < 2)
            nMinbars = 2;
        if (_pData.getInvertion())
            _graph->SetRange('x', (_pInfo.dRanges[XCOORD][1] - fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]) / (double)(2 * (nMinbars - 1))) / _pData.getAxisScale(), (_pInfo.dRanges[XCOORD][0] + fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]) / (double)(2 * (nMinbars - 1))) / _pData.getAxisScale());
        else
            _graph->SetRange('x', (_pInfo.dRanges[XCOORD][0] - fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]) / (double)(2 * (nMinbars - 1))) / _pData.getAxisScale(), (_pInfo.dRanges[XCOORD][1] + fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]) / (double)(2 * (nMinbars - 1))) / _pData.getAxisScale());
        if (_pInfo.sCommand == "plot3d")
        {
            if (_pData.getInvertion(1))
                _graph->SetRange('y', (_pInfo.dRanges[YCOORD][1] - fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1]) / (double)(2 * (nMinbars - 1))) / _pData.getAxisScale(1), (_pInfo.dRanges[YCOORD][0] + fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1]) / (double)(2 * (nMinbars - 1))) / _pData.getAxisScale(1));
            else
                _graph->SetRange('y', (_pInfo.dRanges[YCOORD][0] - fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1]) / (double)(2 * (nMinbars - 1))) / _pData.getAxisScale(1), (_pInfo.dRanges[YCOORD][1] + fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1]) / (double)(2 * (nMinbars - 1))) / _pData.getAxisScale(1));
        }
    }

    if (!isnan(_pData.getColorRange()) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
    {
        if (_pData.getcLogscale() && ((_pData.getColorRange() <= 0.0 && _pData.getColorRange(1) <= 0.0) || _pData.getAxisScale(3) <= 0.0))
        {
            clearData();
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
        }
        else if (_pData.getcLogscale() && _pData.getColorRange() <= 0.0)
        {
            _graph->SetRange('c', _pData.getColorRange(1) * 1e-3 / _pData.getAxisScale(3), _pData.getColorRange(1) / _pData.getAxisScale(3));
            _pInfo.dColorRanges[0] = _pData.getColorRange(1) * 1e-3 / _pData.getAxisScale(3);
            _pInfo.dColorRanges[1] = _pData.getColorRange(1) / _pData.getAxisScale(3);
        }
        else
        {
            _graph->SetRange('c', _pData.getColorRange() / _pData.getAxisScale(3), _pData.getColorRange(1) / _pData.getAxisScale(3));
            _pInfo.dColorRanges[0] = _pData.getColorRange() / _pData.getAxisScale(3);
            _pInfo.dColorRanges[1] = _pData.getColorRange(1) / _pData.getAxisScale(3);
        }
    }
    else if (v_mDataPlots.size())
    {
        double dColorMin = dDataRanges[2][0] / _pData.getAxisScale(3);
        double dColorMax = dDataRanges[2][1] / _pData.getAxisScale(3);
        double dMin = _pData.getMin(2);
        double dMax = _pData.getMax(2);

        weightedRange(2, dMin, dMax);

        if (dMax > dColorMax && sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
            dColorMax = dMax / _pData.getAxisScale(3);

        if (dMin < dColorMin && sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
            dColorMin = dMin / _pData.getAxisScale(3);

        if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0 && dColorMax <= 0.0)
        {
            clearData();
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
        }
        else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0)
        {
            dColorMin = dColorMax * 1e-3;
            _graph->SetRange('c', dColorMin, dColorMax + 0.05 * (dColorMax - dColorMin));
            _pInfo.dColorRanges[0] = dColorMin;
            _pInfo.dColorRanges[1] = dColorMax + 0.05 * (dColorMax - dColorMin);
        }
        else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            _graph->SetRange('c', dColorMin * 0.95, dColorMax * 1.05);
            _pInfo.dColorRanges[0] = dColorMin * 0.95;
            _pInfo.dColorRanges[1] = dColorMax * 1.05;
        }
        else if (_pInfo.sCommand == "implot")
        {
            _graph->SetRange('c', 0, 255.0);
            _pInfo.dColorRanges[0] = 0;
            _pInfo.dColorRanges[1] = 255.0;
        }
        else
        {
            _graph->SetRange('c', dColorMin - 0.05 * (dColorMax - dColorMin), dColorMax + 0.05 * (dColorMax - dColorMin));
            _pInfo.dColorRanges[0] = dColorMin - 0.05 * (dColorMax - dColorMin);
            _pInfo.dColorRanges[1] = dColorMax + 0.05 * (dColorMax - dColorMin);
        }
    }
    else
    {
        if ((_pInfo.b2DVect || _pInfo.b3DVect) && (_pData.getFlow() || _pData.getPipe()))
        {
            if (_pData.getcLogscale())
            {
                _graph->SetRange('c', 1e-3, 1.05);
                _pInfo.dColorRanges[0] = 1e-3;
                _pInfo.dColorRanges[1] = 1.05;
            }
            else
            {
                _graph->SetRange('c', -1.05, 1.05);
                _pInfo.dColorRanges[0] = -1.05;
                _pInfo.dColorRanges[1] = 1.05;
            }
        }
        else if (_pInfo.b2DVect || _pInfo.b3DVect)
        {
            if (_pData.getcLogscale())
            {
                _graph->SetRange('c', 1e-3, 1.05);
                _pInfo.dColorRanges[0] = 1e-3;
                _pInfo.dColorRanges[1] = 1.05;
            }
            else
            {
                _graph->SetRange('c', -0.05, 1.05);
                _pInfo.dColorRanges[0] = -0.05;
                _pInfo.dColorRanges[1] = 1.05;
            }
        }
        else
        {
            double dMax = _pData.getMax();
            double dMin = _pData.getMin();

            weightedRange(PlotData::ALLRANGES, dMin, dMax);

            if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && ((dMin <= 0.0 && dMax) || _pData.getAxisScale(3) <= 0.0))
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dMin <= 0.0)
            {
                _graph->SetRange('c', dMax * 1e-3 / _pData.getAxisScale(3), (dMax + 0.05 * (dMax - dMin)) / _pData.getAxisScale(3));
                _pInfo.dColorRanges[0] = dMax * 1e-3 / _pData.getAxisScale(3);
                _pInfo.dColorRanges[1] = (dMax + 0.05 * (dMax - dMin)) / _pData.getAxisScale(3);
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
            {
                _graph->SetRange('c', dMin * 0.95 / _pData.getAxisScale(3), dMax * 1.05 / _pData.getAxisScale(3));
                _pInfo.dColorRanges[0] = dMin * 0.95 / _pData.getAxisScale(3);
                _pInfo.dColorRanges[1] = dMax * 1.05 / _pData.getAxisScale(3);
            }
            else
            {
                _graph->SetRange('c', (dMin - 0.05 * (dMax - dMin)) / _pData.getAxisScale(3), (dMax + 0.05 * (dMax - dMin)) / _pData.getAxisScale(3));
                _pInfo.dColorRanges[0] = (dMin - 0.05 * (dMax - dMin)) / _pData.getAxisScale(3);
                _pInfo.dColorRanges[1] = (dMax + 0.05 * (dMax - dMin)) / _pData.getAxisScale(3);
            }
        }
    }
    // --> Andere Parameter setzen (die i. A. von den bestimmten Ranges abghaengen) <--
    // --> Gitter-, Koordinaten- und Achsenbeschriftungen <--
    CoordSettings();

    if (_pData.getAxisScale() != 1.0 || _pData.getAxisScale(1) != 1.0 || _pData.getAxisScale(2) != 1.0 || _pData.getAxisScale(3) != 1.0)
    {
        if (_pData.getInvertion())
            _graph->SetRange('x', _pInfo.dRanges[XCOORD][1], _pInfo.dRanges[XCOORD][0]);
        else
            _graph->SetRange('x', _pInfo.dRanges[XCOORD][0], _pInfo.dRanges[XCOORD][1]);
        if (_pData.getInvertion(1))
            _graph->SetRange('y', _pInfo.dRanges[YCOORD][1], _pInfo.dRanges[YCOORD][0]);
        else
            _graph->SetRange('y', _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[YCOORD][1]);
        if (_pData.getInvertion(2))
            _graph->SetRange('z', _pInfo.dRanges[ZCOORD][1], _pInfo.dRanges[ZCOORD][0]);
        else
            _graph->SetRange('z', _pInfo.dRanges[ZCOORD][0], _pInfo.dRanges[ZCOORD][1]);
        if (_pData.getBars() && v_mDataPlots.size() && !_pInfo.b2D)
        {
            int nMinbars = -1;
            for (size_t k = 0; k < v_mDataPlots.size(); k++)
            {
                if (nMinbars == -1 || nMinbars > v_mDataPlots[k][0].nx)
                    nMinbars = v_mDataPlots[k][0].nx;
            }
            if (nMinbars < 2)
                nMinbars = 2;
            if (_pData.getInvertion())
                _graph->SetRange('x', _pInfo.dRanges[XCOORD][1] - fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]) / (double)(2 * (nMinbars - 1)), _pInfo.dRanges[XCOORD][0] + fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]) / (double)(2 * (nMinbars - 1)));
            else
                _graph->SetRange('x', _pInfo.dRanges[XCOORD][0] - fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]) / (double)(2 * (nMinbars - 1)), _pInfo.dRanges[XCOORD][1] + fabs(_pInfo.dRanges[XCOORD][0] - _pInfo.dRanges[XCOORD][1]) / (double)(2 * (nMinbars - 1)));
            if (_pInfo.sCommand == "plot3d")
            {
                if (_pData.getInvertion(1))
                    _graph->SetRange('y', _pInfo.dRanges[YCOORD][1] - fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1]) / (double)(2 * (nMinbars - 1)), _pInfo.dRanges[YCOORD][0] + fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1]) / (double)(2 * (nMinbars - 1)));
                else
                    _graph->SetRange('y', _pInfo.dRanges[YCOORD][0] - fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1]) / (double)(2 * (nMinbars - 1)), _pInfo.dRanges[YCOORD][1] + fabs(_pInfo.dRanges[YCOORD][0] - _pInfo.dRanges[YCOORD][1]) / (double)(2 * (nMinbars - 1)));
            }
        }

        if (!isnan(_pData.getColorRange()) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            if (_pData.getcLogscale() && _pData.getColorRange() <= 0.0)
            {
                _graph->SetRange('c', _pData.getColorRange(1) * 1e-3, _pData.getColorRange(1));
                _pInfo.dColorRanges[0] = _pData.getColorRange(1) * 1e-3;
                _pInfo.dColorRanges[1] = _pData.getColorRange(1);
            }
            else
            {
                _graph->SetRange('c', _pData.getColorRange(), _pData.getColorRange(1));
                _pInfo.dColorRanges[0] = _pData.getColorRange();
                _pInfo.dColorRanges[1] = _pData.getColorRange(1);
            }
        }
        else if (v_mDataPlots.size())
        {
            double dColorMin = dDataRanges[2][0];
            double dColorMax = dDataRanges[2][1];

            double dMin = _pData.getMin(2);
            double dMax = _pData.getMax(2);

            weightedRange(2, dMin, dMax);

            if (dMax > dColorMax && sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
                dColorMax = dMax;
            if (dMin < dColorMin && sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
                dColorMin = dMin;

            if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0 && dColorMax <= 0.0)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0)
            {
                dColorMin = dColorMax * 1e-3;
                _graph->SetRange('c', dColorMin, dColorMax + 0.05 * (dColorMax - dColorMin));
                _pInfo.dColorRanges[0] = dColorMin;
                _pInfo.dColorRanges[1] = dColorMax + 0.05 * (dColorMax - dColorMin);
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
            {
                _graph->SetRange('c', dColorMin * 0.95, dColorMax * 1.05);
                _pInfo.dColorRanges[0] = dColorMin * 0.95;
                _pInfo.dColorRanges[1] = dColorMax * 1.05;
            }
            else
            {
                _graph->SetRange('c', dColorMin - 0.05 * (dColorMax - dColorMin), dColorMax + 0.05 * (dColorMax - dColorMin));
                _pInfo.dColorRanges[0] = dColorMin - 0.05 * (dColorMax - dColorMin);
                _pInfo.dColorRanges[1] = dColorMax + 0.05 * (dColorMax - dColorMin);
            }
        }
        else
        {
            double dMin = _pData.getMin();
            double dMax = _pData.getMax();

            weightedRange(PlotData::ALLRANGES, dMin, dMax);

            if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dMin <= 0.0 && dMax)
            {
                clearData();
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, "", SyntaxError::invalid_position);
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dMin <= 0.0)
            {
                _graph->SetRange('c', dMax * 1e-3, (dMax + 0.05 * (dMax - dMin)));
                _pInfo.dColorRanges[0] = dMax * 1e-3;
                _pInfo.dColorRanges[1] = (dMax + 0.05 * (dMax - dMin));
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
            {
                _graph->SetRange('c', dMin * 0.95, dMax * 1.05);
                _pInfo.dColorRanges[0] = dMin * 0.95;
                _pInfo.dColorRanges[1] = dMax * 1.05;
            }
            else
            {
                _graph->SetRange('c', (dMin - 0.05 * (dMax - dMin)), (dMax + 0.05 * (dMax - dMin)));
                _pInfo.dColorRanges[0] = (dMin - 0.05 * (dMax - dMin));
                _pInfo.dColorRanges[1] = (dMax + 0.05 * (dMax - dMin));
            }
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
    if (_pData.getColorbar() && (_pInfo.sCommand.substr(0, 4) == "grad" || _pInfo.sCommand.substr(0, 4) == "dens" || _pInfo.sCommand == "implot") && !_pInfo.b3D && !_pData.getSchematic())
    {
        if (_pData.getAxisScale(3) != 1.0)
            _graph->SetRange('c', _pInfo.dColorRanges[0] / _pData.getAxisScale(3), _pInfo.dColorRanges[1] / _pData.getAxisScale(3));

        // --> In diesem Fall haetten wir gerne eine Colorbar fuer den Farbwert <--
        if (_pData.getBox())
        {
            if (!(_pData.getContProj() && _pData.getContFilled()) && _pInfo.sCommand.substr(0, 4) != "dens" && _pInfo.sCommand != "implot")
                _graph->Colorbar(_pData.getColorSchemeLight("I>").c_str());
            else
                _graph->Colorbar(_pData.getColorScheme("I>").c_str());
        }
        else
        {
            if (!(_pData.getContProj() && _pData.getContFilled()) && _pInfo.sCommand.substr(0, 4) != "dens" && _pInfo.sCommand != "implot")
                _graph->Colorbar(_pData.getColorSchemeLight(">").c_str());
            else
                _graph->Colorbar(_pData.getColorScheme().c_str());
        }

        if (_pData.getAxisScale(3) != 1.0)
            _graph->SetRange('c', _pInfo.dColorRanges[0], _pInfo.dColorRanges[1]);
    }
    else if (_pData.getColorbar() && !_pData.getSchematic()
             && (_pInfo.sCommand.substr(0, 4) == "mesh"
                 || _pInfo.sCommand.substr(0, 4) == "surf"
                 || _pInfo.sCommand.substr(0, 4) == "cont"
                 || _pInfo.sCommand.substr(0, 4) == "dens"
                 || _pInfo.sCommand.substr(0, 4) == "grad"
                 || (_pInfo.sCommand.substr(0, 6) == "plot3d" && (_pData.getMarks() || _pData.getCrust()))
                )
            )
    {
        if (_pData.getAxisScale(3) != 1.0)
            _graph->SetRange('c', _pInfo.dColorRanges[0] / _pData.getAxisScale(3), _pInfo.dColorRanges[1] / _pData.getAxisScale(3));

        _graph->Colorbar(_pData.getColorScheme().c_str());

        if (_pData.getAxisScale(3) != 1.0)
            _graph->SetRange('c', _pInfo.dColorRanges[0], _pInfo.dColorRanges[1]);
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
            || _pData.getPipe()
            || _pInfo.bDraw3D
            || _pInfo.bDraw
            || (_pInfo.sCommand == "plot3d" && _pData.getCrust()))
    {
        // --> Licht- und Transparenz-Effekte <--
        if (_pInfo.sCommand.substr(0, 4) == "surf"
                || (_pInfo.sCommand.substr(0, 4) == "dens" && _pInfo.sCommand.find("3d") != string::npos && !_pData.getContProj())
                || (_pInfo.sCommand.substr(0, 4) == "cont" && _pInfo.sCommand.find("3d") && _pData.getContFilled() && !_pData.getContProj())
                || _pInfo.bDraw3D
                || _pInfo.bDraw)
            {
                _graph->Alpha(_pData.getTransparency());
                _graph->SetAlphaDef(_pData.getAlphaVal());
            }
        if (_pData.getAlphaMask() && _pInfo.sCommand.substr(0, 4) == "surf" && !_pData.getTransparency())
        {
            _graph->Alpha(true);
            _graph->SetAlphaDef(_pData.getAlphaVal());
        }

        if (_pData.getLighting())
        {
            _graph->Light(true);
            if (!_pData.getPipe() && !_pInfo.bDraw)
            {
                if (_pData.getLighting() == 1)
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
                if (_pData.getLighting() == 1)
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
    if (_pData.getCoords() == PlotData::CARTESIAN)
    {
        if ((_pData.getxLogscale() || _pData.getyLogscale() || _pData.getzLogscale()) || _pData.getcLogscale())
            _graph->SetRanges(0.1, 10.0, 0.1, 10.0, 0.1, 10.0);

        if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale() && !_pData.getcLogscale())
            _graph->SetFunc("lg(x)", "");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale() && !_pData.getcLogscale())
            _graph->SetFunc("lg(x)", "lg(y)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && !_pData.getcLogscale())
            _graph->SetFunc("lg(x)", "lg(y)", "lg(z)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && !_pData.getcLogscale())
            _graph->SetFunc("lg(x)", "lg(y)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale() && !_pData.getcLogscale())
            _graph->SetFunc("", "lg(y)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && !_pData.getcLogscale())
            _graph->SetFunc("", "lg(y)", "lg(z)");
        else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && !_pData.getcLogscale())
            _graph->SetFunc("", "", "lg(z)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && !_pData.getcLogscale())
            _graph->SetFunc("lg(x)", "", "lg(z)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && !_pData.getcLogscale())
            _graph->SetFunc("", "lg(y)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && !_pData.getcLogscale())
            _graph->SetFunc("lg(x)", "");///----------------------------------------------------------------------
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale() && _pData.getcLogscale())
            _graph->SetFunc("lg(x)", "", "", "lg(c)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale() && _pData.getcLogscale())
            _graph->SetFunc("lg(x)", "lg(y)", "", "lg(c)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && _pData.getcLogscale())
            _graph->SetFunc("lg(x)", "lg(y)", "lg(z)", "lg(c)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && _pData.getcLogscale())
            _graph->SetFunc("lg(x)", "lg(y)", "", "lg(c)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale() && _pData.getcLogscale())
            _graph->SetFunc("", "lg(y)", "", "lg(c)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && _pData.getcLogscale())
            _graph->SetFunc("", "lg(y)", "lg(z)", "lg(c)");
        else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && _pData.getcLogscale())
            _graph->SetFunc("", "", "lg(z)", "lg(c)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && _pData.getcLogscale())
            _graph->SetFunc("lg(x)", "", "lg(z)", "lg(c)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && _pData.getcLogscale())
            _graph->SetFunc("", "lg(y)", "", "lg(c)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && _pData.getcLogscale())
            _graph->SetFunc("lg(x)", "", "", "lg(c)");
        else if (!_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale() && _pData.getcLogscale())
            _graph->SetFunc("", "", "", "lg(c)");
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
    mglPoint _mDirection((_pInfo.dRanges[XCOORD][1] + _pInfo.dRanges[XCOORD][0]) / 2.0, (_pInfo.dRanges[YCOORD][1] + _pInfo.dRanges[YCOORD][0]) / 2.0, (_pInfo.dRanges[ZCOORD][1] + _pInfo.dRanges[ZCOORD][0]) / 2.0);
    double dNorm = hypot((_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) / 2.0, (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) / 2.0);
    //cerr << "dNorm=" << dNorm << endl;
    _mPoint.x = dNorm * cos(dPhi / 180.0 * M_PI + M_PI_4) * sin(dTheta / 180.0 * M_PI);
    _mPoint.y = dNorm * sin(dPhi / 180.0 * M_PI + M_PI_4) * sin(dTheta / 180.0 * M_PI);
    _mPoint.z = _pInfo.dRanges[ZCOORD][1];//0.5*dNorm*cos(dTheta/180.0*M_PI);

    _mPoint += _mDirection;

    if (_mPoint.x > _pInfo.dRanges[XCOORD][1])
        _mPoint.x = _pInfo.dRanges[XCOORD][1];
    if (_mPoint.x < _pInfo.dRanges[XCOORD][0])
        _mPoint.x = _pInfo.dRanges[XCOORD][0];
    if (_mPoint.y > _pInfo.dRanges[YCOORD][1])
        _mPoint.y = _pInfo.dRanges[YCOORD][1];
    if (_mPoint.y < _pInfo.dRanges[YCOORD][0])
        _mPoint.y = _pInfo.dRanges[YCOORD][0];

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
            case PlotData::POLAR_PZ:
                z = YCOORD;
                r = ZCOORD;
                break;
            case PlotData::POLAR_RP:
                z = ZCOORD;
                r = XCOORD;
                break;
            case PlotData::POLAR_RZ:
                z = YCOORD;
                r = XCOORD;
                break;
            case PlotData::SPHERICAL_PT:
                r = ZCOORD;
                break;
            case PlotData::SPHERICAL_RP:
                r = XCOORD;
                break;
            case PlotData::SPHERICAL_RT:
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
            if (nCoords == PlotData::CARTESIAN)
                return mglPoint(0.5 * (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) + _pInfo.dRanges[XCOORD][0], _pInfo.dRanges[YCOORD][0] - 0.1 * fabs(_pInfo.dRanges[YCOORD][0]), _pInfo.dRanges[ZCOORD][0] - 0.1 * fabs(_pInfo.dRanges[ZCOORD][0]));
            else if (nCoords == PlotData::POLAR_PZ || nCoords == PlotData::POLAR_RP || nCoords == PlotData::POLAR_RZ)
                return createMglPoint(nCoords, 0.0, 1.5 * M_PI, _pInfo.dRanges[z][0] - 0.1 * fabs(_pInfo.dRanges[z][0]));
            else if (nCoords == PlotData::SPHERICAL_PT || nCoords == PlotData::SPHERICAL_RP || nCoords == PlotData::SPHERICAL_RT)
                return createMglPoint(nCoords, 0.0, 1.5 * M_PI, 0.0, b3D);
        }
        else
        {
            if (nCoords == PlotData::CARTESIAN)
                return mglPoint(_pInfo.dRanges[XCOORD][1] + 0.1 * fabs(_pInfo.dRanges[XCOORD][1]), (0.5 * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) + _pInfo.dRanges[YCOORD][0]), _pInfo.dRanges[ZCOORD][1] + 0.1 * fabs(_pInfo.dRanges[ZCOORD][1]));
            else if (nCoords == PlotData::POLAR_PZ || nCoords == PlotData::POLAR_RP || nCoords == PlotData::POLAR_RZ)
                return createMglPoint(nCoords, 1.1 * _pInfo.dRanges[r][1], 2.0 * M_PI, _pInfo.dRanges[z][1] + 0.1 * fabs(_pInfo.dRanges[z][1]), b3D);
            else if (nCoords == PlotData::SPHERICAL_PT || nCoords == PlotData::SPHERICAL_RP || nCoords == PlotData::SPHERICAL_RT)
                return createMglPoint(nCoords, 1.1 * _pInfo.dRanges[r][1], 2.0 * M_PI, M_PI, b3D);
        }
    }
    else if (dPhi >= 90.0 && dPhi < 180.0)
    {
        if (!nEdge)
        {
            if (nCoords == PlotData::CARTESIAN)
                return mglPoint(0.5 * (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) + _pInfo.dRanges[XCOORD][0], 0.5 * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) + _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][0] - 0.1 * fabs(_pInfo.dRanges[ZCOORD][0]));
            else if (nCoords == PlotData::POLAR_PZ || nCoords == PlotData::POLAR_RP || nCoords == PlotData::POLAR_RZ)
                return createMglPoint(nCoords, 0.0, 0.0, _pInfo.dRanges[z][0] - 0.1 * fabs(_pInfo.dRanges[z][0]), b3D);
            else if (nCoords == PlotData::SPHERICAL_PT || nCoords == PlotData::SPHERICAL_RP || nCoords == PlotData::SPHERICAL_RT)
                return createMglPoint(nCoords, 0.0, 0.0, 0.0, b3D);
        }
        else
        {
            if (nCoords == PlotData::CARTESIAN)
                return mglPoint(_pInfo.dRanges[XCOORD][1] + 0.1 * fabs(_pInfo.dRanges[XCOORD][1]), _pInfo.dRanges[YCOORD][1] + 0.1 * fabs(_pInfo.dRanges[YCOORD][1]), _pInfo.dRanges[ZCOORD][1] + 0.1 * fabs(_pInfo.dRanges[ZCOORD][1]));
            else if (nCoords == PlotData::POLAR_PZ || nCoords == PlotData::POLAR_RP || nCoords == PlotData::POLAR_RZ)
                return createMglPoint(nCoords, _pInfo.dRanges[r][1] * 1.1, 0.5 * M_PI, _pInfo.dRanges[z][1] + 0.1 * fabs(_pInfo.dRanges[z][1]), b3D);
            else if (nCoords == PlotData::SPHERICAL_PT || nCoords == PlotData::SPHERICAL_RP || nCoords == PlotData::SPHERICAL_RT)
                return createMglPoint(nCoords, 1.1 * _pInfo.dRanges[r][1], 0.5 * M_PI, M_PI, b3D);
        }
    }
    else if (dPhi >= 180.0 && dPhi < 270.0)
    {
        if (!nEdge)
        {
            if (nCoords == PlotData::CARTESIAN)
                return mglPoint(_pInfo.dRanges[XCOORD][0] - 0.1 * fabs(_pInfo.dRanges[XCOORD][0]), 0.5 * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) + _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][0] - 0.1 * fabs(_pInfo.dRanges[ZCOORD][0]));
            else if (nCoords == PlotData::POLAR_PZ || nCoords == PlotData::POLAR_RP || nCoords == PlotData::POLAR_RZ)
                return createMglPoint(nCoords, 0.0, 0.5 * M_PI, _pInfo.dRanges[z][0] - 0.1 * fabs(_pInfo.dRanges[z][0]), b3D);
            else if (nCoords == PlotData::SPHERICAL_PT || nCoords == PlotData::SPHERICAL_RP || nCoords == PlotData::SPHERICAL_RT)
                return createMglPoint(nCoords, 0.0, 0.5 * M_PI, 0.0, b3D);
        }
        else
        {
            if (nCoords == PlotData::CARTESIAN)
                return mglPoint((0.5 * (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) + _pInfo.dRanges[XCOORD][0]), _pInfo.dRanges[YCOORD][1] + 0.1 * fabs(_pInfo.dRanges[YCOORD][1]), _pInfo.dRanges[ZCOORD][1] + 0.1 * fabs(_pInfo.dRanges[ZCOORD][1]));
            else if (nCoords == PlotData::POLAR_PZ || nCoords == PlotData::POLAR_RP || nCoords == PlotData::POLAR_RZ)
                return createMglPoint(nCoords, 1.1 * _pInfo.dRanges[r][1], M_PI, _pInfo.dRanges[z][1] + 0.1 * fabs(_pInfo.dRanges[z][1]), b3D);
            else if (nCoords == PlotData::SPHERICAL_PT || nCoords == PlotData::SPHERICAL_RP || nCoords == PlotData::SPHERICAL_RT)
                return createMglPoint(nCoords, 1.1 * _pInfo.dRanges[r][1], M_PI, M_PI, b3D);
        }
    }
    else
    {
        if (!nEdge)
        {
            if (nCoords == PlotData::CARTESIAN)
                return mglPoint(_pInfo.dRanges[XCOORD][0] - 0.1 * fabs(_pInfo.dRanges[XCOORD][0]), _pInfo.dRanges[YCOORD][0] - 0.1 * fabs(_pInfo.dRanges[YCOORD][0]), _pInfo.dRanges[ZCOORD][0] - 0.1 * fabs(_pInfo.dRanges[ZCOORD][0]));
            else if (nCoords == PlotData::POLAR_PZ || nCoords == PlotData::POLAR_RP || nCoords == PlotData::POLAR_RZ)
                return createMglPoint(nCoords, 0.0, M_PI, _pInfo.dRanges[z][0] - 0.1 * fabs(_pInfo.dRanges[z][0]), b3D);
            else if (nCoords == PlotData::SPHERICAL_PT || nCoords == PlotData::SPHERICAL_RP || nCoords == PlotData::SPHERICAL_RT)
                return createMglPoint(nCoords, 0.0, M_PI, 0.0, b3D);
        }
        else
        {
            if (nCoords == PlotData::CARTESIAN)
                return mglPoint(0.5 * (_pInfo.dRanges[XCOORD][1] - _pInfo.dRanges[XCOORD][0]) + _pInfo.dRanges[XCOORD][0], 0.5 * (_pInfo.dRanges[YCOORD][1] - _pInfo.dRanges[YCOORD][0]) + _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][1] + 0.1 * fabs(_pInfo.dRanges[ZCOORD][1]));
            else if (nCoords == PlotData::POLAR_PZ || nCoords == PlotData::POLAR_RP || nCoords == PlotData::POLAR_RZ)
                return createMglPoint(nCoords, 1.1 * _pInfo.dRanges[r][1], 1.5 * M_PI, _pInfo.dRanges[z][1] + 0.1 * fabs(_pInfo.dRanges[z][1]), b3D);
            else if (nCoords == PlotData::SPHERICAL_PT || nCoords == PlotData::SPHERICAL_RP || nCoords == PlotData::SPHERICAL_RT)
                return createMglPoint(nCoords, 1.1 * _pInfo.dRanges[r][1], 1.5 * M_PI, M_PI, b3D);
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
        case PlotData::POLAR_PZ:
            return mglPoint(phi, theta, r);
        case PlotData::POLAR_RP:
            return mglPoint(r, phi, theta);
        case PlotData::POLAR_RZ:
            return mglPoint(r, theta, phi);
        case PlotData::SPHERICAL_PT:
            return mglPoint(phi, theta, r);
        case PlotData::SPHERICAL_RP:
            return mglPoint(r, phi, theta);
        case PlotData::SPHERICAL_RT:
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
            return _pInfo.dRanges[XCOORD][0];
        else
            return _pInfo.dRanges[YCOORD][1];
    }
    else if (dPhi >= 90.0 && dPhi < 180.0)
    {
        if (!nEdge)
            return _pInfo.dRanges[XCOORD][0];
        else
            return _pInfo.dRanges[YCOORD][0];
    }
    else if (dPhi >= 180.0 && dPhi < 270.0)
    {
        if (!nEdge)
            return _pInfo.dRanges[XCOORD][1];
        else
            return _pInfo.dRanges[YCOORD][0];
    }
    else
    {
        if (!nEdge)
            return _pInfo.dRanges[XCOORD][1];
        else
            return _pInfo.dRanges[YCOORD][1];
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
    if (_pData.getLegendStyle())
    {
        if (_pData.getLegendStyle() == 1)
            return sLegend.substr(0, 1) + "-";
        else if (_pData.getLegendStyle() == 2)
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
/// \brief This member function applies axes and
/// axis labels to the graph.
///
/// \return void
///
/////////////////////////////////////////////////
void Plot::CoordSettings()
{
    if (_pData.getAxis())
    {
        for (int i = 0; i < 4; i++)
        {
            if (_pData.getTickTemplate(i).length())
            {
                if (i < 3)
                    _graph->SetTickTempl('x' + i, _pData.getTickTemplate(i).c_str());
                else
                    _graph->SetTickTempl('c', _pData.getTickTemplate(i).c_str());
            }
            else if (max(_pInfo.dRanges[i][0], _pInfo.dRanges[i][1]) / _pData.getAxisScale(i) < 1e-2
                     && max(_pInfo.dRanges[i][0], _pInfo.dRanges[i][1]) / _pData.getAxisScale(i) >= 1e-3)
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
                    for (unsigned int n = 0; n < _pData.getCustomTick(i).length(); n++)
                    {
                        if (_pData.getCustomTick(i)[n] == '\n')
                            nCount++;
                    }
                    //cerr << nCount << endl;
                    _mAxisRange.Create(nCount);

                    // Ranges fuer customn ticks anpassen
                    if (_pData.getCoords() != PlotData::CARTESIAN)
                    {
                        if (!(_pInfo.b2D || _pInfo.b3D || _pInfo.sCommand == "plot3d" || _pInfo.b3DVect || _pInfo.b2DVect))
                        {
                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][0] = 0.0;
                            _pInfo.dRanges[XCOORD][1] = 2.0;
                        }
                        else if (_pInfo.sCommand.find("3d") != string::npos)
                        {
                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][1] = 2.0;
                            if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RP || _pData.getCoords() == PlotData::SPHERICAL_RT)
                            {
                                _pInfo.dRanges[ZCOORD][0] = 0.0;
                                _pInfo.dRanges[ZCOORD][1] = 1.0;
                            }
                        }
                        else if (_pInfo.b2DVect)
                        {
                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][1] = 2.0;
                        }
                        else
                        {
                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[XCOORD][1] = 2.0;
                            _pInfo.dRanges[ZCOORD][0] = 0.0;
                            if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RP || _pData.getCoords() == PlotData::SPHERICAL_RT)
                            {
                                _pInfo.dRanges[YCOORD][0] = 0.0;
                                _pInfo.dRanges[YCOORD][1] = 1.0;
                            }
                        }
                    }

                    if (nCount == 1)
                    {
                        _mAxisRange.a[0] = _pInfo.dRanges[i][0] + (_pInfo.dRanges[i][1] - _pInfo.dRanges[i][0]) / 2.0;
                    }
                    else
                    {
                        for (int n = 0; n < nCount; n++)
                        {
                            _mAxisRange.a[n] = _pInfo.dRanges[i][0] + (double)n * (_pInfo.dRanges[i][1] - _pInfo.dRanges[i][0]) / (double)(nCount - 1);
                            //cerr << _pInfo.dRanges[i][0] + (double)n*(_pInfo.dRanges[i][1]-_pInfo.dRanges[i][0])/(double)(nCount-1) << endl;
                        }
                    }
                    _graph->SetTicksVal('x' + i, _mAxisRange, fromSystemCodePage(_pData.getCustomTick(i)).c_str());
                }
                else
                {
                    for (unsigned int n = 0; n < _pData.getCustomTick(i).length(); n++)
                    {
                        if (_pData.getCustomTick(i)[n] == '\n')
                            nCount++;
                    }
                    _mAxisRange.Create(nCount);
                    if (nCount == 1)
                        _mAxisRange.a[0] = _pInfo.dColorRanges[0] + (_pInfo.dColorRanges[1] - _pInfo.dColorRanges[0]) / 2.0;
                    else
                    {
                        for (int n = 0; n < nCount; n++)
                        {
                            _mAxisRange.a[n] = _pInfo.dColorRanges[0] + (double)n * (_pInfo.dColorRanges[1] - _pInfo.dColorRanges[0]) / (double)(nCount - 1);
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

        if (_pData.getBox() || _pData.getCoords() != PlotData::CARTESIAN)
        {
            if (!(_pInfo.b2D || _pInfo.b3D || _pInfo.sCommand == "plot3d" || _pInfo.b3DVect || _pInfo.b2DVect)) // standard plot
            {
                if (_pData.getCoords() == PlotData::CARTESIAN)
                {
                    if (!_pData.getSchematic())
                    {
                        if (!isnan(_pData.getAddAxis(0).dMin) || !isnan(_pData.getAddAxis(1).dMin))
                        {
                            Axis _axis;
                            _graph->SetOrigin(_pInfo.dRanges[0][1], _pInfo.dRanges[1][1]);
                            for (int i = 0; i < 2; i++)
                            {
                                _axis = _pData.getAddAxis(i);
                                if (_axis.sLabel.length())
                                {
                                    _graph->SetRange('x' + i, _axis.dMin, _axis.dMax);
                                    if (!i)
                                        _graph->Axis("x", _axis.sStyle.c_str());
                                    else
                                        _graph->Axis("y", _axis.sStyle.c_str());
                                    _graph->Label('x' + i, fromSystemCodePage("#" + _axis.sStyle + "{" + _axis.sLabel + "}").c_str(), 0);
                                }
                            }
                            _graph->SetRanges(_pInfo.dRanges[0][0], _pInfo.dRanges[0][1], _pInfo.dRanges[1][0], _pInfo.dRanges[1][1], _pInfo.dRanges[2][0], _pInfo.dRanges[2][1]);
                            _graph->SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0]);
                        }
                        _graph->Axis("xy");
                    }
                }
                else
                {
                    if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                    {
                        _graph->SetOrigin(_pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD), 0.0);
                        _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(YCOORD)).c_str(), CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(YCOORD)).c_str());
                        _graph->SetRange('y', 0.0, APPR_TWO / _pData.getAxisScale(YCOORD));
                        _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD));
                    }
                    else if (_pData.getCoords() != PlotData::CARTESIAN)
                    {
                        _graph->SetOrigin(0.0, _pInfo.dRanges[YCOORD][1] / _pData.getAxisScale(YCOORD));
                        _graph->SetFunc(CoordFunc("y*cos(pi*x*$PS$)", _pData.getAxisScale(XCOORD)).c_str(), CoordFunc("y*sin(pi*x*$PS$)", _pData.getAxisScale(XCOORD)).c_str());
                        _graph->SetRange('x', 0.0, APPR_TWO / _pData.getAxisScale(XCOORD));
                        _graph->SetRange('y', 0.0, _pInfo.dRanges[YCOORD][1] / _pData.getAxisScale(YCOORD));
                    }

                    applyGrid();

                    if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                    {
                        _graph->SetFunc("x*cos(y)", "x*sin(y)");
                        _graph->SetRange('y', 0.0, 2.0 * M_PI);
                        _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1]);
                    }
                    else if (_pData.getCoords() != PlotData::CARTESIAN)
                    {
                        _graph->SetFunc("y*cos(x)", "y*sin(x)");
                        _mAxisVals[0] = fmod(_mAxisVals[0], 2.0 * M_PI);
                        _graph->SetRange('x', 0.0, 2.0 * M_PI);
                        _graph->SetRange('y', 0.0, _pInfo.dRanges[YCOORD][1]);
                    }

                    if (!_pData.getSchematic()
                            || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                            || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                            || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph->Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), 0.25);
                        if (_pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::SPHERICAL_RP)
                            _graph->Label('y', fromSystemCodePage(_pData.getyLabel()).c_str(), 0.0);
                        else
                            _graph->Label('y', fromSystemCodePage(_pData.getzLabel()).c_str(), 0.0);
                    }

                    if (_pData.getBars() || _pData.getArea())
                        _graph->SetOrigin(0.0, 0.0);
                    _pInfo.dRanges[XCOORD][0] = 0.0;
                    _pInfo.dRanges[XCOORD][1] = 2 * M_PI;
                    _pInfo.dRanges[YCOORD][0] = 0.0;
                }
            }
            else if (_pInfo.sCommand.find("3d") != string::npos) // 3d-plots and plot3d and vect3d
            {
                if (_pData.getCoords() == PlotData::POLAR_PZ || _pData.getCoords() == PlotData::POLAR_RP || _pData.getCoords() == PlotData::POLAR_RZ)
                {
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(), 0.0, _pInfo.dRanges[ZCOORD][0] / _pData.getAxisScale(2));
                    _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(), CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(), "z");
                    _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale());
                    _graph->SetRange('y', 0.0, 1.9999999 / _pData.getAxisScale(1));
                    _graph->SetRange('z', _pInfo.dRanges[ZCOORD][0] / _pData.getAxisScale(2), _pInfo.dRanges[ZCOORD][1] / _pData.getAxisScale(2));
                    if (!_pData.getSchematic())
                        _graph->Axis();
                    else
                    {
                        _graph->SetTickLen(1e-20);
                        _graph->Axis("_");
                    }
                    _graph->Box();
                    if (_pInfo.b3DVect && _pData.getGrid())
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    else if (_pData.getGrid() == 1)
                        _graph->Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getGrid() == 2)
                    {
                        _graph->Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    if (_mAxisVals[1].nx)
                    {
                        for (int y = 0; y < _pInfo.nSamples; y++)
                        {
                            _mAxisVals[1].a[y] = ::fmod(_mAxisVals[1].a[y], 2.0 * M_PI);
                        }
                    }
                    _graph->SetFunc("x*cos(y)", "x*sin(y)", "z");
                    _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1]);
                    _graph->SetRange('y', 0.0, 2.0 * M_PI);
                    _graph->SetRange('z', _pInfo.dRanges[ZCOORD][0], _pInfo.dRanges[ZCOORD][1]);
                    if (!_pData.getSchematic()
                            || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                            || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                            || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph->Label('x', fromSystemCodePage(_pData.getzLabel()).c_str(), -0.5);
                        _graph->Label('y', fromSystemCodePage(_pData.getxLabel()).c_str(), (_pData.getRotateAngle(1) - 225.0) / 180.0); //-0.65
                        _graph->Label('z', fromSystemCodePage(_pData.getyLabel()).c_str(), 0.0);
                    }
                    if (_pData.getBars() || _pData.getArea())
                        _graph->SetOrigin(0.0, 0.0, _pInfo.dRanges[ZCOORD][0]);
                    _pInfo.dRanges[XCOORD][0] = 0.0;
                    _pInfo.dRanges[YCOORD][0] = 0.0;
                    _pInfo.dRanges[YCOORD][1] = 2.0 * M_PI;

                }
                else if (_pData.getCoords() == PlotData::SPHERICAL_PT || _pData.getCoords() == PlotData::SPHERICAL_RP || _pData.getCoords() == PlotData::SPHERICAL_RT)
                {
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(), 0.0, 0.5 / _pData.getAxisScale(2));
                    _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(1), _pData.getAxisScale(2)).c_str(), CoordFunc("x*sin(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(1), _pData.getAxisScale(2)).c_str(), CoordFunc("x*cos(pi*z*$PS$)", 1.0, _pData.getAxisScale(2)).c_str());
                    _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale());
                    _graph->SetRange('y', 0.0, 1.9999999 / _pData.getAxisScale(1));
                    _graph->SetRange('z', 0.0, 0.9999999 / _pData.getAxisScale(2));
                    if (!_pData.getSchematic())
                        _graph->Axis();
                    else
                    {
                        _graph->SetTickLen(1e-20);
                        _graph->Axis("_");
                    }
                    _graph->Box();
                    if (_pInfo.b3DVect && _pData.getGrid())
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    else if (_pData.getGrid() == 1)
                        _graph->Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getGrid() == 2)
                    {
                        _graph->Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    _graph->SetFunc("x*cos(y)*sin(z)", "x*sin(y)*sin(z)", "x*cos(z)");
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][1], 0.0, 0.5 * M_PI);
                    if (_mAxisVals[1].nx && _mAxisVals[2].nx)
                    {
                        _mAxisVals[1] = fmod(_mAxisVals[1], 2.0 * M_PI);
                        _mAxisVals[2] = fmod(_mAxisVals[2], 1.0 * M_PI);
                    }
                    _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1]);
                    _graph->SetRange('y', 0.0, 2.0 * M_PI);
                    _graph->SetRange('z', 0.0, 1.0 * M_PI);
                    if (!_pData.getSchematic()
                            || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                            || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                            || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph->Label('x', fromSystemCodePage(_pData.getzLabel()).c_str(), -0.4);
                        _graph->Label('y', fromSystemCodePage(_pData.getxLabel()).c_str(), (_pData.getRotateAngle(1) - 225.0) / 180.0); //-0.7
                        _graph->Label('z', fromSystemCodePage(_pData.getyLabel()).c_str(), -0.9); // -0.4
                    }
                    if (_pData.getBars() || _pData.getArea())
                        _graph->SetOrigin(0.0, 0.0, 0.5 * M_PI);
                    _pInfo.dRanges[XCOORD][0] = 0.0;
                    //dRanges[XCOORD][1] = 2.0;
                    _pInfo.dRanges[YCOORD][0] = 0.0;
                    _pInfo.dRanges[YCOORD][1] = 2.0 * M_PI;
                    _pInfo.dRanges[ZCOORD][0] = 0.0;
                    _pInfo.dRanges[ZCOORD][1] = 1.0 * M_PI;
                }
                else
                {
                    if (!_pData.getSchematic())
                        _graph->Axis("xyz");
                }
            }
            else if (_pInfo.b2DVect) // vect
            {
                if (_pData.getCoords() != PlotData::CARTESIAN)
                {
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(), 0.0);
                    _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(), CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(1)).c_str());
                    _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale());
                    _graph->SetRange('y', 0.0, 1.9999999 / _pData.getAxisScale(1));
                    if (!_pData.getSchematic())
                        _graph->Axis("xy");
                    else
                    {
                        _graph->SetTickLen(1e-20);
                        _graph->Axis("xy_");
                    }
                    _graph->Box();
                    if (_pData.getGrid() == 1)
                    {
                        _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    _graph->SetFunc("x*cos(y)", "x*sin(y)");
                    _mAxisVals[1] = fmod(_mAxisVals[1], 2.0 * M_PI);
                    _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1]);
                    _graph->SetRange('y', 0.0, 2.0 * M_PI);
                    if (!_pData.getSchematic()
                            || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                            || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                            || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph->Label('x', fromSystemCodePage(_pData.getzLabel()).c_str(), 0.0);
                        _graph->Label('y', fromSystemCodePage(_pData.getxLabel()).c_str(), 0.25);
                    }
                    _pInfo.dRanges[XCOORD][0] = 0.0;
                    //dRanges[XCOORD][1] = 2.0;
                    _pInfo.dRanges[YCOORD][0] = 0.0;
                    _pInfo.dRanges[YCOORD][1] = 2.0 * M_PI;
                    //dRanges[ZCOORD][0] =
                    //dRanges[ZCOORD][1] =
                }
                else
                {
                    if (!_pData.getSchematic())
                        _graph->Axis("xy");
                }
            }
            else // 2d plots
            {
                switch (_pData.getCoords())
                {
                    case PlotData::POLAR_PZ:
                        {
                            _graph->SetOrigin(0.0, _pInfo.dRanges[YCOORD][0] / _pData.getAxisScale(YCOORD), _pInfo.dRanges[ZCOORD][1] / _pData.getAxisScale(ZCOORD));
                            _graph->SetFunc(CoordFunc("z*cos(pi*x*$PS$)", _pData.getAxisScale(XCOORD)).c_str(), CoordFunc("z*sin(pi*x*$PS$)", _pData.getAxisScale(XCOORD)).c_str(), "y");

                            _graph->SetRange('x', 0.0, APPR_TWO / _pData.getAxisScale(XCOORD));
                            _graph->SetRange('y', _pInfo.dRanges[YCOORD][0] / _pData.getAxisScale(YCOORD), _pInfo.dRanges[YCOORD][1] / _pData.getAxisScale(YCOORD));
                            _graph->SetRange('z', 0.0, _pInfo.dRanges[ZCOORD][1] / _pData.getAxisScale(ZCOORD));

                            applyGrid();

                            _graph->SetFunc("z*cos(x)", "z*sin(x)", "y");
                            _mAxisVals[0] = fmod(_mAxisVals[0], 2.0 * M_PI);

                            _graph->SetRange('x', 0.0, 2.0 * M_PI);
                            _graph->SetRange('y', _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[YCOORD][1]);
                            _graph->SetRange('z', 0.0, _pInfo.dRanges[ZCOORD][1]);

                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[XCOORD][1] = 2.0 * M_PI;
                            _pInfo.dRanges[ZCOORD][0] = 0.0;
                            break;
                        }
                    case PlotData::POLAR_RP:
                        {
                            _graph->SetOrigin(_pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD), 0.0, _pInfo.dRanges[ZCOORD][0] / _pData.getAxisScale(ZCOORD));
                            _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(YCOORD)).c_str(), CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(YCOORD)).c_str(), "z");

                            _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD));
                            _graph->SetRange('y', 0.0, APPR_TWO / _pData.getAxisScale(YCOORD));
                            _graph->SetRange('z', _pInfo.dRanges[ZCOORD][0] / _pData.getAxisScale(ZCOORD), _pInfo.dRanges[ZCOORD][1] / _pData.getAxisScale(ZCOORD));

                            applyGrid();

                            _graph->SetFunc("x*cos(y)", "x*sin(y)", "z");
                            _mAxisVals[1] = fmod(_mAxisVals[1], 2.0 * M_PI);

                            _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1]);
                            _graph->SetRange('y', 0.0, 2.0 * M_PI);
                            _graph->SetRange('z', _pInfo.dRanges[ZCOORD][0], _pInfo.dRanges[ZCOORD][1]);

                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][1] = 2.0 * M_PI;
                            _pInfo.dRanges[YCOORD][0] = 0.0;
                            break;
                        }
                    case PlotData::POLAR_RZ:
                        {
                            _graph->SetOrigin(_pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD), _pInfo.dRanges[YCOORD][0] / _pData.getAxisScale(YCOORD), 0.0);
                            _graph->SetFunc(CoordFunc("x*cos(pi*z*$PS$)", _pData.getAxisScale(ZCOORD)).c_str(), CoordFunc("x*sin(pi*z*$PS$)", _pData.getAxisScale(ZCOORD)).c_str(), "y");

                            _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD));
                            _graph->SetRange('y', _pInfo.dRanges[YCOORD][0] / _pData.getAxisScale(YCOORD), _pInfo.dRanges[YCOORD][1] / _pData.getAxisScale(YCOORD));
                            _graph->SetRange('z', 0.0, APPR_TWO / _pData.getAxisScale(ZCOORD));

                            applyGrid();

                            _graph->SetFunc("x*cos(z)", "x*sin(z)", "y");

                            _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1]);
                            _graph->SetRange('y', _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[YCOORD][1]);
                            _graph->SetRange('z', 0.0, 2.0 * M_PI);

                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[ZCOORD][0] = 0.0;
                            _pInfo.dRanges[ZCOORD][1] = 2.0 * M_PI;
                            break;
                        }
                    case PlotData::SPHERICAL_PT:
                        {
                            _graph->SetOrigin(0.0, 0.5 / _pData.getAxisScale(YCOORD), _pInfo.dRanges[ZCOORD][1] / _pData.getAxisScale(ZCOORD));
                            _graph->SetFunc(CoordFunc("z*cos(pi*x*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(XCOORD), _pData.getAxisScale(YCOORD)).c_str(), CoordFunc("z*sin(pi*x*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(XCOORD), _pData.getAxisScale(YCOORD)).c_str(), CoordFunc("z*cos(pi*y*$TS$)", 1.0, _pData.getAxisScale(YCOORD)).c_str());

                            _graph->SetRange('x', 0.0, APPR_TWO / _pData.getAxisScale(XCOORD));
                            _graph->SetRange('y', 0.0, APPR_ONE / _pData.getAxisScale(YCOORD));
                            _graph->SetRange('z', 0.0, _pInfo.dRanges[ZCOORD][1] / _pData.getAxisScale(ZCOORD));

                            applyGrid();

                            _graph->SetFunc("z*cos(x)*sin(y)", "z*sin(x)*sin(y)", "z*cos(y)");
                            _graph->SetOrigin(0.0, 0.5 * M_PI, _pInfo.dRanges[ZCOORD][1]);

                            _mAxisVals[0] = fmod(_mAxisVals[0], 2.0 * M_PI);
                            _mAxisVals[1] = fmod(_mAxisVals[1], 1.0 * M_PI);

                            _graph->SetRange('x', 0.0, 2.0 * M_PI);
                            _graph->SetRange('y', 0.0, 1.0 * M_PI);
                            _graph->SetRange('z', 0.0, _pInfo.dRanges[ZCOORD][1]);

                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[XCOORD][1] = 2.0 * M_PI;
                            _pInfo.dRanges[YCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][1] = 1.0 * M_PI;
                            _pInfo.dRanges[ZCOORD][0] = 0.0;
                            break;
                        }
                    case PlotData::SPHERICAL_RP:
                        {
                            _graph->SetOrigin(_pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD), 0.0, 0.5 / _pData.getAxisScale(ZCOORD));
                            _graph->SetFunc(CoordFunc("x*cos(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(YCOORD), _pData.getAxisScale(ZCOORD)).c_str(), CoordFunc("x*sin(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(YCOORD), _pData.getAxisScale(ZCOORD)).c_str(), CoordFunc("x*cos(pi*z*$TS$)", 1.0, _pData.getAxisScale(ZCOORD)).c_str());

                            _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD));
                            _graph->SetRange('y', 0.0, APPR_TWO / _pData.getAxisScale(YCOORD));
                            _graph->SetRange('z', 0.0, APPR_ONE / _pData.getAxisScale(ZCOORD));

                            applyGrid();

                            _graph->SetFunc("x*cos(y)*sin(z)", "x*sin(y)*sin(z)", "x*cos(z)");
                            _graph->SetOrigin(_pInfo.dRanges[XCOORD][1], 0.0, 0.5 * M_PI);

                            _mAxisVals[1] = fmod(_mAxisVals[1], 2.0 * M_PI);

                            _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1]);
                            _graph->SetRange('y', 0.0, 2.0 * M_PI);
                            _graph->SetRange('z', 0.0, 1.0 * M_PI);

                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][1] = 2.0 * M_PI;
                            _pInfo.dRanges[ZCOORD][0] = 0.0;
                            _pInfo.dRanges[ZCOORD][1] = 1.0 * M_PI;
                            break;
                        }
                    case PlotData::SPHERICAL_RT:
                        {
                            _graph->SetOrigin(_pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD), 0.5 / _pData.getAxisScale(YCOORD), 0.0);
                            _graph->SetFunc(CoordFunc("x*cos(pi*z*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(ZCOORD), _pData.getAxisScale(YCOORD)).c_str(), CoordFunc("x*sin(pi*z*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(ZCOORD), _pData.getAxisScale(YCOORD)).c_str(), CoordFunc("x*cos(pi*y*$TS$)", 1.0, _pData.getAxisScale(YCOORD)).c_str());

                            _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1] / _pData.getAxisScale(XCOORD));
                            _graph->SetRange('y', 0.0, APPR_ONE / _pData.getAxisScale(YCOORD));
                            _graph->SetRange('z', 0.0, APPR_TWO / _pData.getAxisScale(ZCOORD));

                            applyGrid();

                            _graph->SetFunc("x*cos(z)*sin(y)", "x*sin(z)*sin(y)", "x*cos(y)");
                            _graph->SetOrigin(_pInfo.dRanges[XCOORD][1], 0.5 * M_PI, 0.0);

                            _mAxisVals[1] = fmod(_mAxisVals[1], 1.0 * M_PI);

                            _graph->SetRange('x', 0.0, _pInfo.dRanges[XCOORD][1]);
                            _graph->SetRange('y', 0.0, 1.0 * M_PI);
                            _graph->SetRange('z', 0.0, 2.0 * M_PI);

                            _pInfo.dRanges[XCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][0] = 0.0;
                            _pInfo.dRanges[YCOORD][1] = 1.0 * M_PI;
                            _pInfo.dRanges[ZCOORD][0] = 0.0;
                            _pInfo.dRanges[ZCOORD][1] = 2.0 * M_PI;
                            break;
                        }
                    default:
                        {
                            if (!_pData.getSchematic())
                                _graph->Axis();
                        }
                }

                if (!_pData.getSchematic()
                        || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                        || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                        || findParameter(_pInfo.sPlotParams, "zlabel", '='))
                {
                    _graph->Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), getLabelPosition(XCOORD));
                    _graph->Label('y', fromSystemCodePage(_pData.getyLabel()).c_str(), getLabelPosition(YCOORD));
                    _graph->Label('z', fromSystemCodePage(_pData.getzLabel()).c_str(), getLabelPosition(ZCOORD));
                }
            }
        }
        else if (isnan(_pData.getOrigin(XCOORD)) && isnan(_pData.getOrigin(YCOORD)) && isnan(_pData.getOrigin(ZCOORD)))
        {
            if (_pInfo.dRanges[XCOORD][0] <= 0.0
                    && _pInfo.dRanges[XCOORD][1] >= 0.0
                    && _pInfo.dRanges[YCOORD][0] <= 0.0
                    && _pInfo.dRanges[YCOORD][1] >= 0.0
                    && _pInfo.dRanges[ZCOORD][0] <= 0.0
                    && _pInfo.dRanges[ZCOORD][1] >= 0.0
                    && _pInfo.nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (_pInfo.b2D && sCommand != "dens"))
               )
            {
                _graph->SetOrigin(0.0, 0.0, 0.0);
                if (!_pData.getSchematic())
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
            else if (_pInfo.dRanges[XCOORD][0] <= 0.0
                     && _pInfo.dRanges[XCOORD][1] >= 0.0
                     && _pInfo.dRanges[YCOORD][0] <= 0.0
                     && _pInfo.dRanges[YCOORD][1] >= 0.0
                     && _pInfo.nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(_pInfo.b2D && sCommand != "dens"))
                    )
            {
                if (_pInfo.sCommand == "dens" || _pInfo.sCommand == "density")
                    _graph->SetOrigin(0.0, 0.0, _pInfo.dRanges[ZCOORD][1]);
                else
                    _graph->SetOrigin(0.0, 0.0, 0.0);

                if (!_pData.getSchematic())
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
                if (_pInfo.dRanges[XCOORD][0] <= 0.0 && _pInfo.dRanges[XCOORD][1] >= 0.0)
                {
                    if (_pInfo.dRanges[YCOORD][0] <= 0.0 && _pInfo.dRanges[YCOORD][1] >= 0.0)
                        _graph->SetOrigin(0.0, 0.0, _pInfo.dRanges[ZCOORD][0]);
                    else if (_pInfo.dRanges[ZCOORD][0] <= 0.0 && _pInfo.dRanges[ZCOORD][1] >= 0.0)
                        _graph->SetOrigin(0.0, _pInfo.dRanges[YCOORD][0], 0.0);
                    else
                        _graph->SetOrigin(0.0, _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][0]);
                }
                else if (_pInfo.dRanges[YCOORD][0] <= 0.0 && _pInfo.dRanges[YCOORD][1] >= 0.0)
                {
                    if (_pInfo.dRanges[ZCOORD][0] <= 0.0 && _pInfo.dRanges[ZCOORD][1] >= 0.0)
                        _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], 0.0, 0.0);
                    else
                        _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], 0.0, _pInfo.dRanges[ZCOORD][0]);
                }
                else if (_pInfo.dRanges[ZCOORD][0] <= 0.0 && _pInfo.dRanges[ZCOORD][1] >= 0.0)
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[YCOORD][0], 0.0);
                else
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][0]);

                if (!_pData.getSchematic())
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
                if (_pInfo.dRanges[XCOORD][0] <= 0.0 && _pInfo.dRanges[XCOORD][1] >= 0.0)
                {
                    if (_pInfo.sCommand == "dens" || _pInfo.sCommand == "density")
                        _graph->SetOrigin(0.0, _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][1]);
                    else
                    _graph->SetOrigin(0.0, _pInfo.dRanges[YCOORD][0]);
                }
                else if (_pInfo.dRanges[YCOORD][0] <= 0.0 && _pInfo.dRanges[YCOORD][1] >= 0.0)
                {
                    if (_pInfo.sCommand == "dens" || _pInfo.sCommand == "density")
                        _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], 0.0, _pInfo.dRanges[ZCOORD][1]);
                    else
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], 0.0);
                }
                else
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[YCOORD][0]);

                if (!_pData.getSchematic())
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
        else if (_pData.getOrigin(XCOORD) != 0.0 || _pData.getOrigin(YCOORD) != 0.0 || _pData.getOrigin(ZCOORD) != 0.0)
        {
            if (_pInfo.dRanges[XCOORD][0] <= _pData.getOrigin(XCOORD)
                    && _pInfo.dRanges[XCOORD][1] >= _pData.getOrigin(XCOORD)
                    && _pInfo.dRanges[YCOORD][0] <= _pData.getOrigin(YCOORD)
                    && _pInfo.dRanges[YCOORD][1] >= _pData.getOrigin(YCOORD)
                    && _pInfo.dRanges[ZCOORD][0] <= _pData.getOrigin(ZCOORD)
                    && _pInfo.dRanges[ZCOORD][1] >= _pData.getOrigin(ZCOORD)
                    && _pInfo.nMaxPlotDim > 2
               )
            {
                _graph->SetOrigin(_pData.getOrigin(XCOORD), _pData.getOrigin(YCOORD), _pData.getOrigin(ZCOORD));
                if (!_pData.getSchematic())
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
            else if (_pInfo.dRanges[XCOORD][0] <= _pData.getOrigin(XCOORD)
                     && _pInfo.dRanges[XCOORD][1] >= _pData.getOrigin(XCOORD)
                     && _pInfo.dRanges[YCOORD][0] <= _pData.getOrigin(YCOORD)
                     && _pInfo.dRanges[YCOORD][1] >= _pData.getOrigin(YCOORD)
                     && _pInfo.nMaxPlotDim <= 2
                    )
            {
                _graph->SetOrigin(_pData.getOrigin(XCOORD), _pData.getOrigin(YCOORD));
                if (!_pData.getSchematic())
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
                if (_pInfo.dRanges[XCOORD][0] <= _pData.getOrigin() && _pInfo.dRanges[XCOORD][1] >= _pData.getOrigin())
                {
                    if (_pInfo.dRanges[YCOORD][0] <= _pData.getOrigin(1) && _pInfo.dRanges[YCOORD][1] >= _pData.getOrigin(1))
                        _graph->SetOrigin(_pData.getOrigin(), _pData.getOrigin(1), _pInfo.dRanges[ZCOORD][0]);
                    else if (_pInfo.dRanges[ZCOORD][0] <= _pData.getOrigin(2) && _pInfo.dRanges[ZCOORD][1] >= _pData.getOrigin(2))
                        _graph->SetOrigin(_pData.getOrigin(), _pInfo.dRanges[YCOORD][0], _pData.getOrigin(2));
                    else
                        _graph->SetOrigin(_pData.getOrigin(), _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][0]);
                }
                else if (_pInfo.dRanges[YCOORD][0] <= _pData.getOrigin(1) && _pInfo.dRanges[YCOORD][1] >= _pData.getOrigin(1))
                {
                    if (_pInfo.dRanges[ZCOORD][0] <= _pData.getOrigin(2) && _pInfo.dRanges[ZCOORD][1] >= _pData.getOrigin(2))
                        _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pData.getOrigin(1), _pData.getOrigin(2));
                    else
                        _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pData.getOrigin(1), _pInfo.dRanges[ZCOORD][0]);
                }
                else if (_pInfo.dRanges[ZCOORD][0] <= _pData.getOrigin(2) && _pInfo.dRanges[ZCOORD][1] >= _pData.getOrigin(2))
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[YCOORD][0], _pData.getOrigin(2));
                else
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][0]);

                if (!_pData.getSchematic())
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
                if (_pInfo.dRanges[XCOORD][0] <= _pData.getOrigin() && _pInfo.dRanges[XCOORD][1] >= _pData.getOrigin())
                {
                    _graph->SetOrigin(_pData.getOrigin(), _pInfo.dRanges[YCOORD][0]);
                }
                else if (_pInfo.dRanges[YCOORD][0] <= _pData.getOrigin(1) && _pInfo.dRanges[YCOORD][1] >= _pData.getOrigin(1))
                {
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pData.getOrigin(1));
                }
                else
                    _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[YCOORD][0]);

                if (!_pData.getSchematic())
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
        else if (_pInfo.dRanges[XCOORD][0] <= 0.0
                 && _pInfo.dRanges[XCOORD][1] >= 0.0
                 && _pInfo.dRanges[YCOORD][0] <= 0.0
                 && _pInfo.dRanges[YCOORD][1] >= 0.0
                 && _pInfo.dRanges[ZCOORD][0] <= 0.0
                 && _pInfo.dRanges[ZCOORD][1] >= 0.0
                 && _pInfo.nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (_pInfo.b2D && sCommand != "dens"))
                )
        {
            _graph->SetOrigin(0.0, 0.0, 0.0);
            if (!_pData.getSchematic())
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
        else if (_pInfo.dRanges[XCOORD][0] <= 0.0
                 && _pInfo.dRanges[XCOORD][1] >= 0.0
                 && _pInfo.dRanges[YCOORD][0] <= 0.0
                 && _pInfo.dRanges[YCOORD][1] >= 0.0
                 && _pInfo.nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(_pInfo.b2D && sCommand != "dens"))
                )
        {
            if (_pInfo.sCommand == "dens" || _pInfo.sCommand == "density")
                _graph->SetOrigin(0.0, 0.0, _pInfo.dRanges[ZCOORD][1]);
            else
                _graph->SetOrigin(0.0, 0.0, 0.0);

            if (!_pData.getSchematic())
                _graph->Axis("AKDTVISO");
            else
            {
                _graph->SetTickLen(1e-20);
                _graph->SetTicks('x', -5, 1);
                _graph->SetTicks('y', -5, 1);
                _graph->Axis("AKDTVISO_");
            }
        }
        else if (_pInfo.nMaxPlotDim > 2) //sCommand.find("3d") != string::npos || (_pInfo.b2D && sCommand != "dens"))
        {
            _graph->SetOrigin(_pInfo.dRanges[XCOORD][0], _pInfo.dRanges[YCOORD][0], _pInfo.dRanges[ZCOORD][0]);
            if (!_pData.getSchematic())
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
            if (!_pData.getSchematic())
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

    if (_pData.getCoords() == PlotData::CARTESIAN)
    {
        if (_pData.getGrid() && !_pInfo.b2DVect && !_pInfo.b3DVect) // Standard-Grid
        {
            if (_pData.getGrid() == 1)
                _graph->Grid("xyzt", _pData.getGridStyle().c_str());
            else if (_pData.getGrid() == 2)
            {
                _graph->Grid("xyzt!", _pData.getGridStyle().c_str());
                _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());
            }
        }
        else if (_pData.getGrid()) // Vektor-Grid
            _graph->Grid("xyzt", _pData.getFineGridStyle().c_str());

        if (_pData.getBox())
        {
            if (!_pData.getSchematic())
                _graph->Box();
            else
                _graph->Box("k", false);
        }

        // --> Achsen beschriften <--
        if (_pData.getAxis()
                && _pData.getBox()
                && (!_pData.getSchematic()
                    || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                    || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                    || findParameter(_pInfo.sPlotParams, "zlabel", '=')))
        {
            _graph->Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), getLabelPosition(XCOORD));
            _graph->Label('y', fromSystemCodePage(_pData.getyLabel()).c_str(), getLabelPosition(YCOORD));
            _graph->Label('z', fromSystemCodePage(_pData.getzLabel()).c_str(), getLabelPosition(ZCOORD));
            //_graph->Label('t', fromSystemCodePage(_pData.getzLabel()).c_str(), 0.0);
        }
        else if (_pData.getAxis()
                 && !_pData.getBox()
                 && (!_pData.getSchematic()
                     || findParameter(_pInfo.sPlotParams, "xlabel", '=')
                     || findParameter(_pInfo.sPlotParams, "ylabel", '=')
                     || findParameter(_pInfo.sPlotParams, "zlabel", '=')))
        {
            _graph->Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), getLabelPosition(XCOORD));
            _graph->Label('y', fromSystemCodePage(_pData.getyLabel()).c_str(), getLabelPosition(YCOORD));
            _graph->Label('z', fromSystemCodePage(_pData.getzLabel()).c_str(), getLabelPosition(ZCOORD));
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
    if (_pData.getCoords() == PlotData::CARTESIAN)
    {
        if (_pData.getBox())
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

        if (_pData.getCoords() >= PlotData::SPHERICAL_PT)
        {
            dCoordPos[RCOORD] = -0.4;
            dCoordPos[THETACOORD] = -0.9;
        }

        switch (_pData.getCoords())
        {
            case PlotData::POLAR_PZ:
                nCoordMap[XCOORD] = PHICOORD;
                nCoordMap[YCOORD] = ZCOORD;
                nCoordMap[ZCOORD] = RCOORD;
                break;
            case PlotData::POLAR_RP:
                nCoordMap[XCOORD] = RCOORD;
                nCoordMap[YCOORD] = PHICOORD;
                nCoordMap[ZCOORD] = ZCOORD;
                break;
            case PlotData::POLAR_RZ:
                nCoordMap[XCOORD] = RCOORD;
                nCoordMap[YCOORD] = ZCOORD;
                nCoordMap[ZCOORD] = PHICOORD;
                break;
            case PlotData::SPHERICAL_PT:
                nCoordMap[XCOORD] = PHICOORD;
                nCoordMap[YCOORD] = THETACOORD;
                nCoordMap[ZCOORD] = RCOORD;
                break;
            case PlotData::SPHERICAL_RP:
                nCoordMap[XCOORD] = RCOORD;
                nCoordMap[YCOORD] = PHICOORD;
                nCoordMap[ZCOORD] = THETACOORD;
                break;
            case PlotData::SPHERICAL_RT:
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
    if (!_pData.getSchematic())
        _graph->Axis(); //U
    else
    {
        _graph->SetTickLen(1e-20);
        _graph->Axis("_");
    }

    if (_pData.getBox() || _pData.getCoords() != PlotData::CARTESIAN)
        _graph->Box();

    if (_pData.getGrid() == 1)
        _graph->Grid("xyzt", _pData.getGridStyle().c_str());
    else if (_pData.getGrid() == 2)
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

    for (int i = 0; i < nNum; i++)
    {
        sColours += _pInfo.sLineStyles[*_pInfo.nStyle];

        if (i + 1 < nNum)
            (*_pInfo.nStyle)++;

        if (*_pInfo.nStyle >= _pInfo.nStyleMax)
            *_pInfo.nStyle = 0;
    }

    return sColours;
}


/////////////////////////////////////////////////
/// \brief This member function checks, whether
/// the selected subplot position is still empty.
///
/// \param nMultiPlot[2] unsigned int
/// \param nSubPlotMap unsigned int&
/// \param nPlotPos unsigned int
/// \param nCols unsigned int
/// \param nLines unsigned int
/// \return bool
///
/////////////////////////////////////////////////
bool Plot::checkMultiPlotArray(unsigned int nMultiPlot[2], unsigned int& nSubPlotMap, unsigned int nPlotPos, unsigned int nCols, unsigned int nLines)
{
    // cols, lines
    if (nPlotPos + nCols - 1 >= (nPlotPos / nMultiPlot[0] + 1)*nMultiPlot[0])
        return false;

    if (nPlotPos / nMultiPlot[0] + nLines - 1 >= nMultiPlot[1])
        return false;

    unsigned int nCol0, nLine0, pos;
    nCol0 = nPlotPos % nMultiPlot[0];
    nLine0 = nPlotPos / nMultiPlot[0];

    for (unsigned int i = 0; i < nLines; i++)
    {
        for (unsigned int j = 0; j < nCols; j++)
        {
            pos = 1;
            pos <<= ((nCol0 + j) + nMultiPlot[0] * (nLine0 + i));

            if (pos & nSubPlotMap)
                return false;
        }
    }

    for (unsigned int i = 0; i < nLines; i++)
    {
        for (unsigned int j = 0; j < nCols; j++)
        {
            pos = 1;
            pos <<= ((nCol0 + j) + nMultiPlot[0] * (nLine0 + i));
            nSubPlotMap |= pos;
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function does the
/// "undefined data point" magic, where the range
/// of the plot is chosen so that "infinity"
/// values are ignored.
///
/// \param nCol int
/// \param dMin double&
/// \param dMax double&
/// \return void
///
/////////////////////////////////////////////////
void Plot::weightedRange(int nCol, double& dMin, double& dMax)
{
    if (log(dMax - dMin) > 5)
    {
        const double dPercentage = 0.975;
        const double dSinglePercentageValue = 0.99;
        double dSinglePercentageUse;

        if (nCol == PlotData::ALLRANGES)
            dSinglePercentageUse = dSinglePercentageValue;
        else
            dSinglePercentageUse = dPercentage;

        if (log(fabs(dMin)) <= 1)
        {
            vector<double> vRanges = _pData.getWeightedRanges(nCol, 1.0, dSinglePercentageUse);
            dMin = vRanges[0];
            dMax = vRanges[1];
        }
        else if (log(fabs(dMax)) <= 1)
        {
            vector<double> vRanges = _pData.getWeightedRanges(nCol, dSinglePercentageUse, 1.0);
            dMin = vRanges[0];
            dMax = vRanges[1];
        }
        else
        {
            vector<double> vRanges = _pData.getWeightedRanges(nCol, dPercentage, dPercentage);
            dMin = vRanges[0];
            dMax = vRanges[1];
        }
    }
}





