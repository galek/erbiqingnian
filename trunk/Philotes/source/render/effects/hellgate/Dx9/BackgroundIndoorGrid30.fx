#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_3_0


#define _TECH_SHDW(shadowtype,diffuse2,normalmap,lightmap,selfillum,specular,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(0,shadowtype,_OFF,_ON,diffuse2,normalmap,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,specular,_ON,_OFF,_OFF,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(3,shadowtype,_OFF,_ON,diffuse2,normalmap,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,specular,_ON,_OFF,_OFF,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(5,shadowtype,_OFF,_ON,diffuse2,normalmap,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,specular,_ON,_OFF,_OFF,psworldnmap,cubemap,scroll)


#ifdef DX10_NO_SHADOW_TYPE_COLOR
#	define T(diffuse2,normalmap,lightmap,selfillum,specular,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_NONE, diffuse2,normalmap,lightmap,selfillum,specular,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_DEPTH,diffuse2,normalmap,lightmap,selfillum,specular,psworldnmap,cubemap,scroll)
#else
#	define T(diffuse2,normalmap,lightmap,selfillum,specular,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_NONE, diffuse2,normalmap,lightmap,selfillum,specular,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_DEPTH,diffuse2,normalmap,lightmap,selfillum,specular,psworldnmap,cubemap,scroll) \
		_TECH_SHDW(SHADOW_TYPE_COLOR,diffuse2,normalmap,lightmap,selfillum,specular,psworldnmap,cubemap,scroll)
#endif


#include "../hellgate/Dx9/_BackgroundRigid64.fxh"


//	diff2	nrm		litm	ilum	spec	pswrl	cube	scrl

T(	0,		0,		1,		0,		0,		0,		0,		0	 )
T(	1,		0,		1,		0,		0,		0,		0,		0	 )
T(	0,		0,		1,		0,		0,		0,		1,		0	 )

T(	0,		0,		1,		0,		1,		0,		0,		0	 )
T(	1,		0,		1,		0,		1,		0,		0,		0	 )
T(	0,		0,		1,		0,		1,		0,		1,		0	 )


T(	0,		1,		1,		0,		1,		0,		0,		0	 )
T(	1,		1,		1,		0,		1,		0,		0,		0	 )
T(	0,		1,		1,		0,		1,		0,		1,		0	 )


T(	0,		0,		1,		1,		0,		0,		0,		0	 )
T(	1,		0,		1,		1,		0,		0,		0,		0	 )
T(	0,		0,		1,		1,		0,		0,		1,		0	 )

T(	0,		0,		1,		1,		1,		0,		0,		0	 )
T(	1,		0,		1,		1,		1,		0,		0,		0	 )
T(	0,		0,		1,		1,		1,		0,		1,		0	 )


T(	0,		1,		1,		1,		1,		0,		0,		0	 )
T(	1,		1,		1,		1,		1,		0,		0,		0	 )
T(	0,		1,		1,		1,		1,		0,		1,		0	 )


T(	0,		0,		1,		1,		0,		0,		0,		1	 )
T(	1,		0,		1,		1,		0,		0,		0,		1	 )
T(	0,		0,		1,		1,		0,		0,		1,		1	 )

T(	0,		0,		1,		1,		1,		0,		0,		1	 )
T(	1,		0,		1,		1,		1,		0,		0,		1	 )
T(	0,		0,		1,		1,		1,		0,		1,		1	 )


T(	0,		1,		1,		1,		1,		0,		0,		1	 )
T(	1,		1,		1,		1,		1,		0,		0,		1	 )
T(	0,		1,		1,		1,		1,		0,		1,		1	 )
