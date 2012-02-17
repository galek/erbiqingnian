
#pragma once

#include "util/any.h"

_NAMESPACE_BEGIN

class RenderVisitor
{
public:

	virtual ~RenderVisitor() {}

	virtual void visit(RenderElement* rend,Any* pAny = 0) = 0;
};

//////////////////////////////////////////////////////////////////////////

class RenderElement
{
public:
	friend class Render;

	RenderElement(void);

	virtual						~RenderElement(void);

public:

	RenderMaterial*				getMaterial();

	RenderMaterialInstance *	getMaterialInstance() const { return m_materialInstance; }

	void						setMaterialInstance(RenderMaterialInstance * val);

	RenderMesh*					getMesh() const { return m_mesh; }

	virtual void				setMesh(RenderMesh * val);

	virtual bool				preQueuedToRender(){return true;}

	virtual bool				postQueuedToRender(){return true;}

	virtual void				getWorldTransforms(Matrix4* xform) const = 0;

protected:

	// ��Ⱦ����
	RenderMesh*					m_mesh;
	
	// ����ʵ��
	RenderMaterialInstance*		m_materialInstance;
};

_NAMESPACE_END

