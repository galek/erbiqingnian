//----------------------------------------------------------------------------
// message.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_MESSAGE_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _MESSAGE_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef _APPCOMMON_H_
#include "appcommon.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MAX_NEW_UNIT_BUFFER				(MAX_SMALL_MSG_SIZE - 128)
#define MAX_WARDROBE_BUFFER				(MAX_SMALL_MSG_SIZE - 128)
#define MAX_WARDROBE_INIT_BUFFER		256 //Shouldn't this and MAX_WARDROBE_BUFFER be the same?
#define MAX_DB_FOR_SELECTION_BUFFER		(MAX_WARDROBE_INIT_BUFFER + 256)  // space for the wardrobe and basic character information like name, guids, etc
#define MAX_CHEAT_LEN					240
#define MAX_SET_LEVEL_MSG_BUFFER		192
#define MAX_STATE_STATS_BUFFER			(MAX_SMALL_MSG_SIZE - 128)
#define	MAX_PATH_BUFFER					(MAX_SMALL_MSG_SIZE - 128)
#define	MAX_PLAYERSAVE_BUFFER			(512 - MSG_HDR_SIZE)
#define MAX_BADGE_BUFFER				64
#define MAX_HOTKEY_ITEMS				2
#define MAX_HOTKEY_SKILLS				2
#define MAX_PORTAL_SPEC_NAME_LEN		32
#define MAX_REQMOVE_MSG_BUFFER_SIZE		254
#define MAX_MISSILE_STATS_SIZE			(MAX_SMALL_MSG_SIZE - 128)
#define MAX_STR_PARAM_SIZE				(MAX_CHARACTER_NAME*2+1)

//----------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT(TOWN_PORTAL_SPEC)
	
	// hellgate data
	MSG_FIELD(0, int, nLevelDefDest, INVALID_LINK)
	MSG_FIELD(1, int, nLevelDefPortal, INVALID_LINK)
	
	// tugboat data
	MSG_FIELD(2, int, nLevelDepthDest, INVALID_LINK)
	MSG_FIELD(3, int, nLevelDepthPortal, INVALID_LINK)
	MSG_FIELD(4, int, nLevelAreaDest, INVALID_LINK)
	MSG_FIELD(5, int, nLevelAreaPortal, INVALID_LINK)
	MSG_FIELD(6, int, nPVPWorld, INVALID_LINK)

	// common data
	MSG_FIELD(7, GAMEID, idGame, INVALID_ID)
	MSG_FIELD(8, UNITID, idPortal, INVALID_ID)

	//----------------------------------------------------------------------------	
	void Init(
		void )
	{
		this->zz_init_msg_struct();
	}
	
	//----------------------------------------------------------------------------	
	void Set(
		const TOWN_PORTAL_SPEC * other )
	{
		ASSERT_RETURN( other );
		
		// hellgate
		this->nLevelDefDest = other->nLevelDefDest;
		this->nLevelDefPortal = other->nLevelDefPortal;
		
		// tugboat
		this->nLevelDepthDest = other->nLevelDepthDest;
		this->nLevelDepthPortal = other->nLevelDepthPortal;
		this->nLevelAreaDest = other->nLevelAreaDest;
		this->nLevelAreaPortal = other->nLevelAreaPortal;
		this->nPVPWorld = other->nPVPWorld;

		// common
		this->idGame = other->idGame;
		this->idPortal = other->idPortal;
		
	}
	
	//----------------------------------------------------------------------------	
	BOOL IsValid(
		void )
	{
#pragma warning(suppress:4127) //conditional expression is constant
		if (AppIsHellgate())
		{
			return ( this->nLevelDefDest != INVALID_LINK && this->nLevelDefPortal != INVALID_LINK );
		}
#pragma warning(suppress:4127) //conditional expression is constant
		else if (AppIsTugboat())
		{
			return ( this->nLevelDepthDest != INVALID_LINK && this->nLevelDepthPortal != INVALID_LINK );
		}
		else
		{
			ASSERT_MSG( "Unknown app" );
			return FALSE;
		}
	}
	
	//----------------------------------------------------------------------------	
	BOOL Equals(
		const TOWN_PORTAL_SPEC * other )
	{
		ASSERT_RETFALSE(other);
		
#pragma warning(suppress:4127) //conditional expression is constant
		if (AppIsHellgate())
		{
			return( this->idGame == other->idGame
				&& this->nLevelDefDest == other->nLevelDefDest && this->nLevelDefPortal == other->nLevelDefPortal );
		}
#pragma warning(suppress:4127) //conditional expression is constant
		else if (AppIsTugboat())
		{
			return( /*this->idGame == other->idGame
				&& */this->nLevelDepthDest == other->nLevelDepthDest && this->nLevelDepthPortal == other->nLevelDepthPortal &&
					this->nLevelAreaDest == other->nLevelAreaDest && this->nLevelAreaPortal == other->nLevelAreaPortal &&
					this->nPVPWorld == other->nPVPWorld );		
		}
		else
		{
			ASSERT_MSG( "Unknown app" );
			return FALSE;
		}
	}
	
