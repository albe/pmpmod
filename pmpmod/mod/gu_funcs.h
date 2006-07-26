

#ifndef gu_funcs_h
#define gu_funcs_h
extern unsigned int g_interface_alpha;

static unsigned int __attribute__((aligned(16))) g_list[2][65536];
static unsigned int	g_list_index = 0;

struct Vertex
{
	float u, v;
	unsigned short color;
	short x, y, z;
};

struct VertexInt
{
	unsigned short u, v;
	unsigned short color;
	short x, y, z;
};


void advancedBlit(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, int slice);
void spriteBlit( int dx, int dy, int sx, int sy, int sw, int sh );

#endif

