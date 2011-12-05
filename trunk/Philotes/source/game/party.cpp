//----------------------------------------------------------------------------
// party.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// game-level party structure
//
// NOTE: it may be possible for us to receive messages from chat server
// out of order?  in which case we may get adds after removes, etc.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "game.h"
#include "gameglobals.h"
#include "units.h"
#include "party.h"
#include "uix_priv.h"
#include "c_message.h"
#include "warp.h"
#include "ServerRunnerAPI.h"
#include "s_partyCache.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define INVALID_PARTYINDEX			(unsigned int)(-1)
#define UNIT_MAP_SIZE				256									// size of hash for player id to party id

//----------------------------------------------------------------------------
// ENUM
//----------------------------------------------------------------------------
enum
{
	PARTY_FLAG_INGAME,													// party member is in this game (and has a unit)
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
// each party member
struct PARTY_MEMBER
{
	PGUID							m_Guid;								// guid of member
	UNIT *							m_Unit;								// unit (could be fake unit if member isn't in this game)
	DWORD							m_dwFlags;							// flags
	PARTY_MEMBER_DATA				m_Data;								// member data (name, appearance, etc.)
};

// party
struct PARTY
{
	PGUID							m_Guid;								// guid of party
	PARTY_MEMBER *					m_PartyMembers;						// sorted list of party members (by guid)
	unsigned int					m_nMemberCount;						// number of party members
	unsigned int					m_nInGameCount;						// number of members in game
	PARTY_DATA						m_Data;
};

// map player unit id to party id
struct UNIT_NODE
{
	UNITID							id;									// unit id for player unit (name for HASH template)
	UNIT_NODE *						next;								// next node in hash (name for HASH template)
	PGUID							m_guidParty;						// guid of party
};

// all parties in game (towns, arenas, special games for example have > 1 party)
struct PARTY_LIST
{
	PARTY *							m_Parties;							// sorted list of parties in the game (by guid)
	unsigned int					m_nPartyCount;						// number of parties

	HASH<UNIT_NODE, UNITID, UNIT_MAP_SIZE>  m_UnitHash;					// hash of units by unit id -> party id

