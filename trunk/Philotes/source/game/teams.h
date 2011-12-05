//----------------------------------------------------------------------------
// teams.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _TEAMS_H_
#define _TEAMS_H_

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	TEAM_BAD,
	TEAM_NEUTRAL,
	TEAM_PLAYER_START,
	TEAM_PLAYER_PVP_FFA,
	TEAM_DESTRUCTIBLE,
	NUM_DEFAULT_TEAMS,
};

enum
{
	RELATION_NEUTRAL,
	RELATION_FRIENDLY,
	RELATION_HOSTILE,
	NUM_RELATIONS,
	RELATION_DEFAULT = RELATION_NEUTRAL,
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void TeamsInit(
	GAME * game);

void TeamsFree(
	GAME * game);

void s_SendTeamJoin( GAME * game,
					 GAMECLIENTID idClient,
					 UNIT * unit,
					 unsigned int team);

int UnitGetDefaultTeam(
	GAME * game,
	GENUS genus,
	int nClass,
	const struct UNIT_DATA * unit_data = NULL);

int UnitGetDefaultTeam(
	GAME * game,
	UNIT * unit);

int UnitOverrideTeam(
	GAME * game,
	UNIT * unit,
	unsigned int team,
	TARGET_TYPE targettype);

void UnitCancelOverrideTeam(
	GAME * game,
	UNIT * unit,
	int index);

int TeamsGetRelation(
	GAME * game,
	unsigned int attacker_team,
	unsigned int defender_team);

unsigned int TeamsAdd(
	GAME * game,
	const WCHAR * team_name,
	const char * pszColors,
	unsigned int template_team_index,
	unsigned int relation_to_type,
	BOOL bDeactivateEmpty=TRUE);

void TeamsSendToClient(
	GAME * game,
	GAMECLIENT * client);

BOOL TeamReceiveFromServer(
	GAME * game,
	struct MSG_SCMD_TEAM_NEW * msg);

BOOL TeamDeactivateReceiveFromServer(
	GAME * game,
	struct MSG_SCMD_TEAM_DEACTIVATE * msg);

BOOL TeamAddUnit(
	UNIT * unit,
	unsigned int team,
	BOOL bSend = TRUE);

void TeamRemoveUnit(
	UNIT * unit);

BOOL TeamJoinReceiveFromServer(
	GAME * game,
	struct MSG_SCMD_TEAM_JOIN * msg);

int TeamGetByName(
	GAME * game,
	const WCHAR * name);

const WCHAR * TeamGetName(
	GAME * game,
	unsigned int team);

template <XFER_MODE mode>
BOOL TeamXfer(
	GAME * game,
	XFER<mode> & tXer,
	UNIT * unit,
	GENUS eGenus,
	unsigned int nClass,
	int & nTeam);
		
template BOOL TeamXfer<XFER_SAVE>(GAME *, XFER<XFER_SAVE> &, UNIT * unit, GENUS, unsigned int, int &);
template BOOL TeamXfer<XFER_LOAD>(GAME *, XFER<XFER_LOAD> &, UNIT * unit, GENUS, unsigned int, int &);


unsigned int TeamsGetTeamCount(
	GAME * game);

int TeamGetBaseType(
	GAME * game,
	unsigned int team);

int TeamGetPlayerCount(
	GAME * game,
	unsigned int team);

struct STATS * TeamGetStatsList(
	GAME * game,
	unsigned int team);

int TeamGetColorIndex(
	GAME * game,
	unsigned int team);

BOOL TeamValidateAttackerDefender(
	UNIT *pAttacker,
	UNIT *pDefender);
	
BOOL InAggressionList(
	GAME * game,
	UNIT * attacker,
	UNIT * defender);

BOOL InHonorableAggression(
	GAME * game,
	UNIT * attacker,
	UNIT * defender);

void RemoveFromAggressionList( 
	UNIT *unit,
	UNIT *target);

void AddToAggressionList( 
	UNIT *unit,
	UNIT *target);

void ClearAggressionList( 
	UNIT *unit );

void s_TeamPlaySound( 
	GAME *game,
	DWORD sound,
	unsigned int team);

//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------



inline int UnitsGetTeamRelation(
	GAME * game,
	UNIT * attacker,
	UNIT * defender)
{
	ASSERT_RETFALSE(game && attacker && defender);
	AUTO_VERSION(
		ASSERT_RETFALSE( TeamValidateAttackerDefender( attacker, defender ) );,
		if (!TeamValidateAttackerDefender( attacker, defender )) return FALSE; )
	int nTeamAttacker = UnitGetTeam( attacker );
	int nTeamDefender = UnitGetTeam( defender );
	if( AppIsTugboat() && InAggressionList( game, attacker, defender ) )
	{
		return RELATION_HOSTILE;
	}
	return TeamsGetRelation(game, nTeamAttacker, nTeamDefender);
}


inline BOOL TestHostile(
	GAME * game,
	UNIT * attacker,
	UNIT * defender)
{
	return (UnitsGetTeamRelation( game, attacker, defender ) == RELATION_HOSTILE ||
		    InAggressionList( game, attacker, defender ));
}


inline BOOL TestFriendly(
	GAME * game,
	UNIT * attacker,
	UNIT * defender)
{
	return (UnitsGetTeamRelation( game, attacker, defender ) == RELATION_FRIENDLY &&
			!InAggressionList( game, attacker, defender));		
}


inline BOOL TestNeutral(
	GAME * game,
	UNIT * attacker,
	UNIT * defender)
{
	return (UnitsGetTeamRelation( game, attacker, defender ) == RELATION_NEUTRAL);		
}


#endif // _TEAMS_H_
