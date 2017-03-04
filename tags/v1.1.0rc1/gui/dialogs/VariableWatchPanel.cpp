/////////////////////////////////////////////////////////////////////////////
// Name:        VariableWatchPanel.cpp
// Purpose:
// Author:
// Modified by:
// Created:     03/29/04 21:43:35
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "VariableWatchPanel.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include <wx/textdlg.h>

#include "VariableWatchPanel.h"
//#include "AddVariableWatch.h"
#include "../NumeReWindow.h"
#include "../../common/tree.h"

#include "../../common/fixvsbug.h"



typedef tree_node_<wxString> ParseTreeNode;
typedef tree<wxString>::iterator PTIterator;


int FindCharOutsideQuotes(const wxString& str, wxChar ch);
int FindCommaPos(const wxString& str);
void ParseEntry(ParseTree& tree, PTIterator& currentNode, wxString& text);
void print_tree(const ParseTree& tr, ParseTree::pre_order_iterator it, ParseTree::pre_order_iterator end);
PTIterator InsertEntry(ParseTree& tree, PTIterator& currentNode, wxString text);

WX_DECLARE_HASH_MAP( ParseTreeNode*,      // type of the keys
					wxTreeItemId,    // type of the values
					wxPointerHash,     // hasher
					wxPointerEqual,   // key equality predicate
					NodeIdHash); // name of the class



////@begin XPM images
////@end XPM images

/*!
 * VariableWatchPanel type definition
 */

IMPLEMENT_CLASS( VariableWatchPanel, wxPanel )

/*!
 * VariableWatchPanel event table definition
 */

BEGIN_EVENT_TABLE( VariableWatchPanel, wxPanel )

////@begin VariableWatchPanel event table entries
    EVT_SIZE( VariableWatchPanel::OnSize )

    EVT_BUTTON( ID_ADDWATCH, VariableWatchPanel::OnAddwatchClick )

    EVT_BUTTON( ID_REMOVEWATCH, VariableWatchPanel::OnRemovewatchClick )

    EVT_BUTTON( ID_CLEARALLWATCHES, VariableWatchPanel::OnClearallwatchesClick )

////@end VariableWatchPanel event table entries

END_EVENT_TABLE()

/*!
 * VariableWatchPanel constructors
 */


VariableWatchPanel::VariableWatchPanel( )
{
}

VariableWatchPanel::VariableWatchPanel( wxWindow* parent, wxEvtHandler* mainframe, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);

	m_parentEventHandler = mainframe;

	wxListItem itemCol;
	itemCol.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_WIDTH;
	itemCol.m_text = _T("Name");
	itemCol.m_width = 120;
	m_list->InsertColumn(0, itemCol);
	itemCol.m_text = _T("Type");
	itemCol.m_width = 80;
	m_list->InsertColumn(1, itemCol);
	itemCol.m_text = _T("Value");
	itemCol.m_width = 400;
	m_list->InsertColumn(2, itemCol);

	wxSize minSize;
	minSize.Set(100, 100);

	m_list->SetMinSize(minSize);


	/*
	int st = 0;

	st |= wxTR_HIDE_ROOT;
	st |= wxTR_HAS_BUTTONS;
	st |= wxTR_VRULE;
	st |= wxTR_HRULE;
	st |= wxTR_FULL_ROW_HIGHLIGHT;

	m_tree->SetFlag(st);
	m_tree->AddColumn("Value", 400);
	m_tree->InsertColumn(0, "Type", 80);
	m_tree->InsertColumn(0, "Name", 120);
	//m_tree->AddColumn("Name", 120);
	//m_tree->AddColumn("Type", 80);




	m_tree->AddRoot("Root");
	m_tree->SetMainColumn(0);
	*/

