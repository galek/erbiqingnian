
#include "_common.fx"

#define USE_OBSTACLE_TEXTURE
#define useSceneDepth
//temp sarah! #define decayNearEdges

//#define USE_GAUSSIAN_BLOCKER

//#define timestep 2
//#define maxGridDim 128

#define MAX_SENDIND_UNITS 20
float4 FocusUnitPositionsInGrid[MAX_SENDIND_UNITS];
float4 FocusUnitHeightAndRadiusInGrid[MAX_SENDIND_UNITS];
float4 focusUnitVelocities[MAX_SENDIND_UNITS];
int numFocusUnits;


//--------------------------------------------------------------------------------------
//Helper functions
//--------------------------------------------------------------------------------------


struct PS_INPUT_FLUIDSIM
{
   float4 pos : SV_Position;
   float3 cell0 : TEXCOORD0;
   float4 cell1 : TEXCOORD1;
   float4 cell2 : TEXCOORD2;
   float4 cell3 : TEXCOORD3;
   float3 cell7 : TEXCOORD7;
nointerpolation uint z : Z;
   
};

//--------------------------------------------------------------------------------------
//Vertex shader
//--------------------------------------------------------------------------------------

PS_INPUT_FLUIDSIM VS_GRID( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_FLUIDSIM output = (PS_INPUT_FLUIDSIM)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);

   output.cell0 = float3(input.Tex0.x/SimulationTextureWidth,input.Tex0.y/SimulationTextureHeight,input.Tex0.z);
   output.cell1 = float4(input.Tex1.x/SimulationTextureWidth,input.Tex1.y/SimulationTextureHeight,
                         input.Tex1.z/SimulationTextureWidth,input.Tex1.w/SimulationTextureHeight);
   output.cell2 = float4(input.Tex2.x/SimulationTextureWidth,input.Tex2.y/SimulationTextureHeight,
                         input.Tex2.z/SimulationTextureWidth,input.Tex2.w/SimulationTextureHeight);
   output.cell3 = float4(input.Tex3.x/SimulationTextureWidth,input.Tex3.y/SimulationTextureHeight,
                         input.Tex3.z/SimulationTextureWidth,input.Tex3.w/SimulationTextureHeight);
   output.cell7 = float3(input.Tex7.x, input.Tex7.y,input.Tex7.z);   
   
   output.z = input.Tex7.z;

   
   return output;
}

//--------------------------------------------------------------------------------------
//Pixel shaders
//--------------------------------------------------------------------------------------



half4 h4texRECTtrilerpVector( Texture2D Texture, float3 s )
{
	// let's try doing this with 2 bilerps
	//float szFloor = floor( s.z - 0.5 ) + 0.5;
    float szFloor = floor( s.z );
    //float4 baseOffsets = Texture_zOffset.SampleLevel( samPointClampUV, float3( szFloor/(WidthZOffset), 0,0 ), 0 );                                 
	float4 baseOffsets = Texture_zOffset.Load( int4( szFloor, 0, 0 ,0 ) );
	
	float2 bilerp0_tc = float2(  (baseOffsets.x + s.x)/(SimulationTextureWidth),
							     (baseOffsets.y + s.y)/(SimulationTextureHeight) );
	float2 bilerp1_tc = float2(  (baseOffsets.z + s.x)/(SimulationTextureWidth),
								 (baseOffsets.w + s.y)/(SimulationTextureHeight) );
								
	half4 bilerp0 = Texture.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp0_tc,0), 0 );
	half4 bilerp1 = Texture.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp1_tc,0), 0 );
#ifdef USE_OBSTACLE_TEXTURE
        bilerp0 *= (1.0-Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp0_tc,0), 0).r);
        bilerp1 *= (1.0-Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp1_tc,0), 0).r);
#endif		
	
	return lerp( bilerp0, bilerp1, (s.z - szFloor) );
}

half4 h4texRECTtrilerpVector( Texture2D Texture, float3 s, int width, int height )
{
	// let's try doing this with 2 bilerps
	//float szFloor = floor( s.z - 0.5 ) + 0.5;
    float szFloor = floor( s.z );
    //float4 baseOffsets = Texture_zOffset.SampleLevel( samPointClampUV, float3( szFloor/(WidthZOffset), 0,0 ), 0 );                                 
	float4 baseOffsets = Texture_zOffset.Load( int4( szFloor, 0, 0 ,0 ) );
	
	float2 bilerp0_tc = float2(  (baseOffsets.x + s.x)/(width),
							     (baseOffsets.y + s.y)/(height) );
	float2 bilerp1_tc = float2(  (baseOffsets.z + s.x)/(width),
								 (baseOffsets.w + s.y)/(height) );
								
	half4 bilerp0 = Texture.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp0_tc,0), 0 );
	half4 bilerp1 = Texture.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp1_tc,0), 0 );
#ifdef USE_OBSTACLE_TEXTURE
        bilerp0 *= (1.0-Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp0_tc,0), 0).r);
        bilerp1 *= (1.0-Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp1_tc,0), 0).r);
#endif		
	
	return lerp( bilerp0, bilerp1, (s.z - szFloor) );
}


float h4texRECTtrilerpScalar( Texture2D Texture, float3 s )
{
    float szFloor = floor( s.z );
	float4 baseOffsets = Texture_zOffset.Load( int4( szFloor, 0, 0 ,0 ) );
	
	float2 bilerp0_tc = float2(  (baseOffsets.x + s.x)/(SimulationTextureWidth),
							     (baseOffsets.y + s.y)/(SimulationTextureHeight) );
	float2 bilerp1_tc = float2(  (baseOffsets.z + s.x)/(SimulationTextureWidth),
								 (baseOffsets.w + s.y)/(SimulationTextureHeight) );
								
	float bilerp0 = Texture.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp0_tc,0), 0 ).r;
	float bilerp1 = Texture.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp1_tc,0), 0 ).r;
#ifdef USE_OBSTACLE_TEXTURE
        bilerp0 *= (1.0-Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp0_tc,0), 0).r);
        bilerp1 *= (1.0-Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp1_tc,0), 0).r);
#endif		
	
	return lerp( bilerp0, bilerp1, (s.z - szFloor) );
}

float h4texRECTtrilerpScalar( Texture2D Texture, float3 s, int width, int height )
{
    float szFloor = floor( s.z );
	float4 baseOffsets = Texture_zOffset.Load( int4( szFloor, 0, 0 ,0 ) );
	
	float2 bilerp0_tc = float2(  (baseOffsets.x + s.x)/(width),
							     (baseOffsets.y + s.y)/(height) );
	float2 bilerp1_tc = float2(  (baseOffsets.z + s.x)/(width),
								 (baseOffsets.w + s.y)/(height) );
								
	float bilerp0 = Texture.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp0_tc,0), 0 ).r;
	float bilerp1 = Texture.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp1_tc,0), 0 ).r;
#ifdef USE_OBSTACLE_TEXTURE
        bilerp0 *= (1.0-Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp0_tc,0), 0).r);
        bilerp1 *= (1.0-Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(bilerp1_tc,0), 0).r);
#endif		
	
	return lerp( bilerp0, bilerp1, (s.z - szFloor) );
}


