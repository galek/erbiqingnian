
// visibilitypatch.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_effect.h"
#include "dxC_types.h"
#include "dxC_2d.h"
#include "dxC_main.h"
#include "dxC_state.h"
#include "level.h"

#include "e_visibilitymesh.h"
#include "e_visibilitypatch.h"
#include "e_frustum.h"

#include "camera_priv.h"

class CVisibilityPatchBoundsRetriever
{

	public:
	
		const BOUNDING_BOX& operator()(CVisibilityPatch* visibilityPatch) const
		{
			return visibilityPatch->BoundingBox();
		}
};


CVisibilityMesh * gpCurrentVisibilityMesh = NULL;




CVisibilityMesh::CVisibilityMesh( const LEVEL& Level ) :			// level this is for
								  m_pQuadTree( NULL ),
								  m_UpdateVisibility( FALSE ),
								m_VisibilityCenter( 0, 0, 0 ), 
								m_VisibilityCenterFloat( 0, 0, 0 ) ,
								m_LastSortCenter( 0, 0, 0 )
{
	m_fVisibilityScale = Level.m_pLevelDefinition->fVisibilityDistanceScale;
	m_PatchVisibilityWidth = Level.m_pLevelDefinition->fVisibilityTileSize;
	ArrayInit(m_pSortedPatches, NULL, 2);	
	float LevelWidth = Level.m_BoundingBox.vMax.fX - Level.m_BoundingBox.vMin.fX + 4;
	float LevelHeight = Level.m_BoundingBox.vMax.fY - Level.m_BoundingBox.vMin.fY + 4;
	m_CellWidth = ( m_PatchVisibilityWidth * KPatchVisibilityTiles );
	m_CellsWide = (int)CEIL( LevelWidth / ( m_CellWidth ) ) + 4;
	m_CellsHigh = (int)CEIL( LevelHeight / ( m_CellWidth ) ) + 4;
	m_VisibilityCellsWide = m_CellsWide * KPatchVisibilityTiles;
	m_VisibilityCellsHigh = m_CellsHigh * KPatchVisibilityTiles;

	VECTOR LevelCenter = Level.m_BoundingBox.vMin;
	LevelCenter.fX += LevelWidth * .5f;
	LevelCenter.fY += LevelHeight * .5f;
	
	VECTOR LevelMinBounds = Level.m_BoundingBox.vMin;
	VECTOR LevelMaxBounds = Level.m_BoundingBox.vMax;

	LevelMinBounds.fX = LevelCenter.fX - ( m_CellsWide * m_CellWidth * .5f );
	LevelMinBounds.fY = LevelCenter.fY - ( m_CellsHigh * m_CellWidth * .5f );

	LevelMaxBounds.fX = LevelCenter.fX + ( m_CellsWide * m_CellWidth * .5f );
	LevelMaxBounds.fY = LevelCenter.fY + ( m_CellsHigh * m_CellWidth * .5f );

	BOUNDING_BOX BBox;
	BBox.vMin = LevelMinBounds;
	BBox.vMax = LevelMaxBounds;
	
	m_pQuadTree = new CQuadtree<CVisibilityPatch*, CVisibilityPatchBoundsRetriever>( );
	
	CVisibilityPatchBoundsRetriever boundsRetriever;
	m_pQuadTree->Init(BBox, m_CellWidth, boundsRetriever);
	
	m_VisibilityData = new BOOL*[m_VisibilityCellsWide];
	for( int i = 0; i < m_VisibilityCellsWide; i++ )
	{
		m_VisibilityData[i] = new BOOL[m_VisibilityCellsHigh];
	}

	ClearVisibilityData();
	//ForceAllVisible();
	ArrayInit(m_pLevelPatches, NULL, m_CellsWide);

	m_LevelTopLeft = VECTOR( LevelMinBounds.fX, LevelMaxBounds.fY, 0 );

	m_BaseOpacity = Level.m_pLevelDefinition->fVisibilityOpacity;

	for( int i = 0; i < m_CellsWide; i++ )
	{
		ArrayAdd(m_pLevelPatches);
		ArrayInit(m_pLevelPatches[i], NULL, m_CellsHigh);
		for (int j = 0; j < m_CellsHigh; j++)
		{
			VECTOR Position( LevelMinBounds.fX + (float)i * m_CellWidth, 
							 LevelMaxBounds.fY +  (float)j * -m_CellWidth,
							 0 );


			CVisibilityPatch* pPatch = new CVisibilityPatch( Position,
														     i * KPatchVisibilityTiles,
														     j * KPatchVisibilityTiles,
														     m_CellWidth,
														     LevelMaxBounds.fZ,
															 m_BaseOpacity );

			ArrayAddItem(m_pLevelPatches[i], pPatch);
		}
	}

	Finalize();

	UpdateLevelWideVisibility();

} // CVisibilityMesh::CVisibilityMesh()



