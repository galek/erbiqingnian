//----------------------------------------------------------------------------
// dxC_drawlist.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_DRAWLIST__
#define __DX9_DRAWLIST__

#include "boundingbox.h"
#include "e_drawlist.h"



//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define	DLFLAG_QUERY_OCCLUSION		MAKE_MASK(0)
#define DLFLAG_MAX					MAKE_MASK(1)

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#define DL_CLEAR_LIST( nDrawList )									dx9_ClearDrawList( nDrawList )
#define DL_CLEAR_LIST_TO_STATES( nDrawList, pToState )				dx9_ClearDrawList( nDrawList, pToState )

#define DL_DRAW( nDrawList, nModelID, dwFlags )						dx9_AppendModelToDrawList( nDrawList, nModelID, DLCOM_DRAW, dwFlags )
#define DL_DRAW_MESH( nRenderableMeshId )							dx9_DrawListAddCommand( DRAWLIST_MESH, DLCOM_DRAW, nRenderableMeshId )
#define DL_DRAW_MESH_BEHIND( nRenderableMeshId )					dx9_DrawListAddCommand( DRAWLIST_MESH, DLCOM_DRAW_BEHIND, nRenderableMeshId )
#define DL_DRAW_BOUNDING_BOX( nDrawList, nModelID )					dx9_DrawListAddCommand( nDrawList, DLCOM_DRAW_BOUNDING_BOX, nModelID )
#define DL_DRAW_SHADOW( nDrawList, nModelID, nShadow )				dx9_DrawListAddCommand( nDrawList, DLCOM_DRAW_SHADOW, nModelID, nShadow )
#define DL_DRAW_MESH_SHADOW( nRenderableMeshId, nShadow )			dx9_DrawListAddCommand( DRAWLIST_MESH, DLCOM_DRAW_SHADOW, nRenderableMeshId, nShadow )
#define DL_DRAW_DEPTH( nDrawList, nModelID )						dx9_DrawListAddCommand( nDrawList, DLCOM_DRAW_DEPTH, nModelID )
#define DL_DRAW_MESH_DEPTH( nRenderableMeshId )						dx9_DrawListAddCommand( DRAWLIST_MESH, DLCOM_DRAW_DEPTH, nRenderableMeshId )
#define DL_DRAW_LAYER( nDrawList, pTexture1, pTexture2, dwColor )	dx9_DrawListAddCommand( nDrawList, DLCOM_DRAW_LAYER, -1, dwColor, pTexture1, pTexture2 )
#define DL_SET_CLIP( nDrawList, pBBox )								dx9_DrawListAddClipPlanes( nDrawList, pBBox )
#define DL_RESET_CLIP( nDrawList )									dx9_DrawListAddCommand( nDrawList, DLCOM_RESET_CLIP )
#define DL_SET_SCISSOR( nDrawList, pBBox )							dx9_DrawListAddScissorRects( nDrawList, pBBox )
#define DL_RESET_SCISSOR( nDrawList )								dx9_DrawListAddCommand( nDrawList, DLCOM_RESET_SCISSOR )
#define DL_SET_VIEWPORT( nDrawList, pBBox )							dx9_DrawListAddViewport( nDrawList, pBBox )
#define DL_RESET_VIEWPORT( nDrawList )								dx9_DrawListAddCommand( nDrawList, DLCOM_RESET_VIEWPORT )
#define DL_RESET_FULLVIEWPORT( nDrawList )							dx9_DrawListAddCommand( nDrawList, DLCOM_RESET_FULLVIEWPORT )
#define DL_SET_RENDER_DEPTH_TARGETS( nDrawList, nRT, nDT )			dx9_DrawListAddCommand( nDrawList, DLCOM_SET_RENDER_DEPTH_TARGETS, nRT, nDT )
#define DL_CLEAR_COLOR( nDrawList, dwColor )						dx9_DrawListAddCommand( nDrawList, DLCOM_CLEAR_COLOR, -1, 0, (DWORD)dwColor )
#define DL_CLEAR_COLORDEPTH( nDrawList, dwColor )					dx9_DrawListAddCommand( nDrawList, DLCOM_CLEAR_COLORDEPTH, -1, 0, (DWORD)dwColor )
#define DL_CLEAR_COLORFLOATDEPTH( nDrawList, pColor )				dx9_DrawListAddCommand( nDrawList, DLCOM_CLEAR_COLORFLOATDEPTH, -1, 0, (void*)pColor )
#define DL_CLEAR_DEPTH( nDrawList )									dx9_DrawListAddCommand( nDrawList, DLCOM_CLEAR_DEPTH )
#define DL_CLEAR_DEPTH_VALUE( nDrawList, pfValue )					dx9_DrawListAddCommand( nDrawList, DLCOM_CLEAR_DEPTH_VALUE, (void*)pfValue )
#define DL_COPY_BACKBUFFER( nDrawList, pToD3DTexture )				dx9_DrawListAddCommand( nDrawList, DLCOM_COPY_BACKBUFFER, pToD3DTexture );
#define DL_COPY_RENDERTARGET( nDrawList, nRTID, pToD3DTexture, pBBox )	dx9_DrawListAddCommand( nDrawList, DLCOM_COPY_RENDERTARGET, nRTID, 0, pToD3DTexture, NULL, 0, pBBox )
#define DL_SET_STATE_DWORD( nDrawList, nState, dwValue )			dx9_DrawListAddCommand( nDrawList, DLCOM_SET_STATE, nState, 0, (DWORD)dwValue )
#define DL_SET_STATE_PTR( nDrawList, nState, pValue )				dx9_DrawListAddCommand( nDrawList, DLCOM_SET_STATE, nState, 0, (QWORD)pValue )
#define DL_RESET_STATES( nDrawList )								dx9_DrawListAddCommand( nDrawList, DLCOM_RESET_STATE )
#define DL_RESET_TO_STATES( nDrawList, pStates )					dx9_DrawListAddCommand( nDrawList, DLCOM_RESET_STATE, pStates )
#define DL_CALLBACK( nDrawList, pFn, nID, pData, dwFlags, pBBox )	dx9_DrawListAddCallback( nDrawList, pFn, nID, pData, dwFlags, pBBox )
#define DL_FENCE( nDrawList )										dx9_DrawListAddCommand( nDrawList, DLCOM_FENCE )
#define DL_DRAW_BLOB_SHADOW( nDrawList, pData )						dx9_DrawListAddCommand( nDrawList, DLCOM_DRAW_BLOB_SHADOW, (void*)pData )

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct D3D_DRAWLIST_COMMAND;
struct D3D_MODEL;

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef PRESULT (* PFN_DRAWLIST_CALLBACK) ( int nDrawList, D3D_DRAWLIST_COMMAND * pCommand );

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum DRAWLIST_COMMAND_TYPE
{
	DLCOM_INVALID=-1,
	DLCOM_DRAW=0,
	DLCOM_DRAW_BOUNDING_BOX,
	DLCOM_DRAW_SHADOW,
	DLCOM_DRAW_DEPTH,
	DLCOM_DRAW_LAYER,
	DLCOM_SET_CLIP,
	DLCOM_RESET_CLIP,
	DLCOM_SET_SCISSOR,
	DLCOM_RESET_SCISSOR,
	DLCOM_SET_VIEWPORT,
	DLCOM_RESET_VIEWPORT,
	DLCOM_RESET_FULLVIEWPORT,
	DLCOM_SET_RENDER_DEPTH_TARGETS,
	DLCOM_CLEAR_COLOR,
	DLCOM_CLEAR_COLORDEPTH,
	DLCOM_CLEAR_COLORFLOATDEPTH,
	DLCOM_CLEAR_DEPTH,
	DLCOM_CLEAR_DEPTH_VALUE,
	DLCOM_COPY_BACKBUFFER,
	DLCOM_COPY_RENDERTARGET,
	DLCOM_SET_STATE,
	DLCOM_RESET_STATE,
	DLCOM_CALLBACK,
	DLCOM_DRAW_BEHIND,
	DLCOM_FENCE,
	DLCOM_DRAW_BLOB_SHADOW,
	NUM_DRAWLIST_COMMANDS
};

