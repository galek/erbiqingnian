//----------------------------------------------------------------------------
// e_model.cpp
//
// - Implementation for model structures and functions
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "boundingbox.h"
#include "e_texture.h"
#include "e_model.h"
#include "e_attachment.h"
#include "e_settings.h"
#include "e_environment.h"
#include "proxmap.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//#define TRACE_MODEL_PROXMAP_ACTIONS



//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

PFN_GET_BONES					gpfn_GetBones			= NULL;
PFN_RAGDOLL_GET_POS				gpfn_RagdollGetPosition = NULL;
PFN_QUERY_BY_FILE				gpfn_BGFileInExcel		= NULL;

RANGE2							gtModelCullDistances[NUM_MODEL_DISTANCE_CULL_TYPES]		=
{
	{	40.f,	40.f + MODEL_ALPHA_FADE_OVER_DISTANCE	},		// BACKGROUND
	{	40.f,	40.f + MODEL_ALPHA_FADE_OVER_DISTANCE	},		// ANIMATED_ACTOR
	{	40.f,	40.f + MODEL_ALPHA_FADE_OVER_DISTANCE	},		// ANIMATED_PROP
	{	20.f,	20.f + MODEL_ALPHA_FADE_OVER_DISTANCE	},		// CLUTTER
};

RANGE2							gtModelCullDistancesTugboat[NUM_MODEL_DISTANCE_CULL_TYPES]		=
{
	{	80.f,	80.f + MODEL_ALPHA_FADE_OVER_DISTANCE	},		// BACKGROUND
	{	80.f,	80.f + MODEL_ALPHA_FADE_OVER_DISTANCE	},		// ANIMATED_ACTOR
	{	80.f,	80.f + MODEL_ALPHA_FADE_OVER_DISTANCE	},		// ANIMATED_PROP
	{	70.f,	70.f + MODEL_ALPHA_FADE_OVER_DISTANCE	},		// CLUTTER
};

RANGE2							gtModelCanopyCullDistances[NUM_MODEL_DISTANCE_CULL_TYPES]		=
{
	{	25.f,	25.f + MODEL_NEAR_ALPHA_FADE_OVER_DISTANCE	},		// BACKGROUND
	{	25.f,	25.f + MODEL_NEAR_ALPHA_FADE_OVER_DISTANCE	},		// ANIMATED_ACTOR
	{	25.f,	25.f + MODEL_NEAR_ALPHA_FADE_OVER_DISTANCE	},		// ANIMATED_PROP
	{	25.f,	25.f + MODEL_NEAR_ALPHA_FADE_OVER_DISTANCE	},		// CLUTTER
};

BOOL							gbModelCullDistanceOverride = FALSE;
RANGE2							gtModelCullDistanceOverride =
{
		0.f,	0.f,		// overridden by call activating distance override
};

RANGE2							gtModelCullScreensizes[NUM_MODEL_DISTANCE_CULL_TYPES]	=
{
	{ MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE, MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE - MODEL_ALPHA_FADE_OVER_SCREENSIZE },		// BACKGROUND
	{ MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE, MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE - MODEL_ALPHA_FADE_OVER_SCREENSIZE },		// ANIMATED_ACTOR
	{ MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE, MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE - MODEL_ALPHA_FADE_OVER_SCREENSIZE },		// ANIMATED_PROP
	{ MODEL_CLUTTER_NEAR_ALPHA_SCREENSIZE, MODEL_CLUTTER_NEAR_ALPHA_SCREENSIZE - MODEL_ALPHA_FADE_OVER_SCREENSIZE },		// CLUTTER
};

RANGE2							gtModelCullScreensizesTugboat[NUM_MODEL_DISTANCE_CULL_TYPES]	=
{
	{ MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE_TUGBOAT, MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE_TUGBOAT - MODEL_ALPHA_FADE_OVER_SCREENSIZE },		// BACKGROUND
	{ MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE_TUGBOAT, MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE_TUGBOAT - MODEL_ALPHA_FADE_OVER_SCREENSIZE },		// ANIMATED_ACTOR
	{ MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE_TUGBOAT, MODEL_DEFAULT_NEAR_ALPHA_SCREENSIZE_TUGBOAT - MODEL_ALPHA_FADE_OVER_SCREENSIZE },		// ANIMATED_PROP
	{ MODEL_CLUTTER_NEAR_ALPHA_SCREENSIZE, MODEL_CLUTTER_NEAR_ALPHA_SCREENSIZE - MODEL_ALPHA_FADE_OVER_SCREENSIZE },		// CLUTTER
};

#ifdef MODEL_BOXMAP
BoxMap							gfModelProxMap;
#else
PointMap						gtModelProxMap;
#endif // MODEL_BOXMAP

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_ModelProxmapInit()
{
	gtModelProxMap.Init( NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_ModelProxmapDestroy()
{
	gtModelProxMap.Free();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int e_ModelProxMapGetClosest( const VECTOR & vPos )
{
	UNDEFINED_CODE();
	return INVALID_ID;
	//VECTOR vTo = vPos;
	//int nID = gtModelProxMap.GetClosest( vTo.x, vTo.y, vTo.z );
	//return nID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef MODEL_BOXMAP
int e_ModelProxMapGetCloseFirstFromNode( PROXNODE & nCurrent, PROXNODE nFrom, float fRange )
{
	nCurrent = gtModelProxMap.Query( nFrom, fRange );
	if ( nCurrent == INVALID_ID )
		return INVALID_ID;
	return gtModelProxMap.GetId( nCurrent );
}
#endif // MODEL_BOXMAP

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int e_ModelProxMapGetCloseFirst( PROXNODE & nCurrent, const VECTOR & vPos, float fRange )
{
#ifdef MODEL_BOXMAP
	//nCurrent = gtModelProxMap.Query( vPos, fRange );
#else
	nCurrent = gtModelProxMap.Query( vPos.x, vPos.y, vPos.z, fRange );
#endif // MODEL_BOXMAP
	if ( nCurrent == INVALID_ID )
		return INVALID_ID;
	return gtModelProxMap.GetId( nCurrent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_ModelProxMapGetCloseNext( PROXNODE & nCurrent )
{
	nCurrent = gtModelProxMap.GetNext( nCurrent );
	if ( nCurrent == INVALID_ID )
		return INVALID_ID;
	return gtModelProxMap.GetId( nCurrent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL e_ModelGetRagdollPosition( int nModelID, VECTOR & vRagPos )
{
	extern PFN_RAGDOLL_GET_POS gpfn_RagdollGetPosition;
	ASSERT_RETNULL( gpfn_RagdollGetPosition );
	BOOL bRet = gpfn_RagdollGetPosition( nModelID, vRagPos );
	if ( bRet )
	{
		float fScale = e_ModelGetScale( nModelID ).fX;
		if ( fScale != 0.0f && fScale != 1.0f )
		{
			VECTOR vModelPosition = e_ModelGetPosition( nModelID );
			VECTOR vDelta = vRagPos - vModelPosition;
			vDelta *= fScale;
			vRagPos = vModelPosition + vDelta;
		}
	}
	return bRet;
}
