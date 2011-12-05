//----------------------------------------------------------------------------
// dx10_fluid.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.

//CL used : 1656100
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_target.h"
#include "dxC_buffer.h"

#include "dxC_state.h"
#include "dxC_device_priv.h"
#include "dxC_light.h"
#include "dxC_model.h"
#include "filepaths.h"
#include "c_camera.h"

#include "dx10_fluid.h"
#ifndef PROFILE
#include "nvapi.h"
#endif
#include <math.h>
#include <iostream>
#include <stdlib.h>


using namespace std;

#define G80_RASTER_RULES

#define USE_OBSTACLE_OPTIMIZATION

#define ADD_TEXTURE_DENSITY
#define ADD_TEXTURE_VELOCITY

static int largestRTWidth;
static int largestRTHeight;
//----------------------------------------------------------------------------
//helper functions
//----------------------------------------------------------------------------


void SetRenderTargetAndViewport(RENDER_TARGET_INDEX rtIndex, int width, int height, DEPTH_TARGET_INDEX zbuffer = DEPTH_TARGET_NONE )
{
   V( dxC_SetRenderTargetWithDepthStencil(rtIndex, zbuffer) );
   V( dxC_SetViewport(0,0,width,height,0,1) );
}

//clear the render target
void ClearRenderTarget( RENDER_TARGET_INDEX rtIndex, float r,float g,float b,float a )
{
	D3DCOLOR ClearColor  = ARGB_MAKE_FROM_INT( r*255, g*255, b*255, a*255 );
	V( dxC_SetRenderTargetWithDepthStencil(rtIndex, DEPTH_TARGET_NONE ) );
	V( dxC_ClearBackBufferPrimaryZ(D3DCLEAR_TARGET, ClearColor, 1, 0) );
}

void ClearRenderTarget( float r,float g,float b,float a )
{
	D3DCOLOR ClearColor  = ARGB_MAKE_FROM_INT( r*255, g*255, b*255, a*255 );
	V( dxC_ClearBackBufferPrimaryZ(D3DCLEAR_TARGET, ClearColor, 1, 0) );
}

void ClearRenderTarget( float r,float g,float b,float a ,float depth, UINT8 stencil )
{
	D3DCOLOR ClearColor  = ARGB_MAKE_FROM_INT( r*255, g*255, b*255, a*255 );
	V( dxC_ClearBackBufferPrimaryZ(D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER |D3DCLEAR_STENCIL, 
		                        ClearColor, depth, stencil) );
}

//bind a texture in the fx file to the texture of a render target
void SetRTTextureAsShaderResource(RENDER_TARGET_INDEX rtIndex,D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, EFFECT_PARAM eParam )
{

	SPD3DCBASESHADERRESOURCEVIEW pTexture = dxC_RTShaderResourceGet(rtIndex);
	V( dx9_EffectSetTexture( pEffect, *pTechnique, eParam, pTexture ) );

}

void SetAlphaDisable()
{
	//AlphaToCoverageEnable = FALSE;
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE ); //BlendEnable[0] = FALSE;
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA ); //RenderTargetWriteMask[0] = 0x0F;
	
}

void SetAdditiveBlendingSim()
{
    //AlphaToCoverageEnable = FALSE;
    dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );       //BlendEnable[0] = TRUE;
    dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );       //SrcBlend = ONE;
    dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );      //DestBlend = ONE;
    dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );      //BlendOp = ADD;
    dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );  //SrcBlendAlpha = ONE;
    dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE ); //DestBlendAlpha = ONE;
    dxC_SetRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD ); //BlendOpAlpha = ADD;
    dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA ); //RenderTargetWriteMask[0] = 0x0F;
}


void SetAlphaBlendingSim()
{
    //AlphaToCoverageEnable = FALSE;
    dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );          //BlendEnable[0] = TRUE;
    dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );     //SrcBlend = SRC_ALPHA;
    dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ); //DestBlend = INV_SRC_ALPHA;
    dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );         //BlendOp = ADD;
    dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );     //SrcBlendAlpha = ONE;
    dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );    //DestBlendAlpha = ONE;
    dxC_SetRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD );    //BlendOpAlpha = ADD;
    dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA ); //RenderTargetWriteMask[0] = 0x0F;
}

void SetAlphaBlendingSimAddForces()
{
    //AlphaToCoverageEnable = FALSE;
    dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );          //BlendEnable[0] = TRUE;
    dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );          //SrcBlend = ONE;
    dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ); //DestBlend = INV_SRC_ALPHA;
    dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );         //BlendOp = ADD;
    dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );     //SrcBlendAlpha = ONE;
    dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );    //DestBlendAlpha = ONE;
    dxC_SetRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD );    //BlendOpAlpha = ADD;
    dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA ); //RenderTargetWriteMask[0] = 0x0F;
}

void SetAlphaBlendingRendering()
{
    //AlphaToCoverageEnable = FALSE;
    dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );               //BlendEnable[0] = TRUE;
    dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );               //SrcBlend = ONE;
    dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );      //DestBlend = INV_SRC_ALPHA;
    dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );              //BlendOp = ADD;
    dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_SRCALPHA );     //SrcBlendAlpha = SRC_ALPHA;
    dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA ); //DestBlendAlpha = INV_SRC_ALPHA;
    dxC_SetRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD );         //BlendOpAlpha = ADD;
    dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE ); //RenderTargetWriteMask[0] = 0x07;
}

void SetSubtractiveBlendingRendering()
{
	//AlphaToCoverageEnable = FALSE;
    dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );               //BlendEnable[0] = TRUE;
    dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );               //SrcBlend = ONE;
    dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );             //DestBlend = ZERO;
    dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT );      //BlendOp = REV_SUBTRACT;         // DST - SRC
    dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );          //SrcBlendAlpha = ONE;
    dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );         //DestBlendAlpha = ONE;
    dxC_SetRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_REVSUBTRACT);  //BlendOpAlpha = REV_SUBTRACT;    // DST - SRC
    dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA ); //RenderTargetWriteMask[0] = 0x0F;

}

PRESULT dx10_AddFluidRTDescs()
{
	int rows =(int)floorf(sqrtf((float)LARGEST_FLUID_DEPTH));
	int cols = rows;
	while( rows * cols < LARGEST_FLUID_DEPTH ) cols++;
	largestRTWidth = cols * LARGEST_FLUID_WIDTH;
	largestRTHeight = rows * LARGEST_FLUID_HEIGHT;
	dxC_AddRenderTargetDesc(FALSE, -1, GLOBAL_RT_FLUID_TEMP1,			largestRTWidth, largestRTHeight, D3DC_USAGE_2DTEX_RT, D3DFMT_R16F,			-1 );
	dxC_AddRenderTargetDesc(FALSE, -1, GLOBAL_RT_FLUID_TEMP2,			largestRTWidth, largestRTHeight, D3DC_USAGE_2DTEX_RT, D3DFMT_A16B16G16R16F,	-1 );
	dxC_AddRenderTargetDesc(FALSE, -1, GLOBAL_RT_FLUID_TEMPVELOCITY,	largestRTWidth, largestRTHeight, D3DC_USAGE_2DTEX_RT, D3DFMT_A16B16G16R16F,	-1 );

	return S_OK;
}

//---------------------------------------------------------------------------
//Grid
//---------------------------------------------------------------------------

void Grid :: Initialize( int gridWidth, int gridHeight, int gridDepth )
{
   dim[0] = gridWidth;
   dim[1] = gridHeight;
   dim[2] = gridDepth;

    ASSERT_RETURN( gridWidth  <= LARGEST_FLUID_WIDTH );
    ASSERT_RETURN( gridHeight <= LARGEST_FLUID_HEIGHT );
    ASSERT_RETURN( gridDepth  <= LARGEST_FLUID_DEPTH );

    rows =(int)floorf(sqrtf((float)dim[2]));
    cols = rows;
    while( rows * cols < dim[2] ) 
    {
        cols++;
    }
    ASSERT_RETURN( rows*cols >= dim[2] );

   texDim[0] = cols * dim[0];
   texDim[1] = rows * dim[1];

   maxDim = max( max( dim[0], dim[1] ), dim[2] );


   slices         = NULL;
   renderQuad     = NULL;
   boundarySlices = NULL;
   boundaryLines  = NULL;
   boundaryPoints = NULL;

   createVertexBuffers();
}

Grid :: ~Grid()
{
  
   FREE_DELETE_ARRAY(NULL, renderQuad, VS_FLUID_SIMULATION_INPUT_STRUCT);
   FREE_DELETE_ARRAY(NULL, slices, VS_FLUID_SIMULATION_INPUT_STRUCT);
   FREE_DELETE_ARRAY(NULL, boundarySlices, VS_FLUID_SIMULATION_INPUT_STRUCT);
   FREE_DELETE_ARRAY(NULL, boundaryLines, VS_FLUID_SIMULATION_INPUT_STRUCT);
   FREE_DELETE_ARRAY(NULL, boundaryPoints, VS_FLUID_SIMULATION_INPUT_STRUCT);
}


void Grid :: createRenderQuad(VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index )
{
   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex1;
   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex2;
   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex3;
   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex4;


   float w = texDim[0];
   float h = texDim[1];

   tempVertex1.Position  = D3DXVECTOR2( 0, 0 );
   tempVertex1.Tex0 = D3DXVECTOR3( 0, 0, 0.5 );
   tempVertex1.Tex1 = D3DXVECTOR4( 0,0,0,0);
   tempVertex1.Tex2 = D3DXVECTOR4( 0,0,0,0);
   tempVertex1.Tex3 = D3DXVECTOR4( 0,0,0,0);
   tempVertex1.Tex7 = D3DXVECTOR3( 0,0,0);

   tempVertex2.Position  = D3DXVECTOR2( w, 0 );
   tempVertex2.Tex0 = D3DXVECTOR3( w, 0, 0.5 );
   tempVertex2.Tex1 = D3DXVECTOR4( 0,0,0,0);
   tempVertex2.Tex2 = D3DXVECTOR4( 0,0,0,0);
   tempVertex2.Tex3 = D3DXVECTOR4( 0,0,0,0);
   tempVertex2.Tex7 = D3DXVECTOR3( 0,0,0);

   tempVertex3.Position  = D3DXVECTOR2( w, h );
   tempVertex3.Tex0 = D3DXVECTOR3( w, h, 0.5 );
   tempVertex3.Tex1 = D3DXVECTOR4( 0,0,0,0);
   tempVertex3.Tex2 = D3DXVECTOR4( 0,0,0,0);
   tempVertex3.Tex3 = D3DXVECTOR4( 0,0,0,0);
   tempVertex3.Tex7 = D3DXVECTOR3( 0,0,0);

   tempVertex4.Position  = D3DXVECTOR2( 0, h );
   tempVertex4.Tex0 = D3DXVECTOR3( 0, h, 0.5 );
   tempVertex4.Tex1 = D3DXVECTOR4( 0,0,0,0);
   tempVertex4.Tex2 = D3DXVECTOR4( 0,0,0,0);
   tempVertex4.Tex3 = D3DXVECTOR4( 0,0,0,0);
   tempVertex4.Tex7 = D3DXVECTOR3( 0,0,0);

   (*vertices)[index++] = tempVertex1;
   (*vertices)[index++] = tempVertex2;
   (*vertices)[index++] = tempVertex3;
   (*vertices)[index++] = tempVertex1;
   (*vertices)[index++] = tempVertex3;
   (*vertices)[index++] = tempVertex4;

}

