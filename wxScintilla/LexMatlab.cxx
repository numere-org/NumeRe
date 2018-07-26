// Scintilla source code edit control
/** @file LexMatlab.cxx
 ** Lexer for Matlab.
 ** Written by José Fonseca
 **
 ** Changes by Christoph Dalitz 2003/12/04:
 **   - added support for Octave
 **   - Strings can now be included both in single or double quotes
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static bool IsMatlabCommentChar(int c) {
	return (c == '%') ;
}

static bool IsOctaveCommentChar(int c) {
	return (c == '%' || c == '#') ;
}

static bool IsMatlabComment(Accessor &styler, int pos, int len) {
	return len > 0 && IsMatlabCommentChar(styler[pos]) ;
}

static bool IsOctaveComment(Accessor &styler, int pos, int len) {
	return len > 0 && IsOctaveCommentChar(styler[pos]) ;
}

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static void ColouriseMatlabOctaveDoc(
            unsigned int startPos, int length, int initStyle,
            WordList *keywordlists[], Accessor &styler,
            bool (*IsCommentChar)(int)) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];

	styler.StartAt(startPos);

	bool transpose = false;

	StyleContext sc(startPos, length, initStyle, styler);
	int nParens = 0;

	for (; sc.More(); sc.Forward()) {

		if (sc.state == SCE_MATLAB_OPERATOR) {
			if (sc.chPrev == '.') {
				if (sc.ch == '*' || sc.ch == '/' || sc.ch == '\\' || sc.ch == '^') {
					sc.ForwardSetState(SCE_MATLAB_DEFAULT);
					transpose = false;
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_MATLAB_DEFAULT);
					transpose = true;
				} else {
					sc.SetState(SCE_MATLAB_DEFAULT);
				}
			} else {
				sc.SetState(SCE_MATLAB_DEFAULT);
			}
		} else if (sc.state == SCE_MATLAB_KEYWORD) {
			if (!isalnum(sc.ch) && sc.ch != '_') {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if (!strcmp(s, "end") && nParens)
				{
					sc.ChangeState(SCE_MATLAB_IDENTIFIER);
					sc.SetState(SCE_MATLAB_DEFAULT);
					transpose = true;
				}
				else if (keywords.InList(s))  {
					sc.SetState(SCE_MATLAB_DEFAULT);
					transpose = false;
				} else if (keywords2.InList(s))  {
					sc.ChangeState(SCE_MATLAB_FUNCTIONS);
					sc.SetState(SCE_MATLAB_DEFAULT);
					transpose = false;
				} else {
					sc.ChangeState(SCE_MATLAB_IDENTIFIER);
					sc.SetState(SCE_MATLAB_DEFAULT);
					transpose = true;
				}
			}
		} else if (sc.state == SCE_MATLAB_NUMBER) {
			if (!isdigit(sc.ch) && sc.ch != '.'
			        && !(sc.ch == 'e' || sc.ch == 'E')
			        && !((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E'))) {
				sc.SetState(SCE_MATLAB_DEFAULT);
				transpose = true;
			}
		} else if (sc.state == SCE_MATLAB_STRING) {
			if (sc.ch == '\'') {
				if (sc.chNext == '\'') {
 					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_MATLAB_DEFAULT);
 				}
			}
		} else if (sc.state == SCE_MATLAB_DOUBLEQUOTESTRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_MATLAB_DEFAULT);
			}
		} else if (sc.state == SCE_MATLAB_COMMENT || sc.state == SCE_MATLAB_COMMAND) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_MATLAB_DEFAULT);
				transpose = false;
			}
		}

		if (sc.state == SCE_MATLAB_DEFAULT) {
			if (IsCommentChar(sc.ch)) {
				sc.SetState(SCE_MATLAB_COMMENT);
			} else if (sc.ch == '!' && sc.chNext != '=' ) {
				sc.SetState(SCE_MATLAB_COMMAND);
			} else if (sc.ch == '\'') {
				if (transpose) {
					sc.SetState(SCE_MATLAB_OPERATOR);
				} else {
					sc.SetState(SCE_MATLAB_STRING);
				}
			} else if (sc.ch == '"') {
				sc.SetState(SCE_MATLAB_DOUBLEQUOTESTRING);
			} else if (isdigit(sc.ch) || (sc.ch == '.' && isdigit(sc.chNext))) {
				sc.SetState(SCE_MATLAB_NUMBER);
			} else if (isalpha(sc.ch)) {
				sc.SetState(SCE_MATLAB_KEYWORD);
			} else if (isoperator(static_cast<char>(sc.ch)) || sc.ch == '@' || sc.ch == '\\') {
			if (sc.ch == ')' || sc.ch == ']' || sc.ch == '}') {
					transpose = true;
				} else {
					transpose = false;
				}
				if (sc.ch == '(' || sc.ch == '[' || sc.ch == '{')
					nParens++;				
				if (sc.ch == ')' || sc.ch == ']' || sc.ch == '}')
					nParens--;
				sc.SetState(SCE_MATLAB_OPERATOR);
			} else {
				transpose = false;
			}
		}
	}
	sc.Complete();
}

static void ColouriseMatlabDoc(unsigned int startPos, int length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {
	ColouriseMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsMatlabCommentChar);
}

static void ColouriseOctaveDoc(unsigned int startPos, int length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {
	ColouriseMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsOctaveCommentChar);
}

static void FoldMatlabOctaveDoc(unsigned int startPos, int length, int initStyle,
                                WordList *[], Accessor &styler,
                                bool (*IsComment)(Accessor&, int, int)) {

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
	for (unsigned int i = startPos; i < endPos; i++) 
	{
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (style == SCE_MATLAB_OPERATOR &&
			(styler.SafeGetCharAt(i) == '('
				|| styler.SafeGetCharAt(i) == '{'
				|| styler.SafeGetCharAt(i) == '[')
			)
		{
			int nPar = 0;
			for (int j = i; j < endPos; j++)
			{
				atEOL = (styler.SafeGetCharAt(j) == '\r' && styler.SafeGetCharAt(j+1) != '\n') 
					|| (styler.SafeGetCharAt(j) == '\n');
				if (styler.StyleAt(j) == SCE_MATLAB_OPERATOR &&
					(styler.SafeGetCharAt(j) == '('
					|| styler.SafeGetCharAt(j) == '{'
					|| styler.SafeGetCharAt(j) == '['))
					nPar++;
				if (styler.StyleAt(j) == SCE_MATLAB_OPERATOR &&
					(styler.SafeGetCharAt(j) == ')'
					|| styler.SafeGetCharAt(j) == '}'
					|| styler.SafeGetCharAt(j) == ']'))
					nPar--;
				if (atEOL) 
				{
					int levelUse = levelCurrent;
					levelUse = levelMinCurrent;
					int lev = levelUse | levelNext << 16;
					/*if (visibleChars == 0)
						lev |= SC_FOLDLEVELWHITEFLAG;*/
					if (levelUse < levelNext)
						lev |= SC_FOLDLEVELHEADERFLAG;
					if (lev != styler.LevelAt(lineCurrent)) 
					{
						styler.SetLevel(lineCurrent, lev);
					}
					lineCurrent++;
					levelCurrent = levelNext;
					levelMinCurrent = levelCurrent;
					visibleChars = 0;
				}
				if (!nPar)
				{
					i = j-1;
					break;
				}
			}
			continue;
		}
		if (style == SCE_MATLAB_KEYWORD) 
		{
			if (styler.Match(i, "end"))			
			{
				levelNext--;
			}
			else if (styler.SafeGetCharAt(i-1) != 'e'
				&& (styler.Match(i, "if")
					|| styler.Match(i, "for")
					|| styler.Match(i, "while")
					|| styler.Match(i, "do")
					|| styler.Match(i, "switch")
					|| styler.Match(i, "try")
					|| styler.Match(i, "classdef")
					|| styler.Match(i, "methods")
					|| styler.Match(i, "properties")
					|| styler.Match(i, "function"))
				) 
			{
				levelNext++;
			}
		}
		if (atEOL || (i == endPos-1)) 
		{
			int levelUse = levelCurrent;
			levelUse = levelMinCurrent;
			int lev = levelUse | levelNext << 16;
			/*if (visibleChars == 0)
				lev |= SC_FOLDLEVELWHITEFLAG;*/
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) 
			{
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			visibleChars = 0;
		}
		if (!IsASpace(ch))
			visibleChars++;
	}

								
	/*int endPos = startPos + length;

	// Backtrack to previous line in case need to fix its fold status
	int lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}
	int spaceFlags = 0;
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, IsComment);
	char chNext = styler[startPos];
	for (int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == endPos)) {
			int lev = indentCurrent;
			int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags, IsComment);
			if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
				// Only non whitespace lines can be headers
				if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK)) {
					lev |= SC_FOLDLEVELHEADERFLAG;
				} else if (indentNext & SC_FOLDLEVELWHITEFLAG) {
					// Line after is blank so check the next - maybe should continue further?
					int spaceFlags2 = 0;
					int indentNext2 = styler.IndentAmount(lineCurrent + 2, &spaceFlags2, IsComment);
					if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext2 & SC_FOLDLEVELNUMBERMASK)) {
						lev |= SC_FOLDLEVELHEADERFLAG;
					}
				}
			}
			indentCurrent = indentNext;
			styler.SetLevel(lineCurrent, lev);
			lineCurrent++;
		}
	}*/
}

static void FoldMatlabDoc(unsigned int startPos, int length, int initStyle,
                          WordList *keywordlists[], Accessor &styler) {
	FoldMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsMatlabComment);
}

static void FoldOctaveDoc(unsigned int startPos, int length, int initStyle,
                          WordList *keywordlists[], Accessor &styler) {
	FoldMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsOctaveComment);
}

static const char * const matlabWordListDesc[] = {
	"Keywords",
	0
};

static const char * const octaveWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmMatlab(SCLEX_MATLAB, ColouriseMatlabDoc, "matlab", FoldMatlabDoc, matlabWordListDesc);

LexerModule lmOctave(SCLEX_OCTAVE, ColouriseOctaveDoc, "octave", FoldOctaveDoc, octaveWordListDesc);
