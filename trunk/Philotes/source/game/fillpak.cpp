//----------------------------------------------------------------------------
// FILE: fillpak.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers

#include "ai_priv.h"
#include "c_backgroundsounds.h"
#include "c_music.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_sound_memory.h"
#include "c_sound_priv.h"
#include "c_appearance.h"
#include "drlg.h"
#include "drlgpriv.h"
#include "e_model.h"
#include "e_compatdb.h"
#include "excel.h"
#include "filepaths.h"
#include "pakfile.h"
#include "fillpak.h"
#include "level.h"
#include "room.h"
#include "room_layout.h"
#include "room_path.h"
#include "skills.h"
#include "states.h"
#include "unit_priv.h"
#include "wardrobe.h"
#include "uix.h"
#include "uix_priv.h"
#include "dictionary.h"
#include "e_wardrobe.h"	// CHB 2006.10.18 - Needed for e_WardrobeInit().
#include "e_effect_priv.h"
#include "weapons_priv.h"
#include "performance.h"
#include "weather.h"
#include "e_profile.h"
#include "e_main.h"
#include "e_model.h"
#include "c_connmgr.h"
#include "c_sound.h"
#include "excel_private.h"
#include "sku.h"
#include "language.h"
#include "stringtable.h"
#include "c_particles.h"
#include "c_movieplayer.h"
#include "e_fillpak.h"
#include "definition_priv.h"

#if ISVERSION(SERVER_VERSION)
#include "FillPakServer.h"
#endif

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

#define MAX_SKU_NAME_LEN (64)

//----------------------------------------------------------------------------

enum FPCONVERT
{
	FPCONVERT_GENERIC_FILTER,
	FPCONVERT_APPEARANCE,
	FPCONVERT_ROOM,
	FPCONVERT_PROP,
	FPCONVERT_TEXTURE,
	FPCONVERT_WARDROBEMODEL,
	FPCONVERT_WARDROBEBLEND,
	FPCONVERT_LAYOUT,
	FPCONVERT_TYPE_COUNT,
	FPCONVERT_SPECIFIC_START = FPCONVERT_APPEARANCE
};

const TCHAR* sgcFPConvertTypes[ FPCONVERT_TYPE_COUNT ] =
{
	"",
	"appearance:",
	"room:",
	"prop:",
	"texture:",
	"wardrobemodel:",
	"wardrobeblend:",
	"layout:"

};

struct FILLPAK_GLOBALS
{
	FILLPAK_GLOBALS() { for( UINT i = 0; i < FPCONVERT_TYPE_COUNT; i++ ) ptConvert[ i ] = NULL; }

	char szSKUName[ MAX_SKU_NAME_LEN ];
	STR_DICTIONARY* ptConvert[ FPCONVERT_TYPE_COUNT ];
};
static FILLPAK_GLOBALS sgtGlobals;

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#define START_SECTION( name )	{				\
	const char _szSectionName[] = name;			\
	LogMessage( "FILLPAK_TIMER:  Begin %s\n", _szSectionName );		\
	TIMER_START2(name)

#define END_SECTION()	sFillpakTraceTime( _szSectionName, TIMER_END2() );	}



//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

static void sFillpakTraceTime( const char * pszName, UINT64 nMs )
{
	UINT nS = (UINT)nMs / 1000;
	UINT nM = nS / 60;		nS %= 60;
	UINT nH = nM / 60;		nM %= 60;
	LogMessage( "FILLPAK_TIMER:  End %s.   Time: %02d:%02d:%02d\n", pszName, nH, nM, nS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FILLPAK_GLOBALS& sFillPakGetGlobals(
	void)
{
	return sgtGlobals;
}

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FillPakInitForApp(
	void)
{
	FILLPAK_GLOBALS& tGlobals = sFillPakGetGlobals();
	tGlobals.szSKUName[ 0 ] = 0;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FillPakFreeForApp(
	void)
{
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPreLoadRoomLayout(
   GAME * pGame,
   ROOM_LAYOUT_GROUP * pGroup)
{
	switch(pGroup->eType)
	{
	case ROOM_LAYOUT_ITEM_GRAPHICMODEL:
		{
			char pszFileNameWithPath[MAX_XML_STRING_LENGTH];
			PStrPrintf(pszFileNameWithPath, MAX_XML_STRING_LENGTH, "%s", pGroup->pszName);

			PropDefinitionGet(pGame, pszFileNameWithPath, TRUE, 0.0f );
		}
		break;
	case ROOM_LAYOUT_ITEM_ATTACHMENT:
		{
			ATTACHMENT_DEFINITION tDefinition;
			e_AttachmentDefInit(tDefinition);
			tDefinition.dwFlags = 0;
			tDefinition.eType = (ATTACHMENT_TYPE)pGroup->dwUnitType;
			tDefinition.nAttachedDefId = INVALID_ID;
			tDefinition.nBoneId = INVALID_ID;
			tDefinition.nVolume = pGroup->nVolume;
			PStrCopy(tDefinition.pszAttached, pGroup->pszName, MAX_XML_STRING_LENGTH);
			PStrCopy(tDefinition.pszBone, "", MAX_XML_STRING_LENGTH);
			tDefinition.vNormal = pGroup->vNormal;
			tDefinition.vPosition = pGroup->vPosition;

			c_AttachmentDefinitionLoad(pGame, tDefinition, INVALID_ID, FILE_PATH_DATA );
		}
		break;
	case ROOM_LAYOUT_ITEM_ENVIRONMENTBOX:
		{
			e_EnvironmentLoad( pGroup->pszName );
		}
		break;
	}

	for(int i=0; i<pGroup->nGroupCount; i++)
	{
		sPreLoadRoomLayout(pGame, &pGroup->pGroups[i]);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPreloadDRLGRoomLayout(
	GAME * pGame,
	DRLG_ROOM * pDRLGRoom,
	DRLG_PASS * def)
{
	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	char szLayout  [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";

	DRLGRoomGetPath(pDRLGRoom, def, szFilePath, szLayout, DEFAULT_FILE_WITH_PATH_SIZE);

	char pszFileNameNoExtension[MAX_XML_STRING_LENGTH];
	PStrRemoveExtension(pszFileNameNoExtension, MAX_XML_STRING_LENGTH, szFilePath );

	char szFilePathLayout[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( szFilePathLayout, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s", pszFileNameNoExtension, szLayout, ROOM_LAYOUT_SUFFIX );
	ROOM_LAYOUT_GROUP_DEFINITION * pLayoutDef = (ROOM_LAYOUT_GROUP_DEFINITION*)DefinitionGetByName( DEFINITION_GROUP_ROOM_LAYOUT, szFilePathLayout );
	sPreLoadRoomLayout(pGame, &pLayoutDef->tGroup);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakEffects()
{
#if FILLPAK_LOAD_EFFECTS
	START_SECTION( "Effects" );
	e_EffectsFillpak();
	END_SECTION();
#endif // FILLPAK_LOAD_EFFECTS
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakWardrobes()
{
#if FILLPAK_LOAD_WARDROBES
	START_SECTION( "Wardrobes" );
	// CHB 2006.10.18
	V( e_WardrobeInit() );
	c_WardrobeLoadAll();
	END_SECTION();
#endif // FILLPAK_LOAD_WARDROBES
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFillPakEngineCleanup()
{
	e_EngineMemoryDump();
	V( e_Cleanup( TRUE ) );
	e_EngineMemoryDump();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakUnitData(GAME * pGame)
{
#if FILLPAK_LOAD_UNITDATA
	START_SECTION( "UnitData" );
	// load all unit data
	for ( int i = 0; i < NUM_GENUS; i++ )
	{
		EXCELTABLE eTable = UnitGenusGetDatatableEnum( (GENUS) i );

		if ( eTable < 0 || eTable >= NUM_DATATABLES )
			continue;

		int nCount = ExcelGetNumRows( pGame, eTable );

		for ( int j = 0; j < nCount; j++ )
		{
			UNIT_DATA * pUnitData = (UNIT_DATA *) ExcelGetData( pGame, eTable, j );
			if ( pUnitData && !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IGNORE_IN_DAT) )
			{
				UnitDataLoad( pGame, eTable, j );
#if !ISVERSION(SERVER_VERSION)
				if(AppIsTugboat())
				{
					// We need to load any icon textures this unit might use too, to throw
					// them into the PAK
					if( PStrLen( pUnitData->szIcon )  != 0 )
					{
						char szFullName[MAX_PATH];
						PStrPrintf( szFullName, MAX_PATH, "%s%s%s", 
							FILE_PATH_ICONS,
							pUnitData->szIcon,
							".png" );
						UIX_TEXTURE* texture = (UIX_TEXTURE*)StrDictionaryFind( g_UI.m_pTextures, szFullName);
						if (!texture)
						{
							texture = UITextureLoadRaw( szFullName );
							if( texture )
							{
								StrDictionaryAdd( g_UI.m_pTextures, szFullName, texture);
							}
						}

					}
				}
#endif //!SERVER_VERSION
			}
			if (j % 100 == 99)
			{
				AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
				V( e_ModelsDestroy() );
				V( e_ModelsInit() );
				CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_UNITS ) ) );
				CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_WARDROBE ) ) );
			}
		}
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	// FileSystemReset();

	CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_UNITS ) ) );
	CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_PARTICLE ) ) );
	CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_WARDROBE ) ) );

	END_SECTION();
#endif // FILLPAK_LOAD_UNITDATA
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakRooms(GAME * pGame)
{
#if FILLPAK_LOAD_ROOMS
	START_SECTION( "Rooms" );

	// load all rooms
	int nRoomIndexCount = ExcelGetNumRows( pGame, DATATABLE_ROOM_INDEX );
	for ( int i = 0; i < nRoomIndexCount; i++ )
	{
		RoomDefinitionGet( pGame, i, TRUE, 0.0f ); // this loads the graphics

		// make sure that everything loads
		AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

		ROOM_INDEX * pRoomIndex = (ROOM_INDEX *) ExcelGetData( pGame, DATATABLE_ROOM_INDEX, i );
		if ( pRoomIndex )
		{
			if ( pRoomIndex->pRoomDefinition )
			{
				// do a little bit of clean up so that we don't run out of memory
				for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
				{
					e_ModelDefinitionDestroy( pRoomIndex->pRoomDefinition->nModelDefinitionId, 
						nLOD, MODEL_DEFINITION_DESTROY_ALL );
				}
				pRoomIndex->pRoomDefinition->nModelDefinitionId = INVALID_ID;
			}

			char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
			PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pRoomIndex->pszFile, ROOM_LAYOUT_SUFFIX );
			ROOM_LAYOUT_GROUP_DEFINITION * pLayoutDef = (ROOM_LAYOUT_GROUP_DEFINITION*)DefinitionGetByName( DEFINITION_GROUP_ROOM_LAYOUT, szFilePath );
			sPreLoadRoomLayout(pGame, &pLayoutDef->tGroup);

			PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pRoomIndex->pszFile, ROOM_PATH_NODE_SUFFIX );
			DefinitionGetByName( DEFINITION_GROUP_ROOM_PATH_NODE, szFilePath );

		}
		if ( i % 40 == 39 )
		{
			CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_BACKGROUND ) ) );
			CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_PARTICLE ) ) );
		}
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	// FileSystemReset();

	END_SECTION();
#endif // FILLPAK_LOAD_ROOMS
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakDRLGs(GAME * pGame)
{
#if FILLPAK_LOAD_DRLGS
	START_SECTION( "DRLGs" );

	// load all DRLGs
	int nRuleCount = ExcelGetNumRows( pGame, DATATABLE_LEVEL_RULES );
	for (int i = 0; i < nRuleCount; i++)
	{
		DRLG_PASS* def = (DRLG_PASS*)ExcelGetData(pGame, DATATABLE_LEVEL_RULES, i);
		if ( def )
		{
			OVERRIDE_PATH tOverride;
			OverridePathByLine( def->nPathIndex, &tOverridePath );
			ASSERT( tOverridePath.szPath[ 0 ] );
			def->pDrlgDef = DRLGLevelDefinitionGet(tOverride.szPath, def->pszDrlgFileName);
		}

		if ( i % 40 == 39 )
		{
			CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_BACKGROUND ) ) );
			CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_PARTICLE ) ) );
		}
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	// FileSystemReset();

#if FILLPAK_LOAD_DRLG_LAYOUTS
	// load all DRLGs layout overrides
	for (int i = 0; i < nRuleCount; i++)
	{
		DRLG_PASS* def = (DRLG_PASS*)ExcelGetData(pGame, DATATABLE_LEVEL_RULES, i);
		if ( def )
		{
			ASSERTV_CONTINUE(def, "Table index %d has no DRLG Pass!", i);
			ASSERTV_CONTINUE(def->pDrlgDef, "Table index %d (%s) has no DRLG definition.", i, def->pszDrlgFileName);

			// Now we need to load any overridden room layouts:
			// First the templates
			for(int j=0; j<def->pDrlgDef->nTemplateCount; j++)
			{
				for(int k=0; k<def->pDrlgDef->pTemplates[j].nRoomCount; k++)
				{
					DRLG_ROOM * pDRLGRoom = &def->pDrlgDef->pTemplates[j].pRooms[k];
					sPreloadDRLGRoomLayout(pGame, pDRLGRoom, def);
				}
			}
			// Now the substitutions
			for(int j=0; j<def->pDrlgDef->nSubstitutionCount; j++)
			{
				for(int k=0; k<def->pDrlgDef->pSubsitutionRules[j].nRuleRoomCount; k++)
				{
					DRLG_ROOM * pDRLGRoom = &def->pDrlgDef->pSubsitutionRules[j].pRuleRooms[k];
					sPreloadDRLGRoomLayout(pGame, pDRLGRoom, def);
				}

				for(int k=0; k<def->pDrlgDef->pSubsitutionRules[j].nReplacementCount; k++)
				{
					for(int l=0; l<def->pDrlgDef->pSubsitutionRules[j].pnReplacementRoomCount[k]; l++)
					{
						DRLG_ROOM * pDRLGRoom = &def->pDrlgDef->pSubsitutionRules[j].ppReplacementRooms[k][l];
						sPreloadDRLGRoomLayout(pGame, pDRLGRoom, def);
					}
				}
			}
		}

		if ( i % 40 == 39 )
		{
			CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_BACKGROUND ) ); )
				CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_PARTICLE ) ); )
		}
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	// FileSystemReset();
#endif // FILLPAK_LOAD_DRLG_LAYOUTS

	END_SECTION();
