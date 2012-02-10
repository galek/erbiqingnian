
#pragma once

#include "renderNode.h"

/// ������Ⱦ�������任�Ľڵ㣬�������𳡾�����Ĳü�
/// �任�ڵ���Ҫ�ҽ��ڳ����ڵ��У���ʵ�ֳ�������

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

	typedef HashTable<String, RenderElement*>	ObjectMap;
	typedef ObjectMap::Iterator					ObjectIterator;

public:

	virtual void			attachObject(RenderElement* obj);

    virtual unsigned short	numAttachedObjects(void) const;

    virtual RenderElement*	getAttachedObject(unsigned short index);

    virtual RenderElement*	getAttachedObject(const String& name);

    virtual RenderElement*	detachObject(unsigned short index);
    
    virtual void			detachObject(RenderElement* obj);

    virtual RenderElement*	detachObject(const String& name);

    virtual void			detachAllObjects(void);

protected:

	ObjectMap mObjectsByName;

};

_NAMESPACE_END