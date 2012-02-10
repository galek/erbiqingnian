
#ifndef RENDERER_MESH_SHAPE_H
#define RENDERER_MESH_SHAPE_H

#include <renderShape.h>

_NAMESPACE_BEGIN

class RenderVertexBuffer;
class RenderIndexBuffer;

class RenderMeshShape : public RenderShape
{
	public:
		RenderMeshShape(Render& renderer, 
							 const Vector3* verts, uint32 numVerts,
							 const Vector3* normals,
							 const scalar* uvs,
							 const uint16* faces, uint32 numFaces, bool flipWinding=false);
		virtual ~RenderMeshShape();
		
	private:
		RenderVertexBuffer*	m_vertexBuffer;
		RenderIndexBuffer*	m_indexBuffer;
};

_NAMESPACE_END

#endif
