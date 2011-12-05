//----------------------------------------------------------------------------
// c_attachment.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "c_attachment.h"
#include "definition.h"
#include "c_particles.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_footsteps.h"


#include "excel.h"
#include "c_appearance.h"
#include "units.h" // also includes game.h
#include "wardrobe.h"
#include "filepaths.h"

#include "e_definition.h"
#include "e_light.h"
#include "e_model.h"
#include "e_anim_model.h"

//#ifdef HAMMER
#include "..\hammer\symbol.h"
/*
#include "hammer.h"
#include "dataDlg.h"
#include "filepaths.h"

*/
const SYMBOL gtAttachmentTypeSymbols[ ] = 
{
	{ ATTACHMENT_LIGHT,		"Light",	},
	{ ATTACHMENT_PARTICLE,	"Particle",	},
	{ ATTACHMENT_MODEL,		"Model",	},
	{ ATTACHMENT_SOUND,		"Sound",	},
	{ ATTACHMENT_FOOTSTEP,	"Footstep",	},
	// don't include particle rope end - it is not supported for direct use in the tools
	{	0,	NULL }, // last line
};
//#endif

CHash<ATTACHMENT_HOLDER> gtAttachmentHolders;
static VECTOR sgvCameraFocus(0,0,0);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentDefinitionLoad( 
	GAME * pGame,
	ATTACHMENT_DEFINITION & tDef, 
	int nAppearanceDefId, 
	const char * pszFolder )
{
	if ( tDef.nAttachedDefId == INVALID_ID )
	{
		switch ( tDef.eType )
		{
		case ATTACHMENT_MODEL:
			tDef.nAttachedDefId = AppearanceDefinitionLoad( pGame, tDef.pszAttached, pszFolder );
			break;

		case ATTACHMENT_PARTICLE:
			tDef.nAttachedDefId = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, tDef.pszAttached );
			c_ParticleSystemLoad(tDef.nAttachedDefId);
			break;

		case ATTACHMENT_LIGHT:
			tDef.nAttachedDefId = DefinitionGetIdByName( DEFINITION_GROUP_LIGHT, tDef.pszAttached );
			break;

		case ATTACHMENT_SOUND:
			tDef.nAttachedDefId = ExcelGetLineByStringIndex( NULL, DATATABLE_SOUNDS, tDef.pszAttached );
			CLT_VERSION_ONLY( c_SoundLoad( tDef.nAttachedDefId ) );
			break;

		case ATTACHMENT_FOOTSTEP:
			tDef.nAttachedDefId = ExcelGetLineByStringIndex( NULL, DATATABLE_FOOTSTEPS, tDef.pszAttached );
#if !ISVERSION(SERVER_VERSION)
			c_FootstepLoad( tDef.nAttachedDefId );
#endif
			break;

		default:
			tDef.nAttachedDefId = INVALID_ID;
			break;
		}

	}

	if ( nAppearanceDefId == INVALID_ID )
	{
		tDef.nBoneId = INVALID_ID;
	}
	else if ( tDef.pszBone[ 0 ] != 0 )
	{
		ASSERT( nAppearanceDefId != INVALID_ID );
		tDef.nBoneId = c_AppearanceDefinitionGetBoneNumber( nAppearanceDefId, tDef.pszBone );
	} 
}


