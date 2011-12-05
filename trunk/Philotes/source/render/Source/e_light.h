//----------------------------------------------------------------------------
// e_light.h
//
// - Header for lights
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_LIGHT__
#define __E_LIGHT__

#include "interpolationpath.h"
#include "proxmap.h"
#include "ptime.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//#define TRACE_LIGHT_PROXMAP_ACTIONS


#define LIGHT_INTENSITY_MDR_SCALE				1.5f


#define DEFAULT_DYNAMIC_LIGHT_SELECT_RANGE		50.f
#define DEFAULT_DYNAMIC_LIGHT_SELECT_MAX_RAD	15.f


// definition flags
#define LIGHTDEF_FLAG_NONE				MAKE_MASK(0)
#define LIGHTDEF_FLAG_DIFFUSE			MAKE_MASK(1)		// Does the light affect Diffuse?
#define LIGHTDEF_FLAG_SPECULAR			MAKE_MASK(2)		// Does the light affect Specular?
#define LIGHTDEF_FLAG_AMBIENT			MAKE_MASK(3)		
#define LIGHTDEF_FLAG_DIRECTIONAL		MAKE_MASK(4)		
#define LIGHTDEF_FLAG_SHADOW			MAKE_MASK(5)		
#define LIGHTDEF_FLAG_NOFADEIN			MAKE_MASK(6)		// can the light fade in when selected?		

// light object flags
#define LIGHT_FLAG_STATIC				MAKE_MASK(0)
#define LIGHT_FLAG_MOVED				MAKE_MASK(1)
#define LIGHT_FLAG_SPOT					MAKE_MASK(2)
#define LIGHT_FLAG_DYING				MAKE_MASK(3)
#define LIGHT_FLAG_DEAD					MAKE_MASK(4)
#define LIGHT_FLAG_FORCE_LOOP			MAKE_MASK(5)
#define LIGHT_FLAG_SPECULAR_ONLY		MAKE_MASK(6)
#define LIGHT_FLAG_PATH_TIME_SET		MAKE_MASK(7)
#define LIGHT_FLAG_NO_APPLY				MAKE_MASK(8)	// don't add this light to models' light lists
#define LIGHT_FLAG_SELECTED				MAKE_MASK(9)	// this light has been selected by a model in the current room
#define LIGHT_FLAG_NODRAW				MAKE_MASK(10)
#define LIGHT_FLAG_NOFADEIN				MAKE_MASK(11)
#define LIGHT_FLAG_DEFINITION_APPLIED	MAKE_MASK(12)

#define LIGHT_MAX_PRIORITY				2.0f

#define LIGHT_GROUP_GENERAL				MAKE_MASK(0)
#define LIGHT_GROUP_ENVIRONMENT			MAKE_MASK(1)

#define LIGHT_GROUP_ALL					0xffffffff
#define LIGHT_GROUPS_DEFAULT			( LIGHT_GROUP_GENERAL | LIGHT_GROUP_ENVIRONMENT )

#define LIGHT_SLOT_FADE_MS				500

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum LIGHT_PRIORITY_TYPE
{
	LIGHT_PRIORITY_TYPE_DEFAULT = 0,
	LIGHT_PRIORITY_TYPE_BACKGROUND,
	LIGHT_PRIORITY_TYPE_MISSILE,
	LIGHT_PRIORITY_TYPE_PLAYER,
	LIGHT_PRIORITY_TYPE_PARTICLE,
	// count
	NUM_LIGHT_PRIORITY_TYPES
};

enum ENV_DIR_LIGHT_TYPE
{
	// if these are changed, be sure to update constants in _common.fx
	ENV_DIR_LIGHT_PRIMARY_DIFFUSE = 0,
	ENV_DIR_LIGHT_SECONDARY_DIFFUSE,
	ENV_DIR_LIGHT_PRIMARY_SPECULAR,
	// count
	NUM_ENV_DIR_LIGHTS
};

enum LIGHT_TYPE
{
	LIGHT_TYPE_INVALID = -1,
	LIGHT_TYPE_POINT = 0,
	LIGHT_TYPE_SPOT,
	// count
	NUM_LIGHT_TYPES
};

