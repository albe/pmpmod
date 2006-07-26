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

	avc_safe_constructor(&p->avc);

	p->audio_context = 0;

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
	
	avc_close(&p->avc);

	if (p->audio_context != 0)
		{
		avcodec_close(p->audio_context);
		av_free(p->audio_context);
		}

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

	p->audio_frame_size = (p->reader.file.header.audio.scale << 1) << p->reader.file.header.audio.stereo;




	result = avc_open(&p->avc, p->reader.file.maximum_packet_size);
	if (result != 0)
		{
		pmp_decode_close(p);
		return(result);
		}




	aspect_ratio_struct_init(p->reader.file.header.video.width, p->reader.file.header.video.height);




	p->audio_decoder = avcodec_find_decoder(CODEC_ID_MP3);
	if (p->audio_decoder == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_find_decoder failed on CODEC_ID_MP3");
		}


	p->audio_context = avcodec_alloc_context();
	if (p->audio_context == 0)
		{
		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_alloc_context failed");
		}


	cpu_clock_set_maximum();

	if (avcodec_open(p->audio_context, p->audio_decoder) < 0)
		{
		cpu_clock_set_minimum();

    	av_free(p->audio_context);
    	p->audio_context = 0;

		pmp_decode_close(p);
		return("pmp_decode_open: avcodec_open failed on audio_context");
		}
	
	cpu_clock_set_minimum();




	p->number_of_frame_buffers = 0;

	int i = 0;
	for (; i < maximum_frame_buffers; i++)
		{
		p->video_frame_buffers[i] = malloc_64(557056);

		if (p->video_frame_buffers[i] == 0)
			{
			break;
			}

		memset(p->video_frame_buffers[i], 0, 557056);

		p->number_of_frame_buffers ++;
		}


	if (p->number_of_frame_buffers < (number_of_free_video_frame_buffers + 4))
		{
		pmp_decode_close(p);
		return("pmp_decode_open: number_of_frame_buffers < 4");
		}


	p->number_of_frame_buffers -= number_of_free_video_frame_buffers;

	i = 0;
	for (; i < number_of_free_video_frame_buffers; i++)
		{
		free_64(p->video_frame_buffers[p->number_of_frame_buffers + i]);

		p->video_frame_buffers[p->number_of_frame_buffers + i] = 0;
		}




	i = 0;
	for (; i <= p->number_of_frame_buffers; i++)
		{
		// p->audio_frame_buffers[p->number_of_frame_buffers] is a null buffer

		p->audio_frame_buffers[i] = malloc_64(p->audio_frame_size * p->reader.file.header.audio.maximum_number_of_frames);

		if (p->audio_frame_buffers[i] == 0)
			{
			pmp_decode_close(p);
			return("pmp_decode_open: malloc_64 failed on audio_frame_buffers");
			}

		memset(p->audio_frame_buffers[i], 0, p->audio_frame_size * p->reader.file.header.audio.maximum_number_of_frames);
		}




	sceDisplayWaitVblankStart();
	sceDisplaySetFrameBuf(p->video_frame_buffers[0], 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);




	p->current_buffer_number = 0;


	return(0);
	}


static void boost_volume(short *audio_buffer, unsigned int number_of_samples, unsigned int volume_boost)
	{
	if (volume_boost != 0)
		{
		while (number_of_samples--)
			{
			int sample = *audio_buffer;
			sample <<= volume_boost;


			if (sample > 32767)
				{
				*audio_buffer++ = 32767;
				}
			else if (sample < -32768)
				{
				*audio_buffer++ = -32768;
				}
			else
				{
				*audio_buffer++ = sample;
				}
			}
		}
	}


char *pmp_decode_get(struct pmp_decode_struct *p, unsigned int frame_number, unsigned int audio_stream, int decode_audio, unsigned int volume_boost, unsigned int aspect_ratio, unsigned int zoom, unsigned int luminosity_boost, unsigned int show_interface, unsigned int show_subtitle, unsigned int subtitle_format, unsigned int loop)
	{
	struct pmp_read_output_struct packet;


	char *result = pmp_read_get(&p->reader, frame_number, audio_stream, &packet);
	if (result != 0)
		{
		return(result);
		}


	sceKernelDcacheWritebackInvalidateAll();
	result = avc_get(&p->avc, packet.video_buffer, packet.video_length, pmp_gu_avc_buffer);
	if (result != 0)
		{
		return(result);
		}


	if (show_interface == 1)
		{
		draw_interface
			(
			p->reader.file.header.video.scale,
			p->reader.file.header.video.rate,
			p->reader.file.header.video.number_of_frames,
			frame_number,
			aspect_ratio,
			zoom,
			luminosity_boost,
			audio_stream,
			volume_boost,
			loop
			);
		}


	sceKernelDcacheWritebackInvalidateAll();
	pmp_gu_draw(aspect_ratio, zoom, luminosity_boost, show_interface, show_subtitle, subtitle_format, frame_number, p->video_frame_buffers[p->current_buffer_number]);




	p->output_frame_buffers[p->current_buffer_number].number_of_audio_frames = packet.number_of_audio_frames;
	p->output_frame_buffers[p->current_buffer_number].first_delay            = packet.first_delay;
	p->output_frame_buffers[p->current_buffer_number].last_delay             = packet.last_delay;
	p->output_frame_buffers[p->current_buffer_number].video_frame            = p->video_frame_buffers[p->current_buffer_number];




	char *audio_result = 0;


	if (packet.audio_buffer == 0 || decode_audio == 0)
		{
		p->output_frame_buffers[p->current_buffer_number].audio_frame = p->audio_frame_buffers[p->number_of_frame_buffers];
		}
	else
		{
		p->output_frame_buffers[p->current_buffer_number].audio_frame = p->audio_frame_buffers[p->current_buffer_number];


		void *audio_buffer = packet.audio_buffer;


		int i = 0;
		for (; i < packet.number_of_audio_frames; i++)
			{
			int audio_length = packet.audio_length[i];
			int audio_output_length;

			if (avcodec_decode_audio(p->audio_context, p->audio_frame_buffers[p->current_buffer_number] + p->audio_frame_size * i, &audio_output_length, audio_buffer, audio_length) != audio_length)
				{
				audio_result = "pmp_decode_get: avcodec_decode_audio failed";
				break;
				}

			audio_buffer += audio_length;


			if (audio_output_length != p->audio_frame_size)
				{
				if (audio_output_length == 0)
					{
					}
				else if (audio_output_length < p->audio_frame_size)
					{
					audio_result = "pmp_decode_get: audio_output_length < audio_frame_size";
					break;
					}
				else if (audio_output_length > p->audio_frame_size)
					{
					audio_result = "pmp_decode_get: audio_output_length > audio_frame_size (severe error)";
					break;
					}
				}
			else
				{
				boost_volume(p->audio_frame_buffers[p->current_buffer_number] + p->audio_frame_size * i, p->reader.file.header.audio.scale << p->reader.file.header.audio.stereo, volume_boost);
				}
			}
		}
	
	
	
	
	pmp_gu_wait();




	p->current_buffer_number = (p->current_buffer_number + 1) % p->number_of_frame_buffers;
	sceKernelDcacheWritebackInvalidateAll();


	return(audio_result);
	}
