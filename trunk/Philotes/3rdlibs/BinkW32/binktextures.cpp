
#define D3D_OVERLOADS
#include <d3d9.h> // change to your 9.0 or up path
#include <d3dx9.h>
#include "binktextures.h"


//
// pointers to our local vertex and pixel shader
//

static IDirect3DPixelShader9  * YCrCbToRGBNoPixelAlpha = 0;
static IDirect3DPixelShader9  * YCrCbAToRGBA = 0;
static LPDIRECT3DVERTEXDECLARATION9 vertex_decl = 0;

typedef struct POS_TC_VERTEX
{
  F32 sx, sy, sz, rhw;  // Screen coordinates
  F32 tu, tv;           // Texture coordinates
} POS_TC_VERTEX;


//
// simple pixel shader to apply the yuvtorgb matrix
//

static const char StrYCrCbToRGBNoPixelAlpha[] =
" sampler tex0   : register( s0 );      "
" sampler tex1   : register( s1 );      "
" sampler tex2   : register( s2 );      "
" float4  tor    : register( c0 );      "
" float4  tog    : register( c1 );      "
" float4  tob    : register( c2 );      "
" float4  consts : register( c3 );      "
"                                       "
" struct VS_OUT                         "
" {                                     "
"     float2 T0: TEXCOORD0;             "
" };                                    "
"                                       "
" float4 main( VS_OUT In ) : COLOR      "
" {                                     "
"   float4 c;                           "
"   float4 p;                           "
"   c.x = tex2D( tex0, In.T0 ).x;       "
"   c.y = tex2D( tex1, In.T0 ).x;       "
"   c.z = tex2D( tex2, In.T0 ).x;       "
"   c.w = consts.x;                     "
"   p.x = dot( tor, c );                "
"   p.y = dot( tog, c );                "
"   p.z = dot( tob, c );                "
"   p.w = consts.w;                     "
"   return p;                           "
" }                                     ";

//
// simple pixel shader to apply the yuvtorgb matrix with alpha
//

static const char StrYCrCbAToRGBA[] =
" sampler tex0   : register( s0 );      "
" sampler tex1   : register( s1 );      "
" sampler tex2   : register( s2 );      "
" sampler tex3   : register( s3 );      "
" float4  tor    : register( c0 );      "
" float4  tog    : register( c1 );      "
" float4  tob    : register( c2 );      "
" float4  consts : register( c3 );      "
"                                       "
" struct VS_OUT                         "
" {                                     "
"     float2 T0: TEXCOORD0;             "
" };                                    "
"                                       "
" float4 main( VS_OUT In ) : COLOR      "
" {                                     "
"   float4 c;                           "
"   float4 p;                           "
"   c.x = tex2D( tex0, In.T0 ).x;       "
"   c.y = tex2D( tex1, In.T0 ).x;       "
"   c.z = tex2D( tex2, In.T0 ).x;       "
"   c.w = consts.x;                     "
"   p.w = tex2D( tex3, In.T0 ).x;       "
"   p.x = dot( tor, c );                "
"   p.y = dot( tog, c );                "
"   p.z = dot( tob, c );                "
"   p.w *= consts.w;                    "
"   return p;                           "
" }                                     ";


//
// matrix to convert yuv to rgb
// not const - we change the final value to reflect global alpha
//

static float yuvtorgb[] =
{
   1.164123535f,  1.595794678f,  0.0f,         -0.87065506f,
   1.164123535f, -0.813476563f, -0.391448975f,  0.529705048f,
   1.164123535f,  0.0f,          2.017822266f, -1.081668854f,
   1.0f, 0.0f, 0.0f, 0.0f
};


