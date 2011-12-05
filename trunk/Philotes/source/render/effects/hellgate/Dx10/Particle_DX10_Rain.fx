//
// Particle Shader for dx10
//



#include "_common.fx"

cbuffer cbChangesEveryFrame
{
//    float3 LightInWorldSpace; 
//    float LightIntensity; 
//    float4x4 InverseView;
//    float3 EyeInWorld;
}

cbuffer cbChangesRarely
{
    float NearPlane;
    float FarPlane;
    float SpriteSize;
    float minBoundx;
    float maxBoundx;
    float minBoundy;
    float maxBoundy;
    float minBoundz;
    float maxBoundz;
}

cbuffer cbImmutable
{
    float2 g_texcoords[4] = 
    { 
        float2(0,1), 
        float2(1,1),
        float2(0,0),
        float2(1,0),
    };
}


SamplerState samAniso
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
};


/*
struct VS_RAIN_PARTICLE
{
    float3 Position     : POSITION;         // position of the particle in world space
    float3 Velocity     : VELOCITY;         // velocity of the particle in world space 
    uint   Type         : TYPE;             // particle type
};
*/



//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////


struct PS_RAIN_PARTICLE
{
    float4 pos : SV_Position;
    float type  : TYPE;
    float3 lightDir1   : LIGHT;
    float3 lightDir2   : LIGHT2;
    float3 lightDir3   : LIGHT3;
    float3 eyeVec     : EYE;
    float2 tex : TEXCOORD;
    float modulate : MOD;

};




PS_RAIN_PARTICLE VS_RENDER_RAIN ( VS_PARTICLE_INPUT In)
{
    PS_RAIN_PARTICLE Out;

    float3 worldPos = mul( In.Position, World );

    Out.type = float(In.Rand*8) + 1; //(int( VertID/6) % 9) + 1 ; 

    Out.pos = mul( float4( In.Position, 1.0f ), WorldViewProjection);         
    Out.lightDir1 = PointLightsPos(WORLD_SPACE)[0].xyz - worldPos;
    Out.lightDir2 = PointLightsPos(WORLD_SPACE)[1].xyz - worldPos;
    Out.lightDir3 = DirLightsDir(WORLD_SPACE)[0].xyz; //float3(0.866,0,0.5);
    Out.lightDir3.z = 0.5; //temp sarah!
    
    Out.eyeVec = EyeInWorld.xyz - worldPos;  
    Out.tex = In.Tex;
    
    Out.modulate = frac(In.Rand*8);
    if( Out.modulate > 0.9 )
       Out.modulate = 0.9;
    else
       Out.modulate = Out.modulate/9.0 + 0.35; //Out.modulate = Out.modulate/3.0 + 0.2;
    
    //Out.modulate = saturate( frac(In.Rand*8) + 0.2 );
 
    return Out;
}




//pixel shader-------------------------------------------------------------------------


