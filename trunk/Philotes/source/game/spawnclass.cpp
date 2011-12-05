//----------------------------------------------------------------------------
// FILE: spawnclass.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "excel_private.h"
#include "game.h"
#include "level.h"
#include "monsters.h"
#include "picker.h"
#include "objects.h"
#include "script.h"
#include "spawnclass.h"
#include "unit_priv.h"


//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

int SpawnClassEvaluatePicks(
	GAME *game,
	LEVEL *pLevel,
	int nSpawnClassIndex, 
	int nPicks[MAX_SPAWN_CLASS_CHOICES],
	RAND &rand,
	int nMonsterExperienceLevel /*=-1*/)	
{
	int nNumPicks = 0;
	
	const SPAWN_CLASS_DATA *pSpawnClassData = SpawnClassGetData(game, nSpawnClassIndex);
		
	if (pSpawnClassData->ePickType == SPAWN_CLASS_PICK_ALL)
	{
		for (int i = 0; i < pSpawnClassData->nNumPicks; ++i)
		{
			nPicks[ i ] = i;
		}
		nNumPicks = pSpawnClassData->nNumPicks;
	}	
	else
	{
		//ASSERT_RETZERO( pLevel );
		int nDesiredPickCount = -1;
		switch (pSpawnClassData->ePickType)
		{
			case SPAWN_CLASS_PICK_ONE:		nDesiredPickCount = 1;		break;
			case SPAWN_CLASS_PICK_TWO:		nDesiredPickCount = 2;		break;
			case SPAWN_CLASS_PICK_THREE:	nDesiredPickCount = 3;		break;
			case SPAWN_CLASS_PICK_FOUR:		nDesiredPickCount = 4;		break;
			case SPAWN_CLASS_PICK_FIVE:		nDesiredPickCount = 5;		break;
			case SPAWN_CLASS_PICK_SIX:		nDesiredPickCount = 6;		break;
			case SPAWN_CLASS_PICK_SEVEN:	nDesiredPickCount = 7;		break;
			case SPAWN_CLASS_PICK_EIGHT:	nDesiredPickCount = 8;		break;
			case SPAWN_CLASS_PICK_NINE:		nDesiredPickCount = 9;		break;			
		}

		if (nDesiredPickCount > 0)
		{
		
			// put all the choices with weights in a picker
			int nAvailablePicks = pSpawnClassData->nNumPicks;
			int nPickSize( 0 );
			PickerStart(game, picker);
			for (int i = 0; i < nAvailablePicks; ++i)
			{
				const SPAWN_CLASS_PICK *pPick = &pSpawnClassData->pPicks[i];				
				const SPAWN_ENTRY *pSpawnEntry = &pPick->tEntry;
				switch (pSpawnEntry->eEntryType)
				{
				case SPAWN_ENTRY_TYPE_MONSTER:
					if( !pLevel ||
						MonsterCanSpawn(pSpawnEntry->nIndex, pLevel, nMonsterExperienceLevel ) )
					{
						nPickSize++;
						PickerAdd(MEMORY_FUNC_FILELINE(game, i, pPick->nPickWeight));
					}
					break;
				case SPAWN_ENTRY_TYPE_UNIT_TYPE:					
				case SPAWN_ENTRY_TYPE_OBJECT:
				case SPAWN_ENTRY_TYPE_SPAWN_CLASS:				
					{
						nPickSize++;
						PickerAdd(MEMORY_FUNC_FILELINE(game, i, pPick->nPickWeight));
					}
					break;				
				}
			}

			// pick nNumPicks of them while there are available picks to do
			int nPickedCount = 0;
			while (nPickedCount != nDesiredPickCount && nPickSize > 0)
			{
				int nPick = PickerChooseRemove(game, rand );
				nPicks[ nPickedCount++ ] = nPick;
				nPickSize--;
			}
			nNumPicks = nPickedCount;
			
		}
		
	}

	return nNumPicks;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SpawnClassEvaluatePicks(
	GAME *game,
	LEVEL *pLevel,
	int nSpawnClassIndex, 
	int nPicks[MAX_SPAWN_CLASS_CHOICES],
	int nMonsterExperienceLevel /*=-1*/ )
{
	return SpawnClassEvaluatePicks( game, pLevel, nSpawnClassIndex, nPicks, game->rand, nMonsterExperienceLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const SPAWN_CLASS_DATA* SpawnClassGetData(
	GAME* game,
	int nClass)
{
	return (const SPAWN_CLASS_DATA*)ExcelGetData(game, DATATABLE_SPAWN_CLASS, nClass);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const SPAWN_CLASS_DATA* SpawnClassGetDataByName(
	GAME *pGame,
	const char *pszName)
{
	int nIndex = ExcelGetLineByStringIndex( pGame, DATATABLE_SPAWN_CLASS, pszName );
	if (nIndex != INVALID_LINK)
	{
		return SpawnClassGetData( pGame, nIndex );
	}
	return NULL;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_SET_FIELD_RESULT SpawnClassParseExcel(
	struct EXCEL_TABLE * table, 
	const struct EXCEL_FIELD * field,
	unsigned int row,
	BYTE * fielddata,
	const char * token,
	unsigned int toklen,
	int param)
{
	ASSERT_RETVAL(table, ESFR_ERROR);
	ASSERT_RETVAL(fielddata, ESFR_ERROR);
	REF(param);

	SPAWN_CLASS_PICK * spawn_pick = (SPAWN_CLASS_PICK *)fielddata;
	SPAWN_ENTRY * spawn_entry = &spawn_pick->tEntry;
	spawn_entry->eEntryType = SPAWN_ENTRY_TYPE_NONE;
	spawn_entry->nIndex = INVALID_LINK;
	
	if (!token || toklen <= 0 || token[0] == 0)
	{
		return ESFR_OK;
	}

	char subtoken[4096];
	const char * buf = token;
	int buflen = (int)toklen + 1;
	int len;
	buf = PStrTok(subtoken, buf, ",", (int)arrsize(subtoken), buflen, &len);
	if (!subtoken[0])
	{
		return ESFR_OK;
	}

	BOOL found = FALSE;
	if (!found)
	{
		int monsterclass = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_MONSTERS, subtoken);
		if (monsterclass != EXCEL_LINK_INVALID)
		{
			const UNIT_DATA * monster_data = MonsterGetData(NULL, monsterclass);
			if (monster_data)
			{
				found = TRUE;										
				if (UnitDataTestFlag( monster_data, UNIT_DATA_FLAG_SPAWN ))
				{
					spawn_entry->eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
					spawn_entry->nIndex = monsterclass;
				}
			}
		}
	}
	if (!found)
	{
		int objectclass = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_OBJECTS, subtoken);
		if (objectclass != EXCEL_LINK_INVALID)
		{
			const UNIT_DATA * object_data = ObjectGetData(NULL, objectclass);
			if (object_data)
			{
				found = TRUE;										
				if (UnitDataTestFlag( object_data, UNIT_DATA_FLAG_SPAWN ))
				{
					spawn_entry->eEntryType = SPAWN_ENTRY_TYPE_OBJECT;
					spawn_entry->nIndex = objectclass;
				}
			}
		}
	}
	if (!found)
	{
		int unittype = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_UNITTYPES, subtoken);
		if (unittype != EXCEL_LINK_INVALID)
		{
			spawn_entry->eEntryType = SPAWN_ENTRY_TYPE_UNIT_TYPE;
			spawn_entry->nIndex = unittype;
			found = TRUE;
		}
	}
	if (!found)
	{
		int monsterspawnclass = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_SPAWN_CLASS, subtoken);
		if (monsterspawnclass > INVALID_LINK)
		{
			spawn_entry->eEntryType = SPAWN_ENTRY_TYPE_SPAWN_CLASS;
			spawn_entry->nIndex = monsterspawnclass;
			found = TRUE;
		}
	}
	//if (!found)
	//{
	//	ASSERTV_MSG("Token '%s' not found when reading table '%s', line %d", subtoken, table->m_Name, row);
	//	return ESFR_ERROR;
	//}
	
	return ESFR_OK;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSpawnPickIsValid(
	SPAWN_CLASS_PICK *pSpawnPick)
{
	SPAWN_ENTRY *pSpawnEntry = &pSpawnPick->tEntry;
	if (pSpawnEntry->eEntryType == SPAWN_ENTRY_TYPE_NONE || pSpawnPick->nPickWeight <= 0)
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const char *sSpawnPickGetString( 
	GAME *pGame,
	SPAWN_CLASS_PICK *pSpawnPick)
{
	ASSERTX_RETNULL( pSpawnPick, "Expected spawn pick" );
	switch (pSpawnPick->tEntry.eEntryType)
	{
	
		//----------------------------------------------------------------------------
		case SPAWN_ENTRY_TYPE_NONE:
		{
			return "[BLANK]";
		}

		//----------------------------------------------------------------------------		
		case SPAWN_ENTRY_TYPE_MONSTER:
		{
			const UNIT_DATA *pUnitData = MonsterGetData( pGame, pSpawnPick->tEntry.nIndex );
			if(pUnitData)
				return pUnitData->szName;
			else
				return "[UNKNOWN MONSTER]";
		}

		//----------------------------------------------------------------------------				
		case SPAWN_ENTRY_TYPE_UNIT_TYPE:
		{
			const UNITTYPE_DATA* pUnitTypeData = (UNITTYPE_DATA*)ExcelGetData( pGame, DATATABLE_UNITTYPES, pSpawnPick->tEntry.nIndex );
			if(pUnitTypeData)
				return pUnitTypeData->szName;
			else
				return "[UNKNOWN UNIT TYPE]";
		}

		//----------------------------------------------------------------------------				
		case SPAWN_ENTRY_TYPE_SPAWN_CLASS:
		{
			const SPAWN_CLASS_DATA *pSpawnClassData = SpawnClassGetData( pGame, pSpawnPick->tEntry.nIndex );
			if(pSpawnClassData)
				return pSpawnClassData->szSpawnClass;
			else
				return "[UNKNOWN SPAWN CLASS]";
		}
	
		//----------------------------------------------------------------------------		
		case SPAWN_ENTRY_TYPE_OBJECT:
		{
			const UNIT_DATA *pUnitData = ObjectGetData( pGame, pSpawnPick->tEntry.nIndex );
			if(pUnitData)
				return pUnitData->szName;
			else
				return "[UNKNOWN OBJECT]";
		}

	}

	return "[UNKNOWN]";
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SpawnClassExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		SPAWN_CLASS_DATA * spawn_class_data = (SPAWN_CLASS_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(spawn_class_data);

		for (unsigned int jj = 0; jj < MAX_SPAWN_CLASS_CHOICES; ++jj)
		{
			SPAWN_CLASS_PICK * spawn_pick = &spawn_class_data->pPicks[jj];
			
			if (sSpawnPickIsValid(spawn_pick))
			{
				spawn_class_data->nNumPicks++;
				spawn_class_data->nTotalWeight += spawn_pick->nPickWeight;
			}
			else
			{
				if (spawn_class_data->ePickType != SPAWN_CLASS_PICK_ALL &&
					spawn_pick->tEntry.eEntryType != SPAWN_ENTRY_TYPE_NONE && 
					spawn_pick->nPickWeight == 0)
				{
					ASSERTV_MSG("Spawnclass '%s' pick '%s' has a weight of zero", spawn_class_data->szSpawnClass, sSpawnPickGetString(NULL, spawn_pick));
				}
			}
		}
		
		// copy this row
		SPAWN_CLASS_PICK tPickCopy[MAX_SPAWN_CLASS_CHOICES];
		MemoryCopy(tPickCopy, MAX_SPAWN_CLASS_CHOICES * sizeof(SPAWN_CLASS_PICK), spawn_class_data->pPicks, sizeof(SPAWN_CLASS_PICK) * MAX_SPAWN_CLASS_CHOICES);

		// clear out the spawn row
		memclear(spawn_class_data->pPicks, sizeof(SPAWN_CLASS_PICK) * MAX_SPAWN_CLASS_CHOICES);
		
		// put back only valid entries, this is nice to do so we can assume that all the
		// entries in here are valid and continuous
		int nValidCount = 0;
		for (unsigned int kk = 0; kk < MAX_SPAWN_CLASS_CHOICES; kk++)
		{
			SPAWN_CLASS_PICK * spawn_pick = &tPickCopy[kk];
			if (sSpawnPickIsValid(spawn_pick))
			{
				spawn_class_data->pPicks[nValidCount++] = *spawn_pick;
			}
		}			
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSpawnEntryPickByMonsterClass(
	GAME* game,
	LEVELID idLevel,
	const SPAWN_ENTRY *pSpawn,
	int nMonsterExperienceLevel,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	int nCurrentCount,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter,
	int nLevelDepthOverride,
	int nSpawnClassSource)
{
	int nMonsterClass = pSpawn->nIndex;
	ASSERT_RETINVALID(nMonsterClass > INVALID_LINK);
	const UNIT_DATA* pUnitData = MonsterGetData(game, nMonsterClass);
	ASSERT_RETINVALID(pUnitData);


	// check of monster can spawn
	LEVEL *pLevel = LevelGetByID( game, idLevel );
	if (MonsterCanSpawn( nMonsterClass, pLevel, nMonsterExperienceLevel ) == FALSE)
	{
		return nCurrentCount;
	}
	
	// check for too many	
	if (nCurrentCount >= MAX_SPAWNS_AT_ONCE)
	{
		return nCurrentCount;
	}
	
	if ( pfnFilter && ! pfnFilter( game, nMonsterClass, pUnitData ) )
	{
		return nCurrentCount;
	}
	tSpawns[nCurrentCount] = *pSpawn;
#if ISVERSION(DEVELOPMENT)
	tSpawns[nCurrentCount].nSpawnClassSource = nSpawnClassSource;
#endif	
	return nCurrentCount+1;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSpawnEntryPickByObjectClass(
	GAME* game,
	LEVELID idLevel,
	const SPAWN_ENTRY *pSpawn,
	int nMonsterExperienceLevel,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	int nCurrentCount,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter,
	int nLevelDepthOverride,
	int nSpawnClassSource)
{
	int nObjectClass = pSpawn->nIndex;
	ASSERT_RETINVALID(nObjectClass > INVALID_LINK);
	const UNIT_DATA* pUnitData = MonsterGetData(game, nObjectClass);
	ASSERT_RETINVALID(pUnitData);
	int nLevelDepth = 0;
	if(AppIsTugboat())
	{
		LEVEL * pLevel = LevelGetByID( game, idLevel );
		// this calculation needs to happen in data, not here!
		nLevelDepth = LevelGetDifficulty( pLevel );
		if( nLevelDepth < 0 )
		{
			nLevelDepth = 0;
		}
		if( nLevelDepthOverride != -1 )
		{
			nLevelDepth = nLevelDepthOverride;
		}
	}
	if (!UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_SPAWN ))
	{
		return nCurrentCount;
	}
	if (nCurrentCount >= MAX_SPAWNS_AT_ONCE)
	{
		return nCurrentCount;
	}
	if(AppIsTugboat())
	{
		if (pUnitData->nMinSpawnLevel > nLevelDepth)
		{
			return nCurrentCount;
		}
		if (pUnitData->nMaxSpawnLevel <= nLevelDepth)
		{
			return nCurrentCount;
		}
	}
	if ( pfnFilter && ! pfnFilter( game, nObjectClass, pUnitData ) )
	{
		return nCurrentCount;
	}
	tSpawns[nCurrentCount] = *pSpawn;
#if ISVERSION(DEVELOPMENT)
	tSpawns[nCurrentCount].nSpawnClassSource = nSpawnClassSource;
#endif		
	return nCurrentCount+1;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSpawnEntryPickByUnitType(
	GAME* game,
	LEVELID idLevel,
	const SPAWN_ENTRY *pSpawn,
	int nMonsterExperienceLevel,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	int nCurrentCount,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter,
	int nLevelDepthOverride,
	int nSpawnClassSource)
{
	LEVEL * pLevel = LevelGetByID( game, idLevel );
	int nUnitType = pSpawn->nIndex;
	int monster_count = ExcelGetNumRows(game, DATATABLE_MONSTERS);
	
	/*
	int nLevelDepth = 0;
	if(AppIsTugboat())
	{
		// this calculation needs to happen in data, not here!
		nLevelDepth = LevelGetDifficulty( pLevel );
		if( nLevelDepth < 0 )
		{
			nLevelDepth = 0;
		}
		if( nLevelDepthOverride != -1 )
		{
			nLevelDepth = nLevelDepthOverride;
		}
	}
	*/
	PickerStart(game, picker);
	for (int ii = 0; ii < monster_count; ii++)
	{
		const UNIT_DATA* pUnitData = MonsterGetData(game, ii);
		if (! pUnitData )
			continue;
		
		if (MonsterCanSpawn( ii, pLevel, nMonsterExperienceLevel ) == FALSE)
		{
			continue;
		}
		
		if (pUnitData->nRarity <= 0)
		{
			continue;
		}
		/*
		if(AppIsTugboat())
		{
			if (pUnitData->nMinSpawnLevel > nLevelDepth )
			{
				continue;
			}
			if (pUnitData->nMaxSpawnLevel <= nLevelDepth )
			{
				continue;
			}
		}*/
		if (!UnitIsA(pUnitData->nUnitType, nUnitType))
		{
			continue;
		}
		PickerAdd(MEMORY_FUNC_FILELINE(game, ii, pUnitData->nRarity));
	}
	int monster_class = PickerChoose(game);
	
	SPAWN_ENTRY tToSpawn = *pSpawn;  // copy settings from type spawn spec
	tToSpawn.nIndex = monster_class;
	tToSpawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
	
	return sSpawnEntryPickByMonsterClass(
		game, 
		idLevel, 
		&tToSpawn, 
		nMonsterExperienceLevel, 
		tSpawns, 
		nCurrentCount, 
		pfnFilter,
		nLevelDepthOverride,
		nSpawnClassSource);
}

//----------------------------------------------------------------------------
// FORWARD DECLARATION
//----------------------------------------------------------------------------
static int sPickSpawns(
	GAME* game,
	LEVELID idLevel,
	const SPAWN_CLASS_DATA* spawn,
	int spawnclass,
	int nMonsterExperienceLevel,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	int nCurrentCount,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter,
	int nLevelDepthoverride);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSpawnEntryPick(
	GAME* game,
	LEVELID idLevel,
	const SPAWN_CLASS_DATA* spawn,
	int pick,
	int nMonsterExperienceLevel,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	int nCurrentCount,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter,
	int nLevelDepthoverride,
	int nSpawnClassSource)
{
	if (pick < 0)
	{
		return INVALID_LINK;
	}
	ASSERT_RETINVALID(pick < MAX_SPAWN_CLASS_CHOICES);
	const SPAWN_CLASS_PICK * pick_data = spawn->pPicks + pick;
	const SPAWN_ENTRY *pSpawnEntry = &pick_data->tEntry;
	switch (pSpawnEntry->eEntryType)
	{
	case SPAWN_ENTRY_TYPE_MONSTER:
		{
			return sSpawnEntryPickByMonsterClass(
				game, 
				idLevel, 
				pSpawnEntry, 
				nMonsterExperienceLevel, 
				tSpawns, 
				nCurrentCount, 
				pfnFilter,
				nLevelDepthoverride,
				nSpawnClassSource);
		}
	case SPAWN_ENTRY_TYPE_UNIT_TYPE:
		{ 
			return sSpawnEntryPickByUnitType(
				game, 
				idLevel, 
				pSpawnEntry, 
				nMonsterExperienceLevel, 
				tSpawns, 
				nCurrentCount, 
				pfnFilter,
				nLevelDepthoverride,
				nSpawnClassSource);
		}
	case SPAWN_ENTRY_TYPE_SPAWN_CLASS:
		{
			const SPAWN_CLASS_DATA *pSpawnClassData = SpawnClassGetData(game, pSpawnEntry->nIndex);
			ASSERTX_RETINVALID(pSpawnClassData, "Expected spawn class");
						
			// how many times do we evaluate this spawn class
			int nCount = SpawnEntryEvaluateCount( game, pSpawnEntry );
			for (int i = 0; i < nCount; ++i)
			{
				nCurrentCount = sPickSpawns(
					game, 
					idLevel, 
					pSpawnClassData, 
					pSpawnEntry->nIndex, 
					nMonsterExperienceLevel, 
					tSpawns, 
					nCurrentCount, 
					pfnFilter,
					nLevelDepthoverride);
			}
			return nCurrentCount;
			
		}
	case SPAWN_ENTRY_TYPE_OBJECT:
		{
			return sSpawnEntryPickByObjectClass(
				game, 
				idLevel, 
				pSpawnEntry, 
				nMonsterExperienceLevel, 
				tSpawns, 
				nCurrentCount, 
				pfnFilter,
				nLevelDepthoverride,
				nSpawnClassSource);
		}
	}
	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sPickSpawns(
	GAME* game,
	LEVELID idLevel,
	const SPAWN_CLASS_DATA* spawn,
	int spawnclass,
	int nMonsterExperienceLevel,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	int nCurrentCount,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter,
	int nLevelDepthOverride )
{
	ASSERT_RETINVALID(spawn);

	if (spawn->ePickType == SPAWN_CLASS_PICK_ALL)
	{
		int nCount = nCurrentCount;
		for(int ii = 0; ii < spawn->nNumPicks; ii++)
		{
			nCount = sSpawnEntryPick(
				game, 
				idLevel, 
				spawn, 
				ii, 
				nMonsterExperienceLevel, 
				tSpawns, 
				nCount, 
				pfnFilter,
				nLevelDepthOverride,
				spawnclass);
		}
		return nCount;
	}
	else
	{

		// do each of the picks
			
		// see if we have a set of picks saved 
		SPAWN_CLASS_MEMORY *pMemory = LevelGetSpawnMemory(game, idLevel, spawnclass);		
		if (pMemory)
		{
			for (int i = 0; i < pMemory->nNumPicks; ++i)
			{
				int nRememberedPick = pMemory->nPicks[ i ];
				nCurrentCount = sSpawnEntryPick(
					game, 
					idLevel,
					spawn, 
					nRememberedPick, 
					nMonsterExperienceLevel, 
					tSpawns, 
					nCurrentCount, 
					pfnFilter,
					nLevelDepthOverride,
					spawnclass);
			}
		}
		else
		{
			int nPicks[ MAX_SPAWN_CLASS_CHOICES ];
			LEVEL *pLevel = LevelGetByID( game, idLevel );
			int nPickCount = SpawnClassEvaluatePicks(game, pLevel, spawnclass, nPicks, nMonsterExperienceLevel);
			for (int i = 0; i < nPickCount; ++i)
			{
				int nPick = nPicks[ i ];
				nCurrentCount = sSpawnEntryPick(
					game, 
					idLevel,
					spawn, 
					nPick, 
					nMonsterExperienceLevel, 
					tSpawns, 
					nCurrentCount, 
					pfnFilter,
					nLevelDepthOverride,
					spawnclass);
			}

		}					
		
	}
	
	return nCurrentCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SpawnClassEvaluate(
	GAME* game,
	LEVELID idLevel,
	int spawnclass,
	int nMonsterExperienceLevel,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter /*= NULL*/,
	int nLevelDepthOverride /*= -1 */,
	DWORD dwSpawnClassEvaluateFlags /*= 0*/)
{
	ASSERT_RETINVALID(game);
	ASSERT_RETINVALID(spawnclass > INVALID_LINK);

	const SPAWN_CLASS_DATA* spawn = SpawnClassGetData(game, spawnclass);
	ASSERT_RETINVALID(spawn);

	SPAWN_ENTRY tPickedSpawns[ MAX_SPAWNS_AT_ONCE ];
	int nNumPickedSpawns = sPickSpawns(
		game, 
		idLevel, 
		spawn, 
		spawnclass, 
		nMonsterExperienceLevel, 
		tPickedSpawns, 
		0, 
		pfnFilter,
		nLevelDepthOverride);

	// copy to result param	
	if (TESTBIT( dwSpawnClassEvaluateFlags, SCEF_RANDOMIZE_PICKS_BIT ))
	{
		if (nNumPickedSpawns > 0)
		{
			PickerStart(game, tPicker);
			for (int i = 0; i < nNumPickedSpawns; ++i)
			{
				PickerAdd(MEMORY_FUNC_FILELINE(game, i, 1));
			}
			for (int nCurrentIndex = 0; nCurrentIndex < nNumPickedSpawns; ++nCurrentIndex)
			{
				int nIndex = PickerChooseRemove(game);
				tSpawns[ nCurrentIndex ] = tPickedSpawns[ nIndex ];
			}
		}
	}
	else
	{
		// copy results straight over
		for (int i = 0; i < nNumPickedSpawns; ++i)
		{
			tSpawns[ i ] = tPickedSpawns[ i ];
		}
	}
		
	return nNumPickedSpawns;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SpawnTreeEvaluateDecisions(
	GAME *pGame,
	LEVEL *pLevel,
	int nSpawnClassIndex,
	PFN_EVALUATE_SPAWN_TREE_CALLBACK pfnCallback,
	void *pUserData)
{
	const SPAWN_CLASS_DATA *pSpawnClassData = SpawnClassGetData(pGame, nSpawnClassIndex );

	// evaluate the picks at this node if there is a choice to make
	if (pSpawnClassData->ePickType != SPAWN_CLASS_PICK_ALL)
	{
		int nPicks[MAX_SPAWN_CLASS_CHOICES];
		int nPickCount = SpawnClassEvaluatePicks(pGame, pLevel, nSpawnClassIndex, nPicks);
		if (nPickCount > 0)
		{
			pfnCallback(pGame, nSpawnClassIndex, nPicks, nPickCount, pUserData);
		}
	}
			
	// evaluate all other nodes
	for (int i = 0; i < pSpawnClassData->nNumPicks; ++i)
	{
		const SPAWN_CLASS_PICK *pPick = &pSpawnClassData->pPicks[ i ];
		const SPAWN_ENTRY *pSpawnEntry = &pPick->tEntry;
		
		if (pSpawnEntry->eEntryType == SPAWN_ENTRY_TYPE_SPAWN_CLASS)
		{
			SpawnTreeEvaluateDecisions(pGame, pLevel, pSpawnEntry->nIndex, pfnCallback, pUserData);
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int SpawnEntryEvaluateCount(
	GAME *pGame,
	const SPAWN_ENTRY *pSpawn)
{
	ASSERTX_RETZERO( pSpawn, "No spawn entry to evaluate" );

	if (pSpawn->codeCount == (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT)
	{
		return 1;  // special case when we create an entry dynamically in code
	}
	else
	{
		int nCodeLength = 0;
		BYTE *pCode = ExcelGetScriptCode( pGame, DATATABLE_SPAWN_CLASS, pSpawn->codeCount, &nCodeLength );
		if (pCode)
		{
			return VMExecI( pGame, pCode, nCodeLength );
		}
	}
	return 0;
	
}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SpawnClassGetPossibleMonsters( 
	GAME* pGame, 
	LEVEL* pLevel, 
	int nSpawnClass,
	int *pnMonsterClassBuffer,
	int nBufferSize,
	int * pnPossibleMonsterCount,
	BOOL bUseLevelSpawnClassMemory)
{
	ASSERTX_RETURN( pnPossibleMonsterCount, "Invalid arguments" );

	if (nSpawnClass != INVALID_LINK)
	{	
		const SPAWN_CLASS_DATA *pSpawnClassData = SpawnClassGetData( pGame, nSpawnClass );

		// this array will be full of the entries that can be picked at this spawn class node
		const SPAWN_ENTRY* tEntries[ MAX_SPAWN_CLASS_CHOICES ];
		int nNumEntries = 0;  // number of valid entires in tEntries[]

		// setup an array of the spawn entries at this spawn class node, is there a memeory 
		// for this spawn class node in the level we will use the saved pickes in the
		// memory, otherwise we will use all the entries
		SPAWN_CLASS_MEMORY *pSpawnMemory = NULL;
		if (bUseLevelSpawnClassMemory == TRUE && pLevel != NULL)
		{
#if !ISVERSION(CLIENT_ONLY_VERSION)
			pSpawnMemory = LevelGetSpawnMemory( pGame, LevelGetID( pLevel ), nSpawnClass );
#endif	
		}
		if (pSpawnMemory)
		{
			// evaluate just the picks saved in memory
			for (int i = 0; i < pSpawnMemory->nNumPicks; ++i)
			{
				int nSavedPickIndex = pSpawnMemory->nPicks[ i ];
				tEntries[ nNumEntries++ ] = &pSpawnClassData->pPicks[ nSavedPickIndex ].tEntry;				
			}
		}
		else
		{
			// no memory for this selection, evaluate them all
			for (int i = 0; i < pSpawnClassData->nNumPicks; ++i)
			{
				tEntries[ nNumEntries++ ] = &pSpawnClassData->pPicks[ i ].tEntry;
			}
		}

		// go through each spawn entry at this spawn class node
		for (int i = 0; i < nNumEntries; ++i)
		{
			const SPAWN_ENTRY* pEntry = tEntries[ i ];
			
			// for each monster at this spawn class node, add it to the buffer if not already present
			if (pEntry->eEntryType == SPAWN_ENTRY_TYPE_MONSTER)
			{
				int nMonsterClass = pEntry->nIndex;
				
				MonsterAddPossible( 
					pLevel,
					nSpawnClass,
					nMonsterClass, 
					pnMonsterClassBuffer, 
					nBufferSize,
					pnPossibleMonsterCount );
			}
			
			// for each unit type at this spawn class node, add all the monsters represented by the type
			else if (pEntry->eEntryType == SPAWN_ENTRY_TYPE_UNIT_TYPE)
			{
				int nUnitType = pEntry->nIndex;
				int nMonsterCount = ExcelGetNumRows( pGame, DATATABLE_MONSTERS );
				for (int j = 0; j < nMonsterCount; ++j)
				{
					const UNIT_DATA* pUnitData = MonsterGetData( pGame, j );
					if (! pUnitData )
						continue;
					if (pUnitData->nUnitType != INVALID_LINK && UnitIsA( pUnitData->nUnitType, nUnitType ))
					{
						int nMonsterClass = j;
						
						MonsterAddPossible( 
							pLevel,
							nSpawnClass,
							nMonsterClass, 
							pnMonsterClassBuffer, 
							nBufferSize,
							pnPossibleMonsterCount );
							
					}
					
				}
				
			}
			
			// for each spawn class at this spawn class node, evaluate it's possibilities too
			else if (pEntry->eEntryType == SPAWN_ENTRY_TYPE_SPAWN_CLASS)
			{
				s_SpawnClassGetPossibleMonsters( 
					pGame, 
					pLevel, 
					pEntry->nIndex, 
					pnMonsterClassBuffer, 
					nBufferSize, 
					pnPossibleMonsterCount,
					bUseLevelSpawnClassMemory);
			}
			
		}
		
	}
	
}
