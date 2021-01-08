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

#include "codeanalyzer.hpp"
#include "editor.h"
#include "searchcontroller.hpp"
#include "../../common/Options.h"
#include "../../kernel/core/ui/language.hpp"

#define ANNOTATION_NOTE wxSTC_NSCR_PROCEDURE_COMMANDS+1
#define ANNOTATION_WARN wxSTC_NSCR_PROCEDURE_COMMANDS+2
#define ANNOTATION_ERROR wxSTC_NSCR_PROCEDURE_COMMANDS+3

#define HIGHLIGHT_ANNOTATION 12

extern Language _guilang;

/////////////////////////////////////////////////
/// Constructor
/// \param parent NumeReEditor*
/// \param opts Options*
///
/////////////////////////////////////////////////
CodeAnalyzer::CodeAnalyzer(NumeReEditor* parent, Options* opts) : m_editor(parent), m_options(opts), m_nCurPos(0), m_nCurrentLine(0), m_hasProcedureDefinition(false)
{
	m_sNote = _guilang.get("GUI_ANALYZER_NOTE");
	m_sWarn = _guilang.get("GUI_ANALYZER_WARN");
	m_sError = _guilang.get("GUI_ANALYZER_ERROR");
}


/////////////////////////////////////////////////
/// \brief Starts the code analysis. Requires a pointer to the editor and to the options
///
/// \return void
///
/// This member function is the wrapper for the static code analyzer
/// It may handle NumeRe and MATLAB files. The results are written
/// directly as annotations in the editor.
/////////////////////////////////////////////////
void CodeAnalyzer::run()
{
    if (!m_editor || !m_options)
        return;

    // Clear all annotations
	m_editor->AnnotationClearAll();

	// Clear the corresponding indicators
    m_editor->SetIndicatorCurrent(HIGHLIGHT_ANNOTATION);
	m_editor->IndicatorClearRange(0, m_editor->GetLastPosition());
	m_editor->IndicatorSetStyle(HIGHLIGHT_ANNOTATION, wxSTC_INDIC_ROUNDBOX);
	m_editor->IndicatorSetForeground(HIGHLIGHT_ANNOTATION, wxColor(0, 0, 255));

	// Ensure that the correct file type is used and that the setting is active
	if (!m_editor->getEditorSetting(NumeReEditor::SETTING_USEANALYZER) || (m_editor->m_fileType != FILE_NSCR && m_editor->m_fileType != FILE_NPRC && m_editor->m_fileType != FILE_MATLAB && m_editor->m_fileType != FILE_CPP))
		return;

    // Determine the annotation style
	m_editor->AnnotationSetVisible(wxSTC_ANNOTATION_BOXED);

	m_nCurrentLine = -1;
	m_hasProcedureDefinition = false;

	bool isContinuedLine = false;
	bool isAlreadyMeasured = false;
	bool isSuppressed = false;

	m_sCurrentLine = "";
	m_sStyles = "";
	string sFirstLine = "";
	string sFirstStyles = "";
	int nFirstLine = -1;

	AnnotationCount AnnotCount;

	// Go through the whole file
	for (m_nCurPos = 0; m_nCurPos < m_editor->GetLastPosition(); m_nCurPos++)
	{
	    // Ignore comments
		if (m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, m_nCurPos) || m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, m_nCurPos))
			continue;

		// It's a new line? Then finalize the contents of the last line
		// and display the contents as annotation
		if (m_nCurrentLine < m_editor->LineFromPosition(m_nCurPos))
		{
		    // Get the line's contents
			string sLine = m_editor->GetLine(m_nCurrentLine).ToStdString();
			StripSpaces(sLine);

            // catch constant expressions
			if (m_options->GetAnalyzerOption(Options::CONSTANT_EXPRESSION) && sLine.length() && sLine.find_first_not_of("\n\r\t") != string::npos && sLine.find_first_not_of("0123456789eE+-*/.,;^(){} \t\r\n") == string::npos)
				AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sLine.substr(0, sLine.find_last_not_of("0123456789eE+-*/.,;^()")), m_sWarn, _guilang.get("GUI_ANALYZER_CONSTEXPR")), ANNOTATION_WARN);

			// Handle line continuations
			if (sLine.find("\\\\") != string::npos)
				isContinuedLine = true;
			else
			{
				isContinuedLine = false;
				m_hasProcedureDefinition = false;
			}

            // Find the summary line for the current file and push the
            // current lines contents, if m_editor is the first line,
            // otherwise simply add the current lines contents to the
            // current line
            if (nFirstLine < 0 && m_nCurrentLine >= 0)
            {
                sFirstLine = m_sCurrentLine;
                sFirstStyles = m_sStyles;
                nFirstLine = m_nCurrentLine;
            }
            else if (m_sCurrentLine.length())
            {
                // Write the annotion to the current line
                m_editor->AnnotationSetText(m_nCurrentLine, m_sCurrentLine);
                m_editor->AnnotationSetStyles(m_nCurrentLine, m_sStyles);
            }

			m_nCurrentLine = m_editor->LineFromPosition(m_nCurPos);

			// Get the new line for finding the trailing semicolon
			sLine = m_editor->GetLine(m_nCurrentLine).ToStdString();

			// Ensure that there's no trailing comment
			if (sLine.rfind("##") != string::npos)
                sLine.erase(sLine.rfind("##"));

            // Remove also block comments
            size_t nBlockStart = 0;

            while ((nBlockStart = sLine.find("#*")) != string::npos)
            {
                if (sLine.find("*#", nBlockStart+2) == string::npos)
                {
                    sLine.erase(nBlockStart);
                    break;
                }
                else
                {
                    sLine.erase(nBlockStart, sLine.find("*#", nBlockStart+2)+2 - nBlockStart);
                }
            }

			// Find the last visible character
			size_t lastVisibleChar = sLine.find_last_not_of(" \t\r\n");

			// Determine, whether it is a semicolon or a line continuation
			if (lastVisibleChar != string::npos
                && (sLine[lastVisibleChar] == ';' || (lastVisibleChar > 0 && sLine.substr(lastVisibleChar-1, 2) == "\\\\")))
                isSuppressed = true;
            else
                isSuppressed = false;

			m_sCurrentLine.clear();
			m_sStyles.clear();
		}

		// Get code metrics for scripts if not already done
		if (m_editor->m_fileType == FILE_NSCR && !isAlreadyMeasured)
		{
			string sLine = m_editor->GetLine(m_nCurrentLine).ToStdString();
			StripSpaces(sLine);
			if (sLine.length() && sLine.find_first_not_of(" \n\r\t") != string::npos)
			{
				string sSyntaxElement =  m_editor->GetFilenameString().ToStdString();
				isAlreadyMeasured = true;

				// Calculate the code metrics:
				// Complexity
				int nCyclomaticComplexity = calculateCyclomaticComplexity(m_nCurrentLine, m_editor->LineFromPosition(m_editor->GetLastPosition()));

				// LinesOfcode
				int nLinesOfCode = calculateLinesOfCode(m_nCurrentLine, m_editor->LineFromPosition(m_editor->GetLastPosition()));

				// Number of comments
				int nNumberOfComments = countNumberOfComments(m_nCurrentLine, m_editor->LineFromPosition(m_editor->GetLastPosition()));

				// comment density
				double dCommentDensity = (double)nNumberOfComments / (double)nLinesOfCode;

				// Compare the metrics with the contants and issue a note or a warning
				if (m_options->GetAnalyzerOption(Options::COMPLEXITY) && nCyclomaticComplexity > MAXCOMPLEXITYWARN)
					AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sWarn, _guilang.get("GUI_ANALYZER_HIGHCOMPLEXITY", toString(nCyclomaticComplexity))), ANNOTATION_WARN);
				else if ((m_options->GetAnalyzerOption(Options::COMPLEXITY) && nCyclomaticComplexity > MAXCOMPLEXITYNOTIFY) || m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS))
					AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sNote, _guilang.get("GUI_ANALYZER_HIGHCOMPLEXITY", toString(nCyclomaticComplexity))), ANNOTATION_NOTE);
				if ((m_options->GetAnalyzerOption(Options::LINES_OF_CODE) && nLinesOfCode > MAXLINESOFCODE) || m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS))
					AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sNote, _guilang.get("GUI_ANALYZER_MANYLINES", toString(nLinesOfCode))), ANNOTATION_NOTE);

				if ((m_options->GetAnalyzerOption(Options::COMMENT_DENSITY) && dCommentDensity < MINCOMMENTDENSITY) || (dCommentDensity < 1.0 && m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS)))
					AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sNote, _guilang.get("GUI_ANALYZER_LOWCOMMENTDENSITY", toString(dCommentDensity * 100.0, 3))), ANNOTATION_NOTE);

				if ((m_options->GetAnalyzerOption(Options::COMMENT_DENSITY) && dCommentDensity > MAXCOMMENTDENSITY)  || (dCommentDensity >= 1.0 && m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS)))
					AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sNote, _guilang.get("GUI_ANALYZER_HIGHCOMMENTDENSITY", toString(dCommentDensity * 100.0, 3))), ANNOTATION_NOTE);
			}
		}

		// Handle the different style types
		if (m_editor->isStyleType(NumeReEditor::STYLE_COMMAND, m_nCurPos))
		{
		    if (!isSuppressed)
            {
                string sWord = m_editor->GetTextRange(m_editor->WordStartPosition(m_nCurPos, true), m_editor->WordEndPosition(m_nCurPos, true)).ToStdString();

                // the commands "matop" or "mtrxop" are needing semicolons
                // and are the first element in a command line
                if (sWord != "matop" && sWord != "mtrxop")
                    isSuppressed = true;
            }
		    // Handle commands
		    AnnotCount += analyseCommands();
		}
		else if (m_editor->isStyleType(NumeReEditor::STYLE_FUNCTION, m_nCurPos)
           || ((m_editor->m_fileType == FILE_NSCR || m_editor->m_fileType == FILE_NPRC) && m_editor->GetStyleAt(m_nCurPos) == wxSTC_NSCR_METHOD))
		{
		    if (m_options->GetAnalyzerOption(Options::RESULT_SUPPRESSION) && !isSuppressed)
            {
                string sWord = m_editor->GetTextRange(m_editor->WordStartPosition(m_nCurPos, true), m_editor->WordEndPosition(m_nCurPos, true)).ToStdString();
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sWord + "()", m_editor->WordStartPosition(m_nCurPos, true), sWord.length()), m_sNote, _guilang.get("GUI_ANALYZER_SUPPRESS_OUTPUT")), ANNOTATION_NOTE);
                isSuppressed = true;
            }
		    // Handle standard functions
		    AnnotCount += analyseFunctions(isContinuedLine);
		}
		else if (m_editor->isStyleType(NumeReEditor::STYLE_PROCEDURE, m_nCurPos))
		{
		    // Handle NumeRe procedure calls (NumeRe only)
		    AnnotCount += analyseProcedures();
		}
		else if ((m_editor->isStyleType(NumeReEditor::STYLE_IDENTIFIER, m_nCurPos) || m_editor->isStyleType(NumeReEditor::STYLE_DATAOBJECT, m_nCurPos))
				 && m_editor->GetCharAt(m_nCurPos) != ' '
				 && m_editor->GetCharAt(m_nCurPos) != '\t'
				 && m_editor->GetCharAt(m_nCurPos) != '\r'
				 && m_editor->GetCharAt(m_nCurPos) != '\n')
		{
		    if (m_options->GetAnalyzerOption(Options::RESULT_SUPPRESSION) && !isSuppressed)
            {
                string sWord = m_editor->GetTextRange(m_editor->WordStartPosition(m_nCurPos, true), m_editor->WordEndPosition(m_nCurPos, true)).ToStdString();
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sWord, m_editor->WordStartPosition(m_nCurPos, true), sWord.length()), m_sNote, _guilang.get("GUI_ANALYZER_SUPPRESS_OUTPUT")), ANNOTATION_NOTE);
                isSuppressed = true;
            }
		    // Handle identifiers (like variable names)
		    AnnotCount += analyseIdentifiers();
		}
		else if (m_editor->isStyleType(NumeReEditor::STYLE_OPERATOR, m_nCurPos))
		{
		    if (m_options->GetAnalyzerOption(Options::RESULT_SUPPRESSION) && m_editor->GetCharAt(m_nCurPos) == '=' && !isSuppressed)
            {
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence("=", m_nCurPos, 1), m_sNote, _guilang.get("GUI_ANALYZER_SUPPRESS_OUTPUT")), ANNOTATION_NOTE);
                isSuppressed = true;
            }
		    // Handle special operators
		    AnnotCount += analyseOperators();
		}
		else if (m_editor->isStyleType(NumeReEditor::STYLE_NUMBER, m_nCurPos))
		{
		    // Handle numbers
		    AnnotCount += analyseNumbers();
		}
	}

	// Clear the annotation and style cache
	m_sCurrentLine.clear();
	m_sStyles.clear();

	// Write the summary lines
	if (AnnotCount.nNotes)
		addToAnnotation(_guilang.get("GUI_ANALYZER_NOTE_TOTAL", toString(AnnotCount.nNotes)), ANNOTATION_NOTE);

	if (AnnotCount.nWarnings)
		addToAnnotation(_guilang.get("GUI_ANALYZER_WARN_TOTAL", toString(AnnotCount.nWarnings)), ANNOTATION_WARN);

	if (AnnotCount.nErrors)
		addToAnnotation(_guilang.get("GUI_ANALYZER_ERROR_TOTAL", toString(AnnotCount.nErrors)), ANNOTATION_ERROR);

    // Append the summary line to the first line (if it is not empty)
	if (m_sCurrentLine.length() && sFirstLine.length())
	{
		m_sCurrentLine += "\n" + sFirstLine;
		m_sStyles += m_sStyles.back() + sFirstStyles;
	}
	else if (sFirstLine.length())
	{
		m_sCurrentLine = sFirstLine;
		m_sStyles = sFirstStyles;
	}

	// Write the first line if it is not empty
	if (m_sCurrentLine.length())
	{
		m_editor->AnnotationSetText(nFirstLine, m_sCurrentLine);
		m_editor->AnnotationSetStyles(nFirstLine, m_sStyles);
	}
}


