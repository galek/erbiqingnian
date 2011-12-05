//----------------------------------------------------------------------------
// s_partyCache.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _S_PARTYCACHE_H_
#define _S_PARTYCACHE_H_

#if !ISVERSION(CLIENT_ONLY_VERSION)



//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "CommonChatMessages.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_CACHED_PARTIES			15000	//	TODO: come up with a reasonable limit...
#define MAX_CACHED_PARTY_MEMBERS	(MAX_CACHED_PARTIES*MAX_CHAT_PARTY_MEMBERS)


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

struct CachedPartyMember : LIST_SL_NODE	//	for free list
{
	CCritSectLite				Lock;
	CHANNELID					IdChannel;
	PGUID						GuidPlayer;
	APPCLIENTID					IdAppClient;
	COMMON_GAMESVR_DATA			MemberData;
};

int
	CachedPartyMemberCompare(
		CachedPartyMember * const &,
		CachedPartyMember * const & );

typedef SORTED_ARRAYEX<
				CachedPartyMember *,
				CachedPartyMemberCompare,
				MAX_CHAT_PARTY_MEMBERS>		MEMBERS_LIST;

struct CachedParty
{
	CHANNELID					id;			//	for hash
	struct CachedParty *		next;		//	for hash

	CCritSectLite				Lock;
	COMMON_PARTY_GAMESVR_DATA	PartyData;
	MEMBERS_LIST				Members;

};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

struct PARTY_CACHE;

PARTY_CACHE *
	PartyCacheInit(
		struct MEMORYPOOL * );

void
	PartyCacheFree(
		PARTY_CACHE * );

BOOL
	PartyCacheClearCache(
		PARTY_CACHE * );

//----------------------------------------------------------------------------

BOOL
	PartyCacheAddParty(
		PARTY_CACHE *,
		CHANNELID,
		COMMON_PARTY_GAMESVR_DATA * );

BOOL
	PartyCacheRemoveParty(
		PARTY_CACHE *,
		CHANNELID );

//	SEE: CachedPartyPtr below.
CachedParty *
	PartyCacheLockParty(
		PARTY_CACHE *,
		CHANNELID );

//	does not lock the party, sends update msg to the chat server.
BOOL
	PartyCacheUpdatePartyInfo(
		CachedParty * );

void
	PartyCacheUnlockParty(
		PARTY_CACHE *,
		CHANNELID );

//----------------------------------------------------------------------------

BOOL
	PartyCacheAddPartyMember(
		PARTY_CACHE *,
		CHANNELID,
		PGUID,
		APPCLIENTID,
		struct COMMON_GAMESVR_DATA * );

BOOL
	PartyCacheRemovePartyMember(
		PARTY_CACHE *,
		CHANNELID,
		PGUID );

//	SEE: CachedPartyMemberPtr below.
CachedPartyMember *
	PartyCacheLockPartyMember(
		PARTY_CACHE *,
		CHANNELID,
		PGUID );

void
	PartyCacheUnlockPartyMember(
		PARTY_CACHE *,
		CHANNELID,
		PGUID );

//----------------------------------------------------------------------------

void
	UpdateChatServerPlayerInfo(
		struct UNIT * );

void
	UpdateChatServerConnectionContext(
		struct UNIT * );

//----------------------------------------------------------------------------

void
	PartyCacheAddToParty(
		struct UNIT *,
		PARTYID );

void 
	PartyCacheCreateParty(
		GAMEID idGameCreator,
		ULONGLONG ullRequestCode );

GAMEID 
	PartyCacheGetMemberGameId(
		 PARTY_CACHE * cache,
		 CHANNELID partyChannel,
		 PGUID guid );

GAMEID PartyCacheGetPartyMemberGameId(
	UNIT *pUnit,
	PGUID guidPartyMember);

int PartyCacheGetPartyMemberLevelArea(
		UNIT *pUnit,
		PGUID guidPartyMember);

int PartyCacheGetPartyMemberPVPWorld(
		UNIT *pUnit,
		PGUID guidPartyMember);

int PartyCacheGetPartyMemberLevelDefinition(
		UNIT *pUnit,
		PGUID guidPartyMember);

BOOL PartyCacheGetMemberTownPortal(
		PARTY_CACHE * cache,
		CHANNELID partyChannel,
		PGUID guid,
		TOWN_PORTAL_SPEC &tTownPortalSpec);

//----------------------------------------------------------------------------
typedef void FN_ITERATE_PARTY_CALLBACK( struct CachedParty *pParty, void *pUserData );

void PartyCacheIterateParties( 
	GAME *pGame, 
	FN_ITERATE_PARTY_CALLBACK *pfnCallback,
	void *pCallbackData);
				
//----------------------------------------------------------------------------
// SMART POINTERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct CachedPartyPtr
{
private:
	PARTY_CACHE * m_cache;
	CachedParty * m_party;
	CachedPartyPtr( void ) {};			//	requires cache pointer
public:
	CachedPartyPtr( PARTY_CACHE * cache ) {
		m_cache = cache;
		m_party = NULL;
	}
	CachedPartyPtr & operator = ( CachedParty * party ) {
		if( m_party )
		{
			PartyCacheUnlockParty(m_cache,m_party->id);
		}
		m_party = party;
		return this[0];
	}
	~CachedPartyPtr( void ) {
		this[0] = NULL;
	}
	CachedParty & operator * ( void ) {
		return *m_party;
	}
	CachedParty * operator->( void ) {
		return m_party;
	}
	operator CachedParty * ( void ) {
		return m_party;
	}
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct CachedPartyMemberPtr
{
private:
	PARTY_CACHE * m_cache;
	CachedPartyMember * m_partyMember;
	CachedPartyMemberPtr( void ) {};	//	requires cache pointer
public:
	CachedPartyMemberPtr( PARTY_CACHE * cache ) {
		m_cache = cache;
		m_partyMember = NULL;
	}
	CachedPartyMemberPtr & operator = ( CachedPartyMember * member ) {
		if( m_partyMember )
		{
			PartyCacheUnlockPartyMember(m_cache,m_partyMember->IdChannel,m_partyMember->GuidPlayer);
		}
		m_partyMember = member;
		return this[0];
	}
	~CachedPartyMemberPtr( void ) {
		this[0] = NULL;
	}
	CachedPartyMember & operator * ( void ) {
		return *m_partyMember;
	}
	CachedPartyMember * operator->( void ) {
		return m_partyMember;
	}
	operator CachedPartyMember * ( void ) {
		return m_partyMember;
	}
};

#endif !ISVERSION(CLIENT_ONLY_VERSION)

#endif // _S_PARTYCACHE_H_