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
#include "../../kernel.hpp"

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

std::string toString(unsigned int);
std::string toHexString(int nNumber);

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
	void ParserByteCode::AddVar(value_type* a_pVar)
	{
		++m_iStackPos;
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		// optimization does not apply
		SToken tok;
		tok.Cmd       = cmVAR;
		tok.Val.ptr   = a_pVar;
		tok.Val.data  = 1;
		tok.Val.data2 = 0;
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
	void ParserByteCode::AddVal(value_type a_fVal)
	{
		++m_iStackPos;
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		// If optimization does not apply
		SToken tok;
		tok.Cmd = cmVAL;
		tok.Val.ptr   = NULL;
		tok.Val.data  = 0;
		tok.Val.data2 = a_fVal;
		m_vRPN.push_back(tok);
	}

	//---------------------------------------------------------------------------
	void ParserByteCode::ConstantFolding(ECmdCode a_Oprt)
	{
		std::size_t sz = m_vRPN.size();
		value_type& x = m_vRPN[sz - 2].Val.data2,
					&y = m_vRPN[sz - 1].Val.data2;
		switch (a_Oprt)
		{
			case cmLAND:
				x = (int)x && (int)y;
				m_vRPN.pop_back();
				break;
			case cmLOR:
				x = (int)x || (int)y;
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
				x = MathImpl<value_type>::Pow(x, y);
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

			// Check for foldable constants like:
			//   cmVAL cmVAL cmADD
			// where cmADD can stand fopr any binary operator applied to
			// two constant values.
			if (sz >= 2 && m_vRPN[sz - 2].Cmd == cmVAL && m_vRPN[sz - 1].Cmd == cmVAL)
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
						if (m_vRPN[sz - 2].Cmd == cmVAR && m_vRPN[sz - 1].Cmd == cmVAL)
						{
							if (m_vRPN[sz - 1].Val.data2 == 2)
								m_vRPN[sz - 2].Cmd = cmVARPOW2;
							else if (m_vRPN[sz - 1].Val.data2 == 3)
								m_vRPN[sz - 2].Cmd = cmVARPOW3;
							else if (m_vRPN[sz - 1].Val.data2 == 4)
								m_vRPN[sz - 2].Cmd = cmVARPOW4;
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
						if ( (m_vRPN[sz - 1].Cmd == cmVAR    && m_vRPN[sz - 2].Cmd == cmVAL)    ||
								(m_vRPN[sz - 1].Cmd == cmVAL    && m_vRPN[sz - 2].Cmd == cmVAR)    ||
								(m_vRPN[sz - 1].Cmd == cmVAL    && m_vRPN[sz - 2].Cmd == cmVARMUL) ||
								(m_vRPN[sz - 1].Cmd == cmVARMUL && m_vRPN[sz - 2].Cmd == cmVAL)    ||
								(m_vRPN[sz - 1].Cmd == cmVAR    && m_vRPN[sz - 2].Cmd == cmVAR    && m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr) ||
								(m_vRPN[sz - 1].Cmd == cmVAR    && m_vRPN[sz - 2].Cmd == cmVARMUL && m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr) ||
								(m_vRPN[sz - 1].Cmd == cmVARMUL && m_vRPN[sz - 2].Cmd == cmVAR    && m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr) ||
								(m_vRPN[sz - 1].Cmd == cmVARMUL && m_vRPN[sz - 2].Cmd == cmVARMUL && m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr) )
						{
							assert( (m_vRPN[sz - 2].Val.ptr == NULL && m_vRPN[sz - 1].Val.ptr != NULL) ||
									(m_vRPN[sz - 2].Val.ptr != NULL && m_vRPN[sz - 1].Val.ptr == NULL) ||
									(m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr) );

							m_vRPN[sz - 2].Cmd = cmVARMUL;
							m_vRPN[sz - 2].Val.ptr    = (value_type*)((long long)(m_vRPN[sz - 2].Val.ptr) | (long long)(m_vRPN[sz - 1].Val.ptr)); // variable
							m_vRPN[sz - 2].Val.data2 += ((a_Oprt == cmSUB) ? -1 : 1) * m_vRPN[sz - 1].Val.data2; // offset
							m_vRPN[sz - 2].Val.data  += ((a_Oprt == cmSUB) ? -1 : 1) * m_vRPN[sz - 1].Val.data; // multiplikatior
							m_vRPN.pop_back();
							bOptimized = true;
						}
						break;

					case  cmMUL:
						if ( (m_vRPN[sz - 1].Cmd == cmVAR && m_vRPN[sz - 2].Cmd == cmVAL) ||
								(m_vRPN[sz - 1].Cmd == cmVAL && m_vRPN[sz - 2].Cmd == cmVAR) )
						{
							m_vRPN[sz - 2].Cmd        = cmVARMUL;
							m_vRPN[sz - 2].Val.ptr    = (value_type*)((long long)(m_vRPN[sz - 2].Val.ptr) | (long long)(m_vRPN[sz - 1].Val.ptr));
							m_vRPN[sz - 2].Val.data   = m_vRPN[sz - 2].Val.data2 + m_vRPN[sz - 1].Val.data2;
							m_vRPN[sz - 2].Val.data2  = 0;
							m_vRPN.pop_back();
							bOptimized = true;
						}
						else if ( (m_vRPN[sz - 1].Cmd == cmVAL    && m_vRPN[sz - 2].Cmd == cmVARMUL) ||
								  (m_vRPN[sz - 1].Cmd == cmVARMUL && m_vRPN[sz - 2].Cmd == cmVAL) )
						{
							// Optimization: 2*(3*b+1) or (3*b+1)*2 -> 6*b+2
							m_vRPN[sz - 2].Cmd     = cmVARMUL;
							m_vRPN[sz - 2].Val.ptr = (value_type*)((long long)(m_vRPN[sz - 2].Val.ptr) | (long long)(m_vRPN[sz - 1].Val.ptr));
							if (m_vRPN[sz - 1].Cmd == cmVAL)
							{
								m_vRPN[sz - 2].Val.data  *= m_vRPN[sz - 1].Val.data2;
								m_vRPN[sz - 2].Val.data2 *= m_vRPN[sz - 1].Val.data2;
							}
							else
							{
								m_vRPN[sz - 2].Val.data  = m_vRPN[sz - 1].Val.data  * m_vRPN[sz - 2].Val.data2;
								m_vRPN[sz - 2].Val.data2 = m_vRPN[sz - 1].Val.data2 * m_vRPN[sz - 2].Val.data2;
							}
							m_vRPN.pop_back();
							bOptimized = true;
						}
						else if (m_vRPN[sz - 1].Cmd == cmVAR && m_vRPN[sz - 2].Cmd == cmVAR &&
								 m_vRPN[sz - 1].Val.ptr == m_vRPN[sz - 2].Val.ptr)
						{
							// Optimization: a*a -> a^2
							m_vRPN[sz - 2].Cmd = cmVARPOW2;
							m_vRPN.pop_back();
							bOptimized = true;
						}
						break;

					case cmDIV:
						if (m_vRPN[sz - 1].Cmd == cmVAL && m_vRPN[sz - 2].Cmd == cmVARMUL && m_vRPN[sz - 1].Val.data2 != 0)
						{
							// Optimization: 4*a/2 -> 2*a
							m_vRPN[sz - 2].Val.data  /= m_vRPN[sz - 1].Val.data2;
							m_vRPN[sz - 2].Val.data2 /= m_vRPN[sz - 1].Val.data2;
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
	void ParserByteCode::AddAssignOp(value_type* a_pVar)
	{
		--m_iStackPos;

		SToken tok;
		tok.Cmd = cmASSIGN;
		tok.Val.ptr = a_pVar;
		m_vRPN.push_back(tok);
	}

	//---------------------------------------------------------------------------
	/** \brief Add function to bytecode.

	    \param a_iArgc Number of arguments, negative numbers indicate multiarg functions.
	    \param a_pFun Pointer to function callback.
	*/
	void ParserByteCode::AddFun(generic_fun_type a_pFun, int a_iArgc)
	{
		if (a_iArgc >= 0)
		{
			m_iStackPos = m_iStackPos - a_iArgc + 1;
		}
		else
		{
			// function with unlimited number of arguments
			m_iStackPos = m_iStackPos + a_iArgc + 1;
		}
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		SToken tok;
		tok.Cmd = cmFUNC;
		tok.Fun.argc = a_iArgc;
		tok.Fun.ptr = a_pFun;
		m_vRPN.push_back(tok);
	}

	//---------------------------------------------------------------------------
	/** \brief Add a bulk function to bytecode.

	    \param a_iArgc Number of arguments, negative numbers indicate multiarg functions.
	    \param a_pFun Pointer to function callback.
	*/
	void ParserByteCode::AddBulkFun(generic_fun_type a_pFun, int a_iArgc)
	{
		m_iStackPos = m_iStackPos - a_iArgc + 1;
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		SToken tok;
		tok.Cmd = cmFUNC_BULK;
		tok.Fun.argc = a_iArgc;
		tok.Fun.ptr = a_pFun;
		m_vRPN.push_back(tok);
	}

	//---------------------------------------------------------------------------
	/** \brief Add Strung function entry to the parser bytecode.
	    \throw nothrow

	    A string function entry consists of the stack position of the return value,
	    followed by a cmSTRFUNC code, the function pointer and an index into the
	    string buffer maintained by the parser.
	*/
	void ParserByteCode::AddStrFun(generic_fun_type a_pFun, int a_iArgc, int a_iIdx)
	{
		m_iStackPos = m_iStackPos - a_iArgc + 1;

		SToken tok;
		tok.Cmd = cmFUNC_STR;
		tok.Fun.argc = a_iArgc;
		tok.Fun.idx = a_iIdx;
		tok.Fun.ptr = a_pFun;
		m_vRPN.push_back(tok);

		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);
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
					m_vRPN[idx].Oprt.offset = i - idx;
					break;

				case cmENDIF:
					idx = stElse.pop();
					m_vRPN[idx].Oprt.offset = i - idx;
					break;

				default:
					break;
			}
		}
	}

	//---------------------------------------------------------------------------
	const SToken* ParserByteCode::GetBase() const
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
			NumeReKernel::print("No bytecode available");
			return;
		}

		NumeReKernel::print("Number of RPN tokens: " + toString(m_vRPN.size()));
		for (std::size_t i = 0; i < m_vRPN.size() && m_vRPN[i].Cmd != cmEND; ++i)
		{
		    NumeReKernel::printPreFmt("|   " + toString(i) + " : \t");
			switch (m_vRPN[i].Cmd)
			{
				case cmVAL:
				    NumeReKernel::printPreFmt("VAL \t[" + toString(m_vRPN[i].Val.data2, 7) + "]\n");
					break;

				case cmVAR:
				    NumeReKernel::printPreFmt("VAR \t[" + toHexString((int)m_vRPN[i].Val.ptr) + "]\n");
					break;

				case cmVARPOW2:
				    NumeReKernel::printPreFmt("VARPOW2 \t[" + toHexString((int)m_vRPN[i].Val.ptr) + "]\n");
					break;

				case cmVARPOW3:
					NumeReKernel::printPreFmt("VARPOW3 \t[" + toHexString((int)m_vRPN[i].Val.ptr) + "]\n");
					break;

				case cmVARPOW4:
					NumeReKernel::printPreFmt("VARPOW4 \t[" + toHexString((int)m_vRPN[i].Val.ptr) + "]\n");
					break;

				case cmVARMUL:
					NumeReKernel::printPreFmt("VARMUL \t[" + toHexString((int)m_vRPN[i].Val.ptr) + "]");
					NumeReKernel::printPreFmt(" * [" + toString(m_vRPN[i].Val.data, 7) + "] + [" + toString(m_vRPN[i].Val.data2, 7) + "]\n");
					break;

				case cmFUNC:
				    NumeReKernel::printPreFmt("CALL \t[ARG: " + toString(m_vRPN[i].Fun.argc) + "] [ADDR: " + toHexString((int)m_vRPN[i].Fun.ptr) + "]\n");
					break;

				case cmFUNC_STR:
				    NumeReKernel::printPreFmt("CALL STRFUNC\t[ARG:" + toString(m_vRPN[i].Fun.argc) + "] [IDX: "+ toString(m_vRPN[i].Fun.idx) + "] [ADDR: " + toHexString((int)m_vRPN[i].Fun.ptr) + "]\n");
					break;

				case cmLT:
				    NumeReKernel::printPreFmt("LT\n");
					break;
				case cmGT:
					NumeReKernel::printPreFmt("GT\n");
					break;
				case cmLE:
					NumeReKernel::printPreFmt("LE\n");
					break;
				case cmGE:
					NumeReKernel::printPreFmt("GE\n");
					break;
				case cmEQ:
					NumeReKernel::printPreFmt("EQ\n");
					break;
				case cmNEQ:
					NumeReKernel::printPreFmt("NEQ\n");
					break;
				case cmADD:
					NumeReKernel::printPreFmt("ADD\n");
					break;
				case cmLAND:
					NumeReKernel::printPreFmt("AND\n");
					break;
				case cmLOR:
					NumeReKernel::printPreFmt("OR\n");
					break;
				case cmSUB:
					NumeReKernel::printPreFmt("SUB\n");
					break;
				case cmMUL:
					NumeReKernel::printPreFmt("MUL\n");
					break;
				case cmDIV:
					NumeReKernel::printPreFmt("DIV\n");
					break;
				case cmPOW:
					NumeReKernel::printPreFmt("POW\n");
					break;

				case cmIF:
				    NumeReKernel::printPreFmt("IF\t[OFFSET: " + toString(m_vRPN[i].Oprt.offset) + "]\n");
					break;

				case cmELSE:
					NumeReKernel::printPreFmt("ELSE\t[OFFSET: " + toString(m_vRPN[i].Oprt.offset) + "]\n");
					break;

				case cmENDIF:
					NumeReKernel::printPreFmt("ENDIF\n");
					break;

				case cmASSIGN:
					NumeReKernel::printPreFmt("ASSIGN \t[" + toHexString((int)m_vRPN[i].Oprt.ptr) + "]\n");
					break;

				default:
				    NumeReKernel::printPreFmt("(unknown \t(" + toString((int)m_vRPN[i].Cmd) + ")\n");
					break;
			} // switch cmdCode
		} // while bytecode

		NumeReKernel::printPreFmt("|   END\n");
	}
} // namespace mu
