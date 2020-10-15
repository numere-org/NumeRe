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

Parser* Fitcontroller::_fitParser = 0;
int Fitcontroller::nDimensions = 0;
mu::varmap_type Fitcontroller::mParams;
double* Fitcontroller::xvar = 0;
double* Fitcontroller::yvar = 0;
double* Fitcontroller::zvar = 0;

Fitcontroller::Fitcontroller()
{
    nIterations = 0;
    dChiSqr = 0.0;
    sExpr = "";

    xvar = 0;
    yvar = 0;
    zvar = 0;
}

Fitcontroller::Fitcontroller(Parser* _parser) : Fitcontroller()
{
    _fitParser = _parser;
    mu::varmap_type mVars = _parser->GetVar();
    xvar = mVars["x"];
    yvar = mVars["y"];
    zvar = mVars["z"];
}

Fitcontroller::~Fitcontroller()
{
    _fitParser = 0;
    xvar = 0;
    yvar = 0;
    zvar = 0;
}

// Bei NaN Ergebnisse auf double MAX gesetzt. Kann ggf. Schwierigkeiten bei Fitfunktionen mit Minima nahe NaN machen...
int Fitcontroller::fitfunction(const gsl_vector* params, void* data, gsl_vector* fvals)
{
    FitData* _fData = static_cast<FitData*>(data);
    unsigned int i = 0;
    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = gsl_vector_get(params, i);
        i++;
    }
    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
    {
        for (unsigned int n = 0; n < _fData->vx.size(); n++)
        {
            if (isnan(_fData->vy[n]) || isinf(_fData->vy[n]) || isnan(_fData->vy_w[n]) || !_fData->vy_w[n])
            {
                gsl_vector_set(fvals, n , 0.0);
                continue;
            }
            *xvar = _fData->vx[n];
            gsl_vector_set(fvals, n, (_fitParser->Eval() - _fData->vy[n])/_fData->vy_w[n]); // Residuen (y-y0)/sigma

        }
        removeNANVals(fvals, _fData->vx.size());
    }
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
    {
        for (unsigned int n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = _fData->vx[n];
            for (unsigned int m = 0; m < _fData->vy.size(); m++)
            {
                if (isnan(_fData->vz[n][m]) || isinf(_fData->vz[n][m]) || isnan(_fData->vz_w[n][m]) || !_fData->vz_w[n][m])
                {
                    gsl_vector_set(fvals, n , 0.0);
                    continue;
                }
                *yvar = _fData->vy[m];
                gsl_vector_set(fvals, m+n*(_fData->vy.size()), (_fitParser->Eval() - _fData->vz[n][m])/_fData->vz_w[n][m]);

            }
        }
        removeNANVals(fvals, _fData->vx.size()*_fData->vy.size());
    }
    return GSL_SUCCESS;
}

int Fitcontroller::fitjacobian(const gsl_vector* params, void* data, gsl_matrix* Jac)
{
    FitData* _fData = static_cast<FitData*>(data);
    unsigned int i = 0;
    const double dEps = _fData->dPrecision*1.0e-1;
    vector<vector<double> > vFuncRes;
    vector<vector<double> > vDiffRes;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = gsl_vector_get(params, i);
        i++;
    }
    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
    {
        vFuncRes = vector<vector<double> >(_fData->vx.size(),vector<double>(1,0.0));
        vDiffRes = vector<vector<double> >(_fData->vx.size(),vector<double>(1,0.0));
        for (unsigned int n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = _fData->vx[n];
            vFuncRes[n][0] = _fitParser->Eval();
        }
    }
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
    {
        vFuncRes = vector<vector<double> >(_fData->vz.size(),vector<double>(_fData->vz[0].size(),0.0));
        vDiffRes = vector<vector<double> >(_fData->vz.size(),vector<double>(_fData->vz[0].size(),0.0));

        for (unsigned int n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = _fData->vx[n];
            for (unsigned int m = 0; m < _fData->vy.size(); m++)
            {
                *yvar = _fData->vy[m];
                vFuncRes[n][m] = _fitParser->Eval();

            }
        }
    }
    i = 0;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = gsl_vector_get(params, i) + dEps;
        if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
        {
            for (unsigned int n = 0; n < _fData->vx.size(); n++)
            {
                *xvar = _fData->vx[n];
                vDiffRes[n][0] = _fitParser->Eval();
            }
            for (unsigned int n = 0; n < _fData->vx.size(); n++)
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
            for (unsigned int n = 0; n < _fData->vx.size(); n++)
            {
                *xvar = _fData->vx[n];
                for (unsigned int m = 0; m < _fData->vy.size(); m++)
                {
                    *yvar = _fData->vy[m];
                    vDiffRes[n][m] = _fitParser->Eval();
                }
            }
            for (unsigned int n = 0; n < _fData->vx.size(); n++)
            {
                for (unsigned int m = 0; m < _fData->vy.size(); m++)
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
        *(iter->second) = gsl_vector_get(params, i);
        i++;
    }
    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size())
        removeNANVals(Jac, _fData->vx.size(), mParams.size());
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size())
        removeNANVals(Jac, _fData->vx.size() * _fData->vy.size(), mParams.size());
    return GSL_SUCCESS;
}

