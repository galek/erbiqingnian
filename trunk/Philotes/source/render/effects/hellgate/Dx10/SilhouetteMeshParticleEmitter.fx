#include "_Common.fx"

#ifdef FXC10_PATH
#include "..\..\include/dx9\_AnimatedShared20.fxh"
#else
#include "dx9/_AnimatedShared20.fxh"
#endif

#include "../hellgate/Dx10/_GpuParticlesShared.fxh"

float4 ParticleGlobalColor;

//////////////////////////////////////////////////////////////////////////////////////////
// PHASE 1
////////////

struct VS_PHASE1_OUTPUT
{
    float4 Position  : POSITION;
};

VS_PHASE1_OUTPUT VS_Phase1( VS_ANIMATED_INPUT In ) 
{
    VS_PHASE1_OUTPUT Out = (VS_PHASE1_OUTPUT)0;
    
	float4 Position;
	float3 Normal;

	// TRANSFORM BONES INTO MODEL SPACE
	GetPositionAndNormal( Position, Normal, In );

	// TRANSFORM MODEL INTO WORLD SPACE
    Out.Position  = mul(  Position, World );           
	
    return Out;      
}


[maxvertexcount(3)]
void GS_Phase1( triangle VS_PHASE1_OUTPUT input[3], inout PointStream<VS_PARTICLE_SIMULATION> output )
{
#ifdef USE_SILHOUETTE
	VS_OUTPUT1 tempTriangle[3];
	tempTriangle[0] = input[0];
	tempTriangle[1] = input[1];
	tempTriangle[2] = input[2];
	
	//TEMPORARILY TRANSFORM INTO PROJECTION SPACE FOR SILHOUETTE TEST
	// ViewProjection is not being loaded properly... using seperate multiplications for now
	tempTriangle[0].Position = mul( tempTriangle[0].Position, View );
	tempTriangle[0].Position = mul( tempTriangle[0].Position, Projection );
	tempTriangle[1].Position = mul( tempTriangle[1].Position, View );
	tempTriangle[1].Position = mul( tempTriangle[1].Position, Projection );
	tempTriangle[2].Position = mul( tempTriangle[2].Position, View );
	tempTriangle[2].Position = mul( tempTriangle[2].Position, Projection );
	

	//QUAZI-SILHOUETTE TEST
	float3 ray1 = tempTriangle[1].Position.xyz - tempTriangle[0].Position.xyz;
	float3 ray2 = tempTriangle[2].Position.xyz - tempTriangle[0].Position.xyz;
	
	//perspectivespace normal
	float3 triangleNormalPerspective = cross(ray1, ray2);

	float3 negZ = float3(0,0,-1);
	
//	float seed = tempTriangle[0].Position.x + tempTriangle[0].Position.y + tempTriangle[0].Position.z;
	float silhouetteThreshhold = clamp(0,1,gpups_GetRandomValue(gpups_launchSphereRadius.fMin,gpups_launchSphereRadius.fMax, seed));
#endif // #ifdef USE_SILHOUETTE

	float seed = dot( input[ 0 ].Position, input[ 1 ].Position );	
	
	VS_PARTICLE_SIMULATION temp = (VS_PARTICLE_SIMULATION)0;
#ifdef USE_SILHOUETTE	
	if( abs(dot(negZ, normalize(triangleNormalPerspective))) < silhouetteThreshhold )
#endif // #ifdef USE_SILHOUETTE
	{				
		// Setup to calculate face normal in worldspace and triangle area
		float3 ray1a = input[1].Position.xyz - input[0].Position.xyz;
		float3 ray2a = input[2].Position.xyz - input[0].Position.xyz;
	
		float3 triangleNormalWorld = cross(ray1a,ray2a);
		float triangleArea = length(triangleNormalWorld)/2.0f;
		triangleNormalWorld = normalize(triangleNormalWorld);

		float ppms = gpups_GetRandomValue( gpupsParams.ratePPMS.fMin, gpupsParams.ratePPMS.fMax, seed );
		
		// Calculate the number of particles to spawn based on particles per second meter squared, 
		// delta time, and triangle area.
		float fParticlesToSpawn = ppms * gpupsParams.fDeltaTime * triangleArea;				

#define MAX_PARTICLES_SPAWNED	3	
#define MIN_PARTICLES_SPAWNED	1
		int nParticlesToSpawn = min( fParticlesToSpawn, MAX_PARTICLES_SPAWNED );
		if ( nParticlesToSpawn < 1 )
		{
			float randomValue = gpups_GetRandomValueNorm(seed);
			if ( fParticlesToSpawn > randomValue )
			//if ( randomValue > 0.75f )
				nParticlesToSpawn = MIN_PARTICLES_SPAWNED;
		}

		for( int nParticlesSpawned = 0; nParticlesSpawned < nParticlesToSpawn; nParticlesSpawned++ )
		{
			float offsetFromFace = gpups_GetRandomValue( gpupsParams.launchOffsetZ.fMin, 
														 gpupsParams.launchOffsetZ.fMax, seed );
	
			// Pick a random point within the triangle.
			// rA + rB + rC = 1 and rA, rB, rC >= 0			
			float rA = gpups_GetRandomValue( 0.0f, 1.0f, seed );
			float rB = gpups_GetRandomValue( 0.0f, 1.0f, seed );
			
			if ( ( rA + rB ) > 1.0f )
			{
				rA = 1.0f - rA;
				rB = 1.0f - rB;
			}
			float rC = 1.0f - rA - rB;			
			
			temp.Position = input[0].Position * rA + input[1].Position * rB + input[2].Position * rC;
			temp.Position += float4( offsetFromFace * triangleNormalWorld, 0 );
			temp.Position.w = 1.0f;
			
			temp.VelocityAndTimer.xyz = triangleNormalWorld * gpups_GetRandomValue( gpupsParams.launchSpeed.fMin, 
																					gpupsParams.launchSpeed.fMax, 
																					temp.Position.x + 
																					temp.Position.y +
																					temp.Position.z );
			temp.VelocityAndTimer.w = gpups_GetRandomValue(
											gpupsParams.duration.fMin,
											gpupsParams.duration.fMax,
													temp.Position.x + 
													temp.Position.y + 
													temp.Position.z	);
															
			temp.SeedDuration.x = frac(gpups_tRandom.SampleLevel( gpups_sRandom, input[ 0 ].Position.x * gpupsParams.fDeltaTime, 0));
 			temp.SeedDuration.y = temp.VelocityAndTimer.w;
			
			output.Append(temp);			
		}
	}
}



