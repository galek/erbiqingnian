//----------------------------------------------------------------------------
// dxC_common.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_COMMON__
#define __DXC_COMMON__	

#ifdef ENGINE_TARGET_DX10
	//#define DX10_EMULATE_DX9_BEHAVIOR
	#define DX10_GET_RUNNING_HACK
	#define FX10_ATTEMPT_BACKWARDS_COMPATIBLITLY
	#define DX10_DISABLE_FX_LOD
	#define DX10_TGA_HACK

#ifndef DX10_EMULATE_DX9_BEHAVIOR
	#define DX10_SOFT_PARTICLES
#endif

	//#define DX10_DEBUG_REF_LAYER
	//#define DX10_DEBUG_DEVICE_PER_API_CALL
	//#define DX10_SAMPLERS_DEFINED_IN_FX_FILE
	//#define DX10_MODEL_HAS_NO_SHADER_DEF_HACK
	#define DX10_PARTICLES_SET_POINTLIGHTPOS
	//#defined DX10_GPU_PARTICLE_EMITTER
	#define DX10_USE_MRTS
	#define DX10_ATI_MSAA_FIX

	#define DX10_BC5_TEXTURES
	#define DX10_DOF
	//#define DX10_DOF_GATHER

#if ISVERSION(DEVELOPMENT)
	#define DXTLIB
#endif
#endif

//#define DX9_MOST_BUFFERS_MANAGED

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------
#include "e_dll.h"
#include "e_common.h"
#include "e_effect_priv.h"

#include "dxC_types.h"
#include "dxC_enums.h"
#include "dxC_macros.h"
#include "dxC_device.h"
#include "dxC_debug.h"
#include "dxC_effect.h"
#include <dxerr.h>
//#include <dxerr9.h>
#include "e_profile.h"
#include "e_math.h"

#include "dx9_compileflags.h"

//----------------------------------------------------------------------------
// Target specific HEADERS and DEFINES
//----------------------------------------------------------------------------
#include DXCINC_9_10(dx9_common.h, dx10_common.h)

#define EXCLUDE_LIBS 
#include "dxtlib/dxtlib.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// need a better way to specify targets than this?  used in paths, etc. (data\effects\DX9)
#define ENGINE_TARGET_FOLDER			DXC_9_10( "Dx9", "Dx10" )

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

// Drawprim count macros
#define DP_GETPRIMCOUNT_TRISTRIP( nVerts )			( nVerts - 2 )
#define DP_GETPRIMCOUNT_TRILIST( nVerts )			( nVerts / 3 )
#define DP_GETPRIMCOUNT_LINESTRIP( nVerts )			( nVerts - 1 )
#define DP_GETPRIMCOUNT_LINELIST( nVerts )			( nVerts / 2 )

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum QUAD_TYPE
{
	FULLSCREEN_QUAD_UV,
	BASIC_QUAD_XY_PLANE,
	QUAD_TYPE_COUNT,
};

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_DrawFullscreenQuad(
	QUAD_TYPE QuadType,
	D3D_EFFECT* pEffect,
	const EFFECT_TECHNIQUE & tTechnique );

PRESULT dxC_DrawQuad(
	QUAD_TYPE QuadType,
	D3D_EFFECT* pEffect,
	const EFFECT_TECHNIQUE& tTechnique );

PRESULT dxC_DrawPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
	UINT StartVertex,
	UINT PrimitiveCount,
	D3D_EFFECT* pEffect = NULL,
	METRICS_GROUP eGroup = METRICS_GROUP_UNKNOWN,
	UINT iPassIndex = 0 );

PRESULT dxC_DrawIndexedPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY,
	INT BaseVertexIndex,
	UINT MinVertexIndex,
	UINT NumVertices,
	UINT startIndex,
	UINT primCount,
	D3D_EFFECT* pEffect = NULL,
	METRICS_GROUP eGroup = METRICS_GROUP_UNKNOWN,
	UINT iPassIndex = 0 );

PRESULT dxC_DrawIndexedPrimitiveInstanced(
	D3DC_PRIMITIVE_TOPOLOGY,
	INT BaseVertexIndex, 
	UINT NumVertices,
	UINT startIndex,
	UINT primCount,
	UINT nInstances,
	UINT nStreams,
	LPD3DCEFFECT pEffect = NULL,
	METRICS_GROUP eGroup = METRICS_GROUP_UNKNOWN,
	D3DC_TECHNIQUE_HANDLE hTech = NULL,
	UINT iPassIndex = 0 );

// Please don't use this. Ok I won't.  Thanks!  What about me? No not even you.
//void dxC_DrawPrimitiveUP(
//	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
//	UINT PrimitiveCount,
//	CONST void* pVertexStreamZeroData,
//	UINT VertexStreamZeroStride,
//	LPD3DCEFFECT pEffect = NULL,
//	METRICS_GROUP eGroup = METRICS_GROUP_UNKNOWN );

//PRESULT dxC_DrawIndexedPrimitiveUP(
//	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
//	UINT MinVertexIndex,
//	UINT NumVertices,
//	UINT PrimitiveCount,
//	CONST void* pIndexData,
//	DXC_FORMAT IndexDataFormat,
//	CONST void* pVertexStreamZeroData,
//	UINT VertexStreamZeroStride,
//	LPD3DCEFFECT pEffect = NULL,
//	METRICS_GROUP eGroup = METRICS_GROUP_UNKNOWN );

//----------------------------------------------------------------------------
// INLINE
//----------------------------------------------------------------------------

inline void dxC_DPSetPrefetch( BOOL bPrefetch )
{
	extern BOOL gbPrefetchVB;
	gbPrefetchVB = bPrefetch;
}

inline BOOL dxC_DPGetPrefetch()
{
	extern BOOL gbPrefetchVB;
	return gbPrefetchVB;
}

#endif //__DXC_COMMON__
