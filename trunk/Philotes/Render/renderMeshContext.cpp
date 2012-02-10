
#include "renderMeshContext.h"
#include "renderMaterialInstance.h"
#include "renderNode.h"

_NAMESPACE_BEGIN

RenderElement::RenderElement(void)
{
	m_parentNode		= 0;
	mesh				= 0;
	materialInstance	= 0;
	boneMatrices		= 0;
	numBones			= 0;
	m_renderer			= 0;
    cullMode			= CLOCKWISE;
	screenSpace			= false;
}

RenderElement::RenderElement( const String& name )
{
	m_parentNode		= 0;
	mesh				= 0;
	materialInstance	= 0;
	boneMatrices		= 0;
	numBones			= 0;
	m_renderer			= 0;
	cullMode			= CLOCKWISE;
	screenSpace			= false;
	m_name				= name;
}

RenderElement::~RenderElement(void)
{
	ph_assert2(isLocked()==false, "Mesh Context still locked to a Render instance during destruction!");
}

bool RenderElement::isValid(void) const
{
	bool ok = true;
	if(!mesh)     
	{
		ok = false;
	}
	if (!materialInstance)
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
	return &materialInstance->getMaterial();
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

_NAMESPACE_END