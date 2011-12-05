//----------------------------------------------------------------------------
// gameunits.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _GAMEUNITS_H_
#define _GAMEUNITS_H_


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _GAME_H_
#error gameunits.h requires game.h be included first
#endif

#ifndef _UNITS_H_
#error gameunits.h requires units.h be included first
#endif


// CHB 2007.03.12 - Add references to silence warnings about unreferenced variables.
#if ISVERSION(CLIENT_ONLY_VERSION)

#define IS_CLIENT(x) (REF(x), TRUE)
#define IS_SERVER(x) (REF(x), FALSE)

#elif ISVERSION(SERVER_VERSION)

#define IS_CLIENT(x) (REF(x), FALSE)
#define IS_SERVER(x) (REF(x), TRUE)

#else

//----------------------------------------------------------------------------
inline BOOL IS_CLIENT(
	const UNIT* unit)
{
	ASSERT(unit);
	return IS_CLIENT(UnitGetGame(unit));
}

//----------------------------------------------------------------------------
inline BOOL IS_SERVER(
	const UNIT* unit)
{
	ASSERT_RETFALSE(unit);
	return IS_SERVER(UnitGetGame(unit));
}

#endif

//----------------------------------------------------------------------------
inline char HOST_CHARACTER(
	const GAME * game)
{
	ASSERT_RETVAL(game, 'x');
	return IS_SERVER(game) ? 's' : 'c';
}

//----------------------------------------------------------------------------
inline char HOST_CHARACTER(
	const UNIT * unit)
{
	ASSERT_RETVAL(unit, 'x');
	return IS_SERVER(unit) ? 's' : 'c';
}

//----------------------------------------------------------------------------
extern int c_AppearanceGetWardrobe( int nAppearanceId );
inline int UnitGetWardrobe(
	UNIT* unit)
{
	if ( IS_SERVER( unit ) )
		return unit->nWardrobe;
	CLT_VERSION_ONLY( return c_AppearanceGetWardrobe( c_UnitGetModelIdThirdPerson( unit ) ) );
	SVR_VERSION_ONLY( return INVALID_ID );
}


#endif