#ifdef USE_GAUSSIAN_BLOCKER
bool withinBlocker (float3 pos) 
{
 //  if(  (pos.z - center.z) < impulseHeight && pos.z > center.z && 
   if( length(pos.xy - center.xy) < 10 )	
   	   return true;
   return false;	
}		
#endif




//ADVECT-----------------------------------------------------------------------------------------------

struct PS_INPUT_ADVECT
{
   float4 pos : SV_Position;
   float2 cell0 : TEXCOORD0;
   float3 cell7 : TEXCOORD7; 
   nointerpolation uint z : Z;
};


PS_INPUT_ADVECT VS_GRID_ADVECT( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_ADVECT output = (PS_INPUT_ADVECT)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);
   output.cell0 = float2(input.Tex0.x/RTWidth,input.Tex0.y/RTHeight);
   output.cell7 = float3(input.Tex7.x, input.Tex7.y,input.Tex7.z);   
   output.z = input.Tex7.z;
   return output;
}

half PS_ADVECT_BFECC( PS_INPUT_ADVECT input ) : SV_Target
{

  float2 cell0 = input.cell0;
  float3 cell7 = input.cell7;
  cell7.z = input.z;  

  float3 pos = cell7 - timestep * forward * Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 ).xyz;  //we already know that cell0.xy corresponds to the right Z slice

#ifdef USE_GAUSSIAN_BLOCKER
  if(withinBlocker(pos)) return float4(0,0,0,0);
#endif   
  
  float3 halfVolumeDim = gridDim.xyz/2.0;
  float3 diff = abs( halfVolumeDim.xyz - cell7.xyz );
  float r;

  // must use regular semi-Lagrangian advection instead of
  // BFECC at the volume boundaries
  if( (diff.x > (halfVolumeDim.x-4)) ||
      (diff.y > (halfVolumeDim.y-4)) ||
      (diff.z > (halfVolumeDim.z-4)) ) 
  {
        r = h4texRECTtrilerpScalar( Texture_color, pos );
  }
  else
  {
     r = -0.5f * h4texRECTtrilerpScalar( Texture_temp3,  pos, SimulationTextureWidth, SimulationTextureHeight );
     r = r + 1.5f * h4texRECTtrilerpScalar( Texture_color,  pos, RTWidth, RTHeight );
  }
  
  float clamp = 1.0;

  // only use this for color
  r = saturate(r);
    
  return r*modulate;
    
}


half PS_ADVECT_1( PS_INPUT_ADVECT input ) : SV_Target
{
  float2 cell0 = input.cell0;
  float3 cell7 = input.cell7;
  cell7.z = input.z; 

  float3 pos = cell7 - timestep * forward * Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 ).xyz;  //we already know that cell0.xy corresponds to the right Z slice

#ifdef USE_GAUSSIAN_BLOCKER
  if(withinBlocker(pos)) return float4(0,0,0,0);
#endif   
  

#ifdef decayNearEdges 
  float heightDiff = gridDim.z - cell7.z;
  float2 halfVolumeDim = gridDim.xy/2.0;
  float diff = halfVolumeDim.x - length( halfVolumeDim.xy - cell7.xy );
  if(heightDiff < decayGridDistance)
      modulate *=  0.5 + (0.5/decayGridDistance)*heightDiff ;
  if( diff  < decayGridDistance)
	  modulate *=  0.5 + (0.5/decayGridDistance)*diff;
#endif   

  return h4texRECTtrilerpScalar( Texture_color, pos, RTWidth, RTHeight )*modulate;
  
}

half PS_ADVECT_2( PS_INPUT_ADVECT input ) : SV_Target
{
  float2 cell0 = input.cell0;
  float3 cell7 = input.cell7;
  cell7.z = input.z;

  float3 pos = cell7 - timestep * forward * Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 ).xyz;  //we already know that cell0.xy corresponds to the right Z slice
#ifdef USE_GAUSSIAN_BLOCKER
  if(withinBlocker(pos)) return float4(0,0,0,0);
#endif  
  return h4texRECTtrilerpScalar( Texture_temp1, pos, SimulationTextureWidth, SimulationTextureHeight )*modulate;
}

half4 PS_ADVECT_VEL( PS_INPUT_ADVECT input ) : SV_Target
{
  float2 cell0 = input.cell0;
  float3 cell7 = input.cell7;
  cell7.z = input.z; 
  
  float3 pos = cell7 - timestep * forward * Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 ).xyz;        //we already know that cell0.xy corresponds to the right Z slice
 
#ifdef USE_GAUSSIAN_BLOCKER
  if(withinBlocker(pos))
	return h4texRECTtrilerpVector( Texture_velocity, pos, RTWidth, RTHeight )*-modulate;
  else
#endif   
  
  return h4texRECTtrilerpVector( Texture_velocity, pos, RTWidth, RTHeight  )*modulate;

 
  // pass-through for testing
  //return Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell0.xy,0), 0 );   
}


//VORTICITY--------------------------------------------------------------------------------------------


struct PS_INPUT_VORTICITY
{
   float4 pos : SV_Position;
   float4 cell1ST : TEXCOORD1;
   float4 cell2ST : TEXCOORD2;
   float4 cell3ST : TEXCOORD3;
   float4 cell1RT : TEXCOORD4;
   float4 cell2RT : TEXCOORD5;
   float4 cell3RT : TEXCOORD6;
   
};

PS_INPUT_VORTICITY VS_GRID_VORTICITY( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_VORTICITY output = (PS_INPUT_VORTICITY)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);

   output.cell1ST = float4(input.Tex1.x/SimulationTextureWidth,input.Tex1.y/SimulationTextureHeight,
                           input.Tex1.z/SimulationTextureWidth,input.Tex1.w/SimulationTextureHeight);
   output.cell2ST = float4(input.Tex2.x/SimulationTextureWidth,input.Tex2.y/SimulationTextureHeight,
                           input.Tex2.z/SimulationTextureWidth,input.Tex2.w/SimulationTextureHeight);
   output.cell3ST = float4(input.Tex3.x/SimulationTextureWidth,input.Tex3.y/SimulationTextureHeight,
                           input.Tex3.z/SimulationTextureWidth,input.Tex3.w/SimulationTextureHeight);
                           
   output.cell1RT = float4(input.Tex1.x/RTWidth,input.Tex1.y/RTHeight,
                           input.Tex1.z/RTWidth,input.Tex1.w/RTHeight);
   output.cell2RT = float4(input.Tex2.x/RTWidth,input.Tex2.y/RTHeight,
                           input.Tex2.z/RTWidth,input.Tex2.w/RTHeight);
   output.cell3RT = float4(input.Tex3.x/RTWidth,input.Tex3.y/RTHeight,
                           input.Tex3.z/RTWidth,input.Tex3.w/RTHeight);
                                                   
   return output;
}



half4 PS_VORTICITY( PS_INPUT_VORTICITY input ): SV_Target
{
  half4 cell1 = input.cell1ST;   // +/- dx
  half4 cell2 = input.cell2ST;   // +/- dy
  half4 cell3 = input.cell3ST;   // +/- dz

  half4 L = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 );
  half4 R = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 );
  half4 B = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 );
  half4 T = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 );
  half4 U = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 );
  half4 D = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 );

  half4 vorticity;
  vorticity.xyz = 0.5 * half3( (( T.z - B.z ) - ( U.y - D.y )) ,
                               (( U.x - D.x ) - ( R.z - L.z )) ,
                               (( R.y - L.y ) - ( T.x - B.x )) );
  return vorticity;

}