float4 rainResponse(PS_RAIN_PARTICLE input, float3 lightVector, float lightIntensity, float3 lightColor, float3 eyeVector)
{
      float opacity = 0.0;

        
	     {
	  	  //create the horizontalLightAngle
		  float2 lightDir = normalize(float2(lightVector.x , lightVector.y ));
		  float2 eyeDir = normalize(float2(eyeVector.x,eyeVector.y));  //normalize(float2(g_eyePos.z - input.worldPos.z , g_eyePos.x - input.worldPos.x ));
		  float cosHAngle = dot(eyeDir,lightDir); 
		  float horizontalLightAngle = acos(cosHAngle)*180.0/PI;  
		  float crossProd = eyeDir.x*lightDir.y - eyeDir.y*lightDir.x;
		  if(crossProd < 0)
			 horizontalLightAngle = 360 - horizontalLightAngle;
	      
	      
		  float textureCoordsH1 = input.tex.x;
		  float textureCoordsH2 = input.tex.x;
		  float textureCoordsV1 = input.tex.y;
		  float textureCoordsV2 = input.tex.y;  
	      
	      
		  //Horizontal Lerp:  0<=horizontalLightAngle<360
		  float hAngle = (horizontalLightAngle-10.0)/20.0;
		  int horizontalLightIndex1 = floor(hAngle);      
		  int horizontalLightIndex2 = horizontalLightIndex1 + 1;
		  float s = frac(hAngle);
	      
		  if(horizontalLightIndex1 < 0)
		  { 
			 horizontalLightIndex1 += 18;
		  }
		  if(horizontalLightIndex2 > 17)
		  {
			 horizontalLightIndex2 -= 18;
		  }
	      
		  if(horizontalLightIndex1 > 8)
		  { 
			 horizontalLightIndex1 = 17 - horizontalLightIndex1;
			 textureCoordsH1 = 1.0 - textureCoordsH1;
		  }
		  if(horizontalLightIndex2 > 8)
		  { 
			 horizontalLightIndex2 = abs(17 - horizontalLightIndex2); 
			 textureCoordsH2 = 1.0 - textureCoordsH2;
		  }

		  //create the verticalLightAngle
		  float cosVAngle = dot( normalize(lightVector), float3(lightDir.x,lightDir.y,0)); //the angle between the lightVector and its projection on the xy plane 
		  float verticalLightAngle = acos(cosVAngle)*180/PI;


		  //Vertical Lerp:  0<=verticalLightAngle<=90
		  float vAngle = (verticalLightAngle-10.0)/20.0;
		  int verticalLightIndex1 = abs(floor(vAngle));      
		  int verticalLightIndex2 = verticalLightIndex1 + 1;
		  float t = frac(vAngle);
	      
		  if(verticalLightIndex1 > 4)
			 verticalLightIndex1 = 4;
		  if(verticalLightIndex2 > 4)
			 verticalLightIndex2 = 4;
	     
	     float col1 =0;
	     float col2 =0;
	     float col3 =0;
	     float col4 =0;
	     
	    
	     float3 tex1 = float3(textureCoordsH1, textureCoordsV1, verticalLightIndex1*90 + horizontalLightIndex1*10 + int(input.type));
	     float3 tex2 = float3(textureCoordsH2, textureCoordsV1, verticalLightIndex1*90 + horizontalLightIndex2*10 + int(input.type));
	     float3 tex3 = float3(textureCoordsH1, textureCoordsV2, verticalLightIndex2*90 + horizontalLightIndex1*10 + int(input.type));
	     float3 tex4 = float3(textureCoordsH2, textureCoordsV2, verticalLightIndex2*90 + horizontalLightIndex2*10 + int(input.type));
	          
	     	     
		  col1 = Texture_rainTextureArray.Sample( samAniso, tex1);
		  col2 = Texture_rainTextureArray.Sample( samAniso, tex2);
		  col3 = Texture_rainTextureArray.Sample( samAniso, tex3);
		  col4 = Texture_rainTextureArray.Sample( samAniso, tex4);
	     
	     float hOpacity1 = lerp(col1,col2,s);
    	 float hOpacity2 = lerp(col3,col4,s);
		 opacity = lerp(hOpacity1,hOpacity2,t);      
    	 opacity = pow(opacity,0.7);
		 opacity = lightIntensity * opacity;
	     
	     }
	     	     	     	     	     

		return float4(lightColor,opacity);

}