/*
	wxTreeItemId root = m_tree->GetRootItem();


	wxTreeItemId first = m_tree->AppendItem(root, "first");
	wxTreeItemId second = m_tree->AppendItem(root, "second");
	wxTreeItemId third = m_tree->AppendItem(root, "third");
	wxTreeItemId fourth = m_tree->AppendItem(root, "fourth");


	m_tree->AppendItem(first, "1-1");
	m_tree->AppendItem(second, "1-2");
	m_tree->AppendItem(second, "1-3");
*/

}

/*!
 * VariableWatchPanel creator
 */

bool VariableWatchPanel::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin VariableWatchPanel member initialisation
    m_list = NULL;
////@end VariableWatchPanel member initialisation

////@begin VariableWatchPanel creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end VariableWatchPanel creation
    return TRUE;
}

/*!
 * Control creation for VariableWatchPanel
 */

void VariableWatchPanel::CreateControls()
{
////@begin VariableWatchPanel content construction
    VariableWatchPanel* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    m_list = new wxListCtrl( itemPanel1, ID_LISTCTRL, wxDefaultPosition, wxSize(400, 300), wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES|wxSIMPLE_BORDER );
    itemBoxSizer2->Add(m_list, 1, wxGROW, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);

    wxButton* itemButton5 = new wxButton( itemPanel1, ID_ADDWATCH, _("Add watch"), wxDefaultPosition, wxSize(80, -1), 0 );
    itemBoxSizer4->Add(itemButton5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton6 = new wxButton( itemPanel1, ID_REMOVEWATCH, _("Remove watch"), wxDefaultPosition, wxSize(80, -1), 0 );
    itemBoxSizer4->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton7 = new wxButton( itemPanel1, ID_CLEARALLWATCHES, _("Clear watches"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end VariableWatchPanel content construction

	/*
	int style = wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT;// | wxTR_FULL_ROW_HIGHLIGHT | wxTR_SINGLE | wxTR_LINES_AT_ROOT;
	m_tree = new wxTreeListCtrl(itemNotebook3, ID_TREELIST);//, wxDefaultPosition, wxDefaultSize, style);
	itemNotebook3->AddPage(m_tree, "Variables (tree)");
	*/

}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ADDWATCH
 */

void VariableWatchPanel::OnAddwatchClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	AddWatch();


}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_REMOVEWATCH
 */

void VariableWatchPanel::OnRemovewatchClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	RemoveWatch();
}

/*!
 * Should we show tooltips?
 */

bool VariableWatchPanel::ShowToolTips()
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
///  public AddWatch
///  Adds a variable watch to the debugger
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void VariableWatchPanel::AddWatch()
{
	wxString varName = wxGetTextFromUser("Variable name to watch:", "Add Variable Watch");

	//AddVariableWatchDialog avwd(this);

	//int result = avwd.ShowModal();



	//if(result == wxOK)
	if(varName != wxEmptyString)
	{
		wxDebugEvent dbg;

		dbg.SetId(ID_DEBUG_ADD_WATCH);

		/*
		//wxString varName = avwd.GetVariableName();
		//wxString funcName = avwd.GetFunctionName();
		//wxString className = wxEmptyString;

		if(avwd.FunctionInClass())
		{
			className = avwd.GetClassName();
		}
		*/

		wxArrayString vars;
		vars.Add(varName);
		dbg.SetVariableNames(vars);
		//dbg.SetFunctionName(funcName);
		//dbg.SetClassName(className);
		m_parentEventHandler->AddPendingEvent(dbg);


	}
}

void VariableWatchPanel::AddWatchedVariables(wxDebugEvent debug)
{
	wxArrayString variableNames = debug.GetVariableNames();

	//wxTreeItemId root = m_tree->GetRootItem();

	for(size_t i = 0; i < variableNames.size(); i++)
	{

		wxString varName = variableNames[i];
		m_list->InsertItem(m_list->GetItemCount(), varName);

		/*
		int varCount = m_tree->GetChildrenCount(root);
		wxTreeItemId item = m_tree->AppendItem(root, varName);
		*/
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public RemoveWatch
///  Removes a variable watch from the debugger
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void VariableWatchPanel::RemoveWatch()
{
	if(m_list->GetSelectedItemCount() > 0)
	{
		int num = -1;
		int index = m_list->GetNextItem(num, wxLIST_NEXT_ALL,	wxLIST_STATE_SELECTED);

		wxListItem item;
		item.SetId(index);
		item.m_mask = wxLIST_MASK_TEXT;

		item.m_col = 0;
		m_list->GetItem(item);
		wxString name = item.m_text;

		wxDebugEvent dbg;

		dbg.SetId(ID_DEBUG_REMOVE_WATCH);
		wxArrayString vars;
		vars.Add(name);
		dbg.SetVariableNames(vars);
		m_parentEventHandler->AddPendingEvent(dbg);

		m_list->DeleteItem(index);

	}

	/*
	wxTreeItemId selectedItem = m_tree->GetSelection();
	if(selectedItem.IsOk())
	{
		wxString name = m_tree->GetItemText(selectedItem, 0);

		wxDebugEvent dbg;

		dbg.SetId(ID_DEBUG_REMOVE_WATCH);
		wxArrayString vars;
		vars.Add(name);
		dbg.SetVariableNames(vars);
		m_parentEventHandler->AddPendingEvent(dbg);

		m_tree->Delete(selectedItem);


	}
	*/
}

//////////////////////////////////////////////////////////////////////////////
///  public UpdateVariableInfo
///  Updates the displayed values of the variables being watched
///
///  @param  event wxDebugEvent  The event containing the variable info
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void VariableWatchPanel::UpdateVariableInfo(wxDebugEvent event)
{
	wxArrayString names = event.GetVariableNames();
	wxArrayString values = event.GetVariableValues();
	wxArrayString types = event.GetVariableTypes();

	//wxSortedArrayString sortedNames(names);

	//m_list->DeleteAllItems();

	wxColour red("red");
	wxColour black("black");

	wxListItem textColorItem;
	textColorItem.SetTextColour(black);
	textColorItem.m_col = 2;

	textColorItem.SetTextColour(red);

	wxListItem retrievalItem;
	retrievalItem.m_mask = wxLIST_MASK_TEXT;
	retrievalItem.m_col = 1;

	// update any new/changed values
	for(int i = 0; i < (int)names.GetCount(); i++)
	{
		wxString name = names[i];
		int idx = names.Index(name);

		if(idx >= values.GetCount())
		{
			continue;
		}

		wxString value = values[idx];
		wxString type = types[idx];

		int nameLineNum = m_list->FindItem(-1, name);

		// found the item's name
		if(nameLineNum > -1)
		{
			m_list->SetItem(nameLineNum, 1, type);
			retrievalItem.m_itemId = nameLineNum;
			retrievalItem.m_col = 2;
			m_list->GetItem(retrievalItem);

			wxString listedValue = retrievalItem.m_text;

			if(listedValue == value)
			{
				m_list->SetItemTextColour(nameLineNum, black);
			}
			else
			{
				m_list->SetItem(nameLineNum, 2, value);
				m_list->SetItemTextColour(nameLineNum, red);
			}
		}

		/*
		wxTreeItemId root = m_tree->GetRootItem();
		wxTreeItemIdValue cookie;
		wxTreeItemId firstChild = m_tree->GetFirstChild(root, cookie);

		wxTreeItemId item = m_tree->FindItem(root, name, wxTL_MODE_NAV_LEVEL | wxTL_MODE_FIND_EXACT);

		if(item.IsOk())
		{
			m_tree->SetItemText(item, 1, type);

			wxString currentValue = m_tree->GetItemText(item, 2);

			if(currentValue == value)
			{
				m_tree->SetItemTextColour(item, black);
			}
			else
			{
				m_tree->SetItemText(item, 2, value);
				m_tree->SetItemTextColour(item, red);
			}

			int braceOpenPos = FindCharOutsideQuotes(value, _T('{'));

			if(braceOpenPos > -1)
			{
				ParseTree tree;
				PTIterator top = tree.begin();
				PTIterator first = tree.insert(top, name);

				wxString copiedValue = value;
				ParseEntry(tree, first, copiedValue);

				DisplayParsedValue(item, tree);
			}
		}
		*/
	}
}

void VariableWatchPanel::DisplayParsedValue(wxTreeItemId currentNodeId, ParseTree& tree)
{

	if(m_tree->GetChildrenCount(currentNodeId) > 0)
	{
		// assume we have already inserted everything
	}
	else
	{
		// insert stuff
		ParseTree::breadth_first_iterator it = tree.begin_breadth_first();

		ParseTree::breadth_first_iterator end = tree.end_breadth_first();

		NodeIdHash addedNodes;

		ParseTreeNode* parentNode = it.node;
		ParseTreeNode* rootNode = parentNode;
		//ParseTreeNode* previousNode = parentNode;
		//int oldDepth = tree.depth(it);
		wxTreeItemId currentParentId = currentNodeId;

		it++;

		while(it!=end)
		{
			//int currentDepth = tree.depth(it);

			if(it.node->parent != parentNode && it.node->parent != rootNode)
			{
				parentNode = it.node->parent;
				currentParentId = addedNodes[parentNode];
			}

			wxString data = it.node->data;
			//ParseTreeNode* currentNode = it.node;


			wxTreeItemId addedId = m_tree->AppendItem(currentParentId, data);
			addedNodes[it.node] = addedId;

			//it.node->parent->data
			++it;
		}
	}

}

//////////////////////////////////////////////////////////////////////////////
///  public ClearVariableValues
///  Gives all displayed variables an empty value
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void VariableWatchPanel::ClearVariableValues()
{
	for(int i = 0; i < m_list->GetItemCount(); i++)
	{
		m_list->SetItem(i, 2, wxEmptyString);
	}

	/*
	wxTreeItemId root = m_tree->GetRootItem();

	int numItems = m_tree->GetChildrenCount(root, false);

	wxTreeItemIdValue cookie;
	wxTreeItemId item = m_tree->GetFirstChild (root, cookie);
	while (item.IsOk())
	{
		m_tree->SetItemText(item, 2, wxEmptyString);

		int numChildren = m_tree->GetChildrenCount(item);

		if(numChildren > 0)
		{
			m_tree->DeleteChildren(item);
		}

		item = m_tree->GetNextSibling (item);
	}
	*/
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CLEARALLWATCHES
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnClearallwatchesClick
///  Removes all variables being watched
///
///  @param  event wxCommandEvent & The generated button event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void VariableWatchPanel::OnClearallwatchesClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	int numItems = m_list->GetItemCount();

	wxListItem selectionItem;
	selectionItem.m_mask = wxLIST_MASK_STATE;
	selectionItem.m_stateMask = wxLIST_STATE_SELECTED;
	selectionItem.m_state = wxLIST_STATE_SELECTED;
	selectionItem.SetId(0);


	for(int i = 0; i < numItems; i++)
	{
		m_list->SetItem(selectionItem);
		RemoveWatch();
	}

	/*
	wxTreeItemId root = m_tree->GetRootItem();
	numItems = m_tree->GetChildrenCount(root,false);

	for(int i = 0; i < numItems; i++)
	{
		wxTreeItemIdValue cookie;
		wxTreeItemId child = m_tree->GetFirstChild(root, cookie);

		m_tree->SelectItem(child);
		RemoveWatch();
	}
	*/


}



/*!
 * Get bitmap resources
 */

wxBitmap VariableWatchPanel::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin VariableWatchPanel bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end VariableWatchPanel bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon VariableWatchPanel::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin VariableWatchPanel icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end VariableWatchPanel icon retrieval
}


/*!
 * wxEVT_SIZE event handler for ID_VARWATCHDIALOG
 */

void VariableWatchPanel::OnSize( wxSizeEvent& event )
{
	/*wxSize size = event.GetSize();
	//int width = size.GetWidth();
	int height = size.GetHeight();

	int i = 42;
	int q = i;
	i = q;*/


    event.Skip();

}

void VariableWatchPanel::DebuggerExited()
{
	ClearVariableValues();
}

void VariableWatchPanel::TestParsing()
{
	ParseTree tree;
	PTIterator top = tree.begin();
	PTIterator first = tree.insert(top, "sparky");

	wxTreeItemId item = m_tree->GetRootItem();

	//wxString copiedValue = "{age = 12, numbers = {1, 2, 4, 8, 16, 32}, name = {static npos = 4294967295, _M_dataplus = {<allocator<char>> = {<new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = \"Sparky the Wonder Dog\"}}}";

	wxString testString = "$1 = {{age = 3, name = {static npos = 4294967295,_M_dataplus = {<allocator<char>> = {<new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x3210c4 \"Fred\"}}, sparky = 2003325944}, {age = 4, name = {static npos = 4294967295,_M_dataplus = {<allocator<char>> = {<new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x3210f4 \"George\"}}, sparky = 2003442721}, {age = 5, name = {static npos = 4294967295,_M_dataplus = {<allocator<char>> = {<new_allocator<char>> = {<No data fields>}, <No data fields>}, _M_p = 0x321124 \"Bob\"}}, sparky = 3280624}}";

	ParseEntry(tree, first, testString);

	DisplayParsedValue(item, tree);
}


void ParseEntry(ParseTree& tree, PTIterator& currentNode, wxString& text)
{
	if (text.IsEmpty())
		return;

	while (1)
	{
		// trim the string from left and right
		text.Trim(true);
		text.Trim(false);

		// find position of '{', '}' and ',' ***outside*** of any quotes.
		// decide which is nearer to the start
		int braceOpenPos = FindCharOutsideQuotes(text, _T('{'));
		if (braceOpenPos == -1)    braceOpenPos = 0xFFFFFE;
		int braceClosePos = FindCharOutsideQuotes(text, _T('}'));
		if (braceClosePos == -1) braceClosePos = 0xFFFFFE;
		int commaPos = FindCommaPos(text);
		if (commaPos == -1) commaPos = 0xFFFFFE;
		int pos = std::min(commaPos, std::min(braceOpenPos, braceClosePos));

		if (pos == 0xFFFFFE)
		{
			// no comma, opening or closing brace
			if (text.Right(3).Matches(_T(" = ")))
				text.Truncate(text.Length() - 3);
			if (!text.IsEmpty())
			{
				//entry.AddChild(text, watch);
				tree.append_child(currentNode, text);
				text.Clear();
			}
			break;
		}
		else
		{
			// display array on a single line?
			// normal (multiple lines) display is taken care below, with array indexing

			/*
			if (watch &&
			watch->is_array &&
			braceOpenPos != 0xFFFFFE &&
			braceClosePos != 0xFFFFFE)
			{
				wxString tmp = text.Left(braceClosePos + 1);
				// if more than one opening/closing brace, then it's a complex array so
				// ignore single-line
				if (text.Freq(_T('{')) == 1 && text.Freq(_T('}')) == 1)
				{
					// array on single line for up to 8 (by default) elements
					// if more elements, fall through to the multi-line display
					int commas = 8;
					if (tmp.Freq(_T(',')) < commas)
					{
						// array watch type
						tmp[braceOpenPos] = _T('[');
						tmp.Last() = _T(']');
						entry.AddChild(tmp, watch);
						text.Remove(0, braceClosePos + 1);
						continue;
					}
				}
			}
			*/

			wxString tmp = text.Left(pos);
			//WatchTreeEntry* newchild = 0;
			PTIterator newChild = tree.end();

			if (tmp.Right(3).Matches(_T(" = ")))
				tmp.Truncate(tmp.Length() - 3); // remove " = " if last in string
			if (!tmp.IsEmpty())
			{
				// take array indexing into account (if applicable)
				//if (array_index != -1)
				//	tmp.Prepend(wxString::Format(_T("[%d]: "), array_index++));

				//newchild = &entry.AddChild(tmp, watch);
				newChild = tree.append_child(currentNode, tmp);
			}
			text.Remove(0, pos + 1);

			if (pos == braceOpenPos)
			{
				if (newChild == tree.end())
				{
					newChild = currentNode;
				}

				//ParseEntry(*newchild, watch, text, array_index); // proceed one level deeper
				ParseEntry(tree, newChild, text);

			}
			else if (pos == braceClosePos)
			{
				break; // return one level up
			}
		}
	}
}

int FindCharOutsideQuotes(const wxString& str, wxChar ch)
{
	int len = str.Length();
	int i = 0;
	bool inSingleQuotes = false;
	bool inDoubleQuotes = false;
	wxChar lastChar = _T('\0');
	while (i < len)
	{
		wxChar currChar = str.GetChar(i);

		// did we find the char outside of any quotes?
		if (!inSingleQuotes && !inDoubleQuotes && currChar == ch)
			return i;

		// double quotes (not escaped)
		if (currChar == _T('"') && lastChar != _T('\\'))
		{
			// if not in single quotes, toggle the flag
			if (!inSingleQuotes)
				inDoubleQuotes = !inDoubleQuotes;
		}
		// single quotes (not escaped)
		else if (currChar == _T('\'') && lastChar != _T('\\'))
		{
			// if not in double quotes, toggle the flag
			if (!inDoubleQuotes)
				inSingleQuotes = !inSingleQuotes;
		}
		// don't be fooled by double-escape
		else if (currChar == _T('\\') && lastChar == _T('\\'))
		{
			// this will be assigned to lastChar
			// so it's not an escape char
			currChar = _T('\0');
		}

		lastChar = currChar;
		++i;
	}
	return -1;
}

int FindCommaPos(const wxString& str)
{
	// comma is a special case because it separates the fields
	// but it can also appear in a function/template signature, where
	// we shouldn't treat it as a field separator

	// what we 'll do now, is decide if the comma is inside
	// a function signature.
	// we 'll do it by counting the opening and closing parenthesis/angled-brackets
	// *up to* the comma.
	// if they 're equal, it's a field separator.
	// if they 're not, it's in a function signature
	// ;)

	int len = str.Length();
	int i = 0;
	int parCount = 0;
	int braCount = 0;
	bool inQuotes = false;
	while (i < len)
	{
		wxChar ch = str.GetChar(i);
		switch (ch)
		{
		case _T('('):
			++parCount; // increment on opening parenthesis
			break;

		case _T(')'):
			--parCount; // decrement on closing parenthesis
			break;

		case _T('<'):
			++braCount; // increment on opening angle bracket
			break;

		case _T('>'):
			--braCount; // decrement on closing angle bracket
			break;

		case _T('"'):
			// fall through
		case _T('\''):
			inQuotes = !inQuotes; // toggle inQuotes flag
			break;

		default:
			break;
		}

		// if it's not inside quotes *and* we have parCount == 0, it's a field separator
		if (!inQuotes && parCount == 0 && braCount == 0 && ch == _T(','))
			return i;
		++i;
	}
	return -1;
}

/*
void print_tree(const ParseTree& tr, ParseTree::pre_order_iterator it, ParseTree::pre_order_iterator end)
{
	if(!tr.is_valid(it))
	{
		return;
	}

	int rootdepth=tr.depth(it);
	std::cout << "-----" << std::endl;

	while(it!=end)
	{
		for(int i=0; i<tr.depth(it)-rootdepth; ++i)
			std::cout << "  ";
		std::cout << (*it) << std::endl << std::flush;
		++it;
	}

	std::cout << "-----" << std::endl;
}
*/

PTIterator InsertEntry(ParseTree& tree, PTIterator& currentNode, wxString& text)
{
	PTIterator result = tree.end();

	if(currentNode == tree.begin())
	{
		result = tree.insert(currentNode, text);
	}
	else
	{
		result = tree.append_child(currentNode, text);
	}

	return result;
}
