
#include "NumberCtrlEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CNumberCtrlEdit

void CNumberCtrlEdit::SetText(const CString& strText)
{
	m_strInitText = strText;

	SetWindowText(strText);
}

BOOL CNumberCtrlEdit::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
			//case VK_ESCAPE:
			case VK_RETURN:
			case VK_UP:
			case VK_DOWN:
			case VK_TAB:
				//::PeekMessage(pMsg, NULL, NULL, NULL, PM_REMOVE);
				// Call update callback.
				if (m_onUpdate)
					m_onUpdate();
				//return TRUE;
			default:
				;
		}
	}
	else if(pMsg->message == WM_MOUSEWHEEL)
	{
		if (GetOwner() && GetOwner()->GetOwner())
			GetOwner()->GetOwner()->SendMessage( WM_MOUSEWHEEL,pMsg->wParam,pMsg->lParam );
		return TRUE;
	}
	
	return CEdit::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CNumberCtrlEdit, CEdit)
	//{{AFX_MSG_MAP(CNumberCtrlEdit)
	ON_WM_CREATE()
	ON_WM_KILLFOCUS()
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNumberCtrlEdit message handlers

int CNumberCtrlEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(CEdit::OnCreate(lpCreateStruct) == -1) 
		return -1;

	CFont* pFont = GetParent()->GetFont();
	SetFont(pFont);

	SetWindowText(m_strInitText);

	return 0;
}

void CNumberCtrlEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	// Call update callback.
	if (m_onUpdate)
		m_onUpdate();
}

BOOL CNumberCtrlEdit::OnEraseBkgnd(CDC* /*pDC*/) 
{
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrlEdit::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);

	SetSel(0,-1);
}

//////////////////////////////////////////////////////////////////////////
bool CNumberCtrlEdit::IsValidChar( UINT nChar )
{
	if ((nChar >= '0' && nChar <= '9') || nChar == '-' || nChar == '.' || nChar == '+' ||
			nChar == 'e' || nChar == 'E' || 
			nChar == VK_BACK || nChar == VK_LEFT || nChar == VK_RIGHT)
	{
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrlEdit::OnKeyDown( UINT nChar,UINT nRepCnt,UINT nFlags )
{
	if (IsValidChar(nChar))
	{
		CEdit::OnKeyDown(nChar,nRepCnt,nFlags);
	}
	else
	{
		NMKEY nmkey;
		nmkey.hdr.code = NM_KEYDOWN;
		nmkey.hdr.hwndFrom = GetSafeHwnd();
		nmkey.hdr.idFrom = GetDlgCtrlID();
		nmkey.nVKey = nChar;
		nmkey.uFlags = nFlags;
		if (GetParent())
			GetParent()->SendMessage( WM_NOTIFY,(WPARAM)GetDlgCtrlID(),(LPARAM)&nmkey );
	}
}

//////////////////////////////////////////////////////////////////////////
void CNumberCtrlEdit::OnChar( UINT nChar,UINT nRepCnt,UINT nFlags )
{
	if (IsValidChar(nChar))
		CEdit::OnChar(nChar,nRepCnt,nFlags);
}
