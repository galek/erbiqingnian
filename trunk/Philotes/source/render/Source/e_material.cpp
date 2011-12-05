//----------------------------------------------------------------------------
// e_material.cpp
//
// - Implementation for material functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_definition.h"
#include "appcommon.h"
#include "filepaths.h"
#include "datatables.h"
#include "e_material.h"
#include "config.h"
#include "pakfile.h"
#include "fillpak.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_LoadGlobalMaterials()
{
	// Changed to call fillpak load global material code later
	FillPak_LoadGlobalMaterials();

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_MaterialGetShaderLineID( int nMaterial )
{
	MATERIAL * pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterial );
	if ( ! pMaterial )
		return INVALID_ID;
	return pMaterial->nShaderLineId;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_MaterialRestore ( int nMaterialID )
{
	MATERIAL * pMaterial = (MATERIAL *) DefinitionGetById( DEFINITION_GROUP_MATERIAL, nMaterialID );
	if ( ! pMaterial )
		return S_FALSE;
	return e_MaterialRestore( pMaterial );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_GetGlobalMaterial( int nIndex )
{
	if ( nIndex == INVALID_ID )
		return INVALID_ID;
	const GLOBAL_MATERIAL_DEFINITION * pDef = (const GLOBAL_MATERIAL_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_MATERIALS_GLOBAL, nIndex );
	ASSERT_RETINVALID( pDef );
	return pDef->nID;
}