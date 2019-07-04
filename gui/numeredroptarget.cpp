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
#include "terminal/wxssh.h"
#include "compositions/filetree.hpp"
#include "DirTraverser.hpp"

#define wxUSE_DRAG_AND_DROP 1

#if wxUSE_DRAG_AND_DROP

string replacePathSeparator(const string&);

NumeReDropTarget::NumeReDropTarget(wxWindow* topwindow, wxWindow* owner, parentType type) : m_owner(owner), m_topWindow(topwindow), m_type(type)
{
    wxDataObjectComposite* dataobj = new wxDataObjectComposite();
    if (type == EDITOR || type == CONSOLE)
    {
        dataobj->Add(new wxTextDataObject(), true);
        dataobj->Add(new wxFileDataObject());
    }
    else
        dataobj->Add(new wxFileDataObject(), true);
    SetDataObject(dataobj);
}

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
        if (format.GetType() == wxDF_TEXT || format.GetType() == wxDF_UNICODETEXT)
            return wxDragNone;
        FileTree* tree = static_cast<FileTree*>(m_owner);
        wxTreeItemId currentId = tree->HitTest(wxPoint(x,y));
        if (currentId.IsOk())
            tree->SetDnDHighlight(currentId);
    }
    return defaultDragResult;
}

wxDragResult NumeReDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult defaultDragResult)
{
    this->GetData();
    wxDataObjectComposite* dataobj = static_cast<wxDataObjectComposite*>(GetDataObject());
    wxDataFormat format = dataobj->GetReceivedFormat();
    wxDataObject* data = dataobj->GetObject(format);
    switch (format.GetType())
    {
        case wxDF_FILENAME:
        {
            wxFileDataObject* filedata = static_cast<wxFileDataObject*>(data);
            wxArrayString filenames = filedata->GetFilenames();

            NumeReWindow* top = static_cast<NumeReWindow*>(m_topWindow);
            vector<string> vPaths = top->getPathDefs();
            if (m_type == EDITOR)
            {
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
            }
            else if (m_type == CONSOLE)
            {
                string sExecutables;
                string sLoadables;
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
                    if (getFileType(filenames[i]) == EXECUTABLE)
                    {
                        if (sExecutables.length())
                            sExecutables += ";";
                        if (filenames[i].find(".nscr") != string::npos)
                        {
                            string sScriptName = replacePathSeparator(filenames[i].ToStdString());
                            sScriptName.erase(sScriptName.rfind(".nscr"));
                            if (sScriptName.substr(0, vPaths[SCRIPTPATH].length()) == vPaths[SCRIPTPATH])
                                sScriptName.erase(0, vPaths[SCRIPTPATH].length());
                            while (sScriptName.front() == '/')
                                sScriptName.erase(0,1);
                            if (sScriptName.find(' ') != string::npos)
                                sScriptName = "\"" + sScriptName + "\"";
                            sExecutables += "start " + sScriptName;
                        }
                        else
                        {
                            string sProcName = replacePathSeparator(filenames[i].ToStdString());
                            sProcName.erase(sProcName.rfind(".nprc"));
                            if (sProcName.substr(0, vPaths[PROCPATH].length()) == vPaths[PROCPATH])
                            {
                                sProcName.erase(0, vPaths[PROCPATH].length());
                                while (sProcName.front() == '/')
                                    sProcName.erase(0,1);
                                while (sProcName.find('/') != string::npos)
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
                        sLoadables += "load \"" + replacePathSeparator(filenames[i].ToStdString()) + "\" -app -ignore";
                    }
                }
                // return error, if nothing remained
                if (!filenames.size())
                    return wxDragNone;

                if (sLoadables.length())
                    top->getTerminal()->pass_command(sLoadables);
                if (sExecutables.length())
                    top->getTerminal()->pass_command(sExecutables);
            }
            else if (m_type == FILETREE)
            {
                FileTree* tree = static_cast<FileTree*>(m_owner);

                wxTreeItemId dragdropsource = top->getDragDropSourceItem();
                if (dragdropsource.IsOk() && (static_cast<FileNameTreeData*>(tree->GetItemData(dragdropsource)))->filename == filenames[0])
                {
                    wxTreeItemId dragdroptarget = tree->HitTest(wxPoint(x,y));
                    wxString pathname = top->getTreeFolderPath(dragdroptarget);

                    if (pathname.length())
                    {
                        wxString filename = pathname + "\\" + filenames[0].substr(filenames[0].rfind('\\')+1);
                        wxCopyFile(filenames[0], filename, true);
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
                    string sFileName = replacePathSeparator(filenames[i].ToStdString());
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
                        if (sFileName.find(".nscr") != string::npos)
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
            if (m_type == EDITOR)
            {
                wxTextDataObject* textdata = static_cast<wxTextDataObject*>(data);
                NumeReEditor* edit = static_cast<NumeReEditor*>(m_owner);
                edit->DoDropText(x, y, textdata->GetText());
            }
            else if (m_type == CONSOLE)
            {
                wxTextDataObject* textdata = static_cast<wxTextDataObject*>(data);
                string sText = textdata->GetText().ToStdString();
                while (sText.find('\n') != string::npos)
                    sText[sText.find('\n')] = ';';
                while (sText.find('\t') != string::npos)
                    sText[sText.find('\t')] = ' ';
                NumeReWindow* top = static_cast<NumeReWindow*>(m_topWindow);
                top->getTerminal()->ProcessInput(sText.length(), sText);
                defaultDragResult = wxDragCopy;
            }
            break;
        }
    }
    return defaultDragResult;
}

NumeReDropTarget::fileType NumeReDropTarget::getFileType(const wxString& filename)
{
    if (filename.find('.') == string::npos)
        return NOEXTENSION;
    static wxString textExtensions = ";txt;dat;log;tex;csv;jdx;jcm;dx;nhlp;ndb;nlng;def;ini;hlpidx;m;cpp;hpp;c;h;cxx;hxx";
    static wxString binaryExtensions = ";ndat;xls;xlsx;ods;labx;ibw;";
    static wxString imageExtensions = ";png;gif;jpg;jpeg;bmp;eps;svg;";
    static wxString execExtensions = ";nscr;nprc;";

    wxString extension = ";"+filename.substr(filename.rfind('.')+1)+";";
    if (textExtensions.find(extension) != string::npos)
        return TEXTFILE;
    if (binaryExtensions.find(extension) != string::npos)
        return BINARYFILE;
    if (imageExtensions.find(extension) != string::npos)
        return IMAGEFILE;
    if (execExtensions.find(extension) != string::npos)
        return EXECUTABLE;

    return NOTSUPPORTED;
}

#endif //wxUSE_DRAG_AND_DROP
