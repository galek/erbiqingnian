#include "../../source/Dx9/dx9_shadowtypeconstants.h"

#ifdef ENGINE_TARGET_DX10
#ifndef DX10_EMULATE_DX9_BEHAVIOR
#define DX10_PCSS_SM
#endif
#endif

#ifdef DX10_PCSS_SM
// -------------------------------------
// STEP 1: Search for potential blockers
// -------------------------------------
void findBlocker( out float avgBlockerDepth, out float numBlockers, TEXTURE2D_LOCAL shadowMapTex, float3 lightPos, half searchWidth )
{
        float blockerSum = 0;
        float blockerCount = 0;

		[unroll] for( int i = 0; i < POISSON_LARGE_COUNT; ++i )
		{
			float fShadMapDepth = shadowMapTex.Sample( FILTER_MIN_MAG_MIP_POINT_CLAMP, lightPos.xy + poissonDiskLarge[i] * searchWidth ).x;
			                       
			// found a blocker
            if ( fShadMapDepth < lightPos.z ) 
            {
                blockerSum += fShadMapDepth;
				blockerCount++;
			}
		}
		
		avgBlockerDepth =  blockerSum / blockerCount;
		numBlockers = blockerCount;
}

// ------------------------------------------------
// STEP 2: Estimate penumbra based on
// blocker estimate, receiver depth, and light size
// ------------------------------------------------
float estimatePenumbra( float receiver, float blocker )
{
      // estimate penumbra using parallel planes approximation
       float penumbra = (receiver - blocker) * gfPCSSLightSize.x / blocker;
       
       return penumbra;
}

// ----------------------------------------------------
// Step 3: Percentage-closer filter implementation with
// variable filter width and number of samples.
// This assumes a square filter with the same number of
// horizontal and vertical samples.
// ----------------------------------------------------

float PCF_Filter( TEXTURE2D_LOCAL shadowMapTex, float3 lightPos, half filterWidth )
{
	float sum = 0.0f;
	
	for( int i = 0; i < POISSON_LARGE_COUNT; ++i )
		sum += shadowMapTex.SampleCmpLevelZero( FILTER_PCF, lightPos.xy + poissonDiskLarge[i] * filterWidth, lightPos.z ).x;
	sum /= POISSON_LARGE_COUNT;
		
	return sum;
}

float DoPCSS( TEXTURE2D_LOCAL shadowMapTex, float4 input )
{
	//return shadowMapTex.SampleCmpLevelZero( FILTER_PCF,  input.xy, input.z );
	float3 coords = input.xyz;

   // The soft shadow algorithm follows:

   // ---------------------------------------------------------
   // Step 1: Find blocker estimate
   float fSearchSize = gfPCSSLightSize.x * coords.z;
   float avgBlockerDepth = 0.0f;
   float numBlockers = 0.0f;
   findBlocker( avgBlockerDepth, numBlockers, shadowMapTex, coords, fSearchSize );
   
   // If we don't find a blocker forget about it
   if ( numBlockers < 1 )
		return 1.0;
   
   // ---------------------------------------------------------
   // Step 2: Estimate penumbra using parallel planes approximation
   half penumbra = 1.0f;
   penumbra = estimatePenumbra( coords.z, avgBlockerDepth );
   penumbra = min( penumbra, gfPCSSLightSize.y );  //Don't go outside the area we searched

   // ---------------------------------------------------------
   // Step 3: Compute percentage-closer filter
   // based on penumbra estimate
   // Now do a penumbra-based percentage-closer filter
   return PCF_Filter( shadowMapTex, coords, penumbra );
}
#endif

void ComputeShadowCoords( out half4 coords, half3 objPos, uniform bool bExtraMap = false )
{
	coords = mul( half4( objPos, 1.0f ), ( bExtraMap ) ? gmShadowMatrix2 : gmShadowMatrix );
}

float ComputeLightingAmount( float4 fragmentPos, uniform bool bExtraMap = false, uniform bool bStatic = false )
{
#ifdef ENGINE_TARGET_DX10
	#ifdef DX10_PCSS_SM
		if( !bStatic )
			return DoPCSS( ( bExtraMap ) ? tShadowMapDepth : tShadowMap, fragmentPos );
		else
	#endif
		if ( bExtraMap )
			return tShadowMapDepth.SampleCmpLevelZero( FILTER_PCF,  fragmentPos.xy, fragmentPos.z );
		else
			return tShadowMap.SampleCmpLevelZero( FILTER_PCF,  fragmentPos.xy, fragmentPos.z );
#else
	return tex2D( ( bExtraMap ) ? ShadowMapDepthSampler : ShadowMapSampler, fragmentPos.xy ).r;
#endif
}


