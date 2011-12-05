//
// Some basic vs11 shaders used across several fx files
//
#ifndef INCLUDE_BASIC_SHADERS
#define INCLUDE_BASIC_SHADERS

//////////////////////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
half4 CombinePointLights( float3 vPosition, float3 Normal, int nPointLights, int nLightStart, uniform int nSpace )
{
	// CML 2007.03.13 - This function has been gutted.
	
	return half4(1,0,0,1);
/*
	half4 lightColor = 0; 
	for ( int i = nLightStart; i < nPointLights; i++ )
	{
		lightColor += GetPointLightColor( vPosition, PointLightPosition_Object[ i ], Normal, PointLightColor[ i ], PointLightFalloff[ i ]);
	}
	return lightColor;
*/
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
// PS11_SPECULAR - does specular lighting for _PS_1_1
//--------------------------------------------------------------------------------------------
struct PS11_SPECULAR_INPUT
{
    float4 Position				: POSITION; //in projection space
    float4 LightColor			: COLOR0; 
    float4 Diffuse				: COLOR1; 
    float2 DiffuseMap 			: TEXCOORD0; 
    float3 HalfAngleVector		: TEXCOORD1; //in tangent space
    float3 Normal				: TEXCOORD2; //in tangent space
	float Fog					: FOG;
};

float4 PS11_SPECULAR( PS11_SPECULAR_INPUT In ) : COLOR
{
    //fetch base Color
	float4 Color = tex2D(DiffuseMapSampler, In.DiffuseMap);

	//expand iterated vectors to [-1,1]
	float3 Normal			= In.Normal;
	float3 halfAngleVector	= In.HalfAngleVector;

	//compute specular Color
	float specular = saturate(dot(Normal, halfAngleVector));
	float specular2 = specular * specular;
	float specular4 = specular2 * specular2;
	float specular8 = specular4 * specular4;
	specular = specular8 * specular8;

	// AlphaConstant should have a 1.0f alpha
	return gvAlphaConstant + Color * (In.Diffuse + In.LightColor * ( specular * Color.wwww )); 
}


#endif
