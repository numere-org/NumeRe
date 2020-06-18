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

#include <wx/image.h>
#include "dataops.hpp"
#include "container.hpp"
#include "../../kernel.hpp"
#include "../ui/language.hpp"
#include "../utils/tools.hpp"
#include "../utils/BasicExcel.hpp"
#include "../ui/error.hpp"
#include "../structures.hpp"
#include "../built-in.hpp"
#include "../maths/parser_functions.hpp"


static string getSourceForDataOperation(const string& sExpression, Indices& _idx, Parser& _parser, Datafile& _data, const Settings& _option);
static void evaluateTransposeForDataOperation(const string& sTarget, Indices& _iSourceIndex, Indices& _iTargetIndex, const Datafile& _data, bool bTranspose);
static void performDataOperation(const string& sSource, const string& sTarget, const Indices& _iSourceIndex, const Indices& _iTargetIndex, Datafile& _data, bool bMove, bool bTranspose);
static string getFilenameFromCommandString(string& sCmd, string& sParams, const string& sDefExt, Parser& _parser, Datafile& _data, Settings& _option);

extern Language _lang;


/////////////////////////////////////////////////
/// \brief This function exports the passed data
/// into an Excel 97-2003 worksheet.
///
/// \param _data Datafile&
/// \param _option Settings&
/// \param sCache const string&
/// \param sFileName const string&
/// \return void
///
/////////////////////////////////////////////////
void export_excel(Datafile& _data, Settings& _option, const string& sCache, const string& sFileName)
{
	using namespace YExcel;

	BasicExcel _excel;
	BasicExcelWorksheet* _sheet;
	BasicExcelCell* _cell;

	string sHeadLine;

	// Create a new sheet
	_excel.New(1);

	// Rename it so that it fits the cache name
	_excel.RenameWorksheet(0u, sCache.c_str());

	// Get a pointer to this sheet
	_sheet = _excel.GetWorksheet(0u);

	// Write the headlines in the first row
	for (long long int j = 0; j < _data.getCols(sCache); j++)
	{
	    // Get the current cell and the headline string
		_cell = _sheet->Cell(0u, j);
		sHeadLine = _data.getHeadLineElement(j, sCache);

		// Replace newlines with the corresponding character code
		while (sHeadLine.find("\\n") != string::npos)
			sHeadLine.replace(sHeadLine.find("\\n"), 2, 1, (char)10);

        // Write the headline
		_cell->SetString(sHeadLine.c_str());
	}

	// Now write the actual table
	for (long long int i = 0; i < _data.getLines(sCache); i++)
	{
		for (long long int j = 0; j < _data.getCols(sCache); j++)
		{
		    // Get the current cell (skip over the first row, because it contains the headline)
			_cell = _sheet->Cell(1 + i, j);

			// Write the cell contents, if the data table contains valid data
			// otherwise clear the cell
			if (_data.isValidEntry(i, j, sCache))
				_cell->SetDouble(_data.getElement(i, j, sCache));
			else
				_cell->EraseContents();
		}
	}

	// Save the excel file with the target filename
	_excel.SaveAs(sFileName.c_str());

	// Inform the user
	if (_option.getSystemPrintStatus())
		NumeReKernel::print(LineBreak(_lang.get("OUTPUT_FORMAT_SUMMARY_FILE", toString((_data.getLines(sCache) + 1)*_data.getCols(sCache)), sFileName), _option));
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper for the
/// Datafile object. It will simply do the whole
/// UI stuff and let the Datafile object do the
/// hard work.
///
/// \param _data Datafile&
/// \param _option Settings&
/// \param _parser Parser&
/// \param sFileName string
/// \return void
///
/////////////////////////////////////////////////
void load_data(Datafile& _data, Settings& _option, Parser& _parser, string sFileName)
{
    // check, if the filename is available
	if (!sFileName.length())
	{
	    // If not, prompt to the user
		NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ENTER_NAME", _data.getPath()), _option));
		do
		{
			NumeReKernel::printPreFmt("|\n|<- ");
			NumeReKernel::getline(sFileName);		// gesamte Zeile einlesen: Koennte ja auch eine Leerstelle enthalten sein
			StripSpaces(sFileName);
		}
		while (!sFileName.length());

		// If the user entered a 0 then he wants to abort
		if (sFileName == "0")
		{
			NumeReKernel::print(_lang.get("COMMON_CANCEL"));
			return;
		}

	}
	// No data available in memory?
	if (!_data.isValid())	// Es sind noch keine Daten vorhanden?
	{
		_data.openFile(sFileName, _option, false, false); 			// gesammelte Daten an die Klasse uebergeben, die den Rest erledigt
	}
	else	// Sind sie doch? Dann muessen wir uns was ueberlegen...
	{
	    // append the data?
		string c = "";
		NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ASK_APPEND", _data.getDataFileName("data")), _option));
		NumeReKernel::printPreFmt("|\n|<- ");
		NumeReKernel::getline(c);

		// Abort, if user entered a zero
		if (c == "0")
		{
			NumeReKernel::print(_lang.get("COMMON_CANCEL"));
			return;
		}
		else if (c == _lang.YES())		// Anhaengen?
		{
		    // append the data -> hand the control over to the corresponding function
			append_data("data -app=\"" + sFileName + "\" i", _data, _option);
		}
		else				// Nein? Dann vielleicht ueberschreiben?
		{
		    // overwrite?
			c = "";
			NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ASK_OVERRIDE"), _option));
			NumeReKernel::printPreFmt("|\n|<- ");
			NumeReKernel::getline(c);

			if (c == _lang.YES())					// Also ueberschreiben
			{
			    // Clear memory
				_data.removeData();			// Speicher freigeben...

				// Open the file and copy its contents
				_data.openFile(sFileName, _option, false, false);
            }
			else							// Kannst du dich vielleicht mal entscheiden?
			{
			    // User aborts
				NumeReKernel::print(_lang.get("COMMON_CANCEL"));
			}
		}
	}
}


/////////////////////////////////////////////////
/// \brief This function presents the passed data
/// to the user in a visual way.
///
/// \param _data Datafile&
/// \param _out Output&
/// \param _option Settings&
/// \param _sCache const string&
/// \param nPrecision size_t
/// \param bData bool
/// \param bCache bool
/// \param bSave bool
/// \param bDefaultName bool
/// \return void
///
/////////////////////////////////////////////////
void show_data(Datafile& _data, Output& _out, Settings& _option, const string& _sCache, size_t nPrecision, bool bData, bool bCache, bool bSave, bool bDefaultName)
{
	string sCache = _sCache;
	string sFileName = "";

	// Do only stuff, if data is available
	if (_data.isValid() || _data.isValidCache())		// Sind ueberhaupt Daten vorhanden?
	{
        // Set the correct cache state
		if (bCache && _data.isValidCache())
			_data.setCacheStatus(true);
		else if (bData && _data.isValid())
			_data.setCacheStatus(false);
		else
		{
			throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, "", SyntaxError::invalid_position);
		}

		if (_option.getUseExternalViewer() && !bSave)
        {
            NumeReKernel::showTable(_data.extractTable(sCache), sCache.substr(sCache.front() == '*' ? 1 : 0));
			return;
        }

		long long int nLine = 0;
		long long int nCol = 0;
		int nHeadlineCount = 0;

		// If the user wants to save the data
		if (bSave && bData)
		{
		    // If the user wants a default file name, create it here
			if (bDefaultName)
			{
			    // Get the stored data file name
				sFileName = _data.getDataFileName(sCache);

				// Remove the path part
				if (sFileName.find_last_of("/") != string::npos)
					sFileName = sFileName.substr(sFileName.find_last_of("/") + 1);
				if (sFileName.find_last_of("\\") != string::npos)
					sFileName = sFileName.substr(sFileName.find_last_of("\\") + 1);

				// Create the file name
				sFileName = _out.getPath() + "/copy_of_" + sFileName;

				// we are not able to write a LABX file, so we simply replace it with "dat"
				if (sFileName.length() > 5 && sFileName.substr(sFileName.length() - 5, 5) == ".labx")
					sFileName = sFileName.substr(0, sFileName.length() - 5) + ".dat";

				if (_option.getbDebug())
					NumeReKernel::print("DEBUG: sFileName = " + sFileName );

                // Set the file name
				_out.setFileName(sFileName);
			}
			_out.setStatus(true);
		}
		else if (bSave && bCache)
		{
		    // Save the cache
			if (bDefaultName)
			{
			    // set up the output class correspondingly
				_out.setPrefix(sCache);
				_out.generateFileName();
			}
			_out.setStatus(true);
		}

		// Handle Excel outputs
		if (bSave && _out.getFileName().substr(_out.getFileName().rfind('.')) == ".xls")
		{
		    // Pass the control and return afterwards
			export_excel(_data, _option, sCache, _out.getFileName());
			_out.reset();
			return;
		}

		// Get the string matrix
		string** sOut = make_stringmatrix(_data, _out, _option, sCache, nLine, nCol, nHeadlineCount, nPrecision, bSave);// = new string*[nLine];		// die eigentliche Ausgabematrix. Wird spaeter gefuellt an Output::format(string**,int,int,Output&) uebergeben

        // Remove the possible asterisk at the front of the cache name
		if (sCache.front() == '*')
			sCache.erase(0, 1); // Vorangestellten Unterstrich wieder entfernen
		if (_data.getCacheStatus() && !bSave)
		{
			_out.setPrefix("cache");
			if (_out.isFile())
				_out.generateFileName();
		}

		// Set the "plugin origin"
		_out.setPluginName("Datenanzeige der Daten aus " + _data.getDataFileName(sCache)); // Anzeige-Plugin-Parameter: Nur Kosmetik

        if (!_out.isFile())
        {
            // Print the table to the console: write the headline
            NumeReKernel::toggleTableStatus();
            make_hline();
            NumeReKernel::print("NUMERE: " + toUpperCase(sCache) + "()");
            make_hline();
        }

        // Format the table (either for the console or for the target file)
        _out.format(sOut, nCol, nLine, _option, (bData || bCache), nHeadlineCount);		// Eigentliche Ausgabe

        if (!_out.isFile())
        {
            // Print the table to the console: write the footer
            NumeReKernel::toggleTableStatus();
            make_hline();
        }
        _out.reset();						// Ggf. bFile in der Klasse = FALSE setzen
        if ((bCache || _data.getCacheStatus()) && bSave)
            _data.setSaveStatus(true);
        _data.setCacheStatus(false);

        // Clear the created memory
        for (long long int i = 0; i < nLine; i++)
        {
            delete[] sOut[i];		// WICHTIG: Speicher immer freigeben!
        }
        delete[] sOut;


		// Reset the Outfile and the Datafile class
	}
	else		// Offenbar sind gar keine Daten geladen. Was soll ich also anzeigen?
	{
        // Throw, if no data is available
		if (bCache)
			throw SyntaxError(SyntaxError::NO_CACHED_DATA, "", SyntaxError::invalid_position);
		else
			throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, "", SyntaxError::invalid_position);
	}
}


