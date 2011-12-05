//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "affix.h"
#include "units.h" // also includes game.h
#include "items.h"
#include "picker.h"
#include "script.h"
#include "colorset.h"
#include "states.h"
#include "stringtable.h"
#include "unit_priv.h"
#include "gameunits.h"
#include "excel_private.h"

#if ISVERSION(SERVER_VERSION)
#include "affix.cpp.tmh"
#endif


struct AFFIX_GLOBALS
{
	AFFIX_GROUP tHeadAffixGroup;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
int gnAffixFocus = INVALID_LINK;  // debugging tools
#endif
static const BOOL sgbAssertOnTooManyAffixes = TRUE;

AFFIX_GLOBALS gAffixGlobals;

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AffixExcelPostProcess(
struct EXCEL_TABLE * table)
{
	return TRUE;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddChildToGroup(
	AFFIX_GROUP * pParentGroup,
	AFFIX_GROUP * pChildGroup)
{
	ASSERT_RETURN(pParentGroup);
	ASSERT_RETURN(pChildGroup);

	pParentGroup->pChildAffixGroups = (AFFIX_GROUP **)REALLOCZ(NULL, pParentGroup->pChildAffixGroups, sizeof(AFFIX_GROUP *) * (pParentGroup->nNumChildAffixGroups + 1));
	pParentGroup->pChildAffixGroups[pParentGroup->nNumChildAffixGroups++] = pChildGroup;
	pChildGroup->pParentGroup = pParentGroup;
}

BOOL AffixGroupsExcelPostProcess(
	struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	memclear(&gAffixGlobals, sizeof(AFFIX_GLOBALS));
	gAffixGlobals.tHeadAffixGroup.nNumChildAffixGroups = 0;
	gAffixGlobals.tHeadAffixGroup.pChildAffixGroups = NULL;
	gAffixGlobals.tHeadAffixGroup.nID = INVALID_LINK;

	int nCount = ExcelGetCountPrivate(table);
	int nAffixCount = ExcelGetCountPrivate(ExcelGetTableNotThreadSafe(DATATABLE_AFFIXES));

	for (int iAffix = 0; iAffix < nAffixCount; ++iAffix)
	{
		AFFIX_DATA * pAffixData = (AFFIX_DATA *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_AFFIXES), iAffix);
		if ( !pAffixData )
			continue;

		if (pAffixData->nGroup != INVALID_LINK)
		{
			AFFIX_GROUP * pGroup = (AFFIX_GROUP *)ExcelGetDataPrivate(table, pAffixData->nGroup);
			ASSERT_RETFALSE(pGroup);

			pGroup->nNumAffixIDs++;
		}
	}

	for (int iGroup = 0; iGroup < nCount; ++iGroup)
	{
		AFFIX_GROUP * pGroup = (AFFIX_GROUP *)ExcelGetDataPrivate(table, iGroup);
		if ( !pGroup )
			continue;

		pGroup->nID = iGroup;
		pGroup->pnAffixIDs = NULL;
		if (pGroup->nParentGroupID == INVALID_LINK)
		{
			sAddChildToGroup(&gAffixGlobals.tHeadAffixGroup, pGroup);
		}
		else
		{
			AFFIX_GROUP * pParentGroup = (AFFIX_GROUP *)ExcelGetDataPrivate(table, pGroup->nParentGroupID);
			if ( pParentGroup )
				sAddChildToGroup(pParentGroup, pGroup);
		}

		if (pGroup->nNumAffixIDs > 0)
		{
			pGroup->pnAffixIDs = (int *)MALLOC(NULL, sizeof(int) * pGroup->nNumAffixIDs);
			pGroup->nNumAffixIDs = 0;
		}
	}

	for (int iAffix = 0; iAffix < nAffixCount; ++iAffix)
	{
		AFFIX_DATA * pAffixData = (AFFIX_DATA *)ExcelGetDataPrivate(ExcelGetTableNotThreadSafe(DATATABLE_AFFIXES), iAffix);
		if ( !pAffixData )
			continue;

		if (pAffixData->nGroup != INVALID_LINK)
		{
			AFFIX_GROUP * pGroup = (AFFIX_GROUP *)ExcelGetDataPrivate(table, pAffixData->nGroup);
			ASSERT_RETFALSE(pGroup);

			pGroup->pnAffixIDs[pGroup->nNumAffixIDs++] = iAffix;
		}
	}

