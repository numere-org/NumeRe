// This file contains an example of reading a wave file.
#include "IgorLib/ReadWave.h"

static void
ReorderBytes(void *p, int bytesPerPoint, long numValues)        // Reverses byte order.
{
        char ch, *p1, *p2;
        unsigned char *pEnd;

        pEnd = (unsigned char *)p + numValues*bytesPerPoint;
        while (p < (void *)pEnd) {
                p1 = (char *) p;
                p2 = (char *)p + bytesPerPoint-1;
                while (p1 < p2) {
                        ch = *p1;
                        *p1++ = *p2;
                        *p2-- = ch;
                }
                p = (unsigned char *)p + bytesPerPoint;
        }
}

static void
ReorderShort(void* sp)
{
        ReorderBytes(sp, 2, 1);
}

static void
ReorderLong(void* lp)
{
        ReorderBytes(lp, 4, 1);
}

static void
ReorderDouble(void* dp)
{
        ReorderBytes(dp, 8, 1);
}

static void
ReorderBinHeader1(BinHeader1* p)
{
        ReorderShort(&p->version);
        ReorderLong(&p->wfmSize);
        ReorderShort(&p->checksum);
}

static void
ReorderBinHeader2(BinHeader2* p)
{
        ReorderShort(&p->version);
        ReorderLong(&p->wfmSize);
        ReorderLong(&p->noteSize);
        ReorderLong(&p->pictSize);
        ReorderShort(&p->checksum);
}

static void
ReorderBinHeader3(BinHeader3* p)
{
        ReorderShort(&p->version);
        ReorderLong(&p->wfmSize);
        ReorderLong(&p->noteSize);
        ReorderLong(&p->formulaSize);
        ReorderLong(&p->pictSize);
        ReorderShort(&p->checksum);
}

static void
ReorderBinHeader5(BinHeader5* p)
{
        ReorderShort(&p->version);
        ReorderShort(&p->checksum);
        ReorderLong(&p->wfmSize);
        ReorderLong(&p->formulaSize);
        ReorderLong(&p->noteSize);
        ReorderLong(&p->dataEUnitsSize);
        ReorderBytes(&p->dimEUnitsSize, 4, 4);
        ReorderBytes(&p->dimLabelsSize, 4, 4);
        ReorderLong(&p->sIndicesSize);
        ReorderLong(&p->optionsSize1);
        ReorderLong(&p->optionsSize2);
}

static void
ReorderWaveHeader2(WaveHeader2* p)
{
        ReorderShort(&p->type);
        ReorderLong(&p->next);
        // char bname does not need to be reordered.
        ReorderShort(&p->whVersion);
        ReorderShort(&p->srcFldr);
        ReorderLong(&p->fileName);
        // char dataUnits does not need to be reordered.
        // char xUnits does not need to be reordered.
        ReorderLong(&p->npnts);
        ReorderShort(&p->aModified);
        ReorderDouble(&p->hsA);
        ReorderDouble(&p->hsB);
        ReorderShort(&p->wModified);
        ReorderShort(&p->swModified);
        ReorderShort(&p->fsValid);
        ReorderDouble(&p->topFullScale);
        ReorderDouble(&p->botFullScale);
        // char useBits does not need to be reordered.
        // char kindBits does not need to be reordered.
        ReorderLong(&p->formula);
        ReorderLong(&p->depID);
        ReorderLong(&p->creationDate);
        // char wUnused does not need to be reordered.
        ReorderLong(&p->modDate);
        ReorderLong(&p->waveNoteH);
        // The wData field marks the start of the wave data which will be reordered separately.
}

static void
ReorderWaveHeader5(WaveHeader5* p)
{
        ReorderLong(&p->next);
        ReorderLong(&p->creationDate);
        ReorderLong(&p->modDate);
        ReorderLong(&p->npnts);
        ReorderShort(&p->type);
        ReorderShort(&p->dLock);
        // char whpad1 does not need to be reordered.
        ReorderShort(&p->whVersion);
        // char bname does not need to be reordered.
        ReorderLong(&p->whpad2);
        ReorderLong(&p->dFolder);
        ReorderBytes(&p->nDim, 4, 4);
        ReorderBytes(&p->sfA, 8, 4);
        ReorderBytes(&p->sfB, 8, 4);
        // char dataUnits does not need to be reordered.
        // char dimUnits does not need to be reordered.
        ReorderShort(&p->fsValid);
        ReorderShort(&p->whpad3);
        ReorderDouble(&p->topFullScale);
        ReorderDouble(&p->botFullScale);
        ReorderLong(&p->dataEUnits);
        ReorderBytes(&p->dimEUnits, 4, 4);
        ReorderBytes(&p->dimLabels, 4, 4);
        ReorderLong(&p->waveNoteH);
        ReorderBytes(&p->whUnused, 4, 16);
        ReorderShort(&p->aModified);
        ReorderShort(&p->wModified);
        ReorderShort(&p->swModified);
        // char useBits does not need to be reordered.
        // char kindBits does not need to be reordered.
        ReorderLong(&p->formula);
        ReorderLong(&p->depID);
        ReorderShort(&p->whpad4);
        ReorderShort(&p->srcFldr);
        ReorderLong(&p->fileName);
        ReorderLong(&p->sIndices);
        // The wData field marks the start of the wave data which will be reordered separately.
}