/////////////////////////////////////////////////
/// \brief Analyses commands in the code
///
/// \return AnnotationCount
///
/// This member function analyses syntax elements, which are highlighted as commands
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::analyseCommands()
{
    AnnotationCount AnnotCount;

    bool canContinue = false;
    int wordstart = m_editor->WordStartPosition(m_nCurPos, true);
    int wordend = m_editor->WordEndPosition(m_nCurPos, true);

	// Get the current syntax element
    string sSyntaxElement = m_editor->GetTextRange(wordstart, wordend).ToStdString();

    // add a message to "throw"
    if (sSyntaxElement == "throw")
    {
        for (int j = wordend; j < m_editor->GetLineEndPosition(m_nCurrentLine); j++)
        {
            if (m_editor->GetStyleAt(j) == wxSTC_NSCR_STRING || m_editor->GetStyleAt(j) == wxSTC_NSCR_STRING_PARSER)
            {
                canContinue = true;
                break;
            }
        }

        // Was a message found?
        if (!canContinue)
        {
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, 5), m_sNote, _guilang.get("GUI_ANALYZER_THROW_ADDMESSAGE")), ANNOTATION_NOTE);
        }
    }

    // check the namespace command
    if (sSyntaxElement == "namespace")
    {
        string sArgs = m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();
        StripSpaces(sArgs);

        // Is there an explicit namespace name? If no, warn the user
        if (!sArgs.length())
        {
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sWarn, _guilang.get("GUI_ANALYZER_NAMESPACE_ALWAYSMAIN")), ANNOTATION_WARN);
        }
        m_nCurPos = wordend;

        // Advance the character pointer and return the number of gathered annotations
        while (m_editor->GetCharAt(m_nCurPos) != ';' && m_editor->GetCharAt(m_nCurPos) != '\r' && m_editor->GetCharAt(m_nCurPos) != '\n')
            m_nCurPos++;
        return AnnotCount;
    }

    // The progress command needs extra runtime (2-4 times). Inform the user about m_editor issue
    if (m_options->GetAnalyzerOption(Options::PROGRESS_RUNTIME) && sSyntaxElement == "progress")
    {
        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sNote, _guilang.get("GUI_ANALYZER_PROGRESS_RUNTIME")), ANNOTATION_NOTE);
    }

    // The install or the start commands are not allowed in scripts and procedures
    if (sSyntaxElement == "install" || sSyntaxElement == "uninstall" || sSyntaxElement == "start")
    {
        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_NOTALLOWED")), ANNOTATION_ERROR);
    }

    // Handle the memory clearance commands
    if (sSyntaxElement == "clear" || sSyntaxElement == "delete" || sSyntaxElement == "remove")
    {
        // some caches may not be removec
        if (sSyntaxElement == "remove" && m_editor->GetStyleAt(m_editor->WordStartPosition(wordend + 1, true)) == wxSTC_NSCR_PREDEFS)
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_CANNOTREMOVEPREDEFS")), ANNOTATION_ERROR);

        // Get everything after the clearance command
        string sArgs = m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();

        // Inform the user that he should append "ignore" as parameter
        if (!findParameter(sArgs, "ignore")
                && !findParameter(sArgs, "i")
                && (sSyntaxElement != "remove" || m_editor->GetStyleAt(m_editor->WordStartPosition(wordend + 1, true)) != wxSTC_NSCR_CUSTOM_FUNCTION))
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sNote, _guilang.get("GUI_ANALYZER_APPENDIGNORE")), ANNOTATION_NOTE);
    }

    // check, whether the current command do have an expression
    // Ignore some special commands, which do not need an expression
    if (sSyntaxElement != "hline"
            && sSyntaxElement != "continue"
            && sSyntaxElement != "break"
            && sSyntaxElement != "else"
            && sSyntaxElement != "end"
            && sSyntaxElement != "endif"
            && sSyntaxElement != "endfor"
            && sSyntaxElement != "endwhile"
            && sSyntaxElement != "endswitch"
            && sSyntaxElement != "endprocedure"
            && sSyntaxElement != "endcompose"
            && sSyntaxElement != "endlayout"
            && sSyntaxElement != "endgroup"
            && sSyntaxElement != "about"
            && sSyntaxElement != "abort"
            && sSyntaxElement != "compose"
            && sSyntaxElement != "layout"
            && sSyntaxElement != "group"
            && sSyntaxElement != "help"
            && sSyntaxElement != "quit"
            && sSyntaxElement != "return"
            && sSyntaxElement != "subplot"
            && sSyntaxElement != "try"
            && sSyntaxElement != "catch"
            && sSyntaxElement != "otherwise"
            && sSyntaxElement != "throw"
            && sSyntaxElement != "namespace" //warning
       )
    {
        canContinue = false;

        // Get everything in the current line after the command
        string sArgs = m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();
        StripSpaces(sArgs);

        // If the line after the command is empty
        if (!sArgs.length())
        {
            // is used as a parameter (legacy)
            for (int j = wordstart; j >= m_editor->PositionFromLine(m_nCurrentLine); j--)
            {
                if (m_editor->GetCharAt(j) == '-')
                    canContinue = true;
            }

            // No expression found and not used as a parameter?
            if (!canContinue)
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_EMPTYEXPRESSION")), ANNOTATION_ERROR);
        }
    }

    // There are some command, which will return values.
    // Check, whether there results are stored into a variable
    if (sSyntaxElement == "zeroes"
            || sSyntaxElement == "extrema"
            || sSyntaxElement == "integrate"
            || sSyntaxElement == "load"
            || sSyntaxElement == "reload"
            || sSyntaxElement == "append"
            || sSyntaxElement == "imread"
            || sSyntaxElement == "eval"
            || sSyntaxElement == "get"
            || sSyntaxElement == "read"
            || sSyntaxElement == "pulse"
            || sSyntaxElement == "diff")
    {
        canContinue = false;

        // Try to find an assignment operator
        for (int j = m_editor->PositionFromLine(m_nCurrentLine); j < wordstart; j++)
        {
            if (m_editor->GetCharAt(j) == '=')
            {
                canContinue = true;
                break;
            }
        }

        // Was an assignment operator found?
        if (m_options->GetAnalyzerOption(Options::RESULT_ASSIGNMENT) && !canContinue)
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sWarn, _guilang.get("GUI_ANALYZER_ASSIGNTOVARIABLE", sSyntaxElement)), ANNOTATION_WARN);
    }

    // Examine the if, elseif and while commands
    if (sSyntaxElement == "if" || sSyntaxElement == "elseif" || sSyntaxElement == "while" || sSyntaxElement == "switch")
    {
        for (int j = wordend; j < m_editor->GetLineEndPosition(m_nCurrentLine); j++)
        {
            // Only examine elements in the first parenthesis for NumeRe syntax.
            // Ignore the parenthesis in the MATLAB case
            if (m_editor->GetCharAt(j) == '(' || m_editor->m_fileType == FILE_MATLAB)
            {
                int nPos;

                // If the first character is a parenthesis
                if (m_editor->GetCharAt(j) == '(')
                {
                    nPos = m_editor->BraceMatch(j);
                    if (nPos < 0)
                    {
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j, 1), m_sError, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_ERROR);
                        break;
                    }
                }
                else
                    nPos = m_editor->GetLineEndPosition(m_nCurrentLine);

                // Get the argument
                string sArgument = m_editor->GetTextRange(j + 1, nPos).ToStdString();
                StripSpaces(sArgument);

                // Is the argument available?
                if (!sArgument.length())
                {
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j, 2), m_sError, _guilang.get("GUI_ANALYZER_MISSINGARGUMENT")), ANNOTATION_ERROR);
                    break;
                }

                // Is it a constant?
                if (sArgument == "true" || (sArgument.find_first_not_of("1234567890.") == string::npos && sArgument != "0"))
                {
                    if (sSyntaxElement == "while")
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()), m_sWarn, _guilang.get("GUI_ANALYZER_WHILE_ALWAYSTRUE")), ANNOTATION_WARN);
                    else if (sSyntaxElement == "switch")
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()), m_sWarn, _guilang.get("GUI_ANALYZER_SWITCH_CONSTANT")), ANNOTATION_WARN);
                    else
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()), m_sWarn, _guilang.get("GUI_ANALYZER_IF_ALWAYSTRUE")), ANNOTATION_WARN);
                }
                else if (sArgument == "false" || sArgument == "0")
                {
                    if (sSyntaxElement == "while")
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()), m_sWarn, _guilang.get("GUI_ANALYZER_WHILE_ALWAYSFALSE")), ANNOTATION_WARN);
                    else if (sSyntaxElement == "switch")
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()), m_sWarn, _guilang.get("GUI_ANALYZER_SWITCH_CONSTANT")), ANNOTATION_WARN);
                    else
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()), m_sWarn, _guilang.get("GUI_ANALYZER_IF_ALWAYSFALSE")), ANNOTATION_WARN);
                }
                else if (containsAssignment(sArgument))
                {
                    // Does it contain an assignment? Warn the user as m_editor is probably not intendet
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()), m_sWarn, _guilang.get("GUI_ANALYZER_ASSIGNMENTINARGUMENT")), ANNOTATION_WARN);
                }
                break;
            }
        }

        // There's an faster, inline if-else operator in NumeRe
        // Propose that, if the current if-else block is quite short
        if (sSyntaxElement == "if" && m_editor->m_fileType != FILE_MATLAB)
        {
            vector<int> vBlock = m_editor->BlockMatch(m_nCurPos);

            // Was the end of the current block found?
            if (vBlock.back() != wxSTC_INVALID_POSITION)
            {
                // Check the length of the current block
                if (m_editor->LineFromPosition(vBlock.back()) - m_nCurrentLine < 5)
                {
                    canContinue = false;

                    // Ensure that no commands except of further ifs and elses are used inside of the found block
                    for (int pos = wordend; pos <= vBlock.back(); pos++)
                    {
                        if (m_editor->GetStyleAt(pos) == wxSTC_NSCR_COMMAND
                                && m_editor->GetTextRange(m_editor->WordStartPosition(pos, true), m_editor->WordEndPosition(pos, true)) != "if"
                                && m_editor->GetTextRange(m_editor->WordStartPosition(pos, true), m_editor->WordEndPosition(pos, true)) != "else"
                                && m_editor->GetTextRange(m_editor->WordStartPosition(pos, true), m_editor->WordEndPosition(pos, true)) != "endif")
                        {
                            // Other command found
                            canContinue = true;
                            break;
                        }
                        else
                        {
                            // jump over other elements
                            pos = m_editor->WordEndPosition(pos, true);
                        }
                    }

                    // Was an other command found?
                    if (m_options->GetAnalyzerOption(Options::INLINE_IF) && !canContinue)
                    {
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sNote, _guilang.get("GUI_ANALYZER_USEINLINEIF")), ANNOTATION_NOTE);
                    }
                }
            }
        }

        // Ensure that a switch does contain at least one case
        if (sSyntaxElement == "switch")
        {
            vector<int> vBlock = m_editor->BlockMatch(m_nCurPos);

            // Examine each match
            for (size_t i = 1; i < vBlock.size(); i++)
            {
                // Invalid position -> missing end. Already handled elsewhere
                if (vBlock[i] == wxSTC_INVALID_POSITION)
                    break;

                // Examine the command
                if (m_editor->GetTextRange(vBlock[i], m_editor->WordEndPosition(vBlock[i], true)) == "case")
                {
                    // Contains a case: everything is alright
                    break;
                }
                else if (m_editor->GetTextRange(vBlock[i], m_editor->WordEndPosition(vBlock[i], true)) == "default")
                {
                    // Only a default statement?
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sWarn, _guilang.get("GUI_ANALYZER_SWITCH_ONLY_DEFAULT")), ANNOTATION_WARN);
                    break;
                }
                else if (m_editor->GetTextRange(vBlock[i], m_editor->WordEndPosition(vBlock[i], true)) == "endswitch")
                {
                    // Neither a case nor a default statement?
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_SWITCH_MISSING_CASE")), ANNOTATION_ERROR);
                    break;
                }
            }
        }
    }

    // Check for fallthroughs and warn the user,
    // if one was found. Also check the appended value.
    if (sSyntaxElement == "case" && m_editor->m_fileType != FILE_MATLAB)
    {
        vector<int> vBlock = m_editor->BlockMatch(m_nCurPos);

        // Search the correct position in the
        // whole block
        for (size_t i = 0; i < vBlock.size(); i++)
        {
            // Is m_editor the correct position?
            if (vBlock[i] == wordstart)
            {
                if (vBlock[i+1] != wxSTC_INVALID_POSITION && m_editor->GetTextRange(vBlock[i+1], m_editor->WordEndPosition(vBlock[i+1], true)) != "endswitch")
                {
                    // Search all occurences of the "break"
                    // command between the two statements
                    vector<int> vMatches = m_editor->m_search->FindAll("break", wxSTC_NSCR_COMMAND, wordend, vBlock[i+1], false);

                    // Ensure that there's one "break"
                    // statement
                    if (m_options->GetAnalyzerOption(Options::SWITCH_FALLTHROUGH) && !vMatches.size())
                    {
                        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sWarn, _guilang.get("GUI_ANALYZER_SWITCH_MISSING_BREAK")), ANNOTATION_WARN);
                    }
                }

                break;
            }
        }

        // Check the value of the case
        wxString sLine = m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine));

        // Does the value contain something, which is not a whitespace
        // and not a comment?
        if (sLine.find_first_not_of(":\r\n#* ") == string::npos || sLine.find(':') == string::npos)
        {
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_SWITCH_MISSING_VALUE")), ANNOTATION_ERROR);
        }

    }

    // Examine the for command
    // Only used for NumeRe syntax
    if (sSyntaxElement == "for" && m_editor->m_fileType != FILE_MATLAB)
    {
        // Go through the current line
        for (int j = wordend; j < m_editor->GetLineEndPosition(m_nCurrentLine); j++)
        {
            // If the current character is an opening parenthesis
            if (m_editor->GetCharAt(j) == '(')
            {
                // Examine the argument
                int nPos = m_editor->BraceMatch(j);
                if (nPos < 0)
                {
                    // Missing parenthesis
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j, 1), m_sError, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_ERROR);
                    break;
                }

                // Get the argument from the parenthesis
                string sArgument = m_editor->GetTextRange(j + 1, nPos).ToStdString();
                StripSpaces(sArgument);


                // Argument is empty?
                if (!sArgument.length())
                {
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j, 2), m_sError, _guilang.get("GUI_ANALYZER_MISSINGARGUMENT")), ANNOTATION_ERROR);
                    break;
                }

                // Important parts of the argument are missing?
                if (sArgument.find(':') == string::npos || sArgument.find('=') == string::npos)
                {
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()), m_sError, _guilang.get("GUI_ANALYZER_FOR_INTERVALERROR")), ANNOTATION_ERROR);
                }

                // Store the for index variable in the list of known
                // local variables
                if (m_editor->m_fileType == FILE_NPRC)
                {
                    for (int i = j+1; i < nPos; i++)
                    {
                        if (m_editor->GetStyleAt(i) == wxSTC_NPRC_IDENTIFIER || m_editor->GetStyleAt(i) == wxSTC_NPRC_CUSTOM_FUNCTION || m_editor->GetStyleAt(i) == wxSTC_NPRC_CLUSTER)
                        {
                            // Store it and break directly
                            m_vLocalVariables.push_back(pair<string,int>(m_editor->GetTextRange(m_editor->WordStartPosition(i, true), m_editor->WordEndPosition(i, true)).ToStdString(), m_editor->GetStyleAt(i)));
                            break;
                        }
                    }
                }

                break;
            }
        }
    }

    // Examine the current usage of the local variable declarators
    // Esp. ensure that the declared variables are used
    if (m_editor->m_fileType == FILE_NPRC && (sSyntaxElement == "var" || sSyntaxElement == "str" || sSyntaxElement == "tab" || sSyntaxElement == "cst"))
    {
        // Handle the special case "list -var"
        if (sSyntaxElement == "var" && m_editor->GetTextRange(m_editor->PositionFromLine(m_nCurrentLine), m_editor->GetLineEndPosition(m_nCurrentLine)).find("list") < (size_t)(wordstart - m_editor->PositionFromLine(m_nCurrentLine)))
        {
            m_nCurPos = wordend;
            return AnnotCount;
        }

        // Get the next line
        int nNextLineStartPosition = m_editor->GetLineEndPosition(m_nCurrentLine) + 1;

        // Find the end of the current procedure
        int nProcedureEndPosition = m_editor->FindText(nNextLineStartPosition, m_editor->GetLastPosition(), "endprocedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD);

        int nStyle = wxSTC_NSCR_IDENTIFIER;

        if (sSyntaxElement == "tab")
            nStyle = wxSTC_NSCR_CUSTOM_FUNCTION;
        else if (sSyntaxElement == "cst")
            nStyle = wxSTC_NSCR_CLUSTER;

        // extract the arguments and strip the spaces
        string sArgs = m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();
        StripSpaces(sArgs);

        // Ensure that the end of the procedure is available
        if (nProcedureEndPosition == -1)
        {
            nProcedureEndPosition = m_editor->GetLastPosition();
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_MISSINGENDPROCEDURE")), ANNOTATION_ERROR);
        }

        // If there are variables available
        if (sArgs.length())
        {
            string currentArg;

            // Extract variable by variable
            while (getNextArgument(sArgs, false).length())
            {
                currentArg = getNextArgument(sArgs, true);

                // remove assignments and parentheses and strip the spaces
                if (currentArg.find('=') != string::npos)
                    currentArg.erase(currentArg.find('='));

                if (currentArg.find_first_of("({") != string::npos)
                    currentArg.erase(currentArg.find_first_of("({"));

                StripSpaces(currentArg);

                if (!currentArg.length())
                    continue;

                m_vLocalVariables.push_back(pair<string,int>(currentArg, nStyle));

                // Try to find the variable in the remaining code
                if (m_options->GetAnalyzerOption(Options::UNUSED_VARIABLES) && !m_editor->m_search->FindAll(currentArg, nStyle, nNextLineStartPosition, nProcedureEndPosition, false).size())//   m_editor->FindText(nNextLine, nProcedureEnd, currentArg, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD) == -1)
                {
                    // No variable found
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(currentArg, m_editor->FindText(wordstart, nProcedureEndPosition, currentArg, wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD), currentArg.length()), m_sWarn, _guilang.get("GUI_ANALYZER_UNUSEDVARIABLE", currentArg)), ANNOTATION_WARN);
                }
            }
        }
        else // No varibles are available
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_NOVARIABLES")), ANNOTATION_ERROR);
    }

    // Examine definitions
    if (m_editor->m_fileType == FILE_NPRC && m_options->GetAnalyzerOption(Options::GLOBAL_VARIABLES) && (sSyntaxElement == "define" || sSyntaxElement == "ifndefined" || sSyntaxElement == "lclfunc" || sSyntaxElement == "def" || sSyntaxElement == "ifndef"))
    {
        // extract the definition and strip the spaces
        string sDefinition = m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)).ToStdString();
        while (sDefinition.back() == '\r' || sDefinition.back() == '\n')
            sDefinition.pop_back();
        StripSpaces(sDefinition);

        // If there is a declaration available
        if (sDefinition.length())
        {
            // remove assignments and parentheses and strip the spaces
            if (sDefinition.find('=') != string::npos)
                sDefinition.erase(sDefinition.find('='));

            if (sDefinition.find_first_of("({") != string::npos)
                sDefinition.erase(sDefinition.find_first_of("({"));

            StripSpaces(sDefinition);

            m_vLocalVariables.push_back(pair<string,int>(sDefinition, wxSTC_NSCR_CUSTOM_FUNCTION));

            // increment the position variable to the last position
            // in the current line, so that we may jump over the definition
            // in the following
            m_nCurPos = m_editor->GetLineEndPosition(m_editor->LineFromPosition(m_nCurPos));
            return AnnotCount;
        }
    }

    // Examine the procedure / MATLAB function starting at m_editor position
    // This includes esp. the calculation of the standard coding metrics
    if ((m_editor->m_fileType == FILE_NPRC && sSyntaxElement == "procedure") || (m_editor->m_fileType == FILE_MATLAB && sSyntaxElement == "function"))
    {
        // Use the block match function, which is capable of doing both: NumeRe and MATLAB syntax
        vector<int> vBlock = m_editor->BlockMatch(m_nCurPos);
        m_hasProcedureDefinition = true;

        // If the current file is a procedure file, then decode the
        // argument list and store it in the list of known local
        // variables
        if (m_editor->m_fileType == FILE_NPRC)
        {
            int nArgumentParensStart = m_editor->FindText(m_nCurPos, m_editor->GetLineEndPosition(m_nCurrentLine), "(");
            int nArgumentParensEnd = m_editor->BraceMatch(nArgumentParensStart);

            if (nArgumentParensStart != -1 && nArgumentParensEnd != -1)
            {
                // Decode the list
                for (int i = nArgumentParensStart+1; i < nArgumentParensEnd; i++)
                {
                    if (m_editor->GetStyleAt(i) == wxSTC_NPRC_IDENTIFIER || m_editor->GetStyleAt(i) == wxSTC_NPRC_CUSTOM_FUNCTION || m_editor->GetStyleAt(i) == wxSTC_NPRC_CLUSTER)
                    {
                        m_vLocalVariables.push_back(pair<string,int>(m_editor->GetTextRange(m_editor->WordStartPosition(i, true), m_editor->WordEndPosition(i, true)).ToStdString(), m_editor->GetStyleAt(i)));
                        i = m_editor->WordEndPosition(i, true);
                    }
                }
            }
        }

        if (vBlock.back() != wxSTC_INVALID_POSITION)
        {
            int nProcedureEnd = vBlock.back();

            // This is only needed for NumeRe procedures
            if (m_editor->m_fileType == FILE_NPRC)
            {
                // check the name of the procedure - is there a naming procedure?
                int nNamingProcedure = m_editor->m_search->FindNamingProcedure();

                if (nNamingProcedure == wxNOT_FOUND)
                {
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sError, _guilang.get("GUI_ANALYZER_NONAMINGPROCEDURE")), ANNOTATION_ERROR);
                }
                else if (m_options->GetAnalyzerOption(Options::THISFILE_NAMESPACE) && nNamingProcedure != m_nCurrentLine)
                {
                    AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sWarn, _guilang.get("GUI_ANALYZER_THISFILEPROCEDURE")), ANNOTATION_WARN);
                }
            }
            // Calculate the code metrics:
            // Complexity
            int nCyclomaticComplexity = calculateCyclomaticComplexity(m_nCurrentLine, m_editor->LineFromPosition(nProcedureEnd));

            // LinesOfCode
            int nLinesOfCode = calculateLinesOfCode(m_nCurrentLine, m_editor->LineFromPosition(nProcedureEnd)) - 2;

            // Number of comments
            int nNumberOfComments = countNumberOfComments(m_nCurrentLine, m_editor->LineFromPosition(nProcedureEnd));

            // Comment density
            double dCommentDensity = (double)nNumberOfComments / (double)nLinesOfCode;

            // Compare the metrics with the contants and issue a note or a warning
            if (m_options->GetAnalyzerOption(Options::PROCEDURE_LENGTH) && nLinesOfCode < 3)
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sWarn, _guilang.get("GUI_ANALYZER_INLINING")), ANNOTATION_WARN);

            if (m_options->GetAnalyzerOption(Options::COMPLEXITY) && nCyclomaticComplexity > MAXCOMPLEXITYWARN)
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sWarn, _guilang.get("GUI_ANALYZER_HIGHCOMPLEXITY", toString(nCyclomaticComplexity))), ANNOTATION_WARN);
            else if ((m_options->GetAnalyzerOption(Options::COMPLEXITY) && nCyclomaticComplexity > MAXCOMPLEXITYNOTIFY) || m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS))
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sNote, _guilang.get("GUI_ANALYZER_HIGHCOMPLEXITY", toString(nCyclomaticComplexity))), ANNOTATION_NOTE);

            if ((m_options->GetAnalyzerOption(Options::LINES_OF_CODE) && nLinesOfCode > MAXLINESOFCODE) || m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS))
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sNote, _guilang.get("GUI_ANALYZER_MANYLINES", toString(nLinesOfCode))), ANNOTATION_NOTE);

            if ((m_options->GetAnalyzerOption(Options::COMMENT_DENSITY) && dCommentDensity < MINCOMMENTDENSITY) || (dCommentDensity < 1.0 && m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS)))
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sNote, _guilang.get("GUI_ANALYZER_LOWCOMMENTDENSITY", toString(dCommentDensity * 100.0, 3))), ANNOTATION_NOTE);

            if ((m_options->GetAnalyzerOption(Options::COMMENT_DENSITY) && dCommentDensity > MAXCOMMENTDENSITY) || (dCommentDensity >= 1.0 && m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS)))
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", sSyntaxElement, m_sNote, _guilang.get("GUI_ANALYZER_HIGHCOMMENTDENSITY", toString(dCommentDensity * 100.0, 3))), ANNOTATION_NOTE);
        }
    }

    // Handle the "return" command in procedures (not needed in MATLAB)
    if (m_editor->m_fileType == FILE_NPRC && sSyntaxElement == "return")
    {
        // Try to find the end of the current procedure
        int nProcedureEnd = m_editor->FindText(m_nCurPos, m_editor->GetLastPosition(), "endprocedure", wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD);

        // Get the argument of the return command and strip the spaces
        string sArgs = m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)).ToStdString();
        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();
        StripSpaces(sArgs);

        // Ensure that the end of the procedure was found
        if (nProcedureEnd == -1)
        {
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_MISSINGENDPROCEDURE")), ANNOTATION_ERROR);
        }

        // Examine the argument
        if (sArgs.length())
        {
            // Inform the user to add an semicolon to the arguments, if he uses something else than "void"
            if (m_options->GetAnalyzerOption(Options::RESULT_SUPPRESSION) && sArgs.back() != ';' && sArgs != "void")
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordend + 1, sArgs.length()), m_sNote, _guilang.get("GUI_ANALYZER_RETURN_ADDSEMICOLON")), ANNOTATION_NOTE);
        }
        else
        {
            // Inform the user that the return value will always be "true", if he doesn't append a value
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sNote, _guilang.get("GUI_ANALYZER_RETURN_ALWAYSTRUE")), ANNOTATION_NOTE);
        }
    }

    // Handle blocks with their corresponding end
    if (sSyntaxElement == "if"
            || sSyntaxElement == "end"
            || sSyntaxElement == "elseif"
            || sSyntaxElement == "else"
            || sSyntaxElement == "endif"
            || sSyntaxElement == "for"
            || sSyntaxElement == "endfor"
            || sSyntaxElement == "while"
            || sSyntaxElement == "endwhile"
            || sSyntaxElement == "switch"
            || sSyntaxElement == "endswitch"
            || sSyntaxElement == "compose"
            || sSyntaxElement == "endcompose"
            || sSyntaxElement == "layout"
            || sSyntaxElement == "endlayout"
            || sSyntaxElement == "group"
            || sSyntaxElement == "endgroup"
            || sSyntaxElement == "function"
            || sSyntaxElement == "procedure"
            || sSyntaxElement == "endprocedure")
    {
        // Try to find the matching block parts
        vector<int> vMatch = m_editor->BlockMatch(m_nCurPos);

        if (vMatch.size() > 1)
        {
            // If there's an invalid position, m_editor means that the current block is unfinished
            if (vMatch.front() == wxSTC_INVALID_POSITION || vMatch.back() == wxSTC_INVALID_POSITION)
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sError, _guilang.get("GUI_ANALYZER_UNFINISHEDBLOCK")), ANNOTATION_ERROR);
        }

        // Clear the list of local variables
        if (sSyntaxElement == "endprocedure")
            m_vLocalVariables.clear();

        // Remove the last declared local variable
        if (sSyntaxElement == "endfor" && m_editor->m_fileType == FILE_NPRC && m_vLocalVariables.size())
            m_vLocalVariables.pop_back();
    }
    m_nCurPos = wordend;

    // Return the counted annotations
    return AnnotCount;
}


