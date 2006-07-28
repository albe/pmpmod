/*
PMP Mod
Copyright (C) 2006 Raphael

E-mail:   raphael@fx-world.org

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
font rendering system
*/

// TODO: Clean up again
// TODO: fix temporary cache for multi-use


#include <pspkernel.h>
#include <pspiofilemgr.h>
#include <pspgu.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "valloc.h"
#include "gu_font.h"


//unsigned short __attribute__((aligned(16))) gu_font_clut[16];

struct gu_font_struct* gu_cur_font = 0;					// Current set font
static struct gu_font_manager_struct gu_font_manager;


static struct gu_glyph_cache_manager_struct gu_glyph_cache_manager;

static struct gu_glyph_cache_struct gu_glyph_temp_cache;		// temporary texture for bypassing glyph cache system


static int gu_font_initialized = 0;
//static int gu_font_border = 1;
//static int gu_font_color = 0xffffff;
//static int gu_font_border_color = 0;


#define IsSet(val,flag) (val&flag)==flag

#define FONTHEIGHT (gu_cur_font->charmaps[0].chars[(unsigned char)'I'].pixel_height)
#define CHARWIDTH(c) (gu_cur_font->charmaps[c>>8].chars[(unsigned char)c].byte_width)
#define CHARWIDTH2(c,style) (((style&gu_cur_font->haveflags)!=style||style==0)?CHARWIDTH(c):\
							 ((style&GU_FONT_BOLD)!=0?(gu_cur_font->hasitalic!=0?\
							   gu_cur_font->charmaps[c>>8].chars[(style<<8)+(unsigned char)c].byte_width:\
							   gu_cur_font->charmaps[c>>8].chars[256+(unsigned char)c].byte_width):\
							   gu_cur_font->charmaps[c>>8].chars[(style<<8)+(unsigned char)c].byte_width))
#define CHARHEIGHT(c) (gu_cur_font->charmaps[c>>8].chars[(unsigned char)c].byte_height)


#ifdef DEBUG
void gu_debug_print_glyph_cache()
	{
	FILE* f = fopen("debug.txt","a+");
	if (!f) return;
	
	fprintf(f, "\nDEBUGPRINT GLYPH CACHE:\n");
	fprintf(f, "--------------------------------------------\n\n");
	fprintf(f, "vmemavail     : %x\n", vmemavail());
	fprintf(f, "vlargestblock : %x\n", vlargestblock());
	
	struct gu_glyph_cache_list_struct* list = gu_glyph_cache_manager.root;
	int i = 1;
	while (list!=0)
		{
			{
			fprintf(f, "----------------\n");
			fprintf(f, "GLYPH CACHE NO. %i\n", i);
			fprintf(f, "list->\n");
			fprintf(f, "----------------\n");
			fprintf(f, "lru_index      : %i\n", list->lru_index);
			fprintf(f, "cache->cacheptr: 0x%x\n", list->cache->cacheptr);
			fprintf(f, "cache->size    : 0x%x\n", list->cache->size);
			fprintf(f, "cache->width   : %i\n", list->cache->width);
			fprintf(f, "cache->height  : %i\n", list->cache->height);
			fprintf(f, "cache->flags   : %i\n", list->cache->flags);
			fprintf(f, "cache->font_id : %i\n", list->cache->font_id);
			fprintf(f, "cache->dirty   : %i\n", list->cache->dirty);
			fprintf(f, "cache->string  : %s\n", list->cache->string);
			fprintf(f, "cache->length  : %i\n", list->cache->length);
			fprintf(f, "----------------\n\n");
			}
		list = list->next;
		i++;
		}
	fprintf(f,"\n\n--------------------------------------------\nFINISHED\n--------------------------------------------\n\n");
	fclose(f);
	}
	

void gu_debug_print_charset()
{
	if (gu_cur_font == 0) return;
	FILE* f = fopen("debug.txt","a+");
	if (!f) return;
	
	fprintf(f, "\nDEBUGPRINT CHARSET:\n");
	fprintf(f, "--------------------------------------------\n\n");
	
	fprintf(f, "----------------\n");
	fprintf(f, "FONT:\n");
	fprintf(f, "----------------\n");
	fprintf(f, "d_width    : %i\n", gu_cur_font->d_width);
	fprintf(f, "d_height   : %i\n", gu_cur_font->d_height);
	fprintf(f, "haveflags  : %i\n", gu_cur_font->haveflags);
	fprintf(f, "\n");
	int i;
	char letter[2];
	letter[1] = '\0';
	for (i=0;i<256;i++)
		{
			{
			letter[0] = i;
			fprintf(f, "----------------\n");
			fprintf(f, "CHARACTER: %s(%i)\n", letter, i);
			fprintf(f, "----------------\n");
			fprintf(f, "byte_width     : %i\n", gu_cur_font->charmaps[0].chars[i].byte_width);
			fprintf(f, "byte_height    : %i\n", gu_cur_font->charmaps[0].chars[i].byte_height);
			fprintf(f, "x_offset       : %i\n", gu_cur_font->charmaps[0].chars[i].x_offset);
			fprintf(f, "y_offset       : %i\n", gu_cur_font->charmaps[0].chars[i].y_offset);
			fprintf(f, "pixel_width    : %i\n", gu_cur_font->charmaps[0].chars[i].pixel_width);
			fprintf(f, "pixel_height   : %i\n", gu_cur_font->charmaps[0].chars[i].pixel_height);
			if (gu_cur_font->hasitalic==1&&gu_cur_font->hasbold==1)
			{
			fprintf(f, "ITALIC:\n" );
			fprintf(f, "----------------\n");
			fprintf(f, "byte_width     : %i\n", gu_cur_font->charmaps[0].chars[256+i].byte_width);
			fprintf(f, "byte_height    : %i\n", gu_cur_font->charmaps[0].chars[256+i].byte_height);
			fprintf(f, "x_offset       : %i\n", gu_cur_font->charmaps[0].chars[256+i].x_offset);
			fprintf(f, "y_offset       : %i\n", gu_cur_font->charmaps[0].chars[256+i].y_offset);
			fprintf(f, "pixel_width    : %i\n", gu_cur_font->charmaps[0].chars[256+i].pixel_width);
			fprintf(f, "pixel_height   : %i\n", gu_cur_font->charmaps[0].chars[256+i].pixel_height);
			fprintf(f, "BOLD:\n" );
			fprintf(f, "----------------\n");
			fprintf(f, "byte_width     : %i\n", gu_cur_font->charmaps[0].chars[512+i].byte_width);
			fprintf(f, "byte_height    : %i\n", gu_cur_font->charmaps[0].chars[512+i].byte_height);
			fprintf(f, "x_offset       : %i\n", gu_cur_font->charmaps[0].chars[512+i].x_offset);
			fprintf(f, "y_offset       : %i\n", gu_cur_font->charmaps[0].chars[512+i].y_offset);
			fprintf(f, "pixel_width    : %i\n", gu_cur_font->charmaps[0].chars[512+i].pixel_width);
			fprintf(f, "pixel_height   : %i\n", gu_cur_font->charmaps[0].chars[512+i].pixel_height);
			fprintf(f, "BOLDITALIC:\n" );
			fprintf(f, "----------------\n");
			fprintf(f, "byte_width     : %i\n", gu_cur_font->charmaps[0].chars[256+512+i].byte_width);
			fprintf(f, "byte_height    : %i\n", gu_cur_font->charmaps[0].chars[256+512+i].byte_height);
			fprintf(f, "x_offset       : %i\n", gu_cur_font->charmaps[0].chars[256+512+i].x_offset);
			fprintf(f, "y_offset       : %i\n", gu_cur_font->charmaps[0].chars[256+512+i].y_offset);
			fprintf(f, "pixel_width    : %i\n", gu_cur_font->charmaps[0].chars[256+512+i].pixel_width);
			fprintf(f, "pixel_height   : %i\n", gu_cur_font->charmaps[0].chars[256+512+i].pixel_height);
			}
			
			}
		}
	fprintf(f,"\n\n--------------------------------------------\nFINISHED\n--------------------------------------------\n\n");
	fclose(f);
}
#endif

