// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#define __WXMSW__
// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "ProfOptionsDialog.h"



// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Define the application class
class MyApp : public wxApp
{
public:
	// Init method
	virtual bool OnInit();
};



IMPLEMENT_APP(MyApp)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

bool MyApp::OnInit()
{
	// Create the main application window (a dialog in this case)
	// NOTE: Vertical dimension comprises the caption bar.
	//       Horizontal dimension has to take into account the thin
	//       hilighting border around the dialog (2 points in
	//       Win 95).
	ProfOptionsDialog *dialog = new ProfOptionsDialog((wxDialog*)NULL, -1, "Chameleon Options Manager",
		wxPoint(0, 0), wxSize(400, 300));

	// Center the dialog when first shown
	dialog->Centre();

	// Show it and tell the application that it's our main window
	dialog->Show(TRUE);
	SetTopWindow(dialog);

	return TRUE;
}
