//----------------------------------------------------------------------------
// party.h
//
// game-level party structure
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "player.h"

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct PARTY_LIST;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// data for the party (name, description, member count, loot & join rules, etc)
struct PARTY_DATA
{
	DWORD							m_dwFlags;
	// loot rules,
	// cached name, description, etc.
};

//----------------------------------------------------------------------------
// data for each party member (name, etc)
struct PARTY_MEMBER_DATA
{
	PGUID							m_Guid;
	WCHAR							m_strName[MAX_CHARACTER_NAME];		// name
	// appearance
	// class
	// level
	// alive vs. dead (status)
	// location (area + depth for tugboat, leveldef for hellgate)
	// town portal info
};

//----------------------------------------------------------------------------
// iterator 
struct PARTY_ITER
{
private:
	GAME *							m_Game;
	unsigned int					m_nPartyIndex;
	unsigned int					m_nMemberIndex;

public:
	PARTY_ITER(
		GAME * game,
		PGUID idParty);

	BOOL SetParty(
		PGUID idParty);

	void Release(
		void);

	unsigned int GetMemberCount(
		void);

	unsigned int GetInGameCount(
		void);

	BOOL GetFirst(
		PARTY_MEMBER_DATA & data);

	BOOL GetNext(
		PARTY_MEMBER_DATA & data);

	UNIT * GetFirstUnitInGame(
		void);

	UNIT * GetNextUnitInGame(
		void);
};

//----------------------------------------------------------------------------
// COMMON
//----------------------------------------------------------------------------

PARTY_LIST * PartyListInit(
	GAME * game);

void PartyListFree(
	GAME * game);

//----------------------------------------------------------------------------
// CLIENT
//----------------------------------------------------------------------------
	
void c_PartyMemberWarpTo(
	PGUID guidPartyMember);

//----------------------------------------------------------------------------
// SERVER
//----------------------------------------------------------------------------

PGUID s_UnitGetPartyGuid(
	GAME * game,
	UNIT * unit);
	
BOOL s_PartyIsInGame(
	GAME *pGame);

void s_PartyIterateCachedPartyMembersInGame(
	GAME *pGame,
	PARTYID idParty,
	PFN_PLAYER_ITERATE pfnCallback,
	void *pCallbackData);	

void s_PartyIterateCachedPartyMembersInGame(
	GAME *pGame,
	struct CachedParty *pParty,
	PFN_PLAYER_ITERATE pfnCallback,
	void *pCallbackData);	

int s_PartyGetMemberCount(
	GAME *pGame,
	PARTYID idParty);
		
	
	
	