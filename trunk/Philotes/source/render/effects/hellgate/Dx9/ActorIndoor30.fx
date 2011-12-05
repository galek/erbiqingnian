#include "Dx9/_ActorCommon.fxh"


#define SHADER_VERSION PS_3_0



#define _TECH_SHDW(shadowtype,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,scatter,scroll) \
	TECHNIQUE_ACTOR(0,0,shadowtype,skinned,_OFF,_ON,normalmap,selfillum,dir0,dir1,sh9,sh4,speculardir,_ON,_OFF,_OFF		  ,_OFF		 ,psworldnmap,cubemap,0,scatter,scroll) \
	TECHNIQUE_ACTOR(2,2,shadowtype,skinned,_OFF,_ON,normalmap,selfillum,dir0,dir1,sh9,sh4,speculardir,_ON,_OFF,_OFF       ,diffusepts,psworldnmap,cubemap,0,scatter,scroll) \
	TECHNIQUE_ACTOR(2,2,shadowtype,skinned,_OFF,_ON,normalmap,selfillum,dir0,dir1,sh9,sh4,speculardir,_ON,_OFF,specularpts,diffusepts,psworldnmap,cubemap,0,scatter,scroll)


#ifdef DX10_NO_SHADOW_TYPE_COLOR
#	define T(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,scatter,scroll) \
		_TECH_SHDW(SHADOW_TYPE_NONE, skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,scatter,scroll) \
		_TECH_SHDW(SHADOW_TYPE_DEPTH,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,scatter,scroll)
#else
#	define T(skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,scatter,scroll) \
		_TECH_SHDW(SHADOW_TYPE_NONE, skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,scatter,scroll) \
		_TECH_SHDW(SHADOW_TYPE_DEPTH,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,scatter,scroll) \
		_TECH_SHDW(SHADOW_TYPE_COLOR,skinned,normalmap,selfillum,dir0,dir1,sh9,sh4,diffusepts,specularpts,speculardir,psworldnmap,cubemap,scatter,scroll)
#endif


#include "Dx9/_ActorCommonTechnique.fxh"




//	skin	nrm		ilum	dir0	dir1	sh9		sh4		difpts	spcpts	spcdr	pswrl	cube	sctr	scrl

//T(	_ON,	1,		0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )

//*

T(	0,		0,		0,		0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		0,		0	 )
T(	_ON,	0,		0,		0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		0,		0	 )
T(	0,		0,		0,		0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	0,		0	 )
T(	_ON,	0,		0,		0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	0,		0	 )
																									
T(	0,		_ON,	0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		0,		0	 )
T(	_ON,	_ON,	0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		0,		0	 )
T(	0,		_ON,	0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	0,		0	 )
T(	_ON,	_ON,	0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	0,		0	 )
																									
T(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		0,		0	 )
T(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		0,		0	 )
T(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	0,		0	 )
T(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	0,		0	 )
																									
T(	0,		_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		0,		0	 )
T(	_ON,	_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		0,		0	 )
T(	0,		_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	0,		0	 )
T(	_ON,	_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	0,		0	 )

/*
T(	0,		0,		0,		0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )
T(	_ON,	0,		0,		0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )
T(	0,		0,		0,		0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	_ON_PS,	0	 )
T(	_ON,	0,		0,		0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	_ON_PS,	0	 )
																									
T(	0,		_ON,	0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )
T(	_ON,	_ON,	0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )
T(	0,		_ON,	0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	_ON_PS,	0	 )
T(	_ON,	_ON,	0,		0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	_ON_PS,	0	 )
//*/
																									
T(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )
T(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )
T(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	_ON_PS,	0	 )
T(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	_ON_PS,	0	 )
																									
T(	0,		_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )
T(	_ON,	_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		_ON_PS,	0	 )
T(	0,		_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	_ON_PS,	0	 )
T(	_ON,	_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	_ON_PS,	0	 )

T(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		0,		_ON	 )
T(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		0,		0,		_ON	 )
T(	0,		0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	0,		_ON	 )
T(	_ON,	0,		_ON,	0,		0,		_ON_VS,	0,		_ON_VS,	_ON_PS,	0,		0,		_ON,	0,		_ON	 )
																									
T(	0,		_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		0,		_ON	 )
T(	_ON,	_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		0,		0,		_ON	 )
T(	0,		_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	0,		_ON	 )
T(	_ON,	_ON,	_ON,	0,		0,		_ON_VS,	0,		_ON_PS,	_ON_PS,	0,		0,		_ON,	0,		_ON	 )

//*/