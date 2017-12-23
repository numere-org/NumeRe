#include <windows.h>
/*
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
*/
//
//  WinMessageBeep() - This function simply calls MessageBeep.
//    It exists (and is in a separate file) so that we don't have
//    to include windows.h in the wxWindows code.
//
void
WinMessageBeep()
{
  MessageBeep(MB_ICONEXCLAMATION);
}

