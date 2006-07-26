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
color space conversion done with the me
*/


#ifndef me_csc_h
#define me_csc_h


#include <pspkernel.h>
#include "me.h"
#include "csc.h"


void me_csc_asm_remaining(struct csc_remaining_struct *p);

void me_csc
	(
	unsigned char *cy,
	unsigned char *cu,
	unsigned char *cv,
	int l0,
	int l1,
	int l2,
	unsigned char *out,
	int width,
	int height,
	int line_size,
	int y_offset
	);

#endif
