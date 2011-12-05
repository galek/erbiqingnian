//----------------------------------------------------------------------------
// dxC_dpvs.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#if !defined(__DXC_DPVS__)
#define __DXC_DPVS__

#include "e_dpvs_priv.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void dxC_DPVS_SetEnabled				( class DPVS::Object * pObject, BOOL bEnabled );
void dxC_DPVS_SetCell					( class DPVS::Object * pObject, LP_INTERNAL_CELL pCell );
void dxC_DPVS_SetObjToCellMatrix		( class DPVS::Object * pObject, MATRIX & tMatrix );
void dxC_DPVS_SetTestModel				( class DPVS::Object * pObject, class DPVS::Model * pCullModel );
void dxC_DPVS_SetWriteModel				( class DPVS::Object * pObject, class DPVS::Model * pWriteModel );
void dxC_DPVS_SetVisibilityParent		( class DPVS::Object * pObject, class DPVS::Object * pParent );

LP_INTERNAL_CELL dxC_DPVS_GetCell		( class DPVS::Object * pObject );

BOOL dxC_DPVS_ModelGetVisible			( const D3D_MODEL * pModel );
void dxC_DPVS_ModelDefCreateTestModel	( D3D_MODEL_DEFINITION * pModelDef );
void dxC_DPVS_ModelDefCreateWriteModel	( D3D_MODEL_DEFINITION * pModelDef );
void dxC_DPVS_ModelCreateCullObject		( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef );
void dxC_DPVS_ModelSetCell				( D3D_MODEL * pModel, LP_INTERNAL_CELL pNewCell );
PRESULT dxC_DPVS_ModelUpdate			( D3D_MODEL * pModel );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------


#endif // __DXC_DPVS__