//////////////////////////////////////////////////////////////////////////////////////////
// PHASE 2
////////////

VS_PARTICLE_SIMULATION VS_Phase2( VS_PARTICLE_SIMULATION input)
{
	return input;
}

[maxvertexcount(1)]
void GS_Phase2(point VS_PARTICLE_SIMULATION input[1], inout PointStream<VS_PARTICLE_SIMULATION> output)
{
	//IF THE PARTICLE HAS LIFE, KEEP IT UPDATED... IF NOT, DON'T OUTPUT IT (DELETE IT FROM SYSTEM)
	if( input[0].VelocityAndTimer.w > 0.0f )
	{
		float accel = lerp( gpupsParams.zAcceleration.fMin, gpupsParams.zAcceleration.fMax, input[ 0 ].SeedDuration.x );
		//float accel = gpupsParams.zAcceleration.fMin;
	 					
		input[0].VelocityAndTimer.xyz += float3(0,0,accel*gpupsParams.fDeltaTime);
		input[0].VelocityAndTimer.w -= gpupsParams.fDeltaTime;
		
		input[0].Position.xyz += input[0].VelocityAndTimer.xyz * gpupsParams.fDeltaTime;
		
		output.Append(input[0]);
	}

}



//////////////////////////////////////////////////////////////////////////////////////////
// PHASE 3
////////////

VS_PARTICLE_SIMULATION VS_Phase3( VS_PARTICLE_SIMULATION input)
{
	//PASS THROUGH PARTICLES TO GEOMETRY SHADER
	return input;
}

struct GS_OUTPUT3
{
    float4 Position  : SV_POSITION;
    float4 Color : COLOR0;
    float2 Texture  : TEXCOORD0;
    float  Fog  : FOG;
    float3 Normal : NORMAL;    
};


