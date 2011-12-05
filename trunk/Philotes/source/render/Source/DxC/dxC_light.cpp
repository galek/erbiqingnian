//----------------------------------------------------------------------------
// dx9_light.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_definition.h"
#include "dxC_model.h"
#include "dxC_main.h"
#include "filepaths.h"
#include "camera_priv.h"
#include "appcommontimer.h"
#include "dxC_primitive.h"
#include "event_data.h"
#include "e_optionstate.h"
#include "perfhier.h"
#include "dxC_drawlist.h"
#include "dxC_irradiance.h"

#include "dxC_light.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define MIN_LIGHT_FALLOFF						2.0f


// these are the main light selection knobs
#define LIGHT_SELECT_DISTANCE_STR				1.0f
#define LIGHT_SELECT_RADIUS_STR					0.25f
#define LIGHT_SELECT_PRIORITY_STR				0.5f


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct LIGHT_SORTABLE
{
	D3D_LIGHT *		pLight;
	float			fWeight;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

// EXTERNABLES

int							gnLightNextId = 0;
CHash<D3D_LIGHT>			gtLights;
BOOL						gbDrawLights = FALSE;
float						gfDynLightSelectionRange = 0.f;
float						gfDynLightSelectionMaxRadius = 0.f;

// STATIC 

static SIMPLE_DYNAMIC_ARRAY_EX<LIGHT_SORTABLE>	sgtSelectLights;

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern PointMap gtLightProxMap[ NUM_LIGHT_TYPES ];
extern SphereMap gtSpecularLightProxMap;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLightAddToProxmap( D3D_LIGHT * pLight )
{
	ASSERT_RETURN( pLight->dwFlags & LIGHT_FLAG_DEFINITION_APPLIED );	// otherwise we might use the wrong type
	ASSERT_RETURN( pLight->nProxmapId == INVALID_ID );
	pLight->nProxmapId = gtLightProxMap[ pLight->eType ].Insert( pLight->nId, 0, pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z );
#ifdef TRACE_LIGHT_PROXMAP_ACTIONS
	trace( "---- Proxmap insert [ %4d ] [ %4d ] [ 0x%08x ]\n", pLight->nProxmapId, pLight->nId, pLight );
#endif // TRACE_LIGHT_PROXMAP_ACTIONS
	ASSERT( pLight->nProxmapId != INVALID_ID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLightMoveInProxmap( D3D_LIGHT * pLight )
{
	ASSERT_RETURN( pLight->nProxmapId != INVALID_ID );
	gtLightProxMap[ pLight->eType ].Move( pLight->nProxmapId, pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLightRemoveFromProxmap( D3D_LIGHT * pLight )
{
	if ( pLight->nProxmapId == INVALID_ID )
		return;
#ifdef TRACE_LIGHT_PROXMAP_ACTIONS
	trace( "---- Proxmap delete [ %4d ] [ %4d ] [ 0x%08x ]\n", pLight->nProxmapId, pLight->nId, pLight );
#endif // TRACE_LIGHT_PROXMAP_ACTIONS
	gtLightProxMap[ pLight->eType ].Delete( pLight->nProxmapId );
	pLight->nProxmapId = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpecularLightAddToProxmap( D3D_LIGHT * pLight )
{
//	ASSERT_RETURN( pLight->nProxmapId == INVALID_ID );
//	ASSERT_RETURN( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY );
//	pLight->nProxmapId = gtSpecularLightProxMap.Insert( pLight->nId, 0, pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z, pLight->fFalloffEnd );
//#ifdef TRACE_LIGHT_PROXMAP_ACTIONS
//	trace( "---- Proxmap insert spec [ %4d ] [ %4d ] [ 0x%08x ]\n", pLight->nProxmapId, pLight->nId, pLight );
//#endif // TRACE_LIGHT_PROXMAP_ACTIONS
//	ASSERT( pLight->nProxmapId != INVALID_ID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpecularLightRemoveFromProxmap( D3D_LIGHT * pLight )
{
//	if ( pLight->nProxmapId == INVALID_ID )
//		return;
//#ifdef TRACE_LIGHT_PROXMAP_ACTIONS
//	trace( "---- Proxmap delete [ %4d ] [ %4d ] [ 0x%08x ]\n", pLight->nProxmapId, pLight->nId, pLight );
//#endif // TRACE_LIGHT_PROXMAP_ACTIONS
//	gtSpecularLightProxMap.Delete( pLight->nProxmapId );
//	pLight->nProxmapId = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugDrawLightSource( D3D_LIGHT * pLight )
{
	BOUNDING_BOX tBox, tBoxLarge;
	float fHalfSize = 0.03f;
	VECTOR vCorner( fHalfSize, fHalfSize, fHalfSize );
	tBox.vMin = pLight->vPosition - vCorner;
	tBox.vMax = pLight->vPosition + vCorner;
	vCorner *= 4.f;
	tBoxLarge.vMin = pLight->vPosition - vCorner;
	tBoxLarge.vMax = pLight->vPosition + vCorner;

	DWORD dwCol = 0xff0000ff;
	if ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY )
		dwCol = 0xffffff00;

	V( e_PrimitiveAddBox( 0, &tBoxLarge, 0xffffffff ) );
	V( e_PrimitiveAddBox( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_RENDER_ON_TOP, &tBox, dwCol ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugDrawLight( D3D_LIGHT * pLight, DWORD dwColor, DWORD dwFlags )
{
	if ( pLight->eType == LIGHT_TYPE_POINT )
	{
		V( e_PrimitiveAddSphere( dwFlags, &pLight->vPosition, pLight->fFalloffEnd, dwColor ) );
	}
	else if ( pLight->eType == LIGHT_TYPE_SPOT )
	{
		LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );
		if ( ! pLightDef )
			return;
		V( e_PrimitiveAddCone( dwFlags, &pLight->vPosition, pLight->fFalloffEnd, &pLight->vDirection, DegreesToRadians( pLightDef->fSpotAngleDeg ), dwColor ) );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDebugDrawLight( D3D_LIGHT * pLight, LIGHT_SLOT * pSlot = NULL )
{
	VECTOR vDiffuse = VECTOR( pLight->vDiffuse.x, pLight->vDiffuse.y, pLight->vDiffuse.z );
	// normalizing the color and multiplying down for a consistent luminosity
	VectorNormalize( vDiffuse );
	vDiffuse *= 0.25f;
	if ( pSlot )
		vDiffuse *= pSlot->fStrength;
	DWORD dwColor = ARGB_MAKE_FROM_FLOAT( vDiffuse.x, vDiffuse.y, vDiffuse.z, 0.f );
	sDebugDrawLight( pLight, dwColor, PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sLightSetFalloff ( D3D_LIGHT * pLight, float fFalloffStart, float fFalloffEnd )
{
	//if ( pLight->fFalloffEnd != fFalloffEnd )
	//{
	//	ROOM* room = RoomGetByID( pGame, pLight->m_idRoom );
	//	if (room)
	//	{
	//		c_RoomLightMoved(room);
	//	}
	//}

	if ( fFalloffEnd - fFalloffStart < MIN_LIGHT_FALLOFF )
	{
		fFalloffStart = fFalloffEnd - MIN_LIGHT_FALLOFF;
		fFalloffStart = max( fFalloffStart, 0.0f );
	}
	pLight->fFalloffStart = fFalloffStart; 
	pLight->fFalloffEnd   = fFalloffEnd; 
	if ( pLight->fFalloffEnd <= pLight->fFalloffStart )
		pLight->vFalloff.y = 0.0f;
	else
		pLight->vFalloff.y = 1.0f / ( pLight->fFalloffEnd - pLight->fFalloffStart );
	pLight->vFalloff.x = pLight->fFalloffEnd * pLight->vFalloff.y; 
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_LightSetLifePercent( int nLightId, float fValue )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	ASSERT_RETURN(pLight);

	pLight->dwFlags |= LIGHT_FLAG_PATH_TIME_SET;
	pLight->fLifePercent = fValue;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
float e_LightGetLifePercent( int nLightId )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	ASSERT_RETVAL(pLight, 0.0f);

	if ( pLight->dwFlags & LIGHT_FLAG_PATH_TIME_SET )
		return pLight->fLifePercent;

	LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );
	if ( ! pLightDef )
		return 0.0f;

	float fTimeCurrent = e_GameGetTimeFloat();

	float fPathTime;
	switch ( pLightDef->nDurationType )
	{
	case DURATION_CONSTANT:
		fPathTime = 0.0f;
		break;
	case DURATION_ONE_TIME:
		{
			float fDuration = pLightDef->fStartTime;
			if ( fDuration == 0.0f )
				fDuration = 0.1f;
			fPathTime = (fTimeCurrent - pLight->fBeginTime) / fDuration;
			if ( pLight->dwFlags & LIGHT_FLAG_FORCE_LOOP )
			{
				fPathTime = fPathTime - FLOOR(fPathTime);
			}
			if ( fPathTime > 1.0f )
			{
				fPathTime = 1.0f;
				pLight->dwFlags |= LIGHT_FLAG_DYING | LIGHT_FLAG_DEAD;
			}
		}
		break;
	case DURATION_LOOPS_RUN_TO_END:
	case DURATION_LOOPS_SKIP_TO_END:
		{
			float fTimeDelta = fTimeCurrent - pLight->fBeginTime;
			if ( pLight->dwFlags & LIGHT_FLAG_DYING )
			{
				fPathTime = (( fTimeCurrent - pLight->fDeathTime ) / pLightDef->fEndTime ) / 3.0f;
				fPathTime += 2.0f / 3.0f;
				if ( fPathTime > 1.0f )
				{
					if ( pLight->dwFlags & LIGHT_FLAG_FORCE_LOOP )
					{
						pLight->fBeginTime = e_GameGetTimeFloat();
						pLight->dwFlags &= ~LIGHT_FLAG_DYING;
					} else {
						pLight->dwFlags |= LIGHT_FLAG_DEAD;
					}
					fPathTime = 1.0f;
				}
				break;
			} else if ( fTimeDelta < pLightDef->fStartTime ) {
				fPathTime = ( fTimeDelta / pLightDef->fStartTime ) / 3.0f;
			} else {
				float fTimeInLoop = fTimeDelta - pLightDef->fStartTime;
				float fLoopDuration = pLightDef->fLoopTime != 0 ? pLightDef->fLoopTime : 0.1f;
				fTimeInLoop /= fLoopDuration;
				fTimeInLoop = fTimeInLoop - FLOOR(fTimeInLoop);
				fPathTime = (1.0f / 3.0f) + fTimeInLoop / 3.0f;
			}
		}
		break;
	}

	return fPathTime;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sLightUpdate ( D3D_LIGHT * pLight )
{
	if ( pLight->dwFlags & LIGHT_FLAG_STATIC )
		return;

	LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );
	if ( ! pLightDef )
		return;

	float fPathTime = e_LightGetLifePercent( pLight->nId );

	float fIntensity = pLightDef->tIntensity.GetValue( fPathTime ).fX;
	CFloatTriple tColor = pLightDef->tColor.GetValue( fPathTime );
	pLight->vDiffuse.x = tColor.x * fIntensity;
	pLight->vDiffuse.y = tColor.y * fIntensity;
	pLight->vDiffuse.z = tColor.z * fIntensity;
	pLight->vDiffuse.w = 1.0f;

	CFloatPair tFalloff = pLightDef->tFalloff.GetValue( fPathTime );
	sLightSetFalloff ( pLight, tFalloff.x, tFalloff.y );

	pLight->vFalloff.z = fIntensity; 
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_LightsUpdateAll ()
{
	for (D3D_LIGHT* pLight = HashGetFirst(gtLights); pLight; pLight = HashGetNext(gtLights, pLight))
	{
		sLightUpdate( pLight );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_LightApplyDefinition( int nLightID )
{
	D3D_LIGHT * pLight = dx9_LightGet( nLightID );
	dx9_LightApplyDefinition( pLight );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dx9_LightApplyDefinition( D3D_LIGHT * pLight )
{
	if ( ! pLight )
		return;
	if ( pLight->dwFlags & LIGHT_FLAG_DEFINITION_APPLIED )
		return;

	// static lights don't have definitions
	if ( pLight->nLightDef != INVALID_ID )
	{
		LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );
		if ( ! pLightDef )
			return;

		if ( pLightDef->dwFlags & LIGHTDEF_FLAG_NOFADEIN )
			pLight->dwFlags |= LIGHT_FLAG_NOFADEIN;
		pLight->eType = pLightDef->eType;
	}

	pLight->dwFlags |= LIGHT_FLAG_DEFINITION_APPLIED;

	//if ( 0 == ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY ) )
	sLightAddToProxmap( pLight );
	e_LightSetPosition( pLight->nId, pLight->vPosition );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sLightDefinitionLoadedCallback(
	void * pDef,
	EVENT_DATA * pEventData )
{
	LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION *) pDef;
	int nLightID = (int)pEventData->m_Data1;
	e_LightApplyDefinition( nLightID );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int	e_LightNew(
	int nLightDef,
	float fPriority,
	const VECTOR& vPosition,
	VECTOR* pvPositionOffset)
{
	int nLightId = gnLightNextId ++;
	D3D_LIGHT* pLight = HashAdd(gtLights, nLightId);
	ASSERT(pLight);
	pLight->nLightDef = nLightDef;
	pLight->fBeginTime = e_GameGetTimeFloat();
	pLight->nProxmapId = INVALID_ID;

	pLight->vPosition = (D3DXVECTOR3 &)vPosition;

	sLightUpdate( pLight );

	if ( pvPositionOffset )
	{
		pLight->vPositionOffset = (D3DXVECTOR3 &)*pvPositionOffset;
	}

	// if no definition, assume point light
	pLight->eType = LIGHT_TYPE_POINT;

	if ( nLightDef != INVALID_ID )
	{
		LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_LIGHT, nLightDef );
		EVENT_DATA tEventData( nLightId );
		if ( pLightDef )
		{
			// this will apply the definition
			sLightDefinitionLoadedCallback( pLightDef, &tEventData );
		} else {
			DefinitionAddProcessCallback( DEFINITION_GROUP_LIGHT, nLightDef, 
				sLightDefinitionLoadedCallback, &tEventData );
		}
	} else
	{
		dx9_LightApplyDefinition( pLight );
	}

	pLight->fPriority = fPriority;
	pLight->fPriorityBonus = 0.0f;

	pLight->dwGroup  = LIGHT_GROUP_GENERAL;

	ASSERT( pLight->nId >= 0 );

	return nLightId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int	e_LightNewStatic(
	STATIC_LIGHT_DEFINITION* pLightDef,
	BOOL bSpecularOnly)
{
	int nLightId = gnLightNextId ++;
	D3D_LIGHT* pLight = HashAdd(gtLights, nLightId);
	ASSERT(pLight);
	pLight->vPosition = pLightDef->vPosition;
	pLight->nProxmapId = INVALID_ID;
	pLight->nLightDef = INVALID_ID;

	float fIntensity = pLightDef->fIntensity;
	pLight->vDiffuse.x = pLightDef->tColor.x * fIntensity;
	pLight->vDiffuse.y = pLightDef->tColor.y * fIntensity;
	pLight->vDiffuse.z = pLightDef->tColor.z * fIntensity;
	pLight->vDiffuse.w = 1.0f;

	sLightSetFalloff ( pLight, pLightDef->tFalloff.x, pLightDef->tFalloff.y );

	pLight->vFalloff.z = fIntensity; 

	pLight->dwFlags |= LIGHT_FLAG_STATIC;
	if ( bSpecularOnly )
	{
		pLight->dwFlags |= LIGHT_FLAG_SPECULAR_ONLY;
		//sSpecularLightAddToProxmap( pLight );
	}
	pLight->fPriority = pLightDef->fPriority;

	pLight->dwGroup  = LIGHT_GROUP_GENERAL;

	// there is no definition, but this will set the flags to indicate that the light is ready
	e_LightApplyDefinition( nLightId );

	ASSERT( pLight->nId >= 0 );

	return nLightId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_LightGetPosition( int nLightId, VECTOR & vPos )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if ( ! pLight )
	{
		vPos = VECTOR(0,0,0);
		return;
	}
	vPos = pLight->vPosition;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_LightSetPosition(
	int nLightId,
	const VECTOR& vPosition,
	const VECTOR* pvDirection)
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight /*|| 0 == ( pLight->dwFlags & LIGHT_FLAG_DEFINITION_APPLIED )*/ )
	{
		return;
	}

	pLight->vPosition = (D3DXVECTOR3 &)vPosition;
	pLight->vPosition += pLight->vPositionOffset;
	if ( pvDirection )
	{
		pLight->vDirection = * (D3DXVECTOR3 *)pvDirection;
#if ISVERSION(DEVELOPMENT)
		float fLen = VectorLengthSquared( pLight->vDirection );
		ASSERTX( fabs( fLen - 1.f ) < 0.1f, "Light direction must be normalized!" );
#endif
	}

	if ( pLight->dwFlags & LIGHT_FLAG_DEFINITION_APPLIED )
		sLightMoveInProxmap( pLight );

	pLight->dwFlags |= LIGHT_FLAG_MOVED;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_LightRemove( int nLightId )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return;
	}
	//if ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY )
	//	sSpecularLightRemoveFromProxmap( pLight );
	//else
	sLightRemoveFromProxmap( pLight );

	if ( pLight->eType == LIGHT_TYPE_SPOT && pLight->nLightDef != INVALID_ID )
	{
		LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );
		//e_TextureReleaseRef( pLightDef->nSpotUmbraTextureID );
	}

	HashRemove(gtLights, nLightId);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_LightHasMoved ( int nLightId, BOOL bClear )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return TRUE;
	}
	BOOL bMoved = ( pLight->dwFlags & LIGHT_FLAG_MOVED ) != 0;
	if ( bClear )
		pLight->dwFlags &= ~LIGHT_FLAG_MOVED;
	return bMoved;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_LightIsSpot ( int nLightId )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return FALSE;
	}
	return ( pLight->dwFlags & LIGHT_FLAG_SPOT ) != 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_LightForceLoop ( int nLightId, BOOL bForce /*= TRUE*/ )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return;
	}
	if ( bForce )
		pLight->dwFlags |= LIGHT_FLAG_FORCE_LOOP;
	else
		pLight->dwFlags &= ~LIGHT_FLAG_FORCE_LOOP;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_LightGetDefinition ( int nLightId )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return INVALID_ID;
	}
	return pLight->nLightDef;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_LightSetGroup ( int nLightId, DWORD dwGroups )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return;
	}
	pLight->dwGroup = dwGroups;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_LightGetGroup ( int nLightId )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return 0;
	}
	return pLight->dwGroup;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_LightSetFlags( int nLightId, DWORD dwFlags, BOOL bSet )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return;
	}

	if ( bSet )
		pLight->dwFlags |= dwFlags;
	else
		pLight->dwFlags &= ~dwFlags;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_LightGetFlags( int nLightId )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return 0;
	}
	return pLight->dwFlags;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

LIGHT_TYPE e_LightGetType( int nLightId )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	if (!pLight)
	{
		return LIGHT_TYPE_INVALID;
	}
	return pLight->eType;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_LightExists ( int nLightId )
{
	D3D_LIGHT* pLight = dx9_LightGet( nLightId );
	return (pLight != NULL);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static int sLoadToolModel( const char * pszFilename, const char * pszFolder )
{
	return e_ModelDefinitionNewFromFile( pszFilename, pszFolder );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
enum 
{
	LIGHT_NORMAL = 0,
	LIGHT_DEBUG_DYNAMIC_SELECTED,
	LIGHT_DEBUG_DYNAMIC_NOT_SELECTED,
	LIGHT_DEBUG_BG_SPECULAR,
	LIGHT_DEBUG_BG_CHAR_SELECTED,
	LIGHT_DEBUG_BG_CHAR_NOT_SELECTED,
	NUM_LIGHT_DEBUG_MODEL_TYPES
};

static void sGetLightDebugModelTypes( D3D_LIGHT * pLight, int * pnTypes )
{
	if ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY )
	{
		pnTypes[ 0 ] = LIGHT_DEBUG_BG_SPECULAR;
		pnTypes[ 1 ] = LIGHT_DEBUG_BG_SPECULAR;
	} else if ( pLight->dwFlags & LIGHT_FLAG_STATIC )
	{
		pnTypes[ 0 ] = LIGHT_DEBUG_BG_CHAR_NOT_SELECTED;
		pnTypes[ 1 ] = LIGHT_DEBUG_BG_CHAR_SELECTED;
	} else 
	{
		pnTypes[ 0 ] = LIGHT_DEBUG_DYNAMIC_NOT_SELECTED;
		pnTypes[ 1 ] = LIGHT_DEBUG_DYNAMIC_SELECTED;
	}
}

BOOL e_ToggleDrawAllLights()
{
	gbDrawLights = ! gbDrawLights;
	return gbDrawLights;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

float e_LightGetPriority( int nLightID )
{
	D3D_LIGHT * pLight = dx9_LightGet( nLightID );
	ASSERT_RETZERO( pLight );

	return dx9_LightGetPriority( pLight );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

float dx9_LightGetPriority( D3D_LIGHT * pLight )
{
	const CAMERA_INFO * pInfo = CameraGetInfo();
	ASSERT_RETZERO( pInfo );

	float fDistSqr = VectorDistanceSquared( pInfo->vPosition, pLight->vPosition );
	if ( fDistSqr > pLight->fFalloffEnd * pLight->fFalloffEnd )
		return 0.f;

	// trying something silly here:  light priority will be D3D_LIGHT::fPriority + 0..1 falloff factor, so max priority would be 2.0f

	return pLight->fPriority + sqrtf( fDistSqr );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLightSelectCalculateWeight( LIGHT_SORTABLE &tLight, VECTOR &vEye )
{
	// Build a per-light weight for sorting.
	// The goal is to allow us to scale max distance and max range but keep proportional weighting for type and priority
	// weight = sat(dist / maxrange) + sat(rad / maxrad) + sat(priority / maxpriority) + typeweight[ type ]

	ASSERT_RETURN( tLight.pLight );

	// distance
	float fDistance = VectorDistanceSquared( vEye, tLight.pLight->vPosition );
	ASSERT_RETURN( fDistance >= 0.f );
	fDistance = sqrtf( fDistance );
	float d = fDistance / gfDynLightSelectionRange;
	d = 1.0f - SATURATE( d );
	d *= LIGHT_SELECT_DISTANCE_STR;

	// radius
	float r = tLight.pLight->fFalloffEnd / gfDynLightSelectionMaxRadius;
	r = SATURATE( r );
	r *= LIGHT_SELECT_RADIUS_STR;

	// priority
	float p = tLight.pLight->fPriority / LIGHT_MAX_PRIORITY;
	p = SATURATE( p );
	p *= LIGHT_SELECT_PRIORITY_STR;

	// type
	// nothing for this, yet
	//float t = gfLightSelectionTypeWeights[ ]
	float t = 0.f;

	tLight.fWeight = d + r + p + t;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLightSelectCalculateWeightSpecular(
	LIGHT_SORTABLE &tLight,
	VECTOR &vFocusPos,
	VECTOR &vViewDir,
	const ENVIRONMENT_DEFINITION * pEnvDef = NULL )
{
	ASSERT_RETURN( tLight.pLight );

	tLight.fWeight = e_LightSpecularCalculateWeight( tLight.pLight, vFocusPos, vViewDir, pEnvDef );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_LightSelectClearList()
{
	ArrayClear(sgtSelectLights);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLightSelectAddToList( const LIGHT_SORTABLE & tLight )
{
	ArrayAddItem(sgtSelectLights, tLight);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void e_LightSelectAddToList( int nLightID, float fWeight )
{
	LIGHT_SORTABLE tLight;
	tLight.pLight = dx9_LightGet( nLightID );
	if ( ! tLight.pLight )
		return;
	tLight.fWeight = fWeight;
	ArrayAddItem(sgtSelectLights, tLight);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCompareSelectLightsAscending( const void * e1, const void * e2 )
{
	const LIGHT_SORTABLE * pl1 = static_cast<const LIGHT_SORTABLE *>(e1);
	const LIGHT_SORTABLE * pl2 = static_cast<const LIGHT_SORTABLE *>(e2);

	// sort ascending
	if ( pl1->fWeight > pl2->fWeight )
		return 1;
	if ( pl1->fWeight < pl2->fWeight )
		return -1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCompareSelectLightsDescending( const void * e1, const void * e2 )
{
	const LIGHT_SORTABLE * pl1 = static_cast<const LIGHT_SORTABLE *>(e1);
	const LIGHT_SORTABLE * pl2 = static_cast<const LIGHT_SORTABLE *>(e2);

	// sort descending
	if ( pl1->fWeight < pl2->fWeight )
		return 1;
	if ( pl1->fWeight > pl2->fWeight )
		return -1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_LightSelectSortList( BOOL bAscending )
{
	if ( bAscending )
		sgtSelectLights.QSort( sCompareSelectLightsAscending );
	else
		sgtSelectLights.QSort( sCompareSelectLightsDescending );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_LightSelectGetFromList( int * pnLightIDs, int & nCount, int nMaxCount, DWORD dwFlagsWant, DWORD dwFlagsDontWant )
{
	nCount = 0;
	for ( int i = 0; nCount < nMaxCount && i < (int)sgtSelectLights.Count(); i++ )
	{
		D3D_LIGHT * pLight = sgtSelectLights[ i ].pLight;
		ASSERT_CONTINUE( pLight );
		if ( dwFlagsWant     && ( pLight->dwFlags & dwFlagsWant     ) == 0 )
			continue;
		if ( dwFlagsDontWant && ( pLight->dwFlags & dwFlagsDontWant ) != 0 )
			continue;

		pnLightIDs[ nCount ] = pLight->nId;
		nCount++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_LightSelectAndSortPointList(
	BOUNDING_SPHERE & tBS,
	const int * pnLights,
	int nLights,
	int * pnOutLights,
	int & nOutLights,
	int nMax,
	DWORD dwFlagsDoWant,
	DWORD dwFlagsDontWant )
{
	e_LightSelectClearList();

	for ( int i = 0; i < nLights; i ++ )
	{
		D3D_LIGHT * pLight = dx9_LightGet( pnLights[ i ] );
		if ( !pLight )
			continue;

		//ASSERT_CONTINUE( 0 == ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY ) );

		// add to a list for sorting


		BOUNDING_SPHERE tBSLight( pLight->vPosition, pLight->fFalloffEnd );
		float fDistance;

#if ISVERSION(DEVELOPMENT)
		//fDistance = tBSLight.Distance( tBS );
		//keytrace( "fDistance: %3.2f  -- ", fDistance );
#endif // DEVELOPMENT


		if ( FALSE == tBSLight.Intersect( tBS ) )
		{
			//keytrace( "culled!\n" );
			continue;
		}

		//fDistance = sqrtf( VectorDistanceSquared( tBSLight.C, tBS.C ) );
		//fDistance = tBSLight.Distance( tBS );
		fDistance = tBS.Distance( tBSLight.C );
		float fStrength = pLight->vFalloff.x - fDistance * pLight->vFalloff.y;
		//fStrength = SATURATE( fStrength );

		//keytrace( "kept!  dist %3.1f, str %3.1f\n", fDistance, fStrength );

		e_LightSelectAddToList( pLight->nId, fStrength );
	}

	// sort descending (strength first)
	e_LightSelectSortList( FALSE );

	e_LightSelectGetFromList( pnOutLights, nOutLights, nMax, dwFlagsDoWant, dwFlagsDontWant );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_LightSelectAndSortSpecList(
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const int * pnLights,
	int nLights,
	int * pnOutLights,
	int & nOutLights,
	int nMax,
	DWORD dwFlagsDoWant,
	DWORD dwFlagsDontWant )
{
	// these are sorted based on distance from eye

	VECTOR vViewDir;
	//VECTOR vFocusPos;
	//e_GetViewFocusPosition( &vFocusPos );
	VECTOR vPos;
	e_GetViewPosition( &vPos );
	e_GetViewDirection( &vViewDir );
	VectorNormalize( vViewDir );

	e_LightSelectClearList();
	for ( int i = 0; i < nLights; i ++ )
	{
		D3D_LIGHT * pLight = dx9_LightGet( pnLights[ i ] );
		if ( ! pLight )
			continue;

		ASSERT_CONTINUE( 0 != ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY ) );


		// add to a list for sorting
		float fStrength = e_LightSpecularCalculateWeight(
			pLight,
			//vFocusPos,
			vPos,
			vViewDir );
		//if ( fStrength < 0.f )
		//	continue;

		e_LightSelectAddToList( pLight->nId, fStrength );
	}

	// sort in descending order (strength first)
	e_LightSelectSortList( FALSE );

	e_LightSelectGetFromList( pnOutLights, nOutLights, nMax, dwFlagsDoWant, dwFlagsDontWant );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

float e_LightSpecularCalculateWeight(
	const D3D_LIGHT * pLight,
	VECTOR & vFocusPos,
	VECTOR & vViewDir,
	const ENVIRONMENT_DEFINITION * pEnvDef /*= NULL*/ )
{
	VECTOR vDeltaToLight = pLight->vPosition - vFocusPos;
	float fDistance = VectorLengthSquared( vDeltaToLight );
	float fRange = pLight->fFalloffEnd * pLight->fFalloffEnd * 1.3f;  // + fudge factor
	if ( fDistance > fRange )
		return -1.f;

	float fStrength = pLight->vFalloff.x - sqrtf( fDistance ) * pLight->vFalloff.y;
	fStrength = SATURATE( fStrength );

	if ( pEnvDef && pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_SPECULAR_FAVOR_FACING )
	{
		// apply a cosine weight bonus
		VectorNormalize( vDeltaToLight );
		float fDot = VectorDot( vViewDir, vDeltaToLight );
		fStrength += fStrength * 0.5f * fDot;
		//fStrength += fStrength * ( 0.5f + fDot );
	}
	
	return fStrength;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sAddToVirtualSlots( LIGHT_SLOT_CONTAINER * pContainer )
{
	int nCount = sgtSelectLights.Count();

	for ( int i = 0; i < pContainer->nSlotsActive; i++ )
	{
		pContainer->pVirtualSlots[ i ].nSlot = i;
		if ( i < nCount )
		{
			LIGHT_SORTABLE * ptLight = &sgtSelectLights[ i ];
			ASSERT_CONTINUE( ptLight->pLight );
			pContainer->pVirtualSlots[ i ].nLightID = ptLight->pLight->nId;
		}
		else
		{
			pContainer->pVirtualSlots[ i ].nLightID = INVALID_ID;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLightBeginFadeUp( LIGHT_SLOT * pSlot )
{
	pSlot->eFadeState = LSFS_FADEUP;
	pSlot->fStrength = 0.f;
	pSlot->fFadeStrStart = 0.f;
	pSlot->fFadeStrEnd = 1.f;
	pSlot->tFadeStart = AppCommonGetCurTime();
	pSlot->tFadeEnd = pSlot->tFadeStart + LIGHT_SLOT_FADE_MS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLightBeginFadeDown( LIGHT_SLOT * pSlot )
{
	pSlot->eFadeState = LSFS_FADEDOWN;
	pSlot->fFadeStrStart = pSlot->fStrength;
	pSlot->fFadeStrEnd = 0.f;
	pSlot->tFadeStart = AppCommonGetCurTime();
	pSlot->tFadeEnd = pSlot->tFadeStart + LIGHT_SLOT_FADE_MS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sPromoteVirtualLights( LIGHT_SLOT_CONTAINER * pContainer )
{
	// select slots for each light, preserving slots for lights that are already active
	enum { INVALID_SLOT = -1 };

	// find the ones that are still selected from last time
	for ( int v = 0; v < pContainer->nSlotsActive; v++ )
	{
		if ( pContainer->pVirtualSlots[ v ].nLightID != INVALID_ID )
		{
			int a;
			for ( a = 0; a < pContainer->nSlotsActive; a++ )
				if ( pContainer->pVirtualSlots[ v ].nLightID == pContainer->pSlots[ a ].nLightID &&
					 pContainer->pSlots[ a ].eState == LSS_ACTIVE )
					break;
			if ( a < pContainer->nSlotsActive )
			{
				// found already active light (common case)
				pContainer->pVirtualSlots[ v ].nSlot = a;
			}
			else
			{
				// need a slot
				pContainer->pVirtualSlots[ v ].nSlot = INVALID_SLOT;
			}
		}
		else
		{
			pContainer->pVirtualSlots[ v ].nSlot = INVALID_SLOT;
		}
	}

	// mark unfilled slots as empty
	for ( int a = 0; a < pContainer->nSlotsActive; a++ )
	{
		int v;
		for ( v = 0; v < pContainer->nSlotsActive; v++ )
			if ( pContainer->pVirtualSlots[ v ].nLightID != INVALID_ID &&
				 pContainer->pVirtualSlots[ v ].nSlot == a )
				break;
		if ( v >= pContainer->nSlotsActive )
		{
			// mark as empty
			pContainer->pSlots[ a ].eState = LSS_EMPTY;
		}
	}

	float fTimeCurrent = e_GameGetTimeFloat();

	// push virtual->active
	for ( int i = 0; i < pContainer->nSlotsActive; i++ )
	{
		VIRTUAL_LIGHT_SLOT * pVirtSlot = &pContainer->pVirtualSlots[ i ];

		if ( pVirtSlot->nSlot != INVALID_SLOT )
		{
			// refresh it
			LIGHT_SLOT * pSlot = &pContainer->pSlots[ pVirtSlot->nSlot ];
			ASSERT( pVirtSlot->nLightID == pSlot->nLightID );
			pSlot->eState = LSS_ACTIVE;
		}
		else
		{
			// find a slot
			for ( int a = 0; a < pContainer->nSlotsActive; a++ )
			{
				if ( pContainer->pSlots[ a ].eState == LSS_EMPTY )
				{
					pVirtSlot->nSlot = a;
					LIGHT_SLOT * pSlot = &pContainer->pSlots[ pVirtSlot->nSlot ];

					pSlot->eState		= LSS_ACTIVE;
					pSlot->nLightID		= pVirtSlot->nLightID;

					BOOL bFadeIn = TRUE;

					if ( pVirtSlot->nLightID != INVALID_ID )
					{
						// check flags to see if this light doesn't want to fade
						D3D_LIGHT * pLight = dx9_LightGet( pVirtSlot->nLightID );
						ASSERT( pLight );

						if ( pLight && pLight->nLightDef != INVALID_ID )
						{
							// or if this light is in the very first short period of it's existence
							LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_LIGHT, pLight->nLightDef );
							if ( pLightDef && pLightDef->nDurationType == DURATION_ONE_TIME )
							{
								if ( (fTimeCurrent - pLight->fBeginTime) < ( LIGHT_SLOT_FADE_MS * 0.5f ) )
									bFadeIn = FALSE;
							}
						}

						if ( pLight && ( pLight->dwFlags & LIGHT_FLAG_NOFADEIN ) )
						{
							bFadeIn = FALSE;
						}
					}

					if ( bFadeIn )
					{
						// fading it up (better response time than fading old one down)
						sLightBeginFadeUp( pSlot );
					}
					else
					{
						// light says NO FADE IN
						pSlot->fStrength = 1.0f;
						pSlot->eFadeState = LSFS_NOT_FADING;
					}
					break;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLightSlotContainerUpdate( LIGHT_SLOT_CONTAINER * pContainer )
{
	for ( int i = 0; i < pContainer->nSlotsActive; i++ )
	{
		LIGHT_SLOT * pSlot = &pContainer->pSlots[ pContainer->pVirtualSlots[ i ].nSlot ];
		if ( pSlot->eFadeState == LSFS_FADEUP || pSlot->eFadeState == LSFS_FADEDOWN )
		{
			// update fade
			float fPhase;

			// ... get phase out of times
			int nDuration = (int)( pSlot->tFadeEnd - pSlot->tFadeStart );
			int nCurrent = (int)( pSlot->tFadeEnd - AppCommonGetCurTime() );
			if ( nCurrent <= 0 )
			{
				fPhase = 1.f;
			} else
			{
				fPhase = 1.f - (float)nCurrent / nDuration;
			}

			ASSERT( fPhase >= 0.f );
			fPhase = min( fPhase, 1.f );

			if ( fPhase == 1.f )
			{
				if ( pSlot->eFadeState == LSFS_FADEUP )
				{
					//pSlot->eState = LSS_ACTIVE;	// was already "active"
					pSlot->eFadeState = LSFS_NOT_FADING;
					pSlot->fStrength = pSlot->fFadeStrEnd;
				} else
				{
					pSlot->eFadeState = LSFS_NOT_FADING;
					pSlot->fStrength = pSlot->fFadeStrEnd;

					// if replace this light with its replacement
					pSlot->nLightID = pContainer->pVirtualSlots[ i ].nLightID;
					if ( pSlot->nLightID == INVALID_ID )
						pSlot->eState = LSS_EMPTY;
				}
			} 
			else
			{
				pSlot->fStrength = LERP( pSlot->fFadeStrStart, pSlot->fFadeStrEnd, fPhase );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_LightSlotsUpdate()
{
	// each slot has a potentially different update method

	LIGHT_SLOT_CONTAINER * pContainer;

	const ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();

	// --------------------------------------------------
	//	                     dynamic diffuse point lights
	// --------------------------------------------------

	pContainer = &gtLightSlots[ LS_LIGHT_DIFFUSE_POINT ];
	ASSERTONCE( pContainer->nSlotsActive > 0 );
	if ( e_OptionState_GetActive()->get_bDynamicLights() )
	{
		// process fades
		sLightSlotContainerUpdate( pContainer );

		{
			//e_LightSlotsClear( LS_LIGHT_DIFFUSE_POINT );
			e_LightSelectClearList();

			LIGHT_SORTABLE tLight;
			VECTOR vFocus;
			e_GetViewFocusPosition( &vFocus );

			//BOOL bNoStatic = !( pEnvDef->nLocation == LEVEL_LOCATION_FLASHLIGHT );
			BOOL bNoStatic = TRUE;
			{
				PROXNODE nCurrent;
				int nLight = e_LightGetCloseFirst( LIGHT_TYPE_POINT, nCurrent, vFocus, gfDynLightSelectionRange );
				while ( nLight != INVALID_ID )
				{
					D3D_LIGHT * pLight = dx9_LightGet( nLight );

					int nLightPrev = nLight;
					PROXNODE nCurrentPrev = nCurrent;

					nLight = e_LightGetCloseNext( LIGHT_TYPE_POINT, nCurrent );

#ifdef TRACE_LIGHT_PROXMAP_ACTIONS
					if ( ! pLight )
						trace( "---- Proxmap missing [ %d ] [ %d ]\n", nCurrentPrev, nLightPrev );
#endif // TRACE_LIGHT_PROXMAP_ACTIONS

					ASSERTONCE_CONTINUE( pLight );
					if ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY
						 || ( pLight->dwFlags & ( LIGHT_FLAG_NODRAW | LIGHT_FLAG_DEAD | LIGHT_FLAG_NO_APPLY ) )
						 || ( bNoStatic && pLight->dwFlags & LIGHT_FLAG_STATIC ) )
						continue;

					tLight.pLight = pLight;
					sLightSelectCalculateWeight( tLight, vFocus );
					sLightSelectAddToList( tLight );
				}
			}

			e_LightSelectSortList( FALSE );

			// pick the top 5 and update the virtual slots
			sAddToVirtualSlots( pContainer );

			// filter into active slots
			sPromoteVirtualLights( pContainer );
		}
	}
	else
	{
		// dynamic lights disabled in settings
		pContainer->Clear();
	}

	//// --------------------------------------------------
	////	             static diffuse/specular point lights
	//// --------------------------------------------------

	// unused



	//// --------------------------------------------------
	////                  static specular-only point lights
	//// --------------------------------------------------

	pContainer = &gtLightSlots[ LS_LIGHT_SPECULAR_ONLY_POINT ];
	ASSERTONCE( pContainer->nSlotsActive > 0 );

	// process fades
	sLightSlotContainerUpdate( pContainer );

	ONCE
	{
		//e_LightSlotsClear( LS_LIGHT_DIFFUSE_POINT );
		e_LightSelectClearList();

		// No spec lights in flashlight levels.
		if ( pEnvDef->nLocation == LEVEL_LOCATION_FLASHLIGHT )
			break;

		LIGHT_SORTABLE tLight;
		VECTOR vFocus, vView;
		e_GetViewFocusPosition( &vFocus );
		e_GetViewDirection( &vView );

		// There are MUCH fewer specular lights, and some levels try to cover a huge area with one spec light.
		float fLightSelectionRange = 10000.f;

		{
			PROXNODE nCurrent;
			int nLight = e_LightGetCloseFirst( LIGHT_TYPE_POINT, nCurrent, vFocus, fLightSelectionRange );
			while ( nLight != INVALID_ID )
			{
				D3D_LIGHT * pLight = dx9_LightGet( nLight );

				int nLightPrev = nLight;
				PROXNODE nCurrentPrev = nCurrent;

				nLight = e_LightGetCloseNext( LIGHT_TYPE_POINT, nCurrent );

#ifdef TRACE_LIGHT_PROXMAP_ACTIONS
				if ( ! pLight )
					trace( "---- Proxmap missing [ %d ] [ %d ]\n", nCurrentPrev, nLightPrev );
#endif // TRACE_LIGHT_PROXMAP_ACTIONS

				ASSERT_CONTINUE( pLight );
				if (    0 == ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY )
					 || ( pLight->dwFlags & ( LIGHT_FLAG_NODRAW | LIGHT_FLAG_DEAD | LIGHT_FLAG_NO_APPLY ) )
					 || 0 == ( pLight->dwFlags & LIGHT_FLAG_STATIC ) )
					continue;

				tLight.pLight = pLight;
				sLightSelectCalculateWeightSpecular( tLight, vFocus, vView, pEnvDef );
				sLightSelectAddToList( tLight );
			}
		}

		e_LightSelectSortList( FALSE );

		// pick the top x and update the virtual slots
		sAddToVirtualSlots( pContainer );

		// filter into active slots
		sPromoteVirtualLights( pContainer );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_MakePointLightList(
	int * pnLights,
	int & nCount,
	const VECTOR & vFromPos,
	DWORD dwFlagsMustList,
	DWORD dwFlagsDontList,
	int nMax )
{
	// iterate proxmap results into a list of up to nMax point lights
	if ( nMax == 0 )
		return S_FALSE;

	PROXNODE nCurrent;
	nCount = 0;
	pnLights[ 0 ] = INVALID_ID;

	int nLight = e_LightGetCloseFirst( LIGHT_TYPE_POINT, nCurrent, vFromPos );

	D3D_LIGHT * pLight = dx9_LightGet( nLight );
	if ( pLight )
	{
		//dx9_LightApplyDefinition( pLight );
		if ( ( ! dwFlagsMustList || dwFlagsMustList == ( dwFlagsMustList & pLight->dwFlags ) ) &&
			 ( ! dwFlagsDontList || 0			    == ( dwFlagsDontList & pLight->dwFlags ) ) )
			pnLights[ nCount++ ] = nLight;
	}

	// special case: no lights in proxmap
	//if ( pnLights[ 0 ] == INVALID_ID )
	//{
	//	keytrace( "No lights?: light[%d], flagsdo[%08x] flagsdont[%08x] flagslight[%08x]\n", nLight, dwFlagsDoList, dwFlagsDontList, pLight->dwFlags );
	//	nCount = 0;
	//	return;
	//}
	//keytrace( "Found one : light[%d], flagsdo[%08x] flagsdont[%08x] flagslight[%08x]\n", nLight, dwFlagsDoList, dwFlagsDontList, pLight->dwFlags );

	while ( nCurrent != INVALID_ID )
	{
		if ( nMax > 0 && nCount >= nMax )
			break;

		nLight = e_LightGetCloseNext( LIGHT_TYPE_POINT, nCurrent );
		pLight = dx9_LightGet( nLight );
		if ( pLight )
		{
			//dx9_LightApplyDefinition( pLight );
			if ( ( ! dwFlagsMustList || dwFlagsMustList == ( dwFlagsMustList & pLight->dwFlags ) ) &&
				 ( ! dwFlagsDontList || 0			    == ( dwFlagsDontList & pLight->dwFlags ) ) )
				pnLights[ nCount++ ] = nLight;
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DebugDrawLights()
{
	for ( D3D_LIGHT* pLight = HashGetFirst(gtLights); pLight; pLight = HashGetNext(gtLights, pLight) )
	{
		sDebugDrawLight( pLight, 0x3f2f2f2f, 0 );
		sDebugDrawLightSource( pLight );
	}

	// debug display lights
	for ( int i = 0; i < NUM_LS_LIGHT_TYPES; ++i )
	{
		LIGHT_SLOT_CONTAINER * pContainer = &gtLightSlots[ i ];

		for ( int i = 0; i < pContainer->nSlotsActive; i++ )
		{
			LIGHT_SLOT * pSlot = &pContainer->pSlots[ i ];
			if ( pSlot->eState != LSS_ACTIVE )
				continue;

			D3D_LIGHT * pLight = dx9_LightGet( pSlot->nLightID );
			if ( ! pLight )
				continue;

			sDebugDrawLight( pLight, pSlot );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_GetDrawAllLights()
{
	return gbDrawLights;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_LightsInit()
{
	HashInit(gtLights, NULL, 32);
	ArrayInit(sgtSelectLights, NULL, 8);

	gfDynLightSelectionRange	 = DEFAULT_DYNAMIC_LIGHT_SELECT_RANGE;
	gfDynLightSelectionMaxRadius = DEFAULT_DYNAMIC_LIGHT_SELECT_MAX_RAD;

	e_LightsCommonInit();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_LightsDestroy()
{
	HashFree(gtLights);
	sgtSelectLights.Destroy();

	e_LightsCommonDestroy();

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_LightsReleaseAll()
{
	D3D_LIGHT * pLight = HashGetFirst( gtLights );
	while ( pLight )
	{
		D3D_LIGHT * pNext = HashGetNext( gtLights, pLight );

		e_LightRemove( pLight->nId );

		pLight = pNext;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_LightRestore(
	void * pDef )
{
	ASSERT_RETURN( pDef );
	LIGHT_DEFINITION * pLightDef = (LIGHT_DEFINITION*)pDef;

	// load spotlight umbra texture if present and light is a SPOT type
	if ( pLightDef->eType == LIGHT_TYPE_SPOT && pLightDef->szSpotUmbraTexture[ 0 ] )
	{
		if ( pLightDef->nSpotUmbraTextureID != INVALID_ID )
		{
			e_TextureReleaseRef( pLightDef->nSpotUmbraTextureID );
		}

		char szFilepath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrPrintf( szFilepath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_LIGHT, pLightDef->szSpotUmbraTexture );

		V( e_TextureNewFromFile(
			&pLightDef->nSpotUmbraTextureID,
			szFilepath,
			TEXTURE_GROUP_NONE,
			TEXTURE_NONE ) );
		e_TextureAddRef( pLightDef->nSpotUmbraTextureID );

		if ( pLightDef->nSpotUmbraTextureID == INVALID_ID )
			ErrorDialog( "Failed to load spotlight umbra texture:\n\n%s\n\nIn light:\n\n%s", szFilepath, pLightDef->tHeader.pszName );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL LightDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad )
{
	ASSERT_RETFALSE( pDef );

	e_LightRestore( pDef );

	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSelectLightsPointAndSH(
	BOUNDING_SPHERE & tBS,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	PERF( DRAW_SELECTLIGHTS );

	if ( ! e_GetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS ) )
		return S_FALSE;

	if ( tLights.IsState( MODEL_RENDER_LIGHT_DATA::STATE_SELECTED_POINT ) )
		return S_FALSE;

	if ( tLights.IsState( MODEL_RENDER_LIGHT_DATA::STATE_FILLED_LIST_POINTSH ) )
		return S_FALSE;

	tLights.SetState( MODEL_RENDER_LIGHT_DATA::STATE_FILLED_LIST_POINTSH,	TRUE );
	tLights.SetState( MODEL_RENDER_LIGHT_DATA::STATE_SELECTED_POINT,		TRUE );

	DWORD dwMustHaveFlags = LIGHT_FLAG_DEFINITION_APPLIED;
	DWORD dwDontWantFlags = LIGHT_FLAG_NODRAW | LIGHT_FLAG_DEAD | LIGHT_FLAG_NO_APPLY | LIGHT_FLAG_SPECULAR_ONLY;
	if ( ! e_OptionState_GetActive()->get_bDynamicLights() )
		dwMustHaveFlags |= LIGHT_FLAG_STATIC;

	V_RETURN( dx9_MakePointLightList(
		tLights.pnLights,
		tLights.nLights,
		tBS.C,
		dwMustHaveFlags,
		dwDontWantFlags,
		MAX_LIGHTS_PER_EFFECT ) );

	//keytrace( "tLights.nLights == %d  (raw)\n", tLights.nLights );

	// select right back into lights list (for SH later)
	V_RETURN( e_LightSelectAndSortPointList(
		tBS,
		tLights.pnLights,
		tLights.nLights,
		tLights.pnLights,
		tLights.nLights,
		MAX_LIGHTS_PER_EFFECT,
		0,
		0 ) );

	//keytrace( "tLights.nLights == %d  (sorted & selected)\n", tLights.nLights );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template<class T>
static
void sAddLight(T & tBlock, const D3D_LIGHT * pLight, const D3D_MODEL * pModel, float fDiffuseARGBScale, float fDiffuseRGBScale, bool bForceAlpha1)
{
	tBlock.pvPosWorld[ tBlock.nLights ] = D3DXVECTOR4( pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z, 1.0f );
	D3DXVec3Transform( 
		&tBlock.pvPosObject[ tBlock.nLights ],
		(D3DXVECTOR3*)&pLight->vPosition,
		(D3DXMATRIXA16*)&pModel->matScaledWorldInverse );

	// fade the light if the slot specifies it
	tBlock.pvColors[ tBlock.nLights ] = *reinterpret_cast<const D3DXVECTOR4 *>( &pLight->vDiffuse ) * fDiffuseARGBScale;
	tBlock.pvColors[ tBlock.nLights ].x *= fDiffuseRGBScale;
	tBlock.pvColors[ tBlock.nLights ].y *= fDiffuseRGBScale;
	tBlock.pvColors[ tBlock.nLights ].z *= fDiffuseRGBScale;
	if (bForceAlpha1)
	{
		tBlock.pvColors[ tBlock.nLights ].w = 1.0f;
	}

	tBlock.pvFalloffsWorld [ tBlock.nLights ] = *reinterpret_cast<const D3DXVECTOR4 *>( &pLight->vFalloff );
	// seems like this is the proper way to get into object scale -- may need debugging
	tBlock.pvFalloffsObject[ tBlock.nLights ] = *reinterpret_cast<const D3DXVECTOR4 *>( &pLight->vFalloff );
	tBlock.pvFalloffsObject[ tBlock.nLights ].y *= pModel->vScale.fX;

	tBlock.nLights++;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_AssembleLightsPoint(
	const D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS ) )
		return S_FALSE;

	if ( tLights.IsState( MODEL_RENDER_LIGHT_DATA::STATE_ASSEMBLED_POINT ) )
		return S_FALSE;

	if ( ! tLights.ShouldUpdateType( MODEL_RENDER_LIGHT_DATA::ENABLE_POINT_SELECT ) )
		return S_FALSE;

	// for effects not using global slot lights

	// used for light distance calculations
	VECTOR vCenter = ( pModel->tRenderAABBWorld.vMin + pModel->tRenderAABBWorld.vMax ) * 0.5f;
	BOUNDING_SPHERE tBS( pModel->tRenderAABBWorld );
	V( sSelectLightsPointAndSH( tBS, tLights ) );

	tLights.SetState( MODEL_RENDER_LIGHT_DATA::STATE_ASSEMBLED_POINT );

	for ( int i = 0; i < tLights.nLights; i ++ )
	{
		D3D_LIGHT * pLight = dx9_LightGet( tLights.pnLights[ i ] );
		if ( !pLight )
		{
			//keytrace( "Light index [%d] ID [%d] is NULL!\n", i, tLights.pnLights[ i ] );
			continue;
		}

		ASSERT_CONTINUE( 0 == ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY ) );

		// diffuse/spec point light

		if ( tLights.ModelPoint.nLights >= MAX_POINT_LIGHTS_PER_EFFECT )
			break;
#if 0	// CHB_TO_DELETE
		tLights.pvModelPointLightPosWorld[ tLights.nModelPointLights ] = D3DXVECTOR4( pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z, 1.0f );
		D3DXVec3Transform( 
			&tLights.pvModelPointLightPosObject[ tLights.nModelPointLights ],
			(D3DXVECTOR3*)&pLight->vPosition,
			(D3DXMATRIXA16*)&pModel->matScaledWorldInverse );

		tLights.pvModelPointLightColors[ tLights.nModelPointLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vDiffuse );
		tLights.pvModelPointLightColors[ tLights.nModelPointLights ].x *= LIGHT_INTENSITY_MDR_SCALE;
		tLights.pvModelPointLightColors[ tLights.nModelPointLights ].y *= LIGHT_INTENSITY_MDR_SCALE;
		tLights.pvModelPointLightColors[ tLights.nModelPointLights ].z *= LIGHT_INTENSITY_MDR_SCALE;

		tLights.pvModelPointLightFalloffsWorld [ tLights.nModelPointLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vFalloff );
		// seems like this is the proper way to get into object scale -- may need debugging
		tLights.pvModelPointLightFalloffsObject[ tLights.nModelPointLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vFalloff );
		tLights.pvModelPointLightFalloffsObject[ tLights.nModelPointLights ].y *= pModel->fScale;

		// useful later for per-mesh light LOD
		float fDist = tBS.Distance( pLight->vPosition );
		tLights.pfModelPointLightLODStrengths[ tLights.nModelPointLights ] = ( pLight->vFalloff.x - fDist * pLight->vFalloff.y );

		tLights.nModelPointLights++;
#else
		// useful later for per-mesh light LOD
		float fDist = tBS.Distance( pLight->vPosition );
		tLights.pfModelPointLightLODStrengths[ tLights.ModelPoint.nLights ] = ( pLight->vFalloff.x - fDist * pLight->vFalloff.y );
		sAddLight(tLights.ModelPoint, pLight, pModel, 1.0f, LIGHT_INTENSITY_MDR_SCALE, false);
#endif
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_AssembleLightsGlobalPoint(
	const D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS ) )
		return S_FALSE;

	if ( tLights.IsState( MODEL_RENDER_LIGHT_DATA::STATE_ASSEMBLED_GLOBAL_POINTS ) )
		return S_FALSE;

	if ( ! tLights.ShouldUpdateType( MODEL_RENDER_LIGHT_DATA::ENABLE_POINT_SLOT ) )
		return S_FALSE;

	tLights.SetState( MODEL_RENDER_LIGHT_DATA::STATE_ASSEMBLED_GLOBAL_POINTS );

	// global slot diffuse/spec

	BOUNDING_BOX tModelBBox_W;
	V( dxC_GetModelRenderAABBInWorld( pModel, &tModelBBox_W ) );

	{
		// for effects using the global light selections  ( background effects, mainly, and others that can't use SH )

		LIGHT_SLOT_CONTAINER * pContainer = &gtLightSlots[ LS_LIGHT_DIFFUSE_POINT ];
		for ( int i = 0; i < pContainer->nSlotsActive; i++ )
		{
			if ( pContainer->pSlots[ i ].eState != LSS_ACTIVE )
				continue;
			D3D_LIGHT * pLight = dx9_LightGet( pContainer->pSlots[ i ].nLightID );
			if ( !pLight )
				continue;

			// If this light isn't close enough to affect this model, don't add it.
			float fDist = BoundingBoxManhattanDistance( &tModelBBox_W, &pLight->vPosition );
			if ( fDist > pLight->fFalloffEnd )
				continue;

			if ( pLight->dwFlags & ( LIGHT_FLAG_NODRAW | LIGHT_FLAG_DEAD | LIGHT_FLAG_NO_APPLY ) )
				continue;

#if 0	// CHB_TO_DELETE
			tLights.pvGlobalPointLightPosWorld[ tLights.nGlobalPointLights ] = D3DXVECTOR4( pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z, 1.0f );
			D3DXVec3Transform( 
				&tLights.pvGlobalPointLightPosObject[ tLights.nGlobalPointLights ],
				(D3DXVECTOR3*)&pLight->vPosition,
				(D3DXMATRIXA16*)&pModel->matScaledWorldInverse );
			// fade the light if the slot specifies it
			tLights.pvGlobalPointLightColors[ tLights.nGlobalPointLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vDiffuse ) * pContainer->pSlots[ i ].fStrength;
			tLights.pvGlobalPointLightColors[ tLights.nGlobalPointLights ].x *= LIGHT_INTENSITY_MDR_SCALE;
			tLights.pvGlobalPointLightColors[ tLights.nGlobalPointLights ].y *= LIGHT_INTENSITY_MDR_SCALE;
			tLights.pvGlobalPointLightColors[ tLights.nGlobalPointLights ].z *= LIGHT_INTENSITY_MDR_SCALE;

			// debug
			//if ( nPointLights < 3 )
			//	tLights.pvGlobalLightColors	 [ tLights.nPointLights ] = D3DXVECTOR4( vLightColors[ nPointLights ], 1.f );
			//else
			//	tLights.pvGlobalLightColors	 [ tLights.nPointLights ] = pLight->vDiffuse;

			tLights.pvGlobalPointLightFalloffsWorld [ tLights.nGlobalPointLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vFalloff );
			// seems like this is the proper way to get into object scale -- may need debugging
			tLights.pvGlobalPointLightFalloffsObject[ tLights.nGlobalPointLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vFalloff );
			tLights.pvGlobalPointLightFalloffsObject[ tLights.nGlobalPointLights ].y *= pModel->fScale;

			tLights.nGlobalPointLights++;
#else
			sAddLight(tLights.GlobalPoint, pLight, pModel, pContainer->pSlots[i].fStrength, LIGHT_INTENSITY_MDR_SCALE, false);
#endif
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_AssembleLightsGlobalSpec(
	const D3D_MODEL * pModel,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	//if ( ! e_GetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS ) )
	//	return S_FALSE;

	//if ( tLights.dwFlags & MODEL_RENDER_LIGHT_DATA::STATE_NO_DYNAMIC_LIGHTS )
	//	return S_FALSE;

	if ( tLights.IsState( MODEL_RENDER_LIGHT_DATA::STATE_ASSEMBLED_GLOBAL_SPECS ) )
		return S_FALSE;

	if ( ! tLights.ShouldUpdateType( MODEL_RENDER_LIGHT_DATA::ENABLE_SPECULAR_SLOT ) )
		return S_FALSE;

#if 0	// CHB_TO_DELETE
	for ( int i = 0; i < MAX_SPECULAR_LIGHTS_PER_EFFECT; i++ )
	{
		tLights.pvGlobalSpecLightPosWorld	   [ i ] = D3DXVECTOR4( 10000.0f, 10000.0f, 10000.0f, 0.0f );
		tLights.pvGlobalSpecLightPosObject	   [ i ] = D3DXVECTOR4( 10000.0f, 10000.0f, 10000.0f, 0.0f );
		tLights.pvGlobalSpecLightColors		   [ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f, -1.0f );
		tLights.pvGlobalSpecLightFalloffsWorld [ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f,  0.0f );
		tLights.pvGlobalSpecLightFalloffsObject[ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f,  0.0f );
	}
#else
	tLights.GlobalSpec.ResetForSpecular();
#endif

	tLights.SetState( MODEL_RENDER_LIGHT_DATA::STATE_ASSEMBLED_GLOBAL_SPECS );

	// global slot spec

	BOUNDING_BOX tModelBBox_W;
	V( dxC_GetModelRenderAABBInWorld( pModel, &tModelBBox_W ) );

	{
		// for effects using the global light selections  ( background effects, mainly, and others that can't use SH )

		LIGHT_SLOT_CONTAINER * pContainer = &gtLightSlots[ LS_LIGHT_SPECULAR_ONLY_POINT ];

		int nMaxSpecs = MIN( pContainer->nSlotsActive, MAX_SPECULAR_LIGHTS_PER_EFFECT );
		if ( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_SPECULAR_FAVOR_FACING )
		{
			// limit selection to the 1 best light 
			nMaxSpecs = 1;
		}

		for ( int i = 0; i < nMaxSpecs; i++ )
		{
			if ( pContainer->pSlots[ i ].eState != LSS_ACTIVE )
				continue;
			D3D_LIGHT * pLight = dx9_LightGet( pContainer->pSlots[ i ].nLightID );
			if ( !pLight )
				continue;

			// If this light isn't close enough to affect this model, don't add it.
			float fDist = BoundingBoxManhattanDistance( &tModelBBox_W, &pLight->vPosition );
			if ( fDist > pLight->fFalloffEnd )
				continue;

#if 0	// CHB_TO_DELETE
			tLights.pvGlobalSpecLightPosWorld[ tLights.nGlobalSpecLights ] = D3DXVECTOR4( pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z, 1.0f );
			D3DXVec3Transform( 
				&tLights.pvGlobalSpecLightPosObject[ tLights.nGlobalSpecLights ],
				(D3DXVECTOR3*)&pLight->vPosition,
				(D3DXMATRIXA16*)&pModel->matScaledWorldInverse );

			//float fDistance = sqrtf( VectorDistanceSquared( pLight->vPosition, *pState->pvEyeLocation ) );
			//float fStrength = pLight->vFalloff.x - fDistance * pLight->vFalloff.y;
			//// saturating in the shader -- this way, it can use distance for selection
			////fStrength = SATURATE( fStrength );

			// fade the light if the slot specifies it
			D3DXVECTOR4 vDiffuse = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vDiffuse ) * pContainer->pSlots[ i ].fStrength;
			vDiffuse.w = 1.f;  // a weight for selection purposes

			tLights.pvGlobalSpecLightColors		   [ tLights.nGlobalSpecLights ] = vDiffuse;
			tLights.pvGlobalSpecLightFalloffsWorld [ tLights.nGlobalSpecLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vFalloff );
			// seems like this is the proper way to get into object scale -- may need debugging
			tLights.pvGlobalSpecLightFalloffsObject[ tLights.nGlobalSpecLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vFalloff );
			tLights.pvGlobalSpecLightFalloffsObject[ tLights.nGlobalSpecLights ].y *= pModel->fScale;
			tLights.nGlobalSpecLights++;
#else
			sAddLight(tLights.GlobalSpec, pLight, pModel, pContainer->pSlots[i].fStrength, 1.0f, true);
#endif
		}
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_AssembleLightsSH(
	BOUNDING_SPHERE & tBS,
	float fScale,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS ) )
		return S_FALSE;

	if ( tLights.IsState( MODEL_RENDER_LIGHT_DATA::STATE_ASSEMBLED_SH ) )
		return S_FALSE;

	if ( ! tLights.ShouldUpdateType( MODEL_RENDER_LIGHT_DATA::ENABLE_SH ) )
		return S_FALSE;

	// fill the point/sh list, and sort/select to isolate the top x
	V_RETURN( sSelectLightsPointAndSH( tBS, tLights ) );

	tLights.SetState( MODEL_RENDER_LIGHT_DATA::STATE_ASSEMBLED_SH );


	// assemble SH lights into coef-baking-ready data
	int nTotalLights = tLights.nLights;
	//BOUNDING_BOX tBBox;
	//e_GetModelRenderAABBInWorldWithRagdoll( pModel->nId, &tBBox );

	ASSERT_RETFAIL( tLights.nSHLights == 0 );

	// just to let me use a statically-allocated stack array and avoid malloc
	if ( nTotalLights > MAX_SH_LIGHTS_PER_EFFECT )
		nTotalLights = MAX_SH_LIGHTS_PER_EFFECT;

	for ( int i = 0; i < nTotalLights; i ++ )
	{
		int nLightId = tLights.pnLights[ i ];
		if ( nLightId == INVALID_ID )
			continue;

		D3D_LIGHT* pLight = dx9_LightGet( nLightId );
		if (!pLight)
			continue;

		ASSERT_CONTINUE( 0 == ( pLight->dwFlags & LIGHT_FLAG_SPECULAR_ONLY ) );

		//float fDistanceSquared = BoundingBoxDistanceSquared( &tBBox, &pLight->vPosition );
		float fDistanceSquared = VectorDistanceSquared( tBS.C, pLight->vPosition );
		fDistanceSquared *= fScale * fScale;
		//if ( fDistanceSquared < pLight->fFalloffEnd * pLight->fFalloffEnd )
		{
			float fDistance = sqrtf( fDistanceSquared );
			float fStrength = pLight->vFalloff.x - fDistance * pLight->vFalloff.y;
			fStrength = SATURATE( fStrength );

			// SH is handled in world space
			tLights.pvSHLightPos   [ tLights.nSHLights ] = D3DXVECTOR4( pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z, 1.0f );
			tLights.pvSHLightColors[ tLights.nSHLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vDiffuse );

			// debug
			//if ( tLights.nSHLights < 3 )
			//	tLights.pvSHLightColors[ tLights.nSHLights ] = D3DXVECTOR4( vLightColors[ tLights.nSHLights ], 1.f );
			//else
			//	tLights.pvSHLightColors[ tLights.nSHLights ] = pLight->vDiffuse;

			//debug
			//tLights.pvSHLightColors[ tLights.nSHLights ].x = 0.f;
			//tLights.pvSHLightColors[ tLights.nSHLights ].y = 1.f;
			//tLights.pvSHLightColors[ tLights.nSHLights ].x = 1.f;

			tLights.pvSHLightColors[ tLights.nSHLights ] *= fStrength;
			tLights.nSHLights++;
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_SelectLightsSpecularOnly(
	const D3D_MODEL * pModel,
	const DRAWLIST_STATE * pState,
	const ENVIRONMENT_DEFINITION * pEnvDef,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	PERF( DRAW_SELECTLIGHTS );

	//if ( ! e_GetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS ) )
	//	return S_FALSE;

	if ( tLights.IsState( MODEL_RENDER_LIGHT_DATA::STATE_SELECTED_SPEC ) )
		return S_FALSE;

	if ( ! tLights.ShouldUpdateType( MODEL_RENDER_LIGHT_DATA::ENABLE_SPECULAR_SELECT ) )
		return S_FALSE;

	ASSERT_RETFAIL( pState && pState->pvEyeLocation );

	tLights.SetState( MODEL_RENDER_LIGHT_DATA::STATE_SELECTED_SPEC );

	// set to default far away light in case not enough lights exist nearby
#if 0	// CHB_TO_DELETE
	for ( int i = 0; i < MAX_SPECULAR_LIGHTS_PER_EFFECT; i++ )
	{
		tLights.pvModelSpecularLightPosWorld	  [ i ] = D3DXVECTOR4( 10000.0f, 10000.0f, 10000.0f, 0.0f );
		tLights.pvModelSpecularLightPosObject	  [ i ] = D3DXVECTOR4( 10000.0f, 10000.0f, 10000.0f, 0.0f );
		tLights.pvModelSpecularLightColors		  [ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f, -1.0f );
		tLights.pvModelSpecularLightFalloffsWorld [ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f,  0.0f );
		tLights.pvModelSpecularLightFalloffsObject[ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f,  0.0f );
	}
#else
	tLights.ModelSpec.ResetForSpecular();
#endif
	ASSERT_RETFAIL( tLights.ModelSpec.nLights == 0 );

	if ( ! AppIsTugboat() )
	{
		int pnLights[ MAX_LIGHTS_PER_EFFECT ];
		int nLights;
		V_RETURN( dx9_MakePointLightList(
			pnLights,
			nLights,
			*pState->pvEyeLocation,
			LIGHT_FLAG_SPECULAR_ONLY | LIGHT_FLAG_DEFINITION_APPLIED,
			LIGHT_FLAG_NODRAW | LIGHT_FLAG_DEAD | LIGHT_FLAG_NO_APPLY,
			MAX_LIGHTS_PER_EFFECT ) );

		// sort and select specular-only background lights
		{
			int pnSpecLights[ MAX_SPECULAR_LIGHTS_PER_EFFECT ];
			int nSpecLights;
			int nMaxLights = MAX_SPECULAR_LIGHTS_PER_EFFECT;

			if ( pEnvDef->dwDefFlags & ENVIRONMENTDEF_FLAG_SPECULAR_FAVOR_FACING )
			{
				// limit selection to the 1 best light 
				nMaxLights = 1;
			}

			V_RETURN( e_LightSelectAndSortSpecList(
				pEnvDef,
				pnLights,
				nLights,
				pnSpecLights,
				nSpecLights,
				nMaxLights,
				0,
				0 ) );

			for ( int i = 0; i < nSpecLights; i++ )
			{
				D3D_LIGHT * pLight = dx9_LightGet( pnSpecLights[ i ] );
				ASSERT_CONTINUE( pLight );
#if 0	// CHB_TO_DELETE
				tLights.pvModelSpecularLightPosWorld[ tLights.nModelSpecularLights ] = D3DXVECTOR4( pLight->vPosition.x, pLight->vPosition.y, pLight->vPosition.z, 1.0f );
				D3DXVec3Transform( 
					&tLights.pvModelSpecularLightPosObject[ tLights.nModelSpecularLights ],
					(D3DXVECTOR3*)&pLight->vPosition,
					(D3DXMATRIXA16*)&pModel->matScaledWorldInverse );

				//float fDistance = sqrtf( VectorDistanceSquared( pLight->vPosition, *pState->pvEyeLocation ) );
				//float fStrength = pLight->vFalloff.x - fDistance * pLight->vFalloff.y;
				//// saturating in the shader -- this way, it can use distance for selection
				////fStrength = SATURATE( fStrength );

				D3DXVECTOR4 vDiffuse = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vDiffuse );
				vDiffuse.w = 1.f;  // a weight for selection purposes
				tLights.pvModelSpecularLightColors		  [ tLights.nModelSpecularLights ] = vDiffuse;
				tLights.pvModelSpecularLightFalloffsWorld [ tLights.nModelSpecularLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vFalloff );
				// seems like this is the proper way to get into object scale -- may need debugging
				tLights.pvModelSpecularLightFalloffsObject[ tLights.nModelSpecularLights ] = *reinterpret_cast<D3DXVECTOR4*>( &pLight->vFalloff );
				tLights.pvModelSpecularLightFalloffsObject[ tLights.nModelSpecularLights ].y *= pModel->fScale;
				tLights.nModelSpecularLights++;
#else
				sAddLight(tLights.ModelSpec, pLight, pModel, 1.0f, 1.0f, true);
#endif
			}
		}
	}

	// hard set the max number of specular because we don't branch on exact specular counts anymore
	// should we again?  this makes things more expensive in a frequent case... maybe spec on/off?
	tLights.ModelSpec.nLights = MAX_SPECULAR_LIGHTS_PER_EFFECT;

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

template< int O >
static PRESULT sComputeLightsSH_Ex(
	const D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights,
	SH_COEFS_RGB<O> & tSHCoefsBase,
	SH_COEFS_RGB<O> * tSHCoefsPointLights,
	MODEL_RENDER_LIGHT_DATA::STATE FLAG_INITIALIZED_SH,
	MODEL_RENDER_LIGHT_DATA::STATE FLAG_COMPUTED_SH )
{
	if ( ! tLights.ShouldUpdateType( MODEL_RENDER_LIGHT_DATA::ENABLE_SH ) )
		return S_FALSE;

	if ( 0 == ( tLights.IsState( FLAG_INITIALIZED_SH ) ) )
	{
		dx9_SHCoefsInit( &tSHCoefsBase );
		for ( int i = 0; i < MAX_POINT_LIGHTS_PER_EFFECT; i++ )
			dx9_SHCoefsInit( &tSHCoefsPointLights[ i ] );

		tLights.SetState( FLAG_INITIALIZED_SH );
	}

	if ( ! e_GetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS ) )
		return S_FALSE;

	if ( tLights.IsState( FLAG_COMPUTED_SH ) )
		return S_FALSE;

	// used for light distance calculations
	BOUNDING_SPHERE tBS( pModel->tRenderAABBWorld );

	// fill the point/sh list, sort/select to isolate the top x, and assemble SH lights
	V_RETURN( dxC_AssembleLightsSH( tBS, pModel->vScale.fX, tLights ) );

	tLights.SetState( FLAG_COMPUTED_SH );

	// bake into SH coefs
	V_RETURN( dx9_ComputeSHPointLights( pModel, &tLights, tSHCoefsBase, tSHCoefsPointLights ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ComputeLightsSH_O2(
	const D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	return sComputeLightsSH_Ex(
		pModel,
		tLights,
		tLights.tSHCoefsBase_O2,
		tLights.tSHCoefsPointLights_O2,
		MODEL_RENDER_LIGHT_DATA::STATE_INITIALIZED_SH_O2,
		MODEL_RENDER_LIGHT_DATA::STATE_COMPUTED_SH_O2 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ComputeLightsSH_O3(
	const D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights )
{
	return sComputeLightsSH_Ex(
		pModel,
		tLights,
		tLights.tSHCoefsBase_O3,
		tLights.tSHCoefsPointLights_O3,
		MODEL_RENDER_LIGHT_DATA::STATE_INITIALIZED_SH_O3,
		MODEL_RENDER_LIGHT_DATA::STATE_COMPUTED_SH_O3 );
}
