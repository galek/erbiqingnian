//----------------------------------------------------------------------------
// s_message.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// SERVER to CLIENT messages
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "items.h"
#include "clients.h"
#include "room.h"
#include "s_message.h"
#include "units.h"
#include "svrstd.h"
#include "GameServer.h"
#if ISVERSION(SERVER_VERSION)
#include "s_message.cpp.tmh"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#endif


//----------------------------------------------------------------------------
// DIRECTIVES
//----------------------------------------------------------------------------
#define BUFFER_GAME_MESSAGES			1										// buffer game messages & send at end of frame


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MAX_USERSERVER_INSTANCE					512								// maximum # of user servers
#define MAX_PLAYERS_PER_SEND_PER_USERSERVER		256								// maximum # of players for multipoint send
#define MAX_PLAYERS_PER_SEND_TOTAL				MAX_CHARACTERS_PER_GAME_SERVER	//Just in case they're all in one game.


//----------------------------------------------------------------------------
// FUNCTIONS (Messages from Server to Client)
//----------------------------------------------------------------------------
extern BOOL SrvValidateMessageTable(
	void);


// ---------------------------------------------------------------------------
// get command list
// ---------------------------------------------------------------------------
NET_COMMAND_TABLE * s_NetGetCommandTable(
	void)
{
	static CCritSectLite cs;
	static NET_COMMAND_TABLE * s_GameServerCommandTable = NULL;

	CSLAutoLock autolock(&cs);

	if (s_GameServerCommandTable)
	{
		NetCommandTableRef(s_GameServerCommandTable);
		return s_GameServerCommandTable;
	}

	NET_COMMAND_TABLE * cmd_tbl = NULL;
	cmd_tbl = NetCommandTableInit(SCMD_LAST);
	ASSERT_RETNULL(cmd_tbl);

#undef  NET_MSG_TABLE_BEGIN
#undef  NET_MSG_TABLE_DEF
#undef  NET_MSG_TABLE_END

#define NET_MSG_TABLE_BEGIN
#define NET_MSG_TABLE_END(c)
#define NET_MSG_TABLE_DEF(c, s, r)		{  MSG_##c msg; MSG_STRUCT_DECL * msg_struct = msg.zz_register_msg_struct(cmd_tbl); \
											NetRegisterCommand(cmd_tbl, c, msg_struct, s, r, static_cast<MFP_PRINT>(&MSG_##c::Print), \
											static_cast<MFP_PUSHPOP>(&MSG_##c::Push), static_cast<MFP_PUSHPOP>(&MSG_##c::Pop)); }

#undef  _S_MESSAGE_ENUM_H_
#include "s_message.h"

	if (!NetCommandTableValidate(cmd_tbl)
#if !ISVERSION(CLIENT_ONLY_VERSION)
		|| !SrvValidateMessageTable()
#endif \\!ISVERSION(CLIENT_ONLY_VERSION)
		)
	{
		NetCommandTableFree(cmd_tbl);
		return NULL;
	}

	s_GameServerCommandTable = cmd_tbl;
	return s_GameServerCommandTable;
}


//----------------------------------------------------------------------------
// FUNCTIONS (Messages from Server to Client)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void s_IncrementGameServerMessageCounter(
	void)
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(serverContext);
	PERF_ADD64(serverContext->m_pPerfInstance, GameServer, GameServerMessagesSentPerSecond, 1);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void s_IncrementGameServerMessageCounter(
	unsigned int count)
{
	REF(count);

#if ISVERSION(SERVER_VERSION)
	if (count <= 0)
	{
		return;
	}
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(serverContext);
	PERF_ADD64(serverContext->m_pPerfInstance, GameServer, GameServerMessagesSentPerSecond, count);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageAppClient(
	APPCLIENTID idClient,
	BYTE command,
	MSG_STRUCT * msg)
{
	NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), idClient);
	ASSERT_RETURN(idNetClient != INVALID_NETCLIENTID64);
	SvrNetSendResponseToClient(idNetClient, msg, command);

	s_IncrementGameServerMessageCounter();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageNetClient(
	NETCLIENTID64 idNetClient,
	BYTE command,
	MSG_STRUCT * msg)
{
	ASSERT_RETURN(idNetClient != INVALID_NETCLIENTID64);
	SvrNetSendResponseToClient(idNetClient, msg, command);

	s_IncrementGameServerMessageCounter();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessage(
	GAME * game,
	GAMECLIENTID idClient,
	NET_CMD command,
	MSG_STRUCT * msg)
{
	ASSERT_RETURN(game && IS_SERVER(game));
	if (GameGetState(game) == GAMESTATE_SHUTDOWN)
	{
		return;
	}

	GAMECLIENT * client = ClientGetById(game, idClient);
	if (!client)
	{
		TraceError("No client for GAMECLIENTID %I64x in s_SendMessage()", idClient);
		return;
	}
	
#if BUFFER_GAME_MESSAGES
	SendMessageToClient(game, idClient, command, msg);
#else
	//If we're not buffering, track here with no size information.  Otherwise, track deeper in.
	client->m_tSMessageTracker.AddMessage(GameGetTick(game), command);

	NETCLIENTID64 idNetClient = ClientGetNetId(game, idClient);
	if (idNetClient == INVALID_NETCLIENTID64)
	{
		return;
	}
	SvrNetSendResponseToClient(idNetClient, msg, command);

	s_IncrementGameServerMessageCounter();
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageNetClientArray(
	NETCLIENTID64 * arrayIdNetClient,
	int nNetClients,
	BYTE command,
	MSG_STRUCT * msg)
{
#if BUFFER_GAME_MESSAGES
	ASSERT_RETURN(0);
#else
	SVRCONNECTIONID arrayIdConnection[MAX_USERSERVER_INSTANCE][MAX_PLAYERS_PER_SEND_PER_USERSERVER];
	int nClientsPerUserServer[MAX_USERSERVER_INSTANCE];
	ZeroMemory(nClientsPerUserServer, sizeof(nClientsPerUserServer));

	int nUserServerCount = SvrClusterGetServerCount(USER_SERVER);
	ASSERTX_RETURN(nUserServerCount <= MAX_USERSERVER_INSTANCE, 
		"Error: more userservers than we have array size for");

	int nErrorTooManyClients = 0;

	for (int i = 0; i < nNetClients; i++)
	{
		SVRINSTANCE nUserServer = ServerIdGetInstance(
			ServiceUserIdGetServer(arrayIdNetClient[i]));
		ASSERT_CONTINUE(nUserServer < MAX_USERSERVER_INSTANCE);

		if (nClientsPerUserServer[nUserServer] >= MAX_PLAYERS_PER_SEND_PER_USERSERVER)
		{
			nErrorTooManyClients++;
			continue;
		}

		arrayIdConnection[nUserServer][nClientsPerUserServer[nUserServer]]
			= ServiceUserIdGetConnectionId(arrayIdNetClient[i]);

		nClientsPerUserServer[nUserServer]++;
	}

	if (nErrorTooManyClients != 0)
	{
		TraceInfo(TRACE_FLAG_GAME_NET,
			"Failed multisend to %d clients over the array limit.", 
			nErrorTooManyClients);
	}

	SVR_VERSION_ONLY(int nNumMessages = 0);
	for (SVRINSTANCE i = 0; i < nUserServerCount; i++)
	{
		switch (nClientsPerUserServer[i])
		{
		case 0: break;
		case 1: 
			SvrNetSendResponseToClient(
				ServiceUserId(USER_SERVER, i, arrayIdConnection[i][0]),
				msg, command);
			SVR_VERSION_ONLY(nNumMessages++);
			break;
		default:
			if(nClientsPerUserServer[i] > 0) 
			{
				SvrNetSendResponseToClients(
					ServerId(USER_SERVER, i), arrayIdConnection[i], nClientsPerUserServer[i],
					msg, command);
				SVR_VERSION_ONLY(nNumMessages += nClientsPerUserServer[i]);
			}
			break;
		}
	}

	SVR_VERSION_ONLY(s_IncrementGameServerMessageCounter(nNumMessages));
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageClientArray(
	GAME * game,
	GAMECLIENT ** clients,
	unsigned int count,
	BYTE command,
	MSG_STRUCT * msg)
{
#if BUFFER_GAME_MESSAGES
	for (unsigned int ii = 0; ii < count; ++ii)
	{
		ASSERT_CONTINUE(clients[ii]);
		SendMessageToClient(game, clients[ii], command, msg);
	}
#else
	SVRCONNECTIONID arrayIdConnection[MAX_USERSERVER_INSTANCE][MAX_PLAYERS_PER_SEND_PER_USERSERVER];
	int nClientsPerUserServer[MAX_USERSERVER_INSTANCE];
	ZeroMemory(nClientsPerUserServer, sizeof(nClientsPerUserServer));

	int nUserServerCount = SvrClusterGetServerCount(USER_SERVER);
	ASSERTX_RETURN(nUserServerCount <= MAX_USERSERVER_INSTANCE, 
		"Error: more userservers than we have array size for");

	int nErrorTooManyClients = 0;

	for (unsigned int ii = 0; ii < count; ++ii)
	{
		ASSERT_CONTINUE(clients[ii]);

		NETCLIENTID64 idNetClient = ClientGetNetId(clients[ii]);
		ASSERT_CONTINUE(idNetClient != INVALID_NETCLIENTID64);

		SVRID idUserServer = ServiceUserIdGetServer(idNetClient);
		SVRINSTANCE nUserServer = ServerIdGetInstance(idUserServer);
		ASSERT_CONTINUE(nUserServer < MAX_USERSERVER_INSTANCE);

		if (nClientsPerUserServer[nUserServer] >= MAX_PLAYERS_PER_SEND_PER_USERSERVER)
		{
			nErrorTooManyClients++;
			continue;
		}

		SVRCONNECTIONID idServerConnection = ServiceUserIdGetConnectionId(idNetClient);
		arrayIdConnection[nUserServer][nClientsPerUserServer[nUserServer]] = idServerConnection;

		nClientsPerUserServer[nUserServer]++;
	}

	if (nErrorTooManyClients != 0)
	{
		TraceInfo(TRACE_FLAG_GAME_NET, "Failed multisend to %d clients over the array limit.", nErrorTooManyClients);
	}

	SVR_VERSION_ONLY(int nNumMessages = 0);
	for (SVRINSTANCE ii = 0; ii < nUserServerCount; ++ii)
	{
		switch (nClientsPerUserServer[ii])
		{
		case 0: break;
		case 1: 
			SvrNetSendResponseToClient(ServiceUserId(USER_SERVER, ii, arrayIdConnection[ii][0]), msg, command);
			SVR_VERSION_ONLY(nNumMessages++);
			break;
		default:
			if (nClientsPerUserServer[ii] > 0) 
			{
				SvrNetSendResponseToClients(ServerId(USER_SERVER, ii), arrayIdConnection[ii], nClientsPerUserServer[ii], msg, command);
				SVR_VERSION_ONLY(nNumMessages += nClientsPerUserServer[ii]);
			}
			break;
		}
	}

	SVR_VERSION_ONLY(s_IncrementGameServerMessageCounter(nNumMessages));
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageToAll(
	GAME * game,
	BYTE command,
	MSG_STRUCT * msg)
{
	ASSERT(game);
	if (GameGetState(game) != GAMESTATE_RUNNING)
	{
		return;
	}

	GAMECLIENT * clients[MAX_PLAYERS_PER_SEND_TOTAL];
	unsigned int count = 0;

	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		ASSERT_BREAK(count < MAX_PLAYERS_PER_SEND_TOTAL);
		clients[count++] = client;
	}

	s_SendMessageClientArray(game, clients, count, command, msg);	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientKnowsRoom(
	GAMECLIENT * client,
	ROOM * room)
{
	ASSERTX_RETFALSE(client, "Expected client");
	ASSERTX_RETFALSE(room, "Expected room");
	LEVEL * level = RoomGetLevel(room);
	
	// get control unit
	UNIT * control = ClientGetControlUnit(client);
	ASSERTX_RETFALSE(control, "Expected control unit");
		
	// it would be great to wrap this up in ClientIsRoomKnown()
	// but we need unique room ids to do that and when we send all the room units
	// to a client we sometimes know the level is correct even though there is
	// no control unit yet (happens when we enter a game)
	LEVEL * unitlevel = UnitGetLevel(control);
	if (unitlevel == level && ClientIsRoomKnown(client, room))
	{
		return TRUE;
	}
	
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageToAllWithRooms(
	GAME * game,
	struct ROOM ** roomarray,
	unsigned int roomcount,
	BYTE command,
	MSG_STRUCT * msg)
{
	ASSERT_RETURN(game);
	if (GameGetState(game) != GAMESTATE_RUNNING)
	{
		return;
	}

	GAMECLIENT * clients[MAX_PLAYERS_PER_SEND_TOTAL];
	unsigned int count = 0;

	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		ASSERT_BREAK(count < MAX_PLAYERS_PER_SEND_TOTAL);

		BOOL bKnowsAllRooms = TRUE;
		
		// check to see if this client knows all the requested rooms
		for (unsigned int ii = 0; ii < roomcount; ++ii)
		{
			ROOM * room = roomarray[ii];
			ASSERTX_CONTINUE(room, "Expected room");
			
			if (ClientKnowsRoom(client, room) == FALSE)
			{
				bKnowsAllRooms = FALSE;
				break;
			}
		}
		
		// only send if knows all the rooms
		if (bKnowsAllRooms)
		{
			clients[count++] = client;
		}
	}
	s_SendMessageClientArray(game, clients, count, command, msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageToAllWithRoom(
	GAME * game,
	ROOM * room,
	BYTE command,
	MSG_STRUCT * msg)
{
	s_SendMessageToAllWithRooms(game, &room, 1, command, msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageToOthers(
	GAME * game,
	GAMECLIENTID idClient,
	BYTE command,
	MSG_STRUCT * msg)
{
	ASSERT_RETURN(game);
	if (GameGetState(game) != GAMESTATE_RUNNING)
	{
		return;
	}

	GAMECLIENT * clients[MAX_PLAYERS_PER_SEND_TOTAL];
	unsigned int count = 0;

	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		ASSERT_BREAK(count < MAX_PLAYERS_PER_SEND_TOTAL);

		GAMECLIENTID idOther = ClientGetId(client);
		if (idOther != idClient)
		{
			clients[count++] = client;
		}
	}
	s_SendMessageClientArray(game, clients, count, command, msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageToOthersWithRoom(
	GAME * game,
	GAMECLIENTID idClient,
	ROOM * room,
	BYTE command,
	MSG_STRUCT * msg)
{
	ASSERT_RETURN(game);
	if (GameGetState(game) != GAMESTATE_RUNNING)
	{
		return;
	}
	if (!room)
	{
		return;
	}
	
	GAMECLIENT * clients[MAX_PLAYERS_PER_SEND_TOTAL];
	unsigned int count = 0;

	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		ASSERT_BREAK(count < MAX_PLAYERS_PER_SEND_TOTAL);

		GAMECLIENTID idOther = ClientGetId(client);
		if (idOther != idClient)
		{			
			if (ClientKnowsRoom(client, room))
			{
				clients[count++] = client;
			}
		}
	}
	s_SendMessageClientArray(game, clients, count, command, msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitMessageToClient(
	UNIT * unit,
	GAMECLIENTID idClient,
	NET_CMD cmd,
	MSG_STRUCT * msg)
{
	ASSERT_RETURN(unit);
	ASSERT_RETURN(msg);
	ASSERT_RETURN(idClient != INVALID_GAMECLIENTID);
	
	if (UnitCanSendMessages(unit) == FALSE)
	{
		return;
	}
	
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);
		
	s_SendMessage(game, idClient, cmd, msg);
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendUnitMessage(
	UNIT * unit,
	NET_CMD cmd,
	MSG_STRUCT * msg,
	GAMECLIENTID idClientExclude)												//  = INVALID_GAMECLIENTID
{
	ASSERT_RETURN(unit);
	ASSERT_RETURN(msg);

#if ISVERSION(CLIENT_ONLY_VERSION)
	REF(cmd);
	REF(idClientExclude);
#else
	// if unit has the don't send messages flag on, don't send anything
	if (UnitCanSendMessages(unit) == FALSE)
	{
		return;
	}

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);
		
	// send to clients	
	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		GAMECLIENTID idClient = ClientGetId(client);
		if (idClient != idClientExclude && UnitIsKnownByClient(client, unit))
		{
			s_SendUnitMessageToClient(unit, idClient, cmd, msg);
		}
	}
#endif
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMultiUnitMessage(
	UNIT **pUnits,
	int nNumUnits,
	BYTE bCommand,
	MSG_STRUCT *pMessage)
{
	ASSERTX_RETURN( pUnits, "Expected unit array" );
	ASSERTX_RETURN( nNumUnits > 0, "Expected unit count" );
	ASSERTX_RETURN( pMessage, "Expected message" );

	// scan each unit
	const int MAX_ROOMS = 32;
	ROOM *pRooms[ MAX_ROOMS ];
	int nNumRooms = 0;
	for (int i = 0; i < nNumUnits; ++i)
	{
		UNIT *pUnit = pUnits[ i ];

		if ( ! pUnit )
			continue;
		
		// if unit is not ready for message sending, we abort
		if (UnitCanSendMessages( pUnit ) == FALSE)
		{	
			return;
		}
		
		// get unit room
		ROOM *pRoom = UnitGetRoom( pUnit );
		
		// is room already in buffer
		BOOL bFound = FALSE;
		for (int j = 0; j < nNumRooms; ++j)
		{
			if (pRooms[ j ] == pRoom)
			{
				bFound = TRUE;
				break;
			}
		}
		
		// add room if not found
		if (bFound == FALSE)
		{	
			ASSERTX_CONTINUE( nNumRooms < MAX_ROOMS - 1, "Too many rooms for multi unit message" );
			pRooms[ nNumRooms++ ] = pRoom;
		}
		
	}	
	
	// send message
	GAME *pGame = UnitGetGame( pUnits[ 0 ] );
	s_SendMessageToAllWithRooms( pGame, pRooms, nNumRooms, bCommand, pMessage );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMessageToAllInLevel(
	GAME * game,
	LEVEL * level,
	BYTE command,
	MSG_STRUCT * msg)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(level);
	ASSERT_RETURN(msg);

	GAMECLIENT * clients[MAX_PLAYERS_PER_SEND_TOTAL];
	unsigned int count = 0;

	for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
	{
		ASSERT_BREAK(count < MAX_PLAYERS_PER_SEND_TOTAL);
		clients[count++] = client;
	}
	s_SendMessageClientArray(game, clients, count, command, msg);	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendGuildActionResult(
	NETCLIENTID64 idNetClient,
	WORD requestType,
	WORD requestResult )
{
	ASSERT_RETURN(idNetClient != INVALID_NETCLIENTID64);

	MSG_SCMD_GUILD_ACTION_RESULT resMsg;
	resMsg.wActionType = requestType;
	resMsg.wChatErrorResultCode = requestResult;

	ASSERT(
		SvrNetSendResponseToClient(
			idNetClient,
			&resMsg,
			SCMD_GUILD_ACTION_RESULT));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendPvPActionResult(
	NETCLIENTID64 idNetClient,
	WORD wActionType, 
	WORD wActionErrorType )
{
	ASSERT_RETURN(idNetClient != INVALID_NETCLIENTID64);

	MSG_SCMD_PVP_ACTION_RESULT resMsg;
	resMsg.wActionType = wActionType;
	resMsg.wActionErrorType = wActionErrorType;

	ASSERT(
		SvrNetSendResponseToClient(
		idNetClient,
		&resMsg,
		SCMD_PVP_ACTION_RESULT));
}

//----------------------------------------------------------------------------
// SPECIALCASE COMPRESSION CODE
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Compressed room add
// Explanation: room adding is the major bandwidth user in tugboat, as far
// as server commands go.  This is because tugboat levels are composed of
// such small tiles, each of which requires a room add message.  The floating
// point nature of the room matrix makes knowledge-free compression of the
// bits somewhat ineffective.  Since tugboat pretty much only flips or 90
// degree rotates rooms, however, we can make a special-case message with
// much less entropy.

// Currently, loading an entire tugboat dungeon level takes around ten
// thousand room add messages.  This may have to change; we'll see.  We
// don't plan to load them all at once at rollout, but we currently do.

// This currently only compresses the mTranslation matrix.  Other optimizations
// include eliminating the dwRoomSeed if the produced room is equivalent to
// the most probable room (most of the time in tugboat, tiles are empty)
// or merely calculating out equivalence.  Folding the bRegion into the
// orientation byte probably won't improve post-compressed size.
// It might be possible to infer the position from the idRoom (if tiles are
// placed in grids of known size) and completely eliminate the three floats,
// but this is very specific to DRLG composition.
// If all of these optimizations are done and work, we are likely down to
// less than 32 bits of information.

// Note on above notes: if grids are of known shape transmitted at play-time
// to the client, this may imply that a hacking client can figure out the
// general shape of the dungeon, which is bad.  However, we can simply make
// the grid much bigger then any planned dungeon and have a lot of "holes"
// in our array of rooms.  All this assumes that rooms in tugboat dungeons
// always have rational size with proportion to each other, or preferably
// equal size.  Whether or not this is true I'm not sure.
//----------------------------------------------------------------------------
/*DEF_MSG_STRUCT(MSG_SCMD_ROOM_ADD)
MSG_FIELD(0, int, idRoomDef, 0)
MSG_FIELD(1, DWORD, idRoom, 0)
MSG_STRUC(2, MSG_MATRIX, mTranslation)
MSG_FIELD(3, DWORD, dwRoomSeed, 0)
MSG_FIELD(4, BYTE, bRegion, 0)
END_MSG_STRUCT*/
/*
DEF_MSG_STRUCT(MSG_SCMD_ROOM_ADD_COMPRESSED) //Putting frequently repeating fields first for easier sequential compression.
MSG_FIELD(0, BYTE, bRegion, 0)
MSG_FIELD(1, BYTE, mOrientation, 0)
MSG_FIELD(2, int, idRoomDef, 0)
MSG_FIELD(3, DWORD, idRoom, 0)
MSG_FIELD(4, float, fXPosition, 0.0f)
MSG_FIELD(5, float, fYPosition, 0.0f)
MSG_FIELD(6, float, fZPosition, 0.0f) //may be unnecessary in tugboat, as it is 2d.
MSG_FIELD(7, DWORD, dwRoomSeed, 0)
END_MSG_STRUCT
*/

BOOL CompressRoomMessage(const MSG_SCMD_ROOM_ADD &original,
						 MSG_SCMD_ROOM_ADD_COMPRESSED &compressed )
{
	//Check if the room matrix is identity or a simple 2d reflection
	//or ninety degree rotation.
	//Anything else, and we return false.

	//Check the 3x3 portion of the matrix for three ones, indicating simple axis flipping or nothing.

	for(int i = 0; i < 3; i++)
	{
		int nOnes = 0;
		for (int j = 0; j < 3; j++)
		{
			if ( ABS(original.mTranslation.m[i][j]) < EPSILON) continue; //close enough to zero.
			else if ( ABS(original.mTranslation.m[i][j] - 1.0f) < EPSILON) nOnes++; //one
			else if ( ABS(original.mTranslation.m[i][j] + 1.0f) < EPSILON) nOnes++; //negative one
			else return FALSE; //A case not handled by this compresser.  Use normal message.
		}
		if(nOnes != 1) return FALSE; //A single one per row, rest close enough to zero.
	}
	//format of mOrientation: first bit is whether x axis is flipped to y axis,
	//second bit is whether x is negative,
	//third bit is whether y is negative.
	//I could do z as well, but neither tugboat nor hellgate inverts rooms.
	//After compression it should nicely reduce to four bits of entropy with
	//bRegion, probably more like 1 or 2 on average.  (since unrotated non-bregion
	//is the majority of the time)
	BYTE mOrientation = 0;

	if( ABS(original.mTranslation.m[1][0]) > EPSILON &&
		ABS(original.mTranslation.m[0][1]) > EPSILON)
	{
		mOrientation = 0x1;
	}
	else if( ABS(original.mTranslation.m[1][1]) > EPSILON &&
		ABS(original.mTranslation.m[0][0]) > EPSILON)
	{
		mOrientation = 0x0;
	}
	else ASSERT_RETFALSE(FALSE); //We have z-axis rotations?
	ASSERT( ABS(original.mTranslation.m[2][2] - 1.0f) < EPSILON)

		if( (original.mTranslation.m[0][1] + (original.mTranslation.m[0][0])) > 0)
			mOrientation += 0;
		else mOrientation += 2; //second bit

		if( (original.mTranslation.m[1][1] + (original.mTranslation.m[1][0])) > 0)
			mOrientation += 0;
		else mOrientation += 4; //third bit

		compressed.mOrientation = mOrientation;
		//translated back by quick table lookup.

		compressed.fXPosition = original.mTranslation._41;
		compressed.fYPosition = original.mTranslation._42;
		compressed.fZPosition = original.mTranslation._43;

		compressed.bRegion		= original.bRegion; 		//tugboat
		compressed.idSubLevel	= original.idSubLevel;
		compressed.idRoomDef	= original.idRoomDef;
		compressed.idRoom		= original.idRoom;
		compressed.dwRoomSeed	= original.dwRoomSeed;
		compressed.bKnownOnServer = original.bKnownOnServer;
		PStrCopy(compressed.szLayoutOverride, 32, original.szLayoutOverride, 32);

		return TRUE;
}


const MATRIX mOrientations[] =
{
	{//identity
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	},
	{//flip axes
		0, 1, 0, 0,
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	},
	{//mirror x
		-1, 0, 0, 0,
		 0, 1, 0, 0,
		 0, 0, 1, 0,
		 0, 0, 0, 1
	},
	{
		0,-1, 0, 0,
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	},
	{//mirror y
		1, 0, 0, 0,
		0,-1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	},
	{
		 0, 1, 0, 0,
		-1, 0, 0, 0,
		 0, 0, 1, 0,
		 0, 0, 0, 1
	},
	{
		-1, 0, 0, 0,
		 0,-1, 0, 0,
		 0, 0, 1, 0,
		 0, 0, 0, 1
	},
	{
		 0,-1, 0, 0,
		-1, 0, 0, 0,
		 0, 0, 1, 0,
		 0, 0, 0, 1
	}
};

void UncompressRoomMessage(MSG_SCMD_ROOM_ADD &original,
						   const MSG_SCMD_ROOM_ADD_COMPRESSED &compressed )
{
	//Translate mOrientation to the original rotation matrix, set everything equal.
	original.bRegion		= compressed.bRegion;
	original.idSubLevel		= compressed.idSubLevel;
	original.idRoomDef		= compressed.idRoomDef;
	original.idRoom			= compressed.idRoom;
	original.dwRoomSeed		= compressed.dwRoomSeed;
	original.bKnownOnServer = compressed.bKnownOnServer;
	PStrCopy(original.szLayoutOverride, 32, compressed.szLayoutOverride, 32);


	original.mTranslation   = mOrientations[compressed.mOrientation];
	//comment out to test if I can break it or it is bypassedoriginal.mTranslation._43;	
	original.mTranslation._41=compressed.fXPosition;
	original.mTranslation._42=compressed.fYPosition;
	original.mTranslation._43=compressed.fZPosition;
}
