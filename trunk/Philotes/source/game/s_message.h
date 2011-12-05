//----------------------------------------------------------------------------
// s_message.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// SERVER to CLIENT messages
// 
// To add a new message "MYMESSAGE":
// 1: add a "SCMD_MYMESSAGE" to the ENUMERATIONS section in this file
// 2: add a DEF_MSG_STRUCT("SCMD_MYMESSAGE") in the STRUCTURES section in this file
//----------------------------------------------------------------------------
#ifndef _S_MESSAGE_H_
#define _S_MESSAGE_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _MESSAGE_H_
#include "message.h"
#endif

#ifndef _PRIME_H_
#include "prime.h"
#endif

#ifndef _GAME_H_
#include "game.h"
#endif

#ifndef _CHATSERVER_H_
#include "../source/ServerSuite/Chat/ChatServer.h"
#endif

#include "../source/ServerSuite/AuctionServer/AuctionServerDefines.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MOVE_FLAG_RESERVE_LOCATION		MAKE_MASK(0)
#define MOVE_FLAG_NOT_ONGROUND			MAKE_MASK(1)
#define MOVE_FLAG_CLEAR_ZVELOCITY		MAKE_MASK(2)
#define MOVE_FLAG_USE_DIRECTION			MAKE_MASK(3)
#define MOVE_FLAG_CLIENT_STRAIGHT		MAKE_MASK(4)


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum LOGINERROR
{
	LOGINERROR_UNKNOWN,
	LOGINERROR_INVALIDNAME,
	LOGINERROR_NAMEEXISTS,
	LOGINERROR_ALREADYONLINE,
	LOGINERROR_PLAYERNOTFOUND,
};


//----------------------------------------------------------------------------
// MESSAGE STRUCTURES
//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_YOURID)
	MSG_FIELD(0, GAMECLIENTID, id, INVALID_GAMECLIENTID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_ERROR)
	MSG_FIELD(0, BYTE, bError, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_GAME_NEW)
	MSG_FIELD(0, GAMEID, idGame, INVALID_ID)
	MSG_FIELD(1, DWORD, dwCurrentTick, 0)
	MSG_FIELD(2, BYTE, bGameSubType, 0)
	MSG_FIELD(3, DWORD, dwGameFlags, 0)
	MSG_STRUC(4, GAME_VARIANT, tGameVariant)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SET_LEVEL_COMMUNICATION_CHANNEL)
	MSG_FIELD(0, GAMEID, idGame, INVALID_ID)
	MSG_FIELD(1, DWORD, idLevel, INVALID_ID)
	MSG_FIELD(2, CHANNELID, idChannel, INVALID_CHANNELID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TEAM_NEW)
	MSG_FIELD(0, DWORD, idTeam, INVALID_ID)
	MSG_FIELD(1, BYTE, idTemplateTeam, -1)
	MSG_WCHAR(2, szTeamName, MAX_TEAM_NAME)	
	MSG_FIELD(3, WORD, ColorIndex, 0)	
	MSG_FIELD(4, BYTE, bDeactivateEmpty, 1)
	MSG_BLOBW(5, buf, MAX_TEAMS_PER_GAME)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TEAM_DEACTIVATE)
	MSG_FIELD(0, DWORD, idTeam, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TEAM_JOIN)
	MSG_FIELD(0, DWORD, idUnit, INVALID_ID)
	MSG_FIELD(1, DWORD, idTeam, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_GAME_CLOSE)
	MSG_FIELD(0, GAMEID, idGame, INVALID_ID)
	MSG_FIELD(1, BYTE, bRestartingGame, 0)
	MSG_FIELD(2, BYTE, bShowCredits, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_OPEN_PLAYER_INITSEND)
	MSG_FIELD(0, DWORD, dwCRC, 0)
	MSG_FIELD(1, DWORD, dwSize, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_OPEN_PLAYER_SENDFILE)
	MSG_BLOBW(0, buf, MAX_PLAYERSAVE_BUFFER)
	END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_SCMD_START_LEVEL_LOAD)
	MSG_FIELD(0, DWORD, nLevelDefinition, INVALID_LINK)
	MSG_FIELD(1, DWORD, nDRLGDefinition, INVALID_LINK)
	MSG_FIELD(2, int, nLevelDepth, 0) //this is for tugboat.
	MSG_FIELD(3, int, nLevelArea, -1) //this is for tugboat.	
	END_MSG_STRUCT

DEF_MSG_STRUCT(SUBLEVEL_DESC)
	MSG_FIELD(0, int, idSubLevel, INVALID_ID)
	MSG_FIELD(1, int, nSubLevelDef, INVALID_LINK)
	MSG_STRUC(2, MSG_VECTOR, vPosition)
END_MSG_STRUCT

#define MAX_SUBLEVEL_DESC 8  // must be same as MAX_SUBLEVELS (but i don't wanna include)
#define MAX_THEMES_PER_LEVEL_DESC 25  // must be same as MAX_THEMES_PER_LEVEL (but i don't wanna include)

DEF_MSG_STRUCT(MSG_SCMD_SET_LEVEL)
	MSG_FIELD(0, DWORD, idLevel, INVALID_ID)
	MSG_FIELD(1, DWORD, dwUID, 0)
	MSG_FIELD(2, DWORD, nLevelDefinition, 0)
	MSG_FIELD(3, DWORD, nDRLGDefinition, 0)
	MSG_FIELD(4, DWORD, nEnvDefinition, INVALID_ID)
	MSG_ARRAY(5, DWORD, nThemes, MAX_THEMES_PER_LEVEL_DESC)
	MSG_FIELD(6, DWORD, nSpawnClass, 0)
	MSG_FIELD(7, DWORD, nNamedMonsterClass, 0)
	MSG_FIELD(8, float, fNamedMonsterChance, 0)
	MSG_STRUC(9, MSG_VECTOR, vBoundingBoxMin)
	MSG_STRUC(10, MSG_VECTOR, vBoundingBoxMax)
	MSG_STARR(11, SUBLEVEL_DESC, tSubLevels, MAX_SUBLEVEL_DESC)
	MSG_BLOBB(12, tBuffer, MAX_SET_LEVEL_MSG_BUFFER)
	MSG_FIELD(13, float, CellWidth, 0)
	MSG_FIELD(14, int, nDepth, 0)
	MSG_FIELD(15, int, nArea, 0)	
	MSG_FIELD(16, int, nAreaDepth, 0)
	MSG_FIELD(17, int, nMusicRef, 0)
	MSG_FIELD(18, int, nAreaDifficulty, 0 )
	MSG_FIELD(19, DWORD, dwPVPWorld, 0 )
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SET_CONTROL_UNIT)
	MSG_FIELD(0, DWORD, id, INVALID_ID)
	MSG_FIELD(1, PGUID, guid, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CONTROLUNIT_FINISHED)
	MSG_FIELD(0, DWORD, id, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PLAYERNEW)
	MSG_FIELD(0, BYTE, idClient, 0)
	MSG_FIELD(1, UNITID, id, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PLAYERREMOVE)
	MSG_FIELD(0, GAMECLIENTID, id, INVALID_GAMECLIENTID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_MISSILENEWXYZ)
	MSG_FIELD(0, int, nClass, 0)
	MSG_FIELD(1, UNITID, id, INVALID_ID)
	MSG_FIELD(2, UNITID, idSource, INVALID_ID)
	MSG_FIELD(3, UNITID, idWeapon0, INVALID_ID)
	MSG_FIELD(4, UNITID, idWeapon1, INVALID_ID)
	MSG_FIELD(5, int, nTeam, 0)
	MSG_FIELD(6, WORD, nSkill, 0)
	MSG_FIELD(7, ROOMID, idRoom, INVALID_ID)
	MSG_STRUC(8, MSG_VECTOR, position);
	MSG_STRUC(9, MSG_VECTOR, MoveDirection);
	MSG_BLOBW(10, tBuffer, MAX_MISSILE_STATS_SIZE)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_MISSILENEWONUNIT)
	MSG_FIELD(0, int, nClass, 0)
	MSG_FIELD(1, UNITID, id, INVALID_ID)
	MSG_FIELD(2, UNITID, idSource, INVALID_ID)
	MSG_FIELD(3, UNITID, idWeapon0, INVALID_ID)
	MSG_FIELD(4, UNITID, idWeapon1, INVALID_ID)
	MSG_FIELD(5, int, nTeam, 0)
	MSG_FIELD(6, WORD, nSkill, 0)
	MSG_FIELD(7, ROOMID, idRoom, INVALID_ID)
	MSG_FIELD(8, UNITID, idTarget, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UPDATE)
	MSG_FIELD(0, DWORD, dwGameTick, 0)
	END_MSG_STRUCT

enum UNITTURN_FLAGS
{
	UT_IMMEDIATE,
	UT_FACEAWAY,
};