void c_AttachmentDefinitionLoadCallback( 
	ATTACHMENT_DEFINITION & tDef, 
	int nAppearanceDefId, 
	const char * pszFolder )
{
	c_AttachmentDefinitionLoad( AppGetCltGame(), tDef, nAppearanceDefId, pszFolder );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sAttachmentLoadAppearanceSounds(void * pDef, EVENT_DATA * pEventData)
{
#if !ISVERSION(SERVER_VERSION)
	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *)pDef;
	c_AppearanceDefinitionFlagSoundsForLoad(pAppearanceDef, TRUE);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_AttachmentDefinitionFlagSoundsForLoad( 
	GAME* pGame,
	ATTACHMENT_DEFINITION& tDef,
	BOOL bLoadNow)
{
	if(tDef.nAttachedDefId == INVALID_ID)
	{
		c_AttachmentDefinitionLoad(pGame, tDef, INVALID_ID, "");
	}
	if ( tDef.nAttachedDefId != INVALID_ID )
	{
		switch ( tDef.eType )
		{
		case ATTACHMENT_MODEL:
			{
				APPEARANCE_DEFINITION * pAppearanceDef = AppearanceDefinitionGet(tDef.nAttachedDefId);
				if(pAppearanceDef)
				{
					c_AppearanceDefinitionFlagSoundsForLoad(pAppearanceDef, bLoadNow);
				}
				else
				{
					EVENT_DATA eventData;
					DefinitionAddProcessCallback(DEFINITION_GROUP_APPEARANCE, tDef.nAttachedDefId, c_sAttachmentLoadAppearanceSounds, &eventData);
				}
			}
			break;

		case ATTACHMENT_PARTICLE:
			c_ParticleSystemFlagSoundsForLoad(tDef.nAttachedDefId, bLoadNow);
			break;

		case ATTACHMENT_LIGHT:
			break;

		case ATTACHMENT_SOUND:
			{
				CLT_VERSION_ONLY( c_SoundFlagForLoad( tDef.nAttachedDefId, bLoadNow ) );
			}
			break;

		case ATTACHMENT_FOOTSTEP:
#if !ISVERSION(SERVER_VERSION)
			c_FootstepFlagForLoad( tDef.nAttachedDefId, bLoadNow );
#endif
			break;

		default:
			break;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sApplyHolderRegion( 
	ATTACHMENT * pAttachment, 
	int nRegionID )
{
	c_AttachmentSetRegion( *pAttachment, nRegionID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sApplyHolderFlags( 
	ATTACHMENT * pAttachment, 
	DWORD dwFlags, 
	BOOL bValue )
{
	if ( dwFlags & ATTACHMENT_HOLDER_FLAGS_NOLIGHTS )
	{
		switch ( pAttachment->eType )
		{
		case ATTACHMENT_LIGHT:
			e_LightSetFlags( pAttachment->nAttachedId, LIGHT_FLAG_NODRAW, bValue );
			break;

		case ATTACHMENT_PARTICLE:
			c_ParticleSystemSetNoLights( pAttachment->nAttachedId, bValue );
			break;
		}
	} 

	if ( ((dwFlags & ATTACHMENT_HOLDER_FLAGS_NOSOUND) || (dwFlags & ATTACHMENT_HOLDER_FLAGS_DONTPLAYSOUND)) && !(pAttachment->dwFlags & ATTACHMENT_FLAGS_DONT_MUTE_SOUND) )
	{
		switch ( pAttachment->eType )
		{
		case ATTACHMENT_SOUND:
			c_SoundSetMute( pAttachment->nAttachedId, bValue );
			break;

		case ATTACHMENT_PARTICLE:
			c_ParticleSystemSetNoSound( pAttachment->nAttachedId, bValue );
			break;
		}
	} 

	if ( dwFlags & ATTACHMENT_HOLDER_FLAGS_VISIBLE )
	{
		c_AttachmentSetVisible( *pAttachment, bValue );
	}

	if ( dwFlags & ATTACHMENT_HOLDER_FLAGS_NODRAW )
	{
		c_AttachmentSetDraw( *pAttachment, ! bValue );
	}

	if ( dwFlags & ATTACHMENT_HOLDER_FLAGS_FIRST_PERSON_PROJ )
	{
		c_AttachmentSetFirstPerson( *pAttachment, bValue );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sgnAttachmentId = 0;
int c_sAttachmentNew( 
	ATTACHMENT_DEFINITION & tDef, 
	ATTACHMENT_HOLDER & tHolder, 
	ATTACHMENT & tAttachment, 
	ROOMID idRoom, 
	const MATRIX & tWorldMatrix,
	const ATTACHMENT::OWNER & tOwner,
	int nAttachedId /*= INVALID_ID*/ )
{
	tAttachment.nId					= sgnAttachmentId++;
	tAttachment.eType				= tDef.eType;
	tAttachment.dwFlags				= tDef.dwFlags;
	tAttachment.vNormalInObject		= tDef.vNormal;
	tAttachment.vPositionInObject	= tDef.vPosition;
	tAttachment.nBoneId				= tDef.nBoneId;
	tAttachment.nVolume				= tDef.nVolume;
	tAttachment.vScale				= tDef.vScale;
	tAttachment.fRotation			= DegreesToRadians( tDef.fRotation );
	tAttachment.fYaw				= DegreesToRadians( tDef.fYaw );
	tAttachment.fPitch				= DegreesToRadians( tDef.fPitch );
	tAttachment.fRoll				= DegreesToRadians( tDef.fRoll );
	tAttachment.tOwner				= tOwner;
	PStrCopy(tAttachment.pszBoneName, tDef.pszBone, MAX_XML_STRING_LENGTH);

	if ( VectorLengthSquared( tAttachment.vNormalInObject ) == 0.0f )
		tAttachment.vNormalInObject.fX = -1.0f;

	if ( nAttachedId == INVALID_ID )
	{
		switch ( tDef.eType )
		{
		case ATTACHMENT_PARTICLE:
			ASSERT_RETINVALID ( ( tDef.dwFlags & ATTACHMENT_FLAGS_PARTICLE_ROPE_END ) == 0 );
			{
				VECTOR vPosition;
				VECTOR vNormal;
				MatrixMultiply( &vPosition,     &tDef.vPosition, &tWorldMatrix );
				MatrixMultiplyNormal( &vNormal, &tAttachment.vNormalInObject,	&tWorldMatrix );
				VectorNormalize( vNormal, vNormal );
#ifdef INTEL_CHANGES
				tAttachment.nAttachedId = c_ParticleSystemNew( tDef.nAttachedDefId, &vPosition, &vNormal, idRoom, INVALID_ID, NULL, FALSE, FALSE, FALSE, &tWorldMatrix );
#else
				tAttachment.nAttachedId = c_ParticleSystemNew( tDef.nAttachedDefId, &vPosition, &vNormal, idRoom, INVALID_ID, NULL, FALSE, FALSE, &tWorldMatrix );
#endif
			} 
			break;
		case ATTACHMENT_PARTICLE_ROPE_END:
			ASSERT_RETINVALID( nAttachedId != INVALID_ID );
			break;

		case ATTACHMENT_MODEL:
			{
				tAttachment.nAttachedId = c_AppearanceNew( tDef.nAttachedDefId,  APPEARANCE_NEW_PLAY_IDLE );
				c_ModelSetScale( tAttachment.nAttachedId, tAttachment.vScale );
			}
			break;
		case ATTACHMENT_LIGHT:
			{
				VECTOR vPosition;
				MatrixMultiply( &vPosition, &tDef.vPosition, &tWorldMatrix );
				tAttachment.nAttachedId = e_LightNew(tDef.nAttachedDefId, e_LightPriorityTypeGetValue( tHolder.eLightPriority ), vPosition );
			}
			break;
		case ATTACHMENT_SOUND:
			{
				if(!(tHolder.dwFlags & ATTACHMENT_HOLDER_FLAGS_DONTPLAYSOUND))
				{
					VECTOR vPosition;
					MatrixMultiply( &vPosition, &tDef.vPosition, &tWorldMatrix );
					SOUND_PLAY_EXINFO tExInfo(tDef.nVolume);
					tExInfo.bForce2D = tDef.dwFlags & ATTACHMENT_FLAGS_FORCE_2D;
					tExInfo.bForce3D = tDef.dwFlags & ATTACHMENT_FLAGS_FORCE_3D;
					tAttachment.nAttachedId = c_SoundPlay( tDef.nAttachedDefId, &vPosition, tExInfo );
				}
			}
			break;
		case ATTACHMENT_FOOTSTEP:
			{
				if(!(tHolder.dwFlags & ATTACHMENT_HOLDER_FLAGS_DONTPLAYSOUND))
				{
					VECTOR vPosition;
					MatrixMultiply( &vPosition, &tDef.vPosition, &tWorldMatrix );
#if !ISVERSION(SERVER_VERSION)
					tAttachment.nAttachedId = c_FootstepPlay( tDef.nAttachedDefId, vPosition, TEST_MASK(tDef.dwFlags, ATTACHMENT_FLAGS_FORCE_2D) );
#else
					tAttachment.nAttachedId	= INVALID_ID;
#endif
					tAttachment.eType = ATTACHMENT_SOUND;
				}
			}
			break;
		default:
			tAttachment.nAttachedId	= INVALID_ID;
			break;
		}
	} else {
		tAttachment.nAttachedId = nAttachedId;
	}

	if ( tDef.eType == ATTACHMENT_MODEL )
	{
		e_ModelSetFlagbit( tAttachment.nAttachedId, MODEL_FLAGBIT_ATTACHMENT, 	! TEST_MASK( tAttachment.dwFlags, ATTACHMENT_FUNC_FLAG_FLOAT ) );
		e_ModelSetFlagbit( tAttachment.nAttachedId, MODEL_FLAGBIT_MIRRORED,		TEST_MASK( tAttachment.dwFlags, ATTACHMENT_FLAGS_MIRROR ) );
	}
	
	c_sApplyHolderFlags( &tAttachment, tHolder.dwFlags, TRUE );
	c_sApplyHolderRegion( &tAttachment, tHolder.nRegionID );

	ASSERT( tAttachment.nId >= 0 );

	return tAttachment.nId;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_AttachmentNew( 
	ATTACHMENT_DEFINITION& tDef, 
	ATTACHMENT_HOLDER& tHolder, 
	ROOMID idRoom, 
	const MATRIX& tWorldMatrix,
	const ATTACHMENT::OWNER & tOwner,
	int nAttachedId)
{
	tHolder.nCount++;
	if ( tHolder.nCount > tHolder.nAllocated )
	{
		if ( tHolder.nAllocated == 0 )
			tHolder.nAllocated = 1;
		tHolder.nAllocated *= 2;
		tHolder.pAttachments = (ATTACHMENT *) REALLOC( NULL, tHolder.pAttachments, sizeof( ATTACHMENT ) * tHolder.nAllocated );
	}
	return c_sAttachmentNew( tDef, tHolder, tHolder.pAttachments[ tHolder.nCount - 1 ], idRoom, tWorldMatrix, tOwner, nAttachedId );
}

int c_AttachmentNewCallback(
	int nModelID,
	struct ATTACHMENT_DEFINITION * pDef)
{
	ASSERT_RETINVALID( pDef );
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );
	int nRoomID = c_ModelGetRoomID( nModelID );
	const MATRIX * pMat = e_ModelGetWorldScaled( nModelID );
	return c_AttachmentNew( *pDef, *pHolder, nRoomID, *pMat, ATTACHMENT::OWNER() );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentNewStaticLight( 
	GAME * pGame,
	ATTACHMENT_HOLDER & tHolder, 
	ROOMID idRoom, 
	const MATRIX & tWorldMatrix, 
	STATIC_LIGHT_DEFINITION & tStaticLightDef, 
	BOOL bSpecularOnly )
{
	tHolder.nCount++;
	if ( tHolder.nCount > tHolder.nAllocated )
	{
		if ( tHolder.nAllocated == 0 )
			tHolder.nAllocated = 1;
		tHolder.nAllocated *= 2;
		tHolder.pAttachments = (ATTACHMENT *) REALLOC( NULL, tHolder.pAttachments, sizeof( ATTACHMENT ) * tHolder.nAllocated );
	}
	ATTACHMENT * pAttachment = &tHolder.pAttachments[ tHolder.nCount - 1 ];
	ZeroMemory( pAttachment, sizeof(ATTACHMENT) );
	pAttachment->nId				= sgnAttachmentId++;
	pAttachment->eType				= ATTACHMENT_LIGHT;
	pAttachment->vNormalInObject	= VECTOR( 1, 0, 0 );
	pAttachment->vPositionInObject	= tStaticLightDef.vPosition;
	pAttachment->nBoneId			= INVALID_ID;
	pAttachment->nAttachedId		= e_LightNewStatic ( &tStaticLightDef, bSpecularOnly );

	c_AttachmentMove( *pAttachment, INVALID_ID, tWorldMatrix, idRoom );

	ASSERT( e_LightExists( pAttachment->nAttachedId ) );
}


void c_AttachmentNewStaticLightCallback( 
	int nModelID,
	STATIC_LIGHT_DEFINITION & tStaticLightDef, 
	BOOL bSpecularOnly )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );
	ASSERT( pHolder );
	int nRoomID = c_ModelGetRoomID( nModelID );
	const MATRIX * pMat = e_ModelGetWorldScaled( nModelID );
	c_AttachmentNewStaticLight( AppGetCltGame(), *pHolder, nRoomID, *pMat, tStaticLightDef, bSpecularOnly );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL c_AttachmentCanDestroy( 
	ATTACHMENT & tAttachment )
{
	switch ( tAttachment.eType )
	{
	case ATTACHMENT_PARTICLE:
	case ATTACHMENT_PARTICLE_ROPE_END:
		return c_ParticleSystemCanDestroy( tAttachment.nAttachedId );

	case ATTACHMENT_LIGHT:
	case ATTACHMENT_MODEL:
		return TRUE;

	case ATTACHMENT_SOUND: 
		return c_SoundCanDestroy( tAttachment.nAttachedId );

	default:
		return TRUE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentDestroy( 
	ATTACHMENT & tAttachment,
	DWORD dwFlags )
{
	switch ( tAttachment.eType )
	{
	case ATTACHMENT_PARTICLE:
	case ATTACHMENT_PARTICLE_ROPE_END:
		c_ParticleSystemKill( tAttachment.nAttachedId, TRUE, (dwFlags & ATTACHMENT_FUNC_FLAG_FORCE_DESTROY) != 0 );
		break;
	case ATTACHMENT_LIGHT:
		e_LightRemove( tAttachment.nAttachedId );
		break;
	case ATTACHMENT_MODEL:
		c_AppearanceDestroy( tAttachment.nAttachedId );
		break;
	case ATTACHMENT_SOUND:
		CLT_VERSION_ONLY( c_SoundStop( tAttachment.nAttachedId ) );
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentDestroy( 
	int nId, 
	ATTACHMENT_HOLDER & tHolder,
	DWORD dwFlags )
{
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		if ( tHolder.pAttachments[ i ].nId == nId )
		{
			BOOL bFloat = (dwFlags & ATTACHMENT_FUNC_FLAG_FLOAT) != 0;
			if ( ! c_AttachmentCanDestroy( tHolder.pAttachments[ i ] ) && ! bFloat )
				return;
			if ( ! bFloat )
				c_AttachmentDestroy( tHolder.pAttachments[ i ], dwFlags );
			if ( i < tHolder.nCount - 1)
			{
				MemoryMove( tHolder.pAttachments + i, (tHolder.nCount - i) * sizeof(ATTACHMENT), tHolder.pAttachments + i + 1, (tHolder.nCount - i - 1) * sizeof(ATTACHMENT));
			}
			tHolder.nCount--;
			return;
		}
	}

	// check the attachments on the models attached to the holder
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		if ( tHolder.pAttachments[ i ].eType == ATTACHMENT_MODEL )
		{
			c_ModelAttachmentRemove( tHolder.pAttachments[ i ].nAttachedId, nId, dwFlags );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentDestroyAll( 
	ATTACHMENT_HOLDER & tHolder,
	DWORD dwFlags )
{
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		c_AttachmentDestroy( tHolder.pAttachments[ i ], dwFlags );
	}
	tHolder.nCount = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentDestroyAllByOwner( 
	ATTACHMENT_HOLDER & tHolder,
	ATTACHMENT::OWNER tOwner,
	DWORD dwFlags )
{
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		// if the skill is passed as INVALID_ID - then remove anything with a skill
		if ( tHolder.pAttachments[ i ].tOwner.nType == tOwner.nType &&
			( tOwner.nId != INVALID_ID && tHolder.pAttachments[ i ].tOwner.nId == tOwner.nId ||
			  tOwner.nId == INVALID_ID ) )
		{
			if ( ! c_AttachmentCanDestroy( tHolder.pAttachments [ i ] ) )
				continue;

			if ( (dwFlags & ATTACHMENT_FUNC_FLAG_FLOAT) == 0 )
				c_AttachmentDestroy( tHolder.pAttachments[ i ], dwFlags );
			if ( i < tHolder.nCount - 1)
			{
				MemoryMove( tHolder.pAttachments + i, (tHolder.nCount - i) * sizeof(ATTACHMENT), tHolder.pAttachments + i + 1, (tHolder.nCount - i - 1) * sizeof(ATTACHMENT));
			}
			tHolder.nCount--;
			i--;
		} else if ( tHolder.pAttachments[ i ].eType == ATTACHMENT_MODEL && (dwFlags & ATTACHMENT_FUNC_FLAG_REMOVE_CHILDREN) != 0 )
		{
			c_ModelAttachmentRemoveAllByOwner( tHolder.pAttachments[ i ].nAttachedId, tOwner.nType, tOwner.nId, dwFlags );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentMove( 
	ATTACHMENT& tAttachment, 
	int nModelId, 
	const MATRIX& mModelWorld, 
	ROOMID idRoom,
	MODEL_SORT_TYPE Sort)
{
	if(tAttachment.dwFlags & ATTACHMENT_FLAGS_DONT_MOVE)
	{
		return;
	}

	MATRIX mWorld;
	if ( tAttachment.nBoneId != INVALID_ID )
	{
		MATRIX mBone;
		mBone.Identity();
		BOOL fRet = c_AppearanceGetBoneMatrix ( nModelId, &mBone, tAttachment.nBoneId );
		ASSERT( fRet );
		MatrixMultiply( &mWorld, &mBone, &mModelWorld );
	} else {
		mWorld = mModelWorld;
	}

	VECTOR vPosition;
	VECTOR vNormal;
	if( tAttachment.dwFlags & ATTACHMENT_FLAGS_NEED_BBOX_UPDATE )
	{
		const BOUNDING_BOX* pBBox = e_ModelDefinitionGetBoundingBox( e_ModelGetDefinition( nModelId ), LOD_ANY );
		if( pBBox )
		{
			float fX = RandGetFloat( AppGetCltGame() );
			float fY = RandGetFloat( AppGetCltGame() );
			float fZ = RandGetFloat( AppGetCltGame() );
			tAttachment.vPositionInObject.fX = pBBox->vMin.fX + ( pBBox->vMax.fX - pBBox->vMin.fX ) * fX;
			tAttachment.vPositionInObject.fY = pBBox->vMin.fY + ( pBBox->vMax.fY - pBBox->vMin.fY ) * fY;
			tAttachment.vPositionInObject.fZ = pBBox->vMin.fZ + ( pBBox->vMax.fZ - pBBox->vMin.fZ ) * fZ;
			tAttachment.dwFlags &= ~ATTACHMENT_FLAGS_NEED_BBOX_UPDATE;
		}
	}
	if ( tAttachment.dwFlags & ATTACHMENT_FLAGS_ABSOLUTE_POSITION )
	{
		MatrixMultiply(	&vPosition, &cgvNone, &mWorld );
		if ( tAttachment.dwFlags & ATTACHMENT_FLAGS_SCALE_OFFSET )
		{
			VECTOR vScale = e_ModelGetScale( nModelId );
			VECTOR vOffset =  tAttachment.vPositionInObject * vScale;
			vPosition += vOffset; 
		} else {
			vPosition += tAttachment.vPositionInObject;
		}
	} else {
		MATRIX mOffset;
		MatrixTransform( &mOffset, &tAttachment.vPositionInObject, 0.0f );
		MatrixMultiply( &mWorld, &mOffset, &mWorld );
		MatrixMultiply(	&vPosition, &cgvNone, &mWorld );
	}

	if ( (tAttachment.dwFlags & ATTACHMENT_FLAGS_POINT_AT_CAMERA_FOCUS) != 0 &&
		! VectorIsZero( sgvCameraFocus ) )
	{
		VECTOR vToFocusNormal = sgvCameraFocus - vPosition;
		VectorNormalize( vToFocusNormal );

		VECTOR vBoneNormal;
		MatrixMultiplyNormal( &vBoneNormal, &tAttachment.vNormalInObject, &mWorld );
		VectorNormalize( vBoneNormal );

		vNormal = VectorLerp( vToFocusNormal, vBoneNormal, 0.7f );
		VectorNormalize( vNormal );
	}
	else if ( tAttachment.dwFlags & ATTACHMENT_FLAGS_ABSOLUTE_NORMAL )
	{
		vNormal = tAttachment.vNormalInObject;
	} else {
		MatrixMultiplyNormal( &vNormal, &tAttachment.vNormalInObject, &mWorld );
		VectorNormalize( vNormal );
	}

	if ( tAttachment.fRotation != 0.0f )
	{
		MATRIX mRotation;
		MatrixRotationAxis( mRotation, vNormal, tAttachment.fRotation );
		MatrixMultiply ( &mWorld, &mRotation, &mWorld );
	}
	else if ( tAttachment.fYaw != 0.0f || tAttachment.fPitch != 0.0f || tAttachment.fRoll != 0.0f)
	{
		MATRIX mTransform;
		// yeah, roll and yaw are reversed for this coordinate system I guess
		MatrixRotationYawPitchRoll( mTransform, tAttachment.fRoll, tAttachment.fPitch, tAttachment.fYaw );
		MatrixMultiply ( &mWorld, &mTransform, &mWorld );
	}

	if ( tAttachment.dwFlags & ATTACHMENT_FLAGS_MIRROR )
	{// HACK to see whether we can mirror weapons
		MATRIX mTransform;
		MatrixIdentity( & mTransform );
		mTransform.SetForward( VECTOR(0,-1,0) );
		MatrixMultiply ( &mWorld, &mTransform, &mWorld );
	}

	switch( tAttachment.eType )
	{
	case ATTACHMENT_PARTICLE:
		c_ParticleSystemMove( tAttachment.nAttachedId, vPosition, &vNormal, idRoom, &mWorld );
		break;

	case ATTACHMENT_PARTICLE_ROPE_END:
		c_ParticleSystemSetRopeEnd( tAttachment.nAttachedId, vPosition, vNormal );
		break;

	case ATTACHMENT_LIGHT:
		e_LightSetPosition( tAttachment.nAttachedId, vPosition, &vNormal );
		break;

	case ATTACHMENT_MODEL:
		c_ModelSetScale( tAttachment.nAttachedId, tAttachment.vScale );				
		c_ModelSetLocation( tAttachment.nAttachedId, &mWorld, vPosition, idRoom, Sort );
		break;

	case ATTACHMENT_SOUND:
		// TODO: get velocity for the sound
		c_SoundSetPosition( tAttachment.nAttachedId, vPosition );
		break;
	}

	if(tAttachment.dwFlags & ATTACHMENT_FLAGS_DONT_MOVE_AFTER_CREATE)
	{
		tAttachment.dwFlags |= ATTACHMENT_FLAGS_DONT_MOVE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentSetVisible( 
	ATTACHMENT & tAttachment, 
	BOOL bSet )
{
	switch ( tAttachment.eType )
	{
	case ATTACHMENT_PARTICLE:
	case ATTACHMENT_PARTICLE_ROPE_END:
		c_ParticleSystemSetVisible( tAttachment.nAttachedId, bSet );
		break;
	case ATTACHMENT_MODEL:
		if ( bSet )
			e_ModelUpdateVisibilityToken( tAttachment.nAttachedId );
		break;
	case ATTACHMENT_LIGHT:
		break;
	case ATTACHMENT_SOUND:
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentSetDraw( 
	ATTACHMENT & tAttachment, 
	BOOL bSet )
{
	switch ( tAttachment.eType )
	{
	case ATTACHMENT_PARTICLE:
	case ATTACHMENT_PARTICLE_ROPE_END:
		c_ParticleSystemSetDraw( tAttachment.nAttachedId, bSet );
		break;
	case ATTACHMENT_MODEL:
		e_ModelSetDrawAndShadow( tAttachment.nAttachedId, bSet );
		break;
	case ATTACHMENT_LIGHT:
		//if ( e_LightGetType( tAttachment.nAttachedId ) == LIGHT_TYPE_SPOT )
		e_LightSetFlags( tAttachment.nAttachedId, LIGHT_FLAG_NODRAW, !bSet );
		break;
	case ATTACHMENT_SOUND:
		{
			if(!(tAttachment.dwFlags & ATTACHMENT_FLAGS_DONT_MUTE_SOUND))
			{
				c_SoundSetMute( tAttachment.nAttachedId, !bSet );
			}
		}
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_ModelSetDrawLayer( 
	int nModelId, 
	DRAW_LAYER eDrawLayer )
{
	V( e_ModelSetDrawLayer( nModelId, eDrawLayer ) );

	ATTACHMENT_HOLDER * pAttachmentHolder = c_AttachmentHolderGet( nModelId );
	if ( ! pAttachmentHolder )
		return;

	for ( int i = 0; i < pAttachmentHolder->nCount; i++ )
	{
		c_AttachmentSetDrawLayer( pAttachmentHolder->pAttachments[ i ], (BYTE)eDrawLayer );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_ModelSetLightPriorityType( 
	int nModelId,
	LIGHT_PRIORITY_TYPE eType )
{
	ATTACHMENT_HOLDER * pAttachmentHolder = c_AttachmentHolderGet( nModelId );
	if ( ! pAttachmentHolder )
		return;

	pAttachmentHolder->eLightPriority = eType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentSetDrawLayer( 
	ATTACHMENT & tAttachment, 
	BYTE bDrawLayer )
{
	switch ( tAttachment.eType )
	{
	case ATTACHMENT_PARTICLE:
	case ATTACHMENT_PARTICLE_ROPE_END:
		c_ParticleSystemSetDrawLayer( tAttachment.nAttachedId, bDrawLayer, FALSE );
		break;
	case ATTACHMENT_MODEL:
		c_ModelSetDrawLayer( tAttachment.nAttachedId, (DRAW_LAYER)bDrawLayer );
		break;
	case ATTACHMENT_LIGHT:
	case ATTACHMENT_SOUND:
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_ModelSetParticleSystemParam( 
	int nModelId, 
	PARTICLE_SYSTEM_PARAM eParam,
	float fValue)
{
	ATTACHMENT_HOLDER * pAttachmentHolder = c_AttachmentHolderGet( nModelId );
	if ( ! pAttachmentHolder )
		return;

	for ( int i = 0; i < pAttachmentHolder->nCount; i++ )
	{
		switch( pAttachmentHolder->pAttachments[ i ].eType )
		{
		case ATTACHMENT_PARTICLE:
		case ATTACHMENT_PARTICLE_ROPE_END:
			c_ParticleSystemSetParam( pAttachmentHolder->pAttachments[ i ].nAttachedId, eParam, fValue );
			break;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_ModelSetRegion( 
	int nModelId, 
	int nRegionID )
{
	e_ModelSetRegionPrivate( nModelId, nRegionID );
	c_AttachmentSetModelRegion( nModelId, nRegionID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentSetRegion( 
	ATTACHMENT & tAttachment, 
	int nRegionID )
{
	switch ( tAttachment.eType )
	{
	case ATTACHMENT_PARTICLE:
	case ATTACHMENT_PARTICLE_ROPE_END:
		c_ParticleSystemSetRegion( tAttachment.nAttachedId, nRegionID );
		break;
	case ATTACHMENT_MODEL:
		c_ModelSetRegion( tAttachment.nAttachedId, nRegionID );
		break;
	case ATTACHMENT_LIGHT:
		break;
	case ATTACHMENT_SOUND:
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentSetModelRegion( 
	int nModelID, 
	int nRegionID )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );
	if (NULL == pHolder)
	{
		return;
	}

	c_AttachmentHolderSetRegion( *pHolder, nRegionID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentSetFirstPerson( 
	ATTACHMENT & tAttachment, 
	BOOL bSet )
{
	switch ( tAttachment.eType )
	{
	case ATTACHMENT_PARTICLE:
	case ATTACHMENT_PARTICLE_ROPE_END:
		c_ParticleSystemSetFirstPerson( tAttachment.nAttachedId, bSet );
		break;
	case ATTACHMENT_MODEL:
		e_ModelSetFlagbit( tAttachment.nAttachedId, MODEL_FLAGBIT_FIRST_PERSON_PROJ, bSet );
		break;
	case ATTACHMENT_LIGHT:
	case ATTACHMENT_SOUND:
		break;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_AttachmentFind( 
	ATTACHMENT_DEFINITION & tDef,
	ATTACHMENT_HOLDER & tHolder,
	int nAttachedId )
{
	if ( tDef.nAttachedDefId == INVALID_ID && nAttachedId == INVALID_ID )
		return INVALID_ID;

	int nModelDef = INVALID_ID;
	if ( tDef.eType == ATTACHMENT_MODEL )
	{
		if ( nAttachedId != INVALID_ID )
		{
			nModelDef = e_ModelGetDefinition( nAttachedId );
		} else {
			nModelDef = c_AppearanceDefinitionGetModelDefinition( tDef.nAttachedDefId );
		}
	}
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		ATTACHMENT & tAttachment = tHolder.pAttachments[ i ];
		if ( tAttachment.eType != tDef.eType )
			continue;
		if ( tAttachment.nBoneId != tDef.nBoneId )
			continue;
		if ( tAttachment.nBoneId == INVALID_ID && tDef.nBoneId == INVALID_ID )
		{
			if (PStrCmp(tAttachment.pszBoneName, tDef.pszBone))
			{
				continue;
			}
		}
		if ( tDef.dwFlags & ATTACHMENT_FLAGS_FIND_CHECK_NORMAL )
		{
			if ( tAttachment.vNormalInObject != tDef.vNormal )
				continue;
		}
		BOOL bFound = FALSE;
		if(tDef.dwFlags & ATTACHMENT_FLAGS_IS_FOOTSTEP)
		{
			if(tAttachment.dwFlags & ATTACHMENT_FLAGS_IS_FOOTSTEP)
			{
				bFound = TRUE;
			}
		}
		else
		{
			switch ( tDef.eType )
			{
			case ATTACHMENT_PARTICLE:
			case ATTACHMENT_PARTICLE_ROPE_END:
				if ( c_ParticleSystemGetDefinition( tAttachment.nAttachedId ) == tDef.nAttachedDefId )
					bFound = TRUE;
				break;
			case ATTACHMENT_MODEL:
				if ( e_ModelGetDefinition( tAttachment.nAttachedId ) == nModelDef )
					bFound = TRUE;
				break;
			case ATTACHMENT_LIGHT:
				if ( e_LightGetDefinition( tAttachment.nAttachedId ) == tDef.nAttachedDefId )
					bFound = TRUE;
				break;
			case ATTACHMENT_SOUND:
				if ( c_SoundGetGroup( tAttachment.nAttachedId ) == tDef.nAttachedDefId )
					bFound = TRUE;
				break;
			}
		}
		if ( bFound )
			return tAttachment.nId;
	}

	if ( tDef.pszBone[ 0 ] != 0 && tDef.nBoneId == INVALID_ID )
	{
		int nBoneId = INVALID_ID;
		int nAttachedModelId = c_AttachmentFindModelWithBone( tHolder, tDef, &nBoneId );
		if ( nAttachedModelId != INVALID_ID )
		{
			tDef.nBoneId = nBoneId;
			int nAttachmentId = c_ModelAttachmentFind( nAttachedModelId, tDef, nAttachedId );
			tDef.nBoneId = INVALID_ID;
			return nAttachmentId;
		}
	}

	// if we can't find it on this list of attachments, then check the models that are attached to it
	for ( int i = 0; i < tHolder.nCount; i++ )
	{

		if ( tHolder.pAttachments[ i ].eType == ATTACHMENT_MODEL )
		{
			int nId = c_ModelAttachmentFind( tHolder.pAttachments[ i ].nAttachedId, tDef, nAttachedId );
			if ( nId != INVALID_ID )
				return nId;
		}
	}
	return INVALID_ID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_AttachmentFind( 
	ATTACHMENT_HOLDER & tHolder,
	int nOwnerType,
	int nOwnerId )
{
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		if ( tHolder.pAttachments[ i ].tOwner.nType != nOwnerType ||
			 tHolder.pAttachments[ i ].tOwner.nId   != nOwnerId )
			continue;
		return tHolder.pAttachments[ i ].nId;
	}

	// if we can't find it on this list of attachments, then check the models that are attached to it
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		if ( tHolder.pAttachments[ i ].eType == ATTACHMENT_MODEL )
		{
			int nId = c_ModelAttachmentFind( tHolder.pAttachments[ i ].nAttachedId, nOwnerType, nOwnerId );
			if ( nId != INVALID_ID )
				return nId;
		}
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

ATTACHMENT * c_AttachmentGet( 
	int nAttachmentId,
	ATTACHMENT_HOLDER & tHolder )
{
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		if ( tHolder.pAttachments[ i ].nId == nAttachmentId )
		{
			return &tHolder.pAttachments[ i ];
		}
	}

	// if we can't find it on this list of attachments, then check the models that are attached to it
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		if ( tHolder.pAttachments[ i ].eType == ATTACHMENT_MODEL )
		{
			ATTACHMENT * pAttachment = c_ModelAttachmentGet( tHolder.pAttachments[ i ].nAttachedId, nAttachmentId );
			if ( pAttachment )
				return pAttachment;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentsRemoveNonExisting( 
	int nModelId )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );
	if ( ! pHolder )
		return;

	for ( int i = 0; i < pHolder->nCount; i++ )
	{
		ATTACHMENT * pAttachment = &pHolder->pAttachments[ i ];
		BOOL bExists = TRUE;
		switch ( pAttachment->eType )
		{
		case ATTACHMENT_PARTICLE:
		case ATTACHMENT_PARTICLE_ROPE_END:
			bExists = c_ParticleSystemExists( pAttachment->nAttachedId );
			break;
		case ATTACHMENT_LIGHT:
			bExists = e_LightExists( pAttachment->nAttachedId );
			break;
		case ATTACHMENT_MODEL:
			bExists = e_ModelExists( pAttachment->nAttachedId );
			break;
		case ATTACHMENT_SOUND:
			bExists = c_SoundExists( pAttachment->nAttachedId );
			break;
		}
		if ( ! bExists )
		{
			if ( i < pHolder->nCount - 1)
				MemoryMove( pHolder->pAttachments + i, (pHolder->nCount - i) * sizeof(ATTACHMENT), pHolder->pAttachments + i + 1, (pHolder->nCount - i - 1) * sizeof(ATTACHMENT));
			pHolder->nCount--;
			i--;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_AttachmentHolderDestroy( 
   ATTACHMENT_HOLDER & tHolder )
{
	if ( tHolder.pAttachments )
		FREE( NULL, tHolder.pAttachments );
	tHolder.nCount = 0;
	tHolder.nAllocated = 0;
}

//----------------------------------------------------------------------------
// we are only checking one level deep right now.  Maybe we should recurse further....
//----------------------------------------------------------------------------
int c_AttachmentFindModelWithBone( 
	ATTACHMENT_HOLDER & tHolder, 
	ATTACHMENT_DEFINITION & tAttachmentDef,
	int * pnBoneId )
{
	for ( int i = 0; i < tHolder.nCount; i++ )
	{
		ATTACHMENT * pAttachment = & tHolder.pAttachments[ i ];
		if ( pAttachment->eType != ATTACHMENT_MODEL )
			continue;
		if ( tAttachmentDef.dwFlags & ATTACHMENT_MASK_WEAPON_INDEX )
		{
			DWORD dwInstanceFlags   = ( pAttachment->dwFlags & ATTACHMENT_MASK_WEAPON_INDEX );
			DWORD dwDefinitionFlags = ( tAttachmentDef.dwFlags & ATTACHMENT_MASK_WEAPON_INDEX );
			if ( ( dwInstanceFlags & dwDefinitionFlags ) == 0 )
				continue;
		}
		int nAppearanceDef = e_ModelGetAppearanceDefinition( pAttachment->nAttachedId );
		if ( nAppearanceDef == INVALID_ID )
			continue;
		int nBoneId = c_AppearanceDefinitionGetBoneNumber( nAppearanceDef, tAttachmentDef.pszBone );
		if ( nBoneId != INVALID_ID )
		{
			if ( pnBoneId )
				*pnBoneId = nBoneId;
			return pAttachment->nAttachedId;
		}
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_AttachmentSetWeaponIndex ( 
	GAME * pGame,
	ATTACHMENT_DEFINITION & tAttachmentDef,
	UNIT * pContainer,
	UNITID idWeapon )
{
	int nWeaponIndex = WardrobeGetWeaponIndex( pContainer, idWeapon );
	if ( nWeaponIndex != INVALID_ID )
	{
		tAttachmentDef.dwFlags |= 1 << ( nWeaponIndex + ATTACHMENT_SHIFT_WEAPON_INDEX );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_AttachmentHolderSetRegion ( 
	ATTACHMENT_HOLDER & tAttachmentHolder,
	int nRegionID )
{
	tAttachmentHolder.nRegionID = nRegionID;

	for ( int i = 0; i < tAttachmentHolder.nCount; i++ )
	{
		ATTACHMENT * pAttachment = &tAttachmentHolder.pAttachments[ i ];

		c_sApplyHolderRegion( pAttachment, nRegionID );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_AttachmentHolderSetFlag ( 
	ATTACHMENT_HOLDER & tAttachmentHolder,
	DWORD dwFlags,
	BOOL bValue )
{
	if ( bValue )
	{
		tAttachmentHolder.dwFlags |= dwFlags;
	} else {
		tAttachmentHolder.dwFlags &= ~dwFlags;
	}

	for ( int i = 0; i < tAttachmentHolder.nCount; i++ )
	{
		ATTACHMENT * pAttachment = &tAttachmentHolder.pAttachments[ i ];

		c_sApplyHolderFlags( pAttachment, dwFlags, bValue );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int c_ModelAttachmentAdd( 
	int nModelId, 
	ATTACHMENT_DEFINITION & tAttachmentDef, 
	const char * pszFolder,
	BOOL bForceNew,
	int nOwnerType,
	int nOwnerId,
	int nAttachedId )
{
	if ( tAttachmentDef.dwFlags & ATTACHMENT_FLAGS_FORCE_NEW )
		bForceNew = TRUE;

	int nAppearanceDef = e_ModelGetAppearanceDefinition( nModelId );

	c_AttachmentDefinitionLoad( AppGetCltGame(), tAttachmentDef, nAppearanceDef, pszFolder );

	int nAttachmentId = c_ModelAttachmentFind( nModelId, tAttachmentDef, nAttachedId );
	if ( nAttachmentId != INVALID_ID && ! bForceNew )
		return nAttachmentId;

	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );

	if ( ! pHolder )
		return INVALID_ID;

	// see if we need to add this to an attachment instead of to this model
	if ( tAttachmentDef.pszBone[ 0 ] != 0 && tAttachmentDef.nBoneId == INVALID_ID )
	{
		int nBoneId = INVALID_ID;
		int nAttachedModelId = c_AttachmentFindModelWithBone( *pHolder, tAttachmentDef, &nBoneId );
		if ( nAttachedModelId != INVALID_ID )
		{
			tAttachmentDef.nBoneId = nBoneId;
			nAttachmentId = c_ModelAttachmentAdd( nAttachedModelId, tAttachmentDef, pszFolder, bForceNew,
												  nOwnerType, nOwnerId, nAttachedId );
			tAttachmentDef.nBoneId = INVALID_ID;
			return nAttachmentId;
		}
	}

	ATTACHMENT::OWNER tOwner( nOwnerType, nOwnerId );

	MATRIX tWorldScaled = *e_ModelGetWorldScaled( nModelId );
	int nModelRoomID = c_ModelGetRoomID( nModelId );

	nAttachmentId = c_AttachmentNew( tAttachmentDef, *pHolder, nModelRoomID, tWorldScaled, tOwner, nAttachedId );
	if ( nAttachmentId != INVALID_ID )
	{
		MODEL_SORT_TYPE Sort = e_ModelGetSortType( nModelId );
		c_AttachmentMove( pHolder->pAttachments[ pHolder->nCount - 1 ], nModelId, tWorldScaled, nModelRoomID, Sort );

		ATTACHMENT * pAttachment = c_AttachmentGet( nAttachmentId, *pHolder );
		if ( pAttachment )
		{
			// set the flags on this attachment
			c_sApplyHolderFlags( pAttachment, ATTACHMENT_HOLDER_FLAGS_NODRAW, e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW ) || e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW_TEMPORARY ) );
			if ( AppIsHellgate() )
			{
				DRAW_LAYER eDrawLayer = e_ModelGetDrawLayer( nModelId );
				c_AttachmentSetDrawLayer( *pAttachment, (BYTE)eDrawLayer );
			}
		}
	}


	return nAttachmentId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ModelAttachmentAddBBox( 
	int nModelId, 
	int nCount,
	ATTACHMENT_DEFINITION & tAttachmentDef, 
	const char * pszFolder,
	BOOL bForceNew,
	int nOwnerType,
	int nOwnerId,
	int nAttachedId,
	BOOL bFloatAttachments )
{
	if ( tAttachmentDef.dwFlags & ATTACHMENT_FLAGS_FLOAT )
		bFloatAttachments = TRUE;

	if ( tAttachmentDef.dwFlags & ATTACHMENT_FLAGS_FORCE_NEW )
		bForceNew = TRUE;

	int nAppearanceDef = e_ModelGetAppearanceDefinition( nModelId );

	c_AttachmentDefinitionLoad( AppGetCltGame(), tAttachmentDef, nAppearanceDef, pszFolder );

	int nAttachmentId = c_ModelAttachmentFind( nModelId, tAttachmentDef, nAttachedId );
	if ( nAttachmentId != INVALID_ID && ! bForceNew )
		return;

	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );

	if ( ! pHolder )
		return;
	// see if we need to add this to an attachment instead of to this model
	if ( tAttachmentDef.pszBone[ 0 ] != 0 && tAttachmentDef.nBoneId == INVALID_ID )
	{
		int nBoneId = INVALID_ID;
		int nAttachedModelId = c_AttachmentFindModelWithBone( *pHolder, tAttachmentDef, &nBoneId );
		if ( nAttachedModelId != INVALID_ID )
		{
			tAttachmentDef.nBoneId = nBoneId;
			nAttachmentId = c_ModelAttachmentAdd( nAttachedModelId, tAttachmentDef, pszFolder, bForceNew,
												  nOwnerType, nOwnerId, nAttachedId );
			tAttachmentDef.nBoneId = INVALID_ID;
			return;
		}
	}
	const BOUNDING_BOX* pBBox = e_ModelDefinitionGetBoundingBox( e_ModelGetDefinition( nModelId ), LOD_ANY );
	MATRIX tWorldScaled = *e_ModelGetWorldScaled( nModelId );
	int nModelRoomID = c_ModelGetRoomID( nModelId );
	for( int i = 0; i < nCount; i++ )
	{
		float fX = RandGetFloat( AppGetCltGame() );
		float fY = RandGetFloat( AppGetCltGame() );
		float fZ = RandGetFloat( AppGetCltGame() );
		if( pBBox )
		{
			tAttachmentDef.vPosition.fX = pBBox->vMin.fX + ( pBBox->vMax.fX - pBBox->vMin.fX ) * fX;
			tAttachmentDef.vPosition.fY = pBBox->vMin.fY + ( pBBox->vMax.fY - pBBox->vMin.fY ) * fY;
			tAttachmentDef.vPosition.fZ = pBBox->vMin.fZ + ( pBBox->vMax.fZ - pBBox->vMin.fZ ) * fZ;
		}
		else
		{
			tAttachmentDef.vPosition.fX = 0;
			tAttachmentDef.vPosition.fY = 0;
			tAttachmentDef.vPosition.fZ = 0;
			tAttachmentDef.dwFlags |= ATTACHMENT_FLAGS_NEED_BBOX_UPDATE;
		}		
		ATTACHMENT::OWNER tOwner( nOwnerType, nOwnerId );


		nAttachmentId = c_AttachmentNew( tAttachmentDef, *pHolder, nModelRoomID, tWorldScaled, tOwner, nAttachedId );
		if ( nAttachmentId != INVALID_ID )
		{
			MODEL_SORT_TYPE Sort = e_ModelGetSortType( nModelId );
			c_AttachmentMove( pHolder->pAttachments[ pHolder->nCount - 1 ], nModelId, tWorldScaled, nModelRoomID, Sort );
			if ( bFloatAttachments )
				c_ModelAttachmentFloat( nModelId, nAttachmentId );
		}
	}


	// reset flags so that the attachments get the flags
	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW,           e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW ) );
	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW_TEMPORARY, e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW_TEMPORARY ) );
	//BYTE bDrawLayer = e_ModelGetDrawLayer( nModelId );
	//e_ModelSetDrawLayer( nModelId, bDrawLayer );

	return;
}
//-------------------------------------------------------------------------------------------------
// this should be cleaned up so that it uses the Picker
//-------------------------------------------------------------------------------------------------

void c_ModelAttachmentAddToRandomBones( 
	int nModelId, 
	int nCount,
	ATTACHMENT_DEFINITION & tAttachmentDef, 
	const char * pszFolder,
	BOOL bForceNew,
	int nOwnerType,
	int nOwnerId,
	int nAttachedId,
	BOOL bFloatAttachments )
{
	if ( tAttachmentDef.dwFlags & ATTACHMENT_FLAGS_FLOAT )
		bFloatAttachments = TRUE;

	int nAppearanceDef = e_ModelGetAppearanceDefinition( nModelId );
	int nBoneCount = c_AppearanceDefinitionGetBoneCount( nAppearanceDef );
	if ( nBoneCount <= 0 )
		return;
	ASSERT_RETURN( nCount > 0 );
	ASSERT_RETURN( nCount < MAX_BONES_PER_MODEL );

	c_AttachmentDefinitionLoad( AppGetCltGame(), tAttachmentDef, nAppearanceDef, pszFolder );

	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );

	// select the bones
	int pnBoneIds[ MAX_BONES_PER_MODEL ];
	if ( nCount >= nBoneCount )
	{
		nCount = nBoneCount;
		for ( int i = 0; i < nCount; i++ )
			pnBoneIds[ i ] = i;
	} else {
		float * pfWeights = c_AppearanceDefinitionGetBoneWeights( nAppearanceDef, WEIGHT_GROUP_ADD_RANDOM_ATTACHMENT );
		if ( pfWeights )
		{
			float fWeightTotal = 0.0f;
			float * pfWeightCurr = pfWeights;
			int nWeightedBones = 0;
			DWORD pdwBoneHasWeight[ MAX_BONES_PER_MODEL / 32 + 1 ];
			ZeroMemory( pdwBoneHasWeight, sizeof( pdwBoneHasWeight ) );
			for ( int i = 0; i < nBoneCount; i++ )
			{
				fWeightTotal += *pfWeightCurr;
				if ( *pfWeightCurr != 0.0f )
				{
					nWeightedBones++;
					SETBIT( pdwBoneHasWeight, i, TRUE );
				}
				pfWeightCurr++;
			}
			ASSERTX_RETURN( nWeightedBones, "Appearance missing random bone weights" );
			if ( nWeightedBones <= nCount )
			{// maybe we don't have enough bones to choose from
				pfWeightCurr = pfWeights;
				int nBoneCurr = 0;
				for ( int i = 0; i < nBoneCount; i++ )
				{
					if ( * pfWeightCurr != 0.0f )
					{
						pnBoneIds[ nBoneCurr ] = i;
						nBoneCurr++;
					}
					pfWeightCurr++;
				}
				nCount = nBoneCurr;
			} else {
				for ( int i = 0; i < nCount; i++ )
				{
					float fRoll = RandGetFloat( AppGetCltGame() );
					fRoll *= fWeightTotal;
					pfWeightCurr = pfWeights;
					pnBoneIds[ i ] = INVALID_ID;
					for ( int j = 0; j < nBoneCount; j++, pfWeightCurr++ )
					{
						if ( ! TESTBIT( pdwBoneHasWeight, j ) )
							continue;
						fRoll -= *pfWeightCurr;
						if ( fRoll < 0.0f )
						{
							pnBoneIds[ i ] = j;
							SETBIT( pdwBoneHasWeight, j, FALSE );// turn off this bone since it is now used
							fWeightTotal -= *pfWeightCurr;
							break;
						}
					}
					if ( pnBoneIds[ i ] == INVALID_ID )
					{// in case some round-off error makes us just miss the last one - do i again
						i--;
					}
				}
			}
		} else {
			// just randomly pick a bone with each bone having an equal chance
			for ( int i = 0; i < nCount; i++ )
			{
				pnBoneIds[ i ] = RandGetNum( AppGetCltGame(), nBoneCount );
				BOOL bDuplicate = FALSE;
				do {
					bDuplicate = FALSE;
					for ( int j = 0; j < i; j++ )
					{
						if ( pnBoneIds[ j ] == pnBoneIds[ i ] )
							bDuplicate = TRUE;
					}
					if ( bDuplicate )
					{
						pnBoneIds[ i ]++; // yes, this will tend to cluster some of the bones together - it looks better, and it makes for a faster algorithm
						pnBoneIds[ i ] %= nBoneCount;
					}
				} while ( bDuplicate );
			}
		}
	}

	int nModelRoomID = c_ModelGetRoomID( nModelId );
	MATRIX mScaledWorld = * e_ModelGetWorldScaled( nModelId );

	for ( int i = 0; i < nCount; i++ )
	{
		tAttachmentDef.nBoneId = pnBoneIds[ i ];

		ATTACHMENT::OWNER tOwner( nOwnerType, nOwnerId );

		int nAttachmentId = c_AttachmentNew( tAttachmentDef, *pHolder, 
			nModelRoomID, mScaledWorld, tOwner, nAttachedId );

		if ( nAttachmentId != INVALID_ID )
		{
			ATTACHMENT * pAttachment = c_AttachmentGet( nAttachmentId, *pHolder ); 
			ASSERT_RETURN( pAttachment );
			c_AttachmentMove( *pAttachment, nModelId, mScaledWorld, nModelRoomID );
			if ( bFloatAttachments )
				c_ModelAttachmentFloat( nModelId, nAttachmentId );
		}
	}

	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW,           e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW ) );
	e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW_TEMPORARY, e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW_TEMPORARY ) );

	if ( AppIsHellgate() ) 
	{
		DRAW_LAYER eDrawLayer = e_ModelGetDrawLayer( nModelId );
		c_ModelSetDrawLayer( nModelId, eDrawLayer );
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ModelAttachmentAddAtBoneWeight( 
	int nModelId, 
	const ATTACHMENT_DEFINITION & tAttachmentDef, 
	BOOL bForceNew,
	int nOwnerType,
	int nOwnerId,
	BOOL bFloat)
{
	ATTACHMENT_DEFINITION tDef;
	memcpy( &tDef, &tAttachmentDef, sizeof(ATTACHMENT_DEFINITION) );
	memclear(tDef.pszBone, MAX_XML_STRING_LENGTH);

	int nAppearanceDefinitionId = c_AppearanceGetDefinition( nModelId );
	int nBoneCount = c_AppearanceDefinitionGetBoneCount(nAppearanceDefinitionId);
	float * fBoneWeights = c_AppearanceDefinitionGetBoneWeightsByName(nAppearanceDefinitionId, tAttachmentDef.pszBone);
	if(fBoneWeights)
	{
		for(int i=0; i<nBoneCount; i++)
		{
			if(fBoneWeights[i] > 0.0f)
			{
				float fRand = RandGetFloat(AppGetCltGame(), 0.0f, 1.0f);
				if(fRand <= fBoneWeights[i])
				{
					tDef.nBoneId = i;
					int nAttachment = c_ModelAttachmentAdd( nModelId, tDef, FILE_PATH_DATA, bForceNew, nOwnerType, nOwnerId, INVALID_ID );
					if(bFloat)
					{
						c_ModelAttachmentFloat( nModelId, nAttachment );
					}
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int c_ModelAttachmentFind( 
	int nModelId,
	ATTACHMENT_DEFINITION & tDef,
	int nAttachedId )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );
	if ( ! pHolder )
		return INVALID_ID;
	return c_AttachmentFind( tDef, *pHolder, nAttachedId );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_ModelAttachmentFind( 
	int nModelId, 
	int nOwnerType, 
	int nOwnerId )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );
	return pHolder ? c_AttachmentFind( *pHolder, nOwnerType, nOwnerId ) : INVALID_ID;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ModelAttachmentFloat(
	int nModelId,
	int nAttachmentId )
{
	ATTACHMENT * pAttachment = c_ModelAttachmentGet( nModelId, nAttachmentId );
	if ( ! pAttachment )
		return;
	c_AttachmentSetVisible( *pAttachment, TRUE );
	c_AttachmentSetDraw( *pAttachment, TRUE );
	if ( pAttachment->eType == ATTACHMENT_MODEL && (pAttachment->dwFlags & ATTACHMENT_FLAGS_MIRROR) )
	{
		e_ModelSetFlagbit( pAttachment->nAttachedId, MODEL_FLAGBIT_MIRRORED, FALSE );
	}
	c_ModelAttachmentRemove( nModelId, nAttachmentId, ATTACHMENT_FUNC_FLAG_FLOAT );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ModelAttachmentRemove( 
	int nModelId, 
	ATTACHMENT_DEFINITION & tDef, 
	const char * pszFolder, 
	DWORD dwFlags )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );

	int nAppearanceDefID = e_ModelGetAppearanceDefinition( nModelId );
	c_AttachmentDefinitionLoad( AppGetCltGame(), tDef, nAppearanceDefID, pszFolder );

	int nAttachmentId = c_ModelAttachmentFind( nModelId, tDef );
	if ( nAttachmentId != INVALID_ID )
	{
		c_AttachmentDestroy( nAttachmentId, *pHolder, dwFlags );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ModelAttachmentRemove( 
	 int nModelId, 
	 int nId, 
	 DWORD dwFlags )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );
	if ( pHolder )
		c_AttachmentDestroy( nId, *pHolder, dwFlags );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ModelAttachmentRemoveAll(
	int nModelId,
	DWORD dwFlags )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );

	if ( pHolder )
		c_AttachmentDestroyAll( *pHolder, dwFlags );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ModelAttachmentRemoveAllByOwner( 
	int nModelId, 
	int nOwnerType, 
	int nOwnerId,
	DWORD dwFlags )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );
	if ( pHolder )
	{
		ATTACHMENT::OWNER tOwner( nOwnerType, nOwnerId );
		c_AttachmentDestroyAllByOwner( *pHolder, tOwner, dwFlags );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

ATTACHMENT * c_ModelAttachmentGet( 
	int nModelId, 
	int nAttachmentId )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelId );
	return pHolder ? c_AttachmentGet( nAttachmentId, *pHolder ) : NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ModelAttachmentsUpdate( 
	int nModelID,							   
	MODEL_SORT_TYPE SortType ) 
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );

	if ( pHolder )
	{
		MATRIX mScaledWorld = * e_ModelGetWorldScaled( nModelID );
		int nModelRoomID = c_ModelGetRoomID( nModelID );

		for ( int i = 0; i < pHolder->nCount; i++ )
		{
			ATTACHMENT & pAttachment = pHolder->pAttachments[ i ];
			if (pAttachment.nBoneId == INVALID_ID && pAttachment.pszBoneName[0] != 0)
			{
				int nAppearanceDefId = e_ModelGetAppearanceDefinition(nModelID);
				if (nAppearanceDefId != INVALID_ID)
				{
					pAttachment.nBoneId = c_AppearanceDefinitionGetBoneNumber( nAppearanceDefId, pAttachment.pszBoneName );
				}
			}
			c_AttachmentMove( pAttachment, nModelID, mScaledWorld, nModelRoomID, SortType );
		}
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ModelSetAttachmentNormals( 
	int nModelID,
	const VECTOR & vNormal ) 
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );

	if ( pHolder )
	{
		for ( int i = 0; i < pHolder->nCount; i++ )
		{
			ATTACHMENT & tAttachment = pHolder->pAttachments[ i ];
			tAttachment.vNormalInObject = vNormal;
		}
		c_ModelAttachmentsUpdate( nModelID );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_ModelAttachmentWarpToCurrentPosition( 
	int nModelID ) 
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );

	if ( pHolder )
	{
		for ( int i = 0; i < pHolder->nCount; i++ )
		{
			ATTACHMENT & tAttachment = pHolder->pAttachments[ i ];
			switch( tAttachment.eType )
			{
			case ATTACHMENT_PARTICLE:
				c_ParticleSystemWarpToCurrentPosition( tAttachment.nAttachedId );
				break;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_AttachmentModelNew( int nModelID )
{
	HashAdd( gtAttachmentHolders, nModelID );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_AttachmentModelRemove( int nModelID )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );
	ASSERT_RETURN( pHolder );
	c_AttachmentDestroyAll( * pHolder, ATTACHMENT_FUNC_FLAG_FORCE_DESTROY );
	c_AttachmentHolderDestroy( * pHolder );
	HashRemove( gtAttachmentHolders, nModelID );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_AttachmentSetCameraFocus( 
	const VECTOR & vPosition )
{
	sgvCameraFocus = vPosition;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_AttachmentSetModelFlagbit( int nModelID, int nFlagbit, BOOL bSet )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );
	ASSERT_RETURN( pHolder );

	DWORD dwHolderFlags = 0;

	// CML 2007.05.19 : MODEL_FLAG_VISIBLE is deprecated.  Use c_AttachmentSetModelVisible instead.

	switch (nFlagbit)
	{
	case MODEL_FLAGBIT_NODRAW:				dwHolderFlags |= ATTACHMENT_HOLDER_FLAGS_NODRAW;				break;
	case MODEL_FLAGBIT_NODRAW_TEMPORARY:	dwHolderFlags |= ATTACHMENT_HOLDER_FLAGS_NODRAW;				break;
	case MODEL_FLAGBIT_FIRST_PERSON_PROJ:	dwHolderFlags |= ATTACHMENT_HOLDER_FLAGS_FIRST_PERSON_PROJ;		break;
	case MODEL_FLAGBIT_NOLIGHTS:			dwHolderFlags |= ATTACHMENT_HOLDER_FLAGS_NOLIGHTS;				break;
	case MODEL_FLAGBIT_MUTESOUND:			dwHolderFlags |= ATTACHMENT_HOLDER_FLAGS_NOSOUND;				break;
	case MODEL_FLAGBIT_NOSOUND:				dwHolderFlags |= ATTACHMENT_HOLDER_FLAGS_DONTPLAYSOUND;			break;
	}

	if ( dwHolderFlags )
		c_AttachmentHolderSetFlag( *pHolder, dwHolderFlags, bSet );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_AttachmentSetModelVisible( int nModelID, BOOL bSet )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nModelID );
	ASSERT_RETURN( pHolder );

	for ( int i = 0; i < pHolder->nCount; i++ )
	{
		c_AttachmentSetVisible( pHolder->pAttachments[ i ], bSet );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_AttachmentInit()
{
	V( e_AttachmentInitCallbacks(
		c_AttachmentNewStaticLightCallback,
		c_AttachmentDefinitionLoadCallback,
		c_AttachmentNewCallback,
		c_ModelAttachmentRemove,
		c_AttachmentModelNew,
		c_AttachmentModelRemove,
		c_AttachmentGetAttachedOfType,
		c_AttachmentGetAttachedById,
		c_AttachmentSetModelFlagbit,
		c_AttachmentSetModelVisible,
		c_ModelAttachmentsUpdate ) );

	HashInit( gtAttachmentHolders, NULL, 256 );
}

void c_AttachmentDestroy()
{
	for ( ATTACHMENT_HOLDER * pHolder = HashGetFirst( gtAttachmentHolders );
		pHolder;
		pHolder = HashGetNext( gtAttachmentHolders, pHolder ) )
		c_AttachmentDestroyAll( * pHolder, 0 );

	HashFree( gtAttachmentHolders );
}
