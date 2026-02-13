/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2026  Erik Haenel et al.

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

#include "utf8lib.hpp"
#include <boost/algorithm/hex.hpp>

#define UTF8_TWOBYTE 192
#define UTF8_THREEBYTE 224
#define UTF8_FOURBYTE 240

/////////////////////////////////////////////////
/// \brief Determines the number of bytes
/// associated with the current char. Returns
/// zero, if the current char points to a Utf8
/// continuation byte.
///
/// \param c char
/// \return size_t
///
/////////////////////////////////////////////////
size_t getUtf8ByteLen(char c)
{
    unsigned char uc = reinterpret_cast<unsigned char&>(c);

    if (uc < 128)
        return 1ull;

    if (uc >= UTF8_TWOBYTE && uc < UTF8_THREEBYTE)
        return 2ull;

    if (uc >= UTF8_THREEBYTE && uc < UTF8_FOURBYTE)
        return 3ull;

    if (uc >= UTF8_FOURBYTE)
        return 4ull;

    return 0ull;
}


/////////////////////////////////////////////////
/// \brief Transforms a UTF8 encoded string into
/// a Win CP1252 string in the internal code
/// page representation.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string utf8ToAnsi(const std::string& sString)
{
    // Static declaration of constant
	static const unsigned char NONANSIBITMASK = 128;
    std::string sWinCp = sString;

    for (size_t i = 0; i < sWinCp.length(); i++)
    {
        if (sWinCp[i] & NONANSIBITMASK)
        {
            if (sWinCp[i] == (char)0xC2)
            {
                // Regl. ANSI > 127
                sWinCp.erase(i, 1);
            }
            else if (sWinCp[i] == (char)0xE2 && sWinCp.length() > i+2)
            {
                // These are the about 20 characters, which have other
                // code points in Win-CP1252 and unicode
                // U+20.. and U+21..

                // Trademark symbol
                if (sWinCp[i+1] == (char)0x84 && (sWinCp[i+2] & ~(char)0xC0) == (char)0x22)
                {
                    sWinCp.erase(i, 2);
                    sWinCp[i] = (char)0x99;
                    continue;
                }

                // All other cases
                char sChar = sWinCp[i+2] & ~(char)0xC0;
                sChar += (sWinCp[i+1] & (char)0x3) << 6;
                sWinCp.erase(i, 2);

                // Switch as lookup table
                switch (sChar)
                {
                    case (char)0xAC:
                        sWinCp[i] = (char)0x80;
                        break;
                    case (char)0x1A:
                        sWinCp[i] = (char)0x82;
                        break;
                    case (char)0x1E:
                        sWinCp[i] = (char)0x84;
                        break;
                    case (char)0x26:
                        sWinCp[i] = (char)0x85;
                        break;
                    case (char)0x20:
                        sWinCp[i] = (char)0x86;
                        break;
                    case (char)0x21:
                        sWinCp[i] = (char)0x87;
                        break;
                    case (char)0x30:
                        sWinCp[i] = (char)0x89;
                        break;
                    case (char)0x39:
                        sWinCp[i] = (char)0x8B;
                        break;
                    case (char)0x18:
                        sWinCp[i] = (char)0x91;
                        break;
                    case (char)0x19:
                        sWinCp[i] = (char)0x92;
                        break;
                    case (char)0x1C:
                        sWinCp[i] = (char)0x93;
                        break;
                    case (char)0x1D:
                        sWinCp[i] = (char)0x94;
                        break;
                    case (char)0x22:
                        sWinCp[i] = (char)0x95;
                        break;
                    case (char)0x13:
                        sWinCp[i] = (char)0x96;
                        break;
                    case (char)0x14:
                        sWinCp[i] = (char)0x97;
                        break;
                    case (char)0x3A:
                        sWinCp[i] = (char)0x9B;
                        break;
                }
            }
            else if (sWinCp[i] >= (char)0xC4 && sWinCp[i] <= (char)0xCB)
            {
                // These are the about 20 characters, which have other
                // code points in Win-CP1252 and unicode
                // U+01.. and U+02..
                char sChar = sWinCp[i+1] & ~(char)0xC0;
                sChar += (sWinCp[i] & (char)0x3) << 6;
                sWinCp.erase(i, 1);

                // Switch as lookup table
                switch (sChar)
                {
                    case (char)0x92:
                        sWinCp[i] = (char)0x83;
                        break;
                    case (char)0xC6:
                        sWinCp[i] = (char)0x88;
                        break;
                    case (char)0x60:
                        sWinCp[i] = (char)0x8A;
                        break;
                    case (char)0x52:
                        sWinCp[i] = (char)0x8C;
                        break;
                    case (char)0x7D:
                        sWinCp[i] = (char)0x8E;
                        break;
                    case (char)0x7E:
                        sWinCp[i] = (char)0x9E;
                        break;
                    case (char)0x78:
                        sWinCp[i] = (char)0x9F;
                        break;
                    case (char)0xDC:
                        sWinCp[i] = (char)0x98;
                        break;
                    case (char)0x61:
                        sWinCp[i] = (char)0x9A;
                        break;
                    case (char)0x53:
                        sWinCp[i] = (char)0x9C;
                        break;
                }
            }
            else if (sWinCp[i] == (char)0xC3)
            {
                // Regl. ANSI > 127
                sWinCp.erase(i, 1);
                sWinCp[i] += 64;
            }
        }
    }

    return sWinCp;
}


