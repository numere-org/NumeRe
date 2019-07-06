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
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include "../../kernel/core/procedure/dependency.hpp"
#include "../../common/datastructures.h"

#include <set>

#define INCLUDEDOCS "includedocs"

extern Language _guilang;


BEGIN_EVENT_TABLE(PackageDialog, wxDialog)
    EVT_BUTTON(ID_PKGDLG_ADD, PackageDialog::OnAddItems)
    EVT_BUTTON(ID_PKGDLG_REMOVE, PackageDialog::OnRemoveItems)
    EVT_BUTTON(ID_PKGDLG_AUTODETECT, PackageDialog::OnAutoDetect)
END_EVENT_TABLE()

// Constructor
PackageDialog::PackageDialog(wxWindow* parent, wxTerm* terminal, IconManager* icons) : wxDialog(parent, wxID_ANY, _guilang.get("GUI_PKGDLG_HEAD"), wxDefaultPosition, wxSize(600, 570), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_packageProperties = nullptr;
    m_fileList = nullptr;
    m_terminal = terminal;
    m_icons = icons;

    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

    // Create new grouped panel for this dialog
    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);

    // Create the properties group
    wxStaticBoxSizer* group = panel->createGroup(_guilang.get("GUI_PKGDLG_PROPERTIES"));

    // Create a property grid containing the package
    // properties
    m_packageProperties = new wxPropertyGrid(group->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_THEME_BORDER | wxPG_TOOLTIPS | wxPG_SPLITTER_AUTO_CENTER);
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_PACKAGENAME"), "-name"));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_AUTHOR"), "-author"));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_VERSION"), "-version", "<AUTO>"));

    wxArrayString flags;
    flags.Add("ENABLE_DEFAULTS");
    flags.Add("ENABLE_FULL_LOGGING");
    flags.Add("DISABLE_SCREEN_OUTPUT");
    m_packageProperties->Append(new wxEnumProperty(_guilang.get("GUI_PKGDLG_FLAGS"), "-flags", flags));

    wxArrayString type;
    type.Add("TYPE_UNSPECIFIED");
    type.Add("TYPE_PLUGIN");
    type.Add("TYPE_PLUGIN_WITH_RETURN_VALUE");
    m_packageProperties->Append(new wxEnumProperty(_guilang.get("GUI_PKGDLG_TYPE"), "-type", type));

    // Add a validator to the package command to enssure that the user
    // only uses alphanumeric characters as command string
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_PLUGINCOMMAND"), "-plugincommand"));
    m_packageProperties->SetPropertyValidator("-plugincommand", wxTextValidator(wxFILTER_ALPHANUMERIC));

    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_PLUGINMAIN"), "-pluginmain"));
    m_packageProperties->Append(new wxStringProperty(_guilang.get("GUI_PKGDLG_PLUGINDESC"), "-plugindesc"));
    m_packageProperties->Append(new wxBoolProperty(_guilang.get("GUI_PKGDLG_INCLUDEDOCUMENTATION"), INCLUDEDOCS));

    // Apply some general settings to the whole property grid
    m_packageProperties->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
    m_packageProperties->SetValidationFailureBehavior(wxPG_VFB_MARK_CELL | wxPG_VFB_STAY_IN_PROPERTY);

    group->Add(m_packageProperties, 0, wxEXPAND | wxALL, 5);

    // Create the files group
    group = panel->createGroup(_guilang.get("GUI_PKGDLG_FILES"));

    // Create a list view for the files and add the image list
    // to display the file icons
    m_fileList = panel->CreateListView(group->GetStaticBox(), group, wxLC_LIST | wxLC_ALIGN_LEFT, wxSize(-1, 150));
    m_fileList->SetImageList(icons->GetImageList(), wxIMAGE_LIST_SMALL);

    // Create the add, remove and autodetect buttons
    wxButton* addButton = new wxButton(group->GetStaticBox(), ID_PKGDLG_ADD, _guilang.get("GUI_PKGDLG_ADDFILES"));
    wxButton* removeButton = new wxButton(group->GetStaticBox(), ID_PKGDLG_REMOVE, _guilang.get("GUI_PKGDLG_REMOVEFILES"));
    wxButton* autoButton = new wxButton(group->GetStaticBox(), ID_PKGDLG_AUTODETECT, _guilang.get("GUI_PKGDLG_AUTODETECTFILES"));

    // Add the buttons to a special horizontal sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    buttonSizer->Add(addButton, 1, wxALL | wxFIXED_MINSIZE, 5);
    buttonSizer->Add(removeButton, 1, wxALL | wxFIXED_MINSIZE, 5);
    buttonSizer->Add(autoButton, 1, wxALL | wxFIXED_MINSIZE, 5);

    // Add the button sizer to the panel
    group->Add(buttonSizer, 0, wxFIXED_MINSIZE | wxSHRINK | wxALIGN_CENTER_HORIZONTAL);

    // Activate scroll bars
    panel->SetScrollbars(0, 20, 0, 200);

    // Add the panel to the vertical sizer of the dialog and
    // add the standard buttons to the dialog as well
    vsizer->Add(panel, 1, wxEXPAND, 0);
    vsizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 5);

    SetSizer(vsizer);
}

