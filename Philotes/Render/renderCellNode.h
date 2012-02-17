
#pragma once

#include "renderNode.h"
#include "math/axisAlignedBox.h"

/// 最小场景裁剪单元，场景在此基础上进行裁剪

_NAMESPACE_BEGIN

class RenderCellNode : public RenderNode
{
public:

	RenderCellNode();

	RenderCellNode(const String& name);

	virtual ~RenderCellNode();

public:

	// CellNode的子节点均为Transform类型
	
	virtual RenderNode* createChildImpl(void);

	virtual RenderNode* createChildImpl(const String& name);

	virtual void _updateBounds(void);

	virtual void _update(bool updateChildren, bool parentHasChanged);

	virtual const AxisAlignedBox& _getWorldAABB(void) const;

protected:

	AxisAlignedBox mWorldAABB;
};

_NAMESPACE_END