
#pragma once

_NAMESPACE_BEGIN

class RenderElement
{
	friend class Render;
	public:
		const RenderMesh			*mesh;
		RenderMaterialInstance	*materialInstance;

		RenderMaterial*			getMaterial();

		virtual const Matrix4&		getTransform(void) const;

		const Matrix4				*boneMatrices;
		uint32						numBones;

        enum CullMode
        {
            CLOCKWISE = 0,
            COUNTER_CLOCKWISE,
		    NONE
        };

        CullMode					cullMode;
		bool						screenSpace;		//TODO: I am not sure if this is needed!

		RenderNode*				getParentNode() const { return m_parentNode; }
		
		bool					isAttached()const{return m_parentNode!=NULL;}

		virtual void			_notifyAttached(RenderNode* parent);

		virtual const String&	getName(void) const { return m_name; }

	public:
		RenderElement(void);

		RenderElement(const String& name);
		
		virtual ~RenderElement(void);
		
		bool isValid(void) const;
		bool isLocked(void) const;
	
	private:

		Render*		m_renderer;

		RenderNode*		m_parentNode;

		String			m_name;
};

_NAMESPACE_END