// This is the event handler for the autodetect button
void PackageDialog::OnAutoDetect(wxCommandEvent& event)
{
    autoDetect(getProcedures());
}

// This is the event handler for the add files button
void PackageDialog::OnAddItems(wxCommandEvent& event)
{
    // Create a file dialog focused on procedure files. The user may
    // select multiple files
    wxFileDialog dlg(this, _guilang.get("GUI_PKGDLG_SELECT_FILES"), m_terminal->getPathSettings()[PROCPATH], wxEmptyString, _guilang.get("COMMON_FILETYPE_NPRC") + " (*.nprc)|*.nprc", wxFD_MULTIPLE);

    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString files;
        dlg.GetPaths(files);

        // Insert the selected items, if they're not already part
        // of the files set
        for (size_t i = 0; i < files.size(); i++)
        {
            if (m_fileList->FindItem(-1, replacePathSeparator(files[i].ToStdString()), false) == -1)
                m_fileList->InsertItem(m_fileList->GetItemCount(), replacePathSeparator(files[i].ToStdString()), m_icons->GetIconIndex(".nprc"));
        }
    }
}

// This is the event handler for the remove files button
void PackageDialog::OnRemoveItems(wxCommandEvent& event)
{
    long selected = m_fileList->GetFirstSelected();

    // Remove all selected items
    while (m_fileList->GetItemCount() && m_fileList->GetSelectedItemCount() && selected != -1)
    {
        m_fileList->DeleteItem(selected);
        selected = m_fileList->GetFirstSelected();
    }
}

// This function autodetects the dependencies of the passed
// files by calling the PackageDialog::followBranch() function
// recursively
void PackageDialog::autoDetect(const wxArrayString& mainfiles)
{
    // Create a set (forces unique names)
    std::set<std::string> fileSet;

    // Call followBranch() for every file name in the array
    // of strings
    for (size_t i = 0; i < mainfiles.size(); i++)
        followBranch(replacePathSeparator(mainfiles[i].ToStdString()), fileSet);

    // Insert only unique file names into the list view
    for (auto iter = fileSet.begin(); iter != fileSet.end(); ++iter)
    {
        if (m_fileList->FindItem(-1, *iter, false) == -1)
            m_fileList->InsertItem(m_fileList->GetItemCount(), *iter, m_icons->GetIconIndex(".nprc"));
    }
}

// This function will recursively call itself to detect a whole
// branch of file dependencies
void PackageDialog::followBranch(const string& sFile, set<string>& fileSet)
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

// This function can be used to insert the current mainfile to the
// dialog. Used by the main window, if the user clicks on "Create package"
void PackageDialog::setMainFile(const wxString& mainfile)
{
    m_fileList->InsertItem(m_fileList->GetItemCount(), replacePathSeparator(mainfile.ToStdString()), m_icons->GetIconIndex(".nprc"));
}

// This member function will create a string array containing
// the procedure files, which are currently part of the list
// view, and return them
wxArrayString PackageDialog::getProcedures()
{
    wxArrayString stringArray;

    for (int i = 0; i < m_fileList->GetItemCount(); i++)
    {
        stringArray.Add(m_fileList->GetItemText(i));
    }

    return stringArray;
}

// This member function will create the install information
// section of the package based upon the setted properties
wxString PackageDialog::getInstallInfo()
{
    wxString installInfo = "\t<info>\n";

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
            installInfo += "\t\t" + prop->GetName() + "=" + prop->GetValueAsString() + "\n";
            continue;
        }
        else if ((prop->GetName().substr(0, 7) == "-plugin" && !isplugin) || prop->GetName()[0] != '-')
            continue;

        // Add the current property to the set
        installInfo += "\t\t" + prop->GetName() + "=\"" + prop->GetValueAsString() + "\"\n";
    }

    // Add the require version property to the current install
    // information
    installInfo += "\t\t-requireversion=\"" + sVersion.substr(0, sVersion.find(' ')) + "\"\n";
    installInfo += "\t<endinfo>\n";

    return installInfo;
}

// This member function returns the name of the package, as
// been set by the user
wxString PackageDialog::getPackageName()
{
    wxString name = m_packageProperties->GetPropertyByName("-name")->GetValueAsString();

    // Fallback name, if nothing set
    if (!name.length())
        return "newpackage";

    return name;
}

// This member function returns the package identifier,
// which is constructed from the name and only contains
// lowercase alphanumeric characters and the underscore
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

    return identifier;
}

// This member function returns whether the user selected the
// "include docs" property
bool PackageDialog::includeDocs()
{
    return m_packageProperties->GetPropertyByName(INCLUDEDOCS)->GetValue().GetBool();
}

// This member function returns, whether the user selected a
// plugin type as install type
bool PackageDialog::isPlugin()
{
    return m_packageProperties->GetPropertyByName("-type")->GetValueAsString().find("TYPE_PLUGIN") != std::string::npos;
}




