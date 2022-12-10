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


// --> CLASS: PLOTDATA <--

#ifndef PLOTDATA_HPP
#define PLOTDATA_HPP

#include <string>
#include <cmath>
#include <vector>

#include "../ui/error.hpp"
#include "../utils/tools.hpp"
#include "../structures.hpp"
#include "../interval.hpp"
#include "plotdef.hpp"

extern const char* SECAXIS_DEFAULT_COLOR;

/////////////////////////////////////////////////
/// \brief This class contains all the plot
/// settings usable by the plotting algorithm.
/////////////////////////////////////////////////
class PlotData : public FileSystem
{
    public:
        enum RangeType
        {
            ALLRANGES = -1,
            ONLYLEFT = -2,
            ONLYRIGHT = -3,
            COLSTART = 0
        };

        enum ParamType
        {
            ALL = 0,
            LOCAL = 1,
            GLOBAL = 2,
            SUPERGLOBAL = 4
        };

        enum LogicalPlotSetting
        {
            LOG_ALLHIGHRES,
            LOG_ALPHA,
            LOG_ALPHAMASK,
            LOG_ANIMATE,
            LOG_AREA,
            LOG_BOX,
            LOG_BOXPLOT,
            LOG_CLOUDPLOT,
            LOG_COLORBAR,
            LOG_COLORMASK,
            LOG_CONNECTPOINTS,
            LOG_CONTFILLED,
            LOG_CONTLABELS,
            LOG_CONTPROJ,
            LOG_CRUST,
            LOG_CUTBOX,
            LOG_DRAWPOINTS,
            LOG_FIXEDLENGTH,
            LOG_FLOW,
            LOG_INTERPOLATE,
            LOG_XERROR,
            LOG_YERROR,
            LOG_OPENIMAGE,
            LOG_ORTHOPROJECT,
            LOG_PARAMETRIC,
            LOG_PIPE,
            LOG_REGION,
            LOG_SCHEMATIC,
            LOG_SILENTMODE,
            LOG_STEPPLOT,
            LOG_TABLE,
            LOG_SETTING_SIZE
        };

        enum IntPlotSetting
        {
            INT_ANIMATESAMPLES,
            INT_AXIS,
            INT_COMPLEXMODE,
            INT_CONTLINES,
            INT_COORDS,
            INT_GRID,
            INT_HIGHRESLEVEL,
            INT_LEGENDPOSITION,
            INT_LEGENDSTYLE,
            INT_LIGHTING,
            INT_MARKS,
            INT_SAMPLES,
            INT_SIZE_X,
            INT_SIZE_Y,
            INT_SETTING_SIZE
        };

        enum FloatPlotSetting
        {
            FLOAT_ALPHAVAL,
            FLOAT_ASPECT,
            FLOAT_BARS,
            FLOAT_HBARS,
            FLOAT_PERSPECTIVE,
            FLOAT_TEXTSIZE,
            FLOAT_SETTING_SIZE
        };

        enum StringPlotSetting
        {
            STR_AXISBIND,
            STR_BACKGROUND,
            STR_BACKGROUNDCOLORSCHEME,
            STR_COLORS,
            STR_COLORSCHEME,
            STR_COLORSCHEMELIGHT,
            STR_COLORSCHEMEMEDIUM,
            STR_COMPOSEDTITLE,
            STR_CONTCOLORS,
            STR_CONTGREYS,
            STR_FILENAME,
            STR_FONTSTYLE,
            STR_GREYS,
            STR_GRIDSTYLE,
            STR_LINESIZES,
            STR_LINESTYLES,
            STR_LINESTYLESGREY,
            STR_PLOTTITLE,
            STR_POINTSTYLES,
            STR_SETTING_SIZE
        };

    private:
        IntervalSet ranges;
        int nRanges;
        int nRequestedLayers;
        bool bRanges[4];
        bool bMirror[4];
        bool bDefaultAxisLabels[3];
        bool bLogscale[4];
        std::string sAxisLabels[3];
        std::string sTickTemplate[4];
        std::string sCustomTicks[4];
        double dRotateAngles[2];
        double dAxisScale[4];
        double dOrigin[3];
        unsigned short nSlices[3];
        int nTargetGUI[2];

        bool logicalSettings[LOG_SETTING_SIZE];
        int intSettings[INT_SETTING_SIZE];
        double floatSettings[FLOAT_SETTING_SIZE];
        std::string stringSettings[STR_SETTING_SIZE];

        std::vector<Line> _lHlines;
        std::vector<Line> _lVLines;
        Axis _AddAxes[2];
        TimeAxis _timeAxes[4];

        PlotData(const PlotData&) = delete;
        PlotData& operator=(const PlotData&) = delete;

        void replaceControlChars(std::string& sString);
        std::string removeSurroundingQuotationMarks(const std::string& sString);
        void rangeByPercentage(double* dData, size_t nLength, double dLowerPercentage, double dUpperPercentage, std::vector<double>& vRanges);

    public:
        PlotData();

        void setParams(const std::string& __sCmd, int nType = ALL);
        std::string getParams(bool asstr = false) const;
        std::string getAxisLabel(size_t axis) const;
        void setSamples(int _nSamples);
        void setFileName(std::string _sFileName);
        void reset();
        void deleteData(bool bGraphFinished = false);

        inline void setGlobalComposeParams(const std::string& __sCmd)
        {
            return setParams(__sCmd, GLOBAL | SUPERGLOBAL);
        }

        inline void setLocalComposeParams(const std::string& __sCmd)
        {
            return setParams(__sCmd, LOCAL);
        }

