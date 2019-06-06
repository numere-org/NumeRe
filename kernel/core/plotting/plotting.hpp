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

#include <mgl2/mgl.h>
#include <string>
#include <vector>

#include "../datamanagement/datafile.hpp"
#include "../ParserLib/muParser.h"
#include "../settings.hpp"
#include "../maths/define.hpp"
#include "plotdata.hpp"
#include "plotinfo.hpp"
#include "graph_helper.hpp"

#ifndef PLOTTING_HPP
#define PLOTTING_HPP

using namespace std;
using namespace mu;

class Plot
{
    private:
        mglData** _mDataPlots;
        mglData _mAxisVals[3];
        int* nDataDim;
        int nDataPlots;
        bool bOutputDesired;

        string sLabels;
        string sDataLabels;
        string sFunc;

        PlotInfo _pInfo;
        mglGraph* _graph;

    protected:
        void determinePlottingDimensions(const string& sPlotCommand);
        size_t createSubPlotSet(PlotData& _pData, Datafile& _data, Parser& _parser, Define& _functions, Settings& _option, string& sOutputName, bool& bAnimateVar, vector<string>& vPlotCompose, size_t nSubPlotStart, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap);
        void applyPlotSizeAndQualitySettings(PlotData& _pData);
        bool createPlotOrAnimation(PlotData& _pData, Datafile& _data, Parser& _parser, const Settings& _option, int& nStyle, size_t nPlotCompose, size_t nPlotComposeSize, size_t& nLegends, bool bNewSubPlot, bool bAnimateVar, vector<string>& vDrawVector, vector<short>& vType, int nFunctions, double dDataRanges[3][2], const string& sOutputName);
        void create2dPlot(PlotData& _pData, Datafile& _data, Parser& _parser, const Settings& _option, vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
        bool plot2d(PlotData& _pData, mglData& _mData, mglData& _mMaskData, mglData* _mAxisVals, mglData& _mContVec, const Settings& _option);
        void createStdPlot(PlotData& _pData, Datafile& _data, Parser& _parser, const Settings& _option, vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
        bool plotstd(PlotData& _pData, mglData& _mData, mglData& _mAxisVals, mglData _mData2[2], const short nType);
        void create3dPlot(PlotData& _pData, const Settings& _option);
        void create3dVect(PlotData& _pData);
        void create2dVect(PlotData& _pData);
        void create2dDrawing(Parser& _parser, Datafile& _data, const Settings& _option, vector<string>& vDrawVector, value_type* vResults, int& nFunctions);
        void create3dDrawing(Parser& _parser, Datafile& _data, const Settings& _option, vector<string>& vDrawVector, value_type* vResults, int& nFunctions);
        void createStd3dPlot(PlotData& _pData, Datafile& _data, Parser& _parser, const Settings& _option, vector<short>& vType, int& nStyle, size_t& nLegends, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
        bool plotstd3d(PlotData& _pData, mglData _mData[3], mglData _mData2[3], const short nType);
        bool checkMultiPlotArray(unsigned int nMultiPlot[2], unsigned int& nSubPlotMap, unsigned int nPlotPos, unsigned int nCols, unsigned int nLines);
        long getNN(const mglData& _mData);
        void evaluatePlotParamString(Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
        void filename(PlotData& _pData, Datafile& _data, Parser& _parser, Settings& _option, size_t nPlotComposeSize, size_t nPlotCompose);
        void setStyles(PlotData& _pData);
        string expandStyleForCurveArray(const string& sCurrentStyle, bool expand);
        void evaluateSubplot(PlotData& _pData, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option, size_t& nLegends, string& sCmd, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap);
        void displayMessage(PlotData& _pData, const Settings& _option);
        void evaluateDataPlots(PlotData& _pData, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option, vector<short>& vType, string& sDataPlots, string& sAxisBinds, string& sDataAxisBinds, double dDataRanges[3][2], double dSecDataRanges[2][2]);
        void createDataLegends(PlotData& _pData, Parser& _parser, Datafile& _data, const Settings& _option);
        string constructDataLegendElement(Parser& _parser, Datafile& _data, const PlotData& _pData, const string& sColumnIndices, const string& sTableName);
        void calculateDataRanges(PlotData& _pData, const string& sDataAxisBinds, double dDataRanges[3][2], double dSecDataRanges[2][2], int i, int l, const VectorIndex& _vLine, size_t numberofColNodes = 2);
        size_t countValidElements(const mglData& _mData);
        void separateLegends();
        void prepareMemory(PlotData& _pData, const string& sFunc, int nFunctions);
        void defaultRanges(PlotData& _pData, double dDataRanges[3][2], double dSecDataRanges[2][2], size_t nPlotCompose, bool bNewSubPlot);
        int fillData(PlotData& _pData, Parser& _parser, const string& sFunc, value_type* vResults, double dt_max, int t_animate, int nFunctions);
        void fitPlotRanges(PlotData& _pData, const string& sFunc, double dDataRanges[3][2], size_t nPlotCompose, bool bNewSubPlot);
        void clearData();
        void passRangesToGraph(PlotData& _pData, const string& sFunc, double dDataRanges[3][2]);
        void applyColorbar(PlotData& _pData);
        void applyLighting(PlotData& _pData);
        void applyGrid(const PlotData& _pData);
        double getLabelPosition(const PlotData& _pData, int nCoord);
        mglPoint createMglPoint(int nCoords, double r, double phi, double theta, bool b3D = false);
        void weightedRange(int nCol, double& dMin, double& dMax, PlotData& _pData);
        void setLogScale(PlotData& _pData, bool bzLogscale);
        void directionalLight(double dPhi, double dTheta, int nId, char cColor = 'w', double dBrightness = 0.5);
        string getLegendStyle(const string& sLegend, const PlotData& _pData);
        mglPoint CalcCutBox(double dPhi, int nEdge = 0, int nCoords = 0, bool b3D = false);
        double getProjBackground(double dPhi, int nEdge = 0);
        mglData fmod(const mglData& _mData, double dDenominator);
        void CoordSettings(const PlotData& _pData);
        string CoordFunc(const string& sFunc, double dPhiScale = 1.0, double dThetaScale = 1.0);
        string composeColoursForBarChart(long int nNum);

        inline double validize(double d)
        {
            return isValidValue(d) ? d : NAN;
        }

    public:
        Plot(string& sCmd, Datafile& _data, Parser& _parser, Settings& _option, Define& _functions, PlotData& _pData);
        ~Plot();
        inline GraphHelper* createGraphHelper(const PlotData& _pData)
            {
                GraphHelper* _helper = new GraphHelper(_graph, _pData);
                _graph = nullptr;
                return _helper;
            }
};


#endif
