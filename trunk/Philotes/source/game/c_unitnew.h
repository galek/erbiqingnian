//----------------------------------------------------------------------------
// FILE: c_unitnew.h
//----------------------------------------------------------------------------

#ifndef __C_UNITNEW_H_
#define __C_UNITNEW_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct MSG_SCMD_UNIT_NEW;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void c_UnitNewInitForGame(
	GAME *pGame);

void c_UnitNewFreeForGame(
	GAME *pGame);
	
void c_UnitNewMessageReceived(
	GAME *pGame,
	MSG_SCMD_UNIT_NEW *pMessage);

UNIT * c_CreateNewClientOnlyUnit(
	BYTE *pData,
	DWORD dwDataSize);

#endif