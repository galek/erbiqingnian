//-------------------------------------------------------------------------------------------------
// presult.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------
#ifndef _PRESULT_H_
#define _PRESULT_H_



//-------------------------------------------------------------------------------------------------
// PRESULT - (equivalent to Windows HRESULT) Prime-common operation status value
//-------------------------------------------------------------------------------------------------


typedef LONG PRESULT;



//  ----------------------------------------------------------------------
//  Prime-specific defines
//

//
//  Generic Prime facilities (from where the result comes)
//

#define PRESULT_BASE_FACILITY			0x40

enum PRESULT_PRIME_FACILITY
{
	FACILITY_PRIME				= PRESULT_BASE_FACILITY,
	FACILITY_PRIME_ENGINE,
	FACILITY_PRIME_SERVER,
	// 
	__PPF_LAST_FACILITY,
	NUM_PRIME_FACILITIES		= (__PPF_LAST_FACILITY - PRESULT_BASE_FACILITY),
};


//
//  Prime-specific facility error string callback
//

typedef const TCHAR* (*PFN_FACILITY_GET_RESULT_STRING)( PRESULT pr );



//  ----------------------------------------------------------------------
//  PRESULT manipulators
//

//
//  Return the code
//

#define PRESULT_CODE(pr)		HRESULT_CODE(pr)

//
//  Return the facility
//

#define PRESULT_FACILITY(pr)	HRESULT_FACILITY(pr)

//
//  Return the severity
//

#define PRESULT_SEVERITY(pr)	HRESULT_SEVERITY(pr)

//
//  Create a PRESULT value from component pieces
//

#define MAKE_PRESULT(sev,fac,code)				MAKE_HRESULT(sev,fac,code)
#define MAKE_STATIC_PRESULT(sev,fac,code)		\
	((PRESULT) ((static_cast<unsigned long>(sev)<<31) | (static_cast<unsigned long>(fac)<<16) | (static_cast<unsigned long>(code))) )



//  ----------------------------------------------------------------------
//  Function headers
//

void PRSetFacilityGetResultStringCallback(
	PRESULT_PRIME_FACILITY eFacility,
	PFN_FACILITY_GET_RESULT_STRING pfnCallback );

const TCHAR* PRGetErrorString(
	PRESULT pr );

void PRTrace(
	const char * szFmt,
	... );

int PRFailureTrace(
	PRESULT pr,
	const TCHAR * pszStatement,
	BOOL bAssert,
	const TCHAR * pszFile,
	int nLine,
	const TCHAR * pszFunction );



//  ----------------------------------------------------------------------
//  Convenience macros
//

#define PRCONCAT_3_(x, y)					x##y
#define PRCONCAT_2_(x, y)					PRCONCAT_3_(x, y)
#define PRCONCAT(x, y)						PRCONCAT_2_(x##__NO_UNBRACED_IFs__, y)
#define PRUIDEN(x)							PRCONCAT(x, __LINE__)

#define V_ACTION(stm,act,asrt,post)		PRESULT PRUIDEN(pr)  = stm;   if ( FAILED(PRUIDEN(pr) ) ) { if ( PRFailureTrace( PRUIDEN(pr), #stm, asrt,  __FILE__, __LINE__, __FUNCSIG__ ) ) { DEBUG_BREAK(); } {act;} }	post
#define V_SAVE_ACTION(pr,stm,act)						pr   = stm;   if ( FAILED(		  pr  ) ) { if ( PRFailureTrace(		   pr , #stm, FALSE, __FILE__, __LINE__, __FUNCSIG__ ) ) { DEBUG_BREAK(); } {act;} }
#define V_SAVE_ERROR(pr,stm)			PRESULT PRUIDEN(lpr) = stm;   if ( FAILED(PRUIDEN(lpr)) ) { pr = PRUIDEN(lpr); if ( PRFailureTrace( PRUIDEN(lpr), #stm, FALSE,  __FILE__, __LINE__, __FUNCSIG__ ) ) { DEBUG_BREAK(); } }

#define V(x)					V_ACTION(x,							, TRUE,												)
#define V_RETURN(x)				V_ACTION(x,		return PRUIDEN(pr);	, TRUE,												)
#define V_CONTINUE(x)			V_ACTION(x,		continue;			, TRUE,												)
#define V_BREAK(x)				V_ACTION(x,		break;				, TRUE,												)
#define V_GOTO(lbl,x)			V_ACTION(x,		goto lbl;			, TRUE,												)
#define V_DO_FAILED(x)			V_ACTION(x,							, FALSE,	if ( FAILED(PRUIDEN(pr)) )				)
#define V_DO_SUCCEEDED(x)		V_ACTION(x,							, FALSE,	if ( SUCCEEDED(PRUIDEN(pr)) )			)
#define V_DO_RESULT(x,res)		V_ACTION(x,							, FALSE,	if ( PRUIDEN(pr) == (PRESULT)(res) )	)

#define V_SAVE(pr,stm)				V_SAVE_ACTION(pr,stm,					)
#define V_SAVE_GOTO(pr,lbl,stm)		V_SAVE_ACTION(pr,stm,	goto lbl;		)


//  ----------------------------------------------------------------------
//  Trace functions
//

enum PR_TRACE_LEVEL
{
	PR_TRACE_LEVEL_NONE = 0,
	PR_TRACE_LEVEL_SIMPLE,
	PR_TRACE_LEVEL_VERBOSE
};

extern BOOL gbPRAssertOnFailure;
extern PR_TRACE_LEVEL gePRTraceLevel;
extern BOOL gbPRBreakOnFailure;


#define PR_ASSERT_REGION(enabled)				_PR_DEBUG_REGION __prdr__##__LINE__( static_cast<int*>(&gbPRAssertOnFailure),	enabled )
#define PR_TRACE_REGION(level)					_PR_DEBUG_REGION __prdr__##__LINE__( static_cast<int*>(&gePRTraceLevel),		level   )
#define PR_BREAK_REGION(enabled)				_PR_DEBUG_REGION __prdr__##__LINE__( static_cast<int*>(&gbPRBreakOnFailure),	enabled )


class _PR_DEBUG_REGION
{
	int * m_pnGlobal;
	int m_nPrevValue;
public:
	_PR_DEBUG_REGION( int* pnGlobal, int nValue )
	{
		m_pnGlobal   = pnGlobal;
		m_nPrevValue = *m_pnGlobal;
		*m_pnGlobal  = nValue;
	}
	~_PR_DEBUG_REGION()
	{
		*m_pnGlobal  = m_nPrevValue;
	}
};



//  ----------------------------------------------------------------------
//  PRESULT special code definitions
//

#define _PRESULT_TYPEDEF_(_c) ((PRESULT)_c)


//
// MessageId: E_PRIME_FAIL
//
//  Generic Prime-specific failure code
//
#define E_PRIME_FAIL					MAKE_STATIC_PRESULT( SEVERITY_ERROR, FACILITY_PRIME, 0 )

//
// MessageId: S_PRIME_OK
//
//  Generic Prime-specific success code
//
#define S_PRIME_OK						MAKE_STATIC_PRESULT( SEVERITY_SUCCESS, FACILITY_PRIME, 0 )

//
// MessageId: S_PE_TRY_AGAIN
//
//  Generic Prime-specific "try-again" code
//
#define S_PE_TRY_AGAIN					MAKE_STATIC_PRESULT( SEVERITY_SUCCESS, FACILITY_PRIME_ENGINE, 0x100 )





#endif // _PRESULT_H_
