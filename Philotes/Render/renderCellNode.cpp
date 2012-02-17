#include "renderCellNode.h"
#include "renderTransform.h"
#include "renderSceneManager.h"

_NAMESPACE_BEGIN

RenderCellNode::RenderCellNode(RenderSceneManager* sm)
	:RenderNode(),m_sceneManager(sm)
{
	mNodeType = NT_CULL_CELL;
}

RenderCellNode::RenderCellNode(RenderSceneManager* sm, const String& name )
	:RenderNode(name),m_sceneManager(sm)
{
	mNodeType = NT_CULL_CELL;
}

RenderCellNode::~RenderCellNode()
{

}

RenderNode* RenderCellNode::createChildImpl( void )
{
	return ph_new(RenderTransform);
}

RenderNode* RenderCellNode::createChildImpl( const String& name )
{
	return ph_new(RenderTransform)(name);
}

void RenderCellNode::_updateBounds( void )
{
	m_worldAABB.setNull();

	ChildNodeIterator it, itend;
	itend = mChildren.End();
	for (it = mChildren.Begin(); it != itend; ++it)
	{
		RenderTransform* child = static_cast<RenderTransform*>(it->Value());
		m_worldAABB.merge(child->_updateBounds());
	}
}

void RenderCellNode::_update( bool updateChildren, bool parentHasChanged )
{
	RenderNode::_update(updateChildren,parentHasChanged);
	_updateBounds();
}

const AxisAlignedBox& RenderCellNode::_getWorldAABB( void ) const
{
	return m_worldAABB;
}

RenderTransform* RenderCellNode::createChildTransformNode( const Vector3& translate /*= Vector3::ZERO*/,
														  const Quaternion& rotate /*= Quaternion::IDENTITY */ )
{
	return static_cast<RenderTransform*>(this->createChild(translate, rotate));
}

RenderTransform* RenderCellNode::createChildTransformNode( const String& name, const Vector3& translate /*= Vector3::ZERO*/,
														  const Quaternion& rotate /*= Quaternion::IDENTITY*/ )
{
	return static_cast<RenderTransform*>(this->createChild(name,translate, rotate));
}

void RenderCellNode::tickVisible( const RenderCamera* camera )
{
	
}

_NAMESPACE_END