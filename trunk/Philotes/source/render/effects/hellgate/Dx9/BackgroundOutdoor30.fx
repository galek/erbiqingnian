#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_3_0


#ifndef TECH
#define TECH _TECH_STD
#endif

#ifndef TECH_SL
#define TECH_SL _TECH_STD
#endif

#define _TECH_NULL(diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll)


#define _TECH_SPOT(diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_NONE,_ON,_OFF,diffuse2,normalmap, lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,speculardir,_OFF,_OFF,_OFF,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_NONE,_ON,_OFF,diffuse2,normalmap, lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,speculardir,_OFF,_OFF,_OFF,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_NONE,_ON,_OFF,diffuse2,normalmap, lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,speculardir,_OFF,_OFF,_OFF,psworldnmap,cubemap,scroll)


#define _TECH_STD_SHDW(shadowtype,diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(0,shadowtype,_OFF,_OFF,diffuse2,normalmap,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,speculardir,_ON,_OFF,_OFF,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(3,shadowtype,_OFF,_OFF,diffuse2,normalmap,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,speculardir,_ON,_OFF,_OFF,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(5,shadowtype,_OFF,_OFF,diffuse2,normalmap,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,speculardir,_ON,_OFF,_OFF,psworldnmap,cubemap,scroll) \


#ifdef DX10_NO_SHADOW_TYPE_COLOR
#	define _TECH_STD(diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_NONE, diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_DEPTH,diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll)
#else
#	define _TECH_STD(diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_NONE, diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_DEPTH,diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_COLOR,diffuse2,normalmap,lightmap,dir0,dir1,sh9,sh4,speculardir,psworldnmap,cubemap,scroll)
#endif


#include "../hellgate/Dx9/_BackgroundRigid64.fxh"




//			diff2	nrm		litm	dir0	dir1	sh9		sh4		spcdr	pswrl	cube	scrl


TECH_SL	(	0,		0,		1,		_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0	 )
TECH	(	1,		0,		1,		_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0	 )
TECH	(	0,		0,		1,		_ON_VS,	_ON_VS,	0,		0,		1,		0,		0,		0	 )
TECH	(	1,		0,		1,		_ON_VS,	_ON_VS,	0,		0,		1,		0,		0,		0	 )
TECH	(	0,		0,		1,		_ON_VS,	_ON_VS,	0,		0,		0,		0,		1,		0	 )
TECH	(	0,		0,		1,		_ON_VS,	_ON_VS,	0,		0,		1,		0,		1,		0	 )

TECH_SL	(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0	 )
TECH	(	1,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0	 )
TECH	(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0,		0	 )
TECH	(	1,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0,		0	 )
TECH	(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		1,		0	 )
TECH	(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		1,		0	 )

TECH_SL	(	0,		1,		1,		_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0	 )
TECH	(	1,		1,		1,		_ON_VS,	_ON_VS,	0,		0,		0,		0,		0,		0	 )
TECH	(	0,		1,		1,		_ON_VS,	_ON_VS,	0,		0,		1,		0,		0,		0	 )
TECH	(	1,		1,		1,		_ON_VS,	_ON_VS,	0,		0,		1,		0,		0,		0	 )
TECH	(	0,		1,		1,		_ON_VS,	_ON_VS,	0,		0,		0,		0,		1,		0	 )
TECH	(	0,		1,		1,		_ON_VS,	_ON_VS,	0,		0,		1,		0,		1,		0	 )

TECH_SL	(	0,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0	 )
TECH	(	1,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0,		0	 )
TECH	(	0,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0,		0	 )
TECH	(	1,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0,		0	 )
TECH	(	0,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		1,		0	 )
TECH	(	0,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		1,		0	 )
