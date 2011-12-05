//----------------------------------------------------------------------------
//	complexmaths.h
//  Copyright 2005, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _COMPLEXMATHS_H_
#define _COMPLEXMATHS_H_

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int LiangBarskyClipT(
	float numerator,
	float denominator, 
	float* m0,
	float* m1)
{
	if (denominator == 0.0f)
	{
		return (numerator <= 0.0f);
	}
	float t = numerator / denominator;

	if (denominator > 0.0f) 
	{
	    if (t > *m1)
		{
			return 0;
		}
		if (t > *m0) 
		{
			*m0 = t;
		}
	} 
	else
	{
		if (t < *m0)
		{
			return 0;
		}
	    if (t < *m1)
		{
			*m1 = t;
		}
	}

	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL ClipLine(
	float& x1,
	float& y1,
	float& x2,
	float& y2,
	float min_x,
	float min_y,
	float max_x,
	float max_y)
{
	if (PointInside(x1, y1, min_x, min_y, max_x, max_y) && PointInside(x2, y2, min_x, min_y, max_x, max_y))
	{
		return TRUE;
	}
	float m0 = 0.0f;
	float m1 = 1.0f;
	float dx = x2 - x1;
	float dy = y2 - y1;

	if (LiangBarskyClipT(min_x - x1, dx, &m0, &m1) &&
		LiangBarskyClipT(x1 - max_x, -dx, &m0, &m1) &&
		LiangBarskyClipT(min_y - y1,  dy, &m0, &m1) &&
		LiangBarskyClipT(y1 - max_y, -dy, &m0, &m1))
    {
		if (m1 < 1.0f)
		{
			x2 = x1 + m1 * dx;
			y2 = y1 + m1 * dy;
		}
		if (m0 > 0)
		{
			x1 += m0 * dx;
			y1 += m0 * dy;
		}
		return TRUE;
    }
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int HLineIntersect2D(
	const float hx1,
	const float hx2,
	const float hy,
	const float lx1,
	const float ly1,
	const float lx2,
	const float ly2,
	float* pix = NULL,
	float* piy = NULL)
{
	ASSERT(hx2 > hx1);
	float dy1 = hy - ly1;
	float dy2 = hy - ly2;
	// same side of hline (or colinear)
	if ((dy1 < EPSILON && dy2 < EPSILON) ||
		(dy1 > -EPSILON && dy2 > -EPSILON))
	{
		return 0;
	}
	// vertical line
	if (ABS(lx1 - lx2) < EPSILON)
	{
		if (lx1 >= hx1 && lx1 <= hx2)
		{
			if (pix)
			{
				*pix = lx1;
				*piy = hy;
			}
			return 1;
		}
		return 0;
	}
	float ix = lx1 + (lx2-lx1) * (dy1/(ly2-ly1));
	if (ix < hx1 - EPSILON || ix > hx2 + EPSILON)
	{
		return 0;
	}
	if (pix)
	{
		*pix = ix;
		*piy = hy;
	}
	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int VLineIntersect2D(
	const float vy1,
	const float vy2,
	const float vx,
	const float lx1,
	const float ly1,
	const float lx2,
	const float ly2,
	float* pix = NULL,
	float* piy = NULL)
{
	ASSERT(vy2 > vy1);
	float dx1 = vx - lx1;
	float dx2 = vx - lx2;
	// same side of hline (or colinear)
	if ((dx1 < EPSILON && dx2 < EPSILON) ||
		(dx1 > -EPSILON && dx2 > -EPSILON))
	{
		return 0;
	}
	// horizontal line
	if (ABS(ly1 - ly2) < EPSILON)
	{
		if (ly1 >= vy1 && ly1 <= vy2)
		{
			if (pix)
			{
				*pix = vx;
				*piy = ly1;
			}
			return 1;
		}
		return 0;
	}
	float iy = ly1 + (ly2-ly1) * (dx1/(lx2-lx1));
	if (iy < vy1 - EPSILON || iy > vy2 + EPSILON)
	{
		return 0;
	}
	if (pix)
	{
		*pix = vx;
		*piy = iy;
	}
	return 1;
}


#endif // _COMPLEXMATHS_H_