enum LIGHT_SLOT_UPDATE_TYPE
{
	LS_UPDATE_TYPE_GENERAL = 0,
	LS_UPDATE_TYPE_HIGH_FREQ,
	// count
	NUM_LS_UPDATE_TYPES
};

enum LIGHT_SLOT_LIGHT_TYPE
{
	LS_LIGHT_DIFFUSE_POINT = 0,
	//LS_LIGHT_STATIC_POINT,
	LS_LIGHT_SPECULAR_ONLY_POINT,
	// count
	NUM_LS_LIGHT_TYPES,
	LS_LIGHT_TYPE_ALL = -1,
};

enum
{
	NUM_SPECULAR_ONLY_POINT_LIGHT_SLOTS = 2,		// for background models
	NUM_STATIC_POINT_LIGHT_SLOTS = 1,				// for animated models
	NUM_DIFFUSE_POINT_LIGHT_SLOTS = 5,				// for background models
};

enum LIGHT_SLOT_FADE_STATE
{
	LSFS_NOT_FADING = 0,
	LSFS_FADEUP,
	LSFS_FADEDOWN,
};

enum LIGHT_SLOT_STATE
{
	LSS_EMPTY = 0,
	LSS_ACTIVE,
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct LIGHT_SLOT
{
	LIGHT_SLOT_UPDATE_TYPE	eUpdateType;

	LIGHT_SLOT_STATE		eState;
	LIGHT_SLOT_FADE_STATE	eFadeState;
	DWORD					dwFlags;
	int						nLightID;

	TIME					tFadeStart;
	TIME					tFadeEnd;
	float					fFadeStrStart;
	float					fFadeStrEnd;

	float					fStrength;		// computed from fade values, etc.
};

struct VIRTUAL_LIGHT_SLOT
{
	int nLightID;		// light ID
	int nSlot;			// what slot it's assigned to
};

struct LIGHT_SLOT_CONTAINER
{
	LIGHT_SLOT *			pSlots;
	VIRTUAL_LIGHT_SLOT *	pVirtualSlots;
	int						nSlotsAlloc;
	int						nSlotsActive;

