
#ifndef D3D9_RENDERER_H
#define D3D9_RENDERER_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include "render.h"

#include <vector>

#if defined(RENDERER_DEBUG)
	//#define D3D_DEBUG_INFO 1
#endif
#include <d3d9.h>
#include <d3dx9.h>

// Enables/Disables use of non-managed resources for vertex/instance buffers.
// Disabled for now as it appears to actually slow us down significantly on particles.
#if defined(RENDERER_XBOX360)
#define RENDERER_ENABLE_DYNAMIC_VB_POOLS 0
#else
#define RENDERER_ENABLE_DYNAMIC_VB_POOLS 1
#endif

_NAMESPACE_BEGIN

void convertToD3D9(D3DCOLOR &dxcolor, const Colour &color);
void convertToD3D9(float *dxvec, const Vector3 &vec);
void convertToD3D9(D3DMATRIX &dxmat, const Matrix4 &mat);

class D3D9RenderResource;

class D3D9Render : public Render
{
	friend class D3D9RenderResource;
	public:
		class D3DXInterface
		{
			friend class D3D9Render;
			private:
				D3DXInterface(void);
				~D3DXInterface(void);
			
			public:
				HRESULT CompileShaderFromFileA(LPCSTR srcFile, CONST D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR functionName, LPCSTR profile, DWORD flags, LPD3DXBUFFER *shader, LPD3DXBUFFER *errorMsgs, LPD3DXCONSTANTTABLE *constantTable);
				LPCSTR  GetVertexShaderProfile(LPDIRECT3DDEVICE9 device);
				LPCSTR  GetPixelShaderProfile(LPDIRECT3DDEVICE9 device);
				
			public:
			#if defined(RENDERER_WINDOWS) 
				HMODULE  m_library;
				
				#define DEFINE_D3DX_FUNCTION(_name, _return, _params) \
				    typedef _return (WINAPI* LP##_name) _params;      \
				    LP##_name m_##_name;
				
				DEFINE_D3DX_FUNCTION(D3DXCompileShaderFromFileA, HRESULT, (LPCSTR, CONST D3DXMACRO*, LPD3DXINCLUDE, LPCSTR, LPCSTR, DWORD, LPD3DXBUFFER*, LPD3DXBUFFER*, LPD3DXCONSTANTTABLE *))
				DEFINE_D3DX_FUNCTION(D3DXGetVertexShaderProfile, LPCSTR, (LPDIRECT3DDEVICE9));
				DEFINE_D3DX_FUNCTION(D3DXGetPixelShaderProfile,  LPCSTR, (LPDIRECT3DDEVICE9));
				
				#undef DEFINE_D3DX_FUNCTION
			#endif
		};
		
		class ShaderEnvironment
		{
			public:
				D3DMATRIX          modelMatrix;
				D3DMATRIX          viewMatrix;
				D3DMATRIX          projMatrix;
				D3DMATRIX          modelViewMatrix;
				D3DMATRIX          modelViewProjMatrix;
				
				D3DXMATRIX         boneMatrices[RENDERER_MAX_BONES];
				uint32              numBones;
				
				float              eyePosition[3];
				float              eyeDirection[3];
				
				D3DCOLOR           ambientColor;
				
				D3DCOLOR           lightColor;
				float              lightIntensity;
				float              lightDirection[3];
				float              lightPosition[3];
				float              lightInnerRadius;
				float              lightOuterRadius;
				float              lightInnerCone;
				float              lightOuterCone;
				IDirect3DTexture9 *lightShadowMap;
				D3DXMATRIX         lightShadowMatrix;
				
			public:
				ShaderEnvironment(void);
		};
		
	public:
		D3D9Render(uint64 windowHandle);
		virtual ~D3D9Render(void);
		
		IDirect3DDevice9        *getD3DDevice(void)               { return m_d3dDevice; }
		D3DXInterface           &getD3DX(void)                    { return m_d3dx; }
		ShaderEnvironment       &getShaderEnvironment(void)       { return m_environment; }
		const ShaderEnvironment &getShaderEnvironment(void) const { return m_environment; }
		
	private:
		bool checkResize(bool isDeviceLost);
		void onDeviceLost(void);
		void onDeviceReset(void);
		void buildDepthStencilSurface(void);
	
	public:
		// clears the offscreen buffers.
		virtual void clearBuffers(void);
		
		// presents the current color buffer to the screen.
		// returns true on device reset and if buffers need to be rewritten.
		virtual bool swapBuffers(void);

		// get the device pointer (void * abstraction)
		virtual void *getDevice()                                 { return static_cast<void*>(getD3DDevice()); }

		// get the window size
		void getWindowSize(uint32 &width, uint32 &height) const;
		
		virtual RenderVertexBuffer   *createVertexBuffer(  const RenderVertexBufferDesc   &desc);
		virtual RenderIndexBuffer    *createIndexBuffer(   const RenderIndexBufferDesc    &desc);
		virtual RenderInstanceBuffer *createInstanceBuffer(const RenderInstanceBufferDesc &desc);
		virtual RenderTexture2D      *createTexture2D(     const RenderTexture2DDesc      &desc);
		virtual RenderTarget         *createTarget(        const Philo::RenderTargetDesc         &desc);
		virtual RenderMaterial       *createMaterial(      const RenderMaterialDesc       &desc);
		virtual RenderBase           *createMesh(          const RenderMeshDesc           &desc);
		virtual RenderLight          *createLight(         const RenderLightDesc          &desc);

		virtual void convertProjectionMatrix(const Matrix4& matrix,Matrix4& dest);

	private:
		virtual bool beginRender(void);
		virtual void endRender(void);
		virtual void bindViewProj(const Matrix4 &eye, const Matrix4 &proj);
		virtual void bindAmbientState(const Colour &ambientColor);
		virtual void bindDeferredState(void);
		virtual void bindMeshContext(const RenderElement &context);
		virtual void beginMultiPass(void);
		virtual void endMultiPass(void);
		virtual void renderDeferredLight(const RenderLight &light);
		
		virtual bool isOk(void) const;
		
	private:
		void addResource(D3D9RenderResource &resource);
		void removeResource(D3D9RenderResource &resource);
		void notifyResourcesLostDevice(void);
		void notifyResourcesResetDevice(void);
	
	private:
		uint32                          m_displayWidth;
		uint32                          m_displayHeight;
		
		IDirect3D9						*m_d3d;
		IDirect3DDevice9				*m_d3dDevice;
		
		IDirect3DSurface9				*m_d3dDepthStencilSurface;
		
		D3DXInterface					m_d3dx;
		
		ShaderEnvironment				m_environment;
		
		D3DPRESENT_PARAMETERS			m_d3dPresentParams;
		
		Matrix4							m_viewMatrix;
		
		// non-managed resources...
		std::vector<D3D9RenderResource*> m_resources;

		IDirect3DVertexDeclaration9*	m_textVDecl;
};

class D3D9RenderResource
{
	friend class D3D9Render;
	public:
		D3D9RenderResource(void)
		{
			m_d3dRender = 0;
		}
		
		virtual ~D3D9RenderResource(void)
		{
			if(m_d3dRender) m_d3dRender->removeResource(*this);
		}
		
	public:
		virtual void onDeviceLost(void)=0;
		virtual void onDeviceReset(void)=0;
	
	private:
		D3D9Render *m_d3dRender;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
