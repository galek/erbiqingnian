#ifndef _C_CHARACTERSELECT_H_
#define _C_CHARACTERSELECT_H_

#include "characterlimit.h"

void CharacterSelectLoginQueueUiInit(void);

BOOL CharacterSelectStartLogin(CHANNEL *pChannel);

#ifndef _BOT
void CharacterSelectProcessMessages(MAILSLOT *slot, CHANNEL *pChannel);
#else
void CharacterSelectProcessMessages(
	MAILSLOT *slot,
	CHANNEL *pChannel,
	int &nCharTotal,
	int &nCharCount,
	int nPlayerSlots[MAX_CHARACTERS_PER_ACCOUNT],
	WCHAR szPlayerNames[MAX_CHARACTER_NAME][MAX_CHARACTERS_PER_ACCOUNT]);
#endif

void CharacterSelectSetCharName(
	__in __notnull  const WCHAR * szCharName, 
	int nCharacterSlot,
	int nDifficulty,
	BOOL bNewPlayer = FALSE);

void CharacterSelectDeleteCharName(
	__in __notnull const WCHAR * szCharName );

CHARACTER_SLOT_TYPE CharacterSelectGetSlotType(int nCharacterSlot);

BOOL CharacterSelectWaitingInLoginQueue(void);

#endif
