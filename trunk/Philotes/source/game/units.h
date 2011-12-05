//----------------------------------------------------------------------------
// units.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#if defined(_MSC_VER)
#pragma once
#endif
#ifdef	_UNITS_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit (slows down compile, so fix this if you get a chance!!!")
#else
#define _UNITS_H_


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _GAME_H_
#include "game.h"
#endif

#ifndef _GAMECLIENT_H_
#include "gameclient.h"
#endif

#ifndef _TARGET_H_
#include "target.h"
#endif

#ifndef _EVENTS_H_
#include "events.h"
#endif

#ifndef _STATS_H_
#include "stats.h"
#endif

#ifndef _UNITTYPES_H_
#include "unittypes.h"
#endif

#ifndef _ROOM_H_
#include "room.h"
#endif

#ifndef _UNITEVENT_H_
#include "unitevent.h"
#endif

#ifndef __CURRENCY_H_
#include "Currency.h"
#endif

#ifndef _PROXMAP_H_
#include "ProxMap.h"
#endif

#ifndef _CLIENTS_H_
#include "clients.h"
#endif

#ifndef __POINTSOFINTEREST_H_
#include "PointsOfInterest.h"
#endif
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
//	#define _DEBUG_KNOWN_STUFF
//	#define _DEBUG_KNOWN_STUFF_TRACE
#endif

//----------------------------------------------------------------------------
// FORWARDS
//----------------------------------------------------------------------------
struct UIX_TEXTURE;
struct CLIENT_SAVE_FILE;
struct RECIPE_PROPERTY_VALUES;
class MYTHOS_POINTSOFINTEREST::cPointsOfInterest;
//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef enum UNITMODE;

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
//#define TRACE_SYNCH
#define GLOBAL_UNIT_TRACKING			(ISVERSION(DEVELOPMENT) || ISVERSION(SERVER_VERSION))


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	MAX_UNITMODES =					256,
	NUM_UNIT_QUALITIES =			64,		// normal, magic, rare, unique...
	NUM_UNIT_PROPERTIES =			5,
	NUM_UNIT_SKILL_SCRIPTS =		3,
	NUM_UNIT_PERLEVEL_PROPERTIES =	2,
	NUM_UNIT_COMBAT_RESULTS =		6,
	NUM_UNIT_START_SKILLS =			12,
	NUM_SKILL_MISSED =				15,
};

//----------------------------------------------------------------------------
enum UNIT_FLAGS
{
	UNITFLAG_JUSTCREATED,
	UNITFLAG_JUSTFREED,
	UNITFLAG_FREED,						// used by UnitFree() to prevent re-entry
	UNITFLAG_ONFREELIST,
	UNITFLAG_DEAD_OR_DYING,
	UNITFLAG_PLAYER_WAITING_FOR_RESPAWN,
	UNITFLAG_ROOMCOLLISION,
	UNITFLAG_RAYTESTCOLLISION,
	UNITFLAG_COLLIDABLE,
	UNITFLAG_CANBEPICKEDUP,
	UNITFLAG_AUTOPICKUP,
	UNITFLAG_CANPICKUPSTUFF,
	UNITFLAG_CANGAINEXPERIENCE,
	UNITFLAG_GIVESLOOT,
	UNITFLAG_SKIPFIRSTSTEP,
	UNITFLAG_ONGROUND,
	UNITFLAG_CANBELOCKEDON,
	UNITFLAG_TRIGGER,
	UNITFLAG_BLOCKING,
	UNITFLAG_AI_INIT,
	UNITFLAG_ON_DIE_DESTROY,		// when unit is killed, it is "immediately" destroy (after a 1 tick delay)
	UNITFLAG_ON_DIE_END_DESTROY,	// when unit finishes its death animation it will be removed.
	UNITFLAG_ON_DIE_HIDE_MODEL,	// UnitDie() will hide the the visual model
	UNITFLAG_DESTROY_DEAD_NEVER,
	UNITFLAG_DELAYED_FREE,
	UNITFLAG_DO_DELAYED_FREE_ON_DEACTIVATE,	// do a delayed free when this unit is deactivated
	UNITFLAG_DONT_STEP_JUST_INTERPOLATE,	
	UNITFLAG_QUESTSTORY,
	UNITFLAG_HASPATH,
	UNITFLAG_CLIENT_PLAYER_MOVING,
	UNITFLAG_CANAIM,			// can use animations for aiming
	UNITFLAG_TURN_WITH_MOVEMENT,			
	UNITFLAG_FACE_SKILL_TARGET, 
	UNITFLAG_ALWAYS_AIM_AT_TARGET,
	UNITFLAG_USE_APPEARANCE_MUZZLE_OFFSET,
	UNITFLAG_DONT_PLAY_IDLE,
	UNITFLAG_ROOMASSERT,		// assert just once if my room gets set to NULL
	UNITFLAG_MISSILETAG,
	UNITFLAG_REGENCHANGE,		// set by stat regen to tell regentarget call back to ignore
	UNITFLAG_JUSTWARPEDIN,		// set by a trigger when you change levels and appear on the "same" trigger on a different level
	UNITFLAG_DONT_SET_WORLD,	// some missiles and things use attachments to set the world matrix.  The unit system shouldn't overwrite it
	UNITFLAG_ATTACHED,			// when a missile or something is attached to a unit
	UNITFLAG_DONT_COLLIDE_NEXT_STEP,	// some missiles need to change direction, but they don't want to collide for two frames in a row.  so don't collide once
	UNITFLAG_KNOCKSBACK_WITH_MOVEMENT,	// missiles knock back the target in the direction that they are moving - other units don't
	UNITFLAG_STEPLIST_REMOVE_REQUESTED,	// Unit has requested that it be removed from the step list
	UNITFLAG_APPEARANCE_LOADED,			// the appearance has been loaded - don't try to reapply the modes
	UNITFLAG_IN_LIMBO,					// This unit is in limbo
	UNITFLAG_DONT_TRIGGER_BY_PROXIMITY,	// Don't trigger this object by proximity, it must be triggered actively
	UNITFLAG_PATHING,
	UNITFLAG_FOOTSTEP_LEG,
	UNITFLAG_WALLWALK,
	UNITFLAG_NOSYNCH,
	UNITFLAG_CLIENT_ONLY,
	UNITFLAG_NO_MESSAGE_SEND,	// do not send unit messages to anybody (presumably we are being created and aren't ready to be sent anywhere)
	UNITFLAG_WARDROBE_CHANGED,	// items in the inventory have moved around that cause wardrobe changes
	UNITFLAG_HOTKEYS_CHANGED,	// items in the inventory have moved around affecting hotkeys
	UNITFLAG_CAN_REQUEST_LEFT_HAND,	// items in the inventory have moved around affecting hotkeys
	UNITFLAG_DONT_COLLIDE_TOWN_PORTALS,	// touching town portals does nothing with this flag on
	UNITFLAG_DONT_ALLOW_PARTY_ACTIONS,
	UNITFLAG_JUMPING,
	UNITFLAG_BOUNCE_OFF_CEILING,
	UNITFLAG_WAITING_FOR_SERVER_INV_MOVE,
	UNITFLAG_TIMESLOW_CONTROLLER,
	UNITFLAG_TEMPORARILY_DISABLE_PATHING,
	UNITFLAG_BEING_REPLACED,
	UNITFLAG_STATE_GRAPHICS_APPLIED,	// states that required the graphics to be loaded have done their events 	
	UNITFLAG_OFFHAND,
	UNITFLAG_NOMELEEBLEND,
	UNITFLAG_DONT_DEACTIVATE_WITH_ROOM,			// unit isn't deactivated with room (still gets deactivated w/ level)
	UNITFLAG_DONT_DEPOPULATE,					// unit isn't depopulated
	UNITFLAG_USING_APPEARANCE_OVERRIDE,		
	UNITFLAG_SAFE_ON_LIMBO_EXIT,	
	UNITFLAG_NOSAVE,			// this is a unit that is just created for temporary use.  It should not be saved for file or database
	UNITFLAG_TEMP_WEAPONCONFIG,	// this is an item that was created for temporary use in a weapon config.  It should be removed when the state which created it is cleared
	UNITFLAG_QUEST_GIVER_ITEM,	// when this unit kills something, a message will be sent to the quests of this unit which can activated by picking up a dropped item and using it
	UNITFLAG_QUEST_GIVER_ITEM_SKIP_UPDATE,// skip the check of all the quests to see if should be set in the next s_QuestsUpdate
	UNITFLAG_QUEST_PARTY_KILL,			// when this unit, or a party member, kills something, a message will be sent the active quests of this unit
	UNITFLAG_QUESTS_UPDATE,
	UNITFLAG_ALLOW_NON_NODE_PATHING,
	UNITFLAG_IGNORE_COLLISION,			// UNIT should ignore all collisions - this occures in movement and after homing. 
	UNITFLAG_DISABLE_CLIENTSIDE_PATHING,	// UNIT should not try to synch to the last known good server location
	UNITFLAG_IN_DATABASE,					// unit is in the Ping0 character database system
	UNITFLAG_NO_DATABASE_OPERATIONS,		// unit can't do database ops (yet)
	UNITFLAG_NEED_INITIAL_DATABASE_ADD,		// is a newly created player and needs to be put in the database
	UNITFLAG_DONT_CHECK_LEVELUP,			// unit is currently in the process of leveling up, don't check it
	UNITFLAG_ON_TRAIN,						// unit is riding on a train car
	UNITFLAG_DEACTIVATED,					// unit is currently deactivated
	UNITFLAG_REREGISTER_EVENTS_ON_ACTIVATE,	// set to force call to s_sUnitActivateRegisterEvents() on unit activate
	UNITFLAG_FACE_DURING_INTERACTION,
	UNITFLAG_IN_NOSAVE_INVENOTRY_SLOT,		// cached value for a unit being in a "no save" inv slot
	UNITFLAG_NO_CLIENT_FIXUP,				// temporarily disable client-side fixup to last known good server location
	UNITFLAG_DISABLE_QUESTS,
	UNITFLAG_IGNORE_STAT_MAX_FOR_XFER,		// doesn't test stats against the max, only during a full unit transfer
	UNITFLAG_SLIDE_TO_DESTINATION,			// slide to our path destination
	UNITFLAG_SHOW_LABEL_WHEN_DEAD,			// display the world space name label when a monster is dead
	UNITFLAG_ALLOW_PLAYER_ROOM_CHANGE,		// a player unit cannot have a room until this is set
	UNITFLAG_HAS_BEEN_HIT,					// set this bit if the unit has been hit (for 1st hit interrupt)		
	UNITFLAG_IS_DECOY,						// This unit is a decoy, and should not do damage
	UNITFLAG_RETURN_FROM_PORTAL,			// Player is returning from a portal
	UNITFLAG_INVENTORY_LOADING,				// The inventory is currently being loaded, so many item transactions will occur in sequence
	UNITFLAG_LOAD_SHARED_STASH,				// this unit needs their shared stash loaded
	UNITFLAG_ABOUT_TO_BE_FREED,				// this unit is about to be freed
	UNITFLAG_NEEDS_DATABASE_UPDATE,			// this unit would like to be updated in the database
	UNITFLAG_FORCE_FADE_ON_DESTROY,			// force this unit to fade when it is destroyed
	UNITFLAG_SHOULD_RESPAWN,				// this monsters respawns later after death
	UNITFLAG_RESPAWN_EXACT,					// this monsters respawns later, EXACTLY as is

