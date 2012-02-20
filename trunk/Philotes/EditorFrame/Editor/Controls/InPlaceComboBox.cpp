
#include "InPlaceComboBox.h"
#include "Settings.h"
#include <WinDef.h>

#pragma   warning(disable:4706)
#define   COMPILE_MULTIMON_STUBS
#include   <multimon.h>

#define WM_USER_ON_SELECTION_CANCEL		(WM_USER + 10)
#define WM_USER_ON_SELECTION_OK			(WM_USER + 11)
#define WM_USER_ON_NEW_SELECTION		(WM_USER + 12)
#define WM_USER_ON_EDITCHANGE			(WM_USER + 13)
#define WM_USER_ON_OPENDROPDOWN			(WM_USER + 14)
#define WM_USER_ON_EDITKEYDOWN			(WM_USER + 15)
#define WM_USER_ON_EDITCLICK			(WM_USER + 16)


/////////////////////////////////////////////////////////////////////////////
// CInPlaceCBEdit

BOOL CInPlaceCBEdit::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN)
	{
		CWnd* pOwner = GetOwner();
		WPARAM nChar = pMsg->wParam;
		::PeekMessage(pMsg, NULL, NULL, NULL, PM_REMOVE);

		switch(nChar)
		{
			case VK_ESCAPE:
			case VK_RETURN:
				pOwner->SendMessage(WM_USER_ON_EDITCHANGE, nChar);
				pOwner->GetParent()->SetFocus();
				return TRUE;
			case VK_UP:
			case VK_DOWN:
			//case VK_TAB:
				//pOwner->SendMessage(WM_USER_ON_NEW_SELECTION, nChar);
				if (nChar == VK_TAB && pOwner)
				{
					NMKEY nmkey;
					nmkey.hdr.code = NM_KEYDOWN;
					nmkey.hdr.hwndFrom = pOwner->GetSafeHwnd();
					nmkey.hdr.idFrom = pOwner->GetDlgCtrlID();
					nmkey.nVKey = nChar;
					nmkey.uFlags = 0;
					if (pOwner && pOwner->GetOwner())
						pOwner->GetOwner()->SendMessage( WM_NOTIFY,(WPARAM)pOwner->GetDlgCtrlID(),(LPARAM)&nmkey );
				}
				return TRUE;
			case VK_TAB:
				break;
			default:
				pOwner->SendMessage(WM_USER_ON_EDITKEYDOWN, nChar);
				return TRUE;
				;
		}
	}
	else if (pMsg->message == WM_LBUTTONDOWN)
	{
		::PeekMessage(pMsg, NULL, NULL, NULL, PM_REMOVE);
		GetOwner()->SendMessage(WM_USER_ON_EDITCLICK);
	}
	else if(pMsg->message == WM_MOUSEWHEEL)
	{
		if (GetOwner() && GetOwner()->GetOwner())
			GetOwner()->GetOwner()->SendMessage( WM_MOUSEWHEEL,pMsg->wParam,pMsg->lParam );
		return TRUE;
	}
	
	return CEdit::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceCBEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	NMCLICK nmclick;
	nmclick.hdr.code = NM_CLICK;
	nmclick.hdr.hwndFrom = GetSafeHwnd();
	nmclick.hdr.idFrom = GetDlgCtrlID();
	nmclick.pt = point;
	nmclick.dwHitInfo = 0;
	nmclick.dwItemData = 0;
	nmclick.dwItemSpec = 0;
	if (GetOwner())
		GetOwner()->SendMessage( WM_NOTIFY,(WPARAM)GetDlgCtrlID(),(LPARAM)&nmclick );
}

BEGIN_MESSAGE_MAP(CInPlaceCBEdit, CEdit)
	//{{AFX_MSG_MAP(CInPlaceCBEdit)
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceCBEdit message handlers

BOOL CInPlaceCBEdit::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CInPlaceCBListBox

CInPlaceCBListBox::CInPlaceCBListBox()
{
	m_pScrollBar = 0;
	m_nLastTopIdx = 0;
}