/////////////////////////////////////////////////
/// \brief This function transforms the data into
/// a string matrix and returns the corresponding
/// pointer.
///
/// \note The calling function is responsible to
/// clear the allocated memory.
///
/// \param _data Datafile&
/// \param _out Output&
/// \param _option Settings&
/// \param sCache const string&
/// \param nLines long longint&
/// \param nCols long longint&
/// \param nHeadlineCount int&
/// \param nPrecision size_t
/// \param bSave bool
/// \return string**
///
/////////////////////////////////////////////////
string** make_stringmatrix(Datafile& _data, Output& _out, Settings& _option, const string& sCache, long long int& nLines, long long int& nCols, int& nHeadlineCount, size_t nPrecision, bool bSave)
{
	nHeadlineCount = 1;

	// Deactivate the compact flag, if the user uses the external viewer
	if (_option.getUseExternalViewer())
		_out.setCompact(false);

    // If the compact flag is not set
	if (!_out.isCompact())
	{
	    // Get the dimensions of the complete headline (i.e. including possible linebreaks)
		nHeadlineCount = _data.getHeadlineCount(sCache);
	}

	// Get the dimensions of the data and add the needed headlins
	nLines = _data.getLines(sCache) + nHeadlineCount;		// Wir muessen Zeilen fuer die Kopfzeile hinzufuegen
	nCols = _data.getCols(sCache);

	// Check for a reasonable dimension
	if (!nCols)
		throw SyntaxError(SyntaxError::NO_CACHED_DATA, "", SyntaxError::invalid_position);

	if (_option.getbDebug())
		NumeReKernel::print("DEBUG: nLine = " + toString(nLines) + ", nCol = " + toString(nCols) );

    if (nLines == nHeadlineCount)
        nLines++;

    // Create the formatting memory
	string** sOut = new string*[nLines];		// die eigentliche Ausgabematrix. Wird spaeter gefuellt an Output::format(string**,int,int,Output&) uebergeben

	for (long long int i = 0; i < nLines; i++)
	{
		sOut[i] = new string[nCols];			// Vollstaendig Allozieren!
	}

	// create a character buffer for sprintf
	char cBuffer[50];

	// Format the table
	for (long long int i = 0; i < nLines; i++)
	{
		for (long long int j = 0; j < nCols; j++)
		{
		    // The first line (at least) is reserved for the headline
			if (!i)						// Erste Zeile? -> Kopfzeilen uebertragen
			{
			    // Get the headlines
				if (_out.isCompact())
					sOut[i][j] = _data.getTopHeadLineElement(j, sCache);
				else
					sOut[i][j] = _data.getHeadLineElement(j, sCache);

				if (_out.isCompact() && (int)sOut[i][j].length() > 11 && !bSave)
				{
                    // Truncate the headlines, if they are too long
					sOut[i][j].replace(8, string::npos, "...");
				}
				else if (nHeadlineCount > 1 && sOut[i][j].find("\\n") != string::npos)
				{
				    // Store the complete headlines separated into the different rows
					string sHead = sOut[i][j];
					int nCount = 0;

					for (unsigned int n = 0; n < sHead.length(); n++)
					{
						if (sHead.substr(n, 2) == "\\n")
						{
							sOut[i + nCount][j] = sHead.substr(0, n);
							sHead.erase(0, n + 2);
							n = 0;
							nCount++;
						}
					}

					sOut[i + nCount][j] = sHead;
				}

				// If this is the last column, then set the line counter to the last headline row
				if (j == nCols - 1)
					i = nHeadlineCount - 1;
				continue;
			}

			// Handle invalid numbers
			if (!_data.isValidEntry(i - nHeadlineCount, j, sCache))
			{
				sOut[i][j] = "---";			// Nullzeile? -> Da steht ja in Wirklichkeit auch kein Wert drin...
				continue;
			}

			// Handle infinity
			if (isinf(_data.getElement(i - nHeadlineCount, j, sCache)) && _data.getElement(i - nHeadlineCount, j, sCache) > 0)
            {
                sOut[i][j] = "inf";
                continue;
            }

            // Handle negative infinity
			if (isinf(_data.getElement(i - nHeadlineCount, j, sCache)) && _data.getElement(i - nHeadlineCount, j, sCache) < 0)
            {
                sOut[i][j] = "-inf";
                continue;
            }

			// Transform the data to strings and write it to the string table
			// We use the C-style conversion function sprintf(), because it is 4 times faster than
			// using the stringstream conversion way.
            if (_out.isCompact() && !bSave)
                sprintf(cBuffer, "%.*g", 4, _data.getElement(i - nHeadlineCount, j, sCache));
            else
                sprintf(cBuffer, "%.*g", nPrecision, _data.getElement(i - nHeadlineCount, j, sCache));

            sOut[i][j] = cBuffer;
		}
	}
	// return the string table
	return sOut;
}


