#include <pspiofilemgr.h>
#include "pmp_stat.h"

#undef printf

void pmp_stat_load( struct pmp_play_struct *p, char* s )
	{
	if (p==0) return;
	
	snprintf( p->resume_filename, 256, "%s.pos", s);
	
	SceUID	fd;

	// device:path
	if((fd = sceIoOpen( p->resume_filename, PSP_O_RDONLY, 0777)))
		{
		sceIoRead( fd, &p->resume_pos, sizeof(int) );
		sceIoRead( fd, &p->audio_stream, sizeof(int) );
		if (p->audio_stream>p->decoder.reader.file.header.audio.number_of_streams)
			p->audio_stream = 0;
			
		sceIoRead( fd, &p->volume_boost, sizeof(int) );
		if (p->volume_boost>3)
			p->volume_boost = 3;
			
		sceIoRead( fd, &p->aspect_ratio, sizeof(int) );
		if (p->aspect_ratio>=number_of_aspect_ratios)
			p->aspect_ratio = number_of_aspect_ratios-1;
		
		sceIoRead( fd, &p->zoom, sizeof(int) );
		if (p->zoom>200)
			p->zoom = 200;
		if (p->zoom<100)
			p->zoom = 100;
		
		sceIoRead( fd, &p->luminosity_boost, sizeof(int) );
		if (p->luminosity_boost>=number_of_luminosity_boosts)
			p->luminosity_boost = number_of_luminosity_boosts-1;
			
		sceIoRead( fd, &p->subtitle, sizeof(int) );
		if (p->subtitle>p->subtitle_count)
			p->subtitle = p->subtitle_count;
		
		sceIoRead( fd, &p->subtitle_format, sizeof(int) );
		if (p->subtitle_format>1)
			p->subtitle_format = 1;

		sceIoRead( fd, &p->subtitle_fontcolor, sizeof(int) );
		if (p->subtitle_fontcolor>=NUMBER_OF_FONTCOLORS)
			p->subtitle_fontcolor = NUMBER_OF_FONTCOLORS-1;

		sceIoRead( fd, &p->subtitle_bordercolor, sizeof(int) );
		if (p->subtitle_bordercolor>=NUMBER_OF_BORDERCOLORS)
			p->subtitle_bordercolor = NUMBER_OF_BORDERCOLORS-1;

		sceIoClose( fd );
		}
	}


void pmp_stat_save( struct pmp_play_struct *p )
	{
	if (p==0) return;
	
	SceUID	fd;
	
	// device:path
	if((fd = sceIoOpen( p->resume_filename, PSP_O_WRONLY|PSP_O_CREAT, 0777)))
		{
		sceIoWrite( fd, &p->last_keyframe_pos, sizeof(int) );
		//printf("pos.\n");
		sceIoWrite( fd, &p->audio_stream, sizeof(int) );
		//printf("audio.\n");
		sceIoWrite( fd, &p->volume_boost, sizeof(int) );
		//printf("volume.\n");
		sceIoWrite( fd, &p->aspect_ratio, sizeof(int) );
		//printf("aspect.\n");
		sceIoWrite( fd, &p->zoom, sizeof(int) );
		//printf("zoom.\n");
		sceIoWrite( fd, &p->luminosity_boost, sizeof(int) );
		//printf("lumi.\n");
		sceIoWrite( fd, &p->subtitle, sizeof(int) );
		//printf("subtitle.\n");
		sceIoWrite( fd, &p->subtitle_format, sizeof(int) );
		//printf("sub format.\n");
		sceIoWrite( fd, &p->subtitle_fontcolor, sizeof(int) );
		//printf("sub forecolor.\n");
		sceIoWrite( fd, &p->subtitle_bordercolor, sizeof(int) );
		//printf("sub bordercolor.\n");
		sceIoClose( fd );
		}
	else
		{
		sceIoRemove( p->resume_filename );	// Delete the file if something got wrong, or else there's a 0kb file
		}

	}
