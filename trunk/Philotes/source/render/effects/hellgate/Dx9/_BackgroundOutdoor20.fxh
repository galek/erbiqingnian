#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_2_0


#include "../hellgate/Dx9/_BackgroundRigid32.fxh"

#ifndef _SPOTLIGHT
#define _SPOTLIGHT _OFF
#endif

#ifndef TECH
#define TECH TECH_SL
#endif

#define TECH_NULL(lightmap,dir0,dir1,sh9,sh4,specular,cubemap,scroll)

#define TECH_SL(lightmap,dir0,dir1,sh9,sh4,specular,cubemap,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_NONE, _SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_NONE, _SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_NONE, _SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_COLOR,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_COLOR,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_COLOR,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll)
	


//			litm	dir0	dir1	sh9		sh4		spec	cube	scrl

TECH_SL	(	1,		_ON_VS,	_ON_VS,	0,		0,		0,		0,		0	 )
TECH	(	1,		_ON_VS,	_ON_VS,	0,		0,		0,		1,		0	 )
TECH	(	1,		_ON_VS,	_ON_VS,	0,		0,		1,		0,		0	 )
TECH_SL	(	1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0	 )
TECH	(	1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1,		0	 )
TECH	(	1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )



#include "../hellgate/Dx9/_BackgroundRigid64.fxh"



#define TECH_SL(lightmap,dir0,dir1,sh9,sh4,specular,cubemap,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_NONE, _SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_NONE, _SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_NONE, _SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_COLOR,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_COLOR,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_COLOR,_SPOTLIGHT,_OFF,_OFF,_OFF,lightmap,_OFF,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,cubemap,scroll)


//			litm	dir0	dir1	sh9		sh4		spec	cube	scrl

TECH_SL	(	1,		_ON_VS,	_ON_VS,	0,		0,		0,		0,		0	 )
TECH	(	1,		_ON_VS,	_ON_VS,	0,		0,		0,		1,		0	 )
TECH	(	1,		_ON_VS,	_ON_VS,	0,		0,		1,		0,		0	 )
TECH_SL	(	1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0	 )
TECH	(	1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1,		0	 )
TECH	(	1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
