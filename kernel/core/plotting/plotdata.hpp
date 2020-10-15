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
//#include <mgl2/fltk.h>

#include "../ui/error.hpp"
#include "../utils/tools.hpp"
#include "../structures.hpp"
#include "../ParserLib/muParser.h"


using namespace std;
using namespace mu;
//extern mglGraph _fontData;
/* --> Prototyp einer Funktion, die nicht per Header eingebunden werden kann
 *     (da dieser Header bereits in den entsprechenden Header eingebunden
 *     wird) <--
 */
bool isNotEmptyExpression(const string& sExpr);

class PlotData : public FileSystem  // CHILD von Filesystem
{
    private:
        double*** dPlotData;        // Datenspeicher fuer die berechneten Plotpunkte (dreidimensionaler Speicher!)
        double dRanges[3][2];       // Speicher fuer die Plot-Intervalle
        double dOrigin[3];
        unsigned short nSlices[3];
        int nRows;                  // Dimensions-Speicher-Int: Spalten
        int nLines;                 // Dimensions-Speicher-Int: Zeilen
        int nLayers;                // Dimensions-Speicher-Int: Ebenen (Jede Ebene fuer einen neue Funktion)
        int nSamples;               // Anzahl der Datenpunkte fuer 1D-Plot, quadratisch fuer 2D-Plots (mesh, grad, cont, surf)
        int nRanges;                // Anzahl der angegeben Plot-Intervalle (wichtig fuer die Berechnung der fehlenden)
        bool bRanges[3];
        bool bMirror[3];
        string sFileName;           // Ausgabe-Dateiname
        string sAxisLabels[3];      // Achsenbeschriftungen fuer alle drei Achsen
        bool bDefaultAxisLabels[3];      // Achsenbeschriftungen fuer alle drei Achsen
        string sPlotTitle;          // Titelzeile fuer den Plot
        string sComposedTitle;
        string sFontStyle;
        string sAxisBind;
        string sFunctionAxisBind;
        double dMin;                // Globales Minimum
        double dMax;                // GLobales Maximum
        double dMaximum;            // Norm-Maximum

        int nGrid;                  // Gitter ein/aus
        int nLighting;             // Lichteffekt ein/aus
        bool bAlpha;                // Transparenz ein/aus
        double dAlphaVal;
        bool bAxis;                 // Achsen ein/aus
        bool bBox;                  // Umschliessende Box ein/aus
        bool bContProj;             // Projektion der Konturlinien auf untere Ebene ein/aus
        bool bContLabels;           // Darstellen der Konturlinien-Werte ein/aus
        bool bContFilled;           // Gefuellte Konturlinien
        size_t nContLines;
        bool bxError;               // Fehlerbalken in x-Richtung ein/aus
        bool byError;               // Fehlerbalken in y-Richtung ein/aus
        bool bConnectPoints;        // Verbindende Punkte zwischen Datenpunkten zeichnen
        bool bDrawPoints;           // Berechnete Punkte auf die Linie zeichnen
        bool bLogscale[4];          // Logscales in die drei Raumrichtungen
        bool bOpenImage;            // Erzeugtes Bild in einem externen Viewer oeffnen (falls vorhanden)
        bool bInterpolate;          // Falls die Zahl der Datenpunkte ausreicht: Als Funktion interpolieren
        bool bSilentMode;           // Modus ohne eine Bildschirmausgabe
        bool bAnimate;              // Soll eine Animation erzeugt werden?
        bool bCutBox;               // Soll ein Teil des 3D-Plots ausgeschnitten werden?
        bool bFlow;                 // Soll ein Flow- anstatt eines Vectorplots angezeigt werden?
        bool bPipe;                 // Soll ein Pipe- anstatt eines Vectorplots angezeigt werden?
        bool bFixedLength;          // Vektorpfeile mit fester Laenge?
        bool bColorbar;             // Colorbar anzeigen?
        bool bOrthoProject;         // Orthogonal-Projektionen berechnen?
        bool bArea;                 // Einen Area-Plot anstatt eines gewoehnlichen Plots zeichnen?
        double dBars;                 // Bar-Plot statt gewoehnlichen Plot zeichnen?
        bool bColorMask;            // Ein zweites 2D-Array als Farbmaske?
        bool bAlphaMask;            // Ein zweites 2D-Array als Alphamaske?
        bool bSchematic;            // Einen schematischen Plot erzeugen?
        bool bCloudPlot;            // Cloud-Plot?
        bool bRegion;
        bool bStepPlot;
        bool bBoxPlot;
        bool bCrust;
        double dHBars;
        double dPerspective;        // optische Perspective?
        string sColorScheme;        // Colorscheme
        string sColorSchemeMedium;  // Colorscheme fuer Konturlinien
        string sColorSchemeLight;   // Colorscheme fuer z.B. grad
        string sBackgroundColorScheme; // Colorscheme fuer geladene Hintergrundbilder
        string sBackground;         // Bild als Hintergrund?
        int nAnimateSamples;        // Zahl der Animations-Samples
        int nHighResLevel;          // Qualitaetsstufen der Ausgabe
        bool bAllHighRes;           // Ueberschreiben des Verhaltens von bHighRes
        int nCoords;                // Koordinatenset: 0 = cartesian, 1 = polar, 2 = spherical
        double dtParam[2];          // t-Parameterintervall fuer 3D-Bahnkurven
        double dColorRange[2];      // Colorrange
        double dRotateAngles[2];    // Rotatationswinkel fuer 3D-Plots
        double dAspect;             // Seitenverhaeltnis des Plots
        double dAxisScale[4];
        int nMarks;                 // Groesse der Marks
        double dTextsize;
        string sColors;
        string sGreys;
        string sPointStyles;
        string sLineStyles;
        string sLineStylesGrey;
        string sContColors;
        string sContGreys;
        string sLineSizes;
        string sTickTemplate[4];
        string sCustomTicks[4];
        string sGridStyle;
        int nLegendstyle;
        vector<Line> _lHlines;
        vector<Line> _lVLines;
        Axis _AddAxes[2];
        TimeAxis _timeAxes[4];
        int nRequestedLayers;
        int nLegendPosition;

