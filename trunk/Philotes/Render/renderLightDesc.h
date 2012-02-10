
#ifndef RENDERER_LIGHT_DESC_H
#define RENDERER_LIGHT_DESC_H

#include <renderLight.h>
#include <renderProjection.h>

_NAMESPACE_BEGIN

class RenderLightDesc
{
	public:
		const RenderLight::Type	type;
		
		RenderColor				color;
		float						intensity;
		
		RenderTexture2D*			shadowMap;
		Matrix4						shadowTransform;
		Matrix4						shadowProjection;
		
	protected:
		RenderLightDesc(RenderLight::Type _type);

		virtual ~RenderLightDesc() {}
		
	public:
		virtual bool isValid(void) const;
	
	private:
		RenderLightDesc &operator=(const RenderLightDesc&) { return *this; }
};

_NAMESPACE_END

#endif
