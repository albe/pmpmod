/*
PMP Mod
Copyright (C) 2006 jonny

Homepage: http://jonny.leffe.dnsalias.com
E-mail:   jonny@leffe.dnsalias.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
buffered io (optimized for psp)
*/


#include <string.h>
#include "buf_read.h"
#include "mem64.h"


void buf_read_safe_constructor(struct buf_read_struct *p)
	{
	p->f   = -1;
	p->buf = 0;
	}


void buf_read_close(struct buf_read_struct *p)
	{
    if (!(p->f < 0)) sceIoClose(p->f);
	if (p->buf != 0) free_64(p->buf);


	buf_read_safe_constructor(p);
	}


static char *buf_read_get_block(struct buf_read_struct *p, unsigned int position)
	{
	p->buf_position = position & 0xffff8000;


	if (sceIoLseek32(p->f, p->buf_position, PSP_SEEK_SET) != p->buf_position) return("buf_read_get_block: seek failed");


	unsigned int maximum_read_size = p->file_size - p->buf_position;
	if (maximum_read_size > p->block_size) maximum_read_size = p->block_size;


	if (sceIoRead(p->f, p->buf, maximum_read_size) != maximum_read_size) return("buf_read_get_block: read failed");


	return(0);
	}


char *buf_read_open(struct buf_read_struct *p, unsigned int maximum_read_size, unsigned int padding, char *s)
	{
	buf_read_safe_constructor(p);




	p->f = sceIoOpen(s, PSP_O_RDONLY, 0777);
	if (p->f < 0)
		{
		buf_read_close(p);
		return("buf_read_open: can't open file");
		}


	p->file_size         = sceIoLseek32(p->f, 0, PSP_SEEK_END);
	p->maximum_read_size = maximum_read_size;
	p->padding           = padding;


	unsigned int mod = maximum_read_size & 0xffff;

	if (mod != 0) maximum_read_size += 0x10000 - mod;

	p->block_size = maximum_read_size + 0x10000;




	p->buf = malloc_64(p->block_size + p->padding);
	if (p->buf == 0)
		{
		buf_read_close(p);
		return("buf_read_open: malloc_64 failed on buf");
		}
	memset(p->buf, 0, p->block_size + p->padding);




	char *buf_read_get_block_result = buf_read_get_block(p, 0);
	if (buf_read_get_block_result != 0)
		{
		buf_read_close(p);
		return(buf_read_get_block_result);
		}


	return(0);
	}


int contained(unsigned int position, unsigned int length, unsigned int position_to_test, unsigned int length_to_test)
	{
	if (position_to_test < position) return(0);

	if (position_to_test + length_to_test > position + length) return(0);

	return(1);
	}


char *buf_read_get(struct buf_read_struct *p, unsigned int position, unsigned int length, void **buf)
	{
	if (length == 0)
		{
		*buf = p->buf;
		return(0);
		}


	if (length > p->maximum_read_size) return("buf_read_get: length > maximum_read_size");


	if (!contained(0, p->file_size, position, length)) return("buf_read_get: out of range");


	if (!contained(p->buf_position, p->block_size, position, length))
		{
		char *buf_read_get_block_result = buf_read_get_block(p, position);
		if (buf_read_get_block_result != 0) return(buf_read_get_block_result);
		}


	position = position - p->buf_position;
	*buf = p->buf + position;


	return(0);
	}
