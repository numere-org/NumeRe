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


#include "plugins.hpp"
#include "../kernel.hpp"

/*
 * Plugin zur Erzeugung von Histogramm-Rubriken
 */


const string PI_HIST = "1.0.1";
extern mglGraph _fontData;

void plugin_histogram (string& sCmd, Datafile& _data, Datafile& _target, Output& _out, Settings& _option, PlotData& _pData, bool bUseCache, bool bUseData)
{
    if (_option.getbDebug())
        cerr << "|-> DEBUG: sCmd = " << sCmd << endl;
	if (_data.isValid() || _data.isValidCache())			// Sind ueberhaupt Daten vorhanden?
	{

		//cerr << "|-> HISTOGRAMM (v " << PI_HIST << ")" << endl;
		//cerr << "|   " << std::setfill((char)196) << std::setw(20) << (char)196 << endl;
        //cerr << LineBreak("|-> Dieses Plugin generiert den Datensatz eines Histogramms aus einer oder mehreren gegebenen Datenreihen.", _option) << endl;
        if (!bUseCache && !bUseData)
        {
            if (!_data.isValid() && _data.isValidCache())
            {
                NumeReKernel::print(LineBreak(_lang.get("HIST_ONLY_CACHE"), _option));
                _data.setCacheStatus(true);
            }
            else if (_data.isValid() && _data.isValidCache())
            {
                char c = 0;
                NumeReKernel::print(LineBreak(_lang.get("HIST_ASK_DATASET"), _option));
                NumeReKernel::printPreFmt("|\n|<- ");

                cin >> c;

                if (c == '0')
                {
                    NumeReKernel::print(_lang.get("COMMON_CANCEL")+".");
                    cin.ignore(1);
                    return;
                }
                else if (c == 'c')
                    _data.setCacheStatus(true);
                NumeReKernel::print(LineBreak(_lang.get("HIST_CONFIRM_DATASET", _data.getDataFileName("data")), _option));
            }
            else
            {
                NumeReKernel::print(LineBreak(_lang.get("HIST_CONFIRM_DATASET", _data.getDataFileName("data")), _option));
            }
        }
        else if (bUseCache || sCmd.find("-cache") != string::npos || _data.matchCache(sCmd).length())
        {
            _data.setCacheStatus(true);
        }

		// --> ein Histogramm kann nur von einer Datenreihe erzeugt werden. Alles andere ist nicht sinnvoll umsetzbar <--
		int nDataRow = 0;			// Variable zur Festlegung, mit welcher Datenreihe begonnen werden soll
		int nDataRowFinal = 0;		// Variable zur Festlegung, mit welcher Datenreihe aufgehoehrt werden soll
		string** sOut;				// Ausgabe-Matrix
		string sLegend = "";
		string sDatatable = "data";
		if (_data.matchCache(sCmd).length())
            sDatatable = _data.matchCache(sCmd);
		if (!_data.getCols(sDatatable) || !_data.getLines(sDatatable))
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);
        string sTargettable = "cache";
		string sBinLabel = "Bins";
		string sCountLabel = "Counts";
		string sAxisLabels[3] = {"x","y","z"};
		double** dHistMatrix = 0;
		double* dTicksVal = 0;
		double dSum = 0;
		string sTicks = "";
		int nBin = 0;				// Anzahl der Daten-Bins --> Je groesser, desto feiner die Aufloesung, aber desto weniger Trend
		int nCount = 0;
		int nMax = 0;
		int nMethod = 0;
		//double dGreatestValue = 0.0;	// Groesster Datenwert
		//double dSmallestValue = 0.0;	// Kleinster Datenwert
		double dIntervallLength = 0.0;	// Laenge der endgueltigen Bin-Breite
		double dIntervallLengthY = 0.0;	// Laenge der endgueltigen Bin-Breite
		double dMin = NAN;
		double dMax = NAN;
		double dMinY = NAN;
		double dMaxY = NAN;
		double dMinZ = NAN;
		double dMaxZ = NAN;
		bool bWriteToCache = false;
		bool bMake2DHist = false;
		bool bSum = false;
		bool bSilent = false;
		bool bGrid = false;
		string sHistSavePath = "";

        if (findCommand(sCmd).sString == "hist2d")
            bMake2DHist = true;

		if (matchParams(sCmd, "cols", '=') || matchParams(sCmd, "c", '='))
		{
            int nPos = 0;
            if (matchParams(sCmd, "cols", '='))
                nPos = matchParams(sCmd, "cols", '=')+4;
            else
                nPos = matchParams(sCmd, "c", '=')+1;
            string sTemp = getArgAtPos(sCmd, nPos);
            string sTemp_2 = "";
            StripSpaces(sTemp);
            if (sTemp.find(':') != string::npos)
            {
                int nSep = 0;
                for (unsigned int i = 0; i < sTemp.length(); i++)
                {
                    if (sTemp[i] == ':')
                    {
                        nSep = i;
                        break;
                    }
                }
                sTemp_2 = sTemp.substr(0,nSep);
                sTemp = sTemp.substr(nSep+1);
                StripSpaces(sTemp);
                StripSpaces(sTemp_2);
                if (sTemp_2.length())
                {
                    nDataRow = (int)StrToDb(sTemp_2);
                }
                else
                    nDataRow = 1;

                if (sTemp.length())
                {
                    nDataRowFinal = (int)StrToDb(sTemp);
                    if (nDataRowFinal > _data.getCols(sDatatable))
                        nDataRowFinal = _data.getCols(sDatatable);
                }
                else
                    nDataRowFinal = _data.getCols(sDatatable);
            }
            else if (sTemp.length())
                nDataRow = (int)StrToDb(sTemp);
            /*nDataRow = (int)StrToDb(sCmd.substr(sCmd.find("cols=")+5, sCmd.find(':', sCmd.find("cols=")+5)-sCmd.find("cols=")-5));
            nDataRowFinal = (int)StrToDb(sCmd.substr(sCmd.find(':', sCmd.find("cols="))+1, sCmd.find(' ', sCmd.find("cols=")+5)-sCmd.find(':', sCmd.find("cols="))-1));*/
        }
		if (matchParams(sCmd, "bins", '=') || matchParams(sCmd, "b", '='))
		{
            if (matchParams(sCmd, "bins", '='))
                nBin = matchParams(sCmd, "bins", '=')+4;
            else
                nBin = matchParams(sCmd, "b", '=')+1;
            nBin = (int)StrToDb(getArgAtPos(sCmd, nBin));
        }
		if (matchParams(sCmd, "width", '=') || matchParams(sCmd, "w", '='))
		{
            int nPos = 0;
            if (matchParams(sCmd, "width", '='))
                nPos = matchParams(sCmd, "width", '=')+5;
            else
                nPos = matchParams(sCmd, "w", '=')+1;
            dIntervallLength = StrToDb(getArgAtPos(sCmd, nPos));
        }
        if (matchParams(sCmd, "save", '=') || matchParams(sCmd, "export", '='))
        {
            int nPos = 0;
            if (matchParams(sCmd, "save", '='))
                nPos = matchParams(sCmd, "save", '=')+4;
            else
                nPos = matchParams(sCmd, "export", '=')+6;
            sHistSavePath = getArgAtPos(sCmd, nPos);
            _out.setStatus(true);
        }
        if ((!bMake2DHist && matchParams(sCmd, "xlabel", '=')) || matchParams(sCmd, "binlabel", '='))
        {
            if (matchParams(sCmd, "xlabel", '='))
            {
                sBinLabel = fromSystemCodePage(getArgAtPos(sCmd, matchParams(sCmd, "xlabel", '=')+6));
            }
            else
                sBinLabel = fromSystemCodePage(getArgAtPos(sCmd, matchParams(sCmd, "binlabel", '=')+8));
            StripSpaces(sBinLabel);
            if (!sBinLabel.length())
                sBinLabel = "Bins";
        }
        if ((!bMake2DHist && matchParams(sCmd, "ylabel", '=')) || matchParams(sCmd, "countlabel", '='))
        {
            if (matchParams(sCmd, "ylabel", '='))
                sCountLabel = fromSystemCodePage(getArgAtPos(sCmd, matchParams(sCmd, "ylabel", '=')+6));
            else
                sCountLabel = fromSystemCodePage(getArgAtPos(sCmd, matchParams(sCmd, "countlabel", '=')+10));
            StripSpaces(sCountLabel);
            if (!sCountLabel.length())
                sCountLabel = "Counts";
        }
        if (matchParams(sCmd, "xlabel", '=') && bMake2DHist)
        {
            sAxisLabels[0] = fromSystemCodePage(getArgAtPos(sCmd, matchParams(sCmd, "xlabel", '=')+6));
            StripSpaces(sAxisLabels[0]);
            if (!sAxisLabels[0].length())
                sAxisLabels[0] = "x";
        }
        if (matchParams(sCmd, "ylabel", '=') && bMake2DHist)
        {
            sAxisLabels[1] = fromSystemCodePage(getArgAtPos(sCmd, matchParams(sCmd, "ylabel", '=')+6));
            StripSpaces(sAxisLabels[1]);
            if (!sAxisLabels[1].length())
                sAxisLabels[1] = "y";
        }
        if (matchParams(sCmd, "zlabel", '=') && bMake2DHist)
        {
            sAxisLabels[2] = fromSystemCodePage(getArgAtPos(sCmd, matchParams(sCmd, "zlabel", '=')+6));
            StripSpaces(sAxisLabels[2]);
            if (!sAxisLabels[2].length())
                sAxisLabels[2] = "z";
        }
        if (matchParams(sCmd, "sum"))
            bSum = true;
        if (matchParams(sCmd, "save") || matchParams(sCmd, "export"))
            _out.setStatus(true);
        if (matchParams(sCmd, "x", '='))
        {
            string sTemp =  getArgAtPos(sCmd, matchParams(sCmd, "x", '=')+1);
            //cerr << sTemp << endl;
            if (sTemp.find(':') != string::npos)
            {
                if (sTemp.substr(0,sTemp.find(':')).length())
                    dMin = StrToDb(sTemp.substr(0,sTemp.find(':')));
                if (sTemp.substr(sTemp.find(':')+1).length())
                    dMax = StrToDb(sTemp.substr(sTemp.find(':')+1));
            }
        }
        if (matchParams(sCmd, "y", '='))
        {
            string sTemp =  getArgAtPos(sCmd, matchParams(sCmd, "y", '=')+1);
            //cerr << sTemp << endl;
            if (sTemp.find(':') != string::npos)
            {
                if (sTemp.substr(0,sTemp.find(':')).length())
                    dMinY = StrToDb(sTemp.substr(0,sTemp.find(':')));
                if (sTemp.substr(sTemp.find(':')+1).length())
                    dMaxY = StrToDb(sTemp.substr(sTemp.find(':')+1));
            }
        }
        if (matchParams(sCmd, "z", '='))
        {
            string sTemp =  getArgAtPos(sCmd, matchParams(sCmd, "z", '=')+1);
            //cerr << sTemp << endl;
            if (sTemp.find(':') != string::npos)
            {
                if (sTemp.substr(0,sTemp.find(':')).length())
                    dMinZ = StrToDb(sTemp.substr(0,sTemp.find(':')));
                if (sTemp.substr(sTemp.find(':')+1).length())
                    dMaxZ = StrToDb(sTemp.substr(sTemp.find(':')+1));
            }
        }
        if (matchParams(sCmd, "tocache"))
            bWriteToCache = true;
        if (matchParams(sCmd, "tocache", '='))
        {
            bWriteToCache = true;
            sTargettable = getArgAtPos(sCmd, matchParams(sCmd, "tocache", '=')+7);
            if (sTargettable.find('(') == string::npos)
                sTargettable += "()";
            if (!_target.isCacheElement(sTargettable))
            {
                _target.addCache(sTargettable, _option);
            }
            sTargettable.erase(sTargettable.find('('));
        }
        if (matchParams(sCmd, "silent"))
            bSilent = true;
        if (matchParams(sCmd, "grid") && nDataRowFinal - nDataRow > 3)
        {
            bGrid = true;
        }
        if (matchParams(sCmd, "method", '='))
        {
            if (getArgAtPos(sCmd, matchParams(sCmd, "method", '=')+6) == "scott")
                nMethod = 1;
            else if (getArgAtPos(sCmd, matchParams(sCmd, "method", '=')+6) == "freedman")
                nMethod = 2;
            else
                nMethod = 0;
        }
        if (matchParams(sCmd, "m", '='))
        {
            if (getArgAtPos(sCmd, matchParams(sCmd, "m", '=')+1) == "scott")
                nMethod = 1;
            else if (getArgAtPos(sCmd, matchParams(sCmd, "m", '=')+1) == "freedman")
                nMethod = 2;
            else
                nMethod = 0;
        }


        if (nDataRow && nDataRowFinal)
        {
            if (nDataRowFinal < nDataRow)
            {
                int n = nDataRow;
                nDataRow = nDataRowFinal;
                nDataRowFinal = n;
            }
        }
        //cerr << nDataRow << nDataRowFinal << endl;

		int nStyle = 0;
		const int nStyleMax = 14;
		string sColorStyles[nStyleMax] = {"r", "g", "b", "q", "m", "P", "u", "R", "G", "B", "Q", "M", "p", "U"};
		for (int i = 0; i < nStyleMax; i++)
		{
            sColorStyles[i] = _pData.getColors()[i];
		}
		mglGraph* _histGraph = new mglGraph();
		double dAspect = 8.0/3.0;
		if (bMake2DHist /*|| !bSilent || !_pData.getSilentMode()*/)
            dAspect = 4.0/3.0;
        if (_pData.getHighRes() == 2 && bSilent && _pData.getSilentMode())           // Aufloesung und Qualitaet einstellen
        {
            double dHeight = sqrt(1920.0 * 1440.0 / dAspect);
            _histGraph->SetSize((int)lrint(dAspect*dHeight), (int)lrint(dHeight));          // FullHD!
        }
        else if (_pData.getHighRes() == 1 && bSilent && _pData.getSilentMode())
        {
            double dHeight = sqrt(1280.0 * 960.0 / dAspect);
            _histGraph->SetSize((int)lrint(dAspect*dHeight), (int)lrint(dHeight));           // ehem. die bessere Standard-Aufloesung
            //_graph.SetQuality(5);
        }
        else
        {
            double dHeight = sqrt(800.0 * 600.0 / dAspect);
            _histGraph->SetSize((int)lrint(dAspect*dHeight), (int)lrint(dHeight));
            // --> Im Falle, dass wir meinen mesh/surf/anders gearteten 3D-Plot machen, drehen wir die Qualitaet runter <--
        }
        // --> Noetige Einstellungen und Deklarationen fuer den passenden Plot-Stil <--
        _histGraph->CopyFont(&_fontData);
        _histGraph->SetFontSizeCM(0.24*((double)(1+_pData.getTextSize())/6.0), 72);
        _histGraph->SetBarWidth(_pData.getBars() ? _pData.getBars() : 0.9);

		_histGraph->SetRanges(1,2,1,2,1,2);
		if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale())
            _histGraph->SetFunc("lg(x)", "");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
            _histGraph->SetFunc("lg(x)", "lg(y)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
            _histGraph->SetFunc("", "lg(y)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
            _histGraph->SetFunc("lg(x)","", "lg(z)");
        else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
            _histGraph->SetFunc("","", "lg(z)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
            _histGraph->SetFunc("","lg(y)", "lg(z)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
            _histGraph->SetFunc("lg(x)","lg(y)", "lg(z)");
		mglData _histData;
		mglData _hist2DData[3];
		mglData _mAxisVals[2];
		if (!bMake2DHist)
		{
            if (_data.getCols(sDatatable) > 1 && !nDataRow)		// Besteht der Datensatz aus mehr als eine Reihe? -> Welche soll denn dann verwendet werden?
            {
                delete _histGraph;
                throw SyntaxError(SyntaxError::NO_COLS, sCmd, SyntaxError::invalid_position);
            }

            if (nDataRow >= _data.getCols(sDatatable)+1)
            {
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak(_lang.get("HIST_GENERATING_DATASETS", toString(_data.getCols(sDatatable))), _option));
                    //cerr << LineBreak("|-> Es werden " + toString(_data.getCols(sDatatable)) + " Histogramm-Datensätze erzeugt.", _option) << endl;
                nDataRowFinal = _data.getCols(sDatatable);
                nDataRow = 0;
            }
            else if (nDataRow && nDataRowFinal)
            {
                nDataRow--;
            }
            else if (nDataRow)
            {
                nDataRowFinal = nDataRow;
                nDataRow--;				// Dekrement aufgrund des natuerlichen Index-Shifts
            }
            else
            {
                nDataRowFinal = 1;
            }


            if (bGrid)
            {
                // x-Range
                if (isnan(dMin) && isnan(dMax))
                {
                    dMin = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1);
                    dMax = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1);
                    double dIntervall = dMax - dMin;
                    dMax += dIntervall / 10.0;
                    dMin -= dIntervall / 10.0;
                }
                else if (isnan(dMin))
                {
                    dMin = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1);
                }
                else if (isnan(dMax))
                {
                    dMax = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1);
                }
                if (dMax < dMin)
                {
                    double dTemp = dMax;
                    dMax = dMin;
                    dMin = dTemp;
                }
                if (!isnan(dMin) && !isnan(dMax))
                {
                    if (dMin > _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1)
                        || dMax < _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1))
                    {
                        delete _histGraph;
                        throw SyntaxError(SyntaxError::INVALID_INTERVAL, sCmd, SyntaxError::invalid_position);
                    }
                }
                // y-Range
                if (isnan(dMinY) && isnan(dMaxY))
                {
                    dMinY = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2);
                    dMaxY = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2);
                    double dIntervall = dMaxY - dMinY;
                    dMaxY += dIntervall / 10.0;
                    dMinY -= dIntervall / 10.0;
                }
                else if (isnan(dMinY))
                {
                    dMinY = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2);
                }
                else if (isnan(dMax))
                {
                    dMaxY = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2);
                }
                if (dMaxY < dMinY)
                {
                    double dTemp = dMaxY;
                    dMaxY = dMinY;
                    dMinY = dTemp;
                }
                if (!isnan(dMinY) && !isnan(dMaxY))
                {
                    if (dMinY > _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2)
                        || dMaxY < _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2))
                    {
                        delete _histGraph;
                        throw SyntaxError(SyntaxError::INVALID_INTERVAL, sCmd, SyntaxError::invalid_position);
                    }
                }
                // z-Range
                if (isnan(dMinZ) && isnan(dMaxZ))
                {
                    dMinZ = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+2,nDataRowFinal);
                    dMaxZ = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+2,nDataRowFinal);
                    double dIntervall = dMaxZ - dMinZ;
                    dMaxZ += dIntervall / 10.0;
                    dMinZ -= dIntervall / 10.0;
                }
                else if (isnan(dMinZ))
                {
                    dMinZ = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+2,nDataRowFinal);
                }
                else if (isnan(dMaxZ))
                {
                    dMaxZ = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+2,nDataRowFinal);
                }
                if (dMaxZ < dMinZ)
                {
                    double dTemp = dMaxZ;
                    dMaxZ = dMinZ;
                    dMinZ = dTemp;
                }
                if (!isnan(dMinZ) && !isnan(dMaxZ))
                {
                    if (dMinZ > _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+2,nDataRowFinal)
                        || dMaxZ < _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+2,nDataRowFinal))
                    {
                        delete _histGraph;
                        throw SyntaxError(SyntaxError::INVALID_INTERVAL, sCmd, SyntaxError::invalid_position);
                    }
                }
            }
            else
            {
                if (isnan(dMin) && isnan(dMax))
                {
                    dMin = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRowFinal);
                    dMax = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRowFinal);
                    double dIntervall = dMax - dMin;
                    dMax += dIntervall / 10.0;
                    dMin -= dIntervall / 10.0;
                }
                else if (isnan(dMin))
                {
                    dMin = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRowFinal);
                }
                else if (isnan(dMax))
                {
                    dMax = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRowFinal);
                }

                if (dMax < dMin)
                {
                    double dTemp = dMax;
                    dMax = dMin;
                    dMin = dTemp;
                }

                if (!isnan(dMin) && !isnan(dMax))
                {
                    if (dMin > _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRowFinal)
                        || dMax < _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRowFinal))
                    {
                        delete _histGraph;
                        throw SyntaxError(SyntaxError::INVALID_INTERVAL, sCmd, SyntaxError::invalid_position);
                    }
                }
            }

            for (int i = 0; i < _data.getLines(sDatatable); i++)
            {
                for (int j = nDataRow+2*bGrid; j < nDataRowFinal; j++)
                {
                    if (_data.isValidEntry(i,j, sDatatable) && _data.getElement(i,j, sDatatable) <= dMax && _data.getElement(i,j, sDatatable) >= dMin)
                        nMax++;
                }
            }

            if (!nBin && dIntervallLength == 0.0)
            {
                if (bGrid)
                {
                    if (!nMethod)
                        nBin = (int)rint(1.0+3.3*log10((double)nMax));
                    else if (nMethod == 1)
                        dIntervallLength = 3.49*_data.std(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow+2, nDataRowFinal)/pow((double)nMax, 1.0/3.0);
                    else if (nMethod == 2)
                        dIntervallLength = 2.0*(_data.pct(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow+2, nDataRowFinal, 0.75) - _data.pct(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow+2, nDataRowFinal, 0.25))/pow((double)nMax, 1.0/3.0);
                }
                else
                {
                    if (!nMethod)
                        nBin = (int)rint(1.0+3.3*log10((double)nMax/(double)(nDataRowFinal-nDataRow)));
                    else if (nMethod == 1)
                        dIntervallLength = 3.49*_data.std(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow)/pow((double)nMax/(double)(nDataRowFinal-nDataRow),1.0/3.0);
                    else if (nMethod == 2)
                        dIntervallLength = 2.0*(_data.pct(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow, -1, 0.75) - _data.pct(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow, -1, 0.25))/pow((double)nMax/(double)(nDataRowFinal-nDataRow),1.0/3.0);
                }
                if(_option.getbDebug())
                    cerr << "|-> DEBUG: nBin = " << nBin << endl;
            }
            if (nBin)
            {
                if (bGrid)
                    _histData.Create(nBin);
                else
                    _histData.Create(nBin, nDataRowFinal-nDataRow);
            }
            else
            {
                if (dIntervallLength == 0.0)
                {
                    NumeReKernel::print(toSystemCodePage(_lang.get("HIST_ASK_BINWIDTH")));
                    NumeReKernel::printPreFmt("|\n|<- ");
                    cin >> dIntervallLength;


                    if (_option.getbDebug())
                        cerr << "|-> DEBUG: dIntervallLength = " << dIntervallLength << endl;
                }
                // --> Gut. Dann berechnen wir daraus die Anzahl der Bins -> Es kann nun aber sein, dass der letzte Bin ueber
                //     das Intervall hinauslaeuft <--
                if (dIntervallLength > dMax - dMin)
                {
                    delete _histGraph;
                    throw SyntaxError(SyntaxError::TOO_LARGE_BINWIDTH, sCmd, SyntaxError::invalid_position);
                }
                for (int i = 0; (i * dIntervallLength)+dMin < dMax+dIntervallLength; i++)
                {
                    nBin++;
                }
                double dDiff = nBin*dIntervallLength-(double)(dMax-dMin);
                dMin -= dDiff/2.0;
                dMax += dDiff/2.0;

                if (_option.getbDebug())
                    cerr << "|-> DEBUG: nBin = " << nBin << endl;

                if (nBin)
                {
                    if (bGrid)
                        _histData.Create(nBin);
                    else
                        _histData.Create(nBin, nDataRowFinal-nDataRow);
                }
            }
            _mAxisVals[0].Create(nBin);

            if (dIntervallLength == 0.0)
            {
                // --> Berechne die Intervall-Laenge (Explizite Typumwandlung von int->double) <--
                if (bGrid)
                {
                    if (_pData.getxLogscale())
                        dIntervallLength = (log10(dMaxZ) - log10(dMinZ)) / (double)nBin;
                    else
                        dIntervallLength = abs(dMaxZ - dMinZ) / (double)nBin;
                }
                else
                {
                    if (_pData.getxLogscale())
                        dIntervallLength = (log10(dMax) - log10(dMin)) / (double)nBin;
                    else
                        dIntervallLength = abs(dMax - dMin) / (double)nBin;
                }
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: dIntervallLength = " << dIntervallLength << endl;
            }

            dHistMatrix = new double*[nBin];
            for (int i = 0; i < nBin; i++)
            {
                dHistMatrix[i] = new double[nDataRowFinal-nDataRow];
            }

            string sCommonExponent = "";
            double dCommonExponent = 1.0;

            if (bGrid)
            {
                nMax = 0;
                for (int k = 0; k < nBin; k++)
                {
                    nCount = 0;
                    for (int i = nDataRow+2; i < nDataRowFinal; i++)
                    {
                        for (int l = 0; l < _data.getLines(sDatatable, true) - _data.getAppendedZeroes(i,sDatatable); l++)
                        {
                            if (_data.getElement(l, nDataRow, sDatatable) > dMax
                                || _data.getElement(l, nDataRow, sDatatable) < dMin
                                || _data.getElement(l, nDataRow+1, sDatatable) > dMaxY
                                || _data.getElement(l, nDataRow+1, sDatatable) < dMinY
                                || _data.getElement(l, i, sDatatable) > dMaxZ
                                || _data.getElement(l, i, sDatatable) < dMinZ)
                                continue;
                            if (_pData.getxLogscale())
                            {
                                if (_data.getElement(l,i, sDatatable) >= pow(10.0,log10(dMinZ) + k * dIntervallLength)
                                        && _data.getElement(l,i, sDatatable) < pow(10.0,log10(dMinZ) + (k+1) * dIntervallLength))
                                    nCount++;
                            }
                            else
                            {
                                if (_data.getElement(l,i, sDatatable) >= dMinZ + k * dIntervallLength
                                        && _data.getElement(l,i, sDatatable) < dMinZ + (k+1) * dIntervallLength)
                                    nCount++;
                            }
                        }
                        if (i == nDataRow+2 && !_pData.getxLogscale())
                            _mAxisVals[0].a[k] = dMinZ + (k+0.5)*dIntervallLength;
                        else if (i == nDataRow+2)
                            _mAxisVals[0].a[k] = pow(10.0, log10(dMinZ)+(k+0.5)*dIntervallLength);
                    }
                    dHistMatrix[k][0] = nCount;
                    _histData.a[k] = nCount;
                    if (nCount > nMax)
                        nMax = nCount;
                }
                //sLegend = sDatatable;
                _histGraph->AddLegend("grid", sColorStyles[nStyle].c_str());
                if (nStyle == nStyleMax-1)
                    nStyle = 0;
                else
                    nStyle++;


                if (toString(dMinZ+dIntervallLength/2.0, 3).find('e') != string::npos || toString(dMinZ+dIntervallLength/2.0, 3).find('E') != string::npos)
                {
                    sCommonExponent = toString(dMinZ+dIntervallLength/2.0, 3).substr(toString(dMinZ+dIntervallLength/2.0, 3).find('e'));
                    dCommonExponent = StrToDb("1.0" + sCommonExponent);
                    for (int i = 0; i < nBin; i++)
                    {
                        if (toString((dMinZ+i*dIntervallLength + dIntervallLength/2.0)/dCommonExponent, 3).find('e') != string::npos)
                        {
                            sCommonExponent = "";
                            dCommonExponent = 1.0;
                            break;
                        }
                    }
                    sTicks = toString((dMinZ+dIntervallLength/2.0)/dCommonExponent, 3) + "\\n";
                }
                else
                    sTicks = toString(dMinZ+dIntervallLength/2.0, 3) + "\\n";
                for (int i = 1; i < nBin-1; i++)
                {
                    //dTicksVal[i] = dTicksVal[i-1] + dIntervallLength;
                    if (nBin > 16)
                    {
                        if (!((nBin-1) % 2) && !(i % 2) && nBin-1 < 33)
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 2) && (i % 2) && nBin-1 < 33)
                            sTicks += "\\n";
                        else if (!((nBin-1) % 4) && !(i % 4))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 4) && (i % 4))
                            sTicks += "\\n";
                        else if (!((nBin-1) % 3) && !(i % 3))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 3) && (i % 3))
                            sTicks += "\\n";
                        else if (!((nBin-1) % 5) && !(i % 5))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 5) && (i % 5))
                            sTicks += "\\n";
                        else if (!((nBin-1) % 7) && !(i % 7))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 7) && (i % 7))
                            sTicks += "\\n";
                        else if (!((nBin-1) % 11) && !(i % 11))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 11) && (i % 11))
                            sTicks += "\\n";
                        else if (((nBin-1) % 2 && (nBin-1) % 3 && (nBin-1) % 5 && (nBin-1) % 7 && (nBin-1) % 11) && !(i % 3))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else
                            sTicks += "\\n";
                    }
                    else
                        sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                }
                //dTicksVal[nBin-1] = dMax-dIntervallLength/2.0;
                sTicks += toString(_mAxisVals[0].a[nBin-1]/dCommonExponent, 3);

                if (sCommonExponent.length())
                {
                    while (sTicks.find(sCommonExponent) != string::npos)
                        sTicks.erase(sTicks.find(sCommonExponent), sCommonExponent.length());

                    if (sCommonExponent.find('-') != string::npos)
                    {
                        sCommonExponent = "\\times 10^{-" + sCommonExponent.substr(sCommonExponent.find_first_not_of('0',2)) + "}";
                    }
                    else
                    {
                        sCommonExponent = "\\times 10^{" + sCommonExponent.substr(sCommonExponent.find_first_not_of('0',2)) + "}";
                    }
                }

            }
            else
            {
                nMax = 0;
                for (int i = 0; i < nDataRowFinal-nDataRow; i++)
                {
                    for (int k = 0; k < nBin; k++)
                    {
                        nCount = 0;
                        for (int l = 0; l < _data.getLines(sDatatable, true) - _data.getAppendedZeroes(i+nDataRow,sDatatable); l++)
                        {
                            if (_pData.getxLogscale())
                            {
                                if (_data.getElement(l,i+nDataRow, sDatatable) >= pow(10.0,log10(dMin) + k * dIntervallLength)
                                        && _data.getElement(l,i+nDataRow, sDatatable) < pow(10.0,log10(dMin) + (k+1) * dIntervallLength))
                                    nCount++;
                            }
                            else
                            {
                                if (_data.getElement(l,i+nDataRow, sDatatable) >= dMin + k * dIntervallLength
                                        && _data.getElement(l,i+nDataRow, sDatatable) < dMin + (k+1) * dIntervallLength)
                                    nCount++;
                            }
                        }
                        if (!i && !_pData.getxLogscale())
                            _mAxisVals[0].a[k] = dMin + (k+0.5)*dIntervallLength;
                        else if (!i)
                            _mAxisVals[0].a[k] = pow(10.0, log10(dMin)+(k+0.5)*dIntervallLength);
                        dHistMatrix[k][i] = nCount;
                        _histData.a[k+(nBin*i)] = nCount;
                        if (nCount > nMax)
                            nMax = nCount;
                    }
                    sLegend = _data.getTopHeadLineElement(i+nDataRow,sDatatable);
                    while (sLegend.find('_') != string::npos)
                    {
                        sLegend[sLegend.find('_')] = ' ';
                    }
                    _histGraph->AddLegend(sLegend.c_str(), sColorStyles[nStyle].c_str());
                    if (nStyle == nStyleMax-1)
                        nStyle = 0;
                    else
                        nStyle++;
                }
                //dTicksVal = new double[nBin];
                //dTicksVal[0] = dMin+dIntervallLength/2.0;
                if (toString(dMin+dIntervallLength/2.0, 3).find('e') != string::npos || toString(dMin+dIntervallLength/2.0, 3).find('E') != string::npos)
                {
                    sCommonExponent = toString(dMin+dIntervallLength/2.0, 3).substr(toString(dMin+dIntervallLength/2.0, 3).find('e'));
                    dCommonExponent = StrToDb("1.0" + sCommonExponent);
                    for (int i = 0; i < nBin; i++)
                    {
                        if (toString((dMin+i*dIntervallLength + dIntervallLength/2.0)/dCommonExponent, 3).find('e') != string::npos)
                        {
                            sCommonExponent = "";
                            dCommonExponent = 1.0;
                            break;
                        }
                    }
                    sTicks = toString((dMin+dIntervallLength/2.0)/dCommonExponent, 3) + "\\n";
                }
                else
                    sTicks = toString(dMin+dIntervallLength/2.0, 3) + "\\n";
                for (int i = 1; i < nBin-1; i++)
                {
                    //dTicksVal[i] = dTicksVal[i-1] + dIntervallLength;
                    if (nBin > 16)
                    {
                        if (!((nBin-1) % 2) && !(i % 2) && nBin-1 < 33)
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 2) && (i % 2) && nBin-1 < 33)
                            sTicks += "\\n";
                        else if (!((nBin-1) % 4) && !(i % 4))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 4) && (i % 4))
                            sTicks += "\\n";
                        else if (!((nBin-1) % 3) && !(i % 3))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 3) && (i % 3))
                            sTicks += "\\n";
                        else if (!((nBin-1) % 5) && !(i % 5))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 5) && (i % 5))
                            sTicks += "\\n";
                        else if (!((nBin-1) % 7) && !(i % 7))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 7) && (i % 7))
                            sTicks += "\\n";
                        else if (!((nBin-1) % 11) && !(i % 11))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else if (!((nBin-1) % 11) && (i % 11))
                            sTicks += "\\n";
                        else if (((nBin-1) % 2 && (nBin-1) % 3 && (nBin-1) % 5 && (nBin-1) % 7 && (nBin-1) % 11) && !(i % 3))
                            sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                        else
                            sTicks += "\\n";
                    }
                    else
                        sTicks += toString(_mAxisVals[0].a[i]/dCommonExponent, 3) + "\\n";
                }
                //dTicksVal[nBin-1] = dMax-dIntervallLength/2.0;
                sTicks += toString(_mAxisVals[0].a[nBin-1]/dCommonExponent, 3);

                if (sCommonExponent.length())
                {
                    while (sTicks.find(sCommonExponent) != string::npos)
                        sTicks.erase(sTicks.find(sCommonExponent), sCommonExponent.length());

                    if (sCommonExponent.find('-') != string::npos)
                    {
                        sCommonExponent = "\\times 10^{-" + sCommonExponent.substr(sCommonExponent.find_first_not_of('0',2)) + "}";
                    }
                    else
                    {
                        sCommonExponent = "\\times 10^{" + sCommonExponent.substr(sCommonExponent.find_first_not_of('0',2)) + "}";
                    }
                }
            }
            // --> Lege nun die Ausgabematrix an (beruecksichtige dabei auch die Tabellenkoepfe) <--
            if (bGrid)
                nDataRowFinal = nDataRow+1;
            sOut = new string*[nBin+1];
            for (int i = 0; i < nBin+1; i++)
            {
                sOut[i] = new string[nDataRowFinal-nDataRow+1];
                for (int j = 0; j < nDataRowFinal-nDataRow+1; j++)
                {
                    sOut[i][j] = "";
                }
            }

            if (_option.getbDebug())
                cerr << "|-> DEBUG: Array initialisiert ..." << endl;

            //cerr << sDatatable << endl;


            // --> Schreibe Tabellenkoepfe <--
            sOut[0][0] = sBinLabel;
            if (bGrid)
                sOut[0][1] = condenseText(sCountLabel) + ":_grid";
            else
            {
                for (int i = 1; i < nDataRowFinal-nDataRow+1; i++)
                {
                    sOut[0][i] = condenseText(sCountLabel) + ":_" + _data.getTopHeadLineElement(i+nDataRow-1,sDatatable);
                }
            }
            // --> Setze die ueblichen Ausgabe-Info-Parameter <--
            if (!bWriteToCache || matchParams(sCmd, "export", '=') || matchParams(sCmd, "save", '='))
            {
                _out.setPluginName(_lang.get("HIS_OUT_PLGNINFO", PI_HIST, toString(nDataRow+1), toString(nDataRowFinal), _data.getDataFileName(sDatatable)));
                //_out.setPluginName("Histogramm (v " + PI_HIST + ") unter Verwendung der Datenreihe(n) " + toString(nDataRow+1) + "-" + toString(nDataRowFinal) + " aus " + _data.getDataFileName(sDatatable));
                if (bGrid)
                    _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE", toString(dMinZ, 5), toString(dMaxZ,5), toString(dIntervallLength, 5)));
                    //_out.setCommentLine("Die Bins bezeichnen immer die Mitte einer Kategorie. Der minimale Wert ist " + toString(dMinZ, _option) + ", der maximale " + toString(dMaxZ, _option) + ". Die Breite einer Kategorie ist " + toString(dIntervallLength, _option) + ".");
                else
                    _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE", toString(dMin, 5), toString(dMax,5), toString(dIntervallLength, 5)));

                _out.setPrefix("hist");
            }
            if (_out.isFile())
            {
                _out.setStatus(true);
                _out.setCompact(false);
                if (sHistSavePath.length())
                    _out.setFileName(sHistSavePath);
                else
                    _out.generateFileName();
            }
            else
                _out.setCompact(_option.getbCompact());

            // --> Fuelle die Ausgabe-Matrix <--
            for (int i = 1; i < nBin+1; i++)
            {
                // --> Die erste Spalte enthaelt immer die linke Grenze des Bin-Intervalls <--
                sOut[i][0] = toString(_mAxisVals[0].a[i-1], _option);
                for (int j = 0; j < nDataRowFinal-nDataRow; j++)
                {
                    sOut[i][j+1] = toString(dHistMatrix[i-1][j], _option);
                }
            }

            if (bWriteToCache)
            {
                _target.setCacheStatus(true);
                int nFirstCol = _target.getCacheCols(sTargettable, false);

                _target.setHeadLineElement(nFirstCol, sTargettable, "Bins");
                //cerr << "error" << endl;

                for (int i = 0; i < nBin; i++)
                {
                    _target.writeToCache(i, nFirstCol, sTargettable, dMin + i*dIntervallLength + dIntervallLength/2.0);
                    for (int j = 0; j < nDataRowFinal-nDataRow; j++)
                    {
                        if (!i)
                        {
                            //_data.setHeadLineElement(nFirstCol+j+1, "Counts_"+toString(j+nDataRow+1));
                            _target.setHeadLineElement(nFirstCol+j+1, sTargettable, sOut[0][j+1]);
                        }
                        _target.writeToCache(i, nFirstCol+j+1, sTargettable, dHistMatrix[i][j]);
                        //cerr << nFirstCol+j+1 << "  "  << (sDatatable == "data" ? "cache" : sDatatable) << endl;
                    }
                }
                _target.setCacheStatus(false);
            }


            // --> Uebergabe an Output::format(string**,int,int,Settings&), das den Rest erledigt
            if (!bWriteToCache || matchParams(sCmd, "save", '=') || matchParams(sCmd, "export", '='))
            {
                /*if (!_out.isFile() && !bSilent && _option.getSystemPrintStatus())
                    cerr << "|" << endl;*/
                if (_out.isFile() || (!bSilent && _option.getSystemPrintStatus()))
                {
                    if (!_out.isFile())
                    {
                        NumeReKernel::toggleTableStatus();
                        make_hline();
                        NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("HIST_HEADLINE"))));
                        make_hline();
                    }
                    _out.format(sOut, nDataRowFinal-nDataRow+1, nBin+1, _option, true);
                    if (!_out.isFile())
                    {
                        NumeReKernel::toggleTableStatus();
                        make_hline();
                    }
                }
            }

            // --> WICHTIG: Speicher wieder freigeben! <--
            for (int i = 0; i < nBin+1; i++)
            {
                delete[] sOut[i];
            }
            delete[] sOut;
            sOut = 0;
            for (int i = 0; i < nBin; i++)
            {
                delete[] dHistMatrix[i];
            }
            delete[] dHistMatrix;
            dHistMatrix = 0;


            if (bGrid)
            {
                dMin = dMinZ;
                dMax = dMaxZ;
            }

            if (_pData.getxLogscale() && dMin <= 0.0 && dMax > 0.0)
                dMin = dMax / 1e3;
            else if (_pData.getxLogscale() && dMin < 0.0 && dMax <= 0.0)
            {
                dMin = 1.0;
                dMax = 1.0;
            }
            if (_pData.getyLogscale())
                _histGraph->SetRanges(dMin, dMax, 0.1, 1.4*(double)nMax);
            else
                _histGraph->SetRanges(dMin, dMax, 0.0, 1.4*(double)nMax);
            if (_pData.getAxis())
            {
                if (!_pData.getxLogscale())
                    _histGraph->SetTicksVal('x', _mAxisVals[0], sTicks.c_str());
                    //_histGraph->SetTicksVal('x', mglData(nBin, dTicksVal), sTicks.c_str());
                if (!_pData.getBox() && dMin <= 0.0 && dMax >= 0.0 && !_pData.getyLogscale())
                {
                    //_histGraph->SetOrigin(0.0,0.0);
                    if (nBin > 40)
                        _histGraph->Axis("UAKDTVISO");
                    else
                        _histGraph->Axis("AKDTVISO");
                }
                else if (!_pData.getBox())
                {
                    if (nBin > 40)
                        _histGraph->Axis("UAKDTVISO");
                    else
                        _histGraph->Axis("AKDTVISO");
                }
                else
                {
                    if (nBin > 40)
                        _histGraph->Axis("U");
                    else
                        _histGraph->Axis();
                }
            }
            if (_pData.getBox())
                _histGraph->Box();

            if (_pData.getAxis())
            {
                _histGraph->Label('x', sBinLabel.c_str(), 0.0);
                if (sCommonExponent.length() && !_pData.getxLogscale())
                {
                    _histGraph->Puts(mglPoint(dMax+(dMax-dMin)/10.0), mglPoint(dMax+(dMax-dMin)/10.0+1), sCommonExponent.c_str(), ":TL", -1.3);
                }
                if (_pData.getBox())
                    _histGraph->Label('y', sCountLabel.c_str(), 0.0);
                else
                    _histGraph->Label('y', sCountLabel.c_str(), 1.1);
            }
            if (_pData.getGrid())
            {
                if (_pData.getGrid() == 2)
                {
                    _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
                    _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
                }
                else
                    _histGraph->Grid("xy", _pData.getGridStyle().c_str());
            }

            if (!_pData.getBox())
                _histGraph->Legend(1.25,1.0);
            else
                _histGraph->Legend(_pData.getLegendPosition());
            sHistSavePath = _out.getFileName();

            // --> Ausgabe-Info-Parameter loeschen und ggf. bFile = FALSE setzen <--
            if (dTicksVal)
                delete[] dTicksVal;

            if (_option.getSystemPrintStatus() && !bSilent)
                NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("HIST_GENERATING_PLOT") + " ... "));
            if (_out.isFile())
                sHistSavePath = sHistSavePath.substr(0, sHistSavePath.length()-4) + ".png";
            else
                sHistSavePath = _option.ValidFileName("<plotpath>/histogramm", ".png");
            string sColor = "";
            nStyle = 0;
            for (int i = 0; i < nDataRowFinal-nDataRow; i++)
            {
                sColor += sColorStyles[nStyle];
                if (nStyle == nStyleMax-1)
                    nStyle = 0;
                else
                    nStyle++;
            }

            _histGraph->Bars(_mAxisVals[0], _histData, sColor.c_str());

            if (_pData.getOpenImage() && !_pData.getSilentMode() && !bSilent)
            {
                GraphHelper* _graphHelper = new GraphHelper(_histGraph, _pData);
                _graphHelper->setAspect(dAspect);
                NumeReKernel::updateGraphWindow(_graphHelper);
                _histGraph = nullptr;
                NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
            }
            else
            {
                _histGraph->WriteFrame(sHistSavePath.c_str());
                if (_option.getSystemPrintStatus() && !bSilent)
                {
                    NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
                    NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("HIST_SAVED_AT", sHistSavePath), _option)+"\n");
                }
            }
            /*if (_pData.getOpenImage() && !(_pData.getSilentMode() || bSilent))
            {
                string sFileName = sHistSavePath;
                if (sFileName.find('/') != string::npos)
                    sFileName = sFileName.substr(sFileName.rfind('/')+1, sFileName.length()-sFileName.rfind('/')-1);
                if (sFileName.find('\\') != string::npos)
                    sFileName = sFileName.substr(sFileName.rfind('\\')+1, sFileName.length()-sFileName.rfind('\\')-1);
                string sPath = sHistSavePath.substr(0,sHistSavePath.length()-sFileName.length()-1);

                //cerr << "|-> Viewer-Fenster schliessen, um fortzufahren ... ";
                NumeReKernel::gotoLine(sPath+"/"+sFileName);
                //openExternally(sPath + "/" + sFileName, _option.getViewerPath(), sPath);
                //cerr << "OK" << endl;
            }*/
            _out.reset();
            _data.setCacheStatus(false);
        }
        else
        {
            if (_data.getCols(sDatatable) > 1 && !nDataRow)		// Besteht der Datensatz aus mehr als eine Reihe? -> Welche soll denn dann verwendet werden?
            {
                delete _histGraph;
                throw SyntaxError(SyntaxError::NO_COLS, sCmd, SyntaxError::invalid_position);
            }

            if (nDataRow >= _data.getCols(sDatatable)+1)
            {
                nDataRowFinal = _data.getCols(sDatatable);
                nDataRow = 0;
            }
            else if (nDataRow && nDataRowFinal)
            {
                nDataRow--;
            }
            else if (nDataRow)
            {
                nDataRowFinal = nDataRow;
                nDataRow--;				// Dekrement aufgrund des natuerlichen Index-Shifts
            }
            else
            {
                nDataRowFinal = 1;
            }

            if (nDataRowFinal-nDataRow < 3)
            {
                delete _histGraph;
                throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
            }
            if (isnan(dMin) && isnan(dMax))
            {
                dMin = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1);
                dMax = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1);

                if (nDataRowFinal-nDataRow == 3)
                {
                    double dIntervall = dMax - dMin;
                    dMax += dIntervall / 10.0;
                    dMin -= dIntervall / 10.0;
                }
            }
            else if (isnan(dMin))
                dMin = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1);
            else if (isnan(dMax))
                dMax = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1);

            if (dMax < dMin)
            {
                double dTemp = dMax;
                dMax = dMin;
                dMin = dTemp;
            }

            if (!isnan(dMin) && !isnan(dMax))
            {
                if (dMin > _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1)
                    || dMax < _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow,nDataRow+1))
                {
                    delete _histGraph;
                    throw SyntaxError(SyntaxError::INVALID_INTERVAL, sCmd, SyntaxError::invalid_position);
                }
            }

            if (_pData.getxLogscale() && dMax < 0.0)
            {
                delete _histGraph;
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
            }
            else if (_pData.getxLogscale())
            {
                if (dMin < 0.0)
                    dMin = (1e-2*dMax < 1e-2 ? 1e-2*dMax : 1e-2);
            }

            if (isnan(dMinY) && isnan(dMaxY))
            {
                dMinY = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2);
                dMaxY = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2);

                if (nDataRowFinal-nDataRow == 3)
                {
                    double dIntervall = dMaxY - dMinY;
                    dMaxY += dIntervall / 10.0;
                    dMinY -= dIntervall / 10.0;
                }
            }
            else if (isnan(dMinY))
                dMinY = _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2);
            else if (isnan(dMaxY))
                dMaxY = _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2);

            if (dMaxY < dMinY)
            {
                double dTemp = dMaxY;
                dMaxY = dMinY;
                dMinY = dTemp;
            }

            if (!isnan(dMinY) && !isnan(dMaxY))
            {
                if (dMinY > _data.max(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2)
                    || dMaxY < _data.min(sDatatable,0,_data.getLines(sDatatable),nDataRow+1,nDataRow+2))
                {
                    delete _histGraph;
                    throw SyntaxError(SyntaxError::INVALID_INTERVAL, sCmd, SyntaxError::invalid_position);
                }
            }

            if (_pData.getyLogscale() && dMaxY < 0.0)
            {
                delete _histGraph;
                throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
            }
            else if (_pData.getyLogscale())
            {
                if (dMinY < 0.0)
                    dMinY = (1e-2*dMaxY < 1e-2 ? 1e-2*dMaxY : 1e-2);
            }

            for (long long int i = 0; i < _data.getLines(sDatatable); i++)
            {
                for (int j = nDataRow+2; j < nDataRowFinal; j++)
                {
                    if (_data.isValidEntry(i,j, sDatatable)
                        && _data.getElement(i,nDataRow, sDatatable) <= dMax
                        && _data.getElement(i,nDataRow, sDatatable) >= dMin
                        && _data.getElement(i,nDataRow+1, sDatatable) <= dMaxY
                        && _data.getElement(i,nDataRow+1, sDatatable) >= dMinY)
                        nMax++;
                }
            }

            if (!nBin && dIntervallLength == 0.0)
            {
                if (!nMethod)
                    nBin = (int)rint(1.0+3.3*log10((double)nMax/(double)(nDataRowFinal-nDataRow-2)));
                else if (nMethod == 1)
                        dIntervallLength = 3.49*_data.std(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow)/pow((double)nMax/(double)(nDataRowFinal-nDataRow),1.0/3.0);
                else if (nMethod == 2)
                    dIntervallLength = 2.0*(_data.pct(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow, -1, 0.75) - _data.pct(sDatatable, 0, _data.getLines(sDatatable, false), nDataRow, -1, 0.25))/pow((double)nMax/(double)(nDataRowFinal-nDataRow),1.0/3.0);

                if(_option.getbDebug())
                    cerr << "|-> DEBUG: nBin = " << nBin << endl;
            }
            if (nBin)
                _histData.Create(nBin);
            else
            {
                if (dIntervallLength == 0.0)
                {
                    NumeReKernel::print(toSystemCodePage(_lang.get("HIST_ASK_BINWIDTH")));
                    NumeReKernel::printPreFmt("|\n|<- ");

                    cin >> dIntervallLength;


                    if (_option.getbDebug())
                        cerr << "|-> DEBUG: dIntervallLength = " << dIntervallLength << endl;
                }
                // --> Gut. Dann berechnen wir daraus die Anzahl der Bins -> Es kann nun aber sein, dass der letzte Bin ueber
                //     das Intervall hinauslaeuft <--
                if (dIntervallLength > dMax - dMin)
                {
                    delete _histGraph;
                    throw SyntaxError(SyntaxError::TOO_LARGE_BINWIDTH, sCmd, SyntaxError::invalid_position);
                }
                for (int i = 0; (i * dIntervallLength)+dMin < dMax+dIntervallLength; i++)
                {
                    nBin++;
                }
                double dDiff = nBin*dIntervallLength-(double)(dMax-dMin);
                dMin -= dDiff/2.0;
                dMax += dDiff/2.0;

                if (_option.getbDebug())
                    cerr << "|-> DEBUG: nBin = " << nBin << endl;

                if (nBin)
                    _histData.Create(nBin);
            }

            _mAxisVals[0].Create(nBin);
            _mAxisVals[1].Create(nBin);
            if (dIntervallLength == 0.0)
            {
                // --> Berechne die Intervall-Laenge (Explizite Typumwandlung von int->double) <--
                if (_pData.getxLogscale())
                    dIntervallLength = (log10(dMax) - log10(dMin)) / (double)nBin;
                else
                    dIntervallLength = abs(dMax - dMin) / (double)nBin;

                if (_option.getbDebug())
                    cerr << "|-> DEBUG: dIntervallLength = " << dIntervallLength << endl;
            }
            if (dIntervallLengthY == 0.0)
            {
                // --> Berechne die Intervall-Laenge (Explizite Typumwandlung von int->double) <--
                if (_pData.getyLogscale())
                    dIntervallLengthY = (log10(dMaxY) - log10(dMinY)) / (double)nBin;
                else
                    dIntervallLengthY = abs(dMaxY - dMinY) / (double)nBin;

                if (_option.getbDebug())
                    cerr << "|-> DEBUG: dIntervallLengthY = " << dIntervallLengthY << endl;
            }
            //cerr << -2 << endl;

            if (!bWriteToCache || matchParams(sCmd, "export", '=') || matchParams(sCmd, "save", '='))
            {
                _out.setPluginName("2D-"+_lang.get("HIS_OUT_PLGNINFO", PI_HIST, toString(nDataRow+1), toString(nDataRowFinal), _data.getDataFileName(sDatatable)));
                _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE2D", toString(dMin, 5), toString(dMax, 5), toString(dIntervallLength, 5), toString(dMinY, 5), toString(dMaxY, 5), toString(dIntervallLengthY, 5)));
                //_out.setCommentLine("Die Bins bezeichnen immer die Mitte einer Kategorie. Der minimale x-Wert ist " + toString(dMin, _option) + ", der maximale " + toString(dMax, _option) + ". Die Breite einer x-Kategorie ist " + toString(dIntervallLength, _option) + ". Der minimale y-Wert ist " + toString(dMinY, _option) + ", der maximale " + toString(dMaxY, _option) + ". Die Breite einer y-Kategorie ist " + toString(dIntervallLengthY, _option) + ".");

                _out.setPrefix("hist2d");
            }
            if (_out.isFile())
            {
                _out.setStatus(true);
                _out.setCompact(false);
                if (sHistSavePath.length())
                    _out.setFileName(sHistSavePath);
                else
                    _out.generateFileName();
            }
            else
                _out.setCompact(_option.getbCompact());
            if (nDataRowFinal-nDataRow == 3)
            {
                for (unsigned int i = 0; i < 3; i++)
                {
                    _hist2DData[i].Create(_data.getLines(sDatatable, false));
                }
                for (long long int i = 0; i < _data.getLines(sDatatable, false); i++)
                {
                    if (_data.isValidEntry(i,nDataRow, sDatatable)
                        && _data.isValidEntry(i,nDataRow+1, sDatatable)
                        && _data.isValidEntry(i,nDataRow+2, sDatatable)
                        && _data.getElement(i,nDataRow, sDatatable) <= dMax
                        && _data.getElement(i,nDataRow, sDatatable) >= dMin
                        && _data.getElement(i,nDataRow+1, sDatatable) <= dMaxY
                        && _data.getElement(i,nDataRow+1, sDatatable) >= dMinY)
                    {
                        _hist2DData[0].a[i] = _data.getElement(i, nDataRow, sDatatable);
                        _hist2DData[1].a[i] = _data.getElement(i, nDataRow+1, sDatatable);
                        _hist2DData[2].a[i] = _data.getElement(i, nDataRow+2, sDatatable);
                    }
                    else
                    {
                        for (unsigned int k = 0; k < 3; k++)
                            _hist2DData[k].a[i] = NAN;
                    }
                }
            }
            else
            {
                bSum = true;
                _hist2DData[0].Create(_data.getLines(sDatatable, false));
                _hist2DData[1].Create(_data.getLines(sDatatable, false));
                _hist2DData[2].Create(_data.getLines(sDatatable, false), nDataRowFinal-nDataRow-2);
                for (long long int i = 0; i < _data.getLines(sDatatable); i++)
                {
                    _hist2DData[0].a[i] = _data.getElement(i,nDataRow, sDatatable);
                    _hist2DData[1].a[i] = _data.getElement(i,nDataRow+1, sDatatable);
                    for (int j = 0; j < nDataRowFinal-nDataRow-2; j++)
                    {
                        if (_data.isValidEntry(i,j+nDataRow+2, sDatatable)
                            && _data.getElement(i,nDataRow, sDatatable) <= dMax
                            && _data.getElement(i,nDataRow, sDatatable) >= dMin
                            && _data.getElement(i,nDataRow+1, sDatatable) <= dMaxY
                            && _data.getElement(i,nDataRow+1, sDatatable) >= dMinY)
                            _hist2DData[2].a[i+j*_data.getLines(sDatatable)] = _data.getElement(i,j+nDataRow+2,sDatatable);
                        else
                            _hist2DData[2].a[i+j*_data.getLines(sDatatable)] = NAN;
                    }
                }

            }
            //cerr << -1 << endl;
            if (bWriteToCache)
                nMax = _target.getCols(sTargettable);
            sOut = new string*[nBin+1];
            for (int k = 0; k < nBin+1; k++)
                sOut[k] = new string[4];
            for (int k = 0; k < nBin; k++)
            {
                dSum = 0.0;
                for (int i = 0; i < _data.getLines(sDatatable); i++)
                {
                    if (_data.isValidEntry(i,nDataRow,sDatatable)
                        && ((!_pData.getxLogscale()
                                && _data.getElement(i,nDataRow, sDatatable) >= dMin + k*dIntervallLength
                                && _data.getElement(i,nDataRow, sDatatable) < dMin + (k+1)*dIntervallLength)
                            || (_pData.getxLogscale()
                                && _data.getElement(i,nDataRow, sDatatable) >= pow(10.0, log10(dMin) + k*dIntervallLength)
                                && _data.getElement(i,nDataRow, sDatatable) < pow(10.0, log10(dMin) + (k+1)*dIntervallLength))
                            )
                        )
                    {
                        if (nDataRowFinal-nDataRow == 3)
                        {
                            if (_data.isValidEntry(i,nDataRow+1,sDatatable)
                                && _data.isValidEntry(i,nDataRow+2,sDatatable)
                                && _data.getElement(i,nDataRow+1,sDatatable) >= dMinY
                                && _data.getElement(i,nDataRow+1,sDatatable) <= dMaxY)
                            {
                                if (bSum)
                                    dSum += _data.getElement(i,nDataRow+2,sDatatable);
                                else
                                    dSum++;
                            }
                        }
                        else
                        {
                            for (int l = 0; l < _data.getLines(sDatatable); l++)
                            {
                                if (_data.isValidEntry(l,nDataRow+1,sDatatable)
                                    && _data.isValidEntry(i,l+2,sDatatable)
                                    && _data.getElement(l,nDataRow+1, sDatatable) >= dMinY
                                    && _data.getElement(l,nDataRow+1, sDatatable) <= dMaxY)
                                    dSum += _data.getElement(i,l+2,sDatatable);
                            }
                        }
                    }
                }
                _histData.a[k] = dSum;
                if (!_pData.getxLogscale())
                    _mAxisVals[0].a[k] = dMin + (k+0.5)*dIntervallLength;
                else
                    _mAxisVals[0].a[k] = pow(10.0, log10(dMin)+(k+0.5)*dIntervallLength);
                sOut[k+1][0] = toString(_mAxisVals[0].a[k], _option);
                sOut[k+1][1] = toString(dSum, _option);
                if (!k)
                {
                    sOut[k][0] = "Bins_[x]";
                    sOut[k][1] = (bSum ? "Sum_[x]" : "Counts_[x]");
                }
                if (bWriteToCache)
                {
                    if (!k)
                    {
                        _target.setHeadLineElement(nMax, sTargettable, "Bins_[x]");
                        _target.setHeadLineElement(nMax+1, sTargettable, (bSum ? "Sum_[x]" : "Counts_[x]"));
                    }
                    _target.writeToCache(k, nMax, sTargettable, _mAxisVals[0].a[k]);
                    _target.writeToCache(k, nMax+1, sTargettable, dSum);
                }
                /*for (int i = 0; i < nDataRowFinal-nDataRow-2; i++)
                {
                    for (int k = 0; k < nBin; k++)
                    {
                        dSum = 0.0;
                        for (int l = 0; l < _data.getLines(sDatatable, true) - _data.getAppendedZeroes(i+nDataRow,sDatatable); l++)
                        {
                            if (_data.getElement(l,nDataRow, sDatatable) >= dMin + k * dIntervallLength
                                    && _data.getElement(l,nDataRow, sDatatable) < dMin + (k+1) * dIntervallLength)
                                dSum += _data.getElement(l,i+nDataRow+2, sDatatable);
                        }
                        _histData.a[k] += dSum;
                    }
                }*/
            }

            _histGraph->MultiPlot(3,3,0,2,1,"<>");
            _histGraph->SetBarWidth(0.9);
            _histGraph->SetTuneTicks(3,1.05);
            //_histGraph->SubPlot(2,2,0);
            dIntervallLength = _histData.Maximal()-_histData.Minimal();

            _histGraph->SetRanges(1,2,1,2,1,2);
            if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("lg(x)", "");
            else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("lg(x)", "");
            else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("", "");
            else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("lg(x)","lg(y)");
            else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("", "lg(y)");
            else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("","lg(y)");
            else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("lg(x)","lg(y)");

            if (_histData.Minimal() >= 0)
            {
                if (_pData.getzLogscale() && _histData.Maximal() > 0.0)
                {
                    if (_histData.Minimal() - dIntervallLength/10.0 > 0)
                        _histGraph->SetRanges(dMin, dMax, _histData.Minimal() - dIntervallLength/10.0, _histData.Maximal()+dIntervallLength/10.0);
                    else if (_histData.Minimal() > 0)
                        _histGraph->SetRanges(dMin, dMax, _histData.Minimal()/2.0, _histData.Maximal()+dIntervallLength/10.0);
                    else
                        _histGraph->SetRanges(dMin, dMax, (1e-2*_histData.Maximal() < 1e-2 ? 1e-2*_histData.Maximal() : 1e-2), _histData.Maximal()+dIntervallLength/10.0);
                }
                else if (_histData.Maximal() < 0.0 && _pData.getzLogscale())
                {
                    for (int k = 0; k < nBin+1; k++)
                        delete[] sOut[k];
                    delete[] sOut;
                    delete _histGraph;
                    throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
                }
                else
                    _histGraph->SetRanges(dMin, dMax, 0.0, _histData.Maximal()+dIntervallLength/10.0);
            }
            else
                _histGraph->SetRanges(dMin, dMax, _histData.Minimal()-dIntervallLength/10.0, _histData.Maximal()+dIntervallLength/10.0);


            _histGraph->Box();
            _histGraph->Axis();

            if (_pData.getGrid() == 1)
                _histGraph->Grid("xy", _pData.getGridStyle().c_str());
            else if (_pData.getGrid() == 2)
            {
                _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
                _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
            }
            _histGraph->Label('x', sAxisLabels[0].c_str(), 0);
            if (bSum)
                _histGraph->Label('y', sAxisLabels[2].c_str(), 0);
            else
                _histGraph->Label('y', sCountLabel.c_str(), 0);

            _histGraph->Bars(_mAxisVals[0], _histData, _pData.getColors().c_str());
            //cerr << 2 << endl;
            nMax = 0;

            _histGraph->MultiPlot(3,3,3,2,2,"<_>");
            _histGraph->SetTuneTicks(3,1.05);

            _histGraph->SetRanges(1,2,1,2,1,2);
            if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("lg(x)", "");
            else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("lg(x)", "lg(y)");
            else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("", "lg(y)");
            else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("lg(x)","", "lg(z)");
            else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("","", "lg(z)");
            else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("","lg(y)", "lg(z)");
            else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("lg(x)","lg(y)", "lg(z)");

            //_histGraph->SubPlot(2,2,2);
            //cerr << 0 << endl;
            if (nDataRowFinal-nDataRow == 3)
            {
                if (_hist2DData[2].Maximal() < 0 && _pData.getzLogscale())
                {
                    for (int k = 0; k < nBin+1; k++)
                        delete[] sOut[k];
                    delete[] sOut;
                    delete _histGraph;
                    throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
                }
                if (_pData.getzLogscale())
                {
                    if (_hist2DData[2].Minimal() > 0.0)
                        _histGraph->SetRanges(dMin, dMax, dMinY, dMaxY, _hist2DData[2].Minimal(), _hist2DData[2].Maximal());
                    else
                        _histGraph->SetRanges(dMin, dMax, dMinY, dMaxY, (1e-2*_hist2DData[2].Maximal() < 1e-2 ? 1e-2*_hist2DData[2].Maximal() : 1e-2), _hist2DData[2].Maximal());
                }
                else
                    _histGraph->SetRanges(dMin, dMax, dMinY, dMaxY, _hist2DData[2].Minimal(), _hist2DData[2].Maximal());
                _histGraph->Box();
                _histGraph->Axis("xy");
                _histGraph->Colorbar(_pData.getColorScheme("I>").c_str());
                if (_pData.getGrid() == 1)
                    _histGraph->Grid("xy", _pData.getGridStyle().c_str());
                else if (_pData.getGrid() == 2)
                {
                    _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
                    _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
                }
                _histGraph->Label('x', sAxisLabels[0].c_str(), 0);
                _histGraph->Label('y', sAxisLabels[1].c_str(), 0);
                _histGraph->Dots(_hist2DData[0], _hist2DData[1], _hist2DData[2], _pData.getColorScheme().c_str());
            }
            else
            {
                if (_hist2DData[2].Maximal() < 0 && _pData.getzLogscale())
                {
                    for (int k = 0; k < nBin+1; k++)
                        delete[] sOut[k];
                    delete[] sOut;
                    delete _histGraph;
                    throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
                }
                if (_pData.getzLogscale())
                {
                    if (_hist2DData[2].Minimal() > 0.0)
                        _histGraph->SetRanges(dMin, dMax, dMinY, dMaxY, _hist2DData[2].Minimal(), _hist2DData[2].Maximal());
                    else
                        _histGraph->SetRanges(dMin, dMax, dMinY, dMaxY, (1e-2*_hist2DData[2].Maximal() < 1e-2 ? 1e-2*_hist2DData[2].Maximal() : 1e-2), _hist2DData[0].Maximal());
                }
                else
                    _histGraph->SetRanges(dMin, dMax, dMinY, dMaxY, _hist2DData[2].Minimal(), _hist2DData[2].Maximal());
                _histGraph->Box();
                _histGraph->Axis("xy");
                _histGraph->Colorbar(_pData.getColorScheme("I>").c_str());
                if (_pData.getGrid() == 1)
                    _histGraph->Grid("xy", _pData.getGridStyle().c_str());
                else if (_pData.getGrid() == 2)
                {
                    _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
                    _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
                }
                _histGraph->Label('x', sAxisLabels[0].c_str(), 0);
                _histGraph->Label('y', sAxisLabels[1].c_str(), 0);

                _histGraph->Dens(_hist2DData[0], _hist2DData[1], _hist2DData[2], _pData.getColorScheme().c_str());
            }

            if (bWriteToCache)
                nMax = _target.getCols(sTargettable);
            for (int k = 0; k < nBin; k++)
            {
                dSum = 0.0;
                for (int i = 0; i < _data.getLines(sDatatable); i++)
                {
                    if (_data.isValidEntry(i,nDataRow+1,sDatatable)
                        && ((!_pData.getyLogscale()
                                && _data.getElement(i,nDataRow+1, sDatatable) >= dMinY + k*dIntervallLengthY
                                && _data.getElement(i,nDataRow+1, sDatatable) < dMinY + (k+1)*dIntervallLengthY)
                            || (_pData.getyLogscale()
                                && _data.getElement(i,nDataRow+1, sDatatable) >= pow(10.0, log10(dMinY) + k*dIntervallLengthY)
                                && _data.getElement(i,nDataRow+1, sDatatable) < pow(10.0, log10(dMinY) + (k+1)*dIntervallLengthY))
                            )
                        )
                    {
                        if (nDataRowFinal-nDataRow == 3)
                        {
                            if (_data.isValidEntry(i,nDataRow, sDatatable)
                                && _data.isValidEntry(i,nDataRow+2, sDatatable)
                                && _data.getElement(i,nDataRow, sDatatable) >= dMin
                                && _data.getElement(i,nDataRow, sDatatable) <= dMax)
                            {
                                if (bSum)
                                    dSum += _data.getElement(i,nDataRow+2,sDatatable);
                                else
                                    dSum++;
                            }
                        }
                        else
                        {
                            for (int l = 0; l < _data.getLines(sDatatable); l++)
                            {
                                if (_data.isValidEntry(l,nDataRow,sDatatable)
                                    && _data.isValidEntry(l,i+2,sDatatable)
                                    && _data.getElement(l,nDataRow, sDatatable) >= dMin
                                    && _data.getElement(l,nDataRow, sDatatable) <= dMax)
                                    dSum += _data.getElement(l,i+2,sDatatable);
                            }
                        }
                    }
                }
                _histData.a[k] = dSum;
                if (!_pData.getyLogscale())
                    _mAxisVals[1].a[k] = dMinY + (k+0.5)*dIntervallLengthY;
                else
                    _mAxisVals[1].a[k] = pow(10.0, log10(dMinY)+(k+0.5)*dIntervallLengthY);
                if (bWriteToCache)
                {
                    if (!k)
                    {
                        _target.setHeadLineElement(nMax, sTargettable, "Bins_[y]");
                        _target.setHeadLineElement(nMax+1, sTargettable, (bSum ? "Sum_[y]" : "Counts_[y]"));
                    }
                    _target.writeToCache(k, nMax, sTargettable, _mAxisVals[1].a[k]);
                    _target.writeToCache(k, nMax+1, sTargettable, dSum);
                }
                sOut[k+1][2] = toString(_mAxisVals[1].a[k], _option);
                sOut[k+1][3] = toString(dSum, _option);
                if (!k)
                {
                    sOut[k][2] = "Bins_[y]";
                    sOut[k][3] = (bSum ? "Sum_[y]" : "Counts_[y]");
                }
            }

            _histGraph->MultiPlot(3,3,5,1,2,"_");
            _histGraph->SetBarWidth(0.9);
            _histGraph->SetTuneTicks(3,1.05);
            //_histGraph->SubPlot(2,2,3);
            dIntervallLength = _histData.Maximal()-_histData.Minimal();

            _histGraph->SetRanges(1,2,1,2,1,2);
            if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("", "");
            else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("", "lg(y)");
            else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
                _histGraph->SetFunc("", "lg(y)");
            else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("lg(x)","");
            else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("lg(x)","");
            else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("lg(x)","lg(y)");
            else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
                _histGraph->SetFunc("lg(x)","lg(y)");

            if (_histData.Minimal() >= 0)
            {
                if (_pData.getzLogscale() && _histData.Maximal() > 0.0)
                {
                    if (_histData.Minimal() - dIntervallLength/10.0 > 0)
                        _histGraph->SetRanges(_histData.Minimal() - dIntervallLength/10.0, _histData.Maximal()+dIntervallLength/10.0, dMinY, dMaxY);
                    else if (_histData.Minimal() > 0)
                        _histGraph->SetRanges(_histData.Minimal()/2.0, _histData.Maximal()+dIntervallLength/10.0, dMinY, dMaxY);
                    else
                        _histGraph->SetRanges((1e-2*_histData.Maximal() < 1e-2 ? 1e-2*_histData.Maximal() : 1e-2), _histData.Maximal()+dIntervallLength/10.0, dMinY, dMaxY);
                }
                else if (_histData.Maximal() < 0.0 && _pData.getzLogscale())
                {
                    for (int k = 0; k < nBin+1; k++)
                        delete[] sOut[k];
                    delete[] sOut;
                    delete _histGraph;
                    throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
                }
                else
                    _histGraph->SetRanges(0.0, _histData.Maximal()+dIntervallLength/10.0, dMinY, dMaxY);
            }
            else
                _histGraph->SetRanges(_histData.Minimal()-dIntervallLength/10.0, _histData.Maximal()+dIntervallLength/10.0, dMinY, dMaxY);


            _histGraph->Box();
            _histGraph->Axis("xy");
            if (_pData.getGrid() == 1)
                _histGraph->Grid("xy", _pData.getGridStyle().c_str());
            else if (_pData.getGrid() == 2)
            {
                _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
                _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
            }
            if (!bSum)
                _histGraph->Label('x', sCountLabel.c_str(), 0);
            else
                _histGraph->Label('x', sAxisLabels[2].c_str(), 0);
            _histGraph->Label('y', sAxisLabels[1].c_str(), 0);

            _histGraph->Barh(_mAxisVals[1], _histData, _pData.getColors().c_str());
            //cerr << 4 << endl;
            /*if (_pData.getAxis())
            {
                if (!_pData.getBox() && dMin <= 0.0 && dMax >= 0.0 && !_pData.getyLogscale())
                {
                    _histGraph->SetOrigin(0.0,0.0);
                    if (nBin > 40)
                        _histGraph->Axis("UAKDTVISO");
                    else
                        _histGraph->Axis("AKDTVISO");
                }
                else if (!_pData.getBox())
                {
                    if (nBin > 40)
                        _histGraph->Axis("UAKDTVISO");
                    else
                        _histGraph->Axis("AKDTVISO");
                }
                else
                {
                    if (nBin > 40)
                        _histGraph->Axis("U");
                    else
                        _histGraph->Axis();
                }
            }
            if (_pData.getBox())
                _histGraph->Box();

            if (_pData.getAxis())
            {
                _histGraph->Label('x', sBinLabel.c_str(), 0.0);
                if (sCommonExponent.length() && !_pData.getxLogscale())
                {
                    _histGraph->Puts(mglPoint(dMax+(dMax-dMin)/10.0), mglPoint(dMax+(dMax-dMin)/10.0+1), sCommonExponent.c_str(), ":TL", -1.3);
                }
                if (_pData.getBox())
                    _histGraph->Label('y', sCountLabel.c_str(), 0.0);
                else
                    _histGraph->Label('y', sCountLabel.c_str(), 1.5);
            }
            if (_pData.getGrid())
            {
                if (_pData.getGrid() == 2)
                {
                    _histGraph->Grid("xy!", "=h");
                    _histGraph->Grid("xy", "-h");
                }
                else
                    _histGraph->Grid("xy", "=h");
            }

            if (!_pData.getBox())
                _histGraph->Legend(1.25,1.0);
            else
                _histGraph->Legend();*/


            if (!bWriteToCache || matchParams(sCmd, "save", '=') || matchParams(sCmd, "export", '='))
            {
                /*if (!_out.isFile() && _option.getSystemPrintStatus() && !bSilent)
                    cerr << "|" << endl;*/
                if (_out.isFile() || (_option.getSystemPrintStatus() && !bSilent))
                {
                    if (!_out.isFile())
                    {
                        NumeReKernel::toggleTableStatus();
                        make_hline();
                        NumeReKernel::print("NUMERE: 2D-" + toSystemCodePage(toUpperCase(_lang.get("HIST_HEADLINE"))));
                        make_hline();
                    }
                    _out.format(sOut, 4, nBin+1, _option, true);
                    if (!_out.isFile())
                    {
                        NumeReKernel::toggleTableStatus();
                        make_hline();
                    }
                }
            }
            sHistSavePath = _out.getFileName();

            // --> Ausgabe-Info-Parameter loeschen und ggf. bFile = FALSE setzen <--
            if (dTicksVal)
                delete[] dTicksVal;
            for (int k = 0; k < nBin+1; k++)
                delete[] sOut[k];
            delete[] sOut;


            if (_option.getSystemPrintStatus() && !bSilent)
                NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("HIST_GENERATING_PLOT") + " ... "));
            if (_out.isFile())
                sHistSavePath = sHistSavePath.substr(0, sHistSavePath.length()-4) + ".png";
            else
                sHistSavePath = _option.ValidFileName("<plotpath>/histogramm2d", ".png");
            string sColor = "";

            if (_pData.getOpenImage() && !_pData.getSilentMode() && !bSilent)
            {
                GraphHelper* _graphHelper = new GraphHelper(_histGraph, _pData);
                _graphHelper->setAspect(dAspect);
                NumeReKernel::updateGraphWindow(_graphHelper);
                _histGraph = nullptr;
                NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
            }
            else
            {
                _histGraph->WriteFrame(sHistSavePath.c_str());
                if (_option.getSystemPrintStatus() && !bSilent)
                    NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
                if (!_out.isFile() && _option.getSystemPrintStatus() && !bSilent)
                    NumeReKernel::printPreFmt(LineBreak("|   "+_lang.get("HIST_SAVED_AT", sHistSavePath), _option) + "\n");
            }
                /*if (_pData.getOpenImage() && !_pData.getSilentMode())
            {
                string sFileName = sHistSavePath;
                if (sFileName.find('/') != string::npos)
                    sFileName = sFileName.substr(sFileName.rfind('/')+1, sFileName.length()-sFileName.rfind('/')-1);
                if (sFileName.find('\\') != string::npos)
                    sFileName = sFileName.substr(sFileName.rfind('\\')+1, sFileName.length()-sFileName.rfind('\\')-1);
                string sPath = sHistSavePath.substr(0,sHistSavePath.length()-sFileName.length()-1);

                //cerr << "|-> Viewer-Fenster schliessen, um fortzufahren ... ";
                NumeReKernel::gotoLine(sPath+"/"+sFileName);
                //openExternally(sPath + "/" + sFileName, _option.getViewerPath(), sPath);
                //cerr << "OK" << endl;
            }*/
            _out.reset();
            _data.setCacheStatus(false);
        }
    }
	else
	{
		throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
	}

	//cerr << "|-> Das Plugin wurde erfolgreich beendet." << endl;
	return;
}
