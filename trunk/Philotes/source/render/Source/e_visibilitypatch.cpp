//----------------------------------------------------------------------------
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
#include "dxC_state.h"
#include "dxC_buffer.h"

#include "e_visibilitymesh.h"
#include "e_visibilitypatch.h"



CVisibilityPatch::CVisibilityPatch( const VECTOR& Position,  			// top-left corner position
								    int TopLeftCellX,					// top left cell index
								    int TopLeftCellY,					// top left cell index
								    float TileWidth, 					// width of individual tile, in world units
								    float TileHeight,					// height (tallness) of an individual tile
									float BaseOpacity ) :				// base obscured opacity
								    m_Position( Position ),
								    m_TileWidth( TileWidth ),
								    m_TileHeight( TileHeight ),
								    m_TopLeftCellX( TopLeftCellX ),
								    m_TopLeftCellY( TopLeftCellY ),
								    m_VisibilityEmpty( TRUE ),
								    m_VisibilityFull( FALSE ),
								    m_VisibilityVertices( 0 ),
								    m_VisibilityIndices( 0 ),
									m_BaseOpacity( BaseOpacity )
{
	m_BoundingBox.vMin = Position;
	m_BoundingBox.vMin.fY -= TileWidth;
	m_BoundingBox.vMax.fZ -= TileHeight;
	m_BoundingBox.vMax = Position;
	m_BoundingBox.vMax.fX += TileWidth;
	m_BoundingBox.vMax.fZ += TileHeight;


	for( int X = 0; X <= KPatchVisibilityTiles; X ++ )
	{
		for( int Y = 0; Y <= KPatchVisibilityTiles; Y ++ )
		{
			m_VisibilityLevel[X][Y] = 255;
			m_FloatVisibilityLevel[X][Y] = 254;
		}
	}

	dxC_VertexBufferDefinitionInit( m_tVisibilityVBDef );
	dxC_IndexBufferDefinitionInit( m_tVisibilityIBDef );

	V( CreateVisibility() );
} // CVisibilityPatch::CVisibilityPatch()

CVisibilityPatch::~CVisibilityPatch( void )
{
	if( dxC_VertexBufferD3DExists( m_tVisibilityVBDef.nD3DBufferID[ 0 ] ) )
	{
		V( dxC_VertexBufferDestroy( m_tVisibilityVBDef.nD3DBufferID[ 0 ] ) );
	}
	V( dxC_IndexBufferDestroy( m_tVisibilityIBDef.nD3DBufferID ) );

} // CVisibilityPatch::~CVisibilityPatch()

PRESULT CVisibilityPatch::DeviceLost( void )
{
	if( dxC_VertexBufferD3DExists( m_tVisibilityVBDef.nD3DBufferID[ 0 ] ) )
	{
		V_RETURN( dxC_VertexBufferDestroy( m_tVisibilityVBDef.nD3DBufferID[ 0 ] ) );
		m_VisibilityEmpty = TRUE;
	}
	return S_OK;
}

