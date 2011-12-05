//----------------------------------------------------------------------------
// dbcounter.cpp
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
// A counter/hash to keep track of database writes, so we can avoid sync
// issues/dupe bugs from doing things before database transactions complete.
//
// Another hash to keep track of all outstanding GUIDS, so we can detect dupes
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "hash.h"
#include "guidcounters.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DatabaseCounter::Increment(const PGUID &id)
{
	ASSERT_RETURN(id != INVALID_GUID);
	DatabaseCounterEntry * pEntry = m_players.GetOrAdd(id);
	ONCE
	{
		ASSERT_BREAK(pEntry);
		pEntry->count++;
	}

	m_players.Unlock(id);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DatabaseCounter::Decrement(const PGUID &id)
{
	ASSERT_RETURN(id != INVALID_GUID);
	DATABASE_COUNTER_ENTRY_CALLBACK callback = NULL;
	void *context = NULL;
	int count = 1;
	BOOL bDeleteOnZero = FALSE;

	DatabaseCounterEntry * pEntry = m_players.Get(id);
	ONCE
	{
		ASSERTX_BREAK(pEntry, "Decrementing non-existent entry.  Imbalanced database operations?");
		pEntry->count--;
		count = pEntry->count;
		context = pEntry->context;
		callback = pEntry->callback;
		bDeleteOnZero = pEntry->bDeleteOnZero;
	}
	if(bDeleteOnZero && count == 0) m_players.Remove(id);
	else m_players.Unlock(id);

	ASSERTX(count >= 0, "Negative count!  Imbalanced database operations?");

	if(callback && count == 0) callback(context);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DatabaseCounter::SetRemoveOnZero(const PGUID &id)
{
	ASSERT_RETURN(id != INVALID_GUID);
	DatabaseCounterEntry * pEntry = m_players.Get(id);
	if(pEntry && pEntry->count == 0)
	{
		m_players.Remove(id);
	}
	else
	{
		if (pEntry)
		{
			pEntry->bDeleteOnZero = TRUE;
		}
		m_players.Unlock(id);
	}		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DatabaseCounter::SetCallbackOnZero(const PGUID &id, void *context, DATABASE_COUNTER_ENTRY_CALLBACK callback)
{
	ASSERT_RETURN(id != INVALID_GUID);
	int count = 1;
	DatabaseCounterEntry * pEntry = m_players.GetOrAdd(id);
	ONCE
	{
		ASSERT_BREAK(pEntry);
		count = pEntry->count;
		ASSERT(pEntry->context == NULL || context == NULL); //We're overwriting a context pointer, so we probably leak memory
		pEntry->context = context;
		ASSERT(pEntry->callback == NULL || callback == NULL); //in future we may turn off this assert as we may have legitimate reasons to overwrite it.
		pEntry->callback = callback;
	}
	m_players.Unlock(id);
	if(count == 0 && callback) callback(context);
}

#if VALIDATE_UNIQUE_GUIDS
//----------------------------------------------------------------------------
// GUIDTRACKER FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Really, most of the handling should be in the unit creation function
// that actually dupes the item.  However, in case we want to do something
// weird with the previous item to possess the guid...
//
// Warning: called within a lock.
//----------------------------------------------------------------------------
void HandleDupe(GuidTrackerEntry * pEntry, GAMEID idGame, UNITID idUnit)
{
	ASSERT_RETURN(pEntry);
	ASSERTV_MSG(
		"Found duplicate item with PGUID %I64x, in game %I64x with id %d and game %I64x with id %d",
		pEntry->id, pEntry->idGame, pEntry->idUnit, idGame, idUnit);
}

//----------------------------------------------------------------------------
// returns FALSE for dupes.  Currently handles dupes by just asserting
//----------------------------------------------------------------------------
BOOL GuidTracker::Increment(const PGUID &id, GAMEID idGame, UNITID idUnit)
{
	if(id == 0 || id == INVALID_GUID) return FALSE;

	BOOL bRet = TRUE;
	GuidTrackerEntry * pEntry = m_units.GetOrAdd(id);
	ONCE
	{
		ASSERT_BREAK(pEntry);
		pEntry->count++;
		if(pEntry->count > 1)
		{
			HandleDupe(pEntry, idGame, idUnit);
			bRet = FALSE;
		}
		pEntry->idGame = idGame;
		pEntry->idUnit = idUnit;
	}
	m_units.Unlock(id);

	return bRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GuidTracker::Decrement(const PGUID &id)
{
	if(id == 0 || id == INVALID_GUID) return;
	int count = 1;
	GuidTrackerEntry * pEntry = m_units.Get(id);
	ONCE
	{
		ASSERTX_BREAK(pEntry, "Decrementing non-existent entry.");
		pEntry->count--;
		count = pEntry->count;
	}
	if(count == 0) m_units.Remove(id);
	else m_units.Unlock(id);

	ASSERTX(count >= 0, "Negative count!  Imbalanced unit creation operations?");
}
#endif