	NUM_UNITFLAGS,
};

enum
{
	MOVEFLAG_TEST_ONLY,
	MOVEFLAG_FORCE_GROUND,
	MOVEFLAG_FLY,
	MOVEFLAG_SOLID_MONSTERS,
	MOVEFLAG_FLOOR_ONLY,
	MOVEFLAG_NO_GEOMETRY,
	MOVEFLAG_ONLY_ON_GROUND_SOLID,
	MOVEFLAG_NOT_WALLS,
	MOVEFLAG_PLAYER_ONLY,
	MOVEFLAG_DOORS,
	MOVEFLAG_MONSTER_MOTION,
	MOVEFLAG_UPDATE_POSITION,
	MOVEFLAG_USE_DESIRED_MELEE_RANGE,
	MOVEFLAG_NOT_REAL_STEP,
	NUM_MOVEFLAGS,

	MOVEMASK_TEST_ONLY =				MAKE_MASK(MOVEFLAG_TEST_ONLY),
	MOVEMASK_FORCEGROUND =				MAKE_MASK(MOVEFLAG_FORCE_GROUND),
	MOVEMASK_FLY =						MAKE_MASK(MOVEFLAG_FLY),
	MOVEMASK_SOLID_MONSTERS =			MAKE_MASK(MOVEFLAG_SOLID_MONSTERS),
	MOVEMASK_FLOOR_ONLY =				MAKE_MASK(MOVEFLAG_FLOOR_ONLY),
	MOVEMASK_NO_GEOMETRY =				MAKE_MASK(MOVEFLAG_NO_GEOMETRY),
	MOVEMASK_ONLY_ONGROUND_SOLID =		MAKE_MASK(MOVEFLAG_ONLY_ON_GROUND_SOLID),
	MOVEMASK_NOT_WALLS =				MAKE_MASK(MOVEFLAG_NOT_WALLS),
	MOVEMASK_PLAYER_ONLY =				MAKE_MASK(MOVEFLAG_PLAYER_ONLY),
	MOVEMASK_DOORS =					MAKE_MASK(MOVEFLAG_DOORS),
	MOVEMASK_MONSTER_MOTION =			MAKE_MASK(MOVEFLAG_MONSTER_MOTION),
	MOVEMASK_UPDATE_POSITION =			MAKE_MASK(MOVEFLAG_UPDATE_POSITION),
	MOVEMASK_USE_DESIRED_MELEE_RANGE =	MAKE_MASK(MOVEFLAG_USE_DESIRED_MELEE_RANGE),
	MOVEMASK_NOT_REAL_STEP =			MAKE_MASK(MOVEFLAG_NOT_REAL_STEP),
};

enum
{
	RESERVE_LOCATION_FAILED = 0,
	RESERVE_LOCATION_OKAY,
	RESERVE_LOCATION_CHANGED,
};

enum
{
	UNIT_LOCRESERVE_NONE = 0,
	UNIT_LOCRESERVE_XYZ,
	UNIT_LOCRESERVE_TARGET,
};

enum UNIT_INTERACTIVE
{
	UNIT_INTERACTIVE_DATA = 0,		// interactive or not from UNIT_DATA
	UNIT_INTERACTIVE_ENABLED,		// override a unit to interactive
	UNIT_INTERACTIVE_RESTRICED,		// override a unit to not be interactive
};

enum
{
	MODE_VELOCITY_WALK = 0,
	MODE_VELOCITY_STRAFE,
	MODE_VELOCITY_JUMP,
	MODE_VELOCITY_RUN,
	MODE_VELOCITY_BACKUP,
	MODE_VELOCITY_KNOCKBACK,
	MODE_VELOCITY_MELEE,
	NUM_MODE_VELOCITIES 
};


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// EXPORT
//----------------------------------------------------------------------------
extern const VECTOR INVALID_POSITION;
extern const VECTOR INVALID_DIRECTION;


#define WEAPON_BLEND_THRESHHOLD	.6f
#define MAX_TURN_RATE			512
#define MAX_ALLOWED_Z_VELOCITY_MAGNITUDE 20.0f
//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

struct UNITGFX
{
	int						nModelIdCurrent;		// currently drawn model id - first or third
	int						nModelIdInventory;		// instance id for inventory mesh
	int						nModelIdInventoryInspect;// instance id for inventory inspect
	int						nModelIdPaperdoll;		// 
	int						nModelIdFirstPerson;	// first person model
	int						nModelIdThirdPerson;	// third person model - default model for most units
	int						nModelIdThirdPersonOverride;

	int						nAppearanceDefIdForCamera; // alternate camera for model view
};



//----------------------------------------------------------------------------
struct UNIT
{
	// put vectors first, etc.
	GAME *							pGame;
	const struct UNIT_DATA *		pUnitData;
	const struct UNIT_DATA *		pAppearanceOverrideUnitData;
	const struct UNIT_DATA *		pSourceAppearanceUnitData;
	WCHAR *							szName;
	UNIT *							hashprev;			// links units in hash buckets for hash by unitid
	UNIT *							hashnext;
	UNIT *							guidprev;			// links units in hash buckets for hash bu guid
	UNIT *							guidnext;
	UIX_TEXTURE *					pIconTexture;
	class CPath	*					m_pActivePath;		// For tugboat flippies
	ROOM *							pRoom;
	struct AI_INSTANCE *			pAI;
	struct UNITSTATS *				stats;
	struct EVENT_HANDLER *			m_pEventHandlers[NUM_UNIT_EVENTS];
	struct EVENT_HANDLER *			m_pEventHandlerPending;
	struct ROOM_CPOLY *				pLastCollide;		// last walk mesh triangle we collided with
	struct INVENTORY_NODE *			invnode;
	struct INVENTORY *				inventory;
	int								nWardrobe;
	struct APPEARANCE_SHAPE *		pAppearanceShape;
	UNIT *							stepprev;			// pStepList list of units in GAME
	UNIT *							stepnext;
	UNIT *							m_FreeListNext;		// next item on free list
	struct PATH_STATE *				m_pPathing;
	UNITGFX *						pGfx;
	struct SKILL_INFO *				pSkillInfo;
	struct UNIT_TAG_STRUCT *		m_pUnitTags;
	struct QUEST_GLOBALS *			m_pQuestGlobals;	// quest states for this player
	struct PLAYER_MUSIC_INFO *		pMusicInfo;
	struct PLAYER_MOVE_INFO *		pPlayerMoveInfo;
	struct RECIPE_PROPERTY_VALUES *	pRecipePropertyValues;	//for recipe properties on for player
	DWORD *							pdwStateFlags;
	struct EMAIL_SPEC *				pEmailSpec;
	SIMPLE_DYNAMIC_ARRAY<PGUID>		tEmailAttachmentsBeingLoaded;

#ifdef HAVOK_ENABLED
	class hkRigidBody *				pHavokRigidBody;
	// the following is used for helping the client physics
	struct HAVOK_IMPACT *			pHavokImpact;
#endif

	PGUID							guid;				// guid of unit, valid forever

	GAME_EVENT						events;
	TARGET_NODE						tRoomNode;
	// units' positions are recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
	PROXNODE						LevelProxMapNode;
	

	VECTOR							vPosition;			// position in game
	VECTOR							vDrawnPosition;		// position drawn for interpolation on client
	VECTOR							vUpDirection;
	VECTOR							vFaceDirection;
	VECTOR							vFaceDirectionDrawn;
	VECTOR							vQueuedFaceDirection;
	VECTOR							vMoveDirection;
	VECTOR							pvWeaponPos[MAX_WEAPONS_PER_UNIT];
	VECTOR							pvWeaponDir[MAX_WEAPONS_PER_UNIT];
	VECTOR							vMoveTarget;

	BOUNDING_BOX					tReservedLocation;

	float							fUnitSpeed;
	float							m_fPathDistance;
	float							fDropPosition;		// For tugboat flippies
	float							fScale;
	float							fVelocityFromAcceleration; // accumulated acceleration
	float							fAcceleration;
	float							fVelocity;		// actual velocity of the unit (adjusted by velocity stat)
	float							fZVelocity;		// used for jumping

	GAME_TICK						tiLastGameTickWarped;	// basic tracking on warping.
	SPECIES							species;
	UNITID							unitid;				// dword id of unit, valid for game instance only
	UNITID							idOwner;
	UNITID							idLastUnitHit;	// keep track of last hit unit on server to prevent
	UNITID							idMoveTarget;
	GAMECLIENTID					idClient;
	TARGET_TYPE						eTargetType;
	DWORD							pdwFlags[DWORD_FLAG_SIZE(NUM_UNITFLAGS)];
	DWORD							pdwMovementFlags[DWORD_FLAG_SIZE(NUM_MOVEFLAGS)];
	DWORD							pdwModeFlags[DWORD_FLAG_SIZE(MAX_UNITMODES)];
	DWORD							dwRoomIterateSequence;	// mark placed in units when iterating them, only good for one level of iteration, we can make this a small array if we really need the room/unit iteration to be re-entrant
	DWORD							dwTestCollide;
	int								nUnitType;
	int								nAppearanceDefId;
	int								nAppearanceDefIdOverride;
	int								m_nEventHandlerIterRef;
	int								nPendingMode;
	int								nTeam;
	int								nTeamOverride;
	int								nReservedType;
	int								nPlayerAngle;			
#if ISVERSION(DEVELOPMENT)
	int								m_nEventHandlerCount;
#endif

