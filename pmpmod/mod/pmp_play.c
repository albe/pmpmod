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
this play the file (av output and basic functions - pause, seek ... )
*/

#include <psppower.h>
#include "pmp_play.h"

extern char psp_full_path[1024 + 1];


#define USE_GU_DISPLAY	1

#if USE_GU_DISPLAY
	#include <pspgu.h>
	#include <psprtc.h>
	
	unsigned int g_interface_alpha = 15;			// fully opaque by default
#endif


void pmp_play_safe_constructor(struct pmp_play_struct *p)
	{
	p->audio_reserved = -1;

	p->semaphore_can_get   = -1;
	p->semaphore_can_put   = -1;
	p->semaphore_can_show  = -1;
	p->semaphore_show_done = -1;

	p->output_thread = -1;
	p->show_thread   = -1;
	p->input_thread  = -1;

	p->last_keyframe_pos = 0;
	p->resume_pos = 0;
	p->zoom_mode = 0;
	p->zoom_mode_prev = 0;

	p->interface_texture = 0;
	p->display_interface = 0;
	p->interface_pos = 0;

	pmp_decode_safe_constructor(&p->decoder);
	}


void pmp_play_close(struct pmp_play_struct *p, int usePos)
	{
	if (!(p->audio_reserved < 0)) sceAudioChRelease(0);

	if (!(p->semaphore_can_get   < 0)) sceKernelDeleteSema(p->semaphore_can_get);
	if (!(p->semaphore_can_put   < 0)) sceKernelDeleteSema(p->semaphore_can_put);
	if (!(p->semaphore_can_show  < 0)) sceKernelDeleteSema(p->semaphore_can_show);
	if (!(p->semaphore_show_done < 0)) sceKernelDeleteSema(p->semaphore_show_done);

	if (!(p->output_thread < 0)) sceKernelDeleteThread(p->output_thread);
	if (!(p->show_thread   < 0)) sceKernelDeleteThread(p->show_thread);
	if (!(p->input_thread  < 0)) sceKernelDeleteThread(p->input_thread);


	// save current position to resume later (if there wasn't any error)
	if (usePos) {
		SceUID	fd;

		// device:path
		if((fd = sceIoOpen( p->resume_filename, PSP_O_WRONLY|PSP_O_CREAT, 0777)))
			{
			sceIoWrite( fd, &p->last_keyframe_pos, sizeof(int) );
			sceIoWrite( fd, &p->decoder.volume_boost, sizeof(int) );
			sceIoWrite( fd, &p->decoder.luminosity_boost, sizeof(int) );
			sceIoWrite( fd, &p->zoom_mode, sizeof(int) );
			sceIoClose( fd );
			} else {
			sceIoRemove( p->resume_filename );	// Delete the file if something got wrong, or else there's a 0kb file
			}
	}

	// free interface texture
	if (p->interface_texture)
		free_64( p->interface_texture );



	pmp_decode_close(&p->decoder);


	pmp_play_safe_constructor(p);
	}
	

static int pmp_wait(volatile struct pmp_play_struct *p, SceUID s, char *e)
	{
	SceUInt t = 1000000;
	

	while (1)
		{
		int result = sceKernelWaitSema(s, 1, &t);

		if (result == SCE_KERNEL_ERROR_OK)
			{
			break;
			}
		else if (result == SCE_KERNEL_ERROR_WAIT_TIMEOUT)
			{
			if (p->return_request == 1)
				{
				return(0);
				}
			}
		else
			{
			p->return_result  = e;
			p->return_request = 1;
			return(0);
			}
		}


	return(1);
	}




#if USE_GU_DISPLAY

#ifndef USE_SKIN
#include "gu_funcs.h"
#endif


#define FONT_START_X	0
#define FONT_START_Y	0
#define FONT_STEP		16
#define FONT_WIDTH		12
#define FONT_HEIGHT		16

void blitTen( int dx, int dy, int val )
{
	int	high = val / 10;
	int	low = val % 10;

	spriteBlit( dx, dy, FONT_START_X + FONT_STEP*high, FONT_START_Y, FONT_WIDTH, FONT_HEIGHT );
	spriteBlit( dx + FONT_WIDTH, dy, FONT_START_X + FONT_STEP*low, FONT_START_Y, FONT_WIDTH, FONT_HEIGHT );
}

