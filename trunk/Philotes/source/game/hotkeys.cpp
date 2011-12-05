//----------------------------------------------------------------------------
// FILE: hotkeys.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "bookmarks.h"
#include "hotkeys.h"
#include "inventory.h"
#include "items.h"
#include "units.h"
#include "game.h"
#include "unittag.h"
#include "unitfile.h"
#include "unitidmap.h"
#include "skills.h"
#include "clients.h"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitHotkeyInit(
	UNIT_TAG_HOTKEY &tHotkey)
{
	for (int i = 0; i < HOTKEY_MAX_ITEMS; ++i)
	{
		tHotkey.m_idItem[ i ] = INVALID_ID;
	}
	for (int i = 0; i < HOTKEY_MAX_SKILLS; ++i)
	{
		tHotkey.m_nSkillID[ i ] = INVALID_LINK;
	}
	tHotkey.m_nLastUnittype = UNITTYPE_NONE;

}

//----------------------------------------------------------------------------
enum HOTKEY_SAVE_METHOD
{
	HOTKEY_DATABASE,
	HOTKEY_FILE
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sHotkeyItemsXferByGUID(
	GAME *pGame,
	UNIT_TAG_HOTKEY *pTag,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	HOTKEY_SAVE_METHOD eMethod)
{

	// num guid items
	unsigned int nNumItems = HOTKEY_MAX_ITEMS;
	XFER_UINT( tXfer, nNumItems, STREAM_BITS_HOTKEY_NUM_ITEMS );
	ASSERT_RETFALSE(nNumItems <= HOTKEY_MAX_ITEMS);

	// xfer each hotkey
	for (int unsigned i = 0; i < nNumItems; ++i)
	{
		BOOL bHasItem = FALSE;
		UNIT *pItem = NULL;
		if (tXfer.IsSave())
		{
			UNITID idItem = INVALID_ID;		
			idItem = pTag->m_idItem[ i ];
			if (idItem != INVALID_ID)
			{
				pItem = UnitGetById( pGame, idItem );
			}
		}
		bHasItem = pItem != NULL;
		XFER_BOOL( tXfer, bHasItem );

		// xfer item if present
		if (bHasItem)
		{
			PGUID guidItem = INVALID_GUID;
			if (tXfer.IsSave())
			{
				ASSERTX_RETFALSE( pItem, "Expected item" );
				guidItem = UnitGetGuid( pItem );
			}
			XFER_GUID( tXfer, guidItem );
			
			// link item up when loading
			if (tXfer.IsLoad())
			{
				pItem = UnitGetByGuid( pGame, guidItem );
				if (pItem && i < HOTKEY_MAX_ITEMS)
				{
					pTag->m_idItem[ i ] = UnitGetId( pItem );
				}
			}
			
		}
				
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sHotkeyItemsXferByAlias(
	GAME *pGame,
	UNIT_TAG_HOTKEY *pTag,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	HOTKEY_SAVE_METHOD eMethod)
{

	// num guid items
	unsigned int nNumItems = HOTKEY_MAX_ITEMS;
	XFER_UINT( tXfer, nNumItems, STREAM_BITS_HOTKEY_NUM_ITEMS );
	ASSERT_RETFALSE(nNumItems <= HOTKEY_MAX_ITEMS);

	// xfer each hotkey
	for (int unsigned i = 0; i < nNumItems; ++i)
	{
		BOOL bHasItem = FALSE;
		unsigned int nAlias = INVALID_ID;
		if (tXfer.IsSave())
		{
			UNITID idItem = INVALID_ID;		
			idItem = pTag->m_idItem[ i ];
			if (idItem != INVALID_ID)
			{
				nAlias = UnitIdMapFindByUnitID( pGame->m_pUnitIdMap, idItem );
			}
		}
		bHasItem = nAlias != INVALID_ID;
		XFER_BOOL( tXfer, bHasItem );

		// xfer item if present
		if (bHasItem)
		{
		
			// xfer alias
			XFER_UINT( tXfer, nAlias, STREAM_BITS_UNIT_ALIAS );
			
			// link item up when loading
			if (tXfer.IsLoad())
			{
				UNITID idUnit = UnitIdMapFindByAlias( pGame->m_pUnitIdMap, nAlias );
				UNIT *pItem = UnitGetById( pGame, idUnit );
				if (pItem && i < HOTKEY_MAX_ITEMS)
				{
					pTag->m_idItem[ i ] = idUnit;
				}
				
			}
			
		}
				
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sHotkeyXfer(
	UNIT *pUnit,
	UNIT_TAG_HOTKEY *pTag,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	HOTKEY_SAVE_METHOD eMethod)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// init struct when loading
	if (tXfer.IsLoad())
	{
		sUnitHotkeyInit( *pTag );
	}

	// xfer hotkey items
	if (eMethod == HOTKEY_DATABASE)
	{
		if (sHotkeyItemsXferByGUID( pGame, pTag, tXfer, tHeader, eMethod ) == FALSE)
		{
			return FALSE;
		}
	}
	else
	{
		if (sHotkeyItemsXferByAlias( pGame, pTag, tXfer, tHeader, eMethod ) == FALSE)
		{
			return FALSE;
		}	
	}
				
	// num hotkey skills
	unsigned int nNumSkills = MAX_HOTKEY_SKILLS;
	XFER_UINT( tXfer, nNumSkills, STREAM_BITS_NUM_HOTKEY_SKILLS );
	for (unsigned int i = 0; i < nNumSkills; ++i)
	{
		DWORD dwSkillCode = INVALID_CODE;
		if (tXfer.IsSave())
		{
			int nSkill = pTag->m_nSkillID[ i ];		
			if (nSkill != INVALID_LINK)
			{
				dwSkillCode = ExcelGetCode( pGame, DATATABLE_SKILLS, nSkill );
			}
		}
		
		BOOL bHasSkill = dwSkillCode != INVALID_CODE;
		XFER_BOOL( tXfer, bHasSkill );
		if (bHasSkill)
		{
		
			// xfer code
			XFER_DWORD( tXfer, dwSkillCode, STREAM_BITS_SKILL_CODE );
			
			// link back up when loading
			if (tXfer.IsLoad())
			{
				int nSkill = ExcelGetLineByCode( NULL, DATATABLE_SKILLS, dwSkillCode );
				pTag->m_nSkillID[ i ] = nSkill;
			}
			
		}

	}
	
	// last unit type
	DWORD dwUnitTypeCode = INVALID_CODE;
	if (tXfer.IsSave())
	{
		if (pTag->m_nLastUnittype != INVALID_LINK)
		{
			dwUnitTypeCode = ExcelGetCode( NULL, DATATABLE_UNITTYPES, pTag->m_nLastUnittype );
		}
	}
	XFER_DWORD( tXfer, dwUnitTypeCode, STREAM_BITS_UNITTYPE_CODE );
	
	// link back up when loading
	if (tXfer.IsLoad())
	{
		int nUnitType = ExcelGetLineByCode( NULL, DATATABLE_UNITTYPES, dwUnitTypeCode );
		pTag->m_nLastUnittype = nUnitType;
	}
	
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TAG_SELECTOR sGetHotkeyTagSelectorByCode(
	DWORD dwCode)
{
	return (TAG_SELECTOR)ExcelGetLineByCode( NULL, DATATABLE_TAG, dwCode );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sGetHotkeyCode(
	TAG_SELECTOR eTagSelector)
{
	return ExcelGetCode( NULL, DATATABLE_TAG, eTagSelector );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL HotkeysXfer(
	UNIT *pUnit,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	BOOKMARKS &tBookmarks,
	BOOL bSkipData)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
		
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_HOTKEYS ))
	{
	
		// set bookmark for hotkeys when saving
		if (tXfer.IsSave())
		{
			SetBookmark( tBookmarks, UB_HOTKEYS, tXfer, tXfer.GetCursor(), tHeader );
		}
	
		// hotkey magic!
		DWORD dwMagic = PLAYERSAVE_HOTKEYS_MAGIC;
		XFER_DWORD( tXfer, dwMagic, STREAM_MAGIC_NUMBER_BITS );
		ASSERTX_RETFALSE( dwMagic == PLAYERSAVE_HOTKEYS_MAGIC, "Expected magic hotkey number" );

		// position in stream of next block of data (so we can skip it when reading)
		unsigned int nCursorNextBlock = tXfer.GetCursor();
		XFER_UINT( tXfer, nCursorNextBlock, STREAM_CURSOR_SIZE_BITS );

		// we have to throw away some old hotkey data
		if (tHeader.nVersion <= UNITSAVE_VESRION_HOTKEYS_WRONG_SELECTOR_2007_07_02)
		{
			bSkipData = TRUE;
		}
				
		// skip if desired when reading
		if (bSkipData && tXfer.IsLoad())
		{
			ASSERTX_RETFALSE( nCursorNextBlock > 0, "Invalid cursor, can't skip hotkey data" );
			if (tXfer.SetCursor( nCursorNextBlock ) == XFER_RESULT_ERROR)
			{
				ASSERTV_RETFALSE( 0, "Unable to set hotkey next cursor block to '%d'", nCursorNextBlock );
			}
			return TRUE;
		}
		
		// num hotkeys			
		int nCursorNumHotKeys = tXfer.GetCursor();
		unsigned int nNumHotKeys = TAG_SELECTOR_HOTKEY_LAST - TAG_SELECTOR_HOTKEY_FIRST;
		XFER_UINT( tXfer, nNumHotKeys, STREAM_MAX_HOTKEY_BITS );

		// set the xfer function for the proper save mode
		HOTKEY_SAVE_METHOD eMethod = HOTKEY_FILE;
		#if ISVERSION(SERVER_VERSION)
			eMethod = HOTKEY_DATABASE;
		#endif
		
		unsigned int nNumHotkeysXfered = 0;
		for (unsigned int i = 0; i < nNumHotKeys; ++i)
		{
		
			// get code for hotkey
			DWORD dwCode = INVALID_CODE;
			if (tXfer.IsSave())
			{
				TAG_SELECTOR eTagSelector = TAG_SELECTOR_INVALID;				
				eTagSelector = (TAG_SELECTOR)(i + TAG_SELECTOR_HOTKEY_FIRST);
				if (UnitGetHotkeyTag( pUnit, eTagSelector ) == NULL)
				{
					continue;  // no hotkey info, skip to next one
				}
				dwCode = sGetHotkeyCode( eTagSelector );
			}			
			XFER_DWORD( tXfer, dwCode, STREAM_BITS_HOTKEY_CODE_BITS );
			TAG_SELECTOR eTagSelector = sGetHotkeyTagSelectorByCode( dwCode );
			
			// get tag hotkey to xfer
			UNIT_TAG_HOTKEY *pTag = NULL;
			UNIT_TAG_HOTKEY tJunk;  // for loading old hotkeys no longer used
			if (tXfer.IsSave())
			{
				pTag = UnitGetHotkeyTag( pUnit, eTagSelector );
			}
			else
			{
				if (eTagSelector != TAG_SELECTOR_INVALID)
				{
					pTag = UnitAddHotkeyTag( pUnit, eTagSelector );				
					if (pTag == NULL)
					{
						ASSERTV( pTag, "Ignoring expected hotkey tag selector '%d'", eTagSelector );
						pTag = &tJunk;
					}
				}
				else
				{
					pTag = &tJunk;  // old hotkey, read into junk struct on stack and throw away				
				}
			}
			
			// xfer the hotkey
			ASSERTX_RETFALSE( pTag, "Expected hotkey tag" );			
			if (sHotkeyXfer( pUnit, pTag, tXfer, tHeader, eMethod ) == FALSE)
			{
				ASSERTV_RETFALSE(
					0,
					"Unable to xfer hotkey '%d', code=%d, unit=%s",
					i,
					dwCode,
					UnitGetDevName( pUnit ));
			}
			nNumHotkeysXfered++;
										
		}	
			
		// go back and write the number of hotkeys written
		if (tXfer.IsSave())
		{
		
			unsigned int nCursorCurrent = tXfer.GetCursor();
			if (tXfer.SetCursor( nCursorNumHotKeys ) == XFER_RESULT_ERROR)
			{
				ASSERTV_RETFALSE( 0, "Unable to set cursor nCursorNumHotKeys (%d)", nCursorNumHotKeys );
			}
			XFER_UINT( tXfer, nNumHotkeysXfered, STREAM_MAX_HOTKEY_BITS );
			
			// go back and write what the position *after* all this hotkey data is
			if (tXfer.SetCursor( nCursorNextBlock ) == XFER_RESULT_ERROR)
			{
				ASSERTV_RETFALSE( 0, "Unable to set cursor nCursorNextBlock (%d)", nCursorNextBlock );			
				return FALSE;
			}
			XFER_UINT( tXfer, nCursorCurrent, STREAM_CURSOR_SIZE_BITS );

			// set buffer to next pos	
			if (tXfer.SetCursor( nCursorCurrent ) == XFER_RESULT_ERROR)
			{
				ASSERTV_RETFALSE( 0, "Unable to set cursor nCursorCurrent (%d)", nCursorCurrent );
				return FALSE;
			}			

		}
						
	}
					
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void HotkeySet( 
	GAME* pGame,
	UNIT* pUnit,
	BYTE bySkillAssign,
	int nSkillID)
{
	//Put in check for skill validity before assigning.
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkillID );
	UNITLOG_ASSERT_RETURN(pSkillData && SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_IN_MOUSE ), pUnit );
	//end check.

	UnitSetStat( pUnit, bySkillAssign == 0 ? STATS_SKILL_LEFT : STATS_SKILL_RIGHT, nSkillID );

	int nCurrentConfig = UnitGetStat(pUnit, STATS_CURRENT_WEAPONCONFIG);
	UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(pUnit, nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1 );
	ASSERT_RETURN(pTag);
	if (bySkillAssign == 0)
		pTag->m_nSkillID[0] = nSkillID;
	else
		pTag->m_nSkillID[1] = nSkillID;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void HotkeysSendAll(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	TAG_SELECTOR eFirstKey,
	TAG_SELECTOR eLastKey)
{
	for (int ii = eFirstKey; ii <= eLastKey; ii++)
	{
		HotkeySend( client, unit, (TAG_SELECTOR)ii);		
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void HotkeySend(
	GAMECLIENT *pClient,
	UNIT *pPlayer,
	TAG_SELECTOR eHotkey)
{
	ASSERTX_RETURN( pClient, "Expected destination client" );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// don't bother with hotkeys when freeing units
	if (UnitTestFlag( pPlayer, UNITFLAG_JUSTFREED ))
	{
		return;
	}
		
	// we can't send hotkeys to clients who have not had thier control unit set
	UNIT *pControlUnit = ClientGetControlUnit( pClient );
	ASSERTX_RETURN( pControlUnit, "Expected control unit" );	
	ASSERTX_RETURN( UnitGetStat( pControlUnit, STATS_CONTROL_UNIT_SENT ) == TRUE, "Cannot send hotkey messages until control unit has been sent for this client" );

	// get hotkey tag
	UNIT_TAG_HOTKEY *pHotkey = UnitGetHotkeyTag( pPlayer, eHotkey );
	if (pHotkey)
	{
		GAME *pGame = UnitGetGame( pPlayer );
			
		// setup message
		MSG_SCMD_HOTKEY tMessage;
		tMessage.idItem[0] = pHotkey->m_idItem[0];
		tMessage.idItem[1] = pHotkey->m_idItem[1];
		tMessage.nSkillID[0] = pHotkey->m_nSkillID[0];
		tMessage.nSkillID[1] = pHotkey->m_nSkillID[1];
		tMessage.nLastUnittype = pHotkey->m_nLastUnittype;
		tMessage.bHotkey = DOWNCAST_INT_TO_BYTE( eHotkey );
		ASSERT( tMessage.nSkillID[0] == INVALID_ID || (unsigned int)tMessage.nSkillID[0] < ExcelGetNumRows( pGame, DATATABLE_SKILLS ));
		ASSERT( tMessage.nSkillID[1] == INVALID_ID || (unsigned int)tMessage.nSkillID[1] < ExcelGetNumRows( pGame, DATATABLE_SKILLS ));
		
		// send
		s_SendMessage( pGame, ClientGetId( pClient ), SCMD_HOTKEY, &tMessage );

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_HotkeysReturnItemsToStandardInventory(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETURN( pGame, "Expected game" );

	int nInventoryIndex = UnitGetInventoryType( pPlayer );
	if (nInventoryIndex > 0)
	{
		const INVENTORY_DATA *pInventoryData = InventoryGetData( pGame, nInventoryIndex );
		ASSERTX_RETURN( pInventoryData, "Expected inventory data" );

		for (int i = pInventoryData->nFirstLoc; i < pInventoryData->nFirstLoc + pInventoryData->nNumLocs; ++i)
		{			
			const INVLOC_DATA *pInvLocData = InvLocGetData( pGame, i );
			if (pInvLocData && InvLocTestFlag( pInvLocData, INVLOCFLAG_WEAPON_CONFIG_LOCATION ))
			{
				int nInvLoc = pInvLocData->nLocation;
				s_ItemsRestoreToStandardLocations( pPlayer, nInvLoc );
			}
			
		}
		
	}	
		
}
