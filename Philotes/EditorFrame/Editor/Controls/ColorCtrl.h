
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	ColorCtrl.h
	������:		������
	����:		ģ����ɫ������	
*********************************************************************/

#pragma once

#include "TemplateMessage.h"
#include "Settings.h"

enum
{
	CC_BLINK_NOCHANGE = 0,
	CC_BLINK_FAST     = 500,
	CC_BLINK_NORMAL   = 1000,
	CC_BLINK_SLOW     = 2000
};

enum
{
	CC_BLINK_TEXT = 1,
	CC_BLINK_BK   = 2,
	CC_BLINK_BOTH = CC_BLINK_TEXT|CC_BLINK_BK
};

#define CC_SYSCOLOR(ind) (0x80000000|((ind) & ~CLR_DEFAULT))

template<class BASE_TYPE = CWnd>
class CColorCtrl : public BASE_TYPE
{
public:
	CColorCtrl();
	virtual ~CColorCtrl(){}

public:

	void		SetTextColor(COLORREF rgbText = CLR_DEFAULT);
	COLORREF	GetTextColor(){ return GetColor(m_rgbText); }
	void		SetTextBlinkColors(COLORREF rgbBlinkText1, COLORREF rgbBlinkText2);

	void		SetBkColor(COLORREF rgbBk = CLR_DEFAULT);
	COLORREF	GetBkColor(){ return GetColor(m_rgbBk); }
	void		SetBkBlinkColors(COLORREF rgbBlinkBk1, COLORREF rgbBlinkBk2);

	void 		SetBkPattern(UINT nBkID = 0);
	void 		SetBkPattern(HBITMAP hbmpBk = 0);
	void 		SetBkBlinkPattern(UINT nBkID1, UINT nBkID2);
	void 		SetBkBlinkPattern(HBITMAP hbmpBk1, HBITMAP hbmpBk2);

	void 		StartBlink(int iWho = CC_BLINK_BOTH, UINT nDelay = CC_BLINK_NOCHANGE);
	void 		StopBlink(int iWho = CC_BLINK_BOTH);
	UINT 		GetDelay() { return m_nDelay; }

	void 		UseSolidColors(BOOL fSolid = TRUE);

	void 		ForceOpaque(){m_fForceOpaque = TRUE;}

	void 		SetBlinkTimer()
	{
		if(m_nTimerID <= 0 && (m_fBlinkText || m_fBlinkBk))
			m_nTimerID = SetTimer(1, m_nDelay, NULL);
	}

	void		KillBlinkTimer()
	{
		KillTimer(m_nTimerID);
		m_nTimerID = 0;
		m_iBlinkPhase = 0;
	}

	COLORREF	GetColor(COLORREF clr)
	{
		if(clr == CLR_NONE)
			return clr;
		DWORD mask = clr & CLR_DEFAULT;
		if(mask == 0x80000000)
			return ::GetSysColor(clr & ~CLR_DEFAULT);
		if(mask == CLR_DEFAULT)
			return CLR_DEFAULT;
		return clr & ~CLR_DEFAULT;
	}

	UINT		m_nTimerID;

	int			m_iBlinkPhase;

	UINT 		m_nDelay;

	BOOL 		m_fBlinkText;

	BOOL 		m_fBlinkBk;

	BOOL 		m_fSolid;

	BOOL 		m_fForceOpaque;

	COLORREF	m_rgbText;

	COLORREF	m_rgbBlinkText[2];

	COLORREF	m_rgbBk;

	CBitmap		m_bmpBk;

	COLORREF	m_rgbBlinkBk[2];

	CBitmap		m_bmpBlinkBk[2];

	CBrush		m_brBk;

public:

	protected:
	virtual void	PreSubclassWindow();

	afx_msg HBRUSH	CtlColor(CDC* pDC, UINT nCtlColor);

	afx_msg HBRUSH	OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	afx_msg void	OnDestroy();

	afx_msg void	OnTimer(UINT_PTR nIDEvent);

	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);

	DECLARE_TEMPLATE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
typedef CColorCtrl<CEdit>      CColoredEdit;
typedef CColorCtrl<CButton>    CColoredButton;
typedef CColorCtrl<CStatic>    CColoredStatic;
typedef CColorCtrl<CScrollBar> CColoredScrollBar;
typedef CColorCtrl<CListBox>   CColoredListBox;
typedef CColorCtrl<CComboBox>  CColoredComboBox;
typedef CColorCtrl<CDialog>    CColoredDialog;
/////////////////////////////////////////////////////////////////////////////

