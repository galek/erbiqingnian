
#pragma once

#include "math/axisAlignedBox.h"
#include "renderNode.h"

/// 负责渲染物体矩阵变换的节点，但不负责场景管理的裁剪
/// 变换节点需要挂接在场景节点中，以实现场景管理

_NAMESPACE_BEGIN

class RenderTransform : public RenderNode
{
public:

	RenderTransform();

	RenderTransform(const String& name);

	virtual ~RenderTransform();

public:

	virtual RenderNode*		createChildImpl(void);

	virtual RenderNode*		createChildImpl(const String& name);

	typedef HashTable<String, RenderTransformElement*>	ObjectMap;

	typedef ObjectMap::Iterator ObjectIterator;

public:

	virtual void					attachObject(RenderTransformElement* obj);

    virtual unsigned short			numAttachedObjects(void) const;

    virtual RenderTransformElement*	getAttachedObject(unsigned short index);

    virtual RenderTransformElement*	getAttachedObject(const String& name);

    virtual RenderTransformElement*	detachObject(unsigned short index);
    
    virtual void					detachObject(RenderTransformElement* obj);

    virtual RenderTransformElement*	detachObject(const String& name);

    virtual void					detachAllObjects(void);

	const AxisAlignedBox&			_updateBounds();

protected:

	ObjectMap						mObjectsByName;

	AxisAlignedBox					mWorldAABB;
};

_NAMESPACE_END