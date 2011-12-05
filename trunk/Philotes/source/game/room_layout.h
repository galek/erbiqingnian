//----------------------------------------------------------------------------
// room_layout.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _ROOM_LAYOUT_H_
#define _ROOM_LAYOUT_H_

#ifndef _DEFINITION_COMMON_H_
#include "definition_common.h"
#endif

//----------------------------------------------------------------------------
// New layout stuffs
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum ROOM_LAYOUT_ITEM_TYPE
{
	ROOM_LAYOUT_ITEM_INVALID = -1,
	
	ROOM_LAYOUT_ITEM_GROUP,
	ROOM_LAYOUT_ITEM_SPAWNPOINT,
	ROOM_LAYOUT_ITEM_GRAPHICMODEL,
	ROOM_LAYOUT_ITEM_ATTACHMENT,
	ROOM_LAYOUT_ITEM_LABELNODE,
	ROOM_LAYOUT_ITEM_LAYOUTLINK,
	ROOM_LAYOUT_ITEM_ENVIRONMENTBOX,
	ROOM_LAYOUT_ITEM_AI_NODE,
	ROOM_LAYOUT_ITEM_STATIC_LIGHT,

	ROOM_LAYOUT_ITEM_MAX,	// keep this last please
};

//----------------------------------------------------------------------------
enum ROOM_SPAWN_TYPE
{
	ROOM_SPAWN_MONSTER,
	ROOM_SPAWN_OBJECT,
	ROOM_SPAWN_SPAWN_CLASS,
	ROOM_SPAWN_INTERACTABLE,
};

//----------------------------------------------------------------------------
enum
{
	ROOM_LAYOUT_WEIGHT_PERCENTAGE,
	ROOM_LAYOUT_RANDOM_ROTATIONS,

	ROOM_LAYOUT_AI_NODE_CROUCH,
	ROOM_LAYOUT_AI_NODE_DOORWAY,
	ROOM_LAYOUT_AI_NODE_LARGE_COVER,

	ROOM_LAYOUT_NOT_THEME,
	ROOM_LAYOUT_NO_THEME,
	
	ROOM_LAYOUT_EXPANDED,
	
	ROOM_LAYOUT_AI_NODE_STONE,	
	
};

#define ROOM_LAYOUT_FLAG_WEIGHT_PERCENTAGE		MAKE_MASK(ROOM_LAYOUT_WEIGHT_PERCENTAGE)
#define ROOM_LAYOUT_FLAG_RANDOM_ROTATIONS		MAKE_MASK(ROOM_LAYOUT_RANDOM_ROTATIONS)
#define ROOM_LAYOUT_FLAG_NOT_THEME				MAKE_MASK(ROOM_LAYOUT_NOT_THEME)
#define ROOM_LAYOUT_FLAG_NO_THEME				MAKE_MASK(ROOM_LAYOUT_NO_THEME)
#define ROOM_LAYOUT_FLAG_EXPANDED				MAKE_MASK(ROOM_LAYOUT_EXPANDED)

#define ROOM_LAYOUT_FLAG_AI_NODE_CROUCH			MAKE_MASK(ROOM_LAYOUT_AI_NODE_CROUCH)
#define ROOM_LAYOUT_FLAG_AI_NODE_DOORWAY		MAKE_MASK(ROOM_LAYOUT_AI_NODE_DOORWAY)
#define ROOM_LAYOUT_FLAG_AI_NODE_LARGE_COVER	MAKE_MASK(ROOM_LAYOUT_AI_NODE_LARGE_COVER)
#define ROOM_LAYOUT_FLAG_AI_NODE_STONE			MAKE_MASK(ROOM_LAYOUT_AI_NODE_STONE)

//----------------------------------------------------------------------------
struct ROOM_LAYOUT_GROUP
{
	// Permanent data
	char							pszName[MAX_XML_STRING_LENGTH];
	int								nTheme;

	int								nGroupCount;
	ROOM_LAYOUT_GROUP *				pGroups;
	
	ROOM_LAYOUT_GROUP *				pParent;

	ROOM_LAYOUT_ITEM_TYPE			eType;
	int								nWeight;	
	VECTOR							vPosition;
	VECTOR							vNormal;	
	float							fRotation;
	BOOL							bInitialized;
	DWORD							dwUnitType;
	DWORD							dwCode;
	int								nVolume;
	DWORD							dwFlags;
	float							fBuffer;
	int								nQuest;
	float							fFalloffNear;
	float							fFalloffFar;
	VECTOR							vScale;
	// Spawn Class Properties		
	float							fSpawnClassRadius;
	int								iSpawnClassExecuteXTimes;							
	// Temporal data
	BOOL							bReadOnly;
	BOOL							bFollowed;
	int								nModelId;
	int								nLayoutId;
	BOOL							bPropIsValid;
	BOOL							bPropIsChecked;
	BOOL							bDisplayAppearance;

};

struct ROOM_LAYOUT_FIXUP
{
	// Fixup data - this is recalculated every time the layout is revisited.
	BOOL							bFixedUp;
	VECTOR							vFixupPosition;
	VECTOR							vFixupNormal;
	float							fFixupRotation;
	// This is bad and evil because the orientation array is REALLOC()ed 
	//struct ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation;
	int								nOrientationIndex;
	ROOM_LAYOUT_GROUP *				pLayoutGroup;
	struct ROOM_PATH_NODE *			pNearestNode;

	int								nGroupCount;
	ROOM_LAYOUT_FIXUP *				pGroups;
};

