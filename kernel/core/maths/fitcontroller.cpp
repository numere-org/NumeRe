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

#include "fitcontroller.hpp"
#include "../../kernel.hpp"

mu::Parser* Fitcontroller::_fitParser = nullptr;
int Fitcontroller::nDimensions = 0;
mu::varmap_type Fitcontroller::mParams;
mu::Variable* Fitcontroller::xvar = nullptr;
mu::Variable* Fitcontroller::yvar = nullptr;
mu::Variable* Fitcontroller::zvar = nullptr;

using namespace std;


/////////////////////////////////////////////////
/// \brief Default constructor
/////////////////////////////////////////////////
Fitcontroller::Fitcontroller()
{
    nIterations = 0;
    dChiSqr = 0.0;
    sExpr = "";

    xvar = nullptr;
    yvar = nullptr;
    zvar = nullptr;
}


/////////////////////////////////////////////////
/// \brief Constructor taking a pointer to the
/// current parser.
///
/// \param _parser mu::Parser*
///
/////////////////////////////////////////////////
Fitcontroller::Fitcontroller(mu::Parser* _parser) : Fitcontroller()
{
    _fitParser = _parser;
    mu::varmap_type mVars = _parser->GetVar();
    xvar = mVars["x"];
    yvar = mVars["y"];
    zvar = mVars["z"];
}


/////////////////////////////////////////////////
/// \brief Destructor.
/////////////////////////////////////////////////
Fitcontroller::~Fitcontroller()
{
    _fitParser = nullptr;
    xvar = nullptr;
    yvar = nullptr;
    zvar = nullptr;
}


// Bei NaN Ergebnisse auf double MAX gesetzt. Kann ggf. Schwierigkeiten bei Fitfunktionen mit Minima nahe NaN machen...
/////////////////////////////////////////////////
/// \brief Fit function for the usual case
/// without any restrictions.
///
/// \param params const gsl_vector*
/// \param data void*
/// \param fvals gsl_vector*
/// \return int
///
/////////////////////////////////////////////////
int Fitcontroller::fitfunction(const gsl_vector* params, void* data, gsl_vector* fvals)
{
    FitData* _fData = static_cast<FitData*>(data);
    size_t i = 0;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = mu::Value(gsl_vector_get(params, i));
        i++;
    }

    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
    {
        for (size_t n = 0; n < _fData->vx.size(); n++)
        {
            if (isnan(_fData->vy[n]) || isinf(_fData->vy[n]) || isnan(_fData->vy_w[n]) || !_fData->vy_w[n])
            {
                gsl_vector_set(fvals, n , 0.0);
                continue;
            }

            *xvar = mu::Value(_fData->vx[n]);
            gsl_vector_set(fvals, n, (_fitParser->Eval().front().getNum().val.real() - _fData->vy[n])/_fData->vy_w[n]); // Residuen (y-y0)/sigma

        }

        removeNANVals(fvals, _fData->vx.size());
    }
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
    {
        for (size_t n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = mu::Value(_fData->vx[n]);

            for (size_t m = 0; m < _fData->vy.size(); m++)
            {
                if (isnan(_fData->vz[n][m]) || isinf(_fData->vz[n][m]) || isnan(_fData->vz_w[n][m]) || !_fData->vz_w[n][m])
                {
                    gsl_vector_set(fvals, n , 0.0);
                    continue;
                }

                *yvar = mu::Value(_fData->vy[m]);
                gsl_vector_set(fvals, m+n*(_fData->vy.size()), (_fitParser->Eval().front().getNum().val.real() - _fData->vz[n][m])/_fData->vz_w[n][m]);

            }
        }

        removeNANVals(fvals, _fData->vx.size()*_fData->vy.size());
    }

    return GSL_SUCCESS;
}


