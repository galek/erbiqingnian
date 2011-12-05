//----------------------------------------------------------------------------
// visibilitymesh.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#ifndef _VISIBILITYMESH_H
#define _VISIBILITYMESH_H


#include "quadtree.h"

// how many tiles wide/high per patch?

const int KPatchVisibilityTiles( 12 );
const int KMaxVisibilityRadius( 24 );


// Forward Declares
class CVisibilityPatchBoundsRetriever;
class CVisibilityPatch;
struct LEVEL;

// a VisibilityMesh contains all the VisibilityPatches, and sorts them for rendering

class CVisibilityMesh
{


public:

	CVisibilityMesh( const LEVEL& Level );			// level this is for

	~CVisibilityMesh( void );
	
	void		Update( float TimeElapsed );	// time elapsed in seconds

	void		Sort( float TimeElapsed );					//teme elapsed in seconds

	void		SortVisible( void );			

	PRESULT		RenderVisibility( const MATRIX& ViewportMatrix );	// viewport matrix

	void		Finalize( void );

	void		ClearVisibilityDataLocal( void );

	void		ClearVisibilityData( void );

	void		ForceAllVisible( void );

	void		NotifyDeviceLost( void );

	void		NotifyDeviceReset( void );
	////////////////////////////////////////////////////////////////////////////////////////////
	//ACCESSORS
	////////////////////////////////////////////////////////////////////////////////////////////

	VECTOR&		Center( void )					{	return m_VisibilityCenter;	};

	VECTOR&		CenterFloat( void )				{	return m_VisibilityCenterFloat;	};

	void		UpdateVisibility( void )		{	m_UpdateVisibility = TRUE;	};

	float		BaseOpacity( void )				{	return m_BaseOpacity;		};

	// CML 2007.1.26 - Moved this inline because it's called frequently in a tight loop that shows up in a profile
	inline BOOL	Visible( int X,		// visibility cell to check x
						 int Y )	// visibility cell to check y
	{
		if( X > 0 && Y > 0 && X < m_VisibilityCellsWide - 1 && Y < m_VisibilityCellsHigh - 1 )
		{
			return m_VisibilityData[X][Y];
		}
		return FALSE;
	}

	int			GetXTile( float X );
	int			GetYTile( float Y );
	int			GetXTileHigh( float X );
	int			GetYTileHigh( float Y );

	int			VisibilityCellsWide( void )	{	return m_VisibilityCellsWide;	};
	int			VisibilityCellsHigh( void )	{	return m_VisibilityCellsHigh;	};

	float		PatchVisibilityWidth( void )	{	return m_PatchVisibilityWidth;	};
	////////////////////////////////////////////////////////////////////////////////////////////
	//MUTATORS
	////////////////////////////////////////////////////////////////////////////////////////////

	void		SetCenter( const VECTOR& Center )	{	m_VisibilityCenter = Center;	};

	void		SetCenterFloat( const VECTOR& Center )	{	m_VisibilityCenterFloat = Center;	};

	void		UpdateLevelWideVisibility( void );		

	void		SetVisibility( int X,			// visibility cell to check x
							   int Y,			// visibility cell to check y	
							   BOOL Value );	// visibility value


private:

	float			m_BaseOpacity;

	int				m_CellsWide;
	int				m_CellsHigh;

	int				m_VisibilityCellsWide;
	int				m_VisibilityCellsHigh;

	float			m_CellWidth;

	VECTOR			m_LevelTopLeft;

	CQuadtree< CVisibilityPatch*, CVisibilityPatchBoundsRetriever>*		m_pQuadTree;

	SIMPLE_DYNAMIC_ARRAY< SIMPLE_DYNAMIC_ARRAY< CVisibilityPatch* > >		m_pLevelPatches;

	SIMPLE_DYNAMIC_ARRAY< CVisibilityPatch* >						m_pSortedPatches;

	
	BOOL				m_UpdateVisibility;

	BOOL**				m_VisibilityData;

	VECTOR			m_VisibilityCenter;
	VECTOR			m_VisibilityCenterFloat;
	VECTOR			m_LastSortCenter;

	float			m_PatchVisibilityWidth;
	float			m_fVisibilityScale;
};


inline CVisibilityMesh * e_VisibilityMeshGetCurrent()
{
	extern CVisibilityMesh * gpCurrentVisibilityMesh;
	return gpCurrentVisibilityMesh;
}

inline void e_VisibilityMeshSetCurrent( CVisibilityMesh * pVisibilityMesh )
{
	extern CVisibilityMesh * gpCurrentVisibilityMesh;
	gpCurrentVisibilityMesh = pVisibilityMesh;
}

#endif