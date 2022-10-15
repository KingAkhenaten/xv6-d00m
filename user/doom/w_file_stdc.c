//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	WAD I/O functions.
//

// Note: xv6 has read/write, but no syscall to seek in that file.
// Instead we push the entire WAD into RAM and handle seeking ourselves.

#include "xv6.h"

#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"

typedef struct
{
    wad_file_t wad;
    // FILE *fstream;
	int fd;
	char * wad_data;
	int wad_seek;
} stdc_wad_file_t;

extern wad_file_class_t stdc_wad_file;

static wad_file_t *W_StdC_OpenFile(char *path)
{
    stdc_wad_file_t *result;
    int fd;

    fd = open(path, O_RDONLY);

    if (fd == -1)
    {
        return NULL;
    }

    // Create a new stdc_wad_file_t to hold the file handle.

    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &stdc_wad_file;
    result->wad.mapped = NULL;
    result->wad.length = M_FileLength(fd);
    result->fd = fd;
	// allocate buffer
	result->wad_data = malloc(result->wad.length);
	if (result->wad_data == NULL) {
		Z_Free(result);
		return NULL;
	}
	// read everything
	if (read(fd,result->wad_data,result->wad.length) != result->wad.length) {
		free(result->wad_data);
		Z_Free(result);
		return NULL;
	}
    return &result->wad;
}

static void W_StdC_CloseFile(wad_file_t *wad)
{
    stdc_wad_file_t *stdc_wad;

    stdc_wad = (stdc_wad_file_t *) wad;

    // fclose(stdc_wad->fstream);
	close(stdc_wad->fd);
    Z_Free(stdc_wad);
}

// Read data from the specified position in the file into the 
// provided buffer.  Returns the number of bytes read.

size_t W_StdC_Read(wad_file_t *wad, unsigned int offset,
                   void *buffer, size_t buffer_len)
{
    stdc_wad_file_t *stdc_wad;
    size_t result;

    stdc_wad = (stdc_wad_file_t *) wad;

    // Jump to the specified position in the file.

    // fseek(stdc_wad->fstream, offset, SEEK_SET);
	stdc_wad->wad_seek = offset;

    // Read into the buffer.

    // result = fread(buffer, 1, buffer_len, stdc_wad->fstream);
	result = buffer_len;
	memmove(buffer,&stdc_wad->wad_data[stdc_wad->wad_seek],buffer_len);
    return result;
}


wad_file_class_t stdc_wad_file = 
{
    W_StdC_OpenFile,
    W_StdC_CloseFile,
    W_StdC_Read,
};


