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


/*
 * Realisierung des Random-Plugins
 */

#include "plugins.hpp"
#include "../kernel.hpp"

const string PI_RAND = "0.2.3";

void plugin_random(string& sCmd, Datafile& _data, Output& _out, Settings& _option, bool bAllowOverride)
{
	long long int nDataPoints = 0;			// Variable zur Festlegung, wie viele Datenpunkte erzeugt werden sollen
	long long int nDataRows = 0;
	long long int nFilledCols = 0;
	if (_data.isValidCache() && !bAllowOverride)       // Frage die Zahl der (irgendwie) vollgeschriebenen Spalten ab
        nFilledCols = _data.getCacheCols("cache", false);
	double dDistributionWidth = 1.0; // Die Breite der Verteilung
	double dDistributionMean = 0.0;	// Der Mittelwert der Verteilung
	unsigned int nFreedoms = 1;
	double dShape = 1.0;
	double dScale = 1.0;
	double dProbability = 0.5;
	unsigned int nUpperBound = 1;
	unsigned int nDistribution = 0;
	static double dSeedBase = 1.0;

    string sDistrib = "normalverteilte";
	double dRandomNumber = 0.0;
	string sInput = "";
	time_t now = dSeedBase * time(0);			// Aktuelle Zeit fuer einen Seed initialisieren

	default_random_engine randomGenerator (now); // Zufallszahlengenerator initialisieren

    // --> Zunaechst extrahieren wir die ggf. uebergebenen Parameter <--
    if (matchParams(sCmd, "lines", '=') || matchParams(sCmd, "l", '='))
    {
        if (matchParams(sCmd, "lines", '='))
            nDataPoints = matchParams(sCmd, "lines", '=')+5;
        else
            nDataPoints = matchParams(sCmd, "l", '=')+1;
        nDataPoints = (int)StrToDb(getArgAtPos(sCmd, nDataPoints));
    }
    if (matchParams(sCmd, "cols", '=') || matchParams(sCmd, "c", '='))
    {
        if (matchParams(sCmd, "cols", '='))
            nDataRows = matchParams(sCmd, "cols", '=')+4;
        else
            nDataRows = matchParams(sCmd, "c", '=')+1;
        nDataRows = (int)StrToDb(getArgAtPos(sCmd, nDataRows));
    }
    if (matchParams(sCmd, "mean", '=') || matchParams(sCmd, "m", '='))
    {
        int nPos = 0;
        if (matchParams(sCmd, "mean", '='))
            nPos = matchParams(sCmd, "mean", '=')+4;
        else
            nPos = matchParams(sCmd, "m", '=')+1;
        dDistributionMean = StrToDb(getArgAtPos(sCmd, nPos));
    }
    if (matchParams(sCmd, "width", '=') || matchParams(sCmd, "w", '='))
    {
        int nPos = 0;
        if (matchParams(sCmd, "width", '='))
            nPos = matchParams(sCmd, "width", '=')+5;
        else
            nPos = matchParams(sCmd, "w", '=')+1;
        dDistributionWidth = StrToDb(getArgAtPos(sCmd, nPos));
    }
    if (matchParams(sCmd, "shape", '=') || matchParams(sCmd, "sh", '='))
    {
        int nPos = 0;
        if (matchParams(sCmd, "shape", '='))
            nPos = matchParams(sCmd, "shape", '=')+5;
        else
            nPos = matchParams(sCmd, "sh", '=')+2;
        dShape = fabs(StrToDb(getArgAtPos(sCmd, nPos)));
    }
    if (matchParams(sCmd, "scale", '=') || matchParams(sCmd, "sc", '='))
    {
        int nPos = 0;
        if (matchParams(sCmd, "scale", '='))
            nPos = matchParams(sCmd, "scale", '=')+5;
        else
            nPos = matchParams(sCmd, "sc", '=')+2;
        dScale = fabs(StrToDb(getArgAtPos(sCmd, nPos)));
    }
    if (matchParams(sCmd, "ubound", '=') || matchParams(sCmd, "ub", '='))
    {
        int nPos = 0;
        if (matchParams(sCmd, "ubound", '='))
            nPos = matchParams(sCmd, "ubound", '=')+6;
        else
            nPos = matchParams(sCmd, "ub", '=')+2;
        nUpperBound = abs(StrToInt(getArgAtPos(sCmd, nPos)));
    }
    if (matchParams(sCmd, "prob", '=') || matchParams(sCmd, "p", '='))
    {
        int nPos = 0;
        if (matchParams(sCmd, "prob", '='))
            nPos = matchParams(sCmd, "prob", '=')+4;
        else
            nPos = matchParams(sCmd, "p", '=')+1;
        dProbability = fabs(StrToDb(getArgAtPos(sCmd, nPos)));
        if (dProbability > 1.0)
            dProbability = 0.5;
    }
    if (matchParams(sCmd, "freedoms", '=') || matchParams(sCmd, "f", '='))
    {
        int nPos = 0;
        if (matchParams(sCmd, "freedoms", '='))
            nPos = matchParams(sCmd, "freedoms", '=')+8;
        else
            nPos = matchParams(sCmd, "f", '=')+1;
        nFreedoms = abs(StrToInt(getArgAtPos(sCmd, nPos)));
    }
    if (matchParams(sCmd, "distrib", '=') || matchParams(sCmd, "d", '='))
    {
        int nPos = 0;

        if (matchParams(sCmd, "distrib", '='))
            nPos = matchParams(sCmd, "distrib", '=')+7;
        else
            nPos = matchParams(sCmd, "d", '=')+1;
        sDistrib = getArgAtPos(sCmd, nPos);
        if (sDistrib == "gauss" || sDistrib == "normal")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_GAUSS");
            nDistribution = 0;
        }
        else if (sDistrib == "poisson")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_POISSON");
            nDistribution = 1;
        }
        else if (sDistrib == "gamma")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_GAMMA");
            nDistribution = 2;
        }
        else if (sDistrib == "uniform")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_UNIFORM");
            nDistribution = 3;
        }
        else if (sDistrib == "binomial")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_BINOMIAL");
            nDistribution = 4;
        }
        else if (sDistrib == "student")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_STUDENT");
            nDistribution = 5;
        }
        else
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_GAUSS");
            nDistribution = 0;
        }
    }


	//cerr << "|-> ZUFALLSZAHLENGENERATOR (v " << PI_RAND << ")" << endl;
	//cerr << "|   " << std::setfill((char)196) << std::setw(32) << (char)196 << endl;
	//cerr << LineBreak("|-> Generiert einen oder mehrere Datensaetze an Zufallszahlen und speichert diese(n) in den Cache.", _option) << endl;


    if (!nDataRows)
    {
        throw SyntaxError(SyntaxError::NO_COLS, sCmd, SyntaxError::invalid_position);
    }

    if (!nDataPoints)
    {
        throw SyntaxError(SyntaxError::NO_ROWS, sCmd, SyntaxError::invalid_position);
    }


	// --> Zufallsverteilungen erzeugen <--
	normal_distribution<double> normalDistribution(dDistributionMean, dDistributionWidth);  // 0
	poisson_distribution<int> poissonDistribution(dDistributionMean);                       // 1
	gamma_distribution<double> gammaDistribution(dShape, dScale);                           // 2
	uniform_real_distribution<double> uniformDistribution(dDistributionMean-0.5*dDistributionWidth, dDistributionMean+0.5*dDistributionWidth);  // 3
	binomial_distribution<int> binomialDistribution(nUpperBound, dProbability);             // 4
	student_t_distribution<double> studentTDistribution(nFreedoms);                         // 5
    if (nDataPoints * nDataRows > 1e6)
        NumeReKernel::printPreFmt(toSystemCodePage("|-> "+_lang.get("RANDOM_RESERVING_MEM")+" ... "));
    if (!_data.setCacheSize(nDataPoints,nDataRows+nFilledCols,-1))
        return;
    if (nDataPoints * nDataRows > 1e6)
        NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");

    if (_option.getbDebug())
        cerr << "|-> DEBUG: Cache angepasst!" << endl;

	for (long long int i = 0; i < nDataPoints; i++)
	{
		for (long long int j = 0; j < nDataRows; j++)
		{
            if (nDistribution == 0)
                dRandomNumber = normalDistribution(randomGenerator);
            else if (nDistribution == 1)
                dRandomNumber = poissonDistribution(randomGenerator);
            else if (nDistribution == 2)
                dRandomNumber = gammaDistribution(randomGenerator);
            else if (nDistribution == 3)
                dRandomNumber = uniformDistribution(randomGenerator);
            else if (nDistribution == 4)
                dRandomNumber = binomialDistribution(randomGenerator);
            else if (nDistribution == 5)
                dRandomNumber = studentTDistribution(randomGenerator);
			_data.writeToCache(i, j+nFilledCols, 0, dRandomNumber);

			if ((!i && !j) || dSeedBase == 0.0)
			{
                if (dSeedBase == dRandomNumber)
                    dSeedBase = 0.0;
                else
                    dSeedBase = dRandomNumber;
            }
		}
	}

	NumeReKernel::print(LineBreak(_lang.get("RANDOM_SUCCESS", toString(nDataRows*nDataPoints), sDistrib), _option));
	//cerr << LineBreak("|-> Es wurde(n) " + toString(nDataRows*nDataPoints) + " " + sDistrib + " Zufallszahlen erfolgreich in den Cache geschrieben.", _option) << endl;
	//cerr << "|-> Das Plugin wurde erfolgreich beendet." << endl;
	return;
}
