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
#include "../guilang.hpp"
#include "../../kernel/core/procedure/includer.hpp"

#define ANNOTATION_NOTE wxSTC_NSCR_PROCEDURE_COMMANDS+1
#define ANNOTATION_WARN wxSTC_NSCR_PROCEDURE_COMMANDS+2
#define ANNOTATION_ERROR wxSTC_NSCR_PROCEDURE_COMMANDS+3

#define HIGHLIGHT_ANNOTATION 12

//#include "../../kernel/core/utils/timer.hpp"

using namespace std;

static void replaceDocStrings(wxString& sStr)
{
    sStr.replace(sStr.find_first_of(" ("), std::string::npos, "()");
}

/////////////////////////////////////////////////
/// Constructor
/// \param parent NumeReEditor*
/// \param opts Options*
///
/////////////////////////////////////////////////
CodeAnalyzer::CodeAnalyzer(NumeReEditor* parent, Options* opts) : m_editor(parent), m_options(opts), m_nCurPos(0), m_nCurrentLine(0), m_hasProcedureDefinition(false)
{
    m_STRING_FUNCS = _guilang.getList("PARSERFUNCS_LISTFUNC_FUNC_*_[STRING]");
    m_STRING_METHODS = _guilang.getList("PARSERFUNCS_LISTFUNC_METHOD_*_[STRING]");
    m_DRAW_FUNCS = _guilang.getList("PARSERFUNCS_LISTFUNC_FUNC_*_[DRAW]");

    std::for_each(m_STRING_FUNCS.begin(), m_STRING_FUNCS.end(), replaceDocStrings);
    std::for_each(m_STRING_METHODS.begin(), m_STRING_METHODS.end(), replaceDocStrings);
    std::for_each(m_DRAW_FUNCS.begin(), m_DRAW_FUNCS.end(), replaceDocStrings);

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

    //Timer t("CodeAnalyzer::run(" + wxToUtf8(m_editor->GetFilenameString()) + ")");

    // Clear all annotations
    m_editor->AnnotationClearAll();
    m_vCurrentScopes.clear();

    // Clear the corresponding indicators
    m_editor->SetIndicatorCurrent(HIGHLIGHT_ANNOTATION);
    m_editor->IndicatorClearRange(0, m_editor->GetLastPosition());
    m_editor->IndicatorSetStyle(HIGHLIGHT_ANNOTATION, wxSTC_INDIC_ROUNDBOX);
    m_editor->IndicatorSetForeground(HIGHLIGHT_ANNOTATION, wxColor(0, 0, 255));

    // Ensure that the correct file type is used and that the setting is active
    if (!m_editor->getEditorSetting(NumeReEditor::SETTING_USEANALYZER)
        || (m_editor->m_fileType != FILE_NSCR
            && m_editor->m_fileType != FILE_NPRC
            && m_editor->m_fileType != FILE_MATLAB
            && m_editor->m_fileType != FILE_CPP))
        return;

    // Determine the annotation style
    m_editor->AnnotationSetVisible(wxSTC_ANNOTATION_BOXED);

    if (m_editor->isNumeReFileType() && m_editor->m_codeParser.isDirty())
        m_editor->parse(0);

    m_nCurrentLine = -1;
    m_hasProcedureDefinition = false;

    bool isContinuedLine = false;
    bool isAlreadyMeasured = false;
    bool isSuppressed = false;

    m_currentMode.clear();
    m_sCurrentLine.clear();
    m_sStyles.clear();
    std::string sFirstLine = "";
    std::string sFirstStyles = "";
    int nFirstLine = -1;

    CALC_COMPLEXITY = m_options->GetAnalyzerOption(Options::COMPLEXITY);
    CALC_PROCEDURE_LENGTH = m_options->GetAnalyzerOption(Options::PROCEDURE_LENGTH);
    CALC_LINES_OF_CODE = m_options->GetAnalyzerOption(Options::LINES_OF_CODE);
    CALC_COMMENT_DENSITY = m_options->GetAnalyzerOption(Options::COMMENT_DENSITY);
    FORCE_SHOW_METRICS = m_options->GetAnalyzerOption(Options::ALWAYS_SHOW_METRICS);
    CHECK_RESULT_ASSIGNMENT = m_options->GetAnalyzerOption(Options::RESULT_ASSIGNMENT);
    CHECK_RESULT_SUPPRESSION = m_options->GetAnalyzerOption(Options::RESULT_SUPPRESSION);
    CHECK_CONSTANT_EXPRESSION = m_options->GetAnalyzerOption(Options::CONSTANT_EXPRESSION);
    RECOMMEND_INLINE_IF = m_options->GetAnalyzerOption(Options::INLINE_IF);
    RECOMMEND_ARGUMENT_UNDERSCORE = m_options->GetAnalyzerOption(Options::ARGUMENT_UNDERSCORE);
    RECOMMEND_TYPE_ORIENTATION = m_options->GetAnalyzerOption(Options::TYPE_ORIENTATION);
    WARN_PROGRESS_RUNTIME = m_options->GetAnalyzerOption(Options::PROGRESS_RUNTIME);
    WARN_FALLTHROUGH = m_options->GetAnalyzerOption(Options::SWITCH_FALLTHROUGH);
    WARN_MISLEADING_TYPE = m_options->GetAnalyzerOption(Options::MISLEADING_TYPE);
    WARN_UNUSED_VARIABLES = m_options->GetAnalyzerOption(Options::UNUSED_VARIABLES);
    WARN_GLOBAL_VARIABLES = m_options->GetAnalyzerOption(Options::GLOBAL_VARIABLES);
    WARN_THISFILE_NAMESPACE = m_options->GetAnalyzerOption(Options::THISFILE_NAMESPACE);
    WARN_TYPE_MISUSE = m_options->GetAnalyzerOption(Options::TYPE_MISUSE);
    WARN_VARIABLE_LENGTH = m_options->GetAnalyzerOption(Options::VARIABLE_LENGTH);
    WARN_MAGIC_NUMBERS = m_options->GetAnalyzerOption(Options::MAGIC_NUMBERS);

    AnnotationCount AnnotCount;

    // Go through the whole file
    for (m_nCurPos = 0; m_nCurPos < m_editor->GetLastPosition(); m_nCurPos++)
    {
        // Ignore comments
        if (m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, m_nCurPos)
            || m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, m_nCurPos))
            continue;

        // It's a new line? Then finalize the contents of the last line
        // and display the contents as annotation
        if (m_nCurrentLine < m_editor->LineFromPosition(m_nCurPos))
        {
            // Get the line's contents
            std::string sLine = wxToUtf8(m_editor->GetLine(m_nCurrentLine));
            StripSpaces(sLine);

            // catch constant expressions
            if (CHECK_CONSTANT_EXPRESSION
                && sLine.length()
                && sLine.find_first_not_of("\n\r\t") != std::string::npos
                && sLine.find_first_not_of("0123456789eE+-*/.,;^(){} \t\r\n") == std::string::npos)
                AnnotCount += addWarning(sLine.substr(0, sLine.find_last_not_of("0123456789eE+-*/.,;^()")),
                                         "GUI_ANALYZER_CONSTEXPR");

            // Handle line continuations
            if (sLine.find("\\\\") != std::string::npos)
                isContinuedLine = true;
            else
            {
                isContinuedLine = false;
                m_hasProcedureDefinition = false;
                m_currentMode.clear();
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
            sLine = wxToUtf8(m_editor->GetLine(m_nCurrentLine));

            // Ensure that there's no trailing comment
            size_t commentStart = sLine.rfind("##");

            if (commentStart != std::string::npos && !isInQuotes(sLine, commentStart, true))
                sLine.erase(commentStart);

            // Remove also block comments
            size_t nBlockStart = 0;

            while ((nBlockStart = sLine.find("#*")) != std::string::npos)
            {
                if (sLine.find("*#", nBlockStart+2) == std::string::npos)
                {
                    sLine.erase(nBlockStart);
                    break;
                }
                else
                    sLine.erase(nBlockStart, sLine.find("*#", nBlockStart+2)+2 - nBlockStart);
            }

            // Find the last visible character
            size_t lastVisibleChar = sLine.find_last_not_of(" \t\r\n");

            // Determine, whether it is a semicolon or a line continuation
            if (lastVisibleChar != std::string::npos
                && (sLine[lastVisibleChar] == ';' || (lastVisibleChar > 0 && sLine.substr(lastVisibleChar-1, 2) == "\\\\")))
                isSuppressed = true;
            else
                isSuppressed = false;

            m_sCurrentLine.clear();
            m_sStyles.clear();
        }

        int style = m_editor->GetStyleAt(m_nCurPos);
        int ch = m_editor->GetCharAt(m_nCurPos);

        // Get code metrics for scripts if not already done
        if (m_editor->m_fileType == FILE_NSCR && !isAlreadyMeasured)
        {
            std::string sLine = wxToUtf8(m_editor->GetLine(m_nCurrentLine));
            StripSpaces(sLine);

            if (sLine.length()
                && sLine.find_first_not_of(" \n\r\t") != std::string::npos)
            {
                std::string sSyntaxElement = wxToUtf8(m_editor->GetFilenameString());
                isAlreadyMeasured = true;

                // Calculate the code metrics:
                // Complexity
                int nCyclomaticComplexity = calculateCyclomaticComplexity(m_nCurrentLine,
                                                                          m_editor->GetLineCount()-1);

                // LinesOfcode
                int nLinesOfCode = calculateLinesOfCode(m_nCurrentLine,
                                                        m_editor->GetLineCount()-1);

                // Number of comments
                int nNumberOfComments = countNumberOfComments(m_nCurrentLine,
                                                              m_editor->GetLineCount()-1);

                // comment density
                double dCommentDensity = (double)nNumberOfComments / (double)nLinesOfCode;

                // Compare the metrics with the contants and issue a note or a warning
                if (CALC_COMPLEXITY && nCyclomaticComplexity > MAXCOMPLEXITYWARN)
                    AnnotCount += addMetric(sSyntaxElement,
                                            "GUI_ANALYZER_HIGHCOMPLEXITY",
                                            toString(nCyclomaticComplexity),
                                            ANNOTATION_WARN);
                else if ((CALC_COMPLEXITY && nCyclomaticComplexity > MAXCOMPLEXITYNOTIFY)
                         || FORCE_SHOW_METRICS)
                    AnnotCount += addMetric(sSyntaxElement,
                                            "GUI_ANALYZER_HIGHCOMPLEXITY",
                                            toString(nCyclomaticComplexity),
                                            ANNOTATION_NOTE);

                if ((CALC_LINES_OF_CODE && nLinesOfCode > MAXLINESOFCODE)
                    || FORCE_SHOW_METRICS)
                    AnnotCount += addMetric(sSyntaxElement,
                                            "GUI_ANALYZER_MANYLINES",
                                            toString(nLinesOfCode),
                                            ANNOTATION_NOTE);

                if ((CALC_COMMENT_DENSITY && dCommentDensity < MINCOMMENTDENSITY)
                    || (dCommentDensity < 1.0 && FORCE_SHOW_METRICS))
                    AnnotCount += addMetric(sSyntaxElement,
                                            "GUI_ANALYZER_LOWCOMMENTDENSITY",
                                            toString(dCommentDensity * 100.0, 3),
                                            ANNOTATION_NOTE);

                if ((CALC_COMMENT_DENSITY && dCommentDensity > MAXCOMMENTDENSITY)
                    || (dCommentDensity >= 1.0 && FORCE_SHOW_METRICS))
                    AnnotCount += addMetric(sSyntaxElement,
                                            "GUI_ANALYZER_HIGHCOMMENTDENSITY",
                                            toString(dCommentDensity * 100.0, 3),
                                            ANNOTATION_NOTE);
            }
        }

        // Handle the different style types
        if (m_editor->isStyleType(NumeReEditor::STYLE_COMMAND, m_nCurPos))
        {
            std::string sWord = wxToUtf8(m_editor->GetTextRange(m_editor->WordStartPosition(m_nCurPos, true),
                                                                m_editor->WordEndPosition(m_nCurPos, true)));

            if (!isSuppressed
                && sWord != "matop"
                && sWord != "mtrxop"
                && sWord != "assert"
                && sWord != "explicit"
                && sWord != "global")
                isSuppressed = true;

            // Handle commands // 200ms
            AnnotCount += analyseCommands();
        }
        else if (m_editor->isStyleType(NumeReEditor::STYLE_FUNCTION, m_nCurPos)
                 || ((m_editor->m_fileType == FILE_NSCR || m_editor->m_fileType == FILE_NPRC)
                     && style == wxSTC_NSCR_METHOD))
        {
            if (CHECK_RESULT_SUPPRESSION
                && !isSuppressed)
            {
                std::string sWord = wxToUtf8(m_editor->GetTextRange(m_editor->WordStartPosition(m_nCurPos, true),
                                                                    m_editor->WordEndPosition(m_nCurPos, true)));

                AnnotCount += addNote(highlightFoundOccurence(sWord + "()", m_editor->WordStartPosition(m_nCurPos, true), sWord.length()),
                                      "GUI_ANALYZER_SUPPRESS_OUTPUT");
                isSuppressed = true;
            }

            // Handle standard functions //30ms
            AnnotCount += analyseFunctions(isContinuedLine);
        }
        else if (m_editor->isStyleType(NumeReEditor::STYLE_PROCEDURE, m_nCurPos))
        {
            // Handle NumeRe procedure calls (NumeRe only) // 20ms
            AnnotCount += analyseProcedures();
        }
        else if ((m_editor->isStyleType(NumeReEditor::STYLE_IDENTIFIER, m_nCurPos)
                  || m_editor->isStyleType(NumeReEditor::STYLE_DATAOBJECT, m_nCurPos))
                 && ch != ' '
                 && ch != '\t'
                 && ch != '\r'
                 && ch != '\n')
        {
            if (CHECK_RESULT_SUPPRESSION
                && !isSuppressed)
            {
                std::string sWord = wxToUtf8(m_editor->GetTextRange(m_editor->WordStartPosition(m_nCurPos, true),
                                                                    m_editor->WordEndPosition(m_nCurPos, true)));

                AnnotCount += addNote(highlightFoundOccurence(sWord, m_editor->WordStartPosition(m_nCurPos, true), sWord.length()),
                                      "GUI_ANALYZER_SUPPRESS_OUTPUT");
                isSuppressed = true;
            }

            // Handle identifiers (like variable names) // 30ms
            AnnotCount += analyseIdentifiers();
        }
        else if (m_editor->isStyleType(NumeReEditor::STYLE_OPERATOR, m_nCurPos))
        {
            if (CHECK_RESULT_SUPPRESSION
                && ch == '='
                && !isSuppressed)
            {
                AnnotCount += addNote(highlightFoundOccurence("=", m_nCurPos, 1),
                                      "GUI_ANALYZER_SUPPRESS_OUTPUT");
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
        else if ((m_editor->m_fileType == FILE_NSCR || m_editor->m_fileType == FILE_NPRC)
                 && style == wxSTC_NSCR_PREDEFS)
        {
            AnnotCount += analysePreDefs();
        }
    }

    // Clear the annotation and style cache
    m_sCurrentLine.clear();
    m_sStyles.clear();

    // Write the summary lines
    if (AnnotCount.nNotes)
        addToAnnotation(_guilang.get("GUI_ANALYZER_NOTE_TOTAL", toString(AnnotCount.nNotes)),
                        ANNOTATION_NOTE);

    if (AnnotCount.nWarnings)
        addToAnnotation(_guilang.get("GUI_ANALYZER_WARN_TOTAL", toString(AnnotCount.nWarnings)),
                        ANNOTATION_WARN);

    if (AnnotCount.nErrors)
        addToAnnotation(_guilang.get("GUI_ANALYZER_ERROR_TOTAL", toString(AnnotCount.nErrors)),
                        ANNOTATION_ERROR);

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
    std::string sSyntaxElement = wxToUtf8(m_editor->GetTextRange(wordstart, wordend));

    // Handle blocks with their corresponding end
    if (m_editor->isBlockStart(sSyntaxElement) != wxNOT_FOUND)
    {
        // Try to find the matching block parts
        vector<int> vMatch = m_editor->BlockMatch(m_nCurPos);

        if (vMatch.size() > 1 && vMatch.back() != wxSTC_INVALID_POSITION)
            m_vCurrentScopes.push_back(vMatch);
    }
    else if (m_editor->isBlockEnd(sSyntaxElement) != wxNOT_FOUND)
    {
        if (m_vCurrentScopes.size() && m_vCurrentScopes.back().back() == wordstart)
            m_vCurrentScopes.pop_back();
    }

    // add a message to "throw"
    if (sSyntaxElement == "throw")
    {
        for (int j = wordend; j < m_editor->GetLineEndPosition(m_nCurrentLine); j++)
        {
            if (m_editor->GetStyleAt(j) == wxSTC_NSCR_STRING
                || m_editor->GetStyleAt(j) == wxSTC_NSCR_STRING_PARSER
                || m_editor->GetStyleAt(j) == wxSTC_NSCR_CLUSTER
                || m_editor->GetStyleAt(j) == wxSTC_NSCR_IDENTIFIER)
            {
                canContinue = true;
                break;
            }
        }

        // Was a message found?
        if (!canContinue)
        {
            AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, 5),
                                  "GUI_ANALYZER_THROW_ADDMESSAGE");
        }
    }

    // check the namespace command
    if (sSyntaxElement == "namespace")
    {
        if (m_editor->m_fileType == FILE_NSCR)
        {
            // Not usable command here
            AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                   "GUI_ANALYZER_NOTALLOWED");
            return AnnotCount;
        }

        std::string sArgs = wxToUtf8(m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)));

        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();

        StripSpaces(sArgs);

        // Is there an explicit namespace name? If no, warn the user
        if (!sArgs.length())
        {
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                     "GUI_ANALYZER_NAMESPACE_ALWAYSMAIN");
        }

        m_nCurPos = wordend;

        // Advance the character pointer and return the number of gathered annotations
        while (m_editor->GetCharAt(m_nCurPos) != ';' && m_editor->GetCharAt(m_nCurPos) != '\r' && m_editor->GetCharAt(m_nCurPos) != '\n')
            m_nCurPos++;

        return AnnotCount;
    }

    // The progress command needs extra runtime (2-4 times). Inform the user about this issue
    if (WARN_PROGRESS_RUNTIME && sSyntaxElement == "progress")
    {
        AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                              "GUI_ANALYZER_PROGRESS_RUNTIME");
    }

    // The install or the start commands are not allowed in scripts and procedures
    if (sSyntaxElement == "install" || sSyntaxElement == "uninstall" || sSyntaxElement == "start")
    {
        AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                               "GUI_ANALYZER_NOTALLOWED");
    }

    // Store mode-specific commands
    if (sSyntaxElement == "matop" || sSyntaxElement == "mtrxop")
    {
        m_currentMode = "matop";
        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                 "GUI_ANALYZER_DEPRECATED");
    }
    else if (sSyntaxElement == "draw" || sSyntaxElement == "draw3d")
        m_currentMode = "draw";

    // Handle the memory clearance commands
    if (sSyntaxElement == "clear" || sSyntaxElement == "delete" || sSyntaxElement == "remove")
    {
        // some caches may not be removec
        if (sSyntaxElement == "remove" && m_editor->GetStyleAt(m_editor->WordStartPosition(wordend + 1, true)) == wxSTC_NSCR_PREDEFS)
            AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                   "GUI_ANALYZER_CANNOTREMOVEPREDEFS");

        // Get everything after the clearance command
        std::string sArgs = wxToUtf8(m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)));
        while (sArgs.back() == '\r' || sArgs.back() == '\n' || sArgs.back() == ';')
            sArgs.pop_back();

        // Inform the user that he should append "ignore" as parameter
        if (!findParameter(sArgs, "ignore")
                && !findParameter(sArgs, "i")
                && (sSyntaxElement != "remove" || m_editor->GetStyleAt(m_editor->WordStartPosition(wordend + 1, true)) != wxSTC_NSCR_CUSTOM_FUNCTION))
            AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                  "GUI_ANALYZER_APPENDIGNORE");
    }

    // check, whether the current command do have an expression
    // Ignore some special commands, which do not need an expression
    if (sSyntaxElement != "hline"
        && sSyntaxElement != "continue"
        && sSyntaxElement != "break"
        && sSyntaxElement != "leave"
        && sSyntaxElement != "separator"
        && sSyntaxElement != "else"
        && m_editor->isBlockEnd(sSyntaxElement) == wxNOT_FOUND
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
        && sSyntaxElement != "clc"
        && sSyntaxElement != "throw"
        && sSyntaxElement != "rethrow"
        && sSyntaxElement != "namespace" //warning
       )
    {
        canContinue = false;

        // Get everything in the current line after the command
        std::string sArgs = wxToUtf8(m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)));
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
                AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                       "GUI_ANALYZER_EMPTYEXPRESSION");
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
        || sSyntaxElement == "taylor"
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
        if (CHECK_RESULT_ASSIGNMENT && !canContinue)
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                     "GUI_ANALYZER_ASSIGNTOVARIABLE");
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
                        AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, j, 1),
                                               "GUI_ANALYZER_MISSINGPARENTHESIS");
                        break;
                    }
                }
                else
                    nPos = m_editor->GetLineEndPosition(m_nCurrentLine);

                // Get the argument
                std::string sArgument = wxToUtf8(m_editor->GetTextRange(j + 1, nPos));
                StripSpaces(sArgument);

                // Is the argument available?
                if (!sArgument.length())
                {
                    AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, j, 2),
                                           "GUI_ANALYZER_MISSINGARGUMENT");
                    break;
                }

                // Is it a constant?
                if (sArgument == "true" || (sArgument.find_first_not_of("1234567890.") == std::string::npos && sArgument != "0"))
                {
                    if (sSyntaxElement == "while")
                        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                                 "GUI_ANALYZER_WHILE_ALWAYSTRUE");
                    else if (sSyntaxElement == "switch")
                        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                                 "GUI_ANALYZER_SWITCH_CONSTANT");
                    else
                        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                                 "GUI_ANALYZER_IF_ALWAYSTRUE");
                }
                else if (sArgument == "false" || sArgument == "0")
                {
                    if (sSyntaxElement == "while")
                        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                                 "GUI_ANALYZER_WHILE_ALWAYSFALSE");
                    else if (sSyntaxElement == "switch")
                        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                                 "GUI_ANALYZER_SWITCH_CONSTANT");
                    else
                        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                                 "GUI_ANALYZER_IF_ALWAYSFALSE");
                }
                else if (containsAssignment(sArgument))
                {
                    // Does it contain an assignment? Warn the user as m_editor is probably not intendet
                    AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                             "GUI_ANALYZER_ASSIGNMENTINARGUMENT");
                }

                break;
            }
        }

        // There's an faster, inline if-else operator in NumeRe
        // Propose that, if the current if-else block is quite short
        if (sSyntaxElement == "if" && m_editor->m_fileType != FILE_MATLAB && RECOMMEND_INLINE_IF)
        {
            // Was the end of the current block found?
            if (m_vCurrentScopes.size() && m_vCurrentScopes.back().front() == wordstart)
            {
                const std::vector<int>& vBlock = m_vCurrentScopes.back();

                // Check the length of the current block
                if (m_editor->LineFromPosition(vBlock.back()) - m_nCurrentLine < 5)
                {
                    canContinue = false;

                    // Ensure that no commands except of further ifs and elses are used inside of the found block
                    for (int pos = wordend; pos <= vBlock.back(); pos++)
                    {
                        if (m_editor->GetStyleAt(pos) == wxSTC_NSCR_COMMAND)
                        {
                            wxString command = m_editor->GetTextRange(m_editor->WordStartPosition(pos, true),
                                                                      m_editor->WordEndPosition(pos, true));
                            if (command != "if"
                                && command != "else"
                                && command != "endif")
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
                    }

                    // Was an other command found?
                    if (!canContinue)
                        AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                              "GUI_ANALYZER_USEINLINEIF");
                }
            }
        }

        // Ensure that a switch does contain at least one case
        if (sSyntaxElement == "switch")
        {
            std::vector<int> vBlock;

            if (m_vCurrentScopes.size() && m_vCurrentScopes.back().front() == wordstart)
                vBlock = m_vCurrentScopes.back();
            else
                vBlock = m_editor->BlockMatch(m_nCurPos);

            // Examine each match
            for (size_t i = 1; i < vBlock.size(); i++)
            {
                // Invalid position -> missing end. Already handled elsewhere
                if (vBlock[i] == wxSTC_INVALID_POSITION)
                    break;

                wxString command = m_editor->GetTextRange(vBlock[i], m_editor->WordEndPosition(vBlock[i], true));

                // Examine the command
                if (command == "case")
                {
                    // Contains a case: everything is alright
                    break;
                }
                else if (command == "default")
                {
                    // Only a default statement?
                    AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                             "GUI_ANALYZER_SWITCH_ONLY_DEFAULT");
                    break;
                }
                else if (command == "endswitch")
                {
                    // Neither a case nor a default statement?
                    AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                           "GUI_ANALYZER_SWITCH_MISSING_CASE");
                    break;
                }
            }
        }
    }

    // Check for fallthroughs and warn the user,
    // if one was found. Also check the appended value.
    if (sSyntaxElement == "case" && m_editor->m_fileType != FILE_MATLAB)
    {
        if (WARN_FALLTHROUGH)
        {
            std::vector<int> vBlock;

            if (m_vCurrentScopes.size()
                && std::find(m_vCurrentScopes.back().begin(), m_vCurrentScopes.back().end(), wordstart) != m_vCurrentScopes.back().end())
                vBlock = m_vCurrentScopes.back();
            else
                vBlock = m_editor->BlockMatch(m_nCurPos);

            // Search the correct position in the
            // whole block
            for (size_t i = 0; i < vBlock.size(); i++)
            {
                // Is this the correct position?
                if (vBlock[i] == wordstart)
                {
                    if (vBlock[i+1] != wxSTC_INVALID_POSITION
                        && m_editor->GetTextRange(vBlock[i+1], m_editor->WordEndPosition(vBlock[i+1], true)) != "endswitch")
                    {
                        // Search all occurences of the "break"
                        // command between the two statements
                        vector<int> vMatches = m_editor->m_search->FindAll("break", wxSTC_NSCR_COMMAND, wordend, vBlock[i+1], false);

                        // Search also for "return"s as an alternative
                        if (!vMatches.size())
                            vMatches = m_editor->m_search->FindAll("return", wxSTC_NSCR_COMMAND, wordend, vBlock[i+1], false);

                        // Search also for "leave"s as an alternative
                        if (!vMatches.size())
                            vMatches = m_editor->m_search->FindAll("leave", wxSTC_NSCR_COMMAND, wordend, vBlock[i+1], false);

                        // Ensure that there's one "break", "leave" or "return"
                        // statement
                        if (!vMatches.size())
                        {
                            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                                     "GUI_ANALYZER_SWITCH_MISSING_BREAK");
                        }
                    }

                    break;
                }
            }
        }

        // Check the value of the case
        wxString sLine = m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine));

        // Does the value contain something, which is not a whitespace
        // and not a comment?
        if (sLine.find_first_not_of(":\r\n#* ") == std::string::npos || sLine.find(':') == std::string::npos)
        {
            AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                   "GUI_ANALYZER_SWITCH_MISSING_VALUE");
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
                size_t nOpPos = std::string::npos;

                if (nPos < 0)
                {
                    // Missing parenthesis
                    AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, j, 1),
                                           "GUI_ANALYZER_MISSINGPARENTHESIS");
                    break;
                }

                // Get the argument from the parenthesis
                std::string sArgument = wxToUtf8(m_editor->GetTextRange(j + 1, nPos));
                StripSpaces(sArgument);


                // Argument is empty?
                if (!sArgument.length())
                {
                    AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, j, 2),
                                           "GUI_ANALYZER_MISSINGARGUMENT");
                    break;
                }

                // Important parts of the argument are missing?
                if ((nOpPos = sArgument.find("->")) != std::string::npos)
                {
                    // Range-based for loop
                    if (nOpPos >= sArgument.length()-2
                        || (!isalpha(sArgument.front()) && sArgument.front() != '_' && sArgument.front() != '{'))
                    {
                        AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                               "GUI_ANALYZER_FOR_INTERVALERROR");
                    }
                }
                else
                {
                    nOpPos = sArgument.find('=');

                    // Normal for loop
                    if (nOpPos >= sArgument.length()-1
                        || (!isalpha(sArgument.front()) && sArgument.front() != '_'))
                    {
                        AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, j+1, sArgument.length()),
                                               "GUI_ANALYZER_FOR_INTERVALERROR");
                    }

                    // Examine the optional condition
                    EndlessVector<std::string> args = getAllArguments(sArgument);

                    if (args.size() > 1)
                    {
                        if (args[1] == "true" || (args[1].find_first_not_of("1234567890.") == std::string::npos && args[1] != "0"))
                        {
                            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1+sArgument.find(args[1]), args[1].length()),
                                                     "GUI_ANALYZER_IF_ALWAYSTRUE");
                        }
                        else if (args[1] == "false" || args[1] == "0")
                        {
                            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1+sArgument.find(args[1]), args[1].length()),
                                                     "GUI_ANALYZER_IF_ALWAYSFALSE");
                        }
                        else if (containsAssignment(args[1]))
                        {
                            // Does it contain an assignment? Warn the user as m_editor is probably not intendet
                            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, j+1+sArgument.find(args[1]), args[1].length()),
                                                     "GUI_ANALYZER_ASSIGNMENTINARGUMENT");
                        }
                    }
                }

                break;
            }
        }
    }

    // Examine the current usage of the local variable declarators
    // Esp. ensure that the declared variables are used
    if (m_editor->m_fileType == FILE_NPRC
        && (sSyntaxElement == "var"
            || sSyntaxElement == "str"
            || sSyntaxElement == "tab"
            || sSyntaxElement == "cst"
            || sSyntaxElement == "obj"))
    {
        // Handle the special case "list -var"
        if (sSyntaxElement == "var"
            && m_editor->GetTextRange(m_editor->PositionFromLine(m_nCurrentLine),
                                      m_editor->GetLineEndPosition(m_nCurrentLine)).find("list") < (size_t)(wordstart - m_editor->PositionFromLine(m_nCurrentLine)))
        {
            m_nCurPos = wordend;
            return AnnotCount;
        }

        // Get the next line
        int nNextLineStartPosition = m_editor->GetLineEndPosition(m_nCurrentLine) + 1;

        // Find the end of the current procedure
        int nProcedureEndPosition = m_editor->FindText(nNextLineStartPosition,
                                                       m_editor->GetLastPosition(),
                                                       "endprocedure",
                                                       wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD);

        int nStyle = wxSTC_NSCR_IDENTIFIER;

        if (sSyntaxElement == "tab")
            nStyle = wxSTC_NSCR_CUSTOM_FUNCTION;
        else if (sSyntaxElement == "cst")
            nStyle = wxSTC_NSCR_CLUSTER;

        // extract the arguments and strip the spaces
        std::string sArgs = wxToUtf8(m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)));

        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();

        StripSpaces(sArgs);

        // Ensure that the end of the procedure is available
        if (nProcedureEndPosition == -1)
        {
            nProcedureEndPosition = m_editor->GetLastPosition();
            AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                   "GUI_ANALYZER_MISSINGENDPROCEDURE");
        }

        // Inform that this command is only executed once within
        // control flow blocks
        std::pair<wxString, int> blockStart = m_editor->getBlockStart(m_nCurrentLine);

        while (blockStart.second != wxNOT_FOUND && blockStart.first != "for" && blockStart.first != "while")
        {
            blockStart = m_editor->getBlockStart(blockStart.second-1);
        }

        if (blockStart.second != wxNOT_FOUND)
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                     "GUI_ANALYZER_ONLYEXECUTEDONCE");

        // If there are variables available
        if (sArgs.length())
        {
            std::string currentArg;

            // Extract variable by variable
            while (getNextArgument(sArgs, false).length())
            {
                currentArg = getNextArgument(sArgs, true);

                // remove assignments and parentheses and strip the spaces
                if (currentArg.find('=') != std::string::npos)
                    currentArg.erase(currentArg.find('='));

                if (currentArg.find_first_of("({;") != std::string::npos)
                    currentArg.erase(currentArg.find_first_of("({;"));

                StripSpaces(currentArg);

                if (!currentArg.length())
                    continue;

                // Will this variable be overwritten by a declare?
                if (m_editor->m_codeParser.isValid() && m_editor->m_codeParser.isSymbol(currentArg, m_nCurrentLine, false))
                {
                    const ParserSymbol& symbol = m_editor->m_codeParser.getSymbol(currentArg, m_nCurrentLine, false);

                    if (symbol.m_class == ParserSymbol::GLOBALCONST || symbol.m_class == ParserSymbol::GLOBALENUM)
                        AnnotCount += addError(currentArg, "GUI_ANALYZER_DECLAREOVERWRITES");
                }

                // Try to find the variable in the remaining code
                if (WARN_UNUSED_VARIABLES
                    && !m_editor->m_search->FindAll(currentArg, nStyle, nNextLineStartPosition, nProcedureEndPosition, false).size())
                {
                    // No variable found
                    int declarationPosition = m_editor->FindText(wordstart,
                                                                 nNextLineStartPosition,
                                                                 currentArg,
                                                                 wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD);
                    AnnotCount += addWarning(highlightFoundOccurence(currentArg, declarationPosition, currentArg.length()),
                                             "GUI_ANALYZER_UNUSEDVARIABLE");
                }
            }
        }
        else // No variables are available
            AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                   "GUI_ANALYZER_NOVARIABLES");
    }
    else if (m_editor->m_fileType == FILE_NSCR
             && (sSyntaxElement == "var"
                 || sSyntaxElement == "str"
                 || sSyntaxElement == "tab"
                 || sSyntaxElement == "cst"
                 || sSyntaxElement == "obj"
                 || sSyntaxElement == "procedure"
                 || sSyntaxElement == "endprocedure"))
    {
        // Not usable command here
        AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                               "GUI_ANALYZER_NOTALLOWED");
    }

    // Examine definitions
    if (sSyntaxElement == "define"
        || sSyntaxElement == "ifndefined"
        || sSyntaxElement == "lclfunc"
        || sSyntaxElement == "def"
        || sSyntaxElement == "ifndef")
    {
        // According Scintilla docu this shall return the start of the next word
        int definitionStart = m_editor->WordEndPosition(wordend, false);
        int definitionEnd = m_editor->WordEndPosition(definitionStart, true);
        std::string sDefinitionName = wxToUtf8(m_editor->GetTextRange(definitionStart, definitionEnd));

        if (m_editor->GetStyleAt(definitionStart) == wxSTC_NSCR_FUNCTION)
            AnnotCount += addError(highlightFoundOccurence(sDefinitionName, definitionStart, sDefinitionName.length()),
                                   "GUI_ANALYZER_STDFUNC_REDEF");

        // increment the position variable to the last position
        // in the current line, so that we may jump over the definition
        // in the following
        m_nCurPos = m_editor->GetLineEndPosition(m_editor->LineFromPosition(m_nCurPos));
        return AnnotCount;
    }

    // Handle includes by resolving them and loading all definitions and declares
    // if they are imported in this file
    if (sSyntaxElement == "include")
    {
        try
        {
            // Creating this instance might fail
            Includer incl(wxToUtf8(m_editor->GetTextRange(wordstart, m_editor->GetLineEndPosition(m_nCurrentLine))),
                          wxToUtf8(m_editor->GetFileName().GetPath()));
        }
        catch (SyntaxError& e)
        {
            AnnotCount += addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE",
                                                       sSyntaxElement,
                                                       m_sError,
                                                       _guilang.get("ERR_NR_2022_0_INCLUDE_NOT_EXIST", e.getToken())),
                                          ANNOTATION_ERROR);
        }

        m_nCurPos = m_editor->GetLineEndPosition(m_editor->LineFromPosition(m_nCurPos));
        return AnnotCount;
    }
