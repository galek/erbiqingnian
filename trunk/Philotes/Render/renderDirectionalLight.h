
#ifndef RENDERER_DIRECTIONAL_LIGHT_H
#define RENDERER_DIRECTIONAL_LIGHT_H

#include <renderLight.h>

_NAMESPACE_BEGIN

class RendererDirectionalLightDesc;

class RendererDirectionalLight : public RendererLight
{
	protected:
		RendererDirectionalLight(const RendererDirectionalLightDesc &desc);
		virtual ~RendererDirectionalLight(void);
	
	public:
		const Vector3 &getDirection(void) const;
		void          setDirection(const Vector3 &dir);
		
	protected:
		Vector3 m_direction;
};

_NAMESPACE_END

#endif
