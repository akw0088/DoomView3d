#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include "include.h"
#include <time.h>


#define WM_TICK 1

void draw_pixels(HDC hdc, HDC hdcMem, int width, int height, waddata_t &data);
void draw_text(unsigned int *vram, unsigned int *tex, char *msg, int x, int y, int xres, int yres);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

char *vram = NULL;
unsigned int xres;
unsigned int yres;
char info[512];
int draw_count = 0;
unsigned int tex_data[1024 * 1024 * 4];
unsigned int *tex = &tex_data[0];
bool solid = false;
bool flag = false;
player_t player;

#define MAX_LEVEL 9
char level_name[MAX_LEVEL][80] = {
	"E1M1",
	"E1M2",
	"E1M3",
	"E1M4",
	"E1M5",
	"E1M6",
	"E1M7",
	"E1M8",
	"E1M9",
};



offset_t level_offsets[MAX_LEVEL] = {
	{ 1180, 4880, 2.5f },
	{ 4700, 1460, 1.5f },
	{ 4720, 3740, 2.0f },
	{ 2680,  640, 2.5 },
	{ 3780, 920, 2.0f },
	{ 4780, 2420, 1.5 },
	{ 4140, 2840, 2.0f },
	{ 6360, 520, 1.0f },
	{ 3220, 1600, 2.0 }
};


void draw_line(int *pixels, int width, int height, int x1, int y1, int x2, int y2, int color);
void draw_rect(int *pixels, int width, int height, float angle, int w, int l, int x, int y, int color);

void Line(waddata_t data, int x1, int y1, int x2, int y2, int view_height)
{
	// center in screen
	// bias keeps us to upper left
	x1 = x1 + xres / 2;
	y1 = y1 + yres / 2 - view_height;
	x2 = x2 + xres / 2;
	y2 = y2 + yres / 2 - view_height;



	// can texture map here, but would need too load BMP's, will probably fix floor/ceiling heights first
	draw_line((int *)vram, xres, yres, x1, y1, x2, y2, 0);
}


void Rectangle(int x1, int y1, int x2, int y2, int color)
{
	draw_line((int *)vram, xres, yres, x1, y1, x2, y1, color); // top line
	draw_line((int *)vram, xres, yres, x1, y2, x2, y2, color); // bottom line

	draw_line((int *)vram, xres, yres, x1, y1, x1, y2, color); // left line
	draw_line((int *)vram, xres, yres, x2, y1, x2, y2, color); // right line
}

void Triangle(int x1, int y1, int x2, int y2, int color)
{
	int half_x = fabs(x2 - x1) / 2;

	draw_line((int *)vram, xres, yres, x1, y2, x2, y2, color); // bottom line
	draw_line((int *)vram, xres, yres, x1 + half_x, y1, x1, y2, color); // left line
	draw_line((int *)vram, xres, yres, x2 - half_x, y1, x2, y2, color); // right line
}



int cross(int x1, int y1, int x2, int y2)
{
	return x1 * y2 - y1 * x2;
}

void intersect(int x1, int y1,
	int x2, int y2,
	int x3, int y3,
	int x4, int y4,
	int &x, int &y)
{
	x = cross(x1, y1, x2, y2);
	int det = cross(x1 - x2, y1 - y2, x3 - x4, y4 - y4);

	if (det != 0)
	{
		x = cross(x, x1 - x2, y, x3 - x4) / det;
		y = cross(x, y1 - y2, y, y3 - y4) / det;
	}
}

inline int lerp(int a, int b, float time)
{
	return a * (1 - time) + b * time;
}


