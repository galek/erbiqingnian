
#pragma once

#include "renderElement.h"

_NAMESPACE_BEGIN

class RenderLineElement : public RenderElement
{
public:

	RenderLineElement();

	RenderLineElement(const String& name);

	virtual ~RenderLineElement();

public:

	void addLine(const Vector3 &p0,const Vector3 &p1,const Colour& color);
	
	void checkResizeLine(uint32 maxVerts);

	void clearLine(void);

	bool preQueuedToRender();

private:

	void init();

	void checkLock(void);

	void checkUnlock(void);

	void addPoint(const Vector3 &p, const Colour &color);

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