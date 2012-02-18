
#ifndef RENDERER_SHAPE_H
#define RENDERER_SHAPE_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class Render;
class RenderBase;

class RenderShape
{
	protected:
		RenderShape(Render &renderer);
	
	public:
		virtual ~RenderShape(void);
		
		RenderBase *getMesh(void);
	
	private:

		RenderShape &operator = (const RenderShape &) { return *this; }
		
	protected:
		Render     &m_renderer;
		RenderBase *m_mesh;
};

_NAMESPACE_END

#endif