DEF_MSG_STRUCT(MSG_SCMD_UNITTURNID)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, UNITID, targetid, INVALID_ID)
	MSG_FIELD(2, BYTE, bFlags, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITTURNXYZ)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_STRUC(1, MSG_NVECTOR, vFaceDirection);
	MSG_STRUC(2, MSG_NVECTOR, vUpDirection);
	MSG_FIELD(3, BYTE, bImmediate, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITDIE)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, UNITID, idKiller, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_ROOM_ADD)
	MSG_FIELD(0, int, idRoomDef, 0)
	MSG_FIELD(1, DWORD, idRoom, 0)
	MSG_STRUC(2, MSG_MATRIX, mTranslation)
	MSG_FIELD(3, DWORD, dwRoomSeed, 0)
	MSG_FIELD(4, int, idSubLevel, INVALID_ID)
	MSG_CHAR (5, szLayoutOverride, 32)
	MSG_FIELD(6, BYTE, bRegion, 0)		//tugboat
	MSG_FIELD(7, int, bKnownOnServer, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_ROOM_ADD_COMPRESSED) //Putting frequently repeating fields first for easier sequential compression.
	MSG_FIELD(0, int, idSubLevel, INVALID_ID)
	MSG_FIELD(1, BYTE, mOrientation, 0)
	MSG_FIELD(2, int, idRoomDef, 0)
	MSG_FIELD(3, DWORD, idRoom, 0)
	MSG_CHAR (4, szLayoutOverride, 32)
	MSG_FLOAT(5, float, fXPosition, 0.0f)
	MSG_FLOAT(6, float, fYPosition, 0.0f)
	MSG_FLOAT(7, float, fZPosition, 0.0f) //may be unnecessary in tugboat, as it is 2d.
	MSG_FIELD(8, DWORD, dwRoomSeed, 0)
	MSG_FIELD(9, BYTE, bRegion, 0)			//tugboat
	MSG_FIELD(10, int, bKnownOnServer, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_ROOM_UNITS_ADDED)
	MSG_FIELD(0, DWORD, idRoom, 0)
	END_MSG_STRUCT
	
DEF_MSG_STRUCT(MSG_SCMD_ROOM_REMOVE)
	MSG_FIELD(0, ROOMID, idRoom, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_LOADCOMPLETE)
	MSG_FIELD(0, DWORD, nLevelDefinition, INVALID_LINK)
	MSG_FIELD(1, DWORD, nDRLGDefinition, INVALID_LINK)
	MSG_FIELD(2, DWORD, nLevelId, INVALID_LINK)
	MSG_FIELD(3, int, nLevelDepth, INVALID_LINK)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNIT_NEW)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, BYTE, bSequence, 0)
	MSG_FIELD(2, DWORD, dwTotalDataSize, 0)
	MSG_BLOBW(3, pData, MAX_NEW_UNIT_BUFFER)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNIT_REMOVE)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, BYTE, flags, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNIT_DAMAGED)
	MSG_FIELD(0, UNITID, idDefender, INVALID_ID)
	MSG_FIELD(1, UNITID, idAttacker, INVALID_ID)
	MSG_FIELD(2, int, nCurHp, 0)
	MSG_FIELD(3, int, nSourceX, 0)
	MSG_FIELD(4, int, nSourceY, 0)
	MSG_FIELD(5, BYTE, bResult, 0)
	MSG_FIELD(6, WORD, wHitState, 0)
	MSG_FIELD(7, BYTE, bIsMelee, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITSETSTAT)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, DWORD, dwStat, 0)
	MSG_FIELD(2, int, nValue, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITSETSTATE)
	MSG_FIELD(0, UNITID, idTarget, INVALID_ID)
	MSG_FIELD(1, UNITID, idSource, INVALID_ID)
	MSG_FIELD(2, WORD, wState, 0)
	MSG_FIELD(3, WORD, wStateIndex, 0)
	MSG_FIELD(4, int, nDuration, 0)
	MSG_FIELD(5, short, wSkill, -1)
	MSG_FIELD(6, BYTE, bDontExecuteSkillStats, false) // bool
	MSG_FIELD(7, DWORD, dwStatsListId, INVALID_ID)
	MSG_BLOBW(8, tBuffer, MAX_STATE_STATS_BUFFER)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITCLEARSTATE)
	MSG_FIELD(0, UNITID, idTarget, INVALID_ID)
	MSG_FIELD(1, UNITID, idSource, INVALID_ID)
	MSG_FIELD(2, WORD, wState, 0)
	MSG_FIELD(3, WORD, wStateIndex, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNIT_SET_FLAG)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, int, nFlag, 0)
	MSG_FIELD(2, BYTE, bValue, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITWARP)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, ROOMID, idRoom, INVALID_ID)
	MSG_STRUC(2, MSG_VECTOR, vPosition)
	MSG_STRUC(3, MSG_NVECTOR, vFaceDirection)
	MSG_STRUC(4, MSG_NVECTOR, vUpDirection)
	MSG_FIELD(5, DWORD, dwUnitWarpFlags, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNIT_PATH_POSITION_UPDATE)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, ROOMID, idPathRoom, INVALID_ID)
	MSG_FIELD(2, int, nPathIndex, -1)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_INVENTORY_LINK_TO)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, UNITID, idContainer, INVALID_ID)
	MSG_STRUC(2, INVENTORY_LOCATION, tLocation)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITDROP)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, ROOMID, idRoom, INVALID_ID)
	MSG_STRUC(2, MSG_VECTOR, vPosition);
	MSG_STRUC(3, MSG_VECTOR, vNewPosition);
	MSG_STRUC(4, MSG_NVECTOR, vFaceDirection);
	MSG_STRUC(5, MSG_NVECTOR, vUpDirection);
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_REMOVE_ITEM_FROM_INV)
	MSG_FIELD(0, UNITID, idItem, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CHANGE_INV_LOCATION)
	MSG_FIELD(0, UNITID, idContainer, INVALID_ID)
	MSG_FIELD(1, UNITID, idItem, INVALID_ID)
	MSG_STRUC(2, INVENTORY_LOCATION, tLocation)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SWAP_INV_LOCATION)
	MSG_FIELD(0, UNITID, idItemSource, INVALID_ID)
	MSG_STRUC(1, INVENTORY_LOCATION, tNewSourceLocation)
	MSG_FIELD(2, UNITID, idItemDest, INVALID_ID)
	MSG_STRUC(3, INVENTORY_LOCATION, tNewDestLocation)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_INV_ITEM_UNUSABLE)
	MSG_FIELD(0, UNITID, idItem, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_INV_ITEM_USABLE)
	MSG_FIELD(0, UNITID, idItem, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_INVENTORY_ADD_PET)
	MSG_FIELD(0, UNITID, idOwner, INVALID_ID)
	MSG_FIELD(1, UNITID, idPet, INVALID_ID)
	MSG_STRUC(2, INVENTORY_LOCATION, tLocation)		
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_INVENTORY_REMOVE_PET)
	MSG_FIELD(0, UNITID, idOwner, INVALID_ID)
	MSG_FIELD(1, UNITID, idPet, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_HOTKEY)
	MSG_ARRAY(0, UNITID, idItem, MAX_HOTKEY_ITEMS)
	MSG_ARRAY(1, int, nSkillID, MAX_HOTKEY_SKILLS)
	MSG_FIELD(2, BYTE, bHotkey, 0)
	MSG_FIELD(3, int, nLastUnittype, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CLEAR_INV_WAITING)
	MSG_FIELD(0, UNITID, idItem, INVALID_ID)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITMODE)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, int, mode, 0)
	MSG_FIELD(2, WORD, wParam, 0)
	MSG_FLOAT(3, float, duration, 0)
	MSG_STRUC(4, MSG_VECTOR, position)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITMODEEND)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, int, mode, 0)
	MSG_STRUC(2, MSG_VECTOR, position)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITPOSITION)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, BYTE, bFlags, 0)
	MSG_STRUC(2, MSG_VECTOR, TargetPosition)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITMOVEXYZ)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, BYTE, bFlags, 0)
	MSG_FIELD(2, int, mode, 0)
	MSG_FLOAT(3, float, fVelocity, 0.0f)
	MSG_FLOAT(4, float, fStopDistance, 0.0f)
	MSG_STRUC(5, MSG_VECTOR, TargetPosition)
	MSG_STRUC(6, MSG_NVECTOR, MoveDirection)
	MSG_FIELD(7, UNITID, idFaceTarget, INVALID_ID)
	MSG_BLOBW(8, buf, MAX_PATH_BUFFER)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITMOVEID)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, BYTE, bFlags, 0)
	MSG_FIELD(2, int, mode, 0)
	MSG_FLOAT(3, float, fVelocity, 0.0f)
	MSG_FLOAT(4, float, fStopDistance, 0.0f)
	MSG_FIELD(5, UNITID, idTarget, INVALID_ID)
	MSG_BLOBW(6, buf, MAX_PATH_BUFFER)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITMOVEDIRECTION)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, BYTE, bFlags, 0)
	MSG_FIELD(2, int, mode, 0)
	MSG_FLOAT(3, float, fVelocity, 0.0f)
	MSG_FLOAT(4, float, fStopDistance, 0.0f)
	MSG_STRUC(5, MSG_NVECTOR, MoveDirection)
	MSG_BLOBW(6, buf, MAX_PATH_BUFFER)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITPATHCHANGE)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_BLOBW(1, buf, MAX_PATH_BUFFER)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITJUMP)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FLOAT(1, float, fZVelocity, 0.0f)
	MSG_FIELD(2, BYTE, bRemoveFromStepList, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITWARDROBE)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_BLOBW(1, buf, MAX_WARDROBE_BUFFER)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITADDIMPACT)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, WORD, wFlags, 0)
	MSG_STRUC(2, MSG_VECTOR, vSource)
	MSG_STRUC(3, MSG_NVECTOR, vDirection)
	MSG_FLOAT(4, float, fForce, 0.0f)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CRAFT_BREAKDOWN)
	MSG_FIELD(0, UNITID, classID0, INVALID_ID)	
	MSG_FIELD(1, UNITID, classID1, INVALID_ID)
	MSG_FIELD(2, UNITID, classID2, INVALID_ID)
	MSG_FIELD(3, UNITID, classID3, INVALID_ID)
	MSG_FIELD(4, UNITID, classID4, INVALID_ID)	
	END_MSG_STRUCT
	
