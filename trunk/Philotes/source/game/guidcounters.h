//----------------------------------------------------------------------------
// guidcounters.h
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
// A counter/hash to keep track of database writes, so we can avoid sync
// issues/dupe bugs from doing things before database transactions complete.
//----------------------------------------------------------------------------

#ifndef _DBCOUNTER_H_
#define _DBCOUNTER_H_

#ifndef _HASH_H_
#include "hash.h"
#endif

//----------------------------------------------------------------------------
// DEFINE
//----------------------------------------------------------------------------
#define VALIDATE_UNIQUE_GUIDS		ISVERSION(SERVER_VERSION)				// unique guid tracking breaks single player because client and server games share same context


//Callback for doing things when we're done with database operations.
typedef void (*DATABASE_COUNTER_ENTRY_CALLBACK)(void *context);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct DatabaseCounterEntry
{
	DatabaseCounterEntry *next;
	PGUID id; //for hash

	int count;

	void *context;
	DATABASE_COUNTER_ENTRY_CALLBACK callback;
	BOOL bDeleteOnZero;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class DatabaseCounter
{
protected:
	CS_HASH_LOCKING<DatabaseCounterEntry, PGUID, 1023> m_players;

public:
	inline void Init(MEMORYPOOL *pPool) {m_players.Init(pPool);}
	inline void Free() {m_players.Destroy();}
	inline DatabaseCounter(MEMORYPOOL *pPool) {Init(pPool);}

	void Increment(const PGUID &id);
	void Decrement(const PGUID &id);

	void SetRemoveOnZero(const PGUID &id);

	void SetCallbackOnZero(
		const PGUID &id, 
		void *context, 
		DATABASE_COUNTER_ENTRY_CALLBACK callback);
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline DatabaseCounter * DatabaseCounterInit(MEMORYPOOL *pPool)
{
	DatabaseCounter * toRet = (DatabaseCounter *)MALLOCZ(pPool, sizeof(DatabaseCounter));
	toRet->Init(pPool);
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void DatabaseCounterFree(MEMORYPOOL *pPool, DatabaseCounter *pCounter)
{
	if(pCounter) 
	{
		pCounter->Free();
		FREE(pPool, pCounter);
	}
}


#if VALIDATE_UNIQUE_GUIDS
//----------------------------------------------------------------------------
// GUID tracking for all items
//----------------------------------------------------------------------------
struct GuidTrackerEntry
{
	GuidTrackerEntry *next;
	PGUID id; //for hash

	int count;

	//Only for the last one entered, if there's a dupe.
	GAMEID idGame;	
	UNITID idUnit;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class GuidTracker
{
protected:
	CS_HASH_LOCKING<GuidTrackerEntry, PGUID, 100023> m_units;

public:
	inline void Init(MEMORYPOOL *pPool) {m_units.Init(pPool);}
	inline void Free() {m_units.Destroy();}
	inline GuidTracker(MEMORYPOOL *pPool) {Init(pPool);}

	BOOL Increment(const PGUID &id, GAMEID idGame, UNITID idUnit); //returns FALSE for dupes.
	void Decrement(const PGUID &id);
};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline GuidTracker * GuidTrackerInit(MEMORYPOOL *pPool)
{
	GuidTracker * toRet = (GuidTracker *)MALLOCZ(pPool, sizeof(GuidTracker));
	toRet->Init(pPool);
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void GuidTrackerFree(MEMORYPOOL *pPool, GuidTracker *pCounter)
{
	if(pCounter) 
	{
		pCounter->Free();
		FREE(pPool, pCounter);
	}
}
#endif //VALIDATE_UNIQUE_GUIDS

#endif //_DBCOUNTER_H_