/////////////////////////////////////////////////
/// \brief Analyses calls to standard functions
///
/// \param isContinuedLine bool Has to be true, if the current line is continued from above
/// \return AnnotationCount
///
/// This member function analyses syntax elements, which are highlighted as functions
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::analyseFunctions(bool isContinuedLine)
{
    AnnotationCount AnnotCount;

    bool canContinue = false;
    int wordstart = m_editor->WordStartPosition(m_nCurPos, true);
    int wordend = m_editor->WordEndPosition(m_nCurPos, true);

    // Get the corresponding syntax element
    string sSyntaxElement = m_editor->GetTextRange(wordstart, wordend).ToStdString();

    // Handle method (modifier) calls, also appends a pair of parentheses if needed
    if ((m_editor->m_fileType == FILE_NSCR || m_editor->m_fileType == FILE_NPRC) && m_editor->GetStyleAt(m_nCurPos) == wxSTC_NSCR_METHOD)
    {
        // ignore modifiers, i.e. method without parentheses
        string sModifier = ",len,cols,lines,grid,avg,std,min,max,med,sum,prd,cnt,num,norm,and,or,xor,";
        if (sModifier.find("," + sSyntaxElement + ",") == string::npos)
            sSyntaxElement += "()";
        sSyntaxElement.insert(0, "VAR.");
    }
    else
        sSyntaxElement += "()";

    // Is the current function called without a target variable?
    if (m_options->GetAnalyzerOption(Options::RESULT_ASSIGNMENT) && m_editor->PositionFromLine(m_nCurrentLine) == wordstart && !isContinuedLine && sSyntaxElement != "sleep()")
    {
        // The function is called at the first position without a target variable
        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart), m_sWarn, _guilang.get("GUI_ANALYZER_ASSIGNTOVARIABLE", sSyntaxElement)), ANNOTATION_WARN);
    }
    else if (sSyntaxElement != "sleep()")
    {
        // Try to find a assignment operator before the function
        // Other possibilities are commands and procedure calls
        for (int j = m_editor->PositionFromLine(m_nCurrentLine); j < wordstart; j++)
        {
            if (m_editor->GetCharAt(j) == '=' || m_editor->isStyleType(NumeReEditor::STYLE_COMMAND, j) || m_editor->isStyleType(NumeReEditor::STYLE_PROCEDURE, j) || m_editor->isStyleType(NumeReEditor::STYLE_DATAOBJECT, j))
            {
                canContinue = true;
                break;
            }
        }

        // Was an operator or a command found?
        if (m_options->GetAnalyzerOption(Options::RESULT_ASSIGNMENT) && !canContinue && !isContinuedLine)
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart), m_sWarn, _guilang.get("GUI_ANALYZER_ASSIGNTOVARIABLE", sSyntaxElement)), ANNOTATION_WARN);
    }

    // There's a missing parenthesis?
    if (m_editor->BraceMatch(wordend) < 0 && sSyntaxElement.find('(') != string::npos)
    {
        // MATLAB doesn't require a parenthesis pair for empty arguments.
        // However, issue a warning as it is good practice to visually distinguish between variables and functions
        if (m_editor->m_fileType == FILE_MATLAB)
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart), m_sWarn, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_WARN);
        else
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart), m_sError, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_ERROR);
    }
    else if (sSyntaxElement != "time()" && sSyntaxElement != "clock()" && sSyntaxElement != "version()" && sSyntaxElement.find('(') != string::npos)
    {
        // Check for missing arguments
        int nPos = m_editor->BraceMatch(wordend);
        string sArgument = m_editor->GetTextRange(wordend + 1, nPos).ToStdString();
        StripSpaces(sArgument);
        if (!sArgument.length())
        {
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordend, 2), m_sError, _guilang.get("GUI_ANALYZER_MISSINGARGUMENT")), ANNOTATION_ERROR);
        }
    }
    m_nCurPos = wordend;

    // return the counted number of gathered annotations
    return AnnotCount;
}


