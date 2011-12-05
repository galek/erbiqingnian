//----------------------------------------------------------------------------
// dx9_caps_def.h
//
// - Header for capabilities definitions/specifications
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

// NOTE: These caps are a generalized superset of platform capabilities.
// The idea is that platforms detect and fill these in as is most appropriate to
// that platform's needs.


#ifdef INCLUDE_CAPS_ENUM
#define DEFINE_CAP(name,fourcc,desc,wantsmaller) name,
#else
// { code name, string name, smaller value is better }
#define DEFINE_CAP(name,fourcc,desc,wantsmaller) { MAKEFOURCC(fourcc[0],fourcc[1],fourcc[2],fourcc[3]), desc, wantsmaller },
#endif

DEFINE_CAP( DX9CAP_GPU_VENDOR_ID,						"VNDR",	"GPU vendor ID",							FALSE )
DEFINE_CAP( DX9CAP_GPU_MULTIPLE_CARDS_MODE,				"MGPU",	"Multiple GPUs (SLI, Crossfire)",			FALSE )
DEFINE_CAP( DX9CAP_MAX_ZBUFFER_DEPTH_BITS,				"ZDEP",	"Max z-buffer bit-depth",					FALSE )
DEFINE_CAP( DX9CAP_3D_HARDWARE_ACCELERATION,			"3DHW",	"Hardware acceleration",					FALSE )
DEFINE_CAP( DX9CAP_HW_SHADOWS,							"HWSH",	"NVIDIA-style hardware shadows",			FALSE )
DEFINE_CAP( DX9CAP_NULL_RENDER_TARGET,					"NLRT",	"Null Render Target",						FALSE )
DEFINE_CAP( DX9CAP_DEPTHBIAS,							"DPTH", "Depth bias",								FALSE )
DEFINE_CAP( DX9CAP_SLOPESCALEDEPTHBIAS,					"SSDP", "Slope-scale depth bias",					FALSE )
DEFINE_CAP( DX9CAP_MAX_VS_VERSION,						"VSVR",	"Max vertex shader version",				FALSE )
DEFINE_CAP( DX9CAP_MAX_PS_VERSION,						"PSVR",	"Max pixel shader version",					FALSE )
DEFINE_CAP( DX9CAP_MAX_VS_CONSTANTS,					"VSCN",	"Max vertex shader constants",				FALSE )
//DEFINE_CAP( DX9CAP_MAX_PS_CONSTANTS,					"PSCN",	"Max pixel shader constants",				FALSE )
DEFINE_CAP( DX9CAP_MAX_VS_STATIC_BRANCH,				"VSSB",	"Max vertex shader static branch depth",	FALSE )
DEFINE_CAP( DX9CAP_MAX_PS_STATIC_BRANCH,				"PSSB",	"Max pixel shader static branch depth",		FALSE )
DEFINE_CAP( DX9CAP_MAX_VS_DYNAMIC_BRANCH,				"VSDB",	"Max vertex shader dynamic branch depth",	FALSE )
DEFINE_CAP( DX9CAP_MAX_PS_DYNAMIC_BRANCH,				"PSDB",	"Max pixel shader dynamic branch depth",	FALSE )
DEFINE_CAP( DX9CAP_MAX_PS20_INSTRUCTIONS,				"PS2I",	"Max pixel shader 2.0 instruction count",	FALSE )
DEFINE_CAP( DX9CAP_MAX_TEXTURE_WIDTH,					"TXWD",	"Max texture width",						FALSE )
DEFINE_CAP( DX9CAP_MAX_TEXTURE_HEIGHT,					"TXHT",	"Max texture height",						FALSE )
DEFINE_CAP( DX9CAP_MAX_SIMUL_RENDER_TARGETS,			"MRTS",	"Max simultaneous render targets",			FALSE )
DEFINE_CAP( DX9CAP_LINEAR_GAMMA_SHADER_IN,				"LGSI",	"Shader input sRGB->linear gamma",			FALSE )
DEFINE_CAP( DX9CAP_LINEAR_GAMMA_SHADER_OUT,				"LGSO",	"Shader output linear->sRGB gamma",			FALSE )
DEFINE_CAP( DX9CAP_FLOAT_HDR,							"FHDR",	"Floating point HDR rendering",				FALSE )
DEFINE_CAP( DX9CAP_INTEGER_HDR,							"IHDR",	"Integer HDR rendering",					FALSE )
DEFINE_CAP( DX9CAP_HARDWARE_GAMMA_RAMP,					"HGMA",	"Hardware gamma ramp",						FALSE )
DEFINE_CAP( DX9CAP_OCCLUSION_QUERIES,					"OCCL",	"Occlusion queries",						FALSE )
DEFINE_CAP( DX9CAP_MINIMUM_TEXTURE_FORMATS,				"MTXF",	"Baseline texture formats",					FALSE )
DEFINE_CAP( DX9CAP_MINIMUM_RENDERTARGET_FORMATS,		"MRTF",	"Baseline rendertarget formats",			FALSE )
DEFINE_CAP( DX9CAP_MINIMUM_DEPTH_FORMATS,				"MDPF",	"Baseline depth buffer formats",			FALSE )
DEFINE_CAP( DX9CAP_SCISSOR_TEST,						"SCSR",	"Scissor test",								FALSE )
DEFINE_CAP( DX9CAP_MINIMUM_GENERAL_CAPS,				"GNRL",	"Baseline general platform caps",			FALSE )
DEFINE_CAP( DX9CAP_HARDWARE_MOUSE_CURSOR,				"HWMS",	"Hardware mouse cursor",					FALSE )
DEFINE_CAP( DX9CAP_MAX_USER_CLIP_PLANES,				"CPLN",	"Maximum user clip planes",					FALSE )
DEFINE_CAP( DX9CAP_ANISOTROPIC_FILTERING,				"ANIS",	"Anisotropic texture filtering",			FALSE )
DEFINE_CAP( DX9CAP_TRILINEAR_FILTERING,					"TLNR",	"Anisotropic texture filtering",			FALSE )
DEFINE_CAP( DX9CAP_SMALLEST_COLOR_TARGET,				"SMRT",	"Smallest color target format",				FALSE )
DEFINE_CAP( DX9CAP_VS_POWER,							"VSPW", "Vertex shader power",						FALSE )
DEFINE_CAP( DX9CAP_PS_POWER,							"PSPW", "Pixel shader power",						FALSE )
DEFINE_CAP( DX9CAP_COVERAGE_SAMPLING,					"CSAA",	"Coverage sample antialiasing",				FALSE )
DEFINE_CAP( DX9CAP_SEPARATE_ALPHA_BLEND_ENABLE,			"SABE",	"Separate alpha blend enable",				FALSE )

DEFINE_CAP( DX9CAP_MAX_POINT_SIZE,						"MXPT", "Maximum point size",						FALSE )
DEFINE_CAP( DX9CAP_MAX_ACTIVE_LIGHTS,					"MACL", "Maximum number of active lights",			FALSE )	
DEFINE_CAP( DX9CAP_MAX_VERTEX_STREAMS,					"MXAS", "Maximum number of vertex streams",			FALSE )	
DEFINE_CAP( DX9CAP_MAX_ACTIVE_RENDERTARGETS,			"MART", "Maximum number of active rendertargets",	FALSE )	
DEFINE_CAP( DX9CAP_MAX_ACTIVE_TEXTURES,					"MATX", "Maximum number of active textures",		FALSE )

DEFINE_CAP( DX9CAP_PIXEL_SHADER_PROFILE,				"PSPF", "Pixel shader profile",						FALSE )		// CHB 2007.07.25

#undef DEFINE_CAP