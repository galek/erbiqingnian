
#pragma once

#include "renderElement.h"
#include "renderShape.h"

_NAMESPACE_BEGIN

class RenderGridShape : RenderShape
{
public:
	RenderGridShape(Render &renderer, uint32 size, float cellSize);

	virtual ~RenderGridShape(void);

private:

	RenderVertexBuffer *m_vertexBuffer;
};

_NAMESPACE_END