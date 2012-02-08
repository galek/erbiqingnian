
#ifndef RENDERER_MATERIAL_INSTANCE_H
#define RENDERER_MATERIAL_INSTANCE_H

#include <renderConfig.h>
#include <renderMaterial.h>

_NAMESPACE_BEGIN

class RendererMaterialInstance
{
	friend class RendererMaterial;
	public:
		RendererMaterialInstance(RendererMaterial &material);
		~RendererMaterialInstance(void);
		
		RendererMaterial &getMaterial(void) { return m_material; }
		
		const RendererMaterial::Variable *findVariable(const char *name, RendererMaterial::VariableType varType);
		
		void writeData(const RendererMaterial::Variable &var, const void *data);
	
		RendererMaterialInstance &operator=(const RendererMaterialInstance&);
		
	private:
		RendererMaterial &m_material;
		uint8             *m_data;
		
};

_NAMESPACE_END

#endif