template<class BASE_TYPE>
CColorCtrl<BASE_TYPE>::CColorCtrl()
{
	m_nTimerID = 0;
	m_iBlinkPhase = 0;
	m_nDelay = CC_BLINK_NORMAL;

	m_fSolid = FALSE;
	m_fForceOpaque = FALSE;

	m_fBlinkText = FALSE;
	m_fBlinkBk = FALSE;

	m_rgbText = CLR_DEFAULT;
	m_rgbBlinkText[0] = m_rgbText;
	m_rgbBlinkText[1] = m_rgbText;

	m_rgbBk = CLR_DEFAULT;
	m_rgbBlinkBk[0] = m_rgbBk;
	m_rgbBlinkBk[1] = m_rgbBk;
}

BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(class BASE_TYPE, CColorCtrl<BASE_TYPE>, BASE_TYPE)
	//{{AFX_MSG_MAP(CColorCtrl)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_TEMPLATE_MESSAGE_MAP_CUSTOM()


template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::SetTextColor(COLORREF rgbText)
{
	m_rgbText = rgbText;
	if(::IsWindow(m_hWnd))
		Invalidate();
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::SetTextBlinkColors(COLORREF rgbBlinkText1, COLORREF rgbBlinkText2)
{
	m_rgbBlinkText[0] = rgbBlinkText1;
	m_rgbBlinkText[1] = rgbBlinkText2;
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::SetBkColor(COLORREF rgbBk)
{
	m_rgbBk = rgbBk;

	if(::IsWindow(m_hWnd))
		Invalidate();
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::SetBkBlinkColors(COLORREF rgbBlinkBk1, COLORREF rgbBlinkBk2)
{
	m_rgbBlinkBk[0] = rgbBlinkBk1;
	m_rgbBlinkBk[1] = rgbBlinkBk2;
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::SetBkPattern(UINT nBkID)
{
	m_bmpBk.DeleteObject();
	if(nBkID > 0)
		m_bmpBk.LoadBitmap(nBkID);

	if(::IsWindow(m_hWnd))
		Invalidate();
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::SetBkPattern(HBITMAP hbmpBk)
{
	m_bmpBk.DeleteObject();
	if(hbmpBk != 0)
		m_bmpBk.Attach(hbmpBk);

	if(::IsWindow(m_hWnd))
		Invalidate();
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::SetBkBlinkPattern(UINT nBkID1, UINT nBkID2)
{
	m_bmpBlinkBk[0].DeleteObject();
	m_bmpBlinkBk[1].DeleteObject();

	if(nBkID1 > 0)
		m_bmpBlinkBk[0].LoadBitmap(nBkID1);
	if(nBkID2 > 0)
		m_bmpBlinkBk[1].LoadBitmap(nBkID2);
}


template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::SetBkBlinkPattern(HBITMAP hbmpBk1, HBITMAP hbmpBk2)
{
	m_bmpBlinkBk[0].DeleteObject();
	m_bmpBlinkBk[1].DeleteObject();

	if(hbmpBk1 != 0)
		m_bmpBlinkBk[0].Attach(hbmpBk1);
	if(hbmpBk2 != 0)
		m_bmpBlinkBk[1].Attach(hbmpBk2);
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::UseSolidColors(BOOL fSolid)
{
	m_fSolid = fSolid;

	if(::IsWindow(m_hWnd))
		Invalidate();
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::StartBlink(int iWho, UINT nDelay)
{
	if(iWho & CC_BLINK_TEXT)
		m_fBlinkText = TRUE;
	if(iWho & CC_BLINK_BK)
		m_fBlinkBk = TRUE;

	if(nDelay != CC_BLINK_NOCHANGE)
	{
		m_nDelay = nDelay;
		if (m_nTimerID > 0)
		{
			ASSERT(::IsWindow(m_hWnd));
			// reset old timer if delay changed
			KillBlinkTimer();
		}
	}
	ASSERT(m_fBlinkText||m_fBlinkBk);
	if(!::IsWindow(m_hWnd))
		return;
	// if no timer - set it
	SetBlinkTimer();
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::StopBlink(int iWho)
{
	if(iWho & CC_BLINK_TEXT)
		m_fBlinkText = FALSE;
	if(iWho & CC_BLINK_BK)
		m_fBlinkBk = FALSE;

	if(m_nTimerID > 0 && !m_fBlinkText && !m_fBlinkBk)
	{
		ASSERT(::IsWindow(m_hWnd));
		KillBlinkTimer();
		Invalidate();
	}
}

template<class BASE_TYPE>
HBRUSH CColorCtrl<BASE_TYPE>::CtlColor(CDC* pDC, UINT nCtlColor)
{
	COLORREF rgbText = GetColor(m_fBlinkText ? m_rgbBlinkText[m_iBlinkPhase] 
	                                         : m_rgbText);
	COLORREF rgbBk = GetColor(m_fBlinkBk ? m_rgbBlinkBk[m_iBlinkPhase] 
	                                     : m_rgbBk);
	
	CBitmap& bmpBk = m_fBlinkBk ? m_bmpBlinkBk[m_iBlinkPhase] : m_bmpBk;
	
	if(rgbText == CLR_DEFAULT && rgbBk == CLR_DEFAULT && !bmpBk.GetSafeHandle())
		return 0;

	if(rgbBk == CLR_DEFAULT)
	{
		switch(nCtlColor)
		{
			case CTLCOLOR_EDIT:
			case CTLCOLOR_LISTBOX:
				rgbBk = GetSysColor(COLOR_WINDOW);
				break;
			case CTLCOLOR_BTN:
			case CTLCOLOR_DLG:
			case CTLCOLOR_MSGBOX:
			case CTLCOLOR_STATIC:
			default:
				rgbBk = ::GetSysColor(COLOR_BTNFACE);
				break;
			case CTLCOLOR_SCROLLBAR:
				return 0; 
		}
	}
	if(rgbText == CLR_DEFAULT)
	{
		switch(nCtlColor)
		{
			default:
				rgbText = ::GetSysColor(COLOR_WINDOWTEXT);
				break;
			case CTLCOLOR_BTN:
				rgbText = ::GetSysColor(COLOR_BTNTEXT);
				break;
		}
	}

	ASSERT(rgbText != CLR_DEFAULT);
	ASSERT(rgbBk != CLR_DEFAULT);
	
	if(m_fSolid)
	{
		if(rgbBk != CLR_NONE)
			rgbBk = ::GetNearestColor(pDC->m_hDC, rgbBk);
		rgbText = ::GetNearestColor(pDC->m_hDC, rgbText);
	}
	
	if(m_fForceOpaque && rgbBk != CLR_NONE)
		pDC->SetBkMode(OPAQUE);
	else
		pDC->SetBkMode(TRANSPARENT);
	
	pDC->SetTextColor(rgbText);
	pDC->SetBkColor(rgbBk);
	
	m_brBk.DeleteObject();
	if(bmpBk.GetSafeHandle())
		m_brBk.CreatePatternBrush(&bmpBk);
	else if(rgbBk != CLR_NONE)
		m_brBk.CreateSolidBrush(rgbBk);
	else
		return (HBRUSH) ::GetStockObject (HOLLOW_BRUSH);

	return (HBRUSH)m_brBk;
}

template<class BASE_TYPE>
HBRUSH CColorCtrl<BASE_TYPE>::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if(pWnd->m_hWnd == m_hWnd)
		return BASE_TYPE::OnCtlColor(pDC, pWnd, nCtlColor);
	return BASE_TYPE::OnCtlColor(pDC, this, nCtlColor);	
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::OnDestroy()
{
	KillBlinkTimer();
	BASE_TYPE::OnDestroy();
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != m_nTimerID)
	{
		BASE_TYPE::OnTimer(nIDEvent);
		return;
	}

	if (m_fBlinkBk || m_fBlinkText)
	{
		m_iBlinkPhase = !m_iBlinkPhase;
		Invalidate();
	}
}

template<class BASE_TYPE>
void CColorCtrl<BASE_TYPE>::PreSubclassWindow() 
{
	SetBlinkTimer();
	BASE_TYPE::PreSubclassWindow();
}

template<class BASE_TYPE>
int CColorCtrl<BASE_TYPE>::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (BASE_TYPE::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetBlinkTimer();
	return 0;
}

template<class BASE_TYPE, int InitialTextColor = CLR_DEFAULT, int InitialBkColor = CLR_DEFAULT>
class CColorCtrlEx : public CColorCtrl<BASE_TYPE>
{
public:
	CColorCtrlEx()
	{
		m_rgbText = InitialTextColor;
		m_rgbBk = InitialBkColor;
	}
};

template<class BASE_TYPE, 
         int InitialTextColor0 = CLR_DEFAULT, 
				 int InitialTextColor1 = CLR_DEFAULT, 
				 int InitialBkColor0 = CLR_DEFAULT,
				 int InitialBkColor1 = CLR_DEFAULT,
				 int InitialDelay = CC_BLINK_NORMAL>
class CBlinkCtrlEx : public CColorCtrl<BASE_TYPE>
{
public:
	CBlinkCtrlEx()
	{
		m_nDelay = InitialDelay;
		m_rgbBlinkText[0] = InitialTextColor0;
		m_rgbBlinkText[1] = InitialTextColor1;
		if(InitialTextColor0 != CLR_DEFAULT || InitialTextColor1 != CLR_DEFAULT)
			m_fBlinkText = TRUE;
		
		m_rgbBlinkBk[0] = InitialBkColor0;
		m_rgbBlinkBk[1] = InitialBkColor1;
		if(InitialBkColor0 != CLR_DEFAULT || InitialBkColor1 != CLR_DEFAULT)
			m_fBlinkBk = TRUE;
	}
};

class CDynamicToolTipCtrl : public CToolTipCtrl
{
public:
	~CDynamicToolTipCtrl() {};
};

template<class BASE_TYPE>
class CColorPushButton : public BASE_TYPE
{
public:
	CColorPushButton();
	~CColorPushButton();
public:

	protected:
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	
public:
	void SetNoFocus(bool fNoFocus = true){m_fNoFocus = fNoFocus;}
	void SetNoDisabledState(bool fNoDisabledState = true) {m_fNoDisabledState = fNoDisabledState;}
	void SetPushedBkColor(COLORREF rgbBk = CLR_DEFAULT);
	COLORREF GetPushedBkColor(){ return GetColor(m_rgbBk); }

	void SetIcon( HICON hIcon,int nIconAlign=BS_CENTER,bool bDrawText=false );
	void SetIcon( LPCSTR lpszIconResource,int nIconAlign=BS_CENTER,bool bDrawText=false );
	void SetToolTip( const char *tooltipText );

protected:
	virtual void ChangeStyle();

	UINT			m_fNoFocus			: 1;

	UINT			m_fNoDisabledState	: 1;

	UINT			m_bDrawText			: 1;

	UINT			m_bOwnerDraw		: 1;

	COLORREF		m_rgbPushedBk;

	CBrush			m_pushedBk;

	int m_nIconAlign;
	HICON m_hIcon;
	CDynamicToolTipCtrl *m_tooltip;
	
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	

	DECLARE_TEMPLATE_MESSAGE_MAP()
};

BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(class BASE_TYPE, CColorPushButton<BASE_TYPE>, BASE_TYPE)
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
END_TEMPLATE_MESSAGE_MAP_CUSTOM()

template<class BASE_TYPE>
CColorPushButton<BASE_TYPE>::CColorPushButton()
{
	m_fNoFocus = true;
	m_fNoDisabledState = false;
	m_hIcon = 0;
	m_tooltip=0;
	m_bDrawText = true;
	m_bOwnerDraw = false;
	m_nIconAlign = BS_CENTER;
	m_pushedBk.CreateSolidBrush( ::GetSysColor(COLOR_HIGHLIGHT) );
}

template<class BASE_TYPE>
CColorPushButton<BASE_TYPE>::~CColorPushButton()
{
	if (m_tooltip)
		delete m_tooltip;
	m_tooltip = 0;
	if (m_hIcon)
		DestroyIcon(m_hIcon);
}

template<class BASE_TYPE>
void CColorPushButton<BASE_TYPE>::SetToolTip( const char *tooltipText )
{
	if (!m_tooltip)
	{
		m_tooltip = new CDynamicToolTipCtrl;
		m_tooltip->Create( this );
		CRect rc;
		GetClientRect(rc);
		m_tooltip->AddTool( this,tooltipText,rc,1 );
	}
	else
	{
		m_tooltip->UpdateTipText( tooltipText,this,1 );
	}
	m_tooltip->Activate(TRUE);

}

template<class BASE_TYPE>
void CColorPushButton<BASE_TYPE>::SetIcon( LPCSTR lpszIconResource,int nIconAlign,bool bDrawText )
{
	if (m_hIcon)
		DestroyIcon(m_hIcon);
	m_hIcon = (HICON)::LoadImage( AfxGetInstanceHandle(),lpszIconResource,IMAGE_ICON,0,0,0 );
	SetIcon( m_hIcon,nIconAlign,bDrawText );
}

template<class BASE_TYPE>
void CColorPushButton<BASE_TYPE>::SetIcon( HICON hIcon,int nIconAlign,bool bDrawText )
{
	m_nIconAlign = nIconAlign;
	m_bDrawText = bDrawText;
	m_hIcon = hIcon;

	CString sCaption;
	GetWindowText(sCaption);
	SetToolTip(sCaption);

	ChangeStyle();
	Invalidate(FALSE);
};

template<class BASE_TYPE>
void CColorPushButton<BASE_TYPE>::SetPushedBkColor(COLORREF rgbBk)
{
	m_rgbPushedBk = rgbBk;
	m_pushedBk.DeleteObject();
	m_pushedBk.CreateSolidBrush( m_rgbPushedBk );
	if(::IsWindow(m_hWnd))
		Invalidate();
}

template<class BASE_TYPE>
BOOL CColorPushButton<BASE_TYPE>::PreTranslateMessage(MSG* pMsg)
{
	if (m_tooltip && m_tooltip->m_hWnd != 0)
		m_tooltip->RelayEvent(pMsg);

	if (pMsg->message == WM_PAINT)
	{
		if (EditorSettings::Get().Gui.bWindowsVista || EditorSettings::Get().Gui.bSkining)
		{
			if (m_bOwnerDraw && !m_hIcon)
			{
				ChangeStyle();
				Invalidate(TRUE);
			}
		}
		else
		{
			if (!m_bOwnerDraw)
			{
				ChangeStyle();
				Invalidate(TRUE);
			}
		}
	}

	return BASE_TYPE::PreTranslateMessage(pMsg);
}

template<class BASE_TYPE>
void CColorPushButton<BASE_TYPE>::PreSubclassWindow() 
{
	if (!EditorSettings::Get().Gui.bWindowsVista && !EditorSettings::Get().Gui.bSkining) // Use standard buttons on windows vista.
		ChangeStyle();
	BASE_TYPE::PreSubclassWindow();
}

template<class BASE_TYPE>
int CColorPushButton<BASE_TYPE>::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (BASE_TYPE::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (!EditorSettings::Get().Gui.bWindowsVista || !EditorSettings::Get().Gui.bSkining) // Use standard buttons on windows vista.
		ChangeStyle();
	return 0;
}

template<class BASE_TYPE>
void CColorPushButton<BASE_TYPE>::OnLButtonDown(UINT nFlags, CPoint point)
{
	BASE_TYPE::OnLButtonDown(nFlags,point);
	if (GetParent())
		GetParent()->SendMessage( (UINT)WM_COMMAND,(WPARAM)MAKEWPARAM(GetDlgCtrlID(),BN_PUSHED),(LPARAM)m_hWnd );
}

template<class BASE_TYPE>
void CColorPushButton<BASE_TYPE>::ChangeStyle()
{
	CString sClassName;
	GetClassName(m_hWnd, sClassName.GetBuffer(10), 10);
	sClassName.ReleaseBuffer();
	DWORD style = GetStyle();
	ASSERT(sClassName.CompareNoCase(_T("BUTTON")) == 0); // must be used only with buttons
	ASSERT((style & (BS_BITMAP)) == 0); // dont supports bitmap buttons
	//ASSERT((style & (BS_ICON|BS_BITMAP)) == 0); // dont supports icon/bitmap buttons
	if ((style & 0xf) == BS_OWNERDRAW && m_bOwnerDraw)
	{
		if ((EditorSettings::Get().Gui.bWindowsVista || EditorSettings::Get().Gui.bSkining) && !m_hIcon)
		{
			m_bOwnerDraw = false;
			ModifyStyle(0xf, BS_PUSHBUTTON);
		}
	}
	if((style & 0xf) == BS_PUSHBUTTON || 
	   (style & 0xf) == BS_DEFPUSHBUTTON)
	{
		if ((!EditorSettings::Get().Gui.bWindowsVista && !EditorSettings::Get().Gui.bSkining) || m_hIcon)
		{
			m_bOwnerDraw = true;
			ModifyStyle(0xf, BS_OWNERDRAW);
		}
	}	
}

template<class BASE_TYPE>
void CColorPushButton<BASE_TYPE>::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect	rc(lpDrawItemStruct->rcItem);
	UINT state = lpDrawItemStruct->itemState;
	CString sCaption;
	GetWindowText(sCaption);
	DWORD style = GetStyle();

	CRect rcFocus(rc);
	rcFocus.DeflateRect(4, 4);

	int iSavedDC = pDC->SaveDC();

	if(((state & ODS_DEFAULT) || (state & ODS_FOCUS)) && !m_fNoFocus)
	{
		pDC->Draw3dRect(rc, GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_WINDOWFRAME));
		rc.DeflateRect(1, 1);
	}

	if(style & BS_FLAT)
	{
		pDC->Draw3dRect(rc, GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_WINDOWFRAME));
		rc.DeflateRect(1, 1);
		pDC->Draw3dRect(rc, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOW));
		rc.DeflateRect(1, 1);
	}
	else
	{
		if(state & ODS_SELECTED)
		{
			pDC->Draw3dRect(rc, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DSHADOW));
			rc.DeflateRect(1, 1);
		}
		else
		{
			pDC->Draw3dRect(rc, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DDKSHADOW));
			rc.DeflateRect(1, 1);
		}
	}

	// ��䱳��
	HBRUSH brush = (HBRUSH)SendMessage(GetSysColor(COLOR_BTNFACE), (WPARAM)pDC->m_hDC, (LPARAM)m_hWnd);

	if(state & ODS_SELECTED)
	{
		pDC->SetBkMode(TRANSPARENT);
		brush = m_pushedBk;
	}

	if(brush)
		::FillRect(pDC->m_hDC, rc, brush);

	if (m_hIcon)
	{
		int w = GetSystemMetrics(SM_CXICON);
		int h = GetSystemMetrics(SM_CYICON);
		int x;
		int y = rc.top + rc.Height()/2 - h/2;
		if (m_nIconAlign == BS_LEFT)
		{
			x = rc.left;
		}
		else if (m_nIconAlign == BS_RIGHT)
		{
			x = rc.right - w;
		}
		else
		{
			x = rc.left + rc.Width()/2 - w/2+1;
		}
		if(state & ODS_SELECTED)
		{
			x += 1; y += 1;
		}
		pDC->DrawIcon( CPoint(x,y),m_hIcon );
	}

	if (m_bDrawText)
	{
		// ��������
		UINT fTextStyle = 0;
		switch(style & BS_CENTER)
		{
		case BS_LEFT : fTextStyle |= DT_LEFT; break;
		case BS_RIGHT : fTextStyle |= DT_RIGHT; break;
		default:
		case BS_CENTER : fTextStyle |= DT_CENTER; break;
		}
		switch(style & BS_VCENTER)
		{
		case BS_TOP : fTextStyle |= DT_TOP; break;
		case BS_BOTTOM : fTextStyle |= DT_BOTTOM; break;
		default:
		case BS_VCENTER : fTextStyle |= DT_VCENTER; break;
		}
		if(!(style & BS_MULTILINE)) fTextStyle |= DT_SINGLELINE;
		else fTextStyle |= DT_WORDBREAK;

		if(state & ODS_DISABLED && !m_fNoDisabledState)
		{
			pDC->SetBkMode(TRANSPARENT);
			rc.DeflateRect(2, 2, 0, 0);
			pDC->SetTextColor(GetSysColor(COLOR_3DHILIGHT));
			pDC->DrawText(sCaption, rc, fTextStyle);
			rc.OffsetRect(-1, -1);
			pDC->SetTextColor(GetSysColor(COLOR_3DSHADOW));
			pDC->DrawText(sCaption, rc, fTextStyle);	
		}
		else
		{
			if(state & ODS_SELECTED)
				rc.DeflateRect(2, 2, 0, 0);

			pDC->DrawText(sCaption, rc, fTextStyle);
		}
	}

	pDC->RestoreDC(iSavedDC);
	
	if ((state & ODS_FOCUS) && !m_fNoFocus)
		pDC->DrawFocusRect(rcFocus);
}