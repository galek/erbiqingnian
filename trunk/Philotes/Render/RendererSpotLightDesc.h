
#ifndef RENDERER_SPOT_LIGHT_DESC_H
#define RENDERER_SPOT_LIGHT_DESC_H

#include <RendererLightDesc.h>

class RendererSpotLightDesc : public RendererLightDesc
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
		RendererSpotLightDesc(void);
		
		virtual bool isValid(void) const;
};

#endif
