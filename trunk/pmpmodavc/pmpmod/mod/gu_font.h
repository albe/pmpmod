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

#ifndef gu_font_h__
#define gu_font_h__


//#include "mmgr.h"
//#define DEBUG


#define MAX_NAME_SIZE 256
#define MAX_CACHE_LINES 8
#define MAX_CACHE_STRING 512
#define MAX_GLYPH_CACHES 32


#define FLAG_NONE			0
#define FLAG_ALIGN_LEFT		0
#define FLAG_ALIGN_CENTER	1
#define FLAG_ALIGN_RIGHT	2
#define FLAG_ALIGN_MASK		3
#define FLAG_SHADOW			4
#define FLAG_NOCACHE		8			// Bypass glyph caching (for text that changes every frame)
#define FLAG_UTF8			16			// interpret input string as UTF8
#define FLAG_UTF16BE		32
#define FLAG_UTF16LE		64



#define FLAG_CLIP_WRAP	0x10
#define FLAG_CLIP_CLAMP	0x20


#define GLYPH_CACHE_VRAM 	0x1000
#define GLYPH_CACHE_SYSMEM	0x2000


// Flags defined in font header
#define GU_FONT_HAS_NORMAL	0x0
#define GU_FONT_HAS_ITALIC	0x1
#define GU_FONT_HAS_BOLD	0x2
#define GU_FONT_HAS_NO_BORDER 0x10
#define GU_FONT_HAS_UNICODE_CHARMAP 0x20


#define GU_FONT_NORMAL GU_FONT_HAS_NORMAL
#define GU_FONT_ITALIC GU_FONT_HAS_ITALIC
#define GU_FONT_BOLD GU_FONT_HAS_BOLD
#define GU_FONT_BOLDITALIC (GU_FONT_HAS_ITALIC|GU_FONT_HAS_BOLD)


#define GU_FONT_REPLACEMENT_CHAR	((unsigned char)'?')

// primes do have their value :)
#define UNICODE_HASH_SIZE 1021 //661 //383
#define UNICODE_HASH_MULT 383 //12167


#define FOURCC(A,B,C,D) (unsigned long)((unsigned char)A << 24 | (unsigned char)B << 16 | (unsigned char)C << 8 | (unsigned char)D)

#define BGR444(col) (((col & 0xF0) >> 4) | ((col & 0xF000) >> 8) | ((col & 0xF00000) >> 12))
#define BGR444_blend1(col,i) ((((col & 0xF0)*i/15) >> 4) | (((col & 0xF000)*i/15) >> 8) | (((col & 0xF00000)*i/15) >> 12))
#define BGR444_blend2(col,col2,i) ((((int)((col & 0xF0)*i+(col2 & 0xF0)*(15-i))/15) >> 4) | (((int)((col & 0xF000)*i+(col2 & 0xF000)*(15-i))/15) >> 8) | (((int)((col & 0xF00000)*i+(col2 & 0xF00000)*(15-i))/15) >> 12))


//extern unsigned short gu_font_clut[16];


struct gu_font_unicode_list_struct {
	unsigned short			unicode;
	unsigned short			charcode;
	struct gu_font_unicode_list_struct *next;
};


struct gu_char_struct {
	char		byte_width;
	char		byte_height;
	char		x_offset;
	char		y_offset;
	char		pixel_width;
	char		pixel_height;
};


struct gu_font_charmap_struct {
	unsigned char			*data;
	struct gu_char_struct	*chars;
};


struct gu_font_struct {
	char					name[MAX_NAME_SIZE];
	int						id;
	unsigned int			color;
	unsigned int			border_color;
	unsigned int			border_enable;
	unsigned short			*clut; //__attribute__((aligned(16))) clut[16];

	struct gu_font_charmap_struct	*charmaps;
	//struct gu_char_struct	*chars;
	//unsigned char*			data;
	int						d_width;				// Bytewidth of bitmap
	int						d_height;				// Byteheight of bitmap
	int						haveflags;
	int						hasitalic;
	int						hasbold;
	int						n_submaps;				// number of subsequent charmaps
	
