
#pragma once

_NAMESPACE_BEGIN

class RenderTransformElement
{
public:

	RenderTransformElement();

	RenderTransformElement(const String& name);

	virtual						~RenderTransformElement();

public:

	virtual const Matrix4&		getTransform(void) const;

	virtual void				_notifyAttached(RenderNode* parent);

	virtual void				visitRenderElement(RenderVisitor* visitor){}

	RenderNode*					getParentNode() const { return m_parentNode; }

	virtual const String&		getName(void) const { return m_name; }

	bool						isAttached()const {return m_parentNode!=NULL;}

protected:

	RenderNode*					m_parentNode;

	String						m_name;
};

_NAMESPACE_END