void Grid :: createSlice( int z, int texZOffset,VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index )
{

   ///////////////////////////////////////////////////
   //                                               //
   //  texture coordinate enumeration:              //
   //                                               //
   //        |------|                               //
   //        | 2.zw |                               //
   // |------|------|------|   |------|   |------|  //
   // | 1.xy |  0   | 1.zw |   | 3.xy |   | 3.zw |  //
   // |------|------|------|   |------|   |------|  //
   //        | 2.xy |           above      below    //
   //        |------|                               //
   //                                               //
   ///////////////////////////////////////////////////
  
   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex1;
   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex2;
   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex3;
   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex4;
   int column,      row;
   int columnAbove, rowAbove;
   int columnBelow, rowBelow;
   int px, py;
   int x[7], y[7];
   int w = dim[0];
   int h = dim[1];

   //compute lower left corner of geometry
   column      = z % cols;
   row         = (int)floorf((float)(z/cols));
   px = column * dim[0];
   py = row    * dim[1];

   //compute lower left corner of tex coords
   if( texZOffset != 0 )
   {
      z += texZOffset;
      column      = z % cols;
      row         = (int)floorf((float)(z/cols));
   }

   columnAbove = (z+1) % cols;
   rowAbove    = (int)floorf((float)((z+1)/cols));

   columnBelow = (z-1) % cols;
   rowBelow    = (int)floorf((float)((z-1)/cols));

   x[0] = column * dim[0];
   y[0] = row    * dim[1];

   x[1] = x[0] - 1;
   y[1] = y[0];

   x[2] = x[0] + 1;
   y[2] = y[0];

   x[3] = x[0];
   y[3] = y[0] - 1;

   x[4] = x[0];
   y[4] = y[0] + 1;

   x[5] = columnAbove * dim[0];
   y[5] = rowAbove    * dim[1];

   x[6] = columnBelow * dim[0];
   y[6] = rowBelow    * dim[1];

   tempVertex1.Tex0 = D3DXVECTOR3( x[0]+1,   y[0]+1, z   );
   tempVertex1.Tex1 = D3DXVECTOR4( x[1]+1,   y[1]+1,
                                   x[2]+1,   y[2]+1 );
   tempVertex1.Tex2 = D3DXVECTOR4( x[3]+1,   y[3]+1,
                                   x[4]+1,   y[4]+1 );
   tempVertex1.Tex3 = D3DXVECTOR4( x[5]+1,   y[5]+1,
                                   x[6]+1,   y[6]+1 );
   tempVertex1.Tex7 = D3DXVECTOR3( 1, 1, z );  //grid coordinates of center cell (instead of texture coordinates)
   tempVertex1.Position  = D3DXVECTOR2( px+1, py+1 );


   tempVertex2.Tex0 = D3DXVECTOR3( x[0]+w-1, y[0]+1, z   );
   tempVertex2.Tex1 = D3DXVECTOR4( x[1]+w-1, y[1]+1,
                                   x[2]+w-1, y[2]+1 );
   tempVertex2.Tex2 = D3DXVECTOR4( x[3]+w-1, y[3]+1,
                                   x[4]+w-1, y[4]+1 );
   tempVertex2.Tex3 = D3DXVECTOR4( x[5]+w-1, y[5]+1,
                                   x[6]+w-1, y[6]+1 );
   tempVertex2.Tex7 = D3DXVECTOR3( w-1, 1, z );  //grid coordinates of center cell (instead of texture coordinates)
   tempVertex2.Position  = D3DXVECTOR2( px+w-1, py+1 );


   tempVertex3.Tex0 = D3DXVECTOR3( x[0]+w-1, y[0]+h-1, z   );
   tempVertex3.Tex1 = D3DXVECTOR4( x[1]+w-1, y[1]+h-1,
                                   x[2]+w-1, y[2]+h-1 );
   tempVertex3.Tex2 = D3DXVECTOR4( x[3]+w-1, y[3]+h-1,
                                   x[4]+w-1, y[4]+h-1 );
   tempVertex3.Tex3 = D3DXVECTOR4( x[5]+w-1, y[5]+h-1,
                                   x[6]+w-1, y[6]+h-1 );
   tempVertex3.Tex7 = D3DXVECTOR3( w-1, h-1, z );  //grid coordinates of center cell (instead of texture coordinates)
   tempVertex3.Position  = D3DXVECTOR2( px+w-1, py+h-1 );


   tempVertex4.Tex0 = D3DXVECTOR3( x[0]+1,   y[0]+h-1, z   );
   tempVertex4.Tex1 = D3DXVECTOR4( x[1]+1,   y[1]+h-1,
                                   x[2]+1,   y[2]+h-1 );
   tempVertex4.Tex2 = D3DXVECTOR4( x[3]+1,   y[3]+h-1,
                                   x[4]+1,   y[4]+h-1 );
   tempVertex4.Tex3 = D3DXVECTOR4( x[5]+1,   y[5]+h-1,
                                   x[6]+1,   y[6]+h-1 );
   tempVertex4.Tex7 = D3DXVECTOR3( 1, h-1, z );  //grid coordinates of center cell (instead of texture coordinates)
   tempVertex4.Position  = D3DXVECTOR2( px+1, py+h-1 );

   (*vertices)[index++] = tempVertex1;
   (*vertices)[index++] = tempVertex2;
   (*vertices)[index++] = tempVertex3;
   (*vertices)[index++] = tempVertex1;
   (*vertices)[index++] = tempVertex3;
   (*vertices)[index++] = tempVertex4;

}

void Grid :: createLine( float x1, float y1,
                         float x2, float y2, int z,
                         float texXOffset,
                         float texYOffset,
                         int   texZOffset,VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index )
{
  
	int column,      row;
   int columnAbove, rowAbove;
   int columnBelow, rowBelow;
   float offsetX, offsetY;
   float offsetAboveX, offsetAboveY;
   float offsetBelowX, offsetBelowY;
   float X1[7], Y1[7];
   float X2[7], Y2[7];
   int px1, py1;
   int px2, py2;

   //compute geometry coordinates
   column      = z % cols;
   row         = (int)floorf((float)(z/cols));
   offsetX = (float)(column * dim[0]);
   offsetY = (float)(row    * dim[1]);
   px1 = offsetX + x1;      px2 = offsetX + x2;
   py1 = offsetY + y1;      py2 = offsetY + y2;

   //compute slice offset
   x1 += texXOffset;
   y1 += texYOffset;
   x2 += texXOffset;
   y2 += texYOffset;
   z += texZOffset;

   column = z % cols;
   row    = (int)floorf((float)(z/cols));

   offsetX = column * dim[0];
   offsetY = row    * dim[1];
   

   columnAbove = (z+1) % cols;
   rowAbove    = (int)floorf((float)((z+1)/cols));
   offsetAboveX = columnAbove * dim[0];
   offsetAboveY = rowAbove    * dim[1];

   columnBelow = (z-1) % cols;
   rowBelow    = (int)floorf((float)((z-1)/cols));
   offsetBelowX = columnBelow * dim[0];
   offsetBelowY = rowBelow    * dim[1];

   X1[0] = (float)(offsetX + x1);            X2[0] = (float)(offsetX + x2);
   Y1[0] = (float)(offsetY + y1);            Y2[0] = (float)(offsetY + y2);
   X1[1] = (float)(X1[0] - 1);               X2[1] = (float)(X2[0] - 1);
   Y1[1] = (float)(Y1[0]);                   Y2[1] = (float)(Y2[0]);
   X1[2] = (float)(X1[0] + 1);               X2[2] = (float)(X2[0] + 1);
   Y1[2] = (float)(Y1[0]);                   Y2[2] = (float)(Y2[0]);
   X1[3] = (float)(X1[0]);                   X2[3] = (float)(X2[0]);
   Y1[3] = (float)(Y1[0] - 1);               Y2[3] = (float)(Y2[0] - 1);
   X1[4] = (float)(X1[0]);                   X2[4] = (float)(X2[0]);
   Y1[4] = (float)(Y1[0] + 1);               Y2[4] = (float)(Y2[0] + 1);
   X1[5] = (float)(offsetAboveX + x1);       X2[5] = (float)(offsetAboveX + x2);
   Y1[5] = (float)(offsetAboveY + y1);       Y2[5] = (float)(offsetAboveY + y2);
   X1[6] = (float)(offsetBelowX + x1);       X2[6] = (float)(offsetBelowX + x2);
   Y1[6] = (float)(offsetBelowY + y1);       Y2[6] = (float)(offsetBelowY + y2);

   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex;

   tempVertex.Tex0 = D3DXVECTOR3( X1[0], Y1[0], z );
   tempVertex.Tex1 = D3DXVECTOR4( X1[1], Y1[1],
                                  X1[2], Y1[2] );
   tempVertex.Tex2 = D3DXVECTOR4( X1[3], Y1[3],
                                  X1[4], Y1[4] );
   tempVertex.Tex3 = D3DXVECTOR4( X1[5], Y1[5],
                                  X1[6], Y1[6] );
   tempVertex.Tex7 = D3DXVECTOR3( x1, y1, z );
   tempVertex.Position  = D3DXVECTOR2( px1, py1 );

   (*vertices)[index++] = tempVertex;

   tempVertex.Tex0 = D3DXVECTOR3( X2[0], Y2[0], z );
   tempVertex.Tex1 = D3DXVECTOR4( X2[1], Y2[1],
                                  X2[2], Y2[2] );
   tempVertex.Tex2 = D3DXVECTOR4( X2[3], Y2[3],
                                  X2[4], Y2[4] );
   tempVertex.Tex3 = D3DXVECTOR4( X2[5], Y2[5],
                                  X2[6], Y2[6] );
   tempVertex.Tex7 = D3DXVECTOR3( x2, y2, z );
   tempVertex.Position  = D3DXVECTOR2( px2, py2 );

   (*vertices)[index++] = tempVertex;
}

void Grid :: createPoint( float x, float y, int z,
                          float texXOffset,
                          float texYOffset,
                          int   texZOffset, VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index )
{
  
	int column,      row;
   int columnAbove, rowAbove;
   int columnBelow, rowBelow;
   float offsetX, offsetY;
   float offsetAboveX, offsetAboveY;
   float offsetBelowX, offsetBelowY;
   float X[7], Y[7];
   int px, py;
   
   //compute geometry coordinates
   column      = z % cols;
   row         = (int)floorf((float)(z/cols));
   offsetX = (float)(column * dim[0]);
   offsetY = (float)(row    * dim[1]);
   px = offsetX + x;
   py = offsetY + y;
   
   //compute slice offset
   x += texXOffset;
   y += texYOffset;
   z += texZOffset;

   column = z % cols;
   row    = (int)floorf((float)(z/cols));

   offsetX = column * dim[0];
   offsetY = row    * dim[1];

   columnAbove = (z+1) % cols;
   rowAbove    = (int)floorf((float)((z+1)/cols));
   offsetAboveX = columnAbove * dim[0];
   offsetAboveY = rowAbove    * dim[1];

   columnBelow = (z-1) % cols;
   rowBelow    = (int)floorf((float)((z-1)/cols));
   offsetBelowX = columnBelow * dim[0];
   offsetBelowY = rowBelow    * dim[1];

   X[0] = (float)(offsetX + x);
   Y[0] = (float)(offsetY + y);
   X[1] = (float)(X[0] - 1);
   Y[1] = (float)(Y[0]);
   X[2] = (float)(X[0] + 1);
   Y[2] = (float)(Y[0]);
   X[3] = (float)(X[0]);
   Y[3] = (float)(Y[0] - 1);
   X[4] = (float)(X[0]);
   Y[4] = (float)(Y[0] + 1);
   X[5] = (float)(offsetAboveX + x);
   Y[5] = (float)(offsetAboveY + y);
   X[6] = (float)(offsetBelowX + x);
   Y[6] = (float)(offsetBelowY + y);

   VS_FLUID_SIMULATION_INPUT_STRUCT tempVertex;

   tempVertex.Tex0 = D3DXVECTOR3( X[0], Y[0], z );
   tempVertex.Tex1 = D3DXVECTOR4( X[1], Y[1],
                                  X[2], Y[2] );
   tempVertex.Tex2 = D3DXVECTOR4( X[3], Y[3],
                                  X[4], Y[4] );
   tempVertex.Tex3 = D3DXVECTOR4( X[5], Y[5],
                                  X[6], Y[6] );
   tempVertex.Tex7 = D3DXVECTOR3( x, y, z );
   tempVertex.Position  = D3DXVECTOR2( px, py );

   (*vertices)[index++] = tempVertex;
}

void Grid :: createBoundaryQuads( VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index )
{
  createSlice( 0, 1, vertices, index );
  createSlice( dim[2]-1, -1, vertices, index );

}

void Grid :: createBoundaryLines( VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index )
{
  //draw boundaries for each slice

  int w = dim[0];
  int h = dim[1];
  int d = dim[2];

  float oYb = 0.5f; 
  float oYt = -1.5f;
  float oXl = 0.5f;
  float oXr = -1.5f;

#ifdef G80_RASTER_RULES
  for( int z = 1; z < dim[2]-1; z++ )  //sarah: changed from 0 to 1, and from dim[2] to dim[2]-1
  {
     //bottom
     createLine( 0, 1,
               w, 1, z,
               //0.0f, 0.5f, 0, vertices, index );
			   0.0f, oYb, 0, vertices, index );
     
     //top
     createLine( 0, h,
               w, h, z,
               //0.0f, -1.5f, 0, vertices, index );
			   0.0f, oYt, 0, vertices, index );

     //left
     createLine( 1, 0,
               1, h, z,
               //0.5f, 0.0f, 0, vertices, index );
			   oXl, 0.0f, 0, vertices, index );
     
     //right
     createLine( w, 0,
               w, h, z,
               //-1.5f, 0.0f, 0, vertices, index );
			   oXr, 0.0f, 0, vertices, index );
  }
#else
  for( int z = 0; z < dim[2]; z++ )
  {
     //bottom
     createLine( 0, 1,
               w, 1, z,
               0.0f, oYb, 0, vertices, index );
     
     //top
     createLine( 0, h,
               w, h, z,
               0.0f, oYt, 0, vertices, index );

     //left
     createLine( 1, 0,
               1, h, z,
               oXl, 0.0f, 0, vertices, index );
     
     //right
     createLine( w, 0,
               w, h, z,
               oXr, 0.0f, 0, vertices, index );
  }
#endif
  
  //need to handle first and last slice differently (copy diagonally)
  
  //FIRST SLICE ------------------------------------------------------
#ifdef G80_RASTER_RULES
  //bottom
  createLine( 0, 1,
            w, 1, 0,
            0.0f, oYb, 1, vertices, index );
  
  //top
  createLine( 0, h,
            w, h, 0,
            0.0f, oYt, 1, vertices, index );

  //left
  createLine( 1, 0,
            1, h, 0,
            oXl, 0.0f, 1, vertices, index );
  
  //right
  createLine( w, 0,
            w, h, 0,
            oXr, 0.0f, 1, vertices, index );
#else
  //bottom
  createLine( 0, 1,
            w, 1, 0,
            0.0f, oYb, 1, vertices, index );
  
  //top
  createLine( 0, h,
            w, h, 0,
            0.0f, oYt, 1, vertices, index );

  //left
  createLine( 1, 0,
            1, h, 0,
            oXl, 0.0f, 1, vertices, index );
  
  //right
  createLine( w, 0,
            w, h, 0,
            oXr, 0.0f, 1, vertices, index );
#endif
  
#ifdef G80_RASTER_RULES
  //LAST SLICE -------------------------------------------------------
  //bottom
  createLine( 0, 1,
            w, 1, d-1,
            0.0f, oYb, -1, vertices, index );
  
  //top
  createLine( 0, h,
            w, h, d-1,
            0.0f, oYt, -1, vertices, index );

  //left
  createLine( 1, 0,
            1, h, d-1,
            oXl, 0.0f, -1, vertices, index );
  
  //right
  createLine( w, 0,
            w, h, d-1,
            oXr, 0.0f, -1, vertices, index );
#else
  //LAST SLICE -------------------------------------------------------
  //bottom
  createLine( 0, 1,
            w, 1, d-1,
            0.0f, oYb, -1, vertices, index );
  
  //top
  createLine( 0, h,
            w, h, d-1,
            0.0f, oYt, -1, vertices, index );

  //left
  createLine( 1, 0,
            1, h, d-1,
            oXl, 0.0f, -1, vertices, index );
  
  //right
  createLine( w, 0,
            w, h, d-1,
            oXr, 0.0f, -1, vertices, index );
#endif

}


