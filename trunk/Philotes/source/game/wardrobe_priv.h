//----------------------------------------------------------------------------
// c_wardrobe.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

#include "c_attachment.h"
#include "e_wardrobe.h"

#ifndef _AFFIX_H_
#include "affix.h"
#endif


#define MAX_LAYERS_PER_WARDROBE			100
#define MAX_ATTACHMENTS_PER_WARDROBE	 10
#define MAX_STATES_PER_WARDROBE			 10
#define MAX_APPEARANCE_GROUPS_PER_BODY_SECTION 24
#define NUM_WARDROBE_BODY_COLOR_CHOICES	24

typedef enum 
{
	WARDROBE_BODY_SECTION_BASE = 0,
	WARDROBE_BODY_SECTION_HEAD,
	WARDROBE_BODY_SECTION_HAIR,
	WARDROBE_BODY_SECTION_FACIAL_HAIR,
	WARDROBE_BODY_SECTION_SKIN,
	WARDROBE_BODY_SECTION_HAIR_COLOR,
	NUM_WARDROBE_BODY_SECTIONS,
}WARDROBE_BODY_SECTION;

enum WARDROBE_PRIORITY
{
	WARDROBE_PRIORITY_NPC,
	WARDROBE_PRIORITY_PC_LOCAL,
	WARDROBE_PRIORITY_INVENTORY,
	WARDROBE_PRIORITY_PC_PARTY,
	WARDROBE_PRIORITY_PC_REMOTE,
	WARDROBE_PRIORITY_LOWEST,
};

#define MAX_WARDROBE_BODY_LAYERS				10
#define WARDROBE_DEFAULT_LAYERS_COUNT_BITS		4

struct WARDROBE_BODY
{
	int									pnLayers[ MAX_WARDROBE_BODY_LAYERS ];
	int									pnAppearanceGroups[ NUM_WARDROBE_BODY_SECTIONS ];
	BYTE								pbBodyColors[ NUM_WARDROBE_BODY_COLORS ];
};

#define MAX_LAYERS_IN_LAYERSET 32
struct WARDROBE_LAYERSET_DATA 
{
	char pszName[ DEFAULT_INDEX_SIZE ];
	int pnLayers[ MAX_LAYERS_IN_LAYERSET ];
	int nLayerCount;
};

#define MAX_LAYERSETS_PER_BODY 4
struct WARDROBE_BODY_DATA 
{
	char pszName[ DEFAULT_INDEX_SIZE ];
	WARDROBE_BODY tBody;
	BOOL bRandomizeBody;
	int pnRandomLayerSets[ MAX_LAYERSETS_PER_BODY ];
};

struct WARDROBE_INIT
{
	int nAppearanceDefId;
	WARDROBE_PRIORITY ePriority;
	int nLOD;
	BOOL bSharesModelDefinition;
	BOOL bDoesNotNeedServerUpdate;
	BOOL bForceLOD;
	int nMipBonus;
	int nAppearanceGroup;
	int nColorSetIndex;
	int nUnittype;
	WARDROBE_BODY tBody;
	int nWardrobeFallback;
	BOOL bForceFallback;		// Use fallback always, regardless of wardrobe count

	WARDROBE_INIT( int nAppearanceGroup = INVALID_ID, int nAppDefId = INVALID_ID, int nUnittypeIn = UNITTYPE_ANY ) :
		nAppearanceDefId( nAppDefId ),
		nLOD( LOD_LOW ),
		bForceLOD( FALSE ),
		bSharesModelDefinition( FALSE ),
		bDoesNotNeedServerUpdate( FALSE ),
		nAppearanceGroup( nAppearanceGroup ),
		nColorSetIndex( INVALID_ID ),
		nUnittype( nUnittypeIn ),
		nMipBonus( 0 ),
		ePriority( WARDROBE_PRIORITY_LOWEST ),
		nWardrobeFallback( INVALID_ID ),
		bForceFallback( FALSE )
	{
		for ( int i = 0; i < MAX_WARDROBE_BODY_LAYERS; i++ )
			tBody.pnLayers[ i ] = INVALID_ID;

		for ( int i = 0; i < NUM_WARDROBE_BODY_SECTIONS; i++ )
			tBody.pnAppearanceGroups[ i ] = INVALID_ID;
	}
};

