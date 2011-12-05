//----------------------------------------------------------------------------
// c_wardrobe.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "clients.h" // also includes prime.h
#include "game.h"
#include "e_settings.h"
#include "e_wardrobe.h"
#include "e_definition.h"
#include "e_texture.h"
#include "e_model.h"
#include "e_material.h"
#include "e_anim_model.h"
#include "s_message.h"
#include "inventory.h"
#include "items.h"
#include "excel_private.h"
#include "c_appearance_priv.h"
#include "c_granny_rigid.h"
#include "weapons.h"
#include "picker.h"
#include "skills.h"
#include "level.h"
#include "filepaths.h"
#include "performance.h"
#include "gameunits.h"
#include "config.h"
#include "unit_priv.h"
#include "wardrobe.h"
#include "wardrobe_priv.h"
#include "appcommon.h"
#include "e_common.h"
#include "teams.h"
#include "colorset.h"
#include "colors.h"
#include "states.h"
#include "windowsmessages.h"
#include "uix.h"
#include "unitmodes.h"
#include "e_budget.h"
#include "player.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "fillpak.h"
#include "unitfileversion.h"
#include "e_environment.h"
#include "e_debug.h"
#include "e_optionstate.h"
#include "e_demolevel.h"

// debug
//#ifdef HAMMER
#include "holodeck.h"
//#endif

using namespace FSSE;

//----------------------------------------------------------------------------
struct WARDROBE 
{
	int									nId;	// field for hashing
	WARDROBE*							pNext;	// field for hashing

	WARDROBE_SERVER *					pServer;
	WARDROBE_CLIENT *					pClient;
	DWORD								dwFlags;

	WARDROBE_WEAPON						tWeapons[MAX_WEAPONS_PER_UNIT];
	WARDROBE_BODY						tBody;
	REFCOUNT							tRefCount;
};
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define WARDROBE_TRACE(fmt, ...)						trace(fmt, __VA_ARGS__)
#else
#define WARDROBE_TRACE(fmt, ...)
#endif

#define WARDROBE_TIMERS 0

#define WARDROBE_EXTRA_APPEARANCE_COUNT_BITS			3
#define WARDROBE_EXTRA_APPEARANCE_BITS					6
#define WARDROBE_LAYER_COUNT_BITS						16
#define WARDROBE_LAYER_ID_BITS							16
#define WARDROBE_LAYER_CC_BITS							16
#define WARDROBE_APPEARANCE_CC_BITS						16
#define WARDROBE_NUM_BODY_COLOR_BITS					4  // im so generous ;)
#define WARDROBE_LAYER_PARAM_BITS						2
#define WARDROBE_WEAPON_CLASS_CC_BITS					16
#define WARDROBE_WEAPON_AFFIX_COUNT_BITS				4 
#define WARDROBE_WEAPON_AFFIX_CODE_BITS					32
#define WARDROBE_COLORSET_BITS							16
#define WARDROBE_SKIN_COLOR_BITS						8
#define WARDROBE_HAIR_COLOR_BITS						8
#define FILE_PATH_WARDROBE								"wardrobe\\"
#define MAX_COSTUME_COUNT								2 // keep two costumes for double buffering purposes.

static DWORD sgpdwColorMaskPreview[ 3 ] = 
{
	ARGB_MAKE_FROM_INT( 137,    108,   66, 255 ), // red
	ARGB_MAKE_FROM_INT(  61,     73,   56, 255 ), // green
	ARGB_MAKE_FROM_INT( 102,    109,  254, 255 ), // blue
};

// client-only list of wardrobes which still need work
static SIMPLE_DYNAMIC_ARRAY_EX<WARDROBE*> sgptWardrobesToUpdate;
static BOOL sgbWardrobesUpdateDirty = FALSE;	// set to TRUE when pending updates get added
static int sgnNextUpdateTicket = 0;
static WARDROBE* sgpCurrentWardrobe = NULL;
static int sgnNextWardrobeId = 0; // For the wardrobe hash

#define TEXTURE_SET_HASH_SIZE 256
static WARDROBE_TEXTURESET_DATA * sgpTextureSetHash[ TEXTURE_SET_HASH_SIZE ];

void c_WardrobeRandomize ( 
	GAME * pGame,
	WARDROBE * pWardrobe );
static PRESULT c_sWardrobeModelLoad ( 
	GAME * pGame, 
	int nAppearanceDefId,
	WARDROBE_MODEL* pWardrobeModel,
	int nLOD,
	BOOL bForceSync = FALSE );
BOOL sWardrobeFree(
	GAME* pGame,
	WARDROBE* pWardrobe );

#define WARDROBE_WEAPON_BITS			32

