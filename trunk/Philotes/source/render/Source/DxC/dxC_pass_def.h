//----------------------------------------------------------------------------
// dxC_pass_def.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef INCLUDE_RPD_ARRAY


static void sRPD_Depth1P( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_DEPTH;
	SETBIT( tDef.dwModelFlags_MustHaveAll	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= SORT_FRONT_TO_BACK;
}

static void sRPD_DepthScene( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_DEPTH;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= SORT_FRONT_TO_BACK;
}

static void sRPD_DepthAlphaScene( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_DEPTH | PASS_FLAG_ALPHABLEND;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	tDef.eMeshSort							= SORT_BACK_TO_FRONT;
}

static void sRPD_Opaque1P( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= 0;
	SETBIT( tDef.dwModelFlags_MustHaveAll	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ? SORT_STATE : SORT_FRONT_TO_BACK;
}

static void sRPD_Alpha1P( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_ALPHABLEND;
	SETBIT( tDef.dwModelFlags_MustHaveAll	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	tDef.eMeshSort							= SORT_BACK_TO_FRONT;
}

static void sRPD_OpaqueScene( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_FOG_ENABLED;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ? SORT_STATE : SORT_FRONT_TO_BACK;
}

static void sRPD_OpaqueBehind( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_BEHIND;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustHaveAll	, MODEL_FLAGBIT_DRAWBEHIND );
	tDef.eMeshSort							= SORT_NONE;
}

static void sRPD_AlphaScene( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_ALPHABLEND | PASS_FLAG_FOG_ENABLED;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	tDef.eMeshSort							= SORT_BACK_TO_FRONT;
}

static void sRPD_OpaqueSkybox( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_SKYBOX;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= SORT_FRONT_TO_BACK;
	tDef.eDrawLayer							= DRAW_LAYER_ENV;
}

static void sRPD_AlphaSkybox( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_SKYBOX | PASS_FLAG_ALPHABLEND;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	tDef.eMeshSort							= SORT_BACK_TO_FRONT;
	tDef.eDrawLayer							= DRAW_LAYER_ENV;
}

static void sRPD_ParticlesGeneral( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_PARTICLES;
	tDef.eDrawLayer							= DRAW_LAYER_GENERAL;
}

static void sRPD_ParticlesEnv( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_PARTICLES;
	tDef.eDrawLayer							= DRAW_LAYER_ENV;
}

static void sRPD_OpaquePreBlob( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_FOG_ENABLED;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_ANIMATED );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ? SORT_STATE : SORT_FRONT_TO_BACK;
}

static void sRPD_OpaqueBlob( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_BLOB_SHADOWS;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_ATTACHMENT );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= SORT_FRONT_TO_BACK;
}

static void sRPD_OpaquePostBlob( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_FOG_ENABLED;
	SETBIT( tDef.dwModelFlags_MustHaveSome	, MODEL_FLAGBIT_ANIMATED );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ? SORT_STATE : SORT_FRONT_TO_BACK;
}


static void sRPD_OpaquePreBlobBehind( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_FOG_ENABLED;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_ANIMATED );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_DRAWBEHIND );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ? SORT_STATE : SORT_FRONT_TO_BACK;
}

static void sRPD_OpaquePostBlobBehind( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_FOG_ENABLED;
	SETBIT( tDef.dwModelFlags_MustHaveSome	, MODEL_FLAGBIT_ANIMATED );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FORCE_ALPHA );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_DRAWBEHIND );
	tDef.dwMeshFlags_MustNotHave			= MESH_FLAG_ALPHABLEND;
	tDef.eMeshSort							= e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ? SORT_STATE : SORT_FRONT_TO_BACK;
}

static void sRPD_OpaquePreBehind( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_FOG_ENABLED;
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_DRAWBEHIND );
	tDef.eMeshSort							= e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ? SORT_STATE : SORT_FRONT_TO_BACK;
}

