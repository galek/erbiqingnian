
#include "gearsMeshSerial.h"

_NAMESPACE_BEGIN

GearsMeshBuilder::GearsMeshBuilder( KeyValueIni *ini,const char *meshName,const void *data,
								   uint32 dlen, MeshImporter *mi,const char *options,
								   MeshImportApplicationResource *appResource )
{
	mINI = ini;
	mCurrentMesh = 0;
	mCurrentCollision = 0;
	mAppResource = appResource;
	importAssetName(meshName,0);
	mi->importMesh(meshName,data,dlen,this,options,appResource);
	gather();
}

GearsMeshBuilder::GearsMeshBuilder( MeshImportApplicationResource *appResource )
{
	mINI = 0;
	mCurrentMesh = 0;
	mCurrentCollision = 0;
	mAppResource = appResource;
}

GearsMeshBuilder::~GearsMeshBuilder( void )
{
	free(mMeshes);
	GearMeshVector::Iterator i;
	for (i=mMyMeshes.Begin(); i!=mMyMeshes.End(); ++i)
	{
		GearMesh *src = (*i);
		delete src;
	}

	if ( !mMyAnimations.IsEmpty() )
	{
		MeshAnimationVector::Iterator i;
		for (i=mMyAnimations.Begin(); i!=mMyAnimations.End(); ++i)
		{
			MeshAnimation *a = (*i);
			for (int32 j=0; j<a->mTrackCount; j++)
			{
				MeshAnimTrack *ma = a->mTracks[j];
				delete []ma->mPose;
				delete ma;
			}
			free(a->mTracks);
			delete a;
		}
	}

	if ( !mMySkeletons.IsEmpty() )
	{
		MeshSkeletonVector::Iterator i;
		for (i=mMySkeletons.Begin(); i!=mMySkeletons.End(); ++i)
		{
			MeshSkeleton *s = (*i);
			delete []s->mBones;
			delete s;
		}
	}
	if ( !mCollisionReps.IsEmpty() )
	{
		MeshCollisionRepresentationVector::Iterator i;
		for (i=mCollisionReps.Begin(); i!=mCollisionReps.End(); ++i)
		{
			GearMeshCollisionRepresentation *mcr = static_cast< GearMeshCollisionRepresentation *>(*i);
			delete mcr;
		}
	}
}

void GearsMeshBuilder::gather( void )
{
	gatherMaterials();
	
	mMaterialCount = (uint32)mMyMaterials.Size();

	if ( mMaterialCount )
		mMaterials = &mMyMaterials[0];
	else
		mMaterials = 0;

	mMeshInstanceCount = (uint32)mMyMeshInstances.Size();

	if ( mMeshInstanceCount )
		mMeshInstances = &mMyMeshInstances[0];
	else
		mMeshInstances = 0;

	mAnimationCount = (uint32)mMyAnimations.Size();
	if ( mAnimationCount )
		mAnimations = &mMyAnimations[0];
	else
		mAnimations = 0;

	int32 bone_count = 0;
	mSkeletonCount = (uint32)mMySkeletons.Size();
	if ( mSkeletonCount )
	{
		mSkeletons = &mMySkeletons[0];
		bone_count = mSkeletons[0]->mBoneCount;
	}
	else
		mSkeletons = 0;

	mMeshCount = (uint32)mMyMeshes.Size();
	if ( mMeshCount )
	{
		free(mMeshes);
		mMeshes    = (Mesh **)malloc(sizeof(Mesh *)*mMeshCount);
		Mesh **dst = mMeshes;
		GearMeshVector::Iterator i;
		for (i=mMyMeshes.Begin(); i!=mMyMeshes.End(); ++i)
		{
			GearMesh *src = (*i);
			src->gather(bone_count);
			*dst++ = static_cast< Mesh *>(src);
		}
	}
	else
	{
		mMeshes = 0;
	}

	mMeshCollisionCount = (uint32)mCollisionReps.Size();

	if ( mMeshCollisionCount )
	{
		mMeshCollisionRepresentations = &mCollisionReps[0];
		for (uint32 i=0; i<mMeshCollisionCount; i++)
		{
			MeshCollisionRepresentation *r = mMeshCollisionRepresentations[i];
			GearMeshCollisionRepresentation *mr = static_cast< GearMeshCollisionRepresentation *>(r);
			mr->gather();
		}
	}
	else
	{
		mMeshCollisionRepresentations = 0;
	}
}

void GearsMeshBuilder::gatherMaterials( void )
{
	mMyMaterials.Reset();
	uint32 mcount = (uint32)mMaterialMap.Size();
	mMyMaterials.Reserve(mcount);

	for (SizeT i = 0; i < mMaterialMap.Size(); i++)
	{
		MeshMaterial m;
		m.mName = mMaterialMap.KeyAtIndex(i);
		m.mMetaData = mMaterialMap.ValueAtIndex(i);
		mMyMaterials.Append(m);
	}
}

