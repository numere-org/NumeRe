/////////////////////////////////////////////////////////////////////////////
// Name:        CompilerOutputPanel.cpp
// Purpose:
// Author:
// Modified by:
// Created:     04/12/04 19:39:08
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "CompilerOutputPanel.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "CompilerOutputPanel.h"
#include "wx/generic/gridctrl.h"
#include "../../common/Options.h"
#include "../../compiler/compilerevent.h"
#include "../../common/datastructures.h"
#include "../NumeReWindow.h"
#include <wx/regex.h>

////@begin XPM images
////@end XPM images

/*!
 * CompilerOutputPanel type definition
 */

IMPLEMENT_CLASS( CompilerOutputPanel, wxPanel )

/*!
 * CompilerOutputPanel event table definition
 */

BEGIN_EVENT_TABLE( CompilerOutputPanel, wxPanel )

////@begin CompilerOutputPanel event table entries
////@end CompilerOutputPanel event table entries
	EVT_COMPILER_START(CompilerOutputPanel::OnCompilerStart)
	EVT_COMPILER_PROBLEM(CompilerOutputPanel::OnCompilerProblem)
	EVT_COMPILER_END(CompilerOutputPanel::OnCompilerEnd)
	EVT_GRID_CELL_LEFT_DCLICK(CompilerOutputPanel::OnGridDoubleClick)

END_EVENT_TABLE()

/*!
 * CompilerOutputPanel constructors
 */


CompilerOutputPanel::CompilerOutputPanel( )
{
}

CompilerOutputPanel::CompilerOutputPanel( wxWindow* parent, NumeReWindow* mainFrame, Options* options, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);

	m_options = options;

	SetAdvanced(true);

	wxFont monospacedFont(10, wxMODERN, wxNORMAL, wxNORMAL, false, "Courier New");
	m_textbox->SetFont(monospacedFont);
	m_grid->SetDefaultCellFont(monospacedFont);
	m_grid->SetEditable(false);
	m_grid->SetGridLineColour(wxColour("black"));
	m_grid->SetColumnWidth(0, 200);
	m_grid->SetColumnWidth(1, 60);
	m_grid->SetColumnWidth(2, 480);
	m_grid->SetColLabelValue(0, "Filename");
	m_grid->SetColLabelValue(1, "Line");
	m_grid->SetColLabelValue(2, "Message");
	// turn off the grid selection cursor
	m_grid->SetCellHighlightPenWidth(0);

	m_mainFrame = mainFrame;

	ClearOutput();
}

/*!
 * CompilerOutputPanel creator
 */

bool CompilerOutputPanel::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CompilerOutputPanel member initialisation
    m_sizer = NULL;
    m_grid = NULL;
    m_textbox = NULL;
////@end CompilerOutputPanel member initialisation

////@begin CompilerOutputPanel creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end CompilerOutputPanel creation
    return TRUE;
}

/*!
 * Control creation for CompilerOutputPanel
 */

