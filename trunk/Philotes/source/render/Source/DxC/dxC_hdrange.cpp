//----------------------------------------------------------------------------
// dxC_hdrange.cpp
//
// - Implementation for DX-specific HDR
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_definition.h"
#include "dxC_texture.h"
#include "dxC_settings.h"
#include "dxC_hdrange.h"
#include "dxC_caps.h"
#include "dxC_debug.h"
#include "dxC_state.h"
#include "filepaths.h"
#include "dxC_target.h"
#include "dxC_device.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

HDR_MODE		geHDRMode					= HDR_MODE_NONE;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

namespace HDRScene
{

    //--------------------------------------------------------------------------------------
    // Data Structure Definitions
    //--------------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------------
    // Namespace-level variables
    //--------------------------------------------------------------------------------------

    SPD3DCTEXTURE2D				g_pTexScene             = NULL;             // The main, floating point, render target
    DXC_FORMAT					g_fmtHDR                = D3DFMT_UNKNOWN;   // Enumerated and either set to 128 or 64 bit

    //--------------------------------------------------------------------------------------
    // Function Prototypes
    //--------------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------------
    //  CreateResources( )
    //
    //      DESC:
    //          This function creates all the necessary resources to render the HDR scene
    //          to a render target for later use. When this function completes successfully
    //          rendering can commence. A call to 'DestroyResources()' should be made when
    //          the application closes.
    //
    //      PARAMS:
    //          pDevice      : The current device that resources should be created with/from
    //          pDisplayDesc : Describes the back-buffer currently in use, can be useful when
    //                         creating GUI based resources.
    //
    //      NOTES:
    //          n/a
    //--------------------------------------------------------------------------------------
    PRESULT CreateResources( const D3DC_TEXTURE2D_DESC* pDisplayDesc )
    {
		if ( ! ( e_GetValidatedHDRMode() == HDR_MODE_LINEAR_FRAMEBUFFER ) )
			return S_FALSE;

		// TODO CML: support this in DX10
		DX10_BLOCK( return E_NOTIMPL; )

#ifdef ENGINE_TARGET_DX9

		ASSERT_RETFAIL( dxC_GetDevice() );

        //[ 1 ] DETERMINE FP TEXTURE SUPPORT
        //----------------------------------
		g_fmtHDR = (DXC_FORMAT)dx9_CapGetValue( DX9CAP_FLOAT_HDR );

        //[ 2 ] CREATE HDR RENDER TARGET
        //------------------------------
        V_DO_FAILED( dxC_GetDevice()->CreateTexture( 
                                pDisplayDesc->Width, pDisplayDesc->Height, 
                                1, D3DUSAGE_RENDERTARGET, g_fmtHDR, 
                                D3DPOOL_DEFAULT, &g_pTexScene, NULL 
                                ) )
        {

            // We couldn't create the texture - lots of possible reasons for this. Best
            // check the D3D debug output for more details.
            OutputDebugString( "HDRScene::CreateResources() - Could not create floating point render target. Examine D3D Debug Output for details.\n" );
            return E_FAIL;

        }

#endif // ENGINE_TARGET_DX9

        return S_OK;
    }



    //--------------------------------------------------------------------------------------
    //  DestroyResources( )
    //
    //      DESC:
    //          Makes sure that the resources acquired in CreateResources() are cleanly
    //          destroyed to avoid any errors and/or memory leaks.
    //
    //--------------------------------------------------------------------------------------
    PRESULT DestroyResources( )
    {
        g_pTexScene = NULL;

		return S_OK;
    }


	SPD3DCRENDERTARGETVIEW GetRenderTarget()
	{
		SPD3DCRENDERTARGETVIEW pRT = NULL;

#ifdef ENGINE_TARGET_DX9

		if ( g_pTexScene )
		{
			V( g_pTexScene->GetSurfaceLevel( 0, &pRT ) );
		}

#endif // ENGINE_TARGET_DX9

		return pRT;
	}


