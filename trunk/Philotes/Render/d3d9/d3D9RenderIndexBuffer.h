
#ifndef D3D9_RENDERER_INDEXBUFFER_H
#define D3D9_RENDERER_INDEXBUFFER_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderIndexBuffer.h>
#include "D3D9Render.h"

_NAMESPACE_BEGIN

class D3D9RenderIndexBuffer : public RenderIndexBuffer, public D3D9RenderResource
{
	public:
		D3D9RenderIndexBuffer(IDirect3DDevice9 &d3dDevice, const RenderIndexBufferDesc &desc);
		virtual ~D3D9RenderIndexBuffer(void);
		
	public:
		virtual void *lock(void);
		virtual void  unlock(void);

	private:
		virtual void bind(void) const;
		virtual void unbind(void) const;
	
		virtual void onDeviceLost(void);
		virtual void onDeviceReset(void);
	
	private:
		IDirect3DDevice9      &m_d3dDevice;
		IDirect3DIndexBuffer9 *m_d3dIndexBuffer;

		DWORD                         m_usage;
		D3DPOOL                       m_pool;
		UINT                          m_bufferSize;
		D3DFORMAT					  m_format;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