void Grid :: createBoundaryPoints(  VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index  )
{

   //copy values in each slice corner
   int w = dim[0];
   int h = dim[1];
   int d = dim[2];

   //XXX go from 1 to d-1, and change this in drawLines() too
#ifdef G80_RASTER_RULES
   for( int z = 1; z < d-1; z++ ) //sarah: changed from 0 to 1 and d to d-1
   {
      createPoint( 1,   1,   z,
                  1.5f - 1,  1.5f - 1, 0, vertices, index  );

      createPoint( w, 1,   z,
                 -0.5f - 1,  1.5f - 1, 0, vertices, index  );

      createPoint( 1,   h, z,
                  1.5f - 1, -0.5f - 1 , 0, vertices, index  );

      createPoint( w, h, z,
                 -0.5f - 1, -0.5f - 1, 0, vertices, index  );
   }
#else
   for( int z = 0; z < d; z++ )
   {
      createPoint( 0,   1, z,
                  1.5f,  0.5f, 0, vertices, index  );

      createPoint( w-1, 1, z,
                 -0.5f,  0.5f, 0, vertices, index  );

      createPoint( 0,   h, z,
                  1.5f, -1.5f, 0, vertices, index  );

      createPoint( w-1, h, z,
                 -0.5f, -1.5f, 0, vertices, index  );
   }
#endif
   
   //need to handle first and last slice differently (copy diagonally)
   
   //FIRST SLICE -------------------------------------------------
#ifdef G80_RASTER_RULES
   createPoint( 1,   1, 0,
               1.5f - 1,  1.5f - 1, 1, vertices, index  );

   createPoint( w, 1, 0,
              -0.5f -1 ,  1.5f - 1, 1, vertices, index  );

   createPoint( 1,   h, 0,
               1.5f -1, -0.5f - 1, 1, vertices, index  );

   createPoint( w, h, 0,
              -0.5f -1, -0.5f - 1, 1, vertices, index  );
#else
   createPoint( 0,   1, 0,
               1.5f,  0.5f, 1, vertices, index  );

   createPoint( w-1, 1, 0,
              -0.5f,  0.5f, 1, vertices, index  );

   createPoint( 0,   h, 0,
               1.5f, -1.5f, 1, vertices, index  );

   createPoint( w-1, h, 0,
              -0.5f, -1.5f, 1, vertices, index  );
#endif
   
   //LAST SLICE --------------------------------------------------
#ifdef G80_RASTER_RULES
   createPoint( 1,   1, d-1,
               1.5f -1,  1.5f - 1, -1, vertices, index  );

   createPoint( w, 1, d-1,
              -0.5f -1,  1.5f - 1, -1, vertices, index  );

   createPoint( 1,   h, d-1,
               1.5f -1, -0.5f - 1, -1, vertices, index  );

   createPoint( w, h, d-1,
              -0.5f -1, -0.5f - 1, -1, vertices, index  );
#else
   createPoint( 0,   1, d-1,
               1.5f,  0.5f, -1, vertices, index  );

   createPoint( w-1, 1, d-1,
              -0.5f,  0.5f, -1, vertices, index  );

   createPoint( 0,   h, d-1,
               1.5f, -1.5f, -1, vertices, index  );

   createPoint( w-1, h, d-1,
              -0.5f, -1.5f, -1, vertices, index  );
#endif
   

}

void Grid :: drawForRender( D3D_EFFECT* pEffect, UINT iPassIndex )
{
  //sarah: change this to kevin's description
  UINT stride =  sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT) ;
  UINT offset =  0 ;
  //set the vertex buffer
  
  dxC_SetVertexDeclaration( VERTEX_DECL_FLUID_SIMULATION_INPUT, pEffect);
  dxC_SetStreamSource( renderQuadBufferDesc, offset, pEffect );


  //draw the primitives
  dxC_DrawPrimitive( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, numVerticesRenderQuad/3, pEffect, METRICS_GROUP_UNKNOWN, iPassIndex );

}

void Grid :: drawSlice( D3D_EFFECT* pEffect, UINT iPassIndex, int sliceNumber, int numSlices)
{
   if( sliceNumber<1 || sliceNumber>=dim[2]-1 )
	   return;

  //sarah: change this to kevin's description
  UINT stride =  sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT) ;
  UINT offset =  0 ;
  //set the vertex buffer
  
  dxC_SetVertexDeclaration( VERTEX_DECL_FLUID_SIMULATION_INPUT, pEffect);
  dxC_SetStreamSource( slicesBufferDesc, offset, pEffect );
  //draw the primitives
  dxC_DrawPrimitive( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, (sliceNumber-1)*6 , 2*numSlices, pEffect, METRICS_GROUP_UNKNOWN, iPassIndex );
}

void Grid :: drawSlices( D3D_EFFECT* pEffect, UINT iPassIndex )
{
  //sarah: change this to kevin's description
  UINT stride =  sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT) ;
  UINT offset =  0 ;
  //set the vertex buffer
  
  dxC_SetVertexDeclaration( VERTEX_DECL_FLUID_SIMULATION_INPUT, pEffect);
  dxC_SetStreamSource(slicesBufferDesc, offset, pEffect );
  //draw the primitives
  dxC_DrawPrimitive( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, numVerticesSlices/3, pEffect, METRICS_GROUP_UNKNOWN, iPassIndex );
}

void Grid :: drawBoundaries(D3D_EFFECT* pEffect, UINT iPassIndex, bool forceDraw )
{
   if( forceDraw )
   {

	  UINT stride =  sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT) ;
	  UINT offset =  0 ;
	  //set the vertex buffer
	  
	  dxC_SetVertexDeclaration( VERTEX_DECL_FLUID_SIMULATION_INPUT, pEffect);
	  dxC_SetStreamSource(boundarySlicesBufferDesc, offset, pEffect );
	  //draw the primitives
	  dxC_DrawPrimitive( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 2, pEffect, METRICS_GROUP_UNKNOWN, iPassIndex );
	  
/* only drawing lower face
	  //dxC_DrawPrimitive( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, numVerticesBoundarySlices/3, pEffect, METRICS_GROUP_UNKNOWN, iPassIndex );


	  dxC_SetVertexDeclaration( VERTEX_DECL_FLUID_SIMULATION_INPUT, pEffect);
	  dxC_SetStreamSource(boundaryLinesBufferDesc, offset, pEffect );
	  //draw the primitives
	  dxC_DrawPrimitive( D3D10_PRIMITIVE_TOPOLOGY_LINELIST,  0, numVerticesBoundaryLines/2, pEffect, METRICS_GROUP_UNKNOWN, iPassIndex   );
	  
	  dxC_SetVertexDeclaration( VERTEX_DECL_FLUID_SIMULATION_INPUT, pEffect);
	  dxC_SetStreamSource(boundaryPointsBufferDesc, offset, pEffect );
	  dxC_DrawPrimitive( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST, 0, numVerticesBoundaryPoints, pEffect, METRICS_GROUP_UNKNOWN, iPassIndex );
*/
	}
}



void Grid :: createSliceQuads(VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index )
{
  for( int z = 1; z < dim[2]-1; z++ )
  {
     createSlice( z, 0, vertices, index );
  }
}


void Grid :: createVertexBuffers( )
{

	//vertices for render quad ----------------------------

   numVerticesRenderQuad = 6;
   renderQuad = MALLOC_NEWARRAY( NULL, VS_FLUID_SIMULATION_INPUT_STRUCT, numVerticesRenderQuad );
   int index = 0;
   createRenderQuad(&renderQuad,index);

   dxC_VertexBufferDefinitionInit( renderQuadBufferDesc );
   renderQuadBufferDesc.dwFlags = 0x00000000;
   renderQuadBufferDesc.eVertexType = VERTEX_DECL_FLUID_SIMULATION_INPUT;
   renderQuadBufferDesc.nBufferSize[ 0 ] = sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT)*numVerticesRenderQuad;
   //renderQuadBufferDesc.nD3DBufferID
   renderQuadBufferDesc.nVertexCount = numVerticesRenderQuad;
   //renderQuadBufferDesc.pLocalData
   renderQuadBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;

   V( dxC_CreateVertexBuffer(0, renderQuadBufferDesc, renderQuad ) );


	// vertex buffer for slice quads ----------------------
   if( slices )
   {
       //sarah : delete
   }
    
   numVerticesSlices = 6*(dim[2]-2);
   slices = MALLOC_NEWARRAY( NULL, VS_FLUID_SIMULATION_INPUT_STRUCT, numVerticesSlices );
   index = 0;
   createSliceQuads(&slices,index);
   
   dxC_VertexBufferDefinitionInit( slicesBufferDesc );
   slicesBufferDesc.dwFlags = 0x00000000;
   slicesBufferDesc.eVertexType = VERTEX_DECL_FLUID_SIMULATION_INPUT;
   slicesBufferDesc.nBufferSize[ 0 ] = sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT)*numVerticesSlices;
   //slicesBufferDesc.nD3DBufferID = ;
   slicesBufferDesc.nVertexCount = numVerticesSlices;
   slicesBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;

   V( dxC_CreateVertexBuffer(0, slicesBufferDesc, slices ) );


   // vertex buffers for boundary geometry ------------------
   if( boundarySlices )
   {
       //sarah : delete
   }
   if( boundaryLines )
   {
       //sarah : delete
   }
   if( boundaryPoints )
   {
       //sarah : delete
   }
   
   numVerticesBoundarySlices = 2*6;
   boundarySlices = MALLOC_NEWARRAY( NULL, VS_FLUID_SIMULATION_INPUT_STRUCT, numVerticesBoundarySlices );
   index = 0;
   createBoundaryQuads(&boundarySlices,index);

   dxC_VertexBufferDefinitionInit( boundarySlicesBufferDesc );
   boundarySlicesBufferDesc.dwFlags = 0x00000000;
   boundarySlicesBufferDesc.eVertexType = VERTEX_DECL_FLUID_SIMULATION_INPUT;
   boundarySlicesBufferDesc.nBufferSize[ 0 ] = sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT)*numVerticesBoundarySlices;
   //boundarySlicesBufferDesc.nD3DBufferID = ;
   boundarySlicesBufferDesc.nVertexCount = numVerticesBoundarySlices;
   boundarySlicesBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;

   V( dxC_CreateVertexBuffer(0, boundarySlicesBufferDesc, boundarySlices ) );

   numVerticesBoundaryLines = 4*2*(dim[2]+2);
   boundaryLines = MALLOC_NEWARRAY( NULL, VS_FLUID_SIMULATION_INPUT_STRUCT, numVerticesBoundaryLines );
   index = 0;
   createBoundaryLines(&boundaryLines,index);

   dxC_VertexBufferDefinitionInit( boundaryLinesBufferDesc );
   boundaryLinesBufferDesc.dwFlags = 0x00000000;
   boundaryLinesBufferDesc.eVertexType = VERTEX_DECL_FLUID_SIMULATION_INPUT;
   boundaryLinesBufferDesc.nBufferSize[ 0 ] = sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT)*numVerticesBoundaryLines;
   //boundaryLinesBufferDesc.nD3DBufferID = ;
   boundaryLinesBufferDesc.nVertexCount = numVerticesBoundaryLines;
   boundaryLinesBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;

   V( dxC_CreateVertexBuffer(0, boundaryLinesBufferDesc, boundaryLines ) );

   numVerticesBoundaryPoints = 4*1*(dim[2]+2);
   boundaryPoints = MALLOC_NEWARRAY( NULL, VS_FLUID_SIMULATION_INPUT_STRUCT, numVerticesBoundaryPoints );
   index = 0;
   createBoundaryPoints(&boundaryPoints,index);

   dxC_VertexBufferDefinitionInit( boundaryPointsBufferDesc );
   boundaryPointsBufferDesc.dwFlags = 0x00000000;
   boundaryPointsBufferDesc.eVertexType = VERTEX_DECL_FLUID_SIMULATION_INPUT;
   boundaryPointsBufferDesc.nBufferSize[ 0 ] = sizeof(VS_FLUID_SIMULATION_INPUT_STRUCT)*numVerticesBoundaryPoints;
   //boundaryPointsBufferDesc.nD3DBufferID = ;
   boundaryPointsBufferDesc.nVertexCount = numVerticesBoundaryPoints;
   boundaryPointsBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;

   V( dxC_CreateVertexBuffer(0, boundaryPointsBufferDesc, boundaryPoints ) );

}





