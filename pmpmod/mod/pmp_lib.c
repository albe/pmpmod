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
.pmp header reading routines
*/


#include "pmp_lib.h"
#include "mem64.h"


void pmp_lib_safe_constructor(struct pmp_lib_struct *p)
	{
	p->f_vid     = 0;
	p->vid_index = 0;
	p->aud_index = 0;
	}


void pmp_lib_close(struct pmp_lib_struct *p)
	{
	if (p->f_vid     != 0) fclose(p->f_vid);
	if (p->vid_index != 0) free_64(p->vid_index);
	if (p->aud_index != 0) free_64(p->aud_index);


	pmp_lib_safe_constructor(p);
	}


char *pmp_lib_open(struct pmp_lib_struct *p, char *s)
	{
	pmp_lib_safe_constructor(p);




	p->f_vid = fopen(s, "rb");
	if (p->f_vid == 0)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: can't open file");
		}


	if (fread(&p->vid, 1, sizeof(struct pmp_lib_vid_struct), p->f_vid) != sizeof(struct pmp_lib_vid_struct))
		{
		pmp_lib_close(p);
		return("pmp_lib_open: can't read video header");
		}


	if (p->vid.pmpm    != 0x6d706d70) { pmp_lib_close(p); return("pmp_lib_open: not a valid pmp file"); }
	if (p->vid.version != 0)          { pmp_lib_close(p); return("pmp_lib_open: invalid file version"); }

	if (p->vid.number_of_frames   == 0) { pmp_lib_close(p); return("pmp_lib_open: number_of_frames == 0"); }
	if (p->vid.width              == 0) { pmp_lib_close(p); return("pmp_lib_open: width == 0"); }
	if (p->vid.height             == 0) { pmp_lib_close(p); return("pmp_lib_open: height == 0"); }
	if (p->vid.scale              == 0) { pmp_lib_close(p); return("pmp_lib_open: scale == 0"); }
	if (p->vid.rate               == 0) { pmp_lib_close(p); return("pmp_lib_open: rate == 0"); }
	if (p->vid.maximum_frame_size == 0) { pmp_lib_close(p); return("pmp_lib_open: maximum_frame_size == 0"); }

	if ((p->vid.width  % 8) != 0)      { pmp_lib_close(p); return("pmp_lib_open: (width % 8) != 0"); }
	if ((p->vid.height % 8) != 0)      { pmp_lib_close(p); return("pmp_lib_open: (height % 8) != 0"); }

	if (p->vid.rate < p->vid.scale)     { pmp_lib_close(p); return("pmp_lib_open: rate < scale"); }

	if (p->vid.width  > 480)            { pmp_lib_close(p); return("pmp_lib_open: width > 480"); }
	if (p->vid.height > 272)            { pmp_lib_close(p); return("pmp_lib_open: height > 272"); }
	
	if (p->vid.scale > 0xffffff)        { pmp_lib_close(p); return("pmp_lib_open: scale > 0xffffff"); }


	p->vid_index = malloc_64(sizeof(unsigned int) * p->vid.number_of_frames);
	if (p->vid_index == 0)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: malloc_64 failed on vid_index");
		}


	if (fread(p->vid_index, 1, sizeof(unsigned int) * p->vid.number_of_frames, p->f_vid) != sizeof(unsigned int) * p->vid.number_of_frames)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: can't read video index");
		}


	unsigned int i        = 0;
	unsigned int vid_size = 0;

	for (; i < p->vid.number_of_frames; i++)
		{
		if ((p->vid_index[i] >> 1) > p->vid.maximum_frame_size) { pmp_lib_close(p); return("pmp_lib_open: frame size can't be > maximum_frame_size"); }

		vid_size += p->vid_index[i] >> 1;
		}


	if (fseek(p->f_vid, vid_size, SEEK_CUR) != 0)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: fseek failed on vid_size");
		}




	if (fread(&p->aud, 1, sizeof(struct pmp_lib_aud_struct), p->f_vid) != sizeof(struct pmp_lib_aud_struct))
		{
		pmp_lib_close(p);
		return("pmp_lib_open: can't read audio header");
		}


	if (p->aud.frame_size       == 0) { pmp_lib_close(p); return("pmp_lib_open: audio frame_size == 0"); }
	if (p->aud.number_of_frames == 0) { pmp_lib_close(p); return("pmp_lib_open: audio number_of_frames == 0"); }


	p->aud_index = malloc_64(sizeof(unsigned char) * p->aud.number_of_frames);
	if (p->aud_index == 0)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: malloc_64 failed on aud_index");
		}


	if (fread(p->aud_index, 1, sizeof(unsigned char) * p->aud.number_of_frames, p->f_vid) != sizeof(unsigned char) * p->aud.number_of_frames)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: can't read audio index");
		}


	unsigned int aud_size = 0;

	for (i = 0; i < p->aud.number_of_frames; i++)
		{
		if (p->aud_index[i] != 0 && p->aud_index[i] != 1) { pmp_lib_close(p); return("pmp_lib_open: aud_index != 0 && aud_index != 1"); }

		aud_size += p->aud.frame_size + p->aud_index[i];
		}




	if (fseek(p->f_vid, 0, SEEK_END) != 0)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: fseek failed on SEEK_END");
		}


	int tot_size = ftell(p->f_vid);
	if (tot_size == -1)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: ftell failed");
		}


	if (tot_size != sizeof(struct pmp_lib_vid_struct) + sizeof(unsigned int) * p->vid.number_of_frames + vid_size + sizeof(struct pmp_lib_aud_struct) + sizeof(unsigned char) * p->aud.number_of_frames + aud_size)
		{
		pmp_lib_close(p);
		return("pmp_lib_open: invalid tot_size");
		}




	p->vid_start = sizeof(struct pmp_lib_vid_struct) + sizeof(unsigned int) * p->vid.number_of_frames;
	p->aud_start = p->vid_start + vid_size + sizeof(struct pmp_lib_aud_struct) + sizeof(unsigned char) * p->aud.number_of_frames;




	fclose(p->f_vid);
	p->f_vid = 0;


	return(0);
	}
