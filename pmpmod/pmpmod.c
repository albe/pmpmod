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
#include <psputility.h>
#include <psputils.h>
#include <time.h>
#include "avformat.h"
#include "me.h"
#include "me_idct.h"
#include "pmp.h"
#include "opendir.h"

/* Get the full path to EBOOT.PBP. */
char psp_full_path[1024 + 1];



PSP_MODULE_INFO("pmpmod", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

//#define PROFILE__ 1

#define video_directory "ms0:/PSP/VIDEO/"


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


#define USE_SKIN 1

#ifdef USE_SKIN

#include <pspgu.h>
#include <psprtc.h>
#include "gu_funcs.h"
#include "pmp_lib.h"

void* framebuffer = 0;
int volume = 127;		// 0-256
int scrollpos = 0;		// 0-256
int batpower = 4;		// Must be 1-4
int isplugged = 0;		// Is power cable plugged in?

unsigned char* skin_texture = NULL;
unsigned char* font_texture = NULL;

void skin_load() {
	// load the interface
	{
		skin_texture = (unsigned char*)malloc_64( 512*512*4 );
		memset( skin_texture, 0, 512*512*4 );

		SceUID	fd;
		char	filename[1024];
		strcpy( filename, psp_full_path );
		strcpy( filename + strlen(filename), "/skin.tga" );
		if ((fd = sceIoOpen( filename, PSP_O_RDONLY, 0777)))
		{
			unsigned char	tgaheader[18];

			sceIoRead( fd, tgaheader, 18 );	// skip the header, we know its 512x272 32bits
			sceIoRead( fd, skin_texture, 512*272*4 );
			sceIoClose( fd );
			
			int x,y;
			long offs = 0;
			char t;
			// Swap R and B
			for (y=0;y<272;y++) {
			 for (x=0;x<512;x++) {
			   t = skin_texture[offs];
			   skin_texture[offs] = skin_texture[offs+2];
			   skin_texture[offs+2] = t;
			   offs += 4;
			 }
			}
		}
	}
}

void gu_init()
{
	sceGuInit();

	sceGuStart(GU_DIRECT,g_list[0]);
	sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
	sceGuDispBuffer(480, 272, (void*)(512*272*4), 512);
	sceGuOffset(2048 - (480/2),2048 - (272/2));
	sceGuViewport(2048, 2048, 480, 272);
	sceGuScissor(0,0,480,272);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFrontFace(GU_CW);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_8888,0,0,0); // 32-bit RGBA
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA); // don't get influenced by any vertex colors
	sceGuTexFilter(GU_NEAREST,GU_NEAREST); // point-filtered sampling
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

	framebuffer = 0;
}

void skin_draw()
{
		sceGuStart(GU_DIRECT,g_list[g_list_index]);
		
		sceGuTexImage(0,512,512,512,skin_texture);

		sceGuDisable( GU_BLEND );
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		//sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		advancedBlit(0,0,480,272,0,0,480,272,32);		// copy full buffer to full screen
		
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuEnable( GU_BLEND );
		
		// Put Volume button in place:
		//spriteBlit( 170+56*volume/256, 243, 500, 0, 12, 13 );
		
		// Put Scrollbar in place:
		//spriteBlit( 462, 59+(scrollpos*(116-69)/256), 500, 13, 12, 69 );
		spriteBlit( 470, 74+(scrollpos*(105-46)/256), 506, 0, 6, 46 );	// argandona
		
		// Put Battery status icon in place:
		spriteBlit( 434, 4, 486, 272-batpower*18, 26, 18 );
		
		// Put Power Cable status icon in place:
		if (isplugged)
		  spriteBlit( 434, 3, 486, 272-4*18-21, 26, 21 );
		
		sceGuFinish();
		
		sceGuSync(0,0);
		pspDebugScreenSetOffset((int)framebuffer);
}

void flip()
{
		sceDisplayWaitVblankStart();
		framebuffer = sceGuSwapBuffers();
}


char* pmp_read_vid_header( struct pmp_lib_vid_struct *pmp_vid_header, char* s )
{
	FILE* f = fopen(s, "rb");
	if (f == 0)
		{
		return("pmp_lib_open: can't open file");
		}


	if (fread(pmp_vid_header, 1, sizeof(struct pmp_lib_vid_struct), f) != sizeof(struct pmp_lib_vid_struct))
		{
		return("pmp_lib_open: can't read video header");
		}
		
	fclose(f);
	return (0);
}
#endif

