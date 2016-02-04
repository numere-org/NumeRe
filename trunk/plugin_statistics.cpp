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

/*
 * Funktionen zur Berechnung von Mittelwert und Standardabweichung
 */



const string PI_MED = "0.2.2";

void plugin_statistics (string& sCmd, Datafile& _data, Output& _out, Settings& _option, bool bUseCache, bool bUseData)
{
	double dAverage;		// Variable fuer Mittelwert
	double dError;			// Variable fuer Abweichung
	double dPercentage;		// Variable fuer den Prozentsatz
	double dSkew;
	double dKurt;
	int nCount;				// Zaehlvariable
	const int nPrecision = 4;
	const int nStatsFields = 17;
	double dUnsure; 		// Variable fuer die Unsicherheit
	string sSavePath = "";  // Variable fuer den Speicherpfad

	if(_data.isValid() || _data.isValidCache())	// Sind eigentlich Daten verfuegbar?
	{

		//cerr << "|-> STATISTIK (v " << PI_MED << ")" << endl;
		//cerr << "|   " << std::setfill((char)196) << std::setw(19) << (char)196 << endl;
        //cerr << LineBreak("|-> Dieses Plugin berechnet aus einer gegebenen Tabelle Mittelwert, Standardabweichung, Unsicherheit und den Prozentsatz der Datenpunkte im Vertrauensintervall.", _option) << endl;


        if (!bUseCache && !bUseData)
        {
            if (!_data.isValid() && _data.isValidCache())
            {
                cerr << LineBreak("|-> Es sind nur Daten im Cache vorhanden. Es werden automatisch diese Punkte verwendet.", _option) << endl;
                _data.setCacheStatus(true);
            }
            else if (_data.isValid() && _data.isValidCache())
            {
                char c = 0;
                cerr << LineBreak("|-> Es sind sowohl Daten im Cache als auch die Daten eines Datenfiles vorhanden. Welche sollen verwendet werden? (c/d)$(0 zum Abbrechen)", _option) << endl;
                cerr << "|" << endl;
                cerr << "|<- ";
                cin >> c;

                if (c == '0')
                {
                    cerr << "|-> ABBRUCH!" << endl;
                    cin.ignore(1);
                    return;
                }
                else if (c == 'c')
                    _data.setCacheStatus(true);
                cerr << LineBreak("|-> Es werden die geladenen Daten aus \"" + _data.getDataFileName("data") + "\" verwendet.", _option) << endl;
            }
            else
            {
                cerr << LineBreak("|-> Es werden die geladenen Daten des Files \"" + _data.getDataFileName("data") + "\" verwendet.", _option) << endl;
            }
        }
        else if (bUseCache)
            _data.setCacheStatus(true);


        if (matchParams(sCmd, "save", '=') || matchParams(sCmd, "export", '='))
        {
            int nPos = 0;
            if (matchParams(sCmd, "save", '='))
                nPos = matchParams(sCmd, "save", '=')+4;
            else
                nPos = matchParams(sCmd, "export", '=')+6;
            _out.setStatus(true);
            sSavePath = getArgAtPos(sCmd, nPos);
        }
        if (matchParams(sCmd, "save") || matchParams(sCmd, "export"))
            _out.setStatus(true);
        string sDatatable = "data";
        if (_data.matchCache(sCmd).length())
        {
            sDatatable = _data.matchCache(sCmd);
        }
        if (!_data.getLines(sDatatable) || !_data.getCols(sDatatable))
            throw NO_CACHED_DATA;

		int nLine = _data.getLines(sDatatable);
		int nCol = _data.getCols(sDatatable);
		// --> Allozieren einer Ausgabe-String-Matrix <--
		string** sOut = new string*[nLine + nStatsFields];
		string** sOverview = 0;
		for (int i = 0; i < nLine+nStatsFields; i++)
		{
			sOut[i] = new string[nCol];
		}

		// --> Berechnung fuer jede Spalte der Matrix! <--
		for (int j = 0; j < nCol; j++)
		{
			dAverage = _data.avg(sDatatable,0,nLine,j); // Wichtig: Nullsetzen aller wesentlichen Variablen!
			dError = _data.std(sDatatable,0,nLine,j);
			dPercentage = 0.0;
			dUnsure = 0.0;
			dSkew = 0.0;
			dKurt = 0.0;
			nCount = 0;
			sOut[0][j] = _data.getHeadLineElement(j, sDatatable);


			for (int i = 0; i < nLine; i++)
			{
				if (!_data.isValidEntry(i,j, sDatatable))
				{
					sOut[i+1][j] = "---";
					continue;
				}
				sOut[i+1][j] = toString(_data.getElement(i,j, sDatatable),_option); // Kopieren der Matrix in die Ausgabe
			}

			// --> Mittelwert: Durch den Anzahl aller Werte teilen <--
			//dAverage = dAverage / (double) (_data.getLines(true) - _data.getAppendedZeroes(j)); // WICHTIG: Angehaengte Nullzeilen ignorieren

			if(_option.getbDebug())
				cerr << "|-> DEBUG: dAverage = " << dAverage << endl;

			/*for (int i = 0; i < nLine; i++)
			{
				if(!_data.isValidEntry(i,j))
					continue;						// Angehaengte Nullzeilen ignorieren
				dError += (_data.getElement(i,j) - dAverage)*(_data.getElement(i,j) - dAverage); // Fehler: Erst mal alle Fehlerquadrate addieren
			}

			// --> Fehler: Jetzt durch die Anzahl aller Werte teilen und danach die Wurzel ziehen <--
			dError = sqrt(dError / (double) (_data.getLines(true) - 1 - _data.getAppendedZeroes(j)));*/ // WICHTIG: Angehaengte Nullzeilen wieder ignorieren
			// --> Unsicherheit: den Fehler nochmals durch die Wurzel aus der Anzahl aller Werte-1 teilen
			dUnsure = dError / sqrt(_data.num(sDatatable,0,nLine,j));

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
			dPercentage = (double)nCount/(double) (_data.num(sDatatable,0,nLine,j))*100.0;
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

			dSkew /= _data.num(sDatatable,0,nLine,j);
			dKurt -= 3;
			dKurt /= _data.num(sDatatable,0,nLine,j);

			sOut[nLine+1][j] = "<<SUMBAR>>"; // Schreiben der berechneten Werte in die letzten drei Zeilen der Ausgabe
			sOut[nLine+2][j] = "MW: " + toString(dAverage, nPrecision);
			sOut[nLine+3][j] = "+/-" + toString(dError, nPrecision);
			sOut[nLine+4][j] = "iVInt: " + toString(dPercentage, nPrecision) + " %";
			sOut[nLine+5][j] = "Uns: " + toString(dUnsure, nPrecision);
			sOut[nLine+6][j] = "Med: " + toString(_data.med(sDatatable,0,nLine,j), nPrecision);
			sOut[nLine+7][j] = "Q1: " + toString(_data.pct(sDatatable,0,nLine,j,-1,0.25), nPrecision);
			sOut[nLine+8][j] = "Q3: " + toString(_data.pct(sDatatable,0,nLine,j,-1,0.75), nPrecision);
			sOut[nLine+9][j] = "RMS: " + toString(_data.norm(sDatatable,0,nLine,j)/sqrt(_data.num(sDatatable,0,nLine,j)), nPrecision);
			sOut[nLine+10][j] = "Skew: " + toString(dSkew, nPrecision);
			sOut[nLine+11][j] = "Exz: " + toString(dKurt, nPrecision);
			sOut[nLine+12][j] = "min: " + toString(_data.min(sDatatable,0,nLine,j), nPrecision);
			sOut[nLine+13][j] = "max: " + toString(_data.max(sDatatable,0,nLine,j), nPrecision);
			sOut[nLine+14][j] = "num: " + toString(_data.num(sDatatable,0,nLine,j), nPrecision);
			sOut[nLine+15][j] = "cnt: " + toString(_data.cnt(sDatatable,0,nLine,j), nPrecision);
			boost::math::students_t dist(_data.num(sDatatable,0,nLine,j));
			sOut[nLine+16][j] = "s_t: " + toString(boost::math::quantile(boost::math::complement(dist, 0.025)), nPrecision);
		}
		//cerr << "|-> Die Statistiken von " << nCol << " Spalte(n) wurden erfolgreich berechnet." << endl;

		// --> Allgemeine Ausgabe-Info-Parameter setzen <--
		_out.setPluginName("Statistik (v " + PI_MED + ") unter Verwendung der Daten aus " + _data.getDataFileName(sDatatable));
		//_out.setCommentLine("Legende: MW = Mittelwert, +/- = Standardabweichung, iVInt = im Vertrauensintervall, Uns = Unsicherheit");
		_out.setPrefix("stats");
		// --> Wenn sCmd einen Eintrag enthaelt, dann soll die Ausgabe automatische in eine Datei geschrieben werden <--

        sOverview = new string*[nStatsFields-1];
        for (unsigned int i = 0; i < nStatsFields-1; i++)
            sOverview[i] = new string[nCol+1];
        sOverview[0][0] = " ";
        for (int j = 0; j < nCol; j++)
        {
            sOverview[0][j+1] = _data.getHeadLineElement(j,sDatatable);
            for (int n = 1; n < nStatsFields-1; n++)
            {
                sOverview[n][j+1] = sOut[nLine+n+1][j].substr(sOut[nLine+n+1][j].find(':')+1);
                if (!j)
                    sOverview[n][0] = sOut[nLine+n+1][j].substr(0,sOut[nLine+n+1][j].find(':')+1);
            }
        }

        if (sSavePath.length())
            _out.setFileName(sSavePath);
        else
            _out.generateFileName();
		// --> Formatieren und Schreiben der Ausgabe <--
        //cerr << "|" << endl;
        _out.setCompact(false);
		_out.setCommentLine("Legende: MW = Mittelwert, +/- = Standardabweichung, iVInt = im Vertrauensintervall, Uns = Unsicherheit, Med = Median, Q1 = unteres Quartil, Q3 = oberes Quartil, RMS = root mean square, Skew = Schiefe, Exz = Exzess, min = Minimum, max = Maximum, num = Zahl der Elemente, cnt = Zahl der Zeilen, s_t = Student-Faktor");
		if (_out.isFile())
            _out.format(sOut, nCol, nLine+nStatsFields, _option, true);
        _out.reset();
        _out.setCompact(false);
		_out.setCommentLine("Legende: MW = Mittelwert, +/- = Standardabweichung, iVInt = im Vertrauensintervall, Uns = Unsicherheit, Med = Median, Q1 = unteres Quartil, Q3 = oberes Quartil, RMS = root mean square, Skew = Schiefe, Exz = Exzess, min = Minimum, max = Maximum, num = Zahl der Elemente, cnt = Zahl der Zeilen, s_t = Student-Faktor");

        make_hline();
        cerr << "|-> NUMERE: STATISTIKEN" << endl;
        make_hline();
        _out.format(sOverview, nCol+1, nStatsFields-1, _option, true);
        _out.reset();
        make_hline();
        // --> Speicher wieder freigeben! <--

        for (unsigned int i = 0; i < nStatsFields-1; i++)
            delete[] sOverview[i];
        delete[] sOverview;
		for (int i = 0; i < nLine+nStatsFields; i++)
		{
			delete[] sOut[i];
		}
		delete[] sOut;
		sOut = 0;
	}
	else				// Oh! Das sind offensichtlich keine Daten vorhanden... Sollte man dem Benutzer wohl mitteilen
	{
		cerr << "|-> FEHLER: Es sind keine Daten verfuegbar!" << endl;
	}
	//cerr << "|-> Das Plugin wurde erfolgreich beendet." << endl;

	// --> Output-Instanz wieder zuruecksetzen <--
	_out.reset();
	_data.setCacheStatus(false);

	return;
}
