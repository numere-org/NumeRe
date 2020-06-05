// Scintilla source code edit control
/** @file LexOthers.cxx
 ** Lexers for batch files, diff results, properties files, make files and error lists.
 ** Also lexer for LaTeX documents.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <map>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static bool strstart(const char *haystack, const char *needle) {
	return strncmp(haystack, needle, strlen(needle)) == 0;
}

static bool Is0To9(char ch) {
	return (ch >= '0') && (ch <= '9');
}

static bool Is1To9(char ch) {
	return (ch >= '1') && (ch <= '9');
}

static bool IsAlphabetic(int ch) {
	return isascii(ch) && isalpha(ch);
}

static inline bool AtEOL(Accessor &styler, unsigned int i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

// Tests for BATCH Operators
static bool IsBOperator(char ch) {
	return (ch == '=') || (ch == '+') || (ch == '>') || (ch == '<') ||
		(ch == '|') || (ch == '?') || (ch == '*');
}

// Tests for BATCH Separators
static bool IsBSeparator(char ch) {
	return (ch == '\\') || (ch == '.') || (ch == ';') ||
		(ch == '\"') || (ch == '\'') || (ch == '/');
}

static void ColouriseBatchLine(
    char *lineBuffer,
    unsigned int lengthLine,
    unsigned int startLine,
    unsigned int endPos,
    WordList *keywordlists[],
    Accessor &styler) {

	unsigned int offset = 0;	// Line Buffer Offset
	unsigned int cmdLoc;		// External Command / Program Location
	char wordBuffer[81];		// Word Buffer - large to catch long paths
	unsigned int wbl;		// Word Buffer Length
	unsigned int wbo;		// Word Buffer Offset - also Special Keyword Buffer Length
	WordList &keywords = *keywordlists[0];      // Internal Commands
	WordList &keywords2 = *keywordlists[1];     // External Commands (optional)

	// CHOICE, ECHO, GOTO, PROMPT and SET have Default Text that may contain Regular Keywords
	//   Toggling Regular Keyword Checking off improves readability
	// Other Regular Keywords and External Commands / Programs might also benefit from toggling
	//   Need a more robust algorithm to properly toggle Regular Keyword Checking
	bool continueProcessing = true;	// Used to toggle Regular Keyword Checking
	// Special Keywords are those that allow certain characters without whitespace after the command
	// Examples are: cd. cd\ md. rd. dir| dir> echo: echo. path=
	// Special Keyword Buffer used to determine if the first n characters is a Keyword
	char sKeywordBuffer[10];	// Special Keyword Buffer
	bool sKeywordFound;		// Exit Special Keyword for-loop if found

	// Skip initial spaces
	while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
		offset++;
	}
	// Colorize Default Text
	styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
	// Set External Command / Program Location
	cmdLoc = offset;

	// Check for Fake Label (Comment) or Real Label - return if found
	if (lineBuffer[offset] == ':') {
		if (lineBuffer[offset + 1] == ':') {
			// Colorize Fake Label (Comment) - :: is similar to REM, see http://content.techweb.com/winmag/columns/explorer/2000/21.htm
			styler.ColourTo(endPos, SCE_BAT_COMMENT);
		} else {
			// Colorize Real Label
			styler.ColourTo(endPos, SCE_BAT_LABEL);
		}
		return;
	// Check for Drive Change (Drive Change is internal command) - return if found
	} else if ((IsAlphabetic(lineBuffer[offset])) &&
		(lineBuffer[offset + 1] == ':') &&
		((isspacechar(lineBuffer[offset + 2])) ||
		(((lineBuffer[offset + 2] == '\\')) &&
		(isspacechar(lineBuffer[offset + 3]))))) {
		// Colorize Regular Keyword
		styler.ColourTo(endPos, SCE_BAT_WORD);
		return;
	}

	// Check for Hide Command (@ECHO OFF/ON)
	if (lineBuffer[offset] == '@') {
		styler.ColourTo(startLine + offset, SCE_BAT_HIDE);
		offset++;
	}
	// Skip next spaces
	while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
		offset++;
	}

	// Read remainder of line word-at-a-time or remainder-of-word-at-a-time
	while (offset < lengthLine) {
		if (offset > startLine) {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
		}
		// Copy word from Line Buffer into Word Buffer
		wbl = 0;
		for (; offset < lengthLine && wbl < 80 &&
		        !isspacechar(lineBuffer[offset]); wbl++, offset++) {
			wordBuffer[wbl] = static_cast<char>(tolower(lineBuffer[offset]));
		}
		wordBuffer[wbl] = '\0';
		wbo = 0;

		// Check for Comment - return if found
		if (CompareCaseInsensitive(wordBuffer, "rem") == 0) {
			styler.ColourTo(endPos, SCE_BAT_COMMENT);
			return;
		}
		// Check for Separator
		if (IsBSeparator(wordBuffer[0])) {
			// Check for External Command / Program
			if ((cmdLoc == offset - wbl) &&
				((wordBuffer[0] == ':') ||
				(wordBuffer[0] == '\\') ||
				(wordBuffer[0] == '.'))) {
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
				// Colorize External Command / Program
				if (!keywords2) {
					styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
				} else if (keywords2.InList(wordBuffer)) {
					styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
				} else {
					styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
				}
				// Reset External Command / Program Location
				cmdLoc = offset;
			} else {
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
				// Colorize Default Text
				styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
			}
		// Check for Regular Keyword in list
		} else if ((keywords.InList(wordBuffer)) &&
			(continueProcessing)) {
			// ECHO, GOTO, PROMPT and SET require no further Regular Keyword Checking
			if ((CompareCaseInsensitive(wordBuffer, "echo") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "goto") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "prompt") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "set") == 0)) {
				continueProcessing = false;
			}
			// Identify External Command / Program Location for ERRORLEVEL, and EXIST
			if ((CompareCaseInsensitive(wordBuffer, "errorlevel") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "exist") == 0)) {
				// Reset External Command / Program Location
				cmdLoc = offset;
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
				// Skip comparison
				while ((cmdLoc < lengthLine) &&
					(!isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
			// Identify External Command / Program Location for CALL, DO, LOADHIGH and LH
			} else if ((CompareCaseInsensitive(wordBuffer, "call") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "do") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "loadhigh") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "lh") == 0)) {
				// Reset External Command / Program Location
				cmdLoc = offset;
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
			}
			// Colorize Regular keyword
			styler.ColourTo(startLine + offset - 1, SCE_BAT_WORD);
			// No need to Reset Offset
		// Check for Special Keyword in list, External Command / Program, or Default Text
		} else if ((wordBuffer[0] != '%') &&
				   (wordBuffer[0] != '!') &&
			(!IsBOperator(wordBuffer[0])) &&
			(continueProcessing)) {
			// Check for Special Keyword
			//     Affected Commands are in Length range 2-6
			//     Good that ERRORLEVEL, EXIST, CALL, DO, LOADHIGH, and LH are unaffected
			sKeywordFound = false;
			for (unsigned int keywordLength = 2; keywordLength < wbl && keywordLength < 7 && !sKeywordFound; keywordLength++) {
				wbo = 0;
				// Copy Keyword Length from Word Buffer into Special Keyword Buffer
				for (; wbo < keywordLength; wbo++) {
					sKeywordBuffer[wbo] = static_cast<char>(wordBuffer[wbo]);
				}
				sKeywordBuffer[wbo] = '\0';
				// Check for Special Keyword in list
				if ((keywords.InList(sKeywordBuffer)) &&
					((IsBOperator(wordBuffer[wbo])) ||
					(IsBSeparator(wordBuffer[wbo])))) {
					sKeywordFound = true;
					// ECHO requires no further Regular Keyword Checking
					if (CompareCaseInsensitive(sKeywordBuffer, "echo") == 0) {
						continueProcessing = false;
					}
					// Colorize Special Keyword as Regular Keyword
					styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_WORD);
					// Reset Offset to re-process remainder of word
					offset -= (wbl - wbo);
				}
			}
			// Check for External Command / Program or Default Text
			if (!sKeywordFound) {
				wbo = 0;
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					// Read up to %, Operator or Separator
					while ((wbo < wbl) &&
						(wordBuffer[wbo] != '%') &&
						(wordBuffer[wbo] != '!') &&
						(!IsBOperator(wordBuffer[wbo])) &&
						(!IsBSeparator(wordBuffer[wbo]))) {
						wbo++;
					}
					// Reset External Command / Program Location
					cmdLoc = offset - (wbl - wbo);
					// Reset Offset to re-process remainder of word
					offset -= (wbl - wbo);
					// CHOICE requires no further Regular Keyword Checking
					if (CompareCaseInsensitive(wordBuffer, "choice") == 0) {
						continueProcessing = false;
					}
					// Check for START (and its switches) - What follows is External Command \ Program
					if (CompareCaseInsensitive(wordBuffer, "start") == 0) {
						// Reset External Command / Program Location
						cmdLoc = offset;
						// Skip next spaces
						while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
						// Reset External Command / Program Location if command switch detected
						if (lineBuffer[cmdLoc] == '/') {
							// Skip command switch
							while ((cmdLoc < lengthLine) &&
								(!isspacechar(lineBuffer[cmdLoc]))) {
								cmdLoc++;
							}
							// Skip next spaces
							while ((cmdLoc < lengthLine) &&
								(isspacechar(lineBuffer[cmdLoc]))) {
								cmdLoc++;
							}
						}
					}
					// Colorize External Command / Program
					if (!keywords2) {
						styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
					} else if (keywords2.InList(wordBuffer)) {
						styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
					} else {
						styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
					}
					// No need to Reset Offset
				// Check for Default Text
				} else {
					// Read up to %, Operator or Separator
					while ((wbo < wbl) &&
						(wordBuffer[wbo] != '%') &&
						(wordBuffer[wbo] != '!') &&
						(!IsBOperator(wordBuffer[wbo])) &&
						(!IsBSeparator(wordBuffer[wbo]))) {
						wbo++;
					}
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
					// Reset Offset to re-process remainder of word
					offset -= (wbl - wbo);
				}
			}
		// Check for Argument  (%n), Environment Variable (%x...%) or Local Variable (%%a)
		} else if (wordBuffer[0] == '%') {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
			wbo++;
			// Search to end of word for second % (can be a long path)
			while ((wbo < wbl) &&
				(wordBuffer[wbo] != '%') &&
				(!IsBOperator(wordBuffer[wbo])) &&
				(!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}
			// Check for Argument (%n) or (%*)
			if (((Is0To9(wordBuffer[1])) || (wordBuffer[1] == '*')) &&
				(wordBuffer[wbo] != '%')) {
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - 2);
				}
				// Colorize Argument
				styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 2);
			// Check for Expanded Argument (%~...) / Variable (%%~...)
			} else if (((wbl > 1) && (wordBuffer[1] == '~')) ||
				((wbl > 2) && (wordBuffer[1] == '%') && (wordBuffer[2] == '~'))) {
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - wbo);
				}
				// Colorize Expanded Argument / Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);
			// Check for Environment Variable (%x...%)
			} else if ((wordBuffer[1] != '%') &&
				(wordBuffer[wbo] == '%')) {
				wbo++;
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - wbo);
				}
				// Colorize Environment Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);
			// Check for Local Variable (%%a)
			} else if (
				(wbl > 2) &&
				(wordBuffer[1] == '%') &&
				(wordBuffer[2] != '%') &&
				(!IsBOperator(wordBuffer[2])) &&
				(!IsBSeparator(wordBuffer[2]))) {
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - 3);
				}
				// Colorize Local Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - 3), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 3);
			}
		// Check for Environment Variable (!x...!)
		} else if (wordBuffer[0] == '!') {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
			wbo++;
			// Search to end of word for second ! (can be a long path)
			while ((wbo < wbl) &&
				(wordBuffer[wbo] != '!') &&
				(!IsBOperator(wordBuffer[wbo])) &&
				(!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}
			if (wordBuffer[wbo] == '!') {
				wbo++;
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - wbo);
				}
				// Colorize Environment Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);
			}
		// Check for Operator
		} else if (IsBOperator(wordBuffer[0])) {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
			// Check for Comparison Operator
			if ((wordBuffer[0] == '=') && (wordBuffer[1] == '=')) {
				// Identify External Command / Program Location for IF
				cmdLoc = offset;
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
				// Colorize Comparison Operator
				styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_OPERATOR);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 2);
			// Check for Pipe Operator
			} else if (wordBuffer[0] == '|') {
				// Reset External Command / Program Location
				cmdLoc = offset - wbl + 1;
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
				// Colorize Pipe Operator
				styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_OPERATOR);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
			// Check for Other Operator
			} else {
				// Check for > Operator
				if (wordBuffer[0] == '>') {
					// Turn Keyword and External Command / Program checking back on
					continueProcessing = true;
				}
				// Colorize Other Operator
				styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_OPERATOR);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
			}
		// Check for Default Text
		} else {
			// Read up to %, Operator or Separator
			while ((wbo < wbl) &&
				(wordBuffer[wbo] != '%') &&
				(wordBuffer[wbo] != '!') &&
				(!IsBOperator(wordBuffer[wbo])) &&
				(!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
			// Reset Offset to re-process remainder of word
			offset -= (wbl - wbo);
		}
		// Skip next spaces - nothing happens if Offset was Reset
		while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
			offset++;
		}
	}
	// Colorize Default Text for remainder of line - currently not lexed
	styler.ColourTo(endPos, SCE_BAT_DEFAULT);
}

static void ColouriseBatchDoc(
    unsigned int startPos,
    int length,
    int /*initStyle*/,
    WordList *keywordlists[],
    Accessor &styler) {

	char lineBuffer[1024];

	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;
	unsigned int startLine = startPos;
	for (unsigned int i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseBatchLine(lineBuffer, linePos, startLine, i, keywordlists, styler);
			linePos = 0;
			startLine = i + 1;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		lineBuffer[linePos] = '\0';
		ColouriseBatchLine(lineBuffer, linePos, startLine, startPos + length - 1,
		                   keywordlists, styler);
	}
}

