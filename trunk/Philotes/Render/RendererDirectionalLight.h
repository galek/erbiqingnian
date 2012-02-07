
#ifndef RENDERER_DIRECTIONAL_LIGHT_H
#define RENDERER_DIRECTIONAL_LIGHT_H

#include <RendererLight.h>

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

#endif
