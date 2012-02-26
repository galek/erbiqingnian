
#include "renderSubMesh.h"

_NAMESPACE_BEGIN

RenderSubMesh::RenderSubMesh()
	:m_vertexBuffer(NULL),
	m_indexBuffer(NULL)
{

}

RenderSubMesh::~RenderSubMesh()
{

}

void RenderSubMesh::setParent( RenderMesh* parent )
{
	m_parentMesh = parent;
}


_NAMESPACE_END