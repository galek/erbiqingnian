#ifndef __DX10_COMMON_FXH__
#define __DX10_COMMON_FXH__

#define DX10_DOF
//#define DX10_DOF_GATHER

shared cbuffer cbRasterizer{
	int g_iFillMode = 3; //Solid
	int g_iCullMode = 1; //NONE
	bool g_bFrontCounterClockWise;
	int g_iDepthBias;
	float g_fDepthBiasClamp;
	float g_fSlopeScaledDepthBias;
	bool g_bDepthClipEnable;
	bool g_bScissorEnable;
	bool g_bMultisampleEnable;
	bool g_bAntialiasedLineEnable;
};

//Blend state
shared cbuffer cbBlend{
	bool g_bAlphaToCoverageEnable;
	bool g_bBlendEnable[8];
	int g_iSrcBlend;
	int g_iDestBlend;
	int g_iBlendOp;
	int g_iSrcBlendAlpha;
	int g_iDestBlendAlpha;
	int g_iBlendOpAlpha;
	int g_iRenderTargetWriteMask[8];
	float4 g_fBlendFactor;
	int g_iSampleMask;
	
	float4 gfPCSSLightSize = float4( 0.03f, 0.005f, 0.0f, 0.0f );
	float4 gfDOFParams = float4( 1.0f, 5.0f, 15.0f, 0.0f);
	float4 gvCamVelocity;
};

shared cbuffer CBAlphaTest
{
	bool g_bAlphaTestEnable;
	int g_iAlphaTestRef;
	int g_iAlphaTestFunc;
};

//Depth stencil state
shared cbuffer cbDepthStencil{
	bool g_bDepthEnable;
	int g_iDepthWriteMask;
	int g_iDepthFunc;
	
	bool g_bStencilEnable;
	int g_iStencilReadMask;
	int g_iStencilWriteMask;
	
	int g_iFrontFaceStencilFailOp;
	int g_iFrontFaceStencilDepthFailOp;
	int g_iFrontFaceStencilPassOp;
	int g_iFrontFaceStencilFunc;
	
	int g_iBackFaceStencilFailOp;
	int g_iBackFaceStencilDepthFailOp;
	int g_iBackFaceStencilPassOp;
	int g_iBackFaceStencilFunc;
	
	int g_iStencilRef;
};

// To use global rasterizer state, use 
//				SetRasterizerStat`e(GlobalRasterizer);
//
// To use global blend state, use
//				SetBlendState(GlobalBlend, g_fBlendFactor, g_iSampleMask);
//
// To use global depth stencil state, use
//				SetDepthStencilState(GlobalDS, g_iStencilRef);

//Texture arrays
//rain--------------------------------------------------------

shared Texture2DArray Texture_rainTextureArray : Texture_rainTextureArray
<
    string UIName = "Texture_rainTextureArray";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>; 


//smoke simulation variables----------------------------------
shared Texture2D Texture_zOffset : Texture_zOffset
<
    string UIName = "Texture_zOffset";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>; 

shared Texture2D Texture_color : Texture_color  
<
    string UIName = "Texture_color";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_velocity : Texture_velocity //1
<
    string UIName = "Texture_velocity";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_pressure : Texture_pressure
<
    string UIName = "Texture_pressure";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_temp1 : Texture_temp1
<
    string UIName = "Texture_temp1";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_temp3 : Texture_temp3
<
    string UIName = "Texture_temp3";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_tempVelocity : Texture_tempVelocity
<
    string UIName = "Texture_tempVelocity";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_depth : Texture_depth
<
    string UIName = "Texture_depth";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_obstacles : Texture_obstacles
<
    string UIName = "Texture_obstacles";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_sceneDepth : Texture_sceneDepth
<
    string UIName = "Texture_sceneDepth";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_raycast : Texture_raycast
<
    string UIName = "Texture_raycast";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_raydataSmall : Texture_raydataSmall
<
    string UIName = "Texture_raydataSmall";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;
shared Texture2D Texture_edgeData : Texture_edgeData
<
    string UIName = "Texture_edgeData";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;


shared Texture2D Texture_densityPattern : Texture_densityPattern
<
    string UIName = "Texture_densityPattern";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;

shared Texture2D Texture_velocityPattern : Texture_velocityPattern
<
    string UIName = "Texture_velocityPattern";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;


