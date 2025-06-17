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
#include "../maths/functionimplementation.hpp"
#include "../strings/functionimplementation.hpp"

#include "../utils/timer.hpp"
#include "testTypes.hpp"

Language _lang;

void testfunc(const std::vector<int>& oplist)
{
    arr testArr;

    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));

    {
        Timer t("arr");
        for (int op : oplist)
        {
            if (op == 0)
                testArr = testArr;
            else if (op == 1)
                testArr = testArr + testArr;
        }
    }
}


void testfunc2(const std::vector<int>& oplist)
{
    mu::Array testArr;

    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));

    {
        Timer t("mu::Array");
        for (int op : oplist)
        {
            if (op == 0)
                testArr = testArr;
            else if (op == 1)
                testArr += testArr;
        }
    }
}

void testfunc3(const std::vector<int>& oplist)
{
    arr testArr;

    /*testArr.push_back(Value(0.0));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(0.0));*/
    testArr.push_back(mu::Value(""));
    /*testArr.push_back(Value(0.0));
    testArr.push_back(Value(""));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(""));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(""));*/

    {
        Timer t("arr-singleton");
        for (int op : oplist)
        {
            if (op == 0)
                testArr = testArr;
            else if (op == 1)
                testArr = testArr + testArr;
        }
    }
}


void testfunc4(const std::vector<int>& oplist)
{
    mu::Array testArr;

    /*testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));*/
    testArr.push_back(mu::Value(""));
    /*testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));
    testArr.push_back(mu::Value(0.0));
    testArr.push_back(mu::Value(""));*/

    {
        Timer t("mu::Array-singleton");
        for (int op : oplist)
        {
            if (op == 0)
                testArr = testArr;
            else if (op == 1)
                testArr += testArr;
        }
    }
}


void runtests()
{
    testfunc({1,0,1,0,1,0,1,1,0,1,0,1,0});
    testfunc2({1,0,1,0,1,0,1,1,0,1,0,1,0});
    testfunc({1,0,1,1,1,0,1,1,0,1,1,0,0});
    testfunc2({1,0,1,1,1,0,1,1,0,1,1,0,0});
    testfunc({1,0,0,1,1,0,1,0,1,1,0,1,1});
    testfunc2({1,0,0,1,1,0,1,0,1,1,0,1,1});
    testfunc({1,0,1,0,1,0,1,1,0,1,0,1,0});
    testfunc2({1,0,1,0,1,0,1,1,0,1,0,1,0});
    testfunc3({1,0,1,0,1,0,1,1,0,1,0,1,0});
    testfunc4({1,0,1,0,1,0,1,1,0,1,0,1,0});
    testfunc3({1,0,1,1,1,0,1,1,0,1,1,0,0});
    testfunc4({1,0,1,1,1,0,1,1,0,1,1,0,0});
    testfunc3({1,0,0,1,1,0,1,0,1,1,0,1,1});
    testfunc4({1,0,0,1,1,0,1,0,1,1,0,1,1});
    testfunc3({1,0,1,0,1,0,1,1,0,1,0,1,0});
    testfunc4({1,0,1,0,1,0,1,1,0,1,0,1,0});
}


float bswapF32(float f)
{
    uint32_t i = *reinterpret_cast<uint32_t*>(&f);
    i = __builtin_bswap32(i);
    return *reinterpret_cast<float*>(&i);
}

double bswapF64(double f)
{
    uint64_t i = *reinterpret_cast<uint64_t*>(&f);
    i = __builtin_bswap64(i);
    return *reinterpret_cast<double*>(&i);
}


mu::Array bswap(const mu::Array& val)
{
    mu::Array ret;

    for (size_t i = 0; i < val.size(); i++)
    {
        const mu::Value& v = val.get(i);

        if (v.getType() != mu::TYPE_NUMERICAL)
            ret.push_back(v);
        else if (v.getNum().getType() == mu::I16)
            ret.push_back(mu::Numerical((int16_t)__builtin_bswap16((uint16_t)v.getNum().asI64())));
        else if (v.getNum().getType() == mu::I32)
            ret.push_back(mu::Numerical((int32_t)__builtin_bswap32((uint32_t)v.getNum().asI64())));
        else if (v.getNum().getType() == mu::I64)
            ret.push_back(mu::Numerical((int64_t)__builtin_bswap64((uint64_t)v.getNum().asI64())));
        else if (v.getNum().getType() == mu::UI16)
            ret.push_back(mu::Numerical(__builtin_bswap16((uint16_t)v.getNum().asUI64())));
        else if (v.getNum().getType() == mu::UI32)
            ret.push_back(mu::Numerical(__builtin_bswap32((uint32_t)v.getNum().asUI64())));
        else if (v.getNum().getType() == mu::UI64)
            ret.push_back(mu::Numerical(__builtin_bswap64(v.getNum().asUI64())));
        else if (v.getNum().getType() == mu::F32)
            ret.push_back(mu::Numerical(bswapF32(v.getNum().asF64())));
        else if (v.getNum().getType() == mu::F64)
            ret.push_back(mu::Numerical(bswapF64(v.getNum().asF64())));
        else if (v.getNum().getType() == mu::CF32)
        {
            std::complex<double> cf0 = v.getNum().asCF64();
            std::complex<float> cf;
            cf.real(bswapF32(cf0.real()));
            cf.imag(bswapF32(cf0.imag()));
            ret.push_back(mu::Numerical(cf));
        }
        else if (v.getNum().getType() == mu::CF64)
        {
            std::complex<double> cf = v.getNum().asCF64();
            cf.real(bswapF64(cf.real()));
            cf.imag(bswapF64(cf.imag()));
            ret.push_back(mu::Numerical(cf));
        }
        else
            ret.push_back(v);
    }

    return ret;
}

