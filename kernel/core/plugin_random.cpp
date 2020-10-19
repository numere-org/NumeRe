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

#include <random>
#include <ctime>

#include "plugins.hpp"
#include "../kernel.hpp"
#include "maths/parser_functions.hpp"
#include "structures.hpp"

/////////////////////////////////////////////////
/// \brief This static function unifies the
/// parameter detection of the random command
/// implementation.
///
/// \param sCmd const std::string&
/// \param sLongVersion const std::string&
/// \param sShortVersion const std::string&
/// \param _parser Parser&
/// \param defaultVal double
/// \return double
///
/////////////////////////////////////////////////
static double getParameterValue(const std::string& sCmd, const std::string& sLongVersion, const std::string& sShortVersion, Parser& _parser, double defaultVal)
{
    if (findParameter(sCmd, sLongVersion, '=') || findParameter(sCmd, sShortVersion, '='))
    {
        int nPos = 0;

        if (findParameter(sCmd, sLongVersion, '='))
            nPos = findParameter(sCmd, sLongVersion, '=')+sLongVersion.length();
        else
            nPos = findParameter(sCmd, sShortVersion, '=')+sShortVersion.length();

        _parser.SetExpr(getArgAtPos(sCmd, nPos));
        return _parser.Eval();
    }

    return defaultVal;
}


/////////////////////////////////////////////////
/// \brief This enumeration defines all available
/// random distributions.
/////////////////////////////////////////////////
enum RandomDistribution
{
    NORMAL_DISTRIBUTION,
    POISSON_DISTRIBUTION,
    GAMMA_DISTRIBUTION,
    UNIFORM_DISTRIBUTION,
    BINOMIAL_DISTRIBUTION,
    STUDENT_DISTRIBUTION
};


/////////////////////////////////////////////////
/// \brief This function is the implementation of
/// the random command.
///
/// \param sCmd string&
/// \return void
///
/////////////////////////////////////////////////
void plugin_random(string& sCmd)
{
    // Get all necessary references
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();
    Indices _idx;

    RandomDistribution nDistribution = NORMAL_DISTRIBUTION;
    static double dSeedBase = 1.0;
    string sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_GAUSS");
    string sTarget = evaluateTargetOptionInCommand(sCmd, "table", _idx, _parser, _data, _option);
    double dRandomNumber = 0.0;
    default_random_engine randomGenerator(dSeedBase * time(0)); // Zufallszahlengenerator initialisieren

    // Get all parameter values (or use default ones)
    long long int nDataPoints = intCast(getParameterValue(sCmd, "lines", "l", _parser, 0.0));
    long long int nDataRows = intCast(getParameterValue(sCmd, "cols", "c", _parser, 0.0));
    double dDistributionMean = getParameterValue(sCmd, "mean", "m", _parser, 0.0);
    double dDistributionWidth = fabs(getParameterValue(sCmd, "width", "w", _parser, 1.0));
    double dShape = fabs(getParameterValue(sCmd, "shape", "sh", _parser, 1.0));
    double dScale = fabs(getParameterValue(sCmd, "scale", "sc", _parser, 1.0));
    double dProbability = fabs(getParameterValue(sCmd, "prob", "p", _parser, 0.5));
    unsigned int nUpperBound = abs(intCast(getParameterValue(sCmd, "ubound", "ub", _parser, 1.0)));
    unsigned int nFreedoms = abs(intCast(getParameterValue(sCmd, "freedoms", "f", _parser, 1.0)));

    // Find the type of random distribution
    if (findParameter(sCmd, "distrib", '=') || findParameter(sCmd, "d", '='))
    {
        int nPos = 0;

        if (findParameter(sCmd, "distrib", '='))
            nPos = findParameter(sCmd, "distrib", '=')+7;
        else
            nPos = findParameter(sCmd, "d", '=')+1;

        sDistrib = getArgAtPos(sCmd, nPos);

        if (sDistrib == "gauss" || sDistrib == "normal")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_GAUSS");
            nDistribution = NORMAL_DISTRIBUTION;
        }
        else if (sDistrib == "poisson")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_POISSON");
            nDistribution = POISSON_DISTRIBUTION;
        }
        else if (sDistrib == "gamma")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_GAMMA");
            nDistribution = GAMMA_DISTRIBUTION;
        }
        else if (sDistrib == "uniform")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_UNIFORM");
            nDistribution = UNIFORM_DISTRIBUTION;
        }
        else if (sDistrib == "binomial")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_BINOMIAL");
            nDistribution = BINOMIAL_DISTRIBUTION;
        }
        else if (sDistrib == "student")
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_STUDENT");
            nDistribution = STUDENT_DISTRIBUTION;
        }
        else
        {
            sDistrib = _lang.get("RANDOM_DISTRIB_TYPE_GAUSS");
            nDistribution = NORMAL_DISTRIBUTION;
        }
    }

    if (!nDataRows)
        throw SyntaxError(SyntaxError::NO_COLS, sCmd, SyntaxError::invalid_position);

    if (!nDataPoints)
        throw SyntaxError(SyntaxError::NO_ROWS, sCmd, SyntaxError::invalid_position);

    // Create random distributions
    normal_distribution<double> normalDistribution(dDistributionMean, dDistributionWidth);
    poisson_distribution<int> poissonDistribution(dDistributionMean);
    gamma_distribution<double> gammaDistribution(dShape, dScale);
    uniform_real_distribution<double> uniformDistribution(dDistributionMean-0.5*dDistributionWidth, dDistributionMean+0.5*dDistributionWidth);
    binomial_distribution<int> binomialDistribution(nUpperBound, dProbability);
    student_t_distribution<double> studentTDistribution(nFreedoms);

    // Fill the table with the newly created random numbers
    for (long long int i = 0; i < nDataPoints; i++)
    {
        for (long long int j = 0; j < nDataRows; j++)
        {
            switch (nDistribution)
            {
                case NORMAL_DISTRIBUTION:
                    dRandomNumber = normalDistribution(randomGenerator);
                    break;
                case POISSON_DISTRIBUTION:
                    dRandomNumber = poissonDistribution(randomGenerator);
                    break;
                case GAMMA_DISTRIBUTION:
                    dRandomNumber = gammaDistribution(randomGenerator);
                    break;
                case UNIFORM_DISTRIBUTION:
                    dRandomNumber = uniformDistribution(randomGenerator);
                    break;
                case BINOMIAL_DISTRIBUTION:
                    dRandomNumber = binomialDistribution(randomGenerator);
                    break;
                case STUDENT_DISTRIBUTION:
                    dRandomNumber = studentTDistribution(randomGenerator);
                    break;
            }

            if (i < _idx.row.size() && j < _idx.col.size())
                _data.writeToTable(_idx.row[i], _idx.col[j], sTarget, dRandomNumber);

            if ((!i && !j) || dSeedBase == 0.0)
            {
                if (dSeedBase == dRandomNumber)
                    dSeedBase = 0.0;
                else
                    dSeedBase = dRandomNumber;
            }
        }
    }

    NumeReKernel::print(_lang.get("RANDOM_SUCCESS", toString(nDataRows*nDataPoints), sDistrib));
}


