//--------------------------------------------------------------------------------------------

struct PS20_ACTOR_INPUT
{
	float4 Position		: POSITION;
	float2 DiffuseMap	: TEXCOORD_(SAMPLER_DIFFUSE);	// CHB 2007.01.04
	float4 Ambient		: COLOR0;
};

//--------------------------------------------------------------------------------------------

PS20_ACTOR_INPUT VS20_ACTOR( VS_ANIMATED_INPUT In, uniform bool bStatic, uniform bool bLegacy )
{
	PS20_ACTOR_INPUT Out = ( PS20_ACTOR_INPUT ) 0;

	float4 Position;

	if (bStatic)
	{
		Position = float4( In.Position, 1.0f );
	}
	else
	{
		int4 Indices;
		if (bLegacy)
		{
			// A D3DCOLOR was used instead of UBYTE4 because some
			// 1.1 cards don't support the UBYTE4 type.
			// This adds one instruction to scale the color
			// components (0 <= x <= 1) by approximately 255
			// to arrive at the original values (0 <= x <= 255).
			Indices = D3DCOLORtoUBYTE4(In.Indices);
		}
		else
		{
			Indices = In.Indices;
		}
		GetPositionEx(Position, In, Indices);
	}

	Out.Position	=  mul( Position, WorldViewProjection );
	Out.DiffuseMap  =  In.DiffuseMap;
	Out.Ambient		=  GetAmbientColor();

	return Out;
}

//--------------------------------------------------------------------------------------------

float4 PS20_ACTOR ( PS20_ACTOR_INPUT In, uniform bool bOpacityMap ) : COLOR
{
	half4 Color;

	if( In.Ambient.g == 0 )
	{
		Color = half4( 0.25f, 0.25f, 0.3f, 1.f );
	}
	else
	{
		Color = half4( 0.35f, 0.25f, 0.25f, 1.f );
	}

	float Alpha = tex2D( DiffuseMapSampler, In.DiffuseMap ).a;
	if( Alpha < .75f )
		Color.a = 0.f;

	// There is a special "visibility" value placed in just the red channel of the Ambient.
	Color *= In.Ambient.r;
	
	return Color;
}

//--------------------------------------------------------------------------------------------

#define OCCLUSIONMAP_TECHNIQUES(metavs, metaps, vsver, psver, legacy) \
technique OcclusionMap< int VSVersion = metavs; int PSVersion = metaps; int Index = 0; > \
{ \
	pass P0 \
	{ \
		VertexShader = compile vsver VS20_ACTOR( false, legacy ); \
		PixelShader  = compile psver PS20_ACTOR( true ); \
	}  	\
} \
technique OcclusionMap_Static< int VSVersion = metavs; int PSVersion = metaps; int Index = 1; > \
{ \
	pass P0 \
	{ \
		VertexShader = compile vsver VS20_ACTOR( true, legacy ); \
		PixelShader  = compile psver PS20_ACTOR( true ); \
	}  	\
} \
