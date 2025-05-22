/*
                 __________
    _____   __ __\______   \_____  _______  ______  ____ _______
   /     \ |  |  \|     ___/\__  \ \_  __ \/  ___/_/ __ \\_  __ \
  |  Y Y  \|  |  /|    |     / __ \_|  | \/\___ \ \  ___/ |  | \/
  |__|_|  /|____/ |____|    (____  /|__|  /____  > \___  >|__|
        \/                       \/            \/      \/
  Copyright (C) 2011 Ingo Berg

  Permission is hereby granted, free of charge, to any person obtaining a copy of this
  software and associated documentation files (the "Software"), to deal in the Software
  without restriction, including without limitation the rights to use, copy, modify,
  merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "muParserBytecode.h"
#include "muHelpers.hpp"
#include "../utils/tools.hpp"

#include <cassert>
#include <string>
#include <stack>
#include <vector>
#include <iostream>

#include "muParserDef.h"
#include "muParserError.h"
#include "muParserToken.h"
#include "muParserStack.h"
#include "muParserTemplateMagic.h"

namespace mu
{
	//---------------------------------------------------------------------------
	/** \brief Bytecode default constructor. */
	ParserByteCode::ParserByteCode()
		: m_iStackPos(0)
		, m_iMaxStackSize(0)
		, m_vRPN()
		, m_bEnableOptimizer(true)
	{
		m_vRPN.reserve(50);
	}

	//---------------------------------------------------------------------------
	/** \brief Copy constructor.

	    Implemented in Terms of Assign(const ParserByteCode &a_ByteCode)
	*/
	ParserByteCode::ParserByteCode(const ParserByteCode& a_ByteCode)
	{
		Assign(a_ByteCode);
	}

	//---------------------------------------------------------------------------
	/** \brief Assignment operator.

	    Implemented in Terms of Assign(const ParserByteCode &a_ByteCode)
	*/
	ParserByteCode& ParserByteCode::operator=(const ParserByteCode& a_ByteCode)
	{
		Assign(a_ByteCode);
		return *this;
	}

	//---------------------------------------------------------------------------
	void ParserByteCode::EnableOptimizer(bool bStat)
	{
		m_bEnableOptimizer = bStat;
	}

	//---------------------------------------------------------------------------
	/** \brief Copy state of another object to this.

	    \throw nowthrow
	*/
	void ParserByteCode::Assign(const ParserByteCode& a_ByteCode)
	{
		if (this == &a_ByteCode)
			return;

		m_iStackPos = a_ByteCode.m_iStackPos;
		m_vRPN = a_ByteCode.m_vRPN;
		m_iMaxStackSize = a_ByteCode.m_iMaxStackSize;
		m_bEnableOptimizer = a_ByteCode.m_bEnableOptimizer;
	}

	//---------------------------------------------------------------------------
	/** \brief Add a Variable pointer to bytecode.
	    \param a_pVar Pointer to be added.
	    \throw nothrow
	*/
	void ParserByteCode::AddVar(Variable* a_pVar)
	{
		++m_iStackPos;
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		// optimization does not apply
		SToken tok;
		tok.Cmd       = cmVAR;
		tok.m_data = SValData{.var{a_pVar}, .data{Value(1.0)}, .isVect{false}};
		m_vRPN.push_back(tok);
	}

    /////////////////////////////////////////////////
    /// \brief Add a Vararray to the bytecode.
    ///
    /// \param a_varArray const VarArray&
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserByteCode::AddVarArray(const VarArray& a_varArray)
	{
		++m_iStackPos;
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		// optimization does not apply
		SToken tok;
		tok.Cmd       = cmVARARRAY;
		tok.m_data = SOprtData{.var{a_varArray}, .offset{0}};
		m_vRPN.push_back(tok);
	}

	//---------------------------------------------------------------------------
	/** \brief Add a Variable pointer to bytecode.

	    Value entries in byte code consist of:
	    <ul>
	      <li>value array position of the value</li>
	      <li>the operator code according to ParserToken::cmVAL</li>
	      <li>the value stored in #mc_iSizeVal number of bytecode entries.</li>
	    </ul>

	    \param a_pVal Value to be added.
	    \throw nothrow
	*/
	void ParserByteCode::AddVal(const Array& a_fVal)
	{
		++m_iStackPos;
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		// If optimization does not apply
		SToken tok;
		tok.Cmd = cmVAL;
		tok.m_data = SValData{.data{Value(0.0)}, .data2{a_fVal}, .isVect{false}};
		m_vRPN.push_back(tok);
	}

	//---------------------------------------------------------------------------
	void ParserByteCode::ConstantFolding(ECmdCode a_Oprt)
	{
		std::size_t sz = m_vRPN.size();
		Array& x = m_vRPN[sz - 2].Val().data2;
		Array& y = m_vRPN[sz - 1].Val().data2;
		switch (a_Oprt)
		{
			case cmLAND:
				x = x && y;
				m_vRPN.pop_back();
				break;
			case cmLOR:
				x = x || y;
				m_vRPN.pop_back();
				break;
			case cmLT:
				x = x < y;
				m_vRPN.pop_back();
				break;
			case cmGT:
				x = x > y;
				m_vRPN.pop_back();
				break;
			case cmLE:
				x = x <= y;
				m_vRPN.pop_back();
				break;
			case cmGE:
				x = x >= y;
				m_vRPN.pop_back();
				break;
			case cmNEQ:
				x = x != y;
				m_vRPN.pop_back();
				break;
			case cmEQ:
				x = x == y;
				m_vRPN.pop_back();
				break;
			case cmADD:
				x = x + y;
				m_vRPN.pop_back();
				break;
			case cmSUB:
				x = x - y;
				m_vRPN.pop_back();
				break;
			case cmMUL:
				x = x * y;
				m_vRPN.pop_back();
				break;
			case cmDIV:

#if defined(MUP_MATH_EXCEPTIONS)
				if (y == 0)
					throw ParserError(ecDIV_BY_ZERO);
#endif

				x = x / y;
				m_vRPN.pop_back();
				break;

			case cmPOW:
				x = x.pow(y);
				m_vRPN.pop_back();
				break;

            case cmVAL2STR:
                x = val2Str(x, y.front().getNum().asUI64());
                m_vRPN.pop_back();
                break;

			default:
				break;
		} // switch opcode
	}

	//---------------------------------------------------------------------------
	/** \brief Add an operator identifier to bytecode.

	    Operator entries in byte code consist of:
	    <ul>
	      <li>value array position of the result</li>
	      <li>the operator code according to ParserToken::ECmdCode</li>
	    </ul>

	    \sa  ParserToken::ECmdCode
	*/
	void ParserByteCode::AddOp(ECmdCode a_Oprt)
	{
		bool bOptimized = false;

		if (m_bEnableOptimizer)
		{
			std::size_t sz = m_vRPN.size();
            size_t prev = sz-2;
            size_t curr = sz-1;

			// Check for foldable constants like:
			//   cmVAL cmVAL cmADD
			// where cmADD can stand fopr any binary operator applied to
			// two constant values.
			if (sz >= 2 && m_vRPN[prev].Cmd == cmVAL && m_vRPN[curr].Cmd == cmVAL)
			{
				ConstantFolding(a_Oprt);
				bOptimized = true;
			}
			else
			{
				switch (a_Oprt)
				{
					case  cmPOW:
						// Optimization for ploynomials of low order
						if (m_vRPN[prev].Cmd == cmVAR && m_vRPN[curr].Cmd == cmVAL)
						{
							if (all(m_vRPN[curr].Val().data2 == Array(Value(2.0))))
								m_vRPN[prev].Cmd = cmVARPOW2;
							else if (all(m_vRPN[curr].Val().data2 == Array(Value(3.0))))
								m_vRPN[prev].Cmd = cmVARPOW3;
							else if (all(m_vRPN[curr].Val().data2 == Array(Value(4.0))))
								m_vRPN[prev].Cmd = cmVARPOW4;
							else if (all(m_vRPN[curr].Val().data2 == Array(Value(m_vRPN[curr].Val().data2.getAsScalarInt()))))
							{
							    m_vRPN[prev].Cmd = cmVARPOWN;
							    m_vRPN[prev].Val().data = m_vRPN[curr].Val().data2;
							}
                            else
								break;

							m_vRPN.pop_back();
							bOptimized = true;
						}
						break;

					case  cmSUB:
					case  cmADD:
						// Simple optimization based on pattern recognition for a shitload of different
						// bytecode combinations of addition/subtraction
                        if ((m_vRPN[curr].Cmd == cmVAR && m_vRPN[prev].Cmd == cmVAL)
                            || (m_vRPN[curr].Cmd == cmVAL && m_vRPN[prev].Cmd == cmVAR)
                            || (m_vRPN[curr].Cmd == cmVAL && m_vRPN[prev].Cmd == cmVARMUL)
                            || (m_vRPN[curr].Cmd == cmVARMUL && m_vRPN[prev].Cmd == cmVAL)
                            || (m_vRPN[curr].Cmd == cmVAR && m_vRPN[prev].Cmd == cmVAR && m_vRPN[prev].Val().var == m_vRPN[curr].Val().var)
                            || (m_vRPN[curr].Cmd == cmVAR && m_vRPN[prev].Cmd == cmVARMUL && m_vRPN[prev].Val().var == m_vRPN[curr].Val().var)
                            || (m_vRPN[curr].Cmd == cmVARMUL && m_vRPN[prev].Cmd == cmVAR && m_vRPN[prev].Val().var == m_vRPN[curr].Val().var)
                            || (m_vRPN[curr].Cmd == cmVARMUL && m_vRPN[prev].Cmd == cmVARMUL && m_vRPN[prev].Val().var == m_vRPN[curr].Val().var))
						{
                            assert((m_vRPN[prev].Val().var == nullptr && m_vRPN[curr].Val().var != nullptr)
                                   || (m_vRPN[prev].Val().var != nullptr && m_vRPN[curr].Val().var== nullptr)
                                   || (m_vRPN[prev].Val().var == m_vRPN[curr].Val().var));

                            // Some vars might not be commutative
                            if (m_vRPN[prev].Val().var == nullptr
                                && m_vRPN[curr].Val().var != nullptr
                                && !m_vRPN[curr].Val().var->isCommutative())
                                m_vRPN[prev].Cmd = cmREVVARMUL; // Only makes sense, if variables are really not commutative
                            else if (m_vRPN[prev].Cmd == cmVARMUL
                                && m_vRPN[curr].Cmd == cmVAR
                                && !m_vRPN[prev].Val().var->isCommutative())
                                break; // Do not chain operations of variables, which are not commutative
                            else
                                m_vRPN[prev].Cmd = cmVARMUL;

							// Update var pointer
							m_vRPN[prev].Val().var = m_vRPN[prev].Val().var != nullptr ? m_vRPN[prev].Val().var : m_vRPN[curr].Val().var;

							// Ensure type compatibility for offset by avoiding converting void to numerical
							if (m_vRPN[curr].Val().data2.size() && m_vRPN[curr].Val().data2.getCommonType() != TYPE_VOID)
                                m_vRPN[prev].Val().data2 += Array(Value((a_Oprt == cmSUB) ? -1.0 : 1.0)) * m_vRPN[curr].Val().data2;

							// Update scale factor
							m_vRPN[prev].Val().data += Array(Value((a_Oprt == cmSUB) ? -1.0 : 1.0)) * m_vRPN[curr].Val().data;
							m_vRPN[prev].Val().data2.zerosToVoid(); // To convert VARMUL-Offsets to void as neutral object for all operations
							m_vRPN[prev].Val().isVect = false;
							m_vRPN.pop_back();
							bOptimized = true;
						}
						else if (m_vRPN[curr].Cmd == cmDIVVAR && m_vRPN[prev].Cmd == cmVAL)
                        {
                            // 1+2/a+3 -> 2/a+4
                            if (m_vRPN[curr].Val().var != nullptr && !m_vRPN[curr].Val().var->isCommutative())
                                break;

                            m_vRPN[prev].Cmd = cmDIVVAR;
                            m_vRPN[prev].Val().var = m_vRPN[curr].Val().var;
                            m_vRPN[prev].Val().data = Array(Value((a_Oprt == cmSUB) ? -1.0 : 1.0)) * m_vRPN[curr].Val().data;

                            if (m_vRPN[curr].Val().data2.getCommonType() != TYPE_VOID)
                                m_vRPN[prev].Val().data2 += Array(Value((a_Oprt == cmSUB) ? -1.0 : 1.0)) * m_vRPN[curr].Val().data2;

                            m_vRPN.pop_back();
							bOptimized = true;
                        }
                        else if (m_vRPN[curr].Cmd == cmVAL && m_vRPN[prev].Cmd == cmDIVVAR)
                        {
                            // 2/a+3+1 -> 2/a+4
                            m_vRPN[prev].Val().data2 += Array(Value((a_Oprt == cmSUB) ? -1.0 : 1.0)) * m_vRPN[curr].Val().data2;
                            m_vRPN.pop_back();
							bOptimized = true;
                        }
						break;

					case  cmMUL:
                        if ((m_vRPN[curr].Cmd == cmVAR && m_vRPN[prev].Cmd == cmVAL)
                            || (m_vRPN[curr].Cmd == cmVAL && m_vRPN[prev].Cmd == cmVAR) )
						{
							m_vRPN[prev].Cmd = cmVARMUL;
							m_vRPN[prev].Val().var = m_vRPN[prev].Val().var != nullptr ? m_vRPN[prev].Val().var : m_vRPN[curr].Val().var;
							m_vRPN[prev].Val().data = m_vRPN[prev].Val().data2 + m_vRPN[curr].Val().data2;
							m_vRPN[prev].Val().data2 = Array();
							m_vRPN[prev].Val().isVect = false;
							m_vRPN.pop_back();
							bOptimized = true;
						}
                        else if ((m_vRPN[curr].Cmd == cmVAL && m_vRPN[prev].Cmd == cmVARMUL)
                                 || (m_vRPN[curr].Cmd == cmVARMUL && m_vRPN[prev].Cmd == cmVAL))
						{
							// Optimization: 2*(3*b+1) or (3*b+1)*2 -> 6*b+2
							m_vRPN[prev].Cmd = cmVARMUL;
							m_vRPN[prev].Val().var = m_vRPN[prev].Val().var != nullptr ? m_vRPN[prev].Val().var : m_vRPN[curr].Val().var;
							m_vRPN[prev].Val().isVect = false;

							if (m_vRPN[curr].Cmd == cmVAL)
							{
								m_vRPN[prev].Val().data  *= m_vRPN[curr].Val().data2;

								if (!m_vRPN[prev].Val().data2.isDefault())
                                    m_vRPN[prev].Val().data2 *= m_vRPN[curr].Val().data2;
							}
							else
							{
								m_vRPN[prev].Val().data  = m_vRPN[curr].Val().data * m_vRPN[prev].Val().data2;

								if (m_vRPN[curr].Val().data2.isDefault())
                                    m_vRPN[prev].Val().data2 = Array();
                                else
                                    m_vRPN[prev].Val().data2 = m_vRPN[curr].Val().data2 * m_vRPN[prev].Val().data2;
							}
							m_vRPN[prev].Val().data2.zerosToVoid(); // To convert VARMUL-Offsets to void as neutral object for all operations
							m_vRPN.pop_back();
							bOptimized = true;
						}
                        else if (m_vRPN[curr].Cmd == cmVAR
                                 && m_vRPN[prev].Cmd == cmVAR
                                 && m_vRPN[curr].Val().var == m_vRPN[prev].Val().var)
						{
							// Optimization: a*a -> a^2
							m_vRPN[prev].Cmd = cmVARPOW2;
							m_vRPN.pop_back();
							bOptimized = true;
						}
                        else if (m_vRPN[curr].Cmd == cmDIVVAR && m_vRPN[prev].Cmd == cmVAL)
						{
							// Optimization: 4*(2/a+3) -> 8/a+12
							m_vRPN[prev].Cmd = cmDIVVAR;
                            m_vRPN[prev].Val().var = m_vRPN[curr].Val().var;
                            m_vRPN[prev].Val().data = m_vRPN[prev].Val().data2 * m_vRPN[curr].Val().data;
							m_vRPN[prev].Val().data2 *= m_vRPN[curr].Val().data2;
							m_vRPN.pop_back();
							bOptimized = true;
						}
                        else if (m_vRPN[prev].Cmd == cmDIVVAR && m_vRPN[curr].Cmd == cmVAL)
						{
							// Optimization: (2/a+3)*4 -> 8/a+12
							m_vRPN[prev].Val().data *= m_vRPN[curr].Val().data2;
							m_vRPN[prev].Val().data2 *= m_vRPN[curr].Val().data2;
							m_vRPN.pop_back();
							bOptimized = true;
						}
						break;

					case cmDIV:
                        if (m_vRPN[curr].Cmd == cmVAL
                            && m_vRPN[prev].Cmd == cmVARMUL
                            && all(m_vRPN[curr].Val().data2 != Array(Value(0.0))))
						{
							// Optimization: 4*a/2 -> 2*a
							m_vRPN[prev].Val().data  /= m_vRPN[curr].Val().data2;

							// Added to avoid problems with the optimisation
							if (m_vRPN[prev].Val().data2.getCommonType() != TYPE_VOID)
                                m_vRPN[prev].Val().data2 /= m_vRPN[curr].Val().data2;

							m_vRPN.pop_back();
							bOptimized = true;
						}
                        else if (m_vRPN[curr].Cmd == cmVAL
                                 && m_vRPN[prev].Cmd == cmVAR)
						{
							// Optimization: 4*a/2 -> 2*a
							m_vRPN[prev].Val().data /= m_vRPN[curr].Val().data2;
							m_vRPN[prev].Val().data2 = Array();
							m_vRPN[prev].Val().isVect = false;
                            m_vRPN[prev].Cmd = cmVARMUL;

							m_vRPN.pop_back();
							bOptimized = true;
						}
                        else if (m_vRPN[curr].Cmd == cmVAL
                                 && m_vRPN[prev].Cmd == cmDIVVAR)
						{
							// Optimization: (4/a+3)/2 -> 2/a+3/2
							m_vRPN[prev].Val().data /= m_vRPN[curr].Val().data2;
							m_vRPN[prev].Val().data2 /= m_vRPN[curr].Val().data2;

							m_vRPN.pop_back();
							bOptimized = true;
						}
                        else if (m_vRPN[prev].Cmd == cmVAL
                                 && m_vRPN[curr].Cmd == cmVAR)
						{
							// Optimization: 4*a/2 -> 2*a
							m_vRPN[prev].Val().data = m_vRPN[prev].Val().data2;
							m_vRPN[prev].Val().var = m_vRPN[curr].Val().var;
							m_vRPN[prev].Val().data2 = Array();
                            m_vRPN[prev].Cmd = cmDIVVAR;

							m_vRPN.pop_back();
							bOptimized = true;
						}
                        else if (m_vRPN[prev].Cmd == cmVAL
                                 && m_vRPN[curr].Cmd == cmVARMUL
                                 && m_vRPN[curr].Val().data2.getCommonType() == TYPE_VOID)
						{
							// Optimization: 4*a/2 -> 2*a
							m_vRPN[prev].Val().data = m_vRPN[prev].Val().data2 / m_vRPN[curr].Val().data;
							m_vRPN[prev].Val().var = m_vRPN[curr].Val().var;
							m_vRPN[prev].Val().data2 = Array();
                            m_vRPN[prev].Cmd = cmDIVVAR;

							m_vRPN.pop_back();
							bOptimized = true;
						}
						break;

                    default:
                        break;
                        // Nothing, just avoiding warnings

				} // switch a_Oprt
			}
		}

		// If optimization can't be applied just write the value
		if (!bOptimized)
		{
			--m_iStackPos;
			SToken tok;
			tok.Cmd = a_Oprt;
			m_vRPN.push_back(tok);
		}
	}

	//---------------------------------------------------------------------------
	void ParserByteCode::AddIfElse(ECmdCode a_Oprt)
	{
		SToken tok;
		tok.Cmd = a_Oprt;
		tok.m_data = SOprtData{.offset{0}};
		m_vRPN.push_back(tok);
	}

	//---------------------------------------------------------------------------
	/** \brief Add an assignement operator

	    Operator entries in byte code consist of:
	    <ul>
	      <li>cmASSIGN code</li>
	      <li>the pointer of the destination variable</li>
	    </ul>

	    \sa  ParserToken::ECmdCode
	*/
	void ParserByteCode::AddAssignOp(Variable* a_pVar, ECmdCode assignmentCode)
	{
		--m_iStackPos;

		SToken tok;
		tok.Cmd = assignmentCode;
		tok.m_data = SOprtData{.var{a_pVar}, .offset{0}};
		m_vRPN.push_back(tok);
	}


    /////////////////////////////////////////////////
    /// \brief Add an assignment operator together
    /// with its associated VarArray instance.
    ///
    /// \param a_varArray const VarArray&
    /// \param assignmentCode ECmdCode
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserByteCode::AddAssignOp(const VarArray& a_varArray, ECmdCode assignmentCode)
	{
		--m_iStackPos;

		SToken tok;
		tok.Cmd = assignmentCode;
		tok.m_data = SOprtData{.var{a_varArray}, .offset{0}};
		m_vRPN.push_back(tok);
	}


    /////////////////////////////////////////////////
    /// \brief Add a function to the bytecode.
    ///
    /// \param a_pFun generic_fun_type
    /// \param a_iArgc int
    /// \param optimizeAway bool
    /// \param funcName const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserByteCode::AddFun(generic_fun_type a_pFun, int a_iArgc, bool optimizeAway, const std::string& funcName)
	{
	    // Shall we try to optimize?
		if (m_bEnableOptimizer && optimizeAway)
		{
			std::size_t sz = m_vRPN.size();
			std::size_t nArg = std::abs(a_iArgc);

			// Ensure that we have enough entries in the
			// bytecode available
			if (sz < nArg)
                throw ParserError(ecINTERNAL_ERROR);

            // Check, whether all arguments are constant
            // values
            for (size_t s = sz-nArg; s < sz; s++)
            {
                // If not a constant value, deactivate the optimizer flag
                if (m_vRPN[s].Cmd != cmVAL)
                {
                    optimizeAway = false;
                    break;
                }
            }

            // Can we still optimize?
            if (optimizeAway)
            {
                // switch according to argument count. Call the corresponding
                // function and replace the first value with the result. Remove
                // all other values afterwards
                switch (a_iArgc)
                {
                    case 0:
                        if (funcName == MU_VECTOR_CREATE)
                            AddVal((*(multfun_type)a_pFun)(nullptr, 0));
                        else
                            AddVal((*(fun_type0)a_pFun)());
                        break;
                    case 1:
                        m_vRPN[sz-1].Val().data2 = (*(fun_type1)a_pFun)(m_vRPN[sz-1].Val().data2);
                        break;
                    case 2:
                        m_vRPN[sz-2].Val().data2 = (*(fun_type2)a_pFun)(m_vRPN[sz-2].Val().data2,
                                                                        m_vRPN[sz-1].Val().data2);
                        m_vRPN.pop_back();
                        break;
                    case 3:
                        m_vRPN[sz-3].Val().data2 = (*(fun_type3)a_pFun)(m_vRPN[sz-3].Val().data2,
                                                                        m_vRPN[sz-2].Val().data2,
                                                                        m_vRPN[sz-1].Val().data2);
                        m_vRPN.resize(sz-2);
                        break;
                    case 4:
                        m_vRPN[sz-4].Val().data2 = (*(fun_type4)a_pFun)(m_vRPN[sz-4].Val().data2,
                                                                        m_vRPN[sz-3].Val().data2,
                                                                        m_vRPN[sz-2].Val().data2,
                                                                        m_vRPN[sz-1].Val().data2);
                        m_vRPN.resize(sz-3);
                        break;
                    case 5:
                        m_vRPN[sz-5].Val().data2 = (*(fun_type5)a_pFun)(m_vRPN[sz-5].Val().data2,
                                                                        m_vRPN[sz-4].Val().data2,
                                                                        m_vRPN[sz-3].Val().data2,
                                                                        m_vRPN[sz-2].Val().data2,
                                                                        m_vRPN[sz-1].Val().data2);
                        m_vRPN.resize(sz-4);
                        break;
                    case 6:
                        m_vRPN[sz-6].Val().data2 = (*(fun_type6)a_pFun)(m_vRPN[sz-6].Val().data2,
                                                                        m_vRPN[sz-5].Val().data2,
                                                                        m_vRPN[sz-4].Val().data2,
                                                                        m_vRPN[sz-3].Val().data2,
                                                                        m_vRPN[sz-2].Val().data2,
                                                                        m_vRPN[sz-1].Val().data2);
                        m_vRPN.resize(sz-5);
                        break;
                    case 7:
                        m_vRPN[sz-7].Val().data2 = (*(fun_type7)a_pFun)(m_vRPN[sz-7].Val().data2,
                                                                        m_vRPN[sz-6].Val().data2,
                                                                        m_vRPN[sz-5].Val().data2,
                                                                        m_vRPN[sz-4].Val().data2,
                                                                        m_vRPN[sz-3].Val().data2,
                                                                        m_vRPN[sz-2].Val().data2,
                                                                        m_vRPN[sz-1].Val().data2);
                        m_vRPN.resize(sz-6);
                        break;
                    case 8:
                        m_vRPN[sz-8].Val().data2 = (*(fun_type8)a_pFun)(m_vRPN[sz-8].Val().data2,
                                                                        m_vRPN[sz-7].Val().data2,
                                                                        m_vRPN[sz-6].Val().data2,
                                                                        m_vRPN[sz-5].Val().data2,
                                                                        m_vRPN[sz-4].Val().data2,
                                                                        m_vRPN[sz-3].Val().data2,
                                                                        m_vRPN[sz-2].Val().data2,
                                                                        m_vRPN[sz-1].Val().data2);
                        m_vRPN.resize(sz-7);
                        break;
                    case 9:
                        m_vRPN[sz-9].Val().data2 = (*(fun_type9)a_pFun)(m_vRPN[sz-9].Val().data2,
                                                                        m_vRPN[sz-8].Val().data2,
                                                                        m_vRPN[sz-7].Val().data2,
                                                                        m_vRPN[sz-6].Val().data2,
                                                                        m_vRPN[sz-5].Val().data2,
                                                                        m_vRPN[sz-4].Val().data2,
                                                                        m_vRPN[sz-3].Val().data2,
                                                                        m_vRPN[sz-2].Val().data2,
                                                                        m_vRPN[sz-1].Val().data2);
                        m_vRPN.resize(sz-8);
                        break;
                    case 10:
                        m_vRPN[sz-10].Val().data2 = (*(fun_type10)a_pFun)(m_vRPN[sz-10].Val().data2,
                                                                          m_vRPN[sz-9].Val().data2,
                                                                          m_vRPN[sz-8].Val().data2,
                                                                          m_vRPN[sz-7].Val().data2,
                                                                          m_vRPN[sz-6].Val().data2,
                                                                          m_vRPN[sz-5].Val().data2,
                                                                          m_vRPN[sz-4].Val().data2,
                                                                          m_vRPN[sz-3].Val().data2,
                                                                          m_vRPN[sz-2].Val().data2,
                                                                          m_vRPN[sz-1].Val().data2);
                        m_vRPN.resize(sz-9);
                        break;
                    default:
                    {
                        if (a_iArgc > 0) // function with variable arguments store the number as a negative value
                            throw ParserError(ecINTERNAL_ERROR);

                        std::vector<Array> vStack;

                        // Copy the values to the temporary stack
                        for (size_t s = sz-nArg; s < sz; s++)
                        {
                            vStack.push_back(m_vRPN[s].Val().data2);
                        }

                        m_vRPN[sz-nArg].Val().data2 = (*(multfun_type)a_pFun)(&vStack[0], nArg);
                        m_vRPN.resize(sz-nArg+1);
                    }
                }
            }
            else if (a_iArgc == 1 && funcName == "-")
            {
                // There is the special combination of cmVAR/cmVARMUL, cmFUNC with FUNC == "-"
                // This can be optimized to cmVARMUL
                if (m_vRPN[sz-1].Cmd == cmVAR)
                {
                    optimizeAway = true;
                    m_vRPN[sz-1].Cmd = cmVARMUL;
                    m_vRPN[sz-1].Val().data = Value(-1.0);
                    m_vRPN[sz-1].Val().data2 = Array();
                    m_vRPN[sz-1].Val().isVect = false;
                }
                else if (m_vRPN[sz-1].Cmd == cmVARMUL || m_vRPN[sz-1].Cmd == cmREVVARMUL)
                {
                    optimizeAway = true;
                    m_vRPN[sz-1].Val().data *= Value(-1.0);
                    m_vRPN[sz-1].Val().data2 *= Value(-1.0);
                }
            }
		}

		// If optimization can't be applied just add the call to the function
		if (!optimizeAway)
		{
            if (a_iArgc >= 0)
                m_iStackPos = m_iStackPos - a_iArgc + 1;
            else
                // function with unlimited number of arguments
                m_iStackPos = m_iStackPos + a_iArgc + 1;

            m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

            SToken tok;
            tok.Cmd = cmFUNC;
            tok.m_data = SFunData{.ptr{a_pFun}, .name{funcName}, .argc{a_iArgc}, .idx{0}};
            m_vRPN.push_back(tok);
		}
	}

    /////////////////////////////////////////////////
    /// \brief Add a method to the bytecode.
    ///
    /// \param a_method const std::string&
    /// \param a_iArgc int
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserByteCode::AddMethod(const std::string& a_method, int a_iArgc)
	{
	    if (a_iArgc >= 0)
            m_iStackPos = m_iStackPos - a_iArgc + 1;

        m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

        SToken tok;
        tok.Cmd = cmMETHOD;
        tok.m_data = SFunData{.name{a_method}, .argc{a_iArgc}, .idx{0}};
        m_vRPN.push_back(tok);
	}

    /////////////////////////////////////////////////
    /// \brief Remove the topmost element from the
    /// bytecode.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserByteCode::pop()
	{
	    if (m_vRPN.size())
            m_vRPN.pop_back();
	}

	//---------------------------------------------------------------------------
	/** \brief Add end marker to bytecode.

	    \throw nothrow
	*/
	void ParserByteCode::Finalize()
	{
		SToken tok;
		tok.Cmd = cmEND;
		m_vRPN.push_back(tok);
		rpn_type(m_vRPN).swap(m_vRPN);     // shrink bytecode vector to fit

		// Determine the if-then-else jump offsets
		ParserStack<int> stIf, stElse;
		int idx;
		for (int i = 0; i < (int)m_vRPN.size(); ++i)
		{
			switch (m_vRPN[i].Cmd)
			{
				case cmIF:
					stIf.push(i);
					break;

				case  cmELSE:
					stElse.push(i);
					idx = stIf.pop();
					m_vRPN[idx].Oprt().offset = i - idx;
					break;

				case cmENDIF:
					idx = stElse.pop();
					m_vRPN[idx].Oprt().offset = i - idx;
					break;

				default:
					break;
			}
		}
	}


    /////////////////////////////////////////////////
    /// \brief Changes all old variable pointers to
    /// the new addresses. Will be used to compensate
    /// for different local variable adresses in
    /// different scopes.
    ///
    /// \param a_pOldVar value_type*
    /// \param a_pNewVar value_type*
    /// \param isVect bool
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserByteCode::ChangeVar(Variable* a_pOldVar, Variable* a_pNewVar, bool isVect)
	{
	    for (size_t i = 0; i < m_vRPN.size(); ++i)
        {
            if (m_vRPN[i].Cmd >= cmVAR && m_vRPN[i].Cmd < cmVAR_END && m_vRPN[i].Val().var == a_pOldVar)
            {
                m_vRPN[i].Val().var = a_pNewVar;
                m_vRPN[i].Val().isVect = isVect;
            }
        }
	}


	//---------------------------------------------------------------------------
	SToken* ParserByteCode::GetBase()
	{
		if (m_vRPN.size() == 0)
			throw ParserError(ecINTERNAL_ERROR);
		else
			return &m_vRPN[0];
	}

	//---------------------------------------------------------------------------
	std::size_t ParserByteCode::GetMaxStackSize() const
	{
		return m_iMaxStackSize + 1;
	}

	//---------------------------------------------------------------------------
	/** \brief Returns the number of entries in the bytecode. */
	std::size_t ParserByteCode::GetSize() const
	{
		return m_vRPN.size();
	}

	//---------------------------------------------------------------------------
	/** \brief Delete the bytecode.

	    \throw nothrow

	    The name of this function is a violation of my own coding guidelines
	    but this way it's more in line with the STL functions thus more
	    intuitive.
	*/
	void ParserByteCode::clear()
	{
		m_vRPN.clear();
		m_iStackPos = 0;
		m_iMaxStackSize = 0;
	}

	//---------------------------------------------------------------------------
	/** \brief Dump bytecode (for debugging only!). */
	void ParserByteCode::AsciiDump()
	{
		if (!m_vRPN.size())
		{
			print("No bytecode available");
			return;
		}

		toggleTableMode();
		print("Stack size: " + toString(m_vRPN.size()));

		for (std::size_t i = 0; i < m_vRPN.size() && m_vRPN[i].Cmd != cmEND; ++i)
		{
		    printFormatted("|   " + toString(i) + " : \t");
			switch (m_vRPN[i].Cmd)
			{
				case cmVAL:
				    printFormatted("VAL       \t<" + m_vRPN[i].Val().data2.print() + ">\n");
					break;

				case cmVAR:
				    printFormatted("VAR       \t[" + m_vRPN[i].Val().var->print() + "]\n");
					break;

				case cmVARARRAY:
				    printFormatted("VARARR    \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				case cmVARPOW2:
				    printFormatted("VARPOW2   \t[" + m_vRPN[i].Val().var->print() + "]\n");
					break;

				case cmVARPOW3:
					printFormatted("VARPOW3   \t[" + m_vRPN[i].Val().var->print() + "]\n");
					break;

				case cmVARPOW4:
					printFormatted("VARPOW4   \t[" + m_vRPN[i].Val().var->print() + "]\n");
					break;

				case cmVARPOWN:
					printFormatted("VARPOWN   \t[" + m_vRPN[i].Val().var->print() + "] ^ <" + m_vRPN[i].Val().data.print() + ">\n");
					break;

				case cmVARMUL:
					printFormatted("VARMUL    \t[" + m_vRPN[i].Val().var->print() + "]");
					printFormatted(" * <" + m_vRPN[i].Val().data.print() + "> + <" + m_vRPN[i].Val().data2.print() + ">\n");
					break;

				case cmREVVARMUL:
					printFormatted("REVVARMUL \t<" + m_vRPN[i].Val().data2.print() + ">");
					printFormatted(" + [" + m_vRPN[i].Val().var->print() + "] * <" + m_vRPN[i].Val().data.print() + ">\n");
					break;

				case cmDIVVAR:
					printFormatted("DIVVAR    \t<" + m_vRPN[i].Val().data.print() + ">");
					printFormatted(" / [" + m_vRPN[i].Val().var->print() + "] + <" + m_vRPN[i].Val().data2.print() + ">\n");
					break;

				case cmFUNC:
				    printFormatted("CALL      \t[ARG: " + toString(m_vRPN[i].Fun().argc) + "] [FUNC: " + m_vRPN[i].Fun().name + "] [ADDR: " + toHexString((size_t)m_vRPN[i].Fun().ptr) + "]\n");
					break;

				case cmMETHOD:
				    printFormatted("CALL      \t[ARG: " + toString(m_vRPN[i].Fun().argc) + "] [METHOD: " + m_vRPN[i].Fun().name + "]\n");
					break;

				case cmLT:
				    printFormatted("LT\n");
					break;
				case cmGT:
					printFormatted("GT\n");
					break;
				case cmLE:
					printFormatted("LE\n");
					break;
				case cmGE:
					printFormatted("GE\n");
					break;
				case cmEQ:
					printFormatted("EQ\n");
					break;
				case cmNEQ:
					printFormatted("NEQ\n");
					break;
				case cmADD:
					printFormatted("ADD\n");
					break;
				case cmLAND:
					printFormatted("AND\n");
					break;
				case cmLOR:
					printFormatted("OR\n");
					break;
				case cmSUB:
					printFormatted("SUB\n");
					break;
				case cmMUL:
					printFormatted("MUL\n");
					break;
				case cmDIV:
					printFormatted("DIV\n");
					break;
				case cmPOW:
					printFormatted("POW\n");
					break;
				case cmVAL2STR:
					printFormatted("VAL2STR\n");
					break;

				case cmIF:
				    printFormatted("IF        \t[OFFSET: " + toString(m_vRPN[i].Oprt().offset) + "]\n");
					break;

				case cmELSE:
					printFormatted("ELSE      \t[OFFSET: " + toString(m_vRPN[i].Oprt().offset) + "]\n");
					break;

				case cmENDIF:
					printFormatted("ENDIF\n");
					break;

				case cmASSIGN:
					printFormatted("ASSIGN    \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				case cmADDASGN:
					printFormatted("ADDASGN   \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				case cmSUBASGN:
					printFormatted("SUBASGN   \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				case cmMULASGN:
					printFormatted("MULASGN   \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				case cmDIVASGN:
					printFormatted("DIVASGN   \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				case cmPOWASGN:
					printFormatted("POWASGN   \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				case cmINCR:
					printFormatted("INCR      \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				case cmDECR:
					printFormatted("DECR      \t[" + m_vRPN[i].Oprt().var.print() + "]\n");
					break;

				default:
				    printFormatted("unknown   \t(" + toString((int)m_vRPN[i].Cmd) + ")\n");
					break;
			} // switch cmdCode
		} // while bytecode

		toggleTableMode();
		printFormatted("|   END\n");
	}
} // namespace mu
