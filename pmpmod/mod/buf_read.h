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


#ifndef buf_read_h
#define buf_read_h


#include <pspiofilemgr.h>


struct buf_read_struct
	{
	SceUID        f;
	unsigned int  file_size;
	unsigned int  maximum_read_size;
	unsigned int  block_size;
	unsigned int  padding;
	void         *buf;
	unsigned int  buf_position;
	};


void buf_read_safe_constructor(struct buf_read_struct *p);
char *buf_read_open(struct buf_read_struct *p, unsigned int maximum_read_size, unsigned int padding, char *s);
void buf_read_close(struct buf_read_struct *p);
char *buf_read_get(struct buf_read_struct *p, unsigned int position, unsigned int length, void **buf);
int contained(unsigned int position, unsigned int length, unsigned int position_to_test, unsigned int length_to_test);


#endif
