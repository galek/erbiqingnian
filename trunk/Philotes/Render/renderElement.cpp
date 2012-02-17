
#include "renderElement.h"
#include "renderMaterialInstance.h"
#include "renderNode.h"
#include "renderMesh.h"

_NAMESPACE_BEGIN

RenderElement::RenderElement(void)
{
	m_mesh				= 0;
	m_materialInstance	= 0;
}

RenderElement::~RenderElement(void)
{
}

RenderMaterial* RenderElement::getMaterial()
{
	return &m_materialInstance->getMaterial();
}

void RenderElement::setMaterialInstance( RenderMaterialInstance * val )
{
	if (m_materialInstance)
	{
		delete m_materialInstance;
	}
	m_materialInstance = val;
}

void RenderElement::setMesh( RenderMesh * val )
{
// 	if (m_mesh)
// 	{
// 		delete m_mesh;
// 	}
	m_mesh = val;
}

_NAMESPACE_END