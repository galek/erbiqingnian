
#ifndef RENDERER_MESH_H
#define RENDERER_MESH_H

#include <renderConfig.h>
#include <renderIndexBuffer.h>

_NAMESPACE_BEGIN

class RendererMeshDesc;
class RendererVertexBuffer;
class RendererIndexBuffer;
class RendererInstanceBuffer;
class RendererMaterial;

class RendererMesh
{
	friend class Renderer;
	public:
		typedef enum Primitive
		{
			PRIMITIVE_POINTS = 0,
			PRIMITIVE_LINES,
			PRIMITIVE_LINE_STRIP,
			PRIMITIVE_TRIANGLES,
			PRIMITIVE_TRIANGLE_STRIP,
			PRIMITIVE_POINT_SPRITES,
			
			NUM_PRIMITIVES
		};
	
	protected:
		RendererMesh(const RendererMeshDesc &desc);
		virtual ~RendererMesh(void);
		
	public:
		void release(void) { delete this; }
		
		Primitive getPrimitives(void) const;
		
		uint32 getNumVertices(void) const;
		uint32 getNumIndices(void) const;
		uint32 getNumInstances(void) const;
		
		void setVertexBufferRange(uint32 firstVertex, uint32 numVertices);
		void setIndexBufferRange(uint32 firstIndex, uint32 numIndices);
		void setInstanceBufferRange(uint32 firstInstance, uint32 numInstances);
		
		uint32                             getNumVertexBuffers(void) const;
		const RendererVertexBuffer *const*getVertexBuffers(void) const;
		
		const RendererIndexBuffer        *getIndexBuffer(void) const;
		
		const RendererInstanceBuffer     *getInstanceBuffer(void) const;
		
	private:
		void         bind(void) const;
		void         render(RendererMaterial *material) const;
		void         unbind(void) const;
		
		virtual void renderIndices(uint32 numVertices, uint32 firstIndex, uint32 numIndices, RendererIndexBuffer::Format indexFormat) const = 0;
		virtual void renderVertices(uint32 numVertices) const = 0;
		
		virtual void renderIndicesInstanced(uint32 numVertices, uint32 firstIndex, uint32 numIndices, RendererIndexBuffer::Format indexFormat,RendererMaterial *material) const = 0;
		virtual void renderVerticesInstanced(uint32 numVertices,RendererMaterial *material) const = 0;
	
	protected:
		Primitive               m_primitives;
		
		RendererVertexBuffer  **m_vertexBuffers;
		uint32                   m_numVertexBuffers;
		uint32                   m_firstVertex;
		uint32                   m_numVertices;
		
		RendererIndexBuffer    *m_indexBuffer;
		uint32                   m_firstIndex;
		uint32                   m_numIndices;
		
		RendererInstanceBuffer *m_instanceBuffer;
		uint32                   m_firstInstance;
		uint32                   m_numInstances;
};

_NAMESPACE_END

#endif
