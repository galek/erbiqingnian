//----------------------------------------------------------------------------
// config.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _CONFIG_H
#define _CONFIG_H

#ifndef _DEFINITION_COMMON_H_
#include "definition_common.h"
#endif

#ifndef _ACCOUNTBADGES_H_
#include "accountbadges.h"
#endif

//----------------------------------------------------------------------------

#define GLOBAL_FLAG_NORAGDOLLS							MAKE_MASK(0)		// must be false for tugboat
#define GLOBAL_FLAG_INVERTMOUSE							MAKE_MASK(1)
#define GLOBAL_FLAG_NO_SHRINKING_BONES					MAKE_MASK(2)
#define GLOBAL_FLAG_CHEAT_LEVELS						MAKE_MASK(3)
#define GLOBAL_FLAG_NOMONSTERS							MAKE_MASK(4)
#define GLOBAL_FLAG_GENERATE_ASSETS_IN_GAME				MAKE_MASK(5)
#define GLOBAL_FLAG_DATA_WARNINGS						MAKE_MASK(6)
#define GLOBAL_FLAG_BACKGROUND_WARNINGS					MAKE_MASK(7)
#define GLOBAL_FLAG_FORCE_SYNCH							MAKE_MASK(8)
#define GLOBAL_FLAG_LOAD_DEBUG_BACKGROUND_TEXTURES		MAKE_MASK(9)
#define GLOBAL_FLAG_SKILL_LEVEL_CHEAT					MAKE_MASK(10)
#define GLOBAL_FLAG_SILENT_ASSERT						MAKE_MASK(11)
#define GLOBAL_FLAG_NOLOOT								MAKE_MASK(12)
#define GLOBAL_FLAG_ABSOLUTELYNOMONSTERS				MAKE_MASK(13)
#define GLOBAL_FLAG_FULL_LOGGING						MAKE_MASK(14)
#define GLOBAL_FLAG_UPDATE_TEXTURES_IN_GAME				MAKE_MASK(15)
#define GLOBAL_FLAG_AUTOMAP_ROTATE						MAKE_MASK(16)
#define GLOBAL_FLAG_FORCEBLOCKINGSOUNDLOAD				MAKE_MASK(17)
#define GLOBAL_FLAG_HAVOKFX_ENABLED						MAKE_MASK(18)		// must be false for tugboat
#define GLOBAL_FLAG_MULTITHREADED_HAVOKFX_ENABLED		MAKE_MASK(19)		// must be false for tugboat
#define GLOBAL_FLAG_HAVOKFX_RAGDOLL_ENABLED				MAKE_MASK(20)		// must be false for tugboat
#define GLOBAL_FLAG_NO_POPUPS                 		    MAKE_MASK(21)
#define GLOBAL_FLAG_NO_CLEANUP_BETWEEN_LEVELS			MAKE_MASK(22)
#define GLOBAL_FLAG_CAST_ON_HOTKEY						MAKE_MASK(23)
#define GLOBAL_FLAG_STRING_WARNINGS						MAKE_MASK(24)
#define GLOBAL_FLAG_MAX_POWER							MAKE_MASK(25)
#define GLOBAL_FLAG_SKILL_COOLDOWN						MAKE_MASK(26)
#define GLOBAL_FLAG_USE_HQ_SOUNDS						MAKE_MASK(27) 
#define GLOBAL_FLAG_PROMPT_FOR_CHECKOUT					MAKE_MASK(28)
#define GLOBAL_FLAG_DONT_UPDATE_TEXTURES_IN_HAMMER		MAKE_MASK(29)

																  
#define GLOBAL_GAMEFLAG_AIFREEZE						MAKE_MASK(1)
#define GLOBAL_GAMEFLAG_AINOTARGET						MAKE_MASK(2)
#define GLOBAL_GAMEFLAG_PLAYERINVULNERABLE				MAKE_MASK(3)
#define GLOBAL_GAMEFLAG_ALLINVULNERABLE					MAKE_MASK(4)
#define GLOBAL_GAMEFLAG_ALWAYSGETHIT					MAKE_MASK(5)
#define GLOBAL_GAMEFLAG_ALWAYSSOFTHIT					MAKE_MASK(6)
#define GLOBAL_GAMEFLAG_ALWAYSKILL						MAKE_MASK(7)
#define GLOBAL_GAMEFLAG_ALLOW_CHEATS					MAKE_MASK(8)
#define GLOBAL_GAMEFLAG_CAN_EQUIP_ALL_ITEMS				MAKE_MASK(9)


typedef struct GLOBAL_DEFINITION {
	DEFINITION_HEADER tHeader;
	float			  fPlayerSpeed;
	DWORD			  dwFlags;
	DWORD			  dwGameFlags;
	DWORD			  dwSeed;
	DWORD			  dwFogColor;
	char			  szPlayerName[MAX_XML_STRING_LENGTH];
	char			  szEnvironment[DEFAULT_FILE_WITH_PATH_SIZE];
	int				  nDefWidth;
	int				  nDefHeight;
	int				  nDebugOutputLevel;
	int				  nDebugSoundDelayTicks;
	char			  szLanguage[MAX_XML_STRING_LENGTH];
	char			  szAuthenticationServer[MAX_XML_STRING_LENGTH];
	BADGE_COLLECTION  Badges;

}GLOBAL_DEFINITION;

//----------------------------------------------------------------------------
#define CONFIG_FLAG_UNUSED				MAKE_MASK( 0 ) 
#define CONFIG_FLAG_NOMUSIC				MAKE_MASK( 1 ) 
#define CONFIG_FLAG_NOSOUND				MAKE_MASK( 2 ) 
#define CONFIG_FLAG_HIGH_QUALITY_SOUNDS	MAKE_MASK( 3 ) 
#define CONFIG_FLAG_LOW_QUALITY_SOUNDS	MAKE_MASK( 4 ) 
struct CONFIG_DEFINITION
{
	DEFINITION_HEADER	tHeader;
	DWORD				dwFlags;
	int					nSoundOutputType;
	int					nSoundSpeakerConfig;
	int					nSoundMemoryReserveType;
	int					nSoundMemoryReserveMegabytes;
	float				fMouseSensitivity;
};

//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline DWORD DefinitionGetGlobalSeed(
	void)
{
	const GLOBAL_DEFINITION * global = DefinitionGetGlobal();
	ASSERT_RETZERO(global);
	return global->dwSeed;
}


#endif
