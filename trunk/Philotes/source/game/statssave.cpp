//----------------------------------------------------------------------------
// FILE: statssave.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "dbunit.h"
#include "stats.h"
#include "statspriv.h"
#include "statssave.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "units.h"
#if ISVERSION(SERVER_VERSION)
#include "statssave.cpp.tmh"
#endif



//----------------------------------------------------------------------------
// Local Globals
//----------------------------------------------------------------------------
BOOL gbStatXferLog = FALSE;
static const unsigned int PARAM_BITS_FOR_PAK = 32;
static const unsigned int VALUE_BITS_FOR_PAK = 32;
const DWORD STREAM_STAT_BITS_FILE =			BIT_SIZE(USHRT_MAX);	// max # of bits needed to stream stats to file (version agnostic)
DWORD STREAM_BITS_STAT_INDEX =				16;						// max # of bits needed to stream stats to net buffer

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsParamValueInit( 
	STATS_FILE_PARAM_VALUE &tParamValue)
{
	tParamValue.nValue = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsParamDescriptionInit(
	STATS_PARAM_DESCRIPTION &tParamDescription)
{
	tParamDescription.eType = PARAM_TYPE_INVALID;
	tParamDescription.nParamBitsFile = 0;
	tParamDescription.nParamShift = 0;
	tParamDescription.nParamOffs = 0;
	tParamDescription.idParamTable = (unsigned int)DATATABLE_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsFileValueInit( 
	STATS_FILE_VALUE &tValue)
{
	tValue.nValue = 0;
	tValue.flValue = 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsValueDescriptionInit( 
	STATS_VALUE_DESCRIPTION &tValueDescription)
{
	tValueDescription.eType = VALUE_TYPE_INVALID;
	tValueDescription.nValBits = 0;
	tValueDescription.nValShift = 0;
	tValueDescription.nValOffs = 0;
	tValueDescription.idValTable = (unsigned int)DATATABLE_NONE;
	tValueDescription.nStatOffset = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsDescriptionInit(
	STATS_DESCRIPTION &tDescription)
{

	tDescription.nStat = INVALID_LINK;
	tDescription.dwCode = INVALID_CODE;
	sStatsValueDescriptionInit( tDescription.tValueDescription );
	tDescription.nParamCount = 0;
	for (int i = 0 ; i < STATS_MAX_PARAMS; ++i)
	{
		STATS_PARAM_DESCRIPTION &tParamDescription = tDescription.tParamDescriptions[ i ];
		sStatsParamDescriptionInit( tParamDescription );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSetStatsDescription( 
	STATS_DESCRIPTION &tDescription,
	int nStat,
	STATS_WRITE_METHOD eMethod)
{
	const STATS_DATA *pStatsData = StatsGetData( NULL, nStat );
	ASSERTX_RETFALSE( pStatsData, "Expected stats data" );

	// init the description struct to clean state
	sStatsDescriptionInit( tDescription );
	
	// set code and index
	tDescription.nStat = nStat;
	tDescription.dwCode = pStatsData->m_wCode;

	// param info
	tDescription.nParamCount = pStatsData->m_nParamCount;
	for (unsigned int i = 0; i < pStatsData->m_nParamCount; ++i)
	{
		STATS_PARAM_DESCRIPTION &tParamDescription = tDescription.tParamDescriptions[ i ];
		
		// save param bits file
		if (eMethod == STATS_WRITE_METHOD_NETWORK)
		{
			tParamDescription.nParamBitsFile = pStatsData->m_nParamBits[ i ];			// actual bits
		}
		else if (eMethod == STATS_WRITE_METHOD_PAK)
		{
			// because pStatsData->m_nParamBitsFile[ i ] has not yet been initialized when 
			// writing stats to the pak file because it's not done until all excel tables are loaded		
			tParamDescription.nParamBitsFile = PARAM_BITS_FOR_PAK;  
		}
		else
		{
			tParamDescription.nParamBitsFile = pStatsData->m_nParamBitsFile[ i ];		// file bits
		}

		// bits for shift and offset
		tParamDescription.nParamShift = pStatsData->m_nParamShift[ i ];
		tParamDescription.nParamOffs = pStatsData->m_nParamOffs[ i ];
				
		// how the value of the param is interpreted
		if (pStatsData->m_nParamTable[ i ] != INVALID_LINK && eMethod != STATS_WRITE_METHOD_NETWORK)
		{
			// save value by code
			tParamDescription.idParamTable = (EXCELTABLE)pStatsData->m_nParamTable[ i ];			
			tParamDescription.eType = PARAM_TYPE_BY_CODE;
		}
		else
		{
			tParamDescription.eType = PARAM_TYPE_BY_VALUE;
		}
		
	}

	// value description
	STATS_VALUE_DESCRIPTION &tValueDescription = tDescription.tValueDescription;
	tValueDescription.nValOffs = pStatsData->m_nValOffs;
	tValueDescription.nValShift = pStatsData->m_nValShift;
	tValueDescription.nStatOffset = pStatsData->m_nStatOffset;
	if (eMethod == STATS_WRITE_METHOD_NETWORK)
	{
		tValueDescription.nValBits = pStatsData->m_nValBits;		// actual bits
	}
	else if (eMethod == STATS_WRITE_METHOD_PAK)
	{
		// because pStatsData->m_nValBitsFile has not yet been initialized when 
		// writing stats to the pak file because it's not done until all excel tables are loaded			
		tValueDescription.nValBits = VALUE_BITS_FOR_PAK;
	}
	else
	{
		tValueDescription.nValBits = pStatsData->m_nValBitsFile;	// file bits
	}
		
	// the value
	if (StatsDataTestFlag( pStatsData, STATS_DATA_FLOAT ))
	{
		tValueDescription.eType = VALUE_TYPE_FLOAT;
		ASSERTX_RETFALSE( tValueDescription.nValBits == STREAM_BITS_FLOAT, "Float values must be 32 bit" );
	}
	else if (StatsDataTestFlag( pStatsData, STATS_DATA_SAVE_VAL_BY_CODE ) && eMethod != STATS_WRITE_METHOD_NETWORK)
	{
		tValueDescription.eType = VALUE_TYPE_BY_CODE;
		tValueDescription.idValTable = (EXCELTABLE)pStatsData->m_nValTable;		
	}
	else
	{
		tValueDescription.eType = VALUE_TYPE_BY_VALUE;
	}

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sMustRunVersionFunction(
	const STATS_DATA *pStatsData,
	const STATS_FILE &tStatsFile,
	STATS_VERSION nStatsVersion,
	BOOL *pbVersionFunctionRequired)
{
	ASSERTX_RETFALSE( pStatsData, "Expected stats data" );
	ASSERTX_RETFALSE( pbVersionFunctionRequired, "Expected bool param" );

	// we do not require a version function by default
	*pbVersionFunctionRequired = FALSE;
		
	// we will run versioning if the version number has changed	
	/*
	if (nStatsVersion != STATS_CURRENT_VERSION)
	{
		return TRUE;
	}
	*/

	// now a version function is required
	*pbVersionFunctionRequired = TRUE;
	
	// if the value is a datatable, it must be the same
	if (tStatsFile.tDescription.tValueDescription.eType == VALUE_TYPE_BY_CODE ||
		StatsDataTestFlag( pStatsData, STATS_DATA_SAVE_VAL_BY_CODE ))
	{
		if ((int)tStatsFile.tDescription.tValueDescription.idValTable != pStatsData->m_nValTable)
		{
			return TRUE;
		}
	}
		
	// if the params of a stat have changed, we will need to version
	if (pStatsData->m_nParamCount != tStatsFile.tDescription.nParamCount)
	{
		return TRUE;
	}
	else
	{
	
		// if any of the params were datatables that changed we must run the versioning
		for (unsigned int i = 0; i < pStatsData->m_nParamCount; ++i)
		{
			const STATS_PARAM_DESCRIPTION *pParamDesc = &tStatsFile.tDescription.tParamDescriptions[ i ];
			if ((int)pParamDesc->idParamTable != pStatsData->m_nParamTable[ i ])
			{
				return TRUE;
			}			
			
		}
		
	}

	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCompareStatsDescriptions( 
	const STATS_DATA *pStatsData,
	const STATS_DESCRIPTION &tSource,
	const STATS_DESCRIPTION &tDest)
{

	// could do a byte by byte compare here, but not all fields are critical
	// and but it's useful in the debugger to know which element was different

	// basic stat info
	ASSERTV_RETFALSE( tSource.nStat == tDest.nStat, "'%s' Version stat mismatch (%d,%d)", pStatsData->m_szName, tSource.nStat, tDest.nStat );
	ASSERTV_RETFALSE( tSource.dwCode == tDest.dwCode , "'%s' Version stat code mismatch (%d,%d)", pStatsData->m_szName, tSource.dwCode, tDest.dwCode );

	// value info
	const STATS_VALUE_DESCRIPTION &tValueDescSource = tSource.tValueDescription;
	const STATS_VALUE_DESCRIPTION &tValueDescDest = tDest.tValueDescription;
	ASSERTV_RETFALSE( tValueDescSource.idValTable == tValueDescDest.idValTable, "'%s' Version stat value table mismatch (%d,%d)", pStatsData->m_szName, tValueDescSource.idValTable, tValueDescDest.idValTable );
	
	// param info
	ASSERTV_RETFALSE( tSource.nParamCount == tDest.nParamCount, "'%s' Version stat param count mismatch (%d,%d)", pStatsData->m_szName, tSource.nParamCount, tDest.nParamCount );
	for (unsigned int i = 0; i < tSource.nParamCount; ++i)
	{
		const STATS_PARAM_DESCRIPTION &tSourceParamDesc = tSource.tParamDescriptions[ i ];
		const STATS_PARAM_DESCRIPTION &tDestParamDesc = tDest.tParamDescriptions[ i ];
		ASSERTV_RETFALSE( tSourceParamDesc.idParamTable == tDestParamDesc.idParamTable, "'%s' Version stat param datatable mismatch (%d,%d)", pStatsData->m_szName, tSourceParamDesc.idParamTable, tDestParamDesc.idParamTable );
	}

	return TRUE;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STAT_VERSION_RESULT sConvertStatsFileToCurrentVersion( 
	STATS_FILE &tStatsFile, 
	STATS_VERSION nStatsVersion,
	UNIT *pUnit,
	STATS_WRITE_METHOD eMethod)
{
	int nStat = tStatsFile.tDescription.nStat;
	const STATS_DATA *pStatsData = StatsGetData( NULL, nStat );
	if (pStatsData)
	{
	
		// should we run the version function
		BOOL bVersionFunctionRequired = FALSE;
		BOOL bRunVersion = sMustRunVersionFunction( pStatsData, tStatsFile, nStatsVersion, &bVersionFunctionRequired );
		
		// run the version function
		if (bRunVersion)
		{

			// we must have a version function
			if (bVersionFunctionRequired)
			{
				ASSERTV_RETVAL( 
					pStatsData->pfnVersion, 
					SVR_ERROR,
					"Stat '%s' has changed and requires a version function.  This is required if a stat changes how it is interpreted, like adding/removing params, changing what a param or stat value means, etc.",
					pStatsData->m_szName);
			}
						
			// run the version function
			if (pStatsData->pfnVersion)
			{
			
				// run function
				STAT_VERSION_RESULT eResult = pStatsData->pfnVersion( tStatsFile, nStatsVersion, pUnit );
				
				// see if we just want to ignore it from here on out
				if (eResult == SVR_IGNORE)
				{
					return SVR_IGNORE;
				}
				
				// the stat must have been versioned all the way up to the current version now
				ASSERTV_RETVAL( 
					nStatsVersion == STATS_CURRENT_VERSION,
					SVR_ERROR,
					"Stat '%s' version (%d) was not versioned all the way up to the current version (%d)",
					pStatsData->m_szName,
					nStatsVersion,
					STATS_CURRENT_VERSION);

				// validate that the stats file description is the same as one we'd use for the current version
				STATS_DESCRIPTION tDescCurrent;
				sSetStatsDescription( tDescCurrent, nStat, eMethod );
				if (sCompareStatsDescriptions( pStatsData, tStatsFile.tDescription, tDescCurrent ) == FALSE)
				{			
					ASSERTV( 0, "Stat '%s' description not properly versioned to current version", pStatsData->m_szName );
					
					// in development, we bail out, but in production, we try to keep going
					#if ISVERSION(DEVELOPMENT)
						return SVR_ERROR;
					#endif
					
				}

				// now that we have verified that the most critical pieces of the descriptions are
				// up to the current version, make it super correct by getting the current description
				// (we do this just in case something after this function needs to look at it and assumes
				// that it represents the current version data)
				sSetStatsDescription( tStatsFile.tDescription, nStat, eMethod );
				
			}
			
		}
				
	}	

	return SVR_OK;
	
}

//----------------------------------------------------------------------------
enum STATS_OPTIONAL_PARAM_ELEMENT
{
	// note that the # of bits of the param value is *not* an optional element here
	OPE_SHIFT,		// param contains shift info
	OPE_OFFSET,		// param contains offset info
};

//----------------------------------------------------------------------------
enum STATS_OPTIONAL_ELEMENT
{
	// note that the # of bits of the stat value is *not* an optional element here
	SOE_VALUE_SHIFT,		// value has shift info
	SOE_VALUE_OFFSET,		// value has offset info
	SOE_STAT_OFFSET,		// stat has offset info
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsFileInitValues(
	STATS_FILE &tStatsFile)
{		

	// value
	sStatsFileValueInit( tStatsFile.tValue );
	
	// each param
	for (int i = 0; i < STATS_MAX_PARAMS; ++i)
	{
		STATS_FILE_PARAM_VALUE &tParamValue = tStatsFile.tParamValue[ i ];
		sStatsParamValueInit( tParamValue );
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static STAT_XFER_RESULT sStatsFileXfer( 
	STATS_FILE &tStatsFile,
	XFER<mode> &tXfer,
	STATS_VERSION nStatsVersion,
	STATS_WRITE_METHOD eMethod,
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit)
{
	STATS_FILE_VALUE &tValue = tStatsFile.tValue;
	const STATS_DESCRIPTION &tDescription = tStatsFile.tDescription;
		
	// init stats file when reading so that we're clean
	if (tXfer.IsLoad())
	{
		sStatsFileInitValues( tStatsFile );
	}

	// debug name to trace
	const char *szStatsName = "";
	const STATS_DATA *pStatsDataNameDebug = NULL;
	if (tDescription.nStat != INVALID_LINK)
	{
		pStatsDataNameDebug = StatsGetData( NULL, tDescription.nStat );	
		ASSERTX_RETVAL( pStatsDataNameDebug, SXR_ERROR, "Expected stats data" );	
		szStatsName = pStatsDataNameDebug->m_szName;
	}

	// do each param	
	for (unsigned int i = 0; i < tDescription.nParamCount; ++i)
	{
		
		// get param value and init when loading
		STATS_FILE_PARAM_VALUE &tParamValue = tStatsFile.tParamValue[ i ];
		if (tXfer.IsLoad())
		{
			sStatsParamValueInit( tParamValue );
		}
		
		// is any data present
		const STATS_PARAM_DESCRIPTION &tParamDescription = tDescription.tParamDescriptions[ i ];		
		if (tParamDescription.nParamBitsFile != 0)
		{
			STATS_FILE_PARAM_VALUE &tParamValue = tStatsFile.tParamValue[ i ];
			
			// finally do the param value itself
			XFER_UINTX_GOTO( tXfer, tParamValue.nValue, tParamDescription.nParamBitsFile, STATS_XFER_ERROR, szStatsName, 0 );
			
		}
										
	}

	// finally do the value itself
	const STATS_DATA *pStatsData = StatsGetData( NULL, tDescription.nStat );
	ASSERTX_RETVAL( pStatsData, SXR_ERROR, "Expected stats data" );
	const STATS_VALUE_DESCRIPTION &tValueDescription = tDescription.tValueDescription;
	if (tValueDescription.eType == VALUE_TYPE_FLOAT)
	{
		ASSERTX_RETVAL( tValueDescription.nValBits == STREAM_BITS_FLOAT, SXR_ERROR, "Expected 32 bits for floating pointer number" );
		
		// support integers that are stored directly as fields in db units
		if (pStatsData->m_eDBUnitField != DBUF_FULL_UPDATE && pDBUnit)
		{
			tValue.flValue = s_DBUnitGetFieldValueFloat( pDBUnit, pStatsData->m_eDBUnitField );
		}

		// xfer value		
		XFER_FLOATX_GOTO( tXfer, tValue.flValue, STATS_XFER_ERROR, szStatsName, 0 );
		
		// return to db unit if necessary
		if (pStatsData->m_eDBUnitField != DBUF_FULL_UPDATE && pDBUnit)
		{
			s_DBUnitSetFieldValueFloat( pDBUnit, pStatsData->m_eDBUnitField, tValue.flValue );
		}
		
	}
	else
	{	
		// support integers that are stored directly as fields in db units
		if (pStatsData->m_eDBUnitField != DBUF_FULL_UPDATE && pDBUnit)
		{
			tValue.nValue = s_DBUnitGetFieldValueDword( pDBUnit, pStatsData->m_eDBUnitField );
		}
		
		// xfer value
		XFER_UINTX_GOTO( tXfer, tValue.nValue, tValueDescription.nValBits, STATS_XFER_ERROR, szStatsName, 0 );

		if (pStatsData->m_eDBUnitField != DBUF_FULL_UPDATE && pDBUnit)
		{
			tValue.nValue = s_DBUnitGetFieldValueDword( pDBUnit, pStatsData->m_eDBUnitField );
		}
	}

	// now provide an opportunity for people to custom convert a stat from one version to another
	STAT_VERSION_RESULT eVersionResult = SVR_OK;	
	if (tXfer.IsLoad() && eMethod != STATS_WRITE_METHOD_NETWORK)
	{
		eVersionResult = sConvertStatsFileToCurrentVersion( tStatsFile, nStatsVersion, pUnit, eMethod );
	}

	if (gbStatXferLog == TRUE)
	{
		const STATS_DATA *pStatsData = NULL;
		if (tDescription.nStat != INVALID_LINK)
		{
			pStatsData = StatsGetData( NULL, tDescription.nStat );
		}
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf(
			szMessage,
			MAX_MESSAGE,
			"sStatsFileXfer() - #%d   %s=",
			tDescription.nStat,
			pStatsData ? pStatsData->m_szName : "Unknown");
			
		// scratch pad
		const int MAX_BUFFER = 256;
		char szBuffer[ MAX_BUFFER ];
		
		// value of stat
		if (tValueDescription.idValTable != INVALID_LINK)
		{
			ASSERT( tValueDescription.eType == VALUE_TYPE_BY_CODE );
			int nLine = ExcelGetLineByCode( NULL, tValueDescription.idValTable, tValue.nValue );
			const char *szName = ExcelGetStringIndexByLine( NULL, tValueDescription.idValTable, nLine );
			PStrPrintf( szBuffer, MAX_BUFFER, "'%s'", szName );				
			
		}
		else
		{
			if (tValueDescription.eType == VALUE_TYPE_BY_VALUE)
			{			
				int nValue = ((tValue.nValue - tValueDescription.nValOffs) << tValueDescription.nValShift) - tValueDescription.nStatOffset;
				PStrPrintf( szBuffer, MAX_BUFFER, "'%d'", nValue );
			}
			else
			{
				ASSERT( tValueDescription.eType == VALUE_TYPE_FLOAT );
				PStrPrintf( szBuffer, MAX_BUFFER, "'%f'", tValue.flValue );
			}
		}
		PStrCat( szMessage, szBuffer, MAX_MESSAGE );
		
		// params				
		if (tDescription.nParamCount > 0)
		{	
		
			// param header with count
			PStrPrintf( szBuffer, MAX_BUFFER, "       %d Params = (", tDescription.nParamCount );
			PStrCat( szMessage, szBuffer, MAX_MESSAGE );
			
			// param values
			for (unsigned int i = 0; i < tDescription.nParamCount; ++i)
			{
				const STATS_PARAM_DESCRIPTION &tParamDescription = tDescription.tParamDescriptions[ i ];
				const STATS_FILE_PARAM_VALUE &tParamValue = tStatsFile.tParamValue[ i ];
				if (tParamDescription.idParamTable != INVALID_LINK)
				{
					ASSERT( tParamDescription.eType == PARAM_TYPE_BY_CODE );
					int nLine = ExcelGetLineByCode( NULL, tParamDescription.idParamTable, tParamValue.nValue );
					const char *szName = ExcelGetStringIndexByLine( NULL, tParamDescription.idParamTable, nLine );
					PStrPrintf( szBuffer, MAX_BUFFER, i > 0 ? ", %s" : "%s", szName );				
				}
				else
				{
					int nParamValue = (tParamValue.nValue - tParamDescription.nParamOffs) << tParamDescription.nParamShift;
					PStrPrintf( szBuffer, MAX_BUFFER, i > 0 ? ", %d" : "%d", nParamValue );
				}
				PStrCat( szMessage, szBuffer, MAX_MESSAGE );
			}
			PStrCat( szMessage, ")", MAX_MESSAGE );
		}
		PStrCat( szMessage, "\n", MAX_MESSAGE );
		trace( szMessage );
				
	}

	// return result code of xfer operation to caller
	if (eVersionResult == SVR_IGNORE)
	{
		// after versioning this stat, we decided we didn't want it at all
		return SXR_IGNORE;
	}

	// successful stat xfer	
	return SXR_OK;

STATS_XFER_ERROR:

	return SXR_ERROR;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanXferStat(
	const STATS_DATA *pStatsData,
	STATS_WRITE_METHOD eMethod)
{
	ASSERTX_RETFALSE( pStatsData, "Expected stats data" );
	
	switch (eMethod)
	{
	
		//----------------------------------------------------------------------------
		case STATS_WRITE_METHOD_FILE:
		case STATS_WRITE_METHOD_DATABASE:
		{
			if (!StatsDataTestFlag( pStatsData, STATS_DATA_SAVE) || pStatsData->m_nValBitsFile <= 0)
			{
				return FALSE;
			}			
			break;
		}
		
		//----------------------------------------------------------------------------		
		case STATS_WRITE_METHOD_PAK:
		{
			// apparently we can always write stats to a pak, which makes sense I suppose ;)
			break;
		}
		
		//----------------------------------------------------------------------------
		case STATS_WRITE_METHOD_NETWORK:
		default:
		{
			if (pStatsData->m_nValBits <= 0)
			{
				return FALSE;  // no data size to write, forget it
			}
			if ( !(StatsDataTestFlag(pStatsData, STATS_DATA_SEND_TO_SELF) || 
				   StatsDataTestFlag(pStatsData, STATS_DATA_SEND_TO_ALL)) )
			{
				return FALSE;  // stat cannot be sent "to self" or "to all", forget it
			}
			break;
		}
		
	}	
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStatsFileAddToStatList(
	GAME *pGame,
	STATS *pStats,
	const STATS_FILE &tStatsFile)
{
	const STATS_DESCRIPTION &tDescription = tStatsFile.tDescription;
	
	// throw away stats that no longer exist
	if (tDescription.nStat == INVALID_LINK)
	{
		return TRUE;
	}

	// get stats data
	const STATS_DATA *pStatsData = StatsGetData( NULL, tDescription.nStat );
	ASSERTX_RETFALSE( pStatsData, "Expected stats data" );
	BOOL bInvalidStat = FALSE;
	
	//
	// CDay: Nice in theory to throw away, but because we don't accurately re-create all 
	// the important things on units (like all item properties such as quantity max) and rely
	// on what was saved ... we can't do this throwing away just yet	
	//
	//// throw away stats that we no longer save
	//if (StatsDataTestFlag( pStatsData, STATS_DATA_SAVE ) == FALSE)
	//{
	//	return TRUE;
	//}
	//
	
	// go through each param
	int nParam = 0;				
	for (unsigned int i = 0; i < tDescription.nParamCount; ++i)
	{
		const STATS_PARAM_DESCRIPTION &tParamDescription = tDescription.tParamDescriptions[ i ];
		
		// if has any data from file
		if (tParamDescription.nParamBitsFile > 0)
		{
			const STATS_FILE_PARAM_VALUE &tParamValue = tStatsFile.tParamValue[ i ];
			
			int nParamValue = 0;
			BOOL bInvalidParam = FALSE;						
			if (tParamDescription.eType== PARAM_TYPE_BY_CODE)
			{
				if (tParamDescription.idParamTable == DATATABLE_NONE)
				{
					// this stat used to reference a datatable that no longer exists
					bInvalidParam = TRUE;
				}
				else
				{
					nParamValue = ExcelGetLineByCode( NULL, tParamDescription.idParamTable, tParamValue.nValue );
					if( nParamValue == INVALID_LINK)
					{
						bInvalidParam = TRUE;
					}
				}
			}
			else
			{
				nParamValue = (tParamValue.nValue - tParamDescription.nParamOffs) << tParamDescription.nParamShift;
			}
			
			// if we have an invalid param, we have an invalid stat
			if (bInvalidParam == TRUE)
			{
				bInvalidStat = TRUE;
			}
			
			// incorporate param
			if (bInvalidParam == FALSE)
			{
				nParam = StatAddParam( pStatsData, nParam, i, nParamValue);
			}
		
		}
		
	}

	// get value
	const STATS_VALUE_DESCRIPTION &tValueDescription = tDescription.tValueDescription;
	const STATS_FILE_VALUE &tValue = tStatsFile.tValue;
	int nValue = 0;
	if (tValueDescription.eType == VALUE_TYPE_BY_CODE)
	{
		if (tValueDescription.idValTable == DATATABLE_NONE)
		{
			// stat references a datatable that no longer exists, forget it
			bInvalidStat = TRUE;
		}
		else
		{
			nValue = ExcelGetLineByCode( NULL, tValueDescription.idValTable, tValue.nValue );
		}
	}
	else
	{
		nValue = ((tValue.nValue - tValueDescription.nValOffs) << tValueDescription.nValShift) - tValueDescription.nStatOffset;				
	}
					
	// whew, finally set the stat
	if (bInvalidStat == FALSE)
	{
		TraceExtraVerbose( TRACE_FLAG_GAME_MISC_LOG, 
			"Set stat: %s  param:%d  value:%d", 
			pStatsData->m_szName, nParam, nValue);
		StatsSetStat( pGame, pStats, tDescription.nStat, nParam, nValue );
	}
		
	return TRUE;
	
}			

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sStatsDescriptionXfer( 
	STATS_DESCRIPTION &tDescription, 
	XFER<mode> &tXfer, 
	int nStatsVersion, 
	STATS_WRITE_METHOD eMethod)
{
	
	// init stats description when reading so that we're clean
	if (tXfer.IsLoad())
	{
		sStatsDescriptionInit( tDescription );
	}
	
	// stat code and index
	const STATS_DATA *pStatsData = NULL;
	if (eMethod == STATS_WRITE_METHOD_NETWORK)
	{
		unsigned int nStatToXfer = (unsigned int)tDescription.nStat;
		XFER_UINTX( tXfer, nStatToXfer, STREAM_BITS_STAT_INDEX, NULL, 0 );
		tDescription.nStat = (int)nStatToXfer;
		ASSERTX_RETFALSE( tDescription.nStat > INVALID_LINK, "Cannot send stat that doesn't exist!" );		
		pStatsData = StatsGetData( NULL, tDescription.nStat );
		ASSERTV_RETFALSE( pStatsData, "Expected stats data (%d)", tDescription.nStat );	
		tDescription.dwCode = pStatsData->m_wCode;  // hook up code just to be complete
	}
	else
	{
		// xfer code
		XFER_DWORD( tXfer, tDescription.dwCode, STREAM_BITS_STAT_CODE );

		// look up index
		tDescription.nStat = ExcelGetLineByCode( NULL, DATATABLE_STATS, tDescription.dwCode );		
	}

	// debug name to trace
	const char *szStatsName = "";
	const STATS_DATA *pStatsDataNameDebug = NULL;
	if (tDescription.nStat != INVALID_LINK)
	{
		pStatsDataNameDebug = StatsGetData( NULL, tDescription.nStat );	
		ASSERTV_RETFALSE( pStatsDataNameDebug, "Expected stats data", tDescription.nStat );	
		szStatsName = pStatsDataNameDebug->m_szName;
		ASSERT(pStatsDataNameDebug->m_wCode == tDescription.dwCode);
	}
//trace ("stats file xfer '%s'\n", szStatsName );
		
	// num params
	if (eMethod == STATS_WRITE_METHOD_NETWORK)
	{
		// not writing to file, use the latest version in memory
		ASSERTX_RETFALSE( pStatsData, "Expected stats data" );
		tDescription.nParamCount = pStatsData->m_nParamCount;
	}
	else
	{
		XFER_UINTX( tXfer, tDescription.nParamCount, STREAM_BITS_STAT_PARAM_COUNT, szStatsName, 0);	
	}

	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "Xfer stat %s with %d params for xfer %I64x",
		szStatsName, tDescription.nParamCount, (UINT64)(&tXfer));

	// do each param
	for (unsigned int i = 0; i < tDescription.nParamCount; ++i)
	{
		STATS_PARAM_DESCRIPTION &tParamDescription = tDescription.tParamDescriptions[ i ];
		
		// is any data present
		BOOL bParamDescDataPresent = tParamDescription.nParamBitsFile != 0;
		XFER_BOOLX( tXfer, bParamDescDataPresent, szStatsName, 0 );			
		
		// continue with rest of data if we have it
		if (bParamDescDataPresent)
		{
			
			// xfer param info
			if (eMethod == STATS_WRITE_METHOD_NETWORK)
			{
			
				// just get param info from stats data
				ASSERTX_RETFALSE( pStatsData, "Expected stats data" );
				tParamDescription.nParamBitsFile = pStatsData->m_nParamBits[ i ];  // actual bits
				tParamDescription.nParamShift = pStatsData->m_nParamShift[ i ];
				tParamDescription.nParamOffs = pStatsData->m_nParamOffs[ i ];
				
				// type is always by value for network
				tParamDescription.eType = PARAM_TYPE_BY_VALUE;
				
			}
			else
			{
			
				// param bits
				XFER_UINTX( tXfer, tParamDescription.nParamBitsFile, STREAM_BITS_STAT_PARAM_NUM_BITS_FILE, szStatsName, 0 );
				
				// shift and offset values, we first write a value that says if there is even param shift/offset
				// data to follow because the vast majority of params do not have this information and we can
				// save that space
				DWORD dwParamElements = 0;
				if (tParamDescription.nParamShift != 0)
				{
					SETBIT( dwParamElements, OPE_SHIFT );
				}
				if (tParamDescription.nParamOffs != 0)
				{
					SETBIT( dwParamElements, OPE_OFFSET );
				}
				XFER_DWORDX( tXfer, dwParamElements, STREAM_BITS_STAT_PARAM_OPTIONAL_ELEMENTS, szStatsName, 0 );
				
				// not xfer each param optional element if present
				if (TESTBIT( dwParamElements, OPE_SHIFT ))
				{
					XFER_UINTX( tXfer, tParamDescription.nParamShift, STREAM_BITS_STAT_PARAM_NUM_BITS_SHIFT, szStatsName, 0 );
				}
				if (TESTBIT( dwParamElements, OPE_OFFSET ))
				{
					XFER_UINTX( tXfer, tParamDescription.nParamOffs, STREAM_BITS_STAT_PARAM_NUM_BITS_OFFSET, szStatsName, 0 );
				}
				
				// xfer type
				XFER_ENUMX( tXfer, tParamDescription.eType, STREAM_BITS_STAT_PARAM_VALUE_TYPE, szStatsName, 0 );		
				
			}
						
			// when xfering by code, we need to know what table the code is for
			ASSERTX_RETFALSE( tParamDescription.eType > PARAM_TYPE_INVALID && tParamDescription.eType < NUM_PARAM_TYPE, "Invalid stat param type" );
			if (tParamDescription.eType == PARAM_TYPE_BY_CODE)
			{
				// need to xfer the excel table this is a code for
				DWORD dwTableCode = INVALID_CODE;
				if (tXfer.IsSave())
				{
					dwTableCode = ExcelTableGetCode( tParamDescription.idParamTable );
				}
				XFER_DWORDX( tXfer, dwTableCode, STREAM_BITS_EXCEL_TABLE_CODE, szStatsName, 0 );
				if (tXfer.IsSave())
				{
					ASSERTX_RETFALSE( tParamDescription.idParamTable != DATATABLE_NONE, "Invalid datatable for stat param value" );
				}
				else
				{
					tParamDescription.idParamTable = ExcelTableGetByCode( dwTableCode );
				}
			}
			
		}
										
	}

	// get value description struct
	STATS_VALUE_DESCRIPTION	&tValueDescription = tDescription.tValueDescription;

	// xfer value info
	if (eMethod == STATS_WRITE_METHOD_NETWORK)
	{
	
		ASSERTX_RETFALSE( pStatsData, "Expected stats data" );
		
		// just use values from stats data since we're guaranteed it's the same version
		tValueDescription.nValBits = pStatsData->m_nValBits;  // use real bits, not file bits
		tValueDescription.nValShift = pStatsData->m_nValShift;
		tValueDescription.nValOffs = pStatsData->m_nValOffs;
		tValueDescription.nStatOffset = pStatsData->m_nStatOffset;
		
		// always by value
		if (StatsDataTestFlag( pStatsData, STATS_DATA_FLOAT ))
		{
			tValueDescription.eType = VALUE_TYPE_FLOAT;		// always by float value
		}
		else
		{
			tValueDescription.eType = VALUE_TYPE_BY_VALUE;	// always by value
		}
		
	}
	else
	{
	
		// value bits (every stat must have this)
		XFER_UINTX( tXfer, tValueDescription.nValBits, STREAM_BITS_STAT_VALUE_NUM_BITS_FILE, szStatsName, 0 );
		
		// optional values, we first write a set of bits that says what kind of optional information 
		// follows because the vast majority of stats do not have this information and we can save space
		DWORD dwOptionElements = 0;
		if (tValueDescription.nValShift != 0)
		{
			SETBIT( dwOptionElements, SOE_VALUE_SHIFT );
		}
		if (tValueDescription.nValOffs != 0)
		{
			SETBIT( dwOptionElements, SOE_VALUE_OFFSET );
		}
		if (tValueDescription.nStatOffset != 0)
		{
			SETBIT( dwOptionElements, SOE_STAT_OFFSET );
		}
		XFER_DWORDX( tXfer, dwOptionElements, STREAM_BITS_STAT_OPTIONAL_ELEMENTS, szStatsName, 0 );
		
		// not xfer each param optional element if present
		if (TESTBIT( dwOptionElements, SOE_VALUE_SHIFT ))
		{
			XFER_UINTX( tXfer, tValueDescription.nValShift, STREAM_BITS_STAT_VALUE_NUM_BITS_SHIFT, szStatsName, 0 );
		}
		if (TESTBIT( dwOptionElements, SOE_VALUE_OFFSET ))
		{
			XFER_UINTX( tXfer, tValueDescription.nValOffs, STREAM_BITS_STAT_VALUE_NUM_BITS_OFFSET, szStatsName, 0 );		
		}
		if (TESTBIT( dwOptionElements, SOE_STAT_OFFSET ))
		{
			XFER_UINTX( tXfer, tValueDescription.nStatOffset, STREAM_BITS_STAT_NUM_BITS_OFFSET, szStatsName, 0 );
		}
				
		// value type
		XFER_ENUMX( tXfer, tValueDescription.eType, STREAM_BITS_STAT_VALUE_TYPE, szStatsName, 0 );
		ASSERTX_RETFALSE( tValueDescription.eType > VALUE_TYPE_INVALID && tValueDescription.eType < NUM_VALUE_TYPE, "Invalid stat value type" );

	}
		
	// when xfering by code, we need to know the excel table the code is for
	if (tValueDescription.eType == VALUE_TYPE_BY_CODE)
	{
		// need to xfer the excel table this is a code for
		DWORD dwTableCode = INVALID_CODE;
		if (tXfer.IsSave())
		{
			dwTableCode = ExcelTableGetCode( tValueDescription.idValTable );
		}
		XFER_DWORDX( tXfer, dwTableCode, STREAM_BITS_EXCEL_TABLE_CODE, szStatsName, 0 );
		if (tXfer.IsSave())
		{
			ASSERTX_RETFALSE( tValueDescription.idValTable != DATATABLE_NONE, "Invalid data table for stat value" );
		}
		else
		{
			tValueDescription.idValTable = ExcelTableGetByCode( dwTableCode );
		}
	}
						
	return TRUE;	

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetStatsParamValue( 
	STATS_FILE_PARAM_VALUE &tParamValue, 
	const STATS_PARAM_DESCRIPTION &tParamDescription,
	STATS_ENTRY *pStat,
	int nParamIndex,
	STATS_WRITE_METHOD eMethod)
{
	unsigned int nStat = STAT_GET_STAT( pStat->stat );
	const STATS_DATA *pStatsData = StatsGetData( NULL, nStat );
	unsigned int nParam = StatGetParam( pStatsData, pStat->stat, nParamIndex );					

	// init param value
	sStatsParamValueInit( tParamValue );
		
	// the value of the param
	if (tParamDescription.idParamTable != INVALID_LINK && eMethod != STATS_WRITE_METHOD_NETWORK)
	{
		// save value by code
		tParamValue.nValue = ExcelGetCode( NULL, tParamDescription.idParamTable, nParam );
	}
	else
	{
		tParamValue.nValue = (nParam >> tParamDescription.nParamShift) + tParamDescription.nParamOffs;
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetStatsFileValue( 
	STATS_FILE &tStatsFile, 
	const STATS_DESCRIPTION &tDescription, 
	STATS_ENTRY *pStat,
	STATS_WRITE_METHOD eMethod)
{

	// save description in stats file
	tStatsFile.tDescription = tDescription;
	
	// set each param value
	for (unsigned int i = 0; i < tDescription.nParamCount; ++i)
	{
		const STATS_PARAM_DESCRIPTION &tParamDescription = tDescription.tParamDescriptions[ i ];
		STATS_FILE_PARAM_VALUE &tParamValue = tStatsFile.tParamValue[ i ];
		sSetStatsParamValue( tParamValue, tParamDescription, pStat, i, eMethod );
	}
	
	// set the stat value
	const STATS_VALUE_DESCRIPTION &tValueDescription = tDescription.tValueDescription;
	STATS_FILE_VALUE &tValue = tStatsFile.tValue;
	sStatsFileValueInit( tValue );
	switch (tValueDescription.eType)
	{	
		case VALUE_TYPE_BY_CODE:
		{
			tValue.nValue = ExcelGetCode(NULL, tValueDescription.idValTable, pStat->value - tValueDescription.nStatOffset);
			break;
		}
		case VALUE_TYPE_BY_VALUE:
		{
			tValue.nValue = (pStat->value >> tValueDescription.nValShift) + tValueDescription.nValOffs;			
			break;
		}
		case VALUE_TYPE_FLOAT:
		{
			tValue.flValue = (float)pStat->value;
			break;
		}

	}	

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sDoStatsListXfer(
	GAME *pGame,
	STATS *pStats,
	XFER<mode> &tXfer,
	STATS_VERSION nStatsVersion,
	unsigned int &nNumStats,
	STATS_WRITE_METHOD eMethod,
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit)
{

	if (tXfer.IsSave())
	{
	
		STATS_ENTRY *pStatFirst = pStats->m_List.m_Stats;
		STATS_ENTRY *pStatCurrent = pStatFirst;
		STATS_ENTRY *pStatEnd = pStatCurrent + pStats->m_List.m_Count;

		for (; pStatCurrent < pStatEnd ; ++pStatCurrent)
		{
			const STATS_DATA *pStatsData = StatsGetData( NULL, pStatCurrent->GetStat() );
			ASSERTX_RETFALSE( pStatsData, "Expected stats data" );
			
			// skip stats that we cant xfer
			if (sCanXferStat( pStatsData, eMethod ) == FALSE)
			{
				continue;
			}

			// fill out a stats description struct that describes how we will
			int nStat = pStatCurrent->GetStat();
			STATS_DESCRIPTION tDescription;
			if (sSetStatsDescription( tDescription, nStat, eMethod ) == FALSE)
			{
				ASSERTV_RETFALSE( 0, "Error setting stat description '%s'", pStatsData->m_szName );
			}
			
			// xfer the description
			if (sStatsDescriptionXfer( tDescription, tXfer, nStatsVersion, eMethod ) == FALSE)
			{
				ASSERTV_RETFALSE( 0, "Error saving stat description '%s'", pStatsData->m_szName );
			}

			// we now have one or more stat to xfer, find out how many we have to xfer by
			// scanning the stats entries until we get to the next stat type.
			// note that this assumes that all the similar stats are organized in memory
			// as next to each other in a continuous block which peter has so wonderfully
			// already done for us cause its fast 
			unsigned int nStatCurrent = pStatCurrent->GetStat();			
			STATS_ENTRY *pStatSimilar = pStatCurrent;			
			pStatSimilar++;  // go to next entry
			unsigned int nNumSimilar = 1;
			while (pStatSimilar < pStatEnd)
			{
				unsigned int nStatOther = STAT_GET_STAT( pStatSimilar->stat );
				if (nStatOther != nStatCurrent)
				{
					break;
				}
				nNumSimilar++;
				pStatSimilar++;
			}
			
			// write if we have more than 1 similar stat, we do this because the vast
			// majority of stats has no params, so it will be just one value, but the
			// space savings from having similar stats saved like this for the ones
			// that do have params is signifigant
			BOOL bHasSimilarCount = nNumSimilar > 1;
			XFER_BOOLX( tXfer, bHasSimilarCount, pStatsData->m_szName, 0 );			
			if (bHasSimilarCount)
			{
			
				// write the number of similar stats to follow
				XFER_UINTX( tXfer, nNumSimilar, STREAM_BITS_SIMILAR_STAT_COUNT_10BIT, pStatsData->m_szName, 0 );

			}
						
			// go through each similar stat of this type
			for (unsigned int i = 0; i < nNumSimilar; ++i)
			{
			
				// fill out a stat file struct
				STATS_FILE tStatsFile;
				sSetStatsFileValue( tStatsFile, tDescription, pStatCurrent, eMethod );
				
				// xfer the stat file value struct
				if (sStatsFileXfer( tStatsFile, tXfer, nStatsVersion, eMethod, pUnit, pDBUnit ) == SXR_ERROR)
				{
					ASSERTV_RETFALSE( 0, "Error saving stat '%s'", pStatsData->m_szName );
				}
								
				// go to next stat entry if not at end ... we don't do it for the last one
				// because pStatCurrent is incremented as a part of the outer for loop we're in
				if (i < nNumSimilar - 1)
				{
					++pStatCurrent;
				}
				
			}
							
			// we're written another one
			nNumStats++;
					
		}
	
	}
	else
	{

		for (unsigned int i = 0; i < nNumStats; ++i)
		{
		
			// read description of stat
			STATS_DESCRIPTION tDescription;
			if (sStatsDescriptionXfer( tDescription, tXfer, nStatsVersion, eMethod ) == FALSE)
			{
				ASSERTV_RETFALSE( 0, "Error reading stats description for stat #%d of %d", i, nNumStats );
			}
			const STATS_DATA *pStatsData = tDescription.nStat != INVALID_LINK ? StatsGetData( NULL, tDescription.nStat ) : NULL;
			REF( pStatsData );
					
			TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "UNIT:%p  read stats description: %s", pUnit, pStatsData->m_szName);

			// how many similar stats are here to read
			unsigned int nNumSimilar = 1;
			BOOL bHasSimilarCount = FALSE;
			XFER_BOOL( tXfer, bHasSimilarCount );
			if (bHasSimilarCount)
			{
			
				int nBits = STREAM_BITS_SIMILAR_STAT_COUNT_10BIT;
				if (nStatsVersion <= STATS_VERSION_NUM_SIMILAR_8BIT)
				{
					nBits = STREAM_BITS_SIMILAR_STAT_COUNT_8BIT;
				}
				XFER_UINT( tXfer, nNumSimilar, nBits );
			}
			
			// loop through and read all similar stat values
			STATS_FILE tStatsFile;			
			for (unsigned int i = 0; i < nNumSimilar; ++i)
			{

				// setup a fresh stats file struct, we initialize a new description
				// because we may have versioned and changed the description for the last
				// stat that we read.
				tStatsFile.tDescription = tDescription;  // set description for how to read it
			
				// read stat
				STAT_XFER_RESULT eResult = sStatsFileXfer(	tStatsFile, tXfer, nStatsVersion, eMethod, pUnit, pDBUnit );
				if (eResult == SXR_ERROR)
				{
					ASSERTV_RETFALSE( 
						0, 
						"Error loading stat description '%s'", 
						pStatsData ? pStatsData->m_szName : "Unknown");
				}
			
				// is it ok to proceed handling this stat
				if (eResult == SXR_OK)
				{
				
					// add to stats list
					if (sStatsFileAddToStatList( pGame, pStats, tStatsFile ) == FALSE)
					{
						ASSERTV_RETFALSE(
							0,
							"Error adding stats to list '%s'",
							pStatsData ? pStatsData->m_szName: "Unknown");
					}
					
				}
								
			}
						
		}

	}

	return TRUE;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sStatsListXfer( 
	GAME *pGame, 
	STATS *pStats, 
	XFER<mode> &tXfer, 
	STATS_WRITE_METHOD eMethod, 
	STATS_VERSION nStatsVersion,
	UNIT *pUnit,
	DATABASE_UNIT *pDBUnit)
{
	ASSERTX_RETFALSE( pGame || eMethod == STATS_WRITE_METHOD_PAK, "Expected game" );
	ASSERTX_RETFALSE( pStats, "Expected stats" );

	// magic
	STATS_TEST_MAGIC_RETVAL( pStats, FALSE );

	// num stats
	int nCursorNumStats = tXfer.GetCursor();
	unsigned int nNumStats = 0;
	XFER_UINT( tXfer, nNumStats, STREAM_STAT_ENTRY_BITS );

	TraceExtraVerbose(TRACE_FLAG_GAME_MISC_LOG, "sStatsListXfer:  UNIT:%p  %d stats", pUnit, nNumStats);

	// xfer stats
	if (sDoStatsListXfer( pGame, pStats, tXfer, nStatsVersion, nNumStats, eMethod, pUnit, pDBUnit ) == FALSE)
	{
		return FALSE;
	}
			
	// go back and write correct number of stats written
	if (tXfer.IsSave())
	{
		int nCursorNext = tXfer.GetCursor();
		tXfer.SetCursor( nCursorNumStats );
		XFER_UINT( tXfer, nNumStats, STREAM_STAT_ENTRY_BITS );
		tXfer.SetCursor( nCursorNext );
	}
		
	return TRUE;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL StatsXfer(
	GAME *pGame,
	STATS *pStats,
	XFER<mode> & tXfer,
	STATS_WRITE_METHOD eMethod,
	int *pnStatsVersion /*= NULL*/,
	UNIT *pUnit /*= NULL*/,
	DATABASE_UNIT *pDBUnit /*= NULL*/)
{
	ASSERTX_RETFALSE( pGame || eMethod == STATS_WRITE_METHOD_PAK, "Expected game" );
	ASSERTX_RETFALSE( pStats, "Expected stats" );
	ASSERTX_RETFALSE( eMethod > STATS_WRITE_METHOD_INVALID && eMethod < NUM_STATS_WRITE_METHODS, "Invalid stats write method" );

	// magic baby!
	STATS_TEST_MAGIC_RETVAL( pStats, FALSE );
	
	// cannot be a rider
	ASSERTX_RETFALSE( StatsTestFlag( pStats, STATSFLAG_RIDER ) == FALSE, "Expected non-rider" );

	// stats get their own version because they are a complex system and are also written
	// independently of unit files (in which we usually store the version in the header) in
	// order to be written/read from the pak file
	STATS_VERSION nStatsVersion = STATS_CURRENT_VERSION;
	XFER_VERSION( tXfer, nStatsVersion );

	// save version in param if available
	if (pnStatsVersion != NULL)
	{
		*pnStatsVersion = nStatsVersion;
	}
	
	// xfer write method
	XFER_ENUM( tXfer, eMethod, STREAM_STAT_WRITE_METHOD_BITS );

	// num riders
	int nCursorNumRiders = tXfer.GetCursor();
	unsigned int nNumRiders = 0;
	XFER_UINT( tXfer, nNumRiders, STREAM_STAT_RIDER_BITS );

	// xfer each rider
	if (tXfer.IsSave())
	{
		STATS *pRider = pStats->m_FirstRider;
		while (pRider)
		{
		
			if (pRider->m_RiderParent == pStats)
			{
			
				// xfer selector
				STATS_SELECTOR eSelector = StatsGetSelector( pRider );
				if (StatsSelectorXfer( tXfer, eSelector, eMethod, nStatsVersion ) == FALSE)
				{
					ASSERTV_RETFALSE( 
						0, 
						"SAVE: StatsXfer() - StatsSelectorXfer failed.  Selector=%d, Method=%d, Version=%d", 
						eSelector,
						eMethod,
						nStatsVersion);
				}
				
				// xfer the stats list
				if (sStatsListXfer( pGame, pRider, tXfer, eMethod, nStatsVersion, pUnit, pDBUnit ) == FALSE)
				{
					ASSERTV_RETFALSE( 
						0, 
						"SAVE: StatsXfer() - Rider sStatsListXfer failed.  Method=%d, Version=%d, Unit=%s", 
						eMethod,
						nStatsVersion,
						UnitGetDevName( pUnit ));
					return FALSE;
				}
				
				nNumRiders++;  // wrote another thing to the file now
				
			}
			
			// check for last stats list
			if (pRider == pStats->m_LastRider)
			{
				break;
			}
			
			// goto next
			pRider = pRider->m_Next;
			ASSERTX_RETFALSE( pRider, "Expected rider" );
			
		}
	
	}
	else
	{
		for (unsigned int i = 0; i < nNumRiders; ++i)
		{
		
			// read selector
			STATS_SELECTOR eSelector = SELECTOR_INVALID;
			if (StatsSelectorXfer( tXfer, eSelector, eMethod, nStatsVersion ) == FALSE)
			{
				ASSERTV_RETFALSE( 
					0, 
					"LOAD: StatsXfer() - StatsSelectorXfer failed.  Selector=%d, Method=%d, Version=%d", 
					eSelector,
					eMethod,
					nStatsVersion);
			}
			
			// create stats list
			STATS *pRider = StatsListAddRider( pGame, pStats, eSelector );
			
			// xfer stats list
			if (sStatsListXfer( pGame, pRider, tXfer, eMethod, nStatsVersion, pUnit, pDBUnit ) == FALSE)
			{
				ASSERTV_RETFALSE( 
					0,  
					"LOAD: StatsXfer() - Riders StatsListXfer failed.  Method=%d, Version=%d, Unit=%s", 
					eMethod,
					nStatsVersion,
					UnitGetDevName( pUnit ));
			}
			
		}	
			
	}
	
	if (tXfer.IsSave())
	{

		// go back and write correct number of riders written	
		int nCursorNext = tXfer.GetCursor();
		tXfer.SetCursor( nCursorNumRiders );
		XFER_UINT( tXfer, nNumRiders, STREAM_STAT_RIDER_BITS );
		tXfer.SetCursor( nCursorNext );

	}
	
	// now this stats list
	if (sStatsListXfer( pGame, pStats, tXfer, eMethod, nStatsVersion, pUnit, pDBUnit ) == FALSE)
	{
		ASSERTV_RETFALSE( 
			0,  
			"%s: StatsXfer() - Main StatsListXfer failed.  Method=%d, Version=%d, Unit=%s", 
			tXfer.IsSave() ? "SAVE" : "LOAD",
			eMethod,
			nStatsVersion,
			UnitGetDevName( pUnit ));	
	}
	
	return TRUE;	 
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL StatsSelectorXfer(
	XFER<mode> &tXfer,
	STATS_SELECTOR &eSelector,
	STATS_WRITE_METHOD eStatsWriteMethod,
	int nStatsVersion)
{	

	if (eStatsWriteMethod == STATS_WRITE_METHOD_NETWORK)
	{
		XFER_ENUM( tXfer, eSelector, STREAM_BITS_STATS_SELECTOR );
	}
	else
	{
		DWORD dwSelectorCode = INVALID_CODE;
		if (tXfer.IsSave())
		{
			dwSelectorCode = ExcelGetCode( NULL, DATATABLE_STATS_SELECTOR, eSelector );
		}
		XFER_DWORD( tXfer, dwSelectorCode, STREAM_BITS_STATS_SELECTOR_CODE );
		if (tXfer.IsLoad())
		{
			eSelector = (STATS_SELECTOR)ExcelGetLineByCode( NULL, DATATABLE_STATS_SELECTOR, dwSelectorCode );
		}
	}
	ASSERTX_RETFALSE( eSelector >= SELECTOR_NONE && eSelector < NUM_STATS_SELECTOR, "Invalid stats selector" );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL StatsListIdXfer(
	XFER<mode> & tXfer,
	DWORD & dwStatsListId,
	STATS_WRITE_METHOD eStatsWriteMethod,
	int nStatsVersion)
{

	if (eStatsWriteMethod == STATS_WRITE_METHOD_NETWORK)
	{
		XFER_DWORD( tXfer, dwStatsListId, STREAM_BITS_STATSLIST_ID );
	}

	return TRUE;
}
