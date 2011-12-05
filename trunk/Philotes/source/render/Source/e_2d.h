//----------------------------------------------------------------------------
// e_2d.h
//
// - Header for 2D renders
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_2D__
#define __E_2D__

#include "e_fillpak.h"
#include "refcount.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define SCREENFX_PRIORITY_NORENDER		-1
#define SCREENFX_PRIORITY_NOTLOADED		-2

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

// MUST match dx9_shaderversionconstants.h
enum SCREEN_EFFECT_OP
{
	SFX_OP_COPY = 0,
	SFX_OP_SHADERS,
	// count
	NUM_SCREEN_EFFECT_OPS
};

enum SCREEN_EFFECT_TARGET
{
	SFX_RT_NONE = 0,
	SFX_RT_BACKBUFFER,
	SFX_RT_FULL,
	// count
	NUM_SCREEN_EFFECT_TARGETS
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

class CScreenEffects
{
public:
	enum TYPE
	{
		TYPE_FULLSCREEN = 0,
		TYPE_PARTICLE,
		// count
		NUM_TYPES
	};

	enum LAYER
	{
		LAYER_ON_TOP = 0,
		LAYER_PRE_BLOOM,
		// count
		NUM_LAYERS
	};

	PRESULT Init();
	PRESULT Destroy();

	PRESULT Update();
	PRESULT Render( TYPE eType, LAYER eLayer );
	UINT nFullScreenPasses();

	PRESULT SetActive( int nScreenEffectDef, BOOL bActive );

private:
	int m_nSourceIdentifier;
	int m_bNeedSort;

	struct INSTANCE
	{
		// sorting values
		int nPriority;
		BOOL bExclusive;
		REFCOUNT tRefCount;

		// data
		int nScreenEffectDef;

		enum STATE
		{
			INACTIVE = 0,
			TRANSITION_IN,
			ACTIVE,
			TRANSITION_OUT,
			DEAD,
		};
		STATE eState;
		float fTransitionRate;			// per second
		float fTransitionPhase;
	};
	SIMPLE_DYNAMIC_ARRAY<INSTANCE> m_tInstances;

	static int sCompareInstances( const void *, const void * );
	PRESULT PrepareSourceTexture( SCREEN_EFFECT_TARGET eSrc, SCREEN_EFFECT_TARGET eDest );
	PRESULT RestoreEffect( int nScreenEffectDef );
	PRESULT BeginTransitionIn( INSTANCE * pInstance, struct SCREEN_EFFECT_DEFINITION * pDef );
	PRESULT ApplyEffect( TYPE eType, LAYER eLayer, int nIndex, BOOL& bContinue );
	int GetIndexByDefID( int nDefID );
	PRESULT RemoveInstance( int nIndex );

	void SetNeedSort()			{ m_bNeedSort = TRUE; }
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern CScreenEffects gtScreenEffects;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_ScreenEffectSetActive( int nScreenEffect, BOOL bActive );
PRESULT e_ScreenFXRestore( void * pDef );
PRESULT e_DOFSetParams(float fNear, float fFocus, float fFar, float fFrames = 0 );
PRESULT e_LoadNonStateScreenEffects( BOOL bSetActive = FALSE );

#endif // __E_2D__
