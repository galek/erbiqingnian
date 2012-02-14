
#pragma once

#include "math/axisAlignedBox.h"

_NAMESPACE_BEGIN

// 顶点语义
enum MeshVertexFlag
{
	MIVF_POSITION       = (1<<0),
	MIVF_NORMAL         = (1<<1),
	MIVF_COLOR          = (1<<2),
	MIVF_TEXEL1         = (1<<3),
	MIVF_TEXEL2         = (1<<4),
	MIVF_TEXEL3         = (1<<5),
	MIVF_TEXEL4         = (1<<6),
	MIVF_TANGENT        = (1<<7),
	MIVF_BINORMAL       = (1<<8),
	MIVF_BONE_WEIGHTING = (1<<9),
	MIVF_RADIUS         = (1<<10),
	MIVF_INTERP1        = (1<<11),
	MIVF_INTERP2        = (1<<12),
	MIVF_INTERP3        = (1<<13),
	MIVF_INTERP4        = (1<<14),
	MIVF_INTERP5        = (1<<15),
	MIVF_INTERP6        = (1<<16),
	MIVF_INTERP7        = (1<<17),
	MIVF_INTERP8        = (1<<18),
	MIVF_ALL = (MIVF_POSITION | MIVF_NORMAL | MIVF_COLOR | MIVF_TEXEL1 | MIVF_TEXEL2 | 
			MIVF_TEXEL3 | MIVF_TEXEL4 | MIVF_TANGENT | MIVF_BINORMAL | MIVF_BONE_WEIGHTING )
};

//////////////////////////////////////////////////////////////////////////

enum MeshSerializeFormat
{
	MSF_EZMESH, 
	MSF_OGRE3D, 

	MSF_LAST
};

//////////////////////////////////////////////////////////////////////////

class MeshVertex 
{
public:
	MeshVertex(void);

	bool  operator == (const MeshVertex &v) const;

	Vector3			mPos;
	Vector3			mNormal;

	uint32			mColor;

	Vector2         mTexel1;
	Vector2			mTexel2;
	Vector2			mTexel3;
	Vector2			mTexel4;

	Quaternion		mInterp1;
	Quaternion      mInterp2;
	Quaternion      mInterp3;
	Quaternion      mInterp4;
	Quaternion      mInterp5;
	Quaternion      mInterp6;
	Quaternion      mInterp7;
	Quaternion      mInterp8;

	Vector3			mTangent;
	Vector3         mBiNormal;
	Vector4		    mWeight;

	unsigned short	mBone[4];

	scalar          mRadius;
};

//////////////////////////////////////////////////////////////////////////

class MeshBone 
{
public:
	MeshBone(void);

	void				Set(const char* name,int32 parentIndex,const Vector3& pos,const Quaternion& rot,const Vector3& scale);

	void				Identity(void);

	void				SetName(const char *name)
	{
		mName = name;
	}

	const char*			GetName(void) const { return mName; };

	int32				GetParentIndex(void) const { return mParentIndex; };
	
	const Vector3&		GetPosition(void) const { return mPosition; };

	const Quaternion&	GetOrientation(void) const { return mOrientation; };

	const Vector3&		GetScale(void) const { return mScale; };

	void				getAngleAxis(Radian &angle,Vector3& axis) const
	{
		mOrientation.ToAngleAxis(angle,axis);
	}

	void				setOrientationFromAxisAngle(const Vector3& axis,const Radian& angle)
	{
		mOrientation.FromAngleAxis(angle,axis);
	}

	const char*		mName;

	int32           mParentIndex;

	Vector3			mPosition;

	Vector3			mScale;

	Quaternion		mOrientation;
};

//////////////////////////////////////////////////////////////////////////

class MeshEntry
{
public:
	MeshEntry(void)
	{
		mName = "";
		mBone = 0;
	}

	const char*	mName;

	// 与模型关联的骨骼
	int32		mBone;
};

//////////////////////////////////////////////////////////////////////////

class MeshSkeleton 
{
public:
	MeshSkeleton(void)
	{
		mName = "";
		mBoneCount = 0;
		mBones = 0;
	}

