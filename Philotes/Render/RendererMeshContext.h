
#ifndef RENDERER_MESH_CONTEXT_H
#define RENDERER_MESH_CONTEXT_H

#include <RendererConfig.h>

class Renderer;
class RendererMesh;
class RendererMaterial;
class RendererMaterialInstance;

class RendererMeshContext
{
	friend class Renderer;
	public:
		const RendererMesh			*mesh;
		RendererMaterial			*material;
		RendererMaterialInstance	*materialInstance;
		const Matrix4				*transform; // TODO: use a float4x3 instead of a 3x4. Which is basically a transposed 3x4 for better GPU packing.

		// TODO: this is kind of hacky, would prefer a more generalized
		//       solution via RendererMatrialInstance.
		const Matrix4				*boneMatrices; // TODO: use a float4x3 instead of a 3x4. Which is basically a transposed 3x4 for better GPU packing.
		uint32						numBones;

        enum Enum
        {
            CLOCKWISE = 0,
            COUNTER_CLOCKWISE,
		    NONE
        };

        Enum					cullMode;
		bool					screenSpace;		//TODO: I am not sure if this is needed!

	public:
		RendererMeshContext(void);
		~RendererMeshContext(void);
		
		bool isValid(void) const;
		bool isLocked(void) const;
	
	private:
		Renderer *m_renderer;
};

#endif
