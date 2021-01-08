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


/////////////////////////////////////////////////
/// \brief Cursor, which is used in the
/// TextManager to identify the actual line and
/// position in the m_text variable. The
/// TextManager is able to convert this cursor
/// into a ViewCursor.
/////////////////////////////////////////////////
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
	bool operator==(const LogicalCursor& cursor) const
	{
		return cursor.isValid == isValid && cursor.pos == pos && cursor.line == line;
	}
	bool operator<(const LogicalCursor& cursor) const
	{
		return (isValid == cursor.isValid) && (line < cursor.line || (line == cursor.line && pos < cursor.pos));
	}
	bool operator<=(const LogicalCursor& cursor) const
	{
		return operator<(cursor) || operator==(cursor);
	}
	bool operator>(const LogicalCursor& cursor) const
	{
		return !(operator<=(cursor));
	}
	bool operator>=(const LogicalCursor& cursor) const
	{
		return !(operator<(cursor));
	}
	bool operator!=(const LogicalCursor& cursor) const
	{
		return !(operator==(cursor));
	}
};



/////////////////////////////////////////////////
/// \brief Cursor, which is used in the terminal.
/// The TextManager is able to convert this
/// cursor into a LogicalCursor.
/////////////////////////////////////////////////
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
	bool operator==(const ViewCursor& cursor) const
	{
		return cursor.isValid == isValid && cursor.x == x && cursor.y == y;
	}
	bool operator<(const ViewCursor& cursor) const
	{
		return (isValid == cursor.isValid) && (y < cursor.y || (y == cursor.y && x < cursor.x));
	}
	bool operator<=(const ViewCursor& cursor) const
	{
		return operator<(cursor) || operator==(cursor);
	}
	bool operator>(const ViewCursor& cursor) const
	{
		return !(operator<=(cursor));
	}
	bool operator>=(const ViewCursor& cursor) const
	{
		return !(operator<(cursor));
	}
	bool operator!=(const ViewCursor& cursor) const
	{
		return !(operator==(cursor));
	}
};



/////////////////////////////////////////////////
/// \brief This structure combines the rendered
/// line with its colors and its coordinates.
/////////////////////////////////////////////////
struct RenderedLine
{
	string sLine;
	vector<unsigned short> colors;
	vector<LogicalCursor> coords;
};



/////////////////////////////////////////////////
/// \brief This structure combines the character
/// information needed in the terminal: the
/// character itself, the associated colour and
/// the editable flag.
/////////////////////////////////////////////////
struct Character
{
    char m_char;
    int m_style;

    enum
    {
        USERTEXT = 0x1000,
        EDITABLE = 0x2000,
        SELECTED = 0x4000,
        COLOR = 0xFFF,
        FLAGS = 0xF000
    };

    Character() : m_char(0), m_style(0) {}
    Character(char c) : m_char(c), m_style(0) {}
    Character(char&& c) : m_style(0)
    {
        m_char = std::move(c);
    }
    Character(const Character& c) : m_char(c.m_char), m_style(c.m_style) {}
    Character(Character&& c)
    {
        m_char = std::move(c.m_char);
        m_style = std::move(c.m_style);
    }
    Character(char c, unsigned char flags) : m_char(c), m_style(flags << 12) {}

    Character& operator=(const Character& c)
    {
        m_char = c.m_char;
        m_style = c.m_style;
        return *this;
    }

    Character& operator=(Character&& c)
    {
        m_char = std::move(c.m_char);
        m_style = std::move(c.m_style);
        return *this;
    }

    Character& operator=(char&& c)
    {
        m_char = std::move(c);
        return *this;
    }

    void setFlags(short flags)
    {
        m_style |= (unsigned char)flags << 12;
    }

    bool userText() const
    {
        return m_style & USERTEXT;
    }

    bool editable() const
    {
        return m_style & EDITABLE;
    }

    void makeUserText()
    {
        m_style |= USERTEXT;
        m_style &= ~EDITABLE;
    }

