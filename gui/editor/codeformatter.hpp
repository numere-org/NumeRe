/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#ifndef CODEFORMATTER_HPP
#define CODEFORMATTER_HPP

#include <wx/wx.h>

class NumeReEditor;

/////////////////////////////////////////////////
/// \brief This class handles all formatting
/// features of the editor.
/////////////////////////////////////////////////
class CodeFormatter
{
    private:
        NumeReEditor* m_editor;

        int determineIndentationLevel(int nLine, int& singleLineIndent);
        int determineIndentationLevelNSCR(int nLine, int& singleLineIndent);
        int determineIndentationLevelMATLAB(int nLine, int& singleLineIndent);
        int determineIndentationLevelCPP(int nLine, int& singleLineIndent);
        void ApplyAutoFormatNSCR(int nFirstLine, int nLastLine);
        void ApplyAutoFormatMATLAB(int nFirstLine, int nLastLine);
        void ApplyAutoFormatCPP(int nFirstLine, int nLastLine);

        int insertTextAndMove(int nPosition, const wxString& sText);

    public:
        CodeFormatter(NumeReEditor* edit) : m_editor(edit) {}

        void IndentCode(int nFirstLine, int nLastLine);
        void FormatCode(int nFirstLine, int nLastLine);
};


#endif // CODEFORMATTER_HPP

