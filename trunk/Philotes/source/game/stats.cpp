//----------------------------------------------------------------------------
// stats2.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// NOTES: filters determine which stats get applied, 0 = all stats,
// TODO: filters, calculated fields, flags
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "stats.h"
#include "statspriv.h"
#include "statssave.h"
#include "script.h"
#include "units.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "inventory.h"
#include "s_message.h"
#include "console.h"
#include "uix.h"
#include "windowsmessages.h"
#include "combat.h"														// used for specialized stats calcs (LOCAL_DAMAGE_MIN etc)
#include "language.h"
#include "stringtable.h"												// used by PrintStats()
#include "items.h"														// used by PrintStats()
#include "unit_priv.h"													// used by PrintStats()
#include "skills.h"														// used by PrintStats()
#include "excel_private.h"												// used by excel post process functions
#include "c_recipe.h"
#include "uix_graphic.h"												// for some color-coding of strings
#include "pstring.h"
#include "s_questsave.h"
#include "unitfileversion.h"
#include "stringreplacement.h"
#include "waypoint.h"
#include "offer.h"
#include "difficulty.h"
#include "s_store.h"
#include "unittag.h"

#if ISVERSION(SERVER_VERSION)
#include "stats.cpp.tmh"
#endif


//----------------------------------------------------------------------------
// debugging
//----------------------------------------------------------------------------
#define STATS_VERIFY_ON_GET				0

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MAX_TOK_LEN						256								// max token length for parsing stats formulas

#define STATSDISPLAY_MAX_LINE_LEN		2048							// max line length for stats display
#define STATSDISPLAY_MAX_VAL_LEN		256								// max length for a single value for stats display
#define STATSDISPLAY_MAX_LOCAL_BUFFER	64								// max # of riders we support via local (stack) buffers

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define STATS_SET_MAGIC(s)				s->m_dwDebugMagic = STATS_DEBUG_MAGIC;
#define DO_PARAM(s, idx, p)				(s->m_nParamBits[idx] ? p : 0)


//----------------------------------------------------------------------------
// ENUM
//----------------------------------------------------------------------------
// stats calc operations
enum STATS_TOKEN
{
	STATS_TOKEN_NEG,
	STATS_TOKEN_POS,
	STATS_TOKEN_TILDE,
	STATS_TOKEN_NOT,
	STATS_TOKEN_POWER,
	STATS_TOKEN_MUL,
	STATS_TOKEN_DIV,
	STATS_TOKEN_MOD,
	STATS_TOKEN_PCT,
	STATS_TOKEN_PLUSPCT,
	STATS_TOKEN_ADD,
	STATS_TOKEN_SUB,
	STATS_TOKEN_LSHIFT,
	STATS_TOKEN_RSHIFT,
	STATS_TOKEN_LT,
	STATS_TOKEN_GT,
	STATS_TOKEN_LTE,
	STATS_TOKEN_GTE,
	STATS_TOKEN_EQ,
	STATS_TOKEN_NEQ,
	STATS_TOKEN_AND,
	STATS_TOKEN_XOR,
	STATS_TOKEN_OR,
	STATS_TOKEN_LAND,
	STATS_TOKEN_LOR,
	STATS_TOKEN_QUESTION,
	STATS_TOKEN_COLON,
	STATS_TOKEN_LPAREN,
	STATS_TOKEN_RPAREN,
	STATS_TOKEN_CONSTANT,
	STATS_TOKEN_STAT,
	STATS_TOKEN_STAT_PARAM_ZERO,
	STATS_TOKEN_STAT_PARAM_N,
	STATS_TOKEN_SUM,													// top-level node
	STATS_TOKEN_LOCAL_DMG_MIN,
	STATS_TOKEN_LOCAL_DMG_MAX,
	STATS_TOKEN_SPLASH_DMG_MIN,
	STATS_TOKEN_SPLASH_DMG_MAX,
	STATS_TOKEN_THORNS_DMG_MIN,
	STATS_TOKEN_THORNS_DMG_MAX,
	STATS_TOKEN_EOL,
	NUM_STATS_TOKEN
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
typedef int FP_UNARY_OP(int a);
typedef int FP_BINARY_OP(int a, int b);


//----------------------------------------------------------------------------
// operator data
struct STATS_CALC_OPERATOR
{
	int									m_nEnum;
	const char *						m_szToken;
	BOOL								m_bIsIdentifier;				// use m_szToken as the identifier during parsing
	unsigned int						m_nPrecedence;
	unsigned int						m_nAssociativity;				// 0 = left-to-right, 1 = right-to-left
	unsigned int						m_nOperandCount;
	FP_UNARY_OP *						m_fpUnaryFunction;
	FP_BINARY_OP *						m_fpBinaryFunction;
};


//----------------------------------------------------------------------------
static inline int sStatsCalcOpErr1(
	int a)
{
	ASSERT_RETZERO(0);
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpErr2(
	int a,
	int b)
{
	ASSERT_RETZERO(0);
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpNeg(
	int a)
{
	return -a;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpPos(
	int a)
{
	return a;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpTilde(
	int a)
{
	return ~a;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpNot(
	int a)
{
	return !a;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpPower(
	int a,
	int b)
{
	if (b < 0 || a == 0)
	{
		return 0;
	}
	if (b == 0 || a == 1)
	{
		return 1;
	}
	int result = 1;
	for (; b; a = a * a, b = b / 2)
	{
		if (b % 2)
		{
			result = result * a;
			--b;
			if (b == 0)
			{
				break;
			}
		}
	}
	return result;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpMul(
	int a,
	int b)
{
	return a * b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpDiv(
	int a,
	int b)
{
	ASSERT_RETZERO(b != 0);
	return a / b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpMod(
	int a,
	int b)
{
	ASSERT_RETZERO(b != 0);
	return a % b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpPct(
	int a,
	int b)
{
	return (b == 100 ? a : PCT(a, b));
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpPlusPct(
	int a,
	int b)
{
	return (b == 0 ? a : PCT(a, 100 + b));
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpAdd(
	int a,
	int b)
{
	return a + b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpSub(
	int a,
	int b)
{
	return a - b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpLShift(
	int a,
	int b)
{
	return (b >= 0 ? (a << b) : (a >> b));
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpRShift(
	int a,
	int b)
{
	return (b >= 0 ? (a >> b) : (a << b));
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpLT(
	int a,
	int b)
{
	return a < b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpGT(
	int a,
	int b)
{
	return a > b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpLTE(
	int a,
	int b)
{
	return a <= b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpGTE(
	int a,
	int b)
{
	return a >= b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpEQ(
	int a,
	int b)
{
	return a == b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpNEQ(
	int a,
	int b)
{
	return a != b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpAnd(
	int a,
	int b)
{
	return a & b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpOr(
	int a,
	int b)
{
	return a | b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpXor(
	int a,
	int b)
{
	return a ^ b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpLAnd(
	int a,
	int b)
{
	return a && b;
}


//----------------------------------------------------------------------------
static inline int sStatsCalcOpLOr(
	int a,
	int b)
{
	return a || b;
}


//----------------------------------------------------------------------------
#define OPDEF(e, tok, isid, pr, as, oc, unf, bif)		{ e, tok, isid, pr, as, oc, unf, bif }
static const STATS_CALC_OPERATOR tblStatsOperator[] =
{
	OPDEF(STATS_TOKEN_NEG,				"-",			FALSE,	10,	1, 1,	sStatsCalcOpNeg,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_POS,				"+",			FALSE,	10,	1, 1,	sStatsCalcOpPos,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_TILDE,			"~",			FALSE,	10,	1, 1,	sStatsCalcOpTilde,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_NOT,				"!",			FALSE,	10,	1, 1,	sStatsCalcOpNot,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_POWER,			"^^",			FALSE,	15,	0, 2,	sStatsCalcOpErr1,		sStatsCalcOpPower	),
	OPDEF(STATS_TOKEN_MUL,				"*",			FALSE,	20, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpMul		),
	OPDEF(STATS_TOKEN_DIV,				"/",			FALSE,	20, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpDiv		),
	OPDEF(STATS_TOKEN_MOD,				"%",			FALSE,	20, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpMod		),
	OPDEF(STATS_TOKEN_PCT,				"%%",			FALSE,	20, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpPct		),
	OPDEF(STATS_TOKEN_PLUSPCT,			"+%",			FALSE,	20, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpPlusPct	),
	OPDEF(STATS_TOKEN_ADD,				"+",			FALSE,	25, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpAdd		),
	OPDEF(STATS_TOKEN_SUB,				"-",			FALSE,	25, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpSub		),
	OPDEF(STATS_TOKEN_LSHIFT,			"<<",			FALSE,	30, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpLShift	),
	OPDEF(STATS_TOKEN_RSHIFT,			">>",			FALSE,	30, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpRShift	),
	OPDEF(STATS_TOKEN_LT,				"<",			FALSE,	35, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpLT		),
	OPDEF(STATS_TOKEN_GT,				">",			FALSE,	35, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpGT		),
	OPDEF(STATS_TOKEN_LTE,				"<=",			FALSE,	35, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpLTE		),
	OPDEF(STATS_TOKEN_GTE,				">=",			FALSE,	35, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpGTE		),
	OPDEF(STATS_TOKEN_EQ,				"==",			FALSE,	40, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpEQ		),
	OPDEF(STATS_TOKEN_NEQ,				"!=",			FALSE,	40, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpNEQ		),
	OPDEF(STATS_TOKEN_AND,				"&",			FALSE,	45, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpAnd		),
	OPDEF(STATS_TOKEN_XOR,				"^",			FALSE,	50, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpOr		),
	OPDEF(STATS_TOKEN_OR,				"|",			FALSE,	55, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpXor		),
	OPDEF(STATS_TOKEN_LAND,				"&&",			FALSE,	60, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpLAnd	),
	OPDEF(STATS_TOKEN_LOR,				"||",			FALSE,	60, 0, 2,	sStatsCalcOpErr1,		sStatsCalcOpLOr		),
	OPDEF(STATS_TOKEN_QUESTION,			"?",			FALSE,	70, 1, 2,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_COLON,			":",			FALSE,	69, 1, 2,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_LPAREN,			"(",			FALSE,	95, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_RPAREN,			")",			FALSE,	95, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_CONSTANT,			"const",		FALSE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_STAT,				"stat",			FALSE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_STAT_PARAM_ZERO,	"stat.0",		FALSE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_STAT_PARAM_N,		"stat.N",		FALSE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_SUM,				"sum",			FALSE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_LOCAL_DMG_MIN,	"loc_dmg_min",	TRUE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_LOCAL_DMG_MAX,	"loc_dmg_max",	TRUE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_SPLASH_DMG_MIN,	"spl_dmg_min",	TRUE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_SPLASH_DMG_MAX,	"spl_dmg_max",	TRUE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_THORNS_DMG_MIN,	"tho_dmg_min",	TRUE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_THORNS_DMG_MAX,	"tho_dmg_max",	TRUE,	 0, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
	OPDEF(STATS_TOKEN_EOL,				"eol",			FALSE,	99, 0, 0,	sStatsCalcOpErr1,		sStatsCalcOpErr2	),
};


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
template <BOOL RUN_SPEC>
static inline void sUnitSetStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int nValue,
	const STATS_DATA * stats_data,
	BOOL bSend = TRUE);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GLOBAL_STATS_TRACKING
#define GLOBAL_GLOBAL_STATS_TRACKING_MAX_STATS			1024
struct GLOBAL_STATS_TRACKER
{
	volatile long						m_SvrCount[GLOBAL_GLOBAL_STATS_TRACKING_MAX_STATS];
	volatile long						m_CltCount[GLOBAL_GLOBAL_STATS_TRACKING_MAX_STATS];
	
	//------------------------------------------------------------------------
	GLOBAL_STATS_TRACKER(
		void)
	{
		for (unsigned int ii = 0; ii < GLOBAL_GLOBAL_STATS_TRACKING_MAX_STATS; ++ii)
		{
			m_SvrCount[ii] = 0;
			m_CltCount[ii] = 0;
		}
	}

	//------------------------------------------------------------------------
	void Add(
		GAME * game,
		unsigned int index)
	{
		if (!game)
		{
			return;
		}
		ASSERT_RETURN(index < GLOBAL_GLOBAL_STATS_TRACKING_MAX_STATS);
#if ISVERSION(CLIENT_ONLY_VERSION)
		InterlockedIncrement(&m_CltCount[index]);
#elif ISVERSION(SERVER_VERSION)
		InterlockedIncrement(&m_SvrCount[index]);
#else
		if (IS_SERVER(game))
		{
			InterlockedIncrement(&m_SvrCount[index]);
		}
		else
		{
			InterlockedIncrement(&m_CltCount[index]);
		}
#endif
	}

	//------------------------------------------------------------------------
	void TraceAll(
		void)
	{
		TraceGameInfo("--= GLOBAL STATS TRACKER =--");
		TraceGameInfo("%3s  %50s  %20s  %20s", "IDX", "STAT", "CLT COUNT", "SVR COUNT");
		unsigned int numstats = GetNumStats();
		for (unsigned int ii = 0; ii < numstats; ++ii)
		{
			const STATS_DATA * stats_data = StatsGetData(NULL, ii);
			if (stats_data == NULL)
			{
				continue;
			}
			unsigned int c_count = (unsigned int)m_CltCount[ii];
			unsigned int s_count = (unsigned int)m_SvrCount[ii];
			if (c_count > 0 || s_count > 0)
			{
				TraceGameInfo("%3d  %50s  %20d  %20d", ii, stats_data->m_szName, c_count, s_count);
			}
		}
	}
};

GLOBAL_STATS_TRACKER	g_GlobalStatsTracker;

//----------------------------------------------------------------------------
static void GlobalStatsTrackerAdd(
	GAME * game,
	unsigned int index)
{
	g_GlobalStatsTracker.Add(game, index);
}
#endif


//----------------------------------------------------------------------------
void GlobalStatsTrackerTrace(
	void)
{
#if GLOBAL_STATS_TRACKING
	g_GlobalStatsTracker.TraceAll();
#endif
}


//----------------------------------------------------------------------------
// EXCEL FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sStatsParseTokenGetStatName(
	const char * token,
	char * statname,
	unsigned int max)
{
	unsigned int len = 0;
	while (len < max - 1 && token[len] != 0 && token[len] != '.')
	{
		statname[len] = token[len];
		len++;
	}
	statname[len] = 0;
	return len;
}


//----------------------------------------------------------------------------
// called by excel to generate dynamic stats headers
//----------------------------------------------------------------------------
BOOL StatsParseToken(
	const char * token,
	int & wStat,
	DWORD & dwParam)
{
	char strStat[MAX_PATH];
	unsigned int len = sStatsParseTokenGetStatName(token, strStat, arrsize(strStat));
	if (len == 0)
	{
		return FALSE;
	}
	wStat = ExcelGetLineByStringIndex(NULL, DATATABLE_STATS, strStat);
	if (wStat == INVALID_LINK)
	{
		return FALSE;
	}
	dwParam = 0;
	if (token[len] == '.')
	{
		const STATS_DATA * stats_data = StatsGetData(NULL, wStat);
		if (stats_data && stats_data->m_nParamTable[0] != INVALID_LINK)
		{
			dwParam = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)stats_data->m_nParamTable[0], token + len + 1);
			dwParam = MAX((DWORD)0, dwParam);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// called after generating the index for stats.txt to add all of the stats 
// to the the scripting language as symbols
//----------------------------------------------------------------------------
BOOL ExcelStatsPostIndex(
	struct EXCEL_TABLE * table)
{
	return ScriptRegisterStatSymbols(table);
}

//----------------------------------------------------------------------------
struct VERSION_LOOKUP
{
	const char *pszName;
	PFN_STAT_VERSION_FUNCTION pfnFunction;
};

//----------------------------------------------------------------------------
static VERSION_LOOKUP sgtVersionFunctionLookup[] = 
{
	{ "QuestSaveState",					s_VersionStatQuestSaveState				},
	{ "WaypointFlag",					s_VersionStatWaypointFlag				},
	{ "StartLevel",						s_VersionStatStartLevel					},
	{ "Offer",							s_VersionStatOffer						},
	{ "MerchantLastBrowsedTick",		s_VersionStatMerchantLastBrowsedTick	},
	{ "Difficulty",						s_VersionStatDifficulty					},
	// Add new functions here
	
	{ NULL,								NULL }		// keep this last
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PFN_STAT_VERSION_FUNCTION sLookupVersionFunction( 
	const char *pszName)
{
	ASSERTX_RETNULL( pszName, "Expected function name" );
	
	if (pszName[ 0 ] != 0)
	{
		for (int i = 0; sgtVersionFunctionLookup[ i ].pszName != NULL; ++i)
		{
			const VERSION_LOOKUP *pLookup = &sgtVersionFunctionLookup[ i ];
			if (PStrICmp( pLookup->pszName, pszName ) == 0)
			{
				return pLookup->pfnFunction;
			}
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsAddStatsChangeCallback(
	STATS_CALLBACK_TABLE * & head,
	FP_STATSCHANGECALLBACK * fpCallback,
	const STATS_CALLBACK_DATA & data)
{
	ASSERT_RETURN(fpCallback);

	STATS_CALLBACK_TABLE * node = (STATS_CALLBACK_TABLE *)MALLOCZ(NULL, sizeof(STATS_CALLBACK_TABLE));
	node->m_fpCallback = fpCallback;
	node->m_Data = data;

	// add to end of callback chain
	STATS_CALLBACK_TABLE * prev = NULL;
	STATS_CALLBACK_TABLE * curr = head;
	while (curr)
	{
		prev = curr;
		curr = curr->m_pNext;
	}
	if (prev)
	{
		prev->m_pNext = node;
	}
	else
	{
		head = node;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsAddStatsChangeCallback(
	STATS_DATA * stats_data,
	FP_STATSCHANGECALLBACK * fpCallback,
	const STATS_CALLBACK_DATA & data,
	BOOL bServer,
	BOOL bClient)
{
	ASSERT_RETURN(stats_data);

	if (bServer)
	{
		sStatsAddStatsChangeCallback(stats_data->m_SvrStatsChangeCallback, fpCallback, data);
	}
	if (bClient)
	{
		sStatsAddStatsChangeCallback(stats_data->m_CltStatsChangeCallback, fpCallback, data);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsAddStatsChangeCallback(
	int stat,
	FP_STATSCHANGECALLBACK * fpCallback,
	const STATS_CALLBACK_DATA & data,
	BOOL bServer,
	BOOL bClient)
{
	STATS_DATA * stats_data = (STATS_DATA *)ExcelGetDataNotThreadSafe(NULL, DATATABLE_STATS, stat);
	ASSERT_RETURN(stats_data);

	StatsAddStatsChangeCallback(stats_data, fpCallback, data, bServer, bClient);
}


//----------------------------------------------------------------------------
// called after reading stats.txt (either txt or cooked)
//----------------------------------------------------------------------------
BOOL ExcelStatsPostProcess(
	struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	unsigned int numStats = ExcelGetCountPrivate(table);
	STREAM_BITS_STAT_INDEX = HIGHBIT(numStats);  // this value is only used for runtime communication, it is not written to disk and can't be since it's dependent on the current data
	static BOOL bUsed = FALSE;
	UNIGNORABLE_ASSERTX(bUsed == FALSE, "Multiple stat post processes!");
	bUsed = TRUE;

	int num_combat_stats = 0;
	int num_feed_stats = 0;
	int num_requirement_stats = 0;
	int num_req_limit_stats = 0;
	for (unsigned int ii = 0; ii < numStats; ii++)
	{
		STATS_DATA * stats_data = (STATS_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(stats_data);

		// lookup version function
		stats_data->pfnVersion = sLookupVersionFunction( stats_data->szVersionFunction );

		// set special offset for unit version based on the unit version value in code, we do this
		// so that we can have a stat "set" for each unit, but units that are actually the current
		// version won't actually take up any memory because they are already the default value for the stat
		if (ii == STATS_UNIT_VERSION)
		{
			stats_data->m_nStatOffset = -((int)USV_CURRENT_VERSION);
		}
		
		// process total param bits
		for (unsigned int jj = 0; jj < STATS_MAX_PARAMS; ++jj)
		{
			if (stats_data->m_nParamBits[jj] <= 0)
			{
				break;
			}
			stats_data->m_nParamGetShift[jj] = stats_data->m_nTotalParamBits;
			stats_data->m_nParamGetMask[jj] = (1 << stats_data->m_nParamBits[jj]) - 1;
			++stats_data->m_nParamCount;
			stats_data->m_nTotalParamBits += stats_data->m_nParamBits[jj];
#if ISVERSION(DEVELOPMENT)
			CLT_VERSION_ONLY(HALT(stats_data->m_nTotalParamBits <= PARAM_BITS));	// CHB 2007.08.01 - String audit: development
			ASSERT(stats_data->m_nTotalParamBits <= PARAM_BITS);
#endif
		}

		if (stats_data->m_nUnitType == INVALID_LINK || stats_data->m_nUnitType == UNITTYPE_NONE)
		{
			stats_data->m_nUnitType = UNITTYPE_ANY;
		}
		if (stats_data->m_nMaxStat != INVALID_LINK)
		{
			ASSERT(!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE));
			CLEARBIT(stats_data->m_dwFlags, STATS_DATA_ACCRUE);
		}
		if (StatsDataTestFlag(stats_data, STATS_DATA_COMBAT))
		{
			num_combat_stats++;
		}
		if (StatsDataTestFlag(stats_data, STATS_DATA_ALWAYS_CALC))
		{
			ASSERT(stats_data->m_nSpecialFunction == STATSSPEC_NONE);
			stats_data->m_nSpecialFunction = STATSSPEC_ALWAYS_CALC;
		}
		switch (stats_data->m_nStatsType)
		{
		case STATSTYPE_REGEN:
		case STATSTYPE_DEGEN:
		case STATSTYPE_REGEN_CLIENT:
		case STATSTYPE_DEGEN_CLIENT:
			if (stats_data->m_nAssociatedStat[0] != INVALID_LINK)
			{
				STATS_DATA * regen_stat = (STATS_DATA *)ExcelGetDataPrivate(table, stats_data->m_nAssociatedStat[0]);
				ASSERT(regen_stat);
				if (regen_stat)
				{
					BOOL bFoundSlot = FALSE;
					for (unsigned int index = 0; index < STATS_NUM_ASSOCATED; index++)
					{
						if (regen_stat->m_nRegenByStat[index] == INVALID_LINK)
						{
							regen_stat->m_nRegenByStat[index] = ii;
							bFoundSlot = TRUE;
							break;
						}
					}
					ASSERT(bFoundSlot);
				}
			}
			break;
		case STATSTYPE_FEED:
			num_feed_stats++;
			break;
		case STATSTYPE_REQUIREMENT:
			num_requirement_stats++;
			break;
		case STATSTYPE_REQ_LIMIT:
			num_req_limit_stats++;
			break;
		case STATSTYPE_MAX:
		case STATSTYPE_MAX_RATIO:
		case STATSTYPE_MAX_RATIO_DECREASE:
		case STATSTYPE_MAX_PURE:
			ASSERT(stats_data->m_nAssociatedStat[0] != INVALID_LINK);
			if (stats_data->m_nAssociatedStat[0] != INVALID_LINK)
			{
				STATS_DATA * cur_stat = (STATS_DATA *)ExcelGetDataPrivate(table, stats_data->m_nAssociatedStat[0]);
				if (cur_stat)
				{
					cur_stat->m_nMaxStat = ii;
				}
			}
			SETBIT(stats_data->m_dwFlags, STATS_DATA_RECALC_CUR_ON_SET, TRUE);
			break;
		}
		if (stats_data->m_nSpecialFunction == STATSSPEC_REGEN_ON_GET)
		{
			ASSERT(stats_data->m_nSpecFuncStat[0] != INVALID_LINK);
			ASSERT(stats_data->m_nSpecFuncStat[1] != INVALID_LINK);
			STATS_DATA * regen_stat = (STATS_DATA *)ExcelGetDataPrivate(table, stats_data->m_nSpecFuncStat[1]);
			ASSERT_RETFALSE(regen_stat);
			regen_stat->m_nSpecialFunction = STATSSPEC_LINKED_REGEN;
			regen_stat->m_nSpecFuncStat[0] = ii;
			if (regen_stat->m_nMonsterRegenDelay || regen_stat->m_nRegenDelayOnDec || regen_stat->m_nRegenDelayOnZero)
			{
				ASSERT(stats_data->m_nSpecFuncStat[3] != INVALID_LINK);
				if (stats_data->m_nSpecFuncStat[3] == INVALID_LINK)
				{
					regen_stat->m_nRegenDelayOnDec = 0;
					regen_stat->m_nRegenDelayOnZero = 0;
					regen_stat->m_nMonsterRegenDelay = 0;
				}
				if(stats_data->m_nSpecFuncStat[4] != INVALID_LINK)
				{
					STATS_DATA * regen_delay_reduction_stat = (STATS_DATA *)ExcelGetDataPrivate(table, stats_data->m_nSpecFuncStat[4]);
					ASSERT_RETFALSE(regen_delay_reduction_stat);

					int nDelay = MAX(stats_data->m_nRegenDelayOnDec, regen_stat->m_nMonsterRegenDelay);
					regen_delay_reduction_stat->m_nMaxSet = nDelay;
				}
			}
		}
		if (stats_data->m_nSpecialFunction == STATSSPEC_PCT_REGEN_ON_GET)
		{
			ASSERT(stats_data->m_nSpecFuncStat[0] != INVALID_LINK);
			ASSERT(stats_data->m_nSpecFuncStat[1] != INVALID_LINK);
			STATS_DATA * regen_stat = (STATS_DATA *)ExcelGetDataPrivate(table, stats_data->m_nSpecFuncStat[1]);
			ASSERT_RETFALSE(regen_stat);
			regen_stat->m_nSpecialFunction = STATSSPEC_PCT_LINKED_REGEN;
			regen_stat->m_nSpecFuncStat[0] = ii;
			if (regen_stat->m_nMonsterRegenDelay || regen_stat->m_nRegenDelayOnDec || regen_stat->m_nRegenDelayOnZero)
			{
				ASSERT(stats_data->m_nSpecFuncStat[3] != INVALID_LINK);
				if (stats_data->m_nSpecFuncStat[3] == INVALID_LINK)
				{
					regen_stat->m_nRegenDelayOnDec = 0;
					regen_stat->m_nRegenDelayOnZero = 0;
					regen_stat->m_nMonsterRegenDelay = 0;
				}
				if(stats_data->m_nSpecFuncStat[4] != INVALID_LINK)
				{
					STATS_DATA * regen_delay_reduction_stat = (STATS_DATA *)ExcelGetDataPrivate(table, stats_data->m_nSpecFuncStat[4]);
					ASSERT_RETFALSE(regen_delay_reduction_stat);

					int nDelay = MAX(stats_data->m_nRegenDelayOnDec, regen_stat->m_nMonsterRegenDelay);
					regen_delay_reduction_stat->m_nMaxSet = nDelay;
				}
			}
		}
	}

	table->m_nDataExRowSize = sizeof(STATS_DATAEX) + 
		((num_combat_stats + num_feed_stats + num_requirement_stats + num_req_limit_stats) * sizeof(int));
	table->m_nDataExCount = 1;
	table->m_DataEx = (BYTE *)MALLOCZ(NULL, table->m_nDataExCount * table->m_nDataExRowSize);
	STATS_DATAEX * ex = (STATS_DATAEX *)table->m_DataEx;
	ex->m_pnCombatStats = (int *)((BYTE *)table->m_DataEx + sizeof(STATS_DATAEX));
	ex->m_nCombatStats = num_combat_stats;
	ex->m_pnFeedStats = (int *)((BYTE *)ex->m_pnCombatStats + sizeof(int) * ex->m_nCombatStats);
	ex->m_nFeedStats = num_feed_stats;
	ex->m_pnRequirementStats = (int *)((BYTE *)ex->m_pnFeedStats + sizeof(int) * ex->m_nFeedStats);
	ex->m_nRequirementStats = num_requirement_stats;
	ex->m_pnRequirementLimitStats = (int *)((BYTE *)ex->m_pnRequirementStats + sizeof(int) * ex->m_nRequirementStats);
	ex->m_nRequirementLimitStats = num_req_limit_stats;

	int * combat_stats = ex->m_pnCombatStats;
	int * feed_stats = ex->m_pnFeedStats;
	int * req_stats = ex->m_pnRequirementStats;
	int * req_limit_stats = ex->m_pnRequirementLimitStats;

	for (unsigned int ii = 0; ii < numStats; ii++)
	{
		const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(stats_data);

		if (StatsDataTestFlag(stats_data, STATS_DATA_COMBAT))
		{
			*combat_stats = ii;
			combat_stats++;
		}
		switch (stats_data->m_nStatsType)
		{
		case STATSTYPE_FEED:
			*feed_stats = ii;
			feed_stats++;
			break;
		case STATSTYPE_REQUIREMENT:
			*req_stats = ii;
			req_stats++;
			break;
		case STATSTYPE_REQ_LIMIT:
			*req_limit_stats = ii;
			req_limit_stats++;
			break;
		}
	}

	UnitInitStatsChangeCallbackTable();

	return TRUE;
}


//----------------------------------------------------------------------------
// stats.txt
// this postprocess function is executed after all tables have been loaded
// it is run for both text and cooked data
//----------------------------------------------------------------------------
BOOL ExcelStatsPostLoadAll(
	EXCEL_TABLE * table)
{
	REF(table);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ii++)
	{
		STATS_DATA * stats_data = (STATS_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(stats_data);

		// set up streaming parameters for each stat (save by code, bits, etc)
		for (unsigned int jj = 0; jj < STATS_MAX_PARAMS; ++jj)
		{
			stats_data->m_nParamBitsFile[jj] = stats_data->m_nParamBits[jj];

			if (stats_data->m_nParamBits[jj] > 0 && stats_data->m_nParamTable[jj] >= 0)
			{
				int max_value = ExcelGetCountPrivate(table, stats_data->m_nParamTable[jj]) + 1;		// the +1 is to leave room for all bits set = INVALID_ID
				stats_data->m_nParamBits[jj] = HIGHBIT(max_value);
				unsigned int code_index_size = ExcelGetCodeIndexSize(stats_data->m_nParamTable[jj]);
				if (code_index_size > 0)
				{
					stats_data->m_nParamBitsFile[jj] = code_index_size * 8;
				}
			}
		}

		stats_data->m_nValBitsFile = stats_data->m_nValBits;
		SETBIT(stats_data->m_dwFlags, STATS_DATA_SAVE_VAL_BY_CODE, FALSE);
		if (stats_data->m_nValBits > 0 && stats_data->m_nValTable >= 0)
		{
			int max_value = ExcelGetCountPrivate(table, stats_data->m_nValTable);
			stats_data->m_nValBits = HIGHBIT(max_value);
			unsigned int code_index_size = ExcelGetCodeIndexSize(stats_data->m_nValTable);
			if (code_index_size > 0)
			{
				stats_data->m_nValBitsFile = code_index_size * 8;
				SETBIT(stats_data->m_dwFlags, STATS_DATA_SAVE_VAL_BY_CODE, TRUE);
			}
		}

		// set up assertion extents based on bits
		if (StatsDataTestFlag(stats_data, STATS_DATA_SEND_TO_ALL) || StatsDataTestFlag(stats_data, STATS_DATA_SEND_TO_SELF) || StatsDataTestFlag(stats_data, STATS_DATA_SAVE))
		{
			if (!StatsDataTestFlag(stats_data, STATS_DATA_SAVE_VAL_BY_CODE) && !StatsDataTestFlag(stats_data, STATS_DATA_FLOAT) && !StatsDataTestFlag(stats_data, STATS_DATA_VECTOR) && 
				stats_data->m_nValShift > 0)
			{
				if (stats_data->m_nValBits + stats_data->m_nValShift < 32)
				{
					stats_data->m_nMaxAssert = MIN(stats_data->m_nMaxAssert, (((1 << stats_data->m_nValBits) - stats_data->m_nValOffs) << stats_data->m_nValShift));
					ASSERT(stats_data->m_nValOffs >= 0);
					stats_data->m_nMinAssert = MAX(stats_data->m_nMinAssert, -(stats_data->m_nValOffs << stats_data->m_nValShift));
				}
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// predec for excel parse function
//----------------------------------------------------------------------------
static void ExcelStatsFuncParse(
	GAME * game,
	const char * cCalcString,
	unsigned int nCalcStringBufLen,
	STATS_FUNCTION * statsfunc,
	unsigned int nLine);

static void sStatsParseCalcFreeNode(
	STATS_CALC * node);

static void sStatsParsePrintAffectsList(
	void);

	
//----------------------------------------------------------------------------
// called after all tables have been loaded (both cooked and raw)
//----------------------------------------------------------------------------
BOOL ExcelStatsFuncPostLoadAll(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(arrsize(tblStatsOperator) == NUM_STATS_TOKEN);
	for (unsigned int ii = 0; ii < NUM_STATS_TOKEN; ++ii)
	{
		ASSERT(tblStatsOperator[ii].m_nEnum == (int)ii);
	}
	for (unsigned int ii = 0; ii < (unsigned int)ExcelGetCountPrivate(table); ++ii)
	{
		STATS_FUNCTION * statsfunc = (STATS_FUNCTION *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(statsfunc);

		ExcelStatsFuncParse(NULL, statsfunc->m_szCalc, MAX_FORMULA_STRLEN, statsfunc, ii);
	}

	sStatsParsePrintAffectsList();

	return TRUE;
}

	
//----------------------------------------------------------------------------
// callback on freeing the stats table for each row
//----------------------------------------------------------------------------
void ExcelStatsRowFree(
	struct EXCEL_TABLE * table,
	BYTE * rowdata)
{
	STATS_DATA * stats_data = (STATS_DATA *)rowdata;
	ASSERT_RETURN(stats_data);

	if (stats_data->m_FormulaAffectedByList)
	{
		FREE(NULL, stats_data->m_FormulaAffectedByList);
		stats_data->m_FormulaAffectedByList = NULL;
	}
	stats_data->m_nFormulaAffectedByList;

	if (stats_data->m_FormulaAffectsList)
	{
		FREE(NULL, stats_data->m_FormulaAffectsList);
		stats_data->m_FormulaAffectsList = NULL;
	}
	stats_data->m_nFormulaAffectsList;

	{
		STATS_CALLBACK_TABLE * curr = stats_data->m_SvrStatsChangeCallback;
		while (curr)
		{
			STATS_CALLBACK_TABLE * next = curr->m_pNext;
			FREE(NULL, curr);
			curr = next;
		}
		stats_data->m_SvrStatsChangeCallback = NULL;
	}

	{
		STATS_CALLBACK_TABLE * curr = stats_data->m_CltStatsChangeCallback;
		while (curr)
		{
			STATS_CALLBACK_TABLE * next = curr->m_pNext;
			FREE(NULL, curr);
			curr = next;
		}
		stats_data->m_CltStatsChangeCallback = NULL;
	}
}


//----------------------------------------------------------------------------
// callback on freeing the stats table for each row
//----------------------------------------------------------------------------
void ExcelStatsFuncRowFree(
	struct EXCEL_TABLE * table,
	BYTE * rowdata)
{
	STATS_FUNCTION * statsfunc = (STATS_FUNCTION *)rowdata;
	ASSERT_RETURN(statsfunc);

	if (statsfunc->m_Calc)
	{
		sStatsParseCalcFreeNode(statsfunc->m_Calc);
		statsfunc->m_Calc = NULL;
	}
}


//----------------------------------------------------------------------------
// STATIC FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const STATS_DATA * sStatsGetStatsData(
	int wStat,
	const STATS_DATA * stats_data)
{
	if (stats_data)
	{
		return stats_data;
	}
	stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	return stats_data;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StatsTestFlag(
	const STATS * stats,
	int flag)
{
	ASSERT(stats);
	return TESTBIT(stats->m_Flags, flag);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sStatsSetFlag(
	STATS * stats,
	int flag)
{
	ASSERT(stats);
	SETBIT(stats->m_Flags, flag);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sStatsSetFlag(
	STATS * stats,
	int flag,
	BOOL value)
{
	ASSERT(stats);
	SETBIT(stats->m_Flags, flag, value);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sStatsClearFlag(
	STATS * stats,
	int flag)
{
	ASSERT(stats);
	CLEARBIT(stats->m_Flags, flag);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PARSE_STAT_STACK
{
	STATS_CALC *					top;								// stack of operators
	unsigned int					count;								// number of nodes on operator_stack
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PARSE_STAT_CALC
{
	// tokenizing
	const char *					start;								// entire stats calc formula
	const char *					cur;								// current character being parsed (points into start)
	const char *					tokstr;								// pointer to start of token (points into start)
	unsigned int					toklen;								// length of token so far
	unsigned int					token;								// token enum
	int								tokidx;								// additional token info (constant value, stat index, etc)
	//
	const STATS_DATA *				target_data;						// data for target_stat
	PARSE_STAT_STACK				operator_stack;						// stack of operators
	PARSE_STAT_STACK				operand_stack;						// stack of operands
	BOOL							bAllowBinaryOperator;				// set if last parse construct is an operand or )
};


//----------------------------------------------------------------------------
// validate that the globally defined operator table for stats calcs is valid
//----------------------------------------------------------------------------
static void sStatsParseCalcValidateOperatorTable(
	void)
{
	static BOOL s_Validated = FALSE;
	if (s_Validated)
	{
		return;
	}
	s_Validated = TRUE;
	ASSERT(arrsize(tblStatsOperator) == STATS_TOKEN_EOL);
	for (int ii = 0; ii < arrsize(tblStatsOperator); ++ii)
	{
		ASSERT(tblStatsOperator[ii].m_nEnum == ii);
	}
}


//----------------------------------------------------------------------------
// return the precedence for a stats calc token
//----------------------------------------------------------------------------
static unsigned int sStatsParseCalcGetPrecedence(
	unsigned int token)
{
	ASSERT_RETZERO(token < arrsize(tblStatsOperator));
	return tblStatsOperator[token].m_nPrecedence;
}


//----------------------------------------------------------------------------
// return the associativity for a stats calc token
//----------------------------------------------------------------------------
static unsigned int sStatsParseCalcGetAssociativity(
	unsigned int token)
{
	ASSERT_RETZERO(token < arrsize(tblStatsOperator));
	return tblStatsOperator[token].m_nAssociativity;
}


//----------------------------------------------------------------------------
// return the string for a stats calc token
//----------------------------------------------------------------------------
static const char * sStatsParseCalcGetTokStr(
	unsigned int token)
{
	ASSERT_RETZERO(token < arrsize(tblStatsOperator));
	return tblStatsOperator[token].m_szToken;
}


//----------------------------------------------------------------------------
// return the number of expected operands for a stats calc token
//----------------------------------------------------------------------------
static unsigned sStatsParseCalcGetOperandCount(
	unsigned int token)
{
	ASSERT_RETZERO(token < arrsize(tblStatsOperator));
	return tblStatsOperator[token].m_nOperandCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sStatsCalcOpUnary(
	unsigned int token,
	int a)
{
	ASSERT_RETZERO(token < arrsize(tblStatsOperator));
	return tblStatsOperator[token].m_fpUnaryFunction(a);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sStatsCalcOpBinary(
	unsigned int token,
	int a,
	int b)
{
	ASSERT_RETZERO(token < arrsize(tblStatsOperator));
	return tblStatsOperator[token].m_fpBinaryFunction(a, b);
}


//----------------------------------------------------------------------------
// fills out parse structure for a given token (of operator type)
//----------------------------------------------------------------------------
static BOOL sStatsParseCalcGetTokenOperator(
	PARSE_STAT_CALC & parse,
	unsigned int toklen,
	unsigned int token,
	int tokidx = -1)
{
	parse.tokstr = parse.cur;
	parse.toklen = toklen;
	parse.cur += toklen;
	parse.token = token;
	parse.tokidx = tokidx;
	return TRUE;
}


//----------------------------------------------------------------------------
// parses an integer constant token, templated for negative vs. positive constants
//----------------------------------------------------------------------------
template <BOOL NEG>
static BOOL sStatsParseCalcGetTokenConstant(
	PARSE_STAT_CALC & parse)
{
	parse.tokstr = parse.cur;
	parse.cur += (NEG ? 1 : 0);
	int num = 0;
	do 
	{
		num = num * 10 + (parse.cur[0] - '0');
		++parse.cur;
	} while (parse.cur[0] >= '0' && parse.cur[0] <= '9');
	parse.toklen = SIZE_TO_UINT(parse.cur - parse.tokstr);
	parse.token = STATS_TOKEN_CONSTANT;
	parse.tokidx = (NEG ? -num : num);
	return TRUE;
}


//----------------------------------------------------------------------------
// helps parse an identifier (stat name, etc)
//----------------------------------------------------------------------------
static BOOL sStatsParseCalcGetTokenIdentifier(
	PARSE_STAT_CALC & parse,
	char * token,
	unsigned int maxlen)
{
	parse.tokstr = parse.cur;
	parse.cur++;
	do 
	{
		char c = *parse.cur;
		__assume(c <= 255);
		switch (c)
		{
		case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'g': case 'h':
		case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p':
		case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z': 
		case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F': case 'G': case 'H':
		case 'I': case 'J': case 'K': 
		case 'M': case 'N': case 'O': case 'P':
		case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z': 
		case '_':
			++parse.cur;
			break;
		default:
			goto _gotiden;
		}
	} while (1);

_gotiden:
	parse.toklen = SIZE_TO_UINT(parse.cur - parse.tokstr);
	if (parse.toklen >= maxlen)
	{
		return FALSE;
	}
	MemoryCopy(token, maxlen, parse.tokstr, sizeof(char) * parse.toklen);
	token[parse.toklen] = 0;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStatsParseIsToken(
	PARSE_STAT_CALC & parse,
	const char * token)
{
	ASSERT_RETFALSE(token);
	if (parse.cur == NULL)
	{
		return FALSE;
	}
	unsigned int len = PStrLen(token);
	if (PStrICmp(parse.cur, token, len) != 0)
	{
		return FALSE;
	}
	parse.cur += len;
	return TRUE;
}


//----------------------------------------------------------------------------
// parses an identifier (stat name, etc)
//----------------------------------------------------------------------------
static BOOL sStatsParseCalcGetTokenIdentifier(
	PARSE_STAT_CALC & parse)
{
	char token[MAX_TOK_LEN];
	if (!sStatsParseCalcGetTokenIdentifier(parse, token, arrsize(token)))
	{
		return FALSE;
	}
	parse.toklen = 0;

	// see if it's a stat
	int stat = ExcelGetLineByStringIndex(NULL, DATATABLE_STATS, token);
	if (stat != INVALID_LINK)
	{
		const STATS_DATA * stats_data = StatsGetData(NULL, stat);
		ASSERT_RETFALSE(stats_data);

		// parse qualifiers (.base etc)
		if (!sStatsParseIsToken(parse, "."))
		{
			parse.token = STATS_TOKEN_STAT;
			parse.tokidx = stat;
			return TRUE;
		}
		if (sStatsParseIsToken(parse, "0"))
		{
			parse.token = STATS_TOKEN_STAT_PARAM_ZERO;
			parse.tokidx = stat;
			return TRUE;
		}
		if (sStatsParseIsToken(parse, "N"))
		{
			parse.token = STATS_TOKEN_STAT_PARAM_N;
			parse.tokidx = stat;
			return TRUE;
		}
		if (sStatsParseIsToken(parse, "shift"))
		{
			parse.token = STATS_TOKEN_CONSTANT;
			parse.tokidx = stats_data->m_nShift;
			return TRUE;
		}
	}

	for (unsigned int ii = 0; ii < arrsize(tblStatsOperator); ++ii)
	{
		if (!tblStatsOperator[ii].m_bIsIdentifier)
		{
			continue;
		}
		// see if it's a variable, etc.
		if (PStrICmp(token, tblStatsOperator[ii].m_szToken) == 0)
		{
			parse.token = tblStatsOperator[ii].m_nEnum;
			parse.tokidx = -1;
			return TRUE;
		}
	}

	// unrecognized identifier
	return FALSE;
}


//----------------------------------------------------------------------------
// parse a single token
//----------------------------------------------------------------------------
static BOOL sStatsParseCalcGetToken(
	PARSE_STAT_CALC & parse)
{
	parse.tokstr = NULL;

	while (1)
	{
		char c = *parse.cur;
		switch (c)
		{
		case 0:
			parse.token = STATS_TOKEN_EOL;
			parse.tokidx = -1;
			return TRUE;
		case ' ':
		case '\t':
		case '\f':
		case '\v':
		case '\r':
		case '\n':
			++parse.cur;
			continue;
		case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'g': case 'h':
		case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p':
		case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z': 
		case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F': case 'G': case 'H':
		case 'I': case 'J': case 'K': 
		case 'M': case 'N': case 'O': case 'P':
		case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z': 
		case '_':
			return sStatsParseCalcGetTokenIdentifier(parse);
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
			return sStatsParseCalcGetTokenConstant<FALSE>(parse);
		case '<':
			if (parse.cur[1] == '<')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_LSHIFT);
			}
			if (parse.cur[1] == '=')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_LTE);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_LT);
		case '>':
			if (parse.cur[1] == '>')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_RSHIFT);
			}
			if (parse.cur[1] == '=')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_GTE);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_GT);
		case '&':
			if (parse.cur[1] == '&')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_LAND);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_AND);
		case '|':
			if (parse.cur[1] == '|')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_LOR);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_OR);
		case '+':
			if (parse.cur[1] == '%')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_PLUSPCT);
			}
			if (parse.bAllowBinaryOperator)
			{
				return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_ADD);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_POS);
		case '-':
			if (parse.bAllowBinaryOperator)
			{
				return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_SUB);
			}
			if (parse.cur[1] >= '0' && parse.cur[1] <= '9')
			{
				return sStatsParseCalcGetTokenConstant<TRUE>(parse);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_NEG);
		case '!':
			if (parse.cur[1] == '=')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_NEQ);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_NOT);	
		case '=':
			if (parse.cur[1] == '=')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_EQ);
			}
			return FALSE;
		case '*':
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_MUL);	
		case '/':
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_DIV);	
		case '%':
			if (parse.cur[1] == '%')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_PCT);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_MOD);	
		case '^':
			if (parse.cur[1] == '^')
			{
				return sStatsParseCalcGetTokenOperator(parse, 2, STATS_TOKEN_POWER);
			}
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_XOR);	
		case '(':
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_LPAREN);	
		case ')':
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_RPAREN);	
		case ':':
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_COLON);	
		case '?':
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_QUESTION);	
		case '~':
			return sStatsParseCalcGetTokenOperator(parse, 1, STATS_TOKEN_TILDE);	
		default:
			// unrecognized character
			return FALSE;
		}
	}
}


//----------------------------------------------------------------------------
// allocate a new STATS_CALC node
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcGetNode(
	unsigned int token,
	int tokidx = -1)
{
	STATS_CALC * node = (STATS_CALC *)MALLOCZ(NULL, sizeof(STATS_CALC));
	node->m_nToken = token;
	node->m_nTokIdx = tokidx;
	return node;
}


//----------------------------------------------------------------------------
// pushes a stats_calc node onto a stack
//----------------------------------------------------------------------------
static void sStatsParseCalcPushNode(
	PARSE_STAT_STACK & stack,
	STATS_CALC * node)
{
	ASSERT_RETURN(node);
	node->m_Next = stack.top;
	stack.top = node;
	++stack.count;
}


//----------------------------------------------------------------------------
// allocates & pushes a stats_calc node onto a stack
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcPushNode(
	PARSE_STAT_STACK & stack,
	unsigned int token,
	int tokidx = -1)
{
	STATS_CALC * node = sStatsParseCalcGetNode(token, tokidx);
	node->m_nToken = token;
	node->m_nTokIdx = tokidx;
	sStatsParseCalcPushNode(stack, node);
	return node;
}


//----------------------------------------------------------------------------
// pop a stats_calc node off of a stack
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcPopNode(
	PARSE_STAT_STACK & stack)
{
	ASSERT_RETNULL(stack.count > 0);
	STATS_CALC * node = stack.top;
	ASSERT_RETNULL(node);
	stack.top = node->m_Next;
	--stack.count;
	node->m_Next = node->m_Prev = NULL;
	return node;
}


//----------------------------------------------------------------------------
// add a child to the end of a parent node
//----------------------------------------------------------------------------
static void sStatsParseCalcAddChild(
	STATS_CALC * parent,
	STATS_CALC * child)
{
	ASSERT_RETURN(parent && child);
	child->m_Parent = parent;
	child->m_Prev = parent->m_ChildLast;
	if (child->m_Prev)
	{
		child->m_Prev->m_Next = child;
	}
	else
	{
		ASSERT(parent->m_ChildFirst == NULL);
		parent->m_ChildFirst = child;
	}
	parent->m_ChildLast = child;
	++parent->m_nChildren;
	child->m_Next = NULL;
}


//----------------------------------------------------------------------------
// add a child to the head of child list
//----------------------------------------------------------------------------
static void sStatsParseCalcAddChildToHead(
	STATS_CALC * parent,
	STATS_CALC * child)
{
	ASSERT_RETURN(parent && child);
	child->m_Parent = parent;
	child->m_Prev = NULL;
	child->m_Next = parent->m_ChildFirst;
	if (child->m_Next)
	{
		child->m_Next->m_Prev = child;
	}
	else
	{
		ASSERT(parent->m_ChildLast == NULL);
		parent->m_ChildLast = child;
	}
	parent->m_ChildFirst = child;
	++parent->m_nChildren;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcDetachNode(
	STATS_CALC * node)
{
	if (!node)
	{	
		return NULL;
	}
	// unlink from parent
	if (node->m_Parent)
	{
		STATS_CALC * parent = node->m_Parent;
		if (parent->m_ChildFirst == node)
		{
			ASSERT(node->m_Prev == NULL);
			parent->m_ChildFirst = node->m_Next;
		}
		if (parent->m_ChildLast == node)
		{
			ASSERT(node->m_Next == NULL);
			parent->m_ChildLast = node->m_Prev;
		}
		--parent->m_nChildren;
		node->m_Parent = NULL;
	}
	// unlink from sibs
	if (node->m_Prev)
	{
		node->m_Prev->m_Next = node->m_Next;
	}
	if (node->m_Next)
	{
		node->m_Next->m_Prev = node->m_Prev;
	}
	node->m_Prev = node->m_Next = NULL;
	return node;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsParseCalcFreeNode(
	STATS_CALC * node)
{
	if (!node)
	{	
		return;
	}
	sStatsParseCalcDetachNode(node);
	// free all children
	while (node->m_ChildFirst)
	{
		sStatsParseCalcFreeNode(node->m_ChildFirst);
	}
	// free self
	FREE(NULL, node);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const STATS_CALC * sStatsParseCalcGetFirstChild(
	const STATS_CALC * parent)
{
	ASSERT_RETNULL(parent->m_nChildren >= 1);
	return parent->m_ChildFirst;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcGetFirstChild(
	STATS_CALC * parent)
{
	ASSERT_RETNULL(parent->m_nChildren >= 1);
	return parent->m_ChildFirst;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const STATS_CALC * sStatsParseCalcGetSecondChild(
	const STATS_CALC * parent,
	const STATS_CALC * child)
{
	ASSERT_RETNULL(child);
	ASSERT_RETNULL(parent->m_nChildren >= 2);
	return child->m_Next;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcGetSecondChild(
	STATS_CALC * parent,
	STATS_CALC * child)
{
	ASSERT_RETNULL(child);
	ASSERT_RETNULL(parent->m_nChildren >= 2);
	return child->m_Next;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStatsParseCalcIsEqual(
	STATS_CALC * a,
	STATS_CALC * b)
{
	ASSERT_RETFALSE(a && b);
	if (a->m_nToken != b->m_nToken)
	{
		return FALSE;
	}
	if (a->m_nTokIdx != b->m_nTokIdx)
	{
		return FALSE;
	}
	if (a->m_nChildren != b->m_nChildren)
	{
		return FALSE;
	}
	STATS_CALC * childa = a->m_ChildFirst;
	STATS_CALC * childb = b->m_ChildFirst;
	for (; childa; childa = childa->m_Next, childb = childb->m_Next)
	{
		if (!sStatsParseCalcIsEqual(childa, childb))
		{
			return FALSE;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// this converts a node into a constant & frees its children
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcOptimizeConstant(
	STATS_CALC * node,
	int value)
{
	node->m_nToken = STATS_TOKEN_CONSTANT;
	node->m_nTokIdx = value;
	while (node->m_ChildFirst)
	{
		sStatsParseCalcFreeNode(node->m_ChildFirst);
	}
	return node;
}


//----------------------------------------------------------------------------
// detach the child and delete the parent, return child
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcOptimizeGetChild(
	STATS_CALC * child)
{
	ASSERT_RETNULL(child);
	STATS_CALC * parent = child->m_Parent;
	if (parent)
	{
		sStatsParseCalcDetachNode(child);
		sStatsParseCalcFreeNode(parent);
	}
	return child;
}


//----------------------------------------------------------------------------
// detach the child of the child and delete the parent, return detached node
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcOptimizeDrillDown(
	STATS_CALC * parent,
	STATS_CALC * child)
{
	child = sStatsParseCalcGetFirstChild(child);
	ASSERT_RETNULL(child);
	sStatsParseCalcDetachNode(child);
	sStatsParseCalcFreeNode(parent);
	return child;
}


//----------------------------------------------------------------------------
// determine if we can do constant folding on associative expression
//----------------------------------------------------------------------------
static BOOL sStatsParseCalcOptimizeAssociativeConstantFolding(
	STATS_CALC *& node,
	STATS_CALC * a,
	STATS_CALC * b,
	unsigned int token)
{
	if ((a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == token))
	{
		STATS_CALC * ba = sStatsParseCalcGetFirstChild(b);
		ASSERT_RETFALSE(ba);
		STATS_CALC * bb = sStatsParseCalcGetSecondChild(b, ba);
		ASSERT_RETFALSE(bb);
		if (bb->m_nToken == STATS_TOKEN_CONSTANT)
		{
			bb->m_nTokIdx = sStatsCalcOpBinary(token, bb->m_nTokIdx, a->m_nTokIdx);
			node = sStatsParseCalcOptimizeGetChild(b);
			return TRUE;
		}
		if (ba->m_nToken == STATS_TOKEN_CONSTANT)
		{
			ba->m_nTokIdx = sStatsCalcOpBinary(token, ba->m_nTokIdx, a->m_nTokIdx);
			node = sStatsParseCalcOptimizeGetChild(b);
			return TRUE;
		}
	}
	if ((b->m_nToken == STATS_TOKEN_CONSTANT && a->m_nToken == token))
	{
		STATS_CALC * aa = sStatsParseCalcGetFirstChild(a);
		ASSERT_RETFALSE(aa);
		STATS_CALC * ab = sStatsParseCalcGetSecondChild(a, aa);
		ASSERT_RETFALSE(ab);
		if (ab->m_nToken == STATS_TOKEN_CONSTANT)
		{
			ab->m_nTokIdx = sStatsCalcOpBinary(token, ab->m_nTokIdx, b->m_nTokIdx);
			node = sStatsParseCalcOptimizeGetChild(a);
			return TRUE;
		}
		if (aa->m_nToken == STATS_TOKEN_CONSTANT)
		{
			aa->m_nTokIdx = sStatsCalcOpBinary(token, aa->m_nTokIdx, b->m_nTokIdx);
			node = sStatsParseCalcOptimizeGetChild(a);
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcOptimize(
	STATS_CALC * node)
{
	if (!node)
	{
		return NULL;
	}

again:
	STATS_CALC * a = NULL;
	STATS_CALC * b = NULL;

	switch (sStatsParseCalcGetOperandCount(node->m_nToken))
	{
	case 1:
		a = sStatsParseCalcGetFirstChild(node);
		ASSERT_GOTO(a, _error);
		break;
	case 2:
		a = sStatsParseCalcGetFirstChild(node);
		ASSERT_GOTO(a, _error);
		b = sStatsParseCalcGetSecondChild(node, a);
		ASSERT_GOTO(b, _error);
		break;
	}

	switch (node->m_nToken)
	{
	case STATS_TOKEN_NEG:
		if (a->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, -a->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_NEG)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_TILDE:
		if (a->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, ~a->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_TILDE)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_NOT:
		if (a->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, !a->m_nTokIdx);
			break;
		}
		// note that !!5 == 1, however !!!5 = !5
		if (a->m_nToken == STATS_TOKEN_NOT)
		{
			STATS_CALC * aa = sStatsParseCalcGetFirstChild(a);
			ASSERT_GOTO(aa, _error);
			if (aa->m_nToken == STATS_TOKEN_NOT)
			{
				node = sStatsParseCalcOptimizeDrillDown(node, a);
				break;
			}
		}
		break;
	case STATS_TOKEN_POWER:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, sStatsCalcOpPower(a->m_nTokIdx, b->m_nTokIdx));
			break;
		}
		if ((a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0) ||
			(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx < 0))
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if ((a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 1) ||
			(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0))
		{
			node = sStatsParseCalcOptimizeConstant(node, 1);
			break;
		}
		break;
	case STATS_TOKEN_MUL:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx * b->m_nTokIdx);
			break;
		}
		if ((a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0) ||
			(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0))
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 1)
		{
			node = sStatsParseCalcOptimizeGetChild(b);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 1)
		{
			node = sStatsParseCalcOptimizeGetChild(a);
			break;
		}
		if (sStatsParseCalcOptimizeAssociativeConstantFolding(node, a, b, STATS_TOKEN_MUL))
		{
			goto again;
		}
		break;
	case STATS_TOKEN_DIV:
		ASSERT_GOTO(!(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0), _error);
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx / b->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 1)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_MOD:
		ASSERT_GOTO(!(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0), _error);
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx % b->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 1)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_PCT:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, PCT(a->m_nTokIdx, b->m_nTokIdx));
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 100)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_PLUSPCT:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, PCT(a->m_nTokIdx, 100 + b->m_nTokIdx));
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_ADD:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx + b->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, b);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		if (sStatsParseCalcOptimizeAssociativeConstantFolding(node, a, b, STATS_TOKEN_ADD))
		{
			goto again;
		}
		break;
	case STATS_TOKEN_SUB:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx - b->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			b = sStatsParseCalcOptimizeDrillDown(node, b);
			node = sStatsParseCalcGetNode(STATS_TOKEN_NEG);
			sStatsParseCalcAddChild(node, b);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_LSHIFT:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx << b->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_RSHIFT:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx >> b->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_LT:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx < b->m_nTokIdx);
			break;
		}
		break;
	case STATS_TOKEN_GT:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx > b->m_nTokIdx);
			break;
		}
		break;
	case STATS_TOKEN_LTE:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx <= b->m_nTokIdx);
			break;
		}
		break;
	case STATS_TOKEN_GTE:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx >= b->m_nTokIdx);
			break;
		}
		break;
	case STATS_TOKEN_EQ:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx == b->m_nTokIdx);
			break;
		}
		break;
	case STATS_TOKEN_NEQ:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx != b->m_nTokIdx);
			break;
		}
		break;
	case STATS_TOKEN_AND:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx & b->m_nTokIdx);
			break;
		}
		if ((a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0) ||
			(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0))
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == -1)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, b);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == -1)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		break;
	case STATS_TOKEN_XOR:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx ^ b->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, b);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		// note that N^N == 0, but not worth it to optimize this
		// note that N^-1 == ~N, but not worth it to optimize this
		break;
	case STATS_TOKEN_OR:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx | b->m_nTokIdx);
			break;
		}
		if (a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, b);
			break;
		}
		if (b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0)
		{
			node = sStatsParseCalcOptimizeDrillDown(node, a);
			break;
		}
		if ((a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == -1) ||
			(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == -1))
		{
			node = sStatsParseCalcOptimizeConstant(node, -1);
			break;
		}
		break;
	case STATS_TOKEN_LAND:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx && b->m_nTokIdx);
			break;
		}
		if ((a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx == 0) ||
			(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx == 0))
		{
			node = sStatsParseCalcOptimizeConstant(node, 0);
			break;
		}
		break;
	case STATS_TOKEN_LOR:
		if (a->m_nToken == STATS_TOKEN_CONSTANT && b->m_nToken == STATS_TOKEN_CONSTANT)
		{
			node = sStatsParseCalcOptimizeConstant(node, a->m_nTokIdx || b->m_nTokIdx);
			break;
		}
		if ((a->m_nToken == STATS_TOKEN_CONSTANT && a->m_nTokIdx != 0) ||
			(b->m_nToken == STATS_TOKEN_CONSTANT && b->m_nTokIdx != 0))
		{
			node = sStatsParseCalcOptimizeConstant(node, 1);
			break;
		}
		break;
	case STATS_TOKEN_QUESTION:
		ASSERT_RETFALSE(b->m_nToken == STATS_TOKEN_COLON && b->m_nChildren == 2);
		{
			STATS_CALC * ba = sStatsParseCalcGetFirstChild(b);
			ASSERT_GOTO(ba, _error);
			STATS_CALC * bb = sStatsParseCalcGetSecondChild(b, ba);
			ASSERT_GOTO(bb, _error);

			if (a->m_nToken == STATS_TOKEN_CONSTANT)
			{
				if (a->m_nTokIdx != 0)
				{
					node = sStatsParseCalcOptimizeDrillDown(node, ba);
					break;
				}
				else
				{
					node = sStatsParseCalcOptimizeDrillDown(node, bb);
					break;
				}
			}
			if (sStatsParseCalcIsEqual(ba, bb))
			{
				node = sStatsParseCalcOptimizeDrillDown(node, ba);
			}
		}
		break;
	case STATS_TOKEN_COLON:
	case STATS_TOKEN_LPAREN:
	case STATS_TOKEN_RPAREN:
	case STATS_TOKEN_CONSTANT:
	case STATS_TOKEN_STAT:
	case STATS_TOKEN_STAT_PARAM_ZERO:
	case STATS_TOKEN_STAT_PARAM_N:
	case STATS_TOKEN_LOCAL_DMG_MIN:
	case STATS_TOKEN_LOCAL_DMG_MAX:
	case STATS_TOKEN_SPLASH_DMG_MIN:
	case STATS_TOKEN_SPLASH_DMG_MAX:
	case STATS_TOKEN_THORNS_DMG_MIN:
	case STATS_TOKEN_THORNS_DMG_MAX:
	default:
		break;
	}
	return node;

