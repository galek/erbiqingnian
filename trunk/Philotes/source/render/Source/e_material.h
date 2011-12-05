//----------------------------------------------------------------------------
// e_material.h
//
// - Header for material functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_MATERIAL__
#define __E_MATERIAL__


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define SPECULAR_MAX_POWER				128.0f
#define SPECULAR_MIN_GLOSSINESS			0.001f

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum MATERIAL_SCROLL_CHANNEL
{
	MATERIAL_SCROLL_DIFFUSE = 0,
	MATERIAL_SCROLL_NORMAL,
	// count
	NUM_MATERIAL_SCROLL_CHANNELS
};

enum MATERIAL_OVERRIDE_TYPE
{
	MATERIAL_OVERRIDE_NORMAL = 0,
	MATERIAL_OVERRIDE_CLONE,
	// count
	NUM_MATERIAL_OVERRIDE_TYPES
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

struct EFFECT_DEFINITION;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct GLOBAL_MATERIAL_DEFINITION
{
	char						szName[ DEFAULT_INDEX_SIZE ];
	char						szMaterial[ DEFAULT_INDEX_SIZE ];
	int							nID;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline float e_GetMinGlossiness()
{
	return SPECULAR_MIN_GLOSSINESS;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_MaterialNew ( int* pnMaterialID, char * pszMaterialName, BOOL bForceSyncLoad = FALSE );
BOOL MaterialDefinitionPostXMLLoad( void * pDef, BOOL bForceSyncLoad );
void e_MaterialsRefresh ( BOOL bForceSyncLoad = FALSE );
PRESULT e_ShadersReloadAll( BOOL bForceSyncLoad = FALSE );
void e_MaterialRelease( int nMaterialID );
PRESULT e_MaterialRestore ( void * pData );
PRESULT e_MaterialRestore ( int nMaterialID );
struct EFFECT_DEFINITION * e_MaterialGetEffectDef( int nMaterial, int nShaderType );
int e_MaterialGetShaderLineID( int nMaterial );
void e_MaterialUpdate ( void * pData );
PRESULT e_LoadGlobalMaterials();
int e_GetGlobalMaterial( int nIndex );
PRESULT e_MaterialComputeScrollAmtLCM( struct MATERIAL * pMaterial );

#endif // __E_MATERIAL__