//m_nCurPos = wordend;
//return AnnotCount;
    // Examine the procedure / MATLAB function starting at m_editor position
    // This includes esp. the calculation of the standard coding metrics
    if ((m_editor->m_fileType == FILE_NPRC && sSyntaxElement == "procedure")
        || (m_editor->m_fileType == FILE_MATLAB && sSyntaxElement == "function"))
    {
        // Use the block match function, which is capable of doing both: NumeRe and MATLAB syntax
        std::vector<int> vBlock;

        if (m_vCurrentScopes.size() && m_vCurrentScopes.back().front() == wordstart)
            vBlock = m_vCurrentScopes.back();
        else
            vBlock = m_editor->BlockMatch(m_nCurPos);

        m_hasProcedureDefinition = true;

        // If the current file is a procedure file, then decode the
        // argument list and store it in the list of known local
        // variables
        if (m_editor->m_fileType == FILE_NPRC)
        {
            int nArgumentParensStart = m_editor->FindText(m_nCurPos, m_editor->GetLineEndPosition(m_nCurrentLine), "(");
            int nArgumentParensEnd = m_editor->BraceMatch(nArgumentParensStart);

            if (nArgumentParensStart != -1 && nArgumentParensEnd != -1 && m_editor->m_codeParser.isValid())
            {
                // Decode the list
                for (int i = nArgumentParensStart+1; i < nArgumentParensEnd; i++)
                {
                    if (m_editor->GetStyleAt(i) == wxSTC_NPRC_IDENTIFIER
                        || m_editor->GetStyleAt(i) == wxSTC_NPRC_CUSTOM_FUNCTION
                        || m_editor->GetStyleAt(i) == wxSTC_NPRC_CLUSTER)
                    {
                        wxString sArg = m_editor->GetTextRange(m_editor->WordStartPosition(i, true), m_editor->WordEndPosition(i, true));

                        // Will this argument be overwritten by a declare?
                        if (m_editor->m_codeParser.isSymbol(wxToUtf8(sArg), m_nCurrentLine, false))
                        {
                            const ParserSymbol& symbol = m_editor->m_codeParser.getSymbol(wxToUtf8(sArg), m_nCurrentLine, false);

                            if (symbol.m_class == ParserSymbol::GLOBALCONST || symbol.m_class == ParserSymbol::GLOBALENUM)
                                AnnotCount += addError(sArg, "GUI_ANALYZER_DECLAREOVERWRITES");
                        }

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
                    AnnotCount += addError(sSyntaxElement, "GUI_ANALYZER_NONAMINGPROCEDURE");
                else if (WARN_THISFILE_NAMESPACE && nNamingProcedure != m_nCurrentLine)
                    AnnotCount += addWarning(sSyntaxElement, "GUI_ANALYZER_THISFILEPROCEDURE");
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
            if (CALC_PROCEDURE_LENGTH)
            {
                m_editor->m_search->FindMarkedProcedure(m_nCurPos+11, false);
                wxString procDef = m_editor->m_search->FindProcedureDefinition();

                if (nLinesOfCode < 5
                    && (procDef.find("::") == std::string::npos
                        || procDef.find("inline", procDef.find("::")) == std::string::npos))
                    AnnotCount += addWarning(sSyntaxElement, "GUI_ANALYZER_INLINING");
            }

            if (CALC_COMPLEXITY && nCyclomaticComplexity > MAXCOMPLEXITYWARN)
                AnnotCount += addMetric(sSyntaxElement,
                                        "GUI_ANALYZER_HIGHCOMPLEXITY",
                                        toString(nCyclomaticComplexity),
                                        ANNOTATION_WARN);
            else if ((CALC_COMPLEXITY && nCyclomaticComplexity > MAXCOMPLEXITYNOTIFY)
                     || FORCE_SHOW_METRICS)
                AnnotCount += addMetric(sSyntaxElement,
                                        "GUI_ANALYZER_HIGHCOMPLEXITY",
                                        toString(nCyclomaticComplexity),
                                        ANNOTATION_NOTE);

            if ((CALC_LINES_OF_CODE && nLinesOfCode > MAXLINESOFCODE)
                || FORCE_SHOW_METRICS)
                AnnotCount += addMetric(sSyntaxElement,
                                        "GUI_ANALYZER_MANYLINES",
                                        toString(nLinesOfCode),
                                        ANNOTATION_NOTE);

            if ((CALC_COMMENT_DENSITY && dCommentDensity < MINCOMMENTDENSITY)
                || (dCommentDensity < 1.0 && FORCE_SHOW_METRICS))
                AnnotCount += addMetric(sSyntaxElement,
                                        "GUI_ANALYZER_LOWCOMMENTDENSITY",
                                        toString(dCommentDensity * 100.0, 3),
                                        ANNOTATION_NOTE);

            if ((CALC_COMMENT_DENSITY && dCommentDensity > MAXCOMMENTDENSITY)
                || (dCommentDensity >= 1.0 && FORCE_SHOW_METRICS))
                AnnotCount += addMetric(sSyntaxElement,
                                        "GUI_ANALYZER_HIGHCOMMENTDENSITY",
                                        toString(dCommentDensity * 100.0, 3),
                                        ANNOTATION_NOTE);
        }
    }

    // Handle the "return" command in procedures (not needed in MATLAB)
    if (m_editor->m_fileType == FILE_NPRC && sSyntaxElement == "return")
    {
        // Try to find the end of the current procedure
        int nProcedureEnd = m_editor->FindText(m_nCurPos,
                                               m_editor->GetLastPosition(),
                                               "endprocedure",
                                               wxSTC_FIND_MATCHCASE | wxSTC_FIND_WHOLEWORD);

        // Get the argument of the return command and strip the spaces
        std::string sArgs = wxToUtf8(m_editor->GetTextRange(wordend, m_editor->GetLineEndPosition(m_nCurrentLine)));

        while (sArgs.back() == '\r' || sArgs.back() == '\n')
            sArgs.pop_back();

        StripSpaces(sArgs);

        // Ensure that the end of the procedure was found
        if (nProcedureEnd == -1)
        {
            AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                   "GUI_ANALYZER_MISSINGENDPROCEDURE");
        }

        // Examine the argument
        if (sArgs.length())
        {
            // Inform the user to add an semicolon to the arguments, if he uses something else than "void"
            if (CHECK_RESULT_SUPPRESSION && sArgs.back() != ';' && sArgs != "void")
                AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordend + 1, sArgs.length()),
                                      "GUI_ANALYZER_RETURN_ADDSEMICOLON");
        }
        else
        {
            // Inform the user that the return value will always be "true", if he doesn't append a value
            AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                  "GUI_ANALYZER_RETURN_ALWAYSTRUE");
        }
    }
