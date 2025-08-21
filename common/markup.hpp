/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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

#ifndef MARKUP_HPP
#define MARKUP_HPP

#include <vector>


namespace Markup
{
    enum LineState
    {
        PARAGRAPH = 0,
        H1,
        H2,
        H3,
        UL
    };

    enum InlineState
    {
        TEXT = 0,
        ITALICS = 0x1,
        BOLD = 0x2,
        BRCKT = 0x4,
        EMPH = 0x8,
        CODE = 0x10
    };

    template<class STRING>
    struct Token
    {
        STRING text;
        LineState line;
        int inLine;
    };


    /////////////////////////////////////////////////
    /// \brief Decode a markup string into individual
    /// tokens defining their respective line and
    /// inline states.
    ///
    /// \param sString const STRING&
    /// \return std::vector<Token<STRING>>
    ///
    /////////////////////////////////////////////////
    template <class STRING>
    std::vector<Token<STRING>> decode(const STRING& sString)
    {
        std::vector<Token<STRING>> styled;

        size_t lastPos = 0;
        bool startOfLine = true;

        LineState lineMarkupState = PARAGRAPH;
        int inlineMarkupState = TEXT;

        for (size_t i = 0; i < sString.size(); i++)
        {
            if (i && sString[i-1] == '\n')
                startOfLine = true;

            if (inlineMarkupState & CODE)
            {
                if (sString[i] == '`')
                {
                    styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                                   .line{lineMarkupState},
                                                   .inLine{inlineMarkupState}});
                    lastPos = i+1;
                    inlineMarkupState &= ~CODE;
                }

                continue;
            }

            if (startOfLine)
            {
                if (sString.substr(i, 3) == "###")
                {
                    styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                                   .line{lineMarkupState},
                                                   .inLine{inlineMarkupState}});
                    lastPos = sString.find_first_not_of("# ", i);
                    lineMarkupState = H3;
                    i += 2;
                }
                else if (sString.substr(i, 2) == "##")
                {
                    styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                                   .line{lineMarkupState},
                                                   .inLine{inlineMarkupState}});
                    lastPos = sString.find_first_not_of("# ", i);
                    lineMarkupState = H2;
                    i++;
                }
                else if (sString[i] == '#')
                {
                    styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                                   .line{lineMarkupState},
                                                   .inLine{inlineMarkupState}});
                    lastPos = sString.find_first_not_of("# ", i);
                    lineMarkupState = H1;
                }
                else if (sString.substr(i, 2) == "- ")
                {
                    styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                                   .line{lineMarkupState},
                                                   .inLine{inlineMarkupState}});
                    lastPos = i+1;
                    lineMarkupState = UL;
                }
                else
                    lineMarkupState = PARAGRAPH;
            }

            if (sString[i] == '[')
            {
                styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                               .line{lineMarkupState},
                                               .inLine{inlineMarkupState}});
                lastPos = i;
                inlineMarkupState |= BRCKT;
            }
            else if (sString[i] == ']')
            {
                styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i + 1 - lastPos)},
                                               .line{lineMarkupState},
                                               .inLine{inlineMarkupState}});
                lastPos = i+1;
                inlineMarkupState &= ~BRCKT;
            }
            else if (sString[i] == '\n' && lineMarkupState)
            {
                styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i + 1 - lastPos)},
                                               .line{lineMarkupState},
                                               .inLine{inlineMarkupState}});
                lastPos = i+1;
                lineMarkupState = PARAGRAPH;
            }
            else if (sString[i] == '`')
            {
                styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                               .line{lineMarkupState},
                                               .inLine{inlineMarkupState}});
                lastPos = i+1;
                inlineMarkupState |= CODE;
            }
            else if (sString.substr(i, 3) == "***"
                     && ((inlineMarkupState & ITALICS && inlineMarkupState & BOLD)
                         || (sString.length() > i+3 && !std::isblank(sString[i+3]))))
            {
                styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                               .line{lineMarkupState},
                                               .inLine{inlineMarkupState}});
                lastPos = i+3;

                if (!(inlineMarkupState & ITALICS) && !(inlineMarkupState & BOLD))
                    inlineMarkupState |= BOLD | ITALICS;
                else if ((inlineMarkupState & ITALICS && inlineMarkupState & BOLD))
                    inlineMarkupState &= ~(BOLD | ITALICS);

                i += 2;
            }
            else if (sString.substr(i, 2) == "**"
                     && ((inlineMarkupState & BOLD) || (sString.length() > i+2 && !std::isblank(sString[i+2]))))
            {
                styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                               .line{lineMarkupState},
                                               .inLine{inlineMarkupState}});
                lastPos = i+2;

                if (!(inlineMarkupState & BOLD))
                    inlineMarkupState |= BOLD;
                else if (inlineMarkupState & BOLD)
                    inlineMarkupState &= ~BOLD;

                i++;
            }
            else if (sString[i] == '*'
                     && ((inlineMarkupState & ITALICS) || (sString.length() > i+1 && !std::isblank(sString[i+1]))))
            {
                styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                               .line{lineMarkupState},
                                               .inLine{inlineMarkupState}});
                lastPos = i+1;

                if (!(inlineMarkupState & ITALICS))
                    inlineMarkupState |= ITALICS;
                else if (inlineMarkupState & ITALICS)
                    inlineMarkupState &= ~ITALICS;
            }
            else if (sString.substr(i, 2) == "=="
                     && ((inlineMarkupState & EMPH) || (sString.length() > i+2 && !std::isblank(sString[i+2]))))
            {
                styled.push_back(Token<STRING>{.text{sString.substr(lastPos, i - lastPos)},
                                               .line{lineMarkupState},
                                               .inLine{inlineMarkupState}});
                lastPos = i+2;

                if (!(inlineMarkupState & EMPH))
                    inlineMarkupState |= EMPH;
                else if (inlineMarkupState & EMPH)
                    inlineMarkupState &= ~EMPH;

                i++;
            }

            startOfLine = false;
        }

        styled.push_back(Token<STRING>{.text{sString.substr(lastPos)},
                                       .line{lineMarkupState},
                                       .inLine{inlineMarkupState}});

        return styled;
    }

}



#endif // MARKUP_HPP


