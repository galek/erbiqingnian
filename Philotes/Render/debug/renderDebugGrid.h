
#pragma once

#include "renderElement.h"
#include "renderShape.h"

_NAMESPACE_BEGIN

class RenderGridElement : public RenderElement
{
public:
	RenderGridElement(uint32 size, float cellSize);

	virtual ~RenderGridElement(void);

	virtual void getWorldTransforms(Matrix4* xform) const;

private:

	RenderVertexBuffer	*m_vertexBuffer;

	GearMaterialAsset	*m_materialAsset;
};

_NAMESPACE_END