#endif // FILLPAK_LOAD_DRLGS
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFillPakLoadEnvironmentFile(
	const char * szPath,
	const char * pszEnvironmentFile,
	const char * pszSuffix)
{
	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s%s", szPath, pszEnvironmentFile, pszSuffix, ENVIRONMENT_SUFFIX );
	ENVIRONMENT_DEFINITION * pEnvDef = (ENVIRONMENT_DEFINITION *) DefinitionGetByName( DEFINITION_GROUP_ENVIRONMENT, szFilePath, -1, TRUE, (pszSuffix && pszSuffix[0]) ? TRUE : FALSE );
	REF(pEnvDef);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakEnvironments(GAME * pGame)
{
#if FILLPAK_LOAD_ENVIRONMENTS
	START_SECTION( "Environments" );

	// load all environments
	int nDRLGCount = ExcelGetNumRows( pGame, DATATABLE_LEVEL_DRLGS );
	int nThemeCount = ExcelGetNumRows( pGame, DATATABLE_LEVEL_THEMES );
	for(int i=0; i<nDRLGCount; i++)
	{
		if ( i % 40 == 39 )
		{
			CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_BACKGROUND ) ); )
				CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_PARTICLE ) ); )
		}

		DRLG_DEFINITION* def = (DRLG_DEFINITION*)ExcelGetData(pGame, DATATABLE_LEVEL_DRLGS, i);

		if ( !def )
			continue;

		OVERRIDE_PATH tOverridePath;
		char szPath    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		OverridePathByLine( def->nPathIndex, &tOverridePath );
		ASSERT_CONTINUE( tOverridePath.szPath[ 0 ] );
		PStrRemovePath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, FILE_PATH_BACKGROUND, tOverridePath.szPath );
		sFillPakLoadEnvironmentFile(szPath, def->pszEnvironmentFile, "");

		for(int j=0; j<nThemeCount; j++)
		{
			const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, j);
			if ( ! pLevelTheme )
				continue;
			if(!pLevelTheme->pszEnvironment || !pLevelTheme->pszEnvironment[0])
				continue;
			
			for(int k=0; k<MAX_LEVEL_THEME_ALLOWED_STYLES; k++)
			{
				if(pLevelTheme->nAllowedStyles[k] == def->nStyle)
				{
					sFillPakLoadEnvironmentFile(szPath, def->pszEnvironmentFile, pLevelTheme->pszEnvironment);
					break;
				}
			}
		}

		for(int j=0; j<MAX_DRLG_THEMES; j++)
		{
			if(def->nThemes[j] >= 0)
			{
				const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, j);
				if (! pLevelTheme)
					continue;
				if(!pLevelTheme->pszEnvironment || !pLevelTheme->pszEnvironment[0])
					continue;

				sFillPakLoadEnvironmentFile(szPath, def->pszEnvironmentFile, pLevelTheme->pszEnvironment);
			}
		}

		if(def->nWeatherSet >= 0)
		{
			const WEATHER_SET_DATA * pWeatherSet = (const WEATHER_SET_DATA *)ExcelGetData(pGame, DATATABLE_WEATHER_SETS, def->nWeatherSet);
			if(pWeatherSet)
			{
				for(int j=0; j<MAX_WEATHER_SETS; j++)
				{
					if(pWeatherSet->pWeatherSets[j].nWeather >= 0)
					{
						const WEATHER_DATA * pWeatherData = (const WEATHER_DATA *)ExcelGetData(pGame, DATATABLE_WEATHER, pWeatherSet->pWeatherSets[j].nWeather);
						if(pWeatherData)
						{
							for(int k=0; k<MAX_THEMES_FROM_WEATHER; k++)
							{
								if(pWeatherData->nThemes[k] >= 0)
								{
									const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, pWeatherData->nThemes[k]);
									if ( ! pLevelTheme )
										continue;
									if(!pLevelTheme->pszEnvironment || !pLevelTheme->pszEnvironment[0])
										continue;

									sFillPakLoadEnvironmentFile(szPath, def->pszEnvironmentFile, pLevelTheme->pszEnvironment);
								}
							}
						}
					}
				}
			}
		}

	}

	CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_BACKGROUND ) ); )
	CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_PARTICLE ) ); )
	CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_WARDROBE ) ); )
	CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_UNITS ) ); )

	END_SECTION();
