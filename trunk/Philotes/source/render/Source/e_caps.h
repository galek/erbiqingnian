//----------------------------------------------------------------------------
// e_caps.h
//
// - Header for capabilities structure
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_CAPS__
#define __E_CAPS__


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define BYTES_PER_MB	(1024 * 1024)

#define UNDEFINED_CAP	0xffffffff

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#define IS_VALID_CAP_TYPE( eType )	( (eType) > CAP_DISABLE && (eType) < NUM_ENGINE_CAPS )

#define FOURCCSTRING(code)		(code)

#define FOURCCSTRING_LEN		5

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

#define INCLUDE_CAPS_ENUM
enum ENGINE_CAP
{
	CAP_DISABLE = 0,
	#include "e_caps_def.h"
	// count
	NUM_ENGINE_CAPS
};
#undef INCLUDE_CAPS_ENUM


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct EngineCap
{
	DWORD dwFourCC;
	char szDesc[ 128 ];
	BOOL bSmallerBetter;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline const char * e_CapGetDescription( const ENGINE_CAP eCap )
{
	ASSERT_RETNULL( IS_VALID_CAP_TYPE( eCap ) );
	extern EngineCap gtCaps[];
	return gtCaps[ eCap ].szDesc;
}

inline DWORD e_CapGetFourCC( const ENGINE_CAP eCap )
{
	ASSERT_RETNULL( IS_VALID_CAP_TYPE( eCap ) );
	extern EngineCap gtCaps[];
	return gtCaps[ eCap ].dwFourCC;
}

inline void e_CapGetFourCCString( const ENGINE_CAP eCap, CHAR * pszOutString, int nBufLen )
{
	pszOutString[0] = NULL;
	ASSERT_RETURN( IS_VALID_CAP_TYPE( eCap ) );
	extern EngineCap gtCaps[];
	PStrCopy( pszOutString, (CHAR*) & FOURCCSTRING( gtCaps[ eCap ].dwFourCC ), nBufLen );
}

inline DWORD e_CapGetValue( const ENGINE_CAP eCap )
{
	ASSERT_RETNULL( IS_VALID_CAP_TYPE( eCap ) );
	extern DWORD gdwCapValues[];
	return gdwCapValues[ eCap ];
}

inline BOOL e_CapIsSmallerBetter( const ENGINE_CAP eCap )
{
	ASSERT_RETNULL( IS_VALID_CAP_TYPE( eCap ) );
	extern EngineCap gtCaps[];
	return gtCaps[ eCap ].bSmallerBetter;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_GatherCaps();
PRESULT e_GatherPlatformCaps();
unsigned int e_GetMaxVSConstants();
BOOL e_CheckVersion();
int e_GetMaxPhysicalVideoMemoryMB();
void e_DebugOutputCaps();
void e_DebugOutputPlatformCaps();
PRESULT e_CapSetValue( const ENGINE_CAP eCap, const DWORD dwValue );
DWORD e_GetEngineTarget4CC();
void e_GetMaxTextureSizes( int & nWidth, int & nHeight );
BOOL e_ShouldCreateMinimalDevice();

#endif // __E_CAPS__