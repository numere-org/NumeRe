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

#include "packagedialog.hpp"
#include "../compositions/grouppanel.hpp"
#include "../globals.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/ui/winlayout.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include "../../externals/tinyxml2/tinyxml2.h"
#include "../../kernel/core/procedure/dependency.hpp"
#include "../../kernel/core/procedure/includer.hpp"
#include "../../kernel/core/io/styledtextfile.hpp"
#include "../../common/vcsmanager.hpp"
#include "../../common/filerevisions.hpp"
#include "../../common/datastructures.h"

#include <set>
#include <memory>

#define INCLUDEDOCS "_doctemplate"
#define DOCFILE "_docfile"

extern Language _guilang;


BEGIN_EVENT_TABLE(PackageDialog, wxDialog)
    EVT_BUTTON(ID_PKGDLG_ADD, PackageDialog::OnAddItems)
    EVT_BUTTON(ID_PKGDLG_REMOVE, PackageDialog::OnRemoveItems)
    EVT_BUTTON(ID_PKGDLG_AUTODETECT, PackageDialog::OnAutoDetect)
    EVT_BUTTON(ID_PKGDLG_LOADPROJECT, PackageDialog::OnLoadProjectFile)
    EVT_BUTTON(ID_PKGDLG_SAVEPROJECT, PackageDialog::OnSaveProjectFile)
    EVT_BUTTON(ID_PKGDLG_CREATEPACKAGE, PackageDialog::OnCreatePackage)
    EVT_BUTTON(ID_PKGDLG_ABORT, PackageDialog::OnAbort)
    EVT_PG_CHANGED(-1, PackageDialog::OnPropGridChange)
    EVT_CLOSE(PackageDialog::OnClose)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Constructor.