static D3DVERTEXELEMENT9 vertex_def[] =
    { { 0, 0,               D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
      { 0, (4*sizeof(F32)), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
      D3DDECL_END() };


//############################################################################
//##                                                                        ##
//## Free the shaders that we use.                                          ##
//##                                                                        ##
//############################################################################

void Free_Bink_shaders( void )
{
  if ( vertex_decl )
  {
    vertex_decl->Release();
    vertex_decl = 0;
  }

  if ( YCrCbToRGBNoPixelAlpha )
  {
    YCrCbToRGBNoPixelAlpha->Release();
    YCrCbToRGBNoPixelAlpha = 0;
  }

  if ( YCrCbAToRGBA )
  {
    YCrCbAToRGBA->Release();
    YCrCbAToRGBA = 0;
  }

}


//############################################################################
//##                                                                        ##
//## Create the three shaders that we use.                                  ##
//##                                                                        ##
//############################################################################

S32 Create_Bink_shaders( LPDIRECT3DDEVICE9 d3d_device )
{
  HRESULT hr;
  ID3DXBuffer* buffer = 0;

  //
  // Define the vertex buffer layout
  //

  if ( vertex_decl == 0 )
  {
    if ( FAILED( d3d_device->CreateVertexDeclaration( vertex_def, &vertex_decl ) ) )
      return( 0 );
  }
  
  //
  // create a pixel shader that goes from YcRcB to RGB (without alpha)
  //

  if ( YCrCbToRGBNoPixelAlpha == 0 )
  {
    hr = D3DXCompileShader( StrYCrCbToRGBNoPixelAlpha, sizeof( StrYCrCbToRGBNoPixelAlpha ),
                            0, 0, "main", "ps_2_0", 0, &buffer, NULL, NULL );
    if ( SUCCEEDED( hr ) )
    {
      hr = d3d_device->CreatePixelShader( (DWORD*) buffer->GetBufferPointer(), &YCrCbToRGBNoPixelAlpha );
      buffer->Release();

      if ( FAILED( hr ) )
      {
        Free_Bink_shaders( );
        return( 0 );
      }
    }
  }

  //
  // create a pixel shader that goes from YcRcB to RGB with an alpha plane
  //

  if ( YCrCbAToRGBA == 0 )
  {
    hr = D3DXCompileShader( StrYCrCbAToRGBA, sizeof( StrYCrCbAToRGBA ),
                            0, 0, "main", "ps_2_0", 0, &buffer, NULL, NULL );
    if ( SUCCEEDED( hr ) )
    {
      hr = d3d_device->CreatePixelShader( (DWORD*) buffer->GetBufferPointer(), &YCrCbAToRGBA );
      buffer->Release();

      if ( FAILED( hr ) )
      {
        Free_Bink_shaders( );
        return( 0 );
      }
    }
  }

  return( 1 );
}


//############################################################################
//##                                                                        ##
//## Free the textures that we allocated.                                   ##
//##                                                                        ##
//############################################################################

void Free_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                         BINKTEXTURESET * set_textures )
{
  BINKFRAMETEXTURES * abt[] = { &set_textures->textures[ 0 ], &set_textures->textures[ 1 ], &set_textures->tex_draw };
  BINKFRAMETEXTURES * bt;
  int i;

  // Free the texture memory and then the textures directly
  for ( i = 0; i < sizeof( abt )/sizeof( *abt ); ++i )
  {
    bt = abt[ i ];
    if ( bt->Ytexture )
    {
      bt->Ytexture->Release();
      bt->Ytexture = NULL;
    }
    if ( bt->cRtexture )
    {
      bt->cRtexture->Release();
      bt->cRtexture = NULL;
    }
    if ( bt->cBtexture )
    {
      bt->cBtexture->Release();
      bt->cBtexture = NULL;
    }
    if ( bt->Atexture )
    {
      bt->Atexture->Release();
      bt->Atexture = NULL;
    }
  }
}


//############################################################################
//##                                                                        ##
//## Create a texture while allocating the memory ourselves.                ##
//##                                                                        ##
//############################################################################

static S32 make_texture( LPDIRECT3DDEVICE9 d3d_device,
                         U32 width, U32 height, DWORD usage, D3DFORMAT format, D3DPOOL pool, U32 pixel_size,
                         LPDIRECT3DTEXTURE9 * out_texture, void ** out_ptr, U32 * out_pitch, U32 * out_size )
{
  LPDIRECT3DTEXTURE9 texture = NULL;

  //
  // Create a dynamic texture.
  //

  if ( SUCCEEDED( d3d_device->CreateTexture(width, height, 1,
                                            usage, format, pool,
                                            &texture, 0 ) ) )
  {
    *out_texture = texture;
    *out_size = width * height * pixel_size;

    if ( out_ptr && out_pitch )
    {
      D3DLOCKED_RECT lr;

      if ( FAILED( texture->LockRect( 0, &lr, 0, 0 ) ) )
        return( 0 );

      *out_pitch = lr.Pitch;
      *out_ptr = lr.pBits;
      texture->UnlockRect( 0 );
    }

    return( 1 );
  }

  //
  // Failed
  //

  if (texture)
  {
    texture->Release();
  }

  return( 0 );
}


//############################################################################
//##                                                                        ##
//## Create 2 sets of textures for Bink to decompress into...               ##
//## Also does some basic sampler and render state init                     ##
//##                                                                        ##
//############################################################################

S32 Create_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                          BINKTEXTURESET * set_textures )
{
  BINKFRAMEBUFFERS * bb;
  BINKFRAMETEXTURES * bt;
  int i;

  //
  // Create our system decompress textures
  //

  bb = &set_textures->bink_buffers;

  for ( i = 0; i < set_textures->bink_buffers.TotalFrames; ++i )
  {
    bt = &set_textures->textures[ i ];
    bt->Ytexture = 0;
    bt->cBtexture = 0;
    bt->cRtexture = 0;
    bt->Atexture = 0;

    // Create Y plane
    if ( bb->Frames[ i ].YPlane.Allocate )
    {
      if ( !make_texture( d3d_device,
                          bb->YABufferWidth, bb->YABufferHeight,
                          0, D3DFMT_L8, D3DPOOL_SYSTEMMEM, 1,
                          &bt->Ytexture,
                          &bb->Frames[ i ].YPlane.Buffer,
                          &bb->Frames[ i ].YPlane.BufferPitch,
                          &bt->Ysize ) )
        goto fail;
    }

    // Create cR plane
    if ( bb->Frames[ i ].cRPlane.Allocate )
    {
      if ( !make_texture( d3d_device,
                          bb->cRcBBufferWidth, bb->cRcBBufferHeight,
                          0, D3DFMT_L8, D3DPOOL_SYSTEMMEM, 1,
                          &bt->cRtexture,
                          &bb->Frames[ i ].cRPlane.Buffer,
                          &bb->Frames[ i ].cRPlane.BufferPitch,
                          &bt->cRsize ) )
        goto fail;
    }

    // Create cB plane
    if ( bb->Frames[ i ].cBPlane.Allocate )
    {
      if ( !make_texture( d3d_device,
                          bb->cRcBBufferWidth, bb->cRcBBufferHeight,
                          0, D3DFMT_L8, D3DPOOL_SYSTEMMEM, 1,
                          &bt->cBtexture,
                          &bb->Frames[ i ].cBPlane.Buffer,
                          &bb->Frames[ i ].cBPlane.BufferPitch,
                          &bt->cBsize ) )
        goto fail;
    }

    // Create alpha plane
    if ( bb->Frames[ i ].APlane.Allocate )
    {
      if ( !make_texture( d3d_device,
                          bb->YABufferWidth, bb->YABufferHeight,
                          0, D3DFMT_L8, D3DPOOL_SYSTEMMEM, 1,
                          &bt->Atexture,
                          &bb->Frames[ i ].APlane.Buffer,
                          &bb->Frames[ i ].APlane.BufferPitch,
                          &bt->Asize ) )
        goto fail;
    }
  }

  //
  // Create our output draw texture (this should be in video card memory)
  //

  bt = &set_textures->tex_draw;
  bt->Ytexture = 0;
  bt->cBtexture = 0;
  bt->cRtexture = 0;
  bt->Atexture = 0;

  // Create Y plane
  if ( bb->Frames[ 0 ].YPlane.Allocate )
  {
    if ( !make_texture( d3d_device,
                        bb->YABufferWidth, bb->YABufferHeight,
                        D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, 1,
                        &bt->Ytexture,
                        0, 0,
                        &bt->Ysize ) )
      goto fail;
  }

  // Create cR plane
  if ( bb->Frames[ 0 ].cRPlane.Allocate )
  {
    if ( !make_texture( d3d_device,
                        bb->cRcBBufferWidth, bb->cRcBBufferHeight,
                        D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, 1,
                        &bt->cRtexture,
                        0, 0,
                        &bt->cRsize ) )
      goto fail;
  }

  // Create cB plane
  if ( bb->Frames[ 0 ].cBPlane.Allocate )
  {
    if ( !make_texture( d3d_device,
                        bb->cRcBBufferWidth, bb->cRcBBufferHeight,
                        D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, 1,
                        &bt->cBtexture,
                        0, 0,
                        &bt->cBsize ) )
      goto fail;
  }

  // Create alpha plane
  if ( bb->Frames[ 0 ].APlane.Allocate )
  {
    if ( !make_texture( d3d_device,
                        bb->YABufferWidth, bb->YABufferHeight,
                        D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, 1,
                        &bt->Atexture,
                        0, 0,
                        &bt->Asize ) )
      goto fail;
  }

  return( 1 );

fail:

  Free_Bink_textures( d3d_device, set_textures );
  return( 0 );
}


