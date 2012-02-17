
#pragma once

#include "renderNode.h"
#include "math/axisAlignedBox.h"

/// ��С�����ü���Ԫ�������ڴ˻����Ͻ��вü�

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

	// CellNode���ӽڵ��ΪTransform����
	virtual RenderNode* createChildImpl(void);

	virtual RenderNode* createChildImpl(const String& name);

	RenderSceneManager* getCreator(){return m_sceneManager;}

	virtual void tickVisible(const RenderCamera* camera);

protected:

	AxisAlignedBox m_worldAABB;

	RenderSceneManager* m_sceneManager;
};

_NAMESPACE_END