CInPlaceCBListBox::~CInPlaceCBListBox()
{
}

BEGIN_MESSAGE_MAP(CInPlaceCBListBox, CListBox)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceCBListBox message handlers


void CInPlaceCBListBox::OnKillFocus(CWnd* pNewWnd)
{
	CListBox::OnKillFocus(pNewWnd);

	ProcessSelected(false);
}

void CInPlaceCBListBox::ProcessSelected(bool bProcess)
{
	//ReleaseCapture();

	CWnd* pOwner = GetOwner();

	if(bProcess)
	{
		int nSelectedItem = GetCurSel();
		pOwner->SendMessage(WM_USER_ON_SELECTION_OK, nSelectedItem, GetItemData(nSelectedItem));
	}
	else
		pOwner->SendMessage(WM_USER_ON_SELECTION_CANCEL);
}

void CInPlaceCBListBox::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CListBox::OnLButtonDown(nFlags, point);
	/*

	CRect rect;
	GetClientRect(rect);

	if(!rect.PtInRect(point))
		ProcessSelected(false);
		*/
}

void CInPlaceCBListBox::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CListBox::OnLButtonUp(nFlags, point);

	CRect rect;
	GetClientRect(rect);

	if(rect.PtInRect(point))
		ProcessSelected();
	//else
	//	ReleaseCapture();
}



int CInPlaceCBListBox::SetCurSel(int nPos)
{
	int nRet = CListBox::SetCurSel( nPos );

	//
	// Check if we have autoscrolled
	//if( m_nLastTopIdx != GetTopIndex() )
	{
		int nDiff = m_nLastTopIdx - GetTopIndex();
		m_nLastTopIdx = GetTopIndex();

		SCROLLINFO info;
		info.cbSize = sizeof(SCROLLINFO);
		if (m_pScrollBar->GetScrollInfo( &info, SIF_ALL ) )
		{
			info.nPos = m_nLastTopIdx;
			info.fMask |= SIF_DISABLENOSCROLL;
			m_pScrollBar->SetScrollInfo( &info );
		}
	}
	return nRet;
}


