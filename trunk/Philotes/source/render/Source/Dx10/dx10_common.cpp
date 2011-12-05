//----------------------------------------------------------------------------
// dx10_common.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_statemanager.h"
#include "dxC_state.h"


extern CStateManagerInterface *		gpStateManager;

void dx10_DrawFullscreenQuadXY( D3D_EFFECT* pEffect )
{
	dxC_SetStreamSource( 0, NULL, 0, 0 );
	dxC_SetVertexDeclaration( VERTEX_DECL_INVALID, pEffect );
	dxC_GetDevice()->IASetPrimitiveTopology( D3DPT_TRIANGLELIST );

	gpStateManager->ApplyBeforeRendering( pEffect );
	dxC_GetDevice()->Draw( 3, 0 );
}