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

#ifndef CODEANALYZER_HPP
#define CODEANALYZER_HPP

#include <string>
#include <vector>
#include <utility>

#include "../../kernel/core/symdef.hpp"

using namespace std;

/////////////////////////////////////////////////
/// \brief Stores the number of annotations for
/// displaying a summary
/////////////////////////////////////////////////
struct AnnotationCount
{
	size_t nNotes;
	size_t nWarnings;
	size_t nErrors;

	AnnotationCount() : nNotes(0), nWarnings(0), nErrors(0) {}
	AnnotationCount& operator+= (const AnnotationCount& annotcount)
	{
		nNotes += annotcount.nNotes;
		nWarnings += annotcount.nWarnings;
		nErrors += annotcount.nErrors;
		return *this;
	}
};


class NumeReEditor;
class Options;


/////////////////////////////////////////////////
/// \brief This class analyzes the code handled
/// by the editor by accessing it with friends
/// rights.
/////////////////////////////////////////////////
class CodeAnalyzer
{
    private:
        NumeReEditor* m_editor;
        Options* m_options;
        SymDefManager m_symdefs;

        string m_sNote;
        string m_sWarn;
        string m_sError;

        string m_sCurrentLine;
        string m_sStyles;

        int m_nCurPos;
        int m_nCurrentLine;

        bool m_hasProcedureDefinition;
        vector<pair<string,int> > m_vLocalVariables;

        const double MINCOMMENTDENSITY = 0.5;
        const double MAXCOMMENTDENSITY = 1.5;
        const int MAXCOMPLEXITYNOTIFY = 15;
        const int MAXCOMPLEXITYWARN = 20;
        const int MAXLINESOFCODE = 100;

        AnnotationCount analyseCommands();
        AnnotationCount analyseFunctions(bool isContinuedLine);
        AnnotationCount analyseProcedures();
        AnnotationCount analyseIdentifiers();
        AnnotationCount analyseOperators();
        AnnotationCount analyseNumbers();
        AnnotationCount addToAnnotation(const string& sMessage, int nStyle);

        string highlightFoundOccurence(const string& sElement, int nPos, int nLength);
        bool containsAssignment(const string& sCurrentLine);
        int calculateCyclomaticComplexity(int startline, int endline);
        int calculateLinesOfCode(int startline, int endline);
        int countNumberOfComments(int startline, int endline);

    public:
        CodeAnalyzer(NumeReEditor* parent, Options* opts);

        void run();
};


#endif // CODEANALYZER_HPP