/////////////////////////////////////////////////
/// \brief Analyses calls to procedures
///
/// \return AnnotationCount
///
/// This member function analyses syntax elements, which are highlighted as procedure calls
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::analyseProcedures()
{
    AnnotationCount AnnotCount;

    int nProcStart = m_nCurPos;

    // Try to find the current procedure call
    string sSyntaxElement = m_editor->m_search->FindMarkedProcedure(m_nCurPos).ToStdString();
    if (!sSyntaxElement.length())
        return AnnotCount;

    // Advance the character pointer until the style type changes
    while (m_editor->isStyleType(NumeReEditor::STYLE_PROCEDURE, m_nCurPos + 1))
        m_nCurPos++;

    // Try to find the correspondung procedure definition
    if (!m_editor->m_search->FindProcedureDefinition().length())
    {
        // Procedure definition was not found
        AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, nProcStart, m_nCurPos-nProcStart+1), m_sError, _guilang.get("GUI_ANALYZER_PROCEDURENOTFOUND", sSyntaxElement)), ANNOTATION_ERROR);
    }

    // return the number of gathered annotations
    return AnnotCount;
}


/////////////////////////////////////////////////
/// \brief Analyses occurences of identifiers.
///
/// \return AnnotationCount
///
/// This member function analyses syntax elements, which are highlighted as identifiers (aka variable names)
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::analyseIdentifiers()
{
    AnnotationCount AnnotCount;

    int wordstart = m_editor->WordStartPosition(m_nCurPos, true);
    int wordend = m_editor->WordEndPosition(m_nCurPos, true);

    // Shift the word end position, if the following character is a dot
    if (m_editor->GetCharAt(wordend) == '.' && m_editor->GetStyleAt(wordend + 1) != wxSTC_NSCR_METHOD)
        wordend = m_editor->WordEndPosition(wordend + 1, true);

    // Get the corresponding syntax element
    string sSyntaxElement = m_editor->GetTextRange(wordstart, wordend).ToStdString();

    // Warn about global variables
    if (m_options->GetAnalyzerOption(Options::GLOBAL_VARIABLES) && m_editor->m_fileType == FILE_NPRC)
    {
        bool bOK = false;

        // Try to find the current identifier in the list
        // of known local variables
        for (size_t i = 0; i < m_vLocalVariables.size(); i++)
        {
            if (m_vLocalVariables[i].first == sSyntaxElement && m_vLocalVariables[i].second == m_editor->GetStyleAt(m_nCurPos))
            {
                bOK = true;
                break;
            }
        }

        // nothing found
        if (!bOK)
        {
            wxString currentline = m_editor->GetLine(m_nCurrentLine);

            // Ignore y* variables from odesolve
            if (currentline.substr(currentline.find_first_not_of(" \t"), 9) != "odesolve " || !(sSyntaxElement[0] == 'y' && sSyntaxElement.length() > 1 && sSyntaxElement.find_first_not_of("0123456789", 1) == string::npos))
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sWarn, _guilang.get("GUI_ANALYZER_GLOBALVARIABLE", sSyntaxElement)), ANNOTATION_WARN);
        }
    }

    // Return, if the current identifier is a data object
    if (m_editor->isStyleType(NumeReEditor::STYLE_DATAOBJECT, m_nCurPos))
    {
        m_nCurPos = wordend;
        return AnnotCount;
    }

    // Handle very short variable names
    if (sSyntaxElement.length() < 4 && sSyntaxElement.length() > 1 && sSyntaxElement.find_first_not_of("\r\n") != string::npos && sSyntaxElement.find('.') == string::npos)
    {
        // Too short
        if (m_options->GetAnalyzerOption(Options::VARIABLE_LENGTH) && !(sSyntaxElement.length() == 2 && ((sSyntaxElement[1] >= '0' && sSyntaxElement[1] <= '9') || sSyntaxElement[0] == 'd')))
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sNote, _guilang.get("GUI_ANALYZER_VARNAMETOOSHORT", sSyntaxElement)), ANNOTATION_NOTE);
    }

    // Handle the variable's names: are they following guidelines?
    if (sSyntaxElement.length() > 2 && sSyntaxElement.find_first_not_of("\r\n") != string::npos && sSyntaxElement.find('.') == string::npos)
    {
        size_t shift = 0;

        // We want to start the procedures arguments with an underscore (not possible in MATLAB)
        if (sSyntaxElement[0] == '_' && m_editor->m_fileType == FILE_NPRC)
            shift++;

        // Because function names definitions are not highlighted different in MATLAB code, we leave the function
        // at m_editor position
        if (m_editor->m_fileType == FILE_MATLAB && m_hasProcedureDefinition && m_editor->GetCharAt(wordend) == '(')
        {
            m_nCurPos = wordend;
            return AnnotCount;
        }
        // numerical/int string float standard vars (x,y,z,t)
        string sFirstChars = "nsfbxyzt";

        if (sFirstChars.find(sSyntaxElement[shift]) == string::npos
                || ((sSyntaxElement[shift + 1] < 'A' || sSyntaxElement[shift + 1] > 'Z') && sSyntaxElement[shift + 1] != '_'))
        {
            // var not type-oriented
            // Add and underscore to indicate the procedures arguments
            if (m_options->GetAnalyzerOption(Options::ARGUMENT_UNDERSCORE) && m_hasProcedureDefinition && !shift && m_editor->m_fileType != FILE_MATLAB)
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sNote, _guilang.get("GUI_ANALYZER_INDICATEARGUMENT")), ANNOTATION_NOTE);

            // variable should begin with lowercase letter indicate its type
            if (m_options->GetAnalyzerOption(Options::TYPE_ORIENTATION))
                AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sNote, _guilang.get("GUI_ANALYZER_VARNOTTYPEORIENTED", sSyntaxElement)), ANNOTATION_NOTE);
        }
        else if (m_options->GetAnalyzerOption(Options::ARGUMENT_UNDERSCORE) && m_hasProcedureDefinition && !shift && m_editor->m_fileType != FILE_MATLAB)
        {
            // Add and underscore to indicate the procedures arguments
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()), m_sNote, _guilang.get("GUI_ANALYZER_INDICATEARGUMENT")), ANNOTATION_NOTE);
        }
    }
    m_nCurPos = wordend;

    // Return the gathered number of annotations
    return AnnotCount;
}


