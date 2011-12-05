//----------------------------------------------------------------------------
// teams.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "unit_priv.h" // also includes units.h, which includes game.h
#include "teams.h"
#include "monsters.h"
#include "clients.h"
#include "stats.h"
#include "s_message.h"
#include "objects.h"
#include "pets.h"
#include "combat.h"
#include "level.h"
#include "metagame.h"

//----------------------------------------------------------------------------
// DEFINE
//----------------------------------------------------------------------------
#define TEAM_TRACE
#define TEAM_UNKNOWN_NAME L"Unknown"


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct TEAM
{
	WCHAR *				m_szTeamName;		// team name
	STATS *				m_TeamStats;
	BYTE *				m_pbRelations;		// table of relations to all other teams, default is RELATION_DEFAULT

	unsigned int		m_nTeamId;			// corresponds to index in team list
	unsigned int		m_nBaseTeam;		// TEAM_BAD etc...  one of NUM_DEFAULT_TEAMS
	unsigned int		m_nPlayerCount;		// number of players on the team
	unsigned int		m_nUnitCount;		// number of units on the team
	unsigned int		m_nRelations;
	int					m_nColorIndex;

	BOOL				m_bActive;			
	BOOL				m_bDeactivateEmpty;	// deactivate this team when the unit count reaches zero
};


