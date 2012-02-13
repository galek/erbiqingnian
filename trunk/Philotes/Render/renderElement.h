
#pragma once

_NAMESPACE_BEGIN

class RenderElement
{
public:
	friend class Render;

	RenderElement(void);

	RenderElement(const String& name);

	virtual ~RenderElement(void);

public:

	RenderMaterial*				getMaterial();

	virtual const Matrix4&		getTransform(void) const;

	RenderNode*					getParentNode() const { return m_parentNode; }
	
	bool						isAttached()const{return m_parentNode!=NULL;}

	virtual const String&		getName(void) const { return m_name; }

	RenderMaterialInstance *	getMaterialInstance() const { return m_materialInstance; }

	void						setMaterialInstance(RenderMaterialInstance * val);

	RenderMesh*					getMesh() const { return m_mesh; }

	virtual void				setMesh(RenderMesh * val);

	virtual void				_notifyAttached(RenderNode* parent);

	virtual bool				preQueuedToRender(){return true;}

protected:

	RenderMesh					*m_mesh;
	
	RenderMaterialInstance		*m_materialInstance;
	
	RenderNode*					m_parentNode;

	String						m_name;
};

_NAMESPACE_END

