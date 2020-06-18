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
    along with m_editor program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "searchcontroller.hpp"
#include "editor.h"
#include "../terminal/wxterm.h"

/////////////////////////////////////////////////
/// \brief Creates a list of procedures in the current file
///
/// \return vector<wxString>
///
/// This member function creates the procedure list from
/// the current viewed file. It does nothing, if the
/// current file is not a procedure file
/////////////////////////////////////////////////
vector<wxString> SearchController::getProceduresInFile()
{
    if (m_editor->m_fileType != FILE_NPRC)
        return vector<wxString>();

    // Find all "procedure" commands in the current file
    vector<int> vMatch = FindAll("procedure", wxSTC_NPRC_COMMAND, 0, m_editor->GetLastPosition(), false);

    if (!vMatch.size())
        return vector<wxString>();

    vector<wxString> vProcDef;

    // Go through all matches and store the corresponding
    // definitions in the procedure definitions vector
    for (size_t i = 0; i < vMatch.size(); i++)
    {
        int pos = vMatch[i]+9;

        // Find the dollar sign
        pos = m_editor->FindText(pos, m_editor->GetLineEndPosition(m_editor->LineFromPosition(pos)), "$");

        if (pos == wxSTC_INVALID_POSITION)
            continue;

        // Find the procedure definition
        if (FindMarkedProcedure(pos+1, false).length())
            vProcDef.push_back(FindProcedureDefinition());
    }

    return vProcDef;
}


/////////////////////////////////////////////////
/// \brief Finds all occurences of a code symbol considering the style
///
/// \param sSymbol const wxString&
/// \param nStyle int
/// \param nStartPos int
/// \param nEndPos int
/// \param bSearchInComments bool
/// \return vector<int>
///
/// This member function detects all occurences of a code symbol
/// between the passed positions. It does take the current style
/// into account and returns the matches as a vector
/////////////////////////////////////////////////
vector<int> SearchController::FindAll(const wxString& sSymbol, int nStyle, int nStartPos, int nEndPos, bool bSearchInComments)
{
    vector<int> vMatches;
    int nCurrentPos = 0;

    if (!sSymbol.length())
        return vMatches;

    // Change the style of the string parser to the identifier
    // style
    if ((m_editor->m_fileType == FILE_NSCR || m_editor->m_fileType == FILE_NPRC) && nStyle == wxSTC_NSCR_STRING_PARSER)
        nStyle = wxSTC_NSCR_IDENTIFIER;

    // Search the next occurence
    while ((nCurrentPos = m_editor->FindText(nStartPos, nEndPos, sSymbol, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD)) != wxSTC_INVALID_POSITION)
    {
        nStartPos = nCurrentPos+1;

        // Is it the correct style and no field of a structure?
        if (m_editor->GetCharAt(nCurrentPos-1) != '.'
            && (m_editor->GetStyleAt(nCurrentPos) == nStyle
                || ((m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, nCurrentPos) || m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, nCurrentPos)) && bSearchInComments)
                || (m_editor->GetStyleAt(nCurrentPos) == wxSTC_NSCR_STRING_PARSER && (m_editor->m_fileType == FILE_NSCR || m_editor->m_fileType == FILE_NPRC))))
            vMatches.push_back(nCurrentPos);
    }

    // return the found matches
    return vMatches;
}


/////////////////////////////////////////////////
/// \brief Returns the word under the cursor while clicking
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString SearchController::FindClickedWord()
{
	int charpos = m_editor->PositionFromPoint(m_editor->m_lastRightClick);
	int startPosition = m_editor->WordStartPosition(charpos, true);
	int endPosition = m_editor->WordEndPosition(charpos, true);

	wxString clickedWord = m_editor->GetTextRange(startPosition, endPosition);
	m_editor->m_clickedWordLength = endPosition - startPosition;
	m_editor->m_clickedWord = clickedWord;
	return clickedWord;
}


/////////////////////////////////////////////////
/// \brief Gets the name of the clicked include file
///
/// \return wxString
///
/// This member function is used from the context menu to
/// obtain the name of the included file and to store it
/// internally
/////////////////////////////////////////////////
wxString SearchController::FindClickedInclude()
{
	int charpos = m_editor->PositionFromPoint(m_editor->m_lastRightClick);
    return FindMarkedInclude(charpos);
}


