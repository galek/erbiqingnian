#include "stdafx.h"
#include "LevelAreasKnownIterator.h"
#include "c_LevelAreasClient.h"
#include "LevelAreaLinker.h"
#include "GameMaps.h"
#include "Excel.h"
#include "Inventory.h"
#include "c_GameMapsClient.h"
#include "ChatServer.h"
#include "Player.h"

using namespace MYTHOS_LEVELAREAS;

CLevelAreasKnownIterator::CLevelAreasKnownIterator( UNIT *pPlayerUnit ):
					m_pPlayer( pPlayerUnit ),
					m_ID( INVALID_ID ),
					m_TableIndex( 0 ),
					m_pInventoryItem( NULL ),
					m_Done( false ),
					m_fPixelX( 0.0f ),
					m_fPixelY( 0.0f ),
					m_fWorldMapPixelX( 0.0f ),
					m_fWorldMapPixelY( 0.0f ),
					m_IterIndex( 0 ),
					m_PartyMemberIndex( 0 ),
					m_PlayerAreaID( INVALID_ID ),
					m_TutorialFinished( FALSE ),
					m_PlayerZone( INVALID_ID ),
					m_AlwaysOpen( 0 )
{
	if( m_pPlayer != NULL )
	{
		if( UnitGetLevel( pPlayerUnit ) )
		{
			m_PlayerAreaID = LevelGetLevelAreaID( UnitGetLevel( pPlayerUnit ) );
		}
		if( m_PlayerAreaID != INVALID_ID )
		{
			m_PlayerZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( m_PlayerAreaID, NULL );
		}

		m_ReturnedIDs.Init();
		m_TutorialFinished = TRUE;//( UnitGetStat( m_pPlayer, STATS_TUTORIAL_FINISHED ) != 0 )?TRUE:FALSE;
		ASSERTX( UnitGetGenus( m_pPlayer ) == GENUS_PLAYER, "Must pass in Player"  );
	}
}

CLevelAreasKnownIterator::~CLevelAreasKnownIterator( void )
{
	m_ReturnedIDs.Destroy(NULL); 
}

inline void CLevelAreasKnownIterator::SetPixelLocation( void )
{
#if !ISVERSION(SERVER_VERSION)
	if( m_pInventoryItem != NULL )
	{
		MYTHOS_MAPS::c_GetMapPixels( m_pInventoryItem, m_fPixelX, m_fPixelY );
	}
	else if( m_ID != INVALID_ID )
	{
		MYTHOS_LEVELAREAS::c_GetLevelAreaPixels( m_ID, INVALID_ID, m_fPixelX, m_fPixelY );
		MYTHOS_LEVELAREAS::c_GetLevelAreaWorldMapPixels( m_ID, INVALID_ID, m_fWorldMapPixelX, m_fWorldMapPixelY );
	}
#endif
}
inline int CLevelAreasKnownIterator::IterateMapItem( void )
{
	if( m_Done )
	{
		m_fPixelX = 0.0f;
		m_fPixelY = 0.0f;
		m_fWorldMapPixelX = 0.0f;
		m_fWorldMapPixelY = 0.0f;
	}

	if( m_pPlayer == NULL )
	{
		return INVALID_ID;
	}
	int location, invX, invY;
	while ( ( m_pInventoryItem = UnitInventoryIterate( m_pPlayer, m_pInventoryItem ) ) != NULL)
	{		
		UnitGetInventoryLocation( m_pInventoryItem, &location, &invX, &invY );
		if( MYTHOS_MAPS::IsMapKnown( m_pInventoryItem ) )
		{		
			m_ID = MYTHOS_MAPS::GetLevelAreaID( m_pInventoryItem );
			return m_ID;
		}
	}	
	return INVALID_ID;
}

inline int CLevelAreasKnownIterator::TestOutsideAreas( int nFlags )
{
	int nCount = ExcelGetCount( NULL, DATATABLE_LEVEL_AREAS_LINKER );	
	while( m_TableIndex < nCount )
	{
		const LEVELAREA_LINK *linker = (const MYTHOS_LEVELAREAS::LEVELAREA_LINK *)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS_LINKER, m_TableIndex );
		m_TableIndex++;		
		//( MYTHOS_LEVELAREAS::HasPlayerVisitedHub( m_pPlayer, linker->m_LevelAreaID ) ||
		//  MYTHOS_LEVELAREAS::HasPlayerVisitedAConnectedHub(m_pPlayer, linker->m_LevelAreaID )))
		if( linker &&
			( ( nFlags & KLVL_AREA_ITER_IGNORE_INVISIBLE_AREAS ) ||			  
			  MYTHOS_LEVELAREAS::IsLevelAreaAHub( linker->m_LevelAreaID ) ||
			  MYTHOS_LEVELAREAS::DoesLevelAreaAlwaysShow( linker->m_LevelAreaID ) ))
		{			
			return linker->m_LevelAreaID;
		}
	}
	int nAlwaysOpenAreas = MYTHOS_LEVELAREAS::LevelAreaGetNumOfLevelAreasAlwaysVisible();
	while( m_AlwaysOpen < nAlwaysOpenAreas )
	{
		int nLevelArea = MYTHOS_LEVELAREAS::LevelAreaGetLevelAreaIDThatIsAlwaysVisibleByIndex( m_AlwaysOpen );
		m_AlwaysOpen++;
		return nLevelArea;
	}
	return INVALID_ID;
}

