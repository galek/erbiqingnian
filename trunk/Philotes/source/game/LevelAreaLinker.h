//----------------------------------------------------------------------------
// FILE: LevelAreaLinker.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __LEVELAREALINKER_H_
#define __LEVELAREALINKER_H_
struct UNIT;
struct LEVEL_TYPE;
struct LEVEL;
struct WARP_SPEC;

#include "excel.h"
namespace MYTHOS_LEVELAREAS
{



const enum ELevelAreaLinker
{
	KLEVELAREA_LINK_NEXT,
	KLEVELAREA_LINK_PREVIOUS
};

//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------
	struct LEVELAREA_LINK
	{		
		int				m_LevelAreaID;
		int				m_LevelAreaIDPrevious;
		int				m_LevelAreaIDNext;
		int				m_LevelAreaIDLinkA;
		int				m_LevelAreaIDLinkB;
		BOOL			m_CanTravelFrom;
		int				m_HubRadius;
	};

//----------------------------------------------------------------------------
// function
//----------------------------------------------------------------------------
	int GetLevelAreaID_First_BetweenLevelAreas( int nLevelAreaAt, int nLevelAreaGoingTo );
	int GetLevelAreaID_NextAndPrev_BetweenLevelAreas( int nLevelAreaAt, ELevelAreaLinker nLinkType, int nDepth, BOOL bCheckLinks = FALSE );
	BOOL FillOutLevelTypeForWarpObject( const LEVEL *pLevel, UNIT *pPlayer, LEVEL_TYPE *levelType, ELevelAreaLinker nLinkType  );
	int GetLevelAreaIDByStairs( UNIT *pStairs );
	BOOL IsLevelAreaAHub( int nLevelAreaID );
	BOOL ShouldStartAtLinkA( int nLevelAreaAt, int nLevelAreaGoingTo );
	int GetLevelAreaLinkerRadius( int nLevelArea );
	BOOL PlayerCanTravelToLevelArea( UNIT *pPlayer, int nLevelAreaGoingTo );
	BOOL CanWarpToSection( UNIT *pPlayer, int nLevelAreaGoingTo );
	void SetPlayerVisitedHub( UNIT *pPlayer, int nLevelAreaHub );
	BOOL HasPlayerVisitedHub( UNIT *pPlayer, int nLevelAreaHub );
	BOOL HasPlayerVisitedAConnectedHub( UNIT *pPlayer, int nLevelAreaHub );
	void ConfigureLevelAreaByLinker( WARP_SPEC &tSpec, UNIT *pPlayer, BOOL allowRoadSkip );
	void InitHubLinks();

	inline const MYTHOS_LEVELAREAS::LEVELAREA_LINK * GetLevelAreaLinkByAreaID( int nLevelAreaID )
	{

		//first we must find the first level area that links to area's together.
		if( nLevelAreaID != INVALID_ID )
		{
			int nCount = ExcelGetCount( NULL, DATATABLE_LEVEL_AREAS_LINKER );
			for( int t = 0; t < nCount; t++ )
			{
				const MYTHOS_LEVELAREAS::LEVELAREA_LINK *linker = (const MYTHOS_LEVELAREAS::LEVELAREA_LINK *)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS_LINKER, t );
				if( linker->m_LevelAreaID == nLevelAreaID )
					return linker;
			}
		}
		return NULL;
	}

};



#endif