END_MSG_STRUCT

DEF_MSG_SUBSTRUCT( WARP_SPEC )
	MSG_FIELD( 0, int, idLevelDest, INVALID_ID )				// destination level id
	MSG_FIELD( 1, int, nLevelDefDest, INVALID_LINK )			// level definition destination (hellgate)
	MSG_FIELD( 2, int, nLevelAreaPrev, INVALID_LINK )			// level area destination (tugboat)
	MSG_FIELD( 3, int, nLevelAreaDest, INVALID_LINK )			// level area destination (tugboat)
	MSG_FIELD( 4, int, nLevelDepthDest, 0 )						// level depth desttination (tugboat)	
	MSG_FIELD( 5, DWORD, dwWarpFlags, 0 )						// see WARP_FLAGS
	MSG_FIELD( 6, PGUID, guidArrivePortalOwner, INVALID_GUID )	// arrive at this town portal in destination location		
	MSG_FIELD( 7, GAMEID, idGameOverride, INVALID_ID )			// rjd - allow forcing warp to specific game
	MSG_FIELD( 8, PGUID, guidArriveAt, INVALID_GUID )			// arrive at this unit with this guid
	MSG_FIELD( 9, int, nDifficulty, INVALID_LINK)
	MSG_FIELD( 10, int, nPVPWorld, 0)							// warping to a PVP world?
	MSG_FIELD( 11, WORD, wObjectCode, 0)						// warpting to object
END_MSG_STRUCT

DEF_MSG_SUBSTRUCT(CHARACTER_INFO)
	MSG_WCHAR(0, szCharName, MAX_CHARACTER_NAME)
	MSG_FIELD(1, int, nPlayerSlot, -1)
	MSG_FIELD(2, int, eSlotType, 0)
	MSG_FIELD(3, int, nDifficulty, 0)
END_MSG_STRUCT

DEF_MSG_SUBSTRUCT(INVENTORY_LOCATION)
	MSG_FIELD(0, int, nInvLocation, 0)  // INVLOC_NONE
	MSG_FIELD(1, int, nX, 0)
	MSG_FIELD(2, int, nY, 0)
END_MSG_STRUCT

enum STR_PARAM_TYPE
{
	STR_PARAM_TYPE_WSTRING,
	STR_PARAM_TYPE_INVALID
};

DEF_MSG_SUBSTRUCT(STR_PARAM)
	MSG_BLOBW(0, paramData,			MAX_STR_PARAM_SIZE)
	MSG_FIELD(1, BYTE, paramType,	STR_PARAM_TYPE_INVALID)
	MSG_FIELD(2, int, eStringReplacement, -1)
void Set( STR_PARAM * other )
{
	if( other )
	{
		ASSERT(MemoryCopy( paramData, MAX_STR_PARAM_SIZE, other->paramData, MSG_GET_BLOB_LEN(other,paramData) ));
		MSG_SET_BLOB_LEN( (*this), paramData, MSG_GET_BLOB_LEN(other,paramData) );
		paramType = other->paramType;
		eStringReplacement = other->eStringReplacement;
	}
	else
	{
		WORD w4Len = 0;
		MSG_SET_BLOB_LEN( (*this), paramData, w4Len );
		paramType = STR_PARAM_TYPE_INVALID;
		eStringReplacement = -1;
	}
}
void SetWStrMsg(
	const WCHAR * message )
{
	if( message )
	{
		UINT len = PStrCopy(
			(WCHAR*)this->paramData,
			(MAX_STR_PARAM_SIZE/sizeof(WCHAR)) - 1,
			message,
			(MAX_STR_PARAM_SIZE/sizeof(WCHAR)) - 1);
		MSG_SET_BLOB_LEN((*this),paramData,(len+1)*sizeof(WCHAR));
		this->paramType = STR_PARAM_TYPE_WSTRING;
	}
	else
	{
		this->Set(NULL);
	}
}

void SetNumericMsg(
	int nNumber)
{
	{
		UINT len = PStrPrintf(
			(WCHAR*)this->paramData,
			(MAX_STR_PARAM_SIZE/sizeof(WCHAR)) - 1,
			L"%d",
			nNumber);
		MSG_SET_BLOB_LEN((*this),paramData,(len+1)*sizeof(WCHAR));
		this->paramType = STR_PARAM_TYPE_WSTRING;
	}

}

DWORD_PTR GetParam()
{
	//if(paramType == STR_PARAM_TYPE_WSTRING)
	{
		return (DWORD_PTR)paramData;
	}
	//else return NULL;
}

void Init(
	void )
{
	WORD w4Len = 0;
	MSG_SET_BLOB_LEN( (*this), paramData, w4Len );
}
END_MSG_STRUCT

DEF_MSG_SUBSTRUCT(LOCALIZED_PARAM_STR)
	MSG_FIELD(0, int, eGlobalString, -1)
	MSG_STRUC(1, STR_PARAM, param1)
	MSG_STRUC(2, STR_PARAM, param2)
	MSG_FIELD(3,BYTE, bPopup, false)
END_MSG_STRUCT

#endif // _MESSAGE_H_