// a reference to a layer within a COSTUME
struct WARDROBE_LAYER_INSTANCE 
{
	int									nId;
	WORD								wParam;
};

struct WARDROBE_BLENDOP_DATA;
struct WARDROBE_LAYER_STACK_ELEMENT
{
	WARDROBE_BLENDOP_DATA *				pBlendOp;
	int									nLayerId;
	int									nOrder;
};

typedef CArrayIndexed<WARDROBE_LAYER_STACK_ELEMENT> WARDROBE_LAYER_STACK;

// The other texture bits correspond to LAYER_TEXTURE_TYPE enum.
#define COSTUME_TEXTURE_FLAG_COLOR_MASK_APPLIED	MAKE_MASK(31)

#define COSTUME_FLAG_LAYER_STACK_CREATED		MAKE_MASK(0)

// an instance of a specific look - Frankensteined together from parts
struct COSTUME 
{
	WARDROBE_LAYER_INSTANCE *			ptLayers;
	DWORD								dwFlags[MAX_WARDROBE_TEXTURE_GROUPS];
	DWORD								dwTextureFlags[MAX_WARDROBE_TEXTURE_GROUPS];
	WARDROBE_LAYER_INSTANCE				tLayerBase;
	int									nColorSetIndex;
	unsigned int						nLayerCount;
	unsigned int						nLayersAllocated;

	// client-only
	WARDROBE_LAYER_STACK				tLayerStack[MAX_WARDROBE_TEXTURE_GROUPS];
	WARDROBE_LAYER_STACK				tModelLayerStack;
	int									ppnTextures[MAX_WARDROBE_TEXTURE_GROUPS][NUM_TEXTURES_PER_SET];
	int									nModelDefinitionId;
	int									nLOD;
	BOOL								bModelsAdded;
	BOOL								bInitialized;
};


struct WARDROBE_SERVER 
{
	COSTUME								tCostume;
	PGUID								guidColorsetItem;
};

struct WARDROBE;
struct WARDROBE_CLIENT
{
	COSTUME *							pCostumes;

	int									nCostumeCount;
	int									nCostumeToDraw;
	int									nCostumeToUpdate;

	int									nAppearanceDefId;
	int									nAppearanceGroup;
	int									nUnittype;
	WARDROBE_PRIORITY					ePriority;			// Priority for wardrobe updates
	int									nUpdateTicket;		// Now serving number...
	int									nLOD;
	int 								nMipBonus;
	int 								nModelDefOverride;
	int									nWardrobeFallback;

	int 								nForceLayer;

	int 								nAttachmentCount;
	int 								pnAttachmentIds[ MAX_ATTACHMENTS_PER_WARDROBE ];
	int 								pnWeaponAttachment[ MAX_WEAPONS_PER_UNIT ];

	int 								nStateCount;
	int 								pnStates[ MAX_STATES_PER_WARDROBE ];
};