/////////////////////////////////////////////////
/// \brief This function handles appending data
/// sets to already existing data.
///
/// \param __sCmd const string&
/// \param _data Datafile&
/// \param _option Settings&
/// \return void
///
/////////////////////////////////////////////////
void append_data(const string& __sCmd, Datafile& _data, Settings& _option)
{
	string sCmd = __sCmd;
	Datafile _cache;

	// Copy the default path and the path tokens
	_cache.setPath(_data.getPath(), false, _data.getProgramPath());
	_cache.setTokens(_option.getTokenPaths());
	int nArgument = 0;
	string sArgument = "";

	// Get string variable values, if needed
	if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
		NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

    // Add quotation marks around the argument
    // Assuming that "app" is the relevant parameter in the command expression
	addArgumentQuotes(sCmd, "app");

	// Parse the arguments if they contain strings. The argument is returned
	// in the corresponding passed argument string variable
	if (extractFirstParameterStringValue(sCmd, sArgument))
	{
	    // If the command expression contains the parameter "all" and the
	    // argument (i.e. the filename) contains wildcards
		if (findParameter(sCmd, "all") && (sArgument.find('*') != string::npos || sArgument.find('?') != string::npos))
		{
		    // Insert the default loadpath, if no path is passed
			if (sArgument.find('/') == string::npos)
				sArgument = "<loadpath>/" + sArgument;

            // Get the file list, which fulfills the file path scheme
			vector<string> vFilelist = getFileList(sArgument, _option, true);

			// Ensure that at least one file exists
			if (!vFilelist.size())
			{
				throw SyntaxError(SyntaxError::FILE_NOT_EXIST, __sCmd, SyntaxError::invalid_position, sArgument);
			}

			// Go through all elements in the vFilelist list
			for (unsigned int i = 0; i < vFilelist.size(); i++)
			{
			    // Load the data directly, if the data object is empty
				if (!_data.isValid())
				{
					_data.openFile(vFilelist[0], _option, false, true);
					continue;
				}

				// Clear the data in the cache
				_cache.removeData(false);

				// Load the data to the cache
				_cache.openFile(vFilelist[i], _option, false, true);

				// Melt the data in memory with the data in the cache
				_data.melt(_cache);
			}

			// Inform the user and return
			if (_data.isValid() && _option.getSystemPrintStatus())
				NumeReKernel::print(LineBreak(_lang.get("BUILTIN_APPENDDATA_ALL_SUCCESS", toString((int)vFilelist.size()), sArgument, toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option));
			return;
		}

		// If data is available
		if (_data.isValid())	// Sind ueberhaupt Daten in _data?
		{
		    // Load the data to cache
			if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
			{
				if (findParameter(sCmd, "head", '='))
					nArgument = findParameter(sCmd, "head", '=') + 4;
				else
					nArgument = findParameter(sCmd, "h", '=') + 1;
				nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
				_cache.openFile(sArgument, _option, false, true, nArgument);
			}
			else
				_cache.openFile(sArgument, _option, false, true);

            // Melt the data in memory with the data in the cache
			_data.melt(_cache);

			// Inform the user
			if (_cache.isValid() && _option.getSystemPrintStatus())
				NumeReKernel::print(LineBreak(_lang.get("BUILTIN_APPENDDATA_SUCCESS", _cache.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option));
        }
		else
		{
		    // Simply load the data directly -> Melting not needed
			if (findParameter(sCmd, "head", '=') || findParameter(sCmd, "h", '='))
			{
				if (findParameter(sCmd, "head", '='))
					nArgument = findParameter(sCmd, "head", '=') + 4;
				else
					nArgument = findParameter(sCmd, "h", '=') + 1;
				nArgument = StrToInt(getArgAtPos(sCmd, nArgument));
				_data.openFile(sArgument, _option, false, true, nArgument);
			}
			else
				_data.openFile(sArgument, _option, false, true);

            // Inform the user
			if (_data.isValid() && _option.getSystemPrintStatus())
				NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_SUCCESS", _data.getDataFileName("data"), toString(_data.getLines("data", true)), toString(_data.getCols("data", false))), _option));
        }
	}
	return;
}


/////////////////////////////////////////////////
/// \brief This function frees the passed
/// Datafile object.
///
/// \param
/// \param
/// \return
///
/////////////////////////////////////////////////
void remove_data(Datafile& _data, Settings& _option, bool bIgnore)
{
    // Only if data is available
	if (_data.isValid())
	{
	    // If the flag "ignore" is not set, ask the user for confirmation
		if (!bIgnore)
		{
			string c = "";
			NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEDATA_CONFIRM"), _option));
			NumeReKernel::printPreFmt("|\n|<- ");
			NumeReKernel::getline(c);

			if (c == _lang.YES())
			{
				_data.removeData();		// Wenn ja: Aufruf der Methode Datafile::removeData(), die den Rest erledigt
				NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEDATA_SUCCESS"), _option));
			}
			else					// Wieder mal anders ueberlegt, hm?
			{
				NumeReKernel::print(_lang.get("COMMON_CANCEL"));
			}
		}
		else
		{
		    // simply remove the data and inform the user, if the output is allowed
			_data.removeData();
			if (_option.getSystemPrintStatus())
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEDATA_SUCCESS"), _option));
		}
	}
	else if (_option.getSystemPrintStatus())
	{
		NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEDATA_NO_DATA"), _option));
	}
	return;
}


/////////////////////////////////////////////////
/// \brief This function removes all allocated
/// tables and frees the assigned memory.
///
/// \param _data Datafile&
/// \param _option Settings&
/// \param bIgnore bool
/// \return void
///
/////////////////////////////////////////////////
void clear_cache(Datafile& _data, Settings& _option, bool bIgnore)
{
    // Only if there is valid data in the cache
	if (_data.isValidCache())
	{
	    // If the flag "ignore" is not set, ask the user for confirmation
		if (!bIgnore)
		{
			string c = "";
			if (!_data.getSaveStatus())
				NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_CONFIRM_NOTSAFED"), _option));
			else
				NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_CONFIRM"), _option));
			NumeReKernel::printPreFmt("|\n|<- ");
			NumeReKernel::getline(c);

			if (c == _lang.YES())
			{
				string sAutoSave = _option.getSavePath() + "/cache.tmp";
				string sCache_file = _option.getExePath() + "/numere.cache";

				// Clear the complete cache and remove the cache files
				_data.clearCache();	// Wenn ja: Aufruf der Methode Datafile::clearCache(), die den Rest erledigt
				remove(sAutoSave.c_str());
				remove(sCache_file.c_str());
			}
			else					// Wieder mal anders ueberlegt, hm?
			{
				NumeReKernel::print(_lang.get("COMMON_CANCEL"));
			}
		}
		else
		{
			string sAutoSave = _option.getSavePath() + "/cache.tmp";
			string sCache_file = _option.getExePath() + "/numere.cache";

			// Clear the complete cache and remove the cache files
			_data.clearCache();
			remove(sAutoSave.c_str());
			remove(sCache_file.c_str());
		}

		// Inform the user, if printing is allowed
		if (_option.getSystemPrintStatus())
			NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_SUCCESS"), _option));
	}
	else if (_option.getSystemPrintStatus())
	{
		NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_EMPTY"), _option));
	}
	return;
}


/////////////////////////////////////////////////
/// \brief This static function searches for the
/// named table in the cache map, evaluates the
/// specified indices and deletes the
/// corresponding contents from the table.
///
/// \param sCache const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
static bool searchAndDeleteTable(const string& sCache, Parser& _parser, Datafile& _data, const Settings& _option)
{
    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
    {
        if (sCache.substr(0, sCache.find('(')) == iter->first)
        {
            // Cache was found
            // Get the indices from the cache expression
            Indices _iDeleteIndex = getIndices(sCache, _parser, _data, _option);

            // Check the indices
            if (!isValidIndexSet(_iDeleteIndex))
                return false;

            // Evaluate the indices
            if (_iDeleteIndex.row.isOpenEnd())
                _iDeleteIndex.row.setRange(0, _data.getLines(iter->first, false)-1);

            if (_iDeleteIndex.col.isOpenEnd())
                _iDeleteIndex.col.setRange(0, _data.getCols(iter->first)-1);

            // Delete the section identified by the cache expression
            // The indices are vectors
            _data.deleteBulk(iter->first, _iDeleteIndex.row, _iDeleteIndex.col);

            // Return true
            return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This static function searches for the
/// named cluster in the cluster map, evaluates
/// the specified indices and deletes the
/// corresponding contents from the cluster.
///
/// \param sCluster const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
static bool searchAndDeleteCluster(const string& sCluster, Parser& _parser, Datafile& _data, const Settings& _option)
{
    for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
    {
        if (sCluster.substr(0, sCluster.find('{')) == iter->first)
        {
            // Cache was found
            // Get the indices from the cache expression
            Indices _iDeleteIndex = getIndices(sCluster, _parser, _data, _option);

            // Check the indices
            if (!isValidIndexSet(_iDeleteIndex))
                return false;

            // Evaluate the indices
            if (_iDeleteIndex.row.isOpenEnd())
                _iDeleteIndex.row.setRange(0, _data.getCluster(iter->first).size()-1);

            // Delete the section identified by the cache expression
            // The indices are vectors
            _data.getCluster(iter->first).deleteItems(_iDeleteIndex.row);

            // Return true
            return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This function removes one or multiple
/// entries in the selected table or cluster.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool deleteCacheEntry(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
	bool bSuccess = false;

	// Remove the command from the command line
	sCmd.erase(0, findCommand(sCmd).nPos + findCommand(sCmd).sString.length());

	// As long as a next argument may be extracted
	while (getNextArgument(sCmd, false).length())
	{
	    // Get the next argument
		string sCache = getNextArgument(sCmd, true);

		// Ignore calls to "data()"
		if (sCache.substr(0, 5) == "data(")
			continue;

        // Try to find the current cache in the list of available caches
		StripSpaces(sCache);

		// Is it a normal table?
		if (!_data.isCluster(sCache) && searchAndDeleteTable(sCache, _parser, _data, _option))
            bSuccess = true;

        // Is it a cluster?
		if (_data.isCluster(sCache) && searchAndDeleteCluster(sCache, _parser, _data, _option))
            bSuccess = true;
	}

	// return the value of the boolean flag
	return bSuccess;
}


/////////////////////////////////////////////////
/// \brief This function copies whole chunks of
/// data between tables.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool CopyData(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
	string sToCopy = "";
	string sTarget = "";
	bool bTranspose = false;

	Indices _iCopyIndex;
	Indices _iTargetIndex;

	// Find the transpose flag
	if (findParameter(sCmd, "transpose"))
		bTranspose = true;

    // Get the target from the option or use the default one
    sTarget = evaluateTargetOptionInCommand(sCmd, "cache", _iTargetIndex, _parser, _data, _option);

    // Avoid data as target for this operation
    if (sTarget == "data")
        throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);


    // Isolate the expression
	sToCopy = sCmd.substr(sCmd.find(' '));

	// Get the actual source data name and the corresponding indices
	sToCopy = getSourceForDataOperation(sToCopy, _iCopyIndex, _parser, _data, _option);
	if (!sToCopy.length())
        return false;

    // Apply the transpose flag on the indices, if necessary
	evaluateTransposeForDataOperation(sTarget, _iCopyIndex, _iTargetIndex, _data, bTranspose);

	// Perform the actual data operation. Move is set to false
	performDataOperation(sToCopy, sTarget, _iCopyIndex, _iTargetIndex, _data, false, bTranspose);

	return true;
}


/////////////////////////////////////////////////
/// \brief This function will move the selected
/// part of a data table to a new location.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool moveData(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
	string sToMove = "";
	string sTarget = "";
	bool bTranspose = false;

	Indices _iMoveIndex;
	Indices _iTargetIndex;

	// Find the transpose flag
	if (findParameter(sCmd, "transpose"))
		bTranspose = true;

    // Get the target expression from the option. The default one is empty and will raise an error
	sTarget = evaluateTargetOptionInCommand(sCmd, "", _iTargetIndex, _parser, _data, _option);

	// If the target cache name is empty, raise an error
	if (!sTarget.length())
        return false;

    // Avoid data as target for this operation
    if (sTarget == "data")
        throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);

    // Isolate the expression
	sToMove = sCmd.substr(sCmd.find(' '));

	// Get the actual source data name and the corresponding indices
    sToMove = getSourceForDataOperation(sToMove, _iMoveIndex, _parser, _data, _option);
	if (!sToMove.length())
        return false;

    // Avoid "data" as source for moving
    if (sToMove == "data")
        throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);

    // Apply the transpose flag on the indices, if necessary
	evaluateTransposeForDataOperation(sTarget, _iMoveIndex, _iTargetIndex, _data, bTranspose);

	// Perform the actual data operation. Move is set to true
    performDataOperation(sToMove, sTarget, _iMoveIndex, _iTargetIndex, _data, true, bTranspose);

    return true;
}


/////////////////////////////////////////////////
/// \brief This function gets the source for a
/// data operation like copy and move.
///
/// \param sExpression const string&
/// \param _idx Indices&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return string
///
/////////////////////////////////////////////////
static string getSourceForDataOperation(const string& sExpression, Indices& _idx, Parser& _parser, Datafile& _data, const Settings& _option)
{
    string sSourceForFileOperation = "";

    // Try to find the corresponding data table in the set of available ones
    DataAccessParser _accessParser(sExpression);

    if (_accessParser.getDataObject().length())
    {
        sSourceForFileOperation = _accessParser.getDataObject();
        _idx = _accessParser.getIndices();

        if (!isValidIndexSet(_idx) || _accessParser.isCluster())
            return "";

        // Evaluate the indices
        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _data.getLines(sSourceForFileOperation, false)-1);

        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _data.getCols(sSourceForFileOperation, false)-1);
    }

	// Return the name of the table
    return sSourceForFileOperation;
}