	return TRUE;		
}

//----------------------------------------------------------------------------
enum AFFIX_PICK_ACTION
{
	AFA_APPLY,		// apply this affix to the item (identified)
	AFA_ATTACH		// attach this affix to the item (unidentified)
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AFFIX_PICK_ACTION sGetAffixPickAction(
	UNIT *pUnit,
	int nAffix)
{
	ASSERTX_RETVAL( pUnit, AFA_APPLY, "Expected unit" );

	if (UnitIsA( pUnit, UNITTYPE_ITEM ))
	{

		// check for affixes that always apply
		const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
		if (pAffixData->bAlwaysApply == TRUE)
		{
			return AFA_APPLY;
		}

		// some qualities always identify anything thrown on them (like normal mods currently)
		int nQuality = ItemGetQuality( pUnit );
		if (nQuality != INVALID_LINK)
		{
			const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( nQuality );
			ASSERT_RETVAL(pItemQualityData, AFA_APPLY);
			if (pItemQualityData->bAlwaysIdentified == TRUE)
			{
				return AFA_APPLY;
			}
		}

		// just attach the affix, making this affix unidentified
		return AFA_ATTACH;

	}

	// everything else that gets affixes are known
	return AFA_APPLY;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int s_AffixPickSpecific(
	UNIT * unit,
	int affix,
	const ITEM_SPEC *pItemSpec)
{
	switch (sGetAffixPickAction(unit, affix))
	{

		//----------------------------------------------------------------------------
	case AFA_APPLY:
		{

			// setup apply flags
			DWORD dwAffixApplyFlags = 0;
			if (pItemSpec)
			{
				if (TESTBIT( pItemSpec->dwFlags, ISF_RESTRICT_AFFIX_LEVEL_CHANGES_BIT ))
				{
					SETBIT( dwAffixApplyFlags, AAF_DO_NOT_SET_ITEM_LEVEL );
				}
			}

			// apply this affix
			return s_AffixApply(unit, affix, dwAffixApplyFlags);

		}

		//----------------------------------------------------------------------------
	case AFA_ATTACH:
		{
			return s_AffixAttach(unit, affix);
		}

		//----------------------------------------------------------------------------		
	default:
		{
			ASSERT_RETINVALID(0);
		}

	}

}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sAffixPickRemoveAffixFromSpec(
	ITEM_SPEC * spec,
	int affix)
{
	ASSERT_RETURN(spec);
	for (unsigned int jj = 0; jj < MAX_SPEC_AFFIXES; jj++)
	{
		if (spec->nAffixes[jj] == affix)
		{
			spec->nAffixes[jj] = INVALID_LINK;
			break;
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sAffixPickCountExistingAffixesInGroup(
	UNIT * unit,
	int affix_group)
{
	int count = 0;

	for (unsigned int ii = 0; ii < AF_NUM_FAMILY; ++ii)
	{
		AFFIX_FAMILY eFamily = (AFFIX_FAMILY)ii;
		int stat = AffixGetNameStat(eFamily);

		// get affix on item now
		STATS_ENTRY tStatsListAffixes[MAX_AFFIX_ARRAY_PER_UNIT];
		int nAttachedAffixCount = UnitGetStatsRange(unit, stat, 0, tStatsListAffixes, AffixGetMaxAffixCount());

		for (int jj = 0; jj < nAttachedAffixCount; ++jj)
		{
			int affix = tStatsListAffixes[jj].value;
			const AFFIX_DATA * affix_data = AffixGetData(affix);
			ASSERT_CONTINUE(affix_data);
			if (affix_data->nGroup == affix_group)
			{
				++count;
			}
		}
	}

	return count;
}
#endif


//----------------------------------------------------------------------------
// affixes can be restricted to items that are usable by certain types
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sAffixPickCanPickAffixCheckContainerType(
	const UNIT_DATA * unit_data,
	const AFFIX_DATA * affix_data)
{
	int typeRequiredContainer = affix_data->nOnlyOnItemsRequiringUnitType;
	if (typeRequiredContainer == UNITTYPE_ANY)
	{
		return TRUE;
	}
	for (unsigned int ii = 0; ii < NUM_CONTAINER_UNITTYPES; ++ii)
	{
		int typeContainer = unit_data->nContainerUnitTypes[ii];
		if (typeContainer == INVALID_LINK)
		{
			continue;
		}
		if (UnitIsA(typeContainer, typeRequiredContainer))
		{
			return TRUE;
		}
	}
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
// determine if a particular affix is a legal pick
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static int s_sAffixPickGetWeight(
	GAME * game,
	UNIT * unit,
	ITEM_SPEC * spec,
	const AFFIX_DATA * affix_data)
{
	int weight = 0;
	if (affix_data->codeWeight == NULL_CODE)
	{
		return 0;
	}

	int codelen = 0;
	BYTE * code = ExcelGetScriptCode(game, DATATABLE_AFFIXES, affix_data->codeWeight, &codelen);
	if (code)
	{
		weight = VMExecI(game, unit, code, codelen);
	}

	if (affix_data->nLuckModWeight > 0 && spec->nLuckModifier > 0.0f)
	{
		weight += (int)(spec->nLuckModifier * affix_data->nLuckModWeight);
	}

	return weight;
}
#endif


//----------------------------------------------------------------------------
// check if affix is in the spec
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sAffixPickIsSpecAffix(
	ITEM_SPEC * spec,
	unsigned int affix)
{
	for (unsigned int jj = 0; jj < MAX_SPEC_AFFIXES; jj++)
	{
		if (spec->nAffixes[jj] == (int)affix)
		{
			return TRUE;
		}
	}
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
// determine if a particular affix is a legal pick
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sAffixPickCanPickAffix(
	GAME * game,
	UNIT * unit,
	const UNIT_DATA * unit_data,
	int level,
	DWORD dwAffixPickFlags,
	ITEM_SPEC * spec,
	BOOL hasSpecAffixes,
	int affix_type,
	int affix_group,
	unsigned int affix,
	const AFFIX_DATA * affix_data)
{
	if (!affix_data->bSpawn)
	{
		return FALSE;
	}
	
	if (TESTBIT(dwAffixPickFlags,APF_FOR_AUGMENTATION_BIT))
	{
		if (!affix_data->bUseWhenAugementing)
		{
			return FALSE;
		}
	}

	if (affix_type != INVALID_LINK && !AffixIsType(affix_data, affix_type))
	{
		return FALSE;
	}
	if (!TESTBIT(dwAffixPickFlags, APF_IGNORE_LEVEL_CHECK) && (affix_data->nMinLevel > level || affix_data->nMaxLevel < level))
	{
		return FALSE;
	}
	if (affix_group != INVALID_LINK && affix_data->nGroup != affix_group)
	{
		return FALSE;
	}
	if (affix_data->eStyle == AFFIX_STYLE_PROC && !TESTBIT(dwAffixPickFlags, APF_ALLOW_PROCS_BIT))
	{
		return FALSE;
	}
	if (!TESTBIT(dwAffixPickFlags, APF_IGNORE_UNIT_TYPE_CHECK) && !IsAllowedUnit(unit, affix_data->nAllowTypes, AFFIX_UNITTYPES))
	{
		return FALSE;
	}
	if (hasSpecAffixes && !s_sAffixPickIsSpecAffix(spec, affix) && affix_group == INVALID_ID)	// we've been ordered to pick from this group, so give it precedence
	{
		return FALSE;
	}
	if (!TESTBIT(dwAffixPickFlags, APF_IGNORE_CONTAINER_TYPE_CHECK) && !s_sAffixPickCanPickAffixCheckContainerType(unit_data, affix_data))
	{
		return FALSE;
	}
	if (!TESTBIT(dwAffixPickFlags, APF_IGNORE_EXISTING_GROUP_CHECK) && affix_data->nGroup != INVALID_ID)
	{
		if (s_sAffixPickCountExistingAffixesInGroup(unit, affix_data->nGroup) > 0)
		{
			return FALSE;
		}
	}
	if (!TESTBIT(dwAffixPickFlags, APF_IGNORE_CONDITION_CODE) && affix_data->codeAffixCond != NULL_CODE)
	{
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(game, DATATABLE_AFFIXES, affix_data->codeAffixCond, &codelen);
		if (code)
		{
			if (!VMExecI(game, unit, code, codelen))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
// when we pick only from the available affix spec, and there are no more affixes to pick in that spec, stop
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sAffixPickSpecAffixesAreRequired(
	ITEM_SPEC * spec,
	DWORD dwAffixPickFlags)
{
	if (!spec)
	{
		return FALSE;
	}
	if (TESTBIT(dwAffixPickFlags, APF_REQUIRED_AFFIX_BIT))
	{
		return FALSE;
	}
	if (!TESTBIT(spec->dwFlags, ISF_AFFIX_SPEC_ONLY_BIT))
	{
		return FALSE;
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sAffixPickHasSpecAffixes(
	ITEM_SPEC * spec)
{
	if (!spec)
	{
		return FALSE;
	}
	for (unsigned int ii = 0; ii < MAX_SPEC_AFFIXES; ++ii)
	{
		if (spec->nAffixes[ii] != INVALID_LINK)
		{
			return TRUE;;
		}
	}
	return FALSE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sAffixGroupsInitForUnit(
	GAME * game,
	UNIT * unit,
	const UNIT_DATA * unit_data,
	int level,
	DWORD dwAffixPickFlags,
	ITEM_SPEC * spec,
	BOOL hasSpecAffixes,
	int affix_type,
	int affix_group)
{
	ASSERT_RETFALSE(game);
	SETBIT(dwAffixPickFlags, APF_IGNORE_EXISTING_GROUP_CHECK);

	int nCount = ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_AFFIX_GROUPS);

	memclear(game->pAffixGroupValidForCurrent, BYTE_FLAG_SIZE(nCount) * sizeof(BYTE));
	for (int iGroup = 0; iGroup < nCount; iGroup++)
	{
		AFFIX_GROUP *pGroup = (AFFIX_GROUP *)ExcelGetData(game, DATATABLE_AFFIX_GROUPS, iGroup);
		if (!pGroup || pGroup->nNumAffixIDs == 0)
			continue;

		if (pGroup->nUnitType != INVALID_LINK &&
			!UnitIsA(unit, pGroup->nUnitType))
		{
			continue;
		}

		if (s_sAffixPickCountExistingAffixesInGroup(unit, iGroup) > 0)
			continue;

		if (affix_group != INVALID_LINK && affix_group != iGroup)
			continue;

		// see if this group has any valid affixes for this item
		for (int iAffix = 0; iAffix < pGroup->nNumAffixIDs; iAffix++)
		{
			int nAffixID = pGroup->pnAffixIDs[iAffix];
			const AFFIX_DATA * pAffixData = AffixGetData(nAffixID);
			if (!pAffixData)
			{
				SETBIT(game->pAffixValidForCurrent, iAffix, FALSE);
				continue;
			}

			if (s_sAffixPickCanPickAffix(game, unit, unit_data, level, dwAffixPickFlags, spec, hasSpecAffixes, affix_type, INVALID_LINK, nAffixID, pAffixData))
			{
				SETBIT(game->pAffixValidForCurrent, nAffixID, TRUE);
				SETBIT(game->pAffixGroupValidForCurrent, pGroup->nID, TRUE);
			}
			else
			{
				SETBIT(game->pAffixValidForCurrent, nAffixID, FALSE);
			}

		}

		// if an affix group has affixes, its parent groups must reflect this (they will have no affixes of their own)
		if (TESTBIT(game->pAffixGroupValidForCurrent, pGroup->nID))
		{
			while (pGroup)
			{
				AFFIX_GROUP *pParent = pGroup->pParentGroup;
				if (pParent && pParent->nID >= 0)
				{
					SETBIT(game->pAffixGroupValidForCurrent, pParent->nID, TRUE);
				}
				pGroup = pParent;
			}
		}
	}

	return TRUE;
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AFFIX_GROUP *s_sAffixPickGroup(
	UNIT * unit,
	AFFIX_GROUP *pStartGroup)
{
	if (!pStartGroup)
		return NULL;

	GAME *game = UnitGetGame(unit);
	if (pStartGroup->nNumAffixIDs > 0)
	{
		ASSERT_RETNULL(TESTBIT(game->pAffixGroupValidForCurrent, pStartGroup->nID));
		return pStartGroup;
	}

	ASSERT_RETNULL(pStartGroup->nNumChildAffixGroups > 0);
	int iPickedChildGroup = -1;
	{
		PickerStart(game, picker);
		for (int iChildGroup=0; iChildGroup < pStartGroup->nNumChildAffixGroups; iChildGroup++)
		{
			ASSERT_RETNULL(pStartGroup->pChildAffixGroups[iChildGroup]);

			if (TESTBIT(game->pAffixGroupValidForCurrent, pStartGroup->pChildAffixGroups[iChildGroup]->nID))
				PickerAdd(MEMORY_FUNC_FILELINE(game, iChildGroup, pStartGroup->pChildAffixGroups[iChildGroup]->nWeight));
		}

		iPickedChildGroup = PickerChoose(game);
	}

	if (iPickedChildGroup < 0 || iPickedChildGroup >= pStartGroup->nNumChildAffixGroups)
	{
		return NULL;
	}

	return s_sAffixPickGroup(unit, pStartGroup->pChildAffixGroups[iPickedChildGroup]);
}

//----------------------------------------------------------------------------
// pick a random affix to apply to a unit (item, monster)
// IN
// dwAffixPickFlags:	affix flag (eg. APF_REQUIRED_AFFIX_BIT)
// affix_type:			type of affix from affixtypes.xls (INVALID_ID for no specific type)
// spec:				spawn spec (flags or specific affixes)
// affix_group:			affix group (nGroup in AFFIX_DATA), EXCEL_LINK_INVALID = any group
// OUT
// affix index
//----------------------------------------------------------------------------
int s_AffixPick(
	UNIT * unit,
	DWORD dwAffixPickFlags,
	int affix_type,
	ITEM_SPEC * spec,
	int affix_group)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETINVALID(unit);
	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETINVALID(unit_data);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETINVALID(IS_SERVER(game));

	int level = UnitGetExperienceLevel(unit);

	BOOL hasSpecAffixes = s_sAffixPickHasSpecAffixes(spec);
	if (s_sAffixPickSpecAffixesAreRequired(spec, dwAffixPickFlags) && !hasSpecAffixes)
	{
		return INVALID_LINK;
	}

	BOOL bUseAffixGroups = AppIsHellgate() && UnitIsA(unit, UNITTYPE_ITEM);
	AFFIX_GROUP *pAffixGroup = NULL;

	if (bUseAffixGroups)
	{
		if (!s_sAffixGroupsInitForUnit(game, unit, unit_data, level, dwAffixPickFlags, spec, hasSpecAffixes, affix_type, affix_group))
		{
			return INVALID_LINK;
		}
	}

pickagain:
	if (bUseAffixGroups)
	{
		pAffixGroup = s_sAffixPickGroup(unit, &gAffixGlobals.tHeadAffixGroup);
		if (!pAffixGroup)
		{
			return INVALID_LINK;
		}
	}

	unsigned int numAffixes = 0;
	if (bUseAffixGroups)
	{
		numAffixes = pAffixGroup->nNumAffixIDs;
	}
	else
	{
		numAffixes = ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_AFFIXES);
	}
	if (numAffixes <= 0)
	{
		return INVALID_LINK;
	}

	PickerStart(game, picker);
	for (unsigned int ii = 0; ii < numAffixes; ++ii)
	{
		int nAffixID = ii;
		if (bUseAffixGroups)
		{
			nAffixID = pAffixGroup->pnAffixIDs[ii];
		}
		const AFFIX_DATA * affix_data = AffixGetData(nAffixID);
		if (!affix_data)
		{
			continue;
		}

		if (!bUseAffixGroups)
		{
			// if we're using affix groups affixes we can't use have already been weeded out
			if (!s_sAffixPickCanPickAffix(game, unit, unit_data, level, dwAffixPickFlags, spec, hasSpecAffixes, affix_type, affix_group, nAffixID, affix_data))
			{
				continue;
			}
		}
		else
		{
			if (!TESTBIT(game->pAffixValidForCurrent, nAffixID))
			{
				continue;
			}
		}

		int weight = s_sAffixPickGetWeight(game, unit, spec, affix_data);
		if (weight <= 0)
		{
			continue;
		}

		PickerAdd(MEMORY_FUNC_FILELINE(game, (int)nAffixID, weight));
	}

	int affix = PickerChoose(game);
	if (affix < 0)
	{
		if (hasSpecAffixes && !TESTBIT(spec->dwFlags, ISF_AFFIX_SPEC_ONLY_BIT))
		{
			hasSpecAffixes = FALSE;
			goto pickagain;
		}
		return INVALID_LINK;
	}

	if (hasSpecAffixes)
	{
		s_sAffixPickRemoveAffixFromSpec(spec, affix);
	}

	// apply affix or attach affix
	return s_AffixPickSpecific(unit, affix, spec);

#else

	return INVALID_LINK;	

#endif	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AffixGetStringListOnUnit(
	UNIT *pUnit,
	char *pszBuffer,
	int nBufferSize)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferSize > 0, "Invalid buffer size" );

	// init result
	pszBuffer[ 0 ] = 0;

	// get the attached and applied affixes
	for (int i = 0; i < AF_NUM_FAMILY; ++i)
	{
		AFFIX_FAMILY eFamily = (AFFIX_FAMILY)i;

		// get stat for affix family
		int nStat = AffixGetNameStat( eFamily );		

		// get list of affixes
		STATS_ENTRY tStatsList[ MAX_AFFIX_ARRAY_PER_UNIT ];	
		int nCount = UnitGetStatsRange( pUnit, nStat, 0, tStatsList, AffixGetMaxAffixCount() );

		// add each affix
		BOOL bFirstAffix = TRUE;
		for (int j = 0; j < nCount; ++j)
		{
			int nAffix = tStatsList [ j ].value;
			ASSERTX_CONTINUE( nAffix != INVALID_LINK, "Expected affix link" );
			const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
			ASSERTV_CONTINUE( pAffixData, "Expected affix data (%d)", nAffix );

			// add header if first affix
			if (bFirstAffix == TRUE)
			{
				PStrCat( pszBuffer, eFamily == AF_ATTACHED ? " Attached: " : " Applied: ", nBufferSize);
				bFirstAffix = FALSE;
			}
			else
			{
				PStrCat( pszBuffer, ", ", nBufferSize );
			}

			// cat name
			PStrCat( pszBuffer, pAffixData->szName, nBufferSize );			

		}

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCommonAffixApply(
	UNIT * unit,
	int affix,
	DWORD dwAffixApplyFlags)
{
	ASSERT_RETINVALID(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETINVALID(game);

	const AFFIX_DATA * affix_data = AffixGetData(affix);
	ASSERT_RETINVALID(affix_data);

	int nAffixCount = AffixGetAffixCount( unit, AF_ALL_FAMILIES );
	if (nAffixCount >= AffixGetMaxAffixCount())
	{
		if (sgbAssertOnTooManyAffixes)
		{
			const int MAX_STRING = 512;
			char szAffixList[ MAX_STRING ];
			AffixGetStringListOnUnit( unit, szAffixList, MAX_STRING );
			ASSERTV_RETINVALID(
				nAffixCount < AffixGetMaxAffixCount(), 
				"Cannot APPLY affix (%s) to item (%s) [id=%d].  Too many affixes - Applied=%d, Attached=%d, Max=%d, List=%s.",
				affix_data->szName,		
				UnitGetDevName( unit ),
				UnitGetId( unit ),
				AffixGetAffixCount( unit, AF_APPLIED ),
				AffixGetAffixCount( unit, AF_ATTACHED ),
				AffixGetMaxAffixCount(),
				szAffixList);
		}
		return INVALID_LINK;
	}

	int state = affix_data->nState;
	if (state != INVALID_ID && !UnitHasState(game, unit, state) )
	{
		if( IS_SERVER(game))
		{
			s_StateSet(unit, unit, affix_data->nState, 0);
		}
		else if( IS_CLIENT(game) )
		{
			c_StateSet(unit, unit, affix_data->nState, 0, 0);
		}
	}

	// create a statslist to add to the item
	STATS * statslist = StatsListInit(game);
	ASSERT_RETINVALID(statslist);
	StatsListAdd(unit, statslist, FALSE, SELECTOR_AFFIX);

	StatsSetStat(game, statslist, STATS_AFFIX_ID, 0, affix);

	// set applied affix
	UnitSetStat(unit, STATS_APPLIED_AFFIX, AffixGetAffixCount(unit, AF_APPLIED), affix);	
	
	// apply level from affix
	if (affix_data->nItemLevel > 0)
	{
		// in order to set the item level, we can't be instructed explicitly to not set it
		if (TESTBIT( dwAffixApplyFlags, AAF_DO_NOT_SET_ITEM_LEVEL ) == FALSE)
		{
			UnitSetExperienceLevel( unit, affix_data->nItemLevel );
		}
	}
	
	// apply properties
	for (int ii = 0; ii < AFFIX_PROPERTIES; ii++)
	{
		if (affix_data->codePropCond[ii] != NULL_CODE)
		{
			int codelen = 0;
			BYTE * code = ExcelGetScriptCode(game, DATATABLE_AFFIXES, affix_data->codePropCond[ii], &codelen);
			if (code)
			{
				if (!VMExecI(game, unit, statslist, code, codelen))
				{
					continue;
				}
			}
		}
		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(game, DATATABLE_AFFIXES, affix_data->codeProperty[ii], &codelen);
		if (code)
		{
			VMExecI(game, unit, statslist, code, codelen);
		}
	}

	// return the affix
	return affix;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_AffixApply(
	UNIT * unit,
	int affix)
{
	ASSERT_RETINVALID(unit && IS_CLIENT(unit));
	return sCommonAffixApply(unit, affix, 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int s_AffixApply(
	UNIT * unit,
	int affix,
	DWORD dwAffixApplyFlags /*= 0*/)
{
	ASSERT_RETINVALID(unit && IS_SERVER(unit));
	return sCommonAffixApply(unit, affix, dwAffixApplyFlags);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int s_AffixAttach(
	UNIT * unit,
	int affix)
{
	ASSERT_RETINVALID(unit && IS_SERVER(unit));
	ASSERTX_RETINVALID(affix != INVALID_LINK, "Expected affix link");
	const AFFIX_DATA *pAffixData = AffixGetData( affix );
	ASSERTX_RETINVALID( pAffixData, "Expected affix data" );
	
	// how many affixes are both attached and applied already to this item, we can't have
	// more than the max of those two combined	
	int nAffixCount = AffixGetAffixCount( unit, AF_ALL_FAMILIES );
	if (nAffixCount >= AffixGetMaxAffixCount())
	{
		if (sgbAssertOnTooManyAffixes)
		{
			const int MAX_STRING = 512;
			char szAffixList[ MAX_STRING ];
			AffixGetStringListOnUnit( unit, szAffixList, MAX_STRING );
			ASSERTV_RETINVALID(
				nAffixCount < AffixGetMaxAffixCount(), 
				"Cannot ATTACH affix (%s) to item (%s) [id=%d].  Too many affixes - Applied=%d, Attached=%d, Max=%d, List=%s.",
				pAffixData->szName,		
				UnitGetDevName( unit ),
				UnitGetId( unit ),
				AffixGetAffixCount( unit, AF_APPLIED ),
				AffixGetAffixCount( unit, AF_ATTACHED ),
				AffixGetMaxAffixCount(),
				szAffixList);
		}
		return INVALID_LINK;
	}
	
	// set a stat for the attached affix	
	int nNumAttachedAffixes = AffixGetAffixCount( unit, AF_ATTACHED );
	UnitSetStat( unit, STATS_ATTACHED_AFFIX_HIDDEN, nNumAttachedAffixes, affix );
	
	return affix;	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAffixAttachedRemove(
	UNIT *pUnit,
	int nAffix)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( nAffix != INVALID_LINK, "Expected affix link" );
		
	// get the stat tracking the requested affixes
	int nStat = AffixGetNameStat( AF_ATTACHED );

	// get the stats list	
	STATS_ENTRY tStatsList[ MAX_AFFIX_ARRAY_PER_UNIT ];	
	int nCount = UnitGetStatsRange( pUnit, nStat, 0, tStatsList, AffixGetMaxAffixCount() );

	// find this affix in the attached stats entries
	for (int i = 0; i < nCount; ++i)
	{
		int nAffixOther = tStatsList[ i ].value;	
		if (nAffix == nAffixOther)
		{
			int nParam = tStatsList[ i ].GetParam();
			UnitClearStat( pUnit, STATS_ATTACHED_AFFIX_HIDDEN, nParam );
			return;
		}
	}
#endif			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_AffixApplyAttached(
	UNIT *pUnit)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// get all of our attached affixes
	STATS_ENTRY tStatsListAffixes[ MAX_AFFIX_ARRAY_PER_UNIT ];
	int nAttachedAffixCount = UnitGetStatsRange(
		pUnit, 
		STATS_ATTACHED_AFFIX_HIDDEN, 
		0, 
		tStatsListAffixes, 
		AffixGetMaxAffixCount());
	
	// apply all the affixes
	for (int i = 0; i < nAttachedAffixCount; ++i)
	{
		// get affix 
		int nAffix = tStatsListAffixes[ i ].value;
		
		if (nAffix != INVALID_ID)
		{	
			// clear this attached affix
			sAffixAttachedRemove( pUnit, nAffix );
			
			// apply the affix	
			s_AffixApply( pUnit, nAffix );
		}
	}
#endif					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_AffixRemoveAll(
	UNIT *pUnit,
	AFFIX_FAMILY eFamily)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	for (int i = 0; i < AF_NUM_FAMILY; ++i)
	{
		AFFIX_FAMILY eFamilyCurrent = (AFFIX_FAMILY)i;
		
		// must match the family passed in (or the family passed in is "all" families)
		if (eFamily == eFamilyCurrent || eFamily == AF_ALL_FAMILIES)
		{
		
			// remove all the attached affixes
			STATS_ENTRY tStatsList[ MAX_AFFIX_ARRAY_PER_UNIT ];	
			int nStat = AffixGetNameStat( eFamilyCurrent );	
			int nCount = UnitGetStatsRange( pUnit, nStat, 0, tStatsList, AffixGetMaxAffixCount() );
			for (int i = 0; i < nCount; ++i)
			{
				int nParam = tStatsList[ i ].GetParam();
				UnitClearStat( pUnit, nStat, nParam );
			}

		}
		
	}
	
}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const AFFIX_DATA * AffixGetData(
	int affix)
{
	return (const AFFIX_DATA *)ExcelGetData(NULL, DATATABLE_AFFIXES, affix);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD AffixGetCode(
	int nAffix)
{
	if (nAffix == INVALID_LINK)
	{
		return INVALID_CODE;
	}
	const AFFIX_DATA *pAffixData = AffixGetData( nAffix );
	ASSERTX_RETINVALID( pAffixData, "Expected affix data" );
	return pAffixData->dwCode;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int AffixGetByCode(
	DWORD dwCode)
{
	return ExcelGetLineByCode(NULL, DATATABLE_AFFIXES, dwCode);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int AffixGetByType(
	GAME * game,
	int affix_type,
	DWORD code)
{
	AFFIX_DATA key;
	key.nAffixType[0] = affix_type;
	for (unsigned int ii = 1; ii < AFFIX_AFFIXTYPES; ++ii)
	{
		key.nAffixType[1] = EXCEL_LINK_INVALID;
	}
	key.dwCode = code;
	return ExcelGetLineByKey(game, DATATABLE_AFFIXES, &key, sizeof(key), 1, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AffixIsFocus(
	int nAffix)
{
#if ISVERSION(DEVELOPMENT)
	if (gnAffixFocus != INVALID_LINK && nAffix == gnAffixFocus)
	{
		return TRUE;
	}
#endif
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int AffixGetNameStat(
	AFFIX_FAMILY eFamily)
{

	int nStat = INVALID_LINK;
	if (eFamily == AF_APPLIED)
	{
		nStat = STATS_APPLIED_AFFIX;	
	}
	else if (eFamily == AF_ATTACHED)
	{
		nStat = STATS_ATTACHED_AFFIX_HIDDEN;
	}
	
	// must have a stat
	ASSERTX_RETZERO( nStat != INVALID_LINK, "Unknown stat to govern unit affix count" );
	return nStat;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int AffixGetAffixCount(
	UNIT *pUnit,
	AFFIX_FAMILY eFamily)
{

	if (eFamily == AF_ALL_FAMILIES)
	{
		int nCount = 0;
		for (int i = 0; i < AF_NUM_FAMILY; ++i)
		{
			AFFIX_FAMILY eFamilyMember = (AFFIX_FAMILY)i;
			nCount += AffixGetAffixCount( pUnit, eFamilyMember );
		}
		return nCount;
	}
	else
	{
	
		// get the stat tracking the requested affixes
		int nStat = AffixGetNameStat( eFamily );

		// get the stats list	
		STATS_ENTRY tStatsList[ MAX_AFFIX_ARRAY_PER_UNIT ];	
		int nCount = UnitGetStatsRange( pUnit, nStat, 0, tStatsList, AffixGetMaxAffixCount() );
		return nCount;

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int AffixGetByIndex(
	UNIT *pUnit,
	AFFIX_FAMILY eFamily,
	int nIndex)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	STATS_ENTRY tStatsList[ MAX_AFFIX_ARRAY_PER_UNIT ];	
	int nStat = AffixGetNameStat( eFamily );	
	int nCount = UnitGetStatsRange( pUnit, nStat, 0, tStatsList, AffixGetMaxAffixCount() );
	AUTO_VERSION(
		ASSERTV_RETINVALID(nIndex < nCount,"Invalid affix index (count=%d, index=%d)", nCount, nIndex),
		if (nIndex >= nCount) return INVALID_ID );	//	server log spam
	return tStatsList[ nIndex ].value; //.GetStat();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AffixGetMagicName(
	int nAffix,
	DWORD dwStringAttributes,
	WCHAR *puszString,
	int nMaxSize,
	BOOL bClean)
{
	ASSERTX_RETURN( nAffix != INVALID_LINK, "Expected affix" );
	ASSERTX_RETURN( puszString, "Expected string storage" );

	// init string
	puszString[ 0 ] = 0;
	
	// get affix data
	const AFFIX_DATA *pAffixData = AffixGetData( nAffix );

	// get string matching to attributes of item base name
	if (pAffixData->nStringName[ ANT_MAGIC ] != INVALID_LINK)
	{
	
		// get name
		const WCHAR *puszAffixName = NULL;
		if (bClean)
		{
		
			// get the string key of the affix name
			const char *pszKey = StringTableGetKeyByStringIndex( pAffixData->nStringName[ ANT_MAGIC ] );
			ASSERTX_RETURN( pszKey, "Cannot find string key for affix name" );
			
			// construct a new string key for the clean desc names
			char szKeyClean[ MAX_STRING_KEY_LENGTH ];
			PStrPrintf( szKeyClean, MAX_STRING_KEY_LENGTH, "%s desc", pszKey );
			
			// get the string
			puszAffixName = StringTableGetStringByKeyVerbose( 
				szKeyClean, 
				dwStringAttributes, 
				0,
				NULL, 
				NULL);
				
		}
		else
		{

			// get the string
			puszAffixName = StringTableGetStringByIndexVerbose( 
				pAffixData->nStringName[ ANT_MAGIC ], 
				dwStringAttributes, 
				0,
				NULL, 
				NULL);
		
		}
		
		// copy result to param
		ASSERTX_RETURN( puszAffixName, "Expected affix name string" );
		if (puszAffixName[ 0 ] != 0)
		{
			PStrCopy( puszString, puszAffixName, nMaxSize );
		}
		else
		{
			PStrPrintf( puszString, nMaxSize, L"MISSING AFFIX STRING '%S'", pAffixData->szName );
		}
			
	}
	
}				

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AffixNameIsPossessive(
	const AFFIX_DATA *pAffixData,
	int nPossessiveAttributeBit)
{
	ASSERTX_RETFALSE( pAffixData, "Expected affix data" );
	
	if (nPossessiveAttributeBit != NONE)
	{
	
		// go through name strings
		for (int i = 0; i < ANT_NUM_TYPES; ++i)
		{
			AFFIX_NAME_TYPE eType = (AFFIX_NAME_TYPE)i;
			int nStringIndex = pAffixData->nStringName[ eType ];
			if (nStringIndex != INVALID_LINK)
			{
				DWORD dwAttributesOfString = 0;
				BOOL bFound = FALSE;
				StringTableGetStringByIndexVerbose( nStringIndex, 0, 0, &dwAttributesOfString, &bFound );
				if (bFound)
				{
					if (TESTBIT( dwAttributesOfString, nPossessiveAttributeBit ))
					{
						return TRUE;
					}
					
				}
				
			}
			
		}

	}
	
	return FALSE;
		
}

int AffixGetMaxAffixCount()
{
	if( AppIsHellgate() )
		return MAX_AFFIX_PER_UNIT_HELLGATE;

	return MAX_AFFIX_PER_UNIT_MYTHOS;
}