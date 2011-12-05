//----------------------------------------------------------------------------
// dx10_caps.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_caps.h"
#include "syscaps.h"

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------
extern D3DCAPS9 gtD3DDeviceCaps;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
DXGI_ADAPTER_DESC adapterDesc;


//-------------------------------------------------------------------------------------------------

static DWORD sSupportsCoverageSampling()
{
	if ( dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID ) != DT_NVIDIA_VENDOR_ID )
		return 0;

	return 1;  //All Dx10 NVIDIA GPUs support CSAA
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT e_GatherPlatformCaps()
{
	dxC_DetectAdapter( dxC_GetAdapter() );

	e_CapSetValue( CAP_PHYSICAL_VIDEO_MEMORY_MB,			SysCaps_Get().dwPhysicalVideoMemoryBytes / BYTES_PER_MB );
	e_CapSetValue( CAP_SYSTEM_MEMORY_MB,					(DWORD)( SysCaps_Get().dwlPhysicalSystemMemoryBytes / (DWORDLONG)BYTES_PER_MB ) );
	e_CapSetValue( CAP_CPU_SPEED_MHZ,						SysCaps_Get().dwCPUSpeedMHz );
	e_CapSetValue( CAP_NATIVE_RESOLUTION_WIDTH,				SysCaps_Get().dwNativeResolutionWidth );
	e_CapSetValue( CAP_NATIVE_RESOLUTION_HEIGHT,			SysCaps_Get().dwNativeResolutionHeight );

	if ( ! dxC_GetDevice() )
	{
		dx9_PlatformCapSetValue( DX9CAP_GPU_VENDOR_ID,		0 );
	}
	else
	{
		DXGI_ADAPTER_DESC adapterDesc;
		V_RETURN( dxC_GetAdapter()->GetDesc(&adapterDesc) );

		dx9_PlatformCapSetValue( DX9CAP_GPU_VENDOR_ID,						adapterDesc.VendorId );
	}

	if ( ! dxC_GetDevice() )
		return S_FALSE;

	//NUTTAPONG TODO: Fill in DX10.0 "caps"
	//sEffectDetectMaxShaderLevels();


	// order is important here -- some caps rely on the results of other caps

	dx9_PlatformCapSetValue( DX9CAP_GPU_MULTIPLE_CARDS_MODE,			FALSE );
	dx9_PlatformCapSetValue( DX9CAP_MAX_ZBUFFER_DEPTH_BITS,				32 );
	dx9_PlatformCapSetValue( DX9CAP_3D_HARDWARE_ACCELERATION,			1 );
	dx9_PlatformCapSetValue( DX9CAP_HW_SHADOWS,							TRUE );
	dx9_PlatformCapSetValue( DX9CAP_NULL_RENDER_TARGET,					TRUE );
	dx9_PlatformCapSetValue( DX9CAP_DEPTHBIAS,							TRUE );
	dx9_PlatformCapSetValue( DX9CAP_SLOPESCALEDEPTHBIAS,				TRUE );
	dx9_PlatformCapSetValue( DX9CAP_MAX_VS_VERSION,						D3DVS_VERSION(4, 0));
	dx9_PlatformCapSetValue( DX9CAP_MAX_PS_VERSION,						D3DPS_VERSION(4, 0));
	dx9_PlatformCapSetValue( DX9CAP_MAX_VS_CONSTANTS,					4096);
	//dx9_PlatformCapSetValue( DX9CAP_MAX_PS_CONSTANTS,					tDeviceCaps. );
	dx9_PlatformCapSetValue( DX9CAP_MAX_VS_STATIC_BRANCH,				64);
	dx9_PlatformCapSetValue( DX9CAP_MAX_PS_STATIC_BRANCH,				64 );
	dx9_PlatformCapSetValue( DX9CAP_MAX_VS_DYNAMIC_BRANCH,				64);
	dx9_PlatformCapSetValue( DX9CAP_MAX_PS_DYNAMIC_BRANCH,				64 );
	dx9_PlatformCapSetValue( DX9CAP_MAX_PS20_INSTRUCTIONS,				512 );
	dx9_PlatformCapSetValue( DX9CAP_MAX_TEXTURE_WIDTH,					8192 );
	dx9_PlatformCapSetValue( DX9CAP_MAX_TEXTURE_HEIGHT,					8192 );
	dx9_PlatformCapSetValue( DX9CAP_MAX_SIMUL_RENDER_TARGETS,			8 );
	dx9_PlatformCapSetValue( DX9CAP_LINEAR_GAMMA_SHADER_IN,				TRUE );
	dx9_PlatformCapSetValue( DX9CAP_LINEAR_GAMMA_SHADER_OUT,			TRUE );
	dx9_PlatformCapSetValue( DX9CAP_FLOAT_HDR,							D3DFMT_A16B16G16R16F );
	dx9_PlatformCapSetValue( DX9CAP_INTEGER_HDR,						D3DFMT_UNKNOWN ); //?
	dx9_PlatformCapSetValue( DX9CAP_HARDWARE_GAMMA_RAMP,				TRUE ); //?
	dx9_PlatformCapSetValue( DX9CAP_MINIMUM_TEXTURE_FORMATS,			TRUE );
	dx9_PlatformCapSetValue( DX9CAP_MINIMUM_RENDERTARGET_FORMATS,		TRUE );
	dx9_PlatformCapSetValue( DX9CAP_MINIMUM_DEPTH_FORMATS,				TRUE );
	dx9_PlatformCapSetValue( DX9CAP_SCISSOR_TEST,						TRUE);
	dx9_PlatformCapSetValue( DX9CAP_MINIMUM_GENERAL_CAPS,				TRUE );
	dx9_PlatformCapSetValue( DX9CAP_HARDWARE_MOUSE_CURSOR,				TRUE );
	dx9_PlatformCapSetValue( DX9CAP_MAX_POINT_SIZE,						1.0f );
	dx9_PlatformCapSetValue( DX9CAP_ANISOTROPIC_FILTERING,				TRUE );
	dx9_PlatformCapSetValue( DX9CAP_TRILINEAR_FILTERING,				TRUE );
	dx9_PlatformCapSetValue( DX9CAP_COVERAGE_SAMPLING,					sSupportsCoverageSampling() );
	dx9_PlatformCapSetValue( DX9CAP_SEPARATE_ALPHA_BLEND_ENABLE,		TRUE );

	dx9_PlatformCapSetValue( DX9CAP_MAX_POINT_SIZE,						1.0f );
	dx9_PlatformCapSetValue( DX9CAP_MAX_ACTIVE_LIGHTS,					99 );
	dx9_PlatformCapSetValue( DX9CAP_MAX_VERTEX_STREAMS,					16 );
	dx9_PlatformCapSetValue( DX9CAP_MAX_ACTIVE_RENDERTARGETS,			8 );
	dx9_PlatformCapSetValue( DX9CAP_MAX_ACTIVE_TEXTURES,				16 );
	dx9_PlatformCapSetValue( DX9CAP_OCCLUSION_QUERIES,					TRUE );
	dx9_PlatformCapSetValue( DX9CAP_SMALLEST_COLOR_TARGET,				DXGI_FORMAT_A8_UNORM );
	dx9_PlatformCapSetValue( DX9CAP_VS_POWER,							0 );
	dx9_PlatformCapSetValue( DX9CAP_PS_POWER,							0 );

	e_CapSetValue( CAP_TOTAL_ESTIMATED_VIDEO_MEMORY_MB,		SysCaps_Get().dwPhysicalVideoMemoryBytes / BYTES_PER_MB );

	return S_OK;
}

PIXEL_SHADER_VERSION dxC_CapsGetMaxPixelShaderVersion()
{
	return PS_4_0;
}

VERTEX_SHADER_VERSION dxC_CapsGetMaxVertexShaderVersion()
{
	return VS_4_0;
}

// CHB 2007.07.25
DWORD dx9_CapsGetAugmentedMaxPixelShaderVersion(void)
{
	return D3DPS_VERSION(4, 0);
}
