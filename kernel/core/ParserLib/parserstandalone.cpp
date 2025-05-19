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

#include "../utils/timer.hpp"

Language _lang;

class BaseValue
{
    public:
        mu::DataType m_type;

        virtual ~BaseValue() {}
        virtual BaseValue& operator=(const BaseValue& other) = 0;
        virtual BaseValue* clone() = 0;

        virtual BaseValue* operator+(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual BaseValue* operator-() const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual BaseValue* operator-(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual BaseValue* operator/(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual BaseValue* operator*(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }

        virtual BaseValue& operator+=(const BaseValue& other)
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual BaseValue& operator-=(const BaseValue& other)
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual BaseValue& operator/=(const BaseValue& other)
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual BaseValue& operator*=(const BaseValue& other)
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }

        virtual BaseValue& pow(const BaseValue& other)
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }

        virtual bool operator!() const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual bool operator==(const BaseValue& other) const
        {
            throw m_type == other.m_type;
        }
        virtual bool operator!=(const BaseValue& other) const
        {
            throw m_type != other.m_type;
        }
        virtual bool operator<(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual bool operator<=(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual bool operator>(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual bool operator>=(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }

        virtual bool operator&&(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }
        virtual bool operator||(const BaseValue& other) const
        {
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        }

        virtual size_t getBytes() const = 0;

        virtual bool isMethod(const std::string& sMethod) const
        {
            return false;
        }
        virtual BaseValue* call(const std::string& sMethod) const
        {
            throw mu::ParserError(mu::ecMETHOD_ERROR, sMethod);
        }
        virtual BaseValue* call(const std::string& sMethod, const BaseValue& arg1) const
        {
            throw mu::ParserError(mu::ecMETHOD_ERROR, sMethod);
        }
        virtual BaseValue* call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const
        {
            throw mu::ParserError(mu::ecMETHOD_ERROR, sMethod);
        }
};

#define IMPL_DEFAULTS(CLASS, ID, TYPE, ATTR)                  \
private:                                                      \
    TYPE ATTR;                                                \
public:                                                       \
    CLASS() : BaseValue(), ATTR()                             \
    {                                                         \
        m_type = ID;                                          \
    }                                                         \
    CLASS(const TYPE& val) : BaseValue(), ATTR(val)           \
    {                                                         \
        m_type = ID;                                          \
    }                                                         \
    CLASS(const CLASS& other): BaseValue()                    \
    {                                                         \
        m_type = ID;                                          \
        ATTR = other.ATTR;                                    \
    }                                                         \
    CLASS(CLASS&& other) = default;                           \
    CLASS(const BaseValue& other) : BaseValue()               \
    {                                                         \
        m_type = ID;                                          \
        if (other.m_type == ID)                               \
            ATTR = static_cast<const CLASS&>(other).ATTR;     \
        else                                                  \
            throw mu::ParserError(mu::ecASSIGNED_TYPE_MISMATCH);      \
    }                                                         \
    CLASS& operator=(const BaseValue& other) override         \
    {                                                         \
        if (other.m_type == ID)                               \
            ATTR = static_cast<const CLASS&>(other).ATTR;     \
        else                                                  \
            throw mu::ParserError(mu::ecASSIGNED_TYPE_MISMATCH);      \
        return *this;                                         \
    }                                                         \
    CLASS& operator=(const TYPE& val)                         \
    {                                                         \
        ATTR = val;                                           \
        return *this;                                         \
    }                                                         \
    CLASS& operator=(const CLASS& other)                      \
    {                                                         \
        ATTR = other.ATTR;                                    \
        return *this;                                         \
    }                                                         \
    CLASS& operator=(CLASS&& other) = default;                \
    BaseValue* clone() override                               \
    {                                                         \
        return new CLASS(*this);                              \
    }                                                         \
    TYPE& get()                                               \
    {                                                         \
        return ATTR;                                          \
    }                                                         \
    const TYPE& get() const                                   \
    {                                                         \
        return ATTR;                                          \
    }

class CategoryValue : public BaseValue
{
    IMPL_DEFAULTS(CategoryValue, mu::TYPE_CATEGORY, mu::Category, m_val)

    BaseValue& operator+=(const BaseValue& other) override
    {
        if (other.m_type == mu::TYPE_CATEGORY)
            m_val.name += static_cast<const CategoryValue&>(other).m_val.name;
        else
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        return *this;
    }
    BaseValue* operator+(const BaseValue& other) const override
    {
        if (other.m_type == mu::TYPE_CATEGORY)
            return new CategoryValue(mu::Category(m_val.val, m_val.name + static_cast<const CategoryValue&>(other).m_val.name));

        throw mu::ParserError(mu::ecTYPE_MISMATCH);
    }

    size_t getBytes() const override
    {
        return m_val.name.length() + m_val.val.getBytes();
    }
};

class NumericValue : public BaseValue
{
    IMPL_DEFAULTS(NumericValue, mu::TYPE_NUMERICAL, mu::Numerical, m_val)

    BaseValue& operator+=(const BaseValue& other) override
    {
        if (other.m_type == mu::TYPE_NUMERICAL)
            m_val += static_cast<const NumericValue&>(other).m_val;
        else if (other.m_type == mu::TYPE_CATEGORY)
            m_val += static_cast<const CategoryValue&>(other).get().val;
        else
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        return *this;
    }
    BaseValue* operator+(const BaseValue& other) const override
    {
        if (other.m_type == mu::TYPE_NUMERICAL)
            return new NumericValue(m_val + static_cast<const NumericValue&>(other).m_val);
        else if (other.m_type == mu::TYPE_CATEGORY)
            return new NumericValue(m_val + static_cast<const CategoryValue&>(other).get().val);

        throw mu::ParserError(mu::ecTYPE_MISMATCH);
    }

    size_t getBytes() const override
    {
        return m_val.getBytes();
    }
};


class StringValue : public BaseValue
{
    IMPL_DEFAULTS(StringValue, mu::TYPE_STRING, std::string, m_val)

    BaseValue& operator+=(const BaseValue& other) override
    {
        if (other.m_type == mu::TYPE_STRING)
            m_val += static_cast<const StringValue&>(other).m_val;
        else if (other.m_type == mu::TYPE_STRING)
            m_val += static_cast<const CategoryValue&>(other).get().name;
        else
            throw mu::ParserError(mu::ecTYPE_MISMATCH);
        return *this;
    }
    BaseValue* operator+(const BaseValue& other) const override
    {
        if (other.m_type == mu::TYPE_STRING)
            return new StringValue(m_val + static_cast<const StringValue&>(other).m_val);
        else if (other.m_type == mu::TYPE_CATEGORY)
            return new StringValue(m_val + static_cast<const CategoryValue&>(other).get().name);

        throw mu::ParserError(mu::ecTYPE_MISMATCH);
    }

    size_t getBytes() const override
    {
        return m_val.length();
    }
};


class Value : public std::unique_ptr<BaseValue>
{
    public:
        Value() : std::unique_ptr<BaseValue>() {}
        Value(BaseValue* other) : std::unique_ptr<BaseValue>(other) {}
        Value(const mu::Numerical& val) : Value()
        {
            reset(new NumericValue(val));
        }
        Value(bool logical) : Value()
        {
            reset(new NumericValue(logical));
        }
        Value(const std::string& val) : Value()
        {
            reset(new StringValue(val));
        }
        Value(const char* val) : Value()
        {
            reset(new StringValue(val));
        }
        Value(const Value& other)
        {
            if (other)
                reset(other->clone());
        }
        Value(Value&& other)
        {
            reset(other.release());
        }
        Value& operator=(const Value& other)
        {
            if ((!get() && other) || (get()->m_type != other->m_type))
                reset(other->clone());
            else
                *get() = *other;

            return *this;
        }
        Value& operator+=(const Value& other)
        {
            if (get() && other)
                *get() += *other;

            return *this;
        }
};

struct arr : public std::vector<Value>
{
    arr() = default;
    arr(const arr& other) : arr()
    {
        resize(other.size());

        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i).reset(other[i]->clone());
        }
    }
    arr(arr&& other) = default;
    arr& operator=(const arr& other)
    {
        //Timer t("arr::operator=()");
        if (size() != other.size())
            resize(other.size());

        for (size_t i = 0; i < other.size(); i++)
        {
            operator[](i) = other[i];
        }

        return *this;
    }
    arr& operator+=(const arr& other)
    {
        for (size_t i = 0; i < std::max(size(), other.size()); i++)
        {
            operator[](i) += other[i];
        }

        return *this;
    }
    arr operator+(const arr& other) const
    {
        arr res;
        res.resize(std::max(size(), other.size()));

        for (size_t i = 0; i < res.size(); i++)
        {
            res[i].reset(*operator[](i) + *other[i]);
        }

        return res;
    }
};


void testfunc(const std::vector<int>& oplist)
{
    arr testArr;

    testArr.push_back(Value(0.0));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(""));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(""));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(""));
    testArr.push_back(Value(0.0));
    testArr.push_back(Value(""));

    {
        Timer t("arr");
        for (int op : oplist)
        {
            if (op == 0)
                testArr = testArr;
            else if (op == 1)
                testArr += testArr;
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
    testArr.push_back(Value(""));
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
                testArr += testArr;
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