/////////////////////////////////////////////////
/// \brief Analyses occurences of operators
///
/// \return AnnotationCount
///
/// This member function analyses syntax elements, which are highlighted as operators
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::analyseOperators()
{
    AnnotationCount AnnotCount;

    // If the current operator is a parenthesis, try to find the matching one
    if (m_editor->GetCharAt(m_nCurPos) == '(' || m_editor->GetCharAt(m_nCurPos) == '[' || m_editor->GetCharAt(m_nCurPos) == '{'
            || m_editor->GetCharAt(m_nCurPos) == ')' || m_editor->GetCharAt(m_nCurPos) == ']' || m_editor->GetCharAt(m_nCurPos) == '}')
    {
        int nPos = m_editor->BraceMatch(m_nCurPos);
        if (nPos < 0)
        {
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(string(1, m_editor->GetCharAt(m_nCurPos)), m_nCurPos, 1), m_sError, _guilang.get("GUI_ANALYZER_MISSINGPARENTHESIS")), ANNOTATION_ERROR);
        }
    }

    return AnnotCount;
}


/////////////////////////////////////////////////
/// \brief Analyses occurences of numerical literals
///
/// \return AnnotationCount
///
/// This member function analyses syntax elements, which are highlighted as numbers
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::analyseNumbers()
{
    AnnotationCount AnnotCount;
    int nNumberStart = m_nCurPos;
    int nLineStartPos = m_editor->PositionFromLine(m_nCurrentLine);

    // Advance until the style of the next character is not a number any more
    while (m_editor->isStyleType(NumeReEditor::STYLE_NUMBER, m_nCurPos + 1))
        m_nCurPos++;

    if (!m_options->GetAnalyzerOption(Options::MAGIC_NUMBERS))
        return AnnotCount;

    // Get the number
    string sCurrentNumber = m_editor->GetTextRange(nNumberStart, m_nCurPos + 1).ToStdString();

    // Go inversely through the line and try to find an assignment operator
    for (int i = nNumberStart; i >= nLineStartPos; i--)
    {
        // If the current character is a operator and the previous one is not
        if (m_editor->isStyleType(NumeReEditor::STYLE_OPERATOR, i))
        {
            // Is an assignment -> no magic number
            if (m_editor->GetCharAt(i) == '='
                && m_editor->GetCharAt(i - 1) != '<'
                && m_editor->GetCharAt(i - 1) != '>'
                && m_editor->GetCharAt(i - 1) != '!'
                && m_editor->GetCharAt(i - 1) != '~'
                && m_editor->GetCharAt(i - 1) != '=')
                break;

            // All other operators are indicating the current number as magic number
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE", highlightFoundOccurence(sCurrentNumber, nNumberStart, sCurrentNumber.length()), m_sWarn, _guilang.get("GUI_ANALYZER_MAGICNUMBER")), ANNOTATION_WARN);
            break;
        }
    }

    // Return the number of gathered annotations
    return AnnotCount;
}


