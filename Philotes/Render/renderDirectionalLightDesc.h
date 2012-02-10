
#ifndef RENDERER_DIRECTIONAL_LIGHT_DESC_H
#define RENDERER_DIRECTIONAL_LIGHT_DESC_H

#include <renderLightDesc.h>

_NAMESPACE_BEGIN

class RenderDirectionalLightDesc : public RenderLightDesc
{
	public:
		Vector3 direction;
		
	public:
		RenderDirectionalLightDesc(void);
		
		virtual bool isValid(void) const;
};

_NAMESPACE_END

#endif
