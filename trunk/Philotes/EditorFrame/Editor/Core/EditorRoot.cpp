
#include "EditorRoot.h"
#include "Xml.h"
#include "XmlUtils.h"
#include "UIEnumsDatabase.h"
#include "EditorTool.h"
#include "ClassPlugin.h"
#include "RollupCtrl.h"
#include "ViewManager.h"
#include "Viewport.h"
#include "ViewPane.h"
#include "../MainFrm.h"
#include "../EditorDoc.h"

_NAMESPACE_BEGIN

CEditorRoot* CEditorRoot::s_RootInstance = 0;

static CMainFrame* GetMainFrame()
{
	CWnd *pWnd = AfxGetMainWnd();
	if (!pWnd)
		return 0;
	if (!pWnd->IsKindOf(RUNTIME_CLASS(CMainFrame)))
		return 0;
	return (CMainFrame*)AfxGetMainWnd();
}

//////////////////////////////////////////////////////////////////////////

CEditorRoot::CEditorRoot()
{
	s_RootInstance		= this;

	m_pXMLUtils			= new CXmlUtils();
	m_pUIEnumsDatabase	= new CUIEnumsDatabase;
	m_classFactory		= CClassFactory::Instance();
	m_viewMan			= new CViewManager();

	SetMasterFolder();
}

CEditorRoot::~CEditorRoot()
{
	SAFE_DELETE(m_pXMLUtils);
	SAFE_DELETE(m_pUIEnumsDatabase);
	SAFE_DELETE(m_classFactory);
	SAFE_DELETE(m_viewMan);
}

CEditorRoot& CEditorRoot::Get()
{
	return *s_RootInstance;
}

void CEditorRoot::Destroy()
{
	if (s_RootInstance)
	{
		delete s_RootInstance;
		s_RootInstance = NULL;
	}
}

XmlNodeRef CEditorRoot::CreateXmlNode( const char *sNodeName/*="" */ )
{
	return new CXmlNode( sNodeName );
}

XmlNodeRef CEditorRoot::LoadXmlFile( const char *sFilename )
{
	return m_pXMLUtils->LoadXmlFile(sFilename);
}

XmlNodeRef CEditorRoot::LoadXmlFromString( const char *sXmlString )
{
	return m_pXMLUtils->LoadXmlFromString(sXmlString);
}

CUIEnumsDatabase* CEditorRoot::GetUIEnumsDatabase()
{
	return m_pUIEnumsDatabase;
}

void CEditorRoot::SetEditTool( CEditTool *tool,bool bStopCurrentTool/*=true */ )
{
// 	if (tool == NULL)
// 	{
// 		if (m_pEditTool != 0 && m_pEditTool->IsKindOf(RUNTIME_CLASS(CObjectMode)))
// 		{
// 			return;
// 		}
// 		else
// 		{
// 			tool = new CObjectMode;
// 		}
// 	}

	if (tool)
	{
		if (!tool->Activate(m_pEditTool))
		{
			return;
		}
	}

	if (bStopCurrentTool)
	{
		if (m_pEditTool != tool && m_pEditTool)
		{
			m_pEditTool->EndEditParams();
			//SetStatusText( "Ready" );
		}
	}

	m_pEditTool = tool;

	if (m_pEditTool)
	{
		m_pEditTool->BeginEditParams( 0 );
	}

	if (tool != m_pickTool)
	{
		m_pickTool = NULL;
	}

	// ÊÂ¼þ
	//Notify( eNotify_OnEditToolChange );
}

void CEditorRoot::SetEditTool( const CString &sEditToolName,bool bStopCurrentTool/*=true */ )
{
	CEditTool *pTool = GetEditTool();
	if (pTool && pTool->GetClassDesc())
	{
		if (stricmp(pTool->GetClassDesc()->ClassName(),sEditToolName) == 0)
			return;
	}

	IClassDesc *pClass = this->GetClassFactory()->FindClass( sEditToolName );
	if (!pClass)
	{
		//Warning( "Editor Tool %s not registered.",(const char*)sEditToolName );
		return;
	}
	if (pClass->SystemClassID() != ESYSTEM_CLASS_EDITTOOL)
	{
		//Warning( "Class name %s is not a valid Edit Tool class.",(const char*)sEditToolName );
		return;
	}
	CRuntimeClass *pRtClass = pClass->GetRuntimeClass();
	if (pRtClass && pRtClass->IsDerivedFrom(RUNTIME_CLASS(CEditTool)))
	{
		CEditTool *pEditTool = (CEditTool*)pRtClass->CreateObject();
		this->SetEditTool(pEditTool);
		return;
	}
	else
	{
		//Warning( "Class name %s is not a valid Edit Tool class.",(const char*)sEditToolName );
		return;
	}
}

CEditTool* CEditorRoot::GetEditTool()
{
	return m_pEditTool;
}