/////////////////////////////////////////////////
/// \brief Creates an up to three byte sequence
/// UTF8 character from a two byte unicode
/// character. The length of the returned
/// byte sequence is stored in nLength.
///
/// \param cHighByte unsigned char
/// \param cLowByte unsigned char
/// \param nLength size_t&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createUTF8FromUnicode(unsigned char cHighByte, unsigned char cLowByte, size_t& nLength)
{
    std::string sUTF8;

    // If the high byte is larger than or equal to 0x08, we will
    // return three bytes, otherwise only two bytes
    if (cHighByte >= 0x08)
    {
        nLength = 3;
        sUTF8 = "   ";
        sUTF8[0] = (char)0xE0 | ((cHighByte & (unsigned char)0xF0) >> 4);
        sUTF8[1] = (char)0x80 | ((cHighByte & (unsigned char)0x0F) << 2) | ((cLowByte & (unsigned char)0xC0) >> 6);
        sUTF8[2] = (char)0x80 | (cLowByte & ~(char)0xC0);
    }
    else
    {
        nLength = 2;
        sUTF8 = "  ";
        sUTF8[0] = (char)0xC0 | ((cHighByte & (unsigned char)0x07) << 2) | ((cLowByte & (unsigned char)0xC0) >> 6);
        sUTF8[1] = (char)0x80 | (cLowByte & ~(char)0xC0);
    }

    return sUTF8;
}


