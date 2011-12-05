//----------------------------------------------------------------------------
// FILE: statssave.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __STATSSAVE_H_
#define __STATSSAVE_H_

//----------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------
#include "stats.h"

//----------------------------------------------------------------------------
// Stats version tracking
//----------------------------------------------------------------------------
#define STATS_CURRENT_VERSION								10
#define STATS_MAP_NOBITFIELDS								9
#define STATS_VERSION_DIFFICULTY_TOO_HIGH					8
#define STATS_VERSION_MERCHANT_LAST_BROWSED_TICK_NO_PARAM	8
#define STATS_VERSION_NUM_SIMILAR_8BIT						7

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct DATABASE_UNIT;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum STAT_VERSION_RESULT
{
	SVR_INVALID = -1,
	
	SVR_OK,					// version process OK
	SVR_ERROR,				// an error occurred
	SVR_IGNORE,				// after versioning, we decided that we want to throw away this stat
	
	SVR_NUM_RESULTS			// keep this last please
	
};

//----------------------------------------------------------------------------
enum STAT_XFER_RESULT
{
	SXR_INVALID = -1,
	
	SXR_OK,					// xfer was ok
	SXR_ERROR,				// there was an error
	SXR_IGNORE,				// we want to ignore this stat and not add it to the stats list
	
	SXR_NUM_RESULTS			// keep this last
	
};

//----------------------------------------------------------------------------
enum STATS_FILE_PARAM_TYPE
{
	PARAM_TYPE_INVALID = -1,
	
	PARAM_TYPE_BY_CODE	= 0,	// param is saved by code - written into files - do not change!
	PARAM_TYPE_BY_VALUE = 1,	// param is saved by its direct value - written into files - do not change!
	// add new types here ... only at the bottom!
	
	NUM_PARAM_TYPE				// keep this last
};

//----------------------------------------------------------------------------
enum STATS_FILE_VALUE_TYPE
{
	VALUE_TYPE_INVALID = -1,
	
	VALUE_TYPE_BY_CODE	= 0,	// value is saved by code - written info files - do not change!
	VALUE_TYPE_BY_VALUE	= 1,	// value is save by its direct value - written info files - do not change!
	VALUE_TYPE_FLOAT	= 2,	// for float values - written info files - do not change!
	// add new types here ... only at the bottom!	
	
	NUM_VALUE_TYPE				// keep this last
};

//----------------------------------------------------------------------------
struct STATS_FILE_PARAM_VALUE
{
	unsigned int nValue;			// the value (either direct or by code according to eType)		
};

//----------------------------------------------------------------------------
struct STATS_PARAM_DESCRIPTION
{
	STATS_FILE_PARAM_TYPE eType;	// this param is either saved by value or by code
	unsigned int nParamBitsFile;	// number of bits to use when reading/writing to file
	unsigned int nParamShift;		// number of bits to shift the param by
	unsigned int nParamOffs;		// offset the param by this value
	unsigned int idParamTable;		// what the param means (which datatable it points to)
};

//----------------------------------------------------------------------------
struct STATS_FILE_VALUE
{
	unsigned int nValue; // the value (either direct or by code according to eType)	
	float flValue;		 // the direct float value (for float types)
};

//----------------------------------------------------------------------------
struct STATS_VALUE_DESCRIPTION
{
	STATS_FILE_VALUE_TYPE eType;
	unsigned int nValBits;			// # of bits used for value
	unsigned int nValShift;			// counter shift, because we don't need to send high precision to client
	unsigned int nValOffs;			// offset the value to encode to client
	unsigned int idValTable;		// what the value means (which datatable it points to)	
	unsigned int nStatOffset;		// stat offset
};

//----------------------------------------------------------------------------
struct STATS_DESCRIPTION
{

	int nStat;  // index of row in stats.xls
	DWORD dwCode;  // code in stats.xls
	
	// how the value is stored and interpreted		
	STATS_VALUE_DESCRIPTION tValueDescription;		
	
	// how the params are stored and interpreted
	unsigned int nParamCount;	// how many param descriptions are present
	STATS_PARAM_DESCRIPTION tParamDescriptions[ STATS_MAX_PARAMS ];	// descriptions of each param
	
};

//----------------------------------------------------------------------------
struct STATS_FILE
{	
	STATS_DESCRIPTION tDescription;  // data explaining the sizes and types of all the things to read/write this stat
	STATS_FILE_VALUE tValue;								// the value of the stat
	STATS_FILE_PARAM_VALUE tParamValue[ STATS_MAX_PARAMS ];	// the value of each param of this stat	
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

// read/write stats to/from stream
template <XFER_MODE mode>
BOOL StatsXfer(
	GAME *pGame,
	STATS *pStats,
	class XFER<mode> & tXfer,
	STATS_WRITE_METHOD eMethod,
	int *pnStatsVersion = NULL,
	UNIT *pUnit = NULL,
	DATABASE_UNIT *pDBUnit = NULL);

template BOOL StatsXfer<XFER_LOAD>(GAME *pGame, STATS *pStats, class XFER<XFER_LOAD> & tXfer, STATS_WRITE_METHOD eMethod, int *pnStatsVersion, UNIT *pUnit, DATABASE_UNIT *pDBUnit);
template BOOL StatsXfer<XFER_SAVE>(GAME *pGame, STATS *pStats, class XFER<XFER_SAVE> & tXfer, STATS_WRITE_METHOD eMethod, int *pnStatsVersion, UNIT *pUnit, DATABASE_UNIT *pDBUnit);

template <XFER_MODE mode>
BOOL StatsSelectorXfer(
	XFER<mode> & tXfer,
	STATS_SELECTOR & eSelector,
	STATS_WRITE_METHOD eStatsWriteMethod,
	int nStatsVersion);

template <XFER_MODE mode>
BOOL StatsListIdXfer(
	XFER<mode> & tXfer,
	DWORD & dwStatsListId,
	STATS_WRITE_METHOD eStatsWriteMethod,
	int nStatsVersion);

template BOOL StatsListIdXfer<XFER_LOAD>(class XFER<XFER_LOAD> & tXfer, DWORD & dwStatsListId, STATS_WRITE_METHOD eMethod, int nStatsVersion);
template BOOL StatsListIdXfer<XFER_SAVE>(class XFER<XFER_SAVE> & tXfer, DWORD & dwStatsListId, STATS_WRITE_METHOD eMethod, int nStatsVersion);

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern DWORD STREAM_BITS_STAT_INDEX;
extern BOOL gbStatXferLog;

#endif  // end __STATSSAVE_H_
