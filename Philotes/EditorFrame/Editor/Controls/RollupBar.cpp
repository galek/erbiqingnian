
#include "RollupBar.h"
#include "Settings.h"

_NAMESPACE_BEGIN

CRollupBar::CRollupBar()
{
}

CRollupBar::~CRollupBar()
{
}

BEGIN_MESSAGE_MAP(CRollupBar, CWnd)
    //{{AFX_MSG_MAP(CWnd)
    ON_WM_CREATE()
	ON_NOTIFY( TCN_SELCHANGE, IDC_ROLLUPTAB, OnTabSelect )
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CRollupBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CMFCUtils::LoadTrueColorImageList( m_tabImageList,IDB_TABPANEL,22,RGB(0x0,0x0,0x0) );

	CRect rc;
	m_tabCtrl.Create( TCS_HOTTRACK|TCS_TABS|TCS_FOCUSNEVER|TCS_SINGLELINE| WS_CHILD|WS_VISIBLE,rc,this,IDC_ROLLUPTAB );
	m_tabCtrl.ModifyStyle( WS_BORDER,0,0 );
	m_tabCtrl.ModifyStyleEx( WS_EX_CLIENTEDGE|WS_EX_STATICEDGE|WS_EX_WINDOWEDGE,0,0 );
	m_tabCtrl.SetImageList( &m_tabImageList );
	m_tabCtrl.SetFont( CFont::FromHandle( (HFONT)EditorSettings::Get().Gui.hSystemFont) );
	
	m_selectedCtrl = 0;

	return 0;
}

void CRollupBar::OnSize(UINT nType, int cx, int cy)
{
	RECT rcRollUp;

	CWnd::OnSize(nType, cx, cy);

	GetClientRect(&rcRollUp);

	if (m_tabCtrl.m_hWnd)
	{
		m_tabCtrl.MoveWindow(	rcRollUp.left, rcRollUp.top,rcRollUp.right,rcRollUp.bottom,FALSE );
	}

	CRect rc;
	for (size_t i = 0; i < m_controls.size(); i++)
	{
		CRect irc;
		m_tabCtrl.GetItemRect( 0,irc );
		m_tabCtrl.GetClientRect( rc );
		if (m_controls[i]) 
		{
			rc.left += 1;
			rc.right -= 2;
			rc.top += irc.bottom-irc.top+8;
			rc.bottom -= 2;
			m_controls[i]->MoveWindow( rc,FALSE );
		}
	}
}

void CRollupBar::OnTabSelect(NMHDR* pNMHDR, LRESULT* pResult)
{
	int sel = m_tabCtrl.GetCurSel();
	Select( sel );
}

void CRollupBar::Select( int num )
{
	m_selectedCtrl = num;
	for (size_t i = 0; i < m_controls.size(); i++)
	{
		if (i == num)
		{
			m_controls[i]->ShowWindow( SW_SHOW );
		}
		else
		{
			m_controls[i]->ShowWindow( SW_HIDE );
		}
	}
}

void CRollupBar::SetRollUpCtrl( int i,CRollupCtrl *pCtrl,const char *sTooltip )
{
	pCtrl->ShowWindow(SW_HIDE);
	if (i >= (int)m_controls.size())
	{
		m_controls.resize( i+1 );
	}
	pCtrl->SetParent( &m_tabCtrl );
	m_controls[i] = pCtrl;
	if (i != m_tabCtrl.GetCurSel())
	{
		//m_controls[i]->ShowWindow( SW_HIDE );
	}
	m_tabCtrl.InsertItem( i,"",i );
	m_tabCtrl.SetCurSel(0);
}

CRollupCtrl* CRollupBar::GetCurrentCtrl()
{
	ASSERT( m_selectedCtrl < (int)m_controls.size() );
	return m_controls[m_selectedCtrl];
}

_NAMESPACE_END