#endif // FILLPAK_LOAD_ENVIRONMENTS
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakStates(GAME * pGame)
{
#if FILLPAK_LOAD_STATES
	START_SECTION( "States" );

	// load global materials
	V( e_LoadGlobalMaterials() );

	// load all states
	CLT_VERSION_ONLY( c_StatesLoadAll( pGame ) );

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	// FileSystemReset();

	END_SECTION();
#endif // FILLPAK_LOAD_STATES
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakSkills(GAME * pGame)
{
#if FILLPAK_LOAD_SKILLS
	START_SECTION( "Skills" );

	// load all skills
	int nSkillCount = ExcelGetNumRows( pGame, DATATABLE_SKILLS );
	for ( int i = 0; i < nSkillCount; i++ )
	{
		CLT_VERSION_ONLY( c_SkillLoad( pGame, i ) );
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	// FileSystemReset();

	END_SECTION();
#endif // FILLPAK_LOAD_SKILLS
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakMeleeWeapons(GAME * pGame)
{
#if FILLPAK_LOAD_MELEEWEAPONS
	START_SECTION( "Melee Weapons" );

	// count how many entries we need
	int nCount = ExcelGetNumRows( pGame, DATATABLE_MELEEWEAPONS );
	SKILL_EVENTS_DEFINITION * pEvents = NULL;

	// fill in the INVALID_ID entries for particles
	for (int nRow = 0; nRow < nCount; nRow++)
	{
		MELEE_WEAPON_DATA * pDef = (MELEE_WEAPON_DATA *)ExcelGetData(pGame, DATATABLE_MELEEWEAPONS, nRow);
		if (! pDef )
			continue;
		if ( pDef->szSwingEvents[ 0 ] != 0 )
		{
			int nSwingEventsId = DefinitionGetIdByName( DEFINITION_GROUP_SKILL_EVENTS, pDef->szSwingEvents );
			pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, nSwingEventsId );
			for ( int i = 0; i < pEvents->nEventHolderCount; i++ )
			{
				SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ i ];
				for ( int j = 0; j < pHolder->nEventCount; j++ )
				{
					SKILL_EVENT * pSkillEvent = &pHolder->pEvents[ j ];
					const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = SkillGetEventTypeData( pGame, pSkillEvent->nType );
					ASSERT_CONTINUE(pSkillEventTypeData);
					//Tugboat only had missiles but Looking at source control I'm guessing this just hasn't
					//been merged yet. So leaving it in.
					if ( pSkillEventTypeData->eAttachedTable == DATATABLE_MISSILES ||
						pSkillEventTypeData->eAttachedTable == DATATABLE_MONSTERS ||
						pSkillEventTypeData->eAttachedTable == DATATABLE_ITEMS ||
						pSkillEventTypeData->eAttachedTable == DATATABLE_OBJECTS )
					{
						int nUnitClass = pSkillEvent->tAttachmentDef.nAttachedDefId;
						if ( nUnitClass != INVALID_ID)
							UnitDataLoad( pGame, pSkillEventTypeData->eAttachedTable, nUnitClass );
					}
					if ( pSkillEventTypeData->bUsesAttachment )
					{
						ASSERT( pSkillEventTypeData->eAttachedTable == DATATABLE_NONE );
						c_AttachmentDefinitionLoad( pGame, pSkillEvent->tAttachmentDef, INVALID_ID, "" );
					} 
				}
			}
		}
	}
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	// FileSystemReset();			

	END_SECTION();
#endif
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static void sFillPakAIs(GAME * pGame)
{
#if FILLPAK_LOAD_AIS
	START_SECTION( "AIs" );

	// load all AIs
	unsigned int nAICount = ExcelGetNumRows(pGame, DATATABLE_AI_INIT);
	for (unsigned int ii = 0; ii < nAICount; ii++)
	{
		AI_INIT * ai_def = (AI_INIT *)ExcelGetData(pGame, DATATABLE_AI_INIT, ii);
		if (! ai_def)
			continue;
		if (ai_def->pszTableName[0] != 0)
		{
			ai_def->nTable = DefinitionGetIdByName(DEFINITION_GROUP_AI, ai_def->pszTableName);
		}
		else
		{
			ai_def->nTable = INVALID_ID;
		}
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	// FileSystemReset();

	END_SECTION();
#endif // FILLPAK_LOAD_AIS
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static BOOL sFillPak( 
	void)
{
	START_SECTION( "Fillpak" );

	GAME * pGame = AppGetCltGame();
	REF( pGame );

	e_CompatDB_LoadMasterDatabaseFile();

	sFillPakEffects();

	sFillPakWardrobes();
	sFillPakEngineCleanup();

	sFillPakUnitData(pGame);
	sFillPakEngineCleanup();

	sFillPakRooms(pGame);
	sFillPakEngineCleanup();

	sFillPakDRLGs(pGame);
	sFillPakEngineCleanup();

	sFillPakEnvironments(pGame);
	sFillPakEngineCleanup();

	sFillPakStates(pGame);

	sFillPakSkills(pGame);

	sFillPakMeleeWeapons(pGame);
	sFillPakEngineCleanup();

	sFillPakAIs(pGame);
	sFillPakEngineCleanup();

	END_SECTION(); // fillpak

	return TRUE;
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static BOOL sFillPakMin( 
	void)
{
	START_SECTION( "FillpakMin" );

	GAME * pGame = AppGetCltGame();
	REF( pGame );

	e_CompatDB_LoadMasterDatabaseFile();

	sFillPakEffects();

	sFillPakWardrobes();
	sFillPakEngineCleanup();

	sFillPakUnitData(pGame);
	sFillPakEngineCleanup();

	sFillPakStates(pGame);

	sFillPakSkills(pGame);

	sFillPakMeleeWeapons(pGame);
	sFillPakEngineCleanup();

	END_SECTION(); // fillpak

	return TRUE;
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static BOOL sFillPakSound( 
	void)
{
#if !ISVERSION(SERVER_VERSION)
	GAME * pGame = AppGetCltGame();
	REF( pGame );

	c_BGSoundsStop();
	c_SoundStopAll();
	c_MusicStop();

#if FILLPAK_LOAD_SOUNDS
	// load all sounds
	// music files are now part of the sound spreadsheet
	int nSoundCount = ExcelGetNumRows( pGame, DATATABLE_SOUNDS );
	for ( int i = 0; i < nSoundCount; i++ )
	{
		c_SoundLoad(i, SLT_FILLPAK);
	}
#endif
#endif
	return TRUE;
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct AD_INVENTORY_ELEMENT
{
	char pszLevelName[MAX_XML_STRING_LENGTH];		// Name from DATATABLE_LEVELS
	char pszZoneName[MAX_XML_STRING_LENGTH];		// Name from DATATABLE_LEVEL_DRLGS
	char pszElementName[MAX_XML_STRING_LENGTH];		// Name from the room's model definition's ad 
	int nTextureWidth;								// We've standardized on 512x512 textures, so these values will always be 512
	int nTextureHeight;								// We've standardized on 512x512 textures, so these values will always be 512
	char pszDescription[MAX_XML_STRING_LENGTH];		// Blank
	char pszRotation[MAX_XML_STRING_LENGTH];		// Set to "Zone Static"
	char pszTextureFormat[MAX_XML_STRING_LENGTH];	// We'll be using DXT1
	char pszOtherNotes[MAX_XML_STRING_LENGTH];		// Blank
	BOOL bAnyReporting;								// Are any of these ads marked as reporting?
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAdInventoryElementByName(
	SIMPLE_DYNAMIC_ARRAY<AD_INVENTORY_ELEMENT> & pAdInventoryElements,
	const LEVEL_DEFINITION * pLevelDef,
	const DRLG_DEFINITION * pDRLGDef,
	const char * pszElementName,
	BOOL bNonReporting)
{
	int nElementCount = pAdInventoryElements.Count();
	for(int i=0; i<nElementCount; i++)
	{
		AD_INVENTORY_ELEMENT & pInventoryElement = pAdInventoryElements[i];
		if(PStrCmp(pInventoryElement.pszLevelName, pLevelDef->pszName, MAX_XML_STRING_LENGTH) == 0)
		{
			if(PStrCmp(pInventoryElement.pszZoneName, pDRLGDef->pszName, MAX_XML_STRING_LENGTH) == 0)
			{
				if(PStrCmp(pInventoryElement.pszElementName, pszElementName, MAX_XML_STRING_LENGTH) == 0)
				{
					// This element already exists
					if(pInventoryElement.bAnyReporting)
					{
						// We've already got a reporting element here, so just exit out
						return;
					}
					else
					{
						// We've got the element, but all of the instances of it that we've seen are non-reporting
						// If this is a reporting element, then mark it now
						if(!bNonReporting)
						{
							pInventoryElement.bAnyReporting = TRUE;
						}
						return;
					}
				}
			}
		}
	}

	AD_INVENTORY_ELEMENT * pInventoryElement = ArrayAdd(pAdInventoryElements);
	ASSERT_RETURN(pInventoryElement);

	PStrCopy(pInventoryElement->pszLevelName, MAX_XML_STRING_LENGTH, pLevelDef->pszName, DEFAULT_INDEX_SIZE);
	PStrCopy(pInventoryElement->pszZoneName, MAX_XML_STRING_LENGTH, pDRLGDef->pszName, DEFAULT_INDEX_SIZE);
	PStrCopy(pInventoryElement->pszElementName, MAX_XML_STRING_LENGTH, pszElementName, DEFAULT_INDEX_SIZE);
	pInventoryElement->nTextureWidth = pInventoryElement->nTextureHeight = 512;
	memclear(pInventoryElement->pszDescription, MAX_XML_STRING_LENGTH * sizeof(char));
	PStrCopy(pInventoryElement->pszRotation, MAX_XML_STRING_LENGTH, "Zone Static", MAX_XML_STRING_LENGTH);
	PStrCopy(pInventoryElement->pszTextureFormat, MAX_XML_STRING_LENGTH, "DXT1", MAX_XML_STRING_LENGTH);
	memclear(pInventoryElement->pszOtherNotes, MAX_XML_STRING_LENGTH * sizeof(char));
	pInventoryElement->bAnyReporting = !bNonReporting;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAdInventoryElementsFromDRLGRoom(
	SIMPLE_DYNAMIC_ARRAY<AD_INVENTORY_ELEMENT> & pAdInventoryElements,
	const LEVEL_DEFINITION * pLevelDef,
	const DRLG_DEFINITION * pDRLGDef,
	DRLG_PASS * pPass,
	const DRLG_ROOM * pDRLGRoom)
{
	char szFilePath[DEFAULT_FILE_WITH_PATH_SIZE] = "";
	char szLayout[DEFAULT_FILE_WITH_PATH_SIZE] = "";
	DRLGRoomGetPath(pDRLGRoom, pPass, szFilePath, szLayout, DEFAULT_FILE_WITH_PATH_SIZE);

	char pszFileNameNoExtension[MAX_XML_STRING_LENGTH];
	PStrRemoveExtension(pszFileNameNoExtension, MAX_XML_STRING_LENGTH, szFilePath );

	//int nRoomIndex = ExcelGetLineByStringIndex(NULL, DATATABLE_ROOM_INDEX, pszFileNameNoExtension);
	int nRoomIndex = RoomFileGetRoomIndexLine(pszFileNameNoExtension);
	if(nRoomIndex < 0)
		return;

	ROOM_DEFINITION * pRoomDef = RoomDefinitionGet( NULL, nRoomIndex, TRUE, 0.0f ); // this loads the graphics
	if(!pRoomDef || pRoomDef->nModelDefinitionId < 0)
		return;

	// Iterate through the Inventory Element Names here from the nModelDefId.
	int nAdObjectCount = 0;
	V_ACTION ( e_ModelDefinitionGetAdObjectDefCount(pRoomDef->nModelDefinitionId, LOD_ANY, nAdObjectCount), return;, TRUE, );

	for(int j=0; j<nAdObjectCount; j++)
	{
		AD_OBJECT_DEFINITION tAdObjDef;
		V_CONTINUE ( e_ModelDefinitionGetAdObjectDef(pRoomDef->nModelDefinitionId, LOD_ANY, j, tAdObjDef) );

		trace("Adding IE %s from Level(%s) DRLG(%s) Pass(%s) Room(%s)\n", tAdObjDef.szName, pLevelDef->pszName, pDRLGDef->pszName, pPass->pszDrlgFileName, szFilePath);
		sAddAdInventoryElementByName(pAdInventoryElements, pLevelDef, pDRLGDef, tAdObjDef.szName, TESTBIT(tAdObjDef.dwFlags, AODF_NONREPORTING_BIT));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAdInventoryElementsFromDRLGDef(
	SIMPLE_DYNAMIC_ARRAY<AD_INVENTORY_ELEMENT> & pAdInventoryElements,
	const LEVEL_DEFINITION * pLevelDef,
	const DRLG_DEFINITION * pDRLGDef)
{
	int nNumRules = ExcelGetNumRows( NULL, DATATABLE_LEVEL_RULES );
	for ( int i = 0; i < nNumRules; i++ )
	{
		DRLG_PASS * pPass = (DRLG_PASS *)ExcelGetData( NULL, DATATABLE_LEVEL_RULES, i );
		if (! pPass )
			continue;

		if (PStrCmp( pPass->pszDRLGName, pDRLGDef->pszDRLGRuleset ) != 0 ||
			pPass->nPathIndex != pDRLGDef->nPathIndex )
		{
			continue;
		}

		if(!pPass->pDrlgDef)
		{
			OVERRIDE_PATH tOverride;
			OverridePathByLine( pPass->nPathIndex, &tOverride );
			ASSERT( tOverride.szPath[ 0 ] );
			pPass->pDrlgDef = DRLGLevelDefinitionGet(tOverride.szPath, pPass->pszDrlgFileName);
		}

		if(!pPass->pDrlgDef)
		{
			continue;
		}

		// First the templates
		for(int j=0; j<pPass->pDrlgDef->nTemplateCount; j++)
		{
			for(int k=0; k<pPass->pDrlgDef->pTemplates[j].nRoomCount; k++)
			{
				DRLG_ROOM * pDRLGRoom = &pPass->pDrlgDef->pTemplates[j].pRooms[k];
				sAddAdInventoryElementsFromDRLGRoom(pAdInventoryElements, pLevelDef, pDRLGDef, pPass, pDRLGRoom);
			}
		}
		// Now the substitutions
		for(int j=0; j<pPass->pDrlgDef->nSubstitutionCount; j++)
		{
			for(int k=0; k<pPass->pDrlgDef->pSubsitutionRules[j].nRuleRoomCount; k++)
			{
				DRLG_ROOM * pDRLGRoom = &pPass->pDrlgDef->pSubsitutionRules[j].pRuleRooms[k];
				sAddAdInventoryElementsFromDRLGRoom(pAdInventoryElements, pLevelDef, pDRLGDef, pPass, pDRLGRoom);
			}

			for(int k=0; k<pPass->pDrlgDef->pSubsitutionRules[j].nReplacementCount; k++)
			{
				for(int l=0; l<pPass->pDrlgDef->pSubsitutionRules[j].pnReplacementRoomCount[k]; l++)
				{
					DRLG_ROOM * pDRLGRoom = &pPass->pDrlgDef->pSubsitutionRules[j].ppReplacementRooms[k][l];
					sAddAdInventoryElementsFromDRLGRoom(pAdInventoryElements, pLevelDef, pDRLGDef, pPass, pDRLGRoom);
				}
			}
		}

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGenerateAdReport()
{
#if !ISVERSION(SERVER_VERSION)
	START_SECTION("Generate Ad Report");

	SIMPLE_DYNAMIC_ARRAY<AD_INVENTORY_ELEMENT> pAdInventoryElements;
	int nStartCount = ExcelGetNumRows(NULL, DATATABLE_LEVEL);
	if(nStartCount <= 0)
	{
		return FALSE;
	}
	
	ArrayInit(pAdInventoryElements, NULL, nStartCount);

	// Go through the levels
	for(int i=0; i<nStartCount; i++)
	{
		const LEVEL_DEFINITION * pLevelDef = (const LEVEL_DEFINITION*)ExcelGetData(NULL, DATATABLE_LEVEL, i);
		if (! pLevelDef )
			continue;

		// Go through the sublevels
		for(int j=0; j<MAX_SUBLEVELS; j++)
		{
			if(pLevelDef->nSubLevel[j] >= 0)
			{
				const SUBLEVEL_DEFINITION * pSublevelDef = (const SUBLEVEL_DEFINITION*)ExcelGetData(NULL, DATATABLE_SUBLEVEL, pLevelDef->nSubLevel[j]);
				if ( !pSublevelDef)
					continue;

				if(pSublevelDef->nDRLGDef >= 0)
				{
					const DRLG_DEFINITION * pDRLGDef = (const DRLG_DEFINITION*)ExcelGetData(NULL, DATATABLE_LEVEL_DRLGS, pSublevelDef->nDRLGDef);
					if (! pDRLGDef)
						continue;

					sAddAdInventoryElementsFromDRLGDef(pAdInventoryElements, pLevelDef, pDRLGDef);
				}
			}
		}

		// Go through the DRLGs
		int nNumRows = ExcelGetNumRows(NULL, DATATABLE_LEVEL_DRLG_CHOICE);
		for(int j=0; j<nNumRows; j++)
		{
			const LEVEL_DRLG_CHOICE * pDRLGChoice = (const LEVEL_DRLG_CHOICE *)ExcelGetData(NULL, DATATABLE_LEVEL_DRLG_CHOICE, j);
			if(!pDRLGChoice)
				continue;

			if(pDRLGChoice->nLevel == i)
			{
				const DRLG_DEFINITION * pDRLGDef = (const DRLG_DEFINITION*)ExcelGetData(NULL, DATATABLE_LEVEL_DRLGS, pDRLGChoice->nDRLG);
				if (! pDRLGDef)
					continue;

				sAddAdInventoryElementsFromDRLGDef(pAdInventoryElements, pLevelDef, pDRLGDef);
			}
		}

		CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_BACKGROUND ) ) );
		CLT_VERSION_ONLY( V( e_TexturesRemoveAll( TEXTURE_GROUP_PARTICLE ) ) );
	}

	char pszOutputFileName[MAX_FILE_NAME_LENGTH];
	PStrPrintf(pszOutputFileName, MAX_FILE_NAME_LENGTH, "AdReport.tsv");
	HANDLE hFile = FileOpen(pszOutputFileName, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	ASSERT_RETFALSE(hFile != INVALID_HANDLE_VALUE);
	int nInventoryElementCount = pAdInventoryElements.Count();
	for(int i=0; i<nInventoryElementCount; i++)
	{
		const int MAX_MESSAGE_LENGTH = 256;
		char pszMessage[MAX_MESSAGE_LENGTH];
		int nCount = PStrPrintf(pszMessage, MAX_MESSAGE_LENGTH, "%s\t%s\t%s\t%d\t%d\t%s\t%s\t%s\t%s\r\n", 
			pAdInventoryElements[i].pszLevelName, 
			pAdInventoryElements[i].pszZoneName, 
			pAdInventoryElements[i].pszElementName, 
			pAdInventoryElements[i].nTextureWidth, 
			pAdInventoryElements[i].nTextureHeight, 
			pAdInventoryElements[i].pszDescription, 
			pAdInventoryElements[i].pszRotation, 
			pAdInventoryElements[i].pszTextureFormat, 
			pAdInventoryElements[i].pszOtherNotes
			);
		if(!pAdInventoryElements[i].bAnyReporting)
		{
			trace("NOT Adding: ");
		}
		trace(pszMessage);
		if(pAdInventoryElements[i].bAnyReporting)
		{
			FileWrite(hFile, pszMessage, nCount);
		}
	}
	FileClose(hFile);

	pAdInventoryElements.Destroy();

	END_SECTION();
#endif
	return TRUE;
}

#endif
//#else // !ISVERSION(SERVER_VERSION)

///////////////////////////////////////////////////////////////////////////////////
// declare the commands ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
#include "FillPakServerMsg.h"

///////////////////////////////////////////////////////////////////////////////////
// define the handlers (client-only) //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
#if !ISVERSION(SERVER_VERSION)

static BOOL sFillPakClt_LoadUI(
	MSG_STRUCT* msg )
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	if (UIInit( FALSE, TRUE ) == FALSE)
	{
		return FALSE;
	}


	return TRUE;
}

static BOOL sFillPakClt_LoadMasterDatabaseFile(
	MSG_STRUCT* msg )
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	return (e_CompatDB_LoadMasterDatabaseFile() != NULL);
}

// Implemented in dx9_effect.cpp
void sEffectFillpakCallback(LPCSTR filename);
static BOOL sFillPakClt_LoadEffect(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	sEffectFillpakCallback(pMsg->strFileName);
	
	return TRUE;
}

// Implemented in demolevel.cpp
void DemoLevelLoadDefinition(LPCSTR filename);
static BOOL sFillPakClt_LoadDemoLevel(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	DemoLevelLoadDefinition(pMsg->strFileName);

	return TRUE;
}

// Implemented in wardrobe.cpp
BOOL c_WardrobeLoadLayer(int i);
static BOOL sFillPakClt_LoadWardrobeLayer(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	return c_WardrobeLoadLayer(pMsg->index1);
}

// Implemented in wardrobe.cpp
BOOL c_WardrobeLoadModel(int i);
static BOOL sFillPakClt_LoadWardrobeModel(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	c_WardrobeLoadModel(pMsg->index1);
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	return TRUE;
}

// Implemented in wardrobe.cpp
BOOL c_WardrobeLoadBlendOp(int j, int i);
static BOOL sFillPakClt_LoadWardrobeBlendOp(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	c_WardrobeLoadBlendOp(pMsg->index1, pMsg->index2);
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	return TRUE;
}

// Implemented in wardrobe.cpp
BOOL c_WardrobeLoadTexture(int i, int k);
static BOOL sFillPakClt_LoadWardrobeTexture(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	c_WardrobeLoadTexture(pMsg->index1, pMsg->index2);
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	return TRUE;
}

static BOOL sFillPakClt_LoadUnit(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	int i = pMsg->index1;
	int j = pMsg->index2;
	GAME* pGame = AppGetCltGame();
	ASSERT_RETFALSE(i < NUM_GENUS);

	EXCELTABLE eTable = UnitGenusGetDatatableEnum((GENUS) i);

	if (eTable < 0 || eTable >= NUM_DATATABLES) {
		return TRUE;
	}

	int nCount = ExcelGetNumRows( pGame, eTable );
	ASSERT_RETFALSE(j < nCount);

	UNIT_DATA * pUnitData = (UNIT_DATA *) ExcelGetData( pGame, eTable, j );
	
	if (pUnitData != NULL && !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IGNORE_IN_DAT)) 
	{
		if ( ! FillPakShouldLoad( pUnitData->szAppearance ) )
			return TRUE;

		if ( AppCommonIsFillpakInConvertMode() ) 
		{
			AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
			AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
		}

		UnitDataLoad(pGame, eTable, j);

		if ( AppCommonIsFillpakInConvertMode() ) 
		{
			AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
			AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
		}

		if(AppIsTugboat()) {
			// We need to load any icon textures this unit might use too, to throw
			// them into the PAK
			if (PStrLen( pUnitData->szIcon )  != 0) {
				char szFullName[MAX_PATH];
				PStrPrintf( szFullName, MAX_PATH, "%s%s%s", 
					FILE_PATH_ICONS,
					pUnitData->szIcon,
					".png" );
				UIX_TEXTURE* texture = (UIX_TEXTURE*)StrDictionaryFind( g_UI.m_pTextures, szFullName);
				if (!texture) {
					// GS - 10-2-2006
					// Uncomment this once Brennan finishes merging the UI bits
					// Travis - OK! 11-7-2006
					texture = UITextureLoadRaw( szFullName );
					//texture = NULL;
					if (texture) {
						StrDictionaryAdd( g_UI.m_pTextures, szFullName, texture);
					}
				}
			}
		}
	}
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	{
		static UINT32 x = 0;
		if (x % 25 == 0) {
			V( e_ModelsDestroy() );
			V( e_ModelsInit() );
			V( e_TexturesRemoveAll(TEXTURE_GROUP_UNITS));
			V( e_TexturesRemoveAll(TEXTURE_GROUP_WARDROBE));
			V( e_TexturesRemoveAll(TEXTURE_GROUP_PARTICLE));
		}
		x++;
	}
	return TRUE;
}


static BOOL sFillPakClt_LoadRoom(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);
	int i = pMsg->index1;

	GAME* pGame = AppGetCltGame();
	RoomDefinitionGet( pGame, i, TRUE, 0.0f ); // this loads the graphics

	// make sure that everything loads
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	ROOM_INDEX * pRoomIndex = (ROOM_INDEX *) ExcelGetData( pGame, DATATABLE_ROOM_INDEX, i );
	if ( ! pRoomIndex )
		return TRUE;
	if (pRoomIndex->pRoomDefinition) {
		// do a little bit of clean up so that we don't run out of memory
		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ ) {
			e_ModelDefinitionDestroy( pRoomIndex->pRoomDefinition->nModelDefinitionId, 
				nLOD, MODEL_DEFINITION_DESTROY_ALL, TRUE );
		}
		pRoomIndex->pRoomDefinition->nModelDefinitionId = INVALID_ID;

		c_SoundMemoryLoadReverbDefinitionByFile(pRoomIndex->pszReverbFile);
	}

	if ( ! FillPakShouldLoad( pRoomIndex->pszFile ) )
		return TRUE;

	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pRoomIndex->pszFile, ROOM_LAYOUT_SUFFIX );
	ROOM_LAYOUT_GROUP_DEFINITION * pLayoutDef = (ROOM_LAYOUT_GROUP_DEFINITION*)DefinitionGetByName( DEFINITION_GROUP_ROOM_LAYOUT, szFilePath );

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
	}

	sPreLoadRoomLayout(pGame, &pLayoutDef->tGroup);

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
	}

	PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pRoomIndex->pszFile, ROOM_PATH_NODE_SUFFIX );
	DefinitionGetByName( DEFINITION_GROUP_ROOM_PATH_NODE, szFilePath );

	// Load any path mask files
	if( pRoomIndex->pszOverrideNodes && strlen( pRoomIndex->pszOverrideNodes ) > 0 )
	{
		PStrPrintf(szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s",FILE_PATH_BACKGROUND, pRoomIndex->pszFile, PATH_MASK_FILE_EXTENSION);
		ROOM_PATH_MASK_DEFINITION *pMaskDef = RoomPathMaskDefinitionGet( szFilePath );
		if( pMaskDef )
			FREE(NULL, pMaskDef->pbyFileStart);
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	{
		static UINT32 x = 0;
		if (x % 10 == 0) {
			V( e_TexturesRemoveAll(TEXTURE_GROUP_BACKGROUND));
			V( e_TexturesRemoveAll(TEXTURE_GROUP_PARTICLE));
		}
		x++;
	}

	return TRUE;
}

static BOOL sFillPakClt_LoadDRLG(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);
	int i = pMsg->index1;

	GAME* pGame = AppGetCltGame();
	DRLG_PASS* def = (DRLG_PASS*)ExcelGetData(pGame, DATATABLE_LEVEL_RULES, i);
	if ( ! def )
		return TRUE;
		
	OVERRIDE_PATH tOverride;
	OverridePathByLine( def->nPathIndex, &tOverride );
	ASSERT( tOverride.szPath[ 0 ] );
	def->pDrlgDef = DRLGLevelDefinitionGet(tOverride.szPath, def->pszDrlgFileName);

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	{
		static UINT32 x = 0;
		if (x % 10 == 0) {
			V( e_TexturesRemoveAll(TEXTURE_GROUP_BACKGROUND));
			V( e_TexturesRemoveAll(TEXTURE_GROUP_PARTICLE));
		}
		x++;
	}

	return TRUE;
}

static BOOL sFillPakClt_LoadDRLGTemplate(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;
	int j = pMsg->index2;
	int k = pMsg->index3;
	int nRuleCount = ExcelGetNumRows( pGame, DATATABLE_LEVEL_RULES );

	ASSERT_RETFALSE(i < nRuleCount);
	DRLG_PASS* def = (DRLG_PASS*)ExcelGetData(pGame, DATATABLE_LEVEL_RULES, i);
	if ( ! def )
		return FALSE;
	//ASSERTV_RETFALSE(def, "Table index %d has no DRLG Pass!", i);
	if (def->pDrlgDef == NULL) {
		// in a multi-client fillpak, the drlg def might have been loaded by another client.
		// Try downloading it from the server first.
		OVERRIDE_PATH tOverride;
//		char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		OverridePathByLine( def->nPathIndex, &tOverride );
		ASSERT( tOverride.szPath[ 0 ] );
		def->pDrlgDef = DRLGLevelDefinitionGet(tOverride.szPath, def->pszDrlgFileName);
	}
	ASSERTV_RETFALSE(def->pDrlgDef, "Table index %d (%s) has no DRLG definition.", i, def->pszDrlgFileName);
	ASSERT_RETFALSE(j < def->pDrlgDef->nTemplateCount);
	ASSERT_RETFALSE(k < def->pDrlgDef->pTemplates[j].nRoomCount);
	
	DRLG_ROOM * pDRLGRoom = &def->pDrlgDef->pTemplates[j].pRooms[k];
	sPreloadDRLGRoomLayout(pGame, pDRLGRoom, def);

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	{
		static UINT32 x = 0;
		if (x % 10 == 0) {
			V( e_TexturesRemoveAll(TEXTURE_GROUP_BACKGROUND));
			V( e_TexturesRemoveAll(TEXTURE_GROUP_PARTICLE));
		}
		x++;
	}

	return TRUE;
}

static BOOL sFillPakClt_LoadDRLGSubstitutionRule(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;
	int j = pMsg->index2;
	int k = pMsg->index3;
	int nRuleCount = ExcelGetNumRows( pGame, DATATABLE_LEVEL_RULES );
	ASSERT_RETFALSE(i < nRuleCount);

	DRLG_PASS* def = (DRLG_PASS*)ExcelGetData(pGame, DATATABLE_LEVEL_RULES, i);
	if ( ! def )
		return FALSE;
	//ASSERTV_RETFALSE(def, "Table index %d has no DRLG Pass!", i);
	if (def->pDrlgDef == NULL) {
		// in a multi-client fillpak, the drlg def might have been loaded by another client.
		// Try downloading it from the server first.
		OVERRIDE_PATH tOverride;
		//		char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		OverridePathByLine( def->nPathIndex, &tOverride );
		ASSERT( tOverride.szPath[ 0 ] );
		def->pDrlgDef = DRLGLevelDefinitionGet(tOverride.szPath, def->pszDrlgFileName);
	}
	ASSERTV_RETFALSE(def->pDrlgDef, "Table index %d (%s) has no DRLG definition.", i, def->pszDrlgFileName);
	ASSERT_RETFALSE(j < def->pDrlgDef->nSubstitutionCount);
	ASSERT_RETFALSE(k < def->pDrlgDef->pSubsitutionRules[j].nRuleRoomCount);

	DRLG_ROOM * pDRLGRoom = &def->pDrlgDef->pSubsitutionRules[j].pRuleRooms[k];
	sPreloadDRLGRoomLayout(pGame, pDRLGRoom, def);

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	{
		static UINT32 x = 0;
		if (x % 10 == 0) {
			V( e_TexturesRemoveAll(TEXTURE_GROUP_BACKGROUND));
			V( e_TexturesRemoveAll(TEXTURE_GROUP_PARTICLE));
		}
		x++;
	}

	return TRUE;
}

static BOOL sFillPakClt_LoadDRLGSubstitutionReplacement(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;
	int j = pMsg->index2;
	int k = pMsg->index3;
	int l = pMsg->index4;
	int nRuleCount = ExcelGetNumRows( pGame, DATATABLE_LEVEL_RULES );
	ASSERT_RETFALSE(i < nRuleCount);

	DRLG_PASS* def = (DRLG_PASS*)ExcelGetData(pGame, DATATABLE_LEVEL_RULES, i);
	if ( ! def )
		return FALSE;
	//ASSERTV_RETFALSE(def, "Table index %d has no DRLG Pass!", i);
	if (def->pDrlgDef == NULL) {
		// in a multi-client fillpak, the drlg def might have been loaded by another client.
		// Try downloading it from the server first.
		OVERRIDE_PATH tOverride;
		//		char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		OverridePathByLine( def->nPathIndex, &tOverride );
		ASSERT( tOverride.szPath[ 0 ] );
		def->pDrlgDef = DRLGLevelDefinitionGet(tOverride.szPath, def->pszDrlgFileName);
	}
	ASSERTV_RETFALSE(def->pDrlgDef, "Table index %d (%s) has no DRLG definition.", i, def->pszDrlgFileName);
	ASSERT_RETFALSE(j < def->pDrlgDef->nSubstitutionCount);
	ASSERT_RETFALSE(k < def->pDrlgDef->pSubsitutionRules[j].nReplacementCount);
	ASSERT_RETFALSE(l < def->pDrlgDef->pSubsitutionRules[j].pnReplacementRoomCount[k]);

	DRLG_ROOM * pDRLGRoom = &def->pDrlgDef->pSubsitutionRules[j].ppReplacementRooms[k][l];
	sPreloadDRLGRoomLayout(pGame, pDRLGRoom, def);
	
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	{
		static UINT32 x = 0;
		if (x % 10 == 0) {
			V( e_TexturesRemoveAll(TEXTURE_GROUP_BACKGROUND));
			V( e_TexturesRemoveAll(TEXTURE_GROUP_PARTICLE));
		}
		x++;
	}

	return TRUE;
}

static BOOL sFillPakClt_LoadEnvironment(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int mode = pMsg->index1;
	int i = pMsg->index2;
	int j = pMsg->index3;
	int k = pMsg->index4;

	int nDRLGCount = ExcelGetNumRows(pGame, DATATABLE_LEVEL_DRLGS);
	int nThemeCount = ExcelGetNumRows(pGame, DATATABLE_LEVEL_THEMES);
	LEVEL_ENVIRONMENT_DEFINITION * env_def = NULL;
	DRLG_DEFINITION * def = NULL;

	if ( mode == 4 )
	{
		env_def = ( LEVEL_ENVIRONMENT_DEFINITION * )ExcelGetData(pGame, DATATABLE_LEVEL_ENVIRONMENTS, i);
	}
	else
	{
		ASSERT_RETFALSE(i < nDRLGCount);

		def = (DRLG_DEFINITION*)ExcelGetData(pGame, DATATABLE_LEVEL_DRLGS, i);
		ASSERTV_RETFALSE(def != NULL, "Error loading level DRLG index %i! -- %s", i, pMsg->strFileName );
		ASSERTV_RETFALSE( def->nEnvironment != INVALID_LINK, "Error! No environment for level DRLG index %i! -- %s", i, pMsg->strFileName );
		env_def = ( LEVEL_ENVIRONMENT_DEFINITION * )ExcelGetData(pGame, DATATABLE_LEVEL_ENVIRONMENTS, def->nEnvironment);
	}

	ASSERT_RETFALSE( env_def != NULL );

	OVERRIDE_PATH tOverride;
	char szPath    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	OverridePathByLine( env_def->nPathIndex, &tOverride );
	ASSERT_RETFALSE( tOverride.szPath[ 0 ] );
	PStrRemovePath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, FILE_PATH_BACKGROUND, tOverride.szPath );

	switch (mode) {
		case 0:
		case 4:
			sFillPakLoadEnvironmentFile(szPath, env_def->pszEnvironmentFile, "");
			break;

		case 1:
			{
				ASSERT_RETFALSE(j < nThemeCount);
				ASSERT_RETFALSE(k < MAX_LEVEL_THEME_ALLOWED_STYLES);

				const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, j);
				ASSERT_RETFALSE(pLevelTheme);
				if(!pLevelTheme->pszEnvironment || !pLevelTheme->pszEnvironment[0]) {
					return TRUE;
				}

				if(pLevelTheme->nAllowedStyles[k] == def->nStyle) {
					sFillPakLoadEnvironmentFile(szPath, env_def->pszEnvironmentFile, pLevelTheme->pszEnvironment);
				}
			}
			break;

		case 2:
			ASSERT_RETFALSE(j < MAX_DRLG_THEMES);
			if (def->nThemes[j] >= 0) {
				ASSERT_RETFALSE(def->nThemes[j] < nThemeCount);
				const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, def->nThemes[j]);
				ASSERT_RETFALSE(pLevelTheme);
				if(!pLevelTheme->pszEnvironment || !pLevelTheme->pszEnvironment[0]) {
					return TRUE;
				}
				sFillPakLoadEnvironmentFile(szPath, env_def->pszEnvironmentFile, pLevelTheme->pszEnvironment);
			}
			break;

		case 3:
			ASSERT_RETFALSE(j < MAX_WEATHER_SETS);
			ASSERT_RETFALSE(k < MAX_THEMES_FROM_WEATHER);

			if (def->nWeatherSet >= 0) {
				const WEATHER_SET_DATA * pWeatherSet = (const WEATHER_SET_DATA *)ExcelGetData(pGame, DATATABLE_WEATHER_SETS, def->nWeatherSet);
				if (pWeatherSet) {
					if(pWeatherSet->pWeatherSets[j].nWeather >= 0) {
						const WEATHER_DATA * pWeatherData = (const WEATHER_DATA *)ExcelGetData(pGame, DATATABLE_WEATHER, pWeatherSet->pWeatherSets[j].nWeather);
						if (pWeatherData) {
							if(pWeatherData->nThemes[k] >= 0) {
								const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, pWeatherData->nThemes[k]);
								ASSERT_RETFALSE(pLevelTheme);
								if(!pLevelTheme->pszEnvironment || !pLevelTheme->pszEnvironment[0]) {
									return TRUE;
								}
								sFillPakLoadEnvironmentFile(szPath, env_def->pszEnvironmentFile, pLevelTheme->pszEnvironment);
							}
						}
					}
				}
			}
			break;

		default:
			ASSERTX_RETFALSE(FALSE, "Environment load option not supported");
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	{
		static UINT32 x = 0;
		if (x % 10 == 0) {
			V( e_TexturesRemoveAll(TEXTURE_GROUP_BACKGROUND));
			V( e_TexturesRemoveAll(TEXTURE_GROUP_PARTICLE));
		}
		x++;
	}

	return TRUE;
}