    //--------------------------------------------------------------------------------------
    //  CalculateResourceUsage( )
    //
    //      DESC:
    //          Based on the known resources this function attempts to make an accurate
    //          measurement of how much VRAM is being used by this part of the application.
    //
    //      NOTES:
    //          Whilst the return value should be pretty accurate, it shouldn't be relied
    //          on due to the way drivers/hardware can allocate memory.
    //
    //          Only the first level of the render target is checked as there should, by
    //          definition, be no mip levels.
    //
    //--------------------------------------------------------------------------------------
    DWORD CalculateResourceUsage( )
    {

        // [ 0 ] DECLARATIONS
        //-------------------
        DWORD usage = 0;

#ifdef ENGINE_TARGET_DX9

		// [ 1 ] RENDER TARGET SIZE
        //-------------------------
		D3DC_TEXTURE2D_DESC texDesc;
        V( g_pTexScene->GetLevelDesc( 0, &texDesc ) );
        usage += ( (texDesc.Width*texDesc.Height) * (g_fmtHDR == D3DFMT_A16B16G16R16F ? 8 : 16 ) );

#endif // ENGINE_TARGET_DX9

        return usage;
    }



    //--------------------------------------------------------------------------------------
    //  GetOutputTexture( )
    //
    //      DESC:
    //          The results of this modules rendering are used as inputs to several
    //          other parts of the rendering pipeline. As such it is necessary to obtain
    //          a reference to the internally managed texture.
    //
    //      PARAMS:
    //          pTexture    : Should be NULL on entry, will be a valid reference on exit
    //
    //      NOTES:
    //          The code that requests the reference is responsible for releasing their
    //          copy of the texture as soon as they are finished using it.
    //
    //--------------------------------------------------------------------------------------
    SPD3DCSHADERRESOURCEVIEW GetOutputTexture()
    {
        return DXC_9_10( g_pTexScene, NULL );
    }
};

namespace Luminance
{

    //--------------------------------------------------------------------------------------
    // Namespace-Level Variables
    //--------------------------------------------------------------------------------------

    static const DWORD			g_dwLumTextures         = 6;                // How many luminance textures we're creating
                                                                            // Be careful when changing this value, higher than 6 might
                                                                            // require you to implement code that creates an additional
                                                                            // depth-stencil buffer due to the luminance textures dimensions
                                                                            // being greater than that of the default depth-stencil buffer.
    SPD3DCSHADERRESOURCEVIEW	g_pTexLuminance[ g_dwLumTextures ];         // An array of the downsampled luminance textures
    D3DFORMAT					g_fmtHDR                = D3DFMT_UNKNOWN;   // Should be either G32R32F or G16R16F depending on the hardware



    //--------------------------------------------------------------------------------------
    // Private Function Prototypes
    //--------------------------------------------------------------------------------------
    PRESULT RenderToTexture( D3D_EFFECT * pEffect, EFFECT_TECHNIQUE & tTechnique );



