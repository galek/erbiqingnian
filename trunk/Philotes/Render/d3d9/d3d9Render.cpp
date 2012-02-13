
#include "D3D9Render.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderVertexBufferDesc.h>
#include "D3D9RenderVertexBuffer.h"

#include <renderIndexBufferDesc.h>
#include "D3D9RenderIndexBuffer.h"

#include <renderInstanceBufferDesc.h>
#include "D3D9RenderInstanceBuffer.h"

#include <renderMeshDesc.h>
#include <renderElement.h>
#include "D3D9RenderMesh.h"

#include <renderMaterialDesc.h>
#include <renderMaterialInstance.h>
#include "D3D9RenderMaterial.h"

#include <renderLightDesc.h>
#include <renderDirectionalLightDesc.h>
#include "D3D9RenderDirectionalLight.h"
#include <renderSpotLightDesc.h>
#include "D3D9RenderSpotLight.h"

#include <renderTexture2DDesc.h>
#include "D3D9RenderTexture2D.h"

#include <renderTargetDesc.h>
#include "D3D9RenderTarget.h"

#include "gearsPlatformUtil.h"

_NAMESPACE_BEGIN

void convertToD3D9(float *dxvec, const Vector3 &vec)
{
	dxvec[0] = vec.x;
	dxvec[1] = vec.y;
	dxvec[2] = vec.z;
}