	// tugboat-specific
	BOOL							bVisible;	// for tugboat line-of-sight
	BOOL							bStructural;// for tugboat line-of-sight
	BOOL							bDropping;			// For tugboat flippies
	MYTHOS_POINTSOFINTEREST::cPointsOfInterest				*m_pPointsOfInterest;			//Points of Interest
	
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL ExcelUnitPostProcessCommon( 
	struct EXCEL_TABLE * table);

// initialize unit hash (by id)
void UnitHashInit(
	GAME * game);

void UnitHashFree(
	GAME * game);

UNIT * UnitGetById(
	GAME * game,
	UNITID unitid);

UNIT * UnitGetByGuid(
	GAME * game,
	PGUID guid);

void UnitPostCreateDo(
	GAME * game,
	UNIT * unit);

void s_UnitActivate(
	UNIT * unit);

BOOL s_UnitClassCanBeDeactivated(
	const UNIT_DATA * unit_data);

BOOL s_UnitClassCanBeDepopulated(
	const UNIT_DATA * unit_data);

BOOL s_UnitCanBeDepopulated(
	UNIT * unit);

BOOL s_UnitCanBeDeactivatedEver(
	UNIT * unit);

BOOL s_UnitCanBeDeactivatedWithRoom(
	UNIT * unit);

void s_UnitDeactivate(
	UNIT * unit,
	BOOL bDeactivateLevel);

#if GLOBAL_UNIT_TRACKING
void GlobalUnitTrackerActivate(
	SPECIES species);

void GlobalUnitTrackerDeactivate(
	SPECIES species);

void GlobalUnitTrackerTrace(
	void);
#else
#define GlobalUnitTrackerActivate(s)
#define GlobalUnitTrackerDeactivate(s)
#define GlobalUnitTrackerTrace(s)
#endif


//----------------------------------------------------------------------------
enum UNIT_ITERATE_RESULT
{
	UIR_CONTINUE,
	UIR_STOP
};
typedef UNIT_ITERATE_RESULT (*PFN_UNIT_ITERATE_CALLBACK)( UNIT *pUnit, void *pCallbackData );

void UnitIterateUnits(
	GAME *pGame,
	PFN_UNIT_ITERATE_CALLBACK pfnCallback,
	void *pCallbackData);

void UnitCreateIconTexture( GAME* game,
							UNIT* unit );

void UnitRecreateIcons(
	GAME *pGame );

void UnitClearIcons(
	GAME *pGame );
	
UNIT * UnitGetFirstByClassSearchAll(
	GAME *pGame,
	GENUS eGenus,
	int nUnitClass);

void UnitInitStatsChangeCallbackTable(
	void);


//----------------------------------------------------------------------------
enum UNIT_FREE_FLAGS
{
	UFF_SEND_TO_CLIENTS		= MAKE_MASK( 0 ),		// send free to clients who care
	UFF_DELAYED				= MAKE_MASK( 1 ),
	UFF_FADE_OUT			= MAKE_MASK( 2 ),
	UFF_SWITCHING_INSTANCE	= MAKE_MASK( 3 ),		// unit is switching instances
	UFF_ROOM_BEING_FREED	= MAKE_MASK( 4 ),		// free the unit because either the room or hash or some such is being freed
	UFF_HASH_BEING_FREED	= MAKE_MASK( 5 ),
	UFF_TRUE_FREE			= MAKE_MASK( 6 ),
};

enum UNIT_FREE_CONTEXT	// this is used for player transaction logging
{
	UFC_NONE,
	UFC_ITEM_DROP,
	UFC_ITEM_DISMANTLE,
	UFC_ITEM_DESTROY,
	UFC_ITEM_UPGRADE,
	UFC_LOST,
};

void UnitFree(
	UNIT* unit,
	DWORD dwFlags = 0,  // see UNIT_FREE_FLAGS
	UNIT_FREE_CONTEXT eContext = UFC_NONE);

void UnitSendRemoveMessage(
	UNIT *pUnit,
	DWORD dwUnitFreeFlags,
	BOOL bFree);

EXCELTABLE UnitGetDatatableEnum(
	UNIT* unit);

EXCELTABLE UnitGenusGetDatatableEnum(
	GENUS eGenus);

const char * ExcelTableGetDataFolder(
	unsigned int idTable);

void UnitSetDefaultTargetType(
	GAME * game,
	UNIT * unit,
	const struct UNIT_DATA * unit_data = NULL);

void UnitChangeTargetType(
	UNIT * unit,
	TARGET_TYPE eTargetType);

void UnitRemoveFromRoomList( 
	UNIT *pUnit,
	bool bRemoveFromLevelProxMap ); // leave the unit in the level's prox map if unit is not changing levels

void UnitUpdateRoom(
	UNIT* unit,
	ROOM* room,
	BOOL bDoNetworkUpdate);

ROOM * UnitUpdateRoomFromPosition(
	GAME * game,
	UNIT * unit,
	const VECTOR * oldpos = 0 /*optional*/, // if the unit has not move, we will not check for a new room
	bool bAssertOnNoRoom = true);

void c_UnitRemove(
	GAME *pGame,
	UNITID idUnit,
	DWORD dwUnitFreeFlags );
	
void UnitRemoveFromWorld(
	UNIT* unit,
	BOOL bSendRemoveMessage);

void UnitSetName(
	UNIT* unit,
	const WCHAR* szName);

void UnitClearName(
	UNIT *pUnit);

void UnitSetNameOverride(
	UNIT *pUnit,
	int nString);

int UnitGetNameOverride(
	UNIT *pUnit);

void c_UnitSetLastKnownGoodPosition(
	UNIT * pUnit,
	const VECTOR & vPosition,
	BOOL bSetOnly = FALSE);

enum UNIT_WARP_FLAGS
{
	UW_FORCE_VISIBLE_BIT = 1,		// force model to be visible after warp
	UW_LEVEL_CHANGING_BIT,
	UW_ITEM_DO_FLIPPY_BIT,			// do flippy logic
	UW_TRIGGER_EVENT_BIT,
	UW_SOFT_WARP,					// only actually perform this warp if the unit is far away
	UW_PATHING_WARP,				// only actually perform this warp if the is more than a very little bit away
	UW_PRESERVE_CAMERA_BIT,			// don't reset camera
	UW_PREMATURE_STOP,				// stop pathing a little early -
};

void UnitWarp(
	UNIT* unit,
	ROOM* room,
	const VECTOR & vPosition,
	const VECTOR & vFaceDirection,
	const VECTOR & vUpDirection,
	DWORD dwUnitWarpFlags,				// see UNIT_WARP_FLAGS
	BOOL bSendToClient = TRUE,
	BOOL bSendToOwner = TRUE,
	LEVELID idLevel = INVALID_ID);

void PathingUnitWarped( 
	UNIT *pUnit,
	int nAIRegisterDelay = 1);

// set the control unit
void GameSetControlUnit(
	GAME * game,
	UNIT* unit);

void UnitSetFaceDirection(
	UNIT* unit,
	const VECTOR& vFaceDirection,
	BOOL bForceDrawnDirection = FALSE,
	BOOL bCheckUpDirection = FALSE,
	BOOL bClearZ = TRUE );

void UnitSetQueuedFaceDirection(
	UNIT* unit,
	const VECTOR& vFaceDirection );

void UnitSetPitch(
	UNIT* unit,
	int pitch);

void UnitSetVisible(
	UNIT* unit,
	BOOL bValue);

void c_UnitSetNoDraw(
	UNIT* unit,
	BOOL bValue,
	BOOL bAllModels = FALSE,
	float fFadeTime = 0.0f,
	BOOL bTemporary = FALSE );

BOOL c_UnitGetNoDraw( UNIT* unit );

BOOL c_UnitGetNoDrawTemporary( UNIT* unit );

BOOL UnitInLineOfSight( GAME *pGame,
						UNIT* pUnit,
						UNIT* pTarget );
void c_UnitSetVisibleAll(
	GAME * game,
	BOOL bValue);

void c_UnitUpdateDrawAll(
	GAME * game,
	float fTimeDelta );

void c_UnitUpdateFaceTarget(
	UNIT * unit,
	BOOL bForce );

void c_UnitUpdateGfx(
	UNIT * unit,
	BOOL bUpdateLighting = TRUE );

void UnitModelGfxInit(
	GAME * pGame,
	UNIT *pUnit,
	EXCELTABLE eTable,
	int nClass,
	int nModelId);

void StatsChangeCallback(
	UNIT * unit,
	WORD wStat,
	PARAM dwParam,
	int newvalue,
	int oldvalue);

UNIT * TestUnitUnitCollision(
	UNIT * unit,
	const VECTOR * pvPos,
	const VECTOR * pvPrevPos,
	DWORD dwTestMask);

BOOL RayIntersectsUnitCylinder(
	const UNIT * pUnit,
	const VECTOR & vRayStart,
	const VECTOR & vRayEnd,
	const float fRayThickness,
	const float fFudgeFactor,
	VECTOR * pvPointAlongRay = NULL);

void StatsChangeCallback(
	UNIT * unit,
	WORD wStat,
	PARAM dwParam,
	int newvalue,
	int oldvalue);

void UnitInitValueTimeTag(
	GAME* pGame, 
	UNIT* unit, 
	int nTag );

//----------------------------------------------------------------------------
enum UNIT_NAME_FLAGS
{
	UNF_EMBED_COLOR_BIT,
	UNF_BASE_NAME_ONLY_BIT,
	UNF_INCLUDE_TITLE_BIT,
	UNF_NAME_NO_MODIFICATIONS,
	UNF_VERBOSE_WARPS_BIT,			// verbose warp information
	UNF_VERBOSE_HEADSTONES_BIT,		// display headstone names in the form of "<PLAYERNAME>'s Headstone"
	UNF_INCLUDE_ITEM_TOKEN_WITH_ITEM_QUALITY,		// for item quality, use the one with the [item] token in the string "Enhanced [item]"
	UNF_OVERRIDE_ITEM_NOUN_ATTRIBUTES,		// for item quality, use the passed in attributes instead of getting them from the item noun
};

BOOL UnitGetName(
	UNIT* unit,
	WCHAR* szName,
	int len,
	DWORD dwUnitNameFlags,				// see UNIT_NAME_FLAGS
	DWORD *pdwColor = NULL);

BOOL UnitHasMagicName(
	UNIT* unit);

BOOL UnitGetClassString(
	UNIT* unit,
	WCHAR* szName,
	int len);

BOOL UnitGetTypeString(
	UNIT* unit,
	WCHAR* szType,
	int len,
	BOOL bLastOnly = FALSE,
	DWORD *pdwAttributesOfTypeString = NULL);

BOOL ItemGetClassReqString(
	UNIT* unit,
	WCHAR* szReqString,
	int len);

BOOL ItemGetFactionReqString(
	UNIT* unit,
	WCHAR* szReqString,
	int len);

BOOL ItemGetQualityString(
	UNIT *unit,
	WCHAR *uszBuffer,
	int nBufferLen,
	DWORD dwUnitNameFlags,	// see UNIT_NAME_FLAGS
	DWORD dwStringAttributesToMatch = -1);

BOOL UnitGetFlavorTextString(
	UNIT* unit,
	WCHAR* szType,
	int len);

int UnitGetNameColor(
	UNIT* unit);

BOOL UnitGetAffixDescription(
	UNIT* unit,
	WCHAR* szDesc,
	int nDescLen);

BOOL UnitGetResistanceDescription(
	UNIT* unit,
	WCHAR* szDesc,
	int nDescLen);
BOOL UnitIsPhysicallyInContainer(
	UNIT *pUnit);

BOOL s_SendInventory(
	GAMECLIENT *pClient,
	UNIT *pUnit);

BOOL s_SendUnitToClient(
	UNIT *pUnit,
	struct GAMECLIENT *pClient,
	DWORD dwKnownFlags = 0,		// see UNIT_KNOWN_FLAGS
	const struct NEW_MISSILE_MESSAGE_DATA *pNewMissileMessageData = NULL);
	
BOOL s_SendUnitToAllWithRoom(
	GAME * game,
	UNIT* unit,
	ROOM* room);

BOOL s_RemoveUnitFromClient( 
	UNIT *pUnit, 
	GAMECLIENT *pClient,
	DWORD dwUnitFreeFlags );

void c_UnitGetDataFolder(
	GAME * game,
	UNIT* pUnit,
	char* pszFolder,
	int nBufLen);

#ifdef HAVOK_ENABLED
void UnitAddImpact(
	GAME * game,
	UNIT* pUnit,
	const HAVOK_IMPACT * pImpact);
#endif

void UnitApplyImpacts(
	GAME * game,
	UNIT* pUnit);

void UnitDrop(
	GAME* game,
	UNIT* pUnit,
	const VECTOR& vPosition );

float UnitGetModeDuration(
	GAME * game, 
	UNIT* unit, 
	UNITMODE eMode,
	BOOL & bHasMode, 
	BOOL bIgnoreBackups = FALSE );

float UnitGetModeDurationScaled(
	GAME* game, 
	UNIT* unit, 
	UNITMODE eMode,
	BOOL & bHasMode );

BOOL UnitHasMode(
	GAME * game, 
	UNIT* unit, 
	UNITMODE eMode );

BOOL UnitHasOriginalMode(
	GAME * game, 
	UNIT* unit, 
	UNITMODE eMode );

void UnitCalcVelocity(
	UNIT* unit);

int UnitGetAnimationGroup(
	UNIT* unit);

void c_UnitDoDelayedDeath(
	UNIT * pUnit );
	
void c_UnitGetWorldMatrixFor1stPerson(
	GAME * pGame,
	VECTOR & vPosition,
	MATRIX & matWorld );

BOOL UnitInLineOfSight(
   GAME *pGame,
   UNIT* pUnit,
   UNIT* pTarget );

VECTOR UnitGetAimPoint(
	UNIT* unit,
	BOOL bForceOffset = FALSE);

void UnitStartJump(
	GAME * game,
	UNIT * unit,
	float fZVelocity,
	BOOL bSend = TRUE,
	BOOL bRemoveFromStepList = FALSE);

void UnitJumpCancel(
	UNIT *pUnit);

BOOL UnitIsJumping(	
	UNIT *pUnit);
		
int UnitGetLookGroup(
	UNIT* unit);

void c_UnitUpdateViewModel(
	UNIT * pUnit,
	BOOL bForce,
	BOOL bWeaponOnly = FALSE );

int c_UnitGetBloodParticleDef(
	GAME * pGame,
	UNIT * pUnit,
	int nCombatResult );

float c_UnitGetMeleeImpactOffset(
	GAME * pGame,
	UNIT * pUnit);

void c_SinglePlayerSynch(
	GAME * game);

int UnitGetSkillID(		//If the unit is a skill, this will return the skill ID, otherwise INVALID_ID
	UNIT * pUnit);

enum USE_RESULT UnitIsUsable(
	UNIT* unit,
	UNIT* item);

BOOL UnitIsNoDrop(
	UNIT * item );

BOOL UnitAskForPickup(
	UNIT * item );

BOOL UnitIsExaminable(
	UNIT * item );

BOOL UnitIsMerchant(
	UNIT* unit);

int UnitMerchantFactionType(
	UNIT * unit );

int UnitMerchantFactionValueNeeded(
	UNIT * unit );

BOOL UnitMerchantSharesInventory(
	UNIT *pUnit);

BOOL UnitTradesmanSharesInventory(
	UNIT *pUnit);

BOOL UnitIsTrader(
	UNIT *pUnit);

BOOL UnitIsTrainer(
	 UNIT *pUnit);

BOOL UnitIsTradesman(
	UNIT *pUnit);

BOOL UnitIsGambler(
	UNIT *pUnit);

BOOL UnitIsStatTrader(
	UNIT *pUnit);

BOOL UnitIsHealer(
	UNIT* unit);

BOOL UnitIsGravekeeper( 
	UNIT* unit);

BOOL UnitIsTaskGiver(
	UNIT* unit);

BOOL UnitIsGodMessanger(
	UNIT* unit);

BOOL UnitIsItemUpgrader(
	UNIT *pUnit);

BOOL UnitIsItemAugmenter(
	UNIT *pUnit);
	
BOOL UnitAutoIdentifiesInventory(
	UNIT* unit);

BOOL UnitIsDungeonWarper(
	UNIT * unit);

BOOL UnitIsGuildMaster(
	UNIT * unit);

BOOL UnitIsRespeccer(
	UNIT * unit);

BOOL UnitIsCraftingRespeccer(
	UNIT * unit);

BOOL UnitIsTransporter(
	UNIT* unit);

BOOL UnitIsForeman(
	UNIT * unit);
	
BOOL UnitIsPvPSignerUpper(
	UNIT * unit);

const char * UnitGetDevName(
	UNIT * unit);

const char * UnitGetDevName( 
	GAME * game,
	UNITID idUnit);

const char * UnitGetDevNameBySpecies(
	SPECIES species);
	
void UnitSetOwner(
	UNIT * unit,
	UNIT * owner);

BOOL UnitIsInTown(
	UNIT * unit);

BOOL UnitIsInPVPTown(
	UNIT * unit);

BOOL UnitIsInRTSLevel(
	UNIT * unit);

BOOL UnitIsInHellrift(
	UNIT * unit);

BOOL UnitIsInPortalSafeLevel(
	UNIT *pUnit);

BOOL UnitIsNPC(
	const UNIT* unit);

BOOL UnitPvPIsEnabled(
	UNIT *pUnit);

BOOL s_UnitKnockback( 
	GAME * pGame, 
	UNIT * pAttacker,
	UNIT * pDefender, 
	float fDistance );

void UnitSetMoveTarget(
	UNIT* unit,
	UNITID idTarget);

void UnitSetMoveTarget(
	UNIT* unit,
	const VECTOR & vMoveTarget,
	const VECTOR & vMoveDirection);

GAMECLIENT *UnitGetClient(
	const UNIT* unit);

void UnitSetClientID(
	UNIT * unit,
	GAMECLIENTID idClient);

APPCLIENTID UnitGetAppClientID(
	const UNIT *pPlayer);
	
BOOL UnitSetGuid(
	UNIT * unit,
	PGUID guid);

int UnitGetBaseClass(
	GENUS eGenus,
	int nUnitClass);

BOOL UnitIsInHierarchyOf(
	GENUS eGenus,
	int nUnitClass,
	const int *pnUnitClassesApartOf,
	int nApartOfCount);

DWORD UnitGetPlayedTime(
	UNIT * pUnit );

void UnitMakeBoundingBox(
	UNIT * pUnit,
	struct BOUNDING_BOX * pBoundingBox);

BOOL UnitCheckCanRunEvent(
	UNIT * unit);

#if ISVERSION(DEVELOPMENT)
BOOL UnitEmergencyDeactivate(
	UNIT * unit,
	const char * szDebugString);
#else
#define UnitEmergencyDeactivate(u, sz)			UnitEmergencyDeactivateRelease(u)
BOOL UnitEmergencyDeactivateRelease(
	UNIT * unit);
#endif

void ControlUnitLoadInventoryUnitData(
	UNIT * pUnit);


//----------------------------------------------------------------------------
// ACCESSOR FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline UNITID UnitGetId(
	const UNIT* unit)
{
	return (unit ? unit->unitid : INVALID_ID);
}

