//----------------------------------------------------------------------------
// dxC_automap.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_AUTOMAP__
#define __DXC_AUTOMAP__

#include "e_automap.h"

namespace FSSE
{;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define AUTOMAP_GENERATE_VERTEX_DECL		VERTEX_DECL_SIMPLE
#define AUTOMAP_DISPLAY_VERTEX_DECL			VERTEX_DECL_XYZ_UV

#define AUTOMAP_EDGE_BUFFER					4.0f
#define AUTOMAP_WALL_WIDTH_METERS			0.8f

#define AUTOMAP_BAND_HEIGHT					4.f
#define AUTOMAP_BAND_HEIGHT_MYTHOS			200.f
#define AUTOMAP_CENTER_POSITION_Z_OFFSET	1.f

#define AUTOMAP_FLOOR_ALPHA					0x70
#define AUTOMAP_WALL_ALPHA					0xff
#define AUTOMAP_WALL_ALPHA_MYTHOS			0x98

#define AUTOMAP_MAX_SCALE_FOR_MSAA			6.f
#define AUTOMAP_MIN_RT_RES_FOR_ALWAYS_MSAA	600

#define FOW_NODE_SIZE						7.f
#define FOW_REVEAL_HEIGHT_RADIUS			2.f			// How far above and below to reveal from any given node position
#define FOW_REVEAL_XY_RADIUS				5.f
#define FOW_NODE_COVERAGE_RADIUS			4


#define FOW_ROUND(f)    (int(floor((f) + .5)))

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef VECTOR3	AUTOMAP_VERTEX;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct FOG_OF_WAR
{
	enum FLAGBIT
	{
		FLAGBIT_DIMENSIONS_SET = 0,
		FLAGBIT_DISPLAY_SET,
		FLAGBIT_NEED_RENDER_GENERATE,
		FLAGBIT_ALWAYS_SHOW_ALL,
	};

	typedef BYTE HEIGHT;
	struct NODE
	{
		HEIGHT	nMax;
		HEIGHT	nMin;
	};
	typedef unsigned short NODE_INDEX;
	static const unsigned int MAX_NODE_INDEX = 0xfffe;
	static const NODE_INDEX INVALID_NODE = -1;

	int m_nRegionID;
protected:
	DWORD m_dwFlagBits;
	BOUNDING_BOX m_tBBox;
	// Uses a 2D index array to index into the sparse node list.
	resizable_array<NODE_INDEX>	m_pIndex;
	resizable_array<NODE> m_pNodes;
	int m_nNodesMapped;
	int m_nXRes;
	int m_nYRes;
	int m_nZRes;
	VECTOR2 m_vBias;
	VECTOR2 m_vScale;
	float m_fHeightBias;
	float m_fHeightScale;
	HEIGHT m_nHeightRevealRad;
	int m_nMaterial;
	int m_nRevealTexture;
	VECTOR m_vPosition;
	float m_fXYRevealRadius;

	D3D_VERTEX_BUFFER_DEFINITION m_tVBDef;
	D3D_INDEX_BUFFER_DEFINITION  m_tIBDef;

	void GetNodeXY( const VECTOR & vPos, int & nX, int & nY )
	{
		ASSERT( TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET ) );
		int nX1 = ROUND( (vPos.x + m_vBias.x) * m_vScale.x );

#if ISVERSION(DEBUG_VERSION)
		int nX2 = FOW_ROUND( (vPos.x + m_vBias.x) * m_vScale.x );
		ASSERT( nX1 == nX2 );
#endif

		nX = nX1;
		nY = ROUND( (vPos.y + m_vBias.y) * m_vScale.y );
	}

	void GetNodeWorld( int nX, int nY, VECTOR & vPos )
	{
		ASSERT( TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET ) );
		vPos.x = nX / m_vScale.x - m_vBias.x;
		vPos.y = nY / m_vScale.y - m_vBias.y;
	}

	void GetNodeScreen( int nX, int nY, VECTOR & vPos )
	{
		ASSERT( TESTBIT_DWORD( m_dwFlagBits, FLAGBIT_DIMENSIONS_SET ) );
		vPos.x = float(nX) / (m_nXRes-1);
		vPos.y = float(nY) / (m_nYRes-1);
	}

	NODE_INDEX * GetNodeIndex( int nX, int nY )
	{
		int i = nY * m_nXRes + nX;
		ASSERT_RETNULL( (DWORD)i < m_pIndex.GetAllocElements() );
		return &m_pIndex[ i ];
	}

	void GetNodeCenter( NODE_INDEX nIndex, VECTOR & vCenter );

	HEIGHT GetMappedHeight( float fZ )
	{
		fZ += m_fHeightBias;
		fZ *= m_fHeightScale;
		int nZ = FLOAT_TO_INT( fZ );
		nZ = MAX( nZ, 0 );
		nZ = MIN( nZ, 255 );
		return static_cast<HEIGHT>(nZ);
	}

	NODE * GetNode( NODE_INDEX nIndex )
	{
		if ( nIndex == INVALID_NODE )
			return NULL;
		ASSERT_RETNULL( nIndex < m_pNodes.GetAllocElements() );
		return &m_pNodes[ nIndex ];
	}

	NODE * AddNode( int nX, int nY )
	{
		ASSERTV_RETNULL( nX >= 0 && nX < m_nXRes, "X %d, XRes %d", nX, m_nXRes );
		ASSERTV_RETNULL( nY >= 0 && nY < m_nYRes, "Y %d, YRes %d", nY, m_nYRes );
		NODE_INDEX * pnIndex = GetNodeIndex( nX, nY );
		if ( ! pnIndex )
			return NULL;
		if ( INVALID_NODE != *pnIndex )
		{
			// already added
			return GetNode( *pnIndex );
		}
		// add it!
		*pnIndex = static_cast<NODE_INDEX>(m_nNodesMapped);
		ASSERT_RETNULL( *pnIndex >= 0 );
		ASSERT_RETNULL( (DWORD)*pnIndex < m_pNodes.GetAllocElements() );
		m_nNodesMapped++;
		m_pNodes[*pnIndex].nMax = 0;
		m_pNodes[*pnIndex].nMin = m_nZRes-1;
		return &m_pNodes[ *pnIndex ];
	}

	void RestoreMaterial();
	void MarkNode( NODE * pNode, HEIGHT nZmax, HEIGHT nZmin );
	PRESULT PrepareBuffers();
	PRESULT FillNodeVerts( int nX, int nY, const NODE * pNode, struct FOGOFWAR_VERTEX * pVerts );
	PRESULT FillNodeVerts( int nIter, const NODE * pNode, struct FOGOFWAR_VERTEX * pVerts );
	void HideAll();
	void ShowAll();

