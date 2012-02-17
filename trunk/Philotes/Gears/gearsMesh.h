
#pragma once

#include "gearsMeshSerialInterface.h"
#include "gearsVertexPool.h"

_NAMESPACE_BEGIN

class GearSubMesh : public SubMesh
{
public:
	GearSubMesh(const String& mat,uint32 vertexFlags);

	~GearSubMesh(void){}

	bool isSame(const String& mat,uint32 vertexFlags) const;

	void gather(void);

	void add(const MeshVertex &v1,const MeshVertex &v2,const MeshVertex &v3,
		VertexPool< MeshVertex > &vpool);

	MeshIndexVector          mIndices;
	VertexPool< MeshVertex > mVertexPool;
};

//////////////////////////////////////////////////////////////////////////

class GearMesh : public Mesh
{
public:
	GearMesh(const String& meshName,const String& skeletonName);

	virtual						~GearMesh(void);

	void						release(void);

	bool						isSame(const String& meshName) const;

	// 获取，如没有则新建
	void						getCurrent(const String& materialName,uint32 vertexFlags);

	virtual void				importTriangle(const String& materialName,
									uint32 vertexFlags,
									const MeshVertex &_v1,
									const MeshVertex &_v2,
									const MeshVertex &_v3);

	virtual void				importIndexedTriangleList(const String& materialName,
									uint32 vertexFlags,
									uint32 vcount,
									const MeshVertex *vertices,
									uint32 tcount,
									const uint32 *indices);

	void						gather(int32 bone_count);

	VertexPool< MeshVertex >	mVertexPool;

	GearSubMesh*				mCurrent;

	SubMeshVector				mSubMeshes;
};

//////////////////////////////////////////////////////////////////////////

class GearMeshCollisionRepresentation : public MeshCollisionRepresentation
{
public:
	
	GearMeshCollisionRepresentation(const String& name,const String& info);

	~GearMeshCollisionRepresentation(void);

	void gather(void);

	MeshCollisionVector mGeometries;
};

typedef Array<GearMesh*> GearMeshVector;


_NAMESPACE_END