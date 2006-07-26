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


#include "me_csc.h"


void me_csc_asm_remaining(struct csc_remaining_struct *p)
	{
	while (p->remaining_height != 0)
		{
		csc_asm(&p->csc);


		p->csc.cy  += p->csc.l0 << 3;
		p->csc.cu  += p->csc.l1 << 2;
		p->csc.cv  += p->csc.l2 << 2;
		p->csc.out += 16384;
		
		
		p->remaining_height -= 8;
		
		
		if (me_signaled()) break;
		}


	me_sceKernelDcacheWritebackInvalidateAll();
	}


void me_csc_asm(struct csc_struct *p)
	{
	csc_asm(p);
	me_sceKernelDcacheWritebackInvalidateAll();
	}


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
	)
	{
	struct csc_struct me;
	struct csc_struct main_cpu;


	int height_div_2 = height >> 1;
	int height_div_4 = height >> 2;

	me.cy              = cy;
	me.cu              = cu;
	me.cv              = cv;
	me.l0              = l0;
	me.l1              = l1;
	me.l2              = l2;
	me.out             = out;
	me.width           = width;
	me.height          = height_div_2;
	me.line_size       = line_size;
	me.y_offset        = y_offset;

	main_cpu.cy        = cy + height_div_2 * l0;
	main_cpu.cu        = cu + height_div_4 * l1;
	main_cpu.cv        = cv + height_div_4 * l2;
	main_cpu.l0        = l0;
	main_cpu.l1        = l1;
	main_cpu.l2        = l2;
	main_cpu.out       = out + height_div_2 * (line_size << 2);
	main_cpu.width     = width;
	main_cpu.height    = height_div_2;
	main_cpu.line_size = line_size;
	main_cpu.y_offset  = y_offset;

	sceKernelDcacheWritebackInvalidateAll();


	me_start((int) &me_csc_asm, (int) &me);
//	me_start((int) &me_csc_c, (int) &me);

	csc_asm(&main_cpu);
//	csc_c(&main_cpu);
	sceKernelDcacheWritebackInvalidateAll();

	me_wait();
	}