void convertToD3D9(D3DMATRIX &dxmat, const Matrix4 &mat)
{
	dxmat = D3DXMATRIX(
		mat[0][0], mat[0][1], mat[0][2], mat[0][3],
		mat[1][0], mat[1][1], mat[1][2], mat[1][3],
		mat[2][0], mat[2][1], mat[2][2], mat[2][3],
		mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
}

void convertToD3D9( D3DCOLOR &dxcolor, const Colour &color )
{
	dxcolor = D3DXCOLOR(color.r, color.g, color.b, color.a);
}

/******************************
* D3D9Render::D3DXInterface *
******************************/ 

D3D9Render::D3DXInterface::D3DXInterface(void)
{
	memset(this, 0, sizeof(*this));

#if defined(RENDERER_WINDOWS)

	#define D3DX_DLL "d3dx9_" RENDERER_TEXT2(D3DX_SDK_VERSION) ".dll"

	m_library = LoadLibraryA(D3DX_DLL);
	ph_assert2(m_library, "Unable to load " D3DX_DLL ".");
	if(!m_library)
	{
		MessageBoxA(0, "Unable to load " D3DX_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Render Error.", MB_OK);
	}
	if(m_library)
	{
		#define FIND_D3DX_FUNCTION(_name)                             \
		    m_##_name = (LP##_name)GetProcAddress(m_library, #_name); \
		    ph_assert2(m_##_name, "Unable to find D3DX9 Function " #_name " in " D3DX_DLL ".");
		
		FIND_D3DX_FUNCTION(D3DXCompileShaderFromFileA)
		FIND_D3DX_FUNCTION(D3DXGetVertexShaderProfile)
		FIND_D3DX_FUNCTION(D3DXGetPixelShaderProfile)
		
		#undef FIND_D3DX_FUNCTION
	}
	#undef D3DX_DLL
#endif
}

D3D9Render::D3DXInterface::~D3DXInterface(void)
{
#if defined(RENDERER_WINDOWS)
	if(m_library) FreeLibrary(m_library);
#endif
}

#if defined(RENDERER_WINDOWS)
	#define CALL_D3DX_FUNCTION(_name, _params)   if(m_##_name) result = m_##_name _params;
#else
	#define CALL_D3DX_FUNCTION(_name, _params)   result = _name _params;
#endif

HRESULT D3D9Render::D3DXInterface::CompileShaderFromFileA(LPCSTR srcFile, CONST D3DXMACRO *defines, LPD3DXINCLUDE include,
															LPCSTR functionName, LPCSTR profile, DWORD flags, LPD3DXBUFFER *shader,
															LPD3DXBUFFER *errorMsgs, LPD3DXCONSTANTTABLE *constantTable)
{

	HRESULT result = D3DERR_NOTAVAILABLE;
	CALL_D3DX_FUNCTION(D3DXCompileShaderFromFileA, (srcFile, defines, include, functionName, profile, flags, shader, errorMsgs, constantTable));
	return result;

}

LPCSTR D3D9Render::D3DXInterface::GetVertexShaderProfile(LPDIRECT3DDEVICE9 device)
{

	LPCSTR result = 0;
	CALL_D3DX_FUNCTION(D3DXGetVertexShaderProfile, (device));
	return result;

}

LPCSTR D3D9Render::D3DXInterface::GetPixelShaderProfile(LPDIRECT3DDEVICE9 device)
{
	LPCSTR result = 0;
	CALL_D3DX_FUNCTION(D3DXGetPixelShaderProfile, (device));
	return result;
}

#undef CALL_D3DX_FUNCTION

/**********************************
* D3D9Render::ShaderEnvironment *
**********************************/ 

D3D9Render::ShaderEnvironment::ShaderEnvironment(void)
{
	memset(this, 0, sizeof(*this));
}

/***************
* D3D9Render *
***************/ 

D3D9Render::D3D9Render(uint64 windowHandle) :
	Render	(DRIVER_DIRECT3D9)
{
	m_textVDecl				= NULL;
	m_pixelCenterOffset      = 0.5f;
	m_displayWidth           = 0;
	m_displayHeight          = 0;
	m_d3d                    = 0;
	m_d3dDevice              = 0;
	m_d3dDepthStencilSurface = 0;
	
	m_viewMatrix = Matrix4::IDENTITY;
	
	GearPlatformUtil* m_platform = GearPlatformUtil::getSingleton();
	m_d3d = static_cast<IDirect3D9*>(m_platform->initializeD3D9());
	ph_assert2(m_d3d, "Could not create Direct3D9 Interface.");
	if(m_d3d)
	{
		memset(&m_d3dPresentParams, 0, sizeof(m_d3dPresentParams));
		m_d3dPresentParams.Windowed               = 1;
		m_d3dPresentParams.SwapEffect             = D3DSWAPEFFECT_DISCARD;
		m_d3dPresentParams.BackBufferFormat       = D3DFMT_X8R8G8B8;
		m_d3dPresentParams.EnableAutoDepthStencil = 0;
		m_d3dPresentParams.AutoDepthStencilFormat = D3DFMT_D24S8;
		m_d3dPresentParams.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE; // turns off V-sync;
		m_d3dPresentParams.hDeviceWindow		  = (HWND)windowHandle;

		HRESULT res = m_platform->initializeD3D9Display(&m_d3dPresentParams, 
														m_deviceName, 
														m_displayWidth, 
														m_displayHeight, 
														&m_d3dDevice);
		ph_assert2(res==D3D_OK, "Failed to create Direct3D9 Device.");
		if(res==D3D_OK)
		{
			checkResize(false);
			onDeviceReset();
		}
	}
}

D3D9Render::~D3D9Render(void)
{
	assert(!m_textVDecl);
	GearPlatformUtil* m_platform = GearPlatformUtil::getSingleton();

	if(m_d3dDepthStencilSurface)
	{
		m_d3dDevice->SetDepthStencilSurface(NULL);
		m_platform->D3D9BlockUntilNotBusy(m_d3dDepthStencilSurface);
		m_d3dDepthStencilSurface->Release();
	}
	m_platform->D3D9DeviceBlockUntilIdle();
	if(m_d3dDevice)              m_d3dDevice->Release();
	if(m_d3d)                    m_d3d->Release();
}

bool D3D9Render::checkResize(bool isDeviceLost)
{
	bool isDeviceReset = false;
#if defined(RENDERER_WINDOWS)
	if(GearPlatformUtil::getSingleton()->getWindowHandle() && m_d3dDevice)
	{
		uint32 width  = 0;
		uint32 height = 0;
		GearPlatformUtil::getSingleton()->getWindowSize(width, height);
		if(width && height && (width != m_displayWidth || height != m_displayHeight) || isDeviceLost)
		{
			bool needsReset = (m_displayWidth&&m_displayHeight ? true : false);
			m_displayWidth  = width;
			m_displayHeight = height;

			D3DVIEWPORT9 viewport = {0};
			m_d3dDevice->GetViewport(&viewport);
			viewport.Width  = (DWORD)m_displayWidth;
			viewport.Height = (DWORD)m_displayHeight;
			viewport.MinZ =  0.0f;
			viewport.MaxZ =  1.0f;

			if(needsReset)
			{
				uint64 res = m_d3dDevice->TestCooperativeLevel();
				if(res == D3D_OK || res == D3DERR_DEVICENOTRESET)	//if device is lost, device has to be ready for reset
				{
					onDeviceLost();
					m_d3dPresentParams.BackBufferWidth  = (UINT)m_displayWidth;
					m_d3dPresentParams.BackBufferHeight = (UINT)m_displayHeight;
					res = m_d3dDevice->Reset(&m_d3dPresentParams);
					ph_assert2(res == D3D_OK, "Failed to reset Direct3D9 Device.");
					if(res == D3D_OK)
					{
						isDeviceReset = true;
					}
					m_d3dDevice->SetViewport(&viewport);
					onDeviceReset();
				}
			}
			else
			{
				m_d3dDevice->SetViewport(&viewport);
			}
		}
	}
#endif
	return isDeviceReset;
}

void D3D9Render::onDeviceLost(void)
{
	notifyResourcesLostDevice();
	if(m_d3dDepthStencilSurface)
	{
		m_d3dDepthStencilSurface->Release();
		m_d3dDepthStencilSurface = 0;
	}
}

void D3D9Render::onDeviceReset(void)
{
	if(m_d3dDevice)
	{
		// set out initial states...
		m_d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	#if defined(RENDERER_WINDOWS)
		m_d3dDevice->SetRenderState(D3DRS_LIGHTING, 0);
	#endif
		m_d3dDevice->SetRenderState(D3DRS_ZENABLE,  1);
		buildDepthStencilSurface();
	}
	notifyResourcesResetDevice();
}

void D3D9Render::buildDepthStencilSurface(void)
{
	if(m_d3dDevice)
	{
		uint32 width  = m_displayWidth;
		uint32 height = m_displayHeight;
		if(m_d3dDepthStencilSurface)
		{
			D3DSURFACE_DESC dssdesc;
			m_d3dDepthStencilSurface->GetDesc(&dssdesc);
			if(width != (uint32)dssdesc.Width || height != (uint32)dssdesc.Height)
			{
				m_d3dDepthStencilSurface->Release();
				m_d3dDepthStencilSurface = 0;
			}
		}
		if(!m_d3dDepthStencilSurface)
		{
			const D3DFORMAT           depthFormat        = D3DFMT_D24S8;
			const D3DMULTISAMPLE_TYPE multisampleType    = D3DMULTISAMPLE_NONE;
			const DWORD               multisampleQuality = 0;
			const BOOL                discard            = 0;
			IDirect3DSurface9 *depthSurface = 0;
			HRESULT result = m_d3dDevice->CreateDepthStencilSurface((UINT)width, (UINT)height, depthFormat, multisampleType, multisampleQuality, discard, &depthSurface, 0);
			ph_assert2(result == D3D_OK, "Failed to create Direct3D9 DepthStencil Surface.");
			if(result == D3D_OK)
			{
				m_d3dDepthStencilSurface = depthSurface;
			}
		}
		m_d3dDevice->SetDepthStencilSurface(m_d3dDepthStencilSurface);
	}
}

// clears the offscreen buffers.
void D3D9Render::clearBuffers(void)
{
	if(m_d3dDevice)
	{
		const DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
		m_d3dDevice->Clear(0, NULL, flags, getClearColor().getAsARGB(), 1.0f, 0);
	}
}

// presents the current color buffer to the screen.
// returns true on device reset and if buffers need to be rewritten.
bool D3D9Render::swapBuffers(void)
{
	bool isDeviceReset = false;
	if(m_d3dDevice)
	{
		HRESULT result = GearPlatformUtil::getSingleton()->D3D9Present();
		ph_assert2(result == D3D_OK || result == D3DERR_DEVICELOST, "Unknown Direct3D9 error when swapping buffers.");
		if(result == D3D_OK || result == D3DERR_DEVICELOST)
		{
			isDeviceReset = checkResize(result == D3DERR_DEVICELOST);
		}
	}
	return isDeviceReset;
}

void D3D9Render::getWindowSize(uint32 &width, uint32 &height) const
{
	width = m_displayWidth;
	height = m_displayHeight;
}

RenderVertexBuffer *D3D9Render::createVertexBuffer(const RenderVertexBufferDesc &desc)
{
	RENDERER_PERFZONE(D3D9Render_createVertexBuffer);
	D3D9RenderVertexBuffer *vb = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Vertex Buffer Descriptor.");
		if(desc.isValid())
		{
			vb = new D3D9RenderVertexBuffer(*m_d3dDevice, desc, m_deferredVBUnlock);
		}
	}
	if(vb) addResource(*vb);
	return vb;
}

RenderIndexBuffer *D3D9Render::createIndexBuffer(const RenderIndexBufferDesc &desc)
{
	RENDERER_PERFZONE(D3D9Render_createIndexBuffer);
	D3D9RenderIndexBuffer *ib = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Index Buffer Descriptor.");
		if(desc.isValid())
		{
			ib = new D3D9RenderIndexBuffer(*m_d3dDevice, desc);
		}
	}
	if(ib) addResource(*ib);
	return ib;
}

RenderInstanceBuffer *D3D9Render::createInstanceBuffer(const RenderInstanceBufferDesc &desc)
{
	RENDERER_PERFZONE(D3D9Render_createInstanceBuffer);
	D3D9RenderInstanceBuffer *ib = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Instance Buffer Descriptor.");
		if(desc.isValid())
		{
			ib = new D3D9RenderInstanceBuffer(*m_d3dDevice, desc);
		}
	}
	if(ib) addResource(*ib);
	return ib;
}

