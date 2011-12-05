/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	File: c_animation.cpp
//
//	Author : Flagship Studios
//			
//	(C)Copyright 2003, Flagship Studios. All rights reserved.
//
//
//	CHANGELOG :
//
//		10-9-06 : Travis - removed unitspeed functions - unitspeed is now retrieved in appearance when the animation is played
//
//		9-29-06 : Travis - several animation play methods were missing unitspeed, which is used for Tug.
//							added them back in, but feels messy, and needs a rework. For now though, it should
//							work. readdress when I get back.
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "appcommon.h"
#include "prime.h"
#include "c_animation.h"
#include "e_texture.h"
#include "c_appearance_priv.h"
#include "e_granny_rigid.h"
#include "unitmodes.h"
#ifdef HAVOK_ENABLED
	#include "c_ragdoll.h"
#endif
#include "uix_scheduler.h"
#include "console.h"
#include "skills.h"
#include "performance.h"
#include "filepaths.h"
#include "pakfile.h"
#include "perfhier.h"
#include "config.h"
#include "e_model.h"
#include "excel_private.h"
#include "states.h"
#include "c_camera.h"
#include "player.h"


#define ANIMATION_FILE_MAGIC_NUMBER			0x12abce21
#define ANIMATION_FILE_VERSION				(1 + GLOBAL_FILE_VERSION)
#define MAX_ANIMATION_FILES_PER_APPEARANCE	1000
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
struct ANIMATION_CONDITION_PARAMS
{
	GAME * pGame;
	UNIT * pUnit;
	int nModelId;
	const ANIMATION_DEFINITION * pAnimDef;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
struct ANIMATION_FILE_CALLBACKDATA
{
	int nAppearanceDefId;
	int nAnimationFileIndex;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static PRESULT sAnimationFileLoadedCallback( 
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	if (data.m_IOSize == 0) 
	{
		return S_FALSE;
	}

	void *pData = spec->buffer;
	ANIMATION_FILE_CALLBACKDATA * pCallbackData = (ANIMATION_FILE_CALLBACKDATA *)spec->callbackData;
	ASSERT_RETFAIL( pCallbackData );

	int nAnimationFileIndex = pCallbackData->nAnimationFileIndex;
	int nAppearanceDefId = pCallbackData->nAppearanceDefId;
			
	ASSERTV_RETVAL( pData, S_FALSE, "File does not exist:\n%s", spec->fixedfilename );

	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDefId );
	ASSERT_RETFAIL( pAppearanceDef );

#ifdef HAVOK_ENABLED
	hkRootLevelContainer * pContainer = NULL;
	if ( AppIsHellgate() )
	{
		BYTE * pFileData  = (BYTE *)pData;

		ASSERT_RETFAIL( pFileData );

		hkMemoryStreamReader tStreamReader( pFileData, data.m_IOSize, hkMemoryStreamReader::MEMORY_INPLACE );

		pContainer = pAppearanceDef->pLoader->load( &tStreamReader );

		ASSERT_RETFAIL( pContainer );

		hkAnimationContainer* pAnimationContainer = reinterpret_cast<hkAnimationContainer*>( pContainer->findObjectByType( hkAnimationContainerClass.getName() ));

		ASSERT_RETFAIL( pAnimationContainer );
		WARNX( pAnimationContainer->m_numAnimations > 0, spec->fixedfilename );
		ASSERT_RETFAIL( pAnimationContainer->m_numAnimations > 0 );
		ASSERT_RETFAIL( pAnimationContainer->m_numBindings   > 0 );

		hkAnimationBinding * pBinding = pAnimationContainer->m_bindings[ 0 ];
		ASSERT_RETFAIL( pBinding				!= NULL );
		ASSERT_RETFAIL( pBinding->m_animation!= NULL );

		ASSERT_RETFAIL( nAnimationFileIndex != INVALID_ID );

		// find all of the files that use this animation and hook up their binding
		for ( int i = 0; i < pAppearanceDef->nAnimationCount; i++ )
		{
			ANIMATION_DEFINITION * pAnimDef = &pAppearanceDef->pAnimations[ i ];
			if ( pAnimDef->nFileIndex != nAnimationFileIndex )
			{
				continue;
			}
			pAnimDef->pBinding = pBinding;
			c_AnimationComputeVelocity( *pAnimDef );
		}

		if ( AppIsHammer() )
		{
			hkSkeleton * pSkeleton = c_AppearanceDefinitionGetSkeletonHavok( pAppearanceDef->tHeader.nId );
			const GLOBAL_DEFINITION* pGlobal = DefinitionGetGlobal();
			if ( pSkeleton && (pGlobal->dwFlags & GLOBAL_FLAG_DATA_WARNINGS) != 0 )
			{
				WARNX( pBinding->m_animation->m_numberOfTracks <= pSkeleton->m_numBones, spec->fixedfilename );


			}
			if ( (pGlobal->dwFlags & GLOBAL_FLAG_DATA_WARNINGS) != 0 )
			{
				hkxScene* pScene = reinterpret_cast<hkxScene*>( pContainer->findObjectByType( hkxSceneClass.getName() ));
				WARNX( !pScene || !pScene->m_meshes, spec->fixedfilename );
			}
		}
	}
#endif

	if ( AppIsTugboat() )
	{
		BYTE * pFileData  = (BYTE *)pData;

		ASSERT_RETFAIL( pFileData );

		granny_file * pLoadedFile = GrannyReadEntireFileFromMemory( data.m_IOSize, pFileData  );

		ASSERT_RETFAIL( pLoadedFile );

		//FREE( NULL, pFileData );

		granny_file_info * pFileInfo =	GrannyGetFileInfo( pLoadedFile );
		ASSERT( pFileInfo );

		// -- Coordinate System --
		// Tell Granny to transform the file into our coordinate system
		e_GrannyConvertCoordinateSystem ( pFileInfo );

		// find all of the files that use this animation and hook up their binding
		for ( int i = 0; i < pAppearanceDef->nAnimationCount; i++ )
		{
			ANIMATION_DEFINITION * pAnimDef = &pAppearanceDef->pAnimations[ i ];
			if ( pAnimDef->nFileIndex != nAnimationFileIndex )
			{
				continue;
			}
			if( pFileInfo->AnimationCount > 0 )
			{
				pAnimDef->pGrannyAnimation = pFileInfo->Animations[ 0 ];
			}
			pAnimDef->pGrannyFile	  = pLoadedFile;
			c_AnimationComputeVelocity( *pAnimDef );
		} 
	}