PRESULT CVisibilityPatch::DeviceReset( void )
{
	// TODO - level based
	float ZPosition = 4.2f;// FIXME : need to set this in Levels.xls m_TileHeight;


	m_VisibilityVertices = ( m_VisibilityTilesWide + 1 ) * ( m_VisibilityTilesHigh + 1 );

	ASSERT_RETFAIL( ! dxC_VertexBufferD3DExists( m_tVisibilityVBDef.nD3DBufferID[ 0 ] ) );

	m_tVisibilityVBDef.nVertexCount = m_VisibilityVertices;
	m_tVisibilityVBDef.tUsage = D3DC_USAGE_BUFFER_MUTABLE;
	m_tVisibilityVBDef.dwFVF = VISIBILITY_FVF;
	m_tVisibilityVBDef.nBufferSize[ 0 ] = m_tVisibilityVBDef.nVertexCount * dxC_GetVertexSize( 0, &m_tVisibilityVBDef );

	// create the vertex buffer
	V_RETURN( dxC_CreateVertexBuffer( 0, m_tVisibilityVBDef ) );

	// Fill vertex buffer
	VISIBILITY_VERTEX *	pVertices = NULL;

	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( m_tVisibilityVBDef.nD3DBufferID[ 0 ] );
	ASSERT_RETFAIL( pVB && pVB->pD3DVertices );

	V_RETURN( dxC_MapEntireBuffer( pVB->pD3DVertices, D3DLOCK_DISCARD, D3D10_MAP_READ_WRITE, ( void ** ) &pVertices ) );
	if( !pVertices )
	{
		return E_FAIL;
	}

	VECTOR StartingPosition( m_Position );

	float TileWidth = m_TileWidth / KPatchVisibilityTiles;

	int	TopLeftX( (int)m_TopLeftCellX * (int)KPatchVisibilityTiles );
	int	TopLeftY( (int)m_TopLeftCellY * (int)KPatchVisibilityTiles );
	
	for( int y = 0; y < m_VisibilityTilesHigh + 1; y++ )
	{
		for( int x = 0; x < m_VisibilityTilesWide + 1; x++ )
		{
			pVertices->vPosition = StartingPosition + VECTOR( (float)x * TileWidth ,
															  (float)y * -TileWidth,
															  ZPosition );
			pVertices->pfTexCoords[0] = x * .15f + y *.15f;
			pVertices->pfTexCoords[1] = y * .15f - x * .15f;
			if( x + TopLeftX == 0 ||
				y + TopLeftY == 0 /*||
				x + TopLeftX >= pLevel.VisibilityWidth() ||
				y + TopLeftY >= pLevel.VisibilityHeight() */)
			{
				pVertices->color = D3DCOLOR_RGBA( 0, 0, 0, 255 );
			}
			else
			{
				pVertices->color = D3DCOLOR_RGBA( 0, 0, 0, 255 );
			}
			pVertices++;		

		}
	}

	V_RETURN( dxC_Unmap( pVB->pD3DVertices ) );
	for( int X = 0; X <= KPatchVisibilityTiles; X ++ )
	{
		for( int Y = 0; Y <= KPatchVisibilityTiles; Y ++ )
		{
			m_VisibilityLevel[X][Y] = 255;
			m_FloatVisibilityLevel[X][Y] = 254;
		}
	}
	return S_OK;
} // CVisibilityPatch::~CVisibilityPatch()



PRESULT CVisibilityPatch::CreateVisibility( void )
{
	m_VisibilityTilesWide = KPatchVisibilityTiles;
	m_VisibilityTilesHigh = KPatchVisibilityTiles;
/*	if( m_TopLeftCellX * KVisibilityCellsPerTile + KPatchVisibilityTiles >= pLevel.VisibilityWidth() )
	{
		m_VisibilityTilesWide++;
	}
	if( m_TopLeftCellY * KVisibilityCellsPerTile + KPatchVisibilityTiles >= pLevel.VisibilityHeight() )
	{
		m_VisibilityTilesHigh++;
	}
*/
	VECTOR Position( m_Position );




	// create the mutable (lockable) vertex buffer
	V_RETURN( DeviceReset() );



	// create the regular index buffer
	ASSERT_RETFAIL( ! dxC_IndexBufferD3DExists( m_tVisibilityIBDef.nD3DBufferID ) );

	m_VisibilityIndices = m_VisibilityTilesWide * m_VisibilityTilesHigh * 6;

	// Create the index buffer
	WORD*	pIndices = NULL;

	m_tVisibilityIBDef.nIndexCount = m_VisibilityIndices;
	m_tVisibilityIBDef.tFormat = D3DFMT_INDEX16;
	m_tVisibilityIBDef.nBufferSize = m_VisibilityIndices * sizeof(WORD);
	m_tVisibilityIBDef.tUsage = D3DC_USAGE_BUFFER_REGULAR;

	V_RETURN( dxC_CreateIndexBuffer( m_tVisibilityIBDef ) );

	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( m_tVisibilityIBDef.nD3DBufferID );
	ASSERT_RETFAIL( pIB && pIB->pD3DIndices );

	// Fill the index buffer
	V_RETURN( dxC_MapEntireBuffer( pIB->pD3DIndices, 0, D3D10_MAP_READ_WRITE, ( void * * ) &pIndices ) );

	int FaceIndex( 0 );
	for( int y = 0; y < m_VisibilityTilesHigh; y++ )
	{
		for( int x = 0; x < m_VisibilityTilesWide; x++ )
		{
			int Index = ( y * ( m_VisibilityTilesWide + 1 ) ) + ( x );
			*pIndices++ = ( WORD ) ( Index + m_VisibilityTilesWide + 2 );
			*pIndices++ = ( WORD ) ( Index + 1 );
			*pIndices++ = ( WORD ) ( Index  );
			*pIndices++ = ( WORD ) ( Index + m_VisibilityTilesWide + 1 );
			*pIndices++ = ( WORD ) ( Index + m_VisibilityTilesWide + 2 );
			*pIndices++ = ( WORD ) ( Index );
		}
	}

	V_RETURN( dxC_Unmap( pIB->pD3DIndices ) );
//	CalculateBounds();
	return S_OK;
} // CVisibilityPatch::CreateVisibility()

