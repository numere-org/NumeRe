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
 * Funktionen zur Berechnung von Mittelwert und Standardabweichung
 */



const string PI_MED = "0.2.2";

void plugin_statistics (string& sCmd, Datafile& _data, Output& _out, Settings& _option, bool bUseCache, bool bUseData)
{
    if (_option.getbDebug())
        cerr << "|-> DEBUG: sCmd = " << sCmd << endl;
	double dAverage;		// Variable fuer Mittelwert
	double dError;			// Variable fuer Abweichung
	double dPercentage;		// Variable fuer den Prozentsatz
	double dSkew;
	double dKurt;
	int nCount;				// Zaehlvariable
	const int nPrecision = 4;
	const int nStatsFields = 16;
	double dUnsure; 		// Variable fuer die Unsicherheit
	string sSavePath = "";  // Variable fuer den Speicherpfad

	if(_data.isValid() || _data.isValidCache())	// Sind eigentlich Daten verfuegbar?
	{
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
                    NumeReKernel::print(_lang.get("COMMON_CANCEL") + ".");
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
        else if (bUseCache)
            _data.setCacheStatus(true);


        if (findParameter(sCmd, "save", '=') || findParameter(sCmd, "export", '='))
        {
            int nPos = 0;
            if (findParameter(sCmd, "save", '='))
                nPos = findParameter(sCmd, "save", '=')+4;
            else
                nPos = findParameter(sCmd, "export", '=')+6;
            _out.setStatus(true);
            sSavePath = getArgAtPos(sCmd, nPos);
        }
        if (findParameter(sCmd, "save") || findParameter(sCmd, "export"))
            _out.setStatus(true);
        string sDatatable = "data";
        if (_data.matchTableAsParameter(sCmd).length())
        {
            sDatatable = _data.matchTableAsParameter(sCmd);
        }
        if (!_data.getLines(sDatatable) || !_data.getCols(sDatatable))
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);

		int nLine = _data.getLines(sDatatable);
		int nCol = _data.getCols(sDatatable);
		int nHeadlines = _data.getHeadlineCount(sDatatable);
		// --> Allozieren einer Ausgabe-String-Matrix <--
		string** sOut = new string*[nLine + nStatsFields + nHeadlines];
		string** sOverview = 0;
		for (int i = 0; i < nLine + nStatsFields + nHeadlines; i++)
		{
			sOut[i] = new string[nCol];
		}

		// --> Berechnung fuer jede Spalte der Matrix! <--
		for (int j = 0; j < nCol; j++)
		{
		    if (!_data.num(sDatatable, 0, nLine-1, j))
            {
                sOut[nHeadlines + nLine + 0][j] = "<<SUMBAR>>"; // Schreiben der berechneten Werte in die letzten drei Zeilen der Ausgabe
                sOut[nHeadlines + nLine + 1][j] = _lang.get("STATS_TYPE_AVG") + ": ---";
                sOut[nHeadlines + nLine + 2][j] = _lang.get("STATS_TYPE_STD") + ": ---";
                sOut[nHeadlines + nLine + 3][j] = _lang.get("STATS_TYPE_CONFINT") + ": ---";
                sOut[nHeadlines + nLine + 4][j] = _lang.get("STATS_TYPE_STDERR") + ": ---";
                sOut[nHeadlines + nLine + 5][j] = _lang.get("STATS_TYPE_MED") + ": ---";
                sOut[nHeadlines + nLine + 6][j] = "Q1: ---";
                sOut[nHeadlines + nLine + 7][j] = "Q3: ---";
                sOut[nHeadlines + nLine + 8][j] = _lang.get("STATS_TYPE_RMS") + ": ---";
                sOut[nHeadlines + nLine + 9][j] = _lang.get("STATS_TYPE_SKEW") + ": ---";
                sOut[nHeadlines + nLine + 10][j] = _lang.get("STATS_TYPE_EXCESS") + ": ---";
                sOut[nHeadlines + nLine + 11][j] = "min: ---";
                sOut[nHeadlines + nLine + 12][j] = "max: ---";
                sOut[nHeadlines + nLine + 13][j] = "num: ---";
                sOut[nHeadlines + nLine + 14][j] = "cnt: ---";
                sOut[nHeadlines + nLine + 15][j] = "s_t: ---";
                continue;
            }
			dAverage = _data.avg(sDatatable,0,nLine-1,j); // Wichtig: Nullsetzen aller wesentlichen Variablen!
			dError = _data.std(sDatatable,0,nLine-1,j);
			dPercentage = 0.0;
			dUnsure = 0.0;
			dSkew = 0.0;
			dKurt = 0.0;
			nCount = 0;
			string sHeadline = _data.getHeadLineElement(j, sDatatable);

            for (int i = 0; i < nHeadlines; i++)
            {
                if (sHeadline.length())
                {
                    sOut[i][j] = sHeadline.substr(0, sHeadline.find("\\n"));
                    if (sHeadline.find("\\n") != string::npos)
                        sHeadline.erase(0, sHeadline.find("\\n") + 2);
                    else
                        break;
                }
            }


			for (int i = 0; i < nLine; i++)
			{
				if (!_data.isValidEntry(i,j, sDatatable))
				{
					sOut[i + nHeadlines][j] = "---";
					continue;
				}
				sOut[i + nHeadlines][j] = toString(_data.getElement(i,j, sDatatable),_option); // Kopieren der Matrix in die Ausgabe
			}


			dUnsure = dError / sqrt(_data.num(sDatatable,0,nLine-1,j));

			// --> Wie viele Werte sind nun innerhalb der Abweichung? <--
			for(int i = 0; i < nLine; i++)
			{
				if (!_data.isValidEntry(i,j,sDatatable))
					continue;						// Angehaengte Nullzeilen brauchen hier auch nicht mitgezaehlt werden!
				if (abs(_data.getElement(i,j,sDatatable)-dAverage) < dError)
					nCount++;		// Counter erhoehen
			}

			if(_option.getbDebug())
				cerr << "|-> DEBUG: dError = " << dError << endl;

			// --> Prozentzahl ausrechen... Explizit in einen double umwandeln, damit nicht die integer-Division verwendet wird
			dPercentage = (double)nCount/(double) (_data.num(sDatatable,0,nLine-1,j))*100.0;
			dPercentage *= 100.0;
			dPercentage = round(dPercentage);
			dPercentage /= 100.0;

			for (int i = 0; i < nLine-1; i++)
			{
                if (!_data.isValidEntry(i,j,sDatatable))
                    continue;
                dSkew += (_data.getElement(i,j,sDatatable)-dAverage)*(_data.getElement(i,j,sDatatable)-dAverage)*(_data.getElement(i,j,sDatatable)-dAverage);
                dKurt += (_data.getElement(i,j,sDatatable)-dAverage)*(_data.getElement(i,j,sDatatable)-dAverage)*(_data.getElement(i,j,sDatatable)-dAverage)*(_data.getElement(i,j,sDatatable)-dAverage);
			}

			dSkew /= _data.num(sDatatable,0,nLine-1,j) * (dError*dError*dError);
			dKurt /= _data.num(sDatatable,0,nLine-1,j) * (dError*dError*dError*dError);
			dKurt -= 3;

			sOut[nHeadlines + nLine+0][j] = "<<SUMBAR>>"; // Schreiben der berechneten Werte in die letzten drei Zeilen der Ausgabe
			sOut[nHeadlines + nLine+1][j] = _lang.get("STATS_TYPE_AVG") + ": " + toString(dAverage, nPrecision);
			sOut[nHeadlines + nLine+2][j] = _lang.get("STATS_TYPE_STD") + ": " + toString(dError, nPrecision);
			sOut[nHeadlines + nLine+3][j] = _lang.get("STATS_TYPE_CONFINT") + ": " + toString(dPercentage, nPrecision) + " %";
			sOut[nHeadlines + nLine+4][j] = _lang.get("STATS_TYPE_STDERR") + ": " + toString(dUnsure, nPrecision);
			sOut[nHeadlines + nLine+5][j] = _lang.get("STATS_TYPE_MED") + ": " + toString(_data.med(sDatatable,0,nLine-1,j), nPrecision);
			sOut[nHeadlines + nLine+6][j] = "Q1: " + toString(_data.pct(sDatatable,0,nLine-1,j,-1,0.25), nPrecision);
			sOut[nHeadlines + nLine+7][j] = "Q3: " + toString(_data.pct(sDatatable,0,nLine-1,j,-1,0.75), nPrecision);
			sOut[nHeadlines + nLine+8][j] = _lang.get("STATS_TYPE_RMS") + ": " + toString(_data.norm(sDatatable,0,nLine-1,j)/sqrt(_data.num(sDatatable,0,nLine-1,j)), nPrecision);
			sOut[nHeadlines + nLine+9][j] = _lang.get("STATS_TYPE_SKEW") + ": " + toString(dSkew, nPrecision);
			sOut[nHeadlines + nLine+10][j] = _lang.get("STATS_TYPE_EXCESS") + ": " + toString(dKurt, nPrecision);
			sOut[nHeadlines + nLine+11][j] = "min: " + toString(_data.min(sDatatable,0,nLine-1,j), nPrecision);
			sOut[nHeadlines + nLine+12][j] = "max: " + toString(_data.max(sDatatable,0,nLine-1,j), nPrecision);
			sOut[nHeadlines + nLine+13][j] = "num: " + toString(_data.num(sDatatable,0,nLine-1,j), nPrecision);
			sOut[nHeadlines + nLine+14][j] = "cnt: " + toString(_data.cnt(sDatatable,0,nLine-1,j), nPrecision);
			boost::math::students_t dist(_data.num(sDatatable,0,nLine-1,j));
			sOut[nHeadlines + nLine+15][j] = "s_t: " + toString(boost::math::quantile(boost::math::complement(dist, 0.025)), nPrecision);
		}

		// --> Allgemeine Ausgabe-Info-Parameter setzen <--
		_out.setPluginName(_lang.get("STATS_OUT_PLGNINFO", PI_MED, _data.getDataFileName(sDatatable)));
		//_out.setCommentLine("Legende: MW = Mittelwert, +/- = Standardabweichung, iVInt = im Vertrauensintervall, Uns = Unsicherheit");
		_out.setPrefix("stats");
		// --> Wenn sCmd einen Eintrag enthaelt, dann soll die Ausgabe automatische in eine Datei geschrieben werden <--

        sOverview = new string*[nStatsFields-2 + nHeadlines];

        for (int i = 0; i < nStatsFields-2+nHeadlines; i++)
            sOverview[i] = new string[nCol+1];

        sOverview[0][0] = " ";

        for (int j = 0; j < nCol; j++)
        {
            string sHeadline = _data.getHeadLineElement(j, sDatatable);

            for (int i = 0; i < nHeadlines; i++)
            {
                if (sHeadline.length())
                {
                    sOverview[i][j+1] = sHeadline.substr(0, sHeadline.find("\\n"));
                    if (sHeadline.find("\\n") != string::npos)
                        sHeadline.erase(0, sHeadline.find("\\n") + 2);
                    else
                        break;
                }
            }

            for (int n = 0; n < nStatsFields-2; n++)
            {
                sOverview[n + nHeadlines][j+1] = sOut[nHeadlines + nLine+n+1][j].substr(sOut[nHeadlines + nLine+n+1][j].find(':')+1);

                if (!j)
                    sOverview[n + nHeadlines][0] = sOut[nHeadlines + nLine+n+1][j].substr(0,sOut[nHeadlines + nLine+n+1][j].find(':')+1);
            }
        }

        if (sSavePath.length())
            _out.setFileName(sSavePath);
        else
            _out.generateFileName();

		// --> Formatieren und Schreiben der Ausgabe <--
        //cerr << "|" << endl;
        _out.setCompact(false);
		_out.setCommentLine(_lang.get("STATS_OUT_COMMENTLINE"));

		if (_out.isFile())
            _out.format(sOut, nCol, nLine+nStatsFields+nHeadlines, _option, true, nHeadlines);

        _out.reset();
        _out.setCompact(false);
		_out.setCommentLine(_lang.get("STATS_OUT_COMMENTLINE"));

        NumeReKernel::toggleTableStatus();
        make_hline();
        NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("STATS_HEADLINE"))));
        make_hline();
        _out.format(sOverview, nCol+1, nStatsFields-2+nHeadlines, _option, true, nHeadlines);
        _out.reset();
        NumeReKernel::toggleTableStatus();
        make_hline();
        // --> Speicher wieder freigeben! <--

        for (int i = 0; i < nStatsFields-2+nHeadlines; i++)
            delete[] sOverview[i];

        delete[] sOverview;

		for (int i = 0; i < nLine + nStatsFields + nHeadlines; i++)
		{
			delete[] sOut[i];
		}

		delete[] sOut;
		sOut = 0;
	}
	else				// Oh! Das sind offensichtlich keine Daten vorhanden... Sollte man dem Benutzer wohl mitteilen
	{
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
	}
	//cerr << "|-> Das Plugin wurde erfolgreich beendet." << endl;

	// --> Output-Instanz wieder zuruecksetzen <--
	_out.reset();
	_data.setCacheStatus(false);

	return;
}
