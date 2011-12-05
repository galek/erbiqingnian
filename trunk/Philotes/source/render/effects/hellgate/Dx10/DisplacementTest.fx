//--------------------------------------------------------------------------------------
// File: DisplacementMapping10.fx
//
// The effect file for the DisplacementMapping10 sample.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "_common.fx"

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

struct VSDisplaceIn
{
    float3 Pos		: POSITION;			// Position
    float4 Norm		: NORMAL;			// Normal
    float4 Tex		: TEXCOORD0;		// Texture coordinate
    //uint   VertID	: SV_VertexID;		// vertex ID, used for consistent tetrahedron generation
};

struct VSDisplaceOut
{
    float4 Pos	: POSITION;		//Position
    float3 vPos : POSVIEW;		//view pos
    float3 Norm : NORMAL;		//Normal
    float3 Tex	: TEXCOORD0;	//Texture coordinate
};

struct PSDisplaceIn
{
    float4 Pos : SV_Position;
    float4 planeDist : TEXCOORD0;
    float3 vPos : TEXCOORD1;
    
    float3 Norm : TEXCOORD2;			// Normal of the first vert
    float3 Tex : TEXCOORD3;				// Texture Coordinate of the first vert
    float3 pos0 : TEXCOORD4;			// Position of the first vert
    
    float4 Gtx : TEXCOORD5;			// Gradient of the tetrahedron for X texcoord
    float4 Gty : TEXCOORD6;			// Gradient of the tetrahedron for Y texcoord
    float4 Gtz : TEXCOORD7;			// Gradient of the tetrahedron for Z texcoord
};

#if 1
	float g_MaxDisplacement = 0.1;
	float g_MinDisplacement = 0.015;
#else
	float g_MaxDisplacement = 0.8;
	float g_MinDisplacement = 0.015;
#endif

