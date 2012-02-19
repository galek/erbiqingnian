
#include "renderSubModel.h"
#include "renderModel.h"
#include "renderSubMesh.h"

_NAMESPACE_BEGIN

RenderSubModel::RenderSubModel( RenderModel* parent)
	:RenderElement(),
	m_parentModel(parent)
{

}

RenderSubModel::~RenderSubModel()
{

}

RenderSubMesh* RenderSubModel::getSubMesh() const
{
	return static_cast<RenderSubMesh*>(m_mesh);
}

void RenderSubModel::getWorldTransforms( Matrix4* xform ) const
{

}

_NAMESPACE_END