static int
Checksum(short *data, int needToReorderBytes, int oldcksum, int numbytes)
{
        unsigned short s;

        numbytes >>= 1;                         // 2 bytes to a short -- ignore trailing odd byte.
        while(numbytes-- > 0) {
                s = *data++;
                if (needToReorderBytes)
                        ReorderShort(&s);
                oldcksum += s;
        }
        return oldcksum&0xffff;
}

/*      NumBytesPerPoint(int type)

        Given a numeric wave type, returns the number of data bytes per point.
*/
static int
NumBytesPerPoint(int type)
{
        int numBytesPerPoint;

        // Consider the number type, not including the complex bit or the unsigned bit.
        switch(type & ~(NT_CMPLX | NT_UNSIGNED)) {
                case NT_I8:
                        numBytesPerPoint = 1;           // char
                        break;
                case NT_I16:
                        numBytesPerPoint = 2;           // short
                        break;
                case NT_I32:
                        numBytesPerPoint = 4;           // long
                        break;
                case NT_FP32:
                        numBytesPerPoint = 4;           // float
                        break;
                case NT_FP64:
                        numBytesPerPoint = 8;           // double
                        break;
                default:
                        return 0;
                        break;
        }

        if (type & NT_CMPLX)
                numBytesPerPoint *= 2;                  // Complex wave - twice as many points.

        return numBytesPerPoint;
}

