//
// Screen effects table of contents
//

#include "_common.fx"
#include "StateBlocks.fxh"


// -------------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------------

// Must match e_2D.h enum CScreenEffects::LAYER
#define LAYER_ON_TOP		0			// default
#define LAYER_PRE_BLOOM		1

// -------------------------------------------------------------------------------
// MACROS
// -------------------------------------------------------------------------------

// all passes must define all three of these annotations, used or not, in this order
//   any custom annotations must follow

#define PASS_COPY( fromRT, toRT ) \
	pass COPY##__LINE__ < int nOp = SFX_OP_COPY; int nFromRT = fromRT; int nToRT = toRT; > {}
#define PASS_SHADERS( fromRT, toRT ) \
	pass SHADERS##__LINE__##fromRT##toRT < int nOp = SFX_OP_SHADERS; int nFromRT = fromRT; int nToRT = toRT; >

// utility to help set labels on elements in the dialog

#define SET_LABEL( element, label ) \
	string element = label;

#define SET_LAYER( layer ) \
	int nLayer = layer;

// -------------------------------------------------------------------------------
// STRUCTS
// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
// GLOBALS
// -------------------------------------------------------------------------------

half4 gfTransition;
half4 gvColor0;
half4 gvColor1;
half4 gfFloats;
TEXTURE2D_LOCAL Texture0;
TEXTURE2D_LOCAL Texture1;
TEXTURE2D_LOCAL TextureRT;

sampler RTSampler : register(SAMPLER(SAMPLER_DIFFUSE)) = sampler_state
{
	Texture = (TextureRT);
	AddressU = CLAMP;
	AddressV = CLAMP;

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_POINT;
#else
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = POINT;
#endif
};

// Individual effect files should set up their own samplers, but bind to registers other than Diffuse 0


//////////////////////////////////////////////////////////////////////////////////////////////
// Fullscreen effect files
//////////////////////////////////////////////////////////////////////////////////////////////


// CHB 2007.08.30 - Vignette too much for 1.1.
#if !defined(FXC_LEGACY_COMPILE)
#include "../common/dx9/SFX_Vignette.fxh"
#endif

#include "../common/dx9/SFX_Greyscale.fxh"
#include "../common/dx9/SFX_Chocolate.fxh"
#include "../common/dx9/SFX_SolidColor.fxh"
#include "../common/dx9/SFX_AddColor.fxh"

#ifdef ENGINE_TARGET_DX10
#define MOTION_BLUR_AS_PASS
#ifndef MOTION_BLUR_AS_PASS
	#include "../hellgate/Dx10/SFX_MotionBlur.fxh"
#endif	
#include "../hellgate/Dx10/SFX_DOF.fxh"
#endif
