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
basic layer for reading av frames
*/


#ifndef pmp_read_h
#define pmp_read_h


#include "pmp_lib.h"
#include "buf_read.h"
#include "time_math.h"


struct pmp_read_struct
	{
	struct pmp_lib_struct header;

	struct buf_read_struct video_reader;
	struct buf_read_struct audio_reader;

	unsigned int maximum_number_of_audio_frames;


	unsigned int current_video_frame;
	unsigned int current_video_position;

	unsigned int current_audio_frame;
	unsigned int current_audio_position;


	struct time_math_interleaving_struct interleaving;
	};


void pmp_read_safe_constructor(struct pmp_read_struct *p);
char *pmp_read_open(struct pmp_read_struct *p, unsigned int padding, char *s);
void pmp_read_close(struct pmp_read_struct *p);
char *pmp_read_get(struct pmp_read_struct *p, unsigned int frame_number, int audio_delay, void **video_buf, void **audio_buf);


#endif
