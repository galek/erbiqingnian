
/********************************************************************
	����:		2012��2��21��
	�ļ���: 	EditorRoot.h
	������:		������
	����:		�༭��������	
*********************************************************************/

#pragma once

#include "XmlInterface.h"
#include "XmlTemplate.h"

class CEditorDoc;
class CXmlUtils;
class CUIEnumsDatabase;
class CClassFactory;
struct IEditorClassFactory;
class CEditTool;
class CRollupCtrl;
class CViewManager;
class CViewport;

//////////////////////////////////////////////////////////////////////////
// �����ؼ�����

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

	void					SetMasterFolder();

	const CString&			GetMasterFolder();

	//////////////////////////////////////////////////////////////////////////

	int						SelectRollUpBar( int rollupBarId );

	int						AddRollUpPage( int rollbarId,LPCTSTR pszCaption, CDialog *pwndTemplate = NULL,
								bool bAutoDestroyTpl = true, int iIndex = -1,bool bAutoExpand=true );

	void					RemoveRollUpPage(int rollbarId,int iIndex);

	void					ExpandRollUpPage(int rollbarId,int iIndex, BOOL bExpand = true);

	void					EnableRollUpPage(int rollbarId,int iIndex, BOOL bEnable = true);

	CRollupCtrl*			GetRollUpControl( int rollupId );

	//////////////////////////////////////////////////////////////////////////

	virtual XmlNodeRef		FindTemplate( const CString &templateName );

	virtual void			AddTemplate( const CString &templateName,XmlNodeRef &tmpl );

	virtual void			ReloadTemplates();

	//////////////////////////////////////////////////////////////////////////

	CViewManager*			GetViewManager();

	CViewport*				GetActiveView();

	void					UpdateViews( int flags );

protected:

	static EditorRoot*		s_RootInstance;

	CXmlUtils*				m_pXMLUtils;

	CUIEnumsDatabase*		m_pUIEnumsDatabase;

	TSmartPtr<CEditTool>	m_pEditTool;

	CEditTool*				m_pickTool;

	CClassFactory*			m_classFactory;

	CEditorDoc*				m_document;

	CXmlTemplateRegistry	m_templateRegistry;

	CString					m_masterFolder;

	CViewManager*			m_viewMan;
};