//
// At least they're in the same file now.
//

#ifndef __DX9_SHADERVERSIONCONSTANTS_H__
#define __DX9_SHADERVERSIONCONSTANTS_H__

#define DXC_ALL_STATE_FROM_ENGINE

#ifdef FX_COMPILE

#define VS_NONE		0
#define VS_1_1		1
#define VS_2_0		2
#define VS_2_a		3
#define VS_3_0		4
#define VS_4_0		5

#define PS_NONE		0
#define PS_1_1		1
#define PS_1_2		2
#define PS_1_3		3
#define PS_1_4		4
#define PS_2_0		5
#define PS_2_b		6
#define PS_2_a		7
#define PS_3_0		8
#define PS_4_0		9

// must match e_2d.h
#define SFX_OP_COPY				0
#define SFX_OP_SHADERS			1

#define SFX_RT_NONE				0
#define SFX_RT_BACKBUFFER		1
#define SFX_RT_FULL				2

#if defined(FXC_LEGACY_COMPILE)		// CHB 2007.08.30

#undef VS_2_0
#undef VS_2_a
#undef VS_3_0
#undef VS_4_0

#define VS_2_0	VS_1_1
#define VS_2_a	VS_1_1
#define VS_3_0	VS_1_1
#define VS_4_0	VS_1_1

#undef PS_1_2
#undef PS_1_3
#undef PS_1_4
#undef PS_2_0
#undef PS_2_b
#undef PS_2_a
#undef PS_3_0
#undef PS_4_0

#define PS_1_2	PS_1_1
#define PS_1_3	PS_1_1
#define PS_1_4	PS_1_1
#define PS_2_0	PS_1_1
#define PS_2_b	PS_1_1
#define PS_2_a	PS_1_1
#define PS_3_0	PS_1_1
#define PS_4_0	PS_1_1

#endif

#else

enum VERTEX_SHADER_VERSION
{
	VS_INVALID = -1,
	VS_NONE = 0,
	VS_1_1,
	VS_2_0,
	VS_2_a,
	VS_3_0,
	VS_4_0,
	// count
	NUM_VERTEX_SHADER_VERSIONS
};

// MANY data dependencies here!
enum PIXEL_SHADER_VERSION
{
	PS_INVALID = -1,
	PS_NONE = 0,
	PS_1_1,
	PS_1_2,
	PS_1_3,
	PS_1_4,
	PS_2_0,
	PS_2_b,
	PS_2_a,
	PS_3_0,
	PS_4_0,
	// count
	NUM_PIXEL_SHADER_VERSIONS
};

#endif

#endif