//m_nCurPos = wordend;
//return AnnotCount;
    // Handle blocks with their corresponding end
    if (m_editor->isBlockStart(sSyntaxElement, true) != wxNOT_FOUND
        || m_editor->isBlockEnd(sSyntaxElement) != wxNOT_FOUND)
    {
        // Try to find the matching block parts
        std::vector<int> vMatch;

        if (m_vCurrentScopes.size()
            && std::find(m_vCurrentScopes.back().begin(), m_vCurrentScopes.back().end(), wordstart) != m_vCurrentScopes.back().end())
            vMatch = m_vCurrentScopes.back();
        else
            vMatch = m_editor->BlockMatch(m_nCurPos);

        if (vMatch.size() > 1)
        {
            // If there's an invalid position, m_editor means that the current block is unfinished
            if (vMatch.front() == wxSTC_INVALID_POSITION || vMatch.back() == wxSTC_INVALID_POSITION)
                AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                       "GUI_ANALYZER_UNFINISHEDBLOCK");
        }
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
    static NumeRe::CallTipProvider _provider = *m_editor->m_terminal->getProvider();
    NumeRe::CallTip _cTip;

    bool canContinue = false;
    int wordstart = m_editor->WordStartPosition(m_nCurPos, true);
    int wordend = m_editor->WordEndPosition(m_nCurPos, true);
    std::string rootType = "*";

    // Get the corresponding syntax element
    std::string sSyntaxElement = wxToUtf8(m_editor->GetTextRange(wordstart, wordend));

    // Handle method (modifier) calls, also appends a pair of parentheses if needed
    if ((m_editor->m_fileType == FILE_NSCR || m_editor->m_fileType == FILE_NPRC)
        && m_editor->GetStyleAt(m_nCurPos) == wxSTC_NSCR_METHOD)
    {
        if (std::find(m_STRING_METHODS.begin(), m_STRING_METHODS.end(), sSyntaxElement + "()") != m_STRING_METHODS.end()
            && m_currentMode == "matop")
            AnnotCount += addError(highlightFoundOccurence(sSyntaxElement + "()", wordstart, wordend-wordstart),
                                   "GUI_ANALYZER_STRINGFUNCTION");

        // ignore modifiers, i.e. method without parentheses
        static std::string sMODIFIER = " " + m_editor->_syntax->getNoArgMethods();

        if (sMODIFIER.find(" " + sSyntaxElement + " ") == std::string::npos)
            sSyntaxElement += "()";

        rootType = m_editor->get_method_root_type(m_nCurPos).first;

        if (rootType.length() && rootType != "*")
            sSyntaxElement.insert(0, rootType + ".");
        else
            sSyntaxElement.insert(0, "VAR.");
    }
    else
        sSyntaxElement += "()";

    // Is the current function called without a target variable?
    if (CHECK_RESULT_ASSIGNMENT && sSyntaxElement != "sleep()")
    {
        int startPos = m_editor->PositionFromLine(m_nCurrentLine);

        if (startPos == wordstart
            && !isContinuedLine)
        {
            // The function is called at the first position without a target variable
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart),
                                     "GUI_ANALYZER_ASSIGNTOVARIABLE");
        }
        else
        {
            // Try to find a assignment operator before the function
            // Other possibilities are commands and procedure calls
            for (int j = startPos; j < wordstart; j++)
            {
                if (m_editor->GetCharAt(j) == '='
                    || m_editor->isStyleType(NumeReEditor::STYLE_COMMAND, j)
                    || m_editor->isStyleType(NumeReEditor::STYLE_PROCEDURE, j)
                    || m_editor->isStyleType(NumeReEditor::STYLE_DATAOBJECT, j))
                {
                    canContinue = true;
                    break;
                }
            }

            // Was an operator or a command found?
            if (!canContinue && !isContinuedLine)
                AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart),
                                         "GUI_ANALYZER_ASSIGNTOVARIABLE");
        }
    }

    // There's a missing parenthesis?
    if (sSyntaxElement.find('(') != std::string::npos)
    {
        int nPos = m_editor->BraceMatch(wordend);

        if (nPos < wordend)
        {
            // MATLAB doesn't require a parenthesis pair for empty arguments.
            // However, issue a warning as it is good practice to visually distinguish between variables and functions
            if (m_editor->m_fileType == FILE_MATLAB)
                AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart),
                                         "GUI_ANALYZER_MISSINGPARENTHESIS");
            else
                AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart),
                                       "GUI_ANALYZER_MISSINGPARENTHESIS");
        }
        else
        {
            // Check for missing arguments
            std::string sArgument = wxToUtf8(m_editor->GetTextRange(wordend + 1, nPos));
            EndlessVector<std::string> args = getAllArguments(sArgument);

            if (sSyntaxElement.find('.') == std::string::npos)
                _cTip = _provider.getFunction(wxToUtf8(m_editor->GetTextRange(wordstart, wordend)));
            else
                _cTip = _provider.getMethod(wxToUtf8(m_editor->GetTextRange(wordstart, wordend)), rootType);

            //g_logger.info(sSyntaxElement + " | " + rootType + ": " + _cTip.sDefinition + " | " + toString(_cTip.arguments.size()));

            if (args.size() > _cTip.arguments.size()
                && !(_cTip.arguments.size() && _cTip.arguments.back() == "..."))
                AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordend, nPos-wordend+1),
                                       "GUI_ANALYZER_TOOMANYARGS");
            else if (args.size() < _cTip.nReqArgs)
                AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordend, nPos-wordend+1),
                                       "GUI_ANALYZER_MISSINGARGUMENT");
        }
    }

    // Examine mode-specific functions
    if (m_currentMode == "matop" && std::find(m_STRING_FUNCS.begin(), m_STRING_FUNCS.end(), sSyntaxElement) != m_STRING_FUNCS.end())
        AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart),
                               "GUI_ANALYZER_STRINGFUNCTION");

    if (m_currentMode != "draw" && std::find(m_DRAW_FUNCS.begin(), m_DRAW_FUNCS.end(), sSyntaxElement) != m_DRAW_FUNCS.end())
        AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordstart, wordend-wordstart),
                               "GUI_ANALYZER_DRAWFUNCTION");

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
    std::string sSyntaxElement = wxToUtf8(m_editor->m_search->FindMarkedProcedure(m_nCurPos));

    if (!sSyntaxElement.length())
        return AnnotCount;

    // Advance the character pointer until the style type changes
    while (m_editor->isStyleType(NumeReEditor::STYLE_PROCEDURE, m_nCurPos + 1))
        m_nCurPos++;

    // Validate the procedure name for unwanted characters
    if (!sSyntaxElement.starts_with("$'"))
    {
        for (size_t i = 1; i < sSyntaxElement.length()-2; i++)
        {
            if (!isalnum(sSyntaxElement[i])
                && sSyntaxElement[i] != '_'
                && sSyntaxElement[i] != '~')
            {
                AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, nProcStart, m_nCurPos-nProcStart+1),
                                       "GUI_ANALYZER_PROCEDUREINVALIDCHARS");
                break;
            }
        }
    }

    std::string procDef = wxToUtf8(m_editor->m_search->FindProcedureDefinition());

    // Try to find the correspondung procedure definition
    if (!procDef.length())
    {
        // Procedure definition was not found
        AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, nProcStart, m_nCurPos-nProcStart+1),
                               "GUI_ANALYZER_PROCEDURENOTFOUND");
    }
    else if (m_editor->m_fileNameAndPath.GetExt().Lower() != "nlyt")
    {
        size_t argParens = procDef.find('(');
        std::string sReqArgs = procDef.substr(argParens+1, getMatchingParenthesis(procDef)-argParens-1);
        EndlessVector<std::string> procArgs = getAllArguments(sReqArgs);

        int wordend = m_nCurPos+1;
        int nPos = m_editor->BraceMatch(wordend);
        std::string sArgument = wxToUtf8(m_editor->GetTextRange(wordend + 1, nPos));
        EndlessVector<std::string> args = getAllArguments(sArgument);

        if (args.size() > procArgs.size())
            AnnotCount += addError(highlightFoundOccurence(sSyntaxElement, wordend, nPos-wordend+1),
                                   "GUI_ANALYZER_TOOMANYARGS");
        else
        {
            for (size_t i = 0; i < procArgs.size(); i++)
            {
                if (procArgs[i].find('=') == std::string::npos && (args.size() <= i || !args[i].length()))
                    AnnotCount += addError(highlightFoundOccurence(procArgs[i], wordend, nPos-wordend+1),
                                           "GUI_ANALYZER_MISSINGARGUMENT");
            }
        }
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
    if (m_editor->GetCharAt(wordend) == '.'
        && m_editor->GetStyleAt(wordend + 1) != wxSTC_NSCR_METHOD
        && m_editor->GetStyleAt(wordend + 1) != wxSTC_NSCR_CUSTOM_METHOD)
        wordend = m_editor->WordEndPosition(wordend + 1, true);

    // Get the corresponding syntax element
    std::string sSyntaxElement = wxToUtf8(m_editor->GetTextRange(wordstart, wordend));

    // Warn about global variables
    if (WARN_GLOBAL_VARIABLES
        && m_editor->m_fileType == FILE_NPRC
        && m_editor->m_codeParser.isValidLine(m_nCurrentLine))
    {
        bool bOK = false;

        if (m_editor->m_codeParser.isSymbol(sSyntaxElement, m_nCurrentLine, true))
        {
            const ParserSymbol& symbol = m_editor->m_codeParser.getSymbol(sSyntaxElement, m_nCurrentLine, true);
            bOK = symbol.m_class != ParserSymbol::GLOBAL;
        }

        // nothing found
        if (!bOK)
        {
            wxString currentline = m_editor->GetLine(m_nCurrentLine);

            // Ignore y* variables from odesolve
            if (currentline.substr(currentline.find_first_not_of(" \t"), 9) != "odesolve "
                || !(sSyntaxElement[0] == 'y'
                     && sSyntaxElement.length() > 1
                     && sSyntaxElement.find_first_not_of("0123456789", 1) == std::string::npos))
                AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                         "GUI_ANALYZER_GLOBALVARIABLE");
        }
    }

    // Warn about global variables
    if (WARN_TYPE_MISUSE
        && (m_editor->m_fileType == FILE_NPRC || m_editor->m_fileType == FILE_NSCR)
        && m_editor->m_codeParser.isValidLine(m_nCurrentLine))
    {
        bool bOK = false;
        int style = m_editor->GetStyleAt(m_nCurPos);

        // Try to find the current identifier in the list
        // of known local variables
        if (m_editor->m_codeParser.isSymbol(sSyntaxElement, m_nCurrentLine, true))
        {
            const ParserSymbol& symbol = m_editor->m_codeParser.getSymbol(sSyntaxElement, m_nCurrentLine, true);
            bOK = ((symbol.m_type == "table" || symbol.m_type == "{value}" || symbol.m_type == "value" || symbol.isFunction()) && style == wxSTC_NSCR_CUSTOM_FUNCTION)
                || (symbol.m_type == "cluster" && style == wxSTC_NSCR_CLUSTER)
                || (symbol.m_type != "table" && symbol.m_type != "cluster" && style == wxSTC_NSCR_IDENTIFIER);

            // nothing found
            if (!bOK)
            {
                wxString currentline = m_editor->GetLine(m_nCurrentLine);

                // Ignore y* variables from odesolve
                if (currentline.substr(currentline.find_first_not_of(" \t"), 9) != "odesolve "
                    || !(sSyntaxElement[0] == 'y'
                         && sSyntaxElement.length() > 1
                         && sSyntaxElement.find_first_not_of("0123456789", 1) == std::string::npos))
                    AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                             "GUI_ANALYZER_TYPE_MISUSE");
            }
        }
    }

    // Return, if the current identifier is a data object
    if (m_editor->isStyleType(NumeReEditor::STYLE_DATAOBJECT, m_nCurPos))
    {
        if (m_options->getSetting(SETTING_B_TABLEREFS).active()
            && m_hasProcedureDefinition
            && m_editor->GetStyleAt(wordstart) == wxSTC_NSCR_CUSTOM_FUNCTION
            && m_editor->GetCharAt(wordstart-1) != '&'
            && m_editor->GetTextRange(wordend, wordend+3) != "()&")
        {
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                     "GUI_ANALYZER_WARN_TABLE_REFERENCE");
        }
    }

    // Handle very short variable names
    if (WARN_VARIABLE_LENGTH
        && sSyntaxElement.length() < 4
        && sSyntaxElement.length() > 1
        && sSyntaxElement.find_first_not_of("\r\n") != std::string::npos
        && sSyntaxElement.find('.') == std::string::npos)
    {
        // Too short
        if (!(sSyntaxElement.length() == 2 && (isdigit(sSyntaxElement[1]) || sSyntaxElement[0] == 'd')))
        {
            if (m_editor->m_codeParser.isValidLine(m_nCurrentLine)
                && m_editor->m_codeParser.isSymbol(sSyntaxElement, m_nCurrentLine, true))
            {
                if (!m_editor->m_codeParser.getSymbol(sSyntaxElement, m_nCurrentLine, true).isFunction())
                    AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                          "GUI_ANALYZER_VARNAMETOOSHORT");
            }
            else
                AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                      "GUI_ANALYZER_VARNAMETOOSHORT");
        }
    }

    // Handle the variable's names: are they following guidelines?
    if (sSyntaxElement.length() > 2
        && sSyntaxElement.find_first_not_of("\r\n") != std::string::npos
        && sSyntaxElement.find('.') == std::string::npos)
    {
        // Because function names definitions are not highlighted different in MATLAB code, we leave the function
        // at m_editor position
        if (m_editor->m_fileType == FILE_MATLAB
            && m_hasProcedureDefinition
            && m_editor->GetCharAt(wordend) == '(')
        {
            m_nCurPos = wordend;
            return AnnotCount;
        }

        if (m_editor->m_codeParser.isValidLine(m_nCurrentLine) && m_editor->m_codeParser.isSymbol(sSyntaxElement, m_nCurrentLine, true))
        {
            const ParserSymbol& symbol = m_editor->m_codeParser.getSymbol(sSyntaxElement, m_nCurrentLine, true);

            // Add and underscore to indicate the procedures arguments
            if (RECOMMEND_ARGUMENT_UNDERSCORE
                && (symbol.m_class == ParserSymbol::ARG || symbol.m_class == ParserSymbol::ARGREF)
                && (sSyntaxElement.front() != '_'))
                AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                      "GUI_ANALYZER_INDICATEARGUMENT");

            if ((RECOMMEND_TYPE_ORIENTATION || WARN_MISLEADING_TYPE)
                && symbol.m_type != "table"
                && symbol.m_type != "cluster"
                && !symbol.isFunction())
            {
                ParserSymbol typeTest(sSyntaxElement);

                if (typeTest.m_type == "void")
                {
                    // var not type-oriented
                    // variable should begin with lowercase letter indicate its type
                    if (RECOMMEND_TYPE_ORIENTATION)
                        AnnotCount += addNote(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                              "GUI_ANALYZER_VARNOTTYPEORIENTED");
                }
                else if (typeTest.getHeuristicEquivalent() != symbol.getHeuristicEquivalent())
                {
                    // var not type-oriented
                    // variable should begin with lowercase letter indicate its type
                    if (WARN_MISLEADING_TYPE)
                        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                                 "GUI_ANALYZER_MISLEADING_TYPE");
                }
            }
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
    if (m_editor->GetCharAt(m_nCurPos) == '('
        || m_editor->GetCharAt(m_nCurPos) == '['
        || m_editor->GetCharAt(m_nCurPos) == '{'
        || m_editor->GetCharAt(m_nCurPos) == ')'
        || m_editor->GetCharAt(m_nCurPos) == ']'
        || m_editor->GetCharAt(m_nCurPos) == '}')
    {
        int nPos = m_editor->BraceMatch(m_nCurPos);

        if (nPos < 0)
        {
            AnnotCount += addError(highlightFoundOccurence(string(1, m_editor->GetCharAt(m_nCurPos)), m_nCurPos, 1),
                                   "GUI_ANALYZER_MISSINGPARENTHESIS");
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

    if (!WARN_MAGIC_NUMBERS)
        return AnnotCount;

    // Get the number
    std::string sCurrentNumber = wxToUtf8(m_editor->GetTextRange(nNumberStart, m_nCurPos + 1));

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
            AnnotCount += addWarning(highlightFoundOccurence(sCurrentNumber, nNumberStart, sCurrentNumber.length()),
                                     "GUI_ANALYZER_MAGICNUMBER");
            break;
        }
    }

    // Return the number of gathered annotations
    return AnnotCount;
}