//////////////////////////////////////////////////////////////////////////
void CInPlaceCBListBox::OnMouseMove(UINT nFlags, CPoint point) 
{
	CRect rcClient;
	GetClientRect( rcClient );
	if( !rcClient.PtInRect( point ) )
	{
		//if (point.x < rcClient.left || point.y > rcClient.right)
			return;
	}

	// Set selection item under mouse
	int nPos = point.y / GetItemHeight(0) + GetTopIndex();
	if (GetCurSel() != nPos)
	{
		SetCurSel( nPos );
	}

	CListBox::OnMouseMove(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
BOOL CInPlaceCBListBox::OnMouseWheel( UINT nFlags,short zDelta,CPoint pt )
{
	int nDiff = -zDelta / 60;
	SCROLLINFO info;
	info.cbSize = sizeof(SCROLLINFO);
	if (m_pScrollBar->GetScrollInfo( &info, SIF_ALL ) )
	{
		info.nPos += nDiff;
		info.fMask |= SIF_DISABLENOSCROLL;
		m_pScrollBar->SetScrollInfo( &info );
		SetTopIdx( info.nPos );
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceCBListBox::OnRButtonDown(UINT nFlags, CPoint point)
{
	CListBox::OnRButtonDown(nFlags, point);

	ProcessSelected(false);
}

BOOL CInPlaceCBListBox::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
			case VK_RETURN:
				ProcessSelected();
				return TRUE;
			case VK_ESCAPE:
			case VK_TAB:
				ProcessSelected(false);
				return TRUE;
			default:
				;
		}
	}
	if(pMsg->message == WM_SYSKEYDOWN)
	{
		ProcessSelected(false);
		return FALSE;
	}

	return CListBox::PreTranslateMessage(pMsg);
}

int CInPlaceCBListBox::GetBottomIndex()
{
	int nTop = GetTopIndex();
	CRect rc;
	GetClientRect( &rc );
	int nVisCount = 0;
	if (GetCount() > 0)
	{
		int nItemH = GetItemHeight(0);
		if (nItemH > 0)
			nVisCount = rc.Height() / nItemH;
	}
	return nTop + nVisCount;
}

void CInPlaceCBListBox::SetTopIdx(int nPos, BOOL bUpdateScrollbar)
{
	m_nLastTopIdx = nPos;
	SetTopIndex( nPos );
	if( bUpdateScrollbar )
	{
		SCROLLINFO info;
		info.cbSize = sizeof(SCROLLINFO);
		if( m_pScrollBar->GetScrollInfo( &info, SIF_ALL ) )
		{
			info.nPos = m_nLastTopIdx;
			info.fMask |= SIF_DISABLENOSCROLL;
			m_pScrollBar->SetScrollInfo( &info );
		}
	}
}



//////////////////////////////////////////////////////////////////////////
// CInPlaceCBScrollBar
/////////////////////////////////////////////////////////////////////////////
CInPlaceCBScrollBar::CInPlaceCBScrollBar()
{
	m_pListBox = 0;
}

CInPlaceCBScrollBar::~CInPlaceCBScrollBar()
{
}

BEGIN_MESSAGE_MAP(CInPlaceCBScrollBar, CScrollBar)
	//{{AFX_MSG_MAP(CInPlaceCBScrollBar)
	ON_WM_MOUSEMOVE()
	ON_WM_VSCROLL_REFLECT()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceCBScrollBar message handlers

void CInPlaceCBScrollBar::OnMouseMove(UINT nFlags, CPoint point) 
{
	//
	// Is mouse within listbox
	CRect rcClient;
	GetClientRect( rcClient );
	if( !rcClient.PtInRect( point ) )
	{
	}

	CScrollBar::OnMouseMove(nFlags, point);
}

void CInPlaceCBScrollBar::VScroll(UINT nSBCode, UINT nPos) 
{
	// TODO: Add your message handler code here
	if( !m_pListBox )
		return;

	int nTop = m_pListBox->GetTopIndex();
	int nBottom = m_pListBox->GetBottomIndex();

	SCROLLINFO info;

	info.cbSize = sizeof(SCROLLINFO);
	if( !GetScrollInfo( &info, SIF_ALL ) )
		return;

	switch( nSBCode )
	{
	case SB_BOTTOM: // Scroll to bottom.
		break;

	case SB_ENDSCROLL: // End scroll.
		break;

	case SB_LINEDOWN: // Scroll one line down.
		info.nPos++;
		if( info.nPos > info.nMax )
			info.nPos = info.nMax;
		m_pListBox->SetTopIdx( info.nPos );
		break;

	case SB_LINEUP: // Scroll one line up.
		info.nPos--;
		if( info.nPos < info.nMin )
			info.nPos = info.nMin;
		m_pListBox->SetTopIdx( info.nPos );
		break;

	case SB_PAGEDOWN: // Scroll one page down.
		info.nPos += info.nPage;
		if( info.nPos > info.nMax )
			info.nPos = info.nMax;
		m_pListBox->SetTopIdx( info.nPos );
		break;

	case SB_PAGEUP: // Scroll one page up.
		info.nPos -= info.nPage;
		if( info.nPos < info.nMin )
			info.nPos = info.nMin;
		m_pListBox->SetTopIdx( info.nPos );
		break;

	case SB_THUMBPOSITION: // Scroll to the absolute position. The current position is provided in nPos.
		info.nPos = nPos;
		m_pListBox->SetTopIdx( info.nPos );
		break;

	case SB_THUMBTRACK: // Drag scroll box to specified position. The current position is provided in nPos.
		info.nPos = nPos;
		m_pListBox->SetTopIdx( info.nPos );
		break;

	case SB_TOP: // Scroll to top. 
		break;

	}
	info.fMask |= SIF_DISABLENOSCROLL;
	SetScrollInfo( &info );
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceCBScrollBar::SetListBox( CInPlaceCBListBox* pListBox )
{
	ASSERT( pListBox != NULL );

	m_pListBox = pListBox;
	int nTop = m_pListBox->GetTopIndex();
	int nBottom = m_pListBox->GetBottomIndex();

	SCROLLINFO info;

	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	info.nMax = m_pListBox->GetCount()-1;
	info.nMin = 0;
	info.nPage = nBottom - nTop;
	info.nPos = 0;
	info.nTrackPos = 0;

	SetScrollInfo( &info );
}

void CInPlaceCBScrollBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRect rc;
	GetClientRect( &rc );
	if( !rc.PtInRect( point ) )
	{
		GetOwner()->SendMessage(WM_USER_ON_SELECTION_CANCEL);
	}

	CScrollBar::OnLButtonDown(nFlags, point);
}


//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CInPlaceComboBox
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

int CInPlaceComboBox::m_nButtonDx = ::GetSystemMetrics(SM_CXHSCROLL);

IMPLEMENT_DYNAMIC(CInPlaceComboBox, CWnd)

CInPlaceComboBox::CInPlaceComboBox()
{
	m_bReadOnly = false;
	m_nCurrentSelection = -1;
}

BEGIN_MESSAGE_MAP(CInPlaceComboBox, CWnd)
	//{{AFX_MSG_MAP(CInPlaceComboBox)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_USER_ON_SELECTION_OK, OnSelectionOk)
	ON_MESSAGE(WM_USER_ON_SELECTION_CANCEL, OnSelectionCancel)
	ON_MESSAGE(WM_USER_ON_NEW_SELECTION, OnNewSelection)
	ON_MESSAGE(WM_USER_ON_EDITCHANGE, OnEditChange)
	ON_MESSAGE(WM_USER_ON_OPENDROPDOWN, OnOpenDropDown)
	ON_MESSAGE(WM_USER_ON_EDITKEYDOWN, OnEditKeyDown)
	ON_MESSAGE(WM_USER_ON_EDITCLICK, OnEditClick)
	
	//}}AFX_MSG_MAP
	ON_WM_MOVE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceComboBox message handlers

int CInPlaceComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if(CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CRect rect;
	GetClientRect(rect);
	rect.right -= m_nButtonDx;

	CWnd* pParent = GetParent();
	ASSERT(pParent != NULL);

	CFont* pFont = pParent->GetFont();

	int flags = 0;
	if (m_bReadOnly)
		flags |= WS_VISIBLE;//|ES_READONLY; // not needed.
	else
		flags |= WS_VISIBLE;

	m_wndEdit.Create(WS_CHILD|ES_AUTOHSCROLL|flags, rect, this, 2);
	m_wndEdit.SetOwner(this);
	m_wndEdit.SetFont(pFont);

	m_minListWidth = m_nButtonDx;

	rect.right += m_nButtonDx - 1;
	rect.top = rect.bottom + 2;
	rect.bottom += 100;

	CString myClassName = AfxRegisterWndClass(CS_VREDRAW|CS_HREDRAW, ::LoadCursor(NULL, IDC_ARROW), NULL, NULL);

	m_wndDropDown.CreateEx( 0,myClassName,0,WS_POPUP|WS_BORDER,rect,pParent,0 );
	//m_wndDropDown.ModifyStyleEx( 0,/*WS_EX_TOOLWINDOW|*/WS_EX_TOPMOST );
	
	rect.right -= m_nButtonDx;
	int nListStyle = lpCreateStruct->style | LBS_DISABLENOSCROLL|LBS_HASSTRINGS|LBS_NOTIFY;
	m_wndList.Create( nListStyle, rect, &m_wndDropDown, 0);
	m_wndList.SetOwner(this);
	m_wndList.SetFont(pFont);

	rect.right += m_nButtonDx;
	m_scrollBar.Create( SBS_VERT|SBS_RIGHTALIGN|WS_CHILD|WS_VISIBLE,rect,&m_wndDropDown,1 );
	m_scrollBar.ShowWindow(SW_SHOW);

	m_wndList.SetScrollBar(&m_scrollBar);
	m_scrollBar.SetListBox(&m_wndList);

	/*
	m_wndDropDown.Create( 0,0,WS_BORDER|WS_CHILD|LBS_DISABLENOSCROLL|LBS_HASSTRINGS|LBS_NOTIFY,rect,GetDesktopWindow(),0 );
	m_wndDropDown.SetFont(pFont);
	m_wndList.SetFont(pFont);
	m_wndList.SetOwner(this);
	m_wndDropDown.SetOwner(this);
	*/
	
	return 0;
}

void CInPlaceComboBox::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	if (m_wndEdit)
		m_wndEdit.SetWindowPos(NULL, 0, 0, cx - m_nButtonDx, cy, SWP_NOZORDER|SWP_NOMOVE);
}

