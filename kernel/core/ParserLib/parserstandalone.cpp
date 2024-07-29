/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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


#include <string>
#include "muParser.h"
#include "../ui/language.hpp"
#include "../structures.hpp"
#include "../maths/functionimplementation.hpp"
#include "../strings/functionimplementation.hpp"

Language _lang;


int main()
{
    mu::Parser _parser;
    _parser.EnableDebugDump(true, false);

    // trigonometric functions
    _parser.DefineFun("sin", parser_sin);
    _parser.DefineFun("cos", parser_cos);
    _parser.DefineFun("tan", parser_tan);
    // arcus functions
    _parser.DefineFun("asin", parser_asin);
    _parser.DefineFun("arcsin", parser_asin);
    _parser.DefineFun("acos", parser_acos);
    _parser.DefineFun("arccos", parser_acos);
    _parser.DefineFun("atan", parser_atan);
    _parser.DefineFun("arctan", parser_atan);
    //DefineFun("atan2", ATan2);
    // hyperbolic functions
    _parser.DefineFun("sinh", parser_sinh);
    _parser.DefineFun("cosh", parser_cosh);
    _parser.DefineFun("tanh", parser_tanh);
    // arcus hyperbolic functions
    _parser.DefineFun("asinh", parser_asinh);
    _parser.DefineFun("arsinh", parser_asinh);
    _parser.DefineFun("acosh", parser_acosh);
    _parser.DefineFun("arcosh", parser_acosh);
    _parser.DefineFun("atanh", parser_atanh);
    _parser.DefineFun("artanh", parser_atanh);
    // Logarithm functions
    _parser.DefineFun("log2", parser_log2);
    _parser.DefineFun("log10", parser_log10);
    _parser.DefineFun("log", parser_log10);
    _parser.DefineFun("ln", parser_ln);
    // misc
    _parser.DefineFun("exp", parser_exp);
    _parser.DefineFun("sqrt", parser_sqrt);
    _parser.DefineFun("sign", parser_sign);
    _parser.DefineFun("rint", parser_rint);
    _parser.DefineFun("abs", parser_abs);
    _parser.DefineFun("time", parser_time);
    _parser.DefineFun("date", parser_date);

    _parser.DefineFun("logtoidx", parser_logtoidx);
    _parser.DefineFun("idxtolog", parser_idxtolog);
    _parser.DefineFun("strlen", strfnc_strlen);
    _parser.DefineFun("substr", strfnc_substr, true, 1);
    _parser.DefineFun("firstch", strfnc_firstch);
    _parser.DefineFun("lastch", strfnc_lastch);
    _parser.DefineFun("strjoin", strfnc_strjoin, true, 2);
    _parser.DefineFun("landau_rd", parser_rd_landau_rd, false, 1);

    std::string sInput;
    int nResults;
    std::vector<mu::Numerical> vals({1,2,3,4,5,6,7,8,9});
    std::vector<mu::Numerical> logicals({false,true,true,false,true,true,false});
    std::vector<std::string> strings({"Hello", "World", "More", "Strings", "in", "Here"});
    mu::Variable vectorVar(vals);
    mu::Variable logicalVar(logicals);
    mu::Variable stringVect(strings);
    mu::Variable var(std::complex<double>(4,8));
    mu::Variable strvar(std::string("Hello"));
    mu::Variable mixed;
    mixed.push_back(mu::Value(std::string("1")));
    mixed.push_back(mu::Value(2.0));
    mixed.push_back(mu::Value(std::string("3")));
    _parser.DefineVar("vect", &vectorVar);
    _parser.DefineVar("logicals", &logicalVar);
    _parser.DefineVar("var", &var);
    _parser.DefineVar("str", &strvar);
    _parser.DefineVar("strvect", &stringVect);
    _parser.DefineVar("mixed", &mixed);

    while (true)
    {
        std::cout << " << ";
        std::getline(std::cin, sInput);

        if (sInput == "quit")
            break;

        try
        {
            mu::Array* res;

            _parser.SetExpr(sInput);
            res = _parser.Eval(nResults);

            for (int i = 0; i < nResults; i++)
            {
                std::cout << i+1 << ">> " << res[i].print() << std::endl;
            }
        }
        catch (...)
        {
            std::cout << " >> ERROR in " << sInput << std::endl;
        }
    }

    return 0;
}


