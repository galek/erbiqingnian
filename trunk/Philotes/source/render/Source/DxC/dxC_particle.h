//----------------------------------------------------------------------------
// dxC_particle.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_PARTICLE__
#define __DX9_PARTICLE__

#include "e_particle.h"

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
// this is used for drawing multiple meshes using the constant buffer
struct PARTICLE_MESH_VERTEX_MULTI
{
	BYTE		pPosition[ 3 ];			//  3 bytes			
	BYTE		bPad0;

	BYTE		fTextureCoords[2];		//  2 bytes
	BYTE		bMatrixIndex;			//  1 byte
	BYTE		bModelIndex;			//  1 byte

	D3DCOLOR	Normal;					// 4 bytes - one unused
	D3DCOLOR	Tangent;				// 4 bytes - one unused
};  // 16 bytes -- needs to be multiple of 32 bytes for best performance

// this is used for Havok FX, drawing multiple meshes using instancing
struct PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS
{
	VECTOR vPosition;      // 12 bytes
	VECTOR vNormal;		// 12 bytes
	BYTE fTextureCoords[2];		// 2 bytes
	BYTE bBoneIndex;			// 1 byte
	BYTE bUnused;				// 1 byte
};  // 28 bytes ... NVidia says that it is okay to not be cache aligned on 3.0 hardward

struct HAVOKFX_PARTICLE_VERTEX
{
	VECTOR		vPosition;
	BYTE		pTextureCoords[2];
	BYTE		bJunk[ 2 ];
}; // 16 bytes -- needs to be multiple of 32 bytes for best performance

struct GPU_PARTICLE_VERTEX
{
	D3DXVECTOR3    Position;
    D3DXVECTOR3    Velocity;
    unsigned char  Type;
};

struct PARTICLE_DRAW_STATS
{
	int				nDrawn;
	float			fWorldArea;
	float			fScreenAreaMeasured;
	float			fScreenAreaPredicted;
	D3D_OCCLUSION	tOcclusion;
	int				nTimesBegun;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern const D3DC_INPUT_ELEMENT_DESC g_ptParticleMeshVertexDeclarationArray_Multi[];
extern const D3DC_INPUT_ELEMENT_DESC g_ptParticleMeshVertexDeclarationArray_Multi_11[];
#ifdef HAVOKFX_ENABLED
extern const D3DC_INPUT_ELEMENT_DESC g_ptParticleMeshVertexDeclarationArray_HavokFX[];
extern const D3DC_INPUT_ELEMENT_DESC g_ptParticleVertexDeclarationArray_HavokFXParticle[];
#endif

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dx9_ParticleSystemsReleaseUnmanaged();
PRESULT dx9_ParticleSystemsRestoreUnmanaged();
PRESULT dx9_ParticleSystemsReleaseVBuffer();
PRESULT dx9_ParticleSystemsRestoreVBuffer();
void dxC_ParticlesSetScreenTexture( SPD3DCBASESHADERRESOURCEVIEW pTexture );
SPD3DCBASESHADERRESOURCEVIEW dxC_ParticlesGetScreenTexture();
EFFECT_TECHNIQUE * e_ParticleSystemGetTechnique(
	struct PARTICLE_SYSTEM_DEFINITION * pDefinition,
	struct D3D_EFFECT * pEffect );
struct D3D_EFFECT * e_ParticleSystemGetEffect( 
	const PARTICLE_SYSTEM_DEFINITION * pDefinition );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


#endif // __DX9_PARTICLE__