	void SetName(const char *name)
	{
		mName = name;
	}

	void SetBones(int32 bcount,MeshBone *bones)
	{
		mBoneCount = bcount;
		mBones     = bones;
	}

	int32			GetBoneCount(void) const { return mBoneCount; };

	const MeshBone& GetBone(int32 index) const { return mBones[index]; };

	MeshBone*		GetBonePtr(int32 index) const { return &mBones[index]; };

	void			SetBone(int32 index,const MeshBone &b) { mBones[index] = b; };

	const char*		GetName(void) const { return mName; };

	const char*		mName;

	int32			mBoneCount;

	MeshBone*		mBones;
};

//////////////////////////////////////////////////////////////////////////

class MeshAnimPose 
{
public:
	MeshAnimPose(void);

	void SetPose (const Vector3& pos, const Quaternion& quat, const Vector3& scale);

	void Sample (Vector3& pos, Quaternion& quat, Vector3& scale) const;

	void getAngleAxis(Radian& angle,Vector3& axis) const;

	Vector3		mPos;

	Quaternion	mQuat;

	Vector3		mScale;
};

//////////////////////////////////////////////////////////////////////////

class MeshAnimTrack 
{
public:

	MeshAnimTrack(void);

	void			SetName(const char *name);

	void			SetPose(int32 frame,const Vector3& pos,const Quaternion& quat,const Vector3& scale);

	const char *	GetName(void) const { return mName; };

	void			SampleAnimation(int32 frame,Vector3& pos,Quaternion& quat,Vector3& scale) const;

	int32			GetFrameCount(void) const { return mFrameCount; };

	MeshAnimPose *	GetPose(int32 index) { return &mPose[index]; };

	const char*		mName;

	int32			mFrameCount;

	scalar			mDuration;

	scalar			mDtime;

	MeshAnimPose*	mPose;
};

//////////////////////////////////////////////////////////////////////////

class MeshAnimation 
{
public:

	MeshAnimation(void);

	void					SetName(const char *name)
	{
		mName = name;
	}

	void					SetTrackName(int32 track,const char *name);

	void					SetTrackPose(int32 track,int32 frame,const Vector3& pos,const Quaternion& quat,const Vector3& scale);

	const char*				GetName(void) const { return mName; }

	const MeshAnimTrack*	LocateTrack(const char *name) const;

	int32					GetFrameIndex(scalar t) const;

	int32					GetTrackCount(void) const { return mTrackCount; }

	scalar					GetDuration(void) const { return mDuration; }

	MeshAnimTrack *			GetTrack(int32 index);;

	int32					GetFrameCount(void) const { return mFrameCount; }

	scalar					GetDtime(void) const { return mDtime; }

	const char*				mName;

	int32					mTrackCount;

	int32					mFrameCount;

	scalar					mDuration;

	scalar					mDtime;

	MeshAnimTrack**			mTracks;
};

//////////////////////////////////////////////////////////////////////////

class MeshMaterial
{
public:
	MeshMaterial(void)
	{
		mName = 0;
		mMetaData = 0;
	}
	const char *mName;
	const char *mMetaData;
};

//////////////////////////////////////////////////////////////////////////

class SubMesh
{
public:
	SubMesh(void)
	{
		mMaterialName = 0;
		mMaterial     = 0;
		mVertexFlags  = 0;
		mTriCount     = 0;
		mIndices      = 0;
	}

	const char          *mMaterialName;
	MeshMaterial        *mMaterial;
	AxisAlignedBox		mAABB;
	uint32				mVertexFlags;
	uint32				mTriCount;
	uint32*				mIndices;
};

//////////////////////////////////////////////////////////////////////////

class Mesh
{
public:
	Mesh(void)
	{
		mName         = 0;
		mSkeletonName = 0;
		mSkeleton     = 0;
		mSubMeshCount = 0;
		mSubMeshes    = 0;
		mVertexFlags  = 0;
		mVertexCount  = 0;
		mVertices     = 0;

	}
	const char*			mName;

	const char*			mSkeletonName;