DEF_MSG_STRUCT(MSG_SCMD_UNITPLAYSOUND)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, DWORD, idSound, INVALID_ID)
	END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_SCMD_AUDIO_DIALOG_PLAY)
	MSG_FIELD(0, UNITID, idSource, INVALID_ID)
	MSG_FIELD(1, DWORD, nSound, INVALID_LINK)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SKILLSTART)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, short, wSkillId, -1)
	MSG_FIELD(2, UNITID, idTarget, INVALID_ID)
	MSG_STRUC(3, MSG_VECTOR, vTarget)
	MSG_FIELD(4, DWORD, dwStartFlags, 0)
	MSG_FIELD(5, DWORD, dwSkillSeed, 0)
	MSG_STRUC(6, MSG_VECTOR, vUnitPosition)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SKILLSTARTXYZ)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, short, wSkillId, -1)
	MSG_STRUC(2, MSG_VECTOR, vTarget)
	MSG_FIELD(3, DWORD, dwStartFlags, 0)
	MSG_FIELD(4, DWORD, dwSkillSeed, 0)
	MSG_STRUC(5, MSG_VECTOR, vUnitPosition)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SKILLSTARTID)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, short, wSkillId, -1)
	MSG_FIELD(2, UNITID, idTarget, INVALID_ID)
	MSG_FIELD(3, DWORD, dwStartFlags, 0)
	MSG_FIELD(4, DWORD, dwSkillSeed, 0)
	MSG_STRUC(5, MSG_VECTOR, vUnitPosition)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SKILLCHANGETARGETID)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, short, wSkill, -1)
	MSG_FIELD(2, UNITID, idTarget, INVALID_ID)
	MSG_FIELD(3, UNITID, idWeapon, INVALID_ID)
	MSG_FIELD(4, BYTE, bIndex, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SKILLSTOP)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, short, wSkillId, -1)
	MSG_FIELD(2, BYTE, bRequest, -1)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SKILLCOOLDOWN)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, UNITID, idWeapon, INVALID_ID)
	MSG_FIELD(2, short, wSkillId, -1)
	MSG_FIELD(3, short, wTicks, -1)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SKILLCHANGETARGETLOC)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, short, wSkillId, -1)
	MSG_FIELD(2, int, nTargetIndex, 0)
	MSG_STRUC(3, MSG_VECTOR, vTarget)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SKILLCHANGETARGETLOCMULTI)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, short, wSkillId, -1)
	MSG_FIELD(2, int, nLocationCount, 1)
	MSG_STRUC(3, MSG_VECTOR, vTarget)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SYS_TEXT)
	MSG_FIELD(0, DWORD, dwColor, INVALID_ID)
	MSG_WCHAR(1, str, MAX_CHEAT_LEN)
	MSG_FIELD(2, BYTE, bConsoleOnly, TRUE)
	END_MSG_STRUCT
	
DEF_MSG_STRUCT(MSG_SCMD_UISHOWMESSAGE)
	MSG_FIELD(0, BYTE, bType, 0)
	MSG_FIELD(1, int, nDialog, INVALID_ID)
	MSG_FIELD(2, int, nParam1, 0)
	MSG_FIELD(3, int, nParam2, 0)
	MSG_FIELD(4, BYTE, bFlags, 0)
	END_MSG_STRUCT
	
DEF_MSG_STRUCT(MSG_SCMD_STATEADDTARGET)
	MSG_FIELD(0, UNITID, idStateSource, INVALID_ID)
	MSG_FIELD(1, UNITID, idStateHolder, INVALID_ID)
	MSG_FIELD(2, UNITID, idTarget, INVALID_ID)
	MSG_FIELD(3, WORD, wState, 0)
	MSG_FIELD(4, WORD, wStateIndex, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_STATEREMOVETARGET)
	MSG_FIELD(0, UNITID, idStateSource, INVALID_ID)
	MSG_FIELD(1, UNITID, idStateHolder, INVALID_ID)
	MSG_FIELD(2, UNITID, idTarget, INVALID_ID)
	MSG_FIELD(3, WORD, wState, 0)
	MSG_FIELD(4, WORD, wStateIndex, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_STATEADDTARGETLOCATION)
	MSG_FIELD(0, UNITID, idStateSource, INVALID_ID)
	MSG_FIELD(1, UNITID, idStateHolder, INVALID_ID)
	MSG_STRUC(2, MSG_VECTOR, vTarget)
	MSG_FIELD(3, WORD, wState, 0)
	MSG_FIELD(4, WORD, wStateIndex, 0)
	END_MSG_STRUCT

/*
DEF_MSG_STRUCT(MSG_SCMD_QUESTEVENT)
	MSG_FIELD(0, UNITID, idSource, INVALID_ID)
	MSG_STRUC(1, MSG_VECTOR, vPosition)
	MSG_FIELD(2, BYTE, byQuestEventId, 0)
	END_MSG_STRUCT
*/