IEditorClassFactory* CEditorRoot::GetClassFactory()
{
	return m_classFactory;
}

CEditorDoc* CEditorRoot::GetDocument()
{
	return m_document;
}

void CEditorRoot::SetDocument( CEditorDoc* doc )
{
	m_document = doc;
}

int CEditorRoot::SelectRollUpBar( int rollupBarId )
{
	if (GetMainFrame())
		return GetMainFrame()->SelectRollUpBar( rollupBarId );
	else
		return 0;
}

int CEditorRoot::AddRollUpPage( int rollbarId,LPCTSTR pszCaption, CDialog *pwndTemplate /*= NULL*/, 
							  bool bAutoDestroyTpl /*= true*/, int iIndex /*= -1*/,bool bAutoExpand/*=true */ )
{
	if (!GetMainFrame())
		return 0;

	if (pwndTemplate && !pwndTemplate->m_hWnd)
	{
		return 0;
	}

	if (!GetRollUpControl(rollbarId))
	{
		return 0;
	}
	HWND hFocusWnd = GetFocus();
	int id = GetRollUpControl(rollbarId)->InsertPage(pszCaption, pwndTemplate, bAutoDestroyTpl, iIndex,bAutoExpand);

	if (hFocusWnd && GetFocus() != hFocusWnd)
	{
		SetFocus(hFocusWnd);
	}
	return id;
}

void CEditorRoot::RemoveRollUpPage( int rollbarId,int iIndex )
{
	if (!GetRollUpControl(rollbarId))
	{
		return;
	}

	GetRollUpControl(rollbarId)->RemovePage(iIndex);
}

void CEditorRoot::ExpandRollUpPage( int rollbarId,int iIndex, BOOL bExpand /*= true*/ )
{
	if (!GetRollUpControl(rollbarId))
	{
		return;
	}

	HWND hFocusWnd = GetFocus();

	GetRollUpControl(rollbarId)->ExpandPage(iIndex, bExpand);

	if (hFocusWnd && GetFocus() != hFocusWnd)
	{
		SetFocus(hFocusWnd);
	}
}

void CEditorRoot::EnableRollUpPage( int rollbarId,int iIndex, BOOL bEnable /*= true*/ )
{
	if (!GetRollUpControl(rollbarId))
		return;

	HWND hFocusWnd = GetFocus();

	GetRollUpControl(rollbarId)->EnablePage(iIndex, bEnable);

	if (hFocusWnd && GetFocus() != hFocusWnd)
	{
		SetFocus(hFocusWnd);
	}
}

CRollupCtrl* CEditorRoot::GetRollUpControl( int rollupId )
{
	if (!GetMainFrame())
	{
		return NULL;
	}
	return GetMainFrame()->GetRollUpControl(rollupId);
}

XmlNodeRef CEditorRoot::FindTemplate( const CString &templateName )
{
	return m_templateRegistry.FindTemplate( templateName );
}

void CEditorRoot::AddTemplate( const CString &templateName,XmlNodeRef &tmpl )
{
	m_templateRegistry.AddTemplate( templateName,tmpl );
}

void CEditorRoot::ReloadTemplates()
{
	m_templateRegistry.LoadTemplates( Path::MakeFullPath("Templates") );
}

void CEditorRoot::SetMasterFolder()
{
	CHAR sFolder[_MAX_PATH];

	GetModuleFileNameA( GetModuleHandle(NULL), sFolder, sizeof(sFolder));
	PathRemoveFileSpecA(sFolder);

	CHAR *lpPath = StrStrIA(sFolder,"\\Bin32");
	if (lpPath)
		*lpPath = 0;
	lpPath = StrStrIA(sFolder,"\\Bin64");
	if (lpPath)
		*lpPath = 0;

	m_masterFolder = sFolder;
	if (!m_masterFolder.IsEmpty())
	{
		if (m_masterFolder[m_masterFolder.GetLength()-1] != '\\')
			m_masterFolder += '\\';
	}

	SetCurrentDirectoryA( sFolder );
}

const CString& CEditorRoot::GetMasterFolder()
{
	return m_masterFolder;
}

CViewManager* CEditorRoot::GetViewManager()
{
	return m_viewMan;
}

CViewport* CEditorRoot::GetActiveView()
{
	if (!GetMainFrame())
	{
		return NULL;
	}

	CLayoutViewPane* viewPane = (CLayoutViewPane*) GetMainFrame()->GetActiveView();
	if (viewPane)
	{
		CWnd *pWnd = viewPane->GetViewport();
		if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CViewport)))
		{
			return (CViewport*)pWnd;
		}
	}
	return 0;
}

void CEditorRoot::UpdateViews( int flags )
{
	m_viewMan->UpdateViews( flags );
}

_NAMESPACE_END