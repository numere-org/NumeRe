/////////////////////////////////////////////////////////////////////////////
// Name:        ProfOptionsDialog.cpp
// Purpose:
// Author:
// Modified by:
// Created:     02/09/04 19:36:08
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "ProfOptionsDialog.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
#include "wx/wx.h"
////@end includes

#include "ProfOptionsDialog.h"
#include "../perms/p.h"
#include "../common/Crc16.h"
#include "../common/fixvsbug.h"
#include <stdlib.h>

////@begin XPM images
////@end XPM images

extern wxString GlobalPermStrings[];

/*!
 * ProfOptionsDialog type definition
 */

IMPLEMENT_CLASS( ProfOptionsDialog, wxDialog )

/*!
 * ProfOptionsDialog event table definition
 */

BEGIN_EVENT_TABLE( ProfOptionsDialog, wxDialog )

////@begin ProfOptionsDialog event table entries
    EVT_BUTTON( ID_GENERATE, ProfOptionsDialog::OnGenerateClick )

    EVT_BUTTON( ID_BUTTON, ProfOptionsDialog::OnButtonCheckAllClick )

    EVT_BUTTON( ID_BUTTON1, ProfOptionsDialog::OnButtonUncheckAllClick )

    EVT_BUTTON( ID_BUTTON2, ProfOptionsDialog::OnDecodeClick )

    EVT_BUTTON( ID_EXITBUTTON, ProfOptionsDialog::OnExitbuttonClick )

////@end ProfOptionsDialog event table entries

	EVT_CLOSE(ProfOptionsDialog::OnQuit)

END_EVENT_TABLE()

#pragma warning( disable : 4267)



/*!
 * ProfOptionsDialog constructors
 */

ProfOptionsDialog::ProfOptionsDialog( )
{
}

ProfOptionsDialog::ProfOptionsDialog( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ProfOptionsDialog creator
 */

bool ProfOptionsDialog::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ProfOptionsDialog member initialisation
////@end ProfOptionsDialog member initialisation

////@begin ProfOptionsDialog creation
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ProfOptionsDialog creation

	for(int i = PERM_FIRST; i < PERM_LAST; i++)
	{
		m_chklstModules->Append(GlobalPermStrings[i]);
	}

	srand(time(0));
    return TRUE;
}

/*!
 * Control creation for ProfOptionsDialog
 */

