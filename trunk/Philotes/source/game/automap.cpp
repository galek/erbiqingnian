//----------------------------------------------------------------------------
// FILE: automap.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "units.h"
#include "level.h"
#include "filepaths.h"
#include "e_automap.h"

using namespace FSSE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define AUTOMAP_SAVE_MAGIC_NUMBER		0x4155544f
#define AUTOMAP_SAVE_ROOM_MAGIC			0x524f4f4d
#define AUTOMAP_SAVE_VERSION			(3 + GLOBAL_FILE_VERSION)


//----------------------------------------------------------------------------
// automap save file
// directory: save\playername\
// filename:  lvl.map  ex.  lvl-001.map
// the lvluid is the first random # generated from the lvl seed, which
// the game passes to the client
// DWORD: magic number #		
// DWORD: version
// DWORD: uniqueid
// DWORD: # of rooms
// for each room::
//   DWORD: room id
//   DWORD: # of automap nodes
//   DATA:  array of [# of automap nodes] bits, set if node is AUTOMAP_VISITED
//----------------------------------------------------------------------------

inline BOOL sAutomapGetFileName( GAME *game,
								 LEVEL *level,
								 OS_PATH_CHAR *szFileName )
{
	ASSERT_RETFALSE( game );
	ASSERT_RETFALSE( level );
	// Don't save the automap for "safe" levels (like towns)
	if ( LevelIsSafe( level ) ||
		( AppIsTugboat() && LevelIsTown( level ) ) )
		return FALSE;

	UNIT * player = GameGetControlUnit(game);
	if ( ! player || ! player->szName )
		return FALSE;
	OS_PATH_CHAR szPlayerName[MAX_PATH];
	PStrCvt(szPlayerName, player->szName, _countof(szPlayerName));

	const OS_PATH_CHAR * pSavePath = FilePath_GetSystemPath(FILE_PATH_SAVE);

	OS_PATH_CHAR szSaveDirectory[MAX_PATH];
	PStrPrintf(szSaveDirectory, MAX_PATH, OS_PATH_TEXT("%s%s"), pSavePath, szPlayerName);
	if (!FileDirectoryExists(szSaveDirectory))
	{
		FileCreateDirectory(szSaveDirectory);
	}
	
	const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet( level );
	ASSERT_RETFALSE( pLevelDef );
	if( AppIsHellgate() )
	{
		PStrPrintf(szFileName, MAX_PATH, OS_PATH_TEXT("%s%s\\lvl-%d-%03d.map"), pSavePath, szPlayerName, UnitGetStat(player, STATS_DIFFICULTY_CURRENT), pLevelDef->wCode );
	}
	else //tugboat
	{
		PStrPrintf(szFileName, MAX_PATH, OS_PATH_TEXT("%s%s\\lvl-%d-%03d-%d.map"), pSavePath, szPlayerName, UnitGetStat(player, STATS_DIFFICULTY_CURRENT), pLevelDef->wCode, LevelGetDepth( level ) );
	}
	return TRUE;
}
								 

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_SaveAutomap(
	GAME * game,
	LEVELID idLevel)
{
	if ( c_GetToolMode() )
		return FALSE;
	
	ASSERT_RETFALSE(game && IS_CLIENT(game));

	if (idLevel == INVALID_ID)
	{
		return FALSE;
	}
	LEVEL * level = LevelGetByID(game, idLevel);
	if (!level)
	{
		return FALSE;
	}
	OS_PATH_CHAR szSaveFilename[MAX_PATH];
	if( !sAutomapGetFileName( game, level, szSaveFilename ) )
	{
		return FALSE;
	}

	WRITE_BUF_DYNAMIC fbuf;

	ASSERT_RETFALSE(fbuf.PushInt((DWORD)0));
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)AUTOMAP_SAVE_VERSION));
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)level->m_dwUID));
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)level->m_nDRLGDef));

#ifdef ENABLE_NEW_AUTOMAP

	V( e_AutomapSave( fbuf ) );

