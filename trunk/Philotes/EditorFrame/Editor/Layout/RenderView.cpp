
#include "RenderView.h"
#include "d3dUtility.h"

_NAMESPACE_BEGIN

IMPLEMENT_DYNCREATE(CRenderView,CViewport)

BEGIN_MESSAGE_MAP(CRenderView, CViewport)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////

static int OnPaintFilter(unsigned int code, struct _EXCEPTION_POINTERS *ep)
{
	OutputDebugStringA("\nException was triggered during WM_PAINT.\n");

	if (IsDebuggerPresent())
	{
		DebugBreak();
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

//////////////////////////////////////////////////////////////////////////

CRenderView::CRenderView()
{
	m_bUpdating = false;
	m_viewSize.SetSize(0,0);

	CreateViewportWindow();
}

CRenderView::~CRenderView()
{

}

float CRenderView::GetAspectRatio() const
{
	// TODO : from Engine
	return 4.0f/3.0f;
}

void CRenderView::Update()
{
	if (!IsWindowVisible())
	{
		return;
	}

	if (m_bUpdating)
	{
		return;
	}
	m_bUpdating = true;

	OnRender();

	CViewport::Update();
	m_bUpdating = false;
}

// 测试
IDirect3DDevice9* gDevice = NULL;

void CRenderView::OnRender()
{
	d3d::Display(gDevice);
}

int CRenderView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CViewport::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 测试
	d3d::InitD3D(GetSafeHwnd(),m_viewSize.cx,m_viewSize.cy,true,&gDevice);

	return 0;
}

void CRenderView::OnSize(UINT nType, int cx, int cy) 
{
	CViewport::OnSize(nType, cx, cy);

	if(!cx || !cy)
	{
		return;
	}

	// resize事件处理

	m_viewSize.cx = cx;
	m_viewSize.cy = cy;

	GetClientRect( m_rcClient );
}

BOOL CRenderView::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}


void CRenderView::OnPaint() 
{
	if (true)
	{
		{
			CPaintDC dc(this);
		}

		OnPaintSafe();
	}
	else
	{
		CPaintDC dc(this);
	}
}

void CRenderView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CViewport::OnLButtonDown(nFlags, point);
}

void CRenderView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CViewport::OnLButtonUp(nFlags, point);
}

void CRenderView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CViewport::OnLButtonDblClk(nFlags, point);
}

void CRenderView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	//SetFocus();

	CViewport::OnRButtonDown(nFlags, point);

	//CaptureMouse();
}

void CRenderView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	CViewport::OnRButtonUp(nFlags,point);
}

void CRenderView::OnMButtonDown(UINT nFlags, CPoint point) 
{
	if (!(nFlags & MK_CONTROL) && !(nFlags & MK_SHIFT))
	{
		CaptureMouse();
	}
	
	CViewport::OnMButtonDown(nFlags, point);
}

void CRenderView::OnMButtonUp(UINT nFlags, CPoint point) 
{
	CViewport::OnMButtonUp(nFlags, point);
}

void CRenderView::OnMouseMove(UINT nFlags, CPoint point) 
{
	CViewport::OnMouseMove(nFlags, point);
}

void CRenderView::OnPaintSafe()
{
	__try
	{
		static bool bUpdating = false;
		if (!bUpdating)
		{
			bUpdating = true;
			Update();
			bUpdating = false;
		}
	}
	__except( OnPaintFilter(GetExceptionCode(), GetExceptionInformation()) ) {}
}

BOOL CRenderView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	return CViewport::OnMouseWheel(nFlags, zDelta, pt);
}

void CRenderView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CViewport::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CRenderView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return CViewport::OnSetCursor(pWnd, nHitTest, message);
}

void CRenderView::OnDestroy()
{
	CViewport::OnDestroy();
}

_NAMESPACE_END