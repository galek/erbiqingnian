
#ifndef RENDERER_INSTANCEBUFFER_H
#define RENDERER_INSTANCEBUFFER_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class RenderInstanceBufferDesc;

class RenderInstanceBuffer
{
	friend class RenderBase;
	public:
		typedef enum Semantic
		{
			SEMANTIC_POSITION = 0,
			SEMANTIC_NORMALX,
			SEMANTIC_NORMALY,
			SEMANTIC_NORMALZ,
			SEMANTIC_VELOCITY_LIFE,// life remain (0-1] is packed into w
			SEMANTIC_DENSITY,

			NUM_SEMANTICS
		};
		
		typedef enum Format
		{
			FORMAT_FLOAT1 = 0,
			FORMAT_FLOAT2,
			FORMAT_FLOAT3,
			FORMAT_FLOAT4,

			NUM_FORMATS,
		};
		
		typedef enum Hint
		{
			HINT_STATIC = 0,
			HINT_DYNAMIC,
		};
	
	public:
		static uint32 getFormatByteSize(Format format);
	
	protected:
		RenderInstanceBuffer(const RenderInstanceBufferDesc &desc);
		virtual ~RenderInstanceBuffer(void);
		
	public:
		void release(void) { delete this; }
		
		Hint   getHint(void) const;
		Format getFormatForSemantic(Semantic semantic) const;
		
		void *lockSemantic(Semantic semantic, uint32 &stride);
		void  unlockSemantic(Semantic semantic);
	
		virtual void *lock(void) = 0;
		virtual void  unlock(void) = 0;
		virtual uint32 getStride(void) const { return m_stride; };
	private:
		
		virtual void  bind(uint32 streamID, uint32 firstInstance) const = 0;
		virtual void  unbind(uint32 streamID) const = 0;
		
		RenderInstanceBuffer &operator=(const RenderInstanceBuffer &) { return *this; }
		
	protected:
		class SemanticDesc
		{
			public:
				Format format;
				uint32  offset;
				bool   locked;
			public:
				SemanticDesc(void)
				{
					format = NUM_FORMATS;
					offset = 0;
					locked = false;
				}
		};
	
	protected:
		const Hint   m_hint;
		uint32 m_maxInstances;
		uint32 m_stride;
		SemanticDesc m_semanticDescs[NUM_SEMANTICS];
	
	private:
		void        *m_lockedBuffer;
		uint32 m_numSemanticLocks;
};

_NAMESPACE_END

#endif