static BOOL sFillPakClt_LoadGlobalMaterial(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);
	int i = pMsg->index1;

	// load materials into global slots
	int nRows = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_MATERIALS_GLOBAL );
	ASSERT_RETFALSE(i < nRows);

	GLOBAL_MATERIAL_DEFINITION * pDef = (GLOBAL_MATERIAL_DEFINITION *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_MATERIALS_GLOBAL, i );
	ASSERT_RETFALSE(pDef);
	if ( pDef->nID != INVALID_ID ) 
	{
		MATERIAL * pMaterial = (MATERIAL*)DefinitionGetById( DEFINITION_GROUP_MATERIAL, pDef->nID );
		if (pMaterial) 
		{
			V( e_MaterialRestore( pDef->nID ) );
			return TRUE;
		}
	}
	V( e_MaterialNew( &pDef->nID, pDef->szMaterial ) );
	V( e_MaterialRestore( pDef->nID ) );
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	return TRUE;
}

static BOOL sFillPakClt_LoadState(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;

	int nStates = ExcelGetNumRows( pGame, DATATABLE_STATES );
	ASSERT_RETFALSE(i < nStates);

	c_StateLoad( pGame, i );

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	return TRUE;
}

// Implemented in states.cpp
void c_StateLightingDataEngineLoad();
static BOOL sFillPakClt_LoadStateLighting(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	c_StateLightingDataEngineLoad();
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	return TRUE;
}

