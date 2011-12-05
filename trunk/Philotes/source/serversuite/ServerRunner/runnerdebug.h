//----------------------------------------------------------------------------
// runnerdebug.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _RUNNERDEBUG_H_
#define _RUNNERDEBUG_H_


//----------------------------------------------------------------------------
// server execution exception filter.
//----------------------------------------------------------------------------
int RunnerExceptFilter(
	EXCEPTION_POINTERS *pExceptionInformation,
	SVRTYPE svrType,
	SVRINSTANCE svrInstance,
	const char * descOfWhatWasHappening );

//----------------------------------------------------------------------------
// desktop mode methods
//----------------------------------------------------------------------------

BOOL
	DesktopServerIsEnabled(
		void );

void
	SetDesktopServerMode(
		BOOL );

BOOL
	QuickStartDesktopServerIsEnabled(
		void );

void
	SetQuickStartDesktopServerMode(
		BOOL );

BOOL
	InitDesktopServerTracing(
		char * filename );

void
	FreeDesktopServerTracing(
		void );

int
	DoDesktopServerAssert(
		char *);

#if ISVERSION(DEVELOPMENT)
void
	DesktopServerTrace(
		const char * format,
		... );
#else
#define DesktopServerTrace(format,...)
#endif


#endif	//	_RUNNERDEBUG_H_
