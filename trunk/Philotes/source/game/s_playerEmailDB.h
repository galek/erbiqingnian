//----------------------------------------------------------------------------
// s_playerEmailDB.h
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#ifndef _S_PLAYER_EMAIL_DB_H_
#define _S_PLAYER_EMAIL_DB_D_


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_EMAIL_ITEM_TREE_NODES	10


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
struct BASE_ITEM_MOVE_DATA
{
	ULONGLONG			MessageId;
	UNIQUE_ACCOUNT_ID	RequestingAccountId;
	GAMEID				RequestingGameId;
	ULONGLONG			RequestingContext;
	PGUID				RootItemId;
	EMAIL_SPEC_SOURCE	SpecSource;
	
	BASE_ITEM_MOVE_DATA::BASE_ITEM_MOVE_DATA( void )
		:	MessageId( INVALID_GUID ),
			RequestingAccountId( INVALID_UNIQUE_ACCOUNT_ID ),
			RequestingGameId( INVALID_GAMEID ),
			RequestingContext( 0 ),
			RootItemId( INVALID_GUID ),
			SpecSource( ESS_INVALID )
	{ }
};


//----------------------------------------------------------------------------
// MOVE ITEM INTO ESCROW
//----------------------------------------------------------------------------
struct PlayerEmailDBMoveItemTreeIntoEscrow
{
	struct DataType : BASE_ITEM_MOVE_DATA
	{
		PGUID				CurrentOwnerCharacterId;
		WCHAR				NewOwnerCharacterName[MAX_CHARACTER_NAME];
		int					NewInventoryLocation;

		PGUID				ItemTreeNodes[MAX_EMAIL_ITEM_TREE_NODES];
		DWORD				ItemTreeNodeCount;
	};
	typedef DataType InitData;

	static BOOL InitRequestData(
		DataType* pRequestData,
		InitData* pInitializer)
	{
		ASSERT_RETFALSE(pRequestData && pInitializer);
		pRequestData[0] = pInitializer[0];
		return TRUE;
	}

	static void FreeRequestData(DataType*) { }

	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC hDBC,
		DataType* pRequestData);

	NULL_BATCH_MEMBERS();
};


//----------------------------------------------------------------------------
// REMOVE ITEM FROM ESCROW
//----------------------------------------------------------------------------
struct PlayerEmailDBRemoveItemTreeFromEscrow
{
	struct DataType : BASE_ITEM_MOVE_DATA
	{
		PGUID				CharacterId;
		int					NewInventoryLocation;
	};
	typedef DataType InitData;

	static BOOL InitRequestData(
		DataType* pRequestData,
		InitData* pInitializer)
	{
		ASSERT_RETFALSE(pRequestData && pInitializer);
		pRequestData[0] = pInitializer[0];
		return TRUE;
	}

	static void FreeRequestData(DataType*) { }

	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC hDBC,
		DataType* pRequestData);

	NULL_BATCH_MEMBERS();
};


//----------------------------------------------------------------------------
// MOVE MONEY INTO ESCROW
//----------------------------------------------------------------------------
struct PlayerEmailDBMoveMoneyIntoEscrow
{
	struct DataType
	{
		ULONGLONG			MessageId;
		UNIQUE_ACCOUNT_ID	RequestingAccountId;
		GAMEID				RequestingGameId;

		PGUID				CurrentOwnerCharacterId;
		WCHAR				NewOwnerCharacterName[MAX_CHARACTER_NAME];
		int					NewInventoryLocation;

		DWORD				MoneyAmmount;
		PGUID				MoneyUnitId;
	};
	typedef DataType InitData;

	static BOOL InitRequestData(
		DataType* pRequestData,
		InitData* pInitializer)
	{
		ASSERT_RETFALSE(pRequestData && pInitializer);
		pRequestData[0] = pInitializer[0];
		return TRUE;
	}

	static void FreeRequestData(DataType*) { }

	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC hDBC,
		DataType* pRequestData);

	NULL_BATCH_MEMBERS();
};


//----------------------------------------------------------------------------
// REMOVE MONEY FROM ESCROW
//----------------------------------------------------------------------------
struct PlayerEmailDBRemoveMoneyFromEscrow
{
	struct DataType
	{
		ULONGLONG			MessageId;
		UNIQUE_ACCOUNT_ID	RequestingAccountId;
		GAMEID				RequestingGameId;

		PGUID				CharacterId;
		DWORD				MoneyAmmount;
		PGUID				MoneyUnitId;
	};
	typedef DataType InitData;

	static BOOL InitRequestData(
		DataType* pRequestData,
		InitData* pInitializer)
	{
		ASSERT_RETFALSE(pRequestData && pInitializer);
		pRequestData[0] = pInitializer[0];
		return TRUE;
	}

	static void FreeRequestData(DataType*) { }

	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC hDBC,
		DataType* pRequestData);

	NULL_BATCH_MEMBERS();
};


#endif	//	_S_PLAYER_EMAIL_DB_D_
#endif	//	ISVERSION(SERVER_VERSION)