// The number of wardrobe LOD flags should be kept in sync with LOD_COUNT in lod.h
#define WARDROBE_FLAG_MODEL_DEF_INITIALIZED			MAKE_MASK(0)
#define WARDROBE_FLAG_SHARES_MODEL_DEF				MAKE_MASK(1)
#define WARDROBE_FLAG_APPEARANCE_DEF_APPLIED		MAKE_MASK(2)
#define WARDROBE_FLAG_USES_COLORMASK				MAKE_MASK(3)
#define WARDROBE_FLAG_UPDATED_BY_SERVER				MAKE_MASK(4)
#define WARDROBE_FLAG_REQUIRES_UPDATE_FROM_SERVER	MAKE_MASK(5)
#define WARDROBE_FLAG_DOES_NOT_NEED_SERVER_UPDATE	MAKE_MASK(6)
#define WARDROBE_FLAG_CLIENT_ONLY					MAKE_MASK(7)
#define WARDROBE_FLAG_FORCE_USE_BODY				MAKE_MASK(8)
#define WARDROBE_FLAG_FORCE_USE_BODY_ONLY			MAKE_MASK(9)
#define WARDROBE_FLAG_IGNORE_APPEARANCE_DEF_LAYERS	MAKE_MASK(10)
#define WARDROBE_FLAG_DONT_APPLY_BODY_COLORS		MAKE_MASK(11)
#define WARDROBE_FLAG_UPGRADE_COSTUME_COUNT			MAKE_MASK(12)
#define WARDROBE_FLAG_DOWNGRADE_COSTUME_COUNT		MAKE_MASK(13)
#define WARDROBE_FLAG_UPGRADE_LOD					MAKE_MASK(14)
#define WARDROBE_FLAG_DOWNGRADE_LOD					MAKE_MASK(15)
#define WARDROBE_FLAG_IGNORE_MIP_BONUS				MAKE_MASK(16)
#define WARDROBE_FLAG_KEEP_COSTUME_COUNT_UPGRADED	MAKE_MASK(17)
#define WARDROBE_FLAG_WARDROBE_FALLBACK				MAKE_MASK(18)
#define WARDROBE_FLAG_PRIMARY_COLOR_REPLACES_SKIN	MAKE_MASK(19)
#define WARDROBE_FLAG_NO_THREADED_PROCESSING		MAKE_MASK(20)

//----------------------------------------------------------------------------
struct WARDROBE_WEAPON
{
	UNITID								idWeaponReal;
	UNITID								idWeaponPlaceholder;
	int									nClass;
	int									nAffixes[MAX_AFFIX_ARRAY_PER_UNIT];
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define MAX_PARTS_PER_LAYER 10
#define WARDROBE_FILE_STRING_SIZE 50
#define NUM_APPEARANCE_GROUPS_PER_LAYER 10
#define MAX_BODY_LAYERS_PER_APPEARANCE_GROUP 30
#define MAX_BLENDOP_COVERAGE 10
#define NUM_LAYERSETS_PER_LAYER 4

typedef enum 
{
	BLENDOP_GROUP_DEFAULT = 0,
	BLENDOP_GROUP_TEMPLAR,
	BLENDOP_GROUP_CABALIST,
	BLENDOP_GROUP_HUNTER,
	BLENDOP_GROUP_SATYR,
	BLENDOP_GROUP_CYCLOPS,
	BLENDOP_GROUP_CYCLOPSFEMALE,
	NUM_WARDROBE_BLENDOP_GROUPS,
}BLENDOP_GROUP;

// a set of textures blends and mesh modifications used to change the look of a wardrobe
struct WARDROBE_LAYER_DATA {
	// organization
	char pszName			[ DEFAULT_INDEX_SIZE ]; 
	WORD wCode;

	int  nRow;   
	int  nRowCollection;
	int	 nOrder;
	BOOL bDebugOnly;

	int  pnRandomAppearanceGroups[ NUM_APPEARANCE_GROUPS_PER_LAYER ];
	int  pnLayerSet[ NUM_LAYERSETS_PER_LAYER ];
	int  nCharScreenAppearanceGroup;

