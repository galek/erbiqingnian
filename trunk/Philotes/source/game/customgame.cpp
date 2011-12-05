//----------------------------------------------------------------------------
// customgame.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "customgame.h"
#include "units.h"
#include "teams.h"
#include "player.h"
#include "console.h"
#include "chat.h"
#include "warp.h"
#include "level.h"
#include "pets.h"
#include "states.h"

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_CustomGameSetup(
	GAME * game,
	const GAME_SETUP * game_setup)
{
	ASSERT_RETFALSE(game);

	GameSetGameFlag(game, GAMEFLAG_DISABLE_PLAYER_INTERACTION, TRUE);

	if (!game_setup)
	{
		return TRUE;
	}
	
	// presumable this should really do something with game_setup->nNumTeams -cday
	
	// for now...
	if( AppIsHellgate() )
	{
		TeamsAdd(game, L"Dragon", "Red-Black-Orange", TEAM_PLAYER_START, RELATION_HOSTILE);
		TeamsAdd(game, L"Salamander", "Grey-Gold-LBlue", TEAM_PLAYER_START, RELATION_HOSTILE);
	}
	else
	{
		TeamsAdd(game, L"Red", "Red-Black-Orange", TEAM_PLAYER_START, RELATION_HOSTILE);
		TeamsAdd(game, L"Blue", "Grey-Gold-LBlue", TEAM_PLAYER_START, RELATION_HOSTILE);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct ROOM * s_CustomGameGetStartPosition(
	GAME * game,
	UNIT * unit,
	LEVELID idLevel,
	VECTOR & vPosition, 
	VECTOR & vFaceDirection, 
	int * pnRotation)
{
	ASSERT_RETNULL(game);
	ASSERT_RETNULL(unit);

	unsigned int team = UnitGetTeam(unit);

	WARP_TO_TYPE eWarpToType = WARP_TO_TYPE_PREV;

	// for now, assign warp based on team index
	if (team == NUM_DEFAULT_TEAMS)
	{
		eWarpToType = WARP_TO_TYPE_NEXT;
	}

	ROOM * room = LevelGetWarpToPosition(
		game, 
		idLevel, 
		eWarpToType, 
		NULL, 
		vPosition, 
		vFaceDirection,
		pnRotation);		
		
	ASSERT_RETNULL(room);

	return room;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CustomGameWarpToStart(
	GAME * game,
	UNIT * player)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(player);

	ROOM * cur_room = UnitGetRoom(player);
	ASSERT_RETURN(cur_room);
	LEVEL * level = RoomGetLevel(cur_room);
	ASSERT_RETURN(level);

	VECTOR vPosition;
	VECTOR vFaceDirection;
	int nRotation;

	ROOM * room = s_CustomGameGetStartPosition(game, player, LevelGetID(level), vPosition, vFaceDirection, &nRotation);
	ASSERT_RETURN(room);

	DWORD dwWarpFlags = 0;
	SETBIT(dwWarpFlags, UW_TRIGGER_EVENT_BIT);
	UnitWarp(player, room, vPosition, vFaceDirection, VECTOR(0, 0, 1), dwWarpFlags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPetAddToOwnerTeam(
	UNIT *pPet,
	void *pUserData )
{
	UNIT * pOwner = (UNIT*)pUserData;
	ASSERT_RETURN( pOwner );
	ASSERT_RETURN( pPet );
	TeamAddUnit( pPet, pOwner->nTeam );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_CustomGameEventAddPlayer(
	GAME * game,
	UNIT * player)
{
	ASSERT_RETURN(game);

	if (GameGetSubType(game) != GAME_SUBTYPE_CUSTOM)
	{
		return;
	}

	if (!IS_SERVER(game))
	{
		return;
	}
	
	// for now... figure out what team the player ought to be on
	int team = INVALID_TEAM;
	unsigned int least = UINT_MAX;

	unsigned int team_count = TeamsGetTeamCount(game);
	for (unsigned int ii = NUM_DEFAULT_TEAMS; ii < team_count; ++ii)
	{
		if (TeamGetBaseType(game, ii) != TEAM_PLAYER_START)
		{
			continue;
		}
		unsigned int player_count = TeamGetPlayerCount(game, ii);
		if (player_count >= least)
		{
			continue;
		}
		team = ii;
		least = player_count;
	}

	StateClearAllOfType( player, STATE_PVP_ENABLED );
	s_StateSet(player, player, STATE_PVP_MATCH, 0 );

	ASSERT_RETURN(team != INVALID_TEAM);
	ASSERT_RETURN(TeamAddUnit(player, team));
	PetIteratePets(player, sPetAddToOwnerTeam, player);
	WCHAR unit_name[MAX_CHARACTER_NAME];
	UnitGetName(player, unit_name, arrsize(unit_name), 0);
	const WCHAR * strTeamName = TeamGetName(game, team);
	ASSERTX_RETURN( strTeamName && strTeamName[0], "expected a non empty string for team name in custom games" ); 
	s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, L"%s joined %s", unit_name, strTeamName);

	CustomGameWarpToStart(game, player);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_CustomGameEventKill(
	GAME * game,
	UNIT * attacker,
	UNIT * defender)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(defender);

	if (!attacker || !defender)
	{
		return;
	}

	if (!UnitIsA(defender, UNITTYPE_PLAYER))
	{
		return;
	}

	WCHAR attacker_name[MAX_CHARACTER_NAME];
	ASSERT_RETURN(UnitGetName(attacker, attacker_name, arrsize(attacker_name), 0));

	WCHAR defender_name[MAX_CHARACTER_NAME];
	ASSERT_RETURN(UnitGetName(defender, defender_name, arrsize(defender_name), 0));

	int attacker_score = 0;
	int defender_score = 0;

	unsigned int attacker_team = UnitGetTeam(attacker);
	if (attacker_team != INVALID_TEAM)
	{
		STATS * attacker_team_stats = TeamGetStatsList(game, attacker_team);
		ASSERT(attacker_team_stats);
		if (attacker_team_stats)
		{
			StatsChangeStat(game, attacker_team_stats, STATS_TEAM_SCORE, attacker_team, 1);
			attacker_score = StatsGetStat(attacker_team_stats, STATS_TEAM_SCORE, attacker_team);
		}
	}

	unsigned int defender_team = UnitGetTeam(defender);
	if (defender_team != INVALID_TEAM)
	{
		STATS * defender_team_stats = TeamGetStatsList(game, defender_team);
		ASSERT(defender_team_stats);
		if (defender_team_stats)
		{
			defender_score = StatsGetStat(defender_team_stats, STATS_TEAM_SCORE, defender_team);
		}
	}

	s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, L"[%s] was killed by [%s].", defender_name, attacker_name);
	const WCHAR * strTeamNameAttacker = TeamGetName(game, attacker_team);
	ASSERTX_RETURN(strTeamNameAttacker && strTeamNameAttacker[0], "expected a non empty string for team name in custom games"); 
	const WCHAR * strTeamNameDefender = TeamGetName(game, defender_team);
	ASSERTX_RETURN(strTeamNameDefender && strTeamNameDefender[0], "expected a non empty string for team name in custom games"); 
	if (attacker_team < defender_team)
	{
		s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, L"SCORE  %s:%d  %s:%d", strTeamNameAttacker, attacker_score, strTeamNameDefender, defender_score);
	}
	else
	{
		s_SendSysTextFmt(CHAT_TYPE_GAME, FALSE, game, NULL, CONSOLE_SYSTEM_COLOR, L"SCORE  %s:%d  %s:%d", strTeamNameDefender, defender_score, strTeamNameAttacker, attacker_score);
	}

}