half4 PS_VORTICITY_OBSTACLES( PS_INPUT_VORTICITY input ): SV_Target
{
  
  half4 L = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1ST.xy,0), 0 );
  half4 R = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1ST.zw,0), 0 );
  half4 B = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2ST.xy,0), 0 );
  half4 T = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2ST.zw,0), 0 );
  half4 U = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3ST.xy,0), 0 );
  half4 D = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3ST.zw,0), 0 );

#ifdef USE_OBSTACLE_TEXTURE
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.xy,0), 0 ).r > 0.1 ) L = 0;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.zw,0), 0 ).r > 0.1 ) R = 0;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.xy,0), 0 ).r > 0.1 ) B = 0;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.zw,0), 0 ).r > 0.1 ) T = 0;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.xy,0), 0 ).r > 0.1 ) U = 0;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.zw,0), 0 ).r > 0.1 ) D = 0;
#endif 

  half4 vorticity;
  vorticity.xyz = 0.5 * half3( (( T.z - B.z ) - ( U.y - D.y )) ,
                               (( U.x - D.x ) - ( R.z - L.z )) ,
                               (( R.y - L.y ) - ( T.x - B.x )) );
  return vorticity;

}

//CONFINEMENT--------------------------------------------------------------------------------------------


struct PS_INPUT_CONFINEMENT
{
   float4 pos : SV_Position;
   float3 cell0 : TEXCOORD0;
   float4 cell1 : TEXCOORD1;
   float4 cell2 : TEXCOORD2;
   float4 cell3 : TEXCOORD3;
   
};

PS_INPUT_CONFINEMENT VS_GRID_CONFINEMENT( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_CONFINEMENT output = (PS_INPUT_CONFINEMENT)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);

   output.cell0 = float3(input.Tex0.x/SimulationTextureWidth,input.Tex0.y/SimulationTextureHeight,input.Tex0.z);
   output.cell1 = float4(input.Tex1.x/SimulationTextureWidth,input.Tex1.y/SimulationTextureHeight,
                         input.Tex1.z/SimulationTextureWidth,input.Tex1.w/SimulationTextureHeight);
   output.cell2 = float4(input.Tex2.x/SimulationTextureWidth,input.Tex2.y/SimulationTextureHeight,
                         input.Tex2.z/SimulationTextureWidth,input.Tex2.w/SimulationTextureHeight);
   output.cell3 = float4(input.Tex3.x/SimulationTextureWidth,input.Tex3.y/SimulationTextureHeight,
                         input.Tex3.z/SimulationTextureWidth,input.Tex3.w/SimulationTextureHeight);   
   return output;
}


half4 PS_CONFINEMENT_BFECC( PS_INPUT_CONFINEMENT input ) : SV_Target
{  
   half2 cell0 = input.cell0.xy;
   half4 cell1 = input.cell1;  // +/- dx
   half4 cell2 = input.cell2;  // +/- dy
   half4 cell3 = input.cell3; 

  half4 omega = Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 ); 

  // TODO: don't find length multiple times - do once for the entire texture
  half omegaL = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 ) );// + 0.001;
  half omegaR = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 ) );// + 0.001;
  half omegaB = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 ) );// + 0.001;
  half omegaT = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 ) );// + 0.001;
  half omegaU = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 ) );// + 0.001;
  half omegaD = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 ) );// + 0.001;

  float3 eta = 0.5 * half3( omegaR - omegaL,
                            omegaT - omegaB,
                            omegaU - omegaD );


  eta = normalize( eta + float3(0.001,0.001,0.001) );
  
  half4 force;
  force.xyz = timestep * confinementEpsilon * half3( eta.y * omega.z - eta.z * omega.y,
                                                     eta.z * omega.x - eta.x * omega.z,
                                                     eta.x * omega.y - eta.y * omega.x );
  return force;

}


half4 PS_CONFINEMENT_NORMAL( PS_INPUT_CONFINEMENT input ) : SV_Target
{   
   half2 cell0 = input.cell0.xy;
   half4 cell1 = input.cell1;  // +/- dx
   half4 cell2 = input.cell2;  // +/- dy
   half4 cell3 = input.cell3; 

  half4 omega = Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 ); 

  // TODO: don't find length multiple times - do once for the entire texture
  half omegaL = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 ) );// + 0.001;
  half omegaR = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 ) );// + 0.001;
  half omegaB = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 ) );// + 0.001;
  half omegaT = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 ) );// + 0.001;
  half omegaU = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 ) );// + 0.001;
  half omegaD = length( Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 ) );// + 0.001;

  float3 eta = 0.5 * half3( omegaR - omegaL,
                            omegaT - omegaB,
                            omegaU - omegaD );

  eta = normalize( eta + float3(0.001,0.001,0.001) );
  
  half4 force;
  force.xyz = timestep * confinementEpsilon * half3( eta.y * omega.z - eta.z * omega.y,
                                                     eta.z * omega.x - eta.x * omega.z,
                                                     eta.x * omega.y - eta.y * omega.x );
  return force;

}

//INCREASE FUNCTIONS---------------------------------------------------------------------------------------


struct PS_INPUT_INCREASE
{
   float4 pos : SV_Position;
   float2 cell0 : TEXCOORD0;
   float3 cell7 : TEXCOORD1;   
};

PS_INPUT_INCREASE VS_GRID_INCREASE( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_INCREASE output = (PS_INPUT_INCREASE)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);
   output.cell0 = float2(input.Tex0.x/RTWidth,input.Tex0.y/RTHeight);                        
   output.cell7 = float3(input.Tex7.x, input.Tex7.y,input.Tex7.z);                       
   return output;
}

half4 PS_GAUSSIAN( PS_INPUT_INCREASE input ) : SV_Target 
{
  float dist = length( input.cell7 - center.xyz ) * impulseSize;
  half4 result;
  result.rgb = splatColor;
  result.a = exp( -dist*dist );
  
#ifdef USE_OBSTACLE_TEXTURE
     if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell0.xy,0), 0 ).r > 0.1 )
    	return 0;
#endif	  
  
  return result;
}

half4 PS_GAUSSIAN_FLAT( PS_INPUT_INCREASE input ) : SV_Target 
{
  float4 result = float4(0,0,0,0);
  for(int i=0;i<numFocusUnits;i++)
  {
      float dist = length( input.cell7.xy - FocusUnitPositionsInGrid[i].xy ) * FocusUnitHeightAndRadiusInGrid[i].y;
      float alpha = exp( -dist*dist );
      result.rgb += focusUnitVelocities[i].rgb*alpha;    
      result.a += alpha;
  }
  
#ifdef USE_OBSTACLE_TEXTURE
     if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell0.xy,0), 0 ).r > 0.1 )
	   return 0;
#endif	  
  
  return result;
}

half4 PS_DENSITYTEXTURE( PS_INPUT_INCREASE input ) : SV_Target 
{
   float tex = Texture_densityPattern.Load( int4(input.cell7.x, input.cell7.y,0,0)); 
   tex *= smokeDensityModifier;
   return float4(tex,tex,tex,1);
}