/////////////////////////////////////////////////
/// \brief This function evaluates the transpose
/// flag and switches the indices correspondingly.
///
/// \param sTarget const string&
/// \param _iSourceIndex Indices&
/// \param _iTargetIndex Indices&
/// \param _data const Datafile&
/// \param bTranspose bool
/// \return void
///
/////////////////////////////////////////////////
static void evaluateTransposeForDataOperation(const string& sTarget, Indices& _iSourceIndex, Indices& _iTargetIndex, const Datafile& _data, bool bTranspose)
{
    if (!isValidIndexSet(_iTargetIndex))
    {
        // This section is for cases, in which the target was not defined
        // Get the dimensions of the target to calculate the indices correspondingly
        // Depending on the transpose flag, the rows and columns are exchanged
        if (!bTranspose)
        {
            _iTargetIndex.row = VectorIndex(0LL, _iSourceIndex.row.size());
            _iTargetIndex.col = VectorIndex(_data.getTableCols(sTarget, false), _data.getTableCols(sTarget, false) + _iSourceIndex.col.size());
        }
        else
        {
            _iTargetIndex.row = VectorIndex(0LL, _iSourceIndex.col.size());
            _iTargetIndex.col = VectorIndex(_data.getTableCols(sTarget, false), _data.getTableCols(sTarget, false) + _iSourceIndex.row.size());
        }
    }
    else if (_iTargetIndex.row.size())
    {
        // This section is for cases, in which the target was defined via vectors
        // Get the dimensions of the target to calculate the indices correspondingly
        // Depending on the transpose flag, the rows and columns are exchanged
        //
        // The vectors are cleared because they will probably contain not reasonable data
        if (!bTranspose)
        {
            if (_iTargetIndex.row.isOpenEnd())
                _iTargetIndex.row = VectorIndex(_iTargetIndex.row.front(), _iTargetIndex.row.front() + _iSourceIndex.row.size());

            if (_iTargetIndex.col.isOpenEnd())
                _iTargetIndex.col = VectorIndex(_iTargetIndex.col.front(), _iTargetIndex.col.front() + _iSourceIndex.col.size());
        }
        else
        {
            if (_iTargetIndex.row.isOpenEnd())
                _iTargetIndex.row = VectorIndex(_iTargetIndex.row.front(), _iTargetIndex.row.front() + _iSourceIndex.col.size());

            if (_iTargetIndex.col.isOpenEnd())
                _iTargetIndex.col = VectorIndex(_iTargetIndex.col.front(), _iTargetIndex.col.front() + _iSourceIndex.row.size());
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function will perform the actual
/// data operation.
///
/// \param sSource const string&
/// \param sTarget const string&
/// \param _iSourceIndex const Indices&
/// \param _iTargetIndex const Indices&
/// \param _data Datafile&
/// \param bMove bool
/// \param bTranspose bool
/// \return void
///
/////////////////////////////////////////////////
static void performDataOperation(const string& sSource, const string& sTarget, const Indices& _iSourceIndex, const Indices& _iTargetIndex, Datafile& _data, bool bMove, bool bTranspose)
{
    Datafile _cache;
    _cache.setCacheStatus(true);

    // First step: copy the contents to the Datafile _cache
    // If the move flag is set, then the contents are cleared at the source location
    // vector indices
    for (unsigned int i = 0; i < _iSourceIndex.row.size(); i++)
    {
        for (unsigned int j = 0; j < _iSourceIndex.col.size(); j++)
        {
            if (!i)
                _cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_iSourceIndex.col[j], sSource));

            if (_data.isValidEntry(_iSourceIndex.row[i], _iSourceIndex.col[j], sSource))
            {
                _cache.writeToTable(i, j, "cache", _data.getElement(_iSourceIndex.row[i], _iSourceIndex.col[j], sSource));

                if (bMove)
                    _data.deleteEntry(_iSourceIndex.row[i], _iSourceIndex.col[j], sSource);
            }
        }
    }

    // Second step: Copy the contents in "_cache" to the new location in the original Datafile object

    for (long long int i = 0; i < _cache.getTableLines("cache", false); i++)
    {
        // Break the operation, if the indices are marking a smaller section
        if (!bTranspose)
        {
            if (i >= _iTargetIndex.row.size())
                break;
        }
        else
        {
            if (i >= _iTargetIndex.col.size())
                break;
        }
        for (long long int j = 0; j < _cache.getTableCols("cache", false); j++)
        {
            if (!bTranspose)
            {
                // Write the headlines
                if (!_iTargetIndex.row[i] && _data.getHeadLineElement(_iTargetIndex.col[j], sTarget).substr(0, 6) == "Spalte")
                {
                    _data.setHeadLineElement(_iTargetIndex.col[j], sTarget, _cache.getHeadLineElement(j, "cache"));
                }

                // Break the operation, if the indices are marking a smaller section
                if (j > _iTargetIndex.col.size())
                    break;

                // Write the data. Invalid data is deleted explicitly, because it might already contain old data
                if (_cache.isValidEntry(i, j, "cache"))
                    _data.writeToTable(_iTargetIndex.row[i], _iTargetIndex.col[j], sTarget, _cache.getElement(i, j, "cache"));
                else if (_data.isValidEntry(_iTargetIndex.row[i], _iTargetIndex.col[j], sTarget))
                    _data.deleteEntry(_iTargetIndex.row[i], _iTargetIndex.col[j], sTarget);
            }
            else
            {
                // We don't have headlines in this case
                // Break the operation, if the indices are marking a smaller section
                if (j > _iTargetIndex.row.size())
                    break;

                // Write the data. Invalid data is deleted explicitly, because it might already contain old data
                if (_cache.isValidEntry(i, j, "cache"))
                    _data.writeToTable(_iTargetIndex.col[j], _iTargetIndex.row[i], sTarget, _cache.getElement(i, j, "cache"));
                else if (_data.isValidEntry(_iTargetIndex.col[j], _iTargetIndex.row[i], sTarget))
                    _data.deleteEntry(_iTargetIndex.col[j], _iTargetIndex.row[i], sTarget);
            }
        }
    }

    _data.setCacheStatus(false);
}


/////////////////////////////////////////////////
/// \brief This static function sorts strings and
/// is called by sortData, if the selected data
/// object equals "string".
///
/// \param sCmd string&
/// \param _idx Indices&
/// \param _parser Parser&
/// \param _data Datafile&
/// \return bool
///
/////////////////////////////////////////////////
static bool sortStrings(string& sCmd, Indices& _idx, Parser& _parser, Datafile& _data)
{
    vector<int> vSortIndex;

    // Evalulate special index values
	if (_idx.row.isOpenEnd())
		_idx.row.setRange(0, _data.getStringElements(_idx.col.front())-1);

	if (_idx.col.isOpenEnd())
		_idx.col.setRange(0, _data.getStringCols()-1);

    // Perform the actual sorting operation
    // The member function will be able to handle the remaining command line parameters by itself
	vSortIndex = _data.sortStringElements(_idx.row.front(), _idx.row.back(), _idx.col.front(), _idx.col.back(), sCmd.substr(11));

	// If the sorting index contains elements, the user had requested them
	if (vSortIndex.size())
	{
	    // Transform the integer indices into doubles
		vector<double> vDoubleSortIndex;

		for (size_t i = 0; i < vSortIndex.size(); i++)
			vDoubleSortIndex.push_back(vSortIndex[i]);

        // Set the vector name and set the vector for the parser
		sCmd = "_~sortIndex[]";
		_parser.SetVectorVar(sCmd, vDoubleSortIndex);
	}
	else
		sCmd.clear(); // simply clear, if the user didn't request a sorting index

    // Return true
	return true;
}


/////////////////////////////////////////////////
/// \brief This static function sorts clusters
/// and is called by sortData, if the selected
/// data object equals a cluster identifier.
///
/// \param sCmd string&
/// \param sCluster const string&
/// \param _idx Indices&
/// \param _parser Parser&
/// \param _data Datafile&
/// \return bool
///
/////////////////////////////////////////////////
static bool sortClusters(string& sCmd, const string& sCluster, Indices& _idx, Parser& _parser, Datafile& _data)
{
    vector<int> vSortIndex;
    NumeRe::Cluster& cluster = _data.getCluster(sCluster);

    // Evalulate special index values
	if (_idx.row.isOpenEnd())
		_idx.row.setRange(0, cluster.size()-1);

    // Perform the actual sorting operation
    // The member function will be able to handle the remaining command line parameters by itself
	vSortIndex = cluster.sortElements(_idx.row.front(), _idx.row.back(), sCmd.substr(5 + sCluster.length()));

	// If the sorting index contains elements, the user had requested them
	if (vSortIndex.size())
	{
	    // Transform the integer indices into doubles
		vector<double> vDoubleSortIndex;

		for (size_t i = 0; i < vSortIndex.size(); i++)
			vDoubleSortIndex.push_back(vSortIndex[i]);

        // Set the vector name and set the vector for the parser
		sCmd = "_~sortIndex[]";
		_parser.SetVectorVar(sCmd, vDoubleSortIndex);
	}
	else
		sCmd.clear(); // simply clear, if the user didn't request a sorting index

    // Return true
	return true;
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper for the
/// corresponding member function of the Datafile
/// object.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool sortData(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	vector<int> vSortIndex;
	DataAccessParser _accessParser(sCmd);

	if (!_accessParser.getDataObject().length())
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);

	// Get the indices
	Indices& _idx = _accessParser.getIndices();

	// Ensure that the indices are reasonable
	if (!isValidIndexSet(_idx))
		throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, "", _idx.row.front(), _idx.row.back(), _idx.col.front(), _idx.col.back());

	// If the current cache equals to "string", leave the function at
	// this point and redirect the control to the string sorting
	// function
	if (_accessParser.getDataObject() == "string")
        return sortStrings(sCmd, _idx, _parser, _data);

    // If the current cache equals a cluster, leave the function at
	// this point and redirect the control to the cluster sorting
	// function
	if (_accessParser.isCluster())
        return sortClusters(sCmd, _accessParser.getDataObject(), _idx, _parser, _data);

	// Evalulate special index values
	if (_idx.row.isOpenEnd())
		_idx.row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);
	if (_idx.col.isOpenEnd())
		_idx.col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);

    // Perform the actual sorting operation
    // The member function will be able to handle the remaining command line parameters by itself
	vSortIndex = _data.sortElements(_accessParser.getDataObject(), _idx.row.front(), _idx.row.back(), _idx.col.front(), _idx.col.back(), sCmd.substr(5 + _accessParser.getDataObject().length()));

	// If the sorting index contains elements, the user had requested them
	if (vSortIndex.size())
	{
	    // Transform the integer indices into doubles
		vector<double> vDoubleSortIndex;
		for (size_t i = 0; i < vSortIndex.size(); i++)
			vDoubleSortIndex.push_back(vSortIndex[i]);

        // Set the vector name and set the vector for the parser
		sCmd = "_~sortIndex[]";
		_parser.SetVectorVar(sCmd, vDoubleSortIndex);
	}
	else
		sCmd.clear(); // simply clear, if the user didn't request a sorting index

    // Return true
	return true;
}