CVisibilityMesh::~CVisibilityMesh( void )
{
	m_pSortedPatches.Destroy();
	if( m_VisibilityData != NULL )
	{
		for( int i = 0; i < m_VisibilityCellsWide; i++ )
		{
			delete[]( m_VisibilityData[i] );
		}
	}
	delete[] ( m_VisibilityData );

	for( unsigned int i = 0; i < m_pLevelPatches.Count(); i++ )
	{
		for( unsigned int j = 0; j < m_pLevelPatches[i].Count(); j++ )
		{
			delete( m_pLevelPatches[i][j] );
		}
		m_pLevelPatches[i].Destroy();
	}
	m_pLevelPatches.Destroy();

	m_pSortedPatches.Destroy();
	
	if( m_pQuadTree )
	{
		delete( m_pQuadTree );
	}

} // CVisibilityMesh::~CVisibilityMesh()


void CVisibilityMesh::ClearVisibilityData( void )
{
	for( int i = 0; i < m_VisibilityCellsWide; i++ )
	{
		for( int j = 0; j < m_VisibilityCellsHigh; j++ )
		{
			m_VisibilityData[i][j] = FALSE;
		}
	}

} // CVisibilityMesh::ClearVisibilityData()



void CVisibilityMesh::ClearVisibilityDataLocal( void )
{

	float fVisibilityRadius = (float)KMaxVisibilityRadius * m_fVisibilityScale;

	int StartX = MAX( GetXTile( m_VisibilityCenterFloat.fX - ( fVisibilityRadius + 10  ) ), 0 );
	int EndX =  MIN( GetXTileHigh( m_VisibilityCenterFloat.fX + ( fVisibilityRadius + 10 ) ), m_VisibilityCellsWide );
	int StartY =  MAX( GetYTile( m_VisibilityCenterFloat.fY + ( fVisibilityRadius + 10 ) ), 0 );
	int EndY = MIN( GetYTileHigh( m_VisibilityCenterFloat.fY - ( fVisibilityRadius + 10 ) ), m_VisibilityCellsHigh );

	for( int i = StartX; i < EndX; i++ )
	{
		for( int j = StartY; j < EndY; j++ )
		{
			m_VisibilityData[i][j] = FALSE;
		}
	}

} // CVisibilityMesh::ClearVisibilityData()

void CVisibilityMesh::ForceAllVisible( void )
{
	for( int i = 0; i < m_VisibilityCellsWide; i++ )
	{
		for( int j = 0; j < m_VisibilityCellsHigh; j++ )
		{
			m_VisibilityData[i][j] = TRUE;
		}
	}

} // CVisibilityMesh::ForceAllVisible()

//BOOL CVisibilityMesh::Visible( int X,	// visibility cell to check x
//							   int Y )	// visibility cell to check y	
//{
//	if( X > 0 && Y > 0 && X < m_VisibilityCellsWide - 1 && Y < m_VisibilityCellsHigh - 1 )
//	{
//		return m_VisibilityData[X][Y];
//	}
//	return FALSE;
//} // CVisibilityMesh::Visible()

