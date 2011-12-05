//----------------------------------------------------------------------------
// objects.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _OBJECTS_H_
#define _OBJECTS_H_

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _EXCEL_H_
#include "excel.h"
#endif


typedef BOOL (*PFN_TRIGGER_FUNCTION)( GAME * pGame, GAMECLIENT * client, UNIT * trigger, const struct OBJECT_TRIGGER_DATA * triggerdata );

//----------------------------------------------------------------------------
struct OBJECT_TRIGGER_DATA
{
	char					pszName[DEFAULT_INDEX_SIZE];
	BOOL					bClearTrigger;
	char				    szTriggerFunction[ MAX_FUNCTION_STRING_LENGTH ];
	SAFE_FUNC_PTR(PFN_TRIGGER_FUNCTION, pfnTrigger);
	BOOL					bIsWarp;
	BOOL					bIsSubLevelWarp;
	BOOL					bIsDoor;
	BOOL					bIsBlocking;
	BOOL					bIsDynamicWarp;
	BOOL					bAllowsInvalidLevelDestination;
	int						nStateBlocking;
	int						nStateOpen;
	BOOL					bDeadCanTrigger;
	BOOL					bGhostCanTrigger;
	BOOL					bHardcoreDeadCanTrigger;
	BOOL					bObjectRequiredCheckIgnored;
	
};

//----------------------------------------------------------------------------
enum OPERATE_RESULT
{
	OPERATE_RESULT_FORBIDDEN,
	OPERATE_RESULT_FORBIDDEN_BY_QUEST,
	OPERATE_RESULT_OK	
};

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum OBJECT_SPEC_FLAGS
{
	OSF_NO_DATABASE_OPERATIONS,
	OSF_IGNORE_SPAWN_LEVEL
};

//----------------------------------------------------------------------------
struct OBJECT_SPEC
{
	int nClass;
	int nLevel;
	ROOM *pRoom;
	VECTOR vPosition;
	const VECTOR *pvFaceDirection;
	const VECTOR *pvNormal;
	UNITID id;
	int nTeam;
	float flScale;
	DWORD dwFlags;		// see OBJECT_SPEC_FLAGS
	PGUID guidOwner;
	const WCHAR *puszName;
	
	OBJECT_SPEC::OBJECT_SPEC( void )
		:	nClass( INVALID_LINK ),
			nLevel( 1 ),
			pRoom( NULL ),
			vPosition( cgvNone ),
			pvFaceDirection( NULL ),
			pvNormal( NULL ),
			id( INVALID_ID ),
			nTeam( INVALID_TEAM ),
			flScale( 1.0f ),
			dwFlags( 0 ),
			guidOwner( INVALID_GUID ),
			puszName( NULL )
	{ }
	
};

UNIT * ObjectInit(
	GAME *game,
	OBJECT_SPEC &tSpec);

void ObjectSetOperateByUnitClass( UNIT *pObject, int nUnitClass, int nItemIndex );
void ObjectSetOperateByUnitType( UNIT *pObject, int nUnitType, int nItemIndex );

void ObjectSetFuse(
	UNIT * pObject,
	int nTicks);
	
void ObjectUnFuse(
	UNIT * pObject);

UNIT* s_ObjectSpawn(
	GAME *pGame,
	OBJECT_SPEC &tSpec);

/*
UNIT* s_ObjectSpawn(
	GAME *game,
	int nClass,
	int nLevel,
	struct ROOM* room,
	const VECTOR &vPosition,
	const VECTOR &vFaceDirection);
*/

void c_ObjectLoad(
	GAME * pGame,
	int nClass );

//Tugboat added
void ObjectClearBlockedPathNodes(
			UNIT* pUnit );

void ObjectReserveBlockedPathNodes(
			UNIT* unit );
//end tugboat added

// "operated" objects are things player click to use (currently) :) -Colin
OPERATE_RESULT ObjectCanOperate(
	UNIT *pOperator,
	UNIT *pObject);

// "triggered" objects are things that are automatically used as the player gets close, like warps and markers
OPERATE_RESULT ObjectCanTrigger(
	UNIT *pUnit,
	UNIT *pObject);

void ObjectTrigger(
	struct GAME * game,
	struct GAMECLIENT * client,
	struct UNIT * unit,
	struct UNIT * object);

void ObjectsCheckForTriggers(
	struct GAME * game,
	struct GAMECLIENT * client,
	struct UNIT * unit);

void ObjectsCheckForTriggersOnRoomChange(
	struct GAME * game,
	struct GAMECLIENT * client,
	struct UNIT * unit,
	struct ROOM * room);

void ObjectTriggerReset(
	struct GAME * game,
	struct UNIT * unit,
	int nClass);

