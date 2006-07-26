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


#include "pmp_decode.h"


void pmp_decode_safe_constructor(struct pmp_decode_struct *p)
	{
	pmp_read_safe_constructor(&p->reader);

	p->video_context = 0;
	p->audio_context = 0;

	p->video_frame = 0;

	p->csc_remaining = 0;

	p->volume_boost = 0;
	p->luminosity_boost = 0;

	int i = 0;
	for (; i < maximum_frame_buffers; i++)
		{
		p->video_frame_buffers[i] = 0;
		p->audio_frame_buffers[i] = 0;
		}
	}


void pmp_decode_close(struct pmp_decode_struct *p)
	{
	pmp_read_close(&p->reader);

	if (p->video_context != 0)
		{
		avcodec_close(p->video_context);
		av_free(p->video_context);
		}

	if (p->audio_context != 0)
		{
		avcodec_close(p->audio_context);
		av_free(p->audio_context);
		}

	if (p->video_frame != 0) av_free(p->video_frame);

	if (p->csc_remaining != 0) free_64(p->csc_remaining);

	int i = 0;
	for (; i < maximum_frame_buffers; i++)
		{
		if (p->video_frame_buffers[i] != 0) free_64(p->video_frame_buffers[i]);
		if (p->audio_frame_buffers[i] != 0) free_64(p->audio_frame_buffers[i]);
		}


	pmp_decode_safe_constructor(p);
	}


char *pmp_decode_open(struct pmp_decode_struct *p, char *s)
	{
	pmp_decode_safe_constructor(p);




	char *result = pmp_read_open(&p->reader, FF_INPUT_BUFFER_PADDING_SIZE, s);
	if (result != 0)
		{
		pmp_decode_close(p);
		return(result);
		}




	p->video_decoder = avcodec_find_decoder(CODEC_ID_MPEG4);
	if (p->video_decoder == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_find_decoder failed on CODEC_ID_MPEG4");
		}


	p->audio_decoder = avcodec_find_decoder(CODEC_ID_MP3);
	if (p->audio_decoder == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_find_decoder failed on CODEC_ID_MP3");
		}




	p->video_context = avcodec_alloc_context();
	if (p->video_context == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_alloc_context failed");
		}


    p->video_context->width  = p->reader.header.vid.width;
    p->video_context->height = p->reader.header.vid.height;

    if (avcodec_open(p->video_context, p->video_decoder) < 0)
    	{
    	av_free(p->video_context);
    	p->video_context = 0;

		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_open failed on video_context");
    	}




	p->audio_context = avcodec_alloc_context();
	if (p->audio_context == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_alloc_context failed");
		}


	if (avcodec_open(p->audio_context, p->audio_decoder) < 0)
		{
    	av_free(p->audio_context);
    	p->audio_context = 0;

		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_open failed on audio_context");
		}




	p->video_frame = avcodec_alloc_frame();
	if (p->video_frame == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_alloc_frame failed");
		}




	p->csc_remaining = malloc_64(sizeof(struct csc_remaining_struct));
	if (p->csc_remaining == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: malloc_64 failed on csc_remaining");
		}




	p->number_of_frame_buffers = maximum_frame_buffers_allocation / (557056 + 4608 * p->reader.maximum_number_of_audio_frames);
	if (p->number_of_frame_buffers < 4)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: number_of_frame_buffers < 4");
		}


	int i = 0;
	for (; i < p->number_of_frame_buffers; i++)
		{
		p->video_frame_buffers[i] = malloc_64(557056);

		if (p->video_frame_buffers[i] == 0)
			{
			pmp_decode_close(p);
			return("pmp_decode_open: malloc_64 failed on video_frame_buffers");
			}

		memset(p->video_frame_buffers[i], 0, 557056);


		p->audio_frame_buffers[i] = malloc_64(4608 * p->reader.maximum_number_of_audio_frames);

		if (p->audio_frame_buffers[i] == 0)
			{
			pmp_decode_close(p);
			return("pmp_decode_open: malloc_64 failed on audio_frame_buffers");
			}

		memset(p->audio_frame_buffers[i], 0, 4608 * p->reader.maximum_number_of_audio_frames);
		}


	// p->audio_frame_buffers[p->number_of_frame_buffers] is a null buffer
	
	p->audio_frame_buffers[i] = malloc_64(4608 * p->reader.maximum_number_of_audio_frames);

	if (p->audio_frame_buffers[i] == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: malloc_64 failed on audio_frame_buffers");
		}

	memset(p->audio_frame_buffers[i], 0, 4608 * p->reader.maximum_number_of_audio_frames);




	p->current_buffer_number = 0;


	return(0);
	}

void	volume_boost( short* audio_data, int length, int boost )
	{
	register int	l = length / 2;
	register short*	d = audio_data;

	register int	b = 1 << boost;

	while (l--)
		{
		register int	val = *d * b;
		if (val > 32767)
			val = 32767;
		else if (val < -32768)
			val = -32768;
		*d++ = (short)val;
		}
	}


