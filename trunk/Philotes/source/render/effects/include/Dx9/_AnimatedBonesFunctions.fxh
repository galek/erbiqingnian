//
// Functions that depend on the Bones array, which
// differs in size for different shader versions.
//

//////////////////////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////////////////////

void GetSkinningBones(out float3x4 Bone0, out float3x4 Bone1, out float3x4 Bone2, int4 Indices)
{
	Bone0[0] = Bones[Indices.x];
	Bone0[1] = Bones[Indices.x + 1];
	Bone0[2] = Bones[Indices.x + 2];

	Bone1[0] = Bones[Indices.y];
	Bone1[1] = Bones[Indices.y + 1];
	Bone1[2] = Bones[Indices.y + 2];

	Bone2[0] = Bones[Indices.z];
	Bone2[1] = Bones[Indices.z + 1];
	Bone2[2] = Bones[Indices.z + 2];
}

#if defined(ENGINE_TARGET_DX10) && defined(ENGINE_BONES_PREV_DECLARED)
void GetSkinningBonesPrev(out float3x4 Bone0, out float3x4 Bone1, out float3x4 Bone2, int4 Indices)
{
	Bone0[0] = BonesPrev[Indices.x];
	Bone0[1] = BonesPrev[Indices.x + 1];
	Bone0[2] = BonesPrev[Indices.x + 2];

	Bone1[0] = BonesPrev[Indices.y];
	Bone1[1] = BonesPrev[Indices.y + 1];
	Bone1[2] = BonesPrev[Indices.y + 2];

	Bone2[0] = BonesPrev[Indices.z];
	Bone2[1] = BonesPrev[Indices.z + 1];
	Bone2[2] = BonesPrev[Indices.z + 2];
}
#endif

float3 WeightSkinnedVertex_Float(float3x4 Bone0, float3x4 Bone1, float3x4 Bone2, float3 Weights, float3 Position)
{
	return
		Weights.x * mul(Bone0, float4( Position, 1.0f ) ) +
		Weights.y * mul(Bone1, float4( Position, 1.0f ) ) +
		Weights.z * mul(Bone2, float4( Position, 1.0f ) );
}

half3 WeightSkinnedVertex_Half(float3x4 Bone0, float3x4 Bone1, float3x4 Bone2, float3 Weights, half3 Position)
{
	return
		Weights.x * mul((half3x3)Bone0, Position ) +
		Weights.y * mul((half3x3)Bone1, Position ) +
		Weights.z * mul((half3x3)Bone2, Position );
}

//--------------------------------------------------------------------------------------------

void GetPositionNormalAndObjectMatrix( 
	out float4 Position, 
	out half3 Normal,
	out half3x3 objToTangentSpace, 
	VS_ANIMATED_INPUT In )
{
	float3x4 Bone0, Bone1, Bone2;
	GetSkinningBones(Bone0, Bone1, Bone2, In.Indices);
	Position.xyz = WeightSkinnedVertex_Float(Bone0, Bone1, Bone2, In.Weights, In.Position);
	Position.w = 1.0f;
	Normal = normalize( WeightSkinnedVertex_Half(Bone0, Bone1, Bone2, In.Weights, ANIMATED_NORMAL) );
	half3 Binormal = normalize( WeightSkinnedVertex_Half(Bone0, Bone1, Bone2, In.Weights, ANIMATED_BINORMAL) );
	half3 Tangent = normalize( WeightSkinnedVertex_Half(Bone0, Bone1, Bone2, In.Weights, ANIMATED_TANGENT) );
	
    objToTangentSpace[0] = Tangent;
    objToTangentSpace[1] = Binormal;
    objToTangentSpace[2] = Normal;
}

//--------------------------------------------------------------------------------------------

void GetPositionEx( out float4 Position, VS_ANIMATED_INPUT In, int4 Indices )
{
	float3x4 Bone0, Bone1, Bone2;
	GetSkinningBones(Bone0, Bone1, Bone2, Indices);
	Position.xyz = WeightSkinnedVertex_Float(Bone0, Bone1, Bone2, In.Weights, In.Position);
	Position.w = 1.0f;
}