DEF_MSG_STRUCT(MSG_SCMD_PLAYERMOVE)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_BLOBB(1, buffer, MAX_REQMOVE_MSG_BUFFER_SIZE)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITSETOWNER)
	MSG_FIELD(0, UNITID, idSource, INVALID_ID)
	MSG_FIELD(1, UNITID, idOwner, INVALID_ID)
	MSG_FIELD(2, BYTE, bChangeTargetType, 0)
	END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_TASK_REWARD_TAKEN)
	MSG_FIELD(0, GAMETASKID, idTask, GAMETASKID_INVALID)		// task instance id on the server
	MSG_FIELD(1, UNITID, idReward, INVALID_ID)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_TASK_EXTERMINATED_COUNT)
	MSG_FIELD(0, GAMETASKID, idTask, GAMETASKID_INVALID)		// task instance id on the server
	MSG_FIELD(1, int, nCount, 0)		// new extermination count
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(FACTION_REWARD)
	MSG_FIELD(0, int, nFaction, INVALID_LINK)	// index of faction row
	MSG_FIELD(1, int, nPoints, 0)				// how many points the faction score will be changed by (+/-)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(FACTION_REWARDS)
	MSG_STARR(0, FACTION_REWARD, tReward, MAX_TASK_FACTION_REWARDS)
	MSG_FIELD(1, int, nCount, 0)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(TASK_COLLECT)
	MSG_FIELD(0, int, nItemClass, INVALID_LINK)		// index of row in item
	MSG_FIELD(1, int, nCount, 0)			// how many of them are desired
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(PENDING_UNIT_TARGET)
	MSG_FIELD(0, DWORD, speciesTarget, SPECIES_NONE)					// target species
	MSG_FIELD(1, int, nTargetUniqueName, MONSTER_UNIQUE_NAME_INVALID)	// unique name
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(TASK_EXTERMINATE)
	MSG_FIELD(0, DWORD, speciesTarget, SPECIES_NONE)	// what to exterminate
	MSG_FIELD(1, int, nCount, 0)						// how many must be exterminated
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(TASK_TEMPLATE)

	MSG_FIELD(0, GAMETASKID, idTask, GAMETASKID_INVALID)		// task instance id on the server
	MSG_FIELD(1, WORD, wCode, TASKCODE_INVALID)			// code of task in tasks.xls 
	MSG_FIELD(2, int, nLevelDefinition, INVALID_LINK)	// index in levels.xls of level area task is in
	MSG_FIELD(3, DWORD, speciesGiver, SPECIES_NONE)		// who gave this task
	MSG_FIELD(4, DWORD, speciesRewarder, SPECIES_NONE)	// who has the reward for this task

	// target units, rewards, etc
	MSG_ARRAY(5, UNITID, idRewards, MAX_TASK_REWARDS)	// rewards
	MSG_FIELD(6, int, nNumRewardsTaken, 0)				// rewards taken
	MSG_STRUC(7, FACTION_REWARDS, tFactionRewards)		// faction rewards
	MSG_STARR(8, TASK_COLLECT, tTaskCollect, MAX_TASK_COLLECT_TYPES)				// things to collect
	MSG_STARR(9, PENDING_UNIT_TARGET, tPendingUnitTargets, MAX_TASK_UNIT_TARGETS)	// task targets
	MSG_FIELD(10, int, nTimeLimitInMinutes, 0)			// time limit
	MSG_STARR(11, TASK_EXTERMINATE, tExterminate, MAX_TASK_EXTERMINATE)		// exterminate targets
	MSG_FIELD(12, int, nExterminatedCount, 0)			// count of things that have been "exterminated"
	MSG_FIELD(13, int, nTriggerPercent, 0)				// percent of level to explore
	MSG_FIELD(14, int, nLevelDepth, 0)					// depth of level area task is in
	MSG_FIELD(15, int, nGoldReward, 0)					// gold reward
	MSG_FIELD(16, int, nExperienceReward, 0)			// xp reward
	MSG_FIELD(17, int, nLevelArea, INVALID_LINK)		// level area task is in
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_AVAILABLE_TASKS)
	MSG_STARR(0, TASK_TEMPLATE, tTaskTemplate, MAX_AVAILABLE_TASKS_AT_GIVER)
	MSG_FIELD(1, UNITID, idTaskGiver, INVALID_ID)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_AVAILABLE_QUESTS)
	MSG_FIELD(0, int, nQuest, INVALID_ID)
	MSG_FIELD(1, UNITID, idQuestGiver, INVALID_ID)
	MSG_FIELD(2, int, nDialog, INVALID_ID)
	MSG_FIELD(3, int, nQuestTask, INVALID_ID)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_RECIPE_LIST_OPEN)
	MSG_FIELD(0, int, nRecipeList, INVALID_ID)
	MSG_FIELD(1, UNITID, idRecipeGiver, INVALID_ID)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_RECIPE_OPEN)
	MSG_FIELD(0, int, nRecipe, INVALID_ID)
	MSG_FIELD(1, UNITID, idRecipeGiver, INVALID_ID)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_RECIPE_CREATED)
	MSG_FIELD(0, int, nRecipe, INVALID_LINK)
	MSG_FIELD(1, BYTE, bSuccess, 1 )
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_RECIPE_FORCE_CLOSED)
	MSG_FIELD(0, UNITID, idRecipeGiver, INVALID_ID)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_TASK_STATUS)
	MSG_FIELD(0, GAMETASKID, idTask, 0)
	MSG_FIELD(1, int, eStatus, 0)
END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_SCMD_ANCHOR_MARKER)
MSG_FIELD(0, int, nLevelId, -1)
MSG_FIELD(1, int, nUnitId, -1)	
MSG_FIELD(2, int, nBaseRow, -1)	
MSG_FIELD(3, float, fX, 0.0f )
MSG_FIELD(4, float, fY, 0.0f )
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_POINT_OF_INTEREST)
MSG_FIELD(0, int, nFlags, 0)
MSG_FIELD(1, int, nUnitId, -1)	
MSG_FIELD(2, int, nX, 0)	
MSG_FIELD(3, int, nY, 0)	
MSG_FIELD(4, BYTE, nMessageType, 0)
MSG_FIELD(5, int, nClass, -1 )
END_MSG_STRUCT


//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_QUEST_TASK_STATUS)
	MSG_FIELD(0, int, nQuestIndex, -1)
	MSG_FIELD(1, int, nTaskIndex, -1)	
	MSG_FIELD(2, int, nMessage, 0)
	MSG_FIELD(3, int, nUnitID, -1 )
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_LEVELAREA_MAP_MESSAGE)
	MSG_FIELD(0, UNITID, nMapID, -1)
	MSG_FIELD(1, int, nMessage, 0)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_TASK_OFFER_REWARD)
	MSG_FIELD(0, UNITID, idTaskGiver, 0)
	MSG_FIELD(1, GAMETASKID, idTask, 0)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_TASK_CLOSE)
	MSG_FIELD(0, GAMETASKID, idTask, 0)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_TASK_RESTORE)
	MSG_STRUC(0, TASK_TEMPLATE, tTaskTemplate)
	MSG_FIELD(1, int, eStatus, -1)  // TASK_STATUS_INVALID
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_TASK_RESTRICTED_BY_FACTION)
	MSG_FIELD(0, UNITID, idGiver, INVALID_ID)
	MSG_ARRAY(1, int, nFactionBad, FACTION_MAX_FACTIONS)	
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_TASK_INCOMPLETE)
	MSG_FIELD(0, GAMETASKID, idTask, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_ENTER_IDENTIFY)
	MSG_FIELD(0, UNITID, idAnalyzer, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CANNOT_USE)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, int, eReason, -1)	// see USE_RESULT
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CANNOT_PICKUP)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, int, eReason, -1)	// see PICKUP_RESULT
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CANNOT_DROP)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, int, eReason, -1)	// see DROP_RESULT
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_HEADSTONE_INFO)
	MSG_FIELD(0, UNITID, idAsker, INVALID_ID)
	MSG_FIELD(1, BYTE, bCanReturnNow, 0)
	MSG_FIELD(2, int, nCost, 0)
	MSG_FIELD(3, int, nLevelDefDest, 0)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DUNGEON_WARP_INFO)		// tugboat
	MSG_FIELD(0, UNITID, idWarper, INVALID_ID)
	MSG_FIELD(1, int, nCost, 0)
	MSG_FIELD(2, int, nDepth, 0)	
	MSG_FIELD(3, int, nCostRealWorld, 0)
END_MSG_STRUCT


DEF_MSG_STRUCT(MSG_SCMD_CREATEGUILD_INFO)		// tugboat
MSG_FIELD(0, UNITID, idGuildmaster, INVALID_ID)
MSG_FIELD(1, int, nCost, 0)
MSG_FIELD(2, int, nCostRealWorld, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_RESPEC_INFO)		// tugboat
MSG_FIELD(0, UNITID, idRespeccer, INVALID_ID)
MSG_FIELD(1, int, nCost, 0)
MSG_FIELD(2, int, nCostRealWorld, 0)
MSG_FIELD(3, int, nCrafting, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRANSPORTER_WARP_INFO)		
MSG_FIELD(0, UNITID, idWarper, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PVP_SIGNER_UPPER_INFO)		
	MSG_FIELD(0, UNITID, idPvPSignerUpper, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(HIRELING_CHOICE)
	MSG_FIELD(0, int, nCost, 0)	
	MSG_FIELD(1, int, nMonsterClass, 0)
	MSG_FIELD(2, int, nRealWorldCost, 0)
END_MSG_STRUCT

#define MAX_HIRELING_MESSAGE 6 // aribrary

DEF_MSG_STRUCT(MSG_SCMD_HIRELING_SELECTION)
	MSG_FIELD(0, UNITID, idForeman, INVALID_ID)
	MSG_STARR(1, HIRELING_CHOICE, tHirelings, MAX_HIRELING_MESSAGE)	
	MSG_FIELD(2, int, nNumHirelings, 0)
	MSG_FIELD(3, BYTE, bSufficientFunds, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TALK_START)
	MSG_FIELD(0, UNITID, idTalkingTo, INVALID_ID)
	MSG_FIELD(1, int, nDialogType, 0)
	MSG_FIELD(2, int, nDialog, INVALID_LINK)
	MSG_FIELD(3, int, nDialogReward, INVALID_LINK)
	MSG_FIELD(4, int, nQuestId, INVALID_ID)
	MSG_FIELD(5, int, nGISecondNPC, -1)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_EXAMINE)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_INSPECT)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_VIDEO_START)
	MSG_FIELD(0, int, nGITalkingTo, -1)
	MSG_FIELD(1, int, nGITalkingTo2, -1)
	MSG_FIELD(2, int, nDialogType, 0)
	MSG_FIELD(3, int, nDialog, INVALID_LINK)
	MSG_FIELD(4, int, nDialogReward, INVALID_LINK)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_EMOTE_START)
	MSG_FIELD(0, int, nDialog, INVALID_LINK)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_INTERACT_INFO)
	MSG_FIELD(0, UNITID, idSubject, INVALID_ID)
	MSG_FIELD(1, BYTE, bInfo, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_LEVELMARKER)
	MSG_FIELD(0, int, nObjectClass, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_QUEST_NPC_UPDATE)
	MSG_FIELD(0, UNITID, idNPC, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_QUEST_STATE)
	MSG_FIELD(0, int, nQuest, 0)
	MSG_FIELD(1, int, nQuestState, 0)
	MSG_FIELD(2, int, nValue, 0)
	MSG_FIELD(3, BYTE, bRestore, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_QUEST_STATUS)
	MSG_FIELD(0, int, nQuest, 0)
	MSG_FIELD(1, int, nStatus, 0)
	MSG_FIELD(2, BYTE, bRestore, 0)
	MSG_FIELD(3, int, nDifficulty, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_END_QUEST_SETUP)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PLAY_MOVIELIST)
	MSG_FIELD(0, int, nMovieListIndex, 0)
	END_MSG_STRUCT

