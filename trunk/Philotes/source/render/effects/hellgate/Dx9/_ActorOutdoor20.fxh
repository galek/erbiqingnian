#include "Dx9/_ActorCommon.fxh"

#define SHADER_VERSION PS_2_0

#ifndef _SPOTLIGHT
#define _SPOTLIGHT _OFF
#endif

#ifndef TECH
#define TECH TECH_SL
#endif

#define TECH_NULL(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll)


#define _TECH_SHDW(shadowtype,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll) \
	TECHNIQUE_ACTOR(0,0,shadowtype,skinned,_SPOTLIGHT,_OFF,normalmap,selfillum,dir0,dir1,sh9,sh4,speculardir,_OFF,_OFF,specularpts,diffusepts,psworldnmap,cubemap,spheremap,_OFF,scroll)


#ifdef DX10_NO_SHADOW_TYPE_COLOR
#	define TECH_SL(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_NONE, skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_DEPTH,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll)
#else
#	define TECH_SL(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_NONE, skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_DEPTH,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_COLOR,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,spheremap,scroll)
#endif


#include "Dx9/_ActorCommonTechnique.fxh"

//			skin	nrm		ilum	dir0	dir1	sh9		sh4		difpts	spcpts	spcdr	pswrl	cube	sphere	scrl

TECH_SL	(	0,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		0,		0	 )
TECH_SL	(	_ON,	0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		0,		0	 )
//TECH	(	0,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		_ON,	0,		0	 )
//TECH	(	_ON,	0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		_ON,	0,		0	 )
TECH	(	0,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		_ON,	0	 )
TECH	(	_ON,	0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		_ON,	0	 )
TECH_SL	(	0,		0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		0,		0	 )
TECH_SL	(	_ON,	0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		0,		0	 )
//TECH	(	0,		0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		_ON,	0,		0	 )
//TECH	(	_ON,	0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		_ON,	0,		0	 )
TECH	(	0,		0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		_ON,	0	 )
TECH	(	_ON,	0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		_ON,	0	 )
TECH_SL	(	0,		0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		0,		_ON	 )
TECH_SL	(	_ON,	0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		0,		_ON	 )
//TECH	(	0,		0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		_ON,	0,		_ON	 )
//TECH	(	_ON,	0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		_ON,	0,		_ON	 )
TECH	(	0,		0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		_ON,	_ON	 )
TECH	(	_ON,	0,		_ON,	_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0,		_ON,	_ON	 )