int CVisibilityMesh::GetXTile( float X )
{
	return (int)FLOOR( ( X - m_LevelTopLeft.fX ) / m_PatchVisibilityWidth );
} // CVisibilityMesh::GetXTile()

int CVisibilityMesh::GetYTile( float Y )
{
	return (int)FLOOR( -( Y - m_LevelTopLeft.fY ) / m_PatchVisibilityWidth );
} // CVisibilityMesh::GetYTile()

int CVisibilityMesh::GetXTileHigh( float X )
{
	return (int)FLOOR( ( X - m_LevelTopLeft.fX ) / m_PatchVisibilityWidth + .5f );
} // CVisibilityMesh::GetXTileHigh()

int CVisibilityMesh::GetYTileHigh( float Y )
{
	return (int)FLOOR( -( Y - m_LevelTopLeft.fY ) / m_PatchVisibilityWidth + .5f );
} // CVisibilityMesh::GetYTileHigh()

void CVisibilityMesh::SetVisibility( int X,			// visibility cell to check x
								     int Y,			// visibility cell to check y	
								     BOOL Value )	// visibility value
{
	if( X > 0 && Y > 0 && X < m_VisibilityCellsWide - 1 && Y < m_VisibilityCellsHigh - 1 )
	{
		m_VisibilityData[X][Y] = Value;
	}
} // CVisibilityMesh::SetVisibility()

void CVisibilityMesh::Finalize( void )
{

	for( int i = 0; i < m_CellsWide; i++ )
	{
		for( int j = 0; j < m_CellsHigh; j++ )
		{
			m_pQuadTree->Add( m_pLevelPatches[i][j],
								  m_pLevelPatches[i][j]->BoundingBox() );
		}
	}

} // CVisibilityMesh::Finalize()

// sort for rendering
void CVisibilityMesh::Sort(float TimeElapsed )					//teme elapsed in seconds
{




	MATRIX matWorld, matView, matProj;
	VECTOR vEyeVector;
	VECTOR vEyeLocation;
	e_InitMatrices( &matWorld, &matView, &matProj, NULL, &vEyeVector, &vEyeLocation );

	const PLANE * tPlanes = e_GetCurrentViewFrustum()->GetPlanes();
	ASSERT_RETURN( tPlanes );
	BOUNDING_BOX AABBBounds;
	e_ExtractAABBBounds( &matView, &matProj, &AABBBounds );
	const struct CAMERA_INFO * pCameraInfo = CameraGetInfo();

	int TileX( GetXTile( pCameraInfo->vLookAt.fX ) );
	int TileY( GetYTile( pCameraInfo->vLookAt.fY ) );

	VECTOR VisibilityCenter( (float)TileX, 0, (float)TileY );


	if( VisibilityCenter != m_LastSortCenter )
	{
		ArrayClear(m_pSortedPatches);
		float fSlop = 120 - ( (float)KMaxVisibilityRadius * m_fVisibilityScale );
		AABBBounds.vMin = pCameraInfo->vLookAt - VECTOR( fSlop, fSlop, 0 );
		AABBBounds.vMax = pCameraInfo->vLookAt + VECTOR( fSlop, fSlop, 0 );
		m_pQuadTree->GetItems( /*tPlanes,*/ AABBBounds, m_pSortedPatches );
	}
	m_LastSortCenter = VisibilityCenter;

	// for performance reasons, should only discard first lock of each frame
	BOOL bDiscard = TRUE;
	for( unsigned int i = 0; i < m_pSortedPatches.Count(); i++ )
	{
		float Dist = BoundingBoxDistanceSquaredXY( &m_pSortedPatches[i]->BoundingBox(), &m_VisibilityCenterFloat );
		if( Dist < 110 * 110 )
		{
			V( m_pSortedPatches[i]->UpdateVisibility( *this, TimeElapsed, bDiscard ) );
		}
		else if( Dist > 120 * 120 )
		{
			m_pSortedPatches[i]->DeviceLost();
		}
		bDiscard = FALSE;
	}

} // CVisibilityMesh::Sort()


