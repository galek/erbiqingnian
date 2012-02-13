
#include <renderMaterialInstance.h>
#include <renderMaterial.h>

_NAMESPACE_BEGIN

RenderMaterialInstance::RenderMaterialInstance(RenderMaterial &material) :
	m_material(material)
{
	m_data = 0;
	uint32 dataSize = m_material.getMaterialInstanceDataSize();
	if(dataSize > 0)
	{
		m_data = new uint8[dataSize];
		memset(m_data, 0, dataSize);
	}
}

RenderMaterialInstance::~RenderMaterialInstance(void)
{
	if(m_data) delete [] m_data;
}

const RenderMaterial::Variable *RenderMaterialInstance::findVariable(const char *name, RenderMaterial::VariableType varType)
{
	RenderMaterial::Variable *var = 0;
	uint32 numVariables = (uint32)m_material.m_variables.Size();
	for(uint32 i=0; i<numVariables; i++)
	{
		RenderMaterial::Variable &v = *m_material.m_variables[i];
		if(!strcmp(v.getName(), name))
		{
			var = &v;
			break;
		}
	}
	if(var && var->getType() != varType)
	{
		var = 0;
	}
	return var;
}

void RenderMaterialInstance::writeData(const RenderMaterial::Variable &var, const void *data)
{
	if(m_data && data)
	{
		memcpy(m_data+var.getDataOffset(), data, var.getDataSize());
	}
}

RenderMaterialInstance &RenderMaterialInstance::operator=(const RenderMaterialInstance &b)
{
	ph_assert(&m_material == &b.m_material);
	if(&m_material == &b.m_material)
	{
		memcpy(m_data, b.m_data, m_material.getMaterialInstanceDataSize());
	}
	return *this;
}

_NAMESPACE_END