    //--------------------------------------------------------------------------------------
    //  CreateResources( )
    //
    //      DESC:
    //          This function creates the necessary texture chain for downsampling the
    //          initial HDR texture to a 1x1 luminance texture.
    //
    //      PARAMS:
    //          pDevice      : The current device that resources should be created with/from
    //          pDisplayDesc : Describes the back-buffer currently in use, can be useful when
    //                         creating GUI based resources.
    //
    //      NOTES:
    //          n/a
    //--------------------------------------------------------------------------------------
    PRESULT CreateResources()
    {
		// TODO CML: support this in DX10
		DX10_BLOCK( return E_NOTIMPL; )

#ifdef ENGINE_TARGET_DX9

		ASSERT_RETFAIL( dxC_GetDevice() );

        //[ 0 ] DECLARATIONS
        //------------------

		g_fmtHDR = (DXC_FORMAT)dx9_CapGetValue( DX9CAP_FLOAT_HDR );
		if ( g_fmtHDR == D3DFMT_UNKNOWN )
			return E_FAIL;


        //[ 2 ] CREATE HDR RENDER TARGETS
        //-------------------------------
		int iTextureSize = 1;
		for( int i = 0; i < Luminance::g_dwLumTextures; i++ )
		{

			// Create this element in the array
			V_DO_FAILED( dxC_GetDevice()->CreateTexture( iTextureSize, iTextureSize, 1, 
												D3DUSAGE_RENDERTARGET, Luminance::g_fmtHDR, 
												D3DPOOL_DEFAULT, &Luminance::g_pTexLuminance[i], NULL ) )
			{
				// Couldn't create this texture, probably best to inspect the debug runtime
				// for more information
				WCHAR str[MAX_PATH];
				PStrPrintf(str, MAX_PATH, L"Luminance::CreateResources() : Unable to create luminance"
											   L" texture of %dx%d (Element %d of %d).\n",
											   iTextureSize,
											   iTextureSize,
											   (i+1),
											   Luminance::g_dwLumTextures );
				OutputDebugStringW( str );
				return E_FAIL;
			}

            // Increment for the next texture
            iTextureSize *= 3;
        }

#endif // ENGINE_TARGET_DX9

		return S_OK;
    }



    //--------------------------------------------------------------------------------------
    //  DestroyResources( )
    //
    //      DESC:
    //          Makes sure that the resources acquired in CreateResources() are cleanly
    //          destroyed to avoid any errors and/or memory leaks.
    //
    //--------------------------------------------------------------------------------------
    PRESULT DestroyResources( )
    {
        for( DWORD i = 0; i < Luminance::g_dwLumTextures; i++ )
            Luminance::g_pTexLuminance[ i ] = NULL;

        return S_OK;
    }