///
/// \param parent wxWindow*
/// \param terminal NumeReTerminal*
/// \param icons IconManager*
///
/////////////////////////////////////////////////
PackageDialog::PackageDialog(wxWindow* parent, NumeReTerminal* terminal, IconManager* icons) : wxDialog(parent, wxID_ANY, _guilang.get("GUI_PKGDLG_HEAD") + " [New Project]", wxDefaultPosition, wxSize(600*g_pixelScale, 800*g_pixelScale), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_packageProperties = nullptr;
    m_fileList = nullptr;
    m_terminal = terminal;
    m_icons = icons;
    m_isAutoIncrement = false;

    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

    // Create new grouped panel for this dialog
    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);

    // Create the properties group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_PKGDLG_PROPERTIES"));

    // Create a property grid containing the package
    // properties
    m_packageProperties = new wxPropertyGrid(group->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize(-1, 260*g_pixelScale), wxPG_THEME_BORDER | wxPG_TOOLTIPS | wxPG_SPLITTER_AUTO_CENTER);
    m_packageProperties->Append(new wxPropertyCategory(_guilang.get("GUI_PKGDLG_GENERAL_CATEGORY")));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_PACKAGENAME"), "-name"));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_AUTHOR"), "-author"));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_VERSION"), "-version", "<AUTO>"));

    wxArrayString flags;
    flags.Add("ENABLE_DEFAULTS");
    flags.Add("ENABLE_FULL_LOGGING");
    flags.Add("DISABLE_SCREEN_OUTPUT");
    m_packageProperties->Append(new wxEnumProperty(_guilang.get("GUI_PKGDLG_FLAGS"), "-flags", flags));

    wxArrayString type;
    type.Add("TYPE_PACKAGE");
    type.Add("TYPE_PLUGIN");
    type.Add("TYPE_PLUGIN_WITH_RETURN_VALUE");
    type.Add("TYPE_GUI_PLUGIN");
    m_packageProperties->Append(new wxEnumProperty(_guilang.get("GUI_PKGDLG_TYPE"), "-type", type));

    // Add a license field
    wxArrayString license;
    license.Add("Apache-2.0");
    license.Add("BSD-2-Clause");
    license.Add("BSD-3-Clause");
    license.Add("CC-BY-4.0");
    license.Add("CC-BY-ND-4.0");
    license.Add("CC-BY-SA-4.0");
    license.Add("CC-BY-SA-ND-4.0");
    license.Add("EPL-2.0");
    license.Add("GPL-2.0-only");
    license.Add("GPL-2.0-or-later");
    license.Add("GPL-3.0-only");
    license.Add("GPL-3.0-or-later");
    license.Add("LGPL-2.0-only");
    license.Add("LGPL-2.0-or-later");
    license.Add("LGPL-3.0-only");
    license.Add("LGPL-3.0-or-later");
    license.Add("MIT");
    license.Add("MPL-2.0");
    m_packageProperties->Append(new wxEditEnumProperty(_guilang.get("GUI_PKGDLG_LICENSE"), "-license", license, wxArrayInt(), license[0]));
    m_packageProperties->Append(new wxLongStringProperty(_guilang.get("GUI_PKGDLG_PLUGINDESC"), "-desc"));
    m_packageProperties->Append(new wxLongStringProperty(_guilang.get("GUI_PKGDLG_KEYWORDS"), "-keywords"));
    m_packageProperties->Append(new wxLongStringProperty(_guilang.get("GUI_PKGDLG_CHANGESLOG"), "-changelog"));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_DEPENDENCIES"), "-requirepackages"));

    // Add a validator to the package command to enssure that the user
    // only uses alphanumeric characters as command string
    wxPGProperty* pluginsCategory = m_packageProperties->Append(new wxPropertyCategory(_guilang.get("GUI_PKGDLG_PLUGIN_CATEGORY")));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_PLUGINCOMMAND"), "-plugincommand"));
    m_packageProperties->SetPropertyValidator("-plugincommand", wxTextValidator(wxFILTER_ALPHANUMERIC));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_PLUGINMENUENTRY"), "-pluginmenuentry"));
    m_packageProperties->SetPropertyValidator("-pluginmenuentry", wxTextValidator(wxFILTER_ASCII));

    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_PLUGINMAIN"), "-pluginmain"));

    wxPGProperty* docsCategory = m_packageProperties->Append(new wxPropertyCategory(_guilang.get("GUI_PKGDLG_DOCS_CATEGORY")));
    m_packageProperties->Append(new wxBoolProperty(_guilang.get("GUI_PKGDLG_INCLUDEDOCUMENTATION"), INCLUDEDOCS));
    wxFileProperty* docfile = new wxFileProperty(_guilang.get("GUI_PKGDLG_DOCUMENTATION"), DOCFILE);
    docfile->SetAttribute(wxPG_FILE_DIALOG_STYLE, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    docfile->SetAttribute(wxPG_FILE_WILDCARD, "Doc files (*.xml,*.nhlp)|*.xml;*.nhlp|Text files (*.txt)|*.txt|All files (*.*)|*.*");
    docfile->SetAttribute(wxPG_FILE_DIALOG_TITLE, _guilang.get("GUI_DLG_OPEN"));
    m_packageProperties->Append(docfile);

    // Apply some general settings to the whole property grid
    m_packageProperties->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
    m_packageProperties->SetValidationFailureBehavior(wxPG_VFB_MARK_CELL | wxPG_VFB_STAY_IN_PROPERTY);

    // Fold the non-general categories
    m_packageProperties->Collapse(wxPGPropArg(pluginsCategory));
    m_packageProperties->Collapse(wxPGPropArg(docsCategory));

    group->Add(m_packageProperties, 1, wxEXPAND | wxALL, 5);

    // Create the files group
    group = panel->createGroup(_guilang.get("GUI_PKGDLG_FILES"));

    // Add a special horizontal sizer for the buttons
    wxBoxSizer* buttonSizer = panel->createGroup(wxHORIZONTAL, group);

    // Create the add, remove and autodetect buttons
    panel->CreateButton(group->GetStaticBox(), buttonSizer, _guilang.get("GUI_PKGDLG_ADDFILES"), ID_PKGDLG_ADD);
    panel->CreateButton(group->GetStaticBox(), buttonSizer, _guilang.get("GUI_PKGDLG_REMOVEFILES"), ID_PKGDLG_REMOVE);
    panel->CreateButton(group->GetStaticBox(), buttonSizer, _guilang.get("GUI_PKGDLG_AUTODETECTFILES"), ID_PKGDLG_AUTODETECT);

    // Create a list view for the files and add the image list
    // to display the file icons
    m_fileList = panel->CreateListView(group->GetStaticBox(), group, wxLC_LIST | wxLC_ALIGN_LEFT, wxSize(-1, 330*g_pixelScale));
    m_fileList->SetImageList(icons->GetImageList(), wxIMAGE_LIST_SMALL);

    // Create a special horizontal sizer for the project buttons
    wxBoxSizer* projectButtonSizer = panel->createGroup(wxHORIZONTAL);

    // Create the project buttons
    panel->CreateButton(panel, projectButtonSizer, _guilang.get("GUI_PKGDLG_SAVE_PROJECT"), ID_PKGDLG_SAVEPROJECT);
    panel->CreateButton(panel, projectButtonSizer, _guilang.get("GUI_PKGDLG_LOAD_PROJECT"), ID_PKGDLG_LOADPROJECT);
    panel->CreateButton(panel, projectButtonSizer, _guilang.get("GUI_PKGDLG_CREATE_PACKAGE"), ID_PKGDLG_CREATEPACKAGE);
    panel->CreateButton(panel, projectButtonSizer, _guilang.get("GUI_BUTTON_CANCEL"), ID_PKGDLG_ABORT);

    // Activate scroll bars
    panel->SetScrollbars(0, 20, 0, 200);

    // Add the panel to the vertical sizer of the dialog
    vsizer->Add(panel, 1, wxEXPAND, 0);

    SetSizer(vsizer);
}


