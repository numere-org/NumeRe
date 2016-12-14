#ifndef DEBUG__H
#define DEBUG__H

#include <wx/wx.h>
#include <wx/log.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#else
#define DEBUG_NEW new
#endif

//#define DEBUGLOG(x) wxString logstring; logstring << __FILE__ << ":" << __LINE__ << ": " << x; ::wxLogDebug(logstring);
//#define DEBUGLOG(x) wxLogDebug(wxT("%s(%d): %s"),__TFILE__, __LINE__, x);




#endif