/////////////////////////////////////////////////
/// \brief This function writes the string
/// contents in the command to a file.
///
/// \param sCmd string&
/// \param _data Datafile&
/// \param _option Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool writeToFile(string& sCmd, Datafile& _data, Settings& _option)
{
	fstream fFile;
	string sFileName = "";
	string sExpression = "";
	string sParams = "";
	string sArgument = "";
	bool bAppend = false;
	bool bTrunc = true;
	bool bNoQuotes = false;
	FileSystem _fSys;

	// Set default tokens and default path
	_fSys.setTokens(_option.getTokenPaths());
	_fSys.setPath(_option.getExePath(), false, _option.getExePath());

	// Try to find the parameter string
	if (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos)
	{
	    // Extract the parameters
		if (sCmd.find("-set") != string::npos)
		{
			sParams = sCmd.substr(sCmd.find("-set"));
			sCmd.erase(sCmd.find("-set"));
		}
		else
		{
			sParams = sCmd.substr(sCmd.find("--"));
			sCmd.erase(sCmd.find("--"));
		}

		// Get the file name
		if (findParameter(sParams, "file", '='))
		{
			if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sParams))
				NumeReKernel::getInstance()->getStringParser().getStringValues(sParams);
			addArgumentQuotes(sParams, "file");
			extractFirstParameterStringValue(sParams, sFileName);
			StripSpaces(sFileName);
			if (!sFileName.length())
				return false;
			string sExt = "";
			if (sFileName.find('.') != string::npos)
				sExt = sFileName.substr(sFileName.rfind('.'));

			// Some file extensions are protected
			if (sExt == ".exe" || sExt == ".dll" || sExt == ".sys")
			{
				throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sExt);
			}

			// Declare the current file extension as valid for the current process
			_fSys.declareFileType(sExt);

			// Create a valid file name
			sFileName = _fSys.ValidFileName(sFileName, sExt);

			// Scripts, procedures and data files may not be written directly
			// this avoids reloads during the execution and other unexpected
			// behavior
			if (sFileName.substr(sFileName.rfind('.')) == ".nprc" || sFileName.substr(sFileName.rfind('.')) == ".nscr" || sFileName.substr(sFileName.rfind('.')) == ".ndat")
			{
				string sErrorToken;
				if (sFileName.substr(sFileName.rfind('.')) == ".nprc")
					sErrorToken = "NumeRe-Prozedur";
				else if (sFileName.substr(sFileName.rfind('.')) == ".nscr")
					sErrorToken = "NumeRe-Script";
				else if (sFileName.substr(sFileName.rfind('.')) == ".ndat")
					sErrorToken = "NumeRe-Datenfile";
				throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sErrorToken);
			}
		}

		// Avoid quotation marks
		if (findParameter(sParams, "noquotes") || findParameter(sParams, "nq"))
			bNoQuotes = true;

        // Get the file open mode
		if (findParameter(sParams, "mode", '='))
		{
			if (getArgAtPos(sParams, findParameter(sParams, "mode", '=') + 4) == "append"
					|| getArgAtPos(sParams, findParameter(sParams, "mode", '=') + 4) == "app")
				bAppend = true;
			else if (getArgAtPos(sParams, findParameter(sParams, "mode", '=') + 4) == "trunc")
				bTrunc = true;
			else if (getArgAtPos(sParams, findParameter(sParams, "mode", '=') + 4) == "override"
					 || getArgAtPos(sParams, findParameter(sParams, "mode", '=') + 4) == "overwrite")
			{
				bAppend = false;
				bTrunc = false;
			}
			else
				return false;
		}
	}

	// Ensure that a filename is available
	if (!sFileName.length())
		throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

    // Extract the expression
	sExpression = sCmd.substr(findCommand(sCmd).nPos + findCommand(sCmd).sString.length());

	// Parse the expression, which should be a string
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sExpression) || _data.containsClusters(sExpression))
	{
		sExpression += " -komq";
		string sDummy = "";
		NumeReKernel::getInstance()->getStringParser().evalAndFormat(sExpression, sDummy, true);
	}
	else
		throw SyntaxError(SyntaxError::NO_STRING_FOR_WRITING, sCmd, SyntaxError::invalid_position);

	// Open the file in the selected mode
	if (bAppend)
		fFile.open(sFileName.c_str(), ios_base::app | ios_base::out | ios_base::ate);
	else if (bTrunc)
		fFile.open(sFileName.c_str(), ios_base::trunc | ios_base::out);
	else
	{
		if (!fileExists(sFileName))
			ofstream fTemp(sFileName.c_str());
		fFile.open(sFileName.c_str());
	}

	// Ensure that the file is read- and writable
	if (fFile.fail())
	{
		throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sCmd, SyntaxError::invalid_position, sFileName);
	}

	// Ensure that the expression has a length and is not only an empty quotation marks pair
	if (!sExpression.length() || sExpression == "\"\"")
		throw SyntaxError(SyntaxError::NO_STRING_FOR_WRITING, sCmd, SyntaxError::invalid_position);

    // Write the expression linewise
    // Add linebreaks after each subexpression
	while (sExpression.length())
	{
	    // get the next argument
		sArgument = getNextArgument(sExpression, true);
		StripSpaces(sArgument);

		// Remove quotation marks if desired
		if (bNoQuotes && sArgument[0] == '"' && sArgument[sArgument.length() - 1] == '"')
			sArgument = sArgument.substr(1, sArgument.length() - 2);

        // Write only strings, which are not empty
		if (!sArgument.length() || sArgument == "\"\"")
			continue;

        // Remove escaped characters
		while (sArgument.find("\\\"") != string::npos)
		{
			sArgument.erase(sArgument.find("\\\""), 1);
		}
		if (sArgument.length() >= 2 && sArgument.substr(sArgument.length() - 2) == "\\ ")
			sArgument.pop_back();

        // Pass the curent argument to the file stream
		fFile << sArgument << endl;

		// Break the loop, if the expression only contains a single comma
		if (sExpression == ",")
			break;
	}

	// close the file stream if it is open and return
	if (fFile.is_open())
		fFile.close();
	return true;
}