/////////////////////////////////////////////////
/// \brief Transforms a Win CP1253 encoded string
/// into a UTF8 string in the internal code page
/// representation.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string ansiToUtf8(const std::string& sString)
{
    // Ensure that element text is available
    if (!sString.length())
        return std::string();

    // convert current element into a regular string
    // object (much easier to handle)
    std::string sUtf8String = sString;

    // static declarations of constants
    static const unsigned char NONANSIBITMASK = 128;
    static const unsigned char TWOBYTEUTF8 = 0xA0;

    // Go through the complete xml text and replace the
    // characters with the UTF8 encoded unicode value. The
    // unicode value of Win-CP1252 and unicode is quite similar
    // except of a set of about 24 characters between NONANSIBITMASK
    // and TWOBYTEUTF8
    for (size_t i = 0; i < sUtf8String.length(); i++)
    {
        // Only do something, if the character is larger than NONANSIBITMASK
        if (sUtf8String[i] & NONANSIBITMASK)
        {
            if ((unsigned char)sUtf8String[i] >= TWOBYTEUTF8)
            {
                // Regular encoding
                char cp1252Char = sUtf8String[i];
                sUtf8String.replace(i, 1, "  ");
                sUtf8String[i] = (char)0xC0 | ((cp1252Char & (unsigned char)0xC0) >> 6);
                sUtf8String[i+1] = (char)0x80 | (cp1252Char & ~(char)0xC0);
                i++;
            }
            else
            {
                // Special code sequences. These have other code points in UTF8
                // and therefore we have to replace them manually
                char cp1252Char = sUtf8String[i];
                size_t nLength = 0;

                // Switch as lookup table
                switch (cp1252Char)
                {
                    case (char)0x80:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0xAC, nLength));
                        break;
                    case (char)0x82:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x1A, nLength));
                        break;
                    case (char)0x83:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x92, nLength));
                        break;
                    case (char)0x84:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x1E, nLength));
                        break;
                    case (char)0x85:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x26, nLength));
                        break;
                    case (char)0x86:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x20, nLength));
                        break;
                    case (char)0x87:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x21, nLength));
                        break;
                    case (char)0x88:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x02, 0xC6, nLength));
                        break;
                    case (char)0x89:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x30, nLength));
                        break;
                    case (char)0x8A:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x60, nLength));
                        break;
                    case (char)0x8B:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x39, nLength));
                        break;
                    case (char)0x8C:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x52, nLength));
                        break;
                    case (char)0x8E:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x7D, nLength));
                        break;
                    case (char)0x91:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x18, nLength));
                        break;
                    case (char)0x92:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x19, nLength));
                        break;
                    case (char)0x93:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x1C, nLength));
                        break;
                    case (char)0x94:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x1D, nLength));
                        break;
                    case (char)0x95:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x22, nLength));
                        break;
                    case (char)0x96:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x13, nLength));
                        break;
                    case (char)0x97:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x14, nLength));
                        break;
                    case (char)0x98:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x02, 0xDC, nLength));
                        break;
                    case (char)0x99:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x21, 0x22, nLength));
                        break;
                    case (char)0x9A:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x61, nLength));
                        break;
                    case (char)0x9B:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x20, 0x3A, nLength));
                        break;
                    case (char)0x9C:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x53, nLength));
                        break;
                    case (char)0x9E:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x7E, nLength));
                        break;
                    case (char)0x9F:
                        sUtf8String.replace(i, 1, createUTF8FromUnicode(0x01, 0x78, nLength));
                        break;
                }

                i += nLength-1;

            }
        }
    }

    return sUtf8String;
}


/////////////////////////////////////////////////
/// \brief Returns the number of Unicode code
/// points in the UTF8 encoded string. This is
/// typical less than the number of bytes. It
/// does not correspond to the number of user-
/// perceivable glyphs.
///
/// \param sString const StringView
/// \return size_t
///
/////////////////////////////////////////////////
size_t countUnicodePoints(StringView sString)
{
    size_t cnt = 0;

    for (char c : sString)
    {
        size_t len = getUtf8ByteLen(c);

        if (len > 1)
            cnt += len-1;
    }

    return sString.length() - cnt;
}


/////////////////////////////////////////////////
/// \brief Find the byte position of the
/// character start.
///
/// \param sString StringView
/// \param pos size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t findCharStart(StringView sString, size_t pos)
{
    if (getUtf8ByteLen(sString[pos]) > 0ull)
        return pos;

    // Find character start to the left
    while (pos > 0 && getUtf8ByteLen(sString[pos]) == 0ull)
        pos--;

    return pos;
}


/////////////////////////////////////////////////
/// \brief Find the byte position of the closest
/// character start. Prefers positions to the
/// left, if both are equal in distance.
///
/// \param sString StringView
/// \param pos size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t findClosestCharStart(StringView sString, size_t pos)
{
    if (getUtf8ByteLen(sString[pos]) > 0ull)
        return pos;

    size_t pl = pos;
    size_t pr = pos;

    // Find character start to the left
    while (pl > 0 && getUtf8ByteLen(sString[pl]) == 0ull)
        pl--;

    // Find character start to the right
    while (pr+1 < sString.length() && getUtf8ByteLen(sString[pr]) == 0ull)
        pr++;

    // Find closest
    if (pr - pos >= pos - pl)
        return pl;

    return pr;
}