mu::Array create_dictstruct(const mu::Array& fields, const mu::Array& vals)
{
    size_t elems = fields.size();

    std::map<std::string, std::unique_ptr<mu::BaseValue>> dict;

    for (size_t i = 0; i < elems; i++)
    {
        std::string fieldName = fields.get(i).getStr();
        dict[fieldName];

        if (!vals.isDefault())
            dict[fieldName].reset(vals.get(i).get()->clone());
    }

    return mu::Value(mu::DictStruct(dict));
}

int main()
{

    mu::Parser _parser;
    _parser.EnableDebugDump(true, false);

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
    _parser.DefineFun("seconds", cast_seconds);
    _parser.DefineFun("weeks", cast_weeks);

    _parser.DefineFun("logtoidx", numfnc_logtoidx);
    _parser.DefineFun("idxtolog", numfnc_idxtolog);
    _parser.DefineFun("strlen", strfnc_strlen);
    _parser.DefineFun("substr", strfnc_substr, true, 1);
    _parser.DefineFun("split", strfnc_split, true, 1);
    _parser.DefineFun("firstch", strfnc_firstch);
    _parser.DefineFun("lastch", strfnc_lastch);
    _parser.DefineFun("to_string", strfnc_to_string);
    _parser.DefineFun("strjoin", strfnc_strjoin, true, 2);
    _parser.DefineFun("valtostr", strfnc_valtostr, true, 2);
    _parser.DefineFun("landau_rd", rndfnc_landau_rd, false, 1);
    _parser.DefineFun("textparse", strfnc_textparse, true, 2);
    _parser.DefineFun("bswap", bswap);
    _parser.DefineFun("dict", create_dictstruct, true, 1);

    _parser.DefinePostfixOprt("i", numfnc_imaginaryUnit);
    _parser.DefineConst("nan", mu::Value(NAN));

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
    mu::Variable pi(mu::Value(3.141593));
    mu::Variable e(mu::Value(2.718282));
    mu::Variable cat;
    cat.push_back(mu::Value(mu::Category(1, "Hello")));
    cat.push_back(mu::Value(mu::TYPE_INVALID));
    cat.push_back(mu::Value(mu::Category(2, "World")));
    cat.push_back(mu::Value(mu::Category(1, "Hello")));
    cat.push_back(mu::Value(mu::Category(2, "World")));
    mu::Variable t(mu::Numerical(-1.26e31f));
    _parser.DefineVar("vect", &vectorVar);
    _parser.DefineVar("logicals", &logicalVar);
    _parser.DefineVar("var", &var);
    _parser.DefineVar("str", &strvar);
    _parser.DefineVar("strvect", &stringVect);
    _parser.DefineVar("mixed", &mixed);
    _parser.DefineVar("categories", &categories);
    _parser.DefineVar("cats", &cat);
    _parser.DefineVar("pi", &pi);
    _parser.DefineVar("e", &e);
    _parser.DefineVar("t", &t);

    runtests();

    while (true)
    {
        std::cout << " << ";
        std::getline(std::cin, sInput);

        if (sInput == "quit")
            break;
        else if (sInput == "runtests")
        {
            runtests();
            continue;
        }

        try
        {
            const mu::StackItem* res;

            _parser.SetExpr(sInput);
            res = _parser.Eval(nResults);
            _parser.Eval();

            for (int i = 0; i < nResults; i++)
            {
                std::cout << i+1 << ">> " << res[i].get().print() << " [" << res[i].get().printDims() << " "
                          << res[i].get().getCommonTypeAsString()
                          << " w/ " << res[i].get().getBytes() << " byte]" << std::endl;
            }
        }
        catch (mu::ParserError& err)
        {
            std::cout << " >> ERROR in " << sInput << std::endl;
            std::cout << " >> " << err.GetMsg() << std::endl;
        }
        catch (...)
        {
            std::cout << " >> ERROR in " << sInput << std::endl;
        }
    }

    return 0;
}