static BOOL sFillPakClt_LoadSkill(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;
	int nSkillCount = ExcelGetNumRows( pGame, DATATABLE_SKILLS );
	ASSERT_RETFALSE(i < nSkillCount);

	c_SkillLoad(pGame, i);

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	return TRUE;
}

static BOOL sFillPakClt_LoadMeleeWeapon(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int nRow = pMsg->index1;
	int i = pMsg->index2;
	int j = pMsg->index3;

	int nCount = ExcelGetNumRows( pGame, DATATABLE_MELEEWEAPONS );
	ASSERT_RETFALSE(nRow < nCount);

	MELEE_WEAPON_DATA * pDef = (MELEE_WEAPON_DATA *)ExcelGetData(pGame, DATATABLE_MELEEWEAPONS, nRow);
	ASSERT_RETFALSE(pDef != NULL);
	if ( i == -1 && j == -1 )
	{
		for (int ii = 0; ii < NUM_COMBAT_RESULTS; ii++)
		{
			c_ParticleEffectSetLoad( &pDef->pEffects[ii], pDef->szEffectFolder );
		}
	}
	else if (pDef->szSwingEvents[0] != 0) 
	{
		int nSwingEventsId = DefinitionGetIdByName( DEFINITION_GROUP_SKILL_EVENTS, pDef->szSwingEvents );
		SKILL_EVENTS_DEFINITION* pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, nSwingEventsId );
		ASSERT_RETFALSE(pEvents != NULL);
		ASSERT_RETFALSE(i < pEvents->nEventHolderCount);

		SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ i ];
		ASSERT_RETFALSE(j < pHolder->nEventCount);

		SKILL_EVENT * pSkillEvent = &pHolder->pEvents[ j ];
		const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = SkillGetEventTypeData( pGame, pSkillEvent->nType );
		ASSERT_RETFALSE(pSkillEventTypeData);
		//Tugboat only had missiles but Looking at source control I'm guessing this just hasn't
		//been merged yet. So leaving it in.
		if (pSkillEventTypeData->eAttachedTable == DATATABLE_MISSILES ||
			pSkillEventTypeData->eAttachedTable == DATATABLE_MONSTERS ||
			pSkillEventTypeData->eAttachedTable == DATATABLE_ITEMS ||
			pSkillEventTypeData->eAttachedTable == DATATABLE_OBJECTS) {
			int nUnitClass = pSkillEvent->tAttachmentDef.nAttachedDefId;
			if ( nUnitClass != INVALID_ID) {
				UnitDataLoad( pGame, pSkillEventTypeData->eAttachedTable, nUnitClass );
			}
		}
		if ( pSkillEventTypeData->bUsesAttachment) {
			ASSERT( pSkillEventTypeData->eAttachedTable == DATATABLE_NONE );
			c_AttachmentDefinitionLoad( pGame, pSkillEvent->tAttachmentDef, INVALID_ID, "" );
		} 
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	return TRUE;
}

static BOOL sFillPakClt_LoadAI(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int ii = pMsg->index1;
	int nAICount = ExcelGetNumRows(pGame, DATATABLE_AI_INIT);
	ASSERT_RETFALSE(ii < nAICount);

	AI_INIT * ai_def = (AI_INIT *)ExcelGetData(pGame, DATATABLE_AI_INIT, ii);
	if (ai_def == NULL) {
		return TRUE;
	}
	ASSERT_RETFALSE(ai_def);
	if (ai_def->pszTableName[0] != 0) {
		ai_def->nTable = DefinitionGetIdByName(DEFINITION_GROUP_AI, ai_def->pszTableName);
	} else {
		ai_def->nTable = INVALID_ID;
	}
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	return TRUE;
}

static BOOL sFillPakClt_DoLoadSound(
	GAME * pGame,
	int i,
	int nSKU,
	BOOL bIsLocalized)
{
	c_BGSoundsStop();
	c_SoundStopAll();
	c_MusicStop();

	// load the specified sound
	int nSoundCount = ExcelGetNumRows( pGame, DATATABLE_SOUNDS );
	ASSERT_RETFALSE(i < nSoundCount);
	if(bIsLocalized)
	{
		SOUND_LOAD_EXINFO tLoadExInfo;
		structclear(tLoadExInfo);
		tLoadExInfo.nSKU = nSKU;
		c_SoundLoad(i, SLT_FILLPAK, &tLoadExInfo);
	}
	else
	{
		c_SoundLoad(i, SLT_FILLPAK);
	}
	return TRUE;
}

static BOOL sFillPakClt_LoadSound(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;

	return sFillPakClt_DoLoadSound(pGame, i, INVALID_ID, FALSE);
}

static BOOL sFillPakClt_LoadSoundLocalized(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;
	int nSKU = pMsg->index2;

	return sFillPakClt_DoLoadSound(pGame, i, nSKU, TRUE);
}

static BOOL sFillPakClt_LoadMovieLocalized(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;
	int nSKU = pMsg->index2;

	const MOVIE_DATA * pMovieData = (const MOVIE_DATA *)ExcelGetData(pGame, DATATABLE_MOVIES, i);
	if(!pMovieData)
		return FALSE;

	BOOL bAddMovie = FALSE;
	for(int j=0; j<MAX_REGIONS_PER_MOVIE; j++)
	{
		WORLD_REGION eRegion = pMovieData->eRegions[ j ];
		if (eRegion < 0)
		{
			break;
		}

		if (SKUContainsRegion( nSKU, eRegion ))
		{
			bAddMovie = TRUE;
			break;
		}
	}
	if(!bAddMovie)
		return FALSE;

	PAK_ENUM ePakHigh;
	PAK_ENUM ePakLow;
	if(pMovieData->bPutInMainPak)
	{
		ePakHigh = PAK_MOVIES;
		ePakLow = PAK_MOVIES;
	}
	else
	{
		ePakHigh = PAK_MOVIES_HIGH;
		ePakLow = PAK_MOVIES_LOW;
	}

	BOOL bSKUCensorsMovies = SKUCensorsMovies(nSKU);
	if(bSKUCensorsMovies && pMovieData->bDisallowInCensoredSKU)
	{
		return FALSE;
	}
	if(!bSKUCensorsMovies && pMovieData->bOnlyWithCensoredSKU)
	{
		return FALSE;
	}

	// Do the high-res movie first:
	if(SKUAllowsHQMovies(nSKU) || pMovieData->bForceAllowHQ)
	{
		trace("Adding HQ '%s'\n", pMovieData->pszName);
		BYTE buffer[sizeof(FILE_HEADER)];

		DECLARE_LOAD_SPEC(spec, pMovieData->pszFileName);
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_ADD_TO_PAK_IMMEDIATE | PAKFILE_LOAD_IGNORE_BUFFER_SIZE | PAKFILE_LOAD_ADD_TO_PAK_NO_COMPRESS | PAKFILE_LOAD_GIANT_FILE;
		spec.buffer = buffer;
		spec.size = arrsize(buffer);
		spec.priority = ASYNC_PRIORITY_SOUNDS;
		spec.pakEnum = ePakHigh;

		PakFileLoadNow(spec);
	}

	// Then do the low-res movie if it exists:
	if(pMovieData->pszFileNameLowres && pMovieData->pszFileNameLowres[0])
	{
		trace("Adding LQ '%s'\n", pMovieData->pszName);
		BYTE buffer[sizeof(FILE_HEADER)];

		DECLARE_LOAD_SPEC(spec, pMovieData->pszFileNameLowres);
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_ADD_TO_PAK_IMMEDIATE | PAKFILE_LOAD_IGNORE_BUFFER_SIZE | PAKFILE_LOAD_ADD_TO_PAK_NO_COMPRESS | PAKFILE_LOAD_GIANT_FILE;
		spec.buffer = buffer;
		spec.size = arrsize(buffer);
		spec.priority = ASYNC_PRIORITY_SOUNDS;
		spec.pakEnum = ePakLow;

		PakFileLoadNow(spec);
	}

	return TRUE;
}

static BOOL sFillPakClt_LoadSoundBus(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;

	// load the specified sound bus
	int nSoundBusCount = ExcelGetNumRows( pGame, DATATABLE_SOUNDBUSES );
	ASSERT_RETFALSE(i < nSoundBusCount);
	SOUND_BUS * pSoundBus = (SOUND_BUS *)ExcelGetData( pGame, DATATABLE_SOUNDBUSES, i );
	if(pSoundBus->pszEffectDefinitionName)
	{
		c_SoundMemoryLoadEffectFileByName(pSoundBus->pszEffectDefinitionName);
	}
	return TRUE;
}

static BOOL sFillPakClt_LoadSoundMixState(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;

	// load the specified sound bus
	int nSoundMixStateCount = ExcelGetNumRows( pGame, DATATABLE_SOUND_MIXSTATES );
	ASSERT_RETFALSE(i < nSoundMixStateCount);
	SOUND_MIXSTATE * pSoundMixState = (SOUND_MIXSTATE *)ExcelGetData( pGame, DATATABLE_SOUND_MIXSTATES, i );
	if(pSoundMixState->pszReverbFileName)
	{
		c_SoundMemoryLoadReverbDefinitionByFile(pSoundMixState->pszReverbFileName);
	}
	return TRUE;
}

static BOOL sFillPakClt_LoadSoundMixStateValue(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME* pGame = AppGetCltGame();
	int i = pMsg->index1;

	// load the specified sound bus
	int nSoundMixStateValueCount = ExcelGetNumRows( pGame, DATATABLE_SOUND_MIXSTATE_VALUES );
	ASSERT_RETFALSE(i < nSoundMixStateValueCount);
	SOUND_MIXSTATE_VALUE * pSoundMixStateValue = (SOUND_MIXSTATE_VALUE *)ExcelGetData( pGame, DATATABLE_SOUND_MIXSTATE_VALUES, i );
	if(pSoundMixStateValue->pszEffectDefinitionName)
	{
		c_SoundMemoryLoadEffectFileByName(pSoundMixStateValue->pszEffectDefinitionName);
	}
	return TRUE;
}

static BOOL sFillPakClt_LoadSpecialFile(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	TCHAR filename[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, _T("%s%s"), FILE_PATH_DATA_COMMON, _T("monitor-DO_NOT_REMOVE.txt"));

	DECLARE_LOAD_SPEC(spec, filename);
	spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK | PAKFILE_LOAD_ADD_TO_PAK;
	void* pBuffer = PakFileLoadNow(spec);

	if (pBuffer != NULL) {
		FREE(NULL, pBuffer);
		return TRUE;
	} else {
		return FALSE;
	}
}

static BOOL sFillPakClt_LoadLevelLocalized(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	GAME *pGame = AppGetCltGame();
	int nFolder = pMsg->index1;
	int nSKU = pMsg->index2;

	return c_LevelLoadLocalizedTextures( pGame, nFolder, nSKU );
}

static BOOL sFillPakClt_LoadEulaText(
	LPCTSTR filename)
{
	DECLARE_LOAD_SPEC(spec, filename);
	spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK | PAKFILE_LOAD_ADD_TO_PAK;
	spec.pakEnum = PAK_LOCALIZED;
	void* pBuffer = PakFileLoadNow(spec);

	if (pBuffer != NULL) {
		FREE(NULL, pBuffer);
		return TRUE;
	} else {
		return FALSE;
	}
}

static BOOL sFillPakClt_LoadEulaLocalized(
	MSG_STRUCT* msg)
{
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	TCHAR filename[DEFAULT_FILE_WITH_PATH_SIZE];
	int nSKU = pMsg->index1;
	ASSERTX(INVALID_LINK != nSKU, "Expected current SKU");

	const SKU_DATA* pSkuDataCurrent = SKUGetData(nSKU);
	ASSERT_RETFALSE(pSkuDataCurrent != NULL);

	if (AppIsHellgate()) 
	{
		int nNumSkus = ExcelGetNumRows(NULL, DATATABLE_SKU);
		int nNumLangs = LanguageGetNumLanguages( AppGameGet() );

		for (int nSkuIndex = 0; nSkuIndex < nNumSkus; nSkuIndex++) {
			if (pSkuDataCurrent->bServer || nSkuIndex == nSKU) {
				const SKU_DATA* pSkuData = SKUGetData(nSkuIndex);
				ASSERT_CONTINUE(pSkuData != NULL);

				for (int i = 0; i < nNumLangs; i++) {
					LANGUAGE eLanguage = (LANGUAGE)i;
					if (SKUContainsLanguage(nSkuIndex, eLanguage, FALSE)) {
						LPCSTR pszLanguageName = LanguageGetName(AppGameGet(), eLanguage);
						ASSERT_CONTINUE(pszLanguageName != NULL);

						PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, "%sdocs\\%s\\HG_EULA_%s.txt", FILE_PATH_DATA, pSkuData->szName, pszLanguageName);
						ASSERTV(sFillPakClt_LoadEulaText(filename), "Failed to load %s", filename);

						PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, "%sdocs\\%s\\HG_TOS_%s.txt", FILE_PATH_DATA, pSkuData->szName, pszLanguageName);
						ASSERTV(sFillPakClt_LoadEulaText(filename), "Failed to load %s", filename);
					}
				}
			}
		}
	} else if (AppIsTugboat()) {
		PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, "eula.txt");
		ASSERT_RETFALSE(sFillPakClt_LoadEulaText(filename));

		PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, "tos.txt");
		ASSERT_RETFALSE(sFillPakClt_LoadEulaText(filename));
	}

	return TRUE;
}



