//----------------------------------------------------------------------------
// FILE: gag.h
//----------------------------------------------------------------------------

#ifndef __GAG_H_
#define __GAG_H_

//----------------------------------------------------------------------------
// Forward Declrations
//----------------------------------------------------------------------------
struct UNIT;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// This enum should match the enum in the CSRPortal
enum GAG_ACTION
{
	GAG_INVALID,
	GAG_1_DAYS,
	GAG_3_DAYS,
	GAG_7_DAYS,
	GAG_PERMANENT,
	GAG_UNGAG,
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Common Functions
//----------------------------------------------------------------------------

GAG_ACTION GagActionParse(
	const WCHAR *puszAction);

time_t GagGetTimeFromNow(
	GAG_ACTION eGagAction);
		

//----------------------------------------------------------------------------
// Server Functions
//----------------------------------------------------------------------------

void s_GagEnable(
	UNIT *pPlayer,
	time_t timeGaggedUntil,
	BOOL bInformChatServer);

void s_GagDisable(
	UNIT *pPlayer,
	BOOL bInformChatServer);
	
BOOL s_GagIsGagged(
	UNIT *pPlayer);

time_t s_GagGetGaggedUntil(
	UNIT *pPlayer);

//----------------------------------------------------------------------------
// Client functions
//----------------------------------------------------------------------------

BOOL c_GagIsGagged(
	UNIT *pPlayer);
	
void c_GagDisplayGaggedMessage(
	void);
	
#endif  // end __GAG_H_