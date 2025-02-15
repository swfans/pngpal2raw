/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet, Genewars or Dungeon Keeper.
/******************************************************************************/
/** @file bfmemut.h
 *     Simplified interface from bflibrary.
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef BFLIBRARY_BFMEMUT_H_
#define BFLIBRARY_BFMEMUT_H_

#include <string.h>

#define LbMemorySet(dst, c, size) memset(dst, c, size)

#define LbMemoryCopy(in_dst, in_src, size) memcpy(in_dst, in_src, size)

#define LbMemoryMove(in_dst, in_src, size) memmove(in_dst, in_src, size)

enum {
    ERR_OK          =  0,
    ERR_CANT_OPEN   = -1, // fopen problem
    ERR_BAD_FILE    = -2, // incorrect file format
    ERR_NO_MEMORY   = -3, // malloc error
    ERR_FILE_READ   = -4, // fget/fread/fseek error
    ERR_FILE_WRITE  = -5, // fput/fwrite error
    ERR_LIMIT_EXCEED= -6, // static limit exceeded
};

#endif // BFLIBRARY_BFMEMUT_H_
/******************************************************************************/