//---------------------------------------------------------------------------
//CFluid
//---------------------------------------------------------------------------
void CFluid :: Initialize( 
	int width, 
	int height, 
	int depth,
	int nDensityTextureId,
	int nVelocityTextureId,
	int nObstructorTextureId,
	int densityTextureIndex, 
	int velocityTextureIndex, 
	int inputObstructorTextureIndex,
	float renderScale,
	D3DXMATRIX* psysWorld,
	//VECTOR vPosition,
	D3D_EFFECT* pEffect, 
	EFFECT_TECHNIQUE* pTechnique )
{


	grid = MALLOC_NEW( NULL, Grid );
	grid->Initialize( width, height, depth );

    lastPositionInitialized = false;

   //create the render targets
    float FLUID_RES_W = grid->texDim[0];
	float FLUID_RES_H = grid->texDim[1];
	DXC_FORMAT VectorFormat = D3DFMT_A16B16G16R16F;
    DXC_FORMAT ScalarFormat = D3DFMT_R16F;

    //scalars
	RENDER_TARGET_FLUID_COLOR0 =    RENDER_TARGET_INDEX(dxC_AddRenderTargetDesc(FALSE,-1,-1,FLUID_RES_W,FLUID_RES_H,D3DC_USAGE_2DTEX_RT,ScalarFormat, -1,FALSE, 1 ));
   	RENDER_TARGET_FLUID_COLOR1 =    RENDER_TARGET_INDEX(dxC_AddRenderTargetDesc(FALSE,-1,-1,FLUID_RES_W,FLUID_RES_H,D3DC_USAGE_2DTEX_RT,ScalarFormat, -1,FALSE, 1 ));
   	RENDER_TARGET_FLUID_PRESSURE =  RENDER_TARGET_INDEX(dxC_AddRenderTargetDesc(FALSE,-1,-1,FLUID_RES_W,FLUID_RES_H,D3DC_USAGE_2DTEX_RT,ScalarFormat, -1,FALSE, 1 ));
	RENDER_TARGET_FLUID_OBSTACLES = RENDER_TARGET_INDEX(dxC_AddRenderTargetDesc(FALSE,-1,-1,FLUID_RES_W,FLUID_RES_H,D3DC_USAGE_2DTEX_RT,ScalarFormat, -1,FALSE, 1 )); 

    //vectors
    RENDER_TARGET_FLUID_VELOCITY =  RENDER_TARGET_INDEX(dxC_AddRenderTargetDesc(FALSE,-1,-1,FLUID_RES_W,FLUID_RES_H,D3DC_USAGE_2DTEX_RT,VectorFormat, -1,FALSE, 1 ));


    V( dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_WIDTH, largestRTWidth) ); 
	V( dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_HEIGHT, largestRTHeight) );
	V( dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_WZO,grid->dim[2] ) );

	nGridDensityTextureIndex = densityTextureIndex;
	nGridVelocityTextureIndex = velocityTextureIndex;

	float VolDim[4] = {grid->dim[0], grid->dim[1], grid->dim[2], 0 };
	V( dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_VOLUME_DIM, VolDim , 0, sizeof(D3DXVECTOR4) ) ); 


	V( dx9_EffectSetInt( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_NUMCOLS, grid->cols ) );


	ClearRenderTarget( RENDER_TARGET_FLUID_PRESSURE, 0.0f, 0.0f, 0.0f, 1.0f ); //initial guess of 0 pressure
	ClearRenderTarget( RENDER_TARGET_FLUID_VELOCITY, 0,0,0,0 );
	ClearRenderTarget( RENDER_TARGET_FLUID_COLOR1, 0,0,0,0 );
	ClearRenderTarget( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMPVELOCITY ), 0,0,0,0 );
	ClearRenderTarget( RENDER_TARGET_FLUID_OBSTACLES, 0,0,0,0 );
    ClearRenderTarget( RENDER_TARGET_FLUID_COLOR0, 0,0,0,0 );

	ClearRenderTarget( dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FLUID_BOXDEPTH ), 0,0,0,0 );
	ClearRenderTarget( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP1 ), 0,0,0,0 );
	ClearRenderTarget( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP2 ), 0,0,0,0 );


	SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_VELOCITY    ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_VELOCITY);
	SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_COLOR1      ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_COLOR);
	SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_PRESSURE    ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_PRESSURE);
	SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_OBSTACLES   ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_OBSTACLES);

	densityPatternTextureId = nDensityTextureId;
	velocityPatternTextureId = nVelocityTextureId;

	//char szFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
	//PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%stextures\\%s", FILE_PATH_PARTICLE, densityTextureName );
	//V( e_TextureNewFromFile( &densityPatternTexture, szFile, TEXTURE_GROUP_NONE, TEXTURE_NONE,0,DXGI_FORMAT_R8G8B8A8_UNORM) );

	//PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%stextures\\%s", FILE_PATH_PARTICLE, velocityTextureName );
	//V( e_TextureNewFromFile( &velocityPatternTexture, szFile, TEXTURE_GROUP_NONE, TEXTURE_NONE,0,DXGI_FORMAT_R8G8B8A8_UNORM) );

	firstUpdate = true;

	saturation = 0.28f;

	ColorTextureNumber = 0;

	addDensity = false;

    mustRenderObstacles = true;

	numGPUs = 1;
#ifndef PROFILE
	NVAPICheck();
#endif

	static int globalFrameCounter = 0;
	frame = globalFrameCounter;
	globalFrameCounter = (globalFrameCounter+1)%numGPUs;

	createOffsetTable( pEffect, pTechnique );

	setFluidType( );

	impulseX = (float)(grid->dim[0] / 2);
	impulseY = (float)(grid->dim[1] / 2);
	impulseZ =  0;
	impulseDx = 0;
	impulseDy = 0;
	impulseDz = 0.6;



#ifdef USE_OBSTACLE_TEXTURE
    
	/* sarah: this part is not working right now, have to find the right render target width and height to set
              or maybe the stride or something else is messed up. In either case this stuff is not being used right now
			  so we'll fix it if/when we need it
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_WIDTH, 0);
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_HEIGHT, 0 );
    SetRenderTargetAndViewport( RENDER_TARGET_FLUID_OBSTACLES , grid->texDim[0],grid->texDim[1] );
	V( dxC_EffectBeginPass(pEffect,passIndexDrawWhite) );
	grid->drawBoundaries(pEffect, passIndexDrawWhite, true);
    */

	obstructorTextureIndex = inputObstructorTextureIndex;
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MAX_OBSTACLE_HEIGHT, obstructorTextureIndex );

	obstacleTextureId = nObstructorTextureId;

	//PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%stextures\\%s", FILE_PATH_PARTICLE, obstructorTextureName );
	//V( e_TextureNewFromFile( &obstacleTextureId, szFile, TEXTURE_GROUP_NONE, TEXTURE_NONE, 0, DXGI_FORMAT_R8_UNORM ) );
#else
	mustUpdateObstacles = false;
#endif


	renderer = MALLOC_NEW( NULL, VolumeRenderer );

	renderer->Initialize( grid->dim[0], grid->dim[1], grid->dim[2],
						  renderScale, psysWorld, 
						  pEffect, pTechnique );

	renderer->timeIncrement = 0.02;
	renderer->sizeMultiplier = 1;

}



void CFluid :: setFluidType(  )
{
   //size in shader. impulseSize = 0.14f; 
#ifdef OFFLINE
   renderW = renderH = 480;
#else
   renderW = renderH = grid->maxDim * 3;
#endif
}

CFluid :: ~CFluid( void )
{
  delete[] texels;
  textureRviewOffset = NULL;
  textureOffsetTable = NULL;
  FREE_DELETE( NULL, grid, Grid );
  FREE_DELETE( NULL, renderer, VolumeRenderer );

}

void CFluid :: GetCenterAndRadius(VECTOR &center, float &radius)
{
	center.x = renderer->World._41;
	center.y = renderer->World._42;
    center.z = renderer->World._43;
/*
	  D3DXVECTOR4 centerVec = D3DXVECTOR4( 0, 0, 0, 0);
	  D3DXVECTOR4 centerInWorld;
	  D3DXVECTOR4 radiusVecInGrid;
	  D3DXVec4Transform(&centerInWorld, &centerVec, &renderer->World);
	  center.x = centerInWorld.x;
      center.y = centerInWorld.y;
	  center.z = centerInWorld.z;
 */
	  float side = renderer->mRenderScale * 2;
	  radius = sqrtf( side*side*2 )/2.0; 

}

void CFluid :: update( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, VECTOR focusUnitPosition, float focusUnitRadius, float focusUnitHeight, VECTOR windDir, float windAmount, float windInfluence, float timestep, float velocityMultiplier, float velocityClamp, float sizeMultiplier, bool useBFECC, VECTOR* monsterPositions, VECTOR* monsterHeightAndRadiusAndSpeed, VECTOR* monsterDirection )
{
    if( frame > 0)
	{
     	frame = (frame+1)%numGPUs;
		return;
	}
	frame = (frame+1)%numGPUs;

	//set all the variables in the shader----------------------------------------------------------------

    if(firstUpdate)
	{
		D3D_TEXTURE * pVelocityTexture = dxC_TextureGet( velocityPatternTextureId );
    	if( pVelocityTexture == NULL) return;
   	    if( ! e_TextureIsLoaded(pVelocityTexture->nId) ) return;
	    V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_VELOCITYPATTERN, pVelocityTexture->GetShaderResourceView() ) );
	   
	    D3D_TEXTURE * pDensityTexture = dxC_TextureGet( densityPatternTextureId );
	    if( pDensityTexture == NULL ) return;
	    if( ! e_TextureIsLoaded(pDensityTexture->nId) ) return;
	    V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_DENSITYPATTERN, pDensityTexture->GetShaderResourceView() ) );

     	D3D_TEXTURE * pObstacleTexture = dxC_TextureGet( obstacleTextureId );
	    if( pObstacleTexture == NULL ) return;
    	if( ! e_TextureIsLoaded(pObstacleTexture->nId) ) return;
        V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_OBSTACLEPATTERN, pObstacleTexture->GetShaderResourceView() ) );

       
        firstUpdate = false; 
	}

    if(useBFECC)
		decay = 0.93;
	else
		decay = 0.97;



	V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_ZOFFSET, textureRviewOffset ) );

	windModifier = windInfluence;

    V( dx9_EffectSetFloat( pEffect,*pTechnique,EFFECT_PARAM_SMOKE_TIMESTEP,timestep ) );

	SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_VELOCITY									,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_VELOCITY);
	SetRTTextureAsShaderResource(dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP1 )		,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_TEMP1);
	SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_PRESSURE									,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_PRESSURE);
	SetRTTextureAsShaderResource(dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP2 )		,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_TEMP3);
	SetRTTextureAsShaderResource(dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMPVELOCITY )	,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_TEMPV);
	SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_OBSTACLES									,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_OBSTACLES);


   if(ColorTextureNumber == 0)
    SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_COLOR1      ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_COLOR);
   else
	SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_COLOR0      ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_COLOR);   

    
   ClearRenderTarget( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMPVELOCITY ), 0,0,0,0 );


   V( dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_WIDTH, largestRTWidth) ); 
   V( dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_HEIGHT, largestRTHeight) ); 
   dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_WZO,grid->dim[2] );
   float VolDim[4] = {grid->dim[0], grid->dim[1], grid->dim[2], 0 };
   dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_VOLUME_DIM, VolDim , 0, sizeof(D3DXVECTOR4) );  
   dx9_EffectSetInt( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_NUMCOLS, grid->cols );

    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_WIDTH, grid->texDim[0] );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_HEIGHT, grid->texDim[1] );

    //disable culling
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    //disable depth and stencil test
  	dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
    dxC_SetRenderState( D3DRS_STENCILENABLE, FALSE);

  //--------------------------------------------------------------------

  //transform the position, radius and height to the grid dimentions
  //set the variables in the shader

  D3DXMATRIX translateOffset;
  D3DXMatrixTranslation(&translateOffset, -0.5, -0.5, -0.5);
  D3DXMATRIX nonuniformScale;
  D3DXMatrixScaling(&nonuniformScale, 
	  (float)grid->dim[0] / grid->maxDim, 
	  (float)grid->dim[1] / grid->maxDim, 
	  (float)grid->dim[2] / grid->maxDim);

  D3DXMATRIX Scale;
  D3DXMatrixScaling(&Scale, grid->dim[0], grid->dim[1], grid->dim[2]);

  D3DXMATRIX world = translateOffset * nonuniformScale * renderer->World;
  
  D3DXMATRIX worldInv;
  D3DXMatrixInverse(&worldInv, 0, &world);
  
  D3DXVECTOR4 windDirV = D3DXVECTOR4(windDir.x,windDir.y,windDir.z,0);
  if(D3DXVec4Length(&windDirV)<0.0001)
  {  
      windAmount *= D3DXVec4Length(&windDirV);
	  windDirV =  D3DXVECTOR4(-1,0,0,0);      
  }

  D3DXVECTOR4 windDirInTexture;
  D3DXVECTOR4 windDirInGrid;
  D3DXVec4Transform(&windDirInTexture, &windDirV, &worldInv);
  D3DXVec4Transform(&windDirInGrid, &windDirInTexture, &Scale);
  D3DXVec4Normalize(&windDirInGrid, &windDirInGrid);
 
#define MAX_SENDIND_UNITS 20

  D3DXVECTOR4 FocusUnitPositionsInGrid[MAX_SENDIND_UNITS];
  D3DXVECTOR4 FocusUnitHeightAndRadiusInGrid[MAX_SENDIND_UNITS];
  D3DXVECTOR4 focusUnitVelocities[MAX_SENDIND_UNITS];
  int numFocusUnits = 0;
  
  VECTOR focusUnitDirection = 0;
  float focusUnitSpeed = 100;


  //loop over all characters
  for(int i=0;i<MAX_INTERACTING_UNITS;i++)
  if(numFocusUnits<MAX_SENDIND_UNITS)  
	  addFocusUnits( monsterPositions[i],monsterHeightAndRadiusAndSpeed[i].y,monsterHeightAndRadiusAndSpeed[i].x,monsterDirection[i],monsterHeightAndRadiusAndSpeed[i].z,velocityMultiplier,velocityClamp,sizeMultiplier,worldInv,Scale,
	     			 FocusUnitPositionsInGrid, FocusUnitHeightAndRadiusInGrid, focusUnitVelocities, numFocusUnits );
    	  
  float maxHeight = 0;
  for(int i=0;i<numFocusUnits;i++)
	  if(FocusUnitHeightAndRadiusInGrid[i].x > maxHeight)
		  maxHeight = FocusUnitHeightAndRadiusInGrid[i].x;


  //set the centers, radii, height, velocity etc to shader
  dx9_EffectSetVectorArray ( pEffect, *pTechnique, FOCUS_UNIT_POSITIONS_ARRAY, FocusUnitPositionsInGrid, MAX_SENDIND_UNITS );
  dx9_EffectSetVectorArray ( pEffect, *pTechnique, FOCUS_UNIT_HEIGHT_AND_RADIUS_ARRAY, FocusUnitHeightAndRadiusInGrid, MAX_SENDIND_UNITS );
  dx9_EffectSetVectorArray ( pEffect, *pTechnique, FOCUS_UNIT_VELOCITIES_ARRAY, focusUnitVelocities, MAX_SENDIND_UNITS );
  dx9_EffectSetInt(  pEffect, *pTechnique, FOCUS_UNIT_NUM, numFocusUnits );


  /*
  D3DXVECTOR4 focusPos = D3DXVECTOR4(focusUnitPosition.x, focusUnitPosition.y, focusUnitPosition.z, 1.0);
  D3DXVECTOR4 focusPosInTexture;
  D3DXVECTOR4 focusPosInGrid;
  D3DXVec4Transform(&focusPosInTexture, &focusPos, &worldInv);
  D3DXVec4Transform(&focusPosInGrid, &focusPosInTexture, &Scale);

  D3DXVECTOR4 radiusVec = D3DXVECTOR4( focusUnitRadius, 0, 0, 0);
  D3DXVECTOR4 radiusVecInTexture;
  D3DXVECTOR4 radiusVecInGrid;
  D3DXVec4Transform(&radiusVecInTexture, &radiusVec, &worldInv);
  D3DXVec4Transform(&radiusVecInGrid, &radiusVecInTexture, &Scale);
  radiusVecInGrid.w = 0.0;

  D3DXVECTOR4 heightVec = D3DXVECTOR4( 0, 0, focusUnitHeight, 0);
  D3DXVECTOR4 heightVecInTexture;
  D3DXVECTOR4 heightVecInGrid;
  D3DXVec4Transform(&heightVecInTexture, &heightVec, &worldInv);
  D3DXVec4Transform(&heightVecInGrid, &heightVecInTexture, &Scale);
  heightVecInGrid.w = 0.0;

  float gridSpaceObstructorPosition[4] = {focusPosInGrid.x, (grid->dim[1]-focusPosInGrid.y), focusPosInGrid.z, 1.0};
  

  float gridSpaceObstructorRadius = D3DXVec4Length(&radiusVecInGrid); 
  float gridSpaceObstructorHeight = D3DXVec4Length(&heightVecInGrid);

  dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_IMPULSE_CENTER, gridSpaceObstructorPosition , 0, sizeof(D3DXVECTOR4) );  
  dx9_EffectSetFloat( pEffect,*pTechnique,EFFECT_PARAM_SMOKE_IMPULSE_SIZE,gridSpaceObstructorRadius );
  dx9_EffectSetFloat( pEffect,*pTechnique,EFFECT_PARAM_SMOKE_IMPULSE_HEIGHT,gridSpaceObstructorHeight );
  */
  //--------------------------------------------------------------------

  SetAlphaDisable();

  if(mustRenderObstacles)
  {
	  //no blending
	  renderObstacles(pEffect, obstructorTextureIndex);
      mustRenderObstacles = false;  
  }

  //no blending for advecting

  if( useBFECC )
      advectColorBFECC( pEffect, pTechnique );
  else
      advectColor( pEffect, pTechnique );
    
  advectVelocity( pEffect, pTechnique );

  //set AdditiveBlending_Sim for confinement
  applyVorticityConfinement( pEffect, pTechnique );