void project_line(waddata_t &data, float angle, float px, float py, float vx1, float vy1, float vx2, float vy2, float ceil_height, float floor_height)
{
	float tx1 = vx1 - px; // transformed x1 (eg transformed to player origin)
	float ty1 = vy1 - py;
	float tx2 = vx2 - px;
	float ty2 = vy2 - py;
	float tz1 = 0;
	float tz2 = 0;

	// rotate based on players view angle
	tz1 = tx1 * cos(angle) + ty1 * sin(angle);
	tz2 = tx2 * cos(angle) + ty2 * sin(angle);
	tx1 = tx1 * sin(angle) - ty1 * cos(angle);
	tx2 = tx2 * sin(angle) - ty2 * cos(angle);


	float x1 = 0;
	float y1a = 0;
	float y1b = 0;
	float x2 = 0;
	float y2a = 0;
	float y2b = 0;


	float height = (ceil_height - floor_height);
	float width_scale = 8 * 16;
	float height_scale = 8 * 4;
	float depth_scale = 0.25f;
	float floor = 0.0f;
	float down_height = height;


	if (flag)
	{
		depth_scale = 0.125f;
	}

	// both points behind player, exit
	if (tz1 < 0 && tz2 < 0)
	{
		return;
	}

	// Normal case, both points infront of player
	if (tz1 > 0 && tz2 > 0)
	{
		x1 = -(tx1 * width_scale) / (tz1 * depth_scale);
		y1a = (height * height_scale) / (tz1 * depth_scale); // this is effectively the +/- height of the wall
		y1b = -(down_height * height_scale) / (tz1 * depth_scale);
		x2 = -(tx2 * width_scale) / (tz2 * depth_scale);
		y2a = (height * height_scale) / (tz2 * depth_scale);
		y2b = -(down_height * height_scale) / (tz2 * depth_scale);

		if (solid)
		{
			for (int x = x1; x < x2; x++)
			{
				float t = fabs(x - x1) / fabs(x2 - x1);

				// need to interpolate between (y1a,y1b) and (y2a,y2b)
				int y1 = lerp(y1a, y2a, t);
				int y2 = lerp(y1b, y2b, t);
				Line(data, x, -y1, x, -y2, floor); // top line
			}
		}
		else
		{
			Line(data, x1, -y1a, x2, -y2a, floor); // top line
			Line(data, x1, -y1b, x2, -y2b, floor); // bottom line
			Line(data, x1, -y1a, x1, -y1b, floor); // vertical 1
			Line(data, x2, -y2a, x2, -y2b, floor); // vertical 2
		}
		return;
	}


	// At least one point of the line is infront of the player
	if (tz1 > 0 || tz2 > 0)
	{
		int ix1;
		int iz1;
		int ix2;
		int iz2;


		// if point z1 is behind us
		if (tz1 <= 0)
		{
			// intersect with clip planes outputs intersected points X and Z
			intersect(tx1, tz1, tx2, tz2, -0.0001, 0.0001, -20, 5, ix1, iz1);

			// if intersected point infront, use it
			if (iz1 > 0)
			{
				tx1 = ix1;
				tz1 = iz1;
			}
			else
			{
				// clipping failed, abort
				return;
			}
		}

		// if point z2 is behind us
		if (tz2 <= 0)
		{
			// intersect with clip planes outputs intersected points X and Z
			intersect(tx1, tz1, tx2, tz2, 0.0001, 0.0001, 20, 5, ix2, iz2);

			// if intersected point infront of us, use it
			if (iz2 > 0)
			{
				tx2 = ix2;
				tz2 = iz2;
			}
			else
			{
				// clipping failed, abort
				return;
			}
		}


		if (tz1 != 0)
		{
			x1 = -(tx1 * width_scale) / (tz1 * depth_scale);
			y1a = -(height * height_scale) / (tz1 * depth_scale);
			y1b = (down_height * height_scale) / (tz1 * depth_scale);
		}

		if (tz2 != 0)
		{
			x2 = -(tx2 * width_scale) / (tz2 * depth_scale);
			y2a = -(height * height_scale) / (tz2 * depth_scale);
			y2b = (down_height * height_scale) / (tz2 * depth_scale);
		}
	}

	if (solid)
	{
		for (int x = x1; x < x2; x++)
		{
			float t = x / fabs(x2 - x1);

			// need to interpolate between (y1a,y1b) and (y2a,y2b)
			int y1 = lerp(y1a, y2a, t);
			int y2 = lerp(y1b, y2b, t);
			Line(data, x, -y1, x, -y2, floor);
		}
	}
	else
	{
		Line(data, x1, -y1a, x2, -y2a, floor); // top line
		Line(data, x1, -y1b, x2, -y2b, floor); // bottom line
		Line(data, x1, -y1a, x1, -y1b, floor); // vertical 1
		Line(data, x2, -y2a, x2, -y2b, floor); // vertical 2
	}

}

