//----------------------------------------------------------------------------
// c_message.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "clients.h"
#include "c_connmgr.h"
#include "c_message.h"
#include "c_network.h"
#include "s_network.h"
#include "player.h"
#include "config.h"
#include "wardrobe.h"
#include "wardrobe_priv.h"
#include "unit_priv.h"
#include "c_appearance.h"
#include "crc.h"
#include "filepaths.h"
#include "uix.h"
#include "globalindex.h"
#include "version.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "gameunits.h"
#include "achievements.h"

//----------------------------------------------------------------------------
// FUNCTIONS (Messages from Client to Server)
//----------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// register command list
// ---------------------------------------------------------------------------
NET_COMMAND_TABLE * c_NetGetCommandTable(
	void)
{
	static CCritSectLite cs;
	static NET_COMMAND_TABLE * s_ClientCommandTable = NULL;

	CSLAutoLock autolock(&cs);

	if (s_ClientCommandTable)
	{
		NetCommandTableRef(s_ClientCommandTable);
		return s_ClientCommandTable;
	}

	NET_COMMAND_TABLE * cmd_tbl = NULL;
	cmd_tbl = NetCommandTableInit(CCMD_LAST);
	ASSERT_RETNULL(cmd_tbl);

#undef  NET_MSG_TABLE_BEGIN
#undef  NET_MSG_TABLE_DEF
#undef  NET_MSG_TABLE_END

#define NET_MSG_TABLE_BEGIN
#define NET_MSG_TABLE_END(c)
#define NET_MSG_TABLE_DEF(c, s, r)		{  MSG_##c msg; MSG_STRUCT_DECL * msg_struct = msg.zz_register_msg_struct(cmd_tbl); \
											NetRegisterCommand(cmd_tbl, c, msg_struct, s, r, static_cast<MFP_PRINT>(&MSG_##c::Print), \
											static_cast<MFP_PUSHPOP>(&MSG_##c::Push), static_cast<MFP_PUSHPOP>(&MSG_##c::Pop)); }

#undef  _C_MESSAGE_ENUM_H_
#include "c_message.h"

	if (!NetCommandTableValidate(cmd_tbl)
#if !ISVERSION(SERVER_VERSION)
		|| !CltValidateMessageTable()
#endif
		)
	{
		NetCommandTableFree(cmd_tbl);
		return NULL;
	}
	s_ClientCommandTable = cmd_tbl;
	return s_ClientCommandTable;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_NetCommandTableFree(
	void)
{
	NetCommandTableFree(gApp.m_GameClientCommandTable);
	gApp.m_GameClientCommandTable = NULL;
}
#endif


//----------------------------------------------------------------------------
// TRUE = don't send, FALSE = send
//----------------------------------------------------------------------------
BOOL c_FilterCommandByAppState(
	BYTE command)
{
	if (AppGetState() == APP_STATE_EXIT)
	{
		if (command != CCMD_PLAYERREMOVE)
		{
			return TRUE;
		}
	}
	if (AppGetState() == APP_STATE_PLAYMOVIELIST)
	{
		if (command != CCMD_MOVIE_FINISHED)
		{
			return TRUE;
		}
		return FALSE;
	}
	if (AppGetState() != APP_STATE_IN_GAME)
	{
		switch (command)
		{
		case CCMD_PLAYERNEW:
		case CCMD_OPEN_PLAYER_FILE_START:
		case CCMD_OPEN_PLAYER_FILE_CHUNK:
		case CCMD_LEVELLOADED:
			break;
		default:
			return TRUE;
		}
	}
	return FALSE;	// don't filter
}

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_SendMessage(
	BYTE command,
	MSG_STRUCT * msg)
{
	if (c_FilterCommandByAppState(command))
	{
		return FALSE;
	}

    if( !ConnectionManagerSend(AppGetGameChannel(),msg,command) &&
		!AppIsSinglePlayer() )
    {
		//	TEMP: probably add a better error handling mechanism...
		AppSetErrorState(
			GS_NETWORK_ERROR_TITLE,
			GS_NETWORK_ERROR);
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
// send the player file to the server
//----------------------------------------------------------------------------
BOOL c_SendPlayerFile(
	const WCHAR * szPlayerName,
	int nDifficultySelected)
{
	ASSERTX_RETFALSE(szPlayerName && szPlayerName[0], "Invalid player name");
	
	// validate difficulty
	if (nDifficultySelected == INVALID_LINK)
	{
		ASSERTX( nDifficultySelected != INVALID_LINK, "Expected difficulty" );
		nDifficultySelected = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
	}

	OS_PATH_CHAR asciiname[MAX_CHARACTER_NAME];
	PStrCvt(asciiname, szPlayerName, _countof(asciiname));

	OS_PATH_CHAR filename[MAX_PATH];
	PStrPrintf(filename, _countof(filename), OS_PATH_TEXT("%s%s.hg1"), FilePath_GetSystemPath(FILE_PATH_SAVE), asciiname);

	DWORD size;
	BYTE * pRichHeader = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE(NULL, filename, &size));
	BYTE * data = RichSaveFileRemoveHeader(pRichHeader, &size);//(BYTE *)FileLoad(NULL, filename, &size);
	if (!data)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(size < USHRT_MAX);
	//BYTE * StartPoint =(RichSaveFileRemoveHeader(data, &size));
	UNIT_FILE_HEADER tHeader;
	UnitFileReadHeader( data, size, tHeader );
	
	MSG_CCMD_OPEN_PLAYER_FILE_START msg;
	PStrCopy(msg.szCharName, szPlayerName, MAX_CHARACTER_NAME);
	msg.nDifficultySelected = nDifficultySelected;
	msg.dwCRC = tHeader.dwCRC;
	msg.dwSize = size;
	c_SendMessage(CCMD_OPEN_PLAYER_FILE_START, &msg);

	BYTE * datatosend = data;
	unsigned int lefttosend = size;
	while (lefttosend != 0)
	{
		unsigned int sizetosend = MIN(lefttosend, (unsigned int)MAX_PLAYERSAVE_BUFFER);
	
		MSG_CCMD_OPEN_PLAYER_FILE_CHUNK msg;	
		memcpy(msg.buf, datatosend, sizetosend);
		MSG_SET_BLOB_LEN(msg, buf, sizetosend);
		c_SendMessage(CCMD_OPEN_PLAYER_FILE_CHUNK, &msg);

		datatosend += sizetosend;
		lefttosend -= sizetosend;
	}

	FREE(NULL, pRichHeader);
	return TRUE;
}
					  

//----------------------------------------------------------------------------
// new player ready to enter game
//----------------------------------------------------------------------------
void c_SendNewPlayer(
	BOOL bNewPlayer,
	const WCHAR * szName,
	int nPlayerSlot,
	unsigned int nClass,
	WARDROBE_BODY * pWardrobeBody,
	APPEARANCE_SHAPE * pAppearanceShape,
	DWORD dwGemActivation,
	DWORD dwNewPlayerFlags,			// see NEW_PLAYER_FLAGS
	int nDifficulty)
{
	ASSERT_RETURN(nClass < UCHAR_MAX);

	WCHAR szPlayerName[MAX_CHARACTER_NAME];

	// set default name if no name is passed in (from either global or hardcoded default)
	if (!szName || !szName[0])
	{
		// first see if there's an existing character file we can use
		ULARGE_INTEGER tLatestTime;
		tLatestTime.HighPart = 0;
		tLatestTime.LowPart = 0;
		OS_PATH_CHAR szPlayerFileName[MAX_CHARACTER_NAME];

		OS_PATH_CHAR filename[MAX_PATH];
		PStrPrintf(filename, _countof(filename), OS_PATH_TEXT("%s*.hg1"), FilePath_GetSystemPath(FILE_PATH_SAVE));
		OS_PATH_FUNC(WIN32_FIND_DATA) data;

		FindFileHandle handle = OS_PATH_FUNC(FindFirstFile)( filename, &data );
		if (handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (data.ftLastWriteTime.dwHighDateTime > tLatestTime.HighPart ||
					(data.ftLastWriteTime.dwHighDateTime == tLatestTime.HighPart &&
					 data.ftLastWriteTime.dwLowDateTime > tLatestTime.LowPart))
				{
					PStrRemoveExtension(szPlayerFileName, _countof(szPlayerFileName), data.cFileName);
					tLatestTime.HighPart = data.ftLastWriteTime.dwHighDateTime;
					tLatestTime.LowPart = data.ftLastWriteTime.dwLowDateTime;
				}

			} while ( OS_PATH_FUNC(FindNextFile)( handle, &data ) );

			PStrCvt(szPlayerName, szPlayerFileName, _countof(szPlayerName));
		}
		else
		{
			GLOBAL_DEFINITION * global = DefinitionGetGlobal();
			if (global)
			{
				PStrCvt(szPlayerName, global->szPlayerName, MAX_CHARACTER_NAME);
			}
			else
			{
				PStrCopy(szPlayerName, DEFAULT_PLAYER_NAME, MAX_CHARACTER_NAME);
			}
		}
	}
	else
	{
		PStrCopy(szPlayerName, szName, MAX_CHARACTER_NAME);
	}

	// if the apptype is open client and it's not a new player, then send the character file first
	if (AppGetType() == APP_TYPE_OPEN_CLIENT && !bNewPlayer)
	{
		ASSERT_RETURN(c_SendPlayerFile(szPlayerName, nDifficulty));
		return;
	}

	MSG_CCMD_PLAYERNEW msg;
	PStrCopy(msg.szCharName, szPlayerName, MAX_CHARACTER_NAME);
	msg.bNewPlayer = (BYTE)bNewPlayer;
	msg.bPlayerSlot = (BYTE)nPlayerSlot;
	msg.bClass = (BYTE)nClass;
    msg.qwBuildVersion = VERSION_AS_ULONGLONG;
	msg.bGem = (BYTE) dwGemActivation;
	msg.dwNewPlayerFlags = dwNewPlayerFlags;
	msg.nDifficulty = nDifficulty;

#if ISVERSION(DEVELOPMENT)
	if(AppTestDebugFlag(ADF_DIFFICULTY_OVERRIDE))
	{
		if(AppTestDebugFlag(ADF_NORMAL))
		{
			msg.nDifficulty = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
		}
		if(AppTestDebugFlag(ADF_NIGHTMARE))
		{
			msg.nDifficulty = GlobalIndexGet( GI_DIFFICULTY_NIGHTMARE );
		}
		if(AppTestDebugFlag(ADF_HELL))
		{
			msg.nDifficulty = GlobalIndexGet( GI_DIFFICULTY_HELL );
		}
	}
#endif

	// set up creation appearance for player?  
	const UNIT_DATA * pUnitData = PlayerGetData(NULL, nClass);

	if (!pAppearanceShape)
	{
		APPEARANCE_SHAPE tAppearanceShape;
		AppearanceShapeRandomize(pUnitData->tAppearanceShapeCreate, tAppearanceShape);
		msg.bHeight = tAppearanceShape.bHeight;
		msg.bWeight = tAppearanceShape.bWeight;
	}
	else
	{
		msg.bHeight = pAppearanceShape->bHeight;
		msg.bWeight = pAppearanceShape->bWeight;
	}

	int nAppearanceGroup = pUnitData->pnWardrobeAppearanceGroup[ UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON ];
	WARDROBE_INIT tWardrobeInit( nAppearanceGroup );
	if (pWardrobeBody)
		memcpy(&tWardrobeInit.tBody, pWardrobeBody, sizeof(WARDROBE_BODY));
	else
		WardrobeRandomizeInitStruct( AppGetCltGame(), tWardrobeInit, nAppearanceGroup, TRUE );

	int nWardrobeBytes = WardrobeInitStructWrite( tWardrobeInit, msg.bufWardrobeInit, MAX_WARDROBE_INIT_BUFFER );
	int nSize = OFFSET(MSG_CCMD_PLAYERNEW, bufWardrobeInit) + nWardrobeBytes;
	REF(nSize);

	c_SendMessage(CCMD_PLAYERNEW, &msg);
}


//----------------------------------------------------------------------------
// leaving game
//----------------------------------------------------------------------------
void c_SendRemovePlayer(
	void)
{
	MSG_CCMD_PLAYERREMOVE msg;
	c_SendMessage(CCMD_PLAYERREMOVE, &msg);

	switch(AppGetType())
	{
	case APP_TYPE_SINGLE_PLAYER:
	case APP_TYPE_OPEN_CLIENT:
	case APP_TYPE_OPEN_SERVER:
		return;
	default:
		AppSwitchState(APP_STATE_EXIT);
	}
}


//----------------------------------------------------------------------------
// advertising a party
//----------------------------------------------------------------------------
void c_SendPartyAdvertise(
	const WCHAR * wszPartyDesc,
	int nMaxPlayers,
	int nMinLevel,
	int nMaxLevel)
{
	MSG_CCMD_PARTY_ADVERTISE msg;

	msg.szPartyDesc[0] = 0;
	if( wszPartyDesc ) {
		PStrCopy(msg.szPartyDesc, wszPartyDesc, PARTY_DESC_SIZE);
	}
	msg.nMaxPlayers = nMaxPlayers;
	msg.nMinLevel = nMinLevel;
	msg.nMaxLevel = nMaxLevel;
	msg.bForSinglePlayerOnly = FALSE;
	msg.nMatchType = 0;

	c_SendMessage(CCMD_PARTY_ADVERTISE, &msg);
}

//----------------------------------------------------------------------------
// advertising a single player
//----------------------------------------------------------------------------
void c_SendPlayerAdvertiseLFG(
	const WCHAR * wszPartyDesc,
	int nMaxPlayers,
	int nMinLevel,
	int nMaxLevel)
{
	MSG_CCMD_PARTY_ADVERTISE msg;

	msg.szPartyDesc[0] = 0;
	if( wszPartyDesc ) {
		PStrCopy(msg.szPartyDesc, wszPartyDesc, PARTY_DESC_SIZE);
	}
	msg.nMaxPlayers = nMaxPlayers;
	msg.nMinLevel = nMinLevel;
	msg.nMaxLevel = nMaxLevel;
	msg.bForSinglePlayerOnly = TRUE;
	msg.nMatchType = 0;

	c_SendMessage(CCMD_PARTY_ADVERTISE, &msg);
}

//----------------------------------------------------------------------------
// send request to save mouse button skill assignments
//----------------------------------------------------------------------------
void c_SendSkillSaveAssign( 
	WORD wSkill, 
	BYTE bySkillAssign )
{
	MSG_CCMD_SKILLSAVEASSIGN msg;
	msg.wSkill = wSkill;
	msg.bySkillAssign = bySkillAssign;
	c_SendMessage(CCMD_SKILLSAVEASSIGN, &msg);
}


//----------------------------------------------------------------------------
// send the debug unit id
//----------------------------------------------------------------------------
void c_SendDebugUnitId( 
	UNITID idDebugUnit )
{
	MSG_CCMD_SETDEBUGUNIT msg;
	msg.idDebugUnit = idDebugUnit;
	c_SendMessage(CCMD_SETDEBUGUNIT, &msg);
}

//----------------------------------------------------------------------------
// start firing your gun
//----------------------------------------------------------------------------
void c_SendSkillStart( 
	const VECTOR & vPosition,
	WORD wSkill, 
	UNITID idTarget, 
	const VECTOR & vTarget )
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETURN(pGame);
	UNIT * pControlUnit = GameGetControlUnit(pGame);
	ASSERT_RETURN(pControlUnit);

	int nWeaponIndex = 0;
	UNIT * pWeapon = WardrobeGetWeapon(pGame, UnitGetWardrobe(pControlUnit), nWeaponIndex);
	if(!pWeapon)
	{
		nWeaponIndex++;
	}
	VECTOR vWeaponPosition, vWeaponDirection;
	UnitGetWeaponPositionAndDirection(pGame, pControlUnit, nWeaponIndex, &vWeaponPosition, &vWeaponDirection);
	VectorNormalize(vWeaponDirection);

	if ( idTarget != INVALID_ID && ( vTarget.fX || vTarget.fY || vTarget.fZ ))
	{
		MSG_CCMD_SKILLSTARTXYZID msg;
		msg.vPosition = vPosition;
		msg.wSkill = wSkill;
		msg.idTarget = idTarget;
		msg.vTarget  = vTarget;
		msg.vWeaponPosition = vWeaponPosition;
		msg.vWeaponDirection = vWeaponDirection;
		c_SendMessage(CCMD_SKILLSTARTXYZID, &msg);
	}
	else if ( idTarget != INVALID_ID )
	{
		MSG_CCMD_SKILLSTARTID msg;
		msg.vPosition = vPosition;
		msg.wSkill = wSkill;
		msg.idTarget = idTarget;
		msg.vWeaponPosition = vWeaponPosition;
		msg.vWeaponDirection = vWeaponDirection;
		c_SendMessage(CCMD_SKILLSTARTID, &msg);
	}
	else if ( vTarget.fX || vTarget.fY || vTarget.fZ )
	{
		MSG_CCMD_SKILLSTARTXYZ msg;
		msg.vPosition = vPosition;
		msg.wSkill = wSkill;
		msg.vTarget = vTarget;
		msg.vWeaponPosition = vWeaponPosition;
		msg.vWeaponDirection = vWeaponDirection;
		c_SendMessage(CCMD_SKILLSTARTXYZ, &msg);
	}
	else 
	{
		MSG_CCMD_SKILLSTART msg;
		msg.vPosition = vPosition;
		msg.wSkill = wSkill;
		msg.vWeaponPosition = vWeaponPosition;
		msg.vWeaponDirection = vWeaponDirection;
		c_SendMessage(CCMD_SKILLSTART, &msg);
	}
}

//----------------------------------------------------------------------------
// send a message changing the current target of a skill to something new.
// only used while firing weapons
//----------------------------------------------------------------------------

void c_SendSkillChangeWeaponTarget(
	WORD wSkill,
	UNITID idWeapon,
	UNITID idTarget,
	int nTargetIndex )
{
	MSG_CCMD_SKILLCHANGETARGETID msg;
	msg.wSkill = wSkill;
	msg.idTarget = idTarget;
	msg.idWeapon = idWeapon;
	msg.bIndex = (BYTE) nTargetIndex;
	ASSERT( nTargetIndex == msg.bIndex );
	c_SendMessage(CCMD_SKILLCHANGETARGETID, &msg);
}


//----------------------------------------------------------------------------
// stop firing
//----------------------------------------------------------------------------
void c_SendSkillStop(
	int nSkill)
{
	MSG_CCMD_SKILLSTOP msg;
	msg.wSkill = (short)nSkill;
	c_SendMessage(CCMD_SKILLSTOP, &msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendSkillShiftEnable(
	int nSkill,
	BOOL bEnable )
{
	MSG_CCMD_SKILLSHIFT_ENABLE msg;
	msg.wSkill = (short)nSkill;
	msg.bEnabled = bEnable ? 1 : 0;
	c_SendMessage(CCMD_SKILLSHIFT_ENABLE, &msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_SendCheat(
	const WCHAR* txt)
{
	MSG_CCMD_CHEAT msg;
	PStrCopy(msg.str, txt, MAX_CHEAT_LEN);
	return c_SendMessage(CCMD_CHEAT, &msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendQuestAbandon(
	int	nQuestID )
{
	MSG_CCMD_QUEST_ABANDON msg;
	msg.nQuestID = nQuestID;
	c_SendMessage(CCMD_QUEST_ABANDON, &msg);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SendPlayerSelectAchievment( int nAchievementID,
								   int nAchievementSlot )
{
	MSG_CCMD_ACHIEVEMENT_SELECTED msg;
	msg.nAchievementID = nAchievementID;
	msg.nAchievementSlot = nAchievementSlot;
	c_SendMessage(CCMD_ACHIEVEMENT_SELECTED, &msg);
}

#endif //!SERVER_VERSION
