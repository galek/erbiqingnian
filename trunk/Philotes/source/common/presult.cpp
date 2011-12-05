//----------------------------------------------------------------------------
// presult.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:6386)
#include <comdef.h>
#pragma warning(pop)


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BOOL			gbPRAssertOnFailure = TRUE;
PR_TRACE_LEVEL	gePRTraceLevel		= PR_TRACE_LEVEL_NONE;
BOOL			gbPRBreakOnFailure	= FALSE;

PFN_FACILITY_GET_RESULT_STRING	gpfnFacilityGetResultString[ NUM_PRIME_FACILITIES ] = { 0 };

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

static int sGetFacilityIndex( PRESULT_PRIME_FACILITY eFacility )
{
	if ( eFacility < PRESULT_BASE_FACILITY || eFacility >= __PPF_LAST_FACILITY )
		return -1;
	return eFacility - PRESULT_BASE_FACILITY;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void PRSetFacilityGetResultStringCallback( PRESULT_PRIME_FACILITY eFacility, PFN_FACILITY_GET_RESULT_STRING pfnCallback )
{
	int nIndex = sGetFacilityIndex( eFacility );
	if ( nIndex < 0 )
		return;

	gpfnFacilityGetResultString[ nIndex ] = pfnCallback;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// returns the string description
const TCHAR* PRGetErrorString(
	PRESULT pr )
{
	PRESULT_PRIME_FACILITY eFacility = (PRESULT_PRIME_FACILITY) PRESULT_FACILITY(pr);

	static const int scnMaxLen = 512;
	static char szErr[ scnMaxLen ];
	const char* pszStr = NULL;

	// see if it's specifically a special facility
	int nIndex = sGetFacilityIndex( eFacility );

	if ( nIndex >= 0 )
	{
		if ( gpfnFacilityGetResultString[ nIndex ] )
			pszStr = (gpfnFacilityGetResultString[ nIndex ])( pr );
	}
	else
	{
		// it's not one of the special ones -- give all of the handlers a chance at it
		for ( int i = 0; i < NUM_PRIME_FACILITIES; i++ )
		{
			if ( gpfnFacilityGetResultString[ i ] )
			{
				pszStr = (gpfnFacilityGetResultString[ i ])( pr );
				if ( pszStr && pszStr[ 0 ] )
					break;
			}
		}
	}

	if ( ! pszStr || ! pszStr[ 0 ] )
	{
		PStrCopy( szErr, (const char*)_com_error( static_cast<HRESULT>(pr) ).ErrorMessage(), scnMaxLen );
		//pszStr = (const char*)_com_error( static_cast<HRESULT>(pr) ).ErrorMessage();
		pszStr = szErr;
	}

	return pszStr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void PRTrace(
	const char * szFmt,
	... )
{
	va_list args;
	va_start(args, szFmt);

	vtrace(szFmt, args);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int PRFailureTrace( PRESULT pr, const TCHAR * pszStatement, BOOL bAssert, const TCHAR * pszFile, int nLine, const TCHAR * pszFunction )
{
#if ISVERSION(DEVELOPMENT)

	if ( gePRTraceLevel > PR_TRACE_LEVEL_NONE )
	{
		switch ( gePRTraceLevel )
		{
		case PR_TRACE_LEVEL_SIMPLE:
			PRTrace(	"FAILED: 0x%08x @ %s:%d\n",
						pr,
						pszFile,
						nLine );
			break;
		case PR_TRACE_LEVEL_VERBOSE:
			PRTrace(	"FAILED: %s\n"
						"        returned 0x%08x - \"%s\"\n"
						"        at %s:%d\n",
						pszStatement,
						pr,
						PRGetErrorString( pr ),
						pszFile,
						nLine );
			break;
		}
	}

	int nResult = 0;

	if ( gbPRAssertOnFailure && bAssert )
	{
		TCHAR szMessage[ 256 ];
		const TCHAR * pszErr = PRGetErrorString( pr );
		if ( pszErr )
			PStrPrintf( szMessage, 256, "FAILED: %s", pszErr );
		else
			PStrPrintf( szMessage, 256, "FAILED: 0x%08X", pr );
		nResult = DoAssert( pszStatement, szMessage, pszFile, nLine, pszFunction );
	} 
	else if ( gbPRBreakOnFailure)
	{
		nResult = 1;
	}

	return nResult;

#else

	// CHB 2007.01.10
	(void)pr;
	(void)pszStatement;
	(void)bAssert;
	(void)pszFile;
	(void)nLine;
	(void)pszFunction;

	return 0;
#endif // DEVELOPMENT
}