half4 PS_VELOCITYTEXTURE( PS_INPUT_INCREASE input ) : SV_Target 
{
   float tex = Texture_velocityPattern.Load( int4(input.cell7.x, input.cell7.y,0,0)); 
   tex *= smokeVelocityModifier;
   return float4(tex*splatColor.x,tex*splatColor.y,tex*splatColor.z,1);
}

float4 PS_OBSTACLETEXTURE( PS_INPUT_INCREASE input ): SV_Target
{
   float tex = Texture_obstaclePattern.Load( int4(input.cell7.x, input.cell7.y,0,0)); 
   return float4(tex,tex,tex,1);
  
}


//DIVERGENCE----------------------------------------------------------------------------------

//this function is not used 
half4 PS_DIFF_DIVERGENCE( PS_INPUT_FLUIDSIM input ) : SV_Target
{
  half4 cell1 = input.cell1;
  half4 cell2 = input.cell2;
  half4 cell3 = input.cell3;

  half4 fieldL = Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 );
  half4 fieldR = Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 );
  half4 fieldB = Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 );
  half4 fieldT = Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 );
  half4 fieldU = Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 );
  half4 fieldD = Texture_velocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 );

  half4 newDivergence =  0.5 * ( ( fieldR.x - fieldL.x ) +
                       ( fieldT.y - fieldB.y ) +
                       ( fieldU.z - fieldD.z ) );
                       
  return (Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell0.xy,0), 0) - newDivergence );                     
  
   
}

struct PS_INPUT_DIVERGENCE
{
   float4 pos : SV_Position;
   float4 cell1ST : TEXCOORD1;
   float4 cell2ST : TEXCOORD2;
   float4 cell3ST : TEXCOORD3;
   float4 cell1RT : TEXCOORD4;
   float4 cell2RT : TEXCOORD5;
   float4 cell3RT : TEXCOORD6;
   
};

PS_INPUT_DIVERGENCE VS_GRID_DIVERGENCE( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_VORTICITY output = (PS_INPUT_VORTICITY)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);

   output.cell1ST = float4(input.Tex1.x/SimulationTextureWidth,input.Tex1.y/SimulationTextureHeight,
                           input.Tex1.z/SimulationTextureWidth,input.Tex1.w/SimulationTextureHeight);
   output.cell2ST = float4(input.Tex2.x/SimulationTextureWidth,input.Tex2.y/SimulationTextureHeight,
                           input.Tex2.z/SimulationTextureWidth,input.Tex2.w/SimulationTextureHeight);
   output.cell3ST = float4(input.Tex3.x/SimulationTextureWidth,input.Tex3.y/SimulationTextureHeight,
                           input.Tex3.z/SimulationTextureWidth,input.Tex3.w/SimulationTextureHeight);
                           
   output.cell1RT = float4(input.Tex1.x/RTWidth,input.Tex1.y/RTHeight,
                           input.Tex1.z/RTWidth,input.Tex1.w/RTHeight);
   output.cell2RT = float4(input.Tex2.x/RTWidth,input.Tex2.y/RTHeight,
                           input.Tex2.z/RTWidth,input.Tex2.w/RTHeight);
   output.cell3RT = float4(input.Tex3.x/RTWidth,input.Tex3.y/RTHeight,
                           input.Tex3.z/RTWidth,input.Tex3.w/RTHeight);
                                                   
   return output;
}

half4 PS_DIVERGENCE( PS_INPUT_DIVERGENCE input ) : SV_Target
{
  half4 cell1 = input.cell1ST;
  half4 cell2 = input.cell2ST;
  half4 cell3 = input.cell3ST;

  half4 fieldL = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 );
  half4 fieldR = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 );
  half4 fieldB = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 );
  half4 fieldT = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 );
  half4 fieldU = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 );
  half4 fieldD = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 );
  
  float divergence =  0.5 * ( ( fieldR.x - fieldL.x ) +
                            ( fieldT.y - fieldB.y ) +
                            ( fieldU.z - fieldD.z ) );
                            
  return float4(divergence, divergence, divergence, divergence);                          
}

half4 PS_DIVERGENCE_OBSTACLES( PS_INPUT_DIVERGENCE input ) : SV_Target
{
  half4 fieldL = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1ST.xy,0), 0 );
  half4 fieldR = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1ST.zw,0), 0 );
  half4 fieldB = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2ST.xy,0), 0 );
  half4 fieldT = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2ST.zw,0), 0 );
  half4 fieldU = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3ST.xy,0), 0 );
  half4 fieldD = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3ST.zw,0), 0 );
  
#ifdef USE_OBSTACLE_TEXTURE
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.xy,0), 0 ).r > 0.1 ) fieldL = 0;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.zw,0), 0 ).r > 0.1 ) fieldR = 0;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.xy,0), 0 ).r > 0.1 ) fieldB = 0;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.zw,0), 0 ).r > 0.1 ) fieldT = 0;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.xy,0), 0 ).r > 0.1 ) fieldU = 0;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.zw,0), 0 ).r > 0.1 ) fieldD = 0;
#endif
  

  float divergence =  0.5 * ( ( fieldR.x - fieldL.x ) +
                            ( fieldT.y - fieldB.y ) +
                            ( fieldU.z - fieldD.z ) );
                            
  return float4(divergence, divergence, divergence, divergence);                          
}


//JACOBI-------------------------------------------------------------------------------

struct PS_INPUT_JACOBI
{
   float4 pos : SV_Position;
   
   float2 cell0ST : TEXCOORD0;
   float4 cell1ST : TEXCOORD1;
   float4 cell2ST : TEXCOORD2;
   float4 cell3ST : TEXCOORD3;
   
   float2 cell0RT : TEXCOORD4;
   float4 cell1RT : TEXCOORD5;
   float4 cell2RT : TEXCOORD6;
   float4 cell3RT : TEXCOORD7;
   
};

PS_INPUT_JACOBI VS_GRID_JACOBI( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_JACOBI output = (PS_INPUT_JACOBI)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);

   output.cell0ST = float2(input.Tex0.x/SimulationTextureWidth,input.Tex0.y/SimulationTextureHeight);
   output.cell1ST = float4(input.Tex1.x/SimulationTextureWidth,input.Tex1.y/SimulationTextureHeight,
                           input.Tex1.z/SimulationTextureWidth,input.Tex1.w/SimulationTextureHeight);
   output.cell2ST = float4(input.Tex2.x/SimulationTextureWidth,input.Tex2.y/SimulationTextureHeight,
                           input.Tex2.z/SimulationTextureWidth,input.Tex2.w/SimulationTextureHeight);
   output.cell3ST = float4(input.Tex3.x/SimulationTextureWidth,input.Tex3.y/SimulationTextureHeight,
                           input.Tex3.z/SimulationTextureWidth,input.Tex3.w/SimulationTextureHeight);
                           
   output.cell0RT = float2(input.Tex0.x/RTWidth,input.Tex0.y/RTHeight);
   output.cell1RT = float4(input.Tex1.x/RTWidth,input.Tex1.y/RTHeight,
                           input.Tex1.z/RTWidth,input.Tex1.w/RTHeight);
   output.cell2RT = float4(input.Tex2.x/RTWidth,input.Tex2.y/RTHeight,
                           input.Tex2.z/RTWidth,input.Tex2.w/RTHeight);
   output.cell3RT = float4(input.Tex3.x/RTWidth,input.Tex3.y/RTHeight,
                           input.Tex3.z/RTWidth,input.Tex3.w/RTHeight);
                                                   
   return output;
}

