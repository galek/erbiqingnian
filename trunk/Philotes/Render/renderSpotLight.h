
#ifndef RENDERER_SPOT_LIGHT_H
#define RENDERER_SPOT_LIGHT_H

#include <renderLight.h>

_NAMESPACE_BEGIN

class RenderSpotLightDesc;

class RenderSpotLight : public RenderLight
{
	protected:
		RenderSpotLight(const RenderSpotLightDesc &desc);

		virtual ~RenderSpotLight(void);
	
	public:
		const Vector3		&getPosition(void) const;
		void				setPosition(const Vector3 &pos);
		
		const Vector3		&getDirection(void) const;
		void                setDirection(const Vector3 &dir);
		
		scalar              getInnerRadius(void) const;
		scalar              getOuterRadius(void) const;
		void                setRadius(scalar innerRadius, scalar outerRadius);
		
		scalar              getInnerCone(void) const;
		scalar              getOuterCone(void) const;
		void                setCone(scalar innerCone, scalar outerCone);
		
	protected:
		Vector3				m_position;
		Vector3				m_direction;
		scalar				m_innerRadius;
		scalar				m_outerRadius;
		scalar				m_innerCone;
		scalar				m_outerCone;
};

_NAMESPACE_END

#endif
