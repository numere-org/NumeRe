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


#include "../settings.hpp"
#include "memory.hpp"


#ifndef DATAFILE_HPP
#define DATAFILE_HPP

using namespace std;

/*
 * Header zur Datafile-Klasse
 */

class PasteHandler	//	Diese Klasse ist ein CHILD von FileSystem und von Cache
{
	private:
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
		PasteHandler();										// Standard-Konstruktor

		// Deprecated since v1.1.2rc1
        Memory* pasteLoad(const Settings& _option);
};

#endif
