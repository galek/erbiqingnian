//----------------------------------------------------------------------------
// s_partyCache.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "difficulty.h"
#include "level.h"
#include "s_partyCache.h"
#include "freelist.h"
#include "svrstd.h"
#include "GameChatCommunication.h"
#include "GameServer.h"
#include "gamevariant.h"
#include "s_townportal.h"
#include "units.h"
#include "prime.h"
#include "clients.h"
#include "hlocks.h"
#include "player.h"
#include "gamevariant.h"
#include "gag.h"

#if ISVERSION(SERVER_VERSION)
#include "svrdebugtrace.h"
#include "s_partyCache.cpp.tmh"
#endif


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_FREE_CACHED_PARTY_MEMBERS 50


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
typedef HASHITER<CachedParty,
				 CHANNELID,
				 MAX_CACHED_PARTIES>	PARTY_HASH;

typedef FREELIST<CachedPartyMember>		PARTY_MEMBER_FREE_LIST;

DEF_HLOCK(PartyCacheLock, HLReaderWriter)
END_HLOCK

struct PARTY_CACHE
{
	struct MEMORYPOOL *				m_pool;
	PARTY_MEMBER_FREE_LIST			m_freeMembers;

	PARTY_HASH						m_partyHash;
	PartyCacheLock					m_partyHashLock;
};
#endif