[maxvertexcount(6)]
void GS_Phase3(point VS_PARTICLE_SIMULATION input[1], inout TriangleStream<GS_OUTPUT3> output)
{
	//TRANSFORM CENTER OF PARTICLE INTO PROJECTION SPACE
	float4 tempPosition;
	tempPosition = mul(input[0].Position, View);
	tempPosition = mul(tempPosition, Projection);

	float duration = input[0].SeedDuration.y; //(gpupsParams.duration.fMax + gpupsParams.duration.fMin)/2.0f;
	float life = duration - input[0].VelocityAndTimer.w;
	float t = life / duration;
	
	float scaleMin = 0.0f;	
	float scaleMax = 0.0f;	
	if( t <= gpupsParams.scaleTime.fP0 )
	{
		scaleMin = gpupsParams.scaleMin.fP0;
		scaleMax = gpupsParams.scaleMax.fP0;
	}
	else if ( t <= gpupsParams.scaleTime.fP1 )
	{
		float st = ( t - gpupsParams.scaleTime.fP0 ) / ( gpupsParams.scaleTime.fP1 - gpupsParams.scaleTime.fP0 );
		scaleMin = lerp( gpupsParams.scaleMin.fP0, gpupsParams.scaleMin.fP1, st );
		scaleMax = lerp( gpupsParams.scaleMax.fP0, gpupsParams.scaleMax.fP1, st );
	}
	else if ( t <= gpupsParams.scaleTime.fP2 )
	{
		float st = ( t - gpupsParams.scaleTime.fP1 ) / ( gpupsParams.scaleTime.fP2 - gpupsParams.scaleTime.fP1 );
		scaleMin = lerp( gpupsParams.scaleMin.fP1, gpupsParams.scaleMin.fP2, st );					    
		scaleMax = lerp( gpupsParams.scaleMax.fP1, gpupsParams.scaleMax.fP2, st );					    
	}
	else		
	{
		float st = ( t - gpupsParams.scaleTime.fP2 ) / ( gpupsParams.scaleTime.fP3 - gpupsParams.scaleTime.fP2 );
		scaleMin = lerp( gpupsParams.scaleMin.fP2, gpupsParams.scaleMin.fP3, st );
		scaleMax = lerp( gpupsParams.scaleMax.fP2, gpupsParams.scaleMax.fP3, st );
	}				
	float scale = lerp( scaleMin, scaleMax, input[0].SeedDuration.x );

	//QUAD
//	float halfwidth = lerp( gpupsParams.launchScale.fMin, gpupsParams.launchScale.fMax, input[ 0 ].SeedDuration.x ) / 2.0f;
	float halfwidth = ( gpupsParams.launchScale.fMin * scale ) / 2.0f;

	//positions
	float3 topleft	= float3(-halfwidth,halfwidth,0);
	float3 topright	= float3(halfwidth,halfwidth,0);
	float3 botleft	= float3(-halfwidth,-halfwidth,0);
	float3 botright	= float3(halfwidth,-halfwidth,0);
	
	//uv's
	float2 topleftUV = float2(0,0);
	float2 toprightUV = float2(1,0);
	float2 botleftUV = float2(0,1);
	float2 botrightUV = float2(1,1);

	//calculate alpha
	float alpha = 0.0f;	
	if( t <= gpupsParams.alphaTime.fP0 )
		alpha = gpupsParams.alpha.fP0;
	else if  ( t <= gpupsParams.alphaTime.fP1 )
		alpha = lerp( gpupsParams.alpha.fP0, gpupsParams.alpha.fP1, 
					( t - gpupsParams.alphaTime.fP0 ) / ( gpupsParams.alphaTime.fP1 - gpupsParams.alphaTime.fP0 ) );
	else if  ( t <= gpupsParams.alphaTime.fP2 )
		alpha = lerp( gpupsParams.alpha.fP1, gpupsParams.alpha.fP2, 
					( t - gpupsParams.alphaTime.fP1 ) / ( gpupsParams.alphaTime.fP2 - gpupsParams.alphaTime.fP1 ) );					
	else		
		alpha = lerp( gpupsParams.alpha.fP2, gpupsParams.alpha.fP3, 
					( t - gpupsParams.alphaTime.fP2 ) / ( gpupsParams.alphaTime.fP3 - gpupsParams.alphaTime.fP2 ) );		

	//calculate color
	float3 color = 0.0f;
	if( t <= gpupsParams.color0.fP3 )
		color = float3( gpupsParams.color0.fP0, gpupsParams.color0.fP1, gpupsParams.color0.fP2 );						  
	else if ( t <= gpupsParams.color1.fP3 )
	{
		color = lerp( float3( gpupsParams.color0.fP0, gpupsParams.color0.fP1, gpupsParams.color0.fP2 ),
					  float3( gpupsParams.color1.fP0, gpupsParams.color1.fP1, gpupsParams.color1.fP2 ),
					( t - gpupsParams.color0.fP3 ) / ( gpupsParams.color1.fP3 - gpupsParams.color0.fP3 ) );
		//color = float3(1,1,1);
	}
	else if ( t <= gpupsParams.color2.fP3 )
	{
		color = lerp( float3( gpupsParams.color1.fP0, gpupsParams.color1.fP1, gpupsParams.color1.fP2 ),
					  float3( gpupsParams.color2.fP0, gpupsParams.color2.fP1, gpupsParams.color2.fP2 ),
					( t - gpupsParams.color1.fP3 ) / ( gpupsParams.color2.fP3 - gpupsParams.color1.fP3 ) );					  
	}
	else		
		color = lerp( float3( gpupsParams.color2.fP0, gpupsParams.color2.fP1, gpupsParams.color2.fP2 ),
					  float3( gpupsParams.color3.fP0, gpupsParams.color3.fP1, gpupsParams.color3.fP2 ),
					( t - gpupsParams.color2.fP3 ) / ( gpupsParams.color3.fP3 - gpupsParams.color2.fP3 ) );					  
	
	GS_OUTPUT3 newVertex[6];
	for(unsigned int x=0; x<6; ++x)
	{
		newVertex[x] = (GS_OUTPUT3)0;		
		newVertex[x].Color = float4(color, alpha );
	}

	//LOWERLEFT FACE
	newVertex[0].Position = tempPosition + float4(botleft,0);
	newVertex[0].Texture = botleftUV;

	newVertex[2].Position = tempPosition + float4(botright,0);
	newVertex[2].Texture = botrightUV;	

	newVertex[1].Position = tempPosition + float4(topleft,0);
	newVertex[1].Texture = topleftUV;

	//UPPERRIGHT FACE

	newVertex[3].Position = tempPosition + float4(botright,0);
	newVertex[3].Texture = botrightUV;

	newVertex[4].Position = tempPosition + float4(topright,0);
	newVertex[4].Texture = toprightUV;

	newVertex[5].Position = tempPosition + float4(topleft,0);
	newVertex[5].Texture = topleftUV;
		

	output.Append(newVertex[0]);
	output.Append(newVertex[1]);
	output.Append(newVertex[2]);
	output.Append(newVertex[3]);
	output.Append(newVertex[4]);
	output.Append(newVertex[5]);
	
	output.RestartStrip();
}