SamplerState g_samLinear : register(SAMPLER(13)) 
{
    Filter = ANISOTROPIC;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState g_samPoint : register(SAMPLER(14)) 
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

float RayDistToPlane( float3 vPoint, float3 vDir, float3 A, float3 planeNorm )
{	
    float Nom = dot( planeNorm, float3(A - vPoint) );
    float DeNom = dot( planeNorm, vDir );
    return Nom/DeNom;
}

void CalcGradients( inout PSDisplaceIn V0, inout PSDisplaceIn V1, inout PSDisplaceIn V2, inout PSDisplaceIn V3, float3 N0, float3 N1, float3 N2, float3 N3 )
{
    float dotN0 = dot(N0, V0.vPos - V3.vPos);
    float dotN1 = dot(N1, V1.vPos - V2.vPos);
    float dotN2 = dot(N2, V2.vPos - V1.vPos);
    float dotN3 = dot(N3, V3.vPos - V0.vPos);
    
    //Tex
    float3	Gtx =  ( V0.Tex.x / dotN0 )*N0;
            Gtx += ( V1.Tex.x / dotN1 )*N1;
            Gtx += ( V2.Tex.x / dotN2 )*N2;
            Gtx += ( V3.Tex.x / dotN3 )*N3;
    
    float3	Gty =  ( V0.Tex.y / dotN0 )*N0;
            Gty += ( V1.Tex.y / dotN1 )*N1;
            Gty += ( V2.Tex.y / dotN2 )*N2;
            Gty += ( V3.Tex.y / dotN3 )*N3;
    
    float3	Gtz =  ( V0.Tex.z / dotN0 )*N0;
            Gtz += ( V1.Tex.z / dotN1 )*N1;
            Gtz += ( V2.Tex.z / dotN2 )*N2;
            Gtz += ( V3.Tex.z / dotN3 )*N3;            
            
    V0.Norm = V0.Norm;
    V0.Tex = V0.Tex;
    V0.pos0 = V0.vPos;
    V0.Gtx.xyz = Gtx;
    V0.Gty.xyz = Gty;
    V0.Gtz.xyz = Gtz;
    
    V1.Norm = V0.Norm;
    V1.Tex = V0.Tex;
    V1.pos0 = V0.vPos;
    V1.Gtx.xyz = Gtx;
    V1.Gty.xyz = Gty;
    V1.Gtz.xyz = Gtz;
    
    V2.Norm = V0.Norm;
    V2.Tex = V0.Tex;
    V2.pos0 = V0.vPos;
    V2.Gtx.xyz = Gtx;
    V2.Gty.xyz = Gty;
    V2.Gtz.xyz = Gtz;
    
    V3.Norm = V0.Norm;
    V3.Tex = V0.Tex;
    V3.pos0 = V0.vPos;
    V3.Gtx.xyz = Gtx;
    V3.Gty.xyz = Gty;
    V3.Gtz.xyz = Gtz;
 }

void GSCreateTetra( in VSDisplaceOut A, in VSDisplaceOut B, in VSDisplaceOut C, in VSDisplaceOut D,
                    inout TriangleStream<PSDisplaceIn> DisplaceStream )
{	
	float3 eye = EyeInWorld;
    float3 AView = normalize( A.vPos - eye );
    float3 BView = normalize( B.vPos - eye );
    float3 CView = normalize( C.vPos - eye );
    float3 DView = normalize( D.vPos - eye );
    
    PSDisplaceIn Aout = (PSDisplaceIn)0;
    Aout.Pos = A.Pos;
    Aout.vPos = A.vPos;
    Aout.Norm = A.Norm;
    Aout.Tex = A.Tex;
    
    PSDisplaceIn Bout = (PSDisplaceIn)0;
    Bout.Pos = B.Pos;
    Bout.vPos = B.vPos;
    Bout.Norm = B.Norm;
    Bout.Tex = B.Tex;
    
    PSDisplaceIn Cout = (PSDisplaceIn)0;
    Cout.Pos = C.Pos;
    Cout.vPos = C.vPos;
    Cout.Norm = C.Norm;
    Cout.Tex = C.Tex;
    
    PSDisplaceIn Dout = (PSDisplaceIn)0;
    Dout.Pos = D.Pos;
    Dout.vPos = D.vPos;
    Dout.Norm = D.Norm;
    Dout.Tex = D.Tex;
    
    float3 AB = C.vPos-B.vPos;
    float3 AC = D.vPos-B.vPos;
    float3 planeNormA = normalize( cross( AC, AB ) );
           AB = D.vPos-A.vPos;
           AC = C.vPos-A.vPos;
    float3 planeNormB = normalize( cross( AC, AB ) );
           AB = B.vPos-A.vPos;
           AC = D.vPos-A.vPos;
    float3 planeNormC = normalize( cross( AC, AB ) );
           AB = C.vPos-A.vPos;
           AC = B.vPos-A.vPos;
    float3 planeNormD = normalize( cross( AC, AB ) );
    
    Aout.planeDist.w = RayDistToPlane( A.vPos, AView, B.vPos, planeNormA );    
    Bout.planeDist.y = RayDistToPlane( B.vPos, BView, A.vPos, planeNormB );    
    Cout.planeDist.z = RayDistToPlane( C.vPos, CView, A.vPos, planeNormC );    
    Dout.planeDist.x = RayDistToPlane( D.vPos, DView, A.vPos, planeNormD );
    
    CalcGradients( Aout, Bout, Cout, Dout, planeNormA, planeNormB, planeNormC, planeNormD );
    
    DisplaceStream.Append( Cout );
    DisplaceStream.Append( Bout );
    DisplaceStream.Append( Aout );
    DisplaceStream.Append( Dout );
    DisplaceStream.Append( Cout );
    DisplaceStream.Append( Bout );
    DisplaceStream.RestartStrip();
}

VSDisplaceOut VSDisplaceMain(VSDisplaceIn input)
{
    VSDisplaceOut output;
  
    output.Pos = float4(input.Pos, 1);
    output.Norm = RIGID_NORMAL_V( input.Norm );
    output.Tex = float3( input.Tex.zw, 0 );
    
    return output;
}

[maxvertexcount(18)]
void GSDisplaceMain( triangle VSDisplaceOut In[3], inout TriangleStream<PSDisplaceIn> DisplaceStream )
{
    //Extrude along the Normals
    VSDisplaceOut v[6];
    
    [unroll] for( int i=0; i<3; i++ )
    {
		v[i] = (VSDisplaceOut)0;
		v[i+3] = (VSDisplaceOut)0;
		
        float4 PosNew = In[i].Pos;
        float4 PosExt = PosNew + float4(In[i].Norm * g_MaxDisplacement,0);
        v[i].vPos = mul( PosNew.xyz, World );
        v[i+3].vPos = mul( PosExt.xyz, World );
        
        v[i].Pos = mul( PosNew, WorldViewProjection );
        v[i+3].Pos = mul( PosExt, WorldViewProjection );
        
        v[i].Tex = float3(In[i].Tex.xy,0);
        v[i+3].Tex = float3(In[i].Tex.xy,1);
        
        v[i].Norm = In[i].Norm;
        v[i+3].Norm = In[i].Norm;                
    }

    //Don't extrude anything that's facing too far away from us
    //Just saves geometry generation
    float3 AB = In[1].Pos - In[0].Pos;
    float3 AC = In[2].Pos - In[0].Pos;
    float3 triNorm = cross( AB, AC );
    float lenTriNorm = length( triNorm );
    triNorm /= lenTriNorm;
   
    // Make sure that our prism hasn't "flipped" on itself after the extrusion
    AB = v[4].vPos - v[3].vPos;
    AC = v[5].vPos - v[3].vPos;
    float3 topNorm = cross( AB, AC );
    float lenTop = length( topNorm );
    topNorm /= lenTop;
    if( 
		lenTriNorm < 0.005f ||						//avoid tiny triangles
        dot( topNorm, triNorm ) < 0.95f ||			//make sure the top of our prism hasn't flipped
        abs((lenTop-lenTriNorm)/lenTriNorm) > 10.0f//11.0f	//make sure we don't balloon out too much
        )
    {
        [unroll] for( int i=0; i<3; i++ )
        {
            float4 PosNew = In[i].Pos;
            float4 PosExt = PosNew + float4(In[i].Norm*g_MinDisplacement,0);
            v[i].vPos = PosNew.xyz;
            v[i+3].vPos = PosExt.xyz;
            
            v[i].Pos = mul( PosNew, WorldViewProjection );
            v[i+3].Pos = mul( PosExt, WorldViewProjection );
        }
    }
        
    // Create 3 tetrahedra
    GSCreateTetra( v[4], v[5], v[0], v[3], DisplaceStream );
    GSCreateTetra( v[5], v[0], v[1], v[4], DisplaceStream );
    GSCreateTetra( v[0], v[1], v[2], v[5], DisplaceStream );
}

#define MAX_DIST 1000000000.0f
#define MAX_STEPS 16
#define MIN_STEPS 4
#define STEP_SIZE (1.0f/2048.0f)
#define OFFSET_MAX 1.0f	//maximum amount of texel space we can cover in one march
float4 PSDisplaceMain(PSDisplaceIn input) : SV_Target
{	    
	float4 modDist = float4(0,0,0,0);
    modDist.x = input.planeDist.x > 0 ? input.planeDist.x : MAX_DIST;
    modDist.y = input.planeDist.y > 0 ? input.planeDist.y : MAX_DIST;
    modDist.z = input.planeDist.z > 0 ? input.planeDist.z : MAX_DIST;
    modDist.w = input.planeDist.w > 0 ? input.planeDist.w : MAX_DIST;
    
    // find distance to the rear of the tetrahedron
    float fDist = min( min( min( modDist.x, modDist.y ), modDist.z ), modDist.w );
        
    // find the texture coords of the entrance point
    float3 texEnter;
    float3 relPos = input.vPos-input.pos0;
    texEnter.x = dot( input.Gtx.xyz,relPos.xyz ) + input.Tex.x;
    texEnter.y = dot( input.Gty.xyz,relPos.xyz ) + input.Tex.y;
    texEnter.z = dot( input.Gtz.xyz,relPos.xyz ) + input.Tex.z;
        
    // find the exit position
    float3 eye = EyeInWorld;
        
    float3 viewExitDir = normalize( input.vPos - eye )*fDist;        
    float3 viewExit = input.vPos + viewExitDir;
        
    // find the texture coords of the exit point
    float3 texExit;
    relPos = viewExit-input.pos0;
    texExit.x = dot( input.Gtx.xyz,relPos.xyz ) + input.Tex.x;
    texExit.y = dot( input.Gty.xyz,relPos.xyz ) + input.Tex.y;
    texExit.z = dot( input.Gtz.xyz,relPos.xyz ) + input.Tex.z;
        
    // March along the Texture space view ray until we either hit something
    // or we exit the tetrahedral prism
    float3 tanGrad = texExit - texEnter;
    float fTanDist = length( float3(tanGrad.xy,0) );	//length in 2d texture space
	      
    if( fTanDist > OFFSET_MAX )
		discard;
			        
    int iSteps = min( ceil( fTanDist / STEP_SIZE ), MAX_STEPS-MIN_STEPS ) + MIN_STEPS;        
    tanGrad /= iSteps-1.0f;
    float3 TexCoord = float3(0,0,0);
    bool bFound = false;
		
    float height = 0;
    int i = 0;
    for( i=0; (i<iSteps && !bFound); i++ )
    {
        TexCoord = texEnter + i*tanGrad;        
#if 0
        float4 colorDiff = tDiffuseMap.SampleLevel( g_samPoint, float3(TexCoord.xy, 0), 0 );
        height = 1.0f - ( ( colorDiff.r + colorDiff.g + colorDiff.b ) / 3.0f );
#else        
        height = tDisplacementMap.SampleLevel( g_samPoint, float3(TexCoord.xy, 0), 0 );
#endif        
        height = max( height, g_MinDisplacement );
        
        if( TexCoord.z <= height )
            bFound = true;
    }
    if( !bFound )
		discard;
	
	return tDiffuseMap.Sample( g_samLinear, float3(TexCoord.xy, 0) );
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_TECH RenderDisplaced
< SHADER_VERSION_44 >
{
    pass P0
    {        
        SetVertexShader( CompileShader( vs_4_0, VSDisplaceMain() ) );
        SetGeometryShader( CompileShader( gs_4_0, GSDisplaceMain() ) );
        SetPixelShader( CompileShader( ps_4_0, PSDisplaceMain() ) );
		        		        
        DXC_RASTERIZER_SOLID_BACK;
        DXC_BLEND_RGBA_NO_BLEND;
        DXC_DEPTHSTENCIL_ZREAD_ZWRITE;
    }  
}