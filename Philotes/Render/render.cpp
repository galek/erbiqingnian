
#include "render.h"
#include "renderCamera.h"
#include "renderBase.h"
#include "renderElement.h"
#include "renderMaterial.h"
#include "renderMaterialInstance.h"
#include "renderTarget.h"
#include "renderLight.h"

#include <algorithm>

#include "d3d9/D3D9Render.h"

_NAMESPACE_BEGIN

Render *Render::createRender(uint64 windowHandle)
{
	Render *renderer = new D3D9Render(windowHandle);

	if(renderer && !renderer->isOk())
	{
		renderer->release();
		renderer = 0;
	}

	ph_assert2(renderer, "Failed to create renderer!");
	return renderer;
}

const char *Render::getDriverTypeName(DriverType type)
{
	const char *name = 0;
	switch(type)
	{
		case DRIVER_OPENGL:     name = "OpenGL";     break;
		case DRIVER_DIRECT3D9:  name = "Direct3D9";  break;
		case DRIVER_DIRECT3D10: name = "Direct3D10"; break;
		case DRIVER_LIBGCM:     name = "LibGCM";     break;
	}
	ph_assert2(name, "Unable to find Name String for Render Driver Type.");
	return name;
}

		
Render::Render(DriverType driver) :
	m_driver						(driver),
	m_deferredVBUnlock				(true)
{
	m_pixelCenterOffset = 0;
	setAmbientColor(Colour(0.25f,0.25f,0.25f,1));
    setClearColor(Colour(0.52f,0.6f,0.7f,1));
	strncpy_s(m_deviceName, sizeof(m_deviceName), "UNKNOWN", sizeof(m_deviceName));
}

void Render::setVertexBufferDeferredUnlocking( bool enabled )
{
	m_deferredVBUnlock = enabled;
}

bool Render::getVertexBufferDeferredUnlocking() const
{
	return m_deferredVBUnlock;
}

Render::~Render(void)
{
}

void Render::release(void)
{
	delete this;
}

// get the driver type for this renderer.
Render::DriverType Render::getDriverType(void) const
{
	return m_driver;
}

// get the offset to the center of a pixel relative to the size of a pixel (so either 0 or 0.5).
scalar Render::getPixelCenterOffset(void) const
{
	return m_pixelCenterOffset;
}

// get the name of the hardware device.
const char *Render::getDeviceName(void) const
{
	return m_deviceName;
}

// adds a mesh to the render queue.
void Render::queueMeshForRender(RenderElement &mesh)
{
	if (mesh.preQueuedToRender())
	{
		switch (mesh.getMaterial()->getType())
		{
		case  RenderMaterial::TYPE_LIT:
			m_visibleLitMeshes.Append(&mesh);
			break;
		default: //case RenderMaterial::TYPE_UNLIT:
			m_visibleUnlitMeshes.Append(&mesh);
		}
	}
}

// adds a light to the render queue.
void Render::queueLightForRender(RenderLight &light)
{
	ph_assert2(!light.isLocked(), "Light is already locked to a Render.");
	if(!light.isLocked())
	{
		light.m_renderer = this;
		m_visibleLights.Append(&light);
	}
}