struct TEAM_TABLE
{
	TEAM **				m_Teams;			// list of teams in the game
	unsigned int		m_nTeamCount;		// number of teams in the game
	unsigned int		m_nTeamBits;		// number of bits neaded to store teams
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static WCHAR * g_szDefaultTeamNames[] =
{
	L"bad",
	L"neutral",
	L"player",
	L"player PvP FFA",
	L"destructible",
};


//----------------------------------------------------------------------------
// STATIC FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL ValidateTeamTable(
	TEAM_TABLE * team_table)
{
#if ISVERSION(DEVELOPMENT)
	ASSERT_RETFALSE(team_table->m_nTeamCount == 0 || team_table->m_Teams != NULL);
	for (unsigned int ii = 1; ii < team_table->m_nTeamCount; ++ii)
	{
		for (unsigned int jj = 0; jj < ii; ++jj)
		{
			if (team_table->m_Teams[ii] == NULL || team_table->m_Teams[jj] == NULL)
			{
				continue;
			}
			if (team_table->m_Teams[ii]->m_bActive == FALSE || team_table->m_Teams[jj]->m_bActive == FALSE)
			{
				continue;
			}
			ASSERT_RETFALSE(team_table->m_Teams[ii] != team_table->m_Teams[jj]);
			ASSERT_RETFALSE(team_table->m_Teams[ii]->m_nRelations == team_table->m_nTeamCount);
			ASSERT_RETFALSE(team_table->m_Teams[ii]->m_pbRelations != NULL);
			ASSERT_RETFALSE(team_table->m_Teams[jj]->m_nRelations == team_table->m_nTeamCount);
			ASSERT_RETFALSE(team_table->m_Teams[jj]->m_pbRelations != NULL);
			ASSERT_RETFALSE(team_table->m_Teams[ii]->m_pbRelations != team_table->m_Teams[jj]->m_pbRelations);
		}
	}
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TEAM * sTeamGetByIndex(
	TEAM_TABLE * team_table,
	unsigned int teamidx)
{
	ASSERT_RETNULL(team_table);
	ASSERT_RETNULL(teamidx < team_table->m_nTeamCount);
	ASSERT_RETNULL(team_table->m_Teams);

	TEAM * team = team_table->m_Teams[teamidx];
	ASSERT_RETNULL(team);
	ASSERT_RETNULL(team->m_nTeamId == teamidx);

	return team;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamClearData(
	GAME * game,
	TEAM * team)
{
	ASSERT_RETURN(game && team);

	ASSERT_RETURN(team->m_nTeamId != INVALID_TEAM);

	if (team->m_nTeamId >= NUM_DEFAULT_TEAMS)
	{
		if (team->m_szTeamName)
		{
			GFREE(game, team->m_szTeamName);
			team->m_szTeamName = NULL;
		}
		team->m_nBaseTeam = INVALID_TEAM;
	}
	else
	{
		team->m_nBaseTeam = team->m_nTeamId;		// base team == team id in this case
	}

	if (team->m_pbRelations)
	{
		GFREE(game, team->m_pbRelations);
		team->m_pbRelations = NULL;
	}
	team->m_nRelations = 0;

	if (team->m_TeamStats)
	{
		StatsListFree(game, team->m_TeamStats);
		team->m_TeamStats = NULL;
	}

	team->m_nUnitCount = 0;
	team->m_nPlayerCount = 0;

	team->m_bActive = FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTeamSetData(
	GAME * game,
	TEAM_TABLE * team_table,
	TEAM * team,
	unsigned int teamidx,
	const WCHAR * team_name,
	BOOL bDeactivateEmpty)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(team);
	ASSERT_RETFALSE(team->m_nTeamId == teamidx);

	sTeamClearData(game, team);

	team->m_nRelations = team_table->m_nTeamCount;
	team->m_pbRelations = (BYTE *)GREALLOCZ(game, team->m_pbRelations, sizeof(BYTE) * team->m_nRelations);
	for (unsigned int ii = 0; ii < team->m_nRelations; ++ii)
	{
		team->m_pbRelations[ii] = RELATION_NEUTRAL;
	}

	// set name of new team
	if (teamidx < NUM_DEFAULT_TEAMS)
	{
		team->m_szTeamName = g_szDefaultTeamNames[teamidx];
	}
	else if (team_name && team_name[0])
	{
		unsigned int team_name_len = PStrLen(team_name, MAX_TEAM_NAME);
		team->m_szTeamName = (WCHAR *)GMALLOC(game, sizeof(WCHAR) * (team_name_len + 1));
		PStrCopy(team->m_szTeamName, team_name, team_name_len + 1);
	}

	team->m_bActive = TRUE;
	team->m_bDeactivateEmpty = bDeactivateEmpty;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TEAM * sTeamAdd(
	GAME * game,
	TEAM_TABLE * team_table,
	const WCHAR * team_name,
	BOOL bDeactivateEmpty)
{
	ASSERT_RETNULL(game && team_table);
	ASSERT_RETNULL(team_table->m_nTeamCount < MAX_TEAMS_PER_GAME);
	ASSERT_RETNULL(ValidateTeamTable(team_table));

	unsigned int nOldTeamCount = team_table->m_nTeamCount;
	unsigned int nNewTeamIndex = team_table->m_nTeamCount;
	++team_table->m_nTeamCount;

	// first expand all existing teams' relations table
	for (unsigned int ii = 0; ii < nOldTeamCount; ++ii)
	{
		TEAM * team = sTeamGetByIndex(team_table, ii);
		ASSERT_CONTINUE(team);
		team->m_pbRelations = (BYTE *)GREALLOCZ(game, team->m_pbRelations, sizeof(BYTE) * (team_table->m_nTeamCount));
		for (unsigned int jj = team->m_nRelations; jj < team_table->m_nTeamCount; ++jj)
		{
			team->m_pbRelations[jj] = RELATION_NEUTRAL;
		}
		team->m_nRelations = team_table->m_nTeamCount;
	}

	team_table->m_nTeamBits = BIT_SIZE(team_table->m_nTeamCount) + 1; // one extra for sign bit
	team_table->m_Teams = (TEAM **)GREALLOCZ(game, team_table->m_Teams, sizeof(TEAM *) * team_table->m_nTeamCount);
	ASSERT_RETNULL(team_table->m_Teams);

	team_table->m_Teams[nNewTeamIndex] = (TEAM *)GMALLOCZ(game, sizeof(TEAM));
	ASSERT_RETNULL(team_table->m_Teams[nNewTeamIndex]);
	team_table->m_Teams[nNewTeamIndex]->m_nTeamId = nNewTeamIndex;

	TEAM * team = sTeamGetByIndex(team_table, nNewTeamIndex);
	ASSERT_RETNULL(team);

	sTeamSetData(game, team_table, team, nNewTeamIndex, team_name, bDeactivateEmpty);

	ASSERT_RETNULL(ValidateTeamTable(team_table));

	return team;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TEAM * sTeamInit(
	GAME * game,
	TEAM_TABLE * team_table,
	unsigned int teamidx,
	const WCHAR * team_name,
	BOOL bDeactivateEmpty)
{
	ASSERT_RETNULL(game && team_table);
	ASSERT_RETNULL(teamidx < MAX_TEAMS_PER_GAME);
	ASSERT_RETNULL(ValidateTeamTable(team_table));

	// fill in any unspecified teams
	while (teamidx > team_table->m_nTeamCount)
	{
		sTeamAdd(game, team_table, NULL, bDeactivateEmpty);
	}

	TEAM * team = NULL;
	if (teamidx == team_table->m_nTeamCount)
	{
		team = sTeamAdd(game, team_table, team_name, bDeactivateEmpty);
		ASSERT_RETNULL(team && team->m_nTeamId == teamidx);
		team->m_bActive = TRUE;
		team->m_bDeactivateEmpty = bDeactivateEmpty;
	}
	else
	{
		team = sTeamGetByIndex(team_table, teamidx);
		ASSERT_RETNULL(team);
		sTeamSetData(game, team_table, team, teamidx, team_name, bDeactivateEmpty);
	}

	ASSERT_RETNULL(ValidateTeamTable(team_table));

	return team;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamSetRelation(
	GAME * game,
	TEAM_TABLE * team_table,
	TEAM * team,
	unsigned int team2,
	unsigned int relation)
{
	ASSERT_RETURN(game && team_table && team);
	ASSERT_RETURN(ValidateTeamTable(team_table));
	ASSERT_RETURN(relation < NUM_RELATIONS);
	ASSERT_RETURN(team2 < team_table->m_nTeamCount);
	ASSERT_RETURN(team2 < team->m_nRelations);

	team->m_pbRelations[team2] = (BYTE)relation;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamSetRelation(
	GAME * game,
	TEAM_TABLE * team_table,
	unsigned int team1idx,
	unsigned int team2idx,
	unsigned int relation)
{
	ASSERT_RETURN(game && team_table);

	ASSERT_RETURN(team2idx < team_table->m_nTeamCount);

	TEAM * team1 = sTeamGetByIndex(team_table, team1idx);
	ASSERT_RETURN(team1);

	sTeamSetRelation(game, team_table, team1, team2idx, relation);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamSetRelationReflexive(
	GAME * game,
	TEAM_TABLE * team_table,
	unsigned int team1idx,
	unsigned int team2idx,
	unsigned int relation)
{
	sTeamSetRelation(game, team_table, team1idx, team2idx, relation);
	sTeamSetRelation(game, team_table, team2idx, team1idx, relation);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sTeamGetRelation(
	TEAM_TABLE * team_table,
	TEAM * team1,
	unsigned int team2)
{
	ASSERT_RETVAL(team_table, RELATION_DEFAULT);
	
	if (team1->m_nRelations <= team2)
	{
		return RELATION_DEFAULT;
	}
	return team1->m_pbRelations[team2];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sTeamGetRelation(
	TEAM_TABLE * team_table,
	unsigned int team1,
	unsigned int team2)
{
	ASSERT_RETVAL(team_table, RELATION_DEFAULT);
	
	TEAM * team = sTeamGetByIndex(team_table, team1);
	ASSERT_RETVAL(team, RELATION_DEFAULT);

	return sTeamGetRelation(team_table, team, team2);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTeamsInit(
	GAME * game)
{
	static const BYTE InitialRelations[] =
	{// Bad					Neutral				Player Start		Player PvP FFA		Destructible
		RELATION_FRIENDLY,	RELATION_NEUTRAL,	RELATION_HOSTILE,	RELATION_HOSTILE,	RELATION_HOSTILE,	// Bad
		RELATION_NEUTRAL,	RELATION_NEUTRAL,	RELATION_NEUTRAL,	RELATION_NEUTRAL,	RELATION_HOSTILE,	// Neutral
		RELATION_HOSTILE,	RELATION_NEUTRAL,	RELATION_FRIENDLY,	RELATION_FRIENDLY,	RELATION_HOSTILE,	// Player Start
		RELATION_HOSTILE,	RELATION_NEUTRAL,	RELATION_FRIENDLY,	RELATION_FRIENDLY,	RELATION_HOSTILE,	// Player PvP FFA
		RELATION_HOSTILE,	RELATION_NEUTRAL,	RELATION_HOSTILE,	RELATION_HOSTILE,	RELATION_HOSTILE,	// Destructible
	};

	ASSERT(arrsize(g_szDefaultTeamNames) == NUM_DEFAULT_TEAMS);
	ASSERT(arrsize(InitialRelations) == NUM_DEFAULT_TEAMS * NUM_DEFAULT_TEAMS);

	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(!game->m_pTeams);
	TEAM_TABLE * team_table = game->m_pTeams = (TEAM_TABLE *)GMALLOCZ(game, sizeof(TEAM_TABLE));

	for (unsigned int ii = 0; ii < NUM_DEFAULT_TEAMS; ++ii)
	{
		TEAM * team = sTeamInit(game, team_table, ii, NULL, FALSE);
		ASSERT_RETFALSE(team);

		for (unsigned int jj = 0; jj <= ii; ++jj)
		{
			sTeamSetRelation(game, team_table, ii, jj, InitialRelations[ii * NUM_DEFAULT_TEAMS + jj]);
			sTeamSetRelation(game, team_table, jj, ii, InitialRelations[jj * NUM_DEFAULT_TEAMS + ii]);
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// template_team_index should be base team type to base relations on, and
//	should be < NUM_DEFAULT_TEAMS
// relation_to_type is the relation the team will have to all other teams
//	with the same base type (except for the actual base team)
//----------------------------------------------------------------------------
static void sTeamSetupRelations(
	GAME * game,
	TEAM_TABLE * team_table,
	TEAM * team,
	TEAM * template_team,
	unsigned int relation_to_type)
{
	ASSERT_RETURN(game && team_table);
	ASSERT_RETURN(team && template_team);

	unsigned int new_team_index = team->m_nTeamId;
	unsigned int template_team_index = template_team->m_nTeamId;
	ASSERT_RETURN(template_team_index < NUM_DEFAULT_TEAMS);
	ASSERT_RETURN(new_team_index != template_team_index);

	unsigned int team_count = team_table->m_nTeamCount;
	for (unsigned int ii = 0; ii < team_count; ++ii)
	{
		if (ii == new_team_index)
		{
			continue;
		}

		TEAM * other_team = sTeamGetByIndex(team_table, ii);
		if (!other_team || !other_team->m_bActive)
		{
			continue;
		}

		if (other_team->m_nBaseTeam == team->m_nBaseTeam && ii >= NUM_DEFAULT_TEAMS)
		{
			sTeamSetRelationReflexive(game, team_table, new_team_index, ii, relation_to_type);
		}
		else
		{
			unsigned int relation = sTeamGetRelation(team_table, template_team, ii);
			sTeamSetRelationReflexive(game, team_table, new_team_index, ii, relation);
		}
	}

	// relation to myself
	sTeamSetRelation(game, team_table, new_team_index, new_team_index, RELATION_FRIENDLY);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamsFree(
	GAME * game)
{
	ASSERT_RETURN(game)
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETURN(team_table);

	for (unsigned int ii = 0; ii < team_table->m_nTeamCount; ++ii)
	{
		TEAM * team = team_table->m_Teams[ii];
		if (team == NULL)
		{
			continue;
		}
		sTeamClearData(game, team);
		GFREE(game, team);
		team_table->m_Teams[ii] = NULL;
	}

	if (team_table->m_Teams)
	{
		GFREE(game, team_table->m_Teams);
	}
	GFREE(game, team_table);
	game->m_pTeams = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamMakeDeactivateMessage(
	MSG_SCMD_TEAM_DEACTIVATE & msg,
	TEAM * team)
{
	ASSERT_RETURN(team);
	msg.idTeam = team->m_nTeamId;
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamMakeUpdateMessage(
	MSG_SCMD_TEAM_NEW & msg,
	TEAM * team)
{
	ASSERT_RETURN(team);
	ASSERT_RETURN(team->m_nTeamId < MAX_TEAMS_PER_GAME);
	msg.idTeam = team->m_nTeamId;
	msg.idTemplateTeam = (BYTE)team->m_nBaseTeam;
	msg.bDeactivateEmpty = (BYTE)team->m_bDeactivateEmpty;
	if (team->m_szTeamName)
	{
		PStrCopy(msg.szTeamName, team->m_szTeamName, MAX_TEAM_NAME);
	}
	else
	{
		PStrCopy(msg.szTeamName, L"", MAX_TEAM_NAME);
	}
	MemoryCopy(msg.buf, sizeof(BYTE) * MAX_TEAMS_PER_GAME, team->m_pbRelations, sizeof(BYTE) * team->m_nRelations);
	MSG_SET_BLOB_LEN(msg, buf, team->m_nRelations);
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamSendToClient(
	GAME * game,
	TEAM * team,
	GAMECLIENT * client)
{
	ASSERT_RETURN(game && IS_SERVER(game));
	ASSERT_RETURN(team);

	if (!team->m_bActive)
	{
		MSG_SCMD_TEAM_DEACTIVATE msg;
		sTeamMakeDeactivateMessage(msg, team);
		s_SendMessage(game, ClientGetId(client), SCMD_TEAM_DEACTIVATE, &msg);
	}
	else
	{
		MSG_SCMD_TEAM_NEW msg;
		sTeamMakeUpdateMessage(msg, team);
		s_SendMessage(game, ClientGetId(client), SCMD_TEAM_NEW, &msg);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamSendToAllClients(
	GAME * game,
	TEAM * team)
{
	ASSERT_RETURN(game && IS_SERVER(game));
	ASSERT_RETURN(team);

	if (!team->m_bActive)
	{
		MSG_SCMD_TEAM_DEACTIVATE msg;
		sTeamMakeDeactivateMessage(msg, team);
		s_SendMessageToAll(game, SCMD_TEAM_DEACTIVATE, &msg);
	}
	else
	{
		MSG_SCMD_TEAM_NEW msg;
		sTeamMakeUpdateMessage(msg, team);
		s_SendMessageToAll(game, SCMD_TEAM_NEW, &msg);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamsSendToAllClients(
	GAME * game)
{
	ASSERT_RETURN(game && IS_SERVER(game));
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETURN(team_table);

	unsigned int team_count = team_table->m_nTeamCount;
	for (unsigned int ii = NUM_DEFAULT_TEAMS; ii < team_count; ++ii)
	{
		TEAM * team = sTeamGetByIndex(team_table, ii);
		if (!team)
		{
			continue;
		}
		sTeamSendToAllClients(game, team);		
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamsTrace(
	GAME * game)
{
#if ISVERSION(DEVELOPMENT) && defined(TEAM_TRACE)
	ASSERT_RETURN(game)
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETURN(team_table);
	ASSERT_RETURN(ValidateTeamTable(team_table));

	// print titles
	trace("\n## %-32s", "team");
	for (unsigned int ii = 0; ii < team_table->m_nTeamCount; ++ii)
	{
		TEAM * team = sTeamGetByIndex(team_table, ii);
		if (!team)
		{
			continue;
		}
		CHAR name[MAX_TEAM_NAME];
		if (team->m_szTeamName)
		{
			PStrCvt(name, team->m_szTeamName, MAX_TEAM_NAME);
		}
		else
		{
			PStrCvt(name, TEAM_UNKNOWN_NAME, MAX_TEAM_NAME);
		}
		trace("%-32s", name);
	}
	trace("\n");

	// print each team
	for (unsigned int ii = 0; ii < team_table->m_nTeamCount; ++ii)
	{
		TEAM * team = sTeamGetByIndex(team_table, ii);
		if (!team)
		{
			continue;
		}
		CHAR name[MAX_TEAM_NAME];
		if (team->m_szTeamName)
		{
			PStrCvt(name, team->m_szTeamName, MAX_TEAM_NAME);
		}
		else
		{
			PStrCvt(name, TEAM_UNKNOWN_NAME, MAX_TEAM_NAME);
		}
		trace("%2d %-32s", ii, name);

		// print relations to other teams
		for (unsigned int jj = 0; jj < team_table->m_nTeamCount; ++jj)
		{
			TEAM * other_team = sTeamGetByIndex(team_table, jj);
			if (!other_team)
			{
				continue;
			}
			unsigned int relation = sTeamGetRelation(team_table, team, jj);
			switch (relation)
			{
			case RELATION_NEUTRAL:
				trace("%-32s", "neutral");
				break;
			case RELATION_FRIENDLY:
				trace("%-32s", "friendly");
				break;
			case RELATION_HOSTILE:
				trace("%-32s", "hostile");
				break;
			default:
				trace("%-32s", "unknown");
				break;
			}
		}
		trace("\n");
	}
#else
	return;
#endif
}


//----------------------------------------------------------------------------
// remove a team
//----------------------------------------------------------------------------
static void sDeactivateTeam(
	GAME * game,
	TEAM * team)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(team);
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETURN(team_table);
	ASSERT_RETURN(ValidateTeamTable(team_table));

	if (team->m_nTeamId < NUM_DEFAULT_TEAMS)
	{
		return;
	}

	sTeamClearData(game, team);
	sTeamsTrace(game);

	if (IS_SERVER(game) && GameGetState(game) == GAMESTATE_RUNNING)
	{
		sTeamSendToAllClients(game, team);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sTeamDeactivate(
	GAME * game,
	unsigned int teamidx)
{
	ASSERT_RETFALSE(game && IS_CLIENT(game));
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETFALSE(team_table);

	TEAM * team = sTeamGetByIndex(team_table, teamidx);
	ASSERT_RETFALSE(team);

	sDeactivateTeam(game, team);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sTeamReceiveFromServer(
	GAME * game,
	MSG_SCMD_TEAM_NEW * msg)
{
	ASSERT_RETFALSE(game && IS_CLIENT(game));
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETFALSE(team_table);
	ASSERT_RETFALSE(msg && msg->hdr.cmd == SCMD_TEAM_NEW);

	unsigned int nRelations = MSG_GET_BLOB_LEN(msg, buf) / sizeof(BYTE);
	ASSERT_RETFALSE(nRelations < MAX_TEAMS_PER_GAME);

	// fill in any unspecified teams
	while (team_table->m_nTeamCount < nRelations)
	{
		sTeamAdd(game, team_table, NULL, msg->bDeactivateEmpty);
	}

	ASSERT_RETFALSE(msg->idTemplateTeam < NUM_DEFAULT_TEAMS);
	TEAM * template_team = sTeamGetByIndex(team_table, msg->idTemplateTeam);
	ASSERT_RETFALSE(template_team);

	TEAM * team = sTeamInit(game, team_table, msg->idTeam, msg->szTeamName, msg->bDeactivateEmpty);
	ASSERT_RETFALSE(team);
	team->m_nBaseTeam = msg->idTemplateTeam;
	team->m_nColorIndex = msg->ColorIndex;

	ASSERT_RETFALSE(team->m_nRelations >= nRelations);
	MemoryCopy(team->m_pbRelations, sizeof(BYTE) * team->m_nRelations, msg->buf, sizeof(BYTE) * nRelations);

	sTeamsTrace(game);

	return TRUE;
}


//----------------------------------------------------------------------------
// remove a unit from a team 
//----------------------------------------------------------------------------
static void sTeamRemoveUnit(
	GAME * game,
	TEAM_TABLE * team_table,
	UNIT * unit)
{
	ASSERT_RETURN(game && team_table);
	ASSERT_RETURN(unit);

	if (unit->nTeam == INVALID_TEAM)
	{
		return;
	}

	ASSERT_RETURN(unit->nTeam < (int)team_table->m_nTeamCount);

	TEAM * team = sTeamGetByIndex(team_table, unit->nTeam);
	ASSERT_RETURN(team);

	if (IS_SERVER(game))
	{
		if (UnitIsA(unit, UNITTYPE_PLAYER))
		{
			ASSERT_RETURN(team->m_nPlayerCount > 0);
			if (team->m_nPlayerCount > 0)
			{
				--team->m_nPlayerCount;
			}
		}
		ASSERT(team->m_nUnitCount > 0);
		if (team->m_nUnitCount > 0)
		{
			--team->m_nUnitCount;
			if (team->m_nUnitCount == 0 &&
				team->m_bDeactivateEmpty)
			{
				sDeactivateTeam(game, team);
			}
		}
	}

	UnitSetTeam(unit, INVALID_TEAM, FALSE); // don't validate an INVALID_TEAM, this will always assert. The validation is meant for catching invalid team when not intended - cmarch
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTeamMakeJoinMessage(
	MSG_SCMD_TEAM_JOIN & msg,
	UNIT * unit,
	unsigned int teamid)
{
	msg.idUnit = UnitGetId(unit);
	msg.idTeam = teamid;
}


//----------------------------------------------------------------------------
// send to all clients that know the unit that it's joining a team
//----------------------------------------------------------------------------
static void sTeamSendJoin(
	GAME * game,
	UNIT * unit,
	unsigned int teamid)
{
	ASSERT_RETURN(game && IS_SERVER(game));
	ASSERT_RETURN(unit);

	MSG_SCMD_TEAM_JOIN msg;
	sTeamMakeJoinMessage(msg, unit, teamid);
	s_SendUnitMessage(unit, SCMD_TEAM_JOIN, &msg);
}


//----------------------------------------------------------------------------
// send to client that unit is joining a team
//----------------------------------------------------------------------------
static void sTeamSendJoin(
	GAME * game,
	GAMECLIENTID idClient,
	UNIT * unit,
	unsigned int teamid)
{
	ASSERT_RETURN(game && IS_SERVER(game));
	ASSERT_RETURN(unit);

	MSG_SCMD_TEAM_JOIN msg;
	sTeamMakeJoinMessage(msg, unit, teamid);
	s_SendUnitMessageToClient(unit, idClient, SCMD_TEAM_JOIN, &msg);
}


//----------------------------------------------------------------------------
// add a unit to a team
//----------------------------------------------------------------------------
static BOOL sTeamAddUnit(
	GAME * game,
	TEAM_TABLE * team_table,
	UNIT * unit,
	unsigned int team_index,
	BOOL bSend = TRUE)
{
	ASSERT_RETFALSE(game && team_table);
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(team_index < team_table->m_nTeamCount);

	if ((unsigned int)unit->nTeam == team_index)
	{
		return FALSE;
	}

	sTeamRemoveUnit(game, team_table, unit);

	ASSERT_RETFALSE(unit->nTeam == INVALID_TEAM);

	TEAM * team = sTeamGetByIndex(team_table, team_index);
	ASSERT_RETFALSE(team);
	ASSERT_RETFALSE(team->m_bActive);

	if (IS_SERVER(game))
	{
		++team->m_nUnitCount;
		if (UnitIsA(unit, UNITTYPE_PLAYER))
		{
			++team->m_nPlayerCount;
		}
	}

	UnitSetTeam(unit, team_index);

	if (IS_SERVER(game) && bSend)
	{
		sTeamSendJoin(game, unit, team_index);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TeamsInit(
	GAME * game)
{
	sTeamsInit(game);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TeamsFree(
	GAME * game)
{
	sTeamsFree(game);
}


//----------------------------------------------------------------------------
// get team index by name
//----------------------------------------------------------------------------
int TeamGetByName(
	GAME * game,
	const WCHAR * name)
{
	ASSERT_RETVAL(game, INVALID_TEAM);
	ASSERT_RETVAL(name && name[0], INVALID_TEAM);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETVAL(team_table, INVALID_TEAM);

	for (unsigned int ii = NUM_DEFAULT_TEAMS; ii < team_table->m_nTeamCount; ++ii)
	{
		TEAM * team = team_table->m_Teams[ii];
		if (!team || !team->m_szTeamName)
		{
			continue;
		}
		if (PStrICmp(name, team->m_szTeamName, MAX_TEAM_NAME) == 0)
		{
			return ii;
		}
	}
	return INVALID_TEAM;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * TeamGetName(
	GAME * game,
	unsigned int teamidx)
{
	ASSERT_RETNULL(game);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETNULL(team_table);

	TEAM * team = sTeamGetByIndex(team_table, teamidx);
	ASSERT_RETNULL(team);

	return team->m_szTeamName;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int TeamGetColorIndex(
	GAME * game,
	unsigned int teamidx)
{
	ASSERT_RETINVALID(game);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETINVALID(team_table);

	TEAM * team = sTeamGetByIndex(team_table, teamidx);
	ASSERT_RETINVALID(team);

	return team->m_nColorIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int TeamsGetTeamCount(
	GAME * game)
{
	ASSERT_RETZERO(game);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETZERO(team_table);
	ASSERT_RETZERO(team_table->m_nTeamCount >= NUM_DEFAULT_TEAMS);

	return team_table->m_nTeamCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int TeamGetBaseType(
	GAME * game,
	unsigned int teamidx)
{
	ASSERT_RETVAL(game, INVALID_TEAM);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETVAL(team_table, INVALID_TEAM);

	TEAM * team = sTeamGetByIndex(team_table, teamidx);
	ASSERT_RETVAL(team, INVALID_TEAM);

	return team->m_nBaseTeam;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int TeamGetPlayerCount(
	GAME * game,
	unsigned int teamidx)
{
	ASSERT_RETVAL(game, INVALID_TEAM);
	ASSERT_RETVAL(IS_SERVER(game), INVALID_TEAM);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETVAL(team_table, INVALID_TEAM);

	TEAM * team = sTeamGetByIndex(team_table, teamidx);
	ASSERT_RETVAL(team, INVALID_TEAM);

	return team->m_nPlayerCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STATS * TeamGetStatsList(
	GAME * game,
	unsigned int teamidx)
{
	ASSERT_RETNULL(game);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETNULL(team_table);

	TEAM * team = sTeamGetByIndex(team_table, teamidx);
	ASSERT_RETNULL(team);

	if (!team->m_TeamStats)
	{
		team->m_TeamStats = StatsListInit(game);
	}

	return team->m_TeamStats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int TeamsGetRelation(
	GAME * game,
	unsigned int attacker_team,
	unsigned int defender_team)
{
	ASSERT_RETVAL(game, RELATION_DEFAULT);
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETVAL(team_table, RELATION_DEFAULT);

	return sTeamGetRelation(team_table, attacker_team, defender_team);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetDefaultTeam(
	GAME * game,
	GENUS genus,
	int nClass,
	const UNIT_DATA * unit_data /* = NULL */)
{
	switch (genus)
	{
	case GENUS_PLAYER:
		{
			return TEAM_PLAYER_START;
		}
	case GENUS_MONSTER:
		{
			if (!unit_data)
			{
				unit_data = MonsterGetData(game, nClass);
			}
			ASSERT_RETVAL(unit_data, TEAM_BAD);

			if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_DESTRUCTIBLE))
			{
				return TEAM_DESTRUCTIBLE;
			}
			if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_IS_GOOD))
			{
				return TEAM_PLAYER_START;
			}
			return TEAM_BAD;
		}
	case GENUS_ITEM:
		return INVALID_TEAM;
	case GENUS_OBJECT:
		{
			if (!unit_data)
			{
				unit_data = ObjectGetData(game, nClass);
			}
			ASSERT_RETVAL(unit_data, TEAM_NEUTRAL);

			if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_DESTRUCTIBLE))
			{
				return TEAM_DESTRUCTIBLE;
			}
			return TEAM_NEUTRAL;
		}
	default:
		return TEAM_NEUTRAL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetDefaultTeam(
	GAME * game,
	UNIT * unit)
{
	return UnitGetDefaultTeam(game, UnitGetGenus(unit), UnitGetClass(unit), UnitGetData(unit));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InAggressionList(
	GAME * game,
	UNIT * attacker,
	UNIT * defender)
{
	if( AppIsHellgate() )
	{
		return FALSE;
	}
	attacker = UnitGetResponsibleUnit( attacker );
	defender = UnitGetResponsibleUnit( defender );
	if( PetIsPet( attacker ) )
	{
		attacker = UnitGetById( game, PetGetOwner( attacker ) );
	}
	if( PetIsPet( defender ) )
	{
		defender = UnitGetById( game, PetGetOwner( defender ) );
	}
	if( defender && attacker &&
		UnitIsA( attacker, UNITTYPE_PLAYER ) && 
		UnitIsA( defender, UNITTYPE_PLAYER ) )
	{
		int attackerID = UnitGetId( attacker );
		int defenderID = UnitGetId( defender );
		int nAggressors = UnitGetStat( attacker, STATS_AGGRESSION_LIST_COUNT );
		for( int i = 0; i < nAggressors; i++ )
		{
			if( UnitGetStat( attacker, STATS_AGGRESSION_LIST_ID, i ) == defenderID )
			{
				return TRUE;
			}
		}
		nAggressors = UnitGetStat( defender, STATS_AGGRESSION_LIST_COUNT );
		for( int i = 0; i < nAggressors; i++ )
		{
			if( UnitGetStat( defender, STATS_AGGRESSION_LIST_ID, i ) == attackerID )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InHonorableAggression(
					  GAME * game,
					  UNIT * attacker,
					  UNIT * defender)
{
	if( AppIsHellgate() )
	{
		return FALSE;
	}
	attacker = UnitGetResponsibleUnit( attacker );
	defender = UnitGetResponsibleUnit( defender );
	if( PetIsPet( attacker ) )
	{
		attacker = UnitGetById( game, PetGetOwner( attacker ) );
	}
	if( PetIsPet( defender ) )
	{
		defender = UnitGetById( game, PetGetOwner( defender ) );
	}
	if( defender && attacker &&
		UnitIsA( attacker, UNITTYPE_PLAYER ) && 
		UnitIsA( defender, UNITTYPE_PLAYER ) )
	{
		int defenderID = UnitGetId( defender );
		int nAggressors = UnitGetStat( attacker, STATS_AGGRESSION_LIST_COUNT );
		for( int i = 0; i < nAggressors; i++ )
		{
			if( UnitGetStat( attacker, STATS_AGGRESSION_LIST_ID, i ) == defenderID &&
				UnitGetStat( attacker, STATS_AGGRESSION_LIST_HONORABLE, i ) > 0 )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClearAggressionList( 
	UNIT *unit )
{
	ASSERT_RETURN( unit );
	UnitSetStat( unit, STATS_AGGRESSION_LIST_COUNT, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AddToAggressionList( 
	UNIT *unit,
	UNIT *target)
{
	ASSERT_RETURN( unit && target );

	if( UnitGetPartyId( unit ) != INVALID_ID &&
		UnitGetPartyId( unit ) == UnitGetPartyId( target ) )
	{
		return;
	}

	LEVEL* pLevel = UnitGetLevel( unit );
	if( LevelIsSafe( pLevel ) )
	{
		return;
	}

	BOOL bHonorable = UnitGetExperienceLevel( target ) < UnitGetExperienceLevel( unit ) + 7;
	RemoveFromAggressionList( unit, target );
	int nCount = UnitGetStat( unit, STATS_AGGRESSION_LIST_COUNT, 0 );
	UnitSetStat( unit, STATS_AGGRESSION_LIST_ID, nCount, UnitGetId( target ) );
	UnitSetStat( unit, STATS_AGGRESSION_LIST_COUNT, nCount + 1 );
	UnitSetStat( unit, STATS_AGGRESSION_LIST_HONORABLE, nCount, bHonorable ? 1 : 0 );

	// this always cuts both ways
	RemoveFromAggressionList( target, unit );
	nCount = UnitGetStat( target, STATS_AGGRESSION_LIST_COUNT, 0 );
	UnitSetStat( target, STATS_AGGRESSION_LIST_ID, nCount, UnitGetId( unit ) );
	UnitSetStat( target, STATS_AGGRESSION_LIST_COUNT, nCount + 1 );
	UnitSetStat( target, STATS_AGGRESSION_LIST_HONORABLE, nCount, 1 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RemoveFromAggressionList( 
	UNIT *unit,
	UNIT *target)
{
	ASSERT_RETURN( unit && target );
	int nAggressors = UnitGetStat( unit, STATS_AGGRESSION_LIST_COUNT );
	int targetID = UnitGetId( target );
	for( int i = 0; i < nAggressors; i++ )
	{
		if( UnitGetStat( unit, STATS_AGGRESSION_LIST_ID, i ) == targetID )
		{
			int nId = UnitGetStat( unit, STATS_AGGRESSION_LIST_ID, nAggressors - 1 );
			int nHonorable = UnitGetStat( unit, STATS_AGGRESSION_LIST_HONORABLE, nAggressors - 1 );
			UnitSetStat( unit, STATS_AGGRESSION_LIST_ID, i, nId );
			UnitSetStat( unit, STATS_AGGRESSION_LIST_HONORABLE, i, nHonorable );
			UnitSetStat( unit, STATS_AGGRESSION_LIST_COUNT, nAggressors - 1 );
			return;
		}
	}	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TeamValidateAttackerDefender(
	UNIT * attacker,
	UNIT * defender)
{
	ASSERT_RETFALSE(attacker);
	ASSERT_RETFALSE(defender);
	
	unsigned int attacker_team = UnitGetTeam(attacker);
	unsigned int defender_team = UnitGetTeam(defender);
	
#if ISVERSION(SERVER_VERSION)
	if (attacker_team == INVALID_TEAM || defender_team == INVALID_TEAM)
	{
		return FALSE;
	}
#else
	ASSERTV_RETFALSE(attacker_team != INVALID_TEAM && defender_team != INVALID_TEAM,
		"Team Validate Failed: Attacker '%s' [%d] Team=%d, Defender '%s' [%d] Team=%d",
		UnitGetDevName(attacker), UnitGetId(attacker), attacker_team,
		UnitGetDevName(defender), UnitGetId(defender), defender_team);
#endif
		
	return TRUE;
}


//----------------------------------------------------------------------------
// call this when a client joins a game, so they know about all the teams
//----------------------------------------------------------------------------
void TeamsSendToClient(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETURN(game && IS_SERVER(game));
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETURN(team_table);

	unsigned int team_count = team_table->m_nTeamCount;
	for (unsigned int ii = NUM_DEFAULT_TEAMS; ii < team_count; ++ii)
	{
		TEAM * team = sTeamGetByIndex(team_table, ii);
		if (!team || !team->m_bActive)
		{
			continue;
		}
		sTeamSendToClient(game, team, client);		
	}
}


//----------------------------------------------------------------------------
// return first unused team slot
//----------------------------------------------------------------------------
static unsigned int sGetFirstAvailableTeamIndex(
	TEAM_TABLE * team_table)
{
	for (unsigned int ii = 0; ii < team_table->m_nTeamCount; ++ii)
	{
		TEAM * team = team_table->m_Teams[ii];
		if (team == NULL || !team->m_bActive)
		{
			return ii;
		}
	}
	return team_table->m_nTeamCount;
}


//----------------------------------------------------------------------------
// add a new team, copying the relations of the template_team
//----------------------------------------------------------------------------
unsigned int TeamsAdd(
	GAME * game,
	const WCHAR * team_name,
	const char * pszColors,
	unsigned int template_team_index,
	unsigned int relation_to_type,
	BOOL bDeactivateEmpty)
{
	ASSERT_RETINVALID(game);
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETINVALID(team_table);
	ASSERT_RETINVALID(template_team_index < NUM_DEFAULT_TEAMS);
	ASSERT_RETINVALID(template_team_index < team_table->m_nTeamCount);
	
	unsigned int new_team_index = sGetFirstAvailableTeamIndex(team_table);
	ASSERT_RETINVALID(new_team_index < MAX_TEAMS_PER_GAME);

	TEAM * template_team = sTeamGetByIndex(team_table, template_team_index);
	ASSERT_RETINVALID(template_team);

	TEAM * team = sTeamInit(game, team_table, new_team_index, team_name, bDeactivateEmpty);
	ASSERT_RETINVALID(team);
	team->m_nBaseTeam = template_team_index;
	team->m_nColorIndex = 0; // ColorSetBlahBlahBlah

	sTeamSetupRelations(game, team_table, team, template_team, relation_to_type);

	if (IS_SERVER(game) && GameGetState(game) == GAMESTATE_RUNNING)
	{
		// send all teams to all clients, since reflexive relations have changed?
		sTeamsSendToAllClients(game);
	}

	return new_team_index;
}


//----------------------------------------------------------------------------
// add a unit to a team
//----------------------------------------------------------------------------
BOOL TeamAddUnit(
	UNIT * unit,
	unsigned int team,
	BOOL bSend)				// true
{
	ASSERT_RETFALSE(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETFALSE(team_table);

	return sTeamAddUnit(game, team_table, unit, team, bSend);
}


//----------------------------------------------------------------------------
// remove a unit from a team
//----------------------------------------------------------------------------
void TeamRemoveUnit(
	UNIT * unit)
{
	ASSERT_RETURN(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETURN(team_table);

	int nTeamRemovedFrom = UnitGetTeam(unit);
	if (nTeamRemovedFrom == INVALID_TEAM)
		return;

	sTeamRemoveUnit(game, team_table, unit);

	if (IS_SERVER(game) && UnitIsPlayer(unit))
	{
		s_MetagameEventRemoveFromTeam(unit, nTeamRemovedFrom);
	}
}


//----------------------------------------------------------------------------
// send this message for new non-default team units (pvp players & pets)
// to inform client of that unit's team affiliation
// note - this shouldn't be needed since the unit's team is xfered
//----------------------------------------------------------------------------
void s_SendTeamJoin(
	GAME * game,
	GAMECLIENTID idClient,
	UNIT * unit,
	unsigned int team)
{
	// sTeamSendJoin(game, idClient, unit, team);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TeamReceiveFromServer(
	GAME * game,
	MSG_SCMD_TEAM_NEW * msg)
{
	return c_sTeamReceiveFromServer(game, msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TeamDeactivateReceiveFromServer(
	GAME * game,
	MSG_SCMD_TEAM_DEACTIVATE * msg)
{
	ASSERT_RETFALSE(game && IS_CLIENT(game));
	ASSERT_RETFALSE(msg && msg->hdr.cmd == SCMD_TEAM_DEACTIVATE);

	c_sTeamDeactivate(game, msg->idTeam);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TeamJoinReceiveFromServer(
	GAME * game,
	struct MSG_SCMD_TEAM_JOIN * msg)
{
	ASSERT_RETFALSE(game && IS_CLIENT(game));
	TEAM_TABLE * team_table = game->m_pTeams;
	ASSERT_RETFALSE(team_table);
	ASSERT_RETFALSE(msg && msg->hdr.cmd == SCMD_TEAM_JOIN);

	UNIT * unit = UnitGetById(game, msg->idUnit);
	ASSERT_RETFALSE(unit);

	sTeamAddUnit(game, team_table, unit, msg->idTeam);

	return TRUE;
};


//----------------------------------------------------------------------------
// note that teams are never saved to disk, it's a run time construct
// the server and the connected clients, both of which have identical team
// structures setup in their game
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL TeamXfer(
	GAME * game,
	XFER<mode> & tXfer,
	UNIT * unit,
	GENUS eGenus,
	unsigned int nClass,
	int & nTeam)
{
	// items don't have teams
	if (eGenus == GENUS_ITEM)
	{
		return TRUE;
	}

	// is default team
	int nTeamDefault = UnitGetDefaultTeam(game, eGenus, nClass, NULL);
	BOOL bIsDefault = FALSE;
	if (tXfer.IsSave())
	{
		ASSERT_RETFALSE(unit);
		nTeam = UnitGetTeam(unit);
		if (nTeam == nTeamDefault)
		{
			bIsDefault = TRUE;
		}
	}
	
	// put is default into the stream
	XFER_BOOL(tXfer, bIsDefault);
	
	// do non default data
	if (bIsDefault == TRUE)
	{
		nTeam = nTeamDefault;
	}
	else
	{
		TEAM_TABLE * team_table = game->m_pTeams;
		ASSERT_RETFALSE(team_table);
		XFER_INT(tXfer, nTeam, team_table->m_nTeamBits);				
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitOverrideTeam(
	GAME * game,
	UNIT * unit,
	unsigned int team,
	TARGET_TYPE targettype)
{
	ASSERT_RETINVALID(game);
	ASSERT_RETINVALID(unit);
	ASSERT_RETINVALID(team != INVALID_TEAM);
	ASSERT_RETINVALID(targettype != TARGET_INVALID);
	TeamAddUnit(unit, team, FALSE);
	UnitChangeTargetType(unit, targettype);
	
	int nIndex = UnitGetStat(unit, STATS_TEAM_OVERRIDE_INDEX) + 1;
	UnitSetStat(unit, STATS_TEAM_OVERRIDE, nIndex, 0, team);
	UnitSetStat(unit, STATS_TEAM_OVERRIDE, nIndex, 1, targettype);
	UnitSetStat(unit, STATS_TEAM_OVERRIDE_INDEX, nIndex);
	return nIndex;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitCancelOverrideTeam(
	GAME * game,
	UNIT * unit,
	int index)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	
	UnitSetStat(unit, STATS_TEAM_OVERRIDE, index, 0, INVALID_TEAM);
	UnitSetStat(unit, STATS_TEAM_OVERRIDE, index, 1, TARGET_INVALID);

	int nOverrideIndex = UnitGetStat(unit, STATS_TEAM_OVERRIDE_INDEX);
	if(nOverrideIndex == index)
	{
		int nTeam = INVALID_TEAM;
		TARGET_TYPE eTargetType = TARGET_INVALID;
		do
		{
			nOverrideIndex--;
			if(nOverrideIndex >= 0)
			{
				nTeam = UnitGetStat(unit, STATS_TEAM_OVERRIDE, nOverrideIndex, 0);
				eTargetType = (TARGET_TYPE)UnitGetStat(unit, STATS_TEAM_OVERRIDE, nOverrideIndex, 1);
			}
		}
		while(nOverrideIndex >= 0 && (nTeam == INVALID_TEAM || eTargetType == TARGET_INVALID));
		UnitSetStat(unit, STATS_TEAM_OVERRIDE_INDEX, nOverrideIndex);

		if(nOverrideIndex >= 0 && (nTeam != INVALID_TEAM && eTargetType != TARGET_INVALID))
		{
			TeamAddUnit( unit, nTeam, FALSE);
			UnitChangeTargetType(unit, eTargetType);
		}
		else
		{
			TeamAddUnit( unit, UnitGetDefaultTeam(game, unit), FALSE);
			UnitSetDefaultTargetType(game, unit);
		}
	}
}

//----------------------------------------------------------------------------
// Play a sound on all the clients for a team
//----------------------------------------------------------------------------
void s_TeamPlaySound( 
	GAME *game,
	DWORD sound,
	unsigned int team)
{
	ASSERT_RETURN(game && sound != INVALID_LINK && team != INVALID_TEAM);
	ASSERT_RETURN(team < INT_MAX);

	for (GAMECLIENT *client = ClientGetFirstInGame( game );
		 client;
		 client = ClientGetNextInGame( client ))
	{
		UNIT *unit = ClientGetControlUnit( client );
		if (UnitGetTeam( unit ) == int(team))
		{
			MSG_SCMD_UNITPLAYSOUND msg;
			msg.id = UnitGetId(unit);
			msg.idSound = sound;
			s_SendMessage(game, ClientGetId(client), SCMD_UNITPLAYSOUND, &msg);
		}
	}
}
