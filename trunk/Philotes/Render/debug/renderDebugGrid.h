
#pragma once

#include "renderMeshContext.h"
#include "renderShape.h"

_NAMESPACE_BEGIN

class RendererGridShape : RendererShape
{
public:
	RendererGridShape(Renderer &renderer, uint32 size, float cellSize);

	virtual ~RendererGridShape(void);

private:

	RendererVertexBuffer *m_vertexBuffer;
};

_NAMESPACE_END