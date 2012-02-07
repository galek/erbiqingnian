
#ifndef RENDERER_LIGHT_DESC_H
#define RENDERER_LIGHT_DESC_H

#include <RendererLight.h>
#include <RendererProjection.h>

class RendererLightDesc
{
	public:
		const RendererLight::Type	type;
		
		RendererColor				color;
		float						intensity;
		
		RendererTexture2D*			shadowMap;
		Matrix4						shadowTransform;
		Matrix4						shadowProjection;
		
	protected:
		RendererLightDesc(RendererLight::Type _type);

		virtual ~RendererLightDesc() {}
		
	public:
		virtual bool isValid(void) const;
	
	private:
		RendererLightDesc &operator=(const RendererLightDesc&) { return *this; }
};

#endif
