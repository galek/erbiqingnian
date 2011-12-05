//----------------------------------------------------------------------------
// ctf.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "pvp.h"
#include "ctf.h"
#include "duel.h"

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PvPGetGameData(PvPGameData *pPvPGameData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(pPvPGameData);

	ZeroMemory(pPvPGameData, sizeof(PvPGameData));	

	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		switch (GameGetSubType(pGame))
		{
		case GAME_SUBTYPE_PVP_CTF:
			return c_CTFGetGameData(pPvPGameData);

		case GAME_SUBTYPE_PVP_PRIVATE:
		case GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED:
			return c_DuelGetGameData(pPvPGameData);

		}
	}

#endif //!ISVERSION(SERVER_VERSION)

	return FALSE;
}