
#include "ViewPane.h"
#include "EditorRoot.h"
#include "Viewport.h"
#include "ViewPaneInterface.h"

const int MAX_CLASSVIEWS = 100;

enum TitleMenuCommonCommands
{
	ID_MAXIMIZED = 50000,
	ID_LAYOUT_CONFIG,

	FIRST_ID_CLASSVIEW,
	LAST_ID_CLASSVIEW = FIRST_ID_CLASSVIEW + MAX_CLASSVIEWS - 1
};

#define VIEW_BORDER 0


IMPLEMENT_DYNCREATE(CLayoutViewPane, CView)

BEGIN_MESSAGE_MAP(CLayoutViewPane, CView)
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_MESSAGE(WM_VIEWPORT_ON_TITLE_CHANGE, OnViewportTitleChange)
END_MESSAGE_MAP()

CLayoutViewPane::CLayoutViewPane()
{
	m_viewport	= NULL;
	m_active	= false;
	m_nBorder	= VIEW_BORDER;

	m_titleHeight = 16;
	m_bFullscreen = false;
	m_viewportTitleDlg.SetViewPane(this);

	m_id = -1;
}

CLayoutViewPane::~CLayoutViewPane()
{
}

void CLayoutViewPane::SetViewClass( const CString &sClass )
{
	if (m_viewPaneClass == sClass && m_viewport != 0)
		return;
	m_viewPaneClass = sClass;

	ReleaseViewport();

	IClassDesc *pClass = EditorRoot::Get().GetClassFactory()->FindClass( sClass );
	if (pClass)
	{
		IViewPaneClass *pViewClass = NULL;
		HRESULT hRes = pClass->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewClass );
		if (!FAILED(hRes))
		{
			CWnd *pNewView = (CWnd*)pViewClass->GetRuntimeClass()->CreateObject();
			pNewView->SetWindowText( m_viewPaneClass );
			AttachViewport( pNewView );
		}
	}
}

CString CLayoutViewPane::GetViewClass() const
{
	return m_viewPaneClass;
}

void CLayoutViewPane::OnDestroy() 
{
	ReleaseViewport();

	CView::OnDestroy();
}

void CLayoutViewPane::OnDraw(CDC* pDC)
{
	CDC &dc = *pDC;
}

#ifdef _DEBUG
void CLayoutViewPane::AssertValid() const
{
}

void CLayoutViewPane::Dump(CDumpContext& dc) const
{
}

#endif //_DEBUG

int CLayoutViewPane::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	m_viewportTitleDlg.Create(CViewportTitleDlg::IDD,this);
	m_viewportTitleDlg.ShowWindow(SW_SHOW);

	CRect rc;
	m_viewportTitleDlg.GetClientRect(rc);
	m_titleHeight = rc.Height();
	
	return 0;
}

void CLayoutViewPane::SwapViewports( CLayoutViewPane *pView )
{
	CWnd *pViewport = pView->GetViewport();
	CWnd *pViewportOld = m_viewport;

	std::swap( m_viewPaneClass,pView->m_viewPaneClass );

	AttachViewport( pViewport );
	pView->AttachViewport(pViewportOld);
}

void CLayoutViewPane::AttachViewport( CWnd *pViewport )
{
	if (pViewport == m_viewport)
	{
		return;
	}

	m_viewport = pViewport;
	if (pViewport)
	{
		m_viewport->ModifyStyle( WS_POPUP,WS_CHILD,0 );
		m_viewport->SetParent( this );

		RecalcLayout();
		m_viewport->ShowWindow( SW_SHOW );
		m_viewport->Invalidate(FALSE);

		SetWindowText( m_viewPaneClass );
		CString title;
		pViewport->GetWindowText(title);
		m_viewportTitleDlg.SetTitle( title );
		Invalidate(FALSE);
	}
}

void CLayoutViewPane::DetachViewport()
{
	m_viewport = 0;
}

void CLayoutViewPane::ReleaseViewport()
{
	if (m_viewport && m_viewport->m_hWnd != 0)
	{
		m_viewport->DestroyWindow();
		m_viewport = 0;
	}
}

void CLayoutViewPane::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	int toolHeight = 0;
	
	RecalcLayout();

	if (m_viewportTitleDlg.m_hWnd)
	{
		int w = 0;
		int h = 0;
		if (m_viewport)
		{
			CRect rc;
			m_viewport->GetClientRect(rc);
			w = rc.Width();
			h = rc.Height();
		}
		m_viewportTitleDlg.SetViewportSize( w,h );
	}
}

void CLayoutViewPane::RecalcLayout()
{
	CRect rc;
	GetClientRect(rc);
	int cx = rc.Width();
	int cy = rc.Height();

	int nBorder = VIEW_BORDER;

	if (m_viewportTitleDlg.m_hWnd)
	{
		m_viewportTitleDlg.MoveWindow( m_nBorder,m_nBorder,cx-m_nBorder,m_titleHeight );
	}

	if (m_viewport)
		m_viewport->MoveWindow( m_nBorder,m_nBorder+m_titleHeight,cx-m_nBorder,cy-m_nBorder*2-m_titleHeight );
}

BOOL CLayoutViewPane::OnEraseBkgnd(CDC* pDC) 
{
	return FALSE;
}

void CLayoutViewPane::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);

	m_active = bActivate;
	m_viewportTitleDlg.SetActive( m_active );
}

void CLayoutViewPane::ToggleMaximize()
{
}

void CLayoutViewPane::ShowTitleMenu()
{

}

void CLayoutViewPane::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CView::OnRButtonDown(nFlags, point);
}

void CLayoutViewPane::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	ToggleMaximize();
}

BOOL CLayoutViewPane::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CLayoutViewPane::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if (m_viewport && m_viewport->IsKindOf(RUNTIME_CLASS(CViewport)))
	{
		((CViewport*)m_viewport)->UpdateContent( 0xFFFFFFFF );
	}
}

void CLayoutViewPane::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	
	if (m_viewport)
	{
		m_viewport->SetFocus();
	}
}

BOOL CLayoutViewPane::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CLayoutViewPane::SetFullscren( bool f )
{
	m_bFullscreen = f;
}

void CLayoutViewPane::SetFullscreenViewport( bool b )
{
	
}

LRESULT CLayoutViewPane::OnViewportTitleChange( WPARAM wParam, LPARAM lParam )
{
	if (m_viewport)
	{
		CString title;
		m_viewport->GetWindowText(title);
		m_viewportTitleDlg.SetTitle( title );
	}
	return FALSE;
}
