//----------------------------------------------------------------------------
// dxC_effect_feature_def.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#if defined( INCLUDE_TECHFEAT_INT_ENUM )
#	define TECHFEAT_FLAG(str,cost,flag,edflag)
#	define TECHFEAT_INT(str,costb,costp,bmin,enumm)		TF_INT_##enumm,
#elif defined( INCLUDE_TECHFEAT_INT )
#	define TECHFEAT_FLAG(str,cost,flag,edflag)
#	define TECHFEAT_INT(str,costb,costp,bmin,enumm)		{ str, costb, costp, bmin },
#elif defined( INCLUDE_TECHFEAT_FLAG )
#	define TECHFEAT_FLAG(str,cost,flag,edflag)			{ str, cost, flag, edflag },
#	define TECHFEAT_INT(str,costb,costp,bmin,enumm)
#endif

//				string					cost (base)	cost (per)	match min	TF_INT_*						
//										( -1 for																		
//										 no subst)

TECHFEAT_INT( "Index",					-1,			0,			FALSE,		INDEX			 					)
TECHFEAT_INT( "PointLights",			0,			10,			TRUE,		MODEL_POINTLIGHTS					)
TECHFEAT_INT( "ShadowType",				15,			0,			FALSE,		MODEL_SHADOWTYPE					)

//				string					cost (base)		TF_FLAG_*									EFFECTDEF_FLAGBIT
//										( -1 for													contribution
//										 no subst)

TECHFEAT_FLAG( "VertexFormat",			-1,				TECHNIQUE_FLAG_MODEL_32BYTE_VERTEX			,-1			)
TECHFEAT_FLAG( "SpotLight",				0,				TECHNIQUE_FLAG_MODEL_SPOTLIGHT				,-1			)	// CHB 2007.07.16
TECHFEAT_FLAG( "NormalMap",				20,				TECHNIQUE_FLAG_MODEL_NORMALMAP	 			,-1			)
TECHFEAT_FLAG( "Specular",				30,				TECHNIQUE_FLAG_MODEL_SPECULAR	 			,-1			)
TECHFEAT_FLAG( "SpecularLUT",			-1,				TECHNIQUE_FLAG_MODEL_SPECULAR_LUT	 		,EFFECTDEF_FLAGBIT_SPECULAR_LUT			)
TECHFEAT_FLAG( "Skinned",				-1,				TECHNIQUE_FLAG_MODEL_SKINNED	 			,EFFECTDEF_FLAGBIT_ANIMATED				)
TECHFEAT_FLAG( "LightMap",				3,				TECHNIQUE_FLAG_MODEL_LIGHTMAP	 			,-1			)
TECHFEAT_FLAG( "DiffuseMap2",			5,				TECHNIQUE_FLAG_MODEL_DIFFUSEMAP2 			,-1			)
TECHFEAT_FLAG( "SphericalHarmonics",	15,				TECHNIQUE_FLAG_MODEL_SPHERICAL_HARMONICS	,-1			)
TECHFEAT_FLAG( "WorldSpaceLight",		7,				TECHNIQUE_FLAG_MODEL_WORLDSPACELIGHTS		,-1			)
TECHFEAT_FLAG( "CubeEnvMap",			20,				TECHNIQUE_FLAG_MODEL_CUBE_ENVMAP			,-1			)
TECHFEAT_FLAG( "SphereEnvMap",			20,				TECHNIQUE_FLAG_MODEL_SPHERE_ENVMAP			,-1			)
TECHFEAT_FLAG( "SelfIllum",				3,				TECHNIQUE_FLAG_MODEL_SELFILLUM				,-1			)
TECHFEAT_FLAG( "ScrollUV",				20,				TECHNIQUE_FLAG_MODEL_SCROLLUV				,-1			)
TECHFEAT_FLAG( "Scatter",				10,				TECHNIQUE_FLAG_MODEL_SCATTERING				,-1			)

TECHFEAT_FLAG( "Additive",				-1,				TECHNIQUE_FLAG_PARTICLE_ADDITIVE			,-1			)
TECHFEAT_FLAG( "Glow",					-1,				TECHNIQUE_FLAG_PARTICLE_GLOW				,-1			)
TECHFEAT_FLAG( "GlowConstant",			-1,				TECHNIQUE_FLAG_PARTICLE_GLOW_CONSTANT		,-1			)
TECHFEAT_FLAG( "SoftParticles",			-1,				TECHNIQUE_FLAG_PARTICLE_SOFT_PARTICLES		,-1			)
TECHFEAT_FLAG( "Multiply",				-1,				TECHNIQUE_FLAG_PARTICLE_MULTIPLY			,-1			)



#undef TECHFEAT_FLAG
#undef TECHFEAT_INT
