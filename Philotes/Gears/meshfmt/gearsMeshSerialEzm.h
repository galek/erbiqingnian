
#pragma once

//ezm��ʽ��Ҳ����xml������ר��

_NAMESPACE_BEGIN

class MeshImporter;

MeshImporter*	createMeshImportEZM(void);

void			releaseMeshImportEZM(MeshImporter *iface);

_NAMESPACE_END