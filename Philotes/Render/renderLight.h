
#ifndef RENDERER_LIGHT_H
#define RENDERER_LIGHT_H

#include <renderConfig.h>
#include <renderColor.h>
#include <renderMaterial.h>
#include <renderProjection.h>

_NAMESPACE_BEGIN

class RendererLightDesc;
class Renderer;
class RendererLight
{
	friend class Renderer;
	public:
		typedef enum Type
		{
			TYPE_POINT = 0,
			TYPE_DIRECTIONAL,
			TYPE_SPOT,
			
			NUM_TYPES
		};
		
	protected:
		RendererLight(const RendererLightDesc &desc);
		virtual ~RendererLight(void);
		
	public:
		void						release(void) { delete this; }
		
		Type						getType(void) const;
		
		RendererMaterial::Pass		getPass(void) const;
		
		const RendererColor&		getColor(void) const;
		void						setColor(const RendererColor &color);
		
		float						getIntensity(void) const;
		void						setIntensity(float intensity);
		
		bool						isLocked(void) const;
		
		RendererTexture2D*			getShadowMap(void) const;
		void						setShadowMap(RendererTexture2D *shadowMap);
		
		const Matrix4&				getShadowTransform(void) const;
		void						setShadowTransform(const Matrix4 &shadowTransform);
		
		const Matrix4&				getShadowProjection(void) const;
		void						setShadowProjection(const Matrix4 &shadowProjection);
	
	private:
		RendererLight&				operator=(const RendererLight &) { return *this; }
		
		virtual void				bind(void) const = 0;
		
	protected:
		const Type					m_type;
		
		RendererColor				m_color;
		float						m_intensity;
		
		RendererTexture2D*			m_shadowMap;
		Matrix4						m_shadowTransform;
		Matrix4						m_shadowProjection;
	
	private:
		Renderer*					m_renderer;
};

_NAMESPACE_END

#endif
