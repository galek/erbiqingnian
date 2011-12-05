//----------------------------------------------------------------------------
// c_chatBuddy.h
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//---------------------------------------------------------------------------
#ifndef _C_CHATBUDDY_H_
#define _C_CHATBUDDY_H_


#ifndef _BUDDYSYSTEM_H_
#include "BuddySystem.h"
#endif


//----------------------------------------------------------------------------
//	CLIENT BUDDY SYSTEM METHODS
//----------------------------------------------------------------------------

void c_BuddyListInit(
	void );

void c_BuddyOnOpResult(
	void * data );

BOOL c_BuddyAdd(
	LPCWSTR TargetCharName);

BOOL c_BuddyRemove(
	LPCWSTR TargetNickName);

BOOL c_BuddyRemove(
	CLIENT_BUDDY_ID idTarget);

BOOL c_BuddySetNick(
	LPCWSTR NickNameOld,
	LPCWSTR NickNameNew);

void c_BuddyShowList();

LPCWSTR c_BuddyGetCharNameByID(
	CLIENT_BUDDY_ID linkID);

LPCWSTR c_BuddyGetNickNameByID(
	CLIENT_BUDDY_ID linkID);

LPCWSTR c_BuddyGetCharNameByGuid(
	PGUID guid);

PGUID c_BuddyGetGuidByID(
	CLIENT_BUDDY_ID linkID);

int c_BuddyGetLevelId(
	PGUID guid);

struct BUDDY_UI_ENTRY
{
	CLIENT_BUDDY_ID	 idLink;
	BOOL  bOnline;
	WCHAR szCharName[MAX_CHARACTER_NAME];
	WCHAR szAcctNick[MAX_CHARACTER_NAME];
	int	  nCharacterLevel;
	int	  nCharacterRank;
	int	  nCharacterClass;
	int   nLevelDefinitionId;
	int   nAreaId;
	int   nAreaDepth;
};

int c_BuddyGetList(
	BUDDY_UI_ENTRY *pEntries,
	int nNumEntries);

//----------------------------------------------------------------------------
//	CLIENT MESSAGE HANDLERS
//----------------------------------------------------------------------------

void sChatCmdBuddyInfo(
	GAME * game,
	__notnull BYTE * data );

void sChatCmdBuddyInfoUpdate(
	GAME * game,
	__notnull BYTE * data );

void sChatCmdBuddyRemoved(
	GAME * game,
	__notnull BYTE * data );


#endif	//	_C_CHATBUDDY_H_