/////////////////////////////////////////////////
/// \brief Create the jacobian matrix without
/// using restrictions.
///
/// \param params const gsl_vector*
/// \param data void*
/// \param Jac gsl_matrix*
/// \return int
///
/////////////////////////////////////////////////
int Fitcontroller::fitjacobian(const gsl_vector* params, void* data, gsl_matrix* Jac)
{
    FitData* _fData = static_cast<FitData*>(data);
    size_t i = 0;
    const double dEps = _fData->dPrecision*1.0e-1;
    FitMatrix vFuncRes;
    FitMatrix vDiffRes;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = mu::Value(gsl_vector_get(params, i));
        i++;
    }

    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
    {
        vFuncRes = FitMatrix(_fData->vx.size(),vector<double>(1,0.0));
        vDiffRes = FitMatrix(_fData->vx.size(),vector<double>(1,0.0));

        for (size_t n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = mu::Value(_fData->vx[n]);
            vFuncRes[n][0] = _fitParser->Eval().front().getNum().val.real();
        }
    }
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
    {
        vFuncRes = FitMatrix(_fData->vz.size(),vector<double>(_fData->vz[0].size(),0.0));
        vDiffRes = FitMatrix(_fData->vz.size(),vector<double>(_fData->vz[0].size(),0.0));

        for (size_t n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = mu::Value(_fData->vx[n]);

            for (size_t m = 0; m < _fData->vy.size(); m++)
            {
                *yvar = mu::Value(_fData->vy[m]);
                vFuncRes[n][m] = _fitParser->Eval().front().getNum().val.real();

            }
        }
    }

    i = 0;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = mu::Value(gsl_vector_get(params, i) + dEps);

        if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
        {
            for (size_t n = 0; n < _fData->vx.size(); n++)
            {
                *xvar = mu::Value(_fData->vx[n]);
                vDiffRes[n][0] = _fitParser->Eval().front().getNum().val.real();
            }

            for (size_t n = 0; n < _fData->vx.size(); n++)
            {
                if (isnan(_fData->vy[n]) || isinf(_fData->vy[n]))
                {
                    gsl_matrix_set(Jac, n, i, 0.0);
                    continue;
                }

                gsl_matrix_set(Jac, n, i, (vDiffRes[n][0]-vFuncRes[n][0])/(dEps*_fData->vy_w[n]));
            }
        }
        else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
        {
            for (size_t n = 0; n < _fData->vx.size(); n++)
            {
                *xvar = mu::Value(_fData->vx[n]);

                for (size_t m = 0; m < _fData->vy.size(); m++)
                {
                    *yvar = mu::Value(_fData->vy[m]);
                    vDiffRes[n][m] = _fitParser->Eval().front().getNum().val.real();
                }
            }

            for (size_t n = 0; n < _fData->vx.size(); n++)
            {
                for (size_t m = 0; m < _fData->vy.size(); m++)
                {
                    if (isnan(_fData->vz[n][m]) || isinf(_fData->vz[n][m]))
                    {
                        gsl_matrix_set(Jac, m+n*(_fData->vy.size()), i, 0.0);
                        continue;
                    }

                    gsl_matrix_set(Jac, m+n*(_fData->vy.size()), i, (vDiffRes[n][m]-vFuncRes[n][m])/(dEps*_fData->vz_w[n][m]));
                }
            }
        }

        *(iter->second) = mu::Value(gsl_vector_get(params, i));
        i++;
    }

    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size())
        removeNANVals(Jac, _fData->vx.size(), mParams.size());
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size())
        removeNANVals(Jac, _fData->vx.size() * _fData->vy.size(), mParams.size());

    return GSL_SUCCESS;
}


/////////////////////////////////////////////////
/// \brief Combination of fit function and the
/// corresponding jacobian.
///
/// \param params const gsl_vector*
/// \param data void*
/// \param fvals gsl_vector*
/// \param Jac gsl_matrix*
/// \return int
///
/////////////////////////////////////////////////
int Fitcontroller::fitfuncjac(const gsl_vector* params, void* data, gsl_vector* fvals, gsl_matrix* Jac)
{
    fitfunction(params, data, fvals);
    fitjacobian(params, data, Jac);
    return GSL_SUCCESS;
}


