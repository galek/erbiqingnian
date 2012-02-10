
#ifndef RENDERER_INDEXBUFFER_H
#define RENDERER_INDEXBUFFER_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class RenderIndexBufferDesc;

class RenderIndexBuffer
{
	friend class RenderMesh;
	public:
		typedef enum Format
		{
			FORMAT_UINT16 = 0,
			FORMAT_UINT32,
			
			NUM_FORMATS
		};
		
		typedef enum Hint
		{
			HINT_STATIC = 0,
			HINT_DYNAMIC,
		};
	
	public:
		static uint32 getFormatByteSize(Format format);
	
	protected:
		RenderIndexBuffer(const RenderIndexBufferDesc &desc);
		
		virtual ~RenderIndexBuffer(void);
		
	public:
		void release(void) { delete this; }
		
		Hint   getHint(void) const;
		Format getFormat(void) const;
		uint32  getMaxIndices(void) const;
		
	public:
		virtual void *lock(void) = 0;
		virtual void  unlock(void) = 0;

	private:
		virtual void bind(void) const = 0;
		virtual void unbind(void) const = 0;
		
		RenderIndexBuffer &operator=(const RenderIndexBuffer &) { return *this; }
		
	protected:
		const Hint   m_hint;
		const Format m_format;
		uint32        m_maxIndices;
};

_NAMESPACE_END

#endif
