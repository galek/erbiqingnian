// Testbed effect


//#define STATIC_BRANCH


float4x4 WorldViewProjection;
float4x4 World;
float4x4 View;
float4x4 Proj;
float4 gvEyeInWorld;

sampler sNormalMap;

//#include "Dx9/_Branch.fxh"
//#include "Dx9/_BackgroundCommon.fxh"



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



struct RIGID_VERTEX
{
	float4	Position	: POSITION;
	float3	Normal		: NORMAL;
	float2	Texture0	: TEXCOORD0;
	float3	Tangent		: TANGENT;
	float3	Binormal	: BINORMAL;
};



struct PSINTERFACE
{
	float3	Pw			: TEXCOORD0;
	float4	Texture0	: TEXCOORD1;
	float3	Nw			: TEXCOORD2;
	float3	Tw			: TEXCOORD3;
	float3	Bw			: TEXCOORD4;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


void VS_Common(
	in half3	P,
	in half3	N,
	in half3	T,
	in half3	B,
	in half4	Tex,
	inout PSINTERFACE Out,
	uniform bool bNormalMap )
{
	Out.Pw			= mul( half4(P,1), World ).xyz;
	Out.Nw			= mul( N, World );
	Out.Texture0	= Tex;

	if ( bNormalMap )
	{
		Out.Tw		= mul( T, World );
		Out.Bw		= mul( B, World );
	}
}



void VS_Main_Rigid(
	in RIGID_VERTEX In,
	out float4 PositionRHW : POSITION,
	out PSINTERFACE Out,
	uniform bool bNormalMap )
{
	Out = (PSINTERFACE) 0;

	PositionRHW	= mul( In.Position, WorldViewProjection );

	VS_Common(
		In.Position,
		In.Normal,
		In.Tangent,
		In.Binormal,
		half4( In.Texture0, 0,0 ),
		Out,
		bNormalMap
		);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------

#define UNPACK_XY_NORMAL( N )				N.z = sqrt( half(1) - N.x*N.x - N.y*N.y )

half3 ReadNormalMap( half2 vCoords )
{
#ifdef ENGINE_TARGET_DX10
	// BC5 compresses 2-channels (x & y), so we need to calculate Z.
	// We read from r & g because no swizzle is done when expanding in the shader.
	half3 N	= tex2D( sNormalMap, vCoords ).rgb * half(2) - half(1);
#else
	// uses the DXT5 normal map 2-channel compression method -- calculates Z	
	half3 N	= tex2D( sNormalMap, vCoords ).agb * half(2) - half(1);
#endif	
	UNPACK_XY_NORMAL( N );
	return N;
}

half3 XformVecByColMatrix_half( half3 column1, half3 column2, half3 column3, half3 invec)
{
    half3   outVector;
    half3x3 xformMatrix  =  half3x3(column1.x, column2.x, column3.x,
                                    column1.y, column2.y, column3.y, 
                                    column1.z, column2.z, column3.z);
    outVector = mul( xformMatrix, invec);
    
    return outVector;
}





void PS_GetWorldNormalFromMap(
	in	half2 Tex,
	in	half3 T,
	in	half3 B,
	in	half3 N,
	out half3 Nw )
{
	half3 Nt = ReadNormalMap( Tex );

	half3 vTangentVec  = normalize( T );
	half3 vBinormalVec = normalize( B );
	half3 vNormalVec   = normalize( N );

	// xform normalTS into whatever space tangent, binormal, and normal vecs are in (world, in this case)
	Nw = XformVecByColMatrix_half( vTangentVec, vBinormalVec, vNormalVec, Nt );

	Nw = normalize(Nw);
}


void PS_Main(
	in PSINTERFACE In,
	out float4 Color : COLOR,
	uniform bool bNormalMap,
	uniform bool bSpecular )
{
	half3 Nw;

	if ( bNormalMap )
	{
		PS_GetWorldNormalFromMap(
			In.Texture0.xy,
			In.Tw,
			In.Bw,
			In.Nw,
			Nw );
	} else
	{
		Nw = normalize( In.Nw );
	}

	half3 Pw = In.Pw.xyz;
	half3 Vw = normalize( gvEyeInWorld - Pw );

	half3 Dc = dot( Vw, Nw ) * Pw;
	//half3 Sc;
	//LightPoint( 0, Nw, Pw, Vw, Dc, Sc, bSpecular );


	Color = float4( Dc, 0.005f );
}



technique ShaderPerf
{
	pass Main
	{
		VertexShader = compile vs_3_0 VS_Main_Rigid ( false );
		PixelShader  = compile ps_3_0 PS_Main		( true, false );
	}
}