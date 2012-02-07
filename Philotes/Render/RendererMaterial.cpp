
#include <RendererMaterial.h>

#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>

static uint32 getVariableTypeSize(RendererMaterial::VariableType type)
{
	uint32 size = 0;
	switch(type)
	{
		case RendererMaterial::VARIABLE_FLOAT:     size = sizeof(float)*1;            break;
		case RendererMaterial::VARIABLE_FLOAT2:    size = sizeof(float)*2;            break;
		case RendererMaterial::VARIABLE_FLOAT3:    size = sizeof(float)*3;            break;
		case RendererMaterial::VARIABLE_FLOAT4:    size = sizeof(float)*4;            break;
		case RendererMaterial::VARIABLE_FLOAT4x4:  size = sizeof(float)*4*4;          break;
		case RendererMaterial::VARIABLE_SAMPLER2D: size = sizeof(RendererTexture2D*); break;
	}
	ph_assert2(size>0, "Unable to compute Variable Type size.");
	return size;
}

RendererMaterial::Variable::Variable(const char *name, VariableType type, uint32 offset)
{
	size_t len = strlen(name)+1;
	m_name = new char[len];
	strcpy_s(m_name, len, name);
	m_type   = type;
	m_offset = offset;
}

RendererMaterial::Variable::~Variable(void)
{
	if(m_name) delete [] m_name;
}

const char *RendererMaterial::Variable::getName(void) const
{
	return m_name;
}

RendererMaterial::VariableType RendererMaterial::Variable::getType(void) const
{
	return m_type;
}

uint32 RendererMaterial::Variable::getDataOffset(void) const
{
	return m_offset;
}

uint32 RendererMaterial::Variable::getDataSize(void) const
{
	return getVariableTypeSize(m_type);
}

const char *RendererMaterial::getPassName(Pass pass)
{
	const char *passName = 0;
	switch(pass)
	{
		case PASS_UNLIT:             passName="PASS_UNLIT";             break;
		
		case PASS_AMBIENT_LIGHT:     passName="PASS_AMBIENT_LIGHT";     break;
		case PASS_POINT_LIGHT:       passName="PASS_POINT_LIGHT";       break;
		case PASS_DIRECTIONAL_LIGHT: passName="PASS_DIRECTIONAL_LIGHT"; break;
		case PASS_SPOT_LIGHT:        passName="PASS_SPOT_LIGHT";        break;
		
		case PASS_NORMALS:           passName="PASS_NORMALS";           break;
		case PASS_DEPTH:             passName="PASS_DEPTH";             break;

		// LRR: The deferred pass causes compiles with the ARB_draw_buffers profile option, creating 
		// multiple color draw buffers.  This doesn't work in OGL on ancient Intel parts.
		//case PASS_DEFERRED:          passName="PASS_DEFERRED";          break;
	}
	ph_assert2(passName, "Unable to obtain name for the given Material Pass.");
	return passName;
}

RendererMaterial::RendererMaterial(const RendererMaterialDesc &desc) :
	m_type(desc.type),
	m_alphaTestFunc(desc.alphaTestFunc),
	m_srcBlendFunc(desc.srcBlendFunc),
	m_dstBlendFunc(desc.dstBlendFunc)
{
	m_alphaTestRef       = desc.alphaTestRef;
	m_blending           = desc.blending;
	m_variableBufferSize = 0;
}

RendererMaterial::~RendererMaterial(void)
{
	uint32 numVariables = (uint32)m_variables.size();
	for(uint32 i=0; i<numVariables; i++)
	{
		delete m_variables[i];
	}
}

void RendererMaterial::bind(RendererMaterial::Pass pass, RendererMaterialInstance *materialInstance, bool instanced) const
{
	if(materialInstance)
	{
		uint32 numVariables = (uint32)m_variables.size();
		for(uint32 i=0; i<numVariables; i++)
		{
			const Variable &variable = *m_variables[i];
			bindVariable(pass, variable, materialInstance->m_data+variable.getDataOffset());
		}
	}
}
