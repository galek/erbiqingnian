//----------------------------------------------------------------------------
// room_layout.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "room_layout.h"
#include "room.h"
#include "room_path.h"
#include "prime.h"
#include "game.h"
#include "picker.h"
#include "units.h"
#include "level.h"
#include "e_definition.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomLayoutGetSpawnPosition( 
	const ROOM_LAYOUT_GROUP * pSpawnPoint, 
	const ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation,
	ROOM * pRoom, 
	VECTOR & vSpawnPosition, 
	VECTOR & vSpawnDirection,
	VECTOR & vSpawnUp )
{
	// transform spawn point position into world space
	MatrixMultiply( 
		&vSpawnPosition, 
		&pSpawnOrientation->vPosition, 
		&pRoom->tWorldMatrix );	

	//vSpawnUp = AppIsTugboat() ? VECTOR(0, 0, 1) : pSpawnOrientation->vNormal;
	vSpawnUp = pSpawnOrientation->vNormal;

	MATRIX zRot;
	VECTOR vRotated;
	VECTOR vToRotate(0, 1, 0);
	if(vSpawnUp != VECTOR(0, 0, 1))
	{
		VECTOR vUp;
		VectorCross(vUp, vSpawnUp, vToRotate);
		VectorCross(vToRotate, vUp, vSpawnUp);
	}

	float fRotation = pSpawnOrientation->fRotation;
	// get spawn point orientation
	MatrixRotationAxis(zRot, vSpawnUp, (fRotation * PI) / 180.0f);
	MatrixMultiplyNormal( &vRotated, &vToRotate, &zRot );
	MatrixMultiplyNormal( &vSpawnDirection, &vRotated, &pRoom->tWorldMatrix );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sDoCopyRoomLayout(
	ROOM_LAYOUT_GROUP * pFrom,
	ROOM_LAYOUT_GROUP * pTo,
	BOOL bCopyData = TRUE)
{
	if(bCopyData)
	{
		ROOM_LAYOUT_GROUP * pSavePointer = pTo->pGroups;
		int nSaveGroupCount = pTo->nGroupCount;
		MemoryCopy(pTo, sizeof(ROOM_LAYOUT_GROUP), pFrom, sizeof(ROOM_LAYOUT_GROUP));
		pTo->pGroups = pSavePointer;
		pTo->nGroupCount = nSaveGroupCount;
		pTo->nModelId = -1;
		pTo->nLayoutId = INVALID_ID;
		pTo->bFollowed = FALSE;
		pTo->bReadOnly = FALSE;
	}

	if (pFrom->nGroupCount)
	{
		pTo->pGroups = (ROOM_LAYOUT_GROUP*)REALLOC(NULL, pTo->pGroups, (pFrom->nGroupCount + pTo->nGroupCount)*sizeof(ROOM_LAYOUT_GROUP));
		memclear(&pTo->pGroups[pTo->nGroupCount], pFrom->nGroupCount*sizeof(ROOM_LAYOUT_GROUP));
		for(int i=pTo->nGroupCount; i<(pTo->nGroupCount + pFrom->nGroupCount); i++)
		{
			sDoCopyRoomLayout(&pFrom->pGroups[i - pTo->nGroupCount], &pTo->pGroups[i], TRUE);
		}
		pTo->nGroupCount += pFrom->nGroupCount;
	}
}

//----------------------------------------------------------------------------
// This function is not static on purpose
//----------------------------------------------------------------------------
static void sRoomLayoutFixLinks(
	ROOM_LAYOUT_GROUP * pGroup)
{
	for(int i=0; i<pGroup->nGroupCount; i++)
	{
		if(pGroup->pGroups[i].eType == ROOM_LAYOUT_ITEM_LAYOUTLINK)
		{
			// Get the linked definition
			ROOM_LAYOUT_GROUP_DEFINITION * pLinkDef = (ROOM_LAYOUT_GROUP_DEFINITION *)DefinitionGetByName(DEFINITION_GROUP_ROOM_LAYOUT, pGroup->pGroups[i].pszName);
			ASSERT(pLinkDef);

			// GS - 9-11-2006
			// For some reason, pLinkDef is returning NULL, even though it's supposed to be loaded synchronously.  Something is Very Bad[tm] here,
			// but I'm just putting this if() here to make sure that it at least doesn't crash in these cases.
			if(pLinkDef)
			{
				// Copy the linked definition into this one
				sDoCopyRoomLayout(&pLinkDef->tGroup, &pGroup->pGroups[i], FALSE);
			}

			// Convert this item into a group.  If we traverse this layout definition again, then
			// we don't want to re-perform the link, and the data has already been copied into the
			// existing definition.
			pGroup->pGroups[i].eType = ROOM_LAYOUT_ITEM_GROUP;
		}

		sRoomLayoutFixLinks(&pGroup->pGroups[i]);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomLayoutDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad)
{
	// Yes, you read that right -- we only do these things in Prime, NOT in Hammer.
	// The reason is that the positions need to be fixed up for the layout selection stuff
	// in Prime, but Hammer needs to edit the raw values.
	if (!AppIsHammer())
	{
		ROOM_LAYOUT_GROUP_DEFINITION * pDefinition = (ROOM_LAYOUT_GROUP_DEFINITION *)pDef;
		if (pDefinition->bFixup)
		{
			sRoomLayoutFixLinks(&pDefinition->tGroup);
		}
	}
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sRoomLayoutFixupFree(
	GAME * pGame,
	ROOM_LAYOUT_FIXUP * pFixup)
{
	ASSERT_RETURN(pFixup);
	ASSERT_DO(pFixup->pGroups != NULL || pFixup->nGroupCount == 0)
	{
		pFixup->nGroupCount = 0;
		return;
	}
	for(int i=0; i<pFixup->nGroupCount; i++)
	{
		sRoomLayoutFixupFree(pGame, &pFixup->pGroups[i]);
	}
	GFREE(pGame, pFixup->pGroups);
	pFixup->pGroups = NULL;
	pFixup->nGroupCount = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sRoomLayoutSetReadOnly(ROOM_LAYOUT_GROUP* pGroup,BOOL bReadOnly, BOOL bSkipIfLayoutLink)
{
	//We want to set the head node for the group (layoutlink sub node will be skipped if bSkipIfLayoutLink)
	pGroup->bReadOnly = bReadOnly;
	
	if(!bReadOnly && pGroup->eType == ROOM_LAYOUT_ITEM_LAYOUTLINK && bSkipIfLayoutLink)
	{
		return;
	}
	
	for(int i = 0; i < pGroup->nGroupCount; i++)
	{
		sRoomLayoutSetReadOnly(&(pGroup->pGroups[i]), bReadOnly, TRUE);
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomLayoutFixPosition(
	GAME * pGame,
	ROOM_LAYOUT_FIXUP * pFixup,
	ROOM_LAYOUT_GROUP * pGroup,
	const MATRIX &matAlign,
	RAND & tRand,
	BOOL bForceFixup = FALSE)
{
	if(!pFixup->pGroups)
	{
		pFixup->pLayoutGroup = pGroup;
		if(pGroup->nGroupCount > 0)
		{
			pFixup->pGroups = (ROOM_LAYOUT_FIXUP*)GMALLOCZ(pGame, pGroup->nGroupCount * sizeof(ROOM_LAYOUT_FIXUP));
		}
		else
		{
			pFixup->pGroups = NULL;
		}
		pFixup->nGroupCount = pGroup->nGroupCount;
	}

	for(int i=0; i<pGroup->nGroupCount; i++)
	{
		BOOL bForceFixupChildren = bForceFixup;
		if (!pFixup->pGroups[i].bFixedUp || bForceFixup)
		{
			VECTOR vNorm = pGroup->pGroups[i].vNormal;
			MatrixMultiplyNormal(&pFixup->pGroups[i].vFixupNormal, &vNorm, &matAlign);
			pFixup->pGroups[i].vFixupNormal += pFixup->vFixupNormal;

			if(pGroup->pGroups[i].dwFlags & ROOM_LAYOUT_FLAG_RANDOM_ROTATIONS)
			{
				pFixup->pGroups[i].fFixupRotation = RandGetFloat(tRand, -180.0f, 180.0f);
				bForceFixupChildren = TRUE;
			}
			else
			{
				pFixup->pGroups[i].fFixupRotation = pGroup->pGroups[i].fRotation + pFixup->fFixupRotation;
			}

			VECTOR vPos = pGroup->pGroups[i].vPosition;
			MatrixMultiply(&pFixup->pGroups[i].vFixupPosition, &vPos, &matAlign);
			pFixup->pGroups[i].vFixupPosition += pFixup->vFixupPosition;
			
			pFixup->pGroups[i].bFixedUp = bForceFixup ? FALSE : TRUE;
		}

		if(pGroup->pGroups[i].eType == ROOM_LAYOUT_ITEM_LAYOUTLINK)
		{
			ASSERTX(FALSE, "Why do we have a layout link left?");
		}

		MATRIX matId;
		MatrixIdentity(&matId);

		VECTOR vNormal = pFixup->pGroups[i].vFixupNormal;

		MATRIX matAlignChildren;
		GetAlignmentMatrix(&matAlignChildren, matId, VECTOR(0), vNormal, pFixup->pGroups[i].fFixupRotation);

		RoomLayoutFixPosition(pGame, &pFixup->pGroups[i], &pGroup->pGroups[i], matAlignChildren, tRand, bForceFixupChildren);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomLayoutDefinitionFixPosition(
	GAME * pGame,
	ROOM_LAYOUT_FIXUP * pFixup,
	ROOM_LAYOUT_GROUP_DEFINITION * pDef,
	RAND & tRand)
{
	ROOM_LAYOUT_GROUP_DEFINITION * pDefinition = (ROOM_LAYOUT_GROUP_DEFINITION *)pDef;
	if (pDefinition->bFixup)
	{
		MATRIX matId;
		MatrixIdentity(&matId);
		RoomLayoutFixPosition(pGame, pFixup, &pDefinition->tGroup, matId, tRand);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sRoomLayoutFixPathNode(
	ROOM_LAYOUT_GROUP * pGroup,
	ROOM_LAYOUT_FIXUP * pFixup,
	ROOM * pRoom)
{
	for(int i=0; i<pGroup->nGroupCount; i++)
	{
		if (!pFixup->pGroups[i].pNearestNode)
		{
			pFixup->pGroups[i].pNearestNode = RoomGetNearestPathNodeRoomSpace(pRoom, pFixup->pGroups[i].vFixupPosition);
		}

		sRoomLayoutFixPathNode(&pGroup->pGroups[i], &pFixup->pGroups[i], pRoom);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
typedef void (*AddToArrayFunc)(GAME*, ROOM*, ROOM_LAYOUT_ITEM_TYPE, ROOM_LAYOUT_GROUP_DEFINITION*, ROOM_LAYOUT_GROUP*, ROOM_LAYOUT_FIXUP*);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomAddLayoutItemSelectionGeneric(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_LAYOUT_ITEM_TYPE eType,
	ROOM_LAYOUT_GROUP_DEFINITION * pDef,
	ROOM_LAYOUT_GROUP * pItem,
	ROOM_LAYOUT_FIXUP * pFixup)
{
	//lets set the theme for the item if it doesn't have one lets check the parent.
	//This IS NOT the best place to do this. Should ask colin ~ marsh
	if( AppIsTugboat() &&
		//eType != ROOM_LAYOUT_ITEM_GROUP &&
		pItem->nTheme <= 0 )
	{
		ROOM_LAYOUT_GROUP *pParent = pItem->pParent;
		while( pParent )
		{
			if( pParent->nTheme > 0 )
			{
				if( pParent->dwFlags & ROOM_LAYOUT_FLAG_NOT_THEME )									
					pItem->dwFlags = pItem->dwFlags | ROOM_LAYOUT_FLAG_NOT_THEME;				
				pItem->nTheme = pParent->nTheme;
				break;
			}
			pParent = pParent->pParent;
		}
	}
	int nCurrentCount = pRoom->pLayoutSelections->pRoomLayoutSelections[eType].nCount;
	pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pGroups = (ROOM_LAYOUT_GROUP**)GREALLOC(pGame, pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pGroups, (nCurrentCount+1)*sizeof(ROOM_LAYOUT_GROUP*));
	ASSERT_RETURN(pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pGroups)
	pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pGroups[nCurrentCount] = pItem;
	pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pOrientations = (ROOM_LAYOUT_SELECTION_ORIENTATION*)GREALLOC(pGame, pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pOrientations, (nCurrentCount+1)*sizeof(ROOM_LAYOUT_SELECTION_ORIENTATION));
	ASSERT_RETURN(pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pOrientations);
	pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pOrientations[nCurrentCount].vPosition = pFixup->vFixupPosition;
	pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pOrientations[nCurrentCount].vNormal = pFixup->vFixupNormal;
	VectorNormalize(pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pOrientations[nCurrentCount].vNormal);
	pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pOrientations[nCurrentCount].fRotation = pFixup->fFixupRotation;
	pRoom->pLayoutSelections->pRoomLayoutSelections[eType].pOrientations[nCurrentCount].pNearestNode = pFixup->pNearestNode;
	pFixup->nOrientationIndex = nCurrentCount;

	pRoom->pLayoutSelections->pRoomLayoutSelections[eType].nCount++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomAddLayoutItemSelectionModel(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_LAYOUT_ITEM_TYPE eType,
	ROOM_LAYOUT_GROUP_DEFINITION * pDef,
	ROOM_LAYOUT_GROUP * pItem,
	ROOM_LAYOUT_FIXUP * pFixup)
{
	pRoom->pLayoutSelections->pPropModelIds = (int*)GREALLOC(pGame, pRoom->pLayoutSelections->pPropModelIds, (pRoom->pLayoutSelections->pRoomLayoutSelections[eType].nCount+1)*sizeof(int));
	ASSERT_RETURN(pRoom->pLayoutSelections->pPropModelIds);
	pRoom->pLayoutSelections->pPropModelIds[pRoom->pLayoutSelections->pRoomLayoutSelections[eType].nCount] = INVALID_ID;
	sRoomAddLayoutItemSelectionGeneric(pGame, pRoom, eType, pDef, pItem, pFixup);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomAddLayoutItemSelectionLink(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_LAYOUT_ITEM_TYPE eType,
	ROOM_LAYOUT_GROUP_DEFINITION * pDef,
	ROOM_LAYOUT_GROUP * pLayoutLink,
	ROOM_LAYOUT_FIXUP * pFixup)
{
	ASSERTV_RETURN(FALSE, "Adding a Layout Link %s to %s?  Why was this not flattened on load?", pLayoutLink->pszName, pDef->tHeader.pszName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AddToArrayFunc AddFuncs[] =
{
	sRoomAddLayoutItemSelectionGeneric,
	sRoomAddLayoutItemSelectionGeneric,
	sRoomAddLayoutItemSelectionModel,
	sRoomAddLayoutItemSelectionGeneric,
	sRoomAddLayoutItemSelectionGeneric,
	sRoomAddLayoutItemSelectionLink,
	sRoomAddLayoutItemSelectionGeneric,
	sRoomAddLayoutItemSelectionGeneric,
	sRoomAddLayoutItemSelectionGeneric,
};

//----------------------------------------------------------------------------
// Return Values for Layout of theme check - was added to allow for groups not to 
//show even if the level has multiple themes.
//----------------------------------------------------------------------------
const enum EROOM_THEME_CHECK
{
	KROOM_THEME_CHECK_SHOW,
	KROOM_THEME_CHECK_DONT_SHOW,
	KROOM_THEME_CHECK_FORCE_NO_SHOW,
	KROOM_THEME_CHECK_COUNT,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static EROOM_THEME_CHECK sRoomSelectLayoutCheckTheme(
	ROOM_LAYOUT_GROUP * pGroup,
	int nThemeIndex)
{
	ASSERT_RETVAL(pGroup, KROOM_THEME_CHECK_SHOW);

	if(nThemeIndex <= 0)
	{
		if(pGroup->nTheme <= 0)
		{
			return KROOM_THEME_CHECK_SHOW;
		}
		else
		{
			if(pGroup->dwFlags & ROOM_LAYOUT_FLAG_NOT_THEME)
			{
				return KROOM_THEME_CHECK_SHOW;
			}
			return KROOM_THEME_CHECK_DONT_SHOW;
		}
	}
	else
	{
		if(pGroup->dwFlags & ROOM_LAYOUT_FLAG_NO_THEME)
		{
			return KROOM_THEME_CHECK_DONT_SHOW;
		}

		LEVEL_THEME * pLevelTheme = (LEVEL_THEME *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_LEVEL_THEMES, nThemeIndex);
		if(pLevelTheme->bHighlander)
		{
			if(pGroup->nTheme <= 0 || !ThemeIsA(nThemeIndex, pGroup->nTheme))
			{
				return KROOM_THEME_CHECK_DONT_SHOW;
			}
		}
		else
		{
			if(pGroup->nTheme <= 0)
			{
				return KROOM_THEME_CHECK_SHOW;
			}
		}

		if(pGroup->dwFlags & ROOM_LAYOUT_FLAG_NOT_THEME)
		{
			if(ThemeIsA(nThemeIndex, pGroup->nTheme))
			{
				//if this is true, then Force it to never show even if it has other themes that tell it to
				return KROOM_THEME_CHECK_FORCE_NO_SHOW;   
			}
			return KROOM_THEME_CHECK_SHOW;
		}
		else
		{
			if(ThemeIsA(nThemeIndex, pGroup->nTheme))
			{
				return KROOM_THEME_CHECK_SHOW;
			}
			return KROOM_THEME_CHECK_DONT_SHOW;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomSelectLayoutCheckThemeArray(
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP * pGroup,
	int * pnThemes,
	int nThemeIndexCount)
{
	if( pRoom && 
		TESTBIT( pRoom->pdwFlags, ROOM_IGNORE_ALL_THEMES ) )
	{
		return TRUE;
	}

	if(nThemeIndexCount <= 0 || !pnThemes)
	{
		return (sRoomSelectLayoutCheckTheme(pGroup, INVALID_ID) == KROOM_THEME_CHECK_SHOW)?TRUE:FALSE;
	}
	BOOL returnValue( FALSE );
	for(int i=0; i<nThemeIndexCount; i++)
	{
		switch( sRoomSelectLayoutCheckTheme(pGroup, pnThemes[i]) )
		{
		case KROOM_THEME_CHECK_SHOW:
			returnValue = TRUE;
			break;
		case KROOM_THEME_CHECK_DONT_SHOW:
			break;
		case KROOM_THEME_CHECK_FORCE_NO_SHOW: //no way can this group be shown
			return FALSE;			
		}

	}
	return returnValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomSelectLayoutCheckPropExists(
	GAME * pGame,
	ROOM_LAYOUT_GROUP * pGroup)
{
	if(pGroup->eType == ROOM_LAYOUT_ITEM_GRAPHICMODEL)
	{
		if(pGroup->bPropIsChecked)
		{
			return pGroup->bPropIsValid;
		}
		else
		{
			char pszNameNoExtension[MAX_XML_STRING_LENGTH];
			PStrRemoveExtension(pszNameNoExtension, MAX_XML_STRING_LENGTH, pGroup->pszName);
			//const void * pExists = ExcelGetDataByStringIndex(EXCEL_CONTEXT(pGame), DATATABLE_PROPS, pszNameNoExtension);
			const void * pExists = RoomFileGetRoomIndex(pszNameNoExtension);
			pGroup->bPropIsChecked = TRUE;
			if(pExists)
			{
				pGroup->bPropIsValid = TRUE;
				return TRUE;
			}
			else
			{
				pGroup->bPropIsValid = FALSE;
				return FALSE;
			}
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRoomPickLayout(
	GAME* game,
	ROOM *pRoom,
	ROOM_LAYOUT_ITEM_TYPE eType,
	ROOM_LAYOUT_GROUP * pLayoutSet,
	int nCount,
	RAND& tRand,
	int * pnThemes,
	int nThemeIndexCount)
{
	PickerStart(game, picker);
	for (int ii = 0; ii < nCount; ii++)
	{
		if(!sRoomSelectLayoutCheckThemeArray( pRoom, &pLayoutSet[ii], pnThemes, nThemeIndexCount))
			continue;

		if(!sRoomSelectLayoutCheckPropExists(game, &pLayoutSet[ii]))
			continue;

		if (pLayoutSet[ii].eType == eType && pLayoutSet[ii].nWeight > 0 && !(pLayoutSet[ii].dwFlags & ROOM_LAYOUT_FLAG_WEIGHT_PERCENTAGE))
		{
			PickerAdd(MEMORY_FUNC_FILELINE(game, ii, pLayoutSet[ii].nWeight));
		}
	}
	int pick = PickerChoose(game, tRand);
	ASSERT_RETINVALID(pick < nCount);
	return pick;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomSelectLayoutTraverse(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP_DEFINITION * pDef,
	ROOM_LAYOUT_GROUP * pLayoutGroup,
	ROOM_LAYOUT_FIXUP * pFixup,
	RAND& tRand,
	int * pnThemes,
	int nThemeIndexCount)
{
	// Add the global items
	// Go through each of this group's children
	for(int i=0; i<pLayoutGroup->nGroupCount; i++)
	{
		if(!sRoomSelectLayoutCheckThemeArray( pRoom, &pLayoutGroup->pGroups[i], pnThemes, nThemeIndexCount))
			continue;

		if(!sRoomSelectLayoutCheckPropExists(pGame, &pLayoutGroup->pGroups[i]))
			continue;

		// If the layout item has a weight of -1 then select it and traverse it
		if(pLayoutGroup->pGroups[i].nWeight == -1)
		{
			// Add this one
			if (AddFuncs[pLayoutGroup->pGroups[i].eType])
			{
				pLayoutGroup->pGroups[i].pParent = pLayoutGroup;	//set it's parent
				(*AddFuncs[pLayoutGroup->pGroups[i].eType])(pGame, pRoom, pLayoutGroup->pGroups[i].eType, pDef, &pLayoutGroup->pGroups[i], &pFixup->pGroups[i]);
			}
			
			// Call this function on the layout group
			sRoomSelectLayoutTraverse(pGame, pRoom, pDef, &pLayoutGroup->pGroups[i], &pFixup->pGroups[i], tRand, pnThemes, nThemeIndexCount);
		}

		// If the layout item has a flag that indicates that the weight is a percentage, then
		// treat it differently from the rest
		if(pLayoutGroup->pGroups[i].dwFlags & ROOM_LAYOUT_FLAG_WEIGHT_PERCENTAGE)
		{
			int nRand = RandGetNum(tRand, 100);
			if(nRand <= pLayoutGroup->pGroups[i].nWeight)
			{
				// Add this one
				if (AddFuncs[pLayoutGroup->pGroups[i].eType])
				{
					pLayoutGroup->pGroups[i].pParent = pLayoutGroup;	//set parent
					(*AddFuncs[pLayoutGroup->pGroups[i].eType])(pGame, pRoom, pLayoutGroup->pGroups[i].eType, pDef, &pLayoutGroup->pGroups[i], &pFixup->pGroups[i]);
				}
				
				// Call this function on the layout group
				sRoomSelectLayoutTraverse(pGame, pRoom, pDef, &pLayoutGroup->pGroups[i], &pFixup->pGroups[i], tRand, pnThemes, nThemeIndexCount);
			}
		}
	}

	// Once for each layout item type
	for(int i=ROOM_LAYOUT_ITEM_GROUP; i<ROOM_LAYOUT_ITEM_MAX; i++)
	{
		// Select a group of the specified type
		int nSelectedLayout = sRoomPickLayout(pGame, pRoom, (ROOM_LAYOUT_ITEM_TYPE)i, pLayoutGroup->pGroups, pLayoutGroup->nGroupCount, tRand, pnThemes, nThemeIndexCount);

		if(nSelectedLayout != INVALID_ID)
		{
			// Add this one
			if (AddFuncs[i])
			{
				pLayoutGroup->pGroups[nSelectedLayout].pParent = pLayoutGroup;	//set parent
				(*AddFuncs[i])(pGame, pRoom, (ROOM_LAYOUT_ITEM_TYPE)i, pDef, &pLayoutGroup->pGroups[nSelectedLayout], &pFixup->pGroups[nSelectedLayout]);
			}

			// Call this function on the layout group
			sRoomSelectLayoutTraverse(pGame, pRoom, pDef, &pLayoutGroup->pGroups[nSelectedLayout], &pFixup->pGroups[nSelectedLayout], tRand, pnThemes, nThemeIndexCount);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomLayoutSelectLayoutItems(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP_DEFINITION * pDef,
	ROOM_LAYOUT_GROUP * pLayoutSet,
	DWORD dwSeed,
	int * pnThemes,
	int nThemeIndexCount)
{
	RandInit(pRoom->tLayoutRand, dwSeed);

	RAND tFixupRand;
	RandInit(tFixupRand, dwSeed);

	ROOM_LAYOUT_FIXUP tFixup;
	structclear(tFixup);
	RoomLayoutDefinitionFixPosition(pGame, &tFixup, pDef, tFixupRand);
	if( AppIsHellgate() )
	{
		sRoomLayoutFixPathNode(&pDef->tGroup, &tFixup, pRoom);
	}
	sRoomSelectLayoutTraverse(pGame, pRoom, pDef, pLayoutSet, &tFixup, pRoom->tLayoutRand, pnThemes, nThemeIndexCount);
	sRoomLayoutFixupFree(pGame, &tFixup);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_LAYOUT_GROUP * RoomGetNodeSet( 
	ROOM * room, 
	const char * pszSetName,
	ROOM_LAYOUT_SELECTION_ORIENTATION ** ppOrientation )
{
	ASSERT_RETNULL( room );
	ASSERT_RETNULL( pszSetName );
	ASSERT_RETNULL( room->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GROUP].nCount );
	ASSERT_RETNULL( ppOrientation );

	for ( int i = 0; i < room->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GROUP].nCount; i++ )
	{
		ROOM_LAYOUT_GROUP * set = room->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GROUP].pGroups[i];
		if ( PStrICmp( set->pszName, pszSetName, MAX_XML_STRING_LENGTH ) == 0 )
		{
			*ppOrientation = &room->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GROUP].pOrientations[i];
			return set;
		}
	}
	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ROOM_LAYOUT_GROUP * RoomGetLabelNode( 
	ROOM * room, 
	const char * pszLabelName,
	ROOM_LAYOUT_SELECTION_ORIENTATION ** ppOrientation )
{
	ASSERT_RETNULL( room );
	ASSERT_RETNULL( pszLabelName );
	ASSERT_RETNULL( ppOrientation );

	for ( int i = 0; i < room->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_LABELNODE].nCount; i++ )
	{
		ROOM_LAYOUT_GROUP * set = room->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_LABELNODE].pGroups[i];
		if ( PStrICmp( set->pszName, pszLabelName, MAX_XML_STRING_LENGTH ) == 0 )
		{
			*ppOrientation = &room->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_LABELNODE].pOrientations[i];
			return set;
		}
	}
	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ROOM_LAYOUT_GROUP * LevelGetLabelNode( 
	LEVEL * level,
	const char * pszLabelName,
	ROOM ** ppRoom,
	ROOM_LAYOUT_SELECTION_ORIENTATION ** ppOrientation)
{
	ASSERT_RETNULL(level);
	ASSERT_RETNULL(pszLabelName);
	ASSERT_RETNULL(ppRoom);
	ASSERT_RETNULL(ppOrientation);

	ROOM * pRoom = LevelGetFirstRoom(level);
	while(pRoom)
	{
		ROOM_LAYOUT_GROUP * pLayoutGroup = RoomGetLabelNode(pRoom, pszLabelName, ppOrientation);
		if(pLayoutGroup)
		{
			*ppRoom = pRoom;
			return pLayoutGroup;
		}
		pRoom = LevelGetNextRoom(pRoom);
	}
	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sRoomLayoutIsValidAINode(
	const VECTOR & vNodePosition,
	const VECTOR & vTargetPosition,
	const VECTOR & vDirectionCheck,
	const VECTOR & vDirectionNode,
	const DWORD dwFlagsCheck,
	const DWORD dwNodeFlags,
	float fMinDistanceSquared,
	float fMaxDistanceSquared)
{
	// If we're looking for a door and we find a crouch, then don't even bother
	if((dwNodeFlags & dwFlagsCheck) == 0)
	{
		return FALSE;
	}

	// If it's too close or too far away, then quit early, too
	float fDistanceSquared = VectorDistanceSquared(vNodePosition, vTargetPosition);
	if(fDistanceSquared < fMinDistanceSquared || fDistanceSquared > fMaxDistanceSquared)
	{
		return FALSE;
	}

	// Calculate the vector to the node from the target
	VECTOR vToNode = vNodePosition - vTargetPosition;
	if(VectorIsZeroEpsilon(vToNode))
	{
		return FALSE;
	}
	VectorNormalize(vToNode);

	// The node must be facing opposite of the check direction
	float fCheck1 = VectorDot(vDirectionCheck, vDirectionNode);
	if(fCheck1 >= 0.0f)
	{
		return FALSE;
	}

	// The node must be in front of the target position
	float fCheck2 = VectorDot(vToNode, vDirectionCheck);
	if(fCheck2 <= 0.0f)
	{
		return FALSE;
	}

	// There's a 90-degree cone that we're declaring to be a legitimate node
	float fCheck3 = VectorDot(vToNode, vDirectionNode);
	if(fCheck3 > -0.7071f)
	{
		return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct AI_NODE_TARGET
{
	enum
	{
		AINODE_DIRECTION,
		AINODE_SIDE1,
		AINODE_SIDE2,
		AINODE_DIRECTIONOPPOSITE,

		AINODE_COUNT,
	};

	ROOM * pRoom;
	ROOM_LAYOUT_GROUP * pGroup;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sRoomGetAINodeFromPoint(
	ROOM * pRoom,
	const VECTOR & vWorldPosition,
	const VECTOR & vTargetWorldPosition,
	const VECTOR & vDirection,
	const VECTOR & vSide1,
	const VECTOR & vSide2,
	const VECTOR & vDirectionOpposite,
	AI_NODE_TARGET * pTargets,
	DWORD dwFlags,
	float fMinDistanceSquared,
	float fMaxDistanceSquared)
{
	ASSERT_RETURN(pRoom && pTargets);

	for ( int i = 0; i < pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_AI_NODE].nCount; i++ )
	{
		ROOM_LAYOUT_GROUP * pNode = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_AI_NODE].pGroups[i];
		ASSERT_CONTINUE(pNode);
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = &pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_AI_NODE].pOrientations[i];
		ASSERT_CONTINUE(pOrientation);

		VECTOR vNodeWorldPosition(0), vNodeDirection(0);
		MatrixMultiply(&vNodeWorldPosition, &pOrientation->vPosition, &pRoom->tWorldMatrix);

		MATRIX mRot;
		MatrixRotationZ(mRot, DegreesToRadians(pNode->fRotation));
		MatrixMultiply(&vNodeDirection, &cgvXAxis, &mRot);
		MatrixMultiply(&vNodeDirection, &vNodeDirection, &pRoom->tWorldMatrix);

		if(sRoomLayoutIsValidAINode(vNodeWorldPosition, vTargetWorldPosition, vDirection, vNodeDirection, dwFlags, pNode->dwFlags, fMinDistanceSquared, fMaxDistanceSquared))
		{
			pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pGroup = pNode;
			pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pRoom = pRoom;
			pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pOrientation = pOrientation;
			break;
		}
		else if(!pTargets[AI_NODE_TARGET::AINODE_SIDE1].pGroup &&
					sRoomLayoutIsValidAINode(vNodeWorldPosition, vTargetWorldPosition, vSide1, vNodeDirection, dwFlags, pNode->dwFlags, fMinDistanceSquared, fMaxDistanceSquared))
		{
			pTargets[AI_NODE_TARGET::AINODE_SIDE1].pGroup = pNode;
			pTargets[AI_NODE_TARGET::AINODE_SIDE1].pRoom = pRoom;
			pTargets[AI_NODE_TARGET::AINODE_SIDE1].pOrientation = pOrientation;
			continue;
		}
		else if(!pTargets[AI_NODE_TARGET::AINODE_SIDE2].pGroup &&
					sRoomLayoutIsValidAINode(vNodeWorldPosition, vTargetWorldPosition, vSide2, vNodeDirection, dwFlags, pNode->dwFlags, fMinDistanceSquared, fMaxDistanceSquared))
		{
			pTargets[AI_NODE_TARGET::AINODE_SIDE2].pGroup = pNode;
			pTargets[AI_NODE_TARGET::AINODE_SIDE2].pRoom = pRoom;
			pTargets[AI_NODE_TARGET::AINODE_SIDE2].pOrientation = pOrientation;
			continue;
		}
		else if(!pTargets[AI_NODE_TARGET::AINODE_DIRECTIONOPPOSITE].pGroup &&
					sRoomLayoutIsValidAINode(vNodeWorldPosition, vTargetWorldPosition, vDirectionOpposite, vNodeDirection, dwFlags, pNode->dwFlags, fMinDistanceSquared, fMaxDistanceSquared))
		{
			pTargets[AI_NODE_TARGET::AINODE_DIRECTIONOPPOSITE].pGroup = pNode;
			pTargets[AI_NODE_TARGET::AINODE_DIRECTIONOPPOSITE].pRoom = pRoom;
			pTargets[AI_NODE_TARGET::AINODE_DIRECTIONOPPOSITE].pOrientation = pOrientation;
			continue;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ROOM_LAYOUT_GROUP * RoomGetAINodeFromPoint( 
	ROOM * pRoom, 
	const VECTOR & vWorldPosition,
	const VECTOR & vTargetWorldPosition,
	DWORD dwFlags,
	ROOM ** pDestinationRoom,
	ROOM_LAYOUT_SELECTION_ORIENTATION ** ppOrientation,
	float fMinDistance,
	float fMaxDistance)
{
	ASSERT_RETNULL( pRoom );
	ASSERT_RETNULL( pDestinationRoom );
	ASSERT_RETNULL( ppOrientation );

	VECTOR vDirection = vWorldPosition - vTargetWorldPosition;
	ASSERT_RETNULL(!VectorIsZeroEpsilon(vDirection));
	VectorNormalize(vDirection);
	VECTOR vDirectionOpposite = -vDirection;

	VECTOR vSide1, vSide2;
	VectorCross(vSide1, vDirection, VECTOR(0, 0, 1));
	vSide2 = -vSide1;

	float fMinDistanceSquared = fMinDistance * fMinDistance;
	float fMaxDistanceSquared = fMaxDistance * fMaxDistance;

	AI_NODE_TARGET pTargets[AI_NODE_TARGET::AINODE_COUNT];
	memclear(pTargets, AI_NODE_TARGET::AINODE_COUNT * sizeof(AI_NODE_TARGET));

	sRoomGetAINodeFromPoint(pRoom, vWorldPosition, vTargetWorldPosition, vDirection, vSide1, vSide2, vDirectionOpposite, pTargets, dwFlags, fMinDistanceSquared, fMaxDistanceSquared);
	if(pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pGroup && pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pRoom)
	{
		*pDestinationRoom = pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pRoom;
		*ppOrientation = pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pOrientation;
		return pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pGroup;
	}

	int nNeighboringRoomCount = RoomGetNearbyRoomCount(pRoom);
	for(int i=0; i<nNeighboringRoomCount; i++)
	{
		ROOM * pNeighboringRoom = RoomGetNearbyRoom(pRoom, i);
		ASSERT_CONTINUE(pNeighboringRoom);

		sRoomGetAINodeFromPoint(pNeighboringRoom, vWorldPosition, vTargetWorldPosition, vDirection, vSide1, vSide2, vDirectionOpposite, pTargets, dwFlags, fMinDistanceSquared, fMaxDistanceSquared);
		if(pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pGroup && pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pRoom)
		{
			*pDestinationRoom = pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pRoom;
			*ppOrientation = pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pOrientation;
			return pTargets[AI_NODE_TARGET::AINODE_DIRECTION].pGroup;
		}
	}

	for(int i=AI_NODE_TARGET::AINODE_SIDE1; i<AI_NODE_TARGET::AINODE_COUNT; i++)
	{
		if(pTargets[i].pGroup && pTargets[i].pRoom)
		{
			*pDestinationRoom = pTargets[i].pRoom;
			*ppOrientation = pTargets[i].pOrientation;
			return pTargets[i].pGroup;
		}
	}
	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RoomLayoutSelectionsInit(
	GAME * pGame,
	ROOM_LAYOUT_SELECTIONS *& pLayoutSelections)
{
	pLayoutSelections = (ROOM_LAYOUT_SELECTIONS *)GMALLOCZ(pGame, sizeof(ROOM_LAYOUT_SELECTIONS));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTrackedSpawnPointInit( 
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint, 
	ROOM_LAYOUT_GROUP *pSpawnPoint)
{
	ASSERTX_RETURN( pTrackedSpawnPoint, "Expected tracked spawn point" );
	pTrackedSpawnPoint->nNumUnits = 0;
	pTrackedSpawnPoint->pidUnits = NULL;
	pTrackedSpawnPoint->pSpawnPoint = pSpawnPoint;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTrackedSpawnPointClearUnits(
	GAME *pGame,
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint)
{
	ASSERTX_RETURN( pTrackedSpawnPoint, "Expected tracked spawn point" );
	
	// free the array and clear the count
	if (pTrackedSpawnPoint->pidUnits)
	{
		GFREE( pGame, pTrackedSpawnPoint->pidUnits );
	}
	pTrackedSpawnPoint->pidUnits = NULL;
	pTrackedSpawnPoint->nNumUnits = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTrackedSpawnPointFree(
	GAME *pGame,
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint)
{
	ASSERTX_RETURN( pTrackedSpawnPoint, "Expected tracked spawn point" );

	// clear what we were tracking
	sTrackedSpawnPointClearUnits( pGame, pTrackedSpawnPoint );	
	
	// no longer a valid tracking spawn point now
	pTrackedSpawnPoint->pSpawnPoint = NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
enum
{
	RLS_FREE_PROP_MODELIDS			= MAKE_MASK(0),
	RLS_FREE_TRACKED_SPAWNPOINTS	= MAKE_MASK(1),
	RLS_FREE_SELECTIONS				= MAKE_MASK(2),
	RLS_FREE_STRUCT					= MAKE_MASK(3),
	
	RLS_FREE_ALL					= RLS_FREE_PROP_MODELIDS | RLS_FREE_TRACKED_SPAWNPOINTS | RLS_FREE_SELECTIONS | RLS_FREE_STRUCT,
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sRoomLayoutSelectionsFree(
	GAME * pGame,
	ROOM_LAYOUT_SELECTIONS * pLayoutSelections,
	DWORD dwFlags)
{
	ASSERT(pLayoutSelections != 0);		// CHB 2006.06.19
	if(TEST_MASK(dwFlags, RLS_FREE_PROP_MODELIDS))
	{
		if(pLayoutSelections->pPropModelIds)
		{
			GFREE(pGame, pLayoutSelections->pPropModelIds);
			pLayoutSelections->pPropModelIds = NULL;
		}
	}

	// free any tracked spawn point data
	if(TEST_MASK(dwFlags, RLS_FREE_TRACKED_SPAWNPOINTS))
	{
		for (int i = 0; i < pLayoutSelections->nNumTrackedSpawnPoints; ++i)
		{
			TRACKED_SPAWN_POINT *pTrackedSpawnPoint = &pLayoutSelections->pTrackedSpawnPoints[ i ];
			sTrackedSpawnPointFree( pGame, pTrackedSpawnPoint );
		}
	
		// free tracked spawn points themselves
		if (pLayoutSelections->pTrackedSpawnPoints)
		{
			GFREE(pGame, pLayoutSelections->pTrackedSpawnPoints);
			pLayoutSelections->pTrackedSpawnPoints = NULL;
		}
	}

	if(TEST_MASK(dwFlags, RLS_FREE_SELECTIONS))
	{
		for(int i=0; i<ROOM_LAYOUT_ITEM_MAX; i++)
		{
			if(pLayoutSelections->pRoomLayoutSelections[i].pGroups)
			{
				GFREE(pGame, pLayoutSelections->pRoomLayoutSelections[i].pGroups);
				pLayoutSelections->pRoomLayoutSelections[i].pGroups = NULL;
				pLayoutSelections->pRoomLayoutSelections[i].nCount = 0;
			}
			if(pLayoutSelections->pRoomLayoutSelections[i].pOrientations)
			{
				GFREE(pGame, pLayoutSelections->pRoomLayoutSelections[i].pOrientations);
				pLayoutSelections->pRoomLayoutSelections[i].pOrientations = NULL;
				pLayoutSelections->pRoomLayoutSelections[i].nCount = 0;
			}
		}
	}
	
	if(TEST_MASK(dwFlags, RLS_FREE_STRUCT))
	{
		GFREE(pGame, pLayoutSelections);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RoomLayoutSelectionsFree(
	GAME * pGame,
	ROOM_LAYOUT_SELECTIONS * pLayoutSelections)
{
	sRoomLayoutSelectionsFree(pGame, pLayoutSelections, RLS_FREE_ALL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomLayoutSelectionsRecreateSelectionData(
	GAME * pGame,
	ROOM * pRoom)
{
	// Free this stuff so that we don't get two copies of everything
	RoomLayoutSelectionsFreeSelectionData(pGame, pRoom);

	int * pnThemes = NULL;
	int nThemeIndexCount = LevelGetSelectedThemes(RoomGetLevel(pRoom), &pnThemes);
	RoomLayoutSelectLayoutItems(pGame, pRoom, pRoom->pLayout, &pRoom->pLayout->tGroup, pRoom->dwRoomSeed, pnThemes, nThemeIndexCount);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomLayoutSelectionsFreeSelectionData(
	GAME * pGame,
	ROOM * pRoom)
{
	sRoomLayoutSelectionsFree(pGame, pRoom->pLayoutSelections, RLS_FREE_PROP_MODELIDS | RLS_FREE_SELECTIONS);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static TRACKED_SPAWN_POINT *sAddTrackedSpawnPoint( 
	GAME *pGame,
	ROOM_LAYOUT_SELECTIONS *pLayoutSelections, 
	ROOM_LAYOUT_GROUP *pSpawnPoint)
{
	ASSERTX_RETNULL( pLayoutSelections, "Expected layout selections" );
	ASSERTX_RETNULL( pSpawnPoint, "Expected spawn point" );

	// bump up number of points
	pLayoutSelections->nNumTrackedSpawnPoints++;
	
	// realloc the array
	pLayoutSelections->pTrackedSpawnPoints = (TRACKED_SPAWN_POINT *)GREALLOC(
		pGame, 
		pLayoutSelections->pTrackedSpawnPoints, 
		pLayoutSelections->nNumTrackedSpawnPoints * sizeof( TRACKED_SPAWN_POINT ));

	// get the new element
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint = &pLayoutSelections->pTrackedSpawnPoints[ pLayoutSelections->nNumTrackedSpawnPoints - 1 ];
	sTrackedSpawnPointInit( pTrackedSpawnPoint, pSpawnPoint );
	return pTrackedSpawnPoint;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TRACKED_SPAWN_POINT *RoomGetOrCreateTrackedSpawnPoint(
	GAME *pGame,
	ROOM *pRoom,
	ROOM_LAYOUT_GROUP *pSpawnPoint)
{
	ASSERTX_RETNULL( pRoom, "Expected room" );
	ASSERTX_RETNULL( pSpawnPoint, "Expected spawn point" );
	
	// search
	ROOM_LAYOUT_SELECTIONS *pLayoutSelections = pRoom->pLayoutSelections;
	if (pLayoutSelections)
	{
		// search for existing entry (if any)
		for (int i = 0; i < pLayoutSelections->nNumTrackedSpawnPoints; ++i)
		{
			TRACKED_SPAWN_POINT *pTrackedSpawnPoint = &pLayoutSelections->pTrackedSpawnPoints[ i ];
			if (pTrackedSpawnPoint->pSpawnPoint == pSpawnPoint)
			{
				return pTrackedSpawnPoint;
			}
		}

		// not found, add another new one
		return sAddTrackedSpawnPoint( pGame, pLayoutSelections, pSpawnPoint );

	}

	return NULL;  // not found
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomTrackedSpawnPointAddUnit(
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint,
	UNIT *pUnit)
{
	ASSERTX_RETURN( pTrackedSpawnPoint, "Expected tracked spawn point" );
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// bump up unit count
	pTrackedSpawnPoint->nNumUnits++;
		
	// bump up storage array
	pTrackedSpawnPoint->pidUnits = (UNITID *)GREALLOC(
		pGame, 
		pTrackedSpawnPoint->pidUnits, 
		pTrackedSpawnPoint->nNumUnits * sizeof( UNITID ));
	
	// add unit to end
	pTrackedSpawnPoint->pidUnits[ pTrackedSpawnPoint->nNumUnits - 1 ] = UnitGetId( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomTrackedSpawnPointReset(
	GAME *pGame,
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint)
{
	ASSERTX_RETURN( pTrackedSpawnPoint, "Expected tracked spawn point" );
	
	// clear tracked contents
	sTrackedSpawnPointClearUnits( pGame, pTrackedSpawnPoint );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_LAYOUT_SELECTION *RoomGetLayoutSelection(
	ROOM *pRoom,
	ROOM_LAYOUT_ITEM_TYPE eType)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	ASSERTX_RETFALSE( pRoom->pLayoutSelections, "Expected layout selections" );
	ASSERTX_RETFALSE( eType > ROOM_LAYOUT_ITEM_INVALID && eType < ROOM_LAYOUT_ITEM_MAX, "Invalid room layout item type" );
	return &pRoom->pLayoutSelections->pRoomLayoutSelections[eType];	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomLayoutGetStaticLightDefinitionFromGroup(
	const ROOM * pRoom,
	const ROOM_LAYOUT_GROUP * pGroup,
	const ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation,
	STATIC_LIGHT_DEFINITION * pDefinition)
{
	ASSERT_RETURN(pRoom);
	ASSERT_RETURN(pGroup);
	ASSERT_RETURN(pSpawnOrientation);
	ASSERT_RETURN(pDefinition);

	pDefinition->dwFlags = 0;
	pDefinition->fIntensity = pGroup->fRotation;
	pDefinition->fPriority = pGroup->fBuffer;
	pDefinition->tColor = pGroup->vNormal;
	pDefinition->tFalloff = CFloatPair(pGroup->fFalloffNear, pGroup->fFalloffFar);
	//MatrixMultiply(&pDefinition->vPosition, &pSpawnOrientation->vPosition, &pRoom->tWorldMatrix);
	pDefinition->vPosition = pSpawnOrientation->vPosition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_LAYOUT_SELECTION_ORIENTATION * RoomLayoutGetOrientationFromGroup(
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP * pLayoutGroup)
{
	ASSERT_RETNULL( pRoom );
	ASSERT_RETNULL( pLayoutGroup );
	ROOM_LAYOUT_SELECTION * pLayoutSelection = RoomGetLayoutSelection(pRoom, pLayoutGroup->eType);
	ASSERT_RETNULL(pLayoutSelection);
	ASSERT_RETNULL(pLayoutSelection->pOrientations);
	ASSERT_RETNULL(pLayoutSelection->pGroups);
	for(int i=0; i<pLayoutSelection->nCount; i++)
	{
		ASSERT_CONTINUE(pLayoutSelection->pGroups[i]);
		if(pLayoutSelection->pGroups[i] == pLayoutGroup)
		{
			return &pLayoutSelection->pOrientations[i];
		}
	}
	return NULL;
}