        PlotData(const PlotData&) = delete;
        PlotData& operator=(const PlotData&) = delete;

        inline bool checkColorChars(const string& sColorSet)
            {
                string sColorChars = "#| wWhHkRrQqYyEeGgLlCcNnBbUuMmPp123456789{}";
                for (unsigned int i = 0; i < sColorSet.length(); i++)
                {
                    if (sColorSet[i] == '{' && i+3 >= sColorSet.length())
                        return false;
                    else if (sColorSet[i] == '{'
                        && (sColorSet[i+3] != '}'
                            || sColorChars.substr(3,sColorChars.length()-14).find(sColorSet[i+1]) == string::npos
                            || sColorSet[i+2] > '9'
                            || sColorSet[i+2] < '1'))
                        return false;
                    if (sColorChars.find(sColorSet[i]) == string::npos)
                        return false;
                }
                return true;
            }
        inline bool checkLineChars(const string& sLineSet)
            {
                string sLineChars = " -:;ij|=";
                for (unsigned int i = 0; i < sLineSet.length(); i++)
                {
                    if (sLineChars.find(sLineSet[i]) == string::npos)
                        return false;
                }
                return true;
            }
        inline bool checkPointChars(const string& sPointSet)
            {
                string sPointChars = " .*+x#sdo^v<>";
                for (unsigned int i = 0; i < sPointSet.length(); i++)
                {
                    if (sPointChars.find(sPointSet[i]) == string::npos)
                        return false;
                }
                return true;
            }
        void replaceControlChars(string& sString);
        string removeSurroundingQuotationMarks(const string& sString);
        void rangeByPercentage(double* dData, size_t nLength, double dLowerPercentage, double dUpperPercentage, vector<double>& vRanges);

    public:
        // --> Konstruktoren und Destruktoren <--
        PlotData();
        PlotData(int _nLines, int _nRows = 1, int _nLayers = 1);
        ~PlotData();

        enum Coordinates
        {
            CARTESIAN = 0,
            POLAR_PZ = 10,
            POLAR_RP,
            POLAR_RZ,
            SPHERICAL_PT = 100,
            SPHERICAL_RP,
            SPHERICAL_RT
        };

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

//        mglFLTK* _graph;

        // --> Wichtigste Funktionen: Daten in Speicher schreiben und daraus lesen <---
        void setData(int _i, int _j, double Data, int _k = 0);
        double getData(int _i, int _j = 0, int _k = 0) const;

        // --> Ebenfalls bedeutend: Plot-Parameter setzen (obige Booleans) und selbige als String lesen <--
        void setParams(const string& __sCmd, Parser& _parser, const Settings& _option, int nType = ALL);
        inline void setGlobalComposeParams(const string& __sCmd, Parser& _parser, const Settings& _option)
            {return setParams(__sCmd, _parser, _option, GLOBAL | SUPERGLOBAL);}
        inline void setLocalComposeParams(const string& __sCmd, Parser& _parser, const Settings& _option)
            {return setParams(__sCmd, _parser, _option, LOCAL);}
        string getParams(const Settings& _option, bool asstr = false) const;
        string getFileName() const;

