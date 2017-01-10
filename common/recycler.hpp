#ifndef RECYCLER_HPP
#define RECYCLER_HPP

#include <windows.h>
#include <shellapi.h>

class Recycler : public SHFILEOPSTRUCT
{
    public:
        Recycler();

        int recycle(const char* psPath, bool bDelete = false);
};

#endif // RECYCLER_HPP