    //--------------------------------------------------------------------------------------
    //  MeasureLuminance( )
    //
    //      DESC:
    //          This is the core function for this particular part of the application, it's
    //          job is to take the previously rendered (in the 'HDRScene' namespace) HDR
    //          image and compute the overall luminance for the scene. This is done by
    //          repeatedly downsampling the image until it is only 1x1 in size. Doing it
    //          this way (pixel shaders and render targets) keeps as much of the work on
    //          the GPU as possible, consequently avoiding any resource transfers, locking
    //          and modification.
    //
    //      PARAMS:
    //          pDevice : The currently active device that will be used for rendering.
    //
    //      NOTES:
    //          The results from this function will eventually be used to compose the final
    //          image. See OnFrameRender() in 'HDRDemo.cpp'.
    //
    //--------------------------------------------------------------------------------------
    PRESULT MeasureLuminance()
    {

#ifdef ENGINE_TARGET_DX9

		//[ 0 ] DECLARE VARIABLES AND ALIASES
        //-----------------------------------
		SPD3DCSHADERRESOURCEVIEW		pSourceTex      = NULL;     // We use this texture as the input
        SPD3DCSHADERRESOURCEVIEW		pDestTex        = NULL;     // We render to this texture...
        SPD3DCRENDERTARGETVIEW			pDestSurf       = NULL;     // ... Using this ptr to it's top-level surface

		D3DC_EFFECT_VARIABLE_HANDLE hParam;




		pDestTex    = Luminance::g_pTexLuminance[ 0 ];
		dxC_DebugTextureGetShaderResource() = pDestTex;






        //[ 1 ] SET THE DEVICE TO RENDER TO THE HIGHEST
        //      RESOLUTION LUMINANCE MAP.
        //---------------------------------------------
        pSourceTex	= HDRScene::GetOutputTexture();
		pDestTex    = Luminance::g_pTexLuminance[ Luminance::g_dwLumTextures - 1 ];

		V_DO_FAILED( pDestTex->GetSurfaceLevel( 0, &pDestSurf ) )
        {

            // Couldn't acquire this surface level. Odd!
            OutputDebugString( "Luminance::MeasureLuminance( ) : Couldn't acquire surface level for hi-res luminance map!\n" );
            return E_FAIL;

        }


		D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_HDR ) );
		if ( ! pEffect || ! pEffect->pD3DEffect )
			return S_FALSE;

		EFFECT_TECHNIQUE * ptTechnique = NULL;
		TECHNIQUE_FEATURES tFeat;
		tFeat.Reset();
		tFeat.nInts[ TF_INT_INDEX ] = HDR_TECHNIQUE_LUMINANCE;
		V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );
		ASSERT_RETFAIL( ptTechnique );

		V_RETURN( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FULL, DEPTH_TARGET_NONE ) );
        V_RETURN( dxC_GetDevice()->SetRenderTarget( 0, pDestSurf ) );

		V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, NULL ) );
		V_RETURN( dxC_EffectBeginPass( pEffect, HDR_LUMINANCE_PASS_GREYSCALE ) );

		hParam = dxC_EffectGetParameterByName( pEffect, "tLuminanceSrc" );
		V_RETURN( dx9_EffectSetTexture( pEffect, hParam, (LPD3DCBASESHADERRESOURCEVIEW)pSourceTex ) );


        //[ 2 ] RENDER AND DOWNSAMPLE THE HDR TEXTURE
        //      TO THE LUMINANCE MAP.
        //-------------------------------------------

        // We need to compute the sampling offsets used for this pass.
        // A 2x2 sampling pattern is used, so we need to generate 4 offsets.
        //
        // NOTE: It is worth noting that some information will likely be lost
        //       due to the luminance map being less than 1/2 the size of the
        //       original render-target. This mis-match does not have a particularly
        //       big impact on the final luminance measurement. If necessary,
        //       the same process could be used - but with many more samples, so as
        //       to correctly map from HDR->Luminance without losing information.
        D3DXVECTOR4 offsets[4];

        // Find the dimensions for the source data
        D3DC_TEXTURE2D_DESC srcDesc;
        V( pSourceTex->GetLevelDesc( 0, &srcDesc ) );

        // Because the source and destination are NOT the same sizes, we
        // need to provide offsets to correctly map between them.
        float sU = (1.0f / static_cast< float >(srcDesc.Width));
        float sV = (1.0f / static_cast< float >(srcDesc.Height));
        
        // The last two components (z,w) are unused. This makes for simpler code, but if
        // constant-storage is limited then it is possible to pack 4 offsets into 2 float4's
        offsets[0] = D3DXVECTOR4( -0.5f * sU,  0.5f * sV, 0.0f, 0.0f );
        offsets[1] = D3DXVECTOR4(  0.5f * sU,  0.5f * sV, 0.0f, 0.0f );
        offsets[2] = D3DXVECTOR4( -0.5f * sU, -0.5f * sV, 0.0f, 0.0f );
        offsets[3] = D3DXVECTOR4(  0.5f * sU, -0.5f * sV, 0.0f, 0.0f );

		hParam = dxC_EffectGetParameterByName( pEffect, "tcLumOffsets" );

        // Set the offsets to the constant table
		V( dx9_EffectSetVectorArray( pEffect, hParam, offsets, 4 ) );

        // With everything configured we can now render the first, initial, pass
        // to the luminance textures.
        V( RenderToTexture( pEffect, *ptTechnique ) );

        // Make sure we clean up the remaining reference
        pDestSurf	= NULL;
        pSourceTex	= NULL;


        //[ 3 ] SCALE EACH RENDER TARGET DOWN
        //      The results ("dest") of each pass feeds into the next ("src")
        //-------------------------------------------------------------------
        for( int i = (Luminance::g_dwLumTextures - 1); i > 0; i-- )
        {

            // Configure the render targets for this iteration
            pSourceTex  = Luminance::g_pTexLuminance[ i ];
            pDestTex    = Luminance::g_pTexLuminance[ i - 1 ];
            V_DO_FAILED( pDestTex->GetSurfaceLevel( 0, &pDestSurf ) )
            {

                // Couldn't acquire this surface level. Odd!
                OutputDebugString( "Luminance::MeasureLuminance( ) : Couldn't acquire surface level for luminance map!\n" );
                return E_FAIL;

            }

            V_RETURN( dxC_GetDevice()->SetRenderTarget( 0, pDestSurf ) );

			V_RETURN( dxC_EffectBeginPass( pEffect, HDR_LUMINANCE_PASS_DOWNSAMPLE ) );

			hParam = dxC_EffectGetParameterByName( pEffect, "tLuminanceSrc" );
			V( dx9_EffectSetTexture( pEffect, hParam, (LPD3DCBASESHADERRESOURCEVIEW)pSourceTex ) );

            // We don't want any filtering for this pass
			// CML: ... but it was set to linear anyway, so I'm leaving it as LINEAR in the FX file for now

            // Because each of these textures is a factor of 3
            // different in dimension, we use a 3x3 set of sampling
            // points to downscale.
            D3DC_TEXTURE2D_DESC srcTexDesc;
			V( pSourceTex->GetLevelDesc( 0, &srcTexDesc ) );

            // Create the 3x3 grid of offsets
            D3DXVECTOR4 DSoffsets[9];
            int idx = 0;
            for( int x = -1; x < 2; x++ )
            {
                for( int y = -1; y < 2; y++ )
                {
                    DSoffsets[idx++] = D3DXVECTOR4(
                                            static_cast< float >( x ) / static_cast< float >( srcTexDesc.Width ),
                                            static_cast< float >( y ) / static_cast< float >( srcTexDesc.Height ),
                                            0.0f,   //unused
                                            0.0f    //unused
                                        );
                }
            }

			hParam = dxC_EffectGetParameterByName( pEffect, "tcDSOffsets" );
			V( dx9_EffectSetVectorArray( pEffect, hParam, DSoffsets, 9 ) );

            // Render the display to this texture
            V( RenderToTexture( pEffect, *ptTechnique ) );

            // Clean-up by releasing the level-0 surface
            pDestSurf = NULL;

        }


        // =============================================================
        //    At this point, the g_pTexLuminance[0] texture will contain
        //    a 1x1 texture that has the downsampled luminance for the
        //    scene as it has currently been rendered.
        // =============================================================

		DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
		V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FINAL_BACKBUFFER, eDT ) );