void CInPlaceComboBox::MoveControl(CRect& rect)
{
	CRect prevRect;
	GetClientRect(prevRect);

	CWnd* pParent = GetParent();

	ClientToScreen(prevRect);
	pParent->ScreenToClient(prevRect);
	pParent->InvalidateRect(prevRect);

	MoveWindow(rect, FALSE);
}

void CInPlaceComboBox::GetDropDownRect( CRect &rect )
{
	CRect rcEdit;
	GetWindowRect(rcEdit);

	// get the monitor containing the control
	HMONITOR hMonitor = MonitorFromRect(rcEdit, MONITOR_DEFAULTTONEAREST);

	// get the work area of that monitor
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	int itemHeight = m_wndList.GetItemHeight(0);
	int nItems = m_wndList.GetCount();
	int nListBoxHeight = nItems > 0 ? 2+nItems*itemHeight : 40; // 2 extra pixels for the frame

	int spaceAbove = max(0l, rcEdit.top-1 - mi.rcWork.top);
	int spaceBelow = max(0l, mi.rcWork.bottom - rcEdit.bottom+1);

	if ( spaceAbove > spaceBelow && spaceBelow < 100 && spaceBelow < nListBoxHeight )
	{
		// show the list above
		rect.bottom = rcEdit.top-1;
		rect.top = max(rect.bottom - nListBoxHeight, mi.rcWork.top);

		// preserve integral height
		rect.top += (rect.Height()-2) % itemHeight;
	}
	else
	{
		// show the list below
		rect.top = rcEdit.bottom+1;
		rect.bottom = min(rect.top + nListBoxHeight, mi.rcWork.bottom);

		// preserve integral height
		rect.bottom -= (rect.Height()-2) % itemHeight;
	}

	// set the width
	rect.left = max(rcEdit.left, mi.rcWork.left);
	rect.right = min(rcEdit.right, mi.rcWork.right);

	int minWidth = m_minListWidth + m_nButtonDx;

	// try expanding on left if needed
	if (rect.Width() < minWidth)
		rect.left -= minWidth - rect.Width();
	if (rect.left < mi.rcWork.left)
		rect.left = mi.rcWork.left;

	// try expanding on right if needed
	if (rect.Width() < minWidth)
		rect.right += minWidth - rect.Width();
	if (rect.right > mi.rcWork.right)
		rect.right = mi.rcWork.right;
}

