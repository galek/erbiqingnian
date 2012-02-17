
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

const Sphere& RenderTransformElement::getWorldBoundingSphere( bool derive /*= false*/ ) const
{
	if (derive)
	{
		const Vector3& scl = m_parentNode->_getDerivedScale();
		scalar factor = Math::Max(Math::Max(scl.x, scl.y), scl.z);
		mWorldBoundingSphere.setRadius(getBoundingRadius() * factor);
		mWorldBoundingSphere.setCenter(m_parentNode->_getDerivedPosition());
	}
	return mWorldBoundingSphere;
}

const AxisAlignedBox& RenderTransformElement::getWorldBoundingBox( bool derive /*= false*/ ) const
{
	if (derive)
	{
		mWorldAABB = this->getBoundingBox();
		mWorldAABB.transformAffine(getTransform());
	}

	return mWorldAABB;
}

_NAMESPACE_END

