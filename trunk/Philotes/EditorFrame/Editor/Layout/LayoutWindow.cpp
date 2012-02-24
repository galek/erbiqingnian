
#include "LayoutWindow.h"
#include "EditorRoot.h"
#include "ViewPane.h"
#include "ViewManager.h"
#include "../EditorDoc.h"

_NAMESPACE_BEGIN

IMPLEMENT_DYNCREATE(CLayoutSplitter,CSplitterWnd)
IMPLEMENT_DYNCREATE(CLayoutWnd,CWnd)

CLayoutSplitter::CLayoutSplitter()
{
	m_cxSplitter = m_cySplitter = 3 + 1 + 1;
	m_cxBorderShare = m_cyBorderShare = 0;
	m_cxSplitterGap = m_cySplitterGap = 3 + 1 + 1;
	m_cxBorder = m_cyBorder = 1;
}

CLayoutSplitter::~CLayoutSplitter()
{
}

BEGIN_MESSAGE_MAP(CLayoutSplitter, CSplitterWnd)
	ON_WM_SIZE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CLayoutSplitter::OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg)
{
	if((nType != splitBorder) || (pDC == NULL))
	{
		CSplitterWnd::OnDrawSplitter(pDC, nType, rectArg);
		return;
	}

	ASSERT_VALID(pDC);

	pDC->Draw3dRect(rectArg, GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT));
}

void CLayoutSplitter::OnSize(UINT nType, int cx, int cy) 
{
	CSplitterWnd::OnSize(nType, cx, cy);

	if (m_hWnd && m_pRowInfo && m_pColInfo)
	{
		CRect rc;
		GetClientRect( rc );

		int i;
		int rows = GetRowCount();
		int cols = GetColumnCount();
		for (i = 0; i < rows; i++)
		{
			SetRowInfo( i,rc.bottom/rows,10 );
		}
		for (i = 0; i < cols; i++)
		{
			SetColumnInfo( i,rc.right/cols,10 );
		}

		RecalcLayout();
	}
}

void CLayoutSplitter::CreateLayoutView( int row,int col,int id,CCreateContext* pContext )
{
	assert( row >= 0 && row < 3 );
	assert( col >= 0 && col < 3 );
	CreateView( row,col,RUNTIME_CLASS(CLayoutViewPane),CSize(100,100),pContext );
	CLayoutViewPane *viewPane = (CLayoutViewPane*)GetPane(row,col);
	if (viewPane)
	{
		viewPane->SetId(id);
	}
}

CLayoutWnd::CLayoutWnd()
{
	m_splitWnd = 0;
	m_splitWnd2 = 0;

	m_bMaximized = false;
	m_maximizedView = 0;
	m_layout = (EViewLayout)-1;

	m_maximizedViewId = 0;
	m_infoBarSize = CSize(0,0);

	for (int i = 0; i < sizeof(m_viewType)/sizeof(m_viewType[0]); i++)
		m_viewType[i] = "";
}

CLayoutWnd::~CLayoutWnd()
{
	if (m_splitWnd)
		delete m_splitWnd;
	m_splitWnd = 0;

	if (m_splitWnd2)
		delete m_splitWnd2;
	m_splitWnd2 = 0;
}

BEGIN_MESSAGE_MAP(CLayoutWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
int CLayoutWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	int hRes = CWnd::OnCreate(lpCreateStruct);

	m_infoBar.Create( CInfoBar::IDD,this );
	m_infoBar.ShowWindow(SW_SHOW);
	return hRes;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

	if (m_hWnd)
	{
		CRect rc;
		GetClientRect( rc );

		if (m_infoBar)
		{
			int h = 26;
			m_infoBar.MoveWindow(rc.left,rc.bottom-h,rc.right,rc.bottom);

			rc.bottom -= h;

			m_infoBarSize.cx = rc.Width();
			m_infoBarSize.cy = h;
		}

		if (m_splitWnd)
		{
			m_splitWnd->MoveWindow(rc);
		}

		if (m_maximizedView)
		{
			m_maximizedView->MoveWindow(rc);
		}
	}
}

void CLayoutWnd::ShowViewports( bool bShow )
{
	int flag = SW_SHOW;
	if (!bShow)
	{
		flag = SW_HIDE;
	}
	
	for (int i = 0; i < MAX_VIEWPORTS; i++)
	{
		CLayoutViewPane *pViewPane = GetViewPane(i);
		if (pViewPane)
		{
			pViewPane->ShowWindow(flag);
		}
	}

	if (m_maximizedView)
		m_maximizedView->ShowWindow(flag);
}

void CLayoutWnd::UnbindViewports()
{
	for (int i = 0; i < MAX_VIEWPORTS; i++)
	{
		CLayoutViewPane *pViewPane = GetViewPane(i);
		if (pViewPane)
		{
			pViewPane->ReleaseViewport();
		}
	}

	if (m_maximizedView)
	{
		m_maximizedView->ReleaseViewport();
	}
}

