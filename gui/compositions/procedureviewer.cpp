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

#include "procedureviewer.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/stringtools.hpp"
#include "../editor/editor.h"

BEGIN_EVENT_TABLE(ProcedureViewer, wxListView)
    EVT_LIST_COL_CLICK(-1, ProcedureViewer::OnColumnClick)
    EVT_LIST_ITEM_ACTIVATED(-1, ProcedureViewer::OnItemClick)
END_EVENT_TABLE()

// Data structure for the procedure viewer to enable
// effective sorting
struct ProcedureViewerData
{
    int ID;
    wxString procedureDefinition;
    wxString procedureFlags;
    wxString procedureReturnVal;

    ProcedureViewerData(int nID, const wxString& procDef, const wxString& flags, const wxString& retVal = "") : ID(nID), procedureDefinition(procDef), procedureFlags(flags), procedureReturnVal(retVal) {}
};

// Sorting callback function. Will use the selected column and the data
// stored in the items to return the order of two elements by returning
// numbers.
// wxCALLBACK aliases some weird calling convention on WIN32.
int wxCALLBACK ProcedureViewerCompare(wxIntPtr item1, wxIntPtr item2, wxIntPtr parent)
{
    ProcedureViewer* viewer = reinterpret_cast<ProcedureViewer*>(parent);

    // Switch to determine the correct data field
    // of the internal data structure
    switch (viewer->nSortColumn)
    {
        case 0:
            if (viewer->vData[item1].ID < viewer->vData[item2].ID)
                return -1;
            else if (viewer->vData[item1].ID == viewer->vData[item2].ID)
                return 0;
            return 1;
        case 1:
            if (viewer->vData[item1].procedureFlags < viewer->vData[item2].procedureFlags)
                return -1;
            else if (viewer->vData[item1].procedureFlags == viewer->vData[item2].procedureFlags)
                return 0;
            return 1;
        case 2:
            if (viewer->vData[item1].procedureReturnVal < viewer->vData[item2].procedureReturnVal)
                return -1;
            else if (viewer->vData[item1].procedureReturnVal == viewer->vData[item2].procedureReturnVal)
                return 0;
            return 1;
        case 3:
            if (viewer->vData[item1].procedureDefinition.Lower() < viewer->vData[item2].procedureDefinition.Lower())
                return -1;
            else if (viewer->vData[item1].procedureDefinition.Lower() == viewer->vData[item2].procedureDefinition.Lower())
                return 0;
            return 1;
    }

    return 0;
}

// This private member function redirects the control
// to the editor to obtain the procedure list of the
// currently watched file
void ProcedureViewer::getProcedureListFromEditor()
{
    if (m_currentEd)
    {
        // Get the list
        updateProcedureList(m_currentEd->getProceduresInFile());
    }
    else
    {
        // Create an empty control
        emptyControl();
    }
}

// This private member function creates an empty control,
// which will display "no procedures in this file" message
void ProcedureViewer::emptyControl()
{
    if (GetItemCount())
    {
        DeleteAllItems();
        vData.clear();
    }

    InsertItem(0, "--");
    SetItem(0, 3, _guilang.get("GUI_PROCEDUREVIEWER_EMPTY"));
    SetItemData(0, 0);

    // Make the message grey and printed in italics
    SetItemFont(0, GetFont().MakeItalic());
    SetItemTextColour(0, wxColour(128, 128, 128));

    // Add a fallback data object
    vData.push_back(ProcedureViewerData(0, "", ""));
}

// Constructor
ProcedureViewer::ProcedureViewer(wxWindow* parent) : wxListView(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_VRULES)
{
    m_currentEd = nullptr;
    nSortColumn = 0;
    vData = std::vector<ProcedureViewerData>();
    AppendColumn("ID");
    AppendColumn("Flags");
    AppendColumn("Returns");
    AppendColumn(_guilang.get("GUI_PROCEDUREVIEWER_SIGNATURE"));

    // Create an empty control
    emptyControl();
}

// This member function removes obsolete whitespaces from
// the passed string
void ProcedureViewer::stripSpaces(wxString& sString)
{
    if (sString.find_first_not_of(' ') != std::string::npos)
    {
        sString.erase(0, sString.find_first_not_of(' '));

        if (sString.find_last_not_of(' ') != std::string::npos)
            sString.erase(sString.find_last_not_of(' ')+1);
    }
    else
        sString.clear();
}

