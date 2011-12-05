// e_effect_priv.h

#ifdef RENDER_EFFECT_ENUM
#define RFX_DEFINE( _type, _class, _excl, _bound, _pfnbegin, _pfnend )		_type,
#else 
#define RFX_DEFINE( _type, _class, _excl, _bound, _pfnbegin, _pfnend )		{ _type, _class, _bound, _excl, 0, _pfnbegin, _pfnend },
#endif

		  // TYPE					// CLASS					// EXCL	// BOUNDED	// PFNBEGIN					// PFNEND
RFX_DEFINE(RFX_NULL,				RFX_CLASS_NONE,				0,		0,			NULL,						NULL )
//RFX_DEFINE(RFX_FULLSCREENGLOW,		RFX_CLASS_POSTPROCESS,		1,		0,			pfnFullScreenGlow,			pfnFullScreenGlow )			// full screen blur and glow masked by framebuffer alpha (bloom)
//RFX_DEFINE(RFX_DYNAMICCUBEMAP,		RFX_CLASS_RENDERTOTEXTURE,	1,		1,			pfnRenderToCubeMapBegin,	pfnRenderToCubeMapEnd )		// render-to-cubic environment map for later use as env map texture
//RFX_DEFINE(RFX_RENDERTOTEXTURE,		RFX_CLASS_RENDERTOTEXTURE,	1,		1,			pfnRenderToTextureBegin,	pfnRenderToTextureEnd )		// generic render-to-texture
//RFX_DEFINE(RFX_PLANARREFLECTION,	RFX_CLASS_RENDERTOTEXTURE,	1,		1,			pfnRenderReflectionBegin,	pfnRenderReflectionEnd )	// render-to-texture for later use as reflection texture
//RFX_DEFINE(RFX_SPATIALWARP,			RFX_CLASS_MODIFIER,			0,		1,			pfnSpatialWarpBegin,		pfnSpatialWarpEnd )			// vertex-based spatial warp effect
//RFX_DEFINE(RFX_SCREENWARP,			RFX_CLASS_VISUAL,			0,		1,			pfnScreenWarpBegin,			pfnScreenWarpEnd )			// rendered polygons use special pixel shader and warp the screen


#undef RFX_DEFINE
