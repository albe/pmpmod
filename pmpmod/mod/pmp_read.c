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


#include "pmp_read.h"


void pmp_read_safe_constructor(struct pmp_read_struct *p)
	{
	pmp_lib_safe_constructor(&p->header);

	buf_read_safe_constructor(&p->video_reader);
	buf_read_safe_constructor(&p->audio_reader);
	}


void pmp_read_close(struct pmp_read_struct *p)
	{
    pmp_lib_close(&p->header);

    buf_read_close(&p->video_reader);
    buf_read_close(&p->audio_reader);


	pmp_read_safe_constructor(p);
	}


static void minimum_and_maximum_number_of_audio_frames_get(struct pmp_lib_struct *p, unsigned int *minimum, unsigned int *maximum)
	{
	struct time_math_interleaving_struct t;


	time_math_interleaving_constructor(&t, p->vid.rate, p->vid.scale, 44100, 1152);


	unsigned int video_frame_number = 0;
	while (video_frame_number < p->vid.number_of_frames)
		{
		time_math_interleaving_get(&t);


		if (video_frame_number == 0)
			{
			*minimum = t.output_number_of_audio_frames;
			*maximum = t.output_number_of_audio_frames;
			}
		else
			{
			if (t.output_number_of_audio_frames < *minimum) *minimum = t.output_number_of_audio_frames;
			if (t.output_number_of_audio_frames > *maximum) *maximum = t.output_number_of_audio_frames;
			}


		video_frame_number++;
		}
	}


char *pmp_read_open(struct pmp_read_struct *p, unsigned int padding, char *s)
	{
	pmp_read_safe_constructor(p);




	char *result = pmp_lib_open(&p->header, s);
	if (result != 0)
		{
		pmp_read_close(p);
		return(result);
		}


	unsigned int minimum_number_of_audio_frames = 0;
	unsigned int maximum_number_of_audio_frames = 0;
	minimum_and_maximum_number_of_audio_frames_get(&p->header, &minimum_number_of_audio_frames, &maximum_number_of_audio_frames);

	if (minimum_number_of_audio_frames == 0)
		{
		pmp_read_close(p);
		return("pmp_read_open: framerate is too high");
		}




	result = buf_read_open(&p->video_reader, p->header.vid.maximum_frame_size, padding, s);
	if (result != 0)
		{
		pmp_read_close(p);
		return(result);
		}

	result = buf_read_open(&p->audio_reader, (p->header.aud.frame_size + 1) * maximum_number_of_audio_frames, padding, s);
	if (result != 0)
		{
		pmp_read_close(p);
		return(result);
		}




	p->maximum_number_of_audio_frames = maximum_number_of_audio_frames;

	p->current_video_frame    = 0;
	p->current_video_position = p->header.vid_start;

	p->current_audio_frame    = 0;
	p->current_audio_position = p->header.aud_start;

	time_math_interleaving_constructor(&p->interleaving, p->header.vid.rate, p->header.vid.scale, 44100, 1152);
	time_math_interleaving_get(&p->interleaving);


	return(0);
	}


static char *pmp_read_video_get(struct pmp_read_struct *p, unsigned int frame_number, void **buf)
	{
	if (p->current_video_frame > frame_number)
		{
		p->current_video_frame    = 0;
		p->current_video_position = p->header.vid_start;
		}


	while (p->current_video_frame != frame_number)
		{
		p->current_video_position += p->header.vid_index[p->current_video_frame] >> 1;

		p->current_video_frame++;
		}


	return(buf_read_get(&p->video_reader, p->current_video_position, p->header.vid_index[p->current_video_frame] >> 1, buf));
	}


static char *pmp_read_audio_get(struct pmp_read_struct *p, int frame_number, unsigned int number_of_frames, int delay, void **buf)
	{
	frame_number += delay;
	if (frame_number < 0)
		{
		*buf = 0;
		return(0);
		}


	if (!contained(0, p->header.aud.number_of_frames, frame_number, number_of_frames))
		{
		*buf = 0;
		return(0);
		}




	if (p->current_audio_frame > frame_number)
		{
		p->current_audio_frame    = 0;
		p->current_audio_position = p->header.aud_start;
		}


	while (p->current_audio_frame != frame_number)
		{
		p->current_audio_position += p->header.aud.frame_size + p->header.aud_index[p->current_audio_frame];

		p->current_audio_frame++;
		}


	unsigned int read_size = 0;
	unsigned int i         = 0;

	for (; i < number_of_frames; i++)
		{
		read_size += p->header.aud.frame_size + p->header.aud_index[p->current_audio_frame + i];
		}


	return(buf_read_get(&p->audio_reader, p->current_audio_position, read_size, buf));
	}


char *pmp_read_get(struct pmp_read_struct *p, unsigned int frame_number, int audio_delay, void **video_buf, void **audio_buf)
	{
	if (!(frame_number < p->header.vid.number_of_frames)) return("pmp_read_get: out of range");


	if (p->interleaving.output_video_frame_number > frame_number)
		{
		time_math_interleaving_constructor(&p->interleaving, p->header.vid.rate, p->header.vid.scale, 44100, 1152);
		time_math_interleaving_get(&p->interleaving);
		}


	while (p->interleaving.output_video_frame_number != frame_number)
		{
		time_math_interleaving_get(&p->interleaving);
		}




	char *result = pmp_read_video_get(p, p->interleaving.output_video_frame_number, video_buf);
	if (result != 0)
		{
		return(result);
		}


	return(pmp_read_audio_get(p, p->interleaving.output_audio_frame_number, p->interleaving.output_number_of_audio_frames, audio_delay, audio_buf));
	}
