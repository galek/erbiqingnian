//----------------------------------------------------------------------------
//	gamerand.h
//
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _GAMERAND_H_
#define _GAMERAND_H_


//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
// [0, DWORD_MAX]
inline DWORD RandGetNum(
	GAME * game)
{
	ASSERT_RETZERO( game );
	return RandGetNum(game->rand);
}


// [0, max)
inline DWORD RandGetNum(
	GAME * game,
	int max)
{
	ASSERT_RETZERO( game );
	return RandGetNum(game->rand, max);
}


// [min, max]
inline DWORD RandGetNum(
	GAME * game,
	int min,
	int max)
{
	ASSERT_RETZERO( game );
	return RandGetNum(game->rand, min, max);
}


// [0, 1]
inline float RandGetFloat(
	GAME * game)
{
	ASSERT_RETZERO( game );
	return RandGetFloat(game->rand);
}


// [0, max]
inline float RandGetFloat(
	GAME * game,
	float max)
{
	ASSERT_RETZERO( game );
	return RandGetFloat(game->rand, max);
}


// [min, max]
inline float RandGetFloat(
	GAME * game,
	float min,
	float max)
{
	ASSERT_RETZERO( game );
	return RandGetFloat(game->rand, min, max);
}

// boolean selection
inline BOOL RandSelect(
	GAME * game,
	int chance,
	int range)
{
	ASSERT_RETZERO( game );
	return RandSelect(game->rand, chance, range);
}

// [-1, 1] per component - Not normalized
inline VECTOR RandGetVector(
	GAME * game)
{
	ASSERT_RETVAL( game, VECTOR(0) );
	return RandGetVector(game->rand);
}

// [-1, 1] per component - Not normalized
inline VECTOR RandGetVectorXY(
	GAME * game)
{
	ASSERT_RETVAL( game, VECTOR(0) );
	return RandGetVectorXY(game->rand);
}

// even distribution
inline VECTOR RandGetVectorEx(
	GAME * game)
{
	ASSERT_RETVAL( game, VECTOR(0, 0, 1) );
	return RandGetVectorEx(game->rand);
}


#endif // _GAMERAND_H_