/////////////////////////////////////////////////
/// \brief Analyses occurences of special
/// predefined variables.
///
/// \return AnnotationCount
///
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::analysePreDefs()
{
    AnnotationCount AnnotCount;

    int wordstart = m_editor->WordStartPosition(m_nCurPos, true);
    int wordend = m_editor->WordEndPosition(m_nCurPos, true);

    // Get the corresponding syntax element
    std::string sSyntaxElement = wxToUtf8(m_editor->GetTextRange(wordstart, wordend));

    if (sSyntaxElement == "string")
        AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                 "GUI_ANALYZER_DEPRECATED");

    int contextPoint = 0;

    for (int i = m_nCurPos; i > m_editor->PositionFromLine(m_nCurrentLine); i--)
    {
        if ((m_editor->GetCharAt(i) == '(' || m_editor->GetCharAt(i) == '{' || m_editor->GetCharAt(i) == '[')
            && (m_editor->BraceMatch(i) >= m_nCurPos || m_editor->BraceMatch(i) == -1) // either no brace (yet) or the brace further right
            && (m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_CLUSTER
                || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_CUSTOM_FUNCTION
                || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_IDENTIFIER
                || m_editor->GetStyleAt(i - 1) == wxSTC_NSCR_PREDEFS)) // table() or data()
        {
            contextPoint = i;
            break;
        }
    }

    if (!contextPoint)
    {
        m_nCurPos = wordend;

        // No context point means used outside of data elements,
        // i.e. also a misuse
        if (sSyntaxElement == "nlen"
            || sSyntaxElement == "nrows"
            || sSyntaxElement == "ncols"
            || sSyntaxElement == "nlayers"
            || sSyntaxElement == "ndim"
            || sSyntaxElement == "nlines")
        {
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                     "GUI_ANALYZER_DIMVAR_MISUSE");
        }

        return AnnotCount;
    }

    // Check dimensionality variables and their corresponding
    // positions in the data element accesses
    if (sSyntaxElement == "nlen")
    {
        // Should only be used in clusters
        if (m_editor->GetStyleAt(contextPoint - 1) != wxSTC_NSCR_CLUSTER && m_editor->GetStyleAt(contextPoint - 1) != wxSTC_NSCR_IDENTIFIER)
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                     "GUI_ANALYZER_DIMVAR_MISUSE");
    }
    else if (sSyntaxElement == "nrows"
             || sSyntaxElement == "ncols"
             || sSyntaxElement == "nlines")
    {
        // Should only be used in tables. Additionally,
        // their order might be mixed up
        if (m_editor->GetStyleAt(contextPoint - 1) != wxSTC_NSCR_CUSTOM_FUNCTION
            && m_editor->GetStyleAt(contextPoint - 1) != wxSTC_NSCR_PREDEFS)
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                     "GUI_ANALYZER_DIMVAR_MISUSE");

        // Now, check for mixed up orientations
        int contextEnd = m_editor->BraceMatch(contextPoint);

        if (contextEnd == wxNOT_FOUND)
            contextEnd = m_editor->GetLineEndPosition(m_nCurrentLine);

        // Get the context content and split it up into
        // the single arguments
        std::string sContext = wxToUtf8(m_editor->GetTextRange(contextPoint+1, contextEnd));
        EndlessVector<std::string> args = getAllArguments(sContext);

        // Check, whether the dimension vars can be found in the
        // opposite dimension
        if ((sSyntaxElement == "ncols" && findVariableInExpression(args[0], sSyntaxElement, 0u) != std::string::npos)
            || (sSyntaxElement != "ncols" && findVariableInExpression(args[1], sSyntaxElement, 0u) != std::string::npos))
            AnnotCount += addWarning(highlightFoundOccurence(sSyntaxElement, wordstart, sSyntaxElement.length()),
                                     "GUI_ANALYZER_DIMVAR_MIXUP");
    }

    m_nCurPos = wordend;

    // Return the gathered number of annotations
    return AnnotCount;
}