/////////////////////////////////////////////////
/// \brief This function reads the content of a
/// file as strings and copies them to a
/// temporary string vector variable.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool readFromFile(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
	string sFileName = "";
	string sInput = "";
	string sCommentEscapeSequence = "";
	string sParams = "";
	ifstream fFile;

	// Get the parameter list
	if (sCmd.rfind('-') != string::npos && !isInQuotes(sCmd, sCmd.rfind('-')))
	{
		sParams = sCmd.substr(sCmd.rfind('-'));
		sCmd.erase(sCmd.rfind('-'));
	}

	// Find the comment escape sequence in the parameter list if available
	if (findParameter(sParams, "comments", '='))
	{
		sCommentEscapeSequence = getArgAtPos(sParams, findParameter(sParams, "comments", '=') + 8);
		if (sCommentEscapeSequence != " ")
			StripSpaces(sCommentEscapeSequence);
		while (sCommentEscapeSequence.find("\\t") != string::npos)
			sCommentEscapeSequence.replace(sCommentEscapeSequence.find("\\t"), 2, "\t");
	}

	// Get the source file name from the command string or the parameter list
	sFileName = getFilenameFromCommandString(sCmd, sParams, ".txt", _parser, _data, _option);

	// Ensure that a filename is present
	if (!sFileName.length())
		throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

    // Clear the command line (it will contain the contents of the read file)
	sCmd.clear();

	// Open the file and ensure that it is readable
	fFile.open(sFileName.c_str());
	if (fFile.fail())
	{
		throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sFileName);
	}

	// create a new vector for the file's contents
	vector<string> vFileContents;

	// Read the complete file, where each line is a separate string expression
	while (!fFile.eof())
	{
		getline(fFile, sInput);

		// Omit empty lines
		if (!sInput.length() || sInput == "\"\"" || sInput == "\"")
			continue;

        // Remove comments from the read lines (only line comments are supported)
		if (sCommentEscapeSequence.length() && sInput.find(sCommentEscapeSequence) != string::npos)
		{
			sInput.erase(sInput.find(sCommentEscapeSequence));
			if (!sInput.length() || sInput == "\"\"" || sInput == "\"")
				continue;
		}

		// Add the missing quotation marks
		if (sInput.front() != '"')
			sInput = '"' + sInput;
		if (sInput.back() == '\\')
			sInput += ' ';
		if (sInput.back() != '"')
			sInput += '"';

        // Escape backslashes
		for (unsigned int i = 1; i < sInput.length() - 1; i++)
		{
			if (sInput[i] == '\\')
				sInput.insert(i + 1, 1, ' ');
			if (sInput[i] == '"' && sInput[i - 1] != '\\')
				sInput.insert(i, 1, '\\');
		}

		// Append the parsed string to the vector
		vFileContents.push_back(sInput);
	}

    // Create a new temporary variable, if we actually
    // read something from the file
    if (vFileContents.size())
        sCmd = NumeReKernel::getInstance()->getStringParser().createTempStringVectorVar(vFileContents);
	else
		sCmd = "\"\"";

    // Close the file
	fFile.close();

	// return true
	return true;
}