RenderTexture2D *D3D9Render::createTexture2D(const RenderTexture2DDesc &desc)
{
	RENDERER_PERFZONE(D3D9Render_createTexture2D);
	D3D9RenderTexture2D *texture = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Texture 2D Descriptor.");
		if(desc.isValid())
		{
			texture = new D3D9RenderTexture2D(*m_d3dDevice, desc);
		}
	}
	if(texture) addResource(*texture);
	return texture;
}

RenderTarget *D3D9Render::createTarget(const RenderTargetDesc &desc)
{
	RENDERER_PERFZONE(D3D9Render_createTarget);
	RenderTarget *target = 0;
#if defined(RENDERER_ENABLE_DIRECT3D9_TARGET)
	D3D9RenderTarget *d3dTarget = 0;
	ph_assert2(desc.isValid(), "Invalid Target Descriptor.");
	if(desc.isValid())
	{
		d3dTarget = new D3D9RenderTarget(*m_d3dDevice, desc);
	}
	if(d3dTarget) addResource(*d3dTarget);
	target = d3dTarget;
#endif
	return target;
}

RenderMaterial *D3D9Render::createMaterial(const RenderMaterialDesc &desc)
{
	RENDERER_PERFZONE(D3D9Render_createMaterial);
	D3D9RenderMaterial *mat = 0;
	ph_assert2(desc.isValid(), "Invalid Material Descriptor.");
	if(desc.isValid())
	{
		mat = new D3D9RenderMaterial(*this, desc);
	}
	return mat;
}

