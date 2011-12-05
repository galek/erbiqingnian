#include "Dx9/_ActorCommon.fxh"

#define SHADER_VERSION PS_2_0

#ifndef _SPOTLIGHT
#define _SPOTLIGHT _OFF
#endif

#ifndef TECH
#define TECH TECH_SL
#endif

#define TECH_NULL(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll)


#define _TECH_SHDW(shadowtype,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll) \
	TECHNIQUE_ACTOR(0,0,shadowtype,skinned,_SPOTLIGHT,_ON,normalmap,selfillum,_OFF,_OFF,sh9,sh4,_OFF,specularmap,_OFF,_OFF,_OFF,psworldnmap,cubemap,0,_OFF,scroll)


#ifdef DX10_NO_SHADOW_TYPE_COLOR
#	define TECH_SL(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_NONE, skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_DEPTH,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll)
#else
#	define TECH_SL(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_NONE, skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_DEPTH,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_COLOR,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,specularmap,psworldnmap,cubemap,scroll)
#endif


#include "Dx9/_ActorCommonTechnique.fxh"

//			skin	nrm		ilum	dir0	dir1	sh9		sh4		difpts	spcpts	spcdr	spcmap	pswrl	cube	scrl

TECH_SL	(	0,		0,		0,		0,		0,		_ON_VS,	0,		0,		0,		0,		0,		0,		0,		0	 )
TECH_SL	(	_ON,	0,		0,		0,		0,		_ON_VS,	0,		0,		0,		0,		0,		0,		0,		0	 )
TECH_SL	(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		0,		0,		0,		0	 )
TECH_SL	(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		0,		0,		0,		0	 )
TECH_SL	(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		0,		0,		0,		_ON	 )
TECH_SL	(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		0,		0,		0,		_ON	 )

TECH	(	0,		0,		0,		0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		0,		0	 )
TECH	(	_ON,	0,		0,		0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		0,		0	 )
TECH	(	0,		0,		0,		0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		_ON,	0	 )
TECH	(	_ON,	0,		0,		0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		_ON,	0	 )
TECH	(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		0,		0	 )
TECH	(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		0,		0	 )
TECH	(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		_ON,	0	 )
TECH	(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		_ON,	0	 )
TECH	(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		0,		_ON	 )
TECH	(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		0,		_ON	 )
TECH	(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		_ON,	_ON	 )
TECH	(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		0,		0,		0,		_ON,	0,		_ON,	_ON	 )
