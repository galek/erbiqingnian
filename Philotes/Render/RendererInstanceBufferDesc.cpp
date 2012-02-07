
#include <RendererInstanceBufferDesc.h>

RendererInstanceBufferDesc::RendererInstanceBufferDesc(void)
{
	hint = RendererInstanceBuffer::HINT_STATIC;
	for(uint32 i=0; i<RendererInstanceBuffer::NUM_SEMANTICS; i++)
	{
		semanticFormats[i] = RendererInstanceBuffer::NUM_FORMATS;
	}
	maxInstances = 0;
}

bool RendererInstanceBufferDesc::isValid(void) const
{
	bool ok = true;
	if(!maxInstances) ok = false;

	bool bValidTurbulence = ok;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_POSITION] != RendererInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALX]  != RendererInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALY]  != RendererInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALZ]  != RendererInstanceBuffer::FORMAT_FLOAT3) ok = false;

	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_POSITION] != RendererInstanceBuffer::FORMAT_FLOAT3) bValidTurbulence = false;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE] != RendererInstanceBuffer::FORMAT_FLOAT4) bValidTurbulence = false;
	
	ph_assert(bValidTurbulence || ok);
	return bValidTurbulence || ok;
}
