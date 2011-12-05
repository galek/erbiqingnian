//----------------------------------------------------------------------------
// c_ui.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "console.h"
#include "e_primitive.h"
#include "game.h"
#include "units.h"
#include "fontcolor.h"
#include "gameunits.h"
#include "gamelist.h"
#include "pets.h"
#include "prime.h"
#include "c_camera.h"
#include "camera_priv.h"
#include "c_font.h"
#include "colors.h"
#include "uix.h"
#include "uix_components_hellgate.h"
#include "monsters.h"
#include "quests.h"
#include "teams.h"
#include "c_ui.h"
#include "e_settings.h"
#include "e_main.h"
#include "matrix.h"
#include "objects.h"
#include "wardrobe.h"
#include "weapons.h"
#include "skills.h"
#include "items.h"
#include "npc.h"
#include "level.h"
#include "unitmodes.h"
#include "c_input.h"
#include "c_appearance.h"
#include "room.h"
#include "Quest.h"
#include "states.h"
#include "quest_tutorial.h"
#include "unittag.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
UI_CLIENT_UNITS gUIClientUnits;
BOOL gUIClientUnitsInited( FALSE );
float gflNameHeightOverride = -1.0f;
static const int MAX_UNIT_DISPLAYS = 256;  // arbitrary
BOOL gbDisplayCollisionHeightLadder = FALSE;
	
