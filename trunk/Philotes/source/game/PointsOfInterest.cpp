#include "stdafx.h"
#include "PointsOfInterest.h"
#include "excel.h"
#include "excel_private.h"
#include "game.h"
#include "s_message.h"
#include "definition.h"
#include "unit_priv.h"
#include "c_message.h"

//#include "units.h"
//#include "player.h"


using namespace MYTHOS_POINTSOFINTEREST;


//Points of Interest
cPointsOfInterest::cPointsOfInterest( UNIT *pPlayer ):
m_bInited( FALSE ),
m_pPlayer( pPlayer )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitGetGame( pPlayer ) );
	ArrayInit(m_PofIList, GameGetMemPool( GetGame() ), 5 );
	m_bInited = TRUE;
}

//destroys Anchors
cPointsOfInterest::~cPointsOfInterest()
{
	m_pPlayer = NULL;
	//destroy PofI
	if( m_bInited )
	{
		m_PofIList.Destroy();
	}
}

//returns the game for the player
inline GAME * cPointsOfInterest::GetGame()
{
	return UnitGetGame( m_pPlayer );
}

//Returns the point of interest by index
const MYTHOS_POINTSOFINTEREST::PointOfInterest * cPointsOfInterest::GetPointOfInterestByIndex( int nIndex )
{
	ASSERT_RETNULL(nIndex >= 0 && nIndex < (int)m_PofIList.Count());
	return &m_PofIList[nIndex];		//return the anchor as a pointer
}

//returns the index of the point of Interest by UnitData
int cPointsOfInterest::GetPointOfInterestIndexByUnitData( const UNIT_DATA *pUnitData )
{
	ASSERT_RETINVALID( m_bInited );
	ASSERT_RETINVALID( pUnitData );
	for( int t = 0; t < GetPointOfInterestCount(); t++ )
	{
		if( m_PofIList[ t ].pUnitData == pUnitData )
		{
			return t;
		}
	}
	return -1;
}

//Returns the number of Points of Interest
int cPointsOfInterest::GetPointOfInterestCount()
{
	return (int)m_PofIList.Count();
}


//sends an update for the marker
void cPointsOfInterest::SendMessageForMarker( int nMarkerIndex, EPOINTSOFINTEREST_MESSAGE_TYPES nMessageType )
{
	ASSERT_RETURN( m_bInited );
	ASSERT_RETURN( m_pPlayer );
	MSG_SCMD_POINT_OF_INTEREST message;
	if( nMarkerIndex != INVALID_ID )
	{
		const MYTHOS_POINTSOFINTEREST::PointOfInterest *pPOfI = GetPointOfInterestByIndex( nMarkerIndex );
		ASSERT_RETURN( pPOfI );
		message.nFlags = pPOfI->nFlags;
		message.nUnitId = pPOfI->nUnitID;
		message.nX = pPOfI->nPosX;
		message.nY = pPOfI->nPosY;		
		message.nClass = pPOfI->nClass;
	}
	message.nMessageType = (BYTE)nMessageType;	

	// send it
	s_SendMessage( GetGame(), 
		UnitGetClientId( m_pPlayer ),
		SCMD_POINT_OF_INTEREST, 
		&message );

}

//Adds a unit as a point of Interest
void cPointsOfInterest::AddPointOfInterest( UNIT *pUnit, int nStringID, int nFlags )
{
	ASSERT_RETURN( m_bInited );
	ASSERT_RETURN( pUnit );
	if( UnitGetGenus( pUnit ) == GENUS_MONSTER )
	{
		SET_MASK( nFlags, KPofI_Flag_IsMonster );
		CLEAR_MASK( nFlags, KPofI_Flag_IsObject );
	}
	else if( UnitGetGenus( pUnit ) == GENUS_OBJECT )
	{
		CLEAR_MASK( nFlags, KPofI_Flag_IsMonster );
		SET_MASK( nFlags, KPofI_Flag_IsObject );
	}
	else
	{
		ASSERTX_RETURN( FALSE, "Can only have Monsters and Objects as points of Interest" );
	}
	VECTOR vPosition = UnitGetPosition( pUnit );
	AddPointOfInterest( pUnit->pUnitData, UnitGetId( pUnit ), UnitGetClass( pUnit ), (int)vPosition.fX, (int)vPosition.fY, nStringID, nFlags );
}