void CLayoutWnd::BindViewports()
{
	UnbindViewports();

	for (int i = 0; i < MAX_VIEWPORTS; i++)
	{
		CLayoutViewPane *pViewPane = GetViewPane(i);
		if (pViewPane)
		{
			BindViewport( pViewPane,m_viewType[pViewPane->GetId()] );
		}
	}

	if (m_splitWnd)
		m_splitWnd->SetActivePane(0,0);

	Invalidate(FALSE);
}

void CLayoutWnd::BindViewport( CLayoutViewPane *vp,CString viewClassName,CWnd *pViewport )
{
	assert( vp );
	if (!pViewport)
	{
		vp->SetViewClass( viewClassName );
	}
	else
	{
		vp->AttachViewport(pViewport);
	}
	vp->ShowWindow( SW_SHOW );
	m_viewType[vp->GetId()] = viewClassName;
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::MaximizeViewport( int paneId )
{
	if (m_layout == ET_Layout0 && m_bMaximized)
	{
		return;
	}

	CString viewClass = m_viewType[paneId];

	CRect rc;
	GetClientRect( rc );
	if (!m_bMaximized)
	{
		CLayoutViewPane *pViewPane = GetViewPane(paneId);
		ShowViewports(false);
		m_maximizedViewId = paneId;
		m_bMaximized = true;

		if (m_maximizedView)
		{
			if (m_splitWnd)
			{
				m_splitWnd->ShowWindow( SW_HIDE );
			}
			if (m_splitWnd2)
			{
				m_splitWnd2->ShowWindow(SW_HIDE);
			}

			if (pViewPane)
			{
				BindViewport( m_maximizedView,viewClass,pViewPane->GetViewport() );
				pViewPane->DetachViewport();
			}
			else
			{
				BindViewport( m_maximizedView,viewClass );
			}

			m_maximizedView->SetFocus();

			((CFrameWnd*)AfxGetMainWnd())->SetActiveView( m_maximizedView );
		}
	}
	else
	{
		CLayoutViewPane *pViewPane = GetViewPane(m_maximizedViewId);
		m_bMaximized = false;
		m_maximizedViewId = 0;

		if (pViewPane && m_maximizedView)
		{
			BindViewport( pViewPane,viewClass,m_maximizedView->GetViewport() );
			m_maximizedView->DetachViewport();
		}

		ShowViewports(true);
		if (m_maximizedView)
			m_maximizedView->ShowWindow(SW_HIDE);

		if (m_splitWnd)
			m_splitWnd->ShowWindow(SW_SHOW);
		if (m_splitWnd2)
			m_splitWnd2->ShowWindow(SW_SHOW);

		if (m_splitWnd)
			m_splitWnd->SetActivePane(0,0);
	}

	RedrawWindow();
}

//////////////////////////////////////////////////////////////////////////
void CLayoutWnd::CreateSubSplitView( int row,int col,EViewLayout splitType,CCreateContext* pContext )
{
	assert( row >= 0 && row < 3 );
	assert( col >= 0 && col < 3 );
	//	m_viewType[row][col] = -1;

	//m_secondSplitWnd = new CLayoutWnd;
	//m_secondSplitWnd->CreateLayout( this,splitType,pCtx,IdFromRowCol(0,0) );
}

CString CLayoutWnd::ViewportTypeToClassName( EViewportType viewType )
{
	CString viewClassName = "";
	std::vector<CViewportDesc*> descriptions;
	CEditorRoot::Get().GetViewManager()->GetViewportDescriptions( descriptions );
	for (int i = 0; i < descriptions.size(); i++)
	{
		if (descriptions[i]->type == viewType && descriptions[i]->pViewClass)
			viewClassName = descriptions[i]->pViewClass->ClassName();
	}
	return viewClassName;
}

void CLayoutWnd::CreateLayoutView( CLayoutSplitter *wndSplitter,int row,int col,int id,EViewportType viewType,CCreateContext* pContext )
{
	CString viewClassName = ViewportTypeToClassName( viewType );
	wndSplitter->CreateLayoutView( row,col,id,pContext );
	m_viewType[id] = viewClassName;
}

void CLayoutWnd::CreateLayout( EViewLayout layout,bool bBindViewports,EViewportType defaultView )
{
	UnbindViewports();

	m_layout		= layout;
	m_bMaximized	= false;

	CCreateContext ctx;
	ZeroMemory(&ctx,sizeof(CCreateContext));
	ctx.m_pNewViewClass		= RUNTIME_CLASS(CLayoutViewPane);
	ctx.m_pCurrentDoc		= CEditorRoot::Get().GetDocument();
	ctx.m_pCurrentFrame		= (CFrameWnd*)AfxGetMainWnd();
	CCreateContext *pCtx	= &ctx;

	if (m_splitWnd)
	{
		m_splitWnd->ShowWindow( SW_HIDE );
		m_splitWnd->DestroyWindow();
		delete m_splitWnd;
		m_splitWnd = 0;
	}

	if (m_splitWnd2)
	{
		delete m_splitWnd2;
		m_splitWnd2 = 0;
	}

	if (m_maximizedView)
	{
		m_maximizedView->ShowWindow( SW_HIDE );
	}

	CRect rcView;
	GetClientRect(rcView);
	rcView.bottom -= m_infoBarSize.cy;

	if (!m_maximizedView)
	{
		m_maximizedView = new CLayoutViewPane;
		m_maximizedView->SetId(0);
		m_maximizedView->Create( 0,0,WS_CHILD|WS_VISIBLE,rcView,this,0,pCtx );
		m_maximizedView->ShowWindow( SW_HIDE );
		m_maximizedView->SetFullscren(true);
	}

	switch (layout)
	{
	case ET_Layout0:
		m_viewType[0] = ViewportTypeToClassName(defaultView);
		if (bBindViewports)
		{
			MaximizeViewport(0);
		}
		break;
	default:
		AfxMessageBox( _T("Trying to Create Unknown Layout"),MB_OK|MB_ICONERROR );
		break;
	};

	if (m_splitWnd)
	{
		m_splitWnd->MoveWindow(rcView);
		m_splitWnd->SetActivePane(0,0);
	}

	if (bBindViewports && !m_bMaximized)
	{
		BindViewports();
	}
}

void CLayoutWnd::SaveConfig()
{
	CWinApp *pApp = AfxGetApp();
	assert( pApp );
	pApp->WriteProfileInt( "ViewportLayout","Layout",(int)m_layout );
	pApp->WriteProfileInt( "ViewportLayout","Maximized",(int)m_maximizedViewId );

	CString str;
	for (int i = 1; i < MAX_VIEWPORTS; i++) 
	{
		CString temp;
		temp.Format( "%s,",(const char*)m_viewType[i] );
		str += temp;
	}
	pApp->WriteProfileString( "ViewportLayout","Viewports",str );
}

bool CLayoutWnd::LoadConfig()
{
	CWinApp *pApp = AfxGetApp();
	assert( pApp );
	int layout = pApp->GetProfileInt( "ViewportLayout","Layout",-1 );
	int maximizedView = pApp->GetProfileInt( "ViewportLayout","Maximized",0 );
	if (layout < 0)
	{
		return false;
	}

	CreateLayout( (EViewLayout)layout,false );

	bool bRebindViewports = false;
	if (m_splitWnd)
	{
		CString str = pApp->GetProfileString( "ViewportLayout","Viewports" );
		int nIndex = 1;
		int curPos = 0;
		CString resToken = str.Tokenize(",",curPos);
		while (resToken != "")
		{
			if (nIndex >= MAX_VIEWPORTS)
				break;
			bRebindViewports = true;
			if (!resToken.IsEmpty())
				m_viewType[nIndex] = resToken;
			resToken = str.Tokenize(",",curPos);
			nIndex++;
		};
	}

	BindViewports();

	if (maximizedView || layout == ET_Layout0)
	{
		MaximizeViewport( maximizedView );
	}

	return true;
}

CLayoutViewPane* CLayoutWnd::GetViewPane( int id )
{
	if (m_splitWnd)
	{
		for (int row = 0; row < m_splitWnd->GetRowCount(); row++)
			for (int col = 0; col < m_splitWnd->GetColumnCount(); col++)
			{
				CWnd *pWnd = m_splitWnd->GetPane(row,col);
				if (pWnd->IsKindOf(RUNTIME_CLASS(CLayoutViewPane)))
				{
					CLayoutViewPane *pane = (CLayoutViewPane*)pWnd;
					if (pane && pane->GetId() == id)
						return pane;
				}
			}
	}
	if (m_splitWnd2)
	{
		for (int row = 0; row < m_splitWnd2->GetRowCount(); row++)
			for (int col = 0; col < m_splitWnd2->GetColumnCount(); col++)
			{
				CWnd *pWnd = m_splitWnd2->GetPane(row,col);
				if (pWnd->IsKindOf(RUNTIME_CLASS(CLayoutViewPane)))
				{
					CLayoutViewPane *pane = (CLayoutViewPane*)pWnd;
					if (pane && pane->GetId() == id)
						return pane;
				}
			}
	}
	return NULL;
}

CLayoutViewPane* CLayoutWnd::FindViewByClass( const CString &viewClassName )
{
	for (int i = 1; i < MAX_VIEWPORTS; i++)
	{
		if (m_viewType[i] == viewClassName)
		{
			return GetViewPane(i);
		}
	}
	return NULL;
}

bool CLayoutWnd::CycleViewport( EViewportType from,EViewportType to )
{
	CString viewClassName = ViewportTypeToClassName(from);
	CLayoutViewPane *vp = FindViewByClass( viewClassName );
	if (m_layout == ET_Layout0 && !vp)
	{
		if (m_maximizedView)
		{
			if (m_maximizedView->GetViewClass() == viewClassName)
			{
				vp = m_maximizedView;
			}
		}
	}
	if (vp)
	{
		BindViewport( vp,ViewportTypeToClassName(to) );
		return true;
	}
	return false;
}

void CLayoutWnd::OnDestroy()
{
	if (m_maximizedView)
	{
		m_maximizedView->DestroyWindow();
		m_maximizedView = 0;
	}

	CWnd::OnDestroy();
}

_NAMESPACE_END