/////////////////////////////////////////////////
/// \brief Constructs the file name of the included file
///
/// \param charpos int
/// \return wxString
///
/// This member function constructs the name of the included
/// file at the passed position, returns its name and stores
/// the name internally
/////////////////////////////////////////////////
wxString SearchController::FindMarkedInclude(int charpos)
{
	int startPosition = m_editor->WordStartPosition(charpos, true);
	int endPosition = m_editor->WordEndPosition(startPosition + 1, true);

	// Find the first position
	while (startPosition && m_editor->GetStyleAt(startPosition - 1) == wxSTC_NSCR_INCLUDES && m_editor->GetCharAt(startPosition - 1) != '@')
		startPosition--;

    // Ignore the quotation mark
	if (m_editor->GetCharAt(startPosition) == '"')
		startPosition++;

	// Find the last position and exclude the trailing
	// quotation mark automatically
	while (endPosition < m_editor->GetLastPosition() && m_editor->GetStyleAt(endPosition) == wxSTC_NSCR_INCLUDES && m_editor->GetCharAt(endPosition) != ':' && m_editor->GetCharAt(endPosition) != '"')
		endPosition++;

    // Get the name from the positions
	wxString clickedWord = m_editor->GetTextRange(startPosition, endPosition);

	// Resolve path tokens, which are probably
	// part of the name
	if (clickedWord.find('<') != string::npos)
	{
		if (clickedWord.find("<>") != string::npos)
			clickedWord.replace(clickedWord.find("<>"), 2, m_terminal->getPathSettings()[EXEPATH]);

		if (clickedWord.find("<this>") != string::npos)
			clickedWord.replace(clickedWord.find("<this>"), 6, m_editor->GetFileName().GetPath(wxPATH_GET_VOLUME));

		if (clickedWord.find("<loadpath>") != string::npos)
			clickedWord.replace(clickedWord.find("<loadpath>"), 10, m_terminal->getPathSettings()[LOADPATH]);

		if (clickedWord.find("<savepath>") != string::npos)
			clickedWord.replace(clickedWord.find("<savepath>"), 10, m_terminal->getPathSettings()[SAVEPATH]);

		if (clickedWord.find("<scriptpath>") != string::npos)
			clickedWord.replace(clickedWord.find("<scriptpath>"), 12, m_terminal->getPathSettings()[SCRIPTPATH]);

		if (clickedWord.find("<procpath>") != string::npos)
			clickedWord.replace(clickedWord.find("<procpath>"), 10, m_terminal->getPathSettings()[PROCPATH]);

		if (clickedWord.find("<plotpath>") != string::npos)
			clickedWord.replace(clickedWord.find("<plotpath>"), 10, m_terminal->getPathSettings()[PLOTPATH]);
	}

	// Prepend the script folder, if necessary
	if (clickedWord.length() > 2 && ((isalpha(clickedWord[0]) && clickedWord[1] == ':') || clickedWord[0] == '.'))
		m_editor->m_clickedInclude = clickedWord + ".nscr";
	else
		m_editor->m_clickedInclude = m_terminal->getPathSettings()[SCRIPTPATH] + "/" + clickedWord + ".nscr";

	return replacePathSeparator(clickedWord.ToStdString());
}


/////////////////////////////////////////////////
/// \brief Gets the name of the clicked procedure
///
/// \return wxString
///
/// This member function is used from the context menu to
/// obtain the name of the procedure and to store it
/// internally
/////////////////////////////////////////////////
wxString SearchController::FindClickedProcedure()
{
	int charpos = m_editor->PositionFromPoint(m_editor->m_lastRightClick);
	return FindMarkedProcedure(charpos);
}