void ObjectDoBlockingCollision(
	UNIT *player,
	UNIT *test);

void ObjectsCheckBlockingCollision(
	UNIT *player);

BOOL ObjectBlocksGhosts( 
	UNIT *pUnit);

BOOL ObjectIsBlocking(
	UNIT *pUnit,
	UNIT *pAsking = NULL);

enum BLOCKING_STAT_VALUE
{
	BSV_NOT_SET = 0,
	BSV_BLOCKING,
	BSV_OPEN,
};

void ObjectSetBlocking(
	UNIT *pUnit);

void ObjectClearBlocking(
	UNIT *pUnit);
	
void ObjectBlockerDisable(
	GAME * game,
	UNIT * player,
	enum GLOBAL_INDEX eGlobalIndexDisable);

BOOL ObjectIsWarp(
	UNIT* pUnit);

BOOL ObjectTriggerIsWarp(
	UNIT *pUnit);

BOOL ObjectIsPortalToTown(
	UNIT *pUnit);

BOOL ObjectIsPortalFromTown(
	UNIT *pUnit);

BOOL ObjectIsVisualPortal(
	UNIT *pUnit);

BOOL ObjectIsDynamicWarp(	
	UNIT *pUnit);

BOOL ObjectWarpAllowsInvalidDestination(	
	UNIT *pUnit);
			
//Tugboat
BOOL ObjectIsDoor(
	UNIT* pUnit);	

int ObjectWarpGetDestinationLevelDepth(
	GAME * game,
	UNIT * trigger);

int ObjectWarpGetDestinationLevelArea(
	GAME * game,
	UNIT * trigger);
//end tugboat

BOOL ObjectIsDoor(
	UNIT* pUnit);	

#if ( ISVERSION( CHEATS_ENABLED ) )		// CHB 2006.07.07 - Inconsistent usage.
void Cheat_OpenWarps(
	GAME * game,
	UNIT * player );
#endif

BOOL ExcelObjectsPostProcess( 
	struct EXCEL_TABLE * table);

BOOL ExcelObjectTriggersPostProcess( 
	struct EXCEL_TABLE * table);

const struct UNIT_DATA* ObjectGetData(
	GAME* game,
	int nClass);

const struct UNIT_DATA* ObjectGetData(
	GAME* game,
	UNIT* unit);

const struct OBJECT_TRIGGER_DATA *ObjectTriggerTryToGetData(
	GAME *pGame,
	int nTriggerType);

BOOL ObjectIsQuestImportantInfo(
	UNIT * unit );
	
BOOL ObjectIsHeadstone( 
	UNIT *pObject, 
	UNIT *pOwner, 
	int *pnCreationTick);

BOOL ObjectIsSubLevelWarp(
	UNIT *pObject);

BOOL ObjectGetName(
	GAME *pGame,
	UNIT *pUnit,
	WCHAR *puszNameBuffer,
	int nBufferSize,
	DWORD dwFlags);

int ObjectGetByCode(
	GAME* game,
	DWORD dwCode);



BOOL s_ObjectAllowedAsStartingLocation( 
	UNIT *pUnit,
	DWORD dwWarpFlags);

UNIT *s_ObjectGetOppositeSubLevelPortal(
	UNIT *pPortal);

SUBLEVELID s_ObjectGetOppositeSubLevel( 
	UNIT *pPortal);

enum TRAIN_COMMANDS
{
	TRAIN_COMMAND_RIDE = 0,
	TRAIN_COMMAND_MOVE,
};

VECTOR ObjectTrainMove( UNIT * train, float fTimeDelta, BOOL bRealStep );
VECTOR ObjectTrainMoveRider( UNIT * pPlayer, VECTOR vCurrentNew, float fTimeDelta );

void c_ObjectStartRide( UNIT * ride, UNIT * player, int nCommand );
void c_ObjectEndRide( UNIT * player );
void s_ObjectStartRide( UNIT * ride, UNIT * player );
void s_ObjectStartMove( UNIT * ride, UNIT * player );
void s_ObjectEndRide( UNIT * player );
void c_ObjectRide( UNIT * player, VECTOR & vRideDelta, float fTimeDelta );

void s_ObjectTrainCheckKill(
	UNIT * pTrain );

enum DOOR_OPERATE
{
	DO_OPEN,
	DO_CLOSE
};

void s_ObjectDoorOperate(
	UNIT *pUnit,
	DOOR_OPERATE eOperate);

enum FONTCOLOR ObjectWarpGetNameColor(
	UNIT *pUnit);

BOOL ObjectRequiresItemToOperation( UNIT *oObject );

BOOL ObjectItemIsRequiredToUse( UNIT *pObject,
							    UNIT *pItem );
#endif
