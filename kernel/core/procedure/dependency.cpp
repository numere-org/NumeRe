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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#include "dependency.hpp"
#include "procedureelement.hpp"
#include "../../kernel.hpp"

// CLASS DEPENDENCYLIST FUNCTIONS
//


/////////////////////////////////////////////////
/// \brief Static helper function for
/// DependencyList::unique.
///
/// \param first const Dependency&
/// \param second const Dependency&
/// \return bool
///
/////////////////////////////////////////////////
static bool compare(const Dependency& first, const Dependency& second)
{
    return first.getProcedureName() < second.getProcedureName();
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// DependencyList::unique.
///
/// \param first const Dependency&
/// \param second const Dependency&
/// \return bool
///
/////////////////////////////////////////////////
static bool isequal(const Dependency& first, const Dependency& second)
{
    return first.getProcedureName() == second.getProcedureName();
}


/////////////////////////////////////////////////
/// \brief Implementation of
/// DependencyList::unique.
///
/// \return void
///
/////////////////////////////////////////////////
void DependencyList::unique()
{
    sort(compare);
    list::unique(isequal);
}


// CLASS DEPENDENCIES FUNCTIONS
//


/////////////////////////////////////////////////
/// \brief Dependencies constructor
///
/// \param procedureFile ProcedureElement*
///
/////////////////////////////////////////////////
Dependencies::Dependencies(ProcedureElement* procedureFile)
{
    // Get a unix-like path name and extract the thisfile
    // namespace
    sFileName = replacePathSeparator(procedureFile->getFileName());
    sThisNameSpace = sFileName.substr(0, sFileName.rfind('/')+1);
    sThisFileNameSpacePrefix = sFileName.substr(sFileName.rfind('/')+1);
    sThisFileNameSpacePrefix.erase(sThisFileNameSpacePrefix.rfind('.'));
    sThisFileNameSpacePrefix += "::";

    // Get the current default procedure path
    std::string sProcDefPath = NumeReKernel::getInstance()->getSettings().getProcsPath();

    // If the default procedure path is part of the
    // thisfile namespace, remove this part and translate
    // it into an actual namespace
    if (sThisNameSpace.substr(0, sProcDefPath.length()) == sProcDefPath)
    {
        sThisNameSpace.erase(0, sProcDefPath.length());

        while (sThisNameSpace.front() == '/')
            sThisNameSpace.erase(0, 1);

        if (sThisNameSpace.length())
        {
            replaceAll(sThisNameSpace, "/", "~");

            while (sThisNameSpace.back() == '~')
                sThisNameSpace.pop_back();
        }
        else
            sThisNameSpace = "main";
    }
    else if (sThisNameSpace.length())
    {
        while (sThisNameSpace.back() == '/')
            sThisNameSpace.pop_back();
    }

    // Call Dependcies::walk() to calculate the dependencies
    walk(procedureFile);
}


/////////////////////////////////////////////////
/// \brief This member function will walk through
/// the file and redirect the control to
/// getProcedureDependencies(), if it hits a
/// procedure head.
///
/// \param procedureFile ProcedureElement*
/// \return void
///
/////////////////////////////////////////////////
void Dependencies::walk(ProcedureElement* procedureFile)
{
    int line = procedureFile->getFirstLine().first;

    // Walk through the whole file
    while (!procedureFile->isLastLine(line))
    {
        // Get the dependencies of the current procedure
        line = getProcedureDependencies(procedureFile, line);
    }

    // Make the list of dependencies unique
    for (auto iter = mDependencies.begin(); iter != mDependencies.end(); ++iter)
        iter->second.unique();
}


/////////////////////////////////////////////////
/// \brief This member function calculates the
/// dependencies of the current procedure.
///
/// \param procedureFile ProcedureElement*
/// \param nCurrentLine int
/// \return int
///
/////////////////////////////////////////////////
int Dependencies::getProcedureDependencies(ProcedureElement* procedureFile, int nCurrentLine)
{
    std::pair<int, ProcedureCommandLine> commandline = procedureFile->getCurrentLine(nCurrentLine);
    std::string sProcedureName;
    std::string sCurrentNameSpace = "main~";

    // Search for the head of the current procedure
    while (commandline.second.getType() != ProcedureCommandLine::TYPE_PROCEDURE_HEAD)
    {
        commandline = procedureFile->getNextLine(commandline.first);

        if (procedureFile->isLastLine(commandline.first))
            return commandline.first;
    }

    // extract procedure name
    sProcedureName = getProcedureName(commandline.second.getCommandLine());

    // Insert the "thisfile" namespace, if the current procedure is not
    // the main procedure of the current file
    if (sProcedureName.find('~') != std::string::npos && procedureFile->getFileName().substr(procedureFile->getFileName().rfind('/')+1) != sProcedureName.substr(sProcedureName.rfind('~')+1) + ".nprc")
        sProcedureName.insert(sProcedureName.rfind('~')+1, sThisFileNameSpacePrefix + "thisfile~");
    else if (sProcedureName.find('/') != std::string::npos && procedureFile->getFileName().substr(procedureFile->getFileName().rfind('/')+1) != sProcedureName.substr(sProcedureName.rfind('/')+1) + ".nprc")
        sProcedureName.insert(sProcedureName.rfind('/')+1, sThisFileNameSpacePrefix + "thisfile~");
    else
        sMainProcedure = sProcedureName;

    // Create a new (empty) dependency list
    mDependencies[sProcedureName] = DependencyList();

    // As long as we do not hit the procedure foot, we search for
    // namespace declarations and procedure calls
    while (commandline.second.getType() != ProcedureCommandLine::TYPE_PROCEDURE_FOOT && !procedureFile->isLastLine(commandline.first))
    {
        commandline = procedureFile->getNextLine(commandline.first);

        if (findCommand(commandline.second.getCommandLine()).sString == "namespace")
        {
            // Resolve the current namespace declaration
            sCurrentNameSpace = decodeNameSpace(commandline.second.getCommandLine(), sThisNameSpace);

            if (sCurrentNameSpace.length())
                sCurrentNameSpace += "~";
            else
                sCurrentNameSpace = "main~";
        }
        else
            resolveProcedureCalls(commandline.second.getCommandLine(), sProcedureName, sCurrentNameSpace);
    }

    // Return the foot line of the procedure
    return commandline.first;
}


/////////////////////////////////////////////////
/// \brief This member function extracts the
/// procedure name from the procedure head.
///
/// \param sCommandLine std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string Dependencies::getProcedureName(std::string sCommandLine) const
{
    if (sCommandLine.find("procedure ") == std::string::npos || sCommandLine.find('$') == std::string::npos)
        return "";

    if (sThisNameSpace.find('/') != std::string::npos)
        return "$" + sThisNameSpace + "/" + sCommandLine.substr(sCommandLine.find('$')+1, sCommandLine.find('(') - sCommandLine.find('$')-1);

    return "$" + sThisNameSpace + "~" + sCommandLine.substr(sCommandLine.find('$')+1, sCommandLine.find('(') - sCommandLine.find('$')-1);
}


/////////////////////////////////////////////////
/// \brief This member function resilves the
/// procedure calls contained in the current
/// procedure command line.
///
/// \param sLine std::string
/// \param sProcedureName const std::string&
/// \param sCurrentNameSpace const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Dependencies::resolveProcedureCalls(std::string sLine, const std::string& sProcedureName, const std::string& sCurrentNameSpace)
{
    if (sLine.find('$') != std::string::npos && sLine.find('(', sLine.find('$')) != std::string::npos)
	{
		sLine += " ";
		size_t nPos = 0;

		// Handle all procedure calls one after the other
		while (sLine.find('$', nPos) != std::string::npos && sLine.find('(', sLine.find('$', nPos)) != std::string::npos)
		{
			nPos = sLine.find('$', nPos) + 1;
            std::string __sName = sLine.substr(nPos, sLine.find('(', nPos) - nPos);

			if (!isInQuotes(sLine, nPos, true))
			{
                // Add namespaces, where necessary
                if (__sName.find('~') == std::string::npos)
                    __sName = sCurrentNameSpace + __sName;

                if (__sName.substr(0, 5) == "this~")
                    __sName.replace(0, 4, sThisNameSpace);

                if (__sName.substr(0, 9) == "thisfile~")
                    __sName = sThisNameSpace + "~" + sThisFileNameSpacePrefix + __sName;

                // Handle explicit procedure file names
                if (sLine[nPos] == '\'')
                    __sName = sLine.substr(nPos + 1, sLine.find('\'', nPos + 1) - nPos - 1);

                if (__sName.find('/') != std::string::npos && __sName.find("thisfile~") == std::string::npos)
                    replaceAll(__sName, "~", "/");

                // Add procedure name and called procedure file name to the
                // dependency list
                mDependencies[sProcedureName].push_back(Dependency("$" + __sName, getProcedureFileName(__sName)));
			}

            nPos += __sName.length() + 1;
		}
	}
}


/////////////////////////////////////////////////
/// \brief This member function returns the file
/// name of the current called procedure.
///
/// \param sProc std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string Dependencies::getProcedureFileName(std::string sProc) const
{
	if (sProc.length())
	{
		// Handle the "thisfile" namespace by using the call stack
		// to obtain the corresponding file name
		if (sProc.find("thisfile~") != std::string::npos)
		    return sFileName;

		// Create a valid file name from the procedure name
		sProc = NumeReKernel::getInstance()->getProcedureInterpreter().ValidFileName(sProc, ".nprc");

		// Replace tilde characters with path separators
		if (sProc.find('~') != std::string::npos)
		{
			size_t nPos = sProc.rfind('/');

            // Find the last path separator
			if (nPos < sProc.rfind('\\') && sProc.rfind('\\') != std::string::npos)
				nPos = sProc.rfind('\\');

            // Replace all tilde characters in the current path
            // string. Consider the special namespace "main", which
            // is a reference to the toplevel procedure folder
			for (size_t i = nPos; i < sProc.length(); i++)
			{
				if (sProc[i] == '~')
				{
					if (sProc.length() > 5 && i >= 4 && sProc.substr(i - 4, 5) == "main~")
						sProc = sProc.substr(0, i - 4) + sProc.substr(i + 1);
					else
						sProc[i] = '/';
				}
			}
		}

		// Append the newly obtained procedure file name
		// to the call stack
		return sProc;
	}

	return "";
}