void gu_glyph_cache_safe_constructor(struct gu_glyph_cache_struct *p)
	{
	if (p==0) return;
	p->cacheptr = 0;
	p->size = 0;
	p->width = 0;
	p->height = 0;
	p->flags = 0;
	p->string[0] = '\0';
	p->length = 0;
	p->font_id = 0;
	p->dirty = 0;
	}


void gu_glyph_cache_safe_destructor(struct gu_glyph_cache_struct *p)
	{
	if (p==0) return;
	if (p->cacheptr!=0)
		vfree( p->cacheptr );
	p->cacheptr = 0;
	p->size = 0;
	p->width = 0;
	p->height = 0;
	p->flags = 0;
	p->string[0] = '\0';
	p->length = 0;
	p->font_id = 0;
	p->dirty = 0;
	free(p);
	}

	
void gu_glyph_cache_list_safe_constructor(struct gu_glyph_cache_list_struct *p)
	{
	if (p==0) return;
	p->cache = 0;
	p->next = 0;
	p->prev = 0;
	p->lru_index = 0;
	}


void gu_glyph_cache_list_safe_destructor(struct gu_glyph_cache_list_struct *p)
	{
	if (p==0) return;
	gu_glyph_cache_safe_destructor(p->cache);
	p->cache = 0;
	p->next = 0;
	p->prev = 0;
	p->lru_index = 0;
	free(p);
	}


struct gu_glyph_cache_struct* gu_glyph_cache_create( unsigned int size )
	{
	struct gu_glyph_cache_struct* p = malloc(sizeof(struct gu_glyph_cache_struct));
	if (p==0) return(0);
	
	gu_glyph_cache_safe_constructor(p);
	
	p->cacheptr = valloc( size );
	if (p->cacheptr==0)
		{
		free(p);
		return(0);
		}
	p->flags = GLYPH_CACHE_VRAM;
	p->size = size;
	p->dirty = 1;
	
	return(p);
	}


struct gu_glyph_cache_list_struct* gu_glyph_cache_list_create( unsigned int size )
	{
	struct gu_glyph_cache_list_struct *p = malloc(sizeof(struct gu_glyph_cache_list_struct));
	if (p==0) return(0);
	
	gu_glyph_cache_list_safe_constructor(p);
	
	p->cache = gu_glyph_cache_create( size );
	if (p->cache==0)
		{
		free(p);
		return(0);
		}
	
	return(p);
	}


char* gu_glyph_cache_init()
	{
	if (gu_font_initialized==1) return(0);
	
	gu_glyph_cache_manager.max_num_caches = MAX_GLYPH_CACHES;
	gu_glyph_cache_manager.lru_counter = 0;
	gu_glyph_cache_manager.num_caches = 0;
	gu_glyph_cache_manager.root = 0;
	gu_glyph_cache_manager.tail = 0;

	// Try to allocate temporary cache in VRAM first
	gu_glyph_cache_safe_constructor(&gu_glyph_temp_cache);
	//gu_glyph_temp_cache.cacheptr = valloc( 256*MAX_CACHE_LINES*16 );
	//gu_glyph_temp_cache.flags = GLYPH_CACHE_VRAM;
	if (gu_glyph_temp_cache.cacheptr==0)
		{
		gu_glyph_temp_cache.cacheptr = memalign(16, 256*MAX_CACHE_LINES*16 );
		gu_glyph_temp_cache.size = 256*MAX_CACHE_LINES*16;
		if (gu_glyph_temp_cache.cacheptr==0)
			return("gu_glyph_cache_init: memalign failed on gu_glyph_temp_cache");
		memset(gu_glyph_temp_cache.cacheptr,0,gu_glyph_temp_cache.size);
		gu_glyph_temp_cache.flags = GLYPH_CACHE_SYSMEM;
		}
	gu_glyph_temp_cache.dirty = 1;		// temporary cache is always dirty
		
	return(0);
	}


void gu_glyph_cache_free()
	{
	/*if (IsSet(gu_glyph_temp_cache.flags,GLYPH_CACHE_VRAM))
		vfree( gu_glyph_temp_cache.cacheptr );
	else if (IsSet(gu_glyph_temp_cache.flags,GLYPH_CACHE_SYSMEM))*/
		free( gu_glyph_temp_cache.cacheptr );

	gu_glyph_temp_cache.flags = 0;
	struct gu_glyph_cache_list_struct *list = gu_glyph_cache_manager.root;
	struct gu_glyph_cache_list_struct *next;
	while (list!=0)
		{
		next = list->next;
		gu_glyph_cache_list_safe_destructor( list );
		list = next;
		}
	gu_glyph_cache_manager.root = 0;
	gu_glyph_cache_manager.tail = 0;
	gu_glyph_cache_manager.num_caches = 0;
	gu_glyph_cache_manager.lru_counter = 0;
	}


static inline void gu_glyph_cache_list_update( struct gu_glyph_cache_list_struct *p, char* s, int len, int size, int width, int height, int flags, int font_id )
	{
	p->cache->size = size;
	p->cache->width = width;
	p->cache->height = height;
	p->cache->font_id = font_id;
	p->cache->flags = (flags&FLAG_ALIGN_MASK);
	p->cache->dirty = 1;
	p->cache->length = len;
	strncpy((char*)p->cache->string,s,MAX_CACHE_STRING-1);
	if (p->cache->cacheptr!=0)
		{
		memset(p->cache->cacheptr,0,size);
		sceKernelDcacheWritebackAll();
		}
	}


static void gu_glyph_cache_list_remove( struct gu_glyph_cache_list_struct* p )
	{
	if (p==0) return;
	if (p->prev!=0)
		p->prev->next = p->next;
	else
		gu_glyph_cache_manager.root = p->next;
	if (p->next!=0)
		p->next->prev = p->prev;
	else
		gu_glyph_cache_manager.tail = p->prev;
	gu_glyph_cache_safe_destructor(p->cache);
	p->cache = 0;
	p->next = 0;
	p->prev = 0;
	p->lru_index = 0;
	free(p);
	gu_glyph_cache_manager.num_caches--;
	}


static void gu_glyph_cache_list_add( struct gu_glyph_cache_list_struct *p )
	{
	if (p==0) return;
	if (gu_glyph_cache_manager.root==0)
		{
		gu_glyph_cache_manager.root = p;
		p->prev = 0;
		}
	if (gu_glyph_cache_manager.tail!=0)
		{
		gu_glyph_cache_manager.tail->next = p;
		p->prev = gu_glyph_cache_manager.tail;
		}
	p->next = 0;
	gu_glyph_cache_manager.tail = p;
	gu_glyph_cache_manager.num_caches++;
	p->lru_index = gu_glyph_cache_manager.lru_counter++;
	}


static void gu_glyph_cache_list_reorder( struct gu_glyph_cache_list_struct *p )
	{
	if (p==0) return;
	if (p->next==0) return;
	if (p->prev==0)
		{
		p->next->prev = 0;
		gu_glyph_cache_manager.root = p->next;
		}
	else
		{
		p->next->prev = p->prev;
		p->prev->next = p->next;
		}
	p->next = 0;
	p->prev = gu_glyph_cache_manager.tail;
	gu_glyph_cache_manager.tail->next = p;
	gu_glyph_cache_manager.tail = p;
	p->lru_index = gu_glyph_cache_manager.lru_counter++;
	}


