
#pragma once

#include "renderElement.h"

_NAMESPACE_BEGIN

class RenderSubModel : RenderElement
{
public:

	RenderSubModel(RenderModel* parent);

	virtual ~RenderSubModel();

public:

	RenderSubMesh* getSubMesh();

	void getWorldTransforms(Matrix4* xform) const;

protected:

	RenderModel*	m_parentModel;

	RenderSubMesh*	m_subMesh;
};

_NAMESPACE_END