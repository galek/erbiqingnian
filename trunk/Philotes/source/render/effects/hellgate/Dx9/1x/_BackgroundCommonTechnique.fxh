// SM 1.x background shader common technique definition

//-----------------------------------------------------------------------------

#define TECH11_SL(nPointLights, nDirLights, bSH, bSelfIllum, bSpotlight) \
DECLARE_TECH TBackground_1X_##nPointLights##nDirLights##bSH##bSelfIllum##bSpotlight \
	< \
		SHADER_VERSION_11\
		int PointLights				= nPointLights; \
		int SelfIllum				= bSelfIllum; \
		int SphericalHarmonics		= bSH; \
		int WorldSpaceLight			= 1; \
		int SpotLight				= bSpotlight; \
	> \
{ \
	pass P0 \
	{ \
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS11_COMMON_RIGID(\
			nPointLights,					/* nPointLights */			\
			nDirLights,						/* nDirectionalLights */	\
			bSH ? 4 : 0,					/* bSH */					\
			true,							/* bLightMap */				\
			false,							/* bParticleLightMap */		\
			bSelfIllum,						/* bSelfIllum */			\
			true,							/* bAmbientOcclusion */		\
			bSpotlight						/* bSpotlight */			\
		)); \
		COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS11_COMMON(\
			true,							/* bLightMap */				\
			false,							/* bParticleLightMap */		\
			bSelfIllum,						/* bSelfIllum */			\
			true,							/* bAmbientOcclusion */		\
			bSpotlight						/* bSpotlight */			\
		)); \
	} \
}

#define TECH11(nPointLights, nDirLights, bSH, bSelfIllum) \
	TECH11_SL(nPointLights, nDirLights, bSH, bSelfIllum, _OFF)
