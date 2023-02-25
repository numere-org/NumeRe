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
#include "plotasset.hpp"
#include "graph_helper.hpp"

class DataAccessParser;

void createPlot(std::string& sCmd, MemoryManager& _data, mu::Parser& _parser, Settings& _option, FunctionDefinitionManager& _functions, PlotData& _pData);




/////////////////////////////////////////////////
/// \brief This class handles the complete
/// plotting process.
/////////////////////////////////////////////////
class Plot
{
    private:
        PlotAssetManager m_manager;
        std::vector<short> m_types;
        IntervalSet dataRanges;
        IntervalSet secDataRanges;
        bool bOutputDesired;
        std::string sFunc;
        std::string sOutputName;
        std::string sCurrentExpr;
        size_t nLegends;

        MemoryManager& _data;
        mu::Parser& _parser;
        Settings& _option;
        FunctionDefinitionManager& _functions;
        PlotData& _pData;

        PlotInfo _pInfo;
        mglGraph* _graph;

        Plot(const Plot&) = delete;
        Plot& operator=(const Plot&) = delete;

    protected:
        void determinePlottingDimensions(const std::string& sPlotCommand);
        size_t createSubPlotSet(bool& bAnimateVar, std::vector<std::string>& vPlotCompose, size_t nSubPlotStart, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap);
        void applyPlotSizeAndQualitySettings();
        bool createPlotOrAnimation(size_t nPlotCompose, size_t nPlotComposeSize, bool bNewSubPlot, bool bAnimateVar, std::vector<std::string>& vDrawVector, const std::vector<std::string>& vDataPlots);
        void create2dPlot(size_t nPlotCompose, size_t nPlotComposeSize);
        bool plot2d(mglData& _mData, mglData& _mData2, mglData* _mAxisVals, mglData& _mContVec);
        void createStdPlot(size_t nPlotCompose, size_t nPlotComposeSize);
        bool plotstd(mglData& _mData, mglData& _mAxisVals, mglData _mData2[3], const short nType);
        void create3dPlot();
        void create3dVect();
        void create2dVect();
        void create2dDrawing(std::vector<std::string>& vDrawVector);
        void create3dDrawing(std::vector<std::string>& vDrawVector);
        void createStd3dPlot(size_t nPlotCompose, size_t nPlotComposeSize);
        bool plotstd3d(mglData _mData[3], mglData _mData2[3], const short nType);
        bool checkMultiPlotArray(unsigned int nMultiPlot[2], unsigned int& nSubPlotMap, unsigned int nPlotPos, unsigned int nCols, unsigned int nLines);
        long getNN(const mglData& _mData);
        void filename(size_t nPlotComposeSize, size_t nPlotCompose);
        void setStyles();
        std::string expandStyleForCurveArray(const std::string& sCurrentStyle, bool expand);
        void evaluateSubplot(std::string& sCmd, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap);
        void displayMessage(bool bAnimateVar);
        std::vector<std::string> separateFunctionsAndData();
        void extractDataValues(const std::vector<std::string>& vDataPlots);
        void createDataLegends();
        std::string constructDataLegendElement(std::string& sColumnIndices, const std::string& sTableName);
        size_t countValidElements(const mglData& _mData);
        void prepareMemory();
        void defaultRanges(size_t nPlotCompose, bool bNewSubPlot);
        void fillData(double dt_max, int t_animate);
        void fitPlotRanges(size_t nPlotCompose, bool bNewSubPlot);
        void clearData();
        void passRangesToGraph();
        void applyColorbar();
        void applyLighting();
        void applyGrid();
        double getLabelPosition(int nCoord);
        mglPoint createMglPoint(int nCoords, double r, double phi, double theta, bool b3D = false);
        void setLogScale(bool bzLogscale);
        void directionalLight(double dPhi, double dTheta, int nId, char cColor = 'w', double dBrightness = 0.5);
        std::string getLegendStyle(const std::string& sLegend);
        mglPoint CalcCutBox(double dPhi, int nEdge = 0, int nCoords = 0, bool b3D = false);
        double getProjBackground(double dPhi, int nEdge = 0);
        mglData fmod(const mglData& _mData, double dDenominator);
        void CoordSettings();
        std::string CoordFunc(const std::string& sFuncDef, double dPhiScale = 1.0, double dThetaScale = 1.0);
        std::string composeColoursForBarChart(long int nNum);

    public:
        Plot(std::string& sCmd, MemoryManager& __data, mu::Parser& __parser, Settings& __option, FunctionDefinitionManager& __functions, PlotData& __pData);
        ~Plot();
        inline GraphHelper* createGraphHelper()
            {
                GraphHelper* _helper = new GraphHelper(_graph, _pData);
                _graph = nullptr;
                return _helper;
            }
};


#endif