float4 PS_RENDER_RAIN ( PS_RAIN_PARTICLE In ) : SV_Target
{

	 
    float3 tex1Light;
    float3 tex2Light;
    float3 tex3Light;
    float3 tex4Light;
    
    float4 lightResponse1 = float4(0,0,0,0);
    float4 lightResponse2 = float4(0,0,0,0);
    float4 lightResponseDirectional = float4(0,0,0,0);

   
    float4 lightColorDir = DirLightsColor[0]; 
    float lightIntensityDir = lightColorDir.w * 0.5;   
    lightResponseDirectional = rainResponse(In, In.lightDir3, lightIntensityDir,lightColorDir.xyz, In.eyeVec);
    
#if 1    
    if( PointLightCount>0 )
    {
      float4 lightColor1 = PointLightsColor[ 0 ];
      float lightIntensity1 = lightColor1.w;//  * 0.75;
      float falloff = PointLightsFalloff( WORLD_SPACE )[ 0 ].x - length(In.lightDir1)*PointLightsFalloff( WORLD_SPACE )[ 0 ].y;
      lightIntensity1 *= falloff;
      
      if(lightIntensity1 > 0.01)
	  {
      	  float3 L = normalize(In.lightDir1);
    	  if(  dot(-L, float3(0,0,-1)) > 0.8 )      
              lightResponse1 = rainResponse(In, In.lightDir1, lightIntensity1 * In.modulate,lightColor1.xyz, In.eyeVec);
      }
    }
     
    if( PointLightCount>1 )
    {
      float4 lightColor2 = PointLightsColor[ 1 ];
      float lightIntensity2 = lightColor2.w;// * 0.75;
      float falloff = PointLightsFalloff( WORLD_SPACE )[ 1 ].x - length(In.lightDir2)*PointLightsFalloff( WORLD_SPACE )[ 1 ].y;
      lightIntensity2 *= falloff;
       
      if(lightIntensity2 > 0.01)
      {
       	  float3 L = normalize(In.lightDir2);
      	  if(  dot(-L, float3(0,0,-1)) > 0.8 )      
              lightResponse2 = rainResponse(In, In.lightDir2, lightIntensity2 * In.modulate,lightColor2.xyz, In.eyeVec);
      }
    }
     
     float totalOpacity = lightResponse1.a + lightResponse2.a + lightResponseDirectional.a;
     return float4( float3(lightResponse1.rgb*lightResponse1.a/totalOpacity + lightResponse2.rgb*lightResponse2.a/totalOpacity + lightResponseDirectional.rgb*lightResponseDirectional.a/totalOpacity), totalOpacity);
#else
// DEBUG...
    float2 v = (float2)0;
    if( PointLightCount>0 )
    {
        v.x = 1.0/length(In.lightDir1);
    }
    if( PointLightCount>1 )
    {
        v.y = 1.0/length(In.lightDir2);
    }
    return float4(v,0,1);
#endif
}