int Fitcontroller::fitfuncjac(const gsl_vector* params, void* data, gsl_vector* fvals, gsl_matrix* Jac)
{
    fitfunction(params, data, fvals);
    fitjacobian(params, data, Jac);
    return GSL_SUCCESS;
}

int Fitcontroller::fitfunctionrestricted(const gsl_vector* params, void* data, gsl_vector* fvals)
{
    FitData* _fData = static_cast<FitData*>(data);
    unsigned int i = 0;
    int nVals = 0;
    value_type* v = 0;
    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = gsl_vector_get(params, i);
        i++;
    }
    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
    {
        for (unsigned int n = 0; n < _fData->vx.size(); n++)
        {
            if (isnan(_fData->vy[n]) || isinf(_fData->vy[n]) || isnan(_fData->vy_w[n]) || !_fData->vy_w[n])
            {
                gsl_vector_set(fvals, n , 0.0);
                continue;
            }
            *xvar = _fData->vx[n];
            v = _fitParser->Eval(nVals);
            gsl_vector_set(fvals, n, (evalRestrictions(v, nVals) - _fData->vy[n])/_fData->vy_w[n]); // Residuen (y-y0)/sigma

        }
        removeNANVals(fvals, _fData->vx.size());
    }
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
    {
        for (unsigned int n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = _fData->vx[n];
            for (unsigned int m = 0; m < _fData->vy.size(); m++)
            {
                if (isnan(_fData->vz[n][m]) || isinf(_fData->vz[n][m]) || isnan(_fData->vz_w[n][m]) || !_fData->vz_w[n][m])
                {
                    gsl_vector_set(fvals, n , 0.0);
                    continue;
                }
                *yvar = _fData->vy[m];
                v = _fitParser->Eval(nVals);
                gsl_vector_set(fvals, m+n*(_fData->vy.size()), (evalRestrictions(v, nVals) - _fData->vz[n][m])/_fData->vz_w[n][m]);

            }
        }
        removeNANVals(fvals, _fData->vx.size()*_fData->vy.size());
    }
    return GSL_SUCCESS;
}

