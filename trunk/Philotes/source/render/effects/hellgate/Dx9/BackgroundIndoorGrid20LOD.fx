#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_2_0


#include "../hellgate/Dx9/_BackgroundRigid32.fxh"

#define T(lightmap, selfillum) \
	TECHNIQUE32(0,SHADOW_TYPE_NONE, _OFF,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \
	TECHNIQUE32(0,SHADOW_TYPE_DEPTH,_OFF,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \
	TECHNIQUE32(0,SHADOW_TYPE_COLOR,_OFF,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \



//	litm	ilum	

//T(	0,		0		)
//T(	0,		1		)
T(	1,		0		)
T(	1,		1		)



#include "../hellgate/Dx9/_BackgroundRigid64.fxh"

#define T(lightmap, selfillum) \
	TECHNIQUE64(0,SHADOW_TYPE_NONE, _OFF,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \
	TECHNIQUE64(0,SHADOW_TYPE_DEPTH,_OFF,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \
	TECHNIQUE64(0,SHADOW_TYPE_COLOR,_OFF,_ON,_OFF,_OFF,lightmap,selfillum,_OFF,_OFF,_OFF,_OFF,_ON_PS,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \


//	litm	ilum	

//T(	0,		0		)
//T(	0,		1		)
T(	1,		0		)
T(	1,		1		)