/////////////////////////////////////////////////
/// \brief Extracts the procedure call at the selected position
///
/// \param charpos int
/// \param ignoreDefinitions bool
/// \return wxString
///
/// This member function extracts the procedure call
/// located around the position charpos and stores
/// its value internally. If ignoreDefinitions is set
/// to false, then also definitions are detected
/////////////////////////////////////////////////
wxString SearchController::FindMarkedProcedure(int charpos, bool ignoreDefinitions)
{
	int startPosition = m_editor->WordStartPosition(charpos, true);
	int endPosition = m_editor->WordEndPosition(charpos, true);

	// Search for the first procedure character
	while (startPosition && m_editor->GetStyleAt(startPosition - 1) == wxSTC_NSCR_PROCEDURES)
		startPosition--;

    // Search for the last procedure character
	while (endPosition < m_editor->GetLastPosition() && m_editor->GetStyleAt(endPosition) == wxSTC_NSCR_PROCEDURES)
		endPosition++;

    // Ignore procedure definitions, if the
    // flag is set to true
    if (ignoreDefinitions)
    {
        wxString currentline = m_editor->GetLine(m_editor->LineFromPosition(startPosition));

        if (currentline.find("procedure") != string::npos && currentline[currentline.find_first_not_of(' ', currentline.find("procedure") + 9)] == '$')
            return "";
    }

    // Extract the procedure call
	wxString clickedWord = m_editor->GetTextRange(startPosition, endPosition);

    // Insert the namespaces, if we use
    // definition as well
	if (!ignoreDefinitions && clickedWord.find('~') == string::npos && GetNameOfNamingProcedure() != clickedWord)
    {
        clickedWord.insert(1, "thisfile~");
    }

    // Search the namespace of the current call
    // and insert it
	if (m_editor->m_fileType == FILE_NPRC && clickedWord.find('~') == string::npos)
	{
	    // Find the the namespace
		wxString sNameSpace = FindNameSpaceOfProcedure(charpos);

		// Insert the namespace, if it is
		// available
		if (sNameSpace.length())
		{
			if (clickedWord[0] == '$')
				clickedWord.insert(1, sNameSpace + "~");
			else
				clickedWord = "$" + sNameSpace + "~" + clickedWord;
		}
	}

	// Store the procedure call
	m_editor->m_clickedProcedure = clickedWord;

	// Remove namespaces for the context menu
	if (clickedWord.find('~') != string::npos)
		clickedWord.erase(1, clickedWord.rfind('~'));

	if (clickedWord[0] != '$')
		clickedWord.insert(0, 1, '$');

    // Return the string for the context menu
	return clickedWord + "()";
}


/////////////////////////////////////////////////
/// \brief Finds the current namespace for the procedure call
///
/// \param charpos int
/// \return wxString
///
/// Returns the namespace of the procedure call at
/// the selected position. Searches the current
/// procedure scope for "namespace" commands and
/// decodes them
/////////////////////////////////////////////////
wxString SearchController::FindNameSpaceOfProcedure(int charpos)
{
	wxString sNameSpace = "this";

	if (m_editor->m_fileType == FILE_NPRC)
	{
		int minpos = 0;
		int maxpos = charpos;

		// Find the start of the current procedure
		minpos = FindCurrentProcedureHead(charpos);

		// Find all occurences of "namespace" between
		// the minimal and the maximal position
		vector<int> namespaces = FindAll("namespace", wxSTC_NPRC_COMMAND, minpos, maxpos, false);

		// Use the last namespace command, if there
		// are any, and decode it
		if (namespaces.size())
        {
            sNameSpace = decodeNameSpace(m_editor->GetLine(m_editor->LineFromPosition(namespaces.back())).ToStdString(), "this");
        }
	}

	return sNameSpace;
}


/////////////////////////////////////////////////
/// \brief Finds procedures, which might match the passed word start
///
/// \param sFirstChars wxString
/// \param sSelectedNamespace wxString
/// \return wxString
///
/// Finds all procedures in the current file, which might
/// fit the passed word start, and return them as a single
/// autocompletion string
/////////////////////////////////////////////////
wxString SearchController::FindProceduresInCurrentFile(wxString sFirstChars, wxString sSelectedNamespace)
{
	wxString sThisFileProcedures;

	// Go through the whole file
	for (int i = 0; i < m_editor->GetLineCount(); i++)
	{
		wxString currentline = m_editor->GetLine(i);

		// Search for procedure commands
		if (currentline.find("procedure") != string::npos
				&& currentline.find('$', currentline.find("procedure")) != string::npos
				&& m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, m_editor->PositionFromLine(i) + currentline.find("procedure"))
				&& m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, m_editor->PositionFromLine(i) + currentline.find("procedure")))
		{
			currentline.erase(0, currentline.find('$') + 1);

			if (currentline.find('(') == string::npos)
				continue;

			currentline.erase(currentline.find('('));

			// combine all matches to the list of procedures
			if (currentline.substr(0, sFirstChars.length()) == sFirstChars)
				sThisFileProcedures += currentline + "(?" + toString(NumeReSyntax::SYNTAX_PROCEDURE) + " ";
		}
	}

	if (sSelectedNamespace.length())
		return sThisFileProcedures;

	return sThisFileProcedures + m_editor->_syntax->getNameSpaceAutoCompList(sFirstChars.ToStdString());
}