half PS_JACOBI_1( PS_INPUT_JACOBI input ) : SV_Target
{
  half2 cell0 = input.cell0ST;
  half4 cell1 = input.cell1RT;
  half4 cell2 = input.cell2RT;
  half4 cell3 = input.cell3RT;

  half xL = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 );
  half xR = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 );
  half xB = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 );
  half xT = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 );
  half xU = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 );
  half xD = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 );

  half bC = Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 );

   return( xL + xR + xB + xT + xU + xD - bC ) /6.0; 
}

half PS_JACOBI_1_OBSTACLES( PS_INPUT_JACOBI input ) : SV_Target
{
  half xL = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.xy,0), 0 );
  half xR = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.zw,0), 0 );
  half xB = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.xy,0), 0 );
  half xT = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.zw,0), 0 );
  half xU = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.xy,0), 0 );
  half xD = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.zw,0), 0 );

#ifdef USE_OBSTACLE_TEXTURE
      half xCenter = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell0RT.xy,0), 0 );
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.xy,0), 0 ).r > 0.1 ) xL = xCenter;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.zw,0), 0 ).r > 0.1 ) xR = xCenter;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.xy,0), 0 ).r > 0.1 ) xB = xCenter;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.zw,0), 0 ).r > 0.1 ) xT = xCenter;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.xy,0), 0 ).r > 0.1 ) xU = xCenter;
      if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.zw,0), 0 ).r > 0.1 ) xD = xCenter;
#endif  

  half bC = Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell0ST.xy,0), 0 );

   return( xL + xR + xB + xT + xU + xD - bC ) /6.0; 
}


half PS_JACOBI_2( PS_INPUT_JACOBI input ) : SV_Target
{
  half2 cell0 = input.cell0ST;
  half4 cell1 = input.cell1ST;
  half4 cell2 = input.cell2ST;
  half4 cell3 = input.cell3ST;

  half xL = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 );
  half xR = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 );
  half xB = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 );
  half xT = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 );
  half xU = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 );
  half xD = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 );

  half bC = Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 );

   return( xL + xR + xB + xT + xU + xD - bC ) /6.0;  
}

half PS_JACOBI_2_OBSTACLES( PS_INPUT_JACOBI input ) : SV_Target
{
  half xL = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1ST.xy,0), 0 );
  half xR = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1ST.zw,0), 0 );
  half xB = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2ST.xy,0), 0 );
  half xT = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2ST.zw,0), 0 );
  half xU = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3ST.xy,0), 0 );
  half xD = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3ST.zw,0), 0 );

#ifdef USE_OBSTACLE_TEXTURE
	  half xCenter = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell0ST.xy,0), 0 );
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.xy,0), 0 ).r > 0.1 ) xL = xCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell1RT.zw,0), 0 ).r > 0.1 ) xR = xCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.xy,0), 0 ).r > 0.1 ) xB = xCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell2RT.zw,0), 0 ).r > 0.1 ) xT = xCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.xy,0), 0 ).r > 0.1 ) xU = xCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell3RT.zw,0), 0 ).r > 0.1 ) xD = xCenter;
#endif  

  half bC = Texture_temp1.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.cell0ST.xy,0), 0 );

   return( xL + xR + xB + xT + xU + xD - bC ) /6.0;  
}


//PROJECT---------------------------------------------------------------------------------



struct PS_INPUT_PROJECT
{
   float4 pos : SV_Position;
   float2 cell0ST : TEXCOORD0;
   float4 cell1RT : TEXCOORD1;
   float4 cell2RT : TEXCOORD2;
   float4 cell3RT : TEXCOORD3;
   
};

PS_INPUT_PROJECT VS_GRID_PROJECT( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_PROJECT output = (PS_INPUT_PROJECT)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);

   output.cell0ST = float2(input.Tex0.x/SimulationTextureWidth,input.Tex0.y/SimulationTextureHeight);
   output.cell1RT = float4(input.Tex1.x/RTWidth,input.Tex1.y/RTHeight,
                           input.Tex1.z/RTWidth,input.Tex1.w/RTHeight);
   output.cell2RT = float4(input.Tex2.x/RTWidth,input.Tex2.y/RTHeight,
                           input.Tex2.z/RTWidth,input.Tex2.w/RTHeight);
   output.cell3RT = float4(input.Tex3.x/RTWidth,input.Tex3.y/RTHeight,
                           input.Tex3.z/RTWidth,input.Tex3.w/RTHeight);
                                                   
   return output;
}

half4 PS_PROJECT( PS_INPUT_PROJECT input ): SV_Target
{
  
  half2 cell0 = input.cell0ST ; 
  half4 cell1 = input.cell1RT ;
  half4 cell2 = input.cell2RT ;
  half4 cell3 = input.cell3RT ;

  half pL = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 );
  half pR = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 );
  half pB = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 );
  half pT = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 );
  half pU = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 );
  half pD = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 );


  half4 velocity;
  velocity.xyz = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0) , 0 ).xyz -
                 0.5*modulate*half3( pR - pL, pT - pB, pU - pD );
  return velocity;              
  
}


half4 PS_PROJECT_OBSTACLES( PS_INPUT_PROJECT input ): SV_Target
{
  
  half2 cell0 = input.cell0ST ; 
  half4 cell1 = input.cell1RT ;
  half4 cell2 = input.cell2RT ;
  half4 cell3 = input.cell3RT ;

  half pL = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 );
  half pR = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 );
  half pB = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 );
  half pT = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 );
  half pU = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 );
  half pD = Texture_pressure.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 );

#ifdef USE_OBSTACLE_TEXTURE
      half pCenter = Texture_temp3.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0), 0 ); //temp sarah: why is this the only place in this function we are using Texture_temp3
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.xy,0), 0 ).r > 0.1 ) pL = pCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell1.zw,0), 0 ).r > 0.1 ) pR = pCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.xy,0), 0 ).r > 0.1 ) pB = pCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell2.zw,0), 0 ).r > 0.1 ) pT = pCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.xy,0), 0 ).r > 0.1 ) pU = pCenter;
	  if( Texture_obstacles.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell3.zw,0), 0 ).r > 0.1 ) pD = pCenter;
#endif


  half4 velocity;
  velocity.xyz = Texture_tempVelocity.SampleLevel( FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(cell0.xy,0) , 0 ).xyz -
                 0.5*modulate*half3( pR - pL, pT - pB, pU - pD);
  return velocity;              
  
}

//DRAW---------------------------------------------------------------------------------------

struct PS_INPUT_DRAW
{
   float4 pos : SV_Position;
   float2 cell0RT : TEXCOORD0;   
};

PS_INPUT_DRAW VS_GRID_DRAW( VS_FLUID_SIMULATION_INPUT input)
{
   PS_INPUT_DRAW output = (PS_INPUT_DRAW)0;
   output.pos = float4((input.Position.x*2.0/(RTWidth)) - 1.0, -(input.Position.y*2.0/(RTHeight)) + 1.0, 0.5, 1.0);
   output.cell0RT = float2(input.Tex0.x/RTWidth,input.Tex0.y/RTHeight);                                                   
   return output;
}

