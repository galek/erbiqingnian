//----------------------------------------------------------------------------
// ai_priv.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _AI_PRIV_H_
#define _AI_PRIV_H_

#ifndef _UNITMODES_H_
#include "unitmodes.h"
#endif

#ifndef _MOVEMENT_H_
#include "movement.h"
#endif


//----------------------------------------------------------------------------
// movement
//----------------------------------------------------------------------------
typedef enum 
{
	MOVE_TYPE_TO_UNIT,
	MOVE_TYPE_TO_UNIT_FLY,
	MOVE_TYPE_ABOVE_UNIT_FLY,
	MOVE_TYPE_CIRCLE,
	MOVE_TYPE_AWAY_FROM_UNIT,
	MOVE_TYPE_AWAY_FROM_UNIT_FLY,
	MOVE_TYPE_RANDOM_WANDER,
	MOVE_TYPE_RANDOM_WANDER_FLY,
	MOVE_TYPE_STRAFE,
	MOVE_TYPE_STRAFE_FLY,
	MOVE_TYPE_BACKUP,
	MOVE_TYPE_BACKUP_FLY,
	MOVE_TYPE_LOCATION,
	MOVE_TYPE_LOCATION_FLY,
	MOVE_TYPE_FACE_DIRECTION,
	MOVE_TYPE_WARP,
}MOVE_TYPE;

#define MOVE_REQUEST_FLAG_RUN_AWAY_ON_ANGLE		MAKE_MASK( 0 )
#define MOVE_REQUEST_FLAG_DONT_TURN				MAKE_MASK( 1 )
#define MOVE_REQUEST_FLAG_CHEAP_LOOKUPS_ONLY	MAKE_MASK( 2 )
#define MOVE_REQUEST_FLAG_ALLOW_HAPPY_PLACES	MAKE_MASK( 3 )

struct MOVE_REQUEST_DATA
{
	MOVE_TYPE	eType;
	DWORD		dwFlags;
	UNIT		*pTarget;
	VECTOR		vTarget;
	float		fDistance;
	float		fDistanceMin;
	float		fStopDistance; 
	float		fWanderRange; 
	float		fMinAltitude; 
	float		fMaxAltitude; 
	UNITMODE	eMode;
	BYTE		bMoveFlags;
	PATH_CALCULATION_TYPE ePathType;

	MOVE_REQUEST_DATA(void) :
	eType( MOVE_TYPE_TO_UNIT ),
	dwFlags( 0 ),
	pTarget( NULL ),
	vTarget( 0 ),
	fDistance( 0.0f ),
	fDistanceMin( 0.0f ),
	fStopDistance( 0.0f ),
	fWanderRange( 0.0f ),
	fMinAltitude( 0.0f ),
	fMaxAltitude( 0.0f ),
	eMode( MODE_WALK ),
	bMoveFlags(0),
	ePathType(PATH_FULL)
	{}
};

//----------------------------------------------------------------------------
// Behavior and behavior tables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Excel data
//----------------------------------------------------------------------------
#define AI_BEHAVIOR_STRING_SIZE	32
#define AI_BEHAVIOR_NUM_PARAMS	6
#define AI_BEHAVIOR_NAME_SIZE	60
#define AI_BEHAVIOR_FUNCTION_NAME_SIZE	40
#define AI_BEHAVIOR_DEF_STRING_SIZE 20
#define AI_BEHAVIOR_NUM_SKILLS 2
#define AI_ACTIVE_RANGE 36

typedef BOOL (* FP_BEHAVIOR)(GAME* game, UNIT* unit, struct AI_BEHAVIOR_INSTANCE * pBehavior, struct AI_CONTEXT & tContext );

struct AI_BEHAVIOR_DATA
{
	char pszName[ AI_BEHAVIOR_NAME_SIZE ];
	char pszStringDescription[ AI_BEHAVIOR_STRING_SIZE ];
	char szFunction[ AI_BEHAVIOR_FUNCTION_NAME_SIZE ];
	SAFE_FUNC_PTR(FP_BEHAVIOR, pfnFunction);
	char pszParamDescription[ AI_BEHAVIOR_NUM_PARAMS ][ AI_BEHAVIOR_STRING_SIZE ];
	float fDefaultPriority;
	float pfParamDefault[ AI_BEHAVIOR_NUM_PARAMS ];
	int nFunctionTheOldWay;
	BOOL pbUsesSkill[ AI_BEHAVIOR_NUM_SKILLS ];
	BOOL bUsesState;
	BOOL bUsesStat;
	BOOL bUsesSound;
	BOOL bUsesMonster;
	BOOL bUsesString;
	BOOL bCanBranch;
	BOOL bForceExitBranch;
	int nTimerIndex;
	int nUsesOnce;
	int nUsesRun;
	int nUsesFly;
	int nUsesDontStop;
	int nUsesWarp;
};

#define MAX_STATES_TO_SHARE 1
//----------------------------------------------------------------------------
struct AI_INIT
{
	char 			pszName[ DEFAULT_INDEX_SIZE ];
	char 			pszTableName[ DEFAULT_FILE_SIZE ];
	int				nTable;