	return S_OK;
} // sAnimationFileLoadedCallback()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sAnimationLoad( 
	APPEARANCE_DEFINITION & tAppearanceDef,
	int nAnimationIndex,
	const char * pszFolder,
	BOOL bForceSync,
	BOOL bFileLoaded )
{
	ANIMATION_DEFINITION & tAnimDef = (nAnimationIndex == -1 ? tAppearanceDef.tInitAnimation : tAppearanceDef.pAnimations[ nAnimationIndex ]);
	if ( (AppIsTugboat() && tAnimDef.pGrannyAnimation) || 
		 (AppIsHellgate() && tAnimDef.pBinding ) )
	{
		return;
	}

	GAME * pGame = AppGetCltGame();
	TIMER_START( tAnimDef.pszFile );

	for ( int i = 0; i < tAnimDef.nEventCount; i++ )
	{
		ANIM_EVENT * pEvent = &tAnimDef.pEvents[ i ];
		switch( pEvent->eType )
		{
		case ANIM_EVENT_FLOAT_ATTACHMENTS_AT_BONES:
		case ANIM_EVENT_ADD_ATTACHMENT_TO_RANDOM_BONE:
		case ANIM_EVENT_ADD_ATTACHMENT_SHRINK_BONE:
		case ANIM_EVENT_ADD_ATTACHMENT:
			c_AttachmentDefinitionLoad( pGame, 
										pEvent->tAttachmentDef, 
										tAppearanceDef.tHeader.nId,
										pszFolder );
			break;
		case ANIM_EVENT_DO_SKILL:
			pEvent->tAttachmentDef.nAttachedDefId = ExcelGetLineByStringIndex( pGame, DATATABLE_SKILLS, pEvent->tAttachmentDef.pszAttached );
			break;
		}
	}

	char szBaseName[ MAX_FILE_NAME_LENGTH ];
	PStrRemoveExtension( szBaseName, MAX_FILE_NAME_LENGTH, tAnimDef.pszFile );

	if ( tAnimDef.pszFile[ 0 ] == 0 || szBaseName[ 0 ] == 0 || bFileLoaded )
	{
		return;
	}

	char szFile[ MAX_XML_STRING_LENGTH ];

	if (AppIsHellgate() && (AppCommonIsAnyFillpak() || AppTestDebugFlag(ADF_FILL_PAK_BIT)))
	{ // load the hkx file for the other version
#ifdef _WIN64
		PStrPrintf( szFile, MAX_XML_STRING_LENGTH, "%s%s", pszFolder, tAnimDef.pszFile );
#else
		PStrPrintf( szFile, MAX_XML_STRING_LENGTH, "%s%s_64%s", pszFolder,
			szBaseName, PStrGetExtension(tAnimDef.pszFile) );
#endif
		DECLARE_LOAD_SPEC( spec, szFile );
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_DONT_LOAD_FROM_PAK | PAKFILE_LOAD_FREE_BUFFER;
		spec.priority = ASYNC_PRIORITY_APPEARANCES;

		PakFileLoadNow( spec );
	}

#ifdef _WIN64
	if ( AppIsHellgate() )
	{
		PStrPrintf( szFile, MAX_XML_STRING_LENGTH, "%s%s_64%s", pszFolder,
			szBaseName, PStrGetExtension(tAnimDef.pszFile) );
	}
	else
		PStrPrintf( szFile, MAX_XML_STRING_LENGTH, "%s%s", pszFolder, tAnimDef.pszFile );
#else
	PStrPrintf( szFile, MAX_XML_STRING_LENGTH, "%s%s", pszFolder, tAnimDef.pszFile );
#endif

	TCHAR szFullname[ MAX_XML_STRING_LENGTH ];
	FileGetFullFileName( szFullname, szFile, MAX_XML_STRING_LENGTH );

	const UNITMODE_DATA * pModeData = UnitModeGetData( tAnimDef.nUnitMode );

	int nPriorityDelta = NUM_ASYNC_PRIORITIES;
	int nPriority = ASYNC_PRIORITY_0 + pModeData ? (nPriorityDelta * pModeData->nLoadPriorityPercent) / 100 : 0;

	ANIMATION_FILE_CALLBACKDATA * pCallbackData = (ANIMATION_FILE_CALLBACKDATA *) MALLOC( NULL, sizeof( ANIMATION_FILE_CALLBACKDATA ) );
	pCallbackData->nAnimationFileIndex = tAnimDef.nFileIndex;
	pCallbackData->nAppearanceDefId = tAppearanceDef.tHeader.nId;

	DECLARE_LOAD_SPEC( spec, szFullname );
	spec.pool = g_ScratchAllocator;

	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
	spec.flags |= PAKFILE_LOAD_WARN_IF_MISSING;
	if (AppIsHellgate() && AppCommonIsAnyFillpak())
	{
		spec.flags |= PAKFILE_LOAD_DONT_LOAD_FROM_PAK;
	}
	spec.priority = nPriority;
	spec.fpLoadingThreadCallback = sAnimationFileLoadedCallback;
	spec.callbackData = pCallbackData;

	if ( bForceSync )
	{
		PakFileLoadNow(spec);
	} 
	else 
	{
		PakFileLoad(spec);
	}
		
} // c_sAnimationLoad()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
const ANIMATION_GROUP_DATA* AnimationGroupGetData(
	int nGroupIndex)
{
	return (const ANIMATION_GROUP_DATA*)ExcelGetData( NULL, DATATABLE_ANIMATION_GROUP, nGroupIndex );
} // AnimationGroupGetData()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
const ANIMATION_CONDITION_DATA* AnimationConditionGetData(
	int nConditionIndex)
{
	return (const ANIMATION_CONDITION_DATA*)ExcelGetData( NULL, DATATABLE_ANIMATION_CONDITION, nConditionIndex );
} // AnimationConditionGetData()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////