/////////////////////////////////////////////////
/// \brief Searches the definition below the cursor
///
/// \return wxString
///
/// This member function searches for the definition of
/// the procedure, which is currently below the cursor.
/////////////////////////////////////////////////
wxString SearchController::FindProcedureDefinition()
{
    // do nothing, if there's no currently selected procedure
	if (!m_editor->m_clickedProcedure.length())
		return "";

	vector<std::string> vPaths = m_terminal->getPathSettings();
	wxString pathname = m_editor->m_clickedProcedure;
	wxString procedurename = pathname.substr(pathname.rfind('~') + 1); // contains a "$", if it's not used for the "thisfile~" case

	// Handle the namespaces
	if (pathname.find("$this~") != string::npos)
	{
	    // This namespace (the current folder)
		wxString thispath = m_editor->GetFileNameAndPath();
		pathname.replace(pathname.find("$this~"), 6, thispath.substr(0, thispath.rfind('\\') + 1));
	}
	else if (pathname.find("$thisfile~") != string::npos)
	{
	    // local namespace
		return FindProcedureDefinitionInLocalFile(procedurename);
	}
	else
	{
	    // All other namespaces
		if (pathname.find("$main~") != string::npos)
			pathname.erase(pathname.find("$main~") + 1, 5);

		while (pathname.find('~') != string::npos)
			pathname[pathname.find('~')] = '/';

		// Add the root folders to the path name
		if (pathname[0] == '$' && pathname.find(':') == string::npos)
			pathname.replace(0, 1, vPaths[PROCPATH] + "/");
		else if (pathname.find(':') == string::npos)
			pathname.insert(0, vPaths[PROCPATH]);
		else // pathname.find(':') != string::npos
		{
		    // Absolute file paths
			pathname = pathname.substr(pathname.find('\'') + 1, pathname.rfind('\'') - pathname.find('\'') - 1);
		}
	}

	// Find the namespace in absolute procedure paths
	while (procedurename.find('\'') != string::npos)
		procedurename.erase(procedurename.find('\''), 1);

	if (procedurename.find('/') != string::npos)
		procedurename = "$" + procedurename.substr(procedurename.rfind('/') + 1);

	if (procedurename.find('\\') != string::npos)
		procedurename = "$" + procedurename.substr(procedurename.rfind('\\') + 1);

	if (procedurename[0] != '$')
		procedurename.insert(0, 1, '$');

	// Find procedure in a global procedure file
	return FindProcedureDefinitionInOtherFile(pathname, procedurename);
}