void GearsMeshBuilder::importUserData( const char *userKey,const char *userValue )
{
	ph_assert(0);
}

void GearsMeshBuilder::importUserBinaryData( const char *name,uint32 len,const uint8 *data )
{
	ph_assert(0);
}

void GearsMeshBuilder::importTetraMesh( const char *tetraName,const char *meshName,uint32 tcount,const scalar *tetraData )
{
	ph_assert(0);
}

void GearsMeshBuilder::importMaterial( const char *matName,const char *metaData )
{
	matName = getMaterialName(matName);
	mMaterialMap[matName] = metaData;
}

void GearsMeshBuilder::importAssetName( const char *assetName,const char *info )
{
	mAssetName = assetName;
	mAssetInfo = info;
}

void GearsMeshBuilder::importMesh( const char *meshName,const char *skeletonName )
{
	String m1	= meshName;
	String s1	= skeletonName;

	mCurrentMesh = 0;
	GearMeshVector::Iterator found;
	for (found=mMyMeshes.Begin(); found != mMyMeshes.End(); found++)
	{
		GearMesh *mm = (*found);
		if ( mm->mName == m1 )
		{
			mCurrentMesh = mm;
			mCurrentMesh->mSkeletonName = s1;
			break;
		}
	}
	if ( mCurrentMesh == 0 )
	{
		GearMesh *m = ph_new(GearMesh)(m1,s1);
		mMyMeshes.Append(m);
		mCurrentMesh = m;
	}
}

void GearsMeshBuilder::getCurrentMesh( const char *meshName )
{
	if ( meshName == 0 )
	{
		meshName = "default_mesh";
	}
	if ( mCurrentMesh == 0 || !mCurrentMesh->isSame(meshName) )
	{
		importMesh(meshName,0); // make it the new current mesh
	}
}

void GearsMeshBuilder::importPlane( const scalar *p )
{
	mPlane[0] = p[0];
	mPlane[1] = p[1];
	mPlane[2] = p[2];
	mPlane[3] = p[3];
}

void GearsMeshBuilder::importTriangle( const char *_meshName, const char *_materialName, uint32 vertexFlags, 
									  const MeshVertex &v1, const MeshVertex &v2, const MeshVertex &v3 )
{
	_materialName = getMaterialName(_materialName);

	getCurrentMesh(_meshName);

	mAABB.setVector(v1.mPos);
	mAABB.setVector(v2.mPos);
	mAABB.setVector(v3.mPos);

	mCurrentMesh->importTriangle(_materialName,vertexFlags,v1,v2,v3);
}

void GearsMeshBuilder::importIndexedTriangleList( const char *_meshName, const char *_materialName, uint32 vertexFlags, 
												 uint32 vcount, const MeshVertex *vertices, uint32 tcount, const uint32 *indices )
{
	_materialName = getMaterialName(_materialName);
	getCurrentMesh(_meshName);
	for (uint32 i=0; i<vcount; i++)
	{
		mAABB.setVector( vertices[i].mPos );
	}
	mCurrentMesh->importIndexedTriangleList(_materialName,vertexFlags,vcount,vertices,tcount,indices);
}

void GearsMeshBuilder::importAnimation( const MeshAnimation &animation )
{
	MeshAnimation *a = ph_new(MeshAnimation);
	a->mName = animation.mName;
	a->mTrackCount = animation.mTrackCount;
	a->mFrameCount = animation.mFrameCount;
	a->mDuration = animation.mDuration;
	a->mDtime = animation.mDtime;
	a->mTracks = (MeshAnimTrack **)malloc(sizeof(MeshAnimTrack *)*a->mTrackCount);
	for (int32 i=0; i<a->mTrackCount; i++)
	{
		const MeshAnimTrack &src =*animation.mTracks[i];
		MeshAnimTrack *t = ph_new(MeshAnimTrack);
		t->mName = src.mName;
		t->mFrameCount = src.mFrameCount;
		t->mDuration = src.mDuration;
		t->mDtime = src.mDtime;
		t->mPose = ph_new(MeshAnimPose)[t->mFrameCount];
		memcpy(t->mPose,src.mPose,sizeof(MeshAnimPose)*t->mFrameCount);
		a->mTracks[i] = t;
	}
	mMyAnimations.Append(a);
}

void GearsMeshBuilder::importSkeleton( const MeshSkeleton &skeleton )
{
	MeshSkeleton *sk = ph_new(MeshSkeleton);
	sk->mName = skeleton.mName;
	sk->mBoneCount = skeleton.mBoneCount;
	sk->mBones = 0;
	if ( sk->mBoneCount > 0 )
	{
		sk->mBones = ph_new(MeshBone)[sk->mBoneCount];
		MeshBone *dest = sk->mBones;
		const MeshBone *src = skeleton.mBones;
		for (uint32 i=0; i<(uint32)sk->mBoneCount; i++)
		{
			*dest = *src;
			dest->mName = src->mName;
			src++;
			dest++;
		}
	}
	mMySkeletons.Append(sk);
}