PRESULT CVisibilityPatch::UpdateVisibility( CVisibilityMesh& pVisibility,	// parent visibility list
											float TimeElapsed,				// time elapsed in seconds
											BOOL bDiscard /*= FALSE*/,		// TRUE on the first call of each frame
											BOOL bForce /*= FALSE*/ )		// force an update
{

	if( !dxC_VertexBufferD3DExists( m_tVisibilityVBDef.nD3DBufferID[ 0 ] ) )
	{
		DeviceReset();
		bForce = TRUE;
		TimeElapsed = 1000;
	}
	BOOL MustUpdate( FALSE );
	int VertexIndex( 0 );
	BOOL   Visible( TRUE );
	float UpdateTime = pow( .01f, TimeElapsed );
	for( int X = 0; X <= (int)m_VisibilityTilesWide; X ++ )
	{
		for( int Y = 0; Y <= (int)m_VisibilityTilesHigh; Y ++ )
		{
			int Level = 255;
			if( pVisibility.Visible( X + m_TopLeftCellX, Y + m_TopLeftCellY ) )
			{
				Level =  0;
			}

			int Surrounding = 0;
			Surrounding += 0 != pVisibility.Visible( X + m_TopLeftCellX - 1, Y + m_TopLeftCellY     );
			Surrounding += 0 != pVisibility.Visible( X + m_TopLeftCellX + 1, Y + m_TopLeftCellY     );
			Surrounding += 0 != pVisibility.Visible( X + m_TopLeftCellX    , Y + m_TopLeftCellY - 1 );
			Surrounding += 0 != pVisibility.Visible( X + m_TopLeftCellX    , Y + m_TopLeftCellY + 1 );
			Surrounding += 0 != pVisibility.Visible( X + m_TopLeftCellX - 1, Y + m_TopLeftCellY - 1 );
			Surrounding += 0 != pVisibility.Visible( X + m_TopLeftCellX - 1, Y + m_TopLeftCellY + 1 );
			Surrounding += 0 != pVisibility.Visible( X + m_TopLeftCellX + 1, Y + m_TopLeftCellY - 1 );
			Surrounding += 0 != pVisibility.Visible( X + m_TopLeftCellX + 1, Y + m_TopLeftCellY + 1 );

			// soften based on adjoining cells
			Level = (int)( (float)Level * ( 1.0f - ( (float)Surrounding / 16.0f ) ) );

			if( Level > 255 )
			{
				Level = 255;
			}
			if( Level < 0 )
			{
				Level = 0;
			}
			Level = (int)( (float) Level );//* .75f );
			if( m_FloatVisibilityLevel[X][Y] > Level )
			{
				float Delta = m_FloatVisibilityLevel[X][Y] - (float)Level;
				Delta *= UpdateTime;
				m_FloatVisibilityLevel[X][Y] = Level + Delta;
				if( m_FloatVisibilityLevel[X][Y] < Level || bForce )
				{
					m_FloatVisibilityLevel[X][Y] = (float)Level;
				}
			}
			else if( m_FloatVisibilityLevel[X][Y] < Level )
			{
				float Delta = (float)Level - m_FloatVisibilityLevel[X][Y];
				Delta *= UpdateTime;
				m_FloatVisibilityLevel[X][Y] = Level - Delta;
				if( m_FloatVisibilityLevel[X][Y] > Level || bForce  )
				{
					m_FloatVisibilityLevel[X][Y] = (float)Level;
				}
			}

			if( bForce || (int)m_FloatVisibilityLevel[X][Y] != m_VisibilityLevel[X][Y] )
			{
				m_VisibilityLevel[X][Y] = (int)m_FloatVisibilityLevel[X][Y];
				MustUpdate = TRUE;
			}
		}
	}


	if( MustUpdate )
	{
		m_VisibilityEmpty = TRUE;
		m_VisibilityFull = TRUE;
		// Fill vertex buffer
		VISIBILITY_VERTEX*	pVertices = NULL;

		ASSERT_RETFAIL( dxC_VertexBufferD3DExists( m_tVisibilityVBDef.nD3DBufferID[ 0 ] ) );

		D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( m_tVisibilityVBDef.nD3DBufferID[ 0 ] );
		ASSERT_RETFAIL( pVB && pVB->pD3DVertices );

		V_RETURN( dxC_MapEntireBuffer( pVB->pD3DVertices, bDiscard ? D3DLOCK_DISCARD : 0, D3D10_MAP_READ_WRITE, ( void * * ) &pVertices ) );
		if( !pVertices )
		{
			return E_FAIL;
		}

		for( int X = 0; X <= (int)m_VisibilityTilesWide; X ++ )
		{
			for( int Y = 0; Y <= (int)m_VisibilityTilesHigh; Y ++ )
			{
				if( m_VisibilityLevel[X][Y] != 0 )
				{
					m_VisibilityEmpty = FALSE;
				}
				if( m_VisibilityLevel[X][Y] != 255 )
				{
					m_VisibilityFull = FALSE;
				}
				int Index = ( Y * ( m_VisibilityTilesWide + 1 ) ) + ( X );

				if( X + m_TopLeftCellX == 0 ||
					Y + m_TopLeftCellY == 0 ||
					X + m_TopLeftCellX >= (int)pVisibility.VisibilityCellsWide() ||
					Y + m_TopLeftCellY >= (int)pVisibility.VisibilityCellsHigh() )
				{
					pVertices[Index].color = D3DCOLOR_RGBA( 0, 0, 0, 255 );
				}
				else
				{
					pVertices[Index].color = D3DCOLOR_RGBA( 0, 
															0, 
															0, 
															(int)( m_VisibilityLevel[X][Y] * m_BaseOpacity ) );
				}
			}
		}

		V_RETURN( dxC_Unmap( pVB->pD3DVertices ) );
	}

	return S_OK;
} // CVisibilityPatch::UpdateVisibility()


PRESULT CVisibilityPatch::RenderVisibility( const MATRIX& ViewportMatrix )	// viewport matrix
{
	if( m_VisibilityIndices > 0 && !m_VisibilityEmpty )
	{

		V_RETURN( dxC_SetTransform( D3DTS_VIEW, (D3DXMATRIXA16*)&ViewportMatrix ) );

		V_RETURN( dxC_SetStreamSource( m_tVisibilityVBDef, 0 ) );
		V_RETURN( dxC_SetIndices( m_tVisibilityIBDef, TRUE ) );

		V_RETURN( dxC_DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_VisibilityVertices, 0, m_VisibilityIndices / 3 ) );
	
	}

	return S_OK;
} // CVisibilityPatch::RenderVisibility()
