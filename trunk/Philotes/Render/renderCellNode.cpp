#include "renderCellNode.h"
#include "renderTransform.h"

_NAMESPACE_BEGIN

RenderCellNode::RenderCellNode()
{

}

RenderCellNode::RenderCellNode( const String& name )
{

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
	mWorldAABB.setNull();

	// 
}

void RenderCellNode::_update( bool updateChildren, bool parentHasChanged )
{
	RenderNode::_update(updateChildren,parentHasChanged);
	_updateBounds();
}

const AxisAlignedBox& RenderCellNode::_getWorldAABB( void ) const
{
	return mWorldAABB;
}


_NAMESPACE_END