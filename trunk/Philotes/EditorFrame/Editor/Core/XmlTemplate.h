
/********************************************************************
	日期:		2012年2月23日
	文件名: 	XmlTemplate.h
	创建者:		王延兴
	描述:		XML属性模板管理	
*********************************************************************/

#pragma once

#include "XmlInterface.h"

class CXmlTemplate
{
public:
	static void 	GetValues( XmlNodeRef &templateNode,const XmlNodeRef &fromNode );

	static void 	SetValues( const XmlNodeRef &templateNode,XmlNodeRef &toNode );
	static bool 	SetValues( const XmlNodeRef &templateNode,XmlNodeRef &toNode,const XmlNodeRef &modifiedNode );

	static void 	AddParam( XmlNodeRef &templ,const char *paramName,bool value );
	static void 	AddParam( XmlNodeRef &templ,const char *paramName,int value,int min=0,int max=10000 );
	static void 	AddParam( XmlNodeRef &templ,const char *paramName,float value,float min=-10000,float max=10000 );
	static void 	AddParam( XmlNodeRef &templ,const char *paramName,const char *sValue );
};

//////////////////////////////////////////////////////////////////////////

class CXmlTemplateRegistry
{
public:
	CXmlTemplateRegistry();

	void			LoadTemplates( const CString &path );
	void			AddTemplate( const CString &name,XmlNodeRef &tmpl );

	XmlNodeRef		FindTemplate( const CString &name );

private:
	std::map<CString,XmlNodeRef> m_templates;
};
