#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_2_0
#define SELF_SHADOW 1

#include "../hellgate/Dx9/_BackgroundRigid32.fxh"


#define T(selfillum,dir0,dir1,sh9,sh4) \
	TECHNIQUE32(0,SHADOW_TYPE_NONE ,_OFF,_ON,_OFF,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \
	TECHNIQUE32(0,SHADOW_TYPE_DEPTH,_OFF,_ON,_OFF,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \
	TECHNIQUE32(0,SHADOW_TYPE_COLOR,_OFF,_ON,_OFF,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \



//	ilum	dir0	dir1	sh9		sh4		

T(	0,		0,		0,		0,		0		)
T(	0,		0,		0,		0,		_ON_PS	)

T(	1,		0,		0,		0,		0		)
T(	1,		0,		0,		0,		_ON_PS	)



#include "../hellgate/Dx9/_BackgroundRigid64.fxh"


#define T(selfillum,dir0,dir1,sh9,sh4) \
	TECHNIQUE64(0,SHADOW_TYPE_NONE ,_OFF,_ON,_OFF,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \
	TECHNIQUE64(0,SHADOW_TYPE_DEPTH,_OFF,_ON,_OFF,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \
	TECHNIQUE64(0,SHADOW_TYPE_COLOR,_OFF,_ON,_OFF,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF,_OFF) \


//	ilum	dir0	dir1	sh9		sh4		

T(	0,		0,		0,		0,		0		)
T(	0,		0,		0,		0,		_ON_PS	)

T(	1,		0,		0,		0,		0		)
T(	1,		0,		0,		0,		_ON_PS	)