enum QUEST_TEMPLATE_MESSAGE_CONSTANTS
{
	MAX_QUEST_TEMPLATE_UNIT_TARGETS = 3,			// max quest unit targets (increase this and you need to add stats to save the targets)
	MAX_QUEST_TEMPLATE_COLLECT_TYPES = 3,		// when collecting stuff, this is what is needed (increase this and you must also add stats to save collect items)
};

//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(SPAWNABLE_UNIT_TARGET)
	MSG_FIELD(0, DWORD, speciesTarget, SPECIES_NONE)					// target species
	MSG_FIELD(1, int, nTargetUniqueName, MONSTER_UNIQUE_NAME_INVALID)	// unique name
	END_MSG_STRUCT

DEF_MSG_SUBSTRUCT(QUEST_TEMPLATE)
	MSG_STARR(0, SPAWNABLE_UNIT_TARGET, tSpawnableUnitTargets, MAX_QUEST_TEMPLATE_UNIT_TARGETS)	// quest targets
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_QUEST_TEMPLATE_RESTORE)
	MSG_FIELD(0, int, nQuest, 0)
	MSG_FIELD(1, UNITID, idPlayer, INVALID_ID)
	MSG_STRUC(2, QUEST_TEMPLATE, tQuestTemplate)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_QUEST_DISPLAY_DIALOG)
	MSG_FIELD(0, int, nQuest, 0)
	MSG_STRUC(1, QUEST_TEMPLATE, tQuestTemplate)
	MSG_FIELD(2, UNITID, idQuestGiver, INVALID_ID)
	MSG_FIELD(3, int, nDialog, INVALID_LINK)
	MSG_FIELD(4, int, nDialogReward, INVALID_LINK)
	MSG_FIELD(5, int, nGossipString, INVALID_LINK)
	MSG_FIELD(6, int, nDialogType, 2)	// NPC_DIALOG_OK_CANCEL
	MSG_FIELD(7, int, nGISecondNPC, -1)
	MSG_FIELD(8, BYTE, bLeaveUIOpenOnOk, FALSE)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_OPERATE_RESULT)
	MSG_FIELD(0, UNITID, idToOperate, INVALID_ID)
	MSG_FIELD(1, int, nResult, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_BLOCKING_STATE)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, int, nState, 0)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TUTORIAL)
	MSG_FIELD(0, int, nDialog, INVALID_LINK)
	END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TOGGLE_UI_ELEMENT)
	MSG_FIELD(0, int, nElement, INVALID_ID)
	MSG_FIELD(1, int, nAction, NONE)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SHOW_MAP_ATLAS)
	MSG_FIELD(0, int, nArea, INVALID_ID)	
	MSG_FIELD(1, int, nResult, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SHOW_RECIPE_PANE)
	MSG_FIELD(0, int, nRecipeID, INVALID_ID)		
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UPDATE_NUM_REWARD_ITEMS_TAKEN)
	MSG_FIELD(0, int, nNumTaken, 0)
	MSG_FIELD(1, int, nMaxTakes, -1)		// REWARD_TAKE_ALL
	MSG_FIELD(2, int, nOfferDefinition, INVALID_LINK)
END_MSG_STRUCT

DEF_MSG_SUBSTRUCT(TRADE_SPEC)
	MSG_FIELD(0, int, eStyle, -1) // see INVENTORY_STYLE
	MSG_FIELD(1, int, nOfferDefinition, INVALID_LINK)
	MSG_FIELD(2, int, nNumRewardTakes, -1)	// value of REWARD_TAKE_ALL
	MSG_FIELD(3, int, nRecipeList, INVALID_LINK)
	MSG_FIELD(4, int, nRecipe, INVALID_LINK)
	MSG_FIELD(5, BYTE, bNPCAllowsSimultaneousTrades, false) // bool
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRADE_START)
	MSG_FIELD(0, UNITID, idTradingWith, INVALID_ID)
	MSG_STRUC(1, TRADE_SPEC, tTradeSpec)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRADE_FORCE_CANCEL)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRADE_STATUS)
	MSG_FIELD(0, UNITID, idTrader, INVALID_ID)
	MSG_FIELD(1, int, nStatus, -1) // see TRADE_STATUS
	MSG_FIELD(2, int, nTradeOfferVersion, NONE) // STRADE_SESSION
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRADE_CANNOT_ACCEPT)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRADE_REQUEST_NEW)
	MSG_FIELD(0, UNITID, idToTradeWith, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRADE_REQUEST_NEW_CANCEL)
	MSG_FIELD(0, UNITID, idInitialRequester, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRADE_MONEY)
	MSG_FIELD(0, int, nMoneyYours, 0)
	MSG_FIELD(1, int, nMoneyTheirs, 0)
	MSG_FIELD(2, int, nMoneyRealWorldYours, 0)
	MSG_FIELD(3, int, nMoneyRealWorldTheirs, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TRADE_ERROR)
	MSG_FIELD(0, UNITID, idTradingWith, INVALID_ID)
	MSG_FIELD(1, int, nTradeError, 0)			// see TRADE_ERROR
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_WALLWALK_ORIENT)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_STRUC(1, MSG_NVECTOR, vFaceDirection)
	MSG_STRUC(2, MSG_NVECTOR, vUpDirection)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_EFFECT_AT_UNIT)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, int, eEffectType, -1)  //-1 = ET_INVALID
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_EFFECT_AT_LOCATION)
	MSG_STRUC(0, MSG_VECTOR, vPosition)
	MSG_FIELD(1, int, eEffectType, -1)	//-1 = ET_INVALID
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_OPERATE_WAYPOINT_MACHINE)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_OPERATE_RUNEGATE)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_JUMP_BEGIN)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_JUMP_END)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)	
	MSG_STRUC(1, MSG_VECTOR, vPosition)
END_MSG_STRUCT


//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_GAME_INSTANCE_UPDATE_BEGIN)
	MSG_FIELD(0, int, nInstanceListType, GAME_INSTANCE_NONE)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_GAME_INSTANCE_UPDATE_END)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_GAME_INSTANCE_UPDATE)
	MSG_FIELD(0, GAMEID, idGame, INVALID_ID )	
	MSG_FIELD(1, int, nInstanceNumber, INVALID_ID)	
	MSG_FIELD(2, int, idLevelDef, INVALID_ID)	
	MSG_FIELD(3, int, nInstanceType, GAME_INSTANCE_NONE)
	MSG_FIELD(4, int, nPVPWorld, INVALID_ID)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_TOWN_PORTAL_CHANGED)
	MSG_FIELD(0, BYTE, bIsOpening, FALSE)
	MSG_STRUC(1, TOWN_PORTAL_SPEC, tTownPortal)
	MSG_WCHAR(2, szName,	MAX_CHARACTER_NAME)
	MSG_FIELD(3, PGUID,		idPlayer, INVALID_ID)

END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_ENTER_TOWN_PORTAL)
	MSG_FIELD(0, UNITID, idPortal, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SELECT_RETURN_DESTINATION)
	MSG_FIELD(0, UNITID, idReturnWarp, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_REMOVING_FROM_RESERVED_GAME)
	MSG_FIELD(0, DWORD, dwDelayInTicks, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PARTY_MEMBER_INFO)
	MSG_FIELD(0, DWORD, idParty, INVALID_ID)
	MSG_WCHAR(1, szName,	MAX_CHARACTER_NAME)
	MSG_FIELD(2, int,		nPlayerUnitClass,	INVALID_LINK)
	MSG_FIELD(3, int,		nLevel, -1)
	MSG_FIELD(4, int,		nRank, -1)
	MSG_FIELD(5, PGUID,		idPlayer, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PARTY_ADVERTISE_RESULT)
	MSG_FIELD(0, int,		eResult,	0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PARTY_ACCEPT_INVITE_CONFIRM)
	MSG_FIELD(0, PGUID,		guidInviter,	INVALID_ID)
	MSG_FIELD(1, BYTE,		bAbandonGame,	false)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PARTY_JOIN_CONFIRM)
	MSG_FIELD(0, BYTE,		bAbandonGame,	false)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_MISSILES_CHANGEOWNER)
	MSG_FIELD(0, UNITID,	idUnit,		INVALID_ID)
	MSG_FIELD(1, UNITID,	idWeapon,	INVALID_ID)
	MSG_FIELD(2, int,		nMissileCount,	0)
	MSG_ARRAY(3, UNITID,	nMissiles,	30)	 // MAX_TARGETS_PER_QUERY
	MSG_FIELD(4, BYTE,		bFlags,		0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PING)
	MSG_FIELD(0, int, timeOfSend, INVALID_ID)
	MSG_FIELD(1, int, timeOfReceive, INVALID_ID)
	MSG_FIELD(2, int, timeStamp1, INVALID_ID)
	MSG_FIELD(3, int, timeStamp2, INVALID_ID)
	MSG_FIELD(4, BYTE, bIsReply, 0)