	int  			nStartFunction;
	int				nBlockingState;
	int				nFrightenUnitType;
	BOOL			bWantsTarget;
	BOOL			bTargetClosest;
	BOOL			bTargetRandomize;
	BOOL			bNoDestructables;
	BOOL			bRandomStartPeriod;
	BOOL			bAiNoFreeze;
	BOOL			bRecordSpawnPoint;
	BOOL			bCheckBusy;
	BOOL			bStartOnAiInit;
	BOOL			bNeverRegisterAIEvent;
	BOOL			bServerOnly;
	BOOL			bClientOnly;
	BOOL			bCanSeeUnsearchables;

	struct STATE_SHARING
	{
		BOOL			bOnDeath;
		int				nTargetUnitType;
		float			fRange;
		int				nState;
	};
	STATE_SHARING	pStateSharing[ MAX_STATES_TO_SHARE ];
};

//----------------------------------------------------------------------------
struct AI_STATE_DEFINITION
{
	char szName[ DEFAULT_INDEX_SIZE ];
	WORD wCode;
};

//----------------------------------------------------------------------------
// XML definition
//----------------------------------------------------------------------------
#define AI_BEHAVIOR_FLAG_ONCE		MAKE_MASK(0)
#define AI_BEHAVIOR_FLAG_RUN		MAKE_MASK(1)
#define AI_BEHAVIOR_FLAG_FLY		MAKE_MASK(2)
#define AI_BEHAVIOR_FLAG_DONT_STOP	MAKE_MASK(3)
#define AI_BEHAVIOR_FLAG_WARP		MAKE_MASK(4)

struct AI_BEHAVIOR_DEFINITION_TABLE
{
	struct AI_BEHAVIOR_DEFINITION * pBehaviors;
	int nBehaviorCount;
};

struct AI_BEHAVIOR_DEFINITION
{
	float fPriority;
	float fChance;
	int nBehaviorId;
	int nSkillId;
	int nSkillId2;
	int nStateId;
	int nStatId;
	int nSoundId;
	int nMonsterId;
	DWORD dwFlags;
	char pszString[ MAX_XML_STRING_LENGTH ];
	float pfParams[ AI_BEHAVIOR_NUM_PARAMS ];
	AI_BEHAVIOR_DEFINITION_TABLE tTable;
};

struct AI_DEFINITION
{
	DEFINITION_HEADER tHeader;
	AI_BEHAVIOR_DEFINITION_TABLE tTable;
};

//----------------------------------------------------------------------------
// instance structures
//----------------------------------------------------------------------------
struct AI_BEHAVIOR_INSTANCE_TABLE
{
	struct AI_BEHAVIOR_INSTANCE * pBehaviors;
	int nCurrentBehavior;
	int nBehaviorCount;
};

struct AI_BEHAVIOR_INSTANCE
{
	int nGUID;
	const AI_BEHAVIOR_DEFINITION * pDefinition;
	AI_BEHAVIOR_INSTANCE_TABLE * pTable;
};

struct AI_INSTANCE
{
	AI_BEHAVIOR_INSTANCE_TABLE tTable;
	struct AI_CONTEXT * pContext;
};

//----------------------------------------------------------------------------
// Context
//----------------------------------------------------------------------------

struct AI_CONTEXT_INSERTED_BEHAVIORS
{
	const AI_BEHAVIOR_DEFINITION *	pInsertedBehavior;
	int								nTimeout;
	int								nGUID;
};

#define MAX_CONTEXT_TARGETS 5
#define MAX_AI_BEHAVIOR_TREE_DEPTH 5
struct AI_CONTEXT
{
	struct TARGET
	{
		UNIT * pUnit;
		int nSkill;
	};
	// these help different behaviors to cache calculations
	TARGET pTargets[ MAX_CONTEXT_TARGETS ];
	int nTargetCount;

	// these help with running the loop
	BOOL bStopAI;
	BOOL bAddAIEvent;
	int nBranchTo;
	int nPeriodOverride;

	AI_INSTANCE * pAIInstance;
	int pnStack[ MAX_AI_BEHAVIOR_TREE_DEPTH ];
	int nStackCurr;

