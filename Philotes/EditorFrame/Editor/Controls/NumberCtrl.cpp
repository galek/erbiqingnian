#include "NumberCtrl.h"
#include "Settings.h"
#include "MathUtil.h"

#define FLOAT_FMT "%.7g"

CNumberCtrl::CNumberCtrl()
{
	m_btnStatus = 0;
	m_btnWidth = 10;
	m_draggin = false;
	m_value = 0;
	m_min = -FLT_MAX;
	m_max = FLT_MAX;
	m_step = 0.01;
	m_enabled = true;
	m_noNotify = false;
	m_integer = false;
	m_nFlags = 0;
	m_bUndoEnabled = false;
	m_bDragged = false;
	m_multiplier = 1;
	m_bLocked = false;
	m_bInitialized = false;

	m_upArrow = 0;
	m_downArrow = 0;

	m_fmt = FLOAT_FMT;
}

CNumberCtrl::~CNumberCtrl()
{
	m_bInitialized = false;
}


BEGIN_MESSAGE_MAP(CNumberCtrl, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ENABLE()
	ON_WM_SETFOCUS()
	ON_EN_SETFOCUS(IDC_EDIT,OnEditSetFocus)
	ON_EN_KILLFOCUS(IDC_EDIT,OnEditKillFocus)

	ON_WM_SIZE()
END_MESSAGE_MAP()

void CNumberCtrl::Create( CWnd* parentWnd,CRect &rc,UINT nID,int nFlags )
{
	m_nFlags = nFlags;
	HCURSOR arrowCursor = AfxGetApp()->LoadStandardCursor( IDC_ARROW );
	CreateEx( 0,AfxRegisterWndClass(NULL,arrowCursor,NULL/*(HBRUSH)GetStockObject(LTGRAY_BRUSH)*/),NULL,WS_CHILD|WS_VISIBLE|WS_TABSTOP,rc,parentWnd,nID );
}

void CNumberCtrl::Create( CWnd* parentWnd,UINT ctrlID,int flags )
{
	ASSERT( parentWnd );
	m_nFlags = flags;
	CRect rc;
	CWnd *ctrl = parentWnd->GetDlgItem( ctrlID );
	if(ctrl)
	{
		ctrl->SetDlgCtrlID( ctrlID + 10000 );
		ctrl->ShowWindow( SW_HIDE );
		ctrl->GetWindowRect( rc );
		parentWnd->ScreenToClient(rc);
	}

	HCURSOR arrowCursor = AfxGetApp()->LoadStandardCursor( IDC_ARROW );
	CreateEx( 0,AfxRegisterWndClass(NULL,arrowCursor,NULL),NULL,WS_CHILD|WS_VISIBLE|WS_TABSTOP,rc,parentWnd,ctrlID );
}

int CNumberCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_upDownCursor = AfxGetApp()->LoadStandardCursor( IDC_SIZENS );
	m_upArrow = (HICON)LoadImage( AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_UP_ARROW),IMAGE_ICON,5,5,LR_DEFAULTCOLOR|LR_SHARED );
	m_downArrow = (HICON)LoadImage( AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_DOWN_ARROW),IMAGE_ICON,5,5,LR_DEFAULTCOLOR|LR_SHARED );
	
	CRect rc;
	GetClientRect( rc );

	if (m_nFlags & LEFTARROW)
	{
		rc.left += m_btnWidth+3;
	}
	else
	{
		rc.right -= m_btnWidth+1;
	}

	DWORD nFlags = WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL;
	if (m_nFlags & LEFTALIGN)
		nFlags |= ES_LEFT;
	else if (m_nFlags & CENTER_ALIGN)
		nFlags |= ES_CENTER;
	else
		nFlags |= ES_RIGHT;

	DWORD nExFlags = 0;
	if (!(m_nFlags & NOBORDER))
		nExFlags |= WS_EX_CLIENTEDGE;

	m_edit.CreateEx( nExFlags, _T("EDIT"),"",nFlags,rc,this,IDC_EDIT );
	m_edit.SetFont( CFont::FromHandle( (HFONT)EditorSettings::Get().Gui.hSystemFont) );
	m_edit.SetUpdateCallback( functor(*this,&CNumberCtrl::OnEditChanged) );

	double val = m_value;
	m_value = val+1;
	SetInternalValue( val );

	m_bInitialized = true;
	
	return 0;
}

