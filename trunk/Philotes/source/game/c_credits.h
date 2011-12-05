//----------------------------------------------------------------------------
// FILE: credits.h
//----------------------------------------------------------------------------

#ifndef __CREDITS_H_
#define __CREDITS_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum UI_MSG_RETVAL;
struct UI_COMPONENT;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
	
//----------------------------------------------------------------------------
// UI Handlers
//----------------------------------------------------------------------------

void c_CreditsTryAutoEnter(
	void);

void c_CreditsSetAutoEnter(
	BOOL bAutoEnter);
	
int c_CreditsGetCurrentMovie(
	void);
	
UI_MSG_RETVAL UIButtonCreditsShow(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIButtonCreditsExit(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam);

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern BOOL gbAutoAdvancePages;

#endif