
#include "D3D9Renderer.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderDesc.h>

#include <renderProjection.h>

#include <renderVertexBufferDesc.h>
#include "D3D9RendererVertexBuffer.h"

#include <renderIndexBufferDesc.h>
#include "D3D9RendererIndexBuffer.h"

#include <renderInstanceBufferDesc.h>
#include "D3D9RendererInstanceBuffer.h"

#include <renderMeshDesc.h>
#include <renderMeshContext.h>
#include "D3D9RendererMesh.h"

#include <renderMaterialDesc.h>
#include <renderMaterialInstance.h>
#include "D3D9RendererMaterial.h"

#include <renderLightDesc.h>
#include <renderDirectionalLightDesc.h>
#include "D3D9RendererDirectionalLight.h"
#include <renderSpotLightDesc.h>
#include "D3D9RendererSpotLight.h"

#include <renderTexture2DDesc.h>
#include "D3D9RendererTexture2D.h"

#include <renderTargetDesc.h>
#include "D3D9RendererTarget.h"

#include "gearsPlatform.h"

_NAMESPACE_BEGIN

void convertToD3D9(D3DCOLOR &dxcolor, const RendererColor &color)
{
	const float inv255 = 1.0f / 255.0f;
	dxcolor = D3DXCOLOR(color.r*inv255, color.g*inv255, color.b*inv255, color.a*inv255);
}

void convertToD3D9(float *dxvec, const Vector3 &vec)
{
	dxvec[0] = vec.x;
	dxvec[1] = vec.y;
	dxvec[2] = vec.z;
}

