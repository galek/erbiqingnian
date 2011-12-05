//----------------------------------------------------------------------------
// dx9_target_def.h
//
// - Header for render target definitions/specifications
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#ifndef DEFINE_TARGET
#	if defined(DEFINE_ONLY_BACKBUFFER_LINKED_TARGETS)
#		define DEFINE_TARGET(name, format, depth,width,height,scale,rsvd,altbb,syscopy,priority,bHg,bTug,bDx9,bMSAA,mintarget)	SWAP_RT_##name,
#	elif defined(DEFINE_ONLY_GLOBAL_TARGETS)
#		define DEFINE_TARGET(name, format, depth,width,height,scale,rsvd,altbb,syscopy,priority,bHg,bTug,bDx9,bMSAA,mintarget)	GLOBAL_RT_##name,
#	endif
#endif


//			   name					format					depth	width		height		scale			reserved	alt bb		syscopy		priority	hellgate	tugboat		dx9		MSAA	mintarget

#if ! defined(DEFINE_ONLY_GLOBAL_TARGETS)
DEFINE_TARGET( FULL,				D3DFMT_A8R8G8B8,		FALSE,	RTB_BACK,	RTB_BACK,	RTSF_1X,		TRUE,		FALSE,		FALSE,		100,		TRUE,		TRUE,		TRUE,	FALSE,	RTT_ANY			)
DEFINE_TARGET( FULL_MSAA,			D3DFMT_A8R8G8B8,		FALSE,	RTB_BACK,	RTB_BACK,	RTSF_1X,		TRUE,		TRUE,		FALSE,		100,		TRUE,		TRUE,		TRUE,	TRUE,	RTT_ANY			)
DEFINE_TARGET( BLUR_FILTERED,		D3DFMT_A8R8G8B8,		FALSE,	RTB_BACK,	RTB_BACK,	RTSF_DOWN_4X,	TRUE,		FALSE,		FALSE,		100,		TRUE,		TRUE,		TRUE,	FALSE,	FXTGT_SM_20_LOW	)	// CHB 2007.08.31 - not used on 1.1
DEFINE_TARGET( BLUR_TARGETX,		D3DFMT_A8R8G8B8,		FALSE,	RTB_BACK,	RTB_BACK,	RTSF_DOWN_4X,	TRUE,		FALSE,		FALSE,		100,		TRUE,		TRUE,		TRUE,	FALSE,	FXTGT_SM_20_LOW	)	// CHB 2007.08.31 - not used on 1.1
DEFINE_TARGET( BLUR_TARGETY,		D3DFMT_A8R8G8B8,		FALSE,	RTB_BACK,	RTB_BACK,	RTSF_DOWN_4X,	TRUE,		FALSE,		FALSE,		100,		TRUE,		TRUE,		TRUE,	FALSE,	RTT_ANY			)
DEFINE_TARGET( DEPTH,				D3DFMT_R32F,			FALSE,	RTB_BACK,	RTB_BACK,	RTSF_1X,		TRUE,		FALSE,		FALSE,		100,		TRUE,		FALSE,		FALSE,	TRUE,	RTT_ANY			) //depth for the whole scene
DEFINE_TARGET( MOTION,				D3DFMT_G16R16F,			FALSE,	RTB_BACK,	RTB_BACK,	RTSF_1X,		TRUE,		FALSE,		FALSE,		100,		TRUE,		FALSE,		FALSE,	TRUE,	RTT_ANY			)
DEFINE_TARGET( DEPTH_WO_PARTICLES,	D3DFMT_R32F,			FALSE,	RTB_BACK,	RTB_BACK,	RTSF_1X,		TRUE,		FALSE,		FALSE,		100,		TRUE,		FALSE,		FALSE,	FALSE,	RTT_ANY			) //depth for the whole scene w/o particles
DEFINE_TARGET( FLUID_BOXDEPTH,		D3DFMT_A32B32G32R32F,	FALSE,	RTB_BACK,	RTB_BACK,	RTSF_1X,		TRUE,		FALSE,		FALSE,		100,		TRUE,		FALSE,		FALSE,	FALSE,	RTT_ANY			)//for smoke rendering
#endif // ! DEFINE_ONLY_GLOBAL_TARGETS

#if ! defined(DEFINE_ONLY_BACKBUFFER_LINKED_TARGETS)
DEFINE_TARGET( AUTOMAP,				D3DFMT_A8R8G8B8,		FALSE,	RTB_MAP,	RTB_MAP,	RTSF_1X,		TRUE,		FALSE,		FALSE,		200,		TRUE,		TRUE,		TRUE,	FALSE,	RTT_ANY			)
DEFINE_TARGET( FOGOFWAR,			D3DFMT_A8R8G8B8,		FALSE,	RTB_MAP,	RTB_MAP,	RTSF_DOWN_2X,	TRUE,		FALSE,		FALSE,		200,		TRUE,		TRUE,		TRUE,	FALSE,	RTT_ANY			)
#endif // ! DEFINE_ONLY_BACKBUFFER_LINKED_TARGETS



#undef DEFINE_TARGET
#undef DEFINE_ONLY_BACKBUFFER_LINKED_TARGETS
#undef DEFINE_ONLY_GLOBAL_TARGETS