/////////////////////////////////////////////////
/// \brief This is the event handler for the
/// autodetect button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnAutoDetect(wxCommandEvent& event)
{
    try
    {
        autoDetect(getProcedures());
    }
    catch (...)
    {
        EndModal(wxID_CANCEL);
        throw;
    }
}


/////////////////////////////////////////////////
/// \brief This is the event handler for the add
/// files button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnAddItems(wxCommandEvent& event)
{
    // Create a file dialog focused on procedure files. The user may
    // select multiple files
    wxFileDialog dlg(this, _guilang.get("GUI_PKGDLG_SELECT_FILES"), m_terminal->getPathSettings()[PROCPATH], wxEmptyString,
                     _guilang.get("GUI_FILTER_INSTALLABLES") + " (*.nprc, *.nlyt)|*.nprc;*.nlyt|"
                     + _guilang.get("COMMON_FILETYPE_NPRC") + " (*.nprc)|*.nprc|"
                     + _guilang.get("COMMON_FILETYPE_NLYT") + " (*.nlyt)|*.nlyt|"
                     + _guilang.get("COMMON_FILETYPE_NSCR") + " (*.nscr)|*.nscr",
                     wxFD_MULTIPLE);

    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString files;
        dlg.GetPaths(files);

        // Insert the selected items, if they're not already part
        // of the files set
        for (size_t i = 0; i < files.size(); i++)
        {
            if (m_fileList->FindItem(-1, replacePathSeparator(files[i].ToStdString()), false) == -1)
            {
                wxString sExt = files[i].substr(files[i].rfind('.'));
                m_fileList->InsertItem(m_fileList->GetItemCount(), replacePathSeparator(files[i].ToStdString()), m_icons->GetIconIndex(sExt));
            }
        }

        markUnsaved();
    }
}


