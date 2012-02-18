
#include "renderSubModel.h"
#include "renderModel.h"
#include "gearsMesh.h"

_NAMESPACE_BEGIN

RenderSubModel::RenderSubModel( RenderModel* parent, GearSubMesh* mesh )
	:RenderElement(),
	mParentModel(parent),mSubMesh(mesh)
{

}

RenderSubModel::~RenderSubModel()
{

}


_NAMESPACE_END