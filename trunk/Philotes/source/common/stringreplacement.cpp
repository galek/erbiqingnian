//----------------------------------------------------------------------------
// FILE: stringreplacement.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "stringreplacement.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct STRING_REPLACEMENT_LOOKUP
{
	STRING_REPLACEMENT eReplacement;
	const WCHAR *puszTokens;
};
static STRING_REPLACEMENT_LOOKUP sgtStringReplacementTable[] = 
{
	{ SR_QUEST_DUNGEON_NAME,			L"[DungeonName]" },
	{ SR_ITEM,							L"[item]" },
	{ SR_NUMBER,						L"[number]" },
	{ SR_QUEST_TITLE,					L"[questtitle]" },
	{ SR_QUEST_ITEMS_LIST_ACCEPT,		L"[QuestItemsListAccept]" },
	{ SR_QUEST_ITEMS_LIST_COMPLETE,		L"[QuestItemsListComplete]" },
	{ SR_AREA_NAME,						L"[areaname]" },
	{ SR_AREA_LEVEL,					L"[arealevel]" },
	{ SR_GROUP_SIZE,					L"[groupsize]" },	
	{ SR_QUEST_ITEM1,					L"[item1]" },
	{ SR_QUEST_ITEM2,					L"[item2]" },
	{ SR_QUEST_ITEM3,					L"[item3]" },
	{ SR_QUEST_ITEM4,					L"[item4]" },
	{ SR_QUEST_ITEM5,					L"[item5]" },
	{ SR_QUEST_ITEM6,					L"[item6]" },
	{ SR_QUEST_ITEMS_FOUND,				L"[questitemsfound]" },	
	{ SR_QUEST_TIME,					L"[questtime]" },
	{ SR_QUEST_TIME_REMAINING,			L"[questtimeremaining]" },
	{ SR_QUEST_NPC,						L"[questnpc]" },
	{ SR_CURRENT_LEVEL,					L"[currentlevel]" },
	{ SR_ITEM_COLLECT_COUNT,			L"[itemcollectcount]" },
	{ SR_MONSTER_UNIT_TYPE,				L"[MonsterUnitType]" },
	{ SR_QUEST_MONSTER_PROPER_NAME,		L"[QUEST MONSTER PROPER NAME]" },
	{ SR_QUEST_LEVEL,					L"[QUEST LEVEL]" },
	{ SR_QUEST_REWARDER_LEVEL,			L"[QUEST REWARDER LEVEL]" },
	{ SR_CHARACTER_NAME,				L"[CHARACTERNAME],[CHARACTER NAME],[PLAYER NAME],[PLAYERNAME]" },
	{ SR_PLAYER_CLASS,					L"[PLAYER CLASS]" },
	{ SR_QUEST_GIVER,					L"[QUEST GIVER]" },
	{ SR_QUEST_REWARDER,				L"[QUEST REWARDER]" },
	{ SR_NEXT_AREA,						L"[NEXTAREA]" },
	{ SR_QUEST_ESCORT_DEST_LEVEL,		L"[QUEST_ESCORT_DEST_LEVEL]" },
	{ SR_QUEST_COLLECT_OBJS,			L"[QUEST COLLECT OBJS]" },
	{ SR_QUEST_COLLECT_OBTAINED,		L"[QUEST COLLECT OBTAINED]" },
	{ SR_INFESTATION_KILLS,				L"[INFESTATION KILLS]" },
	{ SR_OBJECTIVE_COUNT,				L"[OBJECTIVE COUNT]" },
	{ SR_OBJECTIVE_MONSTER,				L"[OBJECTIVE MONSTER]" },
	{ SR_OBJECTIVE_MONSTERS,			L"[OBJECTIVE MONSTERS]" },
	{ SR_OPERATE_OBJECTS,				L"[OPERATE OBJS],[OBJECTIVE OBJECTS],[OBJECTIVE OBJS]" },
	{ SR_OBJECTIVE_OBJECT_OR_MONSTER,	L"[OBJECTIVE OBJECT OR MONSTER]" },
	{ SR_OPERATED_COUNT,				L"[OPERATED COUNT]" },
	{ SR_STARTING_ITEM,					L"[STARTING ITEM]" },
	{ SR_NAME_IN_LOG_OVERRIDE,			L"[NAME IN LOG OVERRIDE]" },
	{ SR_BOSS,							L"[BOSS]" },
	{ SR_CUSTOM_BOSS,					L"[CUSTOM BOSS]" },
	{ SR_PLAYER,						L"[player]" },
	{ SR_REASON,						L"[reason]" },
	{ SR_PLAYER_NAME,					L"[CHARACTERNAME],[CHARACTER NAME],[PLAYER NAME],[PLAYERNAME]"  },
	{ SR_LEVEL,							L"[level]" },
	{ SR_RANK,							L"[rank]" },
	{ SR_PLAYER_CLASS_NPC,				L"[PLAYER CLASS NPC]" },
	{ SR_STRING1,						L"[string1]" },
	{ SR_STRING2,						L"[string2]" },
	{ SR_STRING3,						L"[string3]" },
	{ SR_STRING4,						L"[string4]" },
	{ SR_STRING5,						L"[string5]" },
	{ SR_TARGETS,						L"[targets]" },
	{ SR_COLLECT,						L"[collect]" },
	{ SR_TIME_LIMIT,					L"[timelimit]" },
	{ SR_TARGET_TYPE,					L"[targettype]" },
	{ SR_TARGET_NUMBER,					L"[targetnumber]" },
	{ SR_PERCENT,						L"[percent]" },
	{ SR_AREA,							L"[area]" },
	{ SR_GOLD,							L"[gold]" },
	{ SR_MAGIC,							L"[magic]" },
	{ SR_MAGIC_SUFFIX,					L"[magicsuffix]" },
	{ SR_SET,							L"[set]" },
	{ SR_QUALITY,						L"[quality]" },
	{ SR_MONSTER,						L"[monster]" },
	{ SR_BASE_ITEM,						L"[baseitem]" },
	{ SR_CLASS,							L"[class]" },
	{ SR_MONEY,							L"[money]" },
	{ SR_LEVEL_DEST,					L"[LEVEL_DEST]" },
	{ SR_OBJECT,						L"[object]" },
	{ SR_DISPLAY_FORMAT_TOKEN,			L"string" },
	{ SR_COPPER,						L"[copper]" },
	{ SR_SILVER,						L"[silver]" },
	{ SR_REGION,						L"[REGION],[region]" },
	{ SR_LANGUAGE,						L"[LANGUAGE],[language]" },
	{ SR_LOCALE_PARAM,					L"parameters" },
	{ SR_ACCOUNT_NAME,					L"[ACCOUNTNAME]" },
	{ SR_PROPERTY,						L"[property]" },
	{ SR_VALUE,							L"[value]" },
	{ SR_HUB_NAME,						L"[hubname]" },
	{ SR_BOSS_NAME,						L"[BossName]" },
	{ SR_QUEST_BOSS1,					L"[BossName1]" },
	{ SR_QUEST_BOSS2,					L"[BossName2]" },
	{ SR_QUEST_BOSS3,					L"[BossName3]" },
	{ SR_QUEST_BOSS4,					L"[BossName4]" },
	{ SR_QUEST_BOSS5,					L"[BossName5]" },
	{ SR_QUEST_BOSS6,					L"[BossName6]" },
	{ SR_QUEST_BOSS7,					L"[BossName7]" },
	{ SR_QUEST_BOSS8,					L"[BossName8]" },
	{ SR_QUEST_BOSS9,					L"[BossName9]" },
	{ SR_QUEST_BOSS10,					L"[BossName10]" },
	{ SR_QUEST_BOSS11,					L"[BossName11]" },
	{ SR_QUEST_BOSS12,					L"[BossName12]" },
	{ SR_QUEST_BOSS13,					L"[BossName13]" },
	{ SR_QUEST_BOSS14,					L"[BossName14]" },
	{ SR_QUEST_BOSS15,					L"[BossName15]" },
	{ SR_QUEST_BOSS16,					L"[BossName16]" },
	{ SR_OBJECT_NAME,					L"[objectName]" },
	{ SR_QUEST_OBJECT1,					L"[object1]" },
	{ SR_QUEST_OBJECT2,					L"[object2]" },
	{ SR_QUEST_OBJECT3,					L"[object3]" },
	{ SR_QUEST_OBJECT4,					L"[object4]" },
	{ SR_QUEST_OBJECT5,					L"[object5]" },
	{ SR_QUEST_OBJECT6,					L"[object6]" },
	{ SR_QUEST_OBJECT7,					L"[object7]" },
	{ SR_QUEST_OBJECT8,					L"[object8]" },
	{ SR_QUEST_OBJECT9,					L"[object9]" },
	{ SR_QUEST_OBJECT10,				L"[object10]" },
	{ SR_QUEST_OBJECT11,				L"[object11]" },
	{ SR_QUEST_OBJECT12,				L"[object12]" },
	{ SR_QUEST_OBJECT13,				L"[object13]" },
	{ SR_QUEST_OBJECT14,				L"[object14]" },
	{ SR_QUEST_OBJECT15,				L"[object15]" },
	{ SR_QUEST_OBJECT16,				L"[object16]" },	
	{ SR_QUEST_UNIT,					L"[questunit]" },	
	{ SR_QUEST_UNIT_COUNT,				L"[questunitcount]" },		
	{ SR_QUEST_UNIT_COLLECT_COUNT,		L"[questunitcollectcount]" },
	{ SR_QUEST_ITEMCOUNT1,				L"[itemcount1]" },
	{ SR_QUEST_ITEMCOUNT2,				L"[itemcount2]" },
	{ SR_QUEST_ITEMCOUNT3,				L"[itemcount3]" },
	{ SR_QUEST_ITEMCOUNT4,				L"[itemcount4]" },
	{ SR_QUEST_ITEMCOUNT5,				L"[itemcount5]" },
	{ SR_QUEST_ITEMCOUNT6,				L"[itemcount6]" },	
	{ SR_QUEST_OVERVIEW,				L"[overview]" },	
	{ SR_QUEST_ITEMCOLLECTION,			L"[itemcollection]" },	
	{ SR_QUEST_MONSTERSLAYINGS,			L"[slaymonsters]" },	
	{ SR_QUEST_DESCRIPTION,				L"[questdialog]" },	
	{ SR_QUEST_BOSS_STATUS,				L"[BossStatus]" },
	{ SR_QUEST_OBJECT_DESCRIPTION,		L"[objectdescription]" },
	{ SR_QUEST_OBJECT_INTERACTED,		L"[objectinteracted]" },
	{ SR_QUEST_OBJECT_INTERACTED_LIST,	L"[objectinteractionlist]" },
	{ SR_QUEST_ITEMDROP_FROM_UNIT1,		L"[itemspawnfromunit1]" },
	{ SR_QUEST_ITEMDROP_FROM_UNIT2,		L"[itemspawnfromunit2]" },
	{ SR_QUEST_ITEMDROP_FROM_UNIT3,		L"[itemspawnfromunit3]" },
	{ SR_QUEST_ITEMDROP_FROM_UNIT4,		L"[itemspawnfromunit4]" },
	{ SR_QUEST_ITEMDROP_FROM_UNIT5,		L"[itemspawnfromunit5]" },
	{ SR_QUEST_ITEMDROP_FROM_UNIT6,		L"[itemspawnfromunit6]" },
	{ SR_QUEST_BOSSES_DEFEATED,			L"[DefeatedBosses]" },
	{ SR_QUEST_BOSSES_TOTAL,			L"[TotalBosses]" },
	{ SR_HTTP_ACCOUNTS,					L"http://accounts" },
	{ SR_HTTP_ACCOUNTS_STAGING,			L"http://accounts-staging" },
	{ SR_HTTPS_ACCOUNTS,				L"https://accounts" },
	{ SR_HTTPS_ACCOUNTS_STAGING,		L"https://accounts-staging" },
	{ SR_HTTP_WWW,						L"http://www" },
	{ SR_HTTP_WWW_STAGING,				L"http://www-staging" },
	{ SR_HTTPS_WWW,						L"https://www" },
	{ SR_HTTPS_WWW_STAGING,				L"https://www-staging" },
	{ SR_HTTP_DOWNLOADS,				L"http://downloads" },
	{ SR_HTTP_DOWNLOADS_STAGING,		L"http://downloads-staging" },
	{ SR_HTTPS_DOWNLOADS,				L"https://downloads" },
	{ SR_HTTPS_DOWNLOADS_STAGING,		L"https://downloads-staging" },
	{ SR_HTTP_FORUMS,					L"http://forums" },
	{ SR_HTTP_FORUMS_STAGING,			L"http://forums-staging" },
	{ SR_HTTPS_FORUMS,					L"https://forums" },
	{ SR_HTTPS_FORUMS_STAGING,			L"https://forums-staging" },
	{ SR_CHAT_CHANNEL,					L"[chat_channel]" },
	{ SR_GUILD,							L"[guild]" },
	{ SR_MESSAGE,						L"[message]" },
	{ SR_URL,							L"[url]" },
	{ SR_PATCH_URL,						L"[patch_url]" },
	{ SR_RECIPE_NAME,					L"[recipename]" },
	{ SR_RECIPE_LVL,					L"[recipelvl]" },
	{ SR_VERSION,						L"[VERSION]" },
	{ SR_SINGLEPLAYER_VERSION,			L"[SP_VERSION]" },
	{ SR_MULTIPLAYER_VERSION,			L"[MP_VERSION]" },
	{ SR_MULTIPLAYER_DATA_VERSION,		L"[MP_DATA_VERSION]" },	
	{ SR_SKU_NAME,						L"[SKU_NAME]" },
	{ SR_SKU_NAME_LIST,					L"[SKU_NAME_LIST]" },
	{ SR_SKILL_NAME,					L"[skillname]" },
	{ SR_SKILL_LEVEL,					L"[skilllevel]" },
	{ SR_MIN,							L"[min]" },
	{ SR_MAX,							L"[max]" },
	{ SR_INVALID,						NULL }		// must be last
};

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *StringReplacementTokensGet(
	STRING_REPLACEMENT eReplacement)
{
	for (int i = 0; sgtStringReplacementTable[ i ].eReplacement != SR_INVALID; ++i)
	{
		const STRING_REPLACEMENT_LOOKUP	*pLookup = &sgtStringReplacementTable[ i ];
		if (pLookup->eReplacement == eReplacement)
		{
			return pLookup->puszTokens;
		}
	}
	return NULL;
}	


