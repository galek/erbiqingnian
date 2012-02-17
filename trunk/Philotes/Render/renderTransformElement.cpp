
#include "renderTransformElement.h"
#include "renderNode.h"

_NAMESPACE_BEGIN

RenderTransformElement::RenderTransformElement()
{
	m_parentNode = NULL;
}

RenderTransformElement::RenderTransformElement( const String& name )
{
	m_parentNode = NULL;
	m_name = name;
}

RenderTransformElement::~RenderTransformElement()
{

}

const Matrix4& RenderTransformElement::getTransform( void ) const
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

void RenderTransformElement::_notifyAttached( RenderNode* parent )
{
	m_parentNode = parent;
}


_NAMESPACE_END

