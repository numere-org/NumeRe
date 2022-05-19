#include <wx/filename.h>


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
    wxFileName sWxPathname(sPathname);
    if (!sWxPathname.IsOk())
        return false;

    return sWxPathname.DirExists();
}


/////////////////////////////////////////////////
/// \brief This function checks whether
///
/// \param sPathname std::string
/// \return bool
///
/////////////////////////////////////////////////
bool is_file(std::string sPathname)
{
    wxFileName sWxFilename(sPathname);
    if (!sWxFilename.IsOk())
        return false;

    return sWxFilename.FileExists();
}