/////////////////////////////////////////////////
/// \brief Search the procedure definition in the local file
///
/// \param procedurename const wxString&
/// \return wxString
///
/// This private member function searches for the procedure
/// definition in the currently opened procedure file. It also
/// appends the documentation to the definition, so that
/// it might be shown in the tooltip
/////////////////////////////////////////////////
wxString SearchController::FindProcedureDefinitionInLocalFile(const wxString& procedurename)
{
    wxString procedureline;

    // Force Scintilla to style the whole document
    if (m_editor->GetLastPosition() > m_editor->GetEndStyled() && !m_editor->GetWrapMode())
    {
        m_editor->SetWrapMode(wxSTC_WRAP_WORD);
        m_editor->SetWrapMode(wxSTC_WRAP_NONE);
    }

    // Go through the whole file and search for all occurences of the procedure
    // command
    vector<int> procedures = FindAll("procedure", wxSTC_NSCR_COMMAND, 0, m_editor->GetLastPosition(), false);

    // Examine each occurence
    for (size_t i = 0; i < procedures.size(); i++)
    {
        procedureline = m_editor->GetLine(m_editor->LineFromPosition(procedures[i]));

        if (procedureline.find("$" + procedurename) != string::npos && procedureline[procedureline.find_first_not_of(' ', procedureline.find("$" + procedurename) + procedurename.length() + 1)] == '(')
        {
            if (getMatchingParenthesis(procedureline.substr(procedureline.find("$" + procedurename)).ToStdString()) == string::npos)
                return "";

            // Extraxt the procedure definition
            string sProcDef = procedureline.substr(procedureline.find("$" + procedurename), getMatchingParenthesis(procedureline.substr(procedureline.find("$" + procedurename)).ToStdString()) + 1).ToStdString();
            size_t nFirstParens = sProcDef.find('(');
            string sArgList = sProcDef.substr(nFirstParens + 1, getMatchingParenthesis(sProcDef.substr(nFirstParens)) - 1);
            sProcDef.erase(nFirstParens + 1);

            // Handle the argument list
            while (sArgList.length())
            {
                string currentarg = getNextArgument(sArgList, true);

                if (currentarg.front() == '_')
                    currentarg.erase(0, 1);

                sProcDef += currentarg;

                if (sArgList.length())
                    sProcDef += ", ";
            }

            sProcDef += ") :: local";

            // Handle the flags
            if (procedureline.find("::") != string::npos)
            {
                string sFlags = procedureline.substr(procedureline.find("::") + 2).ToStdString();

                if (sFlags.find("##") != string::npos)
                    sFlags.erase(sFlags.find("##"));

                if (sFlags.find_first_of("\r\n") != string::npos)
                    sFlags.erase(sFlags.find_first_of("\r\n"));

                StripSpaces(sFlags);
                sProcDef += " " + sFlags;
            }

            wxString sDocumentation;

            // Find now the documentations - documentation lines above
            // the current line are preferred:
            for (int docline = m_editor->LineFromPosition(procedures[i])-1; docline >= 0; docline--)
            {
                if (!m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_SECTION_LINE, m_editor->GetLineIndentPosition(docline))
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_SECTION_BLOCK, m_editor->GetLineIndentPosition(docline)))
                {
                    if (docline < m_editor->LineFromPosition(procedures[i])-1)
                    {
                        for (int curline = docline+1; curline < m_editor->LineFromPosition(procedures[i]); curline++)
                        {
                            wxString curdocline = m_editor->GetLine(curline);
                            curdocline.erase(0, curdocline.find_first_not_of(" \t#*!"));
                            curdocline.erase(curdocline.find_first_of("\r\n"));

                            if (curdocline.find("*#") != string::npos)
                                curdocline.erase(curdocline.find("*#"));

                            AppendToDocumentation(sDocumentation, curdocline);
                        }
                    }
                    break;
                }
            }

            // If the documentation string is still empty
            // search below the definition
            if (!sDocumentation.length())
            {
                for (int docline = m_editor->LineFromPosition(procedures[i])+1; docline < m_editor->GetLineCount(); docline++)
                {
                    if (!m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_SECTION_LINE, m_editor->GetLineIndentPosition(docline))
                        && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_SECTION_BLOCK, m_editor->GetLineIndentPosition(docline)))
                    {
                        if (docline > m_editor->LineFromPosition(procedures[i])+1)
                        {
                            for (int curline = m_editor->LineFromPosition(procedures[i])+1; curline < docline; curline++)
                            {
                                wxString curdocline = m_editor->GetLine(curline);
                                curdocline.erase(0, curdocline.find_first_not_of(" \t#*!"));
                                curdocline.erase(curdocline.find_first_of("\r\n"));

                                AppendToDocumentation(sDocumentation, curdocline);
                            }
                        }
                        break;
                    }
                }
            }

            // clean the documentation
            sDocumentation = CleanDocumentation(sDocumentation);

            // Append the documentation if it is present
            if (sDocumentation.length())
                sProcDef += "\n" + sDocumentation.ToStdString();

            return sProcDef;
        }
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief Search the procedure definition in a global file.
///
/// \param pathname const wxString&
/// \param procedurename const wxString&
/// \return wxString
///
/// This private member function searches for the procedure
/// definition in a selected global procedure file. It also
/// appends the documentation to the definition, so that
/// it might be shown in the tooltip.
/////////////////////////////////////////////////
wxString SearchController::FindProcedureDefinitionInOtherFile(const wxString& pathname, const wxString& procedurename)
{
	if (!fileExists((pathname + ".nprc").ToStdString()))
	{
		return "";
	}
	else
	{
		ifstream procedure_in;
		string sProcCommandLine;
		bool bBlockComment = false;
		wxString sDocumentation;
		bool bDocFound = false;
		procedure_in.open((pathname + ".nprc").ToStdString().c_str());

		// Ensure that the file is in good state
		if (!procedure_in.good())
			return "";

		// As long as we're not at the end of the file
		while (!procedure_in.eof())
		{
		    // Read one line and strip all spaces
			getline(procedure_in, sProcCommandLine);
			StripSpaces(sProcCommandLine);

			// Ignore empty lines
			if (!sProcCommandLine.length())
            {
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }

            // Ignore comment lines
			if (sProcCommandLine.substr(0, 2) == "##")
            {
                // Append each documentation string
                if (sProcCommandLine.substr(0, 3) == "##!")
                {
                    AppendToDocumentation(sDocumentation, sProcCommandLine.substr(3));
                }
				continue;
            }

            // Erase line comment parts
			if (sProcCommandLine.find("##") != string::npos)
				sProcCommandLine = sProcCommandLine.substr(0, sProcCommandLine.find("##"));

			// Remove block comments and continue
			if (sProcCommandLine.substr(0, 2) == "#*" && sProcCommandLine.find("*#", 2) == string::npos)
			{
			    if (sProcCommandLine.substr(0, 3) == "#*!")
                {
                    bDocFound = true;
                    AppendToDocumentation(sDocumentation, sProcCommandLine.substr(3));
                }
				bBlockComment = true;
				continue;
			}

			// Search for the end of the current block comment
			if (bBlockComment && sProcCommandLine.find("*#") != string::npos)
			{
				bBlockComment = false;
				if (bDocFound)
                {
                    AppendToDocumentation(sDocumentation, sProcCommandLine.substr(0, sProcCommandLine.find("*#")));
                }
				if (sProcCommandLine.find("*#") == sProcCommandLine.length() - 2)
				{
					continue;
				}
				else
					sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("*#") + 2);
			}
			else if (bBlockComment && sProcCommandLine.find("*#") == string::npos)
			{
			    // if the documentation has a length, append the current block
			    if (bDocFound)
                {
                    AppendToDocumentation(sDocumentation, sProcCommandLine);
                }
				continue;
			}

			// Ignore includes
			if (sProcCommandLine[0] != '@' && findCommand(sProcCommandLine).sString != "procedure")
            {
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }
			else if (sProcCommandLine[0] == '@')
            {
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }

			// Ignore lines without "procedure"
			if (findCommand(sProcCommandLine).sString != "procedure")
            {
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }

			// Search for the current procedure name
			if (sProcCommandLine.find(procedurename.ToStdString()) == string::npos || sProcCommandLine.find('(') == string::npos)
            {
                // clear the documentation string
                sDocumentation.clear();
                bDocFound = false;
				continue;
            }
			else
			{
                // Found the procedure name, now extract the definition
				if (getMatchingParenthesis(sProcCommandLine.substr(sProcCommandLine.find(procedurename.ToStdString()))) == string::npos)
					return "";
				string sProcDef = sProcCommandLine.substr(sProcCommandLine.find(procedurename.ToStdString()), getMatchingParenthesis(sProcCommandLine.substr(sProcCommandLine.find(procedurename.ToStdString()))) + 1);
				size_t nFirstParens = sProcDef.find('(');
				string sArgList = sProcDef.substr(nFirstParens + 1, getMatchingParenthesis(sProcDef.substr(nFirstParens)) - 1);
				sProcDef.erase(nFirstParens + 1);
				while (sArgList.length())
				{
					string currentarg = getNextArgument(sArgList, true);
					if (currentarg.front() == '_')
						currentarg.erase(0, 1);
					sProcDef += currentarg;
					if (sArgList.length())
						sProcDef += ", ";
				}
				sProcDef += ")";

				if (sProcCommandLine.find("::") != string::npos)
				{
					string sFlags = sProcCommandLine.substr(sProcCommandLine.find("::") + 2).c_str();
					if (sFlags.find("##") != string::npos)
						sFlags.erase(sFlags.find("##"));
					StripSpaces(sFlags);
					sProcDef += " :: " + sFlags;
				}

				// If no documentation was found, search in the following lines
				if (!sDocumentation.length())
                {
                    while (!procedure_in.eof())
                    {
                        getline(procedure_in, sProcCommandLine);
                        StripSpaces(sProcCommandLine);
                        if (sProcCommandLine.substr(0, 3) == "##!")
                        {
                            AppendToDocumentation(sDocumentation, sProcCommandLine.substr(3));
                        }
                        else if (sProcCommandLine.substr(0, 3) == "#*!")
                        {
                            AppendToDocumentation(sDocumentation, sProcCommandLine.substr(3));
                            bBlockComment = true;
                        }
                        else if (bBlockComment)
                        {
                            AppendToDocumentation(sDocumentation, sProcCommandLine.substr(0, sProcCommandLine.find("*#")));
                            if (sProcCommandLine.find("*#") != string::npos)
                                break;
                        }
                        else
                            break;
                    }
                }

                // clean the documentation
                sDocumentation = CleanDocumentation(sDocumentation);

				// if the documentation has a length, append it here
				if (sDocumentation.length())
                    sProcDef += "\n" + sDocumentation.ToStdString();
				return sProcDef;
			}
		}
	}
    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the required procedure name for the current file.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString SearchController::GetNameOfNamingProcedure()
{
    return "$" + m_editor->GetFileName().GetName();
}