void DrawPlaneMode7(int xres, int yres, unsigned int *texture, int tex_width, int tex_height, float pscale, float px, float py, float pangle,
	float zNear, float zFar, float fov)
{
	float FoVHalf = (fov / 180.0f) * (3.14159f / 2.0f);

	// Frustum corner points
	float FarX1 = px * pscale + cosf(pangle - FoVHalf) * zFar;
	float FarY1 = py * pscale + sinf(pangle - FoVHalf) * zFar;

	float NearX1 = px * pscale + cosf(pangle - FoVHalf) * zNear;
	float NearY1 = py * pscale + sinf(pangle - FoVHalf) * zNear;

	float FarX2 = px * pscale + cosf(pangle + FoVHalf) * zFar;
	float FarY2 = py * pscale + sinf(pangle + FoVHalf) * zFar;

	float NearX2 = px * pscale + cosf(pangle + FoVHalf) * zNear;
	float NearY2 = py * pscale + sinf(pangle + FoVHalf) * zNear;

	for (int y = 0; y < yres; y++)
	{
		float ystep = (float)y / ((float)yres / 2.0f);

		float StartX = (FarX1 - NearX1) / (ystep)+NearX1;
		float StartY = (FarY1 - NearY1) / (ystep)+NearY1;
		float EndX = (FarX2 - NearX2) / (ystep)+NearX2;
		float EndY = (FarY2 - NearY2) / (ystep)+NearY2;

		for (int x = 0; x < xres; x++)
		{
			float xstep = (float)x / (float)xres;
			float u = (EndX - StartX) * xstep + StartX;
			float v = (EndY - StartY) * xstep + StartY;

			unsigned int tx = u * tex_width;
			unsigned int ty = v * tex_height;


			tx = (tx % tex_width);
			ty = (ty % tex_height);

			vram[x + xres * y] = texture[tx + (tex_width - 1) * ty];
		}
	}
}



#define VXS(x0,y0, x1,y1)    ((x0)*(y1) - (x1)*(y0))   // vxs: 2d Vector cross product

// PointSide: Determine which side of a line the point is on. Return value: <0, =0 or >0.
float PointSide(float px, float py, float x0, float y0, float x1, float y1)
{
	return VXS((x1)-(x0), (y1)-(y0), (px)-(x0), (py)-(y0));
}

// Overlap:  Determine whether the two number ranges overlap.
bool Overlap(float a0, float a1, float b0, float b1)
{
	return (MIN(a0, a1) <= MAX(b0, b1) && MIN(b0, b1) <= MAX(a0, a1));
}