void blitTime2( int dx, int dy, int hour, int min, int sec )
{
	blitTen( dx, dy, hour );
	
	spriteBlit( dx + FONT_WIDTH*2, dy, FONT_START_X + FONT_STEP*10, FONT_START_Y, FONT_WIDTH, FONT_HEIGHT );

	blitTen( dx + FONT_WIDTH*2+5, dy, min );

	spriteBlit( dx + FONT_WIDTH*4+5, dy, FONT_START_X + FONT_STEP*10, FONT_START_Y, FONT_WIDTH, FONT_HEIGHT );

	blitTen( dx + FONT_WIDTH*4+10, dy, sec );
}

void blitTime( int dx, int dy, int time )
{
	int	hour;
	int	min;
	int	sec;

	sec = time % 60;
	min = (time / 60) % 60;
	hour = time / (60*60);

	blitTime2( dx, dy, hour, min, sec );
}

void	CallbackGu( int index )
{
//	sceGuSync(0,0);
//	sceDisplayWaitVblankStart();
//	sceGuSwapBuffers();
}

#endif

int update_counter = 1;
int battery_power = 0;

static int pmp_show_thread(SceSize input_length, void *input)
	{
	volatile struct pmp_play_struct *p = *((void **) input);

	unsigned int	current_buffer_number = 0;

#if USE_GU_DISPLAY
	int			previous_display_setbuffer = 1;
	int			clear_backbuffer = 0;
	sceGuInit();

	sceGuStart(GU_DIRECT,g_list[0]);
	sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
	sceGuDispBuffer(480, 272, (void*)(4*512*272), 512);
	sceGuOffset(2048 - (480/2),2048 - (272/2));
	sceGuViewport(2048, 2048, 480, 272);
	sceGuScissor(0,0,480,272);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFrontFace(GU_CW);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_8888,0,0,0); // 32-bit RGBA
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA); // don't get influenced by any vertex colors
	if (p->zoom_mode == ZOOM_NONE)
		sceGuTexFilter(GU_NEAREST,GU_NEAREST); // point-filtered sampling
	else
		sceGuTexFilter(GU_LINEAR,GU_LINEAR); // point-filtered sampling
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	sceGuSwapBuffers();

	sceGuStart(GU_DIRECT,g_list);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(1);

	void* framebuffer = 0;
