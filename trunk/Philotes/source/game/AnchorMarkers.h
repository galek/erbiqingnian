//----------------------------------------------------------------------------
// FILE: AnchorMarkers.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __ANCHORMARKERS_H_
#define __ANCHORMARKERS_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _UNITS_H_
#include "units.h"
#endif

struct DRLG_PASS;
struct STATS;
struct LEVEL;

namespace MYTHOS_ANCHORMARKERS
{

struct ANCHOR
{
	float	fX;
	float	fY;
	int		nExcelLineNumber;	
	int		nUnitId;
};

class cAnchorMarkers
{
public:
	cAnchorMarkers();
	~cAnchorMarkers();
	void Init( GAME *pGame );

	const MYTHOS_ANCHORMARKERS::ANCHOR * GetAnchorByIndex( int nIndex );
	const MYTHOS_ANCHORMARKERS::ANCHOR * GetAnchorByObjectCode( WORD wCode );
	const MYTHOS_ANCHORMARKERS::ANCHOR * GetAnchorByUnitData( const UNIT_DATA *pUnitData );
	int GetAnchorCount();
	//Unit can be NULL
	void Clear( void );
	void AddAnchor( GAME *pGame, int nExcelLineNumber, float fX, float fY, UNIT *pUnit = NULL );
	void SetAnchorVisited( UNIT *pPlayer, UNIT *pAnchor );
	void SendAnchorsToClient( GAME * pGame, GAMECLIENTID idClient, LEVELID  idLevel );	
	void c_AnchorMarkerSetState( UNIT *pPlayer, UNIT *pAnchor );
	BOOL HasAnchorBeenVisited( UNIT *pPlayer, UNIT *pAnchor );
	BOOL HasAnchorBeenVisited( UNIT *pPlayer, int nExcelLineNumber );
	BOOL HasAnchorBeenVisited( UNIT *pPlayer, const MYTHOS_ANCHORMARKERS::ANCHOR *pAnchor );
	void SetAnchorAsRespawnLocation( UNIT *pPlayer, UNIT *pAnchor );
private:
	SIMPLE_DYNAMIC_ARRAY<ANCHOR>		m_AchorList;
	BOOL								m_bInited;

}; //end namespace
}
#endif
