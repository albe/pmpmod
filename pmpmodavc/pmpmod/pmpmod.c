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


#include <pspdebug.h>
#include <psppower.h>
#include <pspusb.h>
#include <pspusbstor.h>
#include <time.h>
#include <psputility.h>
#include <psputils.h>
#include "avformat.h"
#include "me.h"
#include "pmp.h"
#include "opendir.h"
#include "avc.h"
#include "cpu_clock.h"
#include "pmp_file.h"
#include "pmp_stat.h"

#include "valloc.h"
#include "gu_font.h"


PSP_MODULE_INFO("pmpmod", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(0);


#define video_directory "ms0:/PSP/VIDEO/"


void pmp_pause()
	{
	while (1)
		{
		SceCtrlData controller;
		sceCtrlReadBufferPositive(&controller, 1);

		if (controller.Buttons & PSP_CTRL_CROSS || controller.Buttons & PSP_CTRL_TRIANGLE)
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


int pspDateFmt, pspTimeFmt;

char *static_init()
	{
	cpu_clock_set_minimum();
	scePowerLock(0);


	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);


	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_DATE_FORMAT, &pspDateFmt);
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_TIME_FORMAT, &pspTimeFmt);
	
	av_register_all();

	// Static VRAM allocations needed in pmp_gu.c
	valloc(4 *  512 * 272);	// Drawbuffer
	valloc(4 *  512 * 272); // AVCbuffer
	
	
	gu_font_init();
	char *result=gu_font_load("font10.f");
	if (result!=0)
		{
		return(result);
		}


	result = avc_static_init();


	return(result);
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


//------------------------------------

int usbActive = 0;

void initUSBdrivers(void)
	{
	if (usbActive) return;
	int exec_pu;
	
	exec_pu=sceKernelLoadModule("flash0:/kd/semawm.prx",0,NULL);
	sceKernelStartModule(exec_pu, 0, NULL, 0, NULL);
	
	exec_pu=sceKernelLoadModule("flash0:/kd/usbstor.prx",0,NULL);
	sceKernelStartModule(exec_pu, 0, NULL, 0, NULL);
	
	exec_pu=sceKernelLoadModule("flash0:/kd/usbstormgr.prx",0,NULL);
	sceKernelStartModule(exec_pu, 0, NULL, 0, NULL);
	
	exec_pu=sceKernelLoadModule("flash0:/kd/usbstorms.prx",0,NULL);
	sceKernelStartModule(exec_pu, 0, NULL, 0, NULL);
	
	exec_pu=sceKernelLoadModule("flash0:/kd/usbstorboot.prx",0,NULL);
	sceKernelStartModule(exec_pu, 0, NULL, 0, NULL);

	int ret = 0;
	
	// FIXME: last arg should be NULL (wrong prototype in pspsdk?)
	sceUsbStart("USBBusDriver",0,0);
	ret = sceUsbStart("USBStor_Driver",0,0);
	
	if(ret)
	{
      sceUsbstorBootSetCapacity(0x00800000);
	}

	sceUsbActivate(0x1c8);
	usbActive = 1;
	} 


void exitUSBdrivers(void)
	{
	if (!usbActive) return;
	sceUsbDeactivate(0x1c8);

	// FIXME: last arg should be NULL (wrong prototype in pspsdk?)
	sceUsbStop("USBStor_Driver",0,0);
	sceUsbStop("USBBusDriver",0,0);
	usbActive = 0;
	} 


char* pmp_read_vid_header( struct pmp_header_struct *pmp_vid_header, char* s )
	{
	SceUID	fd;
	
	if((fd = sceIoOpen( s, PSP_O_RDONLY, 0777)))
		{
		if (sceIoRead( fd, pmp_vid_header, sizeof(struct pmp_header_struct) )!=sizeof(struct pmp_header_struct))
			return("pmp_lib_open: can't read video header");
		sceIoClose(fd);
		}
	else
		return("pmp_lib_open: can't open file");
	return(0);
	}


void print_pmp(int licence)
	{
	screen_init();
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenPrintf("PMP Mod AVC v1.02M by jonny\n");
	pspDebugScreenPrintf("subtitle+ui mod by Raphael\n");
	pspDebugScreenPrintf("Press TRIANGLE to exit");
	
	
	if (licence == 0)
		{
		pspDebugScreenPrintf(", X or O to play, SQUARE to see the licence");
		}


	pspDebugScreenSetXY(0, 24);
	pspDebugScreenPrintf("Many thanks goes to:\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("- magiK (PMF Player author) for unlocking the avc api and\n");
	pspDebugScreenPrintf("  x264 compatibility, great work\n");
	pspDebugScreenPrintf("- FFMPEG developers\n");
	pspDebugScreenPrintf("- JiniCho (PMP Mod is a modified version of his original FFMPEG and\n");
	pspDebugScreenPrintf("  FFPLAY psp port)\n");
	pspDebugScreenPrintf("- ps2dev.org developers and users\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("                                   http://jonny.leffe.dnsalias.com/");
	}


void print_licence()
	{
	pspDebugScreenPrintf("This program is free software; you can redistribute it and/or modify");
	pspDebugScreenPrintf("it under the terms of the GNU General Public License as published by");
	pspDebugScreenPrintf("the Free Software Foundation; either version 2 of the License, or\n");
	pspDebugScreenPrintf("(at your option) any later version.\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("This program is distributed in the hope that it will be useful,\n");
	pspDebugScreenPrintf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	pspDebugScreenPrintf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	pspDebugScreenPrintf("GNU General Public License for more details.\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("You should have received a copy of the GNU General Public License\n");
	pspDebugScreenPrintf("along with this program; if not, write to the Free Software\n");
	pspDebugScreenPrintf("Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n");
	pspDebugScreenPrintf("\n");
	}


int print_confirmation( char* s )
	{
	pspDebugScreenSetTextColor(0xffffff);
	pspDebugScreenSetBackColor(0x000000);
	pspDebugScreenSetXY(12, 12);
	pspDebugScreenPrintf("%-44.44s","");
	pspDebugScreenSetXY(12, 13);
	pspDebugScreenPrintf("%-44.44s"," Are you sure you want to delete the file");
	pspDebugScreenSetXY(12, 14);
	pspDebugScreenPrintf("%-44.44s","");
	pspDebugScreenSetXY(12, 14);
	pspDebugScreenPrintf(" '%s'?", s);
	pspDebugScreenSetXY(12, 15);
	pspDebugScreenPrintf("%-44.44s","");
	pspDebugScreenSetXY(12, 16);
	pspDebugScreenPrintf("%-44.44s"," Press X to accept, any other to cancel.");
	pspDebugScreenSetXY(12, 17);
	pspDebugScreenPrintf("%-44.44s","");
	
	sceKernelDelayThread(2000000);
	while (1)
		{
		SceCtrlData controller;
		sceCtrlReadBufferPositive(&controller, 1);

		if (controller.Buttons & PSP_CTRL_CROSS)
			return(1);
		if (controller.Buttons & PSP_CTRL_CIRCLE || controller.Buttons & PSP_CTRL_TRIANGLE || controller.Buttons & PSP_CTRL_SQUARE || controller.Buttons & PSP_CTRL_START || controller.Buttons & PSP_CTRL_SELECT)
			return(0);
		}
	}


static int	strncmpupr( char* s1, char* s2, int n )
	{
	int i = 0;
	while (*s1 && *s2 && i++<n)
		{
		char c1 = *s1++;
		char c2 = *s2++;

		if ((c1 >= 'a') && (c1 <= 'z'))
			c1 -= 'a' - 'A';
		if ((c2 >= 'a') && (c2 <= 'z'))
			c2 -= 'a' - 'A';

		if (c1 > c2)
			return 1;
		if (c1 < c2)
			return -1;
		}
	
	if (i<n)
	{
		if (*s1==0 && *s2!=0)
			return -1;
		if (*s2==0 && *s1!=0)
			return 1;
	}
	return 0;
	}

struct speed_setting_struct speed_settings[] =
{
	{ 120, 120, 60 },
	{ 166, 166, 83 },
	{ 222, 222, 111 },
	{ 266, 266, 133 },
	{ 333, 333, 166 }
};

char* filelist_filter[] =
{
	".pmp",
	0,
};

char* filelist_sub_filter[] =
{
	".sub",
	".srt",
	0,
};

void pmp_fileinfo_printf( char *s, struct SceIoDirent *d )
	{
	
	// Time/Date Display
	// --------------------------------
	struct tm date;
	date.tm_hour = d->d_stat.st_mtime.hour;
	date.tm_min = d->d_stat.st_mtime.minute;
	date.tm_sec = d->d_stat.st_mtime.second;
	date.tm_year = d->d_stat.st_mtime.year-1900;
	date.tm_mon = d->d_stat.st_mtime.month;
	date.tm_mday = d->d_stat.st_mtime.day;
	
	char datefmt[11] = "\0";
	char timefmt[11] = "\0";
	switch (pspDateFmt) {
		case PSP_SYSTEMPARAM_DATE_FORMAT_YYYYMMDD: strftime(datefmt,11,"%Y/%m/%d", &date); break;
		case PSP_SYSTEMPARAM_DATE_FORMAT_MMDDYYYY: strftime(datefmt,11,"%m/%d/%Y", &date); break;
		case PSP_SYSTEMPARAM_DATE_FORMAT_DDMMYYYY: strftime(datefmt,11,"%d.%m.%Y", &date); break;
	};
	switch (pspTimeFmt) {
		case PSP_SYSTEMPARAM_TIME_FORMAT_24HR: strftime(timefmt,6,"%H:%M", &date); break;
		case PSP_SYSTEMPARAM_TIME_FORMAT_12HR: strftime(timefmt,11,"%I:%M%p", &date); break;
	};
	
	char datetime[64];
	snprintf(datetime, 64, "%s %s", datefmt, timefmt);
	
	char* unit = "Mb";
	int size = (int)d->d_stat.st_size/1024/1024;
	if (size<1)
		{
		size = (int)d->d_stat.st_size/1024;
		unit = "Kb";
		}
	else
	if (size>1024)
		{
		size /= 1024;
		unit = "Gb";
		}

	snprintf(s,256, "%-37.37s %6i%2s  %18.18s",
				d->d_name,
				size, unit,
				datetime);
	};

	
void main_loop()
	{
	struct opendir_struct directory;
	struct pmp_header_struct header;
	int vwidth = 0, vheight = 0, vframes = 0, vformat = 0, vpos = 0, vvol = 0, vaspect = 0, vzoom = 0, vnsubs = 0;
	float vrate = 0;


	int selected_entry         = 0;
	int selected_changed       = 1;
	int top_entry              = 0;
	int maximum_number_of_rows = 17;
	int starting_row           = 4;
	
	int max_speed_mode         = sizeof(speed_settings) / sizeof(struct speed_setting_struct);
	int speed_mode             = 0;
	int total_size             = 0;
	int update_delay           = 0;
	int first_frame            = 1;
	

	while (1)
		{
		char *result = opendir_open(&directory, video_directory, filelist_filter, SORT_NAME);
		if (result != 0)
			{
			print_pmp(1);
			pspDebugScreenSetXY(0, 4);
			pspDebugScreenPrintf("\"%s\" not found or empty", video_directory);
			pspDebugScreenSetXY(0, 8);
			print_licence();
			pmp_pause();
			return;
			}
		selected_changed = 1;
		
		//----------------
		initUSBdrivers();
		//----------------

		if (speed_mode>0)
			scePowerSetClockFrequency(120, 120, 60);
		sceKernelDelayThread(200000);
		
		print_pmp(0);
		

        SceCtrlData controller, previous;
		sceCtrlReadBufferPositive(&controller, 1);
		while (1)
			{
			if (update_delay)
				sceKernelDelayThread(update_delay);
			update_delay = 0;

			previous = controller;
			if (first_frame==0)
			while (1)
			{
				sceCtrlReadBufferPositive(&controller, 1);
				if (controller.Buttons!=0) break;
			}

			if (controller.Buttons & PSP_CTRL_CROSS || controller.Buttons & PSP_CTRL_CIRCLE || controller.Buttons & PSP_CTRL_TRIANGLE || controller.Buttons & PSP_CTRL_SQUARE || controller.Buttons & PSP_CTRL_START || controller.Buttons & PSP_CTRL_SELECT)
				{
				break;
				}


			if (controller.Buttons & PSP_CTRL_RTRIGGER)
				{
				speed_mode++;
				if (speed_mode>=max_speed_mode) speed_mode -= max_speed_mode;
				cpu_clock_set_speed( &speed_settings[speed_mode] );
				update_delay = 250000;
				}
			
			if (controller.Buttons & PSP_CTRL_LTRIGGER)
				{
				speed_mode--;
				if (speed_mode<0) speed_mode += max_speed_mode;
				cpu_clock_set_speed( &speed_settings[speed_mode] );
				update_delay = 250000;
				}
			
			
			if (controller.Buttons & PSP_CTRL_DOWN)
				{
				if (selected_entry + 1 < directory.number_of_directory_entries)
					{
					selected_entry++;
					selected_changed = 1;

					if (selected_entry == top_entry + maximum_number_of_rows)
						{
						top_entry++;
						}
						
					if ((previous.Buttons & PSP_CTRL_DOWN) == 0)
						update_delay = 330000;
					else
						update_delay = 10000;
					}
				}


			if (controller.Buttons & PSP_CTRL_UP)
				{
				if (selected_entry != 0)
					{
					selected_entry--;
					selected_changed = 1;

					if (selected_entry == top_entry - 1)
						{
						top_entry--;
						}
						
					if ((previous.Buttons & PSP_CTRL_UP) == 0)
						update_delay = 330000;
					else
						update_delay = 10000;
					}
				}

			if (selected_changed)
				{
				// Update selected file information
				if (pmp_read_vid_header(&header, directory.directory_entry[selected_entry].d_name)==0)
					{
					vformat = header.video.format;
					vwidth = header.video.width;
					vheight = header.video.height;
					if (header.video.scale)
					  vrate = (float)header.video.rate / (float)header.video.scale;
					else
					  vrate = (float)header.video.rate;
					if (vrate==0)
						vrate = 25;
					vframes = header.video.number_of_frames;
					selected_changed = 0;
					}
				pmp_stat_read( directory.directory_entry[selected_entry].d_name, &vpos, &vvol, &vaspect, &vzoom, NULL, NULL, NULL, NULL, NULL );
				
				struct opendir_struct dir;
				vnsubs = 0;
				char *result = opendir_open(&dir, video_directory, filelist_sub_filter, SORT_DEFAULT);
				if (result == 0)
					{
					char *fname = directory.directory_entry[selected_entry].d_name;
					char format[256];
					char* ext = strrchr(fname,'.');
					int format_sz = (ext-fname<256?ext-fname:255);
					strncpy( format, fname, format_sz );
					format[format_sz] = '\0';
					
					int i = 0;
					while (i < dir.number_of_directory_entries)
						if (strncmpupr(dir.directory_entry[i++].d_name,format,format_sz)==0)
						{
							vnsubs++;
							if (vnsubs>=MAX_SUBTITLES) break;
						}
					}
				opendir_close(&dir);
				}


			pspDebugScreenSetXY(1, starting_row);
			pspDebugScreenSetTextColor(0xffffff);
			pspDebugScreenSetBackColor(0xaa4400);
			pspDebugScreenPrintf("%-37.37s %8.8s  %-18.18s", "Filename", "Size", "Date");

			total_size = 0;
			int i = 0;
			for (; i < maximum_number_of_rows; i++)
				{
				int current_entry = top_entry + i;


				pspDebugScreenSetXY(1, starting_row + i + 1);

				if (current_entry < directory.number_of_directory_entries)
					{
					if (current_entry == selected_entry)
						{
						pspDebugScreenSetTextColor(0xffffff);
						pspDebugScreenSetBackColor(0x882200);
						}
					else
						{
						pspDebugScreenSetTextColor(0xffffff);
						pspDebugScreenSetBackColor(0xcc6600);
						}

					char fileinfo[256];
					pmp_fileinfo_printf( fileinfo, &directory.directory_entry[current_entry] );
					pspDebugScreenPrintf( fileinfo );
					total_size += (int)directory.directory_entry[current_entry].d_stat.st_size/1024;
					}
				else
					{
					pspDebugScreenSetTextColor(0xffffff);
					pspDebugScreenSetBackColor(0xcc6600);
					pspDebugScreenPrintf("%-66.66s", "");
					}
				}

			
			pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 1);
			pspDebugScreenSetTextColor(0xffffff);
			pspDebugScreenSetBackColor(0xaa4400);
			char speed[64];
			snprintf(speed,64,"speed setting: (%i,%i,%i)",speed_settings[speed_mode].cpu,speed_settings[speed_mode].ram,speed_settings[speed_mode].bus);
			
			sceIoSync("ms0:", 1);
			unsigned int buf[5];
			unsigned int *pbuf = buf;
			sceIoDevctl("ms0:", 0x02425818, &pbuf, sizeof(pbuf), 0, 0);
			unsigned int total_free = buf[1]*buf[3]*buf[4] / 1024 / 1024;
			unsigned int total_space = buf[0]*buf[3]*buf[4] / 1024 / 1024;
			
			char* tunit = "Kb";
			if (total_size>1024) {
					total_size /= 1024;
					tunit = "Mb";
					if (total_size>1024) {
						total_size /= 1024;
						tunit = "Gb";
					}
			}
			char info[64];
			snprintf(info,64,"Total: %i%s (%iMb free)", total_size, tunit, total_free/*100/total_space*/);
			// Display Video Information
			pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 1);
			pspDebugScreenPrintf(" %ix%i %s %.2ffps, %i:%02i:%02i %35.35s", vwidth, vheight, (vformat==1?"AVC":"PMP"), vrate, (int)(vframes/vrate/3600), (int)(vframes/vrate/60)%60, (int)(vframes/vrate)%60, info);
			
			char pos[64];
			pos[0]='\0';
			if (vpos != 0)
				{
					char* asp;
					if (vaspect==0) asp = "orig";
					else
					if (vaspect==1) asp = "16:9";
					else
					if (vaspect==2) asp = "4:3";
					else
						asp = "2.35";
					
					// Calculate saved position precisely with the float vrate value and display it
					snprintf(pos, 64, "| POS: %i:%02i:%02i @ %s", (int)(vpos/vrate/3600),(int)(vpos/vrate/60)%60, (int)(vpos/vrate)%60, asp);
				}
			
			// Display USB Connection Status
			if ((sceUsbGetState()&PSP_USB_CONNECTION_ESTABLISHED)==PSP_USB_CONNECTION_ESTABLISHED) {
				pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 2);
				pspDebugScreenPrintf(" %i subtitle(s) %-26.26s%25.25s", vnsubs, pos, "USB Status: Connected");
			} else {
				pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 2);
				pspDebugScreenPrintf(" %i subtitle(s) %-26.26s%25.25s", vnsubs, pos, "USB Status: Disconnected");
			}
			pspDebugScreenSetTextColor(0xffffff);
			pspDebugScreenSetBackColor(0x000000);
			pspDebugScreenSetXY(37, starting_row + maximum_number_of_rows + 3);
			pspDebugScreenPrintf("%30.30s", speed);
			
			//sceKernelDelayThread(100000);
			first_frame = 0;
			}



		first_frame = 1;
		if (controller.Buttons & PSP_CTRL_TRIANGLE)
			{
			opendir_close(&directory);
			break;
			}

		if (controller.Buttons & PSP_CTRL_START)
			{
			// refresh
			opendir_close(&directory);
			continue;
			}

		if (controller.Buttons & PSP_CTRL_SELECT)
			{
			// delete file
			if (print_confirmation(directory.directory_entry[selected_entry].d_name))
				sceIoRemove(directory.directory_entry[selected_entry].d_name);
	
			opendir_close(&directory);
			continue;
			}

		if (controller.Buttons & PSP_CTRL_SQUARE)
			{
			screen_init();
			pspDebugScreenSetXY(0, 0);
			print_licence();
			pspDebugScreenPrintf("Press X");
			pmp_pause();
			screen_init();
			sceKernelDelayThread(1000000);
			opendir_close(&directory);
			continue;
			}

		int usePos = 1;
		if (controller.Buttons & PSP_CTRL_CIRCLE)
			usePos = 0;


		//----------------
		exitUSBdrivers();
		//----------------
		
		screen_init();
		pspDebugScreenSetXY(0, 0);
		pspDebugScreenPrintf("Loading ...");

		if (speed_mode>0)
			cpu_clock_set_minimum();

		result = pmp_play(directory.directory_entry[selected_entry].d_name,usePos);
		if (result != 0)
			{
			sceKernelDelayThread(1000000);
			print_error(result);
			screen_init();
			sceKernelDelayThread(1000000);
			}
		opendir_close(&directory);
		}

	
	gu_font_close();
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