int Fitcontroller::fitjacobianrestricted(const gsl_vector* params, void* data, gsl_matrix* Jac)
{
    FitData* _fData = static_cast<FitData*>(data);
    unsigned int i = 0;
    const double dEps = _fData->dPrecision*1.0e-1;
    vector<vector<double> > vFuncRes;
    vector<vector<double> > vDiffRes;
    value_type* v = 0;
    int nVals = 0;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = gsl_vector_get(params, i);
        i++;
    }
    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
    {
        vFuncRes = vector<vector<double> >(_fData->vx.size(),vector<double>(1,0.0));
        vDiffRes = vector<vector<double> >(_fData->vx.size(),vector<double>(1,0.0));
        for (unsigned int n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = _fData->vx[n];
            v = _fitParser->Eval(nVals);
            vFuncRes[n][0] = evalRestrictions(v, nVals);
        }
    }
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size()) // xyz-Fit
    {
        vFuncRes = vector<vector<double> >(_fData->vz.size(),vector<double>(_fData->vz[0].size(),0.0));
        vDiffRes = vector<vector<double> >(_fData->vz.size(),vector<double>(_fData->vz[0].size(),0.0));

        for (unsigned int n = 0; n < _fData->vx.size(); n++)
        {
            *xvar = _fData->vx[n];
            for (unsigned int m = 0; m < _fData->vy.size(); m++)
            {
                *yvar = _fData->vy[m];
                v = _fitParser->Eval(nVals);
                vFuncRes[n][m] = evalRestrictions(v, nVals);

            }
        }
    }
    i = 0;

    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = gsl_vector_get(params, i) + dEps;
        if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size()) // xy-Fit
        {
            for (unsigned int n = 0; n < _fData->vx.size(); n++)
            {
                *xvar = _fData->vx[n];
                v = _fitParser->Eval(nVals);
                vDiffRes[n][0] = evalRestrictions(v, nVals);
            }
            for (unsigned int n = 0; n < _fData->vx.size(); n++)
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
            for (unsigned int n = 0; n < _fData->vx.size(); n++)
            {
                *xvar = _fData->vx[n];
                for (unsigned int m = 0; m < _fData->vy.size(); m++)
                {
                    *yvar = _fData->vy[m];
                    v = _fitParser->Eval(nVals);
                    vDiffRes[n][m] = evalRestrictions(v, nVals);
                }
            }
            for (unsigned int n = 0; n < _fData->vx.size(); n++)
            {
                for (unsigned int m = 0; m < _fData->vy.size(); m++)
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
        *(iter->second) = gsl_vector_get(params, i);
        i++;
    }
    if (_fData->vx.size() && _fData->vy.size() && !_fData->vz.size())
        removeNANVals(Jac, _fData->vx.size(), mParams.size());
    else if (_fData->vx.size() && _fData->vy.size() && _fData->vz.size())
        removeNANVals(Jac, _fData->vx.size() * _fData->vy.size(), mParams.size());
    return GSL_SUCCESS;
}

int Fitcontroller::fitfuncjacrestricted(const gsl_vector* params, void* data, gsl_vector* fvals, gsl_matrix* Jac)
{
    fitfunctionrestricted(params, data, fvals);
    fitjacobianrestricted(params, data, Jac);
    return GSL_SUCCESS;
}

double Fitcontroller::evalRestrictions(const value_type* v, int nVals)
{
    if (nVals == 1)
        return v[0];
    else
    {
        for (int i = 1; i < nVals; i++)
        {
            if (!v[i])
                return NAN;
        }
        return v[0];
    }
    return NAN;
}

void Fitcontroller::removeNANVals(gsl_vector* fvals, unsigned int nSize)
{
    double dMax = NAN;
    for (unsigned int i = 0; i < nSize; i++)
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
    for (unsigned int i = 0; i < nSize; i++)
    {
        if (isnan(gsl_vector_get(fvals, i)) || isinf(gsl_vector_get(fvals, i)))
            gsl_vector_set(fvals, i, dMax);
    }
    return;
}

