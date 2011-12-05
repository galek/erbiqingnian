// Copyright (c) 2003 Flagship Studios, Inc.
// 
//

//
// Functions for scrolling UV channels
//

// must include _Common.fx before this file


#define SCROLL_CHANNEL_DIFFUSE		0
#define SCROLL_CHANNEL_NORMAL		1
// count
#define NUM_SCROLL_CHANNELS			2


// tile_u, tile_v, phase_u, phase_v
struct ScrollData
{
	float2 fTile;
	float2 fPhase;
};
ScrollData gfScrollTextures[ NUM_SCROLL_CHANNELS ];

half gfScrollActive[ NUM_SCROLLING_SAMPLER_TYPES ];


float2 ScrollTextureCoords( half2 fCoords, int nChannel )
{
	return ( gfScrollTextures[ nChannel ].fPhase + fCoords ) * gfScrollTextures[ nChannel ].fTile;
}

half2 ScrollTextureCoordsHalf( half2 fCoords, int nChannel )
{
	return ( half2( gfScrollTextures[ nChannel ].fPhase ) + fCoords ) * half2( gfScrollTextures[ nChannel ].fTile );
}

float2 ScrollTextureCoordsCustom( half2 fCoords, int nTileChannel, int nPhaseChannel )
{
	return ( gfScrollTextures[ nPhaseChannel ].fPhase + fCoords ) * gfScrollTextures[ nTileChannel ].fTile;
}



half2 AdjustUV(
	uniform int nSampler,
	in half2 UV_base,
	in half2 UV_scroll,
	uniform int bScroll )
{
	if ( bScroll )
		return lerp( UV_base, UV_scroll, gfScrollActive[ nSampler ] );
	else
		return UV_base;
}

float2 AdjustUV_float(
	uniform int nSampler,
	in float2 UV_base,
	in float2 UV_scroll,
	uniform int bScroll )
{
	if ( bScroll )
		return lerp( UV_base, UV_scroll, gfScrollActive[ nSampler ] );
	else
		return UV_base;
}