	MeshSkeleton*		mSkeleton;

	AxisAlignedBox		mAABB;

	uint32				mSubMeshCount;

	SubMesh**			mSubMeshes;

	uint32				mVertexFlags;

	uint32				mVertexCount;

	MeshVertex*			mVertices;

};

//////////////////////////////////////////////////////////////////////////

class MeshRawTexture
{
public:
	MeshRawTexture(void)
	{
		mName = 0;
		mData = 0;
		mWidth = 0;
		mHeight = 0;
	}

	const char*	mName;

	uint8*		mData;

	uint32		mWidth;

	uint32		mHeight;
};

//////////////////////////////////////////////////////////////////////////

class MeshInstance
{
public:
	MeshInstance(void)
	{
		mMeshName = 0;
		mMesh     = 0;
		mPosition = Vector3::ZERO;
		mRotation = Quaternion::ZERO;
		mScale	  = Vector3::ZERO;
	}

	const char  *mMeshName;

	Mesh        *mMesh;

	Vector3		mPosition;

	Quaternion	mRotation;

	Vector3		mScale;
};

//////////////////////////////////////////////////////////////////////////

class MeshUserData
{
public:
	MeshUserData(void)
	{
		mUserKey = 0;
		mUserValue = 0;
	}
	const char *mUserKey;
	const char *mUserValue;
};

//////////////////////////////////////////////////////////////////////////

class MeshUserBinaryData
{
public:
	MeshUserBinaryData(void)
	{
		mName     = 0;
		mUserData = 0;
		mUserLen  = 0;
	}
	const char*	mName;
	uint32		mUserLen;
	uint8*		mUserData;
};

//////////////////////////////////////////////////////////////////////////

// 四面体
class MeshTetra
{
public:
	MeshTetra(void)
	{
		mTetraName  = 0;
		mMeshName   = 0; // 与四面体关联的模型
		mMesh       = 0;
		mTetraCount = 0;
		mTetraData  = 0;
	}

	const char*			mTetraName;

	const char*			mMeshName;

	AxisAlignedBox		mAABB;

	Mesh*				mMesh;

	uint32				mTetraCount;

	scalar*				mTetraData;
};

// 文件版本，用于二进制序列化
#define MESH_SYSTEM_VERSION 1

// 模型碰撞体类型
enum MeshCollisionType
{
	MCT_BOX,
	MCT_SPHERE,
	MCT_CAPSULE,
	MCT_CONVEX,
	MCT_LAST
};

//////////////////////////////////////////////////////////////////////////

class MeshCollision 
{
public:
	MeshCollision(void)
	{
		mType = MCT_LAST;
		mName = 0;
		mTransform.setIdentity();
	}

	MeshCollisionType	getType(void) const { return mType; };

	MeshCollisionType	mType;

	const char*			mName;

	Matrix4				mTransform;
};

//////////////////////////////////////////////////////////////////////////

class MeshCollisionBox : public MeshCollision
{
public:
	MeshCollisionBox(void)
	{
		mType = MCT_BOX;
		mSides = Vector3::UNIT_SCALE;
	}
	Vector3	mSides;
};

//////////////////////////////////////////////////////////////////////////

class MeshCollisionSphere : public MeshCollision
{
public:
	MeshCollisionSphere(void)
	{
		mType = MCT_SPHERE;
		mRadius = 1;
	}
	scalar mRadius;
};

//////////////////////////////////////////////////////////////////////////

class MeshCollisionCapsule : public MeshCollision
{
public:
	MeshCollisionCapsule(void)
	{
		mType = MCT_CAPSULE;
		mRadius = 1;
		mHeight = 1;
	}
	scalar  mRadius;
	scalar  mHeight;
};

//////////////////////////////////////////////////////////////////////////

class MeshConvex
{
public:
	MeshConvex(void)
	{
		mVertexCount = 0;
		mVertices = 0;
		mTriCount = 0;
		mIndices = 0;
	}
	uint32		mVertexCount;
	scalar*		mVertices;
	uint32		mTriCount;
	uint32*		mIndices;
};

//////////////////////////////////////////////////////////////////////////