#endif	// ENABLE_NEW_AUTOMAP

	unsigned int end = fbuf.GetPos();
	ASSERT_RETFALSE(fbuf.SetPos(0));
	ASSERT_RETFALSE(fbuf.PushInt((DWORD)AUTOMAP_SAVE_MAGIC_NUMBER));
	ASSERT_RETFALSE(fbuf.SetPos(end));
	ASSERT_RETFALSE(fbuf.SaveNow(szSaveFilename));

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_LevelFreeAutomapInfo(
	GAME * game,
	LEVEL * level)
{
	ASSERT_RETURN(game && IS_CLIENT(game));

	// free automap stuff
	if (level->m_pAutoMapSave)
	{
		for (unsigned int ii = 0; ii < level->m_nAutoMapSaveCount; ++ii)
		{
			if (level->m_pAutoMapSave[ii].pNodeFlags)
			{
				GFREE(game, level->m_pAutoMapSave[ii].pNodeFlags);
			}
		}
		GFREE(game, level->m_pAutoMapSave);
		level->m_pAutoMapSave = NULL;
	}
	level->m_nAutoMapSaveCount = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_LoadAutomap(
	GAME * game,
	LEVELID idLevel)
{
	ASSERT_RETFALSE(game && IS_CLIENT(game));

	if (idLevel == INVALID_ID)
	{
		return FALSE;
	}
	LEVEL * level = LevelGetByID(game, idLevel);
	if (!level)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(level->m_pAutoMapSave == NULL);

	OS_PATH_CHAR szSaveFilename[MAX_PATH];
	if( !sAutomapGetFileName( game, level, szSaveFilename ) )
	{
		return FALSE;
	}

	// CHB 2007.01.11
#if 1
	DWORD nBytesRead = 0;
	BYTE * data = static_cast<BYTE *>(FileLoad(MEMORY_FUNC_FILELINE(NULL, szSaveFilename, &nBytesRead)));
	if ((data == 0) || (nBytesRead < sizeof(DWORD)))
	{
		return false;
	}
#else
	DECLARE_LOAD_SPEC(spec, szSaveFilename);
	spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	BYTE * data = (BYTE *)PakFileLoadNow(spec);
	if (!data || spec.bytesread < sizeof(DWORD))
	{
		return FALSE;
	}
#endif

	BOOL result = FALSE;
	ONCE
	{
		BYTE_BUF buf(data, nBytesRead);

		DWORD dw;
		ASSERT_BREAK(buf.PopUInt(&dw, sizeof(dw)) && dw == AUTOMAP_SAVE_MAGIC_NUMBER);
		// validate version
		ASSERT_BREAK(buf.PopUInt(&dw, sizeof(dw)));
		if (dw != AUTOMAP_SAVE_VERSION)
		{
			break;
		}
		// validate uid
		ASSERT_BREAK(buf.PopUInt(&dw, sizeof(dw)));
		if (dw != level->m_dwUID)
		{
			break;
		}
		// validate drlg def
		ASSERT_BREAK(buf.PopUInt(&dw, sizeof(dw)));
		if (dw != (DWORD)level->m_nDRLGDef)
		{
			break;
		}


#ifdef ENABLE_NEW_AUTOMAP

		V_BREAK( e_AutomapLoad( buf ) );
#endif	// ENABLE_NEW_AUTOMAP

		result = TRUE;
	}

	FREE(NULL, data);
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_RoomApplyAutomapData(
	GAME * game,
	LEVEL * level,
	ROOM * room)
{
	ASSERT_RETFALSE(game && IS_CLIENT(game));
	ASSERT_RETFALSE(level);
	ASSERT_RETFALSE(room);

	// look for the AUTOMAP_SAVE_NODE for this room
	DWORD idRoom = room->idRoom;
	AUTOMAP_SAVE_NODE * automap = NULL;
	for (unsigned int ii = 0; ii < level->m_nAutoMapSaveCount; ++ii)
	{
		if (level->m_pAutoMapSave[ii].idRoom == idRoom)
		{
			automap = &level->m_pAutoMapSave[ii];
			break;
		}
	}
	if (!automap)
	{
		return FALSE;
	}

	// count the number of automap nodes in the room
	unsigned int nodecount = 0;
	AUTOMAP_NODE * node = room->pAutomap;
	while (node)
	{
		node = node->m_pNext;
		++nodecount;
	}

	if (nodecount != automap->nNodeCount)
	{
		return FALSE;
	}

	unsigned int ii = 0;
	node = room->pAutomap;
	while (node)
	{
		if (TESTBIT(automap->pNodeFlags, ii))
		{
			node->m_dwFlags |= AUTOMAP_VISITED;
		}

		node = node->m_pNext;
		++ii;
	}

	return TRUE;
}