//----------------------------------------------------------------------------
// INTERNAL FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void sPartyCacheClearPartyMembers(
	__notnull PARTY_CACHE * cache,
	__notnull CachedParty * party )
{
	for( UINT ii = 0; ii < party->Members.Count(); ++ii )
	{
		if( party->Members[ii] )
		{
			party->Members[ii]->Lock.Free();
			cache->m_freeMembers.Free( cache->m_pool, party->Members[ii] );
		}
	}
	party->Members.Clear( cache->m_pool );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL sPartyCacheInitParty(
	__notnull PARTY_CACHE * cache,
	__notnull CachedParty * party )
{
	party->Lock.Init();
	party->Members.Init();
	party->PartyData.Init();
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void sPartyCacheFreeParty(
	__notnull PARTY_CACHE * cache,
	__notnull CachedParty * party )
{
	party->Lock.Free();
	sPartyCacheClearPartyMembers(cache,party);
	party->Members.Destroy(cache->m_pool);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void sPartyCacheFreePartyHash(
	__notnull PARTY_CACHE * cache )
{
	CachedParty * itr = cache->m_partyHash.GetFirst();
	while(itr)
	{
		sPartyCacheFreeParty(cache,itr);
		itr = cache->m_partyHash.GetNext(itr);
	}
	cache->m_partyHash.Destroy(cache->m_pool);
}
#endif


//----------------------------------------------------------------------------
// PUBLIC FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int CachedPartyMemberCompare(
	CachedPartyMember * const & left,
	CachedPartyMember * const & right )
{
	ASSERT_RETX(left,INT_MAX);
	ASSERT_RETX(right,INT_MAX);
	return ((int)(right->GuidPlayer - left->GuidPlayer));
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
PARTY_CACHE * PartyCacheInit(
	MEMORYPOOL * pool )
{
	PARTY_CACHE * toRet = NULL;
	toRet = (PARTY_CACHE*)MALLOCZ(pool,sizeof(PARTY_CACHE));
	ASSERT_GOTO(toRet,_INIT_ERROR);

	toRet->m_pool = pool;
	toRet->m_freeMembers.Init( MAX_CACHED_PARTY_MEMBERS, MAX_FREE_CACHED_PARTY_MEMBERS );
	toRet->m_partyHashLock.Init();
	toRet->m_partyHash.Init();

	return toRet;
_INIT_ERROR:
	PartyCacheFree(toRet);
	return NULL;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void PartyCacheFree(
	PARTY_CACHE * cache )
{
	if( !cache )
	{
		return;
	}

	cache->m_partyHashLock.Free();
	sPartyCacheFreePartyHash(cache);
	cache->m_freeMembers.Destroy(cache->m_pool);

	FREE(cache->m_pool,cache);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL PartyCacheClearCache(
	PARTY_CACHE * cache )
{
	ASSERT_RETFALSE(cache);

	HLRWWriteLock lock = &cache->m_partyHashLock;

	sPartyCacheFreePartyHash(cache);
	cache->m_partyHash.Init();

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL PartyCacheAddParty(
	PARTY_CACHE * cache,
	CHANNELID channelId,
	COMMON_PARTY_GAMESVR_DATA * partyData )
{
	ASSERT_RETFALSE(cache);
	ASSERT_RETFALSE(partyData);
	if( channelId == INVALID_CHANNELID )
		return FALSE;

	HLRWWriteLock lock = &cache->m_partyHashLock;

	if(cache->m_partyHash.Get(channelId))
	{
		CachedParty * p = cache->m_partyHash.Get(channelId);
		p->PartyData.Set(partyData);
		return TRUE;
	}

	CachedParty * party = cache->m_partyHash.Add( cache->m_pool, channelId );
	ASSERT_GOTO( party, _ADD_PARTY_ERROR );
	ASSERT_GOTO( sPartyCacheInitParty( cache, party ), _ADD_PARTY_ERROR );
	
	// assign common party data coming in
	party->PartyData.Set( partyData );

	return TRUE;
_ADD_PARTY_ERROR:
	if( party )
	{
		sPartyCacheFreeParty( cache, party );
		cache->m_partyHash.Remove( channelId );
	}
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL PartyCacheRemoveParty(
	PARTY_CACHE * cache,
	CHANNELID channelId )
{
	ASSERT_RETFALSE(cache);
	if( channelId == INVALID_CHANNELID )
		return FALSE;

	BOOL toRet = FALSE;
	HLRWWriteLock lock = &cache->m_partyHashLock;
	CachedParty * party = cache->m_partyHash.Get( channelId );
	if( party )
	{
		sPartyCacheFreeParty( cache, party );
		cache->m_partyHash.Remove( channelId );
		toRet = TRUE;
	}
	else
	{
		TraceWarn(TRACE_FLAG_GAME_MISC_LOG,"Party Cache Remove Party: party not found. PARTYID - %u",channelId);
	}

	return toRet;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
CachedParty * PartyCacheLockParty(
	PARTY_CACHE * cache,
	CHANNELID channelId )
{
	ASSERT_RETNULL(cache);
	if( channelId == INVALID_CHANNELID )
		return NULL;

	CachedParty * toRet = NULL;
	cache->m_partyHashLock.ReadLock();
	toRet = cache->m_partyHash.Get( channelId );
	if( toRet )
	{
		toRet->Lock.Enter();
	}
	else
	{
		cache->m_partyHashLock.EndRead();
	}
	return toRet;
}
#endif


//----------------------------------------------------------------------------
//	does not lock the party, sends update msg to the chat server
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL PartyCacheUpdatePartyInfo(
	CachedParty * party )
{
	ASSERT_RETFALSE( party );

	CHAT_REQUEST_MSG_UPDATE_PARTY_COMMON_DATA msg;
	msg.TagParty = party->id;
	msg.PartyData.Set( &party->PartyData );

	return GameServerToChatServerSendMessage(
				&msg,
				GAME_REQUEST_UPDATE_PARTY_COMMON_DATA );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void PartyCacheUnlockParty(
	PARTY_CACHE * cache,
	CHANNELID channelId )
{
	ASSERT_RETURN(cache);
	if( channelId == INVALID_CHANNELID )
		return;

	CachedParty * toUnlock = NULL;
	toUnlock = cache->m_partyHash.Get( channelId );
	if( toUnlock )
	{
		toUnlock->Lock.Leave();
		cache->m_partyHashLock.EndRead();
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL PartyCacheAddPartyMember(
	PARTY_CACHE * cache,
	CHANNELID partyChannel,
	PGUID guid,
	APPCLIENTID idAppclient,
	COMMON_GAMESVR_DATA * memberData )
{
	ASSERT_RETFALSE(cache);
	ASSERT_RETFALSE(partyChannel != INVALID_CHANNELID);
	ASSERT_RETFALSE(guid != INVALID_GUID);
	ASSERT_RETFALSE(memberData);

	BOOL toRet = FALSE;
	CachedPartyPtr party(cache);
	party = PartyCacheLockParty(cache,partyChannel);
	if( party )
	{
		CachedPartyMember * newMember = NULL;
		CachedPartyMember toFind;
		toFind.GuidPlayer = guid;
		CachedPartyMember * * member = party->Members.FindExact( &toFind );
		if( member && member[0] )
		{
			newMember = member[0];
			newMember->Lock.Free();
		}
		else
		{
			newMember = cache->m_freeMembers.Get( cache->m_pool, __FILE__, __LINE__ );
		}

		if( newMember )
		{
			newMember->IdChannel = partyChannel;
			newMember->GuidPlayer = guid;
			newMember->IdAppClient = idAppclient;
			newMember->Lock.Init();
			newMember->MemberData.Set( memberData );
			

			if( !party->Members.FindExact( newMember ) )
			{
				if( party->Members.Insert( cache->m_pool, newMember ) )
				{
					toRet = TRUE;
				}
			}
			else
			{
				toRet = TRUE;
			}
		}
		else
		{
			TraceError( "Party Cache Add Member: No members left in the free"
						" list, member not added. GUID - %llu, PARTYID - %u",
						guid,
						partyChannel );
		}

	}
	else
	{
		TraceWarn(TRACE_FLAG_GAME_MISC_LOG,"Party Cache Add Member: Specified "
											"party not found. PARTYID - %u",
											partyChannel );
	}

	return toRet;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL PartyCacheRemovePartyMember(
	PARTY_CACHE * cache,
	CHANNELID partyChannel,
	PGUID guid )
{
	ASSERT_RETFALSE(cache);
	ASSERT_RETFALSE(partyChannel != INVALID_CHANNELID);
	ASSERT_RETFALSE(guid != INVALID_GUID);

	BOOL toRet = FALSE;
	CachedPartyPtr party(cache);
	party = PartyCacheLockParty(cache,partyChannel);
	if( party )
	{
		CachedPartyMember toFind;
		toFind.GuidPlayer = guid;

		CachedPartyMember * * member = party->Members.FindExact( &toFind );
		if( member && member[0] )
		{
			member[0]->Lock.Free();
			cache->m_freeMembers.Free( cache->m_pool, member[0] );
			party->Members.Delete( cache->m_pool, member[0] );
			toRet = TRUE;
		}
		else
		{
			TraceWarn(
				TRACE_FLAG_GAME_MISC_LOG,
				"Party Cache Remove Member: Member not in cache."
				" GUID - %llu, PARTYID - %u",
				guid,
				partyChannel );
		}
	}
	else
	{
		TraceWarn(
			TRACE_FLAG_GAME_MISC_LOG,
			"Party Cache Remove Member: Specified party not found. PARTYID - %u",
			partyChannel );
	}

	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
CachedPartyMember * PartyCacheLockPartyMember(
		PARTY_CACHE * cache,
		CHANNELID partyChannel,
		PGUID guid )
{
	ASSERT_RETNULL(cache);
	ASSERT_RETNULL(partyChannel != INVALID_CHANNELID);
	ASSERT_RETNULL(guid != INVALID_GUID);

	CachedPartyMember * * toRet = NULL;
	CachedParty * party = PartyCacheLockParty(cache,partyChannel);
	if( party )
	{
		CachedPartyMember toFind;
		toFind.GuidPlayer = guid;

		toRet = party->Members.FindExact( &toFind );
		if( toRet && toRet[0] )
		{
			toRet[0]->Lock.Enter();
		}
		else
		{
			PartyCacheUnlockParty(cache,partyChannel);
		}
	}

	return toRet ? toRet[0] : NULL;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void PartyCacheUnlockPartyMember(
	PARTY_CACHE * cache,
	CHANNELID partyChannel,
	PGUID guid )
{
	ASSERT_RETURN(cache);
	ASSERT_RETURN(partyChannel != INVALID_CHANNELID);
	ASSERT_RETURN(guid != INVALID_GUID);

	CachedParty * party = NULL;
	party = cache->m_partyHash.Get(partyChannel);
	if(party)
	{
		CachedPartyMember toFind;
		toFind.GuidPlayer = guid;

		CachedPartyMember ** toUnlock = party->Members.FindExact( &toFind );
		if( toUnlock && toUnlock[0] )
		{
			toUnlock[0]->Lock.Leave();
			party->Lock.Leave();
			cache->m_partyHashLock.EndRead();
		}
	}
}
#endif


//----------------------------------------------------------------------------
// GAME SPECIFIC HELPER FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sUpdateChatServerPlayerClientInfo(
	UNIT * pUnit )
{
	ASSERT_RETURN(pUnit);
	GameServerContext * svrContext = (GameServerContext*)CurrentSvrGetContext();
	if( !svrContext )
		return; //	shutting down

	CHAT_REQUEST_MSG_UPDATE_MEMBER_CLIENT_INFO cltData;
	cltData.TagMember					= UnitGetGuid(pUnit);
	cltData.PlayerData.idUnit			= UnitGetId(pUnit);
	cltData.PlayerData.idGame			= UnitGetGameId(pUnit);
	cltData.PlayerData.nPlayerUnitClass = UnitGetClass(pUnit);
	cltData.PlayerData.nCharacterExpLevel=UnitGetExperienceLevel(pUnit);
	cltData.PlayerData.nCharacterExpRank= UnitGetExperienceRank(pUnit);
	cltData.PlayerData.nLevelDefId		= UnitGetLevelDefinitionIndex(pUnit);
	
	GAME_VARIANT &tGameVariant = cltData.PlayerData.tGameVariant;
	GameVariantInit( tGameVariant, pUnit );

	if (AppIsTugboat())
	{
		int nArea = 1;
		int nDepth = 0;
		LEVEL* pLevel = UnitGetLevel( pUnit );
		if( pLevel )
		{
			nArea = UnitGetLevelAreaID( pUnit );
			nDepth = UnitGetLevelDepth( pUnit );

		}

		cltData.PlayerData.nArea	= nArea;
		cltData.PlayerData.nDepth	= nDepth;
	}
	s_TownPortalGetSpec( pUnit, &cltData.PlayerData.tTownPortal );

	// chat server player flags
	if (s_GagIsGagged( pUnit ))
	{
		cltData.PlayerData.i64GaggedUntilTime = s_GagGetGaggedUntil( pUnit );
	}

	ASSERT(
		GameServerToChatServerSendMessage(
			&cltData,
			GAME_REQUEST_UPDATE_MEMBER_CLIENT_INFO ) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sUpdateChatServerPlayerGameInfo(
	UNIT * pUnit )
{
	ASSERT_RETURN(pUnit);
	GameServerContext * svrContext = (GameServerContext*)CurrentSvrGetContext();
	if( !svrContext )
		return; //	shutting down

	CHAT_REQUEST_MSG_UPDATE_MEMBER_GAMESVR_INFO svrData;
	COMMON_GAMESVR_DATA &tGameData = svrData.GameData;
	s_TownPortalGetSpec( pUnit, &tGameData.tTownPortalSpec );
	svrData.TagMember = UnitGetGuid(pUnit);
	tGameData.IdGame = UnitGetGameId(pUnit);
	tGameData.nLevelDef	= UnitGetLevelDefinitionIndex(pUnit);
	tGameData.nArea	= UnitGetLevelAreaID(pUnit);
	
	GAME_VARIANT &tGameVariant = tGameData.tGameVariant;
	GameVariantInit( tGameVariant, pUnit );
	
	tGameData.ePartyJoinRestrictions = PARTY_JOIN_RESTRICTION_NONE;
	
	//	player party mode flags
	if (PlayerIsHardcore(pUnit)) 
	{
		tGameData.ePlayerPartyMode = PARTY_MODE_HARDCORE_PLAYER;
	} 
	else 
	{
		tGameData.ePlayerPartyMode = PARTY_MODE_NORMAL;
	}

	//	player party join restrictions
	if (PlayerIsHardcoreDead(pUnit))
	{
		tGameData.ePartyJoinRestrictions = PARTY_JOIN_RESTRICTION_DEAD_HARDCORE_PLAYER;
	}
	else if (AppIsHellgate())
	{
		// turning this off cause we think we can handle it now to allow you to form
		// a party across multiple games cause the warp to party member is now more robust
		//LEVEL * level = UnitGetLevel(pUnit);
		//if (level == NULL || LevelIsActionLevel(level))
		//{
		//	tGameData.ePartyJoinRestrictions = PARTY_JOIN_RESTRICTION_NOT_IN_TOWN;
		//}
	}
	
	ASSERT(
		GameServerToChatServerSendMessage(
			&svrData,
			GAME_REQUEST_UPDATE_MEMBER_GAMESVR_INFO ) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void UpdateChatServerPlayerInfo(
	UNIT * pUnit )
{
	ASSERT_RETURN(pUnit);
	
	// do nothing if we're in limbo or not in a level yet and ready to do stuff
	if (UnitIsInLimbo( pUnit ) || UnitGetRoom( pUnit ) == NULL)
	{
		return;
	}
	
	// update 
	sUpdateChatServerPlayerClientInfo(pUnit);
	sUpdateChatServerPlayerGameInfo(pUnit);
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void UpdateChatServerConnectionContext(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// setup message
	CHAT_REQUEST_MSG_UPDATE_CLIENT_CONNECTION_CONTEXT tMessage;
	tMessage.TagMember = UnitGetGuid( pPlayer );
	tMessage.i64GaggedUntilTime = s_GagGetGaggedUntil( pPlayer );

	// send to chat server
	GameServerToChatServerSendMessage( &tMessage, GAME_REQUEST_UPDATE_CLIENT_CONNECTION_CONTEXT );
		
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void PartyCacheAddToParty(
	UNIT *pUnit,
	PARTYID idParty)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( idParty != INVALID_ID, "Expected party id" );
	
	CHAT_REQUEST_MSG_ADD_MEMBER_TO_PARTY request;
	request.TagParty = idParty;
	request.TagToAdd = UnitGetGuid(pUnit);

	ASSERT(
		GameServerToChatServerSendMessage(
			&request,
			GAME_REQUEST_ADD_MEMBER_TO_PARTY ) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void PartyCacheCreateParty(
   GAMEID idGameCreator,
   ULONGLONG ullRequestCode )
{
	CHAT_REQUEST_MSG_CREATE_PARTY request;
	request.idGame = idGameCreator;
	request.ullRequestCode = ullRequestCode;

	ASSERT(
		GameServerToChatServerSendMessage(
		&request,
		GAME_REQUEST_CREATE_PARTY ) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
GAMEID PartyCacheGetMemberGameId(
	PARTY_CACHE * cache,
	CHANNELID partyChannel,
	PGUID guid )
{
	ASSERT_RETVAL(cache, INVALID_ID);
	ASSERT_RETVAL(partyChannel != INVALID_CHANNELID, INVALID_ID);
	ASSERT_RETVAL(guid != INVALID_GUID, INVALID_ID);
	CachedPartyMemberPtr member(cache);
	member = PartyCacheLockPartyMember(
	   cache,
	   partyChannel,
	   guid);
	ASSERTX_RETVAL(member, INVALID_ID, "Could not find partymember in channel.");

	return member->MemberData.IdGame;
}
#endif
#if !ISVERSION( CLIENT_ONLY_VERSION )
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID PartyCacheGetPartyMemberGameId(
	UNIT *pUnit,
	PGUID guidPartyMember)
{
	ASSERTX_RETVAL( guidPartyMember != INVALID_GUID, INVALID_ID, "Expected guid" );

	GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERTX_RETVAL( pServerContext, INVALID_ID, "Expected server context" );
	PARTY_CACHE *pPartyCache = pServerContext->m_PartyCache;

	GAMECLIENT *pClient = UnitGetClient( pUnit );
	ASSERTX_RETVAL( pClient, INVALID_ID, "Expected client" );
	APPCLIENTID idAppClient = ClientGetAppId( pClient );

	CHANNELID idPartyChannel = ClientSystemGetParty( pServerContext->m_ClientSystem, idAppClient );
	return PartyCacheGetMemberGameId( pPartyCache, idPartyChannel, guidPartyMember );

}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int sPartyCacheGetMemberLevelArea(
	PARTY_CACHE * cache,
	CHANNELID partyChannel,
	PGUID guid )
{
	ASSERTX(AppIsTugboat(), "Level areas are not relevant to hellgate." );
	ASSERT_RETVAL(cache, INVALID_ID);
	ASSERT_RETVAL(partyChannel != INVALID_CHANNELID, INVALID_ID);
	ASSERT_RETVAL(guid != INVALID_GUID, INVALID_ID);
	CachedPartyMemberPtr member(cache);
	member = PartyCacheLockPartyMember(
		cache,
		partyChannel,
		guid);
	ASSERTX_RETVAL(member, INVALID_ID, "Could not find partymember in channel.");

	return member->MemberData.nArea;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int sPartyCacheGetMemberPVPWorld(
	PARTY_CACHE * cache,
	CHANNELID partyChannel,
	PGUID guid )
{
	ASSERTX(AppIsTugboat(), "PVPWorld is not relevant to hellgate." );
	ASSERT_RETVAL(cache, INVALID_ID);
	ASSERT_RETVAL(partyChannel != INVALID_CHANNELID, INVALID_ID);
	ASSERT_RETVAL(guid != INVALID_GUID, INVALID_ID);
	CachedPartyMemberPtr member(cache);
	member = PartyCacheLockPartyMember(
		cache,
		partyChannel,
		guid);
	ASSERTX_RETVAL(member, INVALID_ID, "Could not find partymember in channel.");
	return TESTBIT( member->MemberData.tGameVariant.dwGameVariantFlags, GV_PVPWORLD );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int PartyCacheGetPartyMemberLevelArea(
	UNIT *pUnit,
	PGUID guidPartyMember)
{
	ASSERTX_RETFALSE( guidPartyMember != INVALID_GUID, "Expected guid" );

	GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERTX_RETFALSE( pServerContext, "Expected server context" );
	PARTY_CACHE *pPartyCache = pServerContext->m_PartyCache;

	GAMECLIENT *pClient = UnitGetClient( pUnit );
	ASSERTX_RETFALSE( pClient, "Expected client" );
	APPCLIENTID idAppClient = ClientGetAppId( pClient );

	CHANNELID idPartyChannel = ClientSystemGetParty( pServerContext->m_ClientSystem, idAppClient );
	return sPartyCacheGetMemberLevelArea( pPartyCache, idPartyChannel, guidPartyMember );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int PartyCacheGetPartyMemberPVPWorld(
	UNIT *pUnit,
	PGUID guidPartyMember)
{
	ASSERTX_RETFALSE( guidPartyMember != INVALID_GUID, "Expected guid" );

	GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERTX_RETFALSE( pServerContext, "Expected server context" );
	PARTY_CACHE *pPartyCache = pServerContext->m_PartyCache;

	GAMECLIENT *pClient = UnitGetClient( pUnit );
	ASSERTX_RETFALSE( pClient, "Expected client" );
	APPCLIENTID idAppClient = ClientGetAppId( pClient );

	CHANNELID idPartyChannel = ClientSystemGetParty( pServerContext->m_ClientSystem, idAppClient );
	return sPartyCacheGetMemberPVPWorld( pPartyCache, idPartyChannel, guidPartyMember );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int sPartyCacheGetMemberLevelDefinition(
	PARTY_CACHE * cache,
	CHANNELID partyChannel,
	PGUID guid )
{
	ASSERT_RETVAL(cache, INVALID_ID);
	ASSERT_RETVAL(partyChannel != INVALID_CHANNELID, INVALID_ID);
	ASSERT_RETVAL(guid != INVALID_GUID, INVALID_ID);
	CachedPartyMemberPtr member(cache);
	member = PartyCacheLockPartyMember(
		cache,
		partyChannel,
		guid);
	ASSERTX_RETVAL(member, INVALID_ID, "Could not find partymember in channel.");

	return member->MemberData.nLevelDef;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int PartyCacheGetPartyMemberLevelDefinition(
		UNIT *pUnit,
		PGUID guidPartyMember)
{
	ASSERTX_RETFALSE( guidPartyMember != INVALID_GUID, "Expected guid" );

	GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERTX_RETFALSE( pServerContext, "Expected server context" );
	PARTY_CACHE *pPartyCache = pServerContext->m_PartyCache;

	GAMECLIENT *pClient = UnitGetClient( pUnit );
	ASSERTX_RETFALSE( pClient, "Expected client" );
	APPCLIENTID idAppClient = ClientGetAppId( pClient );

	CHANNELID idPartyChannel = ClientSystemGetParty( pServerContext->m_ClientSystem, idAppClient );
	return sPartyCacheGetMemberLevelDefinition( pPartyCache, idPartyChannel, guidPartyMember );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL PartyCacheGetMemberTownPortal(
	PARTY_CACHE * cache,
	CHANNELID partyChannel,
	PGUID guid,
	TOWN_PORTAL_SPEC &tTownPortalSpec)
{
	UNREFERENCED_PARAMETER(tTownPortalSpec);
	ASSERT_RETFALSE(cache);
	ASSERT_RETFALSE(partyChannel != INVALID_CHANNELID);
	ASSERT_RETFALSE(guid != INVALID_GUID);
	CachedPartyMemberPtr member(cache);
	member = PartyCacheLockPartyMember(
		cache,
		partyChannel,
		guid);
	if(!member) return FALSE;
	tTownPortalSpec.Set(&member->MemberData.tTownPortalSpec);

	return TRUE;
}

#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void PartyCacheIterateParties( 
	GAME *pGame, 
	FN_ITERATE_PARTY_CALLBACK *pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );

	GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERTX_RETURN( pServerContext, "Expected server context" );

	// get party cache and lock for reading	
	PARTY_CACHE *pPartyCache = pServerContext->m_PartyCache;
	HLRWReadLock tReadLock = &pPartyCache->m_partyHashLock;

	// iterate parties
	for (CachedParty *pParty = pPartyCache->m_partyHash.GetFirst();
		 pParty;
		 pParty = pPartyCache->m_partyHash.GetNext( pParty ))
	{
		pfnCallback( pParty, pCallbackData );
	}

}
#endif
