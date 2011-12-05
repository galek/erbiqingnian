#include "stdafx.h"
#include "AnchorMarkers.h"
#include "excel.h"
#include "player.h"
#include "level.h"
#include "excel_private.h"
#include "game.h"
#include "objects.h"
#include "definition.h"
#include "units.h"
#include "s_message.h"
#include "c_message.h"
#include "states.h"

using namespace MYTHOS_ANCHORMARKERS;

//init Anchor Makers
cAnchorMarkers::cAnchorMarkers() :
				m_bInited( FALSE )
{

}
//destroys Anchors
cAnchorMarkers::~cAnchorMarkers()
{
	//destroy anchors
	if( m_bInited )
	{
		m_AchorList.Destroy();
	}
}

void cAnchorMarkers::Init( GAME *pGame )
{
	ASSERT_RETURN( pGame );
	//init array
	ArrayInit(m_AchorList, GameGetMemPool( pGame ), 5 );
	m_bInited = TRUE;
}
//Returns an Anchor struct
const MYTHOS_ANCHORMARKERS::ANCHOR * cAnchorMarkers::GetAnchorByIndex( int nIndex )
{
	ASSERT_RETNULL(nIndex >= 0 && nIndex < (int)m_AchorList.Count());
	return &m_AchorList[nIndex];		//return the anchor as a pointer
}
//Returns an Anchor struct
const MYTHOS_ANCHORMARKERS::ANCHOR * cAnchorMarkers::GetAnchorByObjectCode( WORD wCode )
{
	int nLine = (int)ExcelGetLineByCode( EXCEL_CONTEXT( NULL ), DATATABLE_OBJECTS, wCode );
	for( int t = 0; t < (int)m_AchorList.Count(); t++ )
	{
		if( nLine == m_AchorList[t].nExcelLineNumber )
		{
			return &m_AchorList[t];
		}
	}
	return NULL;
}
//Returns an Anchor struct
const MYTHOS_ANCHORMARKERS::ANCHOR * cAnchorMarkers::GetAnchorByUnitData( const UNIT_DATA *pUnitData )
{
	return GetAnchorByObjectCode( pUnitData->wCode );
}



//returns the number of Anchors in the AnchorMakers
int cAnchorMarkers::GetAnchorCount()
{
	return m_AchorList.Count();
}

void cAnchorMarkers::Clear( void )
{
	ArrayClear( m_AchorList );
}

//adds an anchor by code and position
void cAnchorMarkers::AddAnchor( GAME *pGame, int nExcelLineNumber, float fX, float fY, UNIT *pUnit )
{
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( nExcelLineNumber != INVALID_ID );
	ASSERT_RETURN( fX != 0.0f && fY != 0.0f );	
	if( IS_SERVER( pGame ) )
	{
		ASSERT_RETURN( pUnit );
		ASSERT_RETURN( UnitGetLevel( pUnit ) );
	}
	ANCHOR nAnchor;
	nAnchor.nExcelLineNumber = nExcelLineNumber;
	nAnchor.fX = fX;
	nAnchor.fY = fY;	
	if( pUnit )
	{
		nAnchor.nUnitId = UnitGetId( pUnit );
	}
	ArrayAddItem(m_AchorList, nAnchor);	
	
}

void cAnchorMarkers::SendAnchorsToClient( GAME * pGame, 
										  GAMECLIENTID idClient,
										  LEVELID  idLevel )
{
	int nAnchorCount = GetAnchorCount();

	// send a notice that we need to clear out old markers first
	MSG_SCMD_ANCHOR_MARKER intromessage;

	intromessage.nLevelId = idLevel;
	intromessage.nUnitId = INVALID_ID;
	// send it
	s_SendMessage( pGame, idClient, SCMD_ANCHOR_MARKER, &intromessage );

	for( int i = 0; i < nAnchorCount; i++ )
	{
		const MYTHOS_ANCHORMARKERS::ANCHOR *pAnchor = GetAnchorByIndex( i );

		MSG_SCMD_ANCHOR_MARKER message;

		message.nLevelId = idLevel;
		message.nUnitId = pAnchor->nUnitId;
		message.fX = pAnchor->fX;
		message.fY = pAnchor->fY;
		message.nBaseRow = pAnchor->nExcelLineNumber;		
		// send it
		s_SendMessage( pGame, idClient, SCMD_ANCHOR_MARKER, &message );
	}
	
}
//sets the anchor as visited
void cAnchorMarkers::c_AnchorMarkerSetState( UNIT *pPlayer, 
											 UNIT *pAnchor )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pAnchor );
	ASSERT_RETURN( IS_CLIENT( UnitGetGame( pPlayer ) ) );		//server only	

	if( HasAnchorBeenVisited( pPlayer, pAnchor ) )
	{
		c_StateSet( pAnchor, pAnchor, STATE_ANCHORSTONE_ACTIVE, 0, 0, INVALID_LINK );

	}
}

//sets the anchor as visited
void cAnchorMarkers::SetAnchorVisited( UNIT *pPlayer, UNIT *pAnchor )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pAnchor );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( pPlayer ) ) );		//server only	
	int nExcelIndex = ExcelGetLineByCode( EXCEL_CONTEXT( UnitGetGame( pPlayer ) ), DATATABLE_OBJECTS,  pAnchor->pUnitData->wCode );
	UnitSetStat( pPlayer, STATS_ANCHORMARKERS_VISITED, nExcelIndex, 1 );

}

//returns TRUE if the anchor has been visited
BOOL cAnchorMarkers::HasAnchorBeenVisited( UNIT *pPlayer, UNIT *pAnchor )
{
	ASSERT_RETFALSE( pAnchor );
	int nExcelIndex = ExcelGetLineByCode( EXCEL_CONTEXT( UnitGetGame( pPlayer ) ), DATATABLE_OBJECTS,  pAnchor->pUnitData->wCode );
	return HasAnchorBeenVisited( pPlayer, nExcelIndex );
}
//returns TRUE if the anchor has been visited
BOOL cAnchorMarkers::HasAnchorBeenVisited( UNIT *pPlayer, int nExcelLineNumber )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( nExcelLineNumber != INVALID_ID );
	int nVisited = UnitGetStat( pPlayer, STATS_ANCHORMARKERS_VISITED, nExcelLineNumber );
	return  nVisited == 1;
}
//returns TRUE if the anchor has been visited
BOOL cAnchorMarkers::HasAnchorBeenVisited( UNIT *pPlayer, const MYTHOS_ANCHORMARKERS::ANCHOR *pAnchor )
{
	ASSERT_RETFALSE( pAnchor );
	return HasAnchorBeenVisited( pPlayer, pAnchor->nExcelLineNumber );
}
//sets the anchor as the players respawn location
void cAnchorMarkers::SetAnchorAsRespawnLocation( UNIT *pPlayer, UNIT *pAnchor )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pAnchor );
	PlayerSetRespawnLocation( KRESPAWN_TYPE_ANCHORSTONE, pPlayer, pAnchor, UnitGetLevel( pPlayer ) );
}