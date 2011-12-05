#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_3_0


#ifndef TECH
#define TECH _TECH_STD
#endif

#ifndef TECH_SL
#define TECH_SL _TECH_NULL
#endif

#define _TECH_NULL(diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll)


#define _TECH_SPOT(diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_NONE,_ON,_ON,diffuse2,normalmap, lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specularpt,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_NONE,_ON,_ON,diffuse2,normalmap, lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specularpt,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_NONE,_ON,_ON,diffuse2,normalmap, lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specularpt,psworldnmap,cubemap,scroll)


#define _TECH_STD_SHDW(shadowtype,diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll)	\
	TECHNIQUE64(0,shadowtype,_OFF,_ON,diffuse2,normalmap,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_ON,_OFF,specularpt,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(3,shadowtype,_OFF,_ON,diffuse2,normalmap,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_ON,_OFF,specularpt,psworldnmap,cubemap,scroll) \
	TECHNIQUE64(5,shadowtype,_OFF,_ON,diffuse2,normalmap,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_ON,_OFF,specularpt,psworldnmap,cubemap,scroll)
	

#ifdef DX10_NO_SHADOW_TYPE_COLOR
#define _TECH_STD(diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_NONE, diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_DEPTH,diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll)
#else
#define _TECH_STD(diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_NONE, diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_DEPTH,diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll) \
		_TECH_STD_SHDW(SHADOW_TYPE_COLOR,diffuse2,normalmap,lightmap,selfillum,specularpt,psworldnmap,cubemap,scroll)
#endif


#include "../hellgate/Dx9/_BackgroundRigid64.fxh"


//			diff2	nrm		litm	ilum	spcpt	pswrl	cube	scrll


//*

TECH_SL	(	0,		0,		1,		0,		0,		0,		0,		0	 )
TECH_SL	(	0,		1,		1,		0,		0,		0,		0,		0	 )
TECH_SL	(	1,		0,		1,		0,		0,		0,		0,		0	 )
TECH_SL	(	1,		1,		1,		0,		0,		0,		0,		0	 )
TECH_SL	(	0,		0,		1,		0,		0,		0,		0,		1	 )
TECH_SL	(	0,		1,		1,		0,		0,		0,		0,		1	 )
TECH_SL	(	1,		0,		1,		0,		0,		0,		0,		1	 )
TECH_SL	(	1,		1,		1,		0,		0,		0,		0,		1	 )

TECH	(	0,		0,		1,		0,		0,		0,		0,		0	 )
TECH	(	1,		0,		1,		0,		0,		0,		0,		0	 )
TECH	(	0,		0,		1,		0,		0,		0,		1,		0	 )

TECH	(	0,		0,		1,		0,		1,		0,		0,		0	 )
TECH	(	1,		0,		1,		0,		1,		0,		0,		0	 )
TECH	(	0,		0,		1,		0,		1,		0,		1,		0	 )


TECH	(	0,		1,		1,		0,		1,		0,		0,		0	 )
TECH	(	1,		1,		1,		0,		1,		0,		0,		0	 )
TECH	(	0,		1,		1,		0,		1,		0,		1,		0	 )


TECH	(	0,		0,		1,		1,		0,		0,		0,		0	 )
TECH	(	1,		0,		1,		1,		0,		0,		0,		0	 )
TECH	(	0,		0,		1,		1,		0,		0,		1,		0	 )

TECH	(	0,		0,		1,		1,		1,		0,		0,		0	 )
TECH	(	1,		0,		1,		1,		1,		0,		0,		0	 )
TECH	(	0,		0,		1,		1,		1,		0,		1,		0	 )


TECH	(	0,		1,		1,		1,		1,		0,		0,		0	 )
TECH	(	1,		1,		1,		1,		1,		0,		0,		0	 )
TECH	(	0,		1,		1,		1,		1,		0,		1,		0	 )


TECH	(	0,		0,		1,		1,		0,		0,		0,		1	 )
TECH	(	1,		0,		1,		1,		0,		0,		0,		1	 )
TECH	(	0,		0,		1,		1,		0,		0,		1,		1	 )

TECH	(	0,		0,		1,		1,		1,		0,		0,		1	 )
TECH	(	1,		0,		1,		1,		1,		0,		0,		1	 )
TECH	(	0,		0,		1,		1,		1,		0,		1,		1	 )


TECH	(	0,		1,		1,		1,		1,		0,		0,		1	 )
TECH	(	1,		1,		1,		1,		1,		0,		0,		1	 )
TECH	(	0,		1,		1,		1,		1,		0,		1,		1	 )

//*/