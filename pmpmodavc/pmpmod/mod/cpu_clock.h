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
cpu clock routines
*/


#ifndef cpu_clock_h
#define cpu_clock_h


#include <psppower.h>

struct speed_setting_struct {
	int cpu;
	int ram;
	int bus;
};

void cpu_clock_set_speed( struct speed_setting_struct *speed );
void cpu_clock_set_maximum();
void cpu_clock_set_minimum();


#endif
