
#pragma once

#include "renderNode.h"
#include "math/axisAlignedBox.h"

/// 最小场景裁剪单元，场景在此基础上进行裁剪

_NAMESPACE_BEGIN

class RenderCellNode : public RenderNode
{
public:

	RenderCellNode(RenderSceneManager* sm);

	RenderCellNode(RenderSceneManager* sm,const String& name);

	virtual ~RenderCellNode();

public:

	virtual void _updateBounds(void);

	virtual void _update(bool updateChildren, bool parentHasChanged);

	virtual const AxisAlignedBox& _getWorldAABB(void) const;

	virtual RenderTransform* createChildTransformNode(
		const Vector3& translate = Vector3::ZERO, 
		const Quaternion& rotate = Quaternion::IDENTITY );

	virtual RenderTransform* createChildTransformNode(const String& name, 
		const Vector3& translate = Vector3::ZERO, 
		const Quaternion& rotate = Quaternion::IDENTITY);

	// CellNode的子节点均为Transform类型
	virtual RenderNode* createChildImpl(void);

	virtual RenderNode* createChildImpl(const String& name);

	RenderSceneManager* getCreator(){return m_sceneManager;}

	virtual void tickVisible(const RenderCamera* camera);

protected:

	AxisAlignedBox m_worldAABB;

	RenderSceneManager* m_sceneManager;
};

_NAMESPACE_END