#endif


	while (p->return_request == 0)
		{
		if (pmp_wait(p, p->semaphore_can_show, "pmp_show_thread: sceKernelWaitSema failed on semaphore_can_show") == 0)
			{
			break;
			}

#if USE_GU_DISPLAY
		// interface anim
		if (p->display_interface)
			{
			if (p->interface_pos < 18)
				p->interface_pos++;
			}
		else
			{
			if (p->interface_pos > 0)
				p->interface_pos--;
			}

		if ((p->zoom_mode == ZOOM_NONE) && (p->interface_pos == 0))
			{
			// just use the old setbuffer if non zoom and no inteface display
			sceDisplayWaitVblankStart();
			sceDisplaySetFrameBuf(p->decoder.output_frame_buffers[current_buffer_number].video_frame, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);
			previous_display_setbuffer = 1;
			}
		else
			{
			// use the GU to set the correct zoom
			
			// do the sync & swap at the start to avoid waiting for the GU at the end and slow down the app !
//			if (sceGuSync(0,1) == 0)	// check if finished drawing, otherwise skip a frame blit !
				{
				
				sceGuSync(0,0);

				// don't swap the first frame after a setbuffer to avoid the display of an old back buffer
				if (previous_display_setbuffer == 0)
				{
					sceDisplayWaitVblankStart();
					framebuffer = sceGuSwapBuffers();
				}
				else
				{
					previous_display_setbuffer = 0;
				}


				// swap list
//				if (g_list_index == 1)
//					g_list_index = 0;
//				else
//					g_list_index = 1;

				
				sceGuStart(GU_DIRECT,g_list[g_list_index]);

				sceGuTexImage(0,512,512,512,p->decoder.output_frame_buffers[current_buffer_number].video_frame); // setup texture as a 512x512 texture, even though the buffer is only 512x272 (480 visible)


				// we need to clear the screen for empty zones if the interface is moving, or if the zoom mode changed
				if (((p->interface_pos != 17) && (p->interface_pos)) || (p->zoom_mode_prev != p->zoom_mode))
					clear_backbuffer = 2;	// clear the 2 backbuffers

				if (clear_backbuffer)
				{
					clear_backbuffer--;

					// clear
					sceGuClear(GU_COLOR_BUFFER_BIT);
					// reset the filter
					if (p->zoom_mode == ZOOM_NONE)
						sceGuTexFilter(GU_NEAREST,GU_NEAREST); // point-filtered sampling
					else
						sceGuTexFilter(GU_LINEAR,GU_LINEAR); // point-filtered sampling
				}

	#define BLIT_SLICE	32
		//		advancedBlit(0,0,480,272,0,0,480,272,16);		// copy full buffer to full screen

				int		vid_width = p->decoder.reader.header.vid.width;
				int		vid_height = p->decoder.reader.header.vid.height;

				switch( p->zoom_mode )
					{
					case ZOOM_NONE:		

						advancedBlit((480-vid_width)>>1,(272-vid_height)>>1,vid_width,vid_height,(480-vid_width)>>1,(272-vid_height)>>1,vid_width,vid_height,BLIT_SLICE);	
						
						break;
					
					case ZOOM_WIDTH:		
						{
						int		new_height = (vid_height * 480) / vid_width;
						if (new_height > 272)	// crop top&bottom
							{
							int	crop = ((new_height-272) * vid_width) / 480;
							advancedBlit((480-vid_width)/2,(272-vid_height)/2 + crop/2,vid_width,vid_height-crop,0,0,480,272,BLIT_SLICE);
							}
						else
							{
							advancedBlit((480-vid_width)/2,(272-vid_height)/2,vid_width,vid_height,0,(272-new_height)/2,480,new_height,BLIT_SLICE);
							}
						}
						break;

					case ZOOM_HEIGHT:					
						{
						int		new_width = (vid_width * 272) / vid_height;
						if (new_width > 480)	// crop left&right
							{
							int	crop = ((new_width-480) * vid_height) / 272;
							advancedBlit((480-vid_width)/2 + crop/2,(272-vid_height)/2,vid_width-crop,vid_height,0,0,480,272,BLIT_SLICE);
							}
						else
							{
							advancedBlit((480-vid_width)/2,(272-vid_height)/2,vid_width,vid_height,(480-new_width)/2,0,new_width,272,BLIT_SLICE);
							}
						}
						break;

					case ZOOM_FILL:		advancedBlit((480-vid_width)/2,(272-vid_height)/2,vid_width,vid_height,0,0,480,272,BLIT_SLICE);	break;
					
					};

				// display interface
				if (p->interface_pos)
				{
					int	y = 18 - p->interface_pos;

					sceGuTexImage(0,256,256,256,p->interface_texture);

					sceGuEnable( GU_BLEND );
					sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
					sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
					g_interface_alpha = 7;

					// bg
					spriteBlit( 0, 0-y, 0, 32, 256, 16 );
					spriteBlit( 0, 272-16+y, 0, 64, 256, 16 );

					int	total = p->decoder.reader.header.vid.number_of_frames;
					int	current = p->current_video_frame;

					//float	num =(float)(p->decoder.video_context->time_base.den);
					//float	den =(float)(p->decoder.video_context->time_base.num);
					float	fps = (float)p->decoder.reader.header.vid.rate / (float)p->decoder.reader.header.vid.scale;
					int	videotime = (int)(current/fps);

					spriteBlit( 112 + current*128/total, 272-16+y, 12*16, 0, 3, 16 );

					// time
					blitTime( 16, 272-16+y, videotime );

					// clock
					pspTime	clock;
					sceRtcGetCurrentClockLocalTime( &clock );
					blitTime2( 16, 0-y-1, clock.hour, clock.minutes, clock.seconds );

					// zoom
					spriteBlit( 112, 0-y, 16*p->zoom_mode, 16, 16, 16 );

					// luminosity boost
					spriteBlit( 112+16, 0-y, 128+16*(p->decoder.luminosity_boost/4), 16, 16, 16 );

					// volume boost
					spriteBlit( 112+32, 0-y, 64+16*p->decoder.volume_boost, 16, 16, 16 );
					
					if (!--update_counter) {
					  if (!scePowerIsLowBattery() && scePowerIsBatteryExist())
						battery_power = scePowerGetBatteryLifePercent();
					  else
					    battery_power = 0;

					  update_counter=30;	// update only every 30 frames (~1s)
					}
					
					// battery power
					if (battery_power)
					  spriteBlit( 112+50, 0-y+4, 48, 20, (battery_power<100?battery_power:99)*16/100, 8 );


					sceGuDisable( GU_BLEND );
					sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
				}

	//			sceGuSetCallback( g_list_index, CallbackGu );

				sceGuFinish();
				}
			}
#else
		sceDisplayWaitVblankStart();
		sceDisplaySetFrameBuf(p->decoder.output_frame_buffers[current_buffer_number].video_frame, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);
#endif


		if (sceKernelSignalSema(p->semaphore_show_done, 1) < 0)
			{
			p->return_result  = "pmp_show_thread: sceKernelSignalSema failed on semaphore_show_done";
			p->return_request = 1;
			break;
			}


		current_buffer_number = (current_buffer_number + 1) % p->decoder.number_of_frame_buffers;
		}


