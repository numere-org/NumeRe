#include "NumeRePrintout.h"
#include "../editor/editor.h"
#include "wx/printdlg.h"  // wxPageSetupDialog
#include "wx/progdlg.h"   // wxProgressDialog
#include "../common/Options.h"

#include "debug.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern wxPrintData *g_printData;
extern wxPageSetupData *g_pageSetupData;

NumeRePrintout::NumeRePrintout (NumeReEditor *edit, Options* options, const wxString& title)
: wxPrintout(title) {
	m_edit = edit;
	m_printed = 0;
	m_options = options;

}

//////////////////////////////////////////////////////////////////////////////
///  public OnPrintPage
///  Called by the wxWidgets printing code as needed
///
///  @param  page int  The page to print
///
///  @return bool Whether or not the print succeeded
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeRePrintout::OnPrintPage (int page) {

	wxDC *dc = GetDC();
	if (!dc) return false;

	// scale DC
	PrintScaling (dc);

	// TODO Need to figure out how to preview the appropriate page...
	// right now it just does from the start
	// print page
	if (page == 1) m_printed = 0;
	m_printed = m_edit->FormatRange (1,
		//(int(m_pages.GetCount()) > page) ? m_pages[page - 1] : 0,
		//m_edit->GetLength(),
		m_pages[page-1],
		(int(m_pages.GetCount()) > page) ? m_pages[page] : m_edit->GetLength(),
		dc, dc, m_printRect, m_pageRect);

	return true;
}

//////////////////////////////////////////////////////////////////////////////
///  public OnBeginDocument
///  A setup called by the wxWidgets printing code
///
///  @param  startPage int  The starting page
///  @param  endPage   int  The ending page
///
///  @return bool      Whether or not the setup succeeded
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeRePrintout::OnBeginDocument (int startPage, int endPage) {

	m_printed = 0;


	if(!m_options->GetLineNumberPrinting())
	{
		// Hide line numbers until after printing is finished
		m_edit->SetMarginWidth(0, 0);
		m_edit->SetMarginWidth(1, 0);
	}


	return wxPrintout::OnBeginDocument(startPage, endPage);


	/*
	if (!wxPrintout::OnBeginDocument (startPage, endPage)) {
		return false;
	}

	return true;
	*/
}

//////////////////////////////////////////////////////////////////////////////
///  public GetPageInfo
///  Returns various formatting info needed by wxWidgets
///
///  @param  minPage     int * Filled with some minimum number of pages
///  @param  maxPage     int * Filled with some maximum number of pages
///  @param  selPageFrom int * Filled with something
///  @param  selPageTo   int * Filled with something
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void NumeRePrintout::GetPageInfo (int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) {

	// initialize values
	*minPage = 0;
	*maxPage = 0;
	*selPageFrom = 0;
	*selPageTo = 0;

	m_pages.Clear();

	// scale DC if possible
	wxDC *dc = GetDC();
	if (!dc) return;
	PrintScaling (dc);

	// get print page informations and convert to printer pixels
	wxSize ppiScr;
	GetPPIScreen (&ppiScr.x, &ppiScr.y);
	wxSize page = g_pageSetupData->GetPaperSize();
	page.x = static_cast<int> (page.x * ppiScr.x / 25.4);
	page.y = static_cast<int> (page.y * ppiScr.y / 25.4);
	m_pageRect = wxRect (0,
		0,
		page.x,
		page.y);

	// get margins informations and convert to printer pixels
	int  top = 25; // default 25
	int  bottom = 25; // default 25
	int  left = 20; // default 20
	int  right = 20; // default 20
	wxPoint (top, left) = g_pageSetupData->GetMarginTopLeft();
	wxPoint (bottom, right) = g_pageSetupData->GetMarginBottomRight();
	top = static_cast<int> (top * ppiScr.y / 25.4);
	bottom = static_cast<int> (bottom * ppiScr.y / 25.4);
	left = static_cast<int> (left * ppiScr.x / 25.4);
	right = static_cast<int> (right * ppiScr.x / 25.4);
	m_printRect = wxRect (left,
		top,
		page.x - (left + right),
		page.y - (top + bottom));

	int length = m_edit->GetLength();
	// count pages
	//while (HasPage (*maxPage)) {
	while(m_printed < length)
	{
		m_pages.Add(m_printed);
		m_printed = m_edit->FormatRange (0, m_printed, length,
			dc, dc, m_printRect, m_pageRect);
		*maxPage += 1;

	}
	if (*maxPage > 0) *minPage = 1;
	*selPageFrom = *minPage;
	*selPageTo = *maxPage;
}

//////////////////////////////////////////////////////////////////////////////
///  public HasPage
///  Checks to see if a given page exists
///
///  @param  page int  The page number to check
///
///  @return bool Whether or not the page exists
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeRePrintout::HasPage (int page) {

	//return (m_printed < m_edit->GetLength());
	return (page > 0) && (page-1 < int(m_pages.GetCount())); // pages start at 1
}

//////////////////////////////////////////////////////////////////////////////
///  private PrintScaling
///  Scales the output (for print preview, etc)
///
///  @param  dc   wxDC * The wxDC to use for calculations
///
///  @return bool Whether or not the function succeeded
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeRePrintout::PrintScaling (wxDC *dc){

	// check for dc, return if none
	if (!dc) return false;

	// get printer and screen sizing values
	wxSize ppiScr;
	GetPPIScreen (&ppiScr.x, &ppiScr.y);
	if (ppiScr.x == 0) { // most possible guess 96 dpi
		ppiScr.x = 96;
		ppiScr.y = 96;
	}
	wxSize ppiPrt;
	GetPPIPrinter (&ppiPrt.x, &ppiPrt.y);
	if (ppiPrt.x == 0) { // scaling factor to 1
		ppiPrt.x = ppiScr.x;
		ppiPrt.y = ppiScr.y;
	}
	wxSize dcSize = dc->GetSize();
	wxSize pageSize;
	GetPageSizePixels (&pageSize.x, &pageSize.y);

	// set user scale
	float scale_x = (float)(ppiPrt.x * dcSize.x) /
		(float)(ppiScr.x * pageSize.x);
	float scale_y = (float)(ppiPrt.y * dcSize.y) /
		(float)(ppiScr.y * pageSize.y);
	dc->SetUserScale (scale_x, scale_y);

	return true;
}

void NumeRePrintout::OnEndDocument()
{
	// Display line numbers now that printing's done.  If they were already displayed? No problem.
	m_edit->SetMarginWidth(0, 40);
	m_edit->SetMarginWidth(1, 16);

	wxPrintout::OnEndDocument();
}

