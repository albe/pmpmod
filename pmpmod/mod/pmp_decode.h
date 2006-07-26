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
av decoding in a ring buffer
*/


#ifndef pmp_decode_h
#define pmp_decode_h


#include <pspkernel.h>
#include "pmp_read.h"
#include "avcodec.h"
#include "mem64.h"
#include "csc.h"
#include "me.h"
#include "me_csc.h"


#define maximum_frame_buffers 64
#define maximum_frame_buffers_allocation 14156800


struct pmp_decode_buffer_struct
	{
	void *video_frame;
	void *audio_frame;

	unsigned int number_of_audio_frames;

	int first_delay;
	int last_delay;
	};


struct pmp_decode_struct
	{
	struct pmp_read_struct reader;


	AVCodec *video_decoder;
	AVCodec *audio_decoder;

	AVCodecContext *video_context;
	AVCodecContext *audio_context;

	AVFrame *video_frame;


	struct csc_remaining_struct *csc_remaining;


	void *video_frame_buffers[maximum_frame_buffers];
	void *audio_frame_buffers[maximum_frame_buffers];

	unsigned int number_of_frame_buffers;


	struct pmp_decode_buffer_struct output_frame_buffers[maximum_frame_buffers];

	unsigned int current_buffer_number;

	int		volume_boost;
	int		luminosity_boost;
	};


void pmp_decode_safe_constructor(struct pmp_decode_struct *p);
char *pmp_decode_open(struct pmp_decode_struct *p, char *s);
void pmp_decode_close(struct pmp_decode_struct *p);
char *pmp_decode_get(struct pmp_decode_struct *p, unsigned int frame_number, int audio_delay, int decode_audio);


#endif