enum
{
	DLSTATE_INVALID				= -1,
	DLSTATE_OVERRIDE_SHADERTYPE = 0,
	DLSTATE_OVERRIDE_ENVDEF,
	DLSTATE_VIEW_MATRIX_PTR,
	DLSTATE_PROJ_MATRIX_PTR,
	DLSTATE_PROJ2_MATRIX_PTR,
	DLSTATE_EYE_LOCATION_PTR,
	DLSTATE_MESH_FLAGS_DO_DRAW,
	DLSTATE_MESH_FLAGS_DONT_DRAW,
	DLSTATE_USE_LIGHTS,
	DLSTATE_ALLOW_INVISIBLE_MODELS,
	DLSTATE_NO_BACKUP_SHADERS,
	DLSTATE_NO_DEPTH_TEST,
	DLSTATE_FORCE_LOAD_MATERIAL,
	DLSTATE_VIEWPORT,
	NUM_DLSTATES
};

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct DRAWLIST_STATE
{
	int				nOverrideShaderType;
	int				nOverrideEnvDefID;
	MATRIX		  * pmatView;
	MATRIX		  * pmatProj;
	MATRIX		  * pmatProj2;
	VECTOR		  * pvEyeLocation;
	DWORD			dwMeshFlagsDoDraw;
	DWORD			dwMeshFlagsDontDraw;
	BOOL			bLights;
	BOOL			bAllowInvisibleModels;
	BOOL			bNoBackupShaders;
	BOOL			bNoDepthTest;
	BOOL			bForceLoadMaterial;
	D3DC_VIEWPORT	tVP;
};

