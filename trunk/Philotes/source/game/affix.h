//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _AFFIX_H_
#define _AFFIX_H_


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _SCRIPT_TYPES_H_ 
#include "script_types.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct ITEM_SPEC;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum AFFIX_CONSTANTS
{
	MAX_AFFIX_GROUP_NAME		= 64,
	AFFIX_PROPERTIES			= 6,		// in item quality this is the number of code columns to specify affixes
	MAX_AFFIX_PER_UNIT_HELLGATE = 6,		// arbitrary, bump up as needed
	MAX_AFFIX_PER_UNIT_MYTHOS   = 15,		// arbitrary, bump up as needed
	MAX_AFFIX_ARRAY_PER_UNIT	= MAX_AFFIX_PER_UNIT_MYTHOS,	//this needs to be the highest value between MAX_AFFIX_PER_UNIT_HELLGATE and MAX_AFFIX_PER_UNIT_MYTHOS
	AFFIX_UNITTYPES				= 6,		// Unittypes that can have the affixes
	AFFIX_AFFIXTYPES			= 15,		// The max number in a group of affixes	
};

//----------------------------------------------------------------------------
enum AFFIX_STYLE
{
	AFFIX_STYLE_INVALID = -1,
	
	AFFIX_STYLE_STAT,			// a stat based affix (bonus to health for instance)
	AFFIX_STYLE_PROC,			// affix that executes a procedure (like chance of charged bolt when hit)
	
};

//----------------------------------------------------------------------------
enum AFFIX_NAME_TYPE
{
	ANT_INVALID = -1,
	
	ANT_QUALITY,
	ANT_SET,
	ANT_MAGIC,
	ANT_REPLACEMENT,
	
	ANT_NUM_TYPES				// keep this last
};

struct AFFIX_TYPE_DATA
{
	char			szName[DEFAULT_INDEX_SIZE];
	int				nNameColor;
	int				nDowngradeType;
	BOOL			bRequired;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct AFFIX_GROUP
{
	int				nID;
	EXCEL_STR		(szName);								
	int				nWeight;
	int				nUnitType;
	int				nParentGroupID;
	AFFIX_GROUP		*pParentGroup;
	int*			pnAffixIDs;
	int				nNumAffixIDs;
	AFFIX_GROUP**	pChildAffixGroups; 
	int				nNumChildAffixGroups;
};

struct AFFIX_DATA
{
	EXCEL_STR		(szName);								// key
	BOOL			bAlwaysApply;							// always apply affix (applied affixes are identified)

	int				nStringName[ANT_NUM_TYPES];				// this affix affects the item name it's applied to in these forms
	
	int				nFlavorText;
	int				nLookGroup;
	int				nNameColor;
	int				nGridColor;
	int				nDominance;
	DWORD			dwCode;
	int				nAffixType[AFFIX_AFFIXTYPES];
	BOOL			bIsSuffix;
	int				nGroup;
	AFFIX_STYLE		eStyle;
	BOOL			bUseWhenAugementing;					// This affix is usable when augmenting an item (enchanter)
	BOOL			bSpawn;
	int				nMinLevel;
	int				nMaxLevel;
	int				nAllowTypes[AFFIX_UNITTYPES];
	PCODE			codeWeight;
	int				nLuckModWeight;
	char			szColorSet[MAX_XML_STRING_LENGTH];
	int				nColorSet;
	int				nColorSetPriority;
	int				nState;
	BOOL			bSaveState;
	PCODE			codeBuyPriceMult;
	PCODE			codeBuyPriceAdd;
	PCODE			codeSellPriceMult;
	PCODE			codeSellPriceAdd;
	PCODE			codeAffixCond;
	int				nItemLevel;
	PCODE			codePropCond[AFFIX_PROPERTIES];
	PCODE			codeProperty[AFFIX_PROPERTIES];
	int				nOnlyOnItemsRequiringUnitType;			// this affix can only go on items for a ... "templar" for example
};


//----------------------------------------------------------------------------
enum AFFIX_PICK_FLAGS
{
	APF_FOR_AUGMENTATION_BIT,
	APF_ALLOW_PROCS_BIT,		// allow "proc" style affixes
	APF_REQUIRED_AFFIX_BIT,		// is a required affix pick	
	APF_IGNORE_LEVEL_CHECK,
	APF_IGNORE_UNIT_TYPE_CHECK,
	APF_IGNORE_CONTAINER_TYPE_CHECK,
	APF_IGNORE_EXISTING_GROUP_CHECK,
	APF_IGNORE_CONDITION_CODE,
};

//----------------------------------------------------------------------------
enum AFFIX_FAMILY
{
	AF_INVALID = -1,
	
