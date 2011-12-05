//----------------------------------------------------------------------------
// FILE: act.h
//----------------------------------------------------------------------------

#ifndef __ACT_H_
#define __ACT_H_

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum ACT_DATA_FLAG
{
	ADF_BETA_ACCOUNT_CAN_PLAY,
	ADF_NONSUBSCRIBER_ACCOUNT_CAN_PLAY,
	ADF_TRIAL_ACCOUNT_CAN_PLAY,
};

//----------------------------------------------------------------------------
struct ACT_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];	// name	
	WORD wCode;							// code
	DWORD dwActDataFlags;				// see ACT_DATA_FLAG
	int nMinExperienceLevelToEnter;		// min experience level required for this act
	
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

const ACT_DATA *ActGetData(
	int nAct);

BOOL ActExcelPostProcess(
	struct EXCEL_TABLE *pTable);
	
void ActSetLastAct(
	const char *pszAct);
	
BOOL ActIsAvailable(
	int nAct);

enum WARP_RESTRICTED_REASON ActIsAvailableToUnit(
	UNIT *pUnit,
	int nAct);
	
#endif