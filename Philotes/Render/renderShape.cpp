
//
// RenderShape : base class for convenience classes for generating meshes based on shapes.
//
#include <renderShape.h>

_NAMESPACE_BEGIN

RenderShape::RenderShape(Render &renderer) :
	m_renderer(renderer)
{
	m_mesh = 0;
}

RenderShape::~RenderShape(void)
{
	ph_assert2(m_mesh==0, "Mesh was not properly released before Shape destruction.");
}
	
RenderMesh *RenderShape::getMesh(void)
{
	return m_mesh;
}

_NAMESPACE_END