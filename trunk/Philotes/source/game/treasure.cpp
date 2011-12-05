//----------------------------------------------------------------------------
// treasure.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "excel_private.h"
#include "globalindex.h"
#include "treasure.h"
#include "unit_priv.h" // also includes units.h, which includes game.h
#include "items.h"
#include "picker.h"
#include "inventory.h"
#include "monsters.h"
#include "config.h"
#include "quests.h"
#include "console.h"
#include "dictionary.h"
#include "script.h"
#include "performance.h"
#include "clients.h"
#include "room_path.h"
#include "s_quests.h"
#include "level.h"
#include "quest.h"
#include "skills.h"
#include "Currency.h"
#include "global_themes.h"
#if ISVERSION(SERVER_VERSION)
#include "treasure.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// LOCAL DATA
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
STR_DICT dictTreasureclassCategory[] =
{
	{ "",				TC_CATEGORY_ITEMCLASS		},
	{ "type",			TC_CATEGORY_UNITTYPE		},
	{ "tc",				TC_CATEGORY_TREASURECLASS	},
	{ "quality",		TC_CATEGORY_QUALITY_MOD		},
	{ "modtype",		TC_CATEGORY_UNITTYPE_MOD	},
	{ "maxslots",		TC_CATEGORY_MAX_SLOTS		},
	{ "nodrop",			TC_CATEGORY_NODROP			},

	{ NULL,				TC_CATEGORY_NONE			},
};

