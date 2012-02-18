
#pragma once

#include "renderElement.h"

_NAMESPACE_BEGIN

class RenderSubModel : RenderElement
{
public:

	RenderSubModel(RenderModel* parent, GearSubMesh* mesh);

	virtual ~RenderSubModel();

public:

	

protected:

	RenderModel*	mParentModel;

	GearSubMesh*	mSubMesh;
};

_NAMESPACE_END