float4 PS_DRAW_TEXTURE( PS_INPUT_DRAW input ): SV_Target
{
  
   int textureNumber = 0;

  if( textureNumber == 0)
    return abs(Texture_color.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP,float3(input.cell0RT.x,input.cell0RT.y,0),0)); 
  else if( textureNumber == 1)
    return abs(Texture_velocity.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP,float3(input.cell0RT.x,input.cell0RT.y,0),0)); 
  else if( textureNumber == 2)
    return abs(Texture_obstacles.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP,float3(input.cell0RT.x,input.cell0RT.y,0),0));       
  else
    return abs(Texture_velocity.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP,float3(input.cell0RT.x,input.cell0RT.y,0),0)); 
}  


float4 PS_DRAW_VELOCITY( PS_INPUT_DRAW input ): SV_Target
{
   return Texture_velocity.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP,float3(input.cell0RT.x,input.cell0RT.y,0),0);
}

float4 PS_DRAW_WHITE( PS_INPUT_DRAW input ): SV_Target
{
   return float4(1,1,1,1);
}



//---------------------------------------------------------------------------------------
//volume renderer
//---------------------------------------------------------------------------------------

struct PS_INPUT_RAYDATA_BACK
{
	float4 pos          : SV_Position;
	float3  worldViewPos: TEXCOORD0;
};

struct PS_INPUT_RAYDATA_FRONT
{
    float4 pos              : SV_Position;
    float3 posInGrid        : POSITION;
    float3  worldViewPos    : TEXCOORD0;
};

struct PS_INPUT_RAYCAST
{
	float4 pos       : SV_Position;
	float3 posInGrid : TEXCOORD0;
};

struct PS_INPUT_QUAD
{
    float4 pos : SV_POSITION;              // Transformed position
    float2 tex : TEXCOORD0;
};


struct VS_OUTPUT_EDGEDETECT
{
	// There's no textureUV11 because its weight is zero.
    float4 position      : SV_Position;   // vertex position
    float2 textureUV00   : TEXCOORD0;  // kernel tap texture coords 
    float2 textureUV01   : TEXCOORD1;  // kernel tap texture coords 
    float2 textureUV02   : TEXCOORD2;  // kernel tap texture coords 
    float2 textureUV10   : TEXCOORD3;  // kernel tap texture coords 
    float2 textureUV12   : TEXCOORD4;  // kernel tap texture coords 
    float2 textureUV20   : TEXCOORD5;  // kernel tap texture coords 
    float2 textureUV21   : TEXCOORD6;  // kernel tap texture coords 
    float2 textureUV22   : TEXCOORD7;  // kernel tap texture coords 
};

// A full-screen edge detection pass to locate artifacts
VS_OUTPUT_EDGEDETECT VS_EDGEDETECT( VS_FLUID_RENDERING_INPUT input )
{
	VS_OUTPUT_EDGEDETECT output = (VS_OUTPUT_EDGEDETECT)0;
	output.position = input.Position;

	float2 texelSize = 1.0 / float2(RTWidth,RTHeight);
	float2 center = float2( (input.Position.x+1)/2.0 , 1.0 - (input.Position.y+1)/2.0 );

	// Eight nearest neighbours needed for Sobel.
	output.textureUV00 = center + float2(-texelSize.x, -texelSize.y);
	output.textureUV01 = center + float2(-texelSize.x,  0);
	output.textureUV02 = center + float2(-texelSize.x,  texelSize.y);
	
	output.textureUV10 = center + float2(0, -texelSize.y);
	output.textureUV12 = center + float2(0,  texelSize.y);
	
	output.textureUV20 = center + float2(texelSize.x, -texelSize.y);
	output.textureUV21 = center + float2(texelSize.x,  0);
	output.textureUV22 = center + float2(texelSize.x,  texelSize.y);

	return output;
}

float edgeDetectScalar(float sx, float sy, float threshold)
{
	float dist = (sx*sx+sy*sy);
	float e = (dist > threshold*ZFar)? 1: 0;
	return e;
}

// A full-screen edge detection pass to locate artifacts
// these artifacts are located on a downsized version of the rayDataTexture
// We use a smaller texture both to accurately find all the depth artifacts when raycasting to this smaller size 
// and to save on the cost of this pass
float4 PS_EDGEDETECT(VS_OUTPUT_EDGEDETECT vIn) : SV_Target
{
    // We need eight samples (the centre has zero weight in both kernels).
    float4 col;
    col = Texture_raydataSmall.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, vIn.textureUV00); 
    float g00 = col.a;
    if(col.g < 0)
        g00 *= -1;
    col = Texture_raydataSmall.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, vIn.textureUV01); 
    float g01 = col.a;
    if(col.g < 0)
        g01 *= -1;
    col = Texture_raydataSmall.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, vIn.textureUV02); 
    float g02 = col.a;
    if(col.g < 0)
        g02 *= -1;
    col = Texture_raydataSmall.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, vIn.textureUV10); 
    float g10 = col.a;
    if(col.g < 0)
        g10 *= -1;
    col = Texture_raydataSmall.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, vIn.textureUV12); 
    float g12 = col.a;
    if(col.g < 0)
        g12 *= -1;
    col = Texture_raydataSmall.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, vIn.textureUV20); 
    float g20 = col.a;
    if(col.g < 0)
        g20 *= -1;
    col = Texture_raydataSmall.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, vIn.textureUV21); 
    float g21 = col.a;
    if(col.g < 0)
        g21 *= -1;
    col = Texture_raydataSmall.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, vIn.textureUV22); 
    float g22 = col.a;
    if(col.g < 0)
        g22 *= -1;
	
	
	// Sobel in horizontal dir.
	float sx = 0;
	sx -= g00;
	sx -= g01 * 2;
	sx -= g02;
	sx += g20;
	sx += g21 * 2;
	sx += g22;
	//Sobel in vertical dir - weights are just rotated 90 degrees.
	float sy = 0;
	sy -= g00;
	sy += g02;
	sy -= g10 * 2;
	sy += g12 * 2;
	sy -= g20;
	sy += g22;

	float e = edgeDetectScalar(sx, sy, 0.01);
	return float4(e,e,e,1);

}


PS_INPUT_RAYCAST VS_RAYCAST (VS_FLUID_RENDERING_INPUT input)
{
	PS_INPUT_RAYCAST output = (PS_INPUT_RAYCAST)0;
	output.pos = mul(input.Position, WorldViewProjection);
	output.posInGrid = input.Tex;
	return output;
}	

PS_INPUT_RAYCAST VS_QUAD (VS_FLUID_RENDERING_INPUT input)
{
	PS_INPUT_RAYCAST output = (PS_INPUT_RAYCAST)0;
	output.pos = input.Position;
	float4 unprojectedPos = mul( float4( input.Position.xy, 0, 1 ), InverseProjection );
	unprojectedPos.xy *= ZNear;
    unprojectedPos.z = ZNear;
    unprojectedPos.w = 1;
	output.posInGrid = mul(unprojectedPos, InverseWorldView);
	return output;
		
    /* sarah : this is what it should be 
    output.pos = input.Position;
    output.posInGrid = mul( float4( input.Position.xy*ZNear, 0, ZNear ), InvWorldViewProjection );
    return output;
    */

}

float4 PS_QUAD_DEPTH(PS_INPUT_RAYCAST input) : SV_Target
{
	return float4(ZNear, ZNear, ZNear, ZNear);
}

