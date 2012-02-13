
#include <renderMaterial.h>

#include <renderMaterialDesc.h>
#include <renderMaterialInstance.h>

static uint32 getVariableTypeSize(RenderMaterial::VariableType type)
{
	uint32 size = 0;
	switch(type)
	{
		case RenderMaterial::VARIABLE_FLOAT:     size = sizeof(float)*1;            break;
		case RenderMaterial::VARIABLE_FLOAT2:    size = sizeof(float)*2;            break;
		case RenderMaterial::VARIABLE_FLOAT3:    size = sizeof(float)*3;            break;
		case RenderMaterial::VARIABLE_FLOAT4:    size = sizeof(float)*4;            break;
		case RenderMaterial::VARIABLE_FLOAT4x4:  size = sizeof(float)*4*4;          break;
		case RenderMaterial::VARIABLE_SAMPLER2D: size = sizeof(RenderTexture2D*); break;
	}
	ph_assert2(size>0, "Unable to compute Variable Type size.");
	return size;
}

RenderMaterial::Variable::Variable(const char *name, VariableType type, uint32 offset)
{
	size_t len = strlen(name)+1;
	m_name = new char[len];
	strcpy_s(m_name, len, name);
	m_type   = type;
	m_offset = offset;
}

RenderMaterial::Variable::~Variable(void)
{
	if(m_name) delete [] m_name;
}

const char *RenderMaterial::Variable::getName(void) const
{
	return m_name;
}

RenderMaterial::VariableType RenderMaterial::Variable::getType(void) const
{
	return m_type;
}

uint32 RenderMaterial::Variable::getDataOffset(void) const
{
	return m_offset;
}

uint32 RenderMaterial::Variable::getDataSize(void) const
{
	return getVariableTypeSize(m_type);
}

const char *RenderMaterial::getPassName(Pass pass)
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

RenderMaterial::RenderMaterial(const RenderMaterialDesc &desc) :
	m_type(desc.type),
	m_alphaTestFunc(desc.alphaTestFunc),
	m_srcBlendFunc(desc.srcBlendFunc),
	m_dstBlendFunc(desc.dstBlendFunc)
{
	m_alphaTestRef			= desc.alphaTestRef;
	m_blending				= desc.blending;
	m_variableBufferSize	= 0;
	m_cullMode				= CLOCKWISE;
}

RenderMaterial::~RenderMaterial(void)
{
	uint32 numVariables = (uint32)m_variables.size();
	for(uint32 i=0; i<numVariables; i++)
	{
		delete m_variables[i];
	}
}

void RenderMaterial::bind(RenderMaterial::Pass pass, RenderMaterialInstance *materialInstance, bool instanced) const
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