char *pmp_decode_get(struct pmp_decode_struct *p, unsigned int frame_number, int audio_delay, int decode_audio)
	{
	void *video_buf;
	void *audio_buf;

	char *result = pmp_read_get(&p->reader, frame_number, audio_delay, &video_buf, &audio_buf);
	if (result != 0)
		{
		return(result);
		}




	int got_frame;
	int frame_length = p->reader.header.vid_index[frame_number] >> 1;

	if (avcodec_decode_video(p->video_context, p->video_frame, &got_frame, video_buf, frame_length) != frame_length)
		{
		return("pmp_decode_get: avcodec_decode_video failed");
		}




	p->output_frame_buffers[p->current_buffer_number].number_of_audio_frames = p->reader.interleaving.output_number_of_audio_frames;
	p->output_frame_buffers[p->current_buffer_number].first_delay            = p->reader.interleaving.output_audio_delay[0];
	p->output_frame_buffers[p->current_buffer_number].last_delay             = p->reader.interleaving.output_last_delay;
	p->output_frame_buffers[p->current_buffer_number].video_frame            = p->video_frame_buffers[p->current_buffer_number];




	if (got_frame == 0)
		{
		if (frame_number == 0)
			{
			memset(p->video_frame_buffers[p->current_buffer_number], 0, 557056);
			}
		else
			{
			unsigned int previous_buffer_number;

			if (p->current_buffer_number == 0)
				{
				previous_buffer_number = p->number_of_frame_buffers - 1;
				}
			else
				{
				previous_buffer_number = p->current_buffer_number - 1;
				}


	        void *current_buffer  = p->video_frame_buffers[p->current_buffer_number];
	        void *previous_buffer = p->video_frame_buffers[previous_buffer_number];

	        memcpy(current_buffer, previous_buffer, 557056);
			}
		}
	else
		{
		me_signal_reset();


		p->csc_remaining->csc.cy           = p->video_frame->data[0];
		p->csc_remaining->csc.cu           = p->video_frame->data[1];
		p->csc_remaining->csc.cv           = p->video_frame->data[2];
		p->csc_remaining->csc.l0           = p->video_frame->linesize[0];
		p->csc_remaining->csc.l1           = p->video_frame->linesize[1];
		p->csc_remaining->csc.l2           = p->video_frame->linesize[2];
		p->csc_remaining->csc.out          = p->video_frame_buffers[p->current_buffer_number] + ((480 - p->reader.header.vid.width) << 1) + ((272 - p->reader.header.vid.height) << 10);
		p->csc_remaining->csc.width        = p->reader.header.vid.width;
		p->csc_remaining->csc.height       = 8;
		p->csc_remaining->csc.line_size    = 512;
		p->csc_remaining->csc.y_offset     = p->luminosity_boost;
		p->csc_remaining->remaining_height = p->reader.header.vid.height;


		sceKernelDcacheWritebackInvalidateAll();
		me_start((int) &me_csc_asm_remaining, (int) p->csc_remaining);
		}




	char *audio_result = 0;


	if (audio_buf == 0 || decode_audio == 0)
		{
		p->output_frame_buffers[p->current_buffer_number].audio_frame = p->audio_frame_buffers[p->number_of_frame_buffers];
		}
	else
		{
		p->output_frame_buffers[p->current_buffer_number].audio_frame = p->audio_frame_buffers[p->current_buffer_number];


		int current_audio_frame = p->reader.interleaving.output_audio_frame_number;
		current_audio_frame += audio_delay;


		int i = 0;
		for (; i < p->reader.interleaving.output_number_of_audio_frames; i++)
			{
			int audio_length = p->reader.header.aud.frame_size + p->reader.header.aud_index[current_audio_frame + i];
			int audio_output_length;

			if (avcodec_decode_audio(p->audio_context, p->audio_frame_buffers[p->current_buffer_number] + 4608 * i, &audio_output_length, audio_buf, audio_length) != audio_length)
				{
				audio_result = "pmp_decode_get: avcodec_decode_audio failed";
				break;
				}

			// boost audio volume
			if ((audio_output_length == 4608) && (p->volume_boost))
				volume_boost( p->audio_frame_buffers[p->current_buffer_number] + 4608 * i, audio_output_length, p->volume_boost );


			audio_buf += audio_length;


			if (audio_output_length != 4608)
				{
				if (audio_output_length == 0)
					{
					}
				else if (audio_output_length < 4608)
					{
					audio_result = "pmp_decode_get: audio_output_length < 4608";
					break;
					}
				else if (audio_output_length > 4608)
					{
					audio_result = "pmp_decode_get: audio_output_length > 4608 (severe error)";
					break;
					}
				}
			}
		}




	if (got_frame != 0)
		{
		me_signal();
		me_wait();


		if (p->csc_remaining->remaining_height != 0)
		{
		me_csc
			(
				p->csc_remaining->csc.cy,
				p->csc_remaining->csc.cu,
				p->csc_remaining->csc.cv,
				p->csc_remaining->csc.l0,
				p->csc_remaining->csc.l1,
				p->csc_remaining->csc.l2,
				p->csc_remaining->csc.out,
				p->csc_remaining->csc.width,
				p->csc_remaining->remaining_height,
				p->csc_remaining->csc.line_size,
                p->csc_remaining->csc.y_offset
			);
		}
		}




	p->current_buffer_number = (p->current_buffer_number + 1) % p->number_of_frame_buffers;
	sceKernelDcacheWritebackInvalidateAll();


	return(audio_result);
	}
