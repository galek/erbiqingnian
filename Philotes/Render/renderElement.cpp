
#include "renderElement.h"
#include "renderMaterialInstance.h"
#include "renderNode.h"
#include "renderMesh.h"

_NAMESPACE_BEGIN

RenderElement::RenderElement(void)
{
	m_parentNode		= 0;
	m_mesh				= 0;
	m_materialInstance	= 0;
	m_renderer			= 0;
}

RenderElement::RenderElement( const String& name )
{
	m_parentNode		= 0;
	m_mesh				= 0;
	m_materialInstance	= 0;
	m_renderer			= 0;
	m_name				= name;
}

RenderElement::~RenderElement(void)
{
	ph_assert2(isLocked()==false, "Mesh Context still locked to a Render instance during destruction!");
}

bool RenderElement::isValid(void) const
{
	bool ok = true;
	if(!m_mesh)     
	{
		ok = false;
	}
	if (!m_materialInstance)
	{
		ok = false;
	}
	return ok;
}

bool RenderElement::isLocked(void) const
{
	return m_renderer ? true : false;
}

RenderMaterial* RenderElement::getMaterial()
{
	return &m_materialInstance->getMaterial();
}

const Matrix4& RenderElement::getTransform( void ) const
{
	if (m_parentNode)
	{
		return m_parentNode->_getFullTransform();
	}
	else
	{
		return Matrix4::IDENTITY;
	}
}

void RenderElement::_notifyAttached( RenderNode* parent )
{
	m_parentNode = parent;
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