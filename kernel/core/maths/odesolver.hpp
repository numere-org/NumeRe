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


#ifndef ODESOLVER_HPP
#define ODESOLVER_HPP
#include <iostream>
#include <string>
#include <vector>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_odeiv.h>

#include "../ParserLib/muParser.h"
#include "../utils/tools.hpp"
#include "../datamanagement/memorymanager.hpp"
#include "define.hpp"
#include "parser_functions.hpp"
#include "../settings.hpp"
#include "../ui/error.hpp"

using namespace std;
using namespace mu;

class Odesolver
{
    private:
        MemoryManager* _odeData;
        FunctionDefinitionManager* _odeFunctions;
        Settings* _odeSettings;
        const gsl_odeiv_step_type* odeStepType;
        gsl_odeiv_step* odeStep;
        gsl_odeiv_control* odeControl;
        gsl_odeiv_evolve* odeEvolve;

        static int odeFunction(double x, const double y[], double dydx[], void* params);
        inline static int jacobian(double x, const double y[], double dydx[], double dfdt[], void* params)
            {return GSL_SUCCESS;}

    public:
        static Parser* _odeParser;
        static int nDimensions;
        static mu::varmap_type mVars;

        Odesolver();
        Odesolver(Parser* _parser, MemoryManager* _data, FunctionDefinitionManager* _functions, Settings* _option);
        ~Odesolver();

        inline void setObjects(Parser* _parser, MemoryManager* _data, FunctionDefinitionManager* _functions, Settings* _option)
            {
                _odeParser = _parser;
                _odeData = _data;
                _odeFunctions = _functions;
                _odeSettings = _option;
                return;
            }
        bool solve(const string& sCmd);
};

typedef int (Odesolver::*odeFunction)(double, const double*, double*, void*);

#endif // ODESOLVER_HPP