void CNumberCtrl::SetLeftAlign( bool left )
{
	if (m_edit)
	{
		if (left)
			m_edit.ModifyStyle( ES_RIGHT,ES_LEFT );
		else
			m_edit.ModifyStyle( ES_LEFT,ES_RIGHT );
	}
}

void CNumberCtrl::OnPaint() 
{
	CPaintDC dc(this); 
	DrawButtons( dc );
}

void CNumberCtrl::DrawButtons( CDC &dc )
{
	CRect rc;
	GetClientRect( rc );

	if (!m_enabled)
	{
		dc.FillSolidRect( rc,GetSysColor(COLOR_3DFACE) );
		return;
	}
	int x = 0;
	if (!(m_nFlags & LEFTARROW))
	{
		x = rc.right-m_btnWidth;
	}
	int y = rc.top;
	int w = m_btnWidth;
	int h = rc.bottom;
	int h2 = h/2;
	COLORREF hilight = RGB(130,130,130);
	COLORREF shadow = RGB(100,100,100);

	int smallOfs = 0;
	if (rc.bottom < 18)
		smallOfs = 1;

	if (m_btnStatus == 1 || m_btnStatus == 3)
	{
		dc.Draw3dRect( x,y,w,h2,shadow,hilight );
	}
	else
	{
		dc.Draw3dRect( x,y,w,h2,hilight,shadow );
	}

	if (m_btnStatus == 2 || m_btnStatus == 3)
		dc.Draw3dRect( x,y+h2+1,w,h2-1+smallOfs,shadow,hilight );
	else
		dc.Draw3dRect( x,y+h2+1,w,h2-1+smallOfs,hilight,shadow );

	DrawIconEx( dc,x+2,y+2,m_upArrow,5,5,0,0,DI_NORMAL );

	DrawIconEx( dc,x+2,y+h2+3-smallOfs,m_downArrow,5,5,0,0,DI_NORMAL );
}

void CNumberCtrl::GetBtnRect( int btn,CRect &rc )
{
	CRect rcw;
	GetClientRect( rcw );

	int x = 0;
	if (!(m_nFlags & LEFTARROW))
	{
		x = rcw.right-m_btnWidth;
	}
	int y = rcw.top;
	int w = m_btnWidth;
	int h = rcw.bottom;
	int h2 = h/2;

	if (btn == 0)
	{
		rc.SetRect( x,y,x+w,y+h2 );
	}
	else if (btn == 1)
	{
		rc.SetRect( x,y+h2+1,x+w,y+h );
	}
}

int CNumberCtrl::GetBtn( CPoint point )
{
	for (int i = 0; i < 2; i++)
	{
		CRect rc;
		GetBtnRect( i,rc );
		if (rc.PtInRect(point))
		{
			return i;
		}
	}
	return -1;
}

void CNumberCtrl::SetBtnStatus( int s )
{
	m_btnStatus = s;
	RedrawWindow();
}

void CNumberCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (!m_enabled)
		return;

	m_btnStatus = 0;
	int btn = GetBtn(point);
	if (btn >= 0)
	{
		SetBtnStatus( btn+1 );
		m_bDragged = false;

		//if (m_bUndoEnabled && !CUndo::IsRecording())
			//GetIEditor()->BeginUndo();

		CWnd *parent = GetOwner();
		if (parent)
		{
			::SendMessage( parent->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),EN_BEGIN_DRAG ),(LPARAM)GetSafeHwnd() );
		}
	}
	m_mousePos = point;
	SetCapture();
	m_draggin = true;

	CWnd *parent = GetOwner();
	if (parent)
	{
		::SendMessage( parent->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),WMU_LBUTTONDOWN ),(LPARAM)GetSafeHwnd() );
	}	
	CWnd::OnLButtonDown(nFlags, point);
}

void CNumberCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (!m_enabled)
		return;

	bool bLButonDown = false;
	if (GetCapture() == this)
	{
		bLButonDown = true;
		ReleaseCapture();
	}
	SetBtnStatus( 0 );

	bool bSendEndDrag = false;
	if (m_draggin)
		bSendEndDrag = true;

	m_draggin = false;

	if (bLButonDown)
	{
		int btn = GetBtn(point);
		if (!m_bDragged && btn >= 0 && m_step > 0.f)
		{
			double prevValue = m_value;

			if (btn == 0)
				SetInternalValue( MathUtil::Round(GetInternalValue() + m_step, m_step) );
			else if (btn == 1)
				SetInternalValue( MathUtil::Round(GetInternalValue() - m_step, m_step) );

			if (prevValue != m_value)
				NotifyUpdate(false);
		}
		else if (m_bDragged)
		{
			NotifyUpdate(false);
		}
	}

	//if (m_bUndoEnabled && CUndo::IsRecording())
		//GetIEditor()->AcceptUndo( m_undoText );

	CWnd *parent = GetOwner();
	if (parent)
	{
		::SendMessage( parent->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),WMU_LBUTTONUP ),(LPARAM)GetSafeHwnd() );
		if (bSendEndDrag)
			::SendMessage( parent->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),EN_END_DRAG ),(LPARAM)GetSafeHwnd() );
	}	

	if (m_edit)
		m_edit.SetFocus();
}

void CNumberCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (!m_enabled)
		return;
	
	if (point == m_mousePos)
		return;

	if (m_draggin)
	{
		m_bDragged = true;
		double prevValue = m_value;

		SetCursor( m_upDownCursor );
		if (m_btnStatus != 3)
			SetBtnStatus(3);
		
		int y = point.y-m_mousePos.y;
		if (!m_integer)
			y *= abs(y);
		SetInternalValue( MathUtil::Round(GetInternalValue() - m_step*y, m_step) );

		m_edit.UpdateWindow();

		CPoint cp;
		GetCursorPos( &cp );
		int sX = GetSystemMetrics(SM_CXSCREEN);
		int sY = GetSystemMetrics(SM_CYSCREEN);

		if (cp.y < 20 || cp.y > sY-20)
		{
			CPoint p = m_mousePos;
			ClientToScreen(&p);
			SetCursorPos( p.x,p.y );
		}
		else
		{
			m_mousePos = point;
		}

		if (prevValue != m_value)
			NotifyUpdate(true);
	}
	
	CWnd::OnMouseMove(nFlags, point);
}

void CNumberCtrl::SetRange( double min,double max )
{
	if (m_bLocked)
		return;
	m_min = min;
	m_max = max;
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::SetInternalValue( double val )
{
	if (m_bLocked)
		return;
	CString s;
	
	if (val < m_min) val = m_min;
	if (val > m_max) val = m_max;

	if (m_integer)
	{
		if (MathUtil::IntRound(m_value) == MathUtil::IntRound(val))
			return;
		s.Format( "%d", MathUtil::IntRound(val) );
	}
	else
	{
		s.Format( m_fmt, val );
	}
	m_value = val;
	
	if (m_edit)
	{
		m_noNotify = true;
		m_edit.SetText( s );
		m_noNotify = false;
	}
}

//////////////////////////////////////////////////////////////////////////
double CNumberCtrl::GetInternalValue() const
{
	if (!m_enabled)
		return m_value;

	double v;

	if (m_edit)
	{
		CString str;
		m_edit.GetWindowText(str);
		v = m_value = atof( (const char*)str );
	}
	else
		v = m_value;

	if (v < m_min) v = m_min;
	if (v > m_max) v = m_max;

	return v;
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::SetValue( double val )
{
	if (m_bLocked)
		return;
	SetInternalValue( val*m_multiplier );
}

//////////////////////////////////////////////////////////////////////////
double CNumberCtrl::GetValue() const
{
	return GetInternalValue() / m_multiplier;
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::SetStep( double step )
{
	if (m_bLocked)
		return;
	m_step = step;
	if (m_integer && m_step < 1)
		m_step = 1;
}

//////////////////////////////////////////////////////////////////////////
double CNumberCtrl::GetStep() const
{
	return m_step;
}

//////////////////////////////////////////////////////////////////////////
CString CNumberCtrl::GetValueAsString() const
{
	CString str;
	str.Format( m_fmt, m_value/m_multiplier );
	return str;
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::OnEditChanged()
{
	static bool inUpdate = false;

	if (!inUpdate)
	{
		double prevValue = m_value;

		double v = GetInternalValue();
		if (v != m_value)
		{
			SetInternalValue( v );
		}

		if (prevValue != m_value)
			NotifyUpdate(false);

		inUpdate = false;
	}
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::OnEnable(BOOL bEnable) 
{
	CWnd::OnEnable(bEnable);
	
	m_enabled = (bEnable == TRUE);
	if (m_edit)
	{
		m_edit.EnableWindow(bEnable);
		RedrawWindow();
	}
}

void CNumberCtrl::NotifyUpdate( bool tracking )
{
	if (m_noNotify)
		return;

	CWnd *parent = GetOwner();

// 	bool bUndoRecording = CUndo::IsRecording();
// 	bool bSaveUndo = (!tracking && m_bUndoEnabled && !bUndoRecording);
// 	
// 	if (bSaveUndo)
// 		GetIEditor()->BeginUndo();
// 
// 	if (tracking && m_bUndoEnabled && bUndoRecording)
// 	{
// 		m_bLocked = true;
// 		GetIEditor()->RestoreUndo(); // This can potentially modify m_value;
// 		m_bLocked = false;
// 	}

	if (m_updateCallback)
		m_updateCallback(this);

	if (parent)
	{
		::SendMessage( parent->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),EN_UPDATE ),(LPARAM)GetSafeHwnd() );
		if (!tracking)
		{
			::SendMessage( parent->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),EN_CHANGE ),(LPARAM)GetSafeHwnd() );
		}
	}
	m_lastUpdateValue = m_value;

// 	if (bSaveUndo)
// 		GetIEditor()->AcceptUndo( m_undoText );

}

void CNumberCtrl::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
	
	if (m_edit)
	{
		m_edit.SetFocus();
		m_edit.SetSel(0,-1);
	}
}