/////////////////////////////////////////////////
/// \brief Fit function respecting additional
/// restrictions.
///
/// \param params const gsl_vector*
/// \param data void*
/// \param fvals gsl_vector*
/// \return int
///
/////////////////////////////////////////////////
int Fitcontroller::fitfunctionrestricted(const gsl_vector* params, void* data, gsl_vector* fvals)
{
    FitData* _fData = static_cast<FitData*>(data);
    size_t i = 0;
    int nVals = 0;
    mu::Array* v = nullptr;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = mu::Value(gsl_vector_get(params, i));
        i++;
    }

    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
    {
        for (size_t n = 0; n < _fData->vx.size(); n++)
        {
            if (isnan(_fData->vy[n]) || isinf(_fData->vy[n]) || isnan(_fData->vy_w[n]) || !_fData->vy_w[n])
            {
                gsl_vector_set(fvals, n , 0.0);
                continue;
            }

            *xvar = mu::Value(_fData->vx[n]);
            v = _fitParser->Eval(nVals);
            gsl_vector_set(fvals, n, (evalRestrictions(v, nVals) - _fData->vy[n])/_fData->vy_w[n]); // Residuen (y-y0)/sigma

        }

        removeNANVals(fvals, _fData->vx.size());
    }
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
    {
        for (size_t n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = mu::Value(_fData->vx[n]);

            for (size_t m = 0; m < _fData->vy.size(); m++)
            {
                if (isnan(_fData->vz[n][m]) || isinf(_fData->vz[n][m]) || isnan(_fData->vz_w[n][m]) || !_fData->vz_w[n][m])
                {
                    gsl_vector_set(fvals, n , 0.0);
                    continue;
                }

                *yvar = mu::Value(_fData->vy[m]);
                v = _fitParser->Eval(nVals);
                gsl_vector_set(fvals, m+n*(_fData->vy.size()), (evalRestrictions(v, nVals) - _fData->vz[n][m])/_fData->vz_w[n][m]);

            }
        }

        removeNANVals(fvals, _fData->vx.size()*_fData->vy.size());
    }

    return GSL_SUCCESS;
}


