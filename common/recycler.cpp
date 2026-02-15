#include "recycler.hpp"
#include <tchar.h>

Recycler::Recycler()
{
	memset((SHFILEOPSTRUCTW*)this, 0, sizeof(SHFILEOPSTRUCTW));
	fFlags |= FOF_SILENT;
	fFlags |= FOF_NOERRORUI;
	fFlags |= FOF_NOCONFIRMATION;
}

int Recycler::recycle(const wchar_t* pszPath, bool bDelete)
{
	wchar_t buf[_MAX_PATH + 1];
	wcscpy(buf, pszPath);
	buf[wcslen(buf) + 1] = 0;

	wFunc = FO_DELETE;
	pFrom = buf;
	pTo = nullptr;

	if (bDelete)
		fFlags &= ~FOF_ALLOWUNDO;
	else
		fFlags |= FOF_ALLOWUNDO;

	return SHFileOperationW(this);
}

