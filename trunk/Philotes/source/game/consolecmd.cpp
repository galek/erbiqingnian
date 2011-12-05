//----------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "gamelist.h"
#include "consolecmd.h"
#include "globalindex.h"
#include "console.h"
#include "console_priv.h"
#include "dungeon.h"
#include "s_gdi.h"
#include "clients.h"
#include "c_message.h"
#include "s_message.h"
#include "excel.h"
#include "c_font.h"
#include "windowsmessages.h"
// includes for commands
#include "c_input.h"
#include "c_tasks.h"
#include "c_ui.h"
#include "units.h"
#include "player.h"
#include "unit_priv.h"
#include "drlg.h"
#include "skills.h"
#include "wardrobe.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_sound_priv.h"
#include "c_sound_memory.h"
#include "c_sound_adsr.h"
#include "c_music.h"
#include "c_music_priv.h"
#include "c_backgroundsounds.h"
#include "items.h"
#include "monsters.h"
#include "proc.h"
#include "combat.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "uix.h"
#include "uix_debug.h"
#include "uix_components.h"
#include "uix_menus.h"
#include "uix_chat.h"
#include "ai.h"
#include "ai_priv.h"
#include "c_appearance.h"
#include "inventory.h"
#include "uix_scheduler.h"
#include "objects.h"
#include "unitmodes.h"
#include "picker.h"
#ifdef HAVOK_ENABLED
	#include "c_ragdoll.h"
#endif
#include "stringtable.h"
#include "states.h"
#include "room.h"
#include "room_layout.h"
#include "room_path.h"
#include "movement.h"
#include "astar.h"
#include "c_particles.h"
#include "spawnclass.h"
#include "filepaths.h"
#include "quests.h"
#include "script.h"
#include "perfhier.h"
#include "appcommontimer.h"
#include "CrashHandler.h"
#include "e_settings.h"
#include "e_debug.h"
#include "e_main.h"
#include "e_environment.h"
#include "e_irradiance.h"
#include "e_light.h"
#include "e_definition.h"
#include "definition_priv.h"
#include "e_screenshot.h"
#include "e_caps.h"
#include "e_settings.h"
#include "e_material.h"
#include "e_shadow.h"
#include "e_profile.h"
#include "e_effect_priv.h"
#include "e_dpvs.h"
#include "debugbars_priv.h"
#include "wardrobe.h"
#include "wardrobe_priv.h"
#include "unittag.h"
#include "treasure.h"
#include "performance.h"
#include "debug.h"
#include "s_network.h"
#include "unitdebug.h"
#include "waypoint.h"
#include "chat.h"
#include "teams.h"
#include "colorset.h"
#include "gameunits.h"
#include "s_townportal.h"
#include "warp.h"
#include "e_havokfx.h"
#include "gameexplorer.h"
#include "pets.h"
#include "pakfile.h"
#include "s_trade.h"
#include "c_trade.h"
#include "c_message.h"
#include "e_budget.h"
#include "fillpak.h"
#include "s_quests.h"
#include "s_questgames.h"
#include "s_store.h"
#include "offer.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "dbunit.h"
#include "svrstd.h"
#include "GameServer.h"
#include "npc.h"
#include "dialog.h"
#include "c_itemupgrade.h"
#include "hotkeys.h"
#include "global_themes.h"
#include "skill_strings.h"
#include "version.h"
#include "s_QuestServer.h"
#include "Quest.h"
#include "gameconfig.h"
#include "Currency.h"
#include "quest_tutorial.h"
#include "language.h"
#include "QuestObjectSpawn.h"
#include "achievements.h"
#include "duel.h"
#include "gamevariant.h"
#include "gameconfig.h"
#include "Gag.h"
#include "ptime.h"
#include "s_globalGameEventNetwork.h"
#include "versioning.h"

#if !ISVERSION(SERVER_VERSION)
#include "c_particles.h"
#include "c_quests.h"
#include "c_chatNetwork.h"
#include "c_chatBuddy.h"
#include "c_chatIgnore.h"
#include "UserChatCommunication.h"
#include "c_channelmap.h"
#include "e_features.h"
#include "c_camera.h"
#include "c_voicechat.h"
#include "e_compatdb.h"		// CHB 2006.12.13
#include "e_optionstate.h"	// CHB 2007.03.02
#include "uix_components_hellgate.h"	// CHB 2006.03.04
#include "drlg.h"

#include "ServerSuite\AuctionServer\AuctionServerDefines.h"
#endif

#if ISVERSION(SERVER_VERSION)
#include "ServerSuite/BillingProxy/BillingMsgs.h"
#include "s_battleNetwork.h"
#include "consolecmd.cpp.tmh"
#endif // ISVERSION(SERVER_VERSION)
#include "ServerSuite\BillingProxy\c_BillingClient.h"

#if !ISVERSION(CLIENT_ONLY_VERSION)
#include "namefilter.h"
#include "UserChatCommunication.h"
#include "GameChatCommunication.h"
#endif

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define PARAM_DELIM						L" ,"
#define CONSOLE_COMMAND_BUFSIZE			32			// grow command table array by this amount
#define MAX_CONSOLE_COMMAND_LEN			64			// max length of command string
#define MAX_CONSOLE_PARAM_VALUE_LEN		256			// max length of param value
#define	MAX_CONSOLE_PARAM_NAME_LEN		32			// max length of param name
#define MAX_CONSOLE_USAGE_LEN			256			// max length of usage string
#define MAX_CONSOLE_DESC_LEN			256			// max length of description string
#define CONSOLE_DEFLIST_BUFSIZE			32			// grow definition lists by this amount

#if ISVERSION(RELEASE_CHEATS_ENABLED)
#define NOT_CHEATS						0x00		// non-cheats, generally enabled EXCEPT SERVER SIDE RELEASE
#define CHEATS_NORELEASE				0x01		// Cheats that aren't intended to be included in the final build
#define CHEATS_RELEASE_TESTER			0x02		// Cheats for QA testers in the final build
#define CHEATS_RELEASE_CSR				0x04		// Cheats for Custom Service Reps, in the final build
#else
#define NOT_CHEATS						FALSE
#define CHEATS_NORELEASE				TRUE
#define CHEATS_RELEASE_TESTER			TRUE
#define CHEATS_RELEASE_CSR				TRUE
#endif


/*
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL ConsoleVerifyParams(
	int command,
	const WCHAR * input)
{
	CONSOLE_COMMAND* pCommandTbl = ConsoleGetCommandTable();
	int wanted = pCommandTbl[command].m_nNumParams;
	int existing = ConsoleGetCommandParams(command, input);

	if (wanted == existing)
	{
		return TRUE;
	}
	if (wanted == -2 )
	{
		return TRUE;
	}
	if (wanted == -1 && existing >= 0 && pCommandTbl[command].m_nParamTable == DATATABLE_NONE)
	{
		return TRUE;
	}
	if (wanted == -1 && existing >= 1)
	{
		return TRUE;
	}
	return FALSE;
}
*/


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	COMMAND_CLIENT,
	COMMAND_SERVER,
	COMMAND_BOTH,
};

enum ECONSOLE_PARAM
{
	PARAM_INTEGER,
	PARAM_FLOAT,
	PARAM_BOOLEAN,
	PARAM_HEX,
	PARAM_DATATABLE,
	PARAM_DATATABLE_BYNAME,
	PARAM_DATATABLE_BYSTR,
	PARAM_TABLENAME,
	PARAM_PERF,
	PARAM_HITCOUNT,
	PARAM_GENERIC,
	PARAM_STRINGLIST,
	PARAM_PLAYERNAME,
	PARAM_STRING,
	PARAM_DEFINITION,
	PARAM_TEAM,
	PARAM_CONSOLECMD,
	PARAM_UI_COMPONENT,
};


//----------------------------------------------------------------------------
// TYPEDEF
//----------------------------------------------------------------------------
typedef DWORD (*CONSOLE_COMMAND_FUNC)(GAME * game, GAMECLIENT * client, const struct CONSOLE_COMMAND * cmd, const WCHAR * strParam, UI_LINE * pLine);
typedef BOOL (*CONSOLE_PARAM_AUTOCOMPLETE)(GAME * game, const WCHAR * strLine, const struct CONSOLE_COMMAND * cmd, const WCHAR * strInput, BOOL bReverse);


//----------------------------------------------------------------------------
enum CONSOLE_COMMAND_FLAGS
{
	CCF_DEAD_RESTRICTED = MAKE_MASK( 0 ),		// Players can NOT use this command when dead
};

//----------------------------------------------------------------------------
struct CONSOLE_PARAM
{
	ECONSOLE_PARAM				m_eParamType;
	PStrW						m_strName;
	PStrW 						m_strDefault;
	PStrW *						m_pstrValues;
	unsigned int				m_nValues;
	DWORD_PTR					m_data1;
	DWORD_PTR					m_data2;
	BOOL						m_isRequired;
	BOOL						m_isOrdered;
	CONSOLE_PARAM_AUTOCOMPLETE	m_fCustomAutocomplete;
};

//----------------------------------------------------------------------------
struct CONSOLE_COMMAND
{
	PStrW						m_strCommand;				// this command
	PStrW						m_strCommandLocalizedLink;	// linked command (used for localized console commands to link a pair of commands together)
	const PStrW *				m_strCategory;
	PStrW						m_strDescription;
	PStrW						m_strUsage;
	PStrW						m_strUsageNotes;
	CONSOLE_PARAM *				m_pParams;
	unsigned int				m_nParams;
	BOOL						m_bValidateParams;
	int							m_isCheat;
	int							m_eAppType;
	int							m_eGameType;
	DWORD						m_dwCommandFlags;			// see CONSOLE_COMMAND_FLAGS
	CONSOLE_COMMAND_FUNC		m_fCmdFunction;
	unsigned short int			m_nData;
};


struct CONSOLE_COMMAND_TABLE
{
	CONSOLE_COMMAND *			m_pCommandTable;
	unsigned int				m_nCommandCount;
	unsigned int				m_nLastRegisteredCommand;
	PStrW **					m_pstrCategories;		
	unsigned int				m_nCategories;
	const PStrW *				m_pLastCategory;				

	// specific data
	PStrA *						m_pstrParticles;
	unsigned int				m_nParticles;
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
CONSOLE_COMMAND_TABLE g_ConsoleCommandTable;


//----------------------------------------------------------------------------
// UTILITY FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const PStrW * sCommandTableFindCategory(
	const WCHAR * strCat)
{
	if (!g_ConsoleCommandTable.m_pstrCategories)
	{
		return NULL;
	}

	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nCategories; ++ii)
	{
		ASSERT_CONTINUE(g_ConsoleCommandTable.m_pstrCategories[ii]);
		
		if (PStrICmp(g_ConsoleCommandTable.m_pstrCategories[ii]->GetStr(), strCat) == 0)
		{
			return g_ConsoleCommandTable.m_pstrCategories[ii];
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const CONSOLE_COMMAND * sCommandTableFindCommandExact(
	const WCHAR * strCmd)
{
	ASSERT_RETNULL(g_ConsoleCommandTable.m_pCommandTable);

	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nCommandCount; ++ii)
	{
		const CONSOLE_COMMAND * cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
		if (PStrICmp(cmd->m_strCommand.GetStr(), strCmd) == 0)
		{
			return cmd;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const CONSOLE_PARAM * sConsoleParseGetParam(
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * strParams)
{
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(idxParam < cmd->m_nParams);
	ASSERT_RETFALSE(cmd->m_pParams);

	if (strParams == NULL)
	{
		return FALSE;
	}

	return cmd->m_pParams + idxParam;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR * sConsoleSkipDelimiter(
	const WCHAR * str,
	const WCHAR * delim = NULL)
{
	static const WCHAR * defaultDelim = L" ,";

	ASSERT_RETNULL(str);
	if (str[0] == 0)
	{
		return str;
	}
	if (!delim)
	{
		delim = defaultDelim;
	}
	while (*delim)
	{
		if (str[0] == *delim)
		{
			++str;
			break;
		}
		delim++;
	}
	return PStrSkipWhitespace(str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseIntegerParam(
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	int & val,
	const WCHAR * strDelim = NULL,
	BOOL bAllowHex = FALSE)
{
	val = 0;

	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(param->m_eParamType == PARAM_INTEGER);

	if (bAllowHex)
	{
		strParams = PStrSkipWhitespace(strParams);
		if (strParams[0] == 'x')
		{
			++strParams;
			return PStrHexToInt(strParams, INT_MAX, val, &strParams);
		}
	}
	BOOL result = PStrToIntEx(strParams, val, &strParams);
	strParams = sConsoleSkipDelimiter(strParams, strDelim);
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseDWORDParam(
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	DWORD & val,
	const WCHAR * strDelim = NULL,
	BOOL bAllowHex = FALSE)
{
	val = 0;

	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(param->m_eParamType == PARAM_INTEGER);

	if (bAllowHex)
	{
		strParams = PStrSkipWhitespace(strParams);
		if (strParams[0] == 'x')
		{
			++strParams;
			return PStrHexToInt(strParams, UINT_MAX, val, &strParams);
		}
	}
	BOOL result = PStrToIntEx(strParams, val, &strParams);
	strParams = sConsoleSkipDelimiter(strParams, strDelim);
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseFloatParam(
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	float & val)
{
	val = 0;

	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(param->m_eParamType == PARAM_FLOAT);

	BOOL result = PStrToFloat(strParams, val, &strParams);
	strParams = sConsoleSkipDelimiter(strParams);
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseBooleanParam(
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	BOOL & val)
{
	val = FALSE;

	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(param->m_eParamType == PARAM_BOOLEAN);

	int bufLen = CONSOLE_MAX_STR;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if ((strParams = PStrTok(strToken, strParams, PARAM_DELIM, arrsize(strToken), bufLen)) == NULL)
	{
		return FALSE;
	}
	strParams = PStrSkipWhitespace(strParams);

	if (PStrICmp(strToken[0], L'f') || strToken[0] == L'0' || PStrICmp(strToken, L"off") == 0)
	{
		return TRUE;
	}
	if (PStrICmp(strToken[0], L't') || strToken[0] == L'1' || PStrICmp(strToken, L"on") == 0)
	{
		val = TRUE;
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseToken(
	const CONSOLE_COMMAND * cmd,
	const WCHAR * & strParams,
	WCHAR * strToken,
	unsigned int lenToken,
	const WCHAR * strDelimiter = NULL)
{
	ASSERT_RETFALSE(strToken);
	ASSERT_RETFALSE(lenToken > 0);
	strToken[0] = 0;

	if (strDelimiter == NULL)
	{
		strDelimiter = PARAM_DELIM;
	}
	int lenBuf = CONSOLE_MAX_STR;
	if ((strParams = PStrTok(strToken, strParams, strDelimiter, lenToken, lenBuf)) == NULL)
	{
		return FALSE;
	}
	if (!strToken[0])
	{
		return FALSE;
	}
	strParams = PStrSkipWhitespace(strParams);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParsePlayerNameParam(
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	WCHAR * strName,
	unsigned int lenName)
{
	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(param->m_eParamType == PARAM_PLAYERNAME);

	return sConsoleParseToken(cmd, strParams, strName, lenName);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseDatatableName(
	GAME * game,
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	WCHAR * strLookup,
	unsigned int lenLookup,
	EXCELTABLE & table)
{
	ASSERT_RETFALSE(strLookup);
	strLookup[0] = 0;
	table = DATATABLE_NONE;
	ASSERT_RETFALSE(game);

	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(param->m_eParamType == PARAM_TABLENAME);

	const WCHAR * str = strParams;
	if (!sConsoleParseToken(cmd, str, strLookup, lenLookup))
	{
		return FALSE;
	}
	
	char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chToken, strLookup, MIN(arrsize(chToken), lenLookup));

	for (unsigned int ii = 0; ii < NUM_DATATABLES; ++ii)
	{
		const char * chTableName = ExcelGetTableName((EXCELTABLE)ii);
		ASSERT_BREAK(chTableName);
		if (!chTableName[0])
		{
			continue;
		}

		if (PStrICmp(chToken, chTableName, MAX_CONSOLE_PARAM_VALUE_LEN) == 0)
		{
			table = (EXCELTABLE)ii;
			strParams = str;
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <typename T>
static BOOL sIsValidParamEnd(
	const T wc)
{
	return (wc == 0 || iswspace(wc) || wc == (T)(','));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleExcelLookup(
	GAME * game,
	EXCELTABLE table,
	const WCHAR * strToken,
	unsigned int lenToken,
	int & index)
{
	char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chToken, strToken, MIN(arrsize(chToken), lenToken));

	int ii = ExcelGetLineByStringIndex(EXCEL_CONTEXT(game), table, chToken);
	if (ii == INVALID_LINK)
	{
		return FALSE;
	}

	index = ii;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseExcelLookup(
	GAME * game,
	const CONSOLE_COMMAND * cmd,
	EXCELTABLE table,
	const WCHAR * & strParams,
	WCHAR * strLookup,
	unsigned int lenLookup,
	int & index)
{
	ASSERT_RETFALSE(strLookup);
	strLookup[0] = 0;
	index = INVALID_LINK;
	ASSERT_RETFALSE(game);

	int lenParams = PStrLen(strParams);
	char chParams[CONSOLE_MAX_STR];
	PStrCvt(chParams, strParams, arrsize(chParams));

	const char * chMatchKey = NULL;
	int lenMaxMatch = 0;

	unsigned int rowCount = ExcelGetNumRows(game, table);
	for (unsigned int ii = 0; ii < rowCount; ++ii)
	{
		const char * chKey = ExcelGetStringIndexByLine(game, table, ii);
		if (!chKey)
		{
			continue;
		}
		int lenName = PStrLen(chKey);
		if (lenParams < lenName)
		{
			continue;
		}
		if (lenName < lenMaxMatch)
		{
			continue;
		}

		if (PStrICmp(chParams, chKey, lenName) == 0 && sIsValidParamEnd(strParams[lenName]))
		{
			index = ii;
			lenMaxMatch = lenName;
			chMatchKey = chKey;
		}
	}

	if (index == INVALID_LINK)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(chMatchKey);

	PStrCvt(strLookup, chMatchKey, lenLookup);
	strParams = sConsoleSkipDelimiter(strParams + lenMaxMatch);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseExcelLookup(
	GAME * game,
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	WCHAR * strLookup,
	unsigned int lenLookup,
	int & index)
{
	ASSERT_RETFALSE(strLookup);
	strLookup[0] = 0;
	index = INVALID_LINK;
	ASSERT_RETFALSE(game);

	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(param->m_eParamType == PARAM_DATATABLE);

	EXCELTABLE table = (EXCELTABLE)param->m_data1;
	return sConsoleParseExcelLookup(game, cmd, table, strParams, strLookup, lenLookup, index);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseExcelLookupOrName(
	GAME * game,
	const CONSOLE_COMMAND * cmd,
	EXCELTABLE table,
	const WCHAR * & strParams,
	WCHAR * strToken,
	unsigned int lenToken,
	int & index)
{
	strToken[0] = 0;
	index = INVALID_LINK;

	const WCHAR * str = strParams;
	if (!sConsoleParseToken(cmd, str, strToken, lenToken))
	{
		return FALSE;
	}

	int lenParams = PStrLen(strParams);

	int nIndexFound = INVALID_LINK;
	WCHAR uszTokenFound[CONSOLE_MAX_STR] = { 0 };
	int lenTokenFound = CONSOLE_MAX_STR;
	
	unsigned int rowCount = ExcelGetNumRows(game, table);
	for (unsigned int ii = 0; ii < rowCount; ++ii)
	{	
		WCHAR strKey[CONSOLE_MAX_STR];
		PStrCvt(strKey, ExcelGetStringIndexByLine(game, table, ii), arrsize(strKey));
		int lenName = PStrLen(strKey);
		if (lenParams >= lenName)
		{
			if (PStrICmp(strParams, strKey, lenName) == 0 && sIsValidParamEnd(strParams[lenName]))
			{
				if (nIndexFound == INVALID_LINK || lenName > lenTokenFound)
				{
					nIndexFound = ii;
					PStrCopy(uszTokenFound, strKey, CONSOLE_MAX_STR);				
					lenTokenFound = lenName;
				}
			}
		}

		const WCHAR * strName = ExcelGetRowNameString(game, table, ii);
		if (!strName)
		{
			continue;
		}
		lenName = PStrLen(strName);
		if (lenParams >= lenName)
		{
			if (PStrICmp(strParams, strName, lenName) == 0 && sIsValidParamEnd(strParams[lenName]))
			{
				if (nIndexFound == INVALID_LINK || lenName > lenTokenFound)
				{	
					nIndexFound = ii;
					PStrCopy(uszTokenFound, strName, CONSOLE_MAX_STR);
					lenTokenFound = lenName;
				}
			}
		}
	}
	
	if (nIndexFound != INVALID_LINK)
	{
		PStrCopy(strToken, uszTokenFound, lenToken);
		index = nIndexFound;
		strParams = sConsoleSkipDelimiter(strParams + lenTokenFound);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConsoleParseExcelLookupOrName(
	GAME * game,
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	WCHAR * strToken,
	unsigned int lenToken,
	int & index)
{
	strToken[0] = 0;
	index = INVALID_LINK;

	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	EXCELTABLE table = (EXCELTABLE)param->m_data1;

	return sConsoleParseExcelLookupOrName(game, cmd, table, strParams, strToken, lenToken, index);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * sConsoleParseUnitId(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams)
{
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(idxParam < cmd->m_nParams);
	ASSERT_RETFALSE(cmd->m_pParams);

	UNIT * unit = NULL;
	int idUnit;
	if (!sConsoleParseIntegerParam(cmd, idxParam, strParams, idUnit))
	{
		unit = GameGetDebugUnit(game);
		if (!unit)
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unspecified unit for command %s.", cmd->m_strCommand.GetStr());
			return NULL;
		}
	}
	else
	{
		unit = UnitGetById(game, idUnit);
		if (!unit)
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unknown unit [%d].", idUnit);
			return NULL;
		}
	}
	return unit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
static BOOL sConsoleParseParamParticleDefinition(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	WCHAR * strToken,
	unsigned int lenToken)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(strParams);
	ASSERT_RETFALSE(strToken);
	strToken[0] = 0;

	const CONSOLE_PARAM * param = sConsoleParseGetParam(cmd, idxParam, strParams);
	if (!param)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(param->m_eParamType == PARAM_DEFINITION);

	char chParam[CONSOLE_MAX_STR];
	PStrCvt(chParam, strParams, arrsize(chParam));

	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nParticles; ++ii)
	{
		unsigned int len = g_ConsoleCommandTable.m_pstrParticles[ii].Len();
		if (PStrICmp(chParam, g_ConsoleCommandTable.m_pstrParticles[ii].GetStr(), len) != 0 ||
			!sIsValidParamEnd(chParam[len]))
		{
			continue;
		}
		ASSERT_RETFALSE(len < lenToken);
		PStrCvt(strToken, g_ConsoleCommandTable.m_pstrParticles[ii].GetStr(), lenToken);
		return TRUE;
	}
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGetSpawnVectors(
	UNIT * unit,
	VECTOR * position,
	VECTOR * direction,
	BOOL bAllowUp = FALSE)
{
	ASSERT_RETFALSE(unit);

	if (position)
	{
		*position = UnitGetPosition(unit);
	}

	if (direction)
	{
		*direction = UnitGetFaceDirection(unit, FALSE);
		(*direction).fZ = bAllowUp ? MAX(0.0f, (*direction).fZ) : 0.0f;
		if (VectorIsZero(*direction))
		{
			*direction = cgvXAxis;
		} 
		else 
		{
			VectorNormalize(*direction, *direction);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGetPositionInFrontOfUnit(
	UNIT * unit,
	float distance,
	VECTOR * position)
{
	ASSERT_RETFALSE(position);

	if (!unit)
	{
		*position = cgvNull;
		return TRUE;
	}

	VECTOR direction;
	if (!sGetSpawnVectors(unit, position, &direction, TRUE))
	{
		return FALSE;
	}

	*position = *position + (direction * distance);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR * sGetOnOffString(
	BOOL val)
{
	return val ? GlobalStringGet( GS_ON ) : GlobalStringGet( GS_OFF );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WCHAR * sGetYesNoString(
	BOOL val)
{
	return val ? GlobalStringGet( GS_YES ) : GlobalStringGet( GS_NO );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static const WCHAR * sGetEnabledDisabledString(
	BOOL val)
{
	return val ? GlobalStringGet( GS_ENABLED ) : GlobalStringGet( GS_DISABLED );		
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static const WCHAR * sGetClientServerString(
	GAME * game)
{
	static const WCHAR * strServer = L"SRV";
	static const WCHAR * strClient = L"CLT";
	return (IS_SERVER(game) ? strServer : strClient);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static void sConsolePrintMissingOrInvalidValue(
	GAME * game,
	GAMECLIENT * client,
	const WCHAR * strName,
	const WCHAR * strValue)
{
	ASSERT_RETURN(game && strName && strValue);

	if (strValue[0] == 0)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, GlobalStringGet( GS_CCMD_MISSING ), strName);
	}
	else
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, GlobalStringGet( GS_CCMD_INVALID ), strName, strValue);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
//static	// CHB 2007.03.06
void c_sUIReload(
	BOOL bReloadDirect)
{
	BOOL bOnCharSelect = UIComponentGetActive(UIComponentGetByEnum(UICOMP_CHARACTER_SELECT));
	BOOL bOnMainMenu = AppGetState() == APP_STATE_MAINMENU;

	UNITID idUnit = g_UI_StartgameParams.m_idCharCreateUnit;
	BOOL bMontserTrackerUserActive = UIComponentIsUserActive(UIComponentGetByEnum(UICOMP_MONSTER_TRACKER));

	CSchedulerFree();
	UIFree();
	c_InputMouseAcquire(AppGetCltGame(), FALSE);
	if ( AppGetCltGame() )
	{
		UnitClearIcons( AppGetCltGame() );
	}
	e_TexturesCleanup( TEXTURE_GROUP_UI );
	CSchedulerInit();
	UIInit(TRUE, FALSE, INVALID_LINK, bReloadDirect);
	c_InputMouseAcquire(AppGetCltGame(), TRUE);
	UIShowLoadingScreen(!bOnCharSelect && !bOnMainMenu);
	ConsoleInitInterface();				// Reset the console (it needs to find the component ID of its textbox again)
	if ( AppGetCltGame() )
	{
		UnitRecreateIcons( AppGetCltGame() );
		UNITID idControlUnit = UnitGetId(UIGetControlUnit());
		UISendMessage(WM_SETCONTROLUNIT, idControlUnit, 0);
		UISendMessage(WM_INVENTORYCHANGE, idControlUnit, -1);	//force redraw of the inventory
	}

	if (bOnCharSelect)
	{
		g_UI_StartgameParams.m_idCharCreateUnit = idUnit;

		UICharacterCreateReset();

		if (bOnCharSelect)
		{
//			UIComponentSetActive(UIComponentGetByEnum(UICOMP_CHARACTER_SELECT), TRUE);
			UIComponentActivate(UICOMP_CHARACTER_SELECT, TRUE);
		}
	}
	else if (bOnMainMenu)
	{
		// this should restart the main menu
		AppSwitchState(APP_STATE_MAINMENU);
	}
	else if (AppGetCltGame())
	{
		UNIT *pControlUnit = UIGetControlUnit();
		UNITID idControlUnit = UnitGetId(pControlUnit);

		// re-activate the main components.
		//  Tugboat may want to do something similar
		if (AppIsHellgate())
		{
			UIComponentActivate(UICOMP_DASHBOARD, TRUE);
			if (bMontserTrackerUserActive)
				UIComponentActivate(UICOMP_MONSTER_TRACKER, TRUE, 0, NULL, FALSE, FALSE, TRUE);
		}

		UISendMessage(WM_SETCONTROLUNIT, idControlUnit, 0);
		UISendMessage(WM_INVENTORYCHANGE, idControlUnit, -1);	//force redraw of the inventory

		// tell any quests that care
		if (pControlUnit)
		{
			c_QuestEventReloadUI(pControlUnit);
		}

	}

	UIHideLoadingScreen(!bOnMainMenu);

}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sGetControlUnit(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETNULL(game);
	if (IS_SERVER(game))
	{
		ASSERT_RETNULL(client);
		return ClientGetControlUnit(client);
	}
	else
	{
		return GameGetControlUnit(game);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sGetDebugOrControlUnit(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETNULL(game);

	UNIT * unit = GameGetDebugUnit(game);
	if (!unit)
	{
		if (client)
		{
			unit = ClientGetControlUnit(client);
		}
		if (!unit)
		{
			unit = GameGetControlUnit(game);
		}
	}
	return unit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CONSOLE COMMAND FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#pragma warning(disable : 4100)		//	unreferenced formal parameters

#define CONSOLE_CMD_FUNC(func)			static DWORD func(GAME * game, GAMECLIENT * client, const CONSOLE_COMMAND * cmd, const WCHAR * strParams, UI_LINE * pLine)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdQuit)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	static BOOL bTriedOnce = FALSE;
	if (!bTriedOnce && AppGetState() == APP_STATE_IN_GAME)
	{
		c_SendRemovePlayer();
		bTriedOnce = TRUE;
	}
	else
	{
		PostQuitMessage(0);
	}

	return CR_OK;
}
#endif 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdVersion)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleVersionString();
	return CR_OK;
}
#endif 

#if ISVERSION(DEVELOPMENT)
//#include "dictionary.h"
//extern STR_DICT tDictBadge[];
CONSOLE_CMD_FUNC(s_cCmdBadgesList)
{
	UNIT* pUnit;
	if(!client)
	{
		pUnit = GameGetControlUnit(game);
	}
	else
		return CR_FAILED;


	for(int ii = 0; ii < MAX_BADGES;ii++)
	{

		if(UnitHasBadge(pUnit, ii))
		{
			//Ok, this is really "slow", but meh
			/*
			int jj = 0;
			for(; jj < arrsize(tDictBadge); jj++)
			{
			if(tDictBadge[jj].value == ii)
			{
			ConsoleString("Badge %s", tDictBadge[jj].str );
			break;
			}
			}
			if(jj == arrsize(tDictBadge)) // else...
			*/
			ConsoleString("Badge %d", ii );
		}
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPlayed)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	MSG_CCMD_PLAYED msg;
	c_SendMessage(CCMD_PLAYED, &msg);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdPlayedServer)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * unit = ClientGetControlUnit(client);
	ASSERT_RETVAL(unit, CR_FAILED);

	DWORD dwPlayedTime = UnitGetPlayedTime( unit );
	if ( dwPlayedTime )
	{
		int nDisplaySeconds = dwPlayedTime % 60;
		int nDisplayMinutes = dwPlayedTime / 60;
		int nDisplayHours = nDisplayMinutes / 60;
		nDisplayMinutes -= nDisplayHours * 60;
		int nDisplayDays = nDisplayHours / 24;
		nDisplayHours -= nDisplayDays * 24;
		int nDisplayWeeks = nDisplayDays / 7;
		nDisplayDays -= nDisplayWeeks * 7;

		//L"Played Time: %d weeks, %d days, %d hours, %d minutes, %d seconds"
		s_SendSysTextFmt(CHAT_TYPE_WHISPER, FALSE, game, client, CONSOLE_SYSTEM_COLOR,
			GlobalStringGet(GS_PLAYED), nDisplayWeeks, 
			nDisplayDays, nDisplayHours, nDisplayMinutes, nDisplaySeconds );
	}
	#endif
	return CR_OK;
}


//----------------------------------------------------------------------------
// todo: this should be per-game
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPause)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bPause;
	if (!sConsoleParseBooleanParam(cmd, 0, strParams, bPause))
	{
		bPause = !AppCommonIsPaused();
	}
	AppPause(bPause);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Simulation Pause: %s", sGetOnOffString(bPause));
	return CR_OK;
}
#endif 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdFPS)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL val;
	if (!sConsoleParseBooleanParam(cmd, 0, strParams, val))
	{
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_FPS, -1);
	}
	else
	{
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_FPS, val);
	}

	return CR_OK;
}
#endif 

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _PROFILE
bool g_ProfileSnapshot = false;
long g_ProfileSnapshotCount = 1;
CONSOLE_CMD_FUNC(c_sCmdProfileSnapshot)
{
	g_ProfileSnapshot = true;

	return CR_OK;
}
#endif 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdSetGamma)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	float val;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, val))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	CComPtr<OptionState> pState;
	V_DO_FAILED(e_OptionState_CloneActive(&pState))
	{
		return CR_FAILED;
	}
	float fGammaPower = PIN( val, MIN_GAMMA_ADJUST, MAX_GAMMA_ADJUST );
	pState->set_fGammaPower(fGammaPower);

	V_DO_FAILED(e_OptionState_CommitToActive(pState))
	{
		return CR_FAILED;
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdSetFOV)
{
	if(AppIsHellgate())
	{
		ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

		int nNewFOV;
		if (!sConsoleParseIntegerParam(cmd, 0, strParams, nNewFOV))
		{
			return (CR_FAILED | CRFLAG_BADPARAMS);
		}

		static const int cMinimumFOV = 1;
		static const int cMaximumFOV = 180;

		if ((nNewFOV < cMinimumFOV) || (nNewFOV > cMaximumFOV))
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid vertical FOV. Allowable vertical FOV range is %d to %d degrees.", cMinimumFOV, cMaximumFOV);
			return CR_FAILED;
		}

		c_CameraSetVerticalFov( nNewFOV, c_CameraGetViewType() );
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdWhisper)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	strParams = PStrSkipWhitespace(strParams);

	if (!strParams[0])
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_SendWhisperChat(strPlayer, strParams, pLine);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdChatInstanced)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	strParams = PStrSkipWhitespace(strParams);
	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_SendInstancingChannelChat(
		L"chat",
		strParams,
		pLine);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
// List instancing chat channel names and which instance they are in if any.
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
#include "s_chatNetwork.h"
#include "c_channelMap.h"

CONSOLE_CMD_FUNC(c_sCmdChatChannelList)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	
		int nChannels = ExcelGetCount(EXCEL_CONTEXT(),
					DATATABLE_CHAT_INSTANCED_CHANNELS);
	for(int i = 0; i < nChannels; i++)
	{
		CHANNEL_DEFINITION *pChannelDef = (CHANNEL_DEFINITION *)
			ExcelGetData(NULL, DATATABLE_CHAT_INSTANCED_CHANNELS, i);
		ASSERT_CONTINUE(pChannelDef);

		WCHAR szChannelName[MAX_INSTANCING_CHANNEL_NAME];
		PStrCvt(szChannelName, pChannelDef->szName, MAX_INSTANCING_CHANNEL_NAME);
		WCHAR szInstancedChannelName[MAX_CHAT_CNL_NAME];
		if(GetInstancedChannel(szChannelName,
			szInstancedChannelName) != INVALID_CHANNELID)
		{
			UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_CHAT_CHANNEL_ON), szChannelName, szInstancedChannelName);
		}
		else
		{
			UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet(GS_CHAT_CHANNEL_OFF), szChannelName);
		}
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
// TODO: lowercase the params
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdChatChannelJoin)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	//Check if they've specified which channel they're joining.  If they
	//specifically specify, tell the chat server that tag.  If not, tell
	//the gameserver to join you to an arbitrary channel.

	BOOL bHyphen = FALSE;
	int i;
	for(i = 0; i < MAX_CHAT_CNL_NAME; i++)
	{
		if(strParams[i] == 0) break;
		if(strParams[i] == '-')
		{
			bHyphen = TRUE;
			break;
		}
	}

	if(bHyphen)
	{	//Tell the chat server to join the specific channel
		CHAT_REQUEST_MSG_JOIN_INSTANCED_CHANNEL msg;
		//Copy the all characters before the hyphen as the instancing channel name.
		PStrCopy(msg.wszInstancingChannelName, MAX_INSTANCING_CHANNEL_NAME,
			strParams, i+1);
		PStrLower(msg.wszInstancingChannelName, MAX_INSTANCING_CHANNEL_NAME);

		msg.TagChannel = strParams;
		
		RemoveInstancingChannel(msg.wszInstancingChannelName);
		c_ChatNetSendMessage(&msg, USER_REQUEST_JOIN_INSTANCED_CHANNEL);
	}
	else
	{	//Tell the game server to put me in an arbitrary channel
		MSG_CCMD_JOIN_INSTANCING_CHANNEL msg;
		STATIC_CHECK(sizeof(msg.szInstancingChannelName)/sizeof(msg.szInstancingChannelName[0])
			== MAX_INSTANCING_CHANNEL_NAME, message_size_mismatch);
		PStrCopy(msg.szInstancingChannelName, strParams, MAX_INSTANCING_CHANNEL_NAME);
		
		PStrLower(msg.szInstancingChannelName, MAX_INSTANCING_CHANNEL_NAME);
		RemoveInstancingChannel(msg.szInstancingChannelName);
		c_SendMessage(CCMD_JOIN_INSTANCING_CHANNEL, &msg);
	}


	return CR_OK;
}
#endif
//----------------------------------------------------------------------------
// TODO: lowercase the params
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdChatChannelLeave)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	//Check if they've specified which channel they're leaving.  If they
	//specifically specify, tell the chat server that tag.  If not, look
	//up the tag.

	CHAT_REQUEST_MSG_LEAVE_INSTANCED_CHANNEL msg;
	
	BOOL bHyphen = FALSE;
	for(int i = 0; i < MAX_CHAT_CNL_NAME; i++)
	{
		if(strParams[i] == 0) break;
		if(strParams[i] == '-')
		{
			msg.TagChannel = strParams;

			//Copy the all characters before the hyphen as the instancing channel name.
			PStrCopy(msg.wszInstancingChannelName, MAX_INSTANCING_CHANNEL_NAME, strParams, i+1);

			bHyphen = TRUE;
			break;
		}
	}
	
	if(!bHyphen)
	{
		WCHAR szChannelName[MAX_CHAT_CNL_NAME];
		CHANNELID idChannel = GetInstancedChannel(strParams, szChannelName);
		
		if (idChannel == INVALID_CHANNELID)
		{
			UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER),
				GlobalStringGet( GS_NOT_IN_CHAT_CHANNEL ), strParams);
			return CR_OK;
		}
		else
		{
			msg.TagChannel = szChannelName;
			PStrCopy(msg.wszInstancingChannelName, strParams, MAX_INSTANCING_CHANNEL_NAME);
		}
	}

	PStrLower(msg.wszInstancingChannelName, MAX_INSTANCING_CHANNEL_NAME);
	RemoveInstancingChannel(msg.wszInstancingChannelName);
	c_ChatNetSendMessage(&msg, USER_REQUEST_LEAVE_INSTANCED_CHANNEL);
	UIAddChatLineArgs(
		CHAT_TYPE_SERVER,
		ChatGetTypeColor(CHAT_TYPE_SERVER),
		GlobalStringGet(GS_CHAT_CHANNEL_LEAVE),
		msg.wszInstancingChannelName );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdTownChat)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	strParams = PStrSkipWhitespace(strParams);
	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_SendTownChannelChat(strParams, pLine);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdReply)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	strParams = PStrSkipWhitespace(strParams);
	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	if(gApp.m_wszLastWhisperSender[0] == 0)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_CHAT_ERROR_REPLY_NO_DEST ) );
	}
	else
	{
		c_SendWhisperChat(gApp.m_wszLastWhisperSender, strParams, pLine);
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdMakeAnnouncement)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	if (AppGetBadgeCollection() &&
		!AppGetBadgeCollection()->HasBadge(ACCT_TITLE_ADMINISTRATOR))
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_CONSOLE_ERROR_NO_PERMISSION ));
		return CR_OK;
	}

	CHAT_REQUEST_MSG_ANNOUNCE_TO_ALL_ON_SERVER announcement;
	announcement.Message.SetWStrMsg(strParams);
	announcement.bAlert = FALSE;
	c_ChatNetSendMessage(
		&announcement,
		USER_REQUEST_ANNOUNCE_TO_ALL_ON_SERVER);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdMakeAlert)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	if (AppGetBadgeCollection() &&
		!AppGetBadgeCollection()->HasBadge(ACCT_TITLE_ADMINISTRATOR))
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_CONSOLE_ERROR_NO_PERMISSION ));
		return CR_OK;
	}

	CHAT_REQUEST_MSG_ANNOUNCE_TO_ALL_ON_SERVER announcement;
	announcement.Message.SetWStrMsg(strParams);
	announcement.bAlert = TRUE;
	c_ChatNetSendMessage(
		&announcement,
		USER_REQUEST_ANNOUNCE_TO_ALL_ON_SERVER);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdAdminChat)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	strParams = PStrSkipWhitespace(strParams);
	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	if (AppGetBadgeCollection() &&
		!AppGetBadgeCollection()->HasBadge(ACCT_TITLE_ADMINISTRATOR))
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_CONSOLE_ERROR_NO_PERMISSION ));	
		return CR_OK;
	}

	c_SendAdminChat(strParams);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdIgnore)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_ChatNetIgnoreMemberByName(strPlayer);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdUnignore)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_ChatNetUnignoreMember(strPlayer);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
// Implemented in c_chatIgnore.cpp
void c_ShowIgnoreList();
CONSOLE_CMD_FUNC(c_sCmdIgnoreWho)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_ShowIgnoreList();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdEmote)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (strParams[0] == 0)
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_Emote( strParams );
	return CR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(c_sCmdEmoteSpecific)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	const EMOTE_DATA *pEmote = (EMOTE_DATA *) ExcelGetData(game, DATATABLE_EMOTES, cmd->m_nData);
	ASSERT_RETVAL(pEmote, CR_FAILED);

	c_Emote(cmd->m_nData);

	return CRFLAG_NOSEND | CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdBuddyAdd)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer))) {
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	c_BuddyAdd(strPlayer);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdBuddyRemove)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer))) {
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_BuddyRemove(strPlayer);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdBuddyShowList)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_BuddyShowList();
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdWho)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	//	per Max Schaefer, changing this back to allowed by anyone

	CHAT_REQUEST_MSG_GET_TOTAL_MEMBER_COUNT msg;
	c_ChatNetSendMessage(&msg, USER_REQUEST_GET_TOTAL_MEMBER_COUNT);
	
	return CR_OK;
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdGetGameServer)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GAMEID idGame = GameGetId(game);
	SVRID idGameServer = GameIdGetSvrId(idGame);
	SVRINSTANCE nInstance = ServerIdGetInstance(idGameServer);

	s_SendSysTextFmt(CHAT_TYPE_WHISPER, TRUE, game, client, CONSOLE_SYSTEM_COLOR, 
		L"In server instance %d with game ID %#018I64x", nInstance, idGame);

	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdSeekDuel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

#if ISVERSION(SERVER_VERSION)
	UNIT * player = ClientGetPlayerUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	SendSeekAutoMatch(player);
#endif

	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdSeekCancel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

#if ISVERSION(SERVER_VERSION)
	UNIT * player = ClientGetPlayerUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	SendSeekCancel(player);
#endif

	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(c_sCmdDonate)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

#if !ISVERSION(SERVER_VERSION)
	MSG_CCMD_DONATE_MONEY msg;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, msg.nMoney))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_SendMessage(CCMD_DONATE_MONEY, &msg);
#endif

	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdDonationSetTotalCost)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nCost = 0;
	int nEventCount = 0;
	int nAnnouncementThreshhold = 0;

	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nCost))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nAnnouncementThreshhold))
	{
		trace("No event count found for donation cost setting; assuming infinite events");
		nAnnouncementThreshhold = DONATION_DEFAULT_THRESHHOLD;
	}

	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nEventCount))
	{
		trace("No event count found for donation cost setting; assuming infinite events");
		nEventCount = DONATION_INFINITY;
	}

	ASSERT_RETVAL(nCost >= 0, CRFLAG_BADPARAMS);

	ASSERT(s_GlobalGameEventSendSetDonationCost(nCost, cCurrency(0), nAnnouncementThreshhold, nEventCount) );
#endif

	return CR_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTeam)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * player = ClientGetPlayerUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	if (!strParams[0])
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	int team = TeamGetByName(game, strParams);
	if (team == INVALID_TEAM)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid team name: %s", strParams);	
		return CR_FAILED;
	}

	if (!TeamAddUnit(player, team))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"error joining team: %s", strParams);	
		return CR_FAILED;
	}

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	UnitGetName(player, strPlayer, arrsize(strPlayer), 0);
	s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, L"%s joined %s", strPlayer, strParams);	

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
#ifdef DEBUG_WINDOW
CONSOLE_CMD_FUNC(c_sCmdDebugWindow)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	if (DaveDebugWindowIsActive())
	{
		DaveDebugWindowHide();
	}
	else
	{
		DaveDebugWindowShow();
	}
#endif
	return CR_OK;
}
#endif
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdViewC)
{
	ASSERT_RETVAL(AppGetType() == APP_TYPE_CLOSED_SERVER, CR_FAILED);
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int idClient;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idClient))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	client = ClientGetById(game, idClient);
	if (!client)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"ERROR: Invalid Client ID [%d].", idClient);	
		return CR_FAILED;
	}
	UNIT * unit = ClientGetControlUnit(client);
	if (!unit)
	{
		return CR_FAILED;
	}
	GameSetControlUnit(game, unit);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdNoCheats)
{
	if (game)
	{
		GameSetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS, FALSE);
		if (IS_CLIENT(game))
		{
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Cheats Disabled");
		}
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdGameInfo)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * player = ClientGetPlayerUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	LEVEL * level = UnitGetLevel(player);
	ASSERT_RETVAL(level, CR_FAILED);
	const LEVEL_DEFINITION * level_data = LevelDefinitionGet(level);
	ASSERT_RETVAL(level_data, CR_FAILED);
	ROOM * room = UnitGetRoom(player);
	ASSERT_RETVAL(room && room->pDefinition, CR_FAILED);
	SUBLEVEL *pSubLevel = RoomGetSubLevel( room );
	ASSERT_RETVAL(pSubLevel, CR_FAILED);
	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Game Id:%I64d   Player Count:%d   Dungeon Seed:%lu", GameGetId(game), GameGetClientCount(game), level->m_dwSeed);	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, 
		L"Elite:%d League:%d Hardcore:%d", 
		GameIsVariant( game, GV_ELITE ),
		GameIsVariant( game, GV_LEAGUE ),
		GameIsVariant( game, GV_HARDCORE ));
	VECTOR position = UnitGetPosition(player);

	WCHAR strLevelName[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strLevelName, LevelGetDevName(level), arrsize(strLevelName));
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"World Location:%1.1f, %1.1f, %1.1f", position.fX, position.fY, position.fZ);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Level: (%s)", strLevelName);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Sub Level Monster/Desired Count: %d/%d", pSubLevel->nMonsterCount, pSubLevel->nDesiredMonsters);
	
	if (level->m_nDRLGDef != INVALID_ID)
	{
		const DRLG_DEFINITION * defDRLG = RoomGetDRLGDef(room);
		ASSERT_RETVAL(defDRLG, CR_FAILED);
		WCHAR strDrlgName[MAX_CONSOLE_PARAM_VALUE_LEN];
		PStrCvt(strDrlgName, defDRLG->pszName, arrsize(strDrlgName));
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"DRLG: %s", strDrlgName);
	}
	WCHAR strRoomName[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strRoomName, RoomGetDevName(room), arrsize(strRoomName));
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Room: (%s)", strRoomName);
	return CR_OK;
}
#endif

#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdBackgroundRoom)
{
	UNIT * player = ClientGetPlayerUnit(client);
	ROOM * room = UnitGetRoom(player);

	int nRoomOverride = PStrToInt(strParams);
	WCHAR strRoomName[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strRoomName, RoomGetDevName(room), arrsize(strRoomName));

	GAME_GLOBAL_DEFINITION* game_global_definition = DefinitionGetGameGlobal();
	game_global_definition->nRoomOverride = nRoomOverride;

	PStrCvt(strRoomName, RoomGetDevName(room), arrsize(strRoomName));
	//ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Generated room: (%s)", strRoomName);

	return CR_OK;
}
#endif

/*
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdBackgroundProp)
{
	int nDRLGOverride = PStrToInt(strParams);

	GAME_GLOBAL_DEFINITION* game_global_definition = DefinitionGetGameGlobal();
	game_global_definition->nLevelDefinition = nDRLGOverride;

	return CR_OK;
}
#endif
*/
/*
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdVersionInfo)
{
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"BOGGLE Version %d", PRODUCT_VERSION_STRING);
	return CR_OK;
}
#endif
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPlayerCount)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int monsterscaling;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, monsterscaling))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	if (monsterscaling < 1 || monsterscaling > 99)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"this command requires a numeric parameter between 1 and 99");	
	}
	AppCommonSetMonsterScalingOverride(monsterscaling);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"player count for difficulty scaling set to %d", monsterscaling);	
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTestEnvironment)
{
	
	ASSERT_RETVAL(game, CR_FAILED);			
	int nEnvironmentIndex(0);
	WCHAR uszEnvironment[MAX_CONSOLE_PARAM_VALUE_LEN];	
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, uszEnvironment, arrsize(uszEnvironment), nEnvironmentIndex))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"Environment", uszEnvironment);
		return CR_FAILED;
	}
	c_SetEnvironmentDef( game, 0, nEnvironmentIndex, "", TRUE, TRUE );
	
	if (game && IS_SERVER(game))
	{	
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Invalid Environment", nEnvironmentIndex);	
	}	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTestDefinition)
{
	testDefinition();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTestIndexedArray)
{
	testArrayIndexed();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdSpeed)
{
	float velocityOverride;


	if (!sConsoleParseFloatParam(cmd, 0, strParams, velocityOverride))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	PlayerSetVelocity(velocityOverride);
	if (game && IS_SERVER(game))
	{	
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"global player velocity override set to %4.2f", velocityOverride);	
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTestPhu)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bIsBeta = AppIsBeta();
	BOOL bIsDemo = AppIsDemo();
	if ( bIsBeta )
		ConsoleString( "Is Beta" );
	if ( bIsDemo )
		ConsoleString( "Is Demo" );
	int test;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, test))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	switch (test)
	{
	case 1:		// test scheduler
		test_EventSystem(game);
		break;
	case 2:		// test excel data loading
//		ExcelLoadData(game);
//		ExcelFreeData(game);
		break;
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#if !ISVERSION(SERVER_VERSION)
static BOOL sGSombergStopSound(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
#if !ISVERSION(SERVER_VERSION)
	int nSoundId = (int)event_data.m_Data1;
	c_SoundStop(nSoundId);
	return TRUE;
#endif
}
#endif

CONSOLE_CMD_FUNC(x_sCmdTestGSomberg)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETVAL(game, CR_FAILED);

	int test;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, test))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	switch (test)
	{
	case 0:
		{
			if(IS_SERVER(game))
				return CR_OK;

#if !ISVERSION(SERVER_VERSION)
			SOUND_ADSR_ENVELOPE* pADSREnvelope = c_SoundMemoryLoadADSRFileByName("t.xml");
			if(pADSREnvelope)
			{
				int nIndex = ExcelGetLineByStringIndex(NULL, DATATABLE_SOUNDS, "ElectricalBurnLoop");
				if(nIndex >= 0)
				{
					SOUND_PLAY_EXINFO tExInfo(pADSREnvelope);
					tExInfo.bForce2D = TRUE;
					int nSoundId = c_SoundPlay(nIndex, &c_SoundGetListenerPosition(), &tExInfo);
					UnitRegisterEventTimed(GameGetControlUnit(AppGetCltGame()), sGSombergStopSound, &EVENT_DATA(nSoundId), 100);
				}
			}
#endif
		}
		break;
	case 1:
		{
			if(IS_CLIENT(game))
				return CR_OK;

			UNIT * pDebugUnit = GameGetDebugUnit(game);
			if(pDebugUnit)
			{
				s_UnitVersionProps(pDebugUnit);
			}
		}
		break;
	default:
		{
		}
		break;
	}
#endif //!SERVER_VERSION
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdCDayServer)
{
	UNIT *pPlayer = ClientGetControlUnit( client );
	s_ItemsReturnToStandardLocations( pPlayer, TRUE );		
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdCDayClient)
{			
	return CR_OK;	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdGagToggle)
{
	UNIT *pPlayer = ClientGetControlUnit( client );
	if (s_GagIsGagged( pPlayer ))
	{
		s_GagDisable( pPlayer, TRUE );
	}
	else
	{
		time_t timeGaggedUntil = GagGetTimeFromNow( GAG_1_DAYS );		
		s_GagEnable( pPlayer, timeGaggedUntil, TRUE );
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdGagStatus)
{
	UNIT *pPlayer = ClientGetControlUnit( client );
	BOOL bGagged = s_GagIsGagged( pPlayer );
	if (bGagged == FALSE)
	{
		ConsoleString( L"Not Gagged" );
	}
	else
	{
		time_t timeGaggedUntil = s_GagGetGaggedUntil( pPlayer );
		tm t;
		if (gmtime_s( &t, &timeGaggedUntil ) == 0)
		{
			const int MAX_BUFFER = 1024;
			WCHAR uszBuffer[ MAX_BUFFER ];
			TimeTMToString( &t, uszBuffer, MAX_BUFFER );
			ConsoleString( L"Gagged until %s", uszBuffer );
		}
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdRecipesRefresh)
{
	UNIT *pPlayer = ClientGetControlUnit( client );	
	UnitClearStatAll( pPlayer, STATS_PERSONAL_CRAFTSMAN_LAST_BROWSED_TICK );
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdDifficultyMaxCycle)
{
	UNIT *pPlayer = ClientGetControlUnit( client );
	
	// increase max
	int nDifficultyMax = DifficultyGetMax( pPlayer );
	nDifficultyMax++;
		
	// wrap around
	const int nDifficultyHardest = ExcelGetNumRows( game, DATATABLE_DIFFICULTY );
	if (nDifficultyMax >= nDifficultyHardest)
	{
		nDifficultyMax = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
	}

	// set the new max
	DifficultySetMax( pPlayer, nDifficultyMax );			

	ConsoleString( "'%s' max difficulty is now '%d'", UnitGetDevName( pPlayer ), nDifficultyMax );
		
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdStatXferLog)
{
	gbStatXferLog = 1 - gbStatXferLog;
	ConsoleString( L"Stat Xfer Log: %s", sGetEnabledDisabledString( gbStatXferLog ) );		
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTestCMarch)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString( "sizeof(QUEST_GLOBALS) = %d bytes, %d KB", sizeof(QUEST_GLOBALS), sizeof(QUEST_GLOBALS)/1024 );
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdTransportTruth)
{
	UNIT *pPlayer = ClientGetControlUnit( client );

	s_QuestTransportToNewTruthRoom( pPlayer );
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdLostItemDebug)
{
	gbLostItemDebug = 1 - gbLostItemDebug;
	ConsoleString( 
		"Lost Item Debug: '%S'", 
		sGetEnabledDisabledString( gbLostItemDebug ));
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdSaveValidate)
{
	UNIT *pPlayer = ClientGetControlUnit( client );
	int nNumErrors = UnitFileValidateAll( pPlayer );
	ConsoleString( "Unit files validated.  '%d' errors encountered", nNumErrors );
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdIncrementalSaveLog)
{
	BOOL bEnabled = 1 - gbDBUnitLogEnabled;
	BOOL bEnabledInformClients = bEnabled;
	s_DBUnitLogEnable( game, bEnabled, bEnabledInformClients );
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL gbLoadingUpdateNow = TRUE;
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdLoadingUpdateNow)
{
	gbLoadingUpdateNow = 1 - gbLoadingUpdateNow;
	ConsoleString( 
		L"Loading Update Now: %s", 
		sGetYesNoString( gbLoadingUpdateNow ));	
	return CR_OK;	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdSafe)
{
	UNIT *pPlayer = ClientGetControlUnit( client );
	if (pPlayer)
	{
		UnitSetSafe( pPlayer );
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdFillpakLocalizedTextures)
{

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nSKU;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), nSKU))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"sku", strToken);
		return CR_FAILED;
	}

	int nFolderCount = ExcelGetNumRows( EXCEL_CONTEXT(), DATATABLE_LEVEL_FILE_PATHS );
	for (int i = 0; i < nFolderCount; ++i)
	{
		c_LevelLoadLocalizedTextures( game, i, nSKU );
	}

	return CR_OK;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdTownPortalAllowState)
{
	UNIT *pPlayer = ClientGetControlUnit( client );
	s_TownPortalsRestrictForUnit( pPlayer, !UnitHasState( game, pPlayer, STATE_TOWN_PORTAL_RESTRICTED ) );
	
	ConsoleString( 
		L"Town Portal Restricted State: %s", 
		sGetYesNoString( UnitHasState( game, pPlayer, STATE_TOWN_PORTAL_RESTRICTED ) ));
	
	return CR_OK;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdItemUpgrade)
{		
#if !ISVERSION(SERVER_VERSION)
	c_ItemUpgradeOpen(IUT_UPGRADE);
#endif	
	return CR_OK;	
}
#endif

//----------------------------------------------------------------------------
struct QUALITY_STAT
{
	int nWeaponDrops;
	int nArmorDrops;
	int nOtherDrops;
	int nQualityDrops;
};

//----------------------------------------------------------------------------
struct ITEM_QUALITY_STATS
{
	enum { MAX_QUALITIES = 16 };
	QUALITY_STAT tQualityStat[ MAX_QUALITIES ];
	int nTotalDrops;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ITEM_SPAWN_ACTION sTestQualities(
	GAME *pGame,
	UNIT *pPlayer,
	UNIT *pItem,
	int nItemClass,
	ITEM_SPEC *pItemSpawnSpec, 
	ITEMSPAWN_RESULT *pSpawnResult,
	void *pUserData)
{
	ITEM_QUALITY_STATS *pItemQualityStats = (ITEM_QUALITY_STATS *)pUserData;
	int nQuality = ItemGetQuality( pItem );
	ASSERTX_RETVAL( nQuality < ITEM_QUALITY_STATS::MAX_QUALITIES, ITEM_SPAWN_DESTROY, "Too many item qualities" );
	QUALITY_STAT *pQualityStat = &pItemQualityStats->tQualityStat[ nQuality ];
	
	if (UnitIsA( pItem, UNITTYPE_WEAPON ))
	{
		pQualityStat->nWeaponDrops++;
	}
	else if (UnitIsA( pItem, UNITTYPE_ARMOR ))
	{
		pQualityStat->nArmorDrops++;
	}
	else
	{
		pQualityStat->nOtherDrops++;
	}

	pQualityStat->nQualityDrops++;
	pItemQualityStats->nTotalDrops++;
	
	return ITEM_SPAWN_DESTROY;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdTreasureStats)
{
	ASSERT_RETVAL(game, CR_FAILED);		
	UNIT *pPlayer = ClientGetControlUnit( client );
		
	ITEM_QUALITY_STATS tItemQualityStats;
	memclear( &tItemQualityStats, sizeof( ITEM_QUALITY_STATS ) );

	WCHAR uszTreasure[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nTreasure;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, uszTreasure, arrsize(uszTreasure), nTreasure))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"treasure", uszTreasure);
		return CR_FAILED;
	}

	int nNumEvaluations = 500;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrIsNumeric(strToken))		// quantity
		{
			nNumEvaluations = PStrToInt(strToken);
		}
	}
	
	ITEM_SPEC tSpec;
	tSpec.pfnSpawnCallback = sTestQualities;
	tSpec.pSpawnCallbackData = &tItemQualityStats;	
	for (int i = 0; i < nNumEvaluations; ++i)
	{
		s_TreasureSpawnClass( pPlayer, nTreasure, &tSpec, NULL );
	}

	// display header
	ConsoleString( L"" );
	ConsoleString(
		L"Treasure Class '%S' evaluated '%d' times:", 
		ExcelGetStringIndexByLine( game, DATATABLE_TREASURE, nTreasure ),
		nNumEvaluations);
		
	// display quality info
	int nNumQualities = ExcelGetNumRows( game, DATATABLE_ITEM_QUALITY );	
	for (int i = 0; i < nNumQualities; ++i)
	{
		const ITEM_QUALITY_DATA *pQualityData = ItemQualityGetData( i );
		ASSERT_CONTINUE(pQualityData);
		QUALITY_STAT *pQualityStat = &tItemQualityStats.tQualityStat[ i ];
		
		ConsoleString( 
			L" % 12S: % 6d items [% 6.2f%%] (weapons=%d, armor=%d, other=%d)",
			pQualityData->szName,
			pQualityStat->nQualityDrops,
			((float)pQualityStat->nQualityDrops / (float)tItemQualityStats.nTotalDrops) * 100.0f,
			pQualityStat->nWeaponDrops, 
			pQualityStat->nArmorDrops, 
			pQualityStat->nOtherDrops);
	}

	ConsoleString(  L" -------------------------------------------------" );
	ConsoleString(  
		L" % 12S: % 5d items - '%S' evaluated %d times", 
		"Total", 
		tItemQualityStats.nTotalDrops,
		ExcelGetStringIndexByLine( game, DATATABLE_TREASURE, nTreasure ),
		nNumEvaluations);
			
	return CR_OK;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdCreateNoSpawnZone)
{
	
	// get player
	UNIT *pPlayer = ClientGetControlUnit( client );
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	LevelAddSpawnFreeZone( game, pLevel, UnitGetPosition( pPlayer ), 4.0f, TRUE );
	
	return CR_OK;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdOfferGiveReplace)
{
	ASSERT_RETVAL(game, CR_FAILED);

	// get player
	UNIT *pPlayer = ClientGetControlUnit( client );
	
	WCHAR uszOffer[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nOffer;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, uszOffer, arrsize(uszOffer), nOffer))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"offer", uszOffer);
		return CR_FAILED;
	}

	OFFER_ACTIONS tActions;
	s_OfferDestroyAndGiveNew( pPlayer, pPlayer, nOffer, tActions );
	
	return CR_OK;
			
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdOfferDisplay)
{
	ASSERT_RETVAL(game, CR_FAILED);

	// get player
	UNIT *pPlayer = ClientGetControlUnit( client );
	
	WCHAR uszOffer[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nOffer;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, uszOffer, arrsize(uszOffer), nOffer))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"offer", uszOffer);
		return CR_FAILED;
	}

	OFFER_ACTIONS tActions;
	s_OfferDisplay( pPlayer, pPlayer, nOffer, tActions );
	
	return CR_OK;
			
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdOfferGive)
{
	ASSERT_RETVAL(game, CR_FAILED);

	// get player
	UNIT *pPlayer = ClientGetControlUnit( client );
	
	WCHAR uszOffer[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nOffer;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, uszOffer, arrsize(uszOffer), nOffer))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"offer", uszOffer);
		return CR_FAILED;
	}

	OFFER_ACTIONS tActions;
	s_OfferGive( pPlayer, pPlayer, nOffer, tActions );
	
	return CR_OK;
			
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdMerchantsRefresh)
{
	ASSERT_RETVAL(game, CR_FAILED);

	// get player
	UNIT *pPlayer = ClientGetControlUnit( client );
	s_StoreTryExpireOldItems( pPlayer, TRUE );
		
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdCredits)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	BOOL isPlus = FALSE;
	if (strParams[0] == L'+')
	{
		isPlus = TRUE;
		++strParams;
	}

	int val;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, val))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	if (val == 0 || (val < 0 && isPlus))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	UnitAddCurrency(player, cCurrency( 0, val ) );

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"You now have %d credits.", UnitGetCurrency( player ).GetValue( KCURRENCY_VALUE_REALWORLD ) );	
		
	return CR_OK;
}
#endif

#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED) || _PROFILE
CONSOLE_CMD_FUNC(s_sCmdMoney)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	BOOL isPlus = FALSE;
	if (strParams[0] == L'+')
	{
		isPlus = TRUE;
		++strParams;
	}

	int val;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, val))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	if (val == 0 || (val < 0 && isPlus))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	UnitAddCurrency(player, cCurrency( val, 0 ) );

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"You now have %d money.", UnitGetCurrency( player ).GetValue( KCURRENCY_VALUE_INGAME ) );	
		
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sSpawnItemField(
	GAME * game,
	GAMECLIENT * client,
	int * pnItemClass,
	int nNumClasses)
{
	UNIT * player = ClientGetControlUnit(client);
	
	BOOL bElementEffects[MAX_DAMAGE_TYPES];
	memclear(bElementEffects, arrsize(bElementEffects) * sizeof(BOOL));

	// init spawn spec
	ITEM_SPEC tSpec;
	SETBIT(tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT);
	tSpec.nMoneyAmount = 100;
	tSpec.nRealWorldMoneyAmount = 0;
	
	// find a bunch of places to spawn at
	NEAREST_NODE_SPEC tNodeSpec;
	SETBIT(tNodeSpec.dwFlags, NPN_SORT_OUTPUT);
	SETBIT(tNodeSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES);
	tNodeSpec.fMaxDistance = 100.0f;

	const int MAX_PATH_NODES = 128;
	FREE_PATH_NODE tFreePathNodes[MAX_PATH_NODES];
	unsigned int nNumNodes = (unsigned int)RoomGetNearestPathNodes(game, UnitGetRoom(player), UnitGetPosition(player), MAX_PATH_NODES, tFreePathNodes, &tNodeSpec);

	// spawn them
	for (unsigned int ii = 0; ii < nNumNodes; ++ii)
	{	
		const FREE_PATH_NODE * freePathNode = &tFreePathNodes[ii];
		VECTOR vPosition = RoomPathNodeGetWorldPosition(game, freePathNode->pRoom, freePathNode->pNode);
		tSpec.pvPosition = &vPosition;
		
		// pick a unit type
		int itemClass = pnItemClass[RandGetNum(game, 0, nNumClasses - 1)];
		const UNIT_DATA * itemData = ItemGetData(itemClass);
		
		// if unit is money, assign value
		if (UnitIsA(itemData->nUnitType, UNITTYPE_MONEY))
		{
			tSpec.nMoneyAmount = 100;
			tSpec.nRealWorldMoneyAmount = 0;
		}
		else
		{
			tSpec.nMoneyAmount = 0;
			tSpec.nRealWorldMoneyAmount = 0;
		}
		
		tSpec.nItemClass = itemClass;
		tSpec.pSpawner = player;

		s_ItemSpawnForCheats(game, tSpec, client, ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT, bElementEffects);	
	}	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMoneyPiles)
{
	int nItemClasses[1];
	nItemClasses[0] = GlobalIndexGet(GI_ITEM_MONEY);
	
	sSpawnItemField(game, client, nItemClasses, arrsize(nItemClasses));
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPowerupField)
{
	int nItemClasses[2];
	nItemClasses[0] = GlobalIndexGet(GI_ITEM_HEALTH_MODULE);
	nItemClasses[1] = GlobalIndexGet(GI_ITEM_POWER_MODULE);
	
	sSpawnItemField(game, client, nItemClasses, arrsize(nItemClasses));
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static int s_sCmdItemGetItemClassByDisplayName(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * & strParams,
	ITEM_SPEC & spec)
{
	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_ITEMS);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{
		const UNIT_DATA * itemData = (const UNIT_DATA *)ItemGetData(ii);
		if ( ! itemData )
			continue;

		const WCHAR * strItemName = StringTableGetStringByIndex(itemData->nString);
		ASSERT_RETVAL(strItemName, INVALID_LINK);

		unsigned int lenItemName = PStrLen(strItemName);
		if (lenItemName <= 0)
			continue;

		if (PStrICmp(strItemName, strParams, lenItemName) == 0 && sIsValidParamEnd(strParams[lenItemName]))
		{
			strParams = sConsoleSkipDelimiter(strParams + lenItemName);

			// quick hack for money
			if (UnitIsA(itemData->nUnitType, UNITTYPE_MONEY))
			{
				spec.nMoneyAmount = RandGetNum(game, 100, 200);
				spec.nRealWorldMoneyAmount = 0;
			}
			
			return ii;
		}
	}
	ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid item name for %s command.", cmd->m_strCommand.GetStr());	
	return INVALID_LINK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static int s_sCmdItemGetItemClassOrType(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	unsigned int idxParam,
	const WCHAR * & strParams,
	BOOL & byType,
	ITEM_SPEC & spec)
{
	byType = FALSE;

	if (PStrICmp(cmd->m_strCommand.GetStr(), L"itemr", MAX_CONSOLE_COMMAND_LEN) == 0)
	{
		return s_sCmdItemGetItemClassByDisplayName(game, client, cmd, strParams, spec);
	}

	WCHAR strToken[CONSOLE_MAX_STR];
	if (PStrICmp(cmd->m_strCommand.GetStr(), L"itemtype", MAX_CONSOLE_COMMAND_LEN) == 0)
	{
		int itemType;
		if (!sConsoleParseExcelLookupOrName(game, cmd, DATATABLE_UNITTYPES, strParams, strToken, arrsize(strToken), itemType))
		{
			sConsolePrintMissingOrInvalidValue(game, client, L"item type", strToken);
			return INVALID_LINK;
		}
		byType = TRUE;
		return itemType;
	}

	int itemClass;
	if (!sConsoleParseExcelLookupOrName(game, cmd, idxParam, strParams, strToken, arrsize(strToken), itemClass))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"item", strToken);
		return INVALID_LINK;
	}

	const UNIT_DATA * itemData = (const UNIT_DATA *)ItemGetData(itemClass);
	ASSERT_RETVAL(itemData, INVALID_LINK);

	// quick hack for money
	if (UnitIsA(itemData->nUnitType, UNITTYPE_MONEY))
	{
		spec.nMoneyAmount = RandGetNum(game, 100, 200);
		spec.nRealWorldMoneyAmount = 0;
	}
			
	return itemClass;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static BOOL s_sCmdItemParseQuality(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * strToken,
	const UNIT_DATA * itemData,
	ITEM_SPEC & spec)
{
	strToken = PStrSkipWhitespace(strToken);
	if (strToken[0] == 0)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing quality specifier for %s command.", cmd->m_strCommand.GetStr());	
		return FALSE;
	}

	char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chToken, strToken, arrsize(chToken));

	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_ITEM_QUALITY);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{
		const ITEM_QUALITY_DATA * qualityData = ItemQualityGetData(ii);
		ASSERT_RETFALSE(qualityData);
		if (PStrICmp(qualityData->szName, chToken) == 0)
		{
			if (!itemData || TESTBIT(itemData->pdwQualities, ii))
			{
				spec.nQuality = ii;
				break;
			}
			else
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"quality specifier not valid for %s command.", cmd->m_strCommand.GetStr());
				return FALSE;
			}
		}
	}
	if (spec.nQuality <= INVALID_LINK)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"unrecognized quality specifier for %s command.", cmd->m_strCommand.GetStr());
		return FALSE;
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static int s_sCmdItemParseAffixesGetNextAffixIndex(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	ITEM_SPEC & spec)
{
	for (unsigned int ii = 0; ii < MAX_SPEC_AFFIXES; ii++)
	{
		if (spec.nAffixes[ii] == INVALID_LINK)
		{
			return ii;
		}
	}
	ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"too many affix specifiers for %s command.", cmd->m_strCommand.GetStr());	
	return MAX_SPEC_AFFIXES;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static BOOL s_sCmdItemParseAffixes(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * strToken,
	const UNIT_DATA * itemData,
	int itemType,
	ITEM_SPEC & spec)
{
	unsigned int idxAffix = s_sCmdItemParseAffixesGetNextAffixIndex(game, client, cmd, spec);
	if (idxAffix >= MAX_SPEC_AFFIXES)
	{
		return FALSE;
	}

	strToken = PStrSkipWhitespace(strToken);
	if (strToken[0] == 0)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing affix specifier for %s command.", cmd->m_strCommand.GetStr());	
		return FALSE;
	}

	char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chToken, strToken, arrsize(chToken));

	int nTokLen = PStrLen(chToken);
	DWORD dwToken = INVALID_ID;
	if (nTokLen <= 4)
	{
		dwToken = STRTOFOURCC(chToken);
	}

	int nLowestLevelAffix = INVALID_LINK;
	int nLowestLevelAffixLevel = 1000;

	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_AFFIXES);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{			
		const AFFIX_DATA * affixData = AffixGetData(ii);				
		if(!affixData)
			continue;
		
		if (affixData->dwCode != dwToken && PStrICmp(affixData->szName, chToken) != 0)
		{
			BOOL bFoundOne = FALSE;
			for (int jj = 0; jj < ANT_NUM_TYPES; jj++)
			{
				if (affixData->nStringName[jj] != INVALID_LINK)
				{
					const WCHAR *szFullName = StringTableGetStringByIndex(affixData->nStringName[jj]);
					if (szFullName &&
						PStrICmp(szFullName, strToken, nTokLen) == 0)
					{
						bFoundOne = TRUE;
						break;
					}
				}
			}

			if (!bFoundOne)
				continue;
		}

		if (affixData->codeWeight == NULL_CODE)
		{
			ConsoleStringForceOpen(CONSOLE_ERROR_COLOR, L"Affix specifier [%s] is set to not spawn", strToken, cmd->m_strCommand.GetStr());	
			continue;
		}
		if (!IsAllowedUnit((itemData ? itemData->nUnitType : itemType), affixData->nAllowTypes, AFFIX_UNITTYPES))
		{
			WCHAR szName[256] = L"NULL";
			if (itemData)
			{
				PStrCvt(szName, itemData->szName, arrsize(szName));
			}
			else
			{
				if (itemType >= 0 && itemType < (int)ExcelGetCount(NULL, DATATABLE_UNITTYPES))
				{
					const UNITTYPE_DATA* unittype_data = (UNITTYPE_DATA*)ExcelGetData(NULL, DATATABLE_UNITTYPES, itemType);
					if (unittype_data != NULL)
					{
						int nString = unittype_data->nDisplayName;
						if (nString != INVALID_LINK)
						{
							PStrCopy(szName, StringTableGetStringByIndex(nString), arrsize(szName));
						}
					}
				}
			}
			ConsoleStringForceOpen(CONSOLE_ERROR_COLOR, L"Item [%s] has is an invalid type for affix specifier [%s]", szName, strToken);	
			continue;
		}

		if (spec.nLevel > 0 && (affixData->nMinLevel > spec.nLevel || affixData->nMaxLevel < spec.nLevel))
		{
			// save it for later - we might bump the level up so we can get this affix
			if (itemData != NULL)		// we're not going by type
			{
				WCHAR szName[256] = L"NULL";
				PStrCvt(szName, itemData->szName, arrsize(szName));
				ConsoleStringForceOpen(CONSOLE_ERROR_COLOR, L"Item {%s] has wrong level [%d] for affix specifier [%s]", szName, spec.nLevel, strToken);	
			}
			else if (affixData->nMinLevel < nLowestLevelAffixLevel)
			{
				nLowestLevelAffixLevel = affixData->nMinLevel;
				nLowestLevelAffix = ii;
			}

			continue;
		}
		spec.nAffixes[idxAffix] = ii;
		++idxAffix;
		return TRUE;

	}

	if (nLowestLevelAffix != INVALID_LINK)
	{
		spec.nLevel = nLowestLevelAffixLevel;
		ConsoleStringForceOpen(CONSOLE_ERROR_COLOR, L"Bumping item level to [%d] for affix specifier [%s]", spec.nLevel, strToken);	
		spec.nAffixes[idxAffix] = nLowestLevelAffix;
		++idxAffix;
		return TRUE;
	}

	ConsoleStringForceOpen(CONSOLE_ERROR_COLOR, L"Could not find affix for specifier [%s] for %s command.", strToken, cmd->m_strCommand.GetStr());	
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static BOOL s_sCmdItemParseSkill(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * strToken,
	ITEM_SPEC & spec)
{
	strToken = PStrSkipWhitespace(strToken);
	if (strToken[0] == 0)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing skill specifier for %s command.", cmd->m_strCommand.GetStr());	
		return FALSE;
	}

	char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chToken, strToken, arrsize(chToken));

	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_SKILLS);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{
		const SKILL_DATA * skillData = SkillGetData(game, ii);
		if ( ! skillData )
			continue;

		if (PStrICmp(skillData->szName, chToken, arrsize(chToken)) == 0)
		{
			spec.nSkill = ii;
			return TRUE;
		}
	}
	ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid skill specifier [%s] for %s command.", strToken, cmd->m_strCommand.GetStr());	
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static BOOL s_sCmdItemParseParticleElement(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * strToken,
	ITEM_SPEC & spec,
	BOOL bElementEffects[MAX_DAMAGE_TYPES])
{
	strToken = PStrSkipWhitespace(strToken);
	if (strToken[0] == 0)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing particle element specifier for %s command.", cmd->m_strCommand.GetStr());	
		return FALSE;
	}

	char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chToken, strToken, arrsize(chToken));

	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_DAMAGETYPES);
	for (unsigned int ii = 0; ii < numRows; ii++)
	{
		const DAMAGE_TYPE_DATA * damageData = DamageTypeGetData(game, ii);
		if(!damageData)
			continue;
		if (PStrICmp(damageData->szDamageType, chToken) == 0)
		{
			if (ii == 0)
			{
				for (unsigned int jj = 0; jj < arrsize(bElementEffects); jj++)
				{
					bElementEffects[ii] = TRUE;
				}
			}
			else
			{
				bElementEffects[ii] = TRUE;
			}
			return TRUE;
		}
	}

	ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid particle element specifier [%s] for %s command.", strToken, cmd->m_strCommand.GetStr());	
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static void s_sInitSpawnSpec(
	ITEM_SPEC & spec,
	BOOL bElementEffects[MAX_DAMAGE_TYPES],
	UNIT * player)
{
	ItemSpawnSpecInit(spec);
	spec.nLevel = UnitGetExperienceLevel(player);
	spec.nLevel = MAX(spec.nLevel, 1);

	memclear(bElementEffects, MAX_DAMAGE_TYPES * sizeof(BOOL));
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static BOOL s_sCmdItemParseSpecifiers(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * & strParams,
	const UNIT_DATA * itemData,
	int itemClassOrType,
	ITEM_SPEC & spec,
	BOOL bElementEffects[MAX_DAMAGE_TYPES],
	int & quantity,
	BOOL & bEquip,
	BOOL & bAddMods)
{
	// parse qualifiers
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrIsNumeric(strToken))		// quantity
		{
			quantity = PStrToInt(strToken);
			quantity = MAX(1, quantity);
		}
		else if (strToken[0] == 'e')						// equip
		{
			bEquip = TRUE;
		}
		else if (strToken[0] == 'u')						// useable
		{
			SETBIT(spec.dwFlags, ISF_USEABLE_BY_SPAWNER_BIT);
		}
		else if (strToken[0] == 'i')						// auto identify item
		{
			SETBIT(spec.dwFlags, ISF_IDENTIFY_BIT);
		}		
		else if (strToken[0] == 'm')						// add mods
		{
			bAddMods = TRUE;
			SETBIT(spec.dwFlags, ISF_MAX_SLOTS_BIT);
		}
		else if (strToken[0] == 'f')						// full set of slots ( but no mods )
		{	
			SETBIT(spec.dwFlags, ISF_MAX_SLOTS_BIT);
		}
		else if (PStrICmp(strToken, L"l=", 2) == 0)			// level
		{
			spec.nLevel = PStrToInt(strToken + 2);
			if (spec.nLevel <= 0)
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid level specifier [%d] for %s command.", spec.nLevel, cmd->m_strCommand.GetStr());	
				return FALSE;
			}
		}
		else if (PStrICmp(strToken, L"n=", 2) == 0)			// number
		{
			spec.nNumber = PStrToInt(strToken + 2);
			if (spec.nNumber <= 0)
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid level specifier [%d] for %s command.", spec.nNumber, cmd->m_strCommand.GetStr());	
				return FALSE;
			}
		}
		else if (PStrICmp(strToken, L"q=", 2) == 0)			// quality
		{
			if (!s_sCmdItemParseQuality(game, client, cmd, strToken + 2, itemData, spec))
			{
				return FALSE;
			}
		}
		else if (PStrCmp(strToken, L"r=", 2) == 0)			// rank
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"rank specifier is NYI.");	
		}
		else if (PStrCmp(strToken, L"a=", 2) == 0)			// affixes
		{
			if (!s_sCmdItemParseAffixes(game, client, cmd, strToken + 2, itemData, itemClassOrType, spec))
			{
				return FALSE;
			}
		}
		else if (PStrCmp(strToken, L"s=", 2) == 0)			// skill
		{
			if (!s_sCmdItemParseSkill(game, client, cmd, strToken + 2, spec))
			{
				return FALSE;
			}
		}
		else if (PStrCmp(strToken, L"p=", 2) == 0)			// particle element
		{
			if (!s_sCmdItemParseParticleElement(game, client, cmd, strToken + 2, spec, bElementEffects))
			{
				return FALSE;
			}
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid specifier [%s] for %s command.", strToken, cmd->m_strCommand.GetStr());
			return FALSE;
		}
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdItem)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ITEM_SPEC spec;
	BOOL bElementEffects[MAX_DAMAGE_TYPES];
	s_sInitSpawnSpec(spec, bElementEffects, player);
	
	BOOL byType;
	int itemClassOrType = s_sCmdItemGetItemClassOrType(game, client, cmd, 0, strParams, byType, spec);

	const UNIT_DATA * itemData = NULL;
	if (!byType)
	{
		if (itemClassOrType < 0)
		{
			return CR_FAILED;
		}
		itemData = (const UNIT_DATA *)ItemGetData(itemClassOrType);
		ASSERT_RETVAL(itemData, CR_FAILED);
	}

	BOOL bEquip = FALSE;
	BOOL bAddMods = FALSE;
	int quantity = 1;
	if (!s_sCmdItemParseSpecifiers(game, client, cmd, strParams, itemData, itemClassOrType, spec, bElementEffects, quantity, bEquip, bAddMods))
	{
		return CR_FAILED;
	}

	if (bEquip == TRUE)
	{
		SETBIT(spec.dwFlags, ISF_PICKUP_BIT);	
	}
	else
	{
		SETBIT(spec.dwFlags, ISF_PLACE_IN_WORLD_BIT);	
	}
	SETBIT(spec.dwFlags, ISF_AFFIX_SPEC_ONLY_BIT, spec.nAffixes[0] != INVALID_LINK);

	int nQuantityLeft = quantity;
	while (nQuantityLeft)
	{
		BOOL bDidStacking = FALSE;
		
		if (byType)
		{
			s_DebugTreasureSpawnUnitType(player, itemClassOrType, &spec, NULL);
		}
		else
		{
			spec.nItemClass = itemClassOrType;
			spec.pSpawner = player;

			DWORD dwFlags = ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT;
			if ( bEquip )
				dwFlags |= ITEM_SPAWN_CHEAT_FLAG_PICKUP | ITEM_SPAWN_CHEAT_FLAG_FORCE_EQUIP;
			if ( bAddMods )
				dwFlags |= ITEM_SPAWN_CHEAT_FLAG_ADD_MODS;
			UNIT *pItem = s_ItemSpawnForCheats( game, spec, client, dwFlags, bElementEffects );
			
			// do stacking quantities if we can instead of spawning a unit for each one
			if (pItem && nQuantityLeft > 1 && UnitIsA( pItem, UNITTYPE_MONEY ) == FALSE)
			{
				ASSERT( ItemGetQuantity( pItem ) == 1 );
				int nMaxQuantity = UnitGetStat( pItem, STATS_ITEM_QUANTITY_MAX );
				if (nMaxQuantity > 1)
				{
					int nThisStack = nQuantityLeft;
					if (nThisStack > nMaxQuantity)
					{
						nThisStack = nMaxQuantity;
					}
					ItemSetQuantity( pItem, nThisStack );
					nQuantityLeft -= nThisStack;
					bDidStacking = TRUE;
				}
				
			}
			
		}
		if (bDidStacking == FALSE)
		{
			nQuantityLeft--;
		}
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdIdentifyAll)
{
	UNIT *pPlayer = ClientGetControlUnit( client );
	if (pPlayer)
	{
		s_ItemIdentifyAll( pPlayer );
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdItemsAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	int itemType = UNITTYPE_ANY;
	int tempType = itemType;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), tempType))
	{
		itemType = tempType;
	}

	ITEM_SPEC spec;
	BOOL bElementEffects[MAX_DAMAGE_TYPES];
	s_sInitSpawnSpec(spec, bElementEffects, player);

	BOOL bEquip = FALSE;
	BOOL bAddMods = FALSE;
	int quantity = 1;
	if (!s_sCmdItemParseSpecifiers(game, client, cmd, strParams, NULL, itemType, spec, bElementEffects, quantity, bEquip, bAddMods))
	{
		return CR_FAILED;
	}
	int quality = spec.nQuality;

	PickerStart(game, picker);
	unsigned int itemCount = ExcelGetNumRows(game, DATATABLE_ITEMS);
	for (unsigned int ii = 0; ii < itemCount; ++ii)
	{
		const UNIT_DATA * itemData = ItemGetData(ii);
		if (! itemData )
			continue;

		if (!UnitDataTestFlag( itemData, UNIT_DATA_FLAG_SPAWN ))
		{
			continue;
		}
		if (!UnitIsA(itemData->nUnitType, itemType))
		{
			continue;
		}
		PickerAdd(MEMORY_FUNC_FILELINE(game, (int)ii, 1));
	}

	while (!PickerIsEmpty(game))
	{
		int ii = PickerChooseRemove(game);
		if (ii < INVALID_INDEX)
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"error choosing item to spawn for %s command.", cmd->m_strCommand.GetStr());
		}

		const UNIT_DATA * itemData = ItemGetData(ii);
		ASSERT_BREAK(itemData);
		ASSERT_BREAK(UnitDataTestFlag( itemData, UNIT_DATA_FLAG_SPAWN ));

		if (bEquip == TRUE)
		{
			SETBIT(spec.dwFlags, ISF_PICKUP_BIT);	
		}
		else
		{
			SETBIT(spec.dwFlags, ISF_PLACE_IN_WORLD_BIT);	
		}
		SETBIT(spec.dwFlags, ISF_AFFIX_SPEC_ONLY_BIT, spec.nAffixes[0] != INVALID_LINK);

		if (quality != INVALID_LINK && TESTBIT(itemData->pdwQualities, quality))
		{
			spec.nQuality = quality;
		}

		for (int jj = 0; jj < quantity; ++jj)
		{
			spec.nItemClass = ii;
			spec.pSpawner = player;
			DWORD dwFlags = ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT;
			if ( bEquip )
				dwFlags |= ITEM_SPAWN_CHEAT_FLAG_PICKUP | ITEM_SPAWN_CHEAT_FLAG_FORCE_EQUIP;
			if ( bAddMods )
				dwFlags |= ITEM_SPAWN_CHEAT_FLAG_ADD_MODS;

			s_ItemSpawnForCheats(game, spec, client, dwFlags, bElementEffects);
		}
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSaveAllItems)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ITEM_SPEC spec;
	BOOL bElementEffects[MAX_DAMAGE_TYPES];
	s_sInitSpawnSpec(spec, bElementEffects, player);

	unsigned int itemCount = ExcelGetNumRows(game, DATATABLE_ITEMS);
	for (unsigned int ii = 0; ii < itemCount; ++ii)
	{
		const UNIT_DATA * itemData = ItemGetData(ii);
		if(!itemData)
			continue;

		trace("Attempting to spawning %s...", itemData->szName);

		SETBIT(spec.dwFlags, ISF_AFFIX_SPEC_ONLY_BIT, spec.nAffixes[0] != INVALID_LINK);

		spec.nItemClass = ii;
		spec.pSpawner = player;
		DWORD dwFlags = ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT | ITEM_SPAWN_CHEAT_FLAG_DONT_SEND;
		UNIT * pUnitToTest = s_ItemSpawnForCheats(game, spec, client, dwFlags, bElementEffects);

		if(pUnitToTest)
		{
			trace("Succeeded!\n");
			UnitUpdateRoom(pUnitToTest, UnitGetRoom(player), FALSE);
			for(int i=0; i<NUM_UNITSAVEMODES; i++)
			{
				trace("Attempting Unit Save Mode %d...", i);
				// setup xfer
				BYTE data[ UNITFILE_MAX_SIZE_ALL_UNITS ];
				BIT_BUF buf(data, UNITFILE_MAX_SIZE_ALL_UNITS);
				XFER<XFER_SAVE> tXfer(&buf );
				UNIT_FILE_XFER_SPEC<XFER_SAVE> tSpec(tXfer);
				tSpec.pUnitExisting = pUnitToTest;
				tSpec.eSaveMode = (UNITSAVEMODE)i;
				tSpec.idClient = ClientGetId( client );
				if (UnitFileXfer( game, tSpec ) != pUnitToTest)
				{
					trace("Failed!\n");
					const int MAX_MESSAGE = 256;
					char szMessage[ MAX_MESSAGE ];
					PStrPrintf( 
						szMessage, 
						MAX_MESSAGE, 
						"Error with save type %d, please attach character files to bug.  Unable to xfer unit '%s' [%d] for saving to buffer",
						i,
						UnitGetDevName( pUnitToTest ),
						UnitGetId( pUnitToTest ));
					ASSERTX( 0, szMessage );							
				}

				ASSERT(buf.GetLen() > 0);

				int nSavedSize = buf.GetLen();	
				ASSERTX(nSavedSize > 0, "Player save buffer size is zero" );
				trace("Succeeded!\n");
			}

			UnitFree(pUnitToTest, UFF_SEND_TO_CLIENTS);
		}
		else
		{
			trace("Failed!\n");
		}
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdGridClear)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	{
		UNIT * item = UnitInventoryLocationIterate(player, UnitGetBigBagInvLoc(), NULL);
		while (item)
		{
			UnitFree(item, UFF_SEND_TO_CLIENTS);
			item = UnitInventoryLocationIterate(player, UnitGetBigBagInvLoc(), NULL);
		}
	}

	{
		UNIT * item = UnitInventoryLocationIterate(player, INVLOC_SMALLPACK, NULL);
		while (item)
		{
			UnitFree(item, UFF_SEND_TO_CLIENTS);
			item = UnitInventoryLocationIterate(player, INVLOC_SMALLPACK, NULL);
		}
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdAchievementsRevealAll)
{
	ASSERT_RETVAL(game, CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_REVEAL_ALL_ACHIEVEMENTS);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Show all achievements: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_REVEAL_ALL_ACHIEVEMENTS)));
#if !ISVERSION(SERVER_VERSION)
	UIRepaint();
#endif

	return CR_OK;
}

CONSOLE_CMD_FUNC(s_sCmdAchievementClear)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strAchievement[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nAchievement;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strAchievement, arrsize(strAchievement), nAchievement))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"achievement", strAchievement);
		return CR_FAILED;
	}

	s_CheatCheatCheatAchievementProgress(player, nAchievement, TRUE, FALSE, 0);

	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdRecipeLearn)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strRecipe[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nRecipe;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strRecipe, arrsize(strRecipe), nRecipe))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"Recipe", strRecipe);
		return CR_FAILED;
	}
	PlayerLearnRecipe( player, nRecipe );	

	return CR_OK;
}
CONSOLE_CMD_FUNC(s_sCmdRecipeLearnAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int nCount = (int)ExcelGetNumRows(game, DATATABLE_RECIPES);
	for( int t = 0; t < nCount; t++ )
	{
		const RECIPE_DEFINITION *pRecipeDefinition = RecipeGetDefinition( t );
		if( pRecipeDefinition )
		{
			if( pRecipeDefinition->nWeight > 0 &&
				pRecipeDefinition->nSpawnLevels[0] > 0 )
			{
				PlayerLearnRecipe( player, t );				
			}
		}
	}

	return CR_OK;
}

#endif
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdRecipeTestAll)
{
	
	RecipeToggleCheating();

	return CR_OK;
}

#endif
#if ISVERSION(DEVELOPMENT)


CONSOLE_CMD_FUNC(s_sCmdAchievementProgress)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strAchievement[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nAchievement;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strAchievement, arrsize(strAchievement), nAchievement))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"achievement", strAchievement);
		return CR_FAILED;
	}

	int nProgress;
	if (!sConsoleParseIntegerParam(cmd, 1, strParams, nProgress))
	{
		nProgress = 1;
	}

	s_CheatCheatCheatAchievementProgress(player, nAchievement, FALSE, FALSE, nProgress);

	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdAchievementFinish)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strAchievement[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nAchievement;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strAchievement, arrsize(strAchievement), nAchievement))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"achievement", strAchievement);
		return CR_FAILED;
	}

	s_CheatCheatCheatAchievementProgress(player, nAchievement, FALSE, TRUE, 0);

	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdAchievementFinishAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	unsigned int nCount = ExcelGetCount(game, DATATABLE_ACHIEVEMENTS);
	for (unsigned int iAchievement = 0; iAchievement < nCount; ++iAchievement)
	{
		s_CheatCheatCheatAchievementProgress(player, iAchievement, FALSE, TRUE, 0);
	}
#if !ISVERSION(SERVER_VERSION)
	UIRepaint();
#endif

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdAffixAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	unsigned int numAffixes = ExcelGetNumRows(game, DATATABLE_AFFIXES);
	unsigned int numItems = ExcelGetNumRows(game, DATATABLE_ITEMS);
	unsigned int numQualities = ExcelGetNumRows(game, DATATABLE_ITEM_QUALITY);

	for (unsigned int ii = 0; ii < numAffixes; ++ii)
	{
		const AFFIX_DATA * affixData = AffixGetData(ii);
		if (!affixData || affixData->codeWeight == NULL_CODE || affixData->nAffixType[0] < 0)
		{
			continue;
		}

		ITEM_SPEC spec;
		spec.nLevel = 99;
		spec.nAffixes[0] = ii;

		for (unsigned int jj = 0; jj < numItems; ++jj)
		{
			const UNIT_DATA * itemData = ItemGetData(jj);
			if (!itemData || !UnitDataTestFlag( itemData, UNIT_DATA_FLAG_SPAWN ) || itemData->nRarity <= 0)
			{
				continue;
			}
			if (!IsAllowedUnit(itemData->nUnitType, affixData->nAllowTypes, AFFIX_UNITTYPES))
			{
				continue;
			}
			for (unsigned int kk = 0; kk < numQualities; ++kk)
			{
				if (!TESTBIT(itemData->pdwQualities, kk))
				{
					continue;
				}
				const ITEM_QUALITY_DATA * qualityData = (const ITEM_QUALITY_DATA *)ExcelGetData(game, DATATABLE_ITEM_QUALITY, kk);
				if (!qualityData)
				{
					continue;
				}

				BOOL found = FALSE;
				if (AffixIsType(affixData, qualityData->nPrefixType))
				{
					found = TRUE;
				}
				else if (AffixIsType(affixData, qualityData->nSuffixType))
				{
					found = TRUE;
				}
/*
				if (!found)
				{
					for (int aff = 0; aff < MAX_AFFIX_GEN; aff++)
					{
						if (AffixIsType(affixData, qualityData->nAffixType[aff]))
						{
							found = TRUE;
							break;
						}
					}
				}
*/
				if (!found)
				{
					continue;
				}
				spec.nQuality = kk;
			}
			SETBIT(spec.dwFlags, ISF_PLACE_IN_WORLD_BIT);

			spec.nItemClass = jj;
			spec.pSpawner = player;
			UNIT * item = s_ItemSpawn(game, spec, NULL);
			if (!item)
			{
				WCHAR strItemName[MAX_CONSOLE_PARAM_VALUE_LEN];
				PStrCvt(strItemName, itemData->szName, arrsize(strItemName));
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"error spawning item [%s].", strItemName);
				continue;
			}
			break;
		}
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdAffixFocus)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chToken, strParams, arrsize(strParams));

	DWORD dwToken = INVALID_ID;
	if (PStrLen(chToken) <= 4)
	{
		dwToken = STRTOFOURCC(chToken);
	}

	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_AFFIXES);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{			
		const AFFIX_DATA * affixData = AffixGetData(ii);				
		ASSERT_RETFALSE(affixData);
		
		if (affixData->dwCode == dwToken || PStrICmp(affixData->szName, chToken) == 0)
		{
			gnAffixFocus = ii;

			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"affix focus set to [%s].", strParams);	
			return CR_OK;
		}
	}
	ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid affix [%s].", strParams);	
	return CR_FAILED;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
struct MONSTER_SPAWN_SPEC
{
	int m_monsterClass;
	int m_monsterQuality;
	BOOL m_bWeapon;
	BOOL m_bDead;
	BOOL m_bUnique;
	unsigned int m_count;
	VECTOR *m_position;
	float m_fDistanceFromPosition;
	float m_fZOffset;
	int m_Level;
	
	MONSTER_SPAWN_SPEC::MONSTER_SPAWN_SPEC(int monsterClass) :
			m_monsterClass(monsterClass),
			m_monsterQuality(INVALID_LINK),
			m_bWeapon(FALSE),
			m_bDead(FALSE),
			m_bUnique(FALSE),
			m_count(1),
			m_position(NULL),
			m_fDistanceFromPosition(2.0f),
			m_fZOffset(0.0f),
			m_Level( -1 )
	{ 
	}
};
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static BOOL x_sKillMonster(
	GAME * game,
	UNIT * monster,
	const struct EVENT_DATA & event_data)
{
	VECTOR delta = RandGetVector(game);
	delta.fZ = -0.2f;

#if defined(HELLGATE)
	if(AppIsHellgate())
	{
		HAVOK_IMPACT impact;
		HavokImpactInit(game, impact, RandGetFloat(game, 5.0f, 8.0f), delta, NULL);
		UnitAddImpact(game, monster, &impact);
	}
#endif

	UnitDie(monster, NULL);
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static BOOL x_sSpawnMonster(
	GAME * game,
	UNIT * player,
	const MONSTER_SPAWN_SPEC & spec)
{
	ASSERT_RETFALSE(game && player);
	ASSERT_RETFALSE(spec.m_monsterClass != INVALID_LINK);
	
	VECTOR controlPosition = (spec.m_position ? *spec.m_position : (player ? UnitGetPosition(player) : cgvNull));
	VECTOR direction;
	if (!sGetSpawnVectors(player, NULL, &direction))
	{
		return FALSE;
	}
	
	int weaponClass = INVALID_ID;
	if (spec.m_bWeapon && IS_SERVER(game))
	{
		UNIT * weapon = UnitInventoryGetByLocation(player, INVLOC_RHAND);
		if (!weapon)
		{
			weapon = UnitInventoryGetByLocation(player, INVLOC_LHAND);
		}
		weaponClass = weapon ? UnitGetClass(weapon) : INVALID_ID;
	}

	VECTOR delta = direction * spec.m_fDistanceFromPosition;
	VECTOR position = controlPosition + delta;

	unsigned int spawnCount = 0;
	for (unsigned int ii = 0; ii < spec.m_count; ++ii)
	{
		position += direction;
		ROOM * room = NULL;
		RoomGetNearestFreePathNode(game, UnitGetLevel(player), position, &room);
		if (!room)
		{
			continue;
		}

		position.fZ += spec.m_fZOffset;

		int monsterLevel = spec.m_Level <= 0 ? RoomGetMonsterExperienceLevel(room) : spec.m_Level;
		UNIT * monster = NULL;
		if (IS_CLIENT(game))
		{
			monster = MonsterSpawnClient(game, spec.m_monsterClass, room, position, -direction);
		}
		else
		{
			MONSTER_SPEC monsterSpec;
			monsterSpec.nClass = spec.m_monsterClass;
			monsterSpec.nExperienceLevel = monsterLevel;
			monsterSpec.nMonsterQuality = spec.m_monsterQuality;
			monsterSpec.pRoom = room;
			monsterSpec.vPosition = position;
			monsterSpec.vDirection = -direction;
			monsterSpec.nWeaponClass = weaponClass;
			monster = s_MonsterSpawn(game, monsterSpec);			
		}

		if (monster)
		{
			s_RoomSpawnMinions( game, monster, room, &position );

			++spawnCount;
			if (spec.m_bDead)
			{
				UnitRegisterEventTimed(monster, x_sKillMonster, EVENT_DATA(0), 20);
			}
		}
	}
	return (spawnCount > 0);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static BOOL x_sCmdMonsterParseSpecifiers(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * & strParams,
	MONSTER_SPAWN_SPEC & spec)
{
	ASSERT_RETFALSE(game);

	// parse qualifiers
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrIsNumeric(strToken))		// count
		{
			spec.m_count = PStrToInt(strToken);
			spec.m_count = MAX((unsigned int)1, spec.m_count);
		}
		else if (PStrICmp(strToken[0], L'd'))
		{
			spec.m_bDead = TRUE;
		}
		else if (PStrICmp(strToken[0], L'w'))
		{
			spec.m_bWeapon = TRUE;
		}
		else if (PStrICmp(strToken[0], L'c'))
		{
			spec.m_monsterQuality = MonsterQualityPick(game, MQT_CHAMPION);
		}
		else if (PStrICmp(strToken[0], L'u'))
		{
			spec.m_monsterQuality = MonsterQualityPick(game, MQT_UNIQUE);
			spec.m_bUnique = TRUE;
		}
		else if (PStrICmp(strToken[0], L'f'))
		{
			spec.m_fDistanceFromPosition = 30.0f;
			if (strToken[1] == L'=')
			{
				spec.m_fDistanceFromPosition = PStrToFloat(strToken + 2);
			}
		}
		else if (PStrICmp(strToken[0], L'z'))
		{
			if (strToken[1] == L'=')
			{
				spec.m_fZOffset = PStrToFloat(strToken+2);
			}
		}
		else if (PStrICmp(strToken[0], L'l'))
		{
			if (strToken[1] == L'=')
			{
				spec.m_Level = PStrToInt(strToken+2);
			}
		}
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
static int x_sCmdParseMonsterClass(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * & strParams)
{
	ASSERT_RETINVALID(game && (client || IS_CLIENT(game)) && cmd && strParams);

	int monsterClass;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseExcelLookupOrName(game, cmd, DATATABLE_MONSTERS, strParams, strToken, arrsize(strToken), monsterClass))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"monster", strToken);	
		return INVALID_LINK;
	}
	return monsterClass;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdMonster)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	int monsterClass = x_sCmdParseMonsterClass(game, client, cmd, strParams);
	if (monsterClass == INVALID_LINK)
	{
		return CR_FAILED;
	}

	MONSTER_SPAWN_SPEC spec(monsterClass);
	
	if (!x_sCmdMonsterParseSpecifiers(game, client, cmd, strParams, spec))
	{
		return CR_FAILED;
	}

	x_sSpawnMonster(game, player, spec);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
ROOM_ITERATE_UNITS sMonsterNextFreeUnit(
	UNIT *pUnit,
	void *pCallbackData )
{
	DWORD dwUnitFreeFlags = UFF_DELAYED | UFF_SEND_TO_CLIENTS;
	UnitFree(pUnit, dwUnitFreeFlags);
	return RIU_CONTINUE;
}


static int sgnNextMonsterIndex = 0;
CONSOLE_CMD_FUNC(s_sCmdMonsterNext)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	TARGET_TYPE eTargetTypes[] = {TARGET_BAD, TARGET_INVALID};
	LevelIterateUnits(UnitGetLevel(player), eTargetTypes, sMonsterNextFreeUnit, NULL);

	int nMonsterCount = ExcelGetNumRows(game, DATATABLE_MONSTERS);

	BOOL bSpawnedMonster = FALSE;
	BOUNDED_WHILE(!bSpawnedMonster, nMonsterCount)
	{
		int monsterClass = sgnNextMonsterIndex;
		if (monsterClass < 0 || monsterClass >= nMonsterCount)
		{
			monsterClass = 0;
			sgnNextMonsterIndex = 0;
		}
		sgnNextMonsterIndex++;

		const UNIT_DATA * pUnitData = (const UNIT_DATA *)ExcelGetData(game, DATATABLE_MONSTERS, monsterClass);
		if(pUnitData && !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GOOD))
		{
			MONSTER_SPAWN_SPEC spec(monsterClass);
			bSpawnedMonster = x_sSpawnMonster(game, player, spec);
		}
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdProp)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	char pszParam[256];
	PStrCvt(pszParam, strParams, 256);

	int nProp;
	//nProp = ExcelGetLineByStringIndex(EXCEL_CONTEXT(game), DATATABLE_PROPS, pszParam);
	nProp = RoomFileGetRoomIndexLine(pszParam);

	if(nProp < 0)
	{
		return CR_FAILED;
	}

	ROOM_INDEX * pPropIndex = (ROOM_INDEX *)ExcelGetData(EXCEL_CONTEXT(game), DATATABLE_PROPS, nProp);
	if(!pPropIndex)
	{
		return CR_FAILED;
	}

	ROOM_DEFINITION * pProp = PropDefinitionGet(game, pPropIndex->pszFile, TRUE, 0.0f );
	if (!pProp)
	{
		return CR_FAILED;
	}

	UNIT * pPlayer = GameGetControlUnit(game);
	if(!pPlayer)
	{
		return CR_FAILED;
	}
	FREE_PATH_NODE tOutput;
	NEAREST_NODE_SPEC tSpec;
	tSpec.fMinDistance = 2.0f;
	tSpec.fMaxDistance = 5.0f;
	tSpec.pUnit = pPlayer;
	tSpec.vFaceDirection = UnitGetFaceDirection(pPlayer, FALSE);
	SETBIT(tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES);
	SETBIT(tSpec.dwFlags, NPN_ALLOW_BLOCKED_NODES);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
	SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
	SETBIT(tSpec.dwFlags, NPN_IN_FRONT_ONLY);
	SETBIT(tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY);
	SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
	int nCount = RoomGetNearestPathNodes(game, UnitGetRoom(pPlayer), UnitGetPosition(pPlayer), 1, &tOutput, &tSpec);
	if(nCount <= 0)
	{
		return CR_FAILED;
	}

	MATRIX matFinal;
	VECTOR vPosition = RoomPathNodeGetExactWorldPosition(game, tOutput.pRoom, tOutput.pNode);
	VECTOR vNormal = VECTOR(0, 0, 1);
	float fRotation = 0.0f;
	GetAlignmentMatrix(&matFinal, tOutput.pRoom->tWorldMatrix, vPosition, vNormal, fRotation);

	// We're on the client, so it should be okay to have this be static
	static int snModelId = INVALID_ID;

	if(snModelId >= 0)
	{
		c_ModelRemove(snModelId);
	}
	V( e_ModelNew( &snModelId, pProp->nModelDefinitionId ) );
	c_ModelSetLocation(snModelId, &matFinal, vPosition, RoomGetId(tOutput.pRoom), MODEL_STATIC );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMonsterClient)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = GameGetControlUnit(game);
	ASSERT_RETVAL(player, CR_FAILED);

	int monsterClass = x_sCmdParseMonsterClass(game, client, cmd, strParams);
	if (monsterClass == INVALID_LINK)
	{
		return CR_FAILED;
	}

	MONSTER_SPAWN_SPEC spec(monsterClass);

	if (!x_sCmdMonsterParseSpecifiers(game, client, cmd, strParams, spec))
	{
		return CR_FAILED;
	}

	x_sSpawnMonster(game, player, spec);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMonsterZoo)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ROOM * room = UnitGetRoom(player);
	ASSERT_RETVAL(room, CR_FAILED);
		
	// create each monster
	unsigned int numMonsters = ExcelGetNumRows(game, DATATABLE_MONSTERS);
	for (unsigned int ii = 0; ii < numMonsters; ++ii)
	{
		const UNIT_DATA * monsterData = MonsterGetData(game, ii);
		if ( ! monsterData )
			continue;

		if (!UnitDataTestFlag( monsterData, UNIT_DATA_FLAG_SPAWN ))
		{
			continue;
		}
		int quality = MonsterQualityPick(game, MQT_CHAMPION);
		
		int idxNode = s_RoomGetRandomUnoccupiedNode(game, room, 0);
		ASSERT_CONTINUE(idxNode >= 0);
		ROOM_PATH_NODE * node = RoomGetRoomPathNode(room, idxNode);
		ASSERT_CONTINUE(node);
		VECTOR position = RoomPathNodeGetExactWorldPosition(game, room, node);

		MONSTER_SPAWN_SPEC spec(ii);
		spec.m_monsterQuality = quality;
		spec.m_bUnique = TRUE;
		spec.m_position = &position;
		
		x_sSpawnMonster(game, player, spec);
	}

	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdGlobalMonsterLevel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	int val;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, val))
	{
		if (val < 1 || val > MonsterGetMaxLevel(game))
			val = -1;	// reset to default

		gnMonsterLevelOverride = val;
	}

	if (val == -1)
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Monster level override is now cleared (default levels)");
	else
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Monster level override is now '%d'", val);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdGlobalItemLevel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	int val;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, val))
	{
		val = PIN(val, 1, (int)ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_ITEM_LEVELS));
		gnItemLevelOverride = val;
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Item level override is now '%d'", val);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMonstersAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ROOM * room = UnitGetRoom(player);
	ASSERT_RETVAL(room, CR_FAILED);

	VECTOR position, direction;
	if (!sGetSpawnVectors(player, &position, &direction))
	{
		return CR_FAILED;
	}

	int level = RoomGetMonsterExperienceLevel(room);
	REF(level);

	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_MONSTERS);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{
		const UNIT_DATA * monsterData = MonsterGetData(game, ii);
		if (! monsterData )
			continue;

		if (!UnitDataTestFlag( monsterData, UNIT_DATA_FLAG_SPAWN ))
		{
			continue;
		}
		position += direction;

		MONSTER_SPAWN_SPEC spec(ii);
		spec.m_position = &position;
		
		if (!x_sSpawnMonster(game, player, spec))
		{
			WCHAR strMonsterName[MAX_CONSOLE_PARAM_VALUE_LEN];
			PStrCvt(strMonsterName, monsterData->szName, arrsize(strMonsterName));
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unable to spawn monster [%d: %s (%s)] for some reason.", ii, strMonsterName, StringTableGetStringByIndex(monsterData->nString));
		}
	}
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMonstersPossible)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	// if no version package specified, use the current level the player is in
	const int MAX_MONSTERS = 2048;
	int nMonsterClasses[ MAX_MONSTERS ];
	int nNumMonsters = 0;	
		
	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	// what level is the command requester on
	LEVEL * level = UnitGetLevel(player);
	ASSERT_RETVAL(level, CR_FAILED)
	
	// get possible monsters
	nNumMonsters = s_LevelGetPossibleRandomMonsters(
		level, 
		nMonsterClasses, 
		MAX_MONSTERS, 
		nNumMonsters, 
		TRUE);

	// header of our output will be this level/drlg
	const DRLG_DEFINITION * defDRLG = LevelGetDRLGDefinition(level);
	ASSERT_RETVAL(defDRLG, CR_FAILED)

	WCHAR strDRLGName[MAX_CONSOLE_PARAM_VALUE_LEN] = { 0 };	
	PStrCvt(strDRLGName, defDRLG->pszName, arrsize(strDRLGName));
	ConsoleString( " " );		
	ConsoleString(
		L"The following %d monsters can appear in this level instance of '%s'", 
		nNumMonsters, 
		strDRLGName);
			
	for (int ii = 0; ii < nNumMonsters; ++ii)
	{
		const UNIT_DATA * monsterData = MonsterGetData(game, nMonsterClasses[ii]);
		WCHAR strMonsterName[MAX_CONSOLE_PARAM_VALUE_LEN];
		PStrCvt(strMonsterName, monsterData->szName, arrsize(strMonsterName));
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"   %02d)  %s (%s)", ii + 1, strMonsterName, StringTableGetStringByIndex(monsterData->nString));
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMonstersPossibleVersion)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	// if no version package specified, use the current level the player is in
	const int MAX_MONSTERS = 2048;
	int nMonsterClasses[ MAX_MONSTERS ];
	int nNumMonsters = 0;	
				
	// scan through all levels that are available in the requested version package, and create
	// a huge master mast list of all monsters possible
	int nNumLevels = ExcelGetNumRows( NULL, DATATABLE_LEVEL );
	for (int nLevelDef = 0; nLevelDef < nNumLevels; ++nLevelDef)
	{
	
		// setup flags to get all possible DRLGs
		DWORD dwPossibleFlags = 0;
		SETBIT( dwPossibleFlags, PF_DRLG_ALL_BIT );
		LevelDefGetPossibleMonsters( 
			game,
			nLevelDef, 
			dwPossibleFlags, 
			nMonsterClasses, 
			&nNumMonsters, 
			MAX_MONSTERS);
		
	}

	const int MAX_MESSAGE = 1024;
	char szMessage[ MAX_MESSAGE ];
	PStrCopy( szMessage, " \n", MAX_MESSAGE );
	ConsoleString( szMessage ); trace( szMessage );
	PStrPrintf( szMessage, MAX_MESSAGE, "The following %d monsters can appear in this version\n", nNumMonsters );
	ConsoleString( szMessage ); trace( szMessage );

	int nNumMonsterRows = ExcelGetNumRows( NULL, DATATABLE_MONSTERS );			
	for (int ii = 0; ii < nNumMonsterRows; ++ii)
	{
		BOOL bFound = FALSE;
		for (int j = 0; j < nNumMonsters; ++j)
		{
			if (nMonsterClasses[ j ] == ii)
			{
				bFound = TRUE;
				break;
			}
		}
		
		if (bFound == TRUE)
		{
			const UNIT_DATA * monsterData = MonsterGetData(game, ii);
			WCHAR strMonsterName[MAX_CONSOLE_PARAM_VALUE_LEN];
			PStrCvt(strMonsterName, monsterData->szName, arrsize(strMonsterName));
			PStrPrintf( szMessage, MAX_MESSAGE, "   %02d)  %S (%S)\n", ii + 1, strMonsterName, StringTableGetStringByIndex(monsterData->nString));
			ConsoleString( szMessage ); trace( szMessage );
		}
		else
		{
			ConsoleString(L""); trace("\n");
		}
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL c_sBalanceTestIsFinished(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	if ( ! LevelGetMonsterCountLivingAndBad( UnitGetLevel( unit ), RMCT_DENSITY_VALUE_OVERRIDE ) )
	{
		int nStartingTick	= (int) event_data.m_Data1;
		int nTestGroup		= (int) event_data.m_Data2;
		int nDeltaTicks = GameGetTick( game ) - nStartingTick;

		float fDeltaSeconds = (float) nDeltaTicks / GAME_TICKS_PER_SECOND_FLOAT;
		ConsoleString( "Balance Test group %d time %f seconds", nTestGroup, fDeltaSeconds );
	} else {
		UnitRegisterEventTimed( unit, c_sBalanceTestIsFinished, event_data, 1 );
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define BALANCE_TEST_LEVEL 30
#define BALANCE_MONSTER_PACK_SIZE 25
CONSOLE_CMD_FUNC(s_sCmdBalanceTest)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ROOM * room = UnitGetRoom(player);
	ASSERT_RETVAL(room, CR_FAILED);

	BOOL bDoTest = TRUE;
	int nPlayerLevel = UnitGetExperienceLevel( player );
	if ( nPlayerLevel != BALANCE_TEST_LEVEL )
	{
		ConsoleString( L"Your player needs to be level %d", BALANCE_TEST_LEVEL );
		bDoTest = FALSE;
	}
	int pnWeaponLevels[ MAX_WEAPONS_PER_UNIT ];
	{
		int nWardrobe = UnitGetWardrobe( player );
		ASSERT_RETVAL( nWardrobe != INVALID_ID, CR_FAILED );
		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i ++ )
		{
			UNIT * pWeapon = WardrobeGetWeapon( game, nWardrobe, i );
			pnWeaponLevels[ i ] = pWeapon ? UnitGetExperienceLevel( pWeapon ) : INVALID_ID;

			if ( pWeapon && pnWeaponLevels[ i ] != BALANCE_TEST_LEVEL )
			{
				const int MAX_STRING = 256;
				WCHAR pszWeaponName[ MAX_STRING ];
				if ( UnitGetName( pWeapon, pszWeaponName, MAX_STRING, 0 ) )
				{
					ConsoleString( L"Your weapon, %s, needs to be level %d not level %d", pszWeaponName, BALANCE_TEST_LEVEL, pnWeaponLevels[ i ] );
					bDoTest = FALSE;
				}
			}
		}
	}

	if ( ! bDoTest )
		return CR_FAILED;

	// change the monster spawn level
	ConsoleString( L"Changing monster level to %d", BALANCE_TEST_LEVEL );
	game->m_nDebugMonsterLevel = BALANCE_TEST_LEVEL;

	int nTestGroup = 0;
	sConsoleParseIntegerParam(cmd, 0, strParams, nTestGroup);

	// create each monster
	unsigned int numMonsters = ExcelGetNumRows(game, DATATABLE_MONSTERS);
	for (unsigned int ii = 0; ii < numMonsters; ++ii)
	{
		const UNIT_DATA * monsterData = MonsterGetData(game, ii);
		if ( ! monsterData )
			continue;
		
		if ( monsterData->nBalanceTestCount == 0 )
			continue;
		if ( monsterData->nBalanceTestGroup != nTestGroup )
			continue;
		MONSTER_SPAWN_SPEC spec(ii);

		for ( int nSpawnCount = monsterData->nBalanceTestCount; nSpawnCount > 0; nSpawnCount -= BALANCE_MONSTER_PACK_SIZE )
		{
			spec.m_count = MIN( nSpawnCount, BALANCE_MONSTER_PACK_SIZE );	
			x_sSpawnMonster(game, player, spec);
		}
	}

	EVENT_DATA event_data( GameGetTick( game ), nTestGroup );

	UnitRegisterEventTimed( player, c_sBalanceTestIsFinished, event_data, 1 );

	return CR_OK;			
}
#endif

#if ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdResurrect)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT *pPlayer = ClientGetControlUnit(client);
	ASSERT_RETVAL(pPlayer, CR_FAILED);
	
	if (UnitIsGhost( pPlayer ))
	{
		UnitGhost( pPlayer, FALSE );
	}

	return CR_OK;
		
}
#endif

#if ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdHeadstoneDebug)
{
	gbHeadstoneDebug = 1 - gbHeadstoneDebug;
	ConsoleString("Headstone Debug %s", sGetYesNoString( gbHeadstoneDebug ) );	
	return CR_OK;		
}
#endif

#if ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(c_sCmdPlayMovie)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR uszMovie[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nMovie;
	sConsoleParseToken(cmd, strParams, uszMovie, arrsize(uszMovie));
	char pszMovie[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(pszMovie, uszMovie, arrsize(uszMovie));
	nMovie = ExcelGetLineByStringIndex(game, DATATABLE_MOVIELISTS, pszMovie);
	if(nMovie == INVALID_ID)
	{
		return CR_FAILED;
	}

	AppSwitchState(APP_STATE_PLAYMOVIELIST, nMovie);

	return CR_OK;		
}
#endif

#if ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdLQMovies)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

#if !ISVERSION(SERVER_VERSION) && BINK_ENABLED
	extern BOOL gbForceLowQualityMovies;
	gbForceLowQualityMovies = !gbForceLowQualityMovies;

	ConsoleString("Low Quality Movies %S", sGetOnOffString( gbForceLowQualityMovies ) );
#endif //!ISVERSION(SERVER_VERSION)

	return CR_OK;
}
#endif //ISVERSION(DEVELOPMENT)

//----------------------------------------------------------------------------
enum MONSTER_SPAWN_STAT_TYPE
{
	MSST_CURRENT,
	MSST_ALL,
};

//----------------------------------------------------------------------------
struct SPAWN_STAT_DATA
{
	enum { MAX_UNIT_TYPES = 2048 };
	int nUnitTypeCount[ MAX_UNIT_TYPES ];
	int nExperience[ MAX_UNIT_TYPES ];
	enum { MAX_SPAWN_CLASSES = 1024 };
	int nSpawnClassCount[ MAX_SPAWN_CLASSES ];
	UNIT *pSubject;
	MONSTER_SPAWN_STAT_TYPE eType;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sDoSpawnStats(
	UNIT *pUnit,
	void *pUserData)
{
	SPAWN_STAT_DATA *pSpawnStatData = (SPAWN_STAT_DATA *)pUserData;
		
	if (UnitIsA( pUnit, UNITTYPE_MONSTER ) &&
		UnitsAreInSameSubLevel( pUnit, pSpawnStatData->pSubject ))
	{	
		int nUnitClass = UnitGetClass( pUnit );
		ASSERTX_RETVAL( nUnitClass >= 0 && nUnitClass < SPAWN_STAT_DATA::MAX_UNIT_TYPES, RIU_CONTINUE, "Too many unit types" );
		pSpawnStatData->nUnitTypeCount[ nUnitClass ]++;
		pSpawnStatData->nExperience[ nUnitClass ] += UnitGetExperienceValue( pUnit );
		
		int nSpawnClassSource = UnitGetStat( pUnit, STATS_SPAWN_CLASS_SOURCE );
		if (nSpawnClassSource != INVALID_LINK)
		{
			ASSERTX_RETVAL( nSpawnClassSource >= 0 && nSpawnClassSource < SPAWN_STAT_DATA::MAX_SPAWN_CLASSES, RIU_CONTINUE, "Too many spawn classes" );
			pSpawnStatData->nSpawnClassCount[ nSpawnClassSource ]++;
		}
		
	}
	return RIU_CONTINUE;
}

#if ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoMonsterSpawnStats(
	LEVEL *pLevel,
	UNIT *pActivator,
	MONSTER_SPAWN_STAT_TYPE eType)
{

	// clear console for this whole page of data coming up
	ConsoleClear();

	// activate all rooms for "all" type so all monsters are created
	if (eType == MSST_ALL)
	{
		s_DebugLevelActivateAllRooms( pLevel, pActivator );
	}
	
	// collect stats about monsters
	SPAWN_STAT_DATA tSpawnStatData;
	memclear( &tSpawnStatData, sizeof( tSpawnStatData ) );
	tSpawnStatData.pSubject = pActivator;
	tSpawnStatData.eType = eType;
	LevelIterateUnits( pLevel, NULL, sDoSpawnStats, &tSpawnStatData );

	// count up totals
	int nMonsterTotal = 0;
	for (int i = 0; i < SPAWN_STAT_DATA::MAX_UNIT_TYPES; ++i)
	{
		int nCount = tSpawnStatData.nUnitTypeCount[ i ];
		if (nCount > 0)
		{
			nMonsterTotal += nCount;
		}
	}
	int nSpawnClassTotal = 0;
	for (int i = 0; i < SPAWN_STAT_DATA::MAX_SPAWN_CLASSES; ++i)
	{
		int nCount = tSpawnStatData.nSpawnClassCount[ i ];
		if (nCount > 0)
		{
			nSpawnClassTotal += nCount;
		}
	}

	// display header
	ConsoleString( " " );
	ConsoleString( "Monsters spawned statistics for sub-level '%s'", LevelGetDevName( pLevel ) );
	ConsoleString( " ---------------------------------------------" );
	
	// display results
	int nTotalExperience = 0;
	for (int i = 0; i < SPAWN_STAT_DATA::MAX_UNIT_TYPES; ++i)
	{
		int nUnitClass = i;
		int nCount = tSpawnStatData.nUnitTypeCount[ nUnitClass ];
		if (nCount > 0)
		{
			const UNIT_DATA *pMonsterData = MonsterGetData( NULL, nUnitClass );
			const WCHAR *puszDisplayName = StringTableGetStringByIndex( pMonsterData->nString );
			nTotalExperience += tSpawnStatData.nExperience[ nUnitClass ];
			ConsoleString( " % 5d [% 5.2f%%] %s (%S) - XP: %d", 
				nCount, 
				(float)nCount / (float)nMonsterTotal, 
				pMonsterData->szName,
				puszDisplayName,
				tSpawnStatData.nExperience[ nUnitClass ]);
		}
	}
	ConsoleString( " % 5d TOTAL MONSTERS", nMonsterTotal );
	ConsoleString( " % 5d TOTAL EXPERIENCE", nTotalExperience );

	// display spawn classes used
	ConsoleString( " " );
	ConsoleString( "Monster Spawn Class Sources");
	ConsoleString( " ---------------------------------------------" );	
	for (int i = 0; i < SPAWN_STAT_DATA::MAX_SPAWN_CLASSES; ++i)
	{
		int nSpawnClass = i;
		int nCount = tSpawnStatData.nSpawnClassCount[ nSpawnClass ];
		if (nCount > 0)
		{
			const char *pszName = ExcelGetStringIndexByLine( NULL, DATATABLE_SPAWN_CLASS, nSpawnClass );
			ConsoleString( 
				" % 5d [% 5.2f%%] %s", 
				nCount, 
				(float)nCount / (float)nSpawnClassTotal, 
				pszName);
		}
	}
	ConsoleString( " % 5d TOTAL", nSpawnClassTotal );
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMonsterSpawnStatsCurrent)
{
	UNIT *pPlayer = ClientGetControlUnit( client );	
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	sDoMonsterSpawnStats( pLevel, pPlayer, MSST_CURRENT );
	return CR_OK;		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMonsterSpawnStatsAll)
{
	UNIT *pPlayer = ClientGetControlUnit( client );	
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	sDoMonsterSpawnStats( pLevel, pPlayer, MSST_ALL );
	return CR_OK;		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdZombie)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	MONSTER_SPAWN_SPEC spec(GlobalIndexGet(GI_MONSTER_ZOMBIE));
	x_sSpawnMonster(game, player, spec);
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdChampionSpawnTest)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	int monsterClass = x_sCmdParseMonsterClass(game, client, cmd, strParams);
	if (monsterClass == INVALID_LINK)
	{
		return CR_FAILED;
	}

	// create a spawn entry just for this single target
	SPAWN_ENTRY spawn;
	spawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
	spawn.nIndex = monsterClass;
	spawn.codeCount = (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT;

	int quality = RandGetNum(game, 1, ExcelGetNumRows(game, DATATABLE_MONSTER_QUALITY) - 1);

	VECTOR position;
	ASSERT_RETVAL(sGetPositionInFrontOfUnit(player, 3.0f, &position), CR_FAILED);

	ROOM * room = NULL;
	ROOM_PATH_NODE * pathnode = RoomGetNearestFreePathNode(game, UnitGetLevel(player), position, &room);
	if (!pathnode)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Couldn't find a spawn spot for monster.");
		return CR_FAILED;
	}

	SPAWN_LOCATION spawnLocation;
#if HELLGATE_ONLY
	spawnLocation.vSpawnPosition = pathnode->vPosition;
#else
	spawnLocation.vSpawnPosition = VECTOR( GetPathNodeX( room->pPathDef, pathnode->nOffset ), GetPathNodeY( room->pPathDef, pathnode->nOffset ), 0 );
#endif
	spawnLocation.vSpawnDirection = UnitGetFaceDirection(player, FALSE);
	spawnLocation.vSpawnUp = UnitGetUpDirection(player);
	spawnLocation.fRadius = 0.0f;

	s_RoomSpawnMonsterByMonsterClass(game, &spawn, quality, room, &spawnLocation, NULL, NULL);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSpawnClass)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	LEVEL * level = UnitGetLevel(player);
	ASSERT_RETVAL(level, CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int spawnClass;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), spawnClass))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"spawn class", strToken);
		return CR_FAILED;
	}

	SPAWN_ENTRY spawnArray[MAX_SPAWNS_AT_ONCE];
	unsigned int spawnCount = SpawnClassEvaluate(
		game, 
		level->m_idLevel, 
		spawnClass, 
		RoomGetMonsterExperienceLevel( UnitGetRoom( player ) ), 
		spawnArray, 
		NULL);
		
	// spawn each of the monsters
	for (unsigned int ii = 0; ii < spawnCount; ++ii)
	{
		SPAWN_ENTRY * spawn = &spawnArray[ii];
		
		MONSTER_SPAWN_SPEC spec(spawn->nIndex);
		spec.m_count = SpawnEntryEvaluateCount(game, spawn);
		
		x_sSpawnMonster(game, player, spec);		
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
// clear all spawn memory for every level and reset
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSpawnMemoryReset)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	unsigned int levelCount = DungeonGetNumLevels(game);
	for (unsigned int ii = 0; ii < levelCount; ++ii)
	{
		LEVEL * level = LevelGetByID(game, (LEVELID)ii);	
		LevelResetSpawnMemory(game, level);
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Cleared all monster spawn memory.");
	return CR_OK;
}	
#endif

//----------------------------------------------------------------------------
static const int MAX_MONSTER_CLASSES = KILO * 2;

//----------------------------------------------------------------------------
struct SPAWN_CLASS_STATS
{
	int nCount;	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSpawnClassTest)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	LEVEL * level = UnitGetLevel(player);
	ASSERT_RETVAL(level, CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nSpawnClass;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), nSpawnClass))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"spawn class", strToken);
		return CR_FAILED;
	}

	int nNumEvaluations = 500;
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrIsNumeric(strToken))
		{
			nNumEvaluations = PStrToInt(strToken);
		}
	}

	// init results
	SPAWN_CLASS_STATS tResults[ MAX_MONSTER_CLASSES ];
	for (int i = 0; i < MAX_MONSTER_CLASSES; ++i)
	{
		tResults[ i ].nCount = 0;
	}
	
	// do all the evaluations
	int nTotalCount = 0;
	for (int i = 0; i < nNumEvaluations; ++i)
	{

		// evaluate this spawn class
		SPAWN_ENTRY tSpawnEntries[MAX_SPAWNS_AT_ONCE];
		int nNumEntries = SpawnClassEvaluate(
			game, 
			LevelGetID( level ),
			nSpawnClass, 
			LevelGetMonsterExperienceLevel( level ), 
			tSpawnEntries,
			NULL);

		// go through all the spawn entries
		for (int j = 0; j < nNumEntries; ++j)
		{
			SPAWN_ENTRY *pEntry = &tSpawnEntries[ j ];
			int nMonsterClass = pEntry->nIndex;
			
			// setup spec
			int nCount = SpawnEntryEvaluateCount( game, pEntry );
			ASSERTX_CONTINUE( nMonsterClass >= 0 && nMonsterClass < MAX_MONSTER_CLASSES, "Too many monster classes for statistics" );
			tResults[ nMonsterClass ].nCount += nCount;
			nTotalCount += nCount;
									
		}
		
	}

	// display results


	// display header
	ConsoleString( L"" );
	ConsoleString(
		L"Spawn Class '%S' evaluated '%d' times:", 
		ExcelGetStringIndexByLine( game, DATATABLE_SPAWN_CLASS, nSpawnClass ),
		nNumEvaluations);
		
	// display stats
	for (int i = 0; i < MAX_MONSTER_CLASSES; ++i)
	{
		SPAWN_CLASS_STATS *pSCStats = &tResults[ i ];

		if (pSCStats->nCount > 0)
		{
			const UNIT_DATA *pUnitData = MonsterGetData( game, i );
			
			ConsoleString( 
				L" % 32S: % 6d times [% 6.2f%%]",
				pUnitData->szName,
				pSCStats->nCount,
				((float)pSCStats->nCount / (float)nTotalCount) * 100.0f);

		}
	}

	ConsoleString(  L" -------------------------------------------------" );
	ConsoleString(  
		L" % 32S: % 5d spawns - '%S'", 
		"Total", 
		nTotalCount,
		ExcelGetStringIndexByLine( game, DATATABLE_SPAWN_CLASS, nSpawnClass ));
		
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDisplayDevName)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_DEV_NAME_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Display Dev Name: %s", sGetOnOffString(bEnabled));	
	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDisplayPosition)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_POSITION_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Display Position: %s", sGetOnOffString(bEnabled));
	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDisplayVelocity)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_VELOCITY_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Display Velocity: %s", sGetOnOffString(bEnabled));	
	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDisplayID)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_ID_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Display Unit ID: %s", sGetOnOffString(bEnabled));	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDisplayRoom)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_ROOM_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Display Room: %s", sGetOnOffString(bEnabled));	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDisplayFaction)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_FACTION_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Display Faction: %s", sGetOnOffString(bEnabled));	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCollisionLadder)
{
	gbDisplayCollisionHeightLadder = !gbDisplayCollisionHeightLadder;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, gbDisplayCollisionHeightLadder ? L"Collision Ladder enabled, target any unit to see the ladder" :
		L"Collision ladder disabled");
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCollisionDebug)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_COLLISION_TRIANGLES_BIT);
	//e_SetRenderFlag( RENDER_FLAG_RENDER_MODELS, !bEnabled );
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Collision debug draw: %s", sGetOnOffString(bEnabled));
	return CR_OK;			
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCollisionMaterialDebug)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_COLLISION_MATERIALS_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Collision debug draw: %s", sGetOnOffString(bEnabled));
	return CR_OK;			
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCollisionShapes)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_COLLISION_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Collision shapes: %s", sGetOnOffString(bEnabled));
	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCollisionShapesPathing)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_PATHING_COLLISION_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Pathing Collision shapes: %s", sGetOnOffString(bEnabled));
	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMissileShapes)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_MISSILES_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Missile shapes: %s", sGetOnOffString(bEnabled));
	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMissileVectors)
{
	BOOL bEnabled = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_MISSILE_VECTORS_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Missile vectors: %s", sGetOnOffString(bEnabled));
	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdAimingShapes)
{
	BOOL bEnabled = UnitDebugToggleFlag( UNITDEBUG_FLAG_SHOW_AIMING_BIT );
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Aiming shapes: %s", sGetOnOffString(bEnabled));
	return CR_OK;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static int x_sCmdParseObjectClass(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * & strParams)
{
	ASSERT_RETINVALID(game && client && cmd && strParams);

	int objectClass;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseExcelLookupOrName(game, cmd, DATATABLE_OBJECTS, strParams, strToken, arrsize(strToken), objectClass))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"object", strToken);	
		return INVALID_LINK;
	}
	return objectClass;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdObject)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	int objectClass = x_sCmdParseObjectClass(game, client, cmd, strParams);
	if (objectClass == INVALID_LINK)
	{
		return CR_FAILED;
	}

	// parse qualifiers
	unsigned int count = 1;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrIsNumeric(strToken))
		{
			int val = PStrToInt(strToken);
			if (val > 0)
			{
				count = val;
			}
		}
	}

	// spawn the objects
	VECTOR position, direction;
	if (!sGetSpawnVectors(player, &position, &direction))
	{
		return CR_FAILED;
	}

	position += direction * 2.0f;

	for (unsigned int ii = 0; ii < count; ++ii)
	{
		position += direction;
		ROOM * room = RoomGetFromPosition(game, UnitGetRoom(player), &position);
		if (!room)
		{
			continue;
		}
		
		OBJECT_SPEC tSpec;
		tSpec.nClass = objectClass;
		tSpec.nLevel = RoomGetMonsterExperienceLevel(room);
		tSpec.pRoom = room;
		tSpec.vPosition = position;
		SETBIT(tSpec.dwFlags, OSF_IGNORE_SPAWN_LEVEL);		
		VECTOR reversedirection = -direction;
		tSpec.pvFaceDirection = &reversedirection;
		s_ObjectSpawn(game, tSpec);
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdClearScreen)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	ConsoleClear();
	return (DWORD)(CR_OK | CRFLAG_CLEAR_CONSOLE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdFullscreen)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int width = -1;
	int height = -1;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, width, L" ,x"))
	{
		// Fall through...
	}
	else if (!sConsoleParseIntegerParam(cmd, 1, strParams, height, L" ,x"))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing height specifier.");	
		return CR_FAILED;
	}

	CComPtr<OptionState> pState;
	V_DO_FAILED(e_OptionState_CloneActive(&pState))
	{
		return CR_FAILED;
	}
	pState->SuspendUpdate();
	if ((width > 0) && (height > 0))
	{
		pState->set_nFrameBufferWidth(width);
		pState->set_nFrameBufferHeight(height);
	}
	pState->set_bWindowed(!pState->get_bWindowed());
	pState->ResumeUpdate();
	V(e_OptionState_CommitToActive(pState));

//	InputResetMousePosition();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdWindowInfo)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	HWND hWnd = AppCommonGetHWnd();
	WCHAR str[CONSOLE_MAX_STR];
	RECT Rect;
	::GetWindowRect(hWnd, &Rect);
	int nWidth = Rect.right - Rect.left;
	int nHeight = Rect.bottom - Rect.top;
	PStrPrintf(str, _countof(str), L"HWND=%p at %d, %d, %d, %d (%d x %d)", hWnd, Rect.left, Rect.top, Rect.right, Rect.bottom, nWidth, nHeight);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, str);
	::GetClientRect(hWnd, &Rect);
	::MapWindowPoints(hWnd, 0, static_cast<POINT *>(static_cast<void *>(&Rect)), 2);	// client to screen
	nWidth = Rect.right - Rect.left;
	nHeight = Rect.bottom - Rect.top;
	PStrPrintf(str, _countof(str), L"  Client at %d, %d, %d, %d (%d x %d)", Rect.left, Rect.top, Rect.right, Rect.bottom, nWidth, nHeight);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, str);
	DWORD nStyle = ::GetWindowLong(hWnd, GWL_STYLE);
	DWORD nExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
	PStrPrintf(str, _countof(str), L"  Style=0x%08x, ExStyle=0x%08x", nStyle, nExStyle);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, str);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdSetWindowSize)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int width, height;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, width, L" ,x"))
	{
		WCHAR strMode[CONSOLE_MAX_STR];
		PStrCvt(strMode, e_GetDisplayModeString(), CONSOLE_MAX_STR);
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, strMode);
		return CR_OK;
	}
	if (!sConsoleParseIntegerParam(cmd, 1, strParams, height, L" ,x"))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing height specifier.");	
		return CR_FAILED;
	}

	CComPtr<OptionState> pState;
	V_DO_FAILED(e_OptionState_CloneActive(&pState))
	{
		return CR_FAILED;
	}
	pState->SuspendUpdate();
	pState->set_nFrameBufferWidth(width);
	pState->set_nFrameBufferHeight(height);
	pState->ResumeUpdate();
	V(e_OptionState_CommitToActive(pState));


	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdWireFrame)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	e_ToggleWireFrame();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && (ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))
CONSOLE_CMD_FUNC(c_sCmdCollisionDraw)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	e_ToggleCollisionDraw();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdHullDraw)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	e_ToggleHullDraw();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdBatches)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_BATCHES, -1);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDPVSStats)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_DPVS, -1);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDPVSFlags)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int nFlags;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nFlags))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing flag");
		return CR_FAILED;
	}

	nFlags = (int)e_DPVS_DebugSetDrawFlags( (DWORD)nFlags );
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"flags now: 0x%08X", nFlags);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPerformance)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int window = 0;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L" ,") && strToken[0] != 0)
	{
		if (PStrICmp(strToken, L"window", 6) == 0)
		{
			if (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L" ,"))
			{
				window = PStrToInt(strToken);
			}
		}
		else
		{
			char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
			PStrCvt(chToken, strToken, arrsize(chToken));
			unsigned int len = PStrLen(chToken, 256);
			for (unsigned int ii = 0; ii < PERF_LAST; ++ii)
			{
				if (PStrICmp(chToken, gPerfTable[ii].label, len) == 0)
				{
					gPerfTable[ii].expand = !gPerfTable[ii].expand;
					return CR_OK;
				}
			}
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unrecognized perf label [%s]", strToken);
			return CR_FAILED;
		}
	}

	if (window > 0)
	{
		gnPerfWindow = window;
	}
	PerfZeroAll();
	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_PERFORMANCE, (window > 0 ? 1 : -1));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdHitCount)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	if (strParams[0] != 0)
	{
		char chParam[CONSOLE_MAX_STR];
		PStrCvt(chParam, strParams, arrsize(chParam));

		unsigned int len = PStrLen(chParam, 256);
		for (unsigned int ii = 0; ii < HITCOUNT_LAST; ii++)
		{
			if (PStrICmp(chParam, gHitCountTable[ii].label, len) == 0)
			{
				gHitCountTable[ii].expand = !gHitCountTable[ii].expand;
				return CR_OK;
			}
		}
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unrecognized hit count label [%s]", strParams);
		return CR_FAILED;
	}

	HitCountZeroAll();
	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_HITCOUNT, -1);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMemory)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_MEMORY, -1);
#ifdef HAVOK_ENABLED
	if(AppIsHellgate())
	{
		c_PrintHavokMemoryStats();
	}
#endif
	MemoryDumpCRTStats();

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
extern int g_MemorySystemInfoCmd;
extern int g_MemorySystemInfoSetAverageIndex;
extern int g_MemorySystemInfoMode;
CONSOLE_CMD_FUNC(c_sCmdMemoryPoolInfo)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	
	if(PStrLen(strParams) == 0)
	{
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_MEMORY_POOL_INFO, -1);
	}
	else
	{
		WCHAR token[64];
		int bufLen;
		strParams = PStrTok(token, strParams, L" ", arrsize(token), bufLen);
		
		if(PStrCmp(token, L"set") == 0)
		{
			g_MemorySystemInfoSetAverageIndex = -2;
			if(PStrLen(strParams))
			{
				strParams = PStrTok(token, strParams, L" ", arrsize(token), bufLen);
				g_MemorySystemInfoSetAverageIndex = PStrToInt(token);
			}
		}
		else if(PStrCmp(token, L"growing") == 0)
		{
			g_MemorySystemInfoMode = 0;
		}		
		else if(PStrCmp(token, L"recent") == 0)
		{
			g_MemorySystemInfoMode = 1;
		}
		else
		{
			g_MemorySystemInfoCmd = PStrToInt(token);		
		}
	}

	return CR_OK;
}
#endif



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMemorySnapShot)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	MemoryTraceAllFLIDX();
	MemoryDumpCRTStats();
#ifdef HAVOK_ENABLED
	if( AppIsHellgate() )
	{
		c_PrintHavokMemoryStats();
	}
#endif
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdEventsDebug)
{
	ASSERT_RETVAL(game, CR_FAILED);

	if (PStrICmp(strParams[0], L'c'))
	{
		if (IS_CLIENT(game))
		{
			UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_EVENTS_CLT, -1);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Client Game Events Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_EVENTS_CLT)));
		}
	}
	else if (PStrICmp(strParams[0], L'u'))
	{
		if (IS_CLIENT(game))
		{
			UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_EVENTS_UI, -1);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Client UI Events Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_EVENTS_UI)));
		}
	}
	else if ((PStrICmp(strParams[0], L's') || strParams[0] == 0))
	{
		if (IS_SERVER(game))
		{
			UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_EVENTS_SRV, -1);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Server Game Events Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_EVENTS_SRV)));
		}
	}
	else
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unrecognized parameter: %s", strParams);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdRevis)
{
	if(AppIsHellgate())
	{
		ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

		UNIT * player = GameGetControlUnit(game);
		ASSERT_RETVAL(player, CR_FAILED);

		ROOM * room = UnitGetRoom(player);
		ASSERT_RETVAL(room, CR_FAILED);

		LEVEL * level = RoomGetLevel(room);
		ASSERT_RETVAL(level, CR_FAILED);

		DRLGCalculateVisibleRooms(game, level, room);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdAppearance)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_APPEARANCE, -1);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSync)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_SYNCH, -1);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdLightAdd)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = GameGetControlUnit(game);
	ASSERT_RETVAL(player, CR_FAILED);

	VECTOR position = c_UnitGetPosition(player);
	VECTOR direction = UnitGetFaceDirection(player, TRUE);
	position = position + direction * 3.0f;

	int nLightDefId = DefinitionGetIdByName(DEFINITION_GROUP_LIGHT, "Console");
	e_LightNew(nLightDefId, e_LightPriorityTypeGetValue( LIGHT_PRIORITY_TYPE_PLAYER ), position);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdLights)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_ToggleDrawAllLights();	

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMaxDetail)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int maxDetail;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, maxDetail))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing max detail level");
		return CR_FAILED;
	}

	V( e_SetMaxDetail(maxDetail) );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdWardrobeTest)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	c_WardrobeTest(TRUE);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
static BOOL sPlayerBodyTest(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	static const int DELAY_FOR_PLAYER_BODY_TEST = 10;

	GAMECLIENT * client = UnitGetClient(unit);

	int nModel = (int)event_data.m_Data1;
	int nPlayerClass = (int)event_data.m_Data2;
	WARDROBE_BODY_SECTION eBodySection = (WARDROBE_BODY_SECTION)event_data.m_Data3;
	BOOL bNeedToWait = (BOOL)event_data.m_Data4;

	// see if the wardrobe system has caught up with us yet, and make sure there is a delay to visually inspect things
	if ( nModel != INVALID_ID )
	{
		int nWardrobe = c_AppearanceGetWardrobe( nModel );
		BOOL bUpdated = c_WardrobeIsUpdated( nWardrobe );
		if ( bNeedToWait || ! bUpdated )
		{
			EVENT_DATA tEventData( nModel, nPlayerClass, eBodySection, !bUpdated );
			UnitRegisterEventTimed( unit, sPlayerBodyTest, &tEventData, bUpdated ? DELAY_FOR_PLAYER_BODY_TEST : 1 );
			return TRUE;
		}
	}

	// are we ready to make another model
	BOOL bNeedANewModel = (nModel == INVALID_ID);
	if ( eBodySection >= NUM_WARDROBE_BODY_SECTIONS )
	{
		nPlayerClass ++;
		if ((unsigned int)nPlayerClass >= ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_PLAYERS))
		{
			c_AppearanceDestroy( nModel );
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Player Body Test Complete");
			return TRUE; // we are done
		}
		bNeedANewModel = TRUE;
	}
	
	if ( bNeedANewModel )
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Player Body Test - Creating New Model");
		c_AppearanceDestroy( nModel );

		WARDROBE_INIT tWardrobeInit = c_WardrobeInitForPlayerClass( game, nPlayerClass, FALSE );
		int nWardrobe = WardrobeInit( game, tWardrobeInit );
		// initialize with the first options available
		for ( int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; i++ )
		{
			WARDROBE_BODY_SECTION eBodySection = (WARDROBE_BODY_SECTION) i;
			int pnOptionsForBodySection[ MAX_APPEARANCE_GROUPS_PER_BODY_SECTION ];
			int nCount = c_WardrobeGetAppearanceGroupOptions( nWardrobe, eBodySection, pnOptionsForBodySection, MAX_APPEARANCE_GROUPS_PER_BODY_SECTION );
			if ( nCount <= 0 )
				continue;
			c_WardrobeSetAppearanceGroup( nWardrobe, eBodySection, pnOptionsForBodySection[ 0 ] );
		}

		const UNIT_DATA * pUnitData = PlayerGetData( game, nPlayerClass );
		WARDROBE_CLIENT* pClient = c_WardrobeGetClient( nWardrobe );
		ASSERT_RETFALSE( pClient );
		nModel = c_AppearanceNew( pClient->nAppearanceDefId, APPEARANCE_NEW_DONT_FREE_WARDROBE, NULL, pUnitData, nWardrobe );

		// place model in front of player
		UNIT * player = unit;

		VECTOR vPosition = UnitGetPosition( player );
		VECTOR vOffset = UnitGetFaceDirection( player, TRUE );
		vOffset *= 3.0f;
		vPosition += vOffset;

		ROOM * pRoom = RoomGetFromPosition( game, UnitGetLevel( player ), &vPosition );

		MATRIX mWorld;
		MatrixFromVectors( mWorld, vPosition, VECTOR( 0, 0, 1), UnitGetFaceDirection( player, TRUE ) );

		c_ModelSetLocation( nModel, &mWorld, vPosition, pRoom ? RoomGetId( pRoom ) : UnitGetRoomId( player ) );

		eBodySection = (WARDROBE_BODY_SECTION) 0;

	} else {
		// change the next body section
		int nWardrobe = c_AppearanceGetWardrobe( nModel );
		ASSERT_RETTRUE( nWardrobe != INVALID_ID );
		while ( eBodySection < NUM_WARDROBE_BODY_SECTIONS )
		{
			BODY_SECTION_INCREMENT_RETURN eRetVal = c_WardrobeIncrementBodySection( nWardrobe, eBodySection, TRUE );
			if ( eRetVal == BODY_SECTION_INCREMENT_OK )
				break;

			if ( eRetVal == BODY_SECTION_INCREMENT_LOOPED || // we have tried them all
				 eRetVal == BODY_SECTION_INCREMENT_NO_OPTIONS ) // we have no options for this part (no facial hair on female?)
				eBodySection = ( WARDROBE_BODY_SECTION )( eBodySection + 1 );

			if ( eRetVal == BODY_SECTION_INCREMENT_ERROR )
				break;
		}

		if ( eBodySection < NUM_WARDROBE_BODY_SECTIONS )
		{
			int nCurrentAppearanceGroup = c_WardrobeGetAppearanceGroup( nWardrobe, eBodySection );
			if ( nCurrentAppearanceGroup != INVALID_ID )
			{
				WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroupData = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, nCurrentAppearanceGroup );
				SVR_VERSION_ONLY(REF(pAppearanceGroupData));

				WCHAR strAppearance[MAX_PATH];
				PStrCvt(strAppearance, pAppearanceGroupData->pszName, arrsize(strAppearance));
				ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Player Body Test - switching to %s", strAppearance);
			} 
			else 
			{
				ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Player Body Test - switching to NULL");
			}
		} 
	}

	EVENT_DATA tEventData( nModel, nPlayerClass, eBodySection );
	UnitRegisterEventTimed( unit, sPlayerBodyTest, &tEventData, 1 );

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPlayerBodyTest)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	UnitRegisterEventTimed(player, sPlayerBodyTest, &EVENT_DATA(INVALID_ID, 0, 0), 1);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdFogColor)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	int r, g, b;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, r, NULL, TRUE))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing red value");
		return CR_FAILED;
	}
	if (r < 0 || r > 255)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"red value out of bounds");
		return CR_FAILED;
	}
	if (!sConsoleParseIntegerParam(cmd, 1, strParams, g, NULL, TRUE))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing green value");
		return CR_FAILED;
	}
	if (g < 0 || g > 255)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"green value out of bounds");
		return CR_FAILED;
	}
	if (!sConsoleParseIntegerParam(cmd, 2, strParams, b, NULL, TRUE))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing blue value");
		return CR_FAILED;
	}
	if (b < 0 || b > 255)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"blue value out of bounds");
		return CR_FAILED;
	}

	e_SetFogColor(RGB(r, g, b));
	return CR_OK;
	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMusic)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_MUSIC, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Music Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_MUSIC)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdReverbDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_REVERB, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Reverb Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_REVERB)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundBusDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_SOUNDBUSES, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Sound Bus Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_SOUNDBUSES)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundMixStateDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_SOUNDMIXSTATES, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Sound Mix State Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_SOUNDMIXSTATES)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && (ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))
CONSOLE_CMD_FUNC(c_sCmdMusicScriptDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_MUSICSCRIPT, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Music Script Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_MUSICSCRIPT)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdRoomDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_ROOMDEBUG, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Room Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_ROOMDEBUG)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdRoomList)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	LEVEL * level = UnitGetLevel(player);
	ASSERT_RETVAL(level, CR_FAILED);
	
	WCHAR strLevelName[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strLevelName, LevelGetDevName(level), arrsize(strLevelName));
	ConsoleStringAndTrace(game, client, CONSOLE_SYSTEM_COLOR, L"Room list for level [%s]:", strLevelName);

	for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
		const ROOM_INDEX * idxRoom = RoomGetRoomIndex(game, room);
		WCHAR strRoomName[MAX_CHEAT_LEN];
		PStrCvt(strRoomName, idxRoom->pszFile, arrsize(strRoomName));
		
		ConsoleStringAndTrace(game, client, CONSOLE_SYSTEM_COLOR, L"    room id: %d  [%s]", room->idRoom, strRoomName);
	}
		
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdModelDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	int idModel;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idModel, NULL, TRUE) || idModel == e_GetRenderFlag(RENDER_FLAG_ISOLATE_MODEL))
	{
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_MODELDEBUG, 0);
		e_SetRenderFlag(RENDER_FLAG_ISOLATE_MODEL, INVALID_ID);
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Model Debug: OFF");
		return CR_OK;
	}

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_MODELDEBUG, 1);
	e_SetRenderFlag(RENDER_FLAG_ISOLATE_MODEL, idModel);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Model Debug ID: %d", idModel);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdVirtualSoundDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_VIRTUALSOUND, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Virtual Sound Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_VIRTUALSOUND)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdStoppedSoundDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_STOPPEDSOUND, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Stopped Sound Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_STOPPEDSOUND)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundToggle)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	gApp.m_bNoSound = !gApp.m_bNoSound;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Sound: %s", sGetOnOffString(gApp.m_bNoSound));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundToggleHack)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	extern BOOL bDo3rdPersonSoundHack;
	bDo3rdPersonSoundHack = !bDo3rdPersonSoundHack;

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Sound Hack: %s", sGetOnOffString(bDo3rdPersonSoundHack));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && (ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))
CONSOLE_CMD_FUNC(c_sCmdSoundDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_SOUND, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Sound Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_SOUND)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundMemoryDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_SOUNDMEMORY, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Sound Memory Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_SOUNDMEMORY)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundMemoryDump)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_SoundMemoryTrackerDumpStats();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdReverbReset)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_SoundResetReverb();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Reset reverb to Off");
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundTrace)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_SoundToggleLoggingEnabled();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"toggled sound logging (is it off, on, who knows?)");
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && (ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))
CONSOLE_CMD_FUNC(c_sCmdBGSoundDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_BGSOUND, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Background Sound Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_BGSOUND)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUIInputDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_UI, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"UI Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_UI)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUIGfxDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_UIGFX, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"UI Gfx Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_UIGFX)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUIToggleDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UITestingToggle();

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"UI Debug Toggle: %s", sGetOnOffString(UIDebugTestingToggle()));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdRoomDrawAll)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bDrawAllRooms = c_RoomToggleDrawAllRooms();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Draw All Rooms: %s", sGetOnOffString(bDrawAllRooms));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdRoomDrawConnected)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bDrawOnlyConnected = c_RoomToggleDrawOnlyConnected();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Draw Only Connected Rooms: %s", sGetOnOffString(bDrawOnlyConnected));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdRoomDrawNearby)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bDrawOnlyNearby = c_RoomToggleDrawOnlyNearby();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Draw Only Nearby Rooms: %s", sGetOnOffString(bDrawOnlyNearby));
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdIsClearFlat)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	// get control unit
	UNIT *pPlayer = ClientGetControlUnit( client );
	
	// get node at control unit
	ROOM *pRoom = UnitGetRoom( pPlayer );
	VECTOR vPosition = UnitGetPosition( pPlayer );
	ROOM *pRoomFound = NULL;
	ROOM_PATH_NODE *pNode = RoomGetNearestPathNode( game, pRoom, vPosition, &pRoomFound );
	BOOL bClear = RoomIsClearFlatArea( pRoomFound, pNode, 3.0f, 3.0f, DEFAULT_FLAT_Z_TOLERANCE );
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Clear Flat Area: %s", sGetYesNoString(bClear));
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT) && !ISVERSION(CLIENT_ONLY_VERSION)
CONSOLE_CMD_FUNC(x_sCmdRoomReset)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT *pPlayer = ClientGetControlUnit( client );
	ROOM *pRoom = UnitGetRoom( pPlayer );
	BOOL bReset = s_RoomReset( game, pRoom );
	ConsoleString( game, client, CONSOLE_SYSTEM_COLOR, L"Room Reset: %s", sGetYesNoString( bReset ) );
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugRenderFlag)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bDebugRender = e_ToggleDebugRenderFlag();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Debug Render: %s", sGetOnOffString(bDebugRender));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdShadowFlag)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

#if 0	// CHB !!! - needs to go through e_FeatureLine
	BOOL bShadow = e_ToggleShadowFlag();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Shadows: %s", sGetOnOffString(bShadow));
#endif
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugTextureFlag)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bDebugTexture = e_ToggleDebugTextureFlag();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Debug Texture: %s", sGetOnOffString(bDebugTexture));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdClipDraw)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bClipDraw = e_ToggleClipDraw();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Draw Clip Planes: %s", sGetOnOffString(bClipDraw));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugDraw)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bDebugDraw = e_ToggleDebugDraw();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Draw Debug Mesh: %s", sGetOnOffString(bDebugDraw));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdFXList)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	const int cnBufLen = 4096;
	char * szStr = (char*)MALLOC( NULL, cnBufLen * sizeof(char) );
	ASSERT_RETVAL( szStr, CR_FAILED );

	V_DO_FAILED( e_GetEffectListString( szStr, cnBufLen ) )
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"error dumping list!");
		FREE( NULL, szStr );
		return CR_FAILED;
	}

	char szLine[] = "-------------------------------------------------------------------------";
	trace( "\n%s\n\n%s\n\n%s\n\n", szLine, szStr, szLine );

	FREE( NULL, szStr );
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdFXInfo)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	int nEffectID;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nEffectID))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing effect id");
		return CR_FAILED;
	}

	//const int cnBufLen = 1000000;
	//char * szStr = (char*)MALLOC( NULL, cnBufLen * sizeof(char) );
	//ASSERT_RETVAL( szStr, CR_FAILED );

	char szLine[] = "-------------------------------------------------------------------------";
	trace( "\n%s\n\n", szLine );

	V_DO_FAILED( e_GetFullEffectInfoString( nEffectID ) )
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"error dumping info!");
		//FREE( NULL, szStr );
		return CR_FAILED;
	}

	trace( "\n%s\n\n", szLine );

	//FREE( NULL, szStr );
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdFarClip)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	float dist;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, dist))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing far clip value");
		return CR_FAILED;
	}

	e_SetFarClipPlane(dist, TRUE);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Setting Far Clip Plane to: %4.4f", dist);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdFogDistances)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	float fNear;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, fNear))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing near distance");
		return CR_FAILED;
	}

	float fFar;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, fFar))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing far distance");
		return CR_FAILED;
	}

	e_SetFogDistances( fNear, fFar, TRUE );
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Setting Fog Distances to: %1.3f (near) and %1.3f (far)", fNear, fFar);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdFogDensity)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	float density;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, density))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing fog density");
		return CR_FAILED;
	}

	e_SetFogMaxDensity(density);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Setting Fog Density to: %1.3f", density);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCGSet)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int command;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), command))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"render flag", strToken);
		return CR_FAILED;
	}

	int parameter;
	if (!sConsoleParseIntegerParam(cmd, 1, strParams, parameter))
	{
		int val = e_ToggleRenderFlag((RENDER_FLAG)command);
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"%s is %s.", strToken, sGetEnabledDisabledString(val));
		return CR_OK;
	}

	e_SetRenderFlag((RENDER_FLAG)command, parameter);
	int current = e_GetRenderFlag((RENDER_FLAG)command);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"%s is %d.", strToken, current);
	
	e_RenderFlagPostCommand(command);	// optional post-console cmd steps

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
static void c_sConsoleDumpCGFlags(
	GAME * game,
	GAMECLIENT * client)
{
	static const int columns = 3;

	unsigned int lines = NUM_RENDER_FLAGS / columns + ((NUM_RENDER_FLAGS % columns) != 0);
	for (unsigned int ii = 0; ii < lines; ++ii)
	{
		WCHAR strLine[MAX_CHEAT_LEN];
		strLine[0] = 0;
		for (unsigned int ff = 0; ff < columns; ++ff)
		{
			RENDER_FLAG flag = (RENDER_FLAG)(ii + ff * lines);
			if (flag >= NUM_RENDER_FLAGS)
			{
				break;
			}
			const char * chFlag = (const char *)ExcelGetData(game, DATATABLE_RENDER_FLAGS, flag);
			ASSERT_RETURN(chFlag);
			WCHAR strFlag[MAX_CHEAT_LEN];
			PStrCvt(strFlag, chFlag, arrsize(strFlag));

			int val = e_GetRenderFlag(flag);
			WCHAR strPair[MAX_CHEAT_LEN];
			PStrPrintf(strPair, arrsize(strPair), L"%-20s: %-3i     ", strFlag, val);
			PStrCat(strLine, strPair, arrsize(strLine));
		}

		ConsoleString1(game, client, CONSOLE_SYSTEM_COLOR, strLine);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCGShow)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int command;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), command))
	{
		c_sConsoleDumpCGFlags(game, client);
		return CR_OK;
	}

	int val = e_GetRenderFlag((RENDER_FLAG)command);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"%s is %i.", strToken, val);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)

CONSOLE_CMD_FUNC(s_sCmdWarpPrevFloor)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	LEVEL *pLevel = UnitGetLevel( player );	
	ASSERT_RETVAL(pLevel, CR_FAILED);
	
	int nDepth = max( 0, LevelGetDepth( pLevel ) - 1 );
	nDepth = min( nDepth, pLevel->m_LevelAreaOverrides.nDungeonDepth - 1 );
	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( pLevel, LevelGetLevelAreaID( pLevel ), nDepth );
	if( tSpec.nLevelDefDest == INVALID_ID )
		return CR_FAILED;
	tSpec.nLevelAreaDest = LevelGetLevelAreaID( pLevel );
	tSpec.nLevelDepthDest = nDepth;
	tSpec.nPVPWorld = ( LevelGetPVPWorld( pLevel ) ? 1 : 0 );
	SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	
	// warp!
	s_WarpToLevelOrInstance( player, tSpec );

	return CR_OK;
	
}

CONSOLE_CMD_FUNC(s_sCmdWarpNextFloor)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	LEVEL *pLevel = UnitGetLevel( player );	
	ASSERT_RETVAL(pLevel, CR_FAILED);

	
	int nDepth = min( pLevel->m_LevelAreaOverrides.nDungeonDepth - 1, LevelGetDepth( pLevel ) + 1 );
	nDepth = max( nDepth, 0 );
	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( pLevel, LevelGetLevelAreaID( pLevel ), nDepth );
	if( tSpec.nLevelDefDest == INVALID_ID )
		return CR_FAILED;
	tSpec.nLevelAreaDest = LevelGetLevelAreaID( pLevel );
	tSpec.nLevelDepthDest = nDepth;
	tSpec.nPVPWorld = ( LevelGetPVPWorld( pLevel ) ? 1 : 0 );
	SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	
	// warp!
	s_WarpToLevelOrInstance( player, tSpec );

	return CR_OK;
	
}



CONSOLE_CMD_FUNC(s_sCmdWarpLastFloor)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	LEVEL *pLevel = UnitGetLevel( player );	
	ASSERT_RETVAL(pLevel, CR_FAILED);

		// setup warp spec
	int nDepth = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( LevelGetLevelAreaID( pLevel ) ) - 1;
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( pLevel, LevelGetLevelAreaID( pLevel ), nDepth );
	if( tSpec.nLevelDefDest == INVALID_ID )
		return CR_FAILED;
	tSpec.nLevelAreaDest = LevelGetLevelAreaID( pLevel );
	tSpec.nLevelDepthDest = nDepth;
	tSpec.nPVPWorld = ( LevelGetPVPWorld( pLevel ) ? 1 : 0 );
	SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	
	// warp!
	s_WarpToLevelOrInstance( player, tSpec );

	return CR_OK;
	
}
CONSOLE_CMD_FUNC(s_sCmdWarpToMarker)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	LEVEL *pLevel = UnitGetLevel( player );
	ASSERT_RETVAL(pLevel, CR_FAILED);
	VECTOR vCurrentPosition = UnitGetPosition( player );
	UNIT *pObject = PlayerGetRespawnMarker( player, UnitGetLevel( player ), &vCurrentPosition );
	//if( pObject ) 
	{
		UNIT *pWarpToObject( NULL );
		BOOL bNext( FALSE );
		for ( ROOM * pRoom = LevelGetFirstRoom(pLevel); pRoom; pRoom = LevelGetNextRoom(pRoom) )	
		{
			for ( UNIT * test = pRoom->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
			{
				if( UnitIsA( test, UNITTYPE_ANCHORSTONE_MARKER ) )			
				{
					if( bNext )
					{
						pWarpToObject = test;
						test = NULL;
						pRoom = NULL;
						break;
					}
					if( pWarpToObject == NULL &&
						test != pObject )
					{
						pWarpToObject = test;
					}
					if( test == pObject )
					{
						bNext = TRUE;
					}
				}
			}
			if( pRoom == NULL )
				break;
		}	
		if( !pWarpToObject )
		{
			pWarpToObject = pObject;
		}
		if( pWarpToObject )
		{

			UnitSetPosition( player, UnitGetPosition( pWarpToObject ) );
			PlayerSetRespawnLocation( KRESPAWN_TYPE_ANCHORSTONE, player, pWarpToObject, pLevel ); 
			DWORD dwLevelRecallFlags = 0;
			SETBIT( dwLevelRecallFlags, LRF_FORCE, TRUE );
			SETBIT( dwLevelRecallFlags, LRF_PRIMARY_ONLY, TRUE );
			s_LevelRecall( game, player, dwLevelRecallFlags );

			return CR_OK;
		}
	}

	return CR_FAILED;

}

CONSOLE_CMD_FUNC(s_sCmdWarpToLevelArea)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ROOM * room = UnitGetRoom(player);
	ASSERT_RETVAL(room, CR_FAILED);

	WCHAR strLevel[MAX_CONSOLE_PARAM_VALUE_LEN];
	int level;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strLevel, arrsize(strLevel), level))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"level area", strLevel);
		return CR_FAILED;
	}

	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( NULL, level, 0 );
	tSpec.nLevelAreaDest = level;
	tSpec.nLevelDepthDest = 0;
	tSpec.nPVPWorld = ( PlayerIsInPVPWorld( player ) ? 1 : 0 );
	SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	
	// warp!
	s_WarpToLevelOrInstance( player, tSpec );

	return CR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sCmdLevelGetLevelIdByDisplayName( 
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * & strParams)
{
	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_LEVEL);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{
		const LEVEL_DEFINITION * pLevelDef = (const LEVEL_DEFINITION *)ExcelGetData( EXCEL_CONTEXT(), DATATABLE_LEVEL, ii);
		if ( ! pLevelDef )
			continue;

		const WCHAR * strName = StringTableGetStringByIndex(pLevelDef->nLevelDisplayNameStringIndex);
		ASSERT_RETVAL(strName, INVALID_LINK);

		unsigned int lenName = PStrLen(strName);
		if (lenName <= 0)
			continue;

		if (PStrICmp(strName, strParams, lenName) == 0 && sIsValidParamEnd(strParams[lenName]))
		{
			strParams = sConsoleSkipDelimiter(strParams + lenName);

			return ii;
		}
	}

	return INVALID_ID;
}

CONSOLE_CMD_FUNC(s_sCmdLevel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ROOM * room = UnitGetRoom(player);
	ASSERT_RETVAL(room, CR_FAILED);

	WCHAR strLevel[MAX_CONSOLE_PARAM_VALUE_LEN];
	int level = INVALID_ID;
	if (PStrICmp(cmd->m_strCommand.GetStr(), L"levelr", MAX_CONSOLE_COMMAND_LEN) == 0)
	{
		level = s_sCmdLevelGetLevelIdByDisplayName(game, client, cmd, strParams);
	}
	else if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strLevel, arrsize(strLevel), level))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"level", strLevel);
		return CR_FAILED;
	}

	if ( level == INVALID_ID )
		return CR_FAILED;

	StateClearAllOfType(player, STATE_NO_TRIGGER_WARPS);

	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = level;
	SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	
	// warp!
	s_WarpToLevelOrInstance( player, tSpec );

	return CR_OK;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdLevelDump)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	ConsoleStringAndTrace( 
		game,
		client,
		CONSOLE_SYSTEM_COLOR,	
		L"Levels In Game ID '%d'\n", 
		GameGetId( game ));
	int nCount = 0;	
	for (LEVEL *pLevel = DungeonGetFirstLevel( game ); pLevel; pLevel = DungeonGetNextLevel( pLevel ))
	{
		ConsoleStringAndTrace( 
			game,
			client,
			CONSOLE_SYSTEM_COLOR,
			L"%03d) Pop=%d, Act=%d, Name=%S\n",
			++nCount,
			LevelTestFlag( pLevel, LEVEL_POPULATED_BIT ),
			LevelIsActive( pLevel ),
			LevelGetDevName( pLevel ));
	}
	ConsoleStringAndTrace( 
		game,
		client,
		CONSOLE_SYSTEM_COLOR,
		L"-----------------------\n");
	
	ConsoleStringAndTrace( 
		game,
		client,
		CONSOLE_SYSTEM_COLOR,
		L"'%d' Total Levels\n", 
		nCount);
		
	return CR_OK;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddDRLG(
	int nDRLG,
	int *pnDRLGBuffer,
	int &nBufferCount)
{
	if (nDRLG != INVALID_LINK)
	{
		for (int k = 0; k < nBufferCount; ++k)
		{
			if (pnDRLGBuffer[ k ] == nDRLG)
			{
				return;
			}
		}
		
		pnDRLGBuffer[ nBufferCount++ ] = nDRLG;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSubLevelAddDRLG( 
	int nSubLevelDef,
	int *pnDRLGBuffer,
	int &nBufferCount)
{
	if (nSubLevelDef != INVALID_LINK)
	{
		const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( nSubLevelDef );
		sAddDRLG( ptSubLevelDef->nDRLGDef, pnDRLGBuffer, nBufferCount );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdDRLGsPossibleVersion)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	const int MAX_DRLG = 1024;
	int nDRLGBuffer[ MAX_DRLG ];
	int nNumDLRG = 0;
	
	int nNumLevels = ExcelGetNumRows( NULL, DATATABLE_LEVEL );
	for (int i = 0; i < nNumLevels; ++i)
	{
		const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( i );
		if (ptLevelDef)
		{
			// go through all DRLG choices
			int nNumRows = ExcelGetNumRows(game, DATATABLE_LEVEL_DRLG_CHOICE);
			for(int j=0; j<nNumRows; j++)
			{
				const LEVEL_DRLG_CHOICE * pDRLGChoice = (const LEVEL_DRLG_CHOICE *)ExcelGetData(game, DATATABLE_LEVEL_DRLG_CHOICE, j);
				if(!pDRLGChoice)
					continue;

				if(pDRLGChoice->nLevel == i)
				{
					sAddDRLG( j, nDRLGBuffer, nNumDLRG );
				}
			}

			// sublevels
			for (int j = 0; j < MAX_HELLRIFT_SUBLEVEL_CHOICES; ++j)
			{
				sSubLevelAddDRLG( ptLevelDef->nSubLevelHellrift[j], nDRLGBuffer, nNumDLRG ); 
			}
			sSubLevelAddDRLG( ptLevelDef->nSubLevelDefault, nDRLGBuffer, nNumDLRG ); 
			for (int j = 0; j < MAX_SUBLEVELS; ++j)
			{
				sSubLevelAddDRLG( ptLevelDef->nSubLevel[ j ], nDRLGBuffer, nNumDLRG );
			}

		}

	}

	const int MAX_MESSAGE = 1024;
	char szMessage[ MAX_MESSAGE ];
	PStrCopy( szMessage, " \n", MAX_MESSAGE );
	ConsoleString( szMessage ); trace( szMessage );
	PStrPrintf( szMessage, MAX_MESSAGE, "The following %d DRLGS can appear in this version\n", nNumDLRG );
	ConsoleString( szMessage ); trace( szMessage );

	for (int i = 0; i < nNumDLRG; ++i)
	{
		const DRLG_DEFINITION *ptDRLGDef = DRLGDefinitionGet( nDRLGBuffer[ i ] );
		if ( ! ptDRLGDef )
			continue;
		const LEVEL_FILE_PATH *ptLevelFilePathDef = (const LEVEL_FILE_PATH *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_LEVEL_FILE_PATHS, ptDRLGDef->nPathIndex);
		PStrPrintf( szMessage, MAX_MESSAGE, "   %02d)  %s\t%s\n", i + 1, ptDRLGDef->pszName, ptLevelFilePathDef->pszPath);
		ConsoleString( szMessage ); trace( szMessage );		
	}
	
	return CR_OK;

}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdAIDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bAiOutput = AI_ToggleOutputToConsole();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"AI Debug: %s.", sGetOnOffString(bAiOutput));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSkillDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (PStrICmp(strParams[0], L'c'))
	{
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_SKILL_CLT, -1);
	}
	else if (PStrICmp(strParams[0], L'u'))
	{
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_SKILL_SRV, -1);
	}
	else
	{
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_SKILL_SRV, -1);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSkillStringNumbers)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_SkillStringsToggleDebug();

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSkillDescriptions)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = sGetControlUnit(game, client);
	ASSERT_RETVAL(player, CR_FAILED);
	unsigned int numSkills = ExcelGetNumRows(game, DATATABLE_SKILLS);
	WCHAR uszSkillDescription[ MAX_SKILL_TOOLTIP ];
	WCHAR uszSkillDescriptionStripped[ MAX_SKILL_TOOLTIP ];
	WCHAR uszSkillPerLevelEffect[ MAX_SKILL_TOOLTIP ];
	WCHAR uszClassString[ MAX_SKILL_TOOLTIP ];

	WCHAR filename[MAX_PATH];
	WCHAR fullname[MAX_PATH];
	UnitGetClassString(player, uszClassString, arrsize(uszClassString));
	PStrPrintf(filename, arrsize(filename), L"data\\skill_descriptions_%s.txt", uszClassString);
	FileGetFullFileName(fullname, filename, MAX_PATH);
	HANDLE file = FileOpen(fullname, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	if (file == INVALID_FILE)
		return CR_FAILED;

	for (unsigned int skill = 0; skill < numSkills; ++skill)
	{
		const SKILL_DATA * skillData = SkillGetData(game, skill);
		if (skillData &&
			SkillDataTestFlag(skillData, SKILL_FLAG_LEARNABLE) &&
			UnitIsA(player, skillData->nRequiredUnittype) &&
			!SkillDataTestFlag( skillData, SKILL_FLAG_DISABLED ) )
		{
			DWORD dwSkillLearnFlags = 0;
			SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_NAME );
			SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_DESCRIPTION );
			SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_USE_INSTRUCTIONS );

			if (!UIGetHoverTextSkill( skill, 0, player, dwSkillLearnFlags, uszSkillDescription, arrsize( uszSkillDescription ) ) )
				continue;

			dwSkillLearnFlags = 0;
			SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT );
			int maxSkillLevel = SkillGetMaxLevelForUI(player, skill);
			for (int skillLevel = 1; skillLevel <= maxSkillLevel; ++skillLevel)
			{
				if (UIGetHoverTextSkill( skill, skillLevel, player, dwSkillLearnFlags, uszSkillPerLevelEffect, arrsize( uszSkillPerLevelEffect ) ) )
				{
					PStrCat(uszSkillDescription, uszSkillPerLevelEffect, arrsize(uszSkillDescription));
				}
				else
				{
					continue;
				}

			}

			// strip out the color codes
			WCHAR *to = uszSkillDescriptionStripped;
			WCHAR *from = uszSkillDescription;
			WCHAR *charCode = NULL;
			*to = L'\0';
			do 
			{
				charCode = PStrChr(from, L'{');
				if (charCode)
				{
					*charCode = L'\0';
					++charCode;
				}
				PStrCat(to, from, arrsize(uszSkillDescription));
				if (charCode)
				{
					from = PStrChr(charCode, L'}');
					if (from)
						++from;
				}
			} while (charCode && from && *from != L'\0');

			// newlines
			// replace LF with CRLF (two steps, since replacement string contains originial string)
			PStrReplaceToken(uszSkillDescriptionStripped, arrsize(uszSkillDescriptionStripped), L"`\n", L"\r\a"); // magic ` allows white space token
			PStrReplaceToken(uszSkillDescriptionStripped, arrsize(uszSkillDescriptionStripped), L"`\a", L"\n"); // magic ` allows white space token
			PStrCat(uszSkillDescriptionStripped, L"\r\n\r\n\r\n", arrsize(uszSkillDescriptionStripped));

			//DWORD byteswritten = 
				FileWrite(
					file,
					(const void *) uszSkillDescriptionStripped,
					PStrLen(uszSkillDescriptionStripped, arrsize(uszSkillDescriptionStripped))*sizeof(WCHAR));
		}

	}

	FileClose(file);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdForceSync)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	gApp.m_bForceSynch = !gApp.m_bForceSynch;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Force Sync: %s", sGetOnOffString(gApp.m_bForceSynch));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdRagdollEnableAll)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

#ifdef HAVOK_ENABLED
	if(AppIsHellgate())
	{
		c_RagdollGameEnableAll(game);
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"All ragdolls enabled.");
	}
#endif
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdScript)
{
	char chFileName[MAX_PATH];
	char chFileNameWithPath[MAX_PATH];

	PStrCvt(chFileName, strParams, arrsize(chFileName));

	PStrPrintf(chFileNameWithPath, arrsize(chFileNameWithPath), "%s%s", FILE_PATH_DATA, chFileName);

	if (!FileExists(chFileNameWithPath))
	{
		PStrPrintf(chFileNameWithPath, arrsize(chFileNameWithPath), "%s%s.txt", FILE_PATH_DATA, chFileName);
		if (!FileExists(chFileNameWithPath))
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Script file [%s] not found.", strParams);
			return CR_FAILED;
		}
	}

	DWORD size = 0;
	char * script = (char *)FileLoad(MEMORY_FUNC_FILELINE(NULL, chFileNameWithPath, &size));
	// GS - 2-8-2007
	// This is immediately freeing the data for some reason.
	//AUTOFREE(NULL, script);
	if (size == 0 || !script)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Error with script file: %s", strParams);
		return CR_FAILED;
	}

	char * end = PStrTextFileFixup(script, size, 0);
	char * cur = script;

	while (cur < end)
	{
		WCHAR strLine[CONSOLE_MAX_STR];
		if (*cur != '/')
		{
			PStrCvt(strLine + 1, cur, arrsize(strLine) - 1);
		}
		else
		{
			PStrCvt(strLine, cur, arrsize(strLine));
		}
		ConsoleParseCommand(game, client, strLine);
		cur += PStrLen(cur) + 1;
	}
	FREE(NULL, script);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sUIHide)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UIToggleHideUI();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPakTrace)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Tracing pakfile.");
	PakFileTrace();
//	FileSystemTrace(AppCommonGetFileSystemRef());
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdFileTypeLoadedDump)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	FileTypeLoadedDump();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"File types dumped.");
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdProcessWhenInactive)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	BOOL bProcessWhenInactive = AppToggleFlag(AF_PROCESS_WHEN_INACTIVE_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Process when inactive: %s", sGetOnOffString(bProcessWhenInactive));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && (ISVERSION(DEVELOPMENT)||ISVERSION(RELEASE_CHEATS_ENABLED))
CONSOLE_CMD_FUNC(c_sCmdPathNode)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bShowPathNodes = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_PATH_NODE_BIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Path node display: %s", sGetOnOffString(bShowPathNodes));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && (ISVERSION(DEVELOPMENT)||ISVERSION(RELEASE_CHEATS_ENABLED))
CONSOLE_CMD_FUNC(c_sCmdPathNodeConnection)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bShowPathConnections = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_PATH_NODE_CONNECTION);
	if (bShowPathConnections == TRUE)
	{
		UnitDebugSetFlag(UNITDEBUG_FLAG_SHOW_PATH_NODE_BIT, TRUE);
	}
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Show path node connections: %s", sGetOnOffString(bShowPathConnections));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPathNodeOnTop)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bShowPathNodesOnTop = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_PATH_NODE_ON_TOP_BIT);
	if (bShowPathNodesOnTop == TRUE)
	{
		UnitDebugSetFlag(UNITDEBUG_FLAG_SHOW_PATH_NODE_BIT, TRUE);
	}	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Path node on top: %s", sGetOnOffString(bShowPathNodesOnTop));
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPathNodeClient)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bShowPathNodesClient = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_PATH_NODE_CLIENT_BIT);

	UnitDebugSetFlag(UNITDEBUG_FLAG_SHOW_PATH_NODE_BIT, TRUE);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Client path node display: %s", sGetOnOffString(bShowPathNodesClient));
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPathNodeRadii)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bShowPathNodesRadius = UnitDebugToggleFlag(UNITDEBUG_FLAG_SHOW_PATH_NODE_RADIUS_BIT);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Path node radius display: %s", sGetOnOffString(bShowPathNodesRadius));
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdMoneyChance)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	float flChance;
	if (sConsoleParseFloatParam(cmd, 0, strParams, flChance))
	{
		gflMoneyChanceOverride = flChance;
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Money chance: %f", gflMoneyChanceOverride);
	return CR_OK;

}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGlobalIndexValidate)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Validating Global Indexes.");
	GlobalIndexValidate();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdAIFreeze)
{
	ASSERT_RETVAL(game, CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_AI_FREEZE);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"AiFreeze: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_AI_FREEZE)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdEditCompatDB)
{
	ASSERT_RETVAL(game, CR_FAILED);
	e_CompatDB_DoEdit();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdAINoTarget)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_AI_NOTARGET);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"AiNoTarget: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_AI_NOTARGET)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdBreath)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GameSetDebugFlag(game, DEBUGFLAG_INFINITE_BREATH, !GameGetDebugFlag(game, DEBUGFLAG_INFINITE_BREATH));
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Infinite breath: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_INFINITE_BREATH)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPower)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GameSetDebugFlag(game, DEBUGFLAG_INFINITE_POWER, !GameGetDebugFlag(game, DEBUGFLAG_INFINITE_POWER));
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Infinite power: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_INFINITE_POWER)));

	UNIT * pControlUnit = ClientGetControlUnit(client);
	if(pControlUnit)
	{
		UnitSetStat(pControlUnit, STATS_POWER_CUR, UnitGetStat(pControlUnit, STATS_POWER_MAX));
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)

CONSOLE_CMD_FUNC(s_sCmdGod)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
		
	GameToggleDebugFlag(game, DEBUGFLAG_GOD);
	BOOL bGod = GameGetDebugFlag(game, DEBUGFLAG_GOD);

	if (bGod)
	{
		GameSetDebugFlag(game, DEBUGFLAG_PLAYER_INVULNERABLE, TRUE);	
		s_UnitRestoreVitals(player, TRUE);
		GameSetDebugFlag(game, DEBUGFLAG_INFINITE_BREATH, TRUE);
		GameSetDebugFlag(game, DEBUGFLAG_INFINITE_POWER, TRUE);
	}
	else
	{
		GameSetDebugFlag(game, DEBUGFLAG_PLAYER_INVULNERABLE, FALSE);	
		GameSetDebugFlag(game, DEBUGFLAG_INFINITE_BREATH, FALSE);
		GameSetDebugFlag(game, DEBUGFLAG_INFINITE_POWER, FALSE);
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"God Mode: %s", sGetOnOffString(bGod));
		
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdInvulnerable)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_PLAYER_INVULNERABLE);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Player Invulnerability: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_PLAYER_INVULNERABLE)));
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdAutomapMonsters)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_AUTOMAP_MONSTERS);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Automap monsters: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_AUTOMAP_MONSTERS)));
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdInvulnerableAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_ALL_INVULNERABLE);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Everything Invulnerable: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_ALL_INVULNERABLE)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdInvulnerableUnit)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * unit = sConsoleParseUnitId(game, client, cmd, 0, strParams);
	if (!unit)
	{
		return CR_FAILED;
	}

	BOOL bInvulnerable = FALSE;
	if (UnitHasState(game, unit, STATE_INVULNERABLE_ALL))
	{
		s_StateClear(unit, UnitGetId(unit), STATE_INVULNERABLE_ALL, 0);
	}
	else
	{
		s_StateSet(unit, unit, STATE_INVULNERABLE_ALL, 0);
		bInvulnerable = TRUE;
	}
	WCHAR strMonsterName[MAX_CHEAT_LEN];
	UnitGetName(unit, strMonsterName, arrsize(strMonsterName), 0);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Unit %d [%s] Invulnerability: %s.", UnitGetId(unit), strMonsterName, sGetOnOffString(bInvulnerable));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdAlwaysGetHit)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_ALWAYS_GETHIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Always Get Hit: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_ALWAYS_GETHIT)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdAlwaysSoftHit)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_ALWAYS_SOFTHIT);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Always Soft Hit: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_ALWAYS_SOFTHIT)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdAlwaysKill)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_ALWAYS_KILL);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Always Kill: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_ALWAYS_KILL)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdCanEquipAllItems)
{
	ASSERT_RETVAL(game, CR_FAILED);

	BOOL bParam;
	if (!sConsoleParseBooleanParam(cmd, 0, strParams, bParam))
	{
		bParam = !GameGetDebugFlag(game, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS);
	}
	GameSetDebugFlag(game, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS, bParam);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"[%s] Can Equip All Items: %s", sGetClientServerString(game), sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS)));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdKillAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	s_UnitKillAll(game, UnitGetLevel(player), FALSE);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Kill everything.");
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSuicide)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	UnitDie(player, NULL);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdKillUnit)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * unit = sConsoleParseUnitId(game, client, cmd, 0, strParams);
	if (!unit)
	{
		return CR_FAILED;
	}

	UnitDie(unit, NULL);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdFreeUnit)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * unit = sConsoleParseUnitId(game, client, cmd, 0, strParams);
	if (!unit)
	{
		return CR_FAILED;
	}

	UnitDie(unit, NULL);
	UnitFree(unit, UFF_SEND_TO_CLIENTS);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdDesynchUnit)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	float distance;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, distance))
	{
		distance = 0.5f;
	}

	UNIT * pUnit = GameGetDebugUnit(game);
	if(!pUnit)
		return CR_OK;

	VECTOR vPosition = UnitGetPosition(pUnit);
	MATRIX tMatrix;
	MatrixRotationZ(tMatrix, RandGetFloat(game, 0, 2.0f*PI));;
	VECTOR vDesynch(0, distance, 0);
	MatrixMultiply(&vDesynch, &vDesynch, &tMatrix);
	vPosition += vDesynch;

	UnitSetPosition(pUnit, vPosition);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSkill)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strSkill[MAX_CONSOLE_PARAM_VALUE_LEN];
	int skill;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strSkill, arrsize(strSkill), skill))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"skill", strSkill);
		return CR_FAILED;
	}

	const SKILL_DATA * skillData = SkillGetData(game, skill);
	ASSERT_RETVAL(skillData, CR_FAILED);

	if (!SkillDataTestFlag(skillData, SKILL_FLAG_LEARNABLE))
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Skill [%s] is not learnable.", ExcelGetRowNameString(game, DATATABLE_SKILLS, skill));
		return CR_FAILED;
	}

	if (!UnitIsA(player, skillData->nRequiredUnittype))
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Skill [%s] is not learnable by you.", ExcelGetRowNameString(game, DATATABLE_SKILLS, skill));
		return CR_FAILED;
	}

	int level = 0;
	sConsoleParseIntegerParam(cmd, 1, strParams, level);
	level = MAX(1, level);

	for ( int i = 1; i < level; i++ )
	{
		if (!SkillPurchaseLevel( player, skill, FALSE, TRUE ))
		{
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Unable to grant skill [%s] for some reason.", ExcelGetRowNameString(game, DATATABLE_SKILLS, skill));
			return CR_FAILED;
		}
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSkillsAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	int level = 0;
	sConsoleParseIntegerParam(cmd, 0, strParams, level);
	level = MAX(1, level);

	unsigned int numSkills = ExcelGetNumRows(game, DATATABLE_SKILLS);
	for (unsigned int nSkill = 0; nSkill < numSkills; ++nSkill)
	{
		for ( int i = 0; i < level; i++ )
		{
			SkillPurchaseLevel( player, nSkill, FALSE, TRUE );
		}
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSkillsUnlock)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	unsigned int numSkills = ExcelGetNumRows(game, DATATABLE_SKILLS);
	for (unsigned int nSkill = 0; nSkill < numSkills; ++nSkill)
	{
		const SKILL_DATA * pSkillData = SkillGetData(game, nSkill);
		if(!pSkillData)
			continue;

		if(SkillDataTestFlag(pSkillData, SKILL_FLAG_REQUIRES_STAT_UNLOCK_TO_PURCHASE))
		{
			UnitSetStat(player, STATS_SKILL_UNLOCKED, nSkill, 1);
		}
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) 
CONSOLE_CMD_FUNC(s_sCmdSkillLevelCheat)
{
	ASSERT_RETVAL(game, CR_FAILED);

	if(IS_SERVER(game))
	{
		s_sCmdSkillsUnlock(game, client, cmd, strParams, pLine);
	}

	GLOBAL_DEFINITION * globalData = DefinitionGetGlobal();

	globalData->dwFlags ^= GLOBAL_FLAG_SKILL_LEVEL_CHEAT;

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Skill Level Cheat: %s.", sGetOnOffString(globalData->dwFlags & GLOBAL_FLAG_SKILL_LEVEL_CHEAT));

	UNIT * player = ClientGetControlUnit(client);
	if ( player )
	{
		if(UnitGetStat( player, STATS_SKILL_POINTS ) == 0)
		{
			UnitSetStat( player, STATS_SKILL_POINTS, 1 );
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"We have given your player a skill point");
		}
		if(UnitGetStat( player, STATS_PERK_POINTS ) == 0)
		{
			UnitSetStat( player, STATS_PERK_POINTS, 1 );
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"We have given your player a perk point");
		}
	}
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
static void sClearAllCooldowns(
	GAME * pGame,
	UNIT * pControlUnit)
{
	if(pControlUnit)
	{
		UnitClearAllSkillCooldowns(pGame, pControlUnit);
		UNIT * pItem = UnitInventoryIterate(pControlUnit, NULL);
		while(pItem)
		{
			UnitClearAllSkillCooldowns(pGame, pItem);
			pItem = UnitInventoryIterate(pControlUnit, pItem);
		}
	}
}
#endif // ISVERSION(CHEATS_ENABLED)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdSkillCooldown)
{
	ASSERT_RETVAL(game, CR_FAILED);

	if(IS_SERVER(game))
	{
		extern BOOL gbDoSkillCooldown;
		gbDoSkillCooldown = 1 - gbDoSkillCooldown;

		sClearAllCooldowns(game, ClientGetControlUnit(client));

		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Skill Cooldown: %s", sGetOnOffString(gbDoSkillCooldown));	
	}
	else
	{
		sClearAllCooldowns(game, GameGetControlUnit(game));
	}
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_cCmdSkillStartRequest)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * pUnit = GameGetDebugUnit(game);
	if(!pUnit)
	{
		pUnit = GameGetControlUnit(game);
	}
	if(!pUnit)
	{
		return CR_FAILED;
	}

	WCHAR strSkill[MAX_CONSOLE_PARAM_VALUE_LEN];
	int skill;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strSkill, arrsize(strSkill), skill))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"skill", strSkill);
		return CR_FAILED;
	}

	VECTOR vTarget = UnitGetStatVector(pUnit, STATS_SKILL_TARGET_X, skill);
	SkillStartRequest(game, pUnit, skill, INVALID_ID, vTarget, 0, SkillGetNextSkillSeed(game));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL c_sCmdStuckStopSkillRequest(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	int nStuckSkill = (int)event_data.m_Data1;
	if(nStuckSkill < 0)
	{
		return FALSE;
	}

	SkillStopRequest(unit, nStuckSkill);
	return TRUE;
}
#endif

CONSOLE_CMD_FUNC(c_sCmdStuck)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * pUnit = GameGetControlUnit(game);
	if(!pUnit)
	{
		return CR_OK;
	}

	int nStuckSkill = GlobalIndexGet(GI_SKILLS_STUCK);
	if(nStuckSkill < 0)
	{
		return CR_OK;
	}

	BOOL bIconIsRed;
	float fCooldownScale;
	int nCooldown;
	c_SkillGetIconInfo(pUnit, nStuckSkill, bIconIsRed, fCooldownScale, nCooldown, FALSE);

	if(nCooldown > 0)
	{
		int nSeconds = ((nCooldown + (GAME_TICKS_PER_SECOND/2)) / GAME_TICKS_PER_SECOND);
		nSeconds = MAX(nSeconds, 1);
		const WCHAR * uszMessage = GlobalStringGet(GS_MUST_WAIT_TO_DO_AGAIN);
		if(uszMessage)
		{
			UIAddChatLineArgs(CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_SERVER), uszMessage, nSeconds);
		}
		return CR_OK;
	}

	if(SkillStartRequest(game, pUnit, nStuckSkill, INVALID_ID, VECTOR(0), SKILL_START_FLAG_DONT_SEND_TO_SERVER, 0))
	{
		MSG_CCMD_STUCK tMsg;
		c_SendMessage(CCMD_STUCK, &tMsg);
		UnitRegisterEventTimed(pUnit, c_sCmdStuckStopSkillRequest, EVENT_DATA(nStuckSkill), 1);
	}

	return CR_OK;
#else
	return CR_FAILED;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdZeroSkillLevel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	WCHAR strSkill[MAX_CONSOLE_PARAM_VALUE_LEN];
	int skill;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strSkill, arrsize(strSkill), skill))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"skill", strSkill);
		return CR_FAILED;
	}
	
	UnitSetStat( player, STATS_SKILL_LEVEL, skill, 0 );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdGiveStrengthPoints)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int desiredPoints = 0;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, desiredPoints))
	{
		//dexterity=22, wisdom=25, vitality=20, strength=13
		UnitSetStat( player, STATS_STRENGTH, desiredPoints );
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdGiveWisdomPoints)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int desiredPoints = 0;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, desiredPoints))
	{
		//dexterity=22, wisdom=25, vitality=20, strength=13
		UnitSetStat( player, STATS_WISDOM, desiredPoints );
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdGiveVitalityPoints)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int desiredPoints = 0;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, desiredPoints))
	{
		//dexterity=22, wisdom=25, vitality=20, strength=13
		UnitSetStat( player, STATS_VITALITY, desiredPoints );
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdGiveDexterityPoints)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int desiredPoints = 0;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, desiredPoints))
	{
		//dexterity=22, wisdom=25, vitality=20, strength=13
		UnitSetStat( player, STATS_DEXTERITY, desiredPoints );
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdSkillRespec)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	if ( SkillRespec( player ) ) 
	{
		return CR_OK;
	}
	else 
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Failed to respec. ");	
		return CR_FAILED;
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdSetParticleFPS)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int desiredFPS = 0;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, desiredFPS))
	{
		c_ParticleSetFPS( (float)desiredFPS );		
	}
	return CR_OK;
}


CONSOLE_CMD_FUNC(s_sCmdGiveSkillPoints)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int desiredPoints = 0;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, desiredPoints))
	{
		UnitSetStat( player, STATS_SKILL_POINTS, UnitGetStat( player, STATS_SKILL_POINTS ) + desiredPoints );
	}
	return CR_OK;
}
CONSOLE_CMD_FUNC(s_sCmdGiveCraftingPoints)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int desiredPoints = 0;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, desiredPoints))
	{
		UnitSetStat( player, STATS_CRAFTING_POINTS, UnitGetStat( player, STATS_CRAFTING_POINTS ) + desiredPoints );
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdLevelUp)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	int level = UnitGetExperienceLevel(player);
	int nMaxLevel = UnitGetMaxLevel( player );
	if (level >= nMaxLevel)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Level is already max.");	
		return CR_FAILED;
	}

	int desiredLevel = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, desiredLevel))
	{
		PlayerDoLevelUp(player, level + 1);
		return CR_OK;
	}
	desiredLevel = PIN(desiredLevel, 1, nMaxLevel);
	if (desiredLevel <= level)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Level is already higher than %d.", desiredLevel);	
		return CR_FAILED;
	}

	PlayerDoLevelUp(player, desiredLevel);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdRankUp)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	int rank = UnitGetPlayerRank(player);
	int nMaxRank = UnitGetMaxRank( player );
	if (rank >= nMaxRank)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Rank is already max.");	
		return CR_FAILED;
	}

	int desiredRank = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, desiredRank))
	{
		PlayerDoRankUp(player, rank + 1);
		return CR_OK;
	}
	desiredRank = PIN(desiredRank, 1, nMaxRank);
	if (desiredRank <= rank)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Rank is already higher than %d.", desiredRank);	
		return CR_FAILED;
	}

	PlayerDoRankUp(player, desiredRank);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdHardcore)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	s_PlayerToggleHardcore( player, FALSE );
	BOOL bIsHardcore = PlayerIsHardcore( player );
	GameSetVariant( game, GV_HARDCORE, bIsHardcore );
	ConsoleString( "Hardcore: %d", bIsHardcore );
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdLeague)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	s_PlayerToggleLeague( player, FALSE );
	ConsoleString( "League: %d", PlayerIsLeague( player ) );

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdElite)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	s_PlayerToggleElite( player, FALSE );
	BOOL bIsElite = PlayerIsElite( player );
	GameSetVariant( game, GV_ELITE, bIsElite );
	ConsoleString( "Elite: %d", bIsElite );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMinigameReset)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	if(AppIsHellgate())
	{
		UNIT * player = ClientGetControlUnit(client);
		ASSERT_RETVAL(player, CR_FAILED);

		PlayerSetStartingMinigameTags(game, player);

		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Reset mini-game.");
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdNameLabelHeight)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	float height = 0;
	if (sConsoleParseFloatParam(cmd, 0, strParams, height))
	{
		gflNameHeightOverride = height;
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Name Label Height override: %1.2f", gflNameHeightOverride);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdItemProcChance)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int probability = 0;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, probability))
	{
		if (probability >= 0)
		{
			probability = PIN(probability, 0, 100);
		}
		else
		{
			probability = -1;
		}
		gflItemProcChanceOverride = (float)probability;
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Item Proc Chance: %d", (int)gflItemProcChanceOverride);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdProcProbability)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int probability = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, probability))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing proc probability.");
		return CR_FAILED;
	}

	probability = PIN(probability, 0, 100);
	ProcSetProbabilityOverride(probability);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Proc Probability Override: %d", probability);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdCritProbability)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int probability = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, probability))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing crit probability.");
		return CR_FAILED;
	}

	probability = PIN(probability, 0, 100);
	s_CombatSetCriticalProbabilityOverride(probability);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Crit Probability Override: %d", probability);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdProcOnGotHit)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	InventoryEquippedTriggerEvent(game, EVENT_GOTHIT, player, player, NULL);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdProcOnGotKilled)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	InventoryEquippedTriggerEvent(game, EVENT_GOTKILLED, player, player, NULL);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdChampionProbability)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	float probability = 0;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, probability))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing champion probability.");
		return CR_FAILED;
	}
	if (probability != -1.0f && (probability < -1.0f || probability > 1.0f))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Champion probability override must be between 0.0 - 1.0 (or -1 to disable).");
		return CR_FAILED;
	}

	s_LevelSetChampionSpawnRateOverride(probability);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Champion probability override: %1.4f.", probability);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTaskGenerateTimeDelay)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	gbGenerateTimeDelay = !gbGenerateTimeDelay;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Task generate time delay: %s", sGetEnabledDisabledString(gbGenerateTimeDelay));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTaskAccept)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_TasksTryAccept();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTaskAcceptReward)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_TasksTryAcceptReward();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTaskReject)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_TasksTryReject();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTaskAbandon)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_TasksTryAbandon();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)



CONSOLE_CMD_FUNC(s_sCmdQuestTugobatSpawnBosses)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}

	s_SpawnObjectsForQuestOnLevelAdd( game, UnitGetLevel( player ), player, QuestTaskDefinitionGet( quest ) );	  	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Spawning all boss and chests for questtask.[%s]", strQuest );

	return CR_OK;
}


CONSOLE_CMD_FUNC(s_sCmdQuestAbandonTugboat)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}

	s_QuestClearFromUser( player, QuestTaskDefinitionGet( quest ) );	  	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Forcing quest [%s] to clear.", strQuest);

	return CR_OK;
}

CONSOLE_CMD_FUNC(s_sCmdQuestCompleteTugboat)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}

	s_QuestCompleteTaskForPlayer( game, player, QuestTaskDefinitionGet( quest ), -1, true );	  	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Forcing quest [%s] complete.", strQuest);

	return CR_OK;
}

CONSOLE_CMD_FUNC(s_sCmdQuestComplete)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}

	s_QuestForceComplete(player, quest);	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Forcing quest [%s] complete.", strQuest);
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)



CONSOLE_CMD_FUNC(s_sCmdSetLevelAreaVisited)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	int nCount = ExcelGetCount( NULL, DATATABLE_LEVEL_AREAS );
	for( int t = 0; t < nCount; t++ )
	{
		UnitSetStat( player, STATS_HUBS_VISITED, t, 1 );
	}
	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"All hubs visited.");
	return CR_OK;

}

CONSOLE_CMD_FUNC(s_sCmdTugboatXPVisuals)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	gDisableXPVisuals = !gDisableXPVisuals;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"XPVisuals toggled.");
	return CR_OK;
}

CONSOLE_CMD_FUNC(s_sCmdQuestAdvanceTugboat)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	gQuestForceComplete = TRUE;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Next Quest will complete.");
	return CR_OK;
}
CONSOLE_CMD_FUNC(s_sCmdQuestAdvance)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}

	QUEST * questData = QuestGetQuest(player, quest);
	ASSERT_RETVAL(questData, CR_FAILED);

	s_QuestAdvance(questData);	
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdQuestStatesComplete)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}

	s_QuestForceStatesComplete(player, quest, FALSE);	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Forcing quest states [%s] complete.", strQuest);
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdQuestActivate)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}

	s_QuestForceActivate(player, quest);	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Forcing quest [%s] activate.", strQuest);
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdQuestUpdateCastInfo)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}


	QUEST * questData = QuestGetQuest(player, quest);
	ASSERT_RETVAL(questData, CR_FAILED);
	s_QuestUpdateCastInfo(questData);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Update cast info for quest [%s].", strQuest);
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdQuestReset)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strQuest[MAX_CONSOLE_PARAM_VALUE_LEN];
	int quest;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strQuest, arrsize(strQuest), quest))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"quest", strQuest);
		return CR_FAILED;
	}

	s_QuestForceReset(player, quest);	
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Forcing quest [%s] reset.", strQuest);
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdTaskKillAllTargets)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	Cheat_s_TaskKillAllTargets(game);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdTaskKillTarget)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int index = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, index))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing task target index.");
		return CR_FAILED;
	}
	if (index < 0 || index > MAX_TASK_UNIT_TARGETS)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid task target index [%d].", index);
		return CR_FAILED;
	}
	Cheat_s_TaskKillTarget(game, index);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdTaskCompleteAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	Cheat_s_TaskCompleteAll(player);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTreasure)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strTreasure[MAX_CONSOLE_PARAM_VALUE_LEN];
	int treasure;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strTreasure, arrsize(strTreasure), treasure))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"treasure class", strTreasure);
		return CR_FAILED;
	}

	int numDrops = 1;

	ITEM_SPEC spec;
	SETBIT(spec.dwFlags, ISF_PLACE_IN_WORLD_BIT);
	spec.unitOperator = player;
	
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		if (PStrIsNumeric(strToken))		// quantity
		{
			numDrops = PStrToInt(strToken);
			numDrops = MAX(numDrops, 1);
		}
		else if (PStrICmp(strToken[0], L'r'))		// report only
		{
			SETBIT(spec.dwFlags, ISF_REPORT_ONLY_BIT);
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unrecognized qualifier [%s].", strToken);
			return CR_FAILED;
		}
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Spawning Treasure Class [%s] %d times.", strTreasure, numDrops);

	char chTreasure[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chTreasure, strTreasure, arrsize(chTreasure));
	LogMessage(TREASURE_LOG, ">>> Dropping '%s' %d times --------------------------\n", chTreasure, numDrops);
	for (unsigned int ii = 0; ii < (unsigned int)numDrops; ++ii)
	{
		s_TreasureSpawnClass(player, treasure, &spec, NULL);
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTreasureAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ITEM_SPEC spec;
	SETBIT(spec.dwFlags, ISF_REPORT_ONLY_BIT);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Generating treasure class report.");

	unsigned int numRows = ExcelGetNumRows(game, DATATABLE_TREASURE);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{
		LogMessage(TREASURE_LOG, ">>> Dropping '%s' --------------------------\n", ExcelGetStringIndexByLine(game, DATATABLE_TREASURE, ii));
		s_TreasureSpawnClass(player, ii, &spec, NULL);
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(s_sCmdTreasureLog)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	BOOL bLog = !LogGetLogging(TREASURE_LOG);
	LogSetLogging(TREASURE_LOG, bLog);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Treasure Log: %s.", sGetOnOffString(bLog));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdJabber)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Jabber: %s.", sGetOnOffString(AppToggleDebugFlag(ADF_JABBERWOCIZE)));
	UIRefreshText();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTextLength)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Text Length: %s.", sGetOnOffString(AppToggleDebugFlag(ADF_TEXT_LENGTH)));
	UIRefreshText();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTextLabels)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if(AppIsHellgate())
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Text Labels: %s.", sGetOnOffString(AppToggleDebugFlag(ADF_TEXT_LABELS)));
		UIRefreshText();
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUIClipping)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"UI Clipping: %s.", sGetOnOffString(AppToggleDebugFlag(ADF_UI_NO_CLIPPING)));
	UIRefreshText();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTextPointSize)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Text Point Size: %s.", sGetOnOffString(AppToggleDebugFlag(ADF_TEXT_POINT_SIZE)));
	UIRefreshText();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTextFontName)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Text Font Name: %s.", sGetOnOffString(AppToggleDebugFlag(ADF_TEXT_FONT_NAME)));
	UIRefreshText();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTextDeveloper)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Developer Language: %s.", sGetOnOffString(AppToggleDebugFlag(ADF_TEXT_DEVELOPER)));
	UIRefreshText();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
static BOOL c_sSoundChaosSoundStopCallback(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	SOUND_ID idSound = (SOUND_ID)event_data.m_Data1;
	c_SoundStop(idSound);
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
static BOOL c_sSoundChaosCallback(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	int nSoundGroup = (int)event_data.m_Data1;
	int nTime = (int)event_data.m_Data2;
	int nCount = (int)event_data.m_Data3;
	int nDelay = (int)event_data.m_Data4;
	BOOL bStreamsOnly = (BOOL)event_data.m_Data5;

	if ((unsigned int)nSoundGroup >= ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_SOUNDS))
	{
		if (bStreamsOnly)
		{
			PickerStart(game, picker);
			int nNumRows = ExcelGetNumRows(game, DATATABLE_SOUNDS);
			for(int i=0; i<nNumRows; i++)
			{
				SOUND_GROUP * pSoundGroup = (SOUND_GROUP*)ExcelGetData(game, DATATABLE_SOUNDS, i);
				if(pSoundGroup && TESTBIT(pSoundGroup->dwFlags, SGF_STREAM_BIT))
				{
					PickerAdd(MEMORY_FUNC_FILELINE(game, i, 1));
				}
			}
			int nPick = PickerChoose(game);
			if(nPick < 0)
			{
				return FALSE;
			}
			nSoundGroup = nPick;
		}
		else
		{
			nSoundGroup = RandGetNum(game, ExcelGetNumRows(game, DATATABLE_SOUNDS));
		}
	}
	
	VECTOR position = c_SoundGetListenerPosition();
	for (int ii = 0; ii < nCount; ++ii)
	{
		int idSound = c_SoundPlay(nSoundGroup, &position);
		if (nTime >= 0)
		{
			UnitRegisterEventTimed(unit, c_sSoundChaosSoundStopCallback, EVENT_DATA(idSound), nTime);
		}
	}

	UnitRegisterEventTimed(unit, c_sSoundChaosCallback, &event_data, nDelay);
	
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundSetMixState)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strMixState[MAX_CONSOLE_PARAM_VALUE_LEN];
	int mixState = INVALID_LINK;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strMixState, arrsize(strMixState), mixState))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"state", strMixState);
		return CR_FAILED;
	}

	int ticks = 0;
	if (!sConsoleParseIntegerParam(cmd, 1, strParams, ticks) || ticks <= 0)
	{
		ticks = 5 * GAME_TICKS_PER_SECOND;
	}

	ASSERT_RETVAL(mixState >= 0, CR_FAILED);
	ASSERT_RETVAL(ticks > 0, CR_FAILED);

	c_SoundSetMixState(mixState, ticks);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundChaos)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = GameGetControlUnit(game);
	ASSERT_RETVAL(player, CR_FAILED);

	BOOL bStreamsOnly = FALSE;

	WCHAR strSoundGroup[MAX_CONSOLE_PARAM_VALUE_LEN];
	int soundGroup;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strSoundGroup, arrsize(strSoundGroup), soundGroup))
	{
		if (PStrICmp(strSoundGroup, L"streams") == 0)
		{
			bStreamsOnly = TRUE;
		}
	}

	int nTime = -1;
	int nCount = 1;
	int nDelay = 1;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrICmp(strToken, L"t=", 2) == 0)
		{
			if (!PStrIsNumeric(strToken + 2))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing time value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			nTime = PStrToInt(strToken + 2);
		}
		else if (PStrICmp(strToken, L"c=", 2) == 0)
		{
			if (!PStrIsNumeric(strToken + 2))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing count value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			nCount = PStrToInt(strToken + 2);
		}
		else if (PStrICmp(strToken, L"d=", 2) == 0)
		{
			if (!PStrIsNumeric(strToken + 2))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing delay value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			nDelay = PStrToInt(strToken + 2);
		}
		else if (PStrICmp(strToken, L"streams") == 0 || PStrICmp(strToken, L"all") == 0)
		{
			// do nothing, but don't fail
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"unrecognized qualifier [%s].", strToken);
			return CR_FAILED;
		}
	}

	UnitRegisterEventTimed(player, c_sSoundChaosCallback, EVENT_DATA(soundGroup, nTime, nCount, nDelay, bStreamsOnly), 0);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundChaosStop)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = GameGetControlUnit(game);
	ASSERT_RETVAL(player, CR_FAILED);

	UnitUnregisterTimedEvent(player, c_sSoundChaosCallback);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSoundSetRolloff)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	float rolloff = 0;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, rolloff))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing rolloff value.");
		return CR_FAILED;
	}

	c_SoundSetRolloffFactor(rolloff);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Sound rolloff value: %1.4f", rolloff);
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPlaySound)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = GameGetControlUnit(game);

	WCHAR strSoundGroup[MAX_CONSOLE_PARAM_VALUE_LEN];
	int soundGroup;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strSoundGroup, arrsize(strSoundGroup), soundGroup))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"sound group", strSoundGroup);
		return CR_FAILED;
	}

	float fDistance = 0.0f;
	int nCutoff = -1;
	int nCount = 1;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrICmp(strToken, L"d=", 2) == 0)
		{
			fDistance = PStrToFloat(strToken + 2);
		}
		else if (PStrICmp(strToken, L"c=", 2) == 0)
		{
			if (!PStrIsNumeric(strToken + 2))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing cutoff value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			nCutoff = PStrToInt(strToken + 2);
		}
		else if (PStrICmp(strToken, L"n=", 2) == 0)
		{
			if (!PStrIsNumeric(strToken + 2))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing count value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			nCount = PStrToInt(strToken + 2);
			nCount = MAX(nCount, 1);
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"unrecognized qualifier [%s].", strToken);
			return CR_FAILED;
		}
	}

	VECTOR position;
	ASSERT_RETVAL(sGetPositionInFrontOfUnit(player, fDistance, &position), CR_FAILED);
	position.fZ += 0.25;

	for(int i=0; i<nCount; i++)
	{
		SOUND_ID idSound = c_SoundPlay(soundGroup, &position);

		if (player && nCutoff >= 0)
		{
			UnitRegisterEventTimed(player, c_sSoundChaosSoundStopCallback, EVENT_DATA(idSound), nCutoff);
		}
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdStopSound)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	
	int idSound = INVALID_ID;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idSound))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing sound id.");
		return CR_FAILED;
	}

	c_SoundStop(idSound);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdLoadSound)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strSoundGroup[MAX_CONSOLE_PARAM_VALUE_LEN];
	int soundGroup;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strSoundGroup, arrsize(strSoundGroup), soundGroup))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"sound group", strSoundGroup);
		return CR_FAILED;
	}

	c_SoundLoad(soundGroup);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUnloadSound)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strSoundGroup[MAX_CONSOLE_PARAM_VALUE_LEN];
	int soundGroup;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strSoundGroup, arrsize(strSoundGroup), soundGroup))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"sound group", strSoundGroup);
		return CR_FAILED;
	}

	c_SoundUnload(soundGroup);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
static SIMPLE_DYNAMIC_ARRAY<int> sgtLoadedSounds;
static BOOL sgbLoadingSounds = FALSE;

static BOOL sLoadAllSoundsIsLoaded(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	for(unsigned int i=0; i<sgtLoadedSounds.Count(); i++)
	{
		int nSoundGroup = sgtLoadedSounds[i];
		if(c_SoundMemoryIsFullyLoaded(nSoundGroup))
		{
			SOUND_GROUP * pSoundGroup = (SOUND_GROUP *)ExcelGetData(game, DATATABLE_SOUNDS, nSoundGroup);
			ConsoleString("Sound '%s' is fully loaded", pSoundGroup->pszName);
			c_SoundUnload(nSoundGroup);
			ArrayRemoveByValue(sgtLoadedSounds, nSoundGroup);
		}
	}
	ConsoleString("Waiting for %d sounds", sgtLoadedSounds.Count());
	if(sgtLoadedSounds.Count() != 0 || sgbLoadingSounds)
	{
		UnitRegisterEventTimed(unit, sLoadAllSoundsIsLoaded, EVENT_DATA(), 1);
	}
	return TRUE;
}

static BOOL sLoadAllSoundsLoadFile(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	int nSoundGroup = (int)event_data.m_Data1;
	SOUND_GROUP * pSoundGroup = (SOUND_GROUP *)ExcelGetData(game, DATATABLE_SOUNDS, nSoundGroup);
	while(!pSoundGroup || pSoundGroup->pszName[0] == '\0')
	{
		nSoundGroup++;
		pSoundGroup = (SOUND_GROUP *)ExcelGetData(game, DATATABLE_SOUNDS, nSoundGroup);
	}
	ConsoleString("Loading sound '%s'", pSoundGroup->pszName);
	c_SoundLoad(nSoundGroup);
	ArrayAddItem(sgtLoadedSounds, nSoundGroup);

	int nSoundGroupCount = ExcelGetNumRows(game, DATATABLE_SOUNDS);
	int nNextSoundGroup = nSoundGroup+1;
	if(nNextSoundGroup < nSoundGroupCount)
	{
		UnitRegisterEventTimed(unit, sLoadAllSoundsLoadFile, EVENT_DATA(nSoundGroup+1), 1);
	}
	else
	{
		sgbLoadingSounds = FALSE;
	}
	return TRUE;
}

CONSOLE_CMD_FUNC(c_sCmdLoadAllSounds)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	sgbLoadingSounds = TRUE;

	UNIT * pUnit = GameGetControlUnit(game);
	ArrayInit(sgtLoadedSounds, NULL, 32);
	sLoadAllSoundsLoadFile(game, pUnit, EVENT_DATA(0));
	sLoadAllSoundsIsLoaded(game, pUnit, EVENT_DATA());
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPlayMusic)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strMusicGroup[MAX_CONSOLE_PARAM_VALUE_LEN];
	int musicGroup;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strMusicGroup, arrsize(strMusicGroup), musicGroup))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"track", strMusicGroup);
		return CR_FAILED;
	}

	c_MusicPlay(musicGroup);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdStopMusic)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_MusicStop();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPlayBGSound)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strBackgroundGroup[MAX_CONSOLE_PARAM_VALUE_LEN];
	int backgroundGroup;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strBackgroundGroup, arrsize(strBackgroundGroup), backgroundGroup))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"track", strBackgroundGroup);
		return CR_FAILED;
	}

	c_BGSoundsPlay(backgroundGroup);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdStopBGSound)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_BGSoundsStop();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSetMusicGroove)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strGroove[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseToken(cmd, strParams, strGroove, arrsize(strGroove)))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"groove", strGroove);
		return CR_FAILED;
	}

	char chGroove[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chGroove, strGroove, arrsize(chGroove));

	c_MusicSetGrooveLevel(ExcelGetLineByStringIndex(game, DATATABLE_MUSICGROOVELEVELS, chGroove));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMusicGrooveToggle)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_MusicSetAutoGrooveChangeEnabled(!c_MusicGetAutoGrooveChangeEnabled());
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Music automatic groove change: %s", sGetEnabledDisabledString(c_MusicGetAutoGrooveChangeEnabled()));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdStopAllSounds)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_SoundStopAll();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
static BOOL c_sPlaySoundCallback(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	int idSound = (int)event_data.m_Data1;
	int soundGroup = (int)event_data.m_Data2;
	c_SoundStop(idSound);

	const VECTOR position = UnitGetPosition(unit);
	SOUND_PLAY_EXINFO tExInfo;
	tExInfo.bForceBlockingLoad = TRUE;
	idSound = c_SoundPlay(soundGroup, &position, &tExInfo);

	if ((unsigned int)soundGroup < ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_SOUNDS) - 1)
	{
		UnitRegisterEventTimed(unit, c_sPlaySoundCallback, EVENT_DATA(idSound, soundGroup + 1), 20);
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPlayAllSounds)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = GameGetControlUnit(game);
	ASSERT_RETVAL(player, CR_FAILED);

	UnitRegisterEventTimed(player, c_sPlaySoundCallback, EVENT_DATA(INVALID_ID, 0), 0);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDeltaSound)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strSoundGroup[MAX_CONSOLE_PARAM_VALUE_LEN];
	int soundGroup;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strSoundGroup, arrsize(strSoundGroup), soundGroup))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"sound", strSoundGroup);
		return CR_FAILED;
	}

	BOOL oldBlockingLoad = c_SoundGetForceBlockingLoad();
	c_SoundSetForceBlockingLoad(TRUE);
	c_SoundLoad(soundGroup);
	c_SoundSetForceBlockingLoad(oldBlockingLoad);

	SOUND_GROUP * soundData = (SOUND_GROUP *)ExcelGetData(game, DATATABLE_SOUNDS, soundGroup);
	if (!soundData)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"couldn't get sound group: %s", strSoundGroup);
		return CR_FAILED;
	}

	float min = soundData->fMinRange;
	float max = soundData->fMaxRange;
	float fadeIn = soundData->fFadeinTime;
	float fadeOut = soundData->fFadeoutTime;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrICmp(strToken, L"vol=", 4) == 0)
		{
			int volume;
			if (!PStrToIntEx(strToken + 4, volume))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing volume value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			c_SoundSetGroupVolume(soundData, volume);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set volume for sound [%s] to: %d\n", strSoundGroup, volume);
		}
		else if (PStrICmp(strToken, L"min=", 4) == 0)
		{
			if (!PStrToFloat(strToken + 4, min))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing min range in qualifier [%s].", strToken);
				return CR_FAILED;
			}
		}
		else if (PStrICmp(strToken, L"max=", 4) == 0)
		{
			if (!PStrToFloat(strToken + 4, max))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing max range in qualifier [%s].", strToken);
				return CR_FAILED;
			}
		}
		else if (PStrICmp(strToken, L"cc=", 3) == 0)
		{
			int canCutOff;
			if (!PStrToIntEx(strToken + 3, canCutOff))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing can cut off value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			SETBIT(soundData->dwFlags, SGF_CANCUTOFF_BIT, canCutOff);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set can cut off for sound [%s] to: %d\n", strSoundGroup, canCutOff);
		}
		else if (PStrICmp(strToken, L"fv=", 3) == 0)
		{
			int freqVariation;
			if (!PStrToIntEx(strToken + 3, freqVariation))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing frequency variation value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			c_SoundSetFrequencyVariation(soundData, freqVariation);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set frequency variation for sound [%s] to: %d\n", strSoundGroup, freqVariation);
		}
		else if (PStrICmp(strToken, L"vv=", 3) == 0)
		{
			int volVariation;
			if (!PStrToIntEx(strToken + 3, volVariation))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing volume variation value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			c_SoundSetVolumeVariation(soundData, volVariation);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set volume variation for sound [%s] to: %d\n", strSoundGroup, volVariation);
		}
		else if (PStrICmp(strToken, L"mwr=", 4) == 0)
		{
			int maxSoundsWithinRadius;
			if (!PStrToIntEx(strToken + 4, maxSoundsWithinRadius))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing max sounds within radius value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			soundData->nMaxSoundsWithinRadius = maxSoundsWithinRadius;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set max sounds within radius for sound [%s] to: %d\n", strSoundGroup, maxSoundsWithinRadius);
		}
		else if (PStrICmp(strToken, L"rad=", 4) == 0)
		{
			float radius = 0.0f;
			if (!PStrToFloat(strToken + 4, radius))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing radius value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			soundData->fRadius = radius;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set radius for sound [%s] to: %1.4f\n", strSoundGroup, radius);
		}
		else if (PStrICmp(strToken, L"fi=", 3) == 0)
		{
			if (!PStrToFloat(strToken + 3, fadeIn))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing fade in time value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
		}
		else if (PStrICmp(strToken, L"fo=", 3) == 0)
		{
			if (!PStrToFloat(strToken + 3, fadeOut))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing fade out time value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
		}
		else if (PStrICmp(strToken, L"hp=", 3) == 0)
		{
			int hardwarePriority;
			if (!PStrToIntEx(strToken + 3, hardwarePriority))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing hardware priority value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			soundData->nHardwarePriority = hardwarePriority;
			c_SoundSetGroupVolume(soundData, soundData->nVolume);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set hardware priority for sound [%s] to: %d\n", strSoundGroup, hardwarePriority);
		}
		else if (PStrICmp(strToken, L"gp=", 3) == 0)
		{
			int gamePriority;
			if (!PStrToIntEx(strToken + 3, gamePriority))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing game priority value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			soundData->nGamePriority = gamePriority;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set game priority for sound [%s] to: %d\n", strSoundGroup, gamePriority);
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"unrecognized qualifier [%s].", strToken);
			return CR_FAILED;
		}
	}

	if (min != soundData->fMinRange || max != soundData->fMaxRange)
	{
		c_SoundSetMinMaxDistance(soundData, min, max);
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set min / max range for sound [%s] to: %1.4f / %1.4f\n", strSoundGroup, min, max);
	}
	if (fadeIn != soundData->fFadeinTime || fadeOut != soundData->fFadeoutTime)
	{
		c_SoundSetFadeTimes(soundData, fadeOut, fadeIn);
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set fade in / fade out times for sound [%s] to: %1.4f / %1.4f\n", strSoundGroup, fadeIn, fadeOut);
	}

	c_SoundCopyGroup(soundData);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDeltaOverride)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strSoundGroup[MAX_CONSOLE_PARAM_VALUE_LEN];
	int soundGroup;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strSoundGroup, arrsize(strSoundGroup), soundGroup))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"sound", strSoundGroup);
		return CR_FAILED;
	}

	BOOL oldBlockingLoad = c_SoundGetForceBlockingLoad();
	c_SoundSetForceBlockingLoad(TRUE);
	c_SoundLoad(soundGroup);
	c_SoundSetForceBlockingLoad(oldBlockingLoad);

	SOUND_GROUP * soundData = (SOUND_GROUP *)ExcelGetData(game, DATATABLE_SOUNDS, soundGroup);
	if (!soundData)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"couldn't get sound group: %s", strSoundGroup);
		return CR_FAILED;
	}

	SOUND_OVERRIDE_DATA * pOverrideData = soundData->pSoundOverride;
	if(!pOverrideData)
	{
		pOverrideData = soundData->pSoundOverride = (SOUND_OVERRIDE_DATA*)MALLOC(NULL, sizeof(SOUND_OVERRIDE_DATA));
		PStrCopy(pOverrideData->pszName, DEFAULT_INDEX_SIZE + SOUND_INDEX_BONUS, soundData->pszName, DEFAULT_INDEX_SIZE + SOUND_INDEX_BONUS);
		pOverrideData->nVolume = 1;
		pOverrideData->fVolume = -1.0f;
		pOverrideData->fMinRange = -1.0f;
		pOverrideData->fMaxRange = -1.0f;
		pOverrideData->nReverbSend = 1;
		pOverrideData->fFrontSend = -1.0f;
		pOverrideData->fCenterSend = -1.0f;
		pOverrideData->fRearSend = -1.0f;
		pOverrideData->fSideSend = -1.0f;
		pOverrideData->fLFESend = -1.0f;
		pOverrideData->nFrequencyVariation = -1;
		pOverrideData->fFrequencyVariation = -1.0f;
		pOverrideData->nVolumeVariation = -1;
		pOverrideData->nVolumeVariationdB = -1;
		pOverrideData->fVolumeVariation = -1.0f;
		pOverrideData->nMaxSoundsWithinRadius = -1;
		pOverrideData->fRadius = -1.0f;
		pOverrideData->fFadeoutTime = -1.0f;
		pOverrideData->nFadeoutTime = -1;
		pOverrideData->fFadeinTime = -1.0f;
		pOverrideData->nFadeinTime = -1;
	}

	float min = pOverrideData->fMinRange;
	float max = pOverrideData->fMaxRange;
	float fadeIn = pOverrideData->fFadeinTime;
	float fadeOut = pOverrideData->fFadeoutTime;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrICmp(strToken, L"vol=", 4) == 0)
		{
			int volume;
			if (!PStrToIntEx(strToken + 4, volume))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing volume value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->nVolume = volume;
			if(volume <= 0)
			{
				pOverrideData->fVolume = dBToVolume(volume);
			}
			else
			{
				pOverrideData->fVolume = -1.0f;
			}
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set volume for sound override [%s] to: %d\n", strSoundGroup, volume);
		}
		else if (PStrICmp(strToken, L"min=", 4) == 0)
		{
			if (!PStrToFloat(strToken + 4, min))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing min range in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->fMinRange = min;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set min range for sound override [%s] to: %1.4f\n", strSoundGroup, min);
		}
		else if (PStrICmp(strToken, L"max=", 4) == 0)
		{
			if (!PStrToFloat(strToken + 4, max))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing max range in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->fMaxRange = max;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set max range for sound override [%s] to: %1.4f\n", strSoundGroup, max);
		}
		else if (PStrICmp(strToken, L"fv=", 3) == 0)
		{
			int freqVariation;
			if (!PStrToIntEx(strToken + 3, freqVariation))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing frequency variation value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->nFrequencyVariation = freqVariation;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set frequency variation for sound override [%s] to: %d\n", strSoundGroup, freqVariation);
		}
		else if (PStrICmp(strToken, L"vv=", 3) == 0)
		{
			int volVariation;
			if (!PStrToIntEx(strToken + 3, volVariation))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing volume variation value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->nVolumeVariation = volVariation;
			pOverrideData->fVolumeVariation = (pOverrideData->fVolume * pOverrideData->nVolumeVariation) / 1000.0f;
			int nUpperBounddB = int(VolumeTodB(pOverrideData->fVolume + pOverrideData->fVolumeVariation));
			pOverrideData->nVolumeVariationdB = ABS(nUpperBounddB - pOverrideData->nVolume);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set volume variation for sound override [%s] to: %d\n", strSoundGroup, volVariation);
		}
		else if (PStrICmp(strToken, L"mwr=", 4) == 0)
		{
			int maxSoundsWithinRadius;
			if (!PStrToIntEx(strToken + 4, maxSoundsWithinRadius))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing max sounds within radius value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->nMaxSoundsWithinRadius = maxSoundsWithinRadius;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set max sounds within radius for sound override [%s] to: %d\n", strSoundGroup, maxSoundsWithinRadius);
		}
		else if (PStrICmp(strToken, L"rad=", 4) == 0)
		{
			float radius = 0.0f;
			if (!PStrToFloat(strToken + 4, radius))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing radius value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->fRadius = radius;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set radius for sound override [%s] to: %1.4f\n", strSoundGroup, radius);
		}
		else if (PStrICmp(strToken, L"fi=", 3) == 0)
		{
			if (!PStrToFloat(strToken + 3, fadeIn))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing fade in time value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set fade in time for sound override [%s] to: %1.4f\n", strSoundGroup, fadeIn);
			pOverrideData->fFadeinTime = fadeIn;
			pOverrideData->nFadeinTime = int(pOverrideData->fFadeinTime * GAME_TICKS_PER_SECOND_FLOAT);
			if(pOverrideData->nFadeinTime <= 0)
			{
				pOverrideData->nFadeinTime = -1;
			}
		}
		else if (PStrICmp(strToken, L"fo=", 3) == 0)
		{
			if (!PStrToFloat(strToken + 3, fadeOut))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing fade out time value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set fade out time for sound override [%s] to: %1.4f\n", strSoundGroup, fadeOut);
			pOverrideData->fFadeoutTime = fadeOut;
			pOverrideData->nFadeoutTime = int(pOverrideData->fFadeoutTime * GAME_TICKS_PER_SECOND_FLOAT);
			if(pOverrideData->nFadeoutTime <= 0)
			{
				pOverrideData->nFadeoutTime = -1;
			}
		}
		else if (PStrICmp(strToken, L"rvs=", 4) == 0)
		{
			int reverbSend;
			if (!PStrToIntEx(strToken + 4, reverbSend))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing reverb send value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->nReverbSend = reverbSend;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set reverb send for sound override [%s] to: %d\n", strSoundGroup, reverbSend);
		}
		else if (PStrICmp(strToken, L"fs=", 3) == 0)
		{
			float frontSend;
			if (!PStrToFloat(strToken + 3, frontSend))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing front send value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->fFrontSend = frontSend;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set front send for sound override [%s] to: %1.4f\n", strSoundGroup, frontSend);
		}
		else if (PStrICmp(strToken, L"cs=", 3) == 0)
		{
			float centerSend;
			if (!PStrToFloat(strToken + 3, centerSend))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing center send value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->fCenterSend = centerSend;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set center send for sound override [%s] to: %1.4f\n", strSoundGroup, centerSend);
		}
		else if (PStrICmp(strToken, L"lfe=", 4) == 0)
		{
			float lfeSend;
			if (!PStrToFloat(strToken + 4, lfeSend))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing LFE send value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->fLFESend = lfeSend;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set LFE send for sound override [%s] to: %1.4f\n", strSoundGroup, lfeSend);
		}
		else if (PStrICmp(strToken, L"rs=", 3) == 0)
		{
			float rearSend;
			if (!PStrToFloat(strToken + 3, rearSend))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing rear send value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->fRearSend = rearSend;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set rear send for sound override [%s] to: %1.4f\n", strSoundGroup, rearSend);
		}
		else if (PStrICmp(strToken, L"ss=", 3) == 0)
		{
			float sideSend;
			if (!PStrToFloat(strToken + 3, sideSend))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing side send value in qualifier [%s].", strToken);
				return CR_FAILED;
			}
			pOverrideData->fSideSend = sideSend;
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Set side send for sound override [%s] to: %1.4f\n", strSoundGroup, sideSend);
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"unrecognized qualifier [%s].", strToken);
			return CR_FAILED;
		}
	}

	c_SoundCopyOverride(pOverrideData);

	oldBlockingLoad = c_SoundGetForceBlockingLoad();
	c_SoundSetForceBlockingLoad(TRUE);
	c_SoundUnload(soundGroup);
	c_SoundLoad(soundGroup);
	c_SoundSetForceBlockingLoad(oldBlockingLoad);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdJumpVelocity)
{
	ASSERT_RETVAL(game, CR_FAILED);

	extern float JUMP_VELOCITY;	

	float velocity;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, velocity))
	{
		if (IS_SERVER(game))
		{
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Jump Velocity: %1.3f", JUMP_VELOCITY);
		}
		return CR_OK;
	}
	if (velocity <= 0.0f)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid jump velocity value.");
		return CR_FAILED;
	}

	JUMP_VELOCITY = velocity;
	if (IS_SERVER(game))
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Jump Velocity: %1.3f", JUMP_VELOCITY);
	}
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdGravity)
{
	ASSERT_RETVAL(game, CR_FAILED);

	extern float GRAVITY_ACCELERATION;	

	float gravity;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, gravity))
	{
		if (IS_SERVER(game))
		{
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Gravity: %1.3f", GRAVITY_ACCELERATION);
		}
		return CR_OK;
	}
	if (gravity <= 0.0f)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid gravity value.");
		return CR_FAILED;
	}

	GRAVITY_ACCELERATION = gravity;
	if (IS_SERVER(game))
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Gravity: %1.3f", GRAVITY_ACCELERATION);
	}
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdWarpsOpen)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	Cheat_OpenWarps(game, player);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdWarpCycle)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT *pPlayer = ClientGetControlUnit(client);
	ASSERT_RETVAL(pPlayer, CR_FAILED);

	LEVEL *pLevel = UnitGetLevel( pPlayer );

	// find the warp we are closest to
	const VECTOR &vPosition = UnitGetPosition( pPlayer );
	UNIT *pWarpClosest = LevelFindClosestUnitOfType( pLevel, vPosition, NULL, UNITTYPE_WARP );
	if (pWarpClosest)
	{
		UNIT *pWarpNext = NULL;
		UNIT *pWarpLowest = NULL;

		// look for warp with next highest id
		for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
		{
			for (int i = TARGET_FIRST; i < NUM_TARGET_TYPES; ++i)
			{
				const TARGET_LIST *pTargetList = &pRoom->tUnitList;
				for (UNIT *pUnit = pTargetList->ppFirst[ i ]; pUnit; pUnit = pUnit->tRoomNode.pNext)
				{
					if (UnitIsA( pUnit, UNITTYPE_WARP ))
					{
						if (pWarpClosest != pUnit)
						{

							if (pWarpLowest == NULL ||
								UnitGetId( pUnit ) < UnitGetId( pWarpLowest ))
							{
								pWarpLowest = pUnit;
							}

							if (UnitGetId( pUnit ) > UnitGetId( pWarpClosest ))
							{
								if (pWarpNext == NULL)
								{
									pWarpNext = pUnit;
								}
								else if(UnitGetId( pUnit ) > UnitGetId( pWarpClosest ) &&
									UnitGetId( pUnit ) < UnitGetId( pWarpNext ))
								{
									pWarpNext = pUnit;
								}
							}
						}
					}		
				}
			}			
		}

		if (pWarpNext == NULL)
		{
			pWarpNext = pWarpLowest;
			if (pWarpNext == NULL)
			{
				pWarpNext = pWarpClosest;
			}
		}

		VECTOR vPosition, vDirection;
		ROOM *pRoom = WarpGetArrivePosition( pWarpNext, &vPosition, &vDirection, WD_FRONT );
		UnitWarp( pPlayer, pRoom, vPosition, vDirection, cgvZAxis, 0, TRUE );
	}
		
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdStringVerify)
{
	StringTableVerifyLanguage();
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSKUValidate)
{

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nSKU;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), nSKU))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"sku", strToken);
		return CR_FAILED;
	}

	BOOL bResult = StringTableCompareSKULanguages( nSKU );
	ConsoleString( "String SKU Compare Result: %s", bResult ? "SUCCESS" : "FAILED" );
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdVisitedLevelsAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	if(AppIsHellgate())
	{
		UNIT * player = ClientGetControlUnit(client);
		ASSERT_RETVAL(player, CR_FAILED);

		unsigned int numLevels = ExcelGetNumRows(game, DATATABLE_LEVEL);
		for (unsigned int ii = 0; ii < numLevels; ++ii)
		{
			const LEVEL_DEFINITION * levelData = LevelDefinitionGet(ii);
			ASSERT_CONTINUE(levelData);

			if (levelData->szMapFrame[0])
			{
				LevelMarkVisited(player, ii, WORLDMAP_VISITED);
			}
		}
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdVisitedLevelsClear)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	if(AppIsHellgate())
	{
		UNIT * player = ClientGetControlUnit(client);
		ASSERT_RETVAL(player, CR_FAILED);

		UnitClearStatsRange(player, STATS_PLAYER_VISITED_LEVEL_BITFIELD);
		//I'm not sure if this still works...money's on no - bmanegold
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#ifdef DEBUG_PICKUP
CONSOLE_CMD_FUNC(s_sCmdPickupDebug)
{
	gDrawNearby = TRUE;
	return CR_OK;
}
#endif
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdReserveNodes)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	extern BOOL drbRESERVE_NODES;
	drbRESERVE_NODES = !drbRESERVE_NODES;

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Reserve Nodes: %s", sGetOnOffString(drbRESERVE_NODES));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSTDump)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

#ifdef DRB_SUPER_TRACKER
	int id;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, id))
	{
		return CR_FAILED;
	}
	drbSTDump(GameGetId(game), id);
#endif

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdCameraDebugUnit)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * unit = GameGetDebugUnit(AppGetCltGame());
	if (unit)
	{
		GameSetCameraUnit(AppGetCltGame(), UnitGetId(unit));
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdShowSeed)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (AppGetType() != APP_TYPE_SINGLE_PLAYER || !AppGetSrvGame())
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"seed command is only available in single player.");
		return CR_FAILED;
	}

	DWORD dwSeed;
	if (sConsoleParseDWORDParam(cmd, 0, strParams, dwSeed))
	{
		GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
		if(!pGlobal)
			return CR_FAILED;
		pGlobal->dwSeed = dwSeed;
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Global Dungeon Seed: %lu.", pGlobal->dwSeed);
	}
	UNIT * player = GameGetControlUnit(game);
	ASSERT_RETVAL(player, CR_FAILED);
	LEVEL * level = UnitGetLevel(player);
	ASSERT_RETVAL(level, CR_FAILED);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Current Dungeon Seed: %lu.", level->m_dwSeed);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdStatsDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL wasInactive = !GameGetDebugFlag(game, DEBUGFLAG_STATS_DEBUG);

	if (strParams[0] == 0 && !wasInactive)
	{
		GameSetDebugFlag(game, DEBUGFLAG_STATS_DEBUG, FALSE);
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_STATS, 0);
		return CR_OK;
	}

	GameSetDebugFlag(game, DEBUGFLAG_STATS_DEBUG, TRUE);

	if (wasInactive)
	{
		GameSetDebugFlag(game, DEBUGFLAG_STATS_BASEUNIT, TRUE);
		GameSetDebugFlag(game, DEBUGFLAG_STATS_MODLISTS, TRUE);
		GameSetDebugFlag(game, DEBUGFLAG_STATS_RIDERS, TRUE);
	}

	BOOL setClientServer = FALSE;
	BOOL setShowBaseUnit = FALSE;
	BOOL setShowModlists = FALSE;
	BOOL setShowRiders = FALSE;
	BOOL setDump = FALSE;

	while (strParams[0])
	{
		if (!setClientServer && PStrICmp(strParams[0], L'c') && (sIsValidParamEnd(strParams[1]) || strParams[1] == L'+' || strParams[1] == L'-'))
		{
			setClientServer = TRUE;
			strParams = sConsoleSkipDelimiter(strParams + 1);
			GameSetDebugFlag(game, DEBUGFLAG_STATS_SERVER, FALSE);
		}
		else if (!setClientServer && PStrICmp(strParams[0], L's') && (sIsValidParamEnd(strParams[1]) || strParams[1] == L'+' || strParams[1] == L'-'))
		{
			setClientServer = TRUE;
			strParams = sConsoleSkipDelimiter(strParams + 1);
			GameSetDebugFlag(game, DEBUGFLAG_STATS_SERVER, TRUE);
		}
		else if (!setShowModlists && PStrICmp(strParams[0], L'm') && (sIsValidParamEnd(strParams[1]) || strParams[1] == L'+' || strParams[1] == L'-'))
		{
			setShowModlists = TRUE;
			strParams = sConsoleSkipDelimiter(strParams + 1);
			GameToggleDebugFlag(game, DEBUGFLAG_STATS_MODLISTS);
		}
		else if (!setShowRiders && PStrICmp(strParams[0], L'r') && (sIsValidParamEnd(strParams[1]) || strParams[1] == L'+' || strParams[1] == L'-'))
		{
			setShowRiders = TRUE;
			strParams = sConsoleSkipDelimiter(strParams + 1);
			GameToggleDebugFlag(game, DEBUGFLAG_STATS_RIDERS);
		}
		else if (!setShowBaseUnit && PStrICmp(strParams[0], L'u') && (sIsValidParamEnd(strParams[1]) || strParams[1] == L'+' || strParams[1] == L'-'))
		{
			setShowBaseUnit = TRUE;
			strParams = sConsoleSkipDelimiter(strParams + 1);
			GameToggleDebugFlag(game, DEBUGFLAG_STATS_BASEUNIT);
		}
		else if (!setDump && PStrICmp(strParams[0], L'd') && (sIsValidParamEnd(strParams[1]) || strParams[1] == L'+' || strParams[1] == L'-'))
		{
			setDump = TRUE;
			strParams = sConsoleSkipDelimiter(strParams + 1);
			GameToggleDebugFlag(game, DEBUGFLAG_STATS_DUMP);
		}
		else
		{
			break;
		}
	}

	if (!strParams[0])
	{
		goto _done;
	}

	int addType = 0;
	if (strParams[0] == L'+')
	{
		addType = 1;
		strParams = sConsoleSkipDelimiter(strParams + 1);
	}
	else if (strParams[0] == L'-')
	{
		addType = -1;
		strParams = sConsoleSkipDelimiter(strParams + 1);
	}

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		if (addType < 0)
		{
			GameSetDebugFlag(game, DEBUGFLAG_STATS_DEBUG, FALSE);
		}
		goto _done;
	}

	char chToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chToken, strToken, arrsize(chToken));
	if (chToken[0])
	{
		unsigned int numStats = ExcelGetNumRows(game, DATATABLE_STATS);
		for (unsigned int ii = 0; ii < numStats; ii++)
		{
			const STATS_DATA * statsData = StatsGetData(game, ii);
			ASSERT_BREAK(statsData);
			if (PStrWildcardMatch(statsData->m_szName, (const char *)chToken))
			{
				SETBIT(game->pdwDebugStatsTrace, ii, addType >= 0);
			}
			else if (wasInactive || addType == 0)
			{
				CLEARBIT(game->pdwDebugStatsTrace, ii);
			}
		}
	}

_done:
	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_STATS, GameGetDebugFlag(game, DEBUGFLAG_STATS_DEBUG));

	return CR_OK;
}
#endif


#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUnitList)
{
	ASSERT_RETVAL(game, CR_FAILED);
	GlobalUnitTrackerTrace();
	return CR_OK;
}
#endif

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdEventList)
{
	ASSERT_RETVAL(game, CR_FAILED);
	GlobalEventTrackerTrace();
	return CR_OK;
}
#endif

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdEventHandlerList)
{
	ASSERT_RETVAL(game, CR_FAILED);
	GlobalEventHandlerTrackerTrace();
	return CR_OK;
}
#endif

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdStatsTracker)
{
	ASSERT_RETVAL(game, CR_FAILED);
	GlobalStatsTrackerTrace();
	return CR_OK;
}
#endif

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSetStatsDebugUnit)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	int idUnit;
	if (sConsoleParseIntegerParam(cmd, 0, strParams, idUnit))
	{
		UNIT *pUnit = UnitGetById(game, idUnit);
		if (pUnit)
		{
			GameSetDebugUnit(game, pUnit);
			c_SendDebugUnitId(UnitGetId(pUnit));
			ConsoleString(CONSOLE_SYSTEM_COLOR, "Select debug unit: %d [%s]", pUnit->unitid, UnitGetDevName(pUnit));
			return CR_OK;
		}
	}

	ConsoleString(CONSOLE_SYSTEM_COLOR, "Select debug unit: UNIT NOT FOUND");
	return CR_FAILED;

}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdStatsTrace)
{
	ASSERT_RETVAL(game, CR_FAILED);

	int bufLen = CONSOLE_MAX_STR;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if ((strParams = PStrTok(strToken, strParams, PARAM_DELIM, arrsize(strToken), bufLen)) == NULL)
	{
		GameToggleDebugFlag(game, DEBUGFLAG_STATS_TRACE);
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"%s stats trace set to %s.", sGetClientServerString(game), sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_STATS_TRACE)));
		return CR_OK;
	}

	BOOL add = TRUE;
	if (PStrICmp(strToken, L"c") == 0)
	{
		if (!IS_CLIENT(game))
		{
			return CR_OK;
		}
		if ((strParams = PStrTok(strToken, strParams, PARAM_DELIM, arrsize(strToken), bufLen)) == NULL)
		{
			GameToggleDebugFlag(game, DEBUGFLAG_STATS_TRACE);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Client stats trace: %s.", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_STATS_TRACE)));
			return CR_OK;
		}
	}
	else if (PStrICmp(strToken, L"s") == 0)
	{
		if (!IS_SERVER(game))
		{
			return CR_OK;
		}
		if ((strParams = PStrTok(strToken, strParams, PARAM_DELIM, arrsize(strToken), bufLen)) == NULL)
		{
			GameToggleDebugFlag(game, DEBUGFLAG_STATS_TRACE);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Server stats trace: %s.", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_STATS_TRACE)));
			return CR_OK;
		}
	}

	if ((strToken[0] == L'+' || strToken[0] == L'-') && strToken[1] == 0)
	{
		add = (strToken[0] == L'+');
		if ((strParams = PStrTok(strToken, strParams, PARAM_DELIM, arrsize(strToken), bufLen)) == NULL)
		{
			GameSetDebugFlag(game, DEBUGFLAG_STATS_TRACE, add);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Server stats trace: %s.", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_STATS_TRACE)));
			return CR_OK;
		}
	}

	if (add)
	{
		GameSetDebugFlag(game, DEBUGFLAG_STATS_TRACE, TRUE);
	}

	if (!GameGetDebugFlag(game, DEBUGFLAG_STATS_TRACE))
	{
		return CR_OK;
	}

	char chToken[CONSOLE_MAX_STR];
	PStrCvt(chToken, strToken, CONSOLE_MAX_STR);

	unsigned int numStats = ExcelGetNumRows(game, DATATABLE_STATS);
	for (unsigned int ii = 0; ii < numStats; ii++)
	{
		const STATS_DATA * statsData = StatsGetData(game, ii);
		ASSERT_CONTINUE(statsData);
		if (TESTBIT(game->pdwDebugStatsTrace, ii) == add)
		{
			continue;
		}

		if (PStrWildcardMatch(statsData->m_szName, (const char *)chToken))
		{
			SETBIT(game->pdwDebugStatsTrace, ii, add);
			WCHAR strStat[CONSOLE_MAX_STR];
			PStrCvt(strStat, chToken, CONSOLE_MAX_STR);
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"stats trace for %s set to %s.", strStat, sGetOnOffString(TESTBIT(game->pdwDebugStatsTrace, ii)));
		}
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdModeDebug)
{
	ASSERT_RETVAL(game, CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_MODE_TRACE);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"%s MODEDEBUG: %s", sGetClientServerString(game), sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_MODE_TRACE)));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdModeDump)
{
	ASSERT_RETVAL(game, CR_FAILED);

	UNIT * pUnit = GameGetDebugUnit(game);
	if(!pUnit)
	{
		pUnit = IS_SERVER(game) ? ClientGetControlUnit(client) : GameGetControlUnit(game);
	}
	if(!pUnit)
		return CR_FAILED;

	ConsoleStringAndTrace(game, client, CONSOLE_SYSTEM_COLOR, L"Active Modes for %S on [%S]:\n", UnitGetDevName(pUnit), CLTSRVSTR(game));
	int nNumModes = ExcelGetNumRows(NULL, DATATABLE_UNITMODES);
	for(int i=0; i<nNumModes; i++)
	{
		const UNITMODE_DATA * pUnitModeData = (const UNITMODE_DATA *)ExcelGetData(NULL, DATATABLE_UNITMODES, i);
		if(UnitTestMode(pUnit, (UNITMODE)i))
		{
			ConsoleStringAndTrace(game, client, CONSOLE_SYSTEM_COLOR, L"%S\n", pUnitModeData->szName);
		}
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdFactionChange)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strFaction[MAX_CONSOLE_PARAM_VALUE_LEN];
	int faction;
	if (!sConsoleParseExcelLookupOrName(game, cmd, 0, strParams, strFaction, arrsize(strFaction), faction))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"faction", strFaction);
		return CR_FAILED;
	}

	int score;
	if (!sConsoleParseIntegerParam(cmd, 1, strParams, score))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing faction score.");
	}

	FactionScoreChange(player, faction, score);
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void s_sGetDefaultPlayerName(
	WCHAR * strName,
	unsigned int lenName)
{
	GLOBAL_DEFINITION * globalData = DefinitionGetGlobal();
	ASSERT(globalData);
	if (globalData && globalData->szPlayerName[0])
	{
		PStrCvt(strName, globalData->szPlayerName, lenName);
	}
	else
	{
		PStrCopy(strName, DEFAULT_PLAYER_NAME, lenName);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSaveDefaultName(
	const WCHAR * strName,
	unsigned int lenName)
{
	ASSERT_RETURN(strName);

	char chName[CONSOLE_MAX_STR];
	unsigned int len = MIN(lenName, arrsize(chName));
	PStrCvt(chName, strName, len);

	GLOBAL_DEFINITION * globalData = DefinitionGetGlobal();
	ASSERT_RETURN(globalData);

	len = MIN(lenName, arrsize(globalData->szPlayerName));
	PStrCopy(globalData->szPlayerName, chName, len);

	CDefinitionContainer * container = CDefinitionContainer::GetContainerByType(DEFINITION_GROUP_GLOBAL);
	ASSERT_RETURN(container);

	container->Save(globalData);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPlayerNew)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	WCHAR strName[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strName, arrsize(strName)))
	{
		s_sGetDefaultPlayerName(strName, arrsize(strName));
	}

	// class and gender options
	unsigned int playerClass = 0;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L" ,"))
	{
		// This doesn't make me happy, but it's pretty much the same as it was before.  Oh, well.
		if (PStrICmp(strToken, L"gua", 3) == 0)
		{
			playerClass = 0;
		}
		else if (PStrICmp(strToken, L"bla", 3) == 0)
		{
			playerClass = 2;
		}
		else if (PStrICmp(strToken, L"exe", 3) == 0)
		{
			playerClass = 4;
		}
		else if (PStrICmp(strToken, L"rec", 3) == 0)
		{
			playerClass = 6;
		}
		else if (PStrICmp(strToken, L"eng", 3) == 0)
		{
			playerClass = 8;
		}
		else if (PStrICmp(strToken, L"mar", 3) == 0)
		{
			playerClass = 10;
		}
		else if (PStrICmp(strToken, L"met", 3) == 0)
		{
			playerClass = 12;
		}
		else if (PStrICmp(strToken, L"evo", 3) == 0)
		{
			playerClass = 14;
		}
		else if (PStrICmp(strToken, L"sum", 3) == 0)
		{
			playerClass = 16;
		}
		else if (PStrICmp(strToken, L"fem", 3) == 0)
		{
			playerClass += 1;
		}
	}

	unsigned int numClasses = ExcelGetNumRows(game, DATATABLE_PLAYERS);
	playerClass = PIN(playerClass, (unsigned int)0, (unsigned int)(numClasses - 1));

	if (!s_MakeNewPlayer(game, client, playerClass, strName))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Error generating new player.");
		return CR_FAILED;
	}

	s_sSaveDefaultName(strName, arrsize(strName));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPlayerNewSpecific)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int playerClass;
	if (!sConsoleExcelLookup(game, DATATABLE_PLAYERS, cmd->m_strCommand.GetStr(), CONSOLE_MAX_STR, playerClass))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid player class [%s].", cmd->m_strCommand.GetStr());
		return CR_FAILED;
	}

	WCHAR strName[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strName, arrsize(strName)))
	{
		s_sGetDefaultPlayerName(strName, arrsize(strName));
	}

	if (!s_MakeNewPlayer(game, client, playerClass, strName))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Error generating new player.");
		return CR_FAILED;
	}

	s_sSaveDefaultName(strName, arrsize(strName));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPlayerLoad)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	WCHAR strName[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strName, arrsize(strName)))
	{
		s_sGetDefaultPlayerName(strName, arrsize(strName));
	}

	UNIT * oldunit = ClientGetControlUnit(client);
	ASSERT_RETVAL(oldunit, CR_FAILED);

	CLIENT_SAVE_FILE tClientSaveFile;
	PStrCopy( tClientSaveFile.uszCharName, strName, MAX_CHARACTER_NAME );
	tClientSaveFile.pBuffer = NULL;
	tClientSaveFile.dwBufferSize = 0;
	tClientSaveFile.eFormat = PSF_HIERARCHY;
	UNIT * newunit = PlayerLoad(game, &tClientSaveFile, FALSE, ClientGetId(client));
	if (!newunit)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unable to load player [%s].", strName);
		return CR_FAILED;
	}

	if (!s_PlayerAdd(game, client, newunit))
	{
		UnitFree(newunit, 0);
		ClientSetControlUnit(game, client, oldunit);
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Error adding player to game.");
		return CR_FAILED;
	}

	s_sSaveDefaultName(strName, arrsize(strName));

	UnitFree(oldunit, UFF_SEND_TO_CLIENTS);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPlayerSave)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);
	ASSERT_RETVAL(player->szName, CR_FAILED);

	if (!AppPlayerSave(player))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Error saving player [%s].", player->szName);
		return CR_FAILED;
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Player [%s] saved.", player->szName);	

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
unsigned int sGetFirstWeapon(
	GAME * game,
	unsigned int start = 0)
{
	unsigned int numItems = ExcelGetNumRows(game, DATATABLE_ITEMS);
	for (unsigned int ii = start; ii < numItems; ++ii)
	{
		const UNIT_DATA * itemData = ItemGetData(ii);
		if ( ! itemData )
			continue;
		if (UnitIsA(itemData->nUnitType, UNITTYPE_WEAPON))
		{
			return ii;
		}
	}
	return (unsigned int)INVALID_LINK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
unsigned int sCountTestWeapons(
	GAME * game,
	BOOL testAll,
	unsigned int start,
	int nSkillToTest )
{
	unsigned int count = 0;
	unsigned int numItems = ExcelGetNumRows(game, DATATABLE_ITEMS);

	const SKILL_DATA * pSkillData = SkillGetData( game, nSkillToTest );
	if (! pSkillData)
		return 0;
	for (unsigned int ii = start; ii < numItems; ++ii)
	{
		const UNIT_DATA * itemData = ItemGetData(ii);
		if ( ! itemData )
			continue;

		if (!UnitIsA(itemData->nUnitType, UNITTYPE_WEAPON))
		{
			continue;
		}

		if ( !UnitIsA( itemData->nUnitType, pSkillData->nRequiredWeaponUnittype ))
		{
			continue;
		}

		if (testAll || UnitDataTestFlag( itemData, UNIT_DATA_FLAG_SPAWN ))
		{
			++count;
		}
	}
	return count;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
unsigned int sCountTestModAffixes(
	GAME * game)
{
	unsigned int count = 0;
	unsigned int affixCount = ExcelGetNumRows(game, DATATABLE_AFFIXES);
	for (unsigned int ii = 0; ii < affixCount; ++ii)
	{
		const AFFIX_DATA * affixData = AffixGetData(ii);
		ASSERT_CONTINUE(affixData);
		for (unsigned int jj = 0; jj < AFFIX_UNITTYPES; ++jj)
		{
			if (UnitIsA(affixData->nAllowTypes[jj], UNITTYPE_MOD))
			{
				++count;
				break;
			}
		}
	}
	return count;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
unsigned int sCountTestMonsters(
	GAME * game)
{
	unsigned int count = 0;
	unsigned int monsterCount = ExcelGetNumRows(game, DATATABLE_MONSTERS);
	for (unsigned int ii = 0; ii < monsterCount; ++ii)
	{
		const UNIT_DATA * monsterData = MonsterGetData(game, ii);
		if (! monsterData)
			continue;
		if (UnitDataTestFlag( monsterData, UNIT_DATA_FLAG_SPAWN ))
		{
			++count;
		}
	}
	return count;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static int sGetTestTicks(
	GAME * game,
	UNIT * unit)
{
	int nLeftSkill = UnitGetStat( unit, STATS_SKILL_LEFT );
	const SKILL_DATA * pLeftSkillData = SkillGetData( game, nLeftSkill );
	if ( ! pLeftSkillData )
		return 0;
	for ( int i = -1; i < NUM_FALLBACK_SKILLS; i++ )
	{
		int nSkill = i == -1 ? nLeftSkill : pLeftSkillData->pnFallbackSkills[ i ];

		UNIT * pWeapons[MAX_WEAPONS_PER_UNIT];
		UnitGetWeapons(unit, nSkill, pWeapons, FALSE);

		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			int nWeaponSkill = ItemGetPrimarySkill( pWeapons[ i ] );
			if ( nWeaponSkill != INVALID_ID )
			{
				const SKILL_DATA * pSkillData = SkillGetData( game, nWeaponSkill );
				return pSkillData->nTestTicks;
			}
		}
	}
	return pLeftSkillData->nTestTicks;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static UNIT * s_sSpawnAndEquipWeapon( 
	  UNIT * unit, 
	  int nItemClass)
{
	GAME * game = UnitGetGame( unit );

	ITEM_SPEC tSpec;
	tSpec.nLevel = UnitGetExperienceLevel(unit);
	tSpec.nLevel = MAX(tSpec.nLevel, 1);
	SETBIT(tSpec.dwFlags, ISF_MAX_SLOTS_BIT);
	SETBIT(tSpec.dwFlags, ISF_IDENTIFY_BIT);

	BOOL bElementEffects[MAX_DAMAGE_TYPES];
	memclear(bElementEffects, arrsize(bElementEffects) * sizeof(BOOL));

	UNIT * pItem = NULL;
	int nNumWeaponsToSpawn = UnitIsPlayer( unit ) ? 2 : 1;
	for ( int i = 0; i < nNumWeaponsToSpawn; i++ )
	{
		// spawn two so that you can dual wield them
		tSpec.nItemClass = nItemClass;
		tSpec.pSpawner = unit;

		DWORD dwFlags = ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT | ITEM_SPAWN_CHEAT_FLAG_PICKUP | ITEM_SPAWN_CHEAT_FLAG_FORCE_EQUIP;

		pItem = s_ItemSpawnForCheats( game, tSpec, UnitGetClient(unit), dwFlags, bElementEffects );
	}

	return pItem;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL sTestWeapon(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	int nItemClass = (int)event_data.m_Data1;
	int nTicks = (int)event_data.m_Data2;
	int nModAffix = (int)event_data.m_Data3;
	int nMonsterClass = (int)event_data.m_Data4;
	BOOL bTestAll = (BOOL)event_data.m_Data5;

	if ( IsUnitDeadOrDying( unit ) )
		return FALSE;

	// fill up with power and health between weapons so that skills and monsters can be tested
	UnitSetStat( unit, STATS_POWER_CUR, UnitGetStat( unit, STATS_POWER_MAX ) );
	UnitSetStat( unit, STATS_HP_CUR, UnitGetStat( unit, STATS_HP_MAX ) );

	int nSkill = GlobalIndexGet( GI_SKILLS_PLAYER_TURRET_ZAP );
	const SKILL_DATA * skillData = SkillGetData(game, nSkill); 
	SkillStartCooldown(game, unit, NULL, nSkill, 0, skillData, 200, TRUE);

	if (nMonsterClass != INVALID_ID)
	{
#if ISVERSION(CHEATS_ENABLED)
		s_UnitKillAll(game, NULL, TRUE);
#endif

		int nMonsterClassCount = ExcelGetNumRows(game, DATATABLE_MONSTERS);
		const UNIT_DATA * pMonsterData = NULL;
		for ( ; nMonsterClass < nMonsterClassCount; nMonsterClass++ )
		{
			pMonsterData = MonsterGetData(game, nMonsterClass);
			if ( ! pMonsterData )
				continue;
			if ( UnitDataTestFlag( pMonsterData, UNIT_DATA_FLAG_SPAWN ) )
				break;
		}
		if ( pMonsterData && nMonsterClass < nMonsterClassCount )
		{
			LogMessage(LOG_FILE, "Testing against: %s", pMonsterData->szName);

			WCHAR strMonster[CONSOLE_MAX_STR];
			PStrCvt(strMonster, pMonsterData->szName, arrsize(strMonster));
			GAMECLIENT * client = UnitGetClient(unit);
			if (client)
				ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Testing against: %s", strMonster);

			VECTOR vDirection = UnitGetFaceDirection( unit, FALSE );
			vDirection.fZ = 0.0f;
			VECTOR vDelta = vDirection;
			vDelta *= 2.0f;
			VECTOR vPosition = UnitGetPosition( unit ) + vDelta;
			ROOM * pRoom = RoomGetFromPosition( game, UnitGetRoom( unit ), &vPosition );
			if ( pRoom )
			{
				MONSTER_SPEC tSpec;
				tSpec.nClass = nMonsterClass;
				tSpec.nExperienceLevel = RoomGetMonsterExperienceLevel( pRoom );
				tSpec.pRoom = pRoom;
				tSpec.vPosition= vPosition;
				tSpec.vDirection = -vDirection;
				s_MonsterSpawn( game, tSpec );
			}

			// drone test just keeps spawning the same popcorn monster while switching weapons
			if (!PetIsPet(unit))
			{
				EVENT_DATA tNextEventData;
				memcpy( &tNextEventData, &event_data, sizeof(EVENT_DATA) );
				tNextEventData.m_Data4 = nMonsterClass + 1;

				int nTestTicks = sGetTestTicks( game, unit );
				nTicks = nTestTicks ? nTestTicks : nTicks;

				UnitRegisterEventTimed( unit, sTestWeapon, &tNextEventData, nTicks );
				return TRUE;
			}
		} else {
			nMonsterClass = 0;
		}
	}

	const UNIT_DATA * unitData = NULL;
	int nItemCount = ExcelGetNumRows( game, DATATABLE_ITEMS );
	int nSkillToTest = UnitGetStat( unit, STATS_SKILL_LEFT );
	const SKILL_DATA * pSkillData = SkillGetData( game, nSkillToTest );
	if (! pSkillData)
		return FALSE;
	for( ; nItemClass < nItemCount; nItemClass++ )
	{
		unitData = ItemGetData( nItemClass );
		if ( ! unitData )
			continue;
		if ( ! UnitIsA( unitData->nUnitType, UNITTYPE_WEAPON ) )
			continue;

		if ( !UnitIsA( unitData->nUnitType, pSkillData->nRequiredWeaponUnittype ))
		{
			continue;
		}
		if ( bTestAll || UnitDataTestFlag( unitData, UNIT_DATA_FLAG_SPAWN ) )
			break;
	}
	if ( nItemClass >= nItemCount )
		return TRUE;

	{// clear items from hands
		UNIT* pOldItem = UnitInventoryGetByLocationAndXY( unit, INVLOC_RHAND, 0, 0 );
		if ( pOldItem )
			UnitFree( pOldItem, UFF_SEND_TO_CLIENTS );
		pOldItem = UnitInventoryGetByLocationAndXY( unit, INVLOC_LHAND, 0, 0 );
		if ( pOldItem )
			UnitFree( pOldItem, UFF_SEND_TO_CLIENTS );
	}

	UNIT * pItem = s_sSpawnAndEquipWeapon( unit, nItemClass );

	const AFFIX_DATA * pAffixData = NULL;
	if ( nModAffix != INVALID_ID && pItem )
	{
		ITEM_SPEC tModSpec;
		tModSpec.nLevel = UnitGetExperienceLevel(unit);
		tModSpec.nLevel = MAX(tModSpec.nLevel, 1);

		BOOL bFoundModWithAffix = FALSE;
		int nAffixCount = ExcelGetNumRows( game, DATATABLE_AFFIXES );
		for ( ; !bFoundModWithAffix && nModAffix < nAffixCount; nModAffix++ )
		{
			pAffixData = AffixGetData( nModAffix );
			tModSpec.nAffixes[ 0 ] = nModAffix;
			UNIT_ITERATE_STATS_RANGE(pItem, STATS_ITEM_SLOTS, stats_entry, ii, 16)
			{
				int location = stats_entry[ii].GetParam();
				int slots = stats_entry[ii].value;

				if ( ! slots )
					continue;

				INVLOC_HEADER tLocInfo;
				if (!UnitInventoryGetLocInfo( pItem, location, &tLocInfo))
					continue;

				int item_count = ExcelGetNumRows(game, DATATABLE_ITEMS);
				for (int ii = 0; ii < item_count; ii++)
				{
					const UNIT_DATA* item_data = ItemGetData(ii);
					if (! item_data )
						continue;

					// can this item fit into this location?
					if (! IsAllowedUnit(item_data->nUnitType, tLocInfo.nAllowTypes, INVLOC_UNITTYPES) )
						continue;

					// can we use this affix on this item?
					if (! IsAllowedUnit(item_data->nUnitType, pAffixData->nAllowTypes, AFFIX_UNITTYPES ) )
						continue;

					ITEM_SPEC tModSpecCopy;
					// make a copy because s_ItemSpawn will modify the ITEM_SPEC
					memcpy( &tModSpecCopy, &tModSpec, sizeof( ITEM_SPEC ) );

					CLEARBIT(tModSpecCopy.dwFlags, ISF_PLACE_IN_WORLD_BIT);
					tModSpecCopy.nItemClass = ii;
					tModSpecCopy.pSpawner = unit;
					UNIT* pMod = s_ItemSpawn(game, tModSpecCopy, NULL);
					if ( tModSpec.nAffixes[ 0 ] != tModSpecCopy.nAffixes[ 0 ] )
					{ // The item code clears out affixes that are used.  So if they don't match one was used
						bFoundModWithAffix = TRUE;
						int modlocation, x, y;
						BOOL bLocOpen = ItemGetEquipLocation(pItem, pMod, &modlocation, &x, &y);
						if (bLocOpen && modlocation == location)
						{
							BOOL bItemAdded = UnitInventoryAdd(INV_CMD_PICKUP, pItem, pMod, modlocation, x, y);
							if (!bItemAdded)
							{
								ConsoleString(CONSOLE_ERROR_COLOR, L"ITEM command unable attach mod to item");
								continue;
							}
						}
						break;
					} else {
						UnitFree( pMod, UFF_SEND_TO_CLIENTS );
					}
				}

				if ( bFoundModWithAffix )
					break;
			}
			UNIT_ITERATE_STATS_END;
		}

		if ( !bFoundModWithAffix )
		{ // move to the next item now and start over with the affixes
			sTestWeapon( game, unit, EVENT_DATA( nItemClass + 1, nTicks, 0 ) );
			return TRUE;
		} 
		LogMessage( LOG_FILE, "Creating Weapon: %s - %s", unitData->szName, pAffixData->szName );

	} else if ( pItem ) {
		LogMessage( LOG_FILE, "Creating Weapon: %s", unitData->szName );
	}

	EVENT_DATA tNextEventData;
	tNextEventData.m_Data2 = nTicks;
	if ( nModAffix != INVALID_ID )
	{
		tNextEventData.m_Data1 = nItemClass;
		tNextEventData.m_Data3 = nModAffix + 1;
	} else {
		tNextEventData.m_Data1 = nItemClass + 1;
		tNextEventData.m_Data3 = INVALID_ID;
	}
	tNextEventData.m_Data4 = nMonsterClass;
	tNextEventData.m_Data5 = bTestAll;

	if ( !pItem )
	{
		LogMessage( LOG_FILE, "Bad item/combo to test" );
		sTestWeapon( game, unit, tNextEventData );
	}
	else
	{
		if ( nModAffix != INVALID_ID )
			ConsoleString( CONSOLE_SYSTEM_COLOR, "Testing Weapon: %s - %s", unitData->szName, pAffixData->szName );
		else
			ConsoleString( CONSOLE_SYSTEM_COLOR, "Testing Weapon: %s", unitData->szName );

		int nTestTicks = sGetTestTicks( game, unit );
		nTicks = nTestTicks ? nTestTicks : nTicks;

		UnitRegisterEventTimed( unit, sTestWeapon, &tNextEventData, nTicks );
	}

	return TRUE;
}
#endif // !ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL sFireWeapon(
	GAME * pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
#if !ISVERSION(SERVER_VERSION)
	if ( IsUnitDeadOrDying( pUnit ) )
		return FALSE;

	ASSERT_RETFALSE( IS_CLIENT( pGame ) );
	int nCount = (int)event_data.m_Data1;
	if ( nCount == 0 )
		return FALSE;

	int nTicks = sGetTestTicks( pGame, pUnit );
	nTicks = nTicks ? nTicks : (int)event_data.m_Data2;

	int nSkill = UnitGetStat( pUnit, STATS_SKILL_LEFT );
	SkillStopAllRequests( pUnit );
	c_SkillControlUnitRequestStartSkill( pGame, nSkill );
	UnitRegisterEventTimed( pUnit, sFireWeapon, EVENT_DATA( nCount - 1, nTicks ), nTicks );
#endif
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL sDroneWeaponsTest(
	GAME * game,
	UNIT* player,
	const struct EVENT_DATA& event_data)
{	
	static const unsigned int TIME_BETWEEN_WEAPON_TESTS = 60;
	int nMonsterClass = GlobalIndexGet( GI_MONSTER_TURRET_FLYING );
	UNIT * unit = UnitGetFirstByClassSearchAll( game, GENUS_MONSTER, nMonsterClass ); 
	if (!unit)
		return FALSE;

	if (IS_SERVER( game ) )
	{
		UnitUnregisterTimedEvent( unit, sTestWeapon );
	} 
	else
	{
		UnitUnregisterTimedEvent( unit, sFireWeapon );
	}
	SkillStopAllRequests( unit );

	int firstWeapon = (int)event_data.m_Data1;
	if (firstWeapon == (int)INVALID_LINK)
	{
		firstWeapon = sGetFirstWeapon(game);
		if (firstWeapon == (int)INVALID_LINK)
			return FALSE;
	}

	BOOL testModAffixes = FALSE;
	//BOOL testMonsters = FALSE;
	BOOL testAll = FALSE;

	GameSetDebugFlag(game, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS, TRUE);

	if (IS_SERVER(game))
	{
		int modAffix = testModAffixes ? 0 : INVALID_ID;
		int monsterClass = 	GlobalIndexGet( GI_MONSTER_ZOMBIE );

		sTestWeapon(game, unit, EVENT_DATA(firstWeapon, TIME_BETWEEN_WEAPON_TESTS, modAffix, monsterClass, testAll));
	}

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL sDroneWeaponsTestMakeDrone(
	GAME * game,
	UNIT* player,
	const struct EVENT_DATA& event_data)
{
	if (IS_SERVER(game))
	{
		unsigned int numSkills = ExcelGetNumRows(game, DATATABLE_SKILLS);
		for (unsigned int nSkill = 0; nSkill < numSkills; ++nSkill)
		{
			for ( int i = 0; i < 30; i++ )
			{
				SkillPurchaseLevel( player, nSkill, FALSE, TRUE );
			}
		} 
	}
	
	UnitSetStat(player, STATS_POWER_CUR, UnitGetStat(player, STATS_POWER_MAX));

	int nSkill = GlobalIndexGet( GI_SKILLS_PLAYER_MAKE_TURRET );
	SkillClearCooldown(game, player, NULL, nSkill);
	if ( SkillStartRequest( game, player, nSkill, INVALID_ID, VECTOR(0), 0, SkillGetNextSkillSeed( game ) ) )
	{
		UnitRegisterEventTimed( player, sDroneWeaponsTest, event_data, 80 );
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL sDroneWeaponsTestWaitForEngineer(
	GAME * game,
	UNIT* player,
	const struct EVENT_DATA& event_data)
{
	// the engineer should be ready by now, everyone cheer! =p

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game))		
	{
		c_CameraSetViewType(VIEW_VANITY, TRUE); // why doesn't this work?
		//c_CameraRestoreViewType();
	}
#endif //!ISVERSION(SERVER_VERSION)

	sDroneWeaponsTestMakeDrone(game, player, event_data);
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdWeaponsTest)
{
	static const unsigned int TIME_BETWEEN_WEAPON_TESTS = 60;

	UNIT * unit = sGetControlUnit(game, client);
	ASSERT_RETVAL(unit, CR_FAILED);

	if (IS_SERVER( game ) )
	{
		UnitUnregisterTimedEvent( unit, sTestWeapon );
	} 
	else
	{
		UnitUnregisterTimedEvent( unit, sFireWeapon );
	}
	SkillStopAllRequests( unit );

	int firstWeapon = sGetFirstWeapon(game);
	if (firstWeapon == (int)INVALID_LINK)
	{
		return CR_FAILED;
	}

	BOOL testModAffixes = FALSE;
	BOOL testMonsters = FALSE;
	BOOL testAll = FALSE;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrICmp(strToken, L"all") == 0)
		{
			testAll = TRUE;
		}
		else if (PStrICmp(strToken, L"stop") == 0)
		{
			return CR_OK;
		}
		else if (sConsoleExcelLookup(game, DATATABLE_ITEMS, strToken, arrsize(strToken), firstWeapon))
		{
			firstWeapon = sGetFirstWeapon(game, firstWeapon);
			if (firstWeapon == INVALID_LINK)
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid starting weapon [%s].", strToken);
				return CR_FAILED;
			}
		}
		else if (strToken[0] == L'a')
		{
			testModAffixes = TRUE;
		}
		else if (strToken[0] == L'm')
		{
			testMonsters = TRUE;
		}
	}

	GameSetDebugFlag(game, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS, TRUE);

	if (IS_SERVER(game))
	{
		int modAffix = testModAffixes ? 0 : INVALID_ID;
		int monsterClass = testMonsters ? 0 : INVALID_ID;

		sTestWeapon(game, unit, EVENT_DATA(firstWeapon, TIME_BETWEEN_WEAPON_TESTS, modAffix, monsterClass, testAll));
	}
	else if (IS_CLIENT(game))
	{
		int nSkillToTest = UnitGetStat( unit, STATS_SKILL_LEFT );
		unsigned int testCount = sCountTestWeapons(game, testAll, firstWeapon, nSkillToTest );

		if (testModAffixes)
		{
			unsigned int modAffixCount = sCountTestModAffixes(game);
			testCount *= modAffixCount;
		}

		if (testMonsters)
		{
			unsigned int monsterCount = sCountTestMonsters(game);
			testCount *= monsterCount;
		}
		UnitRegisterEventTimed(unit, sFireWeapon, EVENT_DATA(testCount, 5), 1);
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdWeaponsTestDrone)
{
	static const unsigned int TIME_BETWEEN_WEAPON_TESTS = 60;
	UNIT * player = sGetControlUnit(game, client);
	int nMonsterClass = GlobalIndexGet( GI_MONSTER_TURRET_FLYING );
	UNIT * drone = UnitGetFirstByClassSearchAll( game, GENUS_MONSTER, nMonsterClass );

	int firstWeapon = INVALID_LINK;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrICmp(strToken, L"stop") == 0)
		{
			UnitUnregisterTimedEvent(player, sDroneWeaponsTest);
			if ( drone )
				UnitUnregisterTimedEvent(drone, sTestWeapon);
			return CR_OK;
		}
		else if (sConsoleExcelLookup(game, DATATABLE_ITEMS, strToken, arrsize(strToken), firstWeapon))
		{
			firstWeapon = sGetFirstWeapon(game, firstWeapon);
			if (firstWeapon == INVALID_LINK)
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid starting weapon [%s].", strToken);
				return CR_FAILED;
			}
		}
	}

	// test weapons on the engineer drone
	if ( drone )
	{
		if (IS_SERVER(game))
			sDroneWeaponsTest( game, player, EVENT_DATA(firstWeapon) );
	}
	else if ( UnitGetType( player ) == UNITTYPE_ENGINEER_PLAYER ) 
	{
		sDroneWeaponsTestMakeDrone(game, player, EVENT_DATA(firstWeapon));
	}
	else
	{
		// make an engineer
		if (IS_SERVER(game))
		{
			int playerClass;
			const WCHAR *playerClassString = L"engineer_male";
			if (!sConsoleExcelLookup(game, DATATABLE_PLAYERS, playerClassString, CONSOLE_MAX_STR, playerClass))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid player class [%s].", playerClassString);
				return CR_FAILED;
			}

			WCHAR strName[MAX_CHARACTER_NAME];
			s_sGetDefaultPlayerName(strName, arrsize(strName));
			if (!s_MakeNewPlayer(game, client, playerClass, strName))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Error generating new player.");
				return CR_FAILED;
			}
		}

		UNIT * player = sGetControlUnit(game, client);
		ASSERT_RETVAL(player, CR_FAILED);

		UnitRegisterEventTimed( player, sDroneWeaponsTestWaitForEngineer, EVENT_DATA(firstWeapon), 120);

	}

	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdWeaponNext)
{
	UNIT * pUnit = ClientGetControlUnit( client );
	if ( ! pUnit ) 
		return CR_FAILED;

	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe == INVALID_ID )
		return CR_FAILED;

	int nItemClass = INVALID_ID;
	{
		UNIT * pWeaponCurr = WardrobeGetWeapon( game, nWardrobe, 0 );
		if ( pWeaponCurr )
		{
			nItemClass = UnitGetClass( pWeaponCurr ) + 1;
			const UNIT_DATA * pUnitData = ItemGetData( nItemClass );
			if ( ! UnitIsA( pUnitData->nUnitType, UNITTYPE_WEAPON ) )
				nItemClass = INVALID_ID;
		} 
		
		if ( nItemClass == INVALID_ID )
		{ // this helps it loop
			nItemClass = ItemGetByUnitType( game, UNITTYPE_WEAPON );
		}

		// make sure that we get an item labeled for spawning
		int nNumItems = ExcelGetNumRows( game, DATATABLE_ITEMS );
		for ( ; nItemClass < nNumItems; nItemClass++ )
		{
			const UNIT_DATA * pUnitData = ItemGetData( nItemClass );
			if ( ! pUnitData )
				continue;

			if ( ! UnitIsA( pUnitData->nUnitType, UNITTYPE_WEAPON ) )
			{
				nItemClass = ItemGetByUnitType( game, UNITTYPE_WEAPON );
				break;
			}
			if ( UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_SPAWN ) )
				break;
		}

 	}
	if ( nItemClass == INVALID_ID )
		return CR_FAILED;

	s_sSpawnAndEquipWeapon( pUnit, nItemClass );
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL sGetFirstUsablePlayerClass(GAME * game, int & playerClass)
{
	int numPlayerClasses = ExcelGetNumRows(game, DATATABLE_PLAYERS);
	// get the first valid player class
	while (playerClass < numPlayerClasses && !PlayerGetData(game, playerClass))
		++playerClass;

	return playerClass < numPlayerClasses;
}
#endif 

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static UNIT * s_sMakeNewPlayerForArmorTest(
	GAME * game,
	GAMECLIENT * client,
	int playerClass)
{
	WCHAR strName[MAX_CHARACTER_NAME];
	s_sGetDefaultPlayerName(strName, arrsize(strName));
	if (!s_MakeNewPlayer(game, client, playerClass, strName))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Error generating new player.");
		return NULL;
	}

	UNIT * player = sGetControlUnit(game, client);

	// make sure the player will all the requirements, other than class requirements
	int level = UnitGetExperienceLevel(player);
	int maxLevel = UnitGetMaxLevel( player );
	if (level < maxLevel)
		PlayerDoLevelUp(player, maxLevel);

	UnitSetStat( player, STATS_ACCURACY, 500 );
	UnitSetStat( player, STATS_WILLPOWER, 500 );
	UnitSetStat( player, STATS_STAMINA, 500 );
	UnitSetStat( player, STATS_STRENGTH, 500 );

	return player;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL s_sTestArmor(
	GAME * game,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
#if !ISVERSION(SERVER_VERSION)
	// show the inventory screen, to better see the items
	if (!UIIsInventoryPackPanelUp())
		UIShowInventoryScreen();
#endif // !ISVERSION(SERVER_VERSION)

	int nItemClass = (int)event_data.m_Data1;
	int timeBetweenArmorTests = (int)event_data.m_Data2;
	GAMECLIENT * client = UnitGetClient(unit);

	ITEM_SPEC spec;
	spec.nLevel = UnitGetExperienceLevel(unit);
	spec.nLevel = MAX(spec.nLevel, 1);

	BOOL bElementEffects[MAX_DAMAGE_TYPES];
	memclear(bElementEffects, arrsize(bElementEffects) * sizeof(BOOL));

	int nNumItems = ExcelGetNumRows( game, DATATABLE_ITEMS );
	const UNIT_DATA * unitData = ItemGetData( nItemClass );
	BOOL testItem = TRUE;
	while ( !unitData ||
			(! UnitIsA( unitData->nUnitType, UNITTYPE_ARMOR ) &&
			 ! UnitIsA( unitData->nUnitType, UNITTYPE_CLOTHING ) ) ||
			 ! InventoryItemMeetsClassReqs( unitData, unit ) )
	{
		nItemClass++;
		if ( nItemClass >= nNumItems )
		{
			testItem = FALSE;
			break;
		}
		unitData = ItemGetData( nItemClass );
	}

	if (testItem)
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, "Testing Armor: %s", unitData->szName );
		LogMessage( LOG_FILE, "Testing Armor: %s", unitData->szName );

		spec.nItemClass = nItemClass;
		spec.pSpawner = unit;

		DWORD dwFlags = ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT | ITEM_SPAWN_CHEAT_FLAG_PICKUP | ITEM_SPAWN_CHEAT_FLAG_FORCE_EQUIP | ITEM_SPAWN_CHEAT_FLAG_ADD_MODS;

		s_ItemSpawnForCheats( game, spec, client, dwFlags, bElementEffects );
	}

	if ( nItemClass + 1 < nNumItems )
	{
		UnitRegisterEventTimed( unit, s_sTestArmor, EVENT_DATA( nItemClass + 1, timeBetweenArmorTests ), timeBetweenArmorTests );
	}
	else 
	{
		// start over with a new player class
		// get the next valid player class

#if !ISVERSION(SERVER_VERSION)
		// close the inv screen if done, and if switching characters to fix camera
		UIComponentActivate(UICOMP_INVENTORY, FALSE);
		
#endif // !ISVERSION(SERVER_VERSION)

		int playerClass = UnitGetClass(unit) + 1;
		if (sGetFirstUsablePlayerClass(game, playerClass))
		{
			UNIT * nextPlayer = s_sMakeNewPlayerForArmorTest(game, client, playerClass);
			if (nextPlayer)
			{
				UnitRegisterEventTimed( nextPlayer, s_sTestArmor, EVENT_DATA( 0, timeBetweenArmorTests ), 80 );
			}
		}
	}

	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static int x_sCmdParsePlayerClass(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * & strParams)
{
	ASSERT_RETINVALID(game && (client || IS_CLIENT(game)) && cmd && strParams);

	int playerClass;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseExcelLookupOrName(game, cmd, DATATABLE_PLAYERS, strParams, strToken, arrsize(strToken), playerClass))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"player class", strToken);	
		return INVALID_LINK;
	}
	return playerClass;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdArmorTest)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	// for each player class, try on all the armor that meets class restrictions.
	// start with the class specified on the command line, if any.
	// the test armor function will take care of the rest of the player classes

	int playerClass = x_sCmdParsePlayerClass(game, client, cmd, strParams);
	if (playerClass == INVALID_LINK)
		playerClass = 0;

	int delayBetweenItems = 20;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrIsNumeric(strToken))
		{
			delayBetweenItems = PStrToInt(strToken);
		}
	}

	delayBetweenItems = MAX(1, delayBetweenItems);

	if (sGetFirstUsablePlayerClass(game, playerClass))
	{
		UNIT * player = s_sMakeNewPlayerForArmorTest(game, client, playerClass);
		if (player)
			s_sTestArmor(game, player, EVENT_DATA(0, delayBetweenItems));
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdArmorTestStop)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = sGetControlUnit(game, client);
	ASSERT_RETVAL(player, CR_FAILED);

	UnitUnregisterTimedEvent(player, s_sTestArmor);

#if !ISVERSION(SERVER_VERSION)
	// close the inv screen if done, and if switching characters to fix camera
	UIComponentActivate(UICOMP_INVENTORY, FALSE);

#endif // !ISVERSION(SERVER_VERSION)

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdArmorRandom)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);
	UNIT * player = ClientGetControlUnit(client);
	if ( !s_ItemGenerateRandomArmor( player, TRUE ) )
		return CR_FAILED;
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdMonsterLevel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int level;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, level) || level <= 0 || level > MonsterGetMaxLevel(game))
	{
		game->m_nDebugMonsterLevel = 0;
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Monster level override: DEFAULT.");
		return CR_OK;
	}

	game->m_nDebugMonsterLevel = level;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Monster level override: %d.", game->m_nDebugMonsterLevel);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static ROOM_ITERATE_UNITS s_sMonsterUpgradeToLevel(
	UNIT *pUnit,
	void *pCallbackData)
{
	ASSERT_RETVAL(pUnit, RIU_CONTINUE);
	ASSERT_RETVAL(pCallbackData, RIU_STOP);

	if(!UnitIsA(pUnit, UNITTYPE_MONSTER))
	{
		return RIU_CONTINUE;
	}

	EVENT_DATA * pEventData = (EVENT_DATA *)pCallbackData;
	int nNewMonsterLevel = (int)pEventData->m_Data1;
	int nCurMonsterLevel = UnitGetExperienceLevel(pUnit);
	if(nNewMonsterLevel > nCurMonsterLevel)
	{
		MONSTER_SPEC tSpec;
		MonsterSpecInit( tSpec );
		tSpec.nExperienceLevel = nNewMonsterLevel;
		tSpec.nClass = UnitGetClass( pUnit );
		tSpec.nMonsterQuality = MonsterGetQuality( pUnit );
		tSpec.dwFlags = MSF_IS_LEVELUP;
		s_MonsterInitStats( UnitGetGame(pUnit), pUnit, tSpec );

		UNIT * pItem = UnitInventoryIterate(pUnit, NULL);
		while(pItem)
		{
			ITEM_SPEC tItemSpec;
			SETBIT(tItemSpec.dwFlags, ISF_MONSTER_INVENTORY_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_SKIP_REQUIRED_AFFIXES_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_SKIP_HAND_CRAFTED_AFFIXES_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_RESTRICT_AFFIX_LEVEL_CHANGES_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_SKIP_QUALITY_AFFIXES_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_SKIP_PROPS);
			tItemSpec.nQuality = ItemGetQuality(pItem);
			s_ItemInitProperties(UnitGetGame(pUnit), pItem, pUnit, UnitGetClass(pItem), UnitGetData(pItem), &tItemSpec);
			pItem = UnitInventoryIterate(pUnit, pItem);
		}
	}
	return RIU_CONTINUE;
}

CONSOLE_CMD_FUNC(s_sCmdMonsterUpgrade)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int level = -1;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, level) || level <= 0 || level > MonsterGetMaxLevel(game))
	{
		UNIT * pControlUnit = ClientGetControlUnit(client);
		if(pControlUnit)
		{
			level = UnitGetExperienceLevel(pControlUnit);
		}
	}

	if(level > 0)
	{
		UNIT * pControlUnit = ClientGetControlUnit(client);
		if(pControlUnit)
		{
			LEVEL * pLevel = UnitGetLevel(pControlUnit);
			if(pLevel)
			{
				TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
				EVENT_DATA tEventData(level);
				LevelIterateUnits(pLevel, eTargetTypes, s_sMonsterUpgradeToLevel, &tEventData);
				ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Monster level upgraded: %d.", level);
			}
		}
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdConversationUpdateRate)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
#if !ISVERSION(SERVER_VERSION)
	int nSpeed;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nSpeed))
	{
		return CR_OK;
	}

	extern int CONVERSATION_UPDATE_DELAY_IN_MS;
	CONVERSATION_UPDATE_DELAY_IN_MS = nSpeed;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Conversation text update rate: %d.", CONVERSATION_UPDATE_DELAY_IN_MS);
#endif	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdConversationSpeed)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	int nNumChars;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nNumChars))
	{
		return CR_OK;
	}

	extern int CONVERSATION_CHAR_GROUP_SIZE;
	CONVERSATION_CHAR_GROUP_SIZE = nNumChars;
	CONVERSATION_CHAR_GROUP_SIZE = PIN( CONVERSATION_CHAR_GROUP_SIZE, 1, 1000 );
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Conversation text speed: %d.", CONVERSATION_CHAR_GROUP_SIZE);
#endif	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdItemLevel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int level;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, level) || level <= 0 || level > (int)ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_ITEM_LEVELS))
	{
		game->m_nDebugItemFixedLevel = 0;
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Item level override: DEFAULT.");
		return CR_OK;
	}

	game->m_nDebugItemFixedLevel = level;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Item level override: %d.", game->m_nDebugItemFixedLevel);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
// Note: Make sure this command isn't in our beta or production binaries.
// It's basically designed to exploit security holes so we can find them.
// Sends a message with whatever command is desired.  If no content is
// specified, fills with blank garbage.
// Added generalized "byte by byte" parameters, entered in 2 digit hex.  throw away more than 2 digits.
// Might want to just parse every 2 letters, but meh.  I like hitting spacebar, keeps me grounded.
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSendMessage)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	int command;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, command))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing message command id.");
		return CR_FAILED;
	}

	const int msgSize = MAX_SMALL_MSG_SIZE;	//arbitrary value rather than looking up which size corresponds to this message.
	BYTE buf[MAX_SMALL_MSG_SIZE];
	MSG_STRUCT * msg = (MSG_STRUCT *)buf;
	msg->hdr.cmd = (NET_CMD)command;

	// while there are more hex arguments, put them in the msg struct starting with i=4.
	// realize the size and header get overwritten on a send, to the cmd number.
	strParams = PStrSkipWhitespace(strParams, CONSOLE_MAX_STR);
	const WCHAR * strNext = strParams;
	DWORD val;
	unsigned int cur = 4;
	while (cur < msgSize && PStrHexToInt(strParams, CONSOLE_MAX_STR, val, &strNext))
	{
		strParams = PStrSkipWhitespace(strNext, CONSOLE_MAX_STR);

		buf[cur] = (BYTE)val;
		++cur;
	}
	c_SendMessage((NET_CMD)command, msg);

	return CR_OK;
}


//----------------------------------------------------------------------------
// Chat server version of above message.
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(c_sCmdSendChatMessage)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	int command;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, command))
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_MISSING_MESSAGE_COMMAND_ID ));
		return CR_FAILED;
	}

	const int msgSize = MAX_SMALL_MSG_SIZE;	//arbitrary value rather than looking up which size corresponds to this message.
	BYTE buf[MAX_SMALL_MSG_SIZE];
	MSG_STRUCT * msg = (MSG_STRUCT *)buf;
	msg->hdr.cmd = (NET_CMD)command;

	// while there are more hex arguments, put them in the msg struct starting with i=4.
	// realize the size and header get overwritten on a send, to the cmd number.
	strParams = PStrSkipWhitespace(strParams, CONSOLE_MAX_STR);
	const WCHAR * strNext = strParams;
	DWORD val;
	unsigned int cur = 4;
	while (cur < msgSize && PStrHexToInt(strParams, CONSOLE_MAX_STR, val, &strNext))
	{
		strParams = PStrSkipWhitespace(strNext, CONSOLE_MAX_STR);

		buf[cur] = (BYTE)val;
		++cur;
	}
	c_ChatNetSendMessage(msg, (NET_CMD)command);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
// Game server to client version of above command.
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSendMessageToClient)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	int command;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, command))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing message command id.");
		return CR_FAILED;
	}

	const int msgSize = MAX_SMALL_MSG_SIZE;	//arbitrary value rather than looking up which size corresponds to this message.
	BYTE buf[MAX_SMALL_MSG_SIZE];
	MSG_STRUCT * msg = (MSG_STRUCT *)buf;
	msg->hdr.cmd = (NET_CMD)command;

	// while there are more hex arguments, put them in the msg struct starting with i=4.
	// realize the size and header get overwritten on a send, to the cmd number.
	strParams = PStrSkipWhitespace(strParams, CONSOLE_MAX_STR);
	const WCHAR * strNext = strParams;
	DWORD val;
	unsigned int cur = 4;
	while (cur < msgSize && PStrHexToInt(strParams, CONSOLE_MAX_STR, val, &strNext))
	{
		strParams = PStrSkipWhitespace(strNext, CONSOLE_MAX_STR);

		buf[cur] = (BYTE)val;
		++cur;
	}
	s_SendMessage(game, ClientGetId(client), (NET_CMD)command, msg);

	return CR_OK;
}

//----------------------------------------------------------------------------
// Event to repeatedly send random messages to the client
//----------------------------------------------------------------------------
#define RANDOM_MESSAGE_INTERVAL 4
static const NET_CMD cmdDisallowedForRandomMessages[] =
{	//These cause the client to disconnect or freeze for legitimate reasons!
	SCMD_ERROR,
	SCMD_GAME_NEW,
	SCMD_GAME_CLOSE,
	SCMD_START_LEVEL_LOAD,
	SCMD_SET_LEVEL,
	SCMD_SET_CONTROL_UNIT
};

static BOOL sSCMDIsPrivleged(NET_CMD cmd)
{
	for(int i = 0; i < ARRAYSIZE(cmdDisallowedForRandomMessages); i++)
	{
		if(cmd == cmdDisallowedForRandomMessages[i])
			return TRUE;
	}
	return FALSE;
}

static BOOL SendRandomMessageToClient(
	GAME * game,
	UNIT *,
	const EVENT_DATA &event_data)
{
	GAMECLIENTID idClient = (GAMECLIENTID)event_data.m_Data1 + 
		(GAMECLIENTID(event_data.m_Data2) << 32); //64-bit GameclientID split in two.
	ASSERT_RETFALSE(idClient != INVALID_ID);

	GAMECLIENT *pClient = ClientGetById(game, idClient);
	if(!pClient || ClientGetState(pClient) == GAME_CLIENT_STATE_FREE) return FALSE;

	NET_CMD cmd = (NET_CMD)RandGetNum(game, SCMD_LAST);
	if(sSCMDIsPrivleged(cmd)) cmd = 0; //hope 0 is never priv :)
	unsigned int nMessageSize = 100; //arbitrary.  Theoretically, we can look it up from the message table.

	BYTE			buf_fake_msg[ MAX_LARGE_MSG_SIZE ];
	if(RandGetNum(game, 2)) memclear(buf_fake_msg, sizeof(buf_fake_msg));

	float fAlterationProbability = RandGetFloat(game, 0.0f, 1.0f);
	for(unsigned int i = 0; i < nMessageSize; i++)
	{//Set the msg buf to as much garbage as we like.
		if(RandGetFloat(game, 0.0f, 1.0f) < fAlterationProbability)
		{
			buf_fake_msg[i] = (BYTE)RandGetNum(game, 256);
		}
	}

	MSG_STRUCT *msg = (MSG_STRUCT *) buf_fake_msg;

	s_SendMessage(game, idClient, cmd, msg);

	GameEventRegister(game, SendRandomMessageToClient, NULL, NULL, event_data, RANDOM_MESSAGE_INTERVAL);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdSendRandomMessagesToClient)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	EVENT_DATA event_data( DWORD(client->m_idGameClient & 0xffffffff), DWORD(client-> m_idGameClient >> 32 ));
	GameEventRegister(game, SendRandomMessageToClient, NULL, NULL, event_data, RANDOM_MESSAGE_INTERVAL);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
// Repeatedly send random chat messages to the client.  Console command
// is implemented on the clientside.
//----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION) && ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION) //consistency with definition of chat debug msg
#define RANDOM_CHAT_MESSAGE_INTERVAL 4 //game ticks, i.e. 20/4 = 5 times/sec

static BOOL SendRandomChatMessageToClient(
	GAME * game,
	UNIT *,
	const EVENT_DATA &)
{
	ASSERT_RETFALSE(IS_CLIENT(game));
	
	NET_CMD cmd = (NET_CMD)RandGetNum(game, CHAT_USER_RESPONSE_COUNT);

	CHAT_REQUEST_MSG_SEND_ARBITRARY_MESSAGE msg;
	msg.bNetCmd = cmd;

	unsigned int nMessageSize = 100; //arbitrary.  Theoretically, we can look it up from the message table.

	BYTE			buf_fake_msg[ MAX_LARGE_MSG_SIZE ];
	if(RandGetNum(game, 2)) memclear(buf_fake_msg, sizeof(buf_fake_msg));

	float fAlterationProbability = RandGetFloat(game, 0.0f, 1.0f);
	for(unsigned int i = 0; i < nMessageSize; i++)
	{//Set the msg buf to as much garbage as we like.
		if(RandGetFloat(game, 0.0f, 1.0f) < fAlterationProbability)
		{
			buf_fake_msg[i] = (BYTE)RandGetNum(game, 256);
		}
	}

	ASSERT(MemoryCopy(msg.buf, sizeof(msg.buf), buf_fake_msg, nMessageSize));
	MSG_SET_BLOB_LEN(msg, buf, nMessageSize);

	c_ChatNetSendMessage(&msg, USER_REQUEST_SEND_ARBITRARY_MESSAGE);

	GameEventRegister(game, SendRandomChatMessageToClient, NULL, NULL, NULL, RANDOM_CHAT_MESSAGE_INTERVAL);
	return TRUE;
}

CONSOLE_CMD_FUNC(c_sCmdSendRandomChatMessagesToClient)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	GameEventRegister(game, SendRandomChatMessageToClient, NULL, NULL, NULL, RANDOM_CHAT_MESSAGE_INTERVAL);

	return CR_OK;
}
#endif //DEBUG_VERSION && DEVELOPMENT

//----------------------------------------------------------------------------
// trace message counts
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdNetMsgCount)
{
	ASSERT_RETVAL(game, CR_FAILED);

	TIME time = AppCommonGetCurTime();
	TraceGameInfo("NET MSG COUNTS --  total time: %I64d.%03d sec", INT64(time / MSECS_PER_SEC), int(time % MSECS_PER_SEC));
	if (gApp.m_GameClientCommandTable)
	{
		TraceGameInfo("Client to game server messages:");
		NetCommandTraceMessageCounts(gApp.m_GameClientCommandTable);
	}
	if (gApp.m_GameServerCommandTable)
	{
		TraceGameInfo("Game server to client messages:");
		NetCommandTraceMessageCounts(gApp.m_GameServerCommandTable);
	}
	if (gApp.m_GameLoadBalanceClientCommandTable)
	{
		TraceGameInfo("Client to load-balance server messages:");
		NetCommandTraceMessageCounts(gApp.m_GameLoadBalanceClientCommandTable);
	}
	if (gApp.m_GameLoadBalanceServerCommandTable)
	{
		TraceGameInfo("Load-balance server to client messages:");
		NetCommandTraceMessageCounts(gApp.m_GameLoadBalanceServerCommandTable);
	}
	if (gApp.m_ChatClientCommandTable)
	{
		TraceGameInfo("Client to chat server messages:");
		NetCommandTraceMessageCounts(gApp.m_ChatClientCommandTable);
	}
	if (gApp.m_ChatServerCommandTable)
	{
		TraceGameInfo("Chat server to client messages:");
		NetCommandTraceMessageCounts(gApp.m_ChatServerCommandTable);
	}
	if (gApp.m_PatchClientCommandTable)
	{
		TraceGameInfo("Client to patch server messages:");
		NetCommandTraceMessageCounts(gApp.m_PatchClientCommandTable);
	}
	if (gApp.m_PatchServerCommandTable)
	{
		TraceGameInfo("Patch server to client messages:");
		NetCommandTraceMessageCounts(gApp.m_PatchServerCommandTable);
	}

	return CR_OK;
}


CONSOLE_CMD_FUNC(c_sCmdNetMsgCountReset)
{
	ASSERT_RETVAL(game, CR_FAILED);

	TraceGameInfo("RESET NET MSG COUNTS");
	if (gApp.m_GameClientCommandTable)
		NetCommandResetMessageCounts(gApp.m_GameClientCommandTable);
	if (gApp.m_GameServerCommandTable)
		NetCommandResetMessageCounts(gApp.m_GameServerCommandTable);
	if (gApp.m_GameLoadBalanceClientCommandTable)
		NetCommandResetMessageCounts(gApp.m_GameLoadBalanceClientCommandTable);
	if (gApp.m_GameLoadBalanceServerCommandTable)
		NetCommandResetMessageCounts(gApp.m_GameLoadBalanceServerCommandTable);
	if (gApp.m_ChatClientCommandTable)
		NetCommandResetMessageCounts(gApp.m_ChatClientCommandTable);
	if (gApp.m_ChatServerCommandTable)
		NetCommandResetMessageCounts(gApp.m_ChatServerCommandTable);
	if (gApp.m_BillingClientCommandTable)
		NetCommandResetMessageCounts(gApp.m_BillingClientCommandTable);
	if (gApp.m_BillingServerCommandTable)
		NetCommandResetMessageCounts(gApp.m_BillingServerCommandTable);
	if (gApp.m_PatchClientCommandTable)
		NetCommandResetMessageCounts(gApp.m_PatchClientCommandTable);
	if (gApp.m_PatchServerCommandTable)
		NetCommandResetMessageCounts(gApp.m_PatchServerCommandTable);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
// Display latency tracker.
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(c_sCmdPing)
{ 
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_PING, -1);

	return CR_OK;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdSfxChanceOverride)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	extern int gnSfxChanceOverride;

	int sfxchance;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, sfxchance))
	{
		gnSfxChanceOverride = 0;
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"SFX Chance Override: OFF");
		return CR_OK;
	}

	gnSfxChanceOverride = PIN(sfxchance, 0, 100);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"SFX Chance override: %d", gnSfxChanceOverride);
	return CR_OK;
}
#endif 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdKeyHandlerLoggingToggle)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	gApp.m_bKeyHandlerLogging = !gApp.m_bKeyHandlerLogging;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Key handler logging: %s", sGetOnOffString(gApp.m_bKeyHandlerLogging));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sDoPetDamage(
	UNIT * unit,
	void * data)
{
	ASSERT_RETURN(unit);
	UnitSetStat(unit, STATS_HP_CUR, 1);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPetsDamage)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	PetIteratePets(player, sDoPetDamage, NULL);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sDoPetKill(
	UNIT * unit,
	void * data)
{
	UnitDie(unit, unit);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdPetsKill)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	PetIteratePets(player, sDoPetKill, NULL);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdStopPathing)
{
	ASSERT_RETVAL(game, CR_FAILED);

	UNIT * pDebugUnit = GameGetDebugUnit(game);
	if(!pDebugUnit)
	{
		return CR_FAILED;
	}

	UnitStopPathing(pDebugUnit);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdClearWeapons)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	UNIT * item = UnitInventoryIterate(player, NULL);
	UNIT * next = NULL;
	while (item)
	{
		next = UnitInventoryIterate(player, item);
		if (UnitIsA(item, UNITTYPE_WEAPON))
		{
			UnitFree(item, UFF_SEND_TO_CLIENTS);
		}
		item = next;
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
ROOM * sFindFirstHellriftRoom(
	LEVEL * level,
	ROOM * playerRoom)
{
	ROOM * room = LevelGetFirstRoom(level);
	ROOM * best = NULL;

	if (playerRoom->nNumVisualPortals > 0)
	{
		for (; room; room = LevelGetNextRoom(room))
		{
			if (room->nNumVisualPortals <= 0)
			{
				continue;
			}
			if (playerRoom == room)
			{
				break;
			}
			best = room;
		}
	}
	for (; room; room = LevelGetNextRoom(room))
	{
		if (room->nNumVisualPortals <= 0)
		{
			continue;
		}
		return room;
	}
	return best;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static BOOL sWarpPlayerToPortal(
	GAME * game,
	GAMECLIENT * client,
	UNIT * player,
	LEVEL * level,
	UNIT * portal,
	ROOM * portalRoom)
{
	ASSERT_RETFALSE(portal);

	VECTOR portalPosition = UnitGetPosition(portal);
				
	// find a pathnode a little bit away from the warp
	FREE_PATH_NODE tFreePathNode;
	NEAREST_NODE_SPEC tSpec;
	tSpec.fMinDistance = 5.0f;
	tSpec.fMaxDistance = 20.0f;
	int nodeCount = RoomGetNearestPathNodes(game, UnitGetRoom(player), portalPosition, 1, &tFreePathNode, &tSpec);
	ASSERT_RETFALSE(nodeCount == 1);

	VECTOR destination = RoomPathNodeGetExactWorldPosition(game, tFreePathNode.pRoom, tFreePathNode.pNode);

	// face toward warp
	VECTOR playerPosition = UnitGetPosition(player);
	VECTOR direction;
	VectorSubtract(direction, portalPosition, playerPosition);	
		
	// warp to position
	DWORD dwWarpFlags = 0;
	SETBIT(dwWarpFlags, UW_TRIGGER_EVENT_BIT);
	UnitWarp(player, tFreePathNode.pRoom, destination, direction, UnitGetVUpDirection(player), dwWarpFlags, TRUE);
	return TRUE;			
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdHellriftPortal)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	LEVEL * level = UnitGetLevel(player);
	ASSERT_RETVAL(level, CR_FAILED);

	ROOM * playerRoom = UnitGetRoom(player);
	ASSERT_RETVAL(playerRoom, CR_FAILED);

	ROOM * room = sFindFirstHellriftRoom(level, playerRoom);
	if (!room)
	{
		if (playerRoom->nNumVisualPortals > 0)
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"You are already in the only hellrift room found on the level.");
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"No hellrift room found on level.");
		}
		return CR_FAILED;
	}

	UNITID idPortal = room->pVisualPortals[0].idVisualPortal;
	UNIT * portal = UnitGetById(game, idPortal);
	ASSERT_RETVAL(portal, CR_FAILED);

	if (!sWarpPlayerToPortal(game, client, player, level, portal, room))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"error warping player to hellrift room.");
		return CR_FAILED;
	}
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdDisconnectPlayer)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing player to disconnect");
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	APPCLIENTID	idAppClient = ClientSystemFindByName(AppGetClientSystem(), strPlayer);
	if (idAppClient == INVALID_ID)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid player name [%s].", strPlayer);
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), idAppClient);
	if (idNetClient == INVALID_NETCLIENTID)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid player name [%s].", strPlayer);
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	//	send this first in case we are disconnecting the sender...
	s_SendSysTextFmt(CHAT_TYPE_WHISPER, TRUE, game, client, CONSOLE_SYSTEM_COLOR, L"Killing connection to player [%s].", strPlayer);

	GAMEID idGame = NetClientGetGameId(idNetClient);
	MSG_CCMD_PLAYERREMOVE msg;
	if (idGame == INVALID_ID) 
	{
		AppPostMessageToMailbox(idNetClient, CCMD_PLAYERREMOVE, &msg);
	}
	else 
	{
		SrvPostMessageToMailbox(idGame, idNetClient, CCMD_PLAYERREMOVE, &msg);
	}

	return CR_OK;
}
#endif
//----------------------------------------------------------------------------
// NOTE: Currently ignores the vagaries of saving and loading.
// Thus, we'll likely load on the new machine before we save on
// the old machine-- continuity is not preserved, and dupe bugs obvious.
//
// But then, this is purely for prototyping.
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdSwitchGameServer)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	//Get the NETCLIENTID64 from the gameclient
	//get the SVRINSTANCE from the message parameter

	int nServerInstance;
	sConsoleParseIntegerParam(cmd, 0, strParams, nServerInstance);
#if ISVERSION(SERVER_VERSION)
	NETCLIENTID64 idNetClient = ClientGetNetId(client);

	UNIT * unit = ClientGetPlayerUnit(client);

	//SWITCH!
	GameServerSendSwitchMessageUnsafe(idNetClient, nServerInstance, 
		unit->szName, game);
#endif
	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdWaypointsAll)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	s_LevelSetAllWaypoints(game, player);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTriggerReset)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int objectClass;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), objectClass))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"object class", strToken);
		return CR_FAILED;
	}

	ObjectTriggerReset(game, player, objectClass);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdResetGame)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	MSG_SCMD_GAME_CLOSE msg;
	msg.idGame = GameGetId(game);
	msg.bRestartingGame = TRUE;
	msg.bShowCredits = FALSE;
	s_SendMessage(game, ClientGetId(client), SCMD_GAME_CLOSE, &msg);
	GameSetState(game, GAMESTATE_REQUEST_SHUTDOWN);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdUnitsDebug)
{
	ASSERT_RETVAL(game, CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		GameTraceUnits(game, FALSE);
		return CR_OK;
	}

	if (IS_CLIENT(game) && PStrICmp(strToken[0], L'c'))
	{
		GameTraceUnits(game, FALSE);
		return CR_OK;
	}
	else if (IS_SERVER(game) && PStrICmp(strToken[0], L's'))
	{
		GameTraceUnits(game, FALSE);
		return CR_OK;
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdUnitsVerbose)
{
	ASSERT_RETVAL(game, CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		GameTraceUnits(game, TRUE);
		return CR_OK;
	}

	if (IS_CLIENT(game) && PStrICmp(strToken[0], L'c'))
	{
		GameTraceUnits(game, TRUE);
		return CR_OK;
	}
	else if (IS_SERVER(game) && PStrICmp(strToken[0], L's'))
	{
		GameTraceUnits(game, TRUE);
		return CR_OK;
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCheckString)
{
	ASSERT_RETVAL(game, CR_FAILED);

	if (!strParams || !strParams[0])
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Must provide a string key");
		return CR_FAILED;
	}

	char szKey[256];
	PStrCvt(szKey, strParams, arrsize(szKey));
	int nIndex = StringTableGetStringIndexByKey(szKey);
	if (nIndex == INVALID_LINK)
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"String not found for key [%s]", strParams);
		return CR_OK;
	}

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"String is: %s", StringTableGetStringByIndex(nIndex));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTagDisplay)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strTag[MAX_CONSOLE_PARAM_VALUE_LEN];
	int tag;
	sConsoleParseExcelLookup(game, cmd, 0, strParams, strTag, arrsize(strTag), tag);
	game->m_nDebugTagDisplay = tag;
#if !ISVERSION(SERVER_VERSION)
	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_TAGDISPLAY, -1);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Tag Debug: %s", sGetOnOffString(UIDebugDisplayGetStatus(UIDEBUG_TAGDISPLAY)));
#endif
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sAppSetUnitMetricsFlag(
	APP_FLAG eFlag,
	BOOL bValue)
{
	static const APP_FLAG sUnitMetricsFlags[] =
	{
		AF_UNITMETRICS,
		AF_UNITMETRICS_HCE_ONLY,
		AF_UNITMETRICS_PVP_ONLY,
	};

	for (int ii = 0; ii < arrsize(sUnitMetricsFlags); ++ii)
	{
		AppSetFlag( sUnitMetricsFlags[ii], FALSE );
	}
	AppSetFlag( eFlag, bValue );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdUnitMetricsToggle)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	struct PARAM_MATCH
	{
		const WCHAR *	szKey;
		APP_FLAG		eFlag;
		const WCHAR *	szDesc;
	};
	static const PARAM_MATCH sParamMatch[] =
	{
		{ L"all",		AF_UNITMETRICS,				L"Full" },
		{ L"hce",		AF_UNITMETRICS_HCE_ONLY,	L"Hardcore/Elite" },
		{ L"pvp",		AF_UNITMETRICS_PVP_ONLY,	L"PVP" },
	};

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		// display unit metrics status
		for (int ii = 0; ii < arrsize(sParamMatch); ++ii)
		{
			ConsoleString(
				game,
				client,
				CONSOLE_SYSTEM_COLOR,
				L"%s Unit Metrics %s",
				sParamMatch[ii].szDesc,
				sGetEnabledDisabledString( AppTestFlag(sParamMatch[ii].eFlag) ) );
		}
		return CR_OK;
	}
	else
	{
		for (int ii = 0; ii < arrsize(sParamMatch); ++ii)
		{
			if (PStrICmp(strToken, sParamMatch[ii].szKey, 3) == 0)
			{
				// toggle unit metrics status
				BOOL bUnitMetrics = AppTestFlag( sParamMatch[ii].eFlag );
				sAppSetUnitMetricsFlag( sParamMatch[ii].eFlag, !bUnitMetrics );
				ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"%s Unit Metrics %s", sParamMatch[ii].szDesc, sGetEnabledDisabledString(!bUnitMetrics));
				return CR_OK;
			}
		}
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdCombatLog)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	BOOL wasCombatTraceActive = GameGetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		GameToggleDebugFlag(game, DEBUGFLAG_COMBAT_TRACE);
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Combat Log: %s", sGetOnOffString(GameGetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE)));
		BOOL val = GameGetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE) ? TRUE : FALSE;
		for (unsigned int ii = COMBAT_TRACE_DAMAGE; ii < NUM_COMBAT_TRACE; ++ii)
		{
			GameSetCombatTrace(game, (COMBAT_TRACE_FLAGS)ii, val);
		}
		goto _done;
	}

	struct PARAM_MATCH
	{
		const WCHAR *		str;
		COMBAT_TRACE_FLAGS	flag;
	};
	static const PARAM_MATCH sParamMatch[] =
	{
		{ L"dmg",		COMBAT_TRACE_DAMAGE },
		{ L"sfx",		COMBAT_TRACE_SFX },
		{ L"kills",		COMBAT_TRACE_KILLS },
		{ L"evasion",	COMBAT_TRACE_EVASION },
		{ L"blocking",	COMBAT_TRACE_BLOCKING },
		{ L"shields",	COMBAT_TRACE_SHIELDS },
		{ L"armor",		COMBAT_TRACE_ARMOR },
		{ L"interrupt",	COMBAT_TRACE_INTERRUPT },
		{ L"dbgunit",	COMBAT_TRACE_DEBUGUNIT_ONLY	},
	};

	do 
	{
		for (unsigned int ii = 0; ii < arrsize(sParamMatch); ++ii)
		{
			if (PStrICmp(strToken, sParamMatch[ii].str, 3) == 0)
			{
				GameToggleCombatTrace(game, sParamMatch[ii].flag);
				ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Combat Log (%s): %s", sParamMatch[ii].str, sGetOnOffString(GameGetCombatTrace(game, sParamMatch[ii].flag)));
				break;
			}
		}
	} while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)));

_done:
	if (!wasCombatTraceActive && GameGetDebugFlag(game, DEBUGFLAG_COMBAT_TRACE))
	{
		game->m_idDebugClient = ClientGetId(client);
		DumpCombatHistory(game, client);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdProcTrace)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	extern BOOL gbProcTrace;
	gbProcTrace = !gbProcTrace;

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Proc Trace: %s.", sGetOnOffString(gbProcTrace));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDatatableDisplay)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	EXCELTABLE table = DATATABLE_NONE;
	if (!sConsoleParseDatatableName(game, cmd, 0, strParams, strToken, arrsize(strToken), table))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"table", strToken);
		return CR_FAILED;
	}

	unsigned int numRows = ExcelGetNumRows(game, table);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"  TABLE [%s]  %d rows\n", strToken, numRows);
	for (unsigned int ii = 0; ii < numRows; ++ii)
	{
		WCHAR strIndex[MAX_CONSOLE_PARAM_VALUE_LEN];
		PStrCvt(strIndex, ExcelGetStringIndexByLine(game, table, ii), arrsize(strIndex));
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"  %4d - %s\n", ii, strIndex);
	}	
						
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdLocation)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * unit = sGetDebugOrControlUnit(game, client);
	ASSERT_RETVAL(unit, CR_FAILED);

	VECTOR position = UnitGetPosition(unit);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"X:%.2f, Y:%.2f, Z:%.2f.", position.fX, position.fY, position.fZ);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdWarp)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * unit = sGetDebugOrControlUnit(game, client);
	ASSERT_RETVAL(unit, CR_FAILED);

	ROOM * room = UnitGetRoom(unit);
	ASSERT_RETVAL(room, CR_FAILED);

	VECTOR target = UnitGetPosition(unit);
	
	float dist;
	if (!sConsoleParseFloatParam(cmd, 0, strParams, dist))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing warp distance or location");
	}
	float val;
	if (sConsoleParseFloatParam(cmd, 1, strParams, val))
	{
		target.fX = dist;
		target.fY = val;
		if (sConsoleParseFloatParam(cmd, 2, strParams, val))
		{
			target.fZ = val;
		}
	}
	else
	{
		sGetPositionInFrontOfUnit(unit, dist, &target);
	}

	room = RoomGetFromPosition(game, room, &target);
	if (!room)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid warp destination");
		return CR_OK;
	}

	DWORD dwWarpFlags = 0;
	if (UnitIsA(unit, UNITTYPE_PLAYER))
	{
		SETBIT(dwWarpFlags, UW_TRIGGER_EVENT_BIT);
	}

	UnitWarp(unit, room, target, UnitGetFaceDirection(unit, FALSE), UnitGetUpDirection(unit), dwWarpFlags);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdState)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * unit = sGetDebugOrControlUnit(game, client);
	ASSERT_RETVAL(unit, CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int state;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), state))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"state", strToken);
		return CR_FAILED;
	}

	int ticks;
	if (!sConsoleParseIntegerParam(cmd, 1, strParams, ticks) || ticks <= 0)
	{
		ticks = 5 * GAME_TICKS_PER_SECOND;
	}

	if (PStrICmp(strParams[0], L'c'))
	{
		strParams = sConsoleSkipDelimiter(strParams + 1);
		// set the state on the client only
		MSG_SCMD_UNITSETSTATE   tMsgSet; 
		int nMessageSize = 0;
		tMsgSet.idTarget = UnitGetId( unit );
		tMsgSet.idSource = tMsgSet.idTarget;
		tMsgSet.wState		 = (WORD)state;
		tMsgSet.wStateIndex  = 0;
		tMsgSet.nDuration	 = ticks;
		tMsgSet.wSkill		 = (WORD)-1;
		tMsgSet.dwStatsListId = INVALID_ID;

		ZeroMemory(&tMsgSet.tBuffer, sizeof(tMsgSet.tBuffer));
		MSG_SET_BLOB_LEN(tMsgSet, tBuffer, nMessageSize);

		s_SendUnitMessage( unit, SCMD_UNITSETSTATE, &tMsgSet);
	}
	else
	{
		s_StateSet(unit, unit, state, ticks);
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
CONSOLE_CMD_FUNC(x_sCmdClearState)
{
	ASSERT_RETVAL(game, CR_FAILED);

	UNIT * unit = sGetDebugOrControlUnit(game, client);
	ASSERT_RETVAL(unit, CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int state;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), state))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"state", strToken);
		return CR_FAILED;
	}

	StateClearAllOfType(unit, state);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdToggleTreasure)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	GLOBAL_DEFINITION * globalData = DefinitionGetGlobal();
	ASSERT_RETVAL(globalData, CR_FAILED);
	
	globalData->dwFlags ^= GLOBAL_FLAG_NOLOOT;

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Treasure Dropping: %s.", sGetOnOffString(globalData->dwFlags & GLOBAL_FLAG_NOLOOT));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdToggleRagdolls)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	GLOBAL_DEFINITION * globalData = DefinitionGetGlobal();
	ASSERT_RETVAL(globalData, CR_FAILED);
	
	globalData->dwFlags ^= GLOBAL_FLAG_NORAGDOLLS;

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Ragdolls: %s.", sGetOnOffString(globalData->dwFlags & GLOBAL_FLAG_NORAGDOLLS));

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdWardrobeLayer)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int wardrobeLayer;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), wardrobeLayer))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"wardrobe layer", strToken);
		return CR_FAILED;
	}

	UNIT * unit = sGetDebugOrControlUnit(game, client);

	c_WardrobeForceLayerOntoUnit(unit, wardrobeLayer);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdColorSet)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * unit = sGetDebugOrControlUnit(game, client);
	ASSERT_RETVAL(unit, CR_FAILED);

	int nWardrobe = UnitGetWardrobe(unit);
	if ( nWardrobe == INVALID_ID )
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Selected unit doesn't have wardrobe layer.");
		return CR_FAILED;
	}

	ASSERT_RETINVALID(game && (client || IS_CLIENT(game)) && cmd && strParams);

	int nColorSet;
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseExcelLookupOrName(game, cmd, DATATABLE_COLORSETS, strParams, strToken, arrsize(strToken), nColorSet))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"monster", strToken);	
		nColorSet = INVALID_LINK;
	}

	c_WardrobeSetColorSetIndex( nWardrobe, nColorSet, UnitGetType( unit ), TRUE);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdSquish)
{
	extern BOOL g_bUseSquish;

	g_bUseSquish = !g_bUseSquish;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Squish: %s", sGetOnOffString(g_bUseSquish));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdColorMaskMode)
{
	extern unsigned int g_nColorMaskStrategy;

	int val;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, val))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing color mask strategy");
	}
	g_nColorMaskStrategy = (unsigned int)val;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Color Mask Strategy: %d", g_nColorMaskStrategy);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdColorMaskErrorThreshold)
{
	extern unsigned int g_nColorMaskErrorThreshold;

	int val;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, val))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing color mask error threshold");
	}
	g_nColorMaskErrorThreshold = (unsigned int)val;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Color Mask Error Threshold: %d", g_nColorMaskErrorThreshold);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdToggleTargetBrackets)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UIToggleTargetBrackets();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdToggleTargetIndicator)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UIToggleTargetIndicator();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdListUIIDs)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	char chComponentName[MAX_CONSOLE_PARAM_VALUE_LEN];
	chComponentName[0] = 0;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		PStrCvt(chComponentName, strToken, arrsize(chComponentName));
	}

	UITraceIDs(chComponentName);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUITron)
{
	char chComponent[MAX_CONSOLE_PARAM_VALUE_LEN];
	char chMessage[CONSOLE_MAX_STR];
	char chResult[CONSOLE_MAX_STR];

	chComponent[0] = 0;
	chMessage[0] = 0;
	chResult[0] = 0;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		if (PStrICmp(strToken, L"c=", 2) == 0)
		{
			PStrCvt(chComponent, strToken + 2, arrsize(chComponent));
			if (chComponent[0] == 0)
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing component name.");
				return CR_FAILED;
			}
		}
		else if (PStrICmp(strToken, L"m=", 2) == 0)
		{
			PStrCvt(chMessage, strToken + 2, arrsize(chMessage));
			if (chMessage[0] == 0)
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing message string.");
				return CR_FAILED;
			}
		}
		else if (PStrICmp(strToken, L"r=", 2) == 0)
		{
			PStrCvt(chResult, strToken + 2, arrsize(chResult));
			if (chResult[0] == 0)
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing result string.");
				return CR_FAILED;
			}
		}
	}

	UISetMsgTrace(TRUE, chComponent, chMessage, chResult);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"UI Message Trace: ON");

	return CR_OK;
}

CONSOLE_CMD_FUNC(c_sCmdUIActivateTrace)
{
	g_UI.m_bActivateTrace = !g_UI.m_bActivateTrace;
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"UI Activation Trace: %s", sGetOnOffString(g_UI.m_bActivateTrace));

	return CR_OK;
}

#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUITroff)
{
	UISetMsgTrace(FALSE);

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"UI Message Trace: OFF");
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdToggleSilentAssert)
{
	ASSERT_RETVAL(game, CR_FAILED);

	GLOBAL_DEFINITION * globalData = DefinitionGetGlobal();

	globalData->dwFlags ^= GLOBAL_FLAG_SILENT_ASSERT;

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Silent asserts: %s.", sGetOnOffString(globalData->dwFlags & GLOBAL_FLAG_SILENT_ASSERT));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCrashFromConsole)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	char * pBlah = NULL;
	*pBlah = '\0';

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdAssert)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	ASSERTX(2 == 3, "Assertion test");

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdWarn)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WARNX(2 == 3, "Warning test");

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdHalt)
{
	HALTX(2 == 3, "Halt test");

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdMemCrash)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	char * pBlah = NULL;
	pBlah = (char*)MALLOCBIG(NULL, 1 * GIGA);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdToggleSilentAsserts)
{
	gtAppCommon.m_bSilentAssert = !gtAppCommon.m_bSilentAssert;

	ConsoleString(
		"Assert Silently now = %s",
		gtAppCommon.m_bSilentAssert ? "TRUE" : "FALSE" );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdToggleParticleDebug)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_PARTICLES, -1);
	c_ParticleToggleDebugMode();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdParticle)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = GameGetControlUnit(game);

	WCHAR strToken[CONSOLE_MAX_STR];
	if (!sConsoleParseParamParticleDefinition(game, client, cmd, 0, strParams, strToken, arrsize(strToken)))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"particle", strToken);
		return CR_FAILED;
	}

	char chParticle[CONSOLE_MAX_STR];
	PStrCvt(chParticle, strToken, MAX_PATH);

	int idDefinition = DefinitionGetIdByName(DEFINITION_GROUP_PARTICLE, chParticle);
	if (idDefinition == INVALID_ID)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Unable to load particle [%s].\n", strToken);
		return CR_FAILED;
	}

	VECTOR position;
	ASSERT_RETVAL(sGetPositionInFrontOfUnit(player, 3.0f, &position), CR_FAILED);

	int idParticle = c_ParticleSystemNew(idDefinition, &position, &cgvZAxis, (player ? UnitGetRoomId(player) : INVALID_ID));
	c_ParticleSystemSetVisible(idParticle, TRUE);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdParticleCensor)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	AppSetCensorFlag( AF_CENSOR_PARTICLES, ! AppTestFlag( AF_CENSOR_PARTICLES ), FALSE );

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Particle censorship: %s", sGetOnOffString( c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_CENSORSHIP_ENABLED ) ));

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdParticleDX10)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_ParticleSetGlobalBit( PARTICLE_GLOBAL_BIT_DX10_ENABLED, ! c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_DX10_ENABLED ) );

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Dx10 particles: %s", sGetOnOffString( c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_DX10_ENABLED ) ));

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdParticleDestroyAll)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_ParticleSystemsDestroyAll();

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Destroyed all particles");

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdParticleAsync)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_ParticleSetGlobalBit( PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED, ! c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED ) );

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Async particles: %s", sGetOnOffString( c_ParticleTestGlobalBit( PARTICLE_GLOBAL_BIT_ASYNC_SYSTEMS_ENABLED ) ));

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCensorNoHumans)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bCensorNoHumans = AppTestFlag(AF_CENSOR_NO_HUMANS);
	AppSetFlag( AF_CENSOR_NO_HUMANS, !bCensorNoHumans );

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Censor No Humans: %s", sGetOnOffString( AppTestFlag(AF_CENSOR_NO_HUMANS) ));

	return CR_OK;
}

CONSOLE_CMD_FUNC(c_sCmdCensorNoGore)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	BOOL bCensorNoGore = AppTestFlag(AF_CENSOR_NO_GORE);
	AppSetFlag( AF_CENSOR_NO_GORE, !bCensorNoGore );

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Censor No Humans: %s", sGetOnOffString( AppTestFlag(AF_CENSOR_NO_GORE) ));

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdShape)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UNIT * player = GameGetControlUnit(game);
	ASSERT_RETVAL(player, CR_FAILED);

	int height;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, height))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing height");
		return CR_FAILED;
	}

	int weight;
	if (!sConsoleParseIntegerParam(cmd, 1, strParams, weight))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing weight");
		return CR_FAILED;
	}

	int idModel = c_UnitGetModelIdThirdPerson(player);
	ASSERT_RETVAL(idModel != INVALID_ID, CR_FAILED);

	// need 0-255, but 0-100 is easier to input
	APPEARANCE_SHAPE shape;
	shape.bHeight = (BYTE)((255 * height) / 100); 
	shape.bWeight = (BYTE)((255 * weight) / 100);

	c_AppearanceShapeSet(idModel, shape);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdToggleDPS)
{
	ASSERT_RETVAL(AppGetType() == APP_TYPE_SINGLE_PLAYER, CR_FAILED);
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

//	UnitInitValueTimeTag(game, player, TAG_SELECTOR_DAMAGE_DONE);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_DPS, -1);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdDPSResetTimer)
{
	ASSERT_RETVAL(AppGetType() == APP_TYPE_SINGLE_PLAYER, CR_FAILED);
	UIResetDPSTimer();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdWeaponConfigsClear)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	UNIT * item = UnitInventoryLocationIterate(player, INVLOC_WEAPONCONFIG, NULL);
	while (item)
	{
		UnitInventoryAdd(INV_CMD_PICKUP, player, item, UnitGetBigBagInvLoc());
		item = UnitInventoryLocationIterate(player, INVLOC_WEAPONCONFIG, NULL);
	}

	item = UnitInventoryGetByLocation(player, INVLOC_LHAND);
	if (item)
	{
		UnitInventoryAdd(INV_CMD_PICKUP, player, item, UnitGetBigBagInvLoc());
	}
	item = UnitInventoryGetByLocation(player, INVLOC_RHAND);
	if (item)
	{
		UnitInventoryAdd(INV_CMD_PICKUP, player, item, UnitGetBigBagInvLoc());
	}

	for (unsigned int ii = TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; ii >= TAG_SELECTOR_WEAPCONFIG_HOTKEY1; --ii)
	{
		UNIT_TAG_HOTKEY * tag = UnitGetHotkeyTag(player, ii);
		if (tag)
		{
			tag->m_idItem[0] = INVALID_ID;
			tag->m_idItem[1] = INVALID_ID;
			HotkeySend( client, player, (TAG_SELECTOR)ii);
		}
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(x_sCmdWeaponConfigsDisp)
{
	ASSERT_RETVAL(game, CR_FAILED);

	UNIT * player = sGetControlUnit(game, client);
	ASSERT_RETVAL(player, CR_FAILED);

	sDebugTraceWeaponConfig(game, player);

	return CR_OK;
}
#endif



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugHavokFX)
{
#ifdef HAVOK_ENABLED
	if(AppIsHellgate())
	{
		WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
		if (!sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"missing parameter.");
			return CR_FAILED;
		}
		int param2 = 0;
		sConsoleParseIntegerParam(cmd, 1, strParams, param2);

		if (PStrICmp(strToken, L"h3", 2) == 0)
		{
			e_DumpHavokFXReport(param2, false);
		}
		else if (PStrICmp(strToken, L"fx", 2) == 0)
		{
			e_DumpHavokFXReport(param2, true);
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid parameter [%s].", strToken);
			return CR_FAILED;
		}
	}
#endif
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdInvertMouse)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	c_ToggleInvertMouse();
	return CR_OK;
}

CONSOLE_CMD_FUNC(c_sCmdDirectInputMouse)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	BOOL bResult = c_SetUsingDirectInput(TRUE);
	if (bResult)
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Mouse using Direct Input");
	else
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Mouse unable to use Direct Input");
	return CR_OK;
}

CONSOLE_CMD_FUNC(c_sCmdWindowsInputMouse)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	BOOL bResult = c_SetUsingDirectInput(FALSE);
	if (!bResult)
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Mouse using windows message input");
	else
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Mouse unable to use windows message input");
	return CR_OK;
}


#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdD3DStatesStats)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_STATES, -1);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdParticleDrawStats)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_PARTICLESTATS, -1);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdD3DStats)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_ENGINESTATS, -1);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdD3DResStats)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_ToggleTrackResourceStats();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdHashMetrics)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_HASHMETRICS, -1);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugDumpDrawLists)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_DebugDumpDrawLists();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Dumped draw lists to Logs\\datadump.csv");

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugDumpOcclusion)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_DebugDumpOcclusion();
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Dumped occlusion to Logs\\datadump.csv");

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugDumpTextures)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	V( e_DumpTextureReport() );
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Dumped texture report to data\\texture_report.txt");

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugDumpEffects)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	V( e_DumpEffectReport() );
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Dumped effect report to Hellgate\\Logs\\effect_report.csv");

	return CR_OK;
}
#endif



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugDumpEngineMemory)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	V( e_EngineMemoryDump() );
	//ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Engine memory:");

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugReapEngineMemory)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int nAlertLevel = REAPER_ALERT_LEVEL_NONE;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nAlertLevel))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing alert level.");
		return CR_FAILED;
	}

	BOOL bOverride = TRUE;
	if ( nAlertLevel < REAPER_ALERT_LEVEL_NONE )
	{
		nAlertLevel = REAPER_ALERT_LEVEL_NONE;
		bOverride = FALSE;
	}

	e_ReaperSetAlertLevel( (REAPER_ALERT_LEVEL)nAlertLevel, bOverride );

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugTextureList)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	V( e_TextureDumpList() );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugModelRemove)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int idModel = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idModel))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing model id.");
		return CR_FAILED;
	}

	c_ModelRemove( idModel );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugModelList)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_ModelDumpList();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugDumpMIPLevels)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_TextureBudgetDebugOutputMIPLevels(TRUE, TRUE, TRUE);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugPreloadTexture)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int idTexture = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idTexture))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing texture id.");
		return CR_FAILED;
	}

	V( e_TexturePreload(idTexture) );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdEngineCleanup)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	V( e_Cleanup( TRUE ) );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugPreloadModelTextures)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int idModel = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idModel))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing model id.");
		return CR_FAILED;
	}

	V( e_ModelTexturesPreload(idModel) );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdD3DReset)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	V( e_Reset() );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDisplayCaps)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_DebugOutputCaps();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdScreenShotTiles)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int nTiles = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nTiles))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing tiling factor.");
		return CR_FAILED;
	}

	e_ScreenShotSetTiles( nTiles );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdToggleScreenshotQuality)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	if (e_ToggleScreenshotQuality())
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Screenshot quality set to HIGH (uncompressed)");
	}
	else
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Screenshot quality set to LOW (compressed)");
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sCmdScreenshotTakenDisplayMessage(
	const OS_PATH_CHAR * pszFilename,
	void * pData )
{
	const WCHAR * uszMessage = GlobalStringGet(GS_SCREENSHOT_SAVED_MSG);
	if(uszMessage)
	{
		OS_PATH_CHAR szFileNameNoPath[MAX_PATH];
		PStrRemoveEntirePath(szFileNameNoPath, MAX_PATH, pszFilename);
		UIAddChatLineArgs(CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_SERVER), uszMessage, szFileNameNoPath);
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdTakeScreenshotNoUI)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

#if ISVERSION(DEVELOPMENT)
	BOOL bWithUI = FALSE;
#else
	BOOL bWithUI = TRUE;
#endif

	SCREENSHOT_PARAMS tParams;
	tParams.bWithUI = bWithUI;
	tParams.pfnPostSaveCallback = c_sCmdScreenshotTakenDisplayMessage;
	e_CaptureScreenshot( tParams );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdTakeScreenshotWithUI)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	SCREENSHOT_PARAMS tParams;
	tParams.bWithUI = TRUE;
	tParams.pfnPostSaveCallback = c_sCmdScreenshotTakenDisplayMessage;
	e_CaptureScreenshot( tParams );

	return (DWORD)(CR_OK | CRFLAG_CLEAR_CONSOLE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGenerateEnvMap)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int size = 0;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, size))
	{
		e_GenerateEnvironmentMap();
		return CR_OK;
	}

	e_GenerateEnvironmentMap(size);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdAnimTrace)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_AppearanceToggleAnimTrace();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdAnimDump)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_AppearanceToggleAnimTrace();

	UNIT * player = GameGetControlUnit(game);
	if (player)
	{
		c_AppearanceDumpAnims(player);
	}
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUIReload)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	c_sUIReload(TRUE);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTextShadow)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	bDropShadowsEnabled = !bDropShadowsEnabled;
	
	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Text shadows: %s", sGetOnOffString(bDropShadowsEnabled));
	UIRepaint();
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdAlwaysShowItemNames)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	BOOL bShowNames = UIGetAlwaysShowItemNamesState();
	UISetAlwaysShowItemNames( !bShowNames );

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Always show item names: %s", sGetOnOffString(!bShowNames));
	UIRepaint();
	
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
#include "uix_components_hellgate.h"
CONSOLE_CMD_FUNC(c_sCmdUIShowAccuracyBrackets)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	BOOL bShow = UIToggleFlag(UI_FLAG_SHOW_ACC_BRACKETS);

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Show accuracy brackets: %s", sGetOnOffString(bShow));

	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_TARGETCOMPLEX);
	if (component)
	{
		UI_HUD_TARGETING *pTargeting = UICastToHUDTargeting(component);
		pTargeting->m_bForceUpdate = TRUE;
	}

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUITestTipDialog)
{
  ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int nParam = INVALID_LINK;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nParam))
	{
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"You gotta provide the number for a string.");
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
// CONSOLE_CATEGORY("billing"); //Client console commands
//----------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)

//	CONSOLE_CMD("get_billing_account_status", CHEATS_RELEASE_TESTER, COMMAND_CLIENT, COMMAND_SERVER, s_sCmdGetBillingAccountStatus, false);	
//		CONSOLE_DESC("request account status from billing provider");	
CONSOLE_CMD_FUNC(s_sCmdGetBillingAccountStatus)
{
  TraceVerbose(TRACE_FLAG_BILLING_PROXY, "consolecmd.cpp:s_sCmdGetBillingAccountStatus: !game || IS_CLIENT(game) == %d\n", !game || IS_CLIENT(game));
  ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
  if (AppGetBadgeCollection() && AppGetBadgeCollection()->HasBadge(ACCT_TITLE_TESTER)) {
	  return c_BillingSendMsgRequestAccountStatus() ? CR_OK : CR_FAILED;
  }
  return CR_FAILED;
}

//	CONSOLE_CMD("get_billing_remaining_time", CHEATS_RELEASE_TESTER, COMMAND_CLIENT, COMMAND_CLIENT, c_sCmdBillingDisplayRemainingTime, false);	
//		CONSOLE_DESC("display the time remaining on the account");	
CONSOLE_CMD_FUNC(c_sCmdBillingDisplayRemainingTime)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	c_BillingDisplayRemainingTime();
	return CR_OK;
}


//	CONSOLE_CMD("real_money_txn", CHEATS_RELEASE_TESTER, COMMAND_CLIENT, COMMAND_SERVER, s_sCmdRealMoneyTxn, false);	
//		CONSOLE_PAR_NA("Item Code", PARAM_INTEGER, true, 0, 0, "");
//		CONSOLE_PAR_NA("Item Description", PARAM_STRING, true, 0, 0, "");
//		CONSOLE_PAR_NA("Price", PARAM_INTEGER, true, 0, 0, "");
//		CONSOLE_DESC("initiate a real money transaction");	
CONSOLE_CMD_FUNC(s_sCmdRealMoneyTxn)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
	if (!AppGetBadgeCollection() || !AppGetBadgeCollection()->HasBadge(ACCT_TITLE_TESTER))
		return CR_FAILED;

	int iItemCode = 0, iPrice = 0;
	wchar_t szItemDesc[100] = L"";
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, iItemCode)) {
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"You must specify the item code.");
		return CR_FAILED;
	}
	if (!sConsoleParseToken(cmd, strParams, szItemDesc, arrsize(szItemDesc)) || !szItemDesc[0]) {
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"You must specify the item description.");
		return CR_FAILED;
	}
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, iPrice) || iPrice < 0) {
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"You must specify a valid price.");
		return CR_FAILED;
	}
	return c_BillingSendMsgRealMoneyTxn(iItemCode, szItemDesc, iPrice) ? CR_OK : CR_FAILED;
}

CONSOLE_CMD_FUNC(s_sCmdBillingDebitCurrency) {
  ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
  if (!AppGetBadgeCollection() || !AppGetBadgeCollection()->HasBadge(ACCT_TITLE_TESTER)) return CR_FAILED;

  int debitAmount = 0;
  wchar_t* pCatalogSku = NULL;
  wchar_t* pTransactionDescription = NULL;
  wchar_t catalogSku[8] = L"";
  wchar_t transactionDescription[255] = L"";
  if (!sConsoleParseIntegerParam(cmd, 0, strParams, debitAmount)) {
    ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"You must specify the amount to debit.");
    return CR_FAILED;
  } else if (debitAmount <= 0) {
    ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Amount must be greater than zero.");
    return CR_FAILED;
  }
  if (sConsoleParseToken(cmd, strParams, catalogSku, arrsize(catalogSku)) && catalogSku[0]) {
    pCatalogSku = catalogSku;
    if (!sConsoleParseToken(cmd, strParams, transactionDescription, arrsize(transactionDescription)) || !transactionDescription[0]) {
      pTransactionDescription = transactionDescription;
    }
  }

  return s_BillingSendMsgDebitCurrency(BillingMsgContext_ConsoleCmd, debitAmount, pCatalogSku, pTransactionDescription) ? CR_OK : CR_FAILED;
}

CONSOLE_CMD_FUNC(s_sCmdBillingDebitConfirm) {
  ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);
  if (!AppGetBadgeCollection() || !AppGetBadgeCollection()->HasBadge(ACCT_TITLE_TESTER)) return CR_FAILED;

  wchar_t authorizationId[255] = L"";
  if (!sConsoleParseToken(cmd, strParams, authorizationId, arrsize(authorizationId)) || !authorizationId[0]) {
    ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"You must specify the Authorization ID.");
    return CR_FAILED;
  }

  return s_BillingSendMsgDebitConfirm(BillingMsgContext_ConsoleCmd, authorizationId) ? CR_OK : CR_FAILED;
}

#endif // !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// CONSOLE_CATEGORY("billing"); //Server console commands
//----------------------------------------------------------------------------

//#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)


//#endif // ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdStatDispDebug)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	BOOL bDebugging = AppToggleDebugFlag(ADF_TEXT_STATS_DISP_DEBUGNAME);

	ConsoleStringForceOpen(CONSOLE_SYSTEM_COLOR, L"Stat display labels debugging: %s", sGetOnOffString(bDebugging));
	UIRepaint();
	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdFXReload)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_MaterialsRefresh( TRUE );

	return CR_OK;
}
#endif
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdPCSSLightSize)
{
	WCHAR strToken[ MAX_CONSOLE_PARAM_VALUE_LEN ];
	float fParam[2] = {0.0f, 0.0f };
	char chParam[ CONSOLE_MAX_STR ];

	for( int i = 0; i < 2; ++i )
	{
		if( sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L"," ) )
		{
			PStrCvt(chParam, strToken, arrsize(chParam) );
			fParam[i] = PStrToFloat( strToken );
		}
	}

	e_PCSSSetLightSize( fParam[0], fParam[1] );

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT) 

CONSOLE_CMD_FUNC(c_sSwitchToRef)
{
	e_SwitchToRef();

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDOFParams)
{
	WCHAR strToken[ MAX_CONSOLE_PARAM_VALUE_LEN ];
	float fParam[4] = {0.0f, 0.0f, 0.0f, 0.0f };
	char chParam[ CONSOLE_MAX_STR ];

	for( int i = 0; i < 4; ++i )
	{
		if( sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L"," ) )
		{
			PStrCvt(chParam, strToken, arrsize(chParam) );
			fParam[i] = PStrToFloat( strToken );
		}
	}

	e_DOFSetParams( fParam[0], fParam[1], fParam[2], fParam[3] );

	return CR_OK;
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTraceModel)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int idModel = INVALID_ID;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idModel))
	{
		e_SetTraceModel(INVALID_ID);
		e_DebugIdentifyModel(INVALID_ID);
	}
	else
	{
		e_SetTraceModel(idModel);
		e_DebugIdentifyModel(idModel);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdDebugIdentifyModel)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int idModel = INVALID_ID;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idModel))
	{
		e_DebugIdentifyModel(INVALID_ID);
	}
	else
	{
		e_DebugIdentifyModel(idModel);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTraceRoom)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int idRoom = INVALID_ID;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, idRoom))
	{
		c_SetTraceRoom(INVALID_ID);
		e_DebugIdentifyModel(INVALID_ID);
	}
	else
	{
		c_SetTraceRoom(idRoom);

		int idModel = INVALID_ID;
		ROOM * room = RoomGetByID(game, idRoom);
		if (room)
		{
			idModel = RoomGetRootModel(room);
		}
		e_DebugIdentifyModel(idModel);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTraceQuery)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int address;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, address, NULL, TRUE))
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Missing trace address.");
		return CR_FAILED;
	}

	e_SetTraceQuery(address);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTraceEvent)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	e_ToggleDebugTrace();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTraceBatches)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	int nGroup;
	if (!sConsoleParseIntegerParam(cmd, 0, strParams, nGroup, NULL, TRUE))
	{
		nGroup = METRICS_GROUP_ALL;
	}
	else if ( nGroup < METRICS_GROUP_ALL || nGroup >= NUM_METRICS_GROUPS )
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid metrics group.");
		return CR_FAILED;
	}
	e_DebugTraceBatches( (METRICS_GROUP)nGroup );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdFirebolter)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ITEM_SPEC spec;
	spec.nLevel = UnitGetExperienceLevel(player);
	spec.nLevel = MAX(spec.nLevel, 1);

	BOOL bEquip = TRUE;
	BOOL bAddMods = FALSE;

	BOOL bElementEffects[MAX_DAMAGE_TYPES];
	memclear(bElementEffects, arrsize(bElementEffects) * sizeof(BOOL));

	int classFirebolter = GlobalIndexGet(GI_ITEM_FIREBOLTER);
	ASSERT_RETVAL(classFirebolter != INVALID_ID, CR_FAILED);

	spec.nItemClass = classFirebolter;
	spec.pSpawner = player;

	DWORD dwFlags = ITEM_SPAWN_CHEAT_FLAG_DEBUG_OUTPUT;
	if ( bEquip )
		dwFlags |= ITEM_SPAWN_CHEAT_FLAG_PICKUP | ITEM_SPAWN_CHEAT_FLAG_FORCE_EQUIP;
	if ( bAddMods )
		dwFlags |= ITEM_SPAWN_CHEAT_FLAG_ADD_MODS;

	s_ItemSpawnForCheats(game, spec, client, dwFlags, bElementEffects );	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdFirebolterZombie)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	s_sCmdFirebolter(game, client, cmd, strParams, NULL);
	s_sCmdZombie(game, client, cmd, strParams, NULL);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGameExplorerAdd)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	PrimeGameExplorerAdd(TRUE);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGameExplorerRemove)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	GAME_EXPLORER_ERROR tError;
	BOOL bSuccess = GameExplorerRemove(tError);
	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Games Explorer Remove: %s", bSuccess ? L"OK" : L"FAILED");
	if (bSuccess == FALSE && tError.uszError[0])
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"  Error: %s", tError.uszError);		
	}	
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTrade)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	for (GAMECLIENT * otherClient = ClientGetFirstInGame(game); otherClient; otherClient = ClientGetNextInGame(otherClient))
	{
		UNIT * otherUnit = ClientGetControlUnit(otherClient);
		
		if (otherUnit != player)
		{
			s_TradeRequestNew(player, otherUnit);
			return CR_OK;
		}
	}
	
	ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"No other player in game.");
	return CR_FAILED;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTradeTest)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	TRADE_SPEC tTradeSpec;
	s_TradeStart(player, player, tTradeSpec);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(s_sCmdTradeCancel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);
	ASSERT_RETVAL(client, CR_FAILED);

	UNIT *pPlayer = ClientGetControlUnit(client);
	ASSERT_RETVAL( pPlayer, CR_FAILED );

	if (TradeIsTrading( pPlayer ))
	{
		s_TradeCancel( pPlayer, TRUE );
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Trade cancelled");		
	}
		
	return CR_FAILED;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sPostBugScreenshot(
	const OS_PATH_CHAR * filename,
	void * data)
{
#if ISVERSION(DEVELOPMENT)


#if !defined(_WIN64)
	char * szText = (char *)data;
	//GenerateUserBugReport(szText, CONSOLE_MAX_STR, filename);
	UIShowBugReportDlg(szText, filename);

#endif

	FREE(NULL, data);

#else

	char * szText = (char *)data;
	UIShowBugReportDlg(szText, filename);
#endif // RETAIL_VERSION
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sReportBug)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	BOOL screenshot = FALSE;
	char * text = NULL;

	WCHAR strToken[CONSOLE_MAX_STR];
	const WCHAR * str = strParams;
	while (sConsoleParseToken(cmd, str, strToken, arrsize(strToken), L" ,"))
	{
		if (PStrICmp(strToken, L"s", arrsize(strToken)) == 0)
		{
			strParams = str;
			screenshot = TRUE;
		}
		else
		{
			unsigned int len = PStrLen(strParams);
			if (len > 0)
			{
				text = (char *)MALLOC(NULL, sizeof(char) * (len + 1));
				PStrCvt(text, strParams, len + 1);
			}
			strParams += len;
		}
	}

	if (screenshot)
	{
		SCREENSHOT_PARAMS tParams;
		tParams.bWithUI = TRUE;
		tParams.bSaveCompressed = TRUE;
		tParams.nTiles = 1;
		tParams.pfnPostSaveCallback = c_sPostBugScreenshot;
		tParams.pPostSaveCallbackData = (void*)text;
		e_CaptureScreenshot( tParams );
	}
	else
	{
		c_sPostBugScreenshot(NULL, (void *)text);
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sAppendToLine(
	GAME * game,
	GAMECLIENT * client,
	DWORD color,
	WCHAR * strLine,
	unsigned int lenLine,
	unsigned int & curLen,
	unsigned int spaces,
	const WCHAR * str,
	unsigned int lenStr = -1)
{
	if (lenStr <= 0)
	{
		lenStr = PStrLen(str);
	}

	if (curLen + lenStr + spaces >= lenLine)
	{
		UIAddChatLine(CHAT_TYPE_CLIENT_ERROR, color, strLine);
		strLine[0] = 0;
		curLen = 0;
	}

	for (unsigned int ii = 0; ii < spaces; ++ii)
	{
		strLine[curLen] = L' ';
		++curLen;
	}
	strLine[curLen] = 0;

	PStrCopy(strLine + curLen, str, lenLine - curLen);
	curLen += lenStr;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sPrintCommandCategories(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETURN(g_ConsoleCommandTable.m_pstrCategories);

	static const unsigned int MAX_LINE_WIDTH = 53;

	UIAddChatLine(CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet( GS_CCMD_HELP_CATEGORIES ));

	DWORD dwColorCategory = GetFontColor( FONTCOLOR_WHITE );  // CONSOLE_SYSTEM_COLOR

	WCHAR strLine[MAX_LINE_WIDTH];
	strLine[0] = 0;
	unsigned int curWidth = 0;
	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nCategories; ++ii)
	{
		c_sAppendToLine(game, client, dwColorCategory, strLine, arrsize(strLine), curWidth, 2, g_ConsoleCommandTable.m_pstrCategories[ii]->GetStr(), g_ConsoleCommandTable.m_pstrCategories[ii]->Len());
	}

	if (curWidth > 0)
	{
		UIAddChatLine(CHAT_TYPE_CLIENT_ERROR, dwColorCategory, strLine);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sPrintCommandCommandsInCategory(
	GAME * game,
	GAMECLIENT * client,
	const PStrW * category)
{
	ASSERT_RETURN(category);
	ASSERT_RETURN(g_ConsoleCommandTable.m_pCommandTable);

	static const unsigned int MAX_LINE_WIDTH = 60;

	UIAddChatLineArgs(CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet( GS_CCMD_HELP_COMMANDS ), category->GetStr());

	DWORD dwColorCommand = GetFontColor( FONTCOLOR_WHITE ); // CONSOLE_SYSTEM_COLOR;
	WCHAR strLine[MAX_LINE_WIDTH];
	strLine[0] = 0;
	unsigned int curWidth = 0;
	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nCommandCount; ++ii)
	{
		const CONSOLE_COMMAND * cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
		if (cmd->m_eAppType == COMMAND_SERVER && AppGetType() != APP_TYPE_CLOSED_SERVER)
		{
			continue;
		}
		if (cmd->m_strCategory != category)
		{
			continue;
		}
		c_sAppendToLine(game, client, dwColorCommand, strLine, arrsize(strLine), curWidth, 2, cmd->m_strCommand.GetStr(), cmd->m_strCommand.Len());
	}

	if (curWidth > 0)
	{
		UIAddChatLine(CHAT_TYPE_CLIENT_ERROR, dwColorCommand, strLine);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static const WCHAR * c_sGetStringOrUnknown(
	const WCHAR * str)
{
	static const WCHAR * strUnknown = L"?";
	return ((str && str[0]) ? str : strUnknown);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_ConsoleGetUsageString(
	WCHAR * strUsage,
	const int nMaxLen,
	const CONSOLE_COMMAND * cmd,
	FONTCOLOR eTextColor)
{
	ASSERT_RETURN(strUsage);
	ASSERT_RETURN(cmd);

	for (unsigned int ii = 0; ii < cmd->m_nParams; ++ii)
	{
		WCHAR strParam[CONSOLE_MAX_STR];
		strParam[0] = 0;
		UIAddColorToString(strParam, eTextColor, arrsize(strParam));
		CONSOLE_PARAM * param = cmd->m_pParams + ii;
		const WCHAR * fmt = NULL;
		static const WCHAR * fmtRequired = L" <%s>";
		static const WCHAR * fmtOptional = L" [%s]";

		if (param->m_isRequired)
		{
			fmt = fmtRequired;
		}
		else
		{
			fmt = fmtOptional;
		}

		const WCHAR * strParamName = param->m_strName.GetStr();
		if (strParamName[0] != 0)
		{
			PStrPrintf(strParam, arrsize(strParam), fmt, strParamName);
		}
		else
		{
			switch (param->m_eParamType)
			{
			case PARAM_INTEGER:				PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_INT)); break;
			case PARAM_FLOAT:				PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_FLOAT)); break;
			case PARAM_BOOLEAN:				PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_BOOL)); break;
			case PARAM_HEX:					PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_HEX)); break;
			case PARAM_DATATABLE:
			case PARAM_DATATABLE_BYNAME:
			case PARAM_DATATABLE_BYSTR:
				{
					WCHAR strTableName[CONSOLE_MAX_STR];
					const char * chTableName = ExcelGetTableName((EXCELTABLE)param->m_data1);
					if (chTableName)
					{
						PStrCvt(strTableName, chTableName, arrsize(strTableName));
					}
					else
					{
						PStrCopy(strTableName, L"index", arrsize(strTableName));
					}
					PStrPrintf(strParam, arrsize(strParam), fmt, strTableName); 
					break;
				}
			case PARAM_TABLENAME:			PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_TABLE)); break;
			case PARAM_PERF:				PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_PERF)); break;
			case PARAM_HITCOUNT:			PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_HITCOUNT)); break;
			case PARAM_GENERIC:				PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_GENERIC)); break;
			case PARAM_STRINGLIST:			
				{
					WCHAR strValues[CONSOLE_MAX_STR];
					strValues[0] = 0;
					for (unsigned int ii = 0; ii < param->m_nValues; ++ii)
					{
						PStrCat(strValues, param->m_pstrValues[ii].GetStr(), arrsize(strValues));
						if (ii < param->m_nValues - 1)
						{
							PStrCat(strValues, L"|", arrsize(strValues));
						}
					}
					PStrPrintf(strParam, arrsize(strParam), fmt, strValues); 
					break;
				}
			case PARAM_PLAYERNAME:			PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_PLAYERNAME)); break;
			case PARAM_STRING:				PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_STRING)); break;
			case PARAM_DEFINITION:			PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_DEFINITION)); break;
			case PARAM_TEAM:				PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_TEAM)); break;
			case PARAM_CONSOLECMD:			PStrPrintf(strParam, arrsize(strParam), fmt, GlobalStringGet(GS_CCMD_PARAM_CONSOLECMD)); break;
			default:						PStrPrintf(strParam, arrsize(strParam), fmt, L"?"); break;
			}
		}
		UIAddColorReturnToString(strParam, arrsize(strParam));
		PStrCat(strUsage, strParam, nMaxLen);
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_ConsoleGetUsageString(
	WCHAR * strUsage,
	const int nMaxLen,
	const WCHAR * strCmd,
	FONTCOLOR eTextColor)
{
	ASSERT_RETURN(strUsage);
	ASSERT_RETURN(strCmd);

	const CONSOLE_COMMAND * cmd = sCommandTableFindCommandExact(strCmd);
	c_ConsoleGetUsageString(strUsage, nMaxLen, cmd, eTextColor);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_ConsoleGetDescString(
	WCHAR * strDesc,
	const int nMaxLen,
	const CONSOLE_COMMAND * cmd,
	FONTCOLOR eTextColor)
{
	ASSERT_RETURN(strDesc);
	ASSERT_RETURN(cmd);

	PStrCat(strDesc, c_sGetStringOrUnknown(cmd->m_strDescription.GetStr()), nMaxLen);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_ConsoleGetDescString(
	WCHAR * strDesc,
	const int nMaxLen,
	const WCHAR * strCmd,
	FONTCOLOR eTextColor)
{
	ASSERT_RETURN(strDesc);
	ASSERT_RETURN(strCmd);

	const CONSOLE_COMMAND * cmd = sCommandTableFindCommandExact(strCmd);
	c_ConsoleGetDescString(strDesc, nMaxLen, cmd, eTextColor);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sPrintCommandCommandHelp(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd)
{
	ASSERT_RETURN(cmd);

	const WCHAR * strCommand = cmd->m_strCommand.GetStr();
	ASSERT_RETURN(strCommand && strCommand[0]);

	DWORD dwChatLineColor = GetFontColor( FONTCOLOR_WHITE );
	FONTCOLOR eTextColor = FONTCOLOR_YELLOW;
	FONTCOLOR eColorLabel = FONTCOLOR_LIGHT_YELLOW;	
	const int MAX_BUFFER = 1024;
	WCHAR uszLabel[ MAX_BUFFER ];
	
	UIColorGlobalString( GS_CCMD_HELP_HEADER_COMMAND, eColorLabel, uszLabel, MAX_BUFFER );
	UIAddChatLineArgs(
		CHAT_TYPE_CLIENT_ERROR,
		dwChatLineColor,
		L"%s: %s", 
		uszLabel,
		strCommand);

	UIColorGlobalString( GS_CCMD_HELP_HEADER_DESC, eColorLabel, uszLabel, MAX_BUFFER );		
	UIAddChatLineArgs(
		CHAT_TYPE_CLIENT_ERROR,
		dwChatLineColor,
		L"%s: %s", 
		uszLabel, 
		c_sGetStringOrUnknown(cmd->m_strDescription.GetStr()));
	if (cmd->m_strUsage.Len() > 0)
	{
	
		UIColorGlobalString( GS_CCMD_HELP_HEADER_USAGE, eColorLabel, uszLabel, MAX_BUFFER );
		UIAddChatLineArgs(
			CHAT_TYPE_CLIENT_ERROR,
			dwChatLineColor,
			L"%s: %s", 
			uszLabel,
			c_sGetStringOrUnknown(cmd->m_strUsage.GetStr()));
		goto _printnotes;
	}

	WCHAR strUsage[CONSOLE_MAX_STR];
	UIColorGlobalString( GS_CCMD_HELP_HEADER_USAGE, eColorLabel, uszLabel, MAX_BUFFER );
	PStrPrintf(
		strUsage, 
		arrsize(strUsage), 
		L"%s: %s", 
		uszLabel, 
		cmd->m_strCommand.GetStr());
	c_ConsoleGetUsageString(strUsage, CONSOLE_MAX_STR, cmd, eTextColor);

	UIAddChatLine(CHAT_TYPE_CLIENT_ERROR, dwChatLineColor, strUsage);

_printnotes:
	if (cmd->m_strUsageNotes.Len() > 0)
	{
		UIAddChatLineArgs(
			CHAT_TYPE_CLIENT_ERROR,
			ChatGetTypeColor(CHAT_TYPE_SERVER), 
			L"%s", 
			c_sGetStringOrUnknown(cmd->m_strUsageNotes.GetStr()));
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sHelp)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (!sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		c_sPrintCommandCategories(game, client);
		return CR_OK;
	}

	const CONSOLE_COMMAND * command = sCommandTableFindCommandExact(strToken);
	if (command)
	{
		c_sPrintCommandCommandHelp(game, client, command);
		return CR_OK;
	}

	const PStrW * category = sCommandTableFindCategory(strToken);
	if (category)
	{
		c_sPrintCommandCommandsInCategory(game, client, category);
		return CR_OK;
	}

	c_sPrintCommandCategories(game, client);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdBars)
{
	ASSERT_RETVAL(!game || IS_CLIENT(game), CR_FAILED);

	BOOL forceVisible = FALSE;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int barsSet;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), barsSet))
	{
		PStrCopy(strToken, L"general", arrsize(strToken));
		barsSet = 0;
	}
	else
	{
		forceVisible = TRUE;
	}

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_BARS, forceVisible ? 1 : -1);

	c_SetUIBarsSet(barsSet);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGlobalTheme)
{
	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nGlobalTheme;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), nGlobalTheme))
	{
		return CRFLAG_BADPARAMS;
	}

	GlobalThemeEnable( game, nGlobalTheme );

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Enabled Global Theme, %s.", strToken );

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(sCmdLevelTheme)
{
	UNIT * player;
	if ( IS_SERVER(game) )
	{
		player = ClientGetControlUnit(client);
	} else {
		player = GameGetControlUnit( game );
	}
	ASSERT_RETVAL(player, CR_FAILED);

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nLevelTheme;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), nLevelTheme))
	{
		return CRFLAG_BADPARAMS;
	}

	LEVEL * pLevel = UnitGetLevel( player );

	LevelThemeForce( pLevel, nLevelTheme, TRUE );

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Enabled Level Theme, %s.", strToken );

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGlobalThemesDisplay)
{
	int nGlobalThemeCount = ExcelGetNumRows( EXCEL_CONTEXT(game), DATATABLE_GLOBAL_THEMES );
	BOOL bOneThemeEnabled = FALSE;
	for ( int i = 0; i < nGlobalThemeCount; i++ )
	{
		if ( GlobalThemeIsEnabled( game, i ) )
		{
			GLOBAL_THEME_DATA * pGlobalTheme = (GLOBAL_THEME_DATA *) ExcelGetData( EXCEL_CONTEXT(game), DATATABLE_GLOBAL_THEMES, i );
			WCHAR uzName[ DEFAULT_INDEX_SIZE ];
			PStrCvt( uzName, pGlobalTheme->szName, DEFAULT_INDEX_SIZE );
			ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Global Theme %s is enabled.", uzName );	
			bOneThemeEnabled = TRUE;
		}
	}

	if ( ! bOneThemeEnabled )
		ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"No Global Themes are enabled." );	

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdTownForced)
{

	ConsoleString(game, client, CONSOLE_SYSTEM_COLOR, L"Town Forced: %s.", sGetOnOffString(AppToggleDebugFlag(ADF_TOWN_FORCED)));

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCameraShake)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	float fDuration = 0.0f;
	float fMagnitude = 0.0f;
	float fDegrade = 0.0f;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	while (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken), L","))
	{
		if (PStrICmp(strToken, L"t=", 2) == 0)
		{
			if (!PStrToFloat(strToken + 2, fDuration))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid duration value.");
				return CR_FAILED;
			}
		}
		else if (PStrICmp(strToken, L"m=", 2) == 0)
		{
			if (!PStrToFloat(strToken + 2, fMagnitude))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid magnitude value.");
				return CR_FAILED;
			}
		}
		else if (PStrICmp(strToken, L"d=", 2) == 0)
		{
			if (!PStrToFloat(strToken + 2, fDegrade))
			{
				ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"Invalid degrade value.");
				return CR_FAILED;
			}
		}
		else
		{
			ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"invalid qualifier.");
			return CR_FAILED;
		}
	}

	if (fDuration == 0.0f || fMagnitude == 0.0f)
	{
		ConsoleString(game, client, CONSOLE_ERROR_COLOR, L"camera shake command requires duration (t=) and magnitude (m=) qualifiers.");
		return CR_FAILED;
	}

	c_CameraShakeRandom(game, RandGetVectorXY(game), fDuration, fMagnitude, fDegrade);
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCameraShakeOff)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	c_CameraShakeOFF();

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGWho)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (c_PlayerGetGuildChannelId() == INVALID_CHANNELID)
	{
		ConsoleString(ChatGetTypeColor(CHAT_TYPE_GUILD), L"You are not currently in a guild.");
		return CR_OK;
	}

	ConsoleString(ChatGetTypeColor(CHAT_TYPE_GUILD), L"Guild Members:");
	ConsoleString(ChatGetTypeColor(CHAT_TYPE_GUILD), L"========================================");

	const PLAYER_GUILD_MEMBER_DATA * itr = c_PlayerGuildMemberListGetNextMember(NULL);
	while (itr)
	{
		ConsoleString(
			ChatGetTypeColor(CHAT_TYPE_GUILD),
			L" %s \"%20s\" %s",
			(itr->idMemberCharacterId == c_PlayerGetGuildLeaderCharacterId()) ? L"*" : L" ",
			itr->wszMemberName,
			(itr->bIsOnline) ? L"ONLINE" : L"offline");

		itr = c_PlayerGuildMemberListGetNextMember(itr);
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGRenameRank)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR oldRankName[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, oldRankName, MAX_CHARACTER_NAME))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	WCHAR newRankName[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, newRankName, MAX_CHARACTER_NAME))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	GUILD_RANK targetRank = c_PlayerGetGuildRankFromRankString(oldRankName);
	if (targetRank == GUILD_RANK_INVALID)
	{
		UIAddChatLineArgs(
			CHAT_TYPE_SERVER,
			ChatGetTypeNameColor(CHAT_TYPE_SERVER),
			L"Unknown rank. Expecting: %s, or %s, or %s, or %s.",
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_LEADER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_OFFICER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_MEMBER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_RECRUIT));
		return CR_FAILED;
	}
	
	c_GuildChangeRankName(targetRank, newRankName);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGSettings)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	c_GuildPrintSettings();
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdGPerms)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR actionName[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, actionName, MAX_CHARACTER_NAME))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	WCHAR minRankName[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, minRankName, MAX_CHARACTER_NAME))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	GUILD_ACTION action;
	if (PStrICmp(actionName, L"promote") == 0) {
		action = GUILD_ACTION_PROMOTE;
	}
	else if (PStrICmp(actionName, L"demote") == 0) {
		action = GUILD_ACTION_DEMOTE;
	}
	else if (PStrICmp(actionName, L"email") == 0) {
		action = GUILD_ACTION_EMAIL_GUILD;
	}
	else if (PStrICmp(actionName, L"invite") == 0) {
		action = GUILD_ACTION_INVITE;
	}
	else if (PStrICmp(actionName, L"boot") == 0) {
		action = GUILD_ACTION_BOOT;
	}
	else {
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeNameColor(CHAT_TYPE_SERVER),
			L"Unknown guild action. Valid options: promote, demote, email, invite, boot");
		return CR_FAILED;
	}

	GUILD_RANK rank = c_PlayerGetGuildRankFromRankString(minRankName);
	if (rank == GUILD_RANK_INVALID)
	{
		UIAddChatLineArgs(
			CHAT_TYPE_SERVER,
			ChatGetTypeNameColor(CHAT_TYPE_SERVER),
			L"Unknown rank. Expecting: %s, or %s, or %s, or %s.",
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_LEADER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_OFFICER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_MEMBER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_RECRUIT));
		return CR_FAILED;
	}

	c_GuildChangeActionPermissions(action, rank);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUIEdit)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	GameToggleDebugFlag(game, DEBUGFLAG_UI_EDIT_MODE);
	BOOL bEnabled = GameGetDebugFlag(game, DEBUGFLAG_UI_EDIT_MODE);


	char chComponentName[MAX_CONSOLE_PARAM_VALUE_LEN];
	chComponentName[0] = 0;

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	if (sConsoleParseToken(cmd, strParams, strToken, arrsize(strToken)))
	{
		PStrCvt(chComponentName, strToken, arrsize(chComponentName));
		UIDebugEditSetComponent(chComponentName);
	}

	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_UIEDIT, (LPARAM)bEnabled);

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"UI Debug Edit: %s", sGetOnOffString(bEnabled));
		
	return CR_OK;
	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdCsrChat)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	strParams = PStrSkipWhitespace(strParams);
	if (!strParams[0])
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	if (!PlayerInCSRChat())
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_CLIENT_ERROR), 
			GlobalStringGet(GS_CSR_CHAT_NOT_AVAILABLE));
		
		return CR_FAILED;
	}

	CHAT_REQUEST_MSG_CSRSESSION_MESSAGE csrChatMessage;
	PStrCopy(csrChatMessage.wszChatMessage, strParams, MAX_CHAT_MESSAGE/sizeof(WCHAR));
	c_ChatNetSendMessage(&csrChatMessage, USER_REQUEST_CSRSESSION_MESSAGE);
	c_DisplayChat(CHAT_TYPE_CSR, gApp.m_wszPlayerName, L"", strParams, GlobalStringGet(GS_CSR_CSR));
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCsrChatStart)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	CHAT_REQUEST_MSG_CSRSESSION_START msg;
	PStrCopy(msg.wszTargetPlayer, strPlayer, MAX_CHARACTER_NAME);
	c_ChatNetSendMessage(
		&msg,
		USER_REQUEST_CSRSESSION_START);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCsrChatEnd)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	CHAT_REQUEST_MSG_CSRSESSION_END msg;
	PStrCopy(msg.wszTargetPlayer, strPlayer, MAX_CHARACTER_NAME);
	c_ChatNetSendMessage(
		&msg,
		USER_REQUEST_CSRSESSION_END);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCsrChatAdmin)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	strParams = PStrSkipWhitespace(strParams);
	if (!strParams[0])
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	CHAT_REQUEST_MSG_CSRSESSION_ADMIN_MESSAGE msg;
	PStrCopy(msg.wszTargetPlayer, strPlayer, MAX_CHARACTER_NAME);
	PStrCopy(msg.wszChatMessage, strParams, MAX_CHAT_MESSAGE/sizeof(WCHAR));
	c_ChatNetSendMessage(
		&msg,
		USER_REQUEST_CSRSESSION_ADMIN_MESSAGE);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdCsrRenameGuild)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	CHAT_REQUEST_MSG_CSR_RENAME_GUILD renameMsg;

	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, renameMsg.wszCurrentGuildName, arrsize(renameMsg.wszCurrentGuildName)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, renameMsg.wszNewGuildName, arrsize(renameMsg.wszNewGuildName)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	c_ChatNetSendMessage(
		&renameMsg,
		USER_REQUEST_CSR_RENAME_GUILD );
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
CONSOLE_CMD_FUNC(c_sCmdUIRepaint)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	UIRepaint();
		
	return CR_OK;
	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyInvite)
{
	if(AppIsSinglePlayer())
		return CR_OK;

	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	// invite player
	c_PlayerInvite( strPlayer );	

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
// TODO: error strings from non-development cheat codes need to be localizable
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyCancelInvite)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	CHAT_REQUEST_MSG_CANCEL_PARTY_INVITE uninviteMsg;
	uninviteMsg.TagInvitedMember = strPlayer;
	c_ChatNetSendMessage(&uninviteMsg, USER_REQUEST_CANCEL_PARTY_INVITE);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
// TODO: error strings from non-development cheat codes need to be localizable
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyDeclineInvite)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	PGUID idInviter = c_PlayerGetPartyInviter();
	if (idInviter != INVALID_CLUSTERGUID)
	{
		CHAT_REQUEST_MSG_DECLINE_PARTY_INVITE declineMsg;
		declineMsg.guidInvitingMember = idInviter;
		c_ChatNetSendMessage(&declineMsg, USER_REQUEST_DECLINE_PARTY_INVITE);
		c_PlayerClearPartyInvites();
	}
	else
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_NO_OUTSTANDING_PARTY_INVITES ) );
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
// TODO: error strings from non-development cheat codes need to be localizable
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyAcceptInvite)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	PGUID idInviter = c_PlayerGetPartyInviter();
	if (idInviter != INVALID_CLUSTERGUID)
	{
		CHAT_REQUEST_MSG_ACCEPT_PARTY_INVITE acceptMsg;
		acceptMsg.guidInvitingMember = idInviter;
		c_ChatNetSendMessage(&acceptMsg, USER_REQUEST_ACCEPT_PARTY_INVITE);
	}
	else
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_NO_OUTSTANDING_PARTY_INVITES ) );
	}

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
// TODO: error strings from non-development cheat codes need to be localizable
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyRemoveMember)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	if (c_PlayerGetPartyId() == INVALID_CHANNELID)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_NOT_IN_A_PARTY ) );
	}
	else if (c_PlayerGetPartyLeader() != gApp.m_characterGuid)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_ONLY_LEADER_MAY_BOOT_MEMBERS ) );
	}
	else
	{
		CHAT_REQUEST_MSG_REMOVE_PARTY_MEMBER removeMsg;
		removeMsg.MemberToRemove = strPlayer;
		c_ChatNetSendMessage(&removeMsg, USER_REQUEST_REMOVE_PARTY_MEMBER);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyDisband)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (c_PlayerGetPartyId() == INVALID_CHANNELID)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_NOT_IN_A_PARTY ) );
	}
	else if (c_PlayerGetPartyLeader() != gApp.m_characterGuid)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_ONLY_LEADER_MAY_DISBAND_PARTY ) );
	}
	else
	{
		CHAT_REQUEST_MSG_DISBAND_PARTY disbandMsg;
		c_ChatNetSendMessage(&disbandMsg, USER_REQUEST_DISBAND_PARTY);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyLeave)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	if (c_PlayerGetPartyId() == INVALID_CHANNELID)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_NOT_IN_A_PARTY ) );
	}
	else
	{
		CHAT_REQUEST_MSG_LEAVE_PARTY leaveMsg;
		c_ChatNetSendMessage(&leaveMsg, USER_REQUEST_LEAVE_PARTY);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyMessage)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	strParams = PStrSkipWhitespace(strParams);
	if (!strParams[0])
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	if (c_PlayerGetPartyId() == INVALID_CHANNELID)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_NOT_IN_A_PARTY ) );

		ConsoleSetChatContext(CHAT_TYPE_GAME);
	}
	else
	{
		c_SendPartyChannelChat(strParams, pLine);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartySetLeader)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	if (c_PlayerGetPartyId() == INVALID_CHANNELID)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_NOT_IN_A_PARTY ) );
	}
	else if (c_PlayerGetPartyLeader() != gApp.m_characterGuid)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_PARTY_ONLY_LEADER_MAY_PROMOTE ) );
	}
	else
	{
		CHAT_REQUEST_MSG_SET_PARTY_LEADER newLeaderMsg;
		newLeaderMsg.TagNewLeader = strPlayer;
		c_ChatNetSendMessage(&newLeaderMsg, USER_REQUEST_SET_PARTY_LEADER);
	}
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyAdvertise)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	strParams = PStrSkipWhitespace(strParams);
	if (!strParams[0])
		return (CR_FAILED | CRFLAG_BADPARAMS);

	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
		return CR_FAILED;

	c_SendPartyAdvertise(
		strParams,	//	party desc
		5,	//	max players
		MAX(UnitGetExperienceLevel(unit) - 3,  1),	//	min level
		MIN(UnitGetExperienceLevel(unit) + 3, 50)	//	max level
		);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyStopAdvertising)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	MSG_CCMD_PARTY_UNADVERTISE msg;
	c_SendMessage(CCMD_PARTY_UNADVERTISE, &msg);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPartyGetAdvertised)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	CHAT_REQUEST_MSG_GET_ADVERTISED_PARTIES msg;
	c_ChatNetSendMessage(&msg, USER_REQUEST_GET_ADVERTISED_PARTIES);

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildChat)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	strParams = PStrSkipWhitespace(strParams);
	if (!strParams[0])
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	if (c_PlayerGetGuildChannelId() == INVALID_CHANNELID)
	{
		UIAddChatLine(
			CHAT_TYPE_SERVER,
			ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet( GS_GUILD_NOT_IN_A_GUILD ) );
			
		ConsoleSetChatContext(CHAT_TYPE_GAME);
	}
	else
	{
		c_SendGuildChannelChat(strParams, pLine);
	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildCreate)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	if (!strParams[0])
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	MSG_CCMD_GUILD_CREATE msg;
	PStrCopy(msg.wszGuildName, MAX_CHAT_CNL_NAME, strParams, MAX_CHAT_CNL_NAME);
	c_SendMessage(CCMD_GUILD_CREATE, &msg);
	return CR_OK;
}
#endif
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildInvite)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	MSG_CCMD_GUILD_INVITE msg;
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, msg.wszPlayerToInviteName, MAX_CHARACTER_NAME))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	c_SendMessage(CCMD_GUILD_INVITE, &msg);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildDeclineInvite)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	MSG_CCMD_GUILD_DECLINE_INVITE msg;
	c_SendMessage(CCMD_GUILD_DECLINE_INVITE, &msg);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildAcceptInvite)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	MSG_CCMD_GUILD_ACCEPT_INVITE msg;
	c_SendMessage(CCMD_GUILD_ACCEPT_INVITE, &msg);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildLeaveGuild)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	MSG_CCMD_GUILD_LEAVE msg;
	c_SendMessage(CCMD_GUILD_LEAVE, &msg);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildRemove)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	MSG_CCMD_GUILD_REMOVE msg;
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, msg.wszPlayerToRemoveName, MAX_CHARACTER_NAME))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	c_SendMessage(CCMD_GUILD_REMOVE, &msg);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildPromote)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	MSG_CCMD_GUILD_CHANGE_RANK msg;
	msg.eTargetRank = (BYTE)GUILD_RANK_LEADER;
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, msg.wszTargetName, MAX_CHARACTER_NAME))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}
	c_SendMessage(CCMD_GUILD_CHANGE_RANK, &msg);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdGuildChangeRank)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	MSG_CCMD_GUILD_CHANGE_RANK msg;

	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, msg.wszTargetName, MAX_CHARACTER_NAME))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	GUILD_RANK targetRank = c_PlayerGetGuildRankFromRankString(strParams);
	if (targetRank == GUILD_RANK_INVALID)
	{
		UIAddChatLineArgs(
			CHAT_TYPE_SERVER,
			ChatGetTypeNameColor(CHAT_TYPE_SERVER),
			GlobalStringGet(GS_CCMD_GUILD_CHGRNK_ERROR),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_LEADER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_OFFICER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_MEMBER),
			c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_RECRUIT));
		return CR_FAILED;
	}
	msg.eTargetRank = (BYTE)targetRank;

	c_SendMessage(CCMD_GUILD_CHANGE_RANK, &msg);
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdPvPToggle)
{
	if( AppIsHellgate() )
	{
		ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
		UNIT *pPlayer = GameGetControlUnit(game);
		ASSERT_RETVAL( pPlayer, CR_FAILED);
		c_PlayerPvPToggle( pPlayer );	
		StateClearAllOfType(pPlayer, STATE_REMOVE_ON_PVP);	}
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(c_sCmdDuelInvite)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	WCHAR strPlayer[MAX_CHARACTER_NAME];
	if (!sConsoleParsePlayerNameParam(cmd, 0, strParams, strPlayer, arrsize(strPlayer)))
	{
		return (CR_FAILED | CRFLAG_BADPARAMS);
	}

	// invite player
	UNIT * pPlayer = PlayerGetByName(game, strPlayer);
	c_DuelInviteAttempt(pPlayer, strPlayer, TRUE);

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CMD_FUNC(s_sCmdAutoParty)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	UNIT *pPlayer = GameGetControlUnit(game);
	ASSERT_RETVAL( pPlayer, CR_FAILED);
	c_PlayerToggleAutoParty( pPlayer );
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdHome)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETVAL(player, CR_FAILED);

	ROOM * curRoom = UnitGetRoom(player);
	ASSERT_RETVAL(player, CR_FAILED);

	LEVEL * level = RoomGetLevel(curRoom);
	ASSERT_RETVAL(level, CR_FAILED);

	DWORD dwWarpFlags = 0;
	SETBIT(dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT);
	
	VECTOR position;
	VECTOR faceDirection;

	ROOM * room = LevelGetStartPosition(game, player, LevelGetID(level), position, faceDirection, dwWarpFlags);
	ASSERT_RETVAL(room, CR_FAILED);

	DWORD dwUnitWarpFlags = 0;
	SETBIT(dwUnitWarpFlags, UW_TRIGGER_EVENT_BIT);
	UnitWarp(player, room, position, faceDirection, cgvZAxis, dwUnitWarpFlags);
	s_UnitSetMode(player, MODE_IDLE);
	UnitSetZVelocity(player, 0.0f);
	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatInit)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	CLT_VERSION_ONLY(c_VoiceChatInit());

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatClose)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	CLT_VERSION_ONLY(c_VoiceChatClose());

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatUpdate)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	CLT_VERSION_ONLY(c_VoiceChatUpdate());

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatCreateRoom)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	CLT_VERSION_ONLY(c_VoiceChatCreateRoom());

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatLogin)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

#if !ISVERSION(SERVER_VERSION)
	WCHAR strUserName[MAX_CONSOLE_PARAM_VALUE_LEN];
	WCHAR strPassword[MAX_CONSOLE_PARAM_VALUE_LEN];
	sConsoleParseToken(cmd, strParams, strUserName, arrsize(strUserName), L",");
	sConsoleParseToken(cmd, strParams, strPassword, arrsize(strPassword), L",");
	
	c_VoiceChatLogin(strUserName, strPassword);
#endif

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatLogout)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	CLT_VERSION_ONLY(c_VoiceChatLogout());

	return CR_OK;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatStartHost)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	CLT_VERSION_ONLY(c_VoiceChatStartHost());

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatJoinHost)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	WCHAR strRoomID[MAX_CONSOLE_PARAM_VALUE_LEN];
	sConsoleParseToken(cmd, strParams, strRoomID, arrsize(strRoomID), L",");

	CLT_VERSION_ONLY(c_VoiceChatJoinHost(strRoomID));

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatJoinRoom)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);
	WCHAR strRoomID[MAX_CONSOLE_PARAM_VALUE_LEN];
	sConsoleParseToken(cmd, strParams, strRoomID, arrsize(strRoomID), L",");

	CLT_VERSION_ONLY(c_VoiceChatJoinRoom(strRoomID));

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if 1
CONSOLE_CMD_FUNC(s_cCmdVoiceChatLeaveRoom)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), CR_FAILED);

	CLT_VERSION_ONLY(c_VoiceChatLeaveRoom());

	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CMD_FUNC(s_sCmdFreeLevel)
{
	ASSERT_RETVAL(game && IS_SERVER(game), CR_FAILED);

	UNIT * pUnit = ClientGetControlUnit(client);
	if(!pUnit)
	{
		return CR_FAILED;
	}

	LEVEL * pClientLevel = UnitGetLevel(pUnit);

	WCHAR strLevel[MAX_CONSOLE_PARAM_VALUE_LEN];
	int level;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strLevel, arrsize(strLevel), level))
	{
		sConsolePrintMissingOrInvalidValue(game, client, L"level", strLevel);
		return CR_FAILED;
	}

	LEVEL * pLevel = DungeonGetFirstLevel(game);
	while(pLevel)
	{
		if(pLevel != pClientLevel && LevelGetDefinitionIndex(pLevel) == level)
		{
			DungeonRemoveLevel(game, pLevel);
			break;
		}
		pLevel = DungeonGetNextLevel(pLevel);
	}

	return CR_OK;
}
#endif



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
#include "c_quests.h"
#endif

CONSOLE_CMD_FUNC(s_cCmdDebugNPCVideo)
{
	if( AppIsTugboat() )
	{
		return CR_FAILED;
	}
#if !ISVERSION(SERVER_VERSION)

	//MSG_SCMD_QUEST_DISPLAY_DIALOG tMessage;
	//tMessage.nQuest = QUEST_DEMOEND;

	//UNIT * player = GameGetControlUnit( game );
	//LEVEL * pLevel = UnitGetLevel( player );
	//TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	//UNIT * pMurmur = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, GlobalIndexGet( GI_MONSTER_MURMUR ) );
	//if ( !pMurmur )
	//	return CR_FAILED;

	//tMessage.idQuestGiver = UnitGetId( pMurmur );
	//tMessage.nDialog = DIALOG_DEMOEND_DESCRIPTION;
	//tMessage.nDialogReward = INVALID_LINK;
	//tMessage.nDialogType = NPC_DIALOG_OK;
	//tMessage.nGISecondNPC = GI_INVALID;
	//c_QuestDisplayDialog( game, &tMessage );

	NPC_DIALOG_SPEC spec;
	spec.eGITalkingTo = GI_MONSTER_TIME_ORB_CABALIST;
	spec.nDialog = DIALOG_TIME_ORB_VIDEO;
	spec.eType = NPC_VIDEO_INCOMING;
	UIDisplayNPCVideoAndText(game, &spec);

	//UNIT * player = GameGetControlUnit( game );
	//LEVEL * pLevel = UnitGetLevel( player );
	//TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	//UNIT * pTalkingTo = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, GlobalIndexGet( GI_MONSTER_LORD_ARPHAUN ) );
	//if ( !pTalkingTo )
	//	return CR_FAILED;

	//spec.idTalkingTo = UnitGetId(pTalkingTo);
	//spec.eGITalkingTo = GI_MONSTER_AERON_ALTAIR;
	//spec.nDialog = DIALOG_HELPRESCUE_VIDEO;
	//spec.eType = NPC_DIALOG_OK_CANCEL;

	//UIDisplayNPCDialog(game, &spec);
#endif
	return CR_OK;

	//UNIT * player = GameGetControlUnit( game );
	//LEVEL * pLevel = UnitGetLevel( player );
	//TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	//UNIT * pLucious = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, GlobalIndexGet( GI_MONSTER_LUCIOUS ) );
	//if ( !pLucious )
	//	return CR_FAILED;

	//NPC_DIALOG_SPEC spec;
	//spec.idTalkingTo = UnitGetId( pLucious );
	//spec.eGITalkingTo = GI_MONSTER_TECHSMITH_314;
	//spec.nDialog = DIALOG_CHOCOLATE_LUCIOUS_GOCP;
	//spec.eType = NPC_DIALOG_TWO_OK;
	//UIDisplayNPCDialog(game, &spec);

	//return CR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_cCmdDebugTracker)
{
	if( AppIsTugboat() )
	{
		return CR_FAILED;
	}
#if !ISVERSION(SERVER_VERSION)
	UIQuestTrackerEnable(TRUE);

	VECTOR vec(0.0f, 0.0f, 0.0f);

	int nID = 0;
	sConsoleParseIntegerParam(cmd, 0, strParams, nID);

	if (sConsoleParseFloatParam(cmd, 1, strParams, vec.fX))
	{
		sConsoleParseFloatParam(cmd, 2, strParams, vec.fY);
		sConsoleParseFloatParam(cmd, 3, strParams, vec.fZ);
	}
	else
	{
		UNIT* pPlayer = GameGetControlUnit(game);
		vec = pPlayer->vPosition;
	}

	UIQuestTrackerUpdatePos(nID, vec);

#endif
	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_cCmdDebugTrackerRemove)
{
	if( AppIsTugboat() )
	{
		return CR_FAILED;
	}
#if !ISVERSION(SERVER_VERSION)

	int nID = 0;
	sConsoleParseIntegerParam(cmd, 0, strParams, nID);

	UIQuestTrackerRemove(nID);

#endif
	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_cCmdDebugTrackerClose)
{
	if( AppIsTugboat() )
	{
		return CR_FAILED;
	}
#if !ISVERSION(SERVER_VERSION)

	UIQuestTrackerEnable(FALSE);

#endif
	return CR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if(ISVERSION(DEVELOPMENT))
CONSOLE_CMD_FUNC(s_cCmdRefreshBuddyList)
{
#if !ISVERSION(SERVER_VERSION)
	c_ChatNetRefreshBuddyListInfo();
#endif
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if(ISVERSION(DEVELOPMENT))
CONSOLE_CMD_FUNC(s_cCmdRefreshGuildList)
{
#if !ISVERSION(SERVER_VERSION)
	c_ChatNetRefreshGuildListInfo();
#endif
	return CR_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(s_sCmdDialog)
{
#if !ISVERSION(SERVER_VERSION)

	WCHAR strToken[MAX_CONSOLE_PARAM_VALUE_LEN];
	int nDialog;
	if (!sConsoleParseExcelLookup(game, cmd, 0, strParams, strToken, arrsize(strToken), nDialog))
	{
		// CHB 2007.03.04 - To match the conditional of sConsolePrintMissingOrInvalidValue().
#if ISVERSION(DEVELOPMENT)
		sConsolePrintMissingOrInvalidValue(game, client, L"dialog", strToken);
#endif
		return CR_FAILED;
	}

	UNIT *pPlayer = GameGetControlUnit( game );
	NPC_DIALOG_SPEC spec;
	spec.idTalkingTo = UnitGetId( pPlayer );
	spec.nDialog = nDialog;
	spec.eType = NPC_VIDEO_OK;
	UIDisplayNPCDialog(game, &spec);
	
#endif

	return CR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CONSOLE_CMD_FUNC(c_sCmdAHSearch)
{
#if !ISVERSION(SERVER_VERSION)
	MSG_CCMD_AH_SEARCH_ITEMS msgSearch;

	msgSearch.ItemType = 336;
	msgSearch.ItemMinLevel = 25;
	msgSearch.ItemMaxLevel = 30;
	msgSearch.ItemMaxPrice = 0;
	msgSearch.ItemMinPrice = 0;
	msgSearch.ItemMinQuality = 0;
	msgSearch.ItemSortMethod = AUCTION_RESULT_SORT_BY_PRICE;

	c_SendMessage(CCMD_AH_SEARCH_ITEMS, &msgSearch);


#endif
	return CR_OK;
}


#pragma warning(default : 4100)	//	unreferenced formal parameters

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// END CALLBACK SECTION
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sCommandTableFindCommand(
	const WCHAR * strCommand,
	BOOL * pbIsExact = NULL)
{
	CONSOLE_COMMAND * min = g_ConsoleCommandTable.m_pCommandTable;
	CONSOLE_COMMAND * max = g_ConsoleCommandTable.m_pCommandTable + g_ConsoleCommandTable.m_nCommandCount;
	CONSOLE_COMMAND * ii = min + (max - min) / 2;

	while (max > min)
	{
		int val = PStrICmp(ii->m_strCommand.GetStr(), strCommand, MAX_CONSOLE_COMMAND_LEN);
		if (val > 0)
		{
			max = ii;
		}
		else if (val < 0)
		{
			min = ii + 1;
		}
		else
		{
			if (pbIsExact)
			{
				*pbIsExact = TRUE;
			}
			return SIZE_TO_INT(ii - g_ConsoleCommandTable.m_pCommandTable);
		}
		ii = min + (max - min) / 2;
	}
	if (pbIsExact)
	{
		*pbIsExact = FALSE;
	}
	return SIZE_TO_INT(max - g_ConsoleCommandTable.m_pCommandTable);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sConsoleParamFindValue(
	const CONSOLE_PARAM * param,
	const WCHAR * strValue,
	BOOL * pbIsExact = NULL)
{
	ASSERT_RETZERO(param);
	ASSERT_RETZERO(strValue);

	PStrW * min = param->m_pstrValues;
	PStrW * max = param->m_pstrValues + param->m_nValues;
	PStrW * ii = min + (max - min) / 2;

	while (max > min)
	{
		int val = PStrICmp(ii->GetStr(), strValue, MAX_CONSOLE_PARAM_VALUE_LEN);
		if (val > 0)
		{
			max = ii;
		}
		else if (val < 0)
		{
			min = ii + 1;
		}
		else
		{
			if (pbIsExact)
			{
				*pbIsExact = TRUE;
			}
			return SIZE_TO_INT(ii - param->m_pstrValues);
		}
		ii = min + (max - min) / 2;
	}
	if (pbIsExact)
	{
		*pbIsExact = FALSE;
	}
	return SIZE_TO_INT(max - param->m_pstrValues);
}


//----------------------------------------------------------------------------
// return false if the cmd isn't applicable to the game settings, or game 
// and app type
//----------------------------------------------------------------------------
static BOOL sConsoleCommandIsApplicable(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	BOOL bCheckAppAndGameType = TRUE)
{
	ASSERT_RETFALSE(cmd);

	if (!cmd->m_fCmdFunction)
	{
		return FALSE;
	}
#if ISVERSION(RELEASE_CHEATS_ENABLED) && !ISVERSION(DEVELOPMENT)
	
	BOOL hasPermission = FALSE;
	/* Can't just return on not having badge, cause some cheats are allowed for multiple types account types */
	if((cmd->m_isCheat & CHEATS_RELEASE_TESTER) == CHEATS_RELEASE_TESTER)
	{
		ASSERT_RETFALSE(game);
		if(UnitHasBadge(sGetControlUnit(game, client), ACCT_TITLE_TESTER))
			hasPermission = TRUE;
	}
	if((cmd->m_isCheat & CHEATS_RELEASE_CSR) == CHEATS_RELEASE_CSR)
	{
		ASSERT_RETFALSE(game);
		if(UnitHasBadge(sGetControlUnit(game, client), ACCT_TITLE_CUSTOMER_SERVICE_REPRESENTATIVE))
			hasPermission = TRUE;
	}
	if(!hasPermission && cmd->m_isCheat)
		return FALSE;
#else
	UNREFERENCED_PARAMETER(client);
#endif
	
	if (game && !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS) && ((cmd->m_isCheat & CHEATS_NORELEASE) == CHEATS_NORELEASE))
	{
		return FALSE;
	}



	if (bCheckAppAndGameType)
	{
		if (cmd->m_eAppType == COMMAND_SERVER && AppGetType() != APP_TYPE_CLOSED_SERVER)
		{
			return FALSE;
		}
		if (game && cmd->m_eGameType == COMMAND_CLIENT && !IS_CLIENT(game))
		{
			return FALSE;
		}
		else if (game && cmd->m_eGameType == COMMAND_SERVER && !IS_SERVER(game))
		{
			return FALSE;
		}
	}

	// Check restricted emotes
	if (cmd->m_fCmdFunction == c_sCmdEmoteSpecific)
	{
		if (!game || !IS_CLIENT(game))
		{
			return FALSE;
		}

		UNIT *pPlayer = GameGetControlUnit(game);
		if (!pPlayer)
		{
			return FALSE;
		}

		return x_EmoteCanPerform(pPlayer, cmd->m_nData);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// for a given input, return true if first line is a console command indicator
// plus advance input past indicator & whitespace
//----------------------------------------------------------------------------
static BOOL sConsoleCommandEatSlash(
	const WCHAR * & strInput)
{
	ASSERT_RETFALSE(strInput);

	if (strInput[0] != CONSOLE_COMMAND_PREFIX)
	{
		return FALSE;
	}
	++strInput;
	strInput = (WCHAR *)PStrSkipWhitespace(strInput);
	return TRUE;
}


//----------------------------------------------------------------------------
// for a given input, return the matching console command if there is one,
// advance the input past the command & whitespace
//----------------------------------------------------------------------------
static const CONSOLE_COMMAND * sConsoleGetConsoleCommand(
	GAME * game,
	GAMECLIENT * client,
	const WCHAR * & strInput)
{
	ASSERT_RETNULL(strInput);

	if (!sConsoleCommandEatSlash(strInput))
	{
		return NULL;
	}

	WCHAR strCommand[MAX_CONSOLE_COMMAND_LEN];
	int bufLen = CONSOLE_MAX_STR;
	const WCHAR * str = strInput;
	if ((str = PStrTok(strCommand, str, L", ", arrsize(strCommand), bufLen)) == NULL)
	{
		return NULL;
	}
	str = PStrSkipWhitespace(str);

	BOOL isExact = FALSE;
	unsigned int ii = sCommandTableFindCommand(strCommand, &isExact);
	if (!isExact)
	{
		return NULL;
	}
	ASSERT_RETNULL(ii < g_ConsoleCommandTable.m_nCommandCount);

	const CONSOLE_COMMAND * cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
	ASSERT_RETNULL(cmd);	

	if (!sConsoleCommandIsApplicable(game, client, cmd, TRUE))
	{
		return NULL;
	}

	strInput = str;

	return cmd;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sConsoleVerifyParams(
	GAME * game,
	GAMECLIENT * client,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * strInput)
{
	REF(game);
	REF(client);
	REF(cmd);
	REF(strInput);
	return CR_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ConsoleParseScript(
	GAME * game,
	GAMECLIENT * client,
	const WCHAR * strInput)
{
#if (ISVERSION(DEVELOPMENT))	
	ONCE
	{
		if (game == NULL || !IS_SERVER(game) || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
		{
			break;
		}
		if (!strInput)
		{
			break;
		}

		char chInput[CONSOLE_MAX_STR];
		PStrCvt(chInput, strInput, arrsize(chInput));

		BYTE_STREAM * stream = ByteStreamInit(NULL);
		ASSERT_BREAK(stream);

		if (ScriptParseEx(game->m_vm, stream, chInput, ERROR_LOG, "console input"))
		{
			GAME *clientGame = AppGetCltGame();
			if (!clientGame)
				clientGame = game;

			UNIT * unit = GameGetDebugUnit(clientGame);
			if (unit)
			{
				unit = UnitGetById(game, UnitGetId(unit));
			}
			if (!unit)
			{
				unit = ClientGetControlUnit(client);
			}
			int result = VMExecI(game, unit, stream);
			trace("%d\n", result);

			return (CR_OK | CRFLAG_SCRIPT);
		}
		ByteStreamFree(stream);
	}
#else
	REF(game);
	REF(client);
	REF(strInput);
#endif // DEVELOPMENT
	return CR_NOT_COMMAND;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ConsoleParseChat(
	const WCHAR * strInput,
	UI_LINE * pLine)
{
#if !ISVERSION(SERVER_VERSION)
	if (!sConsoleCommandEatSlash(strInput))
	{
		return CR_NOT_COMMAND;
	}
	WCHAR strCommand[MAX_CONSOLE_COMMAND_LEN];
	int bufLen = CONSOLE_MAX_STR;
	const WCHAR * str = strInput;
	if ((str = PStrTok(strCommand, str, L", ", arrsize(strCommand), bufLen)) == NULL)
	{
		return CR_NOT_COMMAND;
	}
	str = PStrSkipWhitespace(str);

	if(ChannelMapHasInstancingChannel(strCommand))
	{
		c_SendInstancingChannelChat(
			strCommand,
			str,
			pLine);
		return CR_OK | CRFLAG_NOSEND;
	}
	else return CR_NOT_COMMAND;

#else
	UNREFERENCED_PARAMETER(strInput);
	UNREFERENCED_PARAMETER(pLine);
	ASSERT_RETVAL(FALSE, CR_NOT_COMMAND);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void ConsoleAddItem(
	UNIT *pItem)
{
	const WCHAR * strLine = ConsoleGetInputLine();
	if (!strLine)
		return;

	WCHAR strName[CONSOLE_MAX_STR];
	UnitGetName(pItem, strName, CONSOLE_MAX_STR, 0);
	int	nLinelen = PStrLen(strLine);
	if (nLinelen + 20 < CONSOLE_MAX_STR)
	{
		WCHAR strOut[CONSOLE_MAX_STR];
		PStrPrintf(strOut, CONSOLE_MAX_STR, L"<item color=Blue text=\"[%s]\" id=%d>", strName, UnitGetId(pItem));
		ConsoleSetInputLine(strOut, GFXCOLOR_GRAY, nLinelen);
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sValidateCommandExecute(
	GAME *pGame,
	GAMECLIENT *pClient,
	const CONSOLE_COMMAND *pCommand)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pCommand, "Expected console command" );
	
	if (pCommand->m_dwCommandFlags != 0)
	{
		UNIT *pPlayer = NULL;
		if (IS_CLIENT( pGame ))
		{
#if !ISVERSION(SERVER_VERSION)
			pPlayer = UIGetControlUnit();
#endif
		}
		else
		{
			pPlayer = ClientGetControlUnit( pClient );
		}

		// check each of the console command flags
		if (pCommand->m_dwCommandFlags & CCF_DEAD_RESTRICTED)
		{
			ASSERTX_RETFALSE( pPlayer, "Expected player" );
			if (IsUnitDeadOrDying( pPlayer ))
			{
				return FALSE;
			}
		}
				
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ConsoleParseCommand(
	GAME * game,
	GAMECLIENT * client,
	const WCHAR * strInput,
	UI_LINE * pLine)
{
	ASSERT_RETVAL(game, CR_NOT_COMMAND);	
	ASSERT_RETVAL(client || IS_CLIENT(game), CR_NOT_COMMAND);
	ASSERT_RETVAL(strInput, CR_NOT_COMMAND);

	const CONSOLE_COMMAND * cmd = sConsoleGetConsoleCommand(game, client, strInput);
	//Security
	if (!cmd)
	{
		if (!strInput)
		{
			return CR_NOT_COMMAND;
		}

		return ConsoleParseScript(game, client, strInput);
	}
	ASSERT_RETVAL(strInput, CR_NOT_COMMAND);

	DWORD result = sConsoleVerifyParams(game, client, cmd, strInput);
	if (!CRSUCCEEDED(result))
	{
/*
		if (game && IS_CLIENT(game))
		{
			#if !ISVERSION(SERVER_VERSION)
			ConsoleString(ConsoleErrorColor(), GlobalSTringGet( GS_CONSOLE_INVALID_PARAMS ));
			#endif
		}
*/
		return result;
	}

	ASSERT_RETVAL(strInput, CR_NOT_COMMAND);
	ASSERT_RETVAL(cmd->m_fCmdFunction, CR_NOT_COMMAND);

	// verify that we can run this function
	if (sValidateCommandExecute( game, client, cmd ) == FALSE)
	{
		return FALSE;
	}
	
	result = cmd->m_fCmdFunction(game, client, cmd, strInput, pLine);

	if (!game)
	{
		return result;
	}

	if (CRBADPARAMS(result))
	{
#if !ISVERSION(SERVER_VERSION)
		if (IS_CLIENT(game) && cmd->m_eGameType == COMMAND_CLIENT)
		{
			UIAddChatLine(CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_CLIENT_ERROR), GlobalStringGet( GS_CONSOLE_INVALID_PARAMS ));
			
			c_sPrintCommandCommandHelp(game, client, cmd);
		}
#endif
	}

	if (IS_CLIENT(game) && CRSUCCEEDED(result) &&
		cmd->m_eGameType == COMMAND_CLIENT)
	{
		result |= CRFLAG_NOSEND;
	}

	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteSetNewParam(
	const WCHAR * strLine,
	const WCHAR * strParamStart,
	const WCHAR * strNewParam)
{
	WCHAR strNewInput[CONSOLE_MAX_STR];
	size_t offs = strParamStart - strLine;
	ASSERT_RETFALSE(offs < arrsize(strNewInput));
	PStrCopy(strNewInput, arrsize(strNewInput), strLine, (int)offs + 1);
	PStrCopy(strNewInput + offs, strNewParam, arrsize(strNewInput) - (int)offs);

	ConsoleSetInputLine(strNewInput, ConsoleInputColor(), 1);
	return TRUE;
}
#endif	// SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamBoolean(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);	

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	
	if (strParam[0] == L'0')
	{
		PStrCopy(strNewParam, L"1", arrsize(strNewParam));
	}
	else if (strParam[0] == L'1')
	{
		PStrCopy(strNewParam, L"0", arrsize(strNewParam));
	}
	else if (strParam[0] == L't' || strParam[0] == L'T')
	{
		if (PStrICmp(strParam, L"true", CONSOLE_MAX_STR) < 0)
		{
			PStrCopy(strNewParam, L"true", arrsize(strNewParam));
		}
		else
		{
			PStrCopy(strNewParam, L"false", arrsize(strNewParam));
		}
	}
	else if (strParam[0] == L'f' || strParam[0] == L'F')
	{
		if (PStrICmp(strParam, L"false", CONSOLE_MAX_STR) < 0 || PStrICmp(strParam, L"true", CONSOLE_MAX_STR) >= 0)
		{
			PStrCopy(strNewParam, L"false", arrsize(strNewParam));
		}
		else
		{
			PStrCopy(strNewParam, L"true", arrsize(strNewParam));
		}
	}
	else if (strParam[0] == L'o' || strParam[0] == L'O')
	{
		if (PStrICmp(strParam, L"off", CONSOLE_MAX_STR) < 0 || PStrICmp(strParam, L"on", CONSOLE_MAX_STR) >= 0)
		{
			PStrCopy(strNewParam, L"off", arrsize(strNewParam));
		}
		else 
		{
			PStrCopy(strNewParam, L"on", arrsize(strNewParam));
		}
	}
	else if (bReverse)
	{
		PStrCopy(strNewParam, L"0", arrsize(strNewParam));
	}
	else
	{
		PStrCopy(strNewParam, L"1", arrsize(strNewParam));
	}

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamDatatable(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);
	ASSERT_RETFALSE(param->m_data1 >= 0 && param->m_data1 < NUM_DATATABLES);

	char chParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chParam, strParam, arrsize(chParam));

	EXCELTABLE table = (EXCELTABLE)param->m_data1;
	ASSERTV_RETFALSE(table < (int)ExcelGetTableCount(), "sConsoleAutoCompleteParamDatatable(): INVALID TABLE INDEX.  COMMAND:%s", cmd->m_strCommand);
	int line = bReverse ? ExcelGetPrevLineByStrKey(game, table, chParam) : ExcelGetNextLineByStrKey(game, table, chParam);
	if (line < 0)
	{
		return FALSE;
	}

	const char * pszIndex = ExcelGetStringIndexByLine(game, table, line);
	if (!pszIndex)
	{
		return FALSE;
	}

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strNewParam, pszIndex, arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamDatatableByName(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);
	ASSERT_RETFALSE(param->m_data1 >= 0 && param->m_data1 < NUM_DATATABLES);

	EXCELTABLE table = (EXCELTABLE)param->m_data1;
	unsigned int rowCount = ExcelGetNumRows(game, table);
	int closest = INVALID_LINK;

	for (unsigned int ii = 0; ii < rowCount; ++ii)
	{
		const WCHAR * strName = ExcelGetRowNameString(game, table, ii);
		if (strName == NULL)
			continue;

		int compare = PStrICmp(strParam, strName, CONSOLE_MAX_STR);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}

		if (closest == INVALID_LINK)
		{
			closest = ii;
		} 
		else 
		{
			const WCHAR * strBest = ExcelGetRowNameString(game, table, closest);
			int delta = PStrICmp(strName, strBest);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				closest = ii;
			}
		}
	}
	if (closest == INVALID_LINK)
	{
		return FALSE;
	}

	const WCHAR * strBest = ExcelGetRowNameString(game, table, closest);

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strBest);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamDatatableByStr(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);
	ASSERT_RETFALSE(param->m_data1 >= 0 && param->m_data1 < NUM_DATATABLES);

	EXCELTABLE table = (EXCELTABLE)param->m_data1;
	unsigned int rowCount = ExcelGetNumRows(game, table);
	int closest = INVALID_LINK;

	for (unsigned int ii = 0; ii < rowCount; ++ii)
	{
		const BYTE * data = (const BYTE *)ExcelGetData(game, table, ii);
		const WCHAR * strName = (const WCHAR *)(data + (unsigned int)param->m_data1);
		ASSERT_RETFALSE(strName);
		int compare = PStrICmp(strParam, strName, CONSOLE_MAX_STR);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}

		if (closest == INVALID_LINK)
		{
			closest = ii;
		} 
		else 
		{
			const BYTE * data = (const BYTE *)ExcelGetData(game, table, closest);
			const WCHAR * strBest = (const WCHAR *)(data + (unsigned int)param->m_data1);
			int delta = PStrICmp(strName, strBest);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				closest = ii;
			}
		}
	}
	if (closest == INVALID_LINK)
	{
		return FALSE;
	}

	const BYTE * data = (const BYTE *)ExcelGetData(game, table, closest);
	const WCHAR * strBest = (const WCHAR *)(data + (unsigned int)param->m_data1);

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strBest);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamTablename(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);

	char chParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chParam, strParam, arrsize(chParam));

	int closest = INVALID_LINK;

	for (unsigned int ii = 0; ii < NUM_DATATABLES; ++ii)
	{
		const char * chTableName = ExcelGetTableName((EXCELTABLE)ii);
		ASSERT_BREAK(chTableName);
		if (!chTableName[0])
		{
			continue;
		}

		int compare = PStrICmp(chParam, chTableName, MAX_CONSOLE_PARAM_VALUE_LEN);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}
		if (closest == INVALID_LINK)
		{
			closest = ii;
		} 
		else 
		{
			const char * chClosest = ExcelGetTableName((EXCELTABLE)closest);
			ASSERT_BREAK(chClosest);

			int delta = PStrICmp(chTableName, chClosest, MAX_CONSOLE_PARAM_VALUE_LEN);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				closest = ii;
			}
		}
	}
	if (closest == INVALID_LINK)
	{
		return FALSE;
	}

	const char * chClosest = ExcelGetTableName((EXCELTABLE)closest);
	ASSERT_RETFALSE(chClosest && chClosest[0]);

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strNewParam, chClosest, arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamPerf(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);

	char chParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chParam, strParam, arrsize(chParam));

	int closest = INVALID_LINK;

	for (unsigned int ii = 0; ii < PERF_LAST; ++ii)
	{
		if (ii < PERF_LAST - 1)
		{
			if (gPerfTable[ii].spaces >= gPerfTable[ii+1].spaces)
			{
				continue;
			}
		}
		int compare = PStrICmp(chParam, gPerfTable[ii].label, MAX_CONSOLE_PARAM_VALUE_LEN);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}
		if (closest == INVALID_LINK)
		{
			closest = ii;
		} 
		else 
		{
			int delta = PStrICmp(gPerfTable[ii].label, gPerfTable[closest].label, MAX_CONSOLE_PARAM_VALUE_LEN);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				closest = ii;
			}
		}
	}
	if (closest == INVALID_LINK)
	{
		return FALSE;
	}

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strNewParam, gPerfTable[closest].label, arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamHitcount(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);

	char chParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chParam, strParam, arrsize(chParam));

	int closest = INVALID_LINK;

	for (unsigned int ii = 0; ii < HITCOUNT_LAST; ++ii)
	{
		if (ii < HITCOUNT_LAST - 1)
		{
			if (gHitCountTable[ii].spaces >= gHitCountTable[ii+1].spaces)
			{
				continue;
			}
		}
		int compare = PStrICmp(chParam, gHitCountTable[ii].label, MAX_CONSOLE_PARAM_VALUE_LEN);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}
		if (closest == INVALID_LINK)
		{
			closest = ii;
		} 
		else 
		{
			int delta = PStrICmp(gHitCountTable[ii].label, gHitCountTable[closest].label, MAX_CONSOLE_PARAM_VALUE_LEN);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				closest = ii;
			}
		}
	}
	if (closest == INVALID_LINK)
	{
		return FALSE;
	}

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strNewParam, gHitCountTable[closest].label, arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamStringList(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(param->m_nValues > 0);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);

	BOOL isExact = FALSE;
	unsigned int ii = sConsoleParamFindValue(param, strParam, &isExact) % param->m_nValues;

	int dir = (bReverse ? -1 : 1);
	if (isExact)
	{
		ii = UINT_ROTATE(ii, param->m_nValues, dir);
	}

	PStrW * str = param->m_pstrValues + ii;
	ASSERT_RETFALSE(str->Len() > 0);

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCopy(strNewParam, str->GetStr(), arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
struct CONSOLE_DEFINITION_ITERATE
{
	const char *	chPath;
	PStrA * *		ppstrParticles;
	unsigned int *	pnParticles;
};
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sConsoleIterateDefinitionCallback(
	const FILE_ITERATE_RESULT * resultFileFind,
	void * userData)
{
	CONSOLE_DEFINITION_ITERATE * iterData = (CONSOLE_DEFINITION_ITERATE *)userData;
	
	ASSERT_RETURN(resultFileFind && resultFileFind->szFilenameRelativepath);

	if (*iterData->pnParticles % CONSOLE_DEFLIST_BUFSIZE == 0)
	{
		*iterData->ppstrParticles = (PStrA *)REALLOCZ(NULL, *iterData->ppstrParticles, sizeof(PStrA) * (*iterData->pnParticles + CONSOLE_DEFLIST_BUFSIZE));
		ASSERT_RETURN(*iterData->ppstrParticles);
	}
	++(*iterData->pnParticles);

	char chTemp[MAX_PATH];
	PStrRemoveExtension(chTemp, arrsize(chTemp), resultFileFind->szFilenameRelativepath);
	PStrRemovePath(chTemp, arrsize(chTemp), iterData->chPath, chTemp);

	PStrA * str = (*iterData->ppstrParticles) + *iterData->pnParticles - 1;
	str->Copy(chTemp, MAX_PATH);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleGetParticleList(
	int definitionGroup,
	PStrA * & pstrParticles,
	unsigned int & nParticles)
{
	CDefinitionContainer * defContainer = DefinitionContainerGetRef(definitionGroup);
	ASSERT_RETFALSE(defContainer);

	CONSOLE_DEFINITION_ITERATE iterData;
	iterData.chPath = defContainer->GetFilePath();
	iterData.ppstrParticles = &pstrParticles;
	iterData.pnParticles = &nParticles;

	FilesIterateInDirectory(defContainer->GetFilePath(), "*.xml", sConsoleIterateDefinitionCallback, &iterData, TRUE);

	return TRUE;
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamDefinition(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);

	if (g_ConsoleCommandTable.m_nParticles == 0)
	{
		ASSERT_RETFALSE(sConsoleGetParticleList((int)param->m_data1, g_ConsoleCommandTable.m_pstrParticles, g_ConsoleCommandTable.m_nParticles));
	}
	ASSERT_RETFALSE(g_ConsoleCommandTable.m_pstrParticles && g_ConsoleCommandTable.m_nParticles > 0);

	char chParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(chParam, strParam, arrsize(chParam));

	int closest = INVALID_LINK;

	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nParticles; ++ii)
	{
		int compare = PStrICmp(chParam, g_ConsoleCommandTable.m_pstrParticles[ii].GetStr(), MAX_PATH);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}
		if (closest == INVALID_LINK)
		{
			closest = ii;
		} 
		else 
		{
			int delta = PStrICmp(g_ConsoleCommandTable.m_pstrParticles[ii].GetStr(), g_ConsoleCommandTable.m_pstrParticles[closest].GetStr(), MAX_PATH);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				closest = ii;
			}
		}
	}

	if (closest == INVALID_LINK)
	{
		return FALSE;
	}

	WCHAR strNewParam[MAX_PATH];
	PStrCvt(strNewParam, g_ConsoleCommandTable.m_pstrParticles[closest].GetStr(), arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParamTeam(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);

	unsigned int teamCount = TeamsGetTeamCount(game);

	int closest = INVALID_LINK;

	for (unsigned int ii = 0; ii < teamCount; ++ii)
	{
		const WCHAR * strTeamName = TeamGetName(game, ii);
		if (!strTeamName || strTeamName[0] == '\0')
			continue;

		int compare = PStrICmp(strParam, strTeamName, MAX_CONSOLE_PARAM_VALUE_LEN);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}
		if (closest == INVALID_LINK)
		{
			closest = ii;
		} 
		else 
		{
			const WCHAR * strTeamNameClosest = TeamGetName(game, closest);
			if (strTeamNameClosest && strTeamNameClosest[0])
			{
				int delta = PStrICmp(strTeamName, strTeamNameClosest, MAX_CONSOLE_PARAM_VALUE_LEN);
				if (bReverse ? (delta > 0) : (delta < 0))
				{
					closest = ii;
				}
			}
		}
	}

	if (closest == INVALID_LINK)
	{
		return FALSE;
	}

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	const WCHAR * strTeamNameClosest = TeamGetName(game, closest);
	if (strTeamNameClosest && strTeamNameClosest[0])	
		PStrCopy(strNewParam, strTeamNameClosest, arrsize(strNewParam));
	else
		PStrCopy(strNewParam, L"", arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteConsoleCommand(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);

	const PStrW * strClosest = NULL;

	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nCategories; ++ii)
	{
		const PStrW * strCat = g_ConsoleCommandTable.m_pstrCategories[ii];
		
		int compare = PStrICmp(strParam, strCat->GetStr(), MAX_CONSOLE_PARAM_VALUE_LEN);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}
		if (strClosest == NULL)
		{
			strClosest = strCat;
		} 
		else 
		{
			int delta = PStrICmp(strCat->GetStr(), strClosest->GetStr(), MAX_CONSOLE_PARAM_VALUE_LEN);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				strClosest = strCat;
			}
		}
	}

	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nCommandCount; ++ii)
	{
		CONSOLE_COMMAND * command = g_ConsoleCommandTable.m_pCommandTable + ii;

		int compare = PStrICmp(strParam, command->m_strCommand.GetStr(), MAX_CONSOLE_PARAM_VALUE_LEN);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}
		if (strClosest == NULL)
		{
			strClosest = &command->m_strCommand;
		} 
		else 
		{
			int delta = PStrICmp(command->m_strCommand.GetStr(), strClosest->GetStr(), MAX_CONSOLE_PARAM_VALUE_LEN);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				strClosest = &command->m_strCommand;
			}
		}
	}

	if (strClosest == NULL)
	{
		return FALSE;
	}

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCopy(strNewParam, strClosest->GetStr(), arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteUIComponent(
	GAME * game, 
	const WCHAR * strLine, 
	const CONSOLE_COMMAND * cmd, 
	const WCHAR * strParamStart, 
	const CONSOLE_PARAM * param, 
	const WCHAR * strParam,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(param);
	ASSERT_RETFALSE(strLine);
	ASSERT_RETFALSE(strParamStart);
	ASSERT_RETFALSE(strParam);

	char szLookFor[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(szLookFor, strParam, MAX_CONSOLE_PARAM_VALUE_LEN);

	const char * szClosest = NULL;

	UI_COMPONENT *pCurChild = NULL;
	while ((pCurChild = UIComponentIterateChildren(g_UI.m_Components, pCurChild, -1, TRUE)) != NULL)
	{		
		int compare = PStrICmp(szLookFor, pCurChild->m_szName, MAX_CONSOLE_PARAM_VALUE_LEN);
		if (bReverse ? (compare <= 0) : (compare >= 0))
		{
			continue;
		}
		if (szClosest == NULL)
		{
			szClosest = pCurChild->m_szName;
		} 
		else 
		{
			int delta = PStrICmp(pCurChild->m_szName, szClosest, MAX_CONSOLE_PARAM_VALUE_LEN);
			if (bReverse ? (delta > 0) : (delta < 0))
			{
				szClosest = pCurChild->m_szName;
			}
		}
	}

	if (szClosest == NULL)
	{
		return FALSE;
	}

	WCHAR strNewParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	PStrCvt(strNewParam, szClosest, arrsize(strNewParam));

	return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, strNewParam);
}
#endif // SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sConsoleAutoCompleteParam(
	GAME * game,
	const WCHAR * strLine,
	const CONSOLE_COMMAND * cmd,
	const WCHAR * strInput,
	BOOL bReverse)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(cmd);
	ASSERT_RETFALSE(strInput);

	if (!cmd->m_nParams)
	{
		return FALSE;
	}
	unsigned int idxParam = 0;

	WCHAR strParam[MAX_CONSOLE_PARAM_VALUE_LEN];
	strParam[0] = 0;
	int bufLen = arrsize(strParam);
	int tokLen = 0;

	static const WCHAR * stdDelim = L" ,";
	static const WCHAR * endDelim = L",";

	while (1)
	{
		const WCHAR * strDelim = idxParam >= cmd->m_nParams - 1 ? endDelim : stdDelim;
		WCHAR delim = 0;
		if (strInput[0] != 0 && (strInput = PStrTok(strParam, strInput, strDelim, CONSOLE_MAX_STR, bufLen, &tokLen, &delim)) == NULL)
		{
			return FALSE;
		}
		if (delim == 0)
		{
			break;
		}
		++idxParam;
		if (idxParam >= cmd->m_nParams)
		{
			return FALSE;
		}
	}

	const WCHAR * strParamStart = strLine + (strInput - strLine - tokLen);
	ASSERT_RETFALSE(strParamStart - strLine < CONSOLE_MAX_STR);

	ASSERT_RETFALSE(cmd->m_pParams);
	CONSOLE_PARAM * param = cmd->m_pParams + idxParam;
	switch (param->m_eParamType)
	{
	case PARAM_FLOAT:
	case PARAM_INTEGER:
	case PARAM_HEX:
	case PARAM_GENERIC:
	case PARAM_STRING:
		break;
	case PARAM_PLAYERNAME:
		return sConsoleAutoCompleteSetNewParam(strLine, strParamStart, L"NYI");
	case PARAM_BOOLEAN:
		return sConsoleAutoCompleteParamBoolean(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_DATATABLE:
		return sConsoleAutoCompleteParamDatatable(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_DATATABLE_BYNAME:
		return sConsoleAutoCompleteParamDatatableByName(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_DATATABLE_BYSTR:
		return sConsoleAutoCompleteParamDatatableByStr(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_TABLENAME:
		return sConsoleAutoCompleteParamTablename(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_PERF:
		return sConsoleAutoCompleteParamPerf(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_HITCOUNT:
		return sConsoleAutoCompleteParamHitcount(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_STRINGLIST:
		return sConsoleAutoCompleteParamStringList(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_DEFINITION:
		return sConsoleAutoCompleteParamDefinition(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_TEAM:
		return sConsoleAutoCompleteParamTeam(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_CONSOLECMD:
		return sConsoleAutoCompleteConsoleCommand(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	case PARAM_UI_COMPONENT:
		return sConsoleAutoCompleteUIComponent(game, strLine, cmd, strParamStart, param, strParam, bReverse);
	default:
		ASSERT(0);
	}
	return TRUE;
}
#endif // !(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
BOOL ConsoleAutoComplete(
	GAME * game,
	BOOL bReverse)
{
	ASSERT_RETFALSE(g_ConsoleCommandTable.m_nCommandCount > 0);

	const WCHAR * strInput = ConsoleGetInputLine();

	if (!strInput)
	{
		return FALSE;
	}

	if (strInput[0] != '/' && strInput[0] != L'\\')
	{
		return FALSE;
	}

	strInput++;
	const WCHAR * strLine = strInput;

	WCHAR strCommand[MAX_CONSOLE_COMMAND_LEN];
	int bufLen = arrsize(strCommand);
	int tokLen = 0;
	WCHAR delim = 0;
	if (strInput[0] != 0 && (strInput = PStrTok(strCommand, strInput, L" ,.", CONSOLE_MAX_STR, bufLen, &tokLen, &delim)) == NULL)
	{
		return FALSE;
	}

	BOOL isExact = FALSE;
	unsigned int ii = sCommandTableFindCommand(strCommand, &isExact) % g_ConsoleCommandTable.m_nCommandCount;
	if (isExact && delim == L' ')
	{
		return sConsoleAutoCompleteParam(game, strLine, g_ConsoleCommandTable.m_pCommandTable + ii, strInput, bReverse);
	}

	int dir = (bReverse ? -1 : 1);
	if (isExact)
	{
		ii = UINT_ROTATE(ii, g_ConsoleCommandTable.m_nCommandCount, dir);
	}
	unsigned int sentinel = UINT_ROTATE(ii, g_ConsoleCommandTable.m_nCommandCount, -dir);

	const CONSOLE_COMMAND * cmd = NULL;
	for (; ii != sentinel; ii = UINT_ROTATE(ii, g_ConsoleCommandTable.m_nCommandCount, dir))
	{
		if (!sConsoleCommandIsApplicable(game, NULL, g_ConsoleCommandTable.m_pCommandTable + ii, FALSE))
			continue;
		if (g_ConsoleCommandTable.m_pCommandTable[ii].m_strCommand.GetStr()[0] == '_' &&
			AppGetBadgeCollection() &&
			!AppGetBadgeCollection()->HasBadge(ACCT_TITLE_ADMINISTRATOR))
			continue;

		cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
		break;
	}

	if (!cmd)
	{
		return FALSE;
	}

	ConsoleSetInputLine(cmd->m_strCommand.GetStr(), ConsoleInputColor(), 1);
	return TRUE;
}
#endif	//	!SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableRegisterCommandValidate(
	const WCHAR * strCommand)
{
	ASSERT_RETFALSE(strCommand);
	const WCHAR * strCur = strCommand;
	const WCHAR * strEnd = strCur + MAX_CONSOLE_COMMAND_LEN;
	while (strCur < strEnd)
	{
		if (*strCur == 0)
		{
			return (strCur > strCommand);
		}
		if (*strCur == L' ')
		{
			return FALSE;
		}
		if (PStrIsPrintable(*strCur) == FALSE)
		{
			return FALSE;
		}
		++strCur;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableRegisterCommand(
	const WCHAR * strCommandLocalizedLink,
	const WCHAR * strCommand,
	BOOL isCheat,
	int appType,
	int gameType,
	CONSOLE_COMMAND_FUNC cmdFunction,
	BOOL bValidateParams,
	DWORD dwCommandFlags)		// see CONSOLE_COMMAND_FLAGS
{
	ASSERTV_RETFALSE(sCommandTableRegisterCommandValidate(strCommand), "Invalid console command '%S'", strCommand);

	BOOL isExact = FALSE;
	unsigned int ii = sCommandTableFindCommand(strCommand, &isExact);
	if (isExact)
	{
		CONSOLE_COMMAND * cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
		ASSERT_RETFALSE(cmd->m_fCmdFunction == cmdFunction);  // duplicate command must be with same function
		return FALSE;
	}

	if (g_ConsoleCommandTable.m_nCommandCount % CONSOLE_COMMAND_BUFSIZE == 0)
	{
		g_ConsoleCommandTable.m_pCommandTable = (CONSOLE_COMMAND *)REALLOC(NULL, g_ConsoleCommandTable.m_pCommandTable, sizeof(CONSOLE_COMMAND) * (g_ConsoleCommandTable.m_nCommandCount + CONSOLE_COMMAND_BUFSIZE));
		ASSERT_RETFALSE(g_ConsoleCommandTable.m_pCommandTable);
	}
	if (ii < g_ConsoleCommandTable.m_nCommandCount)
	{
		MemoryMove(g_ConsoleCommandTable.m_pCommandTable + ii + 1, (g_ConsoleCommandTable.m_nCommandCount - ii) * sizeof(CONSOLE_COMMAND), g_ConsoleCommandTable.m_pCommandTable + ii, (g_ConsoleCommandTable.m_nCommandCount - ii) * sizeof(CONSOLE_COMMAND));
	}
	g_ConsoleCommandTable.m_nCommandCount++;

	CONSOLE_COMMAND * cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
	memclear(cmd, sizeof(CONSOLE_COMMAND));
	cmd->m_strCommand.Copy(strCommand, MAX_CONSOLE_COMMAND_LEN);
	cmd->m_strCommandLocalizedLink.Copy(L"", MAX_CONSOLE_COMMAND_LEN);
	if (strCommandLocalizedLink)
	{
		cmd->m_strCommandLocalizedLink.Copy(strCommandLocalizedLink, MAX_CONSOLE_COMMAND_LEN);
	}
	cmd->m_strCategory = g_ConsoleCommandTable.m_pLastCategory;
	cmd->m_isCheat = isCheat;
	cmd->m_eAppType = appType;
	cmd->m_eGameType = gameType;
	cmd->m_fCmdFunction = cmdFunction;
	cmd->m_bValidateParams = bValidateParams;
	cmd->m_dwCommandFlags = dwCommandFlags;
	g_ConsoleCommandTable.m_nLastRegisteredCommand = ii;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableAddParamValue(
	CONSOLE_PARAM * param,
	const WCHAR * strVal)
{
	BOOL isExact;
	unsigned int ii = sConsoleParamFindValue(param, strVal, &isExact);
	ASSERT_RETFALSE(!isExact);

	param->m_pstrValues = (PStrW *)REALLOCZ(NULL, param->m_pstrValues, sizeof(PStrW) * (param->m_nValues + 1));
	ASSERT_DO(param->m_pstrValues != NULL)
	{
		param->m_nValues = 0;
		return FALSE;
	}

	if (ii < param->m_nValues)
	{
		MemoryMove(param->m_pstrValues + ii + 1, (param->m_nValues - ii) * sizeof(PStrW), param->m_pstrValues + ii, (param->m_nValues - ii) * sizeof(PStrW));
	}
	param->m_nValues++;

	PStrW * str = param->m_pstrValues + ii;
	memclear(str, sizeof(PStrW));
	str->Copy(strVal, MAX_CONSOLE_PARAM_VALUE_LEN);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableAddCommandParam(
	const WCHAR *puszCommand,
	ECONSOLE_PARAM	eParamType,
	BOOL isRequired,
	BOOL isOrdered,
	DWORD_PTR data1,
	DWORD_PTR data2,
	const WCHAR * strName,
	const WCHAR * strValues,
	const WCHAR * strDefault,
	CONSOLE_PARAM_AUTOCOMPLETE fCustomAutocomplete)
{
	CONSOLE_COMMAND * cmd = NULL;

	// if no command provided as argument, use the last registered command
	if (puszCommand == NULL)
	{
		ASSERT_RETFALSE(g_ConsoleCommandTable.m_nLastRegisteredCommand < g_ConsoleCommandTable.m_nCommandCount);
		ASSERT_RETFALSE(g_ConsoleCommandTable.m_pCommandTable != NULL);

		unsigned int ii = g_ConsoleCommandTable.m_nLastRegisteredCommand;
		ASSERT_RETFALSE(ii >= 0);

		cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
	}
	else
	{
		// note we're casting away const so that we modify an already registered command
		cmd = (CONSOLE_COMMAND * )sCommandTableFindCommandExact(puszCommand);
	}
	ASSERT_RETFALSE(cmd);
	
	cmd->m_pParams = (CONSOLE_PARAM *)REALLOCZ(NULL, cmd->m_pParams, sizeof(CONSOLE_PARAM) * (cmd->m_nParams + 1));
	ASSERT_DO(cmd->m_pParams != NULL)
	{
		cmd->m_nParams = 0;
		return FALSE;
	}
	cmd->m_nParams++;
	
	CONSOLE_PARAM * param = cmd->m_pParams + cmd->m_nParams - 1;
	param->m_eParamType = eParamType;
	param->m_isRequired = isRequired;
	param->m_isOrdered = isOrdered;
	param->m_data1 = data1;
	param->m_data2 = data2;
	param->m_fCustomAutocomplete = fCustomAutocomplete;
	param->m_strDefault.Copy(strDefault, MAX_CONSOLE_PARAM_VALUE_LEN);
	param->m_strName.Copy(strName, MAX_CONSOLE_PARAM_NAME_LEN);

	if (eParamType == PARAM_STRINGLIST && strValues && strValues[0])
	{
		WCHAR strVal[MAX_CONSOLE_PARAM_VALUE_LEN];
		int lenVal = PStrLen(strValues) + 1;

		while ((strValues = PStrTok(strVal, strValues, L" ,", arrsize(strVal), lenVal)) != NULL)
		{
			if (strVal[0])
			{
				ASSERT_CONTINUE(sCommandTableAddParamValue(param, strVal));
			}
		}
	}

	// see if this command has a linked localized command to do the same things to
	if (puszCommand == NULL && cmd->m_strCommandLocalizedLink.Len() > 0)
	{
		if (!sCommandTableAddCommandParam(
				cmd->m_strCommandLocalizedLink.GetStr(),
				eParamType,
				isRequired,
				isOrdered,
				data1,
				data2,
				strName,
				strValues,
				strDefault,
				fCustomAutocomplete))
		{
			return FALSE;
		}
		
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableAddCommandUsage(
	const WCHAR * strCommand,
	const WCHAR * strUsage)
{
	CONSOLE_COMMAND * cmd = NULL;

	if (strCommand == NULL)
	{
		ASSERT_RETFALSE(g_ConsoleCommandTable.m_nLastRegisteredCommand < g_ConsoleCommandTable.m_nCommandCount);
		ASSERT_RETFALSE(g_ConsoleCommandTable.m_pCommandTable != NULL);
		ASSERT_RETFALSE(strUsage);

		unsigned int ii = g_ConsoleCommandTable.m_nLastRegisteredCommand;
		ASSERT_RETFALSE(ii >= 0);

		cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
	}
	else
	{
		cmd = (CONSOLE_COMMAND *)sCommandTableFindCommandExact(strCommand);
	}
	ASSERT_RETFALSE(cmd);
	
	cmd->m_strUsage.Copy(strUsage, MAX_CONSOLE_USAGE_LEN);

	// apply to linked command
	if (strCommand == NULL && cmd->m_strCommandLocalizedLink.Len() > 0)
	{
		if (!sCommandTableAddCommandUsage( cmd->m_strCommandLocalizedLink.GetStr(), strUsage ))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableAddCommandUsageNotes(
	const WCHAR * strCommand,
	const WCHAR * strUsageNotes)
{
	CONSOLE_COMMAND * cmd = NULL;
	
	if (strCommand == NULL)
	{
		ASSERT_RETFALSE(g_ConsoleCommandTable.m_nLastRegisteredCommand < g_ConsoleCommandTable.m_nCommandCount);
		ASSERT_RETFALSE(g_ConsoleCommandTable.m_pCommandTable != NULL);
		ASSERT_RETFALSE(strUsageNotes);

		unsigned int ii = g_ConsoleCommandTable.m_nLastRegisteredCommand;
		ASSERT_RETFALSE(ii >= 0);

		cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
	}
	else
	{
		cmd = (CONSOLE_COMMAND *)sCommandTableFindCommandExact(strCommand);
	}
	ASSERT_RETFALSE(cmd);
		
	cmd->m_strUsageNotes.Copy(strUsageNotes, MAX_CONSOLE_USAGE_LEN);

	// appy to any linked command
	if (strCommand == NULL && cmd->m_strCommandLocalizedLink.Len() > 0)
	{
		if (!sCommandTableAddCommandUsageNotes( cmd->m_strCommandLocalizedLink.GetStr(), strUsageNotes ))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableAddCommandDescription(
	const WCHAR * strDesc)
{
	ASSERT_RETFALSE(g_ConsoleCommandTable.m_nLastRegisteredCommand < g_ConsoleCommandTable.m_nCommandCount);
	ASSERT_RETFALSE(g_ConsoleCommandTable.m_pCommandTable != NULL);
	ASSERT_RETFALSE(strDesc);

	unsigned int ii = g_ConsoleCommandTable.m_nLastRegisteredCommand;
	ASSERT_RETFALSE(ii >= 0);

	CONSOLE_COMMAND * cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
	cmd->m_strDescription.Copy(strDesc, MAX_CONSOLE_DESC_LEN);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableAddCommandData(
	const unsigned short int nData)
{
	ASSERT_RETFALSE(g_ConsoleCommandTable.m_nLastRegisteredCommand < g_ConsoleCommandTable.m_nCommandCount);
	ASSERT_RETFALSE(g_ConsoleCommandTable.m_pCommandTable != NULL);

	unsigned int ii = g_ConsoleCommandTable.m_nLastRegisteredCommand;
	ASSERT_RETFALSE(ii >= 0);

	CONSOLE_COMMAND * cmd = g_ConsoleCommandTable.m_pCommandTable + ii;
	cmd->m_nData = nData;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommandTableAddCommandCategory(
	const WCHAR * strCat)
{
	const PStrW * cat = sCommandTableFindCategory(strCat);
	if (cat)
	{
		g_ConsoleCommandTable.m_pLastCategory = cat;
		return TRUE;
	}

	g_ConsoleCommandTable.m_pstrCategories = (PStrW **)REALLOCZ(NULL, g_ConsoleCommandTable.m_pstrCategories, (g_ConsoleCommandTable.m_nCategories + 1) * sizeof(PStrW *));

	unsigned int len = PStrLen(strCat, MAX_CONSOLE_COMMAND_LEN);
	PStrW * str = (PStrW *)MALLOCZ(NULL, sizeof(PStrW));
	str->Copy(strCat, len + 1);
	g_ConsoleCommandTable.m_pstrCategories[g_ConsoleCommandTable.m_nCategories] = str;
	++g_ConsoleCommandTable.m_nCategories;

	g_ConsoleCommandTable.m_pLastCategory = str;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sConsoleCommandTableFreeParticleList(
	void)
{
	if (g_ConsoleCommandTable.m_nParticles > 0)
	{
		ASSERT(g_ConsoleCommandTable.m_pstrParticles != NULL);
	}
	for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nParticles; ++ii)
	{
		g_ConsoleCommandTable.m_pstrParticles[ii].Free();
	}
	if (g_ConsoleCommandTable.m_pstrParticles)
	{
		FREE(NULL, g_ConsoleCommandTable.m_pstrParticles);
		g_ConsoleCommandTable.m_pstrParticles = NULL;
	}
	g_ConsoleCommandTable.m_nParticles = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleCommandTableFree(
	void)
{
	sConsoleCommandTableFreeParticleList();

	if (g_ConsoleCommandTable.m_nCommandCount > 0)
	{
		ASSERT_RETURN(g_ConsoleCommandTable.m_pCommandTable != NULL);
	}
	for (CONSOLE_COMMAND * cmd = g_ConsoleCommandTable.m_pCommandTable; cmd < g_ConsoleCommandTable.m_pCommandTable + g_ConsoleCommandTable.m_nCommandCount; ++cmd)
	{
		cmd->m_strCommand.Free();
		cmd->m_strCommandLocalizedLink.Free();
		cmd->m_strDescription.Free();
		cmd->m_strUsage.Free();
		cmd->m_strUsageNotes.Free();
		if (cmd->m_nParams > 0)
		{
			ASSERT_CONTINUE(cmd->m_pParams != NULL);
			for (unsigned int ii = 0; ii < cmd->m_nParams; ++ii)
			{
				CONSOLE_PARAM * param = cmd->m_pParams + ii;
				param->m_strName.Free();
				param->m_strDefault.Free();
				for (unsigned int jj = 0; jj < param->m_nValues; ++jj)
				{
					param->m_pstrValues[jj].Free();
				}
				if (param->m_pstrValues)
				{
					FREE(NULL, param->m_pstrValues);
				}
			}
		}
		FREE(NULL, cmd->m_pParams);
	}

	if (g_ConsoleCommandTable.m_pstrCategories)
	{
		for (unsigned int ii = 0; ii < g_ConsoleCommandTable.m_nCategories; ++ii)
		{
			ASSERT_CONTINUE(g_ConsoleCommandTable.m_pstrCategories[ii]);
			g_ConsoleCommandTable.m_pstrCategories[ii]->Free();
			FREE(NULL, g_ConsoleCommandTable.m_pstrCategories[ii]);
		}
		FREE(NULL, g_ConsoleCommandTable.m_pstrCategories);
		g_ConsoleCommandTable.m_pstrCategories = NULL;
	}
	g_ConsoleCommandTable.m_pLastCategory = NULL;

	FREE(NULL, g_ConsoleCommandTable.m_pCommandTable);
	g_ConsoleCommandTable.m_pCommandTable = NULL;
	g_ConsoleCommandTable.m_nCommandCount = 0;
	g_ConsoleCommandTable.m_nLastRegisteredCommand = (unsigned int)-1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConsoleAddEmotes()
{
	unsigned int nCount = ExcelGetCount(NULL, DATATABLE_EMOTES);
	for (unsigned int ii = 0; ii < nCount; ++ii)
	{
		const EMOTE_DATA *pEmote = (EMOTE_DATA *) ExcelGetData(NULL, DATATABLE_EMOTES, ii);
		if (pEmote)
		{
			if (sCommandTableRegisterCommand(
				StringTableGetStringByIndex(pEmote->nCommandStringEnglish),
				StringTableGetStringByIndex(pEmote->nCommandString),
				FALSE, 
				COMMAND_CLIENT,	
				COMMAND_CLIENT,	
				c_sCmdEmoteSpecific,
				TRUE,
				0))
			{
				sCommandTableAddCommandDescription(StringTableGetStringByIndex(pEmote->nDescriptionString));
				sCommandTableAddCommandData((unsigned short int)ii);
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConsoleCommandTableInit(
	void)
{
	#define CONSOLE_DESC(desc)												sCommandTableAddCommandDescription(L##desc)
	#define LOCALIZED_DESC(eGSDesc)											sCommandTableAddCommandDescription(GlobalStringGet(eGSDesc))
	#define CONSOLE_CMD(cmd, cheat, appType, gameType, func, valParam, dwCommandFlags) \
		STATIC_CHECK((cheat) != NOT_CHEATS || (gameType) != COMMAND_SERVER, SERVER_SIDE_COMMANDS_NOT_ALLOWED_IN_RELEASE); \
		sCommandTableRegisterCommand(NULL, L##cmd,  cheat, appType, gameType, func, valParam, dwCommandFlags)
	#define LOCALIZED_CMD(eGSCmdEnglish, eGSCmdLanguage, eGSDescEnglish, eGSDescLanguage, cheat, appType, gameType, func, valParam, dwCommandFlags)	\
		if (sCommandTableRegisterCommand(NULL,							  GlobalStringGet(eGSCmdLanguage), cheat, appType, gameType, func, valParam, dwCommandFlags)) { LOCALIZED_DESC(eGSDescLanguage); } \
		if (sCommandTableRegisterCommand(GlobalStringGet(eGSCmdLanguage), GlobalStringGet(eGSCmdEnglish),  cheat, appType, gameType, func, valParam, dwCommandFlags)) { LOCALIZED_DESC(eGSDescLanguage); } 
	#define CONSOLE_PAR(paramType, req, data1, data2, def)					sCommandTableAddCommandParam(NULL, paramType, req, TRUE, data1, data2, NULL, NULL, L##def, NULL)
	#define CONSOLE_PAR_NA(name, paramType, req, data1, data2, def)			sCommandTableAddCommandParam(NULL, paramType, req, TRUE, data1, data2, L##name, NULL, L##def, NULL)
	#define CONSOLE_PAR_UN(paramType, req, data1, data2, def)				sCommandTableAddCommandParam(NULL, paramType, req, FALSE, data1, data2, NULL, NULL, L##def, NULL)
	#define CONSOLE_PAR_NV(paramType, req, name, values, def)				sCommandTableAddCommandParam(NULL, paramType, req, FALSE, 0, 0, L##name, L##values, L##def, NULL)
	#define CONSOLE_USE(usage)												sCommandTableAddCommandUsage(NULL, L##usage)
	#define LOCALIZED_USE(eGSUse)											sCommandTableAddCommandUsage(NULL, GlobalStringGet(eGSUse))
	#define CONSOLE_NOTE(notes)												sCommandTableAddCommandUsageNotes(NULL, L##notes)
	#define LOCALIZED_NOTE(eGSNote)											sCommandTableAddCommandUsageNotes(NULL, GlobalStringGet(eGSNote))
	#define CONSOLE_CATEGORY(cat)											sCommandTableAddCommandCategory(L##cat)
	#define LOCALIZED_CATEGORY(eGlobalString)								sCommandTableAddCommandCategory(GlobalStringGet(eGlobalString))
	
	ConsoleCommandTableFree();

#if !ISVERSION(SERVER_VERSION)
// -----------------------------
// -----------------------------
LOCALIZED_CATEGORY( GS_CCAT_GENERAL);
	LOCALIZED_CMD(GS_CCMD_VERSION_ENG,				GS_CCMD_VERSION,			GS_CCMD_VERSION_DESC_ENG,			GS_CCMD_VERSION_DESC,				FALSE,		COMMAND_BOTH,	COMMAND_CLIENT,		c_sCmdVersion,						TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_QUIT_ENG,					GS_CCMD_QUIT,				GS_CCMD_QUIT_DESC_ENG,				GS_CCMD_QUIT_DESC,					FALSE,		COMMAND_BOTH,	COMMAND_CLIENT,		c_sCmdQuit,							TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_CLEAR_ENG,				GS_CCMD_CLEAR,				GS_CCMD_CLEAR_DESC_ENG,				GS_CCMD_CLEAR_DESC,					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdClearScreen,					TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_CLEAR2_ENG,				GS_CCMD_CLEAR2,				GS_CCMD_CLEAR_DESC_ENG,				GS_CCMD_CLEAR_DESC,					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdClearScreen,					TRUE, 0);
#if !defined(TUGBOAT)
	LOCALIZED_CMD(GS_CCMD_STUCK_ENG,				GS_CCMD_STUCK,				GS_CCMD_STUCK_DESC_ENG,				GS_CCMD_STUCK_DESC,					FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStuck,						TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_UNSTUCK_ENG,				GS_CCMD_UNSTUCK,			GS_CCMD_STUCK_DESC_ENG,				GS_CCMD_STUCK_DESC,					FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStuck,						TRUE, 0);
#endif
	LOCALIZED_CMD(GS_CCMD_BUG_ENG,					GS_CCMD_BUG,				GS_CCMD_BUG_DESC_ENG,				GS_CCMD_BUG_DESC,					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sReportBug,						FALSE, 0);		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "s", "");		CONSOLE_PAR_NV(PARAM_STRING, FALSE, "text",	"", "");		
	LOCALIZED_NOTE(GS_CCMD_BUG_NOTE);
	LOCALIZED_CMD(GS_CCMD_HELP_ENG,					GS_CCMD_HELP,				GS_CCMD_HELP_DESC_ENG,				GS_CCMD_HELP_DESC,					FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sHelp,							TRUE, 0);		CONSOLE_PAR_NV(PARAM_CONSOLECMD, FALSE, "", "", "");
	LOCALIZED_CMD(GS_CCMD_PLAYED_ENG,				GS_CCMD_PLAYED,				GS_CCMD_PLAYED_DESC_ENG,			GS_CCMD_PLAYED_DESC,				FALSE,		COMMAND_BOTH,	COMMAND_CLIENT,		c_sCmdPlayed,						TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_SCREENSHOT_ENG,			GS_CCMD_SCREENSHOT,			GS_CCMD_SCREENSHOT_DESC_ENG,		GS_CCMD_SCREENSHOT_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTakeScreenshotNoUI,			TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_UIHIDE_ENG,				GS_CCMD_UIHIDE,				GS_CCMD_UIHIDE_DESC_ENG,			GS_CCMD_UIHIDE_DESC,				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sUIHide,							TRUE, 0);


// -----------------------------
// -----------------------------
LOCALIZED_CATEGORY( GS_CCAT_MULTIPLAYER );
	LOCALIZED_CMD(GS_CCMD_PING_ENG,					GS_CCMD_PING,				GS_CCMD_PING_DESC_ENG,				GS_CCMD_PING_DESC,				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPing,							TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_WHO_ENG,					GS_CCMD_WHO,				GS_CCMD_WHO_DESC_ENG,				GS_CCMD_WHO_DESC,				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWho,							TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_INVITE_ENG,				GS_CCMD_INVITE,				GS_CCMD_INVITE_DESC_ENG,			GS_CCMD_INVITE_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyInvite,					TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_UNINVITE_ENG,				GS_CCMD_UNINVITE,			GS_CCMD_UNINVITE_DESC_ENG,			GS_CCMD_UNINVITE_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyCancelInvite,			TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_ACCEPT_ENG,				GS_CCMD_ACCEPT,				GS_CCMD_ACCEPT_DESC_ENG,			GS_CCMD_ACCEPT_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyAcceptInvite,			TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_DECLINE_ENG,				GS_CCMD_DECLINE,			GS_CCMD_DECLINE_DESC_ENG,			GS_CCMD_DECLINE_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyDeclineInvite,			TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_BOOT_ENG,					GS_CCMD_BOOT,				GS_CCMD_BOOT_DESC_ENG,				GS_CCMD_BOOT_DESC,				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyRemoveMember,			TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_LEAVE_ENG,				GS_CCMD_LEAVE,				GS_CCMD_LEAVE_DESC_ENG,				GS_CCMD_LEAVE_DESC,				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyLeave,					TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_DISBAND_ENG,				GS_CCMD_DISBAND,			GS_CCMD_DISBAND_DESC_ENG,			GS_CCMD_DISBAND_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyDisband,					TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_PROMOTE_ENG,				GS_CCMD_PROMOTE,			GS_CCMD_PROMOTE_DESC_ENG,			GS_CCMD_PROMOTE_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartySetLeader,				TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
//	LOCALIZED_CMD(GS_CCMD_AUTOPARTY_ENG,			GS_CCMD_AUTOPARTY,			GS_CCMD_AUTOPARTY_DESC_ENG,			GS_CCMD_AUTOPARTY_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdAutoParty,					TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_FRIENDADD_ENG,			GS_CCMD_FRIENDADD,			GS_CCMD_FRIENDADD_DESC_ENG,			GS_CCMD_FRIENDADD_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdBuddyAdd,						TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_FRIENDREMOVE_ENG,			GS_CCMD_FRIENDREMOVE,		GS_CCMD_FRIENDREMOVE_DESC_ENG,		GS_CCMD_FRIENDREMOVE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdBuddyRemove,					TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_FRIENDLIST_ENG,			GS_CCMD_FRIENDLIST,			GS_CCMD_FRIENDLIST_DESC_ENG,		GS_CCMD_FRIENDLIST_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdBuddyShowList,				TRUE, 0);
//	LOCALIZED_CMD(GS_CCMD_GUILD_CREATE_ENG,			GS_CCMD_GUILD_CREATE,		GS_CCMD_GUILD_CREATE_DESC_ENG,		GS_CCMD_GUILD_CREATE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildCreate,					TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_GUILD_INVITE_ENG,			GS_CCMD_GUILD_INVITE,		GS_CCMD_GUILD_INVITE_DESC_ENG,		GS_CCMD_GUILD_INVITE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildInvite,					TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,  0, "");
	LOCALIZED_CMD(GS_CCMD_GUILD_DECLINE_ENG,		GS_CCMD_GUILD_DECLINE,		GS_CCMD_GUILD_DECLINE_DESC_ENG,		GS_CCMD_GUILD_DECLINE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildDeclineInvite,			TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_GUILD_ACCEPT_ENG,			GS_CCMD_GUILD_ACCEPT,		GS_CCMD_GUILD_ACCEPT_DESC_ENG,		GS_CCMD_GUILD_ACCEPT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildAcceptInvite,			TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_GUILD_LEAVE_ENG,			GS_CCMD_GUILD_LEAVE,		GS_CCMD_GUILD_LEAVE_DESC_ENG,		GS_CCMD_GUILD_LEAVE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildLeaveGuild,				TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_GUILD_REMOVE_ENG,			GS_CCMD_GUILD_REMOVE,		GS_CCMD_GUILD_REMOVE_DESC_ENG,		GS_CCMD_GUILD_REMOVE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildRemove,					TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,  0, "");
	LOCALIZED_CMD(GS_CCMD_GUILD_PROMOTE_ENG,		GS_CCMD_GUILD_PROMOTE,		GS_CCMD_GUILD_PROMOTE_DESC_ENG,		GS_CCMD_GUILD_PROMOTE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildPromote,					TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,  0, "");
	LOCALIZED_CMD(GS_CCMD_GUILD_CHGRNK_ENG,			GS_CCMD_GUILD_CHGRNK,		GS_CCMD_GUILD_CHGRNK_DESC_ENG,		GS_CCMD_GUILD_CHGRNK_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildChangeRank,				TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,  0, "");	CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");

	LOCALIZED_CMD(GS_CCMD_PVP_TOGGLE_ENG,			GS_CCMD_PVP_TOGGLE,			GS_CCMD_PVP_TOGGLE_DESC_ENG,		GS_CCMD_PVP_TOGGLE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPvPToggle,					TRUE, CCF_DEAD_RESTRICTED);
	LOCALIZED_CMD(GS_CCMD_DUEL_INVITE_ENG,			GS_CCMD_DUEL_INVITE,		GS_CCMD_DUEL_INVITE_DESC_ENG,		GS_CCMD_DUEL_INVITE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDuelInvite,					TRUE, CCF_DEAD_RESTRICTED);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");

// -----------------------------												
// -----------------------------												
LOCALIZED_CATEGORY( GS_CCAT_CHAT );												
	LOCALIZED_CMD(GS_CCMD_IGNORE_ENG,				GS_CCMD_IGNORE,				GS_CCMD_IGNORE_DESC_ENG,			GS_CCMD_IGNORE_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdIgnore,						TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,  0, "");
	LOCALIZED_CMD(GS_CCMD_UNIGNORE_ENG,				GS_CCMD_UNIGNORE,			GS_CCMD_UNIGNORE_DESC_ENG,			GS_CCMD_UNIGNORE_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUnignore,						TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,  0, "");
	LOCALIZED_CMD(GS_CCMD_IGNORE_WHO_ENG,			GS_CCMD_IGNORE_WHO,			GS_CCMD_IGNORE_WHO_DESC_ENG,		GS_CCMD_IGNORE_WHO_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdIgnoreWho,					TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_EMOTE_ENG,				GS_CCMD_EMOTE,				GS_CCMD_EMOTE_DESC_ENG,				GS_CCMD_EMOTE_DESC,				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmote,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_EMOTE2_ENG,				GS_CCMD_EMOTE2,				GS_CCMD_EMOTE_DESC_ENG,				GS_CCMD_EMOTE_DESC,				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmote,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_WHISPER_ENG,			GS_CCMD_CHAT_WHISPER,		GS_CCMD_CHAT_WHISPER_DESC_ENG,		GS_CCMD_CHAT_WHISPER_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWhisper,						TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_WHISPER2_ENG,		GS_CCMD_CHAT_WHISPER2,		GS_CCMD_CHAT_WHISPER_DESC_ENG,		GS_CCMD_CHAT_WHISPER_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWhisper,						TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_WHISPER3_ENG,		GS_CCMD_CHAT_WHISPER3,		GS_CCMD_CHAT_WHISPER_DESC_ENG,		GS_CCMD_CHAT_WHISPER_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWhisper,						TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_WHISPER4_ENG,		GS_CCMD_CHAT_WHISPER4,		GS_CCMD_CHAT_WHISPER_DESC_ENG,		GS_CCMD_CHAT_WHISPER_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWhisper,						TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_LOCAL_ENG,			GS_CCMD_CHAT_LOCAL,			GS_CCMD_CHAT_LOCAL_DESC_ENG,		GS_CCMD_CHAT_LOCAL_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTownChat,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_LOCAL2_ENG,			GS_CCMD_CHAT_LOCAL2,		GS_CCMD_CHAT_LOCAL_DESC_ENG,		GS_CCMD_CHAT_LOCAL_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTownChat,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_LOCAL3_ENG,			GS_CCMD_CHAT_LOCAL3,		GS_CCMD_CHAT_LOCAL_DESC_ENG,		GS_CCMD_CHAT_LOCAL_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTownChat,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_PARTY_ENG,			GS_CCMD_CHAT_PARTY,			GS_CCMD_CHAT_PARTY_DESC_ENG,		GS_CCMD_CHAT_PARTY_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyMessage,					TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_PARTY2_ENG,			GS_CCMD_CHAT_PARTY2,		GS_CCMD_CHAT_PARTY_DESC_ENG,		GS_CCMD_CHAT_PARTY_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyMessage,					TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_SHOUT_ENG,			GS_CCMD_CHAT_SHOUT,			GS_CCMD_CHAT_SHOUT_DESC_ENG,		GS_CCMD_CHAT_SHOUT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdChatInstanced,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_SHOUT2_ENG,			GS_CCMD_CHAT_SHOUT2,		GS_CCMD_CHAT_SHOUT_DESC_ENG,		GS_CCMD_CHAT_SHOUT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdChatInstanced,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_YELL_ENG,			GS_CCMD_CHAT_YELL,			GS_CCMD_CHAT_SHOUT_DESC_ENG,		GS_CCMD_CHAT_SHOUT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdChatInstanced,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_YELL2_ENG,			GS_CCMD_CHAT_YELL2,			GS_CCMD_CHAT_SHOUT_DESC_ENG,		GS_CCMD_CHAT_SHOUT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdChatInstanced,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_CHANNEL_LIST_ENG,	GS_CCMD_CHAT_CHANNEL_LIST,	GS_CCMD_CHAT_CHANNEL_LIST_DESC_ENG,	GS_CCMD_CHAT_CHANNEL_LIST_DESC,	FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdChatChannelList,				TRUE, 0);
	LOCALIZED_CMD(GS_CCMD_CHAT_CHANNELJOIN_ENG,		GS_CCMD_CHAT_CHANNELJOIN,	GS_CCMD_CHAT_CHANNELJOIN_DESC_ENG,	GS_CCMD_CHAT_CHANNELJOIN_DESC,	FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdChatChannelJoin,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_CHANNELLEAVE_ENG,	GS_CCMD_CHAT_CHANNELLEAVE,	GS_CCMD_CHAT_CHANNELLEAVE_DESC_ENG,	GS_CCMD_CHAT_CHANNELLEAVE_DESC,	FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdChatChannelLeave,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_REPLY_ENG,			GS_CCMD_CHAT_REPLY,			GS_CCMD_CHAT_REPLY_DESC_ENG,		GS_CCMD_CHAT_REPLY_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdReply,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CHAT_REPLY2_ENG,			GS_CCMD_CHAT_REPLY2,		GS_CCMD_CHAT_REPLY_DESC_ENG,		GS_CCMD_CHAT_REPLY_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdReply,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_ADMIN_ANNOUNCE_ENG,		GS_CCMD_ADMIN_ANNOUNCE,		GS_CCMD_ADMIN_ANNOUNCE_DESC_ENG,	GS_CCMD_ADMIN_ANNOUNCE_DESC,	FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMakeAnnouncement,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,  0, "");
//	LOCALIZED_CMD(GS_CCMD_ADMIN_ALERT_ENG,			GS_CCMD_ADMIN_ALERT,		GS_CCMD_ADMIN_ALERT_DESC_ENG,		GS_CCMD_ADMIN_ALERT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMakeAlert,					TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,  0, "");
	LOCALIZED_CMD(GS_CCMD_ADMIN_CHAT_ENG,			GS_CCMD_ADMIN_CHAT,			GS_CCMD_ADMIN_CHAT_DESC_ENG,		GS_CCMD_ADMIN_CHAT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAdminChat,					TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,  0, "");
	LOCALIZED_CMD(GS_CCMD_GUILD_CHAT_ENG,			GS_CCMD_GUILD_CHAT,			GS_CCMD_GUILD_CHAT_DESC_ENG,		GS_CCMD_GUILD_CHAT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildChat,					TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_GUILD_GCHAT_ENG,			GS_CCMD_GUILD_GCHAT,		GS_CCMD_GUILD_CHAT_DESC_ENG,		GS_CCMD_GUILD_CHAT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGuildChat,					TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CSR_CHAT_ENG,				GS_CCMD_CSR_CHAT,			GS_CCMD_CSR_CHAT_DESC_ENGLISH,		GS_CCMD_CSR_CHAT_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCsrChat,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	LOCALIZED_CMD(GS_CCMD_CSR_CSRCHAT_ENG,			GS_CCMD_CSR_CSRCHAT,		GS_CCMD_CSR_CHAT_DESC_ENGLISH,		GS_CCMD_CSR_CHAT_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCsrChat,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");

//	LOCALIZED_CMD(GS_CCMD_EMOTE_WAVE_ENG,			GS_CCMD_EMOTE_WAVE,			GS_CCMD_EMOTE_WAVE_DESC_ENG,		GS_CCMD_EMOTE_WAVE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteWave,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_CRY_ENG,			GS_CCMD_EMOTE_CRY,			GS_CCMD_EMOTE_CRY_DESC_ENG,			GS_CCMD_EMOTE_CRY_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteCry,						TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_CHEER_ENG,			GS_CCMD_EMOTE_CHEER,		GS_CCMD_EMOTE_CHEER_DESC_ENG,		GS_CCMD_EMOTE_CHEER_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteCheer,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_BOW_ENG,			GS_CCMD_EMOTE_BOW,			GS_CCMD_EMOTE_BOW_DESC_ENG,			GS_CCMD_EMOTE_BOW_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteBow,						TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_BEG_ENG,			GS_CCMD_EMOTE_BEG,			GS_CCMD_EMOTE_BEG_DESC_ENG,			GS_CCMD_EMOTE_BEG_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteBeg,						TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_FLEX_ENG,			GS_CCMD_EMOTE_FLEX,			GS_CCMD_EMOTE_FLEX_DESC_ENG,		GS_CCMD_EMOTE_FLEX_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteFlex,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_KISS_ENG,			GS_CCMD_EMOTE_KISS,			GS_CCMD_EMOTE_KISS_DESC_ENG,		GS_CCMD_EMOTE_KISS_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteKiss,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_LAUGH_ENG,			GS_CCMD_EMOTE_LAUGH,		GS_CCMD_EMOTE_LAUGH_DESC_ENG,		GS_CCMD_EMOTE_LAUGH_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteLaugh,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_POINT_ENG,			GS_CCMD_EMOTE_POINT,		GS_CCMD_EMOTE_POINT_DESC_ENG,		GS_CCMD_EMOTE_POINT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmotePoint,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_STOP_ENG,			GS_CCMD_EMOTE_STOP,			GS_CCMD_EMOTE_STOP_DESC_ENG,		GS_CCMD_EMOTE_STOP_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteStop,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_TAUNT_ENG,			GS_CCMD_EMOTE_TAUNT,		GS_CCMD_EMOTE_TAUNT_DESC_ENG,		GS_CCMD_EMOTE_TAUNT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteTaunt,					TRUE, 0);	
//#if defined(TUGBOAT)
//	LOCALIZED_CMD(GS_CCMD_EMOTE_CHICKEN_ENG,		GS_CCMD_EMOTE_CHICKEN,		GS_CCMD_EMOTE_CHICKEN_DESC_ENG,		GS_CCMD_EMOTE_CHICKEN_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteChicken,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_DOH_ENG,			GS_CCMD_EMOTE_DOH,			GS_CCMD_EMOTE_DOH_DESC_ENG,			GS_CCMD_EMOTE_DOH_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteDoh,						TRUE, 0);	
//#endif
//	LOCALIZED_CMD(GS_CCMD_EMOTE_NO_ENG,				GS_CCMD_EMOTE_NO,			GS_CCMD_EMOTE_NO_DESC_ENG,			GS_CCMD_EMOTE_NO_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteNo,						TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_SALUTE_ENG,			GS_CCMD_EMOTE_SALUTE,		GS_CCMD_EMOTE_SALUTE_DESC_ENG,		GS_CCMD_EMOTE_SALUTE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteSalute,					TRUE, 0);	
//#if defined(TUGBOAT)
//	LOCALIZED_CMD(GS_CCMD_EMOTE_APPLAUD_ENG,		GS_CCMD_EMOTE_APPLAUD,		GS_CCMD_EMOTE_APPLAUD_DESC_ENG,		GS_CCMD_EMOTE_APPLAUD_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteApplaud,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_AMAZED_ENG,			GS_CCMD_EMOTE_AMAZED,		GS_CCMD_EMOTE_AMAZED_DESC_ENG,		GS_CCMD_EMOTE_AMAZED_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteAmazed,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_ANGRY_ENG,			GS_CCMD_EMOTE_ANGRY,		GS_CCMD_EMOTE_ANGRY_DESC_ENG,		GS_CCMD_EMOTE_ANGRY_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteAngry,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_BECKON_ENG,			GS_CCMD_EMOTE_BECKON,		GS_CCMD_EMOTE_BECKON_DESC_ENG,		GS_CCMD_EMOTE_BECKON_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteBeckon,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_CLAPSARCASTIC_ENG,	GS_CCMD_EMOTE_CLAPSARCASTIC,GS_CCMD_EMOTE_CLAPSARCASTIC_DESC_ENG,GS_CCMD_EMOTE_CLAPSARCASTIC_DESC,FALSE,	COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteClapSarcastic,			TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_FLIRT_ENG,			GS_CCMD_EMOTE_FLIRT,		GS_CCMD_EMOTE_FLIRT_DESC_ENG,		GS_CCMD_EMOTE_FLIRT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteFlirt,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_GOODBYE_ENG,		GS_CCMD_EMOTE_GOODBYE,		GS_CCMD_EMOTE_GOODBYE_DESC_ENG,		GS_CCMD_EMOTE_GOODBYE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteGoodbye,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_GREET_ENG,			GS_CCMD_EMOTE_GREET,		GS_CCMD_EMOTE_GREET_DESC_ENG,		GS_CCMD_EMOTE_GREET_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteGreet,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_HANDSTAND_ENG,		GS_CCMD_EMOTE_HANDSTAND,	GS_CCMD_EMOTE_HANDSTAND_DESC_ENG,	GS_CCMD_EMOTE_HANDSTAND_DESC,	FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteHandstand,				TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_HELLO_ENG,			GS_CCMD_EMOTE_HELLO,		GS_CCMD_EMOTE_HELLO_DESC_ENG,		GS_CCMD_EMOTE_HELLO_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteHello,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_IMPATIENT_ENG,		GS_CCMD_EMOTE_IMPATIENT,	GS_CCMD_EMOTE_IMPATIENT_DESC_ENG,	GS_CCMD_EMOTE_IMPATIENT_DESC,	FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteImpatient,				TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_LAUGHAT_ENG,		GS_CCMD_EMOTE_LAUGHAT,		GS_CCMD_EMOTE_LAUGHAT_DESC_ENG,		GS_CCMD_EMOTE_LAUGHAT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteLaughAt,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_LOOKOUT_ENG,		GS_CCMD_EMOTE_LOOKOUT,		GS_CCMD_EMOTE_LOOKOUT_DESC_ENG,		GS_CCMD_EMOTE_LOOKOUT_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteLookout,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_SHRUG_ENG,			GS_CCMD_EMOTE_SHRUG,		GS_CCMD_EMOTE_SHRUG_DESC_ENG,		GS_CCMD_EMOTE_SHRUG_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteShrug,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_SHY_ENG,			GS_CCMD_EMOTE_SHY,			GS_CCMD_EMOTE_SHY_DESC_ENG,			GS_CCMD_EMOTE_SHY_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteShy,						TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_SISSYFIGHT_ENG,		GS_CCMD_EMOTE_SISSYFIGHT,	GS_CCMD_EMOTE_SISSYFIGHT_DESC_ENG,	GS_CCMD_EMOTE_SISSYFIGHT_DESC,	FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteSissyFight,				TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_SOS_ENG,			GS_CCMD_EMOTE_SOS,			GS_CCMD_EMOTE_SOS_DESC_ENG,			GS_CCMD_EMOTE_SOS_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteSOS,						TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_SURRENDER_ENG,		GS_CCMD_EMOTE_SURRENDER,	GS_CCMD_EMOTE_SURRENDER_DESC_ENG,	GS_CCMD_EMOTE_SURRENDER_DESC,	FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteSurrender,				TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_VIOLIN_ENG,			GS_CCMD_EMOTE_VIOLIN,		GS_CCMD_EMOTE_VIOLIN_DESC_ENG,		GS_CCMD_EMOTE_VIOLIN_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteViolin,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_WHISTLE_ENG,		GS_CCMD_EMOTE_WHISTLE,		GS_CCMD_EMOTE_WHISTLE_DESC_ENG,		GS_CCMD_EMOTE_WHISTLE_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteWhistle,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_YAWN_ENG,			GS_CCMD_EMOTE_YAWN,			GS_CCMD_EMOTE_YAWN_DESC_ENG,		GS_CCMD_EMOTE_YAWN_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteYawn,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_YELL_ENG,			GS_CCMD_EMOTE_YELL,			GS_CCMD_EMOTE_YELL_DESC_ENG,		GS_CCMD_EMOTE_YELL_DESC,		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteYell,					TRUE, 0);	
//	LOCALIZED_CMD(GS_CCMD_EMOTE_YES_ENG,			GS_CCMD_EMOTE_YES,			GS_CCMD_EMOTE_YES_DESC_ENG,			GS_CCMD_EMOTE_YES_DESC,			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEmoteYes,						TRUE, 0);	
//#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// END RELEASE CONSOLE COMMANDS ... ONLY PUT DEVELOPMENT/SERVER COMMANDS BELOW
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("gfx");

#endif
#if !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("fps",						NOT_CHEATS,	COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFPS,							TRUE, 0);		CONSOLE_PAR(PARAM_BOOLEAN, FALSE, 0,	0, "");
									CONSOLE_DESC("toggle framerate display.");
#endif
#ifdef _PROFILE
	CONSOLE_CMD("profile_snapshot",			NOT_CHEATS,	COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdProfileSnapshot,				TRUE, 0);		CONSOLE_PAR(PARAM_BOOLEAN, FALSE, 0,	0, "");
									CONSOLE_DESC("Take an instrumented profile snapshot.");
#endif
#if ISVERSION(DEVELOPMENT)
	CONSOLE_CMD("gamma",					FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSetGamma,						TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");		
									CONSOLE_DESC("set gamma level.");		
	CONSOLE_CMD("fov",						FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSetFOV,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");					CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("set vertical field of view.");
	CONSOLE_CMD("fullscreen",				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFullscreen,					TRUE, 0);		CONSOLE_PAR_NV(PARAM_INTEGER, FALSE, "width", "", "");		CONSOLE_PAR_NV(PARAM_INTEGER, FALSE, "height", "", "");
									CONSOLE_DESC("switch to fullscreen mode.");
	CONSOLE_CMD("windowsize",				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSetWindowSize,				TRUE, 0);		CONSOLE_PAR_NV(PARAM_INTEGER, TRUE, "width", "", "");		CONSOLE_PAR_NV(PARAM_INTEGER, TRUE, "height", "", "");
									CONSOLE_DESC("set window size.");
	CONSOLE_CMD("windowinfo",				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWindowInfo,					TRUE, 0);
									CONSOLE_DESC("display window information.");

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("debug");

#ifdef DEBUG_WINDOW	
	CONSOLE_CMD("dbgwin",					TRUE,		COMMAND_BOTH,	COMMAND_CLIENT,		c_sCmdDebugWindow,					TRUE, 0);
									CONSOLE_DESC("toggle external debug window.");
#endif // DEBUG_WINDOW	

	// CHB 2006.10.03 - Inconsistent conditional usage causes compilation errors on release build.

	CONSOLE_CMD("screenshotquality",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdToggleScreenshotQuality,		TRUE, 0);
									CONSOLE_DESC("toggle screen shot quality.");		
	CONSOLE_CMD("screenshottiles",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdScreenShotTiles,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set screen shot tiling factor.");		
	CONSOLE_CMD("pause",					TRUE,		COMMAND_BOTH,	COMMAND_CLIENT,		c_sCmdPause,						TRUE, 0);		CONSOLE_PAR(PARAM_BOOLEAN, FALSE, 0,	0, "");
									CONSOLE_DESC("pause the game.");
	CONSOLE_CMD("displaydevname",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDisplayDevName,				TRUE, 0);
									CONSOLE_DESC("toggle display development name for units.");
	CONSOLE_CMD("displayposition",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDisplayPosition,				TRUE, 0);
									CONSOLE_DESC("toggle display position for units.");
	CONSOLE_CMD("displayvelocity",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDisplayVelocity,				TRUE, 0);
									CONSOLE_DESC("toggle display velocity for units.");
	CONSOLE_CMD("displayid",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDisplayID,					TRUE, 0);
									CONSOLE_DESC("toggle display id for units.");
	CONSOLE_CMD("displayroom",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDisplayRoom,					TRUE, 0);
									CONSOLE_DESC("toggle display room for units.");
	CONSOLE_CMD("displayfaction",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDisplayFaction,				TRUE, 0);
									CONSOLE_DESC("toggle display faction for units.");
	CONSOLE_CMD("collisionladder",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCollisionLadder,				TRUE, 0);
									CONSOLE_DESC("toggle collision ladder display for targeted units.");
	CONSOLE_CMD("collisionshapespathing",	TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCollisionShapesPathing,		TRUE, 0);
									CONSOLE_DESC("toggle pathing collision shapes.");
	CONSOLE_CMD("collisionshapes",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCollisionShapes,				TRUE, 0);
									CONSOLE_DESC("toggle collision shapes.");
	CONSOLE_CMD("missileshapes",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMissileShapes,				TRUE, 0);
									CONSOLE_DESC("toggle missile shapes.");
	CONSOLE_CMD("missilevectors",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMissileVectors,				TRUE, 0);
									CONSOLE_DESC("toggle missile vectors.");
	CONSOLE_CMD("aimingshapes",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAimingShapes,					TRUE, 0);
									CONSOLE_DESC("toggle aiming shapes.");
	CONSOLE_CMD("wireframe",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWireFrame,					TRUE, 0);
									CONSOLE_DESC("toggle wireframe draw mode.");
	CONSOLE_CMD("collisiondebug",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCollisionDebug,				TRUE, 0);
									CONSOLE_DESC("toggle collision debug draw mode.");
	CONSOLE_CMD("collisionmaterialdebug",	TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCollisionMaterialDebug,		TRUE, 0);
									CONSOLE_DESC("toggle collision material debug draw mode.");
	CONSOLE_CMD("hulldraw",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdHullDraw,						TRUE, 0);
									CONSOLE_DESC("toggle room hull drawing.");
	CONSOLE_CMD("appearance",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAppearance,					TRUE, 0);
									CONSOLE_DESC("toggle appearances debug display.");
	CONSOLE_CMD("seed",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdShowSeed,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set or show dungeon seed.");		
	CONSOLE_CMD("statsdebug",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStatsDebug,					TRUE, 0);		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "+,-,c,s,r,m,u", "");		CONSOLE_PAR_NV(PARAM_STRING, FALSE, "match", "", "");
									CONSOLE_DESC("stats debug display (c=client, s=server, u=baseunit, m=modlists, r=riders.");		
	CONSOLE_CMD("debugunitid",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSetStatsDebugUnit,			TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");		
									CONSOLE_DESC("set unit by id for stats debug display.");		
	CONSOLE_CMD("datatabledisplay",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDatatableDisplay,				TRUE, 0);		CONSOLE_PAR(PARAM_TABLENAME, TRUE, 0, 0, "");
									CONSOLE_DESC("dump a data table to the console.");		
	CONSOLE_CMD("location",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdLocation,						TRUE, 0);
									CONSOLE_DESC("trace your location.");		
	CONSOLE_CMD("bars",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdBars,							TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_DEBUG_BARS,	0, "");
									CONSOLE_DESC("toggle debug bars.");		
	CONSOLE_CMD("globaltheme",				TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		c_sCmdGlobalTheme,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_GLOBAL_THEMES,0, "");
									CONSOLE_DESC("enable a global theme.");	
	CONSOLE_CMD("globalthemesdisplay",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGlobalThemesDisplay,			TRUE, 0);		
									CONSOLE_DESC("enable a global theme.");	
	CONSOLE_CMD("leveltheme",				TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		sCmdLevelTheme,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_LEVEL_THEMES,0, "");
									CONSOLE_DESC("enable a level theme.");	
	CONSOLE_CMD("town_forced",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTownForced,					TRUE, 0);
									CONSOLE_DESC("pretend like everywhere is town.");	
	CONSOLE_CMD("unitlist",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUnitList,						TRUE, 0);
									CONSOLE_DESC("dump global unit list.");		
	CONSOLE_CMD("eventlist",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdEventList,					TRUE, 0);
									CONSOLE_DESC("dump global event list.");	
	CONSOLE_CMD("eventhandlerlist",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdEventHandlerList,				TRUE, 0);
									CONSOLE_DESC("dump global event handler list.");	
	CONSOLE_CMD("statstracker",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStatsTracker,					TRUE, 0);
									CONSOLE_DESC("dump global stats find in list counts.");	
#endif // ISVERSION(DEVELOPMENT)
#if(ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))
	CONSOLE_CMD("collisiondraw",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCollisionDraw,				TRUE, 0);
									CONSOLE_DESC("toggle collision draw mode.");
	CONSOLE_CMD("screenshot_ui",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTakeScreenshotWithUI,			TRUE, 0);
									CONSOLE_DESC("take a screen shot with ui.");		
#endif


#if ISVERSION(DEVELOPMENT)
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("performance");
	CONSOLE_CMD("perf",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPerformance,					FALSE, 0);		CONSOLE_PAR(PARAM_PERF, TRUE, 0, 0, "");
									CONSOLE_DESC("performance counter debug display.");
	CONSOLE_CMD("hitcount",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdHitCount,						FALSE, 0);		CONSOLE_PAR(PARAM_HITCOUNT, TRUE, 0, 0, "");
									CONSOLE_DESC("performance hitcount debug display.");
	CONSOLE_CMD("mem",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMemory,						TRUE, 0);
									CONSOLE_DESC("toggle memory usage debug display.");
	CONSOLE_CMD("mempoolinfo", 				FALSE,		COMMAND_CLIENT, COMMAND_CLIENT, 	c_sCmdMemoryPoolInfo,				TRUE, 0);
									CONSOLE_DESC("toggle memory pool usage debug display.");									
	CONSOLE_CMD("mem_snapshot",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMemorySnapShot,				TRUE, 0);
									CONSOLE_DESC("output a snapshot of memory usage to the perf log.");
	CONSOLE_CMD("events",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdEventsDebug,					TRUE, 0);		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "client,server,ui", "client");
									CONSOLE_DESC("toggle events debug display.");
									CONSOLE_USE("events {c=client | s=server | u=ui}");
	CONSOLE_CMD("hashmetrics",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdHashMetrics,					TRUE, 0);
									CONSOLE_DESC("toggle hash metrics display.");		
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("testcode");
	CONSOLE_CMD("environment",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdTestEnvironment,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_LEVEL_ENVIRONMENTS,	0, "");
									CONSOLE_DESC("Environment test.");

	CONSOLE_CMD("tdefinition",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTestDefinition,				TRUE, 0);
									CONSOLE_DESC("defnition test.");
	CONSOLE_CMD("tindexedarray",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTestIndexedArray,				TRUE, 0);
									CONSOLE_DESC("indexed array test.");
	CONSOLE_CMD("phu",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTestPhu,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("phu test.");	
	CONSOLE_CMD("wardrobe_test",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWardrobeTest,					TRUE, 0);
									CONSOLE_DESC("run wardrobe test.");
	CONSOLE_CMD("player_body_test",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPlayerBodyTest,				TRUE, 0);
									CONSOLE_DESC("run player body test.");
	CONSOLE_CMD("globalindexvalidate",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGlobalIndexValidate,			TRUE, 0);
									CONSOLE_DESC("validate global indexes.");
	CONSOLE_CMD("wardrobe_layer",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWardrobeLayer,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_WARDROBE_LAYER,	0, "");
									CONSOLE_DESC("force a wardrobe layer on yourself.");		
	CONSOLE_CMD("colorset",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdColorSet,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_COLORSETS,		0, "");
									CONSOLE_DESC("set your colorset.");		
	CONSOLE_CMD("squish",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSquish,						TRUE, 0);
									CONSOLE_DESC("toggle squish.");		
	CONSOLE_CMD("colorsetmode",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdColorMaskMode,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("set color mask computation mode.");		
	CONSOLE_CMD("colorseterror",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdColorMaskErrorThreshold,		TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("set color mask error threshold.");		
	CONSOLE_CMD("crash",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCrashFromConsole,				TRUE, 0);
									CONSOLE_DESC("cause a crash in the application.");		
	CONSOLE_CMD("assert",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAssert,						TRUE, 0);
									CONSOLE_DESC("cause an assertion in the application.");		
	CONSOLE_CMD("warn",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWarn,							TRUE, 0);
									CONSOLE_DESC("cause a warning in the application.");		
	CONSOLE_CMD("halt",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdHalt,							TRUE, 0);
									CONSOLE_DESC("cause a halt in the application.");		
	CONSOLE_CMD("memcrash",					TRUE,		COMMAND_CLIENT, COMMAND_CLIENT,		c_sCmdMemCrash,						TRUE, 0);
									CONSOLE_DESC("allocate 1GB of memory from the default pool.");
	CONSOLE_CMD("togglesilentasserts",		TRUE,		COMMAND_CLIENT, COMMAND_CLIENT,		c_sCmdToggleSilentAsserts,			TRUE, 0);
									CONSOLE_DESC("toggle the silencing of assert dialogues.");
	CONSOLE_CMD("gameexploreradd",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGameExplorerAdd,				TRUE, 0);		
									CONSOLE_DESC("add application to game explorer.");		
	CONSOLE_CMD("gameexplorerremove",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGameExplorerRemove,			TRUE, 0);		
									CONSOLE_DESC("remove application from game explorer.");		
	CONSOLE_CMD("camerashake",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCameraShake,						FALSE, 0);		CONSOLE_PAR_NA("time", PARAM_FLOAT, FALSE, 0, 0, "");		CONSOLE_PAR_NA("magnitude", PARAM_FLOAT, FALSE, 0, 0, "");		CONSOLE_PAR_NA("damping", PARAM_FLOAT, FALSE, 0, 0, "");		
									CONSOLE_DESC("simulate camera shake.");		
	CONSOLE_CMD("camerashakeoff",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCameraShakeOff,					TRUE, 0);
									CONSOLE_DESC("turn off camera shake.");		
	CONSOLE_CMD("gwho",						FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGWho,							FALSE, 0);
									CONSOLE_DESC("list all guild members client knows about.");
	CONSOLE_CMD("grenamerank",				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGRenameRank,					TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,  0, "");	CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0, 0, "");
									CONSOLE_DESC("change the name of a guild rank.");
	CONSOLE_CMD("gsettings",				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGSettings,					TRUE, 0);
									CONSOLE_DESC("print the current guild settings to the console.");
	CONSOLE_CMD("gperms",					FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGPerms,						TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0, 0, "");	CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0, 0, "");
									CONSOLE_DESC("change the guild action minimum required rank.");
	CONSOLE_CMD("cmarch",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTestCMarch,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("cmarch test.");	
	//CONSOLE_CMD("csrchat",					FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCsrChat,						TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
	//								CONSOLE_DESC("development test command for csr chat.");
	CONSOLE_CMD("csrstart",					FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCsrChatStart,					TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
									CONSOLE_DESC("dev test command for csr sessions.");
	CONSOLE_CMD("csrend",					FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCsrChatEnd,					TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
									CONSOLE_DESC("dev test command for csr sessions.");
	CONSOLE_CMD("csradminchat",				FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCsrChatAdmin,					TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");	CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
									CONSOLE_DESC("dev test command for csr sessions.");
	CONSOLE_CMD("csrrenameguild",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCsrRenameGuild,				TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, ""); CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
									CONSOLE_DESC("dev test command for renaming guilds.");
	CONSOLE_CMD("advertiseparty",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyAdvertise,				TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, TRUE, 0,	0, "");
									CONSOLE_DESC("dev test command for advertising parties.");
	CONSOLE_CMD("unadvertiseparty",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyStopAdvertising,			FALSE, 0);
									CONSOLE_DESC("dev test command for removing an advertised party from the ad list.");
	CONSOLE_CMD("getadvertisedparties",		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPartyGetAdvertised,			FALSE, 0);
									CONSOLE_DESC("dev test command for getting a list of advertised parties.");
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("d3d");
	CONSOLE_CMD("batches",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdBatches,						TRUE, 0);
									CONSOLE_DESC("toggle batch count debug display.");
	CONSOLE_CMD("dpvsstats",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDPVSStats,					TRUE, 0);
									CONSOLE_DESC("toggle dPVS stats debug display.");
	CONSOLE_CMD("dpvsflags",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDPVSFlags,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("toggle dPVS debug display flags.");
	CONSOLE_CMD("lights",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdLights,						TRUE, 0);
									CONSOLE_DESC("toggle lights draw mode.");
	CONSOLE_CMD("maxdetail",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMaxDetail,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("set max texture detail level.");
	CONSOLE_CMD("fogcolor",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFogColor,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");	CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");	CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("set fog color (rgb).");
	CONSOLE_CMD("dbg_render",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugRenderFlag,				TRUE, 0);
									CONSOLE_DESC("toggle debug render mode.");
	CONSOLE_CMD("shadows",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdShadowFlag,					TRUE, 0);
									CONSOLE_DESC("toggle shadow rendering.");
	CONSOLE_CMD("dbg_texture",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugTextureFlag,				TRUE, 0);
									CONSOLE_DESC("toggle debug texture render mode.");
	CONSOLE_CMD("clipdraw",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdClipDraw,						TRUE, 0);
									CONSOLE_DESC("toggle clipping planes draw mode.");
	CONSOLE_CMD("debugdraw",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugDraw,					TRUE, 0);
									CONSOLE_DESC("toggle debug draw mode.");
	CONSOLE_CMD("farclip",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFarClip,						TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("set far clip plane.");
	CONSOLE_CMD("fogdistances",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFogDistances,					TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("set fog distances.");
	CONSOLE_CMD("fogdensity",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFogDensity,					TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("set fog density.");
	CONSOLE_CMD("cg",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCGSet,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_RENDER_FLAGS,	0, "");		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("set render flags.");
	CONSOLE_CMD("cgshow",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCGShow,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_RENDER_FLAGS,	0, "");
									CONSOLE_DESC("display render flag settings.");
	CONSOLE_CMD("d3dstates",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdD3DStatesStats,				TRUE, 0);
									CONSOLE_DESC("toggle d3d states display.");		
	CONSOLE_CMD("d3dstats",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdD3DStats,						TRUE, 0);
									CONSOLE_DESC("toggle d3d stats display.");		
	CONSOLE_CMD("d3dresstats",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdD3DResStats,					TRUE, 0);
									CONSOLE_DESC("toggle d3d resource stats display.");		
	CONSOLE_CMD("particledrawstats",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdParticleDrawStats,			TRUE, 0);
									CONSOLE_DESC("toggle particle draw stats display.");		
	CONSOLE_CMD("dbg_dumpdrawlist",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugDumpDrawLists,			TRUE, 0);
									CONSOLE_DESC("trace draw lists.");		
	CONSOLE_CMD("modellist",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugModelList,				TRUE, 0);
									CONSOLE_DESC("list model definitions.");		
	CONSOLE_CMD("texturelist",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugTextureList,				TRUE, 0);
									CONSOLE_DESC("list textures.");		
	CONSOLE_CMD("dumptextures",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugDumpTextures,			TRUE, 0);
									CONSOLE_DESC("trace textures.");		
	CONSOLE_CMD("dumpeffects",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugDumpEffects,				TRUE, 0);
									CONSOLE_DESC("trace effects.");		
	CONSOLE_CMD("enginemem",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugDumpEngineMemory,		TRUE, 0);
									CONSOLE_DESC("show engine memory usage.");		
	CONSOLE_CMD("enginereap",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugReapEngineMemory,		TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("Reclaim engine memory.");		
	CONSOLE_CMD("removemodel",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugModelRemove,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("remove a model.");		
	CONSOLE_CMD("enginecleanup",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdEngineCleanup,				TRUE, 0);
									CONSOLE_DESC("perform a d3d reset.");		
	CONSOLE_CMD("dumpmiplevels",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugDumpMIPLevels,			TRUE, 0);
									CONSOLE_DESC("trace mip levels.");		
	CONSOLE_CMD("d3dreset",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdD3DReset,						TRUE, 0);
									CONSOLE_DESC("perform a d3d reset.");		
	CONSOLE_CMD("displaycaps",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDisplayCaps,					TRUE, 0);
									CONSOLE_DESC("trace d3d caps.");		
	CONSOLE_CMD("generateenvmap",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdGenerateEnvMap,				TRUE, 0);		CONSOLE_PAR_NA("size", PARAM_INTEGER, FALSE, 0,	0, "");		
									CONSOLE_DESC("generate environment map.");		
	CONSOLE_CMD("fxreload",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFXReload,						TRUE, 0);
									CONSOLE_DESC("reload materials.");		
	CONSOLE_CMD("fxinfo",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFXInfo,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("dump effect info to debug output.");
	CONSOLE_CMD("fxlist",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFXList,						TRUE, 0);
									CONSOLE_DESC("dump effect list to debug output.");		
	CONSOLE_CMD("pcsslightsize",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPCSSLightSize,				TRUE, 0);
									CONSOLE_DESC("set light size for PCSS.");
	CONSOLE_CMD("switchtoref",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,			c_sSwitchToRef,						TRUE, 0);
									CONSOLE_DESC("Switch to Refrast");
	CONSOLE_CMD("dofparams",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDOFParams,					TRUE, 0);
									CONSOLE_DESC("set DOF params near, focus, far");
	CONSOLE_CMD("tron_model",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTraceModel,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set trace model by id.");		
	CONSOLE_CMD("tron_room",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTraceRoom,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set trace room by id.");		
	CONSOLE_CMD("tron_query",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTraceQuery,					TRUE, 0);		CONSOLE_PAR(PARAM_HEX, TRUE, 0,	0, "");
									CONSOLE_DESC("set trace query by address.");		
	CONSOLE_CMD("tron_event",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTraceEvent,					TRUE, 0);
									CONSOLE_DESC("toggle engine event tracing.");		
	CONSOLE_CMD("trace_batches",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTraceBatches,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("trace batches by metrics group.");		
	CONSOLE_CMD("dbg_identifymodel",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugIdentifyModel,			TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set engine debug identify model.");		
	CONSOLE_CMD("dbg_dumpocclusion",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugDumpOcclusion,			TRUE, 0);
									CONSOLE_DESC("dump occlusion info.");		
	CONSOLE_CMD("preloadtexture",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugPreloadTexture,			TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("preload a texture.");		
	CONSOLE_CMD("preloadmodel",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugPreloadModelTextures,	TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("preload textures for a model.");		

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("physics");
	CONSOLE_CMD("ragdollenableall",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdRagdollEnableAll,				TRUE, 0);
									CONSOLE_DESC("enable all ragdolls.");
	CONSOLE_CMD("ragdolls",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdToggleRagdolls,				TRUE, 0);
									CONSOLE_DESC("toggle ragdolls.");		
	CONSOLE_CMD("shape",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdShape,						TRUE, 0);		CONSOLE_PAR_NA("height", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("weight", PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("set your shape.");		
	CONSOLE_CMD("hkfxdebug",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDebugHavokFX,					TRUE, 0);		CONSOLE_PAR_NV(PARAM_STRINGLIST, TRUE, "", "h3,fx", "");		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");		
									CONSOLE_DESC("havok fx debug.");		
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("particles");
	CONSOLE_CMD("particledebug",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdToggleParticleDebug,			TRUE, 0);
									CONSOLE_DESC("toggle particle debug display.");		
	CONSOLE_CMD("particle",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdParticle,						TRUE, 0);		CONSOLE_PAR(PARAM_DEFINITION, TRUE, DEFINITION_GROUP_PARTICLE,	0, "");
									CONSOLE_DESC("create a particle system.");		
	CONSOLE_CMD("particle_censor",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdParticleCensor,				TRUE, 0);	
									CONSOLE_DESC("toggle particle censorship.");		
	CONSOLE_CMD("particle_async",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdParticleAsync,				TRUE, 0);	
									CONSOLE_DESC("toggle async particle systems.");		
	CONSOLE_CMD("particle_dx10",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdParticleDX10,					TRUE, 0);	
									CONSOLE_DESC("toggle dx10 particle systems.");		
	CONSOLE_CMD("particle_destroyall",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdParticleDestroyAll,			TRUE, 0);	
									CONSOLE_DESC("destroy all currently running particle systems.");		
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("rooms");
	CONSOLE_CMD("revis",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdRevis,						TRUE, 0);
									CONSOLE_DESC("recalculate visible rooms for the current room.");
	CONSOLE_CMD("room",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdRoomDebug,					TRUE, 0);
									CONSOLE_DESC("toggle room debug display.");
	CONSOLE_CMD("drawallrooms",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdRoomDrawAll,					TRUE, 0);
									CONSOLE_DESC("toggle draw all rooms mode.");
	CONSOLE_CMD("drawconnected",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdRoomDrawConnected,			TRUE, 0);
									CONSOLE_DESC("toggle draw connected rooms mode.");
	CONSOLE_CMD("drawnearby",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdRoomDrawNearby,				TRUE, 0);
									CONSOLE_DESC("toggle draw nearby rooms mode.");
	CONSOLE_CMD("isclearflat",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdIsClearFlat,					TRUE, 0);
									CONSOLE_DESC("is the current player position a 'clear flat area'");									
	CONSOLE_CMD("roomreset",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdRoomReset,					TRUE, 0);
									CONSOLE_DESC("reset room.");
	CONSOLE_CMD("prop",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		x_sCmdProp,							TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_PROPS, 0, "");
									CONSOLE_DESC("spawn a prop.");
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("models");
	CONSOLE_CMD("modelinfo",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdModelDebug,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0, 0, "");
									CONSOLE_DESC("toggle model info display for a model id.");
	CONSOLE_CMD("animtrace",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAnimTrace,					TRUE, 0);
									CONSOLE_DESC("toggle animation debug display.");		
	CONSOLE_CMD("animdump",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAnimDump,						TRUE, 0);
									CONSOLE_DESC("trace anims.");		
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("skills");
	CONSOLE_CMD("skilldebug",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSkillDebug,					TRUE, 0);
									CONSOLE_DESC("toggle skill debug display.");
	CONSOLE_CMD("skilldescriptions",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSkillDescriptions,			TRUE, 0);
									CONSOLE_DESC("writes to a file descriptions of all skills for all levels, for the current player.");
	CONSOLE_CMD("skillstringnumbers",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSkillStringNumbers,			TRUE, 0);
									CONSOLE_DESC("the bottom of skill descriptions will show the numbers available for the string.");

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("monsters");
	CONSOLE_CMD("monsterc",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMonsterClient,				FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_MONSTERS, 0, "");
									CONSOLE_DESC("spawn a client-only monster.");
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("ai");
	CONSOLE_CMD("aidebug",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAIDebug,						TRUE, 0);
									CONSOLE_DESC("toggle ai debug display.");
	CONSOLE_CMD("pathnodeontop",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPathNodeOnTop,				TRUE, 0);
									CONSOLE_DESC("toggle path nodes on top display.");
	CONSOLE_CMD("pathnodeclient",		TRUE,			COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPathNodeClient,				TRUE, 0);
									CONSOLE_DESC("toggle between client/server path nodes");
	CONSOLE_CMD("pathnoderadii",		TRUE,			COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPathNodeRadii,				TRUE, 0);
									CONSOLE_DESC("toggle showing path node radii");
#endif // ISVERSION(DEVELOPMENT)
#if(ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))
	CONSOLE_CMD("pathnode",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPathNode,						TRUE, 0);
									CONSOLE_DESC("toggle path node debug display.");
	CONSOLE_CMD("pathnodeconnection",		CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPathNodeConnection,			TRUE, 0);
									CONSOLE_DESC("toggle path node connections display.");
#endif
#endif
#if ISVERSION(DEVELOPMENT)
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("objects");
		CONSOLE_CMD("object",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdObject,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_OBJECTS, 0, "");
									CONSOLE_DESC("spawn an object.");
									CONSOLE_USE("object OBJECT_NAME [, #]");
#if !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("lightadd",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdLightAdd,						TRUE, 0);
									CONSOLE_DESC("spawn a light.");

	CONSOLE_CMD("triggerreset",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTriggerReset,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_OBJECTS, 0, "");
									CONSOLE_DESC("reset object trigger.");		

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("quests");
	CONSOLE_CMD("taskgeneratetimedelay",	TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTaskGenerateTimeDelay,		TRUE, 0);
									CONSOLE_DESC("toggle global task generate time delay.");

#endif // !ISVERSION(SERVER_VERSION)
#endif // DEVELOPMENT
#if (ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))
	CONSOLE_CMD("questactivate",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestActivate,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUEST,	0, "");
									CONSOLE_DESC("activate a quest.");
#endif
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
	CONSOLE_CMD("showxpvisuals",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTugboatXPVisuals,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT,	0, "");
									CONSOLE_DESC("complete quest.");

	CONSOLE_CMD("SetHubVisited",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSetLevelAreaVisited,				TRUE, 0);	
									CONSOLE_DESC("complete quest.");

	CONSOLE_CMD("questtugboatcomplete",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestCompleteTugboat,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT,	0, "");
									CONSOLE_DESC("complete quest.");

	CONSOLE_CMD("questtugboatclearquest",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestAbandonTugboat,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT,	0, "");
									CONSOLE_DESC("complete quest.");

	CONSOLE_CMD("questtugboatspawnbossforquests",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestTugobatSpawnBosses,	TRUE, 0);			CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT,	0, "");
									CONSOLE_DESC("complete quest.");

	CONSOLE_CMD("questtugboatadvance",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestAdvanceTugboat,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT,	0, "");
									CONSOLE_DESC("complete quest.");
	CONSOLE_CMD("questcomplete",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestComplete,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUEST,	0, "");
									CONSOLE_DESC("complete quest.");
	CONSOLE_CMD("questadvance",				CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestAdvance,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUEST,	0, "");
									CONSOLE_DESC("advance quest state.");
	CONSOLE_CMD("questreset",				CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestReset,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUEST,	0, "");
									CONSOLE_DESC("reset a quest.");									
#endif

#if ISVERSION(DEVELOPMENT)
	CONSOLE_CMD("questupdatecastinfo",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestUpdateCastInfo,			TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUEST,	0, "");
									CONSOLE_DESC("update cast info for a quest.");
	CONSOLE_CMD("queststatescomplete",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdQuestStatesComplete,			TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_QUEST,	0, "");
									CONSOLE_DESC("set quest states to complete.");

#if ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("taskkillalltargets",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTaskKillAllTargets,			TRUE, 0);
									CONSOLE_DESC("kill all task targets.");
	CONSOLE_CMD("taskkilltarget",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTaskKillTarget,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("kill a task target by index.");
	CONSOLE_CMD("taskcompleteall",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTaskCompleteAll,				TRUE, 0);
									CONSOLE_DESC("complete all tasks.");
#endif  // ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("factionchange",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdFactionChange,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_FACTION,	0, "");		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_USE("factionchange <faction> <points>");
									CONSOLE_DESC("change faction score.");		

#endif // ISVERSION(DEVELOPMENT)

#if !ISVERSION(SERVER_VERSION)

#if ISVERSION(DEVELOPMENT)

	CONSOLE_CMD("taskaccept",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTaskAccept,					TRUE, 0);
									CONSOLE_DESC("accept all available tasks.");
	CONSOLE_CMD("taskacceptreward",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTaskAcceptReward,				TRUE, 0);
									CONSOLE_DESC("accept all available task rewards.");
	CONSOLE_CMD("taskreject",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTaskReject,					TRUE, 0);
									CONSOLE_DESC("reject all available tasks.");
	CONSOLE_CMD("taskabandon",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTaskAbandon,					TRUE, 0);
									CONSOLE_DESC("abandon all tasks.");
	CONSOLE_CMD("factiondisplay",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDisplayFaction,				TRUE, 0);
									CONSOLE_DESC("display faction info.");		
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("network");
	CONSOLE_CMD("sync",						TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSync,							TRUE, 0);
									CONSOLE_DESC("toggle sync debug display.");
	CONSOLE_CMD("forcesync",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdForceSync,					TRUE, 0);
									CONSOLE_DESC("toggle force network sync.");
	CONSOLE_CMD("sendmessage",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSendMessage,					FALSE, 0);	CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");	CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
									CONSOLE_DESC("send arbitrary network message to server.");	
	CONSOLE_CMD("sendchatmessage",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSendChatMessage,				FALSE, 0);	CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");	CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
									CONSOLE_DESC("send arbitrary network message to chat server.");	
	CONSOLE_CMD("netmsgcount",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdNetMsgCount,					TRUE, 0);
									CONSOLE_DESC("trace network message counts.");
	CONSOLE_CMD("netmsgcountreset",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdNetMsgCountReset,					TRUE, 0);
									CONSOLE_DESC("reset trace network message counts.");

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("filesystem");
	CONSOLE_CMD("paktrace",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPakTrace,						TRUE, 0);
									CONSOLE_DESC("trace all files in the pakfile to the log.");
	CONSOLE_CMD("filetypeloadeddump",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdFileTypeLoadedDump,			TRUE, 0);
									CONSOLE_DESC("generate loaded file types report.");		
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("sound");
	CONSOLE_CMD("soundhack",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundToggleHack,				TRUE, 0);
									CONSOLE_DESC("toggle sound 3rd-person listener hack on/off.");
	CONSOLE_CMD("sound",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundToggle,					TRUE, 0);
									CONSOLE_DESC("toggle sound on/off.");
	CONSOLE_CMD("musicdebug",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMusic,						TRUE, 0);
									CONSOLE_DESC("toggle music debug display.");
	CONSOLE_CMD("reverbdebug",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdReverbDebug,					TRUE, 0);
									CONSOLE_DESC("toggle reverb debug display.");
	CONSOLE_CMD("soundbusdebug",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundBusDebug,				TRUE, 0);
									CONSOLE_DESC("toggle sound bus debug display.");
	CONSOLE_CMD("soundmixstatedebug",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundMixStateDebug,			TRUE, 0);
									CONSOLE_DESC("toggle sound mix state debug display.");

	CONSOLE_CMD("sounddebugvirtual",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdVirtualSoundDebug,			TRUE, 0);
									CONSOLE_DESC("toggle virtual sounds debug display.");
	CONSOLE_CMD("sounddebugstopped",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStoppedSoundDebug,			TRUE, 0);
									CONSOLE_DESC("toggle virtual sounds debug display.");
	CONSOLE_CMD("soundmemorydebug",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundMemoryDebug,				TRUE, 0);
									CONSOLE_DESC("toggle sound memory debug display.");
	CONSOLE_CMD("soundmemorydump",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundMemoryDump,				TRUE, 0);
									CONSOLE_DESC("dump sound memory stats.");
	CONSOLE_CMD("soundtrace",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundTrace,					TRUE, 0);
									CONSOLE_DESC("toggle sound tracing debug display.");
	CONSOLE_CMD("reverbreset",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdReverbReset,					TRUE, 0);
									CONSOLE_DESC("force reset the reverb to off.");
	CONSOLE_CMD("playsound",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPlaySound,					FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SOUNDS,	0, "");		CONSOLE_PAR_NA("distance", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("cutoff", PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("play a sound.");		
									CONSOLE_USE("playsound sound [distance=#] [cutoff=#]");
	CONSOLE_CMD("stopsound",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStopSound,					TRUE, 0);		CONSOLE_PAR_NA("soundid", PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("stop playing a sound by id.");		
	CONSOLE_CMD("soundload",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdLoadSound,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SOUNDS,	0, "");
									CONSOLE_DESC("load a sound by name.");		
	CONSOLE_CMD("soundunload",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUnloadSound,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SOUNDS,	0, "");
									CONSOLE_DESC("unload a sound by name.");		
	CONSOLE_CMD("soundloadall",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdLoadAllSounds,				TRUE, 0);
									CONSOLE_DESC("load, play, and unload every sound in the system.");		
	CONSOLE_CMD("playallsounds",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPlayAllSounds,				TRUE, 0);
									CONSOLE_DESC("play all sounds test.");		
	CONSOLE_CMD("stopallsounds",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStopAllSounds,				TRUE, 0);
									CONSOLE_DESC("stop all sounds.");		
	CONSOLE_CMD("playmusic",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPlayMusic,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_MUSIC,	0, "");		CONSOLE_PAR_NA("token", PARAM_STRING, TRUE, 0,	0, "");
									CONSOLE_DESC("play music track.");		
	CONSOLE_CMD("stopmusic",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStopMusic,					TRUE, 0);
									CONSOLE_DESC("stop playing music.");		
	CONSOLE_CMD("setmusicgroove",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSetMusicGroove,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_MUSICGROOVELEVELS,	0, "");		
									CONSOLE_DESC("set music groove level.");		
	CONSOLE_CMD("musicgroovetoggle",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMusicGrooveToggle,			TRUE, 0);
									CONSOLE_DESC("toggle automatic groove change.");		
	CONSOLE_CMD("playbgsound",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPlayBGSound,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_BACKGROUNDSOUNDS,	0, "");		CONSOLE_PAR_NA("token", PARAM_STRING, TRUE, 0,	0, "");
									CONSOLE_DESC("play background sound track.");		
	CONSOLE_CMD("stopbgsound",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStopBGSound,					TRUE, 0);
									CONSOLE_DESC("stop playing background sound.");		
	CONSOLE_CMD("mixstate",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundSetMixState,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SOUND_MIXSTATES,	0, "");		CONSOLE_PAR_NA("time", PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("play background sound track.");		
	CONSOLE_CMD("soundchaos",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundChaos,					FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_SOUNDS,	0, "");		CONSOLE_PAR_NA("time", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("count", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("delay", PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("sound chaos test.");		
	CONSOLE_CMD("soundchaosstop",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundChaosStop,				TRUE, 0);
									CONSOLE_DESC("stop sound chaos test.");		
	CONSOLE_CMD("setrollofffactor",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundSetRolloff,				TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, FALSE, 0,	0, "");
									CONSOLE_DESC("set sound rolloff factor.");		
	CONSOLE_CMD("deltasound",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDeltaSound,					FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SOUNDS,	0, "");		CONSOLE_PAR_NA("token", PARAM_STRING, TRUE, 0,	0, "");		CONSOLE_PAR_NA("vol", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("min", PARAM_FLOAT, FALSE, 0,	0, "");
																																			CONSOLE_PAR_NA("max", PARAM_FLOAT, FALSE, 0,	0, "");		CONSOLE_PAR_NA("cc", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("fv", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("vv", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("mwr", PARAM_INTEGER, FALSE, 0,	0, "");
																																			CONSOLE_PAR_NA("rad", PARAM_FLOAT, FALSE, 0,	0, "");		CONSOLE_PAR_NA("fi", PARAM_FLOAT, FALSE, 0,	0, "");		CONSOLE_PAR_NA("fo", PARAM_FLOAT, FALSE, 0,	0, "");		CONSOLE_PAR_NA("hp", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("gp", PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("change a sound and copy to clipboard.");		
	CONSOLE_CMD("deltaoverride",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDeltaOverride,				FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SOUNDS,	0, "");		CONSOLE_PAR_NA("token", PARAM_STRING, TRUE, 0,	0, "");		CONSOLE_PAR_NA("vol", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("min", PARAM_FLOAT, FALSE, 0,	0, "");
																																			CONSOLE_PAR_NA("max", PARAM_FLOAT, FALSE, 0,	0, "");				CONSOLE_PAR_NA("fv", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("vv", PARAM_INTEGER, FALSE, 0,	0, "");		CONSOLE_PAR_NA("mwr", PARAM_INTEGER, FALSE, 0,	0, "");
																																			CONSOLE_PAR_NA("rad", PARAM_FLOAT, FALSE, 0,	0, "");				CONSOLE_PAR_NA("fi", PARAM_FLOAT, FALSE, 0,	0, "");			CONSOLE_PAR_NA("fo", PARAM_FLOAT, FALSE, 0,	0, "");			CONSOLE_PAR_NA("rvs", PARAM_INTEGER, FALSE, 0,	0, "");
																																			CONSOLE_PAR_NA("fs", PARAM_FLOAT, FALSE, 0,	0, "");					CONSOLE_PAR_NA("cs", PARAM_FLOAT, FALSE, 0,	0, "");			CONSOLE_PAR_NA("lfe", PARAM_FLOAT, FALSE, 0,	0, "");		CONSOLE_PAR_NA("rs", PARAM_FLOAT, FALSE, 0,	0, "");
																																			CONSOLE_PAR_NA("ss", PARAM_FLOAT, FALSE, 0,	0, "");
									CONSOLE_DESC("change a sound override and copy to clipboard.");		
#endif // ISVERSION(DEVELOPMENT)
#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
	CONSOLE_CMD("sounddebug",				CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSoundDebug,					TRUE, 0);
									CONSOLE_DESC("toggle sounds debug display.");
	CONSOLE_CMD("bgsounddebug",				CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdBGSoundDebug,					TRUE, 0);
									CONSOLE_DESC("toggle background sounds debug display.");
	CONSOLE_CMD("musicscriptdebug",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdMusicScriptDebug,				TRUE, 0);
									CONSOLE_DESC("toggle music debug display.");
#endif
#if(ISVERSION(DEVELOPMENT))
CONSOLE_CATEGORY("input");
	CONSOLE_CMD("invertmouse",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdInvertMouse,					TRUE, 0);
									CONSOLE_DESC("toggle inverted mouse.");		
	CONSOLE_CMD("directmouseinput",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDirectInputMouse,				TRUE, 0);
									CONSOLE_DESC("use direct input for the mouse.");		
	CONSOLE_CMD("windowsmouseinput",		TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdWindowsInputMouse,			TRUE, 0);
									CONSOLE_DESC("use windows message input for the mouse.");		
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("ui");
	CONSOLE_CMD("uidebug",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIInputDebug,					TRUE, 0);
									CONSOLE_DESC("toggle ui input debug display.");
	CONSOLE_CMD("uigfxdebug",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIGfxDebug,					TRUE, 0);
									CONSOLE_DESC("toggle ui input debug display.");
	CONSOLE_CMD("uitoggledebug",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIToggleDebug,				TRUE, 0);
									CONSOLE_DESC("toggle ui debug parameter.");
	CONSOLE_CMD("uiedit",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIEdit,						TRUE, 0);		CONSOLE_PAR(PARAM_UI_COMPONENT, FALSE, 0, 0, "");
									CONSOLE_DESC("toggle ui edit debug display.");
	CONSOLE_CMD("uirepaint",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIRepaint,					TRUE, 0);
									CONSOLE_DESC("send a paint message to the ui.");
	CONSOLE_CMD("namelabelheight",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdNameLabelHeight,				TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("set name label height override.");
	CONSOLE_CMD("jabber",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdJabber,						TRUE, 0);
									CONSOLE_DESC("toggle gobbledygook text.");		
	CONSOLE_CMD("textlength",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTextLength,					TRUE, 0);
									CONSOLE_DESC("toggle show text lengths.");		
	CONSOLE_CMD("textlabels",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTextLabels,					TRUE, 0);
									CONSOLE_DESC("toggle show text labels.");		
	CONSOLE_CMD("uiclipping",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIClipping,					TRUE, 0);
									CONSOLE_DESC("toggle ui clipping.");		
	CONSOLE_CMD("textpointsize",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTextPointSize,				TRUE, 0);
									CONSOLE_DESC("toggle show text point size.");		
	CONSOLE_CMD("textfontname",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTextFontName,					TRUE, 0);
									CONSOLE_DESC("toggle show text point size.");		
	CONSOLE_CMD("textdeveloper",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTextDeveloper,				TRUE, 0);
									CONSOLE_DESC("toggle dev text.");		
	CONSOLE_CMD("english",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTextDeveloper,				TRUE, 0);
									CONSOLE_DESC("toggle dev text.");		
	CONSOLE_CMD("logkeyhandler",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdKeyHandlerLoggingToggle,		TRUE, 0);
									CONSOLE_DESC("toggle key handler logging.");		
	CONSOLE_CMD("toggle_target",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdToggleTargetIndicator,		TRUE, 0);
									CONSOLE_DESC("toggle target indicator.");		
	CONSOLE_CMD("toggle_brackets",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdToggleTargetBrackets,			TRUE, 0);
									CONSOLE_DESC("toggle targeting brackets.");		
	CONSOLE_CMD("uicompids",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdListUIIDs,					TRUE, 0);		CONSOLE_PAR(PARAM_STRING, FALSE, 0,	0, "");
									CONSOLE_DESC("list ui component ids.");		
	CONSOLE_CMD("uitron",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUITron,						FALSE, 0);		CONSOLE_PAR_NA("component", PARAM_STRING, FALSE, 0,	0, "");		CONSOLE_PAR_NA("message", PARAM_STRING, FALSE, 0,	0, "");		CONSOLE_PAR_NA("result", PARAM_STRING, FALSE, 0,	0, "");
									CONSOLE_DESC("set ui message trace.");		
	CONSOLE_CMD("uitroff",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUITroff,						TRUE, 0);
									CONSOLE_DESC("turn off ui message trace.");		
	CONSOLE_CMD("uiactivatetrace",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIActivateTrace,				TRUE, 0);		
									CONSOLE_DESC("toggle ui activate trace.");		
	CONSOLE_CMD("uireload",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIReload,						TRUE, 0);
									CONSOLE_DESC("force reload of all ui.");		
	CONSOLE_CMD("textshadow",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdTextShadow,					TRUE, 0);
									CONSOLE_DESC("toggle drop shadows for text.");		
	CONSOLE_CMD("alwasyshowitems",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAlwaysShowItemNames,			TRUE, 0);
									CONSOLE_DESC("Always show the item names on the ground.");		
	CONSOLE_CMD("statdispdebug",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdStatDispDebug,    			TRUE, 0);
									CONSOLE_DESC("Show the debug line for stat disp labels");		
	CONSOLE_CMD("uishowaccuracy",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUIShowAccuracyBrackets,		TRUE, 0);
									CONSOLE_DESC("Show brackets for the accuracy of the gun.");		
	CONSOLE_CMD("uitesttipdialog",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdUITestTipDialog,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");	
									CONSOLE_DESC("Show the tip dialog with a specified string");		
#endif // DEVELOPMENT

#if(ISVERSION(DEVELOPMENT))
	CONSOLE_CATEGORY("AH");
	CONSOLE_CMD("ahsearch",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdAHSearch,					TRUE, 0);
	CONSOLE_DESC("search");		
#endif

#endif // !SERVER_VERSION


// -----------------------------
// -----------------------------
#if !ISVERSION(SERVER_VERSION)

CONSOLE_CATEGORY("billing");
	CONSOLE_CMD("get_billing_account_status", NOT_CHEATS, COMMAND_CLIENT, COMMAND_CLIENT, s_sCmdGetBillingAccountStatus, FALSE, 0);	
		CONSOLE_DESC("request account status from billing provider");	

	CONSOLE_CMD("get_billing_remaining_time", CHEATS_RELEASE_TESTER, COMMAND_CLIENT, COMMAND_CLIENT, c_sCmdBillingDisplayRemainingTime, FALSE, 0);	
		CONSOLE_DESC("display the time remaining on the account");	

	CONSOLE_CMD("real_money_txn", CHEATS_RELEASE_TESTER, COMMAND_CLIENT, COMMAND_CLIENT, s_sCmdRealMoneyTxn, FALSE, 0);	
		CONSOLE_PAR_NA("Item Code", PARAM_INTEGER, true, 0, 0, "");
		CONSOLE_PAR_NA("Item Description", PARAM_STRING, true, 0, 0, "");
		CONSOLE_PAR_NA("Price", PARAM_INTEGER, true, 0, 0, "");
		CONSOLE_DESC("initiate a real money transaction");

  CONSOLE_CMD("debit_ping0ts", NOT_CHEATS, COMMAND_CLIENT, COMMAND_CLIENT, s_sCmdBillingDebitCurrency, FALSE, 0);	
	  CONSOLE_PAR_NA("Amount to Debit", PARAM_INTEGER, true, 0, 0, "");
    CONSOLE_DESC("request debit ping0ts at billing provider");

  CONSOLE_CMD("debit_confirm", NOT_CHEATS, COMMAND_CLIENT, COMMAND_CLIENT, s_sCmdBillingDebitConfirm, FALSE, 0);	
	  CONSOLE_PAR_NA("Authorization ID", PARAM_STRING, true, 0, 0, "");
    CONSOLE_DESC("confirm a previously authorized debit at billing provider");

#endif //!ISVERSION(SERVER_VERSION)

//#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)

//#endif


// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("multiplayer");
	CONSOLE_CMD("getgameserver",CHEATS_RELEASE_CSR|CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGetGameServer,				TRUE, 0);
									CONSOLE_DESC("specifies which server instance you are on.");
	CONSOLE_CMD("playedserver",CHEATS_RELEASE_CSR|CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayedServer,				TRUE, 0);
									CONSOLE_DESC("see what server thinks /played should display.");
	CONSOLE_CMD("seekduel",CHEATS_RELEASE_CSR|CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSeekDuel,				TRUE, 0);
									CONSOLE_DESC("seeks a duel through automated matchmaker.");
	CONSOLE_CMD("seekcancel",CHEATS_RELEASE_CSR|CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSeekCancel,				TRUE, 0);
									CONSOLE_DESC("cancels a seek through automated matchmaker.");
	CONSOLE_CMD("donate",NOT_CHEATS,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdDonate,				TRUE, 0); CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("Sends money to the donation pool to start an event."); //TODO: localize this
	CONSOLE_CMD("donationsettotalcost",CHEATS_RELEASE_CSR|CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdDonationSetTotalCost,				TRUE, 0);  CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");	CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");  CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("Sets how much it costs total to start an event. /donationsetotalcost <in-game-money> <announcement-threshhold> <max-events>");

#if ISVERSION(DEVELOPMENT)
	CONSOLE_CMD("sendmessagetoclient",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSendMessageToClient,			FALSE, 0);	CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");	CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
									CONSOLE_DESC("send arbitrary network message to client from server.");
	CONSOLE_CMD("sendrandommessagestoclient",TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSendRandomMessagesToClient,	FALSE, 0);
									CONSOLE_DESC("repeatedly send randomly generated network messages to client from server.");
#if ISVERSION(DEBUG_VERSION) && !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("sendrandomchatmessagestoclient",TRUE,	COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdSendRandomChatMessagesToClient,FALSE, 0);
									CONSOLE_DESC("repeatedly send randomly generated chat messages to client from server.");
#endif
	CONSOLE_CMD("team",			CHEATS_NORELEASE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTeam,							TRUE, 0);		CONSOLE_PAR(PARAM_TEAM, TRUE, 0, 0, "");
									CONSOLE_DESC("join a team.");
	// -----------------------------
	CONSOLE_CATEGORY("general");
	CONSOLE_CMD("script",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdScript,						TRUE, 0);
									CONSOLE_DESC("run console command script.");
	// -----------------------------
#if !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("processwheninactive",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdProcessWhenInactive,			TRUE, 0);
									CONSOLE_DESC("toggle game processing while application is inactive.");
#endif  // end !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("reset",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdResetGame,					TRUE, 0);
									CONSOLE_DESC("reset the game.");		

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("debug");
	CONSOLE_CMD("viewc",					TRUE,		COMMAND_SERVER,	COMMAND_SERVER,		s_sCmdViewC,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");	
									CONSOLE_DESC("set focus unit for server debug.");		
	CONSOLE_CMD("nocheats",					TRUE,		COMMAND_BOTH,	COMMAND_BOTH,		x_sCmdNoCheats,						TRUE, 0);
									CONSOLE_DESC("disable cheats.");
	CONSOLE_CMD("silentassert",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		x_sCmdToggleSilentAssert,			TRUE, 0);
									CONSOLE_DESC("toggle assert silently.");		

	CONSOLE_CMD("statstrace",				TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdStatsTrace,					FALSE, 0);
									CONSOLE_DESC("stats trace.");		
	CONSOLE_CMD("modedebug",				TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdModeDebug,					TRUE, 0);
									CONSOLE_DESC("toggle unit mode debug display.");		
	CONSOLE_CMD("modedump",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdModeDump,						TRUE, 0);
									CONSOLE_DESC("display all active modes on the debug unit.");		
#ifdef DEBUG_PICKUP
	CONSOLE_CMD("pickupdebug",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPickupDebug,					TRUE, 0);
									CONSOLE_DESC("toggle draw nearby items.");
#endif  // end DEBUG_PICKUP
	CONSOLE_CMD("drb_reserve_nodes",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdReserveNodes,					TRUE, 0);
									CONSOLE_DESC("drb's reserve nodes command.");		
	CONSOLE_CMD("drb_st_dump",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSTDump,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set drb's super tracker level.");		
	CONSOLE_CMD("camera_debug_unit",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdCameraDebugUnit,				TRUE, 0);
									CONSOLE_DESC("set camera to selected debug unit.");		
	CONSOLE_CMD("units",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdUnitsDebug,					TRUE, 0);		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "client,server", "client");
									CONSOLE_DESC("trace all units to console.");		
	CONSOLE_CMD("unitsverbose",				TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdUnitsVerbose,					TRUE, 0);		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "client,server", "client");
									CONSOLE_DESC("trace all units to console (verbose version).");		
	CONSOLE_CMD("checkstringbykey",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdCheckString,					TRUE, 0);		CONSOLE_PAR(PARAM_STRING, FALSE, 0, 0, "");
									CONSOLE_DESC("Find the entry for a string key in the string tables");		
	CONSOLE_CMD("tagdisplay",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdTagDisplay,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_TAG, 0, "");
									CONSOLE_DESC("Toggle unit tag debug display.");
	CONSOLE_CMD("unitmetrics",				TRUE,		COMMAND_SERVER,	COMMAND_SERVER,		s_sCmdUnitMetricsToggle,			TRUE, 0);		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "all,hce,pvp", "all");

#endif // ISVERSION(DEVELOPMENT)
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
	CONSOLE_CMD("info",						CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGameInfo,						TRUE, 0);
									CONSOLE_DESC("display general information about the game.");
	CONSOLE_CMD("background_room",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdBackgroundRoom,				TRUE, 0);	 /*CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_LEVEL_AREAS, 0, "");*/
									CONSOLE_DESC("generates a specific room to view.");
	//CONSOLE_CMD("background_prop",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdBackgroundProp,				TRUE, 0);	 /*CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_LEVEL_AREAS, 0, "");*/
	//								CONSOLE_DESC("generates a specific prop set to view.");
	//CONSOLE_CMD("version",						NOT_CHEATS,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdVersionInfo,						TRUE, 0);
	//								CONSOLE_DESC("display version string in console.");
									

#endif
#if(ISVERSION(DEVELOPMENT))


// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("override");
	CONSOLE_CMD("playercount",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerCount,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");	
									CONSOLE_DESC("set player count override for difficulty scaling.");
	CONSOLE_CMD("affixfocus",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdAffixFocus,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_AFFIXES, 0, "");
									CONSOLE_DESC("set default monster affix.");
#if ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("moneychance",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMoneyChance,					TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("set global money drop chance override.");
#endif  // ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("sfxchanceoverride",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSfxChanceOverride,			TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set override for elemental sfx chance.");		

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("testcode");
	CONSOLE_CMD("gsomberg",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdTestGSomberg,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("gsomberg test.");
	CONSOLE_CMD("cdayserver",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdCDayServer,					TRUE, 0);
									CONSOLE_DESC("cday test.");
	CONSOLE_CMD("cdayclient",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		x_sCmdCDayClient,					TRUE, 0);
									CONSOLE_DESC("test");
	CONSOLE_CMD("gagtoggle",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdGagToggle,					TRUE, 0);
									CONSOLE_DESC("toggle gagged status");
	CONSOLE_CMD("gagstatus",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdGagStatus,					TRUE, 0);
									CONSOLE_DESC("view gagged status");
	CONSOLE_CMD("recipesrefresh",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdRecipesRefresh,				TRUE, 0);
									CONSOLE_DESC("cday test.");
	CONSOLE_CMD("difficulty_max_cycle",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdDifficultyMaxCycle,			TRUE, 0);
									CONSOLE_DESC("test.");
	CONSOLE_CMD("statxferlog",		TRUE,				COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdStatXferLog,					TRUE, 0);
									CONSOLE_DESC("test.");
	CONSOLE_CMD("transporttruth",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdTransportTruth,				TRUE, 0);
									CONSOLE_DESC("test");
	CONSOLE_CMD("lostitemdebug",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdLostItemDebug,				TRUE, 0);
									CONSOLE_DESC("test");
	CONSOLE_CMD("savevalidate",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdSaveValidate,					TRUE, 0);
									CONSOLE_DESC("cday test.");
	CONSOLE_CMD("incrementalsavelog",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdIncrementalSaveLog,			TRUE, 0);
									CONSOLE_DESC("cday test.");
	CONSOLE_CMD("loadingupdatenow",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		x_sCmdLoadingUpdateNow,				TRUE, 0);
									CONSOLE_DESC("test");
	CONSOLE_CMD("safe",						TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdSafe,					TRUE, 0);
									CONSOLE_DESC("cday test.");
	CONSOLE_CMD("fillpaklocalizedtextures",	TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdFillpakLocalizedTextures,		TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SKU, 0, "");
									CONSOLE_DESC("cday test.");
	CONSOLE_CMD("townportalallowstate",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdTownPortalAllowState,			TRUE, 0);
									CONSOLE_DESC("town portal restrict test.");
	CONSOLE_CMD("itemupgrade",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		x_sCmdItemUpgrade,					TRUE, 0);
									CONSOLE_DESC("cday test.");
	CONSOLE_CMD("treasurestats",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdTreasureStats,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_TREASURE,	0, "");
									CONSOLE_DESC("do stats for specified treasure class.");									
	CONSOLE_CMD("createnospawnzone",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdCreateNoSpawnZone,			TRUE, 0);
									CONSOLE_DESC("cday test.");
	CONSOLE_CMD("offergivereplace",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdOfferGiveReplace,				FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_OFFER, 0, "");
									CONSOLE_DESC("do an offering");
	CONSOLE_CMD("offerdisplay",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdOfferDisplay,					FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_OFFER, 0, "");
									CONSOLE_DESC("do an offering");
	CONSOLE_CMD("offergive",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,				s_sCmdOfferGive,					FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_OFFER, 0, "");
									CONSOLE_DESC("do an offering");
	CONSOLE_CMD("merchantsrefresh",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		x_sCmdMerchantsRefresh,				TRUE, 0);
									CONSOLE_DESC("refresh merchant.");
	CONSOLE_CMD("affixall",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdAffixAll,						TRUE, 0);
									CONSOLE_DESC("item affix test.");
	CONSOLE_CMD("weaponstest",				TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdWeaponsTest,					FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_ITEMS,	0, "");		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "all", "");		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "affixes", "");		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "mods", "");
									CONSOLE_DESC("run weapons test.");		
									CONSOLE_USE("weaponstest [weapon name] [, all] [, affixes] [, mods]");
	CONSOLE_CMD("weaponstestdrone",			TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdWeaponsTestDrone,				FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_ITEMS,	0, "");		// TODO cmarch ? CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "all", "");		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "affixes", "");		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "mods", "");
									CONSOLE_DESC("run drone weapons test.");		
									CONSOLE_USE("weaponstestdrone [weapon name] [, stop]");// TODO cmarch ? [, all] [, affixes] [, mods]");
	CONSOLE_CMD("weaponsnext",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWeaponNext,					TRUE, 0);		
									CONSOLE_DESC("advance to next weapon for weaponstest.");		
	CONSOLE_CMD("armortest",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdArmorTest,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_PLAYERS, 0, "");
									CONSOLE_DESC("run armor test.");		
	CONSOLE_CMD("armorteststop",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdArmorTestStop,				TRUE, 0);		
									CONSOLE_DESC("stop armor test.");		
	CONSOLE_CMD("armorrandom",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdArmorRandom,					TRUE, 0);		
									CONSOLE_DESC("randomly spawn and equip some armor.");		
	CONSOLE_CMD("trade",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTrade,						TRUE, 0);
									CONSOLE_DESC("initiate trade with first other player in game.");
	CONSOLE_CMD("tradetest",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,	s_sCmdTradeTest,					TRUE, 0);
									CONSOLE_DESC("test the trade interface with a dummy trade");
	CONSOLE_CMD("tradecancel",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTradeCancel,					TRUE, 0);
									CONSOLE_DESC("initiate trade with first other player in game.");
	CONSOLE_CMD("npc_video_debug",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdDebugNPCVideo,				TRUE, 0);		
									CONSOLE_DESC("test the NPC video dialog");
	CONSOLE_CMD("dialog",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdDialog,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_DIALOG, 0, "");
									CONSOLE_DESC("randomly spawn a monster from a monster spawn class.");
	CONSOLE_CMD("tracker_add",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,			s_cCmdDebugTracker,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0, 0, "0");  CONSOLE_PAR(PARAM_FLOAT, FALSE, 0, 0, "0");  CONSOLE_PAR(PARAM_FLOAT, FALSE, 0, 0, "0");  CONSOLE_PAR(PARAM_FLOAT, FALSE, 0, 0, "0");
									CONSOLE_DESC("add an item to test tracking UI");
	CONSOLE_CMD("tracker_remove",		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,			s_cCmdDebugTrackerRemove,			TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0, 0, "0");  
									CONSOLE_DESC("remove an item from test tracking UI");
	CONSOLE_CMD("tracker_close",			FALSE,	COMMAND_CLIENT,	COMMAND_CLIENT,			s_cCmdDebugTrackerClose,			TRUE, 0);		
									CONSOLE_DESC("close test tracking UI");
	CONSOLE_CMD("refresh_buddy_list",		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdRefreshBuddyList,				TRUE, 0);
									CONSOLE_DESC("request a refresh of all online buddy info");
	CONSOLE_CMD("refresh_guild_list",		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdRefreshGuildList,				TRUE, 0);
									CONSOLE_DESC("request a refresh of all online guild member info");

// -----------------------------
// -----------------------------
#if ISVERSION(CHEATS_ENABLED)
CONSOLE_CATEGORY("cheats");
	CONSOLE_CMD("speed",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdSpeed,						TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("change player speed.");
	CONSOLE_CMD("leveldump",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdLevelDump,					TRUE, 0);
									CONSOLE_DESC("outputs all of the levels in the dungeon.");
	CONSOLE_CMD("freelevel",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdFreeLevel,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_LEVEL, 0, "");
									CONSOLE_DESC("frees the level in the dungeon.");
	CONSOLE_CMD("drlgspossibleversion",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdDRLGsPossibleVersion,			TRUE, 0);
									CONSOLE_DESC("Gather all used DRLGs for the current version");
	CONSOLE_CMD("maxbreath",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdBreath,						TRUE, 0);
									CONSOLE_DESC("toggle infinite breath.");		
	CONSOLE_CMD("maxpower",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPower,						TRUE, 0);
									CONSOLE_DESC("toggle infinite power.");		

	CONSOLE_CMD("invulnerable",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdInvulnerable,					TRUE, 0);
									CONSOLE_DESC("toggle player invunlerability.");
	CONSOLE_CMD("invulnerableall",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdInvulnerableAll,				TRUE, 0);
									CONSOLE_DESC("toggle global invunlerability.");
	CONSOLE_CMD("invulnerableunit",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdInvulnerableUnit,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("toggle invunlerability for a specific unit.");
	CONSOLE_CMD("alwayssofthit",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdAlwaysSoftHit,				TRUE, 0);
									CONSOLE_DESC("toggle always cause soft hits cheat.");
	CONSOLE_CMD("alwaysgethit",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdAlwaysGetHit,					TRUE, 0);
									CONSOLE_DESC("toggle always cause get hits cheat.");

	CONSOLE_CMD("killunit",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdKillUnit,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("kill specific unit.");
	CONSOLE_CMD("killme",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSuicide,						TRUE, 0);
									CONSOLE_DESC("kill self.");
	CONSOLE_CMD("suicide",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSuicide,						TRUE, 0);
									CONSOLE_DESC("kill self.");
	CONSOLE_CMD("freeunit",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdFreeUnit,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("free a specific unit.");
	CONSOLE_CMD("equipall",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdCanEquipAllItems,				TRUE, 0);		CONSOLE_PAR(PARAM_BOOLEAN, FALSE, 0, 0, "");
									CONSOLE_DESC("toggle all items are equippable mode.");
	CONSOLE_CMD("desynchunit",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdDesynchUnit,					TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("desynch a specific unit.");

	CONSOLE_CMD("minigamereset",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMinigameReset,				TRUE, 0);
									CONSOLE_DESC("reset the minigame.");
	CONSOLE_CMD("jumpvelocity",				TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdJumpVelocity,					TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("set your jump velocity.");
	CONSOLE_CMD("gravity",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdGravity,						TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("set global gravity.");		
	CONSOLE_CMD("warpsopen",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWarpsOpen,					TRUE, 0);
									CONSOLE_DESC("open all portals for yourself.");		
	CONSOLE_CMD("warpcycle",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,	s_sCmdWarpCycle,					TRUE, 0);
									CONSOLE_DESC("open all portals for yourself.");		
	CONSOLE_CMD("stringverify",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdStringVerify,					TRUE, 0);
									CONSOLE_DESC("Verify string table.");		
	CONSOLE_CMD("sku_validate",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdSKUValidate,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SKU, 0, "");
									CONSOLE_DESC("randomly spawn a monster from a monster spawn class.");
	CONSOLE_CMD("waypointsall",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWaypointsAll,					TRUE, 0);
									CONSOLE_DESC("give yourself all waypoints.");		
	CONSOLE_CMD("hellriftportal",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdHellriftPortal,				TRUE, 0);
									CONSOLE_DESC("warp to hellrift if there is one.");		

	CONSOLE_CMD("automapmonsters",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdAutomapMonsters,				TRUE, 0);
									CONSOLE_DESC("toggle showing monsters on the minimap.");
	CONSOLE_CMD("balancetest",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdBalanceTest,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("perform a balance test by spawning several monsters and timing you ability to kill them.");
	CONSOLE_CMD("resurrect",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdResurrect,					TRUE, 0);
									CONSOLE_DESC("resurrect");		
	CONSOLE_CMD("headstonedebug",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdHeadstoneDebug,				TRUE, 0);
									CONSOLE_DESC("toggle headstone debug information");		
	CONSOLE_CMD("playmovie",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		c_sCmdPlayMovie,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_MOVIELISTS, 0, "");
									CONSOLE_DESC("play a movie list");		
	CONSOLE_CMD("movielq",					TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdLQMovies,						TRUE, 0);
									CONSOLE_DESC("toggle low-quality movies.");
#endif // ISVERSION(CHEATS_ENABLED)

#endif // ISVERSION(DEVELOPMENT)
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
	CONSOLE_CMD("god",						CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGod,							TRUE, 0);
									CONSOLE_DESC("toggle god mode (invulnerability, infinite power, infinite breath).");
	CONSOLE_CMD("level",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdLevel,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_LEVEL, 0, "");
									CONSOLE_DESC("teleport to a different level.");
	CONSOLE_CMD("levelr",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdLevel,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE_BYNAME, TRUE, DATATABLE_LEVEL, 1, "");
									CONSOLE_DESC("teleport to a different level.");
	CONSOLE_CMD("levelup",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdLevelUp,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("level up cheat.");
	CONSOLE_CMD("rankup",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdRankUp,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("rank up cheat.");
	CONSOLE_CMD("hardcore",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdHardcore,						TRUE, 0);
									CONSOLE_DESC("make the character hardcore.");
	CONSOLE_CMD("league",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdLeague,						TRUE, 0);
									CONSOLE_DESC("make the character league mode.");
	CONSOLE_CMD("elite",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdElite,						TRUE, 0);
									CONSOLE_DESC("make the character elite.");
	CONSOLE_CMD("warp",						CHEATS_RELEASE_CSR,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWarp,							TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("warp to location.");		

	CONSOLE_CMD("warpToNextMarker",		CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWarpToMarker,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_LEVEL_AREAS, 0, "");
									CONSOLE_DESC("teleport to the next marker from your choosen marker.");

	CONSOLE_CMD("warpToLevelArea",		CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWarpToLevelArea,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_LEVEL_AREAS, 0, "");
									CONSOLE_DESC("teleport to a different level.");

	CONSOLE_CMD("warpToPrevFloor",	CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWarpPrevFloor,				TRUE, 0);
									CONSOLE_DESC("teleport to next floor.");

	CONSOLE_CMD("warpToNextFloor",	CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWarpNextFloor,				TRUE, 0);
									CONSOLE_DESC("teleport to next floor.");

	CONSOLE_CMD("warpToLastFloor",	CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWarpLastFloor,				TRUE, 0);
									CONSOLE_DESC("teleport to next floor.");


	CONSOLE_CMD("killall",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdKillAll,						TRUE, 0);
									CONSOLE_DESC("kill all monsters.");
	CONSOLE_CMD("alwayskill",				CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdAlwaysKill,					TRUE, 0);
									CONSOLE_DESC("toggle always kill on first hit mode.");
								

#endif //#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)
#if ISVERSION(DEVELOPMENT)
	CONSOLE_CMD("badges", TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdBadgesList,					TRUE, 0);
									CONSOLE_DESC("list out player badges.");
#endif


#if(ISVERSION(DEVELOPMENT))
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("combat");
	CONSOLE_CMD("combatlog",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdCombatLog,					TRUE, 0);		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "dmg,sfx,kills,evasion,blocking,shields,armor,interrupt,dbgunit", "");
									CONSOLE_DESC("toggle combat log.");
	CONSOLE_CMD("proctrace",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdProcTrace,					TRUE, 0);
									CONSOLE_DESC("toggle proc trace.");
#if !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("dps",						TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdToggleDPS,					TRUE, 0);
									CONSOLE_DESC("toggle damage per second display.");		
	CONSOLE_CMD("dps_resettimer",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdDPSResetTimer,				TRUE, 0);
									CONSOLE_DESC("reset dps timer.");		
#if ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("critprobability",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdCritProbability,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set global item critical probability override.");
#endif  // ISVERSION(CHEATS_ENABLED)

#endif  // !ISVERSION(SERVER_VERSION)

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("rooms");
	CONSOLE_CMD("roomlist",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdRoomList,						TRUE, 0);
									CONSOLE_DESC("list all rooms on level.");
	CONSOLE_CMD("visitdlevelsall",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdVisitedLevelsAll,				TRUE, 0);
									CONSOLE_DESC("set all levels to visited for yourself.");		
	CONSOLE_CMD("visitdlevelsclear",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdVisitedLevelsClear,			TRUE, 0);
									CONSOLE_DESC("clear all visited levels for yourself.");		

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("player");
	CONSOLE_CMD("playernew",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNew,					FALSE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "guardian,blademaster,exemplar,recon,engineer,marksman,metamorph,evoker,summoner", "");		CONSOLE_PAR_NV(PARAM_STRINGLIST, FALSE, "", "male,female", "");
									CONSOLE_DESC("new character.");		
									CONSOLE_USE("playernew [name [, (guardian | blademaster | exemplar | recon | engineer | marksman | metamorph | evoker | summoner)] [, (male | female)]]");
	CONSOLE_CMD("guardian_male",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as guardian male.");		
	CONSOLE_CMD("guardian_female",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as guardian female.");		
	CONSOLE_CMD("blademaster_male",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as blademaster male.");		
	CONSOLE_CMD("blademaster_female",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as blademaster female.");		
	CONSOLE_CMD("exemplar_male",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as exemplar male.");		
	CONSOLE_CMD("exemplar_female",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as exemplar female.");		
	CONSOLE_CMD("recon_male",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as recon male.");		
	CONSOLE_CMD("recon_female",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as recon female.");		
	CONSOLE_CMD("engineer_male",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as engineer male.");		
	CONSOLE_CMD("engineer_female",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as engineer female.");		
	CONSOLE_CMD("marksman_male",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as marksman male.");		
	CONSOLE_CMD("marksman_female",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as marksman female.");		
	CONSOLE_CMD("metamorph_male",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as metamorph male.");		
	CONSOLE_CMD("metamorph_female",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as metamorph female.");		
	CONSOLE_CMD("evoker_male",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as evoker male.");		
	CONSOLE_CMD("evoker_female",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as evoker female.");		
	CONSOLE_CMD("summoner_male",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as summoner male.");		
	CONSOLE_CMD("summoner_female",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerNewSpecific,			TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("reset character as summoner female.");		
	CONSOLE_CMD("playersave",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerSave,					TRUE, 0);
									CONSOLE_DESC("force player save.");		
	CONSOLE_CMD("playerload",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPlayerLoad,					TRUE, 0);		CONSOLE_PAR(PARAM_PLAYERNAME, FALSE, 0,	0, "");		
									CONSOLE_DESC("force player load.");		

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("skills");
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)

	CONSOLE_CMD("strength",					TRUE, 	COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGiveStrengthPoints,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("gives strength points.");

	CONSOLE_CMD("dexterity",				TRUE, 	COMMAND_CLIENT,	COMMAND_SERVER,	s_sCmdGiveDexterityPoints,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("gives dexterity points.");

	CONSOLE_CMD("vitality",					TRUE, 	COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGiveVitalityPoints,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("gives vitality points.");

	CONSOLE_CMD("wisdom",					TRUE, 	COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGiveWisdomPoints,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("gives wisdom points.");

	CONSOLE_CMD("skillpoints",				TRUE, 	COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGiveSkillPoints,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("gives skill points.");

	CONSOLE_CMD("particlefps",				CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSetParticleFPS,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("Set to -1 to restore particle system.");

									

	CONSOLE_CMD("skillrespec",				TRUE, 	COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSkillRespec,				TRUE, 0);
									CONSOLE_DESC("Respec of skills.");

	CONSOLE_CMD("craftingpoints",			TRUE, 	COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGiveCraftingPoints,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("gives crafting points.");


#endif
	CONSOLE_CMD("skillzerolevel",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdZeroSkillLevel,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SKILLS,	0, "");
									CONSOLE_DESC("zero's out the skill level.");

	CONSOLE_CMD("skillsall",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSkillsAll,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("give yourself all skills.");
	CONSOLE_CMD("skill",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSkill,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SKILLS,	0, "");		CONSOLE_PAR_NV(PARAM_INTEGER, FALSE, "level", "", "");
									CONSOLE_DESC("give yourself a point in a specific skill.");
	CONSOLE_CMD("skillsunlock",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSkillsUnlock,					TRUE, 0);
									CONSOLE_DESC("give yourself a point in a specific skill.");
#if ISVERSION(CHEATS_ENABLED)

	CONSOLE_CMD("skillcooldown",			TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		s_sCmdSkillCooldown,				TRUE, 0);
									CONSOLE_DESC("toggle skill cooldowns.");
	CONSOLE_CMD("skillstart",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdSkillStartRequest,			TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SKILLS,	0, "");
									CONSOLE_DESC("request a skill to be started");
#endif  // ISVERSION(CHEATS_ENABLED)
#endif // ISVERSION(DEVELOPMENT)

#if ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)
	CONSOLE_CMD("state",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdState,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_STATES,	0, "");		CONSOLE_PAR_NA("time", PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("set a state on yourself.");		
	CONSOLE_CMD("clearstate",				CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdClearState,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_STATES,	0, "");									
									CONSOLE_DESC("clear a state from yourself.");		
#endif
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) 
	CONSOLE_CMD("skilllevelcheat",			CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSkillLevelCheat,		 		TRUE, 0);		
									CONSOLE_DESC("lets you break all of the rules for skill leveling.");
#endif

#if(ISVERSION(DEVELOPMENT))

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("items");
	CONSOLE_CMD("moneypiles",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMoneyPiles,					TRUE, 0);
									CONSOLE_DESC("generate piles of money.");
	CONSOLE_CMD("powerupfield",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPowerupField,					TRUE, 0);
									CONSOLE_DESC("create a bunch of health and power power-ups.");
	CONSOLE_CMD("identifyall",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdIdentifyAll,					FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_ITEMS, 0, "");
									CONSOLE_DESC("create item(s).");
	CONSOLE_CMD("itemr",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdItem,							FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE_BYNAME, TRUE, DATATABLE_ITEMS, 1, "");
									CONSOLE_DESC("create item(s) by display name.");
	CONSOLE_CMD("itemtype",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdItem,							FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_UNITTYPES, 0, "");
									CONSOLE_DESC("create items by unit type.");
	CONSOLE_CMD("itemsall",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdItemsAll,						FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, FALSE, DATATABLE_UNITTYPES, 0, "");
									CONSOLE_DESC("create all items matching criteria.");
	CONSOLE_CMD("saveallitems",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSaveAllItems,					FALSE, 0);
									CONSOLE_DESC("create all items and test to make sure that they can be saved.");
	CONSOLE_CMD("itemprocchance",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdItemProcChance,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set global item proc spqawn chance override.");
#if ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("procprobability",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdProcProbability,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set global item proc probability override.");
	CONSOLE_CMD("sitemlevel",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGlobalItemLevel,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set global item level override - all items will be spawned at this balance level.");
#endif  // ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("itemlevel",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdItemLevel,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("set item level override - all items will be spawned at this balance level.");		
	CONSOLE_CMD("procongothit",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdProcOnGotHit,					TRUE, 0);
									CONSOLE_DESC("trigger a got hit event on self.");
	CONSOLE_CMD("procongotkilled",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdProcOnGotKilled,				TRUE, 0);
									CONSOLE_DESC("trigger a got killed event on self.");
	CONSOLE_CMD("treasure",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTreasure,						TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_TREASURE,	0, "");		CONSOLE_PAR_UN(PARAM_INTEGER, TRUE, 0, 0, "");		CONSOLE_PAR_NV(PARAM_STRINGLIST, TRUE, "", "r", "");
									CONSOLE_DESC("spawn a treasure class.");
									CONSOLE_USE("treasure [, reportonly] [, #count]");
	CONSOLE_CMD("treasureall",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTreasureAll,					TRUE, 0);
									CONSOLE_DESC("simulate all treasure classes, with report to log.");
#if !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("treasurelog",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdTreasureLog,					TRUE, 0);
									CONSOLE_DESC("toggle treasure logging.");
#endif  // !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("treasuretoggle",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdToggleTreasure,				TRUE, 0);
									CONSOLE_DESC("toggle treasure dropping.");		
	CONSOLE_CMD("firebolter",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdFirebolter,					TRUE, 0);		
									CONSOLE_DESC("quickspawn a firebolter.");		
	CONSOLE_CMD("firebolterzombie",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdFirebolterZombie,				TRUE, 0);		
									CONSOLE_DESC("quickspawn a firebolter and a zombie.");		
#endif // ISVERSION(DEVELOPMENT)
#if(ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED)) || _PROFILE
	CONSOLE_CMD("money",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMoney,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("give yourself money.");
									CONSOLE_USE("money [+|-]<amount>");
#endif
#if(ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))

	CONSOLE_CMD("credit",					CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdCredits,						TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0, 0, "");
									CONSOLE_DESC("give yourself credits.");
									CONSOLE_USE("money [+|-]<amount>");

	CONSOLE_CMD("item",						CHEATS_RELEASE_CSR|CHEATS_RELEASE_TESTER ,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdItem,							FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_ITEMS, 0, "");
									CONSOLE_DESC("create item(s).");
#endif


									
// -----------------------------
// -----------------------------
#if(ISVERSION(DEVELOPMENT) || ISVERSION(RELEASE_CHEATS_ENABLED))
	CONSOLE_CMD("recipelearn",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,	s_sCmdRecipeLearn, TRUE, 0); 	CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_RECIPES, 0, "");
									CONSOLE_DESC("Learn a given recipe in a crafting all the achievements");
	CONSOLE_CMD("recipelearnall",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdRecipeLearnAll,		TRUE, 0);
									CONSOLE_DESC("Learns all recipes");
#endif
#if ISVERSION(DEVELOPMENT)
CONSOLE_CMD("recipeTestAll",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdRecipeTestAll,		TRUE, 0);
									CONSOLE_DESC("Allows all testing of recipes");
#endif

#if ISVERSION(DEVELOPMENT)
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("achievements");
	CONSOLE_CMD("achievementsreveal",			TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,			x_sCmdAchievementsRevealAll,TRUE, 0);
									CONSOLE_DESC("Shows all the achievements");
	CONSOLE_CMD("achievementclear",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,			s_sCmdAchievementClear,		TRUE, 0);	CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_ACHIEVEMENTS, 0, "");
									CONSOLE_DESC("Sets an achievment to the start state");
	CONSOLE_CMD("achievementprogress",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdAchievementProgress,	TRUE, 0);	CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_ACHIEVEMENTS, 0, "");	CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("Advances an achievement by one");		
	CONSOLE_CMD("achievementfinish",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,				s_sCmdAchievementFinish,	TRUE, 0);	CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_ACHIEVEMENTS, 0, "");
									CONSOLE_DESC("Sets an achievement to the finished state");		
	CONSOLE_CMD("achievementfinishall",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,				s_sCmdAchievementFinishAll,	TRUE, 0);	
									CONSOLE_DESC("Sets all achievements to the finished state");		



// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("inventory");
	CONSOLE_CMD("gridclear",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGridClear,					TRUE, 0);
									CONSOLE_DESC("deletes items in your inventory.");
	CONSOLE_CMD("clearweapons",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdClearWeapons,					TRUE, 0);
									CONSOLE_DESC("delete all weapons in your inventory.");		
	CONSOLE_CMD("weaponconfigsclear",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdWeaponConfigsClear,			TRUE, 0);
									CONSOLE_DESC("clear weapon configs.");		
	CONSOLE_CMD("weaponconfigsdisp",		TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdWeaponConfigsDisp,			TRUE, 0);
									CONSOLE_DESC("trace weapon configs.");		

// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("monsters");
	CONSOLE_CMD("monsterzoo",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonsterZoo,					TRUE, 0);		
									CONSOLE_DESC("spawn a slew of monsters.");
	CONSOLE_CMD("monstersall",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonstersAll,					TRUE, 0);
									CONSOLE_DESC("spawn every monster.");
	CONSOLE_CMD("monsterspossible",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonstersPossible,				TRUE, 0);
									CONSOLE_DESC("spawn all spawnable monsters for the current level.");
	CONSOLE_CMD("monsterspossibleversion",	TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonstersPossibleVersion,		TRUE, 0);
									CONSOLE_DESC("what monsters are available in the current version");
	CONSOLE_CMD("monsterspawnstatscurrent",	TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonsterSpawnStatsCurrent,		TRUE, 0);
									CONSOLE_DESC("spawn all spawnable monsters for the current level.");
	CONSOLE_CMD("monsterspawnstatsall",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonsterSpawnStatsAll,			TRUE, 0);
									CONSOLE_DESC("spawn all spawnable monsters for the current level.");
#if ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("smonsterlevel",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdGlobalMonsterLevel,			TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, TRUE, 0,	0, "");
									CONSOLE_DESC("set global monster level override (used for picking monsters to spawn).");
#endif  // ISVERSION(CHEATS_ENABLED)
	CONSOLE_CMD("monsterlevel",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonsterLevel,					TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("set monster level override.");		
	CONSOLE_CMD("conversationupdaterate",	TRUE,	COMMAND_CLIENT,	COMMAND_CLIENT,			s_sCmdConversationUpdateRate,		TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("set monster level override.");		
	CONSOLE_CMD("conversationspeed",TRUE,	COMMAND_CLIENT,	COMMAND_CLIENT,					s_sCmdConversationSpeed,			TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("set monster level override.");		
	CONSOLE_CMD("zombie",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdZombie,						TRUE, 0);		
									CONSOLE_DESC("quick-spawn a zombie.");		
	CONSOLE_CMD("spawnclass",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSpawnClass,					TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SPAWN_CLASS, 0, "");
									CONSOLE_DESC("randomly spawn a monster from a monster spawn class.");
	CONSOLE_CMD("spawnmemoryreset",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSpawnMemoryReset,				TRUE, 0);
									CONSOLE_DESC("clear all monster spawn memory for every level.");
	CONSOLE_CMD("spawnclasstest",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSpawnClassTest,				TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_SPAWN_CLASS, 0, "");
									CONSOLE_DESC("randomly spawn a monster from a monster spawn class.");
	CONSOLE_CMD("championprobability",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdChampionProbability,			TRUE, 0);		CONSOLE_PAR(PARAM_FLOAT, TRUE, 0,	0, "");
									CONSOLE_DESC("set global champion probability override.");
	CONSOLE_CMD("championspawntest",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdChampionSpawnTest,			TRUE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_MONSTERS,	0, "");
									CONSOLE_DESC("run champion spawn test.");
	CONSOLE_CMD("petsdamage",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPetsDamage,					TRUE, 0);
									CONSOLE_DESC("set all pets hit points to 1.");		
	CONSOLE_CMD("petskill",					TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdPetsKill,						TRUE, 0);
									CONSOLE_DESC("kill all your pets.");		
	CONSOLE_CMD("stoppathing",				TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		s_sCmdStopPathing,					TRUE, 0);
									CONSOLE_DESC("stop the debug unit from pathing.");		
	CONSOLE_CMD("monsternext",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonsterNext,					TRUE, 0);
									CONSOLE_DESC("spawn a monster.");
	CONSOLE_CMD("monsterupgrade",			TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonsterUpgrade,				TRUE, 0);		CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("spawn a monster.");
#endif // ISVERSION(DEVELOPMENT)
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED)) || ISVERSION(RELEASE_CHEATS_ENABLED)

	CONSOLE_CMD("monster",					CHEATS_RELEASE_CSR|CHEATS_RELEASE_TESTER,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdMonster,						FALSE, 0);		CONSOLE_PAR(PARAM_DATATABLE, TRUE, DATATABLE_MONSTERS, 0, "");
									CONSOLE_DESC("spawn a monster.");
#endif
#if(ISVERSION(DEVELOPMENT))

	
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("ai");
	CONSOLE_CMD("aifreeze",					TRUE,		COMMAND_CLIENT,	COMMAND_BOTH,		x_sCmdAIFreeze,						TRUE, 0);
									CONSOLE_DESC("toggle ai freeze.");
	CONSOLE_CMD("ainotarget",				TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdAINoTarget,					TRUE, 0);
									CONSOLE_DESC("toggle ai targeting.");
	// -----------------------------
	CONSOLE_CATEGORY("multiplayer");
	CONSOLE_CMD("disconnect_player",		TRUE,		COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdDisconnectPlayer,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
									CONSOLE_DESC("disconnect a player by name.");
#endif
	CONSOLE_CMD("switchgameserver",	CHEATS_RELEASE_CSR|CHEATS_RELEASE_TESTER,	COMMAND_CLIENT,	COMMAND_SERVER,		s_sCmdSwitchGameServer,				FALSE, 0);	CONSOLE_PAR(PARAM_INTEGER, FALSE, 0,	0, "");
									CONSOLE_DESC("switch client to other gameserver.  THIS IS NOT SAFE, and may cause save bugs!");	
#if(ISVERSION(DEVELOPMENT))
// -----------------------------
// -----------------------------
CONSOLE_CATEGORY("voicechat");
	CONSOLE_CMD("voicechatinit",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatInit,				TRUE, 0);
									CONSOLE_DESC("initialize the voice chat system.");
	CONSOLE_CMD("voicechatclose",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatClose,				TRUE, 0);
									CONSOLE_DESC("shut down the voice chat system.");
	CONSOLE_CMD("voicechatupdate",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatUpdate,				TRUE, 0);
									CONSOLE_DESC("update the voice chat system.");
	CONSOLE_CMD("voicechatcreateroom",		FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatCreateRoom,			TRUE, 0);
									CONSOLE_DESC("create a voice chat room.");
	CONSOLE_CMD("voicechatlogin",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatLogin,				TRUE, 0);		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");		CONSOLE_PAR(PARAM_STRING, TRUE, 0,	0, "");
									CONSOLE_DESC("log in to the voice chat system.");
	CONSOLE_CMD("voicechatlogout",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatLogout,				TRUE, 0);
									CONSOLE_DESC("log out of the voice chat system.");
	CONSOLE_CMD("voicechatstarthost",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatStartHost,				TRUE, 0);
									CONSOLE_DESC("start hosting the voice chat.");
	CONSOLE_CMD("voicechatjoinhost",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatJoinHost,				TRUE, 0);
									CONSOLE_DESC("join the voice chat host.");
	CONSOLE_CMD("voicechatjoinroom",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatJoinRoom,				TRUE, 0);
									CONSOLE_DESC("join the voice chat room.");
	CONSOLE_CMD("voicechatleaveroom",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_cCmdVoiceChatLeaveRoom,				TRUE, 0);
									CONSOLE_DESC("leave the voice chat room.");

// -----------------------------
// -----------------------------
	// CHB 2006.12.13
#if !ISVERSION(SERVER_VERSION)
	CONSOLE_CMD("editcompatdb",				TRUE,		COMMAND_CLIENT,	COMMAND_CLIENT,		s_sCmdEditCompatDB,					TRUE, 0);
									CONSOLE_DESC("edit video hardware compatibility database.");
#endif  // !ISVERSION(SERVER_VERSION)

// -----------------------------
// -----------------------------
#if !ISVERSION(SERVER_VERSION)
CONSOLE_CATEGORY("censor");
	CONSOLE_CMD("censor_nohumans",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,			c_sCmdCensorNoHumans,				TRUE, 0);
									CONSOLE_DESC("remove human likenesses.");
	CONSOLE_CMD("censor_nogore",			FALSE,		COMMAND_CLIENT,	COMMAND_CLIENT,			c_sCmdCensorNoGore,					TRUE, 0);
									CONSOLE_DESC("remove gore.");
#endif  // !ISVERSION(SERVER_VERSION)

/*
xxx
*/
#endif // DEVELOPMENT

	#undef CONSOLE_CMD
	#undef CONSOLE_PAR
	#undef CONSOLE_PAR_NA
	#undef CONSOLE_PAR_UN
	#undef CONSOLE_PAR_NV
	#undef CONSOLE_USE
	#undef CONSOLE_NOTE
	#undef CONSOLE_DESC
	#undef CONSOLE_CATEGORY
	#undef LOCALIZED_CMD
	#undef LOCALIZED_CATEGORY
	#undef LOCALIZED_NOTE
	#undef LOCALIZED_USE
	
	ConsoleAddEmotes();
}

