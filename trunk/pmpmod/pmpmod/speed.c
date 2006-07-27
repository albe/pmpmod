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


#include <string.h>
#include <pspdebug.h>
#include <psppower.h>
#include <psprtc.h>
#include "avformat.h"
#include "me.h"
#include "me_idct.h"
#include "pmp.h"
#include "opendir.h"


PSP_MODULE_INFO("pmpmod", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU);


#define video_directory "ms0:/PSP/VIDEO/640x384_30.pmp"


void pmp_pause()
	{
	while (1)
		{
		SceCtrlData controller;
		sceCtrlReadBufferPositive(&controller, 1);

		if (controller.Buttons & PSP_CTRL_CROSS || controller.Buttons & PSP_CTRL_HOME)
			{
			break;
			}
		}
	}


void screen_init()
	{
	pspDebugScreenSetTextColor(0xffffff);
	pspDebugScreenSetBackColor(0x000000);
	pspDebugScreenInit();
	}


void print_error(char *s)
	{
	screen_init();
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenPrintf("Error:\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("\"%s\"\n", s);
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press X");
	pmp_pause();
	}


char *static_init()
	{
	scePowerSetClockFrequency(333, 333, 166);
	scePowerLock(0);


	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);


	av_register_all();


	if (me_struct_init() == 0)
		{
		return("static_init: me_struct_init failed");
		}


	return(0);
	}


int init()
	{
	char *result = static_init();
	if (result != 0)
		{
        print_error(result);

		return(0);
		}


	return(1);
	}


void main_loop()
	{
	screen_init();


	struct pmp_decode_struct decode;
	char *result = pmp_decode_open(&decode, video_directory);
	if (result != 0)
		{
        print_error(result);
		return;
		}


	unsigned long long tick_resolution = sceRtcGetTickResolution();
	unsigned long long tick_count      = 0;
	unsigned long long count           = 0;
	unsigned long long min             = 0;
	unsigned long long max             = 0;

	
	unsigned int current_buffer_number = 0;
	
	
	pmp_gu_init();


	int i = 0;
	for (; i < decode.reader.file.header.video.number_of_frames; i++)
		{
		unsigned long long t0;
		if (sceRtcGetCurrentTick(&t0) < 0)
			{
	        print_error("sceRtcGetCurrentTick failed");
			return;
			}


		char *result = pmp_decode_get(&decode, i, 0, 1, 0, 0, 100, 0, 0, 0);
		if (result != 0)
			{
	        print_error(result);
			return;
			}


		unsigned long long t1;
		if (sceRtcGetCurrentTick(&t1) < 0)
			{
	        print_error("sceRtcGetCurrentTick failed");
			return;
			}


		unsigned long long t = t1 - t0;
		tick_count += t;
		count++;
		

		if (i == 0)
			{
			min = t;
			max = t;
			}
		else
			{
			if (t < min) min = t;
			if (t > max) max = t;
			}




		unsigned long long m      = 1000;
		unsigned long long fps    = t   == 0 ? 1000000000 : m * tick_resolution / t;
	    unsigned long long minfps = min == 0 ? 1000000000 : m * tick_resolution / min;
	    unsigned long long maxfps = max == 0 ? 1000000000 : m * tick_resolution / max;
	    fps    = fps    > 1000000000 ? 1000000000 : fps;
	    minfps = minfps > 1000000000 ? 1000000000 : minfps;
	    maxfps = maxfps > 1000000000 ? 1000000000 : maxfps;


		pspDebugScreenSetXY(0, 0);
		pspDebugScreenPrintf("fps: %i.%03i  ", (int) (fps    / m), (int) (fps    % m));
		pspDebugScreenPrintf("max: %i.%03i  ", (int) (minfps / m), (int) (minfps % m));
		pspDebugScreenPrintf("min: %i.%03i  ", (int) (maxfps / m), (int) (maxfps % m));


		memcpy(decode.output_frame_buffers[current_buffer_number].video_frame, (void *) 0x04000000, 4 * 512 * 8);




		sceDisplaySetFrameBuf(decode.output_frame_buffers[current_buffer_number].video_frame, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);
		current_buffer_number = (current_buffer_number + 1) % decode.number_of_frame_buffers;


		//scePowerTick(0);


		SceCtrlData controller;
		sceCtrlReadBufferPositive(&controller, 1);
		if (controller.Buttons & PSP_CTRL_CROSS)
			{
			return;
			}
		}




	//extern void gprof_cleanup();
	//gprof_cleanup();




	pmp_gu_end();


	pmp_decode_close(&decode);




	unsigned long long frames = decode.reader.file.header.video.number_of_frames;
	unsigned long long m      = 1000;
	unsigned long long fps    = tick_count == 0 ? 1000000000 : m * frames * tick_resolution / tick_count;
    unsigned long long minfps = min        == 0 ? 1000000000 : m * tick_resolution / min;
    unsigned long long maxfps = max        == 0 ? 1000000000 : m * tick_resolution / max;
    fps    = fps    > 1000000000 ? 1000000000 : fps;
    minfps = minfps > 1000000000 ? 1000000000 : minfps;
    maxfps = maxfps > 1000000000 ? 1000000000 : maxfps;


	screen_init();
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenPrintf("fps: %i.%03i  ", (int) (fps    / m), (int) (fps    % m));
	pspDebugScreenPrintf("max: %i.%03i  ", (int) (minfps / m), (int) (minfps % m));
	pspDebugScreenPrintf("min: %i.%03i  ", (int) (maxfps / m), (int) (maxfps % m));
	pmp_pause();
	}


int main(int unused0, char *unused1[])
	{
	if (init())
		{
		main_loop();
		}


	sceKernelExitGame();
	return(0);
	}
