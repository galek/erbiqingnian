#include "Dx9/_ActorCommon.fxh"


#define SHADER_VERSION PS_2_0


#define T(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap) \
	TECHNIQUE_ACTOR(0,0,SHADOW_TYPE_NONE, skinned,_OFF,_ON,normalmap,selfillum,dir0,dir1,sh9,sh4,speculardir,_ON,_OFF,_OFF,_OFF,psworldnmap,cubemap,0,_OFF,_OFF) \
	TECHNIQUE_ACTOR(0,0,SHADOW_TYPE_DEPTH,skinned,_OFF,_ON,normalmap,selfillum,dir0,dir1,sh9,sh4,speculardir,_ON,_OFF,_OFF,_OFF,psworldnmap,cubemap,0,_OFF,_OFF) \
	TECHNIQUE_ACTOR(0,0,SHADOW_TYPE_COLOR,skinned,_OFF,_ON,normalmap,selfillum,dir0,dir1,sh9,sh4,speculardir,_ON,_OFF,_OFF,_OFF,psworldnmap,cubemap,0,_OFF,_OFF) \

#include "Dx9/_ActorCommonTechnique.fxh"




//	skin	nrm		ilum	dir0	dir1	sh9		sh4		difpts	spcpts	spcdr	pswrl	cube

T(	0,		0,		0,		0,		0,		0,		_ON_PS,	0,		0,		0,		0,		0	 )
T(	_ON,	0,		0,		0,		0,		0,		_ON_PS,	0,		0,		0,		0,		0	 )
T(	0,		0,		_ON,	0,		0,		0,		_ON_PS,	0,		0,		0,		0,		0	 )
T(	_ON,	0,		_ON,	0,		0,		0,		_ON_PS,	0,		0,		0,		0,		0	 )