#define UPDATE_WARDROBE_SEQUENTIALLY
#ifdef UPDATE_WARDROBE_SEQUENTIALLY
#define NUM_WARDROBES_BETWEEN_CLEARS	1
#else
#define NUM_WARDROBES_BETWEEN_CLEARS	15
#endif
#define NUM_MS_PER_WARDROBE_UPDATE		10

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WARDROBE* sWardrobeGet( GAME* pGame, int nWardrobe )
{
	if ( ! pGame->pWardrobeHash )
		return NULL;

	return HashGet( (*pGame->pWardrobeHash), nWardrobe );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL WardrobeExists(
	GAME* pGame,
	int nWardrobe )
{
	ASSERT_RETFALSE( pGame );

	if ( ! pGame->pWardrobeHash )
		return FALSE;

	return ( sWardrobeGet( pGame, nWardrobe ) != NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const WARDROBE_APPEARANCE_GROUP_DATA *WardrobeAppearanceGroupGetData(
	int nAppearanceGroup)
{
	return (WARDROBE_APPEARANCE_GROUP_DATA *)ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, nAppearanceGroup);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static COSTUME & sGetCostumeToDraw( 
	WARDROBE * pWardrobe )
{
	return pWardrobe->pClient->pCostumes[ pWardrobe->pClient->nCostumeToDraw ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static COSTUME & sGetCostumeToUpdate( 
	WARDROBE * pWardrobe )
{
	return pWardrobe->pClient->pCostumes[ pWardrobe->pClient->nCostumeToUpdate ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline struct MEMORYPOOL* WARDROBE_MEMPOOL(
	GAME* game)
{
	if (!game || IS_CLIENT(game))
	{
		return NULL;
	}
	return GameGetMemPool(game);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sCostumeUpdateLayerArray( 
	GAME * pGame,
	COSTUME & tCostume )
{
	if ( tCostume.nLayersAllocated >= tCostume.nLayerCount )
		return;

	tCostume.nLayersAllocated = max( tCostume.nLayerCount, tCostume.nLayersAllocated * 2 );

	tCostume.ptLayers = (WARDROBE_LAYER_INSTANCE *)REALLOC(WARDROBE_MEMPOOL(pGame), tCostume.ptLayers, sizeof(WARDROBE_LAYER_INSTANCE) * tCostume.nLayersAllocated );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sLayerCompare ( const void * first, const void * second )
{
	const WARDROBE_LAYER_INSTANCE * pFirst  = (const WARDROBE_LAYER_INSTANCE *) first;
	const WARDROBE_LAYER_INSTANCE * pSecond = (const WARDROBE_LAYER_INSTANCE *) second;
	WARDROBE_LAYER_DATA * pFirstDef  = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, pFirst ->nId );
	WARDROBE_LAYER_DATA * pSecondDef = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, pSecond->nId );
	
	if ( ! pFirstDef || ! pSecondDef )
	{
		if ( pFirstDef )
			return 1;
		if ( pSecondDef )
			return -1;
		return 0;
	}

	if ( pFirstDef->nBoneGroup > pSecondDef->nBoneGroup )
		return 1;
	else if ( pFirstDef->nBoneGroup < pSecondDef->nBoneGroup )
		return -1;
	if ( pFirstDef->nOrder > pSecondDef->nOrder )
		return 1;
	else if ( pFirstDef->nOrder < pSecondDef->nOrder )
		return -1;
	if ( pFirst->nId > pSecond->nId )
		return 1;
	if ( pFirst->nId < pSecond->nId )
		return -1;
	if ( pFirst->wParam > pSecond->wParam )
		return -1;
	if ( pFirst->wParam < pSecond->wParam )
		return 1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCostumeSortLayers (
	COSTUME & tCostume )
{
	if ( tCostume.nLayerCount )
		qsort( tCostume.ptLayers, tCostume.nLayerCount, sizeof(WARDROBE_LAYER_INSTANCE), sLayerCompare );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static inline BOOL sWardrobeModelIsLoaded (
	WARDROBE_MODEL* pWardrobeModel, int nLOD )
{
	return ( pWardrobeModel->nLOD == nLOD )
		  && !pWardrobeModel->bAsyncLoadRequested 
		&& ( pWardrobeModel->pEngineData != NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sWardrobeModelLoadFailed (
	WARDROBE_MODEL* pWardrobeModel,
	int nLOD )
{
	return pWardrobeModel->bLoadFailed[ nLOD ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sWardrobeAdjustLOD( 
	GAME* pGame,
	WARDROBE* pWardrobe,
	int nLOD,
	WARDROBE_MODEL* pWardrobeModel,
	WARDROBE_MODEL* pWardrobeModelBase,
	BOOL bAssertIfMissing )
{
	if ( nLOD == LOD_LOW )
	{
		// We can only upgrade if we have the corresponding base LOD
		if ( sWardrobeModelLoadFailed( pWardrobeModelBase, LOD_HIGH ) )
		{
			if ( bAssertIfMissing )
			{
				// We're missing a critical file, which will leave
				// holes in the final model.

				char szLowFileName[DEFAULT_FILE_WITH_PATH_SIZE];
				e_ModelDefinitionGetLODFileName(szLowFileName, 
					ARRAYSIZE(szLowFileName), pWardrobeModel->pszFileName );
				ErrorDoAssert( "Missing critical file '%s'", szLowFileName );
			}
			
			return FALSE;
		}
		else
		{
			c_WardrobeUpgradeLOD( pWardrobe->nId, TRUE );
			return TRUE;
		}
	}
	else
	{
		// We can only downgrade if we have the corresponding base LOD
		if ( sWardrobeModelLoadFailed( pWardrobeModelBase, LOD_LOW ) )
		{
			if ( bAssertIfMissing )
			{
				// We're missing a critical file, which will leave
				// holes in the final model.

				ErrorDoAssert( "Missing critical file '%s'", 
					pWardrobeModel->pszFileName );
			}

			return FALSE;
		}
		else
		{
			c_WardrobeDowngradeLOD( pWardrobe->nId );
			return TRUE;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTextureSetHashFunction(
	int nTextureSetGroup,
	int pnAppearanceGroup[ NUM_APPEARANCES_PER_TEXTURE_SET_GROUP ] )
{
	int nIndex = 0;
	for ( int i = 0; i < NUM_APPEARANCES_PER_TEXTURE_SET_GROUP; i ++ )
	{
		if ( pnAppearanceGroup[ i ] != INVALID_ID )
			nIndex += ( pnAppearanceGroup[ i ] + 1 )  * 20;
	}

	nIndex += MAX( 0, nTextureSetGroup );

	return nIndex % TEXTURE_SET_HASH_SIZE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WARDROBE_TEXTURESET_DATA * sGetTextureSet(
	int nTextureSetGroup,
	int pnAppearanceGroup[ NUM_APPEARANCES_PER_TEXTURE_SET_GROUP ] )
{
	int nIndex = sTextureSetHashFunction( nTextureSetGroup, pnAppearanceGroup );

	ASSERT_RETNULL( nIndex >= 0 );
	ASSERT_RETNULL( nIndex < TEXTURE_SET_HASH_SIZE );

	WARDROBE_TEXTURESET_DATA * pCurr = sgpTextureSetHash[ nIndex ];
	ASSERT( NUM_APPEARANCES_PER_TEXTURE_SET_GROUP == 2 );
	while ( pCurr )
	{
		if ( pCurr->nTextureSetGroup == nTextureSetGroup && 
			pCurr->pnAppearanceGroup[ 0 ] == pnAppearanceGroup[ 0 ] && 
			pCurr->pnAppearanceGroup[ 1 ] == pnAppearanceGroup[ 1 ] )
			return pCurr;
		pCurr = pCurr->pNext;
	}
	return NULL;
}

//----------------------------------------------------------------------------
// this creates an index based on a hash function.  The ExcelGetDataByKey turns out to be too slow for this table
//----------------------------------------------------------------------------
BOOL ExcelWardrobeTextureSetPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	ZeroMemory(sgpTextureSetHash, sizeof(sgpTextureSetHash));

	for (int ii = (int)ExcelGetCountPrivate(table) - 1; ii >= 0; ii--)
	{
		WARDROBE_TEXTURESET_DATA * texture_set_data = (WARDROBE_TEXTURESET_DATA *)ExcelGetDataPrivate(table, ii);
		if ( ! texture_set_data )
			continue;

		unsigned int index = sTextureSetHashFunction(texture_set_data->nTextureSetGroup, texture_set_data->pnAppearanceGroup);
		ASSERT_CONTINUE(index < TEXTURE_SET_HASH_SIZE);
		texture_set_data->pNext = sgpTextureSetHash[index];
		sgpTextureSetHash[index] = texture_set_data;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelWardrobePostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Index == DATATABLE_WARDROBE_LAYER);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		WARDROBE_LAYER_DATA * wardrobe_layer_data = (WARDROBE_LAYER_DATA *)ExcelGetDataPrivate(table, ii);
		if ( ! wardrobe_layer_data )
			continue;

		wardrobe_layer_data->tAttach.vNormal = VECTOR(1, 0, 0);
		wardrobe_layer_data->tAttach.vPosition = VECTOR( 0 );
		wardrobe_layer_data->tAttach.vScale = VECTOR( 1, 1, 1 );
		wardrobe_layer_data->nRow = (int)ii;
		wardrobe_layer_data->tAttach.nAttachedDefId = INVALID_ID;
		wardrobe_layer_data->tAttach.nBoneId = INVALID_ID;

		for (unsigned int jj = 0; jj < NUM_APPEARANCE_GROUPS_PER_LAYER; ++jj)
		{
			if (wardrobe_layer_data->pnRandomAppearanceGroups[jj] == INVALID_ID)
			{
				continue;
			}
			WARDROBE_APPEARANCE_GROUP_DATA * wardrobe_appearance_group = (WARDROBE_APPEARANCE_GROUP_DATA *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), 
				DATATABLE_WARDROBE_APPEARANCE_GROUP, wardrobe_layer_data->pnRandomAppearanceGroups[jj]);

			if ( ! wardrobe_appearance_group )
				continue;

			ASSERT_CONTINUE(wardrobe_appearance_group->nRandomLayerCount < MAX_BODY_LAYERS_PER_APPEARANCE_GROUP);
			wardrobe_appearance_group->pnBodyLayers[wardrobe_appearance_group->nRandomLayerCount] = (int)ii;
			wardrobe_appearance_group->nRandomLayerCount++;
		}

		for ( int jj = 0; jj < NUM_LAYERSETS_PER_LAYER; jj++ )
		{
			if ( wardrobe_layer_data->pnLayerSet[ jj ] != INVALID_ID )
			{
				WARDROBE_LAYERSET_DATA * pLayerSet = (WARDROBE_LAYERSET_DATA *) ExcelGetDataNotThreadSafe( EXCEL_CONTEXT(), DATATABLE_WARDROBE_LAYERSET, wardrobe_layer_data->pnLayerSet[ jj ] );
				if ( pLayerSet && pLayerSet->nLayerCount < MAX_LAYERS_IN_LAYERSET )
				{
					pLayerSet->pnLayers[ pLayerSet->nLayerCount ] = ii;
					pLayerSet->nLayerCount++;
				}
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelWardrobeModelPostProcessRow(
	struct EXCEL_TABLE * table,
	unsigned int row,
	BYTE * rowdata)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(rowdata);

	WARDROBE_MODEL * wardrobe_model = (WARDROBE_MODEL *)rowdata;
	wardrobe_model->nRow = row;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelWardrobeModelFreeRow(
	struct EXCEL_TABLE * table,
	BYTE * rowdata)
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(rowdata);

#if !ISVERSION(SERVER_VERSION)
	WARDROBE_MODEL * wardrobe_model = (WARDROBE_MODEL *)rowdata;
	e_WardrobeModelResourceFree(wardrobe_model);
#endif //!SERVER_VERSION
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelWardrobeModelFree(
	struct EXCEL_TABLE * table)
{
	ASSERT_RETURN(table);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		BYTE * rowdata = (BYTE *)ExcelGetDataPrivate(table, ii);
		if ( ! rowdata )
			continue;
		ExcelWardrobeModelFreeRow(table, rowdata);
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static int sComparePrioritiesAscending( const void * e1, const void * e2 )
{
	const WARDROBE * pW1 = (*(const WARDROBE **)e1);
	const WARDROBE * pW2 = (*(const WARDROBE **)e2);

	// sort ascending
	if ( pW1->pClient->ePriority > pW2->pClient->ePriority )
		return 1;
	if ( pW1->pClient->ePriority < pW2->pClient->ePriority )
		return -1;
	else // equal priority
	{
		if ( pW1->pClient->nUpdateTicket > pW2->pClient->nUpdateTicket )
			return 1;
		else if ( pW1->pClient->nUpdateTicket < pW2->pClient->nUpdateTicket )
			return -1;
	}

	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT c_sWardrobeAddToUpdateList ( 
	GAME * pGame,
	WARDROBE * pWardrobe )
{
	ASSERT_RETINVALID( pWardrobe->pClient );

	// Remove the wardrobe if it is already on the list
	ArrayRemoveByValue( sgptWardrobesToUpdate, pWardrobe );

	// Reset the current wardrobe we are working on if this is b
	if ( sgpCurrentWardrobe == pWardrobe )
		sgpCurrentWardrobe = NULL;

	// there's new stuff added to the update list, so mark dirty
	sgbWardrobesUpdateDirty = TRUE;

	// Layer stack is dirty, so clear it for the new update.
	COSTUME& tCostume = sGetCostumeToUpdate( pWardrobe );
	for ( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
	{
		tCostume.dwFlags[ i ] &= ~COSTUME_FLAG_LAYER_STACK_CREATED;
		tCostume.tLayerStack[ i ].Clear();
	}
	tCostume.tModelLayerStack.Clear();

	ASSERT_RETFAIL( !pGame || IS_CLIENT( pGame ) );

	pWardrobe->pClient->nUpdateTicket = sgnNextUpdateTicket++;
	ArrayAddItem( sgptWardrobesToUpdate, pWardrobe );
	sgptWardrobesToUpdate.QSort( sComparePrioritiesAscending );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT c_sWardrobeRemoveFromUpdateList ( 
	GAME * pGame,
	WARDROBE * pWardrobe )
{
	ASSERT_RETINVALIDARG( !pGame || IS_CLIENT( pGame ) );
	ArrayRemoveByValue( sgptWardrobesToUpdate, pWardrobe );

	if ( sgpCurrentWardrobe == pWardrobe )
		sgpCurrentWardrobe = NULL;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_WardrobeIsUpdated ( 
	int nWardrobe )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETFALSE( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	if ( ! pWardrobe 
		|| sgptWardrobesToUpdate.GetIndex( pWardrobe ) != INVALID_ID )
		return FALSE;
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static WARDROBE_TEXTURESET_DATA * c_sWardrobeGetTextureSetForLayer( 
	GAME * pGame,
	WARDROBE * pWardrobe,
	WARDROBE_LAYER_DATA * pLayer )
{
	if ( ! pWardrobe || ! pLayer || ! pWardrobe->pClient )
		return NULL;

	if ( pLayer->nTextureSetGroup == INVALID_ID )
		return NULL;

	int pnAppearanceGroup[ NUM_APPEARANCES_PER_TEXTURE_SET_GROUP ];
	pnAppearanceGroup[ 0 ] = pWardrobe->pClient->nAppearanceGroup;
	pnAppearanceGroup[ 1 ] = INVALID_ID;

	WARDROBE_TEXTURESET_DATA * pTextureSet = sGetTextureSet( pLayer->nTextureSetGroup, pnAppearanceGroup );
	if ( pTextureSet )
		return pTextureSet;

	WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroupData = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_APPEARANCE_GROUP, pWardrobe->pClient->nAppearanceGroup );

	if ( !pAppearanceGroupData || pAppearanceGroupData->bNoBodyParts )
		return NULL;

	static const int pnAppearanceGroupPairs[][ NUM_APPEARANCES_PER_TEXTURE_SET_GROUP ] = 
	{
		{ WARDROBE_BODY_SECTION_BASE,			INVALID_ID					},
		{ WARDROBE_BODY_SECTION_HEAD,			INVALID_ID					},
		{ WARDROBE_BODY_SECTION_HEAD,			NUM_WARDROBE_BODY_SECTIONS	},
		{ WARDROBE_BODY_SECTION_HEAD,			WARDROBE_BODY_SECTION_SKIN	},
		{ WARDROBE_BODY_SECTION_HAIR,			INVALID_ID					},
		{ WARDROBE_BODY_SECTION_FACIAL_HAIR,	INVALID_ID					},
		{ WARDROBE_BODY_SECTION_SKIN,			INVALID_ID					},
		{ WARDROBE_BODY_SECTION_HAIR_COLOR,		INVALID_ID					},
	};
	static const int sgnPairCount = sizeof(pnAppearanceGroupPairs) / (NUM_APPEARANCES_PER_TEXTURE_SET_GROUP*sizeof(int));
	for ( int i = 0; i < sgnPairCount; i++ )
	{
		for ( int j = 0; j < NUM_APPEARANCES_PER_TEXTURE_SET_GROUP; j++ )
		{
			int nSection = pnAppearanceGroupPairs[ i ][ j ];
			if ( nSection == NUM_WARDROBE_BODY_SECTIONS )
				pnAppearanceGroup[ j ] = pWardrobe->pClient->nAppearanceGroup;
			else if ( nSection == INVALID_ID )
				pnAppearanceGroup[ j ] = INVALID_ID;
			else
				pnAppearanceGroup[ j ] = pWardrobe->tBody.pnAppearanceGroups[ nSection ];
		}

		if ( pnAppearanceGroup[ 0 ] == INVALID_ID &&
			 pnAppearanceGroup[ 1 ] == INVALID_ID )
			continue;

		WARDROBE_TEXTURESET_DATA * pTextureSet = sGetTextureSet( pLayer->nTextureSetGroup, pnAppearanceGroup );
		if ( pTextureSet )
			return pTextureSet;
	}
	return NULL;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static WARDROBE_MODEL * c_sWardrobeGetModelForLayer ( 
	GAME * pGame,
	WARDROBE * pWardrobe,
	WARDROBE_LAYER_DATA * pLayer )
{
	if ( ! pWardrobe || ! pLayer || ! pWardrobe->pClient )
		return NULL;

	WARDROBE_MODEL tModelKey;
	tModelKey.pnAppearanceGroup[ 0 ]	= pWardrobe->pClient->nAppearanceGroup;
	tModelKey.pnAppearanceGroup[ 1 ]	= INVALID_ID;
	tModelKey.nModelGroup				= pLayer->nModelGroup;
	WARDROBE_MODEL * pWardrobeModel = (WARDROBE_MODEL *) ExcelGetDataByKey( pGame, DATATABLE_WARDROBE_MODEL, &tModelKey, sizeof(tModelKey) );
	if ( pWardrobeModel )
		return pWardrobeModel;

	WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroupData = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_APPEARANCE_GROUP, pWardrobe->pClient->nAppearanceGroup );
	if ( ! pAppearanceGroupData )
		return NULL;

	if ( pAppearanceGroupData->bNoBodyParts )
		return NULL;


	static const int pnAppearanceGroupPairs[][ NUM_APPEARANCES_PER_MODEL_GROUP ] = 
	{
		{ WARDROBE_BODY_SECTION_BASE,			INVALID_ID							},
		{ WARDROBE_BODY_SECTION_HEAD,			INVALID_ID							},
		{ WARDROBE_BODY_SECTION_HEAD,			WARDROBE_BODY_SECTION_FACIAL_HAIR	},
		{ WARDROBE_BODY_SECTION_HEAD,			WARDROBE_BODY_SECTION_SKIN			},
		{ WARDROBE_BODY_SECTION_HAIR,			INVALID_ID							},
		{ WARDROBE_BODY_SECTION_FACIAL_HAIR,	INVALID_ID							},
		{ WARDROBE_BODY_SECTION_SKIN,			INVALID_ID							},
		{ WARDROBE_BODY_SECTION_HAIR_COLOR,		INVALID_ID							},
	};
	static const int sgnPairCount = sizeof(pnAppearanceGroupPairs) / (NUM_APPEARANCES_PER_TEXTURE_SET_GROUP*sizeof(int));

	for ( int i = 0; i < sgnPairCount; i++ )
	{
		for ( int j = 0; j < NUM_APPEARANCES_PER_MODEL_GROUP; j++ )
		{
			int nIndex = pnAppearanceGroupPairs[ i ][ j ];
			if ( nIndex == INVALID_ID || pWardrobe->tBody.pnAppearanceGroups[ nIndex ] == INVALID_ID )
				tModelKey.pnAppearanceGroup[ j ] = INVALID_ID;

			tModelKey.pnAppearanceGroup[ j ] = pWardrobe->tBody.pnAppearanceGroups[ nIndex ];
		}
		if ( tModelKey.pnAppearanceGroup[ 0 ] == INVALID_ID )
			continue;

		pWardrobeModel = (WARDROBE_MODEL *) ExcelGetDataByKey( pGame, DATATABLE_WARDROBE_MODEL, &tModelKey, sizeof(tModelKey) );
		if ( pWardrobeModel )
			return pWardrobeModel;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sWardrobeModelDefUsesTexture(
	WARDROBE* pWardrobe,
	LAYER_TEXTURE_TYPE eLayerTextureType )
{
#if !ISVERSION(SERVER_VERSION)
	BOOL bModelDefUsesTexture = TRUE;
	if ( eLayerTextureType == LAYER_TEXTURE_COLORMASK )
		bModelDefUsesTexture = FALSE;

	return bModelDefUsesTexture;
#else
	return FALSE;
#endif //SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BLENDOP_GROUP sWardrobeGetBlendOpGroup(
	WARDROBE * pWardrobe )
{
	ASSERT_RETVAL( pWardrobe, BLENDOP_GROUP_DEFAULT );
	ASSERT_RETVAL( pWardrobe->pClient, BLENDOP_GROUP_DEFAULT );
	ASSERT_RETVAL( pWardrobe->pClient->nAppearanceGroup != INVALID_ID, BLENDOP_GROUP_DEFAULT );
	WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroup = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, pWardrobe->pClient->nAppearanceGroup );
	if ( ! pAppearanceGroup )
		return BLENDOP_GROUP_DEFAULT;

	BLENDOP_GROUP eBlendOpGroup = pAppearanceGroup->eBlendOpGroup;
	ASSERT_RETVAL( eBlendOpGroup >= 0 && eBlendOpGroup < NUM_WARDROBE_BLENDOP_GROUPS, BLENDOP_GROUP_DEFAULT );

	return eBlendOpGroup;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sWardrobeGetLayer(
	GAME * pGame,
	WARDROBE * pWardrobe,
	COSTUME * pCostume,
	int nLayerStart,
	int nTextureIndex,
	BOOL bGetNext )
{
	unsigned int nNumLayers = WardrobeGetNumLayers(pGame);

	WARDROBE_LAYER_DATA * pLayer = (WARDROBE_LAYER_DATA *) ExcelGetData ( pGame, DATATABLE_WARDROBE_LAYER, nLayerStart );
	if ( ! pLayer )
		return INVALID_ID;

	int nRowCollection = pLayer->nRowCollection;

	BOOL bNeedBlendOp = nTextureIndex != INVALID_ID;
	BLENDOP_GROUP eBlendOpGroup = bNeedBlendOp ? sWardrobeGetBlendOpGroup( pWardrobe ) : BLENDOP_GROUP_DEFAULT;

	if ( bGetNext )
		nLayerStart++;

	for (unsigned int nLayerId = nLayerStart; nLayerId < nNumLayers; nLayerId++ )
	{
		pLayer = (WARDROBE_LAYER_DATA *) ExcelGetData ( pGame, DATATABLE_WARDROBE_LAYER, nLayerId );
		if ( ! pLayer )
			return INVALID_ID;

		if ( pLayer->nRowCollection != nRowCollection )
			return INVALID_ID;

		if ( nTextureIndex == INVALID_ID )
			return nLayerId;

		if ( pLayer->pnWardrobeBlendOp[ eBlendOpGroup ] == INVALID_ID )
			continue;

		if ( ! c_sWardrobeGetTextureSetForLayer ( pGame, pWardrobe, pLayer ) )
			continue;

		WARDROBE_BLENDOP_DATA * pBlendOp = (WARDROBE_BLENDOP_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_BLENDOP, pLayer->pnWardrobeBlendOp[ eBlendOpGroup ] );

		if ( pBlendOp && pBlendOp->nTargetTextureIndex == nTextureIndex )
			return nLayerId;
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTextureGetSize(
	WARDROBE_TEXTURESET_DATA * pTextureSet,
	LAYER_TEXTURE_TYPE eLayerTextureType,
	int nLOD,
	int & nWidth,
	int & nHeight,
	BOOL bIsSourceTexture )
{
	ASSERT_RETURN( pTextureSet );
	if ( eLayerTextureType == LAYER_TEXTURE_BLEND )
	{
		nWidth = nHeight = 0;
		return;
	}

	nWidth  = pTextureSet->pnTextureSizes[ eLayerTextureType ][ 0 ];
	nHeight = pTextureSet->pnTextureSizes[ eLayerTextureType ][ 1 ];

	// This assumes that LOD_LOW canvases should be exactly half the res of the LOD_HIGH canvases.
	if ( nLOD != LOD_HIGH )
	{
		nWidth >>= 1;
		nHeight >>= 1;
	}

	TEXTURE_TYPE eTexture = (TEXTURE_TYPE) e_WardrobeGetLayerTextureType( eLayerTextureType );
	if ( eTexture >= 0 && eTexture < NUM_TEXTURE_TYPES )
	{
		e_TextureBudgetAdjustWidthAndHeight( TEXTURE_GROUP_WARDROBE, eTexture, nWidth, nHeight, bIsSourceTexture );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sLayerGetTexture(
	GAME * pGame,
	WARDROBE * pWardrobe,
	COSTUME * pCostume,
	int nLayerId,
	LAYER_TEXTURE_TYPE eLayerTextureType,
	int nLOD )
{
	if ( eLayerTextureType == LAYER_TEXTURE_NONE )
		return INVALID_ID;

	WARDROBE_LAYER_DATA * pLayer = (WARDROBE_LAYER_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, nLayerId );
	if ( !pLayer )
		return INVALID_ID;
	ASSERT_RETINVALID( pLayer->nTextureSetGroup != INVALID_ID );

	ASSERT_RETINVALID( pWardrobe->pClient->nAppearanceGroup != INVALID_ID );
	WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroup = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_APPEARANCE_GROUP, pWardrobe->pClient->nAppearanceGroup );
	if (! pAppearanceGroup )
		return INVALID_ID;

	WARDROBE_TEXTURESET_DATA * pTextureSet = c_sWardrobeGetTextureSetForLayer ( pGame, pWardrobe, pLayer );
	ASSERT_RETINVALID( pTextureSet );

	if ( eLayerTextureType != LAYER_TEXTURE_BLEND &&
	   ( nLOD == LOD_HIGH && pTextureSet->bLowOnly[ eLayerTextureType ] )
		|| ( nLOD == LOD_LOW && ( eLayerTextureType == LAYER_TEXTURE_NORMAL || 
			 eLayerTextureType == LAYER_TEXTURE_SPECULAR ) ) )
		return INVALID_ID;

	WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroupForTextureSet = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, pTextureSet->nAppearanceGroupForFolder );
	if ( ! pAppearanceGroupForTextureSet )
		return INVALID_ID;

	char * pszTextureName = NULL;
	char * pszBaseFolder = NULL;
	char * pszTextureSetFolder = NULL;
	int * pnTextureId = NULL;
	if ( eLayerTextureType == LAYER_TEXTURE_BLEND )
	{
		pszBaseFolder = pAppearanceGroupForTextureSet->pszBlendFolder;

		BLENDOP_GROUP eBlendOpGroup = pAppearanceGroup->eBlendOpGroup;
		ASSERT_RETINVALID( eBlendOpGroup >= 0 && eBlendOpGroup < NUM_WARDROBE_BLENDOP_GROUPS );
		ASSERT_RETINVALID( pLayer->pnWardrobeBlendOp[ eBlendOpGroup ] != INVALID_ID );
		WARDROBE_BLENDOP_DATA * pBlendOp = (WARDROBE_BLENDOP_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_BLENDOP, pLayer->pnWardrobeBlendOp[ eBlendOpGroup ] );
		if ( ! pBlendOp )
			return INVALID_ID;
		pszTextureName	= pBlendOp->pszBlendTexture;
		pnTextureId		= &pBlendOp->nBlendTexture;
	}
	else 
	{
		pszBaseFolder = pAppearanceGroupForTextureSet->pszTextureFolder;

		pszTextureName = pTextureSet->pszTextureFilenames	[ eLayerTextureType ];
		pnTextureId    = &pTextureSet->pnTextureIds			[ eLayerTextureType ][ nLOD ];

		if ( pszTextureName[ 0 ] == 0 &&
			 pTextureSet->pnTextureSizes[ eLayerTextureType ][ 0 ] > 0 )
		{
			return TEXTURE_WITH_DEFAULT_COLOR;
		}

		pszTextureSetFolder = pTextureSet->pszFolder;
	}

	if ( pszTextureName[ 0 ] == 0 )
		return INVALID_ID;

	if ( *pnTextureId == INVALID_ID || *pnTextureId == BLEND_TEXTURE_DEFINITION_LOADING_ID )
	{
		// Specify an override, so that layer textures load in at the
		// native resolution specified in the file.
		TEXTURE_DEFINITION tDefOverride;
		structclear( tDefOverride );

		if ( eLayerTextureType == LAYER_TEXTURE_BLEND )
		{
			tDefOverride.nMipMapUsed = 0;
#if !ISVERSION(SERVER_VERSION)
			tDefOverride.nFormat = e_GetTextureFormat( 
										e_WardrobeGetLayerFormat( eLayerTextureType ) );
#endif
		}


		// specify the max resolution the settings support/request
		sTextureGetSize( pTextureSet, eLayerTextureType, nLOD, tDefOverride.nWidth, tDefOverride.nHeight, TRUE );


		int nMaxLen = min( WARDROBE_FILE_STRING_SIZE, min( DEFAULT_FILE_SIZE, MAX_PATH ) );
		{
			V( e_WardrobeTextureNewLayer( pnTextureId, pszBaseFolder, pszTextureSetFolder, 
				pszTextureName, nMaxLen, eLayerTextureType, nLOD, pCostume->nModelDefinitionId, 
				&tDefOverride, 0, TEXTURE_FLAG_USE_SCRATCHMEM ) );
		}

		// for now, only addref when the texture is filled in in the Excel data, then release from excel later
		if ( eLayerTextureType != LAYER_TEXTURE_BLEND )
			e_TextureAddRef( *pnTextureId );
	}
#if !ISVERSION(SERVER_VERSION)
	if ( eLayerTextureType != LAYER_TEXTURE_BLEND )
	{
		if ( e_TextureIsLoaded( *pnTextureId, TRUE ) )
		{	
			V( e_WardrobeValidateTexture( *pnTextureId, eLayerTextureType ) );
		}
		else if ( e_TextureLoadFailed( *pnTextureId ) && ! e_TextureLoadFailureWarned( *pnTextureId ) )
		{
			if ( nLOD == LOD_HIGH )
			{
				if ( pszTextureSetFolder )
					ErrorDoAssert( "Could not load wardrobe texture!\n\n%s%s/%s", pszBaseFolder, pszTextureSetFolder, pszTextureName );
				else
					ErrorDoAssert( "Could not load wardrobe texture!\n\n%s%s", pszBaseFolder, pszTextureName );
			}

			e_TextureSetLoadFailureWarned( *pnTextureId );
		}
	}
#endif
	return *pnTextureId;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define DEFAULT_LAYER_STACK_SIZE	( MAX_LAYERS_PER_WARDROBE / 10 )

static void sCostumeInit(
	GAME* pGame,
	WARDROBE& tWardrobe,
	COSTUME& tCostume,
	int nLOD,
	int nColorSetIndex )
{
#if ! ISVERSION(SERVER_VERSION)
	if ( ! tWardrobe.pClient )
		return;
	memset( tCostume.ppnTextures, INVALID_ID, sizeof(int) * MAX_WARDROBE_TEXTURE_GROUPS * NUM_TEXTURES_PER_SET );
	if ( !TEST_MASK( tWardrobe.dwFlags, WARDROBE_FLAG_SHARES_MODEL_DEF ) )
	{
		const char* pszName = DefinitionGetNameById( DEFINITION_GROUP_APPEARANCE, 
			tWardrobe.pClient->nAppearanceDefId );
		tCostume.nModelDefinitionId = e_WardrobeModelDefinitionNewFromMemory( pszName, nLOD );
		e_ModelDefinitionAddRef( tCostume.nModelDefinitionId );
	} 
	else 
	{
		tCostume.nModelDefinitionId = INVALID_ID;
	}

	for ( int nTextureGroup = 0; 
		nTextureGroup < MAX_WARDROBE_TEXTURE_GROUPS; 
		nTextureGroup++ )
	{
		ArrayInit( tCostume.tLayerStack[ nTextureGroup ], 
			WARDROBE_MEMPOOL(pGame), DEFAULT_LAYER_STACK_SIZE);
	}

	ArrayInit( tCostume.tModelLayerStack,
		WARDROBE_MEMPOOL(pGame), DEFAULT_LAYER_STACK_SIZE );

	tCostume.nLOD = nLOD;
	tCostume.nColorSetIndex = nColorSetIndex;
	tCostume.bInitialized = FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL c_sWardrobeInitModelDef(
	GAME* pGame,
	WARDROBE* pWardrobe )
{
#if ! ISVERSION(SERVER_VERSION)
	// Has the model definition already been initialized?
	if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_MODEL_DEF_INITIALIZED ) )
		return TRUE;

	int nAppearanceDefId = pWardrobe->pClient->nAppearanceDefId;
	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return FALSE;
	}

	if ( pWardrobe->dwFlags & WARDROBE_FLAG_SHARES_MODEL_DEF )
	{
		ASSERT_RETTRUE( pWardrobe->pClient->nCostumeCount == 1 );
		if ( pWardrobe->pClient->nModelDefOverride != INVALID_ID )
			pWardrobe->pClient->pCostumes[ 0 ].nModelDefinitionId = pWardrobe->pClient->nModelDefOverride;
		else
			pWardrobe->pClient->pCostumes[ 0 ].nModelDefinitionId = c_AppearanceDefinitionGetModelDefinition( nAppearanceDefId );
		
		// Model definition initialized.
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_MODEL_DEF_INITIALIZED );

		return TRUE;
	} 
	
	int nLayerBaseId = INVALID_ID;
	if ( pWardrobe->dwFlags & WARDROBE_FLAG_IGNORE_APPEARANCE_DEF_LAYERS )
		nLayerBaseId = pWardrobe->pClient->pCostumes[0].tLayerBase.nId;
	else
		nLayerBaseId = pAppearanceDef->nWardrobeBaseId;
	nLayerBaseId = sWardrobeGetLayer( pGame, pWardrobe, NULL, nLayerBaseId, INVALID_ID, FALSE );	

	ASSERT_RETTRUE ( nLayerBaseId != INVALID_ID );
	WARDROBE_LAYER_DATA * pBaseLayer = (WARDROBE_LAYER_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, nLayerBaseId );
	ASSERT_RETTRUE( pBaseLayer );
	WARDROBE_MODEL* pWardrobeModel = c_sWardrobeGetModelForLayer( pGame, pWardrobe, pBaseLayer );
	ASSERTX_RETTRUE( pWardrobeModel, pBaseLayer->pszName );

	int nLOD = pWardrobe->pClient->nLOD;
	if ( !sWardrobeModelIsLoaded( pWardrobeModel, nLOD ) )
	{
		if ( sWardrobeModelLoadFailed( pWardrobeModel, nLOD ) )
		{
			sWardrobeAdjustLOD( pGame, pWardrobe, nLOD, pWardrobeModel, 
				pWardrobeModel, TRUE );

			return TRUE;
		}
		else
		{
			V( c_sWardrobeModelLoad( pGame, nAppearanceDefId, pWardrobeModel, nLOD ) );
			
			if ( c_GetToolMode() && sWardrobeModelIsLoaded( pWardrobeModel, nLOD ) )
			{
				// We sync load the wardrobe model, so let's check to see if the
				// model was successfully loaded.
			}
			else
				return FALSE; // we haven't loaded enough to set up the model definition
		}
	}

	ASSERT_RETTRUE( pWardrobeModel );
	ASSERT_RETTRUE( sWardrobeModelIsLoaded( pWardrobeModel, nLOD ) );
	ASSERT_RETTRUE( pWardrobe->pClient->nAppearanceGroup != INVALID_ID );
	for ( int i = 0; i < pWardrobe->pClient->nCostumeCount; i++ )
	{
		COSTUME& tCostume = pWardrobe->pClient->pCostumes[ i ];

		if ( !tCostume.bInitialized )
		{
			e_ModelDefinitionSetBoundingBox		 ( tCostume.nModelDefinitionId, nLOD, &pWardrobeModel->tBoundingBox );
			e_ModelDefinitionSetRenderBoundingBox( tCostume.nModelDefinitionId, nLOD, &pWardrobeModel->tBoundingBox );
			V( e_AnimatedModelDefinitionNewFromWardrobe( tCostume.nModelDefinitionId, nLOD, pWardrobeModel ) );
			tCostume.bInitialized = TRUE;
		}
	}

	// Model definition initialized.
	SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_MODEL_DEF_INITIALIZED );


#endif //  ! ISVERSION(SERVER_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddRandomLayerSetsToBody(
	GAME * pGame, 						
	WARDROBE_BODY & tBody,
	const WARDROBE_BODY_DATA * pBodyData  )
{
	int nBodyIndex = 0;
	for ( ; nBodyIndex < MAX_WARDROBE_BODY_LAYERS; nBodyIndex ++ )
	{
		if ( tBody.pnLayers[ nBodyIndex ] == INVALID_ID )
			break;
	}

	for ( int nLayerSetIndex = 0; nLayerSetIndex < MAX_LAYERSETS_PER_BODY && nBodyIndex < MAX_WARDROBE_BODY_LAYERS; nLayerSetIndex++ )
	{
		if ( pBodyData->pnRandomLayerSets[ nLayerSetIndex ] == INVALID_ID )
			break;

		const WARDROBE_LAYERSET_DATA * pLayerSetData = (const WARDROBE_LAYERSET_DATA *) ExcelGetData( EXCEL_CONTEXT(), DATATABLE_WARDROBE_LAYERSET, pBodyData->pnRandomLayerSets[ nLayerSetIndex ] );
		if ( ! pLayerSetData || ! pLayerSetData->nLayerCount )
			continue;

		int nRoll = RandGetNum( pGame, pLayerSetData->nLayerCount );
		ASSERT_CONTINUE( nRoll >= 0 && nRoll < MAX_LAYERS_IN_LAYERSET );
		tBody.pnLayers[ nBodyIndex ] = pLayerSetData->pnLayers[ nRoll ];
		nBodyIndex++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRandomizeBody(
	GAME * pGame, 						
	WARDROBE_BODY & tBody,
	int nWardrobeAppearanceGroup,
	BOOL bForce )
{
	RAND tRand;
	RandInit( tRand );

	const WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroup 
		= (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, 
		nWardrobeAppearanceGroup );
	if (! pAppearanceGroup)
		return;

	for ( int i = 0; i < NUM_WARDROBE_BODY_COLORS; i++ )
	{
		tBody.pbBodyColors[ i ] = (BYTE) RandGetNum( tRand, NUM_WARDROBE_BODY_COLOR_CHOICES );
	}

	if ( bForce )
	{
		for ( int m = 0; m < MAX_WARDROBE_BODY_LAYERS; m++ )
		{
			tBody.pnLayers[ m ] = INVALID_ID;
		}
	}

	for ( int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; i++ )
	{
		if ( tBody.pnAppearanceGroups[ i ] != INVALID_ID && ! bForce )
			continue;

		if ( ! pGame )
			tBody.pnAppearanceGroups[ i ] = pAppearanceGroup->pnBodySectionToAppearanceGroup[ i ][ 0 ];
		else
		{
			PickerStart( pGame, picker );
			for ( int j = 0; j < MAX_APPEARANCE_GROUPS_PER_BODY_SECTION; j++ )
			{
				if ( pAppearanceGroup->pnBodySectionToAppearanceGroup[ i ][ j ] == INVALID_ID )
					continue;
				const WARDROBE_APPEARANCE_GROUP_DATA * pBodyAppearanceGroup 
					= (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, 
					pAppearanceGroup->pnBodySectionToAppearanceGroup[ i ][ j ] );
				if ( ! pBodyAppearanceGroup )
					continue;
				if ( pBodyAppearanceGroup->bDontRandomlyPick )
					continue;
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, pAppearanceGroup->pnBodySectionToAppearanceGroup[ i ][ j ], 1));
			}

			if ( PickerIsEmpty( pGame ) )
			{
				tBody.pnAppearanceGroups[ i ] = INVALID_ID;
				continue;
			}

			tBody.pnAppearanceGroups[ i ] = PickerChoose( pGame );
		}
		// TRAVIS: getting a lot of asserts here in the facial hair group? I don't use facial hair? Shouldn't this skip?
		if ( tBody.pnAppearanceGroups[ i ] == INVALID_ID )
			continue;

		const WARDROBE_APPEARANCE_GROUP_DATA * pBodyAppearanceGroup 
			= (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, 
			tBody.pnAppearanceGroups[ i ] );

		if ( pBodyAppearanceGroup && pBodyAppearanceGroup->nRandomLayerCount )
		{
			for ( int m = 0; m < MAX_WARDROBE_BODY_LAYERS; m++ )
			{
				if ( tBody.pnLayers[ m ] == INVALID_ID )
				{
					int nRandomLayer = RandGetNum( tRand, pBodyAppearanceGroup->nRandomLayerCount );
					tBody.pnLayers[ m ] = pBodyAppearanceGroup->pnBodyLayers[ nRandomLayer ];
					break;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CML 2008.04.04 - This is a lot like sRandomizeBody, except for the random part.
//   It's for item icon generation where we don't want actual randomness, but we
//   need actual body-ness.
void WardrobeInitInitializeBody(
	WARDROBE_BODY & tBody,
	int nWardrobeAppearanceGroup,
	BOOL bForce )
{
	const WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroup 
		= (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, 
		nWardrobeAppearanceGroup );
	if (! pAppearanceGroup)
		return;

	for ( int i = 0; i < NUM_WARDROBE_BODY_COLORS; i++ )
	{
		tBody.pbBodyColors[ i ] = 0;
	}

	if ( bForce )
	{
		for ( int m = 0; m < MAX_WARDROBE_BODY_LAYERS; m++ )
		{
			tBody.pnLayers[ m ] = INVALID_ID;
		}
	}

	for ( int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; i++ )
	{
		if ( tBody.pnAppearanceGroups[ i ] != INVALID_ID && ! bForce )
			continue;

		tBody.pnAppearanceGroups[ i ] = pAppearanceGroup->pnBodySectionToAppearanceGroup[ i ][ 0 ];

		// TRAVIS: getting a lot of asserts here in the facial hair group? I don't use facial hair? Shouldn't this skip?
		if ( tBody.pnAppearanceGroups[ i ] == INVALID_ID )
			continue;

		const WARDROBE_APPEARANCE_GROUP_DATA * pBodyAppearanceGroup 
			= (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, 
			tBody.pnAppearanceGroups[ i ] );

		if ( pBodyAppearanceGroup && pBodyAppearanceGroup->nRandomLayerCount )
		{
			for ( int m = 0; m < MAX_WARDROBE_BODY_LAYERS; m++ )
			{
				if ( tBody.pnLayers[ m ] == INVALID_ID )
				{
					int nRandomLayer = 0;
					tBody.pnLayers[ m ] = pBodyAppearanceGroup->pnBodyLayers[ nRandomLayer ];
					break;
				}
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void WardrobeRandomizeInitStruct(
	GAME * pGame,
	WARDROBE_INIT & tWardrobeInit,
	int nWardrobeAppearanceGroup,
	BOOL bForce )
{
	if ( nWardrobeAppearanceGroup != INVALID_ID )
		sRandomizeBody( pGame, tWardrobeInit.tBody, nWardrobeAppearanceGroup, bForce );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanXferAppearanceGroup(
	int nAppearanceGroup)
{
	if ( nAppearanceGroup != INVALID_ID &&
		 nAppearanceGroup > 0 ) // just being explicit here.  We don't want to send the none group
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWardrobeBodyInit(
	WARDROBE_BODY &tBody)
{		

	for (int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; ++i)
	{
		tBody.pnAppearanceGroups[ i ] = INVALID_LINK;
		tBody.pnLayers[ i ] = INVALID_LINK;
	}
	for (int i = 0; i < NUM_WARDROBE_BODY_COLORS; ++i)
	{
		tBody.pbBodyColors[ i ] = 0;
	}	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sValidateWardrobeBody(
	const WARDROBE_BODY &tBody, 
	unsigned int nClass)
{
	if(nClass == INVALID_ID) return TRUE;

	//check that both body and head are non-invalid
	if(tBody.pnAppearanceGroups[WARDROBE_BODY_SECTION_HEAD] == INVALID_ID)
		return FALSE;
	if(tBody.pnAppearanceGroups[WARDROBE_BODY_SECTION_BASE] == INVALID_ID)
		return FALSE;

	const UNIT_DATA * pUnitData = PlayerGetData( NULL, nClass );
	ASSERT_RETFALSE( pUnitData );
	int nAppearanceGroup = pUnitData->
		pnWardrobeAppearanceGroup[UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON];

	const WARDROBE_APPEARANCE_GROUP_DATA * pGroup = 
		(WARDROBE_APPEARANCE_GROUP_DATA *)ExcelGetData( 
			NULL,
			DATATABLE_WARDROBE_APPEARANCE_GROUP, 
			nAppearanceGroup );
	ASSERT_RETFALSE( pGroup );

	for(int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; i++)
	{
		BOOL bFound = FALSE;
		if ( tBody.pnAppearanceGroups[i] == 0 )
		{
			switch ( i )
			{// these may not have 0 in the list, but it is valid
			case WARDROBE_BODY_SECTION_HAIR:
			case WARDROBE_BODY_SECTION_FACIAL_HAIR:
				bFound = TRUE;
				break;
			}
		}
		if ( AppIsHellgate() )
		{// hellgate does not assign these groups
			switch ( i )
			{
			case WARDROBE_BODY_SECTION_SKIN:
			case WARDROBE_BODY_SECTION_HAIR_COLOR:
				bFound = TRUE;
				break;
			}
		}
		else
		{// this is OK in Mythos
			switch ( i )
			{
			case WARDROBE_BODY_SECTION_HAIR_COLOR:
				bFound = TRUE;
				break;
			}
		}
		for(int j = 0; j < MAX_APPEARANCE_GROUPS_PER_BODY_SECTION; j++)
		{
			if(pGroup->pnBodySectionToAppearanceGroup[i][j] == tBody.pnAppearanceGroups[i])
			{
				bFound = TRUE;
				break;
			}
		}
		if ( ! bFound )
			return FALSE;
	}

	for ( int i = 0; i < NUM_WARDROBE_BODY_COLORS; i++ )
	{
		if ( tBody.pbBodyColors[ i ] >= NUM_WARDROBE_BODY_COLOR_CHOICES )
			return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sBodyXfer(
	WARDROBE_BODY &tBody,
	XFER<mode> &tXfer,
	BOOL bIncludeLayers,
	int nClass,
	int nUnitSaveVersion)
{

	if ( bIncludeLayers )
	{
			
		// number of layers
		unsigned int nNumLayers = 0;
		if (tXfer.IsSave())
		{
			for ( int i = 0; i < MAX_WARDROBE_BODY_LAYERS; ++i )
			{
				if ( tBody.pnLayers[ i ] != INVALID_ID )
				{
					nNumLayers++;
				}
			}
		}
		XFER_UINT( tXfer, nNumLayers, WARDROBE_DEFAULT_LAYERS_COUNT_BITS );

		// layer codes
		int nLayerIndexCurr = 0;
		for (unsigned int i = 0; i < nNumLayers; ++i)
		{
			DWORD dwLayerCode = INVALID_CODE;
			if (tXfer.IsSave())
			{
				for ( ; nLayerIndexCurr < MAX_WARDROBE_BODY_LAYERS; nLayerIndexCurr++ )
				{
					if ( tBody.pnLayers[ nLayerIndexCurr ] != INVALID_ID )
						break;
				}
				ASSERT_BREAK( nLayerIndexCurr < MAX_WARDROBE_BODY_LAYERS );
				int nLayerRow = tBody.pnLayers[ nLayerIndexCurr ];
				const WARDROBE_LAYER_DATA *pLayerData = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, nLayerRow );				
				ASSERTX( pLayerData, "Invalid layer" );
				dwLayerCode = pLayerData ? pLayerData->wCode : 0;
				nLayerIndexCurr++;
			}
			XFER_DWORD( tXfer, dwLayerCode, WARDROBE_LAYER_CC_BITS );
			if (tXfer.IsLoad())
			{
				if (i < MAX_WARDROBE_BODY_LAYERS)
				{
					tBody.pnLayers[ i ] = ExcelGetLineByCode( NULL, DATATABLE_WARDROBE_LAYER, dwLayerCode );
				}
				else
				{
					tBody.pnLayers[ i ] = INVALID_LINK;
				}
			}
			
		}
		
	}

	// clear out appearance groups on body when loading in favor of the new data from the stream
	if (tXfer.IsLoad())
	{
		for (int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; i++)
		{
			tBody.pnAppearanceGroups[ i ] = 0;
		}
	}
			
	// appearance counts	
	unsigned int nAppearanceCount = 0;
	if (tXfer.IsSave())
	{
		for ( int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; i++ )
		{
			if (sCanXferAppearanceGroup( tBody.pnAppearanceGroups[ i ] ))
			{
				nAppearanceCount++;
			}
		}
	}
	XFER_UINT( tXfer, nAppearanceCount, WARDROBE_EXTRA_APPEARANCE_COUNT_BITS );
	
	// each appearance groups
	if (tXfer.IsSave())
	{
		for (int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; ++i)
		{
			int nAppearanceGroup = tBody.pnAppearanceGroups[ i ];
			if (sCanXferAppearanceGroup( nAppearanceGroup ))
			{
				const WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceData = WardrobeAppearanceGroupGetData( nAppearanceGroup );
				ASSERT( pAppearanceData );
				DWORD dwCode = pAppearanceData ? pAppearanceData->wCode : 0;
				XFER_DWORD( tXfer, dwCode, WARDROBE_APPEARANCE_CC_BITS );
			}		
										
		}
	}
	else
	{
		for (unsigned int i = 0; i < nAppearanceCount; ++i)
		{
		
			DWORD dwCode = INVALID_CODE;
			XFER_DWORD( tXfer, dwCode, WARDROBE_APPEARANCE_CC_BITS );

			int nRow = ExcelGetLineByCode( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, dwCode );
			if ( nRow != INVALID_ID )
			{
				const WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceData = WardrobeAppearanceGroupGetData( nRow );
				ASSERT( pAppearanceData );
				if ( pAppearanceData && pAppearanceData->eBodySection >= 0 )
				{
					tBody.pnAppearanceGroups[ pAppearanceData->eBodySection ] = nRow;
				}
				
			}
		
		}
		
	}
		
	// colors
	unsigned int nNumBodyColors = NUM_WARDROBE_BODY_COLORS;
	XFER_UINT( tXfer, nNumBodyColors, WARDROBE_NUM_BODY_COLOR_BITS );
	for (unsigned int i = 0; i < nNumBodyColors; ++i)
	{
		unsigned int nBodyColor = 0;
		if (tXfer.IsSave())
		{
			nBodyColor = tBody.pbBodyColors[ i ];
		}
		XFER_UINT( tXfer, nBodyColor, WARDROBE_SKIN_COLOR_BITS );
		if (tXfer.IsLoad() && i < NUM_WARDROBE_BODY_COLORS)
		{
			tBody.pbBodyColors[ i ] = (BYTE)nBodyColor;
		}
		
	}

	// validate on load
	if (tXfer.IsLoad())
	{
		if (sValidateWardrobeBody( tBody, nClass ) == FALSE)
		{
			return FALSE;
		}
	}
	
	return TRUE;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int WardrobeInitStructWrite(
	WARDROBE_INIT & tWardrobeInit,
	BYTE * pbBuffer,
	int nMaxSize)
{
	ZeroMemory( pbBuffer, nMaxSize );
	BIT_BUF buf( pbBuffer, nMaxSize);
	XFER<XFER_SAVE> tXfer(&buf);
	sBodyXfer( tWardrobeInit.tBody, tXfer, TRUE, INVALID_ID, USV_CURRENT_VERSION );
	return buf.GetLen();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL WardrobeInitStructRead(
	WARDROBE_INIT & tWardrobeInit,
	BYTE * pbBuffer,
	int nMaxSize,
	unsigned int nClass)
{
	//returns FALSE if init struct is invalid.
	BIT_BUF buf( pbBuffer, nMaxSize);
	XFER<XFER_LOAD> tXfer(&buf);
	return sBodyXfer( tWardrobeInit.tBody, tXfer, TRUE, nClass, USV_CURRENT_VERSION );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WardrobeInitCreateFromUnit(
	UNIT *pUnit,
	WARDROBE_INIT &tWardrobeInit)
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	ASSERTX_RETURN( pUnit, "Expected unit" );
	WARDROBE* pWardrobe = sWardrobeGet( pGame, UnitGetWardrobe( pUnit ) );
	if (pWardrobe != NULL)
	{	
		tWardrobeInit.nColorSetIndex = pWardrobe->pServer->tCostume.nColorSetIndex;
		tWardrobeInit.tBody = pWardrobe->tBody;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWardrobeWeaponInit( 
	WARDROBE_WEAPON *pWardrobeWeapon)
{
	ASSERTX_RETURN( pWardrobeWeapon, "Expected wardrobe weapon" );

	pWardrobeWeapon->idWeaponReal = INVALID_ID;
	pWardrobeWeapon->idWeaponPlaceholder = INVALID_ID;
	pWardrobeWeapon->nClass = INVALID_LINK;
	for (int i = 0; i < AffixGetMaxAffixCount(); ++i)
	{
		pWardrobeWeapon->nAffixes[ i ] = INVALID_LINK;
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoWeaponUpdate(
	UNIT *pUnit,
	WARDROBE_WEAPON *pWardrobeWeapon,
	UNITID idWeaponRealOld,
	int nWeaponIndex )
{		
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	BOOL bWeaponsChanged = FALSE;
	
	// we might have a client-only unit for this weapon - so we keep a list of the old ones
	if (    idWeaponRealOld != pWardrobeWeapon->idWeaponReal
		|| (pWardrobeWeapon->idWeaponPlaceholder != INVALID_ID && pWardrobeWeapon->idWeaponReal != INVALID_ID) // we don't want both a placeholder and a real one
		|| (pWardrobeWeapon->nClass != INVALID_ID && idWeaponRealOld == INVALID_ID) )
	{
		UNIT * pWeaponRealNew = UnitGetById( pGame, pWardrobeWeapon->idWeaponReal );
		UNIT * pWeaponPlaceholder = UnitGetById( pGame, pWardrobeWeapon->idWeaponPlaceholder );

		BOOL bFreePlaceholder = FALSE;
		BOOL bCreateNewPlaceholder = FALSE;

		if ( pWardrobeWeapon->idWeaponReal != INVALID_ID )
		{
			if ( ! pWeaponRealNew )
			{
				if ( !pWeaponPlaceholder )
				{
					bCreateNewPlaceholder = TRUE;
				}
				else if ( UnitGetClass( pWeaponPlaceholder ) != pWardrobeWeapon->nClass )
				{
					bCreateNewPlaceholder = TRUE;
					bFreePlaceholder = TRUE;
				}
			} 
			else if ( pWardrobeWeapon->idWeaponPlaceholder != INVALID_ID ) 
			{
				bFreePlaceholder = TRUE;
			}
		} 
		else 
		{
			if ( pWardrobeWeapon->idWeaponPlaceholder != INVALID_ID )
				bFreePlaceholder = TRUE;

			if ( pWardrobeWeapon->nClass != INVALID_ID )
				bCreateNewPlaceholder = TRUE;
		}

		if ( bFreePlaceholder )
		{
			ASSERT( pWeaponPlaceholder );
			if ( pWeaponPlaceholder )
			{
				UnitFree( pWeaponPlaceholder );
				pWeaponPlaceholder = NULL;
			}
			pWardrobeWeapon->idWeaponPlaceholder = INVALID_ID;
		}

		if ( bCreateNewPlaceholder )
		{
			ASSERT_RETFALSE( pWardrobeWeapon->nClass != INVALID_ID );

			ITEM_SPEC tItemSpec;
			tItemSpec.nItemClass = pWardrobeWeapon->nClass;
			tItemSpec.vDirection = cgvYAxis;
			// TRAVIS : if the monster wants its weapons scaled up, do it here.
			tItemSpec.flScale = UnitGetData( pUnit )->fWeaponScale;
			if( AppIsHellgate() )
			{
				LEVEL * pLevel = UnitGetLevel(pUnit);
				if(GameGetControlUnit(pGame) == pUnit)
				{
					if(pLevel && LevelIsTown(pLevel))
					{
						tItemSpec.eLoadType = UDLT_TOWN_YOU;
					}
					else
					{
						tItemSpec.eLoadType = UDLT_WARDROBE_YOU;
					}
				}
				else
				{
					if(pLevel && LevelIsTown(pLevel))
					{
						tItemSpec.eLoadType = UDLT_TOWN_OTHER;
					}
					else
					{
						tItemSpec.eLoadType = UDLT_WARDROBE_OTHER;
					}
				}
			}

			pWeaponPlaceholder = ItemInit( pGame, tItemSpec );

			// apply any affixes to the item that should be
			for (int i = 0; i < AffixGetMaxAffixCount(); ++i)
			{
				int nAffix = pWardrobeWeapon->nAffixes[ i ];
				if (nAffix != INVALID_LINK)
				{
					c_AffixApply( pWeaponPlaceholder, nAffix );
				}
			}
			
			pWardrobeWeapon->idWeaponPlaceholder = UnitGetId( pWeaponPlaceholder );

			//if ( pWeaponPlaceholder )
			//{
			//	UnitSetStat( pWeaponPlaceholder, STATS_IDENTIFIED, 1 );  // this is a client-only unit.  The server usually sets this stat.
			//	UnitGfxInventoryInit( pWeaponPlaceholder );
			//	int nInventoryLocation = nWeaponIndex ? INVLOC_LHAND : INVLOC_RHAND;
			//	UnitInventoryAdd( INV_CMD_PICKUP_NOREQ, pUnit, pWeaponPlaceholder, nInventoryLocation );
			//}
		}

		UNIT * pWeaponUsed = UnitGetById( pGame, pWardrobeWeapon->idWeaponReal );
		if ( ! pWeaponUsed )
			pWeaponUsed = UnitGetById( pGame, pWardrobeWeapon->idWeaponPlaceholder );

		if (   idWeaponRealOld != pWardrobeWeapon->idWeaponReal 
			|| bCreateNewPlaceholder 
			|| bFreePlaceholder )
		{
			SkillNewWeaponIsEquipped( pGame, pUnit, pWeaponUsed );
			bWeaponsChanged = TRUE;
		}

		if ( bWeaponsChanged && pWeaponUsed )
			c_UnitSetMode( pWeaponUsed, MODE_ITEM_EQUIPPED_IDLE );
	}
	
	return bWeaponsChanged;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sWardrobeSingleWeaponXfer(
	UNIT *pUnit,
	WARDROBE_WEAPON *pWardrobeWeapon,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	BOOL bIsUpdate,
	BOOL *pbWeaponsChanged,
	int nWeaponIndex )
{
	ASSERTX_RETFALSE( pWardrobeWeapon, "Expected wardrobe weapon" );

	// init memory when loading brand new stuff
	if (tXfer.IsLoad() && bIsUpdate == FALSE)
	{
		sWardrobeWeaponInit( pWardrobeWeapon );
	}

	// old weapon tracking
	UNITID idWeaponRealOld = pWardrobeWeapon->idWeaponReal;
	
	// weapon unit id	
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_WARDROBE_WEAPON_IDS ))
	{
		XFER_UNITID( tXfer, pWardrobeWeapon->idWeaponReal );
	}

	// weapon unit code
	DWORD dwItemCode = 0;
	if (tXfer.IsSave())
	{
		if (pWardrobeWeapon->nClass != INVALID_LINK)
		{
			const UNIT_DATA *pUnitData = ItemGetData( pWardrobeWeapon->nClass );
			ASSERT( pUnitData );
			dwItemCode = pUnitData ? pUnitData->wCode : 0;
		}
	}
	XFER_DWORD( tXfer, dwItemCode, WARDROBE_WEAPON_CLASS_CC_BITS );
	if (tXfer.IsLoad())
	{
		pWardrobeWeapon->nClass = dwItemCode ? ExcelGetLineByCode( NULL, DATATABLE_ITEMS, dwItemCode ) : INVALID_LINK;
	}
		
	// applied affixes if requested so that we can see any visual effects they have
	if (TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_WARDROBE_WEAPON_AFFIXES ))
	{

		// affix count
		int nCursorPosAffixCount = tXfer.GetCursor();
		
		unsigned int nAffixCount = AffixGetMaxAffixCount();
		int bitsToRead = WARDROBE_WEAPON_AFFIX_COUNT_BITS;
		if ( tHeader.nVersion <= USV_WARDROBE_AFFIX_COUNT_CHANGED_FROM_SIX )
		{
			bitsToRead = 3;
			nAffixCount = 6;

		}

		XFER_UINT( tXfer, nAffixCount, bitsToRead );
									
		// xfer each applied affix
		unsigned int nNumAffixWritten = 0;
		for (unsigned int i = 0; i < nAffixCount; ++i)
		{
		
			// when saving, stop on invalid affix
			if (tXfer.IsSave())
			{
				if (pWardrobeWeapon->nAffixes[ i ] == INVALID_LINK)
				{
					break;
				}
			}
			
			// affix code
			DWORD dwAffixCode = INVALID_CODE;
			if (tXfer.IsSave())
			{
				int nAffix = pWardrobeWeapon->nAffixes[ i ];
				dwAffixCode = AffixGetCode( nAffix );
				nNumAffixWritten++; // writing another affix
			}
			XFER_DWORD( tXfer, dwAffixCode, WARDROBE_WEAPON_AFFIX_CODE_BITS );
			
			// link back to affix when loading
			if (tXfer.IsLoad())
			{
				pWardrobeWeapon->nAffixes[ i ] = AffixGetByCode( dwAffixCode );
			}
			
			
		}
		
		// go back and write affix count
		if (tXfer.IsSave())
		{
			int nCursorPosCurrent = tXfer.GetCursor();
			tXfer.SetCursor( nCursorPosAffixCount );
			XFER_UINT( tXfer, nNumAffixWritten, WARDROBE_WEAPON_AFFIX_COUNT_BITS );
			tXfer.SetCursor( nCursorPosCurrent );
		}		
				
	}

	// handle updates when loading
	if (tXfer.IsLoad() && IS_CLIENT( pUnit ) )
	{
		ASSERTX_RETFALSE( pUnit, "Expected unit for update" );
		if (sDoWeaponUpdate( pUnit, pWardrobeWeapon, idWeaponRealOld, nWeaponIndex ) == TRUE)
		{
			if (pbWeaponsChanged)
			{
				*pbWeaponsChanged = TRUE;
			}
		}
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static COSTUME & sGetCostumeForXfer(
	GAME *pGame,
	WARDROBE &tWardrobe,
	BOOL bIsUpdate)
{
	if (bIsUpdate && IS_CLIENT( pGame ))
	{
		return sGetCostumeToUpdate( &tWardrobe );	
	}
	else
	{
		return tWardrobe.pServer ? tWardrobe.pServer->tCostume : tWardrobe.pClient->pCostumes[ 0 ];
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sWardrobeWeaponsXfer(
	UNIT *pUnit,
	WARDROBE *pWardrobe,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader,
	BOOL bIsUpdate,
	BOOL *pbWeaponsChanged)
{
		
	// xfer num wardrobe weapons
	int nCursorPosNumWeapons = tXfer.GetCursor();
	unsigned int nNumWeapons = MAX_WEAPONS_PER_UNIT;
	XFER_UINT( tXfer, nNumWeapons, STREAM_BITS_NUM_WARDROBE_WEAPONS );
	
	// go through each weapon
	WARDROBE_WEAPON tWeaponJunk;
	unsigned int nNumWeaponsXfered = 0;
	for (unsigned  int i = 0; i < nNumWeapons; ++i)
	{
		WARDROBE_WEAPON *pWardrobeWeapon = NULL;
		if (i < MAX_WEAPONS_PER_UNIT)
		{
			pWardrobeWeapon = &pWardrobe->tWeapons[ i ];
		}
		else
		{
			pWardrobeWeapon = &tWeaponJunk;  // file has more weapons that we now allow, ie. decide what to do with it when this actually occurs
		}

		// xfer weapon
		if (sWardrobeSingleWeaponXfer( 
				pUnit, 
				pWardrobeWeapon, 
				tXfer, 
				tHeader, 
				bIsUpdate, 
				pbWeaponsChanged,
				i ) == FALSE)
		{
			return FALSE;
		}
		nNumWeaponsXfered++;
		
	}
	
	// go back and write the number of weapons
	if (tXfer.IsSave())
	{
		int nCursorPosCurrent = tXfer.GetCursor();
		tXfer.SetCursor( nCursorPosNumWeapons );
		XFER_UINT( tXfer, nNumWeaponsXfered, STREAM_BITS_NUM_WARDROBE_WEAPONS );
		tXfer.SetCursor( nCursorPosCurrent );
	}

	return TRUE;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sWardrobeColorsetXfer(
	WARDROBE *pWardrobe,
	COSTUME &tCostume,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader)
{

	// color set
	DWORD dwColorSetCode = 0;
	if (tXfer.IsSave())
	{
		if (tCostume.nColorSetIndex != INVALID_LINK)
		{
			COLORSET_DATA *pColorSetData = (COLORSET_DATA *)ExcelGetData( NULL, DATATABLE_COLORSETS, tCostume.nColorSetIndex );
			ASSERT( pColorSetData );
			dwColorSetCode = pColorSetData ? pColorSetData->wCode : 0;
		}
	}
	XFER_DWORD( tXfer, dwColorSetCode, WARDROBE_COLORSET_BITS );
	if (tXfer.IsLoad())
	{
		tCostume.nColorSetIndex = dwColorSetCode ? ExcelGetLineByCode( NULL, DATATABLE_COLORSETS, dwColorSetCode ) : INVALID_LINK;
	}
					
	// color set item guid
	if ( TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_COLORSET_ITEM_GUID ) )
	{
		PGUID guidColorSetItem = INVALID_GUID;
		if ( pWardrobe->pServer	)
		{
			guidColorSetItem = pWardrobe->pServer->guidColorsetItem;
		}
		XFER_GUID( tXfer, guidColorSetItem );
		if (tXfer.IsLoad() && 
			guidColorSetItem != INVALID_GUID &&
			pWardrobe->pServer != NULL)
		{
			pWardrobe->pServer->guidColorsetItem = guidColorSetItem;
		}
	}

	return TRUE;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
static BOOL sLayersXfer(
	GAME *pGame,
	COSTUME &tCostume,
	XFER<mode> &tXfer,
	UNIT_FILE_HEADER &tHeader)
{

	// wardrobe layers
	if ( TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_WARDROBE_LAYERS ) )
	{
	
		// layer count
		XFER_UINT( tXfer, tCostume.nLayerCount, WARDROBE_LAYER_COUNT_BITS );
		unsigned int nNumLayersInStream = tCostume.nLayerCount;

		// alloate storage if needed
		if (tXfer.IsLoad())
		{
			sCostumeUpdateLayerArray( pGame, tCostume );  // allocate more storage if needed
		}
		
		// xfer each layer
		int nMaxLayerIndex = (int)WardrobeGetNumLayers(NULL);
		for( unsigned int nLayer = 0; nLayer < nNumLayersInStream; nLayer++ )
		{
			DWORD dwCode = INVALID_CODE;
			if (tXfer.IsSave())
			{
				int nLayerId = tCostume.ptLayers[ nLayer ].nId;
				ASSERTX_RETFALSE( nLayerId < nMaxLayerIndex, "Invalid costume layer id" );
				WARDROBE_LAYER_DATA *pLayer = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, nLayerId );
				ASSERT( pLayer );
				dwCode = pLayer ? pLayer->wCode : 0;
			}
			XFER_DWORD( tXfer, dwCode, WARDROBE_LAYER_CC_BITS );
			if (tXfer.IsLoad())
			{
				tCostume.ptLayers[ nLayer ].nId = (short)ExcelGetLineByCode(NULL, DATATABLE_WARDROBE_LAYER, dwCode);					
			}
			ASSERT_RETFALSE( tCostume.ptLayers[ nLayer ].nId >= 0 && tCostume.ptLayers[ nLayer ].nId < nMaxLayerIndex );				

			// get layer
			int nLayerId = tCostume.ptLayers[ nLayer ].nId;
			ASSERTX_RETFALSE( nLayerId >= 0 && nLayerId < nMaxLayerIndex, "Invalid layer index" );
			WARDROBE_LAYER_DATA *pLayer = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, nLayerId );
												
			// has bone index
			DWORD dwBoneIndexParam = 0;
			BOOL bHasBoneIndex = pLayer ? pLayer->bHasBoneIndex : FALSE;
			XFER_BOOL( tXfer, bHasBoneIndex );
			if (bHasBoneIndex)
			{
				if (tXfer.IsSave())
				{
					dwBoneIndexParam = tCostume.ptLayers[ nLayer ].wParam;
				}
				XFER_DWORD( tXfer, dwBoneIndexParam, WARDROBE_LAYER_PARAM_BITS );
				if (tXfer.IsLoad())
				{
					if (!pLayer || pLayer->bHasBoneIndex == FALSE)
					{
						dwBoneIndexParam = 0;  // no longer has bone index, any data read is throw away
					}
				}
				
			}
			tCostume.ptLayers[ nLayer ].wParam = (WORD)dwBoneIndexParam;
			
		}
		
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
BOOL WardrobeXfer(
	UNIT *pUnit,
	int nWardrobe,
	XFER<mode> & tXfer,
	UNIT_FILE_HEADER &tHeader,
	BOOL bIsWardrobeUpdate,
	BOOL *pbWeaponsChanged)
{
	GAME *pGame = UnitGetGame( pUnit );
	
	// get wardrobe
	BOOL bHasWardrobe = nWardrobe != INVALID_ID;
	XFER_BOOL( tXfer, bHasWardrobe );
	if (bHasWardrobe == FALSE)
	{
		return TRUE;
	}	

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	// must have wardrobe at this point to xfer wardrobe data
	ASSERTX_RETFALSE( pWardrobe, "Expected wardrobe" );
	
	// get costume
	COSTUME &tCostume = sGetCostumeForXfer( pGame, *pWardrobe, bIsWardrobeUpdate );

	// xfer weapon
	if (sWardrobeWeaponsXfer( pUnit, pWardrobe, tXfer, tHeader, bIsWardrobeUpdate, pbWeaponsChanged ) == FALSE)
	{
		return FALSE;
	}
	
	// color set
	if (sWardrobeColorsetXfer( pWardrobe, tCostume, tXfer, tHeader ) == FALSE)
	{
		return FALSE;
	}
	
	// body
	BOOL bIncludeLayers = TESTBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_WARDROBE_LAYERS_IN_BODY );
	if (sBodyXfer( pWardrobe->tBody, tXfer, bIncludeLayers, INVALID_ID, tHeader.nVersion ) == FALSE)
	{
		return FALSE;
	}
	
	// layers
	if (sLayersXfer( pGame, tCostume, tXfer, tHeader ) == FALSE)
	{
		return FALSE;
	}
				
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWardrobeWeaponInitFromUnit( 
	WARDROBE_WEAPON *pWardrobeWeapon,
	UNIT *pUnit)
{
	ASSERTX_RETURN( pWardrobeWeapon, "Expected wardrobe weapon" );
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// init to defaults
	sWardrobeWeaponInit( pWardrobeWeapon );
		
	// save item id
	pWardrobeWeapon->idWeaponReal = UnitGetId( pUnit );
	
	// save item type
	pWardrobeWeapon->nClass = UnitGetClass( pUnit );
	
	// get the stat tracking the requested affixes
	int nStat = AffixGetNameStat( AF_APPLIED );

	// get the stats list	
	STATS_ENTRY tStatsListAffixes[ MAX_AFFIX_ARRAY_PER_UNIT ];	
	int nCount = UnitGetStatsRange( pUnit, nStat, 0, tStatsListAffixes, AffixGetMaxAffixCount() );
	for (int i = 0; i < nCount; ++i)
	{
		int nAffix = tStatsListAffixes[ i ].value;	
		pWardrobeWeapon->nAffixes[ i ] = nAffix;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void WardrobeApplyBodyDataToInit(
	GAME * pGame,
	WARDROBE_INIT & tWardrobeInit,
	int nBodyData,
	int nWardrobeAppearanceGroup )
{
	const WARDROBE_BODY_DATA * pWardrobeBodyData = (WARDROBE_BODY_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_BODY, nBodyData );
	ASSERT_RETURN( pWardrobeBodyData );
	MemoryCopy( &tWardrobeInit.tBody, sizeof( WARDROBE_BODY ), 
		&pWardrobeBodyData->tBody, sizeof( WARDROBE_BODY ) );
	if ( pWardrobeBodyData->bRandomizeBody )
	{
		sRandomizeBody( pGame, tWardrobeInit.tBody, nWardrobeAppearanceGroup, FALSE );
	}
	if ( pWardrobeBodyData->pnRandomLayerSets[ 0 ] != INVALID_ID )
	{
		sAddRandomLayerSetsToBody( pGame, tWardrobeInit.tBody, pWardrobeBodyData );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL c_sWardrobeApplyAppearanceDef(
	WARDROBE * pWardrobe,
	BOOL bForce,
	BOOL bClearOldLayers,
	BOOL bAddBody )
{
	if ( !bForce && ( pWardrobe->dwFlags & WARDROBE_FLAG_APPEARANCE_DEF_APPLIED ) != 0 )
		return TRUE;

	int nAppearanceDefId = pWardrobe->pClient->nAppearanceDefId;
	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDefId );
	if ( ! pAppearanceDef )
	{
		return FALSE;
	}

	GAME * pGame = AppGetCltGame();

	if ( pWardrobe->pClient->nAppearanceGroup == INVALID_ID )
		pWardrobe->pClient->nAppearanceGroup = pAppearanceDef->nWardrobeAppearanceGroup;

	int nAppearanceLayers = 0;
	for ( int i = 0; i < MAX_WARDROBE_LAYERS_PER_APPEARANCE; i++ )
	{
		if ( pAppearanceDef->pnWardrobeLayerIds[ i ] != INVALID_ID )
			nAppearanceLayers++;
	}

	if ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_WARDROBE_USES_COLORMASK )
		pWardrobe->dwFlags |= WARDROBE_FLAG_USES_COLORMASK;

	if ( ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_WARDROBE_USES_INVENTORY ) != 0 &&
		 (pWardrobe->dwFlags & WARDROBE_FLAG_DOES_NOT_NEED_SERVER_UPDATE) == 0 )
		pWardrobe->dwFlags |= WARDROBE_FLAG_REQUIRES_UPDATE_FROM_SERVER;

	for ( int ii = 0; ii < pWardrobe->pClient->nCostumeCount; ii++ )
	{
		COSTUME & tCostume = pWardrobe->pClient->pCostumes[ ii ];

		BOOL bAddAppearanceLayers = TRUE;

		if ( pWardrobe->dwFlags & WARDROBE_FLAG_IGNORE_APPEARANCE_DEF_LAYERS )
			bAddAppearanceLayers = FALSE;
		else
			tCostume.tLayerBase.nId = pAppearanceDef->nWardrobeBaseId;
		
		if ( bClearOldLayers )
			tCostume.nLayerCount = 0;

		if ( bClearOldLayers && pWardrobe->pClient->nForceLayer != INVALID_ID )
		{
			tCostume.nLayerCount ++;
			sCostumeUpdateLayerArray( pGame, tCostume );
			tCostume.ptLayers[ tCostume.nLayerCount - 1 ].nId = pWardrobe->pClient->nForceLayer;
			tCostume.ptLayers[ tCostume.nLayerCount - 1 ].wParam = 0;
			bAddAppearanceLayers = FALSE;
		}
		if ( nAppearanceLayers > 0 && bAddAppearanceLayers )
		{
			int nLayerCountCurr = tCostume.nLayerCount;
			tCostume.nLayerCount += nAppearanceLayers;
			sCostumeUpdateLayerArray( pGame, tCostume );
			for ( int i = 0; i < MAX_WARDROBE_LAYERS_PER_APPEARANCE; i++ )
			{
				if ( pAppearanceDef->pnWardrobeLayerIds[ i ] != INVALID_ID )
				{
					tCostume.ptLayers[ nLayerCountCurr ].nId = pAppearanceDef->pnWardrobeLayerIds[ i ];
					tCostume.ptLayers[ nLayerCountCurr ].wParam = (WORD)pAppearanceDef->pnWardrobeLayerParams[ i ];
					nLayerCountCurr++;
				}
			}
		}

		if ( bAddBody )
		{ // the server usually incorporates the body layers in its message, but there is no server in tool mode
			int nBodyLayers = 0;
			for ( int i = 0; i < MAX_WARDROBE_BODY_LAYERS; i++ )
			{
				if ( pWardrobe->tBody.pnLayers[ i ] != INVALID_ID )
					nBodyLayers++;
			}
			int nLayerCountCurr = tCostume.nLayerCount;
			tCostume.nLayerCount += nBodyLayers;
			sCostumeUpdateLayerArray( pGame, tCostume );

			for ( int i = 0; i < MAX_WARDROBE_BODY_LAYERS; i++ )
			{
				if ( pWardrobe->tBody.pnLayers[ i ] != INVALID_ID )
				{
					tCostume.ptLayers[ nLayerCountCurr ].nId = pWardrobe->tBody.pnLayers[ i ];
					nLayerCountCurr++;
				}
			}
		}
		sCostumeSortLayers( tCostume );
	}

	pWardrobe->dwFlags |= WARDROBE_FLAG_APPEARANCE_DEF_APPLIED;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_WardrobeGetCount( int* pnTemplateCount /*= NULL*/, int* pnInstancedCount /*= NULL*/, int* pnTotalCount /*= NULL*/ )
{
	GAME* pGame = AppGetCltGame();

	if ( ! pGame )
		return 0;

	if ( ! pGame->pWardrobeHash )
		return 0;

	int nCount = 0;
	int nTemplateCount = 0;
	int nInstancedCount = 0;
	int nTotalCount = 0;

	WARDROBE_HASH& tWardrobeHash = (*pGame->pWardrobeHash);
	WARDROBE* pWardrobe = HashGetFirst( tWardrobeHash );
	while ( pWardrobe )
	{
		if ( pWardrobe->pClient->nWardrobeFallback != INVALID_ID )
			nInstancedCount++;
		else if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_WARDROBE_FALLBACK ) )
			nTemplateCount++;
		else if ( ! TEST_MASK( pWardrobe->dwFlags, 
						  WARDROBE_FLAG_SHARES_MODEL_DEF 
						| WARDROBE_FLAG_IGNORE_APPEARANCE_DEF_LAYERS ) ) // this flag excludes inventory items
			nCount++;

		nTotalCount++;
		pWardrobe = HashGetNext( tWardrobeHash, pWardrobe );
	}

	if ( pnTemplateCount )
		*pnTemplateCount = nTemplateCount;

	if ( pnInstancedCount )
		*pnInstancedCount = nInstancedCount;

	if ( pnTotalCount )
		*pnTotalCount = nTotalCount;

	return nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void WardrobeHashFree( GAME* pGame )
{
	if ( pGame->pWardrobeHash )
	{
		WARDROBE_HASH& tWardrobeHash = (*pGame->pWardrobeHash);
		
		WARDROBE* pWardrobe = HashGetFirst( tWardrobeHash );
		while ( pWardrobe )
		{
			WARDROBE* pWardrobeNext = HashGetNext( tWardrobeHash, pWardrobe );
			int nNextId = pWardrobeNext ? pWardrobeNext->nId : INVALID_ID;
			ASSERT( sWardrobeFree( pGame, pWardrobe ) );

			// It's possible that we could have freed our next wardrobe as well
			// if the fallback came directly after a fallback instance.
			if ( ! HashGet( tWardrobeHash, nNextId ) )
				pWardrobe = HashGetFirst( tWardrobeHash );
			else			
				pWardrobe = pWardrobeNext;
		}
		ASSERT( HashGetCount( tWardrobeHash ) == 0 );

		HashFree( tWardrobeHash );
		FREE( WARDROBE_MEMPOOL(pGame), pGame->pWardrobeHash );
		pGame->pWardrobeHash = NULL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int WardrobeInit ( 
	GAME* pGame,
	const WARDROBE_INIT& tInit )
{
	ASSERT_RETINVALID( pGame );

	if ( ! pGame->pWardrobeHash )
	{
		pGame->pWardrobeHash = MALLOC_NEW( WARDROBE_MEMPOOL(pGame), WARDROBE_HASH );
		ASSERT_RETINVALID( pGame->pWardrobeHash );
		HashInit( (*pGame->pWardrobeHash), WARDROBE_MEMPOOL(pGame), 16 );
	}

	ASSERT_RETINVALID( pGame->pWardrobeHash );
	WARDROBE* pWardrobe = HashAdd( (*pGame->pWardrobeHash), sgnNextWardrobeId++ );
	ASSERT_RETINVALID( pWardrobe );
	pWardrobe->tRefCount.AddRef();

	MemoryCopy( &pWardrobe->tBody, sizeof( WARDROBE_BODY ), 
		&tInit.tBody, sizeof( WARDROBE_BODY ) );

	if ( pGame && IS_SERVER( pGame ) )
	{
		pWardrobe->pServer = (WARDROBE_SERVER*)MALLOCZ(WARDROBE_MEMPOOL(pGame), sizeof(WARDROBE_SERVER));
		pWardrobe->pServer->tCostume.nColorSetIndex = tInit.nColorSetIndex;
	} else {
#if !ISVERSION(SERVER_VERSION)
		BOOL bSharesModelDefinition = tInit.bSharesModelDefinition;

		if ( tInit.bDoesNotNeedServerUpdate )
			pWardrobe->dwFlags |= WARDROBE_FLAG_DOES_NOT_NEED_SERVER_UPDATE;

		pWardrobe->pClient = (WARDROBE_CLIENT*)MALLOCZ(WARDROBE_MEMPOOL(pGame), sizeof(WARDROBE_CLIENT));

		pWardrobe->pClient->ePriority			= tInit.ePriority;
		pWardrobe->pClient->nLOD				= tInit.nLOD;
		pWardrobe->pClient->nMipBonus			= tInit.nMipBonus;
		pWardrobe->pClient->nAppearanceDefId	= tInit.nAppearanceDefId;
		pWardrobe->pClient->nAppearanceGroup	= tInit.nAppearanceGroup;
		pWardrobe->pClient->nModelDefOverride	= INVALID_ID;
		pWardrobe->pClient->nForceLayer			= INVALID_ID;
		pWardrobe->pClient->nUnittype			= tInit.nUnittype;
		pWardrobe->pClient->nWardrobeFallback	= INVALID_ID;

		int nWardrobeLimit = e_OptionState_GetActive()->get_nWardrobeLimit();
		if ( (   tInit.ePriority != WARDROBE_PRIORITY_PC_LOCAL  // We don't want to use a fallback for the player
			  && tInit.ePriority != WARDROBE_PRIORITY_PC_PARTY  // We don't want to use a fallback for party members
			  && AppGetState() != APP_STATE_CHARACTER_CREATE	// We don't want to use a fallback for the character create screen
			  && nWardrobeLimit > 0 && c_WardrobeGetCount() > nWardrobeLimit )
			|| tInit.bForceFallback )
		{
			if ( tInit.nWardrobeFallback != INVALID_ID )
			{
				WARDROBE* pWardrobeFallback = sWardrobeGet( pGame, tInit.nWardrobeFallback );
				if ( pWardrobeFallback )
				{
					int nModelDefOverride = c_WardrobeGetModelDefinition( tInit.nWardrobeFallback );
					ASSERT( nModelDefOverride != INVALID_ID );

					if ( nModelDefOverride != INVALID_ID )
					{
						pWardrobe->pClient->nWardrobeFallback = tInit.nWardrobeFallback;
						pWardrobeFallback->tRefCount.AddRef();

						bSharesModelDefinition = TRUE;
						pWardrobe->pClient->nModelDefOverride = nModelDefOverride;				
					}					
				}
			}
		}

		if (	 AppIsTugboat() 
			|| ! e_GetRenderFlag( RENDER_FLAG_WARDROBE_LOD )
			||	 e_ModelBudgetGetMaxLOD( MODEL_GROUP_WARDROBE ) > LOD_LOW )
		{
			// Don't force LODs for the local player and members in the player's
			// party.
			if (   tInit.ePriority == WARDROBE_PRIORITY_PC_LOCAL
				|| tInit.ePriority == WARDROBE_PRIORITY_PC_PARTY
				|| ! tInit.bForceLOD )
				pWardrobe->pClient->nLOD = LOD_HIGH;
		}
		
		pWardrobe->pClient->nCostumeCount = 1;

		if ( bSharesModelDefinition )
			pWardrobe->dwFlags |= WARDROBE_FLAG_SHARES_MODEL_DEF;

		// intermediate step - just use one costume
		pWardrobe->pClient->nCostumeToDraw   = 0;
		pWardrobe->pClient->nCostumeToUpdate = 0;

		if ( pWardrobe->pClient->nCostumeCount > 0 )
		{
			pWardrobe->pClient->pCostumes = (COSTUME*)MALLOCZ(WARDROBE_MEMPOOL(pGame), sizeof(COSTUME) * pWardrobe->pClient->nCostumeCount);
			for ( int i = 0; i < pWardrobe->pClient->nCostumeCount; i++ )
			{
				COSTUME& tCostume = pWardrobe->pClient->pCostumes[ i ];
				sCostumeInit( pGame, *pWardrobe, tCostume, pWardrobe->pClient->nLOD, tInit.nColorSetIndex );
			}
		}

		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
			pWardrobe->pClient->pnWeaponAttachment[ i ] = INVALID_ID;

		V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );
#endif //!SERVERVERSION
	}

	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		WARDROBE_WEAPON *pWardrobeWeapon = &pWardrobe->tWeapons[ i ];
		sWardrobeWeaponInit( pWardrobeWeapon );
	}

	CLT_VERSION_ONLY(e_WardrobeInit());

	return pWardrobe->nId;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void sCostumeFree( GAME* pGame,	WARDROBE& tWardrobe, COSTUME& tCostume )
{
	ASSERT( tCostume.nLayersAllocated == 0 || tCostume.ptLayers );
	FREE( WARDROBE_MEMPOOL(pGame), tCostume.ptLayers );
	tCostume.ptLayers = NULL;

	for ( int nTextureGroup = 0; 
		      nTextureGroup < MAX_WARDROBE_TEXTURE_GROUPS; 
			  nTextureGroup++ )
	{
		tCostume.tLayerStack[ nTextureGroup ].Destroy();
	}
	tCostume.tModelLayerStack.Destroy();

	if ( ( tWardrobe.dwFlags & WARDROBE_FLAG_SHARES_MODEL_DEF ) == 0 )
	{
		for ( int ii = 0; ii < MAX_WARDROBE_TEXTURE_GROUPS; ii++ )
		{
			for ( int jj = 0; jj < NUM_TEXTURES_PER_SET; jj++ )
			{
				e_TextureReleaseRef( tCostume.ppnTextures[ ii ][ jj ], TRUE );
			}
		}

		// This will now perform the cleanup immediately when the model def refcount hits 0.
		e_ModelDefinitionReleaseRef( tCostume.nModelDefinitionId, TRUE );
		tCostume.nModelDefinitionId = INVALID_ID;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL sWardrobeFree(
	GAME* pGame,
	WARDROBE* pWardrobe )
{
	if( ! pWardrobe )
		return FALSE;

#if ! ISVERSION(SERVER_VERSION)
	if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_SHARES_MODEL_DEF ) )
	{
		WARDROBE* pWardrobeFallback = sWardrobeGet( pGame, pWardrobe->pClient->nWardrobeFallback );
		if ( pWardrobeFallback )
		{
			pWardrobeFallback->tRefCount.Release();
			pWardrobe->pClient->nWardrobeFallback = INVALID_ID;

			// If there is only one reference left, then that means there are no
			// other wardrobes referencing the fallback wardrobe.
			if ( pWardrobeFallback->tRefCount.GetCount() == 1 )
				sWardrobeFree( pGame, pWardrobeFallback );
		}
	}
	else if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_WARDROBE_FALLBACK ) )
	{
		if ( pWardrobe->tRefCount.GetCount() > 1 )
			return TRUE; // We will get freed when the last instance is freed.
	}
#endif

	pWardrobe->tRefCount.Release();
	if ( pWardrobe->tRefCount.GetCount() != 0 )
		return FALSE;

	if (!pGame || IS_CLIENT(pGame))
	{
#if ! ISVERSION(SERVER_VERSION)
		if ( pGame )
		{
			for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
			{
				WARDROBE_WEAPON *pWardrobeWeapon = &pWardrobe->tWeapons[ i ];
				UNIT * pWeapon = UnitGetById( pGame, pWardrobeWeapon->idWeaponPlaceholder );
				if ( pWeapon )
				{
					ASSERT( UnitTestFlag( pWeapon, UNITFLAG_CLIENT_ONLY ) );
					if ( UnitTestFlag( pWeapon, UNITFLAG_CLIENT_ONLY ) )
						UnitFree( pWeapon ); // this is just a placeholder item for the client
				}
			}
		}

		ASSERT_RETFALSE( pWardrobe->pClient );

		V( c_sWardrobeRemoveFromUpdateList( pGame, pWardrobe ) );

		for ( int i = 0; i < pWardrobe->pClient->nCostumeCount; i++ )
		{
			COSTUME& tCostume = pWardrobe->pClient->pCostumes[ i ];		
			sCostumeFree( pGame, *pWardrobe, tCostume );
		}
		ASSERT_RETFALSE( pWardrobe->pClient->pCostumes );
		FREE( WARDROBE_MEMPOOL(pGame), pWardrobe->pClient->pCostumes );
		pWardrobe->pClient->pCostumes = NULL;
		
		ASSERT_RETFALSE( pWardrobe->pClient );
		FREE( WARDROBE_MEMPOOL(pGame), pWardrobe->pClient );
		pWardrobe->pClient = NULL;
#endif //  ! ISVERSION(SERVER_VERSION)
	} 
	else
	{
		ASSERT_RETFALSE( pWardrobe->pServer->tCostume.nLayersAllocated == 0 
					  || pWardrobe->pServer->tCostume.ptLayers );
		FREE( WARDROBE_MEMPOOL(pGame), pWardrobe->pServer->tCostume.ptLayers );
		pWardrobe->pServer->tCostume.ptLayers = NULL;

		ASSERT_RETFALSE( pWardrobe->pServer )
		FREE( WARDROBE_MEMPOOL(pGame), pWardrobe->pServer );
		pWardrobe->pServer = NULL;
	}

	HashRemove( (*pGame->pWardrobeHash), pWardrobe->nId );

	return TRUE;
}

void WardrobeFree ( 
	GAME * pGame,
	int nWardrobe )
{
	if ( nWardrobe == INVALID_ID )
		return;

	if ( ! pGame )
		return;
	
	if ( ! pGame->pWardrobeHash )
		return;

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	sWardrobeFree( pGame, pWardrobe );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT c_WardrobeSetMipBonusEnabled ( 
	int nWardrobe,
	BOOL bEnabled )
{
#if !ISVERSION( SERVER_VERSION )
	GAME* pGame = AppGetCltGame();	
	ASSERT_RETINVALIDARG( !pGame || IS_CLIENT( pGame ) );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe->pClient );

	DWORD dwFlagsBeforeChange = pWardrobe->dwFlags;
	if ( bEnabled )
		CLEAR_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_IGNORE_MIP_BONUS );
	else
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_IGNORE_MIP_BONUS );
	
	if ( dwFlagsBeforeChange != pWardrobe->dwFlags )
	{
		COSTUME & tCostume = sGetCostumeToUpdate( pWardrobe );
		for( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
		{
			for ( int jj = 0; jj < NUM_TEXTURES_PER_SET; jj++ )
			{
				e_TextureReleaseRef( tCostume.ppnTextures[ i ][ jj ] );
				tCostume.ppnTextures[ i ][ jj ] = INVALID_ID;
			}

			tCostume.dwTextureFlags[ i ] = 0;
		}

		// Upgrade the costume count, so we can do the work in the background and
		// swap the costumes after the work has been completed.
		V( c_WardrobeUpgradeCostumeCount( pWardrobe->nId ) );

		V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );

		// Queue the wardrobe to remove the extra costume once the mip bonus has
		// been removed.
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_COSTUME_COUNT );
	}
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT c_WardrobeUpgradeCostumeCount( int nWardrobe, BOOL bKeepCostumeUpgraded /*=FALSE*/ )
{
#if !ISVERSION( SERVER_VERSION )
	GAME* pGame = AppGetCltGame();
	ASSERT_RETINVALIDARG( !pGame || IS_CLIENT( pGame ) );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe->pClient );

	if ( bKeepCostumeUpgraded )
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_KEEP_COSTUME_COUNT_UPGRADED );

	if ( c_WardrobeIsUpdated( pWardrobe->nId ) )
		CLEAR_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_COSTUME_COUNT );
	else
	{
		// Wardrobe is still updating, so queue this for later.
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_COSTUME_COUNT );
		return S_OK;
	}

	if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_SHARES_MODEL_DEF ) )
		return S_FALSE; // Only one costume is allowed for models that share model definitions.

	int nOldCostumeCount = pWardrobe->pClient->nCostumeCount;
	// Only upgrade costume count if we haven't maxed out.
	if ( nOldCostumeCount >= MAX_COSTUME_COUNT ) 
		return S_FALSE;

	pWardrobe->pClient->nCostumeCount++;
	pWardrobe->pClient->pCostumes = (COSTUME*)REALLOC( WARDROBE_MEMPOOL(pGame), 
		pWardrobe->pClient->pCostumes, sizeof( COSTUME ) * pWardrobe->pClient->nCostumeCount );

	BYTE* pbNewCostumesStart = (BYTE*)pWardrobe->pClient->pCostumes + ( sizeof(COSTUME) * nOldCostumeCount );
	DWORD dwNewCostumesSize = sizeof( COSTUME ) * ( pWardrobe->pClient->nCostumeCount - nOldCostumeCount );
	ZeroMemory( pbNewCostumesStart, dwNewCostumesSize );

	COSTUME& tCostumeTemplate = sGetCostumeToDraw( pWardrobe ); // pWardrobe->pClient->pCostumes[ pWardrobe->pClient->nCostumeToDraw ];	
	for ( int i = nOldCostumeCount; i < pWardrobe->pClient->nCostumeCount; i++ )
	{
		COSTUME& tCostume = pWardrobe->pClient->pCostumes[ i ];
		sCostumeInit( pGame, *pWardrobe, tCostume, pWardrobe->pClient->nLOD, tCostumeTemplate.nColorSetIndex );

		tCostume.tLayerBase = tCostumeTemplate.tLayerBase;
		if ( tCostumeTemplate.nLayerCount )
		{
			tCostume.nLayerCount = tCostumeTemplate.nLayerCount;
			sCostumeUpdateLayerArray( pGame, tCostume );

			int nSize = sizeof( WARDROBE_LAYER_INSTANCE ) * tCostumeTemplate.nLayerCount;
			int nDestSize = sizeof( WARDROBE_LAYER_INSTANCE ) * tCostume.nLayerCount;
			MemoryCopy( tCostume.ptLayers, nDestSize, tCostumeTemplate.ptLayers, nSize );
		}
	}

	if ( tCostumeTemplate.nLOD != pWardrobe->pClient->nLOD )
	{
		const char* pszName = DefinitionGetNameById( DEFINITION_GROUP_APPEARANCE, 
			pWardrobe->pClient->nAppearanceDefId );
		tCostumeTemplate.nModelDefinitionId = e_WardrobeModelDefinitionNewFromMemory( pszName, 
			pWardrobe->pClient->nLOD, tCostumeTemplate.nModelDefinitionId );
		ASSERT( tCostumeTemplate.nModelDefinitionId != INVALID_ID );

		tCostumeTemplate.bInitialized = FALSE;
	}

	// Clear model definition initialization flag.
	CLEAR_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_MODEL_DEF_INITIALIZED );

	if ( c_WardrobeIsUpdated( pWardrobe->nId ) )
		pWardrobe->pClient->nCostumeToUpdate = (pWardrobe->pClient->nCostumeToDraw + 1) % pWardrobe->pClient->nCostumeCount;
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT c_WardrobeDowngradeCostumeCount( int nWardrobe )
{
#if !ISVERSION( SERVER_VERSION )
	GAME* pGame = AppGetCltGame();
	ASSERT_RETINVALIDARG( !pGame || IS_CLIENT( pGame ) );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe->pClient );

	if ( c_WardrobeIsUpdated( pWardrobe->nId ) )
		CLEAR_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_COSTUME_COUNT );
	else
	{
		// Wardrobe is still updating, so queue this for later.
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_COSTUME_COUNT );
		return S_OK;
	}

	if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_SHARES_MODEL_DEF ) )
		return S_FALSE; // Only one costume is allowed for models that share model definitions.

	if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_KEEP_COSTUME_COUNT_UPGRADED ) )
		return S_FALSE;

	int nOldCostumeCount = pWardrobe->pClient->nCostumeCount;
	// Only downgrade costume count if we have more than one costume.
	if ( nOldCostumeCount <= 1 ) 
		return S_FALSE;

	pWardrobe->pClient->nCostumeCount--;

	// If we want to remove the costume that we are drawing from, 
	// then relocate the costume first.
	if ( pWardrobe->pClient->nCostumeToDraw == pWardrobe->pClient->nCostumeCount )
	{
		// Free any resources used by the costume that we are going to
		// overwrite before reallocating memory.
		COSTUME& tCostumeToReplace = sGetCostumeToUpdate( pWardrobe );
		sCostumeFree( pGame, *pWardrobe, tCostumeToReplace );

		COSTUME& tCostumeToDraw = sGetCostumeToDraw( pWardrobe );
		tCostumeToReplace = tCostumeToDraw;
		pWardrobe->pClient->nCostumeToDraw = pWardrobe->pClient->nCostumeToUpdate;
	}
	else
	{
		// Free any resources used by the costume that we are going to remove.
		COSTUME& tCostumeToRemove = pWardrobe->pClient->pCostumes[ pWardrobe->pClient->nCostumeCount ];
		sCostumeFree( pGame, *pWardrobe, tCostumeToRemove );
	}

	pWardrobe->pClient->pCostumes = (COSTUME*)REALLOC( WARDROBE_MEMPOOL(pGame), 
		pWardrobe->pClient->pCostumes, sizeof( COSTUME ) * pWardrobe->pClient->nCostumeCount );

	pWardrobe->pClient->nCostumeToUpdate = (pWardrobe->pClient->nCostumeToDraw + 1) % pWardrobe->pClient->nCostumeCount;
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT c_WardrobeUpgradeLOD( int nWardrobe, BOOL bForce /*=FALSE*/ )
{
#if !ISVERSION( SERVER_VERSION )
	GAME* pGame = AppGetCltGame();
	ASSERT_RETINVALIDARG( !pGame || IS_CLIENT( pGame ) );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe->pClient );

	if ( e_ModelBudgetGetMaxLOD( MODEL_GROUP_WARDROBE ) <= LOD_LOW && ! bForce )
		return S_FALSE;

	if ( ( pWardrobe->pClient->nLOD == LOD_HIGH )
	    || c_WardrobeIsUpdated( pWardrobe->nId ) )
		CLEAR_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_LOD );
	else
	{
		// Wardrobe is still updating, so queue this for later.
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_LOD );
		return S_OK;
	}

	if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_SHARES_MODEL_DEF ) )
		// The parent model def should be upgraded.
		return S_FALSE;

	if ( pWardrobe->pClient->nLOD == LOD_LOW )
	{
		pWardrobe->pClient->nLOD = LOD_HIGH;

		// Upgrade the costume count, so we can do the work in the background and
		// swap the costumes after the work has been completed.
		V( c_WardrobeUpgradeCostumeCount( pWardrobe->nId ) );

		V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );

		// Queue the wardrobe to remove the extra costume once the lod has been upgraded.
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_COSTUME_COUNT );
	}
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT c_WardrobeDowngradeLOD( int nWardrobe )
{
#if !ISVERSION( SERVER_VERSION )
	GAME* pGame = AppGetCltGame();
	ASSERT_RETINVALIDARG( !pGame || IS_CLIENT( pGame ) );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe );
	ASSERT_RETINVALIDARG( pWardrobe->pClient );

	if ( ( pWardrobe->pClient->nLOD == LOD_LOW )
	    || c_WardrobeIsUpdated( pWardrobe->nId ) )
		CLEAR_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_LOD );
	else
	{
		// Wardrobe is still updating, so queue this for later.
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_LOD );
		return S_OK;
	}

	if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_SHARES_MODEL_DEF ) )
		return S_FALSE; // The parent model def should be upgraded.

	if ( pWardrobe->pClient->nLOD == LOD_HIGH )
	{
		pWardrobe->pClient->nLOD = LOD_LOW;

		// Upgrade the costume count, so we can do the work in the background and
		// swap the costumes after the work has been completed.
		V( c_WardrobeUpgradeCostumeCount( pWardrobe->nId ) );

		V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );

		// Queue the wardrobe to remove the extra costume once the lod has been downgraded.
		SET_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_COSTUME_COUNT );
	}
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sAllLayerTexturesLoaded( 
				GAME * pGame,
				WARDROBE * pWardrobe,
				COSTUME & tCostume,
				int nTextureIndex, // MAX_WARDROBE_TEXTURE_GROUPS
				LAYER_TEXTURE_TYPE eLayerTextureType )
{
#if !ISVERSION(SERVER_VERSION)
	if ( pWardrobe->dwFlags & WARDROBE_FLAG_SHARES_MODEL_DEF )
		return TRUE;

	WARDROBE_LAYER_STACK& tLayerStack = tCostume.tLayerStack[nTextureIndex];
	if ( tLayerStack.Count() == 0 )
		return TRUE;

	int nBaseStackId = tLayerStack.GetFirst();
	WARDROBE_LAYER_STACK_ELEMENT* pBaseElement = tLayerStack.Get( nBaseStackId );
	int nLayerBase = sWardrobeGetLayer( pGame, pWardrobe, &tCostume, 
		pBaseElement->nLayerId, nTextureIndex, FALSE );
	if ( nLayerBase == INVALID_ID )
		return TRUE;

	WARDROBE_LAYER_DATA* pBaseLayer = (WARDROBE_LAYER_DATA *)ExcelGetData( 
		pGame, DATATABLE_WARDROBE_LAYER, nLayerBase );
	ASSERT_RETTRUE( pBaseLayer );
	ASSERT_RETTRUE( pBaseLayer->nTextureSetGroup != INVALID_ID );

	int nLOD = tCostume.nLOD;
	int nBaseRGB = sLayerGetTexture( pGame, pWardrobe, &tCostume, 
		nLayerBase, eLayerTextureType, nLOD );
	if ( !e_TextureIsValidID( nBaseRGB ) && nBaseRGB != TEXTURE_WITH_DEFAULT_COLOR )
	{   
		// we will fill in a blank texture if they tell us what size it is.  
		// Sometimes the base is just blank.
		return TRUE;
	}

	// Keep a state variable instead of returning immediately, so all layer 
	// textures are checked. By checking for the layer textures, we implicitly
	// issue a load request.
	BOOL bEverythingIsLoaded = TRUE;

	if ( bEverythingIsLoaded && e_TextureIsValidID( nBaseRGB ) 
		&& !e_TextureIsLoaded( nBaseRGB, TRUE ) 
		&& !e_TextureLoadFailed( nBaseRGB ) )
		bEverythingIsLoaded = FALSE;

	WARDROBE_TEXTURESET_DATA * pBaseTextureSet = 
		c_sWardrobeGetTextureSetForLayer( pGame, pWardrobe, pBaseLayer );
	ASSERT_RETTRUE( pBaseTextureSet );

	// Check if all the layers are loaded.
  	for( int nStackId = tLayerStack.GetNextId( nBaseStackId ); nStackId != INVALID_ID; 
		nStackId = tLayerStack.GetNextId( nStackId ) )
	{
		WARDROBE_LAYER_STACK_ELEMENT* pElement = tLayerStack.Get( nStackId );
		int nLayerId = pElement->nLayerId;

		if ( pElement->pBlendOp->bNoTextureChange )
			continue;

		int nLayerTextureId = sLayerGetTexture( pGame, pWardrobe, &tCostume, 
			nLayerId , eLayerTextureType, nLOD );
		int nBlendTextureId = sLayerGetTexture( pGame, pWardrobe, &tCostume, 
			nLayerId, LAYER_TEXTURE_BLEND, nLOD );

		if ( nLayerTextureId == TEXTURE_WITH_DEFAULT_COLOR || 
			e_TextureIsValidID( nLayerTextureId ) )
		{
			if ( nLayerTextureId != TEXTURE_WITH_DEFAULT_COLOR 
			  && bEverythingIsLoaded 
			  && !e_TextureIsLoaded( nLayerTextureId, TRUE )
			  && !e_TextureLoadFailed( nLayerTextureId ) )
				bEverythingIsLoaded = FALSE;

			if ( nBlendTextureId == BLEND_TEXTURE_DEFINITION_LOADING_ID )
				bEverythingIsLoaded = FALSE;
		}
	}

	return bEverythingIsLoaded;
#else
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pWardrobe);
	UNREFERENCED_PARAMETER(tCostume);
	UNREFERENCED_PARAMETER(nTextureIndex);
	UNREFERENCED_PARAMETER(eLayerTextureType);
	return FALSE;
#endif
}

//----------------------------------------------------------------------------

static BOOL sAddLayerTextures (
	GAME * pGame,
	WARDROBE * pWardrobe,
	COSTUME & tCostume,
	int nTextureIndex, // MAX_WARDROBE_TEXTURE_GROUPS
	LAYER_TEXTURE_TYPE eLayerTextureType,
	int& nUpdateCount )
{
#if !ISVERSION(SERVER_VERSION)
	if ( pWardrobe->dwFlags & WARDROBE_FLAG_SHARES_MODEL_DEF )
		return TRUE;

	WARDROBE_LAYER_STACK& tLayerStack = tCostume.tLayerStack[ nTextureIndex ];
	if ( tLayerStack.Count() == 0 )
		return TRUE;

	int nBaseStackId = tLayerStack.GetFirst();
	WARDROBE_LAYER_STACK_ELEMENT* pBaseElement = tLayerStack.Get( nBaseStackId );
	int nLayerBase = sWardrobeGetLayer( pGame, pWardrobe, &tCostume, 
		pBaseElement->nLayerId, nTextureIndex, FALSE );
	if ( nLayerBase == INVALID_ID )
		return TRUE;
	
	int nLOD = tCostume.nLOD;
	int nBaseRGB = sLayerGetTexture( pGame, pWardrobe, &tCostume, nLayerBase, 
		eLayerTextureType, nLOD );
	if ( !e_TextureIsValidID( nBaseRGB ) && nBaseRGB != TEXTURE_WITH_DEFAULT_COLOR )
	{
		// we will fill in a blank texture if they tell us what size it 
		// is.  Sometimes the base is just blank.
		return TRUE;
	}

	WARDROBE_LAYER_DATA * pBaseLayer = (WARDROBE_LAYER_DATA*)ExcelGetData( pGame, 
		DATATABLE_WARDROBE_LAYER, nLayerBase );
	ASSERT_RETTRUE( pBaseLayer );
	ASSERT_RETTRUE( pBaseLayer->nTextureSetGroup != INVALID_ID );

	int nCanvasWidth = 0;
	int nCanvasHeight = 0;

	WARDROBE_TEXTURESET_DATA * pBaseTextureSet = 
		c_sWardrobeGetTextureSetForLayer( pGame, pWardrobe, pBaseLayer );
	ASSERT_RETTRUE( pBaseTextureSet );


	if ( eLayerTextureType == LAYER_TEXTURE_COLORMASK )
	{
		// COLORMASK AND DIFFUSE_POSTCOLORMASK, must match the diffuse texture's size.
		int nDiffuse = tCostume.ppnTextures[ nTextureIndex ][ LAYER_TEXTURE_DIFFUSE ];
		if ( !e_TextureIsValidID( nDiffuse ) )
			return FALSE;

		e_TextureGetSize( nDiffuse,	nCanvasWidth, nCanvasHeight );

		int nCurrentTexture = tCostume.ppnTextures[ nTextureIndex ][ eLayerTextureType ];
		if ( e_TextureIsValidID( nCurrentTexture ) )
		{
			int nCurrentWidth = 0;
			int nCurrentHeight = 0;

			e_TextureGetSize( nCurrentTexture, nCurrentWidth, nCurrentHeight );

			if ( nCurrentWidth != nCanvasWidth || nCurrentHeight != nCanvasHeight )
			{
				e_TextureReleaseRef( nCurrentTexture, TRUE );
				tCostume.ppnTextures[ nTextureIndex ][ eLayerTextureType ] = INVALID_ID;
			}	
		}
	}
	else
	{
		sTextureGetSize( pBaseTextureSet, eLayerTextureType, nLOD, nCanvasWidth, nCanvasHeight, FALSE );
		
		if ( ! TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_IGNORE_MIP_BONUS ) )
		{
			const int nMipBonus = pWardrobe->pClient->nMipBonus;
			nCanvasHeight >>= nMipBonus;
			nCanvasWidth >>= nMipBonus;
		}
	}

	// AE 2007.11.28: If the base layer for the canvas is a default color and 
	// all layers being applied also are a default color, then skip canvas 
	// creation altogether.
	if ( nBaseRGB == TEXTURE_WITH_DEFAULT_COLOR )
	{
		BOOL bSkipCanvasCreation = TRUE;
		for( int nStackId = tLayerStack.GetNextId( nBaseStackId); 
			nStackId != INVALID_ID; nStackId = tLayerStack.GetNextId( nStackId ) )
		{
			WARDROBE_LAYER_STACK_ELEMENT* pElement = tLayerStack.Get( nStackId );
			int nLayerId = pElement->nLayerId;

			if ( pElement->pBlendOp->bNoTextureChange )
				continue;

			int nLayerTextureId = sLayerGetTexture( pGame, pWardrobe, &tCostume, 
				nLayerId , eLayerTextureType, nLOD );

			if ( nLayerTextureId != TEXTURE_WITH_DEFAULT_COLOR )
			{
				bSkipCanvasCreation = FALSE;
				break;
			}
		}

		if ( bSkipCanvasCreation )
			return TRUE;
	}

	// AE 2007.11.28: Set to 1 if we go back to keeping sysmem textures around for
	//		certain canvases.
#if 0
	DWORD dwTextureFlags = 0;			
	if ( sWardrobeModelDefUsesTexture( pWardrobe, eLayerTextureType ) )
		dwTextureFlags = TEXTURE_FLAG_USE_SCRATCHMEM;
#else
	DWORD dwTextureFlags = TEXTURE_FLAG_USE_SCRATCHMEM; 
#endif

	if ( eLayerTextureType == LAYER_TEXTURE_COLORMASK )
		dwTextureFlags |= TEXTURE_FLAG_SYSMEM;

	int* pnCanvasTextureId = &tCostume.ppnTextures[ nTextureIndex ][ eLayerTextureType ];
	V( e_WardrobeTextureAllocateBaseCanvas( *pnCanvasTextureId, eLayerTextureType, dwTextureFlags ) );

	// If the base layer is included in the layer stack, then make the base 
	// canvas from those textures.
	BOOL bBaseRGBLoadFailed = e_TextureLoadFailed( nBaseRGB );
	{
		//TIMER_START_NEEDED_EX("MakeBaseCanvas", 12);
		V( e_WardrobeTextureMakeBaseCanvas( 
			*pnCanvasTextureId, 
			bBaseRGBLoadFailed ? INVALID_ID : nBaseRGB, 
			eLayerTextureType, 
			bBaseRGBLoadFailed,
			nCanvasWidth, nCanvasHeight ) );
	}

	if ( sWardrobeModelDefUsesTexture( pWardrobe, eLayerTextureType ) )
	{
		V( e_WardrobeSetTexture( tCostume.nModelDefinitionId, nLOD, *pnCanvasTextureId, 
			eLayerTextureType, nTextureIndex ) );
	}

	// Add the layers on top of the base texture.
	for( int nStackId = tLayerStack.GetNextId( nBaseStackId ); 
		nStackId != INVALID_ID; nStackId = tLayerStack.GetNextId( nStackId ) )
	{
		WARDROBE_LAYER_STACK_ELEMENT* pElement = tLayerStack.Get( nStackId );
		int nLayerId = pElement->nLayerId;

		if ( pElement->pBlendOp->bNoTextureChange )
			continue;

		int nLayerTextureId = sLayerGetTexture( pGame, pWardrobe, &tCostume, 
			nLayerId , eLayerTextureType, nLOD );
		int nBlendTextureId = sLayerGetTexture( pGame, pWardrobe, &tCostume, 
			nLayerId, LAYER_TEXTURE_BLEND, nLOD );

		if ( ( nLayerTextureId == TEXTURE_WITH_DEFAULT_COLOR 
			|| e_TextureIsValidID( nLayerTextureId ) ) 
			&& !e_TextureLoadFailed( nLayerTextureId ) )
		{
			if ( nBlendTextureId != INVALID_ID )
			{
				V( e_WardrobeAddTextureToCanvas( *pnCanvasTextureId, 
					nLayerTextureId, nBlendTextureId, eLayerTextureType ) );
			}
			else
			{
				V( e_WardrobeTextureMakeBaseCanvas( *pnCanvasTextureId, 
					nLayerTextureId, eLayerTextureType, FALSE ) );
			}
		}
	}

	//V( e_WardrobeCanvasComplete( *pnCanvasTextureId ) );

#ifdef HAMMER
	//if ( eLayerTextureType == LAYER_TEXTURE_DIFFUSE )
	//	HolodeckGet()->SetGridTexture( pCard->nId );
#endif

	WARDROBE_TRACE("Wardrobe texture updated LOD %d %s\n", nLOD, pBaseTextureSet->pszTextureFilenames[eLayerTextureType]);

	nUpdateCount++;

	return TRUE;
#else
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pWardrobe);
	UNREFERENCED_PARAMETER(tCostume);
	UNREFERENCED_PARAMETER(nTextureIndex);
	UNREFERENCED_PARAMETER(eLayerTextureType);
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sAddAttachment (
	GAME * pGame,
	WARDROBE_LAYER_DATA * pLayerDef,
	WARDROBE * pWardrobe,
	char * pszFolder,
	int nModelId,
	int nAttachedId,
	WORD wBoneIndex,
	int nOldAttachmentCount,
	int pnOldAttachments[ MAX_ATTACHMENTS_PER_WARDROBE ])
{
	ASSERT_RETINVALID(pLayerDef);
	{
		ATTACHMENT_DEFINITION * pAttachmentDef = &pLayerDef->tAttach;
		if ( pAttachmentDef->pszAttached[ 0 ] == 0 && nAttachedId == INVALID_ID )
			return INVALID_ID;
	}
	ATTACHMENT_DEFINITION tAttachmentDef;
	MemoryCopy( &tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ), 
		&pLayerDef->tAttach, sizeof( ATTACHMENT_DEFINITION ) );

	char szBoneNameOld[ MAX_XML_STRING_LENGTH ];
	if ( pLayerDef->bHasBoneIndex )
	{
		PStrCopy( szBoneNameOld, tAttachmentDef.pszBone, MAX_XML_STRING_LENGTH );
		PStrPrintf(  tAttachmentDef.pszBone, MAX_XML_STRING_LENGTH, "%s%d", szBoneNameOld, wBoneIndex + 1 );
	}
	
	const char * pszFolderToUse = tAttachmentDef.eType == ATTACHMENT_MODEL ? pszFolder : NULL;
	c_AttachmentDefinitionLoad( pGame, tAttachmentDef, pWardrobe->pClient->nAppearanceDefId, pszFolderToUse );

	// in case they didn't name the bones properly and we have the first bone
	if (  tAttachmentDef.pszBone[ 0 ] != 0 && tAttachmentDef.nBoneId == INVALID_ID &&
		 pLayerDef->bHasBoneIndex && wBoneIndex == 0 )
	{
		PStrCopy(  tAttachmentDef.pszBone, szBoneNameOld, MAX_XML_STRING_LENGTH );
		c_AttachmentDefinitionLoad( pGame, tAttachmentDef, pWardrobe->pClient->nAppearanceDefId, pszFolderToUse );
	}

	int nAttachmentId = c_ModelAttachmentFind( nModelId, tAttachmentDef );
	pWardrobe->pClient->nAttachmentCount++;
	ASSERT( pWardrobe->pClient->nAttachmentCount < MAX_ATTACHMENTS_PER_WARDROBE );

	if ( nAttachmentId == INVALID_ID )
	{
		nAttachmentId = c_ModelAttachmentAdd( nModelId, tAttachmentDef, pszFolder, FALSE, 
											  ATTACHMENT_OWNER_WARDROBE, INVALID_ID, nAttachedId );
	} else {
		for ( int nOld = 0; nOld < nOldAttachmentCount; nOld++ )
		{
			if ( pnOldAttachments[ nOld ] == nAttachmentId )
			{
				pnOldAttachments[ nOld ] = INVALID_ID; // clear out the id so that we don't remove it later
			}
		}
	}
	pWardrobe->pClient->pnAttachmentIds[ pWardrobe->pClient->nAttachmentCount - 1 ] = nAttachmentId;

	if ( pLayerDef->bHasBoneIndex )
	{
		PStrCopy( tAttachmentDef.pszBone, szBoneNameOld, MAX_XML_STRING_LENGTH );
	}

	return nAttachmentId;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sAddCostumeAttachments ( 
	GAME * pGame, 
	WARDROBE * pWardrobe, 
	COSTUME & tCostume,
	int nModelId )
{
	int nOldAttachmentCount = pWardrobe->pClient->nAttachmentCount;
	int pnOldAttachments[ MAX_ATTACHMENTS_PER_WARDROBE ];
	MemoryCopy( pnOldAttachments, sizeof( pnOldAttachments ), 
		pWardrobe->pClient->pnAttachmentIds, 
		sizeof(int) * pWardrobe->pClient->nAttachmentCount );

	pWardrobe->pClient->nAttachmentCount = 0;
	for ( unsigned int i = 0; i < tCostume.nLayerCount; i++ )
	{
		int nLayerCurr = tCostume.ptLayers[ i ].nId;
		BOOL bFirstLoop = TRUE;
		while ( nLayerCurr != INVALID_ID )
		{
			nLayerCurr = sWardrobeGetLayer( pGame, pWardrobe, &tCostume, nLayerCurr, INVALID_ID, !bFirstLoop );
			if ( nLayerCurr == INVALID_ID )
				break;
			bFirstLoop = FALSE;
			WARDROBE_LAYER_DATA * pLayer = (WARDROBE_LAYER_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, nLayerCurr );
			if ( ! pLayer )
				continue;
			sAddAttachment ( pGame, pLayer, pWardrobe, 
				FILE_PATH_UNITS, nModelId, INVALID_ID,
				tCostume.ptLayers[ i ].wParam,
				nOldAttachmentCount, pnOldAttachments );
		}
	}

	for( int i = 0; i < nOldAttachmentCount; i++ )
	{
		if ( pnOldAttachments[ i ] != INVALID_ID )
		{
			c_ModelAttachmentRemove( nModelId, pnOldAttachments[ i ], 0 );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sWeaponResetAttachments ( 
	GAME* pGame,
	WARDROBE * pWardrobe, 
	int nWeaponIndex )
{
	UNIT * pWeapon = WardrobeGetWeapon( pGame, pWardrobe->nId, nWeaponIndex );
	if ( ! pWeapon )
		return;
	int nWeaponModel = c_UnitGetModelId( pWeapon );
	if ( nWeaponModel != INVALID_ID )
	{
		c_ModelAttachmentRemoveAllByOwner( nWeaponModel, ATTACHMENT_OWNER_ANIMATION, INVALID_ID );
		c_ModelAttachmentRemoveAllByOwner( nWeaponModel, ATTACHMENT_OWNER_SKILL, INVALID_ID );
		c_AppearanceForceInitAnimation( nWeaponModel );
		if ( pWeapon )
			c_UnitSetMode( pWeapon, MODE_ITEM_EQUIPPED_IDLE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeResetWeaponAttachments ( 
	int nWardrobe )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	if ( ! pWardrobe )
		return;
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		sWeaponResetAttachments ( pGame, pWardrobe, i );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sAddWeaponAttachments ( 
	GAME * pGame, 
	UNIT * pUnit,
	WARDROBE * pWardrobe, 
	int nModelId,
	BOOL bUseWeaponModel,
	BOOL bRecreateWeaponAttachments )
{
	BOOL pbAddAttachment[ MAX_WEAPONS_PER_UNIT ];
	BOOL pbRemoveOldAttachment[ MAX_WEAPONS_PER_UNIT ];
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		pbAddAttachment[ i ] = TRUE;
		pbRemoveOldAttachment[ i ] = bRecreateWeaponAttachments;

		int nWeaponAttachmentOld = pWardrobe->pClient->pnWeaponAttachment[ i ];
		
		pWeapons[ i ] = WardrobeGetWeapon( pGame, pWardrobe->nId, i );

		const UNIT_DATA * pUnitData = pWeapons[ i ] ? UnitGetData( pWeapons[ i ] ) : NULL;
		if ( pWeapons[ i ] && !( AppIsTugboat() && UnitIsA( pWeapons[i], UNITTYPE_SHIELD ) ) && bUseWeaponModel && (!pUnitData || !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_NO_WEAPON_MODEL)) ) // focus items don't use weapon models
		{
			int nWeaponModel = c_UnitGetModelId( pWeapons[ i ] );

			if ( nWeaponAttachmentOld != INVALID_ID )
			{
				ATTACHMENT * pAttachmentOld = c_ModelAttachmentGet( nModelId, nWeaponAttachmentOld );
				if ( pAttachmentOld && pAttachmentOld->nAttachedId == nWeaponModel )
					pbAddAttachment[ i ] = bRecreateWeaponAttachments;
				else if ( pAttachmentOld )
					pbRemoveOldAttachment[ i ] = TRUE; 
			}

		} else {
			pbAddAttachment[ i ] = pWeapons[ i ] && bRecreateWeaponAttachments && !( AppIsTugboat() && UnitIsA( pWeapons[i], UNITTYPE_SHIELD ) ) && (!pUnitData || !UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_NO_WEAPON_MODEL));
			if ( nWeaponAttachmentOld != INVALID_ID )
				pbRemoveOldAttachment[ i ] = TRUE;
		}
	}

	// remove first and add later - in case there is some overlap in the weapons
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( ! pbRemoveOldAttachment[ i ] )
			continue;
		 // remove the old weapon - but just stop drawing it since we will need it later
		int nWeaponAttachmentOld = pWardrobe->pClient->pnWeaponAttachment[ i ];
		pWardrobe->pClient->pnWeaponAttachment[ i ] = INVALID_ID;

		ATTACHMENT * pAttachmentOld = c_ModelAttachmentGet( nModelId, nWeaponAttachmentOld );
		if ( pAttachmentOld )
		{
			ASSERT( pAttachmentOld->eType == ATTACHMENT_MODEL );
			if ( pAttachmentOld->eType == ATTACHMENT_MODEL )
			{
				if ( bUseWeaponModel )
				{
					e_ModelSetDrawAndShadow( pAttachmentOld->nAttachedId, FALSE );
					c_ModelAttachmentRemoveAllByOwner( pAttachmentOld->nAttachedId, ATTACHMENT_OWNER_ANIMATION, INVALID_ID );
					c_ModelAttachmentRemoveAllByOwner( pAttachmentOld->nAttachedId, ATTACHMENT_OWNER_SKILL, INVALID_ID );
					c_AttachmentSetDraw( *pAttachmentOld, FALSE );

					// just in case the owner had set some material overrides on the weapon
					e_ModelDeactivateAllClones( pAttachmentOld->nAttachedId );
					V( e_ModelRemoveAllMaterialOverrides( pAttachmentOld->nAttachedId ) );
				}
				int nAttachedModel = pAttachmentOld->nAttachedId;
				c_ModelAttachmentRemove( nModelId, nWeaponAttachmentOld, ATTACHMENT_FUNC_FLAG_FLOAT );
				pAttachmentOld = NULL; // we just freed it.  Let's be explicit about it.
				int nModelRoomID = (int)e_ModelGetRoomID( nAttachedModel );
				if ( nModelRoomID != INVALID_ID )
				{
					ROOM * pOldRoom = ( nModelRoomID != INVALID_ID )	? RoomGetByID( AppGetCltGame(), nModelRoomID )		: NULL;
					c_RoomRemoveModel( pOldRoom, nAttachedModel );
					e_ModelSetRoomID( nAttachedModel, INVALID_ID );
				}
			}
		}
	}

	BOOL bResetDrawState = TRUE;
	if(!AppIsHammer() && UnitGetStat(pUnit, STATS_DONT_DRAW_WEAPONS) != 0)
	{
		bResetDrawState = FALSE;
	}

	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( ! pbAddAttachment[ i ] )
			continue;

		ASSERT_CONTINUE( pWeapons[ i ] );

		const UNIT_DATA * pWeaponUnitData = UnitGetData( pWeapons[ i ] );
		ASSERT_CONTINUE( pWeaponUnitData );

		int nWeaponModel = c_UnitGetModelId( pWeapons[ i ] );
		if(bResetDrawState)
		{
			e_ModelSetDrawAndShadow( nWeaponModel, TRUE );
			UnitSetStat( pWeapons[ i ], STATS_NO_DRAW, 0 );
		}

		ATTACHMENT_DEFINITION tAttachmentDef;
		tAttachmentDef.eType	 = ATTACHMENT_MODEL;
		tAttachmentDef.vPosition = VECTOR( 0 );
		tAttachmentDef.dwFlags  |= 1 << (ATTACHMENT_SHIFT_WEAPON_INDEX + i);
		tAttachmentDef.dwFlags  |= ATTACHMENT_FLAGS_ABSOLUTE_NORMAL;
		if ( i == WEAPON_INDEX_LEFT_HAND && UnitDataTestFlag( pWeaponUnitData, UNIT_DATA_FLAG_MIRROR_IN_LEFT_HAND ) )
		{
			tAttachmentDef.dwFlags |= ATTACHMENT_FLAGS_MIRROR;
		}

		const char * pszBoneName = c_WeaponGetAttachmentBoneName( c_AppearanceGetDefinition( nModelId ), i, 
			MAX( 0, pWeaponUnitData->nWeaponBoneIndex ), tAttachmentDef.vNormal, tAttachmentDef.fRotation );
		if ( pszBoneName )
			PStrCopy( tAttachmentDef.pszBone, pszBoneName, MAX_XML_STRING_LENGTH );
		//trace("c_WardrobeUpdateAttachments added model %d to %s\n", nWeaponModel, pszBoneName );

		pWardrobe->pClient->pnWeaponAttachment[ i ] = c_ModelAttachmentAdd( 
			nModelId, tAttachmentDef, FILE_PATH_WEAPONS, FALSE, 
			ATTACHMENT_OWNER_WARDROBE, INVALID_ID, nWeaponModel );

		// we can't call sWeaponResetAttachments because it would remove any skill attachments that might currently be used
		if ( nWeaponModel != INVALID_ID )
			c_AppearanceForceInitAnimation( nWeaponModel );
		c_UnitSetMode( pWeapons[ i ], MODE_ITEM_EQUIPPED_IDLE );
	}

	BOOL bModelIsDrawing = ! e_ModelGetFlagbit( nModelId, MODEL_FLAGBIT_NODRAW );
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{// make sure that all weapons are visible - there are some order issues if a weapon switched hands
		ATTACHMENT * pAttachment = c_ModelAttachmentGet( nModelId, pWardrobe->pClient->pnWeaponAttachment[ i ] );
		if ( ! pAttachment )
			continue;
		c_AttachmentSetDraw( *pAttachment, bModelIsDrawing );
	}

	//if ( pUnit )
	//{
	//	BOOL bAnyWeaponsWereAdded = FALSE;
	//	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	//	{
	//		if ( pbAddAttachment[ i ] )
	//			bAnyWeaponsWereAdded = TRUE;
	//	}
	//	if ( bAnyWeaponsWereAdded )
	//		c_StateDoAllEventsOnWeapons( pUnit, pbAddAttachment );
	//}
}


//----------------------------------------------------------------------------
// this function runs in one of two modes:
// bJustCheckLoaded == TRUE  - request that models be loaded and see if all of the models are loaded
// bJustCheckLoaded == FALSE - request that models be loaded and mix together the parts from the different models
//----------------------------------------------------------------------------
static BOOL sCostumeAddModelPartsOrCheckWhetherLoaded ( 
	GAME * pGame, 
	WARDROBE* pWardrobe,
	COSTUME& tCostume,
	BOOL bJustCheckLoaded )
{
	if ( pWardrobe->dwFlags & WARDROBE_FLAG_SHARES_MODEL_DEF )
		return TRUE;// no work to do... it's done

	WARDROBE_LAYER_STACK& tLayerStack = tCostume.tModelLayerStack;
	if ( tLayerStack.Count() == 0 )
		return TRUE;
	
	int nBaseStackId = tLayerStack.GetFirst();
	WARDROBE_LAYER_STACK_ELEMENT* pBaseElement = tLayerStack.Get( nBaseStackId );
	int nBaseLayerId = sWardrobeGetLayer( pGame, pWardrobe, &tCostume, 
		pBaseElement->nLayerId, INVALID_ID, FALSE );
	if ( nBaseLayerId == INVALID_ID )
		return TRUE;

	WARDROBE_LAYER_DATA* pBaseLayer = (WARDROBE_LAYER_DATA *) 
		ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, nBaseLayerId );
	ASSERT_RETTRUE( pBaseLayer );

	BLENDOP_GROUP eBlendOpGroup = sWardrobeGetBlendOpGroup( pWardrobe );

	BOOL bEverythingIsLoaded = TRUE;

	// find out what parts we need
	WARDROBE_MODEL* pWardrobeModelBase = c_sWardrobeGetModelForLayer ( pGame, pWardrobe, pBaseLayer );
	ASSERT_RETTRUE( pWardrobeModelBase );
	if ( !sWardrobeModelIsLoaded( pWardrobeModelBase, tCostume.nLOD ) )
	{
		if ( sWardrobeModelLoadFailed( pWardrobeModelBase, tCostume.nLOD ) )
		{
			if ( bJustCheckLoaded )
			{
				char* pszFilename = pWardrobeModelBase->pszFileName;
				char szLowFileName[ DEFAULT_FILE_WITH_PATH_SIZE ];
				if ( tCostume.nLOD == LOD_LOW )
				{
					e_ModelDefinitionGetLODFileName(szLowFileName, 
						ARRAYSIZE(szLowFileName), pWardrobeModelBase->pszFileName );

					pszFilename = szLowFileName;
				}

				ErrorDoAssert( "Missing critical file '%s'", 
					pszFilename );
			}

			return TRUE;
		}
		else
		{
			V( c_sWardrobeModelLoad( pGame, pWardrobe->pClient->nAppearanceDefId, pWardrobeModelBase, tCostume.nLOD ) );

			if ( c_GetToolMode() && sWardrobeModelIsLoaded( pWardrobeModelBase, tCostume.nLOD ) )
			{
				// We sync load the wardrobe model, so let's check to see if the
				// model was successfully loaded.
			}
			else
				bEverythingIsLoaded = FALSE;
		}
	}
	else if ( bJustCheckLoaded )
	{
		if ( NULL == DefinitionGetById( DEFINITION_GROUP_MATERIAL, pWardrobeModelBase->nMaterial ) )
			bEverythingIsLoaded = FALSE;
	}

	int nModelDefinitionId = tCostume.nModelDefinitionId;
	
#define	MAX_WARDROBE_PARTS 80
	PART_CHART pPartChart[ MAX_WARDROBE_PARTS ];

	// Add and remove parts from the layers
	int nPartCount = 0;	
	for( int nStackId = nBaseStackId; 
		 nStackId != INVALID_ID; 
		 nStackId = tLayerStack.GetNextId( nStackId ) )
	{
		WARDROBE_LAYER_STACK_ELEMENT* pElement = tLayerStack.Get( nStackId );
		int nLayerId = pElement->nLayerId;

		if ( nLayerId == INVALID_ID )
			break;

		WARDROBE_LAYER_DATA * pLayer = (WARDROBE_LAYER_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, nLayerId );
		ASSERT_CONTINUE( pLayer );

		if ( pLayer->pnWardrobeBlendOp[ eBlendOpGroup ] == INVALID_ID )
			continue;

		WARDROBE_BLENDOP_DATA * pBlendOp = (WARDROBE_BLENDOP_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_BLENDOP, pLayer->pnWardrobeBlendOp[ eBlendOpGroup ] );
		if ( ! pBlendOp )
			continue;

		// Add parts
		WARDROBE_MODEL * pWardrobeModel = c_sWardrobeGetModelForLayer( pGame, pWardrobe, pLayer );

		if ( ! pWardrobeModel )
			continue;

		if ( !sWardrobeModelIsLoaded( pWardrobeModel, tCostume.nLOD ) )
		{
			if ( sWardrobeModelLoadFailed( pWardrobeModel, tCostume.nLOD ) )
			{
				if ( bJustCheckLoaded && pWardrobeModel != pWardrobeModelBase  )
				{
					char* pszFilename = pWardrobeModel->pszFileName;
					char szLowFileName[ DEFAULT_FILE_WITH_PATH_SIZE ];
					if ( tCostume.nLOD == LOD_LOW )
					{
						e_ModelDefinitionGetLODFileName(szLowFileName, 
							ARRAYSIZE(szLowFileName), pWardrobeModel->pszFileName );

						pszFilename = szLowFileName;
					}

					ErrorDoAssert( "Missing critical file '%s'", pszFilename );
				}
			}
			else
			{
				V( c_sWardrobeModelLoad( pGame, pWardrobe->pClient->nAppearanceDefId, pWardrobeModel, tCostume.nLOD ) );

				if ( c_GetToolMode() && sWardrobeModelIsLoaded( pWardrobeModel, tCostume.nLOD ) )
				{
					// We sync load the wardrobe model, so let's check to see if the
					// model was successfully loaded.
				}
				else
					bEverythingIsLoaded = FALSE;
			}			
		} 
		else if ( NULL == DefinitionGetById( DEFINITION_GROUP_MATERIAL, pWardrobeModel->nMaterial ) )
		{
			bEverythingIsLoaded = FALSE;
		}

		if ( bJustCheckLoaded )
			continue;

		if ( pBlendOp->bReplaceAllParts )
			nPartCount = 0;
		
		// Remove parts
		for ( int j = 0; j < MAX_PARTS_PER_LAYER; j++ )
		{
			if ( pBlendOp->pnRemoveParts[ j ] == INVALID_ID )
				continue;

			for ( int ii = 0; ii < nPartCount; ii++ )
			{
				if ( pPartChart[ ii ].nPart == pBlendOp->pnRemoveParts[ j ] )
				{
					if ( ii < nPartCount - 1)
					{
						MemoryMove( pPartChart + ii, (MAX_WARDROBE_PARTS - ii) * sizeof(PART_CHART), pPartChart + ii + 1, (nPartCount - ii - 1) * sizeof(PART_CHART));
					}
					nPartCount--;
					ii--;
				}
			}
		}

		for ( int j = 0; j < MAX_PARTS_PER_LAYER; j++ )
		{
			if ( pBlendOp->pnAddParts[ j ] == INVALID_ID && !( j == 0 && pBlendOp->bReplaceAllParts ) )
				continue;

			int nWardrobeModelPartCount = 0;
			WARDROBE_MODEL_PART* pWardrobeModelParts = e_WardrobeModelGetParts( pWardrobeModel, nWardrobeModelPartCount );
			for ( int ii = 0; ii < nWardrobeModelPartCount; ii++ )
			{
				if ( pWardrobeModelParts[ ii ].nPartId == INVALID_ID )
					continue;
				if ( pWardrobeModelParts[ ii ].nPartId == pBlendOp->pnAddParts[ j ] ||
					 pBlendOp->bReplaceAllParts )
				{
					nPartCount++;
					int nPart = pWardrobeModelParts[ ii ].nPartId;
					ASSERT_RETTRUE( nPartCount < MAX_WARDROBE_PARTS );
					pPartChart[ nPartCount - 1 ].nModel = pWardrobeModel->nRow;
					pPartChart[ nPartCount - 1 ].nPart  = nPart;
					pPartChart[ nPartCount - 1 ].nIndexInModel = ii;
					pPartChart[ nPartCount - 1 ].nMaterial = pWardrobeModel->nMaterial;

					WARDROBE_MODEL_PART_DATA * pPartDef = (WARDROBE_MODEL_PART_DATA*)ExcelGetData( pGame, DATATABLE_WARDROBE_PART, nPart );
					ASSERT( pPartDef );
					pPartChart[ nPartCount - 1 ].nTextureIndex = pPartDef ? pPartDef->nTargetTextureIndex : 0;
				}
			}
		}
	}

	if ( bJustCheckLoaded )
		return bEverythingIsLoaded;

	// find out what models we need
	int pnWardrobeModelList[ MAX_WARDROBE_MODELS ];
	int nWardrobeModelCount = 0;
	for ( int i = 0; i < nPartCount; i++ )
	{
		int j = 0;
		for ( ; j < nWardrobeModelCount; j++ )
		{
			if ( pnWardrobeModelList[ j ] == pPartChart[ i ].nModel )
				break;
		}
		if ( j == nWardrobeModelCount )
		{
			ASSERT( nWardrobeModelCount < MAX_WARDROBE_MODELS );
			pnWardrobeModelList[ j ] = pPartChart[ i ].nModel;
			nWardrobeModelCount++;
		}
	}
	
	// find out what materials and texture indices we have
	int pnWardrobeMaterials		[ MAX_WARDROBE_MATERIAL_TEXTURE_PAIRS ];
	int pnWardrobeTextureIndex	[ MAX_WARDROBE_MATERIAL_TEXTURE_PAIRS ];
	int nWardrobeMaterialCount = 0;
	for ( int i = 0; i < nPartCount; i++ )
	{
		int j = 0;
		for ( ; j < nWardrobeMaterialCount; j++ )
		{
			if ( pnWardrobeMaterials	[ j ] == pPartChart[ i ].nMaterial &&
				 pnWardrobeTextureIndex	[ j ] == pPartChart[ i ].nTextureIndex )
				break;
		}
		if ( j == nWardrobeMaterialCount )
		{
			ASSERT( nWardrobeMaterialCount < MAX_WARDROBE_MATERIAL_TEXTURE_PAIRS );
			pnWardrobeMaterials		[ j ] = pPartChart[ i ].nMaterial;
			pnWardrobeTextureIndex	[ j ] = pPartChart[ i ].nTextureIndex;
			nWardrobeMaterialCount++;
		}
	}

	// clear the old meshes
	e_ModelDefinitionDestroy( nModelDefinitionId, tCostume.nLOD,
		MODEL_DEFINITION_DESTROY_MESHES | MODEL_DEFINITION_DESTROY_VBUFFERS | MODEL_DEFINITION_DESTROY_TEXTURES, TRUE );
	V( e_AnimatedModelDefinitionClear( nModelDefinitionId, tCostume.nLOD ) );
	
	V( e_WardrobeAddLayerModelMeshes( 
		nModelDefinitionId,
		tCostume.nLOD,
		tCostume.ppnTextures,
		pnWardrobeTextureIndex,
		pnWardrobeMaterials,
		nWardrobeMaterialCount,
		pnWardrobeModelList,
		nWardrobeModelCount,
		pWardrobeModelBase,
		pPartChart,
		nPartCount,
		(pWardrobe->dwFlags & WARDROBE_FLAG_USES_COLORMASK) != 0 ) );

	WARDROBE_TRACE("Wardrobe model updated %s\n", pBaseLayer->pszName );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sWardrobeUpdateCostumeModel ( 
	GAME * pGame, 
	WARDROBE * pWardrobe,
	COSTUME & tCostume,
	int & nUpdateCount )
{
	// have all of the models been loaded and added?
	if (   tCostume.bModelsAdded 
		|| TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_LOD 
				| WARDROBE_FLAG_DOWNGRADE_LOD ) )
		return TRUE;

	if ( sCostumeAddModelPartsOrCheckWhetherLoaded( pGame, pWardrobe, tCostume, TRUE) &&
		TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_MODEL_DEF_INITIALIZED 
			| WARDROBE_FLAG_UPGRADE_LOD | WARDROBE_FLAG_DOWNGRADE_LOD ) )
	{
		nUpdateCount++;
		WARDROBE_TRACE( "Model updated\n" );
		if ( !TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_LOD | WARDROBE_FLAG_DOWNGRADE_LOD ) )
			sCostumeAddModelPartsOrCheckWhetherLoaded( pGame, pWardrobe, tCostume, FALSE );
		tCostume.bModelsAdded = TRUE;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL c_sWardrobeUpdateCostumeTexture (
	GAME * pGame,
	WARDROBE * pWardrobe,
	COSTUME & tCostume,
	int & nUpdateCount,
	int nTextureIndex,
	LAYER_TEXTURE_TYPE eLayerTextureType )
{
#if !ISVERSION(SERVER_VERSION)	
	TEXTURE_TYPE eTexture = (TEXTURE_TYPE) e_WardrobeGetLayerTextureType( eLayerTextureType );
	if ( ! e_TextureSlotIsUsed( eTexture ) )
		tCostume.dwTextureFlags[ nTextureIndex ] |= MAKE_MASK( eLayerTextureType );

	if ( tCostume.dwTextureFlags[ nTextureIndex ] & MAKE_MASK( eLayerTextureType ))
		return !e_TextureIsFiltering( tCostume.ppnTextures[ nTextureIndex ][ eLayerTextureType ] );

	if ( sAllLayerTexturesLoaded( pGame, pWardrobe, tCostume, nTextureIndex, eLayerTextureType )
		&& tCostume.bModelsAdded )
	{
		//TIMER_START_NEEDED_EX("AddLayerTextures", 12);
		if ( sAddLayerTextures( pGame, pWardrobe, tCostume, nTextureIndex, eLayerTextureType, nUpdateCount ) )
		{
			tCostume.dwTextureFlags[ nTextureIndex ] |= MAKE_MASK( eLayerTextureType );
			return !e_TextureIsFiltering( tCostume.ppnTextures[ nTextureIndex ][ eLayerTextureType ] );
		}
	}
#endif //!SERVER_VERSION

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ColorLerp(
	DWORD dwFirst,
	DWORD dwSecond,
	BYTE bLerp )
{
	static const DWORD Masks[ 3 ] = 
	{
		0x00ff0000,
		0x0000ff00,
		0x000000ff
	};

	DWORD dwResult = 0;
	for ( int i = 0; i < 3; i++ )
	{
		DWORD dwChannel = ((dwFirst & Masks[ i ]) * bLerp);
		dwChannel += ((dwSecond & Masks[ i ]) * (255 - bLerp));
		dwChannel /= 255;
		dwChannel &= Masks[ i ];
		dwResult |= dwChannel;
	}
	return dwResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int c_sLayerStackCompare(WARDROBE_LAYER_STACK_ELEMENT* pElement, void* pKey)
{
	int nOrder = *((int*)pKey);

	if ( nOrder >= pElement->nOrder )
		return -1;
	else
		return 1;
}

static void c_sWardrobeCreateLayerStack(
	GAME* pGame,
	WARDROBE* pWardrobe,
	COSTUME& tCostume,
	int nTextureGroup )
{
	WARDROBE_LAYER_STACK& tLayerStack = tCostume.tLayerStack[ nTextureGroup ];

	int nLayerBase = sWardrobeGetLayer( pGame, pWardrobe, &tCostume, 
		tCostume.tLayerBase.nId, nTextureGroup, FALSE );

	USER_CODE
	( AE, 
		static int nTotalLayersRemoved = 0;
		static int nMostLayersRemoved = 0;
	)

	if ( nLayerBase != INVALID_ID )
	{
		// Fill the stack with all the costume's layers.
		for ( int i = -1; i < (int)tCostume.nLayerCount; i++ )
		{
			int nLayerId = INVALID_ID;

			if ( i < 0 )
				nLayerId = nLayerBase;
			else
				nLayerId = tCostume.ptLayers[ i ].nId;

			BOOL bProcessSubsequentLayers = FALSE; // Start with the main layer first.
			while ( nLayerId != INVALID_ID )
			{
				nLayerId = sWardrobeGetLayer( pGame, pWardrobe, &tCostume, nLayerId, 
					nTextureGroup, bProcessSubsequentLayers );			

				if ( nLayerId == INVALID_ID )
					break;

				WARDROBE_LAYER_DATA* pLayer = (WARDROBE_LAYER_DATA*)ExcelGetData( 
					pGame, DATATABLE_WARDROBE_LAYER, nLayerId );
				ASSERT_CONTINUE( pLayer );

				BLENDOP_GROUP eBlendOpGroup = sWardrobeGetBlendOpGroup( pWardrobe );
				ASSERT( pLayer->pnWardrobeBlendOp[ eBlendOpGroup ] != INVALID_ID );
				
				WARDROBE_BLENDOP_DATA* pBlendOp = (WARDROBE_BLENDOP_DATA*)ExcelGetData( 
					pGame, DATATABLE_WARDROBE_BLENDOP, pLayer->pnWardrobeBlendOp[ eBlendOpGroup ] );
				ASSERT_CONTINUE( pBlendOp );

				int nNewId = -1;
				WARDROBE_LAYER_STACK_ELEMENT* pNewElement = 
					tLayerStack.InsertSorted( &nNewId, &pLayer->nOrder, 
						c_sLayerStackCompare );

				pNewElement->nLayerId = nLayerId;
				pNewElement->nOrder = pLayer->nOrder;
				pNewElement->pBlendOp = pBlendOp;

				// Now that the starting layer has been processed, continue with
				// processing subsequent layers 
				bProcessSubsequentLayers = TRUE;
			}
		}
	} 
	USER_CODE
	( AE, 
		int nLayerCountBefore = tLayerStack.Count();
	)

	// Collapse the stack.
	if( tLayerStack.Count() > 0 )
	{		
		for( int nStackId = tLayerStack.GetFirst(); 
			nStackId != INVALID_ID; 
			nStackId = tLayerStack.GetNextId( nStackId ) )
		{
			WARDROBE_LAYER_STACK_ELEMENT* pElement = tLayerStack.Get( nStackId );

			BOOL bRemoveAllPrevious = FALSE;
			if ( pElement->pBlendOp->pszBlendTexture[0] == 0 &&
				!pElement->pBlendOp->bNoTextureChange )
				// No blend texture, which means that it covers all.
				bRemoveAllPrevious = TRUE;

			int nPreviousStackId = nStackId; // prime the iteration
			do
			{
				nPreviousStackId = tLayerStack.GetPrevId( nPreviousStackId );
				if ( nPreviousStackId != INVALID_ID )
				{
					WARDROBE_LAYER_STACK_ELEMENT* pPrevElement = 
						tLayerStack.Get( nPreviousStackId );

					BOOL bRemove = bRemoveAllPrevious;

					if ( !bRemove )
					{
						// If a previous element in the stack carries the same blend 
						// op as the current element or is considered covered by the current
						// element's blend op, then remove them from the stack.
						if ( pPrevElement->pBlendOp == pElement->pBlendOp )
						{
							bRemove = TRUE;
						}
						else
						{
							for( int i = 0; i < MAX_BLENDOP_COVERAGE; i++ )
							{
								int nBlendOpId = pElement->pBlendOp->pnCovers[i];
								
								if ( nBlendOpId == INVALID_ID )
									break;

								WARDROBE_BLENDOP_DATA* pCoveredBlendOp = 
									(WARDROBE_BLENDOP_DATA*)ExcelGetData( pGame,
									DATATABLE_WARDROBE_BLENDOP, nBlendOpId);

								if ( pCoveredBlendOp == pPrevElement->pBlendOp )
								{
									bRemove = TRUE;
									break;
								}
							}
						}
					}

					if ( bRemove )
					{
						int nRemoveId = nPreviousStackId;
						nPreviousStackId = nStackId;
						tLayerStack.Remove( nRemoveId );

						USER_CODE
						( AE,
							++nTotalLayersRemoved;
						)
						continue;
					}
				}
			} while ( nPreviousStackId != INVALID_ID 
				 && ( nPreviousStackId != tLayerStack.GetFirst() ) );
		}
	}

	tCostume.dwFlags[ nTextureGroup ] |= COSTUME_FLAG_LAYER_STACK_CREATED;

	USER_CODE
	( AE,
		int nLayerCountAfter = tLayerStack.Count();
		int nLayersRemoved = nLayerCountBefore - nLayerCountAfter;

		trace( "LAYERS REMOVED FOR MODEL %d: %d\n", 
			tCostume.nModelDefinitionId, nLayersRemoved );
		trace( "TOTAL LAYERS REMOVED: %d\n", nTotalLayersRemoved );

		if ( nLayersRemoved > nMostLayersRemoved )
		{
			nMostLayersRemoved = nLayersRemoved;
			trace( "MOST LAYERS REMOVED: %d\n", nMostLayersRemoved );
		} 
	)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_WardrobeGetBodyColor (
	int nBodyColorIndex,
	int nUnittype,
	BYTE bColorIndex,
	DWORD & dwColor )
{
	int nFirstColorsetIndex = INVALID_ID;
	switch( nBodyColorIndex )
	{
	case WARDROBE_BODY_COLOR_HAIR:
		nFirstColorsetIndex = GlobalIndexGet( GI_HAIR_COLOR_FIRST );
		break;
	case WARDROBE_BODY_COLOR_SKIN:
		nFirstColorsetIndex = GlobalIndexGet( GI_SKIN_COLOR_FIRST );
		break;
	}
	if ( nFirstColorsetIndex == INVALID_ID )
		return FALSE;

	if ( bColorIndex >= NUM_WARDROBE_BODY_COLOR_CHOICES ) 
	{ 
		bColorIndex = NUM_WARDROBE_BODY_COLOR_CHOICES - 1; 
	}

	int nColorSetIndex = nFirstColorsetIndex + (bColorIndex / COLOR_SET_COLOR_COUNT);
	// I'm going to change many of these to asserts once I can change the data
	const COLOR_DEFINITION * pColorSet = ColorSetGetColorDefinition( nColorSetIndex, nUnittype );
	if ( ! pColorSet ) 
	{ 
		pColorSet = ColorSetGetColorDefinition( nColorSetIndex, UNITTYPE_ANY ); 
	}
	if ( ! pColorSet ) 
	{ 
		pColorSet = ColorSetGetColorDefinition( nFirstColorsetIndex, UNITTYPE_ANY ); 
	}
	ASSERT_RETFALSE( pColorSet );

	int nIndexInColorSet = bColorIndex % COLOR_SET_COLOR_COUNT;
	dwColor = pColorSet->pdwColors[ nIndexInColorSet ];
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL c_sWardrobeUpdateCostume (
	GAME * pGame,
	WARDROBE* pWardrobe,
	COSTUME& tCostume,
	int& nUpdateCount )
{
	ASSERT_RETFALSE( pWardrobe->dwFlags & WARDROBE_FLAG_APPEARANCE_DEF_APPLIED );
	ASSERT_RETFALSE( pWardrobe->pClient );

	// Don't bother updating textures / models if the lod will be upgraded / downgraded.
	if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_LOD | WARDROBE_FLAG_DOWNGRADE_LOD ) )
		return TRUE;

	if ( ! TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_MODEL_DEF_INITIALIZED ) )
		return FALSE;

	// Set the costume lod to the current lod setting.
	tCostume.nLOD = pWardrobe->pClient->nLOD;

	BOOL bCheapMode = FALSE;
#ifdef _DEBUG
	//GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if ( pOverrides && pOverrides->dwFlags & GFX_FLAG_LOD_EFFECTS_ONLY )
		bCheapMode = TRUE;
#endif
	
	BOOL bIsDone = TRUE;
	if ( tCostume.tLayerBase.nId != INVALID_ID )
	{
		WARDROBE_LAYER_STACK& tModelLayerStack = tCostume.tModelLayerStack;

		for ( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
		{
			if( !( tCostume.dwFlags[ i ] & COSTUME_FLAG_LAYER_STACK_CREATED )
				&& tCostume.tLayerStack[ i ].IsEmpty() )
			{
				c_sWardrobeCreateLayerStack( pGame, pWardrobe, tCostume, i );

				// The model layer stack is a combined stack of the head and body
				// layer stacks.
				const WARDROBE_LAYER_STACK* ptLayerStack = 
					&tCostume.tLayerStack[ i ];

				for( int nStackId = ptLayerStack->GetFirst(); 
					nStackId != INVALID_ID; 
					nStackId = ptLayerStack->GetNextId( nStackId ) )
				{
					int nNewId = -1;

					WARDROBE_LAYER_STACK_ELEMENT* pNewElement = 
						tModelLayerStack.Add( &nNewId, TRUE );
					*pNewElement = *ptLayerStack->Get(nStackId);
				}
			}

			for ( int j = 0; j < NUM_TEXTURES_PER_SET; j++ )
			{
				if ( bCheapMode && ( j == LAYER_TEXTURE_NORMAL || j == LAYER_TEXTURE_SPECULAR ) )
					continue;
					
#ifdef WARDROBE_TIMERS
				TIMER_START_NEEDED_EX("UpdateCostumeTexture", NUM_MS_PER_WARDROBE_UPDATE);
#endif
				
				if ( !c_sWardrobeUpdateCostumeTexture ( pGame, pWardrobe, tCostume, 
						nUpdateCount, i, (LAYER_TEXTURE_TYPE)j ) )
					bIsDone = FALSE;

				if ( ! c_GetToolMode() )
				{
					UINT64 nDeltaTime = TIMER_GET_TIME();
					if ( nDeltaTime > NUM_MS_PER_WARDROBE_UPDATE )
					{
						//WARDROBE_TRACE( "%6d UpdateTextureInd in %4d ms (group: %d tex: %d)\n", (int)GameGetTick(pGame), (int)nDeltaTime, i, j );
						return FALSE; // We did too much work this frame, so continue on another frame.
					}
				}
			}
		}
	}

	{
		TIMER_START_NEEDED_EX("UpdateCostumeModel", NUM_MS_PER_WARDROBE_UPDATE);
		if ( ! sWardrobeUpdateCostumeModel( pGame, pWardrobe, tCostume, nUpdateCount ) )
		{
			bIsDone = FALSE;
		}
		UINT64 nDeltaTime = TIMER_GET_TIME();
		if ( nDeltaTime > NUM_MS_PER_WARDROBE_UPDATE )
		{
			//WARDROBE_TRACE( "%6d UpdateCostumeModel in %4d ms\n", (int)GameGetTick(pGame), (int)nDeltaTime );
			return FALSE; // We did too much work this frame, so continue on another frame.
		}
	}

	if ( pWardrobe->dwFlags & WARDROBE_FLAG_USES_COLORMASK )
	{
		for ( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
		{
			DWORD dwDesiredUpdateFlag = MAKE_MASK( LAYER_TEXTURE_DIFFUSE )
									  | MAKE_MASK( LAYER_TEXTURE_COLORMASK );

			if ( tCostume.dwTextureFlags[ i ] & COSTUME_TEXTURE_FLAG_COLOR_MASK_APPLIED )
			{
				if ( e_TextureIsFiltering( tCostume.ppnTextures[ i ][ LAYER_TEXTURE_DIFFUSE ] ) ||
					e_TextureIsColorCombining( tCostume.ppnTextures[ i ][ LAYER_TEXTURE_DIFFUSE ] ) )
					bIsDone = FALSE;
				continue;
			}

			if ( ( tCostume.dwTextureFlags[ i ] & dwDesiredUpdateFlag ) == dwDesiredUpdateFlag )
			{ 
				// we have diffuse and colormask ready
				int nUnitType = pWardrobe->pClient->nUnittype;
				const COLOR_DEFINITION * pColorSetDef = ColorSetGetColorDefinition( tCostume.nColorSetIndex, nUnitType );
				ASSERT( pColorSetDef );

				DWORD pdwColors[ WARDROBE_COLORMASK_TOTAL_COLORS ];

				// we have one extra color that was used for eyes.  It is saved for future use, but for now we will just make it white
				ASSERT( COLOR_SET_COLOR_COUNT + NUM_WARDROBE_BODY_COLORS_USED + 1 == WARDROBE_COLORMASK_TOTAL_COLORS );
				pdwColors[ WARDROBE_COLORMASK_TOTAL_COLORS - 1 ] = 0xffffffff;

				MemoryCopy( pdwColors, sizeof( pdwColors ), 
					pColorSetDef->pdwColors, 
					sizeof(DWORD) * COLOR_SET_COLOR_COUNT );

				for ( int k = 0; k < NUM_WARDROBE_BODY_COLORS_USED; k++ )
				{
					BOOL bRet = c_WardrobeGetBodyColor ( k, nUnitType, pWardrobe->tBody.pbBodyColors[ k ], pdwColors[ COLOR_SET_COLOR_COUNT + k ] );
					ASSERTX( bRet, "Expected Wardrobe Body Color" );
					if ( ! bRet )
					{
						pdwColors[ COLOR_SET_COLOR_COUNT + k ] = 0xffffffff;
					}
				}

				if ( pWardrobe->dwFlags & WARDROBE_FLAG_PRIMARY_COLOR_REPLACES_SKIN )
				{
					pdwColors[ COLOR_SET_COLOR_COUNT + WARDROBE_BODY_COLOR_SKIN ] = pdwColors[ 0 ];
				}

				V( e_WardrobeCombineDiffuseAndColorMask( 
					tCostume.ppnTextures[ i ][ LAYER_TEXTURE_DIFFUSE ],
					tCostume.ppnTextures[ i ][ LAYER_TEXTURE_COLORMASK ],
					tCostume.ppnTextures[ i ][ LAYER_TEXTURE_DIFFUSE ],
					&pdwColors[0], 2, 
					( pWardrobe->dwFlags & WARDROBE_FLAG_DONT_APPLY_BODY_COLORS ),
					( pWardrobe->dwFlags & WARDROBE_FLAG_NO_THREADED_PROCESSING ) ) );

				bIsDone = FALSE; // the above function calls async updates for the textures - it will not be done yet

				tCostume.dwTextureFlags[ i ] |= COSTUME_TEXTURE_FLAG_COLOR_MASK_APPLIED;
			}
		}
	} // pWardrobe->dwFlags & WARDROBE_FLAG_USES_COLORMASK

	if ( bIsDone )
	{
		// Now that the costume has been updated, we can preload textures 
		for ( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
		{
			for ( int j = 0; j < NUM_TEXTURES_PER_SET; j++ )
			{
				if ( sWardrobeModelDefUsesTexture( pWardrobe, (LAYER_TEXTURE_TYPE)j ) )
				{
					V( e_WardrobeCanvasComplete( tCostume.ppnTextures[ i ][ j ], TRUE ) );
					e_WardrobePreloadTexture( tCostume.ppnTextures[ i ][ j ] );
				}
				else
				{
					e_TextureReleaseRef( tCostume.ppnTextures[ i ][ j ], TRUE );
					tCostume.ppnTextures[ i ][ j ] = INVALID_ID;
					CLEAR_MASK( tCostume.dwTextureFlags[ i ], MAKE_MASK( j ) );
				}
			}
		}

		// and update the meshes technique masks
		V( e_ModelDefinitionUpdateMeshTechniqueMasks( tCostume.nModelDefinitionId, tCostume.nLOD ) );

		// and the meshes material versions
		V( e_ModelDefinitionUpdateMaterialVersions( tCostume.nModelDefinitionId, tCostume.nLOD ) );
	}

	return bIsDone;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL c_sWardrobeUpdateAllCostumes (
	GAME * pGame,
	WARDROBE * pWardrobe,
	int & nUpdateCount )
{
#if !ISVERSION(SERVER_VERSION)
	COSTUME& tCostume = sGetCostumeToUpdate( pWardrobe );
	if ( c_sWardrobeUpdateCostume( pGame, pWardrobe, tCostume, nUpdateCount ) )
	{
		if ( pWardrobe->pClient->nCostumeToUpdate != pWardrobe->pClient->nCostumeToDraw )
		{
			WARDROBE_TRACE("Wardrobe Costume swapping\n");
			// copy layer data from update to draw so that they match
			{
				COSTUME & tCostumeToUpdate = sGetCostumeToUpdate( pWardrobe );
				COSTUME & tCostumeToDraw   = sGetCostumeToDraw  ( pWardrobe );

				tCostumeToDraw.nLayerCount = tCostumeToUpdate.nLayerCount;
				sCostumeUpdateLayerArray( pGame, tCostumeToDraw );
				if ( tCostumeToUpdate.nLayerCount )
				{
					int nSize = sizeof(WARDROBE_LAYER_INSTANCE) * tCostumeToUpdate.nLayerCount;
					int nDestSize = sizeof(WARDROBE_LAYER_INSTANCE) * tCostumeToDraw.nLayerCount;
					MemoryCopy( tCostumeToDraw.ptLayers, nDestSize, 
						tCostumeToUpdate.ptLayers, nSize );
				}

				tCostumeToDraw.nColorSetIndex = tCostumeToUpdate.nColorSetIndex;
			}

			// swap the costumes
			int nTemp = pWardrobe->pClient->nCostumeToDraw;
			pWardrobe->pClient->nCostumeToDraw = pWardrobe->pClient->nCostumeToUpdate;
			pWardrobe->pClient->nCostumeToUpdate = nTemp;

			// switch model defs so that models use the newly updated costume
			{
				COSTUME & tCostumeToUpdate = sGetCostumeToUpdate( pWardrobe );
				COSTUME & tCostumeToDraw   = sGetCostumeToDraw  ( pWardrobe );

				e_AnimatedModelDefinitionsSwap( tCostumeToUpdate.nModelDefinitionId, tCostumeToDraw.nModelDefinitionId );

				int nTemp = tCostumeToUpdate.nModelDefinitionId;
				tCostumeToUpdate.nModelDefinitionId = tCostumeToDraw.nModelDefinitionId;
				tCostumeToDraw.nModelDefinitionId = nTemp;

				// Update models with new definition.
				e_ModelUpdateByDefinition( tCostumeToDraw.nModelDefinitionId );
			}				

			// clean up old costume so that we don't waste resources
			{
				COSTUME & tCostumeToUpdate = sGetCostumeToUpdate( pWardrobe );
				e_ModelDefinitionDestroy( tCostumeToUpdate.nModelDefinitionId, 
					tCostumeToUpdate.nLOD, MODEL_DEFINITION_DESTROY_MESHES | MODEL_DEFINITION_DESTROY_VBUFFERS | MODEL_DEFINITION_DESTROY_TEXTURES, TRUE );
				V( e_AnimatedModelDefinitionClear( tCostumeToUpdate.nModelDefinitionId, 
					tCostumeToUpdate.nLOD ) );
				
				// we might want to clean up textures here...
			}
		} 
		else 
		{
			ASSERT( pWardrobe->pClient->nCostumeCount );
			if ( pWardrobe->pClient->nCostumeCount )
			{
				pWardrobe->pClient->nCostumeToUpdate = (pWardrobe->pClient->nCostumeToDraw + 1) % pWardrobe->pClient->nCostumeCount;
			}
		}

		// If we've switched between LODs, then remove the old LOD.
		if ( ! TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_SHARES_MODEL_DEF ) )
		{
			COSTUME& tCostume = sGetCostumeToDraw( pWardrobe );
			if ( tCostume.nLOD == LOD_HIGH )
			{
				if ( e_ModelDefinitionExists( tCostume.nModelDefinitionId, LOD_LOW ) )
				{
					e_ModelDefinitionDestroy( tCostume.nModelDefinitionId, LOD_LOW,
						MODEL_DEFINITION_DESTROY_ALL );
				}
			}
			else // LOD_LOW
			{
				if ( e_ModelDefinitionExists( tCostume.nModelDefinitionId, LOD_HIGH ) )
				{
					e_ModelDefinitionDestroy( tCostume.nModelDefinitionId, LOD_HIGH,
						MODEL_DEFINITION_DESTROY_ALL );
				}
			}
		}

		return TRUE;
	} 
	else 
	{
		return FALSE;
	}
#else
	return FALSE;
#endif //!SERVER_VERSION
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWardrobeReleaseSourceTextures()
{
#if !ISVERSION(SERVER_VERSION)
	for (unsigned int i = 0; i < ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_WARDROBE_TEXTURESET); i++)
	{
		WARDROBE_TEXTURESET_DATA * pData = (WARDROBE_TEXTURESET_DATA*) ExcelGetData( NULL, DATATABLE_WARDROBE_TEXTURESET, i );
		if ( !pData )
			continue;
		for ( int t = 0; t < NUM_TEXTURES_PER_SET; t++ )
		{
			for ( int u = 0; u < LOD_COUNT; u++ )
			{
				if ( pData->pnTextureIds[ t ][ u ] != INVALID_ID )
				{
					e_TextureReleaseRef( pData->pnTextureIds[ t ][ u ], TRUE );
					pData->pnTextureIds[ t ][ u ] = INVALID_ID;
				}
			}
		}
	}
#endif 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_WardrobeSystemInit()
{
#if !ISVERSION(SERVER_VERSION)
	ArrayInit(sgptWardrobesToUpdate, NULL, 8);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_WardrobeSystemDestroy()
{
#if !ISVERSION(SERVER_VERSION)
	sgptWardrobesToUpdate.Destroy();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_sWardrobeReportStats()
{
#if !ISVERSION(SERVER_VERSION)
	if ( ! e_GetRenderFlag( RENDER_FLAG_WARDROBE_INFO ) )
		return;

#if ISVERSION(DEVELOPMENT)
	const int cnBufLen = 2048;
	WCHAR wszBuf[ cnBufLen ];

	int nTemplateCount = 0;
	int nInstancedCount = 0;
	int nTotalCount = 0;
	int nCount = c_WardrobeGetCount( &nTemplateCount, &nInstancedCount, &nTotalCount );

	// Wardrobe counts
	PStrPrintf( wszBuf, cnBufLen, 
	L"%24s: %8d\n"
	L"%24s: %8d\n"
	L"%24s: %8d\n"
	L"%24s: %8d\n"
	,
	L"WARDROBE UNIQUE COUNT",		nCount,
	L"WARDROBE TEMPLATE COUNT",		nTemplateCount,
	L"WARDROBE INSTANCED COUNT",	nInstancedCount,
	L"TOTAL COUNT",					nTotalCount );

	VECTOR vPos = VECTOR( 0.95f, 0.0f );
	V( e_UIAddLabel( wszBuf, &vPos, FALSE, 0.5f, 0xffffffff, UIALIGN_BOTTOMRIGHT, UIALIGN_LEFT ) );
#endif //DEVELOPMENT
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_WardrobeUpdate(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	GAME * game = AppGetCltGame();

#ifdef WARDROBE_TIMERS
	TIMER_START_NEEDED_EX("Wardrobe Update", NUM_MS_PER_WARDROBE_UPDATE + 2);
#endif
	
	static int snWardrobesSinceCleanup = 0;
	static UINT64 snLastWardrobeToUpdate = 0;
	static UINT64 snLastWardrobeCostumeUpdate = 0;

	if ( sgbWardrobesUpdateDirty 
#ifdef UPDATE_WARDROBE_SEQUENTIALLY
		&& sgpCurrentWardrobe == NULL
		&& ( snWardrobesSinceCleanup >= NUM_WARDROBES_BETWEEN_CLEARS 
		  || sgptWardrobesToUpdate.Count() == 0 )
#else
		&& ( sgptWardrobesToUpdate.Count() == 0 )
		&& snWardrobesSinceCleanup >= NUM_WARDROBES_BETWEEN_CLEARS 
#endif
		)
	{
		// we need to releaseref the source textures in excel
		sWardrobeReleaseSourceTextures();
		sgbWardrobesUpdateDirty = ( sgptWardrobesToUpdate.Count() > 0 );
		snWardrobesSinceCleanup = 0;

		// while we're here, cleanup textures too
		V(e_TexturesCleanup(TEXTURE_GROUP_WARDROBE));

		EXCEL_TABLE * table = ExcelGetTableNotThreadSafe(DATATABLE_WARDROBE_MODEL);
		ASSERT(table);
		ExcelWardrobeModelFree(table);
	}

	int nUpdateCount = 0;
	for ( unsigned int nWardrobe = 0; nWardrobe < sgptWardrobesToUpdate.Count(); nWardrobe++ )
	{
#ifdef UPDATE_WARDROBE_SEQUENTIALLY
		if ( ! sgpCurrentWardrobe )
			sgpCurrentWardrobe = sgptWardrobesToUpdate[ nWardrobe ];

		WARDROBE* pWardrobe = sgpCurrentWardrobe;
#else
		WARDROBE* pWardrobe = sgptWardrobesToUpdate[ nWardrobe ];
#endif

		BOOL bAddBody = c_GetToolMode();
		BOOL bClearOldLayers = FALSE;
		if ( pWardrobe->dwFlags & WARDROBE_FLAG_FORCE_USE_BODY )
		{
			bAddBody = TRUE;
		}
		if ( pWardrobe->dwFlags & WARDROBE_FLAG_FORCE_USE_BODY_ONLY )
		{
			bClearOldLayers = TRUE;
		}
		BOOL bUpdate = c_sWardrobeApplyAppearanceDef( pWardrobe, FALSE, bClearOldLayers, bAddBody );

		if ( ( pWardrobe->dwFlags & WARDROBE_FLAG_REQUIRES_UPDATE_FROM_SERVER ) != 0 &&
			(pWardrobe->dwFlags & WARDROBE_FLAG_UPDATED_BY_SERVER) == 0 &&
			(pWardrobe->dwFlags & WARDROBE_FLAG_CLIENT_ONLY) == 0 &&
			! c_GetToolMode() ) // character screen needs to update and doesn't get messages
			bUpdate = FALSE;

		BOOL bModelDefInitialized = FALSE;
		BOOL bCostumesFinishedUpdating = FALSE;

		if ( bUpdate )
		{
			snLastWardrobeToUpdate = AppCommonGetCurTime();
			int nLastUpdateCount = nUpdateCount;

			bModelDefInitialized = c_sWardrobeInitModelDef(game, pWardrobe );
			{
				bCostumesFinishedUpdating = c_sWardrobeUpdateAllCostumes(game, pWardrobe, nUpdateCount );
			}
			
			if ( bModelDefInitialized && bCostumesFinishedUpdating )
			{
				V( c_sWardrobeRemoveFromUpdateList(game, pWardrobe ) );
				nWardrobe--;
				snWardrobesSinceCleanup++;

				// The order of these operations is important because a LOD change
				// also issues a downgrade costume count operation. You don't
				// want the downgrade to occur until AFTER the next update because
				// two costumes are needed while changing LODs. Otherwise, with a
				// single costume you would see the LOD change as it is updating.
				if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_COSTUME_COUNT ) )
				{
					V( c_WardrobeDowngradeCostumeCount( pWardrobe->nId ) );
				}

				if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_LOD ) )
				{
					V( c_WardrobeUpgradeLOD( pWardrobe->nId ) );
				}
				else if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_DOWNGRADE_LOD ) )
				{
					V( c_WardrobeDowngradeLOD( pWardrobe->nId ) );
				}

				if ( TEST_MASK( pWardrobe->dwFlags, WARDROBE_FLAG_UPGRADE_COSTUME_COUNT ) )
				{
					V( c_WardrobeUpgradeCostumeCount( pWardrobe->nId ) );
				}				
			}

			if ( nLastUpdateCount != nUpdateCount )
				snLastWardrobeCostumeUpdate = AppCommonGetCurTime();
		}
		else // ! bUpdate
			sgpCurrentWardrobe = NULL;

		// Track the last wardrobe ready-to-update and the last actual costume update to see which one may be stalling
		TIME tCurTime = AppCommonGetCurTime();
		TIMEDELTA tToUpdateDelta = tCurTime - snLastWardrobeToUpdate;
		TIMEDELTA tUpdatedDelta = tCurTime - snLastWardrobeCostumeUpdate;
		TIMEDELTA tAllowedDelta = (TIMEDELTA) ( 10.0f * MSECS_PER_SEC );
		if ( tToUpdateDelta > tAllowedDelta || tUpdatedDelta > tAllowedDelta )
		{
			trace( "Wardrobe update stalling!  Secs since: last wardrobe ready to update %3.2f, last costume update %3.2f.\n  bUpdate[%d] bModelDefInitialized[%d] bCostumesFinishedUpdating[%d]\n", (float)tToUpdateDelta / MSECS_PER_SEC, (float)tUpdatedDelta / MSECS_PER_SEC, bUpdate, bModelDefInitialized, bCostumesFinishedUpdating );
		}

		UINT64 nDeltaTime = TIMER_GET_TIME();
		if (   nDeltaTime > NUM_MS_PER_WARDROBE_UPDATE 
			|| snWardrobesSinceCleanup >= NUM_WARDROBES_BETWEEN_CLEARS )
		{
#ifdef WARDROBE_TIMERS		
			WARDROBE_TRACE("[TICK:%6d] Wardrobe updated in %4d ms with %4d updates\n", (int)GameGetTick(game), (int)nDeltaTime, nUpdateCount);
#endif			
//			if ( nUpdateCount > 0 )
			{
				break;
			}
		}
	}

	c_sWardrobeReportStats();
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeUpdateAttachments (
	UNIT * pUnit,
	int nWardrobe,
	int nModelId,
	BOOL bUseWeaponModel,
	BOOL bJustUpdateWeapon,
	BOOL bRecreateWeaponAttachments )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN ( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	if( ! pWardrobe )
		return;

	ASSERT_RETURN( nModelId != INVALID_ID );
	if ( ! bJustUpdateWeapon )
	{
		COSTUME & tCostume = sGetCostumeToUpdate( pWardrobe );
		sAddCostumeAttachments	( pGame, pWardrobe, tCostume, nModelId );
	}
	sAddWeaponAttachments	( pGame, pUnit, pWardrobe, nModelId, bUseWeaponModel, bRecreateWeaponAttachments );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeUpdateStates (
	UNIT * pUnit,
	int nModelId,
	int nWardrobe )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );
	ASSERT_RETURN( pWardrobe->pClient );
	ASSERT_RETURN( pUnit || nModelId != INVALID_ID );

	int pnStatesOld[ MAX_STATES_PER_WARDROBE ];
	int nStateCountOld = pWardrobe->pClient->nStateCount;
	if ( nStateCountOld )
		MemoryCopy( pnStatesOld, sizeof( pnStatesOld ), 
			pWardrobe->pClient->pnStates, sizeof(int) * nStateCountOld );

	pWardrobe->pClient->nStateCount = 0;

	COSTUME & tCostume = sGetCostumeToUpdate( pWardrobe );
	for ( unsigned int i = 0; i < tCostume.nLayerCount; i++ )
	{
		int nLayerCurr = tCostume.ptLayers[ i ].nId;
		BOOL bFirstLoop = TRUE;
		while ( nLayerCurr != INVALID_ID )
		{
			nLayerCurr = sWardrobeGetLayer( pGame, pWardrobe, &tCostume, nLayerCurr, INVALID_ID, !bFirstLoop );
			if ( nLayerCurr == INVALID_ID )
				break;
			bFirstLoop = FALSE;
			WARDROBE_LAYER_DATA * pLayer = (WARDROBE_LAYER_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, nLayerCurr );
			if ( ! pLayer )
				continue;

			if ( pLayer->nState == INVALID_ID )
				continue;

			BOOL bExists = FALSE;
			for ( int i = 0; i < nStateCountOld; i++ )
			{
				if ( pnStatesOld[ i ] == pLayer->nState )
				{
					pnStatesOld[ i ] = INVALID_ID;
					bExists = TRUE;
				}
			}

			ASSERT_BREAK( pWardrobe->pClient->nStateCount < MAX_STATES_PER_WARDROBE );
			pWardrobe->pClient->pnStates[ pWardrobe->pClient->nStateCount ] = pLayer->nState;
			pWardrobe->pClient->nStateCount++;
			if ( bExists )
				continue;
			
			if ( pUnit )
				c_StateSet( pUnit, pUnit, pLayer->nState, 0, 0, INVALID_ID );
			else if ( AppIsHammer() )
			{
				StateDoEvents( nModelId, pLayer->nState, 0, FALSE, FALSE );
			}
		}
	}

	for( int i = 0; i < nStateCountOld; i++ )
	{
		if ( pnStatesOld[ i ] == INVALID_ID )
			continue;

		if ( pUnit )
			c_StateClear( pUnit, UnitGetId( pUnit ), pnStatesOld[ i ], 0 );
		else if ( AppIsHammer() )
		{
			StateClearEvents( nModelId, pnStatesOld[ i ], 0, FALSE );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sWardrobeChangeUpdateFlagsForColorColorMaskChange (
	WARDROBE * pWardrobe )
{
	if ( c_WardrobeIsUpdated( pWardrobe->nId ) )
	{
		pWardrobe->pClient->nCostumeToUpdate = pWardrobe->pClient->nCostumeToDraw;
	}

	COSTUME & tCostume = sGetCostumeToUpdate( pWardrobe );

	for ( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
	{
		if ( (tCostume.dwTextureFlags[ i ] & COSTUME_TEXTURE_FLAG_COLOR_MASK_APPLIED) != 0 )
		{
			tCostume.dwTextureFlags[ i ] &= ~MAKE_MASK( LAYER_TEXTURE_DIFFUSE );
		}

		tCostume.dwTextureFlags[ i ] &= ~COSTUME_TEXTURE_FLAG_COLOR_MASK_APPLIED;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetUnitFileHeaderForWardrobeUpdateMessage( 
	UNIT_FILE_HEADER &tHeader, 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	BOOL bIsInInventory = UnitGetContainer( pUnit ) != NULL;
	
	// set header info
	UnitFileSetHeader( tHeader, UNITSAVEMODE_INVALID, pUnit, bIsInInventory, FALSE );

	// forget the regular save flags, we want just some wardrobe stuff	
	memclear( tHeader.dwSaveFlags, sizeof( tHeader.dwSaveFlags ) );
	SETBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_WARDROBE );
	SETBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_WARDROBE_WEAPON_IDS );
	SETBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_WARDROBE_WEAPON_AFFIXES );
	SETBIT( tHeader.dwSaveFlags, UNITSAVE_BIT_WARDROBE_LAYERS );
	
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sWardrobeCreateUpdateMsg (
	GAME * pGame,
	UNIT * pUnit,
	WARDROBE * pWardrobe,

	MSG_SCMD_UNITWARDROBE & tMsg )
{

	// set id of unit for the message
	tMsg.id = UnitGetId( pUnit );

	// setup a unit file header
	UNIT_FILE_HEADER tHeader;	
	sSetUnitFileHeaderForWardrobeUpdateMessage( tHeader, pUnit );
	
	// set xfer object	
	ZeroMemory( &tMsg.buf, sizeof( tMsg.buf ) );
	BIT_BUF buf( tMsg.buf, MAX_WARDROBE_BUFFER);
	XFER<XFER_SAVE> tXfer(&buf);

	// write the wardrobe	
	WardrobeXfer( pUnit, pWardrobe->nId, tXfer, tHeader, TRUE, NULL );

	// set the blob len of the message
	MSG_SET_BLOB_LEN(tMsg, buf, buf.GetLen());
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_WardrobeUpdateClient (
	GAME * pGame,
	UNIT * pUnit,
	GAMECLIENT * pClient )
{
	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe == INVALID_ID )
		return;

	if (UnitTestFlag(pUnit, UNITFLAG_JUSTCREATED))
	{
		return;
	}

	if (UnitCanSendMessages( pUnit ) == FALSE)
	{
		return;
	}
	
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );
	
	MSG_SCMD_UNITWARDROBE tMsg;
	sWardrobeCreateUpdateMsg ( pGame, pUnit, pWardrobe, tMsg );

	if (pClient)
	{
		s_SendUnitMessageToClient(pUnit, ClientGetId(pClient), SCMD_UNITWARDROBE, &tMsg);
	}
	else
	{
		s_SendUnitMessage(pUnit, SCMD_UNITWARDROBE, &tMsg);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeMakeClientOnly (
	UNIT * pUnit )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe != INVALID_ID )
	{
		WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
		ASSERT_RETURN( pWardrobe );
		pWardrobe->dwFlags |= WARDROBE_FLAG_FORCE_USE_BODY | WARDROBE_FLAG_CLIENT_ONLY;

		MSG_SCMD_UNITWARDROBE tMsg;
		sWardrobeCreateUpdateMsg ( pGame, pUnit, pWardrobe, tMsg );
		c_WardrobeUpdateLayersFromMsg( pUnit, &tMsg ); 
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sWardrobeSortWeapons( 
	GAME *pGame,
	UNIT * pUnit,
	WARDROBE *pWardrobe)
{

	// we already have a skill function that does this, but it needs a flat array of unit ids, so create one
	UNITID idWeapons[ MAX_WEAPONS_PER_UNIT ];
	WARDROBE_WEAPON tWardrobeWeaponsCopy[ MAX_WEAPONS_PER_UNIT ];	
	for (int i = 0; i < MAX_WEAPONS_PER_UNIT; ++i)
	{
		WARDROBE_WEAPON *pWardrobeWeapon = &pWardrobe->tWeapons[ i ];
		
		// create flat unit id array
		idWeapons[ i ] = pWardrobeWeapon->idWeaponReal == INVALID_ID ? pWardrobeWeapon->idWeaponPlaceholder : pWardrobeWeapon->idWeaponReal;
		
		// also copy the entry to temp storage 
		tWardrobeWeaponsCopy[ i ] = *pWardrobeWeapon;
			
	}
	
	// sort them
	SkillsSortWeapons( pGame, pUnit, idWeapons );
		
	BOOL bChanged = FALSE;
	// now set the new wardrobe weapon order based on the sorted id list
	int nCurrentIndex = 0;
	for (int i = 0; i < MAX_WEAPONS_PER_UNIT; ++i)
	{
		// get the weapon
		UNITID idWeapon = idWeapons[ i ];
		
		// find the wardrobe weapon in the temp storage
		WARDROBE_WEAPON *pWardrobeWeapon = NULL;
		for (int j = 0; j < MAX_WEAPONS_PER_UNIT; ++j)
		{
			if ( idWeapon == INVALID_ID )
			{
				if ( tWardrobeWeaponsCopy[ j ].idWeaponReal == INVALID_ID )
				{
					pWardrobeWeapon = &tWardrobeWeaponsCopy[ j ];
					if ( i != j )
						bChanged = TRUE;
					break;
				}
			}
			else if ( tWardrobeWeaponsCopy[ j ].idWeaponReal == idWeapon || 
				      tWardrobeWeaponsCopy[ j ].idWeaponPlaceholder == idWeapon )
			{
				pWardrobeWeapon = &tWardrobeWeaponsCopy[ j ];
				if ( i != j )
					bChanged = TRUE;
				break;
			}
		}
		
		// copy to wardrobe at the current index
		ASSERTX_RETVAL( pWardrobeWeapon, bChanged, "Expected wardrobe weapon" );
		pWardrobe->tWeapons[ nCurrentIndex++ ] = *pWardrobeWeapon;
		
	}
		
	return bChanged;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCollapseBoneIndices( 
	COSTUME & tCostume )
{// this assumes that the layers have been sorted
	for ( int i = 0; i < (int) tCostume.nLayerCount; i++ )
	{
		//if ( ! tCostume.ptLayers[ i ].wParam )
		//	continue;  // it is already low enough

		WARDROBE_LAYER_DATA * pCurr = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, tCostume.ptLayers[ i ].nId );
		if ( !pCurr->bHasBoneIndex )
			continue;

		tCostume.ptLayers[ i ].wParam = 0;

		if ( i == 0 )
			continue;

		WARDROBE_LAYER_DATA * pPrev = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, tCostume.ptLayers[ i - 1 ].nId );

		if ( pPrev->nBoneGroup == pCurr->nBoneGroup )
			tCostume.ptLayers[ i ].wParam = tCostume.ptLayers[ i - 1 ].wParam + 1;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void WardrobeUpdateFromInventory(
	UNIT * pUnit)
{
	ASSERT_RETURN(pUnit);

	if (!UnitTestFlag(pUnit, UNITFLAG_WARDROBE_CHANGED))
	{
		return;
	}
	UnitSetWardrobeChanged(pUnit, FALSE);		

	GAME * pGame = UnitGetGame( pUnit );
	
	int nWardrobe = UnitGetWardrobe( pUnit );

	if ( nWardrobe == INVALID_ID )
		return;

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );

	COSTUME & tCostume = IS_SERVER( pGame) ? pWardrobe->pServer->tCostume : pWardrobe->pClient->pCostumes[ 0 ];
	// copy old wardrobe
	ASSERT( tCostume.nLayerCount < MAX_LAYERS_PER_WARDROBE );
	WARDROBE_LAYER_INSTANCE ptLayersOld[ MAX_LAYERS_PER_WARDROBE ];
	unsigned int nLayersOldCount = tCostume.nLayerCount;
	if ( tCostume.nLayerCount )
	{
		MemoryCopy( ptLayersOld, sizeof( ptLayersOld ), tCostume.ptLayers, 
			sizeof(WARDROBE_LAYER_INSTANCE) * tCostume.nLayerCount );
	}
	UNITID pidWeaponsOld[ MAX_WEAPONS_PER_UNIT ];
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );

		pidWeaponsOld[ i ] = UnitGetId( pWeapon );

		sWardrobeWeaponInit( &pWardrobe->tWeapons[ i ] );		
	}

	int nColorSetIndexOld = tCostume.nColorSetIndex;

	// update from inventory
	tCostume.nLayerCount = 0;

	// add default layers
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	ASSERT_RETURN( pUnitData );

	for ( int i = 0; i < MAX_WARDROBE_BODY_LAYERS; i++ )
	{
		int nDefaultLayer = pWardrobe->tBody.pnLayers[ i ];
		if ( nDefaultLayer == INVALID_ID )
			continue;

		tCostume.nLayerCount++;

		sCostumeUpdateLayerArray( pGame, tCostume );

		tCostume.ptLayers[ tCostume.nLayerCount - 1 ].nId = nDefaultLayer;
		tCostume.ptLayers[ tCostume.nLayerCount - 1 ].wParam = 0;
	}

	unsigned int nMaxLayerIndex = WardrobeGetNumLayers(pGame);

	unsigned int pnWeaponWardrobeLayers[ MAX_WEAPONS_PER_UNIT ];
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		pnWeaponWardrobeLayers[ i ] = INVALID_ID;

	int nMaxColorSetPriority = 0;
	UNIT * pColorSetItem = NULL;
	if (pWardrobe && pWardrobe->pServer)
		pColorSetItem = UnitGetByGuid( pGame, pWardrobe->pServer->guidColorsetItem );

	UNIT * pDyeKit = UnitInventoryGetByLocation( pUnit, INVLOC_DYE_KIT );
	if ( pDyeKit )
		pColorSetItem = pDyeKit;

	BOOL bUsingColorsetItem = FALSE;
	if (!pColorSetItem || !ItemIsEquipped(pColorSetItem, pUnit))
	{
		tCostume.nColorSetIndex = INVALID_ID;
	} 
	else 
	{
		bUsingColorsetItem = TRUE;
		int nPriority = INVALID_ID;
		int nColorSetIndex = 0;
		WardrobeGetItemColorSetIndexAndPriority(pGame, pColorSetItem, 
			nColorSetIndex, nPriority,
			INVALID_ID );
		tCostume.nColorSetIndex = nColorSetIndex;
	}

	LEVEL * pUnitLevel = UnitGetLevel(pUnit);
	BOOL bIsTown = pUnitLevel ? LevelIsTown(pUnitLevel) : FALSE;
	BOOL bIsControlUnit = UnitIsControlUnit(pUnit);

	BOOL bNeedsBoneIndexCollapsing = FALSE;
	// add layers from inventory
	UNIT * pItemCurr = UnitInventoryIterate( pUnit, NULL);
	while (pItemCurr != NULL)
	{
		UNIT * pItemNext = UnitInventoryIterate( pUnit, pItemCurr );

		int nItemLocation;
		int nItemLocationX;
		int nItemLocationY;
		if (ItemIsEquipped(pItemCurr, pUnit) && UnitGetInventoryLocation(pItemCurr, &nItemLocation, &nItemLocationX, &nItemLocationY))
		{
			INVLOC_HEADER tLocInfo;
			if (UnitInventoryGetLocInfo(pUnit, nItemLocation, &tLocInfo))
			{
				if (InvLocTestFlag(&tLocInfo, INVLOCFLAG_WARDROBE))
				{
					int wardrobe = ItemGetWardrobe( pGame, UnitGetClass(pItemCurr), 
						UnitGetLookGroup(pItemCurr) );
					const UNIT_DATA * pItemCurrData = UnitGetData( pItemCurr );
					ASSERT( pItemCurrData );
					if ( pItemCurrData && UnitDataTestFlag( pItemCurrData, UNIT_DATA_FLAG_DONT_ADD_WARDROBE_LAYER ) )
						wardrobe = INVALID_ID; // we use this for dye kits
					
					if (UnitIsA(pItemCurr, UNITTYPE_HELM) && 
						UnitGetStat( pUnit, STATS_CHAR_OPTION_HIDE_HELMET ) != 0)
					{
						wardrobe = INVALID_ID;
					}

					if (wardrobe != INVALID_ID)
					{
						ASSERTX((unsigned int)wardrobe < nMaxLayerIndex, pUnitData->szName );
						tCostume.nLayerCount++;

						EXCELTABLE eTable = ExcelGetDatatableByGenus(UnitGetGenus(pItemCurr));
						if(eTable == DATATABLE_ITEMS)
						{
							if(bIsControlUnit)
							{
								if(bIsTown)
								{
									UnitDataLoad(pGame, eTable, UnitGetClass(pItemCurr), UDLT_TOWN_YOU);
								}
								else
								{
									UnitDataLoad(pGame, eTable, UnitGetClass(pItemCurr), UDLT_WARDROBE_YOU);
								}
							}
							else
							{
								if(bIsTown)
								{
									UnitDataLoad(pGame, eTable, UnitGetClass(pItemCurr), UDLT_TOWN_OTHER);
								}
								else
								{
									UnitDataLoad(pGame, eTable, UnitGetClass(pItemCurr), UDLT_WARDROBE_OTHER);
								}
							}
						}

						sCostumeUpdateLayerArray( pGame, tCostume );

						tCostume.ptLayers[ tCostume.nLayerCount - 1 ].nId = wardrobe;
						tCostume.ptLayers[ tCostume.nLayerCount - 1 ].wParam = (WORD)nItemLocationX;

						const WARDROBE_LAYER_DATA * pLayerData = (WARDROBE_LAYER_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, wardrobe );
						if ( pLayerData && pLayerData->bHasBoneIndex )
						{
							bNeedsBoneIndexCollapsing = TRUE;
						}
					}

					if ( tLocInfo.nWeaponIndex != INVALID_ID )
					{
						ASSERT_RETURN(tLocInfo.nWeaponIndex < MAX_WEAPONS_PER_UNIT);
						WARDROBE_WEAPON *pWardrobeWeapon = &pWardrobe->tWeapons[ tLocInfo.nWeaponIndex ];
						sWardrobeWeaponInitFromUnit( pWardrobeWeapon, pItemCurr );
						if ( wardrobe != INVALID_ID )
							pnWeaponWardrobeLayers[ tLocInfo.nWeaponIndex ] = tCostume.nLayerCount - 1;
						UNITLOG_ASSERTX( pColorSetItem != pItemCurr, "Weapons cannot be selected for colorset", pUnit );
					}
					
					if ( !bUsingColorsetItem && tLocInfo.nWeaponIndex == INVALID_ID ) // don't look at the weapon slots for color
					{ // we are going with just the power of the affix now - not paying attention to the item slot's priority
						int nPriority = INVALID_ID;
						int nColorSetIndex = 0;
						WardrobeGetItemColorSetIndexAndPriority(pGame, pItemCurr, 
							nColorSetIndex, nPriority,
							INVALID_ID );
						nPriority += UnitGetExperienceLevel(pItemCurr);

						if ( nPriority > nMaxColorSetPriority )
						{
							nMaxColorSetPriority = nPriority;
							tCostume.nColorSetIndex = nColorSetIndex;
						}
					}
				}
			}
		}

		pItemCurr = pItemNext;
	}

	if ( tCostume.nColorSetIndex == INVALID_ID )
	{
		if ( pUnitData->nColorSet == INVALID_ID )
		{
			tCostume.nColorSetIndex = ColorSetPickRandom( pUnit );
		} else {
			tCostume.nColorSetIndex = pUnitData->nColorSet;
		}
	}

	DWORD dwColorsetOverride = (DWORD) -1;
	if ( UnitGetStatAny( pUnit, STATS_COLORSET_OVERRIDE, &dwColorsetOverride ) &&
		dwColorsetOverride )
	{
		tCostume.nColorSetIndex = dwColorsetOverride;
	}

	// sort by order
	sCostumeSortLayers( tCostume );

	if ( bNeedsBoneIndexCollapsing )
	{// this must happen after sorting - the algorithm takes advantage of sorting
		sCollapseBoneIndices( tCostume );
	}

	// tugboat doesn't autoswap weapons
	if( AppIsHellgate() )
	{
		if ( !UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_DONT_SORT_WEAPONS ) )
		{
			if ( sWardrobeSortWeapons( pGame, pUnit, pWardrobe ) )
			{

				//// if the skill sort swapped the weapons, then swap them here
				//int nTemp = pWardrobe->pnWeaponClass[ WEAPON_INDEX_RIGHT_HAND ];
				//pWardrobe->pnWeaponClass[ WEAPON_INDEX_RIGHT_HAND ] = pWardrobe->pnWeaponClass[ WEAPON_INDEX_LEFT_HAND ];
				//pWardrobe->pnWeaponClass[ WEAPON_INDEX_LEFT_HAND ] = nTemp;

				int nTemp = pnWeaponWardrobeLayers[ WEAPON_INDEX_RIGHT_HAND ];
				pnWeaponWardrobeLayers[ WEAPON_INDEX_RIGHT_HAND ] = pnWeaponWardrobeLayers[ WEAPON_INDEX_LEFT_HAND ];
				pnWeaponWardrobeLayers[ WEAPON_INDEX_LEFT_HAND ] = nTemp;
			}
		}

		// use the left handed layer for the weapon in the left hand - helps with Cabalist left focus item
		if ( pnWeaponWardrobeLayers[ WEAPON_INDEX_LEFT_HAND ] != INVALID_ID )
		{
			ASSERT( pnWeaponWardrobeLayers[ WEAPON_INDEX_LEFT_HAND ] < tCostume.nLayerCount );
			if ( pnWeaponWardrobeLayers[ WEAPON_INDEX_LEFT_HAND ] < tCostume.nLayerCount )
			{
				int nLeftWeaponLayer = tCostume.ptLayers[pnWeaponWardrobeLayers[WEAPON_INDEX_LEFT_HAND]].nId;
				const WARDROBE_LAYER_DATA * pLayerData = (WARDROBE_LAYER_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, nLeftWeaponLayer );
				if (pLayerData && pLayerData->nOffhandLayer != INVALID_ID)
				{
					tCostume.ptLayers[pnWeaponWardrobeLayers[WEAPON_INDEX_LEFT_HAND]].nId = pLayerData->nOffhandLayer;
				}
			}
		}
	}


	// set color set by team if the level requires it
	ROOM * pRoom = UnitGetRoom( pUnit );
	if ( pRoom && pRoom->pLevel )
	{
		const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet( pRoom->pLevel );
		if ( pLevelDef->bUseTeamColors )
		{
			int nTeam = UnitGetTeam( pUnit );
			if ( nTeam != INVALID_ID )
				tCostume.nColorSetIndex = TeamGetColorIndex( pGame, nTeam );
		}
	}

	// check for changes and update client
	BOOL bChanged = FALSE;
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{ 
		UNIT * pWeapon = WardrobeGetWeapon( pGame, pWardrobe->nId, i );
		if ( pidWeaponsOld[ i ] != UnitGetId( pWeapon ) )
		{
			bChanged = TRUE;

			SkillNewWeaponIsEquipped( pGame, pUnit, pWeapon );
		}
	}


	if ( nColorSetIndexOld != tCostume.nColorSetIndex )
		bChanged = TRUE;

	if ( nLayersOldCount != tCostume.nLayerCount )
		bChanged = TRUE;
	else if ( ! bChanged ) 
	{
		for ( unsigned int i = 0; i < tCostume.nLayerCount; i++ )
		{
			if ( tCostume.ptLayers[ i ].nId    != ptLayersOld[ i ].nId || 
				 tCostume.ptLayers[ i ].wParam != ptLayersOld[ i ].wParam )
			{
				bChanged = TRUE;
				break;
			}
		}
	}

	if ( bChanged )
	{
		if ( IS_SERVER( pGame ) )
		{
			s_WardrobeUpdateClient( pGame, pUnit, NULL );
		}
		else
		{
			MSG_SCMD_UNITWARDROBE tMsg;
			sWardrobeCreateUpdateMsg ( pGame, pUnit, pWardrobe, tMsg );
			c_WardrobeUpdateLayersFromMsg( pUnit, &tMsg ); 
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL c_sWardrobeUpdateLayersFromMsg (
	GAME * pGame,
	UNIT * pUnit,
	WARDROBE * pWardrobe,
	int nModelId,
	BOOL bUseWeaponModel,
	BOOL bIgnoreExtraAppearanceGroups,
	BOOL bIsFirstPerson,
	MSG_SCMD_UNITWARDROBE * pMsg )
{
	
	// setup header
	UNIT_FILE_HEADER tHeader;
	sSetUnitFileHeaderForWardrobeUpdateMessage( tHeader, pUnit );

	// setup xfer
	BIT_BUF buf( pMsg->buf, MAX_WARDROBE_BUFFER);
	XFER<XFER_LOAD> tXfer(&buf);
			
	// do the xfer
	BOOL bWeaponsChanged = FALSE;	
	if (WardrobeXfer( pUnit, pWardrobe->nId, tXfer, tHeader, TRUE, &bWeaponsChanged ) == FALSE)
	{
		return FALSE;
	}

	// this used to be just after the weapons xfer
	if ( bWeaponsChanged )
	{
		SkillStopAll( pGame, pUnit );
#if !ISVERSION(SERVER_VERSION)

		// BSP - removing this in favor of the new UIMSG_WARDROBECHANGED
		//   will have to check for cases of this being needed
		//if ( GameGetControlUnit( pGame ) == pUnit )
		//	UISendMessage(WM_INVENTORYCHANGE, UnitGetId( pUnit ), INVLOC_NONE);
#endif// !ISVERSION(SERVER_VERSION)
	}

	// this used to be just before the layers xfer
	if ( bIsFirstPerson )
	{ 
		// replace all of the appearance groups used by the body with first person versions so that we can find the right textures and stuff
		for ( int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; i++ )
		{
			int & nGroup = pWardrobe->tBody.pnAppearanceGroups[ i ];
			if ( nGroup == INVALID_ID )
				continue;
			WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroup = ( WARDROBE_APPEARANCE_GROUP_DATA * ) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, nGroup );
			ASSERT_CONTINUE ( pAppearanceGroup );
			nGroup = pAppearanceGroup->nFirstPersonGroup;
		}
	}

	int nAppearanceLayerCount = 0;
	const APPEARANCE_DEFINITION * pAppearanceDef = NULL;
	if ( pWardrobe->dwFlags & WARDROBE_FLAG_APPEARANCE_DEF_APPLIED )
	{
		int nAppearanceDefId = c_AppearanceGetDefinition( nModelId );
		pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
		ASSERT_RETFALSE( pAppearanceDef );
		for ( int i = 0; i < MAX_WARDROBE_LAYERS_PER_APPEARANCE; i++ )
		{
			if ( pAppearanceDef->pnWardrobeLayerIds[ i ] != INVALID_ID )
				nAppearanceLayerCount++;
		}
	}

	// update layer storage
	COSTUME &tCostume = sGetCostumeForXfer( pGame, *pWardrobe, TRUE );
	int nBaseLayerCount = tCostume.nLayerCount;
	tCostume.nLayerCount = nBaseLayerCount + nAppearanceLayerCount;
	sCostumeUpdateLayerArray( pGame, tCostume );

	// remove the invalid wardrobe layers
	int nLayerIndex = nBaseLayerCount;
	for( int i = 0; i < nLayerIndex; i++ )
	{
		if ( tCostume.ptLayers[ i ].nId == INVALID_ID )
		{
			MemoryCopy( &tCostume.ptLayers[ i ], sizeof(WARDROBE_LAYER_INSTANCE), &tCostume.ptLayers[ nLayerIndex - 1 ], sizeof(WARDROBE_LAYER_INSTANCE) );
			nLayerIndex--;
			i--;
		}
	}

	if ( nAppearanceLayerCount > 0 )
	{
		ASSERT_RETFALSE( pAppearanceDef );
		for ( int i = 0; i < MAX_WARDROBE_LAYERS_PER_APPEARANCE; i++ )
		{
			if ( pAppearanceDef->pnWardrobeLayerIds[ i ] == INVALID_ID )
				continue;
			tCostume.ptLayers[ nLayerIndex ].nId = pAppearanceDef->pnWardrobeLayerIds[ i ];
			tCostume.ptLayers[ nLayerIndex ].wParam = 0;
			nLayerIndex++;
			ASSERT_RETFALSE( nLayerIndex <= (int)tCostume.nLayerCount );
		}
	}

	if ( pWardrobe->pClient && pWardrobe->pClient->nForceLayer != INVALID_ID )
	{
		tCostume.nLayerCount ++;
		sCostumeUpdateLayerArray( pGame, tCostume );
		tCostume.ptLayers[ tCostume.nLayerCount - 1 ].nId = pWardrobe->pClient->nForceLayer;
		tCostume.ptLayers[ tCostume.nLayerCount - 1 ].wParam = 0;
	}

	sCostumeSortLayers( tCostume );

	COSTUME & tCostumeToDraw = sGetCostumeToDraw( pWardrobe );

	BOOL bLayersChanged = FALSE;
	if ( pWardrobe->pClient->nCostumeToDraw == pWardrobe->pClient->nCostumeToUpdate )
		bLayersChanged = TRUE;
	else if ( tCostumeToDraw.nLayerCount != tCostume.nLayerCount )
		bLayersChanged = TRUE;
	else 
		bLayersChanged = (memcmp( tCostume.ptLayers, tCostumeToDraw.ptLayers, sizeof( WARDROBE_LAYER_INSTANCE ) * tCostume.nLayerCount ) != 0);

	BOOL bColorSetChanged = ( tCostumeToDraw.nColorSetIndex != tCostume.nColorSetIndex );
	if ( bLayersChanged || bColorSetChanged )
	{
		for( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
		{
			tCostume.dwFlags[ i ] = 0;
			tCostume.dwTextureFlags[ i ] = 0;
		}
		
		tCostume.bModelsAdded = FALSE;

		V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );
	} 
	else
	{
		V( c_sWardrobeRemoveFromUpdateList ( pGame, pWardrobe ) );
	}

	if ( nModelId != INVALID_ID )
	{
		c_WardrobeUpdateAttachments ( pUnit, pWardrobe->nId, nModelId, bUseWeaponModel, !bLayersChanged );
	}

	c_WardrobeUpdateStates( pUnit, nModelId, pWardrobe->nId );

	if ( pUnit )
		c_AppearanceUpdateAnimationGroup( pUnit, pGame, nModelId, UnitGetAnimationGroup( pUnit ) );

	pWardrobe->dwFlags |= WARDROBE_FLAG_UPDATED_BY_SERVER;

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeForceLayerOntoUnit (
	int nWardrobe,
	int nModelThirdPerson,
	int nLayer )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN ( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	if ( ! pWardrobe )
		return;

	if ( nModelThirdPerson == INVALID_ID )
		return;

	if ( ! pWardrobe->pClient )
		return;

	pWardrobe->pClient->nForceLayer = nLayer; // this makes sure that we keep the layer on the unit through randomizations

	COSTUME & tCostume = sGetCostumeToUpdate( pWardrobe );

	// see if there are any debug layers already - of so, just replace them.
	BOOL bFoundDebugLayer = FALSE;
	for ( unsigned int i = 0; i < tCostume.nLayerCount; i++ )
	{
		int nLayerCurr = tCostume.ptLayers[ i ].nId;
		WARDROBE_LAYER_DATA * pLayerData = (WARDROBE_LAYER_DATA *) ExcelGetData( pGame, DATATABLE_WARDROBE_LAYER, nLayerCurr );
		if ( pLayerData && pLayerData->bDebugOnly )
		{
			tCostume.ptLayers[ i ].nId = nLayer;
			bFoundDebugLayer = TRUE;
			break;
		}
	}

	if ( ! bFoundDebugLayer )
	{
		// otherwise, add a new layer
		tCostume.nLayerCount ++;
		sCostumeUpdateLayerArray( pGame, tCostume );

		tCostume.ptLayers[ tCostume.nLayerCount - 1 ].nId = nLayer;
		tCostume.ptLayers[ tCostume.nLayerCount - 1 ].wParam = 0;
	}

	sCostumeSortLayers( tCostume );
	for( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
	{
		tCostume.dwFlags[ i ] = 0;
		tCostume.dwTextureFlags[ i ] = 0;
	}
	
	tCostume.bModelsAdded = FALSE;
	V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeForceLayerOntoUnit (
	UNIT * pUnit,
	int nLayer )
{
	int nWardrobe = UnitGetWardrobe( pUnit );
	int nModelThirdPerson = c_UnitGetModelIdThirdPerson( pUnit );

	c_WardrobeForceLayerOntoUnit( nWardrobe, nModelThirdPerson, nLayer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_WardrobeRemoveLayerFromUnit (
	UNIT * pUnit,
	int nLayer )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN ( pGame );

	int nWardrobe = UnitGetWardrobe( pUnit );
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	if ( ! pWardrobe )
		return;

	int nModelThirdPerson = c_UnitGetModelIdThirdPerson( pUnit );
	if ( nModelThirdPerson == INVALID_ID )
		return;

	if ( ! pWardrobe->pClient )
		return;

	pWardrobe->pClient->nForceLayer = INVALID_ID; 

	COSTUME & tCostume = sGetCostumeToUpdate( pWardrobe );

	for ( unsigned int i = 0; i < tCostume.nLayerCount; i++ )
	{
		if ( tCostume.ptLayers[ i ].nId == nLayer )
		{
			tCostume.ptLayers[ i ].nId = tCostume.ptLayers[ tCostume.nLayerCount - 1 ].nId;
			tCostume.ptLayers[ i ].wParam = tCostume.ptLayers[ tCostume.nLayerCount - 1 ].wParam;
			tCostume.nLayerCount--;
			i--;
		}
	}

	sCostumeSortLayers( tCostume );
	for( int i = 0; i < MAX_WARDROBE_TEXTURE_GROUPS; i++ )
	{
		tCostume.dwFlags[ i ] = 0;
		tCostume.dwTextureFlags[ i ] = 0;
	}

	tCostume.bModelsAdded = FALSE;
	V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void WardrobeSetMask (
	GAME* pGame,
	int nWardrobe,
	DWORD mask )
{
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );

	SET_MASK( pWardrobe->dwFlags, mask );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeSetPrimaryColorReplacesSkin (
	GAME* pGame,
	int nWardrobe,
	BOOL bSet )
{
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );

	BOOL bChanged = TRUE;
	if ( bSet )
	{
		if ( pWardrobe->dwFlags & WARDROBE_FLAG_PRIMARY_COLOR_REPLACES_SKIN )
			bChanged = FALSE;
		else
			pWardrobe->dwFlags |= WARDROBE_FLAG_PRIMARY_COLOR_REPLACES_SKIN;
	} else {
		if ( pWardrobe->dwFlags & WARDROBE_FLAG_PRIMARY_COLOR_REPLACES_SKIN )
			pWardrobe->dwFlags &= ~WARDROBE_FLAG_PRIMARY_COLOR_REPLACES_SKIN;
		else
			bChanged = FALSE;

	}

	if ( bChanged )
	{
		sWardrobeChangeUpdateFlagsForColorColorMaskChange ( pWardrobe );
		c_sWardrobeAddToUpdateList( pGame, pWardrobe );
	}


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeSetColorSetIndex (
	int nWardrobe,
	int nColorSetIndex,
	int nUnittype,
	BOOL bSetAllCostumes )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );

	sWardrobeChangeUpdateFlagsForColorColorMaskChange ( pWardrobe );

	if ( pWardrobe->pClient )
		pWardrobe->pClient->nUnittype = nUnittype;

	if ( bSetAllCostumes )
	{
		for ( int i = 0; i < pWardrobe->pClient->nCostumeCount; i++ )
		{
			COSTUME & tCostume = pWardrobe->pClient->pCostumes[ i ];;
			tCostume.nColorSetIndex = nColorSetIndex;
		}
	} else {
		COSTUME & tCostume = sGetCostumeToUpdate( pWardrobe );
		tCostume.nColorSetIndex = nColorSetIndex;
	}

	V( c_sWardrobeAddToUpdateList( NULL, pWardrobe ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_WardrobeUpdateLayersFromMsg (
	UNIT * pUnit,
	MSG_SCMD_UNITWARDROBE * pMsg )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	if ( GameGetControlUnit( pGame ) == pUnit )
	{
		int nModelCurr = c_UnitGetModelId( pUnit );
		// do the third person model first so that the weapons are officially registered for the animation group selection
		int nModelThird = c_UnitGetModelIdThirdPerson( pUnit );
		int nWardrobe = c_AppearanceGetWardrobe( nModelThird );
		WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
		ASSERT_RETURN( pWardrobe );
		c_sWardrobeUpdateLayersFromMsg( pGame, pUnit, pWardrobe, nModelThird, 
			nModelCurr == nModelThird || nModelCurr == c_UnitGetModelIdThirdPersonOverride( pUnit ), // we still want to add and remove weapons if we are using a model override
			FALSE, FALSE, pMsg );
		if( AppIsHellgate() CLT_VERSION_ONLY(&& ! e_DemoLevelIsActive() ) )
		{
			int nModelFirst = c_UnitGetModelIdFirstPerson( pUnit );
			nWardrobe = c_AppearanceGetWardrobe( nModelFirst );
			WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
			ASSERT_RETURN( pWardrobe );
			c_sWardrobeUpdateLayersFromMsg( pGame, pUnit, pWardrobe, nModelFirst, 
				nModelCurr == nModelFirst, TRUE, TRUE, pMsg );

			V( e_ModelSetFlagbit( nModelFirst, MODEL_FLAGBIT_FIRST_PERSON_PROJ, TRUE  ) );
			V( e_ModelSetFlagbit( nModelThird, MODEL_FLAGBIT_FIRST_PERSON_PROJ, FALSE ) );
		}

#if !ISVERSION(SERVER_VERSION)
		// send this only for the control unit right now - we can send it for other units later if we need to
		UISendMessage(WM_WARDROBECHANGE, UnitGetId( pUnit ), 0);
#endif

	} else {
		int nWardrobe = UnitGetWardrobe( pUnit );
		WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
		ASSERT_RETURN( pWardrobe );

		c_sWardrobeUpdateLayersFromMsg( pGame, pUnit, pWardrobe, 
			c_UnitGetModelIdThirdPerson( pUnit ), TRUE, FALSE, FALSE, pMsg );

		int nModelInventory = c_UnitGetModelIdInventory( pUnit );
		if ( nModelInventory != INVALID_ID )
		{
			nWardrobe = c_AppearanceGetWardrobe( nModelInventory );
			c_sWardrobeUpdateLayersFromMsg( pGame, pUnit, pWardrobe, nModelInventory, TRUE, FALSE, FALSE, pMsg );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_WardrobeTest ( 
	BOOL bTestLowLOD )
{
#if !ISVERSION(SERVER_VERSION)
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN ( pGame );

	LogText( ERROR_LOG, "Wardrobe Test %s\n", bTestLowLOD ? "With LOW TESTING" : "With NO low testing" );
	LogFlush();

	// test the models
	int nModelCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_MODEL );
	for ( int i = 0; i < nModelCount; i++ )
	{
		WARDROBE_MODEL * pWardrobeModel = (WARDROBE_MODEL *) ExcelGetData( NULL, DATATABLE_WARDROBE_MODEL, i );
		if ( ! pWardrobeModel )
			continue;

		WARDROBE_APPEARANCE_GROUP_DATA * pWardrobeAppearanceGroup = 
			(WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, pWardrobeModel->pnAppearanceGroup[ 0 ] );
		if ( ! pWardrobeAppearanceGroup )
			continue;

		int nAppearanceDef = DefinitionGetIdByName( DEFINITION_GROUP_APPEARANCE, pWardrobeAppearanceGroup->pszAppearanceDefinition, 0, TRUE );

		AppearanceDefinitionLoadSkeleton( (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDef ), TRUE );

		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			V( c_sWardrobeModelLoad ( NULL, nAppearanceDef, pWardrobeModel, nLOD, TRUE ) ); 
		}

		ASSERT( pWardrobeModel->pEngineData != NULL );
	}

	// check that all proper textureset groups and model groups are there for each appearance group 
	int nTextureSetGroupCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_TEXTURESET_GROUP );
	REF(nTextureSetGroupCount);
	int nAppearanceGroupCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP );
	int nModelGroupCount	  = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_MODEL_GROUP );

	for ( int j = 0; j < nAppearanceGroupCount; j++ )
	{
		WARDROBE_APPEARANCE_GROUP_DATA * pWardrobeAppearanceGroup = 
			(WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, j );
		if ( ! pWardrobeAppearanceGroup )
			continue;

		for ( int i = 0; i < nTextureSetGroupCount; i++ )
		{
			WARDROBE_TEXTURESET_GROUP * pGroup = (WARDROBE_TEXTURESET_GROUP *) ExcelGetData( NULL, DATATABLE_WARDROBE_TEXTURESET_GROUP, i );
			if (! pGroup )
				continue;

			if ( pGroup->nAppearanceGroupCategory != pWardrobeAppearanceGroup->nCategory ||
				 pGroup->nAppearanceGroupCategory == -1 )
				continue;

			WARDROBE_TEXTURESET_DATA tKey;
			tKey.pnAppearanceGroup[ 0 ]	= j;
			tKey.pnAppearanceGroup[ 1 ]	= INVALID_ID;
			tKey.nTextureSetGroup	= i;
			WARDROBE_TEXTURESET_DATA * pTextureSet = (WARDROBE_TEXTURESET_DATA *)ExcelGetDataByKey( NULL, DATATABLE_WARDROBE_TEXTURESET, &tKey, sizeof(tKey) );
			if ( ! pTextureSet )
			{
				LogText( ERROR_LOG, "%s is missing texture set group %s\n", 
					pWardrobeAppearanceGroup->pszName, 
					pGroup->pszName );
				LogFlush();
			} else {
				char szFolder[ MAX_PATH ];
				PStrPrintf( szFolder, MAX_PATH, "%s\\%s", 
					pWardrobeAppearanceGroup->pszTextureFolder, 
					pTextureSet->pszFolder );
				for ( int k = 0; k < NUM_TEXTURES_PER_SET; k++ )
				{
					if ( pTextureSet->pszTextureFilenames[ k ][ 0 ] == 0 )
						continue;

					char szFilename[ MAX_PATH ];
					PStrPrintf( szFilename, MAX_PATH, "%s\\%s.dds", 
						szFolder,
						pTextureSet->pszTextureFilenames[ k ] );

					if ( ! FileExists( szFilename ) )
					{
						LogText( ERROR_LOG, "%s %s is missing texture file %s\n", 
							pGroup->pszName, 
							pWardrobeAppearanceGroup->pszName,
							szFilename );
						LogFlush();
					}

					if ( k != LAYER_TEXTURE_SPECULAR && 
						 k != LAYER_TEXTURE_NORMAL && 
						 bTestLowLOD )
					{
						PStrPrintf( szFilename, MAX_PATH, "%s\\low\\%s.dds", 
							szFolder,
							pTextureSet->pszTextureFilenames[ k ] );

						if ( ! FileExists( szFilename ) )
						{
							LogText( ERROR_LOG, "LOW - %s %s is missing texture file %s\n", 
								pGroup->pszName, 
								pWardrobeAppearanceGroup->pszName,
								szFilename );
							LogFlush();
						}
					}
				}
			}
		}

		for ( int i = 0; i < nModelGroupCount; i++ )
		{
			WARDROBE_MODEL_GROUP * pGroup = (WARDROBE_MODEL_GROUP *) ExcelGetData( NULL, DATATABLE_WARDROBE_MODEL_GROUP, i );
			if ( ! pGroup )
				continue;
			if ( pGroup->nAppearanceGroupCategory != pWardrobeAppearanceGroup->nCategory ||
				 pGroup->nAppearanceGroupCategory == -1 )
				continue;

			WARDROBE_MODEL tKey;
			tKey.pnAppearanceGroup[ 0 ]	= j;
			tKey.pnAppearanceGroup[ 1 ]	= INVALID_ID;
			tKey.nModelGroup		= i;
			WARDROBE_MODEL * pWardrobeModel = (WARDROBE_MODEL *)ExcelGetDataByKey( NULL, DATATABLE_WARDROBE_MODEL, &tKey, sizeof(tKey) );
			if ( ! pWardrobeModel )
			{
				LogText( ERROR_LOG, "%s is missing model group %s\n", 
					pWardrobeAppearanceGroup->pszName, 
					pGroup->pszName );
				LogFlush();
			}
		}
	}

	// test the blend textures

	// test the layers
	LogText( ERROR_LOG, "******************** Wardrobe Layer Test *********************************\n" );
	LogFlush();

	int nWardrobeAppearanceGroupCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP );
	int nLayerCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_LAYER );
	char * pszName = NULL;
	for ( int i = 0; i < nLayerCount; i++ )
	{
		WARDROBE_LAYER_DATA * pLayerData = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, i );
		if ( ! pLayerData )
			continue;

		BOOL bHasBlendOps = FALSE;
		for ( int j = 0; j < NUM_WARDROBE_BLENDOP_GROUPS; j++ )
		{
			if ( pLayerData->pnWardrobeBlendOp[ j ] != INVALID_ID )
				bHasBlendOps = TRUE;
		}

		if ( pLayerData->pszName[ 0 ] != 0 )
			pszName = pLayerData->pszName;

		if ( ! bHasBlendOps && pLayerData->tAttach.pszAttached[ 0 ] == 0 )
		{
			LogText( ERROR_LOG, "%s on row %d is missing a valid blend op\n", pszName, i + 2 );
		}
		if ( ! bHasBlendOps &&
			pLayerData->nTextureSetGroup == INVALID_ID &&
			pLayerData->nModelGroup == INVALID_ID )
			continue;

		if ( pLayerData->nModelGroup == INVALID_ID )
			continue;

		for ( int nAppearanceGroup = 0; nAppearanceGroup < nWardrobeAppearanceGroupCount; nAppearanceGroup++ )
		{
			WARDROBE_MODEL tModelKey;
			tModelKey.pnAppearanceGroup[ 0 ]	= nAppearanceGroup;
			tModelKey.pnAppearanceGroup[ 1 ]	= INVALID_ID;
			tModelKey.nModelGroup		= pLayerData->nModelGroup;
			WARDROBE_MODEL * pWardrobeModel = (WARDROBE_MODEL *)ExcelGetDataByKey( NULL, DATATABLE_WARDROBE_MODEL, &tModelKey, sizeof(tModelKey) );
			if ( ! pWardrobeModel )
				continue;
			
			int nPartCount = 0;
			WARDROBE_MODEL_PART* pParts = e_WardrobeModelGetParts( pWardrobeModel, nPartCount );

			WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroup = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, nAppearanceGroup );
			if (! pAppearanceGroup )
				continue;

			BLENDOP_GROUP eBlendOpGroup = pAppearanceGroup->eBlendOpGroup;
			ASSERTX_CONTINUE( eBlendOpGroup >= 0, pAppearanceGroup->pszName );
			if ( pLayerData->pnWardrobeBlendOp[ eBlendOpGroup ] == INVALID_ID )
				continue;

			WARDROBE_BLENDOP_DATA * pBlendOpData = (WARDROBE_BLENDOP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_BLENDOP, pLayerData->pnWardrobeBlendOp[ eBlendOpGroup ] );
			if ( ! pBlendOpData )
				continue;

			ASSERTX( pLayerData->nTextureSetGroup != INVALID_ID, pszName ? pszName : "Unknown" );
			if ( pBlendOpData->pnAddParts[ 0 ] != INVALID_ID )
			{
				ASSERTX( pLayerData->nModelGroup != INVALID_ID, pszName ? pszName : "Unknown" );
			}

			BOOL bHasAnyPartsDesired = FALSE;
			for ( int j = 0; j < MAX_PARTS_PER_LAYER && pParts; j++ )
			{
				if ( pBlendOpData->pnAddParts[ j ] == INVALID_ID )
					continue;

				for ( int k = 0; k < nPartCount; k++ )
				{
					if ( pParts[ k ].nPartId == pBlendOpData->pnAddParts[ j ] )
					{
						bHasAnyPartsDesired = TRUE;
					}
				}
			}
			if ( ! bHasAnyPartsDesired )
			{
				for ( int j = 0; j < MAX_PARTS_PER_LAYER && pParts; j++ )
				{
					if ( pBlendOpData->pnAddParts[ j ] == INVALID_ID )
						continue;

					WARDROBE_MODEL_PART_DATA * pPartData = (WARDROBE_MODEL_PART_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_PART, pBlendOpData->pnAddParts[ j ] );
					ASSERT_CONTINUE( pPartData );
					LogText( ERROR_LOG, "%s is missing %s referenced by blend op %s used by layer %s\n", 
						pWardrobeModel->pszFileName, 
						pPartData->pszName,
						pBlendOpData->pszName,
						pszName );
					LogFlush();
				}
			}
		}
	}
#endif //!SERVER_VERSION
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT c_sWardrobeModelLoad ( 
	GAME * pGame, 
	int nAppearanceDefId,
	WARDROBE_MODEL* pWardrobeModel,
	int nLOD,
	BOOL bForceSync )
{
#if !ISVERSION(SERVER_VERSION)
	if ( pWardrobeModel->bAsyncLoadRequested || pWardrobeModel->bLoadFailed[ nLOD ] )
		return S_FALSE;

	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) AppearanceDefinitionGet( nAppearanceDefId );
	if ( ! pAppearanceDef )
		return S_FALSE;
	MATRIX * pmInverseSkinTransform = c_AppearanceDefinitionGetInverseSkinTransform( nAppearanceDefId );
	granny_skeleton* pGrannySkeleton = NULL;
	if( AppIsTugboat() )
	{
		pGrannySkeleton = c_AppearanceDefinitionGetSkeletonGranny	   ( nAppearanceDefId );
	}
#ifdef HAVOK_ENABLED
	hkSkeleton* pSkeleton = NULL;
	if( AppIsHellgate() )
	{
		pSkeleton = c_AppearanceDefinitionGetSkeletonHavok	   ( nAppearanceDefId );
	}
#endif

	if ( pWardrobeModel->nMaterial == INVALID_ID )
	{
		V( e_MaterialNew( &pWardrobeModel->nMaterial, pWardrobeModel->pszDefaultMaterial, TRUE ) );
	}

	ASSERT_RETFAIL( pWardrobeModel->pszFileName );
	TIMER_START( pWardrobeModel->pszFileName );

	for ( int nLODToUpdate = 0; nLODToUpdate < LOD_COUNT; nLODToUpdate++ )
	{
		if ( !e_GetRenderFlag( RENDER_FLAG_WARDROBE_LOD ) && ( nLODToUpdate == LOD_LOW ) )
			continue;

#ifdef HAVOK_ENABLED
		V_RETURN( e_WardrobeModelFileUpdate(
			pWardrobeModel, 
			nLODToUpdate,
			pAppearanceDef->pszFullPath,
			pAppearanceDef->pszSkeleton,
			pmInverseSkinTransform, 
			pSkeleton, 
			pGrannySkeleton,
			FALSE ) );
#else
		V_RETURN( e_WardrobeModelFileUpdate(
			pWardrobeModel, 
			nLODToUpdate,
			pAppearanceDef->pszFullPath,
			pAppearanceDef->pszSkeleton,
			pmInverseSkinTransform, 
			pGrannySkeleton,
			FALSE ) );
#endif
	}

	V_RETURN( e_WardrobeModelLoad( pWardrobeModel, nLOD, bForceSync ) );

#endif //!SERVER_VERSION

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sGetFirstRandomLayer ( 
	COSTUME & tCostume )
{
	return tCostume.tLayerBase.nId + 1;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_WardrobeRandomize ( 
	int nWardrobe )
{
	TIMER_START( "Randomizing Wardrobe" );
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN ( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN ( pWardrobe );
	ASSERT_RETURN ( pWardrobe->pClient );

	WARDROBE_BODY tBody;
	memset( &tBody, -1, sizeof(WARDROBE_BODY) );
	sRandomizeBody( pGame, tBody, pWardrobe->pClient->nAppearanceGroup, TRUE );
	MemoryCopy( &pWardrobe->tBody, sizeof( WARDROBE_BODY ), &tBody, sizeof( WARDROBE_BODY ) );
	pWardrobe->dwFlags &= ~WARDROBE_FLAG_APPEARANCE_DEF_APPLIED;

	for ( int i = 0; i < pWardrobe->pClient->nCostumeCount; i++ )
	{
		for( int j = 0; j < MAX_WARDROBE_TEXTURE_GROUPS; j++ )
		{
			pWardrobe->pClient->pCostumes[ i ].dwFlags[ j ] = 0;
			pWardrobe->pClient->pCostumes[ i ].dwTextureFlags[ j ] = 0;
		}

		pWardrobe->pClient->pCostumes[ i ].bModelsAdded = FALSE;
	}

	V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UNIT * WardrobeGetWeapon( 
	GAME * pGame,
	int nWardrobe,
	int nWeaponIndex )
{
	ASSERT_RETNULL( nWeaponIndex >= 0 && nWeaponIndex < MAX_WEAPONS_PER_UNIT );
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	if ( ! pWardrobe )
		return NULL;
	WARDROBE_WEAPON *pWardrobeWeapon = &pWardrobe->tWeapons[ nWeaponIndex ];
	UNIT * pWeapon = UnitGetById( pGame, pWardrobeWeapon->idWeaponReal );
	if ( IS_CLIENT( pGame ) && ! pWeapon )
		pWeapon = UnitGetById( pGame, pWardrobeWeapon->idWeaponPlaceholder );
	return pWeapon;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int WardrobeGetWeaponIndex( 
	UNIT * pUnit,
	UNITID idWeapon )
{
	if ( ! pUnit )
		return INVALID_ID;
	int nWardrobe = UnitGetWardrobe( pUnit );

	GAME* pGame = UnitGetGame( pUnit );
	ASSERT_RETINVALID( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );

	if ( idWeapon == INVALID_ID )
		return INVALID_ID;
	if ( ! pWardrobe )
		return INVALID_ID;
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		WARDROBE_WEAPON *pWardrobeWeapon = &pWardrobe->tWeapons[ i ];
		if ( pWardrobeWeapon->idWeaponReal == idWeapon )
			return i;
		if ( IS_CLIENT( pUnit ) && idWeapon != INVALID_ID && pWardrobeWeapon->idWeaponPlaceholder == idWeapon )
			return i;
	}
	return INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_WardrobeGetModelDefinition(
	int nWardrobe )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETINVALID( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	if ( ! pWardrobe )
		return INVALID_ID;

	if ( pWardrobe->dwFlags & WARDROBE_FLAG_SHARES_MODEL_DEF )
	{
		if ( pWardrobe->pClient->nModelDefOverride != INVALID_ID )
			return pWardrobe->pClient->nModelDefOverride;

		return c_AppearanceDefinitionGetModelDefinition( pWardrobe->pClient->nAppearanceDefId );
	}
	else
	{
		COSTUME & tCostume = sGetCostumeToDraw( pWardrobe );
		if ( tCostume.nModelDefinitionId != INVALID_ID && ! e_ModelDefinitionExists( tCostume.nModelDefinitionId ) )
		{
			tCostume.nModelDefinitionId = INVALID_ID;
		}
		return tCostume.nModelDefinitionId;
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_WardrobeSetModelDefinitionOverride( 
	int nWardrobe,
	int nModelDefId )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN ( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );
	ASSERT_RETURN( ( pWardrobe->dwFlags & WARDROBE_FLAG_SHARES_MODEL_DEF ) != 0 );
	ASSERT_RETURN( pWardrobe->pClient );
	ASSERT_RETURN( pWardrobe->pClient->nModelDefOverride == INVALID_ID || nModelDefId == INVALID_ID ); // we would need to inform the models about this change otherwise...

	pWardrobe->pClient->nModelDefOverride = nModelDefId;

	for ( int i = 0; i < pWardrobe->pClient->nCostumeCount; i++ )
	{
		//e_ModelDefinitionReleaseRef( pWardrobe->pClient->pCostumes[ i ].nModelDefinitionId );
		pWardrobe->pClient->pCostumes[ i ].nModelDefinitionId = nModelDefId;
		//e_ModelDefinitionAddRef( pWardrobe->pClient->pCostumes[ i ].nModelDefinitionId );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_WardrobeGetWeaponBones(
	int nModel,
	int nWardrobe,
	int * pnBones )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETFALSE( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	if( ! pWardrobe )
		return FALSE;

	ASSERT_RETFALSE( pWardrobe->pClient );
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( pWardrobe->pClient->pnWeaponAttachment[ i ] != INVALID_ID )
		{
			ATTACHMENT * pAttach = c_ModelAttachmentGet( nModel, pWardrobe->pClient->pnWeaponAttachment[ i ] );
			pnBones[ i ] = pAttach ? pAttach->nBoneId : INVALID_ID;
		} else {
			pnBones[ i ] = INVALID_ID;
		}
	}
	return TRUE;
}


//-------------------------------------------------------------------------------------------------
// loads all files so that they can be placed in the pak file
//-------------------------------------------------------------------------------------------------
BOOL c_WardrobeLoadLayer(int i)
{
	WARDROBE_LAYER_DATA * pWardrobeLayer = (WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, i );
	if ( ! pWardrobeLayer )
		return TRUE;

	if ( pWardrobeLayer->tAttach.pszAttached[ 0 ] != 0 )
		c_AttachmentDefinitionLoad( NULL, pWardrobeLayer->tAttach, INVALID_ID, FILE_PATH_UNITS );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sWardrobeModelFileGetFilePath( char * pszIndex, int nBufLen, const char * pszFilePath )
{
	//char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];

	PStrPrintf( pszIndex, nBufLen, "%s%s", FILE_PATH_DATA, pszFilePath );

	//const OS_PATH_CHAR * pszRoot = AppCommonGetRootDirectory( NULL );
	//PStrRemovePath(pszIndex, nBufLen, pszRoot, szFilePath);
	PStrRemoveExtension(pszIndex, nBufLen, pszIndex);

	return TRUE;
}

int WardrobeModelFileGetLine( const char * pszFilePath )
{
	if ( ! pszFilePath )
		return INVALID_ID;

	char pszIndex[MAX_XML_STRING_LENGTH];
	if ( ! sWardrobeModelFileGetFilePath( pszIndex, MAX_XML_STRING_LENGTH, pszFilePath ) )
		return INVALID_ID;

	PStrFixPathBackslash( pszIndex );

	char pszPath[MAX_XML_STRING_LENGTH];
	int nLineCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_MODEL );
	int nLine;
	for ( nLine = 0; nLine < nLineCount; ++nLine )
	{
		WARDROBE_MODEL * pWardrobeModel = (WARDROBE_MODEL *) ExcelGetData( NULL, DATATABLE_WARDROBE_MODEL, nLine );
		ASSERT_CONTINUE( pWardrobeModel );

		PStrCopy( pszPath, pWardrobeModel->pszFileName, MAX_XML_STRING_LENGTH );
		PStrFixPathBackslash( pszPath );
		PStrRemoveExtension( pszPath, MAX_XML_STRING_LENGTH, pszPath );
		if ( 0 == PStrICmp( pszPath, pszIndex ) )
			return nLine;
	}

	return INVALID_ID;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_WardrobeLoadModel(int i)
{
	int nModelCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_MODEL );
	ASSERT_RETFALSE(i < nModelCount);

	WARDROBE_MODEL * pWardrobeModel = (WARDROBE_MODEL *) ExcelGetData( NULL, DATATABLE_WARDROBE_MODEL, i );
	if ( ! pWardrobeModel )
		return TRUE;

	if(pWardrobeModel->pnAppearanceGroup[ 0 ] < 0) {
		return TRUE;
	}

	WARDROBE_APPEARANCE_GROUP_DATA * pWardrobeAppearanceGroup = 
		(WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, pWardrobeModel->pnAppearanceGroup[ 0 ] );
	if (! pWardrobeAppearanceGroup) {
		return TRUE;
	}


	if (	FillPakShouldLoad( pWardrobeModel->pszFileName )
		 /*|| FillPakShouldLoad( pWardrobeAppearanceGroup->pszAppearanceDefinition ) */)
	{
		int nAppearanceDef = DefinitionGetIdByName( DEFINITION_GROUP_APPEARANCE, pWardrobeAppearanceGroup->pszAppearanceDefinition, 0, TRUE );

		AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

		if ( AppCommonIsFillpakInConvertMode() ) 
		{
			AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
			AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
		}

		for (int nLOD = 0; nLOD < LOD_COUNT; nLOD++)
		{
			V( c_sWardrobeModelLoad( NULL, nAppearanceDef, pWardrobeModel, nLOD, TRUE )); 
		}

		if ( AppCommonIsFillpakInConvertMode() ) 
		{
			AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
			AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
		}

		ASSERT( pWardrobeModel->pEngineData != NULL );
	}
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_WardrobeLoadBlendOp(int j, int i)
{
#if !ISVERSION(SERVER_VERSION)
	int nAppearanceGroupCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP );
	ASSERT_RETFALSE(j < nAppearanceGroupCount);

	WARDROBE_APPEARANCE_GROUP_DATA * pWardrobeAppearanceGroup = 
		(WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, j );
	if ( ! pWardrobeAppearanceGroup )
		return TRUE;

	int nBlendOpCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_BLENDOP );
	ASSERT_RETFALSE(i < nBlendOpCount);

	WARDROBE_BLENDOP_DATA * pBlendOp = (WARDROBE_BLENDOP_DATA *)ExcelGetData( NULL, DATATABLE_WARDROBE_BLENDOP, i );
	if ( ! pBlendOp )
		return TRUE;

	char szFilename[ MAX_PATH ];
	const char * pszCurExt = PStrGetExtension( pBlendOp->pszBlendTexture );

	if ( ! pszCurExt )
	{
		// fill in the default extension
		char szExt[8];
		if ( AppIsTugboat() )
		{
			PStrCopy( szExt, 8, "png", 4 );
		} else
		{
			PStrCopy( szExt, 8, "tga", 4 );
		}
		PStrPrintf( szFilename, MAX_PATH, "%s%s.%s", pWardrobeAppearanceGroup->pszBlendFolder, pBlendOp->pszBlendTexture, szExt );
	}
	else
	{
		PStrPrintf( szFilename, MAX_PATH, "%s%s", pWardrobeAppearanceGroup->pszBlendFolder, pBlendOp->pszBlendTexture );
	}

	if ( ! FileExists( szFilename ) ) 
		return TRUE;

	if ( ! FillPakShouldLoad( szFilename ) )
		return TRUE;

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
	}

	TEXTURE_DEFINITION tDefOverride;
	ZeroMemory( &tDefOverride, sizeof( TEXTURE_DEFINITION ) );
	tDefOverride.nWidth  = 0;// not sure about this part
	tDefOverride.nHeight = 0;
	tDefOverride.nMipMapUsed = 0;
	tDefOverride.nFormat = e_GetTextureFormat( 
		e_WardrobeGetLayerFormat( LAYER_TEXTURE_BLEND ) );

	int nMaxLen = min( WARDROBE_FILE_STRING_SIZE, min( DEFAULT_FILE_SIZE, MAX_PATH ) );
	int nTexture;
	DWORD dwReloadFlags = 0;
	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
	{
		V_CONTINUE( e_WardrobeTextureNewLayer( &nTexture, 
			pWardrobeAppearanceGroup->pszBlendFolder, NULL, 
			pBlendOp->pszBlendTexture, nMaxLen, LAYER_TEXTURE_BLEND, 
			nLOD, INVALID_ID, &tDefOverride, dwReloadFlags ) );

		if ( e_GetRenderFlag( RENDER_FLAG_WARDROBE_LOD )
			&& ( nLOD == LOD_LOW ) 
			&& e_TextureIsValidID( nTexture ) 
			&& !e_TextureLoadFailed( nTexture ) )
			dwReloadFlags |= TEXTURE_LOAD_ADD_TO_HIGH_PAK;
	}

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	REF(nTexture);
	return TRUE;
#else
	UNREFERENCED_PARAMETER(i);
	UNREFERENCED_PARAMETER(j);
	return FALSE;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_WardrobeLoadTexture(int i, int k)
{
#if !ISVERSION(SERVER_VERSION)
	int nTextureSetCount = ExcelGetNumRows( NULL, DATATABLE_WARDROBE_TEXTURESET );
	ASSERT_RETFALSE(i < nTextureSetCount);
	ASSERT_RETFALSE(k < NUM_TEXTURES_PER_SET);

	WARDROBE_TEXTURESET_DATA * pTextureSet = (WARDROBE_TEXTURESET_DATA *)ExcelGetData( NULL, DATATABLE_WARDROBE_TEXTURESET, i );
	if (pTextureSet == NULL || pTextureSet->pnAppearanceGroup[0] == INVALID_ID) {
		return TRUE;
	}

	WARDROBE_APPEARANCE_GROUP_DATA * pWardrobeAppearanceGroup = 
		(WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, pTextureSet->nAppearanceGroupForFolder );
	ASSERT_RETFALSE(pWardrobeAppearanceGroup != NULL);

	char szFolder[ MAX_PATH ];
	PStrPrintf( szFolder, MAX_PATH, "%s\\%s", 
		pWardrobeAppearanceGroup->pszTextureFolder, 
		pTextureSet->pszFolder );

	if ( pTextureSet->pszTextureFilenames[ k ][ 0 ] == 0 ) {
		return TRUE;
	}

	char szFilename[ MAX_PATH ];
	const char * pszCurExt = PStrGetExtension( pTextureSet->pszTextureFilenames[ k ] );
	if (!pszCurExt) {
		// fill in the default extension
		char szExt[8];
		if (AppIsTugboat()) {
			PStrCopy( szExt, 8, "png", 4 );
		} else {
			PStrCopy( szExt, 8, "tga", 4 );
		}
		PStrPrintf( szFilename, MAX_PATH, "%s\\%s.%s", szFolder, pTextureSet->pszTextureFilenames[ k ], szExt );
	} else {
		PStrPrintf( szFilename, MAX_PATH, "%s\\%s", szFolder, pTextureSet->pszTextureFilenames[ k ] );
	}

	if ( ! FileExists( szFilename ) ) 
		return TRUE;

	if ( ! FillPakShouldLoad( szFilename ) )
		return TRUE;

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
	}

	int nMaxLen = min( WARDROBE_FILE_STRING_SIZE, min( DEFAULT_FILE_SIZE, MAX_PATH ) );
	int nTexture;
	DWORD dwReloadFlags = 0;
	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ ) {
		TEXTURE_DEFINITION tDefOverride;
		ZeroMemory( &tDefOverride, sizeof( TEXTURE_DEFINITION ) );
		tDefOverride.nWidth  = pTextureSet->pnTextureSizes[ k ][ 0 ];
		tDefOverride.nHeight = pTextureSet->pnTextureSizes[ k ][ 1 ];

		if ( nLOD != LOD_HIGH ) {
			tDefOverride.nWidth  >>= 1;
			tDefOverride.nHeight >>= 1;
		}

		V_CONTINUE( e_WardrobeTextureNewLayer( &nTexture,
			pWardrobeAppearanceGroup->pszTextureFolder,
			pTextureSet->pszFolder, 
			pTextureSet->pszTextureFilenames[ k ], nMaxLen, 
			(LAYER_TEXTURE_TYPE) k, nLOD, INVALID_ID,
			&tDefOverride, dwReloadFlags ) );

		if ( e_GetRenderFlag( RENDER_FLAG_WARDROBE_LOD )
			&& ( nLOD == LOD_LOW ) 
			&& e_TextureIsValidID( nTexture ) 
			&& !e_TextureLoadFailed( nTexture ) ) {
			dwReloadFlags |= TEXTURE_LOAD_ADD_TO_HIGH_PAK;
		}
	}

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	REF(nTexture);
	return TRUE;
#else
	UNREFERENCED_PARAMETER(i);
	UNREFERENCED_PARAMETER(k);
	return FALSE;
#endif
}

void c_WardrobeLoadAll()
{
#if !ISVERSION(SERVER_VERSION)
	FillPak_LoadWardrobesAll();
#endif //!SERVER_VERSION
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sCheckValidWardrobeAndBody( 
	WARDROBE * pWardrobe,
	WARDROBE_BODY_SECTION eBodySection )
{
	ASSERT_RETFALSE( pWardrobe );
	ASSERT_RETFALSE( pWardrobe->pClient );
	ASSERT_RETFALSE( eBodySection >= 0 );
	ASSERT_RETFALSE( eBodySection < NUM_WARDROBE_BODY_SECTIONS );
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WARDROBE_INIT c_WardrobeInitForPlayerClass(
	GAME * pGame,
	int nPlayerClass,
	BOOL bRandomizeBody )
{
	UnitDataLoad( NULL, DATATABLE_PLAYERS, nPlayerClass );

	UNIT_DATA * pUnitData = (UNIT_DATA *) ExcelGetData( NULL, DATATABLE_PLAYERS, nPlayerClass );
	if ( ! pUnitData )
		return NULL;

	int nAppearanceGroup = pUnitData->pnWardrobeAppearanceGroup[ UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON ];
	WARDROBE_INIT tWardrobeInit( nAppearanceGroup, pUnitData->nAppearanceDefId, pUnitData->nUnitType );

	tWardrobeInit.nColorSetIndex = pUnitData->nColorSet;

	if ( bRandomizeBody )
		sRandomizeBody( pGame, tWardrobeInit.tBody, tWardrobeInit.nAppearanceGroup, TRUE );

	return tWardrobeInit;
}

//-------------------------------------------------------------------------------------------------
// fills in an array with all of the appearance groups available for a body section - given the current wardrobe
// it returns the number of array entries that it filled in
//-------------------------------------------------------------------------------------------------
int c_WardrobeGetAppearanceGroupOptions( 
	int nWardrobe,
	WARDROBE_BODY_SECTION eBodySection,
	int * pnBodySectionOptions,
	int nMaxBodySectionOptions )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETINVALID( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETINVALID( pWardrobe );
	if ( ! sCheckValidWardrobeAndBody( pWardrobe, eBodySection ) )
		return INVALID_ID;

	int nApperanceGroup = pWardrobe->pClient->nAppearanceGroup;
	ASSERT_RETINVALID( nApperanceGroup >= 0 );

	WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroupData = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, nApperanceGroup );
	ASSERT_RETINVALID(pAppearanceGroupData);

	UNIT * pControlUnit = GameGetControlUnit( pGame );
	ASSERT_RETINVALID( pControlUnit );

	BOOL bIsSubscriber = PlayerIsSubscriber( pControlUnit );
	
	nMaxBodySectionOptions = min( MAX_APPEARANCE_GROUPS_PER_BODY_SECTION, nMaxBodySectionOptions );
	int nCount = 0;
	for ( int i = 0; i < nMaxBodySectionOptions; i++ )
	{
		int nCurr = pAppearanceGroupData->pnBodySectionToAppearanceGroup[ eBodySection ][ i ];
		if ( nCurr == INVALID_ID )
			continue;
		WARDROBE_APPEARANCE_GROUP_DATA * pSubAppearanceGroupData = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, nCurr );
		if ( ! pSubAppearanceGroupData ) 
			continue;
		
		if ( pSubAppearanceGroupData->bSubscriberOnly && ! bIsSubscriber )
			continue;

		pnBodySectionOptions[ nCount ] = nCurr;
		nCount++;
	}

	return nCount;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_WardrobeSetAppearanceGroup( 
	int nWardrobe,
	WARDROBE_BODY_SECTION eBodySection,
	int nAppearanceGroup )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );

	if ( ! sCheckValidWardrobeAndBody( pWardrobe, eBodySection ) )
		return;

	ASSERT_RETURN( ( pWardrobe->dwFlags & WARDROBE_FLAG_CLIENT_ONLY ) );

	WARDROBE_APPEARANCE_GROUP_DATA * pAppearanceGroupData = (WARDROBE_APPEARANCE_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_APPEARANCE_GROUP, nAppearanceGroup );
	if (! pAppearanceGroupData)
		return;
	ASSERT_RETURN( pAppearanceGroupData->eBodySection < 0 || pAppearanceGroupData->eBodySection == eBodySection );

	// set the appearance group
	int nOldAppearanceGroup = pWardrobe->tBody.pnAppearanceGroups[ eBodySection ];
	pWardrobe->tBody.pnAppearanceGroups[ eBodySection ] = nAppearanceGroup;

	// remove any random layers from the old appearance group
	if ( nOldAppearanceGroup != INVALID_ID )
	{
		for ( int i = 0; i < MAX_WARDROBE_BODY_LAYERS; i++ )
		{
			int nLayerCurr = pWardrobe->tBody.pnLayers[ i ];
			if ( nLayerCurr == INVALID_ID )
				continue;

			const WARDROBE_LAYER_DATA * pLayerData = (const WARDROBE_LAYER_DATA *) ExcelGetData( NULL, DATATABLE_WARDROBE_LAYER, nLayerCurr );
			if ( !pLayerData )
				continue;
			for ( int j = 0; j < NUM_APPEARANCE_GROUPS_PER_LAYER; j++ )
			{
				if ( pLayerData->pnRandomAppearanceGroups[ j ] == nOldAppearanceGroup )
				{
					pWardrobe->tBody.pnLayers[ i ] = INVALID_ID;
				}
			}
		}
	}

	// add any random layers for this appearance group
	RAND tRand;
	RandInit( tRand );

	if ( pAppearanceGroupData->nRandomLayerCount )
	{
		for ( int m = 0; m < MAX_WARDROBE_BODY_LAYERS; m++ )
		{
			if ( pWardrobe->tBody.pnLayers[ m ] == INVALID_ID )
			{
				int nRandomLayer = RandGetNum( tRand, pAppearanceGroupData->nRandomLayerCount );
				pWardrobe->tBody.pnLayers[ m ] = pAppearanceGroupData->pnBodyLayers[ nRandomLayer ];
				break;
			}
		}
	}

	c_sWardrobeApplyAppearanceDef( pWardrobe, TRUE, TRUE, TRUE );
	V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_WardrobeGetAppearanceGroup( 
	int nWardrobe,
	WARDROBE_BODY_SECTION eBodySection )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETINVALID( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETINVALID( pWardrobe );

	if ( ! sCheckValidWardrobeAndBody( pWardrobe, eBodySection ) )
		return INVALID_ID;

	return pWardrobe->tBody.pnAppearanceGroups[ eBodySection ];
}


//-------------------------------------------------------------------------------------------------
// Changes the body section to the next option avaiablable
//-------------------------------------------------------------------------------------------------
BODY_SECTION_INCREMENT_RETURN c_WardrobeIncrementBodySection( 
	int nWardrobe,
	WARDROBE_BODY_SECTION eBodySection,
	BOOL bForward )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETVAL( pGame, BODY_SECTION_INCREMENT_ERROR );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETVAL( pWardrobe, BODY_SECTION_INCREMENT_ERROR );

	if ( ! sCheckValidWardrobeAndBody( pWardrobe, eBodySection ) )
		return BODY_SECTION_INCREMENT_ERROR;

	int pnOptionsForBodySection[ MAX_APPEARANCE_GROUPS_PER_BODY_SECTION ];
	int nOptionCount = c_WardrobeGetAppearanceGroupOptions( nWardrobe, 
		eBodySection, pnOptionsForBodySection, MAX_APPEARANCE_GROUPS_PER_BODY_SECTION );
	if ( nOptionCount <= 0 )
	{
		return BODY_SECTION_INCREMENT_NO_OPTIONS;
	} 

	int nCurrentOption = c_WardrobeGetAppearanceGroup( nWardrobe, eBodySection );
	int nNewOption = INVALID_ID;
	BODY_SECTION_INCREMENT_RETURN eReturnVal = BODY_SECTION_INCREMENT_OK;
	for ( int i = 0; i < nOptionCount; i++ )
	{
		if ( nCurrentOption == pnOptionsForBodySection[ i ] )
		{
			int nOptionIndex;
			if ( bForward )
			{
				nOptionIndex = i + 1;
				if ( nOptionIndex >= nOptionCount )
				{
					nOptionIndex = 0;
					eReturnVal = BODY_SECTION_INCREMENT_LOOPED;
				}
			} else {
				nOptionIndex = i - 1;
				if ( nOptionIndex < 0 )
				{
					nOptionIndex = nOptionCount - 1;
					eReturnVal = BODY_SECTION_INCREMENT_LOOPED;
				}
			}
			nNewOption = pnOptionsForBodySection[ nOptionIndex ];
			break;
		}
	}
	if ( nNewOption == INVALID_ID )
	{
		nNewOption = pnOptionsForBodySection[ 0 ];
		eReturnVal = BODY_SECTION_INCREMENT_ERROR;
	}

	c_WardrobeSetAppearanceGroup( nWardrobe, eBodySection, nNewOption );

	c_sWardrobeApplyAppearanceDef( pWardrobe, TRUE, TRUE, TRUE );

	for ( int i = 0; i < pWardrobe->pClient->nCostumeCount; i++ )
	{
		for( int j = 0; j < MAX_WARDROBE_TEXTURE_GROUPS; j++ )
		{
			pWardrobe->pClient->pCostumes[ i ].dwFlags[ j ] = 0;
			pWardrobe->pClient->pCostumes[ i ].dwTextureFlags[ j ] = 0;
		}

		pWardrobe->pClient->pCostumes[ i ].bModelsAdded = FALSE;
	}

	V( c_sWardrobeAddToUpdateList( pGame, pWardrobe ) );

	return eReturnVal;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_WardrobeIncrementBodyColor( 
	int nWardrobe, 
	int nWardrobeExtraColor, 
	BOOL bNext )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN ( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );
	ASSERT_RETURN( nWardrobeExtraColor >= 0 );
	ASSERT_RETURN( nWardrobeExtraColor < NUM_WARDROBE_BODY_COLORS_USED );

	if ( bNext )
		pWardrobe->tBody.pbBodyColors[ nWardrobeExtraColor ] ++;
	else
		pWardrobe->tBody.pbBodyColors[ nWardrobeExtraColor ] --;

	pWardrobe->tBody.pbBodyColors[ nWardrobeExtraColor ] = (BYTE) PIN( (int)pWardrobe->tBody.pbBodyColors[ nWardrobeExtraColor ], 0, NUM_WARDROBE_BODY_COLOR_CHOICES - 1 );

	sWardrobeChangeUpdateFlagsForColorColorMaskChange ( pWardrobe );

	V( c_sWardrobeAddToUpdateList( NULL, pWardrobe ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

WARDROBE_CLIENT* c_WardrobeGetClient(
	int nWardrobe )
{
	GAME* pGame = AppGetCltGame();
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );

	if ( ! pWardrobe )
		return NULL;

	return pWardrobe->pClient;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

WARDROBE_BODY* c_WardrobeGetBody( 
	int nWardrobe )
{
	GAME* pGame = AppGetCltGame();
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );

	if ( ! pWardrobe )
		return NULL;

	return &pWardrobe->tBody;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

WARDROBE_WEAPON* c_WardrobeGetWeapons(
	int nWardrobe )
{
	GAME* pGame = AppGetCltGame();
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );

	if ( ! pWardrobe )
		return NULL;

	return &pWardrobe->tWeapons[0];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BYTE c_WardrobeGetBodyColorIndex( 
	int nWardrobe, 
	UINT nIndex )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETZERO( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETZERO( pWardrobe );
	ASSERT_RETZERO( nIndex < NUM_WARDROBE_BODY_COLORS );

	return pWardrobe->tBody.pbBodyColors[nIndex];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_WardrobeSetBodyColor( 
	int nWardrobe, 
	UINT nIndex,
	BYTE bColorIndex )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );

	ASSERT_RETURN( nIndex < NUM_WARDROBE_BODY_COLORS );

	pWardrobe->tBody.pbBodyColors[nIndex] = bColorIndex;

	sWardrobeChangeUpdateFlagsForColorColorMaskChange ( pWardrobe );

	V( c_sWardrobeAddToUpdateList( NULL, pWardrobe ) );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_WardrobeSetBodyColors( 
	int nWardrobe, 
	BYTE pbBodyColors[ NUM_WARDROBE_BODY_COLORS ] )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );

	MemoryCopy( pWardrobe->tBody.pbBodyColors, sizeof( pWardrobe->tBody.pbBodyColors ), 
		pbBodyColors, sizeof(BYTE) * NUM_WARDROBE_BODY_COLORS );

	sWardrobeChangeUpdateFlagsForColorColorMaskChange ( pWardrobe );

	V( c_sWardrobeAddToUpdateList( NULL, pWardrobe ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_WardrobeSetBaseLayer( int nWardrobe, int nBaseLayerId )
{
	GAME* pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );

	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );
	ASSERT_RETURN( pWardrobe->pClient );

	for ( int ii = 0; ii < pWardrobe->pClient->nCostumeCount; ii++ )
	{
		COSTUME& tCostume = pWardrobe->pClient->pCostumes[ ii ];
		tCostume.tLayerBase.nId = nBaseLayerId;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void WardrobeGetItemColorSetIndexAndPriority(
	GAME* pGame, 
	UNIT* pItem,
	int& nColorSetIndex, 
	int& nColorSetPriority,
	int nDefaultColorSetPriority /* = -1 */ )
{	
	const UNIT_DATA* pItemData = ItemGetData( UnitGetClass( pItem ) );
	nColorSetIndex = pItemData ? pItemData->nColorSet : INVALID_ID;	
	nColorSetPriority = nDefaultColorSetPriority;

	// Iterate item affixes to find any color sets
	int nRepeats = 0;
	STATS* pModList = StatsGetModList( pItem, SELECTOR_AFFIX );;
	while ( pModList && (nRepeats < 1000))
	{
		int nAffix = StatsGetStat(pModList, STATS_AFFIX_ID);
		pModList = StatsGetModNext( pModList );
		nRepeats++;
		const AFFIX_DATA *pAffix = AffixGetData(nAffix);
		if( pAffix && pAffix->nColorSetPriority >= nColorSetPriority )
		{
			nColorSetIndex = pAffix->nColorSet;
			nColorSetPriority = pAffix->nColorSetPriority;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void s_WardrobeSetColorsetItem( 
	UNIT * unit, 
	UNIT * pItem )
{
	ASSERT_RETURN( unit );
	ASSERT_RETURN( IS_SERVER( unit ) );
	ASSERT_RETURN( !pItem || ItemIsEquipped(pItem, unit) );

	GAME * pGame = UnitGetGame( unit );
	ASSERT_RETURN( pGame );

	int nWardrobe = UnitGetWardrobe( unit );
	WARDROBE* pWardrobe = sWardrobeGet( pGame, nWardrobe );
	ASSERT_RETURN( pWardrobe );
	ASSERT_RETURN( pWardrobe->pServer );

	BOOL bUsesColorsetOverride = FALSE;
	{
		DWORD dwColorsetOverride = 0;
		if ( UnitGetStatAny( unit, STATS_COLORSET_OVERRIDE, &dwColorsetOverride ) &&
			dwColorsetOverride )
			bUsesColorsetOverride = TRUE;
	}

	UNIT * pDyeKit = UnitInventoryGetByLocation( unit, INVLOC_DYE_KIT );
	if ( pWardrobe->pServer->guidColorsetItem != UnitGetGuid( pItem ) )
	{
		pWardrobe->pServer->guidColorsetItem = pItem ? UnitGetGuid( pItem ) : INVALID_GUID;
		if ( pDyeKit || bUsesColorsetOverride )
		{// don't change anything - we already have our colorset override
		}
		else if ( pItem )
		{
			int nColorSetIndex = INVALID_ID;
			int nColorSetPriority = INVALID_ID;
			WardrobeGetItemColorSetIndexAndPriority( pGame, pItem, nColorSetIndex, nColorSetPriority );

			pWardrobe->pServer->tCostume.nColorSetIndex = nColorSetIndex;

			s_WardrobeUpdateClient( UnitGetGame( unit ), unit, NULL );
		}
		else 
		{
			WardrobeUpdateFromInventory( unit );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int WardrobeAllocate(
	GAME* pGame,
	const UNIT_DATA* pUnitData,
	WARDROBE_INIT* pWardrobeInit,
	int nWardrobeFallback,
	BOOL& bWardrobeChanged )
{
	ASSERTX_RETNULL( pUnitData, "Expected unit data" );

	int nAppearanceGroup = pUnitData->pnWardrobeAppearanceGroup[ UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON ];
	WARDROBE_INIT tWardrobeInit( nAppearanceGroup, pUnitData->nAppearanceDefId, pUnitData->nUnitType );
	if ( ! pWardrobeInit )
	{
		pWardrobeInit = &tWardrobeInit;
		pWardrobeInit->bSharesModelDefinition = UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_WARDROBE_SHARES_MODEL_DEF);
		pWardrobeInit->nWardrobeFallback = nWardrobeFallback;
		pWardrobeInit->bForceFallback = UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_ALWAYS_USE_WARDROBE_FALLBACK );
	}

	if ( ! c_GetToolMode() && IS_CLIENT( pGame ) && UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_LOW_LOD_IN_TOWN ) ) // &&	UnitIsInTown( pUnit ) ) // can't see whether it is town with no ROOM
	{
		pWardrobeInit->nLOD = LOD_LOW;
		pWardrobeInit->bForceLOD = TRUE;
	}

	pWardrobeInit->nAppearanceDefId = pUnitData->nAppearanceDefId;
	pWardrobeInit->bDoesNotNeedServerUpdate = IS_CLIENT( pGame );
	pWardrobeInit->nUnittype = pUnitData->nUnitType;

	bWardrobeChanged = FALSE;
	int nWardrobeBody = pUnitData->nWardrobeBody;
	if ( nWardrobeBody != INVALID_LINK )
	{
		WardrobeApplyBodyDataToInit( pGame, *pWardrobeInit, nWardrobeBody, nAppearanceGroup );
		bWardrobeChanged = TRUE;
	}

	return WardrobeInit( pGame, *pWardrobeInit );	
}