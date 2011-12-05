//----------------------------------------------------------------------------
// dxC_pixo.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_caps.h"
#include "e_optionstate.h"
#include "dxC_texture.h"
#include "dxC_buffer.h"

#include "dxC_pixo.h"


namespace FSSE
{;


#ifdef USE_PIXO

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define FORCE_SHOULD_USE_PIXOMATIC

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// Encompass various states that would prevent rendering
struct PIXO_ALLOW_RENDER
{
	BOOL bMaster;
	BOOL bValidRT;

	inline BOOL AllowRender()
	{
		return bMaster && bValidRT;
	}
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static PIXO_SCREEN_BUFFERS sgtPixoScreenBuffers = {0};

static PIXO_VERT_CACHE* sgpPixoVertexCache = NULL;
static size_t sgnPixoVertexCacheAlloc = 0;
static DWORD sgtPixoVertexStreamCount = 0;
static DWORD sgtPixoVertexStreamStride = 0;
static PIXO_INDEX_DATA sgpPixoIndices = NULL;
static DWORD sgnPixoIndexCount = 0;

static PIXO_ALLOW_RENDER sgtPixoAllowRender = { 0 };

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

BOOL dxC_ShouldUsePixomatic()
{
	BOOL bUsePixo = FALSE;

	if ( dxC_CanUsePixomatic() )
	{
		// Check DX9CAPs
		if ( ! dx9_CapGetValue( DX9CAP_3D_HARDWARE_ACCELERATION ) )
			bUsePixo = TRUE;

#ifdef FORCE_SHOULD_USE_PIXOMATIC
		bUsePixo = TRUE;
#endif
	}

	return bUsePixo;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL dxC_IsPixomaticActive()
{
	if ( ! dxC_CanUsePixomatic() )
		return FALSE;

	BOOL bActive = FALSE;

#if defined(USE_PIXO) && defined(ENGINE_TARGET_DX9)
	if ( FXTGT_FIXED_FUNC == dxC_GetCurrentMaxEffectTarget() )
		bActive = TRUE;
#endif

	return bActive;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline static BOOL sPixoAllowRender()
{
	return sgtPixoAllowRender.AllowRender();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sDestroyBuffers(
	PIXO_SCREEN_BUFFERS * pPixoBuffers )
{
	ASSERT_RETINVALIDARG( pPixoBuffers );

	if(pPixoBuffers->pPixoBuffer)
	{
		PixoBufferClose(pPixoBuffers->pPixoBuffer);
		pPixoBuffers->pPixoBuffer = 0;
	}

	VirtualFree(pPixoBuffers->pZ_buffer_block, 0, MEM_RELEASE);
	pPixoBuffers->pZ_buffer_block = pPixoBuffers->pZBuffer = 0;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sCreateBuffers(
	PIXO_SCREEN_BUFFERS * pPixoBuffers,
	HWND hwnd )
{
	PIXO_BUF *pbuf;

	ASSERT_RETINVALIDARG( pPixoBuffers );

	// make sure we're not leaking anything
	sDestroyBuffers( pPixoBuffers );

	ASSERT_RETFAIL( pPixoBuffers->nBufferWidth  > 0 );
	ASSERT_RETFAIL( pPixoBuffers->nBufferHeight > 0 );

	// Try opening our main pixo buffer.
	pbuf = PixoBufferOpen( hwnd, pPixoBuffers->nBufferWidth, pPixoBuffers->nBufferHeight, pPixoBuffers->nFlags );

	if ( pbuf )
	{
		int zbuffer_pad;
		int zbuffer_size;

		pPixoBuffers->pPixoBuffer = pbuf;

		// Lock the buffer to see what width / height it actually is. For
		// example, if we're in antialias mode our z buffer needs to be
		// app_data->buffer_width * 2, app_data->buffer_height * 2.
		PixoBufferLock(pbuf, 0);

		// From the Intel(r) Pentium(r) 4 and Intel(r) Xeon(tm) Processor
		// Optimization Reference Manual, Order Number: 248966-04
		//
		// 64K aliasing for data - first-level cache. If a reference (load or
		// store) occurs that has bits 0-15 of the linear address, which are
		// identical to a reference (load or store) which is under way, then the
		// second reference cannot begin until the first one is kicked out of the
		// cache. Avoiding this kind of aliasing can lead to a factor of three
		// speedup.
		//
		// User/Source Coding Rule 5. (M impact, M generality) When padding
		// variable declarations to avoid aliasing, the greatest benefit comes
		// from avoiding aliasing on second-level cache lines, suggesting an
		// offset of 128 bytes or more.
		if ( pPixoBuffers->eZBufferType == PIXO_ZBUFFER_TYPE_16 )
		{
			zbuffer_size = pbuf->BufferWidth * pbuf->BufferHeight * sizeof(unsigned short);
			zbuffer_pad = 0;
			pPixoBuffers->pZ_buffer_pitch = pbuf->BufferWidth * sizeof(unsigned short);
		}
		else
		{
			ASSERT_RETFAIL(   pPixoBuffers->eZBufferType == PIXO_ZBUFFER_TYPE_24
						   || pPixoBuffers->eZBufferType == PIXO_ZBUFFER_TYPE_24S );

			// For 24-bit and 32-bit z buffers we pad that bugger out to avoid
			// running into 64k aliasing conflicts with the framebuffer
			zbuffer_size = pbuf->BufferWidth * pbuf->BufferHeight * sizeof(unsigned int);
			zbuffer_pad = ((unsigned int)pbuf->Buffer + 512) & 1023;
			pPixoBuffers->pZ_buffer_pitch = pbuf->BufferWidth * sizeof(unsigned int);
		}

		// alloc our zbuffer block
		pPixoBuffers->pZ_buffer_block = (unsigned short *)VirtualAlloc(NULL,
			zbuffer_size + zbuffer_pad, MEM_COMMIT, PAGE_READWRITE);

		// if it succeeded, calculate our offset ZBuffer pointer
		if ( pPixoBuffers->pZ_buffer_block )
			pPixoBuffers->pZBuffer = (char *)pPixoBuffers->pZ_buffer_block + zbuffer_pad;

		PixoSetZBufferType( pPixoBuffers->eZBufferType  );

		// Unlock the buffer
		PixoBufferUnlock(pbuf);
	}

	// error handling
	ASSERT_DO( pbuf && pPixoBuffers->pZBuffer )
	{
		sDestroyBuffers( pPixoBuffers );

		ASSERT_MSG( "Pixomatic buffer creation failed!" );
		return E_FAIL;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sInitializePixoScreenBuffers()
{
	const OptionState * pState = e_OptionState_GetActive();
	sgtPixoScreenBuffers.nBufferWidth  = pState->get_nFrameBufferWidth();
	sgtPixoScreenBuffers.nBufferHeight = pState->get_nFrameBufferHeight();
	sgtPixoScreenBuffers.nFlags = 0;
	sgtPixoScreenBuffers.eZBufferType = PixoGetZBufferType();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoInitialize( HWND hWnd )
{
	ASSERT_RETINVALIDARG( hWnd );

	ASSERTX_RETFAIL( PixoStartup(PIXO_ZCLIPRANGE_0_1), "Pixo startup failed!" )

	{
		// Set the floating-point state to what the rasterizer prefers. SSE underflow
		// can produce a performance slowdown, especially since it produces denormal
		// numbers. If FTZ is set, however, underflowed results are just converted to
		// zeroes, with no  performance penalty.
		PIXO_CPU_FEATURES cpu_features;
		PixoGetCPUIDFeatures(&cpu_features, 0);

		if(cpu_features.features & PIXO_FEATURE_SSEFP)
		{
			int mscsr_val;

			__asm stmxcsr [mscsr_val]
			__asm or [mscsr_val], 0x8000    // _MM_FLUSH_ZERO_ON
			__asm ldmxcsr [mscsr_val]
		}
	}

	// Default to 16 bit z buffer.
	PixoSetZBufferType(PIXO_ZBUFFER_TYPE_16);

	sInitializePixoScreenBuffers();
	V_RETURN( sCreateBuffers( &sgtPixoScreenBuffers, hWnd ) );

	sgtPixoAllowRender.bMaster = TRUE;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoResizeBuffers( HWND hWnd )
{
	ASSERT_RETINVALIDARG( hWnd );

	V( sDestroyBuffers( &sgtPixoScreenBuffers ) );
	sInitializePixoScreenBuffers();
	return sCreateBuffers( &sgtPixoScreenBuffers, hWnd );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoShutdown()
{
	if ( sgpPixoVertexCache )
	{
		FREE( NULL, sgpPixoVertexCache );
		sgpPixoVertexCache = NULL;
	}
	sDestroyBuffers( &sgtPixoScreenBuffers );
	PixoShutdown();
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//// This function works off of ptSysmemTexture only.
//PRESULT dxC_PixoConvertTextureToValidFormat( D3D_TEXTURE * pTexture )
//{
//	// ensure texture layer has been loaded and needs conversion
//
//
///*
//Actually, just change dxC_Create2DSysMemTextureFromFileInMemory and do it there.
//*/
//
//
//	//ASSERT_RETINVALIDARG( pTexture );
//	//ASSERT_RETFAIL( pTexture->dwFlags & TEXTURE_FLAG_SYSMEM );
//	//ASSERT_RETFAIL( pTexture->ptSysmemTexture );
//
//	//SYSMEM_TEXTURE *& pSrcTexture = pTexture->ptSysmemTexture;
//	//SYSMEM_TEXTURE * pDestTexture = NULL;
//
//	//int nLevels = pTexture->ptSysmemTexture->GetLevelCount();
//	//DXC_FORMAT tFmt = PIXO_TEXTURE_FORMAT;
//
//	//int nWidth  = pTexture->ptSysmemTexture->nWidth;
//	//int nHeight = pTexture->ptSysmemTexture->nHeight;
//	//size_t nSize = 0;
//	//BOUNDED_WHILE( 1, 1000 )
//	//{
//	//	nSize += dxC_GetTextureLevelSize( nWidth, nHeight, tFmt );
//	//	if ( nWidth == 1 && nHeight == 1 )
//	//		break;
//	//	nWidth  = max( 1, nWidth  >> 1 );
//	//	nHeight = max( 1, nHeight >> 1 );
//	//}
//
//	//void * pARGB8Bits = MALLOC( NULL, nSize );
//	//ASSERT_RETVAL( pARGB8Bits, E_OUTOFMEMORY );
//
//	//// allocate new texture layer
//	//// for each block
//	//// use squish to decompress each block into new texture
//	//// free old texture layer and swap pointers
//
//	//FREE( NULL, pTexture->ptSysmemTexture->pbData );
//
//	return E_NOTIMPL;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetStreamDefinition( PIXO_VERTEX_DECL pDecl )
{
	PixoSetStreamDefinition( pDecl );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetTexture( DWORD dwStage, D3D_TEXTURE * pTexture )
{
	ASSERT_RETINVALIDARG( dwStage <= 2 );

	PRESULT pr = E_FAIL;

	if ( ! pTexture )
	{
		PixoSetTexture( (int)dwStage, PIXO_TEX_FORMAT_NONE, 0, 0, NULL );
		pr = S_OK;
	}
	else
	{
		D3DLOCKED_RECT tRect;
		V_SAVE_GOTO( pr, err, dxC_MapSystemMemTexture( pTexture->ptSysmemTexture, 0, &tRect ) );

		PIXO_TEX_FORMAT tPixoFmt = PIXO_TEX_FORMAT_NONE;
		if ( dx9_IsEquivalentTextureFormat( (DXC_FORMAT)pTexture->nFormat, D3DFMT_A8R8G8B8 ) )
			tPixoFmt = PIXO_TEX_FORMAT_ARGB8888;
		ASSERT_GOTO( tPixoFmt == PIXO_TEX_FORMAT_ARGB8888, err );

		PixoSetPolygonMipMap( PIXO_POLYGONMIPMAP_ON );
		//PixoSetTexture( (int)dwStage, tPixoFmt, pTexture->nWidthInPixels, pTexture->nHeight, tRect.pBits );
		PixoBeginTextureSpecification( (int)dwStage, tPixoFmt, pTexture->ptSysmemTexture->nWidth, pTexture->ptSysmemTexture->nHeight, pTexture->ptSysmemTexture->nLevels );
		for ( int i = 0; i < pTexture->ptSysmemTexture->nLevels; ++i )
		{
			PixoSetMipImage( (int)dwStage, i, pTexture->ptSysmemTexture->GetLevel( i ) );
		}
		PixoEndTextureSpecification( (int)dwStage );
		pr = S_OK;
	}

err:
	if ( pTexture && pTexture->ptSysmemTexture )
		dxC_UnmapSystemMemTexture( pTexture->ptSysmemTexture, 0 );
	if ( FAILED(pr) )
		dxC_PixoSetTexture( dwStage, NULL );
	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetTexture( TEXTURE_TYPE eType, D3D_TEXTURE * pTexture )
{
	if ( eType == TEXTURE_NONE || pTexture == NULL )
		return S_FALSE;
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eType, NUM_TEXTURE_TYPES ) );

	DWORD dwStage;
	switch (eType)
	{
	case TEXTURE_DIFFUSE:
		dwStage = 1;
		break;
	default:
		return S_FALSE;
	}

	return dxC_PixoSetTexture( dwStage, pTexture );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetStream(
	DWORD dwStream,
	const PIXO_VERTEX_DATA pData,
	DWORD dwOffsetInBytes,
	DWORD dwVertexSize,
	DWORD dwVertexCount )
{
	ASSERT_RETINVALIDARG( dwStream <= DXC_MAX_VERTEX_STREAMS );

	PIXO_VERTEX_DATA pVertexData = (PIXO_VERTEX_DATA)( (char*)pData + dwOffsetInBytes );

	PixoSetStream( (int)dwStream, pVertexData );
	sgtPixoVertexStreamCount = dwVertexCount;
	sgtPixoVertexStreamStride = dwVertexSize;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetViewport( const D3DC_VIEWPORT * ptVP )
{
	ASSERT_RETINVALIDARG( ptVP );
	PixoSetViewport( ptVP->TopLeftX, ptVP->TopLeftY, ptVP->Width, ptVP->Height );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetIndices( const PIXO_INDEX_DATA pIndexData, DWORD nIndexCount, DWORD nIndexSize )
{
	ASSERTX_RETFAIL( nIndexSize == 2, "Indices other than 16-bytes aren't supported by Pixomatic!" );

	sgpPixoIndices = pIndexData;
	sgnPixoIndexCount = nIndexCount;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoGetBufferDimensions( int & nWidth, int & nHeight )
{
	nWidth  = sgtPixoScreenBuffers.nBufferWidth;
	nHeight = sgtPixoScreenBuffers.nBufferHeight;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoGetBufferFormat( DXC_FORMAT & tFormat )
{
	tFormat = D3DFMT_A8R8G8B8;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sResizePixoVertexCache()
{
	// look at current stream settings
	size_t nResizeTo = sgtPixoVertexStreamCount * sizeof(PIXO_VERT_CACHE);

	if ( sgnPixoVertexCacheAlloc < nResizeTo )
	{
		sgpPixoVertexCache = (PIXO_VERT_CACHE*)REALLOC( NULL, sgpPixoVertexCache, nResizeTo );
		sgnPixoVertexCacheAlloc = nResizeTo;
		if ( ! sgpPixoVertexCache )
		{
			sgnPixoVertexCacheAlloc = 0;
			return E_OUTOFMEMORY;
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PIXO_PRIMITIVETYPES sGetPrimitiveType( D3DC_PRIMITIVE_TOPOLOGY PrimitiveType )
{
	switch ( PrimitiveType )
	{
	case D3DPT_TRIANGLELIST:
		return PIXO_TRIANGLELIST;
	case D3DPT_TRIANGLESTRIP:
		return PIXO_TRIANGLESTRIP;
	case D3DPT_LINELIST:
		return PIXO_LINELIST;
	case D3DPT_LINESTRIP:
		return PIXO_LINESTRIP;
	}

	return PIXO_TRIANGLESTRIP;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static DWORD sGetIndexCountFromPrimitiveCount( D3DC_PRIMITIVE_TOPOLOGY PrimitiveType, DWORD dwPrimCount )
{
	switch ( PrimitiveType )
	{
	case D3DPT_TRIANGLELIST:
		return dwPrimCount * 3;
	case D3DPT_TRIANGLESTRIP:
		return dwPrimCount + 2;
	case D3DPT_LINELIST:
		return dwPrimCount * 2;
	case D3DPT_LINESTRIP:
		return dwPrimCount + 1;
	}

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoDrawIndexedPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
	DWORD VertexStart,
	DWORD VertexCount,
	DWORD StartIndex,
	DWORD PrimCount )
{
	if ( ! sPixoAllowRender() )
		return S_FALSE;

	// Clear the floating point control word and SSE MXCSR status bits so that Pixo doesn't complain.
	_clearfp();

	// resize pixo vertex cache
	V_RETURN( sResizePixoVertexCache() );

	PixoSetVertexCache( sgpPixoVertexCache, sgnPixoVertexCacheAlloc / sizeof(PIXO_VERT_CACHE) );

	PixoTransformVertices( sgpPixoVertexCache, VertexStart, VertexCount );

	const PIXO_INDEX_DATA pIndices = sgpPixoIndices + StartIndex;
	DWORD dwIndexCount = sGetIndexCountFromPrimitiveCount( PrimitiveType, PrimCount );
	ASSERT_RETFAIL( dwIndexCount <= sgnPixoIndexCount );

	PixoIndexedDrawPrimitiveStream(
		sGetPrimitiveType( PrimitiveType ),
		pIndices,
		dwIndexCount );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoDrawPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
	DWORD PrimitiveCount )
{
	if ( ! sPixoAllowRender() )
		return S_FALSE;

	// Clear the floating point control word and SSE MXCSR status bits so that Pixo doesn't complain.
	_clearfp();

	// For non-indexed primitives, the vertex count is the same as the theoretical index count.
	DWORD dwVertexCount = sGetIndexCountFromPrimitiveCount( PrimitiveType, PrimitiveCount );

	PixoDrawPrimitiveStream(
		sGetPrimitiveType( PrimitiveType ),
		dwVertexCount );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoClearBuffers(
	DWORD dwClearFlags,
	DWORD tColor,
	float fZ,
	unsigned int nStencil )
{
	if ( ! sPixoAllowRender() )
		return S_FALSE;

	DWORD dwPixoFlags = 0;
	if ( dwClearFlags & D3DCLEAR_TARGET )
		dwPixoFlags |= PIXO_CLEARFRAMEBUFFER;
	if ( dwClearFlags & D3DCLEAR_ZBUFFER )
		dwPixoFlags |= PIXO_CLEARZBUFFER;
	if ( dwClearFlags & D3DCLEAR_STENCIL )
		dwPixoFlags |= PIXO_CLEARSTENCIL;

	PixoClearViewport( dwPixoFlags, tColor, fZ, nStencil );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoResetState()
{
	PixoSetACompare( PIXO_ACOMPARE_GT );
	PixoSetAlphaRefValue( 0 );
	PixoSetBottomUpRendering( PIXO_BOTTOMUPRENDERING_ON );
	PixoSetColorMask( PIXO_COLORMASK_RED | PIXO_COLORMASK_GREEN | PIXO_COLORMASK_BLUE );
	PixoSetCullingPolarityToggle( PIXO_CULLINGPOLARITYTOGGLE_OFF );
	PixoSetCullMode( PIXO_CULL_CCW );
	PixoSetDepthRange( 0.f, 1.f );
	PixoSetFastZSPretest( PIXO_FASTZSPRETEST_OFF );
	PixoSetWireFrame( PIXO_WIREFRAME_OFF, 0 );
	PixoSetZCompare( PIXO_ZCOMPARE_LE );
	PixoSetZPass( PIXO_ZPASS_NONE );
	PixoSetZFail( PIXO_ZFAIL_NONE );
	PixoSetZWrite( PIXO_ZWRITE_ON );

	PixoZEnable();
	PixoStencilDisable();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetRenderState( DWORD dwState, DWORD dwValue )
{
	switch ( dwState )
	{
	case D3DRS_ALPHAFUNC:
		{
			switch ( dwValue )
			{
			case D3DCMP_ALWAYS:			PixoSetACompare( PIXO_ACOMPARE_ALWAYS );	break;
			case D3DCMP_EQUAL:			PixoSetACompare( PIXO_ACOMPARE_EQ );		break;
			case D3DCMP_GREATER:		PixoSetACompare( PIXO_ACOMPARE_GT );		break;
			case D3DCMP_GREATEREQUAL:	PixoSetACompare( PIXO_ACOMPARE_GE ); 		break;
			case D3DCMP_LESS:			PixoSetACompare( PIXO_ACOMPARE_LT ); 		break;
			case D3DCMP_LESSEQUAL:		PixoSetACompare( PIXO_ACOMPARE_LE ); 		break;
			case D3DCMP_NOTEQUAL:		PixoSetACompare( PIXO_ACOMPARE_NE ); 		break;
			// No Pixo "never" alpha test state, so try to simulate with < 0
			case D3DCMP_NEVER:			PixoSetACompare( PIXO_ACOMPARE_LT ); 		PixoSetAlphaRefValue(0);		break;
			default:
				return S_FALSE;
			}
			return S_OK;
		}
	case D3DRS_ALPHAREF:
		{
			PixoSetAlphaRefValue( dwValue );
			return S_OK;
		}
	case D3DRS_ALPHABLENDENABLE:
		{
			if ( dwValue )
				PixoBlendEnable();
			else
				PixoBlendDisable();
			return S_OK;
		}
	case D3DRS_ALPHATESTENABLE:
		{
			if ( dwValue )
				PixoACompareEnable();
			else
				PixoACompareDisable();
			return S_OK;
		}
	case D3DRS_COLORWRITEENABLE:
		{
			DWORD dwPixoValue = 0;
			dwPixoValue |= (dwValue & D3DCOLORWRITEENABLE_ALPHA) ? PIXO_COLORMASK_ALPHA : 0;
			dwPixoValue |= (dwValue & D3DCOLORWRITEENABLE_RED  ) ? PIXO_COLORMASK_RED   : 0;
			dwPixoValue |= (dwValue & D3DCOLORWRITEENABLE_GREEN) ? PIXO_COLORMASK_GREEN : 0;
			dwPixoValue |= (dwValue & D3DCOLORWRITEENABLE_BLUE ) ? PIXO_COLORMASK_BLUE  : 0;
			PixoSetColorMask( dwPixoValue );
			return S_OK;
		}
	case D3DRS_CULLMODE:
		{
			switch ( dwValue )
			{
			case D3DCULL_CCW:		PixoSetCullMode( PIXO_CULL_CCW );		break;
			case D3DCULL_CW:		PixoSetCullMode( PIXO_CULL_CW );		break;
			case D3DCULL_NONE:		PixoSetCullMode( PIXO_CULL_NONE );		break;
			default:
				return S_FALSE;
			}
			return S_OK;
		}
	case D3DRS_FILLMODE:
		{
			switch ( dwValue )
			{
			case D3DFILL_SOLID:			PixoSetWireFrame( PIXO_WIREFRAME_OFF, 0 );				break;
			case D3DFILL_WIREFRAME:		PixoSetWireFrame( PIXO_WIREFRAME_ON,  0xff00ffff );		break;
			default:
				return S_FALSE;
			}
			return S_OK;
		}
	case D3DRS_FOGCOLOR:
		{
			PixoSetFogColor( (PIXO_ARGB0)dwValue );
			return S_OK;
		}
	case D3DRS_FOGENABLE:
		{
			if ( dwValue )
				PixoSetFog( PIXO_FOG_ON );
			else
				PixoSetFog( PIXO_FOG_OFF );
			return S_OK;
		}
	case D3DRS_SCISSORTESTENABLE:
		{
			// Right now, Scissor is not supported in Pixo
			//if ( dwValue )
			//	PixoScissorEnable();
			//else
				PixoStencilDisable();
			return S_OK;
		}
	case D3DRS_SHADEMODE:
		{
			switch( dwValue )
			{
			case D3DSHADE_FLAT:		PixoSetShadeMode( PIXO_SHADEMODE_FLAT );		break;
			case D3DSHADE_GOURAUD:	PixoSetShadeMode( PIXO_SHADEMODE_GOURAUD );		break;
			default:
				return S_FALSE;
			}
			return S_OK;
		}
	case D3DRS_ZENABLE:
		{
			if ( dwValue )
				PixoZEnable();
			else
				PixoZDisable();
			return S_OK;
		}
	case D3DRS_ZFUNC:
		{
			switch ( dwValue )
			{
			case D3DCMP_ALWAYS:			PixoSetZCompare( PIXO_ZCOMPARE_ALWAYS );	break;
			case D3DCMP_EQUAL:			PixoSetZCompare( PIXO_ZCOMPARE_EQ );		break;
			case D3DCMP_GREATER:		PixoSetZCompare( PIXO_ZCOMPARE_GT );		break;
			case D3DCMP_GREATEREQUAL:	PixoSetZCompare( PIXO_ZCOMPARE_GE ); 		break;
			case D3DCMP_LESS:			PixoSetZCompare( PIXO_ZCOMPARE_LT ); 		break;
			case D3DCMP_LESSEQUAL:		PixoSetZCompare( PIXO_ZCOMPARE_LE ); 		break;
			case D3DCMP_NOTEQUAL:		PixoSetZCompare( PIXO_ZCOMPARE_NE ); 		break;
			case D3DCMP_NEVER:			return E_NOTIMPL;
			default:
				return S_FALSE;
			}
			return S_OK;
		}
	case D3DRS_ZWRITEENABLE:
		{
			if ( dwValue )
				PixoSetZWrite( PIXO_ZWRITE_ON );
			else
				PixoSetZWrite( PIXO_ZWRITE_OFF );
			return S_OK;
		}
	}

	return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetWorldMatrix( const MATRIX * pMat )
{
	PixoSetWorldTransformMatrix( (const float*)pMat );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetViewMatrix( const MATRIX * pMat )
{
	PixoSetViewTransformMatrix( (const float*)pMat );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetProjectionMatrix( const MATRIX * pMat )
{
	PixoSetProjectionTransformMatrix( (const float*)pMat );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoUpdateDebugStates()
{
	static unsigned char scCol = 0;
	scCol += 2;
	DWORD dwCol = 0xff000000 | ((unsigned int)scCol << 16) | ((unsigned int)scCol);
	//PixoSetStageConstant( 0, dwCol );

	PixoSetTFactor( dwCol );

	BOOL bUseTexture = TRUE;

	if ( bUseTexture )
	{
		PixoSetColorOp( 0, PIXO_ARG_TFACTOR, PIXO_OP_SELECTARGA, PIXO_ARG_TFACTOR );
		PixoSetAlphaOp( 0, PIXO_ARG_TFACTOR, PIXO_OP_SELECTARGA, PIXO_ARG_TFACTOR );
		//PixoSetColorOp( 0, PIXO_ARG_TEXTURE, PIXO_OP_SELECTARGA, PIXO_ARG_TEXTURE );
		//PixoSetAlphaOp( 0, PIXO_ARG_TFACTOR, PIXO_OP_SELECTARGA, PIXO_ARG_TFACTOR );
		PixoSetColorOp( 1, PIXO_ARG_TEXTURE, PIXO_OP_SELECTARGA, PIXO_ARG_TEXTURE );
		PixoSetAlphaOp( 1, PIXO_ARG_TFACTOR, PIXO_OP_SELECTARGA, PIXO_ARG_TFACTOR );
	}
	else
	{
		PixoSetTexture( 0, PIXO_TEX_FORMAT_NONE, 0, 0, NULL );
		PixoSetColorOp( 0, PIXO_ARG_TFACTOR, PIXO_OP_SELECTARGA, PIXO_ARG_TFACTOR );
		PixoSetAlphaOp( 0, PIXO_ARG_TFACTOR, PIXO_OP_SELECTARGA, PIXO_ARG_TFACTOR );
		PixoSetColorOp( 1, PIXO_ARG_CURRENT, PIXO_OP_DISABLE, PIXO_ARG_CURRENT );
		PixoSetAlphaOp( 1, PIXO_ARG_CURRENT, PIXO_OP_DISABLE, PIXO_ARG_CURRENT );
	}

	//PixoSetColorOp( 1, PIXO_ARG_CURRENT, PIXO_OP_DISABLE, PIXO_ARG_CURRENT );
	//PixoSetAlphaOp( 1, PIXO_ARG_CURRENT, PIXO_OP_DISABLE, PIXO_ARG_CURRENT );
	PixoSetColorOp( 2, PIXO_ARG_CURRENT, PIXO_OP_DISABLE, PIXO_ARG_CURRENT );
	PixoSetAlphaOp( 2, PIXO_ARG_CURRENT, PIXO_OP_DISABLE, PIXO_ARG_CURRENT );

	PixoSetSrcBlend(PIXO_BLEND_ONE);
	PixoSetDestBlend(PIXO_BLEND_ZERO);

	PixoSetStageDestination(0, PIXO_STAGE_DESTINATION_CURRENT);
	PixoSetStageDestination(1, PIXO_STAGE_DESTINATION_CURRENT);

	// FOG IS BROKEN!
	PixoSetFog( PIXO_FOG_OFF );
	PixoACompareDisable();
	//PixoZDisable();
	PixoBlendDisable();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetAllowRender( BOOL bAllowRender )
{
	sgtPixoAllowRender.bMaster = bAllowRender;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetRenderTarget( RENDER_TARGET_INDEX eRT )
{
	BOOL bValid = TRUE;

	if ( eRT == RENDER_TARGET_INVALID || eRT == RENDER_TARGET_NULL )
		bValid = FALSE;

	if ( dxC_GetRenderTargetSwapChainIndex( eRT ) != DEFAULT_SWAP_CHAIN )
		bValid = FALSE;

	if ( dxC_GetRenderTargetIsBackbuffer( eRT ) != TRUE )
		bValid = FALSE;

	sgtPixoAllowRender.bValidRT = bValid;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoSetDepthTarget( DEPTH_TARGET_INDEX eDT )
{
	BOOL bValid = TRUE;

	if ( eDT == DEPTH_TARGET_INVALID || eDT == DEPTH_TARGET_NONE )
		bValid = FALSE;

	if ( dxC_GetDepthTargetSwapChainIndex( eDT ) != DEFAULT_SWAP_CHAIN )
		bValid = FALSE;

	if ( bValid )
		PixoSetZBuffer( sgtPixoScreenBuffers.pZBuffer, sgtPixoScreenBuffers.pZ_buffer_pitch, sgtPixoScreenBuffers.eZBufferType );
	else
		PixoSetZBuffer( NULL, 0, sgtPixoScreenBuffers.eZBufferType );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoBeginScene()
{
	PIXO_BUF * pPixoBuf = sgtPixoScreenBuffers.pPixoBuffer;
	ASSERT_RETFAIL( pPixoBuf );

	// Clear the floating point control word and SSE MXCSR status bits so that Pixo doesn't complain.
	_clearfp();

	// lock the buffer
	PixoBufferLock( pPixoBuf, 0 );

	// set our viewport
	PixoSetViewport( 0, 0, pPixoBuf->BufferWidth, pPixoBuf->BufferHeight );

	// set framebuffer data
	PixoSetFrameBuffer( pPixoBuf->Buffer, pPixoBuf->BufferPitch, pPixoBuf->BufferWidth, pPixoBuf->BufferHeight );

	// set zbuffer information
	PixoSetZBuffer( sgtPixoScreenBuffers.pZBuffer, sgtPixoScreenBuffers.pZ_buffer_pitch, sgtPixoScreenBuffers.eZBufferType );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoBeginPixelAccurateScene()
{
	PIXO_BUF * pPixoBuf = sgtPixoScreenBuffers.pPixoBuffer;
	ASSERT_RETFAIL( pPixoBuf );

	// If we're PIXO_BUF_2XANTIALIAS or PIXO_BUF_2XZOOM'ing the
	// PixoBufferLock(..., PIXO_BUF_LOCK_COMPOSITE) call will scale the
	// framebuffer data down / up to match the g_app_data.buffer_width
	// and g_app_data.buffer_height sizes. If we aren't running in
	// those modes then this call does a whole lotta nothing.
	PixoBufferUnlock( pPixoBuf );
	PixoBufferLock( pPixoBuf, PIXO_BUF_LOCK_COMPOSITE );
	PixoSetFrameBuffer( pPixoBuf->Buffer, pPixoBuf->BufferPitch, pPixoBuf->BufferWidth, pPixoBuf->BufferHeight );

	// Udate our viewport if it has changed via the above lock call
	PixoSetViewport( 0, 0, pPixoBuf->BufferWidth, pPixoBuf->BufferHeight );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoEndScene()
{
	PIXO_BUF * pPixoBuf = sgtPixoScreenBuffers.pPixoBuffer;
	ASSERT_RETFAIL( pPixoBuf );

	// Unlock
	PixoBufferUnlock( pPixoBuf );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PixoPresent()
{
	PIXO_BUF * pPixoBuf = sgtPixoScreenBuffers.pPixoBuffer;
	ASSERT_RETFAIL( pPixoBuf );

	// Blit to the screen
	PixoBufferBlit( pPixoBuf, 0, 0, 0 );

	return S_OK;
}

#else // not USE_PIXO


PRESULT dxC_PixoSetStreamDefinition(PIXO_VERTEX_DECL)				{ return E_NOTIMPL; }
BOOL dxC_ShouldUsePixomatic()										{ return FALSE; }
PRESULT dxC_PixoInitialize(HWND)									{ return E_NOTIMPL; }
PRESULT dxC_PixoResizeBuffers(HWND)									{ return E_NOTIMPL; }
PRESULT dxC_PixoShutdown()											{ return E_NOTIMPL; }
PRESULT dxC_PixoSetTexture(DWORD, D3D_TEXTURE*)						{ return E_NOTIMPL; }
PRESULT dxC_PixoSetTexture(TEXTURE_TYPE, D3D_TEXTURE *)				{ return E_NOTIMPL; }
PRESULT dxC_PixoSetViewport(const D3DC_VIEWPORT*)					{ return E_NOTIMPL; }
PRESULT dxC_PixoGetBufferDimensions(int&, int&)						{ return E_NOTIMPL; }
PRESULT dxC_PixoGetBufferFormat(DXC_FORMAT&)						{ return E_NOTIMPL; }
PRESULT dxC_PixoSetIndices(const PIXO_INDEX_DATA, DWORD, DWORD)		{ return E_NOTIMPL; }
PRESULT dxC_PixoSetStream(
	DWORD,
	const PIXO_VERTEX_DATA,
	DWORD,
	DWORD,
	DWORD)															{ return E_NOTIMPL; }
PRESULT dxC_PixoDrawPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY,
	DWORD)															{ return E_NOTIMPL; }
PRESULT dxC_PixoDrawIndexedPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY,
	DWORD,
	DWORD,
	DWORD,
	DWORD)															{ return E_NOTIMPL; }
PRESULT dxC_PixoClearBuffers(
	DWORD,
	DWORD,
	float,
	unsigned int)													{ return E_NOTIMPL; }
PRESULT dxC_PixoResetState()										{ return E_NOTIMPL; }
PRESULT dxC_PixoSetWorldMatrix(const MATRIX *)						{ return E_NOTIMPL; }
PRESULT dxC_PixoSetViewMatrix(const MATRIX *)						{ return E_NOTIMPL; }
PRESULT dxC_PixoSetProjectionMatrix(const MATRIX *)					{ return E_NOTIMPL; }
PRESULT dxC_PixoSetRenderState(DWORD, DWORD)						{ return E_NOTIMPL; }
PRESULT dxC_PixoUpdateDebugStates()									{ return E_NOTIMPL; }
PRESULT dxC_PixoSetAllowRender(BOOL)								{ return E_NOTIMPL; }
PRESULT dxC_PixoSetRenderTarget(RENDER_TARGET_INDEX)				{ return E_NOTIMPL; }
PRESULT dxC_PixoSetDepthTarget(DEPTH_TARGET_INDEX)					{ return E_NOTIMPL; }
PRESULT dxC_PixoBeginScene()										{ return E_NOTIMPL; }
PRESULT dxC_PixoBeginPixelAccurateScene()							{ return E_NOTIMPL; }
PRESULT dxC_PixoEndScene()											{ return E_NOTIMPL; }
PRESULT dxC_PixoPresent()											{ return E_NOTIMPL; }

#endif // USE_PIXO

}; // namespace FSSE


