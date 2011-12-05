//
// BackgroundCommon - header file for background shader functions
//


//--------------------------------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------------------------------

#define MAX_POINT_LIGHTS		4
#define MAX_DIR_LIGHTS			3

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// STRUCTS
//--------------------------------------------------------------------------------------------


struct PointLight
{
	half4	P;	// position
	half4	C;	// color:3, strength:1
	half4	F;	// falloff:   x == far end / ( far start - far end ), y == 1 / ( far start - far end )
};

struct DirLight
{
	half4	L;	// direction
	half4	C;	// color:3, strength:1
};

struct SpecularResponse
{
#define sr_gloss			sresponse.x
#define sr_level			sresponse.y
#define sr_maskoverride		sresponse.z
#define sr_glow				sresponse.w
	half4	sresponse;							// gloss:1, level:1, maskoverride:1, glow:1
};

struct EnvironmentMapResponse
{
#define emr_level				emresponse.x
#define emr_glossthreshold		emresponse.y
#define emr_LODbias				emresponse.z
	half4	emresponse;							// level:1, glossthreshold:1, LODbias:1, ______:1
};

struct SelfIlluminationResponse
{
#define sir_level				siresponse.x
#define sir_glow				siresponse.w
	half4	siresponse;							// level:1, ______:1, ______:1, glow:1
};

struct Material
{
	SpecularResponse				specular;
	EnvironmentMapResponse			envmap;
	SelfIlluminationResponse		selfillum;
};



//--------------------------------------------------------------------------------------------
// GLOBALS
//--------------------------------------------------------------------------------------------

PointLight	gtPointLights[5];
DirLight	gtDirLights[3];
Material	gtMaterial;

half3		gvEyeInWorld;

//--------------------------------------------------------------------------------------------
// FUNCTIONS
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// NORMAL MAPPING
//--------------------------------------------------------------------------------------------

/*

void GetTangentToWorldMatrix(
	inout half3 T,
	inout half3 B,
	inout half3 N )
{
	VectorsTransform( T, B, N, World );
}


void GetTangentToWorldMatrixPacked(
	in half3 T,
	in half3 B,
	in half3 N,
	out half4 P1,
	out half4 P2 )
{
	half3 tT = T, tB = B, tN = N;
	VectorsTransform( tT, tB, tN, World );
	P1 = half4( tT, tN.x );
	P2 = half4( tB, tN.y );
}


void GetNormalInWorld(
	in half3 iN,
	out half3 oN )
{
	oN = mul( iN, World );
}


void GetTransformedNormal(
	in half3 M_1,
	in half3 M_2,
	in half3 M_3,
	in half3 N_t,
	out half3 N_w )
{
    // transform N_t into space described by column matrix M_*
    N_w = TangentToWorld( M_1, M_2, M_3, N_t );
}


void GetTransformedNormalPacked(
	in half4 P1,
	in half4 P2,
	in half3 N_t,
	out half3 N_w )
{
	half3 c_1 = normalize( P1.xyz );
	half3 c_2 = normalize( P2.xyz );
	half3 c_3 = half3( P1.w, P2.w, 0.f );
	UNPACK_XY_NORMAL( c_3 );

    // transform N_t into space described by column matrix M_*
    N_w = TangentToWorld( c_1, c_2, c_3, N_t );
}


*/


//--------------------------------------------------------------------------------------------
// LIGHTING
//--------------------------------------------------------------------------------------------

void AddDiffuseLight(
	in half3 N,
	in half3 I,
	in half3 C,
	inout half3 Diffuse )
{
	Diffuse += C * saturate( dot( N, I ) );
}




//--------------------------------------------------------------------------------------------
void LightPoint(
	uniform int nP,
	in half3 N,				// normal
	in half3 P,				// position
	in half3 V,				// view unit vector
	out half3 Dc,			// diffuse color
	out half3 Sc,			// specular color
	uniform bool bSpecular )
{
	PointLight	Pl	= gtPointLights[nP];
	Material	M	= gtMaterial;

	half3	I	= Pl.P - P;
	half	id	= length( I );
	half	kd	= saturate( Pl.F.x - id * Pl.F.y );
	half3	D	= kd * Pl.C;
	I			= normalize( I );

	if ( BRANCH( bSpecular ) )
	{
		half3 H = normalize( I + V );
		Sc = BRDF_Phong_Specular( N, H, D, M.specular.sr_level );
	} else
	{
		Sc = 0;
	}

	Dc = BRDF_Phong_Diffuse( N, I, D );
}