#ifdef USE_GAUSSIAN_BLOCKER
  //set AlphaBlending_SimAddForces fot applyBlocker
  if(numFocusUnits>0)
      applyBlocker( pEffect, pTechnique, maxHeight);
#endif

  //set AlphaBlending_Sim for hellgate wind
  applyExternalForces( pEffect, pTechnique, windDirInGrid, windAmount );

  //NoBlending
  SetAlphaDisable();
  computeVelocityDivergence( pEffect );
  
  //NoBlending
  computePressure( pEffect );
  
  //NoBlending
  projectVelocity( pEffect, pTechnique );                                                         
                              

  ColorTextureNumber = 1 - ColorTextureNumber;

}


void CFluid :: renderObstacles( D3D_EFFECT* pEffect, int nGridObstructorTextureIndex )
{
   ClearRenderTarget( RENDER_TARGET_FLUID_OBSTACLES, 0,0,0,0 );
   SetRenderTargetAndViewport( RENDER_TARGET_FLUID_OBSTACLES, grid->texDim[0],grid->texDim[1]  );
   //temp sarah dxC_EffectBeginPass(pEffect,passIndexDensityTexture);
   //temp sarah grid->drawSlice(pEffect,passIndexDensityTexture,nGridObstructorTextureIndex,1);
   	dxC_EffectBeginPass(pEffect,passIndexDrawObstacles) ;
	grid->drawSlice(pEffect,passIndexDrawObstacles,nGridObstructorTextureIndex,1);

}

void CFluid :: addFocusUnits( VECTOR focusUnitPosition, float focusUnitRadius, float focusUnitHeight, VECTOR focusUnitDirection, float focusUnitSpeed, float velocityMultiplier, float velocityClamp, float sizeMultiplier, D3DXMATRIX worldInv, D3DXMATRIX Scale,
							  D3DXVECTOR4* FocusUnitPositionsInGrid, D3DXVECTOR4* FocusUnitHeightAndRadiusInGrid, D3DXVECTOR4* focusUnitVelocities, int& numFocusUnits )
{

	  //if unit is essentially static
      if(focusUnitSpeed<0.0000000001f)
		  return;
 

      float smallNum = 0.000001f;

      //velocity
	  D3DXVECTOR4 velocityInGrid;
	  D3DXVECTOR4 velocityInTexture;
      //transform velocity to grid space
      D3DXVECTOR4 worldSpaceVelocity(focusUnitSpeed*focusUnitDirection.x,focusUnitSpeed*focusUnitDirection.y,focusUnitSpeed*focusUnitDirection.z,0);
      D3DXVec4Transform(&velocityInTexture, &worldSpaceVelocity, &worldInv);
      D3DXVec4Transform(&velocityInGrid, &velocityInTexture, &Scale);
      velocityInGrid.y = -velocityInGrid.y;
      //divide by fps:
      //the value i need is (position - last position) .. this is the absolute change in position from previous frame to this frame; dx.
      //we are getting velocities in meters per second, something like dx/dt
      //In order to put this into the same units we need to multiply by dt (  dx/dt * dt = dx), and dt  = 1/fps
	  velocityInGrid /= AppCommonGetDrawFrameRate();
	  if( velocityMultiplier*D3DXVec4Length(&velocityInGrid) < smallNum)
		  return;

	  //position in grid
	  D3DXVECTOR4 focusPos = D3DXVECTOR4(focusUnitPosition.x, focusUnitPosition.y, focusUnitPosition.z, 1.0);
	  D3DXVECTOR4 focusPosInTexture;
	  D3DXVECTOR4 focusPosInGrid;
	  D3DXVec4Transform(&focusPosInTexture, &focusPos, &worldInv);
	  D3DXVec4Transform(&focusPosInGrid, &focusPosInTexture, &Scale);
	  D3DXVECTOR4 gridSpaceObstructorPosition(focusPosInGrid.x, (grid->dim[1]-focusPosInGrid.y), focusPosInGrid.z, 1.0);
	  
	  //radius
	  D3DXVECTOR4 radiusVec = D3DXVECTOR4( focusUnitRadius, 0, 0, 0);
	  D3DXVECTOR4 radiusVecInTexture;
	  D3DXVECTOR4 radiusVecInGrid;
	  D3DXVec4Transform(&radiusVecInTexture, &radiusVec, &worldInv);
	  D3DXVec4Transform(&radiusVecInGrid, &radiusVecInTexture, &Scale);
	  radiusVecInGrid.w = 0.0;
	  float gridSpaceObstructorRadius = D3DXVec4Length(&radiusVecInGrid); 

	  //if gridSpaceObstructorPosition is outside grid, return
	  if(    gridSpaceObstructorPosition.x < -gridSpaceObstructorRadius 
		  || gridSpaceObstructorPosition.y < -gridSpaceObstructorRadius 
		  || gridSpaceObstructorPosition.x > grid->dim[0] + gridSpaceObstructorRadius 
		  || gridSpaceObstructorPosition.y > grid->dim[1] + gridSpaceObstructorRadius) 
		  return;

	  //height
	  D3DXVECTOR4 heightVec = D3DXVECTOR4( 0, 0, focusUnitHeight, 0);
	  D3DXVECTOR4 heightVecInTexture;
	  D3DXVECTOR4 heightVecInGrid;
	  D3DXVec4Transform(&heightVecInTexture, &heightVec, &worldInv);
	  D3DXVec4Transform(&heightVecInGrid, &heightVecInTexture, &Scale);
	  heightVecInGrid.w = 0.0;
	  float gridSpaceObstructorHeight = D3DXVec4Length(&heightVecInGrid);

	  //if gridSpaceObstructorPosition is outside grid, return
	  if(    gridSpaceObstructorPosition.z < -gridSpaceObstructorHeight
		  || gridSpaceObstructorPosition.z > grid->dim[2] + gridSpaceObstructorHeight)
		  return;



	  //velocity 
	  D3DXVECTOR4 velocity = D3DXVECTOR4( velocityMultiplier * velocityInGrid.x, 
             	      					  velocityMultiplier * velocityInGrid.y, 
										  velocityMultiplier * velocityInGrid.z, 0.0f );
	  
	  float impulseLength = D3DXVec4Length( &velocity );
	  if( impulseLength > velocityClamp )
	  {
		  D3DXVec4Normalize(&velocity, &velocity);
   		  velocity *= velocityClamp;
	  }
	  velocity.w = 1.0f;

	   
	  //add to the array
	  FocusUnitPositionsInGrid[numFocusUnits] = gridSpaceObstructorPosition;
	  FocusUnitHeightAndRadiusInGrid[numFocusUnits] = D3DXVECTOR4(gridSpaceObstructorHeight, 1/(gridSpaceObstructorRadius*sizeMultiplier),0,0);
	  focusUnitVelocities[numFocusUnits] = velocity;
	  numFocusUnits++;
}



void CFluid :: draw( FluidField field , D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique )
{
   //sarah: this makes sense,but looks bad, there is a stutter in the rendering when starting up
	if(ColorTextureNumber == 0)
		SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_COLOR0      ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_COLOR);
	else
		SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_COLOR1      ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_COLOR);   

	SetRenderTargetAndViewport( dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER ), grid->texDim[0],grid->texDim[1] );
	dxC_EffectBeginPass(pEffect,passIndexDrawTexture);
	grid->drawForRender(pEffect,passIndexDrawTexture);

}

//use the volume renderer class to visualize the fluid
void CFluid :: render3D( FluidField field, D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, D3DXMATRIX *View, D3DXMATRIX *Projection, D3DXMATRIX * psysWorld, VECTOR voffsetPosition, float color[])
{
    dx9_EffectSetInt( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MAXGRIDDIM,  grid->maxDim ); 
    V( dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_WIDTH,  grid->texDim[0]) ); 
	V( dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_HEIGHT, grid->texDim[1] ) );

    smokeColor[0] = color[0];
    smokeColor[1] = color[1];
    smokeColor[2] = color[2];
    smokeColor[3] = 1;

    dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_SMOKE_COLOR, smokeColor , 0, sizeof(D3DXVECTOR4) );  

    if(ColorTextureNumber == 0)
       SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_COLOR0 ,pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_COLOR);
    else
       SetRTTextureAsShaderResource(RENDER_TARGET_FLUID_COLOR1, pEffect, pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_COLOR);   

     renderer->UpdateWorldMatrix(psysWorld, voffsetPosition); 
     renderer->draw( pEffect, pTechnique, View, Projection);

}


void CFluid :: reset( void )
{
   ClearRenderTarget( RENDER_TARGET_FLUID_VELOCITY, 0,0,0,0 );
   ClearRenderTarget( RENDER_TARGET_FLUID_COLOR1, 0,0,0,0 );
   ColorTextureNumber = 0;
}

int CFluid :: textureWidth( void )
{
  return grid->texDim[0];
}

int CFluid :: textureHeight( void )
{
  return grid->texDim[1];
}


void CFluid :: advectColor( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique)
{
   if(ColorTextureNumber == 0)
	   SetRenderTargetAndViewport( RENDER_TARGET_FLUID_COLOR0 , grid->texDim[0],grid->texDim[1] );
   else
	   SetRenderTargetAndViewport( RENDER_TARGET_FLUID_COLOR1 , grid->texDim[0],grid->texDim[1] );

   dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MODULATE, decay );
   dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_FORWARD, 1.0f );
   dxC_EffectBeginPass(pEffect,passIndexAdvect1);
   grid->drawSlices(pEffect,passIndexAdvect1);
   //comented copying other app grid->drawBoundaries(pEffect,passIndexAdvect1); 


}

//sarah: the phiBar and phiStar stuff might be a bit touchy
void CFluid :: advectColorBFECC( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique )
{
	ClearRenderTarget(dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP1 ),0,0,0,0);
	ClearRenderTarget(dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP2 ),0,0,0,0);

	//advect forward to get \phi^*
	SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP1 ), grid->texDim[0],grid->texDim[1]  );
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MODULATE, 1.0f );
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_FORWARD, 1.0f );
	dxC_EffectBeginPass(pEffect,passIndexAdvect1);
	grid->drawSlices(pEffect,passIndexAdvect1);
	grid->drawBoundaries(pEffect,passIndexAdvect1); //BFECC

	//advect back to get \bar{\phi}
	SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP2 ), grid->texDim[0],grid->texDim[1]  );
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MODULATE, 1.0f );
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_FORWARD, -1.0f );
	dxC_EffectBeginPass(pEffect,passIndexAdvect2);
	grid->drawSlices(pEffect,passIndexAdvect2);
	grid->drawBoundaries(pEffect,passIndexAdvect2); //BFECC


	//advect forward but use the BFECC advection shader which
	//uses both \phi and \bar{\phi} as source quantity
	//(specifically, (3/2)\phi^n - (1/2)\bar{\phi})
	if(ColorTextureNumber == 0)
		SetRenderTargetAndViewport( RENDER_TARGET_FLUID_COLOR0, grid->texDim[0],grid->texDim[1]  );
	else
		SetRenderTargetAndViewport( RENDER_TARGET_FLUID_COLOR1, grid->texDim[0],grid->texDim[1]  );

	//D3DXVECTOR3 halfVol( grid->dim[0]/2.0f, grid->dim[1]/2.0f, grid->dim[2]/2.0f );
	//HalfVolumeDimShaderVariable->SetFloatVector( (float*)&halfVol);

	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MODULATE, decay);
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_FORWARD, 1.0f );
	dxC_EffectBeginPass(pEffect,passIndexAdvectBFECC);
	grid->drawSlices(pEffect,passIndexAdvectBFECC);
	grid->drawBoundaries(pEffect,passIndexAdvectBFECC); //BFECC

}

