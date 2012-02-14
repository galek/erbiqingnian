
#pragma once

//ezm格式，也就是xml，测试专用

_NAMESPACE_BEGIN

class MeshImporter;

MeshImporter*	createMeshImportEZM(void);

void			releaseMeshImportEZM(MeshImporter *iface);

_NAMESPACE_END