
/********************************************************************
	日期:		2012年2月21日
	文件名: 	EditorRoot.h
	创建者:		王延兴
	描述:		编辑器基础类	
*********************************************************************/

#pragma once

#include "XmlInterface.h"

class CEditorDoc;
class CXmlUtils;
class CUIEnumsDatabase;
class CClassFactory;
struct IEditorClassFactory;
class CEditTool;
class CRollupCtrl;

//////////////////////////////////////////////////////////////////////////
// 收缩控件类型

enum ERollupControlIds
{
	ROLLUP_OBJECTS = 0,
	ROLLUP_TERRAIN,
	ROLLUP_DISPLAY,
	ROLLUP_LAYERS,
	ROLLUP_MODELLING,
};

//////////////////////////////////////////////////////////////////////////

class EditorRoot
{
public:

	EditorRoot();

	virtual ~EditorRoot();

public:

	static EditorRoot&		Get();

	static void				Destroy();

public:

	void					SetEditTool( CEditTool *tool,bool bStopCurrentTool=true );
	
	void					SetEditTool( const CString &sEditToolName,bool bStopCurrentTool=true );
	
	CEditTool*				GetEditTool();

	IEditorClassFactory*	GetClassFactory();

	XmlNodeRef				CreateXmlNode( const char *sNodeName="" );

	XmlNodeRef				LoadXmlFile( const char *sFilename );

	XmlNodeRef				LoadXmlFromString( const char *sXmlString );

	CUIEnumsDatabase*		GetUIEnumsDatabase();

	CEditorDoc*				GetDocument();

	void					SetDocument(CEditorDoc* doc);

	virtual int				SelectRollUpBar( int rollupBarId );

	virtual int				AddRollUpPage( int rollbarId,LPCTSTR pszCaption, CDialog *pwndTemplate = NULL,
									bool bAutoDestroyTpl = true, int iIndex = -1,bool bAutoExpand=true );

	virtual void			RemoveRollUpPage(int rollbarId,int iIndex);

	virtual void			ExpandRollUpPage(int rollbarId,int iIndex, BOOL bExpand = true);

	virtual void			EnableRollUpPage(int rollbarId,int iIndex, BOOL bEnable = true);

	CRollupCtrl*			GetRollUpControl( int rollupId );;

protected:

	static EditorRoot*		s_RootInstance;

	CXmlUtils*				m_pXMLUtils;

	CUIEnumsDatabase*		m_pUIEnumsDatabase;

	TSmartPtr<CEditTool>	m_pEditTool;

	CEditTool*				m_pickTool;

	CClassFactory*			m_classFactory;

	CEditorDoc*				m_document;
};