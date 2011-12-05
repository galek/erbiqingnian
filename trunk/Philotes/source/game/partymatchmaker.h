//----------------------------------------------------------------------------
// FILE: partymatchmaker.h
//----------------------------------------------------------------------------

#ifndef __PARTY_MATCHMAKER_H_
#define __PARTY_MATCHMAKER_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct UNIT;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------

void s_PartyMatchmakerInitForGame(
	GAME *pGame);

void s_PartyMatchmakerFreeForGame(
	GAME *pGame);
	
void s_PartyMatchmakerEnable(
	UNIT *pPlayer,
	DWORD dwTicksTillUpdate = 0);

void s_PartyMatchmakerDisable(
	UNIT *pPlayer);

void s_PartyMatchmakerFormAutoParty(	
	GAME *pGame,
	PARTYID idParty,
	ULONGLONG ullRequestCode);	

BOOL s_PartyMatchmakerCreateNewParty(
	GAME *pGame,
	UNITID idPlayerA,
	UNITID idPlayerB);

BOOL s_PartyMatchmakerIsInPendingParty(
	UNIT *pPlayer);

BOOL  PartyMatchmakerIsEnabledForGame(
	GAME *pGame);
		
#endif  // end __PARTY_MATCHMAKER_H_