/////////////////////////////////////////////////
/// \brief This function reads image data from an
/// image file and stores it as a cache table.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool readImage(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option)
{
	string sFileName = "";
	string sInput = "";
	string sParams = "";
	string sTargetCache = "image";
	Indices _idx;
	vector<double> vIndices;

	string sExpr = sCmd;

	Match _match = findCommand(sCmd, "imread");

	sExpr.replace(_match.nPos, string::npos, "_~imread[~_~]");
	sCmd.erase(0, _match.nPos);

	// Get the target cache from the command line or use the default one
	sTargetCache = evaluateTargetOptionInCommand(sCmd, "image", _idx, _parser, _data, _option);

	// Separate the parameter list from the command expression
	if (sCmd.rfind('-') != string::npos && !isInQuotes(sCmd, sCmd.rfind('-')))
	{
		sParams = sCmd.substr(sCmd.rfind('-'));
		sCmd.erase(sCmd.rfind('-'));
	}

	// Get the file name from the command line or the parameter list
	sFileName = getFilenameFromCommandString(sCmd, sParams, ".bmp", _parser, _data, _option);

	// Ensure that a filename is present
	if (!sFileName.length())
		throw SyntaxError(SyntaxError::NO_FILENAME, sCmd, SyntaxError::invalid_position);

    // Initialize all wxWidgets image handlers (should already be available, though)
	wxInitAllImageHandlers();

	// Create the image object
	wxImage image;

	// Load the file to the image object (wxWidgets will try to autodetect the image file format)
	// and throw an error, if it doesn't succeed
	if (!image.LoadFile(sFileName, wxBITMAP_TYPE_ANY))
	{
		throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sFileName);
	}

	// Get the dimensions of the data and the pointer to the data itself
	int nWidth = image.GetWidth();
	int nHeight = image.GetHeight();
	unsigned char* imageData = image.GetData();

	// Evaluate the indices correspondingly
	if (_idx.row.isOpenEnd())
		_idx.row.setRange(0, _idx.row.front() + nWidth-1);

	if (_idx.col.isOpenEnd())
		_idx.col.setRange(0, _idx.col.front() + 2 + nHeight-1);

    vIndices.push_back(_idx.row.min()+1);
    vIndices.push_back(_idx.row.max()+1);
    vIndices.push_back(_idx.col.min()+1);
    vIndices.push_back(_idx.col.max()+1);

    // Write the axes to the target cache
	for (int i = 0; i < nWidth; i++)
	{
		if (_idx.row[i] == VectorIndex::INVALID)
			break;

		_data.writeToTable(_idx.row[i], _idx.col.front(), sTargetCache, i + 1);
	}

	for (int i = 0; i < nHeight; i++)
	{
		if (_idx.row[i] == VectorIndex::INVALID)
			break;

		_data.writeToTable(_idx.row[i], _idx.col[1], sTargetCache, i + 1);
	}

	// Write headlines
	_data.setHeadLineElement(_idx.col[0], sTargetCache, "x");
	_data.setHeadLineElement(_idx.col[1], sTargetCache, "y");

	// iData is a iterator over the image data
	int iData;

	// Copy the average of the RGB channels (grey scale) to the data object
	for (int j = 0; j < nHeight; j++)
	{
		iData = 0;

		if (_idx.col[j+2] == VectorIndex::INVALID)
			break;

        _data.setHeadLineElement(_idx.col[2+j], sTargetCache, "z(x(:),y(" + toString(j) + "))");

		for (int i = 0; i < nWidth; i++)
		{
			if (_idx.row[i] == VectorIndex::INVALID)
				break;

            // The actual copy process
            // Calculate the luminosity of the three channels and write it to the table
			_data.writeToTable(_idx.row[i], _idx.col[2 + (nHeight - j - 1)], sTargetCache, imageData[j * 3 * nWidth + iData] * 0.3 + imageData[j * 3 * nWidth + iData + 1] * 0.59 + imageData[j * 3 * nWidth + iData + 2] * 0.11);

			// Advance the iterator three channels
			iData += 3;
		}
	}

	_parser.SetVectorVar("_~imread[~_~]", vIndices);
	sCmd = sExpr;

    // return true
	return true;
}


