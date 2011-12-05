//----------------------------------------------------------------------------
// Prime v2.0
//
// groups.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
// contains:
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "units.h"
#include "groups.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_GROUP_SIZE		6

struct GROUP
{
	UNIT_GROUP_ID		idGroup;

	GAMEID		idGame;
	int			nGroupSize;
	UNITID		idUnitList[ MAX_GROUP_SIZE ];

	UNITID		idLeader;

	LOOT_MODES	eLootMode;
	
	GROUP *		next;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

GROUP_ERROR GroupAddUnit( GAME * game, UNIT * leader, UNIT * invite )
{
	if ( UnitGetTeam( leader ) != UnitGetTeam( invite ) )
		return GROUP_ERROR_INVALIDPARAM;

	if ( UnitGetGenus( leader ) != GENUS_PLAYER )
		return GROUP_ERROR_INVALIDPARAM;

	if ( UnitGetGenus( invite ) != GENUS_PLAYER )
		return GROUP_ERROR_INVALIDPARAM;

	if ( leader->idGroup == INVALID_ID )
	{
	}
	else
	{
		GROUP * group = game->m_GroupList;
		while ( group && ( group->idGroup != leader->idGroup ) )
			group = group->next;

		if ( !group )
			return GROUP_ERROR_INVALIDPARAM;

		if ( group->nGroupSize >= MAX_GROUP_SIZE )
			return GROUP_ERROR_FULL;

		if ( group->idLeader != UnitGetId( leader ) )
			return GROUP_ERROR_NOTLEADER;

		if ( leader == invite )
			return GROUP_ERROR_INVALIDPARAM;

		int index = 0;
		while ( ( index < MAX_GROUP_SIZE ) && ( group->idUnitList[ index ] != INVALID_ID ) )
			index++;

		ASSERT( index < MAX_GROUP_SIZE );	// just to catch if this ever happens... dealt with below
		if ( index >= MAX_GROUP_SIZE )
			return GROUP_ERROR_FULL;

		group->idUnitList[index] = UnitGetId( invite );
		group->nGroupSize++;
	}

	return GROUP_ERROR_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

GROUP_ERROR GroupRemoveUnit( GAME * game, UNIT * toremove )
{
	return GROUP_ERROR_NONE;
}