RenderMesh *D3D9Render::createMesh(const RenderMeshDesc &desc)
{
	RENDERER_PERFZONE(D3D9Render_createMesh);
	D3D9RenderMesh *mesh = 0;
	ph_assert2(desc.isValid(), "Invalid Mesh Descriptor.");
	if(desc.isValid())
	{
		mesh = new D3D9RenderMesh(*this, desc);
	}
	return mesh;
}

RenderLight *D3D9Render::createLight(const RenderLightDesc &desc)
{
	RENDERER_PERFZONE(D3D9Render_createLight);
	RenderLight *light = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Light Descriptor.");
		if(desc.isValid())
		{
			switch(desc.type)
			{
				case RenderLight::TYPE_DIRECTIONAL:
					light = new D3D9RenderDirectionalLight(*this, *(RenderDirectionalLightDesc*)&desc);
					break;
				case RenderLight::TYPE_SPOT:
					light = new D3D9RenderSpotLight(*this, *(RenderSpotLightDesc*)&desc);
					break;
				default:
					ph_assert2(0, "Not implemented!");
			}
		}
	}
	return light;
}

bool D3D9Render::beginRender(void)
{
	bool ok = false;
	if(m_d3dDevice)
	{
		ok = m_d3dDevice->BeginScene() == D3D_OK;
	}
	return ok;
}

void D3D9Render::endRender(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->EndScene();
	}
}

