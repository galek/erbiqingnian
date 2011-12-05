#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_2_0


#include "../hellgate/Dx9/_BackgroundRigid32.fxh"

#ifndef _SPOTLIGHT
#define _SPOTLIGHT _OFF
#endif

#ifndef TECH
#define TECH TECH_SL
#endif

#define TECH_NULL(lightmap,selfillum,specular,cubemap,scroll)

#define TECH_SL(lightmap,selfillum,specular,cubemap,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_NONE, _SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_NONE, _SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_NONE, _SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_COLOR,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_COLOR,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_COLOR,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll)
	


//			litm	ilum	spec	cube	scrl

TECH_SL	(	1,		0,		0,		0,		0	 )
TECH	(	1,		0,		0,		1,		0	 )
TECH	(	1,		0,		1,		0,		0	 )
TECH	(	1,		1,		0,		0,		0	 )
TECH	(	1,		1,		0,		1,		0	 )
TECH	(	1,		1,		1,		0,		0	 )
TECH	(	1,		1,		0,		0,		1	 )
TECH	(	1,		1,		0,		1,		1	 )







#include "../hellgate/Dx9/_BackgroundRigid64.fxh"

#define TECH_SL(lightmap,selfillum,specular,cubemap,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_NONE, _SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_NONE, _SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_NONE, _SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_DEPTH,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_COLOR,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_COLOR,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_COLOR,_SPOTLIGHT,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,specular,_OFF,cubemap,scroll)


//			litm	ilum	spec	cube	scrl

TECH_SL	(	1,		0,		0,		0,		0	 )
TECH	(	1,		0,		0,		1,		0	 )
TECH	(	1,		0,		1,		0,		0	 )
TECH	(	1,		1,		0,		0,		0	 )
TECH	(	1,		1,		0,		1,		0	 )
TECH	(	1,		1,		1,		0,		0	 )
TECH	(	1,		1,		0,		0,		1	 )
TECH	(	1,		1,		0,		1,		1	 )
