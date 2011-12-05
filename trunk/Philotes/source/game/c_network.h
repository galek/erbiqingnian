//----------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

BOOL AppInitClientNetwork(
	const char* pszIP);

void AppFreeClientNetwork(
	void);

BOOL AppNegotiateClientConnectionType(
	NETCOM *,
	NETCLIENTID,
	UINT16 serverType );

void AppSetClientId(
	GAMECLIENTID idClient);

GAMECLIENTID AppGetClientId(
	void);

void AppSetCharacterGuid(
	PGUID characterGuid);

PGUID AppGetCharacterGuid(
	void );

void AppLoadServerSelection(
	char * szSeverName);

void AppSaveServerSelection(
	T_SERVER * pServerDef);

void AppSetServerSelection(
	T_SERVER * pServerDef);

void CltProcessMessages( 
	LONG max_time = 0);

void ClientProcessImmediateMessage(
	struct NETCOM *,
	NETCLIENTID, 
	BYTE * data,
	unsigned int size);

void ClientRegisterImmediateMessages();

BOOL CltValidateMessageTable(
	void);

int c_GetPing();