        IntervalSet& getRanges()
        {
            return ranges;
        }

        bool getSettings(LogicalPlotSetting setting) const
        {
            return logicalSettings[setting];
        }

        int getSettings(IntPlotSetting setting) const
        {
            return intSettings[setting];
        }

        double getSettings(FloatPlotSetting setting) const
        {
            return floatSettings[setting];
        }

        std::string getSettings(StringPlotSetting setting) const
        {
            return stringSettings[setting];
        }

        inline bool getRangeSetting(int i = 0) const
        {
            if (i < 3 && i >= 0)
                return bRanges[i];
            else
                return false;
        }

        inline bool getInvertion(int i = 0) const
        {
            if (i < 4 && i >= 0)
                return bMirror[i];
            else
                return false;
        }

        inline double getAxisScale(int i = 0) const
        {
            if (i >= 0 && i < 4)
                return dAxisScale[i];
            else
                return 1.0;
        }

        inline void setFont(const std::string& Font)
        {
            stringSettings[STR_FONTSTYLE] = Font;
            return;
        }

        inline std::string getAxisbind(unsigned int i) const
        {
            if (2*i+1 < stringSettings[STR_AXISBIND].length())
                return stringSettings[STR_AXISBIND].substr(2*i,2);
            else
                return "lb";
        }

        inline int getGivenRanges() const
        {
            return nRanges;
        }

        inline bool getLogscale(size_t i) const
        {
            return i < 4 ? bLogscale[i] : false;
        }

        inline double getRotateAngle(int _i = 0) const
        {
            return dRotateAngles[_i];
        }

        inline int getAnimateSamples() const
        {
            if (logicalSettings[LOG_ANIMATE])
                return intSettings[INT_ANIMATESAMPLES]-1;
            else
                return 0;
        }

        inline double getOrigin(int nDir = 0) const
        {
            if (nDir >= 0 && nDir < 3)
                return dOrigin[nDir];
            else
                return 0.0;
        }

        inline unsigned short getSlices(unsigned int nDir = 0) const
        {
            if (nDir < 3)
                return nSlices[nDir];
            else
                return 1;
        }

        inline std::string getColorScheme(const std::string& _sAddOpt = "") const
        {
            return (stringSettings[STR_COLORSCHEME]+_sAddOpt);
        }

        inline std::string getColorSchemeMedium(const std::string& _sAddOpt = "") const
        {
            return (stringSettings[STR_COLORSCHEMEMEDIUM]+_sAddOpt);
        }

        inline std::string getColorSchemeLight(const std::string& _sAddOpt = "") const
        {
            return (stringSettings[STR_COLORSCHEMELIGHT]+_sAddOpt);
        }

        inline std::string getColors() const
        {
            return (stringSettings[STR_COLORSCHEME] == "wk" || stringSettings[STR_COLORSCHEME] == "kw")
                ? stringSettings[STR_GREYS]
                : stringSettings[STR_COLORS];
        }

        inline std::string getContColors() const
        {
            return (stringSettings[STR_COLORSCHEME] == "wk" || stringSettings[STR_COLORSCHEME] == "kw")
                ? stringSettings[STR_CONTGREYS]
                : stringSettings[STR_CONTCOLORS];
        }

        inline std::string getLineStyles() const
        {
            return (stringSettings[STR_COLORSCHEME] == "wk" || stringSettings[STR_COLORSCHEME] == "kw")
                ? stringSettings[STR_LINESTYLESGREY]
                : stringSettings[STR_LINESTYLES];
        }

        inline std::string getGridStyle() const
        {
            return stringSettings[STR_GRIDSTYLE].substr(0,3);
        }

        inline std::string getFineGridStyle() const
        {
            return stringSettings[STR_GRIDSTYLE].substr(3);
        }

        inline const std::vector<Line>& getHLines() const
        {
            return _lHlines;
        }

        inline const std::vector<Line>& getVLines() const
        {
            return _lVLines;
        }

        inline Axis getAddAxis(unsigned int i = 0) const
        {
            Axis _Axis;
            _Axis.sLabel = "";
            _Axis.sStyle = SECAXIS_DEFAULT_COLOR;
            _Axis.ivl.reset(NAN, NAN);

            if (i < 2)
                return _AddAxes[i];
            else
                return _Axis;
        }

        inline void setAddAxis(unsigned int i, const Interval& _ivl)
        {
            if (i < 2
                && mu::isnan(_AddAxes[i].ivl.front())
                && mu::isnan(_AddAxes[i].ivl.back())
                && !mu::isnan(_ivl.front())
                && !mu::isnan(_ivl.back()))
            {
                _AddAxes[i].ivl = _ivl;

                if (!i && !_AddAxes[i].sLabel.length())
                    _AddAxes[i].sLabel = "\\i x";
                else if (!_AddAxes[i].sLabel.length())
                    _AddAxes[i].sLabel = "\\i y";
            }
        }

        inline TimeAxis getTimeAxis(unsigned int i = 0) const
        {
            TimeAxis axis;

            if (i < 4)
                return _timeAxes[i];

            return axis;
        }

        inline const int* getTargetGUI() const
        {
            return nTargetGUI;
        }

        inline std::string getTickTemplate(int nAxis = 0) const
        {
            if (nAxis >= 0 && nAxis < 4)
                return sTickTemplate[nAxis];
            else
                return "";
        }

        inline std::string getCustomTick(int nAxis = 0) const
        {
            if (nAxis >= 0 && nAxis < 4)
                return sCustomTicks[nAxis];
            else
                return "";
        }


};

#endif