shared Texture2D Texture_obstaclePattern : Texture_obstaclePattern
<
    string UIName = "Texture_obstaclePattern";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;


shared Texture2D Texture_motion : Texture_motion
<
    string UIName = "Texture_motion";
   	int Texcoord = 0;
	int MapChannel = 1;	
	string ResourceType = "2D";
>;

//These textures are to emulate Dx9's texture stages
shared Texture2D TextureStage0 : TextureStage0;
shared Texture2D TextureStage1 : TextureStage1;
shared Texture2D TextureStage2 : TextureStage2;
shared Texture2D TextureStage3 : TextureStage3;
shared Texture2D TextureStage4 : TextureStage4;
shared Texture2D TextureStage5 : TextureStage5;
shared Texture2D TextureStage6 : TextureStage6;
shared Texture2D TextureStage7 : TextureStage7;

//end smoke simulation variables----------------------------------

#ifdef DX10_DOF

float fDOFComputeFarBlur( float fDepth )
{
	float fFarBlur = ( fDepth - gfDOFParams.y ) / ( gfDOFParams.z - gfDOFParams.y );
	return clamp( fFarBlur, 0.0f, 1.0f ); // If fFarBlur is negative fDepth is smaller than the focal depth
}

float fDOFComputeNearBlur( float fDepth )
{
	float fNearBlur = ( gfDOFParams.y - fDepth ) / ( gfDOFParams.y - gfDOFParams.x );
	return max( fNearBlur, 0.0f ); // If fNearBlur is negative fDepth is larger than the focal depth
}

float fDOFComputeBlur( float fDepth )
{
	float fDelta = fDepth - gfDOFParams.y;
	
	if( fDelta < 0 ) //This sample is in the foreground
		// AE 2007.09.19: We don't want foreground DOF
		return 0; //max( fDelta / ( gfDOFParams.y - gfDOFParams.x ), -1 ) ;
	else
		return min( fDelta / ( gfDOFParams.z - gfDOFParams.y ), 1 );
}

#endif

#define POISSON_MED_COUNT 8
#define POISSON_LARGE_COUNT 16

shared cbuffer POISSON_DISKS
{
	float2 poissonDiskMed[POISSON_MED_COUNT] = {
		float2( 0.84593856, -0.70601606 ),
		float2( 0.73318291, 0.68711102 ),
		float2( -0.48267812, 0.97983003 ),
		float2( -0.19016320, -0.86538249 ),
		float2( -0.77416289, -0.42669058 ),
		float2( 0.013684511, 0.089085698 ),
		float2( 0.97674334, -0.018270254 ),
		float2( -0.86144793, 0.32173669 )
};

	float2 poissonDiskLarge[POISSON_LARGE_COUNT] = {
		float2( -0.94201624, -0.39906216 ),
		float2( 0.94558609, -0.76890725 ),
		float2( -0.094184101, -0.92938870 ),
		float2( 0.34495938, 0.29387760 ),
		float2( -0.91588581, 0.45771432 ),
		float2( -0.81544232, -0.87912464 ),
		float2( -0.38277543, 0.27676845 ),
		float2( 0.97484398, 0.75648379 ),
		float2( 0.44323325, -0.97511554 ),
		float2( 0.53742981, -0.47373420 ),
		float2( -0.26496911, -0.41893023 ),
		float2( 0.79197514, 0.19090188 ),
		float2( -0.24188840, 0.99706507 ),
		float2( -0.81409955, 0.91437590 ),
		float2( 0.19984126, 0.78641367 ),
		float2( 0.14383161, -0.14100790 )
};



};

float2 GetPixelVelocity( float4 vPosProjSpaceCurrent, float4 vPosProjSpaceLast )
{          
    // Convert to non-homogeneous points [-1,1] by dividing by w 
    vPosProjSpaceCurrent /= vPosProjSpaceCurrent.w;
    vPosProjSpaceLast /= vPosProjSpaceLast.w;
    
    // Vertex's velocity (in non-homogeneous projection space) is the position 
    // this frame minus its position last frame.
    float2 velocity = vPosProjSpaceCurrent - vPosProjSpaceLast;    
    
    // The velocity is now between (-2,2) so divide by 2 to get it to (-1,1)
    velocity /= 2.0f;   
	    
	return velocity / gvCamVelocity.w; // pixels per second
}

#endif //__DX10_COMMON_FXH__