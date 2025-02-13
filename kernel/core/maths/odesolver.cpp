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
#include "../utils/tools.hpp"
#include "parser_functions.hpp"
#include "../ui/error.hpp"
#include <vector>

extern DefaultVariables _defVars;

typedef int (*GslFunc)(double, const double [], double [], void*);
typedef int (*GslJacobian) (double, const double [], double*, double [], void*);

/////////////////////////////////////////////////
/// \brief A structure for calculation
/// parameters.
/////////////////////////////////////////////////
struct OdeParams
{
    int dims = 0;
    mu::varmap_type vars;
    double eps_abs = 1e-6;
    double eps_rel = 1e-6;
    std::string func;
    std::string method;
};



/////////////////////////////////////////////////
/// \brief This struct wraps the GSL part and
/// provides the solving functionalities.
/////////////////////////////////////////////////
struct OdeSystem
{
    const gsl_odeiv_step_type* steptype;
    gsl_odeiv_step* step;
    gsl_odeiv_control* control;
    gsl_odeiv_evolve* evolve;
    gsl_odeiv_system system;

    /////////////////////////////////////////////////
    /// \brief Create an OdeSystem instance.
    /////////////////////////////////////////////////
    OdeSystem()
    {
        steptype = nullptr;
        step = nullptr;
        control = nullptr;
        evolve = nullptr;
    }

    /////////////////////////////////////////////////
    /// \brief Initialize the OdeSystem.
    ///
    /// \param func GslFunc
    /// \param jac GslJacobian
    /// \param par OdeParams&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void init(GslFunc func, GslJacobian jac, OdeParams& par)
    {
        if (par.method == "rkf45")
            steptype = gsl_odeiv_step_rkf45;
        else if (par.method == "rk2")
            steptype = gsl_odeiv_step_rk2;
        else if (par.method == "rkck")
            steptype = gsl_odeiv_step_rkck;
        else if (par.method == "rk8pd")
            steptype = gsl_odeiv_step_rk8pd;
        else
            steptype = gsl_odeiv_step_rk4;

        step = gsl_odeiv_step_alloc(steptype, par.dims);
        control = gsl_odeiv_control_y_new(par.eps_abs, par.eps_rel);
        evolve = gsl_odeiv_evolve_alloc(par.dims);

        system.function = func;
        system.jacobian = jac;
        system.dimension = par.dims;
        system.params = static_cast<void*>(&par);
    }

    /////////////////////////////////////////////////
    /// \brief Iterate the OdeSystem.
    ///
    /// \param t double*
    /// \param t1 double
    /// \param h double*
    /// \param y std::vector<double>&
    /// \return int
    ///
    /////////////////////////////////////////////////
    int iterate(double* t, double t1, double* h, std::vector<double>& y)
    {
        return gsl_odeiv_evolve_apply(evolve, control, step, &system, t, t1, h, &y[0]);
    }

    /////////////////////////////////////////////////
    /// \brief Reset the OdeSystem.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void reset()
    {
        gsl_odeiv_evolve_reset(evolve);
        gsl_odeiv_step_reset(step);
    }

    /////////////////////////////////////////////////
    /// \brief Destruct an OdeSystem instance and
    /// free up acquired memory.
    /////////////////////////////////////////////////
    ~OdeSystem()
    {
        if (evolve)
            gsl_odeiv_evolve_free(evolve);

        if (control)
            gsl_odeiv_control_free(control);

        if (step)
            gsl_odeiv_step_free(step);
    }
};



/////////////////////////////////////////////////
/// \brief The function used by the OdeSystem
/// instance to integrate the ODE.
///
/// \param x double
/// \param y const double[]
/// \param dydx double[]
/// \param params void*
/// \return int
///
/////////////////////////////////////////////////
static int odeFunction(double x, const double y[], double dydx[], void* params)
{
    mu::Array v;
    static mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    OdeParams* par = static_cast<OdeParams*>(params);

    // Variablen zuweisen
    _defVars.vValue[0][0] = mu::Value(x);

    for (int i = 0; i < par->dims; i++)
    {
        *(par->vars.find("y"+toString(i+1))->second) = mu::Value(y[i]);
    }

    v = _parser.Eval();

    for (size_t i = 0; i < v.size(); i++)
    {
        dydx[i] = v[i].getNum().asF64();
    }

    return GSL_SUCCESS;
}



/////////////////////////////////////////////////
/// \brief The ODE jacobian. Does nothing.
///
/// \param x double
/// \param y const double[]
/// \param dydx double[]
/// \param dfdt double[]
/// \param params void*
/// \return int
///
/////////////////////////////////////////////////
static int jacobian(double x, const double y[], double dydx[], double dfdt[], void* params)
{
    return GSL_SUCCESS;
}