inline int CLevelAreasKnownIterator::IterateTeamMembers( void )
{
#if !ISVERSION(SERVER_VERSION)
	if( m_PartyMemberIndex >= MAX_CHAT_PARTY_MEMBERS )
		return INVALID_ID;
	while( m_PartyMemberIndex < MAX_CHAT_PARTY_MEMBERS )
	{
		if (c_PlayerGetPartyMemberGUID(m_PartyMemberIndex) != INVALID_GUID)
		{
			int nLevelArea = c_PlayerGetPartyMemberLevelArea(m_PartyMemberIndex, FALSE );
			BOOL bPVPWorld = c_PlayerGetPartyMemberPVPWorld(m_PartyMemberIndex, FALSE );

			if( nLevelArea != INVALID_ID &&
				bPVPWorld == PlayerIsInPVPWorld( m_pPlayer ) )
			{
				m_PartyMemberIndex++;
				return nLevelArea;
			}
		}
		m_PartyMemberIndex++;
	}
#endif
	 return INVALID_ID;
}

int CLevelAreasKnownIterator::GetNextLevelAreaID( int nFlags )
{
	if( m_Done )
		return INVALID_ID;

	if( m_IterIndex == 0 )
	{
		m_FoundPlayerArea = FALSE;
	}

	m_ID = INVALID_ID;
	while( m_ID == INVALID_ID &&
		   m_IterIndex < KITER_COUNT )
	{
		switch( m_IterIndex )
		{
		case KITER_PLAYERLEVEL:	//first rule. Always show the level the player is at.
			if( (nFlags & KLVL_AREA_ITER_SKIP_CURRENT_PLAYER_POS ) == 0 )
			{
				if( m_PlayerAreaID != INVALID_ID )
				{
					m_ID = m_PlayerAreaID;
					m_IterIndex++;
				}
			}
			break;
		case KITER_OPENAREAS:	//show all open areas
			if( m_TutorialFinished )  //have to finish tutorial
			{
				m_ID = TestOutsideAreas(nFlags);
			}
			break;
		case KITER_MAPS:		//show any maps they may have
			if( ( KLVL_AREA_ITER_NO_MAPS & nFlags ) == 0 )
			{
				m_ID = IterateMapItem();
			}
			break;
		case KITER_TEAM:
			if( ( nFlags & KLVL_AREA_ITER_NO_TEAM ) == 0 )
			{
				m_ID = IterateTeamMembers();
			}
			break;
		}
		//if we are looking through level area's. Linker level area's are known by all
		//players. But some linker level area's aren't visible. Hence this takes care of that.
		int flags( 0 );
		if( (nFlags & KLVL_AREA_ITER_IGNORE_INVISIBLE_AREAS ) != 0 &&
			m_IterIndex == KITER_OPENAREAS )
		{
			flags = flags | MYTHOS_LEVELAREAS::KLEVELAREA_MAP_FLAGS_ALLOWINVISIBLE;
		}

		//now check to see if the level is allowed to show on the map (same zone, ect ect )
		if( m_IterIndex != KITER_MAPS && //if you own a map you can always see the zones.
			m_IterIndex != KITER_OPENAREAS && //outside area's is a completely different system. it handles it self.
			m_ID != INVALID_ID &&
			MYTHOS_LEVELAREAS::LevelAreaShowsOnMap( m_pPlayer, m_ID, flags ) == FALSE )
		{			
			if( m_IterIndex == KITER_OPENAREAS )
			{
				m_ID = GetNextLevelAreaID( nFlags );
			}
			else
			{
				m_ID = INVALID_ID;
			}
		}



		if( m_ID == INVALID_ID )
		{
			m_IterIndex++; //test next items...			
		} 

		if( m_ID == m_PlayerAreaID )
		{
			if( m_FoundPlayerArea )
			{
				m_ID = INVALID_ID;
			}
			else
			{
				m_FoundPlayerArea = TRUE;
			}
		}


		if( m_ID != INVALID_ID &&
			( nFlags & KLVL_AREA_ITER_IGNORE_ZONE ) == 0 )
		{
			if( m_PlayerZone != MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( m_ID, NULL ) )
			{
				m_ID = INVALID_ID;				
				continue;
			}
		}

	}

	if( (nFlags & KLVL_AREA_ITER_DONT_GET_POSITION ) == 0 )
	{
		SetPixelLocation();
	}
	m_Done = ( m_ID == INVALID_ID )?TRUE:FALSE; //we are done
	

	if(m_Done)
	{
		return m_ID;
	}
	
	// have we already returned this level ID? (this can happen if getting known levels when in a party)
	if(!m_ReturnedIDs.Find(m_ID))
	{
		// no, so add the ID to our list and return it;
		m_ReturnedIDs.PListPushTail(m_ID);
		return m_ID;
	}
	else
	{
		//Yeap, so recursively get and return the next ID.
		return GetNextLevelAreaID(nFlags);
	}

}