/////////////////////////////////////////////////
/// \brief Determine, whether the byte at the
/// position pos indicates a starting point for a
/// valid UTF8 char.
///
/// \param sString const std::string&
/// \param pos size_t
/// \return size_t
///
/////////////////////////////////////////////////
static size_t isValidUtf8CharStart(const std::string& sString, size_t pos)
{
    size_t byteLen = getUtf8ByteLen(sString[pos]);

    // Free continuation bytes are not allowed
    if (byteLen == 0ull)
        return 0ull;

    // Single byte is fine
    if (byteLen == 1ull)
        return byteLen;

    // Multi-byte case
    if (byteLen >= 2ull)
    {
        // Enough remaining bytes?
        if (pos+(byteLen-1) >= sString.length())
            return 0ull;

        // Following bytes incorrect?
        for (size_t contBytes = 1; contBytes < byteLen; contBytes++)
        {
            if (getUtf8ByteLen(sString[pos+contBytes]) > 0ull)
                return 0ull;
        }

        // If there is a byte right after, is it incorrect?
        if (pos+byteLen < sString.length()
            && getUtf8ByteLen(sString[pos+byteLen]) == 0ull)
            return 0ull;
    }

    return byteLen;
}


/////////////////////////////////////////////////
/// \brief Determine, whether the passed byte
/// sequence is a valid UTF8 byte sequence.
///
/// \param sString const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool isValidUtf8Sequence(const std::string& sString)
{
    for (size_t i = 0; i < sString.length(); i++)
    {
        // Determine, whether the current char is a valid
        // start and obtain its byte len
        size_t byteLen = isValidUtf8CharStart(sString, i);

        // Free continuation bytes are not allowed
        if (byteLen == 0ull)
            return false;

        // Advance over trailing bytes
        i += (byteLen-1);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Ensure that the passed string does
/// only contain valid UTF8 by converting the
/// invalid characters to hex values.
///
/// \param sString std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string ensureValidUtf8(std::string sString)
{
    for (size_t i = 0; i < sString.length(); i++)
    {
        // Determine, whether the current char is a valid
        // start and obtain its byte len
        size_t byteLen = isValidUtf8CharStart(sString, i);

        // Free continuation bytes are not allowed
        if (byteLen == 0ull)
        {
            sString.replace(i, 1, "\\x" + boost::algorithm::hex(sString.substr(i, 1)));
            i += 3;
            continue;
        }

        // Advance over trailing bytes
        i += (byteLen-1);
    }

    return sString;
}


/////////////////////////////////////////////////
/// \brief Static helper function to split a
/// StringView into a vector of UTF8 characters.
///
/// \param sStr StringView
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> splitUtf8Chars(StringView sStr)
{
    std::vector<std::string> vUtf8Chars;

    while (sStr.length())
    {
        size_t byteLen = getUtf8ByteLen(sStr.front());

        // Error case
        if (!byteLen)
        {
            sStr.trim_front(1);
            continue;
        }

        vUtf8Chars.push_back(sStr.subview(0, byteLen).to_string());
        sStr.trim_front(byteLen);
    }

    return vUtf8Chars;
}


/////////////////////////////////////////////////
/// \brief Static helper function to match a
/// vector of UTF8 characters to a string at a
/// position p
///
/// \param sString StringView
/// \param vUtf8Chars const std::vector<std::string>&
/// \param p size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t matchesAny(StringView sString, const std::vector<std::string>& vUtf8Chars, size_t p)
{
    for (const std::string& utf8Char : vUtf8Chars)
    {
        if (p+utf8Char.length() <= sString.length()
            && sString.match(utf8Char, p))
            return utf8Char.length();
    }

    return 0ull;
}

