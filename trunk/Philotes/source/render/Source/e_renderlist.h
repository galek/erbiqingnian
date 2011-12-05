//----------------------------------------------------------------------------
// e_renderlist.h
//
// - Header for renderable object/effect and render list abstractions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_RENDERLIST__
#define __E_RENDERLIST__

#include "e_rendereffect.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define _RENDERABLE_FLAG_PASSED_CULL		MAKE_MASK(0)
#define _RENDERABLE_FLAG_NO_RENDER			MAKE_MASK(1)
#define _RENDERABLE_FLAG_FIRST_PERSON_PROJ	MAKE_MASK(2)
#define RENDERABLE_FLAG_ALLOWINVISIBLE		MAKE_MASK(3)
#define RENDERABLE_FLAG_OCCLUDABLE			MAKE_MASK(4)
#define RENDERABLE_FLAG_NOCULLING			MAKE_MASK(5)

#define _RL_PASS_FLAG_OPEN					MAKE_MASK(0)
#define RL_PASS_FLAG_CLEAR_BACKGROUND		MAKE_MASK(1)
#define RL_PASS_FLAG_GENERATE_SHADOWS		MAKE_MASK(2)
#define RL_PASS_FLAG_RENDER_SKYBOX			MAKE_MASK(4)
#define RL_PASS_FLAG_RENDER_PARTICLES		MAKE_MASK(5)
#define RL_PASS_FLAG_DRAW_BEHIND			MAKE_MASK(6)

#define MAX_RENDERLIST_PASSES				16

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum RENDERABLE_TYPE
{
	RENDERABLE_INVALID = -1,
	RENDERABLE_MODEL = 0,
	//RENDERABLE_EFFECT,
	//RENDERABLE_PARTICLES,
	//RENDERABLE_UI,
	NUM_RENDERABLE_TYPES
};

enum RL_PASS_TYPE
{
	// in sorting order
	// ----------------

	// special-case render passes
	RL_PASS_GENERATECUBEMAP = 0,
	// typical render passes
	RL_PASS_DEPTH,
	RL_PASS_PORTAL,
	RL_PASS_ALPHAMODEL,
	RL_PASS_REFLECTION,
	// normal goes last, as the consumer of the other pass types
	RL_PASS_NORMAL,
	// count
	NUM_RL_PASS_TYPES
};

enum RL_PASS_MODEL_SORT_METHOD
{
	RL_PASS_MODEL_SORT_DEPTH_LAST = 0,
	RL_PASS_MODEL_SORT_DEPTH_FIRST,
};

enum RL_PASS_MODEL_CULL_METHOD
{
	RL_PASS_MODEL_CULL_INDOOR = 0,
	RL_PASS_MODEL_CULL_OUTDOOR,
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef int	PASSID;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct Renderable
{
	RENDERABLE_TYPE		eType;
	//DWORD				dwSubType;
	DWORD_PTR			dwData;
	DWORD				dwFlags;
	float				fSortValue;

	BOUNDING_BOX		tBBox;
	int					nParent;		// useful for inheriting visibility
};

struct RL_PASS_DESC
{
	RL_PASS_TYPE		eType;
	DWORD				dwFlags;
	int					nPortalID;
};

struct RENDERLIST_PASS
{
	RL_PASS_TYPE							eType;
	DWORD									dwFlags;
	int										nPortalID;
	int										nRegionID;
	int										nDrawLayer;
	float									fSortValue;
	RL_PASS_MODEL_SORT_METHOD				eModelSort;
	RL_PASS_MODEL_CULL_METHOD				eCull;

	SIMPLE_DYNAMIC_ARRAY_EX<Renderable>		tRenderList;		// source render list
	SIMPLE_DYNAMIC_ARRAY_EX<Renderable>		tModelList;			// sorted model list
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct RenderEffect;

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

PRESULT e_RenderListInit();
void e_RenderListClear();
PRESULT e_RenderListDestroy();

// called before any renderlist passes are begun
void e_RenderListBegin();

// called to begin a model renderlist pass
PASSID e_RenderListBeginPass( const RL_PASS_DESC & tDesc, int nCurRegion );

// called to finalize a model renderlist pass
void e_RenderListEndPass( PASSID nPass );

// called to finalize all renderlist additions
void e_RenderListEnd();

// add a renderable to the renderlist
int e_RenderListAdd(
	PASSID			nPass,
	RENDERABLE_TYPE eType,
	DWORD			dwSubType,
	DWORD_PTR		dwData,
	DWORD			dwFlags = 0 );

// add a model to the renderlist
int e_RenderListAddModel(
	PASSID	nPass,
	int		nModelID,
	DWORD	dwFlags = 0 );

void e_RenderListAddAllModels(
	PASSID	nPass,
	DWORD	dwFlags = 0 );

void e_RenderListSetParent(
	PASSID	nPass,
	int		nIndex,
	int		nParent );

void e_RenderListSetBBox(
	PASSID	nPass,
	int		nIndex,
	const	BOUNDING_BOX * ptBBox );

void e_RenderListPassCull( PASSID nPass );
void e_RenderListPassBuildModelList( PASSID nPass );
void e_RenderListPassMarkVisible( PASSID nPass );
void e_RenderListSortPasses();
void e_RenderListCommit();

//int e_RenderListAddEffectBegin(
//	RENDER_EFFECT_TYPE	eType,
//	DWORD_PTR			dwData,
//	DWORD				dwFlags = RENDERABLE_FLAG_DEFAULT );
//
//int e_RenderListAddEffectEnd(
//	RENDER_EFFECT_TYPE	eType,
//	DWORD_PTR			dwData,
//	DWORD				dwFlags = RENDERABLE_FLAG_DEFAULT );
//
//int e_RenderListAddParticles(
//	int		nGroup,
//	DWORD	dwFlags = RENDERABLE_FLAG_DEFAULT );
//
//int e_RenderListAddUI(
//	void *	pUIData,   // what to pass here?
//	DWORD	dwFlags = RENDERABLE_FLAG_DEFAULT );

#endif // __E_RENDERLIST__