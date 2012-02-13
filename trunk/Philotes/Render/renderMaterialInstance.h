
#ifndef RENDERER_MATERIAL_INSTANCE_H
#define RENDERER_MATERIAL_INSTANCE_H

#include <renderConfig.h>
#include <renderMaterial.h>

_NAMESPACE_BEGIN

class RenderMaterialInstance
{
	friend class RenderMaterial;
	public:

		RenderMaterialInstance(RenderMaterial &material);
		
		virtual ~RenderMaterialInstance(void);
		
		RenderMaterial &getMaterial(void) { return m_material; }
		
		const RenderMaterial::Variable *findVariable(const char *name, RenderMaterial::VariableType varType);
		
		void writeData(const RenderMaterial::Variable &var, const void *data);
	
		RenderMaterialInstance &operator=(const RenderMaterialInstance&);
		
	private:

		RenderMaterial&		m_material;

		uint8*				m_data;
		
};

_NAMESPACE_END

#endif
