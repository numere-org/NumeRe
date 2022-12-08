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


#include "numeredroptarget.hpp"
#include "NumeReWindow.h"
#include "editor/editor.h"
#include "terminal/terminal.hpp"
#include "compositions/filetree.hpp"
#include "DirTraverser.hpp"
#include "../common/vcsmanager.hpp"
#include "../common/filerevisions.hpp"

#define wxUSE_DRAG_AND_DROP 1

#if wxUSE_DRAG_AND_DROP

std::string replacePathSeparator(const std::string&);

/////////////////////////////////////////////////
/// \brief Constructor. Initializes the data objects, which correspond to the assigned wxWindow
///
/// \param topwindow wxWindow*
/// \param owner wxWindow*
/// \param type parentType
///
/////////////////////////////////////////////////
NumeReDropTarget::NumeReDropTarget(wxWindow* topwindow, wxWindow* owner, parentType type) : m_owner(owner), m_topWindow(topwindow), m_type(type)
{
    wxDataObjectComposite* dataobj = new wxDataObjectComposite();

    if (type == EDITOR || type == CONSOLE)
    {
        dataobj->Add(new wxTextDataObject(), true);
        dataobj->Add(new wxFileDataObject());
    }
    else // type == FILETREE
        dataobj->Add(new wxFileDataObject(), true);

    SetDataObject(dataobj);
}

/////////////////////////////////////////////////
/// \brief This method visualizes the DragDrop effect.
///
/// \param x wxCoord
/// \param y wxCoord
/// \param defaultDragResult wxDragResult
/// \return wxDragResult
///
/// It calls the corresponding UI functions, which visualize
/// the cursor position in the editor or the file tree. This function
/// is called from the DragDrop handler automatically.
/////////////////////////////////////////////////
wxDragResult NumeReDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult defaultDragResult)
{
    if (m_type == EDITOR)
    {
        NumeReEditor* edit = static_cast<NumeReEditor*>(m_owner);
        defaultDragResult = edit->DoDragOver(x, y, defaultDragResult);
    }
    else if (m_type == FILETREE)
    {
        this->GetData();
        wxDataObjectComposite* dataobj = static_cast<wxDataObjectComposite*>(GetDataObject());
        wxDataFormat format = dataobj->GetReceivedFormat();

        // The file tree won't accept pure text data
        if (format.GetType() == wxDF_TEXT || format.GetType() == wxDF_UNICODETEXT)
            return wxDragNone;

        // Find the tree item currently below the cursor
        FileTree* tree = static_cast<FileTree*>(m_owner);
        wxTreeItemId currentId = tree->HitTest(wxPoint(x,y));

        // Highlight the item below the cursor
        if (currentId.IsOk())
            tree->SetDnDHighlight(currentId);
    }

    return defaultDragResult;
}

