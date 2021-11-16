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

#ifndef PLOTTING_HPP
#define PLOTTING_HPP

#include <mgl2/mgl.h>
#include <string>
#include <vector>

#include "../datamanagement/memorymanager.hpp"
#include "../ParserLib/muParser.h"
#include "../settings.hpp"
#include "../maths/define.hpp"
#include "plotdata.hpp"
#include "plotinfo.hpp"
#include "graph_helper.hpp"

class DataAccessParser;
struct PlotAsset;

using namespace std;
using namespace mu;

void createPlot(string& sCmd, MemoryManager& _data, Parser& _parser, Settings& _option, FunctionDefinitionManager& _functions, PlotData& _pData);


/////////////////////////////////////////////////
/// \brief This class handles the complete
/// plotting process.
/////////////////////////////////////////////////
class Plot
{
    private:
        std::vector<PlotAsset> m_assets;
        IntervalSet dataRanges;
        IntervalSet secDataRanges;
        mglData _mAxisVals[3];
        //std::vector<std::vector<mglData>> v_mDataPlots;
        bool bOutputDesired;

        string sLabels;
        string sDataLabels;
        string sFunc;

        MemoryManager& _data;
        Parser& _parser;
        Settings& _option;
        FunctionDefinitionManager& _functions;
        PlotData& _pData;

        PlotInfo _pInfo;
        mglGraph* _graph;

        Plot(const Plot&) = delete;
        Plot& operator=(const Plot&) = delete;

    protected:
        void determinePlottingDimensions(const string& sPlotCommand);
        size_t createSubPlotSet(string& sOutputName, bool& bAnimateVar, vector<string>& vPlotCompose, size_t nSubPlotStart, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap);
        void applyPlotSizeAndQualitySettings();
        bool createPlotOrAnimation(int& nStyle, size_t nPlotCompose, size_t nPlotComposeSize, size_t& nLegends, bool bNewSubPlot, bool bAnimateVar, vector<string>& vDrawVector, const vector<string>& vDataPlots, vector<short>& vType, int nFunctions, const string& sDataAxisBinds, const string& sOutputName);
        void create2dPlot(vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
        bool plot2d(mglData& _mData, mglData& _mMaskData, mglData* _mAxisVals, mglData& _mContVec);
        void createStdPlot(vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
        bool plotstd(mglData& _mData, mglData& _mAxisVals, mglData _mData2[2], const short nType);
        void create3dPlot();
        void create3dVect();
        void create2dVect();
        void create2dDrawing(vector<string>& vDrawVector, int& nFunctions);
        void create3dDrawing(vector<string>& vDrawVector, int& nFunctions);
        void createStd3dPlot(vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
        bool plotstd3d(mglData _mData[3], mglData _mData2[3], const short nType);
        bool checkMultiPlotArray(unsigned int nMultiPlot[2], unsigned int& nSubPlotMap, unsigned int nPlotPos, unsigned int nCols, unsigned int nLines);
        long getNN(const mglData& _mData);
        void evaluatePlotParamString();
        void filename(size_t nPlotComposeSize, size_t nPlotCompose);
        void setStyles();
        string expandStyleForCurveArray(const string& sCurrentStyle, bool expand);
        void evaluateSubplot(size_t& nLegends, string& sCmd, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap);
        void displayMessage(bool bAnimateVar);
        std::vector<std::string> evaluateDataPlots(vector<short>& vType, string& sAxisBinds, string& sDataAxisBinds);
        void extractDataValues(const std::vector<std::string>& vDataPlots, const std::string& sDataAxisBinds);
        void getValuesFromData(DataAccessParser& _accessParser, size_t i, const std::string& sDataAxisBinds, bool openEnd);
        void createDataLegends();
        string constructDataLegendElement(string& sColumnIndices, const string& sTableName);
        void calculateDataRanges(const string& sDataAxisBinds, int i, int l, const VectorIndex& _vLine, size_t numberofColNodes = 2);
        size_t countValidElements(const mglData& _mData);
        void separateLegends();
        void prepareMemory(int nFunctions);
        void defaultRanges(size_t nPlotCompose, bool bNewSubPlot);
        int fillData(value_type* vResults, double dt_max, int t_animate, int nFunctions);
        void fitPlotRanges(size_t nPlotCompose, bool bNewSubPlot);
        void clearData();
        void passRangesToGraph();
        void applyColorbar();
        void applyLighting();
        void applyGrid();
        double getLabelPosition(int nCoord);
        mglPoint createMglPoint(int nCoords, double r, double phi, double theta, bool b3D = false);
        void weightedRange(int nCol, double& dMin, double& dMax);
        void setLogScale(bool bzLogscale);
        void directionalLight(double dPhi, double dTheta, int nId, char cColor = 'w', double dBrightness = 0.5);
        string getLegendStyle(const string& sLegend);
        mglPoint CalcCutBox(double dPhi, int nEdge = 0, int nCoords = 0, bool b3D = false);
        double getProjBackground(double dPhi, int nEdge = 0);
        mglData fmod(const mglData& _mData, double dDenominator);
        void CoordSettings();
        string CoordFunc(const std::string& sFuncDef, double dPhiScale = 1.0, double dThetaScale = 1.0);
        string composeColoursForBarChart(long int nNum);

        inline double validize(std::complex<double> d)
        {
            return isValidValue(d.real()) ? d.real() : NAN;
        }

    public:
        Plot(string& sCmd, MemoryManager& __data, Parser& __parser, Settings& __option, FunctionDefinitionManager& __functions, PlotData& __pData);
        ~Plot();
        inline GraphHelper* createGraphHelper()
            {
                GraphHelper* _helper = new GraphHelper(_graph, _pData);
                _graph = nullptr;
                return _helper;
            }
};


#endif
