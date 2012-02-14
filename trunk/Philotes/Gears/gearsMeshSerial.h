
#include "gearsMeshSerialInterface.h"
#include "gearsMesh.h"
#include "parser/keyValIni.h"

_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////

class GearsMeshBuilder : public MeshBuilder
{
public:
	GearsMeshBuilder(KeyValueIni *ini,const char *meshName,const void *data,uint32 dlen,
		MeshImporter *mi,const char *options,MeshImportApplicationResource *appResource);

	GearsMeshBuilder(MeshImportApplicationResource *appResource);

	virtual				~GearsMeshBuilder(void);

	void				gather(void);

	void				gatherMaterials(void);

	virtual void        importUserData(const char *userKey,const char *userValue);

	virtual void        importUserBinaryData(const char *name,uint32 len,const uint8 *data);

	virtual void        importTetraMesh(const char *tetraName,const char *meshName,uint32 tcount,const scalar *tetraData);

	virtual void		importMaterial(const char *matName,const char *metaData);

	virtual void        importAssetName(const char *assetName,const char *info);

	virtual void        importMesh(const char *meshName,const char *skeletonName);

	void				getCurrentMesh(const char *meshName);

	virtual void		importPlane(const scalar *p);

	virtual void        importTriangle(const char *_meshName,
								const char *_materialName,
								uint32 vertexFlags,
								const MeshVertex &v1,
								const MeshVertex &v2,
								const MeshVertex &v3);

	virtual void        importIndexedTriangleList(const char *_meshName,
								const char *_materialName,
								uint32 vertexFlags,
								uint32 vcount,
								const MeshVertex *vertices,
								uint32 tcount,
								const uint32 *indices);

	virtual void        importAnimation(const MeshAnimation &animation);

	virtual void        importSkeleton(const MeshSkeleton &skeleton);

	virtual void        importRawTexture(const char *textureName,const uint8 *pixels,uint32 wid,uint32 hit);

	virtual void        importMeshInstance(const char *meshName,const scalar pos[3],const scalar rotation[4],const scalar scale[3]);

	virtual void		importCollisionRepresentation(const String& name,const String& info);

	void				getCurrentRep(const String& name);

	virtual void		importSphere(const char *collision_rep,
								const char *boneName,
								const Matrix4 &transform,
								scalar radius);

	virtual void		importCapsule(const char *collision_rep,
								const char *boneName,    
								const Matrix4 &transform,
								scalar radius,
								scalar height);

	virtual void		importOBB(const char *collision_rep,		
								const char *boneName,        
								const Matrix4 &transform,
								const Vector3& sides);

	virtual void		importConvexHull(const char *collision_rep, 
								const char *boneName,        
								const Matrix4 &transform,
								uint32 vertex_count,
								const scalar *vertices,
								uint32 tri_count,
								const uint32 *indices);

	virtual void		rotate(scalar rotX,scalar rotY,scalar rotZ);

	virtual void		scale(scalar s);

	const char*			getMaterialName(const char *matName);

private:

	StringMap	                        mMaterialMap;
	MeshMaterialVector                  mMyMaterials;
	MeshInstanceVector					mMyMeshInstances;
	GearMesh*							mCurrentMesh;
	GearMeshVector						mMyMeshes;
	MeshAnimationVector                 mMyAnimations;
	MeshSkeletonVector                  mMySkeletons;
	GearMeshCollisionRepresentation*	mCurrentCollision;
	MeshCollisionRepresentationVector   mCollisionReps;
	MeshImportApplicationResource*		mAppResource;
	KeyValueIni*						mINI;
};

_NAMESPACE_END