
#pragma once

#include "renderBase.h"

_NAMESPACE_BEGIN

class RenderSubMesh
{
public:

	RenderSubMesh();

	virtual ~RenderSubMesh();

public:

	void setParent(RenderMesh* parent);

protected:

	RenderMesh*				m_parentMesh;

	RenderVertexBuffer*		m_vertexBuffer;

	RenderIndexBuffer*		m_indexBuffer;
};

_NAMESPACE_END