//--------------------------------------------------------------------------------------------

#if defined(ENGINE_TARGET_DX10) && defined(ENGINE_BONES_PREV_DECLARED)
void GetPositionPrev( out float4 Position, VS_ANIMATED_INPUT In )
{
	float3x4 Bone0, Bone1, Bone2;
	GetSkinningBonesPrev(Bone0, Bone1, Bone2, In.Indices);
	Position.xyz = WeightSkinnedVertex_Float(Bone0, Bone1, Bone2, In.Weights, In.Position);
	Position.w = 1.0f;
}
#endif

//--------------------------------------------------------------------------------------------

void GetPosition( out float4 Position, VS_ANIMATED_INPUT In )
{
	GetPositionEx(Position, In, In.Indices);
}

//--------------------------------------------------------------------------------------------

void GetPosition_Z( out float4 Position, VS_ANIMATED_ZBUFFER_INPUT In )
{
	float3x4 Bone0, Bone1, Bone2;
	GetSkinningBones(Bone0, Bone1, Bone2, In.Indices);
	Position.xyz = WeightSkinnedVertex_Float(Bone0, Bone1, Bone2, In.Weights, In.Position);
	Position.w = 1.0f;
}

//--------------------------------------------------------------------------------------------

void GetPositionAndNormalEx( out float4 Position, out half3 Normal, VS_ANIMATED_INPUT In, int4 Indices )
{
	float3x4 Bone0, Bone1, Bone2;
	GetSkinningBones(Bone0, Bone1, Bone2, Indices);
	Position.xyz = WeightSkinnedVertex_Float(Bone0, Bone1, Bone2, In.Weights, In.Position);
	Position.w = 1.0f;
	Normal = normalize( WeightSkinnedVertex_Half(Bone0, Bone1, Bone2, In.Weights, ANIMATED_NORMAL) );
}

//--------------------------------------------------------------------------------------------

void GetPositionAndNormal( out float4 Position, out half3 Normal, VS_ANIMATED_INPUT In )
{
	GetPositionAndNormalEx(Position, Normal, In, In.Indices);
}

//--------------------------------------------------------------------------------------------

void GetPositionCheap( out float4 Position, VS_ANIMATED_POS_TEX_INPUT In )
{
	float3x4 Bone0;
	Bone0[0] = Bones[In.Indices.x];
	Bone0[1] = Bones[In.Indices.x + 1];
	Bone0[2] = Bones[In.Indices.x + 2];

	Position.xyz = mul( Bone0, float4( In.Position, 1.0f ) );
	Position.w = 1.0f;	
}

//--------------------------------------------------------------------------------------------

void GetPositionCheap( out float4 Position, VS_ANIMATED_INPUT In )
{
	float3x4 Bone0;
	Bone0[0] = Bones[In.Indices.x];
	Bone0[1] = Bones[In.Indices.x + 1];
	Bone0[2] = Bones[In.Indices.x + 2];

	Position.xyz = mul( Bone0, float4( In.Position, 1.0f ) );
	Position.w = 1.0f;	
}

//--------------------------------------------------------------------------------------------

PS11_DIFFUSE_INPUT VS11_NPS_ACTOR ( 
	VS11_ANIMATED_INPUT In, 
	uniform int nPointLights, 
	uniform bool bSkinned )
{
	PS11_DIFFUSE_INPUT Out = (PS11_DIFFUSE_INPUT)0;
	
	float4 Position;
	
	if ( bSkinned )
	{
		float3x4 Bone0, Bone1, Bone2;
		GetSkinningBones(Bone0, Bone1, Bone2, In.Indices);
		Position.xyz = WeightSkinnedVertex_Float(Bone0, Bone1, Bone2, In.Weights, In.Position);
		Position.w = 1.0f;
	}
	else
	{
		Position = float4( In.Position.xyz, 1.0f );
	}

 	Out.Position = mul( Position, WorldViewProjection );
 	
	Out.DiffuseMap  = In.DiffuseMap;
	Out.LightMap    = In.DiffuseMap;                                       

	Out.Diffuse = 1.0f;
	
	return Out;
}
