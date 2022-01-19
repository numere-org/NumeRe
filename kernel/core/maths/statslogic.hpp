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
        OPERATION_MAX,
        OPERATION_MIN,
        OPERATION_NUM
    };

    mu::value_type m_val;
    mu::value_type m_compval;
    OperationType m_type;

    StatsLogic(OperationType type, double baseVal = 0.0, mu::value_type compVal = 0.0)
        : m_val(baseVal), m_compval(compVal), m_type(type) {}

    /////////////////////////////////////////////////
    /// \brief Implements the statistics logic.
    ///
    /// \param newVal const mu::value_type&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void operator()(const mu::value_type& newVal)
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
                m_val += (newVal - m_compval)*std::conj(newVal - m_compval);
                return;
            case OPERATION_MAX:
                m_val = newVal.real() > m_val.real() || isnan(m_val.real()) ? newVal.real() : m_val.real();
                return;
            case OPERATION_MIN:
                m_val = newVal.real() < m_val.real() || isnan(m_val.real()) ? newVal.real() : m_val.real();
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
                m_val += other.m_val;
                return;
            case OPERATION_MULT:
                m_val *= other.m_val;
                return;
            case OPERATION_MAX:
                m_val = other.m_val.real() > m_val.real() || isnan(m_val.real()) ? other.m_val.real() : m_val.real();
                return;
            case OPERATION_MIN:
                m_val = other.m_val.real() < m_val.real() || isnan(m_val.real()) ? other.m_val.real() : m_val.real();
                return;
        }
    }
};


#endif // STATSLOGIC_HPP