/////////////////////////////////////////////////
/// \brief Appends the text to the current documentation.
///
/// \param sDocumentation wxString&
/// \param sNewDocLine const wxString&
/// \return void
///
/// This member function appends a found documentation line to the overall
/// documentation and converts some TeX-commands into plain
/// text and rudimentary styling.
/////////////////////////////////////////////////
void SearchController::AppendToDocumentation(wxString& sDocumentation, const wxString& sNewDocLine)
{
    static bool bBeginEnd = false;

    if (sNewDocLine.find_first_not_of(" \t") == string::npos)
    {
        if (sDocumentation.length())
            sDocumentation += "\n    ";

        return;
    }

    // Handle some special TeX commands and rudimentary lists
    if (sNewDocLine.find("\\begin{") != string::npos && sNewDocLine.find("\\end{") == string::npos)
    {
        if (sDocumentation.length() && sDocumentation[sDocumentation.length()-1] != '\n')
            sDocumentation += "\n    ";

        bBeginEnd = true;
    }
    else if (sNewDocLine.find("\\begin{") == string::npos && sNewDocLine.find("\\end{") != string::npos)
    {
        if (sDocumentation.length() && sDocumentation[sDocumentation.length()-1] != '\n')
            sDocumentation += "\n    ";

        bBeginEnd = false;
    }
    else if ((sNewDocLine.length() && (sNewDocLine.substr(sNewDocLine.find_first_not_of(" \t"), 2) == "- " || sNewDocLine.substr(sNewDocLine.find_first_not_of(" \t"), 7) == "\\param ")) || bBeginEnd)
    {
        if (sDocumentation.length() && sDocumentation[sDocumentation.length()-1] != '\n')
            sDocumentation += "\n    ";
    }
    else
    {
        if (sDocumentation.length() && sDocumentation[sDocumentation.length()-1] != ' ')
            sDocumentation += " ";
    }

    sDocumentation += sNewDocLine.substr(sNewDocLine.find_first_not_of(" \t"));
    size_t nPos = sDocumentation.find("\\procedure{");

    if (nPos != string::npos)
        sDocumentation.erase(nPos, sDocumentation.find('}', nPos)+1 - nPos);
    else if ((nPos = sDocumentation.find("\\procedure ")) != string::npos)
    {
        size_t nPos2 = nPos + 10;
        nPos2 = sDocumentation.find_first_not_of(" \r\n", nPos2);
        nPos2 = sDocumentation.find_first_of(" \r\n", nPos2);
        sDocumentation.erase(nPos, nPos2-nPos);
    }
}


