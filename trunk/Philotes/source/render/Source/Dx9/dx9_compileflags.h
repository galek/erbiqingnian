//----------------------------------------------------------------------------
// dx9_compileflags.h
//
// - Header for compile-time flags and #defines
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_COMPILEFLAGS__
#define __DX9_COMPILEFLAGS__



// --------------------------------------------
// VERBOSE D3D DEBUG OUTPUT						uncomment to get verbose D3D debug output
// -------------------------------------------- 
#if ISVERSION(DEVELOPMENT)
#	define D3D_DEBUG_INFO
#endif


// --------------------------------------------
// EXTRA TEXTURE VALIDATION						uncomment to get extra texture set validation
// -------------------------------------------- 
//#define EXTRA_SET_TEXTURE_VALIDATION


// --------------------------------------------
// NVIDIA PERFKIT								uncomment to enable nVidia PerfKit measures
// -------------------------------------------- 
//#define NVIDIA_PERFKIT_ENABLE


// --------------------------------------------
// DXDUMP										uncomment to allow a dump of any/all D3D information using dxdump
// -------------------------------------------- 
//#define DXDUMP_ENABLE


// --------------------------------------------
// D3D SHADER DEBUGGING							uncomment to enable D3D Shader debugging
// -------------------------------------------- 
//#define _D3DSHADER_DEBUG


// --------------------------------------------
// NVDXT LIBRARY								uncomment to use nVidia DXT conversion library (higher quality MIPs)
// -------------------------------------------- 
#if ISVERSION(DEVELOPMENT)
	#define DXTLIB
#endif

// --------------------------------------------
// BC5 TEXTURE SUPPORT							uncomment to generate BC5 normal textures for use in DX10
// -------------------------------------------- 
#define DX10_BC5_TEXTURES

// --------------------------------------------
// PIX PROFILE									uncomment to enable pix profile markers and region labels
// -------------------------------------------- 
#if ISVERSION(DEVELOPMENT)
//#	define PIX_PROFILE
#endif


#endif // __DX9_COMPILEFLAGS__