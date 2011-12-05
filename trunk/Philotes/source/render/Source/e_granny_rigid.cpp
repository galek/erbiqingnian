//----------------------------------------------------------------------------
// e_granny_rigid.cpp
//
// - Implementation for Granny rigid model functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_common.h"
#include "granny.h"


#include "boundingbox.h"
#include "e_model.h"

#include "game.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "e_granny_rigid.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static BOOL sg_bGrannyInitialized = FALSE;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

static void sGrannyError(
	granny_log_message_type,	// Type
	granny_log_message_origin,	// Origin
	char const *Error,
	void *)						// UserData
{
	LogMessage(ERROR_LOG, "GRANNY: \"%s\"\n", Error);
}

GRANNY_CALLBACK(void*) sGrannyAllocate(
	char const *File, 
	granny_int32x Line, 
    granny_intaddrx Alignment, 
    granny_intaddrx Size)
{
	return ALIGNED_MALLOC(NULL, (unsigned int)Size, (MEMORY_SIZE)Alignment);   
}

GRANNY_CALLBACK(void) sGrannyDeallocate(
	char const *File, 
	granny_int32x Line, 
	void *Memory)
{
    FREE(NULL, Memory);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_GrannyInit( )
{
	if ( sg_bGrannyInitialized )
		return S_FALSE;
	sg_bGrannyInitialized = TRUE;

	// The very first thing any Granny app should do is make sure the
	//.h file they used during compilation matches the DLL they're
	//running with.  This macro does it automatically. 
	if (!GrannyVersionsMatch)
	{
		LogMessage(ERROR_LOG, "Warning: the Granny DLL currently loaded doesn't match the .h file used during compilation");
	}

	// set up error callback
	granny_log_callback Callback;
	Callback.Function = sGrannyError;
	Callback.UserData = 0;
	GrannySetLogCallback(&Callback);
	
	// Override granny allocators
	//
	GrannySetAllocator(sGrannyAllocate, sGrannyDeallocate);

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GrannyClose()
{
	sg_bGrannyInitialized = FALSE;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_GrannyConvertCoordinateSystem ( 
	granny_file_info * pGrannyFileInfo )
{
	granny_art_tool_info *pArtToolInfo = pGrannyFileInfo->ArtToolInfo;
	float pfRightVector[]	= {-1, 0, 0};
	float pfBackVector[]	= {0, -1, 0};
	float pfUpVector[]		= {0, 0, 1};
	float pfAffine3[3];
	float tLinear3x3[9];
	float tInverseLinear3x3[9];
	GrannyComputeBasisConversion( pGrannyFileInfo, 
		//pArtToolInfo->UnitsPerMeter, 
		1.0f, 
		pArtToolInfo->Origin, 
		pfRightVector, 
		pfUpVector, 
		pfBackVector,
		pfAffine3, 
		tLinear3x3, 
		tInverseLinear3x3);
	GrannyTransformFile( pGrannyFileInfo, pfAffine3, tLinear3x3, tInverseLinear3x3, 1e-5f, 1e-5f, GrannyRenormalizeNormals | GrannyReorderTriangleIndices );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

granny_texture * e_GrannyGetTextureFromChannelName( granny_material * pMaterial, char * pszChannelName, int nSubMapIndex )
{
	granny_material *pFoundMaterial;
	for ( int i = 0; i < pMaterial->MapCount; i++ )
	{
		if ( PStrICmp( pMaterial->Maps[i].Usage, pszChannelName ) == 0 )
		{
			pFoundMaterial = pMaterial->Maps[i].Material;

			if ( nSubMapIndex >= 0 )
			{
				// check if there is a map at the specified index
				if ( pFoundMaterial->MapCount > nSubMapIndex )
				{
					// Get information on the sub-material map # [nSubMapIndex]
					granny_material_map &pMap = pFoundMaterial->Maps[ nSubMapIndex ];

					// Get the pointer to the sub-material
					granny_material *pSubMaterial = pMap.Material;
					if ( pSubMaterial && pSubMaterial->Texture )
					{
						// return the first texture found
						return pSubMaterial->Texture;
					}
				}
			} else
			{
				// return the first texture found
				return pFoundMaterial->Texture;
			}
		}
	}

	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

granny_material * e_GrannyGetMaterialFromChannelName( granny_material * pMaterial, char * pszChannelName, int nSubMapIndex )
{
	granny_material *pFoundMaterial;
	for ( int i = 0; i < pMaterial->MapCount; i++ )
	{
		if ( PStrICmp( pMaterial->Maps[i].Usage, pszChannelName ) == 0 )
		{
			pFoundMaterial = pMaterial->Maps[i].Material;

			if ( nSubMapIndex >= 0 )
			{
				// check if there is a map at the specified index
				if ( pFoundMaterial->MapCount > nSubMapIndex )
				{
					// Get information on the sub-material map # [nSubMapIndex]
					granny_material_map &pMap = pFoundMaterial->Maps[ nSubMapIndex ];

					// Get the pointer to the sub-material
					granny_material *pSubMaterial = pMap.Material;
					if ( pSubMaterial && pSubMaterial->Texture )
					{
						// return the first material found
						return pSubMaterial;
					}
				}
			} else
			{
				// return the first material found
				return pFoundMaterial;
			}
		}
	}

	return NULL;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
granny_file * e_GrannyReadFile( 
	const char * pszFileName)
{
	TCHAR szFullname[DEFAULT_FILE_WITH_PATH_SIZE];
	FileGetFullFileName(szFullname, pszFileName, DEFAULT_FILE_WITH_PATH_SIZE);

	granny_file * pGrannyFile = GrannyReadEntireFile( szFullname );
	return pGrannyFile;
}

//-------------------------------------------------------------------------------------------------
// this starts a conversion utility for updating our granny file version - the bones no longer have a light info structure in the new version
//-------------------------------------------------------------------------------------------------

void CopyVariantIntoBuilder(granny_variant_builder* VBuilder,
							granny_uint8* Obj,
							granny_data_type_definition* Type)
{
	granny_uint8* CurrentObj                 = Obj;
	granny_data_type_definition* CurrentType = Type;

	while (CurrentType && CurrentType->Type != GrannyEndMember)
	{
		switch (CurrentType->Type)
		{
		case GrannyReal32Member:
			{
				if (CurrentType->ArrayWidth == 0)
				{
					granny_real32 Val = *((granny_real32*)CurrentObj);
					GrannyAddScalarMember(VBuilder, CurrentType->Name, Val);
				}
				else
				{
					granny_real32* Vals = (granny_real32*)CurrentObj;
					GrannyAddScalarArrayMember(VBuilder, CurrentType->Name, CurrentType->ArrayWidth, Vals);
				}
			} break;

		case GrannyInt32Member:
			{
				granny_int32 Val = *((granny_int32*)CurrentObj);
				GrannyAddIntegerMember(VBuilder, CurrentType->Name, Val);
			} break;

		case GrannyStringMember:
			{
				char* Vals = *((char**)CurrentObj);
				GrannyAddStringMember(VBuilder, CurrentType->Name, Vals);
			} break;

		case GrannyReferenceMember:
			{
				void* Obj = *((void**)CurrentObj);
				GrannyAddReferenceMember(VBuilder, CurrentType->Name, CurrentType->ReferenceType, Obj);
			} break;

		case GrannyReferenceToArrayMember:
			assert(false);
			break;

		default:
			break;
		}

		CurrentObj += GrannyGetMemberTypeSize(CurrentType);
		++CurrentType;
	}
}


bool
ConvertOldBone(granny_bone* NewBone,
			   void* OldBone,
			   granny_data_type_definition* OldBoneType,
			   granny_string_table *STable)
{
	assert(NewBone);
	assert(OldBone);
	assert(OldBoneType);

	// This routine is going to leak, but we'll just live with it.
	granny_variant_builder *VBuilder = GrannyBeginVariant(STable);
	CopyVariantIntoBuilder(VBuilder,
		(granny_uint8*)NewBone->ExtendedData.Object,
		NewBone->ExtendedData.Type);

	granny_variant OldLightInfo;
	if(GrannyFindMatchingMember(OldBoneType, 0, "LightInfo", &OldLightInfo))
	{
		void** PLightInfo = (void**)((granny_uint8*)OldBone + (granny_intaddrx)OldLightInfo.Object);
		granny_uint8* LightInfo = (granny_uint8*)*PLightInfo;

		if (LightInfo != 0)
		{
			// Lightinfo is a variant reference type, we need to dereference it
			granny_data_type_definition* Type = (granny_data_type_definition*)*((void**)(LightInfo + 0));
			void* Obj                         = *((void**)(LightInfo + 4));

			granny_variant_builder *LightInfoBuilder = GrannyBeginVariant(STable);
			CopyVariantIntoBuilder(LightInfoBuilder, (granny_uint8*)Obj, Type);

			granny_variant LightExtendedData;
			GrannyEndVariant(LightInfoBuilder,
				&LightExtendedData.Type,
				&LightExtendedData.Object);

			GrannyAddReferenceMember(VBuilder,
				"LightInfo",
				LightExtendedData.Type,
				LightExtendedData.Object);
		}
	}


	granny_variant OldCameraInfo;
	if(GrannyFindMatchingMember(OldBoneType, 0, "CameraInfo", &OldCameraInfo))
	{
	}

	bool Success = GrannyEndVariant(VBuilder,
		&NewBone->ExtendedData.Type,
		&NewBone->ExtendedData.Object);

	return Success;
}


bool
SkeletonSubConversion(granny_skeleton* NewSkeleton,
					  void* OldSkeleton,
					  granny_data_type_definition* OldType,
					  granny_string_table *STable)
{
	assert(NewSkeleton);
	assert(OldSkeleton);
	assert(OldType);

	granny_variant OldBones;
	if(!GrannyFindMatchingMember(OldType, 0, "Bones", &OldBones))
	{
		return false;
	}

	granny_data_type_definition* OldBoneType = OldBones.Type;
	granny_uint8* OldBoneArrayLoc   = (granny_uint8*)OldSkeleton + (granny_intaddrx)OldBones.Object;
	granny_uint8* OldBoneArray      = *(granny_uint8**)(OldBoneArrayLoc + 4);
	//granny_int32x OldBoneArrayCount = *(granny_int32x*)(OldBoneArrayLoc + 0);

	granny_int32x const OldBoneSize = GrannyGetTotalObjectSize(OldBones.Type->ReferenceType);

	{for (granny_int32x BoneIdx = 0; BoneIdx < NewSkeleton->BoneCount; ++BoneIdx)
	{
		granny_bone* Bone = &NewSkeleton->Bones[BoneIdx];

		void* OldBone = OldBoneArray + (BoneIdx * OldBoneSize);

		if (!ConvertOldBone(Bone, OldBone, OldBoneType[0].ReferenceType, STable))
		{
			return false;
		}
	}}

	return true;
}

bool ConvertOldLightsAndCameras(granny_file_info* TargetInfo,
								granny_file*      SourceFile,
								granny_string_table *STable)
{
	assert(TargetInfo);
	assert(SourceFile);

	granny_variant OldObject;
	GrannyGetDataTreeFromFile(SourceFile, &OldObject);

	granny_variant OldSkeletons;
	if(!GrannyFindMatchingMember(OldObject.Type, 0,
		"Skeletons", &OldSkeletons))
	{
		return false;
	}

	granny_data_type_definition* OldType = OldSkeletons.Type;
	void** OldPointers = *((void ***)((granny_uint8*)OldObject.Object + (granny_intaddrx)OldSkeletons.Object + sizeof(granny_intaddrx)));

	{for (granny_int32x SkelIdx = 0; SkelIdx < TargetInfo->SkeletonCount; ++SkelIdx)
	{
		granny_skeleton* Skeleton = TargetInfo->Skeletons[SkelIdx];

		void* OldSkeleton = OldPointers[SkelIdx];

		if (!SkeletonSubConversion(Skeleton, OldSkeleton, OldType[0].ReferenceType, STable))
		{
			return false;
		}
	}}

	return true;
}

// Setup any compression or object data section parameters here...
bool WriteToFile(granny_data_type_definition* RootObjectTypeDefinition,
				 void* RootObject,
				 const char* Filename)
{
	granny_file_builder *Builder = GrannyBeginFile(1, GrannyCurrentGRNStandardTag,
		GrannyGRNFileMV_ThisPlatform,
		GrannyGetTemporaryDirectory(),
		"Prefix");
	granny_file_data_tree_writer *Writer =
		GrannyBeginFileDataTreeWriting(RootObjectTypeDefinition, RootObject, 0, 0);
	GrannyWriteDataTreeToFileBuilder(Writer, Builder);
	GrannyEndFileDataTreeWriting(Writer);

	return GrannyEndFile(Builder, Filename);
}


void e_GrannyConvertFile( const char * pszToConvert, const char * pszConverted )
{
	granny_file* pToConvert = GrannyReadEntireFile(pszToConvert);
	ASSERTX_RETURN( pToConvert, pszToConvert );

	granny_file* pConverted = GrannyReadEntireFile(pszToConvert);
	ASSERTX_RETURN( pConverted, pszToConvert );

	granny_file_info* pInfo = GrannyGetFileInfo(pConverted);
	ASSERTX_RETURN( pInfo, "Unable to get file info\n" );

	// Leak the string table for simplicity
	granny_string_table* STable = GrannyNewStringTable();
	if (!ConvertOldLightsAndCameras(pInfo, pToConvert, STable))
	{
		ASSERTX( 0, "Failed to perform the conversion\n" );
		GrannyFreeFile(pToConvert);
		GrannyFreeFile(pConverted);
		return;
	}

	// Write out the new info...
	ASSERT( WriteToFile(GrannyFileInfoType, pInfo, pszConverted ));
}

BOOL e_IsMax9Exporter( granny_exporter_info * pInfo )
{
	ASSERT_RETFALSE( pInfo );

	const int MAX9_MAJOR  = 2;
	const int MAX9_MINOR  = 7;
	const int MAX9_CUSTOM = 0;
	const int MAX9_BUILD  = 25;

	if ( pInfo->ExporterMajorRevision < MAX9_MAJOR )		return FALSE;
	if ( pInfo->ExporterMajorRevision > MAX9_MAJOR )		return TRUE;
	if ( pInfo->ExporterMinorRevision < MAX9_MINOR )		return FALSE;
	if ( pInfo->ExporterMinorRevision > MAX9_MINOR )		return TRUE;
	if ( pInfo->ExporterCustomization < MAX9_CUSTOM )		return FALSE;
	if ( pInfo->ExporterCustomization > MAX9_CUSTOM )		return TRUE;
	if ( pInfo->ExporterBuildNumber   < MAX9_BUILD )		return FALSE;
	if ( pInfo->ExporterBuildNumber   > MAX9_BUILD )		return TRUE;

	// equal to minimum max9 version
	return TRUE;
}

BOOL e_IsCollisionMaterial(
	const char * pszName,
	const BOOL bIsMax9Model)
{
	if(bIsMax9Model)
	{
		return PStrStrI(pszName, "collision_") == pszName;
	}
	else
	{
		return PStrICmp(pszName, "collision") == 0;
	}
}

