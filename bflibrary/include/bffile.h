#ifndef BFLIBRARY_BFFILE_H_
#define BFLIBRARY_BFFILE_H_

#include <stdio.h>

#define INVALID_FILE NULL

typedef FILE * TbFileHandle;

#define Lb_FILE_SEEK_BEGINNING SEEK_SET
#define Lb_FILE_SEEK_CURRENT SEEK_CUR
#define Lb_FILE_SEEK_END SEEK_END

#define Lb_FILE_MODE_READ_ONLY "rb"
#define Lb_FILE_MODE_NEW "wb"
#define Lb_FILE_MODE_OLD "r+b"

#define Lb_SUCCESS ERR_OK
#define Lb_FAIL ERR_CANT_OPEN

#define LbFileOpen(fname, accmode) fopen(fname, accmode)
#define LbFileSeek(fhandle, offset, origin) fseek(fhandle, offset, origin)
#define LbFileRead(fhandle, buffer, len) fread(buffer, 1, len, fhandle)
#define LbFileWrite(fhandle, buffer, len) fwrite(buffer, 1, len, fhandle)
#define LbFileClose(fhandle) fclose(fhandle)
#define LbFilePosition(fhandle) ftell(fhandle)

#endif