/////////////////////////////////////////////////
/// \brief Adds the passed sMessage with the style to the internal cache
///
/// \param sMessage const string&
/// \param nStyle int
/// \return AnnotationCount
///
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::addToAnnotation(const string& sMessage, int nStyle)
{
	AnnotationCount annoCount;

	// Ensure that the options allow the current type of
	// annotation
	if (nStyle == ANNOTATION_NOTE && !m_options->GetAnalyzerOption(Options::USE_NOTES))
        return annoCount;

	if (nStyle == ANNOTATION_WARN && !m_options->GetAnalyzerOption(Options::USE_WARNINGS))
        return annoCount;

	if (nStyle == ANNOTATION_ERROR && !m_options->GetAnalyzerOption(Options::USE_ERRORS))
        return annoCount;

	int chartoadd = 0;
	// Do not show the same message multiple times
	if (m_sCurrentLine.find(sMessage) != string::npos
			&& (!m_sCurrentLine.find(sMessage) || m_sCurrentLine[m_sCurrentLine.find(sMessage) - 1] == '\n'))
		return annoCount;

	if (m_sCurrentLine.length())
	{
		m_sCurrentLine += "\n";
		chartoadd++;
	}

	m_sCurrentLine += sMessage;
	chartoadd += m_editor->countUmlauts(sMessage);

	m_sStyles.append(sMessage.length() + chartoadd, nStyle);

	// Increment the total counter
	if (nStyle == ANNOTATION_NOTE)
		annoCount.nNotes++;
	else if (nStyle == ANNOTATION_WARN)
		annoCount.nWarnings++;
	else if (nStyle == ANNOTATION_ERROR)
		annoCount.nErrors++;

	return annoCount;
}


