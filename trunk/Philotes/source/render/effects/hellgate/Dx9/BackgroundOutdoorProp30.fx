#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_3_0



#define _TECH_SPOT(shadowtype,diffuse2,normalmap,selfillum,dir0,dir1,sh9,sh4,specular,cubemap,scroll) \
	TECHNIQUE64(0,shadowtype,_OFF,_OFF,diffuse2,normalmap,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_ON,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(3,shadowtype,_OFF,_OFF,diffuse2,normalmap,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_ON,_OFF,_OFF,_OFF,cubemap,scroll) \
	TECHNIQUE64(5,shadowtype,_OFF,_OFF,diffuse2,normalmap,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_ON,_OFF,_OFF,_OFF,cubemap,scroll)


#ifdef DX10_NO_SHADOW_TYPE_COLOR
#	define T(diffuse2,normalmap,selfillum,dir0,dir1,sh9,sh4,specular,cubemap,scroll) \
		_TECH_SPOT(SHADOW_TYPE_NONE, diffuse2,normalmap,selfillum,dir0,dir1,sh9,sh4,specular,cubemap,scroll) \
		_TECH_SPOT(SHADOW_TYPE_DEPTH,diffuse2,normalmap,selfillum,dir0,dir1,sh9,sh4,specular,cubemap,scroll)
#else
#	define T(diffuse2,normalmap,selfillum,dir0,dir1,sh9,sh4,specular,cubemap,scroll) \
		_TECH_SPOT(SHADOW_TYPE_NONE, diffuse2,normalmap,selfillum,dir0,dir1,sh9,sh4,specular,cubemap,scroll) \
		_TECH_SPOT(SHADOW_TYPE_DEPTH,diffuse2,normalmap,selfillum,dir0,dir1,sh9,sh4,specular,cubemap,scroll) \
		_TECH_SPOT(SHADOW_TYPE_COLOR,diffuse2,normalmap,selfillum,dir0,dir1,sh9,sh4,specular,cubemap,scroll)
#endif


#include "../hellgate/Dx9/_BackgroundRigid64.fxh"


//	diff2	nrm		ilum	dir0	dir1	sh9		sh4		spec	cube	scrl

T(	0,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0	 )
T(	1,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0	 )
T(	0,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1,		0	 )

T(	0,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
T(	1,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
T(	0,		0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1,		0	 )



T(	0,		1,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
T(	1,		1,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
T(	0,		1,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1,		0	 )



T(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0	 )
T(	1,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		0	 )
T(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1,		0	 )

T(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
T(	1,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
T(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1,		0	 )



T(	0,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
T(	1,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		0	 )
T(	0,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1,		0	 )



T(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		1	 )
T(	1,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0,		1	 )
T(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1,		1	 )

T(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		1	 )
T(	1,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		1	 )
T(	0,		0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1,		1	 )



T(	0,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		1	 )
T(	1,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0,		1	 )
T(	0,		1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1,		1	 )