	unsigned int					m_IterCount;						// we don't allow any add/remove operations unless itercount is 0
};


//----------------------------------------------------------------------------
// STATIC FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PARTY_LIST * sGetPartyList(
	GAME * game)
{
	ASSERT_RETNULL(game);
	return game->m_PartyList;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PARTY * sGetParty(
	PARTY_LIST * parties,
	unsigned int index)
{
	ASSERT_RETNULL(parties);
	ASSERT_RETNULL(index < parties->m_nPartyCount);
	return parties->m_Parties + index;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PARTY * sGetParty(
	GAME * game,
	unsigned int index)
{
	PARTY_LIST * parties = sGetPartyList(game);
	ASSERT_RETNULL(parties);

	PARTY * party = sGetParty(parties, index);
	ASSERT_RETNULL(party);
	return party;
}


//----------------------------------------------------------------------------
// find a party in PARTY_LIST by GUID, or insertion point, return index
//----------------------------------------------------------------------------
static unsigned int sFindParty(
	PARTY_LIST * parties,
	PGUID idParty)
{
	PARTY * min = parties->m_Parties;
	PARTY * max = min + parties->m_nPartyCount;
	PARTY * ii = min + (max - min) / 2;

	while (max > min)
	{
		if (ii->m_Guid > idParty)
		{
			max = ii;
		}
		else if (ii->m_Guid < idParty)
		{
			min = ii + 1;
		}
		else
		{
			return SIZE_TO_INT(ii - parties->m_Parties);
		}
		ii = min + (max - min) / 2;
	}
	return SIZE_TO_INT(max - parties->m_Parties);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PARTY * sGetPartyByGuid(
	PARTY_LIST * parties,
	PGUID idParty)
{
	unsigned int ii = sFindParty(parties, idParty);
	if (ii < parties->m_nPartyCount && parties->m_Parties[ii].m_Guid == idParty)
	{
		return parties->m_Parties + ii;
	}
	return NULL;
}


//----------------------------------------------------------------------------
// find a party member in party by GUID, or insertion point, return index
//----------------------------------------------------------------------------
static unsigned int sFindMember(
	PARTY * party,
	PGUID idMember)
{
	PARTY_MEMBER * min = party->m_PartyMembers;
	PARTY_MEMBER * max = min + party->m_nMemberCount;
	PARTY_MEMBER * ii = min + (max - min) / 2;

	while (max > min)
	{
		if (ii->m_Guid > idMember)
		{
			max = ii;
		}
		else if (ii->m_Guid < idMember)
		{
			min = ii + 1;
		}
		else
		{
			return SIZE_TO_INT(ii - party->m_PartyMembers);
		}
		ii = min + (max - min) / 2;
	}
	return SIZE_TO_INT(max - party->m_PartyMembers);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PARTY_MEMBER * sGetMember(
	PARTY * party,
	unsigned int index)
{
	ASSERT_RETNULL(party);
	ASSERT_RETNULL(index < party->m_nMemberCount);
	return party->m_PartyMembers + index;
}


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PARTY_LIST * PartyListInit(
	GAME * game)
{
	PartyListFree(game);

	game->m_PartyList = (PARTY_LIST *)GMALLOCZ(game, sizeof(PARTY_LIST));
	return game->m_PartyList;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PartyListFree(
	GAME * game)
{
	if (!game->m_PartyList)
	{
		return;
	}
	PARTY_LIST * parties = game->m_PartyList;
	if (parties->m_Parties)
	{
		GFREE(game, parties->m_Parties);
	}

	GFREE(game, game->m_PartyList);
	game->m_PartyList = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PartyListAddParty(
	GAME * game,
	PGUID idParty,
	PARTY_DATA & data)
{
	PARTY_LIST * parties = sGetPartyList(game);
	ASSERT_RETURN(parties);
	ASSERT_RETURN(parties->m_IterCount == 0);

	unsigned int ii = sFindParty(parties, idParty);
	if (ii >= parties->m_nPartyCount || parties->m_Parties[ii].m_Guid != idParty)
	{
		parties->m_Parties = (PARTY *)GREALLOC(game, parties->m_Parties, sizeof(PARTY) * (parties->m_nPartyCount + 1));

		if (ii < parties->m_nPartyCount)
		{
			MemoryMove(parties->m_Parties + ii + 1, (parties->m_nPartyCount - ii) * sizeof(PARTY), parties->m_Parties + ii, (parties->m_nPartyCount - ii) * sizeof(PARTY));
		}
		++parties->m_nPartyCount;
		parties->m_Parties[ii].m_Guid = idParty;
	}
	PARTY * party = parties->m_Parties + ii;
	party->m_Data = data;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PartyListUpdateParty(
	GAME * game,
	PGUID idParty,
	PARTY_DATA & data)
{
	PartyListAddParty(game, idParty, data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PartyListRemoveParty(
	GAME * game,
	PGUID idParty)
{
	PARTY_LIST * parties = sGetPartyList(game);
	ASSERT_RETURN(parties);
	ASSERT_RETURN(parties->m_IterCount == 0);

	unsigned int ii = sFindParty(parties, idParty);
	if (ii >= parties->m_nPartyCount || parties->m_Parties[ii].m_Guid != idParty)
	{
		return;
	}

	if (ii < parties->m_nPartyCount - 1)
	{
		MemoryMove(parties->m_Parties + ii, (parties->m_nPartyCount - ii) * sizeof(PARTY), parties->m_Parties + ii + 1, (parties->m_nPartyCount - ii - 1) * sizeof(PARTY));
	}
	--parties->m_nPartyCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PartyAddMember(
	GAME * game,
	PGUID idParty,
	PGUID idMember,
	PARTY_MEMBER_DATA & data)
{
	PARTY_LIST * parties = sGetPartyList(game);
	ASSERT_RETURN(parties);
	ASSERT_RETURN(parties->m_IterCount == 0);

	PARTY * party = sGetPartyByGuid(parties, idParty);
	ASSERT_RETURN(party);

	unsigned int ii = sFindMember(party, idMember);
	if (ii >= party->m_nMemberCount || party->m_PartyMembers[ii].m_Guid != idMember)
	{
		party->m_PartyMembers = (PARTY_MEMBER *)GREALLOC(game, party->m_PartyMembers, sizeof(PARTY_MEMBER) * (party->m_nMemberCount + 1));

		if (ii < party->m_nMemberCount)
		{
			MemoryMove(party->m_PartyMembers + ii + 1, (party->m_nMemberCount - ii) * sizeof(PARTY_MEMBER), party->m_PartyMembers + ii, (party->m_nMemberCount - ii) * sizeof(PARTY_MEMBER));
		}
		++party->m_nMemberCount;
		party->m_PartyMembers[ii].m_Guid = idMember;
	}
	PARTY_MEMBER * member = party->m_PartyMembers + ii;
	member->m_Data = data;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PartyUpdateMember(
	GAME * game,
	PGUID idParty,
	PGUID idMember,
	PARTY_MEMBER_DATA & data)
{
	PartyAddMember(game, idParty, idMember, data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PartyRemoveMember(
	GAME * game,
	PGUID idParty,
	PGUID idMember)
{
	PARTY_LIST * parties = sGetPartyList(game);
	ASSERT_RETURN(parties);
	ASSERT_RETURN(parties->m_IterCount == 0);

	PARTY * party = sGetPartyByGuid(parties, idParty);
	ASSERT_RETURN(party);

	unsigned int ii = sFindMember(party, idParty);
	if (ii >= party->m_nMemberCount || party->m_PartyMembers[ii].m_Guid != idMember)
	{
		return;
	}

	if (ii < party->m_nMemberCount - 1)
	{
		MemoryMove(party->m_PartyMembers + ii, (party->m_nMemberCount - ii) * sizeof(PARTY), party->m_PartyMembers + ii + 1, (party->m_nMemberCount - ii - 1) * sizeof(PARTY));
	}
	--party->m_nMemberCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PARTY_ITER::Release(
	void)
{
	PARTY_LIST * parties = sGetPartyList(m_Game);
	ASSERT_RETURN(parties);

	if (m_nPartyIndex == INVALID_PARTYINDEX)
	{
		return;
	}
	ASSERT_RETURN(parties->m_IterCount > 0);
	--parties->m_IterCount;

	m_nPartyIndex = INVALID_PARTYINDEX;
	m_nMemberIndex = INVALID_PARTYINDEX;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PARTY_ITER::SetParty(
	PGUID idParty)
{
	Release();

	PARTY_LIST * parties = sGetPartyList(m_Game);
	ASSERT_RETFALSE(parties);
	ASSERT_RETFALSE(idParty != INVALID_GUID);

	unsigned int ii = sFindParty(parties, idParty);
	if (ii >= parties->m_nPartyCount || parties->m_Parties[ii].m_Guid != idParty)
	{
		return FALSE;
	}

	m_nPartyIndex = ii;
	parties->m_IterCount++;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARTY_ITER::PARTY_ITER(
	GAME * game,
	PGUID idParty)
{
	m_Game = game;
	m_nPartyIndex = INVALID_PARTYINDEX;
	m_nMemberIndex = INVALID_PARTYINDEX;

	if (idParty != INVALID_GUID)
	{
		SetParty(idParty);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int PARTY_ITER::GetMemberCount(
	void)
{
	PARTY * party = sGetParty(m_Game, m_nPartyIndex);
	ASSERT_RETZERO(party);

	return party->m_nMemberCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int PARTY_ITER::GetInGameCount(
	void)
{
	PARTY * party = sGetParty(m_Game, m_nPartyIndex);
	ASSERT_RETZERO(party);

	return party->m_nInGameCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PARTY_ITER::GetFirst(
	PARTY_MEMBER_DATA & data)
{
	PARTY * party = sGetParty(m_Game, m_nPartyIndex);
	ASSERT_RETFALSE(party);

	PARTY_MEMBER * member = sGetMember(party, 0);
	ASSERT_RETFALSE(member);

	m_nMemberIndex = 0;
	data = member->m_Data;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PARTY_ITER::GetNext(
	PARTY_MEMBER_DATA & data)
{
	PARTY * party = sGetParty(m_Game, m_nPartyIndex);
	ASSERT_RETFALSE(party);

	PARTY_MEMBER * member = sGetMember(party, m_nMemberIndex + 1);
	if (!member)
	{
		m_nMemberIndex = INVALID_PARTYINDEX;
		return FALSE;
	}

	++m_nMemberIndex;
	data = member->m_Data;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * PARTY_ITER::GetFirstUnitInGame(
	void)
{
	PARTY * party = sGetParty(m_Game, m_nPartyIndex);
	ASSERT_RETNULL(party);

	PARTY_MEMBER * member = party->m_PartyMembers;
	for (unsigned int ii = 0; ii < party->m_nMemberCount; ++ii, ++member)
	{
		if (TESTBIT(member->m_dwFlags, PARTY_FLAG_INGAME))
		{
			m_nMemberIndex = ii;
			return member->m_Unit;
		}
	}
	m_nMemberIndex = INVALID_PARTYINDEX;
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * PARTY_ITER::GetNextUnitInGame(
	void)
{
	PARTY * party = sGetParty(m_Game, m_nPartyIndex);
	ASSERT_RETNULL(party);

	ASSERT_RETNULL(m_nMemberIndex < party->m_nMemberCount);

	PARTY_MEMBER * member = party->m_PartyMembers + m_nMemberIndex + 1;
	for (unsigned int ii = m_nMemberIndex + 1; ii < party->m_nMemberCount; ++ii, ++member)
	{
		if (TESTBIT(member->m_dwFlags, PARTY_FLAG_INGAME))
		{
			m_nMemberIndex = ii;
			return member->m_Unit;
		}
	}
	m_nMemberIndex = INVALID_PARTYINDEX;
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID s_UnitGetPartyGuid(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETVAL(unit, INVALID_GUID);
	PARTY_LIST * parties = sGetPartyList(game);
	ASSERT_RETVAL(parties, INVALID_GUID);

	UNIT_NODE * node = parties->m_UnitHash.Get(UnitGetId(unit));
	if (!node)
	{
		return INVALID_GUID;
	}
	return node->m_guidParty;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PartyMemberWarpTo(
	PGUID guidPartyMember)
{
	ASSERTX_RETURN( guidPartyMember != INVALID_GUID, "Expected guid" );

#if !ISVERSION(SERVER_VERSION)	
	// tell server
	MSG_CCMD_PARTY_MEMBER_TRY_WARP_TO tMessage;
	tMessage.guidPartyMember = guidPartyMember;
	c_SendMessage( CCMD_PARTY_MEMBER_TRY_WARP_TO, &tMessage );
#endif
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PartyIsInGame(
	GAME *pGame)
{
	ASSERTV_RETFALSE( pGame, "Expected game" );
	
	// check all the clients in game
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pPlayer = ClientGetControlUnit( pClient );
		if (pPlayer)
		{
			if (UnitGetPartyId( pPlayer ) != INVALID_ID)
			{
				return TRUE;
			}
		}
		
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PartyIterateCachedPartyMembersInGame(
	GAME *pGame,
	PARTYID idParty,
	PFN_PLAYER_ITERATE pfnCallback,
	void *pCallbackData)
{
	ASSERTV_RETURN( idParty != INVALID_ID, "Invalid party id" );
	ASSERTV_RETURN( pfnCallback, "Expected callback" );

#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN( svr );
	if( svr )
	{
		CachedPartyPtr party(svr->m_PartyCache);
		party = PartyCacheLockParty( svr->m_PartyCache, idParty );	
		if( party )
		{
			s_PartyIterateCachedPartyMembersInGame( pGame, party, pfnCallback, pCallbackData );
		}
	}	
#endif
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PartyIterateCachedPartyMembersInGame(
	GAME *pGame,
	CachedParty *pParty,
	PFN_PLAYER_ITERATE pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pParty, "Expected party" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );

#if !ISVERSION(CLIENT_ONLY_VERSION)
	
	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN( svr );
	
	for( int t = 0; t < (int)pParty->Members.Count(); t++ )
	{
		CachedPartyMember * pPartyMember = pParty->Members[ t ];
		GAMEID idGameMember = pPartyMember->MemberData.IdGame;
		
		if (idGameMember == GameGetId( pGame ))
		{
			if( pPartyMember->IdAppClient != INVALID_ID)
			{
				GAMECLIENTID GameClientId = ClientSystemGetClientGameId( svr->m_ClientSystem, pPartyMember->IdAppClient );
				if( GameClientId != INVALID_GAMECLIENTID )
				{
					GAMECLIENT* pClient = ClientGetById( pGame, GameClientId );
					if( pClient )
					{
						UNIT* pMember = ClientGetPlayerUnit( pClient );
						if( pMember )
						{
							pfnCallback( pMember, pCallbackData );
						}
					}
				}
			}
		}
	}
	
#endif
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCountPartyMember(
	UNIT *pPlayer,
	void *pCallbackData)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( pCallbackData, "Expected callback data" );
	int *pnCount = (int *)pCallbackData;
	*pnCount = *pnCount + 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_PartyGetMemberCount(
	GAME *pGame,
	PARTYID idParty)
{	
	ASSERTX_RETZERO( pGame, "Expected game" );
	ASSERTX_RETZERO( idParty != INVALID_ID, "Expected party id" );	
	int nCount = 0;
	s_PartyIterateCachedPartyMembersInGame( pGame, idParty, sCountPartyMember, &nCount );
	return nCount;
}