//----------------------------------------------------------------------------
inline UNIQUE_ACCOUNT_ID UnitGetAccountId(
	const UNIT* unit)
{
	struct GAMECLIENT * gameClient = UnitGetClient(unit);
	if (gameClient)
	{
		NETCLIENTID64 idNetClient = gameClient->m_idNetClient;
		return NetClientGetUniqueAccountId(idNetClient);
	}

	return INVALID_UNIQUE_ACCOUNT_ID;
}

//----------------------------------------------------------------------------
inline struct GAME * UnitGetGame(
	const UNIT* unit)
{
	ASSERT_RETNULL(unit);
	return unit->pGame;
}

//----------------------------------------------------------------------------
inline GAMEID UnitGetGameId(
	const UNIT * unit )
{
	GAME * game = UnitGetGame( unit );
	return ( game ) ? GameGetId( game ) : INVALID_ID;
}

//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define UnitGetClientId(u)		UnitGetClientIdDbg(u, __FILE__, __LINE__)
inline GAMECLIENTID UnitGetClientIdDbg(
	const UNIT * unit,
	const char * file,
	unsigned int line)
#else
inline GAMECLIENTID UnitGetClientId(
	const UNIT * unit)
#endif
{
#if ISVERSION(DEVELOPMENT)
	if (unit && IS_CLIENT(UnitGetGame(unit)))
	{
		ASSERTX(0, "Unit shouldn't have a client ID on the client");
	}
#endif

	GAMECLIENTID idClient = (unit ? unit->idClient : INVALID_GAMECLIENTID);
#if ISVERSION(DEVELOPMENT)
	FL_ASSERT(idClient != INVALID_ID, file, line);
#else
	ASSERT(idClient != INVALID_ID);
#endif
	return idClient;
}

//----------------------------------------------------------------------------
inline BOOL UnitHasClient(
	const UNIT* unit)
{
	ASSERT_RETFALSE(unit);
	return unit->idClient != INVALID_GAMECLIENTID;
}

//----------------------------------------------------------------------------
inline int UnitGetClass(
	const UNIT* unit)
{
	ASSERT_RETINVALID(unit);
	return (GET_SPECIES_CLASS(unit->species));
}

