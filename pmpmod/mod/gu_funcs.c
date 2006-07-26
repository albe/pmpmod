

#include <pspgu.h>
#include "gu_funcs.h"

void advancedBlit(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, int slice)
{
	int		start, end;
	float	ustart = (float)sx;
	float	ustep = (float)((float)sw / (float)dw) * (float)slice;

	// blit maximizing the use of the texture-cache

	for (start = sx, end = sx+dw; start < end; start += slice, dx += slice)
	{
		struct Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
		int width = (start + slice) < end ? slice : end-start;
		if (width<slice) ustep = sw+sx-ustart;

		vertices[0].u = ustart; 
		vertices[0].v = (float)sy;
		vertices[0].color = (g_interface_alpha) << 12 | (0xfff);
		vertices[0].x = dx; 
		vertices[0].y = dy; 
		vertices[0].z = 0;

		ustart += ustep;
		vertices[1].u = ustart;
		vertices[1].v = (float)(sy + sh);
		vertices[1].color = (g_interface_alpha) << 12 | (0xfff);
		vertices[1].x = dx + width; 
		vertices[1].y = dy + dh;
		vertices[1].z = 0;

		sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_4444|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
	}
}

void spriteBlit( int dx, int dy, int sx, int sy, int sw, int sh )
{
	struct VertexInt* vertices = (struct VertexInt*)sceGuGetMemory(2 * sizeof(struct VertexInt));

	vertices[0].u = sx; 
	vertices[0].v = sy;
	vertices[0].color = (g_interface_alpha) << 12 | (0xfff);
	vertices[0].x = dx; 
	vertices[0].y = dy; 
	vertices[0].z = 0;

	vertices[1].u = sx+sw; 
	vertices[1].v = sy+sh;
	vertices[1].color = (g_interface_alpha) << 12 | (0xfff);
	vertices[1].x = dx+sw; 
	vertices[1].y = dy+sh; 
	vertices[1].z = 0;

	sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_COLOR_4444|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
}
