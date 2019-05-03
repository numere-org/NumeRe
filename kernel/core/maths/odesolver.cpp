/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2015  Erik Haenel et al.

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

#include "odesolver.hpp"
#include "../../kernel.hpp"

extern Integration_Vars parser_iVars;

Parser* Odesolver::_odeParser = 0;
int Odesolver::nDimensions = 0;
mu::varmap_type Odesolver::mVars;

Odesolver::Odesolver()
{
    _odeParser = 0;
    _odeData = 0;
    _odeFunctions = 0;
    _odeSettings = 0;

    odeStepType = 0;
    odeStep = 0;
    odeControl = 0;
    odeEvolve = 0;

    nDimensions = 0;
}

Odesolver::Odesolver(Parser* _parser, Datafile* _data, Define* _functions, Settings* _option)
{
    Odesolver();
    _odeParser = _parser;
    _odeData = _data;
    _odeFunctions = _functions;
    _odeSettings = _option;

    //cerr << odeEvolve << endl;
    //cerr << odeControl << endl;
    //cerr << odeStep << endl;

}

Odesolver::~Odesolver()
{
    _odeParser = 0;
    _odeData = 0;
    _odeFunctions = 0;
    _odeSettings = 0;

    //cerr << odeEvolve << endl;
    //cerr << odeControl << endl;
    //cerr << odeStep << endl;

    if (odeEvolve)
        gsl_odeiv_evolve_free(odeEvolve);
    if (odeControl)
        gsl_odeiv_control_free(odeControl);
    if (odeStep)
        gsl_odeiv_step_free(odeStep);
}


int Odesolver::odeFunction(double x, const double y[], double dydx[], void* params)
{
    int nResults = 0;
    value_type* v = 0;

    // Variablen zuweisen
    parser_iVars.vValue[0][0] = x;

    for (int i = 0; i < nDimensions; i++)
    {
        *(mVars.find("y"+toString(i+1))->second) = y[i];
    }

    v = _odeParser->Eval(nResults);
    for (int i = 0; i < nResults; i++)
    {
        dydx[i] = v[i];
    }
    return GSL_SUCCESS;
}

