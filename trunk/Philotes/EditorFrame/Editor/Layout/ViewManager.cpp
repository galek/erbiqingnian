
#include "ViewManager.h"
#include "Viewport.h"
#include "EditorRoot.h"
#include "RenderView.h"
#include "../MainFrm.h"

class CViewportClassDesc : public TRefCountBase<IViewPaneClass>
{
public:
	CViewportClassDesc( const CString &className,CRuntimeClass *pClass )
	{
		m_className		= className;
		m_pClass		= pClass;
		CoCreateGuid(&m_guid);
	}

	virtual ESystemClassID		SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };

	virtual REFGUID				ClassID() { return m_guid; }

	virtual const char*			ClassName() { return m_className; };

	virtual const char*			Category() { return m_className; };

	virtual CRuntimeClass*		GetRuntimeClass() { return m_pClass; };

	virtual const char*			GetPaneTitle() { return m_className; };

	virtual EDockingDirection	GetDockingDirection() { return DOCK_FLOAT; };

	virtual CRect				GetPaneRect() { return CRect(0,0,400,400); };

	virtual bool 				SinglePane() { return false; };

	virtual bool 				WantIdleUpdate() { return true; };

private:
	CRuntimeClass*				m_pClass;

	CString						m_className;

	GUID						m_guid;
};

//////////////////////////////////////////////////////////////////////////

CViewport* CViewManager::CreateView( EViewportType type,CWnd *pParentWnd )
{
	CViewportDesc *vd = 0;
	for (int i = 0; i < m_viewportDesc.size(); i++)
	{
		if (m_viewportDesc[i]->type == type)
		{
			vd = m_viewportDesc[i];
		}
	}

	if (!vd)
	{
		return NULL;
	}

	CViewport *pViewport = (CViewport*)vd->rtClass->CreateObject();
	if (!pViewport)
	{
		return NULL;
	}

	CRect rcDefault(0,0,100,100);
	LPCTSTR lpClassName = AfxRegisterWndClass(CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|CS_OWNDC,
		AfxGetApp()->LoadStandardCursor(IDC_ARROW), NULL, NULL);
	VERIFY( pViewport->CreateEx( NULL,lpClassName,"",WS_POPUP|WS_CLIPCHILDREN,rcDefault, pParentWnd, NULL));

	pViewport->SetViewManager(this);
	pViewport->SetType( type );
	pViewport->SetName(vd->name);

	return pViewport;
}

void CViewManager::ReleaseView( CViewport *pViewport )
{
	pViewport->DestroyWindow();
}

CViewport* CViewManager::GetViewport( EViewportType type ) const
{
	for (int i = 0; i < m_viewports.size(); i++)
	{
		if (m_viewports[i]->GetType() == type)
		{
			return m_viewports[i];
		}
	}
	return NULL;
}

CViewport* CViewManager::GetViewport( const CString &name ) const
{
	for (int i = 0; i < m_viewports.size(); i++)
	{
		if (stricmp(m_viewports[i]->GetName(),name) == 0)
		{
			return m_viewports[i];
		}
	}
	return NULL;
}

CViewport* CViewManager::GetActiveViewport() const
{
	for (int i = 0; i < m_viewports.size(); i++)
	{
		if (m_viewports[i]->IsActive())
		{
			return m_viewports[i];
		}
	}
	return NULL;
}

CViewport* CViewManager::GetViewportAtPoint( CPoint point ) const
{
	CWnd *wnd = CWnd::WindowFromPoint( point );

	for (int i = 0; i < m_viewports.size(); i++)
	{
		if (m_viewports[i] == wnd)
		{
			return m_viewports[i];
		}
	}

	return NULL;
}

void CViewManager::ResetViews()
{
	for (int i = 0; i < m_viewports.size(); i++)
	{
		m_viewports[i]->ResetContent();
	}
}

void CViewManager::UpdateViews( int flags/*=0xFFFFFFFF */ )
{
	for (int i = 0; i < m_viewports.size(); i++)
	{
		m_viewports[i]->UpdateContent( flags );
	}
}

void CViewManager::RegisterViewportDesc( CViewportDesc *desc )
{
	desc->pViewClass = new CViewportClassDesc(desc->name,desc->rtClass);
	m_viewportDesc.push_back(desc);

	EditorRoot::Get().GetClassFactory()->RegisterClass( desc->pViewClass );
}

void CViewManager::GetViewportDescriptions( std::vector<CViewportDesc*> &descriptions )
{
	descriptions.clear();
	for (int i = 0; i < m_viewportDesc.size(); i++)
	{
		descriptions.push_back( m_viewportDesc[i] );
	}
}

CLayoutWnd* CViewManager::GetLayout() const
{
	CMainFrame *pMainFrame = ((CMainFrame*)AfxGetMainWnd());
	if (pMainFrame)
	{
		return pMainFrame->GetLayout();
	}
	return NULL;
}

CViewport* CViewManager::GetGameViewport() const
{
	return GetViewport(ET_ViewportCamera);
}

void CViewManager::IdleUpdate()
{
	for (int i = 0; i < m_viewports.size(); i++)
	{
		m_viewports[i]->Update();
	}
}

void CViewManager::RegisterViewport( CViewport *vp )
{
	vp->SetViewManager(this);
	m_viewports.push_back(vp);
}

void CViewManager::UnregisterViewport( CViewport *vp )
{
	stl::FindAndErase( m_viewports,vp );
}

CViewManager::CViewManager()
{
	//×¢²áÊÓ¿Ú
	RegisterViewportDesc( new CViewportDesc(ET_ViewportCamera,_T("Í¸ÊÓ"),RUNTIME_CLASS(CRenderView)) );
}

CViewManager::~CViewManager()
{
	m_viewports.clear();
	m_viewportDesc.clear();
}