//############################################################################
//##                                                                        ##
//## Lock Bink textures for use by D3D.                                     ##
//##                                                                        ##
//############################################################################

void Lock_Bink_textures( BINKTEXTURESET * set_textures )
{
  BINKFRAMETEXTURES * bt;
  BINKFRAMEPLANESET * bp;
  D3DLOCKED_RECT lr;
  int frame_cur;
  int i;

  //
  // Lock the frame textures
  //

  bt = set_textures->textures;
  bp = set_textures->bink_buffers.Frames;

  frame_cur = set_textures->bink_buffers.FrameNum;

  for ( i = 0; i < set_textures->bink_buffers.TotalFrames; ++i, ++bt, ++bp )
  {
    U32 lock_flags = ( i == frame_cur ) ? D3DLOCK_NO_DIRTY_UPDATE : D3DLOCK_READONLY;

    if ( SUCCEEDED( bt->Ytexture->LockRect( 0, &lr, NULL, lock_flags ) ) )
    {
      bp->YPlane.Buffer = lr.pBits;
      bp->YPlane.BufferPitch = lr.Pitch;
    }

    if ( SUCCEEDED( bt->cRtexture->LockRect( 0, &lr, NULL, lock_flags ) ) )
    {
      bp->cRPlane.Buffer = lr.pBits;
      bp->cRPlane.BufferPitch = lr.Pitch;
    }

    if ( SUCCEEDED( bt->cBtexture->LockRect( 0, &lr, NULL, lock_flags ) ) )
    {
      bp->cBPlane.Buffer = lr.pBits;
      bp->cBPlane.BufferPitch = lr.Pitch;
    }

    if ( bt->Atexture )
    {
      //
      // Lock the alpha texture
      //

      if ( SUCCEEDED( bt->Atexture->LockRect( 0, &lr, NULL, lock_flags ) ) )
      {
        bp->APlane.Buffer = lr.pBits;
        bp->APlane.BufferPitch = lr.Pitch;
      }
    }
  }
}