/////////////////////////////////////////////////
/// \brief Integrate the ODE system.
///
/// \param params OdeParams
/// \param vStartValues const std::vector<double>&
/// \param timeInterval const Interval&
/// \param nSamples int
/// \param nLyapuSamples int
/// \param _idx Indices&
/// \param sTarget const std::string&
/// \return bool
///
/////////////////////////////////////////////////
static bool integrate(OdeParams params, const std::vector<double>& vStartValues, const Interval& timeInterval, int nSamples, int nLyapuSamples, Indices& _idx, const std::string& sTarget)
{
    bool systemPrints = NumeReKernel::getInstance()->getSettings().systemPrints();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();

    bool bCalcLyapunov = nLyapuSamples > 0;
    bool bAllowCacheClearance = _idx.row.isFullRange(_data.getLines(sTarget));

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _idx.row.front() + nSamples);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + params.dims + (long long int)bCalcLyapunov*2);

    if (bAllowCacheClearance)
        _data.deleteBulk(sTarget, _idx.row, _idx.col);

    std::vector<double> y(params.dims, 0.0);
    std::vector<double> y2;

    if (bCalcLyapunov)
        y2.resize(params.dims, 0.0);

    // Startwerte festlegen
    for (int i = 0; i < params.dims; i++)
    {
        if (i < (int)vStartValues.size())
            y[i] = vStartValues[i];

        if (bCalcLyapunov)
        {
            if (i < (int)vStartValues.size())
                y2[i] = vStartValues[i];

            if (!i)
                y2[i] += 1e-6; // Lyapunov specific offset
        }
    }

    params.vars = _parser.GetVar();

    if (params.dims > 1)
        _parser.SetExpr("{" + params.func + "}");
    else
        _parser.SetExpr(params.func);

    // Write the x column title
    if (bAllowCacheClearance || !_idx.row.front())
        _data.setHeadLineElement(_idx.col.front(), sTarget, "x");

    double startTime = timeInterval.min();

    // Write x start value
    _data.writeToTable(_idx.row.front(), _idx.col.front(), sTarget, startTime);

    // Write position column titles and start values
    for (int j = 0; j < params.dims; j++)
    {
        if (_idx.col[j+1] == VectorIndex::INVALID)
            break;

        if (bAllowCacheClearance || !_idx.row.front())
            _data.setHeadLineElement(_idx.col[1+j], sTarget, "y_"+toString(j+1));

        _data.writeToTable(_idx.row.front(), _idx.col[j+1], sTarget, y[j]);
    }

    // Write the Lyapunov column titles
    if (bCalcLyapunov && (bAllowCacheClearance || !_idx.row.front()) && _idx.col[params.dims+2] != VectorIndex::INVALID)
    {
        _data.setHeadLineElement(_idx.col[1+params.dims], sTarget, "x_lypnv");
        _data.setHeadLineElement(_idx.col[2+params.dims], sTarget, "lyapunov");
    }

    // Initialize systems
    OdeSystem ode;
    OdeSystem lyapunov;

    ode.init(odeFunction, jacobian, params);

    if (bCalcLyapunov)
        lyapunov.init(odeFunction, jacobian, params);

    double lyapu[2] = {0.0, 0.0};
    double dist[2] = {1e-6, 0.0};
    double t = startTime;
    double t2 = startTime;
    double dt = timeInterval.range() / (nSamples - 1);

    double h = params.eps_rel;
    double h2 = params.eps_rel;

    _defVars.vValue[0][0] = mu::Value(startTime);
    time_t tTimeControl = time(nullptr);

    if (systemPrints)
        NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("ODESOLVER_SOLVE_SYSTEM") + " ..."));

    // integrieren
    for (size_t i = 0; i < (size_t)nSamples; i++)
    {
        if (time(nullptr) - tTimeControl > 1 && systemPrints)
        {
            NumeReKernel::printPreFmt("\r|-> " + _lang.get("ODESOLVER_SOLVE_SYSTEM") + " ... "
                                      + toString((int)(i*100.0/(double)(nSamples-1))) + " %");
        }

        if (NumeReKernel::GetAsyncCancelState())
        {
            NumeReKernel::printPreFmt(" " + _lang.get("COMMON_CANCEL") + ".\n");
            throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
        }

        if (_idx.row.size() <= i+1)
            break;

        // Update current target time
        double currentTime = timeInterval(i, nSamples).real();

        // Iterate as long as the simulation time is smaller than the
        // target time
        while (t < currentTime)
        {
            if (GSL_SUCCESS != ode.iterate(&t, currentTime, &h, y))
                break;
        }

        // Store the current simulated time (not the target time)
        _data.writeToTable(_idx.row[i+1], _idx.col[0], sTarget, t);

        // Store the integrated results
        for (int j = 0; j < params.dims; j++)
        {
            if (_idx.col[j+1] == VectorIndex::INVALID)
                break;

            _data.writeToTable(_idx.row[i+1], _idx.col[j+1], sTarget, y[j]);
        }

        // Do we need to calculate Lyapunov values?
        if (bCalcLyapunov)
        {
            h2 = params.eps_rel;

            // Iterate as long as the simulation time is smaller than the
            // target time
            while (t2 < currentTime)
            {
                if (GSL_SUCCESS != lyapunov.iterate(&t2, currentTime, &h2, y2))
                    break;
            }

            dist[1] = 0.0;

            // Calculate the distance
            for (int n = 0; n < params.dims; n++)
            {
                dist[1] += (y[n]-y2[n])*(y[n]-y2[n]);
            }

            dist[1] = std::sqrt(dist[1]);
            lyapu[1] = std::log(dist[1]/dist[0])/dt;

            // Calculate the current Lyapunov parameter
            lyapu[0] = (i*lyapu[0] + lyapu[1])/(double)(i+1);

            // Store the Lyapunov parameter
            if (!((i+1) % nLyapuSamples) && _idx.col[params.dims + 2] != VectorIndex::INVALID)
            {
                _data.writeToTable((i+1)/nLyapuSamples-1, _idx.col[params.dims+1], sTarget, currentTime);
                _data.writeToTable((i+1)/nLyapuSamples-1, _idx.col[params.dims+2], sTarget, lyapu[0]);
            }

            // Update the starting values
            for (int n = 0; n < params.dims; n++)
            {
                y2[n] = y[n] + (y2[n]-y[n])*dist[0]/dist[1];
            }

            lyapunov.reset();
        }
    }

    if (systemPrints)
        NumeReKernel::printPreFmt(" " + _lang.get("COMMON_SUCCESS") + ".\n");

    return true;
}