void convertToD3D9(D3DMATRIX &dxmat, const Matrix4 &mat)
{
 	dxmat = D3DXMATRIX(
 			mat[0][0], mat[1][0], mat[2][0], mat[3][0],
             mat[0][1], mat[1][1], mat[2][1], mat[3][1],
             mat[0][2], mat[1][2], mat[2][2], mat[3][2],
             mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
}

/******************************
* D3D9Renderer::D3DXInterface *
******************************/ 

D3D9Renderer::D3DXInterface::D3DXInterface(void)
{
	memset(this, 0, sizeof(*this));

#if defined(RENDERER_WINDOWS)

	#define D3DX_DLL "d3dx9_" RENDERER_TEXT2(D3DX_SDK_VERSION) ".dll"

	m_library = LoadLibraryA(D3DX_DLL);
	ph_assert2(m_library, "Unable to load " D3DX_DLL ".");
	if(!m_library)
	{
		MessageBoxA(0, "Unable to load " D3DX_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
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

D3D9Renderer::D3DXInterface::~D3DXInterface(void)
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

HRESULT D3D9Renderer::D3DXInterface::CompileShaderFromFileA(LPCSTR srcFile, CONST D3DXMACRO *defines, LPD3DXINCLUDE include,
															LPCSTR functionName, LPCSTR profile, DWORD flags, LPD3DXBUFFER *shader,
															LPD3DXBUFFER *errorMsgs, LPD3DXCONSTANTTABLE *constantTable)
{

	HRESULT result = D3DERR_NOTAVAILABLE;
	CALL_D3DX_FUNCTION(D3DXCompileShaderFromFileA, (srcFile, defines, include, functionName, profile, flags, shader, errorMsgs, constantTable));
	return result;

}

LPCSTR D3D9Renderer::D3DXInterface::GetVertexShaderProfile(LPDIRECT3DDEVICE9 device)
{

	LPCSTR result = 0;
	CALL_D3DX_FUNCTION(D3DXGetVertexShaderProfile, (device));
	return result;

}

LPCSTR D3D9Renderer::D3DXInterface::GetPixelShaderProfile(LPDIRECT3DDEVICE9 device)
{
	LPCSTR result = 0;
	CALL_D3DX_FUNCTION(D3DXGetPixelShaderProfile, (device));
	return result;
}

#undef CALL_D3DX_FUNCTION

/**********************************
* D3D9Renderer::ShaderEnvironment *
**********************************/ 

D3D9Renderer::ShaderEnvironment::ShaderEnvironment(void)
{
	memset(this, 0, sizeof(*this));
}

/***************
* D3D9Renderer *
***************/ 

D3D9Renderer::D3D9Renderer(const RendererDesc &desc) :
	Renderer	(DRIVER_DIRECT3D9)
{
	m_textVDecl				= NULL;
	m_pixelCenterOffset      = 0.5f;
	m_displayWidth           = 0;
	m_displayHeight          = 0;
	m_d3d                    = 0;
	m_d3dDevice              = 0;
	m_d3dDepthStencilSurface = 0;
	
	m_viewMatrix = Matrix4::IDENTITY;
	
	GearPlatform* m_platform = GearPlatform::getSingleton();
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

D3D9Renderer::~D3D9Renderer(void)
{
	assert(!m_textVDecl);
	GearPlatform* m_platform = GearPlatform::getSingleton();

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

bool D3D9Renderer::checkResize(bool isDeviceLost)
{
	bool isDeviceReset = false;
#if defined(RENDERER_WINDOWS)
	if(GearPlatform::getSingleton()->getWindowHandle() && m_d3dDevice)
	{
		uint32 width  = 0;
		uint32 height = 0;
		GearPlatform::getSingleton()->getWindowSize(width, height);
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

void D3D9Renderer::onDeviceLost(void)
{
	notifyResourcesLostDevice();
	if(m_d3dDepthStencilSurface)
	{
		m_d3dDepthStencilSurface->Release();
		m_d3dDepthStencilSurface = 0;
	}
}

void D3D9Renderer::onDeviceReset(void)
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

void D3D9Renderer::buildDepthStencilSurface(void)
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
void D3D9Renderer::clearBuffers(void)
{
	if(m_d3dDevice)
	{
		const DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
		m_d3dDevice->Clear(0, NULL, flags, D3DCOLOR_RGBA(getClearColor().r, getClearColor().g, getClearColor().b, getClearColor().a), 1.0f, 0);
	}
}

// presents the current color buffer to the screen.
// returns true on device reset and if buffers need to be rewritten.
bool D3D9Renderer::swapBuffers(void)
{
	bool isDeviceReset = false;
	if(m_d3dDevice)
	{
		HRESULT result = GearPlatform::getSingleton()->D3D9Present();
		ph_assert2(result == D3D_OK || result == D3DERR_DEVICELOST, "Unknown Direct3D9 error when swapping buffers.");
		if(result == D3D_OK || result == D3DERR_DEVICELOST)
		{
			isDeviceReset = checkResize(result == D3DERR_DEVICELOST);
		}
	}
	return isDeviceReset;
}

void D3D9Renderer::getWindowSize(uint32 &width, uint32 &height) const
{
	width = m_displayWidth;
	height = m_displayHeight;
}

RendererVertexBuffer *D3D9Renderer::createVertexBuffer(const RendererVertexBufferDesc &desc)
{
	RENDERER_PERFZONE(D3D9Renderer_createVertexBuffer);
	D3D9RendererVertexBuffer *vb = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Vertex Buffer Descriptor.");
		if(desc.isValid())
		{
			vb = new D3D9RendererVertexBuffer(*m_d3dDevice, desc, m_deferredVBUnlock);
		}
	}
	if(vb) addResource(*vb);
	return vb;
}

RendererIndexBuffer *D3D9Renderer::createIndexBuffer(const RendererIndexBufferDesc &desc)
{
	RENDERER_PERFZONE(D3D9Renderer_createIndexBuffer);
	D3D9RendererIndexBuffer *ib = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Index Buffer Descriptor.");
		if(desc.isValid())
		{
			ib = new D3D9RendererIndexBuffer(*m_d3dDevice, desc);
		}
	}
	if(ib) addResource(*ib);
	return ib;
}

RendererInstanceBuffer *D3D9Renderer::createInstanceBuffer(const RendererInstanceBufferDesc &desc)
{
	RENDERER_PERFZONE(D3D9Renderer_createInstanceBuffer);
	D3D9RendererInstanceBuffer *ib = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Instance Buffer Descriptor.");
		if(desc.isValid())
		{
			ib = new D3D9RendererInstanceBuffer(*m_d3dDevice, desc);
		}
	}
	if(ib) addResource(*ib);
	return ib;
}

RendererTexture2D *D3D9Renderer::createTexture2D(const RendererTexture2DDesc &desc)
{
	RENDERER_PERFZONE(D3D9Renderer_createTexture2D);
	D3D9RendererTexture2D *texture = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Texture 2D Descriptor.");
		if(desc.isValid())
		{
			texture = new D3D9RendererTexture2D(*m_d3dDevice, desc);
		}
	}
	if(texture) addResource(*texture);
	return texture;
}

RendererTarget *D3D9Renderer::createTarget(const RendererTargetDesc &desc)
{
	RENDERER_PERFZONE(D3D9Renderer_createTarget);
	RendererTarget *target = 0;
#if defined(RENDERER_ENABLE_DIRECT3D9_TARGET)
	D3D9RendererTarget *d3dTarget = 0;
	ph_assert2(desc.isValid(), "Invalid Target Descriptor.");
	if(desc.isValid())
	{
		d3dTarget = new D3D9RendererTarget(*m_d3dDevice, desc);
	}
	if(d3dTarget) addResource(*d3dTarget);
	target = d3dTarget;
#endif
	return target;
}

RendererMaterial *D3D9Renderer::createMaterial(const RendererMaterialDesc &desc)
{
	RENDERER_PERFZONE(D3D9Renderer_createMaterial);
	D3D9RendererMaterial *mat = 0;
	ph_assert2(desc.isValid(), "Invalid Material Descriptor.");
	if(desc.isValid())
	{
		mat = new D3D9RendererMaterial(*this, desc);
	}
	return mat;
}

RendererMesh *D3D9Renderer::createMesh(const RendererMeshDesc &desc)
{
	RENDERER_PERFZONE(D3D9Renderer_createMesh);
	D3D9RendererMesh *mesh = 0;
	ph_assert2(desc.isValid(), "Invalid Mesh Descriptor.");
	if(desc.isValid())
	{
		mesh = new D3D9RendererMesh(*this, desc);
	}
	return mesh;
}

RendererLight *D3D9Renderer::createLight(const RendererLightDesc &desc)
{
	RENDERER_PERFZONE(D3D9Renderer_createLight);
	RendererLight *light = 0;
	if(m_d3dDevice)
	{
		ph_assert2(desc.isValid(), "Invalid Light Descriptor.");
		if(desc.isValid())
		{
			switch(desc.type)
			{
				case RendererLight::TYPE_DIRECTIONAL:
					light = new D3D9RendererDirectionalLight(*this, *(RendererDirectionalLightDesc*)&desc);
					break;
				case RendererLight::TYPE_SPOT:
					light = new D3D9RendererSpotLight(*this, *(RendererSpotLightDesc*)&desc);
					break;
				default:
					ph_assert2(0, "Not implemented!");
			}
		}
	}
	return light;
}

bool D3D9Renderer::beginRender(void)
{
	bool ok = false;
	if(m_d3dDevice)
	{
		ok = m_d3dDevice->BeginScene() == D3D_OK;
	}
	return ok;
}

void D3D9Renderer::endRender(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->EndScene();
	}
}

void D3D9Renderer::bindViewProj(const Matrix4 &eye, const Matrix4 &proj)
{
	// TODO : 可能有问题
	m_viewMatrix = eye.transpose();
	convertToD3D9(m_environment.viewMatrix, m_viewMatrix);
	convertToD3D9(m_environment.projMatrix, proj);
	
	const Vector3 eyeDirection = -eye.getColumn(2);
	const Vector3 eyeT = eye.getTrans();
	memcpy(m_environment.eyePosition,  &eyeT.x,			sizeof(scalar)*3);
	memcpy(m_environment.eyeDirection, &eyeDirection.x, sizeof(scalar)*3);
}

void D3D9Renderer::bindAmbientState(const RendererColor &ambientColor)
{
	convertToD3D9(m_environment.ambientColor, ambientColor);
}

void D3D9Renderer::bindDeferredState(void)
{
	ph_assert2(0, "Not implemented!");
}

void D3D9Renderer::bindMeshContext(const RendererMeshContext &context)
{
	Matrix4 model;
	Matrix4 modelView;
	if(context.transform) 
	{
		model = *context.transform;
	}
	else
	{
		model = Matrix4::IDENTITY;
	}

	modelView = m_viewMatrix * model;
	
	convertToD3D9(m_environment.modelMatrix,     model);
	convertToD3D9(m_environment.modelViewMatrix, modelView);

	// it appears that D3D winding is backwards, so reverse them...
    DWORD cullMode = D3DCULL_CCW;
    switch(context.cullMode)
    {
    case RendererMeshContext::CLOCKWISE: 
       cullMode = D3DCULL_CCW;
        break;
    case RendererMeshContext::COUNTER_CLOCKWISE: 
        cullMode = D3DCULL_CW;
        break;
    case RendererMeshContext::NONE: 
        cullMode = D3DCULL_NONE;
        break;
    default:
        ph_assert2(0, "Invalid Cull Mode");
    }

    m_d3dDevice->SetRenderState(D3DRS_CULLMODE, cullMode);

    ph_assert2(context.numBones <= RENDERER_MAX_BONES, "Too many bones.");
	if(context.boneMatrices && context.numBones>0 && context.numBones <= RENDERER_MAX_BONES)
	{
		for(uint32 i=0; i<context.numBones; i++)
		{
			convertToD3D9(m_environment.boneMatrices[i], context.boneMatrices[i]);
		}
		m_environment.numBones = context.numBones;
	}
}

void D3D9Renderer::beginMultiPass(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  1);
		m_d3dDevice->SetRenderState(D3DRS_SRCBLEND,          D3DBLEND_ONE);
		m_d3dDevice->SetRenderState(D3DRS_DESTBLEND,         D3DBLEND_ONE);
		m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_EQUAL);
	}
}