// FIXME: Using multiple temp caches per frame currently causes drawing errors
// TODO: allocate a new temp cache for every request, add a global temp cache deallocator
static struct gu_glyph_cache_struct* gu_glyph_get_temp_cache( unsigned int size )
	{
	if (gu_glyph_temp_cache.size<size || gu_glyph_temp_cache.cacheptr==0)
		{
		if (gu_glyph_temp_cache.cacheptr!=0)
			free( gu_glyph_temp_cache.cacheptr );

		gu_glyph_temp_cache.cacheptr = memalign( 16, size );
		if (gu_glyph_temp_cache.cacheptr==0) return(0);
		}

		memset(gu_glyph_temp_cache.cacheptr, 0, size);
		sceKernelDcacheWritebackAll();
		return(&gu_glyph_temp_cache);
	}


static int next_pow2(unsigned int w)
	{
	if (w==0) return 0;
	int n = 1;
	while (w>n)
		{
		n <<= 1;
		}
	
	return (n);
	}
	

// return a pointer to a glyph cache useable by the GU
// this is the real cache engine, based on a LRU linked list
struct gu_glyph_cache_struct* gu_glyph_cache_manager_get( char* s, int width, int height, int flags, int font_id )
	{
	int cache_size = next_pow2(width)*height >> 1;
	if (IsSet(flags,FLAG_NOCACHE))
		{
		return(gu_glyph_get_temp_cache( cache_size ));
		}
	
	int len = strlen(s);
	// Check if text output already in cache
	// Check backwards, since MRU is at the end of the list -> faster cache hit probabilty
	struct gu_glyph_cache_list_struct* list = gu_glyph_cache_manager.tail;
	//if (list == 0) return(0);
	while (list!=0)
		{
		if (list->cache->length==len &&
			list->cache->flags==(flags&FLAG_ALIGN_MASK) &&
			list->cache->font_id==font_id &&
			strcmp(list->cache->string,s)==0)
			{
			// cache-hit
			list->cache->width = width;
			list->cache->height = height;
			list->cache->dirty = 0;			// cache doesn't need to be regenerated
			gu_glyph_cache_list_reorder( list );
			
			return(list->cache);
			}
		list = list->prev;
		}
	
	// cache-miss
	// try to allocate a new cache, if not already at max
	if ((gu_glyph_cache_manager.num_caches<gu_glyph_cache_manager.max_num_caches) && (vlargestblock()>=cache_size))
		{
		struct gu_glyph_cache_list_struct* new_cache;
		new_cache = gu_glyph_cache_list_create( cache_size );
		if (new_cache!=0)
			{
			gu_glyph_cache_list_add( new_cache );
			gu_glyph_cache_list_update( new_cache, s, len, cache_size, width, height, flags, font_id );
			return(new_cache->cache);
			}
		}
	
	list = gu_glyph_cache_manager.root;
	if (list == 0) return(gu_glyph_get_temp_cache( cache_size ));	// We have no allocated caches
	struct gu_glyph_cache_list_struct* lru = 0;

	// We couldn't allocate a new cache, so find LRU cache fitting needed size
	while (list!=0)
		{
		if (list->cache->size>=cache_size)
			{
			lru = list;
			break;
			}
		list = list->next;
		}
	
	// Now we either have a matching cache, or we couldn't find one big enough
	if (lru==0)
		{
		do
			{
			// No single cache in list was big enough... so try to put some together
			lru = gu_glyph_cache_manager.root;
			// free LRU cache and check if we have enough free VRAM
			// in worst case we simply fragment the VRAM with this
			gu_glyph_cache_list_remove(lru);
			} while (gu_glyph_cache_manager.root!=0 && vlargestblock()<cache_size);
		
		struct gu_glyph_cache_list_struct* new_cache = 0;
		if (vlargestblock()>=cache_size)
			new_cache = gu_glyph_cache_list_create( cache_size );
		if (new_cache==0) return(gu_glyph_get_temp_cache( cache_size ));

		gu_glyph_cache_list_add( new_cache );
		gu_glyph_cache_list_update( new_cache, s, len, cache_size, width, height, flags, font_id );
		return(new_cache->cache);
		}
	
	// We found a big enough cache in the list
	if (lru->cache==0) return(gu_glyph_get_temp_cache( cache_size ));
	// vrealloc is pretty cheap when only decreasing size
	if (lru->cache->size>cache_size) vrealloc( lru->cache->cacheptr, cache_size );
	gu_glyph_cache_list_update( lru, s, len, cache_size, width, height, flags, font_id );
	gu_glyph_cache_list_reorder( lru );

	return(lru->cache);
	}



void gu_glyph_cache_manager_limit_glyph_caches( unsigned int n )
	{
	if (n>MAX_GLYPH_CACHES) n=MAX_GLYPH_CACHES;
	while (gu_glyph_cache_manager.num_caches>n)
		{
		gu_glyph_cache_list_remove(gu_glyph_cache_manager.root);
		}
	gu_glyph_cache_manager.max_num_caches = n;
	}




/* sw always 16 so there's always 64bit to copy for each line
		s1			s2
  s = |xxxxxxxx|yyyyyyyy|				sx = 0
  s1 << (3*4) = |000xxxxx|
  s1 >> (5*4) = |xxx00000|
  s2 << (3*4) = |000yyyyy|
  s2 >> (5*4) = |yyy00000|
  d = |00000000|00000000|00000000|		dx = 3
  => d' = |000xxxxx|xxxyyyyy|yyy00000|
  
  NOTE:	ORing the bits gives basic colorkeying, allthough not 100% correct
		because overdrawing non-zero pixels with non-zero pixels will
		create lighter pixels (some kind of additive blending).
		This is barely noticeable if only small parts overlap, what should
		most likely be the case.
*/
static inline void gu_font_copy_glyph( int sx, int sy, int sh, char *s, int sstride, int dx, int dy, char *d, int dstride )
{
	int i;
	if ((dx&0x7)==0) {
		unsigned int* u32s = (unsigned int*)((unsigned int)s+((sx >> 1)+(sy*sstride)));
		unsigned int* u32d = (unsigned int*)((unsigned int)d+((dx >> 1)+(dy*dstride)));

		// can do fast copy
		for (i=0;i<sh;i++) {
		  // 32bit copy part
		  *u32d++ |= *u32s++;
		  *u32d++ |= *u32s++;
		  u32s += (sstride-8)>>2;
		  u32d += (dstride-8)>>2;
		}
	} else {

	unsigned int* u32s = (unsigned int*)((unsigned int)s+((sx >> 1)+(sy*sstride)));
	unsigned int* u32d = (unsigned int*)((unsigned int)d+(((dx>>3)<<2)+(dy*dstride)));

	unsigned int mask = 0;
	unsigned int shift = (dx&0x7)<<2;
	for (i=0;i<(dx&0x7);i++) {
		mask <<= 4;	
		mask |= 0xf;
	}

	for (i=0;i<sh;i++) {
	  unsigned int s1 = *u32s++;
	  unsigned int s2 = *u32s++;
	  
	  // copy first halfbytes
	  *u32d++ |= ((*u32d) & mask)|(s1 << shift);	// dst = |000xxxxx|
	  
	  *u32d++ |= (s1 >> (32-shift)) | (s2 << shift);	// dst = |000xxxxx|xxxyyyyy|
	  
	  // copy last halfbytes
	  *u32d++ |= ((*u32d) & ~mask)|(s2 >> (32-shift));		// dst = |000xxxxx|xxxyyyyy|yyy00000|
	  
	  u32s += (sstride-8)>>2;
	  u32d += (dstride-12)>>2;
	}

	}
}


