#ifdef FXC10_PATH
#include "../../../source/DxC/dxC_shaderlimits.h"
#else
#include "../../../source/DxC/dxC_shaderlimits.h"
#endif

float4 Bones[_MAX_BONES_PER_SHADER_20 * 3];	// 3 registers per bone

#ifdef ENGINE_TARGET_DX10
	float4 BonesPrev[_MAX_BONES_PER_SHADER_20 * 3];	// 3 registers per bone
	#define ENGINE_BONES_PREV_DECLARED 1
#endif

#ifdef FXC10_PATH
#include "dx9/_AnimatedShared.fx"
#include "dx9/_AnimatedBonesFunctions.fxh"
#else
#include "_AnimatedShared.fx"
#include "_AnimatedBonesFunctions.fxh"
#endif