#if USE_GU_DISPLAY
	sceGuTerm();
#endif


	return(0);
	}


static int pmp_output_thread(SceSize input_length, void *input)
	{
	volatile struct pmp_play_struct *p = *((void **) input);


    unsigned int first_video_frame     = 1;
	unsigned int current_buffer_number = 0;


	while (p->return_request == 0)
		{
		volatile struct pmp_decode_buffer_struct *current_buffer = &p->decoder.output_frame_buffers[current_buffer_number];


		if (pmp_wait(p, p->semaphore_can_get, "pmp_output_thread: sceKernelWaitSema failed on semaphore_can_get") == 0)
			{
			break;
			}




		if (sceKernelSignalSema(p->semaphore_can_show, 1) < 0)
			{
			p->return_result  = "pmp_output_thread: sceKernelSignalSema failed on semaphore_can_show";
			p->return_request = 1;
			break;
			}


		if (p->seek == 0)
			{
			sceKernelDelayThread(current_buffer->first_delay);
			sceAudioOutputBlocking(0, PSP_AUDIO_VOLUME_MAX, current_buffer->audio_frame);
			}


		if (pmp_wait(p, p->semaphore_show_done, "pmp_output_thread: sceKernelWaitSema failed on semaphore_show_done") == 0)
			{
			break;
			}




		if (first_video_frame == 1)
			{
			first_video_frame = 0;
			}
		else
			{
			if (sceKernelSignalSema(p->semaphore_can_put, 1) < 0)
				{
				p->return_result  = "pmp_output_thread: sceKernelSignalSema failed on semaphore_can_put";
				p->return_request = 1;
				break;
				}
			}




		if (p->seek == 0)
			{
			int i = 1;
			for (; i < current_buffer->number_of_audio_frames; i++)
				{
				sceAudioOutputBlocking(0, PSP_AUDIO_VOLUME_MAX, current_buffer->audio_frame + 4608 * i);
				}

			sceKernelDelayThread(current_buffer->last_delay);
			}


		current_buffer_number = (current_buffer_number + 1) % p->decoder.number_of_frame_buffers;




		while (p->return_request == 0 && p->paused == 1)
			{
			sceKernelDelayThread(100000);
			}
		}


	return(0);
	}