	void Init( int nCount )
	{
		pSlots = (LIGHT_SLOT*)MALLOCZ( NULL, nCount * sizeof(LIGHT_SLOT) );
		pVirtualSlots = (VIRTUAL_LIGHT_SLOT*)MALLOCZ( NULL, nCount * sizeof(VIRTUAL_LIGHT_SLOT) );
		nSlotsAlloc = nCount;
		nSlotsActive = nSlotsAlloc;
	}
	void Destroy()
	{
		if ( pSlots )
		{
			FREE( NULL, pSlots );
			pSlots = NULL;
		}
		if ( pVirtualSlots )
		{
			FREE( NULL, pVirtualSlots );
			pVirtualSlots = NULL;
		}
		nSlotsAlloc = 0;
		nSlotsActive = 0;
	}
	void Clear()
	{
		ASSERT_RETURN( pSlots && pVirtualSlots && nSlotsActive > 0 );
		for ( int i = 0; i < nSlotsActive; i++ )
			pSlots[ i ].eState = LSS_EMPTY;
	}
	void SetMaxActive( int nActive )
	{
		nSlotsActive = MIN( nActive, nSlotsAlloc );
		Clear();
	}
};

struct LIGHT_REFERENCE 
{
	float fPriority;
	int nLightId;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern LIGHT_SLOT_CONTAINER gtLightSlots[ NUM_LS_LIGHT_TYPES ];

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline float e_LightPriorityTypeGetValue( LIGHT_PRIORITY_TYPE eType )
{
	static float sfPriorities[ NUM_LIGHT_PRIORITY_TYPES ] =
	{
		1.f,	// LIGHT_PRIORITY_TYPE_DEFAULT
		1.f,	// LIGHT_PRIORITY_TYPE_BACKGROUND
		0.8f,	// LIGHT_PRIORITY_TYPE_MISSILE
		2.f,	// LIGHT_PRIORITY_TYPE_PLAYER
		0.6f,	// LIGHT_PRIORITY_TYPE_PARTICLE
	};

#if ISVERSION(DEVELOPMENT)
	ASSERT_RETVAL( IS_VALID_INDEX(eType, NUM_LIGHT_PRIORITY_TYPES), 1.f );
#endif

	return sfPriorities[ eType ];
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

int	e_LightNew(
	int nLightDef,
	float fPriority,
	const VECTOR& vPosition,
	VECTOR* pvPositionOffset = NULL);

int	e_LightNewStatic(
	struct STATIC_LIGHT_DEFINITION* pLightDef,
	BOOL bSpecularOnly);

void e_LightRestore(
	void * pDef );

void e_LightGetPosition( 
	int nLight, 
	VECTOR & vPos);

void e_LightSetPosition(
	int nLightId,
	const VECTOR& vPosition,
	const VECTOR* pvDirection = NULL);

void  e_LightRemove		 ( int nLightId );
BOOL  e_LightHasMoved	 ( int nLightId, BOOL bClear );
BOOL  e_LightIsSpot		 ( int nLightId );
void  e_LightsUpdateAll	 ( );
void  e_LightsApplyToAll ( );
void  e_LightForceLoop	 ( int nLightId, BOOL bForce = TRUE );
void  e_LightSetGroup    ( int nLightId, DWORD dwGroups );
DWORD e_LightGetGroup    ( int nLightId );
void  e_LightSetFlags    ( int nLightId, DWORD dwFlags, BOOL bSet );
DWORD e_LightGetFlags	 ( int nLightId );
int   e_LightGetDefinition ( int nLightId );
float e_LightGetLifePercent( int nLightId );
void  e_LightSetLifePercent( int nLightId, float fValue );
BOOL  e_LightExists		 ( int nLightId );
float e_LightGetPriority ( int nLightID );
LIGHT_TYPE e_LightGetType( int nLightId );
void e_LightApplyDefinition( int nLightID );

int e_LightGetClosest( LIGHT_TYPE eType, const VECTOR & vPos );
int e_LightGetClosestMatching( LIGHT_TYPE eType, const VECTOR & vPos, DWORD dwFlagsDoWant, DWORD dwFlagsDontWant, float fRange = DEFAULT_DYNAMIC_LIGHT_SELECT_RANGE );
int e_LightGetCloseFirst( LIGHT_TYPE eType, PROXNODE & nCurrent, const VECTOR & vPos, float fRange = DEFAULT_DYNAMIC_LIGHT_SELECT_RANGE );
int e_LightGetCloseNext( LIGHT_TYPE eType, PROXNODE & nCurrent );

void e_LightSelectClearList();
void e_LightSelectAddToList( int nLightID, float fWeight );
void e_LightSelectSortList( BOOL bAscending = FALSE );
void e_LightSelectGetFromList( int * pnLightIDs, int & nCount, int nMaxCount, DWORD dwFlagsWant = 0, DWORD dwFlagsDontWant = 0 );
PRESULT e_LightSelectAndSortPointList(
	struct BOUNDING_SPHERE & tBS,
	const int * pnLights,
	int nLights,
	int * pnOutLights,
	int & nOutLights,
	int nMax,
	DWORD dwFlagsDoWant = 0,
	DWORD dwFlagsDontWant = 0 );
PRESULT e_LightSelectAndSortSpecList(
	const struct ENVIRONMENT_DEFINITION * pEnvDef,
	const int * pnLights,
	int nLights,
	int * pnOutLights,
	int & nOutLights,
	int nMax,
	DWORD dwFlagsDoWant = 0,
	DWORD dwFlagsDontWant = 0 );
float e_LightSpecularCalculateWeight(
	const struct D3D_LIGHT * pLight,
	VECTOR & vFocusPos,
	VECTOR & vViewDir,
	const struct ENVIRONMENT_DEFINITION * pEnvDef = NULL );
void e_LightsCommonInit();
void e_LightsCommonDestroy();
void e_LightSlotsClear( LIGHT_SLOT_LIGHT_TYPE eType );
void e_LightSlotsUpdate();

PRESULT e_LightSpecularAddToWorld(
	const STATIC_LIGHT_DEFINITION * pLightDef,
	VECTOR & vPos );

PRESULT e_LightSpecularSelectNearest(
	const VECTOR & vFrom,
	float fRange,
	int * pnLights,
	int nMaxLights,
	int & nSelectedLights );

BOOL e_ToggleDrawAllLights();
BOOL e_GetDrawAllLights();
void e_DebugDrawLights();

BOOL LightDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad );

#endif // __E_LIGHT__