#define DIFF_BUFFER_START_SIZE 16
// Note that ColouriseDiffLine analyzes only the first DIFF_BUFFER_START_SIZE
// characters of each line to classify the line.

static void ColouriseDiffLine(char *lineBuffer, int endLine, Accessor &styler) {
	// It is needed to remember the current state to recognize starting
	// comment lines before the first "diff " or "--- ". If a real
	// difference starts then each line starting with ' ' is a whitespace
	// otherwise it is considered a comment (Only in..., Binary file...)
	if (0 == strncmp(lineBuffer, "diff ", 5)) {
		styler.ColourTo(endLine, SCE_DIFF_COMMAND);
	} else if (0 == strncmp(lineBuffer, "Index: ", 7)) {  // For subversion's diff
		styler.ColourTo(endLine, SCE_DIFF_COMMAND);
	} else if (0 == strncmp(lineBuffer, "---", 3) && lineBuffer[3] != '-') {
		// In a context diff, --- appears in both the header and the position markers
		if (lineBuffer[3] == ' ' && atoi(lineBuffer + 4) && !strchr(lineBuffer, '/'))
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else if (lineBuffer[3] == '\r' || lineBuffer[3] == '\n')
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else
			styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (0 == strncmp(lineBuffer, "+++ ", 4)) {
		// I don't know of any diff where "+++ " is a position marker, but for
		// consistency, do the same as with "--- " and "*** ".
		if (atoi(lineBuffer+4) && !strchr(lineBuffer, '/'))
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else
			styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (0 == strncmp(lineBuffer, "====", 4)) {  // For p4's diff
		styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (0 == strncmp(lineBuffer, "***", 3)) {
		// In a context diff, *** appears in both the header and the position markers.
		// Also ******** is a chunk header, but here it's treated as part of the
		// position marker since there is no separate style for a chunk header.
		if (lineBuffer[3] == ' ' && atoi(lineBuffer+4) && !strchr(lineBuffer, '/'))
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else if (lineBuffer[3] == '*')
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else
			styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (0 == strncmp(lineBuffer, "? ", 2)) {    // For difflib
		styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (lineBuffer[0] == '@') {
		styler.ColourTo(endLine, SCE_DIFF_POSITION);
	} else if (lineBuffer[0] >= '0' && lineBuffer[0] <= '9') {
		styler.ColourTo(endLine, SCE_DIFF_POSITION);
	} else if (lineBuffer[0] == '-' || lineBuffer[0] == '<') {
		styler.ColourTo(endLine, SCE_DIFF_DELETED);
	} else if (lineBuffer[0] == '+' || lineBuffer[0] == '>') {
		styler.ColourTo(endLine, SCE_DIFF_ADDED);
	} else if (lineBuffer[0] == '!') {
		styler.ColourTo(endLine, SCE_DIFF_CHANGED);
	} else if (lineBuffer[0] != ' ') {
		styler.ColourTo(endLine, SCE_DIFF_COMMENT);
	} else {
		styler.ColourTo(endLine, SCE_DIFF_DEFAULT);
	}
}

static void ColouriseDiffDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	char lineBuffer[DIFF_BUFFER_START_SIZE];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;
	for (unsigned int i = startPos; i < startPos + length; i++) {
		if (AtEOL(styler, i)) {
			if (linePos < DIFF_BUFFER_START_SIZE) {
				lineBuffer[linePos] = 0;
			}
			ColouriseDiffLine(lineBuffer, i, styler);
			linePos = 0;
		} else if (linePos < DIFF_BUFFER_START_SIZE - 1) {
			lineBuffer[linePos++] = styler[i];
		} else if (linePos == DIFF_BUFFER_START_SIZE - 1) {
			lineBuffer[linePos++] = 0;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		if (linePos < DIFF_BUFFER_START_SIZE) {
			lineBuffer[linePos] = 0;
		}
		ColouriseDiffLine(lineBuffer, startPos + length - 1, styler);
	}
}

static void FoldDiffDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	int curLine = styler.GetLine(startPos);
	int curLineStart = styler.LineStart(curLine);
	int prevLevel = curLine > 0 ? styler.LevelAt(curLine - 1) : SC_FOLDLEVELBASE;
	int nextLevel;

	do {
		int lineType = styler.StyleAt(curLineStart);
		if (lineType == SCE_DIFF_COMMAND)
			nextLevel = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
		else if (lineType == SCE_DIFF_HEADER)
			nextLevel = (SC_FOLDLEVELBASE + 1) | SC_FOLDLEVELHEADERFLAG;
		else if (lineType == SCE_DIFF_POSITION && styler[curLineStart] != '-')
			nextLevel = (SC_FOLDLEVELBASE + 2) | SC_FOLDLEVELHEADERFLAG;
		else if (prevLevel & SC_FOLDLEVELHEADERFLAG)
			nextLevel = (prevLevel & SC_FOLDLEVELNUMBERMASK) + 1;
		else
			nextLevel = prevLevel;

		if ((nextLevel & SC_FOLDLEVELHEADERFLAG) && (nextLevel == prevLevel))
			styler.SetLevel(curLine-1, prevLevel & ~SC_FOLDLEVELHEADERFLAG);

		styler.SetLevel(curLine, nextLevel);
		prevLevel = nextLevel;

		curLineStart = styler.LineStart(++curLine);
	} while (static_cast<int>(startPos) + length > curLineStart);
}

static void ColourisePoLine(
    char *lineBuffer,
    unsigned int lengthLine,
    unsigned int startLine,
    unsigned int endPos,
    Accessor &styler) {

	unsigned int i = 0;
	static unsigned int state = SCE_PO_DEFAULT;
	unsigned int state_start = SCE_PO_DEFAULT;

	while ((i < lengthLine) && isspacechar(lineBuffer[i]))	// Skip initial spaces
		i++;
	if (i < lengthLine) {
		if (lineBuffer[i] == '#') {
			// check if the comment contains any flags ("#, ") and
			// then whether the flags contain "fuzzy"
			if (strstart(lineBuffer, "#, ") && strstr(lineBuffer, "fuzzy"))
				styler.ColourTo(endPos, SCE_PO_FUZZY);
			else
				styler.ColourTo(endPos, SCE_PO_COMMENT);
		} else {
			if (lineBuffer[0] == '"') {
				// line continuation, use previous style
				styler.ColourTo(endPos, state);
				return;
			// this implicitly also matches "msgid_plural"
			} else if (strstart(lineBuffer, "msgid")) {
				state_start = SCE_PO_MSGID;
				state = SCE_PO_MSGID_TEXT;
			} else if (strstart(lineBuffer, "msgstr")) {
				state_start = SCE_PO_MSGSTR;
				state = SCE_PO_MSGSTR_TEXT;
			} else if (strstart(lineBuffer, "msgctxt")) {
				state_start = SCE_PO_MSGCTXT;
				state = SCE_PO_MSGCTXT_TEXT;
			}
			if (state_start != SCE_PO_DEFAULT) {
				// find the next space
				while ((i < lengthLine) && ! isspacechar(lineBuffer[i]))
					i++;
				styler.ColourTo(startLine + i - 1, state_start);
				styler.ColourTo(startLine + i, SCE_PO_DEFAULT);
				styler.ColourTo(endPos, state);
			}
		}
	} else {
		styler.ColourTo(endPos, SCE_PO_DEFAULT);
	}
}

static void ColourisePoDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	char lineBuffer[1024];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;
	unsigned int startLine = startPos;
	for (unsigned int i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColourisePoLine(lineBuffer, linePos, startLine, i, styler);
			linePos = 0;
			startLine = i + 1;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		ColourisePoLine(lineBuffer, linePos, startLine, startPos + length - 1, styler);
	}
}

static inline bool isassignchar(unsigned char ch) {
	return (ch == '=') || (ch == ':');
}

static void ColourisePropsLine(
    char *lineBuffer,
    unsigned int lengthLine,
    unsigned int startLine,
    unsigned int endPos,
    Accessor &styler,
    bool allowInitialSpaces) {

	unsigned int i = 0;
	if (allowInitialSpaces) {
		while ((i < lengthLine) && isspacechar(lineBuffer[i]))	// Skip initial spaces
			i++;
	} else {
		if (isspacechar(lineBuffer[i])) // don't allow initial spaces
			i = lengthLine;
	}

	if (i < lengthLine) {
		if (lineBuffer[i] == '#' || lineBuffer[i] == '!' || lineBuffer[i] == ';') {
			styler.ColourTo(endPos, SCE_PROPS_COMMENT);
		} else if (lineBuffer[i] == '[') {
			styler.ColourTo(endPos, SCE_PROPS_SECTION);
		} else if (lineBuffer[i] == '@') {
			styler.ColourTo(startLine + i, SCE_PROPS_DEFVAL);
			if (isassignchar(lineBuffer[i++]))
				styler.ColourTo(startLine + i, SCE_PROPS_ASSIGNMENT);
			styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
		} else {
			// Search for the '=' character
			while ((i < lengthLine) && !isassignchar(lineBuffer[i]))
				i++;
			if ((i < lengthLine) && isassignchar(lineBuffer[i])) {
				styler.ColourTo(startLine + i - 1, SCE_PROPS_KEY);
				styler.ColourTo(startLine + i, SCE_PROPS_ASSIGNMENT);
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			} else {
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			}
		}
	} else {
		styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
	}
}

static void ColourisePropsDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	char lineBuffer[1024];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;
	unsigned int startLine = startPos;

	// property lexer.props.allow.initial.spaces
	//	For properties files, set to 0 to style all lines that start with whitespace in the default style.
	//	This is not suitable for SciTE .properties files which use indentation for flow control but
	//	can be used for RFC2822 text where indentation is used for continuation lines.
	bool allowInitialSpaces = styler.GetPropertyInt("lexer.props.allow.initial.spaces", 1) != 0;

	for (unsigned int i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColourisePropsLine(lineBuffer, linePos, startLine, i, styler, allowInitialSpaces);
			linePos = 0;
			startLine = i + 1;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		ColourisePropsLine(lineBuffer, linePos, startLine, startPos + length - 1, styler, allowInitialSpaces);
	}
}

// adaption by ksc, using the "} else {" trick of 1.53
// 030721
static void FoldPropsDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);

	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	bool headerPoint = false;
	int lev;

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler[i+1];

		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (style == SCE_PROPS_SECTION) {
			headerPoint = true;
		}

		if (atEOL) {
			lev = SC_FOLDLEVELBASE;

			if (lineCurrent > 0) {
				int levelPrevious = styler.LevelAt(lineCurrent - 1);

				if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
					lev = SC_FOLDLEVELBASE + 1;
				} else {
					lev = levelPrevious & SC_FOLDLEVELNUMBERMASK;
				}
			}

			if (headerPoint) {
				lev = SC_FOLDLEVELBASE;
			}
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;

			if (headerPoint) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}

			lineCurrent++;
			visibleChars = 0;
			headerPoint = false;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}

	if (lineCurrent > 0) {
		int levelPrevious = styler.LevelAt(lineCurrent - 1);
		if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
			lev = SC_FOLDLEVELBASE + 1;
		} else {
			lev = levelPrevious & SC_FOLDLEVELNUMBERMASK;
		}
	} else {
		lev = SC_FOLDLEVELBASE;
	}
	int flagsNext = styler.LevelAt(lineCurrent);
	styler.SetLevel(lineCurrent, lev | (flagsNext & ~SC_FOLDLEVELNUMBERMASK));
}

