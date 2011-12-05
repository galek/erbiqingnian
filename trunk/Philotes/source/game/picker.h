//----------------------------------------------------------------------------
// picker.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// picks an index based on weight, supports either float or int
// call PickerStart, followed by a bunch of PickerAdds, followed by
// PickerChoose.  use PickerStart or PickerStartF for floats
//----------------------------------------------------------------------------
#ifndef _PICKER_H_
#define _PICKER_H_

#ifndef _PICKER_COMMON_H_
#include "picker_common.h"
#endif

#ifndef _GAME_H_
#include "game.h"
#endif


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define PickerStart(container, n)				MY_GAME_PICKER n(container, FALSE);
#define PickerStartF(container, n)				MY_GAME_PICKER n(container, TRUE);


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void PickerInit(
	struct GAME * game);

void PickerFree(
	struct GAME * game);

void PickerResize(
	struct GAME * game,
	PICKER * picker,
	int size);

int PickerChoose(
	struct GAME * game);

int PickerChoose(
	struct GAME * game,
	struct RAND & rand);

int PickerChooseRemove(
	struct GAME * game);

int PickerChooseRemove(
	struct GAME * game,
	struct RAND &rand );

int PickerGetCount(
	GAME * game);


//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline PICKER * PickerGetCurrent(
	GAME * game)
{
	ASSERT_RETNULL(game);
	if (!game->m_pPickers)
	{
		PickerInit(game);
	}
	PICKERS * pickers = game->m_pPickers;
	return PickerGetCurrent(pickers);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline PICKER * PickerGetNew(
	GAME * game)
{
	ASSERT_RETNULL(game);
	if (!game->m_pPickers)
	{
		PickerInit(game);
	}
	PICKERS * pickers = game->m_pPickers;
	return PickerGetNew(pickers);
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerEnd(
	GAME * game)
{
	ASSERT_RETURN(game);
	PICKERS * pickers = game->m_pPickers;
	PickerEnd(pickers);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerResize(MEMORY_FUNCARGS(
	GAME * game,
	PICKER * picker,
	int size))
{
	ASSERT_RETURN(game && picker);

	if (picker->m_nListSize >= size)
	{
		return;
	}
	picker->m_nListSize = size;
	picker->m_pList = (PICK_NODE *)GREALLOCFL(game, picker->m_pList, picker->m_nListSize * sizeof(PICK_NODE), file, line);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void _PickerStart(
	GAME * game)
{
	PICKER * picker = PickerGetNew(game);
	ASSERT_RETURN(picker);
	PickerReset(picker, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void _PickerStartF(
	GAME * game)
{
	PICKER * picker = PickerGetNew(game);
	ASSERT_RETURN(picker);
	PickerReset(picker, TRUE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerAdd(MEMORY_FUNCARGS(
	GAME * game,
	int	index,
	int weight))
{
	PICKER * picker = PickerGetCurrent(game);
	ASSERT_RETURN(picker);

	ASSERT_RETURN(!picker->m_bFloatWeights);
	if (picker->m_nCurrent >= picker->m_nListSize)
	{
		PickerResize(MEMORY_FUNC_PASSFL(game, picker, picker->m_nCurrent + PICKER_SIZE_INCREMENT));
	}
	
	PickerAdd(picker, index, weight);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PickerAdd(MEMORY_FUNCARGS(
	GAME * game,
	int	index,
	float weight))
{
	PICKER * picker = PickerGetCurrent(game);
	ASSERT_RETURN(picker);

	ASSERT_RETURN(picker->m_bFloatWeights);
	if (picker->m_nCurrent >= picker->m_nListSize)
	{
		PickerResize(MEMORY_FUNC_PASSFL(game, picker, picker->m_nCurrent + PICKER_SIZE_INCREMENT));
	}
	PickerAdd(picker, index, weight);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL PickerIsEmpty(
	GAME * game)
{
	PICKER * picker = PickerGetCurrent(game);
	ASSERT_RETVAL(picker, TRUE);

	return PickerIsEmpty(picker);
}


//----------------------------------------------------------------------------
// Helper Class
//----------------------------------------------------------------------------
struct MY_GAME_PICKER
{
	struct GAME * m_pGame;

	MY_GAME_PICKER(
		GAME * game,
		BOOL bFloat) : m_pGame(game)
	{
		if (bFloat)
		{
			_PickerStartF(game);
		}
		else
		{
			_PickerStart(game);
		}
	}
	~MY_GAME_PICKER()
	{
		if(m_pGame)
		{
			PickerEnd(m_pGame);
		}
	}
};


#endif  // _PICKER_H_

