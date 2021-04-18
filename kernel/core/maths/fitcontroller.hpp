/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2016  Erik Haenel et al.

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





// CLASS Fitcontroller

#ifndef FITCONTROLLER_HPP
#define FITCONTROLLER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_vector.h>

#include "../ParserLib/muParser.h"
#include "../utils/tools.hpp"
#include "../ui/error.hpp"

using namespace std;
using namespace mu;

/////////////////////////////////////////////////
/// \brief Typedef for readability.
/////////////////////////////////////////////////
typedef std::vector<double> FitVector;

/////////////////////////////////////////////////
/// \brief Typedef for readability.
/////////////////////////////////////////////////
typedef std::vector<std::vector<double>> FitMatrix;


/////////////////////////////////////////////////
/// \brief Defines the datapoints, which shall
/// be fitted.
/////////////////////////////////////////////////
struct FitData
{
    FitVector vx;
    FitVector vy;
    FitVector vy_w;
    FitMatrix vz;
    FitMatrix vz_w;
    double dPrecision;
};


/////////////////////////////////////////////////
/// \brief This class contains the internal fit
/// logic and the interface to the GSL fitting
/// module.
/////////////////////////////////////////////////
class Fitcontroller
{
    private:
        static int fitfunction(const gsl_vector* params, void* data, gsl_vector* fvals);
        static int fitjacobian(const gsl_vector* params, void* data, gsl_matrix* Jac);
        static int fitfuncjac(const gsl_vector* params, void* data, gsl_vector* fvals, gsl_matrix* Jac);
        static int fitfunctionrestricted(const gsl_vector* params, void* data, gsl_vector* fvals);
        static int fitjacobianrestricted(const gsl_vector* params, void* data, gsl_matrix* Jac);
        static int fitfuncjacrestricted(const gsl_vector* params, void* data, gsl_vector* fvals, gsl_matrix* Jac);
        static double evalRestrictions(const value_type* v, int nVals);
        int nIterations;
        double dChiSqr;
        string sExpr;
        FitMatrix vCovarianceMatrix;
        static double* xvar;
        static double* yvar;
        static double* zvar;

        bool fitctrl(const string& __sExpr, const string& __sRestrictions, FitData& _fData, double __dPrecision, int nMaxIterations);
        static void removeNANVals(gsl_vector* fvals, unsigned int nSize);
        static void removeNANVals(gsl_matrix* Jac, unsigned int nLines, unsigned int nCols);

    public:
        static Parser* _fitParser;
        static int nDimensions;
        static mu::varmap_type mParams;

        Fitcontroller();
        Fitcontroller(Parser* _parser);
        ~Fitcontroller();

        /////////////////////////////////////////////////
        /// \brief Register the parser.
        ///
        /// \param _parser Parser*
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void setParser(Parser* _parser)
            {
                _fitParser = _parser;
                mu::varmap_type mVars = _parser->GetVar();
                xvar = mVars["x"];
                yvar = mVars["y"];
                zvar = mVars["z"];
                return;
            }

        /////////////////////////////////////////////////
        /// \brief Calculate an unweighted 1D-fit.
        ///
        /// \param vx FitVector&
        /// \param vy FitVector&
        /// \param __sExpr const string&
        /// \param __sRestrictions const string&
        /// \param mParamsMap mu::varmap_type&
        /// \param __dPrecision double
        /// \param nMaxIterations int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool fit(FitVector& vx, FitVector& vy, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision = 1e-4, int nMaxIterations = 500)
            {
                FitVector vy_w(vy.size(), 0.0);
                return fit(vx,vy, vy_w, __sExpr, __sRestrictions, mParamsMap, __dPrecision, nMaxIterations);
            }

        bool fit(FitVector& vx, FitVector& vy, FitVector& vy_w, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision = 1e-4, int nMaxIterations = 500);

        /////////////////////////////////////////////////
        /// \brief Calculate an unweighted 2D-fit.
        ///
        /// \param vx FitVector&
        /// \param vy FitVector&
        /// \param vz FitMatrix&
        /// \param __sExpr const string&
        /// \param __sRestrictions const string&
        /// \param mParamsMap mu::varmap_type&
        /// \param __dPrecision double
        /// \param nMaxIterations int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool fit(FitVector& vx, FitVector& vy, FitMatrix& vz, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision = 1e-4, int nMaxIterations = 500)
            {
                FitMatrix vz_w(vz.size(), vector<double>(vz[0].size(), 0.0));
                return fit(vx, vy, vz, vz_w, __sExpr, __sRestrictions, mParamsMap, __dPrecision, nMaxIterations);
            }

        bool fit(FitVector& vx, FitVector& vy, FitMatrix& vz, FitMatrix& vz_w, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision = 1e-4, int nMaxIterations = 500);

        /////////////////////////////////////////////////
        /// \brief Return the weighted sum of the
        /// residuals.
        ///
        /// \return double
        ///
        /////////////////////////////////////////////////
        inline double getFitChi() const
            {return dChiSqr;}

        /////////////////////////////////////////////////
        /// \brief Return the number of needed
        /// iterations.
        ///
        /// \return int
        ///
        /////////////////////////////////////////////////
        inline int getIterations() const
            {return nIterations;}

        string getFitFunction();
        FitMatrix getCovarianceMatrix() const;
};


#endif // FITCONTROLLER_HPP

