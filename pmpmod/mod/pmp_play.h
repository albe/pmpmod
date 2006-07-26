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


#ifndef pmp_play_h
#define pmp_play_h


#include <pspthreadman.h>
#include <pspaudio.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psppower.h>
#include <pspiofilemgr.h>
#include <pspiofilemgr_fcntl.h>
#include "pmp_decode.h"


struct pmp_play_struct
	{
	struct pmp_decode_struct decoder;


    int audio_reserved;

	SceUID semaphore_can_get;
	SceUID semaphore_can_put;
	SceUID semaphore_can_show;
	SceUID semaphore_show_done;

	SceUID output_thread;
	SceUID show_thread;
	SceUID input_thread;


	int   return_request;
	char *return_result;
	
	
	int paused;
	int seek;

	//--------------------------------
	// mod :
	int		last_keyframe_pos;
	int		resume_pos;
enum
{
	ZOOM_NONE = 0,
	ZOOM_WIDTH,
	ZOOM_HEIGHT,
	ZOOM_FILL,
	ZOOM_MAX
} eZoom;
	int		zoom_mode;
	int		zoom_mode_prev;
	char	resume_filename[256];


	unsigned char*	interface_texture;
	int				current_video_frame;
	int				display_interface;
	int				interface_pos;

	};


void pmp_play_safe_constructor(struct pmp_play_struct *p);
char *pmp_play_open(struct pmp_play_struct *p, char *s, int usePos);
void pmp_play_close(struct pmp_play_struct *p, int usePos);
char *pmp_play_start(volatile struct pmp_play_struct *p);


#endif
