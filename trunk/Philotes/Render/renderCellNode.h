
#pragma once

#include "renderNode.h"
#include "math/axisAlignedBox.h"

/// ��С�����ü���Ԫ�������ڴ˻����Ͻ��вü�

_NAMESPACE_BEGIN

class RenderCellNode : public RenderNode
{
public:

	RenderCellNode();

	RenderCellNode(const String& name);

	virtual ~RenderCellNode();

public:

	// CellNode���ӽڵ��ΪTransform����
	
	virtual RenderNode* createChildImpl(void);

	virtual RenderNode* createChildImpl(const String& name);

	virtual void _updateBounds(void);

	virtual void _update(bool updateChildren, bool parentHasChanged);

	virtual const AxisAlignedBox& _getWorldAABB(void) const;

protected:

	AxisAlignedBox mWorldAABB;
};

_NAMESPACE_END