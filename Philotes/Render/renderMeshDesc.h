
#ifndef RENDERER_MESH_DESC_H
#define RENDERER_MESH_DESC_H

#include <renderMesh.h>

_NAMESPACE_BEGIN

class RenderVertexBuffer;
class RenderIndexBuffer;
class RenderInstanceBuffer;

class RenderMeshDesc
{
	public:
		RenderMesh::Primitive primitives;
		
		RenderVertexBuffer  **vertexBuffers;
		uint32                   numVertexBuffers;
		uint32                   firstVertex;
		uint32                   numVertices;
		
		RenderIndexBuffer    *indexBuffer;
		uint32                   firstIndex;
		uint32                   numIndices;
		
		RenderInstanceBuffer *instanceBuffer;
		uint32                   firstInstance;
		uint32                   numInstances;
		
	public:
		RenderMeshDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
