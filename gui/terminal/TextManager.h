/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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


#ifndef TEXTMANAGER__H
#define TEXTMANAGER__H

#include <vector>
#include <deque>
#include <string>

using namespace std;

class GenericTerminal;

// Cursor, which is used in the TextManager to identify the
// actual line and position in the m_text variable. The TextManager
// is able to convert this cursor into a ViewCursor
struct LogicalCursor
{
	size_t pos;
	size_t line;
	bool isValid;

	LogicalCursor() : pos(0), line(0), isValid(0) {}
	LogicalCursor(size_t nPos, size_t nLine) : pos(nPos), line(nLine), isValid(true) {}
	void move(size_t _pos, size_t _line)
	{
		pos = _pos;
		line = _line;
	}
	void advance()
	{
		pos++;
	}
	bool revert()
	{
		if (!pos)
			return false;
		pos--;
		return true;
	}
	bool operator!() const
	{
		return !isValid;
	}
	operator bool() const
	{
		return isValid;
	}
	LogicalCursor& operator++(int)
	{
		this->advance();
		return *this;
	}
	LogicalCursor operator+(int n)
	{
	    LogicalCursor cursor(*this);
	    cursor.pos += n;
	    return cursor;
	}
	LogicalCursor operator-(int n)
	{
	    LogicalCursor cursor(*this);
	    cursor.pos -= n;
	    return cursor;
	}
	bool operator--(int)
	{
		return this->revert();
	}
	bool operator==(const LogicalCursor& cursor)
	{
		return cursor.isValid == isValid && cursor.pos == pos && cursor.line == line;
	}
	bool operator<(const LogicalCursor& cursor)
	{
		return (isValid == cursor.isValid) && (line < cursor.line || (line == cursor.line && pos < cursor.pos));
	}
	bool operator<=(const LogicalCursor& cursor)
	{
		return operator<(cursor) || operator==(cursor);
	}
	bool operator>(const LogicalCursor& cursor)
	{
		return !(operator<=(cursor));
	}
	bool operator>=(const LogicalCursor& cursor)
	{
		return !(operator<(cursor));
	}
	bool operator!=(const LogicalCursor& cursor)
	{
		return !(operator==(cursor));
	}
};

// Cursor, which is used in the terminal. The TextManager
// is able to convert this cursor into a LogicalCursor
struct ViewCursor
{
	size_t x;
	size_t y;
	bool isValid;

	ViewCursor() : x(0), y(0), isValid(false) {}
	ViewCursor(size_t _x, size_t _y) : x(_x), y(_y), isValid(true) {}
	void move(size_t _x, size_t _y)
	{
		x = _x;
		y = _y;
	}
	void advance()
	{
		x++;
	}
	bool revert()
	{
		if (!x)
			return false;
		x--;
		return true;
	}
	bool operator!() const
	{
		return !isValid;
	}
	operator bool() const
	{
		return isValid;
	}
	ViewCursor& operator++(int)
	{
		this->advance();
		return *this;
	}
	bool operator--(int)
	{
		return this->revert();
	}
	bool operator==(const ViewCursor& cursor)
	{
		return cursor.isValid == isValid && cursor.x == x && cursor.y == y;
	}
	bool operator<(const ViewCursor& cursor)
	{
		return (isValid == cursor.isValid) && (y < cursor.y || (y == cursor.y && x < cursor.x));
	}
	bool operator<=(const ViewCursor& cursor)
	{
		return operator<(cursor) || operator==(cursor);
	}
	bool operator>(const ViewCursor& cursor)
	{
		return !(operator<=(cursor));
	}
	bool operator>=(const ViewCursor& cursor)
	{
		return !(operator<(cursor));
	}
	bool operator!=(const ViewCursor& cursor)
	{
		return !(operator==(cursor));
	}
};

// This structure combines the rendered line with its
// colors and its coordinates
struct RenderedLine
{
	string sLine;
	vector<unsigned short> colors;
	vector<LogicalCursor> coords;
};

// This structure combines the character information
// needed in the terminal: the character itself, the
// associated colour and the editable flag
struct Character
{
    char m_char;
    // 0x1 => User
    // 0x2 => Editable
    // 0x80 => Selected
    unsigned char m_info;
    int m_color;

    enum
    {
        USERTEXT = 0x1,
        EDITABLE = 0x2,
        SELECTED = 0x80
    };

    Character() : m_char(0), m_info(0), m_color(0) {}
    Character(char c) : m_char(c), m_info(0), m_color(0) {}
    Character(const Character& c) : m_char(c.m_char), m_info(c.m_info), m_color(c.m_color) {}
    Character(char c, unsigned char flags) : m_char(c), m_info(flags), m_color(0) {}

    Character& operator=(const Character& c)
    {
        m_char = c.m_char;
        m_info = c.m_info;
        m_color = c.m_color;
        return *this;
    }

    void setFlags(short flags)
    {
        m_info |= ((unsigned char)flags);
    }

    bool userText()
    {
        return m_info & USERTEXT;
    }

    bool editable()
    {
        return m_info & EDITABLE;
    }

    void makeUserText()
    {
        m_info |= USERTEXT;
        m_info &= ~EDITABLE;
    }

    void makeEditable()
    {
        m_info |= EDITABLE;
    }

    void select()
    {
        m_info |= SELECTED;
    }

    void unselect()
    {
        m_info &= ~SELECTED;
    }

    bool isSelected()
    {
        return m_info & SELECTED;
    }

    int getColor()
    {
        return m_color;
    }

    void setColor(int color)
    {
        m_color = color;
    }
};

