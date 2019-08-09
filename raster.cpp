#include "include.h"
#include <math.h>

inline int imin(int x, int y)
{
	return y ^ ((x ^ y) & -(x < y));
}

inline int imax(int x, int y)
{
	return y ^ ((x ^ y) & -(x > y));
}

inline void iclamp(int &a, int mi, int ma)
{
	a = imin(imax(a, mi), ma);
}


void draw_line(int *pixels, int width, int height, int x1, int y1, int x2, int y2, int color)
{
	int i;
	int	x, y;
	float slope;
	int deltax;
	int deltay;


	iclamp(x1, 0, width - 1);
	iclamp(y1, 0, height - 1);
	iclamp(x2, 0, width - 1);
	iclamp(y2, 0, height - 1);

	deltax = fabs(x2 - x1);
	deltay = fabs(y2 - y1);

	if (deltax == 0 && deltay == 0)
		return;

	//We want x to always move right
	if (x2 - x1 < 0)
	{
		draw_line(pixels, width, height, x2, y2, x1, y1, color);
		return;
	}

	if (x2 - x1 != 0)
	{
		slope = (float)(y2 - y1) / (x2 - x1);
	}
	else
	{
		slope = 1000.0f;
	}

	if (slope > 1.0f)
	{
		//slope is greater than one, flip axis, redo everything
		if (y2 - y1 < 0)
		{
			draw_line(pixels, width, height, x2, y2, x1, y1, color);
			return;
		}

		slope = 1.0f / slope;

		for (i = 0; i <= deltay; i++)
		{
			y = y1 + i;
			x = x1 + slope * i;

			pixels[x + y * width] = color;
		}
	}
	else if (slope < -1.0f)
	{
		if (y2 - y1 < 0)
		{
			int temp;

			temp = y2;
			y2 = y1;
			y1 = temp;
			temp = x2;
			x2 = x1;
			x1 = temp;
		}

		slope = 1.0f / slope;

		for (i = 0; i <= deltay; i++)
		{
			y = y1 + i;
			x = x1 + slope * i;

			pixels[x + y * width] = color;
		}
	}
	else
	{
		for (i = 0; i <= deltax; i++)
		{
			x = x1 + i;
			y = y1 + slope * i;

			pixels[x + y * width] = color;
		}
	}
}

void draw_rect(int *pixels, int width, int height, float angle, int w, int l, int x, int y, int color)
{
	float	corner[4][2];
	float	corner_rotated[4][2];
	float	sn, cs;
	int		i;

	sn = sinf(angle);
	cs = cosf(angle);

	corner[0][0] = (float)(-w / 2);
	corner[0][1] = (float)(l / 2);

	corner[1][0] = (float)(w / 2);
	corner[1][1] = (float)(l / 2);

	corner[2][0] = (float)(w / 2);
	corner[2][1] = (float)(-l / 2);

	corner[3][0] = (float)(-w / 2);
	corner[3][1] = (float)(-l / 2);

	for (i = 0; i <= 3; i++)
	{
		corner_rotated[i][0] = cs * corner[i][0] - sn * corner[i][1];
		corner_rotated[i][1] = sn * corner[i][0] + cs * corner[i][1];
		corner[i][0] = corner_rotated[i][0];
		corner[i][1] = corner_rotated[i][1];
	}

	for (i = 0; i <= 3; i++)
	{
		corner[i][0] += x;
		corner[i][1] += y;
	}

	draw_line(pixels, width, height, (int)corner[0][0], (int)corner[0][1], (int)corner[1][0], (int)corner[1][1], color);
	draw_line(pixels, width, height, (int)corner[1][0], (int)corner[1][1], (int)corner[2][0], (int)corner[2][1], color);
	draw_line(pixels, width, height, (int)corner[2][0], (int)corner[2][1], (int)corner[3][0], (int)corner[3][1], color);
	draw_line(pixels, width, height, (int)corner[3][0], (int)corner[3][1], (int)corner[0][0], (int)corner[0][1], color);
}