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

#include "dataaccess.hpp"
#include "../utils/tools.hpp"
#include <vector>


void replaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement);
void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, Datafile& _data, Parser& _parser, const Settings& _option);
string createMafDataAccessString(const string& sAccessString, Parser& _parser);
string createEveryDefinition(const string& sLine, Parser& _parser);
string createMafVectorName(string sAccessString);
vector<double> MafDataAccess(Datafile& _data, const string& sMafname, const string& sCache, const string& sMafAccess);
string getMafFromAccessString(const string& sAccessString);
string getMafAccessString(const string& sLine, const string& sEntity);
void handleMafDataAccess(string& sLine, const string& sMafAccess, Parser& _parser, Datafile& _data);

/* --> Diese Funktion durchsucht einen gegebenen String sLine nach den Elementen "data(" oder "cache(" und erstetzt diese
 *     entsprechend der Syntax durch Elemente (oder Vektoren) aus dem Datenfile oder dem Cache. Falls des Weiteren auch
 *     noch Werte in den Cache geschrieben werden sollen (das Datenfile ist READ-ONLY), wird dies von dieser Funktion
 *     ebenfalls determiniert. <--
 * --> Um die ggf. ersetzten Vektoren weiterverwenden zu koennen, muss die Funktion parser_VectorToExpr() auf den String
 *     sLine angewendet werden. <--
 */
string getDataElements(string& sLine, Parser& _parser, Datafile& _data, const Settings& _option, bool bReplaceNANs)
{
    string sCache = "";             // Rueckgabe-string: Ggf. der linke Teil der Gleichung, falls es sich um eine Zuweisung handelt
    string sLine_Temp = "";         // temporaerer string, da wir die string-Referenz nicht unnoetig veraendern wollen
    unsigned int eq_pos = string::npos;                // int zum Zwischenspeichern der Position des "="

    int nParenthesis = 0;

    // Evaluate possible cached equations
    if (_parser.HasCachedAccess() && !_parser.IsCompiling())
    {
        mu::CachedDataAccess _access;
        Indices _idx;
        for (size_t i = 0; i < _parser.HasCachedAccess(); i++)
        {
            _access = _parser.GetCachedAccess(i);

            if (_access.sAccessEquation.find("().") != string::npos)
            {
                // handle cached maf methods
                _parser.SetVectorVar(_access.sVectorName, MafDataAccess(_data, getMafFromAccessString(_access.sAccessEquation), _access.sCacheName, createMafDataAccessString(_access.sAccessEquation, _parser)));
                continue;
            }

            _idx = parser_getIndices(_access.sAccessEquation, _parser, _data, _option);
            if ((_idx.nI[0] == -1 || _idx.nJ[0] == -1) && !_idx.vI.size() && !_idx.vJ.size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position);
            if (_idx.nI[1] == -2 && _idx.nJ[1] == -2)
                throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);
            if (_idx.nI[1] == -1)
                _idx.nI[1] = _idx.nI[0];
            if (_idx.nJ[1] == -1)
                _idx.nJ[1] = _idx.nJ[0];
            if (_idx.nI[1] == -2)
                _idx.nI[1] = _data.getLines(_access.sCacheName, false)-1;
            if (_idx.nJ[1] == -2)
                _idx.nJ[1] = _data.getCols(_access.sCacheName, false)-1;

            vector<long long int> vLine;
            vector<long long int> vCol;

            if (_idx.vI.size() && _idx.vJ.size())
            {
                vLine = _idx.vI;
                vCol = _idx.vJ;
            }
            else
            {
                for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                    vLine.push_back(i);
                for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                    vCol.push_back(j);
            }

            //_parser.SetVectorVar(_access.sVectorName, _data.getElement(vLine, vCol, _access.sCacheName));
            _data.copyElementsInto(_parser.GetVectorVar(_access.sVectorName), vLine, vCol, _access.sCacheName);
            _parser.UpdateVectorVar(_access.sVectorName);
        }
        sLine = _parser.GetCachedEquation();
        sCache = _parser.GetCachedTarget();
        return sCache;
    }


    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        if (sLine[i] == '(' && !isInQuotes(sLine, i, true))
            nParenthesis++;
        if (sLine[i] == ')' && !isInQuotes(sLine, i, true))
            nParenthesis--;
    }
    if (nParenthesis)
        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, SyntaxError::invalid_position);


    /* --> Jetzt folgt der ganze Spass fuer "cache(". Hier ist relativ viel aehnlich, allerdings gibt es
     *     noch den Fall, dass "cache(" links des "=" auftauchen darf, da es sich dabei um eine Zuweisung
     *     eines (oder mehrerer) Wert(e) an den Cache handelt. <--
     */
    if (_data.containsCacheElements(sLine))
    {
        // --> Ist links vom ersten "cache(" ein "=" oder ueberhaupt ein "=" im gesamten Ausdruck? <--
        eq_pos = sLine.find("=");
        if (eq_pos == string::npos              // gar kein "="?
            || !_data.containsCacheElements(sLine.substr(0,eq_pos))    // nur links von "cache("?
            || (_data.containsCacheElements(sLine.substr(0,eq_pos))   // wenn rechts von "cache(", dann nur Logikausdruecke...
                && (sLine[eq_pos+1] == '='
                    || sLine[eq_pos-1] == '<'
                    || sLine[eq_pos-1] == '>'
                    || sLine[eq_pos-1] == '!'
                    )
                )
            )
        {
            // --> Cache-Lese-Status aktivieren! <--
            _data.setCacheStatus(true);

            try
            {
                //cerr << _data.getCacheCount() << endl;
                //map<string,long long int> mCachesMap = _data.getCacheList();
                for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); iter++)
                {
                    //cerr << (iter->first)+"(" << endl;
                    if (sLine.find((iter->first)+"(") != string::npos)
                        replaceDataEntities(sLine, (iter->first)+"(", _data, _parser, _option, bReplaceNANs);
                }
            }
            catch (...)
            {
                _data.setCacheStatus(false);
                throw;
            }
            // --> Cache-Lese-Status deaktivieren <--
            _data.setCacheStatus(false);
        }
        else
        {
            /* --> Nein? Dann ist das eine Zuweisung. Wird komplizierter zu loesen. Außerdem kann dann rechts von
             *     "=" immer noch "cache(" auftreten. <--
             * --> Suchen wir zuerst mal nach der Position des "=" und speichern diese in eq_pos <--
             */
            /// !Achtung! Logikausdruecke abfangen!
            //eq_pos = sLine.find("=");
            // --> Teilen wir nun sLine an "=": Der Teillinks in sCache, der Teil rechts in sLine_Temp <--
            sCache = sLine.substr(0,eq_pos);
            StripSpaces(sCache);
            while (sCache[0] == '(')
                sCache.erase(0,1);
            // --> Gibt's innerhalb von "cache()" nochmal einen Ausdruck "cache("? <--
            if (_data.containsCacheElements(sCache.substr(sCache.find('(')+1)))
            {
                /*if (!_data.isValidCache())
                {
                    throw NO_CACHED_DATA;
                }*/
                _data.setCacheStatus(true);

                sLine_Temp = sCache.substr(sCache.find('(')+1);
                for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                {
                    if (sLine_Temp.find(iter->first+"(") != string::npos)
                        replaceDataEntities(sLine_Temp, iter->first+"(", _data, _parser, _option, bReplaceNANs);
                }
                sCache = sCache.substr(0,sCache.find('(')+1) + sLine_Temp;
                _data.setCacheStatus(false);
            }
            sLine_Temp = sLine.substr(eq_pos+1);

            // --> Gibt es rechts von "=" nochmals "cache("? <--
            if (_data.containsCacheElements(sLine_Temp))
            {
                /* --> Ja? Geht eigentlich trotzdem wie oben, mit Ausnahme, dass ueberall wo "sLine" aufgetreten ist,
                 *     nun "sLine_Temp" auftritt <--
                 */


                _data.setCacheStatus(true);

                try
                {
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (sLine_Temp.find(iter->first+"(") != string::npos)
                            replaceDataEntities(sLine_Temp, iter->first+"(", _data, _parser, _option, bReplaceNANs);
                    }
                }
                catch (...)
                {
                    _data.setCacheStatus(false);
                    throw;
                }
                _data.setCacheStatus(false);
            }
            // --> sLine_Temp an sLine zuweisen <--
            sLine = sLine_Temp;
        }
    }


    // --> Findest du "data("? <--
    if (sLine.find("data(") != string::npos)
    {
        // --> Sind ueberhaupt Daten vorhanden? <--
        if (!_data.isValid() && (!sLine.find("data(") || checkDelimiter(sLine.substr(sLine.find("data(")-1,6))))
        {
            /* --> Nein? Mitteilen, BOOLEAN setzen (der die gesamte, weitere Auswertung abbricht)
             *     und zurueck zur aufrufenden Funktion <--
             */
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sLine, SyntaxError::invalid_position);
        }
        // --> Ist rechts von "data(" noch ein "=" und gehoert das nicht zu einem Logik-Ausdruck? <--
        eq_pos = sLine.find("=", sLine.find("data(")+5);
        if (eq_pos != string::npos
            && sLine[eq_pos+1] != '='
            && sLine[eq_pos-1] != '<'
            && sLine[eq_pos-1] != '>'
            && sLine[eq_pos-1] != '!'
            && !parser_CheckMultArgFunc(sLine.substr(0,sLine.find("data(")), sLine.substr(sLine.find(")",sLine.find("data(")+1)))
            )
        {
            if (sLine.substr(sLine.find("data(")+5,sLine.find(",", sLine.find("data(")+5)-sLine.find("data(")-5).find("#") != string::npos)
            {
                sCache = sLine.substr(0,eq_pos);
                sLine = sLine.substr(eq_pos+1);
            }
            else
            {
                // --> Ja? Dann brechen wir ebenfalls ab, da wir nicht in data() schreiben wollen <--
                throw SyntaxError(SyntaxError::READ_ONLY_DATA, sLine, SyntaxError::invalid_position);
            }
        }
        /* --> Diese Schleife ersetzt nacheinander alle Stellen, in denen "data(" auftritt, durch "Vektoren", die
         *     in einer anderen Funktion weiterverarbeitet werden koennen. Eine aehnliche Schleife findet sich
         *     auch fuer "cache(" etwas weiter unten. <--
         * --> Wenn diese Schleife abgearbeitet ist, wird natuerlich auch noch geprueft, ob auch "cache(" gefunden
         *     wird und ggf. die Schleife fuer den Cache gestartet. <--
         */
        if (sLine.find("data(") != string::npos)
            replaceDataEntities(sLine, "data(", _data, _parser, _option, bReplaceNANs);
    }

    return sCache;
}