void D3D9Renderer::endMultiPass(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  0);
		m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_LESS);
	}
}

void D3D9Renderer::renderDeferredLight(const RendererLight &light)
{
	ph_assert2(0, "Not implemented!");
}

bool D3D9Renderer::isOk(void) const
{
	bool ok = true;
	if(!m_d3d)            ok = false;
	if(!m_d3dDevice)      ok = false;
#if defined(RENDERER_WINDOWS)
	ok = GearPlatform::getSingleton()->isD3D9ok();
	if(!m_d3dx.m_library) ok = false;
#endif
	return ok;
}

void D3D9Renderer::addResource(D3D9RendererResource &resource)
{
	ph_assert2(resource.m_d3dRenderer==0, "Resource already in added to the Renderer!");
	if(resource.m_d3dRenderer==0)
	{
		resource.m_d3dRenderer = this;
		m_resources.push_back(&resource);
	}
}

void D3D9Renderer::removeResource(D3D9RendererResource &resource)
{
	ph_assert2(resource.m_d3dRenderer==this, "Resource not part of this Renderer!");
	if(resource.m_d3dRenderer==this)
	{
		resource.m_d3dRenderer = 0;
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

void D3D9Renderer::notifyResourcesLostDevice(void)
{
	const uint32 numResources  = (uint32)m_resources.size();
	for(uint32 i=0; i<numResources; i++)
	{
		m_resources[i]->onDeviceLost();
	}
}

void D3D9Renderer::notifyResourcesResetDevice(void)
{
	const uint32 numResources  = (uint32)m_resources.size();
	for(uint32 i=0; i<numResources; i++)
	{
		m_resources[i]->onDeviceReset();
	}
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
