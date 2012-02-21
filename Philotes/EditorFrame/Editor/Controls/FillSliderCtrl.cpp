
#include "FillSliderCtrl.h"
#include "ColorUtil.h"

IMPLEMENT_DYNCREATE(CFillSliderCtrl,CSliderCtrlEx)

CFillSliderCtrl::CFillSliderCtrl()
{
	m_bFilled = true;
	m_fillStyle = eFillStyle_Vertical | eFillStyle_Gradient;
	m_fillColorStart = ::GetSysColor(COLOR_HIGHLIGHT);//RGB(100, 100, 100);
	m_fillColorEnd = ::GetSysColor(COLOR_GRAYTEXT);//RGB(180, 180, 180);
	m_mousePos.SetPoint(0,0);
}

CFillSliderCtrl::~CFillSliderCtrl()
{
}

BEGIN_MESSAGE_MAP(CFillSliderCtrl, CSliderCtrlEx)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ENABLE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
BOOL CFillSliderCtrl::OnEraseBkgnd(CDC* pDC)
{
	if (!m_bFilled)
		return __super::OnEraseBkgnd(pDC);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CFillSliderCtrl::DrawFill(CDC &dc, CRect &rect)
{
	m_fillColorStart = ::GetSysColor(COLOR_HIGHLIGHT);
	m_fillColorEnd = ::GetSysColor(COLOR_GRAYTEXT);
	if (m_fillStyle & eFillStyle_ColorHueGradient)
	{
		int width = rect.Width();
		int height = rect.Height();
		float step = 1.0f / float(width);
		float weight = 0.0f;
		ColorValue color;
		for (int i=0; i<width; ++i)
		{
			color = ColorUtil::ColorFromHSV(weight,1,1);
			::XTPDrawHelpers()->DrawLine(&dc, rect.left+i, rect.top, 0, height, ColorUtil::ColorPackBGR888(color));
			weight += step;
		}
		return;
	}

	COLORREF colorStart = m_fillColorStart;
	COLORREF colorEnd = m_fillColorEnd;
	if (!IsWindowEnabled())
	{
		colorStart = RGB(190, 190, 190);
		colorEnd = RGB(200, 200, 200);
	}

	if (m_fillStyle & eFillStyle_Gradient)
	{
		::XTPPaintManager()->GradientFill(
			&dc, rect, colorStart, colorEnd, !(m_fillStyle & eFillStyle_Vertical));
	}
	else
	{
		CBrush brush(colorStart);
		dc.FillRect(rect, &brush);
	}
}

//////////////////////////////////////////////////////////////////////////
void CFillSliderCtrl::OnPaint() 
{
	if (!m_bFilled)
	{
		__super::OnPaint();
		return;
	}

	CPaintDC dc(this);

	float val = m_value;
	
	CRect rc;
	GetClientRect(rc);
	rc.top += 1;
	rc.bottom -= 1;
	float pos = (val-m_min) / fabs(m_max-m_min);
	int splitPos = (int)(pos * rc.Width());

	if (splitPos < rc.left)
		splitPos = rc.left;
	if (splitPos > rc.right)
		splitPos = rc.right;

	if (m_fillStyle & eFillStyle_Background)
	{
		DrawFill(dc, rc);
	}
	else
	{
		CRect fillRc = rc;
		fillRc.right = splitPos;
		DrawFill(dc, fillRc);

		CRect emptyRc = rc;
		emptyRc.left = splitPos+1;
		emptyRc.IntersectRect(emptyRc,rc);
		COLORREF colour = RGB(255, 255, 255);
		if (!IsWindowEnabled())
			colour = RGB(230, 230, 230);
		CBrush brush(colour);
		dc.FillRect(emptyRc, &brush);
	}

	if (IsWindowEnabled())
	{
		CPen *pBlackPen = CPen::FromHandle((HPEN)GetStockObject(BLACK_PEN));
		CPen *pOldPen = dc.GetCurrentPen();
		dc.SelectObject(pBlackPen);
		dc.MoveTo( CPoint(splitPos,rc.top) );
		dc.LineTo( CPoint(splitPos,rc.bottom) );
		dc.SelectObject(pOldPen);
	}
}


//////////////////////////////////////////////////////////////////////////
void CFillSliderCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (!m_bFilled)
	{
		__super::OnLButtonDown(nFlags,point);
		return;
	}

	if (!IsWindowEnabled())
		return;

	CWnd *parent = GetParent();
	if (parent)
	{
		::SendMessage( parent->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),WMU_FS_LBUTTONDOWN),(LPARAM)GetSafeHwnd() );
	}	

	m_bUndoStarted = false;
	
	if (m_bUndoEnabled)
	{
		//GetIEditor()->BeginUndo();
		m_bUndoStarted = true;
	}

	ChangeValue(point.x,false);

	m_mousePos = point;
	SetCapture();
	m_bDragging = true;
}

//////////////////////////////////////////////////////////////////////////
void CFillSliderCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (!m_bFilled)
	{
		__super::OnLButtonUp(nFlags,point);
		return;
	}

	if (!IsWindowEnabled())
		return;

	bool bLButonDown = false;
	m_bDragging = false;
	if (GetCapture() == this)
	{
		bLButonDown = true;
		ReleaseCapture();
	}

	if (bLButonDown && m_bUndoStarted)
	{
		//if (GetIEditor()->IsUndoRecording())
		//	GetIEditor()->AcceptUndo( m_undoText );
		m_bUndoStarted = false;
	}
	m_bDragging = false;

	CWnd *parent = GetParent();
	if (parent)
	{
		::SendMessage( parent->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),WMU_FS_LBUTTONUP),(LPARAM)GetSafeHwnd() );
	}	
}

void CFillSliderCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (!m_bFilled)
	{
		__super::OnMouseMove(nFlags,point);
		return;
	}

	if (!IsWindowEnabled())
		return;

	if (point == m_mousePos)
		return;
	m_mousePos = point;

	if (m_bDragging)
	{
		m_bDragging = true;

		ChangeValue(point.x,true);
	}
}

void CFillSliderCtrl::ChangeValue( int sliderPos,bool bTracking )
{
	if (m_bLocked)
		return;

	CRect rc;
	GetClientRect(rc);
	if (sliderPos < rc.left)
		sliderPos = rc.left;
	if (sliderPos > rc.right)
		sliderPos = rc.right;
	
	float pos = (float)sliderPos / rc.Width();
	m_value = m_min + pos*fabs(m_max-m_min);

	NotifyUpdate(bTracking);
	RedrawWindow();
}

void CFillSliderCtrl::SetValue( float val )
{
	__super::SetValue( val );
	if (m_hWnd && m_bFilled)
	{
		RedrawWindow();
	}
}

void CFillSliderCtrl::NotifyUpdate( bool tracking )
{
	if (m_noNotify)
		return;

// 	if (tracking && m_bUndoEnabled && CUndo::IsRecording())
// 	{
// 		m_bLocked = true;
// 		GetIEditor()->RestoreUndo();
// 		m_bLocked = false;
// 	}

	if (m_updateCallback)
		m_updateCallback(this);

	if (m_hWnd)
	{
		CWnd *parent = GetParent();
		if (parent)
		{
			::SendMessage( parent->GetSafeHwnd(),
				WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),WMU_FS_CHANGED),
				(LPARAM)GetSafeHwnd() );
		}
	}
	m_lastUpdateValue = m_value;
}

//////////////////////////////////////////////////////////////////////////
void CFillSliderCtrl::SetFilledLook( bool bFilled )
{
	m_bFilled = bFilled;
}