#endif // ENGINE_TARGET_DX9

        return S_OK;

    }



    //--------------------------------------------------------------------------------------
    //  GetLuminanceTexture( )
    //
    //      DESC:
    //          The final 1x1 luminance texture created by the MeasureLuminance() function
    //          is required as an input into the final image composition. Because of this
    //          it is necessary for other parts of the application to have access to this
    //          particular texture.
    //
    //      PARAMS:
    //          pTexture    : Should be NULL on entry, will be a valid reference on exit
    //
    //      NOTES:
    //          The code that requests the reference is responsible for releasing their
    //          copy of the texture as soon as they are finished using it.
    //
    //--------------------------------------------------------------------------------------
    SPD3DCSHADERRESOURCEVIEW GetLuminanceTexture()
    {
        return g_pTexLuminance[ 0 ];
    }



    //--------------------------------------------------------------------------------------
    //  ComputeResourceUsage( )
    //
    //      DESC:
    //          Based on the known resources this function attempts to make an accurate
    //          measurement of how much VRAM is being used by this part of the application.
    //
    //      NOTES:
    //          Whilst the return value should be pretty accurate, it shouldn't be relied
    //          on due to the way drivers/hardware can allocate memory.
    //
    //          Only the first level of the render target is checked as there should, by
    //          definition, be no mip levels.
    //
    //--------------------------------------------------------------------------------------
    DWORD ComputeResourceUsage( )
    {

        DWORD usage = 0;

#ifdef ENGINE_TARGET_DX9

		for( DWORD i = 0; i < Luminance::g_dwLumTextures; i++ )
        {

            D3DC_TEXTURE2D_DESC d;
            V( Luminance::g_pTexLuminance[ i ]->GetLevelDesc( 0, &d ) );

            usage += ( ( d.Width * d.Height ) * ( Luminance::g_fmtHDR == D3DFMT_G32R32F ? 8 : 4 ) );

        }

#endif // ENGINE_TARGET_DX9

        return usage;

    }



    //--------------------------------------------------------------------------------------
    //  RenderToTexture( )
    //
    //      DESC:
    //          A simple utility function that draws, as a TL Quad, one texture to another
    //          such that a pixel shader (configured before this function is called) can
    //          operate on the texture. Used by MeasureLuminance() to perform the
    //          downsampling and filtering.
    //
    //      PARAMS:
    //          pDevice : The currently active device
    //
    //      NOTES:
    //          n/a
    //
    //--------------------------------------------------------------------------------------
    PRESULT RenderToTexture( D3D_EFFECT * pEffect, EFFECT_TECHNIQUE & tTechnique )
    {
		/*
        D3DSURFACE_DESC         desc;
        LPDIRECT3DSURFACE9      pSurfRT;

        pDev->GetRenderTarget(0, &pSurfRT);
        pSurfRT->GetDesc(&desc);
        pSurfRT->Release();

        // To correctly map from texels->pixels we offset the coordinates
        // by -0.5f:
        float fWidth = static_cast< float >( desc.Width ) - 0.5f;
        float fHeight = static_cast< float >( desc.Height ) - 0.5f;

        // Now we can actually assemble the screen-space geometry
        Luminance::TLVertex v[4];

        v[0].p = D3DXVECTOR4( -0.5f, -0.5f, 0.0f, 1.0f );
        v[0].t = D3DXVECTOR2( 0.0f, 0.0f );

        v[1].p = D3DXVECTOR4( fWidth, -0.5f, 0.0f, 1.0f );
        v[1].t = D3DXVECTOR2( 1.0f, 0.0f );

        v[2].p = D3DXVECTOR4( -0.5f, fHeight, 0.0f, 1.0f );
        v[2].t = D3DXVECTOR2( 0.0f, 1.0f );

        v[3].p = D3DXVECTOR4( fWidth, fHeight, 0.0f, 1.0f );
        v[3].t = D3DXVECTOR2( 1.0f, 1.0f );

        // Configure the device and render..
        pDev->SetVertexShader( NULL );
        pDev->SetFVF( Luminance::FVF_TLVERTEX );
        pDev->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, v, sizeof( Luminance::TLVertex ) );
		*/

		V_RETURN( dxC_DrawFullscreenQuad( FULLSCREEN_QUAD_UV, pEffect, tTechnique ) );

        return S_OK;

    }
};

