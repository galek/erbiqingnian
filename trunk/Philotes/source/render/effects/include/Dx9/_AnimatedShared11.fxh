#include "../../../source/DxC/dxC_shaderlimits.h"

float4 Bones[_MAX_BONES_PER_SHADER_11 * 3];	// 3 registers per bone

#include "Dx9/_AnimatedShared.fx"
#include "Dx9/_AnimatedBonesFunctions.fxh"