/////////////////////////////////////////////////
/// \brief Checks for assignments
///
/// \param sCurrentLine const string&
/// \return bool
///
/// This member function checks, whether the current line
/// contains an assignement, i.e. the calculation result
/// is stored into a target variable
/////////////////////////////////////////////////
bool CodeAnalyzer::containsAssignment(const string& sCurrentLine)
{
	if (sCurrentLine.find('=') == string::npos)
		return false;

	// Go through the line and check, whether there's
	// a single equal sign
	for (size_t i = 1; i < sCurrentLine.length() - 1; i++)
	{
		if (sCurrentLine[i] == '='
				&& sCurrentLine[i - 1] != '<'
				&& sCurrentLine[i - 1] != '>'
				&& sCurrentLine[i - 1] != '!'
				&& sCurrentLine[i - 1] != '='
				&& sCurrentLine[i + 1] != '='
				&& !isInQuotes(sCurrentLine, i))
			return true;
	}

	return false;
}


/////////////////////////////////////////////////
/// \brief Highlights the occurence in the editor
///
/// \param sElement const string&
/// \param nPos int
/// \param nLength int
/// \return string The passed sElement string with an additional white space
///
/// The found occurence is highlighted in the editor using
/// an indicator
/////////////////////////////////////////////////
string CodeAnalyzer::highlightFoundOccurence(const string& sElement, int nPos, int nLength)
{
    m_editor->SetIndicatorCurrent(HIGHLIGHT_ANNOTATION);
    m_editor->IndicatorFillRange(nPos, nLength);
	return sElement;
}