void c_AnimationsInit( 
	struct APPEARANCE_DEFINITION & tAppearanceDef,
	BOOL bForceSync )
{
	if ( (tAppearanceDef.nAnimationCount && tAppearanceDef.pAnimations[ 0 ].nFileIndex == INVALID_ID) )
	{
		c_AnimationComputeFileIndices( &tAppearanceDef );
	}


//	if ( IS_CLIENT( pGame ) )
#ifdef HAVOK_ENABLED
	if ( AppIsHellgate() && !tAppearanceDef.pLoader )
		tAppearanceDef.pLoader = new hkLoader;
#endif

	{
		DWORD pdwFileIndexLoaded[ DWORD_FLAG_SIZE( MAX_ANIMATION_FILES_PER_APPEARANCE ) ];
		ZeroMemory( pdwFileIndexLoaded, sizeof( pdwFileIndexLoaded ) );
		for ( int i = 0; i < tAppearanceDef.nAnimationCount; i++ )
		{
			if ( AppIsHellgate() && AppIsHammer() )
			{ // sometimes we don't have an extension on the file
				PStrReplaceExtension( tAppearanceDef.pAnimations[ i ].pszFile, MAX_FILE_NAME_LENGTH, 
					tAppearanceDef.pAnimations[ i ].pszFile, MAYHEM_FILE_EXTENSION );
			}
			ASSERT_CONTINUE( tAppearanceDef.pAnimations[ i ].nFileIndex != INVALID_ID ||
							 tAppearanceDef.pAnimations[ i ].pszFile[ 0 ] == 0 );
			ASSERT_CONTINUE( tAppearanceDef.pAnimations[ i ].nFileIndex < MAX_ANIMATION_FILES_PER_APPEARANCE );
			BOOL bFileLoaded = TESTBIT( pdwFileIndexLoaded, tAppearanceDef.pAnimations[ i ].nFileIndex );
			c_sAnimationLoad( tAppearanceDef, i, tAppearanceDef.pszFullPath, bForceSync, bFileLoaded );
			SETBIT( pdwFileIndexLoaded, tAppearanceDef.pAnimations[ i ].nFileIndex, TRUE );
		}
		c_sAnimationLoad( tAppearanceDef, -1, tAppearanceDef.pszFullPath, bForceSync, FALSE );

	}
} // c_AnimationsInit()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationsFlagSoundsForLoad( 
	APPEARANCE_DEFINITION & tAppearanceDef,
	BOOL bLoadNow)
{
#if !ISVERSION(SERVER_VERSION)
	GAME * pGame = AppGetCltGame();
	for ( int i = 0; i < tAppearanceDef.nAnimationCount; i++ )
	{
		ANIMATION_DEFINITION & tAnimDef = tAppearanceDef.pAnimations[ i ];

		ASSERTV(tAnimDef.nEventCount < 200, "animdef event count greater than 200!!  appearance definition [%s]  anim def [%s] index [%d]\n", tAppearanceDef.tHeader.pszName, tAnimDef.pszFile, i);
		for ( int i = 0; i < tAnimDef.nEventCount; i++ )
		{
			ANIM_EVENT * pEvent = &tAnimDef.pEvents[ i ];
			switch( pEvent->eType )
			{
			case ANIM_EVENT_FLOAT_ATTACHMENTS_AT_BONES:
			case ANIM_EVENT_ADD_ATTACHMENT_TO_RANDOM_BONE:
			case ANIM_EVENT_ADD_ATTACHMENT_SHRINK_BONE:
			case ANIM_EVENT_ADD_ATTACHMENT:
				c_AttachmentDefinitionFlagSoundsForLoad( pGame, pEvent->tAttachmentDef, bLoadNow );
				break;
			case ANIM_EVENT_DO_SKILL:
				c_SkillFlagSoundsForLoad(pGame, pEvent->tAttachmentDef.nAttachedDefId, bLoadNow);
				break;
			}
		}
	}
#endif	//	!ISVERSION(SERVER_VERSION)
} // c_AnimationsFlagSoundsForLoad()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationsFree( 
	struct APPEARANCE_DEFINITION * pAppearanceDef,
	BOOL bFreePaths )
{
	for ( int i = 0; i < pAppearanceDef->nAnimationCount; i++ )
	{
		ANIMATION_DEFINITION * pAnimDef = &pAppearanceDef->pAnimations[ i ];
		if ( AppIsHellgate() && pAnimDef->pBinding )
		{
			pAnimDef->pBinding = NULL;
		}
		if ( AppIsTugboat() && pAnimDef->pGrannyFile )
		{
			// these could be duplicated, so only free once!
			for( int j = i + 1; j < pAppearanceDef->nAnimationCount; j++ )
			{
				if( (&pAppearanceDef->pAnimations[ j ])->pGrannyFile == pAnimDef->pGrannyFile )
				{
					(&pAppearanceDef->pAnimations[ j ])->pGrannyFile = NULL;
				}
			}
			GrannyFreeFile( pAnimDef->pGrannyFile );
			pAnimDef->pGrannyFile = NULL;
		}
		if ( bFreePaths )
		{
			pAnimDef->tRagdollBlend.Destroy();
			pAnimDef->tRagdollPower.Destroy();
			pAnimDef->tSelfIllumation.Destroy();
			pAnimDef->tSelfIllumationBlend.Destroy();
		}
	}
} // c_AnimationsFree()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationComputeFileIndices( 
	APPEARANCE_DEFINITION * pAppearanceDef )
{
	int nIndexCurr = 0;
	for ( int i = 0; i < pAppearanceDef->nAnimationCount; i ++ )
	{
		ANIMATION_DEFINITION * pAnimDef = &pAppearanceDef->pAnimations[ i ];
		pAnimDef->nFileIndex = INVALID_ID;
		for ( int j = i - 1; j >= 0; j-- )
		{
			if ( PStrCmp( pAnimDef->pszFile, pAppearanceDef->pAnimations[ j ].pszFile ) == 0 )
			{
				pAnimDef->nFileIndex = pAppearanceDef->pAnimations[ j ].nFileIndex;
				break;
			}
		}
		if ( pAnimDef->nFileIndex == INVALID_ID )
		{
			pAnimDef->nFileIndex = nIndexCurr++;
		}
	}
	ASSERT( nIndexCurr < MAX_ANIMATION_FILES_PER_APPEARANCE );
} // c_AnimationComputeFileIndices()
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sHandleAnimationEvent(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD dwTicket )
{
	PERF(ANIM_EVENT);

	ANIM_EVENT * pEvent = (ANIM_EVENT *)data.m_Data1;
	int nModelId = (int)data.m_Data2;
	int nOwnerId = (int)data.m_Data3;
	ANIMATION_DEFINITION * pAnimDef = (ANIMATION_DEFINITION *)data.m_Data4;
	int nAppearanceDefId = e_ModelGetAppearanceDefinition( nModelId );
	if (nAppearanceDefId == INVALID_ID)
	{
		return;
	}

	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDefId );
	if (!pAppearanceDef)
	{
		return;
	}

	if ( pEvent->fRandChance < 1.0f )
	{ // make some things happen randomly
		if (RandGetFloat(game) > pEvent->fRandChance)
		{
			return;
		}
	}

	UNIT * pUnit = c_AppearanceGetUnit( game, nModelId );
	if ( pUnit && pEvent->tCondition.nType != INVALID_ID && ! ConditionEvalute( pUnit, pEvent->tCondition ) )
	{
		return;
	}

	if (!AppIsTugboat())
	{
		if ( pAnimDef && 
			 pAnimDef->nUnitMode != INVALID_ID &&
			 pUnit &&
			 ( !c_AppearanceIsApplyingAnimation(nModelId, pAnimDef) ||
			   !UnitTestMode( pUnit, (UNITMODE)pAnimDef->nUnitMode ) ) )
		{
			return;
		}
	}

	switch ( pEvent->eType )
	{
	case ANIM_EVENT_ADD_ATTACHMENT:
		{
			int nAttachmentId = c_ModelAttachmentAdd( nModelId, pEvent->tAttachmentDef, FILE_PATH_DATA, FALSE, 
				ATTACHMENT_OWNER_ANIMATION, nOwnerId, INVALID_ID );
			if ( pEvent->tAttachmentDef.dwFlags & ATTACHMENT_FLAGS_FLOAT )
			{
				c_ModelAttachmentFloat( nModelId, nAttachmentId );
			}
		}

		break;
	case ANIM_EVENT_REMOVE_ATTACHMENT:
			c_ModelAttachmentRemove( nModelId, pEvent->tAttachmentDef, FILE_PATH_DATA, 0 );
		break;

	case ANIM_EVENT_REMOVE_ALL_ATTACHMENTS:
		c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_ANIMATION, INVALID_ID );
		break;

	case ANIM_EVENT_RAGDOLL_ENABLE:
#ifdef HAVOK_ENABLED
		if ( AppIsHellgate() )
			c_RagdollEnable( nModelId );
#endif
		break;

	case ANIM_EVENT_ANIMATION_FREEZE:
		//c_RagdollFreeze( nModelId );
		if ( AppIsTugboat() )
			c_AppearanceFreeze( nModelId );
		break;

	case ANIM_EVENT_HIDE_WEAPONS:
		c_AppearanceHideWeapons( nModelId, TRUE );
		break;

	case ANIM_EVENT_SHOW_WEAPONS:
		c_AppearanceHideWeapons( nModelId, FALSE );
		break;

	case ANIM_EVENT_RAGDOLL_DISABLE:
#ifdef HAVOK_ENABLED
		if ( AppIsHellgate() )
			c_RagdollDisable( nModelId );
#endif
		break;

	case ANIM_EVENT_DISABLE_DRAW:
		e_ModelSetDrawAndShadow( nModelId, FALSE );
		c_AppearanceHide( nModelId, TRUE );
		break;

	case ANIM_EVENT_ENABLE_DRAW:
		e_ModelSetDrawAndShadow( nModelId, TRUE );
		c_AppearanceHide( nModelId, FALSE );
		break;

	case ANIM_EVENT_APPLY_FORCE_TO_RAGDOLL:
#ifdef HAVOK_ENABLED
		if ( AppIsHellgate() )
		{
			HAVOK_IMPACT tImpact;
			VECTOR vPosition = pEvent->tAttachmentDef.vPosition;
			const MATRIX * pmWorld = e_ModelGetWorldScaled( nModelId );
			if ( pmWorld )
			{
				MatrixMultiply( &vPosition, &vPosition, pmWorld );
			}
			HavokImpactInit( game, tImpact, pEvent->fParam, vPosition, NULL );
			c_RagdollAddImpact( nModelId, tImpact, e_ModelGetScale( nModelId ).fX, 1.0f );
		}
