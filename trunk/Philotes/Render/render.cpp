
#include <render.h>
#include <renderDesc.h>
#include <renderMesh.h>
#include <renderMeshContext.h>
#include <renderMaterial.h>
#include <renderMaterialInstance.h>
#include <renderProjection.h>
#include <renderTarget.h>
#include <renderLight.h>
#include <algorithm>

#include "d3d9/D3D9Renderer.h"
#include "renderDesc.h"

_NAMESPACE_BEGIN

Renderer *Renderer::createRenderer(const RendererDesc &desc)
{
	Renderer *renderer = 0;
	const bool valid = desc.isValid();
	if(valid)
	{
		switch(desc.driver)
		{
			case DRIVER_DIRECT3D9:
			#if defined(RENDERER_ENABLE_DIRECT3D9)
				renderer = new D3D9Renderer(desc);
			#endif
				break;
		}
	}
	if(renderer && !renderer->isOk())
	{
		renderer->release();
		renderer = 0;
	}
	ph_assert2(renderer, "Failed to create renderer!");
	return renderer;
}

const char *Renderer::getDriverTypeName(DriverType type)
{
	const char *name = 0;
	switch(type)
	{
		case DRIVER_OPENGL:     name = "OpenGL";     break;
		case DRIVER_DIRECT3D9:  name = "Direct3D9";  break;
		case DRIVER_DIRECT3D10: name = "Direct3D10"; break;
		case DRIVER_LIBGCM:     name = "LibGCM";     break;
	}
	ph_assert2(name, "Unable to find Name String for Renderer Driver Type.");
	return name;
}

		
Renderer::Renderer(DriverType driver) :
	m_driver						(driver),
	m_deferredVBUnlock				(true)
{
	m_pixelCenterOffset = 0;
	setAmbientColor(RendererColor(64,64,64, 255));
    setClearColor(RendererColor(133,153,181,255));
	strncpy_s(m_deviceName, sizeof(m_deviceName), "UNKNOWN", sizeof(m_deviceName));
}

void Renderer::setVertexBufferDeferredUnlocking( bool enabled )
{
	m_deferredVBUnlock = enabled;
}

bool Renderer::getVertexBufferDeferredUnlocking() const
{
	return m_deferredVBUnlock;
}

Renderer::~Renderer(void)
{
}

void Renderer::release(void)
{
	delete this;
}

// get the driver type for this renderer.
Renderer::DriverType Renderer::getDriverType(void) const
{
	return m_driver;
}

// get the offset to the center of a pixel relative to the size of a pixel (so either 0 or 0.5).
scalar Renderer::getPixelCenterOffset(void) const
{
	return m_pixelCenterOffset;
}

// get the name of the hardware device.
const char *Renderer::getDeviceName(void) const
{
	return m_deviceName;
}

// adds a mesh to the render queue.
void Renderer::queueMeshForRender(RendererMeshContext &mesh)
{
	ph_assert2( mesh.isValid(),  "Mesh Context is invalid.");
	ph_assert2(!mesh.isLocked(), "Mesh Context is already locked to a Renderer.");
	if(mesh.isValid() && !mesh.isLocked())
	{
		mesh.m_renderer = this;
		if (mesh.screenSpace)
		{
			m_screenSpaceMeshes.push_back(&mesh);
		}
		else switch (mesh.material->getType())
		{
		case  RendererMaterial::TYPE_LIT:
			m_visibleLitMeshes.push_back(&mesh);
			break;
		default: //case RendererMaterial::TYPE_UNLIT:
			m_visibleUnlitMeshes.push_back(&mesh);
		//	break;
		}
	}
}

// adds a light to the render queue.
void Renderer::queueLightForRender(RendererLight &light)
{
	ph_assert2(!light.isLocked(), "Light is already locked to a Renderer.");
	if(!light.isLocked())
	{
		light.m_renderer = this;
		m_visibleLights.push_back(&light);
	}
}

