
//
// RendererShape : base class for convenience classes for generating meshes based on shapes.
//
#include <RendererShape.h>

RendererShape::RendererShape(Renderer &renderer) :
	m_renderer(renderer)
{
	m_mesh = 0;
}

RendererShape::~RendererShape(void)
{
	ph_assert2(m_mesh==0, "Mesh was not properly released before Shape destruction.");
}
	
RendererMesh *RendererShape::getMesh(void)
{
	return m_mesh;
}
