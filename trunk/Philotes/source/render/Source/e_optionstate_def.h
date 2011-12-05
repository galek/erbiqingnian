//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

/*
	OptionState is a state object that houses all interdependent
	engine and graphics options.

	DO put in here:
		*	Settings that need to be validated.
		*	Settings that influence other settings.
		*	Settings that are influenced by other settings.
		*	Settings needed to complete device creation (because
				final caps are not available until the device is
				created and the caps may influence some settings.)
		*	Settings that control engine behavior at run time.

	DO NOT put in here:
		*	Constants.
		*	Defaults.
		*	Flags for querying only.
*/

#define DEF_PARM(type, name, pers, defer) DEF_PARMT(type, name, pers, defer, type, type)

//			Storage									Persis-
//			Type		Name						tent	Defer	DX9 Type				DX10 Type
//			-------		----						-------	-------	--------				---------

// Super Starter Settings
// CHB 2007.03.05 - Although there is no adapter ordinal on DX10,
// leave this in to prevent potential compile-time discrepancies
// due to ENGINE_TARGET_DX9 being defined in one place but not
// another.
//#ifdef ENGINE_TARGET_DX9
DEF_PARM(	unsigned,	nAdapterOrdinal,			false,	false												)	// currently determined at runtime
//#endif

// Starter Settings
DEF_PARMT(	unsigned,	nDeviceDriverType,			false,	false,	D3DC_DRIVER_TYPE,		D3DC_DRIVER_TYPE	)	// currently determined at runtime
DEF_PARMT(	unsigned,	nFrameBufferColorFormat,	false,	false,	DXC_FORMAT,				DXC_FORMAT			)	// currently determined at runtime
DEF_PARMT(	unsigned,	nAdapterColorFormat,		false,	false,	DXC_FORMAT,				DXC_FORMAT			)	// currently determined at runtime
DEF_PARM(	int,		nFrameBufferWidth,			true,	false												)
DEF_PARM(	int,		nFrameBufferHeight,			true,	false												)
DEF_PARM(	unsigned,	nScreenRefreshHzNumerator,	true,	false												)
DEF_PARM(	unsigned,	nScreenRefreshHzDenominator,true,	false												)
DEF_PARM(	unsigned,	nScanlineOrdering,			true,	false												)
DEF_PARM(	unsigned,	nScaling,					true,	false												)
DEF_PARM(	bool,		bFlipWaitVerticalRetrace,	true,	false												)
DEF_PARM(	bool,		bWindowed,					true,	false												)
DEF_PARM(	unsigned,	nBackBufferCount,			false,	false												)	// currently determined at runtime
DEF_PARM(	bool,		bTripleBuffer,				true,	false												)
DEF_PARMT(	int,		nMultiSampleType,			false,	false,	DXC_MULTISAMPLE_TYPE,	DXC_MULTISAMPLE_TYPE)	// persistence maintained by e_featureline
DEF_PARM(	int,		nMultiSampleQuality,		false,	false												)	// persistence maintained by e_featureline

// General Settings
//DEF_PARM(	DWORD,		dwVertexShaderVersion,		false,	false												)	// dependent on other runtime influences
//DEF_PARM(	DWORD,		dwPixelShaderVersion,		false,	false												)	// dependent on other runtime influences
DEF_PARM(	int,		nShadowType,				false,	false												)	// persistence maintained by e_featureline
DEF_PARM(	int,		nShadowReductionFactor,		false,	false												)	// persistence maintained by e_featureline
DEF_PARM(	int,		nMaxEffectTarget,			false,	true												)	// persistence maintained by e_featureline
DEF_PARM(	float,		fEffectLODBias,				false,	true												)	// persistence maintained by e_featureline
DEF_PARM(	float,		fEffectLODScale,			false,	true												)	// persistence maintained by e_featureline
DEF_PARM(	bool,		bGlow,						false,	false												)	// currently determined at runtime
DEF_PARM(	bool,		bSpecular,					false,	false												)	// currently determined at runtime
DEF_PARM(	float,		fTextureQuality,			false,	true												)	// [0.0, 1.0]; persistence maintained by e_featureline
DEF_PARM(	float,		fGammaPower,				true,	false												)	// raw gamma power
DEF_PARM(	bool,		bDepthOnlyPass,				false,	false												)	// currently determined at runtime
DEF_PARM(	float,		fLODBias,					false,	false												)
DEF_PARM(	float,		fLODScale,					false,	false												)
DEF_PARM(	bool,		bAnisotropic,				true,	false												)
DEF_PARM(	bool,		bDynamicLights,				true,	false												)
DEF_PARM(	bool,		bTrilinear,					true,	false												)
DEF_PARM(	float,		fCullSizeScale,				false,	false												)	// persistence maintained by e_featureline
DEF_PARM(	int,		nBackgroundLoadForceLOD,	false,	true												)	// one of LOD_NONE, LOD_LOW, LOD_HIGH; persistence maintained by e_featureline
DEF_PARM(	int,		nAnimatedLoadForceLOD,		false,	true												)	// one of LOD_NONE, LOD_LOW, LOD_HIGH; persistence maintained by e_featureline
DEF_PARM(	int,		nBackgroundShowForceLOD,	false,	false												)	// one of LOD_NONE, LOD_LOW, LOD_HIGH; persistence maintained by e_featureline
DEF_PARM(	int,		nAnimatedShowForceLOD,		false,	false												)	// one of LOD_NONE, LOD_LOW, LOD_HIGH; persistence maintained by e_featureline
DEF_PARM(	bool,		bAsyncEffects,				true,	false												)
DEF_PARM(	bool,		bFluidEffects,				true,	false												)	// DX10 only
DEF_PARM(	bool,		bDX10ScreenFX,				false,	false												)	// persistence maintained by e_featureline
DEF_PARM(	int,		nWardrobeLimit,				false,	false												)	// count of how many custom wardrobes can exist; persistence maintained by e_featureline
