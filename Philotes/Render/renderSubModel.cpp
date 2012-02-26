
#include "renderSubModel.h"
#include "renderModel.h"
#include "renderSubMesh.h"

_NAMESPACE_BEGIN

RenderSubModel::RenderSubModel( RenderModel* parent)
	:RenderElement(),
	m_parentModel(parent),
	m_subMesh(NULL)
{

}

RenderSubModel::~RenderSubModel()
{

}

RenderSubMesh* RenderSubModel::getSubMesh()
{
	return m_subMesh;
}

void RenderSubModel::getWorldTransforms( Matrix4* xform ) const
{

}

_NAMESPACE_END