//############################################################################
//##                                                                        ##
//## Unlock Bink textures for use by D3D.                                   ##
//##                                                                        ##
//############################################################################

void Unlock_Bink_textures( LPDIRECT3DDEVICE9 d3d_device, BINKTEXTURESET * set_textures, HBINK Bink )
{
  BINKFRAMETEXTURES * bt;
  BINKFRAMEPLANESET * bp;
  int i;

  //
  // Unlock the frame textures
  //

  bt = set_textures->textures;
  bp = set_textures->bink_buffers.Frames;

  for ( i = 0; i < set_textures->bink_buffers.TotalFrames; ++i, ++bt, ++bp )
  {
    bt->Ytexture->UnlockRect( 0 );
    bp->YPlane.Buffer = NULL;

    bt->cRtexture->UnlockRect( 0 );
    bp->cRPlane.Buffer = NULL;

    bt->cBtexture->UnlockRect( 0 );
    bp->cBPlane.Buffer = NULL;

    if ( bt->Atexture )
    {
      //
      // Unlock the alpha texture
      //

      bt->Atexture->UnlockRect( 0 );
      bp->APlane.Buffer = NULL;
    }
  }

  //
  // Update the pixels on the drawing texture
  //

  S32 num_rects;
  BINKFRAMETEXTURES * bt_dst;
  BINKFRAMETEXTURES * bt_src;

  bt_src = &set_textures->textures[ set_textures->bink_buffers.FrameNum ];
  bt_dst = &set_textures->tex_draw;

  num_rects = BinkGetRects( Bink, BINKSURFACEFAST );
  if ( num_rects > 0 )
  {
    BINKRECT * brc;
    RECT rc;
    S32 i;

    for ( i = 0; i < num_rects; ++i )
    {
      brc = &Bink->FrameRects[ i ];

      rc.left = brc->Left;
      rc.top = brc->Top;
      rc.right = rc.left + brc->Width;
      rc.bottom = rc.top + brc->Height;
      bt_src->Ytexture->AddDirtyRect( &rc );
      if ( bt_src->Atexture )
      {
        bt_src->Atexture->AddDirtyRect( &rc );
      }

      rc.left >>= 1;
      rc.top >>= 1;
      rc.right >>= 1;
      rc.bottom >>= 1;
      bt_src->cRtexture->AddDirtyRect( &rc );
      bt_src->cBtexture->AddDirtyRect( &rc );
    }

    d3d_device->UpdateTexture( bt_src->Ytexture, bt_dst->Ytexture);
    d3d_device->UpdateTexture( bt_src->cRtexture, bt_dst->cRtexture);
    d3d_device->UpdateTexture( bt_src->cBtexture, bt_dst->cBtexture);

    if ( bt_src->Atexture )
    {
      d3d_device->UpdateTexture( bt_src->Atexture, bt_dst->Atexture);
    }
  }

}