// Could be done easier, but this is to provide the possibility to further add complexity to the playlist (shuffle/sort by x?)
struct play_list_entry_struct {
	int index;				// playlist order index
};

struct play_list_struct {
	struct play_list_entry_struct* play_list_entry;
	int number_of_entries;
	int list_size;
};

void update_playlist( struct play_list_struct *play_list, int entry )
{
	if (!play_list) return;
	if (!play_list->play_list_entry) return;
	if (play_list->number_of_entries==0) {
		// List is empty, so quickly add current entry to list and finish
		play_list->play_list_entry[entry].index = 0;
		play_list->number_of_entries++;
	} else {
		int index;
		if ((index=play_list->play_list_entry[entry].index)<0) {
			// entry not yet selected, so add it 
			play_list->play_list_entry[entry].index = play_list->number_of_entries++;
		} else {
			// entry was already selected, so remove it and move all following entries one back
			int i = 0;
			for (;i < play_list->list_size;i++)
				if (play_list->play_list_entry[i].index > index)
					play_list->play_list_entry[i].index--;
			play_list->play_list_entry[entry].index = -1;
			play_list->number_of_entries--;
		}
	}
	
}

void reorder_playlist( struct play_list_struct *play_list, int entry, int dir )
{
	if (!play_list) return;
	if (!play_list->play_list_entry || play_list->number_of_entries==0) return;
	if (play_list->play_list_entry[entry].index<0) return;	// entry isn't in playlist
	
	if (dir>0) dir = 1; else dir = -1;		// for now only move entries by one each time
	if (play_list->play_list_entry[entry].index+dir<0 || play_list->play_list_entry[entry].index+dir>=play_list->number_of_entries) return;	// entry is already on top/bottom

	int i = 0;
	while (play_list->play_list_entry[i].index!=play_list->play_list_entry[entry].index+dir)
		i++;
	play_list->play_list_entry[i].index = play_list->play_list_entry[entry].index;
	play_list->play_list_entry[entry].index += dir;
}



