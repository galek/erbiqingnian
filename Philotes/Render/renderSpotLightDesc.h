
#ifndef RENDERER_SPOT_LIGHT_DESC_H
#define RENDERER_SPOT_LIGHT_DESC_H

#include <renderLightDesc.h>

_NAMESPACE_BEGIN

class RenderSpotLightDesc : public RenderLightDesc
{
	public:
		Vector3	   position;
		Vector3	   direction;
		
		scalar             innerRadius;
		scalar             outerRadius;
		
		// cone = dot(direction, directionToFragment);
		// so cone value of 0 is completely open and value of 1 is completely closed.
		scalar             innerCone;
		scalar             outerCone;
		
	public:
		RenderSpotLightDesc(void);
		
		virtual bool isValid(void) const;
};

_NAMESPACE_END

#endif