//############################################################################
//##                                                                        ##
//## Draw our textures onto the screen with our vertex and pixel shaders.   ##
//##                                                                        ##
//############################################################################

void Draw_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                         BINKTEXTURESET * set_textures,
                         U32 width,
                         U32 height,
                         F32 x_offset,
                         F32 y_offset,
                         F32 x_scale,
                         F32 y_scale,
                         F32 alpha_level )
{
  POS_TC_VERTEX vertices[ 4 ];
  BINKFRAMEPLANESET * bp;
  BINKFRAMETEXTURES * bt_dst;
  BINKFRAMETEXTURES * bt_src;
  int i;

  bp = &set_textures->bink_buffers.Frames[ set_textures->bink_buffers.FrameNum ];
  bt_src = &set_textures->textures[ set_textures->bink_buffers.FrameNum ];
  bt_dst = &set_textures->tex_draw;

  //
  // Turn on texture filtering and texture clamping
  //

  for( i = 0 ; i < 4 ; i++ )
  {
    d3d_device->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    d3d_device->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
  }

  //
  // turn off Z buffering, culling, and projection (since we are drawing orthographically)
  //

  d3d_device->SetRenderState( D3DRS_ZENABLE, FALSE );
  d3d_device->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
  d3d_device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

  //
  // Set the textures.
  //

  d3d_device->SetTexture( 0, bt_dst->Ytexture );
  d3d_device->SetTexture( 1, bt_dst->cRtexture );
  d3d_device->SetTexture( 2, bt_dst->cBtexture );

  //
  // upload the YUV to RGB matrix
  //

  yuvtorgb[15] = alpha_level;
  d3d_device->SetPixelShaderConstantF( 0, yuvtorgb, 4 );

  //
  // are we using an alpha plane? if so, turn on the 4th texture and set our pixel shader
  //

  if ( bt_src->Atexture )
  {
    //
    // Update and set the alpha texture
    //

    d3d_device->SetTexture( 3, bt_dst->Atexture );

    //
    // turn on our pixel shader
    //

    d3d_device->SetPixelShader( YCrCbAToRGBA );

    goto do_alpha;
  }
  else
  {
    //
    // turn on our pixel shader
    //

    d3d_device->SetPixelShader( YCrCbToRGBNoPixelAlpha );
  }

  //
  // are we completely opaque or somewhat transparent?
  //

  if ( alpha_level >= 0.999f )
  {
    d3d_device->SetRenderState( D3DRS_ALPHABLENDENABLE, 0 );
  }
  else
  {
  do_alpha:
    d3d_device->SetRenderState( D3DRS_ALPHABLENDENABLE, 1 );
    d3d_device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    d3d_device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  }

  //
  // Setup up the vertices.
  //

  d3d_device->SetVertexDeclaration( vertex_decl );

  vertices[ 0 ].sx = x_offset;
  vertices[ 0 ].sy = y_offset;
  vertices[ 0 ].sz = 0.0f;
  vertices[ 0 ].rhw = 1.0f;
  vertices[ 0 ].tu = 0.0f;
  vertices[ 0 ].tv = 0.0f;
  vertices[ 1 ] = vertices[ 0 ];
  vertices[ 1 ].sx = x_offset + ( ( (F32)(S32) width ) * x_scale );
  vertices[ 1 ].tu = 1.0f;
  vertices[ 2 ] = vertices[0];
  vertices[ 2 ].sy = y_offset + ( ( (F32)(S32) height ) * y_scale );
  vertices[ 2 ].tv = 1.0f;
  vertices[ 3 ] = vertices[ 1 ];
  vertices[ 3 ].sy = vertices[ 2 ].sy;
  vertices[ 3 ].tv = 1.0f;

  //
  // Draw the vertices.
  //

  d3d_device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( vertices[ 0 ] ) );

  //
  // clear the textures
  //

  d3d_device->SetTexture( 0, 0 );
  d3d_device->SetTexture( 1, 0 );
  d3d_device->SetTexture( 2, 0 );
  d3d_device->SetTexture( 3, 0 );
}