void CInPlaceComboBox::ResetListBoxHeight()
{
	/*
	CRect rect;

	GetClientRect(rect);
	rect.right -= 1;

	int nItems = m_wndList.GetCount();
	int nListBoxHeight = nItems > 0 ? nItems * m_nButtonDx : DEFAULT_IPLISTBOX_HEIGHT;

	if(nListBoxHeight > DEFAULT_IPLISTBOX_HEIGHT)
		nListBoxHeight = DEFAULT_IPLISTBOX_HEIGHT;
	*/
}

void CInPlaceComboBox::OnPaint() 
{
	CPaintDC dc(this);
	
	CRect rect;

	GetClientRect(rect);
	rect.left = rect.right - m_nButtonDx;

	dc.DrawFrameControl(rect, DFC_SCROLL, m_wndDropDown.IsWindowVisible() ? 
		DFCS_SCROLLDOWN|DFCS_PUSHED : DFCS_SCROLLDOWN);
}

BOOL CInPlaceComboBox::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;	
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceComboBox::OpenDropDownList()
{
	ResetListBoxHeight();

	CRect rc;
	GetDropDownRect(rc);
	m_wndDropDown.SetWindowPos( &wndTopMost, rc.left,rc.top,rc.Width(),rc.Height(), SWP_SHOWWINDOW );
	//m_wndDropDown.MoveWindow( rc );
	m_wndDropDown.GetClientRect(rc);

	rc.right-=m_nButtonDx;
	m_wndList.MoveWindow(rc);

	rc.left = rc.right;
	rc.right += m_nButtonDx;
	m_scrollBar.MoveWindow(rc);
	
	//CRect rect;
	//GetDropDownRect(rect);
	//m_wndDropDown.SetWindowPos( &wndTopMost, rect.left,rect.top,rect.Width(),rect.Height(), SWP_SHOWWINDOW );

	m_wndDropDown.ShowWindow( SW_SHOW );
	m_scrollBar.ShowWindow( SW_SHOW );

	CRect rectButton(rc);
	rectButton.left = rectButton.right - m_nButtonDx;
	InvalidateRect(rectButton, FALSE);

	m_scrollBar.SetListBox(&m_wndList);

	CString str;
	m_wndEdit.GetWindowText(str);
	SelectString(str);

	m_wndList.SetFocus();
	//m_wndList.SetCapture();
}

void CInPlaceComboBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDown(nFlags, point);

	CRect rect;
	GetClientRect(rect);

	CRect rectButton(rect);
	rectButton.left = rectButton.right - m_nButtonDx;

	if(rectButton.PtInRect(point))
	{
		int nDoAction = m_wndDropDown.IsWindowVisible() ? SW_HIDE : SW_SHOW;

		if (nDoAction == SW_SHOW)
		{
			OpenDropDownList();
		}
		else
		{
			m_wndDropDown.ShowWindow(nDoAction);
			InvalidateRect(rectButton, FALSE);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceComboBox::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
	
	if (m_wndEdit)
		m_wndEdit.SetFocus();
}

void CInPlaceComboBox::HideListBox()
{
	if (GetCapture())
		ReleaseCapture();
	m_wndDropDown.ShowWindow(SW_HIDE);

	CRect rectButton;

	GetClientRect(rectButton);
	rectButton.left = rectButton.right - m_nButtonDx;

	InvalidateRect(rectButton, FALSE);

	if (m_wndEdit)
		m_wndEdit.SetFocus();
}

//////////////////////////////////////////////////////////////////////////
LRESULT CInPlaceComboBox::OnSelectionOk(WPARAM wParam, LPARAM /*lParam*/)
{
	HideListBox();

	SetCurSelToEdit(m_nCurrentSelection = int(wParam));

	if (m_updateCallback)
		m_updateCallback();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
LRESULT CInPlaceComboBox::OnSelectionCancel(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	HideListBox();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
LRESULT CInPlaceComboBox::OnEditChange(WPARAM wParam, LPARAM lParam)
{
	HideListBox();
	if (m_updateCallback)
		m_updateCallback();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
LRESULT CInPlaceComboBox::OnOpenDropDown(WPARAM wParam, LPARAM /*lParam*/)
{
	OpenDropDownList();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
LRESULT CInPlaceComboBox::OnEditKeyDown(WPARAM wParam, LPARAM /*lParam*/)
{
	if (!m_wndDropDown.IsWindowVisible())
		OpenDropDownList();

	char nChar = wParam;
	const char str[] = { nChar,'\0' };

	int nSel = m_wndList.FindString( -1,str );
	if (nSel != LB_ERR)
    m_wndList.SetCurSel(nSel);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
LRESULT CInPlaceComboBox::OnEditClick(WPARAM wParam, LPARAM /*lParam*/)
{
	/*
	if (!m_wndDropDown.IsWindowVisible())
		OpenDropDownList();
	*/
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
LRESULT CInPlaceComboBox::OnNewSelection(WPARAM wParam, LPARAM /*lParam*/)
{
	if (!m_wndDropDown.IsWindowVisible())
		OpenDropDownList();


	int nItems = m_wndList.GetCount();

	if(nItems > 0)
	{
		if(wParam == VK_UP)
		{
			if(m_nCurrentSelection > 0)
				SetCurSel(m_nCurrentSelection - 1);
		}
		else
		{
			if(m_nCurrentSelection < nItems - 1)
				SetCurSel(m_nCurrentSelection + 1);
		}
	}

	return TRUE;
}

void CInPlaceComboBox::SetCurSelToEdit(int nSelect)
{
	CString strText;

	if(nSelect != -1)
		m_wndList.GetText(nSelect, strText);
		
	if (m_wndEdit)
	{
		m_wndEdit.SetWindowText(strText);
		m_wndEdit.SetSel(0, -1); 
	}
}

int CInPlaceComboBox::GetCount() const
{
	return m_wndList.GetCount();
}

int CInPlaceComboBox::SetCurSel(int nSelect, bool bSendSetData)
{
	if(nSelect >= m_wndList.GetCount() || nSelect < 0)
		return CB_ERR;

	int nRet = m_wndList.SetCurSel(nSelect);

	if(nRet != -1)
	{
		SetCurSelToEdit(nSelect);
		m_nCurrentSelection = nSelect;

		if(bSendSetData)
		{
			if (m_updateCallback)
				m_updateCallback();
		}
	}

	return nRet;
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceComboBox::SelectString( LPCTSTR pStrText )
{
	if (m_wndEdit)
		m_wndEdit.SetWindowText(pStrText);
	int sel = m_wndList.FindString( -1,pStrText );
	if (sel != LB_ERR)
	{
		SetCurSel(sel,false);
	}
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceComboBox::SelectString( int nSelectAfter,LPCTSTR pStrText )
{
	if (m_wndEdit)
		m_wndEdit.SetWindowText(pStrText);
	int sel = m_wndList.FindString( nSelectAfter,pStrText );
	if (sel != LB_ERR)
	{
		SetCurSel(sel,false);
	}
}

//////////////////////////////////////////////////////////////////////////
CString CInPlaceComboBox::GetSelectedString()
{
	CString str;
	if (m_wndEdit)
		m_wndEdit.GetWindowText( str );
	return str;
}

CString CInPlaceComboBox::GetTextData() const
{
	CString strText;

	if(m_nCurrentSelection != -1)
		m_wndList.GetText(m_nCurrentSelection, strText);

	return strText;
}

int CInPlaceComboBox::AddString(LPCTSTR pStrText, DWORD nData)
{
	int nIndex = m_wndList.AddString(pStrText);

	CDC *dc = GetDC();
	HGDIOBJ hOldFont = dc->SelectObject(EditorSettings::Get().Gui.hSystemFont);
	CSize size = dc->GetOutputTextExtent( pStrText );
	dc->SelectObject(hOldFont);
	ReleaseDC(dc);

	if (size.cx+6 > m_minListWidth)
		m_minListWidth = size.cx+6;

	m_scrollBar.SetListBox( &m_wndList );

	return m_wndList.SetItemData(nIndex, nData);
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceComboBox::ResetContent()
{
	m_wndList.ResetContent();

	m_nCurrentSelection = -1;
}

//////////////////////////////////////////////////////////////////////////
void CInPlaceComboBox::OnMove(int x, int y)
{
	CWnd::OnMove(x, y);

	if (m_wndDropDown.m_hWnd)
	{
		CRect rect;
		//GetDropDownRect(rect);

		//m_wndDropDown.MoveWindow( rect );

		m_wndDropDown.GetClientRect(rect);

		rect.right -= m_nButtonDx;
		m_wndList.MoveWindow(rect);
		
		rect.left = rect.right+1;
		rect.right += m_nButtonDx;
		m_scrollBar.MoveWindow(rect);
	}
}