//Adds a point of interest
void cPointsOfInterest::AddPointOfInterest( const UNIT_DATA *pUnitData, int UnitId, int nClass, int nPosX, int nPosY, int nStringID, int nFlags )
{
	ASSERT_RETURN( m_bInited );
	ASSERT_RETURN( TEST_MASK( nFlags, KPofI_Flag_IsMonster ) ||  TEST_MASK( nFlags, KPofI_Flag_IsObject ) );
	MYTHOS_POINTSOFINTEREST::PointOfInterest PointOfInterest;
	PointOfInterest.nUnitID = UnitId;
	PointOfInterest.nPosX = nPosX;
	PointOfInterest.nPosY = nPosY;
	PointOfInterest.nStringId = ( pUnitData && nStringID == INVALID_ID )?pUnitData->nString:nStringID;
	PointOfInterest.nFlags = nFlags;
	PointOfInterest.nClass = nClass;
	PointOfInterest.pUnitData = pUnitData;
	ArrayAddItem(m_PofIList, PointOfInterest);	
	if( IS_SERVER( GetGame() ) )
	{
		SendMessageForMarker( GetPointOfInterestCount() - 1, KPofI_MessageType_Add );
	}
}
//Removes a point of interest
void cPointsOfInterest::RemovePointOfInterest( int UnitId )
{
	ASSERT_RETURN( m_bInited );
	ASSERT_RETURN( UnitId != INVALID_ID );
	for( int t = 0; t < GetPointOfInterestCount(); t++ )
	{
		if( m_PofIList[ t ].nUnitID == UnitId )
		{
			if( IS_SERVER( GetGame() ) )
			{
				SendMessageForMarker( t, KPofI_MessageType_Remove ); //remove the item on the client
			}

			ArrayRemoveByIndex( m_PofIList, t );
			t--;
		}
	}
}

//clears all the points of interest
void cPointsOfInterest::ClearAllPointsOfInterest()
{
	ASSERT_RETURN( m_bInited );
	ArrayClear( m_PofIList );
	if( IS_SERVER( GetGame() ) )
	{
		SendMessageForMarker( INVALID_ID, KPofI_MessageType_RemoveAll ); //remove the item on the client
	}
}


//tells the client to refresh
void cPointsOfInterest::ClientRefresh()
{
	if( IS_SERVER( GetGame() ) )
	{
		SendMessageForMarker( INVALID_ID, KPofI_MessageType_Refresh ); //remove the item on the client
	}
	else
	{

	}
}

//sets the string on the point of interest
void cPointsOfInterest::SetString( int nIndex, int nStringId )
{
	ASSERT_RETURN(nIndex >= 0 && nIndex < (int)m_PofIList.Count());
	PointOfInterest *pPointOfInterest = &m_PofIList[nIndex];		//return the anchor as a pointer	
	ASSERT_RETURN( pPointOfInterest );
	pPointOfInterest->nStringId = nStringId;
}

//Adds a flags to the point of interest
void cPointsOfInterest::AddFlag( int nIndex, EPOINTOFINTERESTFLAGS nFlag )
{
	ASSERT_RETURN(nIndex >= 0 && nIndex < (int)m_PofIList.Count());
	PointOfInterest *pPointOfInterest = &m_PofIList[nIndex];		//return the anchor as a pointer	
	ASSERT_RETURN( pPointOfInterest );
	SET_MASK( pPointOfInterest->nFlags, nFlag );	
}

//Clears a flags to the point of interest
void cPointsOfInterest::ClearFlag( int nIndex, EPOINTOFINTERESTFLAGS nFlag )
{
	ASSERT_RETURN(nIndex >= 0 && nIndex < (int)m_PofIList.Count());
	PointOfInterest *pPointOfInterest = &m_PofIList[nIndex];		//return the anchor as a pointer	
	ASSERT_RETURN( pPointOfInterest );
	CLEAR_MASK( pPointOfInterest->nFlags, nFlag );	
}

//Checks to see if it has the flag
BOOL cPointsOfInterest::HasFlag( int nIndex, EPOINTOFINTERESTFLAGS nFlag )
{
	ASSERT_RETFALSE(nIndex >= 0 && nIndex < (int)m_PofIList.Count());
	PointOfInterest *pPointOfInterest = &m_PofIList[nIndex];		//return the anchor as a pointer	
	ASSERT_RETFALSE( pPointOfInterest );
	return TEST_MASK( pPointOfInterest->nFlags, nFlag );
}

//Returns the Level Area ID of the Point of Interest if it has one
int cPointsOfInterest::GetLevelAreaID( int nIndex )
{
	ASSERT_RETFALSE(nIndex >= 0 && nIndex < (int)m_PofIList.Count());
	PointOfInterest *pPointOfInterest = &m_PofIList[nIndex];		//return the anchor as a pointer	
	ASSERT_RETFALSE( pPointOfInterest );
	if( !pPointOfInterest->pUnitData ||
		pPointOfInterest->pUnitData->nLinkToLevelArea == INVALID_ID )
	{
		return INVALID_ID;
	}
	return MYTHOS_LEVELAREAS::LevelAreaGenerateLevelIDByPos( pPointOfInterest->pUnitData->nLinkToLevelArea, pPointOfInterest->nPosX, pPointOfInterest->nPosY );
}
	
	

