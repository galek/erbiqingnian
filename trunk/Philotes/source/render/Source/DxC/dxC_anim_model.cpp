//----------------------------------------------------------------------------
// dxC_anim_model.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_anim_model.h"


granny_data_type_definition g_ptAnimatedVertexDataType[] =
{
	{GrannyReal32Member, GrannyVertexPositionName,				0, 3},
	{GrannyReal32Member, GrannyVertexNormalName,				0, 3},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName "0",0, 2},
	{GrannyInt8Member,   GrannyVertexBoneIndicesName,			0, 4},
	{GrannyReal32Member, GrannyVertexBoneWeightsName,			0, 3},
	{GrannyReal32Member, GrannyVertexTangentName,				0, 3},
	{GrannyReal32Member, GrannyVertexBinormalName,				0, 3},
	{GrannyEndMember},
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template <XFER_MODE mode>
PRESULT dxC_AnimatingModelDefinitionXfer( 	
	BYTE_XFER<mode> & tXfer,
	ANIMATING_MODEL_DEFINITION& tAnimModelDef )
{
	BYTE_XFER_ELEMENT( tXfer, tAnimModelDef.dwFlags );
	BYTE_XFER_ELEMENT( tXfer, tAnimModelDef.nBoneCount );
	BYTE_XFER_ELEMENT( tXfer, tAnimModelDef.dwBoneIsSkinned );

	for ( int nMesh = 0; nMesh < MAX_MESHES_PER_MODEL; nMesh++ )
		for ( int nBone = 0; nBone < MAX_BONES_PER_SHADER_ON_DISK; nBone++ )
			BYTE_XFER_ELEMENT( tXfer, tAnimModelDef.ppnBoneMappingModelToMesh[ nMesh ][ nBone ] );

	int nBoneNameOffsetCount = 0;
	if ( tXfer.IsSave() )
		if ( tAnimModelDef.pnBoneNameOffsets )
			nBoneNameOffsetCount = tAnimModelDef.nBoneCount;
	
	BYTE_XFER_ELEMENT( tXfer, nBoneNameOffsetCount );
	
	if ( tXfer.IsLoad() && nBoneNameOffsetCount > 0 )
		tAnimModelDef.pnBoneNameOffsets = MALLOC_TYPE( int, NULL, 
			tAnimModelDef.nBoneCount );
	if ( nBoneNameOffsetCount > 0 )
		BYTE_XFER_POINTER( tXfer, tAnimModelDef.pnBoneNameOffsets, 
			nBoneNameOffsetCount * sizeof( int ) );

	BYTE_XFER_ELEMENT( tXfer, tAnimModelDef.nBoneNameBufferSize );
	if ( tXfer.IsLoad() && tAnimModelDef.nBoneNameBufferSize > 0 )
		tAnimModelDef.pszBoneNameBuffer = MALLOC_TYPE( char, NULL, 
			tAnimModelDef.nBoneNameBufferSize );
	if ( tAnimModelDef.nBoneNameBufferSize > 0 )
		BYTE_XFER_POINTER( tXfer, tAnimModelDef.pszBoneNameBuffer,
			tAnimModelDef.nBoneNameBufferSize * sizeof( char ) );
	
	BYTE_XFER_ELEMENT( tXfer, tAnimModelDef.nBoneMatrixCount );
	if ( tXfer.IsLoad() && tAnimModelDef.nBoneMatrixCount > 0 ) 
		tAnimModelDef.pmBoneMatrices = MALLOC_TYPE( MATRIX, NULL, 
			tAnimModelDef.nBoneMatrixCount );
	if ( tAnimModelDef.nBoneMatrixCount > 0 )
		BYTE_XFER_POINTER( tXfer, tAnimModelDef.pmBoneMatrices, 
			tAnimModelDef.nBoneMatrixCount * sizeof( MATRIX ) );

	return S_OK;
}
