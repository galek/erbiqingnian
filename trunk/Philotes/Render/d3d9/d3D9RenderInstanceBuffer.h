
#pragma once

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderInstanceBuffer.h>
#include "D3D9Render.h"

_NAMESPACE_BEGIN

class D3D9RenderInstanceBuffer : public RenderInstanceBuffer, public D3D9RenderResource
{
	public:
		D3D9RenderInstanceBuffer(IDirect3DDevice9 &d3dDevice, const RenderInstanceBufferDesc &desc);
		virtual ~D3D9RenderInstanceBuffer(void);
		
		void addVertexElements(uint32 streamIndex, std::vector<D3DVERTEXELEMENT9> &vertexElements) const;
		
	protected:
		virtual void *lock(void);
		virtual void  unlock(void);
		
		virtual void  bind(uint32 streamID, uint32 firstInstance) const;
		virtual void  unbind(uint32 streamID) const;
	
	private:
		virtual void onDeviceLost(void);
		virtual void onDeviceReset(void);
	
	private:
#if RENDERER_INSTANCING
		IDirect3DDevice9       &m_d3dDevice;
		IDirect3DVertexBuffer9 *m_d3dVertexBuffer;
#else
		void					*mInstanceBuffer;
#endif
		DWORD                   m_usage;
		D3DPOOL                 m_pool;
		UINT                    m_bufferSize;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