/////////////////////////////////////////////////
/// \brief Checks layout and finishes styling of the documentation string.
///
/// \param __sDoc const wxString&
/// \return string
///
/// This member function checks the layout of the found documentations
/// and applies some special modifications.
/////////////////////////////////////////////////
string SearchController::CleanDocumentation(const wxString& __sDoc)
{
    string sDocumentation = __sDoc.ToStdString();

    if (sDocumentation.find_first_not_of(" \n") != string::npos)
    {
        // Clean whitespace before and after the documentation
        sDocumentation.erase(0, sDocumentation.find_first_not_of(" \n"));

        if (sDocumentation.back() == ' ' || sDocumentation.back() == '\n')
            sDocumentation.erase(sDocumentation.find_last_not_of(" \n")+1);

        size_t nPos = sDocumentation.find("\\param ");

        // Resolve "\param" keywords
        if (nPos != string::npos)
        {
            // Insert a headline above the first parameter
            if (nPos > 5 && sDocumentation.substr(nPos-5, 5) != "\n    ")
                sDocumentation.insert(nPos, "\n    " + toUpperCase(_guilang.get("GUI_EDITOR_CALLTIP_PROC_PARAMS")) + "\n    ");
            else
                sDocumentation.insert(nPos, toUpperCase(_guilang.get("GUI_EDITOR_CALLTIP_PROC_PARAMS")) + "\n    ");

            while ((nPos = sDocumentation.find("\\param ")) != string::npos)
            {
                sDocumentation.replace(nPos, 6, "-");
                sDocumentation.insert(sDocumentation.find(' ', sDocumentation.find_first_not_of(' ', nPos+1)), ":");

                if (sDocumentation[sDocumentation.find_first_not_of(' ', nPos+1)] == '_')
                    sDocumentation.erase(sDocumentation.find_first_not_of(' ', nPos+1), 1);
            }
        }

        // Remove doubled exclamation marks
        while (sDocumentation.find("!!") != string::npos)
            sDocumentation.erase(sDocumentation.find("!!"), 2);

        // Replace \begin{} and \end{} with line breaks
        // This logic bases upon the replacements done
        // in NumeReEditor::AppendToDocumentation
        size_t nMatch = 0;

        while ((nMatch = sDocumentation.find("\\begin{")) != string::npos)
        {
            sDocumentation.erase(nMatch, sDocumentation.find('}', nMatch) + 1 - nMatch);

            if (sDocumentation.substr(nMatch, 5) == "\n    ")
                sDocumentation.erase(nMatch, 5);
        }

        while ((nMatch = sDocumentation.find("\\end{")) != string::npos)
        {
            sDocumentation.erase(nMatch, sDocumentation.find('}', nMatch) + 1 - nMatch + 1);
        }

        // Check the length of the line
        sDocumentation = m_editor->addLinebreaks(sDocumentation, true);
    }
    else
        sDocumentation.clear();

    return sDocumentation;
}


