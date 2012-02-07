
#ifndef RENDERER_MESH_DESC_H
#define RENDERER_MESH_DESC_H

#include <RendererMesh.h>

class RendererVertexBuffer;
class RendererIndexBuffer;
class RendererInstanceBuffer;

class RendererMeshDesc
{
	public:
		RendererMesh::Primitive primitives;
		
		RendererVertexBuffer  **vertexBuffers;
		uint32                   numVertexBuffers;
		uint32                   firstVertex;
		uint32                   numVertices;
		
		RendererIndexBuffer    *indexBuffer;
		uint32                   firstIndex;
		uint32                   numIndices;
		
		RendererInstanceBuffer *instanceBuffer;
		uint32                   firstInstance;
		uint32                   numInstances;
		
	public:
		RendererMeshDesc(void);
		
		bool isValid(void) const;
};

#endif
