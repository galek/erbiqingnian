
#pragma once

#include "renderElement.h"

_NAMESPACE_BEGIN

class RenderSubModel : RenderElement
{
public:

	RenderSubModel(RenderModel* parent);

	virtual ~RenderSubModel();

public:

	RenderSubMesh* getSubMesh() const;

	void getWorldTransforms(Matrix4* xform) const;

protected:

	RenderModel*	m_parentModel;
};

_NAMESPACE_END