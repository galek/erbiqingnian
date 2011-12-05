//*****************************************************************************
//
// Page fault profiling.
//
//*****************************************************************************

#ifndef __E_PFPROF__
#define __E_PFPROF__

//-----------------------------------------------------------------------------

#if 0
	#if defined(_M_IX86) && defined(_WIN32) && (!defined(_WIN64))
		#define ENABLE_PAGE_FAULT_PROFILING 1
	#endif
#endif

//-----------------------------------------------------------------------------

#ifdef ENABLE_PAGE_FAULT_PROFILING

bool e_PfProfInit(void);
void e_PfProfDeinit(void);
void e_PfProfBracket(void);					// synchronize page fault data before proceeding
void e_PfProfMark_(const char * pFmt, ...);	// mark the spot in the page fault log

#define e_PfProfMark(a) e_PfProfMark_ a		// use as e_PfProfMark(( ... ))

#else

#define e_PfProfInit()		true
#define e_PfProfDeinit()	((void)0)
#define e_PfProfBracket()	((void)0)
#define e_PfProfMark(a)		((void)0)

#endif

//-----------------------------------------------------------------------------

#endif
