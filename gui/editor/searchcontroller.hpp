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

#ifndef SEARCHCONTROLLER_HPP
#define SEARCHCONTROLLER_HPP

#include <wx/wx.h>
#include <string>
#include <vector>

class NumeReEditor;
class NumeReTerminal;

/////////////////////////////////////////////////
/// \brief This class handles all specialized search
/// requests of the assigned editor
/////////////////////////////////////////////////
class SearchController
{
    private:
        NumeReEditor* m_editor;
        NumeReTerminal* m_terminal;

		wxString FindProcedureDefinitionInLocalFile(const wxString& procedurename);
		std::string CleanDocumentation(std::string sDocumentation);


    public:
        SearchController(NumeReEditor* edit, NumeReTerminal* terminal) : m_editor(edit), m_terminal(terminal) {}

		std::vector<wxString> getProceduresInFile();
		std::vector<int> FindAll(const wxString& sSymbol, int nStyle, int nStartPos = 0, int nEndPos = -1, bool bSearchInComments = false);

        wxString FindClickedWord();
		wxString FindClickedInclude();
		wxString FindMarkedInclude(int charpos);
		wxString FindClickedProcedure();
		wxString FindMarkedProcedure(int charpos, bool ignoreDefinitions = true);
		wxString FindNameSpaceOfProcedure(int charpos);
		wxString FindProceduresInCurrentFile(wxString sFirstChars, wxString sSelectedNameSpace);
		wxString FindProcedureDefinition();
		wxString GetNameOfNamingProcedure();
		int FindCurrentProcedureHead(int pos);
		int FindNamingProcedure();

};


#endif // SEARCHCONTROLLER_HPP

