
#pragma once

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include "renderBase.h"
#include "D3D9Render.h"

_NAMESPACE_BEGIN

class D3D9RenderBase : public RenderBase
{
	public:
		D3D9RenderBase(D3D9Render &renderer, const RenderMeshDesc &desc);

		virtual ~D3D9RenderBase(void);
	
	public:
		virtual void renderIndices(uint32 numVertices, uint32 firstIndex, uint32 numIndices, RenderIndexBuffer::Format indexFormat) const;

		virtual void renderVertices(uint32 numVertices) const;
		
		virtual void renderIndicesInstanced(uint32 numVertices, uint32 firstIndex, uint32 numIndices, RenderIndexBuffer::Format indexFormat,RenderMaterial *material) const;

		virtual void renderVerticesInstanced(uint32 numVertices,RenderMaterial *material) const;
	
	private:
		D3D9RenderBase &operator=(const D3D9RenderBase &) { return *this; }
	
	private:
		D3D9Render &m_renderer;

		IDirect3DVertexDeclaration9 *m_d3dVertexDecl;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