// DATA().FNC().cols().lines().grid().every(a,b)


/* --> Diese Funktion ersetzt in einem gegebenen String sLine alle Entities (sEntity) von "data(" oder "cache(" und bricht
 *     ab, sobald ein Fehler auftritt. Der Fehler wird in der Referenz von bSegmentationFault gespeichert und kann in
 *     in der aufrufenden Funktion weiterverarbeitet werden <--
 */
void replaceDataEntities(string& sLine, const string& sEntity, Datafile& _data, Parser& _parser, const Settings& _option, bool bReplaceNANs)
{
    Indices _idx;
    string sEntityOccurence = "";
    string sEntityName = sEntity.substr(0, sEntity.find('('));
    unsigned int nPos = 0;
    bool bWriteStrings = false;
    bool bWriteFileName = false;
    vector<double> vEntityContents;
    string sEntityReplacement = "";
    string sEntityStringReplacement = "";

    // handle MAF methods. sEntity already has "(" at its back
    while ((nPos = sLine.find(sEntity + ").", nPos)) != string::npos)
    {
        if (isInQuotes(sLine, nPos, true))
        {
            nPos++;
            continue;
        }
        handleMafDataAccess(sLine, getMafAccessString(sLine, sEntity), _parser, _data);
    }
    if (sLine.find(sEntity) == string::npos)
        return;

    nPos = 0;
    /* --> Diese Schleife ersetzt nacheinander alle Stellen, in denen sEntity auftritt, durch "Vektoren", die
     *     in einer anderen Funktion weiterverarbeitet werden koennen. <--
     */
    do
    {
        /* --> Zunaechst muessen wir herausfinden welche(s) Datenelement(e) wir aus der Datafile-Tabelle
         *     extrahieren sollen. Dies wird durch die Syntax data(i,j) bestimmt, wobei i der Zeilen- und
         *     j der Spaltenindex ist. i und j koennen mittels der Syntax "i_0:i_1" bzw. "j_0:j_1" einen
         *     Bereich bestimmen, allerdings (noch) keine Matrix. (Also entweder nur i oder j) <--
         * --> Speichere zunaechst den Teil des Strings nach "data(" in si_pos[0] <--
         */
        sEntityOccurence = sLine.substr(sLine.find(sEntity, nPos));
        nPos = sLine.find(sEntity, nPos);
        if (nPos && !checkDelimiter(sLine.substr(nPos-1, sEntity.length()+1)))
        {
            nPos++;
            continue;
        }
        //sEntityOccurence = sEntityOccurence.substr(0,getMatchingParenthesis(sEntityOccurence.substr(sEntityOccurence.find('('))) + sEntityOccurence.find('(')+1);
        sEntityOccurence = sEntityOccurence.substr(0,getMatchingParenthesis(sEntityOccurence)+1);
        vEntityContents.clear();
        sEntityReplacement.clear();
        sEntityStringReplacement.clear();

        // Reading the indices happens in this function
        _idx = parser_getIndices(sEntityOccurence, _parser, _data, _option);

        // evaluate the indices regarding the possible combinations
        if ((_idx.nI[0] == -1 || _idx.nJ[0] == -1) && !_idx.vI.size() && !_idx.vJ.size())
            throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position);
        if (_idx.nI[1] == -2 && _idx.nJ[1] == -2)
            throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);
        if (_idx.nI[1] == -1)
            _idx.nI[1] = _idx.nI[0];
        if (_idx.nJ[1] == -1)
            _idx.nJ[1] = _idx.nJ[0];
        if (_idx.nJ[1] == -2)
            _idx.nJ[1] = _data.getCols(sEntityName, false)-1;
        if (_idx.nI[1] == -2)
            _idx.nI[1] = _data.getLines(sEntityName, true)-_data.getAppendedZeroes(_idx.nJ[0], sEntityName)-1;
        if (_idx.nI[0] == -3)
            bWriteStrings = true;
        if (_idx.nJ[0] == -3)
            bWriteFileName = true;

        if (bWriteFileName)
        {
            sEntityStringReplacement = "\"" + _data.getDataFileName(sEntityName) + "\"";
        }
        else if (bWriteStrings)
        {
            if (_idx.vJ.size())
            {
                for (size_t j = 0; j < _idx.vJ.size(); j++)
                {
                    sEntityStringReplacement += "\"" + _data.getHeadLineElement(_idx.vJ[j], sEntityName) + "\", ";
                }
            }
            else
            {
                for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                {
                    sEntityStringReplacement += "\"" + _data.getHeadLineElement(j, sEntityName) + "\", ";
                }
            }
            if (sEntityStringReplacement.length())
                sEntityStringReplacement.erase(sEntityStringReplacement.rfind(','));
        }

        // create a vector containing the data
        if (_idx.vI.size() && _idx.vJ.size())
        {
            vEntityContents = _data.getElement(_idx.vI, _idx.vJ, sEntityName);
        }
        else
        {
            parser_CheckIndices(_idx.nI[0], _idx.nI[1]);
            parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);
            for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
            {
                for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                {
                    vEntityContents.push_back(_data.getElement(i, j, sEntityName));
                }
            }
        }

        // replace the occurences
        if (bWriteStrings)
        {
            _parser.DisableAccessCaching();
            replaceEntityStringOccurence(sLine, sEntityOccurence, sEntityStringReplacement);
        }
        else
        {
            sEntityReplacement = replaceToVectorname(sEntityOccurence);
            _parser.SetVectorVar(sEntityReplacement, vEntityContents);
            mu::CachedDataAccess _access = {sEntityName + "(" + _idx.sCompiledAccessEquation + ")", sEntityReplacement, sEntityName};
            _parser.CacheCurrentAccess(_access);

            replaceEntityOccurence(sLine, sEntityOccurence, sEntityName, sEntityReplacement, _idx, _data, _parser, _option);
        }
    }
    while (sLine.find(sEntity, nPos) != string::npos);

    return;
}

void replaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement)
{
    size_t nPos = 0;
    while ((nPos = sLine.find(sEntityOccurence)) != string::npos)
    {
        sLine.replace(nPos, sEntityOccurence.length(), sEntityStringReplacement);
    }
}

void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, Datafile& _data, Parser& _parser, const Settings& _option)
{
    sLine = " " + sLine + " ";

    vector<long long int> vLine;
    vector<long long int> vCol;
    size_t nPos = 0;

    if (_idx.vI.size() && _idx.vJ.size())
    {
        vLine = _idx.vI;
        vCol = _idx.vJ;
    }
    else
    {
        for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
            vLine.push_back(i);
        for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
            vCol.push_back(j);
    }

    while ((nPos = sLine.find(sEntityOccurence)) != string::npos)
    {
        string sLeft = sLine.substr(0, nPos);
        StripSpaces(sLeft);
        if (sLeft.length() < 3 || sLeft.back() != '(')
        {
            sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
            continue;
        }
        else if (sLeft.length() == 3)
        {
            if (sLeft.substr(sLeft.length()-3) == "or(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("or(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.or_func(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else
                sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
        }
        else if (sLeft.length() >= 4)
        {
            if (sLeft.substr(sLeft.length()-4) == "std(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("std(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.std(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "avg(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("avg(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.avg(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "max(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("max(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.max(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "min(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("min(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.min(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "prd(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("prd(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.prd(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "sum(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("sum(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.sum(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "num(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("num(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.num(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "and(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("and(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.and_func(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "xor(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("xor(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.xor_func(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-3) == "or(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("or(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.or_func(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "cnt(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("cnt(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.cnt(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "med(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("med(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.med(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.length() >= 5 && sLeft.substr(sLeft.length()-5) == "norm(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("norm(", sLine.find(sEntityOccurence)))
                    + toCmdString(_data.norm(sEntityName, vLine, vCol))
                    + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence)+sEntityOccurence.length())+1);
            }
            else if (sLeft.substr(sLeft.length()-4) == "cmp(")
            {
                _parser.DisableAccessCaching();
                double dRef = 0.0;
                int nType = 0;
                string sArg = "";
                sLeft = sLine.substr(sLine.find(sLeft)+sLeft.length(), getMatchingParenthesis(sLine.substr(sLine.find(sLeft)+sLeft.length()-1))-2);
                sArg = getNextArgument(sLeft, true);
                sArg = getNextArgument(sLeft, true);
                if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
                    getDataElements(sArg, _parser, _data, _option);
                _parser.SetExpr(sArg);
                dRef = (double)_parser.Eval();
                sArg = getNextArgument(sLeft, true);
                if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
                    getDataElements(sArg, _parser, _data, _option);
                _parser.SetExpr(sArg);
                nType = (int)_parser.Eval();
                sLine = sLine.replace(sLine.rfind("cmp(", sLine.find(sEntityOccurence)),
                    getMatchingParenthesis(sLine.substr(sLine.rfind("cmp(", sLine.find(sEntityOccurence))+3))+4,
                    toCmdString(_data.cmp(sEntityName, vLine, vCol, dRef, nType)));
            }
            else if (sLeft.substr(sLeft.length()-4) == "pct(")
            {
                _parser.DisableAccessCaching();
                double dPct = 0.5;
                string sArg = "";
                sLeft = sLine.substr(sLine.find(sLeft)+sLeft.length(), getMatchingParenthesis(sLine.substr(sLine.find(sLeft)+sLeft.length()-1))-2);
                sArg = getNextArgument(sLeft, true);
                sArg = getNextArgument(sLeft, true);
                if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
                    getDataElements(sArg, _parser, _data, _option);
                _parser.SetExpr(sArg);
                dPct = _parser.Eval();
                sLine = sLine.replace(sLine.rfind("pct(", sLine.find(sEntityOccurence)),
                    getMatchingParenthesis(sLine.substr(sLine.rfind("pct(", sLine.find(sEntityOccurence))+3))+4,
                    toCmdString(_data.pct(sEntityName, vLine, vCol, dPct)));
            }
            else
                sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
        }
    }
}

void handleMafDataAccess(string& sLine, const string& sMafAccess, Parser& _parser, Datafile& _data)
{
    size_t nPos = 0;
    string sMafVectorName = createMafVectorName(sMafAccess);

    //_parser.DisableAccessCaching();
    _parser.SetVectorVar(sMafVectorName, MafDataAccess(_data, getMafFromAccessString(sMafAccess), sMafAccess.substr(0, sMafAccess.find('(')), createMafDataAccessString(sMafAccess, _parser)));

    mu::CachedDataAccess _access = {sMafAccess, sMafVectorName, sMafAccess.substr(0, sMafAccess.find('('))};
    _parser.CacheCurrentAccess(_access);

    while ((nPos = sLine.find(sMafAccess, nPos)) != string::npos)
    {
        if (isInQuotes(sLine, nPos, true))
        {
            nPos++;
            continue;
        }
        sLine.replace(nPos, sMafAccess.length(), sMafVectorName);
    }
}

// Pass 'DATA().FNC().cols().lines().grid().every(a,b)'
string createMafDataAccessString(const string& sAccessString, Parser& _parser)
{
    string sDataMaf;
    //string sCache = sAccessString.substr(0, sAccessString.find('('));
    if (sAccessString.find(".grid") != string::npos)
        sDataMaf += "grid";
    if (sAccessString.find(".cols") != string::npos)
        sDataMaf += "cols";
    if (sAccessString.find(".lines") != string::npos)
        sDataMaf += "lines";
    if (sAccessString.find(".every(") != string::npos)
        sDataMaf += createEveryDefinition(sAccessString, _parser);
    return sDataMaf;
}

string getMafFromAccessString(const string& sAccessString)
{
    static const int sMafListLength = 13;
    static string sMafList[sMafListLength] = {"std", "avg", "prd", "sum", "min", "max", "norm", "num", "cnt", "med", "and", "or", "xor"};

    for (int i = 0; i < sMafListLength; i++)
    {
        if (sAccessString.find("."+sMafList[i]) != string::npos)
            return sMafList[i];
    }
    return "";
}

string getMafAccessString(const string& sLine, const string& sEntity)
{
    size_t nPos = 0;
    static string sDelim = "+-*/^%&| ,=<>!";
    if ((nPos = sLine.find(sEntity + ").")) != string::npos)
    {
        for (size_t i = nPos; i < sLine.length(); i++)
        {
            if (sLine[i] == '(')
                i += getMatchingParenthesis(sLine.substr(i));
            if (sDelim.find(sLine[i]) != string::npos)
                return sLine.substr(nPos, i-nPos);
            if (i+1 == sLine.length())
                return sLine.substr(nPos, i-nPos+1);
        }
    }
    return "";
}

vector<double> MafDataAccess(Datafile& _data, const string& sMafname, const string& sCache, const string& sMafAccess)
{
    if (sMafname == "std")
        return _data.std(sCache, sMafAccess);
    if (sMafname == "avg")
        return _data.avg(sCache, sMafAccess);
    if (sMafname == "prd")
        return _data.prd(sCache, sMafAccess);
    if (sMafname == "sum")
        return _data.sum(sCache, sMafAccess);
    if (sMafname == "min")
        return _data.min(sCache, sMafAccess);
    if (sMafname == "max")
        return _data.max(sCache, sMafAccess);
    if (sMafname == "norm")
        return _data.norm(sCache, sMafAccess);
    if (sMafname == "num")
        return _data.num(sCache, sMafAccess);
    if (sMafname == "cnt")
        return _data.cnt(sCache, sMafAccess);
    if (sMafname == "med")
        return _data.med(sCache, sMafAccess);
    if (sMafname == "and")
        return _data.and_func(sCache, sMafAccess);
    if (sMafname == "or")
        return _data.or_func(sCache, sMafAccess);
    if (sMafname == "xor")
        return _data.xor_func(sCache, sMafAccess);

    return vector<double>(1, NAN);
}

string createEveryDefinition(const string& sLine, Parser& _parser)
{
    value_type* v = 0;
    int nResults = 0;
    string sExpr = sLine.substr(sLine.find(".every(")+6);
    sExpr.erase(getMatchingParenthesis(sExpr));
    string sEvery;
    if (sExpr.front() == '(')
        sExpr.erase(0,1);
    if (sExpr.back() == ')')
        sExpr.pop_back();

    _parser.SetExpr(sExpr);
    v = _parser.Eval(nResults);

    if (nResults > 1)
    {
        sEvery = "every=" + toString((int)v[0]) + "," + toString((int)v[1]) + " ";
    }
    else
        sEvery = "every=" + toString((int)v[0]) + " ";
    return sEvery;
}

string createMafVectorName(string sAccessString)
{
    sAccessString.replace(sAccessString.find("()"), 2, "[");
    for (size_t i = 0; i < sAccessString.length(); i++)
    {
        if (sAccessString[i] == '.' || sAccessString[i] == '(' || sAccessString[i] == ')' || sAccessString[i] == ',')
            sAccessString[i] = '_';
    }
    return sAccessString + "]";
}

// this function is for extracting the data out of the data object and storing it to a continous block of memory
bool getData(const string& sTableName, Indices& _idx, const Datafile& _data, Datafile& _cache, int nDesiredCols, bool bSort)
{
    // write the data
    // the first case uses vector indices
    if (_idx.vI.size() || _idx.vJ.size())
    {
        for (long long int i = 0; i < _idx.vI.size(); i++)
        {
            for (long long int j = 0; j < _idx.vJ.size(); j++)
            {
                _cache.writeToCache(i, j, "cache", _data.getElement(_idx.vI[i], _idx.vJ[j], sTableName));
                if (!i)
                    _cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_idx.vJ[j], sTableName));
            }
        }
    }
    else // this is for the case of a continous block of data
    {
        if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
            throw SyntaxError(SyntaxError::INVALID_INDEX, sTableName, sTableName);
        if (_idx.nI[1] == -1)
            _idx.nI[1] = _idx.nI[0];
        else if (_idx.nI[1] == -2)
            _idx.nI[1] = _data.getLines(sTableName, false)-1;
        if (_idx.nJ[1] == -1)
            _idx.nJ[1] = _idx.nJ[0];
        else if (_idx.nJ[1] == -2)
        {
            _idx.nJ[1] = _idx.nJ[0]+nDesiredCols-1;
        }
        if (_data.getCols(sTableName, false) <= _idx.nJ[1])
            throw SyntaxError(SyntaxError::INVALID_INDEX, sTableName, sTableName);
        for (long long int i = 0; i <= _idx.nI[1]-_idx.nI[0]; i++)
        {
            if (nDesiredCols == 2)
            {
                _cache.writeToCache(i, 0, "cache", _data.getElement(_idx.nI[0]+i, _idx.nJ[0], sTableName));
                _cache.writeToCache(i, 1, "cache", _data.getElement(_idx.nI[0]+i, _idx.nJ[1], sTableName));
                if (!i)
                {
                    _cache.setHeadLineElement(0, "cache", _data.getHeadLineElement(_idx.nJ[0], sTableName));
                    _cache.setHeadLineElement(1, "cache", _data.getHeadLineElement(_idx.nJ[1], sTableName));
                }
            }
            else
            {
                for (long long int j = 0; j <= _idx.nJ[1]-_idx.nJ[0]; j++)
                {
                    _cache.writeToCache(i, j, "cache", _data.getElement(_idx.nI[0]+i, _idx.nJ[0]+j, sTableName));
                    if (!i)
                        _cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_idx.nJ[0]+j, sTableName));
                }
            }
        }
    }
    // sort, if sorting is activated
    if (bSort)
    {
        _cache.sortElements("cache -sort c=1[2:]");
    }
    return true;
}

Table parser_extractData(const string& sDataExpression, Parser& _parser, Datafile& _data, const Settings& _option)
{
    Cache _cache;
    string si_pos[2] = {"", ""};                    // String-Array fuer die Zeilen-Position: muss fuer alle Spalten identisch sein!
    string sj_pos[3] = {"", "", ""};                // String-Array fuer die Spalten: kann bis zu drei beliebige Werte haben
    string sDatatable = "data";
    int i_pos[2] = {0, 0};                          // Int-Array fuer den Wert der Zeilen-Positionen
    int j_pos[3] = {0, 0, 0};                       // Int-Array fuer den Wert der Spalten-Positionen
    int nMatch = 0;                                 // Int fuer die Position des aktuellen find-Treffers eines Daten-Objekts
    int nDim = 0;
    vector<long long int> vLine;
    vector<long long int> vCol;
    value_type* v = 0;
    int nResults = 0;

    // --> Ist da "cache" drin? Aktivieren wir den Cache-Status <--
    if (_data.containsCacheElements(sDataExpression) && sDataExpression.substr(0,5) != "data(")
    {
        if (_data.isValidCache())
            _data.setCacheStatus(true);
        else
            throw SyntaxError(SyntaxError::NO_CACHED_DATA, sDataExpression, SyntaxError::invalid_position);
        for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
        {
            if (sDataExpression.find(iter->first+"(") != string::npos
                && (!sDataExpression.find(iter->first+"(")
                    || (sDataExpression.find(iter->first+"(") && checkDelimiter(sDataExpression.substr(sDataExpression.find(iter->first+"(")-1, (iter->first).length()+2)))))
            {
                sDatatable = iter->first;
                break;
            }
        }
    }
    else if (!_data.isValid())
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sDataExpression, SyntaxError::invalid_position);
    // --> Klammer und schliessende Klammer finden und in einen anderen String schreiben <--
    nMatch = sDataExpression.find('(');
    si_pos[0] = sDataExpression.substr(nMatch, getMatchingParenthesis(sDataExpression.substr(nMatch))+1);
    if (si_pos[0] == "()" || si_pos[0][si_pos[0].find_first_not_of(' ',1)] == ')')
        si_pos[0] = "(:,:)";
    if (si_pos[0].find("data(") != string::npos || _data.containsCacheElements(si_pos[0]))
    {
        getDataElements(si_pos[0],  _parser, _data, _option);
    }

    if (_option.getbDebug())
        cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << endl;

    // --> Rausgeschnittenen String am Komma ',' in zwei Teile teilen <--
    try
    {
        parser_SplitArgs(si_pos[0], sj_pos[0], ',', _option);
    }
    catch (...)
    {
        throw;
    }
    if (_option.getbDebug())
        cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", sj_pos[0] = " << sj_pos[0] << endl;

    // --> Gibt's einen Doppelpunkt? Dann teilen wir daran auch noch mal <--
    if (si_pos[0].find(':') != string::npos)
    {
        si_pos[0] = "( " + si_pos[0] + " )";
        try
        {
            parser_SplitArgs(si_pos[0], si_pos[1], ':', _option);
        }
        catch (...)
        {
            throw;
        }
        if (!isNotEmptyExpression(si_pos[1]))
            si_pos[1] = "inf";
    }
    else
        si_pos[1] = "";


    // --> Auswerten mit dem Parser <--
    if (isNotEmptyExpression(si_pos[0]))
    {
        _parser.SetExpr(si_pos[0]);
        v = _parser.Eval(nResults);
        if (nResults > 1)
        {
            for (int n = 0; n < nResults; n++)
                vLine.push_back((int)v[n]-1);
        }
        else
            i_pos[0] = (int)v[0] - 1;
    }
    else
        i_pos[0] = 0;
    if (si_pos[1] == "inf")
    {
        i_pos[1] = _data.getLines(sDatatable, false)-1;
    }
    else if (isNotEmptyExpression(si_pos[1]))
    {
        _parser.SetExpr(si_pos[1]);
        i_pos[1] = (int)_parser.Eval() - 1;
    }
    else if (!vLine.size())
        i_pos[1] = i_pos[0]+1;
    // --> Pruefen, ob die Reihenfolge der Indices sinnvoll ist <--
    parser_CheckIndices(i_pos[0], i_pos[1]);

    if (!isNotEmptyExpression(sj_pos[0]))
        sj_pos[0] = "0";

    /* --> Jetzt fuer die Spalten: Fummelig. Man soll bis zu 6 Spalten angeben koennen und
     *     das Programm sollte trotzdem einen Sinn darin finden <--
     */
    int j = 0;
    try
    {
        while (sj_pos[j].find(':') != string::npos && j < 2)
        {
            sj_pos[j] = "( " + sj_pos[j] + " )";
            // --> String am naechsten ':' teilen <--
            parser_SplitArgs(sj_pos[j], sj_pos[j+1], ':', _option);
            // --> Spezialfaelle beachten: ':' ohne linke bzw. rechte Grenze <--
            if (!isNotEmptyExpression(sj_pos[j]))
                sj_pos[j] = "1";
            j++;
            if (!isNotEmptyExpression(sj_pos[j]))
                sj_pos[j] = "inf";
        }
    }
    catch (...)
    {
        throw;
    }
    // --> Alle nicht-beschriebenen Grenzen-Strings auf "" setzen <--
    for (int k = j+1; k < 3; k++)
        sj_pos[k] = "";

    // --> Grenzen-Strings moeglichst sinnvoll auswerten <--
    for (int k = 0; k <= j; k++)
    {
        // --> "inf" bedeutet "infinity". Ergo: die letztmoegliche Spalte <--
        if (sj_pos[k] == "inf")
        {
            j_pos[k] = _data.getCols(sDatatable);
            break;
        }
        else if (isNotEmptyExpression(sj_pos[k]))
        {
            if (j == 0)
            {
                _parser.SetExpr(sj_pos[0]);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                {
                    for (int n = 0; n < nResults; n++)
                    {
                        if (n >= 6)
                            break;
                        vCol.push_back((int)v[n]-1);
                        j_pos[n] = (int)v[n]-1;
                        j = n;
                    }
                    break;
                }
                else
                    j_pos[0] = (int)v[0] - 1;
            }
            else
            {
                // --> Hat einen Wert: Kann man auch auswerten <--
                _parser.SetExpr(sj_pos[k]);
                j_pos[k] = (int)_parser.Eval() - 1;
            }
        }
        else if (!k)
        {
            // --> erstes Element pro Forma auf 0 setzen <--
            j_pos[k] = 0;
        }
        else // "data(2:4::7) = Spalten 2-4,5-7"
        {
            // --> Spezialfall. Verwendet vermutlich niemand <--
            j_pos[k] = j_pos[k]+1;
        }
    }
    if (_option.getbDebug())
    if (i_pos[1] > _data.getLines(sDatatable, false))
        i_pos[1] = _data.getLines(sDatatable, false);
    if (j_pos[1] > _data.getCols(sDatatable)-1)
        j_pos[1] = _data.getCols(sDatatable)-1;
    if (!vLine.size() && !vCol.size() && (j_pos[0] < 0
        || j_pos[1] < 0
        || i_pos[0] > _data.getLines(sDatatable, false)
        || i_pos[1] > _data.getLines(sDatatable, false)
        || j_pos[0] > _data.getCols(sDatatable)-1
        || j_pos[1] > _data.getCols(sDatatable)-1))
    {
        throw SyntaxError(SyntaxError::INVALID_INDEX, sDataExpression, SyntaxError::invalid_position);
    }

    // --> Jetzt wissen wir die Spalten: Suchen wir im Falle von si_pos[1] == inf nach der laengsten <--
    if (si_pos[1] == "inf")
    {
        int nAppendedZeroes = _data.getAppendedZeroes(j_pos[0], sDatatable);
        for (int k = 1; k <= j; k++)
        {
            if (nAppendedZeroes > _data.getAppendedZeroes(j_pos[k], sDatatable))
                nAppendedZeroes = _data.getAppendedZeroes(j_pos[k], sDatatable);
        }
        if (nAppendedZeroes < i_pos[1])
            i_pos[1] = _data.getLines(sDatatable, true) - nAppendedZeroes-1;
    }


    /* --> Bestimmen wir die "Dimension" des zu fittenden Datensatzes. Dabei ist es auch
     *     von Bedeutung, ob Fehlerwerte verwendet werden sollen <--
     */
    nDim = 0;
    if (j == 0 && vCol.size() < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sDataExpression, SyntaxError::invalid_position);
    else if (j == 0)
        nDim = vCol.size();
    else
    {
        nDim = j+1;
    }

    if (vLine.size() && !vCol.size())
    {
        for (int n = 0; n < nDim; n++)
            vCol.push_back(j_pos[n]);
    }

    parser_CheckIndices(i_pos[0], i_pos[1]);

    if (abs(i_pos[0]-i_pos[1]) <= 1 && vLine.size() <= 1)
        throw SyntaxError(SyntaxError::TOO_FEW_LINES, sDataExpression, SyntaxError::invalid_position);

    if (!vLine.size())
    {
        if (nDim == 2)
        {
            for (int i = 0; i <= abs(i_pos[0]-i_pos[1]); i++)
            {
                _cache.writeToCache(i, 0, "cache", _data.getElement(i+i_pos[0], j_pos[0], sDatatable));
                _cache.writeToCache(i, 1, "cache", _data.getElement(i+i_pos[0], j_pos[1], sDatatable));
            }
        }
        else if (nDim == 3)
        {
            for (int i = 0; i <= abs(i_pos[0]-i_pos[1]); i++)
            {
                _cache.writeToCache(i, 0, "cache", _data.getElement(i+i_pos[0], j_pos[0], sDatatable));
                _cache.writeToCache(i, 1, "cache", _data.getElement(i+i_pos[0], j_pos[1], sDatatable));
                _cache.writeToCache(i, 2, "cache", _data.getElement(i+i_pos[0], j_pos[2], sDatatable));
            }
        }
    }
    else
    {
        if (nDim == 2)
        {
            for (size_t i = 0; i < vLine.size(); i++)
            {
                _cache.writeToCache(i, 0, "cache", _data.getElement(vLine[i], vCol[0], sDatatable));
                _cache.writeToCache(i, 1, "cache", _data.getElement(vLine[i], vCol[1], sDatatable));
            }
        }
        else if (nDim == 3)
        {
            for (size_t i = 0; i < vLine.size(); i++)
            {
                _cache.writeToCache(i, 0, "cache", _data.getElement(vLine[i], vCol[0], sDatatable));
                _cache.writeToCache(i, 1, "cache", _data.getElement(vLine[i], vCol[1], sDatatable));
                _cache.writeToCache(i, 2, "cache", _data.getElement(vLine[i], vCol[2], sDatatable));
            }
        }
    }

    _cache.sortElements("cache -sort c=1[2:]");

    _cache.renameCache("cache", sDatatable, true);

    return _cache.extractTable(sDatatable);
}


// --> Prueft, ob ein Ausdruck Nicht-Leer ist (also auch, dass er nicht nur aus Leerzeichen besteht) <--
bool isNotEmptyExpression(const string& sExpr)
{
    if (!sExpr.length())
        return false;
    return sExpr.find_first_not_of(' ') != string::npos;
}


/* --> Diese Funktion teilt den String sToSplit am char cSep auf, wobei oeffnende und schliessende
 *     Klammern beruecksichtigt werden <--
 */
int parser_SplitArgs(string& sToSplit, string& sSecArg, const char& cSep, const Settings& _option, bool bIgnoreSurroundingParenthesis)
{
    int nFinalParenthesis = 0;
    int nParenthesis = 0;
    int nV_Parenthesis = 0;
    int nSep = -1;

    StripSpaces(sToSplit);

    if (!bIgnoreSurroundingParenthesis)
    {
        // --> Suchen wir nach der schliessenden Klammer <--
        for (unsigned int i = 0; i < sToSplit.length(); i++)
        {
            if (sToSplit[i] == '(')
                nParenthesis++;
            if (sToSplit[i] == ')')
                nParenthesis--;
            if (!nParenthesis)
            {
                nFinalParenthesis = i;
                break;
            }
        }

        if (nParenthesis)
        {
            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sToSplit, SyntaxError::invalid_position);
        }
    }
    else
    {
        sToSplit = "(" + sToSplit + ")";
        nFinalParenthesis = sToSplit.length()-1;
    }
    // --> Trennen wir den Rest und die umschliessenden Klammern des Strings ab <--
    sToSplit = sToSplit.substr(1,nFinalParenthesis-1);

    // --> Suchen wir nach dem char cSep <--
    for (unsigned int i = 0; i < sToSplit.length(); i++)
    {
        if (sToSplit[i] == '(')
            nParenthesis++;
        if (sToSplit[i] == ')')
            nParenthesis--;
        if (sToSplit[i] == '{')
        {
            nV_Parenthesis++;
        }
        if (sToSplit[i] == '}')
        {
            nV_Parenthesis--;
        }
        if (sToSplit[i] == cSep && !nParenthesis && !nV_Parenthesis)
        {
            nSep = i;
            break;
        }
    }

    if (nSep == -1)
    {
        throw SyntaxError(SyntaxError::SEPARATOR_NOT_FOUND, sToSplit, SyntaxError::invalid_position);
    }

    // --> Teilen wir nun den string sToSplit in sSecArg und sToSplit auf <--
    sSecArg = sToSplit.substr(nSep+1);
    sToSplit = sToSplit.substr(0,nSep);
    return nFinalParenthesis;
}


/* --> Diese Funktion prueft, ob das Argument, dass sich zwischen sLeft und sRight befindet, in einer
 *     Multi-Argument-Funktion steht <--
 */
bool parser_CheckMultArgFunc(const string& sLeft, const string& sRight)
{
    int nPos = 0;
    string sFunc = "";
    bool bCMP = false;

    for (unsigned int i = 0; i < sRight.length(); i++)
    {
        if (sRight[i] != ' ')
        {
            if (sRight[i] == ')')
                break;
            else if (sRight[i] == ',')
            {
                if (/*sRight.find(',', i+1) != string::npos &&*/ sRight.find(')', i+1) != string::npos
                    /*&& sRight.find(',', i+1) < sRight.find(')', i+1)*/)
                    bCMP = true;
                else
                    return false;
                break;
            }
        }
    }
    for (int i = sLeft.length()-1; i >= 0; i--)
    {
        if (sLeft[i] != ' ')
        {
            if (sLeft[i] != '(')
                return false;
            nPos = i;
            break;
        }
    }
    if (nPos == 2)
    {
        sFunc = sLeft.substr(nPos - 2,2);
        if (sFunc == "or" && !bCMP)
            return true;
        return false;
    }
    else if (nPos >= 3)
    {
        sFunc = sLeft.substr(nPos - 3,3);
        if (sFunc == "max" && !bCMP)
            return true;
        else if (sFunc == "min" && !bCMP)
            return true;
        else if (sFunc == "sum" && !bCMP)
            return true;
        else if (sFunc == "avg" && !bCMP)
            return true;
        else if (sFunc == "num" && !bCMP)
            return true;
        else if (sFunc == "cnt" && !bCMP)
            return true;
        else if (sFunc == "med" && !bCMP)
            return true;
        else if (sFunc == "pct" && bCMP)
            return true;
        else if (sFunc == "std" && !bCMP)
            return true;
        else if (sFunc == "prd" && !bCMP)
            return true;
        else if (sFunc == "and" && !bCMP)
            return true;
        else if (sFunc.substr(1) == "or" && !bCMP)
            return true;
        else if (sFunc == "cmp" && bCMP)
        {
            //cerr << "cmp()" << endl;
            return true;
        }
        else if (sFunc == "orm" && !bCMP)
        {
            if (nPos > 3 && sLeft.substr(nPos - 4, 4) == "norm")
                return true;
            else
                return false;
        }
        else
            return false;
    }
    else
        return false;
}


/*
 * --> Gibt DATENELEMENT-Indices als Ints in einem Indices-Struct zurueck <--
 * --> Index = -1, falls der Index nicht gefunden wurde/kein DATENELEMENT uebergeben wurde <--
 * --> Index = -2, falls der Index den gesamten Bereich erlaubt <--
 * --> Index = -3, falls der Index eine Stringreferenz ist <--
 * --> Gibt alle angegeben Indices-1 zurueck <--
 */
Indices parser_getIndices(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    Indices _idx;
    string sI[2] = {"<<NONE>>", "<<NONE>>"};
    string sJ[2] = {"<<NONE>>", "<<NONE>>"};
    string sArgument = "";
    unsigned int nPos = 0;
    int nParenthesis = 0;
    value_type* v = 0;
    int nResults = 0;
    string sIndexExpressions;
    vector<int> vIndexNumbers;
    for (int i = 0; i < 2; i++)
    {
        _idx.nI[i] = -1;
        _idx.nJ[i] = -1;
    }
    //cerr << sCmd << endl;
    if (sCmd.find('(') == string::npos)
        return _idx;
    nPos = sCmd.find('(');
    for (unsigned int n = nPos; n < sCmd.length(); n++)
    {
        if (sCmd[n] == '(')
            nParenthesis++;
        if (sCmd[n] == ')')
            nParenthesis--;
        if (!nParenthesis)
        {
            sArgument = sCmd.substr(nPos+1, n-nPos-1);
            size_t nPos = 0;
            while ((nPos = sArgument.find(' ')) != string::npos)
                sArgument.erase(nPos, 1);
            break;
        }
    }
    if (sArgument.find("data(") != string::npos || _data.containsCacheElements(sArgument))
        getDataElements(sArgument, _parser, _data, _option);
    // --> Kurzschreibweise!
    if (!sArgument.length())
    {
        _idx.nI[0] = 0;
        _idx.nJ[0] = 0;
        _idx.nI[1] = -2;
        _idx.nJ[1] = -2;
        return _idx;
    }
    _idx.sCompiledAccessEquation = sArgument;
    //cerr << sArgument << endl;
    if (sArgument.find(',') != string::npos)
    {
        nParenthesis = 0;
        nPos = 0;
        for (unsigned int n = 0; n < sArgument.length(); n++)
        {
            if (sArgument[n] == '(' || sArgument[n] == '{')
                nParenthesis++;
            if (sArgument[n] == ')' || sArgument[n] == '}')
                nParenthesis--;
            if (sArgument[n] == ':' && !nParenthesis)
            {
                if (!nPos)
                {
                    if (!n)
                        sI[0] = "<<EMPTY>>";
                    else
                        sI[0] = sArgument.substr(0, n);
                }
                else if (n == nPos)
                {
                    sJ[0] = "<<EMPTY>>";
                }
                else
                {
                    sJ[0] = sArgument.substr(nPos, n-nPos);
                }
                nPos = n+1;
            }
            if (sArgument[n] == ',' && !nParenthesis)
            {
                if (!nPos)
                {
                    if (!n)
                        sI[0] = "<<EMPTY>>";
                    else
                        sI[0] = sArgument.substr(0, n);
                }
                else
                {
                    if (n == nPos)
                        sI[1] = "<<EMPTY>>";
                    else
                        sI[1] = sArgument.substr(nPos, n - nPos);
                }
                nPos = n+1;
            }
        }
        if (sJ[0] == "<<NONE>>")
        {
            if (nPos < sArgument.length())
                sJ[0] = sArgument.substr(nPos);
            else
                sJ[0] = "<<EMPTY>>";
        }
        else if (nPos < sArgument.length())
            sJ[1] = sArgument.substr(nPos);
        else
            sJ[1] = "<<EMPTY>>";

        // --> Vektor prüfen <--
        if (sI[0] != "<<NONE>>" && sI[1] == "<<NONE>>")
        {
            if (sI[0] == "#")
                _idx.nI[0] = -3;
            else
            {
                _parser.SetExpr(sI[0]);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                {
                    for (int n = 0; n < nResults; n++)
                    {
                        if (!isnan(v[n]) && !isinf(v[n]))
                            _idx.vI.push_back((int)v[n]-1);
                    }
                }
                else
                    _idx.nI[0] = (int)v[0]-1;
            }
        }
        if (sJ[0] != "<<NONE>>" && sJ[1] == "<<NONE>>")
        {
            if (sJ[0] == "#")
                _idx.nJ[0] = -3;
            else
            {
                _parser.SetExpr(sJ[0]);
                v = _parser.Eval(nResults);
                if (nResults > 1)
                {
                    for (int n = 0; n < nResults; n++)
                    {
                        if (!isnan(v[n]) && !isinf(v[n]))
                            _idx.vJ.push_back((int)v[n]-1);
                    }
                }
                else
                    _idx.nJ[0] = (int)v[0]-1;
            }
        }

        for (int n = 0; n < 2; n++)
        {
            //cerr << sI[n] << endl;
            //cerr << sJ[n] << endl;
            if (sI[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.nI[n] = -2;
                else
                    _idx.nI[0] = 0;
            }
            else if (sI[n] != "<<NONE>>")
            {
                if (!_idx.vI.size() && _idx.nI[0] != -3)
                {
                    if (sIndexExpressions.length())
                        sIndexExpressions += ",";
                    sIndexExpressions += sI[n];
                    vIndexNumbers.push_back((n+1));
                }
            }
            if (sJ[n] == "<<EMPTY>>")
            {
                if (n)
                    _idx.nJ[n] = -2;
                else
                    _idx.nJ[0] = 0;
            }
            else if (sJ[n] != "<<NONE>>")
            {
                if (!_idx.vJ.size() && _idx.nJ[0] != -3)
                {
                    if (sIndexExpressions.length())
                        sIndexExpressions += ",";
                    sIndexExpressions += sJ[n];
                    vIndexNumbers.push_back(-(n+1));
                }
            }
        }
        if (sIndexExpressions.length())
        {
            _parser.SetExpr(sIndexExpressions);
            v = _parser.Eval(nResults);
            if ((size_t)nResults != vIndexNumbers.size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
            for (int i = 0; i < nResults; i++)
            {
                if (isnan(v[i]) || v[i] <= 0)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
                if (isinf(v[i]))
                    v[i] = -1;
                if (vIndexNumbers[i] > 0)
                    _idx.nI[vIndexNumbers[i]-1] = (int)v[i]-1;
                else
                    _idx.nJ[abs(vIndexNumbers[i])-1] = (int)v[i]-1;
            }
        }
        if (_idx.vI.size() || _idx.vJ.size())
        {
            string sCache = sCmd.substr(0,sCmd.find('('));
            if (!sCache.length())
                throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);
            if (!sCache.find("data(") && !_data.isCacheElement(sCache))
                throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);
            if (sCache.find(' ') != string::npos)
                sCache.erase(0,sCache.rfind(' ')+1);
            if (!_idx.vI.size())
            {
                if (_idx.nI[0] == -1)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
                if (_idx.nI[1] == -2 && _idx.nI[0] != -3)
                {
                    for (long long int i = _idx.nI[0]; i < _data.getLines(sCache, false); i++)
                        _idx.vI.push_back(i);
                }
                else if (_idx.nI[1] == -1)
                    _idx.vI.push_back(_idx.nI[0]);
                else if (_idx.nI[0] != -3)
                {
                    for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
                        _idx.vI.push_back(i);
                }
            }
            if (!_idx.vJ.size())
            {
                if (_idx.nJ[0] == -1)
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
                if (_idx.nJ[1] == -2 && _idx.nJ[0] != -3)
                {
                    for (long long int j = _idx.nJ[0]; j < _data.getCols(sCache); j++)
                        _idx.vJ.push_back(j);
                }
                else if (_idx.nJ[1] == -1)
                    _idx.vJ.push_back(_idx.nJ[0]);
                else if (_idx.nJ[0] != -3)
                {
                    for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
                        _idx.vJ.push_back(j);
                }
            }
        }
    }
    return _idx;
}


// --> Prueft, ob der zweite Index groesser als der erste ist und vertauscht sie ggf. <--
void parser_CheckIndices(int& nIndex_1, int& nIndex_2)
{
    if (nIndex_1 < 0)
        nIndex_1 = 0;
    if (nIndex_2 < nIndex_1)
    {
        int nTemp = nIndex_1;
        nIndex_1 = nIndex_2,
        nIndex_2 = nTemp;
        if (nIndex_1 < 0)
            nIndex_1 = 0;
    }
    return;
}

// --> Prueft, ob der zweite Index groesser als der erste ist und vertauscht sie ggf. <--
void parser_CheckIndices(long long int& nIndex_1, long long int& nIndex_2)
{
    if (nIndex_1 < 0)
        nIndex_1 = 0;
    if (nIndex_2 < nIndex_1)
    {
        long long int nTemp = nIndex_1;
        nIndex_1 = nIndex_2,
        nIndex_2 = nTemp;
        if (nIndex_1 < 0)
            nIndex_1 = 0;
    }
    return;
}