static void ColouriseMakeLine(
    char *lineBuffer,
    unsigned int lengthLine,
    unsigned int startLine,
    unsigned int endPos,
    Accessor &styler) {

	unsigned int i = 0;
	int lastNonSpace = -1;
	unsigned int state = SCE_MAKE_DEFAULT;
	bool bSpecial = false;

	// check for a tab character in column 0 indicating a command
	bool bCommand = false;
	if ((lengthLine > 0) && (lineBuffer[0] == '\t'))
		bCommand = true;

	// Skip initial spaces
	while ((i < lengthLine) && isspacechar(lineBuffer[i])) {
		i++;
	}
	if (lineBuffer[i] == '#') {	// Comment
		styler.ColourTo(endPos, SCE_MAKE_COMMENT);
		return;
	}
	if (lineBuffer[i] == '!') {	// Special directive
		styler.ColourTo(endPos, SCE_MAKE_PREPROCESSOR);
		return;
	}
	int varCount = 0;
	while (i < lengthLine) {
		if (lineBuffer[i] == '$' && lineBuffer[i + 1] == '(') {
			styler.ColourTo(startLine + i - 1, state);
			state = SCE_MAKE_IDENTIFIER;
			varCount++;
		} else if (state == SCE_MAKE_IDENTIFIER && lineBuffer[i] == ')') {
			if (--varCount == 0) {
				styler.ColourTo(startLine + i, state);
				state = SCE_MAKE_DEFAULT;
			}
		}

		// skip identifier and target styling if this is a command line
		if (!bSpecial && !bCommand) {
			if (lineBuffer[i] == ':') {
				if (((i + 1) < lengthLine) && (lineBuffer[i + 1] == '=')) {
					// it's a ':=', so style as an identifier
					if (lastNonSpace >= 0)
						styler.ColourTo(startLine + lastNonSpace, SCE_MAKE_IDENTIFIER);
					styler.ColourTo(startLine + i - 1, SCE_MAKE_DEFAULT);
					styler.ColourTo(startLine + i + 1, SCE_MAKE_OPERATOR);
				} else {
					// We should check that no colouring was made since the beginning of the line,
					// to avoid colouring stuff like /OUT:file
					if (lastNonSpace >= 0)
						styler.ColourTo(startLine + lastNonSpace, SCE_MAKE_TARGET);
					styler.ColourTo(startLine + i - 1, SCE_MAKE_DEFAULT);
					styler.ColourTo(startLine + i, SCE_MAKE_OPERATOR);
				}
				bSpecial = true;	// Only react to the first ':' of the line
				state = SCE_MAKE_DEFAULT;
			} else if (lineBuffer[i] == '=') {
				if (lastNonSpace >= 0)
					styler.ColourTo(startLine + lastNonSpace, SCE_MAKE_IDENTIFIER);
				styler.ColourTo(startLine + i - 1, SCE_MAKE_DEFAULT);
				styler.ColourTo(startLine + i, SCE_MAKE_OPERATOR);
				bSpecial = true;	// Only react to the first '=' of the line
				state = SCE_MAKE_DEFAULT;
			}
		}
		if (!isspacechar(lineBuffer[i])) {
			lastNonSpace = i;
		}
		i++;
	}
	if (state == SCE_MAKE_IDENTIFIER) {
		styler.ColourTo(endPos, SCE_MAKE_IDEOL);	// Error, variable reference not ended
	} else {
		styler.ColourTo(endPos, SCE_MAKE_DEFAULT);
	}
}

static void ColouriseMakeDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	char lineBuffer[1024];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;
	unsigned int startLine = startPos;
	for (unsigned int i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseMakeLine(lineBuffer, linePos, startLine, i, styler);
			linePos = 0;
			startLine = i + 1;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		ColouriseMakeLine(lineBuffer, linePos, startLine, startPos + length - 1, styler);
	}
}

static int RecogniseErrorListLine(const char *lineBuffer, unsigned int lengthLine, int &startValue) {
	if (lineBuffer[0] == '>') {
		// Command or return status
		return SCE_ERR_CMD;
	} else if (lineBuffer[0] == '<') {
		// Diff removal.
		return SCE_ERR_DIFF_DELETION;
	} else if (lineBuffer[0] == '!') {
		return SCE_ERR_DIFF_CHANGED;
	} else if (lineBuffer[0] == '+') {
		if (strstart(lineBuffer, "+++ ")) {
			return SCE_ERR_DIFF_MESSAGE;
		} else {
			return SCE_ERR_DIFF_ADDITION;
		}
	} else if (lineBuffer[0] == '-') {
		if (strstart(lineBuffer, "--- ")) {
			return SCE_ERR_DIFF_MESSAGE;
		} else {
			return SCE_ERR_DIFF_DELETION;
		}
	} else if (strstart(lineBuffer, "cf90-")) {
		// Absoft Pro Fortran 90/95 v8.2 error and/or warning message
		return SCE_ERR_ABSF;
	} else if (strstart(lineBuffer, "fortcom:")) {
		// Intel Fortran Compiler v8.0 error/warning message
		return SCE_ERR_IFORT;
	} else if (strstr(lineBuffer, "File \"") && strstr(lineBuffer, ", line ")) {
		return SCE_ERR_PYTHON;
	} else if (strstr(lineBuffer, " in ") && strstr(lineBuffer, " on line ")) {
		return SCE_ERR_PHP;
	} else if ((strstart(lineBuffer, "Error ") ||
	            strstart(lineBuffer, "Warning ")) &&
	           strstr(lineBuffer, " at (") &&
	           strstr(lineBuffer, ") : ") &&
	           (strstr(lineBuffer, " at (") < strstr(lineBuffer, ") : "))) {
		// Intel Fortran Compiler error/warning message
		return SCE_ERR_IFC;
	} else if (strstart(lineBuffer, "Error ")) {
		// Borland error message
		return SCE_ERR_BORLAND;
	} else if (strstart(lineBuffer, "Warning ")) {
		// Borland warning message
		return SCE_ERR_BORLAND;
	} else if (strstr(lineBuffer, "at line ") &&
	        (strstr(lineBuffer, "at line ") < (lineBuffer + lengthLine)) &&
	           strstr(lineBuffer, "file ") &&
	           (strstr(lineBuffer, "file ") < (lineBuffer + lengthLine))) {
		// Lua 4 error message
		return SCE_ERR_LUA;
	} else if (strstr(lineBuffer, " at ") &&
	        (strstr(lineBuffer, " at ") < (lineBuffer + lengthLine)) &&
	           strstr(lineBuffer, " line ") &&
	           (strstr(lineBuffer, " line ") < (lineBuffer + lengthLine)) &&
	        (strstr(lineBuffer, " at ") < (strstr(lineBuffer, " line ")))) {
		// perl error message
		return SCE_ERR_PERL;
	} else if ((memcmp(lineBuffer, "   at ", 6) == 0) &&
	           strstr(lineBuffer, ":line ")) {
		// A .NET traceback
		return SCE_ERR_NET;
	} else if (strstart(lineBuffer, "Line ") &&
	           strstr(lineBuffer, ", file ")) {
		// Essential Lahey Fortran error message
		return SCE_ERR_ELF;
	} else if (strstart(lineBuffer, "line ") &&
	           strstr(lineBuffer, " column ")) {
		// HTML tidy style: line 42 column 1
		return SCE_ERR_TIDY;
	} else if (strstart(lineBuffer, "\tat ") &&
	           strstr(lineBuffer, "(") &&
	           strstr(lineBuffer, ".java:")) {
		// Java stack back trace
		return SCE_ERR_JAVA_STACK;
	} else {
		// Look for one of the following formats:
		// GCC: <filename>:<line>:<message>
		// Microsoft: <filename>(<line>) :<message>
		// Common: <filename>(<line>): warning|error|note|remark|catastrophic|fatal
		// Common: <filename>(<line>) warning|error|note|remark|catastrophic|fatal
		// Microsoft: <filename>(<line>,<column>)<message>
		// CTags: \t<message>
		// Lua 5 traceback: \t<filename>:<line>:<message>
		// Lua 5.1: <exe>: <filename>:<line>:<message>
		bool initialTab = (lineBuffer[0] == '\t');
		bool initialColonPart = false;
		enum { stInitial,
			stGccStart, stGccDigit, stGccColumn, stGcc,
			stMsStart, stMsDigit, stMsBracket, stMsVc, stMsDigitComma, stMsDotNet,
			stCtagsStart, stCtagsStartString, stCtagsStringDollar, stCtags,
			stUnrecognized
		} state = stInitial;
		for (unsigned int i = 0; i < lengthLine; i++) {
			char ch = lineBuffer[i];
			char chNext = ' ';
			if ((i + 1) < lengthLine)
				chNext = lineBuffer[i + 1];
			if (state == stInitial) {
				if (ch == ':') {
					// May be GCC, or might be Lua 5 (Lua traceback same but with tab prefix)
					if ((chNext != '\\') && (chNext != '/') && (chNext != ' ')) {
						// This check is not completely accurate as may be on
						// GTK+ with a file name that includes ':'.
						state = stGccStart;
					} else if (chNext == ' ') { // indicates a Lua 5.1 error message
						initialColonPart = true;
					}
				} else if ((ch == '(') && Is1To9(chNext) && (!initialTab)) {
					// May be Microsoft
					// Check against '0' often removes phone numbers
					state = stMsStart;
				} else if ((ch == '\t') && (!initialTab)) {
					// May be CTags
					state = stCtagsStart;
				}
			} else if (state == stGccStart) {	// <filename>:
				state = Is1To9(ch) ? stGccDigit : stUnrecognized;
			} else if (state == stGccDigit) {	// <filename>:<line>
				if (ch == ':') {
					state = stGccColumn;	// :9.*: is GCC
					startValue = i + 1;
				} else if (!Is0To9(ch)) {
					state = stUnrecognized;
				}
			} else if (state == stGccColumn) {	// <filename>:<line>:<column>
				if (!Is0To9(ch)) {
					state = stGcc;
					if (ch == ':')
						startValue = i + 1;
					break;
				}
			} else if (state == stMsStart) {	// <filename>(
				state = Is0To9(ch) ? stMsDigit : stUnrecognized;
			} else if (state == stMsDigit) {	// <filename>(<line>
				if (ch == ',') {
					state = stMsDigitComma;
				} else if (ch == ')') {
					state = stMsBracket;
				} else if ((ch != ' ') && !Is0To9(ch)) {
					state = stUnrecognized;
				}
			} else if (state == stMsBracket) {	// <filename>(<line>)
				if ((ch == ' ') && (chNext == ':')) {
					state = stMsVc;
				} else if ((ch == ':' && chNext == ' ') || (ch == ' ')) {
					// Possibly Delphi.. don't test against chNext as it's one of the strings below.
					char word[512];
					unsigned int j, chPos;
					unsigned numstep;
					chPos = 0;
					if (ch == ' ')
						numstep = 1; // ch was ' ', handle as if it's a delphi errorline, only add 1 to i.
					else
						numstep = 2; // otherwise add 2.
					for (j = i + numstep; j < lengthLine && IsAlphabetic(lineBuffer[j]) && chPos < sizeof(word) - 1; j++)
						word[chPos++] = lineBuffer[j];
					word[chPos] = 0;
					if (!CompareCaseInsensitive(word, "error") || !CompareCaseInsensitive(word, "warning") ||
						!CompareCaseInsensitive(word, "fatal") || !CompareCaseInsensitive(word, "catastrophic") ||
						!CompareCaseInsensitive(word, "note") || !CompareCaseInsensitive(word, "remark")) {
						state = stMsVc;
					} else
						state = stUnrecognized;
				} else {
					state = stUnrecognized;
				}
			} else if (state == stMsDigitComma) {	// <filename>(<line>,
				if (ch == ')') {
					state = stMsDotNet;
					break;
				} else if ((ch != ' ') && !Is0To9(ch)) {
					state = stUnrecognized;
				}
			} else if (state == stCtagsStart) {
				if ((lineBuffer[i - 1] == '\t') &&
				        ((ch == '/' && lineBuffer[i + 1] == '^') || Is0To9(ch))) {
					state = stCtags;
					break;
				} else if ((ch == '/') && (lineBuffer[i + 1] == '^')) {
					state = stCtagsStartString;
				}
			} else if ((state == stCtagsStartString) && ((lineBuffer[i] == '$') && (lineBuffer[i + 1] == '/'))) {
				state = stCtagsStringDollar;
				break;
			}
		}
		if (state == stGcc) {
			return initialColonPart ? SCE_ERR_LUA : SCE_ERR_GCC;
		} else if ((state == stMsVc) || (state == stMsDotNet)) {
			return SCE_ERR_MS;
		} else if ((state == stCtagsStringDollar) || (state == stCtags)) {
			return SCE_ERR_CTAG;
		} else {
			return SCE_ERR_DEFAULT;
		}
	}
}