	AF_APPLIED,			// an applied affix, identified and working on an item
	AF_ATTACHED,		// attached affixes will become real applied affixes after an item is identified
	
	AF_NUM_FAMILY,
	AF_ALL_FAMILIES = AF_NUM_FAMILY
	
};


//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------
BOOL AffixExcelPostProcess(
	struct EXCEL_TABLE * table);
	
int s_AffixPick(
	UNIT * unit,
	DWORD dwAffixPickFlags,		// see AFFIX_PICK_FLAGS
	int affix_type,
	ITEM_SPEC * spec,
	int affix_group);

#if !ISVERSION(CLIENT_ONLY_VERSION)
int s_AffixPickSpecific(
	UNIT * unit,
	int affix,
	const ITEM_SPEC *pItemSpec);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
int s_AffixAttach(
	UNIT * unit,
	int affix);
#endif

int c_AffixApply(
	UNIT * unit,
	int affix);

//----------------------------------------------------------------------------
enum AFFIX_APPLY_FLAGS
{
	AAF_DO_NOT_SET_ITEM_LEVEL,			// even if the affix sets an item level, don't set it
};

#if !ISVERSION(CLIENT_ONLY_VERSION)
int s_AffixApply(
	UNIT * unit,
	int affix,
	DWORD dwAffixApplyFlags = 0);
#endif

int AffixGetMaxAffixCount();

void s_AffixApplyAttached(
	UNIT *pUnit);
			
void s_AffixRemoveAll(
	UNIT *pUnit,
	AFFIX_FAMILY eFamily);

const AFFIX_DATA *AffixGetData(
	int nAffix);

DWORD AffixGetCode(
	int nAffix);
	
int AffixGetByCode(
	DWORD dwCode);
	
int AffixGetByType(
	GAME * game,
	int affix_type,
	DWORD code);

BOOL AffixIsFocus(
	int nAffix);

int AffixGetNameStat(
	AFFIX_FAMILY eFamily);

int AffixGetAffixCount(
	UNIT *pUnit,
	AFFIX_FAMILY eFamily);

int AffixGetByIndex(
	UNIT *pUnit,
	AFFIX_FAMILY eFamily,
	int nIndex);
	
void AffixGetMagicName(
	int nAffix,
	DWORD dwStringAttributes,
	WCHAR *puszString,
	int nMaxSize,
	BOOL bClean);

void AffixGetStringListOnUnit(
	UNIT *pUnit,
	char *pszBuffer,
	int nBufferSize);

BOOL AffixNameIsPossessive(
	const AFFIX_DATA *pAffixData,
	int nPossessiveAttributeBit);

BOOL AffixGroupsExcelPostProcess(
	struct EXCEL_TABLE * table);

//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
extern int gnAffixFocus;
#endif

	
//----------------------------------------------------------------------------
// INLINE
//----------------------------------------------------------------------------
inline BOOL AffixIsType(
	const AFFIX_DATA * affix_data,
	int affix_type)
{
	ASSERT_RETFALSE(affix_data);
	for (unsigned int ii = 0; ii < arrsize(affix_data->nAffixType); ++ii)
	{
		if (affix_data->nAffixType[ii] == affix_type)
		{
			return (affix_type >= 0 || ii == 0);
		}
	}
	return FALSE;
}


#endif