#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UNIT * c_UIGetMouseUnit( GAME * pGame )
{

	UNIT * unit = GameGetControlUnit(pGame);
	if (!unit)
	{
		return NULL;
	}
	if (!UnitGetRoom(unit))
	{
		return NULL;
	}
	
	LEVEL * pLevel = UnitGetLevel( unit );
	// used in point line calcs
	BOOL bFattenUnits = TRUE;

	float fMaxLen = 900000;
	float fLengthSq = fMaxLen;
	float fClosestSq = fLengthSq;
	UNIT* selected_unit = NULL;
	VECTOR vOrig;
	VECTOR vDir;

		UI_TB_MouseToScene( pGame, &vOrig, &vDir );
		CRayIntersector tRayIntersector( vOrig, vDir );

		// this needs to be optimized to only search through rooms that matter

		for (ROOM* room = LevelGetFirstRoom( pLevel); room; room = LevelGetNextRoom(room))
		{
			for (int nTargetList = TARGET_COLLISIONTEST_START; nTargetList < NUM_TARGET_TYPES; nTargetList++)
			{
				for (UNIT* pTestUnit = room->tUnitList.ppFirst[nTargetList]; pTestUnit; pTestUnit = pTestUnit->tRoomNode.pNext)
				{ 
					// don't target ourselves
					if (pTestUnit == unit)
					{
						continue;
					}
					if( UnitGetStat(pTestUnit, STATS_DONT_TARGET) )
					{
						continue;
					}
 
					// for now, only select monsters or players
					const UNIT_DATA * pTestUnitData = UnitGetData( pTestUnit );

					BOOL bIsMonster = UnitGetGenus(pTestUnit) == GENUS_MONSTER &&
									  !IsUnitDeadOrDying( pTestUnit );
					if( bIsMonster )
					{
						//If is an NPC and isn't interactive we needto set it as a none monster
						if( UnitIsNPC( pTestUnit ) && !UnitCanInteractWith( pTestUnit, unit ) )
						{
							//BUT before we do we need to see if they are on the same team...
							bIsMonster = !( UnitGetTeam( pTestUnit ) == UnitGetTeam( unit ) );
						}
					}

					BOOL bCanInteractWith = UnitCanInteractWith( pTestUnit, unit );
					BOOL bIsQuestObject = AppIsTugboat() && QuestUnitIsUnitType_QuestItem( pTestUnit ) &&
										  bCanInteractWith;
					BOOL bIsPlayer = UnitIsA(pTestUnit, UNITTYPE_PLAYER) &&
						!IsUnitDeadOrDying( pTestUnit );
					BOOL bIsOpenable = UnitIsA(pTestUnit, UNITTYPE_OBJECT) &&
						bCanInteractWith &&
						UnitDataTestFlag(pTestUnitData, UNIT_DATA_FLAG_INTERACTIVE) && 
						!IsUnitDeadOrDying(pTestUnit); 
					BOOL bIsTrigger = FALSE;
					if( UnitTestFlag( pTestUnit, UNITFLAG_TRIGGER ) && 
						UnitTestFlag( pTestUnit, UNITFLAG_DONT_TRIGGER_BY_PROXIMITY ) &&
						bCanInteractWith &&
						!IsUnitDeadOrDying(pTestUnit) )
					{
						bIsTrigger = TRUE;
					}
//					BOOL bIsWarp = ObjectIsWarp( pTestUnit );
					BOOL bIsDoor = ObjectIsDoor( pTestUnit );
					BOOL bIsMerchant = UnitIsMerchant( pTestUnit ) || UnitIsTaskGiver( pTestUnit ) || UnitIsTradesman( pTestUnit );
					BOOL bDestructible = UnitIsA(pTestUnit, UNITTYPE_DESTRUCTIBLE) && 
										 !IsUnitDeadOrDying(pTestUnit); 
					BOOL bTakeable = UnitTestFlag(pTestUnit, UNITFLAG_CANBEPICKEDUP) && 
									 ItemCanPickup(unit, pTestUnit) == PR_OK;
					BOOL bPlayerPet = PetIsPlayerPet( pGame, pTestUnit );
					BOOL bIsOpen = TRUE;
					if( bIsTrigger )
					{	
						int nModelId = c_UnitGetModelId( pTestUnit );
						if ( e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW ) )
						{
							continue;
						}
					}
					if( !pTestUnit->bVisible )
					{
						continue;
					}
					if( bIsDoor )
					{
							if( !( UnitTestMode( pTestUnit, MODE_OPENED ) ||
									UnitTestMode( pTestUnit, MODE_OPEN ) ) )
							{
								bIsOpen = FALSE;
								
							}
							else
							{
								continue;
							}
					}
					// OK, no more mouseovers on player pets anymore. Frustrating!
					if( bPlayerPet && UnitsGetTeamRelation(pGame, unit, pTestUnit) != RELATION_HOSTILE)
					{
						continue;
					}
					// only care about interactable objects
					if ( //pTestUnit->bDropping ||
						( !bIsQuestObject && !bIsMonster && !bIsPlayer && !bIsOpenable && !bTakeable && !bDestructible && !bIsTrigger ) )
					{
						continue;
					}
					if( bCanInteractWith == FALSE &&									//not interactive
						UnitGetStat( pTestUnit, STATS_UNIT_THEME ) != INVALID_ID &&		//is in a theme
						c_UnitGetNoDraw( pTestUnit ) )									//not drawing. We can't target then.
					{
						continue;
					}
					float fHorizontalUnitFattening = bFattenUnits ? 2.0f : 1.0f;
					float fVerticalUnitFattening   = bFattenUnits ? 1.0f : 1.0f;
					/*if( bDestructible || bIsOpenable )
					{
						fHorizontalUnitFattening *= .7f;
					}
					if( bDestructible || bIsOpenable )
					{
						fVerticalUnitFattening *= .6f;
					}*/

					BOOL bFlyingUnit = TESTBIT( pTestUnit->pdwMovementFlags, MOVEFLAG_FLY );
					BOUNDING_BOX box;
					// create a pseudo bounding box / axis alligned
					float fRadius = 1;
					if( bIsQuestObject || bIsTrigger ||
						bIsOpenable )
					{
						fRadius = UnitGetRawCollisionRadius( pTestUnit );
						if( fRadius < 1 )
							fRadius *= fHorizontalUnitFattening;	// 10% selection buffer lovin
					}
					else
					{
						fRadius = UnitGetCollisionRadius( pTestUnit );
						if( fRadius < 1.0f )
						{
							fRadius = 1.0f;
						}
						if( !UnitIsA( pTestUnit, UNITTYPE_PLAYER ) )
						{
							if( fRadius < 1.5f )
							{
								fRadius = 1.5f;
							}
						}
					}

					float fHeight = UnitGetCollisionHeight( pTestUnit ) * fVerticalUnitFattening;
					if( bIsPlayer )
					{
						fHeight += .4f;
					}
					VECTOR vDrawnPosition = pTestUnit->vDrawnPosition;
					// HACK! For doors, when they're open we artificially offset their centerpoint to the 'opened' position
					// right now this is set up for swinging doors that open to the left. FIXME later! Travis
					if( bIsOpen && bIsDoor )
					{
						VECTOR vRight;
						VectorCross( vRight, pTestUnit->vUpDirection, pTestUnit->vFaceDirectionDrawn );
						vDrawnPosition -= pTestUnit->vFaceDirectionDrawn * 1.8f;
						vDrawnPosition += vRight * 1.8f;
						fHorizontalUnitFattening *= .3f;
						fVerticalUnitFattening *= .5f;
					}
					box.vMin.fX = vDrawnPosition.fX - fRadius;
					box.vMin.fY = vDrawnPosition.fY - fRadius;
					box.vMin.fZ = vDrawnPosition.fZ;
					if ( bFlyingUnit )
						box.vMin.fZ -= fHeight / 2.0f;

					box.vMax = box.vMin;
					VECTOR vTemp;
					vTemp.fX = vDrawnPosition.fX + fRadius;
					vTemp.fY = vDrawnPosition.fY + fRadius;
					vTemp.fZ = box.vMin.fZ + fHeight;

					BoundingBoxExpandByPoint( &box, &vTemp );

					if ( !BoundingBoxIntersectRay( box, tRayIntersector, fMaxLen ) )
						continue;

					// is it the closest unit we are dealing with?
					VECTOR vDirToTestUnit = UnitGetPosition( pTestUnit ) - vOrig;
					float dist_sq = VectorLengthSquared( vDirToTestUnit );
					if( bIsTrigger && bIsOpen )
					{
						dist_sq += 40000;
					}
					else if( bDestructible )
					{
						dist_sq += 10000;
					}
					else if( bIsOpenable  && bIsOpen )
					{
						dist_sq += 20000;
					}
					else if( bTakeable )
					{
						dist_sq += 30000;
					}
					if( bIsMerchant )
					{
						dist_sq += 41000;
					}
					if( bIsDoor && bIsOpen )
					{
						dist_sq += 50000;
					}
					if( bIsPlayer )
					{
						if( UnitsGetTeamRelation(pGame, unit, pTestUnit) == RELATION_HOSTILE )
						{
							//dist_sq += 10000;
						}
						else
						{
							dist_sq += 55000;
						}
					}
					if( bPlayerPet )
					{
						dist_sq += 55000;
					}
					// dead units that we can interact with should always be biased back.
					if( IsUnitDeadOrDying(pTestUnit) )
					{
						dist_sq += 50000;
					}

					if ( dist_sq > fClosestSq )
						continue;

					VectorNormalize( vDirToTestUnit );
					if ( VectorDot( vDirToTestUnit, vDir ) < 0 )
						continue;

					selected_unit = pTestUnit;
					fClosestSq = dist_sq;
				}
			}
		}
		return selected_unit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitHasInteraction(
	UNIT *pUnit)
{
	if (UnitIsMerchant( pUnit ))
	{
		return TRUE;
	}
	else if (UnitIsHealer( pUnit ))
	{
		return TRUE;
	}
	else if (UnitIsTradesman( pUnit ))
	{
		return TRUE;
	}
	else if (UnitIsPvPSignerUpper( pUnit ))
	{
		return !AppTestFlag(AF_CENSOR_NO_PVP);
	}
	else if (UnitHasState( UnitGetGame( pUnit ), pUnit, STATE_NPC_TRIAGE ) )
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void c_sUIUpdateSelectionHellgate( GAME * pGame )
{

#define GFXCOLOR_HALFGREEN	0x7f00ff00
#define GFXCOLOR_HALFRED	0x7fff0000

	if (!pGame)
	{
		// turn the crosshairs or interaction cursor off
		UITargetChangeCursor( NULL, TCUR_NONE, FALSE, FALSE );
		return;
	}

	ASSERT( IS_CLIENT( pGame ) );

	UNIT * unit = GameGetControlUnit(pGame);
	if (!unit)
	{
		return;
	}
	if (!UnitGetRoom(unit))
	{
		return;
	}

	if ( UnitHasState( pGame, unit, STATE_QUEST_RTS ) )
	{
		UNIT * selection = NULL;
		c_UI_RTS_Selection( pGame, NULL, &selection );
		if ( selection )
		{
			UISetTargetUnit( UnitGetId( selection ), TRUE );
		}
		else
		{
			UISetTargetUnit( INVALID_ID, FALSE );
		}

		// turn the crosshairs or interaction cursor off
		UITargetChangeCursor( NULL, TCUR_NONE, FALSE, FALSE );
		return;
	}


	// set up all of the possible targets and whether they are in range
	int nWardrobe = UnitGetWardrobe( unit );
	ASSERT_RETURN( nWardrobe != INVALID_ID );

	int pnSkills[ MAX_WEAPONS_PER_UNIT + 2 ];
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT + 2 ];
	BOOL bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 2 ];
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		pWeapons[ i ] = WardrobeGetWeapon( pGame, nWardrobe, i );
		pnSkills[ i ] = ItemGetPrimarySkill( pWeapons[ i ] );
		bSelectionInRange[ i ] = FALSE;
	}
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( ! pWeapons[ i ] )
		{ // just in case we only have one weapon...
			for ( int j = 0; j < MAX_WEAPONS_PER_UNIT; j++ )
			{
				if ( ! pWeapons[ j ] )
					continue;
				pWeapons[ i ] = pWeapons[ j ];
				pnSkills[ i ] = pnSkills[ j ];
			}
		}
	}
	pnSkills[ MAX_WEAPONS_PER_UNIT + 0 ] = UnitGetStat( unit, STATS_SKILL_LEFT );
	pnSkills[ MAX_WEAPONS_PER_UNIT + 1 ] = UnitGetStat( unit, STATS_SKILL_RIGHT );
	for ( int i = MAX_WEAPONS_PER_UNIT; i < MAX_WEAPONS_PER_UNIT + 2; i++ )
	{
		pWeapons[ i ] = NULL;
		bSelectionInRange[ i ] = FALSE;

		const SKILL_DATA * pSkillData = SkillGetData( pGame, pnSkills[ i ] );
		if ( ! pSkillData )
			continue;
		// see if we need to look at a fallback skill
		if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_WEAPON_IS_REQUIRED ) )
			continue;
		UNIT * pWeaponsForSkill[ MAX_WEAPONS_PER_UNIT ];
		UnitGetWeapons( unit, pnSkills[ i ], pWeaponsForSkill, FALSE );
		pWeapons[ i ] = pWeaponsForSkill[ 0 ];

		if ( pWeapons[ i ] )
			continue;

		for ( int j = 0; j < NUM_FALLBACK_SKILLS; j++ )
		{
			int nFallbackSkill = pSkillData->pnFallbackSkills[ j ];
			const SKILL_DATA * pSkillDataFallback = nFallbackSkill != INVALID_ID ? SkillGetData( pGame, nFallbackSkill ) : NULL;
			if ( !pSkillDataFallback )
				continue;
			if ( SkillDataTestFlag( pSkillDataFallback, SKILL_FLAG_WEAPON_IS_REQUIRED ) )
			{
				UNIT * pWeaponsForFallbackSkill[ MAX_WEAPONS_PER_UNIT ];
				UnitGetWeapons( unit, nFallbackSkill, pWeaponsForFallbackSkill, FALSE );
				if ( pWeaponsForFallbackSkill[ 0 ] )
				{
					pWeapons[ i ] = pWeaponsForFallbackSkill[ 0 ];
					pnSkills[ i ] = nFallbackSkill;
					break;
				}
			} else {
				pnSkills[ i ] = nFallbackSkill;
				break;
			}
		}
	}

	UNIT * selection = NULL;
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT + 2; i++ )
	{
		if ( pnSkills[ i ] == INVALID_ID )
			continue;

		UNIT * pTarget = NULL;

		const SKILL_DATA * pSkillData = SkillGetData( pGame, pnSkills[ i ] );
		ASSERT_CONTINUE(pSkillData);
		float fSkillRange = SkillGetRange( unit, pnSkills[ i ], SkillDataTestFlag( pSkillData, SKILL_FLAG_WEAPON_IS_REQUIRED ) ? NULL : pWeapons[ i ] );

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USE_MOUSE_SKILLS_TARGETING ) && pWeapons[i] )
		{ // focus items allow you to use spells... so they don't really have a good range or targeting on their own
			int nMouseSkill = UnitGetStat( unit, STATS_SKILL_RIGHT );
			for ( int j = 0; j < 2; j++ )
			{
				const SKILL_DATA * pMouseSkillData = SkillGetData( pGame, nMouseSkill );
				if ( pMouseSkillData->nRequiredWeaponUnittype != INVALID_ID && 
					UnitIsA( UnitGetType(pWeapons[ i ]), pMouseSkillData->nRequiredWeaponUnittype ) )
				{
					float fMouseSkillRange = SkillGetRange( unit, nMouseSkill );
					if ( fMouseSkillRange > fSkillRange )
					{
						fSkillRange = fMouseSkillRange;
						pSkillData = pMouseSkillData;
					}
				}
				nMouseSkill = UnitGetStat( unit, STATS_SKILL_LEFT );
			}
		} 

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_UI_USES_TARGET ) )
		{
			int nWeaponIndex = WardrobeGetWeaponIndex( unit, UnitGetId( pWeapons[ i ] ) );
			pTarget = c_WeaponFindTargetForControlUnit( pGame, nWeaponIndex, pnSkills[ i ] );
			if ( pTarget )
			{
				bSelectionInRange[i] = UnitsAreInRange( unit, pTarget, pSkillData->fRangeMin, fSkillRange, NULL );
				if ( i >= MAX_WEAPONS_PER_UNIT )
					selection = pTarget;
			}
		}

	}

	// if you have no weapons, you still want to try and target things
	if ( ! selection )
	{
		if ( UnitIsInTown( unit ) )
		{
			int nTargetSelectorTown = GlobalIndexGet( GI_SKILLS_PLAYER_TARGET_SELECTOR_TOWN );
			selection = c_WeaponFindTargetForControlUnit( pGame, INVALID_ID, nTargetSelectorTown );
			if ( selection )
			{
				float fRange = UNIT_INTERACT_CURSOR_DISTANCE;
				if ( ! UnitsAreInRange( unit, selection, 0.0f, fRange, NULL ) )
					selection = NULL;
			}
		}

		if ( ! selection )
			selection = c_WeaponFindTargetForControlUnit( pGame, INVALID_ID, GlobalIndexGet( GI_SKILLS_PLAYER_TARGET_SELECTOR ) );
	}

	if ( UnitHasState( pGame, unit, STATE_CANNOT_TARGET ) )
		selection = NULL;

	UNITID idTargetOld = UIGetTargetUnitId();

	if ( selection && 
		 !UnitIsA( selection, UNITTYPE_NOTARGET ) && 
		 !UnitHasState( pGame, selection, STATE_NOTARGET ) && 
		 !(PetIsPlayerPet( pGame, selection ) && TestFriendly( pGame, unit, selection ) ) && 
		 ( !(UnitIsPlayer( selection ) && TestFriendly( pGame, unit, selection ) ) || UnitIsInTown( selection ) ) )
	{
		float fUnitDistSquared = UnitsGetDistanceSquared( unit, selection );

		if ( UnitCanInteractWith( selection, unit ) )
		{
			if (UnitIsA(selection, UNITTYPE_OPENABLE) ||
				UnitIsA(selection, UNITTYPE_PASSAGE))
			{
				if ( fUnitDistSquared <= ( UNIT_INTERACT_CURSOR_DISTANCE_SQUARED ) )
				{
					UISetTargetUnit(UnitGetId(selection), bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 0 ] || bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 1 ]);
					// setting the target may fail if it can't be selected at this time,
					// in that case draw a red hand
					if ( UIGetTargetUnit() && fUnitDistSquared <= ( UNIT_INTERACT_DISTANCE_SQUARED ) )
						UITargetChangeCursor( selection, TCUR_HAND, GFXCOLOR_GREEN, FALSE );
					else
						UITargetChangeCursor( selection, TCUR_HAND, GFXCOLOR_RED, FALSE );
				}
				else
				{
					UISetTargetUnit( INVALID_ID, FALSE );
					UITargetChangeCursor( NULL, TCUR_DEFAULT, FALSE, FALSE );
				}
			}
			else
			{
				BOOL bForceOff = FALSE;
				if ( c_NPCGetInfoState( selection ) == INTERACT_INFO_NONE && !sUnitHasInteraction( selection ))
				{
					bForceOff = TRUE;
				}
				if ( bForceOff && ( UnitIsPlayer( selection ) && UnitIsInTown( selection ) ) )
				{
					bForceOff = FALSE;
				}
				if ( bForceOff && UnitIsTaskGiver( selection ) )
				{
					bForceOff = FALSE;
				}
				const UNIT_DATA * pSelectionUnitData = UnitGetData( selection );
				if ( bForceOff && ( UnitGetGenus( selection ) == GENUS_OBJECT ) )
				{
					bForceOff = ObjectCanOperate( unit, selection ) != OPERATE_RESULT_OK;
				}
				else if ( bForceOff && 
					pSelectionUnitData->nSkillWhenUsed != INVALID_ID && 
					SkillCanStart( pGame, unit, pSelectionUnitData->nSkillWhenUsed, INVALID_ID, selection, FALSE, FALSE, 0 ) &&
					SkillIsValidTarget( pGame, unit, selection, NULL, pSelectionUnitData->nSkillWhenUsed, TRUE ) )
				{
					bForceOff = FALSE;
				}
				UISetTargetUnit(UnitGetId(selection), bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 0 ] || bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 1 ]);
				if ( !bForceOff && ( fUnitDistSquared <= ( UNIT_INTERACT_CURSOR_DISTANCE_SQUARED ) ) )
				{
					if ( fUnitDistSquared <= ( UNIT_INTERACT_DISTANCE_SQUARED ) )
						UITargetChangeCursor( selection, TCUR_TALK, GFXCOLOR_GREEN, FALSE );
					else
						UITargetChangeCursor( selection, TCUR_TALK, GFXCOLOR_RED, FALSE );
				}
				else
				{
					UISetTargetUnit(INVALID_ID, FALSE);
					UITargetChangeCursor( NULL, TCUR_DEFAULT, FALSE, FALSE );
				}
			}
		}
		else if ( TestFriendly( pGame, unit, selection ) )
		{
			UISetTargetUnit(INVALID_ID, FALSE);
			UITargetChangeCursor( NULL, TCUR_DEFAULT, FALSE, FALSE );
		}
		else
		{
/*			if ( !c_TutorialInputOk( unit, TUTORIAL_INPUT_CROSSHAIRS ) )
			{
				UISetTargetUnit(INVALID_ID, FALSE);
				UITargetChangeCursor( NULL, TCUR_NONE, FALSE, FALSE );
			}
			else*/
			{
				if (fUnitDistSquared < MONSTER_TARGET_MAXIMUM_DISTANCE_SQUARED)	// don't indicate them if they're out of range
				{
					UISetTargetUnit(UnitGetId(selection), bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 0 ] || bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 1 ] );
				}
				else
				{
					UISetTargetUnit(INVALID_ID, FALSE);
				}

				// go ahead and show the target cursor at any range
				UITargetChangeCursor( selection, 
					TCUR_LOCK, 
					(bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 0 ] ? GFXCOLOR_HALFGREEN : GFXCOLOR_HALFRED), 
					TRUE,
					(bSelectionInRange[ MAX_WEAPONS_PER_UNIT + 1 ] ? GFXCOLOR_HALFGREEN : GFXCOLOR_HALFRED) ); 
			}
		}

		if ( idTargetOld != UIGetTargetUnitId() )
		{
			c_SkillControlUnitChangeTarget( UIGetTargetUnit() );
		}
	}
	else
	{
		UISetTargetUnit(INVALID_ID, FALSE);
		UITargetChangeCursor( NULL, TCUR_DEFAULT, GFXCOLOR_WHITE, FALSE );

		c_SkillControlUnitChangeTarget( selection );
	}

	if ( idTargetOld != UIGetTargetUnitId() )
	{
		c_SkillControlUnitChangeTarget( UIGetTargetUnit() );
	}

	// help things like the harp update more often
	UnitTriggerEvent( pGame, EVENT_AIMUPDATE, unit, NULL, NULL );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_UISetSelectionTugboat( GAME * pGame, 
					   UNIT* selected_unit )
{
	if( selected_unit != UIGetTargetUnit() )
	{
		if( UIGetTargetUnit() )
		{
			// turn off fullbright if we already had a unit selected
			c_AppearanceSetFullbright( c_UnitGetModelIdThirdPerson( UIGetTargetUnit() ),FALSE );
		}
		if( selected_unit )
		{
			c_AppearanceSetFullbright( c_UnitGetModelIdThirdPerson( selected_unit ),TRUE );
		}
	}
	if ( selected_unit )
	{
		UISetTargetUnit(UnitGetId(selected_unit), TRUE);
		UITargetChangeCursor( selected_unit, TCUR_LOCK, GFXCOLOR_GREEN, FALSE );
	}
	else
	{
		UISetTargetUnit(INVALID_ID, FALSE);
		UITargetChangeCursor( NULL, TCUR_DEFAULT, FALSE, FALSE );
	}
	if( AppIsTugboat() )
	{
		UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_PANEL);
		UIComponentHandleUIMessage(pComp, UIMSG_PAINT, 0, 0);
		//UIUpdatePlayerInteractPanel( selected_unit );
	}

}