namespace HDRPipeline
{
	PRESULT CreateResources()
	{
		if ( ! ( e_GetValidatedHDRMode() == HDR_MODE_LINEAR_FRAMEBUFFER ) )
			return S_FALSE;

		// TODO CML: support this in DX10
		DX10_BLOCK( return E_NOTIMPL; )

#ifdef ENGINE_TARGET_DX9

		ASSERT_RETFAIL( dxC_GetDevice() );

		D3DC_TEXTURE2D_DESC tDisplayDesc;
		ZeroMemory( &tDisplayDesc, sizeof(D3DC_TEXTURE2D_DESC) );
		RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER );
		V_RETURN( dxC_GetRenderTargetDimensions( (int&)tDisplayDesc.Width, (int&)tDisplayDesc.Height, nRT ) );

		V_RETURN( HDRScene::CreateResources( &tDisplayDesc ) );
		V_RETURN( Luminance::CreateResources() );

#endif // ENGINE_TARGET_DX9

		return S_OK;
	}


	PRESULT DestroyResources( )
	{
		V( HDRScene::DestroyResources() );
		V( Luminance::DestroyResources() );

		return S_OK;
	}


	PRESULT ApplyLuminance()
	{
#ifdef ENGINE_TARGET_DX9

		DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
		V_RETURN( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FINAL_BACKBUFFER, eDT ) );

		D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_HDR ) );
		if ( ! pEffect || ! pEffect->pD3DEffect )
			return S_FALSE;

		EFFECT_TECHNIQUE * ptTechnique = NULL;
		TECHNIQUE_FEATURES tFeat;
		tFeat.Reset();
		tFeat.nInts[ TF_INT_INDEX ] = HDR_TECHNIQUE_FINALPASS;
		V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );
		ASSERT_RETFAIL( ptTechnique );

		V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, NULL ) );

		// There's only one pass in the final pass technique
		V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

		LPD3DCSHADERRESOURCEVIEW pHDRTex = HDRScene::GetOutputTexture();
		LPD3DCSHADERRESOURCEVIEW pLumTex = Luminance::GetLuminanceTexture();

		//LPDIRECT3DTEXTURE9 pBloomTex = NULL;
		//PostProcess::GetTexture( &pBloomTex );

		D3DC_EFFECT_VARIABLE_HANDLE hParam;
		hParam = dxC_EffectGetParameterByName( pEffect, "tOriginalScene" );
		V( dx9_EffectSetTexture( pEffect, hParam, (LPD3DCBASESHADERRESOURCEVIEW)pHDRTex ) );

		hParam = dxC_EffectGetParameterByName( pEffect, "tLuminance" );
		V( dx9_EffectSetTexture( pEffect, hParam, (LPD3DCBASESHADERRESOURCEVIEW)pLumTex ) );

		hParam = dxC_EffectGetParameterByName( pEffect, "tBloom" );
		V( dx9_EffectSetTexture( pEffect, hParam, NULL ) );

		//D3DSURFACE_DESC d;
		//pBloomTex->GetLevelDesc( 0, &d );
		//g_pFinalPassPSConsts->SetFloat( pd3dDevice, "g_rcp_bloom_tex_w", 1.0f / static_cast< float >( d.Width ) );
		//g_pFinalPassPSConsts->SetFloat( pd3dDevice, "g_rcp_bloom_tex_h", 1.0f / static_cast< float >( d.Height ) );
		//g_pFinalPassPSConsts->SetFloat( pd3dDevice, "fExposure", g_fExposure );
		hParam = dxC_EffectGetParameterByName( pEffect, "fExposure" );
		V( dx9_EffectSetFloat( pEffect, hParam, 1.0f ) );
		//g_pFinalPassPSConsts->SetFloat( pd3dDevice, "fGaussianScalar", PostProcess::GetGaussianMultiplier( ) );
		hParam = dxC_EffectGetParameterByName( pEffect, "fGaussianScalar" );
		V( dx9_EffectSetFloat( pEffect, hParam, 0.0f ) );

		V_RETURN( dxC_DrawFullscreenQuad( FULLSCREEN_QUAD_UV, pEffect, *ptTechnique ) );

