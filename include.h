#pragma once
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a,b)             (((a) < (b)) ? (a) : (b))
#define MAX(a,b)             (((a) > (b)) ? (a) : (b))
#define CLAMP(a, mi,ma)      MIN(MAX(a,mi),ma)


#pragma pack(1)
typedef struct
{
	short start_vert;
	short end_vert;
	short flags;
	short special;
	short sector;
	short right_sidedef;
	short left_sidedef;
} linedef_t;
#pragma pack(8)

#pragma pack(1)
typedef struct
{
	short x;
	short y;
	short angle;
	short type;
	short flags;
} thing_t;
#pragma pack(8)

#pragma pack(1)
typedef struct
{
	short x;
	short y;
} vert_t;
#pragma pack(8)

#pragma pack(1)
typedef struct
{
	short tex_xoffset;
	short tex_yoffset;
	char tex_upper[8];
	char tex_lower[8];
	char tex_middle[8];
	// front sector towards viewer
	short sector;
} sidedef_t;
#pragma pack(8)

#pragma pack(1)
// Segments/Cells enclosed area by lines may not be convex
typedef struct
{
	short floor_height;
	short ceil_height;
	char tex_floor[8];
	char tex_ceil[8];
	short light_level;
	short type;
	short tag;
} sector_t;
#pragma pack(8)

#pragma pack(1)
// segment of a line split by partition line
typedef struct
{
	short start_vert;
	short end_vert;
	short angle;
	short linedef;
	short sidedef;
	short offset;
} seg_t;
#pragma pack(8)

#pragma pack(1)
// BSP split segments (cells) always convex
typedef struct
{
	short num_seg;
	short first_seg;
} subsector_t;
#pragma pack(8)

#pragma pack(1)
typedef struct
{
	// Partition line from (x,y) to (x+dx,y+dy)
	short x;
	short y;
	short dx;
	short dy;
	short bbox[2][4]; // right then left
	short child[2]; // right then left
} node_t;
#pragma pack(8)

typedef struct
{
	vert_t *pvert;
	int num_vert;
	linedef_t *pline;
	int num_line;
	thing_t *pthing;
	int num_thing;
	sidedef_t *pside;
	int num_side;
	sector_t *psector;
	int num_sector;
	seg_t *pseg;
	int num_seg;
	subsector_t *psubsector;
	int num_subsector;
	node_t *pnode;
	int num_node;
	// lazy hacks
	HDC hdc;
	HPEN red, green[255], green_dash[255], blue;
	float scale;
	float offset_x;
	float offset_y;
	int max_depth;
	int max_draw;
	HBRUSH cross_brush;
	bool draw_order;
} waddata_t;



#pragma pack(1)
typedef struct
{
	char type[2];
	int file_size;
	int reserved;
	int offset;
} bmpheader_t;
#pragma pack(8)

#pragma pack(1)
typedef struct
{
	int size;
	int width;
	int height;
	short planes;
	short bpp;
	int compression;
	int image_size;
	int xres;
	int yres;
	int clr_used;
	int clr_important;
} dib_t;
#pragma pack(8)

#pragma pack(1)
typedef struct
{
	bmpheader_t	header;
	dib_t		dib;
} bitmap_t;
#pragma pack(8)

typedef struct
{
	float x;
	float y;
	float scale;
} offset_t;

typedef struct
{
	float x, y;
} vec2;

typedef struct
{
	float x, y, z;
} vec3;

typedef struct
{
	vec3 position;      // Current position
	vec3 velocity;   // Current motion vector
	float angle;   // Looking towards (and sin() and cos() thereof)
}  player_t;



char *get_file(char *filename, unsigned int *size);
void *get_wadfile(char *wadfile, char *level, char *lump, int *lump_size, char **pdata);

#define THING_PLAYER1			0x1
#define THING_PLAYER2			0x2
#define THING_PLAYER3			0x3
#define THING_PLAYER4			0x4
#define THING_BLUEKEY			0x5
#define THING_YELLOWKEY			0x6
#define THING_SPIDERDEMON		0x7
#define THING_BACKPACK			0x8
#define THING_SHOTGUNGUY		0x9
#define THING_BLOODYMESS		0xA
#define THING_DMSTART			0xB
#define THING_BLOODYMESS2		0xC
#define THING_REDKEY			0xD
#define THING_TELEPORTEND		0xE
#define THING_DEADPLAYER		0xF

#define THING_BARREL			0x7F3
#define THING_SHELLS			0x7D8
#define THING_ROCKET			0x7DA
#define THING_MEDIKIT			0x7DC
#define THING_HEALTH			0x7DE
#define THING_BOXBULLET			0x800
#define THING_BOXSHELLS			0x801
#define THING_BOXROCKET			0x8FE

#define THING_IMP				0x3001
#define THING_DEMON				0x3002
#define THING_BARON				0x3003
#define THING_ZOMBIEMAN			0x3004