/*      LoadNumericWaveData(fr, type, npnts, waveDataSize, needToReorderBytes, pp)

        fr is a file reference.
        type is the Igor number type.
        npnts is the total number of elements in all dimensions.
        waveDataSize is the number of data bytes stored in the file.
        needToReorderBytes if the byte ordering of the file is not the byte ordering of the current platform.
        pp is a pointer to a pointer.

        If an error occurs, LoadWaveData returns a non-zero error code and sets
        *pp to NULL.

        If no error occurs, LoadWaveData returns 0 and sets *pp to a pointer allocated
        via malloc. This pointer must be freed by the calling routine.
*/
static int
LoadNumericWaveData(CP_FILE_REF fr, int type, long npnts, unsigned long waveDataSize, int needToReorderBytes, void**pp)
{
        int numBytesPerPoint;
        unsigned long numBytesToRead, numBytesToAllocate;
        unsigned long numBytesRead;
        void* p;
        int err;

        *pp = NULL;                                                     // Assume that we can not allocate memory.

        numBytesPerPoint = NumBytesPerPoint(type);
        if (numBytesPerPoint <= 0) {
                printf("Invalid wave type (0x%x).\n", type);
                return -1;
        }
        numBytesToRead = npnts * numBytesPerPoint;

        numBytesToAllocate = numBytesToRead;
        if (numBytesToAllocate == 0)
                numBytesToAllocate = 8;                 // This is just because malloc refuses to allocate a zero byte block.
        p = malloc(numBytesToAllocate);         // Allocate memory to store the wave data.
        if (p == NULL) {
                printf("Unable to allocate %ld bytes to store data.\n", numBytesToAllocate);
                return -1;
        }
        if (numBytesToRead > 0) {
                if (waveDataSize < numBytesToRead) {
                        /*      If here, this should be a wave governed by a dependency formula
                                for which no wave data was written to the file. Since we can't
                                execute the dependency formula we have no way to recreate the wave's
                                data. Therefore, we return 0 for all points in the wave.
                        */
                        memset(p, 0, numBytesToRead);
                }
                else {
                        if (err = CPReadFile(fr, numBytesToRead, p, &numBytesRead)) {
                                free(p);
                                printf("Error %d occurred while reading the wave data.\n", err);
                                return err;
                        }
                        if (needToReorderBytes) {
                                if (type != 0)                          // Text wave data does not need to be reordered.
                                        ReorderBytes(p, numBytesPerPoint, numBytesToRead/numBytesPerPoint);
                        }
                }
        }

        *pp = p;                                                        // Return the pointer to the calling routine.
        return 0;
}

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
int
ReadWave(CP_FILE_REF fr, int* typePtr, long* npntsPtr, long* ndimptr, double* sfa, double* sfb, void** waveDataPtrPtr, char ** name)
{
        unsigned long startFilePos;
        short version;
        short check;
        int binHeaderSize, waveHeaderSize, checkSumSize;
        unsigned long waveDataSize;
        unsigned long numBytesRead;
        int needToReorderBytes;
        char buffer[512];
        unsigned long modDate;
        long wfmSize;
        long npnts;
        int type;
        int err;
        (* name) = (char *) malloc(64);

        *waveDataPtrPtr = NULL;
        *typePtr = 0;
        *npntsPtr = 0;

        if (err = CPGetFilePosition(fr, &startFilePos))
                return err;

        // Read the file version field.
        if (err = CPReadFile(fr, 2, &version, &numBytesRead)) {
                printf("Error %d occurred while reading the file version.\n", err);
                return err;
        }

        /*      Reorder version field bytes if necessary.
                If the low order byte of the version field of the BinHeader structure
                is zero then the file is from a platform that uses different byte-ordering
                and therefore all data will need to be reordered.
        */
        needToReorderBytes = (version & 0xFF) == 0;
        if (needToReorderBytes)
                ReorderShort(&version);

        // Check the version.
        switch(version) {
                case 1:
                        //printf("This is a version 1 file.\n");
                        binHeaderSize = sizeof(BinHeader1);
                        waveHeaderSize = sizeof(WaveHeader2);
                        checkSumSize = binHeaderSize + waveHeaderSize;
                        break;
                case 2:
                        //printf("This is a version 2 file.\n");
                        binHeaderSize = sizeof(BinHeader2);
                        waveHeaderSize = sizeof(WaveHeader2);
                        checkSumSize = binHeaderSize + waveHeaderSize;
                        break;
                case 3:
                        //printf("This is a version 3 file.\n");
                        binHeaderSize = sizeof(BinHeader3);
                        waveHeaderSize = sizeof(WaveHeader2);
                        checkSumSize = binHeaderSize + waveHeaderSize;
                        break;
                case 5:
                        //printf("This is a version 5 file.\n");
                        binHeaderSize = sizeof(BinHeader5);
                        waveHeaderSize = sizeof(WaveHeader5);
                        checkSumSize = binHeaderSize + waveHeaderSize - 4;      // Version 5 checksum does not include the wData field.
                        break;
                default:
                        //printf("This does not appear to be a valid Igor binary wave file. The version field = %d.\n", version);
                        return -1;
                        break;
        }

        // Load the BinHeader and the WaveHeader into memory.
        CPSetFilePosition(fr, startFilePos, -1);
        if (err = CPReadFile(fr, binHeaderSize+waveHeaderSize, buffer, &numBytesRead)) {
                //printf("Error %d occurred while reading the file headers.\n", err);
                return err;
        }

        // Check the checksum.
        check = Checksum((short*)buffer, needToReorderBytes, 0, checkSumSize);
        if (check != 0) {
                //printf("Error in checksum - should be 0, is %d.\n", check);
                //printf("This does not appear to be a valid Igor binary wave file.\n");
                return -1;
        }

        // Do byte reordering if the file is from another platform.
        if (needToReorderBytes) {
                switch(version) {
                        case 1:
                                ReorderBinHeader1((BinHeader1*)buffer);
                                break;
                        case 2:
                                ReorderBinHeader2((BinHeader2*)buffer);
                                break;
                        case 3:
                                ReorderBinHeader3((BinHeader3*)buffer);
                                break;
                        case 5:
                                ReorderBinHeader5((BinHeader5*)buffer);
                                break;
                }
                switch(version) {
                        case 1:                         // Version 1 and 2 files use WaveHeader2.
                        case 2:
                        case 3:
                                ReorderWaveHeader2((WaveHeader2*)(buffer+binHeaderSize));
                                break;
                        case 5:
                                ReorderWaveHeader5((WaveHeader5*)(buffer+binHeaderSize));
                                break;
                }
        }

        // Read some of the BinHeader fields.
        switch(version) {
                case 1:
                        {
                                BinHeader1* b1;
                                b1 = (BinHeader1*)buffer;
                                wfmSize = b1->wfmSize;
                        }
                        break;

                case 2:
                        {
                                BinHeader2* b2;
                                b2 = (BinHeader2*)buffer;
                                wfmSize = b2->wfmSize;
                        }
                        break;

                case 3:
                        {
                                BinHeader3* b3;
                                b3 = (BinHeader3*)buffer;
                                wfmSize = b3->wfmSize;
                        }
                        break;

                case 5:
                        {
                                BinHeader5* b5;
                                b5 = (BinHeader5*)buffer;
                                wfmSize = b5->wfmSize;
                        }
                        break;
        }

        // Read some of the WaveHeader fields.
        switch(version) {
                case 1:
                case 2:
                case 3:
                        {
                                WaveHeader2* w2;
                                w2 = (WaveHeader2*)(buffer+binHeaderSize);
                                modDate = w2->modDate;
                                npnts = w2->npnts;
                                type = w2->type;
                                ndimptr[0] = npnts;
                                ndimptr[1] = 0;
                                ndimptr[2] = 0;
                                ndimptr[3] = 0;
                                sfa[0] = w2->hsA;
                                sfb[0] = w2->hsB;
                                sfa[1] = 0.0;
                                sfa[2] = 0.0;
                                sfa[3] = 0.0;
                                sfb[1] = 0.0;
                                sfb[2] = 0.0;
                                sfb[3] = 0.0;
                                strcpy((*name), w2->bname);
                        }
                        break;

                case 5:
                        {
                                WaveHeader5* w5;
                                w5 = (WaveHeader5*)(buffer+binHeaderSize);
                                modDate = w5->modDate;
                                npnts = w5->npnts;
                                type = w5->type;
                                ndimptr[0] = w5->nDim[0];
                                ndimptr[1] = w5->nDim[1];
                                ndimptr[2] = w5->nDim[2];
                                ndimptr[3] = w5->nDim[3];
                                sfa[0] = w5->sfA[0];
                                sfa[1] = w5->sfA[1];
                                sfa[2] = w5->sfA[2];
                                sfa[3] = w5->sfA[3];
                                sfb[0] = w5->sfB[0];
                                sfb[1] = w5->sfB[1];
                                sfb[2] = w5->sfB[2];
                                sfb[3] = w5->sfB[3];
                                strcpy((*name), w5->bname);
                        }
                        break;
        }
        //printf("Wave name=%s, npnts=%ld, type=0x%x, wfmSize=%ld.\n", *name, npnts, type, wfmSize);

        // Return information to the calling routine.
        *typePtr = type;
        *npntsPtr = npnts;

        // Determine the number of bytes of wave data in the file.
        switch(version) {
                case 1:
                case 2:
                case 3:
                        waveDataSize = wfmSize - offsetof(WaveHeader2, wData) - 16;
                        break;
                case 5:
                        waveDataSize = wfmSize - offsetof(WaveHeader5, wData);
                        break;
        }

        // Position the file pointer to the start of the wData field.
        switch(version) {
                case 1:
                case 2:
                case 3:
                        CPSetFilePosition(fr, startFilePos+binHeaderSize+waveHeaderSize-16, -1);        // 16 = size of wData field in WaveHeader2 structure.
                        break;
                case 5:
                        CPSetFilePosition(fr, startFilePos+binHeaderSize+waveHeaderSize-4, -1);         // 4 = size of wData field in WaveHeader2 structure.
                        break;
        }

        if (type == 0) {
                // For simplicity, we don't load text wave data in this example program.
                //printf("This is a text wave.\n");
                return 0;
        }

        // Load the data and allocates memory to store it.
        if (err = LoadNumericWaveData(fr, type, npnts, waveDataSize, needToReorderBytes, waveDataPtrPtr))
                return err;

        return 0;
}


/*void doMatlabWrite (int npnts, float * data, char * name, MATFILE *mf)
{
        int i;
        double * buffer;

        buffer = (double *) malloc(sizeof(float) * 2 * npnts);
        for (i = 0; i < npnts; i++) {
                buffer[i] = (double) data[i];
        }

        matfile_addmatrix(mf, name, buffer, npnts, 1 , 0);
}


int loadIntoMatfile(const char* filePath, MATFILE *mf);
int
loadIntoMatfile(const char* filePath, MATFILE *mf)
{
        CP_FILE_REF fr;
        int type;
        long npnts;
        double* dbl_npoints;
        void* waveDataPtr;
        int err;
        int index;
        float * data;
        char * name;

        // Open ibw file for reading.
        if (err = CPOpenFile(filePath, 0, &fr)) {
                printf("Error %d occurred while opening the file.\n", err);
                return err;
        }

        err = ReadWave(fr, &type, &npnts, &waveDataPtr, &name);

        // Here you would do something with the data.
        data  = (float *) waveDataPtr;
        doMatlabWrite(npnts, data, name, mf);

        // Close file, clean up pointer.
        CPCloseFile(fr);

        if (waveDataPtr != NULL)
                free(waveDataPtr);
        return err;
}*/
