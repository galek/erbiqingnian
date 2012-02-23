
#include "Viewport.h"
#include "EditorRoot.h"
#include "EditorTool.h"

IMPLEMENT_DYNAMIC(CViewport,CWnd)

BEGIN_MESSAGE_MAP (CViewport, CWnd)
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP ()


CViewport::CViewport()
{
	m_cViewMenu.LoadMenu(IDR_VIEW_OPTIONS);

	m_eViewMode = NothingMode;

	m_hCurrCursor = NULL;
	m_hCursor[STD_CURSOR_DEFAULT]			= AfxGetApp()->LoadStandardCursor(IDC_ARROW);
	m_hCursor[STD_CURSOR_HIT]				= AfxGetApp()->LoadCursor(IDC_POINTER_OBJHIT);
	m_hCursor[STD_CURSOR_MOVE]				= AfxGetApp()->LoadCursor(IDC_POINTER_OBJECT_MOVE);
	m_hCursor[STD_CURSOR_ROTATE]			= AfxGetApp()->LoadCursor(IDC_POINTER_OBJECT_ROTATE);
	m_hCursor[STD_CURSOR_SCALE]				= AfxGetApp()->LoadCursor(IDC_POINTER_OBJECT_SCALE);
	m_hCursor[STD_CURSOR_SEL_PLUS]			= AfxGetApp()->LoadCursor(IDC_POINTER_PLUS);
	m_hCursor[STD_CURSOR_SEL_MINUS]			= AfxGetApp()->LoadCursor(IDC_POINTER_MINUS);
	m_hCursor[STD_CURSOR_SUBOBJ_SEL]		= AfxGetApp()->LoadCursor(IDC_POINTER_SO_SELECT);
	m_hCursor[STD_CURSOR_SUBOBJ_SEL_PLUS]	= AfxGetApp()->LoadCursor(IDC_POINTER_SO_SELECT_PLUS);
	m_hCursor[STD_CURSOR_SUBOBJ_SEL_MINUS]	= AfxGetApp()->LoadCursor(IDC_POINTER_SO_SELECT_MINUS);
	m_hCursor[STD_CURSOR_CRYSIS]			= AfxGetApp()->LoadCursor(IDC_CRYSISCURSOR);

	m_viewManager = NULL;
}

CViewport::~CViewport()
{

}

void CViewport::Update()
{
	
}

void CViewport::SetName( const CString &name )
{
	if (IsWindow(GetSafeHwnd()))
	{
		SetWindowText(name);
		if (GetParent())
		{
// 			if (GetParent()->IsKindOf(RUNTIME_CLASS(CLayoutViewPane)))
// 			{
// 				GetParent()->SendMessage( WM_VIEWPORT_ON_TITLE_CHANGE );
// 			}
		}
	}
}

CString CViewport::GetName() const
{
	CString name;
	if (IsWindow(GetSafeHwnd()))
	{
		GetWindowText(name);
	}

	return name;
}

void CViewport::SetActive( bool bActive )
{
	m_bActive = bActive;
}

bool CViewport::IsActive() const
{
	return m_bActive;
}

void CViewport::ResetContent()
{

}

void CViewport::OnActivate()
{

}

void CViewport::OnDeactivate()
{

}

void CViewport::SetCursor( HCURSOR hCursor )
{
	m_hCurrCursor = hCursor;
	if (m_hCurrCursor)
	{
		::SetCursor( m_hCurrCursor );
	}
	else
	{
		::SetCursor( m_hCursor[STD_CURSOR_DEFAULT] );
	}
}

CEditorDoc* CViewport::GetDocument()
{
	return EditorRoot::Get().GetDocument();
}

CEditTool* CViewport::GetEditTool()
{
	// todo : 本地处理
	return EditorRoot::Get().GetEditTool();
}

void CViewport::SetEditTool( CEditTool *pEditTool,bool bLocalToViewport/*=false */ )
{
	// todo : 本地记录
	EditorRoot::Get().SetEditTool(pEditTool);
}

void CViewport::CreateViewportWindow()
{
	if (!m_hWnd)
	{
		CRect rcDefault(0,0,100,100);
		LPCTSTR lpClassName = AfxRegisterWndClass(CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|CS_OWNDC,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW), NULL, NULL);
		VERIFY( CreateEx( NULL,lpClassName,"",WS_CHILD|WS_CLIPCHILDREN,rcDefault, AfxGetMainWnd(), NULL));
	}
}

void CViewport::PostNcDestroy()
{
	delete this;
}

void CViewport::OnMouseMove( UINT nFlags, CPoint point )
{
	CWnd::OnMouseMove(nFlags, point);
}

void CViewport::OnDestroy()
{
	CWnd::OnDestroy();
}

BOOL CViewport::OnEraseBkgnd( CDC* pDC )
{
	RECT rect;
	CBrush cFillBrush;

	GetClientRect(&rect);

	cFillBrush.CreateSolidBrush(0x00F0F0F0);

	pDC->FillRect(&rect, &cFillBrush);

	return TRUE;
}

void CViewport::OnPaint()
{
	CPaintDC dc(this);
	RECT rect;
	GetClientRect(&rect);

	CBrush cFillBrush;
	cFillBrush.CreateSolidBrush(0x00F0F0F0);

	dc.FillRect(&rect, &cFillBrush);
}

BOOL CViewport::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt )
{
	return TRUE;
}

void CViewport::OnLButtonDown( UINT nFlags, CPoint point )
{
	if (GetFocus() != this)
		SetFocus();

	CWnd::OnLButtonDown(nFlags, point);
}

void CViewport::OnLButtonUp( UINT nFlags, CPoint point )
{
	CWnd::OnLButtonUp(nFlags, point);
}

void CViewport::OnRButtonDown( UINT nFlags, CPoint point )
{
	CWnd::OnRButtonDown(nFlags, point);
}

void CViewport::OnRButtonUp( UINT nFlags, CPoint point )
{
	CWnd::OnRButtonUp(nFlags, point);
}

void CViewport::OnMButtonDblClk( UINT nFlags, CPoint point )
{
	CWnd::OnMButtonDblClk(nFlags, point);
}

void CViewport::OnMButtonDown( UINT nFlags, CPoint point )
{
	CWnd::OnMButtonDown(nFlags, point);
}

void CViewport::OnMButtonUp( UINT nFlags, CPoint point )
{
	CWnd::OnMButtonUp(nFlags, point);
}

void CViewport::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CViewport::OnRButtonDblClk( UINT nFlags, CPoint point )
{
	CWnd::OnRButtonDblClk(nFlags, point);
}

void CViewport::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CViewport::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}

BOOL CViewport::OnSetCursor( CWnd* pWnd, UINT nHitTest, UINT message )
{
	if (m_hCurrCursor != NULL)
	{
		SetCursor( m_hCurrCursor );
		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CViewport::UpdateContent( int flags )
{

}

void CViewport::SetViewManager( CViewManager* vm )
{
	m_viewManager = vm;
}

void CViewport::CaptureMouse()
{
	if (GetCapture() != this)
	{
		SetCapture();
	}
}

void CViewport::ReleaseMouse()
{
	if (GetCapture() == this)
	{
		ReleaseCapture();
	}
}
