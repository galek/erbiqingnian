//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include "stdafx.h"
#include <windows.h>
#include <limits.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "RectOrganizer.h"
#include "list.h"
#include "array.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define		XAXIS								0
#define		YAXIS								1
#define		ZAXIS								2
#define		ONEAXIS								1
#define		TWOAXIS								2
#define		THREEAXIS							3

#define		RECT_MGMT_BUCKET_SIZE				256
#define		RECT_MGMT_FRAME_COUNT				20

#define		FONT_HEIGHT							16		// font height in world coordinates
#define		MAX_VISIBLE_DISTANCE_SQUARED		16000	// don't display label if it's > this distance from item
#define		INVALID_ID							((DWORD)-1)



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float distsq(
	float dx,
	float dy)
{
	return (float)(dx * dx + dy * dy);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T, unsigned int DIMENSIONS>
struct TUPLE
{
	T			value[DIMENSIONS];

	T & operator[](
		unsigned int ii)
	{
#if ISVERSION(DEVELOPMENT)
		ASSERT(ii < DIMENSIONS);
#endif
		return value[ii];
	}

	const T & operator[](
		unsigned int ii) const
	{
#if ISVERSION(DEVELOPMENT)
		ASSERT(ii < DIMENSIONS);
#endif
		return value[ii];
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct Point2 : TUPLE<float, 2>
{
	Point2(
		void)
	{
	}

	Point2(
		float x,
		float y)
	{
		value[XAXIS] = x;
		value[YAXIS] = y;
	}

	friend Point2 operator+(
		const Point2 & a,
		const Point2 & b)
	{
		return Point2(a.value[XAXIS] + b.value[XAXIS], a.value[YAXIS] + b.value[YAXIS]);
	}

	Point2 & operator+=(
		const Point2 & b)
	{
		value[XAXIS] += b.value[XAXIS];
		value[YAXIS] += b.value[YAXIS];
		return *this;
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct Rect2
{
	Point2				m_center;
	TUPLE<float, 2>		m_radius;

	void FromLTWH(
		float left,
		float top,
		float width,
		float height)
	{
		m_radius[XAXIS] = width / 2.0f;
		m_radius[YAXIS] = height / 2.0f;
		m_center[XAXIS] = left + m_radius[XAXIS];
		m_center[YAXIS] = top + m_radius[YAXIS];
	}

	void FromLTRB(
		float left,
		float top,
		float right,
		float bottom)
	{
		FromLTWH(left, top, right - left, bottom - top);
	}

	void Expand(
		float size)
	{
		m_radius[XAXIS] += size;
		m_radius[YAXIS] += size;
	}

	float Left(
		void) const
	{
		return m_center[XAXIS] - m_radius[XAXIS];
	}

	float Right(
		void) const
	{
		return m_center[XAXIS] + m_radius[XAXIS];
	}

	float Top(
		void) const
	{
		return m_center[YAXIS] - m_radius[YAXIS];
	}

	float Bottom(
		void) const
	{
		return m_center[YAXIS] + m_radius[YAXIS];
	}

	BOOL Overlaps(
		const Rect2 & rect)
	{
		if (Right() <= rect.Left())
		{
			return FALSE;
		}
		if (Left() >= rect.Right())
		{
			return FALSE;
		}
		if (Bottom() <= rect.Top())
		{
			return FALSE;
		}
		if (Top() >= rect.Bottom())
		{
			return FALSE;
		}
		return TRUE;
	}

	BOOL Encompasses(
		const Rect2 & rect)
	{
		if (rect.Right() > Right())
		{
			return FALSE;
		}
		if (rect.Left() < Left())
		{
			return FALSE;
		}
		if (rect.Bottom() > Bottom())
		{
			return FALSE;
		}
		if (rect.Top() < rect.Top())
		{
			return FALSE;
		}
		return TRUE;
	}

	float Area(
		void) const
	{
		return m_radius[XAXIS] * m_radius[YAXIS] * 4;
	}

	float GetDistanceSquared(
		const Point2 & point) const
	{
		float maxInsideDistSquared = distsq(m_radius[XAXIS], m_radius[YAXIS]);
		if (point[XAXIS] < m_center[XAXIS] - m_radius[XAXIS])
		{
			if (point[YAXIS] < m_center[YAXIS] - m_radius[YAXIS])
			{
				return maxInsideDistSquared + distsq(m_center[XAXIS] - m_radius[XAXIS] - point[XAXIS], m_center[YAXIS] - m_radius[YAXIS] - point[YAXIS]);
			}
			else if (point[YAXIS] > m_center[YAXIS] + m_radius[YAXIS])
			{
				return maxInsideDistSquared + distsq(m_center[XAXIS] - m_radius[XAXIS] - point[XAXIS], m_center[YAXIS] + m_radius[YAXIS] - point[YAXIS]);
			}
			else
			{
				return maxInsideDistSquared + distsq(m_center[XAXIS] - m_radius[XAXIS] - point[XAXIS], 0);
			}
		}
		else if (point[XAXIS] > m_center[XAXIS] + m_radius[XAXIS])
		{
			if (point[YAXIS] < m_center[YAXIS] - m_radius[YAXIS])
			{
				return maxInsideDistSquared + distsq(m_center[XAXIS] + m_radius[XAXIS] - point[XAXIS], m_center[YAXIS] - m_radius[YAXIS] - point[YAXIS]);
			}
			else if (point[YAXIS] > m_center[YAXIS] + m_radius[YAXIS])
			{
				return maxInsideDistSquared + distsq(m_center[XAXIS] + m_radius[XAXIS] - point[XAXIS], m_center[YAXIS] + m_radius[YAXIS] - point[YAXIS]);
			}
			else
			{
				return maxInsideDistSquared + distsq(m_center[XAXIS] + m_radius[XAXIS] - point[XAXIS], 0);
			}
		}
		else
		{
			if (point[YAXIS] < m_center[YAXIS] - m_radius[YAXIS])
			{
				return maxInsideDistSquared + distsq(0, m_center[YAXIS] - m_radius[YAXIS] - point[YAXIS]);
			}
			else if (point[YAXIS] > m_center[YAXIS] + m_radius[YAXIS])
			{
				return maxInsideDistSquared + distsq(0, m_center[YAXIS] + m_radius[YAXIS] - point[YAXIS]);
			}
			else
			{
				return distsq(m_center[XAXIS] - point[XAXIS], m_center[YAXIS] - point[YAXIS]);
			}		
		}
	}

	float GetDistanceSquared(
		const Rect2 & rect) const
	{
		if (m_center[XAXIS] + m_radius[XAXIS] < rect.m_center[XAXIS] - rect.m_radius[XAXIS])
		{
			if (m_center[YAXIS] - m_radius[YAXIS] > rect.m_center[YAXIS] + rect.m_radius[YAXIS])
			{
				return distsq((rect.m_center[XAXIS] - rect.m_radius[XAXIS]) - (m_center[XAXIS] + m_radius[XAXIS]), 
					(m_center[YAXIS] - m_radius[YAXIS]) - (rect.m_center[YAXIS] + rect.m_radius[YAXIS]));
			}
			else if (m_center[YAXIS] + m_radius[YAXIS] < rect.m_center[YAXIS] - rect.m_radius[YAXIS])
			{
				return distsq((rect.m_center[XAXIS] - rect.m_radius[XAXIS]) - (m_center[XAXIS] + m_radius[XAXIS]), 
					(rect.m_center[YAXIS] - rect.m_radius[YAXIS]) - (m_center[YAXIS] + m_radius[YAXIS]));
			}
			else
			{
				return distsq((rect.m_center[XAXIS] - rect.m_radius[XAXIS]) - (m_center[XAXIS] + m_radius[XAXIS]), 0);
			}
		}
		else if (m_center[XAXIS] - m_radius[XAXIS] > rect.m_center[XAXIS] + rect.m_radius[XAXIS])
		{
			if (m_center[YAXIS] - m_radius[YAXIS] > rect.m_center[YAXIS] + rect.m_radius[YAXIS])
			{
				return distsq((m_center[XAXIS] - m_radius[XAXIS]) - (rect.m_center[XAXIS] + rect.m_radius[XAXIS]), 
					(m_center[YAXIS] - m_radius[YAXIS]) - (rect.m_center[YAXIS] + rect.m_radius[YAXIS]));
			}
			else if (m_center[YAXIS] + m_radius[YAXIS] < rect.m_center[YAXIS] - rect.m_radius[YAXIS])
			{
				return distsq((m_center[XAXIS] - m_radius[XAXIS]) - (rect.m_center[XAXIS] + rect.m_radius[XAXIS]), 
					(rect.m_center[YAXIS] - rect.m_radius[YAXIS]) - (m_center[YAXIS] + m_radius[YAXIS]));
			}
			else
			{
				return distsq((m_center[XAXIS] - m_radius[XAXIS]) - (rect.m_center[XAXIS] + rect.m_radius[XAXIS]), 0);
			}
		}
		else
		{
			if (m_center[YAXIS] - m_radius[YAXIS] > rect.m_center[YAXIS] + rect.m_radius[YAXIS])
			{
				return distsq(0, (m_center[YAXIS] - m_radius[YAXIS]) - (rect.m_center[YAXIS] + rect.m_radius[YAXIS]));
			}
			else if (m_center[YAXIS] + m_radius[YAXIS] < rect.m_center[YAXIS] - rect.m_radius[YAXIS])
			{
				return distsq(0, (rect.m_center[YAXIS] - rect.m_radius[YAXIS]) - (m_center[YAXIS] + m_radius[YAXIS]));
			}
			else
			{
				return 0;
			}
		}
	}
};



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct RECT_ENTRY
{
	DWORD				m_index;			// index in label system
	Point2				m_idealPos;			// position in "world" coordinates
	Rect2				m_textRect;			// rect in "world" coordinates
	BOOL				m_visible;			// draw the label?
	RECT_ENTRY *		m_prev;
	RECT_ENTRY *		m_next;
	unsigned int		m_proximal;			// nearby items

	// Stored pointers to the original data
	float *				m_pfTop;
	float *				m_pfLeft;
	float *				m_pfRight;
	float *				m_pfBottom;

	unsigned int GetScore(
		const RECT_ENTRY * label) const;

	float GetDistanceSquared(
		const Point2 & point) const;

	BOOL CanMoveInDir(
		Point2 dir, 
		Rect2 rectBounding) const
	{
		Rect2 temprect = m_textRect;
		temprect.m_center += dir;

		return (rectBounding.Encompasses(temprect));
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class RECT_ORGANIZE_SYSTEM
{
private:
	BUCKET_ARRAY<RECT_ENTRY, RECT_MGMT_BUCKET_SIZE>			m_rectArray;
	unsigned int											m_rectCount;
	SINGLE_LIST<RECT_ENTRY>									m_garbage;
	DOUBLE_LIST<RECT_ENTRY>									m_activeList;

	unsigned int											m_frame;			// internal frame counter for how many times Compute() has been called

	BOOL													m_bHideOverlaps;	// whether to hide overlapping rects
	BOOL													m_bRestrictToFrame;	// keep the rects inside m_rectBounding
	Rect2													m_rectBounding;
	BOOL													m_bUseProximal;

	SIMPLE_DYNAMIC_ARRAY<unsigned int>						m_proximalList;
	unsigned int											m_proximalCount;

	unsigned int AddProximal(
		unsigned int index);

	void ComputeProximal(
		RECT_ENTRY * label);

	void RecomputeProximal(
		void);

	void SetupFrame(
		const Rect2 & screenRect);

	void HideOverlappingLabels(
		void);

	unsigned int GetScore(
		RECT_ENTRY * label);

	unsigned int GetScoreNoProximal(
		RECT_ENTRY * label);

	BOOL MakeMove(
		RECT_ENTRY * label,
		unsigned int & bestscore);

	RECT_ENTRY * GetNewRect(
		void);

	void UpdateRealPositions(
		void );

	DWORD Add(
		const Point2 & idealPos,
		const Rect2 & textRect);

	BOOL Compute(
		const Rect2 * screenRect,
		unsigned int maxTries);

	void DebugTrace(
		const Rect2 & screenRect) const;

public:
	void Init(
		void)
	{
		m_rectArray.Init(NULL);
		m_rectCount = 0;
		m_garbage.Init();
		m_activeList.Init();
		m_frame = 0;
		m_bUseProximal = TRUE;

		ArrayInit(m_proximalList, NULL, 10);
		m_proximalCount = 0;
	}

	void Free(
		void)
	{
		m_proximalList.Destroy();
		m_proximalCount = 0;

		m_garbage.Free();
		m_activeList.Free();
		m_rectArray.Free();
		m_rectCount = 0;
		m_frame = 0;
	}

	void Clear(
		void)
	{
		RECT_ENTRY * cur = m_activeList.Head();
		while (cur)
		{
			RECT_ENTRY * next = cur->m_next;
			m_activeList.Remove(cur);
			m_garbage.Add(cur);
			cur = next;
		}

		ArrayClear(m_proximalList);
		m_proximalCount = 0;

		m_frame = 0;
	}

	void SetHideOverlaps(BOOL bHide)	
	{
		m_bHideOverlaps = bHide;
	}

	void SetUseProximal(BOOL bUse)
	{
		m_bUseProximal = bUse;
	}

	DWORD Add(
		float fIdealX,
		float fIdealY,
		float *pfLeft,
		float *pfTop,
		float *pfRight,
		float *pfBottom);

	DWORD Add(
		float fIdealX,
		float fIdealY,
		float *pfLeft,
		float *pfTop,
		float fWidth,
		float fHeight);

	DWORD Add(
		float fIdealX,
		float fIdealY,
		float fLeft,
		float fTop,
		float fWidth,
		float fHeight);

	void Remove(
		DWORD idLabel);

	BOOL Compute(
		float fFrameLeft,
		float fFrameTop,
		float fFrameRight,
		float fFrameBottom,
		unsigned int maxTries);

	BOOL Compute(
		unsigned int maxTries);
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
RECT_ENTRY * RECT_ORGANIZE_SYSTEM::GetNewRect(
	void)
{
	RECT_ENTRY * rect = m_garbage.Remove();
	if (!rect)
	{
		if (m_rectCount % RECT_MGMT_BUCKET_SIZE == 0)
		{
			ArrayResize(m_rectArray, m_rectCount + 1);
		}
		rect = &m_rectArray[m_rectCount];
		rect->m_index = m_rectCount;
		++m_rectCount;
	}

	rect->m_visible = FALSE;
	rect->m_proximal = INVALID_ID;
	rect->m_pfLeft = NULL;
	rect->m_pfLeft = NULL;
	rect->m_pfRight = NULL;
	rect->m_pfBottom = NULL;
	rect->m_prev = NULL;
	rect->m_next = NULL;

	m_activeList.Add(rect);

	m_frame = 0;		// reset computation

	return rect;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD RECT_ORGANIZE_SYSTEM::Add(
	const Point2 & idealPos,
	const Rect2 & textRect)
{
	RECT_ENTRY * rect = GetNewRect();
	rect->m_idealPos = idealPos;
	rect->m_textRect = textRect;

	return rect->m_index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD RECT_ORGANIZE_SYSTEM::Add(
	float fIdealX,
	float fIdealY,
	float *pfLeft,
	float *pfTop,
	float *pfRight,
	float *pfBottom)
{
	ASSERT_RETINVALID(pfLeft && pfTop && pfRight && pfBottom);

	RECT_ENTRY * rect = GetNewRect();
	rect->m_idealPos.value[XAXIS] = fIdealX;
	rect->m_idealPos.value[YAXIS] = fIdealY;
	rect->m_textRect.FromLTRB(*pfLeft, *pfTop, *pfRight, *pfBottom);

	rect->m_pfLeft = pfLeft;
	rect->m_pfTop = pfTop;
	rect->m_pfRight = pfRight;
	rect->m_pfBottom = pfBottom;

	return rect->m_index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD RECT_ORGANIZE_SYSTEM::Add(
	float fIdealX,
	float fIdealY,
	float *pfLeft,
	float *pfTop,
	float fWidth,
	float fHeight)
{
	RECT_ENTRY * rect = GetNewRect();
	rect->m_idealPos.value[XAXIS] = fIdealX;
	rect->m_idealPos.value[YAXIS] = fIdealY;
	rect->m_textRect.FromLTWH(*pfLeft, *pfTop, fWidth, fHeight);

	rect->m_pfLeft = pfLeft;
	rect->m_pfTop = pfTop;

	return rect->m_index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD RECT_ORGANIZE_SYSTEM::Add(
	float fIdealX,
	float fIdealY,
	float fLeft,
	float fTop,
	float fWidth,
	float fHeight)
{
	RECT_ENTRY * rect = GetNewRect();
	rect->m_idealPos.value[XAXIS] = fIdealX;
	rect->m_idealPos.value[YAXIS] = fIdealY;
	rect->m_textRect.m_radius[XAXIS] = (fWidth) / 2.0f;
	rect->m_textRect.m_radius[YAXIS] = (fHeight) / 2.0f;
	rect->m_textRect.m_center[XAXIS] = fLeft + rect->m_textRect.m_radius[XAXIS];
	rect->m_textRect.m_center[YAXIS] = fTop + rect->m_textRect.m_radius[YAXIS];

	return rect->m_index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RECT_ORGANIZE_SYSTEM::Remove(
	DWORD idLabel)
{
	RECT_ENTRY * label = &m_rectArray[idLabel];
	m_activeList.Remove(label);
	m_garbage.Add(label);
	m_frame = 0;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int RECT_ENTRY::GetScore(
	const RECT_ENTRY * label) const
{
	float xdiff1 = (m_textRect.m_center[XAXIS] + m_textRect.m_radius[XAXIS]) - 
		(label->m_textRect.m_center[XAXIS] - label->m_textRect.m_radius[XAXIS]);
	if (xdiff1 <= 0)
	{
		return 0;
	}
	float xdiff2 = (label->m_textRect.m_center[XAXIS] + label->m_textRect.m_radius[XAXIS]) - 
		(m_textRect.m_center[XAXIS] - m_textRect.m_radius[XAXIS]);
	if (xdiff2 <= 0)
	{
		return 0;
	}
	float ydiff1 = (m_textRect.m_center[YAXIS] + m_textRect.m_radius[YAXIS]) - 
		(label->m_textRect.m_center[YAXIS] - label->m_textRect.m_radius[YAXIS]);
	if (ydiff1 <= 0)
	{
		return 0;
	}
	float ydiff2 = (label->m_textRect.m_center[YAXIS] + label->m_textRect.m_radius[YAXIS]) - 
		(m_textRect.m_center[YAXIS] - m_textRect.m_radius[YAXIS]);
	if (ydiff2 <= 0)
	{
		return 0;
	}
	float x1 = MAX(m_textRect.m_center[XAXIS] - m_textRect.m_radius[XAXIS],
		label->m_textRect.m_center[XAXIS] - label->m_textRect.m_radius[XAXIS]);
	float x2 = MIN(m_textRect.m_center[XAXIS] + m_textRect.m_radius[XAXIS],
		label->m_textRect.m_center[XAXIS] + label->m_textRect.m_radius[XAXIS]);
	float y1 = MAX(m_textRect.m_center[YAXIS] - m_textRect.m_radius[YAXIS],
		label->m_textRect.m_center[YAXIS] - label->m_textRect.m_radius[YAXIS]);
	float y2 = MIN(m_textRect.m_center[YAXIS] + m_textRect.m_radius[YAXIS],
		label->m_textRect.m_center[YAXIS] + label->m_textRect.m_radius[YAXIS]);
	float area = (x2 - x1) * (y2- y1);
	return (unsigned int)MAX(0.0f, area);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int RECT_ORGANIZE_SYSTEM::GetScore(
	RECT_ENTRY * label)
{
	unsigned int score = 0;

	if (m_bUseProximal)
	{
		unsigned int prox = label->m_proximal;
		if (prox == INVALID_ID)
		{
			return 0;
		}
		while (m_proximalList[prox] != INVALID_ID)
		{
			RECT_ENTRY * curr = &m_rectArray[m_proximalList[prox]];
			if (curr->m_visible)
			{
				score += label->GetScore(curr);
			}
			++prox;
		}
	}
	else
	{
		label->m_visible = FALSE;

		for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
		{
			if (!curr->m_visible)
			{
				continue;
			}
			score += label->GetScore(curr);
		}

		label->m_visible = TRUE;
	}

	return score;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int RECT_ORGANIZE_SYSTEM::GetScoreNoProximal(
	RECT_ENTRY * label)
{
	unsigned int score = 0;

	label->m_visible = FALSE;

	for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
	{
		if (!curr->m_visible)
		{
			continue;
		}
		score += label->GetScore(curr);
	}

	label->m_visible = TRUE;

	return score;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float RECT_ENTRY::GetDistanceSquared(
	const Point2 & point) const
{
	float maxInsideDistSquared = distsq(m_textRect.m_radius[XAXIS], m_textRect.m_radius[YAXIS]);
	if (m_idealPos[XAXIS] < point[XAXIS] - m_textRect.m_radius[XAXIS])
	{
		if (m_idealPos[YAXIS] < point[YAXIS] - m_textRect.m_radius[YAXIS])
		{
			return maxInsideDistSquared + distsq(point[XAXIS] - m_textRect.m_radius[XAXIS] - m_idealPos[XAXIS], point[YAXIS] - m_textRect.m_radius[YAXIS] - m_idealPos[YAXIS]);
		}
		else if (m_idealPos[YAXIS] > point[YAXIS] + m_textRect.m_radius[YAXIS])
		{
			return maxInsideDistSquared + distsq(point[XAXIS] - m_textRect.m_radius[XAXIS] - m_idealPos[XAXIS], point[YAXIS] + m_textRect.m_radius[YAXIS] - m_idealPos[YAXIS]);
		}
		else
		{
			return maxInsideDistSquared + distsq(point[XAXIS] - m_textRect.m_radius[XAXIS] - m_idealPos[XAXIS], 0);
		}
	}
	else if (m_idealPos[XAXIS] > point[XAXIS] + m_textRect.m_radius[XAXIS])
	{
		if (m_idealPos[YAXIS] < point[YAXIS] - m_textRect.m_radius[YAXIS])
		{
			return maxInsideDistSquared + distsq(point[XAXIS] + m_textRect.m_radius[XAXIS] - m_idealPos[XAXIS], point[YAXIS] - m_textRect.m_radius[YAXIS] - m_idealPos[YAXIS]);
		}
		else if (m_idealPos[YAXIS] > point[YAXIS] + m_textRect.m_radius[YAXIS])
		{
			return maxInsideDistSquared + distsq(point[XAXIS] + m_textRect.m_radius[XAXIS] - m_idealPos[XAXIS], point[YAXIS] + m_textRect.m_radius[YAXIS] - m_idealPos[YAXIS]);
		}
		else
		{
			return maxInsideDistSquared + distsq(point[XAXIS] + m_textRect.m_radius[XAXIS] - m_idealPos[XAXIS], 0);
		}
	}
	else
	{
		if (m_idealPos[YAXIS] < point[YAXIS] - m_textRect.m_radius[YAXIS])
		{
			return maxInsideDistSquared + distsq(0, point[YAXIS] - m_textRect.m_radius[YAXIS] - m_idealPos[YAXIS]);
		}
		else if (m_idealPos[YAXIS] > point[YAXIS] + m_textRect.m_radius[YAXIS])
		{
			return maxInsideDistSquared + distsq(0, point[YAXIS] + m_textRect.m_radius[YAXIS] - m_idealPos[YAXIS]);
		}
		else
		{
			return distsq(point[XAXIS] - m_idealPos[XAXIS], point[YAXIS] - m_idealPos[YAXIS]);
		}		
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RECT_ORGANIZE_SYSTEM::MakeMove(
	RECT_ENTRY * rect,
	unsigned int & bestscore)
{
	static const Point2 dirArray[] =
	{
		Point2(0, 0), Point2(0, -1), Point2(1, 0), Point2(0, 1), Point2(-1, 0),
	};

	unsigned int bestdir = 0;
	bestscore = GetScore(rect);
	Point2 curpos = rect->m_textRect.m_center;

	for (unsigned int dir = 1; dir < arrsize(dirArray); ++dir)
	{
		if (!m_bRestrictToFrame || rect->CanMoveInDir(dirArray[dir], m_rectBounding))
		{
			rect->m_textRect.m_center = curpos + dirArray[dir];
			unsigned int ss = GetScore(rect);
			if (ss < bestscore)
			{
				bestscore = ss;
				bestdir = dir;
			}
			else if (ss == bestscore)
			{
				if (rect->GetDistanceSquared(curpos + dirArray[dir]) < 
					rect->GetDistanceSquared(curpos + dirArray[bestdir]))
				{
					bestscore = ss;
					bestdir = dir;
				}
			}
		}
	}

	rect->m_textRect.m_center = curpos + dirArray[bestdir];
	return (bestdir != 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int RECT_ORGANIZE_SYSTEM::AddProximal(
	unsigned int index)
{
	unsigned int proxnode = m_proximalCount;				// TODO: just replace	m_proximalCount with m_proximalList.Count()
	ArrayAddItem(m_proximalList, index);
	m_proximalCount = m_proximalList.Count();
	//++m_proximalCount;
	//if (m_proximalCount >= m_proximalList.GetSize())
	//{
	//	m_proximalList.Resize(NULL, m_proximalCount);
	//}
	//m_proximalList[proxnode] = index;
	return proxnode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RECT_ORGANIZE_SYSTEM::ComputeProximal(
	RECT_ENTRY * label)
{
	label->m_visible = FALSE;
	label->m_proximal = INVALID_ID;

	for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
	{
		if (!curr->m_visible)
		{
			continue;
		}
		if (label->m_textRect.GetDistanceSquared(curr->m_textRect) > (RECT_MGMT_FRAME_COUNT * 2) * (RECT_MGMT_FRAME_COUNT * 2))
		{
			continue;
		}
		unsigned int prox = AddProximal(curr->m_index);
		if (label->m_proximal == INVALID_ID)
		{
			label->m_proximal = prox;
		}
	}

	if (label->m_proximal != INVALID_ID)
	{
		AddProximal(INVALID_ID);
	}

	label->m_visible = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RECT_ORGANIZE_SYSTEM::RecomputeProximal(
	void)
{
	if (!m_bUseProximal)
	{
		return;
	}

	for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
	{
		if (curr->m_visible)
		{
			ComputeProximal(curr);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RECT_ORGANIZE_SYSTEM::SetupFrame(
	const Rect2 & screenRect)
{
	Rect2 expandedScreenRect = screenRect;
//	expandedScreenRect.Expand(RECT_MGMT_FRAME_COUNT);	// BSP -huh?

	for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
	{
		curr->m_visible = curr->m_textRect.Overlaps(expandedScreenRect);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RECT_ORGANIZE_SYSTEM::HideOverlappingLabels(
	void)
{
	for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
	{
		if (!curr->m_visible)
		{
			continue;
		}
		if (GetScore(curr) != 0)
		{
			curr->m_visible = FALSE;
		}
		//else if (curr->m_textRect.GetDistanceSquared(curr->m_idealPos) > MAX_VISIBLE_DISTANCE_SQUARED)
		//{
		//	curr->m_visible = FALSE;
		//}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RECT_ORGANIZE_SYSTEM::Compute(
	const Rect2 * pScreenRect,
	unsigned int maxTries)
{
	if (pScreenRect)
	{
		SetupFrame(*pScreenRect);
		m_rectBounding = *pScreenRect;
		m_bRestrictToFrame = TRUE;
	}
	else
	{
		m_bRestrictToFrame = FALSE;
	}

	unsigned int tries = 0;
	unsigned int score;
	BOOL move = FALSE;
	while (tries < maxTries)
	{
		//DebugTrace(screenRect);
		if (m_frame % RECT_MGMT_FRAME_COUNT == 0 &&
			m_bUseProximal)
		{
			RecomputeProximal();
		}
		++m_frame;

		// setup
		++tries;
		score = 0;
		move = FALSE;

		for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
		{
			if (!curr->m_visible)
			{
				continue;
			}
			move |= MakeMove(curr, score);			
		}
		if (!move)
		{
			break;
		}
	}

	//trace("tries: %d  score: %d  time: %d\n", tries, score, GetTickCount() - start);

	if (m_bHideOverlaps)
	{
		HideOverlappingLabels();
	}
	UpdateRealPositions();

	return move;
}

BOOL RECT_ORGANIZE_SYSTEM::Compute(
	float fFrameLeft,
	float fFrameTop,
	float fFrameRight,
	float fFrameBottom,
	unsigned int maxTries)
{
	Rect2 screenRect;	
	screenRect.FromLTRB(fFrameLeft, fFrameTop, fFrameRight, fFrameBottom);

	return Compute(&screenRect, maxTries);
}

BOOL RECT_ORGANIZE_SYSTEM::Compute(
	unsigned int maxTries)
{
	return Compute(NULL, maxTries);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RECT_ORGANIZE_SYSTEM::UpdateRealPositions(
		void )
{
	for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
	{
		if (curr->m_pfLeft)
			*curr->m_pfLeft = (float)curr->m_textRect.Left();
		if (curr->m_pfRight)
			*curr->m_pfRight = (float)curr->m_textRect.Right();
		if (curr->m_pfTop)
			*curr->m_pfTop = (float)curr->m_textRect.Top();
		if (curr->m_pfBottom)
			*curr->m_pfBottom = (float)curr->m_textRect.Bottom();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RECT_ORGANIZE_SYSTEM::DebugTrace(
	const Rect2 & screenRect) const
{
	float totalarea = 0.0f;  
	int totallabels = 0;

	for (RECT_ENTRY * curr = m_activeList.Head(); curr; curr = curr->m_next)
	{
		trace("%clabel: %3d   item: %4.0f,%4.0f   label: (%4.0f,%4.0f)-(%4.0f,%4.0f)  dist: %d\n", curr->m_visible ? ' ' : '!',
			curr->m_index, curr->m_idealPos[XAXIS], curr->m_idealPos[YAXIS], 
			curr->m_textRect.m_center[XAXIS] - curr->m_textRect.m_radius[XAXIS],
			curr->m_textRect.m_center[YAXIS] - curr->m_textRect.m_radius[YAXIS],
			curr->m_textRect.m_center[XAXIS] + curr->m_textRect.m_radius[XAXIS],
			curr->m_textRect.m_center[YAXIS] + curr->m_textRect.m_radius[YAXIS],
			(int)sqrt((double)curr->m_textRect.GetDistanceSquared(curr->m_idealPos)));
		if (curr->m_visible)
		{
			totalarea += curr->m_textRect.m_radius[XAXIS] * curr->m_textRect.m_radius[YAXIS];
			++totallabels;
		}
	}
	trace("frame: %d   labels: %d   coverage: %d / %d  (%d%%)\n\n", m_frame, totallabels, totalarea, screenRect.Area(), (totalarea * 100) / (screenRect.Area()));
}


//----------------------------------------------------------------------------
// External functions
//----------------------------------------------------------------------------
RECT_ORGANIZE_SYSTEM * RectOrganizeInit(
	MEMORYPOOL * pool)
{
	RECT_ORGANIZE_SYSTEM *pSystem = (RECT_ORGANIZE_SYSTEM *)MALLOCZ(pool, sizeof(RECT_ORGANIZE_SYSTEM) );
	if (pSystem)
	{
		pSystem->Init();
	}

	return pSystem;
}

BOOL RectOrganizeDestroy(
	MEMORYPOOL * pool,
	RECT_ORGANIZE_SYSTEM * pSystem)
{
	ASSERT_RETFALSE(pSystem);
	pSystem->Free();
	FREE(pool, pSystem);

	return TRUE;
}

DWORD RectOrganizeAddRect(
	RECT_ORGANIZE_SYSTEM * pSystem,
	float fIdealX,
	float fIdealY,
	float *pfLeft,
	float *pfTop,
	float *pfRight,
	float *pfBottom)
{
	ASSERT_RETINVALID(pSystem);
	return pSystem->Add(fIdealX, fIdealY, pfLeft, pfTop, pfRight, pfBottom);
}

DWORD RectOrganizeAddRect(
	RECT_ORGANIZE_SYSTEM * pSystem,
	float fIdealX,
	float fIdealY,
	float *pfLeft,
	float *pfTop,
	float fWidth,
	float fHeight)
{
	ASSERT_RETINVALID(pSystem);
	return pSystem->Add(fIdealX, fIdealY, pfLeft, pfTop, fWidth, fHeight);
}

DWORD RectOrganizeAddRect(
	RECT_ORGANIZE_SYSTEM * pSystem,
	float fIdealX,
	float fIdealY,
	float fLeft,
	float fTop,
	float fWidth,
	float fHeight)
{
	ASSERT_RETINVALID(pSystem);
	return pSystem->Add(fIdealX, fIdealY, fLeft, fTop, fWidth, fHeight);
}


void RectOrganizeClear(
	RECT_ORGANIZE_SYSTEM * pSystem)
{
	ASSERT_RETURN(pSystem);
	pSystem->Clear();
}

void RectOrganizeRemoveRect(
	RECT_ORGANIZE_SYSTEM * pSystem,
	DWORD idRect)
{
	ASSERT_RETURN(pSystem);
	pSystem->Remove(idRect);
}

BOOL RectOrganizeCompute(
	RECT_ORGANIZE_SYSTEM * pSystem,
	float fFrameLeft,
	float fFrameTop,
	float fFrameRight,
	float fFrameBottom,
	unsigned int maxTries)
{
	ASSERT_RETFALSE(pSystem);

	return pSystem->Compute(fFrameLeft, fFrameTop, fFrameRight, fFrameBottom, maxTries);
}

BOOL RectOrganizeCompute(
	RECT_ORGANIZE_SYSTEM * pSystem,
	unsigned int maxTries)
{
	ASSERT_RETFALSE(pSystem);

	return pSystem->Compute(maxTries);
}