struct D3D_DRAWLIST_COMMAND
{
	int			 nCommand;
	int			 nID;
	BOUNDING_BOX bBox;
	int			 nSortValue;
	QWORD		 qwFlags;
	int			 nData;
	void	   * pData;
	void	   * pData2;

	void Clear() { nCommand = DLCOM_INVALID; nID = INVALID_ID; VECTOR empty(0,0,0); bBox.vMin = empty; bBox.vMax = empty; nSortValue = -1; nData = 0; pData = NULL; pData2 = NULL; qwFlags = 0; }
};


//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

int dx9_DrawListAddCommand( int nDrawList, int nCommandType, int nID, int nData, void * pData, void * pData2 = NULL, QWORD qwFlags = 0, BOUNDING_BOX * pBBox = NULL );
void dx9_DrawListAddCommand( int nDrawList, int nCommandType, void * pData, void * pData2 = NULL, DWORD dwFlags = 0 );
void dx9_DrawListAddCommand( int nDrawList, int nCommandType, int nID, void * pData, void * pData2 = NULL, DWORD dwFlags = 0 );
void dx9_DrawListAddCommand( int nDrawList, int nCommandType, int nID, int nData = 0, DWORD dwFlags = 0 );
void dx9_DrawListAddCommand( int nDrawList, int nCommandType, DWORD dwFlags = 0 );
D3D_DRAWLIST_COMMAND * dx9_DrawListAddCommand( int nDrawList );
void dx9_DrawListAddCommand( int nDrawList, const D3D_DRAWLIST_COMMAND * pSrcCommand );
void dx9_DrawListAddCallback( int nDrawList, PFN_DRAWLIST_CALLBACK pCallback, int nID = INVALID_ID, int nData = 0, void * pData2 = NULL, DWORD dwFlags = 0, BOUNDING_BOX * pBBox = NULL );
void dx9_DrawListAddModel(
	SIMPLE_DYNAMIC_ARRAY< D3D_MODEL* >* Models,
	int nDrawList,
	int nModelID,
	VECTOR & vEyeLocation,
	DWORD dwModelDefFlagsDoDraw,
	DWORD dwModelDefFlagsDontDraw,
	BOOL bOccluded,
	int nCommandType = DLCOM_DRAW,
	BOOL bQueryOcclusion = FALSE,
	CArrayIndexed<D3D_DRAWLIST_COMMAND>::PFN_COMPARE_FUNCTION pfnSortFunction = NULL,
	BOOL bAllowInvisible = FALSE,
	BOOL bNoDepthTest = FALSE );
