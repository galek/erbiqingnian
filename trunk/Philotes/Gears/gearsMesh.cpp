
#include "gearsMesh.h"

_NAMESPACE_BEGIN

GearSubMesh::GearSubMesh( const String& mat,uint32 vertexFlags )
{
	mMaterialName = mat;
	mVertexFlags  = (MeshVertexFlag)vertexFlags;
}

bool GearSubMesh::isSame( const String& mat,uint32 vertexFlags ) const
{
	bool ret = false;
	if ( mat == mMaterialName && mVertexFlags == vertexFlags ) 
	{
		ret = true;
	}
	return ret;
}

void GearSubMesh::gather( void )
{
	mTriCount = (uint32)mMyIndices.Size()/3;
	mIndices  = &mMyIndices[0];
}

void GearSubMesh::add( const MeshVertex &v1,const MeshVertex &v2,const MeshVertex &v3,
					  VertexPool< MeshVertex > &vpool )
{
	mAABB.setVector(v1.mPos);
	mAABB.setVector(v2.mPos);
	mAABB.setVector(v3.mPos);

	uint32 i1 = vpool.GetVertex(v1);
	uint32 i2 = vpool.GetVertex(v2);
	uint32 i3 = vpool.GetVertex(v3);

	mMyIndices.Append(i1);
	mMyIndices.Append(i2);
	mMyIndices.Append(i3);
}

GearMesh::GearMesh( const String& meshName,const String& skeletonName )
{
	mName = meshName;
	mSkeletonName = skeletonName;
	mCurrent = 0;
}

GearMesh::~GearMesh( void )
{
	release();
}

void GearMesh::release( void )
{
	mSubMeshes = 0;
	mSubMeshCount = 0;
	SubMeshVector::Iterator i;
	for (i=mMySubMeshes.Begin(); i!=mMySubMeshes.End(); ++i)
	{
		GearSubMesh *s = static_cast<GearSubMesh *>((*i));
		delete s;
	}
	mMySubMeshes.Reset();
}

bool GearMesh::isSame( const String& meshName ) const
{
	return mName == meshName;
}

void GearMesh::getCurrent( const char *materialName,uint32 vertexFlags )
{
	if ( materialName == 0 ) materialName = "default_material";
	if ( mCurrent == 0 || !mCurrent->isSame(materialName,vertexFlags) )
	{
		mCurrent =0;
		SubMeshVector::Iterator i;
		for (i=mMySubMeshes.Begin(); i!=mMySubMeshes.End(); ++i)
		{
			GearSubMesh *s = static_cast< GearSubMesh *>((*i));
			if ( s->isSame(materialName,vertexFlags) )
			{
				mCurrent = s;
				break;
			}
		}
		if ( mCurrent == 0 )
		{
			mCurrent = ph_new(GearSubMesh)(materialName,vertexFlags);
			mMySubMeshes.Append(mCurrent);
		}
	}
}

void GearMesh::importTriangle( const char *materialName, uint32 vertexFlags,
							  const MeshVertex &_v1, const MeshVertex &_v2, const MeshVertex &_v3 )
{
	MeshVertex v1 = _v1;
	MeshVertex v2 = _v2;
	MeshVertex v3 = _v3;
	v1.mNormal.normalise();
	v1.mBiNormal.normalise();
	v1.mTangent.normalise();

	v2.mNormal.normalise();
	v2.mBiNormal.normalise();
	v2.mTangent.normalise();

	v3.mNormal.normalise();
	v3.mBiNormal.normalise();
	v3.mTangent.normalise();


	mAABB.setVector( v1.mPos );
	mAABB.setVector( v2.mPos );
	mAABB.setVector( v3.mPos );

	v1.validate();
	v2.validate();
	v3.validate();

	getCurrent(materialName,vertexFlags);
	mVertexFlags|=vertexFlags;

	mCurrent->add(v1,v2,v3,mVertexPool);
}

void GearMesh::importIndexedTriangleList( const char *materialName, uint32 vertexFlags, uint32 vcount,
										 const MeshVertex *vertices, uint32 tcount, const uint32 *indices )
{
	for (uint32 i=0; i<tcount; i++)
	{
		uint32 i1 = indices[i*3+0];
		uint32 i2 = indices[i*3+1];
		uint32 i3 = indices[i*3+2];
		const MeshVertex &v1 = vertices[i1];
		const MeshVertex &v2 = vertices[i2];
		const MeshVertex &v3 = vertices[i3];
		importTriangle(materialName,vertexFlags,v1,v2,v3);
	}
}

void GearMesh::gather( int32 bone_count )
{
	mSubMeshes = 0;
	mSubMeshCount = 0;
	if ( !mMySubMeshes.IsEmpty() )
	{
		mSubMeshCount = (uint32)mMySubMeshes.Size();
		mSubMeshes    = &mMySubMeshes[0];
		for (uint32 i=0; i<mSubMeshCount; i++)
		{
			GearSubMesh *m = static_cast<GearSubMesh *>(mSubMeshes[i]);
			m->gather();
		}
	}
	mVertexCount = mVertexPool.GetSize();
	if ( mVertexCount > 0 )
	{
		mVertices = mVertexPool.GetBuffer();
		if ( bone_count > 0 )
		{
			for (uint32 i=0; i<mVertexCount; i++)
			{
				MeshVertex &vtx = mVertices[i];
				if ( vtx.mBone[0] >= bone_count ) vtx.mBone[0] = 0;
				if ( vtx.mBone[1] >= bone_count ) vtx.mBone[1] = 0;
				if ( vtx.mBone[2] >= bone_count ) vtx.mBone[2] = 0;
				if ( vtx.mBone[3] >= bone_count ) vtx.mBone[3] = 0;
			}
		}
	}
}

GearMeshCollisionRepresentation::GearMeshCollisionRepresentation( const String& name,const String& info )
{
	mName = name;
	mInfo = info;
}

GearMeshCollisionRepresentation::~GearMeshCollisionRepresentation( void )
{
	MeshCollisionVector::Iterator i;
	for (i=mGeometries.Begin(); i!=mGeometries.End(); i++)
	{
		MeshCollision *mc = (*i);
		if ( mc->getType() == MCT_CONVEX )
		{
			MeshCollisionConvex *mcc = static_cast< MeshCollisionConvex *>(mc);
			delete []mcc->mVertices;
			delete []mcc->mIndices;
		}
	}
}

void GearMeshCollisionRepresentation::gather( void )
{
	mCollisionCount = (uint32)mGeometries.Size();
	mCollisionGeometry = &mGeometries[0];
}


_NAMESPACE_END



