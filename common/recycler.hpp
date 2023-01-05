#ifndef RECYCLER_HPP
#define RECYCLER_HPP

#include <windows.h>
#include <shellapi.h>

class Recycler : public SHFILEOPSTRUCTA
{
	public:
		Recycler();

		int recycle(const char* pszPath, bool bDelete = false);
};

#endif // RECYCLER_HPP