/////////////////////////////////////////////////
/// \brief Calculates the cyclomatic complexity between both lines
///
/// \param startline int
/// \param endline int
/// \return int
///
/// The cyclomatic complexity (also called McGabe metric)
/// is calculated from the number of possible control flow
/// paths, which the code may take in a scope.
/////////////////////////////////////////////////
int CodeAnalyzer::calculateCyclomaticComplexity(int startline, int endline)
{
	int nCycComplx = 1;

	// Go through the block of code indicated by the
	// starting and endling line
	for (int i = m_editor->PositionFromLine(startline); i < m_editor->GetLineEndPosition(endline); i++)
	{
	    // Examine command occurences, which may be
	    // flow control statements
		if (m_editor->isStyleType(NumeReEditor::STYLE_COMMAND, i))
		{
			int wordstart = m_editor->WordStartPosition(i, true);
			int wordend = m_editor->WordEndPosition(i, true);

			if (m_editor->GetTextRange(wordstart, wordend) == "if"
					|| m_editor->GetTextRange(wordstart, wordend) == "elseif"
					|| m_editor->GetTextRange(wordstart, wordend) == "while"
					|| m_editor->GetTextRange(wordstart, wordend) == "case"
					|| m_editor->GetTextRange(wordstart, wordend) == "for")
				nCycComplx++;

			i = wordend;
		}

		// Examine standard function calls, which may be
		// logical functions
		if (m_editor->isStyleType(NumeReEditor::STYLE_FUNCTION, i))
		{
			int wordstart = m_editor->WordStartPosition(i, true);
			int wordend = m_editor->WordEndPosition(i, true);

			if (m_editor->GetTextRange(wordstart, wordend) == "and"
					|| m_editor->GetTextRange(wordstart, wordend) == "or"
					|| m_editor->GetTextRange(wordstart, wordend) == "xor")
				nCycComplx++;

			i = wordend;
		}

		// Examine operaters, which may be logical operators
		if (m_editor->isStyleType(NumeReEditor::STYLE_OPERATOR, i))
		{
			int j = i;

			while (m_editor->isStyleType(NumeReEditor::STYLE_OPERATOR, j))
				j++;

			if (m_editor->GetTextRange(i, j) == "&"
                    || m_editor->GetTextRange(i, j) == "&&"
					|| m_editor->GetTextRange(i, j) == "|"
					|| m_editor->GetTextRange(i, j) == "||"
					|| m_editor->GetTextRange(i, j) == "|||")
				nCycComplx++;

			i = j;
		}
	}

	return nCycComplx;
}


/////////////////////////////////////////////////
/// \brief Calculates the lines of code between both lines
///
/// \param startline int
/// \param endline int
/// \return int
///
/// The lines of code a calculated from the number of
/// all non-comment and non-whitespace lines (i.e. all
/// lines, which contain code)
/////////////////////////////////////////////////
int CodeAnalyzer::calculateLinesOfCode(int startline, int endline)
{
	int nLinesOfCode = 0;
	string currentline;

	// Go through the block of code indicated by the
	// starting and endling line
	for (int i = startline; i <= endline; i++)
	{
		currentline = m_editor->GetLine(i).ToStdString();

        // Ignore line comments
		if (currentline.find("##") != string::npos)
			currentline.erase(currentline.find("##"));

        // Check, whether the line contains at least a single
        // non-whitespace character
		if (currentline.find_first_not_of(" \t\r\n") != string::npos)
		{
		    // Check for block comments
			for (size_t j = m_editor->PositionFromLine(i); j < currentline.length() + m_editor->PositionFromLine(i); j++)
			{
				if (!m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, j)
                        && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, j)
						&& m_editor->GetCharAt(j) != ' '
						&& m_editor->GetCharAt(j) != '\t'
						&& m_editor->GetCharAt(j) != '\r'
						&& m_editor->GetCharAt(j) != '\n')
				{
					nLinesOfCode++;
					break;
				}
			}
		}
	}

	return nLinesOfCode;
}


/////////////////////////////////////////////////
/// \brief Counts the number of comment lines between both lines
///
/// \param startline int
/// \param endline int
/// \return int
///
/// The number of comment lines are calculated from
/// every line comment and the number of lines contained
/// in additional block comments.
/////////////////////////////////////////////////
int CodeAnalyzer::countNumberOfComments(int startline, int endline)
{
	int nComments = 0;

	// Go through the block of code indicated by the
	// starting and endling line
	for (int i = m_editor->PositionFromLine(startline); i < m_editor->GetLineEndPosition(endline); i++)
	{
	    // Count every line comment and block comment
		if (m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, i) || m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, i))
		{
			nComments++;

			// Find the end of the current block comment
			for (int j = i; j < m_editor->GetLineEndPosition(endline); j++)
			{
				if (!m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, j) && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, j))
				{
					i = j;
					break;
				}

				// Count the lines contained in the block comment
				if (j > i + 1 && m_editor->PositionFromLine(m_editor->LineFromPosition(j)) == j)
					nComments++;
			}
		}
	}

	return nComments;
}