static inline void gu_font_cache_glyph( int x, int y, unsigned short c, int flags, char* cache, int stride )
	{
	int sx = (int)((unsigned char)c % 16)*16;
	int sy = (int)((unsigned char)c / 16)*16;
	int offs = 0;
	// Check if specified styles are available for the current font
	if ((flags&gu_cur_font->haveflags)==flags)
		{
		if ((flags&GU_FONT_BOLD)!=0)
			{
			if (gu_cur_font->hasitalic!=0)
				{
				sy += 256;
				offs = 512;
				}
			else
				{
				sx += 256;
				offs = 256;
				}
			}
		if ((flags&GU_FONT_ITALIC)!=0)
			{
			sx += 256;
			offs += 256;
			}
		}
	
	int sw = gu_cur_font->charmaps[c>>8].chars[offs+(unsigned char)c].byte_width;
	int sh = gu_cur_font->charmaps[c>>8].chars[offs+(unsigned char)c].byte_height;
	int dx = x+gu_cur_font->charmaps[c>>8].chars[offs+(unsigned char)c].x_offset;
	int dy = y+gu_cur_font->charmaps[c>>8].chars[offs+(unsigned char)c].y_offset;
	// Clip
	if (dy<0) { sy-=dy; sh+=dy; dy=0; }
	if (dx<0) { sx-=dx; sw+=dx; dx=0; }
	if (dy+sh>=272) { sh=271-dy; }
	if (dx+sw>=480) { sw=479-dx; }

	if (sh==0 || sw==0) return;
	
	gu_font_copy_glyph( sx, sy, sh, (char*)gu_cur_font->charmaps[c>>8].data, gu_cur_font->d_width, dx, dy, (char*)((unsigned int)cache | 0x40000000), stride );
	}


static inline void blit_fast( int x, int y, int w, int h, int x2, int y2 )
{
		int start;
		for (start = 0; start < w; start += 64, x2 += 64)
		{
			struct VertexInt* vertices = (struct VertexInt*)sceGuGetMemory(2 * sizeof(struct VertexInt));
			int width = (start + 64) < w ? 64 : w-start;

			vertices[0].u = x + start; vertices[0].v = y;
			vertices[0].color = 0x3000;
			vertices[0].x = x2; vertices[0].y = y2; vertices[0].z = 0;

			vertices[1].u = x + start + width; vertices[1].v = y + h;
			vertices[1].color = 0x3000;
			vertices[1].x = x2 + width; vertices[1].y = y2 + h; vertices[1].z = 0;

			sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_COLOR_4444|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
		}
}




// NOTE: the following function only handles triple byte utf8 encodings maximum

static unsigned short get_next_utf8( char** utf8 )
	{
	if ((*utf8)==0) return 0;
	
	// skip follow-bytes at start of stream
	while ((*(*utf8)&0xC0)==0x80)
		(*utf8)++;

	unsigned char u1 = *(*utf8);
	if ((u1&0x80)==0)
		return ((unsigned short)u1);		// ASCII
	else
	if ((u1&0xE0)==0xC0)
		{
		(*utf8)++;
		unsigned char u2 = *(*utf8);
		// 
		return (((unsigned short)(u1&0x1F) << 6) | (unsigned short)(u2&0x3F));
		}
	else
	if ((u1&0xF0)==0xE0)
		{
		(*utf8)++;
		unsigned char u2 = *(*utf8)++;
		unsigned char u3 = *(*utf8);
		return (((unsigned short)(u1&0xF) << 12) | ((unsigned short)(u2&0x3F)<<6) | (unsigned short)(u3&0x3F));
		}
	else
		return (unsigned short)u1;
	}


static unsigned short get_next_utf16le( short** utf16 )
	{
	if ((*utf16)==0) return 0;

	return (unsigned short)(*(*utf16));
	}


static unsigned short get_next_utf16be( short** utf16 )
	{
	if ((*utf16)==0) return 0;
	
	unsigned short u1 =*(*utf16);
	
	return (unsigned short)(((u1&0xFF)<<8)|(u1>>8));
	}


static short* utf16be2le( short* utf16 )
	{
	if (utf16==0) return 0;
	
	#define SWAPBYTES(c) (((c&0xFF)<<8)|(c>>8))
	
	short* utf16le = utf16;
	
	while (*utf16le!=0)
		{
		*utf16le++ = SWAPBYTES(*utf16le);
		}
	return(utf16);
	}


#define HASH(x) (((unsigned int)((x&0xFF)+(unsigned int)(x>>8)*UNICODE_HASH_MULT))%UNICODE_HASH_SIZE)

unsigned short gu_font_get_unicodechar( unsigned short unicode )
	{
	if (unicode==0) return(0);
	if (gu_cur_font==0) return((unsigned short)GU_FONT_REPLACEMENT_CHAR);
	if ((gu_cur_font->haveflags&GU_FONT_HAS_UNICODE_CHARMAP)==0 || gu_cur_font->unicode_hash==0) return(GU_FONT_REPLACEMENT_CHAR);
	
	unsigned int hash = HASH(unicode);
	struct gu_font_unicode_list_struct *ucs = gu_cur_font->unicode_hash[hash];
	struct gu_font_unicode_list_struct *prev = 0;
	
	while (ucs!=0)
		{
		if (ucs->unicode==unicode)
			{
			if (prev!=0)
				{
				// dynamically reorder hashlists on MRU base
				prev->next = ucs->next;
				ucs->next = gu_cur_font->unicode_hash[hash];
				gu_cur_font->unicode_hash[hash] = ucs;
				}
			return(ucs->charcode);
			}
		prev = ucs;
		ucs = ucs->next;
		}
	return((unsigned short)GU_FONT_REPLACEMENT_CHAR);
	}


static int gu_font_parse_bbcode( char** c,  int* style )
	{
	if (c==0 || *c==0) return(0);
	if ((*c)[0]=='[')
		{
		// parse bbcode formatting
		if ((*c)[1]=='/' && (*c)[3]==']')
			{
			if ((*c)[2]=='i' || (*c)[2]=='I')
				{
				*style &= ~GU_FONT_ITALIC;
				(*c)+=3;
				}
			else
			if ((*c)[2]=='b' || (*c)[2]=='B')
				{
				*style &= ~GU_FONT_BOLD;
				(*c)+=3;
				}
			else
				return(0);
			}
		else if ((*c)[2]==']')
			{
			if ((*c)[1]=='i' || (*c)[1]=='I')
				{
				*style |= GU_FONT_ITALIC;
				(*c)+=2;
				}
			else
			if ((*c)[1]=='b' || (*c)[1]=='B')
				{
				*style |= GU_FONT_BOLD;
				(*c)+=2;
				}
			else
				return(0);
			}
		else
			return(0);
		}
	else
		return(0);
	
	return(1);
	}
	

static int gu_font_utf16le_parse_bbcode( short** c,  int* style )
	{
	if (c==0 || *c==0) return(0);
	if ((*c)[0]=='[')
		{
		// parse bbcode formatting
		if ((*c)[1]=='/' && (*c)[3]==']')
			{
			if ((*c)[2]=='i' || (*c)[2]=='I')
				{
				*style &= ~GU_FONT_ITALIC;
				(*c)+=3;
				}
			else
			if ((*c)[2]=='b' || (*c)[2]=='B')
				{
				*style &= ~GU_FONT_BOLD;
				(*c)+=3;
				}
			else
				return(0);
			}
		else if ((*c)[2]==']')
			{
			if ((*c)[1]=='i' || (*c)[1]=='I')
				{
				*style |= GU_FONT_ITALIC;
				(*c)+=2;
				}
			else
			if ((*c)[1]=='b' || (*c)[1]=='B')
				{
				*style |= GU_FONT_BOLD;
				(*c)+=2;
				}
			else
				return(0);
			}
		else
			return(0);
		}
	else
		return(0);
	
	return(1);
	}


