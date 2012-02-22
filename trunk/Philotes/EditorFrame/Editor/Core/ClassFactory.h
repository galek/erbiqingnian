
/********************************************************************
	日期:		2012年2月22日
	文件名: 	ClassFactory.h
	创建者:		王延兴
	描述:		类厂和类描述结构基类	
*********************************************************************/

#pragma once

enum ESystemClassID
{
	ESYSTEM_CLASS_OBJECT						= 0x0001,
	ESYSTEM_CLASS_EDITTOOL						= 0x0002,
	
	ESYSTEM_CLASS_PREFERENCE_PAGE				= 0x0020,
	ESYSTEM_CLASS_VIEWPANE						= 0x0021,
	ESYSTEM_CLASS_SCM_PROVIDER					= 0x0022,

	ESYSTEM_CLASS_CONSOLE_CONNECTIVITY			= 0x0023,
	ESYSTEM_CLASS_ASSET_DISPLAY					= 0x0024,

	ESYSTEM_CLASS_FRAMEWND_EXTENSION_PANE		= 0x0030,

	ESYSTEM_CLASS_TRACKVIEW_KEYUI				= 0x0040,

	ESYSTEM_CLASS_USER							= 0x1000,
};

//////////////////////////////////////////////////////////////////////////

struct IClassDesc : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface( const IID &riid, void **ppvObj ) { return E_NOINTERFACE ; }

	virtual ULONG STDMETHODCALLTYPE		AddRef() { return 0; }

	virtual ULONG STDMETHODCALLTYPE		Release() { return 0; }

	virtual ESystemClassID				SystemClassID() = 0;

	virtual REFGUID						ClassID() = 0;

	virtual const char*					ClassName() = 0;

	virtual const char*					Category() = 0;

	virtual CRuntimeClass*				GetRuntimeClass() = 0;
};


struct IEditorClassFactory
{
public:
	
	virtual void RegisterClass( IClassDesc *cls ) = 0;

	virtual IClassDesc* FindClass( const char *className ) const = 0;

	virtual IClassDesc* FindClass( const GUID& clsid ) const = 0;

	virtual void GetClassesBySystemID( ESystemClassID systemCLSID,std::vector<IClassDesc*> &classes ) = 0;

	virtual void GetClassesByCategory( const char* category,std::vector<IClassDesc*> &classes ) = 0;
};