END_MSG_STRUCT

DEF_MSG_SUBSTRUCT(INTERACT_MENU_CHOICE)				// an array of these are requested from the client to the server and then used to display the radial menu
	MSG_FIELD(0, int, nInteraction,	INVALID_LINK)	// link to interact table of the interaction to do when the button is hit
	MSG_FIELD(1, BYTE, bDisabled, FALSE)			// whether the button should be displayed but disabled
	MSG_FIELD(2, int, nOverrideTooltipId, -1)		// override tool tip to be used in the case of disabled menu options
END_MSG_STRUCT

#define MAX_INTERACT_CHOICES (50)

DEF_MSG_STRUCT(MSG_SCMD_INTERACT_CHOICES)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, int, nCount, 0)
	MSG_STARR(2, INTERACT_MENU_CHOICE,	ptInteract,	MAX_INTERACT_CHOICES)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CONSOLE_MESSAGE)
	MSG_FIELD(0, int, nGlobalString, INVALID_LINK)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CONSOLE_DIALOG_MESSAGE)
	MSG_FIELD(0, int, nDialogID, INVALID_LINK)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DPS_INFO)
	MSG_FIELD(0, float,		fDPS,		0.0f)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_QUEST_RADAR)
	MSG_FIELD(0, BYTE, bCommand, 0)
	MSG_FIELD(1, UNITID, idUnit, INVALID_ID)
	MSG_STRUC(2, MSG_VECTOR, vPosition)
	MSG_FIELD(3, int, nType, 0)
	MSG_FIELD(4, DWORD, dwColor, 0xffffffff)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_RIDE_START)
	MSG_FIELD(0, UNITID, idUnit, INVALID_ID)
	MSG_FIELD(1, BYTE, bCommand, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_RIDE_END)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_MUSIC_INFO)
	MSG_FIELD(0, int,		nMonstersSpawned,		0)
	MSG_FIELD(1, int,		nMonstersLeft,			0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_ITEM_UPGRADED)
	MSG_FIELD(0, UNITID,	idItem,		INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_ITEM_AUGMENTED)
	MSG_FIELD(0, UNITID,	idItem,				INVALID_ID	)
	MSG_FIELD(1, int,		nAffixAugmented,	INVALID_LINK)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_REAL_MONEY_TXN_RESULT)
	MSG_FIELD(0, BYTE, bResult, false) // bool
	MSG_FIELD(1, int, iCurrencyBalance, 0)
	MSG_FIELD(2, int, iItemCode, 0)
	MSG_WCHAR(3, szItemDesc, 100)
END_MSG_STRUCT
	
DEF_MSG_STRUCT(MSG_SCMD_ITEMS_RESTORED)
	MSG_FIELD(0, int, nNumItemsRestored, 0 )
	MSG_FIELD(1, int, nNumItemsDropped, 0 )
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_WARP_RESTRICTED)
	MSG_FIELD(0, int, nReason, -1 )			// see WARP_RESTRICTED_REASON
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_CURSOR_ITEM_REFERENCE)
	MSG_FIELD(0, UNITID, idItem, INVALID_ID)
	MSG_FIELD(1, int,	nFromWeaponConfig, -1)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_QUEST_CLEAR_STATES)
	MSG_FIELD(0, int, nQuest, -1)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_SUBLEVEL_STATUS)
	MSG_FIELD(0, int, idLevel, INVALID_ID)
	MSG_FIELD(1, int, idSubLevel, INVALID_ID)
	MSG_FIELD(2, DWORD, dwSubLevelStatus, 0)
END_MSG_STRUCT

#define MAX_DBLOG_MESSAGE (128)

DEF_MSG_STRUCT(MSG_SCMD_DBUNIT_LOG)
	MSG_CHAR (0, szMessage, MAX_DBLOG_MESSAGE)
	MSG_FIELD(1, int, nOperation, -1)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DBUNIT_LOG_STATUS)
	MSG_FIELD(0, int, nEnabled, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNITSETSTATINLIST)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_FIELD(1, DWORD, dwStatsListId, INVALID_ID)
	MSG_FIELD(2, DWORD, dwStat, 0)
	MSG_FIELD(3, int, nValue, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_GUILD_ACTION_RESULT)
	MSG_FIELD(0, WORD, wActionType, (WORD)-1)
	MSG_FIELD(1, WORD, wChatErrorResultCode, (WORD)-1)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_UNIT_GUILD_ASSOCIATION)
	MSG_FIELD(0, UNITID, id, INVALID_ID)
	MSG_WCHAR(1, wszGuildName, MAX_GUILD_NAME)
	MSG_FIELD(2, BYTE, eGuildRank, 0)
	MSG_WCHAR(3, wszRankName, MAX_CHARACTER_NAME)
END_MSG_STRUCT

enum PVP_CONSTANT
{
	PVP_DISABLE_DELAY_IN_SECONDS = 5,
};

enum PVP_ACTION_TYPES
{
	PVP_ACTION_ENABLE,
	PVP_ACTION_DISABLE,
	PVP_ACTION_DISABLE_DELAY,
};

enum PVP_ACTION_ERROR_TYPES
{
	PVP_ACTION_ERROR_NONE,
	PVP_ACTION_ERROR_UNKNOWN,
	PVP_ACTION_ERROR_ALREADY_ENABLED,
	PVP_ACTION_ERROR_IN_A_MATCH,
};

DEF_MSG_STRUCT(MSG_SCMD_PVP_ACTION_RESULT)
	MSG_FIELD(0, WORD, wActionType, (WORD)-1)
	MSG_FIELD(1, WORD, wActionErrorType, (WORD)-1)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DUEL_INVITE)
	MSG_FIELD(0, UNITID, idOpponent, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DUEL_INVITE_DECLINE)
	MSG_FIELD(0, UNITID, idOpponent, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DUEL_INVITE_FAILED)
	MSG_FIELD(0, WORD, wFailReasonInviter, 0)
	MSG_FIELD(1, WORD, wFailReasonInvitee, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DUEL_INVITE_ACCEPT_FAILED)
	MSG_FIELD(0, WORD, wFailReasonInviter, 0)
	MSG_FIELD(1, WORD, wFailReasonInvitee, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DUEL_START)
	MSG_FIELD(0, WORD, wCountdownSeconds, (WORD)5)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_DUEL_MESSAGE)
MSG_FIELD(0, int, nMessage, -1)
MSG_FIELD(1, UNITID, idOpponent, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PVP_TEAM_START_COUNTDOWN)
	MSG_FIELD(0, WORD, wCountdownSeconds, (WORD)5)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PVP_TEAM_START_MATCH)
	MSG_FIELD(0, WORD, wMatchTimeSeconds, (WORD)600)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PVP_TEAM_END_MATCH)
	MSG_FIELD(0, WORD, wScoreTeam0, (WORD)0)
	MSG_FIELD(1, WORD, wScoreTeam1, (WORD)0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PVP_GAME_LIST_BEGIN)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PVP_GAME_LIST_END)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PVP_GAME_LIST_ADD)
	MSG_FIELD(0, GAMEID, idGame, INVALID_ID )	
	MSG_FIELD(1, int, nInstanceNumber, INVALID_ID)	
	MSG_FIELD(2, int, idLevelDef, INVALID_ID)	
	MSG_FIELD(3, int, nPVPWorld, INVALID_ID)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_GLOBAL_THEME_CHANGE)
	MSG_FIELD(0, WORD, wTheme, 0)
	MSG_FIELD(1, BYTE, bEnable, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_GLOBAL_LOCALIZED_ANNOUNCEMENT)
	MSG_STRUC(0, LOCALIZED_PARAM_STR, tLocalizedParamString)
END_MSG_STRUCT