_error:
	sStatsParseCalcFreeNode(node);
	return NULL;
}


//----------------------------------------------------------------------------
// pop lower precedence operators & operands to form expressions
//----------------------------------------------------------------------------
static BOOL sStatsParseCalcConstructExpression(
	PARSE_STAT_CALC & parse,
	unsigned int token)
{
	unsigned int precedence = sStatsParseCalcGetPrecedence(token);
	unsigned int associativity = sStatsParseCalcGetAssociativity(token);

	STATS_CALC * op;
	while ((op = parse.operator_stack.top) != NULL)
	{
		unsigned int peek_prec = sStatsParseCalcGetPrecedence(op->m_nToken);
		if (peek_prec > precedence)
		{
			break;
		}
		if (peek_prec == precedence && associativity == 1)
		{
			break;
		}

		unsigned int operand_count = sStatsParseCalcGetOperandCount(op->m_nToken);
		ASSERT_RETFALSE(parse.operand_stack.count >= operand_count);

		switch (operand_count)
		{
		case 1:
			{
				STATS_CALC * a = sStatsParseCalcPopNode(parse.operand_stack);
				ASSERT_RETFALSE(a);
				op = sStatsParseCalcPopNode(parse.operator_stack);
				sStatsParseCalcAddChild(op, a);
				ASSERT_RETFALSE((op = sStatsParseCalcOptimize(op)) != NULL);
				sStatsParseCalcPushNode(parse.operand_stack, op);
			}
			break;
		case 2:
			{
				STATS_CALC * b = sStatsParseCalcPopNode(parse.operand_stack);
				ASSERT_RETFALSE(b);
				STATS_CALC * a = sStatsParseCalcPopNode(parse.operand_stack);
				ASSERT_RETFALSE(a);
				if (op->m_nToken == STATS_TOKEN_QUESTION)
				{
					ASSERT_RETFALSE(b->m_nToken == STATS_TOKEN_COLON);
				}
				op = sStatsParseCalcPopNode(parse.operator_stack);
				sStatsParseCalcAddChild(op, a);
				sStatsParseCalcAddChild(op, b);
				ASSERT_RETFALSE((op = sStatsParseCalcOptimize(op)) != NULL);
				sStatsParseCalcPushNode(parse.operand_stack, op);
			}
			break;
		default:
			if (op->m_nToken == STATS_TOKEN_LPAREN)
			{
				op = sStatsParseCalcPopNode(parse.operator_stack);
				sStatsParseCalcFreeNode(op);
				if (precedence > 0)		// pop only one left paren
				{
					return TRUE;
				}
				break;
			}
			ASSERT_RETFALSE(0);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// parse a token
//----------------------------------------------------------------------------
static BOOL sStatsParseCalc(
	PARSE_STAT_CALC & parse)
{
	while (sStatsParseCalcGetToken(parse))
	{
		switch (parse.token)
		{
		case STATS_TOKEN_NEG:
		case STATS_TOKEN_TILDE:
		case STATS_TOKEN_NOT:
			ASSERT_RETFALSE(!parse.bAllowBinaryOperator);
			// pop higher-precedence expression
			ASSERT_RETFALSE(sStatsParseCalcConstructExpression(parse, parse.token));
			// push new operator
			sStatsParseCalcPushNode(parse.operator_stack, parse.token, parse.tokidx);
			break;
		case STATS_TOKEN_POWER:
		case STATS_TOKEN_MUL:
		case STATS_TOKEN_DIV:
		case STATS_TOKEN_MOD:
		case STATS_TOKEN_PCT:
		case STATS_TOKEN_PLUSPCT:
		case STATS_TOKEN_ADD:
		case STATS_TOKEN_SUB:
		case STATS_TOKEN_LSHIFT:
		case STATS_TOKEN_RSHIFT:
		case STATS_TOKEN_LT:
		case STATS_TOKEN_GT:
		case STATS_TOKEN_LTE:
		case STATS_TOKEN_GTE:
		case STATS_TOKEN_EQ:
		case STATS_TOKEN_NEQ:
		case STATS_TOKEN_AND:
		case STATS_TOKEN_XOR:
		case STATS_TOKEN_OR:
		case STATS_TOKEN_LAND:
		case STATS_TOKEN_LOR:
		case STATS_TOKEN_QUESTION:
		case STATS_TOKEN_COLON:
			ASSERT_RETFALSE(parse.bAllowBinaryOperator);
			// pop higher-precedence expression
			ASSERT_RETFALSE(sStatsParseCalcConstructExpression(parse, parse.token));
			// push new operator
			sStatsParseCalcPushNode(parse.operator_stack, parse.token, parse.tokidx);
			parse.bAllowBinaryOperator = FALSE;
			break;
		case STATS_TOKEN_LPAREN:
			sStatsParseCalcPushNode(parse.operator_stack, parse.token);
			parse.bAllowBinaryOperator = FALSE;
			break;
		case STATS_TOKEN_RPAREN:
			// pop to left-paren
			ASSERT_RETFALSE(sStatsParseCalcConstructExpression(parse, parse.token));
			parse.bAllowBinaryOperator = TRUE;
			break;
		case STATS_TOKEN_CONSTANT:
		case STATS_TOKEN_STAT:
		case STATS_TOKEN_STAT_PARAM_ZERO:
		case STATS_TOKEN_STAT_PARAM_N:
		case STATS_TOKEN_LOCAL_DMG_MIN:
		case STATS_TOKEN_LOCAL_DMG_MAX:
		case STATS_TOKEN_SPLASH_DMG_MIN:
		case STATS_TOKEN_SPLASH_DMG_MAX:
		case STATS_TOKEN_THORNS_DMG_MIN:
		case STATS_TOKEN_THORNS_DMG_MAX:
			sStatsParseCalcPushNode(parse.operand_stack, parse.token, parse.tokidx);
			parse.bAllowBinaryOperator = TRUE;
			break;
		case STATS_TOKEN_EOL:
			ASSERT_RETFALSE(sStatsParseCalcConstructExpression(parse, parse.token));
			ASSERT_RETFALSE(parse.operator_stack.top == NULL);
			ASSERT_RETFALSE(parse.operand_stack.count <= 1);
			return TRUE;
		default:
			ASSERT_RETFALSE(0);
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// collapse all top-level ADDs to a single SUM
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalcRearrangeAsSum(
	STATS_CALC * parent,
	STATS_CALC * node)
{	
	if (node->m_nToken != STATS_TOKEN_ADD)
	{
		if (!parent)
		{
			return node;
		}
		if (node->m_nToken == STATS_TOKEN_CONSTANT)	// collapse all constants together at front of list
		{
			if (parent->m_ChildFirst != NULL && parent->m_ChildFirst->m_nToken == STATS_TOKEN_CONSTANT)
			{
				parent->m_ChildFirst->m_nTokIdx += node->m_nTokIdx;
				sStatsParseCalcFreeNode(node);
			}
			else
			{
				sStatsParseCalcAddChildToHead(parent, node);
			}
			return parent;
		}
		sStatsParseCalcAddChild(parent, node);
		return parent;
	}
	ASSERT_RETNULL(node->m_nChildren == 2);
	if (!parent)
	{
		parent = sStatsParseCalcGetNode(STATS_TOKEN_SUM);	
	}
	while (node->m_ChildFirst)
	{
		STATS_CALC * child = sStatsParseCalcDetachNode(node->m_ChildFirst);
		ASSERT_RETNULL(child);
		sStatsParseCalcRearrangeAsSum(parent, child);
	}
	sStatsParseCalcFreeNode(node);
	return parent;
}


//----------------------------------------------------------------------------
// parse a specific stats calc
//----------------------------------------------------------------------------
static STATS_CALC * sStatsParseCalc(
	const char * str)
{
	PARSE_STAT_CALC parse;
	structclear(parse);
	parse.start = str;
	parse.cur = parse.start;
	if (!sStatsParseCalc(parse))
	{
		return FALSE;
	}
	ASSERT_RETFALSE(parse.operand_stack.count == 1 && parse.operator_stack.count == 0);
	STATS_CALC * calc = sStatsParseCalcPopNode(parse.operand_stack);
	ASSERT_RETFALSE(calc);
	return sStatsParseCalcRearrangeAsSum(NULL, calc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsParsePrintToken(
	unsigned int token,
	int tokidx)
{
	switch (token)
	{
	case STATS_TOKEN_NEG:
	case STATS_TOKEN_POS:
	case STATS_TOKEN_TILDE:
	case STATS_TOKEN_NOT:
	case STATS_TOKEN_POWER:
	case STATS_TOKEN_MUL:
	case STATS_TOKEN_DIV:
	case STATS_TOKEN_MOD:
	case STATS_TOKEN_PCT:
	case STATS_TOKEN_PLUSPCT:
	case STATS_TOKEN_ADD:
	case STATS_TOKEN_SUB:
	case STATS_TOKEN_LSHIFT:
	case STATS_TOKEN_RSHIFT:
	case STATS_TOKEN_LT:
	case STATS_TOKEN_GT:
	case STATS_TOKEN_LTE:
	case STATS_TOKEN_GTE:
	case STATS_TOKEN_EQ:
	case STATS_TOKEN_NEQ:
	case STATS_TOKEN_AND:
	case STATS_TOKEN_XOR:
	case STATS_TOKEN_OR:
	case STATS_TOKEN_LAND:
	case STATS_TOKEN_LOR:
	case STATS_TOKEN_SUM:
	case STATS_TOKEN_QUESTION:
	case STATS_TOKEN_LOCAL_DMG_MIN:
	case STATS_TOKEN_LOCAL_DMG_MAX:
	case STATS_TOKEN_SPLASH_DMG_MIN:
	case STATS_TOKEN_SPLASH_DMG_MAX:
	case STATS_TOKEN_THORNS_DMG_MIN:
	case STATS_TOKEN_THORNS_DMG_MAX:
		{
			const char * tokstr = sStatsParseCalcGetTokStr(token);
			ASSERT_RETURN(tokstr);
			trace("%s", tokstr);
		}
		break;
	case STATS_TOKEN_CONSTANT:
		{
			trace("%d", tokidx);
		}
		break;
	case STATS_TOKEN_STAT:
		{
			const STATS_DATA * stats_data = StatsGetData(NULL, tokidx);
			ASSERT_RETURN(stats_data && stats_data->m_szName[0]);
			trace(stats_data->m_szName);
		}
		break;
	case STATS_TOKEN_STAT_PARAM_ZERO:
		{
			const STATS_DATA * stats_data = StatsGetData(NULL, tokidx);
			ASSERT_RETURN(stats_data && stats_data->m_szName[0]);
			trace("%s.0", stats_data->m_szName);
		}
		break;
	case STATS_TOKEN_STAT_PARAM_N:
		{
			const STATS_DATA * stats_data = StatsGetData(NULL, tokidx);
			ASSERT_RETURN(stats_data && stats_data->m_szName[0]);
			trace("%s.N", stats_data->m_szName);
		}
		break;
	case STATS_TOKEN_COLON:
	case STATS_TOKEN_LPAREN:
	case STATS_TOKEN_RPAREN:
	default:
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
// debug output a stats calc
//----------------------------------------------------------------------------
static void sStatsParsePrintCalc1(
	const STATS_CALC * calc)
{
	ASSERT_RETURN(calc);
	if (calc->m_nChildren > 0)
	{
		trace("(");
		sStatsParsePrintToken(calc->m_nToken, calc->m_nTokIdx);
		trace(" ");
		for (STATS_CALC * child = calc->m_ChildFirst; child; child = child->m_Next)
		{
			sStatsParsePrintCalc1(child);
			if (!child->m_Next)
			{
				break;
			}
			trace(" ");
		}
		trace(")");
	}
	else
	{
		sStatsParsePrintToken(calc->m_nToken, calc->m_nTokIdx);
	}
}


//----------------------------------------------------------------------------
// debug output a stats calc
//----------------------------------------------------------------------------
static void sStatsParsePrintCalc(
	const char * cCalcString,
	const STATS_CALC * calc)
{
	ASSERT_RETURN(calc);
	ASSERT_RETURN(cCalcString);
	trace("%s:\n  ", cCalcString);
	sStatsParsePrintCalc1(calc);
	trace("\n");
}


//----------------------------------------------------------------------------
// debug output an affects list
//----------------------------------------------------------------------------
static void sStatsParsePrintAffectsList(
	const STATS_DATA * stats_data)
{
	ASSERT_RETURN(stats_data);
	if (!stats_data->m_nFormulaAffectsList)
	{
		return;
	}
	trace("stat: %s\n", stats_data->m_szName);
	
	const STATS_CALC_AFFECTS * cur = stats_data->m_FormulaAffectsList;
	const STATS_CALC_AFFECTS * end = cur + stats_data->m_nFormulaAffectsList;
	for (; cur < end; ++cur)
	{
		const STATS_DATA * target_data = StatsGetData(NULL, cur->m_nTargetStat);
		ASSERT_BREAK(target_data);
		trace("  affects: %s   subformula: ", target_data->m_szName);
		sStatsParsePrintCalc1(cur->m_Calc);
		trace("\n");
	}
}


//----------------------------------------------------------------------------
// debug output an affects list
//----------------------------------------------------------------------------
static void sStatsParsePrintAffectsList(
	void)
{
	trace("stats affects list debug:\n");
	for (unsigned int ii = 0; ii < (unsigned int)GetNumStats(); ++ii)
	{
		const STATS_DATA * stats_data = StatsGetData(NULL, ii);
		ASSERT_CONTINUE(stats_data);
		sStatsParsePrintAffectsList(stats_data);
	}
	trace("\n");
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelStatsFuncAddFormulaToTarget(
	STATS_DATA * target_data,
	STATS_FUNCTION * statsfunc,
	unsigned int statsfuncidx)
{
	ASSERT_RETURN(target_data);
	ASSERT_RETURN(statsfunc && statsfunc->m_Calc);

	target_data->m_FormulaAffectedByList = (unsigned int *)REALLOCZ(NULL, target_data->m_FormulaAffectedByList, sizeof(int) * (target_data->m_nFormulaAffectedByList + 1));
	target_data->m_FormulaAffectedByList[target_data->m_nFormulaAffectedByList] = statsfuncidx;
	target_data->m_nFormulaAffectedByList++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const STATS_FUNCTION * sStatsFuncGetFormula(
	UNIT * unit,
	const STATS_DATA * target_data,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETNULL(unit);
	GAME * game = UnitGetGame(unit);

	ASSERT_RETNULL(target_data);
	unsigned int * cur = target_data->m_FormulaAffectedByList;
	unsigned int * end = cur + target_data->m_nFormulaAffectedByList;
	for (; cur < end; ++cur)
	{
		const STATS_FUNCTION * func = StatsFuncGetData(game, *cur);
		ASSERT_CONTINUE(func);

		if (func->m_nControlUnitType != INVALID_LINK && !UnitIsA(unit, func->m_nControlUnitType))
		{
			continue;
		}

		/*
		check m_codeControlCode
		*/
		return func;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelStatsFuncAddSubFormulaToTarget(
	const STATS_FUNCTION * statsfunc,
	unsigned int statsfuncidx,
	const STATS_CALC * calc,
	int stat)
{
	STATS_DATA * stats_data = (STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, stat);
	ASSERT_RETURN(stats_data);

	int targetstat = statsfunc->m_nTargetStat;

	STATS_CALC_AFFECTS * cur = stats_data->m_FormulaAffectsList;
	STATS_CALC_AFFECTS * end = stats_data->m_FormulaAffectsList + stats_data->m_nFormulaAffectsList;
	STATS_CALC_AFFECTS * inp = NULL;
	for (; cur < end; ++cur)
	{
		if (cur->m_Calc == calc)
		{
			return;
		}
		if (!inp && cur->m_nTargetStat > targetstat)
		{
			inp = cur;
		}
	}
	unsigned int insertionIndex = (unsigned int)(inp ? (inp - stats_data->m_FormulaAffectsList) : stats_data->m_nFormulaAffectsList);

	// need to insert into list in order of m_nTargetStat
	stats_data->m_FormulaAffectsList = (STATS_CALC_AFFECTS *)REALLOCZ(NULL, stats_data->m_FormulaAffectsList, sizeof(STATS_CALC_AFFECTS) * (stats_data->m_nFormulaAffectsList + 1));
	if (insertionIndex < stats_data->m_nFormulaAffectsList)
	{
		MemoryMove(stats_data->m_FormulaAffectsList + insertionIndex + 1, (stats_data->m_nFormulaAffectsList - insertionIndex) * sizeof(STATS_CALC_AFFECTS), 
			stats_data->m_FormulaAffectsList + insertionIndex, (stats_data->m_nFormulaAffectsList - insertionIndex) * sizeof(STATS_CALC_AFFECTS));
	}
	stats_data->m_FormulaAffectsList[insertionIndex].m_Calc = calc;
	stats_data->m_FormulaAffectsList[insertionIndex].m_nStatsFunc = statsfuncidx;
	stats_data->m_FormulaAffectsList[insertionIndex].m_nTargetStat = statsfunc->m_nTargetStat;
	stats_data->m_FormulaAffectsList[insertionIndex].m_nControlUnitType = statsfunc->m_nControlUnitType;
	stats_data->m_FormulaAffectsList[insertionIndex].m_codeControlCode = statsfunc->m_codeControlCode;
	stats_data->m_nFormulaAffectsList++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelStatsFuncAddIterateFormulaForAffectsList(
	const STATS_FUNCTION * statsfunc,
	unsigned int statsfuncidx,
	const STATS_CALC * subformula,
	const STATS_CALC * calc)
{
	switch (calc->m_nToken)
	{
	case STATS_TOKEN_STAT:
	case STATS_TOKEN_STAT_PARAM_ZERO:
	case STATS_TOKEN_STAT_PARAM_N:
		if (calc->m_nTokIdx != INVALID_LINK)
		{
			sExcelStatsFuncAddSubFormulaToTarget(statsfunc, statsfuncidx, subformula, calc->m_nTokIdx);
		}
	}

	for (const STATS_CALC * cur = calc->m_ChildFirst; cur; cur = cur->m_Next)
	{
		sExcelStatsFuncAddIterateFormulaForAffectsList(statsfunc, statsfuncidx, subformula, cur);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelStatsFuncAddFormulaToAffectsList(
	const STATS_FUNCTION * statsfunc,
	unsigned int statsfuncidx)
{
	ASSERT_RETURN(statsfunc && statsfunc->m_Calc);

	const STATS_CALC * calc = statsfunc->m_Calc;
	if (calc->m_nToken == STATS_TOKEN_SUM)
	{
		ASSERT(calc->m_ChildFirst && calc->m_nChildren > 0);
		for (calc = calc->m_ChildFirst; calc; calc = calc->m_Next)
		{
			sExcelStatsFuncAddIterateFormulaForAffectsList(statsfunc, statsfuncidx, calc, calc);
		}
	}
	else
	{
		sExcelStatsFuncAddIterateFormulaForAffectsList(statsfunc, statsfuncidx, calc, calc);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sExcleStatsFuncParseCheckApp(
	STATS_FUNCTION * statsfunc)
{
	if (statsfunc->m_nApp == STATSFUNC_APP_BOTH)
	{
		return TRUE;
	}
	if (statsfunc->m_nApp == STATSFUNC_APP_HELLGATE)
	{
		return (AppIsHellgate());
	}
	if (statsfunc->m_nApp == STATSFUNC_APP_MYTHOS)
	{
		return (AppIsTugboat());
	}
	return FALSE;
}


//----------------------------------------------------------------------------
// special parse function for stats formulas
//----------------------------------------------------------------------------
static void ExcelStatsFuncParse(
	GAME * game,
	const char * cCalcString,
	unsigned int nCalcStringBufLen,
	STATS_FUNCTION * statsfunc,
	unsigned int nLine)
{
	ASSERT_RETURN(statsfunc);

	if (!sExcleStatsFuncParseCheckApp(statsfunc))
	{
		return;
	}
	ASSERT_RETURN(statsfunc->m_nTargetStat != INVALID_LINK);

	STATS_DATA * target_data = (STATS_DATA *)ExcelGetData(game, DATATABLE_STATS, statsfunc->m_nTargetStat);
	ASSERT_RETURN(target_data);

	statsfunc->m_Calc = sStatsParseCalc(cCalcString);
	ASSERT_RETURN(statsfunc->m_Calc);
	// sStatsParsePrintCalc(cCalcString, statsfunc->m_Calc);

	// attach the calc to the target stat
	sExcelStatsFuncAddFormulaToTarget(target_data, statsfunc, nLine);

	// attach each piece to the individual stats
	sExcelStatsFuncAddFormulaToAffectsList(statsfunc, nLine);

	SETBIT(target_data->m_dwFlags, STATS_DATA_CALC_TARGET);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITSTATS * sStatsGetUnitStats(
	UNIT * unit)
{
	ASSERT_RETNULL(unit);
	UNITSTATS * stats = unit->stats;
	ASSERT_RETNULL(stats->m_dwDebugMagic == STATS_DEBUG_MAGIC);
	ASSERT_RETNULL(StatsTestFlag(stats, STATSFLAG_UNITSTATS));
	return stats;
}


//----------------------------------------------------------------------------
// return index of stats_entry, or insertion point (first larger stat)
//----------------------------------------------------------------------------
#if GLOBAL_STATS_TRACKING
static inline unsigned int sFindStatInList(
	GAME * game,
	const STATS_LIST * list,
	DWORD stat)
#else
static inline unsigned int sFindStatInList(
	const STATS_LIST * list,
	DWORD stat)
#endif
{
	ASSERT(list);

#if GLOBAL_STATS_TRACKING
	GlobalStatsTrackerAdd(game, STAT_GET_STAT(stat));
#endif

	const STATS_ENTRY * min = list->m_Stats;
	const STATS_ENTRY * max = min + list->m_Count;
	const STATS_ENTRY * ii = min + (max - min) / 2;

	while (max > min)
	{
		if (ii->stat > stat)
		{
			max = ii;
		}
		else if (ii->stat < stat)
		{
			min = ii + 1;
		}
		else
		{
			return SIZE_TO_UINT(ii - list->m_Stats);
		}
		ii = min + (max - min) / 2;
	}
	return SIZE_TO_UINT(max - list->m_Stats);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sGetDefaultListValue(
	STATS * deflist,
	DWORD stat)
{
	if (!deflist)
	{
		return 0;
	}
#if GLOBAL_STATS_TRACKING
	unsigned int ii = sFindStatInList(NULL, &deflist->m_List, stat);
#else
	unsigned int ii = sFindStatInList(&deflist->m_List, stat);
#endif
	if (ii >= deflist->m_List.m_Count || deflist->m_List.m_Stats[ii].stat != stat)
	{
		return 0;
	}
	return deflist->m_List.m_Stats[ii].value;
}


//----------------------------------------------------------------------------
// get the value given a stat
//----------------------------------------------------------------------------
#if GLOBAL_STATS_TRACKING
static inline int sGetStatInList(
	GAME * game,
	STATS_LIST * list,
	DWORD stat,
	const STATS_DATA * stats_data,
	STATS * deflist = NULL)
#else
static inline int sGetStatInList(
	STATS_LIST * list,
	DWORD stat,
	const STATS_DATA * stats_data,
	STATS * deflist = NULL)
#endif
{
	ASSERT(list && stats_data);

#if GLOBAL_STATS_TRACKING
	unsigned int ii = sFindStatInList(game, list, stat);
#else
	unsigned int ii = sFindStatInList(list, stat);
#endif
	if (ii >= list->m_Count || list->m_Stats[ii].stat != stat)
	{	
		// not found, check default list (un-normalized)
		return sGetDefaultListValue(deflist, stat) - stats_data->m_nStatOffset;
	}
	
	return list->m_Stats[ii].value - stats_data->m_nStatOffset;		// return the value in the list (undo normalization)
}


//----------------------------------------------------------------------------
// get the value given a stat
//----------------------------------------------------------------------------
#if GLOBAL_STATS_TRACKING
static inline int sGetStatInListNoOffset(
	GAME * game,
	STATS_LIST * list,
	DWORD stat,
	const STATS_DATA * stats_data,
	STATS * deflist = NULL)
#else
static inline int sGetStatInListNoOffset(
	STATS_LIST * list,
	DWORD stat,
	const STATS_DATA * stats_data,
	STATS * deflist = NULL)
#endif
{
	ASSERT(list && stats_data);

#if GLOBAL_STATS_TRACKING
	unsigned int ii = sFindStatInList(game, list, stat);
#else
	unsigned int ii = sFindStatInList(list, stat);
#endif
	if (ii >= list->m_Count || list->m_Stats[ii].stat != stat)
	{	
		// not found, check default list (un-normalized)
		return sGetDefaultListValue(deflist, stat);
	}
	
	return list->m_Stats[ii].value;		// return the value in the list
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sSetOrRemoveStatInList(
	STATS_LIST * list,
	unsigned int ii,
	DWORD stat,
	int value)
{
	if (value != 0)		
	{
		// set the new value if it's non-zero
		list->m_Stats[ii].stat = stat;
		list->m_Stats[ii].value = value;
	}
	else	
	{
		// remove the entry from our list if the value was 0
		if (ii < list->m_Count - 1)
		{
			MemoryMove(list->m_Stats + ii, (list->m_Count - ii) * sizeof(STATS_ENTRY), list->m_Stats + ii + 1, (list->m_Count - ii - 1) * sizeof(STATS_ENTRY));
		}
		list->m_Count--;
	}
}


//----------------------------------------------------------------------------
// insert a stat in a list given insertion point
//----------------------------------------------------------------------------
static inline void sInsertStatInList(
	GAME * game,
	STATS_LIST * list,
	unsigned int ii,
	DWORD stat,
	int value)
{
	unsigned int size = (list->m_Count + 1) * sizeof(STATS_ENTRY);
	list->m_Stats = (STATS_ENTRY *)GREALLOC(game, list->m_Stats, size);
	ASSERT_RETURN(list->m_Stats);
	ASSERT_RETURN(ii * sizeof(STATS_ENTRY) < size);
	if (ii < list->m_Count)
	{
		MemoryMove(list->m_Stats + ii + 1, (list->m_Count - ii) * sizeof(STATS_ENTRY), list->m_Stats + ii, (list->m_Count - ii) * sizeof(STATS_ENTRY));
	}
	list->m_Count++;
	list->m_Stats[ii].stat = stat;
	list->m_Stats[ii].value = value;
}


//----------------------------------------------------------------------------
// set a stat in a statslist, return the delta from the old value
//----------------------------------------------------------------------------
static inline int sSetStatInList(
	GAME * game,
	STATS_LIST * list,
	DWORD stat,
	const STATS_DATA * stats_data,
	int value,
	STATS * deflist = NULL)
{
	ASSERT(list);

	// normalize the default value to 0  (0 values aren't stored in the list to save space)
	value += stats_data->m_nStatOffset;		

#if GLOBAL_STATS_TRACKING
	unsigned int ii = sFindStatInList(game, list, stat);
#else
	unsigned int ii = sFindStatInList(list, stat);
#endif
	if (ii < list->m_Count && list->m_Stats[ii].stat == stat)	// found it in the list
	{
		int delta = value - list->m_Stats[ii].value;
		sSetOrRemoveStatInList(list, ii, stat, value);
		return delta;
	}
	
	// Get the default value
	int defaultvalue = sGetDefaultListValue(deflist, stat);

	// Calculate delta based on default value
	int delta = value - defaultvalue;

	if (value == 0)
	{
		return delta;
	}

	// insert a new stat entry in order
	sInsertStatInList(game, list, ii, stat, value);
	return delta;
}


//----------------------------------------------------------------------------
// call this to set a stat in a unit's list
// RUN_SPEC = TRUE to call special function callbacks
//----------------------------------------------------------------------------
template <BOOL RUN_SPEC>
static void sSetStatInUnitList(
	GAME * game,
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int value,
	BOOL bSend)
{
	int max = 0;
	if (stats_data->m_nMaxStat > INVALID_LINK && (IS_SERVER(game) || !UnitTestFlag(unit, UNITFLAG_IGNORE_STAT_MAX_FOR_XFER)))
	{
		max = UnitGetStat(unit, stats_data->m_nMaxStat, dwParam);
		if (stats_data->m_nMinSet != INT_MIN)
		{
			ASSERT_DO(max >= stats_data->m_nMinSet)
			{
				max = stats_data->m_nMinSet;
			}
		}
		value = MIN(value, max);
	}

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

	if (RUN_SPEC)
	{
		sChangeStatSpecialFunctionPreSet(unit, wStat, dwParam, stats_data);
	}

	int delta = sSetStatInList(game, &unit->stats->m_List, dwStat, stats_data, value, unit->stats->m_Default);

	if (RUN_SPEC)
	{
		sChangeStatSpecialFunctionPostSet(unit, wStat, dwParam, stats_data, value - delta, value, max);
	}

	sAccrue<RUN_SPEC>(game, unit->stats, TRUE, dwStat, stats_data, delta, bSend, max);
}


//----------------------------------------------------------------------------
// pin to constant-value min-max
//----------------------------------------------------------------------------
static inline int sChangeStatInListApplyMinMax(
	int value,
	int max,
	const STATS_DATA * stats_data)
{
	if (max != 0)
	{
		if (value > max)
		{
			value = max;
		}
	}
	if (stats_data->m_nMinSet != INT_MIN)
	{
		if (value < stats_data->m_nMinSet)
		{
			value = stats_data->m_nMinSet;
		}
	}
	if (stats_data->m_nMaxSet != -1)
	{
		if (value > stats_data->m_nMaxSet)
		{
			value = stats_data->m_nMaxSet;
		}
	}
	return value;
}


//----------------------------------------------------------------------------
// pin to constant-value min-max
//----------------------------------------------------------------------------
static inline int sChangeStatInListApplyMinMax(
	GAME * game,
	UNIT * unit,
	int value,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	ASSERT_RETZERO(game);
	ASSERT_RETZERO(unit);
	ASSERT_RETZERO(stats_data);
	int max = 0;
	if (stats_data->m_nMaxStat > INVALID_LINK && (IS_SERVER(game) || !UnitTestFlag(unit, UNITFLAG_JUSTCREATED)))
	{
		max = UnitGetStat(unit, stats_data->m_nMaxStat, dwParam);
	}
	return sChangeStatInListApplyMinMax(value, max, stats_data);
}


//----------------------------------------------------------------------------
// add delta to the stat
//----------------------------------------------------------------------------
static inline void sChangeStatInList(
	GAME * game,
	STATS_LIST * list,
	DWORD stat,
	const STATS_DATA * stats_data,
	int delta,
	int max = 0,
	STATS * deflist = NULL,
	int * pOldValue = NULL,
	int * pNewValue = NULL)
{
	ASSERT(game && list);

	if (delta == 0)		// no change, so fill in old & new value if requested
	{
		if (!(pOldValue || pNewValue))
		{
			return;
		}
#if GLOBAL_STATS_TRACKING
		int value = sGetStatInList(game, list, stat, stats_data, deflist);
#else
		int value = sGetStatInList(list, stat, stats_data, deflist);
#endif
		if (pOldValue)
		{
			*pOldValue = value - stats_data->m_nStatOffset;
		}
		if (pNewValue)
		{
			*pNewValue = value - stats_data->m_nStatOffset;
		}
		return;
	}

#if GLOBAL_STATS_TRACKING
	unsigned int ii = sFindStatInList(game, list, stat);
#else
	unsigned int ii = sFindStatInList(list, stat);
#endif
	if (ii < list->m_Count && list->m_Stats[ii].stat == stat)	// found the stat in our list
	{
		if (pOldValue)
		{
			*pOldValue = list->m_Stats[ii].value - stats_data->m_nStatOffset;
		}
		int value = list->m_Stats[ii].value + delta;
		value = sChangeStatInListApplyMinMax(value, max, stats_data);
		if (pNewValue)
		{
			*pNewValue = value - stats_data->m_nStatOffset;
		}

		sSetOrRemoveStatInList(list, ii, stat, value);
		return;
	}

	int oldvalue = 0;
	if (deflist)	// look for it in the default list
	{
		oldvalue = sGetDefaultListValue(deflist, stat);
	}
	if (pOldValue)
	{
		*pOldValue = oldvalue - stats_data->m_nStatOffset;
	}

	int newvalue = oldvalue + delta;
	newvalue = sChangeStatInListApplyMinMax(newvalue, max, stats_data);
	if (pNewValue)
	{
		*pNewValue = newvalue - stats_data->m_nStatOffset;
	}
	if (newvalue == 0)
	{
		return;
	}	

	sInsertStatInList(game, list, ii, stat, newvalue);
}


//----------------------------------------------------------------------------
// add delta to the stat
//----------------------------------------------------------------------------
template <BOOL RUN_SPEC>
static inline void sChangeStatInUnitList(
	GAME * game,
	UNIT * unit,
	STATS_LIST * list,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int delta,
	int max,
	int & oldvalue,
	int & newvalue)
{
	if (RUN_SPEC)
	{
		sChangeStatSpecialFunctionPreSet(unit, wStat, dwParam, stats_data);
	}

	DWORD dwStat = MAKE_STAT(wStat, dwParam);
	sChangeStatInList(game, list, dwStat, stats_data, delta, max, unit->stats->m_Default, &oldvalue, &newvalue);

	if (RUN_SPEC)
	{
		sChangeStatSpecialFunctionPostSet(unit, wStat, dwParam, stats_data, delta, newvalue, max);
	}
}


//----------------------------------------------------------------------------
// send a stat to the client
//----------------------------------------------------------------------------
static inline void sSendStat(
	GAME * game,
	UNIT * unit,
	DWORD stat,
	const STATS_DATA * stats_data,
	int accrvalue)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(game && IS_SERVER(game));
	ASSERT_RETURN(unit);

	if (!UnitCanSendMessages(unit))
	{
		return;
	}

	UNIT * container = UnitGetUltimateContainer(unit);
	if (!container)
	{
		container = unit;
	}

	if (StatsDataTestFlag(stats_data, STATS_DATA_SEND_TO_ALL))
	{
		MSG_SCMD_UNITSETSTAT msg_setstat;
		msg_setstat.id = UnitGetId(unit);
		msg_setstat.dwStat = stat;
		msg_setstat.nValue = accrvalue;

		s_SendUnitMessage(unit, SCMD_UNITSETSTAT, &msg_setstat);
	}
	else if (StatsDataTestFlag(stats_data, STATS_DATA_SEND_TO_SELF) && UnitHasClient(container))
	{
		MSG_SCMD_UNITSETSTAT msg_setstat;
		msg_setstat.id = UnitGetId(unit);
		msg_setstat.dwStat = stat;
		msg_setstat.nValue = accrvalue;

		s_SendMessage(game, UnitGetClientId(container), SCMD_UNITSETSTAT, &msg_setstat);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sAccrueToType(
	UNIT * unit,
	const STATS_DATA * stats_data)
{
	// accrue to it if no allow types specified
	if (stats_data->m_nAccrueTo[0] == 0)	// any
	{
		return TRUE;
	}

	return IsAllowedUnit(unit, stats_data->m_nAccrueTo, MAX_ACCRUE_TO);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATS_CALC_CONTEXT
{
	GAME *								game;
	UNIT *								unit;
	int									wStat;				// this is the stat that originally changed
	PARAM								dwParam;			// this is the param that for above
	const STATS_DATA *					stats_data;			// this is the data for the wStat
	int									oldaccr;			// old accr value
	int									newaccr;			// new accr value
	int									targetstat;			// this is the target stat for the calc node being processed 
															// (we need to get the accr value for this stat rather than the calc value if it's part of the formula)
	int									objecttype;			// used only for "calc on read" stats, this is a passed in parameter
	STATS *								stats;				// used only for "calc on read" stats, this is often a rider
	BOOL								isMelee;			// used only for "calc on read" stats, this is a passed in parameter

	STATS_CALC_CONTEXT(
		void) : game(NULL), unit(NULL), wStat(-1), dwParam(0), stats_data(NULL), oldaccr(0), newaccr(0), targetstat(-1), objecttype(0), stats(NULL), isMelee(FALSE)
	{
	}

	STATS_CALC_CONTEXT(
		GAME * in_game,
		UNIT * in_unit,
		int in_wStat,
		PARAM in_dwParam,
		const STATS_DATA * in_stats_data,
		int in_oldaccr,
		int in_newaccr,
		int in_targetstat) : game(in_game), unit(in_unit), wStat(in_wStat), dwParam(in_dwParam), stats_data(in_stats_data), 
			oldaccr(in_oldaccr), newaccr(in_newaccr), targetstat(in_targetstat), objecttype(0), stats(NULL), isMelee(FALSE)
	{
	}

	STATS_CALC_CONTEXT(
		GAME * in_game,
		UNIT * in_unit,
		int in_wStat,
		PARAM in_dwParam,
		const STATS_DATA * in_stats_data,
		int in_oldaccr,
		int in_newaccr,
		int in_targetstat,
		int in_objecttype,
		STATS * in_stats,
		BOOL in_isMelee) : game(in_game), unit(in_unit), wStat(in_wStat), dwParam(in_dwParam), stats_data(in_stats_data), 
			oldaccr(in_oldaccr), newaccr(in_newaccr), targetstat(in_targetstat), objecttype(in_objecttype), stats(in_stats), isMelee(in_isMelee)
	{
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sStatsCalc(
	STATS_CALC_CONTEXT & context,
	STATS_CALC * node)
{
	ASSERT_RETZERO(node);

	STATS_CALC * nodeA = NULL;
	int valA = 0;
	STATS_CALC * nodeB = NULL;
	int valB = 0;

	switch (sStatsParseCalcGetOperandCount(node->m_nToken))
	{
	case 1:
		nodeA = sStatsParseCalcGetFirstChild(node);
		ASSERT_RETZERO(nodeA);
		valA = sStatsCalc(context, nodeA);
		break;
	case 2:
		nodeA = sStatsParseCalcGetFirstChild(node);
		ASSERT_RETZERO(nodeA);
		valA = sStatsCalc(context, nodeA);
		nodeB = sStatsParseCalcGetSecondChild(node, nodeA);
		ASSERT_RETZERO(nodeB);
		if (node->m_nToken != STATS_TOKEN_QUESTION)
		{
			valB = sStatsCalc(context, nodeB);
		}
		break;
	}

	switch (node->m_nToken)
	{
	case STATS_TOKEN_NEG:
		return (-valA);
	case STATS_TOKEN_TILDE:
		return (~valA);
	case STATS_TOKEN_NOT:
		return (!valA);
	case STATS_TOKEN_POWER:
		return sStatsCalcOpPower(valA, valB);
	case STATS_TOKEN_MUL:
		return (valA * valB);
	case STATS_TOKEN_DIV:
		ASSERT_RETZERO(valB != 0);
		return (valA / valB);
	case STATS_TOKEN_MOD:
		ASSERT_RETZERO(valB != 0);
		return (valA % valB);
	case STATS_TOKEN_PCT:
		return (valB == 100 ? valA : PCT(valA, valB));
	case STATS_TOKEN_PLUSPCT:
		return (valB == 0 ? valA : PCT(valA, 100 + valB));
	case STATS_TOKEN_ADD:
		return (valA + valB);
	case STATS_TOKEN_SUB:
		return (valA - valB);
	case STATS_TOKEN_LSHIFT:
		return (valB >= 0 ? (valA << valB) : (valA >> valB));
	case STATS_TOKEN_RSHIFT:
		return (valB >= 0 ? (valA >> valB) : (valA << valB));
	case STATS_TOKEN_LT:
		return (valA < valB);
	case STATS_TOKEN_GT:
		return (valA > valB);
	case STATS_TOKEN_LTE:
		return (valA <= valB);
	case STATS_TOKEN_GTE:
		return (valA >= valB);
	case STATS_TOKEN_EQ:
		return (valA == valB);
	case STATS_TOKEN_NEQ:
		return (valA != valB);
	case STATS_TOKEN_AND:
		return (valA & valB);
	case STATS_TOKEN_XOR:
		return (valA ^ valB);
	case STATS_TOKEN_OR:
		return (valA | valB);
	case STATS_TOKEN_LAND:
		return (valA && valB);
	case STATS_TOKEN_LOR:
		return (valA || valB);
	case STATS_TOKEN_QUESTION:
		{
			ASSERT_RETZERO(nodeB->m_nToken == STATS_TOKEN_COLON);
			STATS_CALC * nodeBA = sStatsParseCalcGetFirstChild(nodeB);
			ASSERT_RETZERO(nodeBA);
			STATS_CALC * nodeBB = sStatsParseCalcGetSecondChild(nodeB, nodeBA);
			ASSERT_RETZERO(nodeBB);
			if (valA)
			{
				return sStatsCalc(context, nodeBA);
			}
			else
			{
				return sStatsCalc(context, nodeBB);
			}
		}
	case STATS_TOKEN_CONSTANT:
		return node->m_nTokIdx;
	case STATS_TOKEN_STAT:
		if (node->m_nTokIdx == context.wStat)
		{
			return context.newaccr;
		}
		else
		{
			return UnitGetStat(context.unit, node->m_nTokIdx);
		}
	case STATS_TOKEN_STAT_PARAM_ZERO:
		if (node->m_nTokIdx == context.wStat && context.dwParam == 0)
		{
			return context.newaccr;
		}
		else
		{
			return UnitGetStat(context.unit, node->m_nTokIdx, 0);
		}
	case STATS_TOKEN_STAT_PARAM_N:
		if (node->m_nTokIdx == context.wStat && context.dwParam != 0)
		{
			return context.newaccr;
		}
		else
		{
			return UnitGetStat(context.unit, node->m_nTokIdx, context.dwParam);
		}
	case STATS_TOKEN_LOCAL_DMG_MIN:
		return CombatGetMinDamage(context.game, context.unit, context.objecttype, context.stats, context.dwParam, COMBAT_LOCAL_DELIVERY, context.isMelee);
	case STATS_TOKEN_LOCAL_DMG_MAX:
		return CombatGetMaxDamage(context.game, context.unit, context.objecttype, context.stats, context.dwParam, COMBAT_LOCAL_DELIVERY, context.isMelee);
	case STATS_TOKEN_SPLASH_DMG_MIN:
		return CombatGetMinDamage(context.game, context.unit, context.objecttype, context.stats, context.dwParam, COMBAT_SPLASH_DELIVERY, context.isMelee);
	case STATS_TOKEN_SPLASH_DMG_MAX:
		return CombatGetMaxDamage(context.game, context.unit, context.objecttype, context.stats, context.dwParam, COMBAT_SPLASH_DELIVERY, context.isMelee);
	case STATS_TOKEN_THORNS_DMG_MIN:
		return CombatGetMinThornsDamage(context.game, context.unit, context.stats, context.dwParam);
	case STATS_TOKEN_THORNS_DMG_MAX:
		return CombatGetMaxThornsDamage(context.game, context.unit, context.stats, context.dwParam);
	default:
		ASSERT_RETZERO(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATS_CALC_RESULT
{
	int									oldvalue;
	int									newvalue;

	STATS_CALC_RESULT() : oldvalue(0), newvalue(0)
	{
	}
	STATS_CALC_RESULT(
		int v) : oldvalue(v), newvalue(v)
	{
	}
	STATS_CALC_RESULT(
		int ov,
		int nv) : oldvalue(ov), newvalue(nv)
	{
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCalcChangeStat(
	GAME * game,
	UNITSTATS * stats,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int delta,
	int max = 0);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STATS_CALC_RESULT sStatsCalcChange(
	STATS_CALC_CONTEXT & context,
	const STATS_CALC * node)
{
	ASSERT_RETZERO(node);

	const STATS_CALC * nodeA = NULL;
	STATS_CALC_RESULT valA;
	const STATS_CALC * nodeB = NULL;
	STATS_CALC_RESULT valB;

	switch (sStatsParseCalcGetOperandCount(node->m_nToken))
	{
	case 1:
		nodeA = sStatsParseCalcGetFirstChild(node);
		ASSERT_RETZERO(nodeA);
		valA = sStatsCalcChange(context, nodeA);
		break;
	case 2:
		nodeA = sStatsParseCalcGetFirstChild(node);
		ASSERT_RETZERO(nodeA);
		valA = sStatsCalcChange(context, nodeA);
		nodeB = sStatsParseCalcGetSecondChild(node, nodeA);
		ASSERT_RETZERO(nodeB);
		if (node->m_nToken != STATS_TOKEN_QUESTION)
		{
			valB = sStatsCalcChange(context, nodeB);
		}
		break;
	}

	switch (node->m_nToken)
	{
	case STATS_TOKEN_NEG:
		return STATS_CALC_RESULT(-valA.oldvalue, -valA.newvalue);
	case STATS_TOKEN_TILDE:
		return STATS_CALC_RESULT(~valA.oldvalue, ~valA.newvalue);
	case STATS_TOKEN_NOT:
		return STATS_CALC_RESULT(!valA.oldvalue, !valA.newvalue);
	case STATS_TOKEN_POWER:
		return STATS_CALC_RESULT(sStatsCalcOpPower(valA.oldvalue, valB.oldvalue), sStatsCalcOpPower(valA.newvalue, valB.newvalue));
	case STATS_TOKEN_MUL:
		return STATS_CALC_RESULT(valA.oldvalue * valB.oldvalue, valA.newvalue * valB.newvalue);
	case STATS_TOKEN_DIV:
		ASSERT_RETZERO(valB.oldvalue != 0 && valB.newvalue != 0);
		return STATS_CALC_RESULT(valA.oldvalue / valB.oldvalue, valA.newvalue / valB.newvalue);
	case STATS_TOKEN_MOD:
		ASSERT_RETZERO(valB.oldvalue != 0 && valB.newvalue != 0);
		return STATS_CALC_RESULT(valA.oldvalue % valB.oldvalue, valA.newvalue % valB.newvalue);
	case STATS_TOKEN_PCT:
		return STATS_CALC_RESULT((valB.oldvalue == 100 ? valA.oldvalue : PCT(valA.oldvalue, valB.oldvalue)), 
			(valB.newvalue == 100 ? valA.newvalue : PCT(valA.newvalue, valB.newvalue)));
	case STATS_TOKEN_PLUSPCT:
		return STATS_CALC_RESULT((valB.oldvalue == 0 ? valA.oldvalue : PCT(valA.oldvalue, 100 + valB.oldvalue)), 
			(valB.newvalue == 0 ? valA.newvalue : PCT(valA.newvalue, 100 + valB.newvalue)));
	case STATS_TOKEN_ADD:
		return STATS_CALC_RESULT(valA.oldvalue + valB.oldvalue, valA.newvalue + valB.newvalue);
	case STATS_TOKEN_SUB:
		return STATS_CALC_RESULT(valA.oldvalue - valB.oldvalue, valA.newvalue - valB.newvalue);
	case STATS_TOKEN_LSHIFT:
		return STATS_CALC_RESULT((valB.oldvalue >= 0 ? (valA.oldvalue << valB.oldvalue) : (valA.oldvalue >> valB.oldvalue)), 
			(valB.newvalue >= 0 ? (valA.newvalue << valB.newvalue) : (valA.newvalue >> valB.newvalue)));
	case STATS_TOKEN_RSHIFT:
		return STATS_CALC_RESULT((valB.oldvalue >= 0 ? (valA.oldvalue >> valB.oldvalue) : (valA.oldvalue << valB.oldvalue)), 
			(valB.newvalue >= 0 ? (valA.newvalue >> valB.newvalue) : (valA.newvalue << valB.newvalue)));
	case STATS_TOKEN_LT:
		return STATS_CALC_RESULT(valA.oldvalue < valB.oldvalue, valA.newvalue < valB.newvalue);
	case STATS_TOKEN_GT:
		return STATS_CALC_RESULT(valA.oldvalue > valB.oldvalue, valA.newvalue > valB.newvalue);
	case STATS_TOKEN_LTE:
		return STATS_CALC_RESULT(valA.oldvalue <= valB.oldvalue, valA.newvalue <= valB.newvalue);
	case STATS_TOKEN_GTE:
		return STATS_CALC_RESULT(valA.oldvalue >= valB.oldvalue, valA.newvalue >= valB.newvalue);
	case STATS_TOKEN_EQ:
		return STATS_CALC_RESULT(valA.oldvalue == valB.oldvalue, valA.newvalue == valB.newvalue);
	case STATS_TOKEN_NEQ:
		return STATS_CALC_RESULT(valA.oldvalue != valB.oldvalue, valA.newvalue != valB.newvalue);
	case STATS_TOKEN_AND:
		return STATS_CALC_RESULT(valA.oldvalue & valB.oldvalue, valA.newvalue & valB.newvalue);
	case STATS_TOKEN_XOR:
		return STATS_CALC_RESULT(valA.oldvalue ^ valB.oldvalue, valA.newvalue ^ valB.newvalue);
	case STATS_TOKEN_OR:
		return STATS_CALC_RESULT(valA.oldvalue | valB.oldvalue, valA.newvalue | valB.newvalue);
	case STATS_TOKEN_LAND:
		return STATS_CALC_RESULT(valA.oldvalue && valB.oldvalue, valA.newvalue && valB.newvalue);
	case STATS_TOKEN_LOR:
		return STATS_CALC_RESULT(valA.oldvalue || valB.oldvalue, valA.newvalue || valB.newvalue);
	case STATS_TOKEN_QUESTION:
		{
			ASSERT_RETVAL(nodeB->m_nToken == STATS_TOKEN_COLON, STATS_CALC_RESULT(0, 0));
			const STATS_CALC * nodeBA = sStatsParseCalcGetFirstChild(nodeB);
			ASSERT_RETVAL(nodeBA, STATS_CALC_RESULT(0, 0));
			const STATS_CALC * nodeBB = sStatsParseCalcGetSecondChild(nodeB, nodeBA);
			ASSERT_RETVAL(nodeBB, STATS_CALC_RESULT(0, 0));
			int oldvalue;
			if (valA.oldvalue)
			{
				STATS_CALC_RESULT result = sStatsCalcChange(context, nodeBA);
				oldvalue = result.oldvalue;
			}
			else
			{
				STATS_CALC_RESULT result = sStatsCalcChange(context, nodeBB);
				oldvalue = result.oldvalue;
			}
			int newvalue;
			if (valA.newvalue)
			{
				STATS_CALC_RESULT result = sStatsCalcChange(context, nodeBA);
				newvalue = result.newvalue;
			}
			else
			{
				STATS_CALC_RESULT result = sStatsCalcChange(context, nodeBB);
				newvalue = result.newvalue;
			}
			return STATS_CALC_RESULT(oldvalue, newvalue);
		}
	case STATS_TOKEN_CONSTANT:
		return STATS_CALC_RESULT(node->m_nTokIdx);
	case STATS_TOKEN_STAT:
		if (node->m_nTokIdx == context.wStat)
		{
			return STATS_CALC_RESULT(context.oldaccr, context.newaccr);
		}
		else if (node->m_nTokIdx == context.targetstat)
		{
			int value = UnitGetAccruedStat(context.unit, node->m_nTokIdx);
			return STATS_CALC_RESULT(value, value);
		}
		else
		{
			int value = UnitGetStat(context.unit, node->m_nTokIdx);
			return STATS_CALC_RESULT(value, value);
		}
	case STATS_TOKEN_STAT_PARAM_ZERO:
		if (node->m_nTokIdx == context.wStat && context.dwParam == 0)
		{
			return STATS_CALC_RESULT(context.oldaccr, context.newaccr);
		}
		else if (node->m_nTokIdx == context.targetstat)
		{
			int value = UnitGetAccruedStat(context.unit, node->m_nTokIdx);
			return STATS_CALC_RESULT(value, value);
		}
		else
		{
			int value = UnitGetStat(context.unit, node->m_nTokIdx, 0);
			return STATS_CALC_RESULT(value, value);
		}
	case STATS_TOKEN_STAT_PARAM_N:
		if (node->m_nTokIdx == context.wStat && context.dwParam != 0)
		{
			return STATS_CALC_RESULT(context.oldaccr, context.newaccr);
		}
		else if (node->m_nTokIdx == context.targetstat)
		{
			int value = UnitGetAccruedStat(context.unit, node->m_nTokIdx, context.dwParam);
			return STATS_CALC_RESULT(value, value);
		}
		else
		{
			int value = UnitGetStat(context.unit, node->m_nTokIdx, context.dwParam);
			return STATS_CALC_RESULT(value, value);
		}
	case STATS_TOKEN_LOCAL_DMG_MIN:
	case STATS_TOKEN_LOCAL_DMG_MAX:
	case STATS_TOKEN_SPLASH_DMG_MIN:
	case STATS_TOKEN_SPLASH_DMG_MAX:
	case STATS_TOKEN_THORNS_DMG_MIN:
	case STATS_TOKEN_THORNS_DMG_MAX:
	default:
		ASSERT_RETZERO(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsAccrueDebug(
	GAME * game,
	UNIT * unit,
	DWORD stat,
	const STATS_DATA * stats_data,
	int oldvalue,
	int newvalue)
{
#if ISVERSION(DEVELOPMENT)
	// break this out into a function
	if (stats_data->m_nMinAssert != INT_MIN)
	{
		ASSERTX(newvalue >= stats_data->m_nMinAssert, stats_data->m_szName);
	}
	if (stats_data->m_nMaxAssert != INT_MAX)
	{
		ASSERTX(newvalue <= stats_data->m_nMaxAssert, stats_data->m_szName);
	}

	// trace stat changes to console if set
	if (GameGetDebugFlag(game, DEBUGFLAG_STATS_TRACE) && TESTBIT(game->pdwDebugStatsTrace, STAT_GET_STAT(stat)))
	{
		UNIT * debug_unit = GameGetDebugUnit(game);
		if (debug_unit == unit)
		{
			char szParam[1024];
			szParam[0] = 0;

			for (unsigned int jj = 0; jj < stats_data->m_nParamCount; ++jj)
			{
				unsigned int param = StatGetParam(stats_data, stat, jj);

				char temp[256];
				if (stats_data->m_nParamTable[jj] > INVALID_LINK && param != INVALID_ID)
				{
					PStrPrintf(temp, arrsize(temp), "param%d: %s  ", jj, ExcelGetStringIndexByLine(game, (EXCELTABLE)stats_data->m_nParamTable[jj], param));
				}
				else
				{
					PStrPrintf(temp, arrsize(temp), "param%d: %s  ", jj, param);
				}
				PStrCat(szParam, temp, arrsize(szParam));
			}
			ConsoleString(CONSOLE_SYSTEM_COLOR, "[%c] unit[" ID_PRINTFIELD "]  set stat:%s %s to:%d", 
				(IS_SERVER(game) ? 'S' : 'C'), ID_PRINT(UnitGetId(debug_unit)), stats_data->m_szName, szParam, newvalue);
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStatsCalcChangeCheckIfApplies(
	STATS_CALC_CONTEXT & context,
	STATS_CALC_AFFECTS * affects)
{
	if (affects->m_nControlUnitType > 0 && !UnitIsA(context.unit, affects->m_nControlUnitType))
	{
		return FALSE;
	}
	if (affects->m_codeControlCode != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(context.game, DATATABLE_STATS_FUNC, affects->m_codeControlCode, &codelen);
		if (code)
		{
/*
			if (!VMExecI(context.game, context.unit, context.oldaccr, context.newaccr, code, codelen))
			{
				return FALSE;
			}
*/
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// compute the calc value for a stat that targets itself, return as delta
//----------------------------------------------------------------------------
static int sStatsCalcChangeToSelf(
	STATS_CALC_CONTEXT & context,
	int & oldvalue,
	int & newvalue)
{
	if (context.oldaccr == context.newaccr)
	{
		return 0;
	}

	BOOL bFound = FALSE;
	int delta = 0;

	STATS_CALC_AFFECTS * affects = context.stats_data->m_FormulaAffectsList;
	for (unsigned int ii = 0; ii < context.stats_data->m_nFormulaAffectsList; ++ii, ++affects)
	{
		if (affects->m_nTargetStat != context.wStat)
		{
			continue;
		}
		if (!sStatsCalcChangeCheckIfApplies(context, affects))
		{
			continue;
		}
		STATS_CALC_RESULT result = sStatsCalcChange(context, affects->m_Calc);
		delta += (result.newvalue - result.oldvalue);
		bFound = TRUE;
	}
	if (!bFound)
	{
		delta = newvalue - oldvalue;
	}

	return delta;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sStatsHaveMatchingParams(
	const STATS_DATA *pStatData1,
	const STATS_DATA *pStatData2)
{
	if (pStatData1->m_nParamCount !=pStatData2->m_nParamCount)
		return FALSE;

	for (UINT i=0; i < pStatData1->m_nParamCount; i++)
	{
		if (pStatData1->m_nParamTable[i] !=pStatData2->m_nParamTable[i])
			return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsCalcChangeToOthersChangeStat(
	STATS_CALC_CONTEXT & context,
	UNITSTATS * stats,
	int wStat,
	int delta)
{
	GAME * game = context.game;
	ASSERT_RETURN(game);

	UNIT * unit = context.unit;
	ASSERT_RETURN(unit);

	const STATS_DATA * target_data = StatsGetData(game, wStat);
	ASSERT_RETURN(target_data);

	// need to get param to set
	PARAM dwParam = 0;

	if (sStatsHaveMatchingParams(target_data, context.stats_data))
	{
		dwParam = context.dwParam;
	}

	int max = 0;
	if (target_data->m_nMaxStat > INVALID_LINK && (IS_SERVER(game) || !UnitTestFlag(unit, UNITFLAG_JUSTCREATED)))
	{
		DWORD dwMaxParam = 0;

		const STATS_DATA * pMaxStatData = StatsGetData(game, target_data->m_nMaxStat);
		ASSERT_RETURN(pMaxStatData);

		if (sStatsHaveMatchingParams(pMaxStatData, target_data))
		{	
			dwMaxParam = dwParam;
		}	

		max = UnitGetStat(unit, target_data->m_nMaxStat, dwMaxParam);
	}

	sCalcChangeStat(context.game, stats, wStat, dwParam, target_data, delta, max);
}


//----------------------------------------------------------------------------
// compute the calc value for stats changed by this stat
//----------------------------------------------------------------------------
static void sStatsCalcChangeToOthers(
	STATS_CALC_CONTEXT & context)
{
	if (context.oldaccr == context.newaccr)
	{
		return;
	}

	ASSERT_RETURN(context.unit);
	UNITSTATS * stats = sStatsGetUnitStats(context.unit);
	ASSERT_RETURN(stats);

	STATS_CALC_RESULT result;
	int delta = 0;
	int oldtargetstat = -1;

	STATS_CALC_AFFECTS * cur = context.stats_data->m_FormulaAffectsList;
	STATS_CALC_AFFECTS * end = cur + context.stats_data->m_nFormulaAffectsList;
	for (; cur < end; ++cur)
	{
		if (cur->m_nTargetStat == context.wStat)
		{
			continue;
		}
		if (!sStatsCalcChangeCheckIfApplies(context, cur))
		{
			continue;
		}

		if (oldtargetstat != cur->m_nTargetStat && oldtargetstat != -1 && delta != 0)
		{
			sStatsCalcChangeToOthersChangeStat(context, stats, oldtargetstat, delta);
			delta = 0;
		}
		oldtargetstat = cur->m_nTargetStat;

		int oldContextTargetStat = context.targetstat;
		context.targetstat = cur->m_nTargetStat;
		result = sStatsCalcChange(context, cur->m_Calc);
		context.targetstat = oldContextTargetStat;

		delta += (result.newvalue - result.oldvalue);
	}

	if (delta != 0)
	{
		sStatsCalcChangeToOthersChangeStat(context, stats, oldtargetstat, delta);
		delta = 0;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChangeStatCallbacks(
	GAME * game,
	UNIT * unit,
	UNITSTATS * stats,
	DWORD stat,
	int wStat,
	DWORD dwParam,
	const STATS_DATA * stats_data,
	int oldvalue,
	int newvalue,
	int accrvalue,
	BOOL bSend)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(stats);
	ASSERT_RETURN(stats_data);

	// perform callbacks
	if (!UnitTestFlag(unit, UNITFLAG_JUSTFREED))
	{
		STATS_CALLBACK_TABLE * curr = (IS_SERVER(game) ? stats_data->m_SvrStatsChangeCallback : stats_data->m_CltStatsChangeCallback);
		while (curr)
		{
			STATS_CALLBACK_TABLE * next = curr->m_pNext;
			curr->m_fpCallback(unit, wStat, dwParam, oldvalue, newvalue, curr->m_Data, bSend);
			curr = next;
		}
	}

	if (IS_CLIENT(game))
	{
#if !ISVERSION(SERVER_VERSION)

		// BSP - now we're sending two messages if it's the control unit or the target unit
		//   I think this'll be ok now since there's fast filtering in the UI for messages we don't need
		//   It's worth it right now to keep the separate messages for control stats and target stats

		if (GameGetControlUnit(game) == unit)
		{
			UISendMessage(WM_SETCONTROLSTAT, UnitGetId(unit), stat);
		}
		
		if (UIGetTargetUnitId() == UnitGetId(unit))
		{
			UISendMessage(WM_SETTARGETSTAT, UnitGetId(unit), stat);
		}
			 // vvv BSP - this is checked for now in UIProcessStatMessage
		//else if (UIWantsUnitStatChange(UnitGetId(unit)))
		{
			UISendMessage(WM_SETFOCUSSTAT, UnitGetId(unit), stat);
		}
		// vvv BSP - this was never being hit
		//else if (GameGetControlUnit(game) && UnitGetId(GameGetControlUnit(game)) == UnitGetOwnerId(unit))
		//{
		//	UISendMessage(WM_INVENTORYCHANGE, UnitGetId(GameGetControlUnit(game)), INVLOC_NONE);
		//}
#endif // !ISVERSION(SERVER_VERSION)
	}
	else if (bSend)
	{
		sSendStat(game, unit, stat, stats_data, accrvalue);
		bSend = FALSE;
	}

	if (stats_data->m_nFormulaAffectsList > 0)
	{
		STATS_CALC_CONTEXT context(game, unit, wStat, dwParam, stats_data, oldvalue, newvalue, -1);
		sStatsCalcChangeToOthers(context);
	}
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAccrueChangeStat(
	GAME * game,
	UNITSTATS * stats,
	DWORD stat,
	int wStat,
	DWORD dwParam,
	const STATS_DATA * stats_data,
	int delta,
	int max)
{
	ASSERT_RETURN(game);

	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	ASSERT_RETURN(StatsTestFlag(stats, STATSFLAG_UNITSTATS));

	UNIT * unit = stats->m_Unit;
	ASSERT_RETURN(unit);
	if (UnitTestFlag(unit, UNITFLAG_JUSTFREED))
	{
		return;
	}

	ASSERT_RETURN(stats_data);
	ASSERT_RETURN(stats_data->m_nStatOffset == 0);
	if (StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE_ONCE))
	{
		return;
	}
	if (StatsDataTestFlag(stats_data, STATS_DATA_COMBAT) && !StatsTestFlag(stats, STATSFLAG_COMBAT_STATS_ACCRUED))
	{
		return;
	}

	int oldvalue, newvalue;
	sChangeStatInUnitList<TRUE>(game, unit, &(stats->m_Accr), wStat, dwParam, stats_data, delta, max, oldvalue, newvalue);

	if (StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
	{
		// perform calculation... (because this stat affects itself, therefore, don't do anything based on value until the calc value has been established)
		STATS_CALC_CONTEXT context(game, unit, wStat, dwParam, stats_data, oldvalue, newvalue, wStat);
		delta = sStatsCalcChangeToSelf(context, oldvalue, newvalue);
		sChangeStatInUnitList<TRUE>(game, unit, &(stats->m_Calc), wStat, dwParam, stats_data, delta, max, oldvalue, newvalue);
	}

	sStatsAccrueDebug(game, unit, stat, stats_data, oldvalue, newvalue);
}


//----------------------------------------------------------------------------
// perform the SUM accrual method
//----------------------------------------------------------------------------
static void sAccrueUnitToUnitSum(
	GAME * game, 
	UNITSTATS * cloth,
	DWORD stat,
	const STATS_DATA * stats_data,
	int delta,
	int max)
{
	if (cloth == NULL || delta == 0)
	{
		return;
	}

	int wStat = (int)STAT_GET_STAT(stat);
	PARAM dwParam = (PARAM)STAT_GET_PARAM(stat);

	while (TRUE)
	{
		ASSERT_BREAK(cloth->m_Unit);
		if (!sAccrueToType(cloth->m_Unit, stats_data))
		{
			break;
		}

		sAccrueChangeStat(game, cloth, stat, wStat, dwParam, stats_data, delta, max);

		if ((cloth = cloth->m_Cloth) == NULL)
		{
			break;
		}
	}
}


//----------------------------------------------------------------------------
// accrue a value to cloth unit
//----------------------------------------------------------------------------
static void sAccrueUnitToUnit(
	GAME * game, 
	UNITSTATS * stats,
	DWORD stat,
	const STATS_DATA * stats_data,
	int delta,
	int max = 0)
{
	if (stats == NULL || delta == 0)
	{
		return;
	}
	ASSERT(StatsTestFlag(stats, STATSFLAG_UNITSTATS));

	ASSERT_RETURN((stats_data = sStatsGetStatsData(STAT_GET_STAT(stat), stats_data)) != NULL);

	if (!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE))
	{
		return;
	}
	if (StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE_ONCE))
	{
		return;
	}

	switch (stats_data->m_nAccrualMethod)
	{
	case STATS_ACCRUAL_NONE:
		break;
	case STATS_ACCRUAL_SUM:
		sAccrueUnitToUnitSum(game, stats, stat, stats_data, delta, max);
		break;
	case STATS_ACCRUAL_MAX:
//		sAccrueUnitToUnitMax(game, stats, stat, stats_data, delta, max);
		break;
	case STATS_ACCRUAL_MEAN:
//		sAccrueUnitToUnitMean(game, stats, stat, stats_data, delta, max);
		break;
	default:
		__assume(0);
	}
}


//----------------------------------------------------------------------------
// change a stat in a unitstats' calc list
//----------------------------------------------------------------------------
static void sCalcChangeStat(
	GAME * game,
	UNITSTATS * stats,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int delta,
	int max)  // = 0
{
	if(StatsDataTestFlag(stats_data, STATS_DATA_RECALC_CUR_ON_SET))
	{
		// We just need to get it - we don't want to do anything with it.
		// Getting the stat forces its value to be recalculated, and then we throw away the value
		int curstat = stats_data->m_nAssociatedStat[0];
		UnitGetStat(stats->m_Unit, curstat);
	}
	int oldvalue, newvalue;
	sChangeStatInUnitList<TRUE>(game, stats->m_Unit, &(stats->m_Calc), wStat, dwParam, stats_data, delta, max, oldvalue, newvalue);

	DWORD stat = MAKE_STAT(wStat, dwParam);

	sChangeStatCallbacks(game, stats->m_Unit, stats, stat, wStat, dwParam, stats_data, oldvalue, newvalue, newvalue, FALSE);

	delta = newvalue - oldvalue;
	if (delta != 0)
	{
		sAccrueUnitToUnit(game, stats->m_Cloth, stat, stats_data, delta, max);
	}
}


//----------------------------------------------------------------------------
// perform the SUM accrual method
//----------------------------------------------------------------------------
template <BOOL RUN_SPEC>
static void sAccrueSum(
	GAME * game, 
	UNITSTATS * accr,
	BOOL bSelf,				// bSelf controls if sAccrue checks AccrueToType on the first iteration
	DWORD stat,
	const STATS_DATA * stats_data,
	int delta,
	BOOL bSend,
	int max)
{
	if (accr == NULL || delta == 0)
	{
		return;
	}
	STATS_TEST_MAGIC_RETURN(accr);

	if (!bSelf && StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE_ONCE))
	{
		return;
	}

	int wStat = (int)STAT_GET_STAT(stat);
	PARAM dwParam = (PARAM)STAT_GET_PARAM(stat);

	while (TRUE)
	{
		ASSERT(StatsTestFlag(accr, STATSFLAG_UNITSTATS));

		UNIT * unit = accr->m_Unit;
		ASSERT_BREAK(unit);

		if (!bSelf)
		{
			if (!sAccrueToType(unit, stats_data))
			{
				break;
			}
		}
		ASSERTV(bSelf || stats_data->m_bUnitTypeDebug[UnitGetGenus(unit)], "Trying to accrue stat [%s] to invalid unit [%s]", stats_data->m_szName, UnitGetDevName(unit));
		bSelf = FALSE;

		if(StatsDataTestFlag(stats_data, STATS_DATA_RECALC_CUR_ON_SET))
		{
			// We just need to get it - we don't want to do anything with it.
			// Getting the stat forces its value to be recalculated, and then we throw away the value
			int curstat = stats_data->m_nAssociatedStat[0];
			UnitGetStat(accr->m_Unit, curstat);
		}

		int oldvalue, newvalue;
		sChangeStatInUnitList<RUN_SPEC>(game, unit, &(accr->m_Accr), wStat, dwParam, stats_data, delta, max, oldvalue, newvalue);
		int accrvalue = newvalue;

		if (StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
		{
			// perform calculation... (because this stat affects itself, therefore, don't do anything based on value until the calc value has been established)
			STATS_CALC_CONTEXT context(game, unit, wStat, dwParam, stats_data, oldvalue, newvalue, wStat);
			delta = sStatsCalcChangeToSelf(context, oldvalue, newvalue);
			sChangeStatInUnitList<RUN_SPEC>(game, unit, &(accr->m_Calc), wStat, dwParam, stats_data, delta, max, oldvalue, newvalue);
		}

		sStatsAccrueDebug(game, unit, stat, stats_data, oldvalue, newvalue);

		sChangeStatCallbacks(game, unit, accr, stat, wStat, dwParam, stats_data, oldvalue, newvalue, accrvalue, bSend);

		if (!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE))
		{
			break;
		}
		if (StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE_ONCE))
		{
			break;
		}
		ASSERT(stats_data->m_nMaxStat <= INVALID_LINK);

		// combat flags haven't been applied yet
		if (StatsDataTestFlag(stats_data, STATS_DATA_COMBAT) && !StatsTestFlag(accr, STATSFLAG_COMBAT_STATS_ACCRUED))
		{
			break;
		}

		ASSERT(stats_data->m_nStatOffset == 0);

		accr = accr->m_Cloth;
		if (!accr)
		{
			break;
		}
		STATS_TEST_MAGIC_RETURN(accr);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL RUN_SPEC>
static void sAccrue(
	GAME * game, 
	UNITSTATS * accr,
	BOOL bSelf,				// bSelf controls if sAccrue checks AccrueToType on the first iteration
	DWORD stat,
	const STATS_DATA * stats_data,
	int delta,
	BOOL bSend,
	int max = 0)
{
	if (accr == NULL || delta == 0)
	{
		return;
	}

	ASSERT_RETURN((stats_data = sStatsGetStatsData(STAT_GET_STAT(stat), stats_data)) != NULL);

	switch (stats_data->m_nAccrualMethod)
	{
	case STATS_ACCRUAL_NONE:
		break;
	case STATS_ACCRUAL_SUM:
		sAccrueSum<RUN_SPEC>(game, accr, bSelf, stat, stats_data, delta, bSend, max);
		break;
	case STATS_ACCRUAL_MAX:
//		sAccrueMax(game, accr, bSelf, stat stats_data, delta, bSend, max);
		break;
	case STATS_ACCRUAL_MEAN:
//		sAccrueMean(game, accr, bSelf, stat, stats_data, delta, bSend, max);
		break;
	default:
		__assume(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitCalcStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int objecttype = 0,
	STATS * stats = NULL,
	BOOL isMelee = FALSE);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitCalcStatAccruedSum(
	STATS_CALC_CONTEXT & context)
{
	UNITSTATS * stats = sStatsGetUnitStats(context.unit);
	ASSERT_RETZERO(stats);

	int value = 0;

	BOOL bCombat = StatsDataTestFlag(context.stats_data, STATS_DATA_COMBAT);

	ONCE
	{
		DWORD stat = MAKE_STAT(context.wStat, context.dwParam);

		if (context.stats)
		{
#if GLOBAL_STATS_TRACKING
			value += sGetStatInListNoOffset(context.game, &context.stats->m_List, stat, context.stats_data);
#else
			value += sGetStatInListNoOffset(&context.stats->m_List, stat, context.stats_data);
#endif
		}
#if GLOBAL_STATS_TRACKING
		value += sGetStatInListNoOffset(context.game, &stats->m_List, stat, context.stats_data, stats->m_Default);
#else
		value += sGetStatInListNoOffset(&stats->m_List, stat, context.stats_data, stats->m_Default);
#endif

		if (!StatsDataTestFlag(context.stats_data, STATS_DATA_ACCRUE))
		{
			break;
		}

		if (!sAccrueToType(context.unit, context.stats_data))
		{
			break;
		}

		for (STATS * cur = stats->m_Modlist; cur; cur = cur->m_Next)
		{
#if GLOBAL_STATS_TRACKING
			value += sGetStatInListNoOffset(context.game, &cur->m_List, stat, context.stats_data, NULL);
#else
			value += sGetStatInListNoOffset(&cur->m_List, stat, context.stats_data, NULL);
#endif
		}

		// add the value for dye unit
		if (StatsDataTestFlag(context.stats_data, STATS_DATA_ACCRUE_ONCE))
		{
			break;
		}

		for (UNITSTATS * dye = stats->m_Dye; dye; dye = (UNITSTATS *)dye->m_Next)
		{
			StatsTestFlag(dye, STATSFLAG_UNITSTATS);

			if (bCombat && !StatsTestFlag(dye, STATSFLAG_COMBAT_STATS_ACCRUED))
			{
				continue;
			}

			value += sUnitCalcStat(dye->m_Unit, context.wStat, context.dwParam, context.stats_data) + context.stats_data->m_nStatOffset;
		}
	}

	return value - context.stats_data->m_nStatOffset;
}


//----------------------------------------------------------------------------
// debug function to fully calculate a stat
//----------------------------------------------------------------------------
static inline int sUnitCalcStatAccrued(
	STATS_CALC_CONTEXT & context)
{
	int value = 0;

	ASSERT_RETZERO(context.stats_data);
	ASSERT_RETZERO(context.unit);

	switch (context.stats_data->m_nAccrualMethod)
	{
	case STATS_ACCRUAL_NONE:
		break;
	case STATS_ACCRUAL_SUM:
		value = sUnitCalcStatAccruedSum(context);
		break;
	case STATS_ACCRUAL_MAX:
	case STATS_ACCRUAL_MEAN:
	default:
		__assume(0);
	}

	return sChangeStatInListApplyMinMax(context.game, context.unit, value, context.wStat, context.dwParam, context.stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitCalcStatAccruedHelper(
	STATS_CALC_CONTEXT & context,
	int wStat,
	PARAM dwParam)
{
	if (context.wStat == wStat && context.dwParam == dwParam)
	{
		return sUnitCalcStatAccrued(context);
	}

	STATS_CALC_CONTEXT temp = context;
	context.wStat = wStat;
	context.dwParam = dwParam;
	if (wStat != temp.wStat)
	{
		context.stats_data = StatsGetData(context.game, wStat);
	}
	context.oldaccr = 0;
	context.newaccr = 0;

	int val = sUnitCalcStatAccrued(context);
	context = temp;
	return val;
}


//----------------------------------------------------------------------------
// debug function to fully calculate a stat
//----------------------------------------------------------------------------
static inline int sUnitCalcStat(
	STATS_CALC_CONTEXT & context,
	STATS_CALC * node)
{
	STATS_CALC * nodeA = NULL;
	int valA = 0;
	STATS_CALC * nodeB = NULL;
	int valB = 0;

	switch (sStatsParseCalcGetOperandCount(node->m_nToken))
	{
	case 1:
		nodeA = sStatsParseCalcGetFirstChild(node);
		ASSERT_RETZERO(nodeA);
		valA = sUnitCalcStat(context, nodeA);
		break;
	case 2:
		nodeA = sStatsParseCalcGetFirstChild(node);
		ASSERT_RETZERO(nodeA);
		valA = sUnitCalcStat(context, nodeA);
		nodeB = sStatsParseCalcGetSecondChild(node, nodeA);
		ASSERT_RETZERO(nodeB);
		if (node->m_nToken != STATS_TOKEN_QUESTION)
		{
			valB = sUnitCalcStat(context, nodeB);
		}
		break;
	}

	switch (node->m_nToken)
	{
	case STATS_TOKEN_NEG:
		return (-valA);
	case STATS_TOKEN_TILDE:
		return (~valA);
	case STATS_TOKEN_NOT:
		return (!valA);
	case STATS_TOKEN_POWER:
		return sStatsCalcOpPower(valA, valB);
	case STATS_TOKEN_MUL:
		return (valA * valB);
	case STATS_TOKEN_DIV:
		ASSERT_RETZERO(valB != 0);
		return (valA / valB);
	case STATS_TOKEN_MOD:
		ASSERT_RETZERO(valB != 0);
		return (valA % valB);
	case STATS_TOKEN_PCT:
		return (valB == 100 ? valA : PCT(valA, valB));
	case STATS_TOKEN_PLUSPCT:
		return (valB == 0 ? valA : PCT(valA, 100 + valB));
	case STATS_TOKEN_ADD:
		return (valA + valB);
	case STATS_TOKEN_SUB:
		return (valA - valB);
	case STATS_TOKEN_LSHIFT:
		return (valB >= 0 ? (valA << valB) : (valA >> valB));
	case STATS_TOKEN_RSHIFT:
		return (valB >= 0 ? (valA >> valB) : (valA << valB));
	case STATS_TOKEN_LT:
		return (valA < valB);
	case STATS_TOKEN_GT:
		return (valA > valB);
	case STATS_TOKEN_LTE:
		return (valA <= valB);
	case STATS_TOKEN_GTE:
		return (valA >= valB);
	case STATS_TOKEN_EQ:
		return (valA == valB);
	case STATS_TOKEN_NEQ:
		return (valA != valB);
	case STATS_TOKEN_AND:
		return (valA & valB);
	case STATS_TOKEN_XOR:
		return (valA ^ valB);
	case STATS_TOKEN_OR:
		return (valA | valB);
	case STATS_TOKEN_LAND:
		return (valA && valB);
	case STATS_TOKEN_LOR:
		return (valA || valB);
	case STATS_TOKEN_QUESTION:
		{
			ASSERT_RETZERO(nodeB->m_nToken == STATS_TOKEN_COLON);
			STATS_CALC * nodeBA = sStatsParseCalcGetFirstChild(nodeB);
			ASSERT_RETZERO(nodeBA);
			STATS_CALC * nodeBB = sStatsParseCalcGetSecondChild(nodeB, nodeBA);
			ASSERT_RETZERO(nodeBB);
			if (valA)
			{
				return sUnitCalcStat(context, nodeBA);
			}
			else
			{
				return sUnitCalcStat(context, nodeBB);
			}
		}
	case STATS_TOKEN_CONSTANT:
		return node->m_nTokIdx;
	case STATS_TOKEN_STAT:
		if (node->m_nTokIdx == context.wStat || node->m_nTokIdx == context.targetstat)
		{
			return sUnitCalcStatAccruedHelper(context, node->m_nTokIdx, context.dwParam);
		}
		else
		{
			return sUnitCalcStat(context.unit, node->m_nTokIdx, context.dwParam, NULL, context.objecttype, context.stats, context.isMelee);
		}
	case STATS_TOKEN_STAT_PARAM_ZERO:
		if (node->m_nTokIdx == context.wStat || node->m_nTokIdx == context.targetstat)
		{
			return sUnitCalcStatAccruedHelper(context, node->m_nTokIdx, 0);
		}
		else
		{
			return sUnitCalcStat(context.unit, node->m_nTokIdx, 0, NULL, context.objecttype, context.stats, context.isMelee);
		}
	case STATS_TOKEN_STAT_PARAM_N:
		if (node->m_nTokIdx == context.wStat || node->m_nTokIdx == context.targetstat)
		{
			return sUnitCalcStatAccruedHelper(context, node->m_nTokIdx, context.dwParam);
		}
		else
		{
			return sUnitCalcStat(context.unit, node->m_nTokIdx, context.dwParam, NULL, context.objecttype, context.stats, context.isMelee);
		}
	case STATS_TOKEN_SUM:
		{
			int value = 0;
			for (STATS_CALC * child = sStatsParseCalcGetFirstChild(node); child; child = child->m_Next)
			{
				value += sUnitCalcStat(context, child);
			}
			return value;
		}
	case STATS_TOKEN_LOCAL_DMG_MIN:
		return CombatGetMinDamage(context.game, context.unit, context.objecttype, context.stats, context.dwParam, COMBAT_LOCAL_DELIVERY, context.isMelee);
	case STATS_TOKEN_LOCAL_DMG_MAX:
		return CombatGetMaxDamage(context.game, context.unit, context.objecttype, context.stats, context.dwParam, COMBAT_LOCAL_DELIVERY, context.isMelee);
	case STATS_TOKEN_SPLASH_DMG_MIN:
		return CombatGetMinDamage(context.game, context.unit, context.objecttype, context.stats, context.dwParam, COMBAT_SPLASH_DELIVERY, context.isMelee);
	case STATS_TOKEN_SPLASH_DMG_MAX:
		return CombatGetMaxDamage(context.game, context.unit, context.objecttype, context.stats, context.dwParam, COMBAT_SPLASH_DELIVERY, context.isMelee);
	case STATS_TOKEN_THORNS_DMG_MIN:
		return CombatGetMinThornsDamage(context.game, context.unit, context.stats, context.dwParam);
	case STATS_TOKEN_THORNS_DMG_MAX:
		return CombatGetMaxThornsDamage(context.game, context.unit, context.stats, context.dwParam);
	default:
		ASSERT_RETZERO(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitCalcStatGetValueOrDefault(
	UNIT * unit,
	int value,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	if (value + stats_data->m_nStatOffset == 0 && unit->stats->m_Default)
	{
		return sGetDefaultListValue(unit->stats->m_Default, MAKE_STAT(wStat, dwParam)) - stats_data->m_nStatOffset;
	}
	return value;
}


//----------------------------------------------------------------------------
// debug function to fully calculate a stat
//----------------------------------------------------------------------------
static inline int sUnitCalcStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int objecttype,		// = 0
	STATS * stats,		// = NULL
	BOOL isMelee)		// = FALSE
{
	ASSERT_RETZERO(unit);
	ASSERT_RETZERO((stats_data = sStatsGetStatsData(wStat, stats_data)) != NULL);

	STATS_CALC_CONTEXT context(UnitGetGame(unit), unit, wStat, dwParam, stats_data, 0, 0, -1, objecttype, stats, isMelee);

	const STATS_FUNCTION * statsfunc = sStatsFuncGetFormula(unit, stats_data, wStat, dwParam);
	if (!statsfunc)
	{
		int value =  sUnitCalcStatAccrued(context);
		return sUnitCalcStatGetValueOrDefault(unit, value, wStat, dwParam, stats_data);
	}

	ASSERT_RETZERO(statsfunc->m_Calc);

	int value = sUnitCalcStat(context, statsfunc->m_Calc);
	return sUnitCalcStatGetValueOrDefault(unit, value, wStat, dwParam, stats_data);
}


//----------------------------------------------------------------------------
// allocate and initialize a statslist
//----------------------------------------------------------------------------
STATS * StatsListInit(
	GAME * game)
{
	STATS * stats = (STATS *)GMALLOCZ(game, sizeof(STATS));
	STATS_SET_MAGIC(stats);
#if GLOBAL_STATS_TRACKING
	stats->m_Game = game;
#endif
	// game doesn't exist when reading excel tables, so this check is required
	if (game)
	{
		ASSERT(game->idStats < 0x7FFFFFFE);
		if (game->idStats >= 0x7FFFFFFE)
		{
			game->idStats = 0;
		}
		stats->m_idStats = ++game->idStats;
		if (IS_CLIENT(game))
		{
			stats->m_idStats |= 0x80000000;
		}
		else
		{
			ASSERT(game->idStats < 0x7FFFFFFE);
		}
	}
	sStatsSetFlag(stats, STATSFLAG_STATSLIST);

	return stats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StatsIsRider(
	const STATS * stats)
{
	ASSERT_RETFALSE(stats);
	return StatsTestFlag(stats, STATSFLAG_RIDER);
}


//----------------------------------------------------------------------------
// get the "head" rider for a rider
//----------------------------------------------------------------------------
static inline STATS * sStatsGetHeadRider(
	STATS * stats)
{
	ASSERT_RETNULL(!StatsTestFlag(stats, STATSFLAG_UNITSTATS));
	if (StatsTestFlag(stats, STATSFLAG_RIDER))
	{
		return stats->m_RiderParent;
	}
	return stats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _DEBUG
static BOOL sStatsValidateXor(
	const STATS * a,
	const STATS * b)
{
	return (a && b || (!a && !b));
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _DEBUG
static const STATS * sStatsListValidateFindRider(
	const STATS * stats,
	const STATS * rider)
{
	const STATS * cur = stats->m_FirstRider;
	for (; cur; cur = cur->m_Next)
	{
		if (cur == rider)
		{
			return rider;
		}
	}
	return NULL;
}
#endif


//----------------------------------------------------------------------------
// validate the riders for a unit's statslist (both directions)
//----------------------------------------------------------------------------
#ifdef _DEBUG
static void sStatsListValidateRiderChainDye(
	const UNITSTATS * stats)
{
	// iterate all dyes, check that their riders are applied or not, as appropriate
	if (!stats)
	{
		return;
	}
	ASSERT(sStatsValidateXor(stats->m_FirstRider, stats->m_LastRider));

	const STATS * dye = stats->m_Dye;
	for (; dye; dye = dye->m_Next)
	{
		if (StatsTestFlag(dye, STATSFLAG_UNITSTATS))
		{
			sStatsListValidateRiderChainDye((UNITSTATS *)dye);
			if (!StatsTestFlag(stats, STATSFLAG_COMBAT_STATS_ACCRUED))
			{
				continue;
			}
		}
		// validate my rider list
		ASSERT(sStatsValidateXor(dye->m_FirstRider, dye->m_LastRider));
		const STATS * firstRider = sStatsListValidateFindRider(stats, dye->m_FirstRider);
		ASSERT(firstRider == dye->m_FirstRider);
		const STATS * cur = firstRider;
		for (; cur != dye->m_LastRider; cur = cur->m_Next)
		{
			ASSERT(cur);
			ASSERT(cur->m_RiderParent == dye);
		}
	}
}
#endif


//----------------------------------------------------------------------------
// validate the riders for a unit's statslist (both directions)
//----------------------------------------------------------------------------
static void sStatsListValidateRiderChain(
	const UNITSTATS * stats)
{
#ifdef _DEBUG
	while (stats)
	{
		if (!StatsTestFlag(stats, STATSFLAG_COMBAT_STATS_ACCRUED))
		{
			stats = stats->m_Cloth;
			break;
		}
		stats = stats->m_Cloth;
	}
	if (!stats)
	{
		return;
	}
	sStatsListValidateRiderChainDye(stats);
#endif // _DEBUG
}


//----------------------------------------------------------------------------
// allocate & add a rider to the given statslist
//----------------------------------------------------------------------------
STATS * StatsListAddRider(
	GAME * game,
	STATS * stats,
	int selector)
{
	ASSERT_RETNULL(stats);
	STATS_TEST_MAGIC_RETVAL(stats, NULL);
	if (!StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		stats = sStatsGetHeadRider(stats);
		ASSERT_RETNULL(StatsTestFlag(stats, STATSFLAG_STATSLIST));
	}
	ASSERT_RETNULL(!StatsTestFlag(stats, STATSFLAG_RIDER));

	STATS * rider = StatsListInit(game);
	ASSERT_RETNULL(rider);
	sStatsSetFlag(rider, STATSFLAG_RIDER);
	rider->m_Selector = selector;

	rider->m_RiderParent = stats;

	// "insert" into all cloths (really just updating first/last rider)
	UNITSTATS * cloth = stats->m_Cloth;

	while (cloth)
	{
		sStatsListValidateRiderChain(cloth);

		if (!StatsTestFlag(cloth, STATSFLAG_COMBAT_STATS_ACCRUED))
		{
			break;
		}

		if (!cloth->m_FirstRider)
		{
			ASSERT(!cloth->m_LastRider);
			cloth->m_FirstRider = cloth->m_LastRider = rider;
		}
		else if (cloth->m_LastRider == stats->m_LastRider)
		{
			cloth->m_LastRider = rider;
		}
		ASSERT(cloth->m_FirstRider);
		ASSERT(cloth->m_LastRider);
		sStatsListValidateRiderChain(cloth);
		if (!StatsTestFlag(cloth, STATSFLAG_COMBAT_STATS_ACCRUED))
		{
			break;
		}
		cloth = cloth->m_Cloth;
	}

	// append to rider list
	if (stats->m_LastRider)	
	{
		STATS * lastrider = stats->m_LastRider;
		ASSERT(!lastrider->m_Next);
		rider->m_Prev = lastrider;
		lastrider->m_Next = rider;
	}

	// append to modlist
	if (!stats->m_FirstRider)
	{
		ASSERT(!stats->m_LastRider);
		stats->m_FirstRider = rider;
	}
	stats->m_LastRider = rider;
	
	return rider;
}


//----------------------------------------------------------------------------
// remove riders list from a unit
//----------------------------------------------------------------------------
static inline void sStatsListRemoveRidersFromCloths(
	UNITSTATS * cloth,
	STATS * firstrider,
	STATS * lastrider)
{
	ASSERT_RETURN(firstrider && lastrider);
	while (cloth)
	{
		if (cloth->m_FirstRider == firstrider)
		{
			cloth->m_FirstRider = lastrider->m_Next;
		}
		if (cloth->m_LastRider == lastrider)
		{
			cloth->m_LastRider = firstrider->m_Prev;
		}
		cloth = cloth->m_Cloth;
	}
	if (firstrider->m_Prev)
	{
		firstrider->m_Prev->m_Next = lastrider->m_Next;
	}
	if (lastrider->m_Next)
	{
		lastrider->m_Next->m_Prev = firstrider->m_Prev;
	}
	firstrider->m_Prev = NULL;
	lastrider->m_Next = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STATS * StatsGetRider(
	const STATS * stats,
	const STATS * rider)
{
	ASSERT_RETNULL(stats);
	STATS_TEST_MAGIC_RETVAL(stats, NULL);
	ASSERT(StatsTestFlag(stats, STATSFLAG_STATSLIST));
	ASSERT(!StatsTestFlag(stats, STATSFLAG_RIDER));
	if (!rider)
	{
		return stats->m_FirstRider;
	}
	else
	{
		if (rider == stats->m_LastRider)
		{
			return NULL;
		}
		ASSERT(rider->m_RiderParent == stats);
		return rider->m_Next;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STATS * UnitGetRider(
	UNIT * unit,
	STATS * rider)
{
	const STATS * stats = UnitGetStats(unit);
	ASSERT_RETNULL(stats);
	STATS_TEST_MAGIC_RETVAL(stats, NULL);
	ASSERT_RETNULL(StatsTestFlag(stats, STATSFLAG_UNITSTATS));
	ASSERT_RETNULL(!StatsTestFlag(stats, STATSFLAG_RIDER));
	if (!rider)
	{
		return stats->m_FirstRider;
	}
	else
	{
		ASSERT_RETNULL(StatsTestFlag(rider, STATSFLAG_RIDER));
		if (rider == stats->m_LastRider)
		{
			return NULL;
		}
		rider = rider->m_Next;
		if (rider)
		{
			ASSERT_RETNULL(StatsTestFlag(rider, STATSFLAG_RIDER));
		}
		return rider;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitCopyRidersToUnit(
	UNIT * source,
	UNIT * target,
	int selector)
{
	ASSERT_RETURN(source && target && source != target);
	GAME * game = UnitGetGame(source);

	STATS * target_stats = UnitGetStats(target);
	ASSERT_RETURN(target_stats);
	STATS_TEST_MAGIC_RETURN(target_stats);

	STATS * rider = UnitGetRider(source, NULL);
	while (rider)
	{
		if (rider->m_Selector == selector)
		{
			STATS * newrider = StatsListAddRider(game, target_stats, selector);
			
			int size = rider->m_List.m_Count * sizeof(STATS_ENTRY);
			newrider->m_List.m_Stats = (STATS_ENTRY *)GREALLOC(game, newrider->m_List.m_Stats, size);
			newrider->m_List.m_Count = rider->m_List.m_Count;
			MemoryCopy(newrider->m_List.m_Stats, size, rider->m_List.m_Stats, size);
		}
		rider = UnitGetRider(source, rider);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <int MULT, BOOL IS_ACCR, BOOL SEND>
static void sStatsListAddAccr(
	STATS_LIST * list,
	UNIT * cloth,
	BOOL bCombat)
{
	ASSERT_RETURN(list);
	ASSERT_RETURN(cloth);

	GAME * game = UnitGetGame(cloth);

	STATS_ENTRY * cur = list->m_Stats;
	STATS_ENTRY * end = cur + list->m_Count;
	for (; cur < end; ++cur)
	{
		const STATS_DATA * stats_data = StatsGetData(game, cur->GetStat());
		ASSERT_CONTINUE(stats_data);
		if (!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE))
		{
			continue;
		}
		if (IS_ACCR && StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
		{
			continue;
		}
		if (!bCombat && StatsDataTestFlag(stats_data, STATS_DATA_COMBAT))
		{
			continue;
		}
		ASSERT_CONTINUE(stats_data->m_nMaxStat <= INVALID_LINK);
		sAccrue<TRUE>(game, cloth->stats, FALSE, cur->stat, stats_data, MULT * cur->value, SEND);
	}
}


//----------------------------------------------------------------------------
// remove a unit from whatever unit it's attached to
//----------------------------------------------------------------------------
void StatsListRemove(
	UNIT * dye)
{
	ASSERT_RETURN(dye);

	UNITSTATS * dye_stats = dye->stats;
	ASSERT_RETURN(dye_stats && StatsTestFlag(dye_stats, STATSFLAG_UNITSTATS));
	STATS_TEST_MAGIC_RETURN(dye_stats);

	if (!dye_stats->m_Cloth)
	{
		return;
	}

	ASSERT_RETURN(StatsTestFlag(dye_stats->m_Cloth, STATSFLAG_UNITSTATS));

	UNIT * cloth = dye_stats->m_Cloth->m_Unit;
	ASSERT_RETURN(cloth);

	BOOL bCombat = StatsTestFlag(dye_stats, STATSFLAG_COMBAT_STATS_ACCRUED);

	// remove all riders with the dye
	if (dye_stats->m_FirstRider)
	{
		ASSERT(dye_stats->m_LastRider);
		sStatsListValidateRiderChain(dye_stats);
		sStatsListRemoveRidersFromCloths(dye_stats->m_Cloth, dye_stats->m_FirstRider, dye_stats->m_LastRider);
		sStatsListValidateRiderChain(dye_stats);
	}

	// fix up links
	if (dye_stats->m_Prev)
	{
		dye_stats->m_Prev->m_Next = dye_stats->m_Next;
	}
	if (dye_stats->m_Next)
	{
		dye_stats->m_Next->m_Prev = dye_stats->m_Prev;
	}
	if (dye_stats->m_Cloth->m_Dye == dye_stats)
	{
		dye_stats->m_Cloth->m_Dye = (UNITSTATS *)dye_stats->m_Next;
	}
	dye_stats->m_Cloth = NULL;
	dye_stats->m_Prev = NULL;
	dye_stats->m_Next = NULL;

	// subtract this unit's stats from its' cloth's stats
	sStatsListAddAccr<-1, TRUE, FALSE>(&dye_stats->m_Accr, cloth, bCombat);
	sStatsListAddAccr<-1, FALSE, FALSE>(&dye_stats->m_Calc, cloth, bCombat);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsListRemove(
	GAME * game,
	STATS * dye)
{
	ASSERT_RETURN(dye);
	STATS_TEST_MAGIC_RETURN(dye);

	UNITSTATS * cloth = dye->m_Cloth;
	if (!cloth)
	{
		return;
	}

	ASSERT_RETURN(!StatsTestFlag(dye, STATSFLAG_RIDER));

	// remove all riders with the dye
	if (dye->m_FirstRider)
	{
		sStatsListValidateRiderChain(cloth);
		sStatsListRemoveRidersFromCloths(cloth, dye->m_FirstRider, dye->m_LastRider);
		sStatsListValidateRiderChain(cloth);
	}

	// unlink list
	if (dye->m_Prev)
	{
		dye->m_Prev->m_Next = dye->m_Next;
	}
	if (dye->m_Next)
	{
		dye->m_Next->m_Prev = dye->m_Prev;
	}
	if (cloth->m_Modlist == dye)
	{
		cloth->m_Modlist = dye->m_Next;
	}

	UNIT * unit = dye->m_Cloth->m_Unit;

	dye->m_Cloth = NULL;
	dye->m_Prev = NULL;
	dye->m_Next = NULL;
	dye->m_Selector = 0;

	BOOL bSend = StatsTestFlag(dye, STATSFLAG_SENT_ON_ADD);

	// subtract the stats from accrued
	STATS_ENTRY * cur = dye->m_List.m_Stats;
	STATS_ENTRY * end = cur + dye->m_List.m_Count;
	for (; cur < end; ++cur)
	{
		const STATS_DATA * stats_data = StatsGetData(game, cur->GetStat());
		if (!stats_data)
		{
			continue;
		}
		if (!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE))
		{
			continue;
		}
		ASSERT(stats_data->m_nMaxStat <= INVALID_LINK);
		sAccrue<TRUE>(game, cloth, FALSE, cur->stat, stats_data, -cur->value, bSend);
	}

	sStatsClearFlag(dye, STATSFLAG_SENT_ON_ADD);

	if (dye->m_fpRemoveCallback)
	{
		dye->m_fpRemoveCallback(game, unit, dye);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sStatsListAddRidersToCloths(
	UNITSTATS * cloth,
	STATS * firstrider,
	STATS * lastrider)
{
#ifdef _DEBUG
	ASSERT_RETURN(StatsTestFlag(firstrider, STATSFLAG_RIDER));
	ASSERT_RETURN(StatsTestFlag(lastrider, STATSFLAG_RIDER));
	ASSERT_RETURN(lastrider->m_Next == NULL);

	STATS * prev = NULL;
	STATS * cur = firstrider;
	while (cur)
	{
		prev = cur;
		cur = cur->m_Next;
	}
	ASSERT_RETURN(prev == lastrider);
#endif
	while (cloth)
	{
		BOOL bCombatStatsAccrued = StatsTestFlag(cloth, STATSFLAG_COMBAT_STATS_ACCRUED);

		if (bCombatStatsAccrued && cloth->m_Cloth)	// intermediary cloth, just update firstrider & lastrider
		{
			if (!cloth->m_FirstRider)
			{
				ASSERT(!cloth->m_LastRider);
				cloth->m_FirstRider = firstrider;
			}
			else
			{
				ASSERT(cloth->m_LastRider);
			}
		}
		else	// ultimate cloth, actually append the riderlist
		{
			if (cloth->m_LastRider)
			{
				ASSERT(cloth->m_FirstRider);
				cloth->m_LastRider->m_Next = firstrider;
			}
			else
			{
				ASSERT(!cloth->m_FirstRider);
				cloth->m_FirstRider = firstrider;
			}
			firstrider->m_Prev = cloth->m_LastRider;
		}
		cloth->m_LastRider = lastrider;

		if (!bCombatStatsAccrued)
		{
			break;
		}
		cloth = cloth->m_Cloth;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsListAdd(
	STATS_LIST * list,
	UNIT * cloth,
	BOOL bSend)
{
	ASSERT_RETURN(list);
	ASSERT_RETURN(cloth);

	GAME * game = UnitGetGame(cloth);

	STATS_ENTRY * cur = list->m_Stats;
	STATS_ENTRY * end = cur + list->m_Count;
	for (; cur < end; ++cur)
	{
		const STATS_DATA * stats_data = StatsGetData(game, cur->GetStat());
		ASSERT_CONTINUE(stats_data);
		if (!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE))
		{
			continue;
		}
		ASSERT_CONTINUE(stats_data->m_nMaxStat <= INVALID_LINK);
		sAccrue<TRUE>(game, cloth->stats, FALSE, cur->stat, stats_data, cur->value, bSend);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsListAdd(
	UNIT * cloth,
	STATS * dye,
	BOOL bSend,
	int selector,
	int filter)
{
	ASSERT_RETURN(cloth && cloth->stats);
	STATS_TEST_MAGIC_RETURN(cloth->stats);
	ASSERT_RETURN(dye && !StatsTestFlag(dye, STATSFLAG_UNITSTATS));
	STATS_TEST_MAGIC_RETURN(dye);

	StatsListRemove(UnitGetGame(cloth), dye);
	
	// move all the riders to the cloths
	if (dye->m_FirstRider)
	{
		ASSERT_RETURN(dye->m_LastRider);
		sStatsListAddRidersToCloths(cloth->stats, dye->m_FirstRider, dye->m_LastRider);
	}

	dye->m_Cloth = cloth->stats;
	dye->m_Prev = NULL;
	dye->m_Next = cloth->stats->m_Modlist;
	if (dye->m_Next)
	{
		dye->m_Next->m_Prev = dye;
	}
	cloth->stats->m_Modlist = dye;
	dye->m_Selector = selector;

	// add the stats to accrued
	if (!StatsTestFlag(dye, STATSFLAG_RIDER))
	{
		if (bSend)
		{
			sStatsSetFlag(dye, STATSFLAG_SENT_ON_ADD);
		}
		sStatsListAdd(&dye->m_List, cloth, bSend);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsListAdd(
	UNIT * cloth,
	UNIT * dye,
	BOOL bCombat,
	int filter)
{
	ASSERT_RETURN(cloth && cloth->stats);
	STATS_TEST_MAGIC_RETURN(cloth->stats);
	ASSERT_RETURN(dye && dye->stats);
	STATS_TEST_MAGIC_RETURN(dye->stats);

	GAME * game = UnitGetGame(cloth);
	ASSERT_RETURN(game);

	StatsListRemove(dye);

	UNITSTATS * dye_stats = dye->stats;

	// move all the riders to the ultimate cloth
	if (bCombat && dye_stats->m_FirstRider)
	{
		ASSERT(dye_stats->m_LastRider);
		sStatsListAddRidersToCloths(cloth->stats, dye_stats->m_FirstRider, dye_stats->m_LastRider);
	}

	dye_stats->m_Cloth = cloth->stats;
	dye_stats->m_Prev = NULL;
	dye_stats->m_Next = cloth->stats->m_Dye;
	if (dye_stats->m_Next)
	{
		dye_stats->m_Next->m_Prev = dye_stats;
	}
	cloth->stats->m_Dye = dye_stats;

	sStatsSetFlag(dye_stats, STATSFLAG_COMBAT_STATS_ACCRUED, bCombat);

	// add the stats to accrued
	sStatsListAddAccr<1, TRUE, FALSE>(&dye_stats->m_Accr, cloth, bCombat);
	sStatsListAddAccr<1, FALSE, FALSE>(&dye_stats->m_Calc, cloth, bCombat);

	/*
		todo: modify above to not add accr in some cases
	*/
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <int MULT, BOOL IS_ACCR>
static void sStatsListRemoveWeaponStats(
	STATS_LIST * list,
	UNIT * cloth)
{
	ASSERT_RETURN(list);
	ASSERT_RETURN(cloth);

	GAME * game = UnitGetGame(cloth);

	STATS_ENTRY * cur = list->m_Stats;
	STATS_ENTRY * end = cur + list->m_Count;
	for (; cur < end; ++cur)
	{
		const STATS_DATA * stats_data = StatsGetData(game, cur->GetStat());
		ASSERT_CONTINUE(stats_data);
		if (!StatsDataTestFlag(stats_data, STATS_DATA_COMBAT))
		{
			continue;
		}
		if (!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE))
		{
			continue;
		}
		if (IS_ACCR && StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
		{
			continue;
		}
		sAccrue<TRUE>(game, cloth->stats, FALSE, cur->stat, stats_data, cur->value * MULT, FALSE);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsListRemoveWeaponStats(
	UNIT * cloth,
	UNIT * dye)
{
	ASSERT_RETURN(cloth && cloth->stats);
	STATS_TEST_MAGIC_RETURN(cloth->stats);
	ASSERT_RETURN(dye && dye->stats);
	STATS_TEST_MAGIC_RETURN(dye->stats);
	if (StatsGetParent(dye) != cloth)
	{
		return;
	}

	UNITSTATS * dye_stats = dye->stats;

	if (!StatsTestFlag(dye_stats, STATSFLAG_COMBAT_STATS_ACCRUED))
	{
		return;
	}

	// remove all riders
	if (dye_stats->m_FirstRider)
	{
		sStatsListValidateRiderChain(dye_stats);
		sStatsListRemoveRidersFromCloths(dye_stats->m_Cloth, dye_stats->m_FirstRider, dye_stats->m_LastRider);
		sStatsListValidateRiderChain(dye_stats);
	}

	// subtract the stats to accrued
	sStatsListRemoveWeaponStats<-1, TRUE>(&dye_stats->m_Accr, cloth);
	sStatsListRemoveWeaponStats<-1, FALSE>(&dye_stats->m_Calc, cloth);

	sStatsSetFlag(dye_stats, STATSFLAG_COMBAT_STATS_ACCRUED, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsListApplyWeaponStats(
	UNIT * cloth,
	UNIT * dye)
{
	ASSERT_RETURN(cloth && cloth->stats);
	STATS_TEST_MAGIC_RETURN(cloth->stats);
	ASSERT_RETURN(dye && dye->stats);
	STATS_TEST_MAGIC_RETURN(dye->stats);
	ASSERT_RETURN(StatsGetParent(dye) == cloth);

	UNITSTATS * dye_stats = dye->stats;

	if (StatsTestFlag(dye_stats, STATSFLAG_COMBAT_STATS_ACCRUED))
	{
		return;
	}

	sStatsSetFlag(dye_stats, STATSFLAG_COMBAT_STATS_ACCRUED);

	// add riders to cloth
	if (dye_stats->m_FirstRider)
	{
		sStatsListValidateRiderChain(dye_stats);
		sStatsListAddRidersToCloths(dye_stats->m_Cloth, dye_stats->m_FirstRider, dye_stats->m_LastRider);
		sStatsListValidateRiderChain(dye_stats);
	}

	// subtract the stats to accrued
	sStatsListRemoveWeaponStats<1, TRUE>(&dye_stats->m_Accr, cloth);
	sStatsListRemoveWeaponStats<1, FALSE>(&dye_stats->m_Calc, cloth);
}


//----------------------------------------------------------------------------
// get the selector for a stat list
//----------------------------------------------------------------------------
STATS_SELECTOR StatsGetSelector(
	const STATS * stats)
{
	ASSERT_RETVAL(stats, SELECTOR_NONE);
	STATS_TEST_MAGIC_RETVAL(stats, SELECTOR_NONE);
	return (STATS_SELECTOR)stats->m_Selector;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * StatsGetParent(
	const UNIT * unit)
{
	ASSERT_RETNULL(unit && unit->stats);

	const UNITSTATS * cloth = unit->stats->m_Cloth;
	if (!cloth)
	{
		return NULL;
	}

	ASSERT_RETNULL(StatsTestFlag(cloth, STATSFLAG_UNITSTATS) && cloth->m_Unit);
	
	return (cloth->m_Unit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * StatsGetParent(
	const STATS * stats)
{
	ASSERT_RETNULL(stats);

	const UNITSTATS * cloth = stats->m_Cloth;
	if (!cloth)
	{
		return NULL;
	}

	STATS_TEST_MAGIC_RETVAL(cloth, NULL);
	ASSERT_RETNULL(StatsTestFlag(cloth, STATSFLAG_UNITSTATS) && cloth->m_Unit);

	return cloth->m_Unit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsInDyeChain(
	const UNIT * cloth,
	const UNIT * dye)
{
	ASSERT_RETFALSE(cloth);
	ASSERT_RETFALSE(dye);

	UNIT * cur = StatsGetParent(dye);
	while (cur)
	{
		if (cur == cloth)
		{
			return TRUE;
		}
		UNIT * parent = StatsGetParent(dye);
		if(parent == cur)
		{
			return FALSE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STATS * StatsGetModList(
	const UNIT * unit,
	int selector)
{
	ASSERT_RETNULL(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, NULL);

	ASSERT_RETNULL(StatsTestFlag(unit->stats, STATSFLAG_UNITSTATS));

	STATS * modlist = unit->stats->m_Modlist;

	if (selector == 0)
	{
		return modlist;
	}
	while (modlist && modlist->m_Selector != selector)
	{
		modlist = modlist->m_Next;
	}
	return modlist;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STATS * StatsGetModNext(
	STATS * stats,
	int selector)
{
	ASSERT_RETNULL(stats);
	STATS_TEST_MAGIC_RETVAL(stats, NULL);
	ASSERT_RETNULL(!StatsTestFlag(stats, STATSFLAG_UNITSTATS));

	if (selector == 0)
	{
		return stats->m_Next;
	}

	stats = stats->m_Next;
	while (stats && stats->m_Selector != selector)
	{
		stats = stats->m_Next;
	}
	return stats;
}


//----------------------------------------------------------------------------
// free a statslist
//----------------------------------------------------------------------------
void StatsListFree(
	GAME * game,
	STATS * stats)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);

	ASSERT_RETURN(!StatsTestFlag(stats, STATSFLAG_UNITSTATS));
	ASSERT_RETURN(!StatsTestFlag(stats, STATSFLAG_RIDER));

	StatsListRemove(game, stats);

	STATS * rider = stats->m_FirstRider;
	while (rider)
	{
		STATS * next = rider->m_Next;
		if (rider->m_List.m_Stats)
		{
			GFREE(game, rider->m_List.m_Stats);
		}
		GFREE(game, rider);
		rider = next;
	}

	if (stats->m_List.m_Stats)
	{
		GFREE(game, stats->m_List.m_Stats);
	}
	GFREE(game, stats);
}


//----------------------------------------------------------------------------
// get the size of a stat list
//----------------------------------------------------------------------------
int StatsGetCount(
	const STATS * stats)
{
	ASSERT_RETZERO(stats);
	STATS_TEST_MAGIC_RETVAL(stats, 0);
	return stats->m_List.m_Count;
}


//----------------------------------------------------------------------------
// initialize the statslist for a unit
//----------------------------------------------------------------------------
void UnitStatsInit(
	UNIT * unit)
{
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	if (unit->stats)
	{
		UnitStatsFree(unit);
	}
	UNITSTATS * stats = (UNITSTATS *)GMALLOCZ(game, sizeof(UNITSTATS));

	STATS_SET_MAGIC(stats);
#if GLOBAL_STATS_TRACKING
	stats->m_Game = game;
#endif
	stats->m_idStats = ++game->idStats;

	sStatsSetFlag(stats, STATSFLAG_UNITSTATS);
	stats->m_Unit = unit;
	stats->m_Selector = SELECTOR_UNIT;

	unit->stats = (UNITSTATS *)stats;

	EXCELTABLE datatable = UnitGetDatatableEnum(unit);
	if (datatable != NUM_DATATABLES)
	{
		STATS * deflist = ExcelGetStats(game, datatable, UnitGetClass(unit));

		stats->m_Default = deflist;

		// do callbacks for default stats
		if (deflist)
		{
			STATS_ENTRY * entry = deflist->m_List.m_Stats;
			for (unsigned int ii = 0; ii < deflist->m_List.m_Count; ii++, entry++)
			{
				const STATS_DATA * stats_data = StatsGetData(game, entry->GetStat());
				ASSERT_CONTINUE(stats_data);

				STATS_CALLBACK_TABLE * callback = (IS_SERVER(game) ? stats_data->m_SvrStatsChangeCallback : stats_data->m_CltStatsChangeCallback);
				while (callback)
				{
					STATS_CALLBACK_TABLE * next_callback = callback->m_pNext;
					callback->m_fpCallback(unit, entry->GetStat(), entry->GetParam(), 0, entry->value, callback->m_Data, FALSE);
					callback = next_callback;
				}
			}
		}
	}
}


//----------------------------------------------------------------------------
// free a unit's statslist
//----------------------------------------------------------------------------
void UnitStatsFree(
	UNIT * unit)
{
	ASSERT_RETURN(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	UNITSTATS * stats = unit->stats;
	if (!stats)
	{
		return;
	}

	STATS_TEST_MAGIC_RETURN(stats);
	StatsListRemove(unit);

	// free all mod lists
	while (stats->m_Modlist)
	{
		StatsListFree(game, stats->m_Modlist);
	}

	// there shouldn't be anything attached to me
	ASSERT(!stats->m_Dye);
	while (stats->m_Dye)
	{
		UNITSTATS * dye = stats->m_Dye;

		dye->m_Cloth = NULL;
		dye->m_Prev = NULL;
		if (dye->m_Next)
		{
			dye->m_Next->m_Prev = NULL;
		}
		stats->m_Dye = (UNITSTATS *)dye->m_Next;
		dye->m_Next = NULL;
	}

	STATS * rider = stats->m_FirstRider;
	while (rider)
	{
		STATS * next = rider->m_Next;
		if (rider->m_List.m_Stats)
		{
			GFREE(game, rider->m_List.m_Stats);
		}
		GFREE(game, rider);
		rider = next;
	}

	if (stats->m_List.m_Stats)
	{
		GFREE(game, stats->m_List.m_Stats);
	}
	if (stats->m_Accr.m_Stats)
	{
		GFREE(game, stats->m_Accr.m_Stats);
	}
	if (stats->m_Calc.m_Stats)
	{
		GFREE(game, stats->m_Calc.m_Stats);
	}
	GFREE(game, stats);
	unit->stats = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline STATS_LIST * sUnitGetCurOrAccrList(
	UNITSTATS * unitstats,
	const STATS_DATA * stats_data)
{
	if (StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
	{
		return &unitstats->m_Calc;
	}
	else
	{
		return &unitstats->m_Accr;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const STATS_LIST * sUnitGetCurOrAccrListConst(
	const UNITSTATS * unitstats,
	const STATS_DATA * stats_data)
{
	if (StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
	{
		return &unitstats->m_Calc;
	}
	else
	{
		return &unitstats->m_Accr;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitGetCurOrAccrStat(
	GAME * game,
	UNIT * unit,
	UNITSTATS * unitstats,
	DWORD stat,
	const STATS_DATA * stats_data)
{
	ASSERT(game);
	ASSERT(unit);
	ASSERT(stats_data);

	STATS_LIST * list = sUnitGetCurOrAccrList(unitstats, stats_data);
#if GLOBAL_STATS_TRACKING
	int value =  sGetStatInList(game, list, stat, stats_data, unitstats->m_Default);
#else
	int value =  sGetStatInList(list, stat, stats_data, unitstats->m_Default);
#endif
	return sChangeStatInListApplyMinMax(game, unit, value, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), stats_data);
}


//----------------------------------------------------------------------------
// passing in the regen_on_get stat (e.g. power_cur), return the regen value
//----------------------------------------------------------------------------
static inline int sChangeStatSpecialFunctionGetRegenForRegenOnGetStat(
	GAME * game,
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	// get degen value
	if (stats_data->m_nSpecFuncStat[2] != INVALID_LINK)
	{
		int wDegenStat = stats_data->m_nSpecFuncStat[2];
		const STATS_DATA * degen_data = StatsGetData(game, wDegenStat);
		if (degen_data)
		{
			DWORD param = ((degen_data->m_nParamTable[0] == stats_data->m_nParamTable[0]) ? dwParam : 0);
			int degen = UnitGetStat(unit, wDegenStat, param);
			if (degen != 0)
			{
				return degen;
			}
		}
	}

	// get regen value
	if (stats_data->m_nSpecFuncStat[1] != INVALID_LINK)
	{
		int wRegenStat = stats_data->m_nSpecFuncStat[1];
		const STATS_DATA * regen_data = StatsGetData(game, wRegenStat);
		if (regen_data)
		{
			DWORD param = ((regen_data->m_nParamTable[0] == stats_data->m_nParamTable[0]) ? dwParam : 0);
			int regen = UnitGetStat(unit, wRegenStat, param);
			return regen;
		}
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitRecalcStatGetRegenDelay(
	UNIT * unit,
	const STATS_DATA * stats_data)
{
	int nDelay = 0;
	if (stats_data->m_nRegenDelayOnDec > 0 && UnitIsA(unit, UNITTYPE_PLAYER))
	{
		nDelay = stats_data->m_nRegenDelayOnDec;
	}
	else if (stats_data->m_nMonsterRegenDelay > 0 && UnitIsA(unit, UNITTYPE_MONSTER))
	{
		nDelay = stats_data->m_nMonsterRegenDelay;
	}
	if(nDelay > 0)
	{
		if(stats_data->m_nSpecFuncStat[4] != INVALID_LINK)
		{
			int nRegenDelay = UnitGetStat(unit, stats_data->m_nSpecFuncStat[4]);
			nDelay -= nRegenDelay;
			nDelay = MAX(1, nDelay);
		}
	}
	return nDelay;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitRecalcStatSetCurStat(
	GAME * game,
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int value,
	const STATS_DATA * stats_data)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(unit->stats);
	ASSERT_RETURN(stats_data);

	DWORD dwStat = MAKE_STAT(wStat, dwParam);
	sSetStatInList(game, &unit->stats->m_List, dwStat, stats_data, value, unit->stats->m_Default);
}


//----------------------------------------------------------------------------
// special funciton linked stat recalc for regen
// calculate the current value of a regen_on_get stat
//----------------------------------------------------------------------------
static inline int sUnitRecalcStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	GAME * game = UnitGetGame(unit);

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

	int cur = sUnitGetCurOrAccrStat(game, unit, unit->stats, dwStat, stats_data);
	
	if (IS_CLIENT(game) && UnitTestFlag(unit, UNITFLAG_JUSTCREATED))
	{
		return cur;
	}

	int regen = sChangeStatSpecialFunctionGetRegenForRegenOnGetStat(game, unit, wStat, dwParam, stats_data);
	if (regen == 0)
	{
		return cur;
	}

	ASSERT_RETVAL(stats_data->m_nMaxStat != INVALID_LINK, cur);
	int max = UnitGetStat(unit, stats_data->m_nMaxStat, dwParam);
	if (max <= 0)
	{
		return cur;
	}

	int min = INT_MIN;
	if (stats_data->m_nMinSet != INT_MIN)
	{
		min = stats_data->m_nMinSet;
	}

	if (regen > 0 && cur >= max)
	{
		return max;
	}
	if (regen < 0 && cur <= min)
	{
		return min;
	}

	int statTime = stats_data->m_nSpecFuncStat[0];

	GAME_TICK cur_tick = GameGetTick(game);

	const STATS_DATA * time_data = StatsGetData(game, statTime);
	ASSERT_RETVAL(time_data, cur);

	GAME_TICK last_set_tick = (GAME_TICK)UnitGetStat(unit, statTime, dwParam);
	if (last_set_tick == (GAME_TICK)INVALID_ID)
	{
		last_set_tick = cur_tick;
		sUnitSetStat<FALSE>(unit, statTime, dwParam, (int)last_set_tick, time_data, FALSE);
	}

	GAME_TICK last_tick = last_set_tick;
	int regendelay = sUnitRecalcStatGetRegenDelay(unit, stats_data);
	if (regendelay > 0)
	{
		int setdelta = cur_tick - last_set_tick;
		if (setdelta <= regendelay)
		{
			return cur;
		}
		int statRegenTime = stats_data->m_nSpecFuncStat[3];
		ASSERT_RETVAL(statRegenTime != INVALID_ID, cur);
		
		time_data = StatsGetData(game, statTime);
		ASSERT_RETVAL(time_data, cur);

		last_tick = (GAME_TICK)UnitGetStat(unit, statRegenTime, dwParam);
		if (last_tick < last_set_tick + regendelay)
		{
			last_tick = last_set_tick + regendelay;
		}
		statTime = statRegenTime;
	}

	int delta = cur_tick - last_tick;
	if (delta <= 0)
	{
		return cur;
	}

	if (stats_data->m_nMaxStat)
	{
		int max = UnitGetStat(unit, stats_data->m_nMaxStat, dwParam);
		if (cur >= max)
		{
			return cur;
		}
		int regenval = regen * delta / GAME_TICKS_PER_SECOND;
		if (stats_data->m_nRegenDivisor > 0)
		{
			regenval /= stats_data->m_nRegenDivisor;
		}
		cur += regenval;
		cur = PIN(cur, 0, max);
	}
	else
	{
		int regenval = regen * delta / GAME_TICKS_PER_SECOND;
		if (stats_data->m_nRegenDivisor > 0)
		{
			regenval /= stats_data->m_nRegenDivisor;
		}
		cur += regenval;
		cur = MAX(cur, 0);
	}

	sUnitSetStat<FALSE>(unit, statTime, dwParam, (int)cur_tick, time_data, FALSE);

	sUnitSetStat<FALSE>(unit, wStat, dwParam, cur, stats_data, FALSE);

	return cur;
}


//----------------------------------------------------------------------------
// special funciton linked stat recalc for regen
// calculate the current value of a regen_on_get stat
// regens as a percentage of max (as opposed to percentage of cur)
//----------------------------------------------------------------------------
static inline int sUnitRecalcStatPct(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	GAME * game = UnitGetGame(unit);

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

	int cur = sUnitGetCurOrAccrStat(game, unit, unit->stats, dwStat, stats_data);
	
	if (IS_CLIENT(game) && UnitTestFlag(unit, UNITFLAG_JUSTCREATED))
	{
		return cur;
	}

	int regen = sChangeStatSpecialFunctionGetRegenForRegenOnGetStat(game, unit, wStat, dwParam, stats_data);
	if (regen == 0)
	{
		return cur;
	}

	ASSERT_RETVAL(stats_data->m_nMaxStat != INVALID_LINK, cur);
	int max = UnitGetStat(unit, stats_data->m_nMaxStat, dwParam);
	if (max <= 0)
	{
		return cur;
	}

	int min = INT_MIN;
	if (stats_data->m_nMinSet != INT_MIN)
	{
		min = stats_data->m_nMinSet;
	}

	if (regen > 0 && cur >= max)
	{
		return max;
	}
	if (regen < 0 && cur <= min)
	{
		return min;
	}

	int statTime = stats_data->m_nSpecFuncStat[0];

	const STATS_DATA * time_data = StatsGetData(game, statTime);
	ASSERT_RETVAL(time_data, cur);

	GAME_TICK cur_tick = GameGetTick(game);

	GAME_TICK last_set_tick = (GAME_TICK)UnitGetStat(unit, statTime, dwParam);
	if (last_set_tick == (GAME_TICK)INVALID_ID)
	{
		last_set_tick = cur_tick;
		sUnitSetStat<FALSE>(unit, statTime, dwParam, (int)last_set_tick, time_data, FALSE);
	}

	GAME_TICK last_tick = last_set_tick;
	int regendelay = sUnitRecalcStatGetRegenDelay(unit, stats_data);
	if (regendelay > 0)
	{
		int setdelta = cur_tick - last_set_tick;
		if (setdelta <= regendelay)
		{
			return cur;
		}
		int statRegenTime = stats_data->m_nSpecFuncStat[3];
		ASSERT_RETVAL(statRegenTime != INVALID_ID, cur);

		time_data = StatsGetData(game, statTime);
		ASSERT_RETVAL(time_data, cur);

		last_tick = (GAME_TICK)UnitGetStat(unit, statRegenTime, dwParam);
		if (last_tick < last_set_tick + regendelay)
		{
			last_tick = last_set_tick + regendelay;
		}
		statTime = statRegenTime;
	}

	int delta = cur_tick - last_tick;
	if (delta <= 0)
	{
		return cur;
	}

	// we regen regen% per second
	int regenval = (max * regen / 100) * delta / GAME_TICKS_PER_SECOND;
	if (stats_data->m_nRegenDivisor > 0)
	{
		regenval /= stats_data->m_nRegenDivisor;
	}
	cur += regenval;
	cur = PIN(cur, min, max);

	sUnitSetStat<FALSE>(unit, statTime, dwParam, (int)cur_tick, time_data, FALSE);

	sUnitSetStat<FALSE>(unit, wStat, dwParam, cur, stats_data, FALSE);

	return cur;
}


//----------------------------------------------------------------------------
// call this when we set a regen_on_get stat (e.g. power_cur)
// this is a postset function
//----------------------------------------------------------------------------
void StatsChangeStatSpecialFunctionSetRegenOnGetStat(
	GAME * game,
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int oldval,
	int newval,
	int max)
{
	REF(oldval);

	// get the stats data for the TIME stat (e.g. power_cur_time)
	int statTime = stats_data->m_nSpecFuncStat[0];
	ASSERT_RETURN(statTime != INVALID_LINK);
	const STATS_DATA * time_data = StatsGetData(game, statTime);
	ASSERT_RETURN(time_data);

	if (!UnitIsA(unit, time_data->m_nUnitType))
	{
		return;
	}

	if (IsUnitDeadOrDying(unit))
	{
		sUnitSetStat<FALSE>(unit, statTime, dwParam, (GAME_TICK)INVALID_ID, time_data, FALSE);
		return;
	}

	int regen = sChangeStatSpecialFunctionGetRegenForRegenOnGetStat(game, unit, wStat, dwParam, stats_data);
	if (regen == 0)
	{
		sUnitSetStat<FALSE>(unit, statTime, dwParam, (GAME_TICK)INVALID_ID, time_data, FALSE);
		return;
	}

	if (regen < 0 && stats_data->m_nMinSet != INT_MIN)
	{
		if (newval <= stats_data->m_nMinSet)
		{
			sUnitSetStat<FALSE>(unit, statTime, dwParam, (GAME_TICK)INVALID_ID, time_data, FALSE);
			return;
		}
	}

	if (regen > 0 && stats_data->m_nMaxStat != INVALID_LINK)
	{
		if (newval >= max || max <= 0)
		{
			sUnitSetStat<FALSE>(unit, statTime, dwParam, (GAME_TICK)INVALID_ID, time_data, FALSE);
			return;
		}
	}
	
	// record the tick value at which we set this stat
	if(newval == oldval)
	{
		return;
	}

	GAME_TICK dwGameTick = GameGetTick(UnitGetGame(unit));
	GAME_TICK dwStatTick = UnitGetStat(unit, statTime, dwParam);
	if (dwStatTick == (GAME_TICK)INVALID_ID || dwGameTick > dwStatTick)
	{
		sUnitSetStat<FALSE>(unit, statTime, dwParam, (GAME_TICK)dwGameTick, time_data, FALSE);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sChangeStatSpecialFunctionPreSet(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	switch (stats_data->m_nSpecialFunction)
	{
	default:
		return;
	case STATSSPEC_LINKED_REGEN:
		{
			// recalc a on-get stat right before it's retrieved
			ASSERT_RETURN(stats_data->m_nSpecFuncStat[0] > INVALID_LINK);
			const STATS_DATA * dest_data = StatsGetData(UnitGetGame(unit), stats_data->m_nSpecFuncStat[0]);
			ASSERT_RETURN(dest_data);
			if (UnitIsA(unit, dest_data->m_nUnitType))
			{
				sUnitRecalcStat(unit, stats_data->m_nSpecFuncStat[0], dwParam, dest_data);
			}
		}
		break;
	case STATSSPEC_PCT_LINKED_REGEN:
		{
			ASSERT_RETURN(stats_data->m_nSpecFuncStat[0] > INVALID_LINK);
			const STATS_DATA * dest_data = StatsGetData(UnitGetGame(unit), stats_data->m_nSpecFuncStat[0]);
			ASSERT_RETURN(dest_data);
			if (UnitIsA(unit, dest_data->m_nUnitType))
			{
				sUnitRecalcStatPct(unit, stats_data->m_nSpecFuncStat[0], dwParam, dest_data);
			}
		}
		break;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sChangeStatSpecialFunctionPostSet(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	int oldval,
	int newval,
	int max)
{
	switch (stats_data->m_nSpecialFunction)
	{
	case STATSSPEC_REGEN_ON_GET:
	case STATSSPEC_PCT_REGEN_ON_GET:
		// set the time the base stat was changed
		StatsChangeStatSpecialFunctionSetRegenOnGetStat(UnitGetGame(unit), unit, wStat, dwParam, stats_data, oldval, newval, max);
		break;
	default:
		return;
	}
}


//----------------------------------------------------------------------------
// assume that max == 0
//----------------------------------------------------------------------------
void UnitSetStatDelayedTimer(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int value)
{
	ASSERT_RETURN(unit);
	ASSERT_RETURN(wStat != INVALID_ID);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	if (!(stats_data->m_nMonsterRegenDelay || stats_data->m_nRegenDelayOnDec || stats_data->m_nRegenDelayOnZero))
	{
		return;
	}

	int regen = sChangeStatSpecialFunctionGetRegenForRegenOnGetStat(game, unit, wStat, dwParam, stats_data);
	if (regen == 0)
	{
		return;
	}

	int statTime = stats_data->m_nSpecFuncStat[0];
	ASSERT_RETURN(statTime != INVALID_ID);

	const STATS_DATA * time_data = StatsGetData(game, statTime);
	ASSERT_RETURN(time_data);

	GAME_TICK dwGameTick = GameGetTick(game);
	GAME_TICK dwStatTick = UnitGetStat(unit, statTime, dwParam);
	if (dwStatTick == (GAME_TICK)INVALID_ID || dwGameTick > dwStatTick)
	{
		sUnitSetStat<FALSE>(unit, statTime, dwParam, (GAME_TICK)dwGameTick, time_data, FALSE);
	}

	if (IS_SERVER(game))
	{
		sSendStat(game, unit, MAKE_STAT(wStat, dwParam), stats_data, value);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sStatsGetStat(
	STATS * stats,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	ASSERT_RETZERO(stats && stats_data);
	STATS_TEST_MAGIC_RETVAL(stats, 0);

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

#if GLOBAL_STATS_TRACKING
	return sGetStatInList(stats->m_Game, &stats->m_List, dwStat, stats_data);
#else
	return sGetStatInList(&stats->m_List, dwStat, stats_data);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetStat(
	STATS * stats,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(stats);
	STATS_TEST_MAGIC_RETVAL(stats, 0);

	const STATS_DATA * stats_data = StatsGetData(NULL, wStat);
	ASSERT_RETZERO(stats_data);

	return sStatsGetStat(stats, wStat, dwParam, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetStatShift(
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(stats);
	STATS_TEST_MAGIC_RETVAL(stats, 0);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sStatsGetStat(stats, wStat, dwParam, stats_data) >> stats_data->m_nShift;
}


//----------------------------------------------------------------------------
// used for sUnitStatsChangeMaxRatio, where we need the value pre-min max
//----------------------------------------------------------------------------
int UnitGetStatDontApplyMinMax(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	UNITSTATS * unitstats = (UNITSTATS *)unit->stats;

	DWORD dwStat = MAKE_STAT(wStat, dwParam);
	STATS_LIST * list = sUnitGetCurOrAccrList(unitstats, stats_data);

#if GLOBAL_STATS_TRACKING
	return sGetStatInList(game, list, dwStat, stats_data, unitstats->m_Default);
#else
	return sGetStatInList(list, dwStat, stats_data, unitstats->m_Default);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitGetStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	switch (stats_data->m_nSpecialFunction)
	{
	case STATSSPEC_REGEN_ON_GET:
		if (unit && !IsUnitDeadOrDying(unit))
		{
			return sUnitRecalcStat(unit, wStat, dwParam, stats_data);
		}
		break;
	case STATSSPEC_PCT_REGEN_ON_GET:
		if (unit && !IsUnitDeadOrDying(unit))
		{
			return sUnitRecalcStatPct(unit, wStat, dwParam, stats_data);
		}
		break;
	case STATSSPEC_ALWAYS_CALC:
		return StatsGetCalculatedStat(unit, 0, NULL, wStat, dwParam);
	}

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

#if !STATS_VERIFY_ON_GET
	return sUnitGetCurOrAccrStat(UnitGetGame(unit), unit, unit->stats, dwStat, stats_data);
#else
	int calcValue = sUnitCalcStat(unit, wStat, dwParam, stats_data);
	int accrValue = sUnitGetCurOrAccrStat(UnitGetGame(unit), unit, unit->stats, dwStat, stats_data);
	if(calcValue != accrValue)
	{
		trace("Stat %s calculated value (%d) is not equal to accrued value (%d)\n", stats_data->m_szName, calcValue, accrValue);
	}
	return accrValue;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetAccruedStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

#if GLOBAL_STATS_TRACKING
	int value = sGetStatInList(game, &unit->stats->m_Accr, dwStat, stats_data, unit->stats->m_Default);
#else
	int value = sGetStatInList(&unit->stats->m_Accr, dwStat, stats_data, unit->stats->m_Default);
#endif

	return value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sUnitGetStat(unit, wStat, dwParam, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStat(
	struct UNIT * unit,
	int wStat,
	PARAM dwParam1,
	PARAM dwParam2)
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	if(pStatDesc && pStatDesc->m_nValBitsWindow)
		return UnitGetBitfieldStat(unit, wStat, dwParam1, dwParam2);
	else
		return UnitGetStat(unit, wStat, StatParam(wStat, dwParam1, dwParam2));
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStat(
	struct UNIT * unit,
	int wStat,
	PARAM dwParam1,
	PARAM dwParam2,
	PARAM dwParam3)
{
	return UnitGetStat(unit, wStat, StatParam(wStat, dwParam1, dwParam2, dwParam3));
}	

//----------------------------------------------------------------------------
// Used to get the regen value of a passed stat
//----------------------------------------------------------------------------
int UnitGetStatRegen (
	UNIT * unit,
	int wStat,
	PARAM dwParam )
{
	ASSERT_RETZERO(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sChangeStatSpecialFunctionGetRegenForRegenOnGetStat(game, unit, wStat, 0, stats_data);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStatShift(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sUnitGetStat(unit, wStat, dwParam, stats_data) >> stats_data->m_nShift;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitGetBaseStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	ASSERT_RETZERO(unit && unit->stats && stats_data);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);
	DWORD dwStat = MAKE_STAT(wStat, dwParam);

#if GLOBAL_STATS_TRACKING
	return sGetStatInList(UnitGetGame(unit), &unit->stats->m_List, dwStat, stats_data);
#else
	return sGetStatInList(&unit->stats->m_List, dwStat, stats_data);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetBaseStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sUnitGetBaseStat(unit, wStat, dwParam, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetBaseStatShift(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sUnitGetBaseStat(unit, wStat, dwParam, stats_data) >> stats_data->m_nShift;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitGetModStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	ASSERT_RETZERO(unit && unit->stats && stats_data);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

	int accr = sUnitGetCurOrAccrStat(UnitGetGame(unit), unit, unit->stats, dwStat, stats_data);
#if GLOBAL_STATS_TRACKING
	int base = sGetStatInList(UnitGetGame(unit), &unit->stats->m_List, dwStat, stats_data);
#else
	int base = sGetStatInList(&unit->stats->m_List, dwStat, stats_data);
#endif

	return accr - base;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetModStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sUnitGetModStat(unit, wStat, dwParam, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetModStatShift(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sUnitGetModStat(unit, wStat, dwParam, stats_data) >> stats_data->m_nShift;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sStatsSendStatsListStat(
	GAME * game,
	UNIT * unit,
	STATS * stats,
	DWORD dwStat,
	int nValue)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);
	ASSERT_RETURN(stats);
	if (IS_SERVER(game))
	{
		WORD wStat = STAT_GET_STAT(dwStat);
		const STATS_DATA * stats_data = StatsGetData(game, wStat);
		if (stats_data && (StatsDataTestFlag(stats_data, STATS_DATA_SEND_TO_SELF) || StatsDataTestFlag(stats_data, STATS_DATA_SEND_TO_ALL)))
		{
			MSG_SCMD_UNITSETSTATINLIST tMsg;
			tMsg.id = UnitGetId(unit);
			tMsg.dwStatsListId = stats->m_idStats;
			tMsg.dwStat = dwStat;
			tMsg.nValue = nValue;
			s_SendUnitMessage(unit, SCMD_UNITSETSTATINLIST, &tMsg);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sStatsSetStat(
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam,
	int nValue,
	const STATS_DATA * stats_data)
{
	ASSERT_RETURN(stats && stats_data);
	STATS_TEST_MAGIC_RETURN(stats);
	ASSERT_RETURN(!StatsTestFlag(stats, STATSFLAG_UNITSTATS))

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

	ASSERT_RETURN(stats_data->m_nMaxStat <= INVALID_LINK);

	int delta = sSetStatInList(game, &stats->m_List, dwStat, stats_data, nValue);
	if (delta == 0)
	{
		return;
	}

	if (StatsTestFlag(stats, STATSFLAG_RIDER))
	{
		return;
	}

	if (!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE))
	{
		return;
	}

	BOOL bSend = StatsTestFlag(stats, STATSFLAG_SENT_ON_ADD);

	if(!bSend && stats->m_Cloth && stats->m_Cloth->m_Unit)
	{
		sStatsSendStatsListStat(game, stats->m_Cloth->m_Unit, stats, dwStat, nValue);
	}

	sAccrue<TRUE>(game, stats->m_Cloth, TRUE, dwStat, stats_data, delta, bSend);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sStatsGetCalculatedStat(
	GAME * game,
	UNIT * unit,
	int objecttype,
	STATS * stats,
	BOOL bUseRiderAsBase,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data)
{
	ASSERT_RETZERO((stats_data = sStatsGetStatsData(wStat, stats_data)) != NULL);

	int value = 0;

	if (stats_data->m_nSpecialFunction != STATSSPEC_ALWAYS_CALC)
	{
		if (!unit || (bUseRiderAsBase && stats))
		{
			ASSERT_RETZERO(stats);
			value = StatsGetStat(stats, wStat, DO_PARAM(stats_data, 0, dwParam));
		}
		else if (!stats || !StatsDataTestFlag(stats_data, STATS_DATA_CALC_RIDER))
		{
			value = UnitGetStat(unit, wStat, DO_PARAM(stats_data, 0, dwParam));
		}
		else
		{
			value = StatsGetStat(stats, wStat, DO_PARAM(stats_data, 0, dwParam));
		}
	}

	BOOL isMelee = TRUE;			// not sure what to do here -- Tyler
	return sUnitCalcStat(unit, wStat, dwParam, stats_data, objecttype, stats, isMelee);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetCalculatedStat(
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	return sStatsGetCalculatedStat(game, unit, objecttype, stats, TRUE, wStat, dwParam, NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetCalculatedStatShift(
	UNIT * unit,
	int objecttype,
	STATS * stats,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(stats_data);

	return sStatsGetCalculatedStat(game, unit, objecttype, stats, TRUE, wStat, dwParam, stats_data) >> stats_data->m_nShift;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsSetStat(
	GAME * game,
	STATS * stats,
	int wStat,
	int nValue)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		UnitSetStat(unitstats->m_Unit, wStat, nValue);
		return;
	}

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsSetStat(game, stats, wStat, 0, nValue, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsSetStat(
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam,
	int nValue)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		//UnitSetStat(unitstats->m_Unit, wStat, dwParam, nValue);

		STATS_TEST_MAGIC_RETURN(unitstats->m_Unit->stats);
		const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unitstats->m_Unit), wStat);

		sUnitSetStat<TRUE>(unitstats->m_Unit, wStat, dwParam, nValue, stats_data);
		return;
	}

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsSetStat(game, stats, wStat, dwParam, nValue, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float StatsGetAsFloat( 
	int wStat,
	int nStatValueAsInt )
{
	if (!nStatValueAsInt)
	{
		return 0.0f;
	}

	const STATS_DATA * stats_data = StatsGetData(NULL, wStat);
	ASSERT_RETZERO(stats_data);

	int nFraction = 1 << stats_data->m_nShift;
	float fValue = (float)nStatValueAsInt / (float)nFraction;
	return fValue;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetAsShiftedInt( 
	int wStat,
	float fStatValueAsFloat )
{
	if (fStatValueAsFloat == 0.0f)
	{
		return 0;
	}

	const STATS_DATA * stats_data = StatsGetData(NULL, wStat);
	ASSERT_RETZERO(stats_data);

	int nFraction = 1 << stats_data->m_nShift;
	int nValue = (int)(fStatValueAsFloat * (float)nFraction);
	return nValue;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsSetStatShift(
	GAME * game,
	STATS * stats,
	int wStat,
	int nValue)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		UnitSetStatShift(unitstats->m_Unit, wStat, nValue);
		return;
	}
	ASSERT_RETURN(game);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsSetStat(game, stats, wStat, 0, nValue << stats_data->m_nShift, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsSetStatShift(
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam,
	int nValue)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		UnitSetStatShift(unitstats->m_Unit, wStat, dwParam, nValue);
		return;
	}

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsSetStat(game, stats, wStat, dwParam, nValue << stats_data->m_nShift, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL RUN_SPEC>
static inline void sUnitSetStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int value,
	const STATS_DATA * stats_data,
	BOOL bSend)
{
	ASSERT_RETURN(unit && unit->stats && stats_data);
	STATS_TEST_MAGIC_RETURN(unit->stats);
	ASSERT_RETURN(!StatsDataTestFlag(stats_data, STATS_DATA_NO_SET));

	if (stats_data->m_nUnitType > UNITTYPE_NONE)
	{
		if (!UnitIsA(unit, stats_data->m_nUnitType))
		{
			return;
		}
	}

	sSetStatInUnitList<RUN_SPEC>(UnitGetGame(unit), unit, wStat, dwParam, stats_data, value, bSend);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStat(
	UNIT * unit,
	int wStat,
	int nValue)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitSetStat<TRUE>(unit, wStat, 0, nValue, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int nValue)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	if(stats_data && stats_data->m_nValBitsWindow)
		UnitSetBitfieldStat(unit, wStat, dwParam, nValue);
	else
		sUnitSetStat<TRUE>(unit, wStat, dwParam, nValue, stats_data);
}


//----------------------------------------------------------------------------
// 2 params
//----------------------------------------------------------------------------
void UnitSetStat(
	struct UNIT * unit,
	int wStat,
	PARAM dwParam1,
	PARAM dwParam2,
	int nValue)
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	if(pStatDesc && pStatDesc->m_nValBitsWindow)
		UnitSetBitfieldStat(unit, wStat, dwParam1, dwParam2, nValue);
	else
		UnitSetStat(unit, wStat, StatParam(wStat, dwParam1, dwParam2), nValue);

}	


//----------------------------------------------------------------------------
// 3 params
//----------------------------------------------------------------------------
void UnitSetStat(
	struct UNIT * unit,
	int wStat,
	PARAM dwParam1,
	PARAM dwParam2,
	PARAM dwParam3,
	int nValue)
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	if(pStatDesc && pStatDesc->m_nValBitsWindow)
		UnitSetBitfieldStat(unit, wStat, dwParam1, dwParam2, dwParam3, nValue);
	else
		UnitSetStat(unit, wStat, StatParam(wStat, dwParam1, dwParam2, dwParam3), nValue);

}	

void UnitSetBitfieldStat(
	struct UNIT* unit,
	int wStat,
	PARAM dwParam1,
	PARAM dwParam2,
	int value )
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	int nBlockSize = pStatDesc->m_nValBits;
	int nValSize = pStatDesc->m_nValBitsWindow;
	ASSERT(IsPowerOf2(nValSize));

	int nValsPerBlock = nBlockSize/nValSize;
	DWORD nCur = UnitGetStat(unit, wStat, StatParam(wStat, dwParam1/(nValsPerBlock), dwParam2));
	//for(int ii = 0; ii < nValSize; ii++)
	//{
	//	SETBIT(nCur, (unsigned int)(ii+(nValSize*(dwParam1%nValsPerBlock))), (BOOL)(((unsigned int)value >> ii) & 1));
	//}
	nCur |= (value << (nValSize * (dwParam1%nValsPerBlock)));

	sUnitSetStat<TRUE>(unit, wStat, StatParam(wStat, dwParam1/(nValsPerBlock), dwParam2), nCur, pStatDesc);
}

void UnitSetBitfieldStat(
struct UNIT* unit,
	int wStat,
	PARAM dwParam1,
	int value )
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	int nBlockSize = pStatDesc->m_nValBits;
	int nValSize = pStatDesc->m_nValBitsWindow;
	ASSERT(IsPowerOf2(nValSize));

	int nValsPerBlock = nBlockSize/nValSize;
	DWORD nCur = UnitGetStat(unit, wStat, StatParam(wStat, dwParam1/(nValsPerBlock)));
	nCur |= (value << (nValSize * (dwParam1%nValsPerBlock)));

	sUnitSetStat<TRUE>(unit, wStat, StatParam(wStat, dwParam1/(nValsPerBlock)), nCur, pStatDesc);
}

void UnitSetBitfieldStat(
struct UNIT* unit,
	int wStat,
	PARAM dwParam1,
	PARAM dwParam2,
	PARAM dwParam3,
	int value )
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	int nBlockSize = pStatDesc->m_nValBits;
	int nValSize = pStatDesc->m_nValBitsWindow;
	ASSERT(IsPowerOf2(nValSize));

	int nValsPerBlock = nBlockSize/nValSize;
	DWORD nCur = UnitGetStat(unit, wStat, StatParam(wStat, dwParam1/(nValsPerBlock), dwParam2, dwParam3));
	//nCur |= (value << (nValSize * (dwParam1%nValsPerBlock)));
	for(int ii = 0; ii < nBlockSize; ii++)
		SETBIT(nCur, ii, (value >> ii)&1);

	sUnitSetStat<TRUE>(unit, wStat, StatParam(wStat, dwParam1/(nValsPerBlock), dwParam2, dwParam3), nCur, pStatDesc) ;
}

int UnitGetBitfieldStatClearbits(
	struct UNIT* unit,
	int wStat,
	PARAM dwParam1)
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	int nBlockSize = pStatDesc->m_nValBits;
	int nValSize = pStatDesc->m_nValBitsWindow;
	ASSERT(IsPowerOf2(nValSize));

	int nValsPerBlock = nBlockSize/nValSize;
	unsigned int clearBits = 0;
	for(int ii = 0; ii < nValSize;ii++)
	{
		clearBits |= (1 << (ii+(nValSize*(dwParam1%nValsPerBlock))));
	}
	return clearBits;
}




int UnitGetBitfieldStat(
	struct UNIT* unit,
	int wStat,
	PARAM dwParam1 )
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	int nBlockSize = pStatDesc->m_nValBits;
	int nValSize = pStatDesc->m_nValBitsWindow;
	ASSERT(nValSize);
	ASSERT(IsPowerOf2(nValSize));
	int nValsPerBlock = nBlockSize/nValSize;

	int clearBits = UnitGetBitfieldStatClearbits(unit, wStat, dwParam1);
	unsigned int ret = UnitGetStat(unit, wStat, StatParam(wStat, dwParam1/nValsPerBlock));
	return (ret & clearBits) >>(nValSize*(dwParam1%nValsPerBlock)) ;

}

int UnitGetBitfieldStat(
struct UNIT* unit,
	int wStat,
	PARAM dwParam1,
	PARAM dwParam2 )
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	int nBlockSize = pStatDesc->m_nValBits;
	int nValSize = pStatDesc->m_nValBitsWindow;
	ASSERT(nValSize);
	ASSERT(IsPowerOf2(nValSize));
	int nValsPerBlock = nBlockSize/nValSize;

	int clearBits = UnitGetBitfieldStatClearbits(unit, wStat, dwParam1);
	unsigned int ret = UnitGetStat(unit, wStat, StatParam(wStat, dwParam1/nValsPerBlock, dwParam2));
	return (ret & clearBits) >>(nValSize*(dwParam1%nValsPerBlock)) ;

}

int UnitGetBitfieldStat(
struct UNIT* unit,
	int wStat,
	PARAM dwParam1,
	PARAM dwParam2,
	PARAM dwParam3)
{
	STATS_DATA* pStatDesc = (STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, wStat);
	int nBlockSize = pStatDesc->m_nValBits;
	int nValSize = pStatDesc->m_nValBitsWindow;
	ASSERT(nValSize);
	ASSERT(IsPowerOf2(nValSize));
	int nValsPerBlock = nBlockSize/nValSize;

	int clearBits = UnitGetBitfieldStatClearbits(unit, wStat, dwParam1);
	unsigned int ret = UnitGetStat(unit, wStat, StatParam(wStat, dwParam1/nValsPerBlock, dwParam2, dwParam3));
	return (ret & clearBits) >>(nValSize*(dwParam1%nValsPerBlock)) ;

}

//----------------------------------------------------------------------------
// GUID Stat
//----------------------------------------------------------------------------
void UnitSetGUIDStat(
	struct UNIT * unit,
	int wStat,
	PGUID guid)
{
	ASSERTX_RETURN( unit, "Expected unit" );
	ASSERTX_RETURN( wStat != STATS_INVALID, "Invalid stat" );
	
	DWORD dwHigh;
	DWORD dwLow;
	SPLIT_GUID( guid, dwHigh, dwLow );	
	UnitSetStat( unit, wStat, GUID_PARAM_LOW, dwLow );
	UnitSetStat( unit, wStat, GUID_PARAM_HIGH, dwHigh );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID UnitGetGUIDStat(
	struct UNIT * unit,
	int wStat)
{
	ASSERTX_RETVAL( unit, INVALID_GUID, "Expected unit" );
	ASSERTX_RETVAL( wStat != STATS_INVALID, INVALID_GUID, "Invalid stat" );

	// get stats
	DWORD dwLow = UnitGetStat( unit, wStat, GUID_PARAM_LOW );
	DWORD dwHigh = UnitGetStat( unit, wStat, GUID_PARAM_HIGH );

	// construct GUID
	PGUID guid = CREATE_GUID( dwHigh, dwLow );
	return guid;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatNoSend(
	UNIT * unit,
	int wStat,
	int nValue)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitSetStat<TRUE>(unit, wStat, 0, nValue, stats_data, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatShift(
	UNIT * unit,
	int wStat,
	int nValue)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitSetStat<TRUE>(unit, wStat, 0, nValue << stats_data->m_nShift, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatShift(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int nValue)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitSetStat<TRUE>(unit, wStat, dwParam, nValue << stats_data->m_nShift, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatShiftNoSend(
	UNIT * unit,
	int wStat,
	int nValue)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitSetStat<TRUE>(unit, wStat, 0, nValue << stats_data->m_nShift, stats_data, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatShiftNoSend(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int nValue)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitSetStat<TRUE>(unit, wStat, dwParam, nValue << stats_data->m_nShift, stats_data, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sGetStatMax(
	UNITSTATS * stats,
	const STATS_DATA * stats_data,
	int wStat,
	PARAM dwParam = 0)
{
	ASSERT_RETZERO(stats && stats_data);
	STATS_TEST_MAGIC_RETVAL(stats, 0);

	if (stats_data->m_nMaxStat <= INVALID_LINK)
	{
		return 0;
	}

	return sStatsGetStat(stats, stats_data->m_nMaxStat, dwParam, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sStatsChangeStat(
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam,
	int nDelta,
	const STATS_DATA * stats_data)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	ASSERT_RETURN(stats_data)
	ASSERTV(StatsDataTestFlag(stats_data, STATS_DATA_MODLIST), "modlist not set for stat [%s]", stats_data->m_szName);
	ASSERT(!StatsTestFlag(stats, STATSFLAG_UNITSTATS));

	int max = 0;
	if (stats->m_Cloth)
	{
		max = sGetStatMax(stats->m_Cloth, stats_data, wStat, dwParam);
	}

	DWORD dwStat = MAKE_STAT(wStat, dwParam);

	int newvalue;
	sChangeStatInList(game, &stats->m_List, dwStat, stats_data, nDelta, max, NULL, NULL, &newvalue);

	if (StatsTestFlag(stats, STATSFLAG_RIDER))
	{
		return;
	}

	if (!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE))
	{
		return;
	}

	BOOL bSend = StatsTestFlag(stats, STATSFLAG_SENT_ON_ADD);

	if(!bSend && stats->m_Cloth && stats->m_Cloth->m_Unit)
	{
		sStatsSendStatsListStat(game, stats->m_Cloth->m_Unit, stats, dwStat, newvalue);
	}

	sAccrue<TRUE>(game, stats->m_Cloth, TRUE, dwStat, stats_data, nDelta, bSend, max);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsChangeStat(
	GAME * game,
	STATS * stats,
	int wStat,
	int nDelta)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	if (nDelta == 0)
	{
		return;
	}

	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		UnitChangeStat(unitstats->m_Unit, wStat, nDelta);
		return;
	}

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsChangeStat(game, stats, wStat, 0, nDelta, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsChangeStat(
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam,
	int nDelta)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);

	if (nDelta == 0)
	{
		return;
	}

	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		UnitChangeStat(unitstats->m_Unit, wStat, dwParam, nDelta);
		return;
	}

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsChangeStat(game, stats, wStat, dwParam, nDelta, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsChangeStatShift(
	GAME * game,
	STATS * stats,
	int wStat,
	int nDelta)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);

	if (nDelta == 0)
	{
		return;
	}

	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		UnitChangeStatShift(unitstats->m_Unit, wStat, nDelta);
		return;
	}

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsChangeStat(game, stats, wStat, 0, nDelta << stats_data->m_nShift, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsChangeStatShift(
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam,
	int nDelta)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);

	if (nDelta == 0)
	{
		return;
	}

	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		UnitChangeStatShift(unitstats->m_Unit, wStat, dwParam, nDelta);
		return;
	}

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsChangeStat(game, stats, wStat, dwParam, nDelta << stats_data->m_nShift, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sGetStatMax(
	UNIT * unit,
	const STATS_DATA * stats_data,
	int wStat,
	PARAM dwParam = 0)
{
	ASSERT_RETZERO(unit && unit->stats && stats_data);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	if (stats_data->m_nMaxStat <= INVALID_LINK)
	{
		return 0;
	}
	const STATS_DATA * max_data = StatsGetData(UnitGetGame(unit), stats_data->m_nMaxStat);
	ASSERT_RETZERO(max_data);

	return sUnitGetStat(unit, stats_data->m_nMaxStat, dwParam, max_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sUnitChangeStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int delta,
	const STATS_DATA * stats_data)
{
	ASSERT_RETURN(unit && unit->stats && stats_data);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	if (stats_data->m_nUnitType > UNITTYPE_NONE)
	{
		if (!UnitIsA(unit, stats_data->m_nUnitType))
		{
			return;
		}
	}

	GENUS eGenus = UnitGetGenus( unit );
	if (eGenus == GENUS_NONE)
	{
		const int MAX_MESSAGE = 256;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf( 
			szMessage, 
			MAX_MESSAGE,
			"Changing stat '%s' by delta (%d) on unit '%s' that has no genus",
			stats_data->m_szName,
			delta,
			UnitGetDevName( unit ));
		ASSERTX( 0, szMessage );
	}
	if (stats_data->m_bUnitTypeDebug[ eGenus ] == FALSE)
	{
		const int MAX_MESSAGE = 256;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf( 
			szMessage, 
			MAX_MESSAGE,
			"Unit '%s' not allowed to change stat '%s' by delta (%d)",
			UnitGetDevName( unit ),
			stats_data->m_szName,
			delta);
		ASSERTX( 0, szMessage );	
	}

	switch (stats_data->m_nSpecialFunction)
	{
	case STATSSPEC_REGEN_ON_GET:
		if (unit && !IsUnitDeadOrDying(unit))
		{
			sUnitRecalcStat(unit, wStat, dwParam, stats_data);
		}
		break;
	case STATSSPEC_PCT_REGEN_ON_GET:
		if (unit && !IsUnitDeadOrDying(unit))
		{
			sUnitRecalcStatPct(unit, wStat, dwParam, stats_data);
		}
		break;
	case STATSSPEC_ALWAYS_CALC:
		// This causes problems with transfering damages
		//ASSERT_RETURN(0);
		break;
	}

	int max = sGetStatMax(unit, stats_data, wStat, dwParam);

	STATS_LIST * list = &(unit->stats->m_List);

	int oldvalue, newvalue;
	sChangeStatInUnitList<TRUE>(UnitGetGame(unit), unit, list, wStat, dwParam, stats_data, delta, max, oldvalue, newvalue);

	DWORD dwStat = MAKE_STAT(wStat, dwParam);
	sAccrue<TRUE>(UnitGetGame(unit), unit->stats, TRUE, dwStat, stats_data, delta, TRUE, max);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitChangeStat(
	UNIT * unit,
	int wStat,
	int nDelta)
{
	if (nDelta == 0)
	{
		return;
	}
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitChangeStat(unit, wStat, 0, nDelta, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitChangeStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int nDelta)
{
	if (nDelta == 0)
	{
		return;
	}
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitChangeStat(unit, wStat, dwParam, nDelta, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitChangeStatShift(
	UNIT * unit,
	int wStat,
	int nDelta)
{
	if (nDelta == 0)
	{
		return;
	}
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitChangeStat(unit, wStat, 0, nDelta << stats_data->m_nShift, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitChangeStatShift(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	int nDelta)
{
	if (nDelta == 0)
	{
		return;
	}
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitChangeStat(unit, wStat, dwParam, nDelta << stats_data->m_nShift, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsClearStat(
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETURN(game && stats);
	STATS_TEST_MAGIC_RETURN(stats);

	if (StatsTestFlag(stats, STATSFLAG_UNITSTATS))
	{
		UNITSTATS * unitstats = (UNITSTATS *)stats;
		ASSERT(unitstats->m_Unit);
		UnitClearStat(unitstats->m_Unit, wStat, dwParam);
		return;
	}

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	sStatsSetStat(game, stats, wStat, dwParam, 0 - stats_data->m_nStatOffset, stats_data);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitClearStat(
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);
	ASSERT_RETURN(StatsTestFlag(unit->stats, STATSFLAG_UNITSTATS));

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(stats_data);

	sUnitSetStat<TRUE>(unit, wStat, dwParam, 0 - stats_data->m_nStatOffset, stats_data);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitClearStatAll(
	UNIT * unit,
	int wStat)
{
	ASSERTX_RETURN( unit, "Expected unit" );
	ASSERTX_RETURN( wStat != INVALID_LINK, "Expected stat link" );

	// get the stats list	
	const int MAX_STATS = 1024;  // there must be a better way to do this without a hard limit?
	STATS_ENTRY tStatsList[ MAX_STATS ];	
	int nCount = UnitGetStatsRange( unit, wStat, 0, tStatsList, MAX_STATS );
	for (int i = 0; i < nCount; ++i)
	{
		int nParam = tStatsList[ i ].GetParam();
		UnitClearStat( unit, wStat, nParam );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static VECTOR sVectorize(
	DWORD dwX,
	DWORD dwY,
	DWORD dwZ)
{
	VECTOR vRet;
	vRet.fX = *((float *)&dwX);
	vRet.fY = *((float *)&dwY);
	vRet.fZ = *((float *)&dwZ);
	return vRet;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCheckStatVector(
	UNIT * unit,
	int wStat)
{
#if !ISVERSION(OPTIMIZED_VERSION)
	for (int ii = 0; ii < 3; ++ii)
	{
		const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat + ii);
		ASSERT(StatsDataTestFlag(stats_data, STATS_DATA_VECTOR));
		ASSERT(StatsDataTestFlag(stats_data, STATS_DATA_FLOAT));
		ASSERT(!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE));
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCheckStatVector(
	GAME * game,
	STATS * pStats,
	int wStat)
{
#if !ISVERSION(OPTIMIZED_VERSION)
	for (int ii = 0; ii < 3; ++ii)
	{
		const STATS_DATA * stats_data = StatsGetData(game, wStat + ii);
		ASSERT(StatsDataTestFlag(stats_data, STATS_DATA_VECTOR));
		ASSERT(StatsDataTestFlag(stats_data, STATS_DATA_FLOAT));
		ASSERT(!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE));
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitGetStatVector( 
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
	sCheckStatVector(unit, wStat);
	DWORD dwX = UnitGetStat(unit, wStat + 0, dwParam);
	DWORD dwY = UnitGetStat(unit, wStat + 1, dwParam);
	DWORD dwZ = UnitGetStat(unit, wStat + 2, dwParam);
	return sVectorize(dwX, dwY, dwZ);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitGetStatVector( 
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	PARAM dwParam2)
{
	sCheckStatVector(unit, wStat);
	DWORD dwX = UnitGetStat(unit, wStat + 0, dwParam, dwParam2);
	DWORD dwY = UnitGetStat(unit, wStat + 1, dwParam, dwParam2);
	DWORD dwZ = UnitGetStat(unit, wStat + 2, dwParam, dwParam2);
	return sVectorize(dwX, dwY, dwZ);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR UnitGetStatVector( 
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	PARAM dwParam2,
	PARAM dwParam3)
{
	sCheckStatVector(unit, wStat);
	DWORD dwX = UnitGetStat(unit, wStat + 0, dwParam, dwParam2, dwParam3);
	DWORD dwY = UnitGetStat(unit, wStat + 1, dwParam, dwParam2, dwParam3);
	DWORD dwZ = UnitGetStat(unit, wStat + 2, dwParam, dwParam2, dwParam3);
	return sVectorize(dwX, dwY, dwZ);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatVector( 
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	const VECTOR & vValue)
{
	sCheckStatVector(unit, wStat);
	UnitSetStat(unit, wStat + 0, dwParam, *((DWORD *)&vValue.fX));
	UnitSetStat(unit, wStat + 1, dwParam, *((DWORD *)&vValue.fY));
	UnitSetStat(unit, wStat + 2, dwParam, *((DWORD *)&vValue.fZ));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatVector( 
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	PARAM dwParam2,
	const VECTOR & vValue)
{
	sCheckStatVector(unit, wStat);
	UnitSetStat(unit, wStat + 0, dwParam, dwParam2, *((DWORD *)&vValue.fX));
	UnitSetStat(unit, wStat + 1, dwParam, dwParam2, *((DWORD *)&vValue.fY));
	UnitSetStat(unit, wStat + 2, dwParam, dwParam2, *((DWORD *)&vValue.fZ));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatVector( 
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	PARAM dwParam2,
	PARAM dwParam3,
	const VECTOR & vValue)
{
	sCheckStatVector(unit, wStat);
	UnitSetStat(unit, wStat + 0, dwParam, dwParam2, dwParam3, *((DWORD *)&vValue.fX));
	UnitSetStat(unit, wStat + 1, dwParam, dwParam2, dwParam3, *((DWORD *)&vValue.fY));
	UnitSetStat(unit, wStat + 2, dwParam, dwParam2, dwParam3, *((DWORD *)&vValue.fZ));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR StatsGetStatVector( 
	GAME * game,
	STATS * pStats,
	int wStat,
	PARAM dwParam)
{
	sCheckStatVector(game, pStats, wStat);
	DWORD dwX = StatsGetStat(pStats, wStat + 0, dwParam);
	DWORD dwY = StatsGetStat(pStats, wStat + 1, dwParam);
	DWORD dwZ = StatsGetStat(pStats, wStat + 2, dwParam);
	return sVectorize(dwX, dwY, dwZ);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsSetStatVector( 
	GAME * game,
	STATS * pStats,
	int wStat,
	PARAM dwParam,
	const VECTOR & vValue)
{
	sCheckStatVector(game, pStats, wStat);
	StatsSetStat(game, pStats, wStat + 0, dwParam, *((DWORD *)&vValue.fX));
	StatsSetStat(game, pStats, wStat + 1, dwParam, *((DWORD *)&vValue.fY));
	StatsSetStat(game, pStats, wStat + 2, dwParam, *((DWORD *)&vValue.fZ));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UnitGetStatFloat( 
	UNIT * unit,
	int wStat,
	PARAM dwParam)
{
#if !ISVERSION(OPTIMIZED_VERSION)
	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETZERO(StatsDataTestFlag(stats_data, STATS_DATA_FLOAT));
	ASSERT_RETZERO(!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE));
#endif
	DWORD dwVal = UnitGetStat(unit, wStat, dwParam);
	float fVal = *((float *)&dwVal);
	return fVal;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float StatsGetStatFloat( 
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam)
{
#if !ISVERSION(OPTIMIZED_VERSION)
	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETZERO(StatsDataTestFlag(stats_data, STATS_DATA_FLOAT));
	ASSERT_RETZERO(!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE));
#endif
	DWORD dwVal = StatsGetStat(stats, wStat, dwParam);
	float fVal = *((float *)&dwVal);
	return fVal;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetStatFloat( 
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	float fValue)
{
#if !ISVERSION(OPTIMIZED_VERSION)
	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETURN(StatsDataTestFlag(stats_data, STATS_DATA_FLOAT));
	ASSERT_RETURN(!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE));
#endif
	UnitSetStat(unit, wStat, dwParam, *((DWORD *)&fValue));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsSetStatFloat( 
	GAME * game,
	STATS * stats,
	int wStat,
	PARAM dwParam,
	float fValue)
{
#if !ISVERSION(OPTIMIZED_VERSION)
	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(StatsDataTestFlag(stats_data, STATS_DATA_FLOAT));
	ASSERT_RETURN(!StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE));
#endif
	StatsSetStat(game, stats, wStat, dwParam, *((DWORD *)&fValue));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GLOBAL_STATS_TRACKING
static int sStatsListGetTotal(
	GAME * game,
	STATS_LIST * list,
	const STATS_DATA * stats_data,
	int wStat)
#else
static int sStatsListGetTotal(
	STATS_LIST * list,
	const STATS_DATA * stats_data,
	int wStat)
#endif
{
	ASSERT_RETZERO(list);

	DWORD dwStatBeg = MAKE_STAT(wStat, 0);
	DWORD dwStatEnd = MAKE_STAT(wStat + 1, 0);

#if GLOBAL_STATS_TRACKING
	unsigned int ii = sFindStatInList(game, list, dwStatBeg);
#else
	unsigned int ii = sFindStatInList(list, dwStatBeg);
#endif
	STATS_ENTRY * cur = list->m_Stats + ii;
	STATS_ENTRY * end = list->m_Stats + list->m_Count;

	int total = 0;
	int offset = stats_data->m_nStatOffset;

	while (cur < end && cur->stat < dwStatEnd)
	{
		total += cur->value - offset;
		cur++;
	}

	return total;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetTotal(
	STATS * stats,
	int wStat)
{
	ASSERT_RETZERO(stats);
	STATS_TEST_MAGIC_RETVAL(stats, 0);

	const STATS_DATA * stats_data = StatsGetData(NULL, wStat);
	ASSERT_RETZERO(stats_data);

#if GLOBAL_STATS_TRACKING
	return sStatsListGetTotal(stats->m_Game, &stats->m_List, stats_data, wStat);
#else
	return sStatsListGetTotal(&stats->m_List, stats_data, wStat);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStatsTotal(
	struct UNIT * unit,
	int wStat)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETZERO(stats_data);

	if (StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
	{
#if GLOBAL_STATS_TRACKING
		return sStatsListGetTotal(UnitGetGame(unit), &unit->stats->m_Calc, stats_data, wStat);
#else
		return sStatsListGetTotal(&unit->stats->m_Calc, stats_data, wStat);
#endif
	}
	else
	{
#if GLOBAL_STATS_TRACKING
		return sStatsListGetTotal(UnitGetGame(unit), &unit->stats->m_Accr, stats_data, wStat);
#else
		return sStatsListGetTotal(&unit->stats->m_Accr, stats_data, wStat);
#endif
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetBaseStatsTotal(
	struct UNIT * unit,
	int wStat)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETZERO(stats_data);

#if GLOBAL_STATS_TRACKING
	return sStatsListGetTotal(UnitGetGame(unit), &unit->stats->m_List, stats_data, wStat);
#else
	return sStatsListGetTotal(&unit->stats->m_List, stats_data, wStat);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int StatsListGetEntries(
	const STATS * stats,
	STATS_ENTRY * statsentries,
	unsigned int size)
{
	ASSERT_RETZERO(statsentries && stats);

	const STATS_LIST * list = &stats->m_List;

	const STATS_ENTRY * cur = list->m_Stats;
	const STATS_ENTRY * end = cur + MIN(list->m_Count, size);

	while (cur < end)
	{
		const STATS_DATA * stats_data = StatsGetData(NULL, STAT_GET_STAT(cur->stat));
		statsentries->stat = cur->stat;
		statsentries->value = cur->value - stats_data->m_nStatOffset;
		++cur;
		++statsentries;
	}

	return (unsigned int)(cur - list->m_Stats);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsListIterate(
	const STATS * stats,
	PFN_STATSLIST_ITERATE pfnStatsListIterate,
	void *userdata)
{
	ASSERT_RETURN(stats);
	ASSERT_RETURN(pfnStatsListIterate);

	const STATS_LIST * list = &stats->m_List;

	for(unsigned int ii=0; ii < list->m_Count; ii++)
	{
		const STATS_ENTRY * pStatsEntry = &list->m_Stats[ii];
		pfnStatsListIterate(pStatsEntry, userdata);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if GLOBAL_STATS_TRACKING
inline unsigned int sStatsListGetRange(
	GAME * game,
	const STATS_LIST * list,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	STATS_ENTRY * statslist,
	unsigned int nStartingCount,
	unsigned int size)
#else
inline unsigned int sStatsListGetRange(
	const STATS_LIST * list,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	STATS_ENTRY * statslist,
	unsigned int nStartingCount,
	unsigned int size)
#endif
{
	ASSERT(list && statslist && stats_data);

	DWORD dwStatBeg = MAKE_STAT(wStat, dwParam);
	DWORD dwStatEnd = MAKE_STAT(wStat + 1, 0);

#if GLOBAL_STATS_TRACKING
	unsigned int ii = sFindStatInList(game, list, dwStatBeg);
#else
	unsigned int ii = sFindStatInList(list, dwStatBeg);
#endif
	if (ii >= list->m_Count)
	{
		return 0;
	}
	
	int offset = stats_data->m_nStatOffset;
	int max = MIN(list->m_Count - ii, size - nStartingCount);

	STATS_ENTRY * dst = statslist + nStartingCount;
	STATS_ENTRY * src = list->m_Stats + ii;
	STATS_ENTRY * end = src + max;

	while (src < end && src->stat < dwStatEnd)
	{
		dst->stat = src->stat;
		dst->value = src->value - offset;
		dst++;
		src++;
	}

	return (unsigned int)(dst - statslist);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sUnitGetCurOrAccrRange(
	const UNITSTATS * unitstats,
	int wStat,
	PARAM dwParam,
	const STATS_DATA * stats_data,
	STATS_ENTRY * statslist,
	unsigned int nStartingCount,
	unsigned int size)
{
	const STATS_LIST * list = sUnitGetCurOrAccrListConst(unitstats, stats_data);
#if GLOBAL_STATS_TRACKING
	return sStatsListGetRange(unitstats->m_Game, list, wStat, dwParam, stats_data, statslist, nStartingCount, size);
#else
	return sStatsListGetRange(list, wStat, dwParam, stats_data, statslist, nStartingCount, size);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStatsRange(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	STATS_ENTRY * statslist,
	int size)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETZERO(stats_data);

	return sUnitGetCurOrAccrRange(unit->stats, wStat, dwParam, stats_data, statslist, 0, size);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStatsRange(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	STATS_ENTRY * statslist,
	int nStartingCount,
	int size)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETZERO(stats_data);

	return sUnitGetCurOrAccrRange(unit->stats, wStat, dwParam, stats_data, statslist, nStartingCount, size);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStatsRangeBase(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	STATS_ENTRY * statslist,
	int size)
{
	ASSERT_RETZERO(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, 0);

	const STATS_DATA * stats_data = StatsGetData(UnitGetGame(unit), wStat);
	ASSERT_RETZERO(stats_data);

#if GLOBAL_STATS_TRACKING
	return sStatsListGetRange(UnitGetGame(unit), &unit->stats->m_List, wStat, dwParam, stats_data, statslist, 0, size);
#else
	return sStatsListGetRange(&unit->stats->m_List, wStat, dwParam, stats_data, statslist, 0, size);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetRange(
	STATS * stats,
	int wStat,
	PARAM dwParam,
	STATS_ENTRY * statslist,
	int size)
{
	ASSERT_RETFALSE(stats);
	STATS_TEST_MAGIC_RETVAL(stats, FALSE);

	const STATS_DATA * stats_data = StatsGetData(NULL, wStat);
	ASSERT_RETZERO(stats_data);

#if GLOBAL_STATS_TRACKING
	return sStatsListGetRange(stats->m_Game, &stats->m_List, wStat, dwParam, stats_data, statslist, 0, size);
#else
	return sStatsListGetRange(&stats->m_List, wStat, dwParam, stats_data, statslist, 0, size);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetRange(
	STATS * stats,
	int wStat,
	PARAM dwParam,
	STATS_ENTRY * statslist,
	int nStartingCount,
	int size)
{
	ASSERT_RETFALSE(stats);
	STATS_TEST_MAGIC_RETVAL(stats, FALSE);

	const STATS_DATA * stats_data = StatsGetData(NULL, wStat);
	ASSERT_RETZERO(stats_data);

#if GLOBAL_STATS_TRACKING
	return sStatsListGetRange(stats->m_Game, &stats->m_List, wStat, dwParam, stats_data, statslist, nStartingCount, size);
#else
	return sStatsListGetRange(&stats->m_List, wStat, dwParam, stats_data, statslist, nStartingCount, size);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsClearRange(
	GAME * game,
	STATS * stats,
	int wStat)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	ASSERT_RETURN(!StatsTestFlag(stats, STATSFLAG_RIDER));

	STATS_LIST * list = &(stats->m_List);

	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	BOOL bAccrue = StatsDataTestFlag(stats_data, STATS_DATA_ACCRUE);

	DWORD dwStat = MAKE_STAT(wStat, 0);
	DWORD dwStatEnd = MAKE_STAT(wStat + 1, 0);

	BOOL bSend = StatsTestFlag(stats, STATSFLAG_SENT_ON_ADD);

	BOOL bSendStatsListStat = !bSend && stats->m_Cloth && stats->m_Cloth->m_Unit;
	while (1)
	{
#if GLOBAL_STATS_TRACKING
		unsigned int ii = sFindStatInList(game, list, dwStat);
#else
		unsigned int ii = sFindStatInList(list, dwStat);
#endif
		if (ii >= list->m_Count || list->m_Stats[ii].stat >= dwStatEnd)
		{
			return;
		}

		DWORD stat = list->m_Stats[ii].stat;
		int delta = sSetStatInList(game, list, stat, stats_data, 0 - stats_data->m_nStatOffset);

		if (bSendStatsListStat)
		{
			sStatsSendStatsListStat(game, stats->m_Cloth->m_Unit, stats, stat, 0 - stats_data->m_nStatOffset);
		}

		if (bAccrue)
		{
			sAccrue<TRUE>(game, stats->m_Cloth, TRUE, stat, stats_data, delta, bSend);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sStatsClearRange(
	GAME * game,
	UNITSTATS * stats,
	int wStat)
{
	ASSERT_RETURN(game && stats);
	STATS_TEST_MAGIC_RETURN(stats);

	UNIT * unit = stats->m_Unit;
	ASSERT_RETURN(unit);

	STATS_LIST * list = &(stats->m_List);

	const STATS_DATA * stats_data = (STATS_DATA *)StatsGetData(game, wStat);
	ASSERT_RETURN(stats_data);

	DWORD dwStat = MAKE_STAT(wStat, 0);
	DWORD dwStatEnd = MAKE_STAT(wStat + 1, 0);

	while (1)
	{
#if GLOBAL_STATS_TRACKING
		unsigned int ii = sFindStatInList(game, list, dwStat);
#else
		unsigned int ii = sFindStatInList(list, dwStat);
#endif
		if (ii >= list->m_Count || list->m_Stats[ii].stat >= dwStatEnd)
		{
			return;
		}

		DWORD stat = list->m_Stats[ii].stat;

		sSetStatInUnitList<TRUE>(game, unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), stats_data, 0 - stats_data->m_nStatOffset, TRUE);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitClearStatsRange(
	UNIT * unit,
	int wStat)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETURN(game);

	UNITSTATS * stats = (UNITSTATS *)UnitGetStats(unit);
	ASSERT_RETURN(stats);

	sStatsClearRange(game, stats, wStat);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StatsGetStatAny(
	STATS * stats,
	int wStat,
	PARAM * pdwParam)
{
	STATS_ENTRY entry[1];
	if (!StatsGetRange(stats, wStat, 0, entry, 1))
	{
		if (pdwParam)
		{
			*pdwParam = 0;
		}
		return 0;
	}
	if (pdwParam)
	{
		*pdwParam = entry[0].GetParam();
	}
	return entry[0].value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UnitGetStatAny(
	UNIT * unit,
	int wStat,
	PARAM * pdwParam)
{
	STATS_ENTRY entry[1];
	if (!UnitGetStatsRange(unit, wStat, 0, entry, 1))
	{
		if (pdwParam)
		{
			*pdwParam = 0;
		}
		return 0;
	}
	if (pdwParam)
	{
		*pdwParam = entry[0].GetParam();
	}
	return entry[0].value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitStatTestBit(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	unsigned int bit)
{
	DWORD dwStat = (DWORD)UnitGetStat(unit, wStat, dwParam);
	return TESTBIT(dwStat, bit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitStatSetBit(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	unsigned int bit)
{
	DWORD dwStat = (DWORD)UnitGetStat(unit, wStat, dwParam);
	SETBIT(dwStat, bit, TRUE);
	UnitSetStat(unit, wStat, dwParam, dwStat);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitStatClearBit(
	UNIT * unit,
	int wStat,
	PARAM dwParam,
	unsigned int bit)
{
	DWORD dwStat = (DWORD)UnitGetStat(unit, wStat, dwParam);
	CLEARBIT(dwStat, bit);
	UnitSetStat(unit, wStat, dwParam, dwStat);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StatsTimerExpireCallback(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA& event_data)
{
	ASSERT_RETFALSE(unit && unit->stats);
	STATS_TEST_MAGIC_RETVAL(unit->stats, FALSE);
	ASSERT_RETFALSE(StatsTestFlag(unit->stats, STATSFLAG_UNITSTATS));

	DWORD idStats = (DWORD)event_data.m_Data1;
	STATS * modlist = unit->stats->m_Modlist;

	while (modlist && modlist->m_idStats != idStats)
	{
		modlist = modlist->m_Next;
	}
	
	if (!modlist)
	{
		return FALSE;
	}
	
	StatsListRemove(game, modlist);

	StatsListFree(game, modlist);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsExitLimboCallback(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETURN(unit && unit->stats);
	STATS_TEST_MAGIC_RETURN(unit->stats);
	ASSERT_RETURN(StatsTestFlag(unit->stats, STATSFLAG_UNITSTATS));

	GAME_TICK tick = UnitGetStat(unit, STATS_LIMBO_TIME);
	GAME_TICK offset = GameGetTick(game) - tick;

	STATS * modlist = unit->stats->m_Modlist;

	while (modlist)
	{
		modlist->m_tiTimer += offset;

		modlist = modlist->m_Next;
	}
	UnitChangeAllSkillCooldowns(game, unit, offset);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StatsEventUnregisterCheck(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data,
	const struct EVENT_DATA * check_data)
{
	return (DWORD)event_data.m_Data1 == (DWORD)check_data->m_Data1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsClearTimer(
	GAME * game,
	UNIT * cloth,
	STATS * stats)
{
	ASSERT_RETURN(stats && !StatsTestFlag(stats, STATSFLAG_UNITSTATS));
	STATS_TEST_MAGIC_RETURN(stats);
	ASSERT_RETURN(cloth && StatsGetParent(stats) == cloth);

	if (stats->m_tiTimer == 0)
	{
		return;
	}
	ASSERT(stats->m_tiTimer >= GameGetTick(game));
	EVENT_DATA check_data(stats->m_idStats);
	UnitUnregisterTimedEvent(cloth, StatsTimerExpireCallback, StatsEventUnregisterCheck, &check_data);
	stats->m_tiTimer = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsSetTimer(
	GAME * game,
	UNIT * cloth,
	STATS * stats,
	int lengthInTicks)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	ASSERT_RETURN(!StatsTestFlag(stats, STATSFLAG_UNITSTATS));
	ASSERT_RETURN(cloth && StatsGetParent(stats) == cloth);
	ASSERT_RETURN(lengthInTicks > 0);

	if (stats->m_tiTimer)
	{
		StatsClearTimer(game, cloth, stats);
	}
	stats->m_tiTimer = GameGetTick(game) + lengthInTicks;
	EVENT_DATA event_data(stats->m_idStats);
	UnitRegisterEventTimed(cloth, StatsTimerExpireCallback, &event_data, lengthInTicks);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int StatsGetTimer(
	GAME * game,
	STATS * stats)
{
	ASSERT_RETZERO(game);
	ASSERT_RETZERO(stats);
	STATS_TEST_MAGIC_RETVAL(stats, 0);
	ASSERT_RETZERO(!StatsTestFlag(stats, STATSFLAG_UNITSTATS));

	GAME_TICK tick = GameGetTick(game);
	if (stats->m_tiTimer <= tick)
	{
		return 0;
	}
	return (unsigned int)(stats->m_tiTimer - tick);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsSetRemoveCallback(
	GAME * game,
	STATS * stats,
	FP_STATSREMOVECALLBACK fpRemoveCallback)
{
	ASSERT_RETURN(stats);
	STATS_TEST_MAGIC_RETURN(stats);
	stats->m_fpRemoveCallback = fpRemoveCallback;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StatsListAddTimed(
	UNIT * cloth,
	STATS * dye,
	int selector,
	int filter,
	int lengthInTicks,
	FP_STATSREMOVECALLBACK fpRemoveCallback,
	BOOL bSend)
{
	ASSERT_RETINVALID(cloth && dye);
	STATS_TEST_MAGIC_RETVAL(dye, (DWORD)INVALID_ID);

	StatsListAdd(cloth, dye, bSend, selector, filter);
	StatsSetTimer(UnitGetGame(cloth), cloth, dye, lengthInTicks);
	StatsSetRemoveCallback(UnitGetGame(cloth), dye, fpRemoveCallback);
	return dye->m_idStats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StatsListGetId(
	const STATS * stats)
{
	ASSERT_RETINVALID(stats);
	return stats->m_idStats;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsListSetId(
	STATS * stats,
	DWORD dwId)
{
	ASSERT_RETURN(stats);
	stats->m_idStats = dwId;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsClearTimer(
	GAME * game,
	UNIT * cloth,
	int idStats)
{
	UnitUnregisterTimedEvent(cloth, StatsTimerExpireCallback, StatsEventUnregisterCheck, EVENT_DATA(idStats));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL IS_ACCR>
static void sStatsTransfer(
	UNIT * cloth,
	STATS_LIST * dye)
{
	GAME * game = UnitGetGame(cloth);
	ASSERT_RETURN(game);

	STATS_ENTRY * cur = dye->m_Stats;
	STATS_ENTRY * end = cur + dye->m_Count;
	
	int oldstat = INVALID_ID;

	const STATS_DATA * stats_data = NULL;
	for (; cur < end; ++cur)
	{
		int wStat = cur->GetStat();
		if (oldstat != wStat)
		{
			stats_data = StatsGetData(game, wStat);
			oldstat = wStat;
		}
		ASSERT_CONTINUE(stats_data);

		if (!StatsDataTestFlag(stats_data, STATS_DATA_TRANSFER))
		{
			continue;
		}
		if (IS_ACCR && StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
		{
			continue;
		}
		UnitChangeStat(cloth, cur->GetStat(), cur->GetParam(), cur->value);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsTransfer(
	UNIT * cloth,
	UNIT * dye)
{
	ASSERT_RETURN(cloth && cloth->stats);
	STATS_TEST_MAGIC_RETURN(cloth->stats);
	ASSERT_RETURN(dye && dye->stats);
	STATS_TEST_MAGIC_RETURN(dye->stats);

	sStatsTransfer<TRUE>(cloth, &dye->stats->m_Accr);
	sStatsTransfer<FALSE>(cloth, &dye->stats->m_Calc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL IS_ACCR>
static void sStatsTransferAdd(
	GAME * game,
	STATS * cloth,
	STATS_LIST * dye)
{
	STATS_ENTRY * cur = dye->m_Stats;
	STATS_ENTRY * end = cur + dye->m_Count;

	int oldstat = INVALID_ID;

	const STATS_DATA * stats_data = NULL;
	for (; cur < end; ++cur)
	{
		int wStat = cur->GetStat();
		if (oldstat != wStat)
		{
			stats_data = StatsGetData(game, wStat);
			oldstat = wStat;
		}
		ASSERT_CONTINUE(stats_data);

		if (!StatsDataTestFlag(stats_data, STATS_DATA_TRANSFER))
		{
			continue;
		}
		if (IS_ACCR && StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
		{
			continue;
		}
		StatsChangeStat(game, cloth, cur->GetStat(), cur->GetParam(), cur->value);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsTransferAdd(
	GAME * game, 
	STATS * cloth,
	UNIT * dye)
{
	ASSERT_RETURN(cloth);
	STATS_TEST_MAGIC_RETURN(cloth);
	ASSERT_RETURN(dye && dye->stats);
	STATS_TEST_MAGIC_RETURN(dye->stats);

	sStatsTransferAdd<TRUE>(game, cloth, &dye->stats->m_Accr);
	sStatsTransferAdd<FALSE>(game, cloth, &dye->stats->m_Calc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsTransferAdd(
	GAME * game,
	UNIT * cloth,
	STATS * dye)
{
	ASSERT_RETURN(cloth && cloth->stats);
	STATS_TEST_MAGIC_RETURN(cloth->stats);
	ASSERT_RETURN(dye);
	STATS_TEST_MAGIC_RETURN(dye);

	sStatsTransfer<FALSE>(cloth, &dye->m_List);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL IS_ACCR>
static void sStatsTransferSet(
	GAME * game,
	STATS * cloth,
	STATS_LIST * dye)
{
	STATS_ENTRY * cur = dye->m_Stats;
	STATS_ENTRY * end = cur + dye->m_Count;

	int oldstat = INVALID_ID;

	const STATS_DATA * stats_data = NULL;
	for (; cur < end; ++cur)
	{
		int wStat = cur->GetStat();
		if (oldstat != wStat)
		{
			stats_data = StatsGetData(game, wStat);
			oldstat = wStat;
		}
		ASSERT_CONTINUE(stats_data);

		if (!StatsDataTestFlag(stats_data, STATS_DATA_TRANSFER))
		{
			continue;
		}
		if (IS_ACCR && StatsDataTestFlag(stats_data, STATS_DATA_CALC_TARGET))
		{
			continue;
		}
		StatsSetStat(game, cloth, cur->GetStat(), cur->GetParam(), cur->value);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatsTransferSet(
	GAME * game, 
	STATS * cloth,
	UNIT * dye)
{
	ASSERT_RETURN(cloth);
	STATS_TEST_MAGIC_RETURN(cloth);
	ASSERT_RETURN(dye && dye->stats);
	STATS_TEST_MAGIC_RETURN(dye->stats);

	sStatsTransferSet<TRUE>(game, cloth, &dye->stats->m_Accr);
	sStatsTransferSet<FALSE>(game, cloth, &dye->stats->m_Calc);
}


//----------------------------------------------------------------------------
// check if requirements for a stat apply to the unit type of the container
//----------------------------------------------------------------------------
BOOL StatsCheckRequirementType(
	UNIT * container,
	const STATS_DATA * stats_data)
{
	ASSERT_RETFALSE(container);
	ASSERT_RETFALSE(stats_data);

	// check all requirement types for a match, note, however, that if there are no requirement types is the equivalent of all
	BOOL bHasRequirementType = FALSE;
	for (unsigned int ii = 0; ii < arrsize(stats_data->m_nReqirementTypes); ++ii)
	{
		if (stats_data->m_nReqirementTypes[ii] == INVALID_LINK)
		{
			continue;			// end of requirement types
		}
		if (UnitIsA(container, stats_data->m_nReqirementTypes[ii]))
		{
			return TRUE;	// found a match
		}
		bHasRequirementType = TRUE;
	}

	return (!bHasRequirementType);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void StatsVerifyNoSendStats(
	GAME * game,
	STATS * stats,
	const char * szStateName)
{
	ASSERT_RETURN(stats);

	for (unsigned int ii = 0; ii < stats->m_List.m_Count; ++ii)
	{
		const STATS_DATA * stats_data = StatsGetData(game, stats->m_List.m_Stats[ii].GetStat());
		ASSERT_CONTINUE(stats_data);
		ASSERTV((!StatsDataTestFlag(stats_data, STATS_DATA_SEND_TO_ALL) && !StatsDataTestFlag(stats_data, STATS_DATA_SEND_TO_SELF)), "State [%s] has stat [%s] which should be sent, but state is marked as don't send.", szStateName, stats_data->m_szName);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define IPUSH(v, b)					ASSERT_RETFALSE(buf.PushInt(v, b));
#define UPUSH(v, b)					ASSERT_RETFALSE(buf.PushUInt(v, b));
#define UPUSHSTATFL(v, b, f, l)		ASSERTX_RETFALSE(buf.PushUInt(v, b, f, l), stats_data->m_szName);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL WriteStats(
	GAME * game,
	STATS * stats,
	BIT_BUF & buf,
	STATS_WRITE_METHOD eMethod)
{
	XFER<XFER_SAVE> tXfer(&buf);
	ASSERT_RETFALSE(StatsXfer(game, stats, tXfer, eMethod));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ReadStats(
	GAME * game,
	STATS * stats,
	BIT_BUF & buf,
	STATS_WRITE_METHOD eMethod)
{
	XFER<XFER_LOAD> tXfer(&buf);
	ASSERT_RETFALSE(StatsXfer(game, stats, tXfer, eMethod));
	return TRUE;
}


//----------------------------------------------------------------------------
// linear search, client only
//----------------------------------------------------------------------------
inline int StatsGetNumRiders(
	UNIT * unit)
{
	ASSERT_RETZERO(unit);

	int count = 0;
	STATS * rider = UnitGetRider(unit, NULL);
	while (rider)
	{
		count++;
		rider = UnitGetRider(unit, rider);
	}
	return count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEvaluateDisplayCondition(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	const STATS_DISPLAY * display,
	DISPLAY_CONDITION_RESULTS * results,
	int count,
	UNIT * unit,
	STATS * rider)
{
	ASSERT_RETFALSE(display);

	BOOL * curr_results = NULL;
	BOOL * prev_results = NULL;

	if (results)
	{
		ASSERT_RETFALSE(count < results->count);
		ASSERT_RETFALSE(results->curr && results->prev);
		curr_results = results->curr + count * STATSDISPLAY_NUM_CONDITIONS;
		prev_results = results->prev + count * STATSDISPLAY_NUM_CONDITIONS;
	}

	for (int ii = 0; ii < STATSDISPLAY_NUM_CONDITIONS; ii++)
	{
		BOOL result = FALSE;

		int codelen = 0;
		BYTE * displaycondition = ExcelGetScriptCode(game, eDisplayTable, display->codeDisplayCondition[ii], &codelen);
		BOOL bIncludeUnit = !rider || display->bIncludeUnitInCondition[ii];
		UNIT * pUnit = bIncludeUnit ? unit : NULL;

		switch (display->nDisplayRule[ii])
		{
		default:	// TRUE if no display condition, otherwise evaluate display condition
			{
				if (!displaycondition)
				{
					result = TRUE;
					if (results)
					{
						curr_results[ii] = TRUE;
					}
				}
				else
				{
					result = (BOOL)VMExecI(game, pUnit, rider, displaycondition, codelen);
					if (results)
					{
						curr_results[ii] = result;
					}
				}
			}
			break;
		case 1:	// use prev result
			{
				ASSERT_RETFALSE(results);
				result = curr_results[ii] = prev_results[ii];
			}
			break;
		case 2: // evaluate as case 0, but only if prev result was FALSE (switch between all 2's in a row with first prev non-2)
			{
				ASSERT_RETFALSE(results);
				if (prev_results[ii])
				{
					curr_results[ii] = FALSE;
				}
				else
				{
					if (!displaycondition)
					{
						result = curr_results[ii] = TRUE;
					}
					else
					{
						result = curr_results[ii] = (BOOL)VMExecI(game, pUnit, rider, displaycondition, codelen);
					}
				}
			}
			break;
		case 3: // only one of.  Should be in a chain.  special case - if the previous is true, mark this one as true but return false, so the next in the chain will fail
			{
				ASSERT_RETFALSE(results);
				if (prev_results[ii])
				{
					curr_results[ii] = TRUE;
					return FALSE;
				}
				else
				{
					if (!displaycondition)
					{
						result = curr_results[ii] = TRUE;
					}
					else
					{
						result = curr_results[ii] = (BOOL)VMExecI(game, pUnit, rider, displaycondition, codelen);
					}
				}
			}
			break;
		}
		if (!result)
		{
			return FALSE;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int EvaluateStatConditions(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	int nDisplayArea,
	UNIT * unit,
	int * pnStatLines,
	STATS ** ppRiderStats,
	int nMaxStatLines,
	BOOL bPrintRider)
{
	ASSERT_RETFALSE(eDisplayTable == DATATABLE_CHARDISPLAY || eDisplayTable == DATATABLE_ITEMDISPLAY);
	ASSERT_RETFALSE(unit);
	
	int ridercount = StatsGetNumRiders(unit);
	LOCAL_OR_ALLOC_BUFFER<BOOL, STATSDISPLAY_NUM_CONDITIONS * STATSDISPLAY_MAX_LOCAL_BUFFER, TRUE> prealloc_prev(GameGetMemPool(game), ridercount);
	LOCAL_OR_ALLOC_BUFFER<BOOL, STATSDISPLAY_NUM_CONDITIONS * STATSDISPLAY_MAX_LOCAL_BUFFER, TRUE> prealloc_curr(GameGetMemPool(game), ridercount);

	DISPLAY_CONDITION_RESULTS results;
	results.count = ridercount + 1;
	results.prev = prealloc_prev.GetBuf();
	results.curr = prealloc_curr.GetBuf();

	int nResultsCount = 0;
	int table_size = ExcelGetNumRows(game, eDisplayTable);

	for (int ii = 0; ii < table_size; ii++)
	{
		const STATS_DISPLAY * display = (const STATS_DISPLAY *)ExcelGetData(game, eDisplayTable, ii);
		if (!display)
		{
			continue;
		}

		ppRiderStats[nResultsCount] = NULL;

		int count = 0;
		if (display->nTooltipArea == nDisplayArea)
		{
			if (sEvaluateDisplayCondition(game, eDisplayTable, display, &results, count, unit, NULL))
			{
				pnStatLines[nResultsCount] = ii;
				nResultsCount++;
				if (nResultsCount >= nMaxStatLines)
				{
					break;
				}
			}
			if (display->bRider && bPrintRider)
			{
				STATS * rider = UnitGetRider(unit, NULL);
				while (rider)
				{
					count++;
					if (sEvaluateDisplayCondition(game, eDisplayTable, display, &results, count, NULL, rider))
					{
						pnStatLines[nResultsCount] = ii;
						ppRiderStats[nResultsCount] = rider;
						nResultsCount++;
						if (nResultsCount >= nMaxStatLines)
						{
							break;
						}
					}
					rider = UnitGetRider(unit, rider);
				}
			}
		} 

		// swap prev & curr display condition results
		BOOL * temp = results.curr;
		results.curr = results.prev;
		results.prev = temp;
	}

	return nResultsCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EvaluateDisplayCondition(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	int nDisplayLine,
	DISPLAY_CONDITION_RESULTS * results,
	int count,
	UNIT * unit,
	STATS * rider)
{
	const STATS_DISPLAY * display = (const STATS_DISPLAY *)ExcelGetData(game, eDisplayTable, nDisplayLine);
	if (!display)
	{
		return FALSE;
	}

	return sEvaluateDisplayCondition(game, eDisplayTable, display, results, count, unit, rider);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPrintInventory(
	GAME * game,
	UNIT * unit,
	WCHAR * str,
	int len,
	const WCHAR * fmt,
	int location)
{
	ASSERT_RETFALSE(game && unit && str && fmt);

	BOOL bNewLine = FALSE;
	UNIT * item = NULL;
	while (TRUE)
	{
		item = UnitInventoryLocationIterate(unit, location, item);
		if (!item)
		{
			break;
		}
		WCHAR szName[256];
		if (!UnitGetName(item, szName, arrsize(szName), 0))
		{
			return FALSE;
		}
		if (bNewLine)
		{
			PStrCat(str, L"\n", len);
		}
		WCHAR szBuf[256];
		const WCHAR *uszValues[] = { szName };
		PStrPrintfTokenStringValues( 
			szBuf, 
			arrsize(szBuf), 
			fmt, 
			StringReplacementTokensGet( SR_DISPLAY_FORMAT_TOKEN ), 
			uszValues, 
			arrsize( uszValues ),
			FALSE);
			
		PStrCat(str, szBuf, len);
		bNewLine = TRUE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sFormatNumber(
	WCHAR *puszBuffer,
	int nMaxBuffer)
{
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nMaxBuffer > 0, "Invalid buffer size" );

	// storage for formatted string
	const int MAX_FORMATTED = STATSDISPLAY_MAX_VAL_LEN;
	WCHAR uszFormatted[ MAX_FORMATTED ] = { 0 };
	
	// format
	LanguageFormatNumberString( uszFormatted, MAX_FORMATTED, puszBuffer );
	
	// copy back to param
	PStrCopy( puszBuffer, uszFormatted, nMaxBuffer );	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sPrintStatsLine2(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	int nLine,
	const STATS_DISPLAY * display,
	UNIT * unit,
	STATS * stats,
	int cur_param,
	int cur_value,
	WCHAR * szLine,
	int len,
	BOOL bUseShortFormat = FALSE)
{
	ASSERT_RETFALSE(display);

	WCHAR szFmt[STATSDISPLAY_MAX_LINE_LEN];
	WCHAR szVal[STATSDISPLAY_NUM_VALUES][STATSDISPLAY_MAX_VAL_LEN];
	int	nVal[STATSDISPLAY_NUM_VALUES];
	szLine[0] = 0;

	int codelen = 0;
	BYTE * code = NULL;
	int value;

	for (int jj = 0; jj < STATSDISPLAY_NUM_VALUES; jj++)
	{
		nVal[jj] = 0;
		szVal[jj][0] = 0;
		
		WCHAR *szDest = szVal[jj];
		int nDestLen = arrsize(szVal[jj]);
			
		int nValueControl = display->peValueCtrl[jj];
		switch ( nValueControl )
		{
		case DISPLAY_VALUE_CONTROL_SCRIPT_VALUE_NOPRINT:		// just compute and store the number
			codelen = 0;
			code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
			if (!code)
			{
				continue;
			}
			nVal[jj] = value = VMExecI(game, unit, stats, code, codelen);
			break;
		case DISPLAY_VALUE_CONTROL_SCRIPT:		// "%d"
			codelen = 0;
			code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
			if (!code)
			{
				continue;
			}
			nVal[jj] = value = VMExecI(game, unit, stats, code, codelen);

			PStrPrintf(szDest, nDestLen, L"%d", value);
			break;
		case DISPLAY_VALUE_CONTROL_SCRIPT_PLUSMINUS:		// "+/-%d"
			codelen = 0;
			code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
			if (!code)
			{
				continue;
			}
			nVal[jj] = value = VMExecI(game, unit, stats, code, codelen);

			if (value >= 0)
			{
				PStrPrintf(szDest, nDestLen, L"+%d", value);
			}
			else
			{
				PStrPrintf(szDest, nDestLen, L"%d", value);
			}
			break;
		case DISPLAY_VALUE_CONTROL_SCRIPT_PLUSMINUS_NOZERO:		// "+/-%d"  (don't print 0)
			codelen = 0;
			code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
			if (!code)
			{
				continue;
			}
			nVal[jj] = value = VMExecI(game, unit, stats, code, codelen);
			if (value > 0)
			{
				PStrPrintf(szDest, nDestLen, L"+%d", value);
			}
			else if (value < 0)
			{
				PStrPrintf(szDest, nDestLen, L"%d", value);
			}
			else
			{
				PStrPrintf(szDest, nDestLen, L"", value);
			}
			break;
		case DISPLAY_VALUE_CONTROL_CREATED_BY:		// "created by"
		{
			//shows who created the item
			if (stats)
			{
				return FALSE;
			}
			if( !ItemGetCreatorsName( unit, szDest, nDestLen ) )
			{
				return FALSE;
			}
			break;
		}			
		case DISPLAY_VALUE_CONTROL_UNIT_NAME:		// "name"
		{
			if (stats)
			{
				return FALSE;
			}
			DWORD dwFlags = 0;
			if( !( AppIsTugboat() &&
				   ItemBelongsToAnyMerchant( unit ) &&
				   UnitIsGambler( ItemGetMerchant(unit) ) ) )
			{
				SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );
			}
			else
			{
				SETBIT( dwFlags, UNF_NAME_NO_MODIFICATIONS );				
			}
			SETBIT( dwFlags, UNF_INCLUDE_TITLE_BIT );	// used with player only. this displays titles for chars like "commander, etc" on non-NPCs.
			UnitGetName(unit, szDest, nDestLen, dwFlags);
			break;
		}		
		case DISPLAY_VALUE_CONTROL_UNIT_CLASS:		// "class"
			if (stats)
			{
				return FALSE;
			}
			UnitGetClassString(unit, szDest, nDestLen);
			break;
		case DISPLAY_VALUE_CONTROL_UNIT_GUILD:		// "unit guild"
			if (stats)
			{
				return FALSE;
			}
			else
			{
				GUILD_RANK eGuildRank = GUILD_RANK_INVALID;
				UnitGetPlayerGuildAssociationTag(unit, szDest, nDestLen, eGuildRank);
			}
			break;
		case DISPLAY_VALUE_CONTROL_STAT_PARAM_STRING:			// either a string lookup based on display->nDisplayControlStat's paramTable or the param as an int
		case DISPLAY_VALUE_CONTROL_STAT_PARAM_STRING_PLURAL:		// (plural) either a string lookup based on display->nDisplayControlStat's paramTable or the param as an int
			{
				const WCHAR * str = NULL;
				if (display->nDisplayControlStat >= 0)
				{
					const STATS_DATA * stats_data = StatsGetData(game, display->nDisplayControlStat);
					if (stats_data != NULL && stats_data->m_nParamTable[0] >= 0 && ExcelHasRowNameString(game, (EXCELTABLE)stats_data->m_nParamTable[0]))
					{
						// this is to pass these values down through to the color coding script function
						nVal[0] = stats_data->m_nParamTable[0];	
						nVal[1] = cur_param;

						if (nValueControl == DISPLAY_VALUE_CONTROL_STAT_PARAM_STRING)
						{
							str = ExcelGetRowNameString(EXCEL_CONTEXT(game), stats_data->m_nParamTable[0], cur_param);
						}
						else
						{
							str = ExcelGetRowNameString(EXCEL_CONTEXT(game), stats_data->m_nParamTable[0], cur_param, PLURAL);
						}
					}
				}
				if (str)
				{
					PStrPrintf(szDest, nDestLen, L"%s", str);
				}
				else
				{
					PStrPrintf(szDest, nDestLen, L"%d", cur_param);
				}
			}
			break;
		case DISPLAY_VALUE_CONTROL_STAT_VALUE:		// "%d"
			nVal[jj] = value = cur_value;
			PStrPrintf(szDest, nDestLen, L"%d", value);
			break;
		case DISPLAY_VALUE_CONTROL_PLUSMINUS:		// "+/-%d"
			nVal[jj] = value = cur_value;
			if (value >= 0)
			{
				PStrPrintf(szDest, nDestLen, L"+%d", value);
			}
			else
			{
				PStrPrintf(szDest, nDestLen, L"%d", value);
			}
			break;
		case DISPLAY_VALUE_CONTROL_PLUSMINUS_NOZERO:		// "+/-%d"  (don't print 0)
			nVal[jj] = value = cur_value;
			if (value > 0)
			{
				PStrPrintf(szDest, nDestLen, L"+%d", value);
			} 
			else if (value < 0)
			{
				PStrPrintf(szDest, nDestLen, L"%d", value);
			}
			else
			{
				PStrPrintf(szDest, nDestLen, L"", value);
			}
			break;
		case DISPLAY_VALUE_CONTROL_UNIT_TYPE:	// Unit type
			if (stats)
			{
				return FALSE;
			}
			UnitGetTypeString(unit, szDest, nDestLen, FALSE, NULL);
			break;
		case DISPLAY_VALUE_CONTROL_UNIT_TYPE_LASTONLY:	// Unit type, last value only
			if (stats)
			{
				return FALSE;
			}
			UnitGetTypeString(unit, szDest, nDestLen, TRUE, NULL);
			break;
		case DISPLAY_VALUE_CONTROL_UNIT_TYPE_QUALITY:		// unit type with rarity prepended
			{
				if (stats)
				{
					return FALSE;
				}
				
				// get color
				int nColor = UnitGetNameColor(unit);
								
				// get item type as string
				DWORD dwAttributesOfTypeString = 0;
				WCHAR szType[256];
				UnitGetTypeString(unit, szType, arrsize(szType), FALSE, &dwAttributesOfTypeString);

				// get quality string with [item] token
				WCHAR szQuality[256];
				DWORD dwUnitNameFlags = 0;
				SETBIT( dwUnitNameFlags, UNF_INCLUDE_ITEM_TOKEN_WITH_ITEM_QUALITY );
				SETBIT( dwUnitNameFlags, UNF_OVERRIDE_ITEM_NOUN_ATTRIBUTES );
				ItemGetQualityString(
					unit, 
					szQuality, 
					arrsize(szQuality), 
					dwUnitNameFlags,
					dwAttributesOfTypeString);  // NOTE that we're using string attributes of the item TYPE, not the item noun

				if (szQuality[0])
				{
					szDest[0] = L'\0';
					UIAddColorToString(szDest, (FONTCOLOR)nColor, nDestLen);
					PStrCat(szDest, szQuality, nDestLen);
					UIAddColorReturnToString(szDest, nDestLen);
					
					// replace [item] with the item type
					const WCHAR *puszItemToken = StringReplacementTokensGet( SR_ITEM );
					PStrReplaceToken( szDest, nDestLen, puszItemToken, szType );
					
				}
				else
				{
					PStrCopy(szDest, szType, nDestLen);
				}
			}
			break;
		case DISPLAY_VALUE_CONTROL_UNIT_ADDITIONAL:
			if (stats)
			{
				return FALSE;
			}
			if (unit->pUnitData->nAddtlDescripString == INVALID_LINK)
			{
				continue;
			}
			PStrCopy(szLine, StringTableGetStringByIndex(unit->pUnitData->nAddtlDescripString), len );
			return TRUE;
			break;
		case DISPLAY_VALUE_CONTROL_AFFIX:	// Affixes
			if (stats)
			{
				return FALSE;
			}
			ItemGetAffixListString(unit, szDest, nDestLen);
			break;
		case DISPLAY_VALUE_CONTROL_ITEM_UNIQUE:
			if (!unit)
			{
				return FALSE;
			}
			ItemUniqueGetBaseString(unit, szDest, nDestLen);			
			break;			
		case DISPLAY_VALUE_CONTROL_ITEM_INGREDIENT:
			if (!unit)
			{
				return FALSE;
			}
#if !ISVERSION(SERVER_VERSION)
			c_RecipeGetIngredientTooltip(unit, szDest, nDestLen);
#endif
			break;			
		case DISPLAY_VALUE_CONTROL_FLAVOR:	// flavor text
			if (!unit)
			{
				return FALSE;
			}
#if !ISVERSION(SERVER_VERSION)
			UnitGetFlavorTextString(unit, szLine, len);
#endif
			return TRUE;
		case DISPLAY_VALUE_CONTROL_SCRIPT_DIVIDE_BY_100:	// n div 100 as float
			{
				codelen = 0;
				code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
				if (!code)
				{
					continue;
				}
				nVal[jj] = value = VMExecI(game, unit, stats, code, codelen);
				if ((value % 100 == 0) || ABS(value) >= 1000)
				{
					PStrPrintf(szDest, nDestLen, L"%d", value/100);
				}
				else if ((value % 10 == 0) || ABS(value) >= 100)
				{
					PStrPrintf(szDest, nDestLen, L"%1.1f", (float)value / 100.0f);
				}
				else
				{
					PStrPrintf(szDest, nDestLen, L"%03.2f", (float)value / 100.0f);
				}
			}
			break;
		case DISPLAY_VALUE_CONTROL_SCRIPT_DIVIDE_BY_10:	// n div 10 as float
			{
				codelen = 0;
				code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
				if (!code)
				{
					continue;
				}
				nVal[jj] = value = VMExecI(game, unit, stats, code, codelen);
				if (ABS(value) % 10)
				{
					PStrPrintf(szDest, nDestLen, L"%.1f", (float)value / 10.0f);
				}
				else
				{
					PStrPrintf(szDest, nDestLen, L"%d", value/10);
				}
			}
			break;
		case DISPLAY_VALUE_CONTROL_SCRIPT_DIVIDE_BY_20:	// n div 20 as float
			{
				codelen = 0;
				code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
				if (!code)
				{
					continue;
				}
				nVal[jj] = value = VMExecI(game, unit, stats, code, codelen);
				if ((ABS(value)>>1) % 10)
				{
					PStrPrintf(szDest, nDestLen, L"%.1f", (float)value / 20.0f);
				}
				else
				{
					PStrPrintf(szDest, nDestLen, L"%d", value/20);
				}
			}
			break;
		case DISPLAY_VALUE_CONTROL_ITEM_QUALITY:		// item quality
			{
				if (stats)
				{
					return FALSE;
				}
				DWORD dwFlags = 0;
				SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );
				ItemGetQualityString(unit, szDest, nDestLen, dwFlags);
				break;
			}
		case DISPLAY_VALUE_CONTROL_CLASS_REQUIREMENTS:		// item class requirement(s)
			{
				if (!unit)
				{
					return FALSE;
				}
				ItemGetClassReqString(unit, szDest, nDestLen);
			}
			break;
		case DISPLAY_VALUE_CONTROL_FACTION_REQUIREMENTS:		// item faction requirement(s)
			{
				if (!unit)
				{
					return FALSE;
				}
				ItemGetFactionReqString(unit, szDest, nDestLen);
			}
			break;
		case DISPLAY_VALUE_CONTROL_SKILLGROUP:
			{
				if (unit->pUnitData->nSkillRef == INVALID_ID)
				{
					return FALSE;
				}
				const SKILL_DATA * skilldata = SkillGetData(game, unit->pUnitData->nSkillRef);
				if (!skilldata)
				{
					return FALSE;
				}
				const SKILLGROUP_DATA * skillGroup = (const SKILLGROUP_DATA *)ExcelGetData(game, DATATABLE_SKILLGROUPS, skilldata->pnSkillGroup[0]);
				if (skillGroup == NULL)
				{
					return FALSE;			
				}
				PStrPrintf(szDest, nDestLen, L"%s", StringTableGetStringByIndex(skillGroup->nDisplayString));
			}
			
			break;
		case DISPLAY_VALUE_CONTROL_AFFIX_PROPS_LIST:	// list of affixes and their properties
			{
				codelen = 0;
				code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
				BOOL bExtended = FALSE;
				if (code)
				{
					bExtended = VMExecI(game, unit, stats, code, codelen);
				}

				return ItemGetAffixPropertiesString(unit, szLine, len, bExtended);
			}

		case DISPLAY_VALUE_CONTROL_INV_ITEM_PROPS:	// list of affixes and their properties
			{
				codelen = 0;
				code = ExcelGetScriptCode(game, eDisplayTable, display->codeValue[jj], &codelen);
				if (!code)
				{
					continue;
				}
				int nLoc = VMExecI(game, unit, stats, code, codelen);
				return ItemGetInvItemsPropertiesString(unit, nLoc, szLine, len);
			}

		case DISPLAY_VALUE_CONTROL_ITEM_DMG_DESC:	// description of item's primary damage type
			{
				if (!UnitIsA(unit, UNITTYPE_ITEM))
					continue;

				const UNIT_DATA *pUnitData = UnitGetData(unit);
				ASSERT_CONTINUE(pUnitData);

				PStrCopy(szDest, StringTableGetStringByIndex(pUnitData->nDamageDescripString), nDestLen);
			}

		default:
			// do nothing
			break;
		}
		
		// do number formatting on the final string if requested
		sFormatNumber(szDest, nDestLen);
		
	}

	szFmt[0] = 0;
	if (bUseShortFormat)
	{
		if (display->nShortFormatString > INVALID_LINK)
		{
			PStrCopy(szFmt, StringTableGetStringByIndex(display->nShortFormatString), arrsize(szFmt));
		}
	}
	else
	{
		if (display->nFormatString > INVALID_LINK)
		{
			PStrCopy(szFmt, StringTableGetStringByIndex(display->nFormatString), arrsize(szFmt));
		}
	}

	switch (display->eDisplayFunc)
	{
		//----------------------------------------------------------------------------	
		case DISPLAY_FUNCTION_FORMAT:
		{
		
			// put values into array
			const WCHAR *uszValues[] = { szVal[0], szVal[1], szVal[2], szVal[3] };
			
			// print
			PStrPrintfTokenStringValues( 
				szLine, 
				len, 
				szFmt, 
				StringReplacementTokensGet( SR_DISPLAY_FORMAT_TOKEN ),
				uszValues, 
				arrsize( uszValues ),
				FALSE);
				
			break;
		}
		
		//----------------------------------------------------------------------------
		case DISPLAY_FUNCTION_INVENTORY:
		{
			if (!sPrintInventory(game, unit, szLine, len, szFmt, nVal[0]))
			{
				return FALSE;
			}
			break;
		}
	}

#if !ISVERSION(SERVER_VERSION)
	UITextStringReplaceFormattingTokens(szLine);
#endif// !ISVERSION(SERVER_VERSION)

	code = ExcelGetScriptCode(game, eDisplayTable, display->codeColor, &codelen);
	if (code)
	{
		DWORD dwResult = (DWORD)VMExecI(game, unit, stats, nVal[0], nVal[1], code, codelen);
		BYTE byColorCode = (BYTE)LOWORD(dwResult);
		BOOL bUse = (BOOL)HIWORD(dwResult);
		if (bUse)
		{
			// Prepend the color code to the string, and return color at the end
			WCHAR * szTempLine = (WCHAR *)MALLOCZ(NULL, sizeof(WCHAR) * len);
			UIColorCatString(szTempLine, len, (FONTCOLOR)byColorCode, szLine);
			PStrCopy(szLine, szTempLine, len);
			FREE(NULL, szTempLine);
		}

	}

	if (AppTestDebugFlag(ADF_TEXT_STATS_DISP_DEBUGNAME))
	{
		if (PStrLen(szLine) > 0 &&
			display->peValueCtrl[0] != DISPLAY_VALUE_CONTROL_AFFIX_PROPS_LIST &&
			display->peValueCtrl[0] != DISPLAY_VALUE_CONTROL_INV_ITEM_PROPS)
		{
			WCHAR szName[256];
			PStrCvt(szName, display->szDebugName, 256);
			PStrPrintf(szLine, len, L"%d: %s", nLine, szName);
			return TRUE;
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPrintStatsLine1(
	 GAME * game,
	 EXCELTABLE eDisplayTable, 
	 int nLine,
	 const STATS_DISPLAY * display,
	 DISPLAY_CONDITION_RESULTS * results,
	 int count,
	 UNIT * unit,
	 STATS * stats,
	 WCHAR * szLine,
	 int len)
{
	ASSERT_RETFALSE(display);

	BOOL found = FALSE;

	WCHAR szSubLine[STATSDISPLAY_MAX_LINE_LEN];

	switch (display->eDisplayControl)
	{
	case DISPLAY_CONTROL_SINGLE:
		if (sPrintStatsLine2(game, eDisplayTable, nLine, display, unit, stats, 0, 0, szSubLine, STATSDISPLAY_MAX_LINE_LEN))
		{
			found = TRUE;
			if (display->bNewLine && PStrLen(szLine))
			{
				PStrCat(szLine, L"\n", len);
			}
			PStrCat(szLine, szSubLine, len);
		}
		break;
	case DISPLAY_CONTROL_PARAM_RANGE:
		{
			ASSERT_BREAK(display->nDisplayControlStat >= 0);
			if (unit)
			{
				UNIT_ITERATE_STATS_RANGE(unit, display->nDisplayControlStat, stat_list, ii, 32)
				{
					if (sPrintStatsLine2(game, eDisplayTable, nLine, display, unit, stats, stat_list[ii].GetParam(), stat_list[ii].value, szSubLine, STATSDISPLAY_MAX_LINE_LEN))
					{
						found = TRUE;
						if (display->bNewLine && PStrLen(szLine))
						{
							PStrCat(szLine, L"\n", len);
						}
						PStrCat(szLine, szSubLine, len);
					}
				}
				UNIT_ITERATE_STATS_END;
			}
			else
			{
				STATS_ITERATE_STATS_RANGE(stats, display->nDisplayControlStat, stat_list, ii, 32)
				{
					if (sPrintStatsLine2(game, eDisplayTable, nLine, display, unit, stats, stat_list[ii].GetParam(), stat_list[ii].value, szSubLine, STATSDISPLAY_MAX_LINE_LEN))
					{
						found = TRUE;
						if (display->bNewLine && PStrLen(szLine))
						{
							PStrCat(szLine, L"\n", len);
						}
						PStrCat(szLine, szSubLine, len);
					}
				}
				UNIT_ITERATE_STATS_END;
			}

		}
		break;
	}

	return found;
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline static BOOL sPrintStatsLine(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	int nLine,
	const STATS_DISPLAY * display,
	DISPLAY_CONDITION_RESULTS * results,
	int count,
	UNIT * unit,
	STATS * stats,
	WCHAR * szLine,
	int len)
{
	ASSERT_RETFALSE(display);

	if (sEvaluateDisplayCondition(game, eDisplayTable, display, results, count, unit, stats))
	{
		return sPrintStatsLine1(game, eDisplayTable, nLine, display, results, count, unit, stats, szLine, len);
	}
	else
	{
		return FALSE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PrintStatsLine(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	int line,
	DISPLAY_CONDITION_RESULTS * results,
	UNIT * unit,
	WCHAR * szLine,
	int len,
	int nDisplayArea,
	BOOL bPrintRider)
{
	ASSERT_RETFALSE(eDisplayTable == DATATABLE_CHARDISPLAY || eDisplayTable == DATATABLE_ITEMDISPLAY);
	ASSERT_RETFALSE(unit);

	const STATS_DISPLAY * display = (const STATS_DISPLAY *)ExcelGetData(game, eDisplayTable, line);
	if (!display)
	{
		return FALSE;
	}
	if (nDisplayArea != -1 && 
		display->nTooltipArea != nDisplayArea)
	{
		return FALSE;
	}

	szLine[0] = 0;

	int count = 0;
	BOOL found = sPrintStatsLine(game, eDisplayTable, line, display, results, count, unit, NULL, szLine, len);

	if (display->bRider && bPrintRider)
	{
		STATS * rider = UnitGetRider(unit, NULL);
		while (rider)
		{
			count++;
			found |= sPrintStatsLine(game, eDisplayTable, line, display, results, count, unit, rider, szLine, len);
			rider = UnitGetRider(unit, rider);
		}
	}

	return found;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PrintStatsLineStatsOnly(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	int line,
	DISPLAY_CONDITION_RESULTS * results,
	STATS * stats,
	WCHAR * szLine,
	int len,
	int nDisplayArea)
{
	ASSERT_RETFALSE(eDisplayTable == DATATABLE_CHARDISPLAY || eDisplayTable == DATATABLE_ITEMDISPLAY);

	const STATS_DISPLAY * display = (const STATS_DISPLAY *)ExcelGetData(game, eDisplayTable, line);
	if (!display)
	{
		return FALSE;
	}
	if (nDisplayArea != -1 && 
		display->nTooltipArea != nDisplayArea)
	{
		return FALSE;
	}

	szLine[0] = 0;

	int count = 0;
	BOOL found = sPrintStatsLine(game, eDisplayTable, line, display, results, count, NULL, stats, szLine, len);

	return found;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PrintStatsLineIcon(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	int line,
	UNIT * unit,
	STATS * stats,
	WCHAR * szIconText,
	int nIconTextLen,
	WCHAR * szDescripText,
	int nDescripTextLen,
	CHAR * szIconFrame,
	int nIconFrameLen)
{
	ASSERT_RETFALSE(eDisplayTable == DATATABLE_CHARDISPLAY || eDisplayTable == DATATABLE_ITEMDISPLAY);
	ASSERT_RETFALSE(unit);

	BOOL found = FALSE;
	const STATS_DISPLAY * display = (const STATS_DISPLAY *)ExcelGetData(game, eDisplayTable, line);
	if (!display)
	{
		return FALSE;
	}

	szIconText[0] = 0;
	szDescripText[0] = 0;
	szIconFrame[0] = 0;

	if (sPrintStatsLine2(game, eDisplayTable, line, display, unit, stats, 0, 0, szIconText, nIconTextLen, TRUE))
	{
		found = TRUE;

		if (display->nShortDescripString > INVALID_LINK)
		{		
			GRAMMAR_NUMBER eNumber = SINGULAR;
			if (szIconText[0] && PStrToInt(szIconText) > 1)
			{
				eNumber = PLURAL;
			}
			DWORD dwAttributes = StringTableGetGrammarNumberAttribute( eNumber );		
			PStrCopy(
				szDescripText, 
				StringTableGetStringByIndexVerbose(display->nShortDescripString, dwAttributes, 0, NULL, NULL), 
				nDescripTextLen);
		}
		PStrCopy(szIconFrame, display->szIconFrame, nIconFrameLen);
	}

	return found;
}

//----------------------------------------------------------------------------
//this prints any stats that aren't assigned to the item( mod )
//----------------------------------------------------------------------------
BOOL PrintStatsForUnitTypes( 
	GAME * pGame,
	EXCELTABLE eDisplayTable, 
	int nDisplayArea,
	UNIT * unit,
	WCHAR * str,
	WCHAR * tmpStr,
	int len )
{
	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(unit);
	if( !UnitIsA( unit, UNITTYPE_MOD ) ) //only for mods right now
		return FALSE;
	if( UnitGetStat( unit, STATS_ITEM_PROPS_SET ) == 1 ) //the item already has the props set so the player doesn't get to choose any longer
		return FALSE;  

	BOOL bFound( FALSE );
	//not lets do this for any unique properties it shows for items
	int nClass = UnitGetClass(unit);
	static int UNITTYPES_CHECKAGAINST[] = { UNITTYPE_SHIELD,
											UNITTYPE_TRINKET,
											UNITTYPE_ARMOR,
											UNITTYPE_WEAPON };			
											 
	static GLOBAL_STRING PREFIX_STRINGS[] = { GS_SOCKETABLE_SHIELD,
											  GS_SOCKETABLE_TRINKET,
											  GS_SOCKETABLE_ARMOR,
											  GS_SOCKETABLE_WEAPON };

	
	for( int t = 0; t < NUM_UNIT_PROPERTIES; t++ )
	{						
		for( int nUnitTypes = 0; nUnitTypes < 4; nUnitTypes++ )
		{
			if( unit->pUnitData->nCodePropertiesApplyToUnitType[t] != INVALID_ID &&
				s_ItemPropIsValidForUnitType( t, unit->pUnitData, UNITTYPES_CHECKAGAINST[nUnitTypes] ) )
			{
				STATS *pStats = StatsListInit( pGame );
				
				ExcelEvalScript( pGame, unit, NULL, pStats, DATATABLE_ITEMS, (DWORD)OFFSET(UNIT_DATA, codeProperties[t]), nClass );
				tmpStr[0] = 0;
				if( PrintStats(pGame, eDisplayTable, nDisplayArea, NULL, tmpStr, len, pStats,GlobalStringGet( PREFIX_STRINGS[nUnitTypes] ) ) )
				{
					if( bFound )
					{
						PStrCat(str, L"\n", len);
					}
					bFound = TRUE;
					PStrCat( str, tmpStr, len );
				}
				StatsListRemove( pGame, pStats );
			}
		}
	}
	return bFound;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PrintStats(
	GAME * game,
	EXCELTABLE eDisplayTable, 
	int nDisplayArea,
	UNIT * unit,
	WCHAR * str,
	int maxlen,
	STATS * stats, /*= NULL*/
	const WCHAR * stringWrapper /*= NULL*/ )
{
	ASSERT_RETFALSE(maxlen > 0);
	// first clear out the provided buffer.  if it's filled with garbage, wscat will not find the end of it, 
	// and will add on the second string at some random place in memory
	memclear(str, sizeof(WCHAR) * maxlen);

	ASSERT_RETFALSE(eDisplayTable == DATATABLE_CHARDISPLAY || eDisplayTable == DATATABLE_ITEMDISPLAY);
	ASSERT_RETFALSE(unit || stats);
	
	int ridercount = (unit ? StatsGetNumRiders(unit) : 0);

	LOCAL_OR_ALLOC_BUFFER<BOOL, STATSDISPLAY_NUM_CONDITIONS * STATSDISPLAY_MAX_LOCAL_BUFFER, TRUE> prealloc_prev(GameGetMemPool(game), ridercount);
	LOCAL_OR_ALLOC_BUFFER<BOOL, STATSDISPLAY_NUM_CONDITIONS * STATSDISPLAY_MAX_LOCAL_BUFFER, TRUE> prealloc_curr(GameGetMemPool(game), ridercount);

	WCHAR szLine[STATSDISPLAY_MAX_LINE_LEN];

	DISPLAY_CONDITION_RESULTS results;
	results.count = ridercount + 1;
	results.prev = prealloc_prev.GetBuf();
	results.curr = prealloc_curr.GetBuf();

	int nRunningStrLen = 0;
	int table_size = ExcelGetNumRows(game, eDisplayTable);

	BOOL bFoundOne = FALSE;
	BOOL bOk = FALSE;
	WCHAR tmpStr[ MAX_STRING_ENTRY_LENGTH ];

	for (int ii = 0; ii < table_size; ii++)
	{
		if (unit == NULL && stats != NULL)
		{
			bOk = PrintStatsLineStatsOnly(game, eDisplayTable, ii, &results, stats, szLine, STATSDISPLAY_MAX_LINE_LEN, nDisplayArea);
		}
		else
		{
			bOk = PrintStatsLine(game, eDisplayTable, ii, &results, unit, szLine, STATSDISPLAY_MAX_LINE_LEN, nDisplayArea, TRUE);
		}

		if (bOk)
		{
			BOOL bNewLine = TRUE;
			const STATS_DISPLAY * display = (const STATS_DISPLAY *)ExcelGetData(game, eDisplayTable, ii);
			if (display && !display->bNewLine)
			{
				bNewLine = FALSE;
			}
			bFoundOne = TRUE;
			int nStrLen = PStrLen(szLine);
			if (nStrLen + nRunningStrLen < maxlen)
			{
				// add a carriage return between stats (if we have room)
				if (bNewLine &&
					nStrLen > 0 &&
					PStrLen(str) > 0 &&
					nStrLen + nRunningStrLen + 1 < maxlen)
				{
					PStrCat(str, L"\n", maxlen);
				}
				if( stringWrapper )
				{
					tmpStr[0] = 0;	
					PStrPrintf( tmpStr, MAX_STRING_ENTRY_LENGTH, stringWrapper, szLine );
					PStrCat( str, tmpStr, maxlen );
				}
				else
				{
					PStrCat(str, szLine, maxlen);
				}

				nRunningStrLen += nStrLen;
			}
			else
			{
				PStrCat(str, L"...\n", maxlen);
				break;
			}
		}
		// swap prev & curr display condition results
		BOOL * temp = results.curr;
		results.curr = results.prev;
		results.prev = temp;
	}

	return bFoundOne;
}