static int gu_font_utf16be_parse_bbcode( short** c,  int* style )
	{
	if (c==0 || *c==0) return(0);
	#define SWAPBYTES(c) (((c&0xFF)<<8)|(c>>8))
	
	if (SWAPBYTES((*c)[0])=='[')
		{
		// parse bbcode formatting
		if (SWAPBYTES((*c)[1])=='/' && SWAPBYTES((*c)[3])==']')
			{
			if (SWAPBYTES((*c)[2])=='i' || SWAPBYTES((*c)[2])=='I')
				{
				*style &= ~GU_FONT_ITALIC;
				(*c)+=3;
				}
			else
			if (SWAPBYTES((*c)[2])=='b' || SWAPBYTES((*c)[2])=='B')
				{
				*style &= ~GU_FONT_BOLD;
				(*c)+=3;
				}
			else
				return(0);
			}
		else if (SWAPBYTES((*c)[2])==']')
			{
			if (SWAPBYTES((*c)[1])=='i' || SWAPBYTES((*c)[1])=='I')
				{
				*style |= GU_FONT_ITALIC;
				(*c)+=2;
				}
			else
			if (SWAPBYTES((*c)[1])=='b' || SWAPBYTES((*c)[1])=='B')
				{
				*style |= GU_FONT_BOLD;
				(*c)+=2;
				}
			else
				return(0);
			}
		else
			return(0);
		}
	else
		return(0);
	
	return(1);
	}


void gu_font_cache_string( char* s, int maxwidth, int flags, char* cache, int stride )
	{
	char* c = s;
	int i = 0;
	int x = 0;
	int y = 0;
	int style = 0;
	
	int (*gu_font_line_width)( char* );
	if (IsSet(flags,FLAG_UTF8))
		gu_font_line_width = gu_font_utf8_line_width_get;
	else
		gu_font_line_width = gu_font_line_width_get;


	if (IsSet(flags,FLAG_ALIGN_CENTER))
		{
		x = ((maxwidth-gu_font_line_width( s ))/2);
		}
	else if (IsSet(flags,FLAG_ALIGN_RIGHT))
		{
		x = (maxwidth-gu_font_line_width( s ));
		}

	while (*c!='\0' && i<MAX_CACHE_STRING) {
			if (*c==' ')
				{
				x += CHARWIDTH('t');
				i++;
				}
			else if (*c=='\n')
				{
				i++;
				x = 0;
				if (IsSet(flags,FLAG_ALIGN_CENTER))
					{
					x = ((maxwidth-gu_font_line_width( &c[1] ))/2);
					}
				else if (IsSet(flags,FLAG_ALIGN_RIGHT))
					{
					x = (maxwidth-gu_font_line_width( &c[1] ));
					}
				y += FONTHEIGHT;
				}
			else if ((unsigned char)*c>=32)
				{
				if (gu_font_parse_bbcode( &c, &style )==0)
					if (IsSet(flags,FLAG_UTF8))
						{
						unsigned short ucs = gu_font_get_unicodechar(get_next_utf8(&c));
						gu_font_cache_glyph( x, y, ucs, style, cache, stride );
						x += CHARWIDTH2(ucs,style);
						i++;
						}
					else
						{
						gu_font_cache_glyph( x, y, ((unsigned char)*c), style, cache, stride );
						x += CHARWIDTH2(((unsigned char)*c),style);
						i++;
						}
				}
			c++;
			
			if (IsSet(flags,FLAG_CLIP_WRAP))
				if (x>=472)
					{
					x = 0;
					y += FONTHEIGHT;
					}
		}
	sceKernelDcacheWritebackInvalidateAll();
	}




void gu_font_utf16_cache_string( short* s, int maxwidth, int flags, char* cache, int stride )
	{
	short* c = s;
	int i = 0;
	int x = 0;
	int y = 0;
	int style = 0;

	int (*gu_font_line_width)( short* );
	int (*gu_font_utf16_parse_bbcode)( short**, int* );
	unsigned short (*get_next_utf16)( short** );
	if (IsSet(flags,FLAG_UTF16BE))
		{
		gu_font_line_width = gu_font_utf16be_line_width_get;
		gu_font_utf16_parse_bbcode = gu_font_utf16be_parse_bbcode;
		get_next_utf16 = get_next_utf16be;
		}
	else
		{
		gu_font_line_width = gu_font_utf16le_line_width_get;
		gu_font_utf16_parse_bbcode = gu_font_utf16le_parse_bbcode;
		get_next_utf16 = get_next_utf16le;
		}


	if (IsSet(flags,FLAG_ALIGN_CENTER))
		{
		x = ((maxwidth-gu_font_line_width( s ))/2);
		}
	else if (IsSet(flags,FLAG_ALIGN_RIGHT))
		{
		x = (maxwidth-gu_font_line_width( s ));
		}

	while (*c!='\0' && i<MAX_CACHE_STRING) {
			if (*c==' ')
				{
				x += CHARWIDTH('t');
				i++;
				}
			else if (*c=='\n')
				{
				i++;
				x = 0;
				if (IsSet(flags,FLAG_ALIGN_CENTER))
					{
					x = ((maxwidth-gu_font_line_width( &c[1] ))/2);
					}
				else if (IsSet(flags,FLAG_ALIGN_RIGHT))
					{
					x = (maxwidth-gu_font_line_width( &c[1] ));
					}
				y += FONTHEIGHT;
				}
			else if ((unsigned short)*c>=32)
				{
				if (gu_font_utf16_parse_bbcode( &c, &style )==0)
					{
					unsigned short ucs = gu_font_get_unicodechar(get_next_utf16(&c));
					gu_font_cache_glyph( x, y, ucs, style, cache, stride );
					x += CHARWIDTH2(ucs,style);
					i++;
					}
				}
			c++;
			
			if (IsSet(flags,FLAG_CLIP_WRAP))
				if (x>=472)
					{
					x = 0;
					y += FONTHEIGHT;
					}
		}
	sceKernelDcacheWritebackInvalidateAll();
	}



void gu_font_charmap_list_safe_destructor( struct gu_font_unicode_list_struct* p )
	{
	if (p==0) return;
	if (p->next!=0) gu_font_charmap_list_safe_destructor( p->next );
	free(p);
	}


void gu_font_unicode_hash_safe_constructor( struct gu_font_struct* f )
	{
	if (f==0 || f->unicode_hash==0) return;
	memset(f->unicode_hash,0,UNICODE_HASH_SIZE*sizeof( struct gu_font_unicode_list_struct* ));
	}
	
	
void gu_font_unicode_hash_safe_destructor( struct gu_font_struct* f )
	{
	if (f==0 || f->unicode_hash==0) return;
	int l;
	for (l=0;l<UNICODE_HASH_SIZE;l++)
		gu_font_charmap_list_safe_destructor( f->unicode_hash[l] );
	free(f->unicode_hash);
	}


void gu_font_safe_constructor(struct gu_font_struct *f)
	{
	if (f==0) return;
	f->name[0] = '\0';
	f->id = -1;
	f->charmaps = 0;
	f->d_width = 0;
	f->d_height = 0;
	f->haveflags = 0;
	f->hasitalic = 0;
	f->hasbold = 0;
	f->unicode_hash = 0;
	}
	

void gu_font_safe_destructor(struct gu_font_struct *f)
	{
	if (f==0) return;
	if (f->charmaps!=0)
		{
		int i;
		for (i=0;i<f->n_submaps;i++)
			{
			if (f->charmaps[i].data!=0) free(f->charmaps[i].data);
			f->charmaps[i].data = 0;
			if (f->charmaps[i].chars!=0) free(f->charmaps[i].chars);
			f->charmaps[i].chars = 0;
			}
		free(f->charmaps);
		f->charmaps = 0;
		}
	if (f->unicode_hash!=0)
		gu_font_unicode_hash_safe_destructor( f );
	free(f);
	}
	


