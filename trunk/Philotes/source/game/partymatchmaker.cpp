//----------------------------------------------------------------------------
// FILE: partymatchmaker.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "gameglobals.h"
#include "party.h"
#include "partymatchmaker.h"
#include "player.h"
#include "s_partyCache.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
const BOOL gbPartyMatchmakerEnabled = FALSE;  // is the auto party system even active?

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum PARTY_MATCHMAKER_CONSTANTS
{
	AUTO_PARTY_UPATE_DELAY_IN_TICKS = GAME_TICKS_PER_SECOND * 5,
	MAX_PENDING_AUTO_PARTIES = 128,		// arbitrary
	MAX_PENDING_PARTY_PLAYERS = 16,		// must be >= AUTO_PARTY_MAX_MEMBER_COUNT
	INVALID_PENDING_PARTYID = 0,
};

//----------------------------------------------------------------------------
struct CAN_AUTO_JOIN_EXISTING_PARTY_DATA
{
	UNIT *pPlayerSearching;			// the player searching for an auto party
	PARTYID idParty;				// the party we're looking at
	int nNumMembers;				// the number of members in the party
	
	int nNumMembersOKToJoinWith;	// number of members we can join with
	
	CAN_AUTO_JOIN_EXISTING_PARTY_DATA::CAN_AUTO_JOIN_EXISTING_PARTY_DATA(
		void)
		:	pPlayerSearching( NULL ),
			idParty( INVALID_ID ),
			nNumMembers( 0 ),
			nNumMembersOKToJoinWith( 0 )
	{ }
	
};

//----------------------------------------------------------------------------
struct FIND_UNPARTIED_PLAYER_DATA
{
	UNIT *pPlayerSearching;
	UNIT *pPlayerUnpartied;
	
	FIND_UNPARTIED_PLAYER_DATA::FIND_UNPARTIED_PLAYER_DATA( void )
		:	pPlayerSearching( NULL ),
			pPlayerUnpartied( NULL )
	{ }
	
};

//----------------------------------------------------------------------------
struct FIND_EXISTING_AUTO_PARTY_TO_JOIN_DATA
{
	UNIT *pSearcher;
	PARTYID idParty;
	
	FIND_EXISTING_AUTO_PARTY_TO_JOIN_DATA::FIND_EXISTING_AUTO_PARTY_TO_JOIN_DATA( void )
		:	pSearcher( NULL ),
			idParty( INVALID_ID )
	{ }
	
};

//----------------------------------------------------------------------------
struct PENDING_AUTO_PARTY
{
	ULONGLONG ullPendingAutoPartyID;
	int nPlayerCount;
	UNITID idPlayers[ MAX_PENDING_PARTY_PLAYERS ];
};

