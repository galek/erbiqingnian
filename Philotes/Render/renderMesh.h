
#pragma once

_NAMESPACE_BEGIN

class RenderMesh
{
public:

	RenderMesh();

	RenderMesh(const String& name);

	virtual ~RenderMesh();

	typedef Array<RenderSubMesh*> SubMeshList;

public:

	virtual RenderSubMesh*	createSubMesh();

protected:

	SubMeshList	m_subMeshes;

	String		m_name;
};


_NAMESPACE_END