void gu_font_list_safe_constructor(struct gu_font_list_struct *f)
	{
	if (f==0) return;
	f->font = 0;
	f->next = 0;
	}
	

void gu_font_list_safe_destructor(struct gu_font_list_struct *f)
	{
	if (f==0) return;
	gu_font_safe_destructor(f->font);
	f->font = 0;
	f->next = 0;
	free(f);
	}


char* gu_font_init()
	{
	if (gu_font_initialized==1) return(0);
	char* result = gu_glyph_cache_init();
	if (result!=0)
		return(result);
	
	// Create a white color CLUT usable by GU 
	// We now have per-font CLUT
	/*gu_font_color_set( 0xFFFFFF );
	gu_font_border_color_set( 0x000000 );	// Black border
	gu_font_initialized = 1;*/
	
	return(0);
	}
	

void gu_font_close()
	{
	if (gu_font_initialized==0) return;
	gu_glyph_cache_free();
	gu_font_free_all();
	gu_font_initialized=0;
	}
	

struct gu_font_struct* gu_font_find( char* name )
	{
	struct gu_font_list_struct *f = gu_font_manager.list;

	while (f!=0)
		{
		if (f->font!=0)
			{
			if (strcmp( f->font->name, name)==0)
				return(f->font);
			}
		f = f->next;
		}
	
	return(0);
	}


struct gu_font_list_struct* gu_font_list_remove( char* name )
	{
	struct gu_font_list_struct *f = gu_font_manager.list;
	if (f==0) return(0);
	
	if (f->font!=0)
		if (strcmp( f->font->name, name)==0)
			{
			gu_font_manager.list = f->next;
			gu_font_manager.num_fonts--;
			return(f);
			}

	while (f->next!=0)
		{
		if (f->next->font!=0)
			{
			if (strcmp( f->next->font->name, name)==0)
				{
				f->next = f->next->next;
				gu_font_manager.num_fonts--;
				return(f->next);
				}
			}
		f = f->next;
		}
	
	return(0);
	}


char* gu_font_load( char* name )
	{
	struct gu_font_struct* f = gu_font_find( name );
	if (f!=0)
		{
		gu_cur_font = f;
		return(0);
		}
	
		
	SceUID	fd;
	if ((fd = sceIoOpen( name, PSP_O_RDONLY, 0777))<0)
		return("gu_font_load: Could not open file");
	
	f = malloc(sizeof(struct gu_font_struct));
	if (f==0)
		{
		sceIoClose(fd);
		return("gu_font_load: malloc failed on f");
		}
	
	gu_font_safe_constructor( f );
	strncpy(f->name,name,MAX_NAME_SIZE-1);
	f->id = gu_font_manager.id_counter++;
	f->d_width = 128;
	f->d_height = 256;
	
	f->clut = memalign( 16, 16*2 );
	if (f->clut==0)
		{
		free(f);
		sceIoClose(fd);
		return("gu_font_load: memalign failed on clut");
		}
	
	struct gu_font_header_struct header;
	sceIoRead( fd,&header,sizeof(struct gu_font_header_struct) );
	if ((header.ID>>8)!=(FOURCC('G','U','F','n')>>8))
		{
		free(f);
		sceIoClose(fd);
		return("gu_font_load: wrong filetype");
		}

	// safety check for old file versions, although this shouldn't be neccessary
	if ((header.ID&0xFF)==(unsigned char)'1')
		{
		header.flags=0;
		header.submaps = 0;
		}

	f->n_submaps = header.submaps+1;
	f->charmaps = malloc(f->n_submaps * sizeof(struct gu_font_charmap_struct));
	if (f->charmaps==0)
		{
		free(f);
		sceIoClose(fd);
		return("gu_font_load: malloc failed on f->charmaps");
		}
	
	int charmapsize = 1;
	f->haveflags = 0;
	if ((header.flags&GU_FONT_HAS_ITALIC)!=0)
		{
		f->hasitalic = 1;
		f->haveflags |= GU_FONT_ITALIC;
		}
	if ((header.flags&GU_FONT_HAS_BOLD)!=0)
		{
		f->hasbold = 1;
		f->haveflags |= GU_FONT_BOLD;
		}
	if ((f->haveflags==GU_FONT_ITALIC) || (f->haveflags==GU_FONT_BOLD))
		{
		f->d_width <<= 1;
		charmapsize = 2;
		}
	else if (f->haveflags==GU_FONT_BOLDITALIC)
		{
		f->d_width <<= 1;
		f->d_height<<= 1;
		charmapsize = 4;
		}

	struct gu_font_list_struct* new_list = malloc(sizeof(struct gu_font_list_struct));
	if (new_list==0)
		{
		free(f);
		sceIoClose(fd);
		return("gu_font_load: malloc failed on new_list");
		}
	gu_font_list_safe_constructor(new_list);

	if ((header.flags&GU_FONT_HAS_UNICODE_CHARMAP)!=0)
		{
		f->unicode_hash = malloc(UNICODE_HASH_SIZE*sizeof(struct gu_font_unicode_list_struct *));
		if (f->unicode_hash!=0)
			{
			gu_font_unicode_hash_safe_constructor( f );
			int n_charcodes = f->n_submaps * 256;
			unsigned short *unicode_charmap = malloc(n_charcodes*sizeof(unsigned short));
			//unsigned short unicode_charmap[n_charcodes]; <- crash, stack too small most likely
			if (unicode_charmap==0)
				{
				free(f->unicode_hash);
				goto READ_CHARMAPS;
				}
			sceIoRead( fd,unicode_charmap,n_charcodes*sizeof(unsigned short) );
			f->haveflags |= GU_FONT_HAS_UNICODE_CHARMAP;
			int k = 0;
			while (k<n_charcodes)
				{
				if (unicode_charmap[k]!=0)
					{
					struct gu_font_unicode_list_struct* new_item = malloc(sizeof(struct gu_font_unicode_list_struct));
					if (new_item==0)
						{
						f->haveflags &= ~GU_FONT_HAS_UNICODE_CHARMAP;
						gu_font_unicode_hash_safe_destructor( f );
						f->unicode_hash = 0;
						break;
						}
					
					unsigned int hash = HASH(unicode_charmap[k]);
					new_item->unicode = unicode_charmap[k];
					new_item->charcode = k;
					new_item->next = f->unicode_hash[hash];
					f->unicode_hash[hash] = new_item;
					}
				k++;
				}
			free(unicode_charmap);
			}
		}

READ_CHARMAPS:;
	
	int k;
	for (k=0;k<f->n_submaps;k++)
		{
		f->charmaps[k].chars = malloc(charmapsize*sizeof(struct gu_char_struct)*256);
		if (f->charmaps[k].chars==0)
			{
			free(f);
			sceIoClose(fd);
			free(new_list);
			sceIoClose(fd);
			return("gu_font_load: malloc failed on charmap");
			}
	
		f->charmaps[k].data = memalign( 16, f->d_width*f->d_height );
		if (f->charmaps[k].data==0)
			{
			free(f->charmaps[k].chars);
			free(f);
			free(new_list);
			sceIoClose(fd);
			return("gu_font_load: memalign failed on f->data");
			}
		
		sceIoRead( fd,f->charmaps[k].chars,charmapsize*sizeof(struct gu_char_struct)*256 );
		sceIoRead( fd,f->charmaps[k].data,f->d_width*f->d_height );
		
		/* Normalize character offsets */
		int j;
		for (j=0;j<charmapsize;j++)
			{
			char y_min = 127;
			char x_min = 127;
			int i;
			for (i=0;i<256;i++)
				{
				if (f->charmaps[k].chars[(j<<8)+i].y_offset<y_min) y_min=f->charmaps[k].chars[(j<<8)+i].y_offset;
				if (f->charmaps[k].chars[(j<<8)+i].x_offset<x_min) x_min=f->charmaps[k].chars[(j<<8)+i].x_offset;
				}
		
			for (i=0;i<256;i++)
				{
				f->charmaps[k].chars[(j<<8)+i].y_offset -= y_min;
				f->charmaps[k].chars[(j<<8)+i].x_offset -= x_min;
				}
			}
		}

	
	sceIoClose( fd );
	
	sceKernelDcacheWritebackAll();
	gu_font_manager.num_fonts++;
	
	new_list->font = f;
	new_list->next = gu_font_manager.list;
	gu_font_manager.list = new_list;
	gu_cur_font = f;
	
	gu_font_color_set( 0xffffff );
	gu_font_border_color_set( 0x000000 );
	gu_font_border_enable( 1 );

	return(0);
	}


