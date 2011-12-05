//----------------------------------------------------------------------------
// dxC_statemanager.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_STATEMANAGER__
#define __DXC_STATEMANAGER__

#ifdef ENGINE_TARGET_DX9
	#include "dx9_statemanager.h"
#elif defined(ENGINE_TARGET_DX10)
	#include "dx10_statemanager.h"
#endif

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define EFFECT_STATE_MANAGER_STATS_LEN	1024


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum STATE_MANAGER_TYPE
{
	STATE_MANAGER_INVALID = -1,
	STATE_MANAGER_TRACK = 0,
	STATE_MANAGER_OPTIMIZE,
	NUM_STATE_MANAGER_TYPES
};

enum STATE_TYPE
{
	ST_RENDERSTATE,
	ST_TSS,
	ST_NPATCH,
	ST_SAMPLER,
	ST_VERTEXSHADER,
	ST_PIXELSHADER,
};

enum STATE_SET_TYPE
{
	STATE_LIGHT_ENABLE,
	STATE_SET_FVF,
	STATE_SET_LIGHT,
	STATE_SET_MATERIAL,
	STATE_SET_NPATCH_MODE,
	STATE_SET_PIXEL_SHADER,
	STATE_SET_PIXEL_SHADER_CONSTANT_B,
	STATE_SET_PIXEL_SHADER_CONSTANT_F,
	STATE_SET_PIXEL_SHADER_CONSTANT_I,
	STATE_SET_RENDER_STATE,
	STATE_SET_SAMPLER_STATE,
	STATE_SET_TEXTURE,
	STATE_SET_TEXTURE_STAGE_STATE,
	STATE_SET_TRANSFORM,
	STATE_SET_VERTEX_SHADER,
	STATE_SET_VERTEX_SHADER_CONSTANT_B,
	STATE_SET_VERTEX_SHADER_CONSTANT_F,
	STATE_SET_VERTEX_SHADER_CONSTANT_I,
	STATE_SET_VERTEX_DECLARATION,
	STATE_SET_STREAM_SOURCE,
	STATE_SET_INDICES,
	NUM_CACHEABLES
};
//--------------------------------------------------------------------------------------
//STRUCTS
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// CLASSES
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Base implementation of a custom ID3DXEffectStateManager interface
//--------------------------------------------------------------------------------------
class CStateManagerInterface  : public DX9_BLOCK( ID3DXEffectStateManager ) DX10_BLOCK( ID3D10StateMapper )
{
public:

	//virtual void Init() = 0;
	virtual void Destroy()
	{
		// must be called
		DX9_BLOCK( Release(); )
	};

#if defined(ENGINE_TARGET_DX9)
	//virtual ~CStateManagerInterface() {};       // virtual destructor, for cleanup purposes
	virtual void DirtyCachedValues() = 0;       // Cause any cached state change values to be invalid
	virtual void ResetStats() = 0;				// Clear once per frame statistics
	virtual LPWSTR EndFrameStats() = 0;         // Called once per frame retrieve statistics
	virtual void GetStateStats( STATE_SET_TYPE eStat, UINT & nActual, UINT & nTotal ) = 0;

	// Additional non-effect states that we want to be able to track or cache through the manager
	STDMETHOD(SetVertexDeclaration)( LPD3DCIALAYOUT pVertDecl ) = 0;
	STDMETHOD(SetStreamSource)( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, UINT Stride ) = 0;
	STDMETHOD(SetIndices)( LPD3DCIB pIndexData ) = 0;

	virtual void DirtyCachedRenderState( D3DRENDERSTATETYPE d3dRenderState ) {};
	virtual void DirtyCachedSamplerState( DWORD dwStage, D3DSAMPLERSTATETYPE d3dSamplerState ) {};
	virtual void DirtyCachedTextureStageState( DWORD dwStage, D3DTEXTURESTAGESTATETYPE d3dTextureStageState ) {};
#endif

	// Create a state manager interface for the device
	static CStateManagerInterface* Create( STATE_MANAGER_TYPE eType );
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern int gnStateStatsTotalCalls;
extern int gnStateStatsNonEffectCalls;
extern class CStateManagerInterface * gpStateManager;
extern STATE_MANAGER_TYPE geStateManagerType;

#endif //__DXC_STATEMANAGER__