void GearsMeshBuilder::importMeshInstance( const char *meshName,const scalar pos[3],const scalar rotation[4],const scalar scale[3] )
{
	MeshInstance m;
	m.mMeshName = meshName;
	m.mPosition[0] = pos[0];
	m.mPosition[1] = pos[1];
	m.mPosition[2] = pos[2];
	m.mRotation[0] = rotation[0];
	m.mRotation[1] = rotation[1];
	m.mRotation[2] = rotation[2];
	m.mRotation[3] = rotation[3];
	m.mScale[0] = scale[0];
	m.mScale[1] = scale[1];
	m.mScale[2] = scale[2];
	mMyMeshInstances.Append(m);
}

void GearsMeshBuilder::importCollisionRepresentation( const String& name,const String& info )
{
	mCurrentCollision = 0;

	MeshCollisionRepresentationVector::Iterator i;
	for (i=mCollisionReps.Begin(); i!=mCollisionReps.End(); ++i)
	{
		GearMeshCollisionRepresentation *mcr = static_cast< GearMeshCollisionRepresentation *>(*i);
		if ( mcr->mName == name )
		{
			mCurrentCollision = mcr;
			break;
		}
	}
	if ( mCurrentCollision )
	{
		mCurrentCollision->mInfo = info;
	}
	else
	{
		GearMeshCollisionRepresentation *mcr = ph_new(GearMeshCollisionRepresentation)(name,info);
		MeshCollisionRepresentation *mr = static_cast< MeshCollisionRepresentation *>(mcr);
		mCollisionReps.Append(mr);
		mCurrentCollision = mcr;
	}
}

void GearsMeshBuilder::getCurrentRep( const String& name )
{
	if ( mCurrentCollision == 0 || mCurrentCollision->mName != name)
	{
		mCurrentCollision = 0;
		MeshCollisionRepresentationVector::Iterator i;
		for (i=mCollisionReps.Begin(); i!=mCollisionReps.End(); ++i)
		{
			GearMeshCollisionRepresentation *mcr = static_cast< GearMeshCollisionRepresentation *>(*i);
			if ( mcr->mName == name )
			{
				mCurrentCollision = mcr;
				break;
			}
		}
		if ( mCurrentCollision == 0 )
		{
			importCollisionRepresentation(name,0);
		}
	}
}

void GearsMeshBuilder::importSphere( const char *collision_rep, const char *boneName, 
									const Matrix4& transform, scalar radius )
{
	getCurrentRep(collision_rep);
	MeshCollisionSphere *c = ph_new(MeshCollisionSphere);
	c->mName = boneName;
	c->mTransform = transform;
	c->mRadius = radius;
	MeshCollision *mc = static_cast< MeshCollision *>(c);
	mCurrentCollision->mGeometries.Append(mc);
}

void GearsMeshBuilder::importCapsule( const char *collision_rep, const char *boneName, 
									 const Matrix4& transform, scalar radius, scalar height )
{
	getCurrentRep(collision_rep);
	MeshCollisionCapsule *c = ph_new(MeshCollisionCapsule);
	c->mName = boneName;
	c->mTransform = transform;
	c->mRadius = radius;
	c->mHeight = height;
	MeshCollision *mc = static_cast< MeshCollision *>(c);
	mCurrentCollision->mGeometries.Append(mc);
}

void GearsMeshBuilder::importOBB( const char *collision_rep, const char *boneName, 
								 const Matrix4& transform, const Vector3& sides )
{
	getCurrentRep(collision_rep);
	MeshCollisionBox *c = ph_new(MeshCollisionBox);
	c->mName = boneName;
	c->mTransform = transform;
	c->mSides = sides;
	MeshCollision *mc = static_cast< MeshCollision *>(c);
	mCurrentCollision->mGeometries.Append(mc);
}

void GearsMeshBuilder::importConvexHull( const char *collision_rep, const char *boneName, 
										const Matrix4& transform, uint32 vertex_count, 
										const scalar *vertices, uint32 tri_count, const uint32 *indices )
{
	getCurrentRep(collision_rep);
	MeshCollisionConvex *c = ph_new(MeshCollisionConvex);
	c->mName = boneName;
	c->mTransform = transform;
	c->mVertexCount = vertex_count;
	if ( c->mVertexCount )
	{
		c->mVertices = (scalar *)malloc(sizeof(scalar)*vertex_count*3);
		memcpy(c->mVertices,vertices,sizeof(scalar)*vertex_count*3);
	}
	c->mTriCount = tri_count;
	if ( c->mTriCount )
	{
		c->mIndices = (uint32 *)malloc(sizeof(uint32)*tri_count*3);
		memcpy(c->mIndices,indices,sizeof(uint32)*tri_count*3);
	}
	MeshCollision *mc = static_cast< MeshCollision *>(c);
	mCurrentCollision->mGeometries.Append(mc);
}

