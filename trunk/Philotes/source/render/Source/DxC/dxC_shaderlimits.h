//----------------------------------------------------------------------------
// dxC_effect.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_SHADERLIMITS__
#define __DXC_SHADERLIMITS__

#ifndef DXC_SHADERLIMITS_COMPILE_11_AS_20
#define _MAX_BONES_PER_SHADER_11	24	// could be 28, but leaving some room for now
#else
#define _MAX_BONES_PER_SHADER_11	_MAX_BONES_PER_SHADER_20
#endif

#define _MAX_BONES_PER_SHADER_20	60

// Use 1.1 number of bones, then for higher-end machines,
// reconstitute to a higher number of bones.
#define MAX_BONES_PER_SHADER_ON_DISK	_MAX_BONES_PER_SHADER_11
#define MAX_BONES_PER_SHADER_RUN_TIME	_MAX_BONES_PER_SHADER_20

// CHB 2006.06.28 - Reduced from 40 to 20.
// CHB 2006.10.11 - Reduced from 20 to 17 to accomodate TVertexAndPixelShader_f in ParticleMesh.fx.
#define MAX_PARTICLE_MESHES_PER_BATCH	17

#endif
