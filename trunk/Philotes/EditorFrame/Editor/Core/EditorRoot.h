
/********************************************************************
	日期:		2012年2月21日
	文件名: 	EditorRoot.h
	创建者:		王延兴
	描述:		编辑器基础类	
*********************************************************************/

#pragma once

#include "XmlInterface.h"

class CXmlUtils;
class CUIEnumsDatabase;
class CClassFactory;
struct IEditorClassFactory;
class CEditTool;

//////////////////////////////////////////////////////////////////////////

class EditorRoot
{
public:

	EditorRoot();

	virtual ~EditorRoot();

public:

	static EditorRoot&	Get();

	static void			Destroy();

public:

	void					SetEditTool( CEditTool *tool,bool bStopCurrentTool=true );
	
	void					SetEditTool( const CString &sEditToolName,bool bStopCurrentTool=true );
	
	CEditTool*				GetEditTool();

	IEditorClassFactory*	GetClassFactory();

	XmlNodeRef				CreateXmlNode( const char *sNodeName="" );

	XmlNodeRef				LoadXmlFile( const char *sFilename );

	XmlNodeRef				LoadXmlFromString( const char *sXmlString );

	CUIEnumsDatabase*		GetUIEnumsDatabase();

protected:

	static EditorRoot*		s_RootInstance;

	CXmlUtils*				m_pXMLUtils;

	CUIEnumsDatabase*		m_pUIEnumsDatabase;

	TSmartPtr<CEditTool>	m_pEditTool;

	CEditTool*				m_pickTool;

	CClassFactory*			m_classFactory;

};