//----------------------------------------------------------------------------
static const PICKOP_SYMBOL sctPickopSymbols[] = 
{
	{ '!',		PICK_OP_NOT		},
	{ '~',		PICK_OP_NEGATE	},
	{ '*',		PICK_OP_MOD	},
	{ '+',		PICK_OP_SET },			// set exact
	{ '>',		PICK_OP_IGNORE_AND_GREATER },// NOT and any quality above it ignore	
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
float gflMoneyChanceOverride = -1.0f;
#endif

#if ISVERSION(DEVELOPMENT)
#define TreasureWarn(fmt, ...) { GLOBAL_DEFINITION* global = DefinitionGetGlobal(); if ( global && global->nDebugOutputLevel <= OP_HIGH ) ASSERTV_MSG(fmt, __VA_ARGS__) }
#else
#if defined(HELLGATE)
#define TreasureWarn(fmt, ...) ASSERTV_MSG(fmt, __VA_ARGS__)
#elif defined(TUGBOAT)
#define TreasureWarn(fmt, ...) ExcelLog(fmt, __VA_ARGS__)
#else
#define TreasureWarn(fmt, ...) ExcelLog(fmt, __VA_ARGS__)
#endif
#endif

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

#if !ISVERSION(CLIENT_ONLY_VERSION)
static TREASURE_RESULT s_sTreasureSpawn(
	GAME * game,
	UNIT * spawner,
	const TREASURE_DATA * treasure,
	int spawnlevel,
	const TREASURE_MOD_LIST & tModList,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult,
	int nDifficutly = 0,
	int nLevel = INVALID_ID,
	int nTreasureLevelBoost = 0);
#endif


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const TREASURE_DATA* TreasureGetData(
	GAME* game,
	int treasureclass)
{
	return (const TREASURE_DATA*)ExcelGetData(game, DATATABLE_TREASURE, treasureclass);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TreasureDataTestFlag(
	const TREASURE_DATA *pTreasureData,
	TREASURE_DATA_FLAG eFlag)
{
	ASSERTX_RETFALSE( pTreasureData, "Expected treasure data" );
	return TESTBIT( pTreasureData->pdwFlags, eFlag );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TreasureDataTestFlag(
	GAME *pGame,
	int nTreasure,
	TREASURE_DATA_FLAG eFlag)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( nTreasure != INVALID_LINK, "Expected treasure link" );
	const TREASURE_DATA *pTreasureData = TreasureGetData( pGame, nTreasure );
	if (pTreasureData)
	{
		return TreasureDataTestFlag( pTreasureData, eFlag );
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PICKINDEX_OPERATOR sGetPickOperator(
	const char * szField)
{
	if (szField)
	{
		for (unsigned int ii=0; ii < arrsize(sctPickopSymbols); ii++)
		{
			if (szField[0] == sctPickopSymbols[ii].cSymbol)
			{
				return sctPickopSymbols[ii].eOp;
			}
		}
	}

	return PICK_OP_NOOP;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsAMod(
	const TREASURE_PICK & tPick)
{
	if (tPick.eCategory == TC_CATEGORY_NONE)
	{
		return FALSE;
	}

	if (tPick.eCategory == TC_CATEGORY_ITEMCLASS ||
		tPick.eCategory == TC_CATEGORY_UNITTYPE ||
		tPick.eCategory == TC_CATEGORY_NODROP ||
		tPick.eCategory == TC_CATEGORY_TREASURECLASS)
	{
		return (tPick.nValue == 0);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsAPick(
	const TREASURE_PICK & tPick)
{
	if (tPick.eCategory == TC_CATEGORY_NONE)
	{
		return FALSE;
	}

	if (tPick.eCategory == TC_CATEGORY_ITEMCLASS ||
		tPick.eCategory == TC_CATEGORY_UNITTYPE ||
		tPick.eCategory == TC_CATEGORY_NODROP ||
		tPick.eCategory == TC_CATEGORY_TREASURECLASS)
	{
		return (tPick.nValue != 0);
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_SET_FIELD_RESULT TreasureExcelParsePickItem(
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

	TREASURE_PICK * pick = (TREASURE_PICK *)fielddata;
	pick->eCategory = TC_CATEGORY_NONE;
	for (unsigned int ii = 0; ii < MAX_CATEGORY_INDICIES; ii++)
	{
		pick->nIndex[ii] = INVALID_LINK;
		pick->eOperator[ii] = PICK_OP_NOOP;
	}
	
	if (token == NULL || toklen <= 0 || token[0] == 0)
	{
		return ESFR_OK;
	}

	char tokbuf[4096];
	char subtoken[2048];
	int len;

	int buflen = (int)(MIN(toklen + 1, arrsize(tokbuf)));
	PStrCopy(tokbuf, token, buflen);
	PStrLower(tokbuf, arrsize(tokbuf));
	const char * buf = tokbuf;

	buf = PStrTok(subtoken, buf, ":", arrsize(subtoken), buflen, &len);

	pick->eCategory = (TREASURECLASS_CATEGORY)StrDictFind(dictTreasureclassCategory, subtoken);
	if ((int)pick->eCategory == -1)
	{
		pick->eCategory = TC_CATEGORY_ITEMCLASS;
		buf = tokbuf;		//no category indicator was found, set the key back to the beginning
		buflen = (int)(MIN(toklen + 1, arrsize(tokbuf)));
	}

	char subtok2[2048];
	int nNumKeys = 0;
	buf = PStrTok(subtok2, buf, ",", arrsize(subtok2), buflen, &len);
	char * szKey = subtok2;
	while (buf)
	{
		pick->eOperator[nNumKeys] = sGetPickOperator(szKey);
		if (pick->eOperator[nNumKeys] != PICK_OP_NOOP)
		{
			szKey++;		// go past the op symbol
		}

		BOOL found = FALSE;
		switch (pick->eCategory)
		{
		case TC_CATEGORY_ITEMCLASS:
			{
				int nItemClass = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_ITEMS, szKey);
				if (nItemClass > INVALID_LINK)
				{
					pick->nIndex[nNumKeys] = nItemClass;
					found = TRUE;

					if (pick->eOperator[nNumKeys] == PICK_OP_MOD)		// this is an op to change our category (for readability)
					{
						pick->eCategory = TC_CATEGORY_UNITTYPE_MOD;
						pick->eOperator[nNumKeys] = PICK_OP_NOOP;
					}
				}
				else
				{
					TreasureWarn( "Item not found when reading table treasure.txt, line %d, field '%s', item '%s'", row, token, szKey);
				}
			}
			break;
		case TC_CATEGORY_UNITTYPE:
			{
				int unittype = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_UNITTYPES, szKey);
				if (unittype > INVALID_LINK)
				{
					pick->nIndex[nNumKeys] = unittype;
					found = TRUE;

					if (pick->eOperator[nNumKeys] == PICK_OP_MOD)		// this is an op to change our category (for readability)
					{
						pick->eCategory = TC_CATEGORY_UNITTYPE_MOD;
						pick->eOperator[nNumKeys] = PICK_OP_NOOP;
					}
				}
				else
				{
					TreasureWarn( "Item type not found when reading table treasure.txt, line %d, field '%s', type '%s'", row, token, szKey);
				}
			}
			break;
		case TC_CATEGORY_QUALITY_MOD:
			{
				int nQuality = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_ITEM_QUALITY, szKey);
				if (nQuality > INVALID_LINK)
				{
					pick->nIndex[nNumKeys] = nQuality;
					found = TRUE;
				}
				else
				{
					TreasureWarn( "Quality not found when reading table treasure.txt, line %d, field '%s', type '%s'", row, token, szKey);
				}
			}
			break;
		case TC_CATEGORY_TREASURECLASS:
			{
				int treasureclass = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_TREASURE, szKey);
				if (treasureclass > INVALID_LINK)
				{
					pick->nIndex[nNumKeys] = treasureclass;
					found = TRUE;
				}
				else
				{
					TreasureWarn( "Treasure class not found when reading table treasure.txt, line %d, field '%s', tc '%s'", row, token, szKey);
				}
			}
			break;
		case TC_CATEGORY_NODROP:
			{
				pick->nIndex[nNumKeys] = INVALID_LINK;
				found = TRUE;
			}
			break;
		case TC_CATEGORY_MAX_SLOTS:
			{
				found = TRUE;
			}
			break;
		}

		if (!found)
		{
			TreasureWarn( "token not found when reading table treasure.txt, line %d, field '%s'", row, token);
		}

		nNumKeys++;
		buf = PStrTok(subtok2, buf, ",", arrsize(subtok2), buflen, &len);
		szKey = subtok2;
	}
	return ESFR_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_SET_FIELD_RESULT TreasureExcelParseNumPicks(
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

	if (!token || toklen <= 0 || token[0] == 0)
	{
		return ESFR_OK;
	}

	TREASURE_NUM_PICK_CHANCES * pick_chances = (TREASURE_NUM_PICK_CHANCES *)fielddata;

	memclear(pick_chances->pnNumPicks, MAX_NUM_CHANCES * sizeof(int));
	memclear(pick_chances->pnWeights, MAX_NUM_CHANCES * sizeof(int));

	// if there's only a number specified
	if (PStrIsNumeric(token))
	{
		pick_chances->pnNumPicks[0] = PStrToInt(token);
		pick_chances->pnWeights[0] = 1;
		return ESFR_OK;
	}

	// otherwise we need to parse for the list of numdrops with weights
	const char * buf = token;
	char subtoken[4096];
	int buflen = (int)toklen + 1;
	int len;
	int iChanceIndex = 0;

	buf = PStrTok(subtoken, buf, ",", (int)arrsize(subtoken), buflen, &len);
	while (buf && iChanceIndex < MAX_NUM_CHANCES)
	{
		const char * c = subtoken;
		char szAmount[10];
		char szWeight[10];
		memclear(szAmount, 10 * sizeof(char));
		memclear(szWeight, 10 * sizeof(char));
		char * d = szAmount;
		while (c && *c)
		{
			if (isdigit(*c))
			{
				if (d - szWeight < 10 || d - szAmount < 10)
				{
					*d = *c;
					d++;
				}
				else
				{
					ASSERTV_MSG("Error parsing num drop weight list '%s'", subtoken);
					return ESFR_OK;  // maybe this should be ESFR_ERROR if we want to be rigid about it -Colin
				}
			}
			if (*c == ':')
			{
				d = szWeight;
			}
			c++;
		}

		pick_chances->pnNumPicks[iChanceIndex] = PStrToInt(szAmount);
		pick_chances->pnWeights[iChanceIndex] = PStrToInt(szWeight);

		buf = PStrTok(subtoken, buf, ",", arrsize(subtoken), buflen, &len);
		iChanceIndex++;
	}
	
	return ESFR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sProcessTCMods(
	EXCEL_TABLE * table,
	unsigned int row, 
	BOOL * pbChecked)
{
	if (pbChecked[row])
	{
		return;
	}

	pbChecked[row] = TRUE;

	TREASURE_DATA * treasure = (TREASURE_DATA *)ExcelGetDataPrivate(table, row);
	if (! treasure )
		return;

	for (unsigned int jj = 0; jj < MAX_TREASURE_CHOICES; jj++)
	{
		// First check for any invalid choices (picks that have no items that could be parsed)
		if (treasure->pPicks[jj].eCategory == TC_CATEGORY_ITEMCLASS ||
			treasure->pPicks[jj].eCategory == TC_CATEGORY_UNITTYPE ||
			treasure->pPicks[jj].eCategory == TC_CATEGORY_TREASURECLASS )
		{
			BOOL bOk = FALSE;
			for (unsigned int ii = 0; ii < MAX_TREASURE_CHOICES; ii++)
			{
				if (treasure->pPicks[jj].nIndex[ii] != INVALID_LINK)
				{
					bOk = TRUE;
					break;
				}
			}

			if (!bOk)
			{
				treasure->pPicks[jj].nValue = 0;		//remove it from consideration
			}
		}

		// Add all the modifiers for this row to the treasure class's modifier list
		if (sIsAMod(treasure->pPicks[jj]))
		{
			if (treasure->pPicks[jj].eCategory == TC_CATEGORY_TREASURECLASS)
			{
				// add the mods for the sub-treasure class
				// first make sure the mods for this TC have been processed
				int nSub = treasure->pPicks[jj].nIndex[0];
				if(nSub >= 0)
				{
					sProcessTCMods(table, nSub, pbChecked);

					TREASURE_DATA * subtreasure = (TREASURE_DATA *)ExcelGetDataPrivate(table, nSub);
					ASSERT_RETURN(subtreasure);
					if (subtreasure->nPickType != PICKTYPE_MODIFIERS)
					{
						ExcelTrace("Treasure class '%s' is being used as a modifier TC for TC '%s' but is not modifiers-only.\n", subtreasure->szTreasure, treasure->szTreasure);
					}

					treasure->tModList += subtreasure->tModList;	// add the mod lists
				}
			}
			else
			{
				treasure->tModList.AddMod(treasure->pPicks[jj]);
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TreasureExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	// Temporary bool array to keep track of which TC's we've built a complete mod list for, so we don't do it multiple times for deep nestings
	BOOL * pbChecked = (BOOL *)MALLOCZ(NULL, ExcelGetCountPrivate(table) * sizeof(BOOL));
	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ii++)
	{
		sProcessTCMods(table, ii, pbChecked);
	}
	FREE(NULL, pbChecked);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int s_sTreasureGetLevelBoost(
	GAME * game,
	UNIT * spawner,
	const TREASURE_DATA * treasure)
{
	ASSERT_RETZERO(game);

	int level_boost = 0;

	// get boost from treasure class itself		
	if (treasure && treasure->codeLevelBoost != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(game, DATATABLE_TREASURE, treasure->codeLevelBoost, &codelen);
		if (code)
		{
			level_boost += VMExecI(game, spawner, 0, 0, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);
		}
	}

	return level_boost;	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int s_sTreasureGetSpawnLevel(
	GAME * game,
	UNIT * spawner,
	ITEM_SPEC * spec,
	int nDifficulty = 0,
	int nLevel = INVALID_ID)
{
	ASSERT_RETFALSE(game);

	int level = -1;

	// get level from spec and use it
	if (spec)
	{
		level = spec->nLevel;
		if (level > 0)
		{
			return level;
		}
	}

	// if no spec'd level, get level from spawner if one exists
	int monlevel = 0;

	// if we're spawning items for a merchant (that will actually be on the player), 
	//   use the merchant's level.
	if (spec->pMerchant &&
		AppIsTugboat())
	{
		const UNIT_DATA* pUnitData = UnitGetData( spec->pMerchant );
		int DepthMin = pUnitData->nMinMerchantLevel;
		int DepthMax = pUnitData->nMaxMerchantLevel;

		level = RandGetNum(game, DepthMin, DepthMax);
		spawner = NULL;
	}

	if (spawner)
	{
		// if we're spawning items for a merchant (that will actually be on the player), 
		//   use the merchant's level.
		if ( spec->pMerchant && 
			!TESTBIT(spec->dwFlags, ISF_BASE_LEVEL_ON_PLAYER_BIT))
		{
			spawner = spec->pMerchant;
		}

		if (TESTBIT(spec->dwFlags, ISF_BASE_LEVEL_ON_PLAYER_BIT))
		{
			if (!UnitIsA(spawner, UNITTYPE_PLAYER) &&
				spec->unitOperator &&
				UnitIsA(spec->unitOperator, UNITTYPE_PLAYER))
			{
				spawner = spec->unitOperator;
			}
		}

		// note this will apply to items being spawned on the monster (shops & monsters w/ equipment)
		if ( UnitIsA(spawner, UNITTYPE_MONSTER) ||
			 UnitIsA(spawner, UNITTYPE_OPENABLE) ||
			 UnitIsA(spawner, UNITTYPE_DESTRUCTIBLE) )		
		{
			// In single player, vendors are created at level corresponding to Normal/Nightmare etc.
			// But in multi player, all vendors/towns are treated as though they're normal difficulty.
			// Use the difficulty override, if it is not normal difficulty, to determine the monster
			// level for a nightmare treasure spawn on that particular LEVEL.
			LEVEL *pLevel = UnitGetLevel(spawner);
			if (nDifficulty > game->nDifficulty)
			{
				monlevel = LevelGetMonsterExperienceLevel(pLevel, nDifficulty);
			}
			else
			{
				monlevel = UnitGetExperienceLevel(spawner);
			}
			level = monlevel; 

			
			const MONSTER_LEVEL * monlvldata = MonsterLevelGetData(game, level);
			if (monlvldata && monlvldata->codeItemLevel != NULL_CODE)
			{
				int codelen = 0;
				BYTE * code = ExcelGetScriptCode(game, DATATABLE_MONLEVEL, monlvldata->codeItemLevel, &codelen);
				if (code)
				{
					level = VMExecI(game, spawner, monlevel, 0, INVALID_ID, INVALID_SKILL_LEVEL, INVALID_ID, code, codelen);
				}
			}
			level = MAX(1, level);
		}

		if (UnitIsA(spawner, UNITTYPE_PLAYER))
		{
			level = MAX(1, UnitGetExperienceLevel(spawner));
		}
		
		if (UnitIsA(spawner, UNITTYPE_SINGLE_USE_RECIPE ))
		{
			level = MAX( 1, UnitGetExperienceLevel(spawner) );
		}

		if(nLevel != INVALID_ID)
			level = MAX(1, nLevel);
		
	}

	// get boost from monster qualities
	if (spawner && UnitIsA(spawner, UNITTYPE_MONSTER))
	{
		level += MonsterGetTreasureLevelBoost(spawner);
	}

	level = MAX(1, level);

	return level;
}
#endif

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int s_sTreasurePickQuality(
	GAME * game,
	UNIT * unit,
	int spawnlevel,
	const UNIT_DATA * item_data,
	const TREASURE_MOD_LIST & tModList,
	ITEM_SPEC * spec,
	bool useLuck = FALSE )
{
	ASSERT_RETINVALID(game && IS_SERVER(game));
	ASSERT_RETINVALID(item_data);

	// some items require a specific quality and will *always* use it
	if (item_data->nRequiredQuality != INVALID_LINK)
	{
		return item_data->nRequiredQuality;
	}

	// use explicit quality in spec if provided
	if (spec && spec->nQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA *pQualityData = (const ITEM_QUALITY_DATA *)ExcelGetData(game, DATATABLE_ITEM_QUALITY, spec->nQuality);
		ASSERTX_RETINVALID( pQualityData, "Expected quality data" );
		return spec->nQuality;
	}

	PickerStartF(game, picker);

	int num_qualities = ExcelGetNumRows(game, DATATABLE_ITEM_QUALITY);
	for (int ii = 0; ii < num_qualities; ii++)
	{
		const ITEM_QUALITY_DATA * quality_data = (const ITEM_QUALITY_DATA *)ExcelGetData(game, DATATABLE_ITEM_QUALITY, ii);
		if (!quality_data)
		{
			continue;
		}
		if (quality_data->nLevel > spawnlevel ||
			quality_data->nMinLevel > spawnlevel )
		{
			continue;
		}
		if (!TESTBIT(item_data->pdwQualities, ii))
		{
			continue;
		}

		BOOL bSkip = FALSE;
		int nMod = 100;
		for (int nModifier=0; nModifier < tModList.nNumModifiers; ++nModifier)
		{
			int iIndexIndex = 0;
			const TREASURE_PICK * pPickData = &(tModList.pModifiers[nModifier]);
			while (pPickData->nIndex[iIndexIndex] != INVALID_LINK && iIndexIndex < MAX_CATEGORY_INDICIES)
			{
				if (pPickData->nIndex[iIndexIndex] == ii)		// if this is the quality type we're modifying
				{
					// This is saying the item must be of this type
					if (pPickData->eOperator[iIndexIndex] == PICK_OP_SET)
					{
						return ii;
					}

					// This is saying skip items of this type
					if (pPickData->eOperator[iIndexIndex] == PICK_OP_NOT)
					{
						bSkip = TRUE;
						break;
					}

					if( pPickData->eOperator[iIndexIndex] == PICK_OP_IGNORE_AND_GREATER )
					{						
						ii = num_qualities; //and we're done with this and all further qualities
						bSkip = TRUE;
						break;
					}

					if (pPickData->eOperator[iIndexIndex] == PICK_OP_MOD || 
						pPickData->eOperator[iIndexIndex] == PICK_OP_NOOP)
					{
						nMod += (pPickData->nValue - 100);
						break;
					}
				}

				iIndexIndex++;
			}
		}

		if (bSkip)
		{
			continue;
		}

		BOOL bVendor = (spec && (UnitIsMerchant(spec->pSpawner) || spec->pMerchant != NULL));
		float fBaseRarity = (float)quality_data->nRarity;
		if (bVendor)
		{
			if ((spec->pMerchant && UnitIsGambler(spec->pMerchant)) ||
				UnitIsGambler(spec->pSpawner))
			{
				fBaseRarity = (float)quality_data->nGamblingRarity;
			}
			else
			{
				fBaseRarity = (float)quality_data->nVendorRarity;
			}
		}

		float fRarity = fBaseRarity * MAX((float)nMod / 100.0f, 0.0f);
		//Marsh - add the luck modifier
		if( useLuck && spec )
		{
			fRarity += quality_data->nRarityLuck * spec->nLuckModifier;
		}
		//marsh end
		
		// add to picker
		PickerAdd(MEMORY_FUNC_FILELINE(game, ii, fRarity));
		
	}

	int nQuality = PickerChoose(game);

	// ok, if this is a gambler, and we're just generating the items and not buying one
	//   downgrade the (dummy) item to its lowest quality
	if (nQuality != INVALID_LINK && spec && ItemIsSpawningForGambler(spec))
	{
		for (unsigned int ii = 0; ii < MAX_QUALITY_DOWNGRADE_ATTEMPTS; ++ii)
		{
			const ITEM_QUALITY_DATA * quality_data = (const ITEM_QUALITY_DATA *)ExcelGetData(game, DATATABLE_ITEM_QUALITY, nQuality);

			ASSERT_RETINVALID(nQuality != quality_data->nDowngrade);
			if (quality_data->nDowngrade == INVALID_LINK)
			{
				break;
			}
			if (!TESTBIT(item_data->pdwQualities, quality_data->nDowngrade))
			{
				break;
			}

			nQuality = quality_data->nDowngrade;
		}
	}

	return nQuality;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int s_TreasureSimplePickQuality(
	GAME * game,
	int nLevel,
	const UNIT_DATA * item_data)
{
	TREASURE_MOD_LIST tDummyModList;
	return s_sTreasurePickQuality(game, NULL, nLevel, item_data, tDummyModList, NULL);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemMeetsTypeReqs(
	GAME * game,
	const TREASURE_PICK * pick_data,
	int nItemClass,
	const UNIT_DATA * item_data)
{
	if (pick_data->eCategory != TC_CATEGORY_UNITTYPE)
	{
		return TRUE;
	}

	int iIndexIndex = 0;
	int bMatchedAType = 2;		// this is whether we've found a match on any of the item types we're looking for
								//  i'm using a kind of ternary value - 2 is 'not set'
	while (pick_data->nIndex[iIndexIndex] != INVALID_LINK && iIndexIndex < MAX_CATEGORY_INDICIES)
	{
		// This is saying the item must be of this type
		if (pick_data->eOperator[iIndexIndex] == PICK_OP_NOOP)
		{
			if (bMatchedAType == 2)			// if it hasn't been set yet, and it turns out we have a type requirement, set it to false since we haven't met that requirement yet
			{
				bMatchedAType = FALSE;		
			}
			if (UnitIsA(item_data->nUnitType, pick_data->nIndex[iIndexIndex]))
			{
				bMatchedAType = TRUE;
			}
		}

		// This is saying skip items of this type
		if (pick_data->eOperator[iIndexIndex] == PICK_OP_NOT &&	
			UnitIsA(item_data->nUnitType, pick_data->nIndex[iIndexIndex]))
		{
			bMatchedAType = FALSE;
			break;
		}

		iIndexIndex++;
	}

	return bMatchedAType;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sItemMeetsTypeReqs(
	GAME * game,
	const TREASURE_MOD_LIST & tModList,
	int nItemClass,
	const UNIT_DATA * item_data)
{
	for (int ii = 0; ii < tModList.nNumModifiers; ++ii)
	{
		if (!s_sItemMeetsTypeReqs(game, &(tModList.pModifiers[ii]), nItemClass, item_data))
		{
			return FALSE;
		}
	}

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
float s_sGetItemTypeRarityMod(
	GAME * game,
	const UNIT_DATA * item_data,
	const TREASURE_MOD_LIST & tModList)
{
	int nMod = 100;

	for (int ii = 0; ii < tModList.nNumModifiers; ii++)
	{
		if (tModList.pModifiers[ii].eCategory == TC_CATEGORY_UNITTYPE_MOD)
		{
			int iIndexIndex = 0;
			while (tModList.pModifiers[ii].nIndex[iIndexIndex] != INVALID_LINK && iIndexIndex < MAX_CATEGORY_INDICIES)
			{
				if (UnitIsA(item_data->nUnitType, tModList.pModifiers[ii].nIndex[iIndexIndex]))
				{
					nMod += (tModList.pModifiers[ii].nValue - 100);
				}

				iIndexIndex++;
			}
		}
	}
	
	return MAX((float)nMod / 100.0f, 0.0f);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static float s_sTreasureGetMoneyChanceMultiplier(
	const TREASURE_DATA * treasure_data)
{
	return treasure_data->flMoneyChanceMultiplier;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static float s_sTreasureGetMoneyLuckChanceMultiplier(
	const TREASURE_DATA * treasure_data)
{
	return treasure_data->flMoneyLuckChanceMultiplier;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static float s_sTreasureGetMoneyAmountMultiplier(
	const TREASURE_DATA * treasure_data)
{
	return treasure_data->flMoneyAmountMultiplier;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static UNIT * s_sTreasureSpawnPickup(
	UNIT * pUnit,
	UNIT * pItem,
	BOOL bStartingItem)
{
	// if the owner is ready to receive messages this is a create/pickup in the 
	if (UnitCanSendMessages(pUnit))
	{
		ASSERTX_RETNULL(UnitGetRoom( pUnit ), "Trying to treasure spawn and pickup into a container that is not in the world but the container thinks its ready to send network messages" );
		
		// item is ready for messages
		UnitSetCanSendMessages( pItem, TRUE );

		// for now, put item in world and pick it up
		UnitWarp( 
			pItem,
			UnitGetRoom( pUnit ),
			UnitGetPosition( pUnit ),
			UnitGetFaceDirection( pUnit, FALSE ),
			UnitGetVUpDirection( pUnit ),
			0,
			FALSE);
	}

	// pick up item
	UNIT *pPickedUpItem	= s_ItemPickup( pUnit, pItem );
	
	// must have the item in the inventory if it was not stacked during the pickup
	if (pPickedUpItem != NULL)
	{
		UNIT *pContainer = UnitGetContainer( pPickedUpItem );
		ASSERTV( 
			pContainer == pUnit, 
			"Treasure spawn pickup error. Spawned item '%s' expected to be contained by '%s', but is instead contained by '%s'",
			UnitGetDevName( pPickedUpItem ),
			UnitGetDevName( pUnit ),
			pContainer ? UnitGetDevName( pContainer ) : "NOBODY");			
	}
	
	// return the picked up item (or the item it was stack absorbed into)
	return pPickedUpItem;				
	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sTreasureCheckNegateOnQuality(
	int quality,
	const TREASURE_MOD_LIST & tModList)
{
	// Check for a negate modifier on item quality
	for (int nModifier = 0; nModifier < tModList.nNumModifiers; ++nModifier)
	{
		int iIndexIndex = 0;
		const TREASURE_PICK * pick_data = &(tModList.pModifiers[nModifier]);
		ASSERT_RETFALSE(pick_data);
		while (pick_data->nIndex[iIndexIndex] != INVALID_LINK && iIndexIndex < MAX_CATEGORY_INDICIES)
		{
			if (pick_data->nIndex[iIndexIndex] == quality)		// if this is the quality type we're modifying
			{
				if (pick_data->eOperator[iIndexIndex] == PICK_OP_NEGATE)
				{
					return TRUE;
				}
			}

			iIndexIndex++;
		}
	}
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
// find a spawn for the treasure from the open node list (if provided)
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sTreasureGetSpawnPosition(
	GAME * game,
	ITEM_SPEC & spec,
	VECTOR & position)
{
	if (spec.pOpenNodes)
	{
		position = OpenNodeGetNextWorldPosition(game, spec.pOpenNodes);
		spec.pvPosition = &position;
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTreasureClassCanDrop(
	GAME * game,
	const TREASURE_DATA * treasure,
	ITEM_SPEC *spec)
{
	// if this is a subscriber-only TC and the player isn't a subscriber, well...
	if( AppIsHellgate() &&
		TreasureDataTestFlag( treasure, TDF_SUBSCRIBER_ONLY ) &&
		spec->unitOperator && 
		UnitIsA( spec->unitOperator, UNITTYPE_PLAYER ) && 
		!PlayerIsSubscriber(spec->unitOperator))
	{
		return FALSE;
	}

	if(TreasureDataTestFlag( treasure, TDF_MULTIPLAYER_ONLY ) &&
		AppIsSinglePlayer())
	{
		return FALSE;
	}

	if(TreasureDataTestFlag( treasure, TDF_SINGLE_PLAYER_ONLY ) &&
		AppIsMultiplayer())
	{
		return FALSE;
	}

	if(treasure->nGlobalThemeRequired >= 0 &&
		!GlobalThemeIsEnabled(game, treasure->nGlobalThemeRequired) &&
	   TESTBIT(spec->dwFlags, ISF_IGNORE_GLOBAL_THEME_RULES_BIT ) == FALSE)
	{
		return FALSE;
	}

	if(treasure->nSourceLevelTheme != INVALID_LINK )
	{
		ASSERT_RETFALSE(spec->unitOperator);
		LEVEL *pLevel = UnitGetLevel(spec->unitOperator);
		if(!pLevel || !LevelHasThemeEnabled(pLevel, treasure->nSourceLevelTheme))
		{
			return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanDropItem(
	GAME *pGame,
	const UNIT_DATA *pItemData,
	UNIT *pSpawner,
	ITEM_SPEC *pItemSpec,
	BOOL bSpawningSpecificItemClass)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pItemData, "Expected item data" );
	ASSERTX_RETFALSE( pItemSpec, "Expected item spawn spec" );

	if ( pItemSpec->unitOperator && 
		UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY ) &&
		UnitIsA( pItemSpec->unitOperator, UNITTYPE_PLAYER ) &&
		! (UnitHasBadge( pItemSpec->unitOperator, ACCT_TITLE_SUBSCRIBER )|| 
			(UnitHasBadge(pItemSpec->unitOperator, ACCT_TITLE_TEST_CENTER_SUBSCRIBER) && AppTestFlag(AF_TEST_CENTER_SUBSCRIBER))))
	{
		return FALSE;
	}

#if !ISVERSION(DEVELOPMENT)
	if (AppIsSinglePlayer() && UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY))
	{
		return FALSE;
	}
#endif

	// check for must be usable by spawner
	if (TESTBIT( pItemSpec->dwFlags, ISF_USEABLE_BY_SPAWNER_BIT ))
	{
		if (pSpawner)
		{
			if (InventoryItemMeetsClassReqs( pItemData, pSpawner ) == FALSE)
			{
				return FALSE;
			}
		}
	}
	
	// check for usable by operator
	if (TESTBIT( pItemSpec->dwFlags, ISF_USEABLE_BY_OPERATOR_BIT ))
	{
		UNIT *pOperator = pItemSpec->unitOperator;
		if (pOperator)
		{
			if (InventoryItemMeetsClassReqs( pItemData, pOperator ) == FALSE)
			{
				return FALSE;
			}
		}
	}

	if (TESTBIT( pItemSpec->dwFlags, ISF_USEABLE_BY_ENABLED_PLAYER_CLASS ))
	{
		// if no enabled player class can use this item, return false
		BOOL bUseableByEnabledPlayerClass = FALSE;
		int nPlayerClassMax = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_PLAYERS );
		for (int ii = 0; ii < NUM_CONTAINER_UNITTYPES && !bUseableByEnabledPlayerClass; ii++)
		{
			int nReqClass = pItemData->nContainerUnitTypes[ii];
			if (nReqClass > 0)	
			{
				// a specific unittype is required, check if the player exists
				for ( int jj = 0; jj < nPlayerClassMax && !bUseableByEnabledPlayerClass; jj++ )
				{
					UNIT_DATA * player_data = (UNIT_DATA *)ExcelGetData( pGame, DATATABLE_PLAYERS, jj );
					if (player_data == NULL)
						continue;

					if ( UnitIsA(player_data->nUnitType, nReqClass) )
					{
						bUseableByEnabledPlayerClass = TRUE;
					}
				}
			}
		}
		if (!bUseableByEnabledPlayerClass)
			return FALSE;

	}

	if (bSpawningSpecificItemClass == FALSE &&
		!UnitDataTestFlag( pItemData, UNIT_DATA_FLAG_SPAWN_AT_MERCHANT ) &&
		TESTBIT( pItemSpec->dwFlags, ISF_AVAILABLE_AT_MERCHANT_ONLY_BIT ))
	{
		return FALSE;
	}

	if ( pItemData->nLevelThemeRequired != INVALID_ID )
	{
		BOOL bCouldTestTheme = FALSE;
		if ( pItemSpec->unitOperator )
		{
			LEVEL * pLevel = UnitGetLevel( pItemSpec->unitOperator );
			if ( pLevel )
			{
				bCouldTestTheme = TRUE;
				if ( !LevelHasThemeEnabled( pLevel, pItemData->nLevelThemeRequired ) )
					return FALSE;
			}
		}
		if ( ! bCouldTestTheme )
			return FALSE;
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
TREASURE_RESULT s_sTreasureSpawnByItemClass(
	GAME * game,
	UNIT * spawner,
	int nItemClass,
	int spawnlevel,
	const TREASURE_DATA * /*pTreasureData*/,
	const TREASURE_MOD_LIST & tModList,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult)
{
	ASSERT_RETVAL(spec, RESULT_DROP_FAILED);
	ASSERT_RETVAL(spawnlevel > 0, RESULT_DROP_FAILED);

	const UNIT_DATA * item_data = ItemGetData(nItemClass);
	ASSERT_RETVAL(item_data, RESULT_DROP_FAILED);
	int quality = INVALID_LINK;
	int qualityToCheckAgainstLuck = INVALID_LINK;

	if (spec->nLuckModifier != 0)
	{
		qualityToCheckAgainstLuck = s_sTreasurePickQuality(game, spawner, spawnlevel, item_data, tModList, spec, false );
		quality = s_sTreasurePickQuality(game, spawner, spawnlevel, item_data, tModList, spec, true );
	}
	else
	{
		quality = s_sTreasurePickQuality(game, spawner, spawnlevel, item_data, tModList, spec);
	}

	if (quality == INVALID_LINK && qualityToCheckAgainstLuck == INVALID_LINK )
	{
		#if ISVERSION(DEVELOPMENT)
			ConsoleString(CONSOLE_ERROR_COLOR, "Could not find a valid quality for item '%s'", item_data->szName);
		#endif
		TraceTreasure( "Could not find a valid quality for item '%s'", item_data->szName);
		return RESULT_DROP_FAILED;
	}
	if ( quality == INVALID_LINK )
	{
		quality = qualityToCheckAgainstLuck;
	}

	if (s_sTreasureCheckNegateOnQuality(quality, tModList))
	{
		return RESULT_NO_DROP;
	}

	// filter
	if (sCanDropItem( game, item_data, spawner, spec, TRUE ) == FALSE)
	{
		return RESULT_NO_DROP;
	}
	
	if ( spec->nLuckModifier != 0)
	{
		//ok luck must of happened
		if (quality > qualityToCheckAgainstLuck)
		{		
			spec->iLuckModifiedItem += (quality - qualityToCheckAgainstLuck);
		}

		// what is this? bascially we got a better roll without luck. Might as well give it to the player if our luck is high enough
		if (spec->pLuckOwner && RandGetFloat(game) < spec->nLuckModifier)
		{
			if (quality < qualityToCheckAgainstLuck)
			{
				quality = qualityToCheckAgainstLuck;
				spec->iLuckModifiedItem += (quality - qualityToCheckAgainstLuck);
			}
		}
	}
	
	// well, we want to spawn with this quality, but we can't change the spec because it's used throughout
	// the whole treasure drop.  So make a new one with this quality just for this spawn.
	VECTOR vSpawnPosition;
	ITEM_SPEC tSpecCopy;	
	MemoryCopy(&tSpecCopy, sizeof(ITEM_SPEC), spec, sizeof(ITEM_SPEC));
	tSpecCopy.nLevel = spawnlevel;
	tSpecCopy.nQuality = quality;
	s_sTreasureGetSpawnPosition(game, tSpecCopy, vSpawnPosition);

	// spawn the item
	tSpecCopy.nItemClass = nItemClass;
	tSpecCopy.pSpawner = spawner;
	if (TESTBIT(spec->dwFlags, ISF_REPORT_ONLY_BIT))
	{
		CLEARBIT(tSpecCopy.dwFlags, ISF_PLACE_IN_WORLD_BIT);
	}
	UNIT * item = s_ItemSpawn(game, tSpecCopy, spawnResult);
	if (!item)
	{
		return RESULT_DROP_FAILED;
	}

	if (LogGetLogging(TREASURE_LOG) || LogGetTrace(TREASURE_LOG))
	{
//		TraceTreasure("treasure created: %d\t%s\t%s", UnitGetExperienceLevel(item), item_data->szName, ExcelGetStringIndexByLine(game, DATATABLE_ITEM_QUALITY, spec->nQuality));
	}

	if (TESTBIT(spec->dwFlags, ISF_REPORT_ONLY_BIT))
	{
		UnitFree(item);
		return RESULT_ITEM_SPAWNED;
	}
	
	// pickup if instructed to
	if (TESTBIT(spec->dwFlags, ISF_PICKUP_BIT))
	{
		BOOL bStartingItem = TESTBIT( spec->dwFlags, ISF_STARTING_BIT );	
		s_sTreasureSpawnPickup(spawner, item, bStartingItem);
	}
	
	return RESULT_ITEM_SPAWNED;
	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static TREASURE_RESULT s_sTreasureSpawnByUnitType(
	GAME * game,
	UNIT *pSpawner,
	const TREASURE_DATA * treasure,
	int spawnlevel,
	const TREASURE_PICK * pick_data,
	const TREASURE_MOD_LIST & tModList,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult,
	int nDifficulty = 0,
	int nLevel = INVALID_ID)
{
	ASSERT_RETVAL(spec, RESULT_DROP_FAILED);
	ASSERT_RETVAL(pick_data, RESULT_DROP_FAILED);
	ASSERT_RETVAL(spawnlevel > 0, RESULT_DROP_FAILED);

	int item_count = ExcelGetNumRows(game, DATATABLE_ITEMS);

	PickerStartF(game, picker);
	BOOL bDropsOutOfRange = FALSE;
	UNIT *unitOperator = spec->unitOperator;
	for (int ii = 0; ii < item_count; ii++)
	{
		const UNIT_DATA * item_data = ItemGetData(ii);
		if ( ! item_data )
			continue;

		if (!UnitDataTestFlag( item_data, UNIT_DATA_FLAG_SPAWN ))
		{
			continue;
		}

		if (item_data->nRarity <= 0)
		{
			continue;
		}
		if (AppIsHellgate())
		{
			if (item_data->nItemExperienceLevel > spawnlevel || (item_data->nMaxLevel > 0 && item_data->nMaxLevel < spawnlevel))
			{
				continue;
			}
		}
		float fRarity = (float)item_data->nRarity * s_sGetItemTypeRarityMod(game, item_data, tModList);
		if (AppIsTugboat())
		{
			//float fRarity = (float)item_data->nRarity * s_sGetItemTypeRarityMod(game, item_data, tModList);

			if (unitOperator) //for quests
			{
				fRarity += QuestGetAdditionItemWeight(game, unitOperator, item_data);
			}
				
			//fRarity += item_data->nRarity * spec->nLuckModifier;
		}

		if (!s_sItemMeetsTypeReqs(game, pick_data, ii, item_data))
		{
			continue;
		}

		if (!s_sItemMeetsTypeReqs(game, tModList, ii, item_data))
		{
			continue;
		}

		if (sCanDropItem( game, item_data, pSpawner, spec, FALSE ) == FALSE)
		{
			continue;
		}
		if( AppIsTugboat() )
		{
			int nMinLevel = item_data->nMinSpawnLevel;
			int nMaxLevel = item_data->nMaxSpawnLevel;
			if ( spec && 
				(UnitIsMerchant(pSpawner) || spec->pMerchant) )
			{
				if (item_data->nMinMerchantLevel != INVALID_ID )
				{
					nMinLevel = item_data->nMinMerchantLevel;
				}
				if (item_data->nMaxMerchantLevel != INVALID_ID )
				{
					nMaxLevel = item_data->nMaxMerchantLevel;
				}
			}

			if (nMinLevel > spawnlevel)
			{
				bDropsOutOfRange = TRUE;
				continue;
			}
			if (nMaxLevel <= spawnlevel)
			{
				bDropsOutOfRange = TRUE;
				continue;
			}

		}		
		
		PickerAdd(MEMORY_FUNC_FILELINE(game, ii, fRarity));
	}

	if (PickerGetCount(game) <= 0)
	{
		return RESULT_DROP_IMPOSSIBLE;
	}

	int item_class = PickerChoose(game);

	// we tried to drop this, but it was out of level range -
	// in Tug we have a lot of depth-spawning specifics, 
	// and we don't want an assert if we can't drop a ring
	// because it's level 8, not 10
	if( bDropsOutOfRange &&
		item_class == INVALID_LINK )
	{
		return RESULT_NO_DROP;	// don't drop anything
	}

	if (item_class == INVALID_LINK)
	{	
		if (!treasure || TreasureDataTestFlag( treasure, TDF_RESULT_NOT_REQUIRED ) == FALSE)
		{
			ASSERTV_RETX(0, RESULT_DROP_IMPOSSIBLE, "Failed to find an item for the treasure class %s", treasure->szTreasure );
		}
		else
		{
			return RESULT_NO_DROP;	// don't drop anything
		}
	}

	// Now we need to check if there was a negate item type specified.  This means that if you had decided to drop
	//   an item of this type, don't drop anything.
	int iIndexIndex = 0;
	const UNIT_DATA * item_data = ItemGetData(item_class);
	while (pick_data->nIndex[iIndexIndex] != INVALID_LINK &&
		iIndexIndex < MAX_CATEGORY_INDICIES)
	{
		// This is saying the item must be of this type
		if (pick_data->eOperator[iIndexIndex] == PICK_OP_NEGATE &&	
			UnitIsA(item_data->nUnitType, pick_data->nIndex[iIndexIndex]))
		{
			return RESULT_NO_DROP;	// don't drop anything
		}
		iIndexIndex++;
	}

	return s_sTreasureSpawnByItemClass(game, pSpawner, item_class, spawnlevel, treasure, tModList, spec, spawnResult);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static TREASURE_RESULT s_sTreasureSpawnPick(
	GAME * game,
	UNIT * spawner,
	const TREASURE_DATA * treasure,
	int spawnlevel,
	int pick,
	const TREASURE_MOD_LIST & tModList,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult,
	int nDifficulty,
	int nLevel,
	int nTreasureLevelBoost)
{
	ASSERT_RETX(spec, RESULT_INVALID);
	if (pick < 0)
	{
		return RESULT_INVALID;
	}
	ASSERT_RETX(pick < MAX_TREASURE_CHOICES, RESULT_INVALID);

	const TREASURE_PICK * pick_data = treasure->pPicks + pick;
	switch (pick_data->eCategory)
	{
	case TC_CATEGORY_ITEMCLASS:
		{
			if( spawnlevel == INVALID_ID )
			{
				spawnlevel = s_sTreasureGetSpawnLevel(game, spawner, spec, nDifficulty, nLevel);
				spawnlevel += nTreasureLevelBoost;
				spawnlevel = MAX(1, spawnlevel);
			}
			return s_sTreasureSpawnByItemClass(game, spawner, pick_data->nIndex[0], spawnlevel, treasure, tModList, spec, spawnResult);
		}

	case TC_CATEGORY_UNITTYPE:
		{
			if( spawnlevel == INVALID_ID )
			{
				spawnlevel = s_sTreasureGetSpawnLevel(game, spawner, spec, nDifficulty, nLevel);
				spawnlevel += nTreasureLevelBoost;
				spawnlevel = MAX(1, spawnlevel);
			}
			return s_sTreasureSpawnByUnitType(game, spawner, treasure, spawnlevel, pick_data, tModList, spec, spawnResult, nDifficulty, nLevel);
		}

	case TC_CATEGORY_TREASURECLASS:
		{
			const TREASURE_DATA * treasure = TreasureGetData(game, pick_data->nIndex[0]);
			ASSERT_RETX(treasure, RESULT_INVALID);

			if (!sTreasureClassCanDrop(game, treasure, spec))
			{
				return RESULT_NO_DROP;
			}

			// copy the item spec (if present)
			ITEM_SPEC tItemSpecCopy = *spec;
			
			// create a new combined mod list to use for this spawn only
			const TREASURE_MOD_LIST tModListCopy = tModList + treasure->tModList;
			
			// do the spawn
			return s_sTreasureSpawn(game, spawner, treasure, spawnlevel, tModListCopy, &tItemSpecCopy, spawnResult, nDifficulty, nLevel, nTreasureLevelBoost);
			
		}
		break;
	case TC_CATEGORY_NODROP:
		return RESULT_NO_DROP;		// duh
		break;	
	}

	return RESULT_INVALID;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int s_sGetNumPicks(
	GAME * game,
	const TREASURE_DATA * treasure_data)
{
	ASSERT(treasure_data);

	if (treasure_data->nPickType == PICKTYPE_ALL)
	{
		// Picktype all ignores the 'picks' column and just sums all the weights for the total number
		int pick = 0;
		int nTotal = 0;
		while (pick < MAX_TREASURE_CHOICES )
		{
			if (sIsAPick(treasure_data->pPicks[pick]))
			{
				nTotal += treasure_data->pPicks[pick].nValue;
			}
			pick++;
		}
		return nTotal;
	}

	PickerStart(game, picker);
	for (int i=0; i < MAX_NUM_CHANCES; i++)
	{
		if (treasure_data->tNumPicksChances.pnNumPicks[i] >= 0 &&
			treasure_data->tNumPicksChances.pnWeights[i] > 0)
		{
			PickerAdd(MEMORY_FUNC_FILELINE(game, treasure_data->tNumPicksChances.pnNumPicks[i], treasure_data->tNumPicksChances.pnWeights[i]));
		}
	}
	return PickerChoose(game);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sTreasureSpawnMoney(
	GAME * game,
	UNIT * unit,
	const TREASURE_DATA * treasure,
	const TREASURE_MOD_LIST & tModList,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult )
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(spec);

	int spawnlevel = s_sTreasureGetSpawnLevel(game, unit, spec); 

	// copy the item spec so we can modify it
	ITEM_SPEC tSpec = *spec;
	tSpec.nLevel = spawnlevel;

	// take money out of all specs
	int nMoneyAmount = tSpec.nMoneyAmount;
	int nRealWorldMoneyAmount = tSpec.nRealWorldMoneyAmount;
	REF( nRealWorldMoneyAmount );
	tSpec.nMoneyAmount = 0;
	tSpec.nRealWorldMoneyAmount = 0;
	spec->nMoneyAmount = 0;
	spec->nRealWorldMoneyAmount = 0;
			
	/*
	int nNumItems = ExcelGetNumRows(game, DATATABLE_ITEMS);
	PickerStart(game, picker);
	const UNIT_DATA *pItemData = NULL;
	int nMoneyTypesFound = 0;
	for (int ii = 0; ii < nNumItems; ii++)
	{
		pItemData = ItemGetData(ii);
		if ( ! pItemData )
			continue;

		if (!UnitIsA(pItemData->nUnitType, UNITTYPE_MONEY))
		{
			continue;
		}
		if (pItemData->nItemExperienceLevel > spawnlevel)
		{
			continue;
		}
		if (pItemData->nRarity <= 0)
		{
			continue;
		}

		PickerAdd(MEMORY_FUNC_FILELINE(game, ii, pItemData->nRarity));
		nMoneyTypesFound++;
		
	}
	ASSERTX_RETURN(nMoneyTypesFound > 0, "No money items found!");
	*/
	//lets just check the global item.
	const UNIT_DATA *pMoneyType = ItemGetData( GlobalIndexGet(GI_ITEM_MONEY) );
	ASSERTX_RETURN( pMoneyType && UnitIsA(pMoneyType->nUnitType, UNITTYPE_MONEY), "No money type found!");

	// get the baseline money min/max for a spawn at this level
	float flMoneyChance = 0.0f;
	float flMoneyBaselineMin = 0.0f;
	float flMoneyBaselineMax = 0.0f;
	{
		int monlevel = PIN(spawnlevel, 1, MonsterGetMaxLevel(game));

		const MONSTER_LEVEL * monlvldata = MonsterLevelGetData(game, monlevel);
		if (monlvldata)
		{
			flMoneyChance = (float)monlvldata->nMoneyChance;
			flMoneyBaselineMin = (float)monlvldata->nMoneyMin;
			flMoneyBaselineMax = flMoneyBaselineMin + (float)monlvldata->nMoneyDelta;		
		}
	}

	// find the money amount to spawn		
	if (nMoneyAmount < 0)		// if no money amount has been pre-specified
	{
		// monsters can modify the chance
		if (UnitIsA(unit, UNITTYPE_MONSTER))
		{
			float flMultiplier = MonsterGetMoneyChanceMultiplier( unit );
			flMoneyChance *= flMultiplier;
		}

		// modify money chance by treasure class multiplier
		float flChanceMultiplier = s_sTreasureGetMoneyChanceMultiplier(treasure);
		/*if (AppIsTugboat())
		{
			flChanceMultiplier += s_sTreasureGetMoneyLuckChanceMultiplier(treasure) * spec->nLuckModifier;
		}*/
		flMoneyChance *= flChanceMultiplier;	
	
		// apply money chance override
#if ISVERSION(CHEATS_ENABLED)
		if (gflMoneyChanceOverride >= 0)
		{
			flMoneyChance = gflMoneyChanceOverride;
		}
#endif
		
		// roll for getting any money	
		int nMoneyChance = (int)flMoneyChance;
		int nMoneyRoll = RandGetNum(game, 1, 100);
		if (nMoneyRoll <= nMoneyChance)
		{					
			// get baseline money amount 
			float flMoneyAmount = RandGetFloat(game, flMoneyBaselineMin, flMoneyBaselineMax);

			// monsters can modify the amount
			if (UnitIsA( unit, UNITTYPE_MONSTER ))
			{
				float flMultiplier = MonsterGetMoneyAmountMultiplier( unit );
				flMoneyAmount *= flMultiplier;
			}
			
			// modify money amount by treasure class multiplier
			float flAmountMultiplier = s_sTreasureGetMoneyAmountMultiplier(treasure);
			flMoneyAmount *= flAmountMultiplier;
		
			// save money
			nMoneyAmount = (int)CEIL( flMoneyAmount );
		}
	}

	//Tugboat - Misfortune works different on money. It effects the amount that drops, not the chance of drop...
	if( AppIsTugboat() &&
		spec->unitOperator &&
		UnitIsA( spec->unitOperator, UNITTYPE_PLAYER ) )
	{
		int misfortune = UnitGetStat( spec->unitOperator, STATS_MISFORTUNE );
		nMoneyAmount -= PCT( nMoneyAmount, misfortune ); //subtract that money!
	}

	//Hellgate - this is basically misfortune, but managed separately for the anti-fatigue system for minors in China
	if( AppIsHellgate() &&
		spec->unitOperator &&
		UnitIsA( spec->unitOperator, UNITTYPE_PLAYER ) )
	{
		int misfortune = UnitGetStat( spec->unitOperator, STATS_ANTIFATIGUE_XP_DROP_PENALTY_PCT );
		nMoneyAmount -= PCT( nMoneyAmount, misfortune ); //subtract that money!
	}

	// are we creating money
	if (nMoneyAmount > 0)
	{
		// we probably don't ever want to spawn money for monsters and have them pick
		// it up ... in fact, the only reason I can see to auto pickup money would be
		// to give a starting amount to players -Colin
		if (TESTBIT( spec->dwFlags, ISF_PICKUP_BIT ) == TRUE &&
			UnitIsA( unit, UNITTYPE_PLAYER ) == FALSE)
		{
			if( AppIsHellgate() )
			{
				const int MAX_MESSAGE = 1024;
				char szMessage[ MAX_MESSAGE ];
				PStrPrintf( 
					szMessage, 
					MAX_MESSAGE, 
					"Treasure class '%s' is telling a non player unit '%s' to auto pickup money, you probably don't want the money spawned in the first place, tell the treasure.xls spreadsheet.\n",
					treasure->szTreasure,
					UnitGetDevName( unit ));
				ASSERTX_RETURN( 0, szMessage );
			}
			//No reason to give money to NPCs. But why assert? So many factors come into figuring out money, lets not complicate it for NPC's by asserting. just exit. 
			return;
		}
		
		// log that we're creating money!
		if (LogGetLogging(TREASURE_LOG))
		{
			TraceTreasure( "\t\t%d money\t\t",  nMoneyAmount );
		}

		// create it	
		if (!TESTBIT(tSpec.dwFlags, ISF_REPORT_ONLY_BIT))
		{	
			int nNumMoneyPiles = 1;
			
			// if the money amount is over a threshold value of money given this spawn level we will
			// create additional piles (cause it's fun to pickup lots of money dammit!)
			int nThreshold = (int)(flMoneyBaselineMax * 0.6f);
			if (nThreshold > 0 && nMoneyAmount > nThreshold)
			{
				nNumMoneyPiles = (nMoneyAmount / (nThreshold * 2)) + 1;
			}

			// we have a max money piles
			const int MAX_MONEY_PILES = 16;
			nNumMoneyPiles = PIN( nNumMoneyPiles, 1, MAX_MONEY_PILES );
			
			// how much will we vary each pile we create
			int nMoneyPerPile = nMoneyAmount / nNumMoneyPiles;
			int nPileVariation = (nMoneyPerPile / 8) + 1;
			int nMoneyPerPileMin = nMoneyPerPile - nPileVariation;
			int nMoneyPerPileMax = nMoneyPerPile + nPileVariation;
									
			// create all money piles
			int nMoneyLeft = nMoneyAmount;
			//pick a type of money to create (one day there might be different gfx variations of money) - NOTE changed this to use just money. Later maybe
			//we'll switch it back.
			int nMoneyClass = GlobalIndexGet(GI_ITEM_MONEY); // PickerChoose(game);
			while (nMoneyLeft)
			{
				// how much money is in this pile
				int nMoneyInThisPile = RandGetNum( game, nMoneyPerPileMin, nMoneyPerPileMax );
				nMoneyInThisPile = PIN( nMoneyInThisPile, 1, nMoneyLeft );
									
				// take the money away from our big pile
				nMoneyLeft -= nMoneyInThisPile;

				// if there is only a wee little bit of money left after this pile, forget it
				// and just put it in with this pile
				if (nMoneyLeft <= nPileVariation)
				{
					nMoneyInThisPile += nMoneyLeft;
					nMoneyLeft = 0;
				}
				
				// set money value into spec
				tSpec.nMoneyAmount = nMoneyInThisPile;


										
				// create money unit
				s_sTreasureSpawnByItemClass(game, unit, nMoneyClass, spawnlevel, treasure, tModList, &tSpec, spawnResult);
			}		
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static float s_sTreasureGetNoDrop(
	GAME * game,
	const TREASURE_DATA * treasure,
	UNIT * spawner)
{
	float fNoDrop = treasure->fNoDrop;
	
	int nTreasureBonus = spawner ? UnitGetStat(spawner, STATS_LOOT_BONUS_PERCENT) : 0;
	const MONSTER_SCALING * monster_scaling = MonsterGetScaling(game, GameGetClientCount(game));
	if (monster_scaling)
		nTreasureBonus += monster_scaling->nTreasure;

	if (nTreasureBonus > 0 && nTreasureBonus != 100)
	{
		float fDrop = 100.0f - fNoDrop;
		if (fDrop > 0.0f)
		{
			fDrop = fDrop * (float)nTreasureBonus / 100.0f;
			fNoDrop = 100.0f - fDrop;
			if (fNoDrop <= 0.0f)
			{
				fNoDrop = 0.0f;
			}
		}
	}

	return fNoDrop;
}
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sStackResults( 
	ITEMSPAWN_RESULT *pSpawnResult)
{
	ASSERTX_RETURN( pSpawnResult, "Expected spawn result" );
	
	// go through each result
	for (int i = 0; i < pSpawnResult->nTreasureBufferCount; ++i)
	{
		UNIT *pItem = pSpawnResult->pTreasureBuffer[ i ];
		
		// scan each of the remaining items in the treasure buffer
		for (int j = i + 1; j < pSpawnResult->nTreasureBufferCount; ++j)
		{
			UNIT *pItemOther = pSpawnResult->pTreasureBuffer[ j ];
			
			if (ItemsAreIdentical( pItem, pItemOther ))
			{
				if (ItemStack( pItem, pItemOther ) == ITEM_STACK_COMBINE)
				{
				
					// item has been stacked, fill in the hold at pItemOther with the
					// end of the item spawn result buffer
					if (j < pSpawnResult->nTreasureBufferCount - 1)
					{
						pSpawnResult->pTreasureBuffer[ j ] = pSpawnResult->pTreasureBuffer[ pSpawnResult->nTreasureBufferCount - 1 ];
						pSpawnResult->pTreasureBuffer[ pSpawnResult->nTreasureBufferCount - 1 ] = NULL;
						j--;  // do this index over again
					}
					
					// we've used up another item
					pSpawnResult->nTreasureBufferCount--;
					
				}
				
			}
			
		}
		
	}
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpecSetOptionsFromTreasureData(
	ITEM_SPEC *pSpec,
	const TREASURE_DATA *pTreasureData)
{	
	ASSERTX_RETURN( pSpec, "Expected item spec" );
	ASSERTX_RETURN( pTreasureData, "Expected treasure data" );
	
	// set options for usable by spawner and/or operator
	if (TreasureDataTestFlag( pTreasureData, TDF_REQUIRED_USABLE_BY_SPAWNER ))
	{
		SETBIT( pSpec->dwFlags, ISF_USEABLE_BY_SPAWNER_BIT );
	}
	if (TreasureDataTestFlag( pTreasureData, TDF_REQUIRED_USABLE_BY_OPERATOR ))
	{
		SETBIT( pSpec->dwFlags, ISF_USEABLE_BY_OPERATOR_BIT );
	}
	if (TreasureDataTestFlag( pTreasureData, TDF_BASE_LEVEL_ON_PLAYER ))
	{
		SETBIT( pSpec->dwFlags, ISF_BASE_LEVEL_ON_PLAYER_BIT );
	}
	
	// some treasure classes spawn items with their max slots
	if (TreasureDataTestFlag( pTreasureData, TDF_MAX_SLOTS ))
	{
		SETBIT(pSpec->dwFlags, ISF_MAX_SLOTS_BIT);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static TREASURE_RESULT s_sTreasureSpawn(
	GAME * game,
	UNIT * spawner,
	const TREASURE_DATA * treasure,
	int spawnlevel,
	const TREASURE_MOD_LIST & tModList,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult,
	int nDifficulty,
	int nLevel,
	int nTreasureLevelBoost)
{
//	TIMER_STARTEX("s_sTreasureSpawn()", 2);

	ASSERT_RETVAL(game, RESULT_INVALID);
	ASSERT_RETVAL(spec, RESULT_INVALID);
	ASSERT_RETVAL(treasure, RESULT_INVALID);

	//if (LogGetLogging(TREASURE_LOG))
	//	LogMessage(TREASURE_LOG,"    Dropping treasure class '%s'", treasure->szTreasure);

	// setup options based on this treasure class
	sSpecSetOptionsFromTreasureData( spec, treasure );
	
	// for now, decrease the no drop based on players in the game, need to change this later
	float fNoDrop = s_sTreasureGetNoDrop(game, treasure, spawner);	

	// get the treasure level boosts
	nTreasureLevelBoost += s_sTreasureGetLevelBoost(game, spawner, treasure);

	//Tugboat - Misfortune will increase the chance for no drop
	//for party...
	if( AppIsTugboat() &&
		spec->unitOperator &&
		UnitIsA( spec->unitOperator, UNITTYPE_PLAYER ) )
	{
		int misfortune = UnitGetStat( spec->unitOperator, STATS_MISFORTUNE );
		if( fNoDrop != 0 ) //drop of 0 is always has to happen - for instance a key!
		{
			fNoDrop += (float)PCT( PIN( PrimeFloat2Int( 100.f - fNoDrop ), 0, 100 ), misfortune ); //No drop for you!!
		}
	}

	//Hellgate - this is basically misfortune, but managed separately for the anti-fatigue system for minors in China
	if( AppIsHellgate() &&
		spec->unitOperator &&
		UnitIsA( spec->unitOperator, UNITTYPE_PLAYER ) )
	{
		int misfortune = UnitGetStat( spec->unitOperator, STATS_ANTIFATIGUE_XP_DROP_PENALTY_PCT );
		fNoDrop += (float)PCT( PIN( PrimeFloat2Int( 100.f - fNoDrop ), 0, 100 ), misfortune ); //No drop for you!!
	}

	if( !sTreasureClassCanDrop(game, treasure, spec) )
	{
		return RESULT_NO_DROP;
	}

	if (RandGetFloat(game, 100) <= fNoDrop)
	{
		return RESULT_NO_DROP;
	}

	int nPicksToMake = s_sGetNumPicks(game, treasure);
	if (nPicksToMake <= 0)
	{
		return RESULT_NO_DROP;
	}
	
	// some treasure classes can only be evaluated by a subset of unit types, for everybody else
	// this treasure class will evaluate to nothing
	if (treasure->nAllowUnitTypes[0] != 0 && spec->unitOperator)
	{
		if (IsAllowedUnit(spec->unitOperator, treasure->nAllowUnitTypes, TREASURE_ALLOWED_UNITTYPES) == FALSE)
		{
			return RESULT_NO_DROP;
		}	
	}

	if(treasure->nSourceUnitType != INVALID_LINK)
	{
		if(!UnitIsA(spawner, treasure->nSourceUnitType))
		{
			return RESULT_NO_DROP;
		}
	}

	// stuff from merchants is always identified
	if ( (UnitIsMerchant( spawner ) || spec->pMerchant) &&
		 !UnitIsGambler( spawner ) && !( spec->pMerchant && UnitIsGambler( spec->pMerchant ) ) )
	{
		SETBIT( spec->dwFlags, ISF_IDENTIFY_BIT );
	}
		
	if( AppIsTugboat() &&
		(UnitIsMerchant(spawner) || spec->pMerchant) )
	{
		const UNIT_DATA* pUnitData = UnitGetData( spawner );
		int DepthMin = pUnitData->nMinMerchantLevel;
		int DepthMax = pUnitData->nMaxMerchantLevel;
		
		spawnlevel = spec->nLevel = RandGetNum(game, DepthMin, DepthMax);

	}

	BOOL bPicksMade[MAX_TREASURE_CHOICES];
	memclear(bPicksMade, sizeof(bPicksMade));

	for (int ii = 0; ii < nPicksToMake; ii++)
	{
		switch (treasure->nPickType)
		{
		case PICKTYPE_RAND:
		case PICKTYPE_RAND_ELIMINATE:
			{
				int pick = 0;
				PickerStart(game, picker);
				while (pick < MAX_TREASURE_CHOICES )
				{
					if (sIsAPick(treasure->pPicks[pick]) &&
						(treasure->nPickType != PICKTYPE_RAND_ELIMINATE || bPicksMade[pick] == FALSE))
					{
						// we have to check if this is a subscriber-only treasure class pick
						BOOL bOk = TRUE;
						const TREASURE_PICK * pick_data = treasure->pPicks + pick;
						if (pick_data->eCategory == TC_CATEGORY_TREASURECLASS)
						{
							const TREASURE_DATA * pTC = TreasureGetData(game, pick_data->nIndex[0]);
							if (!pTC || !sTreasureClassCanDrop(game, pTC, spec))
							{
								bOk = FALSE;
							}
						}
						
						if (bOk)
							PickerAdd(MEMORY_FUNC_FILELINE(game, pick, treasure->pPicks[pick].nValue));
					}
					pick++;
				}
				ASSERT_RETVAL(!PickerIsEmpty(game), RESULT_INVALID);

				int picktodrop = PickerChoose(game);
				bPicksMade[picktodrop] = TRUE;
				s_sTreasureSpawnPick(game, spawner, treasure, spawnlevel, picktodrop, tModList, spec, spawnResult, nDifficulty, nLevel, nTreasureLevelBoost);
			}
			break;

		case PICKTYPE_ALL:
			{
				int pick = 0;
				int nTotal = 0;
				while (pick < MAX_TREASURE_CHOICES )
				{
					if (sIsAPick(treasure->pPicks[pick]))
					{
						nTotal += treasure->pPicks[pick].nValue;
						if (ii < nTotal)
						{
							s_sTreasureSpawnPick(game, spawner, treasure, spawnlevel, pick, tModList, spec, spawnResult, nDifficulty, nLevel, nTreasureLevelBoost);
							break;
						}
					}
					pick++;
				}
			}
			break;

		case PICKTYPE_FIRST_VALID:
			{
				int pick = 0;
				PickerStart(game, picker);
				while (pick < MAX_TREASURE_CHOICES )
				{
					if (sIsAPick(treasure->pPicks[pick]))
					{
						// we have to check if this is a subscriber-only treasure class pick
						BOOL bOk = TRUE;
						const TREASURE_PICK * pick_data = treasure->pPicks + pick;
						if (pick_data->eCategory == TC_CATEGORY_TREASURECLASS)
						{
							const TREASURE_DATA * pTC = TreasureGetData(game, pick_data->nIndex[0]);
							if (!pTC || !sTreasureClassCanDrop(game, pTC, spec))
							{
								bOk = FALSE;
							}
						}

						if (bOk)
						{
							PickerAdd(MEMORY_FUNC_FILELINE(game, pick, treasure->pPicks[pick].nValue));
							break;
						}
					}
					pick++;
				}
				ASSERT_RETVAL(!PickerIsEmpty(game), RESULT_INVALID);

				int picktodrop = PickerChoose(game);
				bPicksMade[picktodrop] = TRUE;
				s_sTreasureSpawnPick(game, spawner, treasure, spawnlevel, picktodrop, tModList, spec, spawnResult, nDifficulty, nLevel, nTreasureLevelBoost);
			}
			break;

		case PICKTYPE_INDEPENDENT_PERCENTS:
			{
				int pick = 0;
				while (pick < MAX_TREASURE_CHOICES )
				{
					if (treasure->pPicks[pick].nValue > 0 &&
						(int)RandGetNum(game, 0, 100) < treasure->pPicks[pick].nValue)
					{
						s_sTreasureSpawnPick(game, spawner, treasure, spawnlevel, pick, tModList, spec, spawnResult, 0, INVALID_ID, 0);
					}

					pick++;
				}
			}
			break;

		default:
			{
				char szBuf[2048];
				PStrPrintf(szBuf, 2048, "Treasure trying to pick on an invalid treasure class type - '%s'", treasure->szTreasure);
				ASSERTX_RETX(FALSE, szBuf, RESULT_INVALID);
			}
			break;
		}
	}

	// stack like items if instructed to by the treasure class
	if (TreasureDataTestFlag( treasure, TDF_STACK_TREASURE ))
	{
		s_sStackResults( spawnResult );
	}
	
	return RESULT_ITEM_SPAWNED;
}

TREASURE_RESULT s_TreasureSpawn(
	GAME * game,
	UNIT * spawner,
	const TREASURE_DATA * treasure,
	int spawnlevel,
	const TREASURE_MOD_LIST & tModList,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult,
	int nDifficulty,
	int nLevel,
	int nTreasureLevelBoost)
{
	return s_sTreasureSpawn(game, spawner, treasure, spawnlevel, tModList,
		spec, spawnResult, nDifficulty, nLevel, nTreasureLevelBoost);
}

#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_TreasureClassHasNoTreasure(
	const TREASURE_DATA * pTreasureData)
{
	ASSERT_RETTRUE(pTreasureData);

	if(pTreasureData->flMoneyChanceMultiplier > 0.0f)
	{
		return FALSE;
	}

	for(int i=0; i<MAX_TREASURE_CHOICES; i++)
	{
		if(pTreasureData->pPicks[i].eCategory != TC_CATEGORY_NONE)
		{
			return FALSE;
		}
	}

	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TreasureSpawnClass(
	UNIT * unit,
	int treasureclass,
	ITEM_SPEC* spec,
	ITEMSPAWN_RESULT* spawnResult,
	int nDifficulty /* = 0 */,
	int nLevel /* = INVALID_ID*/,
	int nSpawnLevel /* = INVALID_ID*/)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)

	ASSERT_RETURN(unit);
	GAME* pGame= UnitGetGame(unit);
	ASSERT_RETURN(pGame && IS_SERVER(pGame));

	if (treasureclass == INVALID_LINK)
	{
		return;
	}

	// get treasure definition
	const TREASURE_DATA* pTreasureData = TreasureGetData(pGame, treasureclass);
	ASSERT_RETURN(pTreasureData);

	if(s_TreasureClassHasNoTreasure(pTreasureData))
	{
		return;
	}

	// if there are no spawn options specified, use the default ones
	ITEM_SPEC specDefault;
	if (spec == NULL)
	{
		SETBIT( specDefault.dwFlags, ISF_PLACE_IN_WORLD_BIT );
		spec = &specDefault;
	}

	// set options in the spawn spec based on treasure data
	sSpecSetOptionsFromTreasureData( spec, pTreasureData );
	
	// if supposed to place in world, never equip
	if (TESTBIT(spec->dwFlags, ISF_PLACE_IN_WORLD_BIT))
	{
		CLEARBIT(spec->dwFlags, ISF_PICKUP_BIT);
	}

	// setup struct to track the spawned items
	UNIT *pTreasure[ MAX_TRACKED_LOOT_PER_DROP ];
	ITEMSPAWN_RESULT trackedLoot;
	trackedLoot.pTreasureBuffer = pTreasure;
	trackedLoot.nTreasureBufferSize = MAX_TRACKED_LOOT_PER_DROP;

	// get a bunch of room nodes around the spawn location
	OPEN_NODES tOpenNodes;		
	if ( !TESTBIT(spec->dwFlags, ISF_PICKUP_BIT) &&
		 unit && 
		 UnitGetRoom( unit ) != NULL)
	{
		NEAREST_NODE_SPEC tSpec;
		SETBIT(tSpec.dwFlags, NPN_SORT_OUTPUT);
		SETBIT(tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES);
		SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
		SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
		SETBIT(tSpec.dwFlags, NPN_USE_XY_DISTANCE);
		SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
		if(AppIsHellgate())
		{
			SETBIT(tSpec.dwFlags, NPN_CHECK_LOS);
		}
		tSpec.pUnit = unit;
		tSpec.fMaxDistance = 3.0f;

		const int MAX_PATH_NODES = 64;
		FREE_PATH_NODE tFreePathNodes[ MAX_PATH_NODES ];

		VECTOR vTestPosition = UnitGetPosition(unit);
		if(!UnitIsOnGround(unit) || UnitGetStat(unit, STATS_CAN_FLY))
		{
			const float flSlop = 0.05f;
			vTestPosition.fZ += flSlop;
			float fDistance = LevelLineCollideLen(pGame, UnitGetLevel(unit), vTestPosition, VECTOR(0, 0, -1.0f), 100.0f);
			vTestPosition.fZ -= (fDistance - flSlop);
		}

		int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(unit), vTestPosition, MAX_PATH_NODES, tFreePathNodes, &tSpec);

		// setup position structure for the spawn spec
		if (nNumNodes > 0)
		{
			tOpenNodes.pFreePathNodes = tFreePathNodes;
			tOpenNodes.nNumPathNodes = nNumNodes;
			tOpenNodes.nCurrentNode = 0;
			spec->pOpenNodes = &tOpenNodes;
		}

	}
			
	// drop money at the top (note that we don't evaluate money for all sub-treasure classes
	// cause that would cause us to try to drop money too many times)
	s_sTreasureSpawnMoney(pGame, unit, pTreasureData, pTreasureData->tModList, spec, &trackedLoot);

	// do spawn
	s_sTreasureSpawn(pGame, unit, pTreasureData, nSpawnLevel, pTreasureData->tModList, spec, &trackedLoot, nDifficulty, nLevel, 0);

	// copy to the spawn result passed in if any
	if (spawnResult)
	{
		for (int i = 0; i < trackedLoot.nTreasureBufferCount; i++)
		{
			// put in buffer if it can fit
			ASSERTX_BREAK( i <= spawnResult->nTreasureBufferSize, "Treasure buffer too small for treasure spawned" );
			spawnResult->pTreasureBuffer[ i ] = trackedLoot.pTreasureBuffer[ i ];
			spawnResult->nTreasureBufferCount++;
		}
	}
	
	// if the spawner is supposed to tell the quest system any loot it drops, do so now
	const UNIT_DATA *pUnitDataSpawner = UnitGetData( unit );
	if (UnitDataTestFlag(pUnitDataSpawner, UNIT_DATA_FLAG_INFORM_QUESTS_OF_LOOT_DROP))
	{
		s_QuestEventLootSpawned( unit, &trackedLoot );
	}
	
#endif
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* s_TreasureSpawnSingleItemSimple(
	GAME* pGame,
	int treasureclass,
	int nTreasureLevelBoost)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETNULL(pGame != NULL);

	const TREASURE_DATA* pTreasureData = TreasureGetData(pGame, treasureclass);
	ASSERT_RETNULL(pTreasureData != NULL);
	ASSERT_RETNULL(!s_TreasureClassHasNoTreasure(pTreasureData));

	ITEM_SPEC specItem;
	UNIT* ppItem[MAX_TRACKED_LOOT_PER_DROP];
	ITEMSPAWN_RESULT trackedLoot;
		
	// set options in the spawn spec based on treasure data
	sSpecSetOptionsFromTreasureData(&specItem, pTreasureData);

	SETBIT(specItem.dwFlags, ISF_IDENTIFY_BIT);

	trackedLoot.pTreasureBuffer = ppItem;
	trackedLoot.nTreasureBufferSize = MAX_TRACKED_LOOT_PER_DROP;
	
	s_sTreasureSpawn(pGame, NULL, pTreasureData, INVALID_ID, pTreasureData->tModList, &specItem, &trackedLoot, 0, -1, nTreasureLevelBoost);
	if (trackedLoot.nTreasureBufferCount == 0) {
		return NULL;
	}

	DWORD dwRnd = RandGetNum(pGame, trackedLoot.nTreasureBufferCount);
	for (DWORD i = 0; i < (DWORD)trackedLoot.nTreasureBufferCount; i++) {
		if (i != dwRnd) {
			UnitFree(ppItem[i]);
		}
	}
	return ppItem[dwRnd];
#else
	return NULL;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTreasureClassGetToSpawn(
	UNIT *pUnit,
	int nTreasureClassOverride)
{
	
	int nTreasureClass = nTreasureClassOverride;
	if (nTreasureClass == INVALID_LINK)
	{
		if (pUnit)
		{
			nTreasureClass = UnitGetStat( pUnit, STATS_TREASURE_CLASS );
		}
	}

	return nTreasureClass;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTreasureClassGetCraftingToSpawn( UNIT *pUnit )									
{
	ASSERT_RETINVALID( pUnit );
	return pUnit->pUnitData->nTreasureCrafting;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TreasureSpawn(
	UNIT * unit,
	int nTreasureClassOverride,
	ITEM_SPEC * spec,
	ITEMSPAWN_RESULT * spawnResult)
{
#ifdef _DEBUG
	const GLOBAL_DEFINITION* global = DefinitionGetGlobal();
	if (global->dwFlags & GLOBAL_FLAG_NOLOOT)
	{
		return;
	}
#endif

	ASSERT_RETURN(unit);
	GAME* game = UnitGetGame(unit);
	ASSERT_RETURN(game && IS_SERVER(game));
		
	// get treasure class
	int nTreasureClass = sTreasureClassGetToSpawn( unit, nTreasureClassOverride );	
	if (nTreasureClass != INVALID_LINK)
	{
		s_TreasureSpawnClass( unit, nTreasureClass, spec, spawnResult );	
	}

	// get crafting treasure class
	int nCraftingTreasureClass = sTreasureClassGetCraftingToSpawn( unit );	
	if (nCraftingTreasureClass != INVALID_LINK)
	{
		s_TreasureSpawnClass( unit, nCraftingTreasureClass, spec, NULL );	
	}

	int nMonsterQuality = MonsterGetQuality( unit );
	if (nMonsterQuality != INVALID_LINK)
	{
		const MONSTER_QUALITY_DATA *pMonsterQuality = MonsterQualityGetData( NULL, nMonsterQuality );
		if (pMonsterQuality->nTreasureClass != INVALID_LINK)
		{
			s_TreasureSpawnClass(unit, pMonsterQuality->nTreasureClass, spec, spawnResult);
		}
	}



	
}
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
struct SPAWN_TREASURE_DATA
{
	UNIT *pUnitWithTreasure;
	UNIT *pOriginalOperator;
	int nTreasureClassOverride;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnTreasureForPlayer(
	UNIT *pPlayer,
	void *pCallbackData)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Unit is not a player" );
	GAME *pGame = UnitGetGame( pPlayer );
		
	// get callback data
	const SPAWN_TREASURE_DATA *pSpawnTreasureData = (const SPAWN_TREASURE_DATA *)pCallbackData;
	UNIT *pUnitWithTreasure = pSpawnTreasureData->pUnitWithTreasure;
	ASSERTX_RETURN( pUnitWithTreasure, "Expected unit with treasure" );
	UNIT *pOperator = pSpawnTreasureData->pOriginalOperator;

	// check for treasure classes that always spawn for a player in the level
	BOOL bAllPlayersGetDrop = FALSE;
	int nTreasureClass = sTreasureClassGetToSpawn( pUnitWithTreasure, pSpawnTreasureData->nTreasureClassOverride );
	if (nTreasureClass != INVALID_LINK)
	{
		const TREASURE_DATA *pTreasureData = TreasureGetData( pGame, nTreasureClass );
		if (pTreasureData && TreasureDataTestFlag( pTreasureData, TDF_CREATE_FOR_ALL_PLAYERS_IN_LEVEL ))
		{
			bAllPlayersGetDrop = TRUE;
		}
	}

	// if not able to get drop, see if proximity will allow it
	if (bAllPlayersGetDrop == FALSE)
	{
		
		// player must either be the operator or be close enough to get booty from this drop
		if (pOperator != pPlayer)
		{
			if( AppIsTugboat() )
			{
				PARTYID idParty = UnitGetPartyId( pPlayer );
				if( idParty == INVALID_ID ||
					( pOperator && idParty != UnitGetPartyId( pOperator ) ) )
				{
					return;
				}
			}

			LEVEL *pLevel = UnitGetLevel( pUnitWithTreasure );
			float flMaxRadius = LevelGetMaxTreasureDropRadius( pLevel );
			float flMaxRadiusSq = flMaxRadius * flMaxRadius;
			float flDistSq = UnitsGetDistanceSquared( pPlayer, pUnitWithTreasure );
			if (flDistSq > flMaxRadiusSq)
			{
				// ok, so they're too far form the unit with the treasure, but if they are 
				// kinda close to the operator too then we will also instance the loot ...
				// this will allow players with ranged attacks to hang back together and
				// both share in the loot
				if (pOperator == NULL)
				{
					return;  // no operator to check, oh well, yer just too far for booty!
				}
				else
				{
					flDistSq = UnitsGetDistanceSquared( pPlayer, pOperator );
					float flMaxRadiusSqToOperator = flMaxRadiusSq / 2.0f;  // has to be a bit closer to operator
					if (flDistSq > flMaxRadiusSqToOperator)
					{
						return;  // nope, too far away to receive any booty for this drop
					}
				}
			}
		}
	
	}
			
	// setup item spec
	ITEM_SPEC tSpec;
	SETBIT( tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT );
	SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
	tSpec.guidRestrictedTo = UnitGetGuid( pPlayer );
	tSpec.unitOperator = pPlayer;  // the operator for *this* instance drop is *this* player

	// add some luck of the attacker to the drop for everybody ... 
	// yeah, luck comes from the original operator so it trickles down to your party member too.
	if (pOperator != NULL)
	{
		// phu note - instead of adding luck to item spawn, 
		// luck should be a separate roll that occurs, on a special jackpot table			
		ItemSpawnSpecAddLuck( tSpec, pOperator ); // puts the luck into the item spawn
	}

	// do spawn
	s_TreasureSpawn( pUnitWithTreasure, pSpawnTreasureData->nTreasureClassOverride, &tSpec, NULL );
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TreasureSpawnInstanced(
	UNIT *pUnitWithTreasure,
	UNIT *pOperator,
	int nTreasureClassOverride)
{
	ASSERTX_RETURN( pUnitWithTreasure, "Expected unit with treasure" );
	
	// setup callback data
	SPAWN_TREASURE_DATA tCallbackData;
	tCallbackData.pUnitWithTreasure = pUnitWithTreasure;
	tCallbackData.pOriginalOperator = pOperator;
	tCallbackData.nTreasureClassOverride = nTreasureClassOverride;
	
	// iterate players in level
	LEVEL *pLevel = UnitGetLevel( pUnitWithTreasure );
	LevelIteratePlayers( pLevel, sSpawnTreasureForPlayer, &tCallbackData );

}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
TREASURE_RESULT s_DebugTreasureSpawnUnitType(
	UNIT *pSpawner,
	int nUnitType,
	ITEM_SPEC *pSpec,
	ITEMSPAWN_RESULT *pSpawnResult)
{
	ASSERT_RETX(pSpawner, RESULT_INVALID);
	GAME *pGame = UnitGetGame(pSpawner);
	ASSERT_RETX(pGame && IS_SERVER(pGame), RESULT_INVALID);

	// if not item spawn, use default one
	ITEM_SPEC tSpecDefault;
	if (pSpec == NULL)
	{	
		pSpec = &tSpecDefault;
		SETBIT(pSpec->dwFlags, ISF_PLACE_IN_WORLD_BIT);
	}
	
	// when in world, force not equip
	if (TESTBIT(pSpec->dwFlags, ISF_PLACE_IN_WORLD_BIT))
	{
		CLEARBIT(pSpec->dwFlags, ISF_PICKUP_BIT);
	}
	
	TREASURE_PICK tPick;	// dummy here with one item
	tPick.eCategory = TC_CATEGORY_UNITTYPE;
	tPick.eOperator[0] = PICK_OP_NOOP;
	tPick.nIndex[0] = nUnitType;
	tPick.nValue = 1;

	TREASURE_MOD_LIST tModList;		// empty to start

	int spawnlevel = s_sTreasureGetSpawnLevel(pGame, pSpawner, pSpec);
	spawnlevel = MAX(1, spawnlevel);

	// do spawn
	return s_sTreasureSpawnByUnitType(
		pGame, 
		pSpawner, 
		NULL, 
		spawnlevel, 
		&tPick, 
		tModList, 
		pSpec, 
		pSpawnResult); 
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int TreasureGetFirstItemClass(
	GAME *pGame,
	int nTreasureClass )
{
	if (nTreasureClass == INVALID_LINK)
	{
		return INVALID_LINK;
	}

	// get treasure definition
	const TREASURE_DATA* pTreasureData = TreasureGetData(pGame, nTreasureClass);
	ASSERT_RETINVALID(pTreasureData);

	const TREASURE_PICK * pick_data = pTreasureData->pPicks;
	return ( pick_data->eCategory == TC_CATEGORY_ITEMCLASS ) ? pick_data->nIndex[0] : INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TREASURE_PICK::TREASURE_PICK() 
{
	for (int i=0; i<MAX_CATEGORY_INDICIES; i++) 
	{
		nIndex[i] = INVALID_LINK; 
		eOperator[i] = PICK_OP_NOOP;
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_TreasureGambleRecreateItem(
	UNIT *&pItem,
	UNIT *pMerchant,
	UNIT *pPlayer)
{
	ASSERT_RETFALSE( pItem );
	ASSERT_RETFALSE( pMerchant );
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitGetGenus( pPlayer ) == GENUS_PLAYER );

#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME *pGame = UnitGetGame(pPlayer);
	const TREASURE_DATA* pTreasureData = NULL;
	
	const UNIT_DATA *pMerchantData = UnitGetData(pMerchant);
	ASSERT_RETFALSE(pMerchantData);
	if (pMerchantData->nStartingTreasure != INVALID_LINK)
	{
		pTreasureData = TreasureGetData(pGame, pMerchantData->nStartingTreasure);
		ASSERT_RETFALSE(pTreasureData);
	}

	ITEM_SPEC tItemSpec;
	sSpecSetOptionsFromTreasureData(&tItemSpec, pTreasureData);
	SETBIT( tItemSpec.dwFlags, ISF_IDENTIFY_BIT );
	tItemSpec.pMerchant = pMerchant;

	// get the treasure level boosts
	int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
	int nTreasureLevelBoost = s_sTreasureGetLevelBoost(pGame, pPlayer, pTreasureData);
	int spawnlevel = s_sTreasureGetSpawnLevel(pGame, pPlayer, &tItemSpec, nDifficulty);
	spawnlevel += nTreasureLevelBoost;
	spawnlevel = MAX(1, spawnlevel);

	UNIT* arItems[ 1 ] = {NULL};
	ITEMSPAWN_RESULT tResultItems;
	tResultItems.pTreasureBuffer = arItems;
	tResultItems.nTreasureBufferSize = arrsize(arItems);
	TREASURE_RESULT eResult = s_sTreasureSpawnByItemClass( pGame, pPlayer, UnitGetClass(pItem), spawnlevel, pTreasureData, pTreasureData->tModList, &tItemSpec, &tResultItems );

	if (eResult == RESULT_ITEM_SPAWNED)
	{
		ASSERT_RETFALSE(arItems[0]);

		// release the dummy item
		UnitFree(pItem, UFF_SEND_TO_CLIENTS);

		// return the new item
		pItem = arItems[0];

		// put it into the personal store inventory location
		// this is a copy of the code that is done when an item is normally spawned in a merchant inventory.
		// it is replicated here so we can get back to the normal code path of buying an item with fewer changes.
		int nInvLocWarehouse = GlobalIndexGet( GI_INVENTORY_LOCATION_MERCHANT_WAREHOUSE );		
		ASSERTX_RETVAL( nInvLocWarehouse != INVALID_LINK, ITEM_SPAWN_DESTROY, "Expected inventory location for merchant warehouse" );

		if (InventoryMoveToAnywhereInLocation( pPlayer, pItem, nInvLocWarehouse, CLF_ALLOW_NEW_CONTAINER_BIT ))
		{
			// item is ready to send messages
			UnitSetCanSendMessages( pItem, TRUE );

			// send unit to client
			GAMECLIENT *pClient = UnitGetClient( pPlayer );
			s_SendUnitToClient( pItem, pClient );

			return TRUE;
		}

	}

	if (arItems[0])
	{
		if (pItem == arItems[0])
			pItem = NULL;

		// clean up the created item
		UnitFree(arItems[0]);
	}

#endif

	return FALSE;
}