void Render::render(RenderCamera* camera, RenderTarget *target, bool depthOnly)
{
	RENDERER_PERFZONE(Render_render);
	const uint32 numLights = (uint32)m_visibleLights.Size();
	if(target)
	{
		target->bind();
	}

	// TODO: ²ÄÖÊ·Ö×é
	if(beginRender())
	{
		if(depthOnly)
		{
			RENDERER_PERFZONE(Render_render_depthOnly);
			bindAmbientState(Colour(0,0,0,1));
			bindViewProj(camera);
			renderMeshes(m_visibleLitMeshes,   RenderMaterial::PASS_DEPTH);
			renderMeshes(m_visibleUnlitMeshes, RenderMaterial::PASS_DEPTH);
		}
		else if(numLights > RENDERER_DEFERRED_THRESHOLD)
		{
			RENDERER_PERFZONE(Render_render_deferred);
			bindDeferredState();
			bindViewProj(camera);
			renderMeshes(m_visibleLitMeshes,   RenderMaterial::PASS_UNLIT);
			renderMeshes(m_visibleUnlitMeshes, RenderMaterial::PASS_UNLIT);
			renderDeferredLights();
		}
		else if(numLights > 0)
		{
			RENDERER_PERFZONE(Render_render_lit);
			bindAmbientState(m_ambientColor);
			bindViewProj(camera);
			RenderLight &light0 = *m_visibleLights[0];
			light0.bind();
			renderMeshes(m_visibleLitMeshes, light0.getPass());
			light0.m_renderer = 0;
			
			//bindAmbientState(Colour(0,0,0,1));
			beginMultiPass();
			for(uint32 i=1; i<numLights; i++)
			{
				RenderLight &light = *m_visibleLights[i];
				light.bind();
				renderMeshes(m_visibleLitMeshes, light.getPass());
				light.m_renderer = 0;
			}
			endMultiPass();
			renderMeshes(m_visibleUnlitMeshes, RenderMaterial::PASS_UNLIT);
		}
		else
		{
			RENDERER_PERFZONE(Render_render_unlit);
			bindAmbientState(Colour(0,0,0,1));
			bindViewProj(camera);
			renderMeshes(m_visibleLitMeshes,   RenderMaterial::PASS_UNLIT);
			renderMeshes(m_visibleUnlitMeshes, RenderMaterial::PASS_UNLIT);
		}
		endRender();
	}
	if(target) 
	{
		target->unbind();
	}
	m_visibleLitMeshes.Reset();
	m_visibleUnlitMeshes.Reset();
	m_visibleLights.Reset();
}

// sets the ambient lighting color.
void Render::setAmbientColor(const Colour &ambientColor)
{
	m_ambientColor   = ambientColor;
	m_ambientColor.a = 1;
}

void Render::setClearColor(const Colour &clearColor)
{
	m_clearColor   = clearColor;
}

void Render::renderMeshes(Array<RenderElement*> & meshes, RenderMaterial::Pass pass)
{
	RENDERER_PERFZONE(Render_renderMeshes);
	
	RenderMaterial         *lastMaterial         = 0;
	RenderMaterialInstance *lastMaterialInstance = 0;
	const RenderBase       *lastMesh             = 0;
	
	const uint32 numMeshes = (uint32)meshes.Size();
	for(uint32 i=0; i<numMeshes; i++)
	{
		RenderElement &context = *meshes[i];
		bindMeshContext(context);
		bool instanced = context.m_mesh->getInstanceBuffer()?true:false;
		
		if(context.m_materialInstance && context.m_materialInstance != lastMaterialInstance)
		{
			if(lastMaterial) lastMaterial->unbind();
			lastMaterialInstance =  context.m_materialInstance;
			lastMaterial         = &context.m_materialInstance->getMaterial();
			lastMaterial->bind(pass, lastMaterialInstance, instanced);
		}
		else if(context.getMaterial() != lastMaterial)
		{
			if(lastMaterial) lastMaterial->unbind();
			lastMaterialInstance = 0;
			lastMaterial         = context.getMaterial();
			lastMaterial->bind(pass, lastMaterialInstance, instanced);
		}
		
		if(lastMaterial) 
		{
			lastMaterial->bindMeshState(instanced);
		}
		if(context.m_mesh != lastMesh)
		{
			if(lastMesh) lastMesh->unbind();
			lastMesh = context.m_mesh;
			if(lastMesh) lastMesh->bind();
		}
		if(lastMesh)
		{
			context.m_mesh->render(context.getMaterial());
		}
	}
	if(lastMesh)     
	{
		lastMesh->unbind();
	}
	if(lastMaterial)
	{
		lastMaterial->unbind();
	}
}


void Render::renderDeferredLights(void)
{
	RENDERER_PERFZONE(Render_renderDeferredLights);
	
	const uint32 numLights = (uint32)m_visibleLights.Size();
	for(uint32 i=0; i<numLights; i++)
	{
		renderDeferredLight(*m_visibleLights[i]);
	}
}

_NAMESPACE_END