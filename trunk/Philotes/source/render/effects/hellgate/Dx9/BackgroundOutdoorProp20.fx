#include "Dx9/_BackgroundCommon.fxh"


#define SHADER_VERSION PS_2_0



#define T(diffuse2,selfillum,dir0,dir1,sh9,sh4,specular,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_NONE ,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_NONE ,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_NONE ,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_DEPTH,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_DEPTH,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_DEPTH,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE32(0,SHADOW_TYPE_COLOR,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE32(3,SHADOW_TYPE_COLOR,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE32(5,SHADOW_TYPE_COLOR,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll)



#include "../hellgate/Dx9/_BackgroundRigid32.fxh"



//	diff2	ilum	dir0	dir1	sh9		sh4		spec	scrl

T(	0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0		)
T(	1,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0		)

T(	0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0		)
T(	1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0		)

T(	0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1		)
T(	1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1		)

T(	0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0		)
T(	1,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0		)

T(	0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0		)
T(	1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0		)

//T(	0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1		)
//T(	1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1		)



#define T(diffuse2,selfillum,dir0,dir1,sh9,sh4,specular,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_NONE ,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_NONE ,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_NONE ,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_DEPTH,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_DEPTH,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_DEPTH,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE64(0,SHADOW_TYPE_COLOR,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE64(3,SHADOW_TYPE_COLOR,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll) \
	TECHNIQUE64(5,SHADOW_TYPE_COLOR,_OFF,_OFF,diffuse2,_OFF,_OFF,selfillum,dir0,dir1,sh9,sh4,_OFF,specular,_OFF,_OFF,_OFF,_OFF,_OFF,scroll)



#include "../hellgate/Dx9/_BackgroundRigid64.fxh"



//	diff2	ilum	dir0	dir1	sh9		sh4		spec	scrl

T(	0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0		)
T(	1,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0		)

T(	0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0		)
T(	1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		0		)

T(	0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1		)
T(	1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		0,		1		)

T(	0,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0		)
T(	1,		0,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0		)

T(	0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0		)
T(	1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		0		)

//T(	0,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1		)
//T(	1,		1,		_ON_VS,	_ON_VS,	_ON_VS,	0,		1,		1		)