void CNumberCtrl::SetInteger( bool enable )
{
	m_integer = enable;
	m_step = 1;
	double f = GetInternalValue();
	m_value = f+1;
	SetInternalValue(f);
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::OnEditSetFocus()
{
	if (!m_bInitialized)
		return;
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::OnEditKillFocus()
{
	if (!m_bInitialized)
		return;

	if (m_value != m_lastUpdateValue)
		NotifyUpdate(false);
}


int CNumberCtrl::CalculateDecimalPlaces( double infNumber, int iniMaxPlaces )
{
	assert(iniMaxPlaces>=0);
	char str[256],*_str=str;

	sprintf(str,"%f",infNumber);

	while(*_str!='.')_str++;											// search the comma
	int ret=0;

	if(*_str!=0)																	// comma?
	{
		int cur=1;
		_str++;																			// jump over comma

		while(*_str>='0' && *_str<='9')
		{
			if(*_str!='0')ret=cur;
			_str++;cur++;
		}

		if(ret>iniMaxPlaces)ret=iniMaxPlaces;				// bound to maximum
	}
	return(ret);
}



void CNumberCtrl::SetInternalPrecision( int iniDigits )
{
	m_step = pow(10.f, -iniDigits);
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::EnableUndo( const CString& undoText )
{
	m_undoText = undoText;
	m_bUndoEnabled = true;
}

void CNumberCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	if (m_edit.m_hWnd)
	{
		CRect rc;
		GetClientRect( rc );
		if (m_nFlags & LEFTARROW)
		{
			rc.left += m_btnWidth+3;
		}
		else
		{
			rc.right -= m_btnWidth+1;
		}
		m_edit.MoveWindow(rc,FALSE);
	}
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::SetFont( CFont* pFont,BOOL bRedraw )
{
	CWnd::SetFont(pFont,bRedraw);
	if (m_edit.m_hWnd)
		m_edit.SetFont(pFont,bRedraw);
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::SetMultiplier( double fMultiplier )
{
	if (fMultiplier != 0)
		m_multiplier = fMultiplier;
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrl::SetFloatFormat( const char* fmt)
{
	m_fmt = fmt;
}

//////////////////////////////////////////////////////////////////////////
BOOL CNumberCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (CWnd::OnNotify(wParam, lParam, pResult))
		return TRUE;

	if (GetOwner() && wParam == IDC_EDIT)
	{
		NMHDR *pNHDR = (NMHDR*)lParam;
		pNHDR->hwndFrom = GetSafeHwnd();
		pNHDR->idFrom = GetDlgCtrlID();
		*pResult = GetOwner()->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), lParam );
	}
	return TRUE;
}
