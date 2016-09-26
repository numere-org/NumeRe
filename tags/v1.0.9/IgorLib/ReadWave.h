#ifndef READWAVE_H
#define READWAVE_H

#include <stdio.h>
#include <tchar.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "IgorLib/CrossPlatformFileIO.h"
#include "IgorLib/IgorBin.h"


/*      ReadWave(fr, typePtr, npntsPtr, waveDataPtrPtr)

        Reads the wave file and prints some information about it.

        Returns to the calling routine the wave's type, number of points, and the
        wave data. The calling routine must free *waveDataPtrPtr if it is
        not null.

        Returns 0 or an error code.

        This routine is written such that it could be used to read waves
        from an Igor packed experiment file as well as from a standalone
        Igor binary wave file. In order to achieve this, we must not assume
        that the wave is at the start of the file. We do assume that, on entry
        to this routine, the file position is at the start of the wave.
*/
int ReadWave(CP_FILE_REF fr, int* typePtr, long* npntsPtr, long* ndimptr, double* sfa, double* sfb, void** waveDataPtrPtr, char ** name);





#endif // READWAVE_H