// This class resembles an extended string class,
// which contains the extended character class.
class CharacterVector : public vector<Character>
{
    private:
        Character m_dummy;

        void assign(const string& sText)
        {
            clear();
            append(sText);
        }

        void append(const string& sText)
        {
            for (size_t i = 0; i < sText.length(); i++)
                emplace_back(sText[i]);
        }

    public:
        CharacterVector() : vector<Character>() {}

        CharacterVector(const string& sText) : vector<Character>()
        {
            append(sText);
        }

        CharacterVector(const string& sText, short flags) : vector<Character>()
        {
            append(sText);

            if (flags)
            {
                for (size_t i = 0; i < size(); i++)
                    operator[](i).setFlags(flags);
            }
        }

        CharacterVector& operator+=(const string& sText)
        {
            append(sText);
            return *this;
        }

        CharacterVector& operator+=(const CharacterVector& vect)
        {
            vector<Character>::insert(end(), vect.begin(), vect.end());
            return *this;
        }

        CharacterVector& operator=(const string& sText)
        {
            assign(sText);
            return *this;
        }

        Character& operator[](size_t i)
        {
            if (!size())
                return m_dummy;

            return vector<Character>::operator[](i);
        }

        Character& front()
        {
            if (!size())
                return m_dummy;

            return vector<Character>::front();
        }

        Character& back()
        {
            if (!size())
                return m_dummy;

            return vector<Character>::back();
        }

        string toString()
        {
            string sRet;

            for (size_t i = 0; i < size(); i++)
                sRet.append(1u, operator[](i).m_char);

            return sRet;
        }

        string substr(size_t pos, size_t len = string::npos)
        {
            return toString().substr(pos, len);
        }

        vector<unsigned short> subcolors(size_t pos, size_t len = string::npos)
        {
            if (pos >= size())
                return vector<unsigned short>();

            if (pos+len >= size())
                len = size() - pos;

            vector<unsigned short> vect;

            for (size_t i = 0; i < len; i++)
                vect.push_back(operator[](pos+i).getColor());

            return vect;

        }

        size_t length()
        {
            return size();
        }

        iterator insert(size_t pos, const string& sText, short flags = 0)
        {
            return insert(pos, CharacterVector(sText, flags));
        }

        iterator insert(size_t pos, const CharacterVector& vect)
        {
            if (pos >= size())
                return vector<Character>::insert(end(), vect.begin(), vect.end());

            return vector<Character>::insert(begin() + pos, vect.begin(), vect.end());
        }

        void append(size_t n, char c)
        {
            append(string(n, c));
        }
};

// This class manages the actual text by storing it in an internal buffer.
// During an update the TextManager will render the text in the internal
// buffer into the text block in screen coordinates
class TextManager
{
	public:
		TextManager(GenericTerminal* parent = nullptr, int width = 80, int height = 24, int maxWidth = 160, int maxHeight = 300);
		~TextManager();

		void printOutput(const string& sLine);
		void insertInput(const string& sLine, size_t logicalpos = string::npos);

		ViewCursor toViewCursor(const LogicalCursor& logCursor);
		ViewCursor getCurrentViewPos();
		LogicalCursor toLogicalCursor(const ViewCursor& viewCursor);
		LogicalCursor getCurrentLogicalPos();

		string getRenderedString(size_t viewLine);
		vector<unsigned short> getRenderedColors(size_t viewLine);

		size_t tab();
		void newLine();
		void backspace(const LogicalCursor& logCursor);
		void eraseLine();
		bool clearRange(const ViewCursor& cursor1, const ViewCursor& cursor2);

		void selectText(const ViewCursor& viewCursor, bool bSelect = true);
		void unselectAll();
		bool isSelected(const ViewCursor& viewCursor);
		string getSelectedText();
		string getCurrentInputLine();

		int GetSize();
		int GetMaxSize();
		int GetHeight();
		int GetNumLinesScrolled();
		int GetLinesReceived();
		string GetInputHistory(bool vcursorup = true);
		string GetTextRange(int y, int x0, int x1);
		string GetWordAt(int y, int x);
		string GetWordStartAt(int y, int x);
		char GetCharAdjusted(int y, int x);
		char GetCharLogical(const LogicalCursor& cursor);
		bool IsUserText(int y, int x);
		bool IsEditable(int y, int x);
		bool IsEditableLogical(LogicalCursor& logCursor);
		unsigned short GetColor(int y, int x);
		unsigned short GetColorAdjusted(int y, int x);

		void ChangeEditableState();

		void SetMaxSize(int newSize);
		void ResetVirtualCursorLine()
		{
			m_virtualCursor = m_managedText.size() - 1;
		}
		void SetColorAdjusted(int y, int x, unsigned short value);

		bool Scroll(int numLines, bool scrollUp);
		void Resize(int width, int height);
		void Reset();

		string operator[](int index);

		int AdjustIndex(int index);

	private:
		GenericTerminal* m_parent;

		int m_topLine;
		int m_bottomLine;
		int m_numLinesScrolledUp;
		int m_viewportWidth;
		int m_viewportHeight;
		int m_linesReceived;
		int m_maxWidth;
		int m_maxHeight;
		int m_virtualCursor;

		size_t m_tabLength;
		size_t m_indentDepth;

		deque<CharacterVector> m_managedText;
		vector<RenderedLine> m_renderedBlock;

		int calc_color(int fg, int bg, int flags);
		void updateColors(bool isErrorLine = false);
		void renderLayout();
		size_t findNextLinebreak(const string& currentLine, size_t currentLinebreak);

		enum
		{
		    KERNEL_TEXT,
		    USER_TEXT,
		    EDITABLE_TEXT
		};

};


#endif
