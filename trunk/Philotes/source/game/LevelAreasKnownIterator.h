#pragma once
#ifndef __LEVELAREAITERATOR_H_
#define __LEVELAREAITERATOR_H_
#ifndef _UNITS_H_
#include "units.h"
#endif
namespace MYTHOS_LEVELAREAS
{

const enum ELVL_AREA_ITER_FLAGS
{
	KLVL_AREA_ITER_NO_MAPS = 1,	
	KLVL_AREA_ITER_IGNORE_INVISIBLE_AREAS = 2,
	KLVL_AREA_ITER_NO_TEAM = 4,
	KLVL_AREA_ITER_DONT_GET_POSITION = 8,
	KLVL_AREA_ITER_SKIP_CURRENT_PLAYER_POS = 16,
	KLVL_AREA_ITER_IGNORE_ZONE = 32
};

class CLevelAreasKnownIterator
{
public:
	CLevelAreasKnownIterator( UNIT *pPlayerUnit );
	~CLevelAreasKnownIterator( void );
	inline int GetLevelAreaIDCurrent(){ return m_ID; }
	inline UNIT * GetPlayerUnit(){ return m_pPlayer; }
	int GetNextLevelAreaID( int nFlags = 0 );
	float GetLevelAreaPixelX(){ return m_fPixelX; }
	float GetLevelAreaPixelY(){ return m_fPixelY; }
	float GetLevelAreaWorldMapPixelX(){ return m_fWorldMapPixelX; }
	float GetLevelAreaWorldMapPixelY(){ return m_fWorldMapPixelY; }
private:
const enum ELevelAreaIteratorTests
{
	KITER_PLAYERLEVEL,
	KITER_OPENAREAS,
	KITER_MAPS,
	KITER_TEAM,	
	KITER_COUNT
};
	inline int IterateMapItem( void );
	inline void SetPixelLocation( void );
	inline int TestOutsideAreas( int nFlags );
	inline int IterateTeamMembers( void );
	float						m_fPixelX;
	float						m_fPixelY;
	float						m_fWorldMapPixelX;
	float						m_fWorldMapPixelY;
	int							m_IterIndex;	
	int							m_ID;					//current Level ID
	UNIT						*m_pPlayer;			//Player
	int							m_TableIndex;		//excel tables
	int							m_AlwaysOpen;		//always open level areas
	UNIT						*m_pInventoryItem;  //maps
	BOOL						m_Done;
	int							m_PlayerAreaID;
	int							m_PlayerZone;
	int							m_PartyMemberIndex;
	BOOL						m_TutorialFinished;
	BOOL						m_FoundPlayerArea;
	PList<int>					m_ReturnedIDs;
	
};



} //end namespace
#endif
