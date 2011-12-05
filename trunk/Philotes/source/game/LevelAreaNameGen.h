#pragma once
#ifndef __LEVELAREANAMEGEN_H_
#define __LEVELAREANAMEGEN_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _UNITS_H_
#include "units.h"
#endif

struct DRLG_PASS;
struct STATS;
struct LEVEL;

namespace MYTHOS_LEVELAREAS
{

#define MADLIB_ENTRY(s)		struct s{ int m_StringIndex; };

const enum MYTHOS_MADLIB_TYPES
{
	MYTHOS_MADLIB_SENTENCES,
	MYTHOS_MADLIB_PROPERNAMES,
	MYTHOS_MADLIB_ADJECTIVES,
	MYTHOS_MADLIB_NOUNS,
	MYTHOS_MADLIB_AFFIXS,
	MYTHOS_MADLIB_SUFFIXS,
	MYTHOS_MADLIB_COUNT
};
//----------------------------------------------------------------------------
struct LEVELAREA_MADLIB_ENTRY
{
	int				m_StringIndex;	
};

MADLIB_ENTRY( MADLIB_SENTENCE );
MADLIB_ENTRY( MADLIB_CAVE_NAMES );
MADLIB_ENTRY( MADLIB_TEMPLE_NAMES );
MADLIB_ENTRY( MADLIB_GOTHIC_NAMES );
MADLIB_ENTRY( MADLIB_DESERTGOTHIC_NAMES );
MADLIB_ENTRY( MADLIB_GOBLIN_NAMES );
MADLIB_ENTRY( MADLIB_HEATH_NAMES );
MADLIB_ENTRY( MADLIB_CANYON_NAMES );
MADLIB_ENTRY( MADLIB_FOREST_NAMES );
MADLIB_ENTRY( MADLIB_FARMLAND_NAMES );
MADLIB_ENTRY( MADLIB_ADJECTIVES );
MADLIB_ENTRY( MADLIB_ADJ_BRIGHT );
MADLIB_ENTRY( MADLIB_AFFIXS );
MADLIB_ENTRY( MADLIB_SUFFIXS );
MADLIB_ENTRY( MADLIB_PROPERNAMEZONE1 );
MADLIB_ENTRY( MADLIB_PROPERNAMEZONE2 );
MADLIB_ENTRY( MADLIB_GOBLIN_PROPERNAMES );
MADLIB_ENTRY( MADLIB_MINES_NAMES );
MADLIB_ENTRY( MADLIB_CELLAR_NAMES );

//----------------------------------------------------------------------------
void GetRandomLibName( const LEVEL *pLevel,
					   int nLevelAreaID,
					   int nLevelAreaDepth,
					   WCHAR *puszBuffer,
					   int nBufferSize,
					   BOOL bForceGeneralNameOfArea = FALSE );


}

#endif