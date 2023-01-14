/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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



#include "DirTraverser.hpp"


/////////////////////////////////////////////////
/// \brief Constuctor.
///
/// \param therootNode wxTreeCtrl*
/// \param theiconmanager IconManager*
/// \param theid wxTreeItemId
/// \param thepath const wxString&
/// \param thefilespec FileFilterType
///
/////////////////////////////////////////////////
DirTraverser::DirTraverser(wxTreeCtrl* therootNode, IconManager* theiconmanager, wxTreeItemId theid, const wxString& thepath, FileFilterType thefilespec)
{
    rootNode = therootNode;
    iconManager = theiconmanager;
    id = theid;
    path = thepath;
    fileSpec = thefilespec;
    vcurrentnodes.push_back(id);
    ncurrentdepth = 0;

    // Calculate the current folder depth
    for (size_t i = 0; i < path.length(); i++)
    {
        if (path[i] == '\\' || path[i] == '/')
            ncurrentdepth++;
    }
}


/////////////////////////////////////////////////
/// \brief This method classifies the files found
/// during traversing the directory and appends
/// them to the tree, if they correspond to one
/// of the selected file filter types.
///
/// \param filename const wxString&
/// \return wxDirTraverseResult
///
/////////////////////////////////////////////////
wxDirTraverseResult DirTraverser::OnFile(const wxString& filename)
{
    if (filename.find('.') == std::string::npos || filename.find(".revisions") != std::string::npos)
        return wxDIR_CONTINUE;

    wxString filespec;
    wxString extension = filename.substr(filename.rfind('.')+1).Lower();

    // Determine, whether the current file matches to the
    // selected file filter types
    switch (fileSpec)
    {
        case FILE_NSCR:
            if (filename.length() < 6 || (extension != "nscr" && extension != "nlyt" && extension != "npkp" && extension != "nhlp"))
                return wxDIR_CONTINUE;

            break;
        case FILE_NPRC:
            if (filename.length() < 6 || (extension != "nprc" && extension != "nlyt" && extension != "npkp" && extension != "nhlp"))
                return wxDIR_CONTINUE;

            break;
        case FILE_NUMERE:
            if (filename.length() < 6)
                return wxDIR_CONTINUE;

            filespec = "*.nscr;*.nprc;*.nlyt;*.ndat;*.npkp;*.nhlp;";

            if (filespec.find("*."+extension+";") == std::string::npos)
                return wxDIR_CONTINUE;

            break;
        case FILE_DATAFILES:
            if (filename.length() < 4)
                return wxDIR_CONTINUE;

            filespec = "*.ndat;*.dat;*.xls;*.xlsx;*.ods;*.csv;*.txt;*.labx;*.ibw;*.jdx;*.jcm;*.dx;*.png;*.log;*.tex;*.pdf;*.m;*.cpp;*.cxx;*.c;*.hpp;*.hxx;*.h;*.xml;*.wav;*.diff;";

            if (filespec.find("*."+extension + ";") == std::string::npos)
                return wxDIR_CONTINUE;

            break;
        case FILE_IMAGEFILES:
            if (filename.length() < 5)
                return wxDIR_CONTINUE;

            filespec = "*.png;*.jpg;*.jpeg;*.eps;*.svg;*.gif;*.bmp;*.tif;*.tiff;";

            if (filespec.find("*."+extension+";") == std::string::npos)
                return wxDIR_CONTINUE;

            break;
        default:
            filespec  = "*.*";
    }

    unsigned int ndepth = 0;

    for (size_t i = 0; i < filename.length(); i++)
    {
        if (filename[i] == '/' || filename[i] == '\\')
            ndepth++;
    }

    while (ndepth-1 < ncurrentdepth)
    {
        vcurrentnodes.pop_back();
        ncurrentdepth--;
    }

    FileNameTreeData* data = new FileNameTreeData();
    data->filename = filename;

    rootNode->AppendItem(vcurrentnodes.back(), filename.substr(filename.rfind('\\')+1), iconManager->GetIconIndex(extension),-1, data);
    return wxDIR_CONTINUE;
}


/////////////////////////////////////////////////
/// \brief This method appends the folders found
/// during traversing the directory to the file
/// tree.
///
/// \param dirname const wxString&
/// \return wxDirTraverseResult
///
/////////////////////////////////////////////////
wxDirTraverseResult DirTraverser::OnDir(const wxString& dirname)
{
    if (dirname.find(".revisions") != std::string::npos)
        return wxDIR_IGNORE;

    unsigned int ndepth = 0;

    for (size_t i = 0; i < dirname.length(); i++)
    {
        if (dirname[i] == '/' || dirname[i] == '\\')
            ndepth++;
    }

    while (ndepth < ncurrentdepth)
    {
        vcurrentnodes.pop_back();
        ncurrentdepth--;
    }

    if (ndepth == ncurrentdepth)
        vcurrentnodes.pop_back();

    if (ndepth > ncurrentdepth)
        ncurrentdepth = ndepth;

    FileNameTreeData* data = new FileNameTreeData();
    data->filename = dirname;
    data->isDir = true;
    vcurrentnodes.push_back(rootNode->AppendItem(vcurrentnodes.back(), dirname.substr(dirname.rfind('\\')+1), iconManager->GetIconIndex("FOLDEROPEN"), -1, data));

    return wxDIR_CONTINUE;
}