float ComputeLightingAmount_Color_FromDistance( float4 vLightSpacePosition, float fShadowMapDistance )
{
  	float fDistanceToLight = vLightSpacePosition.z / vLightSpacePosition.w;
  	const float fShadowMapBias = 0.0f;
	float fLight = ((fDistanceToLight - fShadowMapDistance) > fShadowMapBias) ? 0.0f : 1.0f;
	return fLight;
}

float ComputeLightingAmount_Color_PCF4( sampler samplerShadowMap, float4 vLightSpacePosition, float fTextureSize, float fTexelSize /* fTexelSize = 1 / fTextureSize */)
{
	float2 ShadowMapUV = vLightSpacePosition.xy / vLightSpacePosition.w;
  	float fDistanceToLight = vLightSpacePosition.z / vLightSpacePosition.w;
  	const float fShadowMapBias = 0.0f;

	// Texel grid:
	//	AB
	//	CD
	float fShadowMapDistanceA = tex2D(samplerShadowMap, ShadowMapUV + float2(-fTexelSize, -fTexelSize)).x;
	float fShadowMapDistanceB = tex2D(samplerShadowMap, ShadowMapUV + float2(0, -fTexelSize)).x;
	float fShadowMapDistanceC = tex2D(samplerShadowMap, ShadowMapUV + float2(-fTexelSize, 0)).x;
	float fShadowMapDistanceD = tex2D(samplerShadowMap, ShadowMapUV).x;
	
	float fLightA = ((fDistanceToLight - fShadowMapDistanceA) > fShadowMapBias) ? 0 : 1;
	float fLightB = ((fDistanceToLight - fShadowMapDistanceB) > fShadowMapBias) ? 0 : 1;
	float fLightC = ((fDistanceToLight - fShadowMapDistanceC) > fShadowMapBias) ? 0 : 1;
	float fLightD = ((fDistanceToLight - fShadowMapDistanceD) > fShadowMapBias) ? 0 : 1;

#if 1
	// Convert sample UV into texel coordinates.
	// Offset by 0.5 because sampling a 1x1 texel area centered
	// around 0,0 takes 25% from each of the 4 adjacent texels.
	float2 vFracs = frac(fTextureSize * ShadowMapUV /*+ 0.5*/);
	// Lighting amount is computed by figuring the percentage area
	// of each adjacent texel that contributes to the 1x1 texel
	// sample, and multiplying that by the source texel's value.
	// It goes like this:
	//	Total = (1 - fy)(1 - fx)A + (1 - fy)(fx)B + (fy)(1 - fx)C + (fy)(fx)D
	// where fx is the texel fraction in the X direction, and
	// fy is the texel fraction in the Y direction.
	// Factor out the common Y components for
	//	Total = (1 - fy)[(1 - fx)A + (fx)B] + (fy)[(1 - fx)C + (fx)D]
	// Note that for the pattern
	//	(1 - k)a + kb, 0 <= k < 1
	// we can substitue lerp(a, b, k)
	float fLight = lerp(lerp(fLightA, fLightB, vFracs.x), lerp(fLightC, fLightD, vFracs.x), vFracs.y);
#else
	float fLight = (fLightA + fLightB + fLightC + fLightD) * 0.25;
#endif
	
	return fLight;
}



float ComputeLightingAmount_Color( float4 vLightSpacePosition, uniform bool bExtraMap = false, uniform bool bStatic = false )
{
#ifdef ENGINE_TARGET_DX10
	return ComputeLightingAmount( vLightSpacePosition, bExtraMap, bStatic );
#else
#if 1
	return ComputeLightingAmount_Color_PCF4( 
		( bExtraMap ) ? ExtraColorShadowMapSampler : ColorShadowMapSampler, 
		vLightSpacePosition, gvShadowSize.x, gvShadowSize.z );
#else
	float fShadowMapDistance = tex2D( ( bExtraMap ) ? ExtraColorShadowMapSampler : ColorShadowMapSampler, vLightSpacePosition.xy ).x;
	return ( fShadowMapDistance >= 1.0f ) ? 1.0f : ComputeLightingAmount_Color_FromDistance(vLightSpacePosition, fShadowMapDistance);
#endif
#endif
}