void gu_font_free( char* name )
	{
	struct gu_font_list_struct* f = gu_font_list_remove( name );
	gu_font_list_safe_destructor( f );
	}
	

void gu_font_free_all()
	{
	struct gu_font_list_struct* f = gu_font_manager.list;
	struct gu_font_list_struct* t = 0;
	
	while (f!=0)
		{
		t = f->next;
		gu_font_list_safe_destructor(f);
		f = t;
		}
	gu_font_manager.list = 0;
	gu_font_manager.num_fonts = 0;
	gu_font_manager.id_counter = 0;
	}


void gu_font_set( char* name )
	{
	struct gu_font_struct* f = gu_font_find( name );
	if (f!=0)
		gu_cur_font = f;
	}


void* gu_font_texture_get( char* name )
	{
	struct gu_font_struct* f = gu_font_find( name );
	if (f==0) return(0);
	return(f->charmaps[0].data);
	}


void gu_font_border_enable( int enable )
	{
	if (gu_cur_font==0) return;
	if (gu_cur_font->border_enable==enable) return;
	gu_cur_font->border_enable = enable;
	if (enable)
		{
		gu_font_border_color_set( gu_cur_font->border_color );
		gu_font_color_set( gu_cur_font->color );
		}
	else
		{
		unsigned short* clut = (unsigned short*)(((unsigned int)gu_cur_font->clut)|0x40000000);
		int i;
		for (i=0;i<8;i++)
			*(clut++) = 0x0;
		for (i=8;i<16;i++)
			*(clut++) = (i << 12) | BGR444(gu_cur_font->color);
		sceKernelDcacheWritebackAll();
		}
	}
	

void gu_font_border_color_set( int color )
	{
	if (gu_cur_font==0) return;
	gu_cur_font->border_color = color;
	if (gu_cur_font->border_enable==0) return;

	int alphatab[7] = { 2,4,7,10,14,15,15 };	// some pseudo quadratic-logarithmic distribution
	unsigned short* clut = (unsigned short*)(((unsigned int)gu_cur_font->clut)|0x40000000);
	int i;
	*(clut++) = 0x0;		// Transparent parts always black
	for (i=1;i<8;i++)
		*(clut++) = (alphatab[i-1] << 12) | BGR444(color);

	sceKernelDcacheWritebackAll();
	}
	

void gu_font_color_set( int color )
	{
	if (gu_cur_font==0) return;
	gu_cur_font->color = color;
	
	unsigned short* clut = (unsigned short*)(((unsigned int)gu_cur_font->clut)|0x40000000);
	int i;
	clut+=8;
	if (gu_cur_font->border_enable==0)
		{
		for (i=8;i<16;i++)
			*(clut++) = (i << 12) | BGR444(color);
		}
	else
		{
		for (i=8;i<16;i++)
			*(clut++) = (15 << 12) | BGR444_blend1(color,i);
		}
	sceKernelDcacheWritebackAll();
	}



// FIXME: returned width for asian chars is too small
int gu_font_line_width_get( char* s )
	{
	if (s==0 || gu_cur_font==0) return 0;
	char* c = s;
	int x = 0;
	int style = 0;
	int lastcharstyle = 0;

	while (*c!='\0' && *c!='\n')
		{
		if (*c==' ')
			{
			x += CHARWIDTH('t');
			}
		else if ((unsigned char)*c>32)
			{
			if (gu_font_parse_bbcode( &c, &style )==0)
				{
				x += CHARWIDTH2((unsigned char)*c,style);
				lastcharstyle = style;
				}
			}
		c++;
		}
	if (lastcharstyle!=0) x+=4;
	x+=8;
	return (x);
	}
	

int gu_font_width_get( char* s, int flag )
	{
	if (s==0 || gu_cur_font==0) return 0;
	char* c = s;
	int x = 0;
	int width = 0;
	if (flag == 0) flag--;
	int style=0;
	int lastcharstyle=0;
	
	while (*c!='\0' && flag!=0)
		{
		if (*c==' ')
			{
			flag--;
			x += CHARWIDTH('t');
			}
		else if (*c=='\n')
			{
			if (lastcharstyle!=0) x+=4;
			if (x>width) width=x;
			x = 0;
			}
		else if ((unsigned char)*c>32)
			{
			if (gu_font_parse_bbcode( &c, &style )==0)
				{
				x += CHARWIDTH2((unsigned char)*c,style);
				lastcharstyle = style;
				}
			}
		c++;
		}
	if (lastcharstyle!=0) x+=4;
	x+=8;
	return (x>width?x:width);
	}
	

int gu_font_utf8_line_width_get( char* s )
	{
	if (s==0 || gu_cur_font==0) return 0;
	char* c = s;
	int x = 0;
	int style = 0;
	int lastcharstyle = 0;

	while (*c!='\0' && *c!='\n')
		{
		if (*c==' ')
			{
			x += CHARWIDTH('t');
			}
		else if ((unsigned char)*c>32)
			{
			if (gu_font_parse_bbcode( &c, &style )==0)
				{
				unsigned short ucs = gu_font_get_unicodechar(get_next_utf8(&c));
				x += CHARWIDTH2(ucs,style);
				lastcharstyle = style;
				}
			}
		c++;
		}
	if (lastcharstyle!=0) x+=4;
	x+=8;
	return (x);
	}


int gu_font_utf16le_line_width_get( short* s )
	{
	if (s==0 || gu_cur_font==0) return 0;
	short* c = s;
	int x = 0;
	int style = 0;
	int lastcharstyle = 0;

	while (*c!=0 && *c!='\n')
		{
		if (*c==' ')
			{
			x += CHARWIDTH('t');
			}
		else if ((unsigned short)*c>32)
			{
			if (gu_font_utf16le_parse_bbcode( &c, &style )==0)
				{
				unsigned short ucs = gu_font_get_unicodechar(get_next_utf16le(&c));
				x += CHARWIDTH2(ucs,style);
				lastcharstyle = style;
				}
			}
		c++;
		}
	if (lastcharstyle!=0) x+=4;
	x+=8;
	return (x);
	}
	

int gu_font_utf16be_line_width_get( short* s )
	{
	if (s==0 || gu_cur_font==0) return 0;
	short* c = s;
	int x = 0;
	int style = 0;
	int lastcharstyle = 0;
	#define SWAPBYTES(c) (((c&0xFF)<<8)|(c>>8))
	
	while (*c!=0 && SWAPBYTES(*c)!='\n')
		{
		if (SWAPBYTES(*c)==' ')
			{
			x += CHARWIDTH('t');
			}
		else if ((unsigned short)SWAPBYTES(*c)>32)
			{
			if (gu_font_utf16be_parse_bbcode( &c, &style )==0)
				{
				unsigned short ucs = gu_font_get_unicodechar(get_next_utf16be(&c));
				x += CHARWIDTH2(ucs,style);
				lastcharstyle = style;
				}
			}
		c++;
		}
	if (lastcharstyle!=0) x+=4;
	x+=8;
	return (x);
	}
	