//----------------------------------------------------------------------------
struct PARTY_MATCHMAKER_GLOBALS
{
	PENDING_AUTO_PARTY tPendingAutoParty[ MAX_PENDING_AUTO_PARTIES ];
	int nNumPendingParties;
	ULONGLONG ullNextPendingPartyID;
};
static PARTY_MATCHMAKER_GLOBALS sgtMatchmakerGlobals;

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PARTY_MATCHMAKER_GLOBALS *sGetMatchmakerGlobals(
	void)
{
	return &sgtMatchmakerGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPendingAutoPartyInit(
	PENDING_AUTO_PARTY &tPending)
{
	tPending.ullPendingAutoPartyID = INVALID_PENDING_PARTYID;
	tPending.nPlayerCount = 0;
	for (int i = 0; i < MAX_PENDING_PARTY_PLAYERS; ++i)
	{
		tPending.idPlayers[ i ] = INVALID_ID;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PartyMatchmakerInitForGame(
	GAME *pGame)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	PARTY_MATCHMAKER_GLOBALS *pGlobals = sGetMatchmakerGlobals();
	ASSERTX_RETURN( pGlobals, "Expected globals" );

	ASSERTX( 
		MAX_PENDING_PARTY_PLAYERS >= AUTO_PARTY_MAX_MEMBER_COUNT, 
		"Pending parties not large enough to hold the number of players allowed in an auto party" );
	
	// init each pending party
	pGlobals->nNumPendingParties = 0;
	for (int i = 0; i < MAX_PENDING_AUTO_PARTIES; ++i)
	{
		PENDING_AUTO_PARTY *pPending = &pGlobals->tPendingAutoParty[ i ];
		sPendingAutoPartyInit( *pPending );
	}

	// init id request codes
	pGlobals->ullNextPendingPartyID = 0;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PartyMatchmakerFreeForGame(
	GAME *pGame)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	s_PartyMatchmakerInitForGame( pGame );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanAutoPartyWithPlayer(
	UNIT *pPlayer,
	UNIT *pPlayerOther)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( pPlayerOther, "Expected unit" );
	ASSERTX_RETFALSE( UnitGetGame( pPlayer ) == UnitGetGame( pPlayerOther ), "Units not in same game!" );

	// cannot party with yourself (should not happen, but just to be sure)
	if (pPlayer == pPlayerOther)
	{
		return FALSE;
	}	

	// must be in the same level (should not happen, but just to be sure)
	if (UnitsAreInSameLevel( pPlayer, pPlayerOther ) == FALSE)
	{
		return FALSE;
	}

	// level must allow auto parties (should not happen, but just to be sure)
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	if (LevelCanFormAutoParties( pLevel ) == FALSE)
	{
		return FALSE;
	}
		
	// players must have auto party on
	if (s_PlayerIsAutoPartyEnabled( pPlayer ) == FALSE ||
		s_PlayerIsAutoPartyEnabled( pPlayerOther ) == FALSE)
	{
		return FALSE;
	}
	
	// game variants must match
	GAME_VARIANT tVariantPlayer;
	GAME_VARIANT tVariantOther;
	GameVariantInit( tVariantPlayer, pPlayer );
	GameVariantInit( tVariantOther, pPlayerOther );
	if (GameVariantsAreEqual( tVariantOther, tVariantPlayer ) == FALSE)
	{
		return FALSE;
	}
		
	// PvP auto partying rules for specific PvP game types:
	// for team based games, the players must be on the same team
	if (GameGetSubType( UnitGetGame( pPlayer ) ) == GAME_SUBTYPE_PVP_CTF)
	{
		if (UnitGetTeam( pPlayer ) != UnitGetTeam( pPlayerOther ))
		{
			return FALSE;
		}
	}
	else
	{
		// must be within the level range for an auto party
		int nExperienceLevelPlayer = UnitGetExperienceLevel( pPlayer );
		int nExperienceLevelOther = UnitGetExperienceLevel( pPlayerOther );
		if (ABS( nExperienceLevelOther - nExperienceLevelPlayer ) > AUTO_PARTY_MAX_EXPERIENCE_LEVEL_DIFFERENCE)
		{	
			return FALSE;
		}
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCanAutoPartyWithPlayerInParty(
	UNIT *pPlayerInParty,
	void *pCallbackData)
{
	ASSERTX_RETURN( pPlayerInParty, "Expected unit" );
	ASSERTX_RETURN( pCallbackData, "Expected callback data" );
	CAN_AUTO_JOIN_EXISTING_PARTY_DATA *pCanAutoJoinExistingPartyData = (CAN_AUTO_JOIN_EXISTING_PARTY_DATA *)pCallbackData;
	UNIT *pPlayerSearching = pCanAutoJoinExistingPartyData->pPlayerSearching;
	ASSERTX_RETURN( pPlayerSearching, "Expected unit" );

	if (sCanAutoPartyWithPlayer( pPlayerInParty, pPlayerSearching ))
	{
		pCanAutoJoinExistingPartyData->nNumMembersOKToJoinWith++;
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sCanAutoJoinParty(
	UNIT *pPlayer,
	CachedParty *pParty)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( pParty , "Expected party" );

	// forget it if party has no members
	if (pParty->Members.Count() == 0)
	{
		return FALSE;
	}
	
	// setup callback data for checking against each party member
	CAN_AUTO_JOIN_EXISTING_PARTY_DATA tCanAutoJoinExistingPartyData;
	tCanAutoJoinExistingPartyData.pPlayerSearching = pPlayer;
	tCanAutoJoinExistingPartyData.idParty = pParty->id;
	tCanAutoJoinExistingPartyData.nNumMembers = pParty->Members.Count();
	tCanAutoJoinExistingPartyData.nNumMembersOKToJoinWith = 0;
	
	// check requirements against each member
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETFALSE( pGame, "Expected game" );
	s_PartyIterateCachedPartyMembersInGame( pGame, pParty, sCanAutoPartyWithPlayerInParty, &tCanAutoJoinExistingPartyData );

	// if couldn't group with all of the players can't join party ... either all the party
	// members were not in this game or we were not allowed to party up with one of them
	// because of restrictions etc
	if (tCanAutoJoinExistingPartyData.nNumMembers != tCanAutoJoinExistingPartyData.nNumMembersOKToJoinWith)
	{
		return FALSE;
	}

	// if there are too many members in the party already, can't join
	int nMemberCount = pParty->Members.Count();
	if (nMemberCount >= AUTO_PARTY_MAX_MEMBER_COUNT)
	{
		return FALSE;
	}

	return TRUE;
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sFindPendingPartyIndexByPlayerID(
	UNITID idPlayer)
{	
	ASSERTX_RETINVALID( idPlayer != INVALID_ID, "Expected player id" );
	PARTY_MATCHMAKER_GLOBALS *pGlobals = sGetMatchmakerGlobals();
	ASSERTX_RETINVALID( pGlobals, "Expected globals" );

	// search pending parties
	for (int i = 0; i < pGlobals->nNumPendingParties; ++i)
	{
		PENDING_AUTO_PARTY *pPending = &pGlobals->tPendingAutoParty[ i ];
		for (int j = 0; j  < pPending->nPlayerCount; ++j)
		{
			if (pPending->idPlayers[ j ] == idPlayer)
			{
				return i;
			}			
		}
	}
	
	return INVALID_INDEX;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sFindPendingPartyIndexByPendingPartyID(
	ULONGLONG ullPendingPartyID)
{	
	ASSERTX_RETINVALID( ullPendingPartyID != INVALID_PENDING_PARTYID, "Expected pending party id" );
	PARTY_MATCHMAKER_GLOBALS *pGlobals = sGetMatchmakerGlobals();
	ASSERTX_RETINVALID( pGlobals, "Expected globals" );

	// search pending parties
	for (int i = 0; i < pGlobals->nNumPendingParties; ++i)
	{
		PENDING_AUTO_PARTY *pPending = &pGlobals->tPendingAutoParty[ i ];
		if (pPending->ullPendingAutoPartyID == ullPendingPartyID)
		{
			return i;
		}
	}
	
	return INVALID_INDEX;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PENDING_AUTO_PARTY *sFindPendingPartyByID(
	ULONGLONG ullPendingPartyID)
{
	ASSERTX_RETNULL( ullPendingPartyID != INVALID_PENDING_PARTYID, "Expected pending party id" );
	PARTY_MATCHMAKER_GLOBALS *pGlobals = sGetMatchmakerGlobals();
	ASSERTX_RETNULL( pGlobals, "Expected globals" );

	// find pending party index
	int nIndex = sFindPendingPartyIndexByPendingPartyID( ullPendingPartyID );
	if (nIndex >= 0 && nIndex < pGlobals->nNumPendingParties)
	{
		// return pointer
		return &pGlobals->tPendingAutoParty[ nIndex ];
	}
	
	// not found
	return NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sFindExistingAutoPartyToJoin(
	CachedParty *pParty,
	void *pCallbackData)
{
	ASSERTX_RETURN( pParty, "Expected party" );
	FIND_EXISTING_AUTO_PARTY_TO_JOIN_DATA *pFindAutoPartyData = (FIND_EXISTING_AUTO_PARTY_TO_JOIN_DATA *)pCallbackData;

	// if we found a party already, no need to keep searching
	if (pFindAutoPartyData->idParty == INVALID_ID)
	{
			
		// if we can join this party, we will use it
		if (sCanAutoJoinParty( pFindAutoPartyData->pSearcher, pParty ))
		{
			pFindAutoPartyData->idParty = pParty->id;
		}
						
	}
	
}
#endif 

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoFindUnpartiedPlayerToAutoPartyWith(
	UNIT *pPlayer,
	void *pCallbackData)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	FIND_UNPARTIED_PLAYER_DATA *pFindUnpartiedData = (FIND_UNPARTIED_PLAYER_DATA *)pCallbackData;
	ASSERTX_RETURN( pFindUnpartiedData, "Expected find unpartied player data pointer" );

	// must not be the player that is searching
	if (pFindUnpartiedData->pPlayerSearching != pPlayer)
	{	
	
		// don't consider players that are not in the game yet
		if (UnitIsInLimbo( pPlayer ) == FALSE)
		{
		
			// only care if we haven't found anybody yet (for now, we can make this better later if
			// we can come up with some new rules
			if (pFindUnpartiedData->pPlayerUnpartied == NULL)
			{
			
				// this player must be unpartied (and not in the process of auto joining anothyer party)
				// and able to auto party with the player that is searching
				if (UnitGetPartyId( pPlayer ) == INVALID_ID &&
					s_PartyMatchmakerIsInPendingParty( pPlayer ) == FALSE &&
					sCanAutoPartyWithPlayer( pFindUnpartiedData->pPlayerSearching, pPlayer ) == TRUE)
				{
					pFindUnpartiedData->pPlayerUnpartied = pPlayer;
				}
				
			}

		}
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITID sFindUnpartiedPlayerToAutoPartyWith(
	UNIT *pPlayer)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player" );
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	ASSERTX_RETINVALID( pLevel, "Expected level" );

	// setup callback data	
	FIND_UNPARTIED_PLAYER_DATA tFindUnpartiedData;
	tFindUnpartiedData.pPlayerSearching = pPlayer;
	tFindUnpartiedData.pPlayerUnpartied = NULL;
	
	// search players in level
	LevelIteratePlayers( pLevel, sDoFindUnpartiedPlayerToAutoPartyWith, &tFindUnpartiedData );

	// return player (if we found one)
	UNITID idPlayerToPartyWith = INVALID_ID;
	if (tFindUnpartiedData.pPlayerUnpartied != NULL)
	{
		idPlayerToPartyWith = UnitGetId( tFindUnpartiedData.pPlayerUnpartied );
	}
	
	return idPlayerToPartyWith;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddPlayerIDToPendingParty(
	UNITID idPlayer,
	PENDING_AUTO_PARTY *pPending)
{
	ASSERTX_RETFALSE( idPlayer != INVALID_ID, "Expected player id" );
	ASSERTX_RETFALSE( pPending, "Expected pending party pointer" );
	
	// must have enough room to add player
	ASSERTX_RETFALSE( pPending->nPlayerCount <= MAX_PENDING_PARTY_PLAYERS - 1, "Too many players in pending party" );

	// add playe rid	
	pPending->idPlayers[ pPending->nPlayerCount++ ] = idPlayer;

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddUnitIDToPendingPartyID(
	UNITID idPlayer,
	ULONGLONG ullPendingParty)
{
	ASSERTX_RETVAL( idPlayer != INVALID_ID, INVALID_PENDING_PARTYID, "Expected player id" );
	PARTY_MATCHMAKER_GLOBALS *pGlobals = sGetMatchmakerGlobals();
	ASSERTX_RETVAL( pGlobals, INVALID_PENDING_PARTYID, "Expected globals" );

	// validate that this player is not already in a pending party
	ASSERTX_RETVAL( 
		sFindPendingPartyIndexByPlayerID( idPlayer ) == INVALID_INDEX, 
		INVALID_PENDING_PARTYID, 
		"Player id already in a pending party" );	

	// get index of pending party
	int nPendingPartyIndex = sFindPendingPartyIndexByPendingPartyID( ullPendingParty );
	ASSERTX_RETFALSE( nPendingPartyIndex >= 0 && nPendingPartyIndex < MAX_PENDING_AUTO_PARTIES, "Invalid pending party index" );

	// get the pending party
	PENDING_AUTO_PARTY *pPending = &pGlobals->tPendingAutoParty[ nPendingPartyIndex ];
	ASSERTX_RETFALSE( pPending, "Expected pending party pointer" );
	if (sAddPlayerIDToPendingParty( idPlayer, pPending ) == FALSE)
	{
		ASSERTX_RETVAL( 0, INVALID_PENDING_PARTYID, "Unable to add additional player to pending party" );
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ULONGLONG sFindPendingPartyIDToJoin(
	UNIT *pPlayer)
{
	ASSERTX_RETVAL( pPlayer, INVALID_PENDING_PARTYID, "Expected unit" );
	ASSERTX_RETVAL( UnitIsPlayer( pPlayer ), INVALID_PENDING_PARTYID, "Expected player" );
	PARTY_MATCHMAKER_GLOBALS *pGlobals = sGetMatchmakerGlobals();
	ASSERTX_RETVAL( pGlobals, INVALID_PENDING_PARTYID, "Expected globals" );
	GAME *pGame = UnitGetGame( pPlayer );
	
	// search for a pending party
	for (int i = 0; i < pGlobals->nNumPendingParties; ++i)
	{
		PENDING_AUTO_PARTY *pPending = &pGlobals->tPendingAutoParty[ i ];
		
		// must be able to party with all players
		BOOL bCanPartyWithAllPlayers = TRUE;
		for (int j = 0; j < pPending->nPlayerCount; ++j)
		{
			UNIT *pPlayerOther = UnitGetById( pGame, pPending->idPlayers[ j ] );
			if (sCanAutoPartyWithPlayer( pPlayer, pPlayerOther ) == FALSE)
			{
				bCanPartyWithAllPlayers = FALSE;
				break;
			}
		}

		// if we can party with all players, we will use this pending party
		if (bCanPartyWithAllPlayers == TRUE)
		{
			pPending->ullPendingAutoPartyID;
		}
				
	}
	
	// no suitable pending party found
	return INVALID_PENDING_PARTYID;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoPlayerTryAutoParty(
	GAME *pGame,
	UNIT *pPlayer,
	const EVENT_DATA &tEventData)
{

#if !ISVERSION(CLIENT_ONLY_VERSION)

	// we must have auto party enabled
	if (s_PlayerIsAutoPartyEnabled( pPlayer ) &&
		UnitGetPartyId( pPlayer ) == INVALID_ID)
	{
		BOOL bTryMatchmaking = TRUE;

		// if we're in the process of joining one, try to join or form an auto party
		if (s_PartyMatchmakerIsInPendingParty( pPlayer ) == FALSE)
		{

			// see if we can join a pending party (this will let us form the biggest auto
			// parties we can if people are trying to auto party at the same time)
			ULONGLONG ullPendingPartyID = sFindPendingPartyIDToJoin( pPlayer );
			if (ullPendingPartyID != INVALID_PENDING_PARTYID)
			{
			
				// add to pending paryt
				if (sAddUnitIDToPendingPartyID( UnitGetId( pPlayer ), ullPendingPartyID ) == TRUE)
				{
					// don't run match making again because we're in a party now and are done
					bTryMatchmaking = FALSE;				
				}
				
			}
			
			// if still need to search
			if (bTryMatchmaking == TRUE)
			{
			
				// setup callback data
				FIND_EXISTING_AUTO_PARTY_TO_JOIN_DATA tFindAutoPartyData;
				tFindAutoPartyData.pSearcher = pPlayer;
				tFindAutoPartyData.idParty = INVALID_ID;

				// find a party to join
				LEVEL *pLevel = UnitGetLevel( pPlayer );
				if (pLevel)
				{
					PartyCacheIterateParties( pGame, sFindExistingAutoPartyToJoin, &tFindAutoPartyData );
				}
				
				// join party if we found one			
				if (tFindAutoPartyData.idParty != INVALID_ID)
				{
				
					// add to party
					PartyCacheAddToParty( pPlayer, tFindAutoPartyData.idParty );		
					
					// don't run match making again because we're in a party now and are done
					bTryMatchmaking = FALSE;
					
				}
				else
				{
					// didn't find a party, find a player that we can auto party with
					UNITID idPlayerOther = sFindUnpartiedPlayerToAutoPartyWith( pPlayer );
					if (idPlayerOther != INVALID_ID)
					{
					
						// form a new party
						if (s_PartyMatchmakerCreateNewParty( pGame, UnitGetId( pPlayer ), idPlayerOther ))
						{						
							// don't run match making again because we're in a party now and are done
							bTryMatchmaking = FALSE;
						}
						
					}
								
				}

			}
						
		}
		
		// register another event if we're still not in a party
		if (bTryMatchmaking == TRUE)
		{
			s_PartyMatchmakerEnable( pPlayer, AUTO_PARTY_UPATE_DELAY_IN_TICKS);
		}

	}

#endif

	return TRUE;
			
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PartyMatchmakerEnable(
	UNIT *pPlayer,
	DWORD dwTicksTillUpdate /*= 0*/)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// disable any event we used to have
	s_PartyMatchmakerDisable( pPlayer );

	// do nothing if this sub-system is disabled
	GAME *pGame = UnitGetGame( pPlayer );
	if (!PartyMatchmakerIsEnabledForGame( pGame ))
	{
		return;
	}
	
	// in single player we don't care
	if (AppIsSinglePlayer())
	{
		return;
	}

	// the level that the player is on must allow auto parties to form
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	if (pLevel == NULL)
	{
		// not setup in a level yet
		return;
	}
	if (LevelCanFormAutoParties( pLevel ) == FALSE)
	{
		return;
	}

	// must have auto party turned on
	if (s_PlayerIsAutoPartyEnabled( pPlayer ) == FALSE)
	{
		return;
	}
				
	// register an event to try to auto party	
	UnitRegisterEventTimed( pPlayer, sDoPlayerTryAutoParty, NULL, dwTicksTillUpdate );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PartyMatchmakerDisable(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// unregister existing one if they have one
	if (UnitHasTimedEvent( pPlayer, sDoPlayerTryAutoParty ))
	{
		UnitUnregisterTimedEvent( pPlayer, sDoPlayerTryAutoParty );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemovePendingParty(
	ULONGLONG ullPendingPartyID)
{
	ASSERTX_RETURN( ullPendingPartyID != INVALID_PENDING_PARTYID, "Expected pending party id" );
	PARTY_MATCHMAKER_GLOBALS *pGlobals = sGetMatchmakerGlobals();
	ASSERTX_RETURN( pGlobals, "Expected globals" );

	// find the index of the pending party
	int nIndex = sFindPendingPartyIndexByPendingPartyID( ullPendingPartyID );
	if (nIndex >= 0 && nIndex < pGlobals->nNumPendingParties)
	{
	
		// if there is more than one pending party
		if (pGlobals->nNumPendingParties > 1)
		{

			// put the last pending party at the current index
			PENDING_AUTO_PARTY *pLastPending = &pGlobals->tPendingAutoParty[ pGlobals->nNumPendingParties - 1 ];
			pGlobals->tPendingAutoParty[ nIndex ] = *pLastPending;
			
			// clear out the last pending party
			sPendingAutoPartyInit( *pLastPending );
			
		}

		// we now have one less pending party
		pGlobals->nNumPendingParties--;
				
	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PartyMatchmakerFormAutoParty(
	GAME *pGame,
	PARTYID idParty,
	ULONGLONG ullPendingPartyID)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( idParty != INVALID_ID, "Expected party id" );
	ASSERTX_RETURN( ullPendingPartyID != INVALID_PENDING_PARTYID, "Expected pending party ID" );

#if !ISVERSION(CLIENT_ONLY_VERSION)
	
	// get pending party
	PENDING_AUTO_PARTY *pPending = sFindPendingPartyByID( ullPendingPartyID );
	if (pPending)
	{

		// all players must still exist
		BOOL bAllPlayersExist = TRUE;
		for (int i = 0; i < pPending->nPlayerCount; ++i)
		{
			UNITID idPlayer = pPending->idPlayers[ i ];
			UNIT *pUnit = UnitGetById( pGame, idPlayer );
			if (pUnit == NULL)
			{
				bAllPlayersExist = FALSE;
			}
		}
		
		// add each player to the party
		if (bAllPlayersExist == TRUE)
		{
			for (int i = 0; i < pPending->nPlayerCount; ++i)
			{
				UNITID idPlayer = pPending->idPlayers[ i ];
				UNIT *pUnit = UnitGetById( pGame, idPlayer );
				if (pUnit)
				{
				
					// add to party
					PartyCacheAddToParty( pUnit, idParty );
					
					// turn off the matchmaker for this player
					s_PartyMatchmakerDisable( pUnit );
					
				}
				
			}

		}
				
		// remove the pending party
		sRemovePendingParty( ullPendingPartyID );
		
	}

#endif
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ULONGLONG sNewPendingAutoParty(
	UNITID idPlayer)
{
	ASSERTX_RETVAL( idPlayer != INVALID_ID, INVALID_PENDING_PARTYID, "Expected player id" );
	PARTY_MATCHMAKER_GLOBALS *pGlobals = sGetMatchmakerGlobals();
	ASSERTX_RETVAL( pGlobals, INVALID_PENDING_PARTYID, "Expected globals" );
	
	// validate that this player is not already in a pending party
	ASSERTX_RETVAL( 
		sFindPendingPartyIndexByPlayerID( idPlayer ) == INVALID_INDEX, 
		INVALID_PENDING_PARTYID, 
		"Player id already in a pending party" );	

	// must have enough room for a new pending party
	ASSERTX_RETVAL( pGlobals->nNumPendingParties <= MAX_PENDING_AUTO_PARTIES - 1, INVALID_PENDING_PARTYID, "Too many pending parties" );
	
	// create new pending party
	PENDING_AUTO_PARTY *pPending = &pGlobals->tPendingAutoParty[ pGlobals->nNumPendingParties++ ];

	// init pending party
	sPendingAutoPartyInit( *pPending );
		
	// assign unique id to this request
	pPending->ullPendingAutoPartyID = ++pGlobals->ullNextPendingPartyID;
	
	// put player in it
	if (sAddPlayerIDToPendingParty( idPlayer, pPending ) == FALSE)
	{
		ASSERTX_RETVAL( 0, INVALID_PENDING_PARTYID, "Unable to add first player to pending party" );
	}

	// return the pending party id			
	return pPending->ullPendingAutoPartyID;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PartyMatchmakerCreateNewParty(
	GAME *pGame,
	UNITID idPlayerA,
	UNITID idPlayerB)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( idPlayerA != INVALID_ID, "Expected unit id" );
	ASSERTX_RETFALSE( idPlayerB != INVALID_ID, "Expected unit id" );
	BOOL bPartyRequested = FALSE;

#if !ISVERSION(CLIENT_ONLY_VERSION)

	// add first player to new pending party
	ULONGLONG ullPendingParty = sNewPendingAutoParty( idPlayerA );
	ASSERTX_RETFALSE( ullPendingParty != INVALID_PENDING_PARTYID, "Expected pending party id" );
	
	// add second player to pending party
	if (sAddUnitIDToPendingPartyID( idPlayerB, ullPendingParty ) == TRUE)
	{
	
		// request a new party id from the chat server
		GAMEID idGame = GameGetId( pGame );
		PartyCacheCreateParty( idGame, ullPendingParty );
	
		// success
		bPartyRequested = TRUE;
		
	}
	else
	{
	
		// error creating party, remove anything we've done so far
		if (ullPendingParty != INVALID_PENDING_PARTYID)
		{
			sRemovePendingParty( ullPendingParty );
		}
		
	}

#endif
			
	return bPartyRequested;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PartyMatchmakerIsInPendingParty(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	
	// find their pending party index
	return sFindPendingPartyIndexByPlayerID( UnitGetId( pPlayer ) ) != INVALID_INDEX;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PartyMatchmakerIsEnabledForGame(
	GAME *pGame)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	return gbPartyMatchmakerEnabled || GameGetSubType( pGame ) == GAME_SUBTYPE_PVP_CTF;
}