void Fitcontroller::removeNANVals(gsl_matrix* Jac, unsigned int nLines, unsigned int nCols)
{
    double dMax = NAN;
    for (unsigned int i = 0; i < nLines; i++)
    {
        for (unsigned int j = 0; j < nCols; j++)
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
    for (unsigned int i = 0; i < nLines; i++)
    {
        for (unsigned int j = 0; j < nCols; j++)
        {
            if (isnan(gsl_matrix_get(Jac, i, j)) || isinf(gsl_matrix_get(Jac, i, j)))
                gsl_matrix_set(Jac, i, j, dMax);
        }
    }
    return;
}

bool Fitcontroller::fitctrl(const string& __sExpr, const string& __sRestrictions, FitData& _fData, double __dPrecision, int nMaxIterations)
{
    dChiSqr = 0.0;
    nIterations = 0;
    sExpr = "";
    double params[mParams.size()];
    int nStatus = 0;
    int nRetry = 0;
    unsigned int nPoints = (_fData.vz.size()) ? _fData.vx.size()*_fData.vy.size() : _fData.vx.size();

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

	size_t n = 0;
    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        params[n] = *(iter->second);
        n++;
    }

    gsl_vector_view vparams = gsl_vector_view_array(params, mParams.size());
    const gsl_multifit_fdfsolver_type* type = gsl_multifit_fdfsolver_lmsder;
    gsl_multifit_fdfsolver* solver = gsl_multifit_fdfsolver_alloc(type, nPoints, mParams.size());
    gsl_multifit_function_fdf func;
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

    gsl_multifit_fdfsolver_set(solver, &func, &vparams.vector);
    do
    {
        if (gsl_multifit_fdfsolver_iterate(solver))
        {
            if (!nIterations && !nRetry) //Algorithmus kommt mit den Startparametern nicht klar (und nur mit diesen)
            {
                for (size_t j = 0; j < mParams.size(); j++)
                {
                    if (!gsl_vector_get(&vparams.vector, j)) // 0 durch 1 ersetzen und nochmal probieren
                        gsl_vector_set(&vparams.vector, j, 1.0);
                }
                gsl_multifit_fdfsolver_free(solver);
                solver = gsl_multifit_fdfsolver_alloc(type, nPoints, mParams.size());
                gsl_multifit_fdfsolver_set(solver, &func, &vparams.vector);
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
        nStatus = gsl_multifit_test_delta(solver->dx, solver->x, _fData.dPrecision, _fData.dPrecision);
        nIterations++;
        if (NumeReKernel::GetAsyncCancelState())//GetAsyncKeyState(VK_ESCAPE))
        {
            gsl_multifit_fdfsolver_free(solver);
            throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
        }
    }
    while (nStatus == GSL_CONTINUE && nIterations < nMaxIterations);

    gsl_matrix* mCovar = gsl_matrix_alloc(mParams.size(), mParams.size());
    gsl_multifit_covar(solver->J, 0.0, mCovar);
    vCovarianceMatrix = vector<vector<double> >(mParams.size(), vector<double>(mParams.size(), 0.0));
    for (size_t i = 0; i < mParams.size(); i++)
    {
        for (size_t j = 0; j < mParams.size(); j++)
            vCovarianceMatrix[i][j] = gsl_matrix_get(mCovar, i, j);
    }
    gsl_matrix_free(mCovar);

    n = 0;
    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        *(iter->second) = gsl_vector_get(solver->x, n);
        n++;
    }
    dChiSqr = gsl_blas_dnrm2(solver->f);
    dChiSqr*=dChiSqr;
    gsl_multifit_fdfsolver_free(solver);

    if (__sRestrictions.length())
    {
        int nVals = 0;
        value_type* v = _fitParser->Eval(nVals);
        if (isnan(evalRestrictions(v, nVals)))
            return false;
    }
    return true;
}

bool Fitcontroller::fit(vector<double>& vx, vector<double>& vy, vector<double>& vy_w, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision, int nMaxIterations)
{
    mParams = mParamsMap;
    FitData _fData;
    _fData.vx = vx;
    _fData.vy = vy;
    _fData.vy_w = vy_w;
    return fitctrl(__sExpr, __sRestrictions, _fData, __dPrecision, nMaxIterations);
}

bool Fitcontroller::fit(vector<double>& vx, vector<double>& vy, vector<vector<double> >& vz, vector<vector<double> >& vz_w, const string& __sExpr, const string& __sRestrictions, mu::varmap_type& mParamsMap, double __dPrecision, int nMaxIterations)
{
    mParams = mParamsMap;
    FitData _fData;
    _fData.vx = vx;
    _fData.vy = vy;
    _fData.vz = vz;
    _fData.vz_w = vz_w;
    return fitctrl(__sExpr, __sRestrictions, _fData, __dPrecision, nMaxIterations);
}

string Fitcontroller::getFitFunction()
{
    if (!sExpr.length())
        return "";
    sExpr += " ";
    for (auto iter = mParams.begin(); iter != mParams.end(); ++iter)
    {
        for (unsigned int i = 0; i < sExpr.length(); i++)
        {
            if (sExpr.substr(i, (iter->first).length()) == iter->first)
            {
                if ((!i && checkDelimiter(" "+sExpr.substr(i, (iter->first).length()+1)))
                    || (i && checkDelimiter(sExpr.substr(i-1, (iter->first).length()+2))))
                {
                    sExpr.replace(i, (iter->first).length(), toString(*(iter->second), 5));
                    i += toString(*(iter->second), 5).length();
                }
            }
        }
    }
    for (unsigned int i = 0; i < sExpr.length()-1; i++)
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

vector<vector<double> > Fitcontroller::getCovarianceMatrix() const
{
    return vCovarianceMatrix;
}


