
#pragma once

#include "renderMeshContext.h"

_NAMESPACE_BEGIN

class RendererDebugLine
{
public:

	RendererDebugLine(Renderer &renderer, GearAssetManager &assetmanager);
	virtual ~RendererDebugLine(void);

	void addLine(const Vector3 &p0, const Vector3 &p1, const RendererColor &color);
	void checkResizeLine(uint32 maxVerts);
	void queueForRenderLine(void);
	void clearLine(void);

private:
	void checkLock(void);
	void checkUnlock(void);
	void addVert(const Vector3 &p, const RendererColor &color);

private:
	RendererDebugLine &operator=(const RendererDebugLine&) { return *this; }

private:
	Renderer				&m_renderer;
	GearAssetManager		&m_assetmanager;

	GearMaterialAsset		*m_material;

	uint32					m_maxVerts;
	uint32					m_numVerts;
	RendererVertexBuffer	*m_vertexbuffer;
	RendererMesh			*m_mesh;
	RendererMeshContext		m_meshContext;

	void					*m_lockedPositions;
	uint32		            m_positionStride;

	void					*m_lockedColors;
	uint32			        m_colorStride;
};

_NAMESPACE_END