//-------------------------------------------------------------------------------------------------
//
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
//-------------------------------------------------------------------------------------------------
#ifndef _INCLUDE_DX9_HAVOKFX
#define _INCLUDE_DX9_HAVOKFX

//-------------------------------------------------------------------------------------------------
// INCLUDES
//-------------------------------------------------------------------------------------------------
#include "e_particle.h"

int dxC_HavokFXGetRigidAllTransforms(LPD3DCVB ppRigidBodyTransforms[ HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS ], LPD3DCVB * posVB, LPD3DCVB * velVB );
//int dxC_HavokFXGetRigidAllTransforms(
//	IDirect3DVertexBuffer9 * ppRigidBodyTransforms[ HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS ],
//	LPD3DCVB* posVB,
//	LPD3DCVB* velVB
//);


void dxC_HavokFXDeviceInit();
void dxC_HavokFXDeviceLost();
void dxC_HavokFXDeviceReset();
void dxC_HavokFXDeviceRelease();


#endif