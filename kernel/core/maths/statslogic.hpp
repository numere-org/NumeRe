/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef STATSLOGIC_HPP
#define STATSLOGIC_HPP

#include "../ParserLib/muParserDef.h"

inline mu::Value conj(const mu::Value& val)
{
    return mu::Numerical(std::conj(val.getNum().asCF64()));
}


/////////////////////////////////////////////////
/// \brief Simplify the creation of some
/// statistics by externalizing the operation
/// code and unifying the driver code.
/////////////////////////////////////////////////
struct StatsLogic
{
    enum OperationType
    {
        OPERATION_ADD,
        OPERATION_MULT,
        OPERATION_ADDSQ,
        OPERATION_ADDSQSUB,
        OPERATION_ADDCBSUB,
        OPERATION_ADDSQSQSUB,
        OPERATION_MAX,
        OPERATION_MIN,
        OPERATION_NUM
    };

    mu::Value m_val;
    mu::Value m_compval;
    OperationType m_type;

    StatsLogic(OperationType type, mu::Value baseVal = 0.0, mu::Value compVal = 0.0)
        : m_val(baseVal), m_compval(compVal), m_type(type) {}

    /////////////////////////////////////////////////
    /// \brief Implements the statistics logic.
    ///
    /// \param newVal const mu::Value&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void operator()(const mu::Value& newVal)
    {
        if (mu::isnan(newVal))
            return;

        switch (m_type)
        {
            case OPERATION_ADD:
                m_val += newVal;
                return;
            case OPERATION_MULT:
                m_val *= newVal;
                return;
            case OPERATION_ADDSQ:
                m_val += newVal * conj(newVal);
                return;
            case OPERATION_ADDSQSUB:
                m_val += (newVal - m_compval)*conj(newVal - m_compval);
                return;
            case OPERATION_ADDCBSUB:
                m_val += (newVal - m_compval)*conj(newVal - m_compval)*(newVal - m_compval);
                return;
            case OPERATION_ADDSQSQSUB:
                m_val += (newVal - m_compval)*conj(newVal - m_compval)*(newVal - m_compval)*conj(newVal - m_compval);
                return;
            case OPERATION_MAX:
                m_val = bool(newVal > m_val) || mu::isnan(m_val) ? newVal : m_val;
                return;
            case OPERATION_MIN:
                m_val = bool(newVal < m_val) || mu::isnan(m_val) ? newVal : m_val;
                return;
            case OPERATION_NUM:
                m_val += 1.0;
                return;
        }
    }

    /////////////////////////////////////////////////
    /// \brief Combines results from different
    /// StatsLogic instances into a single result.
    ///
    /// \param other const StatsLogic&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void combine(const StatsLogic& other)
    {
        if (mu::isnan(other.m_val))
            return;

        switch (m_type)
        {
            case OPERATION_ADD:
            case OPERATION_NUM:
            case OPERATION_ADDSQ:
            case OPERATION_ADDSQSUB:
            case OPERATION_ADDCBSUB:
            case OPERATION_ADDSQSQSUB:
                m_val += other.m_val;
                return;
            case OPERATION_MULT:
                m_val *= other.m_val;
                return;
            case OPERATION_MAX:
                m_val = bool(other.m_val > m_val) || mu::isnan(m_val) ? other.m_val : m_val;
                return;
            case OPERATION_MIN:
                m_val = bool(other.m_val < m_val) || mu::isnan(m_val) ? other.m_val : m_val;
                return;
        }
    }
};


#endif // STATSLOGIC_HPP