static BOOL sFillPakClt_EngineCleanUp(
	MSG_STRUCT* msg)
{
	UNREFERENCED_PARAMETER(msg);
	sFillPakEngineCleanup();
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////
// assign handlers for the server-to-client commands (client-only) ////////////////
///////////////////////////////////////////////////////////////////////////////////
struct {
	FILLPAK_SERVER_RESPONSE_COMMANDS iCmd;
	BOOL (*handler)(MSG_STRUCT* pMsg);
	BOOL bAllowFilter;
} FILLPAK_CMD_HANDLER_ARRAY[] =
{
	{	FPC_LOAD_UI,								sFillPakClt_LoadUI,								FALSE	},
	{	FPC_LOAD_MASTER_DB,							sFillPakClt_LoadMasterDatabaseFile,				FALSE	},
	{	FPC_LOAD_EFFECT,							sFillPakClt_LoadEffect,							FALSE	},
	{	FPC_LOAD_WARDROBE_LAYER,					sFillPakClt_LoadWardrobeLayer,					FALSE	},
	{	FPC_LOAD_WARDROBE_MODEL,					sFillPakClt_LoadWardrobeModel,					TRUE	},
	{	FPC_LOAD_WARDROBE_BLENDOP,					sFillPakClt_LoadWardrobeBlendOp,				TRUE	},
	{	FPC_LOAD_WARDROBE_TEXTURE,					sFillPakClt_LoadWardrobeTexture,				TRUE	},
	{	FPC_LOAD_UNIT,								sFillPakClt_LoadUnit,							TRUE	},
	{	FPC_LOAD_ROOM,								sFillPakClt_LoadRoom,							TRUE	},
	{	FPC_LOAD_DRLG,								sFillPakClt_LoadDRLG,							TRUE	},
	{	FPC_LOAD_DRLG_TEMPLATE,						sFillPakClt_LoadDRLGTemplate,					FALSE	},
	{	FPC_LOAD_DRLG_SUBSTITUTION_RULE,			sFillPakClt_LoadDRLGSubstitutionRule,			FALSE	},
	{	FPC_LOAD_DRLG_SUBSTITUTION_REPLACEMENT,		sFillPakClt_LoadDRLGSubstitutionReplacement,	FALSE	},
	{	FPC_LOAD_ENVIRONMENT,						sFillPakClt_LoadEnvironment,					FALSE	},
	{	FPC_LOAD_GLOBAL_MATERIAL,					sFillPakClt_LoadGlobalMaterial,					FALSE	},
	{	FPC_LOAD_STATE,								sFillPakClt_LoadState,							FALSE	},
	{	FPC_LOAD_STATE_LIGHTING,					sFillPakClt_LoadStateLighting,					FALSE	},
	{	FPC_LOAD_SKILL,								sFillPakClt_LoadSkill,							FALSE	},
	{	FPC_LOAD_MELEE_WEAPON,						sFillPakClt_LoadMeleeWeapon,					FALSE	},
	{	FPC_LOAD_AI,								sFillPakClt_LoadAI,								FALSE	},
	{	FPC_LOAD_SOUND,								sFillPakClt_LoadSound,							FALSE	},
	{	FPC_LOAD_SOUNDBUS,							sFillPakClt_LoadSoundBus,						FALSE	},
	{	FPC_LOAD_SOUNDMIXSTATE,						sFillPakClt_LoadSoundMixState,					FALSE	},
	{	FPC_LOAD_SOUNDMIXSTATEVALUE,				sFillPakClt_LoadSoundMixStateValue,				FALSE	},
	{	FPC_LOAD_CLEANUP,							sFillPakClt_EngineCleanUp,						FALSE	},
	{	FPC_LOAD_FINISHED,							NULL,											FALSE	},
	{	FPC_FILE_INFO_RESPONSE,						NULL,											FALSE	},
	{	FPC_LOAD_SOUND_LOCALIZED,					sFillPakClt_LoadSoundLocalized,					FALSE	},
	{   FPC_LOAD_LEVEL_LOCALIZED,					sFillPakClt_LoadLevelLocalized,					FALSE	},
	{   FPC_LOAD_MOVIE_LOCALIZED,					sFillPakClt_LoadMovieLocalized,					FALSE	},
	{   FPC_LOAD_EULA_LOCALIZED,					sFillPakClt_LoadEulaLocalized,					FALSE	},
	{	FPC_FILE_ANNOUNCE,							NULL,											FALSE	},
	{	FPC_LOAD_SPECIAL_FILE,						sFillPakClt_LoadSpecialFile,					FALSE	},
	{	FPC_LOAD_DEMOLEVEL,							sFillPakClt_LoadDemoLevel,						FALSE	}
};
#endif

void FillPakServerAddPendingJob(FPC_LOAD_GENERAL_MSG* msg) {
	UNREFERENCED_PARAMETER(msg);
}

static BOOL sFillPak_Execute(
	BYTE cmd,
	MSG_STRUCT* msg)
{
#if ISVERSION(SERVER_VERSION)
	FPC_LOAD_GENERAL_MSG* pMsg = (FPC_LOAD_GENERAL_MSG*)msg;
	ASSERT_RETFALSE(pMsg != NULL);

	pMsg->cmd = cmd;
	FillPakServerAddPendingJob(pMsg);

	return TRUE;
#else
	ASSERT_RETFALSE(cmd >= 0 && cmd < FILLPAK_RESPONSE_COUNT);
	ASSERT_RETFALSE(FILLPAK_CMD_HANDLER_ARRAY[cmd].iCmd == cmd);

	FILLPAK_GLOBALS& tGlobals = sFillPakGetGlobals();

	if (	 FILLPAK_CMD_HANDLER_ARRAY[cmd].handler
		&& ( ! tGlobals.ptConvert[ FPCONVERT_GENERIC_FILTER ] 
			|| FILLPAK_CMD_HANDLER_ARRAY[cmd].bAllowFilter ) ) 
	{
		return FILLPAK_CMD_HANDLER_ARRAY[cmd].handler(msg);
	} 
	else 
	{
		return TRUE;
	}
#endif
}

#if !ISVERSION(SERVER_VERSION)
BOOL FillPakClt_SendFinishedMsg()
{
#if ISVERSION(DEVELOPMENT)
	CHANNEL* pChannel = AppGetFillPakChannel();
	if (pChannel != NULL) {
		FILLPAK_CLIENT_JOB_FINISHED_MSG msg;
		return ConnectionManagerSend(pChannel, &msg, FILLPAK_CLIENT_JOB_FINISHED);
	} else {
		return FALSE;
	}
#else
	return TRUE;
#endif
	
}
BOOL FillPak_Execute(
	MSG_STRUCT* msg,
	BOOL& bExit)
{
	if (msg->hdr.cmd == FPC_LOAD_FINISHED) {
		bExit = TRUE;
		return TRUE;
	} else {
		BOOL bRet = sFillPak_Execute(msg->hdr.cmd, msg);
		if (msg->hdr.cmd != FPC_LOAD_CLEANUP) {
			FillPakClt_SendFinishedMsg();
		}
		return bRet;
	}
}
#endif

static void sFillPak_LoadUI()
{
	START_SECTION("UI");

	FPC_LOAD_GENERAL_MSG msg(0);
	sFillPak_Execute(FPC_LOAD_UI, &msg);

	END_SECTION();
}


static void sFillPak_LoadMasterDatabaseFile()
{
	START_SECTION("Master Database File");

	FPC_LOAD_GENERAL_MSG msg(0);
	sFillPak_Execute(FPC_LOAD_MASTER_DB, &msg);

	END_SECTION();
}

static void sFillPak_LoadEffect(
	const FILE_ITERATE_RESULT * resultFileFind,
	void * userData)
{
	ASSERT_RETURN(resultFileFind && resultFileFind->szFilenameRelativepath);
	FPC_LOAD_GENERAL_MSG msg(0);
	PStrCopy(msg.strFileName,
		resultFileFind->szFilenameRelativepath,
		DEFAULT_FILE_WITH_PATH_SIZE);
	sFillPak_Execute(FPC_LOAD_EFFECT, &msg);
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
}

static void sFillPak_LoadEffects()
{
#if FILLPAK_LOAD_EFFECTS
	START_SECTION( "Effects" );
	{
		FilesIterateInDirectory( FILE_PATH_EFFECT,		  "*.fxo", sFillPak_LoadEffect, NULL, TRUE);
		FilesIterateInDirectory( FILE_PATH_EFFECT_COMMON, "*.fxo", sFillPak_LoadEffect, NULL, TRUE);
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();

	int nScreenFXNonState = e_ScreenFXGetNumNonState();
	for ( int i = 0; i < nScreenFXNonState; ++i )
	{
		V( e_ScreenFXGetNonStateEffect( i, NULL, NULL ) );
	}
#endif // FILLPAK_LOAD_EFFECTS
}

static void sFillPak_LoadDemoLevel(
	const FILE_ITERATE_RESULT * resultFileFind,
	void * userData)
{
	ASSERT_RETURN(resultFileFind && resultFileFind->szFilenameRelativepath);
	FPC_LOAD_GENERAL_MSG msg(0);
	PStrCopy(msg.strFileName,
		resultFileFind->szFilenameRelativepath,
		DEFAULT_FILE_WITH_PATH_SIZE);
	sFillPak_Execute(FPC_LOAD_DEMOLEVEL, &msg);
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
}

static void sFillPak_LoadDemoLevels()
{
#if FILLPAK_LOAD_DEMOLEVELS
	START_SECTION( "DemoLevels" );
	{
		FilesIterateInDirectory( FILE_PATH_DEMOLEVEL,		  "*.xml", sFillPak_LoadDemoLevel, NULL, TRUE);
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_DEMOLEVELS
}

static void sFillPak_LoadWardrobesAll()
{
#if FILLPAK_LOAD_WARDROBES
	START_SECTION( "Wardrobes" );
	{
		CLT_VERSION_ONLY(V( e_WardrobeInit()));

		// attachments in layers
		int nLayerCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_LAYER );
		for ( int i = 0; i < nLayerCount; i++ ) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_WARDROBE_LAYER, &msg);
		}

		// models
		int nModelCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_MODEL );
		for (int i = 0; i < nModelCount; i++) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_WARDROBE_MODEL, &msg);
		}

		// textures
		int nAppearanceGroupCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP );
		for ( int j = 0; j < nAppearanceGroupCount; j++ ) {
			// blend textures
			int nBlendOpCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_BLENDOP );
			for ( int i = 0; i < nBlendOpCount; i++ ) {
				FPC_LOAD_GENERAL_MSG msg(j, i);
				sFillPak_Execute(FPC_LOAD_WARDROBE_BLENDOP, &msg);
			}
		}

		int nTextureSetCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_TEXTURESET );
		for ( int i = 0; i < nTextureSetCount; i++ ) {
			for ( int k = 0; k < NUM_TEXTURES_PER_SET; k++ ) {
				FPC_LOAD_GENERAL_MSG msg(i, k);
				sFillPak_Execute(FPC_LOAD_WARDROBE_TEXTURE, &msg);
			}
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_WARDROBES
}

