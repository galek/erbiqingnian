
#pragma once

#include "renderElement.h"

_NAMESPACE_BEGIN

class RenderDebugLine
{
public:

	RenderDebugLine(Render &renderer, GearAssetManager &assetmanager);
	virtual ~RenderDebugLine(void);

	void addLine(const Vector3 &p0, const Vector3 &p1, const Colour &color);
	void checkResizeLine(uint32 maxVerts);
	void queueForRenderLine(void);
	void clearLine(void);

private:
	void checkLock(void);
	void checkUnlock(void);
	void addVert(const Vector3 &p, const Colour &color);

private:
	RenderDebugLine &operator=(const RenderDebugLine&) { return *this; }

private:
	Render					&m_renderer;
	GearAssetManager		&m_assetmanager;

	GearMaterialAsset		*m_material;

	uint32					m_maxVerts;
	uint32					m_numVerts;
	RenderVertexBuffer		*m_vertexbuffer;
	RenderMesh				*m_mesh;
	RenderElement			m_meshContext;

	void					*m_lockedPositions;
	uint32		            m_positionStride;

	void					*m_lockedColors;
	uint32			        m_colorStride;
	RenderTransform			*m_transfrom;
};

class RenderLineElement : public RenderElement
{
public:

	RenderLineElement();

	RenderLineElement(const String& name);

	virtual ~RenderLineElement();

public:

	void addLine(const Vector3 &p0, const Vector3 &p1, const Colour &color);
	
	void checkResizeLine(uint32 maxVerts);

	void clearLine(void);

private:

	void init();

	void checkLock(void);

	void checkUnlock(void);

	void addVert(const Vector3 &p, const Colour &color);

protected:

	GearMaterialAsset		*m_materialAsset;

	uint32					m_maxVerts;
	uint32					m_numVerts;
	RenderVertexBuffer		*m_vertexbuffer;

	void					*m_lockedPositions;
	uint32		            m_positionStride;

	void					*m_lockedColors;
	uint32			        m_colorStride;
};

_NAMESPACE_END