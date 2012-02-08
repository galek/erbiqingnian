
#ifndef RENDERER_SHAPE_H
#define RENDERER_SHAPE_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class Renderer;
class RendererMesh;

class RendererShape
{
	protected:
		RendererShape(Renderer &renderer);
	
	public:
		virtual ~RendererShape(void);
		
		RendererMesh *getMesh(void);
	
	private:
		RendererShape &operator=(const RendererShape &) { return *this; }
		
	protected:
		Renderer     &m_renderer;
		RendererMesh *m_mesh;
};

_NAMESPACE_END

#endif