/////////////////////////////////////////////////
/// \brief Adds the passed sMessage with the style to the internal cache
///
/// \param sMessage const wxString&
/// \param nStyle int
/// \return AnnotationCount
///
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::addToAnnotation(const wxString& sMessage, int nStyle)
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
    if (m_sCurrentLine.find(sMessage) != std::string::npos
        && (!m_sCurrentLine.find(sMessage) || m_sCurrentLine[m_sCurrentLine.find(sMessage) - 1] == '\n'))
        return annoCount;

    if (m_sCurrentLine.length())
    {
        m_sCurrentLine += "\n";
        chartoadd++;
    }

    m_sCurrentLine += sMessage;
    m_sStyles.append(wxToUtf8(sMessage).length() + chartoadd, nStyle);

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
/// \brief Wrapper member function to add an
/// analyzer message of type NOTE.
///
/// \param sSymbol const wxString&
/// \param messageId const char*
/// \return AnnotationCount
///
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::addNote(const wxString& sSymbol, const char* messageId)
{
    return addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE",
                                        sSymbol,
                                        m_sNote,
                                        _guilang.get(messageId, sSymbol)),
                           ANNOTATION_NOTE);
}


/////////////////////////////////////////////////
/// \brief Wrapper member function to add an
/// analyzer message of type WARNING.
///
/// \param sSymbol const wxString&
/// \param messageId const char*
/// \return AnnotationCount
///
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::addWarning(const wxString& sSymbol, const char* messageId)
{
    return addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE",
                                        sSymbol,
                                        m_sWarn,
                                        _guilang.get(messageId, sSymbol)),
                           ANNOTATION_WARN);
}


