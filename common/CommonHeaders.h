#ifndef PRECOMPILEDHEADER_H
#define PRECOMPILEDHEADER_H



#include <wx/wx.h>
#include <wx/string.h>
#include <wx/wxprec.h>
#include <wx/fdrepdlg.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/timer.h>
#include <wx/regex.h>
#include <wx/splash.h>
#include <wx/utils.h>
#include <wx/wfstream.h>
#include <wx/frame.h>
#include <wx/toolbar.h>
#include <wx/bitmap.h>
#include <wx/frame.h>
#include <wx/string.h>
#include <wx/filedlg.h>
#include <wx/file.h>
#include <wx/textfile.h>
#include <wx/splitter.h>
#include <wx/grid.h>
#include <wx/dynarray.h>
#include <wx/notebook.h>
//#include <wx/tabctrl.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/socket.h>
#include <wx/checklst.h>
#include <wx/treectrl.h>
#include <wx/mstream.h>


#if (_MSC_VER >= 1400)       // VC8+
#pragma warning(disable : 4996)    // Either disable all deprecation warnings,
// Or just turn off warnings about the newly deprecated CRT functions.
// #ifndef _CRT_SECURE_NO_DEPRECATE
// #define _CRT_SECURE_NO_DEPRECATE
// #endif
// #ifndef _CRT_NONSTDC_NO_DEPRECATE
// #define _CRT_NONSTDC_NO_DEPRECATE
// #endif



#if defined(__WXMSW__) && !defined(__WXWINCE__)
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df'\"")
#endif

#endif // VC8

#endif