//----------------------------------------------------------------------------
enum
{
	EDIT_TYPE_POLY_PICK,
	EDIT_TYPE_FREE_CONTROL,
	EDIT_TYPE_FREE_ROTATE,
	EDIT_TYPE_MULTIDROP,
	EDIT_TYPE_POLY_PICK_NONORMAL,
};

//----------------------------------------------------------------------------
struct ROOM_LAYOUT_GROUP_DEFINITION
{
	DEFINITION_HEADER				tHeader;
	ROOM_LAYOUT_GROUP				tGroup;
	BOOL							bShowIcons;
	int								nEditType;
	BOOL							bExists;
	BOOL							bFixup;
};

//----------------------------------------------------------------------------
struct ROOM_LAYOUT_SELECTION_ORIENTATION
{
	VECTOR	vPosition;
	VECTOR	vNormal;
	float	fRotation;
	struct ROOM_PATH_NODE *	pNearestNode;
};

//----------------------------------------------------------------------------
struct ROOM_LAYOUT_SELECTION
{
	int nCount;
	ROOM_LAYOUT_GROUP ** pGroups;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientations;
};

//----------------------------------------------------------------------------
struct TRACKED_SPAWN_POINT
{
	ROOM_LAYOUT_GROUP *pSpawnPoint;		// the spawn point this is keeping track of
	int nNumUnits;						// number of units created via this spawn point
	UNITID *pidUnits;					// the unit instances created via this spawn point
};

//----------------------------------------------------------------------------
struct ROOM_LAYOUT_SELECTIONS
{
	ROOM_LAYOUT_SELECTION		pRoomLayoutSelections[ROOM_LAYOUT_ITEM_MAX];
	int *						pPropModelIds;
	TRACKED_SPAWN_POINT			*pTrackedSpawnPoints;
	int							nNumTrackedSpawnPoints;
};

#define ROOM_LAYOUT_SUFFIX "_layout.xml"

void sRoomLayoutSetReadOnly(
	 ROOM_LAYOUT_GROUP* pGroup,
     BOOL bReadOnly,
	 BOOL bSkipIfLayoutLink);

BOOL RoomLayoutDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad);

void RoomLayoutFixPosition(
	ROOM_LAYOUT_GROUP * pGroup,
	const MATRIX &matAlign);

ROOM_LAYOUT_GROUP * RoomGetNodeSet( 
	struct ROOM * room, 
	const char * pszSetName,
	ROOM_LAYOUT_SELECTION_ORIENTATION ** ppOrientation );

ROOM_LAYOUT_GROUP * RoomGetLabelNode( 
	struct ROOM * room, 
	const char * pszLabelName,
	ROOM_LAYOUT_SELECTION_ORIENTATION ** ppOrientation );

ROOM_LAYOUT_GROUP * LevelGetLabelNode( 
	struct LEVEL * level,
	const char * pszLabelName,
	struct ROOM ** pRoom,
	ROOM_LAYOUT_SELECTION_ORIENTATION ** ppOrientation);

ROOM_LAYOUT_GROUP * RoomGetAINodeFromPoint( 
	struct ROOM * pRoom, 
	const VECTOR & vWorldPosition,
	const VECTOR & vTargetWorldPosition,
	DWORD dwFlags,
	struct ROOM ** pDestinationRoom,
	ROOM_LAYOUT_SELECTION_ORIENTATION ** ppOrientation,
	float fMinDistance,
	float fMaxDistance);

void RoomLayoutGetSpawnPosition( 
	const ROOM_LAYOUT_GROUP * pSpawnPoint, 
	const ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation,
	struct ROOM * pRoom, 
	VECTOR & vSpawnPosition, 
	VECTOR & vSpawnDirection,
	VECTOR & vSpawnUp );

void RoomLayoutSelectLayoutItems(
	struct GAME * pGame,
	struct ROOM * pRoom,
	ROOM_LAYOUT_GROUP_DEFINITION * pDef,
	ROOM_LAYOUT_GROUP * pLayoutSet,
	DWORD dwSeed,
	int * pnThemes = NULL,
	int nThemeIndexCount = 0);

void RoomLayoutSelectionsInit(
	GAME * pGame,
	ROOM_LAYOUT_SELECTIONS *& pLayoutSelections);

void RoomLayoutSelectionsFree(
	GAME * pGame,
	ROOM_LAYOUT_SELECTIONS * pLayoutSelections);

void RoomLayoutSelectionsRecreateSelectionData(
	GAME * pGame,
	ROOM * pRoom);

void RoomLayoutSelectionsFreeSelectionData(
	GAME * pGame,
	ROOM * pRoom);

TRACKED_SPAWN_POINT *RoomGetOrCreateTrackedSpawnPoint(
	GAME *pGame,
	ROOM *pRoom,
	ROOM_LAYOUT_GROUP *pSpawnPoint);

void RoomTrackedSpawnPointAddUnit(
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint,
	struct UNIT *pUnit);

void RoomTrackedSpawnPointReset(
	GAME *pGame,
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint);

ROOM_LAYOUT_SELECTION *RoomGetLayoutSelection(
	ROOM *pRoom,
	ROOM_LAYOUT_ITEM_TYPE eType);

void RoomLayoutGetStaticLightDefinitionFromGroup(
	const ROOM * pRoom,
	const ROOM_LAYOUT_GROUP * pGroup,
	const ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation,
	struct STATIC_LIGHT_DEFINITION * pDefinition);

ROOM_LAYOUT_SELECTION_ORIENTATION * RoomLayoutGetOrientationFromGroup(
	ROOM * pRoom,
	ROOM_LAYOUT_GROUP * pLayoutGroup);

#endif