void CFluid :: advectVelocity(  D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique )
{

  
    SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMPVELOCITY ), grid->texDim[0],grid->texDim[1] );

    //advect velocity by the fluid velocity
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MODULATE, 1.0f );
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_FORWARD, 1.0f );
    dxC_EffectBeginPass(pEffect,passIndexAdvectVel);
    grid->drawSlices(pEffect,passIndexAdvectVel);


	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MODULATE, -1.0f );
	dxC_EffectBeginPass(pEffect,passIndexAdvectVel);
    grid->drawBoundaries(pEffect,passIndexAdvectVel); //advectVelocity
    
}


void CFluid :: applyVorticityConfinement( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique )
{

  //compute vorticity --------------------------------------
  ClearRenderTarget(dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP1 ),0,0,0,0);

  SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP1 ), grid->texDim[0],grid->texDim[1] );

#ifdef USE_OBSTACLE_OPTIMIZATION

  dxC_EffectBeginPass(pEffect,passIndexVorticity_Obstacles);
  grid->drawSlice(pEffect,passIndexVorticity_Obstacles,1,obstructorTextureIndex+3);
  
  dxC_EffectBeginPass(pEffect,passIndexVorticity);
  grid->drawSlice(pEffect,passIndexVorticity,obstructorTextureIndex+4,grid->dim[2] - obstructorTextureIndex - 4 - 1);

#else
  dxC_EffectBeginPass(pEffect,passIndexVorticity_Obstacles);
  grid->drawSlices(pEffect,passIndexVorticity_Obstacles);
#endif

  //compute and apply vorticity confinement force -----------
  SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMPVELOCITY ), grid->texDim[0],grid->texDim[1] );
  SetAdditiveBlendingSim();
  dxC_EffectBeginPass(pEffect,passIndexConfinementNormal);
  grid->drawSlices(pEffect,passIndexConfinementNormal);

}


float lilrand()
{
  return (rand()/float(RAND_MAX) - 0.5f)*5.0f;
}


#ifdef USE_GAUSSIAN_BLOCKER

void CFluid::applyBlocker(  D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, float maxHeight)
{
    //velocity impulse:
    SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMPVELOCITY ), grid->texDim[0],grid->texDim[1]  );
	SetAlphaBlendingSimAddForces();
	dxC_EffectBeginPass(pEffect,passIndexGaussianFlat);
    grid->drawSlice(pEffect, passIndexGaussianFlat, 2, int(maxHeight));

}
#endif // USE_GAUSSIAN_BLOCKER


void CFluid :: applyExternalForces( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, D3DXVECTOR4 windDir, float windAmount )
{

	const int numCenters = 5;
	//float center[4] = { impulseX+lilrand(), impulseY+lilrand(),impulseZ+lilrand(), 0 };
	/*
	float centers[numCenters][4] = { { (float)(grid->dim[0] / 2),(float)(grid->dim[1] / 2), 0, 0 },
		                             { (float)(grid->dim[0] / 4),(float)(grid->dim[1] / 4), 0, 0 },
									 { (float)(grid->dim[0] / 4),(float)(grid->dim[1] *3/ 4), 0, 0 },
									 { (float)(grid->dim[0] *3/ 4),(float)(grid->dim[1] / 4), 0, 0 },
	                                 { (float)(grid->dim[0] *3/ 4),(float)(grid->dim[1] *3/ 4), 0, 0} }; 
    */
	float cx = (grid->dim[0] / 2);
	float cy = (grid->dim[1] / 2);
	float centers[numCenters][4] = { { cx, cy, 0, 0 },
		                             { cx - float(96 / 4), cy - float(96 / 4), 0, 0 },
									 { cx - float(96 / 4), cy + float(96 / 4), 0, 0 },
									 { cx + float(96 / 4), cy - float(96 / 4), 0, 0 },
									 { cx + float(96 / 4), cy + float(96 / 4), 0, 0 } };

	//float sizes[numCenters] = { 0.1, 0.08,0.1,0.065,0.07}; 
	float sizes[numCenters] = { 0.12, 0.1,0.12,0.08,0.09}; 
	for(int r =0;r<numCenters;r++)
		  sizes[r] *= renderer->sizeMultiplier;
	//size = 0.05;

#ifdef ADD_TEXTURE_DENSITY
	{
		 //pass to draw texture to grid cell

		 if( ColorTextureNumber == 0)
			SetRenderTargetAndViewport( RENDER_TARGET_FLUID_COLOR0, grid->texDim[0],grid->texDim[1]  );
		 else 
			SetRenderTargetAndViewport( RENDER_TARGET_FLUID_COLOR1, grid->texDim[0],grid->texDim[1]  );

		 SetAlphaDisable();
		 dxC_EffectBeginPass(pEffect,passIndexDensityTexture);
		 grid->drawSlice(pEffect,passIndexDensityTexture,nGridDensityTextureIndex,2);
     }
	 

#else
      //draw gaussian ball of color

	//if( /*addDensity && mouseDown*/ 1 )
      {
	     if( ColorTextureNumber == 0)
            SetRenderTargetAndViewport( RENDER_TARGET_FLUID_COLOR0, grid->texDim[0],grid->texDim[1]  );
		 else 
            SetRenderTargetAndViewport( RENDER_TARGET_FLUID_COLOR1, grid->texDim[0],grid->texDim[1]  );

         dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_SMOKE_COLOR, smokeColor , 0, sizeof(D3DXVECTOR4) );  

		 for(int i=0;i<numCenters;i++)
		 {
			 float center[4] = {centers[i][0], centers[i][1], centers[i][2], 0 };
			 dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_IMPULSE_CENTER, center , 0, sizeof(D3DXVECTOR4) );  

			 float mod = 1;
			 static float t = 0.0f;
			 //t += 0.01f;
			 t += renderer->timeIncrement;
			 mod = (sin(t) + 1)/3.0 + (1.0/3.0); // between 1/3 and 1
			 //mod +=	 0.5; //rand()/(float(RAND_MAX)*2); //between 0 and 0.5
			 dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_IMPULSE_SIZE,sizes[i]*mod); 
			 dxC_EffectBeginPass(pEffect,passIndexGaussian);
			 grid->drawSlice(pEffect,passIndexGaussian,nGridDensityTextureIndex,2);
			 //grid->drawSlices(pEffect,passIndexGaussian);
		 }

      }
#endif

#ifdef ADD_TEXTURE_VELOCITY
   
	  {
      SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMPVELOCITY ), grid->texDim[0],grid->texDim[1]  );
	  
	  float color[4] = { impulseDx,impulseDy,impulseDz,1.0f };
      dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_SMOKE_COLOR, color , 0, sizeof(D3DXVECTOR4) );  
     
     SetAlphaDisable();
  	 dxC_EffectBeginPass(pEffect,passIndexVelocityTexture);
     grid->drawSlice(pEffect,passIndexVelocityTexture,nGridVelocityTextureIndex,2);
	  }
#else
      //draw gaussian ball of velocity
      {
		  
         //velocity impulse
         SetRenderTargetAndViewport( GLOBAL_RT_FLUID_TEMPVELOCITY, grid->texDim[0],grid->texDim[1]  );

		 float color[4] = { impulseDx + (rand()/float(RAND_MAX))*0.1,
	                   		impulseDy + (rand()/float(RAND_MAX))*0.1,
							impulseDz,1.0f };
         dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_SMOKE_COLOR, color , 0, sizeof(D3DXVECTOR4) );  

		 
			 for(int i=0;i<numCenters;i++)
			 {
				 float center[4] = { centers[i][0], centers[i][1],centers[i][2], 0 };
				 dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_IMPULSE_CENTER, center , 0, sizeof(D3DXVECTOR4) );  
		
				 float mod = 1;
				 static float t = 0.0f;
				 //t += 0.01f;
				 t += renderer->timeIncrement;
				 mod = (sin(t) + 1)/3.0 + (1.0/3.0); // between 1/3 and 1
				 //mod +=	 0.5;//rand()/(float(RAND_MAX)*2); //between 0 and 0.5
				 dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_IMPULSE_SIZE,sizes[i]*mod); 

				 dxC_EffectBeginPass(pEffect,passIndexGaussian);
				 grid->drawSlice(pEffect,passIndexGaussian,nGridVelocityTextureIndex,2);
				 //grid->drawSlices(pEffect,passIndexGaussian);
			 }
	   }
#endif	 


#define USE_HELLGATE_WIND
#ifdef USE_HELLGATE_WIND
	    
	     if(windAmount*windModifier > 0 )
		 {
	  
			 D3DXVECTOR3 gridCenter = D3DXVECTOR3( grid->dim[0]/2.0, grid->dim[1]/2.0, grid->dim[2]/2.0); 
			 float center[4] = { gridCenter.x - windDir.x*gridCenter.x,
								 gridCenter.y - windDir.y*gridCenter.y,
								 gridCenter.z - windDir.z*gridCenter.z, 0 };
			 if( center[0] < 0)
				 center[0] = 0;
			 if( center[0] >= grid->dim[0] )
				 center[0] = grid->dim[0]-1;

			 if( center[1] < 0)
				 center[1] = 0;
			 if( center[1] >= grid->dim[1] )
				 center[1] = grid->dim[1]-1;
			 
			 if( center[2] < 0)
				 center[2] = 0;
			 if( center[2] >= grid->dim[2] )
				 center[2] = grid->dim[2]-1;

			 //temp sarah!!
			 center[2] = 75;

			 dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_IMPULSE_CENTER, center , 0, sizeof(D3DXVECTOR4) );  
			 dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_IMPULSE_SIZE,0.05);
			 
			 float color[4] = {windDir.x*windAmount*windModifier, windDir.y*windAmount*windModifier, windDir.z*windAmount*windModifier, 1.0f };
			 dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_SMOKE_COLOR, color , 0, sizeof(D3DXVECTOR4) );  
		  
	         #else
	  		 //some wind from +x
			 float center[4] = { 0, grid->dim[1]/2.0+lilrand()*1.5,grid->dim[2]/2.0+lilrand()*1.5, 0 };
			 dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_IMPULSE_CENTER, center , 0, sizeof(D3DXVECTOR4) );  
			 dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_IMPULSE_SIZE,0.041);
			 
			 static float t = rand()/float(RAND_MAX);
			 t += 0.02f;
			 float mod = (sin(t) + 1)/2.0; // between 0 and 1
			 float color[4] = { -1.3*mod, 0, 0.9*mod, 1.0f };
			 dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_SMOKE_COLOR, color , 0, sizeof(D3DXVECTOR4) );  
		 
	         #endif
			 SetAlphaBlendingSim();
			 dxC_EffectBeginPass(pEffect,passIndexGaussian);
			 grid->drawSlices(pEffect,passIndexGaussian);	
		 }
		 
}


void CFluid :: computeVelocityDivergence( D3D_EFFECT* pEffect )
{
  ClearRenderTarget(dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP1 ),0,0,0,0 );

  SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP1 ), grid->texDim[0],grid->texDim[1]  );
  SetAlphaDisable();

#ifdef USE_OBSTACLE_OPTIMIZATION
  dxC_EffectBeginPass(pEffect,passIndexDivergence_Obstacles);
  grid->drawSlice(pEffect,passIndexDivergence_Obstacles, 1, obstructorTextureIndex+3);

  dxC_EffectBeginPass(pEffect,passIndexDivergence);
  grid->drawSlice(pEffect,passIndexDivergence, obstructorTextureIndex+4, grid->dim[2] - obstructorTextureIndex - 4 - 1);
#else
  dxC_EffectBeginPass(pEffect,passIndexDivergence_Obstacles);
  grid->drawSlices(pEffect,passIndexDivergence_Obstacles);
  grid->drawBoundaries(pEffect,passIndexDivergence_Obstacles); //divergence
#endif
}


void CFluid :: computePressure( D3D_EFFECT* pEffect )
{
	
   //solve the Poisson equation $\nabla^{2} p = \nabla \cdot w$ using Jacobi iterations

   const int nIterations = 2;
   
   ClearRenderTarget( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP2 ), 0.0f, 0.0f, 0.0f, 1.0f );
   
   for( int iteration = 0; iteration < nIterations; iteration++ )
   {
      SetRenderTargetAndViewport( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_FLUID_TEMP2 ), grid->texDim[0],grid->texDim[1]  );
      
#ifdef USE_OBSTACLE_OPTIMIZATION
	  dxC_EffectBeginPass(pEffect,passIndexJacobi1_Obstacles);
	  grid->drawSlice(pEffect, passIndexJacobi1_Obstacles, 1, obstructorTextureIndex+3);

	  dxC_EffectBeginPass(pEffect,passIndexJacobi1);
	  grid->drawSlice(pEffect, passIndexJacobi1, obstructorTextureIndex+4, grid->dim[2] - obstructorTextureIndex - 4 - 1);
#else
	  dxC_EffectBeginPass(pEffect,passIndexJacobi1_Obstacles);
	  grid->drawSlices(pEffect, passIndexJacobi1_Obstacles);
	  grid->drawBoundaries(pEffect,passIndexJacobi1_Obstacles );//compute pressure, jacobi 2
#endif

      SetRenderTargetAndViewport( RENDER_TARGET_FLUID_PRESSURE, grid->texDim[0],grid->texDim[1]  );

#ifdef USE_OBSTACLE_OPTIMIZATION

	  dxC_EffectBeginPass(pEffect,passIndexJacobi2_Obstacles);
	  grid->drawSlice( pEffect,passIndexJacobi2_Obstacles,1,obstructorTextureIndex+3);

  	  dxC_EffectBeginPass(pEffect,passIndexJacobi2);
	  grid->drawSlice( pEffect,passIndexJacobi2,obstructorTextureIndex+4,grid->dim[2] - obstructorTextureIndex - 4 - 1);

#else      
	  dxC_EffectBeginPass(pEffect,passIndexJacobi2_Obstacles);
	  grid->drawSlices( pEffect,passIndexJacobi2_Obstacles);
	  grid->drawBoundaries(pEffect,passIndexJacobi2_Obstacles ); //compute pressure, jacobi 1
#endif 
   }
}

void CFluid :: projectVelocity( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique )
{
  	
  SetRenderTargetAndViewport( RENDER_TARGET_FLUID_VELOCITY, grid->texDim[0],grid->texDim[1]  );

  dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MODULATE,  1.0f ); 

