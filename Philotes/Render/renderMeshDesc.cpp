
#include <renderMeshDesc.h>

_NAMESPACE_BEGIN

RendererMeshDesc::RendererMeshDesc(void)
{
	primitives = RendererMesh::PRIMITIVE_POINTS;
	
	vertexBuffers    = 0;
	numVertexBuffers = 0;
	firstVertex      = 0;
	numVertices      = 0;
	
	indexBuffer = 0;
	firstIndex  = 0;
	numIndices  = 0;
	
	instanceBuffer = 0;
	firstInstance  = 0;
	numInstances   = 0;
}
		
bool RendererMeshDesc::isValid(void) const
{
	bool ok = true;
	// remove vertexBuffer check for sprites
	/* if(!vertexBuffers || !numVertexBuffers) ok = false; */
	if(numIndices     && !indexBuffer)      ok = false;
	if(numInstances   && !instanceBuffer)   ok = false;
	return ok;
}

_NAMESPACE_END