void dx9_DrawListAddClipPlanes( int nDrawList, const BOUNDING_BOX *pClipBox );
void dx9_DrawListAddScissorRects( int nDrawList, const BOUNDING_BOX *pScissorRect );
void dx9_DrawListAddViewport( int nDrawList, const BOUNDING_BOX *pViewportParams );
//void dx9_DrawListAddRoom (
//	int nDrawList,
//	VISIBLE_ROOM * pVisRoom, 
//	MATRIX * pmatView, 
//	MATRIX * pmatProj, 
//	VECTOR & vEyeLocation,
//	DWORD dwModelDefFlagsDoDraw,
//	DWORD dwModelDefFlagsDontDraw,
//	BOOL bQueryOcclusion,
//	int nCommandType = DLCOM_DRAW,
//	CArrayIndexed<D3D_DRAWLIST_COMMAND>::PFN_COMPARE_FUNCTION pfnSortFunction = NULL );
void dx9_ClearDrawList ( int nDrawList );
void dx9_BuildZPassDrawListByRooms (
	int nDrawList,
	MATRIX * pmatView, 
	MATRIX * pmatProj, 
	VECTOR & vEyeLocation,
	DWORD dwModelDefFlagsDoDraw,
	DWORD dwModelDefFlagsDontDraw,
	BOOL bQueryOcclusion,
	int nCommandType = DLCOM_DRAW );
void dx9_BuildDrawListByRooms (
	int nDrawList,
	MATRIX * pmatView, 
	MATRIX * pmatProj, 
	VECTOR & vEyeLocation,
	DWORD dwModelDefFlagsDoDraw,
	DWORD dwModelDefFlagsDontDraw,
	BOOL bQueryOcclusion,
	int nCommandType = DLCOM_DRAW );
void dx9_AddAllModelsToDrawList(
	int nDrawList,
	int nCommandType = DLCOM_DRAW, 
	DWORD dwModelDefFlagsDoDraw = 0,
	DWORD dwModelDefFlagsDontDraw = 0,
	BOOL bForceRender = FALSE );
void dx9_AppendModelToDrawList( int nDrawList, int nModelID, int nCommandType = -1, DWORD dwFlags = 0 );
PRESULT dx9_DrawListsInit();
PRESULT dx9_DrawListsDestroy();
PRESULT dx9_RenderDrawList (
	int nDrawList );
const DRAWLIST_STATE * dx9_GetDrawListState( int nDrawList );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline D3D_DRAWLIST_COMMAND * dx9_DrawListCommandGet( const int nDrawList, const int nID )
{
	extern SIMPLE_DYNAMIC_ARRAY<D3D_DRAWLIST_COMMAND> gtDrawList[];
	return gtDrawList[ nDrawList ].GetPointer( nID );
}

inline int dx9_DrawListCommandGetFirstID( const int nDrawList )
{
	extern SIMPLE_DYNAMIC_ARRAY<D3D_DRAWLIST_COMMAND> gtDrawList[];
	if ( gtDrawList[ nDrawList ].Count() == 0 )
		return INVALID_ID;
	else
		return 0;
}

inline int dx9_DrawListCommandGetNextID( const int nDrawList, const int nID )
{
	extern SIMPLE_DYNAMIC_ARRAY<D3D_DRAWLIST_COMMAND> gtDrawList[];
	if ( gtDrawList[ nDrawList ].Count() == 0
 	|| ( gtDrawList[ nDrawList ].Count() == (UINT)( nID + 1 ) ) )
		return INVALID_ID;
	else
		return ( nID + 1 );
}

#endif // __DX9_DRAWLIST__