static void ColouriseErrorListLine(
    char *lineBuffer,
    unsigned int lengthLine,
    unsigned int endPos,
    Accessor &styler,
	bool valueSeparate) {
	int startValue = -1;
	int style = RecogniseErrorListLine(lineBuffer, lengthLine, startValue);
	if (valueSeparate && (startValue >= 0)) {
		styler.ColourTo(endPos - (lengthLine - startValue), style);
		styler.ColourTo(endPos, SCE_ERR_VALUE);
	} else {
		styler.ColourTo(endPos, style);
	}
}

static void ColouriseErrorListDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	char lineBuffer[10000];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;

	// property lexer.errorlist.value.separate
	//	For lines in the output pane that are matches from Find in Files or GCC-style
	//	diagnostics, style the path and line number separately from the rest of the
	//	line with style 21 used for the rest of the line.
	//	This allows matched text to be more easily distinguished from its location.
	bool valueSeparate = styler.GetPropertyInt("lexer.errorlist.value.separate", 0) != 0;
	for (unsigned int i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseErrorListLine(lineBuffer, linePos, i, styler, valueSeparate);
			linePos = 0;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		ColouriseErrorListLine(lineBuffer, linePos, startPos + length - 1, styler, valueSeparate);
	}
}

static bool latexIsSpecial(int ch) {
	return (ch == '#') || (ch == '$') || (ch == '%') || (ch == '&') || (ch == '_') ||
	       (ch == '{') || (ch == '}') || (ch == ' ');
}

static bool latexIsBlank(int ch) {
	return (ch == ' ') || (ch == '\t');
}

static bool latexIsBlankAndNL(int ch) {
	return (ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n');
}

static bool latexIsLetter(int ch) {
	return isascii(ch) && isalpha(ch);
}

static bool latexIsTagValid(int &i, int l, Accessor &styler) {
	while (i < l) {
		if (styler.SafeGetCharAt(i) == '{') {
			while (i < l) {
				i++;
				if (styler.SafeGetCharAt(i) == '}') {
					return true;
				}	else if (!latexIsLetter(styler.SafeGetCharAt(i)) &&
                   styler.SafeGetCharAt(i)!='*') {
					return false;
				}
			}
		} else if (!latexIsBlank(styler.SafeGetCharAt(i))) {
			return false;
		}
		i++;
	}
	return false;
}

static bool latexNextNotBlankIs(int i, int l, Accessor &styler, char needle) {
  char ch;
	while (i < l) {
    ch = styler.SafeGetCharAt(i);
		if (!latexIsBlankAndNL(ch) && ch != '*') {
      if (ch == needle)
        return true;
      else
        return false;
		}
		i++;
	}
	return false;
}

static bool latexLastWordIs(int start, Accessor &styler, const char *needle) {
  unsigned int i = 0;
	unsigned int l = static_cast<unsigned int>(strlen(needle));
	int ini = start-l+1;
	char s[32];

	while (i < l && i < 32) {
		s[i] = styler.SafeGetCharAt(ini + i);
    i++;
	}
	s[i] = '\0';

	return (strcmp(s, needle) == 0);
}

static void ColouriseLatexDoc(unsigned int startPos, int length, int initStyle,
                              WordList *[], Accessor &styler) {

	styler.StartAt(startPos);

	int state = initStyle;
	char chNext = styler.SafeGetCharAt(startPos);
	styler.StartSegment(startPos);
	int lengthDoc = startPos + length;
  char chVerbatimDelim = '\0';

	for (int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if (styler.IsLeadByte(ch)) {
			i++;
			chNext = styler.SafeGetCharAt(i + 1);
			continue;
		}

		switch (state) {
		case SCE_L_DEFAULT :
			switch (ch) {
			case '\\' :
				styler.ColourTo(i - 1, state);
				if (latexIsSpecial(chNext)) {
					state = SCE_L_SPECIAL;
				} else {
					if (latexIsLetter(chNext)) {
						state = SCE_L_COMMAND;
					}	else {
						if (chNext == '(' || chNext == '[') {
							styler.ColourTo(i-1, state);
							styler.ColourTo(i+1, SCE_L_SHORTCMD);
							state = SCE_L_MATH;
							if (chNext == '[')
								state = SCE_L_MATH2;
							i++;
							chNext = styler.SafeGetCharAt(i+1);
						} else {
							state = SCE_L_SHORTCMD;
						}
					}
				}
				break;
			case '$' :
				styler.ColourTo(i - 1, state);
				state = SCE_L_MATH;
				if (chNext == '$') {
					state = SCE_L_MATH2;
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
				}
				break;
			case '%' :
				styler.ColourTo(i - 1, state);
				state = SCE_L_COMMENT;
				break;
			}
			break;
		case SCE_L_ERROR:
			styler.ColourTo(i-1, state);
			state = SCE_L_DEFAULT;
			break;
		case SCE_L_SPECIAL:
		case SCE_L_SHORTCMD:
			styler.ColourTo(i, state);
			state = SCE_L_DEFAULT;
			break;
		case SCE_L_COMMAND :
			if (!latexIsLetter(chNext)) {
				styler.ColourTo(i, state);
				state = SCE_L_DEFAULT;
        if (latexNextNotBlankIs(i+1, lengthDoc, styler, '[' )) {
          state = SCE_L_CMDOPT;
				} else if (latexLastWordIs(i, styler, "\\begin")) {
					state = SCE_L_TAG;
				} else if (latexLastWordIs(i, styler, "\\end")) {
					state = SCE_L_TAG2;
				} else if (latexLastWordIs(i, styler, "\\verb") &&
                   chNext != '*' && chNext != ' ') {
          chVerbatimDelim = chNext;
					state = SCE_L_VERBATIM;
				}
			}
			break;
		case SCE_L_CMDOPT :
      if (ch == ']') {
        styler.ColourTo(i, state);
        state = SCE_L_DEFAULT;
      }
			break;
		case SCE_L_TAG :
			if (latexIsTagValid(i, lengthDoc, styler)) {
				styler.ColourTo(i, state);
				state = SCE_L_DEFAULT;
				if (latexLastWordIs(i, styler, "{verbatim}")) {
					state = SCE_L_VERBATIM;
				} else if (latexLastWordIs(i, styler, "{comment}")) {
					state = SCE_L_COMMENT2;
				} else if (latexLastWordIs(i, styler, "{math}")) {
					state = SCE_L_MATH;
				} else if (latexLastWordIs(i, styler, "{displaymath}")) {
					state = SCE_L_MATH2;
				} else if (latexLastWordIs(i, styler, "{equation}")) {
					state = SCE_L_MATH2;
				}
			} else {
				state = SCE_L_ERROR;
				styler.ColourTo(i, state);
				state = SCE_L_DEFAULT;
			}
			chNext = styler.SafeGetCharAt(i+1);
			break;
		case SCE_L_TAG2 :
			if (latexIsTagValid(i, lengthDoc, styler)) {
				styler.ColourTo(i, state);
				state = SCE_L_DEFAULT;
			} else {
				state = SCE_L_ERROR;
			}
			chNext = styler.SafeGetCharAt(i+1);
			break;
		case SCE_L_MATH :
			if (ch == '$') {
				styler.ColourTo(i, state);
				state = SCE_L_DEFAULT;
			} else if (ch == '\\' && chNext == ')') {
				styler.ColourTo(i-1, state);
				styler.ColourTo(i+1, SCE_L_SHORTCMD);
				i++;
				chNext = styler.SafeGetCharAt(i+1);
				state = SCE_L_DEFAULT;
			} else if (ch == '\\') {
				int match = i + 3;
				if (latexLastWordIs(match, styler, "\\end")) {
					match++;
					if (latexIsTagValid(match, lengthDoc, styler)) {
						if (latexLastWordIs(match, styler, "{math}")) {
							styler.ColourTo(i-1, state);
							state = SCE_L_COMMAND;
						}
					}
				}
			}

			break;
		case SCE_L_MATH2 :
			if (ch == '$') {
        if (chNext == '$') {
          i++;
          chNext = styler.SafeGetCharAt(i + 1);
          styler.ColourTo(i, state);
          state = SCE_L_DEFAULT;
        } else {
          styler.ColourTo(i, SCE_L_ERROR);
          state = SCE_L_DEFAULT;
        }
			} else if (ch == '\\' && chNext == ']') {
				styler.ColourTo(i-1, state);
				styler.ColourTo(i+1, SCE_L_SHORTCMD);
				i++;
				chNext = styler.SafeGetCharAt(i+1);
				state = SCE_L_DEFAULT;
			} else if (ch == '\\') {
				int match = i + 3;
				if (latexLastWordIs(match, styler, "\\end")) {
					match++;
					if (latexIsTagValid(match, lengthDoc, styler)) {
						if (latexLastWordIs(match, styler, "{displaymath}")) {
							styler.ColourTo(i-1, state);
							state = SCE_L_COMMAND;
						} else if (latexLastWordIs(match, styler, "{equation}")) {
							styler.ColourTo(i-1, state);
							state = SCE_L_COMMAND;
						}
					}
				}
			}
			break;
		case SCE_L_COMMENT :
			if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, state);
				state = SCE_L_DEFAULT;
			}
			break;
		case SCE_L_COMMENT2 :
			if (ch == '\\') {
				int match = i + 3;
				if (latexLastWordIs(match, styler, "\\end")) {
					match++;
					if (latexIsTagValid(match, lengthDoc, styler)) {
						if (latexLastWordIs(match, styler, "{comment}")) {
							styler.ColourTo(i-1, state);
							state = SCE_L_COMMAND;
						}
					}
				}
			}
			break;
		case SCE_L_VERBATIM :
			if (ch == '\\') {
				int match = i + 3;
				if (latexLastWordIs(match, styler, "\\end")) {
					match++;
					if (latexIsTagValid(match, lengthDoc, styler)) {
						if (latexLastWordIs(match, styler, "{verbatim}")) {
							styler.ColourTo(i-1, state);
							state = SCE_L_COMMAND;
						}
					}
				}
			} else if (chNext == chVerbatimDelim) {
        styler.ColourTo(i+1, state);
				state = SCE_L_DEFAULT;
        chVerbatimDelim = '\0';
      } else if (chVerbatimDelim != '\0' && (ch == '\n' || ch == '\r')) {
        styler.ColourTo(i, SCE_L_ERROR);
				state = SCE_L_DEFAULT;
        chVerbatimDelim = '\0';
      }
			break;
		}
	}
	styler.ColourTo(lengthDoc-1, state);
}

static const char *const batchWordListDesc[] = {
	"Internal Commands",
	"External Commands",
	0
};

static const char *const emptyWordListDesc[] = {
	0
};

static void ColouriseNullDoc(unsigned int startPos, int length, int, WordList *[],
                            Accessor &styler) {
	// Null language means all style bytes are 0 so just mark the end - no need to fill in.
	if (length > 0) {
		styler.StartAt(startPos + length - 1);
		styler.StartSegment(startPos + length - 1);
		styler.ColourTo(startPos + length - 1, 0);
	}
}