/////////////////////////////////////////////////
/// \brief Wrapper member function to add an
/// analyzer message of type ERROR.
///
/// \param sSymbol const wxString&
/// \param messageId const char*
/// \return AnnotationCount
///
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::addError(const wxString& sSymbol, const char* messageId)
{
    return addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE",
                                        sSymbol,
                                        m_sError,
                                        _guilang.get(messageId, sSymbol)),
                           ANNOTATION_ERROR);
}


/////////////////////////////////////////////////
/// \brief Add a metric with note or warning
/// level.
///
/// \param sSymbol const wxString&
/// \param messageId const char*
/// \param sValue const std::string&
/// \param nStyle int
/// \return AnnotationCount
///
/////////////////////////////////////////////////
AnnotationCount CodeAnalyzer::addMetric(const wxString& sSymbol, const char* messageId, const std::string& sValue, int nStyle)
{
    if (nStyle == ANNOTATION_NOTE)
        return addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE",
                                            sSymbol,
                                            m_sNote,
                                            _guilang.get(messageId, sValue)),
                               ANNOTATION_NOTE);

    return addToAnnotation(_guilang.get("GUI_ANALYZER_TEMPLATE",
                                        sSymbol,
                                        m_sWarn,
                                        _guilang.get(messageId, sValue)),
                           ANNOTATION_WARN);
}