// renders the current scene to the offscreen buffers. empties the render queue when done.
void Renderer::render(const Matrix4 &eye, const Matrix4 &proj, RendererTarget *target, bool depthOnly)
{
	RENDERER_PERFZONE(Renderer_render);
	const uint32 numLights = (uint32)m_visibleLights.size();
	if(target)
	{
		target->bind();
	}

	// TODO: Sort meshes by material..
	if(beginRender())
	{
		if(!depthOnly)
		{
			// What the hell is this? Why is there specialized code in here for a projection matrix...
			// YOU CAN PASS THE PROJECTION MATIX RIGHT INTO THIS FUNCTION!
			// TODO: Get rid of this.
			if (m_screenSpaceMeshes.size())
			{
				Matrix4 id = Matrix4::IDENTITY;
				Matrix4 pj;
				RendererProjection::makeProjectionMatrix(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,pj);
				bindViewProj(id, pj);	//TODO: pass screen space matrices
				renderMeshes(m_screenSpaceMeshes, RendererMaterial::PASS_UNLIT);	//render screen space stuff first so stuff we occlude doesn't waste time on shading.
			}
		}
		
		if(depthOnly)
		{
			RENDERER_PERFZONE(Renderer_render_depthOnly);
			bindAmbientState(RendererColor(0,0,0,255));
			bindViewProj(eye, proj);
			renderMeshes(m_visibleLitMeshes,   RendererMaterial::PASS_DEPTH);
			renderMeshes(m_visibleUnlitMeshes, RendererMaterial::PASS_DEPTH);
		}
		else  if(numLights > RENDERER_DEFERRED_THRESHOLD)
		{
			RENDERER_PERFZONE(Renderer_render_deferred);
			bindDeferredState();
			bindViewProj(eye, proj);
			renderMeshes(m_visibleLitMeshes,   RendererMaterial::PASS_UNLIT);
			renderMeshes(m_visibleUnlitMeshes, RendererMaterial::PASS_UNLIT);
			renderDeferredLights();
		}
		else if(numLights > 0)
		{
			RENDERER_PERFZONE(Renderer_render_lit);
			bindAmbientState(m_ambientColor);
			bindViewProj(eye, proj);
			RendererLight &light0 = *m_visibleLights[0];
			light0.bind();
			renderMeshes(m_visibleLitMeshes, light0.getPass());
			light0.m_renderer = 0;
			
			bindAmbientState(RendererColor(0,0,0,255));
			beginMultiPass();
			for(uint32 i=1; i<numLights; i++)
			{
				RendererLight &light = *m_visibleLights[i];
				light.bind();
				renderMeshes(m_visibleLitMeshes, light.getPass());
				light.m_renderer = 0;
			}
			endMultiPass();
			renderMeshes(m_visibleUnlitMeshes, RendererMaterial::PASS_UNLIT);
		}
		else
		{
			RENDERER_PERFZONE(Renderer_render_unlit);
			bindAmbientState(RendererColor(0,0,0,255));
			bindViewProj(eye, proj);
			renderMeshes(m_visibleLitMeshes,   RendererMaterial::PASS_UNLIT);
			renderMeshes(m_visibleUnlitMeshes, RendererMaterial::PASS_UNLIT);
		}
		endRender();
	}
	if(target) target->unbind();
	m_visibleLitMeshes.clear();
	m_visibleUnlitMeshes.clear();
	m_screenSpaceMeshes.clear();
	m_visibleLights.clear();
}

// sets the ambient lighting color.
void Renderer::setAmbientColor(const RendererColor &ambientColor)
{
	m_ambientColor   = ambientColor;
	m_ambientColor.a = 255;
}

void Renderer::setClearColor(const RendererColor &clearColor)
{
	m_clearColor   = clearColor;
	m_clearColor.a = 255;
}

void Renderer::renderMeshes(std::vector<RendererMeshContext*> & meshes, RendererMaterial::Pass pass)
{
	RENDERER_PERFZONE(Renderer_renderMeshes);
	
	RendererMaterial         *lastMaterial         = 0;
	RendererMaterialInstance *lastMaterialInstance = 0;
	const RendererMesh       *lastMesh             = 0;
	
	const uint32 numMeshes = (uint32)meshes.size();
	for(uint32 i=0; i<numMeshes; i++)
	{
		RendererMeshContext &context = *meshes[i];
		bindMeshContext(context);
		bool instanced = context.mesh->getInstanceBuffer()?true:false;
		
		if(context.materialInstance && context.materialInstance != lastMaterialInstance)
		{
			if(lastMaterial) lastMaterial->unbind();
			lastMaterialInstance =  context.materialInstance;
			lastMaterial         = &context.materialInstance->getMaterial();
			lastMaterial->bind(pass, lastMaterialInstance, instanced);
		}
		else if(context.material != lastMaterial)
		{
			if(lastMaterial) lastMaterial->unbind();
			lastMaterialInstance = 0;
			lastMaterial         = context.material;
			lastMaterial->bind(pass, lastMaterialInstance, instanced);
		}
		
		if(lastMaterial) lastMaterial->bindMeshState(instanced);
		if(context.mesh != lastMesh)
		{
			if(lastMesh) lastMesh->unbind();
			lastMesh = context.mesh;
			if(lastMesh) lastMesh->bind();
		}
		if(lastMesh) context.mesh->render(context.material);
		context.m_renderer = 0;
	}
	if(lastMesh)     lastMesh->unbind();
	if(lastMaterial) lastMaterial->unbind();
}


void Renderer::renderDeferredLights(void)
{
	RENDERER_PERFZONE(Renderer_renderDeferredLights);
	
	const uint32 numLights = (uint32)m_visibleLights.size();
	for(uint32 i=0; i<numLights; i++)
	{
		renderDeferredLight(*m_visibleLights[i]);
	}
}

_NAMESPACE_END