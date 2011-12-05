//----------------------------------------------------------------------------
// dx9_light.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_LIGHT__
#define __DX9_LIGHT__
//KMNV TODO HACK: dx9 pollution, file was merely moved
#include "proxmap.h"
#include "e_light.h"
#include "appcommontimer.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define LIGHT_UPDATE_RATE_MIN_SCREENSIZE	0.08f
#define LIGHT_UPDATE_RATE_MIN_HZ			15.f
#define LIGHT_UPDATE_RATE_MAX_SCREENSIZE	0.2f
#define LIGHT_UPDATE_RATE_MAX_HZ			60.f


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum
{
	LIGHT_MODEL_NOT_SELECTED = 0,
	LIGHT_MODEL_SELECTED,
	NUM_LIGHT_MODELS
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct D3D_MODEL;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct D3D_LIGHT
{
	int			nId;
	D3D_LIGHT*	pNext;

	LIGHT_TYPE	eType;
	DWORD		dwFlags;
	int			nLightDef;
	PROXNODE	nProxmapId;		// ID of node in proximity map

	float		fBeginTime;
	float		fDeathTime; // only used by DURATION_LOOPS_RUN_TO_END
	float		fLifePercent;

	VECTOR4		vDiffuse;
	VECTOR4		vFalloff;
	float		fFalloffStart;
	float		fFalloffEnd;

	float		fPriority;
	float		fPriorityBonus;

	VECTOR		vPosition;
	VECTOR		vPositionOffset;
	VECTOR		vDirection;
	DWORD		dwGroup;
};

template<int MAX>
struct MODEL_RENDER_LIGHT_BLOCK
{
	int nLights;
	D3DXVECTOR4 pvPosWorld		[ MAX ];
	D3DXVECTOR4 pvPosObject		[ MAX ];
	D3DXVECTOR4 pvColors		[ MAX ];
	D3DXVECTOR4 pvFalloffsWorld [ MAX ];
	D3DXVECTOR4 pvFalloffsObject[ MAX ];

	void Clear(void)
	{
		nLights = 0;
	}

	void ResetForSpecular(void)
	{
		for ( int i = 0; i < MAX; i++ )
		{
			pvPosWorld		[ i ] = D3DXVECTOR4( 10000.0f, 10000.0f, 10000.0f, 0.0f );
			pvPosObject		[ i ] = D3DXVECTOR4( 10000.0f, 10000.0f, 10000.0f, 0.0f );
			pvColors		[ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f, -1.0f );
			pvFalloffsWorld	[ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f,  0.0f );
			pvFalloffsObject[ i ] = D3DXVECTOR4( 0.0f, 0.0f, 0.0f,  0.0f );
		}
	}
};

struct MODEL_RENDER_LIGHT_DATA
{
	enum FLAGBITS
	{
		ENABLE_SH = 0,
		ENABLE_SPECULAR_SELECT,
		ENABLE_SPECULAR_SLOT,
		ENABLE_POINT_SELECT,
		ENABLE_POINT_SLOT,
		NEVER_UPDATE,
		NUM_FLAGBITS
	};

	enum STATE
	{
		STATE_FILLED_LIST_POINTSH = 0,
		STATE_FILLED_LIST_SPEC,
		STATE_SELECTED_POINT,
		STATE_SELECTED_SPEC,
		STATE_ASSEMBLED_POINT,
		STATE_ASSEMBLED_GLOBAL_POINTS,
		STATE_ASSEMBLED_GLOBAL_SPECS,
		STATE_ASSEMBLED_SH,
		STATE_INITIALIZED_SH_O2,
		STATE_INITIALIZED_SH_O3,
		STATE_COMPUTED_SH_O2,
		STATE_COMPUTED_SH_O3,
	};

	DWORD dwStateFlagbits;
	DWORD dwEnableFlagbits;

	TIME tLastUpdate;
	TIMEDELTA tUpdateDelay;
	DWORD dwUpdateToken;
	DWORD dwCurrentToken;

	int nLights;
	int pnLights[ MAX_LIGHTS_PER_EFFECT ];		// global lights potentially-affecting list (from proxmap query)

	SH_COEFS_RGB<2> tSHCoefsBase_O2;											// all but the top x point lights
	SH_COEFS_RGB<2> tSHCoefsPointLights_O2[ MAX_POINT_LIGHTS_PER_EFFECT ];		// the top x point lights, each on their own

	SH_COEFS_RGB<3> tSHCoefsBase_O3;											// all but the top x point lights
	SH_COEFS_RGB<3> tSHCoefsPointLights_O3[ MAX_POINT_LIGHTS_PER_EFFECT ];		// the top x point lights, each on their own

	// Speculars
	typedef MODEL_RENDER_LIGHT_BLOCK<MAX_SPECULAR_LIGHTS_PER_EFFECT> SPECULARS;
	SPECULARS GlobalSpec;	// global specular slot lights
	SPECULARS ModelSpec;	// per-model specular-only lights

	// Points
	typedef MODEL_RENDER_LIGHT_BLOCK<MAX_POINT_LIGHTS_PER_EFFECT> POINTS;
	POINTS GlobalPoint;		// global point slot lights
	POINTS ModelPoint;		// per-model selected lights
	float pfModelPointLightLODStrengths[ MAX_POINT_LIGHTS_PER_EFFECT ];

	// it'd be nice to not have *any* limits, even just simple memory limits, but this lets us not check and bake lights per mesh
	// we cache these because we could be doing order 2 on one mesh and order 3 on a later mesh
	int nSHLights;
	D3DXVECTOR4 pvSHLightPos   [ MAX_SH_LIGHTS_PER_EFFECT ];
	D3DXVECTOR4 pvSHLightColors[ MAX_SH_LIGHTS_PER_EFFECT ];

	void Clear()
	{
		dwStateFlagbits			= 0;
		GlobalSpec.Clear();
		GlobalPoint.Clear();
		ModelSpec.Clear();
		ModelPoint.Clear();
		nSHLights				= 0;
		nLights					= 0;
	}

	void Init()
	{
		dwEnableFlagbits		= 0;
		Clear();
		// force the first update
		dwUpdateToken			= 1;
		dwCurrentToken			= 0;
		tUpdateDelay			= 1000;
	}

	void SetFlag( FLAGBITS eFlagbit, BOOL bEnabled )
	{
		SETBIT( dwEnableFlagbits, eFlagbit, bEnabled );
	}

	bool IsState( STATE eState )
	{
		return TESTBIT_DWORD( dwStateFlagbits, eState );
	}

	void SetState( STATE eState, bool bValue = TRUE )
	{
		SETBIT( dwStateFlagbits, eState, bValue );
	}

	void SetUpdateRate( float fUpdatesPerSecond )
	{
		if ( fUpdatesPerSecond <= 0.f )
			tUpdateDelay = 0;
		else
			tUpdateDelay = (TIMEDELTA)( 1.f / fUpdatesPerSecond * MSECS_PER_SEC );
	}

	void SetUpdateRateByScreensize( float fScreensize )
	{
		fScreensize = MIN( LIGHT_UPDATE_RATE_MAX_SCREENSIZE, MAX( LIGHT_UPDATE_RATE_MIN_SCREENSIZE, fScreensize ) );
		SetUpdateRate( MAP_VALUE_TO_RANGE( fScreensize, LIGHT_UPDATE_RATE_MIN_SCREENSIZE, LIGHT_UPDATE_RATE_MAX_SCREENSIZE, LIGHT_UPDATE_RATE_MIN_HZ, LIGHT_UPDATE_RATE_MAX_HZ ) );
	}

	void SetForceUpdate()
	{
		// force next update
		dwUpdateToken = dwCurrentToken+1;
	}

	void CheckUpdate()
	{
		dwCurrentToken++;

		if ( TESTBIT_DWORD( dwEnableFlagbits, NEVER_UPDATE ) )
			return;

		TIME tCur = AppCommonGetCurTime();
		TIMEDELTA tDelta = tCur - tLastUpdate;
		if ( tDelta > tUpdateDelay )
		{
			// Should update!  Match the token and update the timer.

			dwUpdateToken = dwCurrentToken;
			tLastUpdate = tCur;
			Clear();
		}
	}

	inline BOOL ShouldUpdateType( FLAGBITS eFlagbit )
	{
		if ( dwUpdateToken != dwCurrentToken )
			return FALSE;
		REF( eFlagbit );
		//if ( ! TESTBIT_DWORD( dwEnableFlagbits, eFlagbit ) )
		//	return FALSE;
		return TRUE;
	}
};

struct MESH_RENDER_LIGHT_DATA
{
	//DWORD dwFlags;

	int nRealLights;			// number of "real" lights of the x shader lights
	int nSplitLights;			// number of "split" lights of the x shader lights -- must equal 0 or 1

	// for the one split light, this is the relative strengths of the "real" and "SH" portions
	float fSplitRealStr;
	float fSplitSHStr;
};


//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dx9_LightsInit();
PRESULT dx9_LightsDestroy();
float dx9_LightGetPriority( D3D_LIGHT * pLight );
PRESULT dx9_MakePointLightList(
	int * pnLights,
	int & nCount,
	const VECTOR & vFromPos,
	DWORD dwFlagsMustList,
	DWORD dwFlagsDontList,
	int nMax );
void dx9_LightApplyDefinition( D3D_LIGHT * pLight );

PRESULT dxC_ComputeLightsSH_O2(
	const struct D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights );
PRESULT dxC_ComputeLightsSH_O3(
	const struct D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights );
PRESULT dxC_AssembleLightsPoint(
	const struct D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights );
PRESULT dxC_AssembleLightsGlobalPoint(
	const struct D3D_MODEL * pModel,
	MODEL_RENDER_LIGHT_DATA & tLights );
PRESULT dxC_AssembleLightsGlobalSpec(
	const struct D3D_MODEL * pModel,
	const struct ENVIRONMENT_DEFINITION * pEnvDef,
	MODEL_RENDER_LIGHT_DATA & tLights );
PRESULT dxC_AssembleLightsSH(
	struct BOUNDING_SPHERE & tBS,
	float fScale,
	MODEL_RENDER_LIGHT_DATA & tLights );
PRESULT dxC_SelectLightsSpecularOnly(
	const struct D3D_MODEL * pModel,
	const struct DRAWLIST_STATE * pState,
	const struct ENVIRONMENT_DEFINITION * pEnvDef,
	MODEL_RENDER_LIGHT_DATA & tLights );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline D3D_LIGHT * dx9_LightGet( int nLightId )
{
	extern CHash<D3D_LIGHT> gtLights;
	return HashGet( gtLights, nLightId );
}

#endif // __DX9_LIGHT__