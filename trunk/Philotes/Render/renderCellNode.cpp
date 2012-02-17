#include "renderCellNode.h"
#include "renderTransform.h"
#include "renderSceneManager.h"
#include "renderElement.h"
#include "render.h"
#include "renderTransformElement.h"
#include "gearsApplication.h"

_NAMESPACE_BEGIN

class SimpleRenderVisitor : public RenderVisitor
{
	virtual void visit(RenderElement* rend,Any* pAny = 0)
	{
		GearApplication::getApp()->getRender()->queueMeshForRender(*rend);
	}
};

//////////////////////////////////////////////////////////////////////////

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
	ChildNodeIterator it;
	ChildNodeIterator itend = mChildren.End();
	for (it = mChildren.Begin(); it!=itend; ++it)
	{
		if(it->Value()->getNodeType() == NT_TRANSFORM)
		{
			RenderTransform* tn = static_cast<RenderTransform*>(it->Value());
			// TODO : Ð§ÂÊÌáÉý
			size_t at = tn->numAttachedObjects();
			for (size_t i=0; i<at; i++)
			{
				SimpleRenderVisitor sv;
				tn->getAttachedObject(i)->visitRenderElement(&sv);
			}
		}
		else if (it->Value()->getNodeType() == NT_CULL_CELL)
		{
			RenderCellNode* cn = static_cast<RenderCellNode*>(it->Value());
			cn->tickVisible(camera);
		}
	}
}

_NAMESPACE_END