/*
#define SCE_NSCR_DEFAULT 0
#define SCE_NSCR_IDENTIFIER 1
#define SCE_NSCR_COMMENT_LINE 2
#define SCE_NSCR_COMMENT_BLOCK 3
#define SCE_NSCR_COMMAND 4
#define SCE_NSCR_OPTION 5
#define SCE_NSCR_FUNCTION 6
#define SCE_NSCR_METHOD 7
#define SCE_NSCR_CONSTANTS 8
#define SCE_NSCR_PREDEFS 9
#define SCE_NSCR_STRING 10
#define SCE_NSCR_STRING_PARSER 11
#define SCE_NSCR_DEFAULT_VARS 12
#define SCE_NSCR_OPERATORS 13
#define SCE_NSCR_OPERATOR_KEYWORDS 14
#define SCE_NSCR_PROCEDURES 15
#define SCE_NSCR_INCLUDES 16
#define SCE_NSCR_NUMBERS 17
#define SCE_NSCR_CUSTOM_FUNCTION 18
#define SCE_NSCR_CLUSTER 19
#define SCE_NSCR_DOCCOMMENT_LINE 20
#define SCE_NSCR_DOCCOMMENT_BLOCK 21
#define SCE_NSCR_DOCKEYWORD 22
#define SCE_NSCR_INSTALL 23
#define SCE_NSCR_PROCEDURE_COMMANDS 24

#define SCE_NPRC_DEFAULT SCE_NSCR_DEFAULT
#define SCE_NPRC_IDENTIFIER SCE_NSCR_IDENTIFIER
#define SCE_NPRC_COMMENT_LINE SCE_NSCR_COMMENT_LINE
#define SCE_NPRC_COMMENT_BLOCK SCE_NSCR_COMMENT_BLOCK
#define SCE_NPRC_COMMAND SCE_NSCR_COMMAND
#define SCE_NPRC_OPTION SCE_NSCR_OPTION
#define SCE_NPRC_FUNCTION SCE_NSCR_FUNCTION
#define SCE_NPRC_METHOD SCE_NSCR_METHOD
#define SCE_NPRC_CONSTANTS SCE_NSCR_CONSTANTS
#define SCE_NPRC_PREDEFS SCE_NSCR_PREDEFS
#define SCE_NPRC_STRING SCE_NSCR_STRING
#define SCE_NPRC_STRING_PARSER SCE_NSCR_STRING_PARSER
#define SCE_NPRC_DEFAULT_VARS SCE_NSCR_DEFAULT_VARS
#define SCE_NPRC_OPERATORS SCE_NSCR_OPERATORS
#define SCE_NPRC_OPERATOR_KEYWORDS SCE_NSCR_OPERATOR_KEYWORDS
#define SCE_NPRC_PROCEDURES SCE_NSCR_PROCEDURES
#define SCE_NPRC_INCLUDES SCE_NSCR_INCLUDES
#define SCE_NPRC_NUMBERS SCE_NSCR_NUMBERS
#define SCE_NPRC_CUSTOM_FUNCTION SCE_NSCR_CUSTOM_FUNCTION
#define SCE_NPRC_CLUSTER SCE_NSCR_CLUSTER
#define SCE_NPRC_DOCCOMMENT_LINE SCE_NSCR_DOCCOMMENT_LINE
#define SCE_NPRC_DOCCOMMENT_BLOCK SCE_NSCR_DOCCOMMENT_BLOCK
#define SCE_NPRC_DOCKEYWORD SCE_NSCR_DOCKEYWORD
#define SCE_NPRC_FLAGS 23


#define SCE_TXTADV_DEFAULT 0
#define SCE_TXTADV_MODIFIER 1
#define SCE_TXTADV_ITALIC 2
#define SCE_TXTADV_BOLD 3
#define SCE_TXTADV_BOLD_ITALIC 4
#define SCE_TXTADV_UNDERLINE 5
#define SCE_TXTADV_STRIKETHROUGH 6
#define SCE_TXTADV_URL 7
#define SCE_TXTADV_HEAD 8
#define SCE_TXTADV_BIGHEAD 9


*/

/* Nested comments require keeping the value of the nesting level for every
   position in the document.  But since scintilla always styles line by line,
   we only need to store one value per line. The non-negative number indicates
   nesting level at the end of the line.
*/

// Underscore, letter, digit and universal alphas from C99 Appendix D.

static bool IsWordStart(int ch) {
	return (isascii(ch) && (isalpha(ch) || ch == '_' || ch == '\'')) || !isascii(ch);
}

static bool IsWord(int ch) {
	return (isascii(ch) && (isalnum(ch) || ch == '_')) || !isascii(ch);
}

static bool IsOperator(int ch)
{
	if (IsAlphaNumeric(ch))
		return false;
	if (ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
	        ch == '^' || ch == ')' || ch == '(' || ch == '?' ||
	        ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
	        ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
	        ch == '<' || ch == '>' || ch == ',' || ch == '!' ||
	        ch == '&' || ch == '%' || ch == '\\')
		return true;
	return false;
}
/*
static bool IsDoxygen(int ch) {
	if (isascii(ch) && islower(ch))
		return true;
	if (ch == '$' || ch == '@' || ch == '\\' ||
		ch == '&' || ch == '#' || ch == '<' || ch == '>' ||
		ch == '{' || ch == '}' || ch == '[' || ch == ']')
		return true;
	return false;
}*/

static bool IsStringSuffix(int ch) {
	return ch == 'c' || ch == 'w' || ch == 'd';
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_D_COMMENT ||
		style == SCE_D_COMMENTDOC ||
		style == SCE_D_COMMENTDOCKEYWORD ||
		style == SCE_D_COMMENTDOCKEYWORDERROR;
}

// An individual named option for use in an OptionSet

// Options used for LexerNSCR
struct OptionsNSCR {
	bool fold;
	bool foldSyntaxBased;
	bool foldComment;
	bool foldCommentMultiline;
	bool foldCommentExplicit;
	std::string foldExplicitStart;
	std::string foldExplicitEnd;
	bool foldExplicitAnywhere;
	bool foldCompact;
	bool foldAtElse;
	OptionsNSCR() {
		fold = false;
		foldSyntaxBased = true;
		foldComment = false;
		foldCommentMultiline = true;
		foldCommentExplicit = true;
		foldExplicitStart = "";
		foldExplicitEnd   = "";
		foldExplicitAnywhere = false;
		foldCompact = false;
		foldAtElse = true;
	}
};

static const char * const NSCRWordLists[] = {
			"Commands",
			"Options",
			"Functions",
			"Methods",
			"Predefined variables",
			"Constants",
			"Special predefs",
			"Operator keywords",
			"Documentation keywords",
			"Procedure commands",
			0
		};

struct OptionSetNSCR : public OptionSet<OptionsNSCR> {
	OptionSetNSCR() {
		DefineProperty("fold", &OptionsNSCR::fold);

		DefineProperty("fold.nscr.syntax.based", &OptionsNSCR::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("fold.comment", &OptionsNSCR::foldComment);

		DefineProperty("fold.nscr.comment.multiline", &OptionsNSCR::foldCommentMultiline,
			"Set this property to 0 to disable folding multi-line comments when fold.comment=1.");

		DefineProperty("fold.nscr.comment.explicit", &OptionsNSCR::foldCommentExplicit,
			"Set this property to 0 to disable folding explicit fold points when fold.comment=1.");

		DefineProperty("fold.nscr.explicit.start", &OptionsNSCR::foldExplicitStart,
			"The string to use for explicit fold start points, replacing the standard //{.");

		DefineProperty("fold.nscr.explicit.end", &OptionsNSCR::foldExplicitEnd,
			"The string to use for explicit fold end points, replacing the standard //}.");

		DefineProperty("fold.nscr.explicit.anywhere", &OptionsNSCR::foldExplicitAnywhere,
			"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

		DefineProperty("fold.compact", &OptionsNSCR::foldCompact);

		DefineProperty("fold.at.else", &OptionsNSCR::foldAtElse);

		DefineWordListSets(NSCRWordLists);
	}
};

class LexerNSCR : public ILexer {
	bool caseSensitive;
	WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	WordList keywords5;
	WordList keywords6;
	WordList keywords7;
	WordList keywords8;
	WordList keywords9;
	WordList keywords10;
	/*
	"Commands",
	"Options",
	"Functions",
	"Predefined variables",
	"Constants",
	"Special predefs",
	"Operator keywords",
	0,
	*/
	OptionsNSCR options;
	OptionSetNSCR osNSCR;
public:
	LexerNSCR(bool caseSensitive_) :
		caseSensitive(caseSensitive_) {
	}
	virtual ~LexerNSCR() {
	}
	void SCI_METHOD Release() {
		delete this;
	}
	int SCI_METHOD Version() const {
		return lvOriginal;
	}
	const char * SCI_METHOD PropertyNames() {
		return osNSCR.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) {
		return osNSCR.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) {
		return osNSCR.DescribeProperty(name);
	}
	int SCI_METHOD PropertySet(const char *key, const char *val);
	const char * SCI_METHOD DescribeWordListSets() {
		return osNSCR.DescribeWordListSets();
	}
	int SCI_METHOD WordListSet(int n, const char *wl);
	void SCI_METHOD Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess);

	void * SCI_METHOD PrivateCall(int, void *) {
		return 0;
	}

	static ILexer *LexerFactoryNSCR() {
		return new LexerNSCR(true);
	}
	static ILexer *LexerFactoryNSCRInsensitive() {
		return new LexerNSCR(false);
	}
};

int SCI_METHOD LexerNSCR::PropertySet(const char *key, const char *val) {
	if (osNSCR.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

int SCI_METHOD LexerNSCR::WordListSet(int n, const char *wl) {
	WordList *wordListN = 0;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
	case 1:
		wordListN = &keywords2;
		break;
	case 2:
		wordListN = &keywords3;
		break;
	case 3:
		wordListN = &keywords4;
		break;
	case 4:
		wordListN = &keywords5;
		break;
	case 5:
		wordListN = &keywords6;
		break;
	case 6:
		wordListN = &keywords7;
		break;
	case 7:
		wordListN = &keywords8;
		break;
	case 8:
		wordListN = &keywords9;
		break;
	case 9:
		wordListN = &keywords10;
		break;
	}
	int firstModification = -1;
	if (wordListN) {
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
		}
	}
	return firstModification;
}

void SCI_METHOD LexerNSCR::Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess)
{
	LexAccessor styler(pAccess);
	int nInstallStart = -1;
	
	if (startPos > 0 && (styler.StyleAt(startPos) == SCE_NSCR_INSTALL || styler.StyleAt(startPos-1) == SCE_NSCR_INSTALL || styler.StyleAt(startPos) == SCE_NSCR_PROCEDURE_COMMANDS || styler.StyleAt(startPos-1) == SCE_NSCR_PROCEDURE_COMMANDS))
	{
		startPos--;
		length++;
		
		while (startPos > 0 && (styler.StyleAt(startPos) == SCE_NSCR_INSTALL || styler.StyleAt(startPos) == SCE_NSCR_PROCEDURE_COMMANDS))
		{
			startPos--;
			length++;
		}
		
		while (startPos+length < styler.Length() && (styler.StyleAt(startPos+length) == SCE_NSCR_INSTALL || styler.StyleAt(startPos+length) == SCE_NSCR_PROCEDURE_COMMANDS))
			length++;
		
		initStyle = styler.StyleAt(startPos);
		
		//if (initStyle == SCE_NSCR_INSTALL)
		//	initStyle = SCE_NSCR_DEFAULT;
	}

	int styleBeforeDCKeyword = SCE_NSCR_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	int curLine = styler.GetLine(startPos);
	int curNcLevel = curLine > 0? styler.GetLineState(curLine-1): 0;
	bool numFloat = false; // Float literals have '+' and '-' signs
	bool numHex = false;
	bool possibleMethod = false;

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			curLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(curLine, curNcLevel);
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_NSCR_OPERATORS:
				sc.SetState(SCE_NSCR_DEFAULT);
				break;
			case SCE_NSCR_OPERATOR_KEYWORDS:
				if (sc.ch == '>')
				{
					sc.ForwardSetState(SCE_NSCR_DEFAULT);
				}
				break;
			case SCE_NSCR_NUMBERS:
				// We accept almost anything because of hex. and number suffixes
				if (isdigit(sc.ch))
				{
					continue;
				}
				else if (sc.ch == '.' && sc.chNext != '.' && !numFloat)
				{
					// Don't parse 0..2 as number.
					numFloat=true;
					continue;
				}
				else if (( sc.ch == '-' || sc.ch == '+' )
					&& (sc.chPrev == 'e' || sc.chPrev == 'E' ))
				{
					// Parse exponent sign in float literals: 2e+10 0x2e+10
					continue;
				}
				else if (( sc.ch == 'e' || sc.ch == 'E' )
					&& (sc.chNext == '+' || sc.chNext == '-' || isdigit(sc.chNext)))
				{
					// Parse exponent sign in float literals: 2e+10 0x2e+10
					continue;
				}
				else
				{
					sc.SetState(SCE_NSCR_DEFAULT);
				}
				break;
			case SCE_NSCR_STRING_PARSER:
				if (!IsWord(sc.ch) && sc.ch != '~')
				{
					sc.SetState(SCE_NSCR_DEFAULT);
				}
				break;
			case SCE_NSCR_IDENTIFIER:
				if (!IsWord(sc.ch)) /** TODO*/
				{
					// reverse identify the currently marched identifiers depending on the word list
					char s[1000];
					if (caseSensitive)
					{
						sc.GetCurrent(s, sizeof(s));
					}
					else
					{
						sc.GetCurrentLowered(s, sizeof(s));
					}
					/*
					"Commands",
					"Options",
					"Functions",
					"Methods",
					"Predefined variables",
					"Constants",
					"Special predefs",
					"Operator keywords",
					0,
					*/
					if (keywords.InList(s) && nInstallStart == -1)
					{
						sc.ChangeState(SCE_NSCR_COMMAND);
					}
					else if (keywords3.InList(s) && sc.ch == '(')
					{
						sc.ChangeState(SCE_NSCR_FUNCTION);
					}
					else if (keywords4.InList(s) && possibleMethod)
					{
						sc.ChangeState(SCE_NSCR_METHOD);
					}
					else if (keywords5.InList(s))
					{
						sc.ChangeState(SCE_NSCR_DEFAULT_VARS);
					}
					else if (keywords6.InList(s))
					{
						sc.ChangeState(SCE_NSCR_CONSTANTS);
					}
					else if (keywords7.InList(s))
					{
						sc.ChangeState(SCE_NSCR_PREDEFS);
					}
					else if (keywords8.InList(s))
					{
						sc.ChangeState(SCE_NSCR_OPERATOR_KEYWORDS);
					}
					else if (keywords10.InList(s))
					{
						sc.ChangeState(SCE_NSCR_PROCEDURE_COMMANDS);
					}
					else if (sc.ch == '(')
					{
						sc.ChangeState(SCE_NSCR_CUSTOM_FUNCTION);
					}
					else if (sc.ch == '{')
					{
						sc.ChangeState(SCE_NSCR_CLUSTER);
					}
					else if (keywords2.InList(s))
					{
						sc.ChangeState(SCE_NSCR_OPTION);
					}
					possibleMethod = false;
					sc.SetState(SCE_NSCR_DEFAULT);
				}
				break;
			case SCE_NSCR_COMMENT_BLOCK:
			case SCE_NSCR_DOCCOMMENT_BLOCK:
				if (sc.Match('*', '#')) {
					sc.Forward();
					sc.ForwardSetState(SCE_NSCR_DEFAULT);
				}
				break;
			case SCE_NSCR_INCLUDES:
			case SCE_NSCR_COMMENT_LINE:
			case SCE_NSCR_DOCCOMMENT_LINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_NSCR_DEFAULT);
				}
				break;
			case SCE_NSCR_STRING:
				if (sc.ch == '\\')
				{
					if (sc.chNext == '"' || sc.chNext == '\\')
					{
						sc.Forward();
					}
				}
				else if (sc.ch == '"')
				{
					sc.ForwardSetState(SCE_NSCR_DEFAULT);
				}
				break;
			case SCE_NSCR_FUNCTION:
			case SCE_NSCR_PROCEDURES:
				if (sc.ch == ' ' || sc.ch == '(' || sc.atLineStart)
				{
					sc.SetState(SCE_NSCR_DEFAULT);
				}
				break;
		}
		
		// Forward search for documentation keywords
		if ((sc.state == SCE_NSCR_DOCCOMMENT_LINE || sc.state == SCE_NSCR_DOCCOMMENT_BLOCK) && sc.Match('\\'))
		{
			int nCurrentState = sc.state;
			sc.SetState(SCE_NSCR_DEFAULT);
			sc.Forward();
			
			while (sc.More() && IsWord(sc.ch))
				sc.Forward();
			
			char s[1000];
			sc.GetCurrent(s, sizeof(s));

			if (keywords9.InList(s))
				sc.ChangeState(SCE_NSCR_DOCKEYWORD);
			else
				sc.ChangeState(nCurrentState);
			
			sc.SetState(nCurrentState);
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_NSCR_DEFAULT)
		{
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext)))
			{
				sc.SetState(SCE_NSCR_NUMBERS);
				numFloat = sc.ch == '.';
			}
			else if (sc.ch == '.' && IsWordStart(static_cast<char>(sc.chNext)))
				possibleMethod = true;
			else if (IsWordStart(static_cast<char>(sc.ch)))
			{
				sc.SetState(SCE_NSCR_IDENTIFIER);
			}
			else if (sc.ch == '$')
			{
				sc.SetState(SCE_NSCR_PROCEDURES);
			}
			else if (sc.Match("#*!"))
			{
				sc.SetState(SCE_NSCR_DOCCOMMENT_BLOCK);
				sc.Forward();   // Eat the * so it isn't used for the end of the comment
			}
			else if (sc.Match("##!"))
			{
				sc.SetState(SCE_NSCR_DOCCOMMENT_LINE);
				sc.Forward();
			}
			else if (sc.Match('#', '*'))
			{
				sc.SetState(SCE_NSCR_COMMENT_BLOCK);
				sc.Forward();   // Eat the * so it isn't used for the end of the comment
			}
			else if (sc.Match('#', '#'))
			{
				sc.SetState(SCE_NSCR_COMMENT_LINE);
				sc.Forward();
			}
			else if (sc.ch == '"')
			{
				sc.SetState(SCE_NSCR_STRING);
			}
			else if (sc.ch == '#')
			{
				sc.SetState(SCE_NSCR_STRING_PARSER);
			}
			else if (sc.ch == '@')
			{
				sc.SetState(SCE_NSCR_INCLUDES);
			}
			else if (IsOperator(static_cast<char>(sc.ch)))
			{
				if (sc.ch == '<'
					&& (sc.Match("<wp>") || sc.Match("<this>") || sc.Match("<loadpath>") || sc.Match("<savepath>") || sc.Match("<plotpath>") || sc.Match("<procpath>") || sc.Match("<scriptpath>")))
				{
					sc.SetState(SCE_NSCR_OPERATOR_KEYWORDS);
				}
				else if ((sc.ch == '<') && sc.Match("<install>"))
				{
					nInstallStart = sc.currentPos;
				}
				else if ((sc.ch == '<') && sc.Match("<endinstall>") && nInstallStart != -1)
				{
					styler.Flush();
					styler.StartAt(nInstallStart);
					styler.StartSegment(nInstallStart);
					
					for (int i = nInstallStart; i <= sc.currentPos; i++)
					{
						if (styler.StyleAt(i) == SCE_NSCR_PROCEDURE_COMMANDS)
						{
							styler.ColourTo(i-1, SCE_NSCR_INSTALL);
							
							while (styler.StyleAt(i+1) == SCE_NSCR_PROCEDURE_COMMANDS)
								i++;
							
							styler.ColourTo(i, SCE_NSCR_PROCEDURE_COMMANDS);
						}
					}
					
					styler.ColourTo(sc.currentPos+11, SCE_NSCR_INSTALL);
					sc.Forward(11);
					//sc.SetState(SCE_NSCR_DEFAULT);
					nInstallStart = -1;
				}
				else
				{
					sc.SetState(SCE_NSCR_OPERATORS);
				}
			}
		}
	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".

