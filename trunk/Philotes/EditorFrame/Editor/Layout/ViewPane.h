
#pragma once

#include "ViewTitleDialog.h"

_NAMESPACE_BEGIN

class CLayoutViewPane : public CView
{
public:
	CLayoutViewPane();

	virtual ~CLayoutViewPane();

	DECLARE_DYNCREATE(CLayoutViewPane)

public:
	void				SetId( int id ) { m_id = id; }

	int					GetId() { return m_id; }

	void				SetViewClass( const CString &sClass );

	CString				GetViewClass() const;

	void				SwapViewports( CLayoutViewPane *pView );

	void				SwaphViewports( CLayoutViewPane *pView );

	void 				AttachViewport( CWnd *pViewport );

	void 				DetachViewport();

	void 				ReleaseViewport();

	void 				SetFullscren( bool f );

	bool 				IsFullscreen() const { return m_bFullscreen; };

	void 				SetFullscreenViewport( bool b );

	CWnd*				GetViewport() { return m_viewport; };

	bool 				IsActiveView() const { return m_active; };

	void 				ShowTitleMenu();

	void 				ToggleMaximize();

public:
	virtual BOOL 		OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

protected:
	virtual void 		OnDraw(CDC* pDC);

	virtual void 		OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);

	virtual void 		OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

protected:
#ifdef _DEBUG
	virtual void 		AssertValid() const;

	virtual void 		Dump(CDumpContext& dc) const;
#endif

protected:

	afx_msg int			OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void 		OnLButtonDblClk(UINT nFlags, CPoint point);

	afx_msg void 		OnSize(UINT nType, int cx, int cy);

	afx_msg BOOL 		OnEraseBkgnd(CDC* pDC);

	afx_msg void 		OnDestroy();

	afx_msg BOOL 		OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	afx_msg void 		OnRButtonDown(UINT nFlags, CPoint point);

	afx_msg void 		OnSetFocus(CWnd* pOldWnd);

	afx_msg LRESULT 	OnViewportTitleChange( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()

	void 				DrawView( CDC &dc );

	void 				RecalcLayout();

private:
	CString				m_viewPaneClass;

	bool				m_bFullscreen;

	CViewportTitleDlg	m_viewportTitleDlg;

	int 				m_id;

	int 				m_nBorder;

	int 				m_titleHeight;

	CWnd*				m_viewport;

	bool				m_active;
};

_NAMESPACE_END