class MeshCollisionConvex : public MeshCollision, public MeshConvex
{
public:
	MeshCollisionConvex(void)
	{
		mType = MCT_CONVEX;
	}
};

//////////////////////////////////////////////////////////////////////////

class MeshCollisionRepresentation 
{
public:

	MeshCollisionRepresentation(void)
	{
		mName = 0;
		mInfo = 0;
		mCollisionCount = 0;
		mCollisionGeometry = 0;
	}

	const char*		mName;

	const char*		mInfo;

	uint32			mCollisionCount;

	MeshCollision**	mCollisionGeometry;
};

//////////////////////////////////////////////////////////////////////////

class MeshSystem
{
public:
	MeshSystem(void)
	{
		mAssetName           = 0;
		mAssetInfo           = 0;
		mTextureCount        = 0;
		mTextures            = 0;
		mSkeletonCount       = 0;
		mSkeletons           = 0;
		mAnimationCount      = 0;
		mAnimations          = 0;
		mMaterialCount       = 0;
		mMaterials           = 0;
		mMeshCount           = 0;
		mMeshes              = 0;
		mMeshInstanceCount   = 0;
		mMeshInstances       = 0;
		mUserDataCount       = 0;
		mUserData            = 0;
		mUserBinaryDataCount = 0;
		mUserBinaryData      = 0;
		mTetraMeshCount      = 0;
		mTetraMeshes         = 0;
		mMeshSystemVersion   = MESH_SYSTEM_VERSION;
		mAssetVersion        = 0;
		mMeshCollisionCount  = 0;
		mMeshCollisionRepresentations = 0;
		mPlane[0] = 1;
		mPlane[1] = 0;
		mPlane[2] = 0;
		mPlane[3] = 0;
	}


	const char           *mAssetName;
	const char           *mAssetInfo;
	int32				mMeshSystemVersion;
	int32				mAssetVersion;
	AxisAlignedBox		mAABB;
	uint32				mTextureCount;  
	MeshRawTexture      **mTextures;    

	uint32				mTetraMeshCount;
	MeshTetra           **mTetraMeshes; 

	uint32				mSkeletonCount; 
	MeshSkeleton        **mSkeletons;   

	uint32				mAnimationCount;
	MeshAnimation       **mAnimations;

	uint32				mMaterialCount; 
	MeshMaterial		*mMaterials;

	uint32				mUserDataCount;
	MeshUserData		**mUserData;

	uint32				mUserBinaryDataCount;
	MeshUserBinaryData  **mUserBinaryData;

	uint32				mMeshCount;
	Mesh                **mMeshes;

	uint32				mMeshInstanceCount;
	MeshInstance		*mMeshInstances;

	uint32				mMeshCollisionCount;
	MeshCollisionRepresentation **mMeshCollisionRepresentations;

	scalar				mPlane[4];

};

//////////////////////////////////////////////////////////////////////////

class MeshBoneInstance 
{
public:
	MeshBoneInstance(void)
	{
		mBoneName = "";
	}

	void composeInverse(void)
	{
		mInverseTransform = mTransform.inverse();
	}

	const char*	mBoneName;
	int32		mParentIndex;
	Matrix4		mLocalTransform;
	Matrix4		mAnimTransform;
	Matrix4		mTransform;
	Matrix4		mCompositeAnimTransform;
	Matrix4		mInverseTransform;
};

//////////////////////////////////////////////////////////////////////////

class MeshSkeletonInstance
{
public:
	MeshSkeletonInstance(void)
	{
		mName      = "";
		mBoneCount = 0;
		mBones     = 0;
	}

	const char			*mName;
	int32				mBoneCount;
	MeshBoneInstance	*mBones;
};

//////////////////////////////////////////////////////////////////////////

class VertexIndex
{
public:
	virtual uint32			getIndex(const Vector3& pos,bool &newPos) = 0;

	virtual const scalar*   getVertices(void) const = 0;

	virtual const scalar*   getVertex(uint32 index) const = 0;

	virtual uint32			getVcount(void) const = 0;
};

_NAMESPACE_END