/////////////////////////////////////////////////
/// \brief Jacobian matrix respecting additional
/// restrictions.
///
/// \param params const gsl_vector*
/// \param data void*
/// \param Jac gsl_matrix*
/// \return int
///
/////////////////////////////////////////////////
int Fitcontroller::fitjacobianrestricted(const gsl_vector* params, void* data, gsl_matrix* Jac)
{
    FitData* _fData = static_cast<FitData*>(data);
    size_t i = 0;
    const double dEps = _fData->dPrecision*1.0e-1;
    FitMatrix vFuncRes;
    FitMatrix vDiffRes;
    mu::Array* v = nullptr;
    int nVals = 0;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = mu::Value(gsl_vector_get(params, i));
        i++;
    }

    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
    {
        vFuncRes = FitMatrix(_fData->vx.size(),vector<double>(1,0.0));
        vDiffRes = FitMatrix(_fData->vx.size(),vector<double>(1,0.0));

        for (size_t n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = mu::Value(_fData->vx[n]);
            v = _fitParser->Eval(nVals);
            vFuncRes[n][0] = evalRestrictions(v, nVals);
        }
    }
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
    {
        vFuncRes = FitMatrix(_fData->vz.size(),vector<double>(_fData->vz[0].size(),0.0));
        vDiffRes = FitMatrix(_fData->vz.size(),vector<double>(_fData->vz[0].size(),0.0));

        for (size_t n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = mu::Value(_fData->vx[n]);

            for (size_t m = 0; m < _fData->vy.size(); m++)
            {
                *yvar = mu::Value(_fData->vy[m]);
                v = _fitParser->Eval(nVals);
                vFuncRes[n][m] = evalRestrictions(v, nVals);

            }
        }
    }

    i = 0;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = mu::Value(gsl_vector_get(params, i) + dEps);

        if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
        {
            for (size_t n = 0; n < _fData->vx.size(); n++)
            {
                *xvar = mu::Value(_fData->vx[n]);
                v = _fitParser->Eval(nVals);
                vDiffRes[n][0] = evalRestrictions(v, nVals);
            }

            for (size_t n = 0; n < _fData->vx.size(); n++)
            {
                if (isnan(_fData->vy[n]) || isinf(_fData->vy[n]))
                {
                    gsl_matrix_set(Jac, n, i, 0.0);
                    continue;
                }

                gsl_matrix_set(Jac, n, i, (vDiffRes[n][0]-vFuncRes[n][0])/(dEps*_fData->vy_w[n]));
            }
        }
        else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
        {
            for (size_t n = 0; n < _fData->vx.size(); n++)
            {
                *xvar = mu::Value(_fData->vx[n]);

                for (size_t m = 0; m < _fData->vy.size(); m++)
                {
                    *yvar = mu::Value(_fData->vy[m]);
                    v = _fitParser->Eval(nVals);
                    vDiffRes[n][m] = evalRestrictions(v, nVals);
                }
            }

            for (size_t n = 0; n < _fData->vx.size(); n++)
            {
                for (size_t m = 0; m < _fData->vy.size(); m++)
                {
                    if (isnan(_fData->vz[n][m]) || isinf(_fData->vz[n][m]))
                    {
                        gsl_matrix_set(Jac, m+n*(_fData->vy.size()), i, 0.0);
                        continue;
                    }

                    gsl_matrix_set(Jac, m+n*(_fData->vy.size()), i, (vDiffRes[n][m]-vFuncRes[n][m])/(dEps*_fData->vz_w[n][m]));
                }
            }
        }

        *(iter->second) = mu::Value(gsl_vector_get(params, i));
        i++;
    }

    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size())
        removeNANVals(Jac, _fData->vx.size(), mParams.size());
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size())
        removeNANVals(Jac, _fData->vx.size() * _fData->vy.size(), mParams.size());

    return GSL_SUCCESS;
}


/////////////////////////////////////////////////
/// \brief Combination of fit function and the
/// corresponding jacobian respecting additional
/// restrictions.
///
/// \param params const gsl_vector*
/// \param data void*
/// \param fvals gsl_vector*
/// \param Jac gsl_matrix*
/// \return int
///
/////////////////////////////////////////////////
int Fitcontroller::fitfuncjacrestricted(const gsl_vector* params, void* data, gsl_vector* fvals, gsl_matrix* Jac)
{
    fitfunctionrestricted(params, data, fvals);
    fitjacobianrestricted(params, data, Jac);
    return GSL_SUCCESS;
}


/////////////////////////////////////////////////
/// \brief Evaluate additional restrictions.
///
/// \param v const mu::Array*
/// \param nVals int
/// \return double
///
/////////////////////////////////////////////////
double Fitcontroller::evalRestrictions(const mu::Array* v, int nVals)
{
    if (nVals == 1)
        return v[0].front().getNum().val.real();
    else
    {
        for (int i = 1; i < nVals; i++)
        {
            if (!mu::all(v[i]))
                return NAN;
        }

        return v[0].front().getNum().val.real();
    }

    return NAN;
}


/////////////////////////////////////////////////
/// \brief Replace NaNs with some very large
/// value to avoid converging towards NaNs.
///
/// \param fvals gsl_vector*
/// \param nSize size_t
/// \return void
///
/////////////////////////////////////////////////
void Fitcontroller::removeNANVals(gsl_vector* fvals, size_t nSize)
{
    double dMax = NAN;

    for (size_t i = 0; i < nSize; i++)
    {
        if (!isnan(gsl_vector_get(fvals, i)) && !isinf(gsl_vector_get(fvals, i)))
        {
            if (isnan(dMax))
                dMax = fabs(gsl_vector_get(fvals, i));
            else if (dMax < fabs(gsl_vector_get(fvals, i)))
                dMax = fabs(gsl_vector_get(fvals, i));
        }
    }

    if (isnan(dMax))
        dMax = 1.0e120;

    dMax *= 100.0;

    for (size_t i = 0; i < nSize; i++)
    {
        if (isnan(gsl_vector_get(fvals, i)) || isinf(gsl_vector_get(fvals, i)))
            gsl_vector_set(fvals, i, dMax);
    }
}