int gu_font_utf8_width_get( char* s, int flag )
	{
	if (s==0 || gu_cur_font==0) return 0;
	char* c = s;
	int x = 0;
	int width = 0;
	if (flag == 0) flag--;
	int style=0;
	int lastcharstyle = 0;
	unsigned char ch;
	
	while ((ch=(unsigned char)*c)!='\0' && flag!=0)
		{
		if (ch==' ')
			{
			flag--;
			x += CHARWIDTH('t');
			}
		else if (ch=='\n')
			{
			if (lastcharstyle!=0) x+=4;
			x+=8;
			if (x>width) width=x;
			x = 0;
			}
		else if (ch>32)
			{
			if (gu_font_parse_bbcode( &c, &style )==0)
				{
				unsigned short ucs = gu_font_get_unicodechar(get_next_utf8(&c));
				x += CHARWIDTH2(ucs,style);
				lastcharstyle = style;
				}
			}
		c++;
		}
	if (lastcharstyle!=0) x+=4;
	x+=8;
	return (x>width?x:width);
	}


int gu_font_utf16le_width_get( short* s, int flag )
	{
	if (s==0 || gu_cur_font==0) return 0;
	short* c = s;
	int x = 0;
	int width = 0;
	if (flag == 0) flag--;
	int style=0;
	int lastcharstyle=0;
	unsigned short ch;
	
	while ((ch=(unsigned short)*c)!=0 && flag!=0)
		{
		if (ch==' ')
			{
			flag--;
			x += CHARWIDTH('t');
			}
		else if (ch=='\n')
			{
			if (lastcharstyle!=0) x+=4;
			x+=8;
			if (x>width) width=x;
			x = 0;
			}
		else if (ch>32)
			{
			if (gu_font_utf16le_parse_bbcode( &c, &style )==0)
				{
				unsigned short ucs = gu_font_get_unicodechar(ch);
				x += CHARWIDTH2(ucs,style);
				lastcharstyle = style;
				}
			}
		c++;
		}
	if (lastcharstyle!=0) x+=4;
	x+=8;
	return (x>width?x:width);
	}


int gu_font_utf16be_width_get( short* s, int flag )
	{
	if (s==0 || gu_cur_font==0) return 0;
	short* c = s;
	int x = 0;
	int width = 0;
	if (flag == 0) flag--;
	int style=0;
	int lastcharstyle=0;
	#define SWAPBYTES(c) (((c&0xFF)<<8)|(c>>8))
	
	unsigned short ch;
	while ((ch=(unsigned short)SWAPBYTES((*c)))!=0 && flag!=0)
		{
		if (ch==' ')
			{
			flag--;
			x += CHARWIDTH('t');
			}
		else if (ch=='\n')
			{
			if (lastcharstyle!=0) x+=4;
			x+=8;
			if (x>width) width=x;
			x = 0;
			}
		else if (ch>32)
			{
			if (gu_font_utf16be_parse_bbcode( &c, &style )==0)
				{
				unsigned short ucs = gu_font_get_unicodechar(ch);
				x += CHARWIDTH2(ucs,style);
				lastcharstyle = style;
				}
			}
		c++;
		}
	if (lastcharstyle!=0) x+=4;
	x+=8;
	return (x>width?x:width);
	}


int gu_font_height_get( char* s )
	{
	if (s == 0 || gu_cur_font==0) return 0;
	char* c = s;
	int height = FONTHEIGHT;
	
	while (*c!='\0')
		{
		if (*c=='\n')
			{
			height += FONTHEIGHT;
			}
		c++;
		}
	return (height);
	}


int gu_font_utf16be_height_get( short* s )
	{
	if (s == 0 || gu_cur_font==0) return 0;
	short* c = s;
	int height = FONTHEIGHT;
	
	while (*c!=0)
		{
		if (*c=='\n')
			{
			height += FONTHEIGHT;
			}
		c++;
		}
	return (height);
	}


int gu_font_utf16le_height_get( short* s )
	{
	if (s == 0 || gu_cur_font==0) return 0;
	short* c = s;
	int height = FONTHEIGHT;
	
	while (*c!=0)
		{
		if ((*c>>8)=='\n')
			{
			height += FONTHEIGHT;
			}
		c++;
		}
	return (height);
	}


inline int gu_font_height()
	{
	if (gu_cur_font==0) return(0);
	return FONTHEIGHT;
	}


void gu_font_printf( int x, int y, int flags, char* fmt, ... )
	{
	if (gu_cur_font==0) return;
	
	va_list         ap;
	char   p[512];

	va_start( ap,fmt );
	vsnprintf( p,512,fmt,ap );
	va_end( ap );

	gu_font_print( x, y, flags, p );
	}


void gu_font_print( int x, int y, int flags, char* s )
	{
	if (gu_cur_font==0 || s==0) return;
	
	if (!IsSet(gu_cur_font->haveflags,GU_FONT_HAS_UNICODE_CHARMAP))
		{
		flags &= ~FLAG_UTF8;
		flags &= ~FLAG_UTF16BE;
		flags &= ~FLAG_UTF16LE;
		}
	
	int width, height;
	if (IsSet(flags,FLAG_UTF16BE))
		{
		if (s[0]=='\0'&&s[1]==0) return;
		width = gu_font_utf16be_width_get( (short*)s, 0 );
		height = gu_font_utf16be_height_get( (short*)s );
		}
	else
	if (IsSet(flags,FLAG_UTF16LE))
		{
		if (s[0]=='\0'&&s[1]==0) return;
		width = gu_font_utf16le_width_get( (short*)s, 0 );
		height = gu_font_utf16le_height_get( (short*)s );
		}
	else
	if (IsSet(flags,FLAG_UTF8))
		{
		if (s[0]=='\0') return;
		width = gu_font_utf8_width_get( s, 0 );
		height = gu_font_height_get( s );
		}
	else
		{
		if (s[0]=='\0') return;
		width = gu_font_width_get( s, 0 );
		height = gu_font_height_get( s );
		}
		
	int stride = next_pow2(width);
	int tex_height = next_pow2(height);

	struct gu_glyph_cache_struct* gu_drawbuffer = gu_glyph_cache_manager_get( s, width, height, flags, gu_cur_font->id );
	if (gu_drawbuffer==0) return;

	if (gu_drawbuffer->cacheptr==0) return;

	if (gu_drawbuffer->dirty==1)
		{
		if (IsSet(flags,FLAG_UTF16BE)||IsSet(flags,FLAG_UTF16LE))
			gu_font_utf16_cache_string( (short*)s, width, flags, gu_drawbuffer->cacheptr, stride >> 1 );
		else
			gu_font_cache_string( s, width, flags, gu_drawbuffer->cacheptr, stride >> 1 );
		sceGuTexFlush();
		}


	// string is now in cache, so just draw
	sceGuClutMode(GU_PSM_4444,0,0xff,0); // 16-bit palette
	sceGuClutLoad((8/8),gu_cur_font->clut); // upload 2*8 entries (16)
	sceGuTexMode(GU_PSM_T4,0,0,0); // 4-bit image, but unswizzled
	sceGuTexImage(0,stride,tex_height,stride,gu_drawbuffer->cacheptr);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuEnable( GU_BLEND );

	if (IsSet(flags,FLAG_SHADOW))
		{
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
		blit_fast( 0, 0, width, height, x+2, y+2 );
		}
	
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	blit_fast( 0, 0, width, height, x, y );
	}
	