technique10 RenderRainParticles < SHADER_VERSION_44 > 
{
    pass p0
    {
        SetVertexShader( CompileShader(   vs_4_0, VS_RENDER_RAIN() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader(    ps_4_0, PS_RENDER_RAIN() ) );
        
        //DXC_RASTERIZER_SOLID_NONE;
        //DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD; 
        //DXC_DEPTHSTENCIL_ZREAD; 
    }  
}

//--------------------------------------------------------------------------------------------
//render and expand rain
//--------------------------------------------------------------------------------------------
/*
struct GS_RAIN_PARTICLE
{
    float3 Position     : POSITION;         // position of the particle in world space
    float3 Velocity     : VELOCITY;         // velocity of the particle in world space 
    uint index1         :INDEX1;
    uint index2         :INDEX2;
    uint index3         :INDEX3;
    uint index4         :INDEX4;
    bool flipH1         :FLIP1;
    bool flipH2         :FLIP2;
    float2 fractions1   :TEX5;
};

//vertex shader-------------------------------------------------------------------------------

GS_RAIN_PARTICLE VS_RENDER_EXPAND_RAIN ( VS_RAIN_PARTICLE In )
{
    GS_RAIN_PARTICLE Out = (GS_RAIN_PARTICLE)0;

    Out.Position = In.Position;          
    Out.Velocity = In.Velocity;

    float3 worldPos = mul( In.Position, World );
    float3 eyeVector =  EyeInWorld - worldPos;
    float3 lightVector = LightInWorldSpace - worldPos;

    caculateRainTextureIndices(lightVector,eyeVector,In.Type,
    Out.index1,Out.index2,Out.index3,Out.index4,Out.flipH1,Out.flipH2,Out.fractions1.x, Out.fractions1.y);
        
    return Out;
}

//geometry shader-----------------------------------------------------------------------------

bool cullSprite( float3 position, float SpriteSize)
{
    float4 vpos = mul(float4(position,1), WorldView);
        
    if( (vpos.z < (NearPlane - SpriteSize )) || ( vpos.z > (FarPlane + SpriteSize)) ) 
    {
		return true;
	}
	else 
	{
		float4 ppos = mul( vpos, Projection);
		float wext = ppos.w + SpriteSize;
		if( (ppos.x < -wext) || (ppos.x > wext) ||
			(ppos.y < -wext) || (ppos.y > wext) ) {
			return true;
		}
		else 
		{
			return false;
		}
	}
    
    return false;
}


//we can later put in the velocity for rendering aswell

[maxvertexcount(4)]
void GS_RENDER_EXPAND_RAIN(point GS_RAIN_PARTICLE input[1], inout TriangleStream<PS_RAIN_PARTICLE> SpriteStream)
{
    if(!cullSprite(input[0].Position,2*SpriteSize))
    {    
		PS_RAIN_PARTICLE output;
	    
		output.index1 = input[0].index1;
        output.index2 = input[0].index2;
        output.index3 = input[0].index3;
        output.index4 = input[0].index4;
        output.flipH1 = input[0].flipH1;
        output.flipH2 = input[0].flipH2;
        output.fractions1 = input[0].fractions1;

        float3 velVec = input[0].Velocity;

		float3 pos[4];
	    
	    float height = SpriteSize;
	    float3 width = InverseView[0] * (SpriteSize/64);
	    
		pos[0] =  mul( input[0].Position, World );
		pos[1] = pos[0] + width;
		pos[2] = pos[0] + (velVec * height);
		pos[3] = pos[2] + width;
		
		output.Position = mul( float4(pos[0],1.0), WorldViewProjection );
		output.Tex = g_texcoords[0];
		SpriteStream.Append(output);
		
		output.Position = mul( float4(pos[1],1.0), WorldViewProjection );
		output.Tex = g_texcoords[1];
		SpriteStream.Append(output);
		
		output.Position = mul( float4(pos[2],1.0), WorldViewProjection );
		output.Tex = g_texcoords[2];
		SpriteStream.Append(output);
		
		output.Position = mul( float4(pos[3],1.0), WorldViewProjection );
		output.Tex = g_texcoords[3];
		SpriteStream.Append(output);
		
		SpriteStream.RestartStrip();
	}   
}

technique10 RenderExpandRainParticles < SHADER_VERSION_44 >
{
    pass p0
    {
        SetVertexShader( CompileShader(   vs_4_0, VS_RENDER_EXPAND_RAIN() ) );
        SetGeometryShader( CompileShader( gs_4_0, GS_RENDER_EXPAND_RAIN() ) );
        SetPixelShader( CompileShader(    ps_4_0, PS_RENDER_RAIN() ) );
        
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD; 
        DXC_DEPTHSTENCIL_ZREAD; 
    }  
}

*/
//--------------------------------------------------------------------------------------------
// advance rain
//--------------------------------------------------------------------------------------------

/*
VS_RAIN_PARTICLE VS_ADVANCE_RAIN(VS_RAIN_PARTICLE input)
{
   //input.Position += input.Velocity;
   
   //if(input.Position.x < minBoundx || input.Position.x > maxBoundx || input.Position.y < minBoundy || input.Position.y > maxBoundy || input.Position.z < minBoundz || input.Position.z > maxBoundz)
   //{
       //respawn
       
   //}
   return input;
} 


GeometryShader gsStreamOut = ConstructGSWithSO( CompileShader( vs_4_0, VS_ADVANCE_RAIN() ), "POSITION.xyz; VELOCITY.xyz; TYPE.x" ); 
technique10 AdvanceParticles < SHADER_VERSION_44 >
{
    pass p0
    {
        SetVertexShader( CompileShader( vs_4_0, VS_ADVANCE_RAIN() ) );
        //SetGeometryShader( gsStreamOut );
        //SetGeometryShader( NULL );
        SetPixelShader( NULL );
        
        DXC_DEPTHSTENCIL_OFF;
    }  
}
*/