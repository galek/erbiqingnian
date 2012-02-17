
#pragma once

#include "math/sphere.h"
#include "math/axisAlignedBox.h"

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

	virtual void				visitRenderElement(RenderVisitor* visitor) = 0;

	RenderNode*					getParentNode() const { return m_parentNode; }

	virtual const String&		getName(void) const { return m_name; }

	bool						isAttached()const {return m_parentNode!=NULL;}

	virtual const AxisAlignedBox& getBoundingBox(void) const = 0;

	virtual scalar				getBoundingRadius(void) const = 0;

	virtual const AxisAlignedBox& getWorldBoundingBox(bool derive = false) const;

	virtual const Sphere&		getWorldBoundingSphere(bool derive = false) const;

protected:

	RenderNode*					m_parentNode;

	String						m_name;

	mutable AxisAlignedBox		mWorldAABB;
	
	mutable Sphere				mWorldBoundingSphere;
};

_NAMESPACE_END