	struct gu_font_unicode_list_struct **unicode_hash;
	struct gu_font_unicode_list_struct *unicode_hash_mem;	// allocated hash memory pointer
};


struct gu_font_list_struct {
	struct gu_font_struct	*font;
	struct gu_font_list_struct	*next;
};


struct gu_font_manager_struct {
	struct gu_font_list_struct*	list;
	int			num_fonts;
	int			id_counter;
};


struct gu_font_header_struct {
	int				ID;		// 'GUFn' n = version number
	unsigned char	flags;
	unsigned char	submaps;
	char	reserved[10];
};



// Glyph caching structures

struct gu_glyph_cache_struct {
	void*			cacheptr;
	int				size;
	int				width;
	int				height;
	int				flags;
	char			string[MAX_CACHE_STRING];
	int				length;
	int				font_id;
	int				dirty;
};


struct gu_glyph_cache_list_struct {
	struct gu_glyph_cache_struct 		*cache;
	unsigned long	lru_index;
	
	struct gu_glyph_cache_list_struct	*next;
	struct gu_glyph_cache_list_struct	*prev;
};


struct gu_glyph_cache_manager_struct {
	struct gu_glyph_cache_list_struct *root;
	struct gu_glyph_cache_list_struct *tail;

	unsigned int		max_num_caches;
	unsigned long		lru_counter;
	unsigned int		num_caches;
};



struct VertexInt
{
	unsigned short u, v;
	unsigned short color;
	short x, y, z;
};


extern struct gu_font_struct* gu_cur_font;

// GLYPH CACHE SYSTEM FUNCTIONS
// ** normally there should be no need to call them manually
#ifdef DEBUG
void gu_debug_print_glyph_cache();
void gu_debug_print_charset();
#endif
char* gu_glyph_cache_init();
void gu_glyph_cache_free();
void gu_glyph_cache_reset();	// call this when VRAM gets reset (when calling sceGuTerm)
struct gu_glyph_cache_struct* gu_glyph_cache_manager_get( char* s, int width, int height, int flags, int font_id );
void gu_glyph_cache_manager_limit_glyph_caches( unsigned int n );

char* gu_font_init();							// call once on startup before any gu_font_load calls, sets white color clut with black border
void gu_font_close();
char* gu_font_load( char* name );				// loads font 'name' and makes it current
void gu_font_free( char* name );
void gu_font_free_all();


void gu_font_set( char* name );
void* gu_font_texture_get( char* name );		// to be able to display the texture of a specific font
int gu_font_texture_width_get( char* name );
int gu_font_texture_height_get( char* name );

void gu_font_border_enable( int enable );
void gu_font_border_color_set( int color );
void gu_font_color_set( int color );


unsigned short gu_font_get_unicodechar( unsigned short unicode );

// ASCII
int gu_font_line_width_get( char* s );
// flag < 0 : return width of whole string
// flag > 0 : return width of 'flag' first chars
int gu_font_width_get( char* s, int flag );

// UTF-8
int gu_font_utf8_line_width_get( char* s );
int gu_font_utf8_width_get( char* s, int flag );

// UTF-16BE
int gu_font_utf16be_line_width_get( short* s );
int gu_font_utf16be_width_get( short* s, int flag );
int gu_font_utf16be_height_get( short* s );

// UTF-16LE
int gu_font_utf16le_line_width_get( short* s );
int gu_font_utf16le_width_get( short* s, int flag );
int gu_font_utf16le_height_get( short* s );


int gu_font_height_get( char* s );
int gu_font_height();							// same as gu_char_height_get( 'I' )+3


void gu_font_printf( int x, int y, int flags, char* fmt, ... );	// This only works with non UTF-16 encodings
void gu_font_print( int x, int y, int flags, char* s );

#endif
