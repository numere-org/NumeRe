#ifndef CHAMELEON_PRINTOUT__H
#define CHAMELEON_PRINTOUT__H


#include "wx/print.h"

class NumeReEditor;
class Options;

/**
 * This class was swiped almost entirely from wxGuide, and only had a couple slight changes made.
 * Basically, it works, and that's all I need to know.
 */
class NumeRePrintout: public wxPrintout
{

public:

	//! constructor
	NumeRePrintout (NumeReEditor *edit, Options* options, wxChar *title = _T(""));

	//! event handlers
	bool OnPrintPage (int page);
	bool OnBeginDocument (int startPage, int endPage);
	void OnEndDocument();

	//! print functions
	bool HasPage (int page);
	void GetPageInfo (int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);

private:
	NumeReEditor *m_edit;
	int m_printed;
	wxRect m_pageRect;
	wxRect m_printRect;
	Options* m_options;
	wxArrayInt m_pages;

	bool PrintScaling (wxDC *dc);
};



#endif