#ifdef USE_OBSTACLE_OPTIMIZATION
  dxC_EffectBeginPass(pEffect, passIndexProject_Obstacles);
  grid->drawSlice( pEffect,passIndexProject_Obstacles,1,obstructorTextureIndex+3);

  dxC_EffectBeginPass(pEffect, passIndexProject);
  grid->drawSlice( pEffect,passIndexProject,obstructorTextureIndex+4,grid->dim[2] - obstructorTextureIndex - 4 - 1);

#else
  dxC_EffectBeginPass(pEffect, passIndexProject_Obstacles);
  grid->drawSlices( pEffect,passIndexProject_Obstacles);
#endif

  dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MODULATE, -1.0f ); //XXX

#ifdef USE_OBSTACLE_OPTIMIZATION
#else
  dxC_EffectBeginPass(pEffect, passIndexProject_Obstacles);
  grid->drawBoundaries(pEffect, passIndexProject_Obstacles);//XXX //project
#endif
}


void CFluid :: createOffsetTable( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique )
{

  //allocate a lookup table for a
  //function mapping a z value to the (x,y) offset in the
  //flattened 3d offsetTable of the corresponding slice
  
  texels = new float[ grid->dim[2] * 4 ];

  for( int z=0; z<grid->dim[2]; z++ )
  {

    texels[ z*4 + 0 ] = float ( z %            grid->cols ) * grid->dim[0];
    texels[ z*4 + 1 ] = floorf( z /     (float)grid->cols ) * grid->dim[1];
    texels[ z*4 + 2 ] = float ( (z+1) %        grid->cols ) * grid->dim[0];
    texels[ z*4 + 3 ] = floorf( (z+1) / (float)grid->cols ) * grid->dim[1];

  }


    HRESULT hr;

	textureOffsetTable = NULL;
    textureRviewOffset = NULL;
	hr = dx10_Create2DTexture( grid->dim[2], 1, 1, D3DC_USAGE_2DTEX, D3DFMT_A32B32G32R32F, &textureOffsetTable, texels, 0 );
	CreateSRVFromTex2D( textureOffsetTable, &textureRviewOffset);
	dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_ZOFFSET, textureRviewOffset );

}



//---------------------------------------------------------------------------------------------
//volume rendering code
//---------------------------------------------------------------------------------------------


VolumeRenderer::VolumeRenderer( ) : maxDim(0),mRenderScale(0),timeIncrement(0),
                                    sizeMultiplier(0),RayCastWidth(0),RayCastHeight(0),
                                    BackBufferWidth(0),BackBufferHeight(0)
{    
    memset(gridDim,0, sizeof(gridDim));
    D3DXMatrixIdentity(&World);
}


// constructor
void VolumeRenderer :: Initialize( int gridWidth, int gridHeight, int gridDepth, 
                                  float inputRenderScale, D3DXMATRIX * psysWorld,
								  D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique )
{
    gridDim[0] = gridWidth;
    gridDim[1] = gridHeight;
    gridDim[2] = gridDepth;


    maxDim = (float)(gridDim[0] > gridDim[1] && gridDim[0] > gridDim[2] ? gridDim[0] :
        gridDim[1] > gridDim[2] ? gridDim[1] :
        gridDim[2]);

   dx9_EffectSetInt( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_MAXGRIDDIM,  maxDim ); 

    createBox();
    createGridBox();
	createScreenQuad();

    mRenderScale = inputRenderScale;
    UpdateWorldMatrix(psysWorld, VECTOR(0,0,0));

    BackBufferWidth = AppCommonGetWindowWidth();
	BackBufferHeight = AppCommonGetWindowHeight();
    CalculateRayCastSizes( );
    UpdateTexturesAndRenderTargets(); 

}
// destructor
VolumeRenderer :: ~VolumeRenderer( void) 
{

}

void VolumeRenderer :: UpdateWorldMatrix(D3DXMATRIX * psysWorld, VECTOR voffsetPosition)
{

    //scale
	D3DXMATRIX scale;
	D3DXMatrixScaling(&scale, mRenderScale, mRenderScale, mRenderScale);

	World = scale * *psysWorld; //ts2

    //putting in the offset 
	D3DXVECTOR4 newTranslation;
	D3DXVECTOR4 translationOffset = D3DXVECTOR4(voffsetPosition.x,voffsetPosition.y,voffsetPosition.z,1);
	
	// ts1 World = *psysWorld;

	D3DXVec4Transform(&newTranslation,&translationOffset,&World); 
	World._41 = newTranslation.x;
	World._42 = newTranslation.y;
	World._43 = newTranslation.z;
    World._44 = newTranslation.w;
	
	// ts1 World = scale * World;
	
}





void VolumeRenderer :: draw( D3D_EFFECT* pEffect,  EFFECT_TECHNIQUE* pTechnique , D3DXMATRIX *View, D3DXMATRIX *Projection)
{

    int widthBB = AppCommonGetWindowWidth();
	int heightBB = AppCommonGetWindowHeight();
    if(widthBB != BackBufferWidth || heightBB != BackBufferHeight )
    {
         BackBufferWidth = AppCommonGetWindowWidth();
	     BackBufferHeight = AppCommonGetWindowHeight();
         CalculateRayCastSizes( );
         UpdateTexturesAndRenderTargets(); 
    }
    
    //real depth texture
	//This is set in e_ViewerRender_Hellgate now
	dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_SCENE_DEPTH, dxC_RTShaderResourceGet( dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_DEPTH_WO_PARTICLES ) ) );
    //dxC_SaveTextureToFile( "HellgateDepth.dds", D3DX10_IFF_DDS, dxC_RTResourceGet(RENDER_TARGET_DEPTH_WO_PARTICLES) , false );
  	
    //disable depth and stencil test
  	dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
    dxC_SetRenderState( D3DRS_STENCILENABLE, FALSE);

    //set variables for raycasting and preprocessing steps
    D3DXMATRIX worldView;
    D3DXMatrixMultiply(&worldView,&World,View);
    D3DXVECTOR3 worldXaxis = D3DXVECTOR3(worldView._11, worldView._12, worldView._13);
    float worldScale = D3DXVec3Length(&worldXaxis);
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RENDER_SCALE, worldScale );
    //set the near and far planes
	float Zfar = e_GetFarClipPlane();
	float Znear = e_GetNearClipPlane();
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_ZFAR,  Zfar  );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_ZNEAR, Znear );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_ZFAR_MULT_ZNEAR,  Zfar*Znear );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_ZFAR_MINUS_ZNEAR, Zfar-Znear );
    D3DXMATRIX scale;
    D3DXMatrixIdentity(&scale);
    D3DXMatrixScaling(&scale, gridDim[0] / maxDim, gridDim[1] / maxDim, gridDim[2] / maxDim);
    D3DXMATRIXA16 WorldViewProjection;
    WorldViewProjection = scale * World * *View * *Projection;
    dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &WorldViewProjection );
    D3DXMATRIXA16 WorldViewMatrix;
	WorldViewMatrix = scale * worldView;
    dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDVIEW, &WorldViewMatrix );

    float fovy = c_CameraGetVerticalFovDegrees( c_CameraGetViewType() );  
	fovy = DEG_TO_RAD( (float)fovy );
    float distanceToScreenY = tan( fovy/2.0 );
    dx9_EffectSetFloat ( pEffect, *pTechnique, EFFECT_PARAM_DISTANCETOSCREEN_Y, distanceToScreenY );

    float distanceToScreenX = tan( fovy/2.0 ) * e_GetWindowAspect();
    dx9_EffectSetFloat ( pEffect, *pTechnique, EFFECT_PARAM_DISTANCETOSCREEN_X, distanceToScreenX );
   
    //draw back faces of smoke volume
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
    SetAlphaDisable();
    SetRenderTargetAndViewport( dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FLUID_BOXDEPTH ), widthBB, heightBB, DEPTH_TARGET_NONE );
	ClearRenderTarget( 0,0,0,0 ); //RENDER_TARGET_BOX_DEPTH 
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_WIDTH, widthBB );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_HEIGHT, heightBB );
    dxC_EffectBeginPass(pEffect,passIndexRayDataBack);
    drawBoxDepth(pEffect, passIndexRayDataBack);

    //down sample the raydata texture
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    //no blending: already set
	dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_WIDTH, RayCastWidth );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_HEIGHT, RayCastHeight );
    SetRenderTargetAndViewport( RENDER_TARGET_RAYDATASMALL, RayCastWidth, RayCastHeight, DEPTH_TARGET_NONE );
    ClearRenderTarget( 0,0,0,0 );
	SetRTTextureAsShaderResource(dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FLUID_BOXDEPTH ),pEffect,pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_BOXDEPTH );
    dxC_EffectBeginPass(pEffect,passIndexQuadDownSampleRayDataTexture);
    drawScreenQuad(pEffect, passIndexQuadDownSampleRayDataTexture);

    //do edge detection 
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    //no blending: already set
    SetRenderTargetAndViewport( RENDER_TARGET_EDGEDATA, RayCastWidth, RayCastHeight, DEPTH_TARGET_NONE );
    ClearRenderTarget( 0,0,0,0 );
    SetRTTextureAsShaderResource(RENDER_TARGET_RAYDATASMALL,pEffect,pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_RAYDATASMALL );
    dxC_EffectBeginPass(pEffect,passIndexQuadEdgeDetect);
    drawScreenQuad(pEffect, passIndexQuadEdgeDetect);

    //draw front faces of smoke volume
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	SetSubtractiveBlendingRendering(); //SubtractiveBlending_Rendering;
    SetRenderTargetAndViewport( dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FLUID_BOXDEPTH ), widthBB, heightBB, DEPTH_TARGET_NONE );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_WIDTH, widthBB );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_HEIGHT, heightBB );
    dxC_EffectBeginPass(pEffect,passIndexRayDataFront);
    drawBoxDepth(pEffect,passIndexRayDataFront);


    //set variables for the raycasting step
	D3DXVECTOR3 scaleVec = D3DXVECTOR3(gridDim[0] / maxDim, gridDim[1] / maxDim, gridDim[2] / maxDim);
    D3DXMATRIX scaleM;
    D3DXMatrixIdentity(&scaleM);
    D3DXMatrixScaling(&scaleM, scaleVec.x, scaleVec.y, scaleVec.z);
	worldView = scaleM * World * *View;
    WorldViewProjection;
    WorldViewProjection = worldView * *Projection;
	// decompose to get the eyeInGridSpace...
	D3DXMATRIX worldViewInv;
	D3DXMatrixInverse(&worldViewInv, NULL, &worldView);
    D3DXVECTOR3 eyeInGridSpace;
	D3DXQUATERNION rotation;
	D3DXMatrixDecompose(&scaleVec, &rotation, &eyeInGridSpace, &worldViewInv);
	eyeInGridSpace += D3DXVECTOR3(0.5, 0.5, 0.5);
	dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, &WorldViewProjection );
	D3DXVECTOR4 eyeInGridSpaceVec4( eyeInGridSpace.x, eyeInGridSpace.y, eyeInGridSpace.z, 0.0f );
	V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_EYEINWORLD, &eyeInGridSpaceVec4 ) );
	D3DXMATRIX translation;
	D3DXMatrixIdentity(&translation);
    D3DXMatrixTranslation(&translation, -0.5, -0.5, -0.5);
	D3DXMATRIX wv2;
	wv2 = translation * scaleM * World * *View;
	D3DXMATRIXA16 wv2Inv;
	D3DXMatrixInverse(&wv2Inv, NULL, &wv2);
	dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_INVERSEWORLDVIEWMATRIX, &wv2Inv);
	D3DXMATRIXA16 InvProjection;
	D3DXMatrixInverse(&InvProjection, NULL, Projection);
    dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_INVERSEPROJECTIONMATRIX, &InvProjection );

    //raycast to a smaller texture
    SetAlphaDisable();
	SetRTTextureAsShaderResource(dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FLUID_BOXDEPTH ),pEffect,pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_BOXDEPTH );
	SetRenderTargetAndViewport( RENDER_TARGET_RAYCAST, RayCastWidth, RayCastHeight ,DEPTH_TARGET_NONE );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_WIDTH, RayCastWidth );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_HEIGHT, RayCastHeight );
	dxC_EffectBeginPass(pEffect,passIndexQuadRaycast);
	drawScreenQuad(pEffect,passIndexQuadRaycast);

    //copy raycast texture to real screen
	SetAlphaBlendingRendering(); //AlphaBlending_Rendering
	SetRenderTargetAndViewport( dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER ), widthBB, heightBB, dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO ) );
    SetRTTextureAsShaderResource(RENDER_TARGET_RAYCAST,pEffect,pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_RAYCAST );
    SetRTTextureAsShaderResource(RENDER_TARGET_EDGEDATA,pEffect,pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_EDGEDATA );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_WIDTH, widthBB );
    dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_RT_HEIGHT, heightBB );
	dxC_EffectBeginPass(pEffect,passIndexQuadRaycastCopy);
	drawScreenQuad(pEffect,passIndexQuadRaycastCopy);

}


void VolumeRenderer :: drawBoxDepth( D3D_EFFECT* pEffect, UINT iPassIndex )
{
    // Set vertex buffer
    UINT stride = sizeof( VS_FLUID_RENDERING_INPUT_STRUCT );
    UINT offset = 0;
    dxC_SetVertexDeclaration(VERTEX_DECL_VS_FLUID_RENDERING_INPUT,pEffect);
    dxC_SetStreamSource(GridBoxVertexBufferDesc,offset,pEffect);

    // Set index buffer
    dxC_SetIndices( GridBoxIndexBufferDesc, TRUE );

	//draw the primitive
    dxC_DrawIndexedPrimitive( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0,	0, 24, 0, 36/3,pEffect, METRICS_GROUP_UNKNOWN, iPassIndex );


}