/////////////////////////////////////////////////
/// \brief This function extracts the filename
/// from a given (and already separated) command
/// string and returns it as a valid file name.
///
/// \param sCmd string&
/// \param sParams string&
/// \param sDefExt const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option Settings&
/// \return string
///
/////////////////////////////////////////////////
static string getFilenameFromCommandString(string& sCmd, string& sParams, const string& sDefExt, Parser& _parser, Datafile& _data, Settings& _option)
{
    string sFileName;
    string sExt = sDefExt;
    FileSystem _fSys;
	_fSys.setTokens(_option.getTokenPaths());
	_fSys.setPath(_option.getExePath(), false, _option.getExePath());

    // If the parameter list contains "file", use its value
    // Otherwise use the expression from the command line
	if (findParameter(sParams, "file", '='))
	{
	    // Parameter available
		if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sParams))
			NumeReKernel::getInstance()->getStringParser().getStringValues(sParams);

        if (_data.containsTablesOrClusters(sParams))
        if (_data.containsTablesOrClusters(sParams))
            getDataElements(sParams, _parser, _data, _option);
		addArgumentQuotes(sParams, "file");

		// Parse the string argument and return it in the second argument
		extractFirstParameterStringValue(sParams, sFileName);

		// Strip the spaces and ensure that there are other characters
		StripSpaces(sFileName);
		if (!sFileName.length())
			return "";

        // If the filename contains a extension, extract it here and declare it as a valid file type
		if (sFileName.find('.') != string::npos)
		{
			unsigned int nPos = sFileName.find_last_of('/');
			if (nPos == string::npos)
				nPos = 0;
			if (sFileName.find('\\', nPos) != string::npos)
				nPos = sFileName.find_last_of('\\');
			if (sFileName.find('.', nPos) != string::npos)
				sExt = sFileName.substr(sFileName.rfind('.'));

            // There are some protected ones
			if (sExt == ".exe" || sExt == ".dll" || sExt == ".sys")
			{
				throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sExt);
			}

			// Declare the extension
			_fSys.declareFileType(sExt);
		}
	}
	else
	{
	    // Use the expression
		if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sCmd))
			NumeReKernel::getInstance()->getStringParser().getStringValues(sCmd);

        if (_data.containsTablesOrClusters(sCmd))
            getDataElements(sCmd, _parser, _data, _option);

		// Get the expression
		if (sCmd.find(' ') != string::npos)
            sFileName = sCmd.substr(sCmd.find_first_not_of(' ', sCmd.find(' ')));
        else
            sFileName = sCmd;

		// Strip the spaces and ensure that there's something left
		StripSpaces(sFileName);
		if (!sFileName.length())
			return "";

        // If there's a string in the file name, parse it here
		if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sFileName))
		{
			sFileName += " -nq";
			string sDummy;
			NumeReKernel::getInstance()->getStringParser().evalAndFormat(sFileName, sDummy, true);
		}

		// If the filename contains a extension, extract it here and declare it as a valid file type
		if (sFileName.find('.') != string::npos)
		{
			unsigned int nPos = sFileName.find_last_of('/');
			if (nPos == string::npos)
				nPos = 0;
			if (sFileName.find('\\', nPos) != string::npos)
				nPos = sFileName.find_last_of('\\');
			if (sFileName.find('.', nPos) != string::npos)
				sExt = sFileName.substr(sFileName.rfind('.'));

            // There are some protected ones
			if (sExt == ".exe" || sExt == ".dll" || sExt == ".sys")
			{
				throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, sCmd, SyntaxError::invalid_position, sExt);
			}

			// Declare the extension
			_fSys.declareFileType(sExt);
		}
	}

	// Get a valid file name
    sFileName = _fSys.ValidFileName(sFileName, sExt);

    // Return the file name
    return sFileName;
}

