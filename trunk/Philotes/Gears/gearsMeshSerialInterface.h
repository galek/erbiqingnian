
#pragma once

#include "gearsMeshData.h"

_NAMESPACE_BEGIN

class MeshSystemContainer;

class MeshSerialize
{
public:
	MeshSerialize(MeshSerializeFormat format)
	{
		mFormat = format;
		mBaseData = 0;
		mBaseLen  = 0;
		mExtendedData = 0;
		mExtendedLen = 0;
		mSaveFileName = 0;
		mExportTransform.setIdentity();
	}

	MeshSerializeFormat mFormat;

	uint8			*mBaseData;

	uint32			mBaseLen;

	uint8			*mExtendedData;

	uint32			mExtendedLen;

	const char		*mSaveFileName;

	Matrix4			mExportTransform;
};

//////////////////////////////////////////////////////////////////////////

class MeshImportInterface
{
public:
	virtual void        		importMaterial(const char *matName,const char *metaData) = 0;

	virtual void        		importUserData(const char *userKey,const char *userValue) = 0;

	virtual void        		importUserBinaryData(const char *name,uint32 len,const uint8 *data) = 0;

	virtual void        		importTetraMesh(const char *tetraName,const char *meshName,uint32 tcount,const scalar *tetraData) = 0;

	virtual void        		importAssetName(const char *assetName,const char *info) = 0;

	virtual void        		importMesh(const char *meshName,const char *skeletonName) = 0;

	virtual void        		importTriangle(const char *meshName,
													const char *materialName,
													uint32 vertexFlags,
													const MeshVertex &v1,
													const MeshVertex &v2,
													const MeshVertex &v3) = 0;

	virtual void        		importIndexedTriangleList(const char *meshName,
													const char *materialName,
													uint32 vertexFlags,
													uint32 vcount,
													const MeshVertex *vertices,
													uint32 tcount,
													const uint32 *indices) = 0;

	virtual void        		importAnimation(const MeshAnimation &animation) = 0;

	virtual void        		importSkeleton(const MeshSkeleton &skeleton) = 0;

	virtual void        		importRawTexture(const char *textureName,const uint8 *pixels,uint32 wid,uint32 hit) = 0;

	virtual void        		importMeshInstance(const char *meshName,const Vector3& pos,const Quaternion& rotation,const Vector3& scale)= 0;

	virtual void				importCollisionRepresentation(const char *name,const char *info) = 0;

	virtual void				importConvexHull(const char *collision_rep,
													const char *boneName,
													const scalar *transform,
													uint32 vertex_count,
													const scalar *vertices,
													uint32 tri_count,
													const uint32 *indices) = 0;

	virtual void				importSphere(const char *collision_rep,
													const char *boneName,
													const scalar *transform,
													scalar radius) = 0;

	virtual void				importCapsule(const char *collision_rep,
													const char *boneName,
													const scalar *transform,
													scalar radius,
													scalar height) = 0;

	virtual void				importOBB(const char *collision_rep,
													const char *boneName,
													const scalar *transform,
													const scalar *sides) = 0;

	virtual int32				getSerializeFrame(void) = 0;

	virtual void				importPlane(const scalar *p) = 0;
};

//////////////////////////////////////////////////////////////////////////

class MeshImportApplicationResource
{
public:
	virtual void*				getApplicationResource(const char *base_name,const char *resource_name,uint32 &len) = 0;
	
	virtual void				releaseApplicationResource(void *mem) = 0;
};


//////////////////////////////////////////////////////////////////////////

class MeshImporter
{
public:
	virtual int32				getExtensionCount(void) { return 1; };

	virtual const char *		getExtension(int32 index=0) = 0;	

	virtual const char *		getDescription(int32 index=0) = 0;	

	virtual bool				importMesh(const char *meshName,const void *data,int32 dlen,MeshImportInterface *callback,
										const char *options,MeshImportApplicationResource *appResource) = 0;
};

//////////////////////////////////////////////////////////////////////////

class MeshImport
{
public:
	virtual void					addImporter(MeshImporter *importer) = 0; // add an additional importer

	virtual bool					importMesh(const char *meshName,const void *data,uint32 dlen,MeshImportInterface *callback,const char *options) = 0;

	virtual MeshSystemContainer*	createMeshSystemContainer(void) = 0;

	virtual MeshSystemContainer*	createMeshSystemContainer(const char *meshName,
										const void *data,
										uint32 dlen,
										const char *options) = 0;

	virtual void             		releaseMeshSystemContainer(MeshSystemContainer *mesh) = 0;

	virtual MeshSystem*     		getMeshSystem(MeshSystemContainer *mb) = 0;

	virtual bool             		serializeMeshSystem(MeshSystem *mesh,MeshSerialize &data) = 0;

	virtual void             		releaseSerializeMemory(MeshSerialize &data) = 0;

	virtual int32					getImporterCount(void) = 0;

	virtual MeshImporter*			getImporter(int32 index) = 0;

	virtual MeshImporter*   		locateMeshImporter(const char *fname) = 0;

	virtual const char*			    getFileRequestDialogString(void) = 0;

	virtual void					setMeshImportApplicationResource(MeshImportApplicationResource *resource) = 0;

	virtual MeshSkeletonInstance*	createMeshSkeletonInstance(const MeshSkeleton &sk) = 0;

	virtual bool					sampleAnimationTrack(int32 trackIndex,const MeshSystem *msystem,MeshSkeletonInstance *skeleton) = 0;

	virtual void					releaseMeshSkeletonInstance(MeshSkeletonInstance *sk) = 0;

	virtual void					transformVertices(uint32 vcount,
										const MeshVertex *source_vertices,
										MeshVertex *dest_vertices,
										MeshSkeletonInstance *skeleton) = 0;

	virtual MeshImportInterface*	getMeshImportInterface(MeshSystemContainer *msc) = 0;

	virtual void					gather(MeshSystemContainer *msc) = 0;

	virtual void					scale(MeshSystemContainer *msc,scalar scale) = 0;

	virtual void					rotate(MeshSystemContainer *msc,scalar rotX,scalar rotY,scalar rotZ) = 0;

	virtual VertexIndex*			createVertexIndex(scalar granularity) = 0;

	virtual void					releaseVertexIndex(VertexIndex *vindex) = 0;

protected:

	virtual ~MeshImport(void) { };

};

//////////////////////////////////////////////////////////////////////////

class MeshBuilder : public MeshSystem, public MeshImportInterface
{
public:
	virtual void gather	(void) = 0;

	virtual void scale	(scalar s) = 0;

	virtual void rotate	(scalar rotX,scalar rotY,scalar rotZ) = 0;
};

_NAMESPACE_END