static void c_sUIUpdateSelectionTugboat( GAME * pGame )
{

#define GFXCOLOR_HALFGREEN	0x7f00ff00
#define GFXCOLOR_HALFRED	0x7fff0000

	if (!pGame)
		return;

	ASSERT( IS_CLIENT( pGame ) );


	UNIT * unit = GameGetControlUnit(pGame);
	if (!unit)
	{
		return;
	}
	if (!UnitGetRoom(unit))
	{
		return;
	}

	UI_COMPONENT *pLightHud = UIComponentGetByEnum(UICOMP_LIGHT_HUD);
	UI_COMPONENT *pShadowHud = UIComponentGetByEnum(UICOMP_SHADOW_HUD);

	if( PlayerIsInPVPWorld( unit ) )
	{
		if( UIComponentGetVisible( pLightHud ) )
		{
			UIComponentSetActive( pShadowHud, TRUE );
			UIComponentSetVisible( pShadowHud, TRUE );
			UIComponentSetActive( pLightHud, FALSE );
			UIComponentSetVisible( pLightHud, FALSE );
		}
	}
	else
	{
		if( UIComponentGetVisible( pShadowHud ) )
		{
			UIComponentSetActive( pShadowHud, FALSE );
			UIComponentSetVisible( pShadowHud, FALSE );
			UIComponentSetActive( pLightHud, TRUE );
			UIComponentSetVisible( pLightHud, TRUE );
		}

	}

	UI_TB_LookForBadItems();
	if( e_GetUICovered() ||
		UI_TB_MouseOverPanel() ||
		c_CameraGetViewType() == VIEW_VANITY )
	{
		UISetTargetUnit( INVALID_ID, TRUE );
		UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, INVALID_ID );
		return;
	}
	UNIT* selected_unit = NULL;
	UNIT* mouse_selected_unit = UIGetTargetUnit();

	BOOL bLeft = UIGetClickedTargetLeft();
	if( UIGetTargetUnit() && UIPlayerInteractMenuOpen() )
	{
		selected_unit = UIGetTargetUnit();
	}
	else if( UIGetClickedTargetUnit() && 
		( ( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) && bLeft ) ||
		  ( InputGetMouseButtonDown( MOUSE_BUTTON_RIGHT ) && !bLeft ) ) )
	{
		selected_unit = UIGetClickedTargetUnit();
	}
	else if( CameraGetUpdated() )
	{
		selected_unit = c_UIGetMouseUnit( pGame );
	}

	// if we've got something selected from another level, blank it.
	if( selected_unit &&
		UnitGetLevel( selected_unit ) != UnitGetLevel( GameGetControlUnit( pGame ) ) )
	{
		UISetTargetUnit( INVALID_ID, TRUE );
		UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, INVALID_ID );
		selected_unit = NULL;
	}
	// if we've got something selected from another level, blank it.
	if( mouse_selected_unit &&
		UnitGetLevel( mouse_selected_unit ) != UnitGetLevel( GameGetControlUnit( pGame ) ) )
	{
		UISetTargetUnit( INVALID_ID, TRUE );
		mouse_selected_unit = NULL;
	}

	// update item alt selection list
	UNIT* alt_unit = UIGetAltTargetUnit();

	if( alt_unit )
	{
		selected_unit = alt_unit;
	}

	c_UISetSelectionTugboat( pGame, selected_unit );


	// help things like the harp update more often
	// TRAVIS: don't need anymore?
	//UnitTriggerEvent( pGame, EVENT_AIMUPDATE, unit, NULL, NULL );

	// update item "selection"
	//UIUpdateClosestItem();

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_UIUpdateSelection( GAME * pGame )
{
	if(AppIsHellgate())
	{
		c_sUIUpdateSelectionHellgate(pGame);
	}
	else if(AppIsTugboat())
	{
		c_sUIUpdateSelectionTugboat(pGame);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_UI_RTS_Selection( GAME * pGame, VECTOR * pvSelectedLocation, UNIT ** ppSelectedUnit )
{
	ASSERT( IS_CLIENT( pGame ) );

	UNIT * pPlayer = GameGetControlUnit( pGame );
	if ( !pPlayer )
		return;
	
	if ( c_CameraGetViewType() != VIEW_RTS )
		return;

	LEVEL * pLevel = UnitGetLevel( pPlayer );
	if ( !pLevel )
		return;

	VECTOR vOrigin, vDirection;
	void MouseToScene( GAME * pGame, VECTOR *pvOrig, VECTOR *pvDir );
	MouseToScene( pGame, &vOrigin, &vDirection );
	VectorNormalize( vDirection, vDirection );

	// send ray into scene
	VECTOR vNormal;
	float fRayLength = 500.0f;
	float fDistance = LevelLineCollideLen( pGame, pLevel, vOrigin, vDirection, fRayLength, &vNormal );
	if ( fDistance >= fRayLength )
		return;

	if ( fDistance < 2.0f )
		return;

	VECTOR vEndPoint;
	vEndPoint.fX = ( vDirection.fX * fDistance ) + vOrigin.fX;
	vEndPoint.fY = ( vDirection.fY * fDistance ) + vOrigin.fY;
	vEndPoint.fZ = ( vDirection.fZ * fDistance ) + vOrigin.fZ;

	if ( pvSelectedLocation && ( vNormal.fZ >= CollisionWalkableZNormal() ) )
	{
		*pvSelectedLocation = vEndPoint;
	}

	if ( ppSelectedUnit )
	{
		*ppSelectedUnit = NULL;

		// used in point line calcs
		VECTOR vEdge = vEndPoint - vOrigin;
		float fLengthSq = VectorLengthSquared( vEdge );
		float fClosestSq = fLengthSq;

		TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_OBJECT };
		int nTargetTypescount = arrsize(eTargetTypes);

		// this needs to be optimized to only search through rooms that matter
		for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
		{
			for(int i=0; i<nTargetTypescount; i++)
			{
				for ( UNIT * test = room->tUnitList.ppFirst[eTargetTypes[i]]; test; test = test->tRoomNode.pNext )
				{
					if( UnitIsA( test, UNITTYPE_NOTARGET ) || UnitHasState( pGame, test, STATE_NOTARGET ) || !UnitGetCanTarget( test ) )
					{
						continue;
					}

					if ( UnitGetGenus( test ) == GENUS_OBJECT )
					{
						if ( !UnitIsInteractive( test ) )
							continue;
					}

					// is it the closest unit we are dealing with?
					VECTOR vPos = test->vPosition;
					vPos.fZ += 0.5f;
					float dist_sq = VectorDistanceSquared( vEndPoint, vPos );
					if ( dist_sq > fClosestSq )
					{
						continue;
					}

					BOUNDING_BOX box;
					// first grab the unit's model bounding box
					int nModelId = c_UnitGetModelId( test );
					if ( nModelId == INVALID_ID )
					{
						continue;
					}

					e_GetModelRenderAABBInWorld( nModelId, &box );
					if(VectorIsZeroEpsilon(BoundingBoxGetSize(&box)))
					{
						UnitMakeBoundingBox(test, &box);
					}

					// find center point
					VECTOR vCenter = box.vMax + box.vMin;
					VectorScale( vCenter, vCenter, 0.5f );

					// find closest point from our selection ray to this unit's center of their bounding box

					// I have no idea where the bug is in this code. I have looked at it for 2 days
					// and can't figure it out, so I put back the unoptimized vector-point distance
					// routine and will figure this out later - drb
					float fU = ( ( ( vCenter.fX - vOrigin.fX ) * vEdge.fX ) +
						( ( vCenter.fY - vOrigin.fY ) * vEdge.fY ) +
						( ( vCenter.fZ - vOrigin.fZ ) * vEdge.fZ ) ) /
						fLengthSq;

					// Check if point falls within the line segment
					if ( fU < 0.0f || fU > 1.0f )
						continue;

					VECTOR vClosest;
					vClosest.fX = vOrigin.fX + ( fU * vEdge.fX );
					vClosest.fY = vOrigin.fY + ( fU * vEdge.fY );
					vClosest.fZ = vOrigin.fZ + ( fU * vEdge.fZ );

					// check if that point is within the bounding box
					if ( !BoundingBoxTestPoint( &box, &vClosest ) )
					{
						continue;
					}

					*ppSelectedUnit = test;
					fClosestSq = dist_sq;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sCompareUnitDisplays( const void * e1, const void * e2 )
{
	UI_UNIT_DISPLAY * p1 = (UI_UNIT_DISPLAY *)e1;
	UI_UNIT_DISPLAY * p2 = (UI_UNIT_DISPLAY *)e2;
	if ( p1->fCameraDistSq > p2->fCameraDistSq )
		return -1; // sort farther distance earlier
	if ( p1->fCameraDistSq < p2->fCameraDistSq )
		return 1; // sort closer distance later
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sDoCollisionHeightTool( 
	GAME *pGame,
	UI_UNIT_DISPLAY *pUnitDisplays,
	int nNumLabels)
{
	// who to display on
	UNIT *pUnit = NULL;
	UNITID idTarget = UIGetTargetUnitId();
	if (idTarget != INVALID_ID)
	{
		pUnit = UnitGetById( pGame, idTarget );
	}
	if (pUnit == NULL)
	{
		pUnit = GameGetControlUnit( pGame );
	}
	
	const CAMERA_INFO *pCameraInfo = c_CameraGetInfo( FALSE );
	if (pCameraInfo)
	{

		VECTOR vWorldPos = UnitGetDrawnPosition( pUnit );
		
		// transform world pos into screen space
		MATRIX vMat;
		VECTOR4 vTransformed;
		V( e_GetWorldViewProjMatrix( &vMat ) );
	
		// build each label
		VECTOR vWorldPosLabel = vWorldPos;
		float flZLastLabel = vWorldPosLabel.fZ + 5.0f;
		float flTotalIncrement = 0.0f;
		const float flIncrement = 0.1f;
		while (vWorldPosLabel.fZ < flZLastLabel)
		{
			UI_UNIT_DISPLAY *pDisplay = &pUnitDisplays[ nNumLabels ];
			
			pDisplay->nUnitID = UnitGetId( pUnit );
			VECTOR vEye = CameraInfoGetPosition( pCameraInfo );
			pDisplay->fCameraDistSq = VectorDistanceSquared( vEye, vWorldPos );
			ASSERT( pDisplay->fCameraDistSq > 0.f );

			MatrixMultiply( &vTransformed, &vWorldPosLabel, &vMat );
			pDisplay->vScreenPos.fX = vTransformed.fX / vTransformed.fW;
			pDisplay->vScreenPos.fY = vTransformed.fY / vTransformed.fW;
			pDisplay->vScreenPos.fZ = vTransformed.fZ / vTransformed.fW;
			if (flTotalIncrement == 0.0)
			{			
				// display name at the feet
				UnitGetName( pUnit, pDisplay->uszDisplay, arrsize(pDisplay->uszDisplay), 0 );			
			}
			else
			{
				PStrPrintf( pDisplay->uszDisplay, arrsize(pDisplay->uszDisplay), L"%.1f", flTotalIncrement );
			}
			nNumLabels++;
			
			// go to next step
			vWorldPosLabel.fZ += flIncrement;
			flTotalIncrement += flIncrement;
			
		}

	}
	
	return nNumLabels;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FONTCOLOR c_UIGetOverheadNameColor( GAME *pGame, UNIT *pUnit, UNIT *pPlayer )
{
	ASSERT_RETVAL( pGame, FONTCOLOR_NAME_NPC );
	ASSERT_RETVAL( pUnit, FONTCOLOR_NAME_NPC );
	ASSERT_RETVAL( pPlayer, FONTCOLOR_NAME_NPC );

	if ( UnitIsA( pUnit, UNITTYPE_PLAYER ) )
	{
		WCHAR szGuildName[ MAX_UNIT_DISPLAY_STRING ] = L"";
		GUILD_RANK eGuildRank = GUILD_RANK_INVALID;
		UnitGetPlayerGuildAssociationTag(pUnit, szGuildName, arrsize(szGuildName), eGuildRank);
		
		if (PlayerIsHardcoreDead(pUnit))
		{
			return FONTCOLOR_NAME_HARDCORE_DEAD;
		}
		else if (UnitsGetTeamRelation(pGame, pPlayer, pUnit) == RELATION_HOSTILE)
		{
			return FONTCOLOR_NAME_PVP;
		}
		else if (UnitGetGuid( pUnit ) != INVALID_GUID && 
				 c_PlayerFindPartyMember(UnitGetGuid( pUnit ) ) != INVALID_INDEX)
		{
			return FONTCOLOR_NAME_PARTY;
		}
		else if (szGuildName[0] && !PStrCmp(szGuildName, c_PlayerGetGuildName(), arrsize(szGuildName)))
		{
			return FONTCOLOR_NAME_GUILD;
		}
		else if (PlayerIsHardcore(pUnit))
		{
			return FONTCOLOR_NAME_HARDCORE;
		}
		else if (PlayerIsElite(pUnit))
		{
			return FONTCOLOR_NAME_ELITE;
		}
		else
		{
			return FONTCOLOR_NAME_PC;
		}
	}

	return FONTCOLOR_NAME_NPC;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UIUpdateNames( GAME * pGame )
{
	#define UI_NAME_MAX_DISTANCE			40.f
	#define UI_NAME_MAX_UNITS				MAX_TARGETS_PER_QUERY // it's important that MAX_UNITS doesn't exceed this!
	#define UI_NAME_FOV_WIDEN				1.25f

	// finds visible units, sorts depth first, finds display points, and passes to UI

	if ( !pGame )
		return;

	UNITID pnUnitIDs[ UI_NAME_MAX_UNITS ];
	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
	if (!pCameraInfo)
	{
		return;
	}
	UNIT *pControlUnit = GameGetControlUnit( pGame );
	if(!pControlUnit)
	{
		return;
	}
	
	VECTOR vEye = CameraInfoGetPosition( pCameraInfo );
	VECTOR vLocation = AppIsHellgate() ? vEye : pControlUnit->vPosition;

	// get a generous cos FOV assuming a square aspect in largest dimension
	float fCos = AppIsHellgate() ? c_CameraGetCosFOV( UI_NAME_FOV_WIDEN ) : 0.0f;

	SKILL_TARGETING_QUERY tQuery;
	tQuery.fMaxRange			= UI_NAME_MAX_DISTANCE;
	tQuery.nUnitIdMax			= UI_NAME_MAX_UNITS;
	tQuery.pnUnitIds			= pnUnitIDs;
	tQuery.pSeekerUnit			= pControlUnit;
	tQuery.pvLocation			= &vEye;
	tQuery.fDirectionTolerance	= fCos;
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT);	
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM);		
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION);
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND);
	
	// turning this off for tugboat so we dont get so much item label drop out on high density maps
	if(AppIsHellgate())
	{
		SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_TARGET_DEAD_OR_DYING);
	}

	BOOL bDisplayDebugLabels = UnitDebugTestLabelFlags();
	if (bDisplayDebugLabels)
	{
		tQuery.fMaxRange *= 5.0f;
		SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND);
		SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY);
		SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_SEEKER);
	}
	
	BOOL bShowItems = UIGetShowItemNamesState();
	//if (bShowItems)
	//{
		//tQuery.tFilter.nUnitType = UNITTYPE_ITEM;
	//}
	//else
	{
		// only look for friendlies with a line of sight to the camera
		SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND);
		SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY);
		SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_UNTARGETABLES);

		// what units should have displayed names?
		tQuery.tFilter.nUnitType = UNITTYPE_ANY;
		
	}
	SkillTargetQuery( pGame, tQuery );

	// if can't display anything, or nothing found to display, return
	if ( tQuery.nUnitIdCount <= 0 || UnitHasState( pGame, pControlUnit, STATE_HIDE_UNIT_NAMES ) ||
		( AppIsTugboat() &&
		  c_CameraGetViewType() == VIEW_VANITY ) ) 
	{
		// But first tell the UI to clear
		UIUnitLabelsAdd( NULL, 0, 0.0f );
		return;
	}

	// setup where to store the labels
	UI_UNIT_DISPLAY tUnitDisplays[ MAX_UNIT_DISPLAYS ];

	// build the name structures and then sort them
	int nNumUnitDisplays = 0;
	BOOL bTooManyLables = FALSE;
	for ( int i = 0; i < tQuery.nUnitIdCount; i++ )
	{
		UNITID idUnit = pnUnitIDs[ i ];
		UNIT *pUnit = UnitGetById( pGame, idUnit );
		ASSERT_CONTINUE( pUnit );
			
		// filter results
		BOOL bAddLabel = FALSE;
		if (bDisplayDebugLabels == TRUE)
		{
			bAddLabel = TRUE;
		}
		else
		{
			if ( AppIsHellgate() && UnitHasState( pGame, pUnit, STATE_QUEST_HIDDEN ) )
			{
				continue;
			}

			// some states hide names
			if (UnitHasState( pGame, pUnit, STATE_HIDE_NAME ))
			{
				continue;
			}

			if (UnitGetStat( pUnit, STATS_NO_DRAW ) != 0)
			{
				continue;
			}
			
			if (IsUnitDeadOrDying(pUnit) && !UnitTestFlag( pUnit, UNITFLAG_SHOW_LABEL_WHEN_DEAD ))
			{
				continue;
			}

			// players
			if ( UnitIsA( pUnit, UNITTYPE_PLAYER ) )
			{
				bAddLabel = TRUE;
			}

			// objects
			if (AppIsHellgate() && UnitIsA( pUnit, UNITTYPE_OBJECT ))
			{
				bAddLabel = FALSE;  // don't add objects by default
				const UNIT_DATA *pObjectData = ObjectGetData( pGame, pUnit );
				if (pObjectData && UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_ALWAYS_SHOW_LABEL))
				{
					bAddLabel = TRUE;  // unless it's a warp
				}
			}
						
			// monsters
			if (UnitIsA( pUnit, UNITTYPE_MONSTER ))
			{

//				if (IsUnitDeadOrDying( pUnit ) == FALSE)
				{
					
					if (UnitAlwaysShowsLabel( pUnit ) == TRUE)
					{
						bAddLabel = TRUE;  // show friendlies
					}
					else if (UnitIsA(pUnit, UNITTYPE_TURRET))
					{
						bAddLabel = FALSE;
					}
					else if (AppIsHellgate() && 
							 MonsterUniqueNameGet( pUnit ) != INVALID_LINK ||
							 MonsterQualityShowsLabel( MonsterGetQuality( pUnit ) ))
					{
						bAddLabel = TRUE;  // show uniques
					}					
					else if (PetIsPet( pUnit ))
					{
						UNITID idOwner = PetGetOwner( pUnit );
						if(idOwner != INVALID_ID)
						{
							UNIT * pOwner = UnitGetById( pGame, idOwner );
							if(pOwner && UnitIsA( pUnit, UNITTYPE_PLAYER ))
							{
								bAddLabel = TRUE;  // show pets
							}
						}
					}

				}
								
			}
			
			// items
			if ( UnitIsA( pUnit, UNITTYPE_ITEM ) && !UnitIsA(pUnit, UNITTYPE_TURRET) )
			{
				bAddLabel = FALSE;
				if (AppIsHellgate() && bShowItems && ItemCanPickup(pControlUnit, pUnit, ICP_DONT_CHECK_CONDITIONS_MASK ) == PR_OK)
				{
					bAddLabel = TRUE;
				}
				else if( AppIsTugboat() && ( bShowItems || pUnit == UIGetTargetUnit() ) && ItemCanPickup(pControlUnit, pUnit) == PR_OK )
				{
					bAddLabel = TRUE;
				}
			}
			
			// objects
			if (UnitIsA( pUnit, UNITTYPE_OBJECT ))
			{
			
				if (UnitIsA( pUnit, UNITTYPE_HEADSTONE ))
				{
					if (UnitGetStat( pUnit, STATS_NEWEST_HEADSTONE_FLAG ) == TRUE &&
						UnitHasState( pGame, pUnit, STATE_DONT_DRAW_QUICK ) == FALSE)
					{
						bAddLabel = TRUE;
					}
				}
				
			}

			// mythos target units			
			if( AppIsTugboat() && pUnit == UIGetTargetUnit() &&
				( UnitIsA( pUnit, UNITTYPE_OBJECT ) ||
				   UnitIsA( pUnit, UNITTYPE_DESTRUCTIBLE ) ||
				   UnitTestFlag( pUnit, UNITFLAG_CANBEPICKEDUP ) ) && ItemCanPickup(pControlUnit, pUnit) == PR_OK)
			{
				bAddLabel = TRUE;
			}
		}		

		if ( bAddLabel && AppIsHellgate() )
		{
			VECTOR vDirection = UnitGetAimPoint( pUnit ) - vEye;
			float fDistance = VectorNormalize( vDirection );

			fDistance = PIN( fDistance, 0.0f, tQuery.fMaxRange );

			if ( fDistance )
			{
				LEVEL* pLevel = UnitGetLevel( pUnit );

				float fDistToCollide = LevelLineCollideLen( pGame, pLevel, vEye, vDirection, fDistance );
				if ( fDistToCollide < fDistance )
					bAddLabel = FALSE;
			}

		}

		// add label if we have room
		if (bAddLabel == TRUE)
		{
		
			if (nNumUnitDisplays < MAX_UNIT_DISPLAYS - 1)
			{
			
				// get display and increment
				UI_UNIT_DISPLAY *pDisplay = &tUnitDisplays[ nNumUnitDisplays++ ];
				memclear(pDisplay, sizeof(UI_UNIT_DISPLAY));
				
				// save the id
				pDisplay->nUnitID = pnUnitIDs[ i ];

				DWORD dwFlags = 0;
				SETBIT( dwFlags, UNF_INCLUDE_TITLE_BIT );									
				if ( UnitIsA( pUnit, UNITTYPE_PLAYER ) )
				{
					WCHAR szTemp[ MAX_UNIT_DISPLAY_STRING ];
					UnitGetName( pUnit, szTemp, arrsize(szTemp), dwFlags );

					pDisplay->uszDisplay[0] = L'\0';

					FONTCOLOR nameColor = c_UIGetOverheadNameColor(pGame, pUnit, pControlUnit);

					if (UnitsGetTeamRelation(pGame, pControlUnit, pUnit) == RELATION_HOSTILE)
					{
						UIAddColorToString(pDisplay->uszDisplay, nameColor, MAX_UNIT_DISPLAY_STRING);
						PStrCat(pDisplay->uszDisplay, szTemp, MAX_UNIT_DISPLAY_STRING);
						if( AppIsHellgate() )
						{
							const WCHAR *teamName = TeamGetName(pGame, UnitGetTeam(pUnit));
							if (teamName && teamName[0])
							{
								PStrCat(pDisplay->uszDisplay, L" [", MAX_UNIT_DISPLAY_STRING);
								PStrCat(pDisplay->uszDisplay, teamName, MAX_UNIT_DISPLAY_STRING);
								PStrCat(pDisplay->uszDisplay, L"]", MAX_UNIT_DISPLAY_STRING);
							}
						}
					}
					else 
					{
						UIColorCatString(pDisplay->uszDisplay, MAX_UNIT_DISPLAY_STRING, nameColor, szTemp);
					}
				}
				else
				{
					SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );
					SETBIT( dwFlags, UNF_VERBOSE_WARPS_BIT );
					SETBIT( dwFlags, UNF_VERBOSE_HEADSTONES_BIT );
					UnitGetName( pUnit, pDisplay->uszDisplay, arrsize(pDisplay->uszDisplay), dwFlags );

					// KCK: Append subscriber only string to subscriber only NPCs
					const UNIT_DATA * pUnitData = UnitGetData( pUnit );
					if ( UnitIsNPC(pUnit) && UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY))
					{
						UNIT * pPlayer = GameGetControlUnit(pGame);
						if (!pPlayer || !PlayerIsSubscriber(pPlayer))
							UIAddColorToString(pDisplay->uszDisplay, FONTCOLOR_AUTOMAP_WARP_CLOSED, MAX_UNIT_DISPLAY_STRING);
						WCHAR temp[MAX_UNIT_DISPLAY_STRING];
						PStrPrintf(temp, MAX_UNIT_DISPLAY_STRING, L"\n(%s)", GlobalStringGet( GS_SUBSCRIBER_ONLY ));
						PStrCat(pDisplay->uszDisplay, temp, MAX_UNIT_DISPLAY_STRING);
					}
				}

				static const float fLabelHeight = 2.00f;  // in meters
				UnitDisplayGetOverHeadPositions( pUnit, pControlUnit, vEye, fLabelHeight, pDisplay->vScreenPos, pDisplay->fLabelScale, pDisplay->fPlayerDistSq, pDisplay->fCameraDistSq);

				//WCHAR uszTemp[MAX_UNIT_DISPLAY_STRING];
				//PStrPrintf(uszTemp, MAX_UNIT_DISPLAY_STRING, L" %0.2f/%0.2f", fTransformedHeight, fLabelHeight);
				//PStrCat(pDisplay->uszDisplay, uszTemp, MAX_UNIT_DISPLAY_STRING);
			}
			else
			{
				bTooManyLables = TRUE;
			}
					
		}
		
	}

	// sort the names depth first
	qsort( tUnitDisplays, nNumUnitDisplays, sizeof(UI_UNIT_DISPLAY), sCompareUnitDisplays );

	// pass the names to UI
	UIUnitLabelsAdd( tUnitDisplays, nNumUnitDisplays, tQuery.fMaxRange );

	// if we had too many lables to display, print error
	if (bTooManyLables == TRUE)
	{
		static DWORD dwLastMessageTime = 0;
		DWORD dwNow = GameGetTick( pGame );
		
		const DWORD dwMessageDelayTime = GAME_TICKS_PER_SECOND * 10;
		if (dwNow - dwLastMessageTime >= dwMessageDelayTime)
		{
			ConsoleString( CONSOLE_ERROR_COLOR, "Too many unit ui lables to display" );
			dwLastMessageTime = dwNow;
		}
				
	}

	// these will replace the existing names, but that's ok
	if (gbDisplayCollisionHeightLadder)
	{
		nNumUnitDisplays = sDoCollisionHeightTool( pGame, tUnitDisplays, 0 );
		
		// sort the names depth first
		qsort( tUnitDisplays, nNumUnitDisplays, sizeof(UI_UNIT_DISPLAY), sCompareUnitDisplays );

		// pass the names to UI
		UIUnitLabelsAdd( tUnitDisplays, nNumUnitDisplays, tQuery.fMaxRange );
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UIUpdateDebugLabels()
{
	UIDebugLabelsClear();

	RoomDisplayDebugOverview();

	int nCount;
	const UI_DEBUG_LABEL * pList = e_UIGetDebugLabels( nCount );

	// if no labels, return
	if ( nCount <= 0 )
	{
		return;
	}
	ASSERT_RETURN( pList );

	//// sort the names depth first
	//qsort( tUnitDisplays, nNumUnitDisplays, sizeof(UI_UNIT_DISPLAY), sCompareUnitDisplays );

	for ( int i = 0; i < nCount; i++ )
	{
		if ( pList[ i ].bWorldSpace )
			UIDebugLabelAddWorld(  pList[ i ].wszLabel, &pList[ i ].vPos, pList[ i ].fScale, pList[ i ].dwColor, pList[ i ].eAnchor, pList[ i ].eAlign, pList[ i ].bPinToEdge );
		else
			UIDebugLabelAddScreen( pList[ i ].wszLabel, &pList[ i ].vPos, pList[ i ].fScale, pList[ i ].dwColor, pList[ i ].eAnchor, pList[ i ].eAlign, pList[ i ].bPinToEdge );
	}

	e_UIClearDebugLabels();
}

//free's any client units created
void c_UIFreeClientUnits( GAME *pGame )
{
	//if it hasn't been init'ed then no need to free the units
	if( !gUIClientUnitsInited ||  //if it hasn't been init'ed return
		gUIClientUnits.m_iMaxUnitsCreated <= 0) //if we don't have any items to remove return
	{
		return;
	}
	for( int t = 0; t < gUIClientUnits.m_iMaxUnitsCreated; t++ )
	{
		if( gUIClientUnits.gUIClientUnits[ t ] != NULL )
		{
			UnitFree( gUIClientUnits.gUIClientUnits[ t ] );
			gUIClientUnits.gUIClientUnits[ t ] = NULL;
		}
	}
	gUIClientUnits.m_iCurrentClientIndex = 0;
	gUIClientUnits.m_iMaxUnitsCreated = 0;
}
//actually creates the unit and sends it back
static UNIT * sUICreateClientUNIT( GENUS eGenus,
								   int nClassID,
								   int nQuality,
								   int nUnitCount, /*=1*/
								   DWORD nFlags /*=0*/)
{
	UNIT *pReturnUnit( NULL );
	UNIT *pPlayer = UIGetControlUnit();
	ASSERT_RETNULL( pPlayer );
	GAME *pGame = UnitGetGame( pPlayer );
	UNIT_DATA *pUnitData = UnitGetData( pGame, eGenus, nClassID );
	ASSERT_RETNULL( pUnitData );
	//ok so now we have everything we need to create the unit
	VECTOR nPositionZero( 0, 0, 0 );
	switch( eGenus )
	{
	case GENUS_ITEM:
		{
			ITEM_SPEC tItemSpec;		
			tItemSpec.nItemClass = nClassID;
			tItemSpec.pvPosition = &nPositionZero;
			tItemSpec.nNumber = nUnitCount;
			tItemSpec.guidUnit = INVALID_ID;
			tItemSpec.idUnit = INVALID_ID;
			tItemSpec.nQuality = nQuality;
			tItemSpec.eLoadType = UDLT_INVENTORY;
			pReturnUnit = ItemInit( pGame, tItemSpec );
			if( pReturnUnit )
			{
				//init the item
				s_ItemPropsInit( pGame, pReturnUnit, UIGetControlUnit() );
				// make it identified
				UnitSetStat( pReturnUnit, STATS_IDENTIFIED, TRUE );
				// set the quantity of the item needed
				if( nUnitCount > 1 )
				{
					if( UnitGetStat( pReturnUnit, STATS_ITEM_QUANTITY_MAX ) <  nUnitCount )
						UnitSetStat( pReturnUnit, STATS_ITEM_QUANTITY_MAX, nUnitCount );
					ItemSetQuantity( pReturnUnit, nUnitCount );
				}
				
			}
		}
		break;
	case GENUS_MONSTER:

		break;
	case GENUS_OBJECT:

		break;
	}

	ASSERT_RETNULL( pReturnUnit );
	//I so need to find a better way of doing this
	if( gUIClientUnits.gUIClientUnits[ gUIClientUnits.m_iCurrentClientIndex ] != NULL )
	{
		UnitFree( gUIClientUnits.gUIClientUnits[ gUIClientUnits.m_iCurrentClientIndex ] );
	}
	gUIClientUnits.gUIClientUnits[ gUIClientUnits.m_iCurrentClientIndex ] = pReturnUnit;	
	if( gUIClientUnits.m_iMaxUnitsCreated < UI_MAX_CLIENT_UNITS )
		gUIClientUnits.m_iMaxUnitsCreated++; //move on to the next one.
	gUIClientUnits.m_iCurrentClientIndex++;
	if( gUIClientUnits.m_iCurrentClientIndex >= UI_MAX_CLIENT_UNITS )
		gUIClientUnits.m_iCurrentClientIndex = 0; //have to make it loop back around

	
	return pReturnUnit;
}

//will pass back a client only unit.  If the unit isn't found, it creates it.
UNIT * c_UIGetClientUNIT( GENUS eGenus,
						  int nClassID,
						  int nQuality,
						  int nUnitCount, /*=1*/
						  DWORD nFlags /*=0*/)
{

	ASSERT_RETNULL( nClassID != INVALID_ID );	
	ASSERT_RETNULL( nUnitCount > 0 );
	if( !gUIClientUnitsInited )
	{
		//if it hasn't been init'ed then we better init it.
		ZeroMemory( &gUIClientUnits, sizeof( UI_CLIENT_UNITS ));		
		gUIClientUnitsInited = TRUE;
	}
	
	for( int t = 0; t < gUIClientUnits.m_iMaxUnitsCreated; t++ )
	{
		if( UnitGetGenus( gUIClientUnits.gUIClientUnits[ t ] ) == eGenus &&
			UnitGetClass( gUIClientUnits.gUIClientUnits[ t ] ) == nClassID )
		{
			if( ItemGetQuantity( gUIClientUnits.gUIClientUnits[ t ] ) == nUnitCount/* &&
				ItemGetQuality( gUIClientUnits.gUIClientUnits[ t ] ) == nQuality*/ )
			{
				return gUIClientUnits.gUIClientUnits[ t ];
			}
		}
	}
	
	return sUICreateClientUNIT( eGenus, nClassID, nQuality, nUnitCount, nFlags );
}

void c_UIFreeClientUnit( UNIT *pUnit )
{
	ASSERT_RETURN( pUnit );		
	if( !gUIClientUnitsInited )
		return;
	for( int t = 0; t < gUIClientUnits.m_iMaxUnitsCreated; t++ )
	{
		if( gUIClientUnits.gUIClientUnits[ t ] == pUnit )			
		{
			UnitFree( gUIClientUnits.gUIClientUnits[ t ] );			
			gUIClientUnits.gUIClientUnits[ t ] = gUIClientUnits.gUIClientUnits[ gUIClientUnits.m_iMaxUnitsCreated - 1 ];
			gUIClientUnits.m_iMaxUnitsCreated--;
			gUIClientUnits.m_iCurrentClientIndex--;
			gUIClientUnits.gUIClientUnits[ gUIClientUnits.m_iMaxUnitsCreated ] = NULL;
			return;
		}
	}
}


#endif //!SERVER_VERSION
