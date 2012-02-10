
#include <renderInstanceBufferDesc.h>

_NAMESPACE_BEGIN

RenderInstanceBufferDesc::RenderInstanceBufferDesc(void)
{
	hint = RenderInstanceBuffer::HINT_STATIC;
	for(uint32 i=0; i<RenderInstanceBuffer::NUM_SEMANTICS; i++)
	{
		semanticFormats[i] = RenderInstanceBuffer::NUM_FORMATS;
	}
	maxInstances = 0;
}

bool RenderInstanceBufferDesc::isValid(void) const
{
	bool ok = true;
	if(!maxInstances) ok = false;

	bool bValidTurbulence = ok;
	if(semanticFormats[RenderInstanceBuffer::SEMANTIC_POSITION] != RenderInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RenderInstanceBuffer::SEMANTIC_NORMALX]  != RenderInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RenderInstanceBuffer::SEMANTIC_NORMALY]  != RenderInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RenderInstanceBuffer::SEMANTIC_NORMALZ]  != RenderInstanceBuffer::FORMAT_FLOAT3) ok = false;

	if(semanticFormats[RenderInstanceBuffer::SEMANTIC_POSITION] != RenderInstanceBuffer::FORMAT_FLOAT3) bValidTurbulence = false;
	if(semanticFormats[RenderInstanceBuffer::SEMANTIC_VELOCITY_LIFE] != RenderInstanceBuffer::FORMAT_FLOAT4) bValidTurbulence = false;
	
	ph_assert(bValidTurbulence || ok);
	return bValidTurbulence || ok;
}

_NAMESPACE_END