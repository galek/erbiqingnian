//----------------------------------------------------------------------------
//
// excel.cpp
// 
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// NOTES:
// you can sort a table on load by setting EXCEL_TABLE_SORT in the table
// definition.  warning: the sort used is QUICKSORT which isn't stable.
// you can only sort a table if the table doesn't have any fields which
// index back into itself.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include "excel_private.h"
#include "act.h"
#include "filepaths.h"
#include "pakfile.h"
#include "c_palette.h"
#include "e_texture.h"
#include "e_texture_priv.h"
#include "dbunit.h"
#include "dictionary.h"
#include "unit_events.h"
#include "states.h"
#include "combat.h"
#include "items.h"
#include "unitmodes.h"
#include "script.h"
#include "language.h"
#include "stringtable.h"
#include "c_font.h"
#include "colorset.h"
#include "skills.h"
#include "skills_shift.h"
#include "player.h"
#include "faction.h"
#include "globalindex.h"
#include "c_sound_priv.h"
#include "c_music.h"
#include "e_effect_priv.h"
#include "e_budget.h"
#include "wardrobe.h"
#include "wardrobe_priv.h"
#include "characterclass.h"
#include "treasure.h"
#include "monsters.h"
#include "quests.h"
#include "c_animation.h"
#include "proc.h"
#include "dialog.h"
#include "c_backgroundsounds.h"
#include "c_footsteps.h"
#include "quest.h"
#include "recipe.h"
#include "npc.h"
#include "ai_priv.h"
#include "weather.h"
#include "LevelAreas.h"
#include "weapons_priv.h"
#include "offer.h"
#include "s_reward.h"
#include "objects.h"
#include "tasks.h"
#include "gossip.h"
#include "drlg.h"
#include "e_initdb.h"
#include "c_appearance_priv.h"
#include "uix_priv.h"
#include "unittag.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "LevelZones.h"
#include "LevelAreaNameGen.h"
#include "global_themes.h"
#include "c_movieplayer.h"
#include "gameglobals.h"
#include "LevelAreaLinker.h"
#include "quest_template.h"
#include "chat.h"
#include "s_chatNetwork.h"
#include "difficulty.h"
#include "warp.h"
#include "uix_components.h"
#include "region.h"
#include "wordfilter.h"
#include "achievements.h"
#include "gamevariant.h"
#include "crafting_properties.h"
#include "region.h"
#include "s_globalGameEventNetwork.h"
#include "versioning.h"

#ifdef HAVOK_ENABLED
#include "havok.h"
#endif
#include "sku.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STR_DICT tooltipareas[] =
{
	{ "",				SDTTA_OTHER			},
	{ "other",			SDTTA_OTHER			},
	{ "header",			SDTTA_HEADER		},
	{ "mod",			SDTTA_MOD			},
	{ "modicons",		SDTTA_MODICONS		},
	{ "damage",			SDTTA_DAMAGE		},
	{ "armor",			SDTTA_ARMOR			},
	{ "level",			SDTTA_LEVEL			},
	{ "price",			SDTTA_PRICE			},
	{ "requirements",	SDTTA_REQUIREMENTS	},
	{ "itemunqiue",		SDTTA_ITEMUNQIUE	},
	{ "ingredient",		SDTTA_INGREDIENT	},
	{ "itemtypes",		SDTTA_ITEMTYPES 	},
	{ "dps",			SDTTA_DPS			},
	{ "feeds",			SDTTA_FEEDS			},
	{ "extras",			SDTTA_EXTRAS		},
	{ "usable",			SDTTA_USABLE		},
	{ "sfx",			SDTTA_SFX			},
	{ "weaponstats",	SDTTA_WEAPONSTATS	},
	{ "affixprops",		SDTTA_AFFIXPROPS	},
	{ "otherpropshdr",	SDTTA_OTHERPROPSHDR	},
	{ "otherprops",		SDTTA_OTHERPROPS	},
	{ "modlist",		SDTTA_MODLIST		},
	{ "defense",		SDTTA_DEFENSE		},
	{ "meleespeed",		SDTTA_MELEESPEED	},
	{ "abovehead",		SDTTA_ABOVEHEAD		},
	{ "none",			SDTTA_NONE			},
	{ NULL,				SDTTA_OTHER			},
};

//----------------------------------------------------------------------------
STR_DICT sgtRegionDict[] =
{
	{ "",					REGION_INVALID },
	
	{ "NorthAmerica",		WORLD_REGION_NORTH_AMERICA },
	{ "Europe",				WORLD_REGION_EUROPE },
	{ "Japan",				WORLD_REGION_JAPAN },
	{ "Korea",				WORLD_REGION_KOREA },
	{ "China",				WORLD_REGION_CHINA },
	{ "Taiwan",				WORLD_REGION_TAIWAN },
	{ "SoutheastAsia",		WORLD_REGION_SOUTHEAST_ASIA },
	{ "SouthAmerica",		WORLD_REGION_SOUTH_AMERICA },
	{ "Australia",			WORLD_REGION_AUSTRALIA },
		
	{ NULL,					WORLD_REGION_INVALID },
};

//----------------------------------------------------------------------------
STR_DICT sgtLanguageDict[] =
{
	{ "",						LANGUAGE_INVALID },
	
	{ "English",				LANGUAGE_ENGLISH },
	{ "Japanese",				LANGUAGE_JAPANESE },
	{ "Korean",					LANGUAGE_KOREAN },
	{ "Chinese_Simplified",		LANGUAGE_CHINESE_SIMPLIFIED },
	{ "Chinese_Traditional",	LANGUAGE_CHINESE_TRADITIONAL },
	{ "French",					LANGUAGE_FRENCH },
	{ "Spanish",				LANGUAGE_SPANISH },
	{ "German",					LANGUAGE_GERMAN },
	{ "Italian",				LANGUAGE_ITALIAN },
	{ "Polish",					LANGUAGE_POLISH },
	{ "Czech",					LANGUAGE_CZECH },
	{ "Hungarian",				LANGUAGE_HUNGARIAN },
	{ "Russian",				LANGUAGE_RUSSIAN },
	{ "Thai",					LANGUAGE_THAI },
	{ "Vietnamese",				LANGUAGE_VIETNAMESE },
	
	{ NULL,						LANGUAGE_INVALID }
	
};

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(SKU_DATA)
	EF_STRING(		"Name",											szName															)
	EF_TWOCC(		"Code",											wCode															)
	EF_BOOL(		"Default",										bDefault,									FALSE				)
	EF_BOOL(		"Server",										bServer,									FALSE				)
	EF_BOOL(		"Developer",									bDeveloper,									FALSE				)
	EF_BOOL(		"Censor Locked",								bCensorLocked,								FALSE				)
	EF_BOOL(		"Censor Particles",								bCensorParticles,							FALSE				)
	EF_BOOL(		"Censor Bone Shrinking",						bCensorBoneShrinking,						FALSE				)
	EF_BOOL(		"Censor Monster Class Replacements No Humans",	bCensorMonsterClassReplacementsNoHumans,	FALSE				)
	EF_BOOL(		"Censor Monster Class Replacements No Gore",	bCensorMonsterClassReplacementsNoGore,		FALSE				)
	EF_BOOL(		"Censor PvP For Under Age",						bCensorPvPForUnderAge,						FALSE				)
	EF_BOOL(		"Censor PvP",									bCensorPvP,									FALSE				)
	EF_BOOL(		"Censor Movies",								bCensorMovies,								FALSE				)
	EF_BOOL(		"Low-Quality Movies Only",						bLowQualityMoviesOnly,						FALSE				)
	EF_STRING(		"Ad Public Key",								szAdPublicKey													)
	EF_STRING(		"Movielist Intro",								szMovielistIntro												)
	EF_LINK_INT(	"Titlescreen Movie",							nMovieTitleScreen,							DATATABLE_MOVIES	)
	EF_LINK_INT(	"Titlescreen Movie Wide",						nMovieTitleScreenWide,						DATATABLE_MOVIES	)
	EF_DICT_INT_ARR("Languages",									eLanguages,									sgtLanguageDict		)
	EF_DICT_INT_ARR("Regions",										eRegions,									sgtRegionDict		)			
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")	
	EXCEL_SET_POSTPROCESS_ALL(SKUExcelPostProcessAll)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SKU, SKU_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "sku") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(STRING_FILE_DEFINITION)
	EF_STRING(		"Name",									szName																)
	EF_BOOL(		"Is Common",							bIsCommon,		FALSE												)	
	EF_BOOL(		"Loaded by Game",						bLoadedByGame,	FALSE												)		
	EF_BOOL(		"Credits File",							bCreditsFile,	FALSE												)			
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_STRING_FILES,	STRING_FILE_DEFINITION,	APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "string_files") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(EXCEL_TABLE_DEFINITION)
	EF_STRING(	"Name",										szName																)
	EF_TWOCC(	"Code",										wCode																)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_EXCELTABLES, EXCEL_TABLE_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "exceltables") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(TAG_DEFINITION)
	EF_STRING(	"Name",										szName																)
	EF_TWOCC(	"Code",										wCode																)
	EF_BOOL(	"is value time",							bIsValueTimeTag														)
	EF_BOOL(	"is hotkey",								bIsHotkey															)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_TAG, TAG_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "tag") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(PALETTE_DATA)
	EF_FILEID(		"file",									fileid,										FILE_PATH_PALETTE,			PAK_DEFAULT			)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_PALETTES,	PALETTE_DATA, APP_GAME_ALL,	EXCEL_TABLE_PRIVATE, "palettes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(FONT_COLOR_DATA)
	EF_STRING(		"color",								szColor																)
	EF_BYTE(		"red",									rgbaColor.bRed														)
	EF_BYTE(		"green",								rgbaColor.bGreen													)
	EF_BYTE(		"blue",									rgbaColor.bBlue														)
	EF_BYTE(		"alpha",								rgbaColor.bAlpha													)
	EF_LINK_INT(	"fontcolor",							nFontColor,									DATATABLE_FONTCOLORS	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "color")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_FONTCOLORS, FONT_COLOR_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "fontcolor") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(COLORSET_DATA)
	EF_STRING(		"name",									szColor																)
	EF_TWOCC(		"code",									wCode																)
	EF_BOOL(		"can be random pick",					bCanBeRandomPick													)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
	EXCEL_SET_POSTPROCESS_ALL(ColorSetsExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_COLORSETS, COLORSET_DATA,	APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "colorsets") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(TEXTURE_TYPE_DATA)
	EF_STRING(		"name",									pszName																)
	EF_INT(			"background priority",					pnPriority[TEXTURE_GROUP_BACKGROUND]								)
	EF_INT(			"unit priority",						pnPriority[TEXTURE_GROUP_UNITS]										)
	EF_INT(			"wardrobe priority",					pnPriority[TEXTURE_GROUP_WARDROBE]									)
	EF_INT(			"particle priority",					pnPriority[TEXTURE_GROUP_PARTICLE]									)
	EF_INT(			"ui priority",							pnPriority[TEXTURE_GROUP_UI]										)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_TEXTURE_TYPES, TEXTURE_TYPE_DATA,	APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "texturetypes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(UNITTYPE_DATA)
	EF_STR_LOWER(	"type",									szName																)
	EF_TWOCC(		"code",									wCode																)
	EF_LINK_IARR(	"isa",									nTypeEquiv,									DATATABLE_UNITTYPES		)
	EF_STR_TABLE(	"string",								nDisplayName,								APP_GAME_ALL			)
	EF_LINK_INT(	"hellgate auction faction",			nHellgateAuctionFaction,					DATATABLE_UNITTYPES		)
	EF_LINK_INT(	"hellgate auction type",			nHellgateAuctionType,						DATATABLE_UNITTYPES		)
	EF_STR_TABLE(	"hellgate auction string",			nHellgateAuctionName,						APP_GAME_HELLGATE		)
	EF_LINK_INT(	"mythos auction faction",			nMythosAuctionFaction,						DATATABLE_UNITTYPES		)
	EF_LINK_INT(	"mythos auction type",				nMythosAuctionType,							DATATABLE_UNITTYPES		)
	EF_STR_TABLE(	"mythos auction string",			nMythosAuctionName,							APP_GAME_TUGBOAT		)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "type")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
	EXCEL_SET_NAMEFIELD("string")		
	EXCEL_SET_ISA("isa")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_UNITTYPES, UNITTYPE_DATA,	APP_GAME_ALL, EXCEL_TABLE_SHARED, "unittypes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(INVLOCIDX_DATA)
	EF_STRING(		"location",								szName																)
	EF_TWOCC(		"code",									wCode																)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "location")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_INVLOCIDX, INVLOCIDX_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "invloc") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT sgUnitRecordDict[] =
{
	{ "",					UNIT_EVENT_RECORD_IGNORE		},
	{ "new",				UNIT_EVENT_RECORD_NEW_ENTRY		},
	{ "count",				UNIT_EVENT_RECORD_COUNT_EVENTS	},
	{ "damage",				UNIT_EVENT_RECORD_SUM_DAMAGE	},
	{ "skill count",		UNIT_EVENT_RECORD_SKILL_COUNT	},
	{ "stat count",			UNIT_EVENT_RECORD_STAT			},
	{ "add summary",		UNIT_EVENT_RECORD_ADD_SUMMARY_EVENTS},
	{ NULL,					0								},
};

EXCEL_STRUCT_DEF(UNIT_EVENT_TYPE_DATA)
	EF_STRING(		"name",									szName																)
	EF_DICT_INT(	"record monster",						pRecordMethodByGenus[GENUS_MONSTER],		sgUnitRecordDict		)
	EF_DICT_INT(	"record player",						pRecordMethodByGenus[GENUS_PLAYER],			sgUnitRecordDict		)
	EF_DICT_INT(	"record object",						pRecordMethodByGenus[GENUS_OBJECT],			sgUnitRecordDict		)
	EF_DICT_INT(	"record missile",						pRecordMethodByGenus[GENUS_MISSILE],		sgUnitRecordDict		)
	EF_DICT_INT(	"record item",							pRecordMethodByGenus[GENUS_ITEM],			sgUnitRecordDict		)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_UNIT_EVENT_TYPES,	UNIT_EVENT_TYPE_DATA, APP_GAME_ALL,	EXCEL_TABLE_SHARED, "unitevents") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(STATE_EVENT_TYPE_DATA)
	EF_STRING(		"name",									szName																)
	EF_STRING(		"param text",							szParamText,														)
	EF_STRING(		"set event handler",					szSetEventHandler,													)		
	EF_STRING(		"clear event handler",					szClearEventHandler,												)
	EF_BOOL(		"client only",							bClientOnly															)
	EF_BOOL(		"server only",							bServerOnly															)
	EF_BOOL(		"can clear on free",					bCanClearOnFree														)
	EF_BOOL(		"uses bone",							bUsesBone															)
	EF_BOOL(		"uses bone weight",						bUsesBoneWeight														)
	EF_BOOL(		"uses attachments",						bUsesAttachments													)
	EF_BOOL(		"uses material override type",			bUsesMatOverrideType												)
	EF_BOOL(		"uses environment override",			bUsesEnvironmentOverride											)
	EF_BOOL(		"uses screen effects",					bUsesScreenEffects													)
	EF_BOOL(		"adds ropes on add target",				bAddsRopes															)
	EF_BOOL(		"refresh stats pointer",				bRefreshStatsPointer												)
	EF_BOOL(		"control unit only",					bControlUnitOnly													)
	EF_BOOL(		"requires loaded graphics",				bRequiresLoadedGraphics												)
	EF_BOOL(		"apply to appearance override",			bApplyToAppearanceOverride											)
	EF_LINK_TABLE(	"uses table 0",							pnUsesTable[STATE_TABLE_REFERENCE_ATTACHMENT]						)
	EF_LINK_TABLE(	"uses table 1",							pnUsesTable[STATE_TABLE_REFERENCE_EVENT]							)
	EF_FLAG(		"uses force new",						dwFlagsUsed,								STATE_EVENT_FLAG_FORCE_NEW_BIT							)
	EF_FLAG(		"uses first person",					dwFlagsUsed,								STATE_EVENT_FLAG_FIRST_PERSON_BIT						)
	EF_FLAG(		"uses add to center",					dwFlagsUsed,								STATE_EVENT_FLAG_ADD_TO_CENTER_BIT						)
	EF_FLAG(		"uses control unit only",				dwFlagsUsed,								STATE_EVENT_FLAG_CONTROL_UNIT_ONLY_BIT					)
	EF_FLAG(		"uses float",							dwFlagsUsed,								STATE_EVENT_FLAG_FLOAT_BIT								)
	EF_FLAG(		"uses owned by control",				dwFlagsUsed,								STATE_EVENT_FLAG_OWNED_BY_CONTROL_BIT					)
	EF_FLAG(		"uses set immediately",					dwFlagsUsed,								STATE_EVENT_FLAG_SET_IMMEDIATELY_BIT					)
	EF_FLAG(		"uses clear immediately",				dwFlagsUsed,								STATE_EVENT_FLAG_CLEAR_IMMEDIATELY_BIT					)
	EF_FLAG(		"uses not control unit",				dwFlagsUsed,								STATE_EVENT_FLAG_NOT_CONTROL_UNIT_BIT					)
	EF_FLAG(		"uses on weapons",						dwFlagsUsed,								STATE_EVENT_FLAG_ON_WEAPONS_BIT							)
	EF_FLAG(		"uses ignore camera",					dwFlagsUsed,								STATE_EVENT_FLAG_IGNORE_CAMERA_BIT						)
	EF_FLAG(		"uses on clear",						dwFlagsUsed,								STATE_EVENT_FLAG_ON_CLEAR_BIT							)	
	EF_FLAG(		"",										dwFlagsUsed,								STATE_EVENT_FLAG_CHECK_CONDITION_ON_CLEAR_BIT,	TRUE	)	
	EF_FLAG(		"reapply on appearance load",			dwFlagsUsed,								STATE_EVENT_FLAG_REAPPLY_ON_APPEARANCE_LOAD_BIT			)		
	EF_FLAG(		"uses share duration",					dwFlagsUsed,								STATE_EVENT_FLAG_SHARE_DURATION_BIT,			TRUE	)	
	EF_FLAG(		"uses check condition on weapons",		dwFlagsUsed,								STATE_EVENT_FLAG_CHECK_CONDITION_ON_WEAPONS_BIT,TRUE	)	
	EF_INT(			"combo filter function 0",				pnTableFilter[0],							INVALID_ID				)
	EF_INT(			"combo filter function 1",				pnTableFilter[1],							INVALID_ID				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelStateEventTypePostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_STATE_EVENT_TYPES, STATE_EVENT_TYPE_DATA,	APP_GAME_ALL, EXCEL_TABLE_SHARED, "state_event_types") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(STATE_LIGHTING_DATA)
	EF_STRING(		"name",									szName,																)
	EF_STRING(		"SH_cubemap_filename",					szSHCubemap,														)
	EF_INT(			"",										nSHCubemap,									INVALID_ID				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_STATE_LIGHTING, STATE_LIGHTING_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "state_lighting") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT tDictGameVariant[] =
{
	{ "",			GV_INVALID		},
	{ "elite",		GV_ELITE		},
	{ "hardcore",	GV_HARDCORE		},
	{ "league",		GV_LEAGUE		},
	{ "pvpworld",	GV_PVPWORLD		},
	{ NULL,			GV_INVALID		},	
};

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(STATE_DATA)
	EF_DSTRING(		"name",									szName																)
	EF_TWOCC(		"code",									wCode																)
	EF_DSTRING(		"file",									szEventFile															)
	EF_LINK_INT(	"on death",								nOnDeathState,								DATATABLE_STATES		)
	EF_FLAG(		"used in hellgate",						pdwFlags,									SDF_USED_IN_HELLGATE	)
	EF_FLAG(		"used in tugboat",						pdwFlags,									SDF_USED_IN_TUGBOAT		)
	EF_FLAG(		"Stacks",								pdwFlags,									SDF_STACKS				)
	EF_FLAG(		"Stacks per Source",					pdwFlags,									SDF_STACKS_PER_SOURCE	)
	EF_FLAG(		"send to all",							pdwFlags,									SDF_SEND_TO_ALL			)
	EF_FLAG(		"send to self",							pdwFlags,									SDF_SEND_TO_SELF		)
	EF_FLAG(		"send stats",							pdwFlags,									SDF_SEND_STATS			)
	EF_FLAG(		"client needs duration",				pdwFlags,									SDF_CLIENT_NEEDS_DURATION)
	EF_FLAG(		"client only",							pdwFlags,									SDF_CLIENT_ONLY			)
	EF_FLAG(		"execute parent events",				pdwFlags,									SDF_EXECUTE_PARENT_EVENTS)
	EF_FLAG(		"trigger notarget on set",				pdwFlags,									SDF_TRIGGER_NO_TARGET_EVENT_ON_SET)
	EF_FLAG(		"save with unit",						pdwFlags,									SDF_SAVE_WITH_UNIT		)
	EF_FLAG(		"trigger digest save",					pdwFlags,									SDF_TRIGGER_DIGEST_SAVE )	
	EF_FLAG(		"save position on set",					pdwFlags,									SDF_SAVE_POSITION_ON_SET)
	EF_FLAG(		"sharing mod state",					pdwFlags,									SDF_SHARING_MOD_STATE	)
	EF_FLAG(		"flag for load",						pdwFlags,									SDF_FLAG_FOR_LOAD		)
	EF_INT(			"duration",								nDefaultDuration													)
	EF_INT(			"skill script param",					nSkillScriptParam													)
	EF_LINK_IARR(	"isa",									pnTypeEquiv,								DATATABLE_STATES		)
	EF_LINK_INT(	"state prevented by",					nStatePreventedBy,							DATATABLE_STATES		)	
	EF_LINK_INT(	"element",								nDamageTypeElement,							DATATABLE_DAMAGETYPES	)
	EF_CODE(		"pulse rate in ms",						codePulseRateInMS													)
	EF_CODE(		"pulse rate in ms client",				codePulseRateInMSClient												)
	EF_DSTRING(		"pulse skill",							tPulseSkill.szSkillPulse											)	
	EF_FLAG(		"pulse on source",						pdwFlags,									SDF_PULSE_ON_SOURCE		)
	EF_FLAG(		"is bad",								pdwFlags,									SDF_IS_BAD				)
	EF_INT(			"icon order",							nIconOrder															)
	EF_DSTRING(		"UI icon",								szUIIconFrame														)	
	EF_DSTRING(		"UI icon texture",						szUIIconTexture														)	
	EF_LINK_INT(	"icon color",							nIconColor,									DATATABLE_FONTCOLORS	)
	EF_DSTRING(		"UI icon back",							szUIIconBackFrame													)	
	EF_LINK_INT(	"icon back color",						nIconBackColor,								DATATABLE_FONTCOLORS	)
	EF_STR_TABLE(	"icon tooltip string (all)",			nIconTooltipStr[ APP_GAME_ALL ],			APP_GAME_ALL			)
	EF_STR_TABLE(	"icon tooltip string (hellgate)",		nIconTooltipStr[ APP_GAME_HELLGATE ],		APP_GAME_HELLGATE		)
	EF_STR_TABLE(	"icon tooltip string (mythos)",			nIconTooltipStr[ APP_GAME_TUGBOAT ],		APP_GAME_TUGBOAT		)
	EF_LINK_INT(	"assoc state 1",						nStateAssoc[0],								DATATABLE_STATES		)
	EF_LINK_INT(	"assoc state 2",						nStateAssoc[1],								DATATABLE_STATES		)
	EF_LINK_INT(	"assoc state 3",						nStateAssoc[2],								DATATABLE_STATES		)
	EF_FLAG(		"on change repaint item ui",			pdwFlags,									SDF_ON_CHANGE_REPAINT_ITEM_UI)
	EF_FLAG(		"Execute Attack Script Melee",			dwFlagsScripts,								STATE_SCRIPT_FLAG_EXCEL_ATTACK_MELEE	)
	EF_FLAG(		"Execute Attack Script Ranged",			dwFlagsScripts,								STATE_SCRIPT_FLAG_EXCEL_ATTACK_RANGED	)	
	EF_FLAG(		"execute skill script on remove",		dwFlagsScripts,								STATE_SCRIPT_FLAG_EXCEL_ON_REMOVE		)	
	EF_FLAG(		"execute script on source",				dwFlagsScripts,								STATE_SCRIPT_FLAG_EXCEL_EXECUTE_ON_SOURCE	)	
	EF_FLAG(		"pulse on client too",					dwFlagsScripts,								STATE_SCRIPT_FLAG_EXCEL_PULSE_CLIENT	)	
	EF_DICT_INT(	"game flag",							eGameVariant,								tDictGameVariant		)
	EF_FLAG(		"save in unitfile header",				pdwFlags,									SDF_SAVE_IN_UNITFILE_HEADER	)	
	EF_FLAG(		"persist timer across games",	pdwFlags,									SDF_PERSISTANT	)	
	EF_FLAG(		"update chat server on change",			pdwFlags,									SDF_UPDATE_CHAT_SERVER	)		
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
	EXCEL_SET_POSTPROCESS_ALL(ExcelStatesPostProcess)
	EXCEL_SET_ROWFREE(ExcelStatesRowFree)
	EXCEL_SET_ISA("isa")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_STATES, STATE_DATA, APP_GAME_ALL,	EXCEL_TABLE_SHARED, "states") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT statstype[] =
{
	{ "",					STATSTYPE_NONE				},
	{ "regen",				STATSTYPE_REGEN				},
	{ "feed",				STATSTYPE_FEED				},
	{ "req",				STATSTYPE_REQUIREMENT		},
	{ "req-limit",			STATSTYPE_REQ_LIMIT			},
	{ "%degen",				STATSTYPE_DEGEN_PERCENT		},
	{ "alloc",				STATSTYPE_ALLOCABLE			},
	{ "max",				STATSTYPE_MAX				},
	{ "max-ratio",			STATSTYPE_MAX_RATIO			},
	{ "max-ratio-decrease",	STATSTYPE_MAX_RATIO_DECREASE},
	{ "maxpure",			STATSTYPE_MAX_PURE			},
	{ "degen",				STATSTYPE_DEGEN				},
	{ "%regen",				STATSTYPE_REGEN_PERCENT		},
	{ "regenclient",		STATSTYPE_REGEN_CLIENT		},
	{ "degenclient",		STATSTYPE_DEGEN_CLIENT		},
	{ NULL,					0							},
};

STR_DICT statspecfunc[] =
{
	{ "",					STATSSPEC_NONE				},
	{ "regen_on_get",		STATSSPEC_REGEN_ON_GET		},
	{ "pct_regen_on_get",	STATSSPEC_PCT_REGEN_ON_GET	},
	{ NULL,					0							},
};

STR_DICT tDictDBUnitField[] =
{
	{ "",					DBUF_FULL_UPDATE		},
	{ "experience",			DBUF_EXPERIENCE			},
	{ "money",				DBUF_MONEY				},
	{ "quantity",			DBUF_QUANTITY			},
	{ "inventory location",	DBUF_INVENTORY_LOCATION	},
	{ "rankexp",			DBUF_RANK_EXPERIENCE    },
	{ NULL,					DBUF_FULL_UPDATE		},
};

EXCEL_STRUCT_DEF(STATS_DATA)
	EF_STRING(		"stat",									m_szName,																	)
	EF_TWOCC(		"code",									m_wCode,																	)
	EF_LINK_INT(	"unittype",								m_nUnitType,						DATATABLE_UNITTYPES						)
	EF_FLAG(		"calc",									m_dwFlags,							STATS_DATA_ALWAYS_CALC					)
	EF_FLAG(		"cur",									m_dwFlags,							STATS_DATA_CURRENT_ONLY					)
	EF_FLAG(		"no max/cur when dead",					m_dwFlags,							STATS_DATA_NO_REACT_WHEN_DEAD			)
	EF_LINK_INT(	"",										m_nMaxStat,							DATATABLE_STATS							)
	EF_INT(			"min set",								m_nMinSet,							INT_MIN									)
	EF_INT(			"max set",								m_nMaxSet,							-1										)
	EF_INT(			"min assert",							m_nMinAssert,						INT_MIN									)
	EF_INT(			"max assert",							m_nMaxAssert,						INT_MAX									)
	EF_DICT_INT(	"type",									m_nStatsType,						statstype								)
	EF_LINK_INT(	"assocstat1",							m_nAssociatedStat[0],				DATATABLE_STATS							)
	EF_LINK_INT(	"assocstat2",							m_nAssociatedStat[1],				DATATABLE_STATS							)
	EF_LINK_INT(	"",										m_nRegenByStat[0],					DATATABLE_STATS							)
	EF_LINK_INT(	"",										m_nRegenByStat[1],					DATATABLE_STATS							)
	EF_INT(			"regen interval in ms",					m_nRegenIntervalInMS														)
	EF_INT(			"shift",								m_nShift																	)
	EF_INT(			"offset",								m_nStatOffset																)
	EF_FLAG(		"vector",								m_dwFlags,							STATS_DATA_VECTOR						)
	EF_FLAG(		"float",								m_dwFlags,							STATS_DATA_FLOAT						)
	EF_BOOL(		"player",								m_bUnitTypeDebug[GENUS_PLAYER]												)
	EF_BOOL(		"monster",								m_bUnitTypeDebug[GENUS_MONSTER]												)
	EF_BOOL(		"missile",								m_bUnitTypeDebug[GENUS_MISSILE]												)
	EF_BOOL(		"item",									m_bUnitTypeDebug[GENUS_ITEM]												)
	EF_BOOL(		"object",								m_bUnitTypeDebug[GENUS_OBJECT]												)
	EF_FLAG(		"modlist",								m_dwFlags,							STATS_DATA_MODLIST						)
	EF_FLAG(		"accrue",								m_dwFlags,							STATS_DATA_ACCRUE						)
	EF_FLAG(		"accrue once only",						m_dwFlags,							STATS_DATA_ACCRUE_ONCE					)
	EF_LINK_UTYPE(	"accrue to types",						m_nAccrueTo																	)
	EF_LINK_IARR(	"check against types",					m_nReqirementTypes,					DATATABLE_UNITTYPES						)
	EF_FLAG(		"calc rider",							m_dwFlags,							STATS_DATA_CALC_RIDER					)
	EF_FLAG(		"transfer",								m_dwFlags,							STATS_DATA_TRANSFER						)
	EF_FLAG(		"transfer to missile",					m_dwFlags,							STATS_DATA_TRANSFER_TO_MISSILE			)
	EF_FLAG(		"don't transfer to nonweapon missile",	m_dwFlags,							STATS_DATA_DONT_TRANSFER_TO_NONWEAPON_MISSILE			)
	EF_FLAG(		"combat",								m_dwFlags,							STATS_DATA_COMBAT						)
	EF_FLAG(		"directdmg",							m_dwFlags,							STATS_DATA_DIRECT_DAMAGE				)
	EF_FLAG(		"send",									m_dwFlags,							STATS_DATA_SEND_TO_SELF					)
	EF_FLAG(		"sendall",								m_dwFlags,							STATS_DATA_SEND_TO_ALL					)
	EF_FLAG(		"save",									m_dwFlags,							STATS_DATA_SAVE							)
	EF_FLAG(		"state monitors c",						m_dwFlags,							STATS_DATA_STATE_CAN_MONITOR_CLIENT		)
	EF_FLAG(		"state monitors s",						m_dwFlags,							STATS_DATA_STATE_CAN_MONITOR_SERVER		)
	EF_FLAG(		"update database",						m_dwFlags,							STATS_DATA_UPDATE_DATABASE				)	
	EF_FLAG(		"clear on item restore",				m_dwFlags,							STATS_DATA_CLEAR_ON_ITEM_RESORE			)		
	EF_INT(			"valBits",								m_nValBits																	)
	EF_INT(			"valWindow",							m_nValBitsWindow															)
	EF_INT(			"valShift",								m_nValShift																	)
	EF_INT(			"valOffs",								m_nValOffs																	)
	EF_LINK_TABLE(	"valTable",								m_nValTable																	)
	EF_INT(			"param1Bits",							m_nParamBits[0]																)
	EF_INT(			"param1Shift",							m_nParamShift[0]															)
	EF_INT(			"param1Offs",							m_nParamOffs[0]																)
	EF_LINK_TABLE(	"param1Table",							m_nParamTable[0]															)
	EF_INT(			"param2Bits",							m_nParamBits[1]																)
	EF_INT(			"param2Shift",							m_nParamShift[1]															)
	EF_INT(			"param2Offs",							m_nParamOffs[1]																)
	EF_LINK_TABLE(	"param2Table",							m_nParamTable[1]															)
	EF_INT(			"param3Bits",							m_nParamBits[2]																)
	EF_INT(			"param3Shift",							m_nParamShift[2]															)
	EF_INT(			"param3Offs",							m_nParamOffs[2]																)
	EF_LINK_TABLE(	"param3Table",							m_nParamTable[2]															)
	EF_INT(			"param4Bits",							m_nParamBits[3]																)
	EF_INT(			"param4Shift",							m_nParamShift[3]															)
	EF_INT(			"param4Offs",							m_nParamOffs[3]																)
	EF_LINK_TABLE(	"param4Table",							m_nParamTable[3]															)
	EF_DICT_INT(	"specFunc",								m_nSpecialFunction,					statspecfunc							)
	EF_LINK_INT(	"sfStat1",								m_nSpecFuncStat[0],					DATATABLE_STATS							)
	EF_LINK_INT(	"sfStat2",								m_nSpecFuncStat[1],					DATATABLE_STATS							)
	EF_LINK_INT(	"sfStat3",								m_nSpecFuncStat[2],					DATATABLE_STATS							)
	EF_LINK_INT(	"sfStat4",								m_nSpecFuncStat[3],					DATATABLE_STATS							)
	EF_LINK_INT(	"sfStat5",								m_nSpecFuncStat[4],					DATATABLE_STATS							)
	EF_INT(			"regen divisor",						m_nRegenDivisor,					1										)
	EF_INT(			"regen delay on zero",					m_nRegenDelayOnZero,														)
	EF_INT(			"regen delay on dec",					m_nRegenDelayOnDec,															)
	EF_INT(			"regen delay monster",					m_nMonsterRegenDelay,														)	
	EF_STR_TABLE(	"req fail string",						m_nRequirementFailString[ RFA_ALL ],		APP_GAME_ALL					)
	EF_STR_TABLE(	"req fail string hellagate",			m_nRequirementFailString[ RFA_HELLGATE ],	APP_GAME_HELLGATE				)
	EF_STR_TABLE(	"req fail string mythos",				m_nRequirementFailString[ RFA_MYTHOS ],		APP_GAME_TUGBOAT				)
	EF_STRING(		"version function",						szVersionFunction															)	
	EF_INT(			"min ticks between db commits",			m_nMinTicksBetweenDatabaseCommits,											)		
	EF_DICT_INT(	"database unit field",					m_eDBUnitField,						tDictDBUnitField						)
	
	EXCEL_SET_POSTINDEX(ExcelStatsPostIndex)
	EXCEL_SET_POSTPROCESS_TABLE(ExcelStatsPostProcess)
	EXCEL_SET_POSTPROCESS_ALL(ExcelStatsPostLoadAll)
	EXCEL_SET_ROWFREE(ExcelStatsRowFree)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "stat")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_STATS, STATS_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "stats") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT statsfuncapp[] =
{
	{ "",					STATSFUNC_APP_BOTH			},
	{ "hellgate",			STATSFUNC_APP_HELLGATE		},
	{ "mythos",				STATSFUNC_APP_MYTHOS		},
};

EXCEL_STRUCT_DEF(STATS_FUNCTION)
	EF_LINK_INT(	"target",								m_nTargetStat,						DATATABLE_STATS							)
	EF_DICT_INT(	"app",									m_nApp,								statsfuncapp							)
	EF_LINK_INT(	"control unit",							m_nControlUnitType,					DATATABLE_UNITTYPES						)
	EF_STRING(		"formula",								m_szCalc																	)
	EXCEL_SET_POSTPROCESS_ALL(ExcelStatsFuncPostLoadAll)
	EXCEL_SET_ROWFREE(ExcelStatsFuncRowFree)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_STATS_FUNC, STATS_FUNCTION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "statsfunc") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(STATS_SELECTOR_DATA)
	EF_STRING(		"Name",		szName	)
	EF_TWOCC(		"Code",		wCode	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")	
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_STATS_SELECTOR, STATS_SELECTOR_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "statsselector") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(DAMAGE_TYPE_DATA)
	EF_STRING(		"damagetype",							szDamageType																)
	EF_TWOCC(		"code",									wCode																		)
	EF_STRING(		"miniicon",								szMiniIcon																	)
	EF_STRING(		"string",								szDisplayString																)
	EF_LINK_INT(	"color",								nColor,								DATATABLE_FONTCOLORS					)
	EF_LINK_INT(	"shield hit",							nShieldHitState,					DATATABLE_STATES						)
	EF_LINK_INT(	"critical state",						nCriticalState,						DATATABLE_STATES						)
	EF_LINK_INT(	"soft hit state",						nSoftHitState,						DATATABLE_STATES						)
	EF_LINK_INT(	"medium hit state",						nMediumHitState,					DATATABLE_STATES						)
	EF_LINK_INT(	"big hit state",						nBigHitState,						DATATABLE_STATES						)
	EF_LINK_INT(	"fumble hit state",						nFumbleHitState,					DATATABLE_STATES						)
	EF_LINK_INT(	"damage over time state",				nDamageOverTimeState,				DATATABLE_STATES						)
	EF_LINK_INT(	"field missile",						nFieldMissile,						DATATABLE_MISSILES						)
	EF_LINK_INT(	"invulnerable state",					nInvulnerableState,					DATATABLE_STATES						)
	EF_LINK_INT(	"thorns state",							nThornsState,						DATATABLE_STATES						)
	EF_LINK_INT(	"invulnerable sfx state",				nInvulnerableSFXState,				DATATABLE_STATES						)
	EF_INT(			"Vulnerability in PVP Tugboat",			nVulnerabilityInPVPTugboat,			0										)
	EF_INT(			"Vulnerability in PVP Hellgate",		nVulnerabilityInPVPHellgate,		0										)
	EF_FLOAT(		"sfx invulnerability duration mult in PVP Tugboat",		fSFXInvulnerabilityDurationMultInPVPTugboat,	0.0f		)
	EF_FLOAT(		"sfx invulnerability duration mult in PVP Hellgate",	fSFXInvulnerabilityDurationMultInPVPHellgate,	0.0f		)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "damagetype")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_DAMAGETYPES, DAMAGE_TYPE_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "damagetypes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(DAMAGE_EFFECT_DATA)
	EF_STRING(		"damageeffect",							szDamageEffect,																)
	EF_TWOCC(		"Code",									wCode																		)
	EF_LINK_INT(	"damagetype",							nDamageTypeID,						DATATABLE_DAMAGETYPES					)
	EF_CODE(		"code sfx duration in ms",				codeSfxDuration																)
	EF_CODE(		"code sfx effect",						codeBaseSfxEffect															)	
	EF_CODE(		"conditional",							codeConditional																)	
	EF_CODE(		"missile stats",						codeMissileStats															)	
	EF_LINK_INT(	"invulnerable state",					nInvulnerableState,					DATATABLE_STATES						)
	EF_LINK_INT(	"sfx state",							nSfxState,							DATATABLE_STATES						)
	EF_LINK_INT(	"Execute Skill",						nAttackerSkill,						DATATABLE_SKILLS						)
	EF_LINK_INT(	"Execute Skill On Target",				nTargetSkill,						DATATABLE_SKILLS						)
	EF_LINK_INT(	"attackers prohibiting state",			nAttackerProhibitingState,			DATATABLE_STATES						)
	EF_LINK_INT(	"defenders prohibiting state",			nDefenderProhibitingState,			DATATABLE_STATES						)
	EF_LINK_INT(	"attacker requires state",				nAttackerRequiresState,				DATATABLE_STATES						)
	EF_LINK_INT(	"defender requires state",				nDefenderRequiresState,				DATATABLE_STATES						)
	EF_LINK_INT(	"missile to attach",					nMissileToAttach,					DATATABLE_MISSILES						)
	EF_LINK_INT(	"field missile",						nFieldMissile,						DATATABLE_MISSILES						)
	EF_BOOL(        "no roll if parent dmg type success",	bUseParentsRoll,															)
	EF_BOOL(        "no roll needed",						bNoRollNeeded,																)	
	EF_BOOL(        "must be crit",							bMustBeCrit,																)	
	EF_BOOL(        "monster must die",						bMonsterMustDie																)
	EF_BOOL(        "requires no damage",					bRequiresNoDamage															)
	EF_BOOL(        "does not require damage",				bDoesntRequireDamage														)
	EF_BOOL(        "don't use ultimate attacker",			bDontUseUltimateAttacker													)
	EF_BOOL(        "don't use sfx defense",				bDoesntUseSFXDefense														)	
	EF_BOOL(        "use override stats",					bUsesOverrideStats															)	
	EF_INT(			"Player vs. Monster Scaling Index",		nPlayerVsMonsterScalingIndex,		0										)
	EF_BOOL(        "do not use effect chance stat",		bDontUseEffectChance														)	
	EF_LINK_INT(	"attack stat",							nAttackStat,						DATATABLE_STATS							)
	EF_LINK_INT(	"attack local stat",					nAttackLocalStat,					DATATABLE_STATS							)
	EF_LINK_INT(	"attack splash stat",					nAttackSplashStat,					DATATABLE_STATS							)
	EF_LINK_INT(	"attack pct stat",						nAttackPctStat,						DATATABLE_STATS							)
	EF_LINK_INT(	"attack pct local stat",				nAttackPctLocalStat,				DATATABLE_STATS							)
	EF_LINK_INT(	"attack pct splash stat",				nAttackPctSplashStat,				DATATABLE_STATS							)
	EF_LINK_INT(	"attack pct caste stat",				nAttackPctCasteStat,				DATATABLE_STATS							)
	EF_LINK_INT(	"defense stat",							nDefenseStat,						DATATABLE_STATS							)
	EF_LINK_INT(	"effect defense stat",					nEffectDefenseStat,					DATATABLE_STATS							)
	EF_LINK_INT(	"effect defense pct stat",				nEffectDefensePctStat,				DATATABLE_STATS							)
	EF_INT(			"default duration in ticks",			nDefaultDuration,					0										)
	EF_INT(			"default strength",						nDefaultStrength,					0										)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "damageeffect")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_DAMAGE_EFFECTS, DAMAGE_EFFECT_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "damageeffects") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(PROPERTYDEF_DATA)
	EF_CODE(		"code",									codeProperty																)
	EXCEL_ADD_ANCESTOR(DATATABLE_STATES)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_PROPERTIES, PROPERTYDEF_DATA,	APP_GAME_ALL, EXCEL_TABLE_SHARED, "properties") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(UNITMODEGROUP_DATA)
	EF_STRING(		"name",									szName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_UNITMODE_GROUPS, UNITMODEGROUP_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "unitmode_groups") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT sgUnitModeVelocityDict[] =
{
	{ "",				INVALID_ID				},
	{ "walk",			MODE_VELOCITY_WALK 		},
	{ "strafe",			MODE_VELOCITY_STRAFE	},
	{ "jump",			MODE_VELOCITY_JUMP		},
	{ "run",			MODE_VELOCITY_RUN		},
	{ "backup",			MODE_VELOCITY_BACKUP	},
	{ "knockback",		MODE_VELOCITY_KNOCKBACK	},
	{ "melee",			MODE_VELOCITY_MELEE		},
	{ NULL,				0						},
};

EXCEL_STRUCT_DEF(UNITMODE_DATA)
	EF_DSTRING(		"mode",									szName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EF_LINK_INT(	"group",								nGroup,								DATATABLE_UNITMODE_GROUPS				)
	EF_LINK_INT(	"other hand",							eOtherHandMode,						DATATABLE_UNITMODES						)
	EF_LINK_INT(	"backup",								eBackupMode,						DATATABLE_UNITMODES						)
	EF_LINK_IARR(	"block",								pnBlockingModes,					DATATABLE_UNITMODE_GROUPS				)
	EF_BOOL(		"blockonground",						bBlockOnGround																)		
	EF_LINK_IARR(	"wait",									pnWaitModes,						DATATABLE_UNITMODE_GROUPS				)
	EF_LINK_IARR(	"clear",								pnClearModes,						DATATABLE_UNITMODE_GROUPS				)
	EF_BOOL(		"force clear",							bForceClear																	)
	EF_BOOL(		"clearai",								bClearAi																	)
	EF_BOOL(		"clearskill",							bClearSkill																	)
	EF_LINK_INT(	"setstateformode",						nState,								DATATABLE_STATES						)
	EF_LINK_INT(	"clearstate",							nClearState,						DATATABLE_STATES						)
	EF_LINK_INT(	"clearstate end",						nClearStateEnd,						DATATABLE_STATES						)
	EF_BOOL(		"doevent",								bDoEvent																	)
	EF_BOOL(		"endevent",								bEndEvent																	)
	EF_DSTRING(		"DoFunction",							szDoFunction																)
	EF_DSTRING(		"ClearFunction",						szClearFunction																)
	EF_DSTRING(		"EndFunction",							szEndFunction																)
	EF_LINK_IARR(	"endblock",								pnBlockEndModes,					DATATABLE_UNITMODE_GROUPS				)
	EF_LINK_INT(	"endmode",								nEndMode,							DATATABLE_UNITMODES						)
	EF_BOOL(		"noloop",								bNoLoop																		)	
	EF_BOOL(		"restoreai",							bRestoreAi																	)
	EF_BOOL(		"steps",								bSteps																		)
	EF_BOOL(		"no animation",							bNoAnimation																)
	EF_INT(			"anim priority",						nAnimationPriority															)
	EF_DICT_INT(	"velocity name",						nVelocityIndex,						sgUnitModeVelocityDict					)
	EF_INT(			"velocity priority",					nVelocityPriority,					-1										)
	EF_INT(			"load priority percent",				nLoadPriorityPercent														)
	EF_BOOL(		"velocity uses melee speed",			bVelocityUsesMeleeSpeed														)
	EF_BOOL(		"velocity changed by stats",			bVelocityChangedByStats														)
	EF_BOOL(		"ragdoll",								bRagdoll																	)
	EF_BOOL(		"play right",							bPlayRightAnim																)
	EF_BOOL(		"play left",							bPlayLeftAnim																)
	EF_BOOL(		"play torso",							bPlayTorsoAnim																)
	EF_BOOL(		"play legs",							bPlayLegAnim																)
	EF_BOOL(		"play just default",					bPlayJustDefault															)
	EF_BOOL(		"is aggressive",						bIsAggressive																)	
	EF_BOOL(		"play all variations",					bPlayAllVariations															)		
	EF_BOOL(		"reset mixable on start",				bResetMixableOnStart														)
	EF_BOOL(		"reset mixable on end",					bResetMixableOnEnd															)
	EF_BOOL(		"rand start time",						bRandStartTime																)
	EF_BOOL(		"duration from anims",					bDurationFromAnims															)	
	EF_BOOL(		"duration from contact",				bDurationFromContactPoint													)	
	EF_BOOL(		"clear adjust stance",					bClearAdjustStance															)	
	EF_BOOL(		"check can interrupt",					bCheckCanInterrupt															)
	EF_BOOL(		"lazy end for control unit",			bLazyEndForControlUnit														)
	EF_BOOL(		"use backup mode anims",				bUseBackupModeAnims															)
	EF_BOOL(		"play on inventory model",				bPlayOnInventoryModel														)
	EF_BOOL(		"hide weapons",							bHideWeapons																)
	EF_BOOL(		"emote allowed hellgate",				bEmoteAllowedHellgate														)
	EF_BOOL(		"emote allowed mythos",					bEmoteAllowedMythos															)
	EXCEL_SET_MAXROWS(MAX_UNITMODES)
	EXCEL_SET_POSTPROCESS_TABLE(ExcelUnitModesPostProcess)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "mode")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_UNITMODES, UNITMODE_DATA,	APP_GAME_ALL, EXCEL_TABLE_SHARED, "unitmodes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(ANIMATION_STANCE_DATA)
	EF_STRING(		"name",									szName																		)
	EF_BOOL(		"don't change from",					bDontChangeFrom																)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ANIMATION_STANCE,	ANIMATION_STANCE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "animation_stance") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(ANIMATION_GROUP_DATA)
	EF_STRING(		"name",									szName																		)
	EF_LINK_INT(	"fallback",								nFallBack,							DATATABLE_ANIMATION_GROUP				)
	EF_LINK_INT(	"right weapon",							nRightWeapon,						DATATABLE_ANIMATION_GROUP				)
	EF_LINK_INT(	"left weapon",							nLeftWeapon,						DATATABLE_ANIMATION_GROUP				)
	EF_LINK_INT(	"right anims",							nRightAnims,						DATATABLE_ANIMATION_GROUP				)
	EF_LINK_INT(	"left anims",							nLeftAnims,							DATATABLE_ANIMATION_GROUP				)
	EF_LINK_INT(	"leg anims",							nLegAnims,							DATATABLE_ANIMATION_GROUP				)
	EF_BOOL(		"play right left",						bPlayRightLeftAnims															)
	EF_BOOL(		"play legs",							bPlayLegAnims																)
	EF_BOOL(		"show in Hammer",						bShowInHammer																)
	EF_BOOL(		"copy footsteps",						bCopyFootsteps																)
	EF_BOOL(		"only play subgroups",					bOnlyPlaySubgroups															)
	EF_LINK_INT(	"default stance",						nDefaultStance,						DATATABLE_ANIMATION_STANCE				)
	EF_LINK_INT(	"default stance in town",				nDefaultStanceInTown,				DATATABLE_ANIMATION_STANCE				)
	EF_BOOL(		"can start skill with left weapon",		bCanStartSkillWithLeftWeapon												)
	EF_FLOAT(		"seconds to hold stance",				fSecondsToHoldStance														)
	EF_FLOAT(		"seconds to hold stance in town",		fSecondsToHoldStanceInTown													)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ANIMATION_GROUP, ANIMATION_GROUP_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE,	"animation_groups") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(ANIMATION_CONDITION_DATA)
	EF_STRING(	"Name",										szName																		)
	EF_STRING(	"Condition",								szFunctionName																)	
	EF_INT(		"PriorityBoostSuccess",						nPriorityBoostSuccess														)
	EF_INT(		"PriorityBoostFailure",						nPriorityBoostFailure														)
	EF_BOOL(	"RemoveOnFailure",							bRemoveOnFailure															)	
	EF_BOOL(	"Check On Play",							bCheckOnPlay																)	
	EF_BOOL(	"Check On Update Weights",					bCheckOnUpdateWeights														)	
	EF_BOOL(	"Check On Context Change",					bCheckOnContextChange														)	
	EF_BOOL(	"Ignore on Failure",						bIgnoreOnFailure															)	
	EF_BOOL(	"Ignore Stance Outside Condition",			bIgnoreStanceOutsideCondition												)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelAnimationConditionPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ANIMATION_CONDITION, ANIMATION_CONDITION_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE,	"animation_condition") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT sgDisplayRules[] =
{
	{ "",			0	},
	{ "always",		0	},
	{ "ditto",		1	},
	{ "elif",		2	},
	{ "elseif",		2	},
	{ "oneof",		3	},
	{ NULL,			0	},
};
STR_DICT sgDisplayControl[] =
{
	{ "",			DISPLAY_CONTROL_SINGLE	},
	{ "range",		DISPLAY_CONTROL_PARAM_RANGE	},
	{ NULL,			0	},
};
STR_DICT sgDisplayFunctions[] =
{
	{ "",			DISPLAY_FUNCTION_FORMAT	},
	{ "format",		DISPLAY_FUNCTION_FORMAT	},
	{ "inventory",	DISPLAY_FUNCTION_INVENTORY },
	{ NULL,			0 },
};
STR_DICT sgDisplayValueControls[] =
{
	{	"",							DISPLAY_VALUE_CONTROL_SCRIPT_VALUE_NOPRINT },	
	{	"script noprint",			DISPLAY_VALUE_CONTROL_SCRIPT_VALUE_NOPRINT },	
	{	"script",					DISPLAY_VALUE_CONTROL_SCRIPT },					
	{	"script plusminus",			DISPLAY_VALUE_CONTROL_SCRIPT_PLUSMINUS },				
	{	"script plusminus nozero",	DISPLAY_VALUE_CONTROL_SCRIPT_PLUSMINUS_NOZERO },		
	{	"unit name",				DISPLAY_VALUE_CONTROL_UNIT_NAME },				
	{	"unit class",				DISPLAY_VALUE_CONTROL_UNIT_CLASS },			
	{	"unit guild",				DISPLAY_VALUE_CONTROL_UNIT_GUILD },			
	{	"created by",				DISPLAY_VALUE_CONTROL_CREATED_BY },				
	{	"param string",				DISPLAY_VALUE_CONTROL_STAT_PARAM_STRING },		
	{	"param string plural",		DISPLAY_VALUE_CONTROL_STAT_PARAM_STRING_PLURAL},	
	{	"stat value",				DISPLAY_VALUE_CONTROL_STAT_VALUE },			
	{	"plusminus",				DISPLAY_VALUE_CONTROL_PLUSMINUS },				
	{	"plusminus nozero",			DISPLAY_VALUE_CONTROL_PLUSMINUS_NOZERO },		
	{	"unit type",				DISPLAY_VALUE_CONTROL_UNIT_TYPE },				
	{	"unit type lastonly",		DISPLAY_VALUE_CONTROL_UNIT_TYPE_LASTONLY },			
	{	"unit type quality",		DISPLAY_VALUE_CONTROL_UNIT_TYPE_QUALITY },
	{	"unit additional",			DISPLAY_VALUE_CONTROL_UNIT_ADDITIONAL },
	{	"affix",					DISPLAY_VALUE_CONTROL_AFFIX },					
	{	"flavor text",				DISPLAY_VALUE_CONTROL_FLAVOR },				
	{	"div by 100",				DISPLAY_VALUE_CONTROL_SCRIPT_DIVIDE_BY_100 },	
	{	"div by 10",				DISPLAY_VALUE_CONTROL_SCRIPT_DIVIDE_BY_10 },	
	{	"div by 20",				DISPLAY_VALUE_CONTROL_SCRIPT_DIVIDE_BY_20 },	
	{	"item quality",				DISPLAY_VALUE_CONTROL_ITEM_QUALITY },				
	{	"class requirements",		DISPLAY_VALUE_CONTROL_CLASS_REQUIREMENTS },		
	{	"faction requirements",		DISPLAY_VALUE_CONTROL_FACTION_REQUIREMENTS },		
	{	"skill group",				DISPLAY_VALUE_CONTROL_SKILLGROUP },
	{	"item unique",				DISPLAY_VALUE_CONTROL_ITEM_UNIQUE },
	{	"item ingredient",			DISPLAY_VALUE_CONTROL_ITEM_INGREDIENT },
	{	"affix props list",			DISPLAY_VALUE_CONTROL_AFFIX_PROPS_LIST },
	{	"inv item properties",		DISPLAY_VALUE_CONTROL_INV_ITEM_PROPS },
	{	"item dmg desc",			DISPLAY_VALUE_CONTROL_ITEM_DMG_DESC},
	
	{	NULL,						0 },	
};
	
EXCEL_STRUCT_DEF(STATS_DISPLAY)
	EF_STRING(		"example/description",					szDebugName																	)
	EF_BOOL(		"rider",								bRider																		)
	EF_STRING(		"key",									szName																		)
	EF_DICT_INT(	"tooltip area",							nTooltipArea,						tooltipareas							)
	EF_BOOL(		"newline",								bNewLine,							TRUE									)
	EF_DICT_INT(	"rule1",								nDisplayRule[0],					sgDisplayRules							)
	EF_BOOL(		"incl unit in cond1",					bIncludeUnitInCondition[0]													)
	EF_CODE(		"displaycondition1",					codeDisplayCondition[0]														)
	EF_DICT_INT(	"rule2",								nDisplayRule[1],					sgDisplayRules							)
	EF_BOOL(		"incl unit in cond2",					bIncludeUnitInCondition[1]													)
	EF_CODE(		"displaycondition2",					codeDisplayCondition[1]														)
	EF_DICT_INT(	"rule3",								nDisplayRule[2],					sgDisplayRules							)
	EF_BOOL(		"incl unit in cond3",					bIncludeUnitInCondition[2]													)
	EF_CODE(		"displaycondition3",					codeDisplayCondition[2]														)
	EF_DICT_INT(	"displayctrl",							eDisplayControl,					sgDisplayControl						)
	EF_LINK_INT(	"ctrlstat",								nDisplayControlStat,				DATATABLE_STATS							)
	EF_DICT_INT(	"displayfunc",							eDisplayFunc,						sgDisplayFunctions						)
	EF_STR_TABLE(	"formatstring",							nFormatString,						APP_GAME_ALL							)
	EF_STR_TABLE(	"formatshort",							nShortFormatString,					APP_GAME_ALL							)
	EF_STR_TABLE(	"descripshort",							nShortDescripString,				APP_GAME_ALL							)
	EF_STRING(		"iconframe",							szIconFrame																	)
	EF_CODE(		"color",								codeColor																	)
	EF_DICT_INT(	"ctrl1",								peValueCtrl[0],						sgDisplayValueControls					)
	EF_CODE(		"val1",									codeValue[0]																)
	EF_DICT_INT(	"ctrl2",								peValueCtrl[1],						sgDisplayValueControls					)
	EF_CODE(		"val2",									codeValue[1]																)
	EF_DICT_INT(	"ctrl3",								peValueCtrl[2],						sgDisplayValueControls					)
	EF_CODE(		"val3",									codeValue[2]																)
	EF_DICT_INT(	"ctrl4",								peValueCtrl[3],						sgDisplayValueControls					)
	EF_CODE(		"val4",									codeValue[3]																)
	EF_STR_TABLE(	"tooltip text",							nToolTipText,						APP_GAME_ALL							)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "key")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_CHARDISPLAY, STATS_DISPLAY, APP_GAME_ALL,	EXCEL_TABLE_PRIVATE, "display_char") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_ITEMDISPLAY, STATS_DISPLAY, APP_GAME_ALL,	EXCEL_TABLE_PRIVATE, "display_item") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_INDEX_DEF(DATATABLE_INVENTORY_TYPES, "Name");
EXCEL_TBIDX_DEF(DATATABLE_INVENTORY_TYPES, EXCEL_INDEX_TABLE, APP_GAME_ALL, EXCEL_TABLE_SHARED, "inventory_types") EXCEL_TABLE_END
				
//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(INVLOC_DATA)
	EF_STRING(		"description",							szName																		)
	EF_LINK_INT(	"inventory type",						nInventoryType,						DATATABLE_INVENTORY_TYPES				)
	EF_LINK_INT(	"location",								nLocation,							DATATABLE_INVLOCIDX						)
	EF_INT(			"Color Set Priority",					nColorSetPriority,					-1										)
	EF_LINK_INT(	"slot stat",							nSlotStat,							DATATABLE_STATS							)
	EF_LINK_INT(	"max slot stat",						nMaxSlotStat,						DATATABLE_STATS							)
	EF_FLAG(		"dynamic",								dwFlags,							INVLOCFLAG_DYNAMIC						)
	EF_FLAG(		"onesize",								dwFlags,							INVLOCFLAG_ONESIZE						)
	EF_FLAG(		"grid",									dwFlags,							INVLOCFLAG_GRID							)
	EF_FLAG(		"wardrobe",								dwFlags,							INVLOCFLAG_WARDROBE						)
	EF_FLAG(		"autopickup",							dwFlags,							INVLOCFLAG_AUTOPICKUP					)
	EF_FLAG(		"equip location",						dwFlags,							INVLOCFLAG_EQUIPSLOT					)	
	EF_FLAG(		"use in random armor",					dwFlags,							INVLOCFLAG_USE_IN_RANDOM_ARMOR			)	
	EF_FLAG(		"store",								dwFlags,							INVLOCFLAG_STORE						)		
	EF_FLAG(		"merchant warehouse",					dwFlags,							INVLOCFLAG_MERCHANT_WAREHOUSE			)			
	EF_FLAG(		"known only when stash open",			dwFlags,							INVLOCFLAG_ONLY_KNOWN_WHEN_STASH_OPEN	)
	EF_FLAG(		"stash location",						dwFlags,							INVLOCFLAG_STASH_LOCATION				)	
	EF_FLAG(		"ingredient loc",						dwFlags,							INVLOCFLAG_INGREDIENTS					)				
	EF_FLAG(		"pet slot",								dwFlags,							INVLOCFLAG_PETSLOT						)
	EF_FLAG(		"don't save",							dwFlags,							INVLOCFLAG_DONTSAVE						)
	EF_FLAG(		"resurrectable",						dwFlags,							INVLOCFLAG_RESURRECTABLE				)
	EF_FLAG(		"link deaths",							dwFlags,							INVLOCFLAG_LINKDEATHS					)
	EF_FLAG(		"offhand wardrobe",						dwFlags,							INVLOCFLAG_OFFHANDWARDROBE				)
	EF_FLAG(		"skills check on ultimate owner",		dwFlags,							INVLOCFLAG_SKILL_CHECK_ON_ULTIMATE_OWNER)
	EF_FLAG(		"skills check on control unit",			dwFlags,							INVLOCFLAG_SKILL_CHECK_ON_CONTROL_UNIT	)
	EF_FLAG(		"destroy pet on level change",			dwFlags,							INVLOCFLAG_DESTROY_PET_ON_LEVEL_CHANGE	)
	EF_FLAG(		"remove from inventory on owner death",	dwFlags,							INVLOCFLAG_REMOVE_FROM_INVENTORY_ON_OWNER_DEATH	)
	EF_FLAG(		"cannot accept no drop items",			dwFlags,							INVLOCFLAG_CANNOT_ACCEPT_NO_DROP_ITEMS  )
	EF_FLAG(		"cannot accept no trade items",			dwFlags,							INVLOCFLAG_CANNOT_ACCEPT_NO_TRADE_ITEMS )
	EF_FLAG(		"cannot dismantle items",				dwFlags,							INVLOCFLAG_CANNOT_DISMANTLE_ITEMS		)
	EF_FLAG(		"weaponconfig location",				dwFlags,							INVLOCFLAG_WEAPON_CONFIG_LOCATION		)
	EF_FLAG(		"free on size change",					dwFlags,							INVLOCFLAG_FREE_ON_SIZE_CHANGE			)
	EF_FLAG(		"enable cache location",				dwFlags,							INVLOCFLAG_CACHE_ENABLING				)
	EF_FLAG(		"disable cache location",				dwFlags,							INVLOCFLAG_CACHE_DISABLING				)
	EF_FLAG(		"is email location",					dwFlags,							INVLOCFLAG_EMAIL_LOCATION				)	
	EF_FLAG(		"is email inbox",						dwFlags,							INVLOCFLAG_EMAIL_INBOX					)		
	EF_LINK_UTYPE(	"types",								nAllowTypes																	)
	EF_LINK_UTYPE(	"only items usable by type",			nUsableByTypes,						"any"									)
	EF_INT(			"width",								nWidth,								1										)
	EF_INT(			"height",								nHeight,							1										)
	EF_INT(			"filter",								nFilter																		)
	EF_INT(			"check class",							bCheckClass																	)	
	EF_LINK_INT(	"preventloc1",							nPreventLocation1,					DATATABLE_INVLOCIDX						)
	EF_LINK_UTYPE(	"preventtype1",							nPreventTypes1																)
	EF_LINK_INT(	"allowskill1",							nAllowTypeSkill[0],					DATATABLE_SKILLS						)
	EF_LINK_UTYPE(	"skillallowtype1",						nSkillAllowTypes[0]															)
	EF_INT(			"allowskilllevel1",						nSkillAllowLevels[0],				1										)
	EF_LINK_INT(	"allowskill2",							nAllowTypeSkill[1],					DATATABLE_SKILLS						)
	EF_LINK_UTYPE(	"skillallowtype2",						nSkillAllowTypes[1]															)
	EF_INT(			"allowskilllevel2",						nSkillAllowLevels[1],				1										)
	EF_LINK_INT(	"allowskill3",							nAllowTypeSkill[2],					DATATABLE_SKILLS						)
	EF_LINK_UTYPE(	"skillallowtype3",						nSkillAllowTypes[2]															)
	EF_INT(			"allowskilllevel3",						nSkillAllowLevels[2],				1										)
	EF_INT(			"weapon index",							nWeaponIndex,						INVALID_ID								)
	EF_BOOL(		"physically in container",				bPhysicallyInContainer														)			
	EF_BOOL(		"trade location",						bTradeLocation																)				
	EF_BOOL(		"return stuck items to standard loc",	bReturnStuckItemsToStandardLoc												)						
	EF_BOOL(		"standard location",					bStandardLocation															)					
	EF_BOOL(		"on person location",					bOnPersonLocation															)						
	EF_BOOL(		"allowed hotkey source location",		bAllowedHotkeySourceLocation												)						
	EF_BOOL(		"reward location",						bRewardLocation																)
	EF_BOOL(		"server only location",					bServerOnyLocation															)	
	EF_BOOL(		"cursor location",						bIsCursor																	)	
	EF_CODE(		"player put restricted",				codePlayerPutRestricted														)
	EF_CODE(		"player take restricted",				codePlayerTakeRestricted													)
	EF_LINK_INT(	"inv loc fallback on load error",		nInvLocLoadFallbackOnLoadError,		DATATABLE_INVLOCIDX						)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "inventory type", "location")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_INVLOC, INVLOC_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "inventory") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(INVENTORY_DATA)
	EF_INT(			"count",								nNumLocs																	)
	EXCEL_SET_POSTPROCESS_TABLE(ExcelInventoryPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_INVENTORY, INVENTORY_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE,	"inventoryex") EXCEL_TABLE_SET_NOLOAD  EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(SKILLTAB_DATA)
	EF_STRING(		"Name",									szName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EF_STR_TABLE(	"DisplayString",						nDisplayString,			APP_GAME_ALL										)
	EF_BOOL(		"draw only known",						bDrawOnlyKnown																)
	EF_BOOL(		"perk tab",								bIsPerkTab																	)
	EF_STRING(		"icon texture name",					szSkillIconTexture															)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SKILLTABS, SKILLTAB_DATA,	APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "skilltabs") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(SKILLGROUP_DATA)
	EF_STRING(		"Name",									szName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EF_BOOL(		"Display In Skill String",				bDisplayInSkillString,			FALSE										)
	EF_BOOL(		"Don't clear cooldown on death",		bDontClearCooldownOnDeath,		FALSE										)
	EF_STR_TABLE(	"DisplayString",						nDisplayString,			APP_GAME_ALL										)
	EF_STRING(		"BackgroundIcon",						szBackgroundIcon															)
	EXCEL_SET_NAMEFIELD("DisplayString");
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SKILLGROUPS, SKILLGROUP_DATA,	APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "skillgroups") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(CMDMENU_DATA)
	EF_STRING(		"cmd menu",								szCmdMenu																	)
	EF_STRING(		"chat command",							szChatCommand																)
	EF_BOOL(		"immediate",							bImmediate																	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "cmd menu", "chat command")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_CMD_MENUS, CMDMENU_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "cmd_menus") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT sgtSkillUsage[] =
{
	{ "",				USAGE_INVALID },
	{ "Active",			USAGE_ACTIVE },
	{ "Passive",		USAGE_PASSIVE },
	{ "Toggle",			USAGE_TOGGLE },
	{ NULL,				USAGE_INVALID },
};
STR_DICT sgtSkillActivatorKeys[] =
{
	{ "",				SKILL_ACTIVATOR_KEY_INVALID },
	{ "Shift",			SKILL_ACTIVATOR_KEY_SHIFT },
	{ "Potion",			SKILL_ACTIVATOR_KEY_POTION },
	{ "Melee",			SKILL_ACTIVATOR_KEY_MELEE },
	{ NULL,				SKILL_ACTIVATOR_KEY_INVALID },
};
STR_DICT sgtPlayerInputOverrides[] =
{
	{ "",				PLAYER_MOVE_NONE },
	{ "forward right",	PLAYER_MOVE_FORWARD | PLAYER_STRAFE_RIGHT },
	{ "forward left",	PLAYER_MOVE_FORWARD | PLAYER_STRAFE_LEFT },
	{ "forward",		PLAYER_MOVE_FORWARD },
	{ "back right",		PLAYER_MOVE_BACK | PLAYER_STRAFE_RIGHT },
	{ "back left",		PLAYER_MOVE_BACK | PLAYER_STRAFE_LEFT  },
	{ "back",			PLAYER_MOVE_BACK },
	{ "left",			PLAYER_STRAFE_LEFT },
	{ "right",			PLAYER_STRAFE_RIGHT },
	{ NULL,				PLAYER_MOVE_NONE },
};
STR_DICT sgtSkillStartFunctions[] =
{
	{ "",				SKILL_STARTFUNCTION_NULL	},
	{ "Null",			SKILL_STARTFUNCTION_NULL	},
	{ "Generic",		SKILL_STARTFUNCTION_GENERIC	},
	{ "No_Modes",		SKILL_STARTFUNCTION_NOMODES	},
	{ "Warm_Up",		SKILL_STARTFUNCTION_WARMUP	},
	{ NULL,				SKILL_STARTFUNCTION_NULL	},
};

STR_DICT sgtSkillFamily[] =
{
	{ "",				SKILL_FAMILY_INVALID },
	{ "TurretGround",	SKILL_FAMILY_TURRET_GROUND },
	{ "TurretFlying",	SKILL_FAMILY_TURRET_FLYING },	
	{ NULL,				SKILL_FAMILY_INVALID }
};

EXCEL_STRUCT_DEF(SKILL_DATA)
	EF_STRING(		"skill",								szName																		)
	EF_FOURCC(		"code",									dwCode																		)
	EF_STR_TABLE(	"Display Name",							nDisplayName,						APP_GAME_ALL							)
	EF_FILE(		"events",								szEvents																	)
	EF_INT(			"",										nEventsId,							INVALID_ID								)
	EF_FILE(		"summoned ai",							szSummonedAI																)
	EF_INT(			"",										nSummonedAI,						INVALID_ID								)
	EF_LINK_IARR(	"summoned inv location",				pnSummonedInventoryLocations,		DATATABLE_INVLOCIDX						)	
	EF_LINK_IARR(	"skill level includes skills",			nSkillLevelIncludeSkills,			DATATABLE_SKILLS						)	
	
	EF_INT(			"max summoned minor pet classes",		nMaxSummonedMinorPetClasses,		INVALID_ID								)
	EF_IARR(		"levels",								pnSkillLevelToMinLevel,				-1										)
	EF_INT(			"max level",							nMaxLevel,							1										)
	EF_IARR(		"Perk Point Cost",						pnPerkPointCost,					-1										)
	EF_STRING(		"largeicon",							szLargeIcon,																)
	EF_STRING(		"smallicon",							szSmallIcon,																)
	EF_LINK_INT(	"IconColor",							nIconColor,							DATATABLE_FONTCOLORS					)
	EF_LINK_INT(	"IconBackgroundColor",					nIconBkgndColor,					DATATABLE_FONTCOLORS					)
	EF_LINK_INT(	"SkillTab",								nSkillTab,							DATATABLE_SKILLTABS						)
	EF_LINK_IARR(	"SkillGroup",							pnSkillGroup,						DATATABLE_SKILLGROUPS					)
	EF_LINK_INT(	"Mode Override",						nModeOverride,						DATATABLE_UNITMODES						)
	EF_LINK_IARR(	"Required Stats",						pnRequiredStats,					DATATABLE_STATS							)
	EF_IARR(		"Required Stat Values 1",				pnRequiredStatValues[0],			-1										)
	EF_IARR(		"Required Stat Values 2",				pnRequiredStatValues[1],			-1										)
	EF_LINK_IARR(	"Required Skills",						pnRequiredSkills,					DATATABLE_SKILLS						)

	EF_BOOL(		"Uses Crafting Points",					bUsesCraftingPoints														)

	EF_IARR(		"Levels of Required Skills",			pnRequiredSkillsLevels,				-1										)
	EF_BOOL(		"Only Require One",						bOnlyRequireOneSkill														)
	EF_INT(			"SkillPage Column",						nSkillPageX,						INVALID_ID								)
	EF_INT(			"SkillPage Row",						nSkillPageY,						INVALID_ID								)
	EF_LINK_INT(	"ui throbs on state",					nUIThrobsOnState,					DATATABLE_STATES						)	
	EF_FLOAT(		"power cost",							fPowerCost																	)
	EF_FLOAT(		"power cost per level",					fPowerCostPerLevel															)	
	EF_CODE(		"Power Cost Script",					codePowerCost																)
	EF_CODE(		"Power Cost PCT Mult Script",			codePowerCostMultPCT														)
	
	EF_CODE(		"Cooldown Skill Script",				codeCoolDown																)
	
	EF_CODE(		"select cost",							codeSelectCost																)
	EF_LINK_INT(	"select check stat",					nSelectCheckStat,					DATATABLE_STATS							)
	EF_INT(			"priority",								nPriority,							-1										)
	EF_LINK_INT(	"gives skill",							nGivesSkill,						DATATABLE_SKILLS						)
	EF_LINK_INT(	"extra skill to turn on",				nExtraSkillToTurnOn,				DATATABLE_SKILLS						)
	EF_LINK_INT(	"target unittype",						tTargetQueryFilter.nUnitType,		DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"ignore unittype",						tTargetQueryFilter.nIgnoreUnitType,	DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"requires unittype",					nRequiredUnittype,					DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"requires weapon unittype",				nRequiredWeaponUnittype,			DATATABLE_UNITTYPES						)
	EF_FLAG(		"uses item requirements",				dwFlags,							SKILL_FLAG_USES_ITEM_REQUIREMENTS		)	
	EF_LINK_INT(	"fuse missiles on state clear",			nFuseMissilesOnState,				DATATABLE_STATES						)
	EF_LINK_INT(	"requires state",						nRequiredState,						DATATABLE_STATES						)
	EF_LINK_IARR(	"prohibiting state",					nProhibitingStates,					DATATABLE_STATES						)
	EF_CODE(		"select condition",						codeSelectCondition															)
	EF_LINK_INT(	"state on select",						nStateOnSelect,						DATATABLE_STATES						)
	EF_LINK_INT(	"clear state on select",				nClearStateOnSelect,				DATATABLE_STATES						)
	EF_LINK_INT(	"prevent clear state on select",		nPreventClearStateOnSelect,			DATATABLE_STATES						)
	EF_LINK_INT(	"clear state on deselect",				nClearStateOnDeselect,				DATATABLE_STATES						)

	EF_LINK_IARR(	"ignore targets with state",			tTargetQueryFilter.pnIgnoreUnitsWithState,		DATATABLE_STATES			)
	EF_LINK_IARR(	"select targets with state",			tTargetQueryFilter.pnAllowUnitsOnlyWithState,	DATATABLE_STATES			)
	EF_LINK_IARR(	"bonus skills( 5 max )",				tSkillBonus.nSkillBonusBySkills,				DATATABLE_SKILLS			)
	EF_CODE(		"bonus skill script 0",					tSkillBonus.nSkillBonusByValueByScript[0]									)
	EF_CODE(		"bonus skill script 1",					tSkillBonus.nSkillBonusByValueByScript[1]									)
	EF_CODE(		"bonus skill script 2",					tSkillBonus.nSkillBonusByValueByScript[2]									)
	EF_CODE(		"bonus skill script 3",					tSkillBonus.nSkillBonusByValueByScript[3]									)
	EF_CODE(		"bonus skill script 4",					tSkillBonus.nSkillBonusByValueByScript[4]									)	
	EF_CODE(		"skill var 0",							codeVariables[SKILL_VARIABLE_ONE]											)
	EF_CODE(		"skill var 1",							codeVariables[SKILL_VARIABLE_TWO]											)
	EF_CODE(		"skill var 2",							codeVariables[SKILL_VARIABLE_THREE]											)
	EF_CODE(		"skill var 3",							codeVariables[SKILL_VARIABLE_FOUR]											)
	EF_CODE(		"skill var 4",							codeVariables[SKILL_VARIABLE_FIVE]											)
	EF_CODE(		"skill var 5",							codeVariables[SKILL_VARIABLE_SIX]											)
	EF_CODE(		"skill var 6",							codeVariables[SKILL_VARIABLE_SEVEN]											)
	EF_CODE(		"skill var 7",							codeVariables[SKILL_VARIABLE_EIGHT]											)
	EF_CODE(		"skill var 8",							codeVariables[SKILL_VARIABLE_NINE]											)
	EF_CODE(		"skill var 9",							codeVariables[SKILL_VARIABLE_TEN]											)
	EF_CODE(		"Range Mult Script( gets div by 100 )",	codeSkillRangeScript														)
	EF_CODE(		"cost",									codeCost																	)
	EF_CODE(		"cooldown percent change",				codeCooldownPercentChange													)
	EF_CODE(		"Crafting Script",						codeCrafting																)
	EF_CODE(		"Crafting Properties Script",			codeCraftingProperties														)
	EF_CODE(		"statsTransferOnAttack",				codeStatsSkillTransferOnAttack												)
	EF_CODE(		"statsSkillEvent",						codeStatsSkillEvent															)
	EF_CODE(		"statsSkillEventServer",				codeStatsSkillEventServer													)
	EF_CODE(		"statsSkillEventServer postprocess",	codeStatsSkillEventServerPostProcess										)	
	EF_CODE(		"statsServerPostLaunch",				codeStatsServerPostLaunch													)
	EF_CODE(		"statsServerPostLaunch post",			codeStatsServerPostLaunchPostProcess										)
	EF_CODE(		"statsPostLaunch",						codeStatsPostLaunch															)
	EF_CODE(		"statsOnStateSet",						codeStatsOnStateSet															)
	EF_CODE(		"statsServerOnStateSet",				codeStatsOnStateSetServerOnly												)
	EF_CODE(		"statsServerOnStateSet post",			codeStatsOnStateSetServerPostProcess										)
	EF_CODE(		"statsOnStateSet post",					codeStatsOnStateSetPostProcess												)
	EF_CODE(		"statsOnPulse ServerOnly",				codeStatsOnPulseServerOnly													)
	EF_CODE(		"statsOnPulse",							codeStatsOnPulse															)
	EF_CODE(		"statsOnDeselectServerOnly",			codeStatsOnDeselectServerOnly												)
	EF_CODE(		"statsOnLevelChange",					codeStatsOnLevelChange														)
	EF_CODE(		"statsOnSkillStart",					codeStatsOnSkillStart														)
	EF_CODE(		"statsScriptOnTarget",					codeStatsScriptOnTarget														)
	EF_CODE(		"scriptFromScriptEvents",				codeStatsScriptFromEvents													)
	EF_CODE(		"start condition",						codeStartCondition															)
	EF_CODE(		"start condition on target",			codeStartConditionOnTarget													)
	EF_CODE(		"state duration in ticks",				codeStateDuration															)
	EF_CODE(		"state duration bonus",					codeStateDurationBonus														)
	EF_CODE(		"activator condition",					codeActivatorCondition														)
	EF_CODE(		"activator condition on target",		codeActivatorConditionOnTargets												)
	EF_CODE(		"event chance",							codeEventChance																)
	EF_CODE(		"event param 0",						codeEventParam0																)
	EF_CODE(		"event param 1",						codeEventParam1																)
	EF_CODE(		"event param 2",						codeEventParam2																)
	EF_CODE(		"event param 3",						codeEventParam3																)
	EF_CODE(		"info script",							codeInfoScript																)
	EF_CODE(		"state removed server",					codeStateRemoved															)
	EF_DICT_INT(	"startfunc",							nStartFunc,							sgtSkillStartFunctions					)
	EF_INT(			"targetfunc",							nTargetingFunc																)
	EF_INT(			"hold ticks",							nMinHoldTicks																)
	EF_LINK_INT(	"hold with mode",						nHoldWithMode,						DATATABLE_UNITMODES						)
	EF_INT(			"Warmup Ticks",							nWarmupTicks																)
	EF_INT(			"Test Ticks",							nTestTicks,							40										)
	EF_INT(			"Cooldown",								nCooldownTicks																)
	
	EF_LINK_INT(	"cooldown skill group",					nCooldownSkillGroup,				DATATABLE_SKILLGROUPS					)
	EF_INT(			"Cooldown for Group",					nCooldownTicksForSkillGroup													)
	EF_LINK_INT(	"cooldown finished sound",				nCooldownFinishedSound,				DATATABLE_SOUNDS						)
	EF_INT(			"cooldown min percent",					nCooldownMinPercent,				50										)
	EF_FLOAT(		"RangeMin",								fRangeMin																	)
	EF_FLOAT(		"RangeMax",								fRangeMax																	)
	EF_FLOAT(		"Range Percent Per Level",				fRangePercentPerLevel														)
	EF_FLOAT(		"FiringCone",							fFiringCone,						180.0f									)
	EF_FLOAT(		"RangeDesired",							fRangeDesired																)
	EF_FLOAT(		"Weapon Range Multiplier",				fWeaponRangeMultiplier,				1.0f									)
	EF_FLOAT(		"Damage Multiplier",					fDamageMultiplier,					-1.0f									)
	EF_FLOAT(		"mode speed",							fModeSpeedMultiplier,				1.0f									)
	EF_LINK_INT(	"damage type override",					nDamageTypeOverride,				DATATABLE_DAMAGETYPES					)
	EF_INT(			"Max Extra Spread Bullets",				nMaxExtraSpreadBullets,				-1										)
	EF_INT(			"Spread Bullet Multiplier",				nSpreadBulletMultiplier,			1										)	
	EF_FLOAT(		"impact forward bias",					fImpactForwardBias,					0.0f									)
	EF_FLOAT(		"Reflective Lifetime In Seconds",		flReflectiveLifetimeInSeconds,		0.0f									)
	EF_FLOAT(		"Param1",								fParam1,							0.0f									)
	EF_FLOAT(		"Param2",								fParam2,							0.0f									)
	EF_LINK_IARR(	"Weapon Location",						pnWeaponInventoryLocations,			DATATABLE_INVLOCIDX,					)
	EF_LINK_IARR(	"Fallback Skills",						pnFallbackSkills,					DATATABLE_SKILLS,						)
	EF_DICT_INT(	"player input override",				nPlayerInputOverride,				sgtPlayerInputOverrides					)
	EF_DICT_INT(	"activator key",						eActivatorKey,						sgtSkillActivatorKeys					)
	EF_LINK_INT(	"activate mode",						nActivateMode,						DATATABLE_UNITMODES						)
	EF_LINK_INT(	"activate skill",						nActivateSkill,						DATATABLE_SKILLS						)
	EF_LINK_INT(	"Skill On Pulse",						nPulseSkill,						DATATABLE_SKILLS						)	
	EF_LINK_IARR(	"unitevent trigger",					nUnitEventTrigger,					DATATABLE_UNIT_EVENT_TYPES				)	
	EF_CODE(		"unitevent trigger chance script",		codeUnitEventTriggerChance													)
	EF_INT(			"activate priority",					nActivatePriority,					0										)
	EF_STRING(		"activator",							szActivator																	)
	EF_FLAG(		"",										dwFlags,							SKILL_FLAG_LOADED						)
	EF_FLAG(		"Uses Weapon",							dwFlags,							SKILL_FLAG_USES_WEAPON					)
	EF_FLAG(		"Weapon is Required",					dwFlags,							SKILL_FLAG_WEAPON_IS_REQUIRED			)
	EF_FLAG(		"Uses Weapon Skill",					dwFlags,							SKILL_FLAG_USES_WEAPON_SKILL			)
	EF_FLAG(		"Use Weapon Targeting",					dwFlags,							SKILL_FLAG_USES_WEAPON_TARGETING		)
	EF_FLAG(		"use weapon cooldown",					dwFlags,							SKILL_FLAG_USES_WEAPON_COOLDOWN			)
	EF_FLAG(		"cooldown unit instead of weapon",		dwFlags,							SKILL_FLAG_COOLDOWN_UNIT_INSTEAD_OF_WEAPON	)
	EF_FLAG(		"use weapon icon",						dwFlags,							SKILL_FLAG_USES_WEAPON_ICON				)
	EF_FLAG(		"use all weapons",						dwFlags,							SKILL_FLAG_USE_ALL_WEAPONS				)
	EF_FLAG(		"combine weapon damage",				dwFlags,							SKILL_FLAG_COMBINE_WEAPON_DAMAGE		)
	EF_FLAG(		"average combined damage",				dwFlags,							SKILL_FLAG_AVERAGE_COMBINED_DAMAGE		)
	EF_FLAG(		"uses unit firing error",				dwFlags,							SKILL_FLAG_USES_UNIT_FIRING_ERROR		)
	EF_FLAG(		"display firing error",					dwFlags,							SKILL_FLAG_DISPLAY_FIRING_ERROR			)
	EF_FLAG(		"prevent animation cutoff",				dwFlags,							SKILL_FLAG_NO_BLEND						)
	EF_FLAG(		"check LOS",							dwFlags,							SKILL_FLAG_CHECK_LOS					)
	EF_FLAG(		"fire to location",						dwFlags,							SKILL_FLAG_FIRE_TO_LOCATION				)
	EF_FLAG(		"default to player location",			dwFlags,							SKILL_FLAG_FIRE_TO_DEFAULT_UNIT				)
	EF_FLAG(		"test firing cone on start",			dwFlags,							SKILL_FLAG_TEST_FIRING_CONE_ON_START	)
	EF_FLAG(		"find target unit",						dwFlags,							SKILL_FLAG_FIND_TARGET_UNIT				)
	EF_FLAG(		"no low aiming 3rd person",				dwFlags,							SKILL_FLAG_NO_LOW_AIMING_THIRDPERSON	)
	EF_FLAG(		"no high aiming 3rd person",			dwFlags,							SKILL_FLAG_NO_HIGH_AIMING_THIRDPERSON	)
	EF_FLAG(		"can target unit", 						dwFlags,							SKILL_FLAG_CAN_TARGET_UNIT				)
	EF_FLAG(		"must target unit",						dwFlags,							SKILL_FLAG_MUST_TARGET_UNIT				)
	EF_FLAG(		"monster must target unit",				dwFlags,							SKILL_FLAG_MONSTER_MUST_TARGET_UNIT		)
	EF_FLAG(		"must not target unit",					dwFlags,							SKILL_FLAG_MUST_NOT_TARGET_UNIT			)
	EF_FLAG(		"cannot retarget",						dwFlags,							SKILL_FLAG_CANNOT_RETARGET				)
	EF_FLAG(		"ui uses target", 						dwFlags,							SKILL_FLAG_UI_USES_TARGET				)
	EF_FLAG(		"target dead",							dwFlags,							SKILL_FLAG_TARGET_DEAD					)
	EF_FLAG(		"target only dying",					dwFlags,							SKILL_FLAG_TARGET_ONLY_DYING			)
	EF_FLAG(		"target dying on start",				dwFlags,							SKILL_FLAG_TARGET_DYING_ON_START		)
	EF_FLAG(		"target dead and alive",				dwFlags,							SKILL_FLAG_TARGET_DEAD_AND_ALIVE		)
	EF_FLAG(		"target dying after start",				dwFlags,							SKILL_FLAG_TARGET_DYING_AFTER_START		)
	EF_FLAG(		"target selectable dead",				dwFlags,							SKILL_FLAG_TARGET_SELECTABLE_DEAD		)
	EF_FLAG(		"target friend",						dwFlags,							SKILL_FLAG_TARGET_FRIEND				)
	EF_FLAG(		"target enemy",							dwFlags,							SKILL_FLAG_TARGET_ENEMY					)
	EF_FLAG(		"target container",						dwFlags,							SKILL_FLAG_TARGET_CONTAINER				)
	EF_FLAG(		"target pets",							dwFlags,							SKILL_FLAG_TARGET_PETS					)
	EF_FLAG(		"don't target pets",					dwFlags,							SKILL_FLAG_DONT_TARGET_PETS				)
	EF_FLAG(		"target objects",						dwFlags,							SKILL_FLAG_ALLOW_OBJECTS				)
	EF_FLAG(		"target destructables",					dwFlags,							SKILL_FLAG_TARGET_DESTRUCTABLES			)
	EF_FLAG(		"don't target destructables",			dwFlags,							SKILL_FLAG_DONT_TARGET_DESTRUCTABLES	)
	EF_FLAG(		"allow untargetables",					dwFlags,							SKILL_FLAG_ALLOW_UNTARGETABLES			)
	EF_FLAG(		"ignore teams",							dwFlags,							SKILL_FLAG_IGNORE_TEAM					)
	EF_FLAG(		"ignore champions",						dwFlags,							SKILL_FLAG_IGNORE_CHAMPIONS				)
	EF_FLAG(		"is melee",								dwFlags,							SKILL_FLAG_IS_MELEE						)
	EF_FLAG(		"is ranged",							dwFlags,							SKILL_FLAG_IS_RANGED					)
	EF_FLAG(		"is spell",								dwFlags,							SKILL_FLAG_IS_SPELL						)
	EF_FLAG(		"do melee item events",					dwFlags,							SKILL_FLAG_DO_MELEE_ITEM_EVENTS			)
	EF_FLAG(		"can delay melee",						dwFlags,							SKILL_FLAG_DELAY_MELEE					)
	EF_FLAG(		"dead can do",							dwFlags,							SKILL_FLAG_DEAD_CAN_DO					)
	EF_FLAG(		"ghost can do",							dwFlags,							SKILL_FLAG_GHOST_CAN_DO					)
	EF_FLAG(		"get hit can do",						dwFlags,							SKILL_FLAG_GETHIT_CAN_DO				)
	EF_FLAG(		"moving can't do",						dwFlags,							SKILL_FLAG_MOVING_CANT_DO				)
	EF_FLAG(		"stopall",								dwFlags,							SKILL_FLAG_USE_STOP_ALL					)
	EF_FLAG(		"stop on dead",							dwFlags,							SKILL_FLAG_STOP_ON_DEAD					)	
	EF_FLAG(		"start on select",						dwFlags,							SKILL_FLAG_START_ON_SELECT				)
	EF_FLAG(		"always selected",						dwFlags,							SKILL_FLAG_ALWAYS_SELECT				)
	EF_FLAG(		"don't select on purchase",				dwFlags,							SKILL_FLAG_DONT_SELECT_ON_PURCHASE		)
	EF_FLAG(		"highlight when selected",				dwFlags,							SKILL_FLAG_HIGHLIGHT_WHEN_SELECTED		)
	EF_FLAG(		"repeat fire",							dwFlags,							SKILL_FLAG_REPEAT_FIRE					)
	EF_FLAG(		"repeat all",							dwFlags,							SKILL_FLAG_REPEAT_ALL					)
	EF_FLAG(		"hold",									dwFlags,							SKILL_FLAG_HOLD							)
	EF_FLAG(		"loop mode",							dwFlags,							SKILL_FLAG_LOOP_MODE					)
	EF_FLAG(		"stop on collide",						dwFlags,							SKILL_FLAG_STOP_ON_COLLIDE				)
	EF_FLAG(		"no idle on stop",						dwFlags,							SKILL_FLAG_NO_IDLE_ON_STOP				)
	EF_FLAG(		"do not clear remove on move states",	dwFlags,							SKILL_FLAG_DO_NOT_CLEAR_REMOVE_ON_MOVE_STATES	)
	EF_FLAG(		"run to target",						dwFlags,							SKILL_FLAG_RUN_TO_RANGE					)
	EF_FLAG(		"Skill Is On",							dwFlags,							SKILL_FLAG_SKILL_IS_ON					)
	EF_FLAG(		"on ground only",						dwFlags,							SKILL_FLAG_ON_GROUND_ONLY				)
	EF_FLAG(		"learnable",							dwFlags,							SKILL_FLAG_LEARNABLE					)
	EF_FLAG(		"hotkeyable",							dwFlags,							SKILL_FLAG_HOTKEYABLE					)
	EF_FLAG(		"can go in mouse button",				dwFlags,							SKILL_FLAG_ALLOW_IN_MOUSE				)
	EF_FLAG(		"can go in left mouse button",			dwFlags,							SKILL_FLAG_ALLOW_IN_LEFT_MOUSE			)
	EF_FLAG(		"allow request",						dwFlags,							SKILL_FLAG_ALLOW_REQUEST				)
	EF_FLAG(		"track request",						dwFlags,							SKILL_FLAG_TRACK_REQUEST				)
	EF_FLAG(		"track metrics",						dwFlags,							SKILL_FLAG_TRACK_METRICS				)
	EF_FLAG(		"uses power",							dwFlags,							SKILL_FLAG_USES_POWER					)
	EF_FLAG(		"drains power",							dwFlags,							SKILL_FLAG_DRAINS_POWER					)	
	EF_FLAG(		"adjust power by level",				dwFlags,							SKILL_FLAG_ADJUST_POWER_BY_LEVEL		)	
	EF_FLAG(		"power cost bounded by max power",		dwFlags,							SKILL_FLAG_POWER_COST_BOUNDED_BY_MAX_POWER	)	
	EF_FLAG(		"save missiles",						dwFlags,							SKILL_FLAG_SAVE_MISSILES				)
	EF_FLAG(		"remove missiles on stop",				dwFlags,							SKILL_FLAG_REMOVE_MISSILES_ON_STOP		)
	EF_FLAG(		"no player skill input",				dwFlags,							SKILL_FLAG_NO_PLAYER_SKILL_INPUT		)
	EF_FLAG(		"no player movement input",				dwFlags,							SKILL_FLAG_NO_PLAYER_MOVEMENT_INPUT		)
	EF_FLAG(		"player stop moving",					dwFlags,							SKILL_FLAG_PLAYER_STOP_MOVING			)
	EF_FLAG(		"don't face target",					dwFlags,							SKILL_FLAG_DONT_FACE_TARGET				)
	EF_FLAG(		"use bone for missile position",		dwFlags,							SKILL_FLAG_USE_BONE_FOR_MISSILE_POSITION )
	EF_FLAG(		"face target position",					dwFlags,							SKILL_FLAG_FACE_TARGET_POSITION			)
	EF_FLAG(		"must face forward",					dwFlags,							SKILL_FLAG_MUST_FACE_FORWARD			)
	EF_FLAG(		"aim at target",						dwFlags,							SKILL_FLAG_AIM_AT_TARGET				)
	EF_FLAG(		"stop holding skills",					dwFlags,							SKILL_FLAG_STOP_HOLDING_SKILLS			)
	EF_FLAG(		"hold other skills",					dwFlags,							SKILL_FLAG_HOLD_OTHER_SKILLS			)
	EF_FLAG(		"prevent other skills",					dwFlags,							SKILL_FLAG_PREVENT_OTHER_SKILLS			)
	EF_FLAG(		"prevent skills by priority",			dwFlags,							SKILL_FLAG_PREVENT_OTHER_SKILLS_BY_PRIORITY		)
	EF_FLAG(		"Use Range",							dwFlags,							SKILL_FLAG_USE_RANGE					)
	EF_FLAG(		"Display Range",						dwFlags,							SKILL_FLAG_DISPLAY_RANGE				)
	EF_FLAG(		"Targets Position",						dwFlags,							SKILL_FLAG_TARGETS_POSITION				)
	EF_FLAG(		"keep target position on request",		dwFlags,							SKILL_FLAG_KEEP_TARGET_POSITION_ON_REQUEST		)
	EF_FLAG(		"constant cooldown",					dwFlags,							SKILL_FLAG_CONSTANT_COOLDOWN			)
	EF_FLAG(		"ignore cooldown on start",				dwFlags,							SKILL_FLAG_IGNORE_COOLDOWN_ON_START		)
	EF_FLAG(		"use unit's cooldown",					dwFlags,							SKILL_FLAG_USE_UNIT_COOLDOWN			)
	EF_FLAG(		"add monsters cooldown",				dwFlags,							SKILL_FLAG_ADD_UNIT_COOLDOWN		)	
	EF_FLAG(		"display cooldown",						dwFlags,							SKILL_FLAG_DISPLAY_COOLDOWN				)
	EF_FLAG(		"target pos in stat",					dwFlags,							SKILL_FLAG_TARGET_POS_IN_STAT			)	
	EF_FLAG(		"Check Range on Start",					dwFlags,							SKILL_FLAG_CHECK_RANGE_ON_START			)
	EF_FLAG(		"Check Melee Range on Start",			dwFlags,							SKILL_FLAG_CHECK_TARGET_IN_MELEE_RANGE_ON_START	)
	EF_FLAG(		"dont use weapon range",				dwFlags,							SKILL_FLAG_DONT_USE_WEAPON_RANGE		)	
	EF_FLAG(		"can start in town",					dwFlags,							SKILL_FLAG_CAN_START_IN_TOWN			)	
	EF_FLAG(		"can start in pvp town",				dwFlags,							SKILL_FLAG_CAN_START_IN_PVP_TOWN		)	
	EF_FLAG(		"Always Test Can Start Skill",			dwFlags,							SKILL_FLAG_ALWAYS_TEST_CAN_START_SKILL	)	
	EF_FLAG(		"must start in portal safe level",		dwFlags,							SKILL_FLAG_MUST_START_IN_PORTAL_SAFE_LEVEL			)	
	EF_FLAG(		"can't start in pvp",					dwFlags,							SKILL_FLAG_CANT_START_IN_PVP			)	
	EF_FLAG(		"can start in rts",						dwFlags,							SKILL_FLAG_CAN_START_IN_RTS_MODE		)	
	EF_FLAG(		"can NOT start in rts level",			dwFlags,							SKILL_FLAG_CAN_NOT_START_IN_RTS_LEVEL	)	
	EF_FLAG(		"can NOT do in hellrift",				dwFlags,							SKILL_FLAG_CAN_NOT_DO_IN_HELLRIFT		)	
	EF_FLAG(		"is aggressive",						dwFlags,							SKILL_FLAG_IS_AGGRESSIVE				)	
	EF_FLAG(		"angers target on execute",				dwFlags,							SKILL_FLAG_ANGERS_TARGET_ON_EXECUTE		)	
	EF_FLAG(		"use mouse skill's targeting",			dwFlags,							SKILL_FLAG_USE_MOUSE_SKILLS_TARGETING	)	
	EF_FLAG(		"server only",							dwFlags,							SKILL_FLAG_SERVER_ONLY					)	
	EF_FLAG(		"client only",							dwFlags,							SKILL_FLAG_CLIENT_ONLY					)	
	EF_FLAG(		"check inventory space",				dwFlags,							SKILL_FLAG_CHECK_INVENTORY_SPACE		)	
	EF_FLAG(		"activator while moving",				dwFlags,							SKILL_FLAG_ACTIVATE_WHILE_MOVING		)	
	EF_FLAG(		"activator ignore moving",				dwFlags,							SKILL_FLAG_ACTIVATE_IGNORE_MOVING		)	
	EF_FLAG(		"ui is red on fallback",				dwFlags,							SKILL_FLAG_UI_IS_RED_ON_FALLBACK		)	
	EF_FLAG(		"impact uses aim bone",					dwFlags,							SKILL_FLAG_IMPACT_USES_AIM_BONE			)	
	EF_FLAG(		"decoy cannot use",						dwFlags,							SKILL_FLAG_DECOY_CANNOT_USE				)
	EF_FLAG(		"do not prefer for mouse",				dwFlags,							SKILL_FLAG_DO_NOT_PREFER_FOR_MOUSE		)
	EF_FLAG(		"Use Holy Aura For Range",				dwFlags,							SKILL_FLAG_USE_HOLY_AURA_FOR_RANGE		)
	EF_FLAG(		"play cooldown on weapon",				dwFlags,							SKILL_FLAG_PLAY_COOLDOWN_ON_WEAPON		)
	EF_FLAG(		"disallow same skill",					dwFlags,							SKILL_FLAG_DISALLOW_SAME_SKILL			)
	EF_FLAG(		"Don't use range for melee events",		dwFlags,							SKILL_FLAG_DONT_USE_RANGE_FOR_MELEE		)
	EF_FLAG(		"Force skill range for melee events",	dwFlags,							SKILL_FLAG_FORCE_SKILL_RANGE_FOR_MELEE	)
	EF_FLAG(		"disabled",								dwFlags,							SKILL_FLAG_DISABLED						)
	EF_FLAG(		"Skill Level From State Target",		dwFlags,							SKILL_FLAG_SKILL_LEVEL_FROM_STATE_TARGET)
	EF_FLAG(		"moves unit",							dwFlags,							SKILL_FLAG_MOVES_UNIT					)
	EF_FLAG(		"selects a melee skill",				dwFlags,							SKILL_FLAG_SELECTS_A_MELEE_SKILL		)
	EF_FLAG(		"verify target",						dwFlags,							SKILL_FLAG_VERIFY_TARGET				)
	EF_FLAG(		"verify target on request",				dwFlags,							SKILL_FLAG_VERIFY_TARGETS_ON_REQUEST	)
	EF_FLAG(		"requires skill level",					dwFlags,							SKILL_FLAG_REQUIRES_SKILL_LEVEL			)
	EF_FLAG(		"disable clientside pathing",			dwFlags,							SKILL_FLAG_DISABLE_CLIENTSIDE_PATHING	)
	EF_FLAG(		"execute requested skill on melee attack",	dwFlags,						SKILL_FLAG_EXECUTE_REQUESTED_SKILL_ON_MELEE_ATTACK	)
	EF_FLAG(		"can be executed from melee attack",	dwFlags,							SKILL_FLAG_CAN_BE_EXECUTED_FROM_MELEE_ATTACK		)
	EF_FLAG(		"don't stop request after execute",		dwFlags,							SKILL_FLAG_DONT_STOP_REQUEST_AFTER_EXECUTE			)
	EF_FLAG(		"lerp camera while on",					dwFlags,							SKILL_FLAG_LERP_CAMERA_WHILE_ON			)
	EF_FLAG(		"force use weapon targeting",			dwFlags,							SKILL_FLAG_FORCE_USE_WEAPON_TARGETING	)
	EF_FLAG(		"don't clear cooldown on death",		dwFlags,							SKILL_FLAG_DONT_CLEAR_COOLDOWN_ON_DEATH	)
	EF_FLAG(		"default shift skill enabled",			dwFlags,							SKILL_FLAG_DEFAULT_SHIFT_SKILL_ENABLED	)
	EF_FLAG(		"shift skill always enabled",			dwFlags,							SKILL_FLAG_SHIFT_SKILL_ALWAYS_ENABLED	)
	EF_FLAG(		"can kill pets for power cost",			dwFlags,							SKILL_FLAG_CAN_KILL_PETS_FOR_POWER_COST	)
	EF_FLAG(		"reduce power max by power cost",		dwFlags,							SKILL_FLAG_REDUCE_POWER_MAX_BY_POWER_COST)
	EF_FLAG(		"Skill From Unit Event Trigger Needs Damage Increment",	dwFlags,			SKILL_FLAG_UNIT_EVENT_TRIGGER_SKILL_NEEDS_DAMAGE_INCREMENT	)
	EF_FLAG(		"AI is busy while on",					dwFlags,							SKILL_FLAG_AI_IS_BUSY_WHEN_ON			)
	EF_FLAG(		"Never set cooldown",					dwFlags,							SKILL_FLAG_NEVER_SET_COOLDOWN			)
	EF_FLAG(		"ignore prevent all skills",			dwFlags,							SKILL_FLAG_IGNORE_PREVENT_ALL_SKILLS	)
	EF_FLAG(		"power on event",						dwFlags,							SKILL_FLAG_POWER_ON_EVENT				)
	EF_FLAG(		"don't cooldown on start",				dwFlags,							SKILL_FLAG_DONT_COOLDOWN_ON_START		)
	EF_FLAG(		"require path to target",				dwFlags,							SKILL_FLAG_REQUIRE_PATH_TO_TARGET		)
	EF_FLAG(		"restart skill on unit reactivate",		dwFlags,							SKILL_FLAG_RESTART_SKILL_ON_REACTIVATE	)
	EF_FLAG(		"subscription required to learn",		dwFlags,							SKILL_FLAG_SUBSCRIPTION_REQUIRED_TO_LEARN)
	EF_FLAG(		"subscription required to use",			dwFlags,							SKILL_FLAG_SUBSCRIPTION_REQUIRED_TO_USE	)
	EF_FLAG(		"force ui to show effect",				dwFlags,							SKILL_FLAG_FORCE_UI_SHOW_EFFECT			)
	EF_FLAG(		"don't ignore owned state",				dwFlags,							SKILL_FLAG_TARGET_DONT_IGNORE_OWNED_STATE)
	EF_FLAG(		"transfer damages to pets",				dwFlags,							SKILL_FLAG_TRANSFER_DAMAGES_TO_PETS		)
	EF_FLAG(		"don't stagger",						dwFlags,							SKILL_FLAG_DONT_STAGGER					)
	EF_FLAG(		"does not actively use weapon",			dwFlags,							SKILL_FLAG_DOES_NOT_ACTIVELY_USE_WEAPON	)
	EF_FLAG(		"do not allow respec",					dwFlags,							SKILL_FLAG_DONT_ALLOW_RESPEC			)
	EF_FLAG(		"Apply SkillGroup Damage Percent",		dwFlags,							SKILL_FLAG_APPLY_SKILLGROUP_DAMAGE_PERCENT	)
	EF_FLAG(		"requires stat unlock to purchase",		dwFlags,							SKILL_FLAG_REQUIRES_STAT_UNLOCK_TO_PURCHASE	)
	EF_STRING(		"description string function",			pszStringFunctions[SKILL_STRING_DESCRIPTION]								)
	EF_STRING(		"effect string function",				pszStringFunctions[SKILL_STRING_EFFECT]										)		
	EF_STRING(		"accumulation string function",			pszStringFunctions[SKILL_STRING_EFFECT_ACCUMULATION]						)
	EF_STRING(		"skill bonus function",					pszStringFunctions[SKILL_STRING_SKILL_BONUSES]								)		
	EF_INT(			"",										pnStringFunctions[SKILL_STRING_DESCRIPTION],	INVALID_ID					)
	EF_INT(			"",										pnStringFunctions[SKILL_STRING_EFFECT],			INVALID_ID					)
	EF_INT(			"",										pnStringFunctions[SKILL_STRING_EFFECT_ACCUMULATION],	INVALID_ID			)
	EF_INT(			"",										pnStringFunctions[SKILL_STRING_SKILL_BONUSES],	INVALID_ID					)
	EF_DICT_INT(	"usage",								eUsage,								sgtSkillUsage							)		
	EF_DICT_INT(	"family",								eFamily,							sgtSkillFamily							)			
	EF_STR_TABLE(	"description string",					pnStrings[SKILL_STRING_DESCRIPTION],APP_GAME_ALL							)	
	EF_STR_TABLE(	"effect string",						pnStrings[SKILL_STRING_EFFECT],		APP_GAME_ALL							)	
	EF_STR_TABLE(	"accumulation string",					pnStrings[SKILL_STRING_EFFECT_ACCUMULATION],	APP_GAME_ALL				)	
	EF_STR_TABLE(	"skill bonus string",					pnStrings[SKILL_STRING_SKILL_BONUSES],			APP_GAME_ALL				)	
	EF_STR_TABLE(	"string after required weapon",			pnStrings[SKILL_STRING_AFTER_REQUIRED_WEAPON],	APP_GAME_ALL				)	
	EF_LINK_IARR(   "skills to Accumulate(3)",				nSkillsToAccumulate,				DATATABLE_SKILLS						)
	EF_LINK_IARR(	"linked level skill",					pnLinkedLevelSkill,					DATATABLE_SKILLS						)	
	EF_LINK_INT(	"skill parent",							nSkillParent,						DATATABLE_SKILLS						)
	EF_LINK_INT(	"Field Missile",						nFieldMissile,						DATATABLE_MISSILES						)
	EF_LINK_INT(	"unlock purchase item",					nUnlockPurchaseItem,				DATATABLE_ITEMS							)
	EXCEL_SET_NAMEFIELD("Display Name");	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "skill")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")	
	EXCEL_ADD_ANCESTOR(DATATABLE_ACHIEVEMENTS)
	EXCEL_ADD_ANCESTOR(DATATABLE_SKILLS)
	EXCEL_ADD_ANCESTOR(DATATABLE_RECIPECATEGORIES)
	EXCEL_ADD_ANCESTOR(DATATABLE_RECIPEPROPERTIES)
	EXCEL_ADD_ANCESTOR(DATATABLE_SKILL_STATS)	
	EXCEL_SET_POSTPROCESS_TABLE(ExcelSkillsFuncPostProcess)
	EXCEL_SET_POSTPROCESS_ALL(ExcelSkillsPostProcessAll)
	EXCEL_SET_ROWFREE(SkillDataFreeRow)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SKILLS, SKILL_DATA, APP_GAME_ALL,	EXCEL_TABLE_PRIVATE, "skills")
	EXCEL_TABLE_ADD_FILE("skills", APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skills_player", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skills_player2", APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skills_achievements", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skills_crafting", APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skills_monster", APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skills_weapon", APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skills_misc", APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skills_perks", APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE)
EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(SKILL_EVENT_TYPE_DATA)
	EF_STRING(		"name",									pszName																		)
	EF_STRING(		"param desc",							szParamStrings[0]															)
	EF_STRING(		"param desc1",							szParamStrings[1]															)
	EF_STRING(		"param desc2",							szParamStrings[2]															)
	EF_STRING(		"param desc3",							szParamStrings[3]															)
	EF_LINK_TABLE(	"attachment table",						eAttachedTable																)
	EF_INT(			"param contains count",					nParamContainsCount,				-1										)
	EF_BOOL(		"does not require table entry",			bDoesNotRequireTableEntry,			FALSE									)
	EF_BOOL(		"apply skill stats",					bApplySkillStats,					FALSE									)
	EF_BOOL(		"uses attachment",						bUsesAttachment,					FALSE									)
	EF_BOOL(		"uses bones",							bUsesBones,							FALSE									)
	EF_BOOL(		"uses bone weights",					bUsesBoneWeights,					FALSE									)
	EF_BOOL(		"server only",							bServerOnly,						FALSE									)
	EF_BOOL(		"client only",							bClientOnly,						FALSE									)
	EF_BOOL(		"needs duration",						bNeedsDuration,						FALSE									)
	EF_BOOL(		"aiming event",							bAimingEvent,						FALSE									)
	EF_BOOL(		"is melee",								bIsMelee,							FALSE									)
	EF_BOOL(		"is ranged",							bIsRanged,							FALSE									)
	EF_BOOL(		"is spell",								bIsSpell,							FALSE									)
	EF_BOOL(		"subskill inherit",						bSubSkillInherit,					FALSE									)
	EF_BOOL(		"convert degrees to dot",				bConvertParamFromDegreesToDot,		FALSE									)
	EF_BOOL(		"uses target index",					bUsesTargetIndex,					FALSE									)
	EF_BOOL(		"tests its condition",					bTestsItsCondition,					FALSE									)
	EF_BOOL(		"uses lasers",							bUsesLasers,						FALSE									)
	EF_BOOL(		"uses missiles",						bUsesMissiles,						FALSE									)
	EF_BOOL(		"params are used in skill string",		bParamsUsedInSkillString,			FALSE									)
	EF_BOOL(		"can multi bullets",					bCanMultiBullets,					FALSE									)
	EF_BOOL(		"starts cooling and power cost",		bStartCoolingAndPowerCost,			FALSE									)
	EF_BOOL(		"consumes item",						bConsumeItem,						FALSE									)
	EF_BOOL(		"check pet power cost",					bCheckPetPowerCost,					FALSE									)
	EF_BOOL(		"causes attack event",					bCausesAttackEvent,					FALSE									)
	EF_STRING(		"event handler",						szEventHandler																)	
	EF_INT(			"event string function",				nEventStringFunction,				-1										)
	EF_FLAG(		"uses Laser Turns",						dwFlagsUsed,						0,				FALSE					)
	EF_FLAG(		"uses Requires Target",					dwFlagsUsed,						1,				FALSE					)
	EF_FLAG(		"uses Force New",						dwFlagsUsed,						2,				FALSE					)
	EF_FLAG(		"uses Laser Seeks Surfaces",			dwFlagsUsed,						3,				FALSE					)
	EF_FLAG(		"uses Face Target",						dwFlagsUsed,						4,				FALSE					)
	EF_FLAG(		"uses Use Unit Target",					dwFlagsUsed,						5,				FALSE					)
	EF_FLAG(		"uses Use Event Offset",				dwFlagsUsed,						6,				FALSE					)
	EF_FLAG(		"uses Loop",							dwFlagsUsed,						7,				FALSE					)
	EF_FLAG(		"uses Use Event Offset Absolute",		dwFlagsUsed,						8,				FALSE					)
	EF_FLAG(		"uses Place On Target",					dwFlagsUsed,						9,				FALSE					)
	EF_FLAG(		"",										dwFlagsUsed,						10,				TRUE					)
	EF_FLAG(		"uses Transfer Stats",					dwFlagsUsed,						11,				FALSE					)
	EF_FLAG(		"uses When Target In Range",			dwFlagsUsed,						12,				FALSE					)
	EF_FLAG(		"uses Add to Center",					dwFlagsUsed,						13,				FALSE					)
	EF_FLAG(		"uses 360 targeting",					dwFlagsUsed,						14,				FALSE					)	
	EF_FLAG(		"uses place on skill target",			dwFlagsUsed,						15,				FALSE					)	
	EF_FLAG(		"uses Use AI Target",					dwFlagsUsed,						16,				FALSE					)	
	EF_FLAG(		"uses Use Offhand Weapon",				dwFlagsUsed,						17,				FALSE					)	
	EF_FLAG(		"uses Float",							dwFlagsUsed,						18,				FALSE					)	
	EF_FLAG(		"uses Don't Validate Target",			dwFlagsUsed,						19,				FALSE					)	
	EF_FLAG(		"uses Random Firing Direction",			dwFlagsUsed,						20,				FALSE					)	
	EF_FLAG(		"uses autoaim projectile",				dwFlagsUsed,						21,				FALSE					)	
	EF_FLAG(		"uses Target Weapon",					dwFlagsUsed,						22,				FALSE					)	
	EF_FLAG(		"",										dwFlagsUsed,						23,				TRUE					)
	EF_FLAG(		"",										dwFlagsUsed,						24,				TRUE					)
	EF_FLAG(		"uses Use Holy Radius for Range",		dwFlagsUsed,						25,				FALSE					)
	EF_FLAG(		"",										dwFlagsUsed,						26,				TRUE					)
	EF_FLAG(		"",										dwFlagsUsed,						27,				TRUE					)
	EF_FLAG(		"",										dwFlagsUsed,						28,				TRUE					)
	EF_FLAG(		"uses Laser Attacks Location",			dwFlagsUsed,						29,				FALSE					)
	EF_FLAG(		"uses At Next Cooldown",				dwFlagsUsed,						30,				FALSE					)
	EF_FLAG(		"uses Aim with Weapon",					dwFlagsUsed,						31,				FALSE					)
	EF_FLAG(		"uses Aim with Weapon Zero",			dwFlagsUsed2,						0,				FALSE					)
	EF_FLAG(		"",										dwFlagsUsed2,						1,				TRUE					)
	EF_FLAG(		"",										dwFlagsUsed2,						2,				TRUE					)
	EF_FLAG(		"",										dwFlagsUsed2,						3,				TRUE					)
	EF_FLAG(		"",										dwFlagsUsed2,						4,				TRUE					)
	EF_FLAG(		"uses Use Ultimate Owner",				dwFlagsUsed2,						5,				FALSE					)
	EF_FLAG(		"",										dwFlagsUsed2,						6,				TRUE					)
	EF_FLAG(		"",										dwFlagsUsed2,						7,				TRUE					)
	EF_FLAG(		"uses Include in UI Count",				dwFlagsUsed2,						8,				FALSE					)
	EF_FLAG(		"",										dwFlagsUsed2,						9,				TRUE					)
	EF_FLAG(		"",										dwFlagsUsed2,						10,				FALSE					)
	EF_FLAG(		"uses Laser Don't Target Unit",			dwFlagsUsed2,						11,				FALSE					)
	EF_FLAG(		"uses Don't Execute Stats",				dwFlagsUsed2,						12,				FALSE					)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_POSTPROCESS_TABLE(SkillEventTypeExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SKILLEVENTTYPES, SKILL_EVENT_TYPE_DATA, APP_GAME_ALL,	EXCEL_TABLE_SHARED, "skilleventtypes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// skill_levels.txt
EXCEL_STRUCT_DEF(SKILL_LEVELS)
	EF_INT(			"level",								nLevel,												-1						)		
	EF_INT(			"base damage",							nSkillLevelValues[KSkillLevel_DMG],					-1						)
	EF_INT(			"armor",								nSkillLevelValues[KSkillLevel_Armor],				-1						)
	EF_INT(			"attack speed",							nSkillLevelValues[KSkillLevel_AttackSpeed],			-1						)
	EF_INT(			"tohit",								nSkillLevelValues[KSkillLevel_ToHit],				-1						)
	EF_INT(			"percent damage",						nSkillLevelValues[KSkillLevel_PercentDamage],		-1						)
	EF_INT(			"tier",									nSkillLevelValues[KSkillLevel_Tier],				0						)
	EF_INT(			"crafting tier",						nSkillLevelValues[KSkillLevel_CraftingTier],				0						)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SKILL_LEVELS,	SKILL_LEVELS, APP_GAME_TUGBOAT,	EXCEL_TABLE_PRIVATE, "skill_levels") EXCEL_TABLE_END
//----------------------------------------------------------------------------
// skillstats.txt
EXCEL_STRUCT_DEF(SKILL_STATS)
	EF_STRING(		"Name",									szName																		)	
	EF_LINK_INT(	"skill name",							nSkillId,											DATATABLE_SKILLS		)	
	EF_LINK_INT(	"Links To",								nLinksTo,											DATATABLE_SKILL_STATS	)	
	EF_INT(			"Max Columns",							nUsingXColumns,										0						)
	EF_INT(			"Percent Mod By Tree Investment ( 0-1000 )",		nSkillModValueByTreeInvestment,			0						)
	EF_INT(			"PVP % mod",										nSkillModValueForPVP,					-1						)
	EF_INT(			"Skill Level 1",						nSkillStatValues[0],								0						)
	EF_INT(			"Skill Level 2",						nSkillStatValues[1],								0						)
	EF_INT(			"Skill Level 3",						nSkillStatValues[2],								0						)
	EF_INT(			"Skill Level 4",						nSkillStatValues[3],								0						)
	EF_INT(			"Skill Level 5",						nSkillStatValues[4],								0						)
	EF_INT(			"Skill Level 6",						nSkillStatValues[5],								0						)
	EF_INT(			"Skill Level 7",						nSkillStatValues[6],								0						)
	EF_INT(			"Skill Level 8",						nSkillStatValues[7],								0						)
	EF_INT(			"Skill Level 9",						nSkillStatValues[8],								0						)
	EF_INT(			"Skill Level 10",						nSkillStatValues[9],								0						)
	EF_INT(			"Skill Level 11",						nSkillStatValues[10],								0						)
	EF_INT(			"Skill Level 12",						nSkillStatValues[11],								0						)
	EF_INT(			"Skill Level 13",						nSkillStatValues[12],								0						)
	EF_INT(			"Skill Level 14",						nSkillStatValues[13],								0						)
	EF_INT(			"Skill Level 15",						nSkillStatValues[14],								0						)
	EF_INT(			"Skill Level 16",						nSkillStatValues[15],								0						)
	EF_INT(			"Skill Level 17",						nSkillStatValues[16],								0						)
	EF_INT(			"Skill Level 18",						nSkillStatValues[17],								0						)
	EF_INT(			"Skill Level 19",						nSkillStatValues[18],								0						)
	EF_INT(			"Skill Level 20",						nSkillStatValues[19],								0						)
	EF_INT(			"Skill Level 21",						nSkillStatValues[20],								0						)
	EF_INT(			"Skill Level 22",						nSkillStatValues[21],								0						)
	EF_INT(			"Skill Level 23",						nSkillStatValues[22],								0						)
	EF_INT(			"Skill Level 24",						nSkillStatValues[23],								0						)
	EF_INT(			"Skill Level 25",						nSkillStatValues[24],								0						)
	EF_INT(			"Skill Level 26",						nSkillStatValues[25],								0						)
	EF_INT(			"Skill Level 27",						nSkillStatValues[26],								0						)
	EF_INT(			"Skill Level 28",						nSkillStatValues[27],								0						)
	EF_INT(			"Skill Level 29",						nSkillStatValues[28],								0						)
	EF_INT(			"Skill Level 30",						nSkillStatValues[29],								0						)
	
	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SKILL_STATS,	SKILL_STATS, APP_GAME_TUGBOAT,	EXCEL_TABLE_PRIVATE, "skillstats") 
	EXCEL_TABLE_ADD_FILE("skillstats", APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skillstats_player", APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skillstats_player2", APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("skillstats_crafting", APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE)	
	EXCEL_TABLE_ADD_FILE("heraldry_stats", APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE)
EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(ITEM_LOOK_GROUP_DATA)
	EF_STRING(		"look group",							szName																		)
	EF_ONECC(		"code",									bCode																		)
	EXCEL_SET_MAXROWS(255)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "look group");
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code");
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ITEM_LOOK_GROUPS,	ITEM_LOOK_GROUP_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "item_look_groups") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(AFFIX_TYPE_DATA)
	EF_STRING(		"affixtype",							szName																		)
	EF_LINK_INT(	"namecolor",							nNameColor,							DATATABLE_FONTCOLORS					)
	EF_LINK_INT(	"downgrade",							nDowngradeType,						DATATABLE_AFFIXTYPES					)
	EF_BOOL(		"required",								bRequired																	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "affixtype");
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_AFFIXTYPES, AFFIX_TYPE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "affixtypes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(RARENAME_DATA)
	EF_STRING(		"name",									szName																		)
	EF_FOURCC(		"code",									dwCode																		)
	EF_BOOL(		"suffix",								bSuffix																		)
	EF_INT(			"level",								nLevel																		)
	EF_LINK_UTYPE(	"types",								nAllowTypes																	)
	EF_CODE(		"cond",									codeRareNameCond															)
	EXCEL_SET_INDEX(0, 0, "suffix", "level", "code")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_RARENAMES, RARENAME_DATA,	APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "rarenames") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT dictAffixStyle[] =
{
	{ "",		AFFIX_STYLE_INVALID	},
	{ "stat",	AFFIX_STYLE_STAT	},
	{ "proc",	AFFIX_STYLE_PROC	},
	{ NULL,		AFFIX_STYLE_INVALID	},
};
EXCEL_STRUCT_DEF(AFFIX_DATA)
	EF_DSTRING(		"affix",								szName																		)
	EF_BOOL(		"always apply",							bAlwaysApply																)	
	EF_STR_TABLE(	"quality name string",					nStringName[ANT_QUALITY],			APP_GAME_ALL							)
	EF_STR_TABLE(	"set name string",						nStringName[ANT_SET],				APP_GAME_ALL							)	
	EF_STR_TABLE(	"magic name string",					nStringName[ANT_MAGIC],				APP_GAME_ALL							)
	EF_STR_TABLE(	"replace name string",					nStringName[ANT_REPLACEMENT],		APP_GAME_ALL							)
	EF_INT(			"dom",									nDominance																	)
	EF_LINK_INT(	"namecolor",							nNameColor,							DATATABLE_FONTCOLORS					)
	EF_LINK_INT(	"gridcolor",							nGridColor,							DATATABLE_FONTCOLORS					)
	EF_LINK_INT(	"look group",							nLookGroup,							DATATABLE_ITEM_LOOK_GROUPS				)
	EF_FOURCC(		"code",									dwCode																		)
	EF_LINK_IARR(	"affixtype",							nAffixType,							DATATABLE_AFFIXTYPES					)	
	EF_BOOL(		"suffix",								bIsSuffix																	)	
//	EF_STRINT(		"group",								nGroup																		)
	EF_LINK_INT(	"group",								nGroup,								DATATABLE_AFFIX_GROUPS					)
	EF_DICT_INT(	"style",								eStyle,								dictAffixStyle							)
	EF_INT(			"use when augmenting",					bUseWhenAugementing,				1										)
	EF_INT(			"spawn",								bSpawn																		)
	EF_INT(			"min level",							nMinLevel																	)
	EF_INT(			"max level",							nMaxLevel																	)
	EF_LINK_UTYPE(	"allow types",							nAllowTypes																	)
	EF_CODE(		"cond",									codeAffixCond																)
	EF_CODE(		"weight",								codeWeight																	)
	EF_INT(			"weight luck",							nLuckModWeight																)
	EF_LINK_INT(	"color set",							nColorSet,							DATATABLE_COLORSETS						)
	EF_INT(			"color set priority",					nColorSetPriority,					INVALID_ID								)
	EF_CODE(		"buy price mult",						codeBuyPriceMult															)
	EF_CODE(		"buy price add",						codeBuyPriceAdd																)
	EF_CODE(		"sell price mult",						codeSellPriceMult															)
	EF_CODE(		"sell price add",						codeSellPriceAdd															)
	EF_LINK_INT(	"state",								nState,								DATATABLE_STATES						)
	EF_BOOL(		"save state",							bSaveState																	)
	EF_INT(			"item level",							nItemLevel																	)	
	EF_CODE(		"property1",							codeProperty[0]																)
	EF_CODE(		"prop1cond",							codePropCond[0]																)
	EF_CODE(		"property2",							codeProperty[1]																)
	EF_CODE(		"prop2cond",							codePropCond[1]																)
	EF_CODE(		"property3",							codeProperty[2]																)
	EF_CODE(		"prop3cond",							codePropCond[2]																)
	EF_CODE(		"property4",							codeProperty[3]																)
	EF_CODE(		"prop4cond",							codePropCond[3]																)
	EF_CODE(		"property5",							codeProperty[4]																)
	EF_CODE(		"prop5cond",							codePropCond[4]																)
	EF_CODE(		"property6",							codeProperty[5]																)
	EF_CODE(		"prop6cond",							codePropCond[5]																)
	EF_STR_TABLE(	"flavortext",							nFlavorText,						APP_GAME_ALL							)
	EF_LINK_INT(	"only on items requiring unit type",	nOnlyOnItemsRequiringUnitType,		DATATABLE_UNITTYPES						)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "affix")
	EXCEL_SET_INDEX(1, 0, "affixtype", "code")
	EXCEL_SET_INDEX(2, EXCEL_INDEX_WARN_DUPLICATE, "code")
	EXCEL_SET_POSTPROCESS_TABLE(AffixExcelPostProcess)
//	EXCEL_TABLE_SORT(affixes)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_AFFIXES, AFFIX_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "affixes") 
	EXCEL_TABLE_ADD_FILE("affixes", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("affixes_monsters", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(AFFIX_GROUP)
	EF_DSTRING(		"Name",									szName																		)
	EF_INT(			"Weight",								nWeight																		)	
	EF_LINK_INT(	"Parent",								nParentGroupID,						DATATABLE_AFFIX_GROUPS					)
	EF_LINK_INT(	"Only Unit Type",						nUnitType,							DATATABLE_UNITTYPES						)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(AffixGroupsExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_AFFIX_GROUPS, AFFIX_GROUP, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "affix_groups") 
EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(FACTION_DEFINITION)
	EF_STRING(		"Name",									szName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EF_STR_TABLE(	"DisplayString",						nDisplayString,						APP_GAME_ALL							)	
	EF_LINK_INT(	"Unit Type Start Standing 1",			tFactionInit[0].nUnitType,			DATATABLE_UNITTYPES						)		
	EF_LINK_INT(	"Level Def Start Standing 1",			tFactionInit[0].nLevelDef,			DATATABLE_LEVEL							)
	EF_LINK_INT(	"Start Standing 1",						tFactionInit[0].nFactionStanding,	DATATABLE_FACTION_STANDING				)		
	EF_LINK_INT(	"Unit Type Start Standing 2",			tFactionInit[1].nUnitType,			DATATABLE_UNITTYPES						)		
	EF_LINK_INT(	"Level Def Start Standing 2",			tFactionInit[1].nLevelDef,			DATATABLE_LEVEL							)	
	EF_LINK_INT(	"Start Standing 2",						tFactionInit[1].nFactionStanding,	DATATABLE_FACTION_STANDING				)		
	EF_LINK_INT(	"Unit Type Start Standing 3",			tFactionInit[2].nUnitType,			DATATABLE_UNITTYPES						)		
	EF_LINK_INT(	"Level Def Start Standing 3",			tFactionInit[2].nLevelDef,			DATATABLE_LEVEL							)	
	EF_LINK_INT(	"Start Standing 3",						tFactionInit[2].nFactionStanding,	DATATABLE_FACTION_STANDING				)		
	EF_LINK_INT(	"Unit Type Start Standing 4",			tFactionInit[3].nUnitType,			DATATABLE_UNITTYPES						)		
	EF_LINK_INT(	"Level Def Start Standing 4",			tFactionInit[3].nLevelDef,			DATATABLE_LEVEL							)	
	EF_LINK_INT(	"Start Standing 4",						tFactionInit[3].nFactionStanding,	DATATABLE_FACTION_STANDING				)		
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_TABLE(FactionExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_FACTION, FACTION_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "faction") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT sgtFactionStandingMood[] =
{
	{ "",			FSM_MOOD_NEUTRAL },
	{ "Bad",		FSM_MOOD_BAD },
	{ "Neutral",	FSM_MOOD_NEUTRAL },
	{ "Good",		FSM_MOOD_GOOD },
	{ NULL,			FSM_MOOD_NEUTRAL},
};
EXCEL_STRUCT_DEF(FACTION_STANDING_DEFINITION)
	EF_STRING(		"Name",									szName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EF_STR_TABLE(	"DisplayString",						nDisplayString,						APP_GAME_ALL							)		
	EF_STR_TABLE(	"DisplayStringNumbers",					nDisplayStringNumbers,				APP_GAME_ALL							)		
	EF_INT(			"MinScore",								nMinScore																	)
	EF_INT(			"MaxScore",								nMaxScore																	)
	EF_DICT_INT(	"Mood",									eMood,								sgtFactionStandingMood					)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_TABLE(FactionStandingExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_FACTION_STANDING,	FACTION_STANDING_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "faction_standing") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(PROC_DEFINITION)
	EF_STRING(	"Name",										szName																		)
	EF_TWOCC(	"Code",										wCode																		)
	EF_BOOL(	"VerticalCenter",							bVerticalCenter																)
	EF_FLOAT(	"CooldownInSeconds",						flCooldownInSeconds															)
	EF_FLAG(	"TargetInstrumentOwner",					dwFlags,							PROC_TARGET_INSTRUMENT_OWNER_BIT		)	
	EF_FLOAT(	"DelayedProcTimeInSeconds",					flDelayedProcTimeInSeconds													)
	EF_LINK_INT("Skill1",									tProcSkills[0].nSkill,				DATATABLE_SKILLS						)
	EF_INT(		"Skill1Param",								tProcSkills[0].nParam,				0										)
	EF_LINK_INT("Skill2",									tProcSkills[1].nSkill,				DATATABLE_SKILLS						)
	EF_INT(		"Skill2Param",								tProcSkills[1].nParam,				0										)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_PROCS, PROC_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "procs") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// movies.txt
EXCEL_STRUCT_DEF(MOVIE_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_STRING(		"filename",								pszFileName																	)
	EF_STRING(		"lowres filename",						pszFileNameLowres															)
	EF_DICT_INT_ARR("Audio Languages",						eLanguages,							sgtLanguageDict							)
	EF_DICT_INT_ARR("Region List",							eRegions,							sgtRegionDict							)		
	EF_BOOL(		"Loops",								bLoops																		)
	EF_BOOL(		"Use In Credits",						bUseInCredits																)
	EF_BOOL(		"Widescreen Only",						bWidescreen																	)	
	EF_BOOL(		"can pause",							bCanPause																	)
	EF_BOOL(		"no skip",								bNoSkip																		)
	EF_BOOL(		"no skip first time",					bNoSkipFirstTime															)
	EF_BOOL(		"put in main pakfile",					bPutInMainPak																)
	EF_BOOL(		"Force Allow High Quality",				bForceAllowHQ																)
	EF_BOOL(		"Only With Censored SKU",				bOnlyWithCensoredSKU														)
	EF_BOOL(		"Disallow in Censored SKU",				bDisallowInCensoredSKU														)
	EF_FLOAT(		"Credit Movie Display Time In Seconds",	flCreditMovieDisplayTimeInSeconds											)		
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MOVIES, MOVIE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "movies") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// movielists.txt
EXCEL_STRUCT_DEF(MOVIE_LIST_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_IARR(	"list",									nMovieList[0],						DATATABLE_MOVIES						)
	EF_LINK_IARR(	"list2",								nMovieList[1],						DATATABLE_MOVIES						)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MOVIELISTS, MOVIE_LIST_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "movielists") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// movie_subtitles.txt
EXCEL_STRUCT_DEF(MOVIE_SUBTITLE_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_IARR(	"movie",								nMovie,								DATATABLE_MOVIES						)
	EF_DICT_INT(	"language",								eLanguage,							sgtLanguageDict							)	
	EF_STR_TABLE(	"string",								nString,							APP_GAME_ALL							)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelMovieSubtitlesPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MOVIE_SUBTITLES, MOVIE_SUBTITLE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "movie_subtitles") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// soundvcas.txt
EXCEL_STRUCT_DEF(SOUND_VCA)
	EF_STRING(		"Name",									pszName																		)
	EF_INT(			"Volume",								nVolume,							1										)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SOUNDVCAS, SOUND_VCA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "soundvcas") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// soundvcasets.txt
EXCEL_STRUCT_DEF(SOUND_VCA_SET)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"VCA1",									nVCAs[0],							DATATABLE_SOUNDVCAS						)
	EF_LINK_INT(	"VCA2",									nVCAs[1],							DATATABLE_SOUNDVCAS						)
	EF_LINK_INT(	"VCA3",									nVCAs[2],							DATATABLE_SOUNDVCAS						)
	EF_LINK_INT(	"VCA4",									nVCAs[3],							DATATABLE_SOUNDVCAS						)
	EF_LINK_INT(	"VCA5",									nVCAs[4],							DATATABLE_SOUNDVCAS						)
	EF_LINK_INT(	"VCA6",									nVCAs[5],							DATATABLE_SOUNDVCAS						)
	EF_LINK_INT(	"VCA7",									nVCAs[6],							DATATABLE_SOUNDVCAS						)
	EF_LINK_INT(	"VCA8",									nVCAs[7],							DATATABLE_SOUNDVCAS						)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SOUNDVCASETS,	SOUND_VCA_SET, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "soundvcasets") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// soundbuses.txt
STR_DICT sgSoundBusUserControlDict[] =
{
	{ "",		INVALID_ID		},
	{ "sfx",	SBUC_SFX		},
	{ "mx",		SBUC_MX			},
	{ "uix",	SBUC_UIX		},
	{ NULL,					0								},
};

EXCEL_STRUCT_DEF(SOUND_BUS)
	EF_STRING(		"Name",									pszName																		)
	EF_INT(			"Volume",								nVolume,							1										)
	EF_INT(			"",										nVolumeWithVCAs,					1										)
	EF_STRING(		"Effects",								pszEffectDefinitionName														)
	EF_LINK_INT(	"sends to",								nSendsTo,							DATATABLE_SOUNDBUSES					)
	EF_LINK_IARR(	"VCAs",									nVCAs,								DATATABLE_SOUNDVCAS						)
	EF_DICT_INT(	"User Control",							nUserControl,						sgSoundBusUserControlDict				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SOUNDBUSES, SOUND_BUS, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "soundbuses") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT soundrollofftype[] =
{
	{ "",			SOUND_ROLLOFF_INVALID		},
	{ "log",		SOUND_ROLLOFF_LOGARITHMIC	},
	{ "linear",		SOUND_ROLLOFF_LINEAR		},
	{ "inverse",	SOUND_ROLLOFF_INVERSE		},
	{ NULL,			SOUND_ROLLOFF_INVALID		},
};
EXCEL_STRUCT_DEF(SOUND_OVERRIDE_DATA)
	EF_STRING(		"Name",									pszName,																	)
	EF_INT(			"Volume",								nVolume,							 1										)
	EF_FLOAT(		"MinRange",								fMinRange,							-1.0f									)
	EF_FLOAT(		"MaxRange",								fMaxRange,							-1.0f									)
	EF_DICT_INT(	"rolloff type",							nRolloffType,						soundrollofftype						)
	EF_INT(			"Reverb Send",							nReverbSend,						 1										)
	EF_INT(			"FreqVar",								nFrequencyVariation,				-1										)
	EF_INT(			"VolVar",								nVolumeVariation,					-1										)
	EF_INT(			"MaxWithinRad",							nMaxSoundsWithinRadius,				-1										)
	EF_FLOAT(		"Radius",								fRadius,							-1.0f									)
	EF_FLOAT(		"Fadeout Time",							fFadeoutTime,						-1.0f									)
	EF_INT(			"",										nFadeoutTime,						-1										)
	EF_FLOAT(		"Fadein Time",							fFadeinTime,						-1.0f									)
	EF_INT(			"",										nFadeinTime,						-1										)
	EF_FLOAT(		"Front Send",							fFrontSend,							-1.0f									)
	EF_FLOAT(		"Center Send",							fCenterSend,						-1.0f									)
	EF_FLOAT(		"Rear Send",							fRearSend,							-1.0f									)
	EF_FLOAT(		"Side Send",							fSideSend,							-1.0f									)
	EF_FLOAT(		"LFE Send",								fLFESend,							-1.0f									)
	EF_LINK_INT(	"Bus",									nBus,								DATATABLE_SOUNDBUSES					)
	EF_LINK_IARR(	"VCAs",									nVCAs,								DATATABLE_SOUNDVCAS						)
	EF_STRING(		"Effects",								pszEffectDefinitionName														)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_ROW(ExcelSoundOverridesPostProcessRow)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SOUNDOVERRIDES, SOUND_OVERRIDE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "soundoverrides") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT soundpicktype[] =
{
	{ "",			SOUND_PICKTYPE_RAND	},
	{ "rand",		SOUND_PICKTYPE_RAND	},
	{ "all",		SOUND_PICKTYPE_ALL	},
	{ NULL,			SOUND_PICKTYPE_RAND	},
};
EXCEL_STRUCT_DEF(SOUND_GROUP)
	EF_DSTRING(		"Name",									pszName																		)
	EF_DICT_INT(	"picktype",								nPickType,							soundpicktype							)
	EF_DICT_INT(	"language",								eLanguage,							sgtLanguageDict							)		
	EF_FLAG(		"nonblock",								dwFlags,							SGF_NONBLOCK_BIT						)
	EF_FLAG(		"stream",								dwFlags,							SGF_STREAM_BIT							)
	EF_INT(			"Volume",								nVolume,							0										)
	EF_FLAG(		"software",								dwFlags,							SGF_SOFTWARE_BIT						)
	EF_FLOAT(		"MinRange",								fMinRange,							0.0f									)
	EF_FLOAT(		"MaxRange",								fMaxRange,							30.0f									)
	EF_DICT_INT(	"rolloff type",							nRolloffType,						soundrollofftype						)
	EF_INT(			"Reverb Send",							nReverbSend,						0										)
	EF_FLAG(		"3D",									dwFlags,							SGF_3D_BIT,	TRUE						)
	EF_FLAG(		"Loops",								dwFlags,							SGF_LOOPS_BIT							)
	EF_FLAG(		"CanCutOff",							dwFlags,							SGF_CANCUTOFF_BIT						)
	EF_FLAG(		"Highlander",							dwFlags,							SGF_HIGHLANDER_BIT						)
	EF_FLAG(		"Group Highlander",						dwFlags,							SGF_GROUP_HIGHLANDER_BIT				)
	EF_FLAG(		"Head Relative",						dwFlags,							SGF_HEAD_RELATIVE_BIT					)
	EF_FLAG(		"Don't Randomize Start",				dwFlags,							SGF_DONT_RANDOMIZE_START_POINT_BIT		)
	EF_FLAG(		"Don't Crossfade Variations",			dwFlags,							SGF_DONT_CROSSFADE_VARIATIONS_BIT		)
	EF_FLAG(		"Load at Startup",						dwFlags,							SGF_LOAD_AT_STARTUP_BIT					)
	EF_INT(			"FreqVar",								nFrequencyVariation,				-1										)
	EF_INT(			"VolVar",								nVolumeVariation,					-1										)
	EF_INT(			"MaxWithinRad",							nMaxSoundsWithinRadius,				-1										)
	EF_FLOAT(		"Radius",								fRadius																		)
	EF_FLOAT(		"Fadeout Time",							fFadeoutTime																)
	EF_INT(			"",										nFadeoutTime																)
	EF_FLOAT(		"Fadein Time",							fFadeinTime																	)
	EF_INT(			"",										nFadeinTime																	)
	EF_FLOAT(		"Front Send",							fFrontSend,							1.0f									)
	EF_FLOAT(		"Center Send",							fCenterSend																	)
	EF_FLOAT(		"Rear Send",							fRearSend																	)
	EF_FLOAT(		"Side Send",							fSideSend																	)
	EF_FLOAT(		"LFE Send",								fLFESend,							1.0f									)
	EF_INT(			"Hardware Priority",					nHardwarePriority,					128										)
	EF_INT(			"Game Priority",						nGamePriority,						16										)
	EF_FLAG(		"Is Music",								dwFlags,							SGF_IS_MUSIC_BIT,	FALSE				)
	EF_LINK_INT(	"Music Ref",							nMusicReference,					DATATABLE_MUSIC_REF						)
	EF_LINK_INT(	"Stinger Ref",							nMusicStingerReference,				DATATABLE_MUSICSTINGERS					)
	EF_LINK_INT(	"Mix State",							nMixState,							DATATABLE_SOUND_MIXSTATES				)
	EF_LINK_INT(	"Bus",									nBus,								DATATABLE_SOUNDBUSES					)
	EF_LINK_IARR(	"VCAs",									nVCAs,								DATATABLE_SOUNDVCAS						)
	EF_DSTRING(		"Effects",								pszEffectDefinitionName														)
	EF_DSTRING(		"ADSR",									pszADSRDefinitionName														)
	EF_DSTRING(		"Directory",							pszFolder																	)
	EF_DSTRING(		"Extension",							pszExtension																)
	EF_DSTRING(		"LQ Extension",							pszLQExtension																)
	EF_DSTRING(		"",										pSoundData[0].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[1].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[2].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[3].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[4].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[5].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[6].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[7].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[8].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[9].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[10].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[11].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[12].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_DSTRING(		"",										pSoundData[13].pszFilename													) EXCEL_SET_FLAG(EXCEL_FIELD_DO_NOT_TRANSLATE)
	EF_PARSE(		"Filename1",							pSoundData[0],						SoundExcelParse							)
	EF_PARSE(		"Filename2",							pSoundData[1],						SoundExcelParse							)
	EF_PARSE(		"Filename3",							pSoundData[2],						SoundExcelParse							)
	EF_PARSE(		"Filename4",							pSoundData[3],						SoundExcelParse							)
	EF_PARSE(		"Filename5",							pSoundData[4],						SoundExcelParse							)
	EF_PARSE(		"Filename6",							pSoundData[5],						SoundExcelParse							)
	EF_PARSE(		"Filename7",							pSoundData[6],						SoundExcelParse							)
	EF_PARSE(		"Filename8",							pSoundData[7],						SoundExcelParse							)
	EF_PARSE(		"Filename9",							pSoundData[8],						SoundExcelParse							)
	EF_PARSE(		"Filename10",							pSoundData[9],						SoundExcelParse							)
	EF_PARSE(		"Filename11",							pSoundData[10],						SoundExcelParse							)
	EF_PARSE(		"Filename12",							pSoundData[11],						SoundExcelParse							)
	EF_PARSE(		"Filename13",							pSoundData[12],						SoundExcelParse							)
	EF_PARSE(		"Filename14",							pSoundData[13],						SoundExcelParse							)
	EF_FLOAT(		"Weight1",								pSoundData[0].fWeight,				1.0f									)
	EF_FLOAT(		"Weight2",								pSoundData[1].fWeight,				1.0f									)
	EF_FLOAT(		"Weight3",								pSoundData[2].fWeight,				1.0f									)
	EF_FLOAT(		"Weight4",								pSoundData[3].fWeight,				1.0f									)
	EF_FLOAT(		"Weight5",								pSoundData[4].fWeight,				1.0f									)
	EF_FLOAT(		"Weight6",								pSoundData[5].fWeight,				1.0f									)
	EF_FLOAT(		"Weight7",								pSoundData[6].fWeight,				1.0f									)
	EF_FLOAT(		"Weight8",								pSoundData[7].fWeight,				1.0f									)
	EF_FLOAT(		"Weight9",								pSoundData[8].fWeight,				1.0f									)
	EF_FLOAT(		"Weight10",								pSoundData[9].fWeight,				1.0f									)
	EF_FLOAT(		"Weight11",								pSoundData[10].fWeight,				1.0f									)
	EF_FLOAT(		"Weight12",								pSoundData[11].fWeight,				1.0f									)
	EF_FLOAT(		"Weight13",								pSoundData[12].fWeight,				1.0f									)
	EF_FLOAT(		"Weight14",								pSoundData[13].fWeight,				1.0f									)
	EF_FLAG(		"",										dwFlags,							SGF_LOADED_BIT,	FALSE					)
	EF_INT(			"",										nTotalSounds,						0										)
	EF_INT(			"",										nTable,								0										)
	EF_FLAG(		"",										dwFlags,							SGF_FLAGGED_FOR_LOAD_BIT				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_ROW(ExcelSoundsPostProcessRow)
	EXCEL_SET_POSTPROCESS_TABLE(ExcelSoundsPostProcess)
	EXCEL_SET_POSTPROCESS_ALL(ExcelSoundsPostProcessAll)
	EXCEL_SET_SELFINDEX()
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SOUNDS, SOUND_GROUP, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "sounds")
	EXCEL_TABLE_ADD_FILE("soundsmisc", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSMISC)
	EXCEL_TABLE_ADD_FILE("soundsmonsters", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSMONSTERS)
	EXCEL_TABLE_ADD_FILE("soundsweapons", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSWEAPONS)
	EXCEL_TABLE_ADD_FILE("soundsvox", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSVOX)
	EXCEL_TABLE_ADD_FILE("soundsui", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSUI)
	EXCEL_TABLE_ADD_FILE("soundsbg", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSBG)
	EXCEL_TABLE_ADD_FILE("soundsmusic", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSMUSIC)
	EXCEL_TABLE_ADD_FILE("soundsfootsteps", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSFOOTSTEPS)
	EXCEL_TABLE_ADD_FILE("soundsskills", APP_GAME_ALL, EXCEL_TABLE_PRIVATE, SOUNDTABLE_SOUNDSSKILLS)
EXCEL_TABLE_END

//----------------------------------------------------------------------------
// soundmixstatevalues.txt
EXCEL_STRUCT_DEF(SOUND_MIXSTATE_VALUE)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"Bus",									nBus,								DATATABLE_SOUNDBUSES					)
	EF_INT(			"Bus Volume",							nBusVolume,							1										)
	EF_FLOAT(		"",										fBusVolume,							-1.0f									)
	EF_STRING(		"Effects",								pszEffectDefinitionName														)
	EF_BOOL(		"Absolute Volume",						bAbsoluteVolume																)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelSoundMixStateValuesPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SOUND_MIXSTATE_VALUES, SOUND_MIXSTATE_VALUE, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "soundmixstatevalues") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// soundmixstatesets.txt
EXCEL_STRUCT_DEF(SOUND_MIXSTATE_SET)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_IARR(	"values",								nMixStateValues,					DATATABLE_SOUND_MIXSTATE_VALUES			)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SOUND_MIXSTATE_SETS, SOUND_MIXSTATE_SET, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "soundmixstatesets") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// soundmixstates.txt
EXCEL_STRUCT_DEF(SOUND_MIXSTATE)
	EF_STRING(		"Name",									pszName																		)
	EF_INT(			"priority",								nPriority,							0										)
	EF_LINK_IARR(	"sets",									nMixStateSets,						DATATABLE_SOUND_MIXSTATE_SETS			)
	EF_LINK_IARR(	"values",								nMixStateValues,					DATATABLE_SOUND_MIXSTATE_VALUES			)
	EF_FLOAT(		"fade in time in seconds",				fFadeInTimeSeconds,					0.0f									)
	EF_INT(			"",										nFadeInTimeTicks,					0										)
	EF_FLOAT(		"fade out time in seconds",				fFadeOutTimeSeconds,				0.0f									)
	EF_INT(			"",										nFadeOutTimeTicks,					0										)
	EF_STRING(		"reverb override",						pszReverbFileName															)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_ALL(ExcelSoundMixStatesPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SOUND_MIXSTATES, SOUND_MIXSTATE, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "soundmixstates") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(DIALOG_DATA)
	EF_STRING(		"Name",									szName																		)
	EF_LINK_INT(	"Sound",								nSound,								DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Mode",									eUnitMode,							DATATABLE_UNITMODES						)
	EF_LINK_INT(	"Movielist On Finished",				nMovieListOnFinished,				DATATABLE_MOVIELISTS					)	
	EF_STR_TABLE(	"String All",							tDialogs[ DIALOG_APP_ALL ].nString,			APP_GAME_ALL,					)	
	EF_STR_TABLE(	"String Hellgate",						tDialogs[ DIALOG_APP_HELLGATE ].nString,	APP_GAME_HELLGATE,				)
	EF_STR_TABLE(	"String Mythos",						tDialogs[ DIALOG_APP_MYTHOS ].nString,		APP_GAME_TUGBOAT,				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_DIALOG, DIALOG_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "dialog")
	EXCEL_TABLE_ADD_FILE("dialog_quests", APP_GAME_ALL, EXCEL_TABLE_SHARED)
EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(BACKGROUND_SOUND_DATA_3D)
	EF_STRING(		"Name",									pszName																		)
	EF_BOOL(		"Front",								bAllowableQuadrants[QUADRANT_FRONT]											)
	EF_BOOL(		"Left",									bAllowableQuadrants[QUADRANT_LEFT]											)
	EF_BOOL(		"Right",								bAllowableQuadrants[QUADRANT_RIGHT]											)
	EF_BOOL(		"Back",									bAllowableQuadrants[QUADRANT_BACK]											)
	EF_INT(			"",										nAllowableQuadrants															)
	EF_FLOAT(		"Min Volume",							fMinVolume,							0.0f									)
	EF_FLOAT(		"Max Volume",							fMaxVolume,							100.0f									)
	EF_FLOAT(		"Min Interset Delay",					fMinIntersetDelay,					0.5f									)
	EF_FLOAT(		"Max Interset Delay",					fMaxIntersetDelay,					1.0f									)
	EF_INT(			"Min Set Count",						nMinSetCount,						1										)
	EF_INT(			"Max Set Count",						nMaxSetCount,						5										)
	EF_FLOAT(		"Min Intraset Delay",					fMinIntrasetDelay,					5.0f									)
	EF_FLOAT(		"Max Intraset Delay",					fMaxIntrasetDelay,					10.0f									)
	EF_FLOAT(		"Set Chance",							fSetChance,							50.0f									)
	EF_LINK_INT(	"Sound",								nSoundGroup,						DATATABLE_SOUNDS						)	
	EXCEL_SET_INDEX(0, 0, "Name")
	EXCEL_SET_POSTPROCESS_ROW(ExcelBGSounds3DPostProcessRow)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_BACKGROUNDSOUNDS3D, BACKGROUND_SOUND_DATA_3D, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "backgroundsounds3d") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(BACKGROUND_SOUND_DATA_2D)
	EF_STRING(		"Name",									pszName																		)
	EF_FLOAT(		"Min Volume",							fMinVolume,							0.0f									)
	EF_FLOAT(		"Max Volume",							fMaxVolume,							100.0f									)
	EF_FLOAT(		"Silent Chance",						fSilentChance,						50.0f									)
	EF_FLOAT(		"Min Play Time",						fMinPlayTime,						10.0f									)
	EF_FLOAT(		"Max Play Time",						fMaxPlayTime,						30.0f									)
	EF_FLOAT(		"Min Silent Time",						fMinSilentTime,						10.0f									)
	EF_FLOAT(		"Max Silent Time",						fMaxSilentTime,						30.0f									)
	EF_FLOAT(		"Fade In",								fFadeIn,							1.0f									)
	EF_FLOAT(		"Fade Out",								fFadeOut,							1.0f									)
	EF_DSTRING(		"ADSR",									pszADSRDefinition															)
	EF_LINK_INT(	"Sound",								nSoundGroup,						DATATABLE_SOUNDS						)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_ROW(ExcelBGSounds2DPostProcessRow)
	EXCEL_SET_POSTPROCESS_TABLE(ExcelBGSounds2DPostProcessTable)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_BACKGROUNDSOUNDS2D, BACKGROUND_SOUND_DATA_2D, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "backgroundsounds2d") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(BACKGROUND_SOUND_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_IARR(	"2D Sounds",							n2DSounds,							DATATABLE_BACKGROUNDSOUNDS2D			)
	EF_LINK_IARR(	"3D Sounds",							n3DSounds,							DATATABLE_BACKGROUNDSOUNDS3D			)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_BACKGROUNDSOUNDS,	BACKGROUND_SOUND_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "backgroundsounds") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT stingertype[] =
{
	{ "",			STINGER_TYPE_LAYER		},
	{ "layer",		STINGER_TYPE_LAYER		},
	{ "interrupt",	STINGER_TYPE_INTERRUPT	},
	{ "breakdown",	STINGER_TYPE_INTERRUPT	},
	{ "override",	STINGER_TYPE_OVERRIDE	},
	{ NULL,			STINGER_TYPE_LAYER		},
};

EXCEL_STRUCT_DEF(MUSIC_STINGER_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_DICT_INT(	"type",									eType,								stingertype								)
	EF_INT(			"Fade Out Beats",						nFadeOutBeats,						1										)
	EF_INT(			"Fade In Beats",						nFadeInBeats,						1										)
	EF_INT(			"Fade In Delay Beats",					nFadeInDelayBeats															)
	EF_INT(			"Fade Out Delay Beats",					nFadeOutDelayBeats															)
	EF_INT(			"",										nReferenceCount																)
	EF_INT(			"Intro Beats",							nIntroBeats																	)
	EF_INT(			"Outro Beats",							nOutroBeats																	)
	EF_LINK_INT(	"SoundGroup",							nSoundGroup,						DATATABLE_SOUNDS						)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelMusicStingersPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MUSICSTINGERS, MUSIC_STINGER_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "musicstingers") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(MUSIC_GROOVE_LEVEL_STINGER_SET)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"Music Ref",							nMusicRef,							DATATABLE_MUSIC_REF						)
	EF_IARR(		"stinger1 measures",					pStingers[0].pnBeatsAllowed,		-1										)
	EF_LINK_INT(	"stinger1",								pStingers[0].nStinger,				DATATABLE_MUSICSTINGERS					)
	EF_FLOAT(		"stinger1 Chance",						pStingers[0].fStingerChance,		1.0f									)
	EF_IARR(		"stinger2 measures",					pStingers[1].pnBeatsAllowed,		-1										)
	EF_LINK_INT(	"stinger2",								pStingers[1].nStinger,				DATATABLE_MUSICSTINGERS					)
	EF_FLOAT(		"stinger2 Chance",						pStingers[1].fStingerChance,		1.0f									)
	EF_IARR(		"stinger3 measures",					pStingers[2].pnBeatsAllowed,		-1										)
	EF_LINK_INT(	"stinger3",								pStingers[2].nStinger,				DATATABLE_MUSICSTINGERS					)
	EF_FLOAT(		"stinger3 Chance",						pStingers[2].fStingerChance,		1.0f									)
	EF_IARR(		"stinger4 measures",					pStingers[3].pnBeatsAllowed,		-1										)
	EF_LINK_INT(	"stinger4",								pStingers[3].nStinger,				DATATABLE_MUSICSTINGERS					)
	EF_FLOAT(		"stinger4 Chance",						pStingers[3].fStingerChance,		1.0f									)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelMusicStingerSetsPostProcess)
	EXCEL_SET_POSTPROCESS_ALL(ExcelMusicStingerSetsPostProcessAll)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MUSICSTINGERSETS, MUSIC_GROOVE_LEVEL_STINGER_SET, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "musicstingersets") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(MUSIC_GROOVE_LEVEL_TYPE)
	EF_STRING(		"Name",									pszName																		)
	EF_BOOL(		"Required",								bRequired,							FALSE									)
	EF_LINK_INT(	"Alternative",							nAlternativeGrooveLevelType,		DATATABLE_MUSICGROOVELEVELTYPES			)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelMusicGrooveLevelTypesPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MUSICGROOVELEVELTYPES, MUSIC_GROOVE_LEVEL_TYPE, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "musicgrooveleveltypes") EXCEL_TABLE_END


//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(MUSIC_GROOVE_LEVEL_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"Groove Level Type",					nGrooveLevelType,					DATATABLE_MUSICGROOVELEVELTYPES			)
	EF_LINK_INT(	"Music Ref",							nMusicRef,							DATATABLE_MUSIC_REF						)
	EF_INT(			"Track Number",							nTrackNumber,						1										)
	EF_INT(			"Volume",								nVolume,							0										)
	EF_BOOL(		"Is Action",							bIsAction,							FALSE									)
	EF_INT(			"Min Playtime in measures",				nMinPlayTime,						1										)
	EF_INT(			"Max Playtime in measures",				nMaxPlayTime,						0										)
	EF_LINK_INT(	"Groove Level after Max Playtime",		nGrooveLevelAfterMaxPlaytime,		DATATABLE_MUSICGROOVELEVELS				)
	EF_LINK_INT(	"Mix State",							nMixState,							DATATABLE_SOUND_MIXSTATES				)
	EF_LINK_IARR(	"play stinger sets",					nPlayStingerSet,					DATATABLE_MUSICSTINGERSETS				)
	EF_LINK_INT(	"transition stinger set from action",	nTransitionStingerSetFromAction,	DATATABLE_MUSICSTINGERSETS				)
	EF_LINK_INT(	"transition stinger set from ambient",	nTransitionStingerSetFromAmbient,	DATATABLE_MUSICSTINGERSETS				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelMusicGrooveLevelsPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MUSICGROOVELEVELS, MUSIC_GROOVE_LEVEL_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "musicgroovelevels") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(MUSIC_CONDITION_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"Music Ref",							nMusicRef,							DATATABLE_MUSIC_REF						)
	EF_PARSE(		"evaluate1",							pConditions[0],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove1",								pConditions[0].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance1",						pConditions[0].nPercentChance,		100										)
	EF_CODE(		"character class1",						pConditions[0].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code1",					pConditions[0].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition1",							pConditions[0].codeCondition												)
	EF_PARSE(		"evaluate2",							pConditions[1],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove2",								pConditions[1].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance2",						pConditions[1].nPercentChance,		100										)
	EF_CODE(		"character class2",						pConditions[1].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code2",					pConditions[1].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition2",							pConditions[1].codeCondition												)
	EF_PARSE(		"evaluate3",							pConditions[2],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove3",								pConditions[2].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance3",						pConditions[2].nPercentChance,		100										)
	EF_CODE(		"character class3",						pConditions[2].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code3",					pConditions[2].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition3",							pConditions[2].codeCondition												)
	EF_PARSE(		"evaluate4",							pConditions[3],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove4",								pConditions[3].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance4",						pConditions[3].nPercentChance,		100										)
	EF_CODE(		"character class4",						pConditions[3].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code4",					pConditions[3].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition4",							pConditions[3].codeCondition												)
	EF_PARSE(		"evaluate5",							pConditions[4],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove5",								pConditions[4].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance5",						pConditions[4].nPercentChance,		100										)
	EF_CODE(		"character class5",						pConditions[4].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code5",					pConditions[4].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition5",							pConditions[4].codeCondition												)
	EF_PARSE(		"evaluate6",							pConditions[5],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove6",								pConditions[5].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance6",						pConditions[5].nPercentChance,		100										)
	EF_CODE(		"character class6",						pConditions[5].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code6",					pConditions[5].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition6",							pConditions[5].codeCondition												)
	EF_PARSE(		"evaluate7",							pConditions[6],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove7",								pConditions[6].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance7",						pConditions[6].nPercentChance,		100										)
	EF_CODE(		"character class7",						pConditions[6].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code7",					pConditions[6].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition7",							pConditions[6].codeCondition												)
	EF_PARSE(		"evaluate8",							pConditions[7],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove8",								pConditions[7].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance8",						pConditions[7].nPercentChance,		100										)
	EF_CODE(		"character class8",						pConditions[7].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code8",					pConditions[7].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition8",							pConditions[7].codeCondition												)
	EF_PARSE(		"evaluate9",							pConditions[8],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove9",								pConditions[8].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance9",						pConditions[8].nPercentChance,		100										)
	EF_CODE(		"character class9",						pConditions[8].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code9",					pConditions[8].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition9",							pConditions[8].codeCondition												)
	EF_PARSE(		"evaluate10",							pConditions[9],						MusicConditionExcelParse				)
	EF_LINK_INT(	"groove10",								pConditions[9].nRequiredGrooveLevelType,DATATABLE_MUSICGROOVELEVELTYPES		)
	EF_INT(			"percent chance10",						pConditions[9].nPercentChance,		100										)
	EF_CODE(		"character class10",					pConditions[9].codeCharacterClass											)
	EF_LINK_INT(	"Use Condition Code10",					pConditions[9].nUseConditionCode,	DATATABLE_MUSICCONDITIONS				)
	EF_CODE(		"condition10",							pConditions[9].codeCondition												)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_ALL(ExcelMusicConditionsPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MUSICCONDITIONS, MUSIC_CONDITION_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "musicconditions") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(MUSIC_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_INT(			"",										nIndex																		)
	EF_LINK_INT(	"Music Ref",							nMusicRef,							DATATABLE_MUSIC_REF						)
	EF_LINK_INT(	"Base Condition",						nBaseCondition,						DATATABLE_MUSICCONDITIONS				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelMusicPostProcess)
	EXCEL_SET_POSTPROCESS_ALL(ExcelMusicPostProcessAll)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MUSIC, MUSIC_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "music") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(MUSIC_REF)
	EF_STRING(		"Name",									pszName																		)
	EF_INT(			"",										nIndex																		)
	EF_FLOAT(		"BPM",									fBeatsPerMinute,					0.0f									)
	EF_FLOAT(		"",										fBeatsPerTick,						0.0f									)
	EF_INT(			"Beats Per Measure",					nBeatsPerMeasure,					0										)
	EF_INT(			"Offset in milliseconds",				nOffset,							0										)
	EF_INT(			"Groove Update in measures",			nGrooveUpdate,						1										)
	EF_LINK_INT(	"Default Groove",						nDefaultGroove,						DATATABLE_MUSICGROOVELEVELS				)
	EF_LINK_INT(	"Off Groove",							nOffGroove,							DATATABLE_MUSICGROOVELEVELS				)
	EF_LINK_INT(	"SoundGroup",							nSoundGroup,						DATATABLE_SOUNDS						)
	EF_INT(			"",										nMeasuresTotal,						-1										)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelMusicRefPostProcess)
	EXCEL_SET_POSTPROCESS_ALL(ExcelMusicRefPostProcessAll)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MUSIC_REF, MUSIC_REF, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "musicref") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(FOOTSTEP_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"Concrete",								nFootstepSounds[0],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Wood",									nFootstepSounds[1],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Metal",								nFootstepSounds[2],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Tile",									nFootstepSounds[3],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Squishy",								nFootstepSounds[4],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Gravel",								nFootstepSounds[5],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Snow",									nFootstepSounds[6],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Dirt",									nFootstepSounds[7],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Water",								nFootstepSounds[8],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Rubble",								nFootstepSounds[9],					DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Car",									nFootstepSounds[10],				DATATABLE_SOUNDS						)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_FOOTSTEPS, FOOTSTEP_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "footsteps") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(QUEST_DICTIONARY_DEFINITION_TUGBOAT)
	EF_TWOCC(		"code",									wCode																		)		
	EF_STR_TABLE(	"string",								nStringDefTableLink,				APP_GAME_TUGBOAT						)			
	EF_STR_TABLE(	"string def",							nStringTableLink,					APP_GAME_TUGBOAT						)			
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_DICTIONARY_FOR_TUGBOAT, QUEST_DICTIONARY_DEFINITION_TUGBOAT, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE,  "questdictionary" ) EXCEL_TABLE_END
//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(QUEST_REWARD_LEVEL_TUGBOAT)
	EF_INT(			"EXP % Min Player Lvl",						nExpMin,						0										)		
	EF_INT(			"EXP % Max Player Lvl",						nExpMax,						0										)		
	EF_INT(			"Gold % Min Player Lvl",					nGoldMin,						0										)		
	EF_INT(			"Gold % Max Player Lvl",					nGoldMax,						0										)		

EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_REWARD_BY_LEVEL_TUGBOAT, QUEST_REWARD_LEVEL_TUGBOAT, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "questreward") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(QUEST_COUNT_TUGBOAT)	
	EF_TWOCC(		"Code",										wCode																	)		
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_TABLE(QuestCountForTugboatExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_COUNT_TUGBOAT, QUEST_COUNT_TUGBOAT, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "questcount") EXCEL_TABLE_END



//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(QUEST_RANDOM_DEFINITION_TUGBOAT)
	EF_STRING(		"Parent Title Random",						szName																	)
	EF_TWOCC(		"Code",										wCode																	)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Parent Title Random")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_RANDOM_FOR_TUGBOAT, QUEST_RANDOM_DEFINITION_TUGBOAT, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "questtitlesrnd") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(QUEST_RANDOM_TASK_DEFINITION_TUGBOAT)
	EF_STRING(		"Quest Task Random",						szName																	)
	EF_TWOCC(		"Code",										wCode																	)	
	EF_LINK_INT(	"Parent Quest Random",						nParentID,						DATATABLE_QUEST_RANDOM_FOR_TUGBOAT		)		
	EF_BOOL(		"Find Level Area In Zone",					bFindRandomLevelAreaInZone												)
	EF_INT(			"Num of Level Areas to Find",				nNumOfLevelAreasToFind,			1										)
	EF_BOOL(		"Can Be Global",							bIsGlobal																)
	EF_BOOL(		"Spawn Item from Boss",						bSpawnFromBoss															)
	EF_BOOL(		"Spawn Item from Chest",					bSpawnFromChest															)
	EF_INT(			"Num of Items To Collect",					nNumOfItemsToCollect,			0										)		
	EF_LINK_IARR(	"Level Areas To Give",						nLevelAreas,					DATATABLE_LEVEL_AREAS					)
	EF_LINK_IARR(	"Items To Choose (25)",						nItemsToCollect,				DATATABLE_ITEMS							)
	EF_LINK_IARR(	"Unit Type Spawn From (25)",				nItemsSpawnFrom,				DATATABLE_UNITTYPES						)
	EF_IARR(		"Random Unit Type Group(25)",				nItemSpawnGroupIndex,			0										)		
	EF_LINK_IARR(	"Random Unit Types Group1(5)",				nItemSpawnFromGroupOfUnitTypes[0],			DATATABLE_UNITTYPES						)
	EF_LINK_IARR(	"Random Unit Types Group2(5)",				nItemSpawnFromGroupOfUnitTypes[1],			DATATABLE_UNITTYPES						)
	EF_LINK_IARR(	"Random Unit Types Group3(5)",				nItemSpawnFromGroupOfUnitTypes[2],			DATATABLE_UNITTYPES						)
	EF_LINK_IARR(	"Random Unit Types Group4(5)",				nItemSpawnFromGroupOfUnitTypes[3],			DATATABLE_UNITTYPES						)
	EF_LINK_IARR(	"Random Unit Types Group5(5)",				nItemSpawnFromGroupOfUnitTypes[4],			DATATABLE_UNITTYPES						)
	EF_IARR(		"Min Items to Collect(25)",					nItemsMinCollect,				0										)		
	EF_IARR(		"Max Items to Collect(25)",					nItemsMaxCollect,				0										)
	EF_IARR(		"Chance Of Spawn(25)",						nItemsChanceToSpawn,			100										)
	EF_LINK_IARR(	"Collect Item From Treasure Class",			nCollectItemFromSpawnClass,		DATATABLE_TREASURE						)
	EF_LINK_IARR(	"Bosses Spawn Class",						nBossSpawnClasses,				DATATABLE_SPAWN_CLASS					)
	EF_BOOL(		"Boss Not Required to Complete",			bBossNotRequiredtoComplete,		0																)
	EF_LINK_IARR(	"Reward Treasure Class(5)",					nRewardOfferings,				DATATABLE_TREASURE						)
	EF_LINK_INT(	"Dialog Intro",								nStrings[KQUEST_STRING_INTRO],			DATATABLE_DIALOG				)
	EF_LINK_INT(	"Dialog Accept",							nStrings[KQUEST_STRING_ACCEPTED],		DATATABLE_DIALOG				)
	EF_LINK_INT(	"Dialog Return",							nStrings[KQUEST_STRING_RETURN],			DATATABLE_DIALOG				)
	EF_LINK_INT(	"Dialog Complete",							nStrings[KQUEST_STRING_COMPLETE],		DATATABLE_DIALOG				)
	EF_STR_TABLE(	"String Quest Title",						nStrings[KQUEST_STRING_TITLE],			APP_GAME_TUGBOAT				)
	EF_STR_TABLE(	"String Task Complete",						nStrings[KQUEST_STRING_TASK_COMPLETE_MSG],	APP_GAME_TUGBOAT			)
	EF_STR_TABLE(	"String Log",								nStrings[KQUEST_STRING_LOG],			APP_GAME_TUGBOAT				)
	EF_STR_TABLE(	"String Log Complete",						nStrings[KQUEST_STRING_LOG_COMPLETE],	APP_GAME_TUGBOAT				)
	EF_STR_TABLE(	"String Log Meet before",					nStrings[KQUEST_STRING_LOG_BEFORE_MEET],	APP_GAME_TUGBOAT			)
	EF_STR_TABLE(	"String Task Complete Msg",					nStrings[KQUEST_STRING_TASK_COMPLETE_MSG],	APP_GAME_TUGBOAT			)
	EF_STR_TABLE(	"String Quest Complete Msg",				nStrings[KQUEST_STRING_QUEST_COMPLETE_MSG],	APP_GAME_TUGBOAT			)	
	EF_IARR(		"duration(min,max)",						nDurationLength,				-1										)
	EF_IARR(		"GoldPCT",									nGoldPCT,						-1										)
	EF_IARR(		"XPPCT",									nXPPCT,							-1										)
	

	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Quest Task Random")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_TABLE(QuestRandomForTugboatExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_RANDOM_TASKS_FOR_TUGBOAT, QUEST_RANDOM_TASK_DEFINITION_TUGBOAT, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "questrnd") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(QUEST_DEFINITION_TUGBOAT)
	EF_STRING(		"Quest Titles",							szName																		)
	EF_TWOCC(		"Code",									wCode																		)	
	EF_LINK_INT(	"Quest Dialog Title",					nQuestTitleID,						DATATABLE_DIALOG						)		
	EF_LINK_INT(	"Quest Dialog Intro",					nQuestDialogIntroID,				DATATABLE_DIALOG						)
	EF_STR_TABLE(	"Quest Complete Text",					nQuestCompleteTextID,				APP_GAME_TUGBOAT						)	
	EF_BOOL(		"QuestIsAlwaysOn",						bQuestAlwaysOn																)
	EF_BOOL(		"Quest Cannot be Abandoned",			bQuestCannnotBeAbandoned													)
	EF_INT(			"QuestStartLevel",						nQuestPlayerStartLevel,				-1										)		
	EF_INT(			"QuestEndLevel",						nQuestPlayerEndLevel,				-1										)
	EF_LINK_IARR(	"States Needed To Start",				nQuestStatesNeeded,					DATATABLE_STATES						)
	EF_LINK_IARR(	"Unittypes Allowed",					nQuestUnitTypeNeeded,				DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"ItemNeeded",							nQuestItemNeededToStart,			DATATABLE_ITEMS							)
	EF_LINK_INT(	"Random Quest Parent",					nQuestRandomID,						DATATABLE_QUEST_RANDOM_FOR_TUGBOAT		)
	EF_LINK_INT(	"Reward Treasure Class",				nQuestRewardOfferClass,				DATATABLE_OFFER							)
	EF_INT(			"Reward XP PCT",						nQuestRewardXP_PCT,					-1										)
	EF_INT(			"Reward Gold PCT",						nQuestRewardGold_PCT,				-1										)
	EF_INT(			"Reward Level",							nQuestRewardLevel,				-1										)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Quest Titles")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_TITLES_FOR_TUGBOAT, QUEST_DEFINITION_TUGBOAT, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "queststitlesfortugboat") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(QUEST_TASK_DEFINITION_TUGBOAT)
	EF_STRING(		"Quest Task Titles",					szName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EF_BOOL(		"do not show quest complete message",	bDoNotShowCompleteMessage,			FALSE									)	
	EF_LINK_INT(	"NPC",									nNPC,								DATATABLE_NPC							)
	EF_STR_TABLE(	"NameStringKey",						nNameStringKey,						APP_GAME_TUGBOAT						)	
	EF_LINK_INT(	"Dialog On Quest Complete",				nQuestDialogOnTaskCompleteAnywhere,	DATATABLE_DIALOG						)
	EF_LINK_INT(	"Quest Dialog",							nQuestDialogID,						DATATABLE_DIALOG						)
	EF_LINK_INT(	"Quest Dialog Come Back",				nQuestDialogComeBackID,				DATATABLE_DIALOG						)	
	EF_LINK_INT(	"Quest Dialog On Completion",			nQuestDialogCompleteID,				DATATABLE_DIALOG						)	
	EF_STR_TABLE(	"Description Before Meeting",			nDescriptionBeforeMeeting,			APP_GAME_TUGBOAT						)	
	EF_STR_TABLE(	"Description",							nDescriptionString,					APP_GAME_TUGBOAT						)	
	EF_STR_TABLE(	"Complete Description",					nCompleteDescriptionString,			APP_GAME_TUGBOAT						)	
	EF_LINK_INT(	"Parent Quest",							nParentQuest,						DATATABLE_QUEST_TITLES_FOR_TUGBOAT		)
	EF_INT(			"Quest Order Index",					nQuestMaskLocal,					0										)		
	EF_LINK_IARR(	"Start on Quests Complete",				nQuestCanStartOnQuestsCom,			DATATABLE_QUEST_TITLES_FOR_TUGBOAT		)
	EF_LINK_IARR(	"Start on Quests Active",				nQuestCanStartOnQuestActive,		DATATABLE_QUEST_TITLES_FOR_TUGBOAT		)
	EF_LINK_IARR(	"Quest Item Names to Collect",			nQuestItemsIDToCollect,				DATATABLE_ITEMS							)
	EF_STR_TABLE(	"Task Complete Text",					nQuestTaskCompleteTextID,			APP_GAME_TUGBOAT						)	
	EF_IARR_STR(	"Quest Item flavored texts",			nQuestItemsFlavoredTextToCollect,	APP_GAME_TUGBOAT						)	
	EF_IARR(		"Quest Item Counts to Collect",			nQuestItemsNumberToCollect,			-1										)		
	EF_IARR(		"Spawn Only For Attacker",				nQuestItemsSpawnUniqueForAttacker,	-1										)		
	EF_CODE(		"Item Validation Script",				pQuestItemValidationCode														)
	EF_LINK_IARR(	"Items Drop In Levels",					nQuestItemsAppearOnLevel,			DATATABLE_LEVEL_AREAS							)	
	EF_LINK_IARR(	"Unittypes to Spawn From",				nUnitTypesToSpawnFrom,				DATATABLE_UNITTYPES						)
	EF_LINK_IARR(	"Monsters To Spawn From",				nUnitMonstersToSpawnFrom,			DATATABLE_MONSTERS						)
	EF_LINK_IARR(	"Monsters Quality",						nUnitQualitiesToSpawnFrom,			DATATABLE_MONSTER_QUALITY				)	
	EF_IARR(		"% Chance From Monsters",				nUnitPercentChanceOfDrop,			-1										)	
	EF_IARR(		"Add Weight On Random Drops",			nQuestItemsWeight,					-1										)	
	EF_LINK_IARR(	"Finish on Quests Complete",			nQuestQuestsToFinish,				DATATABLE_QUEST_TITLES_FOR_TUGBOAT		)
	EF_LINK_IARR(	"Quest Item Rewards",					nQuestRewardItemID,					DATATABLE_ITEMS							)	
	EF_LINK_IARR(	"Quest Item Rewards Choices",			nQuestRewardChoicesItemID,			DATATABLE_ITEMS							)	
	EF_INT(			"Quest Reward Choice Count",			nQuestRewardChoiceCount														)		
	EF_LINK_INT(	"Offer Reward",							nQuestGivesItems,					DATATABLE_OFFER							)	
	/////////////Flags
	EF_FLAG(		"must complete all tasks in dungeon",	dwFlags,							QUESTTB_FLAG_MUST_FINISH_IN_DUNGEON		)	
	
	/////////////recipes	
	EF_LINK_IARR(	"Recipes to Give",						nQuestGiveRecipesOnQuestAccept,		DATATABLE_RECIPES						)
	EF_LINK_IARR(	"Remove Recipes",						nQuestRemoveRecipesAtQuestEnd,		DATATABLE_RECIPES						)

	EF_IARR(		"Remove Items at task complete",		nQuestItemsRemoveAtQuestEnd,		-1										)	
	EF_LINK_IARR(	"Remove Additional Items",				nQuestAdditionalItemsToRemove,		DATATABLE_ITEMS							)	
	EF_IARR(		"Remove Count",							nQuestAdditionalItemsToRemoveCount,	-1										)	
	///////////Level Areas To Award
	EF_LINK_IARR(	"Give Level Ares On Task Accepted",		nQuestLevelAreaOnTaskAccepted,		DATATABLE_LEVEL_AREAS					)
	EF_IARR("Level Areas not removed task is complete",		nQuestLevelAreaOnTaskCompleteNoRemove,	0									)
	EF_LINK_IARR(	"Give Level Areas On Task Complete",	nQuestLevelAreaOnTaskCompleted,		DATATABLE_LEVEL_AREAS					)
	EF_LINK_IARR(	"NPC Gives Level Area",					nQuestLevelAreaNPCToCommunicateWith,	DATATABLE_NPC						)
	EF_LINK_IARR(	"NPC's Level Area to give",				nQuestLevelAreaNPCCommunicate,		DATATABLE_LEVEL_AREAS					)
	///////////Scripts
	EF_CODE(		"Before Quest Starts Script",			pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_BEFORE]						)
	EF_CODE(		"Quest Starts Script",					pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_START]							)
	EF_CODE(		"Quest Ends Script",					pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_COMPLETE]						)
	EF_CODE(		"Quest Talk To NPC",					pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_TALK_NPC]						)
	EF_CODE(		"Quest After Talking To NPC",			pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_AFTER_TALK_NPC]				)
	EF_CODE(		"Quest Mini or Boss Defeated",			pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_BOSS_DEFEATED]					)
	EF_CODE(		"Interact With Quest Unit",				pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_INTERACT_QUESTUNIT]			)
	EF_CODE(		"Quest Ends Script Client",				pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_COMPLETE_CLIENT]				)
	EF_CODE(		"Quest Task Ends",						pQuestCodes_Server[QUEST_WORLD_UPDATE_QSTATE_TASK_COMPLETE]					)	
	///////////Monster Spawn
	EF_LINK_IARR(	"Monsters To Add",						nQuestMonstersIDsToSpawn,			DATATABLE_MONSTERS						)
	EF_LINK_IARR(	"Dungeons To Spawn In",					nQuestDungeonsToSpawnMonsters,		DATATABLE_LEVEL_AREAS					)	
	EF_LINK_IARR(	"Monsters To Create Quality",			nQuestMonstersToSpawnQuality,		DATATABLE_MONSTER_QUALITY				)	
	EF_IARR(		"Levels To Spawn In",					nQuestLevelsToSpawnMonstersIn,		-1										)		
	EF_IARR(		"Number of Monsters in a Pack",			nQuestNumOfMonstersPerPack,			-1										)	
	EF_IARR(		"Number Of Packs To Spawn Per Level",	nQuestPacksCountToSpawn,			-1										)
	EF_IARR_STR(	"Monster Unique Names",					nQuestMonstersUniqueNames,			APP_GAME_TUGBOAT						)
	//////////End Monster Spawn
	//////////Object Spawn
	EF_LINK_IARR(	"Objects To Create",					nQuestObjectIDsToSpawn,				DATATABLE_OBJECTS						)
	EF_LINK_IARR(	"Spawn Item from Object",				nQuestObjectCreateItemIDs,			DATATABLE_ITEMS							)
	EF_IARR_STR(	"Spawn Item Flavored Texts",			nQuestObjectCreateItemFlavoredText,	APP_GAME_TUGBOAT						)
	EF_LINK_IARR(	"Objects Created In Dungeon",			nQuestDungeonsToSpawnObjects,		DATATABLE_LEVEL_AREAS					)	
	EF_LINK_IARR(	"Boss Objects Created In Dungeon",		nQuestDungeonsToSpawnBossObjects,	DATATABLE_LEVEL_AREAS					)	
	EF_LINK_IARR(	"Boss Create (Monster Name)",			nQuestBossIDsToSpawn,				DATATABLE_MONSTERS						)
	EF_LINK_IARR(	"Boss Quality",							nQuestBossQuality,					DATATABLE_MONSTER_QUALITY				)
	EF_LINK_IARR(	"Spawn Item From Boss",					nQuestBossItemDrop,					DATATABLE_ITEMS							)
	EF_IARR_STR(	"Spawn Item Boss Flavored Text",		nQuestBossItemFlavoredText,			APP_GAME_TUGBOAT						)
	EF_IARR_STR(	"Boss Unique Name",						nQuestBossUniqueNames,				APP_GAME_TUGBOAT						)
	EF_IARR(		"Flag Boss Do Not Register To Complete Quest",	nQuestBossNotNeededToCompleteQuest,		0							)
	EF_IARR(		"Flag Boss No Task Description",		nQuestBossNoTaskDescription,		0										)
	EF_IARR(		"Objects Created On Level",				nQuestLevelsToSpawnObjectsIn,		-1										)
	EF_IARR(		"Boss Objects Created On Level",		nQuestLevelsToSpawnBossObjectsIn,	-1										)
	EF_LINK_IARR(	"PlaceBossAtSpawnPoint(UnitType)",		nQuestBossSpawnFromUnitType,		DATATABLE_UNITTYPES						)		
	EF_IARR(		"Bucket to Increment",					nQuestBucketToIncrement														)
	EF_IARR(		"Bucket to Lock Object",				nQuestBucketToLockBy														)
	EF_IARR(		"Bucket Count to Unlock Object",		nQuestBucketToUnLockByCount													)	
	EF_IARR_STR(	"Text when Object is used",				nQuestObjectTextWhenUsed,	APP_GAME_TUGBOAT								)
	EF_IARR(		"Skill to use on Object Use",			nQuestObjectSkillWhenUsed,			-1										)
	EF_LINK_IARR(	"Remove Item from Player Inventory",	nQuestObjectRemoveItemWhenUsed,		DATATABLE_ITEMS							)
	EF_IARR(		"object requires items",					nQuestObjectRequiresItemWhenUsed,	0										)
	
	EF_IARR(		"Give Item directly to Player",			nQuestObjectGiveItemDirectlyToPlayer										)
	//////////End Object Spawn
	//////////NPCs that you have to talk to
	EF_LINK_IARR(	"NPCs Must talk to",					nQuestSecondaryNPCTalkTo,			DATATABLE_NPC							)
	EF_LINK_IARR(	"NPCs Dialog",							nQuestNPCTalkToDialog,				DATATABLE_DIALOG						)
	EF_LINK_IARR(	"NPC's Dialog After Complete",			nQuestNPCTalkQuestCompleteDialog,	DATATABLE_DIALOG						)
	EF_IARR(		"Talk to NPC for Quest Complete",		nQuestNPCTalkToCompleteQuest,		-1										)	
	//////////End NPC talk to
	//////////Quest Markers ( Points of Interest )
	EF_LINK_IARR(	"Monster Markers",						nQuestPointsOfInterestMonsters,				DATATABLE_MONSTERS				)
	EF_LINK_IARR(	"Monster Markers On Tasks Complete",	nQuestPointsOfInterestMonstersOnComplete,	DATATABLE_MONSTERS				)
	EF_LINK_IARR(	"Object Markers",						nQuestPointsOfInterestObjects,				DATATABLE_OBJECTS				)
	EF_LINK_IARR(	"Object Markers on Tasks Complete",		nQuestPointsOfInterestObjectsOnComplete,	DATATABLE_OBJECTS				)
	//////////End Quest Markers ( Points of Interest )
	//////////Items to give
	EF_LINK_IARR(	"items to give",						nQuestGiveItemsOnAccept,			DATATABLE_ITEMS							)
	EF_BOOL(		"do not auto give maps",				bDoNotAutoGiveMaps,					FALSE									)	
	//////////End Items to give
	//////////Themes
	EF_LINK_IARR(	"Theme Dungeons",						m_QuestThemes.nQuestDungeonAreaThemes,			DATATABLE_LEVEL_AREAS		)
	EF_LINK_IARR(	"Themes",								m_QuestThemes.nQuestDungeonThemes,				DATATABLE_LEVEL_THEMES		)
	EF_IARR(		"Theme Levels",							m_QuestThemes.nQuestDungeonThemeLevels,			-1							)	
	EF_IARR(		"ThemeProperty",						m_QuestThemes.nQuestDungeonThemeProperties,		-1							)	
	
	//////////End Themes
	//////////Duration
	EF_INT(			"duration",								nQuestDuration,									-1							)		
	//////////End Duration
	//////////Shows dialog to team mates
	EF_BOOL("Broadcast NPC Dialog to Party",				bQuestNPCDialogToParty,				FALSE									)	
	EF_BOOL("Broadcast Secondary NPC Dialog To Party",		bQuestSecondaryNPCDialogToParty,	FALSE									)	
	//////////End show dialog
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Quest Task Titles")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_TABLE(QuestForTugboatExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, QUEST_TASK_DEFINITION_TUGBOAT, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "questsfortugboat") EXCEL_TABLE_END


//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(RECIPE_PROPERTIES)
EF_STRING(		"Property Name",							szName																		)	
EF_STR_TABLE(	"Description String",						nString,							APP_GAME_ALL							)	
EF_INT(			"default range",							nDefaultRange,											80					)		
EF_INT(			"default min % value",						nDefaultValues[RECIPE_VALUE_PER_PROP_MIN_PERCENT],		0					)		
EF_INT(			"default max % value",						nDefaultValues[RECIPE_VALUE_PER_PROP_MAX_PERCENT],		0					)		
EF_INT(			"default min value",						nDefaultValues[RECIPE_VALUE_PER_PROP_MIN_VALUE],		0					)		
EF_INT(			"default max value",						nDefaultValues[RECIPE_VALUE_PER_PROP_MAX_VALUE],		0					)		
EF_CODE(		"min percent script",						codeValueManipulator[RECIPE_VALUE_PER_PROP_MIN_PERCENT]						) 
EF_CODE(		"max percent script",						codeValueManipulator[RECIPE_VALUE_PER_PROP_MAX_PERCENT]						) 
EF_CODE(		"min value script",							codeValueManipulator[RECIPE_VALUE_PER_PROP_MIN_VALUE]						) 
EF_CODE(		"max value script",							codeValueManipulator[RECIPE_VALUE_PER_PROP_MAX_VALUE]						) 
EF_FLAG(		"flag compute as range",					dwFlags,		RECIPE_PROPERTIES_FLAG_COMPUTE_AS_RANGE						)
EF_FLAG(		"flag ignore default range",				dwFlags,		RECIPE_PROPERTIES_FLAG_IGNORE_DEFAULT_PERCENT_INVESTED		)
EF_TWOCC(		"Code",										wCode																		)	
EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Property Name")
EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_RECIPEPROPERTIES, RECIPE_PROPERTIES,APP_GAME_ALL, EXCEL_TABLE_SHARED, "recipe_properties") EXCEL_TABLE_END


//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(RECIPE_CATEGORY)
EF_STRING(		"CategoryName",							szName																		)	
EF_STR_TABLE(	"Category String",						nString,							APP_GAME_TUGBOAT						)
EF_LINK_IARR(	"Execute Item on Skills( 20 )",			nSkillsToExecuteOnItem,				DATATABLE_SKILLS						)
EF_LINK_INT(	"Recipe Pane",							nRecipePane,						DATATABLE_SKILLTABS								)
EF_CODE(		"Category Level by Script",				codeScripts[RECIPE_CATEGORY_SCRIPT_LEVEL]									)
EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "CategoryName")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_RECIPECATEGORIES, RECIPE_CATEGORY,APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "recipe_categories") EXCEL_TABLE_END


//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(RECIPE_DEFINITION)
	EF_STRING(		"Recipe",								szName																		)		
	
	EF_CODE(		"property script",						codeProperties																) 


	EF_TWOCC(		"Code",									wCode																		)	
	EF_BOOL(		"Cube Recipe",							bCube																		)
	EF_BOOL(		"Always Known",							bAlwaysKnown																)
	EF_BOOL(		"Don't Require Exact Ingredients",		bDontRequireExactIngredients												)
	EF_BOOL(		"Allow In Random Single Use",			bAllowInRandomSingleUse														)
	EF_BOOL(		"Remove On Load",						bRemoveOnLoad																)	
	EF_INT(			"Weight",								nWeight																		)
	EF_LINK_IARR(	"recipe categories required(0 to 3 )",			nRecipeCategoriesNecessaryToCraft,				DATATABLE_RECIPECATEGORIES		)
	EF_IARR(		"recipe category levels required(0 to 3 )",		nRecipeCategoriesLevelsNecessaryToCraft,		-1								)

	//price
	EF_INT(			"base cost",							nBaseCost																	)

	//spawn with
	EF_LINK_IARR(	"spawn for only unit type",				nSpawnForUnittypes,										DATATABLE_UNITTYPES	)
	EF_LINK_IARR(	"spawn for only monster class",			nSpawnForMonsterClass,									DATATABLE_MONSTERS	)

	//flags
	EF_FLAG(		"spawn at merchant",					pdwFlags,							RECIPE_FLAG_CAN_SPAWN_AT_MERCHENT		)
	EF_FLAG(		"can spawn",							pdwFlags,							RECIPE_FLAG_CAN_SPAWN					)
	EF_FLAG(		"can be learned",						pdwFlags,							RECIPE_FLAG_CAN_BE_LEARNED				)
	//if the item can spawn than at what levels
	EF_IARR(		"spawn level (min,max)",				nSpawnLevels,						-1										)


	EF_BOOL(		"Result Quality Modifies Ingredient Quantity",		bResultQualityModifiesIngredientQuantity						)
	EF_LINK_IARR(	"Craft Result",							nRewardItemClass,					DATATABLE_ITEMS							)
	EF_LINK_IARR(	"Treasure Result",						nTreasureClassResult,				DATATABLE_TREASURE						)	
	EF_INT(			"Experience Earned",					nExperience																	)	
	EF_INT(			"Gold Reward",							nGold																		)	
	EF_LINK_INT(	"Must Place In Inv Slot",				nInvLocIngredients,					DATATABLE_INVLOCIDX						)						
	
	EF_LINK_INT(	"Ingredient 1 - Item Class",			tIngredientDef[ 0 ].nItemClass,		DATATABLE_ITEMS							)
	EF_LINK_INT(	"Ingredient 1 - Unit Type",				tIngredientDef[ 0 ].nUnitType,		DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"Ingredient 1 - Item Quality",			tIngredientDef[ 0 ].nItemQuality,	DATATABLE_ITEM_QUALITY					)
	EF_INT(			"Ingredient 1 - Min Quantity",			tIngredientDef[ 0 ].nQuantityMin											)
	EF_INT(			"Ingredient 1 - Max Quantity",			tIngredientDef[ 0 ].nQuantityMax											)

	EF_LINK_INT(	"Ingredient 2 - Item Class",			tIngredientDef[ 1 ].nItemClass,		DATATABLE_ITEMS							)
	EF_LINK_INT(	"Ingredient 2 - Unit Type",				tIngredientDef[ 1 ].nUnitType,		DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"Ingredient 2 - Item Quality",			tIngredientDef[ 1 ].nItemQuality,	DATATABLE_ITEM_QUALITY					)
	EF_INT(			"Ingredient 2 - Min Quantity",			tIngredientDef[ 1 ].nQuantityMin											)
	EF_INT(			"Ingredient 2 - Max Quantity",			tIngredientDef[ 1 ].nQuantityMax											)

	EF_LINK_INT(	"Ingredient 3 - Item Class",			tIngredientDef[ 2 ].nItemClass,		DATATABLE_ITEMS							)
	EF_LINK_INT(	"Ingredient 3 - Unit Type",				tIngredientDef[ 2 ].nUnitType,		DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"Ingredient 3 - Item Quality",			tIngredientDef[ 2 ].nItemQuality,	DATATABLE_ITEM_QUALITY					)
	EF_INT(			"Ingredient 3 - Min Quantity",			tIngredientDef[ 2 ].nQuantityMin											)
	EF_INT(			"Ingredient 3 - Max Quantity",			tIngredientDef[ 2 ].nQuantityMax											)

	EF_LINK_INT(	"Ingredient 4 - Item Class",			tIngredientDef[ 3 ].nItemClass,		DATATABLE_ITEMS							)
	EF_LINK_INT(	"Ingredient 4 - Unit Type",				tIngredientDef[ 3 ].nUnitType,		DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"Ingredient 4 - Item Quality",			tIngredientDef[ 3 ].nItemQuality,	DATATABLE_ITEM_QUALITY					)
	EF_INT(			"Ingredient 4 - Min Quantity",			tIngredientDef[ 3 ].nQuantityMin											)
	EF_INT(			"Ingredient 4 - Max Quantity",			tIngredientDef[ 3 ].nQuantityMax											)

	EF_LINK_INT(	"Ingredient 5 - Item Class",			tIngredientDef[ 4 ].nItemClass,		DATATABLE_ITEMS							)
	EF_LINK_INT(	"Ingredient 5 - Unit Type",				tIngredientDef[ 4 ].nUnitType,		DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"Ingredient 5 - Item Quality",			tIngredientDef[ 4 ].nItemQuality,	DATATABLE_ITEM_QUALITY					)
	EF_INT(			"Ingredient 5 - Min Quantity",			tIngredientDef[ 4 ].nQuantityMin											)
	EF_INT(			"Ingredient 5 - Max Quantity",			tIngredientDef[ 4 ].nQuantityMax											)

	EF_LINK_INT(	"Ingredient 6 - Item Class",			tIngredientDef[ 5 ].nItemClass,		DATATABLE_ITEMS							)
	EF_LINK_INT(	"Ingredient 6 - Unit Type",				tIngredientDef[ 5 ].nUnitType,		DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"Ingredient 6 - Item Quality",			tIngredientDef[ 5 ].nItemQuality,	DATATABLE_ITEM_QUALITY					)
	EF_INT(			"Ingredient 6 - Min Quantity",			tIngredientDef[ 5 ].nQuantityMin											)
	EF_INT(			"Ingredient 6 - Max Quantity",			tIngredientDef[ 5 ].nQuantityMax											)
	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Recipe")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_ADD_ANCESTOR(DATATABLE_RECIPECATEGORIES)
	EXCEL_ADD_ANCESTOR(DATATABLE_RECIPEPROPERTIES)
	EXCEL_SET_POSTPROCESS_ROW(RecipeDefinitionPostProcessRow)
	EXCEL_SET_POSTPROCESS_ALL(RecipeDefinitionPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_RECIPES, RECIPE_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "recipes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(RECIPELIST_DEFINITION)
	EF_STRING(		"Recipe List",							szName																		)	
	EF_TWOCC(		"Code",									wCode																		)
	EF_LINK_IARR(	"Recipes",								nRecipes,							DATATABLE_RECIPES						)
	EF_BOOL(		"Randomly Selectable",					bRandomlySelectable															)		
	EF_INT(			"Random Select Weight",					nRandomSelectWeight															)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Recipe List")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_RECIPELISTS, RECIPELIST_DEFINITION,APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "recipelists") EXCEL_TABLE_END



//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(NPC_DATA)
	EF_STRING(		"Name",									szName																		)	
	EF_LINK_INT(	"Greeting Generic",						nSound[IE_HELLO_GENERIC],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Greeting Templar",						nSound[IE_HELLO_TEMPLAR],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Greeting Cabalist",					nSound[IE_HELLO_CABALIST],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Greeting Hunter",						nSound[IE_HELLO_HUNTER],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Greeting Male",						nSound[IE_HELLO_MALE],				DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Greeting Female",						nSound[IE_HELLO_FEMALE],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Greeting Faction Bad",					nSound[IE_HELLO_FACTION_BAD],		DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Greeting Faction Neutral",				nSound[IE_HELLO_FACTION_NEUTRAL],	DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Greeting Faction Good",				nSound[IE_HELLO_FACTION_GOOD],		DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Generic",						nSound[IE_GOODBYE_GENERIC],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Templar",						nSound[IE_GOODBYE_TEMPLAR],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Cabalist",						nSound[IE_GOODBYE_CABALIST],		DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Hunter",						nSound[IE_GOODBYE_HUNTER],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Male",							nSound[IE_GOODBYE_MALE],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Female",						nSound[IE_GOODBYE_FEMALE],			DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Faction Bad",					nSound[IE_GOODBYE_FACTION_BAD],		DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Faction Neutral",				nSound[IE_GOODBYE_FACTION_NEUTRAL],	DATATABLE_SOUNDS						)
	EF_LINK_INT(	"Goodbye Faction Good",					nSound[IE_GOODBYE_FACTION_GOOD],	DATATABLE_SOUNDS						)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_NPC, NPC_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "npc") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(INTERACT_MENU_DATA)
	EF_STRING(		"Name",									szName																		)
	EF_LINK_INT(	"Interaction",							nInteraction,						DATATABLE_INTERACT						)
	EF_STR_TABLE(	"String Title",							nStringTitle,						APP_GAME_ALL							)	
	EF_STR_TABLE(	"String Tooltip",						nStringTooltip,						APP_GAME_ALL							)	
	EF_STRING(		"Frame Icon",							szIconFrame																	)
	EF_INT(			"Menu Button",							nButtonNum																	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_INTERACT_MENU, INTERACT_MENU_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "interact_menu") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(INTERACT_DATA)
	EF_STRING(		"Name",									szName																		)
	EF_BOOL(		"Face During Interaction",				bFaceDuringInteraction														)	
	EF_BOOL(		"Allow Ghost",							bAllowGhost																	)		
	EF_INT(			"Priority",								nPriority,							NONE									)	
	EF_BOOL(		"Set Talking To",						bSetTalkingTo																)	
	EF_BOOL(		"Play Greeting",						bPlayGreeting																)		
	EF_LINK_INT(	"Interact Menu",						nInteractMenu,						DATATABLE_INTERACT_MENU					)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_INTERACT, INTERACT_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "interact") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(ITEM_QUALITY_DATA)
	EF_STRING(		"quality",								szName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EF_LINK_INT(	"namecolor",							nNameColor,							DATATABLE_FONTCOLORS					)
	EF_LINK_INT(	"bkgdcolor",							nGridColor,							DATATABLE_FONTCOLORS					)
	EF_LINK_INT(	"look group",							nLookGroup,							DATATABLE_ITEM_LOOK_GROUPS				)
	EF_LINK_INT(	"crafting breakdown treasure",			nBreakDownTreasure,					DATATABLE_TREASURE						)
	EF_STR_TABLE(	"displayname",							nDisplayName,						APP_GAME_ALL							)
	EF_STR_TABLE(	"display name with item format",		nDisplayNameWithItemFormat,			APP_GAME_ALL							)	
	EF_STRING(		"badge frame",							szTooltipBadgeFrame															)
	EF_BOOL(		"show base desc",						bShowBaseDesc																)		
	EF_BOOL(		"randomly named",						bRandomlyNamed																)		
	EF_STR_TABLE(	"basedescformatstring",					nBaseDescFormat,					APP_GAME_ALL							)	
	EF_BOOL(		"do transaction logging",				bDoTransactionLogging														)	
	EF_BOOL(		"change item class to match required quality only",		bChangeClass												)	
	EF_BOOL(		"always identified",					bAlwaysIdentified															)	
	EF_FLOAT(		"PriceMultiplier",						fPriceMultiplier,					1.0f									)
	EF_FLOAT(		"recipe quantity multiplier",			fRecipeQuantityMultiplier,			1.0f									)
	EF_INT(			"min level",							nMinLevel,							0										)
	EF_INT(			"rarity",								nRarity,							1										)
	EF_INT(			"vendor rarity",						nVendorRarity,						1										)
	EF_INT(			"luck rarity",							nRarityLuck,						0										)
	EF_INT(			"gambling rarity",						nGamblingRarity,					1										)
	EF_LINK_INT(	"state",								nState,								DATATABLE_STATES						)
	EF_LINK_INT(	"flippy sound",							nFlippySound,						DATATABLE_SOUNDS						)
	EF_BOOL(		"useable",								bUseable,							1										)
	EF_LINK_INT(	"Scrap Quality",						nScrapQuality,						DATATABLE_ITEM_QUALITY					)
	EF_LINK_INT(	"Scrap Quality Default",				nScrapQualityDefault,				DATATABLE_ITEM_QUALITY					)	
	EF_BOOL(		"Is Special Scrap Quality",				bIsSpecialScrapQuality														)	
	EF_INT(			"Extra scrap chance",					nExtraScrapChancePct,				0										)
	EF_LINK_INT(	"Extra scrap item",						nExtraScrapItem,					DATATABLE_ITEMS							)
	EF_LINK_INT(	"Extra scrap quality",					nExtraScrapQuality,					DATATABLE_ITEM_QUALITY					)
	EF_LINK_INT(	"Dismantle result sound",				nDismantleResultSound,				DATATABLE_SOUNDS						)
	EF_LINK_INT(	"downgrade",							nDowngrade,							DATATABLE_ITEM_QUALITY					)
	EF_LINK_INT(	"upgrade",								nUpgrade,							DATATABLE_ITEM_QUALITY					)	
	EF_INT(			"Quality Level",						nQualityLevel,						0										)	
	EF_FLOAT_PCT(	"proc chance",							flProcChance,						0										)
	EF_FLOAT_PCT(	"luck prob mod",						flLuckMod,							0										)
	EF_BOOL(		"crafting allows quality",				bAllowCraftingToCreateQuality												)	
	EF_CODE(		"affix1 chance",						tAffixPicks[0].codeAffixChance												)
	EF_CODE(		"affix1 type1 weight",					tAffixPicks[0].codeAffixTypeWeight[0]										)
	EF_LINK_INT(	"affix1 type1",							tAffixPicks[0].linkAffixType[0],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix1 type2 weight",					tAffixPicks[0].codeAffixTypeWeight[1]										)
	EF_LINK_INT(	"affix1 type2",							tAffixPicks[0].linkAffixType[1],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix1 type3 weight",					tAffixPicks[0].codeAffixTypeWeight[2]										)
	EF_LINK_INT(	"affix1 type3",							tAffixPicks[0].linkAffixType[2],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix1 type4 weight",					tAffixPicks[0].codeAffixTypeWeight[3]										)
	EF_LINK_INT(	"affix1 type4",							tAffixPicks[0].linkAffixType[3],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix1 type5 weight",					tAffixPicks[0].codeAffixTypeWeight[4]										)
	EF_LINK_INT(	"affix1 type5",							tAffixPicks[0].linkAffixType[4],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix1 type6 weight",					tAffixPicks[0].codeAffixTypeWeight[5]										)
	EF_LINK_INT(	"affix1 type6",							tAffixPicks[0].linkAffixType[5],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix2 chance",						tAffixPicks[1].codeAffixChance												)
	EF_CODE(		"affix2 type1 weight",					tAffixPicks[1].codeAffixTypeWeight[0]										)
	EF_LINK_INT(	"affix2 type1",							tAffixPicks[1].linkAffixType[0],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix2 type2 weight",					tAffixPicks[1].codeAffixTypeWeight[1]										)
	EF_LINK_INT(	"affix2 type2",							tAffixPicks[1].linkAffixType[1],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix2 type3 weight",					tAffixPicks[1].codeAffixTypeWeight[2]										)
	EF_LINK_INT(	"affix2 type3",							tAffixPicks[1].linkAffixType[2],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix2 type4 weight",					tAffixPicks[1].codeAffixTypeWeight[3]										)
	EF_LINK_INT(	"affix2 type4",							tAffixPicks[1].linkAffixType[3],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix2 type5 weight",					tAffixPicks[1].codeAffixTypeWeight[4]										)
	EF_LINK_INT(	"affix2 type5",							tAffixPicks[1].linkAffixType[4],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix2 type6 weight",					tAffixPicks[1].codeAffixTypeWeight[5]										)
	EF_LINK_INT(	"affix2 type6",							tAffixPicks[1].linkAffixType[5],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix3 chance",						tAffixPicks[2].codeAffixChance												)
	EF_CODE(		"affix3 type1 weight",					tAffixPicks[2].codeAffixTypeWeight[0]										)
	EF_LINK_INT(	"affix3 type1",							tAffixPicks[2].linkAffixType[0],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix3 type2 weight",					tAffixPicks[2].codeAffixTypeWeight[1]										)
	EF_LINK_INT(	"affix3 type2",							tAffixPicks[2].linkAffixType[1],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix3 type3 weight",					tAffixPicks[2].codeAffixTypeWeight[2]										)
	EF_LINK_INT(	"affix3 type3",							tAffixPicks[2].linkAffixType[2],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix3 type4 weight",					tAffixPicks[2].codeAffixTypeWeight[3]										)
	EF_LINK_INT(	"affix3 type4",							tAffixPicks[2].linkAffixType[3],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix3 type5 weight",					tAffixPicks[2].codeAffixTypeWeight[4]										)
	EF_LINK_INT(	"affix3 type5",							tAffixPicks[2].linkAffixType[4],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix3 type6 weight",					tAffixPicks[2].codeAffixTypeWeight[5]										)
	EF_LINK_INT(	"affix3 type6",							tAffixPicks[2].linkAffixType[5],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix4 chance",						tAffixPicks[3].codeAffixChance												)
	EF_CODE(		"affix4 type1 weight",					tAffixPicks[3].codeAffixTypeWeight[0]										)
	EF_LINK_INT(	"affix4 type1",							tAffixPicks[3].linkAffixType[0],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix4 type2 weight",					tAffixPicks[3].codeAffixTypeWeight[1]										)
	EF_LINK_INT(	"affix4 type2",							tAffixPicks[3].linkAffixType[1],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix4 type3 weight",					tAffixPicks[3].codeAffixTypeWeight[2]										)
	EF_LINK_INT(	"affix4 type3",							tAffixPicks[3].linkAffixType[2],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix4 type4 weight",					tAffixPicks[3].codeAffixTypeWeight[3]										)
	EF_LINK_INT(	"affix4 type4",							tAffixPicks[3].linkAffixType[3],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix4 type5 weight",					tAffixPicks[3].codeAffixTypeWeight[4]										)
	EF_LINK_INT(	"affix4 type5",							tAffixPicks[3].linkAffixType[4],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix4 type6 weight",					tAffixPicks[3].codeAffixTypeWeight[5]										)
	EF_LINK_INT(	"affix4 type6",							tAffixPicks[3].linkAffixType[5],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix5 chance",						tAffixPicks[4].codeAffixChance												)
	EF_CODE(		"affix5 type1 weight",					tAffixPicks[4].codeAffixTypeWeight[0]										)
	EF_LINK_INT(	"affix5 type1",							tAffixPicks[4].linkAffixType[0],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix5 type2 weight",					tAffixPicks[4].codeAffixTypeWeight[1]										)
	EF_LINK_INT(	"affix5 type2",							tAffixPicks[4].linkAffixType[1],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix5 type3 weight",					tAffixPicks[4].codeAffixTypeWeight[2]										)
	EF_LINK_INT(	"affix5 type3",							tAffixPicks[4].linkAffixType[2],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix5 type4 weight",					tAffixPicks[4].codeAffixTypeWeight[3]										)
	EF_LINK_INT(	"affix5 type4",							tAffixPicks[4].linkAffixType[3],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix5 type5 weight",					tAffixPicks[4].codeAffixTypeWeight[4]										)
	EF_LINK_INT(	"affix5 type5",							tAffixPicks[4].linkAffixType[4],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix5 type6 weight",					tAffixPicks[4].codeAffixTypeWeight[5]										)
	EF_LINK_INT(	"affix5 type6",							tAffixPicks[4].linkAffixType[5],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix6 chance",						tAffixPicks[5].codeAffixChance												)
	EF_CODE(		"affix6 type1 weight",					tAffixPicks[5].codeAffixTypeWeight[0]										)
	EF_LINK_INT(	"affix6 type1",							tAffixPicks[5].linkAffixType[0],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix6 type2 weight",					tAffixPicks[5].codeAffixTypeWeight[1]										)
	EF_LINK_INT(	"affix6 type2",							tAffixPicks[5].linkAffixType[1],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix6 type3 weight",					tAffixPicks[5].codeAffixTypeWeight[2]										)
	EF_LINK_INT(	"affix6 type3",							tAffixPicks[5].linkAffixType[2],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix6 type4 weight",					tAffixPicks[5].codeAffixTypeWeight[3]										)
	EF_LINK_INT(	"affix6 type4",							tAffixPicks[5].linkAffixType[3],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix6 type5 weight",					tAffixPicks[5].codeAffixTypeWeight[4]										)
	EF_LINK_INT(	"affix6 type5",							tAffixPicks[5].linkAffixType[4],	DATATABLE_AFFIXTYPES					)
	EF_CODE(		"affix6 type6 weight",					tAffixPicks[5].codeAffixTypeWeight[5]										)
	EF_LINK_INT(	"affix6 type6",							tAffixPicks[5].linkAffixType[5],	DATATABLE_AFFIXTYPES					)
	EF_BOOL(		"prefixname",							bPrefixName,						0										)
	EF_LINK_INT(	"prefixtype",							nPrefixType,						DATATABLE_AFFIXTYPES					)
	EF_CODE(		"prefixcount",							codePrefixCount																)
	EF_BOOL(		"force",								bForce,								0										)
	EF_BOOL(		"suffixname",							bSuffixName,						0										)
	EF_LINK_INT(	"suffixtype",							nSuffixType,						DATATABLE_AFFIXTYPES					)
	EF_CODE(		"suffixcount",							codeSuffixCount																)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "quality")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_NAMEFIELD("displayname");
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ITEM_QUALITY, ITEM_QUALITY_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "itemquality") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(AI_BEHAVIOR_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_STRING(		"function",								szFunction																	)
	EF_INT(			"function old",							nFunctionTheOldWay,					INVALID_ID								)
	EF_FLOAT(		"priority",								fDefaultPriority															)
	EF_BOOL(		"force exit branch",					bForceExitBranch															)
	EF_BOOL(		"uses skill 1",							pbUsesSkill[0]																)
	EF_BOOL(		"uses skill 2",							pbUsesSkill[1]																)
	EF_BOOL(		"uses state",							bUsesState																	)
	EF_BOOL(		"uses stat",							bUsesStat																	)
	EF_BOOL(		"uses sound",							bUsesSound																	)
	EF_BOOL(		"uses monster",							bUsesMonster																)
	EF_BOOL(		"uses string",							bUsesString																	)
	EF_BOOL(		"can branch",							bCanBranch																	)
	EF_INT(			"uses once",							nUsesOnce																	)
	EF_INT(			"uses run",								nUsesRun																	)
	EF_INT(			"uses fly",								nUsesFly																	)
	EF_INT(			"uses don't stop",						nUsesDontStop																)
	EF_INT(			"uses warp",							nUsesWarp																	)
	EF_INT(			"timer index",							nTimerIndex																	)
	EF_STRING(		"string desc",							pszStringDescription														)
	EF_STRING(		"param 0 desc",							pszParamDescription[0]														)
	EF_STRING(		"param 1 desc",							pszParamDescription[1]														)
	EF_STRING(		"param 2 desc",							pszParamDescription[2]														)
	EF_STRING(		"param 3 desc",							pszParamDescription[3]														)
	EF_STRING(		"param 4 desc",							pszParamDescription[4]														)
	EF_STRING(		"param 5 desc",							pszParamDescription[5]														)
	EF_FLOAT(		"param 0 default",						pfParamDefault[0]															)
	EF_FLOAT(		"param 1 default",						pfParamDefault[1]															)
	EF_FLOAT(		"param 2 default",						pfParamDefault[2]															)
	EF_FLOAT(		"param 3 default",						pfParamDefault[3]															)
	EF_FLOAT(		"param 4 default",						pfParamDefault[4]															)
	EF_FLOAT(		"param 5 default",						pfParamDefault[5]															)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(AiBehaviorDataExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_AI_BEHAVIOR, AI_BEHAVIOR_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "ai_behavior") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_INDEX_DEF(DATATABLE_AI_START, "Name")
EXCEL_TBIDX_DEF(DATATABLE_AI_START, EXCEL_INDEX_TABLE, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "ai_start") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(AI_INIT)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"start function",						nStartFunction,						DATATABLE_AI_START						)
	EF_FILE(		"behavior table",						pszTableName																)
	EF_INT(			"",										nTable,								INVALID_ID								)
	EF_LINK_INT(	"blocking state",						nBlockingState,						DATATABLE_STATES						)
	EF_BOOL(		"wants target",							bWantsTarget																)
	EF_BOOL(		"target closest",						bTargetClosest																)
	EF_BOOL(		"randomize targets",					bTargetRandomize															)
	EF_BOOL(		"no destructables",						bNoDestructables															)
	EF_BOOL(		"random start period",					bRandomStartPeriod															)
	EF_BOOL(		"dont freeze",							bAiNoFreeze																	)
	EF_BOOL(		"record spawn point",					bRecordSpawnPoint															)
	EF_BOOL(		"check busy",							bCheckBusy																	)
	EF_BOOL(		"start on AI init",						bStartOnAiInit																)
	EF_BOOL(		"never register AI",					bNeverRegisterAIEvent														)
	EF_BOOL(		"server only",							bServerOnly																	)
	EF_BOOL(		"client only",							bClientOnly																	)
	EF_BOOL(		"can see unsearchables",				bCanSeeUnsearchables														)
	EF_BOOL(		"give state on death",					pStateSharing[0].bOnDeath													)
	EF_LINK_INT(	"give state to unittype",				pStateSharing[0].nTargetUnitType,	DATATABLE_UNITTYPES						)
	EF_FLOAT(		"give state range",						pStateSharing[0].fRange														)
	EF_LINK_INT(	"state to give",						pStateSharing[0].nState,			DATATABLE_STATES						)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_POSTPROCESS_ALL(ExcelAIPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_AI_INIT, AI_INIT, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "ai_init") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// aicommon_state.txt
EXCEL_STRUCT_DEF(AI_STATE_DEFINITION)
	EF_STRING(		"Name",									szName																		)
	EF_TWOCC(		"code",									wCode																		)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_AICOMMON_STATE, AI_STATE_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "aicommon_state") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(FONT_DEFINITION)
	EF_STRING(		"font",									pszName																		)
	EF_FILE(		"SystemName",							pszSystemName																)
	EF_FILE(		"LocalPath",							pszLocalPath																)
	EF_BOOL(		"Bold",									bBold																		)			
	EF_BOOL(		"Italic",								bItalic																		)
	EF_INT(			"FontSize",								nGDISize																	)
	EF_INT(			"SizeInTexture",						nSizeInTexture																)
	EF_INT(			"LettersAcross",						nLetterCountWidth															)
	EF_INT(			"LettersDown",							nLetterCountHeight															)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "font")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_FONT, FONT_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "font") EXCEL_TABLE_END

//----------------------------------------------------------------------------
#define EF_SHADER_TYPE(name, etype)	\
	EF_LINK_INT(	name,									nEffectIndex[etype],				DATATABLE_EFFECTS						) \
	EF_INT(			"",										nEffectFile[etype],					INVALID_ID								) \
	EF_INT(			"",										nEffectID[etype],					INVALID_ID								)

EXCEL_STRUCT_DEF(SHADER_TYPE_DEFINITION)
	EF_STRING(		"Name",									pszShaderName																)
	EF_SHADER_TYPE(	"IndoorEffect",							SHADER_TYPE_INDOOR															)
	EF_SHADER_TYPE(	"OutdoorEffect",						SHADER_TYPE_OUTDOOR															)
	EF_SHADER_TYPE(	"IndoorGridEffect",						SHADER_TYPE_INDOORGRID														)
	EF_SHADER_TYPE(	"FlashlightEffect",						SHADER_TYPE_FLASHLIGHT														)
	EF_BOOL(		"NoCollide",							bNoCollide																	)
	EF_BOOL(		"For Particles",						bForParticles																)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_EFFECTS_SHADERS, SHADER_TYPE_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "effects_shaders") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT effectvertexformattype[] =
{
	{ "",				VERTEX_DECL_INVALID				},
	{ "rigid64",		VERTEX_DECL_RIGID_64			},
	{ "rigid32",		VERTEX_DECL_RIGID_32			},
	{ "rigid16",		VERTEX_DECL_RIGID_16			},
	{ "animated",		VERTEX_DECL_ANIMATED			},
	{ "animated11",		VERTEX_DECL_ANIMATED_11			},
	{ "particle",		VERTEX_DECL_PARTICLE_MESH		},
	{ "particle11",		VERTEX_DECL_PARTICLE_MESH_11	},
	{ NULL,				VERTEX_DECL_INVALID				},
};
STR_DICT effectfolder[] =
{
	{ "",				EFFECT_FOLDER_DEFAULT			},
	{ "common",			EFFECT_FOLDER_COMMON			},
	{ "hellgate",		EFFECT_FOLDER_HELLGATE			},
	{ "tugboat",		EFFECT_FOLDER_TUGBOAT			},
	{ NULL,				EFFECT_FOLDER_DEFAULT			},
};
STR_DICT effectsubfolder[] =
{
	{ "",				EFFECT_SUBFOLDER_NONE			},
	{ "1x",				EFFECT_SUBFOLDER_1X				},
};
STR_DICT sgTechniqueGroupDict[] =
{
	{ "model",			TECHNIQUE_GROUP_MODEL			},
	{ "particle",		TECHNIQUE_GROUP_PARTICLE		},
	{ "blur",			TECHNIQUE_GROUP_BLUR			},
	{ "general",		TECHNIQUE_GROUP_GENERAL			},
	{ "list",			TECHNIQUE_GROUP_LIST			},
	{ NULL,				TECHNIQUE_GROUP_GENERAL			},
};

EXCEL_STRUCT_DEF(EFFECT_DEFINITION)
	EF_STR_LOWER(	"FXOName",								pszFXOName																	)
	EF_DICT_INT(	"Folder",								eFolder,							effectfolder							)
	EF_DICT_INT(	"SubFolder",							eSubFolder,							effectsubfolder							)
	EF_STR_LOWER(	"FXFile",								pszFXFileName																)
	EF_DICT_INT(	"VertexFormat",							eVertexFormat,						effectvertexformattype					)
	EF_DICT_INT(	"TechniqueGroup",						nTechniqueGroup,					sgTechniqueGroupDict					)
	EF_FLAG(		"StateFromEffect",						dwFlags,							EFFECTDEF_FLAGBIT_STATE_FROM_EFFECT,		1	)    
	EF_FLAG(		"CastShadow",							dwFlags,							EFFECTDEF_FLAGBIT_CAST_SHADOW,				1	)    
	EF_FLAG(		"ReceiveShadow",						dwFlags,							EFFECTDEF_FLAGBIT_RECEIVE_SHADOW,			0	)    
	EF_FLAG(		"ReceiveRain",							dwFlags,							EFFECTDEF_FLAGBIT_RECEIVE_RAIN,				0	)    
	EF_FLAG(		"Animated",								dwFlags,							EFFECTDEF_FLAGBIT_ANIMATED,					0	)    
	EF_FLAG(		"RenderToZ",							dwFlags,							EFFECTDEF_FLAGBIT_RENDER_TO_Z,				0	)    
	EF_FLAG(		"ForceAlphaPass",						dwFlags,							EFFECTDEF_FLAGBIT_FORCE_ALPHA_PASS,			0	)    
	EF_FLAG(		"AlphaBlend",							dwFlags,							EFFECTDEF_FLAGBIT_ALPHA_BLEND,				0	)    
	EF_FLAG(		"AlphaTest",							dwFlags,							EFFECTDEF_FLAGBIT_ALPHA_TEST,				0	)    
	EF_FLAG(		"CheckFormat",							dwFlags,							EFFECTDEF_FLAGBIT_CHECK_FORMAT,				0	)    
	EF_FLAG(		"Fragments",							dwFlags,							EFFECTDEF_FLAGBIT_FRAGMENTS,				0	)    
	EF_FLAG(		"Force Lightmap",						dwFlags,							EFFECTDEF_FLAGBIT_FORCE_LIGHTMAP,			0	)    
	EF_FLAG(		"EmissiveDiffuse",						dwFlags,							EFFECTDEF_FLAGBIT_EMISSIVE_DIFFUSE,			0	)    
	EF_FLAG(		"Needs Normal",							dwFlags,							EFFECTDEF_FLAGBIT_NEEDS_NORMAL,				0	)    
	EF_FLAG(		"Compress Tex Coord",					dwFlags,							EFFECTDEF_FLAGBIT_COMPRESS_TEX_COORD,		0	)    
	EF_FLAG(		"Specular LUT",							dwFlags,							EFFECTDEF_FLAGBIT_SPECULAR_LUT,				0	)    
	EF_FLAG(		"UseBGSHCoefs",							dwFlags,							EFFECTDEF_FLAGBIT_USE_BG_SH_COEFS,			0	)    
	EF_FLAG(		"UseGlobalLights",						dwFlags,							EFFECTDEF_FLAGBIT_USE_GLOBAL_LIGHTS,		0	)    
	EF_FLAG(		"DirectionalInSH",						dwFlags,							EFFECTDEF_FLAGBIT_DIRECTIONAL_IN_SH,		0	)    
	EF_FLAG(		"BackupTransSpecular",					dwFlags,							EFFECTDEF_FLAGBIT_LOD_TRANS_SPECULAR,		0	)    
	EF_FLAG(		"EmitsGPUParticles",					dwFlags,							EFFECTDEF_FLAGBIT_EMITS_GPU_PARTICLES,		0	)    
	EF_FLAG(		"is screen effect",						dwFlags,							EFFECTDEF_FLAGBIT_IS_SCREEN_EFFECT,			0	)    	
	EF_FLAG(		"LoadAllTechniques",					dwFlags,							EFFECTDEF_FLAGBIT_LOAD_ALL_TECHNIQUES,		0	)    	
	EF_FLAG(		"one particle system",					dwFlags,							EFFECTDEF_FLAGBIT_ONE_PARTICLE_SYSTEM,		0	)    
	EF_FLAG(		"uses portals",							dwFlags,							EFFECTDEF_FLAGBIT_USES_PORTALS,				0	)    	
	EF_FLAG(		"requires havok fx",					dwFlags,							EFFECTDEF_FLAGBIT_REQUIRES_HAVOKFX,			0	)    	
	EF_INT(         "SBranchDepthVS",						nStaticBranchDepthVS									          	        )    
	EF_INT(         "SBranchDepthPS",						nStaticBranchDepthPS									          	        )    
	EF_FLOAT(		"RangeToFallback",						fRangeToLOD,						-1.0f									)
	EF_LINK_INT(	"DistanceFallback",						nLODEffectIndex,					DATATABLE_EFFECTS						)
	EF_INT(			"",										nLODEffectDef,						INVALID_ID								)
	EF_INT(			"",										nLODD3DEffectId,					INVALID_ID								)
	EF_INT(			"",										nD3DEffectId,						INVALID_ID								)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "FXOName")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "FXFile")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_EFFECTS_FILES, EFFECT_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "effects_files") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// effects_index.xls
EXCEL_STRUCT_DEF(EFFECT_INDEX)
	EF_STR_LOWER(	"EffectName",							pszEffectName																)
	EF_LINK_INT(	"SM_40",								pnFXOIndex[FXTGT_SM_40],			DATATABLE_EFFECTS_FILES					)
	EF_LINK_INT(	"SM_30",								pnFXOIndex[FXTGT_SM_30],			DATATABLE_EFFECTS_FILES					)
	EF_LINK_INT(	"SM_20_HIGH",							pnFXOIndex[FXTGT_SM_20_HIGH],		DATATABLE_EFFECTS_FILES					)
	EF_LINK_INT(	"SM_20_LOW",							pnFXOIndex[FXTGT_SM_20_LOW],		DATATABLE_EFFECTS_FILES					)
	EF_LINK_INT(	"SM_11",								pnFXOIndex[FXTGT_SM_11],			DATATABLE_EFFECTS_FILES					)
	EF_LINK_INT(	"FixedFunc",							pnFXOIndex[FXTGT_FIXED_FUNC],		DATATABLE_EFFECTS_FILES					)
	EF_BOOL(        "Required",								bRequired																	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "EffectName")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_EFFECTS, EFFECT_INDEX, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "effects_index") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// budgets.xls
STR_DICT budgetstexturegrouptype[] =
{
	{ "",				TEXTURE_GROUP_NONE				},
	{ "Background",		TEXTURE_GROUP_BACKGROUND		},
	{ "Units",			TEXTURE_GROUP_UNITS				},
	{ "Particle",		TEXTURE_GROUP_PARTICLE			},
	{ "UI",				TEXTURE_GROUP_UI				},
	{ "Wardrobe",		TEXTURE_GROUP_WARDROBE			},
	{ NULL,				TEXTURE_GROUP_NONE				},
};
EXCEL_STRUCT_DEF(TEXTURE_BUDGET_MIP_GROUP)
	EF_DICT_INT(	"Group",								eGroup,								budgetstexturegrouptype					)
	EF_FLOAT(		"Diffuse",								fMIP[TEXTURE_DIFFUSE]														)
	EF_FLOAT(		"Normal",								fMIP[TEXTURE_NORMAL]														)
	EF_FLOAT(		"SelfIllum",							fMIP[TEXTURE_SELFILLUM]														)
	EF_FLOAT(		"Diffuse2",								fMIP[TEXTURE_DIFFUSE2]														)
	EF_FLOAT(		"Specular",								fMIP[TEXTURE_SPECULAR]														)
	EF_FLOAT(		"Envmap",								fMIP[TEXTURE_ENVMAP]														)
	EF_FLOAT(		"Lightmap",								fMIP[TEXTURE_LIGHTMAP]														)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Group")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_BUDGETS_TEXTURE_MIPS, TEXTURE_BUDGET_MIP_GROUP, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "budgets_texture_mips") EXCEL_TABLE_END

//----------------------------------------------------------------------------
//STR_DICT budgetstechlevel[] =
//{
//	{ "",				BUDGET_TECH_LEVEL_HIGH			},
//	{ "High",			BUDGET_TECH_LEVEL_HIGH			},
//	{ "Low",			BUDGET_TECH_LEVEL_LOW			},
//	{ NULL,				BUDGET_TECH_LEVEL_HIGH			},
//};
//EXCEL_STRUCT_DEF(TEXTURE_BUDGET_HEURISTICS_GROUP)
//	EF_STRING(		"Locale",								szLocale																	)
//	EF_DICT_INT(	"Tech",									eTech,								budgetstechlevel						)
//	EF_DICT_INT(	"Group",								eGroup,								budgetstexturegrouptype					)
//	EF_INT(			"Diffuse",								nMB[TEXTURE_DIFFUSE]														)
//	EF_INT(			"Normal",								nMB[TEXTURE_NORMAL]															)
//	EF_INT(			"SelfIllum",							nMB[TEXTURE_SELFILLUM]														)
//	EF_INT(			"Diffuse2",								nMB[TEXTURE_DIFFUSE2]														)
//	EF_INT(			"Specular",								nMB[TEXTURE_SPECULAR]														)
//	EF_INT(			"Envmap",								nMB[TEXTURE_ENVMAP]															)
//	EF_INT(			"Lightmap",								nMB[TEXTURE_LIGHTMAP]															)
//	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Locale", "Tech", "Group")
//EXCEL_STRUCT_END
//EXCEL_TABLE_DEF(DATATABLE_BUDGETS_TEXTURE_HEURISTICS, TEXTURE_BUDGET_HEURISTICS_GROUP, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "budgets_texture_heuristics") EXCEL_TABLE_END

STR_DICT budgetsmodelgrouptype[] =
{
	{ "",				MODEL_GROUP_NONE			},
	{ "Background",		MODEL_GROUP_BACKGROUND		},
	{ "Units",			MODEL_GROUP_UNITS			},
	{ "Particle",		MODEL_GROUP_PARTICLE		},
	{ "UI",				MODEL_GROUP_UI				},
	{ "Wardrobe",		MODEL_GROUP_WARDROBE		},
	{ NULL,				MODEL_GROUP_NONE			},
};
EXCEL_STRUCT_DEF(MODEL_BUDGET_GROUP)
EF_DICT_INT(	"Group",								eGroup,								budgetsmodelgrouptype					)
EF_FLOAT(		"LOD Rate",								fLODRate																	)
EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Group")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_BUDGETS_MODEL, MODEL_BUDGET_GROUP, APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "budgets_model") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// CHB 2007.03.08 - initdb.xls
// We use NaN so we can distinguish between entered values and blanks.
EXCEL_STRUCT_DEF(INITDB_TABLE)
	EF_BOOL(		"Skip",									bSkip,								FALSE									)
	EF_FOURCC(		"Criteria",								nCriteria,							INITDB_TABLE::INVALID_FOURCC			)
	EF_INT(			"RangeLow",								nRangeLow,							INITDB_TABLE::INVALID_RANGE				)
	EF_INT(			"RangeHigh",							nRangeHigh,							INITDB_TABLE::INVALID_RANGE				)
	EF_STRING(		"NumKnob",								szNumKnob																	)
	EF_FLOAT(		"NumMin",								fNumMin,							NaN										)
	EF_FLOAT(		"NumMax",								fNumMax,							NaN										)
	EF_FLOAT(		"NumInit",								fNumInit,							NaN										)
	EF_FOURCC(		"FeatKnob",								nFeatKnob,							INITDB_TABLE::INVALID_FOURCC			)
	EF_FOURCC(		"FeatMin",								nFeatMin,							INITDB_TABLE::INVALID_FOURCC			)
	EF_FOURCC(		"FeatMax",								nFeatMax,							INITDB_TABLE::INVALID_FOURCC			)
	EF_FOURCC(		"FeatInit",								nFeatInit,							INITDB_TABLE::INVALID_FOURCC			)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_INITDB, INITDB_TABLE, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "initdb") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT tDictPackEnum[] =
{
	{ "",				PAK_DEFAULT },
	{ "default",		PAK_DEFAULT },
	{ "graphics high",	PAK_GRAPHICS_HIGH },
	{ "sound",			PAK_SOUND },
	{ "sound high",		PAK_SOUND_HIGH },
	{ "sound low",		PAK_SOUND_LOW },
	{ "sound music",	PAK_SOUND_MUSIC },
	{ "localized",		PAK_LOCALIZED },
	{ NULL,				PAK_DEFAULT }
};

//----------------------------------------------------------------------------
// badges.xls
STR_DICT tDictBadge[] =
{
	{ "",					BADGE_TYPE_INVALID },
	{ "QA",					ACCT_TITLE_TESTER},
	{ "CSR",				ACCT_TITLE_CUSTOMER_SERVICE_REPRESENTATIVE},
	{ "Subscriber",			ACCT_TITLE_SUBSCRIBER},
	{ "Trial",				ACCT_MODIFIER_TRIAL_SUBSCRIPTION},
	{ "Standard",			ACCT_MODIFIER_STANDARD_SUBSCRIPTION},
	{ "Lifetime",			ACCT_MODIFIER_LIFETIME_SUBSCRIPTION},
	{ "Gamestop",			ACCT_MODIFIER_PREORDER_EBGS},
	{ "Best Buy",			ACCT_MODIFIER_PREORDER_BESTBUY},
	{ "Walmart",			ACCT_MODIFIER_PREORDER_WALMART},
	{ "Generic",			ACCT_MODIFIER_PREORDER_GEN},
	{ "Korean Dye Kit 1",	ACCT_MODIFIER_ASIA_DK_01},
	{ "Korean Dye Kit 2",	ACCT_MODIFIER_ASIA_DK_02},
	{ "Korean Dye Kit 3",	ACCT_MODIFIER_ASIA_DK_03},
	{ "Korean Dye Kit 4",	ACCT_MODIFIER_ASIA_DK_04},
	{ "Korean Dye Kit 5",	ACCT_MODIFIER_ASIA_DK_05},
	{ "Korean Dye Kit 6",	ACCT_MODIFIER_ASIA_DK_06},
	{ "Korean Dye Kit 7",	ACCT_MODIFIER_ASIA_DK_07},
	{ "Korean Dye Kit 8",	ACCT_MODIFIER_ASIA_DK_08},
	{ "Korean Dye Kit 9",	ACCT_MODIFIER_ASIA_DK_09},
	{ "Korean Dye Kit 10",	ACCT_MODIFIER_ASIA_DK_10},
	{ "Korean Dye Kit 11",	ACCT_MODIFIER_ASIA_DK_11},
	{ "Korean Dye Kit 12",	ACCT_MODIFIER_ASIA_DK_12},
	{ "CE",					ACCT_MODIFIER_COLLECTORS_EDITION},
	{ "PCGamer",			ACCT_MODIFIER_PC_GAMER},
	{ "FSSPing0",			ACCT_TITLE_FSSPING0_EMPLOYEE },
	{ "EA",					ACCT_TITLE_EA_EMPLOYEE }, 
	{ "IAH",				ACCT_MODIFIER_IAH_CUSTOMER_HALLOWEEN }, 
	{ "HBS Coco Pet",		ACCT_MODIFIER_HBS_COCO_PET} ,
	{ "Exp Bonus Bang 5",	ACCT_MODIFIER_HBS_PC_BANG_5EXP},		// HBS - PC BANG Badge Adds 5% EXP Bonus
	{ "Exp Bonus Bang 10",	ACCT_MODIFIER_HBS_PC_BANG_10EXP},
	{ "HBS Halloween",		ACCT_MODIFIER_HBS_CUSTOMER_HALLOWEEN}
};

EXCEL_STRUCT_DEF(BADGE_REWARDS_DATA)
	EF_STRING(		"Badge Reward Name",					pszName																			)
	EF_TWOCC(		"Code",									m_wCode																			)
	EF_DICT_INT(	"BadgeName",							m_nBadge,						tDictBadge										)
	EF_LINK_INT(	"Item",									m_nItem,						DATATABLE_ITEMS									)
	EF_LINK_INT(	"don’t apply if player has reward item for",m_nDontApplyIfPlayerHasRewardItemFor, DATATABLE_BADGE_REWARDS				)
	EF_LINK_INT(	"State",								m_nState,						DATATABLE_STATES								)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Badge Reward Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_BADGE_REWARDS, BADGE_REWARDS_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "badge_rewards") EXCEL_TABLE_END
// filepath, struct_name (already defined elsewhere), ...

EXCEL_STRUCT_DEF(DONATION_REWARDS_DATA)
EF_STRING(		"Donation Reward Name",					pszName																			)
EF_TWOCC(		"Code",									m_wCode																			)
EF_LINK_INT(	"Item",									m_nItem,						DATATABLE_ITEMS									)
EF_LINK_INT(	"don’t apply if player has reward item for",m_nDontApplyIfPlayerHasRewardItemFor, DATATABLE_BADGE_REWARDS				)
EF_LINK_INT(	"State",								m_nState,						DATATABLE_STATES								)
EF_INT(			"Duration in ticks",					m_nTicks,						INVALID_ID										)
EF_INT(			"Weight",								m_nWeight																		)
EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Donation Reward Name")
EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_DONATION_REWARDS, DONATION_REWARDS_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "donation_rewards") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// levels.xls
EXCEL_STRUCT_DEF(LEVEL_FILE_PATH)
	EF_FOURCC(		"Code",									dwCode																		)
	EF_STRING(		"Path",									pszPath																		)
	EF_LINK_IARR(	"Localized Folders",					nFolderLocalized,				DATATABLE_LEVEL_FILE_PATHS					)
	EF_DICT_INT(	"Language",								eLanguage,						sgtLanguageDict								)
	EF_DICT_INT(	"Pakfile",								ePakfile,						tDictPackEnum								)		
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_ROW(ExcelLevelFilePathPostProcessRow)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_FILE_PATHS, LEVEL_FILE_PATH, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levels_file_path") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// levels_env.txt
EXCEL_STRUCT_DEF(LEVEL_ENVIRONMENT_DEFINITION)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_FOURCC(	"FolderCode",							nPathIndex,							DATATABLE_LEVEL_FILE_PATHS				)
	EF_FILE(		"Filename",								pszEnvironmentFile															)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_ENVIRONMENTS, LEVEL_ENVIRONMENT_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levels_env") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// levels_room_index.txt
// props.txt
EXCEL_STRUCT_DEF(ROOM_INDEX)
	EF_STRING(		"Name",									pszFile																		)
	EF_BOOL(		"Outdoor",								bOutdoor																	)
	EF_BOOL(		"OutdoorVisibility",					bOutdoorVisibility															)
	EF_BOOL(		"ComputeAmbientOcclusion",				bComputeObscurance															)
	EF_BOOL(		"NoCollision",							bNoCollision																)
	EF_BOOL(		"NoMonsterSpawn",						bNoMonsterSpawn																)
	EF_BOOL(		"NoAdventures",							bNoAdventures																)
	EF_BOOL(		"NoSubLevelEntrance",					bNoSubLevelEntrance															)	
	EF_BOOL(		"Occupies Nodes",						bOccupiesNodes																)
	EF_BOOL(		"Raises Nodes",							bRaisesNodes																)
	EF_BOOL(		"Don't Obstruct Sound",					bDontObstructSound															)
	EF_BOOL(		"Don't Occlude Visibility",				bDontOccludeVisibility														)
	EF_BOOL(		"Third Person Camera Ignore",			b3rdPersonCameraIgnore														)
	EF_BOOL(		"RTS Camera Ignore",					bRTSCameraIgnore															)
	EF_BOOL(		"Full Collision",						bFullCollision																)
	EF_BOOL(		"Use MatID",							bUseMatID																	)
	EF_BOOL(		"Canopy",								bCanopyProp																	)
	EF_BOOL(		"Clutter",								bClutter																	)
	EF_FLOAT(		"Node Buffer",							fNodeBuffer																	)
	EF_INT(  		"HavokSliceType",						nHavokSliceType																)
	EF_INT(  		"Room Version",							nExcelVersion																)

	EF_STR_TABLE(	"Room Message",							nMessageString,							APP_GAME_ALL						)
	EF_LINK_IARR(	"Spawn Class",							nSpawnClass,							DATATABLE_SPAWN_CLASS				)
	EF_IARR(  		"Run Spawn Class X Times",				nSpawnClassRunXTimes,					1									)
	

	EF_STRING(		"Reverb Environment",					pszReverbFile																)
	EF_STRING(		"Override Nodes",						pszOverrideNodes															)
	EF_CODE(		"MonsterLevel Override",				codeMonsterLevel[DIFFICULTY_NORMAL]											)
	EF_LINK_INT(	"Background Sound",						nBackgroundSound,					DATATABLE_BACKGROUNDSOUNDS				)
	EF_LINK_INT(	"No Gore",								nNoGoreProp,						DATATABLE_PROPS							)
	EF_LINK_INT(	"No Humans",							nNoHumansProp,						DATATABLE_PROPS							)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelRoomIndexPostProcess)
	EXCEL_SET_ROWFREE(ExcelRoomIndexFreeRow);
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ROOM_INDEX, ROOM_INDEX, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levels_room_index") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_PROPS, ROOM_INDEX, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "props") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(LEVEL_THEME)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_IARR(	"IsA",									nIsA,								DATATABLE_LEVEL_THEMES					)
	EF_BOOL(		"Don't Display in Editor",				bDontDisplayInLayoutEditor													)
	EF_BOOL(		"Highlander",							bHighlander																	)
	EF_STRING(		"Environment",							pszEnvironment																)
	EF_INT(			"Env Priority",							nEnvironmentPriority														)
	EF_LINK_IARR(	"Allowed Styles",						nAllowedStyles,						DATATABLE_LEVEL_DRLGS,	LEVEL_DRLG_STYLE_INDEX	)
	EF_LINK_INT(	"global theme required",				nGlobalThemeRequired,				DATATABLE_GLOBAL_THEMES					)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_THEMES, LEVEL_THEME, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levels_themes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(WEATHER_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"State",								nState,								DATATABLE_STATES						)
	EF_LINK_IARR(	"Themes",								nThemes,							DATATABLE_LEVEL_THEMES					)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WEATHER, WEATHER_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "weather") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(WEATHER_SET_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_LINK_INT(	"Weather1",								pWeatherSets[0].nWeather,			DATATABLE_WEATHER						)
	EF_INT(			"Weight1",								pWeatherSets[0].nWeight														)
	EF_LINK_INT(	"Weather2",								pWeatherSets[1].nWeather,			DATATABLE_WEATHER						)
	EF_INT(			"Weight2",								pWeatherSets[1].nWeight														)
	EF_LINK_INT(	"Weather3",								pWeatherSets[2].nWeather,			DATATABLE_WEATHER						)
	EF_INT(			"Weight3",								pWeatherSets[2].nWeight														)
	EF_LINK_INT(	"Weather4",								pWeatherSets[3].nWeather,			DATATABLE_WEATHER						)
	EF_INT(			"Weight4",								pWeatherSets[3].nWeight														)
	EF_LINK_INT(	"Weather5",								pWeatherSets[4].nWeather,			DATATABLE_WEATHER						)
	EF_INT(			"Weight5",								pWeatherSets[4].nWeight														)
	EF_LINK_INT(	"Weather6",								pWeatherSets[5].nWeather,			DATATABLE_WEATHER						)
	EF_INT(			"Weight6",								pWeatherSets[5].nWeight														)
	EF_LINK_INT(	"Weather7",								pWeatherSets[6].nWeather,			DATATABLE_WEATHER						)
	EF_INT(			"Weight7",								pWeatherSets[6].nWeight														)
	EF_LINK_INT(	"Weather8",								pWeatherSets[7].nWeather,			DATATABLE_WEATHER						)
	EF_INT(			"Weight8",								pWeatherSets[7].nWeight														)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WEATHER_SETS, WEATHER_SET_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "weather_sets") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// levels_rules.txt
EXCEL_STRUCT_DEF(DRLG_PASS)
	EF_STRING(		"DrlgFileName",							pszDrlgFileName																)
	EF_LINK_FOURCC(	"FolderCode",							nPathIndex,							DATATABLE_LEVEL_FILE_PATHS				)
	EF_STRING(		"DRLGRuleset",							pszDRLGName																	)
	EF_STRING(		"Label",								pszLabel																	)
	EF_INT(			"MinSubs",								nMinSubs[0]																	)
	EF_INT(			"MaxSubs",								nMaxSubs[0]																	)
	EF_INT(			"MinSubs Nightmare",					nMinSubs[1]																	)
	EF_INT(			"MaxSubs Nightmare",					nMaxSubs[1]																	)
	EF_INT(			"Attempts",								nAttempts																	)
	EF_BOOL(		"ReplaceAll",							bReplaceAll																	)
	EF_BOOL(		"ReplaceAndCheck",						bReplaceWithCollisionCheck													)
	EF_BOOL(		"Once for ReplaceAndCheck",				bReplaceAndCheckOnce														)
	EF_BOOL(		"MustPlace",							bMustPlace																	)
	EF_BOOL(		"Weighted",								bWeighted																	)
	EF_BOOL(		"ExitRule",								bExitRule																	)
	EF_BOOL(		"DeadEndRule",							bDeadEndRule																)
	EF_INT(			"Looping",								nLoopAmount																	)
	EF_STRING(		"LoopLabel",							pszLoopLabel																)
	EF_BOOL(		"AskQuests",							bAskQuests																	)
	EF_INT(			"RandomQuestPercent",					nQuestPercent																)
	EF_LINK_INT(	"QuestTaskComplete",					nQuestTaskComplete,					DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)
	EF_LINK_INT(	"Theme_RunRule",						nThemeRunRule,						DATATABLE_LEVEL_THEMES					)
	EF_LINK_INT(	"Theme_SkipRule",						nThemeSkipRule,						DATATABLE_LEVEL_THEMES					)
	EXCEL_SET_INDEX(0, 0, "DrlgFileName")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelLevelRulesPostProcess)
	EXCEL_SET_POSTPROCESS_ALL(ExcelLevelRulesPostLoadAll)
	EXCEL_SET_ROWFREE(ExcelLevelRulesFreeRow);
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_RULES, DRLG_PASS, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levels_rules") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// levels_drlgs.txt
EXCEL_STRUCT_DEF(DRLG_DEFINITION)
	EF_STRING(		"DRLGName",								pszName																		)
	EF_STRING(		"DRLGRuleset",							pszDRLGRuleset																)
	EF_STR_TABLE(	"DRLGDisplayName",						nDRLGDisplayNameStringIndex,		APP_GAME_ALL							)
	EF_STRINT(		"Style",								nStyle																		)	
	EF_LINK_IARR(	"Theme",								nThemes,							DATATABLE_LEVEL_THEMES					)
	EF_LINK_INT(	"Weather Set",							nWeatherSet,						DATATABLE_WEATHER_SETS					)
	EF_LINK_FOURCC(	"FolderCode",							nPathIndex,							DATATABLE_LEVEL_FILE_PATHS				)
	EF_LINK_INT(	"Environment",							nEnvironment,						DATATABLE_LEVEL_ENVIRONMENTS			)
	EF_BOOL(		"PopulateAllVisible",					bPopulateAllVisible															)
	EF_FLOAT(		"Override Champion Chance %",			flChampionSpawnRateOverride,		-1.0f									)	
	EF_FLOAT(		"Champion Zone Radius",					flChampionZoneRadius,				-1.0f									)		
	EF_FLOAT(		"MarkerSpawnChance",					flMarkerSpawnChance,				0.1f									)
	EF_FLOAT_PCT(	"MonsterRoomDensity",					flMonsterRoomDensityPercent,		0.0f									)	
	EF_FLOAT_PCT(	"RoomMonsterChance",					flRoomMonsterChance,				0.0f									)	
	EF_FLOAT_PCT(	"CritterRoomDensity",					tDRLGSpawn[ DRLG_SPAWN_TYPE_CRITTER ].flRoomDensityPercent,			0.0f	)	
	EF_LINK_INT(	"Critter SpawnClass",					tDRLGSpawn[ DRLG_SPAWN_TYPE_CRITTER ].nSpawnClass,	  DATATABLE_SPAWN_CLASS	)
	EF_FLOAT(		"Max Treasure Drop Radius",				flMaxTreasureDropRadius,			DEFAULT_MAX_TREASURE_DROP_RADIUS		)
	EF_LINK_INT(	"Random State",							nRandomState,						DATATABLE_STATES						)
	EF_INT(			"Random State Rate Min",				nRandomStateRateMin,	 			0										)
	EF_INT(			"Random State Rate Max",				nRandomStateRateMax,	 			0										)
	EF_INT(			"Random State Duration",				nRandomStateDuration,	 			0										)
	EF_BOOL(		"Havok FX",								bCanUseHavokFX,						FALSE									)
	EF_BOOL(		"ForceDrawAllRooms",					bForceDrawAllRooms,					FALSE									)	
	EF_BOOL(		"Is Outdoors",							bIsOutdoors,						FALSE									)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "DRLGName")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_DONT_ADD_BLANK, "Style")
EXCEL_STRUCT_END
static int LEVEL_DRLG_STYLE_INDEX = 1;
EXCEL_TABLE_DEF(DATATABLE_LEVEL_DRLGS, DRLG_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levels_drlgs") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// sublevel.txt
STR_DICT tDictSubLevelType[] =
{
	{ "",			ST_INVALID },
	{ "Hellrift",	ST_HELLRIFT },
	{ "TruthAOld",	ST_TRUTH_A_OLD },
	{ "TruthANew",	ST_TRUTH_A_NEW },
	{ "TruthBOld",	ST_TRUTH_B_OLD },
	{ "TruthBNew",	ST_TRUTH_B_NEW },
	{ "TruthCOld",	ST_TRUTH_C_OLD },
	{ "TruthCNew",	ST_TRUTH_C_NEW },
	{ "TruthDOld",	ST_TRUTH_D_OLD },
	{ "TruthDNew",	ST_TRUTH_D_NEW },
	{ "TruthEOld",	ST_TRUTH_E_OLD },
	{ "TruthENew",	ST_TRUTH_E_NEW },
	{ NULL,			ST_INVALID },
};
EXCEL_STRUCT_DEF(SUBLEVEL_DEFINITION)
	EF_STRING(		"Name",									szName																		)
	EF_LINK_INT(	"DRLG",									nDRLGDef,							DATATABLE_LEVEL_DRLGS					)
	EF_LINK_INT(	"Weather",								nWeatherSet,						DATATABLE_WEATHER_SETS					)
	EF_BOOL(		"Allow Town Portals",					bAllowTownPortals															)	
	EF_BOOL(		"Allow Monster Distribution",			bAllowMonsterDistribution													)
	EF_BOOL(		"Headstone at Entrance Object",			bHeadstoneAtEntranceObject													)	
	EF_BOOL(		"Respawn At Entrance",					bRespawnAtEntrance															)		
	EF_BOOL(		"Party Portals At Entrance",			bPartyPortalsAtEntrance														)			
	EF_DICT_INT(	"Type",									eType,								tDictSubLevelType						)	
	EF_FLOAT(		"Default Position X",					vDefaultPosition.fX															)	
	EF_FLOAT(		"Default Position Y",					vDefaultPosition.fY															)	
	EF_FLOAT(		"Default Position Z",					vDefaultPosition.fZ															)	
	EF_FLOAT(		"Entrance Flat Z Tolerance",			flEntranceFlatZTolerance													)		
	EF_FLOAT(		"Entrance Flat Radius",					flEntranceFlatRadius														)			
	EF_FLOAT(		"Entrance Flat Height Min",				flEntranceFlatHeightMin														)				
	EF_BOOL(		"Auto Create Entrance",					bAutoCreateEntrance															)			
	EF_BOOL(		"Allow Layout Markers For Entrance",	bAllowLayoutMarkersForEntrance												)			
	EF_STRING(		"Layout Marker Entrance Name",			szLayoutMarkerEntrance														)	
	EF_BOOL(		"Allow Path Nodes For Entrance",		bAllowPathNodesForEntrance													)				
	EF_LINK_INT(	"Object Entrance",						nObjectEntrance,					DATATABLE_OBJECTS						)
	EF_LINK_INT(	"Object Exit",							nObjectExit,						DATATABLE_OBJECTS						)	
	EF_LINK_INT(	"Alternative Entrance Unittype",		nAlternativeEntranceUnitType,		DATATABLE_UNITTYPES						)	
	EF_LINK_INT(	"Alternative Exit Unittype",			nAlternativeExitUnitType,			DATATABLE_UNITTYPES						)	
	EF_LINK_INT(	"Sub Level Next",						nSubLevelDefNext,					DATATABLE_SUBLEVEL						)			
	EF_BOOL(		"Override Level Spawns",				bOverrideLevelSpawns														)
	EF_LINK_INT(	"Spawn Class",							nSpawnClass,						DATATABLE_SPAWN_CLASS					)
	EF_LINK_INT(	"State When Inside SubLevel",			nStateWhenInside,					DATATABLE_STATES						)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SUBLEVEL, SUBLEVEL_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "sublevel") EXCEL_TABLE_END

//----------------------------------------------------------------------------
STR_DICT srvleveltype[] =
{
	{ "",			SRVLEVELTYPE_DEFAULT	},
	{ "soloinst",	SRVLEVELTYPE_TUTORIAL	},
	{ "minitown",	SRVLEVELTYPE_MINITOWN	},
	{ "bigtown",	SRVLEVELTYPE_BIGTOWN	},
	{ "custom",		SRVLEVELTYPE_CUSTOM		},
	{ "ctf",		SRVLEVELTYPE_PVP_CTF	},
	{ "openworld",	SRVLEVELTYPE_OPENWORLD	},
	{ NULL,			SRVLEVELTYPE_DEFAULT	},
};
EXCEL_STRUCT_DEF(LEVEL_DEFINITION)
	EF_STRING(		"LevelName",							pszName																		)
	EF_TWOCC(		"code",									wCode																		)
	EF_INT(			"bitfield index",						nBitIndex																	)
	EF_LINK_INT(	"Default Sub Level",					nSubLevelDefault,					DATATABLE_SUBLEVEL						)	
	EF_LINK_INT(	"PreviousLevel",						nPrevLevel,							DATATABLE_LEVEL							)
	EF_LINK_INT(	"NextLevel",							nNextLevel,							DATATABLE_LEVEL							)
	EF_STR_TABLE(	"LevelDisplayName",						nLevelDisplayNameStringIndex,		APP_GAME_ALL							)
	EF_STR_TABLE(	"FloorSuffixName",						nFloorSuffixName,					APP_GAME_ALL							)
	EF_STR_TABLE(	"FinalSuffixFloorName",					nFinalFloorSuffixName,				APP_GAME_ALL							)
	EF_FLOAT(		"CameraDollyScale",						flCameraDollyScale,					1.0f									)	
	EF_FLOAT(		"Min Z",								fMinPathingZ,						-.5f									)	
	EF_FLOAT(		"Max Z",								fMaxPathingZ,						.5f										)	
	EF_FLOAT(		"Automap Width",						fAutomapWidth,						0.0f										)	
	EF_FLOAT(		"Automap Height",						fAutomapHeight,						0.0f										)	
	EF_BOOL(		"Contoured",							bContoured,							FALSE																		)
	EF_BOOL(		"Client Discard Rooms",					bClientDiscardRooms,				FALSE																		)
	EF_BOOL(		"Town",									bTown																		)
	EF_BOOL(		"RTS",									bRTS																		)
	EF_BOOL(		"Always Active",						bAlwaysActive																)	
	EF_FLOAT(		"VisibilityTileSize",					fVisibilityTileSize,				1.0f									)	
	EF_FLOAT(		"VisibilityOpacity",					fVisibilityOpacity,					1.0f									)	
	EF_FLOAT(		"VisibilityDistanceScale",				fVisibilityDistanceScale,			1.0f									)	
	EF_BOOL(		"OrientedToMap",						bOrientedToMap,						0										)	
	EF_BOOL(		"AllowRandomOrientation",				bAllowRandomOrientation,			0										)	
	EF_FLOAT(		"FixedOrientation",						fFixedOrientation,					0.0f									)	
	EF_BOOL(		"StartingLocation",						bStartingLocation															)
	EF_LINK_INT(	"Level Restart Redirect",				nLevelRestartRedirect,				DATATABLE_LEVEL							)	
	EF_BOOL(		"PortalAndRecallLoc",					bPortalAndRecallLoc															)
	EF_BOOL(		"First Portal and Recall Loc",			bFirstPortalAndRecallLoc													)	
	EF_BOOL(		"Safe",									bSafe																		)
	EF_BOOL(		"Hardcore dead can visit",				bHardcoreDeadCanVisit														)
	EF_BOOL(		"Can form auto parties",				bCanFormAutoParties															)	
	EF_LINK_INT(	"Sub Level Town Portal",				nSubLevelDefTownPortal,				DATATABLE_SUBLEVEL						)
	EF_BOOL(		"Tutorial",								bTutorial																	)	
	EF_BOOL(		"Disable Town Portals",					bDisableTownPortals															)	
	EF_BOOL(		"On Enter Clear Bad States",			bOnEnterClearBadStates														)
	EF_BOOL(		"Player Owned Cannot Die",				bPlayerOwnedCannotDie														)	
	EF_INT(			"Hellrift Chance %",					nHellriftChance																)	
	EF_LINK_IARR(	"Hellrift Sub Levels",					nSubLevelHellrift,					DATATABLE_SUBLEVEL						)		
	EF_LINK_INT(	"Sub Level 1",							nSubLevel[0],						DATATABLE_SUBLEVEL						)
	EF_LINK_INT(	"Sub Level 2",							nSubLevel[1],						DATATABLE_SUBLEVEL						)
	EF_LINK_INT(	"Sub Level 3",							nSubLevel[2],						DATATABLE_SUBLEVEL						)
	EF_LINK_INT(	"Sub Level 4",							nSubLevel[3],						DATATABLE_SUBLEVEL						)
	EF_LINK_INT(	"Sub Level 5",							nSubLevel[4],						DATATABLE_SUBLEVEL						)
	EF_LINK_INT(	"Sub Level 6",							nSubLevel[5],						DATATABLE_SUBLEVEL						)
	EF_LINK_INT(	"Sub Level 7",							nSubLevel[6],						DATATABLE_SUBLEVEL						)
	EF_LINK_INT(	"Sub Level 8",							nSubLevel[7],						DATATABLE_SUBLEVEL						)
	EF_BOOL(		"use team colors",						bUseTeamColors																)	
	EF_DICT_INT(	"SrvLevelType",							eSrvLevelType,						srvleveltype							)
	EF_INT(			"Player Max",							nMaxPlayerCount,					INT_MAX									)
	EF_INT(			"Party Size Recommended",				nPartySizeRecommended,				0										)
	EF_LINK_INT(	"waypoint",								nWaypointMarker,					DATATABLE_OBJECTS						)
	EF_BOOL(		"autowaypoint",							bAutoWaypoint																)
	EF_BOOL(		"MultiplayerOnly",						bMultiplayerOnly															)
	EF_BOOL(		"Monster Level From Activator",			bMonsterLevelFromActivatorLevel												)
	EF_INT(			"Monster Level Activator Delta",		nMonsterLevelActivatorDelta,												)
	EF_CODE(		"MonsterLevel",							codeMonsterLevel[DIFFICULTY_NORMAL]											)
	EF_CODE(		"MonsterLevel Nightmare",				codeMonsterLevel[DIFFICULTY_NIGHTMARE]										)
	EF_BOOL(		"Monster Level From Parent Level",		bMonsterLevelFromCreatorLevel												)	
	EF_INT(			"Select Random Theme Pct",				nSelectRandomThemePct														)
	EF_FLOAT(		"Champion Spawn Chance % At Each Spawn Location",	flChampionSpawnRate,	0.0f									)	
	EF_FLOAT(		"Unique Monster Chance %",				flUniqueSpawnChance,				0.0f									)		
	EF_LINK_INT(	"QuestSpawnClass",						nQuestSpawnClass,					DATATABLE_SPAWN_CLASS					)
	EF_LINK_INT(	"Interactable SpawnClass",				nInteractableSpawnClass,			DATATABLE_SPAWN_CLASS					)
	EF_INT(			"MapX",									nMapX,								-1										)
	EF_INT(			"MapY",									nMapY,								-1										)
	EF_STR_TABLE(	"StringEnter",							nStringEnter,						APP_GAME_ALL							)
	EF_STR_TABLE(	"StringLeave",							nStringLeave,						APP_GAME_ALL							)
	EF_LINK_INT(	"Madlibs",								nLevelMadlibNames[MYTHOS_MADLIB_SENTENCES],		DATATABLE_EXCELTABLES		)	
	EF_LINK_INT(	"Nouns",								nLevelMadlibNames[MYTHOS_MADLIB_NOUNS],			DATATABLE_EXCELTABLES		)	
	EF_LINK_INT(	"Adjectives",							nLevelMadlibNames[MYTHOS_MADLIB_ADJECTIVES],	DATATABLE_EXCELTABLES		)	
	EF_LINK_INT(	"Suffixs",								nLevelMadlibNames[MYTHOS_MADLIB_SUFFIXS],		DATATABLE_EXCELTABLES		)	
	EF_LINK_INT(	"Affixs",								nLevelMadlibNames[MYTHOS_MADLIB_AFFIXS],		DATATABLE_EXCELTABLES		)	
	EF_LINK_INT(	"ProperNames",							nLevelMadlibNames[MYTHOS_MADLIB_PROPERNAMES],	DATATABLE_EXCELTABLES		)	
	EF_BOOL(		"FirstLevel",							bFirstLevel,						FALSE									)
	EF_BOOL(		"FirstLevel Cheating",					bFirstLevelCheating,				FALSE									)
	EF_INT(			"PlayerExpLevel",						nPlayerExpLevel,					-1										)	
	EF_INT(			"Starting Gold",						nStartingGold,						0										)	
	EF_LINK_INT(	"BonusStartingTreasure",				nBonusStartingTreasure,				DATATABLE_TREASURE						)	
	EF_INT(			"SequenceNumber",						nSequenceNumber,					-1										)			
	EF_LINK_INT(	"Act",									nAct,								DATATABLE_ACT							)			
	EF_INT(			"WorldMapRow",							nMapRow,							-1										)
	EF_INT(			"WorldMapCol",							nMapCol,							-1										)
	EF_STRING(		"WorldMapFrame",						szMapFrame																	)
	EF_STRING(		"WorldMapFrameUnexplored",				szMapFrameUnexplored														)
	EF_LINK_INT(	"WorldMapColor",						nMapColor,							DATATABLE_FONTCOLORS					)
	EF_DICT_INT(	"WorldMapLabelPos",						eMapLabelPos,						pAlignEnumTbl							)
	EF_FLOAT(		"WorldMapLabelXOffs",					fMapLabelXOffs,						0.0f									)
	EF_FLOAT(		"WorldMapLabelYOffs",					fMapLabelYOffs,						0.0f									)
	EF_LINK_IARR(	"WorldMapConnectIDs",					pnMapConnectIDs,					DATATABLE_LEVEL							)
	EF_INT(			"Adventure Chance %",					nAdventureChancePercent,			0										)	
	EF_CODE(		"Num Adventures",						codeNumAdventures															)	
	EF_BOOL(		"Enable Room Reset",					bRoomResetEnabled,					FALSE									)	
	EF_INT(			"Room Reset Delay In Seconds",			nRoomResetDelayInSeconds,			NONE									)		
	EF_INT(			"Room Reset When Monsters Below Percent",	nRoomResetMonsterPercent,		NONE									)			
	EF_FLOAT(		"Monster Room Density Multiplier",		flMonsterRoomDensityMultiplier,		1.0f									)
	EF_BOOL(		"Contents Always Revealed",				bContentsAlwaysRevealed,			FALSE									)
	EF_CODE(		"ScriptPlayerEnterLevel",				codePlayerEnterLevel														)	
	EF_BOOL(		"AllowOverworldTravel",					bAllowOverworldTravel,				FALSE									)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "LevelName")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelLevelsPostProcess)
	EXCEL_SET_ROWFREE(ExcelLevelFreeRow)
	EXCEL_SET_TABLEFREE(ExcelLevelsFree);
	EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL, LEVEL_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levels") EXCEL_TABLE_END

EXCEL_STRUCT_DEF(LEVEL_DRLG_CHOICE)
	EF_INHERIT_ROW( "base row",								nBaseRow																	)
	EF_DSTRING(		"Name",									pszName																		)
	EF_TWOCC(		"Code",									wCode																		)
	EF_LINK_INT(	"Level Name",							nLevel,								DATATABLE_LEVEL							)
	EF_LINK_INT(	"Difficulty",							nDifficulty,						DATATABLE_DIFFICULTY					)
	EF_LINK_INT(	"DRLG",									nDRLG,								DATATABLE_LEVEL_DRLGS					)
	EF_LINK_INT(	"Spawn Class",							nSpawnClass,						DATATABLE_SPAWN_CLASS					)
	EF_LINK_INT(	"Named Monster Class",					nNamedMonsterClass,					DATATABLE_MONSTERS						)
	EF_FLOAT(		"Named Monster Chance",					fNamedMonsterChance,				0.0f									)		
	EF_INT(			"Weight",								nDRLGWeight,						0										)
	EF_LINK_INT(	"Music",								nMusicRef,							DATATABLE_MUSIC							)
	EF_LINK_INT(	"Environment Override",					nEnvironmentOverride,				DATATABLE_LEVEL_ENVIRONMENTS			)
	EF_FLOAT_PCT(	"Environment Spawn Class Room Density",	fEnvRoomDensityPercent,				0.0f									)	
	EF_LINK_INT(	"Environment Spawn Class",				nEnvSpawnClass,						DATATABLE_SPAWN_CLASS					)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_DRLG_CHOICE, LEVEL_DRLG_CHOICE, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levels_drlg_choice") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// madlib stuff
using namespace MYTHOS_LEVELAREAS;
using namespace CRAFTING;

EXCEL_STRUCT_DEF(MADLIB_SENTENCE)
	EF_STR_TABLE(	"Madlib",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_MADLIB, MADLIB_SENTENCE, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_madlib") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_CAVE_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_CAVE_NOUNS, MADLIB_CAVE_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_cave") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_TEMPLE_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_TEMPLE_NOUNS, MADLIB_TEMPLE_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_temple") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_GOTHIC_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_GOTHIC_NOUNS, MADLIB_GOTHIC_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_gothic") EXCEL_TABLE_END

EXCEL_STRUCT_DEF(MADLIB_MINES_NAMES)
EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_MINES_NOUNS, MADLIB_MINES_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_mines") EXCEL_TABLE_END

EXCEL_STRUCT_DEF(MADLIB_CELLAR_NAMES)
EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_CELLAR_NOUNS, MADLIB_CELLAR_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_cellar") EXCEL_TABLE_END


EXCEL_STRUCT_DEF(MADLIB_DESERTGOTHIC_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_DESERTGOTHIC_NOUNS, MADLIB_DESERTGOTHIC_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_desertgothic") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_GOBLIN_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_GOBLIN_NOUNS, MADLIB_GOBLIN_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_goblin") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_HEATH_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_HEATH_NOUNS, MADLIB_HEATH_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_heath") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_CANYON_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_CANYON_NOUNS, MADLIB_CANYON_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_canyon") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_FOREST_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_FOREST_NOUNS, MADLIB_FOREST_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_forest") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_FARMLAND_NAMES)
	EF_STR_TABLE(	"Nouns",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_FARMLAND_NOUNS, MADLIB_FARMLAND_NAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_farmland") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_ADJ_BRIGHT)
	EF_STR_TABLE(	"Adjectives",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_ADJ_BRIGHT, MADLIB_ADJ_BRIGHT, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_adj_bright") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_PROPERNAMEZONE2)
	EF_STR_TABLE(	"ProperName",		m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_PROPERNAMEZONE2, MADLIB_PROPERNAMEZONE2, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_properzone2") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_GOBLIN_PROPERNAMES)
	EF_STR_TABLE(	"ProperName",		m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_GOBLIN_NAMES, MADLIB_GOBLIN_PROPERNAMES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_goblinnames") EXCEL_TABLE_END



EXCEL_STRUCT_DEF(MADLIB_ADJECTIVES)
	EF_STR_TABLE(	"Adjectives",			m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_ADJECTIVES, MADLIB_ADJECTIVES, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_adjectives") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_AFFIXS)
	EF_STR_TABLE(	"Affixs",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_AFFIXS, MADLIB_AFFIXS, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_affixs") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_SUFFIXS)
	EF_STR_TABLE(	"Suffixs",				m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_SUFFIXS, MADLIB_SUFFIXS, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_suffixs") EXCEL_TABLE_END
EXCEL_STRUCT_DEF(MADLIB_PROPERNAMEZONE1)
	EF_STR_TABLE(	"ProperName",		m_StringIndex,			APP_GAME_TUGBOAT				)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_PROPERNAMEZONE1, MADLIB_PROPERNAMEZONE1, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_properzone1") EXCEL_TABLE_END

//end madlibs
///////////////////////


//----------------------------------------------------------------------------
// level_zones.txt
using namespace MYTHOS_LEVELZONES;
EXCEL_STRUCT_DEF(LEVEL_ZONE_DEFINITION)
	EF_STR_TABLE(	"ZoneNameString",						nZoneNameStringID,					APP_GAME_TUGBOAT						)
	EF_STRING(		"zoneName",								pszZoneName																	)	
	EF_STRING(		"imageName",							pszImageName																)	
	EF_STRING(		"imageFrame",							pszImageFrame																)	
	EF_STRING(		"automapFrame",							pszAutomapFrame																)	
	EF_TWOCC(		"code",									wCode																		)
	EF_FLOAT(		"ZoneMapWidth",							fZoneMapWidth,						0										)
	EF_FLOAT(		"ZoneMapHeight",							fZoneMapHeight,						0										)
	EF_FLOAT(		"ZoneMapOffsetX",						fZoneOffsetX,						0										)
	EF_FLOAT(		"ZoneMapOffsetY",						fZoneOffsetY,						0										)
	EF_FLOAT(		"WorldWidth",							fWorldWidth,							0										)
	EF_FLOAT(		"WorldHeight",							fWorldHeight,						0										)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "ZoneName")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
	EXCEL_SET_POSTPROCESS_TABLE(LevelZoneExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_ZONES, LEVEL_ZONE_DEFINITION, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "level_zones") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// level_areas.txt
using namespace MYTHOS_LEVELAREAS;
STR_DICT sgtLevelAreaType[] =
{
	{ "",				KLEVELTYPE_NORMAL },
	{ "Normal",			KLEVELTYPE_NORMAL },
	{ "PVP",			KLEVELTYPE_PVP },
	{ NULL,				KLEVELTYPE_NORMAL },
};
EXCEL_STRUCT_DEF(LEVEL_AREA_DEFINITION)
	EF_STRING(		"LevelName",							pszName																		)
	EF_STR_TABLE(	"LevelDisplayName",						nLevelAreaDisplayNameStringIndex,	APP_GAME_TUGBOAT						)


	
	EF_TWOCC(		"code",									wCode																		)		
	EF_LINK_INT(	"LevelsAppearsInZone",					nZone,								DATATABLE_LEVEL_ZONES					)
	EF_LINK_IARR(	"LevelsAppearAroundHubs(6)",			nAreaHubs,							DATATABLE_LEVEL_AREAS					)		
	EF_LINK_IARR(	"Can Warp To Level Areas",				nTransporterAreas,					DATATABLE_LEVEL_AREAS					)		
	EF_INT(			"Area Warp Level Requirement",			nAreaWarpLevelRequirement,			0										)

	EF_LINK_IARR(	"LevelSpawnsTreasureFromRandomMonster",	nRandomMonsterDropsTreasureClass,	DATATABLE_TREASURE						)
	EF_BOOL(		"LinkSpawnTreasureToObjectCreate",		bRandomMonsterDropsLinkToObjectCreate										)
	

	EF_INT(			"MinDifficultyLevel",					nMinDifficultyLevel															)
	EF_INT(			"MaxDifficultyLevel",					nMaxDifficultyLevel															)
	EF_IARR(		"Min/Max Additional Difficulty Range(int)",	nDifficultyLevelAdditional,					0							)		
	EF_LINK_IARR(	"random themes( 5 )",					nAreaThemes,						DATATABLE_LEVEL_THEMES					)		
	EF_FLOAT(		"Difficulty Scale By Level(min)",		fDifficultyScaleByLevelMin,			.5f										)
	EF_FLOAT(		"Difficulty Scale By Level(max)",		fDifficultyScaleByLevelMax,			.5f										)
	EF_INT(			"LevelRequirement",						nLevelRequirement															)
	EF_BOOL(		"Starting Area",						bStartArea																	)
	EF_BOOL(		"Intro Area",							bIntroArea																	)
	EF_BOOL(		"ShowsOnMap",							bShowsOnMap																	)	
	EF_BOOL(		"AlwaysShowsOnMap",						bAlwaysShowsOnMap																	)	
	EF_BOOL(		"ShowsOnWorldMap",						bShowsOnWorldMap															)	
	EF_BOOL(		"RuneStoneOnly",						bRuneStone																	)	
	EF_BOOL(		"IsHostile",							bIsHostile																	)	
	EF_BOOL(		"Is Static World",						bIsStaticWorld																)	
	EF_BOOL(		"Allow Primary Save",					bPrimarySave																)
	EF_INT(			"CanWarpToSection",						nCanWarpToSection,					0										)
	EF_INT(			"IconType",								nIconType																	)
	EF_INT(			"X Location",							nX																			)
	EF_INT(			"Y Location",							nY																			)	
	EF_INT(			"World X Location",						nWorldX																		)
	EF_INT(			"World Y Location",						nWorldY																		)	
	EF_LINK_INT(	"lastlevel",							nLastLevel,							DATATABLE_LEVEL							)	
	EF_LINK_INT(	"linkToLevelArea",						nLinkToLevelArea,					DATATABLE_LEVEL_AREAS						)	
	EF_LINK_IARR(	"levelAreaThemesGeneric(25)",			nThemesGeneric,						DATATABLE_LEVEL_THEMES					)
	EF_LINK_IARR(	"levelAreaThemeGenericQuestTasks(25)",	nThemesQuestTaskGeneric,			DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)
	EF_IARR(		"levelAreaThemesGenericProps(25)",		nThemePropertyGeneric,				-1										)	
	
	


	EF_LINK_IARR(	"levelAreas Section1",					m_Sections[0].nLevels,				DATATABLE_LEVEL							)	
	EF_IARR(		"levelAreasSection1MinMaxDepth",		m_Sections[0].nDepthRange,			-1										)
	EF_STR_TABLE(	"levelAreaSection1NameOverride",		m_Sections[0].nLevelAreaNameOverride,APP_GAME_TUGBOAT						)
	EF_BOOL(		"levelAreaSection1DONTShowDepth",		m_Sections[0].bDontShowDepth,		FALSE									)
	EF_LINK_IARR(	"levelAreaSection1SpawnClasses(3)",		m_Sections[0].nSpawnClassOverride,	DATATABLE_SPAWN_CLASS					)
	EF_LINK_IARR(	"levelAreaSection1Themes(5)",			m_Sections[0].m_Themes.nThemes,		DATATABLE_LEVEL_THEMES					)
	EF_LINK_IARR(	"levelAreaSection1ThemeQuestTasks(5)",	m_Sections[0].m_Themes.nQuestTask,	DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)
	EF_IARR(		"levelAreaSection1ThemeProps",			m_Sections[0].m_Themes.nThemeProperty,		-1								)	
	EF_IARR(		"levelScaleSection1(min,max)",			m_Sections[0].nLevelScale,			-1										)	
	EF_LINK_IARR(	"levelAreasSection1Weather(5)",			m_Sections[0].nWeathers,			DATATABLE_WEATHER_SETS					)	
	EF_BOOL(		"allowLevelTreasureSpawnSec1",			m_Sections[0].bAllowLevelAreaTreasureSpawn, FALSE							)

	EF_LINK_IARR(	"levelAreas Section2",					m_Sections[1].nLevels,				DATATABLE_LEVEL							)	
	EF_IARR(		"levelAreasSection2MinMaxDepth",		m_Sections[1].nDepthRange,			-1										)
	EF_STR_TABLE(	"levelAreaSection2NameOverride",		m_Sections[1].nLevelAreaNameOverride,APP_GAME_TUGBOAT						)
	EF_BOOL(		"levelAreaSection2DONTShowDepth",		m_Sections[1].bDontShowDepth,		FALSE									)
	EF_LINK_IARR(	"levelAreaSection2SpawnClasses(3)",		m_Sections[1].nSpawnClassOverride,	DATATABLE_SPAWN_CLASS					)
	EF_LINK_IARR(	"levelAreaSection2Themes(5)",			m_Sections[1].m_Themes.nThemes,		DATATABLE_LEVEL_THEMES					)
	EF_LINK_IARR(	"levelAreaSection2ThemeQuestTasks(5)",	m_Sections[1].m_Themes.nQuestTask,	DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)
	EF_IARR(		"levelAreaSection2ThemeProps",			m_Sections[1].m_Themes.nThemeProperty,		-1								)	
	EF_IARR(		"levelScaleSection2(min,max)",			m_Sections[1].nLevelScale,			-1										)	
	EF_LINK_IARR(	"levelAreasSection2Weather(5)",			m_Sections[1].nWeathers,			DATATABLE_WEATHER_SETS					)	
	EF_BOOL(		"allowLevelTreasureSpawnSec2",			m_Sections[1].bAllowLevelAreaTreasureSpawn, FALSE							)

	EF_LINK_IARR(	"levelAreas Section3",					m_Sections[2].nLevels,				DATATABLE_LEVEL							)	
	EF_IARR(		"levelAreasSection3MinMaxDepth",		m_Sections[2].nDepthRange,			-1										)
	EF_STR_TABLE(	"levelAreaSection3NameOverride",		m_Sections[2].nLevelAreaNameOverride,APP_GAME_TUGBOAT						)
	EF_BOOL(		"levelAreaSection3DONTShowDepth",		m_Sections[2].bDontShowDepth,		FALSE									)
	EF_LINK_IARR(	"levelAreaSection3SpawnClasses(3)",		m_Sections[2].nSpawnClassOverride,	DATATABLE_SPAWN_CLASS					)
	EF_LINK_IARR(	"levelAreaSection3Themes(5)",			m_Sections[2].m_Themes.nThemes,		DATATABLE_LEVEL_THEMES					)
	EF_LINK_IARR(	"levelAreaSection3ThemeQuestTasks(5)",	m_Sections[2].m_Themes.nQuestTask,	DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)
	EF_IARR(		"levelAreaSection3ThemeProps",			m_Sections[2].m_Themes.nThemeProperty,		-1								)	
	EF_IARR(		"levelScaleSection3(min,max)",			m_Sections[2].nLevelScale,			-1										)	
	EF_LINK_IARR(	"levelAreasSection3Weather(5)",			m_Sections[2].nWeathers,			DATATABLE_WEATHER_SETS					)	
	EF_BOOL(		"allowLevelTreasureSpawnSec3",			m_Sections[2].bAllowLevelAreaTreasureSpawn, FALSE							)

	EF_LINK_IARR(	"levelAreas Section4",					m_Sections[3].nLevels,				DATATABLE_LEVEL							)	
	EF_IARR(		"levelAreasSection4MinMaxDepth",		m_Sections[3].nDepthRange,			-1										)
	EF_STR_TABLE(	"levelAreaSection4NameOverride",		m_Sections[3].nLevelAreaNameOverride,APP_GAME_TUGBOAT						)
	EF_BOOL(		"levelAreaSection4DONTShowDepth",		m_Sections[3].bDontShowDepth,		FALSE									)
	EF_LINK_IARR(	"levelAreaSection4SpawnClasses(3)",		m_Sections[3].nSpawnClassOverride,	DATATABLE_SPAWN_CLASS					)
	EF_LINK_IARR(	"levelAreaSection4Themes(5)",			m_Sections[3].m_Themes.nThemes,		DATATABLE_LEVEL_THEMES					)
	EF_LINK_IARR(	"levelAreaSection4ThemeQuestTasks(5)",	m_Sections[3].m_Themes.nQuestTask,	DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)
	EF_IARR(		"levelAreaSection4ThemeProps",			m_Sections[3].m_Themes.nThemeProperty,		-1								)	
	EF_IARR(		"levelScaleSection4(min,max)",			m_Sections[3].nLevelScale,			-1										)	
	EF_LINK_IARR(	"levelAreasSection4Weather(5)",			m_Sections[3].nWeathers,			DATATABLE_WEATHER_SETS					)
	EF_BOOL(		"allowLevelTreasureSpawnSec4",			m_Sections[3].bAllowLevelAreaTreasureSpawn, FALSE							)

	EF_LINK_IARR(	"levelAreas Section5",					m_Sections[4].nLevels,				DATATABLE_LEVEL							)	
	EF_IARR(		"levelAreasSection5MinMaxDepth",		m_Sections[4].nDepthRange,			-1										)
	EF_STR_TABLE(	"levelAreaSection5NameOverride",		m_Sections[4].nLevelAreaNameOverride,APP_GAME_TUGBOAT						)
	EF_BOOL(		"levelAreaSection5DONTShowDepth",		m_Sections[4].bDontShowDepth,		FALSE									)
	EF_LINK_IARR(	"levelAreaSection5SpawnClasses(3)",		m_Sections[4].nSpawnClassOverride,	DATATABLE_SPAWN_CLASS					)
	EF_LINK_IARR(	"levelAreaSection5Themes(5)",			m_Sections[4].m_Themes.nThemes,		DATATABLE_LEVEL_THEMES					)
	EF_LINK_IARR(	"levelAreaSection5ThemeQuestTasks(5)",	m_Sections[4].m_Themes.nQuestTask,	DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)
	EF_IARR(		"levelAreaSection5ThemeProps",			m_Sections[4].m_Themes.nThemeProperty,		-1								)	
	EF_IARR(		"levelScaleSection5(min,max)",			m_Sections[4].nLevelScale,			-1										)	
	EF_LINK_IARR(	"levelAreasSection5Weather(5)",			m_Sections[4].nWeathers,			DATATABLE_WEATHER_SETS					)
	EF_BOOL(		"allowLevelTreasureSpawnSec5",			m_Sections[4].bAllowLevelAreaTreasureSpawn, FALSE							)

	EF_LINK_INT(	"ShowIfQuestComplete",					nQuestCompleteShow,					DATATABLE_QUEST_TITLES_FOR_TUGBOAT		)
	EF_LINK_INT(	"ItemMustHave",							nItemCheck,							DATATABLE_ITEMS							)
	EF_INT(			"LevelsToCreateRandomly",				nDungeonsToCreateFromTemplate												)	
	EF_INT(			"override pick weight",					nPickWeight																	)	
	EF_DICT_INT(	"Level Type",							nLevelType,							sgtLevelAreaType						)			
	EF_IARR(		"Min Max Party Size(int)",				nPartySizeMinMax,					-1										)	
	EF_IARR(		"Min Max Level Scale Global(int)",		nLevelSizeMultMinMax,				-1										)	
	EF_IARR(		"Min/Max Density Global (int)",			nMonsterDensityGlobal,				-1										)
	EF_IARR(		"Levels for Room Density",				nMonsterDensityLevels,				-1										)	
	EF_IARR(		"Levels Density Min(int)",				nMonsterDensityMin,					-1										)	
	EF_IARR(		"Levels Density Max(int)",				nMonsterDensityMax,					-1										)	
	EF_FLOAT(		"Level Damage Scale",					nLevelDMGScale,						-1.0f									)		
	EF_INT(			"Max Open Player Size",					nMaxOpenPlayerSize,					1										)
	EF_INT(			"Max Open Player Size PVP World",		nMaxOpenPlayerSizePVPWorld,			1										)
	EF_INT(			"Champion Chance Mult Global(int)",		nChampionMultGlobal,				100										)
	EF_IARR(		"Levels For Champion Spawn",			nChampionMultLevel															)	
	EF_IARR(		"Champion mult per level(int)",			nChampionMult,						-1										)	

	EF_IARR(		"XPMod for normal monsters by level",	nXPModPerNormalMonsters,			100										)	
	EF_IARR(		"XPMod level",							nXPModLevel,						0										)	


	EF_BOOL(		"No Champion On Last Level",			nChampionNotAllowedOnLastLevel,     1										)	
	EF_IARR(		"Levels To assign Spawn Class",			nSpawnClassFloorsToSpawnIn,			-1										)	
	EF_LINK_IARR(	"Spawn Class assigned per level",		nSpanwClassFloorsSpawnClasses,		DATATABLE_SPAWN_CLASS					)
	EF_LINK_IARR(	"Boss Qualities",						nBossQuality,						DATATABLE_MONSTER_QUALITY				)
	EF_LINK_IARR(	"Elite Boss Qualities",					nBossEliteQuality,					DATATABLE_MONSTER_QUALITY				)
	EF_LINK_INT(	"Random end Boss Spawn Class",			nBossSpawnClass,					DATATABLE_SPAWN_CLASS					)
	EF_LINK_INT(	"Elite Bosses Spawn Class",				nBossEliteSpawnClass,				DATATABLE_SPAWN_CLASS					)	
	EF_INT(			"Chance for Elite Boss",				nChanceForEliteBoss,				100										)
	EF_LINK_IARR(	"Object Create",						nObjectsToSpawn,					DATATABLE_OBJECTS						)
	EF_INT(			"Chance for Object",					nChanceForObjectSpawn,				100										)
	EF_CODE(		"Client Level Area Load",				codeScripts[KSCRIPT_LEVELAREA_AREALOAD]										)
	EF_CODE(		"Client Unit Init",						codeScripts[KSCRIPT_LEVELAREA_UNITINIT]										)
	EF_CODE(		"Monster Unit Init",					codeScripts[KSCRIPT_LEVELAREA_MONSTERINIT]									)
	EF_CODE(		"Boss Monster Script",					codeScripts[KSCRIPT_LEVELAREA_BOSSINIT]										)
	EF_INT(			"Visit Only X times Min",				nCon_VisitXTimesMin,				-1										)
	EF_INT(			"Visit Only X times Max",				nCon_VisitXTimesMax,				-1										)
	EF_INT(			"Disappears X num of Hours Min",		nCon_AfterXNumberOfHoursMin,		-1										)
	EF_INT(			"Disappears X num of Hours Max",		nCon_AfterXNumberOfHoursMax,		-1										)
	EF_BOOL(		"Disappears After Unique Item Found",	bCon_AfterUniqueItemFound													)
	EF_BOOL(		"Disappears after Elite Boss Death",	bCon_AfterEliteBossFound													)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "LevelName")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
	EXCEL_ADD_ANCESTOR( DATATABLE_QUEST_TITLES_FOR_TUGBOAT )
	EXCEL_ADD_ANCESTOR( DATATABLE_QUESTS_TASKS_FOR_TUGBOAT )
	EXCEL_SET_POSTPROCESS_TABLE(LevelAreaExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS, LEVEL_AREA_DEFINITION, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "level_areas") EXCEL_TABLE_END


//----------------------------------------------------------------------------
// levelarea_linking.txt
EXCEL_STRUCT_DEF(LEVELAREA_LINK)	
	EF_LINK_INT(	"LevelArea",				m_LevelAreaID,					DATATABLE_LEVEL_AREAS					)	
	EF_LINK_INT(	"Previous",					m_LevelAreaIDPrevious,			DATATABLE_LEVEL_AREAS					)	
	EF_LINK_INT(	"Next",						m_LevelAreaIDNext,				DATATABLE_LEVEL_AREAS					)	
	EF_LINK_INT(	"LinkPointA",				m_LevelAreaIDLinkA,				DATATABLE_LEVEL_AREAS					)	
	EF_LINK_INT(	"LinkPointB",				m_LevelAreaIDLinkB,				DATATABLE_LEVEL_AREAS					)	
	EF_BOOL(		"CanTravelToOtherAreas",	m_CanTravelFrom															)
	EF_INT(			"HubRadius",				m_HubRadius,					75										)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "LevelArea")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_AREAS_LINKER, LEVELAREA_LINK, APP_GAME_TUGBOAT, EXCEL_TABLE_PRIVATE, "levelarea_linking") EXCEL_TABLE_END


//----------------------------------------------------------------------------
// wardrobe_appearance_group
STR_DICT sgtWardrobeAppearanceGroupCategoryIndex[] =
{
	{ "none",			-2	},
	{ "zombie",			0	},
	{ "minion",			1	},
	{ "3p gender",		2	},
	{ "3p faction",		3	},
	{ "1p gender",		4	},
	{ "1p faction",		5	},
	{ "body",			6	},
	{ "head",			7	},
	{ "hair",			8	},
	{ "face extras",	9	},
	{ "ignore",			-1	},
	{ NULL,				0	},
};
STR_DICT sgtWardrobeAppearanceGroupSectionIndex[] =
{
	{ "none",			-2									},
	{ "base",			WARDROBE_BODY_SECTION_BASE			},
	{ "head",			WARDROBE_BODY_SECTION_HEAD			},
	{ "hair",			WARDROBE_BODY_SECTION_HAIR			},
	{ "facial hair",	WARDROBE_BODY_SECTION_FACIAL_HAIR	},
	{ "skin",			WARDROBE_BODY_SECTION_SKIN	},
	{ "haircolor",		WARDROBE_BODY_SECTION_HAIR_COLOR	},
	{ NULL,				-1	},
};
STR_DICT sgtWardrobeBlendOpGroupIndex[] =
{
	{ "default",	BLENDOP_GROUP_DEFAULT	},
	{ "templar",	BLENDOP_GROUP_TEMPLAR	},
	{ "cabalist",	BLENDOP_GROUP_CABALIST	},
	{ "hunter",		BLENDOP_GROUP_HUNTER	},
	{ "satyr",		BLENDOP_GROUP_SATYR},
	{ "cyclops",	BLENDOP_GROUP_CYCLOPS},
	{ "cyclopsfemale",	BLENDOP_GROUP_CYCLOPSFEMALE},
	{ NULL,			BLENDOP_GROUP_DEFAULT	},
};

EXCEL_STRUCT_DEF(WARDROBE_APPEARANCE_GROUP_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_TWOCC(		"code",									wCode																		)
	EF_FILE(		"Blend Texture Folder",					pszBlendFolder																)
	EF_FILE(		"Texture Folder",						pszTextureFolder															)
	EF_FILE(		"Appearance",							pszAppearanceDefinition														)
	EF_DICT_INT(	"Category",								nCategory,							sgtWardrobeAppearanceGroupCategoryIndex	) EXCEL_SET_FLAG(EXCEL_FIELD_DICTIONARY_ALLOW_NEGATIVE)
	EF_DICT_INT(	"Section",								eBodySection,						sgtWardrobeAppearanceGroupSectionIndex	) EXCEL_SET_FLAG(EXCEL_FIELD_DICTIONARY_ALLOW_NEGATIVE)
	EF_DICT_INT(	"BlendOp Group",						eBlendOpGroup,						sgtWardrobeBlendOpGroupIndex			)
	EF_BOOL(		"No Body Parts",						bNoBodyParts																)
	EF_BOOL(		"Don't randomly pick",					bDontRandomlyPick															)
	EF_BOOL(		"Subscriber Only",						bSubscriberOnly																)
	EF_LINK_INT(	"First Person",							nFirstPersonGroup,					DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EF_LINK_IARR(	"Base",									pnBodySectionToAppearanceGroup[WARDROBE_BODY_SECTION_BASE],			DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EF_LINK_IARR(	"Heads",								pnBodySectionToAppearanceGroup[WARDROBE_BODY_SECTION_HEAD],			DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EF_LINK_IARR(	"Hair",									pnBodySectionToAppearanceGroup[WARDROBE_BODY_SECTION_HAIR],			DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EF_LINK_IARR(	"Facial Hair",							pnBodySectionToAppearanceGroup[WARDROBE_BODY_SECTION_FACIAL_HAIR],	DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EF_LINK_IARR(	"Skin",									pnBodySectionToAppearanceGroup[WARDROBE_BODY_SECTION_SKIN],			DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EF_LINK_IARR(	"Hair Color Texture",					pnBodySectionToAppearanceGroup[WARDROBE_BODY_SECTION_HAIR_COLOR],	DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_APPEARANCE_GROUP, WARDROBE_APPEARANCE_GROUP_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe_appearance_group") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_model_group.txt
EXCEL_STRUCT_DEF(WARDROBE_MODEL_GROUP)
	EF_STRING(		"Name",									pszName																		)
	EF_DICT_INT(	"Appearance Group Category",			nAppearanceGroupCategory,			sgtWardrobeAppearanceGroupCategoryIndex	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_MODEL_GROUP, WARDROBE_MODEL_GROUP, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe_model_group") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_model
STR_DICT sgtWardrobePartGroup[] =
{
	{ "",				-1	},
	{ "third person",	0	},
	{ "first person",	1	},
	{ "monster",		-1	},
	{ NULL,				0	},
};

EXCEL_STRUCT_DEF(WARDROBE_MODEL)
	EF_LINK_INT(	"Model Group",							nModelGroup,						DATATABLE_WARDROBE_MODEL_GROUP			)
	EF_LINK_INT(	"Appearance Group",						pnAppearanceGroup[ 0 ],				DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EF_LINK_INT(	"Appearance Group2",					pnAppearanceGroup[ 1 ],				DATATABLE_WARDROBE_APPEARANCE_GROUP		)
	EF_STRING(		"Filename",								pszFileName																	)
	EF_FILE(		"Default Material",						pszDefaultMaterial															)
	EF_DICT_INT(	"Part Group",							nPartGroup,							sgtWardrobePartGroup					)
	EF_INT(			"",										nMaterial,							INVALID_ID								)
	EF_FLOAT(		"BoxMinX",								tBoundingBox.vMin.fX,				-1.0f									)
	EF_FLOAT(		"BoxMinY",								tBoundingBox.vMin.fY,				-1.0f									)
	EF_FLOAT(		"BoxMinZ",								tBoundingBox.vMin.fZ,			    0.0f									)
	EF_FLOAT(		"BoxMaxX",								tBoundingBox.vMax.fX,			    1.0f									)
	EF_FLOAT(		"BoxMaxY",								tBoundingBox.vMax.fY,			    1.0f									)
	EF_FLOAT(		"BoxMaxZ",								tBoundingBox.vMax.fZ,			    2.0f									)
	EF_BOOL(		"",										bAsyncLoadRequested,				FALSE									)
	EF_BOOL(		"",										bLoadFailed[LOD_LOW],				FALSE									)
	EF_BOOL(		"",										bLoadFailed[LOD_HIGH],				FALSE									)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Model Group", "Appearance Group", "Appearance Group2")
	EXCEL_SET_POSTPROCESS_ROW(ExcelWardrobeModelPostProcessRow)
	EXCEL_SET_ROWFREE(ExcelWardrobeModelFreeRow)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_MODEL, WARDROBE_MODEL, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe_model") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_textureset_group
EXCEL_STRUCT_DEF(WARDROBE_TEXTURESET_GROUP)
	EF_STRING(		"Name",									pszName																		)
	EF_DICT_INT(	"Appearance Group Category",			nAppearanceGroupCategory,			sgtWardrobeAppearanceGroupCategoryIndex	)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_TEXTURESET_GROUP, WARDROBE_TEXTURESET_GROUP,	APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe_textureset_group") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_textureset
EXCEL_STRUCT_DEF(WARDROBE_TEXTURESET_DATA)
	EF_LINK_INT(	"TextureSet Group",						nTextureSetGroup,					DATATABLE_WARDROBE_TEXTURESET_GROUP		)
	EF_LINK_INT(	"Appearance Group 1",					pnAppearanceGroup[0],				DATATABLE_WARDROBE_APPEARANCE_GROUP		)		
	EF_LINK_INT(	"Appearance Group 2",					pnAppearanceGroup[1],				DATATABLE_WARDROBE_APPEARANCE_GROUP		)		
	EF_LINK_INT(	"Appearance Group Folder",				nAppearanceGroupForFolder,			DATATABLE_WARDROBE_APPEARANCE_GROUP		)		
	EF_FILE(		"Folder",								pszFolder																	)
	EF_STRING(		"Diffuse",								pszTextureFilenames[LAYER_TEXTURE_DIFFUSE]									)
	EF_STRING(		"Normal",								pszTextureFilenames[LAYER_TEXTURE_NORMAL]									)
	EF_STRING(		"Specular",								pszTextureFilenames[LAYER_TEXTURE_SPECULAR]									)
	EF_STRING(		"Lightmap",								pszTextureFilenames[LAYER_TEXTURE_SELFILLUM]									)
	EF_STRING(		"ColorMask",							pszTextureFilenames[LAYER_TEXTURE_COLORMASK]								)
	EF_BOOL(		"",										bLowOnly[LAYER_TEXTURE_DIFFUSE]												)
	EF_BOOL(		"",										bLowOnly[LAYER_TEXTURE_NORMAL]												)
	EF_BOOL(		"",										bLowOnly[LAYER_TEXTURE_SPECULAR]											)
	EF_BOOL(		"",										bLowOnly[LAYER_TEXTURE_SELFILLUM]											)
	EF_BOOL(		"",										bLowOnly[LAYER_TEXTURE_COLORMASK]											)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_DIFFUSE][LOD_HIGH],					INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_NORMAL][LOD_HIGH],					INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_SPECULAR][LOD_HIGH],					INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_SELFILLUM][LOD_HIGH],					INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_COLORMASK][LOD_HIGH],				INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_DIFFUSE][LOD_LOW],					INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_NORMAL][LOD_LOW],					INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_SPECULAR][LOD_LOW],					INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_SELFILLUM][LOD_LOW],					INVALID_ID	)
	EF_INT(			"",										pnTextureIds[LAYER_TEXTURE_COLORMASK][LOD_LOW],					INVALID_ID	)
	EF_INT(			"SizeDiffuseW",							pnTextureSizes[LAYER_TEXTURE_DIFFUSE][0]									)
	EF_INT(			"SizeDiffuseH",							pnTextureSizes[LAYER_TEXTURE_DIFFUSE][1]									)
	EF_INT(			"SizeNormalW",							pnTextureSizes[LAYER_TEXTURE_NORMAL][0]										)
	EF_INT(			"SizeNormalH",							pnTextureSizes[LAYER_TEXTURE_NORMAL][1]										)
	EF_INT(			"SizeSpecularW",						pnTextureSizes[LAYER_TEXTURE_SPECULAR][0]									)
	EF_INT(			"SizeSpecularH",						pnTextureSizes[LAYER_TEXTURE_SPECULAR][1]									)
	EF_INT(			"SizeLightmapW",						pnTextureSizes[LAYER_TEXTURE_SELFILLUM][0]									)
	EF_INT(			"SizeLightmapH",						pnTextureSizes[LAYER_TEXTURE_SELFILLUM][1]									)
	EF_INT(			"SizeColorMaskW",						pnTextureSizes[LAYER_TEXTURE_COLORMASK][0]									)
	EF_INT(			"SizeColorMaskH",						pnTextureSizes[LAYER_TEXTURE_COLORMASK][1]									)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "TextureSet Group", "Appearance Group 1", "Appearance Group 2")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelWardrobeTextureSetPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_TEXTURESET, WARDROBE_TEXTURESET_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe_textureset") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_part
STR_DICT sgtWardrobeTextureIndex[] =
{
	{ "",			0	},
	{ "body",		0	},
	{ "head",		1	},
	{ "cape",		2   }, // tugboat only 
	{ "insignia",	2   }, // hellgate only 
	{ "helmet",		3   }, // tugboat only
	{ "other",		1	}, // Byes, this has a 1 on purpose
	{ "right",		0	}, // yes, this has a 0 on purpose
	{ "left",		1	}, // yes, this has a 1 on purpose
	{ NULL,			0	},
};
EXCEL_STRUCT_DEF(WARDROBE_MODEL_PART_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_STRING(		"Label",								pszLabel																	)
	EF_INT(			"MaterialIndex",						nMaterialIndex,						-1										)
	EF_STRING(		"MaterialName",							pszMaterialName																)
	EF_DICT_INT(	"Target Texture",						nTargetTextureIndex,				sgtWardrobeTextureIndex					)
	EF_DICT_INT(	"Part Group",							nPartGroup,							sgtWardrobePartGroup					)
	EXCEL_SET_INDEX(0, 0, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_PART, WARDROBE_MODEL_PART_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe_part") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_blendop
EXCEL_STRUCT_DEF(WARDROBE_BLENDOP_DATA)
	EF_STRING(		"Name",									pszName																		)
	EF_STRING(		"Blend",								pszBlendTexture																)
	EF_LINK_IARR(	"Covers",								pnCovers,							DATATABLE_WARDROBE_BLENDOP				)
	EF_BOOL(		"No Texture Change",					bNoTextureChange															)
	EF_BOOL(		"Replace All Parts",					bReplaceAllParts															)
	EF_LINK_IARR(	"Remove Parts",							pnRemoveParts,						DATATABLE_WARDROBE_PART					)
	EF_LINK_IARR(	"Add Parts",							pnAddParts,							DATATABLE_WARDROBE_PART					)
	EF_DICT_INT(	"Target",								nTargetTextureIndex,				sgtWardrobeTextureIndex					)
	EF_INT(			"",										nBlendTexture,						INVALID_ID								)
	EXCEL_SET_INDEX(0, 0, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_BLENDOP,	WARDROBE_BLENDOP_DATA,	APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe_blendop") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_layerset
EXCEL_STRUCT_DEF(WARDROBE_LAYERSET_DATA)
	EF_STRING(		"Name",									pszName																		)
	EXCEL_SET_INDEX(0, 0, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_LAYERSET,	WARDROBE_BLENDOP_DATA,	APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "wardrobe_layerset") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_layer_player.txt, wardrobe_layer_mod.txt
STR_DICT sgtWardrobeAttachmentTypeIndex[] =
{
	{ "",			0	},
	{ "model",		ATTACHMENT_MODEL	},
	{ "particle",	ATTACHMENT_PARTICLE	},
	{ "light",		ATTACHMENT_LIGHT	},
	{ NULL,			0	},
};
STR_DICT sgtWardrobeBoneGroup[] =
{
	{ "",					0	},
	{ "slot_fuel",			1	},
	{ "slot_rocketclip",	2	},
	{ "slot_battery",		3	},
	{ "slot_relic",			4	},
	{ "slot_tech",			5	},
	{ NULL,	0 },
};
#define WARDROBE_RANDOM_BODY_GROUP_INDEX 2

EXCEL_STRUCT_DEF(WARDROBE_LAYER_DATA)
	EF_STRING(		"Name",									pszName																				)
	EF_TWOCC(		"code",									wCode																				)
	EF_INT(			"Order",								nOrder																				)
	EF_BOOL(		"Debug",								bDebugOnly																			)
	EF_LINK_INT(	"Model Group",							nModelGroup,						DATATABLE_WARDROBE_MODEL_GROUP					)
	EF_LINK_INT(	"TextureSet Group",						nTextureSetGroup,					DATATABLE_WARDROBE_TEXTURESET_GROUP				)
	EF_LINK_INT(	"BlendOp",								pnWardrobeBlendOp[0],				DATATABLE_WARDROBE_BLENDOP						)
	EF_LINK_INT(	"BlendOp Templar",						pnWardrobeBlendOp[1],				DATATABLE_WARDROBE_BLENDOP						)
	EF_LINK_INT(	"BlendOp Cabalist",						pnWardrobeBlendOp[2],				DATATABLE_WARDROBE_BLENDOP						)
	EF_LINK_INT(	"BlendOp Hunter",						pnWardrobeBlendOp[3],				DATATABLE_WARDROBE_BLENDOP						)
	EF_LINK_INT(	"BlendOp Satyr",						pnWardrobeBlendOp[4],				DATATABLE_WARDROBE_BLENDOP						)
	EF_LINK_INT(	"BlendOp Cyclops",						pnWardrobeBlendOp[5],				DATATABLE_WARDROBE_BLENDOP						)
	EF_LINK_INT(	"BlendOp Cyclops Female",				pnWardrobeBlendOp[6],				DATATABLE_WARDROBE_BLENDOP						)
	EF_LINK_INT(	"Offhand Layer",						nOffhandLayer,						DATATABLE_WARDROBE_LAYER						)
	EF_STRING(		"Attach Name",							tAttach.pszAttached																	)
	EF_STRING(		"Attach Bone",							tAttach.pszBone																		)
	EF_DICT_INT(	"Bone Index",							nBoneGroup,							sgtWardrobeBoneGroup							)
	EF_DICT_INT(	"Attach Type",							tAttach.eType,						sgtWardrobeAttachmentTypeIndex					)
	EF_INT(			"",										tAttach.nAttachedDefId,				INVALID_ID										)
	EF_INT(			"",										tAttach.nBoneId,					INVALID_ID										)
	EF_BOOL(		"Has Bone Index",						bHasBoneIndex																		)
	EF_LINK_IARR(	"Layer Set",							pnLayerSet,							DATATABLE_WARDROBE_LAYERSET						)
	EF_INT(			"Row Collection",						nRowCollection																		)
	EF_LINK_IARR(	"Random Appearance Groups",				pnRandomAppearanceGroups,			DATATABLE_WARDROBE_APPEARANCE_GROUP				)
	EF_LINK_INT(	"State",								nState,								DATATABLE_STATES								)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "code")
	EXCEL_SET_POSTPROCESS_ALL(ExcelWardrobePostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_LAYER, WARDROBE_LAYER_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe")
	EXCEL_TABLE_ADD_FILE("wardrobe_layer_players", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("wardrobe_layer_mods", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
EXCEL_TABLE_END

//----------------------------------------------------------------------------
// wardrobe_body
EXCEL_STRUCT_DEF(WARDROBE_BODY_DATA)
	EF_STRING(		"Name",									pszName																				)
	EF_BOOL(		"Randomize",							bRandomizeBody																		)
	EF_LINK_IARR(	"Random LayerSets",						pnRandomLayerSets,					DATATABLE_WARDROBE_LAYERSET						)
	EF_LINK_IARR(	"Layers",								tBody.pnLayers,						DATATABLE_WARDROBE_LAYER						)
	EF_LINK_INT(	"Base",									tBody.pnAppearanceGroups[WARDROBE_BODY_SECTION_BASE],			DATATABLE_WARDROBE_APPEARANCE_GROUP	)
	EF_LINK_INT(	"Head",									tBody.pnAppearanceGroups[WARDROBE_BODY_SECTION_HEAD],			DATATABLE_WARDROBE_APPEARANCE_GROUP	)
	EF_LINK_INT(	"Hair",									tBody.pnAppearanceGroups[WARDROBE_BODY_SECTION_HAIR],			DATATABLE_WARDROBE_APPEARANCE_GROUP	)
	EF_LINK_INT(	"Facial Hair",							tBody.pnAppearanceGroups[WARDROBE_BODY_SECTION_FACIAL_HAIR],	DATATABLE_WARDROBE_APPEARANCE_GROUP	)
	EF_LINK_INT(	"Skin",									tBody.pnAppearanceGroups[WARDROBE_BODY_SECTION_SKIN],			DATATABLE_WARDROBE_APPEARANCE_GROUP	)
	EF_LINK_INT(	"Hair Color Texture",					tBody.pnAppearanceGroups[WARDROBE_BODY_SECTION_HAIR_COLOR],		DATATABLE_WARDROBE_APPEARANCE_GROUP	)
	EF_BYTE(		"Skin Color",							tBody.pbBodyColors[WARDROBE_BODY_COLOR_SKIN],					0					)
	EF_BYTE(		"Hair Color",							tBody.pbBodyColors[WARDROBE_BODY_COLOR_HAIR],					0					)
	EXCEL_SET_INDEX(0, 0, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_WARDROBE_BODY, WARDROBE_BODY_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "wardrobe_body") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// meleeweapons
EXCEL_STRUCT_DEF(MELEE_WEAPON_DATA)
	EF_STRING(		"name",									szName																						)
	EF_FILE(		"swing events",							szSwingEvents																				)
	EF_INT(			"",										nSwingEventsId,						INVALID_ID												)
	EF_FILE(		"Particle Folder",						szEffectFolder																				)
	EF_DSTRING(		"Tiny Hit",								pEffects[COMBAT_RESULT_HIT_TINY].szDefault													)
	EF_DSTRING(		"Tiny Default",							pEffects[COMBAT_RESULT_HIT_TINY].tElementEffect[DAMAGE_TYPE_ALL].szPerElement				)
	EF_DSTRING(		"Tiny Toxic",							pEffects[COMBAT_RESULT_HIT_TINY].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement				)
	EF_DSTRING(		"Tiny Poison",							pEffects[COMBAT_RESULT_HIT_TINY].tElementEffect[DAMAGE_TYPE_POISON].szPerElement			)	
	EF_DSTRING(		"Tiny Spectral",						pEffects[COMBAT_RESULT_HIT_TINY].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement			)
	EF_DSTRING(		"Tiny Fire",							pEffects[COMBAT_RESULT_HIT_TINY].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement				)
	EF_DSTRING(		"Tiny Electric",						pEffects[COMBAT_RESULT_HIT_TINY].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement		)
	EF_DSTRING(		"Tiny Physical",						pEffects[COMBAT_RESULT_HIT_TINY].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement			)
	EF_DSTRING(		"Soft Hit",								pEffects[COMBAT_RESULT_HIT_LIGHT].szDefault													)
	EF_DSTRING(		"Soft Default",							pEffects[COMBAT_RESULT_HIT_LIGHT].tElementEffect[DAMAGE_TYPE_ALL].szPerElement				)
	EF_DSTRING(		"Soft Toxic",							pEffects[COMBAT_RESULT_HIT_LIGHT].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement			)
	EF_DSTRING(		"Soft Poison",							pEffects[COMBAT_RESULT_HIT_LIGHT].tElementEffect[DAMAGE_TYPE_POISON].szPerElement			)
	EF_DSTRING(		"Soft Spectral",						pEffects[COMBAT_RESULT_HIT_LIGHT].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement			)
	EF_DSTRING(		"Soft Fire",							pEffects[COMBAT_RESULT_HIT_LIGHT].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement				)
	EF_DSTRING(		"Soft Electric",						pEffects[COMBAT_RESULT_HIT_LIGHT].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement		)
	EF_DSTRING(		"Soft Physical",						pEffects[COMBAT_RESULT_HIT_LIGHT].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement			)
	EF_DSTRING(		"Medium Hit",							pEffects[COMBAT_RESULT_HIT_MEDIUM].szDefault												)
	EF_DSTRING(		"Medium Default",						pEffects[COMBAT_RESULT_HIT_MEDIUM].tElementEffect[DAMAGE_TYPE_ALL].szPerElement				)
	EF_DSTRING(		"Medium Toxic",							pEffects[COMBAT_RESULT_HIT_MEDIUM].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement			)
	EF_DSTRING(		"Medium Poison",						pEffects[COMBAT_RESULT_HIT_MEDIUM].tElementEffect[DAMAGE_TYPE_POISON].szPerElement			)
	EF_DSTRING(		"Medium Spectral",						pEffects[COMBAT_RESULT_HIT_MEDIUM].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement		)
	EF_DSTRING(		"Medium Fire",							pEffects[COMBAT_RESULT_HIT_MEDIUM].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement			)
	EF_DSTRING(		"Medium Electric",						pEffects[COMBAT_RESULT_HIT_MEDIUM].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement		)
	EF_DSTRING(		"Medium Physical",						pEffects[COMBAT_RESULT_HIT_MEDIUM].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement		)
	EF_DSTRING(		"Hard Hit",								pEffects[COMBAT_RESULT_HIT_HARD].szDefault													)
	EF_DSTRING(		"Hard Default",							pEffects[COMBAT_RESULT_HIT_HARD].tElementEffect[DAMAGE_TYPE_ALL].szPerElement				)
	EF_DSTRING(		"Hard Toxic",							pEffects[COMBAT_RESULT_HIT_HARD].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement				)
	EF_DSTRING(		"Hard Poison",							pEffects[COMBAT_RESULT_HIT_HARD].tElementEffect[DAMAGE_TYPE_POISON].szPerElement			)	
	EF_DSTRING(		"Hard Spectral",						pEffects[COMBAT_RESULT_HIT_HARD].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement			)
	EF_DSTRING(		"Hard Fire",							pEffects[COMBAT_RESULT_HIT_HARD].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement				)
	EF_DSTRING(		"Hard Electric",						pEffects[COMBAT_RESULT_HIT_HARD].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement		)
	EF_DSTRING(		"Hard Physical",						pEffects[COMBAT_RESULT_HIT_HARD].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement			)
	EF_DSTRING(		"Kill",									pEffects[COMBAT_RESULT_KILL].szDefault														)
	EF_DSTRING(		"Kill Default",							pEffects[COMBAT_RESULT_KILL].tElementEffect[DAMAGE_TYPE_ALL].szPerElement					)
	EF_DSTRING(		"Kill Toxic",							pEffects[COMBAT_RESULT_KILL].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement					)
	EF_DSTRING(		"Kill Poison",							pEffects[COMBAT_RESULT_KILL].tElementEffect[DAMAGE_TYPE_POISON].szPerElement				)
	EF_DSTRING(		"Kill Spectral",						pEffects[COMBAT_RESULT_KILL].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement				)
	EF_DSTRING(		"Kill Fire",							pEffects[COMBAT_RESULT_KILL].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement					)
	EF_DSTRING(		"Kill Electric",						pEffects[COMBAT_RESULT_KILL].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement			)
	EF_DSTRING(		"Kill Physical",						pEffects[COMBAT_RESULT_KILL].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement				)
	EF_DSTRING(		"Block",								pEffects[COMBAT_RESULT_BLOCK].szDefault														)
	EF_DSTRING(		"Block Default",						pEffects[COMBAT_RESULT_BLOCK].tElementEffect[DAMAGE_TYPE_ALL].szPerElement					)
	EF_DSTRING(		"Block Toxic",							pEffects[COMBAT_RESULT_BLOCK].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement					)
	EF_DSTRING(		"Block Poison",							pEffects[COMBAT_RESULT_BLOCK].tElementEffect[DAMAGE_TYPE_POISON].szPerElement				)
	EF_DSTRING(		"Block Spectral",						pEffects[COMBAT_RESULT_BLOCK].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement				)
	EF_DSTRING(		"Block Fire",							pEffects[COMBAT_RESULT_BLOCK].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement					)
	EF_DSTRING(		"Block Electric",						pEffects[COMBAT_RESULT_BLOCK].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement			)
	EF_DSTRING(		"Block Physical",						pEffects[COMBAT_RESULT_BLOCK].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement				)
	EF_DSTRING(		"Fumble",								pEffects[COMBAT_RESULT_FUMBLE].szDefault														)
	EF_DSTRING(		"Fumble Default",						pEffects[COMBAT_RESULT_FUMBLE].tElementEffect[DAMAGE_TYPE_ALL].szPerElement					)
	EF_DSTRING(		"Fumble Toxic",							pEffects[COMBAT_RESULT_FUMBLE].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement					)
	EF_DSTRING(		"Fumble Poison",						pEffects[COMBAT_RESULT_FUMBLE].tElementEffect[DAMAGE_TYPE_POISON].szPerElement				)
	EF_DSTRING(		"Fumble Spectral",						pEffects[COMBAT_RESULT_FUMBLE].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement				)
	EF_DSTRING(		"Fumble Fire",							pEffects[COMBAT_RESULT_FUMBLE].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement					)
	EF_DSTRING(		"Fumble Electric",						pEffects[COMBAT_RESULT_FUMBLE].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement			)
	EF_DSTRING(		"Fumble Physical",						pEffects[COMBAT_RESULT_FUMBLE].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MELEEWEAPONS, MELEE_WEAPON_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "meleeweapons") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// condition_functions.txt
EXCEL_STRUCT_DEF(CONDITION_FUNCTION_DATA)
	EF_STRING(		"Name",									szName																				)
	EF_STRING(		"Function",								szFunction																			)	
	EF_BOOL(		"Not",									bNot																				)
	EF_BOOL(		"Uses State",							bUsesState																			)
	EF_BOOL(		"Uses UnitType",						bUsesUnitType																		)
	EF_BOOL(		"Uses Skill",							bUsesSkill																			)
	EF_BOOL(		"Uses Monster Class",					bUsesMonsterClass																	)
	EF_BOOL(		"Uses Object Class",					bUsesObjectClass																	)
	EF_BOOL(		"Uses Stat",							bUsesStat																			)
	EF_STRING(		"Param Text 0",							pszParamText[0]																		)
	EF_STRING(		"Param Text 1",							pszParamText[1]																		)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ConditionFunctionsExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_CONDITION_FUNCTIONS, CONDITION_FUNCTION_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "condition_functions") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// item_levels.txt
EXCEL_STRUCT_DEF(ITEM_LEVEL)
	EF_INT(			"level",								nLevel																				)
	EF_INT(			"base damage mult",						nDamageMult																			)
	EF_INT(			"shields",								nBaseShields																		)
	EF_INT(			"shield buffer",						nBaseShieldBuffer																	)
	EF_INT(			"shield regen",							nBaseShieldRegen																	)
	EF_INT(			"armor",								nBaseArmor																			)
	EF_INT(			"armor buffer",							nBaseArmorBuffer																	)
	EF_INT(			"armor regen",							nBaseArmorRegen																		)
	EF_INT(			"feed",									nBaseFeed																			)
	EF_INT(			"sfx attack ability",					nBaseSfxAttackAbility																)
	EF_INT(			"sfx defense ability",					nBaseSfxDefenseAbility																)
	EF_INT(			"interrupt attack",						nBaseInterruptAttack																)
	EF_INT(			"interrupt defense",					nBaseInterruptDefense																)
	EF_INT(			"stealth attack",						nBaseStealthAttack																	)
	EF_INT(			"ai changer attack",					nBaseAIChangerAttack																)
	EF_INT(			"level requirement",					nLevelRequirement																	)
	EF_INT(			"item level min",						nItemLevelRequirement																)
	EF_INT(			"item level max",						nItemLevelLimit																		)
	EF_CODE(		"buy price base",						codeBuyPriceBase																	)
	EF_CODE(		"sell price base",						codeSellPriceBase																	)
	EF_INT(			"augment cost common",					nAugmentCost[ IAUGT_ADD_COMMON ]													)
	EF_INT(			"augment cost rare",					nAugmentCost[ IAUGT_ADD_RARE ]														)
	EF_INT(			"augment cost legendary",				nAugmentCost[ IAUGT_ADD_LEGENDARY ]													)
	EF_INT(			"augment cost random",					nAugmentCost[ IAUGT_ADD_RANDOM ]													)
	EF_INT(			"scrap upgrade quantity",				nQuantityScrap																		)
	EF_INT(			"special scrap upgrade quantity",		nQuantityScrapSpecial																)
	EF_INT(			"item levels per upgrade",				nItemLevelsPerUpgrade																)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ITEM_LEVELS, ITEM_LEVEL, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "item_levels") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// monsters.txt, players.txt, objects.txt, weapons.txt, misc.txt, armor.txt, mods.txt
STR_DICT sgtGenderDictionary[] = 
{
	{ "" ,		GENDER_INVALID },
	{ "male" ,	GENDER_MALE	},
	{ "female",	GENDER_FEMALE	},
	{ NULL ,	GENDER_INVALID }
};

STR_DICT tDictVisualPortalDirection[] = 
{
	{ "" ,		VPD_INVALID },
	{ "from" ,	VPD_FROM	},
	{ "to",		VPD_TO		},
	{ NULL ,	VPD_INVALID }
};

STR_DICT tDictRoomPopulatePass[] = 
{
	{ "" ,					RPP_CONTENT				},
	{ "sublevel_populate",	RPP_SUBLEVEL_POPULATE	},
	{ "setup",				RPP_SETUP				},
	{ "content",			RPP_CONTENT				},
	{ NULL,					RPP_CONTENT				}
};

STR_DICT tDictWarpResolvePass[] = 
{
	{ "" ,					WRP_SUBLEVEL_POPULATE	},
	{ "sublevel_populate",	WRP_SUBLEVEL_POPULATE	},
	{ "level_activate",		WRP_LEVEL_FIRST_ACTIVATE},
	{ "on_use",				WRP_ON_USE				},
	
	{ NULL,					WRP_SUBLEVEL_POPULATE	}
};

STR_DICT tDictQualityName[] = 
{
	{ "" ,					QN_INVALID					},
	{ "itemquality",		QN_ITEM_QUALITY_DATATABLE	},
	{ "affix",				QN_AFFIX_DATATABLE			},
	{ NULL,					QN_INVALID					}
};

//----------------------------------------------------------------------------
STR_DICT sgtDictWeaponBoneIndex[] = 
{
	{ "" ,					WEAPON_BONE_GENERIC },
	{ "generic",			WEAPON_BONE_GENERIC },
	{ "focus",				WEAPON_BONE_FOCUS },
	{ NULL,					WEAPON_BONE_GENERIC }
};

//----------------------------------------------------------------------------
STR_DICT tDictRMTTangibilityEnum[] =
{
	{ "",				RMT_TANGIBLE_NONE },
	{ "item",			RMT_TANGIBLE_ITEM },
	{ "accountwide",	RMT_TANGIBLE_ACCOUNTWIDE },
	{ NULL,				RMT_TANGIBLE_NONE}
};

//----------------------------------------------------------------------------
STR_DICT tDictRMTPricingEnum[] =
{
	{ "",				RMT_PRICING_NONE },
	{ "onetime",		RMT_PRICING_ONETIME },
	{ "recurring",		RMT_PRICING_RECURRING },
	{ NULL,				RMT_PRICING_NONE}
};

EXCEL_STRUCT_DEF(UNIT_DATA)
	EF_INHERIT_ROW( "base row",								nUnitDataBaseRow																	)	
	EF_DSTR_LOWER(	"name",									szName																				)
	EF_DSTRING(		"folder",								szFileFolder																		)
	EF_DSTRING(		"appearance",							szAppearance																		)
	EF_DSTRING(		"appearance first",						szAppearance_FirstPerson															)
	EF_DSTRING(		"icon",									szIcon																				)
	EF_DSTRELE(		"diffuse",								szTextureOverrides,					UNIT_TEXTURE_OVERRIDE_DIFFUSE					)
	EF_DSTRELE(		"normal",								szTextureOverrides,					UNIT_TEXTURE_OVERRIDE_NORMAL 					)
	EF_DSTRELE(		"specular",								szTextureOverrides,					UNIT_TEXTURE_OVERRIDE_SPECULAR					)
	EF_DSTRELE(		"lightmap",								szTextureOverrides,					UNIT_TEXTURE_OVERRIDE_SELFILLUM					)
	EF_DSTRELE(		"override source 1",					szTextureSpecificOverrides,			0												)
	EF_DSTRELE(		"override source 2",					szTextureSpecificOverrides,			2												)
	EF_DSTRELE(		"override source 3",					szTextureSpecificOverrides,			4												)
	EF_DSTRELE(		"override source 4",					szTextureSpecificOverrides,			6												)
	EF_DSTRELE(		"override source 5",					szTextureSpecificOverrides,			8												)
	EF_DSTRELE(		"override dest 1",						szTextureSpecificOverrides,			1												)
	EF_DSTRELE(		"override dest 2",						szTextureSpecificOverrides,			3												)
	EF_DSTRELE(		"override dest 3",						szTextureSpecificOverrides,			5												)
	EF_DSTRELE(		"override dest 4",						szTextureSpecificOverrides,			7												)
	EF_DSTRELE(		"override dest 5",						szTextureSpecificOverrides,			9												)
	EF_DSTRING(		"holy radius",							szHolyRadius																		)
	EF_DSTRING(		"particle folder",						szParticleFolder																	)
	EF_DSTRELE(		"tiny hit particle",					szBloodParticles,					COMBAT_RESULT_HIT_TINY							)
	EF_DSTRELE(		"light hit particle",					szBloodParticles,					COMBAT_RESULT_HIT_LIGHT							)
	EF_DSTRELE(		"medium hit particle",					szBloodParticles,					COMBAT_RESULT_HIT_MEDIUM						)
	EF_DSTRELE(		"hard hit particle",					szBloodParticles,					COMBAT_RESULT_HIT_HARD							)
	EF_DSTRELE(		"killed particle",						szBloodParticles,					COMBAT_RESULT_KILL								)
	EF_DSTRING(		"fizzle particle",						szFizzleParticle																	)
	EF_DSTRING(		"reflect particle",						szReflectParticle																	)
	EF_DSTRING(		"RestoreVitalsParticle",				szRestoreVitalsParticle																)	
	EF_DSTRING(		"damaging melee particle",				szDamagingMeleeParticle																	)	
	EF_LINK_INT(	"ColorSet",								nColorSet,							DATATABLE_COLORSETS								)	
	EF_TWOCC(		"code",									wCode																				)
	EF_LINK_INT(	"unittype",								nUnitType,							DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"respawn spawnclass",					nRespawnSpawnClass,					DATATABLE_SPAWN_CLASS							)
	EF_LINK_INT(	"respawn monsterclass",					nRespawnMonsterClass,				DATATABLE_MONSTERS							)
	EF_INT(			"respawn chance",						nRespawnPercentChance,				100												)
	EF_FLOAT(		"respawn radius",						fRespawnRadius,						0												)
	EF_LINK_INT(	"unittype for leveling",				nUnitTypeForLeveling,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"prefered by unittype",					nPreferedByUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"family",								nFamily,							DATATABLE_MONSTERS								)
	EF_LINK_INT(	"censor class no humans",				nCensorBackupClass_NoHumans,		DATATABLE_MONSTERS								)
	EF_LINK_INT(	"censor class no gore",					nCensorBackupClass_NoGore,			DATATABLE_MONSTERS								)
	EF_DICT_INT(	"sex",									eGender,							sgtGenderDictionary								)	
	EF_LINK_INT(	"race",									nRace,								DATATABLE_PLAYER_RACE							)		
	EF_LINK_INT(	"paperdoll background level",			nPaperdollBackgroundLevel,			DATATABLE_LEVEL									)
	EF_LINK_IARR(	"paperdoll weapon",						pnPaperdollWeapons,					DATATABLE_ITEMS									)
	EF_LINK_INT(	"paperdoll skill",						nPaperdollSkill,					DATATABLE_SKILLS								)
	EF_LINK_INT(	"paperdoll colorset",					nPaperdollColorset,					DATATABLE_COLORSETS								)
	EF_DSTRING(		"char select font",						szCharSelectFont																	)
	EF_LINK_INT(	"monster quality",						nMonsterQualityRequired,			DATATABLE_MONSTER_QUALITY						)	
	EF_LINK_INT(	"monster class at unique quality",		nMonsterClassAtUniqueQuality,		DATATABLE_MONSTERS								)		
	EF_LINK_INT(	"monster class at champion quality",	nMonsterClassAtChampionQuality,		DATATABLE_MONSTERS								)		
	EF_LINK_INT(	"item quality",							nItemQualityRequired,				DATATABLE_ITEM_QUALITY							)		
	EF_LINK_INT(	"minion class",							nMinionClass,						DATATABLE_MONSTERS								)		
	EF_LINK_INT(	"monster name type",					nMonsterNameType,					DATATABLE_MONSTER_NAME_TYPES					)		
	EF_INT(			"Density Value Override",				nDensityValueOverride,				1												)
	EF_CODE(		"MinionPackSize",						codeMinionPackSize																	)	
	EF_FLAG(		"unidentified name from base row",		pdwFlags,							UNIT_DATA_FLAG_UNIDENTIFIED_NAME				)
	EF_FLAG(		"no random proper name",				pdwFlags,							UNIT_DATA_FLAG_NO_RANDOM_PROPER_NAME			)	
	EF_FLAG(		"no name modifications",				pdwFlags,							UNIT_DATA_FLAG_NO_NAME_MODIFICATIONS			)	
	EF_FLAG(		"draw using cut up wardrobe",			pdwFlags,							UNIT_DATA_FLAG_DRAW_USING_CUT_UP_WARDROBE		)
	EF_DICT_INT(	"weapon bone index",					nWeaponBoneIndex,					sgtDictWeaponBoneIndex							)
	EF_FLAG(		"die on client trigger",				pdwFlags,							UNIT_DATA_FLAG_DIE_ON_CLIENT_TRIGGER			)
	EF_FLAG(		"trigger",								pdwFlags,							UNIT_DATA_FLAG_TRIGGER							)
	EF_FLAG(		"uses skills",							pdwFlags,							UNIT_DATA_FLAG_USES_SKILLS						)
	EF_FLAG(		"never destroy dead",					pdwFlags,							UNIT_DATA_FLAG_NEVER_DESTROY_DEAD				)
	EF_FLAG(		"collide when dead",					pdwFlags,							UNIT_DATA_FLAG_COLLIDE_WHEN_DEAD				)
	EF_FLAG(		"Start dead",							pdwFlags,							UNIT_DATA_FLAG_START_DEAD						)
	EF_FLAG(		"gives loot",							pdwFlags,							UNIT_DATA_FLAG_GIVES_LOOT						)
	EF_FLAG(		"dont trigger by proximity",			pdwFlags,							UNIT_DATA_FLAG_DONT_TRIGGER_BY_PROXIMITY		)
	EF_FLAG(		"Ignores Skill Power Cost",				pdwFlags,							UNIT_DATA_FLAG_IGNORES_POWER_COST_FOR_SKILLS	)
	EF_FLAG(		"trigger on enter room",				pdwFlags,							UNIT_DATA_FLAG_TRIGGER_ON_ENTER_ROOM			)
	EF_FLAG(		"destructible",							pdwFlags,							UNIT_DATA_FLAG_DESTRUCTIBLE						)
	EF_DICT_INT(	"room populate pass",					eRoomPopulatePass,					tDictRoomPopulatePass							)	
	EF_FLAG(		"preload",								pdwFlags,							UNIT_DATA_FLAG_PRELOAD							)
	EF_FLAG(		"ignore in dat",						pdwFlags,							UNIT_DATA_FLAG_IGNORE_IN_DAT					)
	EF_FLAG(		"ignore saved states",					pdwFlags,							UNIT_DATA_FLAG_IGNORE_SAVED_STATES				)
	EF_FLAG(		"is good",								pdwFlags,							UNIT_DATA_FLAG_IS_GOOD							)
	EF_FLAG(		"is npc",								pdwFlags,							UNIT_DATA_FLAG_IS_NPC							)
	EF_FLAG(		"has quest info",						pdwFlags,							UNIT_DATA_FLAG_QUESTS_CAST_MEMBER				)
	EF_FLAG(		"cannot be moved",						pdwFlags,							UNIT_DATA_FLAG_CANNOT_BE_MOVED					)
	EF_FLAG(		"in air",								pdwFlags,							UNIT_DATA_FLAG_SPAWN_IN_AIR						)
	EF_FLAG(		"wall walk",							pdwFlags,							UNIT_DATA_FLAG_WALL_WALK						)
	EF_FLAG(		"uses petlevel",						pdwFlags,							UNIT_DATA_FLAG_USES_PETLEVEL					)
	EF_FLAG(		"cannot turn",							pdwFlags,							UNIT_DATA_FLAG_CANNOT_TURN						)
	EF_FLAG(		"turn neck instead of body",			pdwFlags,							UNIT_DATA_FLAG_TURN_NECK_INSTEAD_OF_BODY		)
	EF_FLAG(		"Merchant",								pdwFlags,							UNIT_DATA_FLAG_IS_MERCHANT						)
	EF_FLAG(		"Merchant Shared Inventory",			pdwFlags,							UNIT_DATA_FLAG_MERCHANT_SHARES_INVENTORY		)	
	EF_INT(			"Merchant Starting Pane",				nMerchantStartingPane,				INVALID_ID										)
	EF_FLAG(		"Merchant Does Not Refresh",			pdwFlags,							UNIT_DATA_FLAG_MERCHANT_NO_REFRESH				)	
	EF_LINK_INT(	"MerchantFactionType",					nMerchantFactionType,				DATATABLE_FACTION								)
	EF_INT(			"MerchantFactionValueNeeded",			nMerchantFactionValueNeeded															)
	EF_FLAG(		"Trader",								pdwFlags,							UNIT_DATA_FLAG_IS_TRADER						)	
	EF_FLAG(		"Healer",								pdwFlags,							UNIT_DATA_FLAG_IS_HEALER						)
	EF_FLAG(		"Gravekeeper",							pdwFlags,							UNIT_DATA_FLAG_IS_GRAVEKEEPER					)	
	EF_FLAG(		"TaskGiver",							pdwFlags,							UNIT_DATA_FLAG_IS_TASKGIVER						)
	EF_FLAG(		"TaskGiver No Starting Icon",			pdwFlags,							UNIT_DATA_FLAG_IS_TASKGIVER_NO_STARTING_ICON	)	
	EF_FLAG(		"Tradesman",							pdwFlags,							UNIT_DATA_FLAG_IS_TRADESMAN						)
	EF_FLAG(		"Gambler",								pdwFlags,							UNIT_DATA_FLAG_IS_GAMBLER						)
	EF_FLAG(		"Stat Trader",							pdwFlags,							UNIT_DATA_FLAG_IS_STATTRADER					)
	EF_FLAG(		"MapVendor",							pdwFlags,							UNIT_DATA_FLAG_IS_MAP_VENDOR					)
	EF_FLAG(		"Trainer",								pdwFlags,							UNIT_DATA_FLAG_IS_TRAINER						)
	EF_FLAG(		"Transporter",							pdwFlags,							UNIT_DATA_FLAG_IS_TRANSPORTER					)
	EF_FLAG(		"GodQuestMessanger",					pdwFlags,							UNIT_DATA_FLAG_IS_GOD_MESSENGER					)
	EF_LINK_IARR(	"Recipes To Sell By Unittype",			nRecipeToSellByUnitType,			DATATABLE_UNITTYPES								)
	EF_LINK_IARR(	"Recipes to Sell By Not Unittype",		nRecipeToSellByUnitTypeNot,			DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Recipe Pane",							nRecipePane,						DATATABLE_SKILLTABS								)
	EF_LINK_INT(	"Recipe List",							nRecipeList,						DATATABLE_RECIPELISTS							)
	EF_LINK_INT(	"Recipe Single Use",					nRecipeSingleUse,					DATATABLE_RECIPES								)
	EF_LINK_INT("Merchant Not Available Till Quest Task Complete",	nMerchantNotAvailableTillQuest,		DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)
	EF_FLAG(		"Can Upgrade Items",					pdwFlags,							UNIT_DATA_FLAG_ITEM_UPGRADER					)	
	EF_FLAG(		"Can Augment Items",					pdwFlags,							UNIT_DATA_FLAG_ITEM_AUGMENTER					)	
	EF_FLAG(		"Auto Identifies Inventory",			pdwFlags,							UNIT_DATA_FLAG_AUTO_IDENTIFIES_INVENTORY		)	
	EF_FLAG(		"NPCGuildMaster",						pdwFlags,							UNIT_DATA_FLAG_IS_GUILDMASTER					)
	EF_FLAG(		"NPCRespeccer",							pdwFlags,							UNIT_DATA_FLAG_IS_RESPECCER						)
	EF_FLAG(		"NPCCraftingRespeccer",					pdwFlags,							UNIT_DATA_FLAG_IS_CRAFTING_RESPECCER			)
	EF_FLAG(		"NPCDungeonWarp",						pdwFlags,							UNIT_DATA_FLAG_IS_DUNGEON_WARPER				)
	EF_FLAG(		"PvPSignerUpper",						pdwFlags,							UNIT_DATA_FLAG_IS_PVP_SIGNER_UPPER				)
	EF_FLAG(		"Foreman",								pdwFlags,							UNIT_DATA_FLAG_IS_FOREMAN						)
	EF_FLAG(		"QuestImportantInfo",					pdwFlags,							UNIT_DATA_FLAG_QUEST_IMPORTANT_INFO				)
	EF_FLAG(		"AskQuestsForOperate",					pdwFlags,							UNIT_DATA_FLAG_ASK_QUESTS_FOR_OPERATE			)
	EF_FLAG(		"AskQuestsForKnown",					pdwFlags,							UNIT_DATA_FLAG_ASK_QUESTS_FOR_KNOWN				)	
	EF_FLAG(		"AskQuestsForVisible",					pdwFlags,							UNIT_DATA_FLAG_ASK_QUESTS_FOR_VISIBLE			)
	EF_FLAG(		"AskFactionForOperate",					pdwFlags,							UNIT_DATA_FLAG_ASK_FACTION_FOR_OPERATE			)
	EF_FLAG(		"AskPvPCensorshipForOperate",			pdwFlags,							UNIT_DATA_FLAG_ASK_PVP_CENSORSHIP_FOR_OPERATE	)
	EF_FLAG(		"Structural",							pdwFlags,							UNIT_DATA_FLAG_STRUCTURAL						)
	EF_FLAG(		"Don't Display Name",					pdwFlags,							UNIT_DATA_FLAG_HIDE_NAME						)
	EF_FLAG(		"inform quests on init",				pdwFlags,							UNIT_DATA_FLAG_INFORM_QUESTS_ON_INIT			)
	EF_FLAG(		"InformQuestsOfLootDrop",				pdwFlags,							UNIT_DATA_FLAG_INFORM_QUESTS_OF_LOOT_DROP		)
	EF_FLAG(		"inform quests on death",				pdwFlags,							UNIT_DATA_FLAG_INFORM_QUESTS_ON_DEATH			)
	EF_FLAG(		"item is targeted",						pdwFlags,							UNIT_DATA_FLAG_ITEM_IS_TARGETED					)
	EF_LINK_INT(	"QuestRequirement",						nQuestRequirement,					DATATABLE_QUEST									)
	EF_FLAG(		"no trade",								pdwFlags,							UNIT_DATA_FLAG_NO_TRADE							)	
	EF_FLAG(		"bind to level area",					pdwFlags,							UNIT_DATA_FLAG_BIND_TO_LEVELAREA				)	
	EF_FLOAT(		"No Spawn Radius",						fNoSpawnRadius																		)

	EF_LINK_INT(	"requires item of Unit Type 1",			nUnitTypeRequiredToOperate,			DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"spawn treasure class in level",		nUnitTypeSpawnTreasureOnLevel,		DATATABLE_TREASURE								)
	EF_FLAG(		"treasure class before room",			pdwFlags,							UNIT_DATA_FLAG_LEVELTREASURE_SPAWN_BEFORE_UNIT	)	

	EF_FLAG(		"Flag Room As No Spawn",				pdwFlags,							UNIT_DATA_FLAG_ROOM_NO_SPAWN					)	
	EF_FLAG(		"MonitorPlayerApproach",				pdwFlags,							UNIT_DATA_FLAG_MONITOR_PLAYER_APPROACH			)			
	EF_FLOAT(		"MonitorApproachRadius",				flMonitorApproachRadius																)				
	EF_FLAG(		"MonitorApproachClearLOS",				pdwFlags,							UNIT_DATA_FLAG_MONITOR_APPROACH_CLEAR_LOS		)	
	EF_LINK_INT(	"TasksGeneratedStat",					nTasksGeneratedStat,				DATATABLE_STATS									)	
	EF_FLAG(		"on die destroy",						pdwFlags,							UNIT_DATA_FLAG_ON_DIE_DESTROY					)
	EF_FLAG(		"on die end destroy",					pdwFlags,							UNIT_DATA_FLAG_ON_DIE_END_DESTROY				)
	EF_FLAG(		"fade on destroy",						pdwFlags,							UNIT_DATA_FLAG_FADE_ON_DESTROY					)
	EF_FLAG(		"on die hide model",					pdwFlags,							UNIT_DATA_FLAG_ON_DIE_HIDE_MODEL				)
	EF_FLAG(		"selectable dead or dying",				pdwFlags,							UNIT_DATA_FLAG_SELECTABLE_DEAD					)
	EF_FLOAT(		"extra dying time in seconds",			flExtraDyingTimeInSeconds															)	
	EF_LINK_INT(	"npc info (row in npc.xls)",			nNPC,								DATATABLE_NPC									)		
	EF_INT(			"balance test count",					nBalanceTestCount																	)		
	EF_INT(			"balance test group",					nBalanceTestGroup																	)
	EF_FLAG(		"interactive",							pdwFlags,							UNIT_DATA_FLAG_INTERACTIVE						)
	EF_FLAG(		"HideDialogHead",						pdwFlags,							UNIT_DATA_FLAG_HIDE_DIALOG_HEAD					)
	EF_FLAG(		"can fizzle",							pdwFlags,							UNIT_DATA_FLAG_CAN_FIZZLE						)
	EF_FLAG(		"inherits direction",					pdwFlags,							UNIT_DATA_FLAG_INHERITS_DIRECTION				)
	EF_FLOAT(		"Homing turn angle radians",			flHomingTurnAngleInRadians,			0.0f											)			
	EF_FLOAT(		"homing mod based dis",					flHomingTurnAngleInRadiansModByDis,	0.0f											)			
	EF_FLOAT(		"home after % * unit radius",			flHomingUnitScalePct,				1.0f											)			
	EF_FLOAT(		"collidable after x seconds",			flSecondsBeforeBecomingCollidable,	0.0f											)				
	EF_FLOAT(		"home after x seconds",					flHomingAfterXSeconds,				0.0f											)				
	EF_FLAG(		"Can Attack Friends",					pdwFlags,							UNIT_DATA_FLAG_CAN_ATTACK_FRIENDS				)
	EF_FLAG(		"collide good",							pdwFlags,							UNIT_DATA_FLAG_COLLIDE_GOOD						)
	EF_FLAG(		"dont path",							pdwFlags,							UNIT_DATA_FLAG_DONT_PATH						)
	EF_FLAG(		"snap to pathnode on create",			pdwFlags,							UNIT_DATA_FLAG_SNAP_TO_PATH_NODE_ON_CREATE		)		
	EF_FLAG(		"untargetable",							pdwFlags,							UNIT_DATA_FLAG_UNTARGETABLE						)	
	EF_FLAG(		"FaceDuringInteraction",				pdwFlags,							UNIT_DATA_FLAG_FACE_DURING_INTERACTION			)	
	EF_FLAG(		"no synch",								pdwFlags,							UNIT_DATA_FLAG_DONT_SYNCH						)
	EF_FLAG(		"modes ignore AI",						pdwFlags,							UNIT_DATA_FLAG_MODES_IGNORE_AI					)
	EF_FLAG(		"collide bad",							pdwFlags,							UNIT_DATA_FLAG_COLLIDE_BAD						)
	EF_LINK_INT(	"only collide with unittype",			nOnlyCollideWithUnitType,			DATATABLE_UNITTYPES								)
	EF_FLOAT(		"impact camera shake duration",			fImpactCameraShakeDuration,			0.0f											)
	EF_FLOAT(		"impact camera shake magnitude",		fImpactCameraShakeMagnitude,		0.0f											)
	EF_FLOAT(		"impact camera shake degrade",			fImpactCameraShakeDegrade,			0.0f											)
	EF_INT(			"maximum impact frequency",				nMaximumImpactFrequency,			0												)
	EF_LINK_INT(	"field missile",						nFieldMissile,						DATATABLE_MISSILES								)	
	EF_FLAG(		"bounce on unit hit",					dwBounceFlags,						BF_BOUNCE_ON_HIT_UNIT_BIT						)
	EF_FLAG(		"bounce on background hit",				dwBounceFlags,						BF_BOUNCE_ON_HIT_BACKGROUND_BIT					)
	EF_FLAG(		"New Direction on Bounce",				dwBounceFlags,						BF_BOUNCE_NEW_DIRECTION_BIT						)
	EF_FLAG(		"cannot ricochet",						dwBounceFlags,						BF_CANNOT_RICOCHET_BIT							)
	EF_FLAG(		"retarget on bounce",					dwBounceFlags,						BF_BOUNCE_RETARGET_BIT							)
	EF_FLAG(		"ignores tohit",						pdwFlags,							UNIT_DATA_FLAG_IGNORES_TO_HIT					)
	EF_FLAG(		"autopickup",							pdwFlags,							UNIT_DATA_FLAG_AUTO_PICKUP						)
	EF_FLOAT(		"auto pickup distance",					flAutoPickupDistance																)	
	EF_LINK_INT(	"pickup pull state",					nPickupPullState,					DATATABLE_STATES								)		
	EF_FLAG(		"start in town idle",					pdwFlags,							UNIT_DATA_FLAG_START_IN_TOWN_IDLE				)
	EF_FLAG(		"check radius when pathing",			pdwFlags,							UNIT_DATA_FLAG_CHECK_RADIUS_WHEN_PATHING		)
	EF_FLAG(		"check height when pathing",			pdwFlags,							UNIT_DATA_FLAG_CHECK_HEIGHT_WHEN_PATHING		)
	EF_INT(			"min monster experience level",			nMinMonsterExperienceLevel															)
	EF_INT(			"level",								nItemExperienceLevel																)
	EF_INT(			"baselevel",							nBaseLevel																			)
	EF_INT(			"caplevel",								nCapLevel,							-1												)	
	EF_INT(			"min merchant level",					nMinMerchantLevel,					-1												)
	EF_INT(			"max merchant level",					nMaxMerchantLevel,					-1												)
	EF_INT(			"min spawn level",						nMinSpawnLevel,						0												)
	EF_INT(			"max spawn level",						nMaxSpawnLevel,						100000											)
	EF_INT(			"maxlevel",								nMaxLevel,							0												)
	EF_INT(			"maxrank",								nMaxRank,							0												)
	EF_LINK_INT(	"object downgrades to",					nObjectDownGradeToObject,			DATATABLE_OBJECTS								)		
	EF_CODE(		"fixedlevel",							codeFixedLevel																		)
	EF_INT(			"hp_min",								nMinHitPointPct,					100												)
	EF_INT(			"hp_max",								nMaxHitPointPct,					100												)
	EF_INT(			"power_max",							nMaxPower																			)
	EF_INT(			"experience",							nExperiencePct																		)	
	EF_CODE(		"luck bonus",							codeLuckBonus																		)
	EF_INT(			"luck chance to spawn",					nLuckChanceBonus																	)
	EF_LINK_INT(	"treasure",								nTreasure,							DATATABLE_TREASURE								)
	EF_FLAG(		"can be champion",						pdwFlags,							UNIT_DATA_FLAG_CAN_BE_CHAMPION					)	
	EF_LINK_INT(	"champion treasure",					nTreasureChampion,					DATATABLE_TREASURE								)
	EF_LINK_INT(	"first-time treasure",					nTreasureFirstTime,					DATATABLE_TREASURE								)
	EF_LINK_INT(	"crafting treasure",					nTreasureCrafting,					DATATABLE_TREASURE								)
	EF_FLAG(		"nolevel",								pdwFlags,							UNIT_DATA_FLAG_NO_LEVEL							)	
	EF_INT(			"rarity",								nRarity																				)
	EF_INT(			"requires affix or suffix",				bRequireAffix																		)
	EF_INT(			"spawn chance",							nSpawnPercentChance,				100												)
	EF_LINK_INT(	"inventory wardrobe",					nInventoryWardrobeLayer,			DATATABLE_WARDROBE_LAYER						)
	EF_LINK_INT(	"character screen wardrobe",			nCharacterScreenWardrobeLayer,		DATATABLE_WARDROBE_LAYER						)
	EF_LINK_INT(	"character screen state",				nCharacterScreenState,				DATATABLE_STATES								)
	EF_LINK_INT(	"wardrobe body",						nWardrobeBody,						DATATABLE_WARDROBE_BODY							)
	EF_LINK_INT(	"wardrobe fallback",					nWardrobeFallback,					DATATABLE_MONSTERS								)
	EF_INT(			"",										nWardrobeFallbackId,				INVALID_ID										)
	EF_FLAG(		"always use fallback",					pdwFlags,							UNIT_DATA_FLAG_ALWAYS_USE_WARDROBE_FALLBACK		)
	EF_LINK_INT(	"wardrobe appearance group",			pnWardrobeAppearanceGroup[0],		DATATABLE_WARDROBE_APPEARANCE_GROUP				)
	EF_LINK_INT(	"wardrobe appearance group 1st",		pnWardrobeAppearanceGroup[1],		DATATABLE_WARDROBE_APPEARANCE_GROUP				)
	EF_FLAG(		"wardrobe per unit",					pdwFlags,							UNIT_DATA_FLAG_WARDROBE_PER_UNIT				)
	EF_FLAG(		"no weapon model",						pdwFlags,							UNIT_DATA_FLAG_NO_WEAPON_MODEL					)
	EF_FLAG(		"wardrobe shares model def",			pdwFlags,							UNIT_DATA_FLAG_WARDROBE_SHARES_MODEL_DEF		)
	EF_INT(			"wardrobe mip",							nWardrobeMipBonus																	)
	EF_FLOAT(		"server missile offset",				fServerMissileOffset																)
	EF_LINK_INT(	"starting stance",						nStartingStance,					DATATABLE_ANIMATION_STANCE						)
	EF_LINK_INT(	"animgroup",							nAnimGroup,							DATATABLE_ANIMATION_GROUP						)
	EF_LINK_INT(	"meleeweapon",							nMeleeWeapon,						DATATABLE_MELEEWEAPONS							)
	EF_LINK_FARR(	"qualities",							pdwQualities,						DATATABLE_ITEM_QUALITY							)
	EF_LINK_INT(	"required quality",						nRequiredQuality,					DATATABLE_ITEM_QUALITY							)		
	EF_DICT_INT(	"quality name",							eQualityName,						tDictQualityName								)	
	EF_FLAG(		"no quality downgrade",					pdwFlags,							UNIT_DATA_FLAG_NO_QUALITY_DOWNGRADE				)	
	EF_LINK_IARR(	"required affix groups",				nReqAffixGroup,						DATATABLE_AFFIX_GROUPS							)
	EF_INT(			"firing error increase",				nFiringErrorIncrease																)
	EF_INT(			"firing error decrease",				nFiringErrorDecrease																)
	EF_INT(			"firing error max",						nFiringErrorMax																		)
	EF_INT(			"accuracy base",						nAccuracyBase																		)
	EF_INT(			"cd_ticks",								nCooldown																			)	
	EF_INT(			"duration",								nDuration																			)
	EF_FLOAT(		"*approx dps",							fEstDPS																				)
	EF_LINK_INT(	"spawn monster class",					nSpawnMonsterClass,					DATATABLE_MONSTERS								)
	EF_LINK_INT(	"safe state",							nStateSafe,							DATATABLE_STATES								)	
	EF_LINK_INT(	"skill ghost",							nSkillGhost,						DATATABLE_SKILLS								)
	EF_DSTRING(		"tooltip damage icon",					szDamageIcon																		)
	EF_STR_TABLE(	"tooltip damage string",				nDamageDescripString,				APP_GAME_ALL									)
	EF_LINK_INT(	"dmgtype",								nDamageType,						DATATABLE_DAMAGETYPES							)
	EF_CODE(		"min base dmg",							codeMinBaseDmg																		)
	EF_CODE(		"max base dmg",							codeMaxBaseDmg																		)
	EF_INT(			"don't use weapon damage",				nDontUseWeaponDamage																)
	EF_FLAG(		"ignore item requirements",				pdwFlags,							UNIT_DATA_FLAG_IGNORES_ITEM_REQUIREMENTS				)	
	EF_INT(			"weapon damage scale",					nWeaponDamageScale																	)
	EF_INT(			"attack rating",						nAttackRatingPct																	)
	EF_INT(			"sfx attack % focus",					nSfxAttackPercentFocus																)
	EF_INT(			"sfx physical ability %",										tSpecialEffect[DAMAGE_TYPE_PHYSICAL].nAbilityPercent		)
	EF_INT(			"sfx physical defense %",										tSpecialEffect[DAMAGE_TYPE_PHYSICAL].nDefensePercent		)
	EF_INT(			"sfx physical knockback in cm",									tSpecialEffect[DAMAGE_TYPE_PHYSICAL].nStrengthPercent		)
	EF_FLOAT(		"sfx physical stun duration in seconds",						tSpecialEffect[DAMAGE_TYPE_PHYSICAL].flDurationInSeconds	)	
	EF_INT(			"sfx electric ability %",										tSpecialEffect[DAMAGE_TYPE_ELECTRICITY].nAbilityPercent		)
	EF_INT(			"sfx electric defense %",										tSpecialEffect[DAMAGE_TYPE_ELECTRICITY].nDefensePercent		)
	EF_INT(			"sfx electric damage % of initial per shock",					tSpecialEffect[DAMAGE_TYPE_ELECTRICITY].nStrengthPercent	)
	EF_FLOAT(		"sfx electric duration in seconds",								tSpecialEffect[DAMAGE_TYPE_ELECTRICITY].flDurationInSeconds	)	
	EF_INT(			"sfx fire ability %",											tSpecialEffect[DAMAGE_TYPE_FIRE].nAbilityPercent			)
	EF_INT(			"sfx fire defense %",											tSpecialEffect[DAMAGE_TYPE_FIRE].nDefensePercent			)
	EF_STAT(		"sfx_fire_damage_percent",				STATS_SFX_FIRE_DAMAGE_PERCENT														)
	EF_INT(			"sfx spectral ability %",										tSpecialEffect[DAMAGE_TYPE_SPECTRAL].nAbilityPercent		)
	EF_INT(			"sfx spectral defense %",										tSpecialEffect[DAMAGE_TYPE_SPECTRAL].nDefensePercent		)
	EF_INT(			"sfx spectral - (outgoing damage %) and + (incoming damage %)",	tSpecialEffect[DAMAGE_TYPE_SPECTRAL].nStrengthPercent		)
	EF_FLOAT(		"sfx spectral duration in seconds",								tSpecialEffect[DAMAGE_TYPE_SPECTRAL].flDurationInSeconds	)
	EF_INT(			"sfx toxic ability %",											tSpecialEffect[DAMAGE_TYPE_TOXIC].nAbilityPercent			)
	EF_INT(			"sfx toxic defense %",											tSpecialEffect[DAMAGE_TYPE_TOXIC].nDefensePercent			)
	EF_INT(			"sfx toxic damage as % of damage delivered from attack",		tSpecialEffect[DAMAGE_TYPE_TOXIC].nStrengthPercent			)
	EF_FLOAT(		"sfx toxic duration in seconds",								tSpecialEffect[DAMAGE_TYPE_TOXIC].flDurationInSeconds		)
	EF_INT(			"sfx poison ability %",											tSpecialEffect[DAMAGE_TYPE_POISON].nAbilityPercent			)
	EF_INT(			"sfx poison defense %",											tSpecialEffect[DAMAGE_TYPE_POISON].nDefensePercent			)
	EF_INT(			"sfx poison damage as % of damage delivered from attack",		tSpecialEffect[DAMAGE_TYPE_POISON].nStrengthPercent			)
	EF_FLOAT(		"sfx poison duration in seconds",								tSpecialEffect[DAMAGE_TYPE_POISON].flDurationInSeconds		)
	EF_INT(			"dmg increment",						nDmgIncrement																		)
	EF_INT(			"radial dmg increment",					nDmgIncrementRadial																	)
	EF_INT(			"field dmg increment",					nDmgIncrementField																	)
	EF_INT(			"DOT dmg increment",					nDmgIncrementDOT																	)
	EF_INT(			"ai changer dmg increment",				nDmgIncrementAIChanger																)
	EF_INT(			"critical %",							nCriticalChance																		)
	EF_INT(			"critical mult",						nCriticalMultiplier																	)
	EF_INT(			"stamina drain chance%",				nStaminaDrainChance																	)
	EF_INT(			"stamina drain",						nStaminaDrain																		)
	EF_FLAG(		"immune to critical",					pdwFlags,							UNIT_DATA_FLAG_IMMUNE_TO_CRITICAL				)
	EF_INT(			"interrupt attack pct",					nInterruptAttackPct																	)
	EF_INT(			"interrupt defense pct",				nInterruptDefensePct																)
	EF_INT(			"interrupt chance on any hit",			nInterruptChance																	)
	EF_INT(			"stealth attack pct",					nStealthAttackPct																	)
	EF_INT(			"stealth defense pct",					nStealthDefensePct																	)
	EF_STAT(		"stealth reveal duration",				STATS_STEALTH_REVEAL_DURATION														)
	EF_STAT(		"stealth reveal radius",				STATS_STEALTH_REVEAL_RADIUS															)
	EF_INT(			"ai change defense",					nAIChangeDefense																	)
	EF_INT(			"tohit",								nToHitBonus																			)
	EF_FLOAT(		"armor",							fArmor[DAMAGE_TYPE_ALL]																)
	EF_INT(			"max armor",							nArmorMax[DAMAGE_TYPE_ALL]															)
	EF_FLOAT(		"armor phys",						fArmor[DAMAGE_TYPE_PHYSICAL]														)
	EF_FLOAT(		"armor fire",						fArmor[DAMAGE_TYPE_FIRE]															)
	EF_FLOAT(		"armor elec",						fArmor[DAMAGE_TYPE_ELECTRICITY]														)
	EF_FLOAT(		"armor spec",						fArmor[DAMAGE_TYPE_SPECTRAL]														)
	EF_FLOAT(		"armor toxic",						fArmor[DAMAGE_TYPE_TOXIC]															)
	EF_FLOAT(		"armor poison",						fArmor[DAMAGE_TYPE_POISON]															)
	EF_FLOAT(		"shields",							fShields[DAMAGE_TYPE_ALL]															)
	EF_FLOAT(		"shield phys",						fShields[DAMAGE_TYPE_PHYSICAL]														)
	EF_FLOAT(		"shield fire",						fShields[DAMAGE_TYPE_FIRE]															)
	EF_FLOAT(		"shield elec",						fShields[DAMAGE_TYPE_ELECTRICITY]													)
	EF_FLOAT(		"shield spec",						fShields[DAMAGE_TYPE_SPECTRAL]														)
	EF_FLOAT(		"shield toxic",						fShields[DAMAGE_TYPE_TOXIC]															)
	EF_FLOAT(		"shield poison",					fShields[DAMAGE_TYPE_POISON]														)
	EF_INT(			"strength percent",						nStrengthPercent																	)
	EF_INT(			"dexterity percent",					nDexterityPercent																	)
	EF_INT(			"starting accuracy",					nStartingAccuracy																	)
	EF_INT(			"starting strength",					nStartingStrength																	)
	EF_INT(			"starting stamina",						nStartingStamina																	)
	EF_INT(			"starting willpower",					nStartingWillpower																	)
	EF_CODE(		"recipe props",							codeRecipeProperty																	)
	EF_CODE(		"skillprops1",							codeSkillExecute[0]																	)
	EF_CODE(		"skillprops2",							codeSkillExecute[1]																	)
	EF_CODE(		"skillprops3",							codeSkillExecute[2]																	)
	EF_CODE(		"props1",								codeProperties[0]																	)
	EF_CODE(		"props2",								codeProperties[1]																	)
	EF_CODE(		"props3",								codeProperties[2]																	)
	EF_CODE(		"props4",								codeProperties[3]																	)
	EF_CODE(		"props5",								codeProperties[4]																	)
	EF_LINK_INT(	"props1 applies to unitype",			nCodePropertiesApplyToUnitType[0],	DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"props2 applies to unitype",			nCodePropertiesApplyToUnitType[1],	DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"props3 applies to unitype",			nCodePropertiesApplyToUnitType[2],	DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"props4 applies to unitype",			nCodePropertiesApplyToUnitType[3],	DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"props5 applies to unitype",			nCodePropertiesApplyToUnitType[4],	DATATABLE_UNITTYPES								)
	EF_CODE(		"props elite",							codeEliteProperties																	)
	EF_CODE(		"per level props1",						codePerLevelProperties[0]															)	
	EF_CODE(		"per level props2",						codePerLevelProperties[1]															)	
	EF_LINK_IARR(	"affixes",								nAffixes,							DATATABLE_AFFIXES								)	
	EF_FLAG(		"no random affixes",					pdwFlags,							UNIT_DATA_FLAG_NO_RANDOM_AFFIXES				)		
	EF_DEFCODE(		"max slots"																													)
	EF_INT(			"base cost",							nBaseCost,							10												)
	EF_INT(			"PVP Point Cost",						nPVPPointCost,						0												)
	EF_FLAG(		"ignore sell with inventory confirm",	pdwFlags,							UNIT_DATA_FLAG_IGNORE_SELL_WITH_INVENTORY_CONFIRM)			
	EF_CODE(		"buy price mult",						codeBuyPriceMult																	)
	EF_CODE(		"buy price add",						codeBuyPriceAdd																		)
	EF_CODE(		"sell price mult",						codeSellPriceMult																	)
	EF_CODE(		"sell price add",						codeSellPriceAdd																	)
	EF_FLAG(		"cannot be dismantled",					pdwFlags,							UNIT_DATA_FLAG_CANNOT_BE_DISMANTLED				)		
	EF_FLAG(		"cannot be upgraded",					pdwFlags,							UNIT_DATA_FLAG_CANNOT_BE_UPGRADED				)		
	EF_FLAG(		"cannot be augmented",					pdwFlags,							UNIT_DATA_FLAG_CANNOT_BE_AUGMENTED				)		
	EF_FLAG(		"cannot be de-modded",					pdwFlags,							UNIT_DATA_FLAG_CANNOT_BE_DEMODDED				)			
	EF_CODE(		"stacksize",							codeStackSize																		)			
	EF_INT(			"max pickup",							nMaxPickup																			)	
	EF_LINK_INT(	"inventory",							nInventory,							DATATABLE_INVENTORY_TYPES						)
	EF_LINK_INT(	"crafting inventory",					nCraftingInventory,					DATATABLE_INVENTORY_TYPES						)
	EF_LINK_INT(	"recipe ingredient inv loc",			nInvLocRecipeIngredients,			DATATABLE_INVLOCIDX								)	
	EF_LINK_INT(	"recipe result inv loc",				nInvLocRecipeResults,				DATATABLE_INVLOCIDX								)	
	EF_LINK_INT(	"starting treasure",					nStartingTreasure,					DATATABLE_TREASURE								)
	EF_LINK_INT(	"skill ref",							nSkillRef,							DATATABLE_SKILLS								)	
	EF_LINK_IARR(	"startingskills",						nStartingSkills,					DATATABLE_SKILLS,								)
	EF_LINK_INT(	"unitdie begin skill",					nUnitDieBeginSkill,					DATATABLE_SKILLS								)
	EF_LINK_INT(	"unitdie begin skill client",			nUnitDieBeginSkillClient,			DATATABLE_SKILLS								)
	EF_CODE(		"scriptOnUnitDieEnd",					codeScriptUnitDieBegin																)
	EF_LINK_INT(	"unitdie end skill",					nUnitDieEndSkill,					DATATABLE_SKILLS								)
	EF_LINK_INT(	"dead skill",							nDeadSkill,							DATATABLE_SKILLS								)
	EF_LINK_INT(	"kick skill",							nKickSkill,							DATATABLE_SKILLS								)
	EF_LINK_INT(	"right skill",							nRightSkill,						DATATABLE_SKILLS								)
	EF_LINK_IARR(	"init skill",							nInitSkill,							DATATABLE_SKILLS								)
	EF_LINK_INT(	"class skill",							nClassSkill,						DATATABLE_SKILLS								)
	EF_LINK_INT(	"race skill",							nRaceSkill,							DATATABLE_SKILLS								)
	EF_LINK_INT(	"skill level active",					nSkillLevelActive,					DATATABLE_SKILLS								)	
	EF_LINK_IARR(	"init state",							pnInitStates,						DATATABLE_STATES								)
	EF_INT(			"init state ticks",						nInitStateTicks																		)
	EF_LINK_INT(	"dying state",							nDyingState,						DATATABLE_STATES								)
	EF_LINK_INT(	"skill when used",						nSkillWhenUsed,						DATATABLE_SKILLS								)
	EF_CODE(		"script on use",						codeUseScript																		)			
	EF_LINK_INT(	"SkillTab",								nSkillTab[0],						DATATABLE_SKILLTABS								)
	EF_LINK_INT(	"SkillTab2",							nSkillTab[1],						DATATABLE_SKILLTABS								)
	EF_LINK_INT(	"SkillTab3",							nSkillTab[2],						DATATABLE_SKILLTABS								)
	EF_LINK_INT(	"SkillTab4",							nSkillTab[3],						DATATABLE_SKILLTABS								)
	EF_LINK_INT(	"SkillTab5",							nSkillTab[4],						DATATABLE_SKILLTABS								)
	EF_LINK_INT(	"SkillTab6",							nSkillTab[5],						DATATABLE_SKILLTABS								)
	EF_LINK_INT(	"SkillTab7",							nSkillTab[6],						DATATABLE_SKILLTABS								)
	EF_FLAG(		"no draw on init",						pdwFlags,							UNIT_DATA_FLAG_NO_DRAW_ON_INIT					)	
	EF_INT(			"invwidth",								nInvWidth																			)
	EF_INT(			"invheight",							nInvHeight																			)
	EF_CODE(		"pickupcondition",						codePickupCondition																	)
	EF_DSTRING(		"PickupFunction",						szPickupFunction																	)
	EF_CODE(		"usecondition",							codeUseCondition																	)
	EF_FLAG(		"NoDrop",								pdwFlags,							UNIT_DATA_FLAG_NO_DROP							)
	EF_FLAG(		"NoDropExceptForDuplicates",			pdwFlags,							UNIT_DATA_FLAG_NO_DROP_EXCEPT_FOR_DUPLICATES	)
	EF_FLAG(		"AskQuestsForPickup",					pdwFlags,							UNIT_DATA_FLAG_ASK_QUESTS_FOR_PICKUP			)
	EF_FLAG(		"InformQuestsOnPickup",					pdwFlags,							UNIT_DATA_FLAG_INFORM_QUESTS_ON_PICKUP			)	
	EF_LINK_INT(	"QuestDescription",						nQuestItemDescriptionID,			DATATABLE_DIALOG								)
	EF_FLAG(		"Examinable",							pdwFlags,							UNIT_DATA_FLAG_EXAMINABLE,	1						)
	EF_FLAG(		"No Spin",								pdwFlags,							UNIT_DATA_FLAG_NOSPIN							)
	EF_FLAG(		"InformQuestsToUse",					pdwFlags,							UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE				)
	EF_FLAG(		"Consume When Used",					pdwFlags,							UNIT_DATA_FLAG_CONSUMED_WHEN_USED				)			
	EF_FLAG(		"always do transaction logging",		pdwFlags,							UNIT_DATA_FLAG_ALWAYS_DO_TRANSACTION_LOGGING	)
	EF_LINK_UTYPE(	"container unittype",					nContainerUnitTypes,				"any"											)
	EF_LINK_INT(	"refillhotkey",							m_nRefillHotkeyInvLoc,				DATATABLE_INVLOCIDX								)
	EF_LINK_INT(	"levelup state",						m_nLevelUpState,					DATATABLE_STATES								)	
	EF_LINK_INT(	"rankup state",							m_nRankUpState,						DATATABLE_STATES								)	
	EF_LINK_INT(	"sounds.xls",							m_nSound,							DATATABLE_SOUNDS								)
	EF_LINK_INT(	"flippysound",							m_nFlippySound,						DATATABLE_SOUNDS								)
	EF_LINK_INT(	"invpickupsound",						m_nInvPickupSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"invputdownsound",						m_nInvPutdownSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"invequipsound",						m_nInvEquipSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"pickupsound",							m_nPickupSound,						DATATABLE_SOUNDS								)
	EF_LINK_INT(	"usesound",								m_nInvUseSound,						DATATABLE_SOUNDS								)
	EF_LINK_INT(	"cantusesound",							m_nInvCantUseSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"cantfiresound",						m_nCantFireSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"blocksound",							m_nBlockSound,						DATATABLE_SOUNDS								)
	EF_LINK_INT(	"get hit 0",							pnGetHitSounds[0],					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"get hit 1",							pnGetHitSounds[1],					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"get hit 2",							pnGetHitSounds[2],					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"get hit 3",							pnGetHitSounds[3],					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"get hit 4",							pnGetHitSounds[4],					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"get hit 5",							pnGetHitSounds[5],					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"interact sound",						nInteractSound,						DATATABLE_SOUNDS								)
	EF_LINK_INT(	"damaging sound",						nDamagingSound,						DATATABLE_SOUNDS								)
	EF_LINK_INT(	"forward footstep left",				nFootstepForwardLeft,				DATATABLE_FOOTSTEPS								)
	EF_LINK_INT(	"forward footstep right",				nFootstepForwardRight,				DATATABLE_FOOTSTEPS								)
	EF_LINK_INT(	"backward footstep left",				nFootstepBackwardLeft,				DATATABLE_FOOTSTEPS								)
	EF_LINK_INT(	"backward footstep right",				nFootstepBackwardRight,				DATATABLE_FOOTSTEPS								)
	EF_LINK_INT(	"first-person jump footstep",			nFootstepJump1stPerson,				DATATABLE_FOOTSTEPS								)
	EF_LINK_INT(	"first-person land footstep",			nFootstepLand1stPerson,				DATATABLE_FOOTSTEPS								)
	EF_LINK_INT(	"out of mana sound",					m_nOutOfManaSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"inventory full sound",					m_nInventoryFullSound,				DATATABLE_SOUNDS								)
	EF_LINK_INT(	"can't use sound",						m_nCantUseSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"can't use yet sound",					m_nCantUseYetSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"can't sound",							m_nCantSound,						DATATABLE_SOUNDS								)
	EF_LINK_INT(	"can't cast sound",						m_nCantCastSound,					DATATABLE_SOUNDS								)
	EF_LINK_INT(	"can't cast yet sound",					m_nCantCastYetSound,				DATATABLE_SOUNDS								)
	EF_LINK_INT(	"can't cast here sound",				m_nCantCastHereSound,				DATATABLE_SOUNDS								)
	EF_LINK_INT(	"locked sound",							m_nLockedSound,						DATATABLE_SOUNDS								)
	EF_FLOAT(		"collisionradius",						fCollisionRadius,					1.0f											)
	EF_FLOAT(		"collisionradiushorizontal",			fCollisionRadiusHorizontal,			-1.0f											)
	EF_FLOAT(		"pathingcollisionradius",				fPathingCollisionRadius,			0.5f											)
	EF_FLOAT(		"collisionheight",						fCollisionHeight,					1.0f											)
	EF_FLOAT(		"blocking radius override",				fBlockingCollisionRadiusOverride,	-1.0f											)
	EF_FLOAT(		"warp plane forward multiplier",		fWarpPlaneForwardMultiplier,		0.0f											)
	EF_FLOAT(		"spin speed",							fSpinSpeed,							.45f											)
	EF_FLOAT(		"max turn rate",						fMaxTurnRate,						512.0f											)
	EF_FLOAT(		"warp out distance",					flWarpOutDistance,					-1.0f											)	
	EF_INT(			"snap to angle in degrees",				nSnapAngleDegrees,					-1												)	
	EF_FLOAT(		"offset up",							fOffsetUp,							0.0f											)
	EF_FLOAT(		"vanity height",						fVanityHeight,						0.0f											)
	EF_FLOAT(		"weapon scale",							fWeaponScale,						1.0f											)
	EF_FLOAT(		"scale",								fScale,								1.0f											)
	EF_FLOAT(		"scale delta",							fScaleDelta,						0.0f											)
	EF_FLOAT(		"scale multiplier",						fScaleMultiplier,					1.0f											)
	EF_FLOAT(		"champion scale",						fChampionScale,						0.0f											)
	EF_FLOAT(		"champion scale delta",					fChampionScaleDelta,				0.0f											)
	EF_FLOAT(		"melee impact offset",					fMeleeImpactOffset,					0.0f											)
	EF_FLOAT(		"ragdoll force multiplier",				fRagdollForceMultiplier,			1.0f											)
	EF_FLOAT(		"melee range max",						fMeleeRangeMax,						0.0f											)
	EF_FLOAT(		"melee range desired",					fMeleeRangeDesired,					0.0f											)
	EF_FLAG(		"must face melee target",				pdwFlags,							UNIT_DATA_FLAG_MUST_FACE_MELEE_TARGET,	FALSE	)
	EF_FLOAT(		"max automap radius",					flMaxAutomapRadius,					0.0f											)	
	EF_FLOAT(		"fuse",									fFuse,								0.0f											)		
	EF_INT(			"client minimum lifetime",				nClientMinimumLifetime																)
	EF_FLOAT(		"range base",							fRangeBase,							0.0f											)
	EF_FLOAT(		"range desired mult",					fDesiredRangeMultiplier,			0.8f											)
	EF_INT(			"range min",							nRangeMinDeltaPercent,				0												)
	EF_INT(			"range max",							nRangeMaxDeltaPercent,				0												)
	EF_FLOAT(		"verticle accuracy",					fVerticleAccuracy,					0.0f											)
	EF_FLOAT(		"horizontal accuracy",					fHorizontalAccuracy,				0.0f											)
	EF_FLOAT(		"hit backup",							fHitBackup,							0.0f											)
	EF_FLOAT(		"force",								fForce,								0.0f											)
	EF_FLOAT(		"jump velocity",						fJumpVelocity,						0.0f											)
	EF_FLOAT(		"velocity for impact",					fVelocityForImpact,					0.0f											)
	EF_FLOAT(		"acceleration",							fAcceleration,						0.0f											)
	EF_FLOAT(		"bounce",								fBounce,							0.0f											)
	EF_FLOAT(		"dampening",							fDampening,							0.0f											)
	EF_FLOAT(		"friction",								fFriction,							0.0f											)
	EF_FLOAT(		"spawn randomize length percent",		fSpawnRandomizePercent,				0.0f											)
	EF_FLOAT(		"missile arc height",					fMissileArcHeight,					2.0f											)
	EF_FLOAT(		"missile arc delta",					fMissileArcDelta,					0.5f											)
	EF_INT(			"stop after X num of ticks",			nTicksToDestoryAfter,				0												)	
	EF_FLAG(		"don't destroy if velocity is zero",	pdwFlags,							UNIT_DATA_FLAG_DO_NOT_DESTROY_IF_VELOCITY_ZERO,	FALSE)
	EF_FLOAT(		"walk and run delta",					fDeltaVelocityModify																)
	EF_FLOAT(		"walk speed",							tVelocities[MODE_VELOCITY_WALK].fVelocityBase										)
	EF_FLOAT(		"walk min",								tVelocities[MODE_VELOCITY_WALK].fVelocityMin										)
	EF_FLOAT(		"walk max",								tVelocities[MODE_VELOCITY_WALK].fVelocityMax										)
	EF_FLOAT(		"strafe speed",							tVelocities[MODE_VELOCITY_STRAFE].fVelocityBase										)
	EF_FLOAT(		"strafe min",							tVelocities[MODE_VELOCITY_STRAFE].fVelocityMin										)
	EF_FLOAT(		"strafe max",							tVelocities[MODE_VELOCITY_STRAFE].fVelocityMax										)
	EF_FLOAT(		"jump speed",							tVelocities[MODE_VELOCITY_JUMP].fVelocityBase										)
	EF_FLOAT(		"jump min",								tVelocities[MODE_VELOCITY_JUMP].fVelocityMin										)
	EF_FLOAT(		"jump max",								tVelocities[MODE_VELOCITY_JUMP].fVelocityMax										)
	EF_FLOAT(		"run speed",							tVelocities[MODE_VELOCITY_RUN].fVelocityBase										)
	EF_FLOAT(		"run min",								tVelocities[MODE_VELOCITY_RUN].fVelocityMin											)
	EF_FLOAT(		"run max",								tVelocities[MODE_VELOCITY_RUN].fVelocityMax											)
	EF_FLOAT(		"backup speed",							tVelocities[MODE_VELOCITY_BACKUP].fVelocityBase										)
	EF_FLOAT(		"backup min",							tVelocities[MODE_VELOCITY_BACKUP].fVelocityMin										)
	EF_FLOAT(		"backup max",							tVelocities[MODE_VELOCITY_BACKUP].fVelocityMax										)
	EF_FLOAT(		"knockback speed",						tVelocities[MODE_VELOCITY_KNOCKBACK].fVelocityBase									)
	EF_FLOAT(		"knockback min",						tVelocities[MODE_VELOCITY_KNOCKBACK].fVelocityMin									)
	EF_FLOAT(		"knockback max",						tVelocities[MODE_VELOCITY_KNOCKBACK].fVelocityMax									)
	EF_FLOAT(		"melee speed",							tVelocities[MODE_VELOCITY_MELEE].fVelocityBase										)
	EF_FLOAT(		"melee min",							tVelocities[MODE_VELOCITY_MELEE].fVelocityMin										)
	EF_FLOAT(		"melee max",							tVelocities[MODE_VELOCITY_MELEE].fVelocityMax										)
#ifdef HAVOK_ENABLED
	EF_INT(			"HavokShape",							nHavokShape,    					HAVOK_SHAPE_NONE								)
#else
	EF_INT(			"HavokShape",							nHavokShape,    					0												)
#endif
	EF_STR_TABLE(	"string",								nString,							APP_GAME_ALL									)
	EF_STR_TABLE(	"typedescription",						nTypeDescrip,						APP_GAME_ALL									)
	EF_STR_TABLE(	"flavortext",							nFlavorText,						APP_GAME_ALL									)
	EF_STR_TABLE(	"additional description",				nAddtlDescripString,				APP_GAME_ALL									)
	EF_STR_TABLE(	"additional race description",			nAddtlRaceDescripString,			APP_GAME_ALL									)
	EF_STR_TABLE(	"additional race bonus description",	nAddtlRaceBonusDescripString,		APP_GAME_ALL									)
	EF_FLAG(		"get flavortext from Quest",			pdwFlags,							UNIT_DATA_FLAG_QUEST_FLAVOR_TEXT						)
	EF_STR_TABLE(	"analyze",								nAnalyze,							APP_GAME_ALL									)
	EF_LINK_INT(	"missile hit unit",						nMissileHitUnit,					DATATABLE_MISSILES								)
	EF_LINK_INT(	"missile hit background",				nMissileHitBackground,				DATATABLE_MISSILES								)	
	EF_LINK_INT(	"missile on free or fuse",				nMissileOnFreeOrFuse,				DATATABLE_MISSILES								)
	EF_LINK_INT(	"missile on missed",					nMissileOnMissed,					DATATABLE_MISSILES								)
	EF_LINK_INT(	"skill hit unit",						nSkillIdHitUnit,					DATATABLE_SKILLS								)
	EF_LINK_INT(	"skill hit background",					nSkillIdHitBackground,				DATATABLE_SKILLS								)	
	EF_LINK_IARR(	"skill missed",							nSkillIdMissedArray,				DATATABLE_SKILLS								)
	EF_LINK_INT(	"skill on fuse",						nSkillIdOnFuse,						DATATABLE_SKILLS								)
	EF_LINK_INT(	"skill on damage repeat",				nSkillIdOnDamageRepeat,				DATATABLE_SKILLS								)
	EF_INT(			"damage repeat rate",					nDamageRepeatRate																	)
	EF_INT(			"damage repeat chance",					nDamageRepeatChance																	)
	EF_BOOL(		"repeat damage immediately",			bRepeatDamageImmediately,			FALSE											)	
	EF_INT(			"server src dmg",						nServerSrcDamage																	)
	EF_INT(			"missile tag",							nMissileTag																			)
	EF_BOOL(		"block ghosts",							bBlockGhosts,						FALSE											)	
	EF_LINK_INT(	"monster",								nMonsterToSpawn,					DATATABLE_MONSTERS								)
	EF_LINK_INT(	"triggertype",							nTriggerType,						DATATABLE_OBJECTTRIGGERS						)
	EF_LINK_INT(	"triggersound",							m_nTriggerSound,					DATATABLE_SOUNDS								)
	EF_LINK_IARR(	"operator states trigger prohibited",	nOperatorStatesTriggerProhibited,	DATATABLE_STATES								)	
	EF_LINK_INT(	"sublevel dest",						nSubLevelDest,						DATATABLE_SUBLEVEL								)	
	EF_LINK_INT(	"operate required quest",				nQuestOperateRequired,				DATATABLE_QUEST									)	
	EF_FLAG(		"operate requires good quest status",	pdwFlags,							UNIT_DATA_FLAG_OPERATE_REQUIRES_GOOD_QUEST_STATUS)
	EF_LINK_INT(	"operate required quest state",			nQuestStateOperateRequired,			DATATABLE_QUEST_STATE							)	
	EF_LINK_INT(	"operate required quest state value",	nQuestStateValueOperateRequired,	DATATABLE_QUEST_STATE_VALUE						)		
	EF_LINK_INT(	"operate prohibited quest state value",	nQuestStateValueOperateProhibited,	DATATABLE_QUEST_STATE_VALUE						)			
	EF_DICT_INT(	"one way visual portal dir",			eOneWayVisualPortalDirection,		tDictVisualPortalDirection						)
	EF_FLAG(		"reverse arrive direction",				pdwFlags,							UNIT_DATA_FLAG_REVERSE_ARRIVE_DIRECTION			)					
	EF_FLAG(		"face after warp",						pdwFlags,							UNIT_DATA_FLAG_FACE_AFTER_WARP					)						
	EF_FLAG(		"never a start location",				pdwFlags,							UNIT_DATA_FLAG_NEVER_A_START_LOCATION			)
	EF_DSTRING(		"triggerstring1",						szTriggerString1																	)
	EF_DSTRING(		"triggerstring2",						szTriggerString2																	)
	EF_FLAG(		"always show label",					pdwFlags,							UNIT_DATA_FLAG_ALWAYS_SHOW_LABEL				)		
	EF_FLOAT(		"label scale",							flLabelScale,						1.0f											)
	EF_FLOAT(		"label forward offset",					flLabelForwardOffset,				0.0f											)	
	EF_LINK_INT(	"decoy_monster",						nMonsterDecoy,						DATATABLE_MONSTERS								)	
	EF_INT(			"respawn period",						nRespawnPeriod,						1200											)
	EF_LINK_STAT(	"ai",									STATS_AI,							DATATABLE_AI_INIT								)
	EF_LINK_STAT(	"ai unarmed",							STATS_AI_UNARMED,					DATATABLE_AI_INIT								)
	EF_STAT(		"ai period",							STATS_AI_PERIOD,					0												)
	EF_STAT_FLOAT(	"aim_height",							STATS_AIM_HEIGHT																	)
	EF_STAT(		"ai_awake_range",						STATS_AI_AWAKE_RANGE,				18												)
	EF_STAT(		"ai_follow_range",						STATS_AI_FOLLOW_RANGE,				10												)
	EF_STAT(		"ai_sight_range",						STATS_AI_SIGHT_RANGE,				8												)
	EF_INT(			"anger range",							nAngerRange,						0												)
	EF_DSTRING(		"Muzzle Flash",							MissileEffects[MISSILE_EFFECT_MUZZLE].szDefault													)
	EF_DSTRING(		"MF Default",							MissileEffects[MISSILE_EFFECT_MUZZLE].tElementEffect[DAMAGE_TYPE_ALL].szPerElement				)
	EF_DSTRING(		"MF Toxic",								MissileEffects[MISSILE_EFFECT_MUZZLE].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement			)
	EF_DSTRING(		"MF Poison",							MissileEffects[MISSILE_EFFECT_MUZZLE].tElementEffect[DAMAGE_TYPE_POISON].szPerElement			)
	EF_DSTRING(		"MF Spectral",							MissileEffects[MISSILE_EFFECT_MUZZLE].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement			)
	EF_DSTRING(		"MF Fire",								MissileEffects[MISSILE_EFFECT_MUZZLE].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement				)
	EF_DSTRING(		"MF Electric",							MissileEffects[MISSILE_EFFECT_MUZZLE].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement		)
	EF_DSTRING(		"MF Physical",							MissileEffects[MISSILE_EFFECT_MUZZLE].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement			)
	EF_DSTRING(		"Projectile / Rope with Target",		MissileEffects[MISSILE_EFFECT_PROJECTILE].szDefault												)
	EF_DSTRING(		"Proj Default",							MissileEffects[MISSILE_EFFECT_PROJECTILE].tElementEffect[DAMAGE_TYPE_ALL].szPerElement			)
	EF_DSTRING(		"Proj Toxic",							MissileEffects[MISSILE_EFFECT_PROJECTILE].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement		)
	EF_DSTRING(		"Proj Poison",							MissileEffects[MISSILE_EFFECT_PROJECTILE].tElementEffect[DAMAGE_TYPE_POISON].szPerElement		)
	EF_DSTRING(		"Proj Spectral",						MissileEffects[MISSILE_EFFECT_PROJECTILE].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement		)
	EF_DSTRING(		"Proj Fire",							MissileEffects[MISSILE_EFFECT_PROJECTILE].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement			)
	EF_DSTRING(		"Proj Electric",						MissileEffects[MISSILE_EFFECT_PROJECTILE].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement	)
	EF_DSTRING(		"Proj Physical",						MissileEffects[MISSILE_EFFECT_PROJECTILE].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement		)
	EF_DSTRING(		"Trail / Rope no Target",				MissileEffects[MISSILE_EFFECT_TRAIL].szDefault													)
	EF_DSTRING(		"Trail Default",						MissileEffects[MISSILE_EFFECT_TRAIL].tElementEffect[DAMAGE_TYPE_ALL].szPerElement				)
	EF_DSTRING(		"Trail Toxic",							MissileEffects[MISSILE_EFFECT_TRAIL].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement				)
	EF_DSTRING(		"Trail Poison",							MissileEffects[MISSILE_EFFECT_TRAIL].tElementEffect[DAMAGE_TYPE_POISON].szPerElement			)
	EF_DSTRING(		"Trail Spectral",						MissileEffects[MISSILE_EFFECT_TRAIL].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement			)
	EF_DSTRING(		"Trail Fire",							MissileEffects[MISSILE_EFFECT_TRAIL].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement				)
	EF_DSTRING(		"Trail Electric",						MissileEffects[MISSILE_EFFECT_TRAIL].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement		)
	EF_DSTRING(		"Trail Physical",						MissileEffects[MISSILE_EFFECT_TRAIL].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement			)
	EF_DSTRING(		"Impact Wall",							MissileEffects[MISSILE_EFFECT_IMPACT_WALL].szDefault											)
	EF_DSTRING(		"Imp Wall Default",						MissileEffects[MISSILE_EFFECT_IMPACT_WALL].tElementEffect[DAMAGE_TYPE_ALL].szPerElement			)
	EF_DSTRING(		"Imp Wall Toxic",						MissileEffects[MISSILE_EFFECT_IMPACT_WALL].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement		)
	EF_DSTRING(		"Imp Wall Spectral",					MissileEffects[MISSILE_EFFECT_IMPACT_WALL].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement	)
	EF_DSTRING(		"Imp Wall Fire",						MissileEffects[MISSILE_EFFECT_IMPACT_WALL].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement		)
	EF_DSTRING(		"Imp Wall Electric",					MissileEffects[MISSILE_EFFECT_IMPACT_WALL].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement	)
	EF_DSTRING(		"Imp Wall Physical",					MissileEffects[MISSILE_EFFECT_IMPACT_WALL].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement	)
	EF_DSTRING(		"Impact Unit",							MissileEffects[MISSILE_EFFECT_IMPACT_UNIT].szDefault											)
	EF_DSTRING(		"Imp Unit Default",						MissileEffects[MISSILE_EFFECT_IMPACT_UNIT].tElementEffect[DAMAGE_TYPE_ALL].szPerElement			)
	EF_DSTRING(		"Imp Unit Toxic",						MissileEffects[MISSILE_EFFECT_IMPACT_UNIT].tElementEffect[DAMAGE_TYPE_TOXIC].szPerElement		)
	EF_DSTRING(		"Imp Unit Spectral",					MissileEffects[MISSILE_EFFECT_IMPACT_UNIT].tElementEffect[DAMAGE_TYPE_SPECTRAL].szPerElement	)
	EF_DSTRING(		"Imp Unit Fire",						MissileEffects[MISSILE_EFFECT_IMPACT_UNIT].tElementEffect[DAMAGE_TYPE_FIRE].szPerElement		)
	EF_DSTRING(		"Imp Unit Electric",					MissileEffects[MISSILE_EFFECT_IMPACT_UNIT].tElementEffect[DAMAGE_TYPE_ELECTRICITY].szPerElement	)
	EF_DSTRING(		"Imp Unit Physical",					MissileEffects[MISSILE_EFFECT_IMPACT_UNIT].tElementEffect[DAMAGE_TYPE_PHYSICAL].szPerElement	)
	EF_FLAG(		"",										pdwFlags,							UNIT_DATA_FLAG_LOADED													)
	EF_INT(			"",										nAppearanceDefId,					INVALID_ID										)
	EF_INT(			"",										nAppearanceDefId_FirstPerson,		INVALID_ID										)
	EF_INT(			"",										nIconTextureId,						INVALID_ID										)
	EF_INT(			"",										nHolyRadiusId,						INVALID_ID										)
	EF_INT(			"",										nDamagingMeleeParticleId,					INVALID_ID										)
	EF_BOOL(		"Has Appearance Shape",					tAppearanceShapeCreate.bHasShape													)
	EF_BYTE_PCT(	"Appearance Height Min",				tAppearanceShapeCreate.bHeightMin,			50										)
	EF_BYTE_PCT(	"Appearance Height Max",				tAppearanceShapeCreate.bHeightMax,			50										)
	EF_BYTE_PCT(	"Appearance Weight Min",				tAppearanceShapeCreate.bWeightMin,			50										)
	EF_BYTE_PCT(	"Appearance Weight Max",				tAppearanceShapeCreate.bWeightMax,			50										)
	EF_BOOL(		"Appearance Use Line Bounds",			tAppearanceShapeCreate.bUseLineBounds,		FALSE									)
	EF_BYTE_PCT(	"Appearance Short Skinny",				tAppearanceShapeCreate.bWeightLineMin0,		50										)
	EF_BYTE_PCT(	"Appearance Tall Skinny",				tAppearanceShapeCreate.bWeightLineMax0,		50										)
	EF_BYTE_PCT(	"Appearance Short Fat",					tAppearanceShapeCreate.bWeightLineMin1,		50										)
	EF_BYTE_PCT(	"Appearance Tall Fat",					tAppearanceShapeCreate.bWeightLineMax1,		50										)
	EF_FLOAT_PCT(	"Height Percent",						fHeightPercent,								0.0f									)
	EF_FLOAT_PCT(	"Weight Percent",						fWeightPercent,								0.0f									)
	EF_FLAG(		"ShowsPortrait",						pdwFlags,							UNIT_DATA_FLAG_SHOWS_PORTRAIT, TRUE				)
	EF_FLAG(		"Pet Gets Stat Points per Level",		pdwFlags,							UNIT_DATA_FLAG_PET_GETS_STAT_POINTS_PER_LEVEL	)
	EF_FLAG(		"Pet Dies On Warp",						pdwFlags,							UNIT_DATA_FLAG_PET_ON_WARP_DESTROY				)
	EF_INT(			"corpse explode points",				nCorpseExplodePoints																)
	EF_FLAG(		"Assign GUID",							pdwFlags,							UNIT_DATA_FLAG_ASSIGN_GUID						)	
	EF_LINK_INT(	"summoned invloc",						nSummonedLocation,					DATATABLE_INVLOCIDX						)
	EF_LINK_INT(	"Required Attacker Unittype",			nRequiredAttackerUnitType,			DATATABLE_UNITTYPES								)
	EF_FLAG(		"spawn",								pdwFlags,							UNIT_DATA_FLAG_SPAWN							)
	EF_FLAG(		"spawn at merchant",					pdwFlags,							UNIT_DATA_FLAG_SPAWN_AT_MERCHANT				)	
	EF_FLAG(		"ForceIgnoresScale",					pdwFlags,							UNIT_DATA_FLAG_FORCE_IGNORES_SCALE				)
	EF_FLAG(		"impact on fuse",						pdwFlags,							UNIT_DATA_FLAG_ADD_IMPACT_ON_FUSE				)
	EF_FLAG(		"impact on free",						pdwFlags,							UNIT_DATA_FLAG_ADD_IMPACT_ON_FREE				)
	EF_FLAG(		"impact on hit unit",					pdwFlags,							UNIT_DATA_FLAG_ADD_IMPACT_ON_HIT_UNIT			)
	EF_FLAG(		"impact on hit background",				pdwFlags,							UNIT_DATA_FLAG_ADD_IMPACT_ON_HIT_BACKGROUND		)
	EF_FLAG(		"havok ignores direction",				pdwFlags,							UNIT_DATA_FLAG_HAVOK_IMPACT_IGNORES_DIRECTION	)
	EF_FLAG(		"damages on fuse",						pdwFlags,							UNIT_DATA_FLAG_DAMAGE_ONLY_ON_FUSE				)
	EF_FLAG(		"hits units",							pdwFlags,							UNIT_DATA_FLAG_HITS_UNITS						)
	EF_FLAG(		"kill on unit hit",						pdwFlags,							UNIT_DATA_FLAG_KILL_ON_UNIT_HIT					)
	EF_FLAG(		"hits background",						pdwFlags,							UNIT_DATA_FLAG_HITS_BACKGROUND					)
	EF_FLAG(		"no ray collision",						pdwFlags,							UNIT_DATA_FLAG_NO_RAY_COLLISION					)
	EF_FLAG(		"kill on background",					pdwFlags,							UNIT_DATA_FLAG_KILL_ON_BACKGROUND_HIT			)
	EF_FLAG(		"stick on hit",							pdwFlags,							UNIT_DATA_FLAG_STICK_ON_HIT						)
	EF_FLAG(		"stick on init",						pdwFlags,							UNIT_DATA_FLAG_STICK_ON_INIT					)
	EF_FLAG(		"damages on hit background",			pdwFlags,							UNIT_DATA_FLAG_DAMAGES_ON_HIT_BACKGROUND		)
	EF_FLAG(		"damages on hit unit",					pdwFlags,							UNIT_DATA_FLAG_DAMAGES_ON_HIT_UNIT				)
	EF_FLAG(		"pulses stats on hit unit",				pdwFlags,							UNIT_DATA_FLAG_PULSES_STATS_ON_HIT_UNIT			)
	EF_FLAG(		"Sync",									pdwFlags,							UNIT_DATA_FLAG_SYNC								)
	EF_FLAG(		"Client only",							pdwFlags,							UNIT_DATA_FLAG_CLIENT_ONLY						)
	EF_FLAG(		"Server only",							pdwFlags,							UNIT_DATA_FLAG_SERVER_ONLY						)
	EF_FLAG(		"use source vel",						pdwFlags,							UNIT_DATA_FLAG_USE_SOURCE_VELOCITY				)
	EF_FLAG(		"must hit",								pdwFlags,							UNIT_DATA_FLAG_MUST_HIT							)
	EF_FLAG(		"follow ground",						pdwFlags,							UNIT_DATA_FLAG_FOLLOW_GROUND							)
	EF_FLAG(		"ignore target on repeat dmg",			pdwFlags,							UNIT_DATA_FLAG_IGNORE_AI_TARGET_ON_REPEAT_DMG	)	
	EF_FLAG(		"prioritize target",					pdwFlags,							UNIT_DATA_FLAG_PRIORITIZE_TARGET				)
	EF_FLAG(		"TrailEffectsUseProjectile",			pdwFlags,							UNIT_DATA_FLAG_TRAIL_EFFECTS_USE_PROJECTILE		)
	EF_FLAG(		"ImpactEffectsUseProjectile",			pdwFlags,							UNIT_DATA_FLAG_IMPACT_EFFECTS_USE_PROJECTILE	)
	EF_FLAG(		"destroy other missiles",				pdwFlags,							UNIT_DATA_FLAG_DESTROY_OTHER_MISSILES			)
	EF_FLAG(		"dont hit skill target",				pdwFlags,							UNIT_DATA_FLAG_DONT_HIT_SKILL_TARGET			)
	EF_FLAG(		"flip face direction",					pdwFlags,							UNIT_DATA_FLAG_FLIP_FACE_DIRECTION				)
	EF_FLAG(		"don't use range for skill",			pdwFlags,							UNIT_DATA_FLAG_DONT_USE_RANGE_FOR_SKILL			)
	EF_FLAG(		"pulls target",							pdwFlags,							UNIT_DATA_FLAG_PULLS_TARGET						)
	EF_FLAG(		"always check for collisions",			pdwFlags,							UNIT_DATA_FLAG_ALWAYS_CHECK_FOR_COLLISIONS		)
	EF_FLAG(		"use source appearance",				pdwFlags,							UNIT_DATA_FLAG_USE_SOURCE_APPEARANCE			)
	EF_FLAG(		"set shape percentages",				pdwFlags,							UNIT_DATA_FLAG_SET_SHAPE_PERCENTAGES			)
	EF_FLAG(		"don't transfer riders from owner",		pdwFlags,							UNIT_DATA_FLAG_DONT_TRANSFER_RIDERS_FROM_OWNER	)
	EF_FLAG(		"don't transfer damages on client",		pdwFlags,							UNIT_DATA_FLAG_DONT_TRANSFER_DAMAGES_ON_CLIENT	)
	EF_FLAG(		"is nonweapon missile",					pdwFlags,							UNIT_DATA_FLAG_IS_NONWEAPON_MISSILE				)
	EF_FLAG(		"missile ignore postlaunch",			pdwFlags,							UNIT_DATA_FLAG_MISSILE_IGNORE_POSTLAUNCH		)
	EF_FLAG(		"attacks location on hit unit",			pdwFlags,							UNIT_DATA_FLAG_ATTACKS_LOCATION_ON_HIT_UNIT		)
	EF_FLAG(		"dont deactivate with room",			pdwFlags,							UNIT_DATA_FLAG_DONT_DEACTIVATE_WITH_ROOM		)
	EF_FLAG(		"dont depopulate",						pdwFlags,							UNIT_DATA_FLAG_DONT_DEPOPULATE					)
	EF_FLAG(		"missile use ultimate owner",			pdwFlags,							UNIT_DATA_FLAG_MISSILE_USE_ULTIMATEOWNER		)
	
	EF_FLAG(		"anger others on damaged",				pdwFlags,							UNIT_DATA_FLAG_ANGER_OTHERS_ON_DAMAGED			)
	EF_FLAG(		"anger others on death",				pdwFlags,							UNIT_DATA_FLAG_ANGER_OTHERS_ON_DEATH			)
	EF_FLAG(		"always face skill target",				pdwFlags,							UNIT_DATA_FLAG_ALWAYS_FACE_SKILL_TARGET			)
	EF_FLAG(		"Set Rope End with No Target",			pdwFlags,							UNIT_DATA_FLAG_SET_ROPE_END_WITH_NO_TARGET		)
	EF_FLAG(		"force draw direction to move direction",pdwFlags,							UNIT_DATA_FLAG_FORCE_DRAW_TO_MATCH_VELOCITY		)
	EF_FLAG(		"quest name color",						pdwFlags,							UNIT_DATA_FLAG_USE_QUEST_NAME_COLOR				)
	EF_FLAG(		"do not sort weapons",					pdwFlags,							UNIT_DATA_FLAG_DONT_SORT_WEAPONS				)
	EF_FLAG(		"Ignores equip class reqs",				pdwFlags,							UNIT_DATA_FLAG_IGNORES_EQUIP_CLASS_REQS			)
	EF_FLAG(		"do not use sorce for tohit",			pdwFlags,							UNIT_DATA_FLAG_DONT_USE_SOURCE_FOR_TOHIT		)
	EF_FLAG(		"angle while pathing",					pdwFlags,							UNIT_DATA_FLAG_ANGLE_WHILE_PATHING				)
	EF_FLAG(		"don't add wardrobe layer",				pdwFlags,							UNIT_DATA_FLAG_DONT_ADD_WARDROBE_LAYER			)
	EF_FLAG(		"no gender for icon",					pdwFlags,							UNIT_DATA_FLAG_NO_GENDER_FOR_ICON				)
	EF_FLAG(		"don't use container appearance",		pdwFlags,							UNIT_DATA_FLAG_DONT_USE_CONTAINER_APPEARANCE	)
	EF_FLAG(		"subscriber only",						pdwFlags,							UNIT_DATA_FLAG_SUBSCRIBER_ONLY					)
	EF_FLAG(		"compute level requirement",			pdwFlags,							UNIT_DATA_FLAG_COMPUTE_LEVEL_REQUIREMENT		)
	EF_FLAG(		"don't fatten collision",				pdwFlags,							UNIT_DATA_FLAG_DONT_FATTEN_COLLISION_FOR_SELECTION	)
	EF_FLAG(		"automap save",							pdwFlags,							UNIT_DATA_FLAG_AUTOMAP_SAVE						)
	EF_FLAG(		"requires can operate to be known",		pdwFlags,							UNIT_DATA_FLAG_REQUIRES_CAN_OPERATE_TO_BE_KNOWN )
	EF_FLAG(		"force free on room reset",				pdwFlags,							UNIT_DATA_FLAG_FORCE_FREE_ON_ROOM_RESET			)
	EF_FLAG(		"can reflect",							pdwFlags,							UNIT_DATA_FLAG_CAN_REFLECT						)
	EF_FLAG(		"select target ignores aim pos",		pdwFlags,							UNIT_DATA_FLAG_SELECT_TARGET_IGNORES_AIM_POS	)
	EF_FLAG(		"can melee above height",				pdwFlags,							UNIT_DATA_FLAG_CAN_MELEE_ABOVE_HEIGHT			)
	EF_FLAG(		"ignore interact distance",				pdwFlags,							UNIT_DATA_FLAG_IGNORE_INTERACT_DISTANCE			)
	EF_FLAG(		"cull by screensize",					pdwFlags,							UNIT_DATA_FLAG_CULL_BY_SCREENSIZE				)
	EF_FLAG(		"is boss",								pdwFlags,							UNIT_DATA_FLAG_IS_BOSS							)
	EF_FLAG(		"link warp dest by level type",			pdwFlags,							UNIT_DATA_FLAG_LINK_WARP_DEST_BY_LEVEL_TYPE		)
	EF_FLAG(		"take responsibility on kill",			pdwFlags,							UNIT_DATA_FLAG_TAKE_RESPONSIBILITY_ON_KILL		)
	EF_FLAG(		"always known for sounds",				pdwFlags,							UNIT_DATA_FLAG_ALWAYS_KNOWN_FOR_SOUNDS			)
	EF_FLAG(		"don't collide with destructibles",		pdwFlags,							UNIT_DATA_FLAG_DONT_COLLIDE_DESTRUCTIBLES		)
	EF_FLAG(		"blocks everything",					pdwFlags,							UNIT_DATA_FLAG_BLOCKS_EVERYTHING				)
	EF_FLAG(		"everyone can target",					pdwFlags,							UNIT_DATA_FLAG_EVERYONE_CAN_TARGET				)
	EF_FLAG(		"missile plot arc",						pdwFlags,							UNIT_DATA_FLAG_MISSILE_PLOT_ARC					)
	EF_FLAG(		"missile is gore",						pdwFlags,							UNIT_DATA_FLAG_MISSILE_IS_GORE					)
	EF_FLAG(		"low lod in town",						pdwFlags,							UNIT_DATA_FLAG_LOW_LOD_IN_TOWN					)
	EF_FLAG(		"don't shrink bones",					pdwFlags,							UNIT_DATA_FLAG_DONT_SHRINK_BONES				)
	EF_FLAG(		"auto identify affixs",					pdwFlags,							UNIT_DATA_FLAG_IDENTIFY_ON_CREATE				)
	EF_FLAG(		"allow object stepping",				pdwFlags,							UNIT_DATA_FLAG_ALLOW_OBJECT_STEPPING			)
	EF_FLAG(		"cannot spawn random level treasure",	pdwFlags,							UNIT_DATA_FLAG_NO_RANDOM_LEVEL_TREASURE			)
	//EF_FLAG(		"single-player only",					pdwFlags,							UNIT_DATA_FLAG_SINGLE_PLAYER_ONLY				)
	EF_FLAG(		"multiplayer only",						pdwFlags,							UNIT_DATA_FLAG_MULTIPLAYER_ONLY					)
	EF_FLAG(		"xfer missile stats",					pdwFlags,							UNIT_DATA_FLAG_XFER_MISSILE_STATS				)
	EF_FLAG(		"specific to difficulty",				pdwFlags,							UNIT_DATA_FLAG_SPECIFIC_TO_DIFFICULTY			)
	EF_FLAG(		"is field missile",						pdwFlags,							UNIT_DATA_FLAG_IS_FIELD_MISSILE					)
	EF_FLAG(		"ignore fuse ms stat",					pdwFlags,							UNIT_DATA_FLAG_IGNORE_FUSE_MS					)
	EF_FLAG(		"match obj lvl to level",				pdwFlags,							UNIT_DATA_FLAG_SET_OBJECT_TO_LEVEL_EXPERIENCE	)
	EF_FLAG(		"is point of interest",					pdwFlags,							UNIT_DATA_FLAG_IS_POINT_OF_INTEREST				)
	EF_FLAG(		"is static point of interest",			pdwFlags,							UNIT_DATA_FLAG_IS_STATIC_POINT_OF_INTEREST		)
	EF_FLAG(		"confirm on use",						pdwFlags,							UNIT_DATA_FLAG_CONFIRM_ON_USE					)
	EF_FLAG(		"mirror in left hand",					pdwFlags,							UNIT_DATA_FLAG_MIRROR_IN_LEFT_HAND				)
	
	EF_DICT_INT(	"warp resolve time",					eWarpResolvePass,					tDictWarpResolvePass							)
	EF_LINK_INT(	"StartingLevelArea",					nStartingLevelArea,					DATATABLE_LEVEL_AREAS							)
	EF_LINK_INT(	"mode group on client",					eModeGroupClient,					DATATABLE_UNITMODE_GROUPS						)
	EF_LINK_INT(	"WarpToLevelArea",						nLinkToLevelArea,					DATATABLE_LEVEL_AREAS							)
	EF_LINK_INT(	"resides in level area",				nResidesInLevelArea,				DATATABLE_LEVEL_AREAS							)


	EF_INT(			"WarpToFloor",							nLinkToLevelAreaFloor,				0												)
	EF_LINK_INT(	"global theme required",				nGlobalThemeRequired,				DATATABLE_GLOBAL_THEMES							)
	EF_LINK_INT(	"level theme required",					nLevelThemeRequired,				DATATABLE_LEVEL_THEMES							)
//	EF_LINK_CHAR(	"Difficulty",							cDifficulty,						DATATABLE_DIFFICULTY,			0				)
	EF_FARR(		"camera target",						fCameraTarget,						0												)
	EF_FARR(		"camera eye",							fCameraEye,							0												)
// RMT INFO
	EF_BOOL(		"RMT Available",						bRMTAvailable,						FALSE											)	
	EF_INT(			"realworld_cost",						nRealWorldCost,						0												)
	EF_LINK_IARR(	"RMT Badges",							nRMTBadgeIdArray,					DATATABLE_BADGE_REWARDS							)
	EF_DICT_INT(	"RMT Tangibility",						nRMTTangibility,					tDictRMTTangibilityEnum							)
	EF_DICT_INT(	"RMT Pricing",							nRMTPricing,						tDictRMTPricingEnum							)

	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "code")
	EXCEL_ADD_ANCESTOR( DATATABLE_LEVEL_AREAS )
	EXCEL_ADD_ANCESTOR( DATATABLE_LEVEL_ZONES )
	EXCEL_ADD_ANCESTOR( DATATABLE_LEVEL )
	EXCEL_SET_NAMEFIELD("string");
	EXCEL_SET_POSTPROCESS_TABLE(ExcelUnitPostProcessCommon)
	EXCEL_SET_ROWFREE(UnitDataFreeRow)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ITEMS, UNIT_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "items") 
	EXCEL_TABLE_ADD_FILE("items_misc", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("items_weapons", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("items_armor", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
	EXCEL_TABLE_ADD_FILE("items_mods", APP_GAME_ALL, EXCEL_TABLE_PRIVATE)
EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_MONSTERS, UNIT_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "monsters") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_MISSILES, UNIT_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "missiles") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_PLAYERS, UNIT_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "players") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_OBJECTS, UNIT_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "objects") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// item_looks
EXCEL_STRUCT_DEF(ITEM_LOOK_DATA)
	EF_LINK_INT(	"item",									nItemClass,							DATATABLE_ITEMS									)
	EF_LINK_INT(	"look group",							nLookGroup,							DATATABLE_ITEM_LOOK_GROUPS						)
	EF_FILE(		"folder",								szFileFolder																		)
	EF_FILE(		"appearance",							szAppearance																		)
	EF_LINK_INT(	"wardrobe",								nWardrobe,							DATATABLE_WARDROBE_LAYER						)
	EF_INT(			"",										nAppearanceId,						INVALID_ID										)
	EXCEL_SET_INDEX(0, 0, "item", "look group")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ITEM_LOOKS, ITEM_LOOK_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "item_looks") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// treasure
STR_DICT treasurepicktype[] =
{
	{ "",				PICKTYPE_RAND					},
	{ "one",			PICKTYPE_RAND					},
	{ "all",			PICKTYPE_ALL					},
	{ "modifiers_only",	PICKTYPE_MODIFIERS				},
	{ "ind_percent",	PICKTYPE_INDEPENDENT_PERCENTS	},
	{ "one_eliminate",	PICKTYPE_RAND_ELIMINATE			},
	{ "first_valid",	PICKTYPE_FIRST_VALID			},

	{ NULL,				PICKTYPE_RAND					},
};

EXCEL_STRUCT_DEF(TREASURE_DATA)
	EF_STR_LOWER(	"treasureclass",						szTreasure																			)
	EF_LINK_UTYPE(	"allow unit types",						nAllowUnitTypes																		)
	EF_FLAG(		"required usable by spawner",			pdwFlags,							TDF_REQUIRED_USABLE_BY_SPAWNER					)
	EF_FLAG(		"required usable by operator",			pdwFlags,							TDF_REQUIRED_USABLE_BY_OPERATOR					)
	EF_LINK_INT(	"spawn from monster unittype",			nSourceUnitType,					DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"spawn from level theme",				nSourceLevelTheme,					DATATABLE_LEVEL_THEMES							)
	EF_FLAG(		"subscriber only",						pdwFlags,							TDF_SUBSCRIBER_ONLY								)
	EF_FLAG(		"multiplayer only",						pdwFlags,							TDF_MULTIPLAYER_ONLY							)
	EF_FLAG(		"single player only",					pdwFlags,							TDF_SINGLE_PLAYER_ONLY							)
	EF_FLAG(		"max slots",							pdwFlags,							TDF_MAX_SLOTS									)
	EF_FLAG(		"base on player level",					pdwFlags,							TDF_BASE_LEVEL_ON_PLAYER						)
	EF_FLOAT(		"gamble price range min",				fGamblePriceRangeMin,				0.8f											)	
	EF_FLOAT(		"gamble price range max",				fGamblePriceRangeMax,				2.0f											)
	EF_LINK_INT(	"global theme required",				nGlobalThemeRequired,				DATATABLE_GLOBAL_THEMES							)
	EF_DICT_INT(	"picktype",								nPickType,							treasurepicktype								)
	EF_PARSE(		"picks",								tNumPicksChances,					TreasureExcelParseNumPicks						)
	EF_FLOAT_PCT(	"nodrop",								fNoDrop																				)
	EF_CODE(		"level boost",							codeLevelBoost																		)		
	EF_FLOAT(		"money chance multiplier",				flMoneyChanceMultiplier,			1.0f											)
	EF_FLOAT(		"money luck chance multiplier",			flMoneyLuckChanceMultiplier,		1.0f											)
	EF_FLOAT(		"money amount multiplier",				flMoneyAmountMultiplier,			1.0f											)
	EF_FLAG(		"results not required",					pdwFlags,							TDF_RESULT_NOT_REQUIRED							)		
	EF_FLAG(		"stack treasure",						pdwFlags,							TDF_STACK_TREASURE								)			
	EF_PARSE(		"item1",								pPicks[0],							TreasureExcelParsePickItem						)
	EF_INT(			"value1",								pPicks[0].nValue																	)	
	EF_PARSE(		"item2",								pPicks[1],							TreasureExcelParsePickItem						)
	EF_INT(			"value2",								pPicks[1].nValue																	)	
	EF_PARSE(		"item3",								pPicks[2],							TreasureExcelParsePickItem						)
	EF_INT(			"value3",								pPicks[2].nValue																	)	
	EF_PARSE(		"item4",								pPicks[3],							TreasureExcelParsePickItem						)
	EF_INT(			"value4",								pPicks[3].nValue																	)	
	EF_PARSE(		"item5",								pPicks[4],							TreasureExcelParsePickItem						)
	EF_INT(			"value5",								pPicks[4].nValue																	)	
	EF_PARSE(		"item6",								pPicks[5],							TreasureExcelParsePickItem						)
	EF_INT(			"value6",								pPicks[5].nValue																	)	
	EF_PARSE(		"item7",								pPicks[6],							TreasureExcelParsePickItem						)
	EF_INT(			"value7",								pPicks[6].nValue																	)	
	EF_PARSE(		"item8",								pPicks[7],							TreasureExcelParsePickItem						)
	EF_INT(			"value8",								pPicks[7].nValue																	)	
	EF_FLAG(		"create for all players in level",		pdwFlags,							TDF_CREATE_FOR_ALL_PLAYERS_IN_LEVEL				)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "treasureclass")
	EXCEL_SET_POSTPROCESS_TABLE(TreasureExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_TREASURE,	TREASURE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "treasure") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// playerrace.txt
EXCEL_STRUCT_DEF(PLAYER_RACE_DATA)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"code",									wCode																				)
	EF_STR_TABLE(	"String",								nDisplayString,						APP_GAME_ALL									)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_PLAYER_RACE, PLAYER_RACE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "playerrace") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// playerlevels
EXCEL_STRUCT_DEF(PLAYERLEVEL_DATA)
	EF_LINK_INT(	"unittype",								nUnitType,							DATATABLE_UNITTYPES								)
	EF_INT(			"level",								nLevel																				)
	EF_INT(			"experience",							nExperience																			)
	EF_INT(			"hp_max",								nHp																					)
	EF_INT(			"attack_rating",						nAttackRating																		)
	EF_INT(			"sfx_defense",							nSfxDefenseRating																	)
	EF_INT(			"ai changer attack",					nAIChangerAttack																	)
	EF_INT(			"stat_points",							nStatPoints																			)
	EF_INT(			"skill_points",							nSkillPoints																		)
	EF_INT(			"crafting_points",						nCraftingPoints																		)
	EF_INT(			"stat respec cost",						nStatRespecCost																		)
	EF_INT(			"skill power cost",						nPowerCostPercent																	)
	EF_INT(			"headstone return cost",				nHeadstoneReturnCost																)	
	EF_INT(			"death experience penalty percent",		nDeathExperiencePenaltyPercent														)		
	EF_INT(			"restart health percent",				nRestartHealthPercent																)		
	EF_INT(			"restart power percent",				nRestartPowerPercent 																)		
	EF_INT(			"restart shield percent",				nRestartShieldPercent																)			
	EF_CODE(		"prop1",								codeProperty[0]																		)
	EF_CODE(		"prop2",								codeProperty[1]																		)
	EF_CODE(		"prop3",								codeProperty[2]																		)
	EF_CODE(		"prop4",								codeProperty[3]																		)
	EF_LINK_INT(	"SpellSlot1",							nSkillGroup[0],						DATATABLE_SKILLGROUPS							)
	EF_LINK_INT(	"SpellSlot2",							nSkillGroup[1],						DATATABLE_SKILLGROUPS							)
	EF_LINK_INT(	"SpellSlot3",							nSkillGroup[2],						DATATABLE_SKILLGROUPS							)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "unittype", "level")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelClassLevelsPostProcess)
	// EXCEL_TABLE_SORT(playerlevels)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_PLAYERLEVELS, PLAYERLEVEL_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "playerlevels") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// playerranks
EXCEL_STRUCT_DEF(PLAYERRANK_DATA)
	EF_LINK_INT(	"unittype",								nUnitType,							DATATABLE_UNITTYPES								)
	EF_INT(			"level",								nLevel																				)
	EF_INT(			"rank experience",						nRankExperience																		)
	EF_INT(			"perk points",							nPerkPoints																			)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "unittype", "level")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelClassRanksPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_PLAYERRANKS, PLAYERRANK_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "playerranks") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// character_class.txt
EXCEL_STRUCT_DEF(CHARACTER_CLASS_DATA)
	EF_STRING(		"Name",									szName																				)
	EF_INT(			"unit version to get skill respec",		nUnitVersionToGetSkillRespec,			-1											)
	EF_INT(			"unit version to get perk respec",		nUnitVersionToGetPerkRespec,			-1											)
	EF_LINK_IARR(	"Male Unit",							tClasses[GENDER_MALE].nPlayerClass,	DATATABLE_PLAYERS								)
	EF_BOOL(		"Male Enabled",							tClasses[GENDER_MALE].bEnabled														)
	EF_LINK_IARR(	"Female Unit",							tClasses[GENDER_FEMALE].nPlayerClass,	DATATABLE_PLAYERS							)
	EF_BOOL(		"Female Enabled",						tClasses[GENDER_FEMALE].bEnabled													)
	EF_STR_TABLE(	"String One Letter Code",				nStringOneLetterCode,				APP_GAME_ALL									)
	EF_BOOL(		"Default",								bDefault																			)		
	EF_LINK_INT(	"Scrap Item Class (Special)",			nItemClassScrapSpecial,				DATATABLE_ITEMS									)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_CHARACTER_CLASS, CHARACTER_CLASS_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "character_class") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// monlevel.txt
// petlevel.txt
// elite_monlevel.txt
EXCEL_STRUCT_DEF(MONSTER_LEVEL)
	EF_INT(			"level",								nLevel																				)
	EF_INT(			"hitpoints",							nHitPoints																			)
	EF_INT(			"experience",							nExperience																			)
	EF_INT(			"damage",								nDamage																				)
	EF_INT(			"attack rating",						nAttackRating																		)
	EF_INT(			"armor",								nArmor																				)
	EF_INT(			"armor buffer",							nArmorBuffer																		)
	EF_INT(			"armor regen",							nArmorRegen																			)
	EF_INT(			"tohitbonus",							nToHitBonus																			)
	EF_INT(			"shield",								nShield																				)
	EF_INT(			"shield buffer",						nShieldBuffer																		)
	EF_INT(			"shield regen",							nShieldRegen																		)
	EF_INT(			"sfx attack",							nSfxAttack																			)
	EF_INT(			"sfx strength %",						nSfxStrengthPercent																	)
	EF_INT(			"sfx defense",							nSfxDefense																			)
	EF_INT(			"interrupt attack",						nInterruptAttack																	)
	EF_INT(			"interrupt defense",					nInterruptDefense																	)
	EF_INT(			"stealth defense",						nStealthDefense																		)
	EF_INT(			"ai change attack",						nAIChangeAttack																		)
	EF_INT(			"ai change defense",					nAIChangeDefense																	)
	EF_INT(			"corpse explode points",				nCorpseExplodePoints																)
	EF_INT(			"money chance",							nMoneyChance																		)
	EF_INT(			"money min",							nMoneyMin																			)
	EF_INT(			"money delta",							nMoneyDelta																			)
	EF_INT(			"dexterity",							nDexterity																			)
	EF_INT(			"strength",								nStrength																			)
	EF_INT(			"stat_points",							nStatPoints																			)
	EF_CODE(		"item level",							codeItemLevel																		)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MONLEVEL, MONSTER_LEVEL, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "monlevel") EXCEL_TABLE_END
//EXCEL_TABLE_DEF(DATATABLE_ELITE_MONLEVEL, MONSTER_LEVEL, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "elite_monlevel") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_PETLEVEL, MONSTER_LEVEL, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "petlevel") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// monscaling.txt
EXCEL_STRUCT_DEF(MONSTER_SCALING)
	EF_INT(			"player count",							nPlayerCount																		)
	EF_FLOAT(		"player vs. monster increment pct 0",	fPlayerVsMonsterIncrementPct[0]														)
	EF_FLOAT(		"player vs. monster increment pct 1",	fPlayerVsMonsterIncrementPct[1]														)
	EF_INT(			"monster vs. player increment pct",		nMonsterVsPlayerIncrementPct														)
	EF_INT(			"experience total",						nExperience																			)
	EF_INT(			"treasure per player",					nTreasure																			)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MONSCALING, MONSTER_SCALING, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "monscaling") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// monster_names.txt
EXCEL_STRUCT_DEF(MONSTER_NAME_DATA)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																				)
	EF_STR_TABLE(	"String Key",							nStringKey,								APP_GAME_ALL								)		
	EF_LINK_INT(	"Monster Name Type",					nMonsterNameType,						DATATABLE_MONSTER_NAME_TYPES				)
	EF_BOOL(		"Is Name Override",						bIsNameOverride																		)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MONSTER_NAMES, MONSTER_NAME_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "monster_names") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// monster_name_types.txt
EXCEL_STRUCT_DEF(MONSTER_NAME_TYPE_DEFINITION)
	EF_STRING(	"Name",										szName																				)
	EF_TWOCC(	"Code",										wCode																				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MONSTER_NAME_TYPES, MONSTER_NAME_TYPE_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "monster_name_types") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// spawclass.txt
STR_DICT sgtSpawnClassPickType[] = 
{
	{ "" ,		SPAWN_CLASS_PICK_ONE },
	{ "one" ,	SPAWN_CLASS_PICK_ONE },
	{ "two",	SPAWN_CLASS_PICK_TWO },
	{ "three",	SPAWN_CLASS_PICK_THREE },
	{ "four",	SPAWN_CLASS_PICK_FOUR },
	{ "five",	SPAWN_CLASS_PICK_FIVE },
	{ "six",	SPAWN_CLASS_PICK_SIX },
	{ "seven",	SPAWN_CLASS_PICK_SEVEN },
	{ "eight",	SPAWN_CLASS_PICK_EIGHT },
	{ "nine",	SPAWN_CLASS_PICK_NINE },
	{ "all",	SPAWN_CLASS_PICK_ALL },
	{ NULL ,	SPAWN_CLASS_PICK_ONE }
};

EXCEL_STRUCT_DEF(SPAWN_CLASS_DATA)
	EF_STRING(		"SpawnClass",							szSpawnClass																		)
	EF_TWOCC(		"Code",									wCode																				)
	EF_DICT_INT(	"PickType",								ePickType,							sgtSpawnClassPickType							)
	EF_BOOL(		"RememberPick",							bLevelRemembersPicks																)	
	EF_PARSE(		"Pick1",								pPicks[0],							SpawnClassParseExcel							)
	EF_INT(			"Weight1",								pPicks[0].nPickWeight																)	
	EF_CODE(		"Count1",								pPicks[0].tEntry.codeCount															)	
	EF_PARSE(		"Pick2",								pPicks[1],							SpawnClassParseExcel							)
	EF_INT(			"Weight2",								pPicks[1].nPickWeight																)	
	EF_CODE(		"Count2",								pPicks[1].tEntry.codeCount															)	
	EF_PARSE(		"Pick3",								pPicks[2],							SpawnClassParseExcel							)
	EF_INT(			"Weight3",								pPicks[2].nPickWeight																)	
	EF_CODE(		"Count3",								pPicks[2].tEntry.codeCount															)	
	EF_PARSE(		"Pick4",								pPicks[3],							SpawnClassParseExcel							)
	EF_INT(			"Weight4",								pPicks[3].nPickWeight																)	
	EF_CODE(		"Count4",								pPicks[3].tEntry.codeCount															)	
	EF_PARSE(		"Pick5",								pPicks[4],							SpawnClassParseExcel							)
	EF_INT(			"Weight5",								pPicks[4].nPickWeight																)	
	EF_CODE(		"Count5",								pPicks[4].tEntry.codeCount															)	
	EF_PARSE(		"Pick6",								pPicks[5],							SpawnClassParseExcel							)
	EF_INT(			"Weight6",								pPicks[5].nPickWeight																)	
	EF_CODE(		"Count6",								pPicks[5].tEntry.codeCount															)	
	EF_PARSE(		"Pick7",								pPicks[6],							SpawnClassParseExcel							)
	EF_INT(			"Weight7",								pPicks[6].nPickWeight																)	
	EF_CODE(		"Count7",								pPicks[6].tEntry.codeCount															)	
	EF_PARSE(		"Pick8",								pPicks[7],							SpawnClassParseExcel							)
	EF_INT(			"Weight8",								pPicks[7].nPickWeight																)	
	EF_CODE(		"Count8",								pPicks[7].tEntry.codeCount															)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "SpawnClass")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_TABLE(SpawnClassExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_SPAWN_CLASS, SPAWN_CLASS_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "spawnclass") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// monster_quality.txt
STR_DICT sgtDictMonsterQualityType[] = 
{
	{ "" ,				MQT_CHAMPION },
	{ "champion" ,		MQT_CHAMPION },
	{ "top champion",	MQT_TOP_CHAMPION },
	{ "unique",			MQT_UNIQUE },
	{ NULL,				MQT_CHAMPION }
};

EXCEL_STRUCT_DEF(MONSTER_QUALITY_DATA)
	EF_STRING(		"Quality",								szMonsterQuality																	)
	EF_TWOCC(		"Code",									wCode																				)
	EF_INT(			"Rarity",								nRarity																				)	
	EF_LINK_INT(	"NameColor",							nNameColor,							DATATABLE_FONTCOLORS							)	
	EF_STR_TABLE(	"DisplayNameStringKey",					nDisplayNameStringKey,				APP_GAME_ALL									)	
	EF_STR_TABLE(	"ChampionFormatStringKey",				nChampionFormatStringKey,			APP_GAME_ALL									)		
	EF_BOOL(		"Pick Proper Name",						bPickProperName																		)	
	EF_DICT_INT(	"Type",									eType,								sgtDictMonsterQualityType						)	
	EF_BYTE_PCT(	"AppearanceHeightMin",					tAppearanceShapeCreate.bHeightMin,	50												)
	EF_BYTE_PCT(	"AppearanceHeightMax",					tAppearanceShapeCreate.bHeightMax,	50												)
	EF_BYTE_PCT(	"AppearanceWeightMin",					tAppearanceShapeCreate.bWeightMin,	50												)
	EF_BYTE_PCT(	"AppearanceWeightMax",					tAppearanceShapeCreate.bWeightMax,	50												)
	EF_LINK_INT(	"State",								nState,								DATATABLE_STATES								)	
	EF_FLOAT(		"MoneyChanceMultiplier",				flMoneyChanceMultiplier,			1.0f											)		
	EF_FLOAT(		"MoneyAmountMultiplier",				flMoneyAmountMultiplier,			1.0f											)			
	EF_INT(			"TreasureLevelBoost",					nTreasureLevelBoost																	)			
	EF_FLOAT(		"HealthMultiplier",						flHealthMultiplier																	)		
	EF_FLOAT(		"ToHit Multiplier",						flToHitMultiplier																	)		
	EF_CODE(		"MinionPackSize",						codeMinionPackSize																	)	
	EF_CODE(		"prop1",								codeProperties[0]																	)	
	EF_CODE(		"prop2",								codeProperties[1]																	)	
	EF_CODE(		"prop3",								codeProperties[2]																	)	
	EF_INT(			"AffixCount",							nAffixCount																			)	
	EF_CODE(		"AffixProbability1",					codeAffixChance[0]																	)
	EF_LINK_INT(	"AffixType1",							nAffixType[0],						DATATABLE_AFFIXTYPES							)
	EF_CODE(		"AffixProbability2",					codeAffixChance[1]																	)
	EF_LINK_INT(	"AffixType2",							nAffixType[1],						DATATABLE_AFFIXTYPES							)
	EF_CODE(		"AffixProbability3",					codeAffixChance[2]																	)
	EF_LINK_INT(	"AffixType3",							nAffixType[2],						DATATABLE_AFFIXTYPES							)
	EF_INT(			"experience multiplier",				nExperienceMultiplier,				1												)
	EF_INT(			"luck multiplier",						nLuckMultiplier,					1												)
	EF_LINK_INT(	"MonsterQualityDowngrade",				nMonsterQualityDowngrade,			DATATABLE_MONSTER_QUALITY						)	
	EF_LINK_INT(	"MinionQuality",						nMinionQuality,						DATATABLE_MONSTER_QUALITY						)	
	EF_BOOL(		"ShowLabel",							bShowLabel																			)		
	EF_LINK_INT(	"Treasure Class",						nTreasureClass,						DATATABLE_TREASURE								)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "quality")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MONSTER_QUALITY, MONSTER_QUALITY_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "monster_quality") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// levelscaling.txt
EXCEL_STRUCT_DEF(LEVEL_SCALING_DATA)
	EF_INT(			"level diff",							nLevelDiff																			)
	EF_INT(			"P atk M - dmg",						nPlayerAttacksMonsterDmg															)
	EF_INT(			"P atk M - exp",						nPlayerAttacksMonsterExp															)
	EF_INT(			"M atk P - dmg",						nMonsterAttacksPlayerDmg															)
	EF_INT(			"P atk P - dmg",						nPlayerAttacksPlayerDmg																)
	EF_INT(			"P atk P - karma",						nPlayerAttacksPlayerKarma															)
	EF_INT(			"P atk M - karma",						nPlayerAttacksMonsterKarma															)
	EF_INT(			"P dmg effect M",						nPlayerAttacksMonsterDmgEffect,					100									)
	EF_INT(			"P dmg effect P",						nPlayerAttacksPlayerDmgEffect,					100									)
	EF_INT(			"M dmg effect P",						nMonsterAttacksPlayerDmgEffect,					100									)

	EXCEL_SET_POSTPROCESS_TABLE(LevelScalingExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LEVEL_SCALING, LEVEL_SCALING_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "levelscaling") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// bones.txt, boneweights.txt
EXCEL_STRUCT_DEF(BONE_DATA)
	EF_STRING(	"name",										szName																				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_BONES, BONE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "bones") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_BONEWEIGHTS, BONE_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "boneweights") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// bookmarks.txt
EXCEL_STRUCT_DEF(BOOKMARK_DEFINITION)
	EF_STRING(	"Name",										szName																				)
	EF_TWOCC(	"Code",										wCode																				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_BOOKMARKS, BOOKMARK_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "bookmarks") EXCEL_TABLE_END

//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(QUEST_TEMPLATE_DEFINITION)

	EF_STRING(		"Name",									szName																				)
	EF_FOURCC(		"Code",									dwCode																				)	
	EF_BOOL(		"Update Party Kill On Setup",			bUpdatePartyKillOnSetup																)	
	EF_BOOL(		"Remove On Join Game",					bRemoveOnJoinGame																	)	
	
	EF_STRING(		"Init Function",						tFunctions.tTable[ QFUNC_INIT ].szFunctionName										)
	EF_STRING(		"Free Function",						tFunctions.tTable[ QFUNC_FREE ].szFunctionName										)
	EF_STRING(		"On Enter Game Function",				tFunctions.tTable[ QFUNC_ON_ENTER_GAME ].szFunctionName								)
	EF_STRING(		"Complete Function",					tFunctions.tTable[ QFUNC_ON_COMPLETE].szFunctionName								)
	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
#if HELLGATE_ONLY
	EXCEL_SET_POSTPROCESS_TABLE(QuestTemplateExcelPostProcess)
#endif
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_TEMPLATE, QUEST_TEMPLATE_DEFINITION, APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "quest_template") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// quest_status.txt
EXCEL_STRUCT_DEF(QUEST_STATUS_DEFINITION)
	EF_STRING(	"Name",										szName																				)
	EF_TWOCC(	"Code",										wCode																				)
	EF_BOOL(	"Is Good",									bIsGood																				)		
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_STATUS, QUEST_STATUS_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "quest_status") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// quest_state_value
EXCEL_STRUCT_DEF(QUEST_STATE_VALUE_DEFINITION)
	EF_STRING(	"Name",										szName																				)
	EF_TWOCC(	"Code",										wCode																				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_STATE_VALUE, QUEST_STATE_VALUE_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "quest_state_value") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// quest_cast.txt
EXCEL_STRUCT_DEF(QUEST_CAST_DEFINITION)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																				)
	EF_LINK_INT(	"Unit Type 0",							tCastOptions[0].nUnitType,			DATATABLE_UNITTYPES								)		
	EF_LINK_INT(	"Monster 0",							tCastOptions[0].nMonsterClass,		DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Unit Type 1",							tCastOptions[1].nUnitType,			DATATABLE_UNITTYPES								)		
	EF_LINK_INT(	"Monster 1",							tCastOptions[1].nMonsterClass,		DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Unit Type 2",							tCastOptions[2].nUnitType,			DATATABLE_UNITTYPES								)		
	EF_LINK_INT(	"Monster 2",							tCastOptions[2].nMonsterClass,		DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Unit Type 3",							tCastOptions[3].nUnitType,			DATATABLE_UNITTYPES								)		
	EF_LINK_INT(	"Monster 3",							tCastOptions[3].nMonsterClass,		DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Unit Type 4",							tCastOptions[4].nUnitType,			DATATABLE_UNITTYPES								)		
	EF_LINK_INT(	"Monster 4",							tCastOptions[4].nMonsterClass,		DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Unit Type 5",							tCastOptions[5].nUnitType,			DATATABLE_UNITTYPES								)		
	EF_LINK_INT(	"Monster 5",							tCastOptions[5].nMonsterClass,		DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Unit Type 6",							tCastOptions[6].nUnitType,			DATATABLE_UNITTYPES								)		
	EF_LINK_INT(	"Monster 6",							tCastOptions[6].nMonsterClass,		DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Unit Type 7",							tCastOptions[7].nUnitType,			DATATABLE_UNITTYPES								)		
	EF_LINK_INT(	"Monster 7",							tCastOptions[7].nMonsterClass,		DATATABLE_MONSTERS								)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_CAST, QUEST_CAST_DEFINITION, APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "quest_cast") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// quest.txt
static const STR_DICT sgtQuestStyleDict[] =
{
	{ "",			QS_INVALID },
	{ "Story",		QS_STORY },
	{ "Faction",	QS_FACTION },
	{ "Class",		QS_CLASS },
	{ "Random",		QS_RANDOM },
	{ "Adventure",	QS_ADVENTURE },
	{ NULL,			QS_INVALID }
};

EXCEL_STRUCT_DEF(QUEST_DEFINITION)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																				)	
	EF_STR_TABLE(	"Name String Key",						nStringKeyName,						APP_GAME_ALL									)
	EF_DICT_INT(	"Style",								eStyle,								sgtQuestStyleDict								)		
	EF_BOOL(		"Subscriber Only",						bSubscriberOnly,																	)
	EF_BOOL(		"Multiplayer Only",						bMultiplayerOnly,																	)
	EF_BOOL(		"Starting Quest Cheat",					bStartingQuestCheat																	)
	EF_BOOL(		"QuestCheatCompleted",					bQuestCheatCompleted																)
	EF_BOOL(		"Hide Quest Log",						bHideQuestLog																)
	EF_LINK_INT(	"Auto Activate Level",					nLevelDefAutoActivate,				DATATABLE_LEVEL									)
	EF_BOOL(		"Close On Complete",					bCloseOnComplete																	)
	EF_BOOL(		"Repeatable",							bRepeatable																			)
	EF_INT(			"Repeat Rate in seconds",				nRepeatRate,						0												)
	EF_BOOL(		"SkipActivateFanfare",					bSkipActivateFanfare																)
	EF_BOOL(		"SkipCompleteFanfare",					bSkipCompleteFanfare																)
	EF_BOOL(		"Auto-Track On Activate",				bAutoTrackOnActivate																)
	EF_INT(			"End of Act Number",					nEndOfAct,							0												)
	EF_BOOL(		"Only one difficulty",					bOneDifficulty																		)
	EF_LINK_IARR(	"Quest Prereqs",						nQuestPrereqs,						DATATABLE_QUEST									)
	EF_BOOL(		"Currently Unavailable",				bUnavailable																		)
	EF_INT(			"Min Level Prereq",						nMinLevelPrereq,					1												)		
	EF_INT(			"Max Level Prereq",						nMaxLevelPrereq,					-1												)		
	EF_LINK_INT(	"Faction Type Prereq",					nFactionTypePrereq,					DATATABLE_FACTION								)		
	EF_INT(			"Faction Amount Prereq",				nFactionAmountPrereq																)		
	EF_LINK_IARR(	"Levels Preloaded With",				nLevelDefPreloadedWith,				DATATABLE_LEVEL									)	
	EF_LINK_INT(	"Offer Reward",							nOfferReward,						DATATABLE_OFFER									)
	EF_LINK_INT(	"Starting Items Treasure Class",		nStartingItemsTreasureClass,		DATATABLE_TREASURE								)
	EF_BOOL(		"Remove Starting Items On Complete",	bRemoveStartingItemsOnComplete														)
	EF_LINK_INT(	"Cast Giver",							nCastGiver,							DATATABLE_QUEST_CAST							)
	EF_LINK_INT(	"Level Story Quest Starts In",			nLevelDefQuestStart,				DATATABLE_LEVEL									)
	EF_LINK_INT(	"Cast Rewarder",						nCastRewarder,						DATATABLE_QUEST_CAST							)
	EF_LINK_INT(	"Level Rewarder NPC",					nLevelDefRewarderNPC,				DATATABLE_LEVEL									)
	EF_LINK_INT(	"Giver Item",							nGiverItem,							DATATABLE_ITEMS									)
	EF_LINK_INT(	"Giver Item Monster",					nGiverItemMonster,					DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Giver Item Level",						nGiverItemLevelDef,					DATATABLE_LEVEL									)
	EF_FLOAT(		"Giver Item Drop Rate",					fGiverItemDropRate,					1.0f											)
	EF_INT(			"Level",								nLevel[ DIFFICULTY_NORMAL ],		1												)
	EF_INT(			"Level Nightmare",						nLevel[ DIFFICULTY_NIGHTMARE ],		31												)
	EF_FLOAT(		"Experience Multiplier",				fExperienceMultiplier																)
	EF_FLOAT(		"Money Multiplier",						fMoneyMultiplier																	)
	EF_INT(			"Stat Points",							nStatPoints																			)
	EF_INT(			"Skill Points",							nSkillPoints																		)
	EF_LINK_INT(	"Faction Type Reward",					nFactionTypeReward,					DATATABLE_FACTION								)
	EF_INT(			"Faction Amount Reward",				nFactionAmountReward																)
	
	EF_STRING(		"Init Function",						tFunctions.tTable[ QFUNC_INIT ].szFunctionName										)
	EF_STRING(		"Free Function",						tFunctions.tTable[ QFUNC_FREE ].szFunctionName										)
	EF_STRING(		"On Enter Game Function",				tFunctions.tTable[ QFUNC_ON_ENTER_GAME ].szFunctionName								)
	EF_STRING(		"Complete Function",					tFunctions.tTable[ QFUNC_ON_COMPLETE].szFunctionName								)

	EF_LINK_INT(	"Warp to Open on Activate",				nWarpToOpenActivate,				DATATABLE_OBJECTS								)
	EF_LINK_INT(	"Warp to Open on Complete",				nWarpToOpenComplete,				DATATABLE_OBJECTS								)

	EF_LINK_IARR(	"Items to remove on Abandon",			nAbandonItemList,					DATATABLE_ITEMS									)
	EF_LINK_IARR(	"Items to remove on Complete",			nCompleteItemList,					DATATABLE_ITEMS									)

	EF_STRING(		"Version Function",						szVersionFunction																	)	
	EF_BOOL(		"Remove On Join Game",					bRemoveOnJoinGame																	)
	EF_BOOL(		"Beat Game On Complete",				bBeatGameOnComplete																	)

	EF_INT(			"Weight",								nWeight																				)		
	EF_FLOAT(		"Radius",								flRadius																			)		
	EF_FLOAT(		"Height",								flHeight																			)			
	EF_FLOAT(		"Flat Z Tolerance",						flFlatZTolerance																	)				
	EF_LINK_IARR(	"Allowed DRLG Styles",					nDRLGStyleAllowed,					DATATABLE_LEVEL_DRLGS,	LEVEL_DRLG_STYLE_INDEX	)
	EF_LINK_IARR(	"Level Destinations",					nLevelDefDestinations,				DATATABLE_LEVEL									)
	EF_STRING(		"Layout Adventure",						szAdventureLayout																	)	
	EF_LINK_INT(	"Object Adventure",						nObjectAdventure,					DATATABLE_OBJECTS								)
	EF_LINK_INT(	"Treasure Room Class",					nTreasureClassTreasureRoom,			DATATABLE_TREASURE								)
	EF_LINK_INT(	"Template",								nTemplate,							DATATABLE_QUEST_TEMPLATE						)
	EF_LINK_INT(	"Description Dialog",					nDescriptionDialog,					DATATABLE_DIALOG								)	
	EF_LINK_INT(	"Complete Dialog",						nCompleteDialog,					DATATABLE_DIALOG								)	
	EF_LINK_INT(	"Incomplete Dialog",					nIncompleteDialog,					DATATABLE_DIALOG								)	
	EF_LINK_INT(	"Reward Dialog",						nRewardDialog,						DATATABLE_DIALOG								)	
	EF_LINK_INT(	"Unavailable Dialog",					nUnavailableDialog,					DATATABLE_DIALOG								)	
	EF_LINK_INT(	"Log Override State",					nLogOverrideState,					DATATABLE_QUEST_STATE							)
	EF_STR_TABLE(	"Log Override String",					nLogOverrideString,					APP_GAME_ALL									)
	EF_STR_TABLE(	"Accept Button Text",					nAcceptButtonText,					APP_GAME_ALL									)	
	EF_FLOAT(		"Time Limit",							fTimeLimit																			)				
	EF_LINK_INT(	"Objective Monster",					nObjectiveMonster,					DATATABLE_MONSTERS								)	
	EF_LINK_INT(	"Objective UnitType",					nObjectiveUnitType,					DATATABLE_UNITTYPES								)	
	EF_LINK_INT(	"Objective Object",						nObjectiveObject,					DATATABLE_OBJECTS								)
	EF_BOOL(		"Disable Spawning",						bDisableSpawning																	)
	EF_INT(			"Objective Count",						nObjectiveCount,					1												)
	EF_LINK_INT(	"Collect Item",							nCollectItem,						DATATABLE_ITEMS									)
	EF_FLOAT(		"Collect Drop Rate",					fCollectDropRate,					1.0f											)				
	EF_FLOAT(		"Explore Percent",						fExplorePercent																		)				
	EF_STRING(		"Spawn Node Label",						szSpawnNodeLabel																	)
	EF_INT(			"Spawn Count",							nSpawnCount																			)
	EF_STR_TABLE(	"Name Override String Key",				nStringKeyNameOverride,				APP_GAME_ALL									)
	EF_STR_TABLE(	"Name in Log Override String Key",		nStringKeyNameInLogOverride,		APP_GAME_ALL									)
	EF_DICT_INT(	"SubLevel Type Truth Old",						eSubLevelTypeTruthOld,					tDictSubLevelType					)	
	EF_DICT_INT(	"SubLevel Type Truth New",						eSubLevelTypeTruthNew,					tDictSubLevelType					)	
	EF_LINK_INT(	"Quest State Advance To At SubLevel Truth Old",	nQuestStateAdvanceToAtSubLevelTruthOld,	DATATABLE_QUEST_STATE				)
	EF_LINK_INT(	"Monster Boss",							nMonsterBoss,						DATATABLE_MONSTERS								)
	EF_LINK_INT(	"global theme required",				nGlobalThemeRequired,				DATATABLE_GLOBAL_THEMES							)
	EF_INT(			"Loot Bonus Percent Per Player With Quest",		nLootBonusPerPlayerWithQuest														)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_INDEX(2, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Offer Reward") // TODO cmarch: support non-unique quest rewards, if we need it
	EXCEL_SET_INDEX(3, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Giver Item")   // giver item should only drop for one quest, since we only support activation of one quest at a time for now
#if HELLGATE_ONLY
	EXCEL_SET_POSTPROCESS_TABLE(QuestExcelPostProcess)
#endif
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST, QUEST_DEFINITION, APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "quest") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// quest_state.txt
EXCEL_STRUCT_DEF(QUEST_STATE_DEFINITION)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																				)		
	EF_LINK_INT(	"Quest",								nQuest,								DATATABLE_QUEST									)	
	EF_BOOL(		"Activate With Quest",					bActivateWithQuest																	)	
	EF_BOOL(		"Restore Point",						bRestorePoint																		)		
	EF_BOOL(		"Show Log On Activate",					bShowLogOnActivate																	)
	EF_BOOL(		"Show Log On Complete",					bShowLogOnComplete																	)
	EF_BOOL(		"Auto-Track On Complete",				bAutoTrackOnComplete																)
	EF_BOOL(		"Don't Show Quick Message On Update",	bNoQuickMessageOnUpdate																)
	EF_LINK_INT(	"Dialog For State",						nDialogForState,					DATATABLE_DIALOG								)
	EF_LINK_INT(	"Monster Class For State Dialog",		nNPCForState[0],					DATATABLE_MONSTERS								)
	EF_LINK_INT(	"Interrupt Monster Class For State Dialog",	nNPCForState[1],				DATATABLE_MONSTERS								)
	EF_STR_TABLE(	"Murmur String",						nMurmurString,						APP_GAME_ALL									)	
	EF_LINK_INT(	"Dialog Full Description",				nDialogFullDesc,					DATATABLE_DIALOG								)
	EF_LINK_INT(	"Template",								nQuestTemplate,						DATATABLE_QUEST_TEMPLATE						)
	EF_LINK_INT(	"Gossip NPC 1",							nGossipNPC[0],						DATATABLE_MONSTERS								)
	EF_STR_TABLE(	"Gossip String 1",						nGossipString[0],					APP_GAME_ALL									)	
	EF_LINK_INT(	"Gossip NPC 2",							nGossipNPC[1],						DATATABLE_MONSTERS								)
	EF_STR_TABLE(	"Gossip String 2",						nGossipString[1],					APP_GAME_ALL									)	
	EF_LINK_INT(	"Gossip NPC 3",							nGossipNPC[2],						DATATABLE_MONSTERS								)
	EF_STR_TABLE(	"Gossip String 3",						nGossipString[2],					APP_GAME_ALL									)	
	EF_LINK_INT(	"Log Unit Type 0",						tQuestLogOptions[0].nUnitType,		DATATABLE_UNITTYPES								)
	EF_STR_TABLE(	"Log String 0",							tQuestLogOptions[0].nString,		APP_GAME_ALL									)	
	EF_LINK_INT(	"Log Unit Type 1",						tQuestLogOptions[1].nUnitType,		DATATABLE_UNITTYPES								)
	EF_STR_TABLE(	"Log String 1",							tQuestLogOptions[1].nString,		APP_GAME_ALL									)	
	EF_LINK_INT(	"Log Unit Type 2",						tQuestLogOptions[2].nUnitType,		DATATABLE_UNITTYPES								)
	EF_STR_TABLE(	"Log String 2",							tQuestLogOptions[2].nString,		APP_GAME_ALL									)	
	EF_LINK_INT(	"Log Unit Type 3",						tQuestLogOptions[3].nUnitType,		DATATABLE_UNITTYPES								)
	EF_STR_TABLE(	"Log String 3",							tQuestLogOptions[3].nString,		APP_GAME_ALL									)	
	EF_LINK_INT(	"Log Unit Type 4",						tQuestLogOptions[4].nUnitType,		DATATABLE_UNITTYPES								)
	EF_STR_TABLE(	"Log String 4",							tQuestLogOptions[4].nString,		APP_GAME_ALL									)	
	EF_LINK_INT(	"Log Unit Type 5",						tQuestLogOptions[5].nUnitType,		DATATABLE_UNITTYPES								)
	EF_STR_TABLE(	"Log String 5",							tQuestLogOptions[5].nString,		APP_GAME_ALL									)	
	EF_LINK_INT(	"Log Unit Type 6",						tQuestLogOptions[6].nUnitType,		DATATABLE_UNITTYPES								)
	EF_STR_TABLE(	"Log String 6",							tQuestLogOptions[6].nString,		APP_GAME_ALL									)	
	EF_LINK_INT(	"Log Unit Type 7",						tQuestLogOptions[7].nUnitType,		DATATABLE_UNITTYPES								)
	EF_STR_TABLE(	"Log String 7",							tQuestLogOptions[7].nString,		APP_GAME_ALL									)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_QUEST_STATE, QUEST_STATE_DEFINITION, APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "quest_state") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// offer.txt
EXCEL_STRUCT_DEF(OFFER_DEFINITION)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																				)	
	EF_BOOL(		"No Duplicates",						bNoDuplicates																		)
	EF_BOOL(		"Do Not Identify",						bDoNotIdentify																		)	
	EF_INT(			"Num Allowed Takes",					nNumAllowedTakes,					REWARD_TAKE_ALL									)
	EF_STR_TABLE(	"Offer String",							nOfferStringKey,					APP_GAME_ALL									)	
	EF_LINK_INT(	"Treasure 0 Unit Type",					tOffers[0].nUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Treasure 0",							tOffers[0].nTreasure,				DATATABLE_TREASURE								)
	EF_LINK_INT(	"Treasure 1 Unit Type",					tOffers[1].nUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Treasure 1",							tOffers[1].nTreasure,				DATATABLE_TREASURE								)
	EF_LINK_INT(	"Treasure 2 Unit Type",					tOffers[2].nUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Treasure 2",							tOffers[2].nTreasure,				DATATABLE_TREASURE								)
	EF_LINK_INT(	"Treasure 3 Unit Type",					tOffers[3].nUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Treasure 3",							tOffers[3].nTreasure,				DATATABLE_TREASURE								)
	EF_LINK_INT(	"Treasure 4 Unit Type",					tOffers[4].nUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Treasure 4",							tOffers[4].nTreasure,				DATATABLE_TREASURE								)
	EF_LINK_INT(	"Treasure 5 Unit Type",					tOffers[5].nUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Treasure 5",							tOffers[5].nTreasure,				DATATABLE_TREASURE								)
	EF_LINK_INT(	"Treasure 6 Unit Type",					tOffers[6].nUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Treasure 6",							tOffers[6].nTreasure,				DATATABLE_TREASURE								)
	EF_LINK_INT(	"Treasure 7 Unit Type",					tOffers[7].nUnitType,				DATATABLE_UNITTYPES								)
	EF_LINK_INT(	"Treasure 7",							tOffers[7].nTreasure,				DATATABLE_TREASURE								)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_OFFER, OFFER_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "offer") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// objecttriggers.txt
EXCEL_STRUCT_DEF(OBJECT_TRIGGER_DATA)
	EF_STRING(		"name",									pszName																				)
	EF_INT(			"clear trigger",						bClearTrigger																		)
	EF_STRING(		"function",								szTriggerFunction																	)			
	EF_INT(			"is warp",								bIsWarp																				)
	EF_INT(			"is sublevel warp",						bIsSubLevelWarp																		)	
	EF_INT(			"is door",								bIsDoor																				)	
	EF_INT(			"allows invalid destination",			bAllowsInvalidLevelDestination														)
	EF_INT(			"is dynamic warp",						bIsDynamicWarp																		)
	EF_INT(			"is blocking",							bIsBlocking																			)
	EF_LINK_INT(	"state blocking",						nStateBlocking,						DATATABLE_STATES								)	
	EF_LINK_INT(	"state open",							nStateOpen,							DATATABLE_STATES								)
	EF_BOOL(		"dead can trigger",						bDeadCanTrigger,					FALSE											)	
	EF_BOOL(		"ghost can trigger",					bGhostCanTrigger,					FALSE											)
	EF_BOOL(		"hardcore dead can trigger",			bHardcoreDeadCanTrigger,			FALSE											)	
	EF_BOOL(		"object required check ingnored",		bObjectRequiredCheckIgnored,		FALSE											)	
	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelObjectTriggersPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_OBJECTTRIGGERS, OBJECT_TRIGGER_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "objecttriggers") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// task_status.txt
EXCEL_STRUCT_DEF(TASK_STATUS_DEFINITION)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_TASK_STATUS, TASK_STATUS_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "task_status") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// tasks.txt
EXCEL_STRUCT_DEF(TASK_DEFINITION)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																 				)
	EF_BOOL(		"Implemented",							bImplemented																		)
	EF_INT(			"Rarity",								nRarity																				)
	EF_BOOL(		"HostileAreaOnly",						bHostileAreasOnly																	)	
	EF_BOOL(		"AccessibleAreaOnly",					bAccessibleAreaOnly																	)	
	EF_BOOL(		"CanSave",								bCanSave																			)	
	EF_BOOL(		"DoNotOfferSimilarTasks",				bDoNotOfferSimilarTask																)	
	EF_INT(			"MinSlotsOnReward",						nMinSlotsOnReward																	)		
	EF_BOOL(		"FillAllRewardSlots",					bFillAllRewardSlots																	)			
	EF_INT(			"FilledSlotsOnForgeReward",				nFilledSlotsOnForgeReward															)			
	EF_BOOL(		"CollectModdedToRewards",				bCollectModdedToRewards																)				
	EF_CODE(		"TimeLimitInMinutes",					codeTimeLimit																		)
	EF_CODE(		"ExterminateCount",						codeExterminateCount																)
	EF_CODE(		"TriggerPercent",						codeTriggerPercent																	)
	EF_LINK_INT(	"Object Class",							nObjectClass,						DATATABLE_OBJECTS								)	
	EF_STR_TABLE(	"NameStringKey",						nNameStringKey,						APP_GAME_ALL									)
	EF_LINK_INT(	"DescriptionDialog",					nDescriptionDialog,					DATATABLE_DIALOG								)	
	EF_LINK_INT(	"CompleteDialog",						nCompleteDialog,					DATATABLE_DIALOG								)	
	EF_LINK_INT(	"IncompleteDialog",						nIncompleteDialog,					DATATABLE_DIALOG								)	
	EF_INT(			"Num Reward Takes",						nNumRewardTakes,					REWARD_TAKE_ALL									)		
	EF_LINK_INT(	"TreasureClassReward",					nTreasureClassReward,				DATATABLE_TREASURE								)	
	EF_LINK_INT(	"TreasureClassCollect",					nTreasureClassCollect,				DATATABLE_TREASURE								)	
	EF_STRING(		"CreateFunction",						szCreateFunction																	)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_TABLE(TasksExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_TASKS, TASK_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "tasks") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// gossip.txt
static const STR_DICT sgtQuestStatusDict[] =
{
	{ "",			QUEST_STATUS_INVALID },
	{ "inactive",	QUEST_STATUS_INACTIVE },
	{ "active",		QUEST_STATUS_ACTIVE },
	{ "complete",	QUEST_STATUS_COMPLETE },
	{ "offering",	QUEST_STATUS_OFFERING },	
	{ "closed",		QUEST_STATUS_CLOSED },	
	{ "fail",		QUEST_STATUS_FAIL },
	{ NULL,		0 }
};
EXCEL_STRUCT_DEF(GOSSIP_DATA)
	EF_LINK_INT(	"npc",									nNPCMonsterClass,					DATATABLE_MONSTERS								)
	EF_LINK_INT(	"quest",								nQuest,								DATATABLE_QUEST									)
	EF_LINK_INT(	"questTugboat",							nQuestTugboat,						DATATABLE_QUESTS_TASKS_FOR_TUGBOAT				)
	EF_INT(			"playerlevel",							nPlayerLevel,						0												)
	EF_INT(			"priority",								nPriority,							0												)
	EF_DICT_INT(	"quest status",							eQuestStatus,						sgtQuestStatusDict								)
	EF_LINK_INT(	"dialog",								nDialog,							DATATABLE_DIALOG								)
	EXCEL_SET_INDEX(0, 0, "npc")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_GOSSIP, GOSSIP_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "gossip") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// materials_collision.txt
EXCEL_STRUCT_DEF(ROOM_COLLISION_MATERIAL_DATA)
	EF_STRING(		"Name",									pszName																				)
	EF_INT(			"Material Number",						nMaterialNumber																		)
	EF_BOOL(		"Floor",								bFloor																				)
	EF_LINK_INT(	"Maps To",								nMapsTo,							DATATABLE_MATERIALS_COLLISION					)	
	EF_FLOAT(		"Direct Occlusion",						fDirectOcclusion																	)
	EF_FLOAT(		"Reverb Occlusion",						fReverbOcclusion																	)
	EF_LINK_INT(	"Debug Color",							nDebugColor,						DATATABLE_FONTCOLORS							)	
	EF_STRING(		"Max9 Name",							pszMax9Name																			)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MATERIALS_COLLISION, ROOM_COLLISION_MATERIAL_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "materials_collision") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// materials_global.txt
EXCEL_STRUCT_DEF(GLOBAL_MATERIAL_DEFINITION)
	EF_STRING(		"Name",									szName																				)
	EF_STRING(		"Material",								szMaterial																			)
	EF_INT(			"",										nID,								INVALID_ID										)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MATERIALS_GLOBAL, GLOBAL_MATERIAL_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "materials_global") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// chat_instanced_channels.txt
EXCEL_STRUCT_DEF(CHANNEL_DEFINITION)
EF_STRING(		"Name",									szName																				)
EF_BOOL(		"Opt out",								bOptOut																				)	
EF_INT(			"Max members",							nMaxMembers,						200												)
EF_CODE(		"code",									codeScriptChannelCode																		)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_CHAT_INSTANCED_CHANNELS, CHANNEL_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "chat_instanced_channels") EXCEL_TABLE_END


//----------------------------------------------------------------------------
// chatfilter.txt
EXCEL_STRUCT_DEF(FILTER_WORD_DEFINITION)
EF_STRING(		"Word",									szWord																				)
EF_BOOL(		"Allow prefixed",						bAllowPrefixed																		)	
EF_BOOL(		"Allow suffixed",						bAllowSuffixed																		)	
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_FILTER_CHATFILTER, FILTER_WORD_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "chatfilter") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// namefilter.txt
// uses above FILTER_WORD_DEFINITION
EXCEL_TABLE_DEF(DATATABLE_FILTER_NAMEFILTER, FILTER_WORD_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "namefilter") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// global_themes.txt
STR_DICT sgGlobalThemeMonthDict[] =
{
	{ "",					GTM_INVALID		},
	{ "January",			GTM_JANUARY		},
	{ "February",			GTM_FEBRUARY	},
	{ "March",				GTM_MARCH		},
	{ "April",				GTM_APRIL		},
	{ "May",				GTM_MAY			},
	{ "June",				GTM_JUNE		},
	{ "July",				GTM_JULY		},
	{ "August",				GTM_AUGUST		},
	{ "September",			GTM_SEPTEMBER	},
	{ "October",			GTM_OCTOBER		},
	{ "November",			GTM_NOVEMBER	},
	{ "December",			GTM_DECEMBER	},
	{ NULL,					GTM_INVALID		},
};

STR_DICT sgGlobalThemeWeekdayDict[] =
{
	{ "",					GTW_INVALID		},
	{ "Sunday",				GTW_SUNDAY		},
	{ "Monday",				GTW_MONDAY		},
	{ "Tuesday",			GTW_TUESDAY		},
	{ "Wednesday",			GTW_WEDNESDAY	},
	{ "Thursday",			GTW_THURSDAY	},
	{ "Friday",				GTW_FRIDAY		},
	{ "Saturday",			GTW_SATURDAY	},
	{ NULL,					GTW_INVALID		},
};

EXCEL_STRUCT_DEF(GLOBAL_THEME_DATA)
	EF_STRING(		"Name",									szName																	)
	EF_BOOL(		"activate by time",						bActivateByTime,						FALSE							)
	EF_DICT_INT(	"start month",							nStartMonth,							sgGlobalThemeMonthDict			)
	EF_INT(			"start day",							nStartDay,								-1								)
	EF_DICT_INT(	"start day of week",					nStartDayOfWeek,						sgGlobalThemeWeekdayDict		)
	EF_DICT_INT(	"end month",							nEndMonth,								sgGlobalThemeMonthDict			)
	EF_INT(			"end day",								nEndDay,								-1								)
	EF_DICT_INT(	"end day of week",						nEndDayOfWeek,							sgGlobalThemeWeekdayDict		)
	EF_LINK_IARR(	"treasureclass pre and post 0",			pnTreasureClassPreAndPost[ 0 ],			DATATABLE_TREASURE				)
	EF_LINK_IARR(	"treasureclass pre and post 1",			pnTreasureClassPreAndPost[ 1 ],			DATATABLE_TREASURE				)
	EF_LINK_IARR(	"treasureclass pre and post 2",			pnTreasureClassPreAndPost[ 2 ],			DATATABLE_TREASURE				)
	EF_LINK_IARR(	"treasureclass pre and post 3",			pnTreasureClassPreAndPost[ 3 ],			DATATABLE_TREASURE				)
	EF_LINK_IARR(	"treasureclass pre and post 4",			pnTreasureClassPreAndPost[ 4 ],			DATATABLE_TREASURE				)
	EF_LINK_IARR(	"treasureclass pre and post 5",			pnTreasureClassPreAndPost[ 5 ],			DATATABLE_TREASURE				)
	EF_LINK_IARR(	"treasureclass pre and post 6",			pnTreasureClassPreAndPost[ 6 ],			DATATABLE_TREASURE				)
	EF_LINK_IARR(	"treasureclass pre and post 7",			pnTreasureClassPreAndPost[ 7 ],			DATATABLE_TREASURE				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_POSTPROCESS_TABLE(ExcelGlobalThemesPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_GLOBAL_THEMES, GLOBAL_THEME_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "global_themes") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// loading_tips.txt
EXCEL_STRUCT_DEF(LOADING_TIP_DATA)
	EF_STRING(		"Name",									szName																				)
	EF_INT(			"weight",								nWeight,								1											)	
	EF_CODE(		"condition",							codeCondition																		)	
	EF_BOOL(		"don't use without a game",				bDontUseWithoutAGame																)	
	EF_STR_TABLE(	"String Key",							nStringKey,								APP_GAME_ALL								)	
	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LOADING_TIPS, LOADING_TIP_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "loading_tips") EXCEL_TABLE_END


//----------------------------------------------------------------------------
// loading_screen.txt
EXCEL_STRUCT_DEF(LOADING_SCREEN_DATA)
EF_STRING(		"Name",									szName																				)
EF_STRING(		"path",									szPath																				)
EF_INT(			"weight",								nWeight,								1											)	
EF_CODE(		"condition",							codeCondition																		)	

EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_LOADING_SCREEN, LOADING_SCREEN_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "loading_screen") EXCEL_TABLE_END

////----------------------------------------------------------------------------
//// difficulty.txt
EXCEL_STRUCT_DEF(DIFFICULTY_DATA)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																				)	
	EF_STR_TABLE(	"Unlocked String",						nUnlockedString,					APP_GAME_HELLGATE								)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_DIFFICULTY, DIFFICULTY_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "difficulty") EXCEL_TABLE_END

////----------------------------------------------------------------------------
//// act.txt
EXCEL_STRUCT_DEF(ACT_DATA)
	EF_STRING(		"Name",									szName																				)
	EF_TWOCC(		"Code",									wCode																				)	
	EF_FLAG(		"Beta Account Can Play",				dwActDataFlags,						ADF_BETA_ACCOUNT_CAN_PLAY						)	
	EF_FLAG(		"Non-Subscriber Account Can Play",		dwActDataFlags,						ADF_NONSUBSCRIBER_ACCOUNT_CAN_PLAY				)	
	EF_FLAG(		"Trial Account Can Play",				dwActDataFlags,						ADF_TRIAL_ACCOUNT_CAN_PLAY						)		
	EF_INT(			"Minimum Experience Level To Enter",	nMinExperienceLevelToEnter,			1												)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")
	EXCEL_SET_POSTPROCESS_TABLE(ActExcelPostProcess)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ACT, ACT_DATA, APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "act") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// musicscriptdebug.txt
EXCEL_STRUCT_DEF(MUSIC_SCRIPT_DEBUG_DATA)
	EF_STRING(		"Name",									pszName																				)
	EF_CODE(		"Script",								codeScript																			)		
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE_NONBLANK, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_MUSIC_SCRIPT_DEBUG, MUSIC_SCRIPT_DEBUG_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "musicscriptdebug") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// ui_component.txt
EXCEL_STRUCT_DEF(UI_COMPONENT_DATA)
	EF_STRING(		"Name",											szName																		)
	EF_STRING(		"Component String",								szComponentString															)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_UI_COMPONENT, UI_COMPONENT_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "ui_component") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// achievementslots.txt
EXCEL_STRUCT_DEF(ACHIEVEMENT_UNLOCK_PLAYER_DATA)
	EF_STRING(		"Slot Name",					szName																	)
	EF_TWOCC(		"Code",							wCode																	)	
	EF_CODE(		"conditional script",			codeConditional															)	
	EF_INT(			"unlocks at level",				nPlayerLevel,							-1								)	
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Code")	
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Slot Name")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ACHIEVEMENT_SLOTS,	ACHIEVEMENT_UNLOCK_PLAYER_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "achievementslots") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// achievements.txt
STR_DICT dicAchievementTypes[] =
{
	{ "Kill",			ACHV_KILL },
	{ "Die",			ACHV_DIE },
	{ "WeaponKill",		ACHV_WEAPON_KILL },
	{ "Equip",			ACHV_EQUIP },

	{ "QuestComplete",	ACHV_QUEST_COMPLETE },
	{ "Collect",		ACHV_COLLECT },
	{ "MinigameWin",	ACHV_MINIGAME_WIN },
	{ "CubeUse",		ACHV_CUBE_USE },
	{ "ItemUse",		ACHV_ITEM_USE },
	{ "ItemMod",		ACHV_ITEM_MOD },
	{ "ItemCraft",		ACHV_ITEM_CRAFT },
	{ "ItemUpgrade",	ACHV_ITEM_UPGRADE },
	{ "ItemIdentify",	ACHV_ITEM_IDENTIFY },
	{ "ItemDismantle",	ACHV_ITEM_DISMANTLE },
	{ "LevelTime",		ACHV_LEVELING_TIME },
	{ "SkillKill",		ACHV_SKILL_KILL },
	{ "SkillToLevel",	ACHV_SKILL_LEVEL },
	{ "FinishSkillTree",ACHV_SKILL_TREE_COMPLETE },
	{ "LevelFind",		ACHV_LEVEL_FIND },
	{ "PartyInvite",	ACHV_PARTY_INVITE },
	{ "PartyAccept",	ACHV_PARTY_ACCEPT },
	{ "InventoryFill",	ACHV_INVENTORY_FILL },
	{ "TimedKills",		ACHV_TIMED_KILLS },
	{ "StatValue",		ACHV_STAT_VALUE },
	{ "Summon",			ACHV_SUMMON},

	{ NULL,				ACHV_KILL	},
};

STR_DICT dicRevealTypes[] =
{
	{ "Always",				REVEAL_ACHV_ALWAYS },
	{ "Amt complete",		REVEAL_ACHV_AMOUNT },
	{ "Completion",			REVEAL_ACHV_COMPLETE },
	{ "Parent Complete",	REVEAL_ACHV_PARENT_COMPLETE },

	{ NULL,					REVEAL_ACHV_ALWAYS	},
};


EXCEL_STRUCT_DEF(ACHIEVEMENT_DATA)
	EF_STRING(		"Name",							szName																	)
	EF_TWOCC(		"Code",							wCode																	)	

	EF_STR_TABLE(	"Name string",					nNameString,					APP_GAME_ALL							)	
	EF_STR_TABLE(	"Descrip format string",		nDescripString,					APP_GAME_ALL							)	
	EF_STR_TABLE(	"Details string",				nDetailsString,					APP_GAME_ALL							)	
	EF_STRING(		"Icon",							szIcon,																	)
	EF_STR_TABLE(	"reward type string",			nRewardTypeString,				APP_GAME_ALL							)	

	EF_DICT_INT(	"Reveal condition",				eRevealCondition,				dicRevealTypes							)
	EF_INT(			"Reveal value",					nRevealValue,					0										)
	EF_LINK_INT(	"Reveal Parent Achievement",	nRevealParentID,				DATATABLE_ACHIEVEMENTS					)
	EF_LINK_INT(	"Not Active Till Parent Complete",nActiveWhenParentIDIsComplete,DATATABLE_ACHIEVEMENTS					)
	
	EF_LINK_UTYPE(	"Player Class",					nPlayerType																)
	EF_DICT_INT(	"Type",							eAchievementType,				dicAchievementTypes						)
	EF_INT(			"Complete number",				nCompleteNumber,				0										)
	EF_INT(			"Param 1",						nParam1,						0										)

	EF_LINK_UTYPE(	"Monster Unit type",			nUnitType																)
	EF_LINK_INT(	"Monster",						nMonsterClass,					DATATABLE_MONSTERS						)
	EF_LINK_INT(	"Object",						nObjectClass,					DATATABLE_OBJECTS						)
	EF_LINK_UTYPE(	"Item Unittype",				nItemUnittype															)
	EF_LINK_INT(	"Quality",						nItemQuality,					DATATABLE_ITEM_QUALITY					)
	EF_LINK_INT(	"Skill",						nSkill,							DATATABLE_SKILLS						)
	EF_LINK_INT(	"Level",						nLevel,							DATATABLE_LEVEL							)	
	EF_LINK_INT(	"Stat",							nStat,							DATATABLE_STATS							)	
	EF_LINK_INT(	"quest task complete",			nQuestTaskTugboat,				DATATABLE_QUESTS_TASKS_FOR_TUGBOAT		)	
	EF_INT(			"Random Quests",				nRandomQuests,					0										)
	EF_INT(			"Crafting Failures",			nCraftingFailures,				0										)

	EF_INT(			"Reward Achievement Points",	nRewardAchievementPoints,		0										)
	EF_LINK_INT(	"Reward Treasure Class",		nRewardTreasureClass,			DATATABLE_TREASURE						)
	EF_INT(			"Reward XP",					nRewardXP,						0										)
	EF_LINK_INT(	"Reward Skill",					nRewardSkill,					DATATABLE_SKILLS						)
	EF_CODE(		"Reward Script",				codeRewardScript														)	
	EF_STR_TABLE(	"Reward Title",					nRewardTitleString,				APP_GAME_ALL							)	
		
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Name")
	EXCEL_SET_INDEX(1, EXCEL_INDEX_WARN_DUPLICATE, "Code")	
	EXCEL_SET_POSTPROCESS_TABLE(AchievementsExcelPostProcess)	
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_ACHIEVEMENTS,	ACHIEVEMENT_DATA, APP_GAME_ALL, EXCEL_TABLE_PRIVATE, "achievements") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// versioningunits.txt
EXCEL_STRUCT_DEF(VERSIONING_UNITS)
	EF_INT(			"unitversion",					nUnitVersion															)
	EF_LINK_TABLE(	"table",						eTable																	)
	EF_LINK_INT(	"unittype include",				nUnitTypeInclude,				DATATABLE_UNITTYPES						)
	EF_LINK_INT(	"unittype exclude",				nUnitTypeExclude,				DATATABLE_UNITTYPES						)
	EF_DSTRING(		"class",						pszUnitClass															)
	EF_CODE(		"script",						codeScript																)	
	EXCEL_SET_POSTPROCESS_TABLE(VersioningUnitsExcelPostProcess)	
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_VERSIONINGUNITS,	VERSIONING_UNITS, APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "versioningunits") EXCEL_TABLE_END

//----------------------------------------------------------------------------
// versioningaffixes.txt
EXCEL_STRUCT_DEF(VERSIONING_AFFIXES)
	EF_INT(			"unitversion",					nUnitVersion															)
	EF_LINK_INT(	"affix",						nAffix,							DATATABLE_AFFIXES						)
	EF_CODE(		"script",						codeScript																)	
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_VERSIONINGAFFIXES,	VERSIONING_AFFIXES, APP_GAME_HELLGATE, EXCEL_TABLE_PRIVATE, "versioningaffixes") EXCEL_TABLE_END


//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(EMOTE_DATA)
	EF_STRING(		"Name",							szName																	)
	EF_STR_TABLE(	"command string",				nCommandString,					APP_GAME_ALL							)
	EF_STR_TABLE(   "description string",			nDescriptionString,				APP_GAME_ALL							)
	EF_STR_TABLE(	"text string",					nTextString,					APP_GAME_ALL							)
	EF_STR_TABLE(	"command string english",		nCommandStringEnglish,			APP_GAME_ALL							)
	EF_STR_TABLE(   "description string english",	nDescriptionStringEnglish,		APP_GAME_ALL							)
	EF_STR_TABLE(	"text string english",			nTextStringEnglish,				APP_GAME_ALL							)
	EF_LINK_INT(	"unit mode",					nUnitMode,						DATATABLE_UNITMODES						)
	EF_LINK_INT(	"requires achievement",			nRequiresAchievement,			DATATABLE_ACHIEVEMENTS)
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_EMOTES, EMOTE_DATA, APP_GAME_ALL, EXCEL_TABLE_SHARED, "emotes") EXCEL_TABLE_END



struct NIL_DEFINITION
{
};

EXCEL_STRUCT_DEF(NIL_DEFINITION)
EXCEL_STRUCT_END

#if ISVERSION(SERVER_VERSION)
EXCEL_TABLE_DEF(DATATABLE_DEBUG_BARS, NIL_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "debugbars") EXCEL_TABLE_END
EXCEL_TABLE_DEF(DATATABLE_RENDER_FLAGS, NIL_DEFINITION,	APP_GAME_ALL, EXCEL_TABLE_SHARED, "renderflags") EXCEL_TABLE_END
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RegisterExcelFunctions(
	void)
{
	ExcelRegisterImports(ScriptParseEx, StatsListInit, StatsListFree, VMExecI, StatsGetStat, StatsSetStat, StatsGetStatFloat, StatsSetStatFloat, WriteStats, ReadStats, StatsParseToken, 
		ScriptAddMarker, ScriptWriteToBuffer, ScriptReadFromBuffer, ScriptGetImportCRC, StringTablesLoadAll);
}


//----------------------------------------------------------------------------
// call register functions for all tables
//----------------------------------------------------------------------------
EXCEL_TABLES_BEGIN
#define DATATABLE_ENUM(e)			EXCEL_TABLE(e);
	#include "../data_common/excel/exceltables_hdr.h"
#undef	DATATABLE_ENUM
EXCEL_TABLES_END


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
struct GENUS_DATATABLE_LOOKUP
{
	GENUS		eGenus;
	EXCELTABLE	eTable;
};

static GENUS_DATATABLE_LOOKUP sgtGenusDatatableLookup[NUM_GENUS] =
{
	{ GENUS_NONE,		DATATABLE_NONE },
	{ GENUS_PLAYER,		DATATABLE_PLAYERS },
	{ GENUS_MONSTER,	DATATABLE_MONSTERS },
	{ GENUS_MISSILE,	DATATABLE_MISSILES },
	{ GENUS_ITEM,		DATATABLE_ITEMS },
	{ GENUS_OBJECT,		DATATABLE_OBJECTS }
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCELTABLE ExcelGetDatatableByGenus(
	GENUS eGenus)
{
	return sgtGenusDatatableLookup[eGenus].eTable;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GENUS ExcelGetGenusByDatatable(
	EXCELTABLE eTable)
{
	for (int i = 0; i < NUM_GENUS; ++i)
	{
		const GENUS_DATATABLE_LOOKUP *pLookup = &sgtGenusDatatableLookup[i];
		if (pLookup->eTable == eTable)
		{
			return pLookup->eGenus;
		}
	}
	return GENUS_NONE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GENUS GlobalIndexGetUnitGenus(
	GLOBAL_INDEX eGlobalIndex)
{
	EXCELTABLE eTable = GlobalIndexGetDatatable(eGlobalIndex);
	return ExcelGetGenusByDatatable(eTable);
}
	

