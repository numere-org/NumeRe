#include <wx/filename.h>
#include "../../kernel.hpp"


/////////////////////////////////////////////////
/// \brief This function checks for the invalid
/// chars that can not appear in a valid directory
/// name.
///
/// \param sPathname std::string
/// \return bool
///
/////////////////////////////////////////////////
static bool checkInvalidChars(std::string sPathname)
{
    // Define the invalid chars
    const std::string sINVALID_CHARS = "\"#%&|*?";

    // Go through all chars and check for invalid ones
    for (size_t i = 0; i < sPathname.length(); i++)
    {
        if (sPathname[i] == '~' || sINVALID_CHARS.find(sPathname[i]) != std::string::npos)
            return false;
    }
    return true;
}


/////////////////////////////////////////////////
/// \brief This function checks whether a given
/// string is a valid directory path.
///
/// \param sPathname std::string
/// \return bool
///
/////////////////////////////////////////////////
bool is_dir(std::string sPathname)
{
    // Check for invalid chars
    if (!checkInvalidChars(sPathname))
        return false;

    // Get the FileSystem instance to replace symbols in the file name using an existing method
    FileSystem& fileSystemInstance = NumeReKernel::getInstance()->getFileSystem();
    sPathname = fileSystemInstance.ValidFolderName(sPathname, false);

    // Check for double ':' at the beginning of the path or left over '<' '>' symbols
    if (sPathname.find(':', 2) != std::string::npos || sPathname.find_first_of("<>") != std::string::npos)
        return false;

    // Check for a valid volume identifier (be of pattern "C:/" if not a network path)
    if (sPathname[2] != '/' && !(sPathname.starts_with("//") && isalpha(sPathname[2])))
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief This function checks whether a given
/// string is a valid file path.
///
/// \param sPathname std::string
/// \return bool
///
/////////////////////////////////////////////////
bool is_file(std::string sPathname)
{
    // Check for invalid chars
    if (!checkInvalidChars(sPathname))
        return false;

    // Get the FileSystem instance to replace symbols in the file name using an existing method
    FileSystem& fileSystemInstance = NumeReKernel::getInstance()->getFileSystem();
    sPathname = fileSystemInstance.ValidFileName(sPathname, "", false, false);

    // Check for double ':' at the beginning of the path or left over '<' '>' symbols
    if (sPathname.find(':', 2) != std::string::npos || sPathname.find_first_of("<>") != std::string::npos)
        return false;

    // Check for a valid volume identifier (be of pattern "C:/" if not a network path)
    if (sPathname[2] != '/' && !(sPathname.starts_with("//") && isalpha(sPathname[2])))
        return false;

    // Manually check if there could be file extension. Check for a dot and if there are chars after it
    if (sPathname.find_last_of(".") == std::string::npos || sPathname.back() == '.')
        return false;

    // The file extension can not contain anything but alphabetical chars
    size_t extStart = sPathname.find_last_of(".");
    std::string ext = sPathname.substr(extStart + 1, sPathname.length() - extStart);
    if (!std::all_of(ext.begin(), ext.end(), [](char const &c) { return std::isalnum(c);}))
        return false;

    // Check if there is valid file name
    if (!isalnum(sPathname[sPathname.find_last_of(".") - 1]))
        return false;

    return true;
}

