//----------------------------------------------------------------------------
// dx9_renderlist.cpp
//
// - Implementation for renderlist
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"


//#include "plane.h"
//#include "boundingbox.h"
//#include "e_frustum.h"
//#include "e_settings.h"
#include "dxC_model.h"

#include "dxC_renderlist.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_RenderListAddAllModels(
	PASSID	nPass,
	DWORD	dwFlags /*=0*/ )
{
	for ( D3D_MODEL * pModel = dx9_ModelGetFirst();
		pModel;
		pModel = dx9_ModelGetNext( pModel ) )
	{
		dxC_ModelUpdateVisibilityToken( pModel );
		e_RenderListAddModel( nPass, pModel->nId, dwFlags );
	}
}