void GearsMeshBuilder::rotate( scalar rotX,scalar rotY,scalar rotZ )
{
	Matrix3 mat;
	mat.FromEulerAnglesXYZ(Radian(rotX),Radian(rotY),Radian(rotZ));
	Quaternion quat(mat);

	{
		MeshSkeletonVector::Iterator i;
		for (i=mMySkeletons.Begin(); i!=mMySkeletons.End(); ++i)
		{
			MeshSkeleton *ms = (*i);
			for (int32 j=0; j<ms->mBoneCount; j++)
			{
				MeshBone &b = ms->mBones[j];
				if ( b.mParentIndex == -1 )
				{
					b.mPosition = quat * b.mPosition;
					b.mOrientation = quat * b.mOrientation;
				}
			}
		}
	}

	{
		GearMeshVector::Iterator i;
		for (i=mMyMeshes.Begin(); i!=mMyMeshes.End(); ++i)
		{
			GearMesh *m = (*i);
			uint32 vcount = m->mVertexPool.GetSize();
			if ( vcount > 0 )
			{
				MeshVertex *vb = m->mVertexPool.GetBuffer();
				for (uint32 j=0; j<vcount; j++)
				{
					vb->mPos = quat * vb->mPos;
					vb++;
				}
			}
		}
	}

	{
		MeshAnimationVector::Iterator i;
		for (i=mMyAnimations.Begin(); i!=mMyAnimations.End(); ++i)
		{
			MeshAnimation *ma = (*i);
			for (int32 j=0; j<ma->mTrackCount && j <1; j++)
			{
				MeshAnimTrack *t = ma->mTracks[j];
				for (int32 k=0; k<t->mFrameCount; k++)
				{
					MeshAnimPose &p = t->mPose[k];
					p.mPos = quat * p.mPos;
					p.mQuat = quat * p.mQuat;
				}
			}
		}
	}
}

void GearsMeshBuilder::scale( scalar s )
{
	{
		MeshSkeletonVector::Iterator i;
		for (i=mMySkeletons.Begin(); i!=mMySkeletons.End(); ++i)
		{
			MeshSkeleton *ms = (*i);
			for (int32 j=0; j<ms->mBoneCount; j++)
			{
				MeshBone &b = ms->mBones[j];
				b.mPosition[0]*=s;
				b.mPosition[1]*=s;
				b.mPosition[2]*=s;
			}
		}
	}

	{
		GearMeshVector::Iterator i;
		for (i=mMyMeshes.Begin(); i!=mMyMeshes.End(); ++i)
		{
			GearMesh *m = (*i);
			uint32 vcount = m->mVertexPool.GetSize();
			if ( vcount > 0 )
			{
				MeshVertex *vb = m->mVertexPool.GetBuffer();
				for (uint32 j=0; j<vcount; j++)
				{
					vb->mPos[0]*=s;
					vb->mPos[1]*=s;
					vb->mPos[2]*=s;
					vb++;
				}
			}
		}
	}

	{
		MeshAnimationVector::Iterator i;
		for (i=mMyAnimations.Begin(); i!=mMyAnimations.End(); ++i)
		{
			MeshAnimation *ma = (*i);
			for (int32 j=0; j<ma->mTrackCount; j++)
			{
				MeshAnimTrack *t = ma->mTracks[j];
				for (int32 k=0; k<t->mFrameCount; k++)
				{
					MeshAnimPose &p = t->mPose[k];
					p.mPos[0]*=s;
					p.mPos[1]*=s;
					p.mPos[2]*=s;
				}
			}
		}
	}
}

const char* GearsMeshBuilder::getMaterialName( const char *matName )
{
	const char *ret = matName;
	if ( mINI )
	{
		uint32 keycount;
		uint32 lineno;
		const KeyValueSection *section = locateSection(mINI,"REMAP_MATERIALS",keycount,lineno);
		if ( section )
		{
			for (uint32 i=0; i<keycount; i++)
			{
				const char *key = getKey(section,i,lineno);
				const char *value = getValue(section,i,lineno);
				if ( key && value )
				{
					if ( stricmp(key,matName) == 0 )
					{
						ret = value;
						break;
					}
				}
			}
		}
	}
	return ret;
}

void GearsMeshBuilder::importRawTexture( const char *textureName,const uint8 *pixels,uint32 wid,uint32 hit )
{
	
}

_NAMESPACE_END

