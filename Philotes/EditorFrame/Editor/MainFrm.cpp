
#include "Editor.h"

#include "MainFrm.h"
#include "LocalTheme.h"
#include "LayoutWindow.h"
#include "TerrainPanel.h"

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

//////////////////////////////////////////////////////////////////////////

namespace
{
	XTPPaintTheme s_themePaint				= xtpThemeOffice2003;
	XTPDockingPanePaintTheme s_themePane	= xtpPaneThemeOffice2003;
	BOOL s_bDockingHelpers					= TRUE;
	CString s_lpszSkinPath					= "Styles\\ElecDark.cjstyles";
}

#define STYLES_PATH			"Styles\\"
#define LAYOUTS_PATH		"Layouts\\"
#define LAYOUTS_EXTENSION	".layout"
#define LAYOUTS_WILDCARD	"*.layout"
#define DUMMY_LAYOUT_NAME	"Dummy_Layout"

//////////////////////////////////////////////////////////////////////////

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	CXTRegistryManager regMgr;
	s_themePaint		= (XTPPaintTheme)regMgr.GetProfileInt(_T("Settings"), _T("PaintTheme"), s_themePaint );
	s_themePane			= (XTPDockingPanePaintTheme)regMgr.GetProfileInt(_T("Settings"), _T("PaneTheme"), s_themePane);
	s_lpszSkinPath		= regMgr.GetProfileString(_T("Settings"), _T("SkinPath"), s_lpszSkinPath);;
	s_bDockingHelpers	= regMgr.GetProfileInt(_T("Settings"), _T("DockingHelpers"), s_bDockingHelpers );

	m_layoutWnd = NULL;

	EditorSettings::Get().Gui.bSkining = TRUE;
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

	// 主题/皮肤
	SwitchTheme(s_themePaint,s_themePane);

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
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (!pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDW_VIEW_ROLLUP_BAR:
				pwndDockWindow->Attach(&m_wndRollUp);
				break;
			}
		}
		return TRUE;
	}
	return FALSE;
}

void CMainFrame::CreateRollUpBar()
{
	CXTPDockingPane* pDockRollup = GetDockingPaneManager()->CreatePane( IDW_VIEW_ROLLUP_BAR,CRect(0,0,220,500),xtpPaneDockLeft);
	pDockRollup->SetTitle( _T("工具") );

	m_wndRollUp.Create( NULL,NULL,WS_CHILD|WS_VISIBLE,CRect(0,0,1,1),this,0 );

// 	m_objectRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
// 	m_wndRollUp.SetRollUpCtrl( 0,&m_objectRollupCtrl,"Create" );

	m_terrainRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
	m_wndRollUp.SetRollUpCtrl( 0,&m_terrainRollupCtrl,"地形编辑" );

	m_terrainPanel = new CTerrainPanel(this);
	m_terrainRollupCtrl.InsertPage("地形",m_terrainPanel );

	m_wndRollUp.Select(0);
}

void CMainFrame::SwitchTheme( XTPPaintTheme paintTheme,XTPDockingPanePaintTheme paneTheme )
{
	if (paintTheme == xtpThemeOffice2007 || paintTheme == xtpThemeRibbon)
	{
		paintTheme = xtpThemeOffice2007;
		if (XTPResourceImages()->SetHandle( Path::Make(STYLES_PATH,"Office2007Blue.dll") ))
		{
			CXTPPaintManager::SetTheme(paintTheme);
			EnableOffice2007Frame(GetCommandBars());
			GetCommandBars()->GetPaintManager()->m_bEnableAnimation = TRUE;
		}
		else
		{
			paintTheme = xtpThemeNativeWinXP;
		}
	}
	else
	{
		if (s_themePaint == xtpThemeRibbon)
		{
			EnableOffice2007Frame(0);
		}

		CXTPPaintManager::SetTheme(paintTheme);
	}


	if (EditorSettings::Get().Gui.bSkining)
	{
		LoadElecBoxSkin();
	}
	else
	{
		XTPSkinManager()->LoadSkin( "","" );
		XTPSkinManager()->SetApplyOptions( 0 );
	}
	s_themePaint = paintTheme;
	s_themePane = paneTheme;

	XTP_COMMANDBARS_ICONSINFO* pIconsInfo = XTPPaintManager()->GetIconsInfo();

	if (GetDockingPaneManager())
	{
		GetDockingPaneManager()->SetTheme(paneTheme);
	}

	XTPPaintManager()->RefreshMetrics();
	GetCommandBars()->GetPaintManager()->RefreshMetrics();

	RecalcLayout(FALSE);
	if (GetCommandBars())
		GetCommandBars()->RedrawCommandBars();

	RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );
	RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );

	GetDockingPaneManager()->RedrawPanes();

	//if (m_layoutWnd)
	//{
	//	CWnd* pWnd = CWnd::FromHandle(*m_layoutWnd)->GetWindow(GW_CHILD);
	//	while(pWnd)
	//	{
	//		pWnd->RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );
	//		pWnd = pWnd->GetWindow(GW_HWNDNEXT);
	//	}
	//}

	OnSkinChanged();
}

void CMainFrame::LoadElecBoxSkin()
{
	CXTPPaintManager::SetCustomTheme(new CLocalTheme());
	BOOL bLoaded = XTPSkinManager()->LoadSkin( s_lpszSkinPath );

	XTPSkinManager()->SetApplyOptions(xtpSkinApplyMetrics|xtpSkinApplyColors|xtpSkinApplyFrame|xtpSkinApplyMenus);
	XTPSkinManager()->SetAutoApplyNewWindows(TRUE);
	XTPSkinManager()->SetAutoApplyNewThreads(TRUE);
	XTPSkinManager()->EnableCurrentThread();
	XTPSkinManager()->ApplyWindow(this->GetSafeHwnd());

	bool test = XTPSkinManager()->IsColorFilterExists();
	XTPSkinManager()->RedrawAllControls();

	OnSkinChanged();
}

void CMainFrame::OnSkinChanged()
{
	OnSysColorChange();
	RedrawWindow(0, 0, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN);		

	XTPPaintManager()->RefreshMetrics();

	GetCommandBars()->GetPaintManager()->RefreshMetrics();
	GetCommandBars()->RedrawCommandBars();	
}

BOOL CMainFrame::OnCreateClient( LPCREATESTRUCT lpcs, CCreateContext* pContext )
{
	m_layoutWnd = new CLayoutWnd;
	CRect rc(0,0,1,1);
	m_layoutWnd->CreateEx( 0,NULL,NULL,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rc,this,AFX_IDW_PANE_FIRST );
//	if (IsPreview())
//	{
		m_layoutWnd->CreateLayout( ET_Layout0,true,ET_ViewportModel );
//	}
// 	else
// 	{
// 		if (!m_layoutWnd->LoadConfig())
// 		{
// 			m_layoutWnd->CreateLayout( ET_Layout0 );
// 		}
// 	}

	return TRUE;
}
