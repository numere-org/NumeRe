#include "recycler.hpp"
#include <tchar.h>

Recycler::Recycler()
{
	memset((SHFILEOPSTRUCT*)this, 0, sizeof(SHFILEOPSTRUCT));
	fFlags |= FOF_SILENT;
	fFlags |= FOF_NOERRORUI;
	fFlags |= FOF_NOCONFIRMATION;
}

int Recycler::recycle(const char* pszPath, bool bDelete)
{
	TCHAR buf[_MAX_PATH + 1];
	strcpy(buf, pszPath);
	buf[strlen(buf) + 1] = 0;

	wFunc = FO_DELETE;
	pFrom = buf;
	pTo = nullptr;

	if (bDelete)
	{
		fFlags &= ~FOF_ALLOWUNDO;
	}
	else
	{
		fFlags |= FOF_ALLOWUNDO;
	}
	return SHFileOperation(this);
}