// for drawing a quad on screen space
void VolumeRenderer::drawScreenQuad( D3D_EFFECT* pEffect, UINT iPassIndex )
{

  UINT stride = sizeof(VS_FLUID_RENDERING_INPUT_STRUCT);
  UINT offset = 0;
  //set the vertex buffer
  dxC_SetVertexDeclaration(VERTEX_DECL_VS_FLUID_RENDERING_INPUT,pEffect);
  dxC_SetStreamSource(ScreenQuadVertexBufferDesc,offset,pEffect);
  //draw the primitive
  dxC_DrawPrimitive( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 0, 4-2, pEffect, METRICS_GROUP_UNKNOWN, iPassIndex );
 
}

void VolumeRenderer::CalculateRayCastSizes(  )
{
    int maxProjectedSide = int(sqrt(3.0f)*maxDim);
    int maxScreenDim = max(BackBufferWidth,BackBufferHeight);
    
    float screenAspectRatio = ((float)BackBufferWidth)/BackBufferHeight;

    if( maxScreenDim > maxProjectedSide)
    {
        if(BackBufferWidth > BackBufferHeight)
        {
            RayCastHeight = maxProjectedSide;
            RayCastWidth = (int)(screenAspectRatio * maxProjectedSide);
        }
        else
        {
            RayCastWidth = maxProjectedSide;
            RayCastHeight = (int)((1.0f/screenAspectRatio) * maxProjectedSide);
        }
    }
    else
    {
        RayCastWidth = BackBufferWidth;
        RayCastHeight = BackBufferHeight;
    }
}

void VolumeRenderer::UpdateTexturesAndRenderTargets()
{
    RENDER_TARGET_RAYCAST =			RENDER_TARGET_INDEX(dxC_AddRenderTargetDesc(FALSE,-1,-1,RayCastWidth,RayCastHeight,D3DC_USAGE_2DTEX_RT,D3DFMT_A32B32G32R32F,	-1,FALSE, 1 ));
    RENDER_TARGET_RAYDATASMALL =	RENDER_TARGET_INDEX(dxC_AddRenderTargetDesc(FALSE,-1,-1,RayCastWidth,RayCastHeight,D3DC_USAGE_2DTEX_RT,D3DFMT_A32B32G32R32F,	-1,FALSE, 1 ));
    RENDER_TARGET_EDGEDATA =		RENDER_TARGET_INDEX(dxC_AddRenderTargetDesc(FALSE,-1,-1,RayCastWidth,RayCastHeight,D3DC_USAGE_2DTEX_RT,D3DFMT_R32F,				-1,FALSE, 1 ));
}

float w0( float a )
{
    return (1.0f/6.0f)*( -a*a*a + 3.0f*a*a - 3.0f*a + 1 );
}

float w1( float a )
{
    return (1.0f/6.0f)*( 3.0f*a*a*a - 6.0f*a*a + 4.0f );
}

float w2( float a )
{
    return (1.0f/6.0f)*( -3.0f*a*a*a + 3.0f*a*a + 3.0f*a + 1.0f );
}

float w3( float a )
{
    return (1.0f/6.0f)*( a*a*a );
}

float g0( float a )
{
    return w0(a) + w1(a);
}

float g1( float a )
{
    return w2(a) + w3(a);
}

float h0( float a )
{
    return -1.0f + w1(a)/(w0(a)+w1(a));
}

float h1( float a )
{
    return 1.0f + w3(a)/(w2(a)+w3(a));
}


void VolumeRenderer::createScreenQuad(void ) 
{

    // Create a screen quad for all render to texture operations
    VS_FLUID_RENDERING_INPUT_STRUCT svQuad[4];
    svQuad[0].Position = D3DXVECTOR4(-1.0f, 1.0f, 0.0f, 1.0f);
    svQuad[0].Tex = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    svQuad[1].Position = D3DXVECTOR4(1.0f, 1.0f, 0.0f, 1.0f);
    svQuad[1].Tex = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    svQuad[2].Position = D3DXVECTOR4(-1.0f, -1.0f, 0.0f, 1.0f);
    svQuad[2].Tex = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    svQuad[3].Position = D3DXVECTOR4(1.0f, -1.0f, 0.0f, 1.0f);
    svQuad[3].Tex = D3DXVECTOR3(1.0f, 1.0f, 0.0f);
    
    //create the vertex buffer

	dxC_VertexBufferDefinitionInit( ScreenQuadVertexBufferDesc );
	//ScreenQuadVertexBufferDesc.nD3DBufferID;
	ScreenQuadVertexBufferDesc.nBufferSize[ 0 ] = sizeof(VS_FLUID_RENDERING_INPUT_STRUCT)*4;
	ScreenQuadVertexBufferDesc.dwFlags = 0x00000000;
	ScreenQuadVertexBufferDesc.nVertexCount = 4; 	
	ScreenQuadVertexBufferDesc.eVertexType = VERTEX_DECL_VS_FLUID_RENDERING_INPUT;
	ScreenQuadVertexBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;
	//ScreenQuadVertexBufferDesc.dwFVF;
	//ScreenQuadVertexBufferDesc.pLocalData;

    V( dxC_CreateVertexBuffer(0, ScreenQuadVertexBufferDesc, svQuad ) );

}

void VolumeRenderer::createBox (void) 
{

    // Create vertex buffer
    VS_FLUID_RENDERING_INPUT_STRUCT vertices[] =
    {
		{ D3DXVECTOR4( -0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4(  0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4(  0.5f,  0.5f,  0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( -0.5f,  0.5f,  0.5f, 1 ), D3DXVECTOR3(0,0,0) },

        { D3DXVECTOR4( -0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4(  0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4(  0.5f, -0.5f,  0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( -0.5f, -0.5f,  0.5f, 1 ), D3DXVECTOR3(0,0,0) },

        { D3DXVECTOR4( -0.5f, -0.5f,  0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( -0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( -0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( -0.5f,  0.5f,  0.5f, 1 ), D3DXVECTOR3(0,0,0) },

        { D3DXVECTOR4( 0.5f, -0.5f,  0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( 0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( 0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( 0.5f,  0.5f,  0.5f, 1 ), D3DXVECTOR3(0,0,0) },

        { D3DXVECTOR4( -0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4(  0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4(  0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( -0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3(0,0,0) },

        { D3DXVECTOR4( -0.5f, -0.5f, 0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4(  0.5f, -0.5f, 0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4(  0.5f,  0.5f, 0.5f, 1 ), D3DXVECTOR3(0,0,0) },
        { D3DXVECTOR4( -0.5f,  0.5f, 0.5f, 1 ), D3DXVECTOR3(0,0,0) },
    };

	dxC_VertexBufferDefinitionInit( BoxVertexBufferDesc );
	//BoxVertexBufferDesc.nD3DBufferID;
	BoxVertexBufferDesc.nBufferSize[ 0 ] = sizeof(VS_FLUID_RENDERING_INPUT_STRUCT)*24;
	BoxVertexBufferDesc.dwFlags = 0x00000000;
	BoxVertexBufferDesc.nVertexCount = 24; 	
	BoxVertexBufferDesc.eVertexType = VERTEX_DECL_VS_FLUID_RENDERING_INPUT;
	BoxVertexBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;
	//BoxVertexBufferDesc.dwFVF;
	//BoxVertexBufferDesc.pLocalData;

    V( dxC_CreateVertexBuffer(0, BoxVertexBufferDesc, vertices ) );



    // Create index buffer
    DWORD indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

	dxC_IndexBufferDefinitionInit( BoxIndexBufferDesc );
	BoxIndexBufferDesc.nBufferSize = sizeof( DWORD )*36;
	BoxIndexBufferDesc.nIndexCount = 36;
	BoxIndexBufferDesc.tFormat = DXGI_FORMAT_R32_UINT;
	BoxIndexBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;

    dxC_CreateIndexBuffer(BoxIndexBufferDesc, indices );

}

void VolumeRenderer::createGridBox (void) 
{

    VS_FLUID_RENDERING_INPUT_STRUCT vertices[] =
    {
        { D3DXVECTOR4( -0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3( 0.0f, 1.0f, 0.0f) },
        { D3DXVECTOR4(  0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3( 1.0f, 1.0f, 0.0f) },
        { D3DXVECTOR4(  0.5f,  0.5f,  0.5f, 1 ), D3DXVECTOR3( 1.0f, 1.0f, 1.0f) },
        { D3DXVECTOR4( -0.5f,  0.5f,  0.5f, 1 ), D3DXVECTOR3( 0.0f, 1.0f, 1.0f) },

        { D3DXVECTOR4( -0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3( 0.0f, 0.0f, 0.0f) },
        { D3DXVECTOR4(  0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3( 1.0f, 0.0f, 0.0f) },
        { D3DXVECTOR4(  0.5f, -0.5f,  0.5f, 1 ), D3DXVECTOR3( 1.0f, 0.0f, 1.0f) },
        { D3DXVECTOR4( -0.5f, -0.5f,  0.5f, 1 ), D3DXVECTOR3( 0.0f, 0.0f, 1.0f) },

        { D3DXVECTOR4( -0.5f, -0.5f,  0.5f, 1 ), D3DXVECTOR3( 0.0f, 0.0f, 1.0f) },
        { D3DXVECTOR4( -0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3( 0.0f, 0.0f, 0.0f) },
        { D3DXVECTOR4( -0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3( 0.0f, 1.0f, 0.0f) },
        { D3DXVECTOR4( -0.5f,  0.5f,  0.5f, 1 ), D3DXVECTOR3( 0.0f, 1.0f, 1.0f) },

        { D3DXVECTOR4( 0.5f, -0.5f,  0.5f, 1 ), D3DXVECTOR3( 1.0f, 0.0f, 1.0f) },
        { D3DXVECTOR4( 0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3( 1.0f, 0.0f, 0.0f) },
        { D3DXVECTOR4( 0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3( 1.0f, 1.0f, 0.0f) },
        { D3DXVECTOR4( 0.5f,  0.5f,  0.5f, 1 ), D3DXVECTOR3( 1.0f, 1.0f, 1.0f) },

        { D3DXVECTOR4( -0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3( 0.0f, 0.0f, 0.0f) },
        { D3DXVECTOR4(  0.5f, -0.5f, -0.5f, 1 ), D3DXVECTOR3( 1.0f, 0.0f, 0.0f) },
        { D3DXVECTOR4(  0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3( 1.0f, 1.0f, 0.0f) },
        { D3DXVECTOR4( -0.5f,  0.5f, -0.5f, 1 ), D3DXVECTOR3( 0.0f, 1.0f, 0.0f) },

        { D3DXVECTOR4( -0.5f, -0.5f, 0.5f, 1 ), D3DXVECTOR3( 0.0f, 0.0f, 1.0f) },
        { D3DXVECTOR4(  0.5f, -0.5f, 0.5f, 1 ), D3DXVECTOR3( 1.0f, 0.0f, 1.0f) },
        { D3DXVECTOR4(  0.5f,  0.5f, 0.5f, 1 ), D3DXVECTOR3( 1.0f, 1.0f, 1.0f) },
        { D3DXVECTOR4( -0.5f,  0.5f, 0.5f, 1 ), D3DXVECTOR3( 0.0f, 1.0f, 1.0f) },
    };

	dxC_VertexBufferDefinitionInit( GridBoxVertexBufferDesc );
	GridBoxVertexBufferDesc.dwFlags = 0x00000000;
	GridBoxVertexBufferDesc.eVertexType = VERTEX_DECL_VS_FLUID_RENDERING_INPUT;
	GridBoxVertexBufferDesc.nBufferSize[ 0 ] = sizeof(VS_FLUID_RENDERING_INPUT_STRUCT)*24;
	GridBoxVertexBufferDesc.nVertexCount = 24;
	GridBoxVertexBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;

	V( dxC_CreateVertexBuffer(0, GridBoxVertexBufferDesc, vertices ) );



    // Create index buffer
    DWORD indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

	dxC_IndexBufferDefinitionInit( GridBoxIndexBufferDesc );
	GridBoxIndexBufferDesc.nBufferSize = sizeof( DWORD )*36;
	GridBoxIndexBufferDesc.nIndexCount = 36;
	GridBoxIndexBufferDesc.tFormat = DXGI_FORMAT_R32_UINT;
	GridBoxIndexBufferDesc.tUsage = D3DC_USAGE_BUFFER_REGULAR;

    dxC_CreateIndexBuffer(GridBoxIndexBufferDesc, indices );
}



#ifndef PROFILE
bool CFluid::NVAPICheck()
{
    //NvDisplayHandle     displayHandles[NVAPI_MAX_DISPLAYS];
    //NvU32               displayCount;
    NvLogicalGpuHandle  logicalGPUs[NVAPI_MAX_LOGICAL_GPUS];
    NvU32               logicalGPUCount;
    NvPhysicalGpuHandle physicalGPUs[NVAPI_MAX_PHYSICAL_GPUS];
    NvU32               physicalGPUCount;

    NvAPI_Status status;
    status = NvAPI_Initialize();

    if (status != NVAPI_OK)
    {
        // error
		return false;
    }

    NV_DISPLAY_DRIVER_VERSION	version = {0};
    version.version = NV_DISPLAY_DRIVER_VERSION_VER;

    status = NvAPI_GetDisplayDriverVersion(NVAPI_DEFAULT_HANDLE, &version);

    if (status != NVAPI_OK)
    {
        // error
		return false;
    }

    // enumerate logical gpus
    status = NvAPI_EnumLogicalGPUs(logicalGPUs, &logicalGPUCount);
    if (status != NVAPI_OK)
    {
        // error
		return false;
    }

    // enumerate physical gpus
    status = NvAPI_EnumPhysicalGPUs(physicalGPUs, &physicalGPUCount);
    if (status != NVAPI_OK)
    {
        // error
    	return false;
    }

    // a properly configured sli system should show up as one logical device with multiple physical devices
    if (logicalGPUCount == 1)
    {
        if (physicalGPUCount > 1)
            numGPUs = physicalGPUCount; // we have an sli system
        else
            numGPUs = 1; // logicalGPUCount == physicalGPUCount == 1, so it's a single GPU system
    }
    else
    {
        // we have more than one logical GPU; it is likely that this is a multi-gpu system with SLI disabled in the control panel
        numGPUs = 1;
    }

    return true;
}
#endif