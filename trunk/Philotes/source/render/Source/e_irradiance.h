//----------------------------------------------------------------------------
// e_irradiance.h
//
// - Header for lightmaps, SH, or anything else representing irradiance
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_IRRADIANCE__
#define __E_IRRADIANCE__


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// uncomment to enable timing of lightmap renders (slows load)
//#define LIGHTMAP_TIMERS

#define CUBEMAP_GEN_FLAG_APPLY_TO_ENV			MAKE_MASK(0)

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

template <int order>
struct SH_COEFS_RGB
{
	static const int nOrder = order;
	static const int nCoefs = order * order;
	float pfRed  [ order * order ];
	float pfGreen[ order * order ];
	float pfBlue [ order * order ];
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern int gnEnvMapTextureID;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline BOOL e_GeneratingCubeMap()
{
	extern BOOL gbCubeMapGenerate;
	return gbCubeMapGenerate;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_CubeMapSetGenerating( BOOL bGenerating )
{
	extern BOOL gbCubeMapGenerate;
	gbCubeMapGenerate = bGenerating;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline int & e_CubeMapCurrentFace()
{
	extern int gnCubeMapFace;
	return gnCubeMapFace;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline int & e_CubeMapSize()
{
	extern int gnCubeMapSize;
	return gnCubeMapSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline char * e_CubeMapFilename()
{
	extern char gszCubeMapFilename[];
	return gszCubeMapFilename;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline DWORD & e_CubeMapFlags()
{
	extern DWORD gdwCubeMapFlags;
	return gdwCubeMapFlags;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void e_GenerateEnvironmentMap( int nSizeOverride = 0, const char * pszFilenameOverride = NULL, DWORD dwFlags = CUBEMAP_GEN_FLAG_APPLY_TO_ENV );
void e_SetupCubeMapMatrices(
	MATRIX *pmatView, 
	MATRIX *pmatProj, 
	MATRIX *pmatProj2, 
	VECTOR *pvEyeVector, 
	VECTOR *pvEyeLocation );
void e_SetupCubeMapGeneration();
void e_CubeMapGenSetViewport();
void e_SaveCubeMapFace();	// CML: Does nothing in DX10
void e_SaveFullCubeMap();

PRESULT e_GetSHCoefsFromCubeMap(
	__in int nCubeMapID,
	__out SH_COEFS_RGB<2> & tCoefs2,
	__out SH_COEFS_RGB<3> & tCoefs3,
	BOOL bForceLinear );

#endif // __E_IRRADIANCE__
