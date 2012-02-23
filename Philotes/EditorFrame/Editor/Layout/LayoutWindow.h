
/********************************************************************
	日期:		2012年2月22日
	文件名: 	LayoutWindow.h
	创建者:		王延兴
	描述:		视图布局窗口
*********************************************************************/

#pragma once

#include "InfoBar.h"
#include "Viewport.h"

enum EViewLayout
{
	ET_Layout0 = 0,
	ET_Layout1,
	ET_Layout2,
	ET_Layout3,
	ET_Layout4,
	ET_Layout5,
	ET_Layout6,
	ET_Layout7,
	ET_Layout8,

	MAX_VIEWPORTS
};

//////////////////////////////////////////////////////////////////////////
class CLayoutViewPane;

class CLayoutSplitter : public CSplitterWnd
{
	DECLARE_DYNCREATE(CLayoutSplitter)
public:
	CLayoutSplitter();

public:
	virtual ~CLayoutSplitter();

protected:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

	void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg);

	void CreateLayoutView( int row,int col,int id,CCreateContext* pContext );
	void CreateSplitView( int row,int col,ESplitType splitType,CCreateContext* pContext );

private:
	friend class CLayoutWnd;
};

//////////////////////////////////////////////////////////////////////////

class CLayoutWnd : public CWnd
{
	DECLARE_DYNCREATE(CLayoutWnd)
public:
	CLayoutWnd();

	virtual				~CLayoutWnd();

public:
	CLayoutViewPane*	GetViewPane( int id );

	void				MaximizeViewport( int paneId );

	void				CreateLayout( EViewLayout layout,bool bBindViewports=true,EViewportType defaultView=ET_ViewportCamera );

	EViewLayout 		GetLayout() const { return m_layout; }

	void 				SaveConfig();

	bool 				LoadConfig();

	CLayoutViewPane*	FindViewByClass( const CString &viewClassName );

	void				ShowViewports( bool bShow );

	void				BindViewport( CLayoutViewPane *vp,CString viewClassName,CWnd *pViewport=NULL );

	CString				ViewportTypeToClassName( EViewportType viewType );

protected:
	afx_msg int			OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void 		OnSize(UINT nType, int cx, int cy);

	afx_msg void 		OnDestroy();

	DECLARE_MESSAGE_MAP()

	void 				BindViewports();

	void 				UnbindViewports();

	void 				CreateSubSplitView( int row,int col,EViewLayout splitType,CCreateContext* pContext );

	void 				CreateLayoutView( CLayoutSplitter *wndSplitter,int row,int col,int id,EViewportType viewType,CCreateContext* pContext );

	bool 				CycleViewport( EViewportType from,EViewportType to );

private:
	bool				m_bMaximized;

	EViewLayout			m_layout;

	CString				m_viewType[MAX_VIEWPORTS];

	CLayoutSplitter*	m_splitWnd;

	CLayoutSplitter*	m_splitWnd2;

	CLayoutViewPane*	m_maximizedView;

	int					m_maximizedViewId;

	CInfoBar			m_infoBar;

	CSize				m_infoBarSize;
};