//----------------------------------------------------------------------------
DEF_MSG_STRUCT(MSG_SCMD_CUBE_OPEN)
	MSG_FIELD(0, UNITID, idCube, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_BACKPACK_OPEN)
	MSG_FIELD(0, UNITID, idBackpack, INVALID_ID)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PLACE_FIELD)
	MSG_FIELD(0, UNITID, idOwner, INVALID_ID)
	MSG_FIELD(1, int, nFieldMissile, INVALID_ID)
	MSG_STRUC(2, MSG_VECTOR, vPosition)
	MSG_FIELD(3, int, nRadius, 0)
	MSG_FIELD(4, int, nDuration, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PLACE_FIELD_PATHNODE)
	MSG_FIELD(0, UNITID, idOwner, INVALID_ID)
	MSG_FIELD(1, int, nFieldMissile, INVALID_ID)
	MSG_FIELD(2, ROOMID, idRoom, INVALID_ID)
	MSG_FIELD(3, int, nPathNode, INVALID_ID)
	MSG_FIELD(4, int, nRadius, 0)
	MSG_FIELD(5, int, nDuration, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_PLAYED)
	MSG_FIELD(0, DWORD, dwPlayedTime, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_EMAIL_RESULT)
	MSG_FIELD(0, DWORD, actionType, INVALID_ID)
	MSG_FIELD(1, DWORD, actionResult, INVALID_ID)
	MSG_FIELD(2, BYTE,	actionSenderContext, 0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_EMAIL_METADATA)
	MSG_FIELD(0, ULONGLONG,					idEmailMessageId,			INVALID_GUID)
	MSG_FIELD(1, ULONGLONG,					idSenderCharacterId,		INVALID_GUID)
	MSG_WCHAR(2, wszSenderCharacterName,	MAX_CHARACTER_NAME)
	MSG_FIELD(3, BYTE,						eMessageStatus,				(BYTE)-1)
	MSG_FIELD(4, BYTE,						eMessageType,				(BYTE)-1)
	MSG_FIELD(5, ULONGLONG,					timeSentUTC,				0)
	MSG_FIELD(6, ULONGLONG,					timeOfManditoryDeletionUTC,	0)
	MSG_FIELD(7, BYTE,						bIsMarkedRead,				TRUE)
	MSG_FIELD(8, PGUID,						idAttachedItemId,			INVALID_GUID)
	MSG_FIELD(9, DWORD,						dwAttachedMoney,			0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_EMAIL_DATA)
	MSG_FIELD(0, ULONGLONG,					idEmailMessageId,			INVALID_GUID)
	MSG_WCHAR(1, wszEmailSubject,			MAX_EMAIL_SUBJECT)
	MSG_WCHAR(2, wszEmailBody,				MAX_EMAIL_BODY)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_EMAIL_UPDATE)
	MSG_FIELD(0, ULONGLONG,					idEmailMessageId,			INVALID_GUID)
	MSG_FIELD(1, ULONGLONG,					timeOfManditoryDeletionUTC,	0)
	MSG_FIELD(2, BYTE,						bIsMarkedRead,				TRUE)
	MSG_FIELD(3, PGUID,						idAttachedItemId,			INVALID_GUID)
	MSG_FIELD(4, DWORD,						dwAttachedMoney,			0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_AH_ERROR)
	MSG_FIELD(0, DWORD,						ErrorCode,					0)
	MSG_FIELD(1, PGUID,						ItemGUID,					INVALID_GUID	)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_AH_SEARCH_RESULT)
	MSG_FIELD(0, DWORD,						ResultSize,					0)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_AH_SEARCH_RESULT_NEXT)
	MSG_FIELD(0, DWORD,						ResultSize,					0							)
	MSG_FIELD(1, DWORD,						ResultCurIndex,				0							)
	MSG_FIELD(2, DWORD,						ResultCurCount,				0							)
	MSG_ARRAY(3, PGUID,						ItemGUIDs,					AUCTION_SEARCH_MAX_RESULT	)
	MSG_FIELD(4, BYTE,						ResultOwnItem,				FALSE						)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_AH_SEARCH_RESULT_ITEM_INFO)
	MSG_FIELD(0, PGUID,						ItemGUID,					INVALID_GUID				)
	MSG_FIELD(1, DWORD,						ItemPrice,					0							)
	MSG_WCHAR(2,							szSellerName,				MAX_CHARACTER_NAME			)	
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_AH_SEARCH_RESULT_ITEM_BLOB)
	MSG_FIELD(0, PGUID,						ItemGUID,					INVALID_GUID					)
	MSG_BLOBW(1,							ItemBlob,					DEFAULT_MAX_ITEM_BLOB_MSG_SIZE	)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_AH_INFO)
	MSG_FIELD(0, DWORD,						dwFeeRate,					0	)
	MSG_FIELD(1, DWORD,						dwMaxItemSaleCount,			0	)
END_MSG_STRUCT

DEF_MSG_STRUCT(MSG_SCMD_AH_PLAYER_ITEM_SALE_LIST)
	MSG_FIELD(0, DWORD,						ItemCount,					0								)
	MSG_ARRAY(1, PGUID,						ItemGUIDs,					AUCTION_MAX_ITEM_SALE_COUNT		)
END_MSG_STRUCT

//----------------------------------------------------------------------------
// FUNCTIONS - Server to Client
//----------------------------------------------------------------------------
struct NET_COMMAND_TABLE * s_NetGetCommandTable(
	void);

void s_NetCommandTableFree(
	void);

void s_SendMessageAppClient(
	APPCLIENTID idClient,
	BYTE command,
	MSG_STRUCT * msg);

void s_SendMessageNetClient(
	NETCLIENTID64 idNetClient,
	BYTE command,
	MSG_STRUCT * msg);

void s_SendMessage(
	GAME * game,
	GAMECLIENTID idClient,
	NET_CMD command,
	MSG_STRUCT * msg);

void s_SendMessageToAll(
	GAME * game,
	BYTE command,
	MSG_STRUCT * msg);

void s_SendMessageToAllWithRoom(
	GAME * game,
	struct ROOM * room,
	BYTE command,
	MSG_STRUCT * msg);

void s_SendMessageToAllWithRooms(
	GAME * game,
	struct ROOM ** roomarray,
	unsigned int roomcount,
	BYTE command,
	MSG_STRUCT * msg);

void s_SendMessageToOthers(
	GAME * game,
	GAMECLIENTID idClient,
	BYTE command,
	MSG_STRUCT * msg);

void s_SendMessageToOthersWithRoom(
	GAME * game,
	GAMECLIENTID idClient,
	struct ROOM * room,
	BYTE command,
	MSG_STRUCT * msg);

void s_SendUnitMessageToClient(
	UNIT * unit,
	GAMECLIENTID idClient,
	NET_CMD cmd,
	MSG_STRUCT * msg);

void s_SendUnitMessage(
	UNIT * unit,
	NET_CMD cmd,
	MSG_STRUCT * msg,
	GAMECLIENTID idClientExclude = INVALID_GAMECLIENTID);

void s_SendMultiUnitMessage(
	UNIT **pUnits,
	int nNumUnits,
	BYTE bCommand,
	MSG_STRUCT *pMessage);

void s_SendMessageToAllInLevel(
	GAME * pGame,
	struct LEVEL * pLevel,
	BYTE bCommand,
	MSG_STRUCT * pMessage );

void SendError(
	APPCLIENTID idClient,
	int nError);

void s_SendMessageClientArray(
	GAME * game,
	struct GAMECLIENT ** clients,
	unsigned int count,
	BYTE command,
	MSG_STRUCT * msg);
	
BOOL ClientKnowsRoom(
	struct GAMECLIENT * client,
	ROOM * room);

void s_SendGuildActionResult(
	NETCLIENTID64 idNetClient,
	WORD requestType,
	WORD requestResult );

void s_SendPvPActionResult(
	NETCLIENTID64 idNetClient,
	WORD wActionType, 
	WORD wActionErrorType );

//----------------------------------------------------------------------------
// FUNCTIONS - Message related
//----------------------------------------------------------------------------
BOOL CompressRoomMessage(const MSG_SCMD_ROOM_ADD &original,
						 MSG_SCMD_ROOM_ADD_COMPRESSED &compressed );

void UncompressRoomMessage(MSG_SCMD_ROOM_ADD &original,
						   const MSG_SCMD_ROOM_ADD_COMPRESSED &compressed );
	
#endif // _S_MESSAGE_H_


//----------------------------------------------------------------------------
// MESSAGE ENUMERATIONS  (Server to Client messages)
//----------------------------------------------------------------------------
#ifndef _S_MESSAGE_ENUM_H_
#define _S_MESSAGE_ENUM_H_