public:
	void Init( int nRegion );

	void Destroy()
	{
		m_pNodes.Destroy();
		m_pIndex.Destroy();

		for ( int i = 0; i < DXC_MAX_VERTEX_STREAMS; ++i )
		{
			if ( m_tVBDef.pLocalData[i] )
			{
				FREE( NULL, m_tVBDef.pLocalData[i] );
				m_tVBDef.pLocalData[i] = NULL;
			}
			if ( dxC_VertexBufferD3DExists( m_tVBDef.nD3DBufferID[i] ) )
			{
				V( dxC_VertexBufferDestroy( m_tVBDef.nD3DBufferID[i] ) );
			}
		}

		if ( m_tIBDef.pLocalData )
		{
			FREE( NULL, m_tIBDef.pLocalData );
			m_tIBDef.pLocalData = NULL;
		}
		if ( dxC_IndexBufferD3DExists( m_tIBDef.nD3DBufferID ) )
		{
			V( dxC_IndexBufferDestroy( m_tIBDef.nD3DBufferID ) );
		}
	}

	PRESULT Load( class BYTE_BUF & buf );
	PRESULT Save( class WRITE_BUF_DYNAMIC & fbuf );

	PRESULT DeviceLost();
	PRESULT DeviceReset();

	PRESULT SetShowAll( BOOL bShowAll );
	PRESULT SetDisplay( const VECTOR & vPos, float fRadius );
	PRESULT SetDimensions( const BOUNDING_BOX & tBBox );
	PRESULT MarkVisibleInRange( const VECTOR& vPos, float fRadius );
	float GetMappedCurrentHeight();
	PRESULT Update();
	PRESULT RenderGenerate();

	void SetDirty()
	{
		SETBIT( m_dwFlagBits, FLAGBIT_NEED_RENDER_GENERATE, TRUE );
	}
};








struct AUTOMAP
{
	// CHash bits
	int nId;
	AUTOMAP * pNext;

	enum COLORS
	{
		SAME	= ARGB_MAKE_FROM_INT( 127, 127, 127, 255 ),
		SHADOW	= ARGB_MAKE_FROM_INT( 100, 100, 100, 100 ),
	};

	enum BUFFER_TYPE
	{
		GROUND = 0,
		WALL,

		NUM_BUFFERS,
	};

	enum BANDS
	{
		BAND_LOW = 0,
		BAND_MID,
		BAND_HIGH,

		NUM_BANDS
	};

	enum FLAGBITS
	{
		FLAGBIT_BOUNDING_BOX_INIT = 0,
		FLAGBIT_FINALIZED,
		FLAGBIT_RENDER_DIRTY,
	};

	DWORD m_dwFlagBits;
	BOUNDING_BOX m_tMapBBox;
	MATRIX m_mWorldToVP;
	float m_fMetersPerPixel;
	float m_fMetersSpan;

	VECTOR m_vCenterPos_W;
	float m_fScaleFactor;
	MATRIX m_mRotate;

	FOG_OF_WAR m_tFogOfWar;

	int m_nTotalBands;
	int m_nCurBandLow;
	float m_fBandMin[ NUM_BANDS ];
	float m_fBandWeights[ NUM_BANDS ];


	D3D_VERTEX_BUFFER_DEFINITION	m_tVBDef[ NUM_BUFFERS ];			// two streams: position(static) and visibility factor(dynamic)

	D3D_VERTEX_BUFFER_DEFINITION	m_tRenderVBDef;

	PRESULT Init( int nRegion );
	PRESULT Free();

	PRESULT Prealloc( BUFFER_TYPE eType, int nAddlTris );

private:
	PRESULT AddWall( const VECTOR * pv1, const VECTOR * pv2, const VECTOR * pv3 );
	PRESULT UpdateBand();
public:
	PRESULT AddTri( const VECTOR * pv1, const VECTOR * pv2, const VECTOR * pv3, BOOL bWall );
	PRESULT Finalize();
	PRESULT SetDisplay( const VECTOR * pvPos, const MATRIX * pmRotate, float * pfScale );
	PRESULT GetItemPos( const VECTOR & vWorldPos, VECTOR & vNewPos, const E_RECT & tRect );
	PRESULT SetShowAll( BOOL bShowAll );

	PRESULT RenderGenerate();
	PRESULT RenderDisplay(
		const struct E_RECT & tRect,
		DWORD dwColor,
		float fZFinal,
		float fScale );

	void SetDirty()
	{
		SETBIT( m_dwFlagBits, FLAGBIT_RENDER_DIRTY, TRUE );
		m_tFogOfWar.SetDirty();
	}
};



//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_AutomapDeviceLost();
PRESULT dxC_AutomapDeviceReset();

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


};	// FSSE
#endif // __DX9_PORTAL__
