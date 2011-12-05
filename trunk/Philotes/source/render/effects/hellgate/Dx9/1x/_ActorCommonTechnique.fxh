// SM 1.x actor shader common technique definition

//-----------------------------------------------------------------------------

#define TECH11_SL(nPointLights, nDirLights, bSkinned, bSphericalHarmonics, bSelfIllum, bSpotlight) \
DECLARE_TECH TActor_1X_##nPointLights##_Skinned##bSkinned##_SH##bSphericalHarmonics##_SelfIllum##bSelfIllum##_Spotlight##bSpotlight \
	< \
		SHADER_VERSION_11\
		int PointLights				= nPointLights; \
		int Skinned					= bSkinned; \
		int SphericalHarmonics		= bSphericalHarmonics; \
		int SelfIllum				= bSelfIllum; \
		int WorldSpaceLight			= 1; \
		int SpotLight				= bSpotlight; \
	> \
{ \
	pass P0 \
	{ \
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS11_COMMON_ANIMATED(\
			nPointLights,					/* nPointLights */			\
			nDirLights,						/* nDirectionalLights */	\
			bSphericalHarmonics ? 4 : 0,	/* nSHLevel */				\
			bSelfIllum,						/* bSelfIllum */			\
			bSkinned,						/* bSkinned */				\
			bSpotlight						/* bSpotlight */			\
		)); \
		COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS11_COMMON(\
			false,							/* bLightMap */				\
			false,							/* bParticleLightMap */		\
			bSelfIllum,						/* bSelfIllum */			\
			false,							/* bAmbientOcclusion */		\
			bSpotlight						/* bSpotlight */			\
		)); \
	} \
}

#define TECH11(nPointLights, nDirLights, bSkinned, bSphericalHarmonics, bSelfIllum) \
	TECH11_SL(nPointLights, nDirLights, bSkinned, bSphericalHarmonics, bSelfIllum, _OFF)