    void makeEditable()
    {
        m_style |= EDITABLE;
    }

    void select()
    {
        m_style |= SELECTED;
    }

    void unselect()
    {
        m_style &= ~SELECTED;
    }

    bool isSelected() const
    {
        return m_style & SELECTED;
    }

    int getColor() const
    {
        return m_style & COLOR;
    }

    void setColor(int color)
    {
        m_style = (m_style & FLAGS) | (color & COLOR);
    }
};



/////////////////////////////////////////////////
/// \brief This class resembles an extended
/// string class, which contains the extended
/// character class.
/////////////////////////////////////////////////
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

        const Character& operator[](size_t i) const
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

        const Character& front() const
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

        const Character& back() const
        {
            if (!size())
                return m_dummy;

            return vector<Character>::back();
        }

        string toString() const
        {
            string sRet;

            for (size_t i = 0; i < size(); i++)
                sRet.append(1u, operator[](i).m_char);

            return sRet;
        }

        string substr(size_t pos, size_t len = string::npos) const
        {
            return toString().substr(pos, len);
        }

        vector<unsigned short> subcolors(size_t pos, size_t len = string::npos) const
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

        size_t length() const
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



/////////////////////////////////////////////////
/// \brief This class manages the actual text by
/// storing it in an internal buffer. During an
/// update the TextManager will render the text
/// in the internal buffer into the text block in
/// screen coordinates.
/////////////////////////////////////////////////
class TextManager
{
	public:
		TextManager(GenericTerminal* parent = nullptr, int width = 80, int height = 24, int maxWidth = 160, int maxHeight = 300);
		~TextManager();

		void printOutput(const string& sLine);
		void insertInput(const string& sLine, size_t logicalpos = string::npos);

		ViewCursor toViewCursor(const LogicalCursor& logCursor) const;
		ViewCursor getCurrentViewPos() const;
		LogicalCursor toLogicalCursor(const ViewCursor& viewCursor) const;
		LogicalCursor getCurrentLogicalPos() const;

		string getRenderedString(size_t viewLine) const;
		vector<unsigned short> getRenderedColors(size_t viewLine) const;

		size_t tab();
		void newLine();
		void backspace(const LogicalCursor& logCursor);
		void eraseLine();
		bool clearRange(const ViewCursor& cursor1, const ViewCursor& cursor2);

		void selectText(const ViewCursor& viewCursor, bool bSelect = true);
		void unselectAll();
		bool isSelected(const ViewCursor& viewCursor) const;
		bool isSelectedLogical(const LogicalCursor& cursor) const;
		string getSelectedText() const;
		string getCurrentInputLine() const;
		string getPreviousLine() const;

		int GetSize() const;
		int GetMaxSize() const;
		int GetHeight() const;
		int GetNumLinesScrolled() const;
		int GetLinesReceived() const;
		string GetInputHistory(bool vcursorup = true);
		string GetTextRange(int y, int x0, int x1) const;
		string GetWordAt(int y, int x) const;
		string GetWordStartAt(int y, int x) const;
		char GetCharAdjusted(int y, int x) const;
		char GetCharLogical(const LogicalCursor& cursor) const;
		bool IsUserText(int y, int x) const;
		bool IsEditable(int y, int x) const;
		bool IsEditableLogical(const LogicalCursor& logCursor) const;
		unsigned short GetColor(int y, int x) const;
		unsigned short GetColorAdjusted(int y, int x) const;

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

		int AdjustIndex(int index) const;

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
		deque<RenderedLine> m_renderedBlock;

		int calc_color(int fg, int bg, int flags);
		void updateColors(bool isErrorLine = false);
		void renderLayout();
		void synchronizeRenderedBlock(int linesToDelete);
		size_t findNextLinebreak(const string& currentLine, size_t currentLinebreak) const;

		enum
		{
		    KERNEL_TEXT,
		    USER_TEXT,
		    EDITABLE_TEXT
		};

};


#endif