#endif
		break;

	case ANIM_EVENT_DO_SKILL:
		{
			if ( pUnit && pEvent->tAttachmentDef.nAttachedDefId != INVALID_ID )
			{
				SkillStartRequest( game, pUnit, pEvent->tAttachmentDef.nAttachedDefId, INVALID_ID, VECTOR(0), 0, 0 );
			}
		}
		break;

	case ANIM_EVENT_ADD_ATTACHMENT_SHRINK_BONE:
#ifdef _DEBUG
		{
			GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
			if ( (pGlobal->dwFlags & GLOBAL_FLAG_NO_SHRINKING_BONES) != 0 )
			{
				break;
			}
		}
#endif
		{
			ATTACHMENT_DEFINITION tAttachDef = pEvent->tAttachmentDef;

			// use the bone that was shrunk
			int nAppearanceDef = c_AppearanceGetDefinition( nModelId );

			c_AttachmentDefinitionLoad( game, pEvent->tAttachmentDef, nAppearanceDef, "" );

			tAttachDef.nBoneId = c_AppearanceShrunkBoneAdd( nModelId, pEvent->tAttachmentDef.nBoneId );

			if(tAttachDef.nAttachedDefId >= 0)
			{
				int nAttachmentId = c_ModelAttachmentAdd( nModelId, tAttachDef, FILE_PATH_DATA, FALSE, 
					ATTACHMENT_OWNER_ANIMATION, nOwnerId, INVALID_ID );

				if ( nAttachmentId >= 0 && pEvent->tAttachmentDef.dwFlags & ATTACHMENT_FLAGS_FLOAT )
				{
					c_ModelAttachmentFloat( nModelId, nAttachmentId );
				}
			}
		}
		break;

	case ANIM_EVENT_ADD_ATTACHMENT_TO_RANDOM_BONE:
		c_ModelAttachmentAddToRandomBones( nModelId, (int)pEvent->fParam, pEvent->tAttachmentDef, FILE_PATH_DATA, FALSE, 
			ATTACHMENT_OWNER_ANIMATION, nOwnerId, INVALID_ID, FALSE );
		break;

	case ANIM_EVENT_FLOAT_ATTACHMENTS_AT_BONES:
		{
			c_ModelAttachmentAddAtBoneWeight(nModelId, pEvent->tAttachmentDef, FALSE, ATTACHMENT_OWNER_ANIMATION,
												nOwnerId, (pEvent->tAttachmentDef.dwFlags & ATTACHMENT_FLAGS_FLOAT) != 0);
		}
		break;
	case ANIM_EVENT_RAGDOLL_FALL_APART:
#ifdef HAVOK_ENABLED
		if ( AppIsHellgate() )
		{
			BOOL removeConstraints = TRUE;
			BOOL turnConstraintsToBallAndSocket = FALSE;
			BOOL disableAllCollisions = FALSE;
			BOOL dampFallenBodies = FALSE;
			float newAngularDamping = 15.0f;
			BOOL changeCapsulesToBoxes = TRUE;
			c_RagdollFallApart( nModelId, removeConstraints, turnConstraintsToBallAndSocket, disableAllCollisions, 
				dampFallenBodies, newAngularDamping, changeCapsulesToBoxes );
		}
