//----------------------------------------------------------------------------
// e_granny_rigid.h
//
// - Header for Granny rigid model functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_GRANNY_RIGID__
#define __E_GRANNY_RIGID__


#include "granny.h"
#include "e_texture.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//#define FORCE_REMAKE_MODEL_BIN

const char sgszParticlePrefix[] = "particle_";
const int sgnParticleSystemPrefixLength = (int)PStrLen( sgszParticlePrefix );

const char sgszObjectPrefix[] = "object_";
const int sgnObjectPrefixLength = (int)PStrLen( sgszObjectPrefix );

const char sgszMonsterPrefix[] = "monster_";
const int sgnMonsterPrefixLength = (int)PStrLen( sgszMonsterPrefix );

#define MAX_FILE_NAME_LENGTH 256
#define MAX_ANIMATIONS_PER_MODEL 10
#define MAX_BONES_PER_MODEL 200
#define MIN_BOUNDING_BOX_SCALE 0.2f
#define TRIANGLES_PER_ORIGIN 2

#define CLOSE_POINT_DISTANCE		0.0001f

//-------------------------------------------------------------------------------------------------
// Art tool-specific exporter data 
//-------------------------------------------------------------------------------------------------
#define ART_TOOL_TILE_FLAG_U	0x00000001
#define ART_TOOL_TILE_FLAG_V	0x00000002
#define ART_TOOL_TILE_FLAG_W	0x00000004

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef WORD	INDEX_BUFFER_TYPE;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct BOUNDING_BOX;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Vertex Only definition - used for silhouettes 
//-------------------------------------------------------------------------------------------------

struct VERTEX_ONLY
{
	VECTOR vPosition;      // Vertex Position
};

static granny_data_type_definition g_ptGrannyPositionOnlyDataType[] =
{
	{GrannyReal32Member, GrannyVertexPositionName,				0, 3},
	{GrannyEndMember},
};

//-------------------------------------------------------------------------------------------------
// Simple vertex definition - used for loading things like position markers etc... 
//-------------------------------------------------------------------------------------------------

struct SIMPLE_VERTEX
{
	VECTOR vPosition;      // Vertex Position
	VECTOR vNormal;		// Vertex Normal
};

//-------------------------------------------------------------------------------------------------
// Granny vertex definition - for use outside of the engine library.
//   64 bytes -- Keep this in sync with VS_BACKGROUND_INPUT_64 defined in dxC_VertexDeclarations.h
//-------------------------------------------------------------------------------------------------
struct GRANNY_VERTEX_64
{ 
	VECTOR vPosition;									// Vertex Position
	DWORD vNormal;										// Vertex Normal
	FLOAT  fTextureCoords[TEXCOORDS_MAX_CHANNELS][2];	// The texture coordinates
	VECTOR vBinormal;									// Used for Pixel Lighting
	VECTOR vTangent;									// Used for Pixel Lighting
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------
#ifndef __LOD__
#include "lod.h"
#endif

PRESULT e_GrannyInit ();
PRESULT e_GrannyClose();

BOOL e_GrannyUpdateDRLGFile ( 
	const char * pszFileName, 
	const char * pszFileNameWithPath );

BOOL e_GrannyUpdateModelFile ( 
	const char * pszFileName, 
	BOOL bHasRoomData = FALSE,
	int nLOD = LOD_ANY );

BOOL e_GrannyUpdateModelFileEx (
	const char * pszFileName,
	BOOL bHasRoomData,
	bool bSourceFileOptional,
	int nLOD );

void e_GrannyConvertCoordinateSystem ( 
	struct granny_file_info * pGrannyFileInfo );

granny_texture * e_GrannyGetTextureFromChannelName(
	granny_material * pMaterial,
	char * pszChannelName,
	int nSubMapIndex =- 1 );

granny_material * e_GrannyGetMaterialFromChannelName(
	granny_material * pMaterial,
	char * pszChannelName,
	int nSubMapIndex );

void e_AddToModelBoundingBox(
	int nRootModelDef,
	int nAddModelDef );

granny_file * e_GrannyReadFile( 
	const char * pszFileName);

DWORD e_GrannyTransformAndCompressNormal (
	VECTOR * pvGrannyNormal,
	MATRIX * pmMatrix );

VECTOR e_GrannyTransformNormal (
	VECTOR * pvGrannyNormal,
	MATRIX * pmMatrix );

void e_GrannyModelToDRLGRooms (
	granny_model * pGrannyModel,
	struct DRLG_ROOM * pRooms,
	char * pszFilePath,
	int * pnRoomCount,
	int nMaxRooms );

void e_GrannyGetVertices(
	GRANNY_VERTEX_64*& pVertices,
	int & nVertexCount,
	int & nVertexBufferSize,
	granny_mesh * pMesh,
	MATRIX & mTransformationMatrix );

void e_GrannyGetTransformationFromOriginMesh (
	MATRIX * pOutMatrix,
	granny_mesh * pMesh,
	granny_tri_material_group * pTriMaterialGroup,
	const char * pszFileName );

void e_GrannyGetVectorAndRotationFromOriginMesh(
	granny_mesh * pMesh,
	granny_tri_material_group * pTriMaterialGroup,
	VECTOR *pvPos,
	float *pfRotation );

void e_GrannyCollapseMeshByUsage ( 
	WORD * pwTriangleCount,
	int * pnIndexBufferSize,
	INDEX_BUFFER_TYPE * pwIndexBuffer,
	int nMeshTriangleCount, 
	BOOL * pbIsSubsetTriangle );

void e_GrannyFindLowestPointInVertexList (
	void * pVertices,
	int nVertexCount,
	int nVertexStructSize,
	VECTOR * pvPosition,
	int * pnDirection );

void e_GrannyFindAveragePointAndTextureCorner(
	GRANNY_VERTEX_64* pVertices,
	int nVertexCount,
	VECTOR & vPosition,
	VECTOR & vDirection );

BOOL e_IsMax9Exporter(
	granny_exporter_info * pInfo );

BOOL e_IsCollisionMaterial(
	const char * pszName,
	const BOOL bIsMax9Model );

#endif // __E_GRANNY_RIGID__
