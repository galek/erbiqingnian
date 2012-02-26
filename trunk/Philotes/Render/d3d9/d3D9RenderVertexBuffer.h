
#ifndef D3D9_RENDERER_VERTEXBUFFER_H
#define D3D9_RENDERER_VERTEXBUFFER_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderVertexBuffer.h>
#include "D3D9Render.h"

_NAMESPACE_BEGIN

class D3D9RenderVertexBuffer : public RenderVertexBuffer, public D3D9RenderResource
{
	public:
		D3D9RenderVertexBuffer(IDirect3DDevice9 &d3dDevice, const RenderVertexBufferDesc &desc, bool deferredUnlock);

		virtual ~D3D9RenderVertexBuffer(void);
		
		void			addVertexElements(uint32 streamIndex, std::vector<D3DVERTEXELEMENT9> &vertexElements) const;

		virtual bool	checkBufferWritten(void);
		
	protected:
		virtual void *	lockImpl(void);
		virtual void	unlockImpl(void);
		
		virtual void	bind(uint32 streamID, uint32 firstVertex);
		virtual void	unbind(uint32 streamID);
	
	private:
		virtual void	onDeviceLost(void);
		virtual void	onDeviceReset(void);

	private:
		IDirect3DDevice9             &m_d3dDevice;
		IDirect3DVertexBuffer9       *m_d3dVertexBuffer;
		
		DWORD                         m_usage;
		D3DPOOL                       m_pool;
		UINT                          m_bufferSize;

		bool                          m_bufferWritten;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
