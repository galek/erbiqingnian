//----------------------------------------------------------------------------
// dxC_macros.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_MACROS__
#define __DXC_MACROS__

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------
#include <atlcomcli.h>

//For inserting small blocks of API specific code
#if defined(ENGINE_TARGET_DX9)
	#define DX10_BLOCK( ... ) 
	#define DX9_BLOCK( ... ) __VA_ARGS__
#elif defined(ENGINE_TARGET_DX10)
	#define DX10_BLOCK( ... ) __VA_ARGS__
	#define DX9_BLOCK( ... ) 
#endif

//Picks one label depending on target
#if defined(ENGINE_TARGET_DX10)
	#define DXC_9_10(dx9label, dx10label) dx10label
#elif defined(ENGINE_TARGET_DX9)
	#define DXC_9_10(dx9label, dx10label) dx9label
#endif

//Includes dx9 or dx10 header depending on target
#if defined(ENGINE_TARGET_DX10)
	#define DXCINC_9_10(dx9header, dx10header) <dx10header>
#elif defined(ENGINE_TARGET_DX9)
	#define DXCINC_9_10(dx9header, dx10header) <dx9header>
#endif


//Initializes structures if targeting DX9
#if defined(ENGINE_TARGET_DX10)
	#define DXC_9INIT1(a)
	#define DXC_9INIT2(a,b)
	#define DXC_9INIT3(a,b,c)
	#define DXC_9INIT4(a,b,c,d)
	#define DXC_9INIT5(a,b,c,d,e)
	#define DXC_9INIT6(a,b,c,d,e,f)
#elif defined(ENGINE_TARGET_DX9)
	#define DXC_9INIT1(a) a
	#define DXC_9INIT2(a,b) a, b
	#define DXC_9INIT3(a,b,c) a, b, c
	#define DXC_9INIT4(a,b,c,d) a, b, c, d
	#define DXC_9INIT5(a,b,c,d,e) a, b, c, d, e
	#define DXC_9INIT6(a,b,c,d,e,f) a, b, c, d, e, f
#endif

//Initializes structures if targeting DX10
#if defined(ENGINE_TARGET_DX10)
	#define DXC_10INIT1(a) a
	#define DXC_10INIT2(a,b) a, b
	#define DXC_10INIT3(a,b,c) a, b, c
	#define DXC_10INIT4(a,b,c,d) a, b, c, d
	#define DXC_10INIT5(a,b,c,d,e) a, b, c, d, e
	#define DXC_10INIT6(a,b,c,d,e,f) a, b, c, d, e, f
#elif defined(ENGINE_TARGET_DX9)
	#define DXC_10INIT1(a)
	#define DXC_10INIT2(a,b)
	#define DXC_10INIT3(a,b,c)
	#define DXC_10INIT4(a,b,c,d)
	#define DXC_10INIT5(a,b,c,d,e)
	#define DXC_10INIT6(a,b,c,d,e,f)
#endif

//DXC typedefs
#define DXC_TYPEDEF(dx9type, dx10type) typedef DXC_9_10(dx9type, dx10type)
#define DXC_TYPEDEF_LP(dx9type, dx10type) typedef struct DXC_9_10(dx9type, dx10type)*
#define DXC_TYPEDEF_SP(dx9type, dx10type) typedef CComPtr<DXC_9_10(dx9type, dx10type)>

//Map/Lock and Unmap/Unlock macros
#if defined(ENGINE_TARGET_DX9)
	#define dxC_MapEntireBuffer( pointer, dx9flags, dx10maptype, ppData )	pointer->Lock( 0, 0, ppData, dx9flags )
	#define dxC_pMappedRectData( rect )										rect.pBits
	#define dxC_nMappedRectPitch( rect )									(UINT)rect.Pitch
	#define dxC_Unmap(pointer)												pointer->Unlock()

#elif defined(ENGINE_TARGET_DX10)
	#define dxC_MapEntireBuffer( pointer, dx9flags, dx10maptype, ppData ) pointer->Map( dx10maptype, 0, ppData )
	#define dxC_pMappedRectData( rect ) rect.pData
	#define dxC_nMappedRectPitch( rect ) rect.RowPitch
	#define dxC_Unmap(pointer) S_OK; pointer->Unmap(); // DX10: Unmap doesn't return a value.

#endif

#define ARGB_DWORD_TO_VEC( dword ) (float)((dword >> 16) & 0x000000FF) / 255.0f, (float)((dword >> 8) & 0x000000FF) / 255.0f, (float)((dword) & 0x000000FF) / 255.0f, (float)((dword >> 24)  & 0x000000FF) / 255.0f
#define ARGB_DWORD_TO_RGBA_D3DXVECTOR4( dword ) D3DXVECTOR4( ARGB_DWORD_TO_VEC( dword ) )
#define ARGB_DWORD_TO_RGBA_FLOAT4( dword ) { ARGB_DWORD_TO_VEC( dword ) }

// CHB 2006.06.22 - Unification of vertex formats
#if defined(ENGINE_TARGET_DX9)
	#define D3DC_INPUT_VERTEX(Offset, Semantic, DX9Index, DX10Index, DX9Type, DX10Type) \
			{ 0,		Offset,	D3DDECLTYPE_##DX9Type,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_##Semantic,	DX9Index	},
	#define D3DC_INPUT_INSTANCE(Offset, Semantic, DX9Index, DX10Index, DX9Type, DX10Type, StreamIndex) \
			{ StreamIndex,	Offset,	D3DDECLTYPE_##DX9Type,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_##Semantic,	DX9Index	},
	//		{ Stream,		Offset,	Type,					Method,					Usage,						UsageIndex	}
	#define D3DC_INPUT_END() D3DDECL_END()
#elif defined(ENGINE_TARGET_DX10)
	#define D3DC_INPUT_VERTEX(Offset, Semantic, DX9Index, DX10Index, DX9Type, DX10Type) \
			{ #Semantic,	DX9Index,				DXGI_FORMAT_##DX10Type, 0,			Offset,				D3D10_INPUT_PER_VERTEX_DATA,	0						},
	#define D3DC_INPUT_INSTANCE(Offset, Semantic, DX9Index, DX10Index, DX9Type, DX10Type, StreamIndex) \
			{ #Semantic,	DX9Index,				DXGI_FORMAT_##DX10Type, 0,			Offset,				D3D10_INPUT_PER_INSTANCE_DATA,	0						},
	//		{ SemanticName,			SemanticIndex,	Format,					InputSlot,	AlignedByteOffset,	InputSlotClass,					InstanceDataStepRate	}
	#define D3DC_INPUT_END()
#else
	#error
#endif

#ifdef ENGINE_TARGET_DX9
	#define DX9VERIFY( statement) V( statement )
#elif defined(ENGINE_TARGET_DX10)
	#define DX9VERIFY( statement) statement
#endif

#ifdef ENGINE_TARGET_DX9
	#define EFFECT_INTERFACE_ISVALID( interface ) ( interface != NULL )
#elif defined( ENGINE_TARGET_DX10 )
	#define EFFECT_INTERFACE_ISVALID( interface ) ( ( interface != NULL ) && interface->IsValid() )
#endif


#endif //__DXC_MACROS__
