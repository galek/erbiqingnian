
#ifndef RENDERER_VERTEXBUFFER_H
#define RENDERER_VERTEXBUFFER_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class RendererVertexBufferDesc;

class RendererVertexBuffer
{
	friend class RendererMesh;
	public:
		typedef enum Semantic
		{
			SEMANTIC_POSITION = 0,
			SEMANTIC_COLOR,
			SEMANTIC_NORMAL,
			SEMANTIC_TANGENT,
			
			SEMANTIC_BONEINDEX,
			SEMANTIC_BONEWEIGHT,
			
			SEMANTIC_TEXCOORD0,
			SEMANTIC_TEXCOORD1,
			SEMANTIC_TEXCOORD2,
			SEMANTIC_TEXCOORD3,
			SEMANTIC_TEXCOORDMAX = SEMANTIC_TEXCOORD3,
			
			NUM_SEMANTICS,
			NUM_TEXCOORD_SEMANTICS = SEMANTIC_TEXCOORDMAX - SEMANTIC_TEXCOORD0,
		};
		
		typedef enum Format
		{
			FORMAT_FLOAT1 = 0,
			FORMAT_FLOAT2,
			FORMAT_FLOAT3,
			FORMAT_FLOAT4,
			
			FORMAT_UBYTE4,
			FORMAT_USHORT4,
			
			FORMAT_COLOR, // RendererColor
			
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
		RendererVertexBuffer(const RendererVertexBufferDesc &desc);
		virtual ~RendererVertexBuffer(void);
	
	public:
		void release(void) { delete this; }
		
		uint32  getMaxVertices(void) const;
		
		Hint   getHint(void) const;
		Format getFormatForSemantic(Semantic semantic) const;

		void *lockSemantic(Semantic semantic, uint32 &stride);
		void  unlockSemantic(Semantic semantic);

		//Checks buffer written state for vertex buffers in D3D
		virtual bool checkBufferWritten() { return true; }

	private:
		virtual void  swizzleColor(void *colors, uint32 stride, uint32 numColors) {}
		
		virtual void *lock(void) = 0;
		virtual void  unlock(void) = 0;
		
		virtual void  bind(uint32 streamID, uint32 firstVertex) = 0;
		virtual void  unbind(uint32 streamID) = 0;
		
		RendererVertexBuffer &operator=(const RendererVertexBuffer &) { return *this; }
		
	protected:
		void prepareForRender(void);
	
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
		uint32 m_maxVertices;
		uint32 m_stride;
		bool         m_deferredUnlock;
		SemanticDesc m_semanticDescs[NUM_SEMANTICS];
		
	private:
		void        *m_lockedBuffer;
		uint32 m_numSemanticLocks;
};

_NAMESPACE_END

#endif