static int pmp_input_thread(SceSize input_length, void *input)
	{
	volatile struct pmp_play_struct *p = *((void **) input);


	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);


	SceCtrlData controller_previous;
	sceCtrlReadBufferPositive(&controller_previous, 1);

	while (p->return_request == 0)
		{
		scePowerTick(0);
		sceKernelDelayThread(100000);


		SceCtrlData controller;
		sceCtrlReadBufferPositive(&controller, 1);


		if (controller.Buttons != 0)
			{
			if (controller.Buttons & PSP_CTRL_TRIANGLE)
				{
				p->return_request = 1;
				}
			else if (controller.Buttons & PSP_CTRL_HOME)
				{
				p->resume_pos = -1;
				}
			else if (controller.Buttons & PSP_CTRL_START)
				{
				if ((p->interface_pos == 18) || (p->interface_pos == 0))
					p->display_interface = !p->display_interface;
				}
			else
				{
				if (p->paused == 1)
					{
					p->seek = 0;

					if (controller.Buttons & PSP_CTRL_CROSS)
						{
						p->paused = 0;
						}
					}
				else
					{
					if (controller.Buttons & PSP_CTRL_SQUARE)
						{
						p->paused = 1;
						p->seek   = 0;
						}
					else if (controller.Buttons & PSP_CTRL_RIGHT)
						{
						if (controller.Buttons & PSP_CTRL_CROSS)
							p->seek = 2;
						else
							p->seek = 1;
						}
					else if (controller.Buttons & PSP_CTRL_LEFT)
						{
						if (controller.Buttons & PSP_CTRL_CROSS)
							p->seek = -2;
						else
							p->seek = -1;
						}
					else
						{
						p->seek = 0;
						}

					if ((controller.Buttons & PSP_CTRL_UP) && ((controller_previous.Buttons & PSP_CTRL_UP) == 0))
						{
						p->decoder.volume_boost++;
						if (p->decoder.volume_boost > 3)
							p->decoder.volume_boost = 3;
						}
					else if ((controller.Buttons & PSP_CTRL_DOWN) && ((controller_previous.Buttons & PSP_CTRL_DOWN) == 0))
						{
						p->decoder.volume_boost--;
						if (p->decoder.volume_boost < 0)
							p->decoder.volume_boost = 0;
						}

					if ((controller.Buttons & PSP_CTRL_RTRIGGER) && ((controller_previous.Buttons & PSP_CTRL_RTRIGGER) == 0))
						{
						p->decoder.luminosity_boost += 4;
						if (p->decoder.luminosity_boost > 16)
							p->decoder.luminosity_boost = 16;
						}
					else if ((controller.Buttons & PSP_CTRL_LTRIGGER) && ((controller_previous.Buttons & PSP_CTRL_LTRIGGER) == 0))
						{
						p->decoder.luminosity_boost -= 4;
						if (p->decoder.luminosity_boost < 0)
							p->decoder.luminosity_boost = 0;
						}

					if ((controller.Buttons & PSP_CTRL_SELECT) && ((controller_previous.Buttons & PSP_CTRL_SELECT) == 0))
						{
						p->zoom_mode++;

						int		vid_width = p->decoder.reader.header.vid.width;
						int		vid_height = p->decoder.reader.header.vid.height;

						if ((vid_width == 480) && (p->zoom_mode == ZOOM_WIDTH))
							p->zoom_mode++;	// not needed -> skip !
						if ((vid_height == 272) && (p->zoom_mode == ZOOM_HEIGHT))
							p->zoom_mode++;	// not needed -> skip !
						if ((vid_width == 480) && (vid_height == 272) && (p->zoom_mode == ZOOM_FILL))
							p->zoom_mode++;	// not needed -> skip !

						if (p->zoom_mode >= ZOOM_MAX)
							p->zoom_mode = 0;
						}

					}
				}
			}

		controller_previous = controller;
		}


	return(0);
	}


