
#include "renderMesh.h"
#include "renderSubMesh.h"

_NAMESPACE_BEGIN

RenderMesh::RenderMesh( const String& name )
	:m_name(name)
{

}

RenderMesh::RenderMesh()
{

}

RenderMesh::~RenderMesh()
{

}

RenderSubMesh* RenderMesh::createSubMesh()
{
	RenderSubMesh* sub = ph_new(RenderSubMesh);
	sub->setParent(this);

	m_subMeshes.Append(sub);

	return sub;
}


_NAMESPACE_END