	// operation
	int	 nModelGroup;
	int	 nTextureSetGroup;
	int	 pnWardrobeBlendOp[ NUM_WARDROBE_BLENDOP_GROUPS ];
	int	 nOffhandLayer;
	BOOL bCanBeMipped;
	ATTACHMENT_DEFINITION tAttach;	// used mostly by mods
	BOOL bHasBoneIndex;				// used mostly by mods
	int nBoneGroup;
	int nState;
};

struct WARDROBE_TEXTURESET_GROUP 
{
	char pszName		[ DEFAULT_INDEX_SIZE ]; 
	int nAppearanceGroupCategory;
};

// a set of textures used by a layer when blending
#define NUM_APPEARANCES_PER_TEXTURE_SET_GROUP 2
struct WARDROBE_TEXTURESET_DATA {
	int nTextureSetGroup;
	int nAppearanceGroupForFolder;
	int pnAppearanceGroup[ NUM_APPEARANCES_PER_TEXTURE_SET_GROUP ];
	char pszFolder[ DEFAULT_FILE_SIZE ];
	char pszTextureFilenames[ NUM_TEXTURES_PER_SET ][ WARDROBE_FILE_STRING_SIZE ];
	BOOL bLowOnly		[ NUM_TEXTURES_PER_SET ];
	int  pnTextureIds	[ NUM_TEXTURES_PER_SET ][ LOD_COUNT ];
	int  pnTextureSizes	[ NUM_TEXTURES_PER_SET ][ 2 ];
	SAFE_PTR(WARDROBE_TEXTURESET_DATA*, pNext);
};

struct WARDROBE_APPEARANCE_GROUP_DATA {
	char pszName[ DEFAULT_INDEX_SIZE ];
	char pszBlendFolder[ DEFAULT_FILE_SIZE ];
	char pszTextureFolder[ DEFAULT_FILE_SIZE ];
	char pszAppearanceDefinition[ DEFAULT_FILE_SIZE ];
	WORD wCode;
	BOOL bSubscriberOnly;
	BOOL bNoBodyParts;
	BOOL bDontRandomlyPick;
	BLENDOP_GROUP eBlendOpGroup;
	int nFirstPersonGroup;
	int pnBodySectionToAppearanceGroup[ NUM_WARDROBE_BODY_SECTIONS ][ MAX_APPEARANCE_GROUPS_PER_BODY_SECTION ];
	int pnBodyLayers[ MAX_BODY_LAYERS_PER_APPEARANCE_GROUP ];
	int nRandomLayerCount;
	int nCategory;
	WARDROBE_BODY_SECTION eBodySection;
};

// the blend texture and parts to add and remove - used by layers
struct WARDROBE_BLENDOP_DATA {
	char pszName		[ DEFAULT_INDEX_SIZE ]; 

	BOOL bReplaceAllParts;
	BOOL bNoTextureChange;
	int pnRemoveParts	[ MAX_PARTS_PER_LAYER ];
	int pnAddParts		[ MAX_PARTS_PER_LAYER ];

	char pszBlendTexture[ WARDROBE_FILE_STRING_SIZE ]; 
	int  nBlendTexture;
	int nTargetTextureIndex;

	int pnCovers[ MAX_BLENDOP_COVERAGE ];
};

struct WARDROBE_MODEL_GROUP 
{
	char pszName		[ DEFAULT_INDEX_SIZE ]; 
	int nAppearanceGroupCategory;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL ExcelWardrobePostProcess(
	struct EXCEL_TABLE * table);

void ExcelWardrobeModelFreeRow(
	struct EXCEL_TABLE * table,
	BYTE * rowdata);

////////////////////////////////////////////////////////////
// functions for the character creation screen
////////////////////////////////////////////////////////////
int c_WardrobeGetAppearanceGroupOptions( 
	int nWardrobe,
	WARDROBE_BODY_SECTION eBodySection,
	int * pnBodySectionOptions,
	int nMaxBodySectionOptions );

void c_WardrobeSetAppearanceGroup( 
	int nWardrobe,
	WARDROBE_BODY_SECTION eBodySection,
	int nAppearanceGroup );

int c_WardrobeGetAppearanceGroup( 
	int nWardrobe,
	WARDROBE_BODY_SECTION eBodySection );

WARDROBE_INIT c_WardrobeInitForPlayerClass( 
	GAME * pGame,
	int nPlayerClass,
	BOOL bRandomizeBody );

WARDROBE_CLIENT* c_WardrobeGetClient(
	int nWardrobe );

WARDROBE_BODY* c_WardrobeGetBody( 
	int nWardrobe );

WARDROBE_WEAPON* c_WardrobeGetWeapons(
	int nWardrobe );

typedef enum
{
	BODY_SECTION_INCREMENT_OK,
	BODY_SECTION_INCREMENT_LOOPED,
	BODY_SECTION_INCREMENT_NO_OPTIONS,
	BODY_SECTION_INCREMENT_ERROR,
}BODY_SECTION_INCREMENT_RETURN;

BODY_SECTION_INCREMENT_RETURN c_WardrobeIncrementBodySection( 
	int nWardrobe,
	WARDROBE_BODY_SECTION eBodySection,
	BOOL bForward );