/////////////////////////////////////////////////
/// \brief Checks for assignments
///
/// \param sCurrentLine const std::string&
/// \return bool
///
/// This member function checks, whether the current line
/// contains an assignement, i.e. the calculation result
/// is stored into a target variable
/////////////////////////////////////////////////
bool CodeAnalyzer::containsAssignment(const std::string& sCurrentLine)
{
    if (sCurrentLine.find('=') == std::string::npos)
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
/// \param sElement const std::string&
/// \param nPos int
/// \param nLength int
/// \return wxString The passed sElement string with an additional white space
///
/// The found occurence is highlighted in the editor using
/// an indicator
/////////////////////////////////////////////////
wxString CodeAnalyzer::highlightFoundOccurence(const std::string& sElement, int nPos, int nLength)
{
    m_editor->SetIndicatorCurrent(HIGHLIGHT_ANNOTATION);
    m_editor->IndicatorFillRange(nPos, nLength);
    return wxFromUtf8(sElement);
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

    if (!CALC_COMPLEXITY)
        return nCycComplx;

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
                || m_editor->GetTextRange(wordstart, wordend) == "try"
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

    if (!CALC_LINES_OF_CODE)
        return nLinesOfCode;

    string currentline;

    // Go through the block of code indicated by the
    // starting and endling line
    for (int i = startline; i <= endline; i++)
    {
        currentline = wxToUtf8(m_editor->GetLine(i));

        // Ignore line comments
        if (currentline.find("##") != std::string::npos)
            currentline.erase(currentline.find("##"));

        // Check, whether the line contains at least a single
        // non-whitespace character
        if (currentline.find_first_not_of(" \t\r\n") != std::string::npos)
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

    if (!CALC_COMMENT_DENSITY)
        return nComments;

    // Go through the block of code indicated by the
    // starting and endling line
    for (int i = m_editor->PositionFromLine(startline); i < m_editor->GetLineEndPosition(endline); i++)
    {
        // Count every line comment and block comment
        if (m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, i)
            || m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, i))
        {
            nComments++;

            // Find the end of the current block comment
            for (int j = i; j < m_editor->GetLineEndPosition(endline); j++)
            {
                if (!m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_LINE, j)
                    && !m_editor->isStyleType(NumeReEditor::STYLE_COMMENT_BLOCK, j))
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


/////////////////////////////////////////////////
/// \brief Returns the leading character, which
/// indicates the type of this variable or a null
/// character, if either the leading character is
/// not known or if it does not fulfill the
/// requirements.
///
/// \param sVarName const std::string&
/// \return char
///
/////////////////////////////////////////////////
char CodeAnalyzer::getVariableType(const std::string& sVarName)
{
    // Check some special cases first
    if (sVarName.length() <= 2
        || sVarName.find_first_not_of("\r\n") == std::string::npos
        || sVarName.find('.') != std::string::npos)
        return '\0';

    size_t shift = 0;

    // We want to start the procedures arguments with an underscore (not possible in MATLAB)
    while (sVarName[shift] == '_')
        shift++;

    // numerical/int string float standard vars (x,y,z,t)
    static std::string sFirstChars = "onsfbdxyzt";

    if (sVarName.length() > shift+1
        && (std::isupper(sVarName[shift+1]) || sVarName[shift+1] == '_')
        && sFirstChars.find(sVarName[shift]) != std::string::npos)
        return sVarName[shift];

    return '\0';
}


/////////////////////////////////////////////////
/// \brief Checks, whether the passed type is a
/// numeric type
///
/// \param c char
/// \return bool
///
/////////////////////////////////////////////////
static bool isNumericType(char c)
{
    return c == 'n' || c == 'd' || c == 'f';
}


/////////////////////////////////////////////////
/// \brief Change the variable's type to the
/// passed type.
///
/// \param sVarName std::string&
/// \param type char
/// \return void
///
/////////////////////////////////////////////////
void CodeAnalyzer::changeVariableType(std::string& sVarName, char type)
{
    char currentType = getVariableType(sVarName);

    // floats and integers are not so sharply separated
    if (currentType == type
        || (isNumericType(currentType) && isNumericType(type)))
        return;

    size_t shift = 0;

    // We want to start the procedures arguments with an underscore (not possible in MATLAB)
    while (sVarName[shift] == '_')
        shift++;

    // Now create the fixed variable name
    if (currentType) // Has already a type
        sVarName[shift] = type;
    else
    {
        if (!std::isupper(sVarName[shift]))
            sVarName[shift] = std::toupper(sVarName[shift]);

        sVarName.insert(shift, 1, type);
    }
}