NET_MSG_TABLE_BEGIN
	// command												sendlog	recvlog
	NET_MSG_TABLE_DEF(SCMD_YOURID,							TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_ERROR,							TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_GAME_NEW,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_SET_LEVEL_COMMUNICATION_CHANNEL,	TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TEAM_NEW,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TEAM_DEACTIVATE,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TEAM_JOIN,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_GAME_CLOSE,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_OPEN_PLAYER_INITSEND,			TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_OPEN_PLAYER_SENDFILE,			TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_START_LEVEL_LOAD,				TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_SET_LEVEL,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_SET_CONTROL_UNIT,				TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PLAYERNEW,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PLAYERREMOVE,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_MISSILENEWXYZ,					TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_MISSILENEWONUNIT,				TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UPDATE,							FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNITTURNID,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITTURNXYZ,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITDIE,							TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_ROOM_ADD,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_ROOM_ADD_COMPRESSED,				TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_ROOM_UNITS_ADDED,				TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_ROOM_REMOVE,						TRUE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_LOADCOMPLETE,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNIT_NEW,						TRUE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_UNIT_REMOVE,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNIT_DAMAGED,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNITSETSTAT,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNITSETSTATE,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNITCLEARSTATE,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNIT_SET_FLAG,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNITWARP,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNITDROP,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNIT_PATH_POSITION_UPDATE,		FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_INVENTORY_LINK_TO,				TRUE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_REMOVE_ITEM_FROM_INV,			TRUE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_CHANGE_INV_LOCATION,				TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_SWAP_INV_LOCATION,				TRUE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_INV_ITEM_UNUSABLE,				TRUE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_INV_ITEM_USABLE,					TRUE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_INVENTORY_ADD_PET,				TRUE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_INVENTORY_REMOVE_PET,			TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_HOTKEY,							TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_CLEAR_INV_WAITING,				TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UNITMODE,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITMODEEND,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITPOSITION,					FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITMOVEXYZ,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITMOVEID,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITMOVEDIRECTION,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITPATHCHANGE,					FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITJUMP,						TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITWARDROBE,					TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITADDIMPACT,					TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SKILLSTART,						TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SKILLSTARTXYZ,					TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SKILLSTARTID,					TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SKILLCHANGETARGETID,				TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SKILLCHANGETARGETLOC,			TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SKILLCHANGETARGETLOCMULTI,		TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SKILLSTOP,						TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SKILLCOOLDOWN,					TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_SYS_TEXT,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UISHOWMESSAGE,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_STATEADDTARGET,					FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_STATEREMOVETARGET,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_STATEADDTARGETLOCATION,			FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_CRAFT_BREAKDOWN,					TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITPLAYSOUND,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AUDIO_DIALOG_PLAY,				FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_PLAYERMOVE,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITSETOWNER,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AVAILABLE_QUESTS,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_RECIPE_LIST_OPEN,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_RECIPE_OPEN,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_RECIPE_FORCE_CLOSED,				FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_RECIPE_CREATED,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AVAILABLE_TASKS,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TASK_STATUS,						FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_POINT_OF_INTEREST,					FALSE,	TRUE) //tugboat
	NET_MSG_TABLE_DEF(SCMD_ANCHOR_MARKER,					FALSE,	TRUE) //tugboat
	NET_MSG_TABLE_DEF(SCMD_QUEST_TASK_STATUS,				FALSE,	TRUE) //tugboat
	NET_MSG_TABLE_DEF(SCMD_LEVELAREA_MAP_MESSAGE,			FALSE,	TRUE) //tugboat	
	NET_MSG_TABLE_DEF(SCMD_TASK_OFFER_REWARD,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TASK_CLOSE,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TASK_RESTORE,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TASK_RESTRICTED_BY_FACTION,		FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TASK_INCOMPLETE,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_ENTER_IDENTIFY,					FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_CANNOT_USE,						FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_CANNOT_PICKUP,					FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_CANNOT_DROP,						FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_HEADSTONE_INFO,					FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_DUNGEON_WARP_INFO,				FALSE,	TRUE)	// tugboat
	NET_MSG_TABLE_DEF(SCMD_CREATEGUILD_INFO,				FALSE,	TRUE)	// tugboat
	NET_MSG_TABLE_DEF(SCMD_RESPEC_INFO,						FALSE,	TRUE)	// tugboat
	NET_MSG_TABLE_DEF(SCMD_TRANSPORTER_WARP_INFO,			FALSE,	TRUE)	// tugboat
	NET_MSG_TABLE_DEF(SCMD_PVP_SIGNER_UPPER_INFO,			FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_HIRELING_SELECTION,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TALK_START,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_EXAMINE,							FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_INSPECT,							FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_VIDEO_START,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_EMOTE_START,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_INTERACT_INFO,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_LEVELMARKER,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_QUEST_NPC_UPDATE,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_QUEST_STATE,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_QUEST_STATUS,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_END_QUEST_SETUP,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PLAY_MOVIELIST,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_QUEST_DISPLAY_DIALOG,			FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_QUEST_TEMPLATE_RESTORE,			FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_OPERATE_RESULT,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_BLOCKING_STATE,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TOGGLE_UI_ELEMENT,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_SHOW_MAP_ATLAS,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_SHOW_RECIPE_PANE,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TUTORIAL,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TASK_REWARD_TAKEN,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TASK_EXTERMINATED_COUNT,			FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_UPDATE_NUM_REWARD_ITEMS_TAKEN,	FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_TRADE_START,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TRADE_FORCE_CANCEL,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TRADE_STATUS,					FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_TRADE_CANNOT_ACCEPT,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TRADE_REQUEST_NEW,				FALSE,	TRUE)			
	NET_MSG_TABLE_DEF(SCMD_TRADE_REQUEST_NEW_CANCEL,		FALSE,	TRUE)			
	NET_MSG_TABLE_DEF(SCMD_TRADE_MONEY,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TRADE_ERROR,						FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_WALLWALK_ORIENT,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_EFFECT_AT_UNIT,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_EFFECT_AT_LOCATION,				FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_OPERATE_WAYPOINT_MACHINE,		FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_OPERATE_RUNEGATE,				FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_JUMP_BEGIN,						FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_JUMP_END,						FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_GAME_INSTANCE_UPDATE_BEGIN,		FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_GAME_INSTANCE_UPDATE_END,		FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_GAME_INSTANCE_UPDATE,			FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_TOWN_PORTAL_CHANGED,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_ENTER_TOWN_PORTAL,				FALSE,	TRUE)				
	NET_MSG_TABLE_DEF(SCMD_SELECT_RETURN_DESTINATION,		FALSE,	TRUE)			
	NET_MSG_TABLE_DEF(SCMD_REMOVING_FROM_RESERVED_GAME,		FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PARTY_MEMBER_INFO,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PARTY_ADVERTISE_RESULT,			FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PARTY_ACCEPT_INVITE_CONFIRM,		FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PARTY_JOIN_CONFIRM,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_MISSILES_CHANGEOWNER,			TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_CONTROLUNIT_FINISHED,			FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PING,							FALSE,	FALSE)	
	NET_MSG_TABLE_DEF(SCMD_INTERACT_CHOICES,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_CONSOLE_MESSAGE,					FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_CONSOLE_DIALOG_MESSAGE,			FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_DPS_INFO,						FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_QUEST_RADAR,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_RIDE_START,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_RIDE_END,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_MUSIC_INFO,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_ITEM_UPGRADED,					FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_ITEM_AUGMENTED,					FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_REAL_MONEY_TXN_RESULT,			FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_ITEMS_RESTORED,					FALSE,	TRUE)	
	NET_MSG_TABLE_DEF(SCMD_WARP_RESTRICTED,					FALSE,	TRUE)		
	NET_MSG_TABLE_DEF(SCMD_CURSOR_ITEM_REFERENCE,			TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_QUEST_CLEAR_STATES,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_SUBLEVEL_STATUS,					FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_DBUNIT_LOG,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_DBUNIT_LOG_STATUS,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNITSETSTATINLIST,				FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_GUILD_ACTION_RESULT,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_UNIT_GUILD_ASSOCIATION,			FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_PVP_ACTION_RESULT,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_DUEL_INVITE,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_DUEL_INVITE_DECLINE,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_DUEL_INVITE_FAILED,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_DUEL_INVITE_ACCEPT_FAILED,		FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_DUEL_START,						FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_DUEL_MESSAGE,					FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_PVP_TEAM_START_COUNTDOWN,		FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_PVP_TEAM_START_MATCH,			FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_PVP_TEAM_END_MATCH,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_GLOBAL_THEME_CHANGE,				FALSE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_CUBE_OPEN,						FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_BACKPACK_OPEN,					FALSE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_PLACE_FIELD,						TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_PLACE_FIELD_PATHNODE,			TRUE,	FALSE)
	NET_MSG_TABLE_DEF(SCMD_PLAYED,							TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_EMAIL_RESULT,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_EMAIL_METADATA,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_EMAIL_DATA,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_EMAIL_UPDATE,					TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_GLOBAL_LOCALIZED_ANNOUNCEMENT,	TRUE,	TRUE)

	// Messages from Auction Server
	NET_MSG_TABLE_DEF(SCMD_AH_INFO,							TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AH_ERROR,						TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AH_PLAYER_ITEM_SALE_LIST,		TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AH_SEARCH_RESULT,				TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AH_SEARCH_RESULT_NEXT,			TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AH_SEARCH_RESULT_ITEM_INFO,		TRUE,	TRUE)
	NET_MSG_TABLE_DEF(SCMD_AH_SEARCH_RESULT_ITEM_BLOB,		TRUE,	TRUE)
	
NET_MSG_TABLE_END(SCMD_LAST)


#endif // _S_MESSAGE_ENUM_H_

