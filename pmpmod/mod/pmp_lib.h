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
.pmp header reading routines
*/


#ifndef pmp_lib_h
#define pmp_lib_h


#include <stdio.h>


struct pmp_lib_vid_struct
	{
	unsigned int pmpm;
	unsigned int version;
	unsigned int number_of_frames;
	unsigned int width;
	unsigned int height;
	unsigned int scale;
	unsigned int rate;
	unsigned int maximum_frame_size;
	};


struct pmp_lib_aud_struct
	{
	unsigned int frame_size;
	unsigned int number_of_frames;
	};


struct pmp_lib_struct
	{
	FILE                      *f_vid;
	
	
	struct pmp_lib_vid_struct  vid;
	unsigned int              *vid_index;
	unsigned int               vid_start;


	struct pmp_lib_aud_struct  aud;
	unsigned char             *aud_index;
	unsigned int               aud_start;
	};


void pmp_lib_safe_constructor(struct pmp_lib_struct *p);
char *pmp_lib_open(struct pmp_lib_struct *p, char *s);
void pmp_lib_close(struct pmp_lib_struct *p);


#endif
