
#include "gearsAsset.h"

_NAMESPACE_BEGIN

GearAsset::GearAsset(Type type, const String& path) :
m_type(type)
{
	m_path = path;
	m_numUsers = 0;
}

GearAsset::~GearAsset(void)
{
}


_NAMESPACE_END