static int pmp_next_video_frame(volatile struct pmp_play_struct *p, int current_video_frame)
	{

	// save last keyframe pos
	if (p->decoder.reader.header.vid_index[current_video_frame] & 1)
		p->last_keyframe_pos = current_video_frame;
	
	// resume position / restart
	if (p->resume_pos != 0)
		{
		if (p->resume_pos < 0)	// restart
			{
			p->resume_pos = 0;
			return(0);
			}
		else	// resume
			{
			int	pos = p->resume_pos;
			p->resume_pos = 0;
			return(pos);
			}
		}

	if (p->seek > 0)
		{
		int	skip_nb = 1;
		if (p->seek == 2)
			skip_nb = 20;

		int new_video_frame = current_video_frame + 1;

		while (new_video_frame < p->decoder.reader.header.vid.number_of_frames)
			{
			if (p->decoder.reader.header.vid_index[new_video_frame] & 1)
				{
				if (--skip_nb == 0)
					return(new_video_frame);
				}

			new_video_frame++;
			}

		return(p->decoder.reader.header.vid.number_of_frames);
		}


	if (p->seek < 0)
		{
		int	skip_nb = 1;
		if (p->seek == -2)
			skip_nb = 20;
		int new_video_frame = current_video_frame - 1;

		while (new_video_frame > 0)
			{
			if (p->decoder.reader.header.vid_index[new_video_frame] & 1)
				{
				if (--skip_nb == 0)
					return(new_video_frame);
				}

			new_video_frame--;
			}
			
		return(0);
		}


	return(current_video_frame + 1);
	}


char *pmp_play_start(volatile struct pmp_play_struct *p)
	{
	sceKernelStartThread(p->output_thread, 4, &p);
	sceKernelStartThread(p->show_thread,   4, &p);
	sceKernelStartThread(p->input_thread,  4, &p);




	int current_video_frame = 0;


	while (p->return_request == 0 && current_video_frame != p->decoder.reader.header.vid.number_of_frames)
		{
		if (pmp_wait(p, p->semaphore_can_put, "pmp_play_start: sceKernelWaitSema failed on semaphore_can_put") == 0)
			{
			break;
			}


		char *result = pmp_decode_get((struct pmp_decode_struct *) &p->decoder, current_video_frame, 0, 1);
		if (result != 0)
			{
			p->return_result  = result;
			p->return_request = 1;
			break;
			}


		if (sceKernelSignalSema(p->semaphore_can_get, 1) < 0)
			{
			p->return_result  = "pmp_play_start: sceKernelSignalSema failed on semaphore_can_get";
			p->return_request = 1;
			break;
			}


		current_video_frame = pmp_next_video_frame(p, current_video_frame);

		// save current video frame
		p->current_video_frame = current_video_frame;
		}

	// if the video was ended, set the resume pos to 0 so that the next time the video could restart from start
	if (current_video_frame == p->decoder.reader.header.vid.number_of_frames)
		p->last_keyframe_pos = 0;


	p->return_request = 1;




	sceKernelWaitThreadEnd(p->output_thread, 0);
	sceKernelWaitThreadEnd(p->show_thread,   0);
	sceKernelWaitThreadEnd(p->input_thread,  0);


	return(p->return_result);
	}