// This member function is used to create and manage
// the Editor <-> ProcedureViewer binding, so that the
// editor may update the procedure list after saving
void ProcedureViewer::setCurrentEditor(NumeReEditor* editor)
{
    // If the procedure viewer was already registered
    // in an editor, unregister it
    if (m_currentEd)
        m_currentEd->registerProcedureViewer(nullptr);

    m_currentEd = editor;

    // If the new editor is not a null pointer,
    // register the procedure viewer
    if (m_currentEd)
        m_currentEd->registerProcedureViewer(this);

    // Get the current procedure list
    getProcedureListFromEditor();
}

// This member function decodes the obtained procedure
// list and updates the contents of this widget
void ProcedureViewer::updateProcedureList(const std::vector<wxString>& vProcedures)
{
    // Clear the contents of the list
    if (GetItemCount())
    {
        DeleteAllItems();
        vData.clear();
    }

    wxString procdef;
    wxString flags;
    wxString returns;

    // Go through the list
    for (size_t i = 0; i < vProcedures.size(); i++)
    {
        // Get the current definition
        procdef = vProcedures[i];
        returns.clear();

        // Split the definition at the flags (if available)
        // and at the definition string (if available)
        if (procdef.find("::") != std::string::npos)
        {
            flags = procdef.substr(procdef.find("::")+2);
            procdef.erase(procdef.find("::"));

            if (flags.find("\n") != std::string::npos)
                flags.erase(flags.find("\n"));

            if (flags.find("->") != std::string::npos)
            {
                returns = flags.substr(flags.find("->")+2);
                flags.erase(flags.find("->"));
            }
        }
        else
        {
            flags.clear();

            if (procdef.find("\n") != std::string::npos)
                procdef.erase(procdef.find("\n"));

            if (procdef.find("->") != std::string::npos)
            {
                returns = procdef.substr(procdef.find("->")+2);
                procdef.erase(procdef.find("->"));
            }
        }

        // Remove obsolete whitespaces
        this->stripSpaces(flags);
        this->stripSpaces(procdef);
        this->stripSpaces(returns);

        // Create a new data object in the buffer
        vData.push_back(ProcedureViewerData(i, procdef, flags, returns));

        // Create the new list item and store
        // a pointer to the current data object
        InsertItem(i, toString(i+1));
        SetItem(i, 1, flags);
        SetItem(i, 2, returns);
        SetItem(i, 3, procdef);
        //SetItemPtrData(i, (wxUIntPtr)&vData[vData.size()-1]);
        SetItemData(i, i);

        // If the current procedure is not flagged as
        // "local" (which is done by the editor automatically)
        // then make its text bold
        if (flags.find("local") == std::string::npos)
            SetItemFont(i, GetFont().MakeBold());
    }

    // Sort the items according the last selected
    // sorting column or create an empty control,
    // if no data is available in the control
    if (GetItemCount())
        SortItems(ProcedureViewerCompare, (wxIntPtr)this);
    else
        emptyControl();

    // Resize the columns
    if (vProcedures.size())
    {
        SetColumnWidth(0, wxLIST_AUTOSIZE);
        SetColumnWidth(1, wxLIST_AUTOSIZE);

        if (GetColumnWidth(1) < 50)
            SetColumnWidth(1, 50);

        SetColumnWidth(2, wxLIST_AUTOSIZE);

        if (GetColumnWidth(2) < 55)
            SetColumnWidth(2, 55);

        SetColumnWidth(3, wxLIST_AUTOSIZE);

        if (GetColumnWidth(3) < 250)
            SetColumnWidth(3, 250);
    }
    else
    {
        SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
        SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
        SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
        SetColumnWidth(3, wxLIST_AUTOSIZE);
    }
}

// This member function is the event handler for
// clicking on the columns header to trigger
// the sorting process
void ProcedureViewer::OnColumnClick(wxListEvent& event)
{
    nSortColumn = event.GetColumn();

    SortItems(ProcedureViewerCompare, (wxIntPtr)this);
}

// This member function is the event handler called
// after the user activated an item. It will focus the
// selected procedure in the editor
void ProcedureViewer::OnItemClick(wxListEvent& event)
{
    if (!vData[event.GetData()].procedureDefinition.length())
        return;

    wxString sProcDef = vData[event.GetData()].procedureDefinition.substr(0, vData[event.GetData()].procedureDefinition.find('('));

    // We can use the "thisfile" namespace always. The
    // editor will find the correct procedure although
    // it might be the naming procedure of the current
    // file
    m_currentEd->FindAndOpenProcedure("$thisfile~" + sProcDef.substr(1));
}