void SCI_METHOD LexerNSCR::Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess)
{

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	bool foldAtElse = options.foldAtElse;
	bool foundElse = false;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (unsigned int i = startPos; i < endPos; i++)
	{
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (options.foldComment && options.foldCommentMultiline)
		{
			if (!(stylePrev == SCE_NSCR_COMMENT_BLOCK))
			{
				levelNext++;
			}
			else if (!(styleNext == SCE_NSCR_COMMENT_BLOCK) && !atEOL)
			{
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldComment && options.foldCommentExplicit && ((style == SCE_NSCR_COMMENT_LINE) || options.foldExplicitAnywhere))
		{
			if (userDefinedFoldMarkers)
			{
				if (styler.Match(i, options.foldExplicitStart.c_str()))
				{
 					levelNext++;
				}
				else if (styler.Match(i, options.foldExplicitEnd.c_str()))
				{
 					levelNext--;
 				}
			}
			else
			{
				if ((ch == '#') && (chNext == '#'))  //##{
				{
					char chNext2 = styler.SafeGetCharAt(i + 2);
					if (chNext2 == '{')
					{
						levelNext++;
					}
					else if (chNext2 == '}')
					{
						levelNext--;
					}
				}
 			}
 		}
		if (options.foldSyntaxBased && (style == SCE_NSCR_IDENTIFIER || style == SCE_NSCR_COMMAND || style == SCE_NSCR_INSTALL || style == SCE_NSCR_PROCEDURE_COMMANDS))
		{
			bool isCommand = style == SCE_NSCR_COMMAND || style == SCE_NSCR_PROCEDURE_COMMANDS;
			
			if (isCommand && (styler.Match(i, "endif")
				|| styler.Match(i, "endfor")
				|| styler.Match(i, "endwhile")
				|| styler.Match(i, "endprocedure")
				|| styler.Match(i, "endcompose")
				|| styler.Match(i, "endswitch"))
				|| styler.Match(i, "<endinstall>")
				|| styler.Match(i, "<endinfo>")
				|| styler.Match(i, "</helpindex>")
				|| styler.Match(i, "</helpfile>")
				|| styler.Match(i, "</article>")
				|| styler.Match(i, "</keywords>")
				|| styler.Match(i, "</keyword>")
				|| styler.Match(i, "</codeblock>")
				|| styler.Match(i, "</exprblock>")
				|| styler.Match(i, "</example>")
				|| styler.Match(i, "</item>")
				|| styler.Match(i, "</list>"))
			{
				levelNext--;
				foundElse = false;
			}
			else if (styler.SafeGetCharAt(i-1) != 'd'
				&& styler.SafeGetCharAt(i-1) != 'e'
				&& (isCommand && (styler.Match(i, "if ") || styler.Match(i, "if(")
					|| styler.Match(i, "for ") || styler.Match(i, "for(")
					|| styler.Match(i, "while ") || styler.Match(i, "while(")
					|| styler.Match(i, "switch ") || styler.Match(i, "switch(")
					|| styler.Match(i, "procedure ") || styler.Match(i, "compose"))
					|| styler.Match(i, "<install>")
					|| styler.Match(i, "<info>")
					|| styler.Match(i, "<helpindex>")
					|| styler.Match(i, "<helpfile>")
					|| styler.Match(i, "<article")
					|| styler.Match(i, "<keywords>")
					|| styler.Match(i, "<keyword>")
					|| styler.Match(i, "<codeblock>")
					|| styler.Match(i, "<exprblock>")
					|| styler.Match(i, "<example")
					|| styler.Match(i, "<item")
					|| styler.Match(i, "<list")
				))
			{
				// Measure the minimum before a '{' to allow
				// folding on "} else {"
				/*if (levelMinCurrent > levelNext)
				{
					levelMinCurrent = levelNext;
				}*/
				levelNext++;
			}
			/*else if (styler.Match(i, "else"))
			{
				foundElse = true;
				int lev = levelCurrent | (levelNext-1) << 16;

				//if (visibleChars == 0 && options.foldCompact)
				//	lev |= SC_FOLDLEVELWHITEFLAG;
				//if (levelMinCurrent < levelNext-1)
				//	lev |= SC_FOLDLEVELHEADERFLAG;
				if (lev != styler.LevelAt(lineCurrent-1))
				{
					styler.SetLevel(lineCurrent-1, lev);
				}
			}*/
		}
		if (atEOL || (i == endPos-1))
		{
			if (options.foldComment && options.foldCommentMultiline)
			{  // Handle nested comments
				int nc;
				nc =  styler.GetLineState(lineCurrent);
				nc -= lineCurrent>0? styler.GetLineState(lineCurrent-1): 0;
				levelNext += nc;
			}
			int levelUse = levelCurrent;
			if (options.foldSyntaxBased && foldAtElse)
			{
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext || foundElse)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
			{
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			visibleChars = 0;
			foundElse = false;
		}
		if (!IsASpace(ch))
			visibleChars++;
	}
}



struct OptionsNPRC {
	bool fold;
	bool foldSyntaxBased;
	bool foldComment;
	bool foldCommentMultiline;
	bool foldCommentExplicit;
	std::string foldExplicitStart;
	std::string foldExplicitEnd;
	bool foldExplicitAnywhere;
	bool foldCompact;
	bool foldAtElse;
	OptionsNPRC() {
		fold = false;
		foldSyntaxBased = true;
		foldComment = false;
		foldCommentMultiline = true;
		foldCommentExplicit = true;
		foldExplicitStart = "";
		foldExplicitEnd   = "";
		foldExplicitAnywhere = false;
		foldCompact = false;
		foldAtElse = true;
	}
};

static const char * const NPRCWordLists[] = {
			"Commands",
			"Options",
			"Functions",
			"Methods",
			"Predefined variables",
			"Constants",
			"Special predefs",
			"Operator keywords",
			"Documentation keywords",
			0
		};

struct OptionSetNPRC : public OptionSet<OptionsNPRC> {
	OptionSetNPRC() {
		DefineProperty("fold", &OptionsNPRC::fold);

		DefineProperty("fold.nscr.syntax.based", &OptionsNPRC::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("fold.comment", &OptionsNPRC::foldComment);

		DefineProperty("fold.nscr.comment.multiline", &OptionsNPRC::foldCommentMultiline,
			"Set this property to 0 to disable folding multi-line comments when fold.comment=1.");

		DefineProperty("fold.nscr.comment.explicit", &OptionsNPRC::foldCommentExplicit,
			"Set this property to 0 to disable folding explicit fold points when fold.comment=1.");

		DefineProperty("fold.nscr.explicit.start", &OptionsNPRC::foldExplicitStart,
			"The string to use for explicit fold start points, replacing the standard //{.");

		DefineProperty("fold.nscr.explicit.end", &OptionsNPRC::foldExplicitEnd,
			"The string to use for explicit fold end points, replacing the standard //}.");

		DefineProperty("fold.nscr.explicit.anywhere", &OptionsNPRC::foldExplicitAnywhere,
			"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

		DefineProperty("fold.compact", &OptionsNPRC::foldCompact);

		DefineProperty("fold.at.else", &OptionsNPRC::foldAtElse);

		DefineWordListSets(NPRCWordLists);
	}
};

class LexerNPRC : public ILexer {
	bool caseSensitive;
	WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	WordList keywords5;
	WordList keywords6;
	WordList keywords7;
	WordList keywords8;
	WordList keywords9;
	/*
	"Commands",
	"Options",
	"Functions",
	"Predefined variables",
	"Constants",
	"Special predefs",
	"Operator keywords",
	0,
	*/
	OptionsNPRC options;
	OptionSetNPRC osNPRC;
public:
	LexerNPRC(bool caseSensitive_) :
		caseSensitive(caseSensitive_) {
	}
	virtual ~LexerNPRC() {
	}
	void SCI_METHOD Release() {
		delete this;
	}
	int SCI_METHOD Version() const {
		return lvOriginal;
	}
	const char * SCI_METHOD PropertyNames() {
		return osNPRC.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) {
		return osNPRC.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) {
		return osNPRC.DescribeProperty(name);
	}
	int SCI_METHOD PropertySet(const char *key, const char *val);
	const char * SCI_METHOD DescribeWordListSets() {
		return osNPRC.DescribeWordListSets();
	}
	int SCI_METHOD WordListSet(int n, const char *wl);
	void SCI_METHOD Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess);

	void * SCI_METHOD PrivateCall(int, void *) {
		return 0;
	}

	static ILexer *LexerFactoryNPRC() {
		return new LexerNPRC(true);
	}
	static ILexer *LexerFactoryNPRCInsensitive() {
		return new LexerNPRC(false);
	}
};

int SCI_METHOD LexerNPRC::PropertySet(const char *key, const char *val) {
	if (osNPRC.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

int SCI_METHOD LexerNPRC::WordListSet(int n, const char *wl) {
	WordList *wordListN = 0;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
	case 1:
		wordListN = &keywords2;
		break;
	case 2:
		wordListN = &keywords3;
		break;
	case 3:
		wordListN = &keywords4;
		break;
	case 4:
		wordListN = &keywords5;
		break;
	case 5:
		wordListN = &keywords6;
		break;
	case 6:
		wordListN = &keywords7;
		break;
	case 7:
		wordListN = &keywords8;
		break;
	case 8:
		wordListN = &keywords9;
		break;
	}
	int firstModification = -1;
	if (wordListN) {
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
		}
	}
	return firstModification;
}

void SCI_METHOD LexerNPRC::Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess)
{
	LexAccessor styler(pAccess);

	int styleBeforeDCKeyword = SCE_NPRC_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	int curLine = styler.GetLine(startPos);
	int curNcLevel = curLine > 0? styler.GetLineState(curLine-1): 0;
	bool numFloat = false; // Float literals have '+' and '-' signs
	bool numHex = false;
	bool possibleMethod = false;

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			curLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(curLine, curNcLevel);
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_NPRC_OPERATORS:
				sc.SetState(SCE_NPRC_DEFAULT);
				break;
			case SCE_NPRC_OPERATOR_KEYWORDS:
				if (sc.ch == '>')
				{
					sc.ForwardSetState(SCE_NPRC_DEFAULT);
				}
				break;
			case SCE_NPRC_NUMBERS:
				// We accept almost anything because of hex. and number suffixes
				if (isdigit(sc.ch))
				{
					continue;
				}
				else if (sc.ch == '.' && sc.chNext != '.' && !numFloat)
				{
					// Don't parse 0..2 as number.
					numFloat=true;
					continue;
				}
				else if (( sc.ch == '-' || sc.ch == '+' )
					&& (sc.chPrev == 'e' || sc.chPrev == 'E' ))
				{
					// Parse exponent sign in float literals: 2e+10 0x2e+10
					continue;
				}
				else if (( sc.ch == 'e' || sc.ch == 'E' )
					&& (sc.chNext == '+' || sc.chNext == '-' || isdigit(sc.chNext)))
				{
					// Parse exponent sign in float literals: 2e+10 0x2e+10
					continue;
				}
				else
				{
					sc.SetState(SCE_NPRC_DEFAULT);
				}
				break;
			case SCE_NPRC_STRING_PARSER:
				if (!IsWord(sc.ch) && sc.ch != '~')
				{
					sc.SetState(SCE_NPRC_DEFAULT);
				}
				break;
			case SCE_NPRC_IDENTIFIER:
				if (!IsWord(sc.ch)) /** TODO*/
				{
					// reverse identify the currently marched identifiers depending on the word list
					char s[1000];
					if (caseSensitive)
					{
						sc.GetCurrent(s, sizeof(s));
					}
					else
					{
						sc.GetCurrentLowered(s, sizeof(s));
					}
					/*
					"Commands",
					"Options",
					"Functions",
					"Predefined variables",
					"Constants",
					"Special predefs",
					"Operator keywords",
					0,
					*/
					if (keywords.InList(s))
					{
						sc.ChangeState(SCE_NPRC_COMMAND);
					}
					else if (keywords3.InList(s) && sc.ch == '(')
					{
						sc.ChangeState(SCE_NPRC_FUNCTION);
					}
					else if (keywords4.InList(s) && possibleMethod)
					{
						sc.ChangeState(SCE_NPRC_METHOD);
					}
					else if (keywords5.InList(s))
					{
						sc.ChangeState(SCE_NPRC_DEFAULT_VARS);
					}
					else if (keywords6.InList(s))
					{
						sc.ChangeState(SCE_NPRC_CONSTANTS);
					}
					else if (keywords7.InList(s))
					{
						sc.ChangeState(SCE_NPRC_PREDEFS);
					}
					else if (keywords8.InList(s))
					{
						sc.ChangeState(SCE_NPRC_OPERATOR_KEYWORDS);
					}
					else if (sc.ch == '(')
					{
						sc.ChangeState(SCE_NPRC_CUSTOM_FUNCTION);
					}
					else if (sc.ch == '{')
					{
						sc.ChangeState(SCE_NPRC_CLUSTER);
					}
					else if (keywords2.InList(s))
					{
						sc.ChangeState(SCE_NPRC_OPTION);
					}
					sc.SetState(SCE_NPRC_DEFAULT);
					possibleMethod = false;
				}
				break;
			case SCE_NPRC_COMMENT_BLOCK:
			case SCE_NPRC_DOCCOMMENT_BLOCK:
				if (sc.Match('*', '#')) {
					sc.Forward();
					sc.ForwardSetState(SCE_NPRC_DEFAULT);
				}
				break;
			case SCE_NPRC_INCLUDES:
			case SCE_NPRC_FLAGS:
			case SCE_NPRC_COMMENT_LINE:
			case SCE_NPRC_DOCCOMMENT_LINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_NSCR_DEFAULT);
				}
				break;
			case SCE_NPRC_STRING:
				if (sc.ch == '\\')
				{
					if (sc.chNext == '"' || sc.chNext == '\\')
					{
						sc.Forward();
					}
				}
				else if (sc.ch == '"')
				{
					sc.ForwardSetState(SCE_NPRC_DEFAULT);
				}
				break;
			case SCE_NPRC_FUNCTION:
			case SCE_NPRC_PROCEDURES:
				if (sc.ch == ' ' || sc.ch == '(' || sc.atLineStart)
				{
					sc.SetState(SCE_NPRC_DEFAULT);
				}
				break;
		}
		
		// Forward search for documentation keywords
		if ((sc.state == SCE_NPRC_DOCCOMMENT_LINE || sc.state == SCE_NPRC_DOCCOMMENT_BLOCK) && sc.Match('\\'))
		{
			int nCurrentState = sc.state;
			sc.SetState(SCE_NPRC_DEFAULT);
			sc.Forward();
			
			while (sc.More() && IsWord(sc.ch))
				sc.Forward();
			
			char s[1000];
			sc.GetCurrent(s, sizeof(s));

			if (keywords9.InList(s))
				sc.ChangeState(SCE_NPRC_DOCKEYWORD);
			else
				sc.ChangeState(nCurrentState);
			
			sc.SetState(nCurrentState);
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_NPRC_DEFAULT)
		{
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext)))
			{
				sc.SetState(SCE_NSCR_NUMBERS);
				numFloat = sc.ch == '.';
			}
			else if (sc.ch == '.' && IsWordStart(static_cast<char>(sc.chNext)))
				possibleMethod = true;
			else if (IsWordStart(static_cast<char>(sc.ch)))
			{
				sc.SetState(SCE_NPRC_IDENTIFIER);
			}
			else if (sc.ch == '$')
			{
				sc.SetState(SCE_NPRC_PROCEDURES);
			}
			else if (sc.Match("#*!"))
			{
				sc.SetState(SCE_NPRC_DOCCOMMENT_BLOCK);
				sc.Forward();   // Eat the * so it isn't used for the end of the comment
			}
			else if (sc.Match("##!"))
			{
				sc.SetState(SCE_NPRC_DOCCOMMENT_LINE);
				sc.Forward();
			}
			else if (sc.Match('#', '*'))
			{
				sc.SetState(SCE_NPRC_COMMENT_BLOCK);
				sc.Forward();   // Eat the * so it isn't used for the end of the comment
			}
			else if (sc.Match('#', '#'))
			{
				sc.SetState(SCE_NPRC_COMMENT_LINE);
				sc.Forward();
			}
			else if (sc.ch == '"')
			{
				sc.SetState(SCE_NPRC_STRING);
			}
			else if (sc.ch == '#')
			{
				sc.SetState(SCE_NPRC_STRING_PARSER);
			}
			else if (sc.ch == '@')
			{
				sc.SetState(SCE_NPRC_INCLUDES);
			}
			else if (IsOperator(static_cast<char>(sc.ch)))
			{
				if (sc.ch == '<'
					&& (sc.Match("<wp>") || sc.Match("<this>") || sc.Match("<loadpath>") || sc.Match("<savepath>") || sc.Match("<plotpath>") || sc.Match("<procpath>") || sc.Match("<scriptpath>")))
				{
					sc.SetState(SCE_NPRC_OPERATOR_KEYWORDS);
				}
				else if (sc.ch == ':' && sc.chNext == ':')
				{
					sc.SetState(SCE_NPRC_FLAGS);
				}
				else
				{
					sc.SetState(SCE_NPRC_OPERATORS);
				}
			}
		}
	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".