// IntersectBox: Determine whether two 2D-boxes intersect.
bool IntersectBox(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
{
	return (Overlap(x0, x1, x2, x3) && Overlap(y0, y1, y2, y3));
}


// Intersect: Calculate the point of intersection between two lines.
void Intersect(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, vec2 *rval)
{
	float xi = VXS(VXS(x1, y1, x2, y2), (x1)-(x2), VXS(x3, y3, x4, y4), (x3)-(x4)) / VXS((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4));
	float yi = VXS(VXS(x1, y1, x2, y2), (y1)-(y2), VXS(x3, y3, x4, y4), (y3)-(y4)) / VXS((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4));

	rval->x = x1;
	rval->y = x1;
}

char *get_file(char *filename, unsigned int *size)
{
	FILE	*file;
	char	*buffer;
	int	file_size, bytes_read;

	file = fopen(filename, "rb");
	if (file == NULL)
		return 0;
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	buffer = new char[file_size + 1];
	bytes_read = (int)fread(buffer, sizeof(char), file_size, file);
	if (bytes_read != file_size)
	{
		delete[] buffer;
		fclose(file);
		return 0;
	}
	fclose(file);
	buffer[file_size] = '\0';

	if (size != NULL)
	{
		*size = file_size;
	}

	return buffer;
}


void load_wad(waddata_t *data, int level)
{
	int lump_size;
	char *pdata = NULL;

	data->pvert = (vert_t *)get_wadfile("DOOM1.WAD", level_name[level], "VERTEXES", &lump_size, &pdata);
	data->num_vert = lump_size / (sizeof(vert_t));

	data->pline = (linedef_t *)get_wadfile("DOOM1.WAD", level_name[level], "LINEDEFS", &lump_size, &pdata);
	data->num_line = lump_size / (sizeof(linedef_t));

	data->pthing = (thing_t *)get_wadfile("DOOM1.WAD", level_name[level], "THINGS", &lump_size, &pdata);
	data->num_thing = lump_size / (sizeof(thing_t));

	data->pside = (sidedef_t *)get_wadfile("DOOM1.WAD", level_name[level], "SIDEDEFS", &lump_size, &pdata);
	data->num_side = lump_size / (sizeof(sidedef_t));

	data->psector = (sector_t *)get_wadfile("DOOM1.WAD", level_name[level], "SECTORS", &lump_size, &pdata);
	data->num_sector = lump_size / (sizeof(sector_t));

	data->pseg = (seg_t *)get_wadfile("DOOM1.WAD", level_name[level], "SEGS", &lump_size, &pdata);
	data->num_seg = lump_size / (sizeof(seg_t));

	data->psubsector = (subsector_t *)get_wadfile("DOOM1.WAD", level_name[level], "SSECTORS", &lump_size, &pdata);
	data->num_subsector = lump_size / (sizeof(subsector_t));

	data->pnode = (node_t *)get_wadfile("DOOM1.WAD", level_name[level], "NODES", &lump_size, &pdata);
	data->num_node = lump_size / (sizeof(node_t));
}



void DrawSubsector(waddata_t &data, short index)
{
	int j;
	float scale = data.scale;
	float offset_x = data.offset_x;
	float offset_y = data.offset_y;

	if (index < 0 || index >= data.num_subsector)
	{
		printf("Fail\r\n");
		return;
	}


	if (draw_count >= data.max_draw)
	{
		return;
	}
	draw_count++;

	int first = data.psubsector[index].first_seg;
	int count = data.psubsector[index].num_seg;

	for (j = first; j < first + count; j++)
	{
		int a = data.pseg[j].start_vert;
		int b = data.pseg[j].end_vert;

		float startx = (data.pvert[a].x * scale + offset_x * scale) / 10.0f;
		float starty = (data.pvert[a].y * scale + offset_y * scale) / 10.0f;
		float endx = (data.pvert[b].x * scale + offset_x * scale) / 10.0f;
		float endy = (data.pvert[b].y * scale + offset_y * scale) / 10.0f;

		linedef_t *line = &data.pline[data.pseg[j].linedef];
		sidedef_t *side;
		if (data.pseg[j].sidedef)
		{
			side = &data.pside[line->left_sidedef];
		}
		else
		{
			side = &data.pside[line->right_sidedef];
		}

		char *tex_high = side->tex_upper;
		char *tex_mid = side->tex_middle;
		char *tex_low = side->tex_lower;

		int ceil = data.psector[side->sector].ceil_height;
		int floor = data.psector[side->sector].floor_height;
		project_line(data, player.angle, player.position.x, player.position.y, startx, starty, endx, endy, ceil, floor);
	}

}

int PointOnSide(float x, float y, node_t *node)
{
	if (PointSide(x, y, node->x, node->y, node->x + node->dx, node->y + node->dy) > 0)
		return 1;
	else
		return 0;
}

bool CheckBBox(short *bbox)
{
	short x0, y0, x1, y1;


	// top, bottom, left, right
	y0 = bbox[0];
	y1 = bbox[1];
	x0 = bbox[2];
	x1 = bbox[3];

	return IntersectBox(x0, y0, x1, y1, (short)player.position.x, (short)player.position.y, (short)player.position.x + 1, (short)player.position.y + 1);
}


void RenderBSPNode(waddata_t &data, short bspnum, int i)
{
	node_t *node;
	int		side;

	// Found a leaf (subsector), draw it
	if (bspnum & 0x8000)
	{
		if (bspnum == -1)
			DrawSubsector(data, 0);
		else
			DrawSubsector(data, bspnum & 0x7FFF);
		return;
	}

	node = &data.pnode[bspnum];

	// Decide which side the view point is on.
	int px = player.position.x * 10.0f / data.scale - data.offset_x;
	int py = player.position.y * 10.0f / data.scale - data.offset_y;

	side = PointOnSide(px, py, node);
	i++;

	if (i >= data.max_depth)
		return;


	// Recursively divide front space.
	RenderBSPNode(data, node->child[side], i);

	// Possibly divide back space.
	if (CheckBBox(node->bbox[side ^ 1]) || data.draw_order == true)
	{
		RenderBSPNode(data, node->child[side ^ 1], i);
	}
}

char *tga_24to32(int width, int height, char *pBits, bool bgr)
{
	int lImageSize = width * height * 4;
	char *pNewBits = new char[lImageSize * sizeof(char)];

	for (int i = 0, j = 0; i < lImageSize; i += 4)
	{
		if (bgr)
		{
			pNewBits[i + 2] = pBits[j++];
			pNewBits[i + 1] = pBits[j++];
			pNewBits[i + 0] = pBits[j++];
		}
		else
		{
			pNewBits[i + 0] = pBits[j++];
			pNewBits[i + 1] = pBits[j++];
			pNewBits[i + 2] = pBits[j++];
		}
		pNewBits[i + 3] = 0;
	}
	return pNewBits;
}


void read_bitmap(char *filename, int &width, int &height, unsigned int **data)
{
	FILE *file;
	bitmap_t	bitmap;

	memset(&bitmap, 0, sizeof(bitmap_t));

	file = fopen(filename, "rb");
	if (file == NULL)
	{
		perror("Unable to write file");
		return;
	}

	fread(&bitmap, 1, sizeof(bitmap_t), file);
	width = bitmap.dib.width;
	height = bitmap.dib.height;
	fread((void *)*data, 1, width * height * 4, file);
	fclose(file);

	if (bitmap.dib.bpp == 24)
	{
		*data = (unsigned int *)tga_24to32(width, height, (char *)*data, true);
	}
}


void Draw(waddata_t &data)
{
	int i;

	// clear screen
	memset(vram, ~0, xres * yres * 4);

	float zNear = 0.0025f;
	float zFar = 0.03f;
	//DrawPlaneMode7(hdc, cxClient, cyClient, &tex[0], 1024, 1024, 0.1f, player.position.x, player.position.y, player.angle, zNear, zFar, 90.0f);
	sprintf(info, "MaxDepth %d MaxDraw %d Player (%d,%d)", data.max_depth, data.max_draw, (int)player.position.x, (int)player.position.y);

	draw_text((unsigned int *)vram, tex, info, 0, 0, xres, yres);


	// draw vertices
	/*
	for (i = 0; i < data.num_vert; i += 2)
	{
		// Transform World->Client
		float x = (data.pvert[i].x * data.scale + data.offset_x * data.scale) / 10.0f;
		float y = (data.pvert[i].y * data.scale + data.offset_y * data.scale) / 10.0f;

		Rectangle(
			(x - 3.0f),
			(y - 3.0f),
			(x + 3.0f),
			(y + 3.0f),
			0);
	}
	*/

	// draw lines
	for (i = 0; i < data.num_line; i++)
	{
		int a = data.pline[i].start_vert;
		int b = data.pline[i].end_vert;

		float startx = (data.pvert[a].x * data.scale + data.offset_x * data.scale) / 10.0f;
		float starty = (data.pvert[a].y * data.scale + data.offset_y * data.scale) / 10.0f;
		float endx = (data.pvert[b].x * data.scale + data.offset_x * data.scale) / 10.0f;
		float endy = (data.pvert[b].y * data.scale + data.offset_y * data.scale) / 10.0f;

		draw_line((int *)vram, xres, yres, startx, starty, endx, endy, 0);
	}

	// draw things
	for (i = 0; i < data.num_thing; i++)
	{
		float x = (data.pthing[i].x * data.scale + data.offset_x * data.scale) / 10.0f;
		float y = (data.pthing[i].y * data.scale + data.offset_y * data.scale) / 10.0f;
		static int once = 1;

		if (data.pthing[i].type == THING_PLAYER1 && once)
		{
			player.position.x = x;
			player.position.y = y;
			player.angle = data.pthing[i].angle;
			once = 0;
		}

		switch (data.pthing[i].type)
		{
		case THING_PLAYER1:
			Rectangle(
				(x - 3.0f),
				(y - 3.0f),
				(x + 3.0f),
				(y + 3.0f),
				RGB(255, 0, 0));
			break;
		case THING_PLAYER2:
		case THING_PLAYER3:
		case THING_PLAYER4:
		case THING_DMSTART:
			Rectangle(
				(x - 3.0f),
				(y - 3.0f),
				(x + 3.0f),
				(y + 3.0f),
				RGB(128, 0, 0));
			break;
		case THING_IMP:
		case THING_SHOTGUNGUY:
		case THING_ZOMBIEMAN:
			Triangle(
				(x - 3.0f),
				(y - 3.0f),
				(x + 3.0f),
				(y + 3.0f),
				RGB(0, 0, 255));
			break;
		case THING_REDKEY:
			Rectangle(
				(x - 1.0f),
				(y - 3.0f),
				(x + 1.0f),
				(y + 3.0f),
				RGB(0, 0, 255));
			break;
		case THING_BLUEKEY:
			Rectangle(
				(x - 1.0f),
				(y - 3.0f),
				(x + 1.0f),
				(y + 3.0f),
				RGB(255, 0, 0));
			break;
		case THING_YELLOWKEY:
			Rectangle(
				(x - 1.0f),
				(y - 3.0f),
				(x + 1.0f),
				(y + 3.0f),
				RGB(0, 255, 255));
			break;
		case THING_BARREL:
			Rectangle(
				(x - 2.0f),
				(y - 3.0f),
				(x + 2.0f),
				(y + 3.0f),
				RGB(0, 255, 0));
			break;
			case THING_SHELLS:
			case THING_ROCKET:
			case THING_MEDIKIT:
			case THING_HEALTH:
			case THING_BOXBULLET:
			case THING_BOXSHELLS:
			case THING_BOXROCKET:
				Rectangle(
					(x - 2.0f),
					(y - 2.0f),
					(x + 2.0f),
					(y + 2.0f),
					RGB(0, 128, 0));
				break;


		default:
			Rectangle(
				(x - 3.0f),
				(y - 3.0f),
				(x + 3.0f),
				(y + 3.0f),
				RGB(0, 0, 0));
			break;
		}

	}


	draw_count = 0;
	RenderBSPNode(data, data.num_node - 1, 0);



	// draw player
	{
		float x = player.position.x;
		float y = player.position.y;


		Rectangle(
			(x - 6.0f),
			(y - 6.0f),
			(x + 6.0f),
			(y + 6.0f),
			RGB(255,0,0)
		);
	}

}

void draw_pixels(HDC hdc, HDC hdcMem, int width, int height, waddata_t &data)
{
	HBITMAP hBitmap, hOldBitmap;

	hBitmap = CreateCompatibleBitmap(hdc, width, height);
	SetBitmapBits(hBitmap, sizeof(int) * width * height, vram);
	hdcMem = CreateCompatibleDC(hdc);
	hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

	// This scaling is a little strange because Stretch maintains aspect ratios
	StretchBlt(hdc, 0, 0, xres, yres, hdcMem, 0, 0, width, height, SRCCOPY);
	SelectObject(hdcMem, hOldBitmap);
	DeleteDC(hdcMem);
	DeleteObject(hBitmap);
}


void draw_text(unsigned int *vram, unsigned int *tex, char *msg, int x, int y, int xres, int yres)
{
	int x_shift = 0;

	for (int c = 0; ; c++)
	{
		if (msg[c] == '\0')
			break;

		//256x256 font image, 16x16 fonts
		int row = msg[c] / 16;
		int col = msg[c] % 16;

		// have a valid character, copy pixels into vram x,y pos (assuming 16 pixel fonts)
		for (int j = 0; j < 16; j++)
		{
			int py = (row * 16 + j);

			for (int i = 0; i < 16; i++)
			{
				int px = (col * 16 + i);

				if ((x + i + x_shift) >= xres)
					break;

				// bitmap is upside down, so invert Y
				vram[(x + i + x_shift) + (y + j) * xres] = tex[((255 - py) * 256) + px];
			}
		}

		// move over to the right 16 pixels for next character
		x_shift += 16;
	}
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT	rect = { 0, 0, 1, 1 };
	static HDC	hdcMem;
	static int	old_style = 0;
	static int	new_style = WS_CHILD | WS_VISIBLE;
	HDC		hdc;
	PAINTSTRUCT	ps;
	static bool show_cursor = true;
	static POINT center;
	static int level = 0;
	static waddata_t data;
	static bool initialized = false;

	switch (message)
	{
	case WM_CREATE:
	{
		load_wad(&data, level);

		int checksize = 8;
		for (int y = 0; y < 512; y++)
		{
			for (int x = 0; x < 512; x++)
			{
				int total = floor(x / 512.0f * checksize) +
					floor(y / 512.0f * checksize);
				bool isEven = total % 2 == 0;

				if (isEven)
					tex[x + y * 512] = ~0;
				else
					tex[x + y * 512] = 0;

			}
		}

		int tw, th;
		read_bitmap("font.bmp", tw, th, &tex);




		data.scale = level_offsets[level].scale;
		data.offset_x = level_offsets[level].x;
		data.offset_y = level_offsets[level].y;
		data.max_depth = 100;
		data.max_draw = 250;
		data.draw_order = true;

		player.position.x = 50;
		player.position.y = 50;

		data.red = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
		data.blue = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));
		data.cross_brush = CreateHatchBrush(HS_CROSS, RGB(64, 64, 64));

		srand(time(0));
		for (int i = 0; i < 255; i++)
		{
			data.green[i] = CreatePen(PS_SOLID, 2, RGB(rand() % 255 - i, rand() % 255 - i, rand() % 255 - i));
		}
		for (int i = 0; i < 255; i++)
		{
			data.green_dash[i] = CreatePen(PS_DOT, 1, RGB(rand() % 255 - 1, rand() % 255 - i, rand() % 255 - i));
		}



		HMONITOR hmon;
		MONITORINFO mi = { sizeof(MONITORINFO) };

		hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		GetMonitorInfo(hmon, &mi);

		vram = (char *)malloc(xres * yres * 4 + 2048);
		SetTimer(hwnd, WM_TICK, 16, NULL);
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		int lx = LOWORD(lParam);
		int ly = HIWORD(lParam);

		// Transform Client->World
		//		player.position.x = lx * 10.0f / data.scale - data.offset_x;
		//		player.position.y = ly * 10.0f / data.scale - data.offset_y;
	}
	return 0;
	case WM_TIMER:
	{
		if (initialized)
		{
			Draw(data);
			InvalidateRect(hwnd, 0, TRUE);
		}
		return 0;
	}
	case WM_ERASEBKGND:
		return TRUE;
	case WM_SIZE:
		xres = LOWORD(lParam);
		yres = HIWORD(lParam);
		rect.right = xres;
		rect.bottom = yres;

		center.x = xres / 2;
		center.y = yres / 2;
		if (vram != NULL)
		{
			free((void *)vram);
			vram = NULL;
		}
		vram = (char *)malloc(xres * yres * 4);

		if (initialized == false)
		{
			initialized = true;
		}

		return 0;
	case WM_KEYDOWN:
	{
		bool pressed = (message == WM_KEYDOWN) ? true : false;


		switch (wParam)
		{
		case VK_UP:
			player.position.x += cos(player.angle) * 2.0f;
			player.position.y += sin(player.angle) * 2.0f;
			break;
		case VK_DOWN:
			player.position.x -= cos(player.angle) * 2.0f;
			player.position.y -= sin(player.angle) * 2.0f;
			break;
		case VK_RIGHT:
			player.angle += 0.1f;
			break;
		case VK_LEFT:
			player.angle -= 0.1f;
			break;
		case VK_SPACE:
			solid = !solid;
			break;
		case VK_CONTROL:
			flag = !flag;
			break;



		case VK_ESCAPE:
			if (pressed)
			{
				old_style = SetWindowLong(hwnd, GWL_STYLE, new_style);
				new_style = old_style;
				SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, xres, yres, 0);
			}
			break;

		case VK_NEXT:
			data.scale += 0.5f;
			break;
		case VK_PRIOR:
			data.scale -= 0.5f;
			break;
		case VK_ADD:
			level++;
			if (level >= MAX_LEVEL)
				level = MAX_LEVEL - 1;

			load_wad(&data, level);
			data.scale = level_offsets[level].scale;
			data.offset_x = level_offsets[level].x;
			data.offset_y = level_offsets[level].y;

			break;
		case VK_SUBTRACT:
			level--;
			if (level < 0)
				level = 0;

			load_wad(&data, level);
			data.scale = level_offsets[level].scale;
			data.offset_x = level_offsets[level].x;
			data.offset_y = level_offsets[level].y;
			break;

		case VK_F1:
			data.draw_order = false;
			if (data.max_depth > 0)
				data.max_depth--;
			break;
		case VK_F2:
			data.draw_order = false;
			data.max_depth++;
			break;
		case VK_F3:
			data.draw_order = true;
			data.max_depth = 250;
			if (data.max_draw > 0)
				data.max_draw--;
			break;
		case VK_F4:
			data.draw_order = true;
			data.max_depth = 250;
			data.max_draw++;
			break;

		}

		return 0;
	}
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		draw_pixels(hdc, hdcMem, xres, yres, data);
		EndPaint(hwnd, &ps);
		return 0;


	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = TEXT("DoomView3D");
	HWND         hwnd;
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(NULL_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("Program requires Windows NT!"), szAppName, MB_ICONERROR);
		return 0;
	}


	hwnd = CreateWindow(szAppName, TEXT("DoomView3D"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}