SamplerState Phase3Sampler
{
	AddressU = Clamp;
	AddressV = Clamp;
	Filter = MIN_MAG_MIP_LINEAR;
};


float4 PS_Phase3( GS_OUTPUT3 input ) : SV_TARGET
{
	float4 sampledColor = tDiffuseMap.Sample( Phase3Sampler, float3(input.Texture,0) );
	input.Color.xyz *= sampledColor.xyz; // * ParticleGlobalColor;
	input.Color.w = sampledColor.w * input.Color.w;
	
	return input.Color;
	//return float4(0,1,1,1);
}



//////////////////////////////////////////////////////////////////////////////////////////
//    TECHNIQUES
//////////////

DepthStencilState Phase1n2Depth
{
	DepthEnable = FALSE;
	DepthWriteMask = 0x00000000;
};


DECLARE_TECH Phase2
	< SHADER_VERSION_44 int Index = 1; >
{
	pass p0
	{

		SetVertexShader( CompileShader( vs_4_0, VS_Phase2() ) );
		SetGeometryShader( 
			ConstructGSWithSO( CompileShader( gs_4_0, GS_Phase2() ), "POSITION.xyzw; TEXCOORD0.xyzw; TEXCOORD1.xy")
						);
		SetPixelShader( NULL );
		
		SetDepthStencilState ( Phase1n2Depth, 0);		
	}
}


BlendState Phase3Blend
{
	BlendOp = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = ONE;
	BlendEnable[0] = TRUE;
	
	//BlendEnableAlpha[0] = TRUE;
	//SrcBlendAlpha = ZERO;
	//DestlendAlpha = ONE;
	//BlendOpAlpha = ADD;
	
	
};



DepthStencilState Phase3Depth
{
	DepthEnable = TRUE;
	DepthWriteMask = 0x00000000;
};

DECLARE_TECH Phase3
	< SHADER_VERSION_44 int Index = 2; >
{
	pass p0
	{

		SetVertexShader(	CompileShader( vs_4_0, VS_Phase3() ) );
		SetGeometryShader(	CompileShader( gs_4_0, GS_Phase3() ) );
		SetPixelShader(		CompileShader( ps_4_0, PS_Phase3() ) );
		
		SetBlendState(	Phase3Blend, float4(0,0,0,0), 0xFFFFFFFF );
		SetDepthStencilState ( Phase3Depth, 0);

	}

}


DECLARE_TECH Phase1
	< SHADER_VERSION_44 int Index = 0; >
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, VS_Phase1() ) );
		SetGeometryShader( ConstructGSWithSO( 
							CompileShader( gs_4_0, GS_Phase1() ), 
								"POSITION.xyzw; TEXCOORD0.xyzw; TEXCOORD1.xy" ) );
		SetPixelShader( NULL );
		
		SetDepthStencilState ( Phase1n2Depth, 0);	
	}
}