
#ifndef TESTTYPES_HPP
#define TESTTYPES_HPP

#include "muParser.h"
//#define TESTTYPES_HEADERONLY
//#define STRUCTURES_HEADERONLY

#ifdef TESTTYPES_HEADERONLY

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
        else if (other.m_type == mu::TYPE_CATEGORY)
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

#else

class BaseValue
{
    public:
        mu::DataType m_type;

        virtual ~BaseValue() {}
        virtual BaseValue& operator=(const BaseValue& other) = 0;
        virtual BaseValue* clone() = 0;

        virtual BaseValue* operator+(const BaseValue& other) const;
        virtual BaseValue* operator-() const;
        virtual BaseValue* operator-(const BaseValue& other) const;
        virtual BaseValue* operator/(const BaseValue& other) const;
        virtual BaseValue* operator*(const BaseValue& other) const;

        virtual BaseValue& operator+=(const BaseValue& other);
        virtual BaseValue& operator-=(const BaseValue& other);
        virtual BaseValue& operator/=(const BaseValue& other);
        virtual BaseValue& operator*=(const BaseValue& other);

        virtual BaseValue& pow(const BaseValue& other);

        virtual bool operator!() const;
        virtual bool operator==(const BaseValue& other) const;
        virtual bool operator!=(const BaseValue& other) const;
        virtual bool operator<(const BaseValue& other) const;
        virtual bool operator<=(const BaseValue& other) const;
        virtual bool operator>(const BaseValue& other) const;
        virtual bool operator>=(const BaseValue& other) const;

        virtual bool operator&&(const BaseValue& other) const;
        virtual bool operator||(const BaseValue& other) const;

        virtual size_t getBytes() const = 0;

        virtual bool isMethod(const std::string& sMethod) const;
        virtual BaseValue* call(const std::string& sMethod) const;
        virtual BaseValue* call(const std::string& sMethod, const BaseValue& arg1) const;
        virtual BaseValue* call(const std::string& sMethod, const BaseValue& arg1, const BaseValue& arg2) const;
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

    BaseValue& operator+=(const BaseValue& other) override;
    BaseValue* operator+(const BaseValue& other) const override;

    size_t getBytes() const override
    {
        return m_val.name.length() + m_val.val.getBytes();
    }
};

class NumericValue : public BaseValue
{
    IMPL_DEFAULTS(NumericValue, mu::TYPE_NUMERICAL, mu::Numerical, m_val)

    BaseValue& operator+=(const BaseValue& other) override;
    BaseValue* operator+(const BaseValue& other) const override;

    size_t getBytes() const override
    {
        return m_val.getBytes();
    }
};


class StringValue : public BaseValue
{
    IMPL_DEFAULTS(StringValue, mu::TYPE_STRING, std::string, m_val)

    BaseValue& operator+=(const BaseValue& other) override;
    BaseValue* operator+(const BaseValue& other) const override;

    size_t getBytes() const override
    {
        return m_val.length();
    }
};

#endif


#ifdef STRUCTURES_HEADERONLY

class Value : public std::unique_ptr<BaseValue>
{
    public:
        Value();
        Value(BaseValue* other);
        Value(const mu::Numerical& val);
        Value(bool logical);
        Value(const std::string& val);
        Value(const char* val);
        Value(const Value& other);
        /*Value(Value&& other)
        {
            reset(other.release());
        }*/
        Value& operator=(const Value& other)
        {
            if (!other.get())
                reset(nullptr);
            else if (!get() || (get()->m_type != other->m_type))
                reset(other->clone());
            else if (this != &other)
                *get() = *other.get();

            return *this;
        }
        Value& operator+=(const Value& other)
        {
            if (get() && other)
                *get() += *other;

            return *this;
        }
};

struct arr : public std::vector<mu::Value>
{
    arr();
    arr(const arr& other);
    //arr(arr&& other) = default;
    arr& operator=(const arr& other)
    {
        if (other.size() == 1)
        {
            if (size() != 1)
                resize(1);

            front() = other.front();
        }
        else
        {
            resize(other.size());

            for (size_t i = 0; i < other.size(); i++)
            {
                operator[](i) = other[i];
            }
        }

        m_commonType = other.m_commonType;
        return *this;
    }
    arr& operator+=(const arr& other)
    {
        if (size() < other.size())
            operator=(operator+(other));
        else
        {
            for (size_t i = 0; i < size(); i++)
            {
                operator[](i) += other.get(i);
            }
        }

        return *this;
    }
    arr operator+(const arr& other) const
    {
        arr res;
        res.resize(std::max(size(), other.size()));

        for (size_t i = 0; i < res.size(); i++)
        {
            res[i].reset(*operator[](i).get() + *other[i].get());
        }

        return res;
    }
    const mu::Value& get(size_t i) const
    {
        if (size() == 1u)
            return front();
        else if (size() <= i)
            return m_default;

        return operator[](i);
    }

    protected:
    static const mu::Value m_default;
    mutable mu::DataType m_commonType;
};


#else


class Value : public std::unique_ptr<BaseValue>
{
    public:
        Value();
        Value(BaseValue* other);
        Value(const mu::Numerical& val);
        Value(bool logical);
        Value(const std::string& val);
        Value(const char* val);
        Value(const Value& other);
        /*Value(Value&& other)
        {
            reset(other.release());
        }*/
        Value& operator=(const Value& other);
        Value& operator+=(const Value& other);
};

struct arr : public std::vector<mu::Value>
{
    arr();
    arr(const arr& other);
    //arr(arr&& other) = default;
    arr& operator=(const arr& other);
    arr& operator+=(const arr& other);
    arr operator+(const arr& other) const;
    const mu::Value& get(size_t i) const;

    protected:
    static const mu::Value m_default;
    mutable mu::DataType m_commonType;
};

#endif

struct Var : public arr
{
    Var& operator=(const mu::Value& other);
    Var& operator=(const arr& other);
    Var& operator=(const Var& other);
};

#endif // TESTTYPES_HPP

