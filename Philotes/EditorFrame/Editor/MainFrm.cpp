// MainFrm.cpp : implementation of the CMainFrame class
//

#include "Editor.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////

#define  IDW_VIEW_EDITMODE_BAR			AFX_IDW_CONTROLBAR_FIRST+10
#define  IDW_VIEW_OBJECT_BAR			AFX_IDW_CONTROLBAR_FIRST+11
#define  IDW_VIEW_MISSION_BAR			AFX_IDW_CONTROLBAR_FIRST+12
#define  IDW_VIEW_TERRAIN_BAR			AFX_IDW_CONTROLBAR_FIRST+13
#define  IDW_VIEW_AVI_RECORD_BAR		AFX_IDW_CONTROLBAR_FIRST+14

#define  IDW_VIEW_ROLLUP_BAR			AFX_IDW_CONTROLBAR_FIRST+20
#define  IDW_VIEW_CONSOLE_BAR			AFX_IDW_CONTROLBAR_FIRST+21
#define  IDW_VIEW_INFO_BAR				AFX_IDW_CONTROLBAR_FIRST+22
#define  IDW_VIEW_TRACKVIEW_BAR			AFX_IDW_CONTROLBAR_FIRST+23
#define  IDW_VIEW_DIALOGTOOL_BAR		AFX_IDW_CONTROLBAR_FIRST+24
#define  IDW_VIEW_DATABASE_BAR			AFX_IDW_CONTROLBAR_FIRST+25

//////////////////////////////////////////////////////////////////////////


// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(XTP_ID_CUSTOMIZE, OnCustomize)
	ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, OnDockingPaneNotify)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

static UINT uHideCmds[] =
{
	ID_FILE_PRINT,
	ID_FILE_PRINT_PREVIEW,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	if (!InitCommandBars())
		return -1;

	CXTPCommandBars* pCommandBars = GetCommandBars();
	if(pCommandBars == NULL)
	{
		TRACE0("Failed to create command bars object.\n");
		return -1;
	}

	CXTPCommandBar* pMenuBar = pCommandBars->SetMenu(
		_T("Menu Bar"), IDR_MAINFRAME);
	if(pMenuBar == NULL)
	{
		TRACE0("Failed to create menu bar.\n");
		return -1;
	}

	CXTPToolBar* pToolBar = (CXTPToolBar*)
		pCommandBars->Add(_T("Standard"), xtpBarTop);
	if (!pToolBar || !pToolBar->LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;
	}

	CXTPPaintManager::SetTheme(xtpThemeOffice2003);

	pCommandBars->HideCommands(uHideCmds, _countof(uHideCmds));
	pCommandBars->GetCommandBarsOptions()->bAlwaysShowFullMenus = FALSE;
	pCommandBars->GetShortcutManager()->SetAccelerators(IDR_MAINFRAME);
	LoadCommandBars(_T("CommandBars"));

	m_paneManager.InstallDockingPanes(this);
	m_paneManager.SetTheme(xtpPaneThemeOffice2003);

	CXTPDockingPaneLayout layoutNormal(&m_paneManager);
	if (layoutNormal.Load(_T("NormalLayout")))
	{
		m_paneManager.SetLayout(&layoutNormal);
	}

	CreateRollUpBar();

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.style |= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
	return TRUE;
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif

void CMainFrame::OnClose()
{
	CFrameWnd::OnClose();
}

void CMainFrame::OnCustomize()
{
}

LRESULT CMainFrame::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		return TRUE;
	}
	return FALSE;
}

void CMainFrame::CreateRollUpBar()
{
	CXTPDockingPane* pDockRollup = GetDockingPaneManager()->CreatePane( IDW_VIEW_ROLLUP_BAR,CRect(0,0,220,500),xtpPaneDockLeft);
	pDockRollup->SetTitle( _T("¹¤¾ß") );

	m_wndRollUp.Create( NULL,NULL,WS_CHILD|WS_VISIBLE,CRect(0,0,1,1),this,0 );

	m_objectRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
	m_wndRollUp.SetRollUpCtrl( 0,&m_objectRollupCtrl,"Create" );

	m_terrainRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
	m_wndRollUp.SetRollUpCtrl( 1,&m_terrainRollupCtrl,"Terrain Editing" );


	m_wndRollUp.Select(0);
}
