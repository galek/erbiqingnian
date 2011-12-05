//----------------------------------------------------------------------------
//	statspriv.h
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _STATSPRIV_H_
#define _STATSPRIV_H_


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define STATS_DEBUG_MAGIC				'STAT'
#define GLOBAL_STATS_TRACKING			(ISVERSION(DEVELOPMENT) /* || ISVERSION(SERVER_VERSION) */ )


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	STATSFLAG_STATSLIST,												// free-standing statslist (STATS *)
	STATSFLAG_UNITSTATS,												// stats is a unitstats structure
	STATSFLAG_RIDER,													// stats is a damage rider
	STATSFLAG_COMBAT_STATS_ACCRUED,										// combat stats have been accrued
	STATSFLAG_SENT_ON_ADD,												// statslist sent stats changes on add
	NUM_STATSFLAGS,
};


//----------------------------------------------------------------------------
// DATA STRUCTURES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// sorted array which stores stat-value pairs.  zero values are not stored.
// some stats have offsets to support non-zero default values (usually -1)
//----------------------------------------------------------------------------
struct STATS_LIST
{
	STATS_ENTRY *					m_Stats;							// realloc'd array of stat-value pairs
	unsigned int					m_Count;							// size of m_Stats
};


//----------------------------------------------------------------------------
// this struct stores a stats_list and the necessary links to allow it to
// modify a unit.  a STATS can actually consist of multiple linked lists
// (called "riders").  they get added and removed from a unit en-masse.
//----------------------------------------------------------------------------
struct STATS
{
	DWORD							m_dwDebugMagic;						// debug header
	DWORD							m_Flags;							// flags
#if GLOBAL_STATS_TRACKING
	GAME *							m_Game;
#endif
	STATS *							m_Prev;								// prev statslist in modlist / or damage rider list
	STATS *							m_Next;								// next	statslist in modlist / or damage rider list
	STATS_LIST						m_List;								// list of stat-value pairs
	struct UNITSTATS *				m_Cloth;							// unit this statslist modifies

	union
	{
		STATS *						m_RiderParent;						// if list is a rider, this is the modlist parent
		struct
		{
			STATS *					m_FirstRider;						// list of attached riders (if this list isn't a rider)
			STATS *					m_LastRider;
		};
	};

	int								m_Selector;							// use to find/classify modlists
	DWORD							m_idStats;							// id for statslist

	GAME_TICK						m_tiTimer;							// absolute tick at which statslist is removed
	FP_STATSREMOVECALLBACK *		m_fpRemoveCallback;					// callback after statslist has been removed
};


//----------------------------------------------------------------------------
// UNITSTATS is the stats structure for a unit.  it contains a cached list
// of accrued stat values (accr) and a cached list of calculated stat
// values (calc).  a stat for a unit can be found either in accr or in
// calc, depending on if the stat is a target for a calc or not
//----------------------------------------------------------------------------
struct UNITSTATS : public STATS
{
	UNIT *							m_Unit;								// back pointer to the unit

	STATS_LIST						m_Accr;								// accrued stats
	STATS_LIST						m_Calc;								// calculated stats

	UNITSTATS *						m_Dye;								// first unit that's modifying me
	STATS *							m_Modlist;							// first modlist that's modifying me

	STATS *							m_Default;							// default list from excel
	STATS *							m_AttackList[STATS_MAX_ATTACKS];	// cached attack lists
};


//----------------------------------------------------------------------------
// internal structure used for stats display
//----------------------------------------------------------------------------
struct DISPLAY_CONDITION_RESULTS
{
	int								count;
	BOOL *							prev;
	BOOL *							curr;
};


#endif // _STATSPRIV_H_