void ProfOptionsDialog::CreateControls()
{
////@begin ProfOptionsDialog content construction

    ProfOptionsDialog* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* item4 = new wxBoxSizer(wxVERTICAL);
    item3->Add(item4, 0, wxALIGN_TOP|wxALL, 5);

    wxStaticText* item5 = new wxStaticText( item1, wxID_STATIC, _("Available modules:"), wxDefaultPosition, wxDefaultSize, 0 );
    item4->Add(item5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* item6Strings = NULL;
    wxCheckListBox* item6 = new wxCheckListBox( item1, ID_CHECKLISTBOX, wxDefaultPosition, wxSize(200, 240), 0, item6Strings, 0 );
    m_chklstModules = item6;
    item4->Add(item6, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* item7 = new wxBoxSizer(wxVERTICAL);
    item3->Add(item7, 0, wxALIGN_TOP|wxALL, 5);

    wxStaticText* item8 = new wxStaticText( item1, wxID_STATIC, _("Generated activation code:"), wxDefaultPosition, wxDefaultSize, 0 );
    item7->Add(item8, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item9 = new wxTextCtrl( item1, ID_TXTCODE, _(""), wxDefaultPosition, wxSize(140, -1), 0 );
    m_txtGeneratedCode = item9;
    item7->Add(item9, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);

    wxButton* item10 = new wxButton( item1, ID_GENERATE, _("Generate code"), wxDefaultPosition, wxSize(90, -1), 0 );
    m_genButton = item10;
    item7->Add(item10, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxCheckBox* item11 = new wxCheckBox( item1, ID_RANDOMIZE, _("Randomize code"), wxDefaultPosition, wxSize(100, -1), 0 );
    m_chkRandomize = item11;
    item11->SetValue(FALSE);
    item7->Add(item11, 0, wxALIGN_LEFT|wxALL, 5);

    wxButton* item12 = new wxButton( item1, ID_BUTTON, _("Check all modules"), wxDefaultPosition, wxSize(120, -1), 0 );
    m_butCheckAll = item12;
    item7->Add(item12, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 5);

    wxButton* item13 = new wxButton( item1, ID_BUTTON1, _("Uncheck all modules"), wxDefaultPosition, wxSize(120, -1), 0 );
    m_butUncheckAll = item13;
    item7->Add(item13, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* item14 = new wxBoxSizer(wxHORIZONTAL);
    item7->Add(item14, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* item15 = new wxButton( item1, ID_BUTTON2, _("Decode:"), wxDefaultPosition, wxSize(90, -1), 0 );
    m_decodeButton = item15;
    item7->Add(item15, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);

    wxTextCtrl* item16 = new wxTextCtrl( item1, ID_TEXTCTRL, _(""), wxDefaultPosition, wxSize(140, -1), 0 );
    m_txtDecode = item16;
    item7->Add(item16, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* item17 = new wxBoxSizer(wxHORIZONTAL);
    item7->Add(item17, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* item18 = new wxButton( item1, ID_EXITBUTTON, _("Exit"), wxDefaultPosition, wxDefaultSize, 0 );
    m_exitButton = item18;
    item7->Add(item18, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxTOP, 5);

////@end ProfOptionsDialog content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GENERATE
 */

void ProfOptionsDialog::OnGenerateClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	unsigned int newCode = 0;

	for(int i = 0; i < m_chklstModules->GetCount(); i++)
	{
		if(m_chklstModules->IsChecked(i))
		{
			newCode |= 1 << i;
		}
	}

	if(m_chkRandomize->IsChecked())
	{
		int bitcounter = 30;
		for(bitcounter; bitcounter >= PERM_LAST; bitcounter--)
		{
			if(rand() % 2 == 0)
			{
				newCode |= 1 << bitcounter;
			}

		}
	}
	wxString newAuthCodeString;


	newAuthCodeString.Printf("%u", newCode);
	if(newAuthCodeString.Len() < 3)
	{
		newAuthCodeString.Prepend("000000");
	}


	/*

	long firstNum;
	long nextToLastNum;
	long lastNum;
	int len = newAuthCodeString.Len();
	wxString(newAuthCodeString.GetChar(0)).ToLong(&firstNum);
	wxString(newAuthCodeString.GetChar(len - 2)).ToLong(&nextToLastNum);
	wxString(newAuthCodeString.GetChar(len - 1)).ToLong(&lastNum);

	long firstResult = firstNum + lastNum;
	char firstCheckChar = AuthCodeLookupTable[firstResult];


	long secondResult = firstResult + nextToLastNum;
	if(secondResult > 25)
	{
		secondResult -= 26;
	}

	char secondCheckChar = AuthCodeLookupTable[secondResult];

	char firstRandomChar = (char)( (rand() % 26) + 65);
	char secondRandomChar = (char)( (rand() % 26) + 65);
	*/

	ClsCrc16 crc;
	crc.CrcInitialize();
	crc.CrcAdd(newAuthCodeString.c_str(), newAuthCodeString.Len());
	wxString prefixChars;
	//prefixChars << secondCheckChar << firstRandomChar << secondRandomChar << firstCheckChar;
	prefixChars.Printf("%X", crc.CrcGet());

	newAuthCodeString.Prepend(prefixChars);

	m_txtGeneratedCode->SetValue(newAuthCodeString);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_EXITBUTTON
 */

void ProfOptionsDialog::OnExitbuttonClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
	Destroy();
}

/*!
 * Should we show tooltips?
 */

bool ProfOptionsDialog::ShowToolTips()
{
  return TRUE;
}

void ProfOptionsDialog::OnQuit(wxCloseEvent &event)
{
	Destroy();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
 */

void ProfOptionsDialog::OnButtonUncheckAllClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	for(int i = 0; i < m_chklstModules->GetCount(); i++)
	{
		m_chklstModules->Check(i, false);
	}
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void ProfOptionsDialog::OnButtonCheckAllClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	for(int i = 0; i < m_chklstModules->GetCount(); i++)
	{
		m_chklstModules->Check(i, true);
	}
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON2
 */

void ProfOptionsDialog::OnDecodeClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	for(int i = 0; i < m_chklstModules->GetCount(); i++)
	{
		m_chklstModules->Check(i, false);
	}

	wxString stringToDecode = m_txtDecode->GetValue();

	wxString numberString = stringToDecode.Right(stringToDecode.Len() - 4);
	wxString prefixString = stringToDecode.Left(4);

	ClsCrc16 crc;
	crc.CrcInitialize();
	crc.CrcAdd(numberString.c_str(), numberString.Len());
	wxString generatedCRC;

	generatedCRC.Printf("%X", crc.CrcGet());

	if(generatedCRC != prefixString)
	{
		wxMessageBox("Invalid authorization code!", "Invalid code", wxOK | wxICON_WARNING);
		return;
	}

	long numberLong;

	numberString.ToLong(&numberLong);


	for(int i = 0; i < PERM_LAST; i++)
	{
		m_chklstModules->Check(i, (numberLong & (1 << i)));
	}




}


