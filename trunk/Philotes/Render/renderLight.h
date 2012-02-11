
#ifndef RENDERER_LIGHT_H
#define RENDERER_LIGHT_H

#include <renderConfig.h>
#include <renderMaterial.h>

_NAMESPACE_BEGIN

class RenderLightDesc;
class Render;
class RenderLight
{
	friend class Render;
	public:
		typedef enum Type
		{
			TYPE_POINT = 0,
			TYPE_DIRECTIONAL,
			TYPE_SPOT,
			
			NUM_TYPES
		};
		
	protected:
		RenderLight(const RenderLightDesc &desc);
		virtual ~RenderLight(void);
		
	public:
		void						release(void) { delete this; }
		
		Type						getType(void) const;
		
		RenderMaterial::Pass		getPass(void) const;
		
		const Colour&				getColor(void) const;
		void						setColor(const Colour &color);
		
		float						getIntensity(void) const;
		void						setIntensity(float intensity);
		
		bool						isLocked(void) const;
		
		RenderTexture2D*			getShadowMap(void) const;
		void						setShadowMap(RenderTexture2D *shadowMap);
		
		const Matrix4&				getShadowTransform(void) const;
		void						setShadowTransform(const Matrix4 &shadowTransform);
		
		const Matrix4&				getShadowProjection(void) const;
		void						setShadowProjection(const Matrix4 &shadowProjection);
	
	private:
		RenderLight&				operator=(const RenderLight &) { return *this; }
		
		virtual void				bind(void) const = 0;
		
	protected:
		const Type					m_type;
		
		Colour						m_color;
		float						m_intensity;
		
		RenderTexture2D*			m_shadowMap;
		Matrix4						m_shadowTransform;
		Matrix4						m_shadowProjection;
	
	private:
		Render*						m_renderer;
};

_NAMESPACE_END

#endif