#endif
		break;

	case ANIM_EVENT_CAMERA_SHAKE:
		{
			VECTOR vDirection = RandGetVector(game);
			VectorNormalize(vDirection);

			c_CameraShakeRandom(game, vDirection, pEvent->fParam, float(pEvent->tAttachmentDef.nVolume), 0.0f);
		}
		break;
	}

	c_AppearanceRemoveEvent( nModelId, dwTicket );
} // sHandleAnimationEvent()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationScheduleEvents( 
	GAME * pGame, 
	int nModelId, 
	float fDuration,
	ANIMATION_DEFINITION & tAnimDef,
	BOOL bLooping,
	BOOL bClearAttachments,
	BOOL bOnlyEvents /* = FALSE */ )			// animation isn't played
{
	if ( fDuration == 0.0f )
	{
		fDuration = tAnimDef.fDuration;
	}
	if( AppIsTugboat() && tAnimDef.fVelocity != 0.0f )
	{
		fDuration /= max( .1f, tAnimDef.fVelocity );
	}

	// schedule events
	APPEARANCE_EVENTS * pEvents = c_AppearanceGetEvents( nModelId );
	ASSERT_RETURN( pEvents );
	ASSERT_RETURN( tAnimDef.nEventCount < 100 );

	if ( bClearAttachments )
	{
		c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_ANIMATION, ANIM_DEFAULT_OWNER_ID, 0 );
	}

	TIME tiCurTime = CSchedulerGetTime();

	for ( int i = 0; i < tAnimDef.nEventCount; i++ )
	{
		ANIM_EVENT * pAnimEvent = & tAnimDef.pEvents[ i ];

		TIME tiTime = (TIME)((fDuration * pAnimEvent->fTime) * SCHEDULER_TIME_PER_SECOND);
		tiTime += tiCurTime;

		DWORD dwTicket = 
			CSchedulerRegisterEvent(
				tiTime, 
				sHandleAnimationEvent, 
				CEVENT_DATA(
					(DWORD_PTR)pAnimEvent, 
					nModelId, 
					ANIM_DEFAULT_OWNER_ID, 
					bOnlyEvents ? NULL : (DWORD_PTR)&tAnimDef ) );
		ArrayAddItem(*pEvents, dwTicket);
	}
} // c_AnimationScheduleEvents()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Reschedules animation events to match a new duration, and/or changed animation control speed.
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationRescheduleEvents( 
	const ANIMATION_DEFINITION *pAnimDef,
	APPEARANCE_EVENTS * pEvents, 
	float fDuration ) 	// must already be divided by control speed
{
	// no events?
	if ( pEvents == NULL )
		return;

	ASSERT_RETURN( pAnimDef != NULL );

	// default duration to animation definition duration
	if ( fDuration == 0.0f )
	{
		fDuration = pAnimDef->fDuration;
	}
	if( AppIsTugboat() && pAnimDef->fVelocity != 0.0f )
	{
		fDuration /= max( .1f, pAnimDef->fVelocity );
	}
	TIME tiCurTime = CSchedulerGetTime();
	
	// for each appearance event
	//	if the event matches the anim definition
	//		cancel the event
	//		and reschedule the event given the time offset and new duration
	for ( UINT i = 0; i < pEvents->Count(); ++i )
	{
		DWORD *pTicket = (DWORD *)pEvents->GetPointer( i );
		ASSERT_CONTINUE( pTicket != NULL );
		CEVENT_DATA tEventData;
		if ( CSchedulerGetEventData( (DWORD)*pTicket, tEventData ) &&
			 pAnimDef == ( (ANIMATION_DEFINITION *)tEventData.m_Data4 ) )
		{
			ANIM_EVENT * pAnimEvent = (ANIM_EVENT *)tEventData.m_Data1;

			TIME tiTime = (TIME)((fDuration * pAnimEvent->fTime) * SCHEDULER_TIME_PER_SECOND);
			tiTime += tiCurTime;
			
			// I assume an external check if the event time really needs to change 
			// (check if control speed or duration is actually changing)
			CSchedulerCancelEvent( (DWORD&)*pTicket );

			*pTicket =
				CSchedulerRegisterEvent(
					tiTime, sHandleAnimationEvent, tEventData);
		}
	}
} // c_AnimationRescheduleEvents()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Cancels all scheduled appearance events for a given animation definition
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationCancelEvents( 
	const ANIMATION_DEFINITION *pAnimDef,
	APPEARANCE_EVENTS * pEvents )
{
	// no events?
	if ( pEvents == NULL )
		return;

	ASSERT_RETURN( pAnimDef != NULL );

	// for each appearance event
	//	if the event matches the anim definition
	//		cancel the event
	for ( UINT i = 0; i < pEvents->Count(); ++i )
	{
		DWORD *pTicket = (DWORD *)pEvents->GetPointer( i );
		ASSERT_CONTINUE( pTicket != NULL );
		CEVENT_DATA tEventData;
		if ( CSchedulerGetEventData( (DWORD)*pTicket, tEventData ) &&
			 pAnimDef == ( (ANIMATION_DEFINITION *)tEventData.m_Data4 ) )
		{
			CSchedulerCancelEvent( (DWORD&)*pTicket );
		}
	}
	ArrayClear(*pEvents);
} // c_AnimationRescheduleEvents()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
bool c_AnimationHasFreezeEvent( 
	ANIMATION_DEFINITION & tAnimDef )		
{


	for ( int i = 0; i < tAnimDef.nEventCount; i++ )
	{
		ANIM_EVENT * pAnimEvent = & tAnimDef.pEvents[ i ];
		if( pAnimEvent->eType == ANIM_EVENT_ANIMATION_FREEZE )
		{
			return TRUE;
		}
	}
	return FALSE;
} // c_AnimationHasFreezeEvent()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationDoAllEvents( 
	GAME* game, 
	int nModelId, 
	int nOwnerId,
	ANIMATION_DEFINITION& tAnimDef,
	BOOL bClearAttachments )
{
	if ( bClearAttachments )
		c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_ANIMATION, INVALID_ID, 0 );

	for (int ii = 0; ii < tAnimDef.nEventCount; ii++)
	{
		sHandleAnimationEvent(game, CEVENT_DATA((DWORD_PTR)(&tAnimDef.pEvents[ii]), nModelId, nOwnerId, (DWORD_PTR)(&tAnimDef) ), 0 );
	}
} // c_AnimationDoAllEvents()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationStopAll( 
	GAME * pGame, 
	int nModelId )
{
	c_AppearanceCancelAllEvents( nModelId );

	c_AppearanceDestroyAllAnimations ( nModelId );

	c_AppearanceFreeze( nModelId, FALSE );

#ifdef HAVOK_ENABLED
	c_RagdollDisable( nModelId );
#endif
} // c_AnimationStopAll()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationPlayByMode( 
	UNIT * pUnit, 
	int nUnitMode, 
	float fDuration, 
	DWORD dwPlayAnimFlags,
	ANIMATION_RESULT *pAnimResult )
{
	if ( ! pUnit )
	{
		return;
	}

	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	int nAnimationGroup = pUnit ? UnitGetAnimationGroup( pUnit ) : 0;

	const UNITMODE_DATA * pModeData = (UNITMODE_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODES, nUnitMode );
	ASSERT_RETURN( pModeData );
	if ( AppIsHellgate() )
	{
#ifdef HAVOK_ENABLED
		if( !pModeData->bRagdoll )
		{
			int nAppearanceId = c_UnitGetModelIdThirdPerson( pUnit );
			c_AppearanceFreeze( nAppearanceId, FALSE );
			c_RagdollDisable( nAppearanceId );
			c_RagdollFreeze( nAppearanceId );
	
			if ( GameGetControlUnit( pGame ) == pUnit )
			{
				int nAppearanceFirst = c_UnitGetModelIdFirstPerson( pUnit );
				c_AppearanceFreeze( nAppearanceFirst, FALSE );
				c_RagdollDisable( nAppearanceFirst );
			}
		}
#endif
	}
	else if( AppIsTugboat() )
	{
		int nAppearanceId = c_UnitGetModelIdThirdPerson( pUnit );
		if( nUnitMode != MODE_DEAD )
		{
			c_AppearanceFreeze( nAppearanceId, FALSE );
		}
	}

	if ( nUnitMode == MODE_IDLE && UnitTestFlag( pUnit, UNITFLAG_DONT_PLAY_IDLE ) &&
		(dwPlayAnimFlags & PLAY_ANIMATION_FLAG_FORCE_TO_PLAY) == 0 )
		return;

	if ( AppIsHellgate() && pModeData->bClearAdjustStance )
	{
		c_AnimationEndByMode( pUnit, MODE_ADJUST_STANCE );
	}
	// don't we need to ensure that this is the default anim if it is an idle?
	// if we don't we lose the anim ( At least in Mythos ) and then things pop
	// to a static frame when going back to 'idle' in lots of cases.
	if( nUnitMode == MODE_IDLE )
	{
		dwPlayAnimFlags |= PLAY_ANIMATION_FLAG_DEFAULT_ANIM;
	}

	if ( AppIsHellgate() && GameGetControlUnit( pGame ) == pUnit )
	{
	
		// first person
		c_AnimationPlayByMode( 
			pUnit, 
			pGame, 
			c_UnitGetModelIdFirstPerson( pUnit ), 
			nUnitMode, 
			nAnimationGroup, 
			fDuration, 
			dwPlayAnimFlags, 
			TRUE, 
			INVALID_ID, 
			pAnimResult );
	}

	// third person			
	c_AnimationPlayByMode( 
			pUnit, 
			pGame, 
			c_UnitGetModelIdThirdPerson( pUnit ), 
			nUnitMode, 
			nAnimationGroup, 
			fDuration, 
			dwPlayAnimFlags, 
			TRUE, 
			INVALID_ID, 
			pAnimResult );


	if ( pModeData->bPlayOnInventoryModel )
	{
		c_AnimationPlayByMode( 
			pUnit, 
			pGame, 
			c_UnitGetModelIdInventory( pUnit ), 
			nUnitMode, 
			nAnimationGroup, 
			fDuration, 
			dwPlayAnimFlags, 
			TRUE, 
			INVALID_ID, 
			pAnimResult );

		c_AnimationPlayByMode( 
			pUnit, 
			pGame, 
			c_UnitGetModelIdInventoryInspect( pUnit ), 
			nUnitMode, 
			nAnimationGroup, 
			fDuration, 
			dwPlayAnimFlags, 
			TRUE, 
			INVALID_ID, 
			pAnimResult );
	}

	if ( UnitTestFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
	{

		c_AnimationPlayByMode( 
			pUnit, 
			pGame, 
			c_UnitGetModelIdThirdPersonOverride( pUnit ), 
			nUnitMode, 
			nAnimationGroup, 
			fDuration, 
			dwPlayAnimFlags, 
			TRUE, 
			INVALID_ID, 
			pAnimResult );

	}
			
} // c_AnimationPlayByMode()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AnimationPlayTownAnims( 
	UNIT * pUnit )
{
	// KCK: Check for animation overrides first
	if (UnitHasState(UnitGetGame(pUnit), pUnit, STATE_ANIMATION_OVERRIDE_IN_TOWN))
		return TRUE;
	if (UnitHasState(UnitGetGame(pUnit), pUnit, STATE_ANIMATION_OVERRIDE_AGGRESSIVE))
		return FALSE;
	return pUnit ? (UnitIsInTown( pUnit ) && (!UnitIsPlayer( pUnit ) || !PlayerIsElite( pUnit ))) : FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AnimationEndByMode( 
	UNIT * pUnit, 
	int nUnitMode )
{
	if ( ! pUnit )
	{
		return FALSE;
	}

	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETFALSE( IS_CLIENT( pGame ) );

	UNITMODE_DATA * pModeData = (UNITMODE_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODES, nUnitMode );
	ASSERT_RETFALSE( pModeData );

	if ( pModeData->eBackupMode != MODE_INVALID &&
		( pModeData->bUseBackupModeAnims || !UnitHasOriginalMode( pGame, pUnit, (UNITMODE)nUnitMode ) ) )
	{
		nUnitMode = pModeData->eBackupMode;
	}

	if ( UnitTestFlag( pUnit, UNITFLAG_USING_APPEARANCE_OVERRIDE ) )
	{
		int nAppearanceId = c_UnitGetModelIdThirdPersonOverride( pUnit );
		c_AppearanceEndAnimation( nAppearanceId, nUnitMode );
	}
	if ( AppIsHellgate() && GameGetControlUnit( pGame ) == pUnit )
	{
		int nAppearanceId = c_UnitGetModelIdFirstPerson( pUnit );
		BOOL bRet = c_AppearanceEndAnimation( nAppearanceId, nUnitMode );

		nAppearanceId = c_UnitGetModelIdThirdPerson( pUnit );
		bRet = c_AppearanceEndAnimation( nAppearanceId, nUnitMode );

		return bRet;

	} 
	else 
	{
		int nAppearanceId = c_UnitGetModelId( pUnit ) ;

		return c_AppearanceEndAnimation( nAppearanceId, nUnitMode );
	}
} // c_AnimationEndByMode()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationPlayByMode( 
	UNIT * pUnit,
	GAME * pGame,
	int nModelId,
	int nUnitMode, 
	int nAnimationGroup,
	float fDuration, 
	DWORD dwPlayAnimFlags,
	BOOL bPlaySubgroups,
	int nTargetStance,
	ANIMATION_RESULT* pAnimResult,
	float fEaseOverride,
	DWORD dwAnimationFlags,
	int nAnimation,
	int * pnUnitType )
{
	if ( nModelId == INVALID_ID )
	{
		return;
	}

	int nAppearanceDefId = c_AppearanceGetDefinition( nModelId );
	if ( nAppearanceDefId == INVALID_ID )
	{
		return;
	}

	BOOL bForcePlaySubgroups = FALSE;
	APPEARANCE_DEFINITION * pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !AppIsTugboat() && pAppearanceDef && (pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_ALWAYS_PLAY_SUBGROUP_ANIMS) != 0 )
	{
		bForcePlaySubgroups = TRUE;
	}

	UNITMODE_DATA * pModeData = (UNITMODE_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODES, nUnitMode );
	ASSERT_RETURN(pModeData);
	
	if ( (pModeData->bUseBackupModeAnims || !UnitHasOriginalMode( pGame, pUnit, (UNITMODE)nUnitMode )) &&
		pModeData->eBackupMode != MODE_INVALID )
	{
		nUnitMode = pModeData->eBackupMode;
	}

	if ( pModeData->bPlayJustDefault ||
		AppIsTugboat() )
	{
		bPlaySubgroups = FALSE;
		nAnimationGroup = 0;
	}
	ANIMATION_GROUP_DATA * pAnimGroupDef = (ANIMATION_GROUP_DATA *) ExcelGetData( pGame, DATATABLE_ANIMATION_GROUP, nAnimationGroup );
	if ( bPlaySubgroups || bForcePlaySubgroups )
	{
		// legs always stack on top of the other animations
		// play the legs first because they usually don't change stances and the arms do
		if (  (pAnimGroupDef && pAnimGroupDef->nLegAnims != INVALID_ID) &&
			(bForcePlaySubgroups || (pModeData->bPlayLegAnim && pAnimGroupDef->bPlayLegAnims ) ) )
		{
			c_AnimationPlayByMode( pUnit, pGame, nModelId, nUnitMode, pAnimGroupDef->nLegAnims,
				fDuration, dwPlayAnimFlags, FALSE, nTargetStance, pAnimResult, fEaseOverride, dwAnimationFlags, nAnimation );
		}
		if ( bForcePlaySubgroups || ( pAnimGroupDef && pAnimGroupDef->bPlayRightLeftAnims ) )
		{
			if ( pModeData->bPlayRightAnim && pAnimGroupDef->nRightAnims != INVALID_ID )
			{
				c_AnimationPlayByMode( pUnit, pGame, nModelId, nUnitMode, pAnimGroupDef->nRightAnims,
					fDuration, dwPlayAnimFlags, FALSE, nTargetStance, pAnimResult, fEaseOverride, dwAnimationFlags, nAnimation );
			}

			if ( pModeData->bPlayLeftAnim && pAnimGroupDef->nLeftAnims != INVALID_ID )
			{
				c_AnimationPlayByMode( pUnit, pGame, nModelId, nUnitMode, pAnimGroupDef->nLeftAnims, 
					fDuration, dwPlayAnimFlags, FALSE, nTargetStance, pAnimResult, fEaseOverride, dwAnimationFlags, nAnimation );
			}
		}
		if ( pAnimGroupDef->bOnlyPlaySubgroups )
		{
			return;
		}
	}

	if ( !pAppearanceDef )
	{
		return;
	}

	// find animations that meet the criteria
