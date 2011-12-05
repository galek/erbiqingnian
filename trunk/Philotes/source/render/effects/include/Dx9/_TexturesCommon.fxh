//
// _TexturesCommon - header file for texture structs/registers and common samplers
//

//--------------------------------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX10
#	define TEXTURE2D		shared Texture2D
#	define TEXTURE2D_LOCAL	Texture2D
#	define TEXTURECUBE		shared TextureCube
#	define TEXSAMPLER		shared sampler
#	define TEXSHARED			
#else
#	define TEXTURE2D		texture
#	define TEXTURE2D_LOCAL	texture
#	define TEXTURECUBE		texture
#	define TEXSAMPLER		sampler
#	define TEXSHARED		
#endif 

#ifdef ENGINE_TARGET_DX10
#define DECLARE_TEXTURE_STAGE_SAMPLER( stage, dx10filter, dx9MinFilter, dx9MagFilter, AddressModeU, AddressModeV ) \
	sampler Sampler##stage : register(s##stage) = sampler_state	\
	{										\
		Texture = TextureStage##stage;		\
		Filter = dx10filter;				\
		AddressU = AddressModeU;			\
		AddressV = AddressModeV;			\
	};										
#else
#define DECLARE_TEXTURE_STAGE_SAMPLER( stage, dx10filter, dx9MinFilter, dx9MagFilter, AddressModeU, AddressModeV ) \
	sampler Sampler##stage : register(s##stage) = sampler_state	\
	{										\
		MinFilter = dx9MinFilter;			\
		MagFilter = dx9MagFilter;			\
		AddressU = AddressModeU;			\
		AddressV = AddressModeV;			\
	};	
#endif



#include "../../source/dxC/dxC_texturesamplerconstants.h"	// CHB 2007.02.16



//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

#define SAMPLER(reg)	s##reg
#define TEXCOORD_(x)	TEXCOORD##x
