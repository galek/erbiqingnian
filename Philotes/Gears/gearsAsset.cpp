
#include "gearsAsset.h"

_NAMESPACE_BEGIN

GearAsset::GearAsset(Type type, const char *path) :
m_type(type)
{
	size_t len = strlen(path)+1;
	m_path = new char[len];
	strcpy_s(m_path, len, path);
	m_numUsers = 0;
}

GearAsset::~GearAsset(void)
{
	if(m_path) delete [] m_path;
}


_NAMESPACE_END