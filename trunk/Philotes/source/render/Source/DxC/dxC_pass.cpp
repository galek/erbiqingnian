//----------------------------------------------------------------------------
// dxC_pass.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_model.h"
#include "dxC_renderlist.h"
#include "dxC_meshlist.h"
#include "dxC_render.h"
#include "dxC_viewer.h"
#include "dxC_commandbuffer.h"
#include "e_settings.h"

#include "dxC_pass.h"


namespace FSSE
{;


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef void (*PFN_RPD_CREATE)( RenderPassDefinition & tDef );

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BOOL gbRPDAlpha[ NUM_RENDER_PASS_TYPES ] =
{
#	define INCLUDE_RPD_ALPHA
#	include "dxC_pass_def.h"
};

//----------------------------------------------------------------------------
// Include pass definition enum string
#define INCLUDE_RPD_ENUM_STR
const char gszRPDEnum[ NUM_RENDER_PASS_TYPES ][ MAX_PASS_NAME_LEN ] =
{
#	include "dxC_pass_def.h"
};

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Include pass definition functions
#define INCLUDE_RPD_ARRAY
#include "dxC_pass_def.h"
//----------------------------------------------------------------------------


PRESULT dxC_PassDefinitionGet(
	RENDER_PASS_TYPE ePass,
	RenderPassDefinition & tRPD )
{
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( ePass, NUM_RENDER_PASS_TYPES ) );

	tRPD.Reset();
	ASSERT_RETFAIL( gpfn_RPDCreate[ePass] );
	gpfn_RPDCreate[ ePass ]( tRPD );

	return S_OK;
}


PRESULT dxC_PassEnumGetString(
	RENDER_PASS_TYPE ePass,
	WCHAR * pwszString,
	int nBufLen )
{
	ASSERT_RETINVALIDARG( pwszString );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( ePass, NUM_RENDER_PASS_TYPES ) );

	PStrCvt( pwszString, gszRPDEnum[ ePass ], nBufLen );
	return S_OK;
}


}; // namespace FSSE