#endif // ENGINE_TARGET_DX9

		return S_OK;
	}
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_TexturesReadSRGB()
{
	switch ( geHDRMode )
	{
	case HDR_MODE_LINEAR_FRAMEBUFFER:
	case HDR_MODE_LINEAR_SHADERS:
		return TRUE;
	case HDR_MODE_SRGB_LIGHTS:
	case HDR_MODE_NONE:
	default:
		return FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ColorsTranslateSRGB()
{
	switch ( geHDRMode )
	{
	case HDR_MODE_LINEAR_FRAMEBUFFER:
	case HDR_MODE_LINEAR_SHADERS:
		return TRUE;
	case HDR_MODE_SRGB_LIGHTS:
	case HDR_MODE_NONE:
	default:
		return FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_IsHDRModeSupported( HDR_MODE eMode )
{
	if ( ! IS_VALID_INDEX( eMode, NUM_HDR_MODES ) )
		return FALSE;

	BOOL bLinearIn			= (BOOL)dx9_CapGetValue( DX9CAP_LINEAR_GAMMA_SHADER_IN );
	BOOL bLinearOut			= (BOOL)dx9_CapGetValue( DX9CAP_LINEAR_GAMMA_SHADER_OUT );
	DXC_FORMAT tFPHDR		= (DXC_FORMAT)dx9_CapGetValue( DX9CAP_FLOAT_HDR );
	DXC_FORMAT tIntHDR		= (DXC_FORMAT)dx9_CapGetValue( DX9CAP_INTEGER_HDR );

	switch ( eMode )
	{
	case HDR_MODE_LINEAR_FRAMEBUFFER:
		if ( bLinearIn && ( tFPHDR != D3DFMT_UNKNOWN || tIntHDR != D3DFMT_UNKNOWN ) )
			return TRUE;
		return FALSE;
	case HDR_MODE_LINEAR_SHADERS:
		if ( bLinearIn && bLinearOut )
			return TRUE;
		return FALSE;
	case HDR_MODE_SRGB_LIGHTS:
	case HDR_MODE_NONE:
		return TRUE;
	default:
		ErrorDialog( "Invalid HDR mode detected!" );
		return FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_SetHDRMode( HDR_MODE eMode )
{
	if ( eMode == HDR_MODE_INVALID )
	{
		// find the best mode
		if ( e_IsHDRModeSupported( HDR_MODE_LINEAR_FRAMEBUFFER ) )
			geHDRMode = HDR_MODE_LINEAR_FRAMEBUFFER;
		else if ( e_IsHDRModeSupported( HDR_MODE_LINEAR_SHADERS ) )
			geHDRMode = HDR_MODE_LINEAR_SHADERS;
		else if ( e_IsHDRModeSupported( HDR_MODE_SRGB_LIGHTS ) )
			geHDRMode = HDR_MODE_SRGB_LIGHTS;
		else
			geHDRMode = HDR_MODE_NONE;
	}
	else
	{
		// validate this mode
		if ( e_IsHDRModeSupported( eMode ) )
			geHDRMode = eMode;
		else
			return FALSE;
	}

	V( dxC_RestoreHDRMode() );
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

HDR_MODE e_GetValidatedHDRMode()
{
	if ( ! e_IsHDRModeSupported( geHDRMode ) )
	{
		return HDR_MODE_NONE;
	}
	return geHDRMode;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_RestoreHDRMode()
{
	if ( ! dxC_GetDevice() )
		return S_FALSE;

	// this will do stuff related to setting up for the current HDR mode
	switch ( geHDRMode )
	{
	case HDR_MODE_SRGB_LIGHTS:
		{
			DX9VERIFY( dxC_SetRenderState( D3DRS_SRGBWRITEENABLE, FALSE ) );
		}
		break;
	case HDR_MODE_LINEAR_SHADERS:
		{
			DX9VERIFY( dxC_SetRenderState( D3DRS_SRGBWRITEENABLE, TRUE ) );
		}
		break;
	case HDR_MODE_LINEAR_FRAMEBUFFER:
		{
			DX9VERIFY( dxC_SetRenderState( D3DRS_SRGBWRITEENABLE, FALSE ) );
		}
		break;
	case HDR_MODE_NONE:
	default:
		{
			DX9VERIFY( dxC_SetRenderState( D3DRS_SRGBWRITEENABLE, FALSE ) );
		}
		break;
	}

	return S_OK;
}
