//----------------------------------------------------------------------------
// FILE: AnchorMarkers.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __POINTSOFINTEREST_H_
#define __POINTSOFINTEREST_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

struct UNIT;
struct UNIT_DATA;


namespace MYTHOS_POINTSOFINTEREST
{


	const enum EPOINTOFINTERESTFLAGS
	{
		KPofI_Flag_IsQuestMarker = MAKE_MASK( 1 ),	
		KPofI_Flag_IsMonster = MAKE_MASK( 2 ),	
		KPofI_Flag_IsObject = MAKE_MASK( 3 ),	
		KPofI_Flag_Static = MAKE_MASK( 4 ),
		KPofI_Flag_Display = MAKE_MASK( 5 )
	};

	const enum EPOINTSOFINTEREST_MESSAGE_TYPES
	{
		KPofI_MessageType_Add,
		KPofI_MessageType_Remove,
		KPofI_MessageType_RemoveAll,
		KPofI_MessageType_Refresh,
		KPofI_MessageType_Count
	};

	struct PointOfInterest
	{
		int	nPosX;
		int	nPosY;
		int nFlags;
		int	nStringId;	
		int nUnitID;
		int nClass;
		const UNIT_DATA *pUnitData;
	};

	class cPointsOfInterest
	{
	public:
		cPointsOfInterest( UNIT *pPlayer );
		~cPointsOfInterest();


		//Returns the point of interest by index
		const MYTHOS_POINTSOFINTEREST::PointOfInterest * GetPointOfInterestByIndex( int nIndex );
		//returns the index of the point of Interest by UnitData
		int GetPointOfInterestIndexByUnitData( const UNIT_DATA *pUnitData );
		//returns the number of points of interest
		int GetPointOfInterestCount();	
		//Adds a unit as a point of Interest
		void AddPointOfInterest( UNIT *pUnit, int nStringID, int nFlags );
		//Adds a point of interest
		void AddPointOfInterest( const UNIT_DATA *pUnitData, int UnitId, int nClass, int nPosX, int nPosY, int nStringID, int nFlags );
		//removes a point of interest
		void RemovePointOfInterest( int UnitId );
		//removes all points of interest
		void ClearAllPointsOfInterest();
		//tells the client to refresh
		void ClientRefresh();
		//sets the string on the point of interest
		void SetString( int nIndex, int nStringId );
		//Adds a flags to the point of interest
		void AddFlag( int nIndex, EPOINTOFINTERESTFLAGS nFlag );
		//Clears a flags to the point of interest
		void ClearFlag( int nIndex, EPOINTOFINTERESTFLAGS nFlag );
		//Checks to see if it has the flag
		BOOL HasFlag( int nIndex, EPOINTOFINTERESTFLAGS nFlag );
		//Returns the Level Area ID of the Point of Interest if it has one
		int GetLevelAreaID( int nIndex );
	private:
		inline GAME * GetGame();	
		//sends an update for the marker
		void SendMessageForMarker( int nMarkerIndex, EPOINTSOFINTEREST_MESSAGE_TYPES nMessageType  );
		UNIT										*m_pPlayer;
		SIMPLE_DYNAMIC_ARRAY<PointOfInterest>		m_PofIList;	
		BOOL										m_bInited;

	}; 
	
} //end namespace
#endif
