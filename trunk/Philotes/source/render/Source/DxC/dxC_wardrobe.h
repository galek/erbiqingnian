//----------------------------------------------------------------------------
// dxC_wardrobe.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_WARDROBE__
#define __DXC_WARDROBE__

#include "e_granny_rigid.h"
#include "e_wardrobe.h"

#include "dxC_model.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// a collection of parts which can be added to a wardrobe
struct WARDROBE_MODEL_ENGINE_DATA
{
	struct D3D_VERTEX_BUFFER_DEFINITION tVertexBufferDef;
	INDEX_BUFFER_TYPE* pIndexBuffer;

	int nTriangleCount;
	int nIndexBufferSize;
	BOOL* pbVertexUsedArray;			// used by wardrobe system as a scratch pad for what vertices are used
	BOOL* pbTriangleUsedArray;			// used by wardrobe system as a scratch pad for what triangles are used
	INDEX_BUFFER_TYPE* pIndexMapping; // used by wardrobe system as a scratch pad for mapping old indices to new

	int nBoneCount;

	int nPartCount;
	WARDROBE_MODEL_PART* pParts;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_WardrobeLayerTexturePreSaveBlendTexture(
	TEXTURE_DEFINITION * pTextureDefinition,
	void* pTextureFull,
	D3DXIMAGE_INFO * pImageInfoFull,
	DWORD dwFlags );

// DXT1 to 24-bit RGB
void dxC_ConvertDXT1ToRGB(
	BYTE * pbDestRGB[ 4 ], // each pointer is a row in the dest texture/block
	const BYTE * pbSourceDXT1,
	BOOL bForceNoAlpha );

// DXT1 to 32-bit ARGB
void dxC_ConvertDXT1ToRGBA(
	BYTE * pbDestRGBA[ 4 ], // each pointer is a row in the dest texture/block
	const BYTE * pbSourceDXT1,
	BOOL bForceNoAlpha );

// DXT3 to 32-bit ARGB
void dxC_ConvertDXT3ToRGBA(
	BYTE * pbDestRGBA[ 4 ], // each pointer is a row in the dest texture/block
	const BYTE * pbSourceDXT3 );

// DXT5 to 32-bit ARGB
void dxC_ConvertDXT5ToRGBA(
	BYTE * pbDestRGBA[ 4 ], // each pointer is a row in the dest texture/block
	const BYTE * pbSourceDXT5 );

PRESULT dxC_WardrobeDeviceLost();
PRESULT dxC_WardrobeDeviceReset();


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


#endif // __DXC_WARDROBE__