void D3D9Render::bindViewProj(const Matrix4 &eye, const Matrix4 &proj)
{
	m_viewMatrix = eye;
	convertToD3D9(m_environment.viewMatrix, m_viewMatrix);
	convertToD3D9(m_environment.projMatrix, proj);
	
	const Vector3 eyeDirection = -eye.getColumn(2);
	const Vector3 eyeT = eye.getTrans();
	memcpy(m_environment.eyePosition,  &eyeT.x,			sizeof(scalar)*3);
	memcpy(m_environment.eyeDirection, &eyeDirection.x, sizeof(scalar)*3);
}

void D3D9Render::bindAmbientState(const Colour &ambientColor)
{
	convertToD3D9(m_environment.ambientColor, ambientColor);
}

void D3D9Render::bindDeferredState(void)
{
	ph_assert2(0, "Not implemented!");
}

void D3D9Render::bindMeshContext(const RenderElement &context)
{
	Matrix4 model = context.getTransform();
	Matrix4 modelView;

	modelView = m_viewMatrix * model;
	
	convertToD3D9(m_environment.modelMatrix,     model);
	convertToD3D9(m_environment.modelViewMatrix, modelView);

	// it appears that D3D winding is backwards, so reverse them...

	// ¹Ç÷À¾ØÕó ×öÓ²¼þÃÉÆ¤
//     ph_assert2(context.numBones <= RENDERER_MAX_BONES, "Too many bones.");
// 	if(context.boneMatrices && context.numBones>0 && context.numBones <= RENDERER_MAX_BONES)
// 	{
// 		for(uint32 i=0; i<context.numBones; i++)
// 		{
// 			convertToD3D9(m_environment.boneMatrices[i], context.boneMatrices[i]);
// 		}
// 		m_environment.numBones = context.numBones;
// 	}
}

void D3D9Render::beginMultiPass(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  1);
		m_d3dDevice->SetRenderState(D3DRS_SRCBLEND,          D3DBLEND_ONE);
		m_d3dDevice->SetRenderState(D3DRS_DESTBLEND,         D3DBLEND_ONE);
		m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_EQUAL);
	}
}

void D3D9Render::endMultiPass(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  0);
		m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_LESS);
	}
}

void D3D9Render::renderDeferredLight(const RenderLight &light)
{
	ph_assert2(0, "Not implemented!");
}

bool D3D9Render::isOk(void) const
{
	bool ok = true;
	if(!m_d3d)            ok = false;
	if(!m_d3dDevice)      ok = false;
#if defined(RENDERER_WINDOWS)
	ok = GearPlatformUtil::getSingleton()->isD3D9ok();
	if(!m_d3dx.m_library) ok = false;
#endif
	return ok;
}

void D3D9Render::addResource(D3D9RenderResource &resource)
{
	ph_assert2(resource.m_d3dRender==0, "Resource already in added to the Render!");
	if(resource.m_d3dRender==0)
	{
		resource.m_d3dRender = this;
		m_resources.push_back(&resource);
	}
}

void D3D9Render::removeResource(D3D9RenderResource &resource)
{
	ph_assert2(resource.m_d3dRender==this, "Resource not part of this Render!");
	if(resource.m_d3dRender==this)
	{
		resource.m_d3dRender = 0;
		const uint32 numResources  = (uint32)m_resources.size();
		uint32       foundResource = numResources;
		for(uint32 i=0; i<numResources; i++)
		{
			if(m_resources[i] == &resource)
			{
				foundResource = i;
				break;
			}
		}
		if(foundResource < numResources)
		{
			m_resources[foundResource] = m_resources.back();
			m_resources.pop_back();
		}
	}
}

void D3D9Render::notifyResourcesLostDevice(void)
{
	const uint32 numResources  = (uint32)m_resources.size();
	for(uint32 i=0; i<numResources; i++)
	{
		m_resources[i]->onDeviceLost();
	}
}

void D3D9Render::notifyResourcesResetDevice(void)
{
	const uint32 numResources  = (uint32)m_resources.size();
	for(uint32 i=0; i<numResources; i++)
	{
		m_resources[i]->onDeviceReset();
	}
}

void D3D9Render::convertProjectionMatrix( const Matrix4& matrix,Matrix4& dest )
{
	dest = matrix;

	// Convert depth range from [-1,+1] to [0,1]
	dest[2][0] = (dest[2][0] + dest[3][0]) / 2;
	dest[2][1] = (dest[2][1] + dest[3][1]) / 2;
	dest[2][2] = (dest[2][2] + dest[3][2]) / 2;
	dest[2][3] = (dest[2][3] + dest[3][3]) / 2;
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