static void sFillPak_LoadUnitsAll()
{
#if FILLPAK_LOAD_UNITDATA
	START_SECTION( "UnitData" );
	{
		GAME* pGame = AppGetCltGame();

		for ( int i = 0; i < NUM_GENUS; i++ ) {

			EXCELTABLE eTable = UnitGenusGetDatatableEnum( (GENUS) i );
			if ( eTable < 0 || eTable >= NUM_DATATABLES )
				continue;

			int nCount = ExcelGetNumRows( pGame, eTable );

			for ( int j = 0; j < nCount; j++ ) {
				FPC_LOAD_GENERAL_MSG msg(i, j);
				sFillPak_Execute(FPC_LOAD_UNIT, &msg);
			}
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_UNITDATA
}

static void sFillPak_LoadRooms()
{
#if FILLPAK_LOAD_ROOMS
	START_SECTION( "Rooms" );
	{
		GAME* pGame = AppGetCltGame();

		int nRoomIndexCount = ExcelGetNumRows( pGame, DATATABLE_ROOM_INDEX );
		for ( int i = 0; i < nRoomIndexCount; i++ ) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_ROOM, &msg);
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_ROOMS
}

static void sFillPak_LoadDRLGs()
{
#if FILLPAK_LOAD_DRLGS
	START_SECTION( "DRLGs" );
	{
		GAME* pGame = AppGetCltGame();

		int nRuleCount = ExcelGetNumRows( pGame, DATATABLE_LEVEL_RULES );

		// load all DRLGs
		for (int i = 0; i < nRuleCount; i++) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_DRLG, &msg);
		}

#if FILLPAK_LOAD_DRLG_LAYOUTS
//		SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());

		// load all DRLGs layout overrides
		for (int i = 0; i < nRuleCount; i++) {
			DRLG_PASS* def = (DRLG_PASS*)ExcelGetData(pGame, DATATABLE_LEVEL_RULES, i);
			if ( ! def )
				continue;
			//ASSERT_CONTINUE(def);

			OVERRIDE_PATH tOverride;
			OverridePathByLine(def->nPathIndex, &tOverride);
			ASSERT_CONTINUE( tOverride.szPath[ 0 ] );
			def->pDrlgDef = DRLGLevelDefinitionGet(tOverride.szPath, def->pszDrlgFileName);

			if ( AppCommonIsFillpakInConvertMode() && ! def->pDrlgDef )
				continue;

			ASSERT_CONTINUE(def->pDrlgDef);

			// Now we need to load any overridden room layouts:
			// First the templates
			for(int j=0; j<def->pDrlgDef->nTemplateCount; j++) {
				for(int k=0; k<def->pDrlgDef->pTemplates[j].nRoomCount; k++) {
					FPC_LOAD_GENERAL_MSG msg(i, j, k);
					sFillPak_Execute(FPC_LOAD_DRLG_TEMPLATE, &msg);
				}
			}

			// Now the substitutions
			for(int j=0; j<def->pDrlgDef->nSubstitutionCount; j++) {
				for(int k=0; k<def->pDrlgDef->pSubsitutionRules[j].nRuleRoomCount; k++) {
					FPC_LOAD_GENERAL_MSG msg(i, j, k);
					sFillPak_Execute(FPC_LOAD_DRLG_SUBSTITUTION_RULE, &msg);
				}

				for(int k=0; k<def->pDrlgDef->pSubsitutionRules[j].nReplacementCount; k++) {
					for(int l=0; l<def->pDrlgDef->pSubsitutionRules[j].pnReplacementRoomCount[k]; l++) {
						FPC_LOAD_GENERAL_MSG msg(i, j, k, l);
						sFillPak_Execute(FPC_LOAD_DRLG_SUBSTITUTION_REPLACEMENT, &msg);
					}
				}
			}
		}
#endif //FILLPAK_LOAD_DRLG_LAYOUTS
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_DRLGS
}

static void sFillPak_LoadEnvironments()
{
#if FILLPAK_LOAD_ENVIRONMENTS
	START_SECTION( "Environments" );
	{
		GAME* pGame = AppGetCltGame();

		// load all environments
		int nDRLGCount = ExcelGetNumRows(pGame, DATATABLE_LEVEL_DRLGS);
		int nThemeCount = ExcelGetNumRows(pGame, DATATABLE_LEVEL_THEMES);

		for(int i = 0; i < nDRLGCount; i++) {
			{
				FPC_LOAD_GENERAL_MSG msg(0, i);
				sFillPak_Execute(FPC_LOAD_ENVIRONMENT, &msg);
			}

			for (int j = 0; j < nThemeCount; j++) {
				for(int k=0; k<MAX_LEVEL_THEME_ALLOWED_STYLES; k++) {
					FPC_LOAD_GENERAL_MSG msg(1, i, j, k);
					sFillPak_Execute(FPC_LOAD_ENVIRONMENT, &msg);
				}
			}

			for(int j = 0; j < MAX_DRLG_THEMES; j++) {
				FPC_LOAD_GENERAL_MSG msg(2, i, j);
				sFillPak_Execute(FPC_LOAD_ENVIRONMENT, &msg);
			}

			for(int j = 0; j < MAX_WEATHER_SETS; j++) {
				for(int k = 0; k < MAX_THEMES_FROM_WEATHER; k++) {
					FPC_LOAD_GENERAL_MSG msg(3, i, j, k);
					sFillPak_Execute(FPC_LOAD_ENVIRONMENT, &msg);
				}
			}
		}

		int nNumChoices = ExcelGetNumRows(pGame, DATATABLE_LEVEL_DRLG_CHOICE);
		for ( int i = 0; i < nNumChoices; i++ )
		{
			LEVEL_DRLG_CHOICE * pChoice = ( LEVEL_DRLG_CHOICE * )ExcelGetData(pGame, DATATABLE_LEVEL_DRLG_CHOICE, i);
			if ( pChoice && ( pChoice->nEnvironmentOverride != INVALID_LINK ) )
			{
				FPC_LOAD_GENERAL_MSG msg(4, pChoice->nEnvironmentOverride);
				sFillPak_Execute(FPC_LOAD_ENVIRONMENT, &msg);
			}
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_ENVIRONMENTS
}

static void sFillPak_LoadGlobalMaterials()
{
	// load materials into global slots
	// iterate the global material spreadsheet
	int nRows = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_MATERIALS_GLOBAL );
	for (int i = 0; i < nRows; i++) {
		FPC_LOAD_GENERAL_MSG msg(i);
		sFillPak_Execute(FPC_LOAD_GLOBAL_MATERIAL, &msg);
	}
}

static void sFillPak_LoadStatesAll()
{
#if FILLPAK_LOAD_STATES
	START_SECTION( "States" );
	{
		GAME* pGame = AppGetCltGame();

		sFillPak_LoadGlobalMaterials();

		int nStates = ExcelGetNumRows( pGame, DATATABLE_STATES );
		for (int i = 0; i < nStates; i++) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_STATE, &msg);
		}

		{
			FPC_LOAD_GENERAL_MSG msg(0);
			sFillPak_Execute(FPC_LOAD_STATE_LIGHTING, &msg);
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_STATES
}

static void sFillPak_LoadSkillsAll()
{
#if FILLPAK_LOAD_SKILLS
	START_SECTION( "Skills" );
	{
		GAME* pGame = AppGetCltGame();

		int nSkillCount = ExcelGetNumRows(pGame, DATATABLE_SKILLS);
		for (int i = 0; i < nSkillCount; i++) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_SKILL, &msg);
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_SKILLS
}

static void sFillPak_LoadMeleeWeaponsAll()
{
#if FILLPAK_LOAD_MELEEWEAPONS
	START_SECTION( "Melee Weapons" );
	{
		GAME* pGame = AppGetCltGame();

		int nCount = ExcelGetNumRows( pGame, DATATABLE_MELEEWEAPONS );
		SKILL_EVENTS_DEFINITION * pEvents = NULL;

		for (int nRow = 0; nRow < nCount; nRow++) {
			MELEE_WEAPON_DATA * pDef = (MELEE_WEAPON_DATA *)ExcelGetData(pGame, DATATABLE_MELEEWEAPONS, nRow);
			if ( ! pDef )
				continue;
			{ // load the data in MELEE_WEAPON_DATA which is not in the events
				FPC_LOAD_GENERAL_MSG msg(nRow, -1, -1);
				sFillPak_Execute(FPC_LOAD_MELEE_WEAPON, &msg);
			}
			if ( pDef->szSwingEvents[ 0 ] != 0 ) {
				int nSwingEventsId = DefinitionGetIdByName( DEFINITION_GROUP_SKILL_EVENTS, pDef->szSwingEvents );
				pEvents = (SKILL_EVENTS_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_SKILL_EVENTS, nSwingEventsId );
				for ( int i = 0; i < pEvents->nEventHolderCount; i++ ) {
					SKILL_EVENT_HOLDER * pHolder = &pEvents->pEventHolders[ i ];
					for ( int j = 0; j < pHolder->nEventCount; j++ ) {
						FPC_LOAD_GENERAL_MSG msg(nRow, i, j);
						sFillPak_Execute(FPC_LOAD_MELEE_WEAPON, &msg);
					}
				}
			}
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif //FILLPAK_LOAD_MELEEWEAPONS
}

static void sFillPak_LoadAI()
{
#if FILLPAK_LOAD_AIS
	START_SECTION( "AIs" );
	{
		GAME* pGame = AppGetCltGame();

		// load all AIs
		unsigned int nAICount = ExcelGetNumRows(pGame, DATATABLE_AI_INIT);
		for (unsigned int ii = 0; ii < nAICount; ii++) {
			FPC_LOAD_GENERAL_MSG msg(ii);
			sFillPak_Execute(FPC_LOAD_AI, &msg);
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif // FILLPAK_LOAD_AIS
}

static void sFillPak_LoadSounds()
{
	GAME * pGame = AppGetCltGame();
	REF( pGame );

#if FILLPAK_LOAD_SOUNDS
	START_SECTION( "Sounds" );
	{
		// load all sounds
		// music files are now part of the sound spreadsheet
		int nSoundCount = ExcelGetNumRows( pGame, DATATABLE_SOUNDS );
		for ( int i = 0; i < nSoundCount; i++ ) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_SOUND, &msg);
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif
}

static void sFillPak_LoadSoundBuses()
{
	GAME * pGame = AppGetCltGame();
	REF( pGame );

#if FILLPAK_LOAD_SOUNDS
	START_SECTION( "SoundBuses" );
	{
		// load all sound buses
		int nSoundBusCount = ExcelGetNumRows( pGame, DATATABLE_SOUNDBUSES );
		for ( int i = 0; i < nSoundBusCount; i++ ) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_SOUNDBUS, &msg);
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif
}

static void sFillPak_LoadSoundMixStates()
{
	GAME * pGame = AppGetCltGame();
	REF( pGame );

#if FILLPAK_LOAD_SOUNDS
	START_SECTION( "SoundMixStates" );
	{
		// load all sound mix states
		int nSoundMixStateCount = ExcelGetNumRows( pGame, DATATABLE_SOUND_MIXSTATES );
		for ( int i = 0; i < nSoundMixStateCount; i++ ) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_SOUNDMIXSTATE, &msg);
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif
}

static void sFillPak_LoadSoundMixStateValues()
{
	GAME * pGame = AppGetCltGame();
	REF( pGame );

#if FILLPAK_LOAD_SOUNDS
	START_SECTION( "SoundMixStateValues" );
	{
		// load all sound mix states
		int nSoundMixStateValueCount = ExcelGetNumRows( pGame, DATATABLE_SOUND_MIXSTATE_VALUES );
		for ( int i = 0; i < nSoundMixStateValueCount; i++ ) {
			FPC_LOAD_GENERAL_MSG msg(i);
			sFillPak_Execute(FPC_LOAD_SOUNDMIXSTATEVALUE, &msg);
		}
	}
	//SVR_VERSION_ONLY(FillPakServerWaitForJobsToFinish());
	END_SECTION();
#endif
}

void FillPakServerSendCleanUpToClients()
{

}

static void sFillPak_EngineCleanUp()
{
	CLT_VERSION_ONLY(sFillPakClt_EngineCleanUp(NULL));
	SVR_VERSION_ONLY(FillPakServerSendCleanUpToClients());
}

static BOOL sFillPakServerFn()
{
	BOOL bDoFillPakRegular = TRUE;
	FILLPAK_GLOBALS& tGlobals = sFillPakGetGlobals();

#if !ISVERSION(SERVER_VERSION)
	if ( AppCommonIsFillpakInConvertMode() )
	{
		STR_DICTIONARY* pDict = tGlobals.ptConvert[ FPCONVERT_GENERIC_FILTER ];
		if ( ! pDict || StrDictionaryGetCount( pDict ) == 0 )
			bDoFillPakRegular = FALSE;

		for ( UINT nType = FPCONVERT_SPECIFIC_START; nType < FPCONVERT_TYPE_COUNT; nType++ )
		{
			pDict = tGlobals.ptConvert[ nType ];
			if ( pDict )
			{
				AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
				AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );

				UINT nCount = StrDictionaryGetCount( pDict );
				for ( UINT i = 0; i < nCount; i++ )
				{
					const char* str = StrDictionaryGetStr( pDict, i );

					switch ( nType )
					{
					case FPCONVERT_ROOM:
						{
							GAME* pGame = AppGetCltGame();
							int nRoomIndex = RoomFileGetRoomIndexLine( str );
							ASSERTV_CONTINUE( nRoomIndex != INVALID_ID, "Room not found in spreadsheet: '%s'", str );
							ROOM_DEFINITION* pDef = RoomDefinitionGet( pGame, nRoomIndex, TRUE );
							ASSERTV_CONTINUE( pDef, "Room conversion failed: '%s'", str );
						}
						break;

					case FPCONVERT_APPEARANCE:
						{
							APPEARANCE_DEFINITION* pDef = (APPEARANCE_DEFINITION*)DefinitionGetByName( DEFINITION_GROUP_APPEARANCE, str, 0, TRUE );
							ASSERTV_CONTINUE( pDef, "Appearance not found: '%s'", str );
							AppearanceDefinitionLoadSkeleton( pDef, TRUE );
						}
						break;

					case FPCONVERT_LAYOUT:
						{
							ROOM_LAYOUT_GROUP_DEFINITION *pDef = (ROOM_LAYOUT_GROUP_DEFINITION *) DefinitionGetByName( DEFINITION_GROUP_ROOM_LAYOUT,str,0,TRUE );
							ASSERTV_CONTINUE( pDef, "Layout not found: '%s'", str );
							CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( DEFINITION_GROUP_ROOM_LAYOUT );
							if(pContainer)
							{
								pContainer->SaveCooked(pDef);
							}
							
						}
						break;

					case FPCONVERT_PROP:
						{
							GAME* pGame = AppGetCltGame();
							PropDefinitionGet( pGame, str, TRUE, 0 );
						}
						break;

					case FPCONVERT_WARDROBEBLEND:
						{
							DWORD dwReloadFlags = TEXTURE_LOAD_NO_ASYNC_DEFINITION | TEXTURE_LOAD_NO_ASYNC_TEXTURE;

							char szFilePathImage[ DEFAULT_FILE_WITH_PATH_SIZE ];
							PStrPrintf( szFilePathImage, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, str );
							int nTextureID = INVALID_ID;
							V_CONTINUE( e_WardrobeLoadBlendTexture( &nTextureID, szFilePathImage, dwReloadFlags, 0 ) );
						}
						break;

					case FPCONVERT_TEXTURE:
						{
							DWORD dwReloadFlags = TEXTURE_LOAD_NO_ASYNC_DEFINITION | TEXTURE_LOAD_NO_ASYNC_TEXTURE;

							char szFilePathImage[ DEFAULT_FILE_WITH_PATH_SIZE ];
							PStrPrintf( szFilePathImage, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, str );
							int nTextureID = INVALID_ID;
							V_CONTINUE( e_TextureNewFromFile( &nTextureID, szFilePathImage, TEXTURE_GROUP_NONE, TEXTURE_NONE, 0, 0, NULL, NULL, NULL, NULL, dwReloadFlags ) );
						}
						break;

					case FPCONVERT_WARDROBEMODEL:
						{
							int nWardrobeModelIndex = WardrobeModelFileGetLine( str );
							ASSERTV_CONTINUE( nWardrobeModelIndex != INVALID_ID, "Wardrobe model not found in spreadsheet: '%s'", str );
							BOOL bResult = c_WardrobeLoadModel( nWardrobeModelIndex );
							ASSERTV_CONTINUE( bResult, "Wardrobe model conversion failed: '%s'", str );
						}
						break;

					}
				}

				AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
				AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
			}
		}
	}
#endif // SERVER_VERSION

	if ( bDoFillPakRegular )
	{
		START_SECTION("Fillpak Regular");

		sFillPak_LoadUI();
		sFillPak_LoadMasterDatabaseFile();

		sFillPak_LoadEffects();

		sFillPak_LoadDemoLevels();

		sFillPak_LoadWardrobesAll();
		sFillPak_EngineCleanUp();

		sFillPak_LoadUnitsAll();
		sFillPak_EngineCleanUp();

		sFillPak_LoadRooms();
		sFillPak_EngineCleanUp();

		sFillPak_LoadDRLGs();
		sFillPak_EngineCleanUp();

		sFillPak_LoadEnvironments();
		sFillPak_EngineCleanUp();

		sFillPak_LoadStatesAll();

		sFillPak_LoadSkillsAll();

		sFillPak_LoadMeleeWeaponsAll();
		sFillPak_EngineCleanUp();

		sFillPak_LoadAI();
		sFillPak_EngineCleanUp();

		// Dave Berk's special file
		{
			START_SECTION("Dave Berk's special file");

			FPC_LOAD_GENERAL_MSG msg;
			sFillPak_Execute(FPC_LOAD_SPECIAL_FILE, &msg);

			END_SECTION();
		}

		END_SECTION();
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sFillPakServerMinPakFn( 
	void)
{
	START_SECTION( "FillpakMin" );

	GAME * pGame = AppGetCltGame();
	REF( pGame );

	sFillPak_LoadUI();
	sFillPak_LoadMasterDatabaseFile();

	sFillPak_LoadEffects();

	sFillPak_LoadWardrobesAll();
	sFillPak_EngineCleanUp();

	sFillPak_LoadUnitsAll();
	sFillPak_EngineCleanUp();

	sFillPak_LoadStatesAll();

	sFillPak_LoadSkillsAll();

	sFillPak_LoadMeleeWeaponsAll();
	sFillPak_EngineCleanUp();

	END_SECTION(); // fillpak

	return TRUE;
}


static BOOL sFillPakSoundServerFn()
{
	START_SECTION("Fillpak Sound");

	sFillPak_LoadSounds();
	sFillPak_LoadSoundBuses();
	sFillPak_LoadSoundMixStates();
	sFillPak_LoadSoundMixStateValues();

	END_SECTION();

	return TRUE;
}

void FillPak_LoadStatesAll()
{
	sFillPak_LoadStatesAll();
}

void FillPak_LoadGlobalMaterials()
{
	sFillPak_LoadGlobalMaterials();
}

void FillPak_LoadEffects()
{
	sFillPak_LoadEffects();
}

void FillPak_LoadDemoLevels()
{
	sFillPak_LoadDemoLevels();
}

void FillPak_LoadWardrobesAll()
{
	sFillPak_LoadWardrobesAll();
}

#if !ISVERSION(SERVER_VERSION)
/////////////////////////////////////////////////////////////////
// Client Server stuff //////////////////////////////////////////
/////////////////////////////////////////////////////////////////

// message table for responses (server to client)
NET_COMMAND_TABLE* FillPakClientGetResponseTable()
{
	NET_COMMAND_TABLE * toRet = NULL;

	toRet = NetCommandTableInit(FILLPAK_RESPONSE_COUNT);
	ASSERT_RETNULL(toRet != NULL);

#undef  FILLPAK_MSG_RESPONSE_TABLE_BEGIN
#undef  FILLPAK_MSG_RESPONSE_TABLE_DEF
#undef  FILLPAK_MSG_RESPONSE_TABLE_END

#define FILLPAK_MSG_RESPONSE_TABLE_BEGIN(tbl)
#define FILLPAK_MSG_RESPONSE_TABLE_END(count)
#define FILLPAK_MSG_RESPONSE_TABLE_DEF(c, strct, s, r) {						\
	strct msg;																\
	MSG_STRUCT_DECL* msg_struct = msg.zz_register_msg_struct(toRet);		\
	NetRegisterCommand(toRet, c, msg_struct, s, r,							\
	static_cast<MFP_PRINT>(& strct::Print),								\
	static_cast<MFP_PUSHPOP>(& strct::Push),							\
	static_cast<MFP_PUSHPOP>(& strct::Pop));							\
	}

#undef  _FILLPAK_RESPONSE_TABLE_
#include "fillpakservermsg.h"

	if (NetCommandTableValidate(toRet) == FALSE) {
		NetCommandTableFree(toRet);
		return NULL;
	} else {
		return toRet;
	}
}

// message table for requests (client to server)
NET_COMMAND_TABLE* FillPakClientGetRequestTable()
{
	NET_COMMAND_TABLE* toRet = NULL;

	toRet = NetCommandTableInit(FILLPAK_REQUEST_COUNT);
	ASSERT_RETNULL(toRet != NULL);

#undef  FILLPAK_MSG_REQUEST_TABLE_BEGIN
#undef  FILLPAK_MSG_REQUEST_TABLE_DEF
#undef  FILLPAK_MSG_REQUEST_TABLE_END

#define FILLPAK_MSG_REQUEST_TABLE_BEGIN(tbl)
#define FILLPAK_MSG_REQUEST_TABLE_END(count)
#define FILLPAK_MSG_REQUEST_TABLE_DEF(c, strct, hdl, s, r) {					\
	strct msg;																\
	MSG_STRUCT_DECL* msg_struct = msg.zz_register_msg_struct(toRet);		\
	NetRegisterCommand(toRet, c, msg_struct, s, r,							\
	static_cast<MFP_PRINT>(& strct::Print),								\
	static_cast<MFP_PUSHPOP>(& strct::Push),							\
	static_cast<MFP_PUSHPOP>(& strct::Pop));							\
	}

#undef  _FILLPAK_REQUEST_TABLE_
#include "fillpakservermsg.h"

	if (NetCommandTableValidate(toRet) == FALSE) {
		NetCommandTableFree(toRet);
		return NULL;
	} else {
		return toRet;
	}
}

static BOOL sFillPakCheckForMessage(
	FILLPAK_SERVER_RESPONSE_COMMANDS cmd,
	MSG_STRUCT* pMSG)
{
#if ISVERSION(DEVELOPMENT)
	MAILSLOT* mailslot = AppGetFillPakMailSlot();
	ASSERT_RETFALSE(mailslot != NULL);

	MSG_BUF *buf, *head, *tail, *next, *prev;
	UINT32 count = MailSlotGetMessages(mailslot, head, tail);

	if (count == 0) {
		return FALSE;
	}

	buf = head;
	prev = NULL;
	while (buf != NULL) {
		next = buf->next;
		if (buf->msg[0] == cmd) {
			if (next) {
				MailSlotPushbackMessages(mailslot, next, tail);
			}
			if (prev) {
				MailSlotPushbackMessages(mailslot, head, prev);
			}

			BOOL result = TRUE;
			if (pMSG != NULL) {
				result = NetTranslateMessageForRecv(gApp.m_FillPakServerCommandTable, buf->msg, buf->size, pMSG);
			}
			MailSlotRecycleMessages(mailslot, buf, buf);

			ASSERT_RETFALSE(result);
			return TRUE;
		} else {
			prev = buf;
			buf = next;
		}
	}
	MailSlotPushbackMessages(mailslot, head, tail);
	return FALSE;
#else
	UNREFERENCED_PARAMETER(cmd);
	UNREFERENCED_PARAMETER(pMSG);
	return TRUE;
#endif
}

#if ISVERSION(DEVELOPMENT)

HANDLE hSendFileEvent = NULL;
BOOL bSend = FALSE;

BOOL FillPakSendFileToServer(
	LPCTSTR filepath,
	LPCTSTR filename,
	UINT32 filesize,
	UINT32 filesize_compressed,
	PAK_ENUM paktype,
	void* pBuffer,
	UINT8* hash,
	FILE_HEADER* pHeader)
{
	static CCriticalSection lock(TRUE);
	CSAutoLock autoLock(&lock);

	ASSERT_RETFALSE(AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT));
	if (AppGetState() == APP_STATE_INIT) {
		return TRUE; 
	}
	CHANNEL* pChannel = AppGetFillPakChannel();
	ASSERT_RETFALSE(pChannel != NULL);

	FILLPAK_CLIENT_SEND_FILE_INFO_MSG msgInfo;
	PStrPrintf(msgInfo.filename, DEFAULT_FILE_WITH_PATH_SIZE, _T("%s%s"), filepath, filename);
	MemoryCopy(msgInfo.sha_hash, SHA1HashSize, hash, SHA1HashSize);
	MemoryCopy(msgInfo.fileHeader, sizeof(FILE_HEADER), pHeader, sizeof(FILE_HEADER));
	msgInfo.filesize = filesize;
	msgInfo.filesize_compressed = filesize_compressed;
	msgInfo.paktype = paktype;

	ResetEvent(hSendFileEvent);
	ASSERT_RETFALSE(ConnectionManagerSend(pChannel, &msgInfo, FILLPAK_CLIENT_SEND_FILE_INFO));
	WaitForSingleObject(hSendFileEvent, INFINITE);
	if (bSend == FALSE) {
		return TRUE;
	}

/*	FPC_FILE_INFO_RESPONSE_MSG msgResponse;
	while (sFillPakCheckForMessage(FPC_FILE_INFO_RESPONSE, &msgResponse) == FALSE) {Sleep(0);}
	if (msgResponse.bSend == FALSE) {
		return TRUE;
	}*/

	UINT32 total = (filesize_compressed == 0) ? filesize : filesize_compressed;
	UINT32 cur = 0, size, diff;
	FILLPAK_CLIENT_SEND_FILE_MSG msgData;
	while (cur < total) {
		diff = total - cur;
		size = (diff > DEFAULT_FPC_FILE_DATA_SIZE) ? DEFAULT_FPC_FILE_DATA_SIZE : diff;
		msgData.offset = cur;
		MemoryCopy(msgData.data, DEFAULT_FPC_FILE_DATA_SIZE, (UINT8*)pBuffer + cur, size);
		MSG_SET_BLOB_LEN(msgData, data, size);
		ASSERT_RETFALSE(ConnectionManagerSend(pChannel, &msgData, FILLPAK_CLIENT_SEND_FILE));
		cur += size;
	}
	ASSERT_RETFALSE(cur == total);
//	WaitForSingleObject(hSendFileEvent, INFINITE);
//	FPC_FILE_INFO_RESPONSE_MSG msgResponse;
//	while (sFillPakCheckForMessage(FPC_FILE_INFO_RESPONSE, &msgResponse) == FALSE) {Sleep(0);}

	return TRUE;
}

VOID FillPakClientResponseHandler(
	struct NETCOM* pNetcom,
	NETCLIENTID netCltId,
	BYTE* data,
	UINT32 msg_size)
{
	UNREFERENCED_PARAMETER(pNetcom);
	UNREFERENCED_PARAMETER(netCltId);
	UNREFERENCED_PARAMETER(data);
	UNREFERENCED_PARAMETER(msg_size);

	FPC_FILE_INFO_RESPONSE_MSG msgInfo;
	ASSERT(NetTranslateMessageForRecv(gApp.m_FillPakServerCommandTable, data, msg_size, &msgInfo));
	bSend = msgInfo.bSend;
	if (hSendFileEvent != NULL) {
		SetEvent(hSendFileEvent);
	}
}

#endif
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sFillPakLocalized(
	void)
{
	START_SECTION("Fillpak Localized");

	ASSERTX_RETFALSE( PakFileUsedInApp( PAK_LOCALIZED ), "Cannot fill language pak, pak is not created for this app" );
	FILLPAK_GLOBALS& tGlobals = sFillPakGetGlobals();
	
	// translate SKU name to datatable index
	int nSKU = ExcelGetLineByStrKey( DATATABLE_SKU, tGlobals.szSKUName );
	ASSERTV_RETFALSE( nSKU != INVALID_LINK, "Invalid SKU (%s) for fill pak SKU", tGlobals.szSKUName );

	{
		START_SECTION("Strings");

		// fill strings for all languages in this sku
		if (StringTablesFillPak( nSKU ) == FALSE)
		{
			return FALSE;
		}
		END_SECTION();
	}

	{
		START_SECTION("Textures");

		// add localized textures from level textures
		int nFolderCount = ExcelGetNumRows( EXCEL_CONTEXT(), DATATABLE_LEVEL_FILE_PATHS );
		for (int i = 0; i < nFolderCount; ++i)
		{
			FPC_LOAD_GENERAL_MSG tMessage( i, nSKU );
			sFillPak_Execute( FPC_LOAD_LEVEL_LOCALIZED, &tMessage );
		}
		END_SECTION();
	}

	// add the SKU code to the pak file
	if (SKUWriteToPak( nSKU ) == FALSE)
	{
		return FALSE;
	}

	{
		START_SECTION("Voice");

		// Add localized voice files to the language pak here
		int nSoundCount = ExcelGetNumRows(EXCEL_CONTEXT(), DATATABLE_SOUNDS);
		for(int i=0; i<nSoundCount; i++)
		{
			FPC_LOAD_GENERAL_MSG msg(i, nSKU);
			sFillPak_Execute(FPC_LOAD_SOUND_LOCALIZED, &msg);
		}
		END_SECTION();
	}

	// add any localized UI elements
	{
		START_SECTION("Localized UI");

		CLT_VERSION_ONLY(UIInit(FALSE, TRUE, nSKU));
		END_SECTION();
	}

	{
		START_SECTION("Movies");

		// Add movies to the movie pak here
		int nMovieCount = ExcelGetNumRows(EXCEL_CONTEXT(), DATATABLE_MOVIES);
		for(int i=0; i<nMovieCount; i++)
		{
			FPC_LOAD_GENERAL_MSG msg(i, nSKU);
			sFillPak_Execute(FPC_LOAD_MOVIE_LOCALIZED, &msg);
		}
		END_SECTION();
	}

	{
		START_SECTION("EULA text");

		// Add the localized EULA text
		{
			FPC_LOAD_GENERAL_MSG msg(nSKU);
			sFillPak_Execute(FPC_LOAD_EULA_LOCALIZED, &msg);
		}
		END_SECTION();
	}

	END_SECTION();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FillPak(
	FILLPAK_TYPE eType)
{
	if ( AppCommonIsFillpakInConvertMode() && eType != FILLPAK_DEFAULT )
		return TRUE;

	// fill pak file
	switch (eType)
	{
	case FILLPAK_DEFAULT:		return sFillPakServerFn();
	case FILLPAK_SOUND:			return sFillPakSoundServerFn();
//	case FILLPAK_DEFAULT:		return sFillPak();
//	case FILLPAK_SOUND:			return sFillPakSound();
#if !ISVERSION(SERVER_VERSION)
	case FILLPAK_MIN:			return sFillPakServerMinPakFn();
	case FILLPAK_ADVERT_DUMP:	return sGenerateAdReport();
#endif
	case FILLPAK_LOCALIZED:		return sFillPakLocalized();
	default:				ASSERTX_RETFALSE( 0, "Unknown fillpak type" );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FillPakLocalizedSetup(
	const char *pszSKU)
{
	FILLPAK_GLOBALS& tGlobals = sFillPakGetGlobals();
	tGlobals.szSKUName[ 0 ] = 0;
	
	// save the specific type of language pak building we want to do			
	if (pszSKU)
	{
		PStrCopy( tGlobals.szSKUName, pszSKU, MAX_SKU_NAME_LEN );
	}

	if ( tGlobals.szSKUName[ 0 ])
	{
		// set flag that we're doing some sort of language pak
		AppSetDebugFlag( ADF_FILL_LOCALIZED_PAK_BIT, TRUE );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FillPakAddFilter(
	const TCHAR* filterString )
{
	FILLPAK_GLOBALS& tGlobals  = sFillPakGetGlobals();
	FPCONVERT eConvertType = FPCONVERT_GENERIC_FILTER;
	
	for ( UINT nType = FPCONVERT_SPECIFIC_START; nType < FPCONVERT_TYPE_COUNT; nType++ )
	{
		if ( StrStrI( filterString, sgcFPConvertTypes[ nType ] ) )
		{
			UINT nLen = PStrLen( sgcFPConvertTypes[ nType ] );
			eConvertType = (FPCONVERT)nType;
			filterString += nLen;
		}
	}

	if ( ! tGlobals.ptConvert[ eConvertType ] )
		tGlobals.ptConvert[ eConvertType ] = StrDictionaryInit( DEFAULT_DICT_BUFFER_SIZE, TRUE );

	StrDictionaryAdd( tGlobals.ptConvert[ eConvertType ], filterString, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FillPakShouldLoad( const char* key )
{
	BOOL bFound = TRUE;

	FILLPAK_GLOBALS& tGlobals = sFillPakGetGlobals();
	STR_DICTIONARY* pDict = tGlobals.ptConvert[ FPCONVERT_GENERIC_FILTER ];
	if ( pDict )
	{
		bFound = FALSE;

		UINT nCount = StrDictionaryGetCount( pDict );
		for ( UINT i = 0; i < nCount; i++ )
		{
			const char* str = StrDictionaryGetStr( pDict, i );
			if ( PStrStrI( key, str ) )
			{
				bFound = TRUE;
				break;
			}
		}		
		//StrDictionaryFind( tGlobals.ptFilter, key, &bFound );
	}

	return bFound;
}


//#endif