/////////////////////////////////////////////////
/// \brief Solve the ODE.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool Odesolver::solve(CommandLineParser& cmdParser)
{
    OdeParams params;
    params.func = cmdParser.getExprAsMathExpression(true);
    params.method = cmdParser.getParameterValue("method");

    Indices _idx;

    if (!cmdParser.getParameterList().length())
        throw SyntaxError(SyntaxError::NO_OPTIONS_FOR_ODE, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    if (!params.func.length())
        throw SyntaxError(SyntaxError::NO_EXPRESSION_FOR_ODE, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    std::string sTarget = cmdParser.getTargetTable(_idx, "ode");

    if (!isValidIndexSet(_idx))
        throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), "", _idx.row.to_string() + ", " + _idx.col.to_string());

    std::vector<double> vStartValues;

    if (cmdParser.hasParam("fx0"))
    {
        mu::Array v = cmdParser.getParsedParameterValue("fx0");

        for (size_t i = 0; i < v.size(); i++)
        {
            vStartValues.push_back(v[i].getNum().asF64());
        }
    }

    int nSamples = 100;

    if (cmdParser.hasParam("samples"))
    {
        nSamples = cmdParser.getParsedParameterValue("samples").getAsScalarInt();

        if (nSamples <= 0)
            nSamples = 100;
    }

    int nLyapuSamples = 0;

    if (cmdParser.hasParam("lyapunov"))
    {
        if (nSamples <= 200)
            nLyapuSamples = nSamples / 10;
        else if (nSamples <= 1000)
            nLyapuSamples = nSamples / 20;
        else
            nLyapuSamples = nSamples / 100;
    }

    if (cmdParser.hasParam("tol"))
    {
        mu::Array v = cmdParser.getParsedParameterValue("tol");

        if (v.size() > 1)
        {
            params.eps_rel = std::abs(v[0].getNum().asF64());
            params.eps_abs = std::abs(v[1].getNum().asF64());
        }
        else
        {
            params.eps_rel = std::abs(v[0].getNum().asF64());
            params.eps_abs = std::abs(v[0].getNum().asF64());
        }

        if (std::isnan(params.eps_rel) || std::isinf(params.eps_rel))
            params.eps_rel = 1e-6;

        if (std::isnan(params.eps_abs) || std::isinf(params.eps_abs))
            params.eps_abs = 1e-6;
    }

    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();

    // Dimension des ODE-Systems bestimmen: odesolve dy1 = y2*x, dy2 = sin(y1)
    _parser.SetInitValue(mu::Value(0.0));

    try
    {
        _parser.SetExpr(params.func);
        _parser.Eval(params.dims);

        std::string sVarDecl = "y1";

        for (int i = 1; i < params.dims; i++)
            sVarDecl += ", y" + toString(i+1);

        _parser.SetExpr(sVarDecl);
        _parser.Eval();
    }
    catch (...)
    {
        _parser.SetInitValue(mu::Value());
        throw;
    }

    _parser.SetInitValue(mu::Value());

    IntervalSet ivlSet = cmdParser.parseIntervals(false);

    if (!ivlSet.intervals.size()
        || std::isnan(ivlSet.intervals[0].min())
        || std::isinf(ivlSet.intervals[0].min())
        || std::isnan(ivlSet.intervals[0].max())
        || std::isinf(ivlSet.intervals[0].max()))
        throw SyntaxError(SyntaxError::NO_INTERVAL_FOR_ODE, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    return integrate(params, vStartValues, ivlSet.intervals[0], nSamples, nLyapuSamples, _idx, sTarget);
}