void CompilerOutputPanel::CreateControls()
{
////@begin CompilerOutputPanel content construction
    CompilerOutputPanel* itemPanel1 = this;

    m_sizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(m_sizer);

    m_grid = new wxGrid( itemPanel1, ID_COMPILERGRID, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    m_grid->SetDefaultColSize(80);
    m_grid->SetDefaultRowSize(20);
    m_grid->SetColLabelSize(20);
    m_grid->SetRowLabelSize(0);
    m_grid->CreateGrid(1, 3, wxGrid::wxGridSelectRows);
    m_sizer->Add(m_grid, 1, wxGROW|wxALL, 5);

    m_textbox = new wxTextCtrl( itemPanel1, ID_COMPILERTEXT, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
    m_sizer->Add(m_textbox, 1, wxGROW|wxALL, 5);

////@end CompilerOutputPanel content construction
}

/*!
 * Should we show tooltips?
 */

bool CompilerOutputPanel::ShowToolTips()
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
///  public ClearOutput
///  Clears the output from both the grid and the textbox
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void CompilerOutputPanel::ClearOutput()
{
	int numGridRows = m_grid->GetNumberRows();
	if(numGridRows > 0)
	{
		m_grid->DeleteRows(0, numGridRows);
	}

	m_textbox->SetValue(wxEmptyString);
	m_numErrors = 0;
}

//////////////////////////////////////////////////////////////////////////////
///  public SetAdvanced
///  Enables/disables the textbox and grid as appropriate
///
///  @param  advanced bool  Whether to display advanced or simple output
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void CompilerOutputPanel::SetAdvanced(bool advanced)
{
	m_isAdvanced = advanced;


	m_sizer->Detach(m_grid);
	m_sizer->Detach(m_textbox);

	if(advanced)
	{
		m_textbox->Hide();
		m_sizer->Add(m_grid, 1, wxGROW|wxALL, 5);
		m_grid->Show();
	}
	else
	{
		m_grid->Hide();
		m_sizer->Add(m_textbox, 1, wxGROW|wxALL, 5);
		m_textbox->Show();
	}
	m_sizer->Layout();

}

//////////////////////////////////////////////////////////////////////////////
///  public OnCompilerStart
///  Displays output from a "compile started" event
///
///  @param  event CompilerEvent & The generated compiler event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void CompilerOutputPanel::OnCompilerStart(CompilerEvent& event)
{
	//ClearOutput();

	wxFileName fn = event.GetFile();
	wxString filename = fn.GetFullPath(event.IsRemoteFile() ? wxPATH_UNIX : wxPATH_DOS);
	wxString commandLine = event.GetCommandLine();

	wxString compileOutput;
	if(filename == "Linking")
	{
		//special case for Linking
		compileOutput = "Linking:";
	}
	else
	{
		if(m_options->GetShowCompileCommands())
		{
			compileOutput = wxString::Format("Compiling: %s\r\nCompile command: %s", filename, commandLine);
		}
		else
		{
			compileOutput = wxString::Format("Compiling: %s", filename);
		}
	}

	int newRowNum = m_grid->GetNumberRows();
	m_grid->AppendRows(1);

	// merge all three cells in the new row into a single cell
	m_grid->SetCellSize(newRowNum, 0, 1, 3);

	m_grid->SetCellValue(compileOutput, newRowNum, 0);

	*m_textbox << compileOutput << "\r\n";

	m_grid->AutoSizeRow(newRowNum, false);

	m_grid->MakeCellVisible(newRowNum, 0);

}

//////////////////////////////////////////////////////////////////////////////
///  public OnCompilerProblem
///  Displays output from a compiler warning/error event
///
///  @param  event CompilerEvent & The generated compiler event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void CompilerOutputPanel::OnCompilerProblem(CompilerEvent &event)
{
	wxFileName fn = event.GetFile();
	wxString filename = fn.GetFullPath(event.IsRemoteFile() ? wxPATH_UNIX : wxPATH_DOS);

	int linenum = event.GetInt();
	wxString lineString;
	lineString << linenum;
	wxString message = event.GetMessage();
	wxString output = event.GetGCCOutput();
	wxString outputCopy = output;

	wxRegEx reParseOutput("(?:(?:\\A|\\n)(.+?):(\\d+):(?:\\d+:)?(.+))", wxRE_ADVANCED);

	while(outputCopy != wxEmptyString)
	{
		wxString line = outputCopy.BeforeFirst('\r');
		outputCopy = outputCopy.AfterFirst('\n');

		if(reParseOutput.Matches(line))
		{
			m_numErrors++;
			size_t start = 0;
			size_t length = 0;
			int counter = 1;

			wxString parsedFile = reParseOutput.GetMatch(line, 1);
			wxString parsedLine = reParseOutput.GetMatch(line, 2);
			wxString parsedMessage = reParseOutput.GetMatch(line, 3);

			//if(m_isAdvanced)
			//{
				int newRowNum = m_grid->GetNumberRows();
				m_grid->AppendRows(1);

				m_grid->SetCellValue(parsedFile, newRowNum, 0);
				m_grid->SetCellValue(parsedLine, newRowNum, 1);
				m_grid->SetCellValue(parsedMessage, newRowNum, 2);
				m_grid->SetCellRenderer(newRowNum , 2, new wxGridCellAutoWrapStringRenderer);
			//}
			//else
			//{
			//	*m_textbox << parsedFile << ":" << parsedLine << ":" << parsedMessage << "\r\n";
			//}
		}
	}

	*m_textbox << output;
}

//////////////////////////////////////////////////////////////////////////////
///  public OnCompilerEnd
///  Displays output from a "compile ended" event
///
///  @param  event CompilerEvent & The generated compiler event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void CompilerOutputPanel::OnCompilerEnd(CompilerEvent &event)
{
	wxFileName fn = event.GetFile();
	wxString filename = fn.GetFullPath(event.IsRemoteFile() ? wxPATH_UNIX : wxPATH_DOS);
	CompileResult cr = event.GetResult();

	wxString compileResult;
	switch(cr)
	{
		case CR_OK:
			compileResult = filename + " compiled successfully.";
			break;
		case CR_ERROR:
		{
			wxString errorResult;
			errorResult.Printf("Compilation failed. %d total errors / warnings.", m_numErrors);
			compileResult = errorResult;
			break;
		}
		case CR_TERMINATED:
			compileResult = "Compilation terminated by user.";
			break;
	}

	//if(m_isAdvanced)
	//{
		int newRowNum = m_grid->GetNumberRows();
		m_grid->AppendRows(1);

		// merge all three cells in the new row into a single cell
		m_grid->SetCellSize(newRowNum, 0, 1, 3);

		m_grid->SetCellValue(compileResult, newRowNum, 0);
	//}
	//else
	//{
		*m_textbox << "\r\n" << compileResult;
	//}
}

//////////////////////////////////////////////////////////////////////////////
///  public OnGridDoubleClick
///  Tells the main window to focus on the appropriate line when an item in the grid is double-clicked
///
///  @param  event wxGridEvent & The generated grid event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void CompilerOutputPanel::OnGridDoubleClick(wxGridEvent &event)
{
	int rownum = event.GetRow();
	int cellSizeRows = 0;
	int cellSizeCols = 0;
	m_grid->GetCellSize(rownum, 0, &cellSizeRows, &cellSizeCols);

	// if this is greater than one, it's a non-problem row
	if(cellSizeCols == 1)
	{
		wxString filename = m_grid->GetCellValue(rownum, 0);
		wxString linestring = m_grid->GetCellValue(rownum, 1);
		long linenum = 0;
		linestring.ToLong(&linenum);

		m_mainFrame->FocusOnLine(filename, (int)linenum, false);
	}

}


//////////////////////////////////////////////////////////////////////////////
///  public virtual SetFocus
///  Ensures that the right widget is focused
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void CompilerOutputPanel::SetFocus()
{
	if(m_isAdvanced)
	{
		m_grid->SetFocus();
		m_grid->Refresh();
	}
	else
	{
		m_textbox->SetFocus();
		m_textbox->Refresh();
	}
}

/*!
 * Get bitmap resources
 */

wxBitmap CompilerOutputPanel::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CompilerOutputPanel bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end CompilerOutputPanel bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CompilerOutputPanel::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CompilerOutputPanel icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CompilerOutputPanel icon retrieval
}
