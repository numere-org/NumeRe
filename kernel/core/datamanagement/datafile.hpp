/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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


#include <string>
#include <vector>

#include "../ui/error.hpp"
#include "../settings.hpp"
#include "cache.hpp"


#ifndef DATAFILE_HPP
#define DATAFILE_HPP

string toString(int);
string toLowerCase(const string&);

using namespace std;

int findParameter(const string& sCmd, const string& sParam, const char cFollowing);
string getArgAtPos(const string& sCmd, unsigned int nPos);
void StripSpaces(string& sToStrip);
string getClipboardText();
string utf8parser(const string& sString);
int StrToInt(const string&);
/*
 * Header zur Datafile-Klasse
 */

class Datafile : public MemoryManager	//	Diese Klasse ist ein CHILD von FileSystem und von Cache
{
	private:
		ifstream file_in;								// ifstream, der zum Einlesen eines Datenfiles verwendet wird
		ofstream file_out;                              // ofstream, der zum Schreiben des Datenfiles verwendet wird
		bool bValidData;								// TRUE, wenn die Instanz der Klasse auch Daten enthaelt


		// Only needed for pasting data
		void replaceDecimalSign(string& _sToReplace)
        {
            if (_sToReplace.find(',') == string::npos)
                return;
            else
            {
                for (unsigned int i = 0; i < _sToReplace.length(); i++)
                {
                    if (_sToReplace[i] == ',')
                        _sToReplace[i] = '.';
                }
                return;
            }
        }

        // Only needed for pasting data
		void replaceTabSign(string& _sToReplace, bool bAddPlaceholders = false)
        {
            if (_sToReplace.find('\t') == string::npos)
                return;
            else
            {
                for (unsigned int i = 0; i < _sToReplace.length(); i++)
                {
                    if (_sToReplace[i] == '\t')
                    {
                        _sToReplace[i] = ' ';
                        if (bAddPlaceholders)
                        {
                            if (!i)
                                _sToReplace.insert(0,1,'_');
                            else if (_sToReplace[i-1] == ' ')
                                _sToReplace.insert(i,1,'_');
                            if (i+1 == _sToReplace.length())
                                _sToReplace += "_";
                        }
                    }
                }
                return;
            }
        }

        // Only needed for pasting data
		void stripTrailingSpaces(string& _sToStrip)
        {
            if (_sToStrip.find(' ') == string::npos && _sToStrip.find('\t') == string::npos)
                return;
            else
            {
                while (_sToStrip[_sToStrip.length()-1] == ' ' || _sToStrip[_sToStrip.length()-1] == '\t')
                {
                    _sToStrip = _sToStrip.substr(0,_sToStrip.length()-1);
                }
            }
            return;
        }

        // Only needed for pasting data
		bool isNumeric(const string& _sString);

        // DEPRECATED since v1.1.2rc1
        vector<string> getPastedDataFromCmdLine(const Settings& _option, bool& bKeepEmptyTokens);

 	public:
		Datafile();										// Standard-Konstruktor
		Datafile(long long int _nLines, long long int _nCols);	// Allgemeiner Konstruktor (generiert zugleich die Matrix dDatafile und die Arrays
                                                                // 		auf Basis der uebergeben Werte)
		~Datafile();									// Destruktor (wendet delete[] auf die Matrix und alle Arrays an, sofern es noetig ist)

		// Deprecated since v1.1.2rc1
        void pasteLoad(const Settings& _option);
// MEMORYMANAGER
		// MEMORYMANAGER
        void melt (Datafile& _cache);					// Methode, um die Daten einer anderen Instanz dieser Klasse den Daten dieser Klasse

};

#endif