/////////////////////////////////////////////////
/// \brief This method handles the file operations after DragDrop
///
/// \param x wxCoord
/// \param y wxCoord
/// \param defaultDragResult wxDragResult
/// \return wxDragResult
///
/////////////////////////////////////////////////
wxDragResult NumeReDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult defaultDragResult)
{
    this->GetData();
    wxDataObjectComposite* dataobj = static_cast<wxDataObjectComposite*>(GetDataObject());
    wxDataFormat format = dataobj->GetReceivedFormat();
    wxDataObject* data = dataobj->GetObject(format);

    // Switch for the dropped file type
    switch (format.GetType())
    {
        case wxDF_FILENAME:
        {
            // a plain file
            wxFileDataObject* filedata = static_cast<wxFileDataObject*>(data);
            wxArrayString filenames = filedata->GetFilenames();

            NumeReWindow* top = static_cast<NumeReWindow*>(m_topWindow);
            std::vector<std::string> vPaths = top->getPathDefs();

            // Select the current window type
            if (m_type == EDITOR)
            {
                // Files are opened in the editor, if
                // it supports this type of file.
                // clear out the passed filenames
                for (size_t i = 0; i < filenames.size(); i++)
                {
                    while (filenames.size()
                        && (getFileType(filenames[i]) == NOTSUPPORTED
                            || getFileType(filenames[i]) == IMAGEFILE
                            || getFileType(filenames[i]) == BINARYFILE)
                           )
                    {
                        filenames.erase(filenames.begin()+i);
                    }
                }

                // return error, if nothing remained
                if (!filenames.size())
                    return wxDragNone;

                top->OpenSourceFile(filenames);
                top->GetCurrentEditor()->SetFocus();
            }
            else if (m_type == CONSOLE)
            {
                // Files are either executed or loaded to
                // memory, if dragged onto the console
                std::string sExecutables;
                std::string sLoadables;

                // clear out the passed filenames
                for (size_t i = 0; i < filenames.size(); i++)
                {
                    while (filenames.size()
                        && (getFileType(filenames[i]) == IMAGEFILE
                            || getFileType(filenames[i]) == NOTSUPPORTED))
                    {
                        filenames.erase(filenames.begin()+i);
                    }

                    if (!filenames.size())
                        break;

                    // Is the current file a executable file (a script or
                    // a procedure)? Then we'll append it to the list of
                    // executables, which will be executed afterwards
                    if (getFileType(filenames[i]) == EXECUTABLE)
                    {
                        if (sExecutables.length())
                            sExecutables += ";";

                        // Is the current file a script or a procedure?
                        if (filenames[i].find(".nscr") != std::string::npos)
                        {
                            std::string sScriptName = replacePathSeparator(filenames[i].ToStdString());
                            sScriptName.erase(sScriptName.rfind(".nscr"));

                            if (sScriptName.substr(0, vPaths[SCRIPTPATH].length()) == vPaths[SCRIPTPATH])
                                sScriptName.erase(0, vPaths[SCRIPTPATH].length());

                            while (sScriptName.front() == '/')
                                sScriptName.erase(0,1);

                            if (sScriptName.find(' ') != std::string::npos)
                                sScriptName = "\"" + sScriptName + "\"";

                            sExecutables += "start " + sScriptName;
                        }
                        else
                        {
                            std::string sProcName = replacePathSeparator(filenames[i].ToStdString());
                            sProcName.erase(sProcName.rfind(".nprc"));

                            if (sProcName.substr(0, vPaths[PROCPATH].length()) == vPaths[PROCPATH])
                            {
                                sProcName.erase(0, vPaths[PROCPATH].length());

                                while (sProcName.front() == '/')
                                    sProcName.erase(0,1);

                                while (sProcName.find('/') != std::string::npos)
                                    sProcName[sProcName.find('/')] = '~';
                            }
                            else
                                sProcName = "'" + sProcName + "'";

                            sExecutables += "$" + sProcName + "()";
                        }
                    }
                    else
                    {
                        if (sLoadables.length())
                            sLoadables += ";";

                        // This is another file type. We'll add it to the list
                        // of loadable files and try to load it to memory afterwards
                        sLoadables += "load \"" + replacePathSeparator(filenames[i].ToStdString()) + "\" -app -ignore";
                    }
                }

                // return error, if nothing remained
                if (!filenames.size())
                    return wxDragNone;

                // Load the files here
                if (sLoadables.length())
                    top->getTerminal()->pass_command(sLoadables, false);

                // Execute the executable files here
                if (sExecutables.length())
                    top->getTerminal()->pass_command(sExecutables, false);

                top->getTerminal()->SetFocus();
            }
            else if (m_type == FILETREE)
            {
                // Files are added to the corresponding
                // standard paths, if they are dragged
                // onto the file tree
                FileTree* tree = static_cast<FileTree*>(m_owner);
                VersionControlSystemManager manager(top);
                wxTreeItemId dragdropsource = top->getDragDropSourceItem();

                // Internal drag-drop
                if (dragdropsource.IsOk() && (static_cast<FileNameTreeData*>(tree->GetItemData(dragdropsource)))->filename == filenames[0])
                {
                    wxTreeItemId dragdroptarget = tree->HitTest(wxPoint(x,y));
                    wxString pathname = top->getTreeFolderPath(dragdroptarget);

                    if (pathname.length())
                    {
                        wxString filename = pathname + "\\" + filenames[0].substr(filenames[0].rfind('\\')+1);

                        // Do not drag files on themselves
                        if (wxFileName(filename) == wxFileName(filenames[0]))
                        {
                            tree->SetDnDHighlight(wxTreeItemId());
                            return wxDragNone;
                        }

                        // If the current file already has revisions,
                        // add this file operation to the revision and
                        // mirror the operation for the revision file
                        if (manager.hasRevisions(filenames[0]))
                        {
                            std::unique_ptr<FileRevisions> revisions(manager.getRevisions(filenames[0]));

                            if (revisions.get())
                            {
                                revisions->moveFile(filenames[0], filename, manager.getRevisionPath(filename));
                            }
                        }

                        top->UpdateLocationIfOpen(filenames[0], filename);

                        if (wxCopyFile(filenames[0], filename, true))
                            wxRemoveFile(filenames[0]);

                        tree->SetDnDHighlight(wxTreeItemId());
                        return defaultDragResult;
                    }

                    tree->SetDnDHighlight(wxTreeItemId());
                    return wxDragNone;
                }

                // check, if file already exists in the location
                for (size_t i = 0; i < filenames.size(); i++)
                {
                    while (filenames.size()
                        && getFileType(filenames[i]) == NOTSUPPORTED)
                    {
                        filenames.erase(filenames.begin()+i);
                    }

                    if (!filenames.size())
                        return wxDragNone;

                    fileType type = getFileType(filenames[i]);
                    std::string sFileName = replacePathSeparator(filenames[i].ToStdString());

                    if (type == TEXTFILE || type == BINARYFILE)
                    {
                        // load or savepath
                        // already exists
                        if (sFileName.substr(vPaths[LOADPATH].length()) == vPaths[LOADPATH]
                            || sFileName.substr(vPaths[SAVEPATH].length()) == vPaths[SAVEPATH])
                            continue;

                        if (wxFileExists(vPaths[LOADPATH] + sFileName.substr(sFileName.rfind('/')))
                            || wxFileExists(vPaths[SAVEPATH] + sFileName.substr(sFileName.rfind('/'))))
                            continue;

                        wxCopyFile(sFileName, vPaths[LOADPATH] + sFileName.substr(sFileName.rfind('/')));
                    }
                    else if (type == EXECUTABLE)
                    {
                        // script or procpath
                        // already exists
                        if (sFileName.substr(vPaths[SCRIPTPATH].length()) == vPaths[SCRIPTPATH]
                            || sFileName.substr(vPaths[PROCPATH].length()) == vPaths[PROCPATH])
                            continue;

                        PathID pathID;

                        if (sFileName.find(".nscr") != std::string::npos)
                            pathID = SCRIPTPATH;
                        else
                            pathID = PROCPATH;

                        if (wxFileExists(vPaths[pathID] + sFileName.substr(sFileName.rfind('/'))))
                            continue;

                        wxCopyFile(sFileName, vPaths[pathID] + sFileName.substr(sFileName.rfind('/')));
                    }
                    else if (type == IMAGEFILE)
                    {
                        // script or procpath
                        // already exists
                        if (sFileName.substr(vPaths[PLOTPATH].length()) == vPaths[PLOTPATH])
                            continue;

                        PathID pathID = PLOTPATH;

                        if (wxFileExists(vPaths[pathID] + sFileName.substr(sFileName.rfind('/'))))
                            continue;

                        wxCopyFile(sFileName, vPaths[pathID] + sFileName.substr(sFileName.rfind('/')));
                    }
                }

                tree->SetDnDHighlight(wxTreeItemId());
                // if no: copy the file to the fitting location
            }

            break;
        }
        case wxDF_TEXT:
        case wxDF_UNICODETEXT:
        {
            // a selection of text
            if (m_type == EDITOR)
            {
                // Text is copied in the editor at the
                // selected position
                wxTextDataObject* textdata = static_cast<wxTextDataObject*>(data);
                NumeReEditor* edit = static_cast<NumeReEditor*>(m_owner);
                edit->DoDropText(x, y, textdata->GetText());
                edit->SetFocus();
            }
            else if (m_type == CONSOLE)
            {
                // Text is copied into the console at the
                // current position (not the selected one)
                wxTextDataObject* textdata = static_cast<wxTextDataObject*>(data);
                std::string sText = textdata->GetText().ToStdString();

                size_t nQuotes = 0;

                // Replace line comments with block comments, where necessary
                for (size_t i = 0; i < sText.length(); i++)
                {
                    if (sText[i] == '"' && (!i || sText[i-1] != '\\'))
                        nQuotes++;

                    if (nQuotes % 2)
                        continue;

                    if (sText.substr(i, 2) == "##")
                    {
                        size_t p = sText.find('\n', i);

                        // The last one does not need any replacement
                        if (p == std::string::npos)
                            break;

                        sText[i+1] = '*';
                        sText.insert(p, "*#");
                        i = p+2;
                    }
                }

                size_t pos, pos2;

                // Replace line continuation markers including their trailing
                // line breaks with a single whitespace
                while ((pos = sText.find("\\\\")) != std::string::npos
                       && (pos2 = sText.find_first_not_of(" \t", pos+2)) != std::string::npos
                       && sText[pos2] == '\n')
                {
                    sText.replace(pos, pos2-pos+1, " ");
                }

                // Replace line break and tabulator characters,
                // because they won't be parsed correctly
                while ((pos = sText.find('\n')) != std::string::npos)
                {
                    if (pos
                        && sText.find_last_not_of(" \t", pos-1) != std::string::npos
                        && sText[sText.find_last_not_of(" \t", pos-1)] != ';')
                        sText[pos] = ';';
                    else
                        sText[pos] = ' ';
                }

                while ((pos = sText.find('\t')) != std::string::npos)
                    sText[pos] = ' ';

                NumeReWindow* top = static_cast<NumeReWindow*>(m_topWindow);
                top->getTerminal()->ProcessInput(sText.length(), sText);
                top->getTerminal()->SetFocus();
                defaultDragResult = wxDragCopy;
            }

            break;
        }
    }

    return defaultDragResult;
}