bool Odesolver::solve(const string& sCmd)
{
    if (!_odeParser || !_odeData || !_odeFunctions || !_odeSettings)
        return false;

    // Warum auch immer diese Pointer an dieser Stelle bereits eine Adresse hatten...???
    odeEvolve = 0;
    odeControl = 0;
    odeStep = 0;

    const gsl_odeiv_step_type* odeStepType_ly = 0;
    gsl_odeiv_step* odeStep_ly = 0;
    gsl_odeiv_control* odeControl_ly = 0;
    gsl_odeiv_evolve* odeEvolve_ly = 0;
    gsl_odeiv_system odeSystem_ly;


    //cerr << 1 << endl;
    double t0 = 0.0;
    double t1 = 0.0;
    double t2 = 0.0;
    double dt = 0.0;
    double h = 0.0;
    double h2 = 0.0;
    double dRelTolerance = 0.0;
    double dAbsTolerance = 0.0;
    int nSamples = 100;
    int nLyapuSamples = 100;
    value_type* v = 0;
    vector<double> vInterval;
    vector<double> vStartValues;

    string sFunc = "";
    string sParams = "";
    string sTarget = "ode()";
    string sVarDecl = "y1";
    Indices _idx;
    bool bAllowCacheClearance = false;
    bool bCalcLyapunov = false;

    time_t tTimeControl = time(0);

    double* y = 0;
    double* y2 = 0;
    double lyapu[2] = {0.0, 0.0};
    double dist[2] = {1.0e-6,0.0};
    double t = 0.0;

    if (containsStrings(sCmd) || _odeData->containsStringVars(sCmd))
    {
        //sErrorToken = "odesolve";
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "odesolve");
    }

    if (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos)
    {
        if (sCmd.find("-set") != string::npos)
        {
            sFunc = sCmd.substr(0,sCmd.find("-set"));
            sParams = sCmd.substr(sCmd.find("-set"));
        }
        else
        {
            sFunc = sCmd.substr(0,sCmd.find("--"));
            sParams = sCmd.substr(sCmd.find("--"));
        }
        sFunc.erase(0,findCommand(sFunc).nPos+8); //odesolve EXPR -set...
        StripSpaces(sFunc);
    }
    else
        throw SyntaxError(SyntaxError::NO_OPTIONS_FOR_ODE, sCmd, SyntaxError::invalid_position);

    //cerr << 2 << endl;
    if (!sFunc.length())
        throw SyntaxError(SyntaxError::NO_EXPRESSION_FOR_ODE, sCmd, SyntaxError::invalid_position);
    if (!_odeFunctions->call(sFunc))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sFunc, sFunc);
    if (sFunc.find("data(") != string::npos || _odeData->containsTablesOrClusters(sFunc))
        getDataElements(sFunc, *_odeParser, *_odeData, *_odeSettings);

    if (matchParams(sParams, "target", '='))
    {
        sTarget = getArgAtPos(sParams, matchParams(sParams, "target", '=')+6);
        sParams.erase(sParams.find(sTarget, matchParams(sParams, "target", '=')+6), sTarget.length());
        sParams.erase(matchParams(sParams, "target", '=')-1, 7);
        if (sTarget.find('(') != string::npos)
        {
            sTarget += "()";
            bAllowCacheClearance = true;
        }
    }
    else
        bAllowCacheClearance = true;

    //cerr << 3 << endl;
    if (!_odeData->isCacheElement(sTarget))
        _odeData->addCache(sTarget, *_odeSettings);

    _idx = parser_getIndices(sTarget, *_odeParser, *_odeData, *_odeSettings);

    if (!isValidIndexSet(_idx))
        return false;

    sTarget.erase(sTarget.find('('));

    if (!_odeFunctions->call(sParams))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sParams, sParams);
    if (sParams.find("data(") != string::npos || _odeData->containsTablesOrClusters(sParams))
        getDataElements(sParams, *_odeParser, *_odeData, *_odeSettings);

    //cerr << 4 << endl;
    if (matchParams(sParams, "method", '='))
    {
        if (getArgAtPos(sParams, matchParams(sParams, "method", '=')) == "rkf45")
        {
            odeStepType = gsl_odeiv_step_rkf45;
        }
        else if (getArgAtPos(sParams, matchParams(sParams, "method", '=')) == "rk2")
        {
            odeStepType = gsl_odeiv_step_rk2;
        }
        else if (getArgAtPos(sParams, matchParams(sParams, "method", '=')) == "rkck")
        {
            odeStepType = gsl_odeiv_step_rkck;
        }
        else if (getArgAtPos(sParams, matchParams(sParams, "method", '=')) == "rk8pd")
        {
            odeStepType = gsl_odeiv_step_rk8pd;
        }
        else
        {
            odeStepType = gsl_odeiv_step_rk4;
        }
    }
    else
    {
        odeStepType = gsl_odeiv_step_rk4;
    }
    if (matchParams(sParams, "lyapunov"))
        bCalcLyapunov = true;
    if (matchParams(sParams, "tol", '='))
    {
        int nRes = 0;
        string sToleranceParams = getArgAtPos(sParams, matchParams(sParams, "tol", '=')+3);
        if (sToleranceParams.front() == '[' && sToleranceParams.back() == ']')
        {
            sToleranceParams.pop_back();
            sToleranceParams.erase(0,1);
        }
        _odeParser->SetExpr(sToleranceParams);
        v = _odeParser->Eval(nRes);
        if (nRes > 1)
        {
            dRelTolerance = v[0];
            dAbsTolerance = v[1];
        }
        else
        {
            dRelTolerance = v[0];
            dAbsTolerance = v[0];
        }
    }
    else
    {
        dRelTolerance = 1e-6;
        dAbsTolerance = 1e-6;
    }
    if (matchParams(sParams, "fx0", '='))
    {
        int nRes = 0;
        string sStartValues = getArgAtPos(sParams, matchParams(sParams, "fx0", '=')+3);
        if (sStartValues.front() == '[' && sStartValues.back() == ']')
        {
            sStartValues.pop_back();
            sStartValues.erase(0,1);
        }
        _odeParser->SetExpr(sStartValues);
        v = _odeParser->Eval(nRes);
        for (int i = 0; i < nRes; i++)
        {
            vStartValues.push_back(v[i]);
        }
    }
    if (matchParams(sParams, "samples", '='))
    {
        _odeParser->SetExpr(getArgAtPos(sParams, matchParams(sParams, "samples", '=')+7));
        nSamples = (int)_odeParser->Eval();
        if (nSamples <= 0)
            nSamples = 100;
    }

    if (bCalcLyapunov)
    {
        if (nSamples <= 200)
            nLyapuSamples = nSamples / 10;
        else if (nSamples <= 1000)
            nLyapuSamples = nSamples / 20;
        else
            nLyapuSamples = nSamples / 100;
    }
    //cerr << 5 << endl;
    vInterval = parser_IntervalReader(sParams, *_odeParser, *_odeData, *_odeFunctions, *_odeSettings, false);
    //cerr << 6 << endl;
    if (!vInterval.size() || isnan(vInterval[0]) || isinf(vInterval[0]) || isnan(vInterval[1]) || isinf(vInterval[1]))
        throw SyntaxError(SyntaxError::NO_INTERVAL_FOR_ODE, sCmd, SyntaxError::invalid_position);
    dt = (vInterval[1]-vInterval[0])/(double)nSamples;
    t0 = vInterval[0];
    t1 = vInterval[0];
    h = dRelTolerance;
    h2 = dRelTolerance;
    //cerr << 7 << endl;

    parser_iVars.vValue[0][0] = t0;

    // Dimension des ODE-Systems bestimmen: odesolve dy1 = y2*x, dy2 = sin(y1)
    _odeParser->SetExpr(sFunc);
    v = _odeParser->Eval(nDimensions);

    //cerr << sFunc << endl;
    //cerr << nDimensions << endl;
    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _idx.row.front() + nSamples);
    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + nDimensions + (long long int)bCalcLyapunov*2);

    if (bAllowCacheClearance)
    {
        _odeData->deleteBulk(sTarget, 0, _odeData->getLines(sTarget, false) - 1, 0, nDimensions+(long long int)bCalcLyapunov*2);
    }

    y = new double[nDimensions];
    if (bCalcLyapunov)
        y2 = new double[nDimensions];

    // Startwerte festlegen
    for (int i = 0; i < nDimensions; i++)
    {
        if (i < (int)vStartValues.size())
            y[i] = vStartValues[i];
        else
            y[i] = 0.0;
        if (bCalcLyapunov)
        {
            if (i < (int)vStartValues.size())
                y2[i] = vStartValues[i];
            else
                y2[i] = 0.0;
            if (!i)
                y2[i] += dist[0];
        }
    }

    for (int i = 1; i < nDimensions; i++)
        sVarDecl += ", y" + toString(i+1);
    _odeParser->SetExpr(sVarDecl);
    _odeParser->Eval();
    mVars = _odeParser->GetVar();

    _odeParser->SetExpr(sFunc);

    // Routinen initialisieren
    odeStep = gsl_odeiv_step_alloc(odeStepType, nDimensions);
    odeControl = gsl_odeiv_control_y_new(dAbsTolerance, dRelTolerance);
    odeEvolve = gsl_odeiv_evolve_alloc(nDimensions);

    gsl_odeiv_system odeSystem = {odeFunction, jacobian, (unsigned)nDimensions, 0};

    if (bCalcLyapunov)
    {
        odeStepType_ly = odeStepType;
        odeStep_ly = gsl_odeiv_step_alloc(odeStepType_ly, nDimensions);
        odeControl_ly = gsl_odeiv_control_y_new(dAbsTolerance, dRelTolerance);
        odeEvolve_ly = gsl_odeiv_evolve_alloc(nDimensions);

        odeSystem_ly.function = odeFunction;
        odeSystem_ly.jacobian = jacobian;
        odeSystem_ly.dimension = (unsigned)nDimensions;
        odeSystem_ly.params = 0;
    }

    NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("ODESOLVER_SOLVE_SYSTEM") + " ..."));
    if (bAllowCacheClearance || !_idx.row.front())
        _odeData->setHeadLineElement(_idx.col.front(), sTarget, "x");

    _odeData->writeToCache(_idx.row.front(), _idx.col.front(), sTarget, t);

    for (int j = 0; j < nDimensions; j++)
    {
        if (_idx.col[j+1] == VectorIndex::INVALID)
            break;

        if (bAllowCacheClearance || !_idx.row.front())
            _odeData->setHeadLineElement(_idx.col[1+j], sTarget, "y"+toString(j+1));

        _odeData->writeToCache(_idx.row.front(), _idx.col[j+1], sTarget, y[j]);
    }

    if (bCalcLyapunov && (bAllowCacheClearance || !_idx.row.front()) && _idx.col[nDimensions+2] != VectorIndex::INVALID)
    {
        _odeData->setHeadLineElement(_idx.col[1+nDimensions], sTarget, "x_lypnv");
        _odeData->setHeadLineElement(_idx.col[2+nDimensions], sTarget, "lyapunov");
    }

    // integrieren
    for (int i = 0; i < nSamples; i++)
    {
        if (time(0) - tTimeControl > 1)
        {
            NumeReKernel::printPreFmt(toSystemCodePage("\r|-> " + _lang.get("ODESOLVER_SOLVE_SYSTEM") + " ... " + toString((int)(i*100.0/(double)nSamples)) + " %"));
        }
        if (NumeReKernel::GetAsyncCancelState())//GetAsyncKeyState(VK_ESCAPE))
        {
            NumeReKernel::printPreFmt(" " + toSystemCodePage(_lang.get("COMMON_CANCEL")) + ".\n");
            gsl_odeiv_evolve_free(odeEvolve);
            gsl_odeiv_control_free(odeControl);
            gsl_odeiv_step_free(odeStep);

            if (bCalcLyapunov)
            {
                gsl_odeiv_evolve_free(odeEvolve_ly);
                gsl_odeiv_control_free(odeControl_ly);
                gsl_odeiv_step_free(odeStep_ly);
            }

            odeEvolve = 0;
            odeControl = 0;
            odeStep = 0;

            if (y)
                delete[] y;
            if (y2)
                delete[] y2;

            throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
        }

        if (_idx.row.size() <= i+1)
            break;

        t1 += dt;

        while (t < t1)
        {
            if (GSL_SUCCESS != gsl_odeiv_evolve_apply(odeEvolve, odeControl, odeStep, &odeSystem, &t, t1, &h, y))
                break;
        }

        h2 = dRelTolerance;

        while (t2 < t1 && bCalcLyapunov)
        {
            if (GSL_SUCCESS != gsl_odeiv_evolve_apply(odeEvolve_ly, odeControl_ly, odeStep_ly, &odeSystem_ly, &t2, t1, &h2, y2))
                break;
        }
        if (bCalcLyapunov)
        {
            dist[1] = 0.0;
            for (int n = 0; n < nDimensions; n++)
            {
                dist[1] += (y[n]-y2[n])*(y[n]-y2[n]);
            }
            dist[1] = sqrt(dist[1]);
            lyapu[1] = log(dist[1]/dist[0])/dt;
            lyapu[0] = (i*lyapu[0] + lyapu[1])/(double)(i+1);

            if (!((i+1) % nLyapuSamples) && _idx.col[nDimensions + 2] != VectorIndex::INVALID)
            {
                _odeData->writeToCache((i+1)/nLyapuSamples-1, _idx.col[nDimensions+1], sTarget, t1);
                _odeData->writeToCache((i+1)/nLyapuSamples-1, _idx.col[nDimensions+2], sTarget, lyapu[0]);
            }

            for (int n = 0; n < nDimensions; n++)
            {
                y2[n] = y[n] + (y2[n]-y[n])*dist[0]/dist[1];
            }

            gsl_odeiv_evolve_reset(odeEvolve_ly);
            gsl_odeiv_step_reset(odeStep_ly);
        }

        _odeData->writeToCache(_idx.row[i+1], _idx.col[0], sTarget, t);

        for (int j = 0; j < nDimensions; j++)
        {
            if (_idx.col[j+1] != VectorIndex::INVALID)
                break;

            _odeData->writeToCache(_idx.row[i+1], _idx.col[j+1], sTarget, y[j]);
        }
    }

    gsl_odeiv_evolve_free(odeEvolve);
    gsl_odeiv_control_free(odeControl);
    gsl_odeiv_step_free(odeStep);

    if (bCalcLyapunov)
    {
        gsl_odeiv_evolve_free(odeEvolve_ly);
        gsl_odeiv_control_free(odeControl_ly);
        gsl_odeiv_step_free(odeStep_ly);
    }

    odeEvolve = 0;
    odeControl = 0;
    odeStep = 0;

    if (y)
        delete[] y;

    if (y2)
        delete[] y2;

    NumeReKernel::printPreFmt(" " + _lang.get("COMMON_SUCCESS") + ".\n");
    return true;
}