void SCI_METHOD LexerNPRC::Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess)
{

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	bool foldAtElse = options.foldAtElse;
	bool foundElse = false;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (unsigned int i = startPos; i < endPos; i++)
	{
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (options.foldComment && options.foldCommentMultiline)
		{
			if (!(stylePrev == SCE_NPRC_COMMENT_BLOCK))
			{
				levelNext++;
			}
			else if (!(styleNext == SCE_NPRC_COMMENT_BLOCK) && !atEOL)
			{
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldComment && options.foldCommentExplicit && ((style == SCE_NPRC_COMMENT_LINE) || options.foldExplicitAnywhere))
		{
			if (userDefinedFoldMarkers)
			{
				if (styler.Match(i, options.foldExplicitStart.c_str()))
				{
 					levelNext++;
				}
				else if (styler.Match(i, options.foldExplicitEnd.c_str()))
				{
 					levelNext--;
 				}
			}
			else
			{
				if ((ch == '#') && (chNext == '#'))  //##{
				{
					char chNext2 = styler.SafeGetCharAt(i + 2);
					if (chNext2 == '{')
					{
						levelNext++;
					}
					else if (chNext2 == '}')
					{
						levelNext--;
					}
				}
 			}
 		}
		if (options.foldSyntaxBased && (style == SCE_NPRC_IDENTIFIER || style == SCE_NPRC_COMMAND))
		{
			if (styler.Match(i, "endif")
				|| styler.Match(i, "endfor")
				|| styler.Match(i, "endwhile")
				|| styler.Match(i, "endprocedure")
				|| styler.Match(i, "endswitch")
				|| styler.Match(i, "endcompose"))
			{
				levelNext--;
				foundElse = false;
			}
			else if (styler.SafeGetCharAt(i-1) != 'd'
				&& styler.SafeGetCharAt(i-1) != 'e'
				&& (styler.Match(i, "if ") || styler.Match(i, "if(")
					|| styler.Match(i, "for ") || styler.Match(i, "for(")
					|| styler.Match(i, "while ") || styler.Match(i, "while(")
					|| styler.Match(i, "switch ") || styler.Match(i, "switch(")
					|| styler.Match(i, "procedure ") || styler.Match(i, "compose")))
			{
				// Measure the minimum before a '{' to allow
				// folding on "} else {"
				/*if (levelMinCurrent > levelNext)
				{
					levelMinCurrent = levelNext;
				}*/
				levelNext++;
			}
			/*else if (styler.Match(i, "else"))
			{
				foundElse = true;
				int lev = levelCurrent | (levelNext-1) << 16;

				//if (visibleChars == 0 && options.foldCompact)
				//	lev |= SC_FOLDLEVELWHITEFLAG;
				//if (levelMinCurrent < levelNext-1)
				//	lev |= SC_FOLDLEVELHEADERFLAG;
				if (lev != styler.LevelAt(lineCurrent-1))
				{
					styler.SetLevel(lineCurrent-1, lev);
				}
			}*/
		}
		if (atEOL || (i == endPos-1))
		{
			if (options.foldComment && options.foldCommentMultiline)
			{  // Handle nested comments
				int nc;
				nc =  styler.GetLineState(lineCurrent);
				nc -= lineCurrent>0? styler.GetLineState(lineCurrent-1): 0;
				levelNext += nc;
			}
			int levelUse = levelCurrent;
			if (options.foldSyntaxBased && foldAtElse)
			{
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext || foundElse)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
			{
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			visibleChars = 0;
			foundElse = false;
		}
		if (!IsASpace(ch))
			visibleChars++;
	}
}




class LexerTXTADV : public ILexer {
	bool caseSensitive;
	/*WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	WordList keywords5;
	WordList keywords6;
	WordList keywords7;
	WordList keywords8;*/
	/*
	"Commands",
	"Options",
	"Functions",
	"Predefined variables",
	"Constants",
	"Special predefs",
	"Operator keywords",
	0,
	*/
	//OptionsNSCR options;
	//OptionSetNSCR osNSCR;
public:
	LexerTXTADV(bool caseSensitive_) :
		caseSensitive(caseSensitive_) {
	}
	virtual ~LexerTXTADV() {
	}
	void SCI_METHOD Release() {
		delete this;
	}
	int SCI_METHOD Version() const {
		return lvOriginal;
	}
	const char * SCI_METHOD PropertyNames() {
		return "";
	}
	int SCI_METHOD PropertyType(const char *name) {
		return 0;
	}
	const char * SCI_METHOD DescribeProperty(const char *name) {
		return "";
	}
	int SCI_METHOD PropertySet(const char *key, const char *val) {return 0;}
	const char * SCI_METHOD DescribeWordListSets() {
		return "";
	}
	int SCI_METHOD WordListSet(int n, const char *wl) {return 0;}
	void SCI_METHOD Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {return;};

	void * SCI_METHOD PrivateCall(int, void *) {
		return 0;
	}

	static ILexer *LexerFactoryTXTADV() {
		return new LexerTXTADV(true);
	}
	static ILexer *LexerFactoryTXTADVInsensitive() {
		return new LexerTXTADV(false);
	}
};

/*int SCI_METHOD LexerNSCR::PropertySet(const char *key, const char *val) {
	if (osNSCR.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

int SCI_METHOD LexerNSCR::WordListSet(int n, const char *wl) {
	WordList *wordListN = 0;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
	case 1:
		wordListN = &keywords2;
		break;
	case 2:
		wordListN = &keywords3;
		break;
	case 3:
		wordListN = &keywords4;
		break;
	case 4:
		wordListN = &keywords5;
		break;
	case 5:
		wordListN = &keywords6;
		break;
	case 6:
		wordListN = &keywords7;
		break;
	case 7:
		wordListN = &keywords8;
		break;
	}
	int firstModification = -1;
	if (wordListN) {
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
		}
	}
	return firstModification;
}*/

// helper defines
#define SCE_TXTADV_POSSIB_URL 20
void SCI_METHOD LexerTXTADV::Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess)
{
	LexAccessor styler(pAccess);

	int styleBeforeDCKeyword = SCE_TXTADV_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	int curLine = styler.GetLine(startPos);
	int curNcLevel = curLine > 0? styler.GetLineState(curLine-1): 0;

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			curLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(curLine, curNcLevel);
		}

		// Determine if the current state should terminate.
		switch (sc.state)
		{
			case SCE_TXTADV_MODIFIER:
				sc.SetState(SCE_TXTADV_DEFAULT);
				break;
			case SCE_TXTADV_BOLD:
			case SCE_TXTADV_ITALIC:
			case SCE_TXTADV_BOLD_ITALIC:
				if (sc.ch == '*')
				{
					if (sc.state == SCE_TXTADV_BOLD_ITALIC)
					{
						if (sc.Match("***"))
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_DEFAULT);
						}
						else if (sc.Match("**"))
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_ITALIC);
						}
						else
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_BOLD);
						}
					}
					else if (sc.state == SCE_TXTADV_BOLD)
					{
						if (sc.Match("***"))
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_ITALIC);
						}
						else if (sc.Match("**"))
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_DEFAULT);
						}
						else
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_BOLD_ITALIC);
						}
					}
					else
					{
						if (sc.Match("***"))
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_BOLD);
						}
						else if (sc.Match("**"))
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_BOLD_ITALIC);
						}
						else
						{
							sc.SetState(SCE_TXTADV_MODIFIER);
							sc.ForwardSetState(SCE_TXTADV_DEFAULT);
						}
					}
				}
				break;
			case SCE_TXTADV_HEAD:
			case SCE_TXTADV_BIGHEAD:
				if (sc.atLineStart)
					sc.SetState(SCE_TXTADV_DEFAULT);
				break;
			case SCE_TXTADV_POSSIB_URL:
				if (sc.ch == ' ' || sc.ch == '\t' || sc.ch == '*' || sc.ch == '_' || sc.ch == '-' || sc.atLineStart)
					sc.ChangeState(SCE_TXTADV_DEFAULT);
				else if (sc.Match("://"))
					sc.ChangeState(SCE_TXTADV_URL);
				break;
			case SCE_TXTADV_URL:
				if (sc.ch == ' ' || sc.atLineStart)
					sc.SetState(SCE_TXTADV_DEFAULT);
				break;
			case SCE_TXTADV_STRIKETHROUGH:
				if (sc.ch == '-' && (sc.chPrev == ' ' || sc.chNext == ' '))
				{
					sc.SetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_DEFAULT);
				}
				break;
			case SCE_TXTADV_UNDERLINE:
				if (sc.ch == '_' && (sc.chPrev == ' ' || sc.chNext == ' '))
				{
					sc.SetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_DEFAULT);
				}
				break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_TXTADV_DEFAULT)
		{
			if (sc.ch == '*')
			{
				if (sc.Match("***"))
				{
					sc.SetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_BOLD_ITALIC);
				}
				else if (sc.Match("**"))
				{
					sc.SetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_BOLD);
				}
				else
				{
					sc.SetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_ITALIC);
				}
			}
			else if (sc.ch == '_' && sc.chPrev == ' ')
			{
				sc.SetState(SCE_TXTADV_MODIFIER);
				sc.ForwardSetState(SCE_TXTADV_UNDERLINE);
			}
			else if (sc.ch == '#')
			{
				if (sc.Match("##"))
				{
					sc.SetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_HEAD);
				}
				else
				{
					sc.SetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_BIGHEAD);
				}
			}
			else if (sc.ch == '-')
			{
				if (sc.Match("--"))
				{
					sc.Forward();
				}
				else if (sc.chPrev == ' ')
				{
					sc.SetState(SCE_TXTADV_MODIFIER);
					sc.ForwardSetState(SCE_TXTADV_STRIKETHROUGH);
				}
			}
			else if (isalpha(sc.ch))
				sc.SetState(SCE_TXTADV_POSSIB_URL);
		}
	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