/////////////////////////////////////////////////
/// \brief This method classifies the file type of the passed file type.
///
/// \param filename const wxString&
/// \return NumeReDropTarget::fileType
///
/////////////////////////////////////////////////
NumeReDropTarget::fileType NumeReDropTarget::getFileType(const wxString& filename)
{
    if (filename.find('.') == std::string::npos)
        return NOEXTENSION;

    // Declare the categories containing the
    // corresponding file extensions
    static wxString textExtensions = ";txt;dat;log;tex;csv;jdx;jcm;dx;nhlp;nlyt;ndb;nlng;def;ini;hlpidx;m;cpp;hpp;c;h;cxx;hxx;xml;diff;";
    static wxString binaryExtensions = ";ndat;xls;xlsx;ods;labx;ibw;";
    static wxString imageExtensions = ";png;gif;jpg;jpeg;bmp;eps;svg;";
    static wxString execExtensions = ";nscr;nprc;";

    wxString extension = ";"+filename.substr(filename.rfind('.')+1)+";";

    // Identify the type of the file by its extensions
    if (textExtensions.find(extension) != std::string::npos)
        return TEXTFILE;

    if (binaryExtensions.find(extension) != std::string::npos)
        return BINARYFILE;

    if (imageExtensions.find(extension) != std::string::npos)
        return IMAGEFILE;

    if (execExtensions.find(extension) != std::string::npos)
        return EXECUTABLE;

    return NOTSUPPORTED;
}

#endif //wxUSE_DRAG_AND_DROP

