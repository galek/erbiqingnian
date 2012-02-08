
#ifndef RENDERER_MESH_SHAPE_H
#define RENDERER_MESH_SHAPE_H

#include <renderShape.h>

_NAMESPACE_BEGIN

class RendererVertexBuffer;
class RendererIndexBuffer;

class RendererMeshShape : public RendererShape
{
	public:
		RendererMeshShape(Renderer& renderer, 
							 const Vector3* verts, uint32 numVerts,
							 const Vector3* normals,
							 const scalar* uvs,
							 const uint16* faces, uint32 numFaces, bool flipWinding=false);
		virtual ~RendererMeshShape();
		
	private:
		RendererVertexBuffer*	m_vertexBuffer;
		RendererIndexBuffer*	m_indexBuffer;
};

_NAMESPACE_END

#endif
