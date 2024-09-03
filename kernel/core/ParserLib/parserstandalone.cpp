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
    _parser.EnableDebugDump(true, true);

    // trigonometric functions
    _parser.DefineFun("sin", numfnc_sin);
    _parser.DefineFun("cos", numfnc_cos);
    _parser.DefineFun("tan", numfnc_tan);
    // arcus functions
    _parser.DefineFun("asin", numfnc_asin);
    _parser.DefineFun("arcsin", numfnc_asin);
    _parser.DefineFun("acos", numfnc_acos);
    _parser.DefineFun("arccos", numfnc_acos);
    _parser.DefineFun("atan", numfnc_atan);
    _parser.DefineFun("arctan", numfnc_atan);
    //DefineFun("atan2", ATan2);
    // hyperbolic functions
    _parser.DefineFun("sinh", numfnc_sinh);
    _parser.DefineFun("cosh", numfnc_cosh);
    _parser.DefineFun("tanh", numfnc_tanh);
    // arcus hyperbolic functions
    _parser.DefineFun("asinh", numfnc_asinh);
    _parser.DefineFun("arsinh", numfnc_asinh);
    _parser.DefineFun("acosh", numfnc_acosh);
    _parser.DefineFun("arcosh", numfnc_acosh);
    _parser.DefineFun("atanh", numfnc_atanh);
    _parser.DefineFun("artanh", numfnc_atanh);
    // Logarithm functions
    _parser.DefineFun("log2", numfnc_log2);
    _parser.DefineFun("log10", numfnc_log10);
    _parser.DefineFun("log", numfnc_log10);
    _parser.DefineFun("ln", numfnc_ln);
    // misc
    _parser.DefineFun("exp", numfnc_exp);
    _parser.DefineFun("sqrt", numfnc_sqrt);
    _parser.DefineFun("sign", numfnc_sign);
    _parser.DefineFun("rint", numfnc_rint);
    _parser.DefineFun("abs", numfnc_abs);
    _parser.DefineFun("time", timfnc_time, false);
    _parser.DefineFun("date", timfnc_date, true, 2);
    _parser.DefineFun("as_date", timfnc_as_date, true, 2);

    _parser.DefineFun("logtoidx", numfnc_logtoidx);
    _parser.DefineFun("idxtolog", numfnc_idxtolog);
    _parser.DefineFun("strlen", strfnc_strlen);
    _parser.DefineFun("substr", strfnc_substr, true, 1);
    _parser.DefineFun("firstch", strfnc_firstch);
    _parser.DefineFun("lastch", strfnc_lastch);
    _parser.DefineFun("to_string", strfnc_to_string);
    _parser.DefineFun("strjoin", strfnc_strjoin, true, 2);
    _parser.DefineFun("valtostr", strfnc_valtostr, true, 2);
    _parser.DefineFun("landau_rd", rndfnc_landau_rd, false, 1);

    _parser.DefinePostfixOprt("i", numfnc_imaginaryUnit);

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
    mu::Variable categories;
    categories.push_back(mu::Value(mu::Category(1, "Hello")));
    categories.push_back(mu::Value(mu::Category(2, "World")));
    categories.push_back(mu::Value(mu::Category(1, "Hello")));
    _parser.DefineVar("vect", &vectorVar);
    _parser.DefineVar("logicals", &logicalVar);
    _parser.DefineVar("var", &var);
    _parser.DefineVar("str", &strvar);
    _parser.DefineVar("strvect", &stringVect);
    _parser.DefineVar("mixed", &mixed);
    _parser.DefineVar("categories", &categories);

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
                std::cout << i+1 << ">> " << res[i].print() << " [" << res[i].printDims() << " " << res[i].getCommonTypeAsString()
                          << " w/ " << res[i].getBytes() << " byte]" << std::endl;
            }
        }
        catch (...)
        {
            std::cout << " >> ERROR in " << sInput << std::endl;
        }
    }

    return 0;
}