float4 PS_QUAD_SOLID(PS_INPUT_RAYCAST input) : SV_Target
{
	return float4(1,0,0,1);
	//float depth = abs(Texture_depth.Load( int3(input.pos.x, input.pos.y,0)).r);
	//return float4( input.pos3d.xy,depth,1);
}

float4 PS_QUAD_COPY(PS_INPUT_RAYCAST input) : SV_Target
{
	return Texture_depth.Load( int4(input.pos.x, input.pos.y,0,0)).r;
}


PS_INPUT_RAYDATA_BACK VS_RAYDATA_BACK(VS_FLUID_RENDERING_INPUT input)
{
	PS_INPUT_RAYDATA_BACK output = (PS_INPUT_RAYDATA_BACK )0;
	output.pos = mul(input.Position, WorldViewProjection);
	output.worldViewPos = mul(input.Position, WorldView).xyz;
	return output;
}

float4 PS_VOLUME_DEPTH_BACK(PS_INPUT_RAYDATA_BACK input) : SV_Target
{

    float4 output;
    output.xyz = float3(0,-1,0); 
    
    float inputDepth = length(input.worldViewPos);
   
    float2 normalizedInputPos = float2(input.pos.x/RTWidth, input.pos.y/RTHeight);
    float SceneZ = Texture_sceneDepth.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, normalizedInputPos, 0).r;
    float2 inputPosClipSpace = float2((normalizedInputPos.x*2.0)-1.0,(normalizedInputPos.y*2.0)-1.0);
    float ratioY = SceneZ*distanceToScreenY;
    float ratioX = SceneZ*distanceToScreenX;
    SceneZ = length(float3( inputPosClipSpace.x*ratioX, inputPosClipSpace.y*ratioY, SceneZ ));
    
    output.w = min(inputDepth, SceneZ);
    return output;
}


PS_INPUT_RAYDATA_FRONT VS_RAYDATA_FRONT(VS_FLUID_RENDERING_INPUT input)
{
    PS_INPUT_RAYDATA_FRONT output = (PS_INPUT_RAYDATA_FRONT)0;
    output.pos = mul(input.Position, WorldViewProjection);
    output.posInGrid = input.Tex.xyz;
    output.worldViewPos = mul(input.Position, WorldView).xyz;
    return output;
}

#define KILL_PIXEL_XCOORD 1.0f
float4 PS_VOLUME_DEPTH_FRONT(PS_INPUT_RAYDATA_FRONT input) : SV_Target
{
    float4 output;
    
    float2 normalizedInputPos = float2(input.pos.x/RTWidth, input.pos.y/RTHeight);
    float SceneZ = Texture_sceneDepth.SampleLevel( FILTER_MIN_MAG_MIP_LINEAR_CLAMP, normalizedInputPos, 0).r;
    float2 inputPosClipSpace = float2((normalizedInputPos.x*2.0)-1.0,(normalizedInputPos.y*2.0)-1.0);
    float ratioY = SceneZ*distanceToScreenY;
    float ratioX = SceneZ*distanceToScreenX;
    SceneZ = length(float3( inputPosClipSpace.x*ratioX, inputPosClipSpace.y*ratioY, SceneZ ));

    float inputDepth = length(input.worldViewPos);

    if(SceneZ < inputDepth)
    {
        // If the scene occludes intersection point we want to kill the pixel early in PS
        return float4(KILL_PIXEL_XCOORD,0,0,0);
    }
    
    // We negate input.posInGrid because we use subtractive blending in front faces
    //  Note that we set xyz to 0 when rendering back faces
    output.xyz = -input.posInGrid;
    output.w = inputDepth;
    
    return output;
}


float4 RaySampleTrilinear( float3 O ) 
{
	float2 texCoord = float2( O.x * gridDim.x, (1-O.y) * gridDim.y );
	
	float szFloor = floor(O.z * gridDim.z); 
	float4 baseOffsets = Texture_zOffset.Load( int4( szFloor, 0, 0 ,0 ) );

	float2 texCoord0 = texCoord + float2( baseOffsets.x, baseOffsets.y); 
	texCoord0 /= float2( SimulationTextureWidth, SimulationTextureHeight);
	float sample0 = Texture_color.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(texCoord0,0), 0);
    
	float2 texCoord1 = texCoord + float2( baseOffsets.z, baseOffsets.w); 
	texCoord1 /= float2( SimulationTextureWidth, SimulationTextureHeight);
	float sample1 = Texture_color.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(texCoord1,0), 0);

	float col = lerp( sample0, sample1, (O.z * gridDim.z) - szFloor);
	return float4(col,col,col,col);
}


float4 RaySampleTrilinearOld( float3 O ) 
{

	float2 texCoord = float2( O.x * gridDim.x, (1-O.y) * gridDim.y );

	uint Z0 = O.z * gridDim.z; 
	uint xOffset0 = (Z0 % float(cols));
	uint yOffset0 = (Z0 / float(cols));

	float2 texCoord0 = texCoord + float2( xOffset0 * gridDim.x, yOffset0 * gridDim.y); 
	texCoord0 /= float2( SimulationTextureWidth, SimulationTextureHeight);
	float sample0 = Texture_color.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(texCoord0,0), 0);
  
  
	uint Z1 = Z0 + 1;
	uint xOffset1 = (Z1 % float(cols));
	uint yOffset1 = (Z1 / float(cols));
	float2 texCoord1 = texCoord + float2( xOffset1 * gridDim.x, yOffset1 * gridDim.y); 
	texCoord1 /= float2( SimulationTextureWidth, SimulationTextureHeight);
	float sample1 = Texture_color.SampleLevel(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(texCoord1,0), 0);

	float col = lerp( sample0, sample1, (O.z*gridDim.z) - Z0);
    return float4(col,col,col,col);
}

float4 raycast(PS_INPUT_RAYCAST input)
{
   float2 normalizedInputPos = float2(input.pos.x/RTWidth, input.pos.y/RTHeight);
   float4 rayData = Texture_depth.Sample(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float3(normalizedInputPos.xy,0));
   
          
   if((rayData.x < 0) || (rayData.w <= 0))
        return float4(0,0,0,0);

    if(rayData.y < 0)
    {
       //initialize the position of the fragment and adject the depth
       rayData.xyz = input.posInGrid;
       float2 inputPos = float2((normalizedInputPos.x*2.0)-1.0,(normalizedInputPos.y*2.0)-1.0);
       float ratioY = ZNear*distanceToScreenY;
       float ratioX = ZNear*distanceToScreenX;
       float distanceToNearPlane = length(float3( inputPos.x*ratioX, inputPos.y*ratioY, ZNear ));
       rayData.w = rayData.w - distanceToNearPlane;
    }

    float3 O = rayData.xyz;
    float rayLength = rayData.w;
     
    float fnSamples = ( rayLength / renderScale * maxGridDim );
    int nSamples = floor(fnSamples);
    float3 D = normalize( (O - EyeInWorld.xyz) * gridDim.xyz ) / gridDim.xyz;
  
    float4 color = 0.0;
    float4 sample;
    float alpha;
    float2 texCoord; 
    float weight = 1.0;

  

  for( int i=0; i<=nSamples; i++ )
  {
    // if doing front-to-back blending we can do early exit when opacity saturates
    if( color.a > 0.99 )
        break;
  
    if(i==nSamples)
        weight = frac(fnSamples);    


    if ( O.z > 1 || O.x<0 || O.x > 1 || O.y < 0 || O.y > 1 )
	{
		alpha = 0;
	}
	else
	{
	    sample = RaySampleTrilinear( O ); 
	    sample *= weight;
		alpha = (sample.r) * smokeThickness; //increase to make thicker looking
	}
	
	/* this is the red glow hack, currently not used
    if(alpha > glowMinDensity )
    {
		sample.xyz = (sample.rrr*(1.0+glowMinDensity-alpha)) + glowColorCompensation.xyz*(alpha-glowMinDensity); 
    }*/
    
    
    
    float t = alpha * (1.0-color.a);
    color.rgb += t * (smokeAmbientLight.xxx + sample.xyz)*splatColor.rgb;
    color.a += t;
    O += D;
  }
  color.w *= 0.978;
  return color;
}


