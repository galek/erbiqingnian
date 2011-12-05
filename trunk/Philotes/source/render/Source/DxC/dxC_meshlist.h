//----------------------------------------------------------------------------
// dxC_meshlist.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_MESHLIST__
#define __DXC_MESHLIST__

#include "safe_vector.h"
#include "safe_map.h"
#include "list.h"
#include "e_frustum.h"
#include "dxC_render.h"
#include "dxC_drawlist.h"
#include "dxC_state.h"


namespace FSSE
{
//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------


typedef PList<MeshDraw>							MeshDrawBucket;
typedef safe_vector<struct MeshDrawSortable>	MeshList;
typedef safe_vector<MeshList>					MeshListVector;

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


struct MeshDraw
{
	enum FLAGBITS
	{
		FLAGBIT_FORCE_CURRENT_ENV = 0,
		FLAGBIT_FORCE_NO_SCISSOR,
		FLAGBIT_DEPTH_ONLY,
		FLAGBIT_BEHIND,
	};

	DWORD							dwFlagBits;
	D3D_MESH_DEFINITION*			pMesh;
	D3D_MODEL*						pModel;
	D3D_MODEL_DEFINITION*			pModelDef;
	int								nMeshIndex;
	int								nLOD;
	ENVIRONMENT_DEFINITION*			pEnvDef;
	MATRIX							matWorld;
	MATRIX							matView;
	MATRIX							matProj;
	MATRIX							matWorldViewProj;	
	VECTOR4							vEyeInWorld;
	float							fEffectTransitionStr;
	//MODEL_RENDER_LIGHT_DATA			tLights;
	//int								nParticleSystemId;
	float*							pfBones;
	E_RECT							tScissor;
#ifdef USE_SIMULATED_LOD
	float							fLODSimulationFactor;
#endif

	// cached sort metrics
	float							fDist;
	int								nShaderType;
	D3D_EFFECT*						pEffect;
	EFFECT_DEFINITION*				pEffectDef;
	EFFECT_TECHNIQUE*				pTechnique;
	MATERIAL*						pMaterial;
	DRAWLIST_STATE					tState;
	float							fAlpha;

	// dependencies?

	void Clear() 
	{
		structclear( *this );
		nMeshIndex = INVALID_ID;
		nLOD = INVALID_ID;
		//nParticleSystemId = INVALID_ID;
	}
};


// Lightweight indirection for sorting MeshDraw objects
struct MeshDrawSortable
{
	//PList<MeshDraw>::USER_NODE * pMeshDrawNode;			// could just be the MeshDraw pointer...
	MeshDraw *			pMeshDraw;

	// sort factors
	float				fDist;
	//D3D_EFFECT*			pEffect;
	EFFECT_TECHNIQUE*	pTechnique;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct RENDER_CONTEXT
{
	MATRIX							matView;
	MATRIX							matProj;
	MATRIX							matProj2;
	VECTOR3							vEyePos_w;
	VECTOR3							vEyeDir_w;
	ENVIRONMENT_DEFINITION*			pEnvDef;
	BOOL							bForceCurrentEnv;
	E_RECT							tScissor;

	DRAWLIST_STATE					tState;				// CML TODO replace


	/*
	V_RETURN( sGetEnvironment(
	pState,
	pModel,
	pEnvDef,
	bForceCurrentEnv ) );

	if ( !pEnvDef )
	return S_FALSE;
	*/

	void Reset()
	{
		structclear( (*this) );
		MatrixIdentity( &matView );
		MatrixIdentity( &matProj );
		MatrixIdentity( &matProj2 );
		extern DRAWLIST_STATE gtResetDrawListState;
		tState = gtResetDrawListState;
		dxC_GetFullScissorRect( tScissor );
	};
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct ModelDraw
{
	enum FLAGBITS
	{
		FLAGBIT_DATA_COMPLETE = 0,
		FLAGBIT_DATA_INITIALIZED,
		FLAGBIT_FORCE_NO_SCISSOR,
	};

	// Aligned data must go first
	MATRIX 							mWorld;
	MATRIX 							mView;
	MATRIX 							mProj;
	MATRIX 							mWorldView;
	MATRIX 							mWorldViewProjection;

	// Unaligned data
	ModelDraw*						Next; // For use in a CPool
	DWORD							dwFlagBits;

	VECTOR4							vEye_w;
	float							fDistanceToEyeSquared;
	float							fScreensize;
	E_RECT							tScissor;

	const RENDER_CONTEXT *			pContext;

	int								nModelID;
	D3D_MODEL*						pModel;
	int								nModelDefinitionId;
	int								nLOD;
	D3D_MODEL_DEFINITION*			pModelDefinition;

	DWORD 							dwMeshFlagsDoDraw;
	DWORD 							dwMeshFlagsDontDraw;
	float 							fModelAlpha;
	MODEL_RENDER_LIGHT_DATA			tLights;
	PLANE							tPlanes[ NUM_FRUSTUM_PLANES ];		// for mesh culling

#if ISVERSION(DEVELOPMENT)
	int								nIsolateMesh;
#endif

	void Clear()
	{
		structclear((*this));
		nModelID				= INVALID_ID;
		nModelDefinitionId		= INVALID_ID;
		nLOD					= INVALID_ID;

#if ISVERSION(DEVELOPMENT)
		nIsolateMesh			= INVALID_ID;
#endif
	}
};

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

PRESULT dxC_MeshListsInit();
PRESULT dxC_MeshListsDestroy();
PRESULT dxC_MeshListsClear();

PRESULT dxC_MeshListBeginNew( int &nID, MeshList *& pMeshList );
PRESULT dxC_MeshListAddMesh( MeshList & tMeshList, MeshDraw & tMesh );
PRESULT dxC_MeshListGet( int nMeshListID, MeshList *& pMeshList );


}; // namespace FSSE

#endif // __DXC_MESHLIST__