#define MAX_ANIMATIONS_TO_PICK_FROM 10
	ANIMATION_DEFINITION * pValidAnimations[ MAX_ANIMATIONS_TO_PICK_FROM ];
	int nValidAnimationCount = 0;
	int nWeightTotal = 0;
	BOOL bPickVariation = FALSE;
	const ANIMATION_GROUP_DATA* pAnimGroup = AnimationGroupGetData( nAnimationGroup );  // for debugging
	REF(pAnimGroup);
	ANIMATION_DEFINITION * pAnimDefCurr 
		= AppearanceDefinitionGetAnimation ( pGame, nAppearanceDefId, nAnimationGroup, (UNITMODE)nUnitMode );

	BOOL bIsInTown = c_AnimationPlayTownAnims( pUnit );
	int nStance = c_AppearanceGetStance( nModelId ); // grab the stance before we change it
	for ( ; pAnimDefCurr; pAnimDefCurr = pAnimDefCurr->pNextInGroup )
	{

		//// check condition restrictions
		BOOL bIgnoreStance = FALSE;
		if (pUnit)
		{

			if ( AppIsHellgate() )
			{
				if ( bIsInTown && ( pAnimDefCurr->dwFlags & ANIMATION_DEFINITION_FLAG_NOT_IN_TOWN ) != 0 )
				{
					continue;
				}
	
				if ( !bIsInTown && ( pAnimDefCurr->dwFlags & ANIMATION_DEFINITION_FLAG_ONLY_IN_TOWN ) != 0 )
				{
					continue;
				}
			}

			// is there a condition link
			BOOL bFailedConditions = FALSE;
			for ( int i = 0; i < NUM_ANIMATION_CONDITIONS; i++ )
			{
				int nAnimationCondition = pAnimDefCurr->nAnimationCondition[ i ];
				if (nAnimationCondition == INVALID_LINK)
					continue;

				ANIMATION_CONDITION_DATA * pCondition = (ANIMATION_CONDITION_DATA *) ExcelGetData( NULL, DATATABLE_ANIMATION_CONDITION, nAnimationCondition );
				if ( pCondition->bCheckOnPlay && ! c_AnimationConditionEvaluate( pGame, pUnit, nAnimationCondition, nModelId, pAnimDefCurr ) )
				{
					bFailedConditions = TRUE;
					break;
				}

				if ( pCondition->bIgnoreStanceOutsideCondition )
				{
					bIgnoreStance = TRUE;
				}
			}
			if ( bFailedConditions )
				continue;

		}		
	
		if ( !bIgnoreStance && 
			 (pAnimDefCurr->nStartStance  != nStance &&
			  pAnimDefCurr->nStartStance2 != nStance &&
			  pAnimDefCurr->nStartStance3 != nStance &&
			 (pAnimDefCurr->dwFlags & ANIMATION_DEFINITION_FLAG_USE_STANCE ) != 0 ) )
		{
			 continue;
		}

		if ( !bIgnoreStance && nTargetStance != INVALID_ID &&
			 pAnimDefCurr->nEndStance != nTargetStance )
		{			 
			 continue;
		}

		if ( pAnimDefCurr->tCondition.nType != INVALID_ID )
		{
			if ( ! ConditionEvalute( pUnit, pAnimDefCurr->tCondition, INVALID_ID, INVALID_ID, NULL, (UNITTYPE*)pnUnitType ) )
			{
				continue;
			}
			else if ( pAnimDefCurr->dwFlags & ANIMATION_DEFINITION_FLAG_FORCE_ON_CONDITION )
			{ // condition passed, we must use this animation
				pValidAnimations[ 0 ] = pAnimDefCurr;
				nWeightTotal = pAnimDefCurr->nWeight;
				nValidAnimationCount = 1;
				break;
			}
		}

		if ( pAnimDefCurr->dwFlags & ANIMATION_DEFINITION_FLAG_PICK_VARIATION )
		{
			bPickVariation = TRUE;
		}
		
		if ( nValidAnimationCount < MAX_ANIMATIONS_TO_PICK_FROM )
		{
			pValidAnimations[ nValidAnimationCount ] = pAnimDefCurr;
			nValidAnimationCount++;
			nWeightTotal += pAnimDefCurr->nWeight;
		}
	}

	if ( nValidAnimationCount == 0 )
	{
		if ( ! pModeData->bNoAnimation && pModeData->bIsAggressive )
		{
			c_AppearanceMakeAggressive( nModelId );
		}
		return;
	}

	// see if we do events only
	if ( pModeData->bNoAnimation )
	{
		dwPlayAnimFlags |= PLAY_ANIMATION_FLAG_ONLY_EVENTS;
	}

	if ( pModeData->bRandStartTime )
	{
		dwPlayAnimFlags |= PLAY_ANIMATION_FLAG_RAND_START_TIME;
	}

	// play all variations if mode tell us to ... if the mode becomes a "not good" place to
	// store this, we could do it with a checkbox on the animation definition dialog
	if(dwAnimationFlags & ANIMATION_DEFINITION_FLAG_PICK_VARIATION)
		bPickVariation = TRUE;

	if (pModeData->bPlayAllVariations && ! bPickVariation )
	{
			for (int i = 0; i < nValidAnimationCount; ++i)
			{
				c_AnimationPlay( 
					pGame, 
					nModelId, 
					pValidAnimations[ i ] , 
					fDuration, 
					dwPlayAnimFlags,
					pAnimResult,
					fEaseOverride );		
			}
	}
	else
	{
	
		int nValidIndex = 0;
		if ( nValidAnimationCount != 1 )
		{
			if ( bPickVariation )
			{// sometimes we just want to always use the same one per monster
				nValidIndex = (nAnimation == INVALID_ID?nModelId: nAnimation) % nValidAnimationCount;
			} 
			else 
			{
				// roll the dice to pick which one to use
				int nRoll = RandGetNum(pGame, nWeightTotal);
				ANIMATION_DEFINITION * pAnimDef = pValidAnimations[ 0 ];
				while ( nRoll >= 0 )
				{
					if ( pAnimDef->nWeight == 0 )
					{
						break;
					}
					nRoll -= pAnimDef->nWeight;
					if ( nRoll >= 0 )
					{
						nValidIndex++;
						pAnimDef = pValidAnimations[ nValidIndex ];
					}
				}
			}
		}
	
		c_AnimationPlay( 
			pGame, 
			nModelId, 
			pValidAnimations[ nValidIndex ] , 
			fDuration, 
			dwPlayAnimFlags,
			pAnimResult,
			fEaseOverride );			
	}

	// some modes reset mix times of all animations on the appearance when they are set
	// we do this with the jump landing animations, they reset the mix on the appearance 
	// so the feet on the landing line up with the start of a new run cycle
	if (pModeData->bResetMixableOnStart /*|| dwAnimationFlags & ANIMATION_DEFINITION_FLAG_PICK_VARIATION*/)
	{
		c_AppearanceSyncMixableAnimations( nModelId, 0.0f );
	} 

	ATTACHMENT_HOLDER * pAttachmentHolder = c_AttachmentHolderGet(nModelId);
	for(int i=0; i<pAttachmentHolder->nCount; i++)
	{
		if(pAttachmentHolder->pAttachments[i].eType == ATTACHMENT_MODEL && pAttachmentHolder->pAttachments[i].dwFlags & ATTACHMENT_FLAGS_INHERIT_PARENT_MODES && pAttachmentHolder->pAttachments[i].nAttachedId != INVALID_ID)
		{
			c_AnimationPlayByMode(pUnit, pGame, pAttachmentHolder->pAttachments[i].nAttachedId, nUnitMode, nAnimationGroup, fDuration, dwPlayAnimFlags, bPlaySubgroups, nTargetStance, pAnimResult, fEaseOverride, dwAnimationFlags, nAnimation);
		}
	}

} // c_AnimationPlayByMode()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AnimationPlay( 
	GAME * pGame, 
	int nModelId, 
	ANIMATION_DEFINITION * pAnimDef, 
	float fDuration, 
	DWORD dwPlayAnimFlags,
	ANIMATION_RESULT *pAnimResult,
	float fEaseOverride )
{
	if ( nModelId == INVALID_ID )
	{
		return FALSE;
	}

	float fDurationForEvents = fDuration;

	// play animation
	BOOL bOnlyEvents = ( dwPlayAnimFlags & PLAY_ANIMATION_FLAG_ONLY_EVENTS ) != 0;
	if ( ! bOnlyEvents )
	{
		// animations with a freeze event DON'T loop.
		if( c_AnimationHasFreezeEvent( *pAnimDef ) )
		{
			dwPlayAnimFlags &= ~PLAY_ANIMATION_FLAG_LOOPS;
		}

		BOOL bResult = 
			c_AppearancePlayAnimation ( 
				pGame, 
				nModelId, 
				pAnimDef, 
				fDuration, 
				dwPlayAnimFlags, 
				pAnimResult, 
				&fDurationForEvents,
				fEaseOverride );

		if (bResult == FALSE)
		{
			return FALSE;
		}
	}

	c_AnimationScheduleEvents( 
		pGame, 
		nModelId,
		AppIsHellgate() ? fDurationForEvents : fDuration, 
		*pAnimDef, 
		dwPlayAnimFlags & PLAY_ANIMATION_FLAG_LOOPS, 
		!bOnlyEvents, 
		bOnlyEvents );

	return TRUE;
} // c_AnimationPlay()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationComputeDuration( 
	ANIMATION_DEFINITION & tAnimation )
{
	if ( AppIsHellgate() && tAnimation.pBinding != NULL )
	{
#ifdef HAVOK_ENABLED
		ASSERT_RETURN( tAnimation.pBinding && tAnimation.pBinding->m_animation );
		tAnimation.fDuration = tAnimation.pBinding->m_animation->m_duration;
#endif //#ifdef HAVOK_ENABLED
	}
	
	if ( AppIsTugboat() && tAnimation.pGrannyAnimation != NULL )
	{
		ASSERT_RETURN( tAnimation.pGrannyAnimation );
		tAnimation.fDuration = tAnimation.pGrannyAnimation->Duration;
	}
	
} // c_AnimationComputeDuration()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimationComputeVelocity( 
	ANIMATION_DEFINITION & tAnimation )
{
	if ( AppIsTugboat() )
	{
		if( tAnimation.nUnitMode == MODE_WALK ||
			tAnimation.nUnitMode == MODE_RUN )
		{
			tAnimation.bIsMobile = TRUE;
		}
		else
		{
			tAnimation.bIsMobile = FALSE;
		}
		//float fDuration = tAnimation.pGrannyAnimation->Duration;
		return;
	}

#ifdef HAVOK_ENABLED
	if (! tAnimation.pBinding || ! tAnimation.pBinding->m_animation )
	{
		return;
	}

	const hkAnimatedReferenceFrame * pMotion = tAnimation.pBinding->m_animation->m_extractedMotion;
	if ( ! pMotion )
	{
		return;
	}


	float fDuration = tAnimation.pBinding->m_animation->m_duration;
	ASSERT_RETURN( fDuration );
	hkQsTransform mMotion;
	pMotion->getReferenceFrame( fDuration, mMotion );

	float fDeltaY = mMotion.getTranslation()( 1 );
	float fDeltaX = mMotion.getTranslation()( 0 );

	if ( ( tAnimation.dwFlags & ANIMATION_DEFINITION_FLAG_FORCE_SPEED ) == 0 )
	{
		if ( fDeltaY != 0.0f || fDeltaX != 0.0f )
		{
			tAnimation.fVelocity = max( fabsf( fDeltaY / fDuration ), fabsf( fDeltaX / fDuration ) );
			if (tAnimation.fVelocity < 0.01f)
			{
				tAnimation.fVelocity = 0.0f;
			}
		}
	}

	float fTurnSpeed = mMotion.m_rotation.getAngle();

	if ( fTurnSpeed != 0.0f )
	{
		tAnimation.fTurnSpeed = fTurnSpeed / fDuration;
	}
#endif //#ifdef HAVOK_ENABLED

} // c_AnimationComputeVelocity()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
ANIM_EVENT * c_AnimationEventAdd( ANIMATION_DEFINITION & tAnimDef )
{
	tAnimDef.nEventCount++;
	tAnimDef.pEvents = ( ANIM_EVENT * ) REALLOC( NULL, tAnimDef.pEvents, sizeof( ANIM_EVENT ) * tAnimDef.nEventCount );
	ANIM_EVENT * pEvent = & tAnimDef.pEvents[ tAnimDef.nEventCount - 1 ];
	ZeroMemory( pEvent, sizeof( ANIM_EVENT ) );
	pEvent->eType = ANIM_EVENT_ADD_ATTACHMENT;
	pEvent->fRandChance = 1.0f;
	e_AttachmentDefInit  ( pEvent->tAttachmentDef );
	ConditionDefinitionInit( pEvent->tCondition );
	return pEvent;
} // c_AnimationEventAdd()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AnimationConditionEvaluate(
	GAME * pGame,
	UNIT* pUnit,
	int nAnimationCondition,
	int nModelId,
	ANIMATION_DEFINITION * pAnimDef )
{
	// is there a condition link
	if (nAnimationCondition == INVALID_LINK )
	{
		return TRUE;
	}
	if ( ! pUnit )
	{
		return TRUE;
	}

	const ANIMATION_CONDITION_DATA * pAnimCondition = AnimationConditionGetData( nAnimationCondition );

	if ( ! pAnimCondition->pfnHandler )
	{
		return TRUE;
	}

	ANIMATION_CONDITION_PARAMS tParams;
	tParams.nModelId = nModelId;
	tParams.pAnimDef = pAnimDef;
	tParams.pGame = pGame;
	tParams.pUnit = pUnit;
	return pAnimCondition->pfnHandler( tParams );
} // c_AnimationConditionEvaluate()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimConditionsAggressive(
	ANIMATION_CONDITION_PARAMS & tParams )
{	
	// KCK: Check for animation overrides first
	if (UnitHasState(tParams.pGame, tParams.pUnit, STATE_ANIMATION_OVERRIDE_AGGRESSIVE))
		return TRUE;
	if (UnitHasState(tParams.pGame, tParams.pUnit, STATE_ANIMATION_OVERRIDE_IN_TOWN))
		return FALSE;
	return c_AppearanceIsAggressive( tParams.nModelId );
} // sAnimConditionsAggressive()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimConditionsPassive(
	ANIMATION_CONDITION_PARAMS & tParams )
{	
	return !c_AppearanceIsAggressive( tParams.nModelId );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimConditionsOtherAnimsUseSameStance(
	ANIMATION_CONDITION_PARAMS & tParams )
{	
	ASSERT_RETFALSE( tParams.nModelId != INVALID_ID );
	ASSERT_RETFALSE( tParams.pAnimDef );
	ASSERT_RETFALSE( tParams.pUnit );

	return c_AppearanceCheckOtherAnimsForSameStance( tParams.nModelId, tParams.pAnimDef );
} // sAnimConditionsOtherAnimsUseSameStance()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimConditionsNotMoving(
	ANIMATION_CONDITION_PARAMS & tParams )
{	
	return ! UnitTestModeGroup( tParams.pUnit, MODEGROUP_MOVE );
} // sAnimConditionsNotMoving()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimConditionsMoving(
	ANIMATION_CONDITION_PARAMS & tParams )
{	
	return UnitTestModeGroup( tParams.pUnit, MODEGROUP_MOVE );
} // sAnimConditionsMoving()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimConditionsNotAdjustingContext(
	ANIMATION_CONDITION_PARAMS & tParams )
{	
	return ! c_AppearanceIsAdjustingContext( tParams.nModelId );
} // sAnimConditionsNotAdjustingContext()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimConditionsOnlyPlayers(
	ANIMATION_CONDITION_PARAMS & tParams )
{	
	return !tParams.pUnit || UnitIsA( tParams.pUnit, UNITTYPE_PLAYER );
} // sAnimConditionsOnlyPlayers()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimConditionsNotPlayers(
	ANIMATION_CONDITION_PARAMS & tParams )
{	
	return !tParams.pUnit || !UnitIsA( tParams.pUnit, UNITTYPE_PLAYER );
} // sAnimConditionsNotPlayers()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
struct ANIMATION_CONDITION_LOOKUP
{
	const char *pszName;
	PFN_ANIMATION_CONDITION fpHandler;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static const ANIMATION_CONDITION_LOOKUP sgtAnimationConditionLookupTable[] = 
{
	{ "Passive",						sAnimConditionsPassive							},
	{ "Aggressive",						sAnimConditionsAggressive						},
	{ "OtherAnimsUseSameStance",		sAnimConditionsOtherAnimsUseSameStance			},
	{ "NotMoving",						sAnimConditionsNotMoving						},
	{ "Moving",							sAnimConditionsMoving							},
	{ "NotAdjustingStance",				sAnimConditionsNotAdjustingContext				},
	{ "OnlyPlayers",					sAnimConditionsOnlyPlayers						},
	{ "NotPlayers",						sAnimConditionsNotPlayers						},

	{ NULL, NULL }		// keep this last!
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ExcelAnimationConditionPostProcess(
	EXCEL_TABLE * table)
{
#if !ISVERSION(SERVER_VERSION)
	// go through all skill event types
	for (unsigned int i = 0; i < ExcelGetCountPrivate(table); ++i)
	{
		// get skill data
		ANIMATION_CONDITION_DATA *pConditionData = (ANIMATION_CONDITION_DATA *)ExcelGetDataPrivate(table, i);
		ASSERT_RETFALSE(pConditionData);

		// clear out event handler
		pConditionData->pfnHandler = NULL;

		// what function are we looking for
		const char *pszFunctionName = pConditionData->szFunctionName;

		// scan table looking for function match if we have a string to look for
		int nFunctionNameLength = PStrLen( pszFunctionName );
		if ( nFunctionNameLength <= 0 )
		{
			continue;
		}

		for ( int j = 0; sgtAnimationConditionLookupTable[ j ].pszName != NULL; ++j )
		{
			const ANIMATION_CONDITION_LOOKUP *pLookup = &sgtAnimationConditionLookupTable[ j ];
			if ( PStrCmp( pLookup->pszName, pConditionData->szFunctionName ) == 0 )
			{
				pConditionData->pfnHandler = pLookup->fpHandler;
				break;
			}
		}

		// check for no match
		if ( pConditionData->pfnHandler == NULL )
		{
			ASSERTV_CONTINUE(0, "Unable to find animation event handler '%s' for '%s' on row '%d'", pConditionData->szName, pConditionData->szFunctionName, i);	
		}
	}
#endif
	return TRUE;
}