//front to back raycasting
float4 PS_RAYCAST(PS_INPUT_RAYCAST input) : SV_Target
{
    return raycast(input);
}

float4 PS_RAYCASTCOPY_QUAD(PS_INPUT_RAYCAST input) : SV_Target
{
    float edge = Texture_edgeData.Sample(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float2(input.pos.x/RTWidth,input.pos.y/RTHeight)).r;
 
     float4 tex = Texture_raycast.Sample(FILTER_MIN_MAG_MIP_LINEAR_CLAMP, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
     if(edge > 0 && tex.a > 0)
         return raycast(input);
      else
         return float4(tex.r,tex.g,tex.b,tex.a);
     
}

float4 PS_RAYDATA_COPY(PS_INPUT_RAYCAST input) : SV_Target
{
   return Texture_depth.Sample(FILTER_MIN_MAG_MIP_POINT_CLAMP, float3(input.pos.x/RTWidth,input.pos.y/RTHeight,0));
}

//--------------------------------------------------------------------------------------
//techniques
//--------------------------------------------------------------------------------------


technique10 FluidSim < SHADER_VERSION_44 > 
{

    pass Advect1 //0
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_ADVECT()   )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_ADVECT_1()      ));
    }
    
    pass Advect2 //1
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_ADVECT()    )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_ADVECT_2()      ));
    }

    pass AdvectBFECC //2
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_ADVECT()   )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_ADVECT_BFECC()  ));
    }
    
    pass AdvectVel //3
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_ADVECT()   )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_ADVECT_VEL()    ));
    }
    
    pass Vorticity //4
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_VORTICITY() )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_VORTICITY()      ));
    }

    pass Confinement //5
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_CONFINEMENT()    )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_CONFINEMENT_BFECC()   ));
    }

    pass Gaussian //6
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_INCREASE() )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_GAUSSIAN()      ));
    }

    pass Divergence //7
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_DIVERGENCE()  )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_DIVERGENCE()       ));
    }

    pass Jacobi1 //8
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_JACOBI()   )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_JACOBI_1()      ));
    }
    
    pass Jacobi2 //9
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_JACOBI()   )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_JACOBI_2()      ));
    }

    pass Project //10
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_PROJECT()  )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_PROJECT()       ));
    }

    pass DiffDivergence //11
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID()          )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_DIFF_DIVERGENCE()       ));
    }
    
    pass DrawTexture //12
    { 
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_DRAW()     )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_DRAW_TEXTURE()  ));
    }
    
     pass DrawVelocity //13
     {
         SetVertexShader(CompileShader( vs_4_0,  VS_GRID_DRAW()     )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_DRAW_VELOCITY()  ));
     }
    
     pass DrawWhite //14
    { 
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_DRAW()   )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_DRAW_WHITE()  ));
    }
    
    pass passIndexConfinementNormal //15
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_CONFINEMENT()      )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_CONFINEMENT_NORMAL()   ));
    }
    
     // for VolumeRenderer-------------------------------------------------------------
    pass RayDataFront //16
    {
		SetVertexShader(CompileShader( vs_4_0, VS_RAYDATA_FRONT() ));
		SetGeometryShader ( NULL );
		SetPixelShader( CompileShader( ps_4_0, PS_VOLUME_DEPTH_FRONT() ));
	}
	pass RayDataBack  //17
       {
		SetVertexShader(CompileShader( vs_4_0, VS_RAYDATA_BACK() ));
		SetGeometryShader ( NULL );
		SetPixelShader( CompileShader( ps_4_0, PS_VOLUME_DEPTH_BACK() ));
	}
	
	pass QuadDownSampleRayDataTexture //18
	{
		SetVertexShader(CompileShader( vs_4_0, VS_QUAD() ));
		SetGeometryShader ( NULL );
		SetPixelShader( CompileShader( ps_4_0, PS_RAYDATA_COPY() ));	
	}
	
	
    pass suspicious //19
    {
    
		SetVertexShader(CompileShader( vs_4_0, VS_QUAD() ));
		SetGeometryShader ( NULL );
		SetPixelShader( CompileShader( ps_4_0, PS_RAYCAST() ));
    }
   
   	pass DensityTexture //20
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_INCREASE()    )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_DENSITYTEXTURE()   ));
    }	    
    
	pass QuadRaycastCopy  //21
	{
		SetVertexShader(CompileShader( vs_4_0, VS_QUAD() ));
		SetGeometryShader ( NULL );
		SetPixelShader( CompileShader( ps_4_0, PS_RAYCASTCOPY_QUAD() ));
	}

	pass QuadRaycast //22 
	{
		SetVertexShader(CompileShader( vs_4_0, VS_QUAD() ));
		SetGeometryShader ( NULL );
		SetPixelShader( CompileShader( ps_4_0, PS_RAYCAST() ));
	}
	
	pass GaussianFlat //23 
	{
  	    SetVertexShader(CompileShader(vs_4_0, VS_GRID_INCREASE()  ));
		SetGeometryShader ( NULL );
		SetPixelShader(CompileShader(ps_4_0,PS_GAUSSIAN_FLAT()    ));
	}
    
    
    pass Vorticity_Obstacles //24
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_VORTICITY()          )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_VORTICITY_OBSTACLES()     ));
    }


    pass Divergence_Obstacles //25
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_DIVERGENCE()         )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_DIVERGENCE_OBSTACLES()    ));
    }

    pass Jacobi1_Obstacles //26
    { 
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_JACOBI()             )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_JACOBI_1_OBSTACLES()      ));
    }
    
    pass Jacobi2_Obstacles //27
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_JACOBI()             )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_JACOBI_2_OBSTACLES()      ));
    }

    pass Project_Obstacles //28 
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_PROJECT()            )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_PROJECT_OBSTACLES()       ));
    }
    
    pass VelocityTexture //29
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_INCREASE()     )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_VELOCITYTEXTURE()   ));
    } 
    
     pass QuadEdgeDetect //30
    {
        SetVertexShader(CompileShader( vs_4_0,  VS_EDGEDETECT()   )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_EDGEDETECT()   ));
    }	    
    
    pass DrawObstacle //31
     {
        SetVertexShader(CompileShader( vs_4_0,  VS_GRID_INCREASE()    )); 
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0,  PS_OBSTACLETEXTURE()  ));        
     }
}
