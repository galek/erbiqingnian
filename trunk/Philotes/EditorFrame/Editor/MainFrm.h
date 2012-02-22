
/********************************************************************
	日期:		2012年2月20日
	文件名: 	MainFrm.h
	创建者:		王延兴
	描述:		主框架类	
*********************************************************************/

#pragma once

#include "RollupBar.h"

class CMainFrame : public CXTPFrameWnd, public CXTPOffice2007FrameHook
{
protected:
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	virtual ~CMainFrame();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:

	CXTPDockingPaneManager*		GetDockingPaneManager() { return &m_paneManager; }

protected: 

	CXTPStatusBar				m_wndStatusBar;

	CXTPDockingPaneManager		m_paneManager;

	CRollupBar					m_wndRollUp;

	CRollupCtrl 				m_objectRollupCtrl;

	CRollupCtrl 				m_terrainRollupCtrl;

	class CTerrainPanel*		m_terrainPanel;

protected:

	afx_msg int					OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void				OnClose();

	afx_msg void				OnCustomize();

	afx_msg LRESULT				OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:

	void						CreateRollUpBar();

	void						SwitchTheme( XTPPaintTheme paintTheme,XTPDockingPanePaintTheme paneTheme );

	void						LoadElecBoxSkin();

	void						OnSkinChanged();
};
