//----------------------------------------------------------------------------
// FILE: pets.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __PETS_H_
#define __PETS_H_

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
typedef void (* PFN_PET_CALLBACK)( UNIT *pPet, void *pUserData );

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

BOOL PetCanAdd(
	UNIT * pOwner,
	int nPowerCost,
	int nInventoryLocation);

BOOL PetAdd(
	UNIT * pPet,
	UNIT * pOwner,
	int nInventoryLocation,
	int nRequiredOwnerSkill=INVALID_ID,
	int nOwnerMaxPowerPenalty=0,
	BOOL bAddToInventory=TRUE);

void s_SendPetMessage(
	GAME * game,
	UNIT * owner,
	UNIT * pet);

void PetsReapplyPowerCostToOwner(
	UNIT * pOwner);

int PetGetPetMaxPowerPenalty(
	UNIT * pOwner,
	UNIT * pPet);

BOOL PetIsPet(
	UNIT * pUnit);

BOOL PetIsPlayerPet(
	GAME * pGame,
	UNIT * pUnit);

UNITID PetGetOwner(
	UNIT *pUnit);

int PetIteratePets(
	UNIT * pOwner,
	PFN_PET_CALLBACK pfnCallback,
	void * pUserData);
	
#endif
