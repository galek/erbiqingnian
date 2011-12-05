//----------------------------------------------------------------------------
// e_light.cpp
//
// - Implementation for lights
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "proxmap.h"
#include "e_main.h"
#include "e_light.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

LIGHT_SLOT_CONTAINER gtLightSlots[ NUM_LS_LIGHT_TYPES ];
PointMap			 gtLightProxMap[ NUM_LIGHT_TYPES ];
SphereMap			 gtSpecularLightProxMap;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void e_LightsCommonInit()
{
	// in case we add one and forget to update these functions
	ASSERT( NUM_LS_LIGHT_TYPES == 2 );

	gtLightSlots[ LS_LIGHT_DIFFUSE_POINT	   ].Init( NUM_DIFFUSE_POINT_LIGHT_SLOTS );
	//gtLightSlots[ LS_LIGHT_STATIC_POINT		   ].Init( NUM_STATIC_POINT_LIGHT_SLOTS );
	gtLightSlots[ LS_LIGHT_SPECULAR_ONLY_POINT ].Init( NUM_SPECULAR_ONLY_POINT_LIGHT_SLOTS );

	for ( int i = 0; i < NUM_LIGHT_TYPES; i++ )
		gtLightProxMap[ i ].Init( NULL );

	gtSpecularLightProxMap.Init( NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_LightsCommonDestroy()
{
	// in case we add one and forget to update these functions
	ASSERT( NUM_LS_LIGHT_TYPES == 2 );

	gtLightSlots[ LS_LIGHT_DIFFUSE_POINT	   ].Destroy();
	//gtLightSlots[ LS_LIGHT_STATIC_POINT		   ].Destroy();
	gtLightSlots[ LS_LIGHT_SPECULAR_ONLY_POINT ].Destroy();

	for ( int i = 0; i < NUM_LIGHT_TYPES; i++ )
		gtLightProxMap[ i ].Free();

	gtSpecularLightProxMap.Free();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_LightGetClosestMatching(
	LIGHT_TYPE eType,
	const VECTOR & vPos,
	DWORD dwFlagsDoWant,
	DWORD dwFlagsDontWant,
	float fRange /*= DEFAULT_DYNAMIC_LIGHT_SELECT_RANGE*/ )
{
	PROXNODE nCurrent;
	int nLight = e_LightGetCloseFirst( eType, nCurrent, vPos, fRange );

	BOUNDED_WHILE( nLight != INVALID_ID, 1000000 )
	{
		DWORD dwFlags = e_LightGetFlags( nLight );

		if ( ( dwFlagsDoWant   && ( dwFlags & dwFlagsDoWant   ) != dwFlagsDoWant ) ||
			 ( dwFlagsDontWant && ( dwFlags & dwFlagsDontWant ) != 0 ) )
		{
			nLight = e_LightGetCloseNext( eType, nCurrent );
			continue;
		}

		return nLight;
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int e_LightGetClosest( LIGHT_TYPE eType, const VECTOR & vPos )
{
	int nID = gtLightProxMap[ eType ].GetClosest( vPos.x, vPos.y, vPos.z );
	return nID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int e_LightGetCloseFirst( LIGHT_TYPE eType, PROXNODE & nCurrent, const VECTOR & vPos, float fRange )
{
	nCurrent = gtLightProxMap[ eType ].Query( vPos.x, vPos.y, vPos.z, fRange );
	if ( nCurrent == INVALID_ID )
		return INVALID_ID;
	return gtLightProxMap[ eType ].GetId( nCurrent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_LightGetCloseNext( LIGHT_TYPE eType, PROXNODE & nCurrent )
{
	nCurrent = gtLightProxMap[ eType ].GetNext( nCurrent );
	if ( nCurrent == INVALID_ID )
		return INVALID_ID;
	return gtLightProxMap[ eType ].GetId( nCurrent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int e_SpecularLightGetIntersectingFirst( PROXNODE & nCurrent, const VECTOR & vPos, float fRange )
{
	nCurrent = gtSpecularLightProxMap.Query( vPos, fRange );
	if ( nCurrent == INVALID_ID )
		return INVALID_ID;
	return gtSpecularLightProxMap.GetId( nCurrent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_SpecularLightGetIntersectingNext( PROXNODE & nCurrent )
{
	nCurrent = gtSpecularLightProxMap.GetNext( nCurrent );
	if ( nCurrent == INVALID_ID )
		return INVALID_ID;
	return gtSpecularLightProxMap.GetId( nCurrent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
int e_SpecularLightGetIntersectingNext( PROXNODE & nCurrent, const VECTOR & vPos, float fRange )
{
	// NOTE: currently iterating all spec light nodes until an XYZ Query is implemented!
	nCurrent = gtSpecularLightProxMap.GetNext( nCurrent );
	while ( nCurrent != INVALID_ID )
	{
		int nLightID = gtSpecularLightProxMap.GetId( nCurrent );
		D3D_LIGHT * pLight = dx9_LightGet( nLightID );
		if ( pLight )
		{
			VECTOR vNodePos;
			gtSpecularLightProxMap.GetPosition( nCurrent, vNodePos.x, vNodePos.y, vNodePos.z );
			float fMaxRange = fRange + pLight->fFalloffEnd;
			if ( VectorDistanceSquared( vPos, vNodePos ) < fMaxRange )
			{
				// found intersecting!
				break;
			}
		}

		nCurrent = gtSpecularLightProxMap.GetNext( nCurrent );
	}

	if ( nCurrent == INVALID_ID )
		return INVALID_ID;
	return gtSpecularLightProxMap.GetId( nCurrent );
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_LightSpecularAddToWorld(
	const STATIC_LIGHT_DEFINITION * pLightDef,
	VECTOR & vPos )
{
	//ASSERT_RETINVALIDARG( pLightDef );

	//const float cfMinSpecOuterRadius = 2.f;
	//const float cfMinSpecInnerRadius = 1.f;

	//STATIC_LIGHT_DEFINITION tLightDef = *pLightDef;
	//PROXNODE nCurrent = 0;
	//float & rfFalloffStart = tLightDef.tFalloff.x;
	//float & rfFalloffEnd   = tLightDef.tFalloff.y;
	//float fRange = rfFalloffEnd;

	//int nIntLightID = e_SpecularLightGetIntersectingFirst( nCurrent, vPos, fRange );
	//BOUNDED_WHILE( nIntLightID != INVALID_ID, 10000 )
	//{
	//	// try to shrink the new light to fit, up to its inner radius or a minimum size
	//	D3D_LIGHT * pIntLight = dx9_LightGet( nIntLightID );
	//	if ( pIntLight )
	//	{
	//		float fDistToEdge = sqrtf( vPos, pIntLight->vPosition );
	//		fDistToEdge -= pIntLight->fFalloffEnd;

	//		ASSERTXONCE( fDistToEdge <= fRange, "SphereMap error: non-intersecting sphere returned as intersecting!" );

	//		float fShrinkPct    = fDistToEdge / rfFalloffEnd;
	//		fShrinkPct = min( fShrinkPct, 1.f );
	//		float fNewMinRadius = rfFalloffStart * fShrinkPct;

	//		if ( fDistToEdge <= cfMinSpecOuterRadius || fNewMinRadius <= cfMinSpecInnerRadius )
	//		{
	//			// Too close to an existing light!  Can't create!
	//			return S_FALSE;
	//		}

	//		// set the new outer and inner radii
	//		rfFalloffStart = fNewMinRadius;
	//		rfFalloffEnd   = fDistToEdge;
	//	}

	//	nIntLightID = e_SpecularLightGetIntersectingNext( nCurrent );
	//}

	//
	//int nNewLightID = e_LightNewStatic( &tLightDef, TRUE );
	//ASSERT_RETFAIL( nNewLightID != INVALID_ID );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_LightSpecularSelectNearest(
	const VECTOR & vFrom,
	float fRange,
	int * pnLights,
	int nMaxLights,
	int & nSelectedLights )
{
	//ASSERT_RETINVALIDARG( pnLights );
	//if ( nMaxLights <= 0 )
	//	return S_FALSE;

	//nSelectedLights = 0;
	//PROXNODE nCurrent = 0;

	//// NOTE:  The list of spheres this returns is intersecting, but unsorted.
	//// We should sort and add them in order.

	//int nLightID = e_SpecularLightGetIntersectingFirst( nCurrent, vFrom, fRange );
	//BOUNDED_WHILE( nLightID != INVALID_ID && nSelectedLights < nMaxLights, 10000 )
	//{
	//	D3D_LIGHT * pLight = dx9_LightGet( nLightID );
	//	if ( pLight )
	//	{
	//		// add it, baby
	//		pnLights[ nSelectedLights ] = nLightID;
	//		nSelectedLights++;
	//	}

	//	nLightID = e_SpecularLightGetIntersectingNext( nCurrent );
	//}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_LightSlotsClear( LIGHT_SLOT_LIGHT_TYPE eType )
{
	int eStart = (int)eType;
	int eEnd = (int)eType + 1;
	if ( eType == LS_LIGHT_TYPE_ALL )
	{
		eStart = 0;
		eEnd = NUM_LS_LIGHT_TYPES;
	}
	for ( int i = (int)eStart; i < (int)eEnd; i++ )
		gtLightSlots[ i ].Clear();
}