//----------------------------------------------------------------------------
inline GENUS UnitGetGenus(
	const UNIT* unit)
{
	ASSERT_RETVAL(unit, GENUS_NONE);
	return ((GENUS)GET_SPECIES_GENUS(unit->species));
}

//----------------------------------------------------------------------------
inline SPECIES UnitGetSpecies(
	const UNIT* unit)
{
	ASSERT_RETVAL(unit, SPECIES_NONE);
	return unit->species;
}

//----------------------------------------------------------------------------
inline int UnitGetType(
	UNIT* unit)
{
	return unit->nUnitType;
}

//----------------------------------------------------------------------------
inline void UnitSetType(
	UNIT* unit,
	int nUnitType)
{
	ASSERT_RETURN(unit);
	ASSERT( nUnitType != INVALID_ID );
	unit->nUnitType = nUnitType;
}

//----------------------------------------------------------------------------
inline BOOL UnitIsA(
	const UNIT* unit,
	int nUnitType)
{
	ASSERT_RETFALSE(unit);
	if (nUnitType == UNITTYPE_HELLGATE)
	{
		return AppIsHellgate();
	}
	else if (nUnitType == UNITTYPE_TUGBOAT)
	{
		return AppIsTugboat();
	}
	else
	{
		return UnitIsA(unit->nUnitType, nUnitType);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitTypeGetAllEquivTypes(
	int nUnitType,
	SIMPLE_DYNAMIC_ARRAY<int>& arrayUnitParentTypes);

//----------------------------------------------------------------------------
inline void UnitSetFlag(
	UNIT* unit,
	int flag)
{
	ASSERT_RETURN(flag >= 0 && flag < NUM_UNITFLAGS);
	ASSERT_RETURN(unit);
	SETBIT(unit->pdwFlags, flag);
}

//----------------------------------------------------------------------------
inline void UnitSetFlag(
	UNIT* unit,
	int flag,
	BOOL value)
{
	ASSERT_RETURN(flag >= 0 && flag < NUM_UNITFLAGS);
	ASSERT_RETURN(unit);
	SETBIT(unit->pdwFlags, flag, value);
}

//----------------------------------------------------------------------------
inline void UnitClearFlag(
	UNIT* unit,
	int flag)
{
	ASSERT_RETURN(flag >= 0 && flag < NUM_UNITFLAGS);
	ASSERT_RETURN(unit);
	CLEARBIT(unit->pdwFlags, flag);
}

//----------------------------------------------------------------------------
inline BOOL UnitTestFlag(
	const UNIT* unit,
	int flag)
{
	ASSERT_RETFALSE(flag >= 0 && flag < NUM_UNITFLAGS);
	ASSERT_RETFALSE(unit);
	return TESTBIT(unit->pdwFlags, flag);
}

//----------------------------------------------------------------------------
inline TARGET_TYPE UnitGetTargetType(
	UNIT* unit)
{
	ASSERT_RETVAL(unit, TARGET_INVALID);
	return unit->eTargetType;
}

//----------------------------------------------------------------------------
inline DWORD UnitGetCollisionMask(
	UNIT* unit)
{
	ASSERT_RETZERO(unit);
	return unit->dwTestCollide;
}

//----------------------------------------------------------------------------
inline int UnitGetPlayerAngle(
	const UNIT* unit)
{
	ASSERT_RETZERO(unit);
	ASSERT(UnitIsA(unit, UNITTYPE_PLAYER));
	return unit->nPlayerAngle;
}

//----------------------------------------------------------------------------

BOOL UnitGetWeaponPositionAndDirection(
	GAME * game,
	UNIT* unit,
	int nWeaponIndex,
	VECTOR* pvPosition,
	VECTOR* pvDirection);

//----------------------------------------------------------------------------
inline const VECTOR& UnitGetPosition(
	const UNIT* unit)
{
	ASSERT_RETVAL(unit, cgvNone);
	return unit->vPosition;
}

//----------------------------------------------------------------------------
inline struct ROOM * UnitGetRoom(
	const UNIT * unit)
{
	ASSERT_RETNULL(unit);
	return unit->pRoom;
}

//----------------------------------------------------------------------------
inline struct LEVEL* UnitGetLevel(
	const UNIT* unit)
{
	ASSERT_RETNULL(unit);
	if ( ! unit->pRoom )
	{
		return NULL;
	}
	return RoomGetLevel( unit->pRoom );
}

//----------------------------------------------------------------------------
inline int UnitGetRoomId(
	UNIT* unit)
{
	ASSERT_RETINVALID(unit);
	ROOM* room = UnitGetRoom(unit);
	return RoomGetId(room);
}

//----------------------------------------------------------------------------
inline STATS* UnitGetStats(
	UNIT* unit)
{
	ASSERT_RETNULL(unit);
	return (STATS*)unit->stats;
}

//----------------------------------------------------------------------------
inline UNITSTATS* UnitGetUnitStats(
	UNIT* unit)
{
	ASSERT_RETNULL(unit);
	return unit->stats;
}

//----------------------------------------------------------------------------
inline UNITID UnitGetOwnerId(
	UNIT* unit)
{
	if (!unit)
	{
		return INVALID_ID;
	}
	return unit->idOwner;
}

//----------------------------------------------------------------------------
inline const VECTOR* UnitGetMoveTarget(
	UNIT* unit)
{
	if (unit->idMoveTarget != INVALID_ID)
	{
		UNIT* pTarget = UnitGetById(UnitGetGame(unit), unit->idMoveTarget);
		if (pTarget)
		{
			return &UnitGetPosition(pTarget);
		}
		return NULL;
	}
	if (unit->vMoveTarget.fX || unit->vMoveTarget.fY || unit->vMoveTarget.fZ)
	{
		return &unit->vMoveTarget;
	}
	return NULL;
}

//----------------------------------------------------------------------------
inline UNITID UnitGetMoveTargetId(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	return unit->idMoveTarget;
}

//----------------------------------------------------------------------------
inline const VECTOR & UnitGetMoveDirection(
	const UNIT* unit)
{
	return unit->vMoveDirection;
}

//----------------------------------------------------------------------------
inline void UnitSetMoveDirection(
	UNIT* unit,
	const VECTOR & vMoveDirection)
{
	ASSERT_RETURN( unit );
	unit->vMoveDirection = vMoveDirection;
	unit->idMoveTarget = INVALID_ID;
	unit->vMoveTarget.fX = unit->vMoveTarget.fY = unit->vMoveTarget.fZ = 0.0f;
}

//----------------------------------------------------------------------------
inline VECTOR UnitGetFaceDirection(
	UNIT * unit,
	BOOL bDrawn)
{ 
	if (!unit)
	{
		return VECTOR(0, 1, 0);
	}
	return bDrawn ? unit->vFaceDirectionDrawn : unit->vFaceDirection;
}

//----------------------------------------------------------------------------
inline VECTOR UnitGetDirectionXY(
	UNIT* unit)
{ 
	ASSERT_RETZERO(unit);
	VECTOR vDirection = unit->vFaceDirection;
	vDirection.fZ = 0.0f;
	return vDirection;
}

//----------------------------------------------------------------------------
inline VECTOR UnitGetVUpDirection(
	UNIT* unit)
{ 
	if (!unit)
	{
		return VECTOR(0, 0, 1);
	}
	return unit->vUpDirection;
}

//----------------------------------------------------------------------------
inline UNIT * UnitGetOwnerUnit(
	UNIT * unit)
{
	UNITID idOwner = UnitGetOwnerId(unit);
	if (idOwner == INVALID_ID)
	{
		return NULL;
	}
	return UnitGetById(UnitGetGame(unit), idOwner);
}

//----------------------------------------------------------------------------
inline UNIT * UnitGetUltimateOwner(
	UNIT * unit)
{
	ASSERT_RETNULL(unit);		// don't change this, too many places assume a unit gets returned

	GAME * game = UnitGetGame(unit);
	ASSERT_RETNULL(game);

	UNIT * owner = unit;
	do
	{
		UNITID idOwner = UnitGetOwnerId(owner);
		if (idOwner == INVALID_ID || idOwner == UnitGetId(owner))
		{
			return owner;
		}
		UNIT * next = UnitGetById(UnitGetGame(owner), idOwner);
		if (!next)
		{
			return owner;
		}
		owner = next;
	} while (1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int UnitGetTeam(
	UNIT * unit)
{
	ASSERT_RETINVALID(unit);
	
	UNIT * pOwner = UnitGetOwnerUnit( unit );
	if (pOwner && pOwner != unit)
	{
		return UnitGetTeam( pOwner );
	}
	return unit->nTeamOverride != INVALID_TEAM ? unit->nTeamOverride : unit->nTeam;
}


//----------------------------------------------------------------------------
#if GAME_EVENTS_DEBUG
 #if GLOBAL_EVENTS_TRACKING
  #define UnitRegisterEventTimed(u, fp, ed, t)			static unsigned int UIDEN(GET__) = GlobalEventTrackerRegisterEvent(#fp); UnitRegisterEventTimedFunc(u, UIDEN(GET__), #fp, #fp, __LINE__, fp, ed, t)
  #define UnitRegisterEventTimedRet(r, u, fp, ed, t)	static unsigned int UIDEN(GET__) = GlobalEventTrackerRegisterEvent(#fp); r = UnitRegisterEventTimedFunc(u, UIDEN(GET__), #fp, #fp, __LINE__, fp, ed, t)
 #else
  #define UnitRegisterEventTimed(u, fp, ed, t)			UnitRegisterEventTimedFunc(u, #fp, __LINE__, fp, ed, t)
  #define UnitRegisterEventTimedRet(r, u, fp, ed, t)	r = UnitRegisterEventTimedFunc(u, #fp, __LINE__, fp, ed, t)
 #endif
#else
 #if GLOBAL_EVENTS_TRACKING
  #define UnitRegisterEventTimed(u, fp, ed, t)			static unsigned int UIDEN(GET__) = GlobalEventTrackerRegisterEvent(#fp); UnitRegisterEventTimedFunc(u, UIDEN(GET__), #fp, fp, ed, t)
  #define UnitRegisterEventTimedRet(r, u, fp, ed, t)	static unsigned int UIDEN(GET__) = GlobalEventTrackerRegisterEvent(#fp); r = UnitRegisterEventTimedFunc(u, UIDEN(GET__), #fp, fp, ed, t)
 #else
  #define UnitRegisterEventTimed(u, fp, ed, t)			UnitRegisterEventTimedFunc(u, fp, ed, t)
  #define UnitRegisterEventTimedRet(r, u, fp, ed, t)	r = UnitRegisterEventTimedFunc(u, fp, ed, t)
 #endif
#endif

inline GAME_EVENT * UnitRegisterEventTimedFunc(
	UNIT * unit,
#if GLOBAL_EVENTS_TRACKING
	unsigned int idGlobalTrackingIndex,
	const char * szGlobalTrackingName,
#endif
#if GAME_EVENTS_DEBUG
	const char * file,
	int line,
#endif
	FP_UNIT_EVENT_TIMED * pfCallback,
	const struct EVENT_DATA * event_data, 
	int nTick)
{
	ASSERT_RETNULL(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETNULL(game);
	ASSERT_RETNULL(!UnitTestFlag(unit, UNITFLAG_JUSTFREED));

	return GameEventRegisterFunc(game, 
#if GLOBAL_EVENTS_TRACKING
		idGlobalTrackingIndex, szGlobalTrackingName,
#endif
#if GAME_EVENTS_DEBUG
		file, line,
#endif
		pfCallback, unit, &unit->events, event_data, nTick);
}

//----------------------------------------------------------------------------
inline GAME_EVENT * UnitRegisterEventTimedFunc(
	UNIT * unit,
#if GLOBAL_EVENTS_TRACKING
	unsigned int idGlobalTrackingIndex,
	const char * szGlobalTrackingName,
#endif
#if GAME_EVENTS_DEBUG
	const char * file,
	int line,
#endif
	FP_UNIT_EVENT_TIMED * pfCallback,
	const struct EVENT_DATA & event_data, 
	int nTick)
{
	ASSERT_RETNULL(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETNULL(game);
	ASSERT_RETNULL(!UnitTestFlag(unit, UNITFLAG_JUSTFREED));

	return GameEventRegisterFunc(game, 
#if GLOBAL_EVENTS_TRACKING
		idGlobalTrackingIndex, szGlobalTrackingName,
#endif
#if GAME_EVENTS_DEBUG
		file, line,
#endif
		pfCallback, unit, &unit->events, &event_data, nTick);
}


//----------------------------------------------------------------------------
inline void UnitUnregisterAllTimedEvents(
	UNIT* unit)
{
	GameEventUnregisterAll(UnitGetGame(unit), NULL, &unit->events);
}


//----------------------------------------------------------------------------
inline void UnitUnregisterTimedEvent(
	UNIT * unit,
	FP_UNIT_EVENT_TIMED * pfCallback,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck = NULL,
	const struct EVENT_DATA * check_data = NULL)
{
	GameEventUnregisterAll(UnitGetGame(unit), pfCallback, &unit->events, pfUnregisterCheck, check_data);
}

//----------------------------------------------------------------------------
inline void UnitUnregisterTimedEvent(
	UNIT * unit,
	FP_UNIT_EVENT_TIMED * pfCallback,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck,
	const struct EVENT_DATA & check_data)
{
	GameEventUnregisterAll(UnitGetGame(unit), pfCallback, &unit->events, pfUnregisterCheck, check_data);
}

BOOL CheckEventParam1(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data);

BOOL CheckEventParam12(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data);

BOOL CheckEventParam2(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data);

BOOL CheckEventParam123(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data);
	
BOOL CheckEventParam13(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data);

BOOL CheckEventParam14(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data);

BOOL CheckEventParam4(
	GAME* pGame,
	UNIT* unit, 
	const struct EVENT_DATA& event_data, 
	const struct EVENT_DATA* check_data);

//----------------------------------------------------------------------------
inline BOOL UnitHasTimedEvent(
	UNIT * unit,
	FP_UNIT_EVENT_TIMED * pfCallback,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck = NULL,
	const struct EVENT_DATA * check_data = NULL)
{
	return GameEventExists(UnitGetGame(unit), pfCallback, &unit->events, pfUnregisterCheck, check_data);
}

//----------------------------------------------------------------------------
inline BOOL UnitHasTimedEvent(
	UNIT * unit,
	FP_UNIT_EVENT_TIMED * pfCallback,
	FP_UNIT_EVENT_TIMED_UNREGISTER_CHECK * pfUnregisterCheck,
	const struct EVENT_DATA & check_data)
{
	return GameEventExists(UnitGetGame(unit), pfCallback, &unit->events, pfUnregisterCheck, &check_data);
}


//----------------------------------------------------------------------------
inline BOOL IsUnitDeadOrDying(
	UNIT * unit)
{
	// the restart flag is a redundant safety feature in case a player has its dead_or_dying flag cleared w/o restarting
	return (UnitTestFlag(unit, UNITFLAG_DEAD_OR_DYING) || UnitTestFlag(unit, UNITFLAG_PLAYER_WAITING_FOR_RESPAWN));
}

//----------------------------------------------------------------------------
inline BOOL UnitIsInLimbo(
	UNIT * unit)
{
	return UnitTestFlag(unit, UNITFLAG_IN_LIMBO);
}


//----------------------------------------------------------------------------
// Callback for sound system
//----------------------------------------------------------------------------
BOOL IsControlUnitDeadOrDying();

//----------------------------------------------------------------------------
inline float UnitGetVelocity(
	const UNIT* unit)
{
	ASSERT_RETZERO(unit);
	return unit->fVelocity;
}

//----------------------------------------------------------------------------
inline float UnitGetSpeed(
	const UNIT* unit)
{
	ASSERT_RETZERO(unit);
	return unit->fUnitSpeed;
}


//----------------------------------------------------------------------------
inline void UnitSetVelocity(
	UNIT* unit,
	float fVelocity)
{
	ASSERT_RETURN(unit);
	unit->fVelocity = fVelocity;
}

//----------------------------------------------------------------------------
inline void UnitSetAcceleration(
	UNIT* unit, 
	float fAcceleration)
{
	ASSERT_RETURN(unit);
	unit->fAcceleration = fAcceleration;
}

//----------------------------------------------------------------------------
float UnitGetVelocityForMode(
	UNIT* pUnit,
	UNITMODE eMode );

//----------------------------------------------------------------------------
inline float UnitGetScale(
	const UNIT* unit)
{
	ASSERT_RETVAL(unit,1.0f);
	return unit->fScale;
}

void UnitSetScale(
	UNIT* unit,
	float fScale);

float UnitGetLuck( 
	UNIT *pUnit);

int UnitGetDifficultyOffset(
	UNIT * pUnit);

void UnitSetDontTarget(
	UNIT* unit,
	BOOL value);

void UnitSetDontAttack(
	UNIT *pUnit,
	BOOL bValue);

BOOL UnitGetCanTarget(
	UNIT* unit);

void UnitSetDontInterrupt(
	UNIT* unit,
	BOOL value);

BOOL UnitGetCanInterrupt(UNIT* unit);

void UnitSetPreventAllSkills(
	UNIT* unit,
	BOOL value);

//----------------------------------------------------------------------------
inline BOOL IsAllowedUnit(
	const int unittype,
	const int* allowtypes,
	int size)
{
	for (int ii = 0; ii < size; ii++)
	{
		int type = allowtypes[ii];
		if (type == 0)
		{
			break;
		}
		if (type < 0 && UnitIsA(unittype, -type))
		{
			return FALSE;
		}
	}
	for (int jj = 0; jj < size; jj++)
	{
		int type = allowtypes[jj];
		if (type == 0)
		{
			break;
		}
		if (type > 0 && UnitIsA(unittype, type))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
inline BOOL IsAllowedUnit(
	UNIT * unit,
	const int * allowtypes,
	int size)
{
	ASSERT_RETFALSE(unit);
	int unittype = unit->nUnitType;

	return IsAllowedUnit(unittype, allowtypes, size);
}

//----------------------------------------------------------------------------
float UnitsGetDistanceSquared(
	const UNIT * unitFirst,
	const UNIT * unitSecond);

float UnitsGetDistanceSquaredXY(
	const UNIT * unitFirst,
	const UNIT * unitSecond);

//----------------------------------------------------------------------------
inline float UnitsGetDistance(
	const UNIT * unit,
	const VECTOR & vTarget)
{
	ASSERT_RETZERO(unit);
	VECTOR vDelta;
	VectorSubtract(vDelta, vTarget, UnitGetPosition(unit));
	return VectorLength(vDelta);
}

BOOL UnitsAreInRange(
	const UNIT* unit,
	const UNIT* target,
	float fRangeMin,
	float fRangeMax,
	float * pfDistanceSquaredIn );

BOOL UnitIsInRange(
	const UNIT* unit,
	const VECTOR & vTarget,
	float fRangeMin,
	float fRangeMax,
	float * pfDistanceSquaredIn );

BOOL UnitIsInRange(
	const UNIT* unit,
	const VECTOR & vPosition,
	float fRange,
	float * pfDistanceSquaredIn );

//----------------------------------------------------------------------------
inline void UnitGetWorldMatrix(
	const UNIT* pUnit,
	MATRIX & mMatrix )
{
	MATRIX mTransform;
	MatrixFromVectors( mTransform, UnitGetPosition(pUnit), pUnit->vUpDirection, -pUnit->vFaceDirection );
	MATRIX mScale;
	MatrixScale( &mScale, UnitGetScale( pUnit ) );
	MatrixMultiply( &mMatrix, &mScale, &mTransform );
}

//----------------------------------------------------------------------------
inline const VECTOR& UnitGetDrawnPosition(
	const UNIT* unit)
{
	ASSERT_RETVAL(unit, cgvNone);
	return unit->vDrawnPosition;
}

//----------------------------------------------------------------------------
inline const VECTOR& c_UnitGetPosition(
	const UNIT* unit)
{
	ASSERT_RETVAL(unit, cgvNone);
	return UnitGetPosition(unit);
}

//----------------------------------------------------------------------------
VECTOR UnitGetCenter(
	UNIT* unit);

VECTOR UnitGetPositionAtPercentHeight(
	UNIT* unit,
	float fPercent );

//----------------------------------------------------------------------------
int c_sGetParticleDefId(
	const char * pszName );

//----------------------------------------------------------------------------
float UnitGetPathingCollisionRadius(
	const UNIT * pUnit);

//----------------------------------------------------------------------------
BOOL UnitGetChecksRadiusWhenPathing(
	const UNIT * pUnit);

BOOL UnitGetChecksHeightWhenPathing(
	const UNIT * pUnit);

//----------------------------------------------------------------------------
float UnitGetCollisionRadius(
	const UNIT * pUnit);

//----------------------------------------------------------------------------
float UnitGetCollisionRadiusHorizontal(
	const UNIT * pUnit);


//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------

#define DISTANCE_ON_ANOTHER_LEVEL 100000.0f
inline float UnitsGetDistanceSquaredMinusRadiusXY(
	const UNIT* unitFirst,
	const UNIT* unitSecond)
{
	ASSERT_RETZERO(unitFirst);
	ASSERT_RETZERO(unitSecond);

	if (UnitGetLevel(unitFirst) != UnitGetLevel(unitSecond) )
		return DISTANCE_ON_ANOTHER_LEVEL;

	return VectorDistanceSquaredXY(UnitGetPosition(unitFirst), UnitGetPosition(unitSecond)) -
		    ( UnitGetCollisionRadius( unitSecond ) * UnitGetCollisionRadius( unitSecond ) );
}

//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
BOOL IsInAttackRange( 
					 UNIT* pUnit,
					 GAME *pGame, 
					 UNIT *pTarget,
					 int nSkill,
					 BOOL bLineOfSightCheck = TRUE );

//----------------------------------------------------------------------------
float UnitGetRawCollisionRadius(
	const UNIT * pUnit);

//----------------------------------------------------------------------------
float UnitGetRawCollisionRadiusHorizontal(
	const UNIT * pUnit);

//----------------------------------------------------------------------------
float UnitGetCollisionHeight(
	const UNIT * pUnit);

//----------------------------------------------------------------------------
float UnitGetMaxCollisionRadius( 
	GAME *pGame,
	GENUS eGenus,
	int nUnitClass);

//----------------------------------------------------------------------------
float UnitGetMaxCollisionHeight( 
	GAME *pGame,
	GENUS eGenus,
	int nUnitClass);

//----------------------------------------------------------------------------
inline VECTOR UnitGetUpDirection(
	const UNIT* unit)
{
	ASSERT_RETVAL( unit, VECTOR(0,0,1) );
	return unit->vUpDirection;
}

//----------------------------------------------------------------------------
inline void UnitSetUpDirection(
	UNIT* unit,
	const VECTOR & vUpDir )
{
	ASSERT_RETURN( unit );
	unit->vUpDirection = vUpDir;
}

//----------------------------------------------------------------------------
inline VECTOR UnitGetVelocityVector(
	const UNIT* unit)
{
	ASSERT_RETZERO( unit );
	VECTOR vVelocity;
	VectorScale(vVelocity, UnitGetMoveDirection(unit), UnitGetVelocity(unit));
	return vVelocity;
}

//----------------------------------------------------------------------------
inline int c_UnitGetModelId(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	if (unit->pGfx == NULL)
	{
		return INVALID_ID;
	}
	return unit->pGfx->nModelIdCurrent;
}

//----------------------------------------------------------------------------
inline int c_UnitGetModelIdFirstPerson(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	if (unit->pGfx == NULL)
	{
		return INVALID_ID;
	}
	return unit->pGfx->nModelIdFirstPerson;
}

//----------------------------------------------------------------------------
inline int c_UnitGetModelIdThirdPerson(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	if (unit->pGfx == NULL)
	{
		return INVALID_ID;
	}
	return unit->pGfx->nModelIdThirdPerson;
}

//----------------------------------------------------------------------------
inline int c_UnitGetModelIdThirdPersonOverride(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	if (unit->pGfx == NULL)
	{
		return INVALID_ID;
	}
	return unit->pGfx->nModelIdThirdPersonOverride;
}

//----------------------------------------------------------------------------
inline int c_UnitGetAppearanceId(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	if (unit->pGfx == NULL)
	{
		return INVALID_ID;
	}
	return unit->pGfx->nModelIdCurrent; // models and appearances share the same id
}

//----------------------------------------------------------------------------
float c_UnitGetHeightFactor(
	UNIT *pUnit);

//----------------------------------------------------------------------------
struct APPEARANCE_DEFINITION * UnitGetAppearanceDef(
	const UNIT* unit);

//----------------------------------------------------------------------------
int UnitGetAppearanceDefId(
	const UNIT* unit,
	BOOL bForceThirdPerson = FALSE );

//----------------------------------------------------------------------------
int UnitGetAppearanceDefIdForCamera( 
	const UNIT* unit );

//----------------------------------------------------------------------------
inline int c_UnitGetModelIdInventory(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	if (unit->pGfx == NULL)
	{
		return INVALID_ID;
	}
	return unit->pGfx->nModelIdInventory; // models and appearances share the same id
}

//----------------------------------------------------------------------------
int c_UnitGetModelIdInventoryInspect(
	UNIT* unit);

//----------------------------------------------------------------------------
inline int c_UnitGetPaperdollModelId(
	UNIT* unit)
{
	ASSERT_RETINVALID( unit );
	if (unit->pGfx == NULL)
	{
		return INVALID_ID;
	}
	return unit->pGfx->nModelIdPaperdoll; // models and appearances share the same id
}

//----------------------------------------------------------------------------
BOOL c_UnitCreatePaperdollModel(
	UNIT *pUnit, 
	DWORD dwFlags);

//----------------------------------------------------------------------------
inline PGUID UnitGetGuid(
	const UNIT * unit)
{
	ASSERT_RETVAL(unit, INVALID_GUID);
	return unit->guid;
}

//----------------------------------------------------------------------------
inline PGUID UnitGetUltimateOwnerGuid(
	UNIT * unit)
{
	ASSERT_RETINVALID(unit);

	UNIT * pUltimateOwner = UnitGetUltimateOwner(unit);

	ASSERT_RETINVALID(pUltimateOwner);

	return UnitGetGuid(pUltimateOwner);
}

//----------------------------------------------------------------------------
void UnitOverrideAppearanceSet(
	GAME *pGame,
	UNIT * pUnit,
	EXCELTABLE eTable,
	int nOverrideClass );

void UnitOverrideAppearanceClear(
	GAME *pGame,
	UNIT * pUnit );

void UnitGfxInventoryInit(
	UNIT* unit,
	BOOL bCreateWardrobe);

void UnitGfxInventoryFree(
	UNIT* unit);

void UnitGfxInventoryInspectFree(
	UNIT* unit);

#if ISVERSION(CHEATS_ENABLED) || ISVERSION(RELEASE_CHEATS_ENABLED)
void s_UnitKillAll(
   GAME * game,
   LEVEL * pLevel,
   BOOL bFreeUnits);
#endif

int UnitGetLevelDefinitionIndex( 
	UNIT *pUnit);

LEVELID UnitGetLevelID(
	const UNIT* unit);

int UnitGetLevelDepth( 
	UNIT *pUnit);

int UnitGetLevelAreaID( 
	UNIT *pUnit);

void UnitSetInteractive(
	UNIT *pUnit,
	UNIT_INTERACTIVE eInteractiveState );

BOOL UnitIsInteractive(
	UNIT *pUnit);

BOOL UnitCanInteractWith(
	UNIT *pInteractiveUnit,
	UNIT *pOtherUnit);

BOOL UnitIsDualWielding( UNIT *pUnit, 
						 int nSkill );

BOOL UnitIsArmed( UNIT *pUnit );

BOOL UnitIsInAttack( UNIT *pUnit  );


BOOL UnitIsInAttackLoose( UNIT *pUnit  );

void UnitCalculateVisibleDamage( 
	UNIT *pUnit );

void UnitCalculatePVPGearDrop( 
	UNIT *pUnit );

int UnitGetFactions( 
	UNIT *pUnit, 
	int *pnFactions);

int UnitGetPrimaryFaction( 
	UNIT *pUnit);

BOOL UnitIsInvulnerable(
	UNIT *pUnit,
	int nDamageType = 0);

int UnitGetExtraDyingTicks(
	UNIT *pUnit);

BOOL UnitAlwaysShowsLabel(
	UNIT *pUnit);

BOOL UnitCanHoldMoreMoney(
	UNIT *pUnit);
	

void UnitAddCurrency(
	UNIT *pUnit,
	cCurrency currency);

void UnitRemoveCurrencyUnsafe(
	UNIT *pUnit,
	cCurrency currencyToRemove );

__checkReturn BOOL UnitRemoveCurrencyValidated(
	UNIT *pUnit,
	cCurrency currencyToRemove );


/*
BOOL UnitSetMoney(
	UNIT *pUnit,
	int nMoney);

int UnitGetMoney(
	UNIT *pUnit);

int UnitGetMaxMoney(
	UNIT *pUnit);
*/
float UnitGetLabelScale( 
	const UNIT *pUnit);

//----------------------------------------------------------------------------

struct VITALS_SPEC
{

	int nHealthPercent;
	int nPowerPercent;
	int nShieldPercent;
	int nHealthAbsolute;
	// maybe someday add other flags here to clear or not clear other vital restoration components
	
	VITALS_SPEC::VITALS_SPEC( void )
		:	nHealthPercent( 100 ),
			nPowerPercent( 100 ),
			nShieldPercent( 100 ),
			nHealthAbsolute( -1 )
	{ }
	
};

void VitalsSpecZero(
	VITALS_SPEC &tVitalsSpec);
	
void s_UnitRestoreVitals(
	UNIT *pUnit,
	BOOL bPlayEffects,
	const VITALS_SPEC *pVitalsSpec = NULL);

BOOL UnitCanSendMessages( 
	UNIT *pUnit);

void UnitSetCanSendMessages(
	UNIT *pUnit,
	BOOL bReady);

BOOL UnitIsInWorld(
	UNIT *pUnit);

void s_SendUnitJump(
	UNIT *pUnit,
	GAMECLIENT *pClient);

void UnitSetZVelocity(
	UNIT *pUnit,
	float flZVelocity);

void UnitChangeZVelocity(
	UNIT *pUnit,
	float flZVelocityDelta);
	
float UnitGetZVelocity(
	const UNIT *pUnit);

void UnitSetOnGround(
	UNIT *pUnit,
	BOOL bOnGround);

BOOL UnitIsOnGround(
	const UNIT *pUnit);

BOOL UnitIsOnTrain(
	const UNIT * pUnit);

BOOL UnitIsInTutorial(
	UNIT * pUnit);

PARTYID UnitGetPartyId(
	const UNIT *pUnit);

#if !ISVERSION(SERVER_VERSION)
BOOL c_UnitIsInMyParty(
	const UNIT *pUnit);
#endif


int UnitGetStartLevelDefinition(
	UNIT *pUnit);

int UnitGetReturnLevelDefinition(
	UNIT *pUnit);

void UnitSetShape(
	UNIT *pUnit,
	float fHeightPercent,
	float fWeightPercent );

void UnitGetShape(
	UNIT *pUnit,
	float& fHeightPercent,
	float& fWeightPercent );

BOOL UnitCanDoPartyOperations(
	UNIT *pUnit);

void UnitDelayedRemoveFromPartyGame(
	UNIT *pUnit);

void c_UnitBreathChanged(
	UNIT * unit );

void s_UnitBreathInit(
	UNIT *pUnit);

void s_UnitBreathLevelChange(
	UNIT *pUnit);

void UnitFaceUnit(
	UNIT *pUnit,
	UNIT *pUnitOther);

enum GENDER UnitGetGender(
	UNIT *pUnit);

int UnitGetRace(
	UNIT *pUnit);

void UnitWaitingForInventoryMove(
	UNIT *pUnit, 
	BOOL bWaiting);

BOOL UnitIsWaitingForInventoryMove(
	UNIT *pUnit);

WORD UnitGetClassCode(
	UNIT* unit);

void c_UnitPlayCantUseSound(
	GAME* game,
	UNIT* container,
	UNIT* item);

void c_UnitPlayCantSound(
							GAME* game,
							UNIT* pUnit );

BOOL UnitIsDecoy(
	UNIT *pUnit);

void UnitSetPosition( 
	UNIT *pUnit,
	const VECTOR &vPosition);

void UnitSetDrawnPosition( 
	UNIT *pUnit,
	const VECTOR &vPosition);

void UnitChangePosition(
	UNIT * unit,
	const VECTOR & position);

void UnitDisplayGetOverHeadPositions( 
	UNIT *pUnit,
	UNIT *pControlUnit,
	VECTOR vEye,
	float fLabelHeight, 
	VECTOR& vScreenPos,
	float &fLabelScale,
	float &fControlDistSq,
	float &fCameraDistSq);

BOOL UnitIsInanimate(
	UNIT *pUnit);

void UnitSetGUIDOwner(
	UNIT *pUnit,
	UNIT *pOwner);

PGUID UnitGetGUIDOwner(
	UNIT *pUnit);

void UnitSetSafe(
	UNIT *pUnit );

void UnitClearSafe(
	UNIT *pUnit);

void UnitGhost( 
	UNIT *pUnit,
	BOOL bEnable);
	
BOOL UnitIsGhost(
	UNIT *pUnit,
	BOOL bIncludeHardcoreDead = TRUE);

BOOL UnitIsSubscriberOnly(
	UNIT *pUnit);

BOOL UnitIsInFrontOf(
	UNIT *pUnit,
	UNIT *pUnitFrontOf);

BOOL UnitIsBehindOf(
	UNIT *pUnit,
	UNIT *pUnitBehindOf);

BOOL UnitIsPlayer(
	const UNIT *pUnit);

SUBLEVELID UnitGetSubLevelID(
	const UNIT *pUnit);

SUBLEVEL *UnitGetSubLevel(
	const UNIT *pUnit);
		
BOOL UnitsAreInSameLevel(
	const UNIT *pUnitA,
	const UNIT *pUnitB);

BOOL UnitsAreInSameSubLevel(
	const UNIT *pUnitA,
	const UNIT *pUnitB);

BOOL UnitIsGI( 
	UNIT *pUnit,
	enum GLOBAL_INDEX eGlobalIndex);
				
BOOL UnitCanRun(	
	UNIT *pUnit);

BOOL UnitCanIdentify(
	UNIT *pUnit);

void HolyRadiusAdd(
	int nModelId,
	int nParticleSystemDefId,
	int nState,
	float fHolyRadius,
	int nEnemyCount );

void c_UnitEnableHolyRadius( 
	UNIT * pUnit );

void c_UnitDisableHolyRadius(
	UNIT * pUnit );

void c_UnitUpdateHolyRadiusSize(
	UNIT * pUnit );

void c_UnitUpdateHolyRadiusUnitCount(
	UNIT * pUnit );

typedef void (*PFN_FUSE_FUNCTION)( UNIT *pUnit, int nTicks );
typedef void (*PFN_UNFUSE_FUNCTION)( UNIT *pUnit );
typedef int (*PFN_GETFUSE_FUNCTION)( UNIT *pUnit );


void UnitSetFuse(
	UNIT *pUnit,
	int nTicks);
	
void UnitUnFuse(
	UNIT *pUnit);

int UnitGetFuse(
	UNIT *pUnit);

void UnitAllowTriggers(
	UNIT *pUnit,
	BOOL bAllow,
	float flDurationInSeconds);

BOOL s_UnitCanBeSaved(
	UNIT *pUnit,
	BOOL bIgnoreNoSave);

BOOL UnitHasSkill(	
	UNIT *pUnit,
	int nSkill);

void UnitSetRestrictedToGUID(
	UNIT *pUnit,
	PGUID guidRestrictedTo);

PGUID UnitGetRestrictedToGUID(
	UNIT *pUnit);

BOOL UnitCanUse(
	UNIT *pUnit,
	UNIT *pObject);
		
int s_UnitGetPossibleExperiencePenaltyPercent(
	UNIT *pUnit);

void s_UnitGiveExperience(
	UNIT * unit,
	int experience);

BOOL UnitIsMonsterClass(
	UNIT *pUnit,
	int nMonsterClass);

BOOL UnitIsItemClass(
	UNIT *pUnit,
	int nItemClass);
	
BOOL UnitIsObjectClass(
	UNIT *pUnit,
	int nObjectClass);
	
BOOL UnitIsGenusClass(
	UNIT *pUnit,
	GENUS eGenus,
	int nClass);

BOOL UnitIsControlUnit(
	UNIT* pUnit);
	
BOOL UnitHasBadge(
	UNIT *pUnit,
	int nBadge );

int UnitGetPlayerRank(
	UNIT *pUnit);

void UnitSetPlayerRank(
	UNIT *pUnit,
	int nRank);

int UnitGetExperienceLevel(
	UNIT *pUnit);

int UnitGetExperienceRank(
	UNIT *pUnit);

void UnitSetExperienceLevel(
	UNIT *pUnit,
	int nExperienceLevel);

int UnitGetExperienceValue(
	UNIT *pUnit);

int UnitGetRankExperienceValue(
	UNIT *pUnit);

BOOL UnitInventoryCanBeRemoved(
	UNIT *pUnit);

void UnitSetTeam(
	UNIT *pUnit,
	int nTeam,
	BOOL bValidate = TRUE);

void s_UnitVersion(
	UNIT *pUnit);

int UnitGetVersion(
	UNIT *pUnit);

void c_UnitSetVisibilityOnClientByThemesAndGameEvents( UNIT *pUnit,
													   int nThemeOverride = INVALID_ID  );


BOOL UnitsThemeIsVisibleInLevel( UNIT *pUnit,
								 int nThemeOverride = INVALID_ID );

BOOL IsUnitDualWielding(
	GAME * pGame,
	UNIT * pUnit,
	UNITTYPE eUnitType);

void UnitFreeListFree(
	GAME * game);

SPECIES UnitResolveSpecies(
	GENUS eGenus,
	DWORD dwClassCode);

int UnitGetMaxLevel(
	UNIT* unit );

int UnitGetMaxRank(
	UNIT* unit );

#if ISVERSION(DEVELOPMENT)
void UnitLostItemDebug(
	UNIT *pUnit);

#else
#define UnitLostItemDebug(u)
#endif

BOOL UnitIsInInstanceLevel(
	UNIT *pUnit);

BOOL UnitCanBeCreated(
	GAME *pGame,
	const UNIT_DATA *pUnitData);

BOOL UnitIsAlwaysKnownByPlayer(
	UNIT *pUnit,
	UNIT *pPlayer);
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_UnitRespawnFromCorpse( UNIT *pUnit );

BOOL s_MonsterRespawnFromRespawner( 
	GAME * pGame,
	UNIT * unit,
	const struct EVENT_DATA & tEventData);

BOOL s_ObjectRespawnFromRespawner( 
	GAME * pGame,
	UNIT * unit,
	const struct EVENT_DATA & tEventData);
#endif	

struct UNITGFX * UnitDataGfxInit(
	const struct UNIT_DATA * pUnitData,
	int nUnitDataClass,
	struct WARDROBE_INIT * pWardrobeInit,
	BOOL bUnitIsClientOnly,
	BOOL bIsControlUnit,
	GENUS eUnitGenus,
	float fScale);

void UnitSetLostAtTime(
	UNIT *pUnit,
	time_t utcTime);

time_t UnitGetLostAtTime(
	UNIT *pUnit);

enum UNIT_HIERARCHY
{
	UH_OWNER,
	UH_CONTAINER
};

BOOL UnitTestFlagHierarchy(
	UNIT *pUnit,
	UNIT_HIERARCHY eHiearchy,
	int nUnitFlag);

const char *GenusGetString(
	GENUS eGenus);

int UnitDataGetIconTextureID(
	const struct UNIT_DATA * pUnitData );

int UnitGetWarpToDepth( UNIT *pUnit );

int UnitGetWarpToLevelAreaID( UNIT *pUnit );

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
extern BOOL gbHeadstoneDebug;
extern BOOL gbLostItemDebug;
#endif


//----------------------------------------------------------------------------
// INLINE
//----------------------------------------------------------------------------
inline void UnitSetWardrobeChanged(
	UNIT * unit,
	BOOL bNeedsUpdate)
{
	ASSERT_RETURN(unit);
	UnitSetFlag(unit, UNITFLAG_WARDROBE_CHANGED, bNeedsUpdate);	
}

inline cCurrency UnitGetCurrency( UNIT *unit )
{
	return cCurrency( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//this function will take all the values that are set that match a unit's unittypes. 
inline int UnitGetStatMatchingUnittype(UNIT *pUnit,
									   UNIT **pSecondaryUnit, //weapon for instance
									   int nCount,
									   STATS_TYPE nStat )
{		
	ASSERT_RETZERO( pUnit );
	
	int nValue( UnitGetStat( pUnit, nStat ) );	//get initial stat
	for( int i = 0; i < nCount; i++ )
	{
		if( pSecondaryUnit[i] )
		{
			STATS_ENTRY tStatsUnitTypes[ 10 ]; //hard coded for now. Testing..
			int nUnitTypeCounts = UnitGetStatsRange( pUnit, nStat, 0, tStatsUnitTypes, 10 );	
			for( int t = 0; t < nUnitTypeCounts; t++ )
			{
				int nUnitType = STAT_GET_PARAM( tStatsUnitTypes[ t ].stat );			
				if( nUnitType != INVALID_ID )
				{				
					if( UnitIsA( pSecondaryUnit[i], nUnitType ) )
					{
						nValue += tStatsUnitTypes[ t ].value;	
					}
				}
			}			
		}
	}
	return nValue;
}

#endif // _UNITS_H_
