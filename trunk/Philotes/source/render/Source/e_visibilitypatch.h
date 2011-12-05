//----------------------------------------------------------------------------
// visibilitypatch.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _VISIBILITYPATCH_H
#define _VISIBILITYPATCH_H

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

class CVisibilityMesh;

typedef struct {
	VECTOR vPosition;
	D3DCOLOR	color;
	float pfTexCoords[ 2 ];
} VISIBILITY_VERTEX;





// a VisibilityPatch contains a model, which is its visual representation,
// as well as data about contents and passability.

class CVisibilityPatch
{


public:

	CVisibilityPatch( const VECTOR& Position,		// top-left corner position
					 int TopLeftCellX,				// top left cell index
					 int TopLeftCellY,				// top left cell index
					 float TileWidth,					// width of individual tile, in world units
					 float TileHeight,				// height (tallness) of individual tile
					 float BaseOpacity );			// base opacity of 'obscured' areas
	~CVisibilityPatch( void );


	PRESULT 		RenderVisibility(const MATRIX& ViewportMatrix );	// viewport matrix


	////////////////////////////////////////////////////////////////////////////////////////////
	//ACCESSORS
	////////////////////////////////////////////////////////////////////////////////////////////

	VECTOR*	GetCullingBounds( void )		{	return m_CullingBounds;	};

	const BOUNDING_BOX& BoundingBox( void )			{	return m_BoundingBox;		};

	////////////////////////////////////////////////////////////////////////////////////////////
	//MUTATORS
	////////////////////////////////////////////////////////////////////////////////////////////

	void			SetBaseOpacity( float BaseOpacity )	{	m_BaseOpacity = BaseOpacity; }


	PRESULT 		UpdateVisibility(	CVisibilityMesh& pVisibility,	// parent visibility list
										float TimeElapsed,				// time elapsed in seconds
										BOOL bDiscard = FALSE,			// TRUE on the first call of each frame
										BOOL bForce = FALSE );			// force an update


	PRESULT			CreateVisibility( void );	// direct3d device
	PRESULT			DeviceLost( void );
	PRESULT			DeviceReset( void );

private:





	VECTOR			m_Position;

	float				m_TileWidth;

	float				m_TileHeight;

	int					m_VisibilityTilesWide;
	int					m_VisibilityTilesHigh;

	int					m_VisibilityVertices;
	int					m_VisibilityIndices;

	int					m_TopLeftCellX;
	int					m_TopLeftCellY;

	float				m_BaseOpacity;

	//D3D_INDEX_BUFFER	m_pVisibilityIB;
	//SPD3DCVB			m_pVisibilityVB;
	D3D_INDEX_BUFFER_DEFINITION		m_tVisibilityIBDef;
	D3D_VERTEX_BUFFER_DEFINITION	m_tVisibilityVBDef;
	BOOL							m_VisibilityEmpty;	// has no partially transparent
	BOOL							m_VisibilityFull;	// has ONLY opaque pieces

	BOUNDING_BOX		m_BoundingBox;
	VECTOR				m_CullingBounds[8];

	int					m_VisibilityLevel[KPatchVisibilityTiles+2][KPatchVisibilityTiles+2];
	float				m_FloatVisibilityLevel[KPatchVisibilityTiles+2][KPatchVisibilityTiles+2];
};

#endif