/////////////////////////////////////////////////
/// \brief Find the procedure head line for the selected position
///
/// \param pos int
/// \return int
///
/// This member function identifies the position of the procedure
/// head, to which the selected position belongs.
/////////////////////////////////////////////////
int SearchController::FindCurrentProcedureHead(int pos)
{
    vector<int> procedureheads;

    // Find all occurences of procedure heads in the current
    // file, which are before the selected position
    if (m_editor->m_fileType == FILE_NPRC)
        procedureheads = FindAll("procedure", wxSTC_NSCR_COMMAND, 0, pos, false);
    else if (m_editor->m_fileType == FILE_MATLAB)
        procedureheads = FindAll("function", wxSTC_MATLAB_KEYWORD, 0, pos, false);

    // If any was found, return the last one
    if (procedureheads.size())
        return procedureheads.back();

    return 0;
}


/////////////////////////////////////////////////
/// \brief Searches the file for the naming procedure
///
/// \return int
///
/// Searches the file for the naming procedure and returns
/// its position in the file or wxNOT_FOUND, if no matching
/// procedure can be found.
/////////////////////////////////////////////////
int SearchController::FindNamingProcedure()
{
	wxString sNamingProcedure = GetNameOfNamingProcedure() + "(";

	// Go through the whole file and search for all occurences of
	// the "procedure" command
	for (int i = 0; i < m_editor->LineFromPosition(m_editor->GetLastPosition()); i++)
	{
		wxString currentline = m_editor->GetLine(i);

		if (currentline.find("procedure") != string::npos && currentline.find(sNamingProcedure) != string::npos)
		{
			int linepos = m_editor->PositionFromLine(i);
			int offset = 0;

			while (currentline.find("procedure", offset) != string::npos && m_editor->GetStyleAt(linepos + currentline.find("procedure", offset) + 1) != wxSTC_NPRC_COMMAND)
				offset = currentline.find("procedure", offset) + 10;

            // If a candidate was found, check, whether it has the correct name
			if (currentline.find("procedure", offset) != string::npos && m_editor->GetStyleAt(linepos + currentline.find("procedure", offset) + 1) == wxSTC_NPRC_COMMAND)
			{
				int procloc = currentline.find("procedure", offset) + 9;

				while (currentline.find(sNamingProcedure, procloc) != string::npos && m_editor->GetStyleAt(linepos + currentline.find(sNamingProcedure, procloc) + 1) != wxSTC_NPRC_PROCEDURES)
					procloc = currentline.find(sNamingProcedure, procloc) + 1 + sNamingProcedure.length();

                // Return the position of this procedure, if the name is fitting
				if (currentline.find(sNamingProcedure, procloc) != string::npos && m_editor->GetStyleAt(linepos + currentline.find(sNamingProcedure) + 1) == wxSTC_NPRC_PROCEDURES)
					return i;
			}
		}
	}

	// Return not found, if nothing was found
	return wxNOT_FOUND;
}