// sort for rendering
void CVisibilityMesh::SortVisible( void )				
{
	MATRIX matWorld, matView, matProj;
	VECTOR vEyeVector;
	VECTOR vEyeLocation;
	e_InitMatrices( &matWorld, &matView, &matProj, NULL, &vEyeVector, &vEyeLocation );

	const PLANE * tPlanes = e_GetCurrentViewFrustum()->GetPlanes();
	ASSERT_RETURN( tPlanes );
	BOUNDING_BOX AABBBounds;
	e_ExtractAABBBounds( &matView, &matProj, &AABBBounds );
	ArrayClear(m_pSortedPatches);

	m_pQuadTree->GetItems( /*tPlanes,*/ AABBBounds, m_pSortedPatches );


} // CVisibilityMesh::SortVisible()

void CVisibilityMesh::UpdateLevelWideVisibility( void )
{
	for( int i = 0; i < m_CellsWide; i++ )
	{
		for( int j = 0; j < m_CellsHigh; j++ )
		{
			m_pLevelPatches[i][j]->UpdateVisibility( *this, 1000, TRUE, TRUE );
		}
	}

} // CVisibilityMesh::UpdateLevelWideVisibility()

// assumes that the level has already been sorted
PRESULT CVisibilityMesh::RenderVisibility( const MATRIX& ViewportMatrix )		// viewport matrix
{
#ifdef ENGINE_TARGET_DX9
	V( dx9_SetFVF( VISIBILITY_FVF ) );
	V( dx9_SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE ) );
/*
	dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );*/
	//dxC_SetRenderState( D3DRS_LIGHTING, FALSE );


	// render visibility shadows (fog of war)
	//for( unsigned int i = 0; i < m_pSortedPatches.size(); i++ )
	//{
	//	m_pSortedPatches[i]->RenderVisibility( ViewportMatrix );
	//}

	MATRIX NewMatrix;
	dxC_SetRenderState( D3DRS_ZENABLE, TRUE );
	/*MatrixTranslation( &NewMatrix, &VECTOR( 0, 0, -2.1f ) );
	MatrixMultiply( &NewMatrix, &NewMatrix, &ViewportMatrix );
	
	// render visibility shadows (fog of war)
	for( unsigned int i = 0; i < m_pSortedPatches.size(); i++ )
	{
		m_pSortedPatches[i]->RenderVisibility( NewMatrix );
	}*/

	MatrixTranslation( &NewMatrix, &VECTOR( 0, 0, -.01f ) );
	MatrixMultiply( &NewMatrix, &NewMatrix, &ViewportMatrix );
	// render visibility shadows (fog of war)
	for( unsigned int i = 0; i < m_pSortedPatches.Count(); i++ )
	{
		V( m_pSortedPatches[i]->RenderVisibility( NewMatrix ) );
	}

	//dxC_SetRenderState( D3DRS_LIGHTING, TRUE );

	// alpha blending back off
/*	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	dx9_SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );

	// turn zwrite/read back on
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
	dxC_SetRenderState( D3DRS_ZENABLE, TRUE );*/
	V( dx9_SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE ) );
	V( dx9_SetFVF( D3DC_FVF_STD ) );
#endif
	return S_OK;
} // CVisibilityMesh::RenderVisibility()



void CVisibilityMesh::NotifyDeviceLost( void )
{
	for( int i = 0; i < m_CellsWide; i++ )
	{
		for( int j = 0; j < m_CellsHigh; j++ )
		{
			V( m_pLevelPatches[i][j]->DeviceLost() );
		}
	}

} // CVisibilityMesh::NotifyDeviceLost()



void CVisibilityMesh::NotifyDeviceReset( void )
{
	for( int i = 0; i < m_CellsWide; i++ )
	{
		for( int j = 0; j < m_CellsHigh; j++ )
		{
			V( m_pLevelPatches[i][j]->DeviceReset() );
		}
	}
	UpdateLevelWideVisibility();
} // CVisibilityMesh::NotifyDeviceLost()