/////////////////////////////////////////////////
/// \brief Replace NaNs with some very large
/// value to avoid converging towards NaNs.
///
/// \param Jac gsl_matrix*
/// \param nLines size_t
/// \param nCols size_t
/// \return void
///
/////////////////////////////////////////////////
void Fitcontroller::removeNANVals(gsl_matrix* Jac, size_t nLines, size_t nCols)
{
    double dMax = NAN;

    for (size_t i = 0; i < nLines; i++)
    {
        for (size_t j = 0; j < nCols; j++)
        {
            if (!isnan(gsl_matrix_get(Jac, i, j)) && !isinf(gsl_matrix_get(Jac, i, j)))
            {
                if (isnan(dMax))
                    dMax = fabs(gsl_matrix_get(Jac, i, j));
                else if (dMax < fabs(gsl_matrix_get(Jac, i, j)))
                    dMax = fabs(gsl_matrix_get(Jac, i, j));
            }
        }
    }

    if (isnan(dMax))
        dMax = 1.0e120;

    dMax *= 100.0;

    for (size_t i = 0; i < nLines; i++)
    {
        for (size_t j = 0; j < nCols; j++)
        {
            if (isnan(gsl_matrix_get(Jac, i, j)) || isinf(gsl_matrix_get(Jac, i, j)))
                gsl_matrix_set(Jac, i, j, dMax);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This is the central fitting function
/// using the data passed from the interface
/// functions.
///
/// \param __sExpr const string&
/// \param __sRestrictions const string&
/// \param _fData FitData&
/// \param __dPrecision double
/// \param nMaxIterations int
/// \return bool
///
/////////////////////////////////////////////////
bool Fitcontroller::fitctrl(const string& __sExpr, const string& __sRestrictions, FitData& _fData, double __dPrecision, int nMaxIterations)
{
    dChiSqr = 0.0;
    nIterations = 0;
    sExpr = "";

    int nStatus = 0;
    int nRetry = 0;
    size_t nPoints = (_fData.vz.size()) ? _fData.vx.size()*_fData.vy.size() : _fData.vx.size();

    // Validation
    if (_fData.vz.size() && _fData.vx.size() && _fData.vy.size())
    {
        if (_fData.vz.size() != _fData.vx.size()
            || _fData.vz[0].size() != _fData.vy.size()
            || _fData.vz.size() != _fData.vz_w.size()
            || _fData.vz[0].size() != _fData.vz_w[0].size())
            return false;
    }
    else if (_fData.vx.size() && _fData.vy.size())
    {
        if (_fData.vx.size() != _fData.vy.size() || _fData.vy.size() != _fData.vy_w.size())
            return false;
    }
    else
        return false;

    // Validate the algorithm parameters
    if (!isnan(__dPrecision) && !isinf(__dPrecision))
        _fData.dPrecision = fabs(__dPrecision);
    else
        _fData.dPrecision = 1e-4;

    if (nMaxIterations <= 1)
        nMaxIterations = 500;

    if (__sRestrictions.length())
        _fitParser->SetExpr(__sExpr+","+__sRestrictions);
    else
        _fitParser->SetExpr(__sExpr);

    _fitParser->Eval();
    sExpr = __sExpr;

    // Adapt the fit weights
    if (_fData.vy_w.size())
    {
        for (size_t i = 0; i < _fData.vy_w.size(); i++)
            _fData.vy_w[i] += 1.0;
    }

    if (_fData.vz_w.size())
    {
        for (size_t i = 0; i < _fData.vz_w.size(); i++)
        {
            for (size_t j = 0; j < _fData.vz_w[i].size(); j++)
                _fData.vz_w[i][j] += 1.0;
        }
    }

    // gsl_vector seems to be better in the interaction
    // with GSL than std::vector
    gsl_vector* params = gsl_vector_alloc(mParams.size());
    size_t n = 0;

    // Copy the parameter values
    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        gsl_vector_set(params, n, iter->second->front().getNum().val.real());
        n++;
    }

    // Prepare the GSL fitting module
    const gsl_multifit_fdfsolver_type* solver_type = gsl_multifit_fdfsolver_lmsder;
    gsl_multifit_fdfsolver* solver = gsl_multifit_fdfsolver_alloc(solver_type, nPoints, mParams.size());
    gsl_multifit_function_fdf func;

    // Assign the correct functions to the
    // multifit function structure
    if (__sRestrictions.length())
    {
        func.f = fitfunctionrestricted;
        func.df = fitjacobianrestricted;
        func.fdf = fitfuncjacrestricted;
    }
    else
    {
        func.f = fitfunction;
        func.df = fitjacobian;
        func.fdf = fitfuncjac;
    }

    func.n = nPoints;
    func.p = mParams.size();
    func.params = &_fData;

    gsl_multifit_fdfsolver_set(solver, &func, params);

    // Main fit iterator
    do
    {
        // Iterate the fit solver
        if (gsl_multifit_fdfsolver_iterate(solver))
        {
            // Failure in this iteration
            if (!nIterations && !nRetry) //Algorithmus kommt mit den Startparametern nicht klar (und nur mit diesen)
            {
                for (size_t j = 0; j < mParams.size(); j++)
                {
                    if (!gsl_vector_get(params, j)) // 0 durch 1 ersetzen und nochmal probieren
                        gsl_vector_set(params, j, 1.0);
                }

                // Reset the solver
                gsl_multifit_fdfsolver_free(solver);
                solver = gsl_multifit_fdfsolver_alloc(solver_type, nPoints, mParams.size());
                gsl_multifit_fdfsolver_set(solver, &func, params);
                nRetry = 1;
                nStatus = GSL_CONTINUE;
                continue;
            }
            else if (!nIterations && nRetry == 1)
            {
                nRetry++;
                nStatus = GSL_CONTINUE;
                continue;
            }
            else
                break;
        }

        // Test, whether the required precision is already
        // in the desired range
        nStatus = gsl_multifit_test_delta(solver->dx, solver->x, _fData.dPrecision, _fData.dPrecision);
        nIterations++;

        // Cancel the algorithm if required by the user
        if (NumeReKernel::GetAsyncCancelState())
        {
            gsl_multifit_fdfsolver_free(solver);
            throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
        }
    }
    while (nStatus == GSL_CONTINUE && nIterations < nMaxIterations);

    // Get the covariance matrix

    gsl_matrix* mCovar = gsl_matrix_alloc(mParams.size(), mParams.size());
#ifdef NR_HAVE_GSL2
	gsl_matrix *J = gsl_matrix_alloc(solver->fdf->n, solver->fdf->p);
	gsl_multifit_fdfsolver_jac(solver, J);
	gsl_multifit_covar (J, 0.0, mCovar);
	gsl_matrix_free (J);
#else
	gsl_multifit_covar(solver->J, 0.0, mCovar);
#endif

    vCovarianceMatrix = FitMatrix(mParams.size(), vector<double>(mParams.size(), 0.0));

    for (size_t i = 0; i < mParams.size(); i++)
    {
        for (size_t j = 0; j < mParams.size(); j++)
            vCovarianceMatrix[i][j] = gsl_matrix_get(mCovar, i, j);
    }

    gsl_matrix_free(mCovar);

    n = 0;
    // Get the parameter from the solver
    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = mu::Value(gsl_vector_get(solver->x, n));
        n++;
    }

    // Get the Chi^2
    dChiSqr = gsl_blas_dnrm2(solver->f);
    dChiSqr *= dChiSqr;

    // Free the memory needed by the solver
    gsl_multifit_fdfsolver_free(solver);
    gsl_vector_free(params);

    // Examine the restrictions
    if (__sRestrictions.length())
    {
        int nVals = 0;
        mu::Array* v = _fitParser->Eval(nVals);

        if (isnan(evalRestrictions(v, nVals)))
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Calculate a weighted 1D-fit.
///
/// \param vx FitVector&
/// \param vy FitVector&
/// \param vy_w FitVector&
/// \param __sExpr const string&
/// \param __sRestrictions const string&
/// \param mParamsMap mu::varmap_type&
/// \param __dPrecision double
/// \param nMaxIterations int
/// \return bool
///
/////////////////////////////////////////////////
bool Fitcontroller::fit(FitVector& vx, FitVector& vy, FitVector& vy_w, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision, int nMaxIterations)
{
    mParams = mParamsMap;
    FitData _fData;
    _fData.vx = vx;
    _fData.vy = vy;
    _fData.vy_w = vy_w;

    return fitctrl(__sExpr, __sRestrictions, _fData, __dPrecision, nMaxIterations);
}


/////////////////////////////////////////////////
/// \brief Calculate a weighted 2D-fit.
///
/// \param vx FitVector&
/// \param vy FitVector&
/// \param vz FitMatrix&
/// \param vz_w FitMatrix&
/// \param __sExpr const string&
/// \param __sRestrictions const string&
/// \param mParamsMap mu::varmap_type&
/// \param __dPrecision double
/// \param nMaxIterations int
/// \return bool
///
/////////////////////////////////////////////////
bool Fitcontroller::fit(FitVector& vx, FitVector& vy, FitMatrix& vz, FitMatrix& vz_w, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision, int nMaxIterations)
{
    mParams = mParamsMap;
    FitData _fData;
    _fData.vx = vx;
    _fData.vy = vy;
    _fData.vz = vz;
    _fData.vz_w = vz_w;

    return fitctrl(__sExpr, __sRestrictions, _fData, __dPrecision, nMaxIterations);
}


/////////////////////////////////////////////////
/// \brief Returns the fit function, where each
/// parameter is replaced with its value
/// converted to a string.
///
/// \return string
///
/////////////////////////////////////////////////
string Fitcontroller::getFitFunction()
{
    if (!sExpr.length())
        return "";

    sExpr += " ";
    MutableStringView expr(sExpr);

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        for (size_t i = 0; i < expr.length(); i++)
        {
            if (expr.match(iter->first, i))
            {
                if (expr.is_delimited_sequence(i, iter->first.length()))
                {
                    expr.replace(i, (iter->first).length(), iter->second->print(5));
                    i += iter->second->print(5).length();
                }
            }
        }
    }

    for (size_t i = 0; i < sExpr.length()-1; i++)
    {
        if (sExpr.find_first_not_of(' ', i+1) == string::npos)
            break;

        if (sExpr[i] == '+' && sExpr[sExpr.find_first_not_of(' ', i+1)] == '-')
            sExpr.erase(i,1);

        if (sExpr[i] == '-' && sExpr[sExpr.find_first_not_of(' ', i+1)] == '+')
            sExpr.erase(sExpr.find_first_not_of(' ', i+1), 1);

        if (sExpr[i] == '-' && sExpr[sExpr.find_first_not_of(' ', i+1)] == '-')
        {
            sExpr[i] = '+';
            sExpr.erase(sExpr.find_first_not_of(' ', i+1), 1);
        }
    }

    StripSpaces(sExpr);
    return sExpr;
}


/////////////////////////////////////////////////
/// \brief Returns the covariance matrix of the
/// performed fit.
///
/// \return FitMatrix
///
/////////////////////////////////////////////////
FitMatrix Fitcontroller::getCovarianceMatrix() const
{
    return vCovarianceMatrix;
}