static void sRPD_OpaquePostBehind( RenderPassDefinition & tDef )
{
	tDef.dwPassFlags						= PASS_FLAG_FOG_ENABLED;
	SETBIT( tDef.dwModelFlags_MustHaveSome	, MODEL_FLAGBIT_DRAWBEHIND );
	SETBIT( tDef.dwModelFlags_MustNotHave	, MODEL_FLAGBIT_FIRST_PERSON_PROJ );
	tDef.eMeshSort							= e_GetRenderFlag( RENDER_FLAG_ZPASS_ENABLED ) ? SORT_STATE : SORT_FRONT_TO_BACK;
}

#endif // INCLUDE_RPD_ARRAY



//----------------------------------------------------------------------------
// Pass definitions
//----------------------------------------------------------------------------

#ifdef INCLUDE_RPD_ARRAY
#	define _RPD_START()					PFN_RPD_CREATE	gpfn_RPDCreate[ NUM_RENDER_PASS_TYPES ] = {
#	define _RPD( name, pfn, alpha )		pfn,
#	define _RPD_END()					};
#	undef INCLUDE_RPD_ARRAY
#elif defined(INCLUDE_RPD_ALPHA)		
#	define _RPD_START()
#	define _RPD( name, pfn, alpha )		alpha,
#	define _RPD_END()
#	undef INCLUDE_RPD_ALPHA
#elif defined(INCLUDE_RPD_ENUM)
#	define _RPD_START()
#	define _RPD( name, pfn, alpha )		name,
#	define _RPD_END()
#	undef INCLUDE_RPD_ENUM
#elif defined(INCLUDE_RPD_ENUM_STR)
#	define _RPD_START()
#	define _RPD( name, pfn, alpha )		#name,
#	define _RPD_END()
#	undef INCLUDE_RPD_ENUM_STR
#endif


_RPD_START()
//    name								def func					alpha
_RPD( RPTYPE_OPAQUE_1P,					sRPD_Opaque1P,				FALSE			)
_RPD( RPTYPE_ALPHA_1P,					sRPD_Alpha1P,				TRUE			)
_RPD( RPTYPE_OPAQUE_SCENE,				sRPD_OpaqueScene,			FALSE			)
_RPD( RPTYPE_ALPHA_SCENE,				sRPD_AlphaScene,			TRUE			)
_RPD( RPTYPE_OPAQUE_SKYBOX,				sRPD_OpaqueSkybox,			FALSE			)
_RPD( RPTYPE_ALPHA_SKYBOX,				sRPD_AlphaSkybox,			TRUE			)
_RPD( RPTYPE_PARTICLES_GENERAL,			sRPD_ParticlesGeneral,		TRUE			)
_RPD( RPTYPE_PARTICLES_ENV,				sRPD_ParticlesEnv,			TRUE			)
_RPD( RPTYPE_OPAQUE_PRE_BLOB,			sRPD_OpaquePreBlob,			FALSE			)
_RPD( RPTYPE_OPAQUE_BLOB,				sRPD_OpaqueBlob,			FALSE			)
_RPD( RPTYPE_OPAQUE_POST_BLOB,			sRPD_OpaquePostBlob,		FALSE			)
_RPD( RPTYPE_DEPTH_1P,					sRPD_Depth1P,				FALSE			)
_RPD( RPTYPE_DEPTH_SCENE,				sRPD_DepthScene,			FALSE			)
_RPD( RPTYPE_DEPTH_ALPHA_SCENE,			sRPD_DepthAlphaScene,		TRUE			)
_RPD( RPTYPE_OPAQUE_BEHIND,				sRPD_OpaqueBehind,			FALSE			)
_RPD( RPTYPE_OPAQUE_PRE_BEHIND,			sRPD_OpaquePreBehind,		FALSE			)
_RPD( RPTYPE_OPAQUE_POST_BEHIND,		sRPD_OpaquePostBehind,		FALSE			)
_RPD( RPTYPE_OPAQUE_PRE_BLOB_BEHIND,	sRPD_OpaquePreBlobBehind,	FALSE			)
_RPD( RPTYPE_OPAQUE_POST_BLOB_BEHIND,	sRPD_OpaquePostBlobBehind,	FALSE			)
_RPD_END()


#undef _RPD_START
#undef _RPD
#undef _RPD_END