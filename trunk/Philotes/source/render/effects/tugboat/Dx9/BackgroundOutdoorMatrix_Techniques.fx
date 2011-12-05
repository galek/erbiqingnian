//
// 

#include "_common.fx"

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
// 2.0 Techniques
//--------------------------------------------------------------------------------------------
#define TECHNIQUE20_NAME(name,points,specular,normal,diffusemap2,shadowtype,debugflag) \
	DECLARE_TECH name##points##specular##normal##diffusemap2##shadowtype##debugflag < \
				SHADER_VERSION_22\
				int Specular = specular; \
				int LightMap = 1; \
				int DiffuseMap2 = diffusemap2; \
				int ShadowType = shadowtype; > { \
	pass P0	{ \
		COMPILE_SET_VERTEX_SHADER( VS_2_V, VS20_BACKGROUND( diffusemap2, shadowtype ) ); \
		COMPILE_SET_PIXEL_SHADER( PS_2_V, PS20_BACKGROUND( specular, diffusemap2, shadowtype ) ); \
		} \
	} 

#define TECHNIQUE20(points,specular,normal,diffusemap2,shadowtype,debugflag) TECHNIQUE20_NAME(tech_,points,specular,normal,diffusemap2,shadowtype,debugflag)

#ifndef TECHNIQUE20_SHADOW0
#define TECHNIQUE20_SHADOW0(points,specular,normal,diffusemap2,debugflag) TECHNIQUE20(points,specular,normal,diffusemap2,SHADOW_TYPE_NONE,debugflag)
#endif

#ifndef TECHNIQUE20_SHADOW1
#define TECHNIQUE20_SHADOW1(points,specular,normal,diffusemap2,debugflag) TECHNIQUE20(points,specular,normal,diffusemap2,SHADOW_TYPE_DEPTH,debugflag)
#endif

#ifdef PRIME
#include "dx9/_BackgroundHeader.fx"
#endif
