//----------------------------------------------------------------------------
// FILE: hotkeys.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __HOTKEYS_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __HOTKEYS_H_

//----------------------------------------------------------------------------
// Forward References
//----------------------------------------------------------------------------
struct GAMECLIENT;
enum TAG_SELECTOR;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
#define UNITSAVE_VESRION_HOTKEYS_WRONG_SELECTOR_2007_07_02 174  // hotkeys saved in the database were using the hotkey save method for file instead of database

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL HotkeysXfer(
	UNIT *pUnit,
	XFER<mode> &tXfer,
	struct UNIT_FILE_HEADER &tHeader,
	BOOKMARKS &tBookmarks,
	BOOL bSkipData);

template BOOL HotkeysXfer<XFER_SAVE>(UNIT *, XFER<XFER_SAVE> &, struct UNIT_FILE_HEADER &, BOOKMARKS &, BOOL);
template BOOL HotkeysXfer<XFER_LOAD>(UNIT *, XFER<XFER_LOAD> &, struct UNIT_FILE_HEADER &, BOOKMARKS &, BOOL);


void HotkeysSendAll(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	TAG_SELECTOR eFirstKey,
	TAG_SELECTOR eLastKey);

void HotkeySet( 
	GAME* pGame,
	UNIT* pUnit,
	BYTE bySkillAssign,
	int nSkillID);

void HotkeySend(	
	GAMECLIENT *pClient,
	UNIT *pPlayer,
	TAG_SELECTOR eHotkey);

void s_HotkeysReturnItemsToStandardInventory(
	UNIT *pPlayer);
	
#endif