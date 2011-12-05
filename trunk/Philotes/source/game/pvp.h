//----------------------------------------------------------------------------
// pvp.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PVP_H_
#define _PVP_H_

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

enum
{
	PVP_MAX_TEAMS = 2,
	PVP_MAX_PLAYERS_PER_TEAM = 5,
};

enum CTF_FLAG_STATUS;

//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct PvPPlayerData
{
	BOOL bIsControlUnit;
	int nPlayerKills;		
	int nFlagCaptures;		 
	BOOL bCarryingFlag;		
	WCHAR szName[MAX_CHARACTER_NAME];
};

struct PvPTeamData
{
	PvPPlayerData tPlayerData[PVP_MAX_PLAYERS_PER_TEAM];
	int nNumPlayers;
	int nScore;
	CTF_FLAG_STATUS eFlagStatus;
	WCHAR szName[MAX_TEAM_NAME + 1];
};

struct PvPGameData
{
	PvPTeamData tTeamData[PVP_MAX_TEAMS];
	int nNumTeams;
	BOOL bHasTimelimit;
	int nSecondsRemainingInMatch;
};

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

BOOL c_PvPGetGameData(PvPGameData *pPvPGameData);

#endif // _PVP_H_