	AI_CONTEXT_INSERTED_BEHAVIORS *	pInsertedBehaviors;
	int								nInsertedBehaviorCount;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

typedef void AI_START_FUNC( GAME* pGame, UNIT* pUnit, int nAiIndex );
typedef BOOL AI_DO_FUNC( GAME* pGame, UNIT* pUnit, UNIT* target, int nAiIndex );

#define DECLARE_AI_DO_FUNC(x) BOOL x( GAME* pGame, UNIT* pUnit, UNIT* target, int nAiIndex )
#define DECLARE_AI_START_FUNC(x) void x( GAME* pGame, UNIT* pUnit, int nAiIndex )
	
void AI_RemoveCurrentBehavior( 
	UNIT * pUnit,
	AI_CONTEXT & tContext );

void AI_RemoveBehavior( 
	UNIT * pUnit,
	int nGuid );

void AI_RemoveBehavior( 
	UNIT * pUnit,
	AI_BEHAVIOR_INSTANCE_TABLE * pTable,
	int nIndex,
	AI_CONTEXT * pContext );

void AIGetBehaviorString(
	const AI_BEHAVIOR_DEFINITION * pBehaviorDefinition,
	char * szName,
	int nNameLength,
	BOOL & bBold);

int AIPickRandomChild(
	GAME* pGame,
	AI_BEHAVIOR_INSTANCE_TABLE * pTable);

BOOL AiBehaviorDataExcelPostProcess(
	struct EXCEL_TABLE * table);

//----------------------------------------------------------------------------
// ai.cpp
//----------------------------------------------------------------------------
// this could be removed if excel could access definitions directly
BOOL ExcelAIPostProcess( 
	EXCEL_TABLE * table);

enum
{
	AIGTF_USE_SKILL_RANGE_BIT,
	AIGTF_DONT_DO_TARGET_QUERY_BIT,
	AIGTF_DONT_VALIDATE_RANGE_BIT,
	AIGTF_PICK_NEW_TARGET_BIT,
};

UNIT * AI_GetTarget(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	DWORD * pdwSearchFlags,
	DWORD dwAIGetTargetFlags,
	float fDirectionTolerance = 0.0f,
	const VECTOR * pvLocation = NULL,
	UNIT * pSeekerUnit = NULL);

UNIT* AI_GetNearest(
	GAME * pGame,
	UNIT * pUnit,
	DWORD * pdwSearchFlags );

UNIT* AI_ShouldBeAwake(
	GAME * pGame,
	UNIT * pUnit,
	DWORD * pdwSearchFlags );

BOOL AIIsBusy(
	GAME * pGame,
	UNIT * pUnit );

AI_DEFINITION * AIGetDefinition(
	AI_INIT * pAiData);

//----------------------------------------------------------------------------
// aimove.cpp
//----------------------------------------------------------------------------

void AI_TurnTowardsTargetId(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pTarget,
	BOOL bImmediate = FALSE,
	BOOL bFaceAway = FALSE);

void AI_TurnTowardsTargetXYZ(
	GAME * pGame,
	UNIT * pUnit,
	const VECTOR & vTarget,
	const VECTOR & vUp,
	BOOL bImmediate = FALSE);

BOOL AI_MoveRequest(
	GAME * pGame,
	UNIT * pUnit,
	MOVE_REQUEST_DATA & tMoveRequest,
	const char *pszLabel );

BOOL CanMoveInDirection(
	GAME* pGame,
	UNIT* unit,
	VECTOR & vDirection );

int AICopyPathToMessageBuffer(
	GAME* pGame,
	UNIT* pUnit,
	BYTE* pBuffer,
	BOOL bEmptyPath = FALSE);

BOOL AI_FindPathNode(
	GAME * pGame,
	UNIT * pUnit,
	VECTOR & vDestination,
	UNIT * pTargetUnit = NULL);

BOOL AI_FindPathNode(
	GAME * pGame,
	UNIT * pUnit,
	const VECTOR & vDestination,
	struct FREE_PATH_NODE & tNearestPathNode,
	UNIT * pTargetUnit = NULL);

float AIGetAltitude( 
	GAME * pGame, 
	UNIT * pUnit );

BOOL AIBehaviorPostProcess(
	void * pDef,
	BOOL bForceSyncLoad);

//----------------------------------------------------------------------------
// aimissiles.cpp
//----------------------------------------------------------------------------

DECLARE_AI_DO_FUNC(AI_ChargedBolt);
DECLARE_AI_DO_FUNC(AI_ChargedBoltNova);
DECLARE_AI_DO_FUNC(AI_Peacemaker);
DECLARE_AI_DO_FUNC(AI_ArcJumper);
DECLARE_AI_DO_FUNC(AI_AssassinBugHive);
DECLARE_AI_DO_FUNC(AI_FirebrandPod);
DECLARE_AI_DO_FUNC(AI_ScarabBall);
DECLARE_AI_DO_FUNC(AI_HandGrappler);
DECLARE_AI_DO_FUNC(AI_EelLauncher);
DECLARE_AI_DO_FUNC(AI_FireBeetles);
DECLARE_AI_DO_FUNC(AI_SimpleHoming);
DECLARE_AI_DO_FUNC(AI_BlessedHammer);
DECLARE_AI_DO_FUNC(AI_RandomMovement);
DECLARE_AI_DO_FUNC(AI_ThrowSword);
DECLARE_AI_DO_FUNC(AI_ThrowSwordHoming);
DECLARE_AI_DO_FUNC(AI_Artillery);
DECLARE_AI_DO_FUNC(AI_ProximityMine);
DECLARE_AI_DO_FUNC(AI_ThrowShield);
DECLARE_AI_DO_FUNC(AI_ThrowShieldHoming);
DECLARE_AI_DO_FUNC(AI_AttackLocation);
DECLARE_AI_START_FUNC(AI_AssassinBugHiveInit);
DECLARE_AI_START_FUNC(AI_ScarabBallInit);
DECLARE_AI_START_FUNC(AI_ArtilleryInit);

#endif
