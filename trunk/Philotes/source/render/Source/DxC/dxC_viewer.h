//----------------------------------------------------------------------------
// dxC_viewer.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_VIEWER__
#define __DXC_VIEWER__

#include "e_frustum.h"
#include <map>
#include "memoryallocator_stl.h"
#include "e_viewer.h"
#include "dxC_target.h"
#include "camera_priv.h"
#include "dxC_commandbuffer.h"
#include "dxC_pass.h"
#include "pool.h"

namespace FSSE
{

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef std::map< DWORD, struct Viewer, std::less<DWORD>, FSCommon::CMemoryAllocatorSTL<std::pair<DWORD, struct Viewer> > > ViewerMap;
typedef safe_vector< struct ViewParameters >	ViewParametersVector;

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


#if ISVERSION(DEVELOPMENT)
struct VIEWER_STATS
{
	int nModelsToTest;
	int nVisibleMeshes;
	int nCommands;
};
#endif // DEVELOPMENT

struct VISIBLE_MESH
{
	enum FLAGBITS
	{
		FLAGBIT_FORCE_NO_SCISSOR = 0,
		FLAGBIT_HAS_SCISSOR,
	};

	// Data used to break into render meshlists.
	int nViewParams;
	int nSection;

	// Model information.
	DWORD dwFlagBits;
	D3D_MODEL * pModel;
	int nLOD;
	D3D_MODEL_DEFINITION * pModelDef;
	D3D_MESH_DEFINITION * pMesh;
	int nMesh;
	MATERIAL_OVERRIDE_TYPE eMatOverrideType;
	float fDistFromEyeSq;
	float fPointDistFromEyeSq;
	float fScreensize;
	E_RECT tScissor;

	ModelDraw* pModelDraw;
};

//----------------------------------------------------------------------------
// Viewer - culling structure used to define a viewpoint for rendering.
//	 This is a generic structure -- it can be used for shadow buffer renders,
//   planar reflection renders, normal renders, etc.
//
struct Viewer
{
	enum 
	{
		FLAGBIT_SELECTED_VISIBLE = 0,
		FLAGBIT_USE_DPVS,
		FLAGBIT_FRUSTUM_CULL,
		FLAGBIT_ENABLE_SKYBOX,
		FLAGBIT_ENABLE_BEHIND,
		FLAGBIT_ENABLE_PARTICLES,
		FLAGBIT_ALLOW_STALE_CAMERA,
		FLAGBIT_IGNORE_GLOBAL_CAMERA_PARAMS
	};

	// identifiers
	int						nID;

	// behavior
	DWORD					dwFlagBits;
	VIEWER_RENDER_TYPE		eRenderType;

	// camera parameters
	struct CAMERA_INFO		tCameraInfo;
	int						nCameraRoom;		// for tracking the camera cell
	int 					nCameraCell;		// could be from the room or the region

	// viewport
	D3DC_VIEWPORT			tVP;

	// target
	SWAP_CHAIN_RENDER_TARGET	eSwapRT;
	SWAP_CHAIN_DEPTH_TARGET		eSwapDT;

	// optional swap chain override
	//const GAME_WINDOW *		ptGameWindow;
	int						nSwapChainIndex;

	// default draw layer
	DRAW_LAYER				eDefaultDrawLayer;

	// culling
#ifdef ENABLE_DPVS
	class DPVS::Camera *	pDPVSCamera;
#endif
	int						nCurSection;
	//int						anLastSection[ 1 + MAX_PORTAL_DEPTH ];		// Last section used by each viewparam
	PLANE					tFrustumPlanes[ NUM_FRUSTUM_PLANES ];
	int						nCurEnvironmentDef;
	//int						nCurRegion;
	REFCOUNT				tCurNoScissor;
	int						nCurViewParams;

	// addl potentially visible models
	safe_vector<D3D_MODEL*>	tModelsToTest;

	// Post-selection visible meshes
	safe_vector<VISIBLE_MESH>	tVisibleMeshList;

	FSCommon::CPool<ModelDraw, false>*  pModelDrawPool;

	// caches
	resizable_array<RENDER_CONTEXT>					tContexts;
	resizable_array<RenderPassDefinition>			tRPDs;
	resizable_array<RenderPassDefinitionSortable>	tRPDSorts;

	CommandBuffer			tCommands;

#if ISVERSION(DEVELOPMENT)
	VIEWER_STATS			tStats;
#endif
};


struct ViewParameters
{
	enum FlagBits
	{
		INVERT_CULL = 0,
		SCISSOR_RECT,
		CLIP_PLANE,
		VIEW_MATRIX,
		PROJ_MATRIX,
		ENV_DEF,
		EYE_POS
	};

	MATRIX					mView;
	MATRIX					mProj;
	VECTOR					vEyePos;
	DWORD					dwFlagBits;
	E_RECT					tScissor;
	PLANE					tClip;
	int						nEnvDefID;

	int						nLastSection;
	// stencil settings?
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline void dxC_ViewerSetCurrent_Private( Viewer * pViewer )
{
	extern Viewer * gpViewerCurrent;
	gpViewerCurrent = pViewer;
}

inline Viewer * dxC_ViewerGetCurrent_Private()
{
	extern Viewer * gpViewerCurrent;
	return gpViewerCurrent;
}

inline int dxC_ViewParametersGetCount()
{
	extern ViewParametersVector * g_ViewParametersVector;
	if ( g_ViewParametersVector )
		return (int)g_ViewParametersVector->size();
	return 0;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_ViewerSetViewport( Viewer * pViewer, const struct D3DC_VIEWPORT & tVP );
PRESULT dxC_ViewerSetViewport( Viewer * pViewer, RENDER_TARGET_INDEX eTarget );
PRESULT dxC_ViewerSetColorTarget( Viewer * pViewer, SWAP_CHAIN_RENDER_TARGET eSwapRT );
PRESULT dxC_ViewerSetDepthTarget( Viewer * pViewer, SWAP_CHAIN_DEPTH_TARGET eSwapDT );
PRESULT dxC_ViewerRender( int nViewerID );
PRESULT dxC_ViewerGet( int nViewerID, Viewer *& pViewer );
PRESULT dxC_ViewerAddModel( Viewer * pViewer, struct D3D_MODEL * pModel );

#if ISVERSION(DEVELOPMENT)
void dxC_ViewerSaveStats( Viewer * pViewer );
#endif // DEVELOPMENT

PRESULT dxC_ViewerAddModelToVisibleList_Private( Viewer * pViewer, struct D3D_MODEL * pModel, struct E_RECT * pRect, int nViewParams = INVALID_ID );
PRESULT dxC_ViewParametersGet( int nViewParametersID, ViewParameters *& pViewParams );
PRESULT dxC_ViewParametersStackPush( int & nViewParametersID );
PRESULT dxC_ViewParametersStackPop();
PRESULT dxC_ViewParametersGetCurrent_Private( int & nViewParametersID );

}; // namespace FSSE

#endif // __DXC_VIEWER__
