
#ifndef RENDERER_MATERIAL_H
#define RENDERER_MATERIAL_H

#include <renderConfig.h>
#include <vector>

_NAMESPACE_BEGIN

class RenderMaterialDesc;

class RenderTexture2D;

//ps3 need this declare
class RenderMaterialInstance;

class RenderMaterial
{
	friend class Render;
	friend class RenderMaterialInstance;
	public:
		typedef enum Type
		{
			TYPE_UNLIT = 0,
			TYPE_LIT,		
			NUM_TYPES,
		};
		
		typedef enum AlphaTestFunc
		{
			ALPHA_TEST_ALWAYS = 0, // disabled alpha testing...
			ALPHA_TEST_EQUAL,
			ALPHA_TEST_NOT_EQUAL,
			ALPHA_TEST_LESS,
			ALPHA_TEST_LESS_EQUAL,
			ALPHA_TEST_GREATER,
			ALPHA_TEST_GREATER_EQUAL,
			
			NUM_ALPHA_TEST_FUNCS,
		};
		
		typedef enum BlendFunc
		{
			BLEND_ZERO = 0,
			BLEND_ONE,
			BLEND_SRC_COLOR,
			BLEND_ONE_MINUS_SRC_COLOR,
			BLEND_SRC_ALPHA,
			BLEND_ONE_MINUS_SRC_ALPHA,
			BLEND_DST_ALPHA,
			BLEND_ONE_MINUS_DST_ALPHA,
			BLEND_DST_COLOR,
			BLEND_ONE_MINUS_DST_COLOR,
			BLEND_SRC_ALPHA_SATURATE,
		};

		typedef enum CullMode
		{
			CLOCKWISE = 0,
			COUNTER_CLOCKWISE,
			NONE
		};
		
		typedef enum Pass
		{
			PASS_UNLIT = 0,
			
			PASS_AMBIENT_LIGHT,
			PASS_POINT_LIGHT,
			PASS_DIRECTIONAL_LIGHT,
			PASS_SPOT_LIGHT,
			
			PASS_NORMALS,
			PASS_DEPTH,

			// LRR: The deferred pass causes compiles with the ARB_draw_buffers profile option, creating 
			// multiple color draw buffers.  This doesn't work in OGL on ancient Intel parts.
			//PASS_DEFERRED,
			
			NUM_PASSES,
		};
		
		typedef enum VariableType
		{
			VARIABLE_FLOAT = 0,
			VARIABLE_FLOAT2,
			VARIABLE_FLOAT3,
			VARIABLE_FLOAT4,
			VARIABLE_FLOAT4x4,
			
			VARIABLE_SAMPLER2D,
			
			NUM_VARIABLE_TYPES
		};
		
		class Variable
		{
			friend class RenderMaterial;
			protected:
				Variable(const char *name, VariableType type, uint32 offset);
				virtual ~Variable(void);
			
			public:
				const char  *getName(void) const;
				VariableType getType(void) const;
				uint32        getDataOffset(void) const;
				uint32        getDataSize(void) const;
				
			private:
				char        *m_name;
				VariableType m_type;
				uint32        m_offset;
		};
	
	public:
		static const char *getPassName(Pass pass);

		virtual void setModelMatrix(const scalar *matrix) = 0;
		
	protected:
		RenderMaterial(const RenderMaterialDesc &desc);
		virtual ~RenderMaterial(void);
	
	public:
		void release(void) { delete this; }
		
		__forceinline	Type			getType(void)						const { return m_type; }
		__forceinline	AlphaTestFunc	getAlphaTestFunc(void)				const { return m_alphaTestFunc; }
		__forceinline	float			getAlphaTestRef(void)				const { return m_alphaTestRef; }
		__forceinline	bool			getBlending(void)					const { return m_blending; }
		__forceinline	BlendFunc		getSrcBlendFunc(void)				const { return m_srcBlendFunc; }
		__forceinline	BlendFunc		getDstBlendFunc(void)				const { return m_dstBlendFunc; }
		__forceinline	uint32			getMaterialInstanceDataSize(void)	const { return m_variableBufferSize; }

		__forceinline	CullMode		getCullMode() const { return m_cullMode; }
		__forceinline	void			setCullMode(CullMode val) { m_cullMode = val; }
		
	protected:
		virtual void bind(RenderMaterial::Pass pass, RenderMaterialInstance *materialInstance, bool instanced) const;
		virtual void bindMeshState(bool instanced) const = 0;
		virtual void unbind(void) const = 0;
		virtual void bindVariable(Pass pass, const Variable &variable, const void *data) const = 0;
		
		RenderMaterial &operator=(const RenderMaterial&) { return *this; }
		
	protected:
		const Type          m_type;
		
		const AlphaTestFunc m_alphaTestFunc;
		float               m_alphaTestRef;
		
		bool                m_blending;
		const BlendFunc     m_srcBlendFunc;
		const BlendFunc     m_dstBlendFunc;

		CullMode			m_cullMode;

		std::vector<Variable*> m_variables;
		uint32               m_variableBufferSize;
};

_NAMESPACE_END

#endif