char *pmp_play_open(struct pmp_play_struct *p, char *s, int usePos)
	{
	pmp_play_safe_constructor(p);

	char	new_name[256];

	strcpy( new_name, s );

	// look if the .POS file was selected if so, remove the .POS extension
	{
		int	i = 0;
		while (new_name[i])
			i++;
		if (i > 4)
		{
			if ((new_name[i-1] == 'S') &&
				(new_name[i-2] == 'O') &&
				(new_name[i-3] == 'P') &&
				(new_name[i-4] == '.'))
			{
				new_name[i-4] = 0;
			}
		}
	}



	// load the interface
	{
		p->interface_texture = (unsigned char*)malloc_64( 256*256*4 );
		memset( p->interface_texture, 0, 256*256*4 );

		SceUID	fd;
		char	filename[1024];
		strcpy( filename, psp_full_path );
		strcpy( filename + strlen(filename), "/interf.tga" );
		if ((fd = sceIoOpen( filename, PSP_O_RDONLY, 0777)))
		{
			unsigned char	tgaheader[18];

			sceIoRead( fd, tgaheader, 18 );	// skip the header, we know its 256x80 32bits
			sceIoRead( fd, p->interface_texture, 256*80*4 );
			sceIoClose( fd );
			
			int x, y;
			char t;
			unsigned int offs = 0;
			// Swap R and B
			for (y=0;y<80;y++) {
			 for (x=0;x<256;x++) {
			   t = p->interface_texture[offs];
			   p->interface_texture[offs] = p->interface_texture[offs+2];
			   p->interface_texture[offs+2] = t;
			   offs += 4;
			 }
			}
		}
	}



	p->audio_reserved = sceAudioChReserve(0, 1152, PSP_AUDIO_FORMAT_STEREO);
	if (p->audio_reserved < 0)
		{
		pmp_play_close(p,0);
		return("pmp_play_open: sceAudioChReserve failed");
		}




	char *result = pmp_decode_open(&p->decoder, new_name);
	if (result != 0)
		{
		pmp_play_close(p,0);
		return(result);
		}


	// try to find a saved position to resume
	if (usePos) {
		strcpy( p->resume_filename, "ms0:/PSP/VIDEO/" );
		int	i = 0;
		while (p->resume_filename[i])
			i++;
		char*	s2 = new_name;
		while (*s2)
			p->resume_filename[i++] = *s2++;
		p->resume_filename[i+0] = '.';
		p->resume_filename[i+1] = 'P';
		p->resume_filename[i+2] = 'O';
		p->resume_filename[i+3] = 'S';
		p->resume_filename[i+4] = 0;

		SceUID	fd;
		if ((fd = sceIoOpen(p->resume_filename, PSP_O_RDONLY, 0777)))
			{
			sceIoRead( fd, &p->resume_pos, sizeof(int) );
			sceIoRead( fd, &p->decoder.volume_boost, sizeof(int) );
			sceIoRead( fd, &p->decoder.luminosity_boost, sizeof(int) );
			sceIoRead( fd, &p->zoom_mode, sizeof(int) );
			sceIoClose( fd );
			}
	}


	p->semaphore_can_get = sceKernelCreateSema("can_get", 0, 0, p->decoder.number_of_frame_buffers, 0);
	if (p->semaphore_can_get < 0)
		{
		pmp_play_close(p,0);
		return("pmp_play_open: sceKernelCreateSema failed on semaphore_can_get");
		}


	p->semaphore_can_put = sceKernelCreateSema("can_put", 0, p->decoder.number_of_frame_buffers, p->decoder.number_of_frame_buffers, 0);
	if (p->semaphore_can_put < 0)
		{
		pmp_play_close(p,0);
		return("pmp_play_open: sceKernelCreateSema failed on semaphore_can_put");
		}


	p->semaphore_can_show = sceKernelCreateSema("can_show", 0, 0, 1, 0);
	if (p->semaphore_can_show < 0)
		{
		pmp_play_close(p,0);
		return("pmp_play_open: sceKernelCreateSema failed on semaphore_can_show");
		}


	p->semaphore_show_done = sceKernelCreateSema("show_done", 0, 0, 1, 0);
	if (p->semaphore_show_done < 0)
		{
		pmp_play_close(p,0);
		return("pmp_play_open: sceKernelCreateSema failed on semaphore_show_done");
		}




	p->output_thread = sceKernelCreateThread("output", pmp_output_thread, 0x8, 0x10000, 0, 0);
	if (p->output_thread < 0)
		{
		pmp_play_close(p,0);
		return("pmp_play_open: sceKernelCreateThread failed on output_thread");
		}


	p->show_thread = sceKernelCreateThread("show", pmp_show_thread, 0x8, 0x10000, 0, 0);
	if (p->show_thread < 0)
		{
		pmp_play_close(p,0);
		return("pmp_play_open: sceKernelCreateThread failed on show_thread");
		}


	p->input_thread = sceKernelCreateThread("input", pmp_input_thread, 0x4, 0x10000, 0, 0);
	if (p->input_thread < 0)
		{
		pmp_play_close(p,0);
		return("pmp_play_open: sceKernelCreateThread failed on input_thread");
		}




	p->return_request = 0;
	p->return_result  = 0;


	p->paused = 0;
	p->seek   = 0;


	return(0);
	}
