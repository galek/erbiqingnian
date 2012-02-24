
/********************************************************************
	日期:		2012年2月22日
	文件名: 	ClassPlugin.h
	创建者:		王延兴
	描述:		类厂等	
*********************************************************************/

#pragma once

#include "ClassFactory.h"

_NAMESPACE_BEGIN

class CRefCountClassDesc : public IClassDesc
{
public:
	HRESULT STDMETHODCALLTYPE QueryInterface( const IID &riid, void **ppvObj ) 
	{
		return E_NOINTERFACE ; 
	}
	
	ULONG STDMETHODCALLTYPE AddRef()
	{
		m_nRefCount++;
		return m_nRefCount;
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		int refs = m_nRefCount;
		if (--m_nRefCount <= 0)
			delete this;
		return refs;
	}
private:
	int m_nRefCount;
};

//////////////////////////////////////////////////////////////////////////

class CClassFactory : public IEditorClassFactory
{
public:
	CClassFactory();

	~CClassFactory();

	static CClassFactory* Instance();

	void			RegisterClass( IClassDesc *cls );

	IClassDesc*		FindClass( const char *className ) const;
	
	IClassDesc*		FindClass( const GUID& clsid ) const;

	void			GetClassesBySystemID( ESystemClassID systemCLSID,std::vector<IClassDesc*> &classes );

	void			GetClassesByCategory( const char* category,std::vector<IClassDesc*> &classes );

private:
	void			RegisterAutoTypes();

	std::vector<IClassDesc*>	m_classes;

	typedef std::map<CString,IClassDesc*> NameMap;
	NameMap						m_nameToClass;

	typedef std::map<GUID,IClassDesc*,guid_less_predicate> GuidMap;
	GuidMap						m_guidToClass;

	static CClassFactory*		m_instance;
};

//////////////////////////////////////////////////////////////////////////

class CAutoRegisterClassHelper
{
public:
	CAutoRegisterClassHelper( IClassDesc *pClassDesc )
	{
		m_pClassDesc = pClassDesc;
		m_pNext = 0;
		if (!m_pLast)
		{
			m_pFirst = this;
		}
		else
			m_pLast->m_pNext = this;
		m_pLast = this;
	}

	IClassDesc *m_pClassDesc;
	CAutoRegisterClassHelper* m_pNext;
	static CAutoRegisterClassHelper *m_pFirst;
	static CAutoRegisterClassHelper *m_pLast;
};

//////////////////////////////////////////////////////////////////////////
// 使用这个宏可自动注册新的类描述
#define REGISTER_CLASS_DESC( ClassDesc ) \
	ClassDesc g_AutoReg##ClassDesc; \
	CAutoRegisterClassHelper g_AutoRegHelper##ClassDesc( &(g_AutoReg##ClassDesc) );

_NAMESPACE_END