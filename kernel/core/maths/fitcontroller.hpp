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

struct FitData
{
    vector<double> vx;
    vector<double> vy;
    vector<double> vy_w;
    vector<vector<double> > vz;
    vector<vector<double> > vz_w;
    double dPrecision;
};

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
        vector<vector<double> > vCovarianceMatrix;
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

        inline void setParser(Parser* _parser)
            {
                _fitParser = _parser;
                mu::varmap_type mVars = _parser->GetVar();
                xvar = mVars["x"];
                yvar = mVars["y"];
                zvar = mVars["z"];
                return;
            }
        inline bool fit(vector<double>& vx, vector<double>& vy, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision = 1e-4, int nMaxIterations = 500)
            {
                vector<double> vy_w(vy.size(), 0.0);
                return fit(vx,vy, vy_w, __sExpr, __sRestrictions, mParamsMap, __dPrecision, nMaxIterations);
            }
        bool fit(vector<double>& vx, vector<double>& vy, vector<double>& vy_w, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision = 1e-4, int nMaxIterations = 500);
        inline bool fit(vector<double>& vx, vector<double>& vy, vector<vector<double> >& vz, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision = 1e-4, int nMaxIterations = 500)
            {
                vector<vector<double> > vz_w(vz.size(), vector<double>(vz[0].size(), 0.0));
                return fit(vx, vy, vz, vz_w, __sExpr, __sRestrictions, mParamsMap, __dPrecision, nMaxIterations);
            }
        bool fit(vector<double>& vx, vector<double>& vy, vector<vector<double> >& vz, vector<vector<double> >& vz_w, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision = 1e-4, int nMaxIterations = 500);

        inline double getFitChi() const
            {return dChiSqr;}
        inline int getIterations() const
            {return nIterations;}

        string getFitFunction();
        vector<vector<double> > getCovarianceMatrix() const;
};


#endif // FITCONTROLLER_HPP

