//----------------------------------------------------------------------------
// dxC_debug.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_DEBUG__
#define __DX9_DEBUG__

#include "dxC_effect.h"
#include "e_debug.h"
#include <map>
#include "memoryallocator_stl.h"

#ifdef DXDUMP_ENABLE
#include "..\\dxdump\\dxdump.h"
#endif // DXDUMP_ENABLE

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define DEBUG_MIP_TEXTURE_COUNT		11
#define MIN_MIP_TEXTURE_SIZE		2
#define MAX_MIP_TEXTURE_SIZE		( MIN_MIP_TEXTURE_SIZE << ( DEBUG_MIP_TEXTURE_COUNT - 1 ) )

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef void (* PFN_DEBUG_SHADER_PROFILE_STATS_CALLBACK)( const struct SHADER_PROFILE_STATS * pStats );

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct DEBUG_MIP_TEXTURE
{
	int nTextureID;
	int nResolution;
};

struct DEBUG_MESH_RENDER_INFO
{
	int nModelID;
	int nMeshIndex;
	int nMaterial;
	EFFECT_TECHNIQUE_REF tFXRef;
	float fDistToEyeSqr;
};

struct SHADER_PROFILE_RENDER
{
	int nEffectID;
	int nTechniqueIndex;
	int nOverdraw;
	DWORD dwBranches;
	HWND hWnd;
	PFN_DEBUG_SHADER_PROFILE_STATS_CALLBACK pfnCallback;
};

struct SHADER_PROFILE_STATS
{
	HWND  hWnd;
	float fFramesPerSecond;
	UINT  nPixelsPerSecond;
};


template< class T >
struct GraphData
{
	int N;
	T * tBuffer;
	int nNext;

	void Init( int nSize )
	{
		N = nSize;
		tBuffer = (T*)MALLOCZ( NULL, N * sizeof(T) );
	}
	void Destroy()
	{
		if ( tBuffer )
		{
			FREE(NULL, tBuffer);
			tBuffer = NULL;
		}
	}

	void Zero()	{ memclear( tBuffer, sizeof(T)*N ); nNext = 0; }
	T Avg()
	{
		T sum = 0;
		for ( int i = 0; i < N; i++ )
		{
			sum += tBuffer[i];
		}
		return sum / N;
	}
	T Max()
	{
		T max = 0;
		for ( int i = 0; i < N; i++ )
		{
			max = MAX( tBuffer[i], max );
		}
		return max;
	}
	void Push( T value )
	{
		tBuffer[ nNext ] = value;
		nNext = ( nNext + 1 ) % N;
	}
};



template< class T >
class LINE_GRAPH_LINE
{
	typedef T element_type;
	GraphData<T> tData;
	element_type tLifePeak;
	element_type tLifeSum;

	DWORD dwColor;
	static const int cnNameLen = 32;
	WCHAR wszName[cnNameLen];

	void Destroy();
public:
	GraphData<T> & Graph()			{ return tData; }
	const T & GetLifetimePeak()		{ return tLifePeak; }
	const T & GetLifetimeSum()		{ return tLifeSum; }
	void SetColor( DWORD dwCol )	{ dwColor = dwCol; }
	DWORD GetColor()				{ return dwColor; }

	void Reset();
	void SetName( const WCHAR * pszName );
	void Update( const element_type & tValue );

	void Init( int nNumElements );

	LINE_GRAPH_LINE()				{ tData.tBuffer = NULL; }
	~LINE_GRAPH_LINE()				{ Destroy(); }
};



template< class T >
class LINE_GRAPH
{
	typedef T							element_type;
	typedef LINE_GRAPH_LINE<T>			line_type;
	typedef std::map<DWORD, line_type, std::less<DWORD>, FSCommon::CMemoryAllocatorSTL<std::pair<DWORD, line_type> > >	line_map;
	typedef typename line_map::iterator line_map_itr;

	line_map*		tLines;

	E_RECT			tRenderRect;

	line_type * GetLine( int nID );

public:

	void Init();
	void Destroy();
	void ResetCounts();
	void SetRect( const E_RECT & tRect );
	PRESULT AddLine( int nID, const WCHAR * pszName, DWORD dwColor );
	PRESULT UpdateLine( int nID, const T & tValue );
	PRESULT Render();
};


//----------------------------------------------------------------------------
// FORWARD_DECLARATIONS
//----------------------------------------------------------------------------

struct D3D_DRAWLIST_COMMAND;
struct D3D_TEXTURE;

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern CArrayIndexed<DEBUG_MIP_TEXTURE> gtDebugMipTextures;
extern BOOL gbDumpDrawList;
extern DEBUG_MESH_RENDER_INFO gtDebugMeshRenderInfo;

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void dx9_DXDumpAll();
void dx9_DXDestroy();
PRESULT dx9_GetDebugMipTextureOverride( D3D_TEXTURE * pTexture, LPD3DCBASETEXTURE * ppTexture );
PRESULT dx9_DumpDrawListBuffer();
PRESULT dx9_DumpQuery( const char * pszEvent, const int nModelID, const int nInt, const void * pPtr );
PRESULT dx9_DumpDrawListCommand( int nDrawList, D3D_DRAWLIST_COMMAND * pCommand );
PRESULT dx9_DebugInit();
PRESULT dx9_DebugDestroy();
BOOL dx9_IsDebugRuntime();

PRESULT dxC_DebugSetShaderProfileRender( const SHADER_PROFILE_RENDER * pSettings );
BOOL dxC_DebugShaderProfileRender();

int dxC_DebugTrackMesh(
	const D3D_MODEL * pModel,
	const D3D_MODEL_DEFINITION * pModelDef );

void dx9_DebugTrackMeshRenderInfo(
	int nModelID,
	int nMeshIndex,
	int nMaterial,
	struct EFFECT_TECHNIQUE_REF & tFXRef,
	float fDistToEyeSqr );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------




#endif // __DX9_DEBUG__