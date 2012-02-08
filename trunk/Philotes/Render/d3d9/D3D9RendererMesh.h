
#ifndef D3D9_RENDERER_MESH_H
#define D3D9_RENDERER_MESH_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderMesh.h>
#include "D3D9Renderer.h"

_NAMESPACE_BEGIN

class D3D9RendererMesh : public RendererMesh
{
	public:
		D3D9RendererMesh(D3D9Renderer &renderer, const RendererMeshDesc &desc);
		virtual ~D3D9RendererMesh(void);
	
	public:
		virtual void renderIndices(uint32 numVertices, uint32 firstIndex, uint32 numIndices, RendererIndexBuffer::Format indexFormat) const;
		virtual void renderVertices(uint32 numVertices) const;
		
		virtual void renderIndicesInstanced(uint32 numVertices, uint32 firstIndex, uint32 numIndices, RendererIndexBuffer::Format indexFormat,RendererMaterial *material) const;
		virtual void renderVerticesInstanced(uint32 numVertices,RendererMaterial *material) const;
	
	private:
		D3D9RendererMesh &operator=(const D3D9RendererMesh &) { return *this; }
	
	private:
		D3D9Renderer                &m_renderer;
		IDirect3DVertexDeclaration9 *m_d3dVertexDecl;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