/////////////////////////////////////////////////
/// \brief This is the event handler for the
/// remove files button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnRemoveItems(wxCommandEvent& event)
{
    long selected = m_fileList->GetFirstSelected();

    // Remove all selected items
    while (m_fileList->GetItemCount() && m_fileList->GetSelectedItemCount() && selected != -1)
    {
        m_fileList->DeleteItem(selected);
        selected = m_fileList->GetFirstSelected();
    }

    markUnsaved();
}


/////////////////////////////////////////////////
/// \brief Button event handler to load an
/// existing project file.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnLoadProjectFile(wxCommandEvent& event)
{
    if (!isSaved())
    {
        int res = wxMessageBox(_guilang.get("GUI_UNSAVEDFILE_CLOSE"), _guilang.get("GUI_SAVE_QUESTION"), wxYES_NO);

        if (res == wxYES)
        {
            wxFileDialog dialog(this, _guilang.get("GUI_DLG_SAVE"), m_terminal->getPathSettings()[SCRIPTPATH] + "/packages", getPackageIdentifier() + ".npkp", _guilang.get("GUI_FILTER_NPKP") + " (*.npkp)|*.npkp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

            if (dialog.ShowModal() == wxID_OK)
                saveProjectFile(dialog.GetPath());
        }
    }

    wxFileDialog dialog(this, _guilang.get("GUI_DLG_OPEN"), m_terminal->getPathSettings()[SCRIPTPATH] + "/packages", wxEmptyString, _guilang.get("GUI_FILTER_NPKP") + " (*.npkp)|*.npkp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() == wxID_OK)
        loadProjectFile(dialog.GetPath());
}


/////////////////////////////////////////////////
/// \brief Button event handler to save a
/// prepared package setting to a project file.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnSaveProjectFile(wxCommandEvent& event)
{
    wxFileDialog dialog(this, _guilang.get("GUI_DLG_SAVE"), m_terminal->getPathSettings()[SCRIPTPATH] + "/packages", getPackageIdentifier() + ".npkp", _guilang.get("GUI_FILTER_NPKP") + " (*.npkp)|*.npkp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() == wxID_OK)
        saveProjectFile(dialog.GetPath());
}


/////////////////////////////////////////////////
/// \brief Called upon clicking on "Create
/// package" button. Will ask for saving the
/// project.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnCreatePackage(wxCommandEvent& event)
{
    SaveOnClose();
    EndModal(wxID_OK);
}


/////////////////////////////////////////////////
/// \brief Called upon clicking on "Cancel"
/// button. Will ask for saving the project.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnAbort(wxCommandEvent& event)
{
    SaveOnClose();
    EndModal(wxID_CANCEL);
}


/////////////////////////////////////////////////
/// \brief Event handler for all property grid
/// changes.
///
/// \param event wxPropertyGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnPropGridChange(wxPropertyGridEvent& event)
{
    if (isSaved())
        SetTitle(GetTitle().insert(GetTitle().find("[")+1, "*"));

    event.GetProperty()->SetTextColour(*wxRED);
}


/////////////////////////////////////////////////
/// \brief Event handler, which is called, when
/// the user closes the dialog.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::OnClose(wxCloseEvent& event)
{
    SaveOnClose();
    EndModal(wxCANCEL);
}


/////////////////////////////////////////////////
/// \brief Evaluates, whether the current project
/// has been saved and prompt the user a saving
/// dialog, if not.
///
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::SaveOnClose()
{
    if (!isSaved())
    {
        int res = wxMessageBox(_guilang.get("GUI_UNSAVEDFILE_CLOSE"), _guilang.get("GUI_SAVE_QUESTION"), wxYES_NO);

        if (res == wxYES)
        {
            wxFileDialog dialog(this, _guilang.get("GUI_DLG_SAVE"), m_terminal->getPathSettings()[SCRIPTPATH] + "/packages", getPackageIdentifier() + ".npkp", _guilang.get("GUI_FILTER_NPKP") + " (*.npkp)|*.npkp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

            if (dialog.ShowModal() == wxID_OK)
                saveProjectFile(dialog.GetPath());
        }
    }
}


/////////////////////////////////////////////////
/// \brief Static function to resolve the
/// includes in the passed set and add them to
/// the set afterwards.
///
/// \param fileSet std::set<std::string>&
/// \return void
///
/////////////////////////////////////////////////
static void resolveIncludes(std::set<std::string>& fileSet)
{
    std::vector<std::string> vIncludes;

    for (auto iter = fileSet.begin(); iter != fileSet.end(); ++iter)
    {
        StyledTextFile depFile(*iter);
        std::string sSearchPath = depFile.getFileName();
        sSearchPath.erase(sSearchPath.find_last_of("/\\"));

        for (int i = 0; i < depFile.getLinesCount(); i++)
        {
            if (Includer::is_including_syntax(depFile.getStrippedLine(i)))
            {
                Includer incl(depFile.getStrippedLine(i), sSearchPath);
                vIncludes.push_back(incl.getIncludedFileName());
            }
        }
    }

    for (const auto& incl : vIncludes)
        fileSet.insert(incl);
}


/////////////////////////////////////////////////
/// \brief This function autodetects the
/// dependencies of the passed files by calling
/// the PackageDialog::followBranch() function
/// recursively.
///
/// \param mainfiles const wxArrayString&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::autoDetect(const wxArrayString& mainfiles)
{
    // Create a set (forces unique names)
    std::set<std::string> fileSet;

    // Call followBranch() for every file name in the array
    // of strings (except of layout files)
    for (size_t i = 0; i < mainfiles.size(); i++)
    {
        if (mainfiles[i].substr(0, 4) == "(!) ")
            continue;

        if (mainfiles[i].rfind(".nlyt") == std::string::npos)
            followBranch(replacePathSeparator(mainfiles[i].ToStdString()), fileSet);
        else
            findLayoutDependencies(replacePathSeparator(mainfiles[i].ToStdString()), fileSet);
    }

    resolveIncludes(fileSet);

    // Insert only unique file names into the list view
    for (auto iter = fileSet.begin(); iter != fileSet.end(); ++iter)
    {
        if (m_fileList->FindItem(-1, *iter, false) == -1)
        {
            std::string sExt = (*iter).substr((*iter).rfind('.'));
            m_fileList->InsertItem(m_fileList->GetItemCount(), *iter, m_icons->GetIconIndex(sExt));
        }
    }

    markUnsaved();
}


/////////////////////////////////////////////////
/// \brief This function will recursively call
/// itself to detect a whole branch of file
/// dependencies.
///
/// \param sFile const std::string&
/// \param fileSet std::set<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::followBranch(const std::string& sFile, std::set<std::string>& fileSet)
{
    // Add the current file to the set
    fileSet.insert(sFile);

    // Get a reference to the procedure library
    // and calculate the dependencies of the current file
    ProcedureLibrary& procLib = m_terminal->getKernel().getProcedureLibrary();
    Dependencies* dep = procLib.getProcedureContents(replacePathSeparator(sFile))->getDependencies();

    // If the returned dependencies are not already part of
    // the set, call this function recursively using the dependencies,
    // which are not part of the set yet
    for (auto iter = dep->getDependencyMap().begin(); iter != dep->getDependencyMap().end(); ++iter)
    {
        for (auto listiter = iter->second.begin(); listiter != iter->second.end(); ++listiter)
        {
            if (fileSet.find(listiter->getFileName()) == fileSet.end())
                followBranch(listiter->getFileName(), fileSet);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function uses the event
/// procedures of a window layout file to create
/// a dependency tree for this layout.
///
/// \param sFile const std::string&
/// \param fileSet std::set<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::findLayoutDependencies(const std::string& sFile, std::set<std::string>& fileSet)
{
    // Get a list of all event procedures
    std::vector<std::string> vProcs = getEventProcedures(sFile);

    // Get the tree for every not already examined
    // procedure
    for (size_t i = 0; i < vProcs.size(); i++)
    {
        if (fileSet.find(vProcs[i]) == fileSet.end())
            followBranch(vProcs[i], fileSet);
    }
}


/////////////////////////////////////////////////
/// \brief Loads a NumeRe package project file to
/// memory and updates the UI correspondingly.
///
/// \param filename const wxString&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::loadProjectFile(const wxString& filename)
{
    tinyxml2::XMLDocument project;
    project.LoadFile(filename.c_str());
    tinyxml2::XMLElement* root = project.FirstChildElement("project");
    bool allFilesFound = true;

    // Ensure that the file is readable and that the version
    // is known
    if (!root || !root->Attribute("version.npkp", "1"))
        return;

    tinyxml2::XMLElement* info = root->FirstChildElement("info");
    tinyxml2::XMLElement* files = root->FirstChildElement("files");

    if (!info || !files)
        return;

    m_isAutoIncrement = false;

    // Get an iterator for the properties grid and iterate
    // through it
    for (auto iter = m_packageProperties->GetIterator(); !iter.AtEnd(); iter++)
    {
        wxPGProperty* prop = *iter;
        prop->SetTextColour(*wxBLACK);
        tinyxml2::XMLElement* infoitem = info->FirstChildElement(prop->GetName().substr(1).c_str());

        if (!infoitem || !infoitem->GetText())
            continue;

        if (prop->GetName() == "-version")
        {
            if (infoitem->BoolAttribute("autoincrement"))
            {
                prop->SetValueFromString(incrementVersion(infoitem->GetText()));
                prop->SetTextColour(*wxRED);
                m_isAutoIncrement = true;
                continue;
            }
        }
        else if (prop->GetName() == DOCFILE && !fileExists(infoitem->GetText()))
        {
            allFilesFound = false;
            continue;
        }

        prop->SetValueFromString(infoitem->GetText());
    }

    m_fileList->DeleteAllItems();

    tinyxml2::XMLElement* file = files->FirstChildElement();

    // Load all files to the table
    while (file)
    {
        if (!fileExists(file->GetText()))
        {
            allFilesFound = false;

            m_fileList->InsertItem(m_fileList->GetItemCount(), "(!) " + std::string(file->GetText()), m_icons->GetIconIndex(""));
        }
        else
        {
            std::string sExt = file->GetText();
            sExt = sExt.substr(sExt.rfind('.'));

            m_fileList->InsertItem(m_fileList->GetItemCount(), file->GetText(), m_icons->GetIconIndex(sExt));
        }


        file = file->NextSiblingElement();
    }

    // Update the window title to reflect the loaded project
    wxFileName fname(filename);
    SetTitle(_guilang.get("GUI_PKGDLG_HEAD") + " [" + fname.GetFullName() + "]");

    if (m_isAutoIncrement)
        markUnsaved();

    if (!allFilesFound)
        wxMessageBox(_guilang.get("GUI_PKGDLG_NOTALLFILESFOUND"), _guilang.get("GUI_PKGDLG_NOTALLFILESFOUND_HEAD"), wxOK, this);
}


/////////////////////////////////////////////////
/// \brief Saves a prepared package configuration
/// as a NumeRe package project file to the HDD.
///
/// \param filename const wxString&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::saveProjectFile(const wxString& filename)
{
    VersionControlSystemManager manager(static_cast<NumeReWindow*>(GetParent()));
    std::unique_ptr<FileRevisions> revisions(manager.getRevisions(filename));

    // Ensure that the revisions are complete
    if (revisions.get())
    {
        if (!revisions->getRevisionCount() && wxFileExists(filename))
        {
            wxFile tempfile(filename);
            wxString contents;
            tempfile.ReadAll(&contents);
            revisions->addRevision(contents);
        }
    }

    tinyxml2::XMLDocument project;
    tinyxml2::XMLElement* root = project.NewElement("project");
    tinyxml2::XMLElement* info = project.NewElement("info");
    tinyxml2::XMLElement* files = project.NewElement("files");

    root->SetAttribute("version.npkp", "1");
    root->SetAttribute("version.numere", sVersion.c_str());

    project.InsertFirstChild(root);
    root->InsertFirstChild(info);
    root->InsertEndChild(files);

    // Get an iterator for the properties grid and iterate
    // through it
    for (auto iter = m_packageProperties->GetIterator(); !iter.AtEnd(); iter++)
    {
        wxPGProperty* prop = *iter;
        prop->SetTextColour(*wxBLACK);
        tinyxml2::XMLElement* infoitem = project.NewElement(prop->GetName().substr(1).c_str());

        if (prop->GetName() == "-version" && (prop->GetValueAsString() == "<AUTO>" || m_isAutoIncrement))
        {
            if (prop->GetValueAsString() == "<AUTO>")
                infoitem->SetText("0.0.1");
            else
                infoitem->SetText(prop->GetValueAsString().ToStdString().c_str());

            infoitem->SetAttribute("autoincrement", true);
        }
        else
            infoitem->SetText(prop->GetValueAsString().ToStdString().c_str());

        info->InsertEndChild(infoitem);
    }

    files->SetAttribute("count", m_fileList->GetItemCount());

    // Write all files to the project file
    for (int i = 0; i < m_fileList->GetItemCount(); i++)
    {
        tinyxml2::XMLElement* file = project.NewElement("file");
        std::string sFile = m_fileList->GetItemText(i).ToStdString();

        if (sFile.substr(0, 4) == "(!) ")
            sFile.erase(0, 4);

        file->SetText(sFile.c_str());
        files->InsertEndChild(file);
    }

    project.SaveFile(filename.c_str());

    // Read the written XML file and store it in
    // the list of revisions
    if (revisions.get())
    {
        if (wxFileExists(filename))
        {
            wxFile tempfile(filename);
            wxString contents;
            tempfile.ReadAll(&contents);
            revisions->addRevision(contents);
        }
    }

    wxFileName fname(filename);

    // Update the window title to reflect the loaded project
    SetTitle(_guilang.get("GUI_PKGDLG_HEAD") + " [" + fname.GetFullName() + "]");
}


/////////////////////////////////////////////////
/// \brief Marks the current project as modified.
///
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::markUnsaved()
{
    if (isSaved())
        SetTitle(GetTitle().insert(GetTitle().find("[")+1, "*"));
}


/////////////////////////////////////////////////
/// \brief Returns true, if the current project
/// is in a saved state.
///
/// \return bool
///
/////////////////////////////////////////////////
bool PackageDialog::isSaved()
{
    return GetTitle().find("[*") == std::string::npos;
}


/////////////////////////////////////////////////
/// \brief This function can be used to insert
/// the current mainfile to the dialog. Used by
/// the main window, if the user clicks on
/// "Create package".
///
/// \param mainfile const wxString&
/// \return void
///
/////////////////////////////////////////////////
void PackageDialog::setMainFile(const wxString& mainfile)
{
    if (mainfile.rfind(".nlyt") == std::string::npos)
        m_fileList->InsertItem(m_fileList->GetItemCount(), replacePathSeparator(mainfile.ToStdString()), m_icons->GetIconIndex(".nprc"));
    else
        m_fileList->InsertItem(m_fileList->GetItemCount(), replacePathSeparator(mainfile.ToStdString()), m_icons->GetIconIndex(".nlyt"));

    markUnsaved();
}


/////////////////////////////////////////////////
/// \brief This member function will create a
/// string array containing the procedure files,
/// which are currently part of the list view,
/// and return them.
///
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString PackageDialog::getProcedures()
{
    wxArrayString stringArray;

    for (int i = 0; i < m_fileList->GetItemCount(); i++)
    {
        stringArray.Add(m_fileList->GetItemText(i));
    }

    return stringArray;
}


/////////////////////////////////////////////////
/// \brief This member function will create the
/// install information section of the package
/// based upon the setted properties.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString PackageDialog::getInstallInfo()
{
    wxString installInfo = "\t<info>\r\n";

    // Has the user selected a plugin type as package type?
    bool isplugin = isPlugin();

    // Get an iterator for the properties grid and iterate
    // through it
    for (auto iter = m_packageProperties->GetIterator(); !iter.AtEnd(); iter++)
    {
        wxPGProperty* prop = *iter;

        // "-type" and "-flag" don't need quoation marks
        if (prop->GetName() == "-type" || prop->GetName() == "-flags")
        {
            installInfo += "\t\t" + prop->GetName() + "=" + prop->GetValueAsString() + "\r\n";
            continue;
        }
        else if ((prop->GetName().substr(0, 7) == "-plugin" && (!isplugin || !prop->GetValueAsString().length())) || prop->GetName()[0] != '-')
            continue;
        else if (prop->GetName() == "-name")
        {
            installInfo += "\t\t-name=\"" + getPackageName() + "\"\r\n";
            continue;
        }
        else if (prop->GetName() == "-version" && prop->GetValueAsString() == "<AUTO>")
        {
            installInfo += "\t\t-version=\"0.0.1\"\r\n";
            continue;
        }
        else if (prop->GetName() == "-requirepackages" && !prop->GetValueAsString().length())
            continue;

        // Add the current property to the set
        wxString value = prop->GetValueAsString();
        value.Replace("\"", "\\\"");
        installInfo += "\t\t" + prop->GetName() + "=\"" + value + "\"\r\n";
    }

    // Add the require version property to the current install
    // information
    installInfo += "\t\t-requireversion=\"" + sVersion.substr(0, sVersion.find(' ')) + "\"\r\n";
    installInfo += "\t<endinfo>\r\n";

    return installInfo;
}


/////////////////////////////////////////////////
/// \brief This member function returns the name
/// of the package, as been set by the user.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString PackageDialog::getPackageName()
{
    wxString name = m_packageProperties->GetPropertyByName("-name")->GetValueAsString();

    // Fallback name, if nothing set
    if (!name.length())
        return "newpackage";

    return name;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// package identifier, which is constructed from
/// the name and only contains lowercase
/// alphanumeric characters and the underscore.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString PackageDialog::getPackageIdentifier()
{
    // Get the name and transform it to lower case letters
    std::string identifier = getPackageName().ToStdString();
    identifier = toLowerCase(identifier);

    // Replace every non-alphanumeric character with an underscore
    for (size_t i = 0; i < identifier.length(); i++)
    {
        if (!isalnum(identifier[i]) && identifier[i] != '_')
            identifier[i] = '_';
    }

    if (isPlugin())
        return "plgn_" + identifier;

    return "pkg_" + identifier;
}


/////////////////////////////////////////////////
/// \brief Returns the version of the defined
/// package as a simple string.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString PackageDialog::getPackageVersion()
{
    if (m_packageProperties->GetPropertyByName("-version")->GetValueAsString() == "<AUTO>")
        return "0.0.1";

    return m_packageProperties->GetPropertyByName("-version")->GetValueAsString();
}

/////////////////////////////////////////////////
/// \brief Returns the user-chosen documentation
/// file.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString PackageDialog::getDocFile()
{
    return m_packageProperties->GetPropertyByName(DOCFILE)->GetValueAsString();
}


/////////////////////////////////////////////////
/// \brief This member function returns whether
/// the user selected the "include docs" property.
///
/// \return bool
///
/////////////////////////////////////////////////
bool PackageDialog::includeDocs()
{
    return m_packageProperties->GetPropertyByName(INCLUDEDOCS)->GetValue().GetBool();
}


/////////////////////////////////////////////////
/// \brief This member function returns, whether
/// the user selected a plugin type as install
/// type.
///
/// \return bool
///
/////////////////////////////////////////////////
bool PackageDialog::isPlugin()
{
    return m_packageProperties->GetPropertyByName("-type")->GetValueAsString().find("PLUGIN") != std::string::npos;
}




