//----------------------------------------------------------------------------
// FILE: gamevariant.h
//----------------------------------------------------------------------------

#ifndef __GAME_VARIANT_H_
#define __GAME_VARIANT_H_


//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef	_NETMSG_H_
#include "netmsg.h"
#endif


//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct UNIT;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum GAME_VARIANT_TYPE // Any additional flags MUST be added to the end
{
	GV_INVALID = -1,
	
	GV_HARDCORE,		// this player or game is a dedicated hardcore game
	GV_ELITE,			// this player is elite, or this game is a dedicated elite game
	GV_LEAGUE,			// this player is in a player league.
	GV_PVPWORLD,		
	
	GV_UTILGAME,		// this game has been setup as the utility game for this game server

	GV_NUM_VARIANTS		// keep this last please
	
};

//----------------------------------------------------------------------------
enum GAME_VARIANT_COMPARE_FLAG
{
	GVC_DIFFICULTY =	MAKE_MASK( 0 ),		// check the difficulty when comparing variants
	GVC_VARIANT_TYPE =	MAKE_MASK( 1 ),		// check the variant type bit flags when comparing variants
	
	GVC_ALL = GVC_DIFFICULTY |		\
			  GVC_VARIANT_TYPE,				// check all compare options
			  
};

//----------------------------------------------------------------------------
// These are things that can be different from a normal game.  A player
// can decide to play the game at a higher difficulty, or with special
// game modes of play (elite, hardcore, league, etc)
//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(GAME_VARIANT)
	MSG_FIELD(0, int, nDifficultyCurrent,		INVALID_LINK)
	MSG_FIELD(1, DWORD,	dwGameVariantFlags,		0)	// see GAME_VARIANT_TYPE enum
END_MSG_STRUCT

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void GameVariantInit(
	struct GAME_VARIANT &tGameVariant,
	UNIT *pUnit);
	
DWORD s_GameVariantFlagsGetFromNewPlayerFlags(
	APPCLIENTID idAppClient,
	DWORD dwNewPlayerFlags);		// see NEW_PLAYER_FLAGS

BOOL GameVariantsAreEqual(
	const struct GAME_VARIANT &tVariant1,
	const struct GAME_VARIANT &tVariant2,
	DWORD dwGameVariantCompareFlags = GVC_ALL,	// see GAME_VARIANT_COMPARE_FLAG
	DWORD *pdwDifferences = NULL);

DWORD GameVariantFlagsGet(
	UNIT *pPlayer);

DWORD GameVariantFlagsGetStaticUnit(
		UNIT *pPlayer);

DWORD GameVariantFlagsGetStatic(
	DWORD dwGameVariantFlags);

#endif