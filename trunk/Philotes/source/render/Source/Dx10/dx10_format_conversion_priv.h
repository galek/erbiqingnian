//private header file for dx10_format_conversion.h, helps simplify/inline conversion code

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#undef CASE_FORMAT
#if defined( DX10_FORMAT_CONVERSION_DSV )
	#define CASE_FORMAT( resfmt, SRVfmt, RTVfmt, DSVfmt ) case resfmt : return DSVfmt;
#elif defined( DX10_FORMAT_CONVERSION_RTV )
	#define CASE_FORMAT( resfmt, SRVfmt, RTVfmt, DSVfmt ) case resfmt : return RTVfmt;
#elif defined( DX10_FORMAT_CONVERSION_SRV )
	#define CASE_FORMAT( resfmt, SRVfmt, RTVfmt, DSVfmt ) case resfmt : return SRVfmt;
#else
	#define CASE_FORMAT( resfmt, SRVfmt, RTVfmt, DSVfmt ) 
#endif

//----------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------
CASE_FORMAT( DXGI_FORMAT_R24G8_TYPELESS,	DXGI_FORMAT_R24_UNORM_X8_TYPELESS,	DXGI_FORMAT_R24_UNORM_X8_TYPELESS,	DXGI_FORMAT_D24_UNORM_S8_UINT )
CASE_FORMAT( DXGI_FORMAT_R32_TYPELESS,		DXGI_FORMAT_R32_FLOAT,				DXGI_FORMAT_R32_FLOAT,				DXGI_FORMAT_D32_FLOAT )
CASE_FORMAT( DXGI_FORMAT_R16_TYPELESS,		DXGI_FORMAT_R16_UNORM,				DXGI_FORMAT_R16_UNORM,				DXGI_FORMAT_D16_UNORM )
CASE_FORMAT( DXGI_FORMAT_R16G16_TYPELESS,	DXGI_FORMAT_R16G16_FLOAT,			DXGI_FORMAT_R16G16_FLOAT,			DXGI_FORMAT_R16G16_FLOAT )