/*
void SCI_METHOD LexerNSCR::Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess)
{

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	bool foldAtElse = options.foldAtElse;
	bool foundElse = false;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (unsigned int i = startPos; i < endPos; i++)
	{
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (options.foldComment && options.foldCommentMultiline)
		{
			if (!(stylePrev == SCE_NSCR_COMMENT_BLOCK))
			{
				levelNext++;
			}
			else if (!(styleNext == SCE_NSCR_COMMENT_BLOCK) && !atEOL)
			{
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldComment && options.foldCommentExplicit && ((style == SCE_NSCR_COMMENT_LINE) || options.foldExplicitAnywhere))
		{
			if (userDefinedFoldMarkers)
			{
				if (styler.Match(i, options.foldExplicitStart.c_str()))
				{
 					levelNext++;
				}
				else if (styler.Match(i, options.foldExplicitEnd.c_str()))
				{
 					levelNext--;
 				}
			}
			else
			{
				if ((ch == '#') && (chNext == '#'))  //##{
				{
					char chNext2 = styler.SafeGetCharAt(i + 2);
					if (chNext2 == '{')
					{
						levelNext++;
					}
					else if (chNext2 == '}')
					{
						levelNext--;
					}
				}
 			}
 		}
		if (options.foldSyntaxBased && (style == SCE_NSCR_IDENTIFIER || style == SCE_NSCR_COMMAND || style == SCE_NSCR_INSTALL || style == SCE_NSCR_PROCEDURE_COMMANDS))
		{
			if (styler.Match(i, "endif")
				|| styler.Match(i, "endfor")
				|| styler.Match(i, "endwhile")
				|| styler.Match(i, "endprocedure")
				|| styler.Match(i, "endcompose")
				|| styler.Match(i, "<endinstall>")
				|| styler.Match(i, "<endinfo>")
				|| styler.Match(i, "</helpindex>")
				|| styler.Match(i, "</helpfile>")
				|| styler.Match(i, "</article>")
				|| styler.Match(i, "</keywords>")
				|| styler.Match(i, "</keyword>")
				|| styler.Match(i, "</codeblock>")
				|| styler.Match(i, "</exprblock>")
				|| styler.Match(i, "</example>")
				|| styler.Match(i, "</item>")
				|| styler.Match(i, "</list>"))
			{
				levelNext--;
				foundElse = false;
			}
			else if (styler.SafeGetCharAt(i-1) != 'd'
				&& styler.SafeGetCharAt(i-1) != 'e'
				&& (styler.Match(i, "if ") || styler.Match(i, "if(")
					|| styler.Match(i, "for ") || styler.Match(i, "for(")
					|| styler.Match(i, "while ") || styler.Match(i, "while(")
					|| styler.Match(i, "procedure ") || styler.Match(i, "compose")
					|| styler.Match(i, "<install>")
					|| styler.Match(i, "<info>")
					|| styler.Match(i, "<helpindex>")
					|| styler.Match(i, "<helpfile>")
					|| styler.Match(i, "<article")
					|| styler.Match(i, "<keywords>")
					|| styler.Match(i, "<keyword>")
					|| styler.Match(i, "<codeblock>")
					|| styler.Match(i, "<exprblock>")
					|| styler.Match(i, "<example")
					|| styler.Match(i, "<item")
					|| styler.Match(i, "<list")
				))
			{
				// Measure the minimum before a '{' to allow
				// folding on "} else {"

				levelNext++;
			}

		}
		if (atEOL || (i == endPos-1))
		{
			if (options.foldComment && options.foldCommentMultiline)
			{  // Handle nested comments
				int nc;
				nc =  styler.GetLineState(lineCurrent);
				nc -= lineCurrent>0? styler.GetLineState(lineCurrent-1): 0;
				levelNext += nc;
			}
			int levelUse = levelCurrent;
			if (options.foldSyntaxBased && foldAtElse)
			{
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext || foundElse)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
			{
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			visibleChars = 0;
			foundElse = false;
		}
		if (!IsASpace(ch))
			visibleChars++;
	}
}
*/




LexerModule lmNSCR(SCLEX_NSCR, LexerNSCR::LexerFactoryNSCR, "NSCR", NSCRWordLists);
LexerModule lmNPRC(SCLEX_NPRC, LexerNPRC::LexerFactoryNPRC, "NPRC", NPRCWordLists);
LexerModule lmTXTADV(SCLEX_TXTADV, LexerTXTADV::LexerFactoryTXTADV, "TXTADV", emptyWordListDesc);

LexerModule lmBatch(SCLEX_BATCH, ColouriseBatchDoc, "batch", 0, batchWordListDesc);
LexerModule lmDiff(SCLEX_DIFF, ColouriseDiffDoc, "diff", FoldDiffDoc, emptyWordListDesc);
LexerModule lmPo(SCLEX_PO, ColourisePoDoc, "po", 0, emptyWordListDesc);
LexerModule lmProps(SCLEX_PROPERTIES, ColourisePropsDoc, "props", FoldPropsDoc, emptyWordListDesc);
LexerModule lmMake(SCLEX_MAKEFILE, ColouriseMakeDoc, "makefile", 0, emptyWordListDesc);
LexerModule lmErrorList(SCLEX_ERRORLIST, ColouriseErrorListDoc, "errorlist", 0, emptyWordListDesc);
LexerModule lmLatex(SCLEX_LATEX, ColouriseLatexDoc, "latex", 0, emptyWordListDesc);
LexerModule lmNull(SCLEX_NULL, ColouriseNullDoc, "null");
