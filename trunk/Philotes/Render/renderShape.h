
#ifndef RENDERER_SHAPE_H
#define RENDERER_SHAPE_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class Render;
class RenderMesh;

class RenderShape
{
	protected:
		RenderShape(Render &renderer);
	
	public:
		virtual ~RenderShape(void);
		
		RenderMesh *getMesh(void);
	
	private:

		RenderShape &operator = (const RenderShape &) { return *this; }
		
	protected:
		Render     &m_renderer;
		RenderMesh *m_mesh;
};

_NAMESPACE_END

#endif