        // --> Obige Booleans lesen <--
        inline bool getRangeSetting(int i = 0) const
            {
                if (i < 3 && i >= 0)
                    return bRanges[i];
                else
                    return false;
            }
        inline bool getInvertion(int i = 0) const
            {
                if (i < 3 && i >= 0)
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
        inline void setFont(const string& Font)
            {
                sFontStyle = Font;
                return;
            }
        inline string getAxisbind(unsigned int i) const
            {
                if (2*i+1 < sAxisBind.length())
                    return sAxisBind.substr(2*i,2);
                else
                    return "lb";
            }
        inline string getFunctionAxisbind(unsigned int i) const
            {
                if (2*i+1 < sFunctionAxisBind.length())
                    return sFunctionAxisBind.substr(2*i,2);
                else
                    return "lb";
            }
        inline void setFunctionAxisbind(const string& sBind)
            {
                if (sBind.length())
                    sFunctionAxisBind = sBind;
                return;
            }
        inline int getGrid() const
            {return nGrid;};
        inline int getLighting() const
            {return nLighting;};
        inline bool getTransparency() const
            {return bAlpha;};
        inline double getAlphaVal() const
            {
                return dAlphaVal;
            }
        inline bool getAxis() const
            {return bAxis;};
        inline int getSamples() const
            {return nSamples;};
        inline bool getBox() const
            {return bBox;};
        inline int getGivenRanges() const
            {return nRanges;};
        inline bool getContProj() const
            {return bContProj;};
        inline bool getContLabels() const
            {return bContLabels;};
        inline bool getContFilled() const
            {return bContFilled;}
        inline size_t getNumContLines() const
            {
                return nContLines;
            }
        inline bool getxError() const
            {return bxError;};
        inline bool getyError() const
            {return byError;};
        inline bool getxLogscale() const
            {return bLogscale[0];};
        inline bool getyLogscale() const
            {return bLogscale[1];};
        inline bool getzLogscale() const
            {return bLogscale[2];};
        inline bool getcLogscale() const
            {return bLogscale[3];};
        inline int getCoords() const
            {return nCoords;}
        inline bool getConnectPoints() const
            {return bConnectPoints;}
        inline bool getDrawPoints() const
            {return bDrawPoints;}
        inline double gettBoundary(int _i = 0) const
            {return dtParam[_i];}
        inline double getColorRange(int _i = 0) const
            {return dColorRange[_i];}
        inline double getRotateAngle(int _i = 0) const
            {return dRotateAngles[_i];}
        inline double getAspect() const
            {return dAspect;}
        inline bool getOpenImage() const
            {return bOpenImage;}
        inline bool getInterpolate() const
            {return bInterpolate;}
        inline int getHighRes() const
            {return nHighResLevel;}
        inline bool getSilentMode() const
            {return bSilentMode;}
        inline int getMarks() const
            {return nMarks;}
        inline int getAnimateSamples() const
            {
                if (bAnimate)
                    return nAnimateSamples-1;
                else
                    return 0;
            }
        inline bool getCutBox() const
            {return bCutBox;}
        inline bool getFlow() const
            {return bFlow;}
        inline bool getPipe() const
            {return bPipe;}
        inline bool getFixedLength() const
            {return bFixedLength;}
        inline bool getColorbar() const
            {return bColorbar;}
        inline bool getOrthoProject() const
            {return bOrthoProject;}
        inline bool getArea() const
            {return bArea;}
        inline double getBars() const
            {return dBars;}
        inline double getHBars() const
            {return dHBars;}
        inline bool getStepplot() const
            {return bStepPlot;}
        inline bool getBoxplot() const
            {return bBoxPlot;}
        inline bool getColorMask() const
            {return bColorMask;}
        inline bool getAlphaMask() const
            {return bAlphaMask;}
        inline bool getSchematic() const
            {return bSchematic;}
        inline bool getCloudPlot() const
            {return bCloudPlot;}
        inline bool getRegion() const
            {return bRegion;}
        inline bool getCrust() const
            {return bCrust;}
        inline double getPerspective() const
            {return dPerspective;}
        inline double getTextSize() const
            {return dTextsize;}
        inline int getLegendPosition() const
            {return nLegendPosition;}
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

        inline string getColorScheme(string _sAddOpt = "") const
            {return (sColorScheme+_sAddOpt);}
        inline string getColorSchemeMedium(string _sAddOpt = "") const
            {return (sColorSchemeMedium+_sAddOpt);}
        inline string getColorSchemeLight(string _sAddOpt = "") const
            {return (sColorSchemeLight+_sAddOpt);}
        inline string getBGColorScheme() const
            {return sBackgroundColorScheme;}
        inline string getBackground() const
            {return sBackground;}

        inline string getColors() const
            {return (sColorScheme == "wk" || sColorScheme == "kw") ? sGreys : sColors;}
        inline string getContColors() const
            {return (sColorScheme == "wk" || sColorScheme == "kw") ? sContGreys : sContColors;}
        inline string getPointStyles() const
            {return sPointStyles;}
        inline string getLineStyles() const
            {return (sColorScheme == "wk" || sColorScheme == "kw") ? sLineStylesGrey : sLineStyles;}
        inline string getLineSizes() const
            {return sLineSizes;}
        inline int getLegendStyle() const
            {return nLegendstyle;}
        inline string getGridStyle() const
            {return sGridStyle.substr(0,3);}
        inline string getFineGridStyle() const
            {return sGridStyle.substr(3);}
        inline Line getHLines(unsigned int i = 0)
            {
                Line _lLine;
                _lLine.dPos = 0.0;
                _lLine.sDesc = "";
                if (!i)
                    _lHlines[i].dPos = getMax();
                if (i == 1)
                    _lHlines[i].dPos = getMin();
                if (i < _lHlines.size())
                    return _lHlines[i];
                else
                    return _lLine;
            }
        inline size_t getHLinesSize() const
            {
                return _lHlines.size();
            }
        inline Line getVLines(unsigned int i = 0) const
            {
                Line _lLine;
                _lLine.dPos = 0.0;
                _lLine.sDesc = "";
                if (i < _lVLines.size())
                    return _lVLines[i];
                else
                    return _lLine;
            }
        inline size_t getVLinesSize() const
            {
                return _lVLines.size();
            }
        inline Axis getAddAxis(unsigned int i = 0) const
            {
                Axis _Axis;
                _Axis.sLabel = "";
                _Axis.sStyle = "k";
                _Axis.dMax = NAN;
                _Axis.dMin = NAN;
                if (i < 2)
                    return _AddAxes[i];
                else
                    return _Axis;
            }
        inline void setAddAxis(unsigned int i = 0, double _dMin = NAN, double _dMax = NAN)
            {
                if (i < 2 && isnan(_AddAxes[i].dMax) && isnan(_AddAxes[i].dMin) && !isnan(_dMin) && !isnan(_dMax))
                {
                    _AddAxes[i].dMin = _dMin;
                    _AddAxes[i].dMax = _dMax;
                    if (!i && !_AddAxes[i].sLabel.length())
                        _AddAxes[i].sLabel = "\\i x";
                    else if (!_AddAxes[i].sLabel.length())
                        _AddAxes[i].sLabel = "\\i y";
                }
                return;
            }
        inline TimeAxis getTimeAxis(unsigned int i = 0) const
            {
                TimeAxis axis;

                if (i < 4)
                    return _timeAxes[i];

                return axis;
            }

        // --> Lesen der einzelnen Achsenbeschriftungen <--
        string getxLabel() const;
        string getyLabel() const;
        string getzLabel() const;


        inline string getTickTemplate(int nAxis = 0) const
            {
                if (nAxis >= 0 && nAxis < 4)
                    return sTickTemplate[nAxis];
                else
                    return "";
            }
        inline string getCustomTick(int nAxis = 0) const
            {
                if (nAxis >= 0 && nAxis < 4)
                    return sCustomTicks[nAxis];
                else
                    return "";
            }

        // --> Lesen der Titelzeile <--
        inline string getTitle() const
            {return sPlotTitle;}
        inline string getComposedTitle() const
            {return sComposedTitle;}

        // --> Einstellen der Groesse des Speichers <--
        void setDim(int _i, int _j = 1, int _k = 1);
        void setRanges(int _j, double x_0, double x_1);
        void setSamples(int _nSamples);
        void normalize(int nDim = 2, int t_animate = 0);

        // --> Setzen des Dateinamens <--
        void setFileName(string _sFileName);

        // --> Zuruecksetzen des gesamten Objekts bzw. Loeschen des Speichers <--
        void reset();
        void deleteData(bool bGraphFinished = false);

        // --> Dimensionen des Speichers lesen <--
        int getRows() const;
        int getLines() const;
        int getLayers(bool bFull = false) const;

        // --> Maximum und Minimum aller Werte im Speicher lesen <--
        double getMin(int nCol = ALLRANGES) const;
        double getMax(int nCol = ALLRANGES) const;
        vector<double> getWeightedRanges(int nCol = ALLRANGES, double dLowerPercentage = 0.75, double dUpperPercentage = 0.75);

        // --> Intervallgrenzen lesen <--
        double getRanges(int _j, int _i = 0) const;
};

#endif
