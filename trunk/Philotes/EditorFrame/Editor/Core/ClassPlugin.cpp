
#include "ClassPlugin.h"

_NAMESPACE_BEGIN

CClassFactory* CClassFactory::m_instance = 0;

CAutoRegisterClassHelper* CAutoRegisterClassHelper::m_pFirst = 0;
CAutoRegisterClassHelper* CAutoRegisterClassHelper::m_pLast = 0;

CClassFactory::CClassFactory()
{
	m_classes.reserve(100);
	RegisterAutoTypes();
}

CClassFactory::~CClassFactory()
{
	for (int i = 0; i < m_classes.size(); i++)
	{
		m_classes[i]->Release();
	}
}

void CClassFactory::RegisterAutoTypes()
{
	CAutoRegisterClassHelper *pClass = CAutoRegisterClassHelper::m_pFirst;
	while (pClass)
	{
		RegisterClass( pClass->m_pClassDesc );
		pClass = pClass->m_pNext;
	}
}


CClassFactory* CClassFactory::Instance()
{
	if (!m_instance)
	{
		m_instance = new CClassFactory;
	}
	return m_instance;
}

void CClassFactory::RegisterClass( IClassDesc *cls )
{
	assert( cls );
	m_classes.push_back( cls );
	m_guidToClass[cls->ClassID()] = cls;
	m_nameToClass[cls->ClassName()] = cls;
}

IClassDesc* CClassFactory::FindClass( const char *className ) const
{
	IClassDesc *cls = stl::FindInMap( m_nameToClass,className,(IClassDesc*)0 );
	if (cls)
		return cls;

	const char *subClassName = strstr(className, "::");
	if (!subClassName)
		return NULL;

	CString name;
	name.Append(className, subClassName - className);
	return stl::FindInMap( m_nameToClass,name,(IClassDesc*)0 );
}

IClassDesc* CClassFactory::FindClass( const GUID& clsid ) const
{
	IClassDesc *cls = stl::FindInMap( m_guidToClass,clsid,(IClassDesc*)0 );
	return cls;
}

bool ClassDescNameComparer( IClassDesc* arg1, IClassDesc* arg2 )
{
	return ( stricmp( arg1->ClassName(), arg2->ClassName() ) < 0 );
}

void CClassFactory::GetClassesBySystemID( ESystemClassID systemCLSID,std::vector<IClassDesc*> &classes )
{
	classes.clear();
	for (int i = 0; i < m_classes.size(); i++)
	{
		if (m_classes[i]->SystemClassID() == systemCLSID)
		{
			classes.push_back( m_classes[i] );
		}
	}
	std::sort( classes.begin(), classes.end(), ClassDescNameComparer );
}

void CClassFactory::GetClassesByCategory( const char* category,std::vector<IClassDesc*> &classes )
{
	classes.clear();
	for (int i = 0; i < m_classes.size(); i++)
	{
		if (stricmp(category,m_classes[i]->Category()) == 0)
		{
			classes.push_back( m_classes[i] );
		}
	}
	std::sort( classes.begin(), classes.end(), ClassDescNameComparer );
}

_NAMESPACE_END