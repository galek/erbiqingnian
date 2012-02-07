
#ifndef RENDERER_DIRECTIONAL_LIGHT_DESC_H
#define RENDERER_DIRECTIONAL_LIGHT_DESC_H

#include <RendererLightDesc.h>

class RendererDirectionalLightDesc : public RendererLightDesc
{
	public:
		Vector3 direction;
		
	public:
		RendererDirectionalLightDesc(void);
		
		virtual bool isValid(void) const;
};

#endif
