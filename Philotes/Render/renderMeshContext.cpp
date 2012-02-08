
#include <renderMeshContext.h>

_NAMESPACE_BEGIN

RendererMeshContext::RendererMeshContext(void)
{
	mesh             = 0;
	material         = 0;
	materialInstance = 0;
	transform        = 0;
	boneMatrices     = 0;
	numBones         = 0;
	m_renderer       = 0;
    cullMode         = CLOCKWISE;
	screenSpace		 = false;
}

RendererMeshContext::~RendererMeshContext(void)
{
	ph_assert2(isLocked()==false, "Mesh Context still locked to a Renderer instance during destruction!");
}

bool RendererMeshContext::isValid(void) const
{
	bool ok = true;
	if(!mesh)     ok = false;
	if(!material) ok = false;
	return ok;
}

bool RendererMeshContext::isLocked(void) const
{
	return m_renderer ? true : false;
}

_NAMESPACE_END