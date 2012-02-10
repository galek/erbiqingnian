
#ifndef D3D9_RENDERER_MATERIAL_H
#define D3D9_RENDERER_MATERIAL_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderMaterial.h>
#include "D3D9Render.h"

_NAMESPACE_BEGIN

class D3D9Render;

class D3D9RenderMaterial : public RenderMaterial
{
	public:
		D3D9RenderMaterial(D3D9Render &renderer, const RenderMaterialDesc &desc);
		virtual ~D3D9RenderMaterial(void);
		virtual void setModelMatrix(const scalar *matrix);
		
	private:
		virtual void bind(RenderMaterial::Pass pass, RenderMaterialInstance *materialInstance, bool instanced) const;
		virtual void bindMeshState(bool instanced) const;
		virtual void unbind(void) const;
		virtual void bindVariable(Pass pass, const Variable &variable, const void *data) const;
		
		void loadCustomConstants(ID3DXConstantTable &table, Pass pass);
	
	private:
		class ShaderConstants
		{
			public:
				LPD3DXCONSTANTTABLE table;
				
				D3DXHANDLE          modelMatrix;
				D3DXHANDLE          viewMatrix;
				D3DXHANDLE          projMatrix;
				D3DXHANDLE          modelViewMatrix;
				D3DXHANDLE          modelViewProjMatrix;
				
				D3DXHANDLE          boneMatrices;
				
				D3DXHANDLE          eyePosition;
				D3DXHANDLE          eyeDirection;
				
				D3DXHANDLE          ambientColor;
				
				D3DXHANDLE          lightColor;
				D3DXHANDLE          lightIntensity;
				D3DXHANDLE          lightDirection;
				D3DXHANDLE          lightPosition;
				D3DXHANDLE          lightInnerRadius;
				D3DXHANDLE          lightOuterRadius;
				D3DXHANDLE          lightInnerCone;
				D3DXHANDLE          lightOuterCone;
				D3DXHANDLE          lightShadowMap;
				D3DXHANDLE          lightShadowMatrix;
				
				D3DXHANDLE          vfaceScale;
			
			public:
				ShaderConstants(void);
				~ShaderConstants(void);
				
				void loadConstants(void);
				
				void bindEnvironment(IDirect3DDevice9 &d3dDevice, const D3D9Render::ShaderEnvironment &shaderEnv) const;
		};
		
		class D3D9Variable : public Variable
		{
			friend class D3D9RenderMaterial;
			public:
				D3D9Variable(const char *name, VariableType type, uint32 offset);
				virtual ~D3D9Variable(void);
				
				void addVertexHandle(ID3DXConstantTable &table, D3DXHANDLE handle);
				void addFragmentHandle(ID3DXConstantTable &table, D3DXHANDLE handle, Pass pass);
			
			private:
				D3D9Variable &operator=(const D3D9Variable&) { return *this; }
				
			private:
				D3DXHANDLE          m_vertexHandle;
				UINT                m_vertexRegister;
				D3DXHANDLE          m_fragmentHandles[NUM_PASSES];
				UINT                m_fragmentRegisters[NUM_PASSES];
		};
	
	private:
		D3D9RenderMaterial &operator=(const D3D9RenderMaterial&) { return *this; }
		
	private:
		D3D9Render           &m_renderer;
		
		D3DCMPFUNC              m_d3dAlphaTestFunc;
		D3DBLEND                m_d3dSrcBlendFunc;
		D3DBLEND                m_d3dDstBlendFunc;
		
		IDirect3DVertexShader9 *m_vertexShader;
		IDirect3DVertexShader9 *m_instancedVertexShader;
		IDirect3DPixelShader9  *m_fragmentPrograms[NUM_PASSES];
		
		ShaderConstants         m_vertexConstants;
		ShaderConstants         m_instancedVertexConstants;
		ShaderConstants         m_fragmentConstants[NUM_PASSES];
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