void screen_init()
	{
	#ifndef USE_SKIN
	pspDebugScreenSetTextColor(0xffffff);
	pspDebugScreenSetBackColor(0x000000);
	#else
	pspDebugScreenSetTextColor(0xffffff);
	pspDebugScreenSetBackColor(0x96440A);
	#endif
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


	if (me_idct_p_malloc() == 0)
		{
		return("static_init: me_idct_p_malloc failed");
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

#ifdef USE_SKIN
void print_about()
	{
	screen_init();
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenPrintf("PMP Mod v1.02 (M4g3) by jonny\n");
	pspDebugScreenPrintf("Additions by malloc and Raphael\n");
	pspDebugScreenPrintf("GUI graphics by argandona\n");
	pspDebugScreenPrintf("\n\n");
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
	pspDebugScreenPrintf("PMP Mod uses libavcodec from FFMPEG (http://ffmpeg.sourceforge.net/)\n");
	pspDebugScreenPrintf("Many thanks goes to:\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("- FFMPEG developers\n");
	pspDebugScreenPrintf("- JiniCho (PMP Mod is a modified version of his original FFMPEG and\n");
	pspDebugScreenPrintf("  FFPLAY psp port)\n");
	pspDebugScreenPrintf("- ps2dev.org developers and users\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("                                   http://jonny.leffe.dnsalias.com/");
	
	}
#else
void print_pmp(int licence)
	{
	screen_init();
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenPrintf("PMP Mod v1.02 (M4r3) by jonny\n");
	pspDebugScreenPrintf("Some additions by malloc and Raphael\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("HOME = exit");
	
	
	if (licence == 0)
		{
		pspDebugScreenPrintf(", X = play, SQUARE = licence, START = refresh list");
		}


	pspDebugScreenSetXY(0, 23);
	pspDebugScreenPrintf("PMP Mod uses libavcodec from FFMPEG (http://ffmpeg.sourceforge.net/)\n");
	pspDebugScreenPrintf("Many thanks goes to:\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("- FFMPEG developers\n");
	pspDebugScreenPrintf("- JiniCho (PMP Mod is a modified version of his original FFMPEG and\n");
	pspDebugScreenPrintf("  FFPLAY psp port)\n");
	pspDebugScreenPrintf("- ps2dev.org developers and users\n");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("                                   http://jonny.leffe.dnsalias.com/");
	}
#endif

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



//------------------------------------

void initUSBdrivers(void)
{
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
// prototype in pspsdk has only one arg, is that correct?
//      sceUsbstorBootSetCapacity(0x00800000,ret);
      sceUsbstorBootSetCapacity(0x00800000);
   }

	sceUsbActivate(0x1c8);
} 

void exitUSBdrivers(void)
{
   sceUsbDeactivate();

// FIXME: last arg should be NULL (wrong prototype in pspsdk?)
   sceUsbStop("USBStor_Driver",0,0);
   sceUsbStop("USBBusDriver",0,0);
} 

int	strcmpupr( char* s1, char* s2 )
{
	while (*s1 && *s2)
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

	if (*s1)
		return 1;
	if (*s2)
		return -1;

	return 0;
}

//------------------------------

void main_loop()
	{

	#ifdef USE_SKIN
	gu_init();
	screen_init();
	#endif

	//----------------
	initUSBdrivers();
	//----------------

	while (1)
		{

		#ifdef USE_SKIN
		skin_load();
		scrollpos = 0;
		skin_draw();
		#endif
		
		struct opendir_struct directory;

		char *result = opendir_open(&directory, video_directory);
		if (result != 0)
			{
			#ifndef USE_SKIN
			print_pmp(1);
			pspDebugScreenSetXY(0, 4);
			#else
			pspDebugScreenSetXY(2, 4);
			#endif
			pspDebugScreenPrintf("\"%s\" not found or empty", video_directory);
			pspDebugScreenPrintf("Error: %s", result);
			#ifndef USE_SKIN
			pspDebugScreenSetXY(0, 8);
			#else
			pspDebugScreenSetXY(2, 8);
			#endif
			print_licence();

			//----------------
			exitUSBdrivers();
			//----------------
			opendir_close(&directory);
			free_64(skin_texture);
			pmp_pause();
			return;
			}


		//--------------------------
		// sort files
		int		number_of_files = 0;
		char**	sorted_files = NULL;
		SceIoStat** sorted_stats = NULL;
		unsigned int total_size = 0;
		{
			int		i;

			for (i = 0; i < directory.number_of_directory_entries; i++)
			{
				char*	name = directory.directory_entry[i].d_name;

				if ((strcmp( name + strlen(name) - 4, ".PMP") == 0) ||
					(strcmp( name + strlen(name) - 4, ".pmp") == 0))
					number_of_files++;
			}

			if (number_of_files)
			{
				sorted_files = (char**)malloc_64( sizeof(char*) * number_of_files );
				sorted_stats = (SceIoStat**)malloc_64( sizeof(SceIoStat*) * number_of_files );

				int	n = 0;

				for (i = 0; i < directory.number_of_directory_entries; i++)
				{
					char	name[1024];
					strcpy( name, directory.directory_entry[i].d_name );
					int		len = strlen( name );

					if ((strcmp( name + len - 4, ".PMP") == 0) ||
						(strcmp( name + len - 4, ".pmp") == 0))
					{
						// save pointer to entry name
						sorted_stats[n]   = &directory.directory_entry[i].d_stat;
						total_size += (unsigned int)sorted_stats[n]->st_size / 1024;
						sorted_files[n++] = directory.directory_entry[i].d_name;
					}
				}

				// bubble sort
				int		swap = 1;
				while (swap)
				{
					swap = 0;

					for (i = 0; i < number_of_files - 1; i++)
					{
						if (strcmpupr( sorted_files[i] , sorted_files[i+1]) > 0)
						{
							swap = 1;
							
							SceIoStat *temp = sorted_stats[i];
							sorted_stats[i] = sorted_stats[i+1];
							sorted_stats[i+1] = temp;
							
							char*	name = sorted_files[i];
							sorted_files[i] = sorted_files[i+1];
							sorted_files[i+1] = name;
						}
					}
				}
			}		
		}

		//--------------------------
		struct play_list_struct play_list;
		play_list.play_list_entry = NULL;
		if (number_of_files) {
			play_list.play_list_entry = malloc_64(number_of_files * sizeof(struct play_list_entry_struct));
			// Init playlist
			if (!play_list.play_list_entry) {
				print_error("main_loop: malloc_64 failed on play_list");
				return;
			}
			memset( play_list.play_list_entry, -1, number_of_files * sizeof(struct play_list_entry_struct) );
		}
		play_list.list_size = number_of_files;
		play_list.number_of_entries = 0;
		
		#ifdef USE_SKIN		
		sceIoSync("msfat0:", 1);
		unsigned int buf[5];
		unsigned int *pbuf = buf;
		sceIoDevctl("ms0:", 0x02425818, &pbuf, sizeof(pbuf), 0, 0);
		unsigned int total_free = buf[1]*buf[3]*buf[4] / 1024 / 1024;
		unsigned int total_space = buf[0]*buf[3]*buf[4] / 1024 / 1024;
		
		char tunit[3] = "Kb\0";
		if (total_size>1024) {
				total_size /= 1024;
				strcpy( tunit, "Mb");
				if (total_size>1024) {
					total_size /= 1024;
					strcpy( tunit, "Gb");
				}
		}
		
		char sel_unit[3];		// Unit of filesize of selected entry
		unsigned int sel_size = 0;
		#endif


		int last_selected          = -1;	// 
		int selected_entry         = 0;
		int top_entry              = 0;
		int maximum_number_of_rows = 14;
		int starting_row           = 7;
		unsigned int vwidth = 0, vheight = 0, vrate = 0, vframes = 0;		// Video Information
		scrollpos = 0;

		#ifndef USE_SKIN
		print_pmp(0);
		#endif


        SceCtrlData controller;

		while (1)
			{
			sceCtrlReadBufferPositive(&controller, 1);

			if (controller.Buttons & PSP_CTRL_CROSS || controller.Buttons & PSP_CTRL_HOME || controller.Buttons & PSP_CTRL_SQUARE  || controller.Buttons & PSP_CTRL_START || controller.Buttons & PSP_CTRL_CIRCLE)
				{
				break;
				}

			if (controller.Buttons & PSP_CTRL_DOWN)
				{
				if (selected_entry + 1 < number_of_files)
					{
					selected_entry++;

					if (selected_entry == top_entry + maximum_number_of_rows)
						{
						top_entry++;
						}
					}
				}


			if (controller.Buttons & PSP_CTRL_UP)
				{
				if (selected_entry != 0)
					{
					selected_entry--;

					if (selected_entry == top_entry - 1)
						{
						top_entry--;
						}
					}
				}
				
			if (play_list.play_list_entry) {
			if (controller.Buttons & PSP_CTRL_LTRIGGER && play_list.play_list_entry[selected_entry].index>=0)
				{
					// Move entry one up
					reorder_playlist( &play_list, selected_entry, -1 );
				}
				
			if (controller.Buttons & PSP_CTRL_RTRIGGER && play_list.play_list_entry[selected_entry].index>=0)
				{
					// Move entry one down
					reorder_playlist( &play_list, selected_entry, 1 );
				}
			
			if (controller.Buttons & PSP_CTRL_TRIANGLE)
				{
				update_playlist( &play_list, selected_entry );
				sceKernelDelayThread(100000);	// Add some delay so a slightly too long press isn't recognized as two single presses (check + instant uncheck)
				}
			}
		
			#ifdef USE_SKIN
			// Update Video Information of selected Video
			if (sorted_files)
			if (last_selected!=selected_entry && sorted_files[selected_entry]) {
				last_selected = selected_entry;
				struct pmp_lib_vid_struct *pmp_struc = malloc_64(sizeof(struct pmp_lib_vid_struct));
				if (!pmp_struc) {
					print_error("main_loop: malloc_64 failed on pmp_struc");
					return;
				}
				if (pmp_read_vid_header(pmp_struc, sorted_files[selected_entry])==0) {
					vwidth = pmp_struc->width;
					vheight = pmp_struc->height;
					if (pmp_struc->scale)
					  vrate = pmp_struc->rate / pmp_struc->scale;
					else
					  vrate = pmp_struc->rate;
					vframes = pmp_struc->number_of_frames;
				}
				free_64(pmp_struc);
			}
						

			// Battery Power Calculation
			// Battery Icon will have:
			//   - empty (no bars) if Battery is below 25 percent
			//   - one bar if battery is below 50 percent
			//   - two bars if battery is below 75 percent
			//   - three bars if battery is over or equal 75 percent
			if (scePowerIsLowBattery() || !scePowerIsBatteryExist())
			  batpower = 1;
			else
			  batpower = (scePowerGetBatteryLifePercent()*4)/100 + 1;
			if (batpower>4) batpower=4;
			
			isplugged = scePowerIsPowerOnline();
			
			if (number_of_files<=maximum_number_of_rows)
			{
				scrollpos = 2048;	// not visible
			} else {
				scrollpos = 256 * top_entry / (number_of_files - maximum_number_of_rows);
			}
			skin_draw();
			
			
			// Battery Power Display
			// --------------------------------
			int battime = scePowerGetBatteryLifeTime();
			pspDebugScreenSetXY(61, 3);
			// argandona skin colors:
			pspDebugScreenSetTextColor(0xffffff);
			pspDebugScreenSetBackColor(0x914006);
			if (battime>0 && battime<24*60)
			pspDebugScreenPrintf("%2i:%02i", battime/60,battime%60);
			
			
			// Time/Date Display
			// --------------------------------
			struct tm *date;
			time_t tp;
			tp = sceKernelLibcTime(&tp);
			date = gmtime(&tp);
			int val;
			pspTime	clock;
			sceRtcGetCurrentClockLocalTime( &clock );
			date->tm_hour = clock.hour;
			date->tm_min = clock.minutes;
			date->tm_sec = clock.seconds;
			
			char datefmt[6] = "\0";
			char timefmt[11] = "\0";
			sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_DATE_FORMAT, &val);
			switch (val) {
				case PSP_SYSTEMPARAM_DATE_FORMAT_YYYYMMDD: strftime(datefmt,6,"%m/%d", date); break;
				case PSP_SYSTEMPARAM_DATE_FORMAT_MMDDYYYY: strftime(datefmt,6,"%m/%d", date); break;
				case PSP_SYSTEMPARAM_DATE_FORMAT_DDMMYYYY: strftime(datefmt,6,"%d/%m", date); break;
			};
			sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_TIME_FORMAT, &val);
			switch (val) {
				case PSP_SYSTEMPARAM_TIME_FORMAT_24HR: strftime(timefmt,6,"%H:%M", date); pspDebugScreenSetXY(50, 1); break;
				case PSP_SYSTEMPARAM_TIME_FORMAT_12HR: strftime(timefmt,11,"%I:%M%p", date); pspDebugScreenSetXY(48, 1); break;
			};
			pspDebugScreenPrintf("%5.5s %s", datefmt, timefmt);
			
			
			
			pspDebugScreenSetXY(1, starting_row);
			// argandona skin colors:
			pspDebugScreenSetTextColor(0xffffff);
			pspDebugScreenSetBackColor(0x9B4307);
			// WMP skin colors:
			//pspDebugScreenSetTextColor(0x000000);
			//pspDebugScreenSetBackColor(0xf0f0f0);
			pspDebugScreenPrintf("%-39.39s %8.8s  %-16.16s", "Filename", "Size", "Date");
			#else
			pspDebugScreenSetXY(1, starting_row);
			pspDebugScreenSetTextColor(0xffffff);
			pspDebugScreenSetBackColor(0xaa4400);
			pspDebugScreenPrintf("%-36.36s %8.8s  %-16.16s", "Filename", "Size", "Date");
			#endif


			int i = 0;
			for (; i < maximum_number_of_rows; i++)
				{
				int current_entry = top_entry + i;

				#ifdef USE_SKIN
				pspDebugScreenSetXY(1, starting_row + i + 2);
				#else
				pspDebugScreenSetXY(1, starting_row + i + 1);
				#endif

				if (current_entry < number_of_files)
					{
					if (play_list.play_list_entry[current_entry].index<0)
						pspDebugScreenSetTextColor(0xffffff);
					else
						pspDebugScreenSetTextColor(0x2CE4FF);

					
					if (current_entry == selected_entry)
						{
						#ifdef USE_SKIN
						// argandona skin colors
						//pspDebugScreenSetTextColor(0xffffff);
						pspDebugScreenSetBackColor(0xE88449);
						// WMP skin colors
						//pspDebugScreenSetTextColor(0x79420E);
						//pspDebugScreenSetBackColor(0xff7f7f);
						#else
						//pspDebugScreenSetTextColor(0xffffff);
						pspDebugScreenSetBackColor(0x882200);
						#endif
						}
					else
						{
						#ifdef USE_SKIN
						// argandona skin colors
						//pspDebugScreenSetTextColor(0xffffff);
						pspDebugScreenSetBackColor(0xB0571E);
						// WMP skin colors
						//pspDebugScreenSetTextColor(0x79420E);
						//pspDebugScreenSetBackColor(0xF9F2ED);
						#else
						//pspDebugScreenSetTextColor(0xffffff);
						pspDebugScreenSetBackColor(0xcc6600);
						#endif
						}


					if (sorted_stats) {
					char unit[3] = "Mb\0";
					int size = (int)sorted_stats[current_entry]->st_size/1024/1024;
					if (size<1) {
						size = (int)sorted_stats[current_entry]->st_size/1024;
						strcpy( unit, "Kb");
					}
					if (size>1024) {
						size /= 1024;
						strcpy( unit, "Gb");
					}
					if (current_entry==selected_entry) {
						sel_size = size;
						strcpy( sel_unit, unit );
					}
					pspDebugScreenPrintf("%-39.39s %6i%2s  %02i.%02i.%-4i %02i:%02i", sorted_files[current_entry],
								size, unit,
								sorted_stats[current_entry]->st_mtime.day,
								sorted_stats[current_entry]->st_mtime.month,
								sorted_stats[current_entry]->st_mtime.year,
								sorted_stats[current_entry]->st_mtime.hour,
								sorted_stats[current_entry]->st_mtime.minute);
					}
					//pspDebugScreenPrintf("%-66.66s", sorted_files[current_entry]);
					}
				else
					{
					#ifdef USE_SKIN
					pspDebugScreenSetTextColor(0xffffff);
					pspDebugScreenSetBackColor(0xB0571E);
					//pspDebugScreenPrintf("%-66.66s", "");
					#else
					pspDebugScreenSetTextColor(0xffffff);
					pspDebugScreenSetBackColor(0xaa4400);
					pspDebugScreenPrintf("%-66.66s", "");
					#endif
					}
				}

			#ifndef USE_SKIN
			pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 1);
			pspDebugScreenSetTextColor(0xffffff);
			pspDebugScreenSetBackColor(0xaa4400);
			pspDebugScreenPrintf("%-66.66s", "");
			#else
			pspDebugScreenSetXY(2, starting_row + maximum_number_of_rows + 3);
			// argandona skin colors:
			pspDebugScreenSetTextColor(0xffffff);
			pspDebugScreenSetBackColor(0x91400B);
			// WMP skin colors:
			//pspDebugScreenSetTextColor(0xffffff);
			//pspDebugScreenSetBackColor(0xB36A36);
			
			// Display General Information
			pspDebugScreenPrintf("%3i Item(s) %40.40s %4i%s", number_of_files, "Total Size:", total_size, tunit);
			pspDebugScreenSetXY(2, starting_row + maximum_number_of_rows + 4);
			pspDebugScreenPrintf("%52.52s %4iMb (%i%%)", "Total Free:", total_free, total_free*100/total_space );
			
			// Display Video Information
			if (vwidth && vheight && vrate && vframes) {
				pspDebugScreenSetXY(2, starting_row + maximum_number_of_rows + 7);
				pspDebugScreenPrintf("%ix%i %ifps, %i:%02i:%02i (%i%s)", vwidth, vheight, vrate, (vframes/vrate/3600),(vframes/vrate/60)%60, (vframes/vrate)%60, sel_size, sel_unit); 
			}

			// Display Playlist Information
			if (play_list.number_of_entries==0) {
				pspDebugScreenSetXY(2, starting_row + maximum_number_of_rows + 8);
				pspDebugScreenPrintf("Playlist empty, press Triangle to add/remove and R/L to reorder");
			} else if (play_list.play_list_entry) {
				if (play_list.play_list_entry[selected_entry].index>=0) {
					pspDebugScreenSetXY(2, starting_row + maximum_number_of_rows + 8);
					pspDebugScreenPrintf("Playlist position: %i of %i", play_list.play_list_entry[selected_entry].index+1,play_list.number_of_entries);
				} else {
					pspDebugScreenSetXY(2, starting_row + maximum_number_of_rows + 8);
					pspDebugScreenPrintf("%i Item(s) in playlist", play_list.number_of_entries);
				}
			}
			
			// Display USB Connection Status
			if ((sceUsbGetState()&PSP_USB_CONNECTION_ESTABLISHED)==PSP_USB_CONNECTION_ESTABLISHED) {
				pspDebugScreenSetXY(2, starting_row + maximum_number_of_rows + 9);
				pspDebugScreenPrintf("USB Status: Connected");
			} else {
				pspDebugScreenSetXY(2, starting_row + maximum_number_of_rows + 9);
				pspDebugScreenPrintf("USB Status: Disconnected");
			}


			flip();
			#endif

			sceKernelDelayThread(50000);
			}


		free_64(sorted_stats);
		#ifdef USE_SKIN
		free_64(skin_texture);
		#endif
		
		if (controller.Buttons & PSP_CTRL_HOME)
			{
			//-----------------
			free_64(sorted_files);
			free_64(play_list.play_list_entry);
			//-----------------
			opendir_close(&directory);

			break;
			}


		if (controller.Buttons & PSP_CTRL_SQUARE)
			{
			#ifndef USE_SKIN
			screen_init();
			pspDebugScreenSetXY(0, 0);
			print_licence();
			pspDebugScreenPrintf("Press X");
			#else
			print_about();
			pspDebugScreenPrintf("\nPress X");
			#endif
			pmp_pause();
			#ifndef USE_SKIN
			screen_init();
			#endif
			//-----------------
			free_64(sorted_files);
			free_64(play_list.play_list_entry);
			//-----------------
			opendir_close(&directory);
			sceKernelDelayThread(1000000);

			continue;
			}

		// just refresh
		if (controller.Buttons & PSP_CTRL_START)	
			{
			//-----------------
			free_64(sorted_files);
			free_64(play_list.play_list_entry);
			//-----------------
			opendir_close(&directory);
			continue;
			}
			
		// no files in list, don't try to play anything
		if (!number_of_files || !play_list.play_list_entry)
			{
			//-----------------
			free_64(sorted_files);
			free_64(play_list.play_list_entry);
			//-----------------
			opendir_close(&directory);
			continue;
			}


		// Only leave USB connection during playback, not for just refreshing filelist
		//-----------------
		exitUSBdrivers();
		//-----------------

		int usePos = 1;
		if (controller.Buttons & PSP_CTRL_CIRCLE)
			usePos = 0;

		// No entry was added to playlist, but X or O was pressed, so play the current selected file
		if (play_list.number_of_entries==0) {
			play_list.number_of_entries++;
			play_list.play_list_entry[selected_entry].index = 0;
		}

		pspDebugScreenSetTextColor(0xffffff);
		pspDebugScreenSetBackColor(0x000000);
		pspDebugScreenInit();
		pspDebugScreenSetXY(0, 0);
		pspDebugScreenPrintf("Loading ...");
		sceKernelDelayThread(100000);
		
		// Playlist code here
		int i = 0;
		for (;i<play_list.number_of_entries;i++) {
		
		int play_entry = 0;
		// find playlist entry with index i
		while (play_list.play_list_entry[play_entry].index!=i)
			play_entry++;
		
		#ifdef PROFILE__
		/* Clear the existing profile regs */
		pspDebugProfilerClear();
		/* Enable profiling */
		pspDebugProfilerEnable();
		#endif
		result = pmp_play(sorted_files[play_entry], usePos);
		if (result != 0)
			{
			sceKernelDelayThread(1000000);
			print_error(result);
			screen_init();
			sceKernelDelayThread(1000000);
			}

		#ifdef PROFILE__
		flip();
		pspDebugScreenSetOffset((int)framebuffer);
		pspDebugScreenInit();
		pspDebugScreenSetXY(0, 0);
		pspDebugProfilerPrint();
		pspDebugProfilerDisable();
		flip();
		pmp_pause();
		controller.Buttons = 0;
		#endif
		
		} // Playlist

		//-----------------
		free_64(sorted_files);
		free_64(play_list.play_list_entry);
		//-----------------
		
		
		
		opendir_close(&directory);
		

		//----------------
		initUSBdrivers();
		//----------------

		}
		
		//-----------------
		exitUSBdrivers();
		//-----------------

		#ifdef USE_SKIN
		sceGuTerm();
		#endif
	}


int main(int argc, char *argv[])
	{
	char *psp_eboot_path;
 
	strncpy(psp_full_path, argv[0], sizeof(psp_full_path) - 1);
	psp_full_path[sizeof(psp_full_path) - 1] = '\0';
 
	psp_eboot_path = strrchr(psp_full_path, '/');
	if (psp_eboot_path != NULL)
	    {
		*psp_eboot_path = '\0';
		} 


	if (init())
		{
		main_loop();
		}


	sceKernelExitGame();
	return(0);
	}
