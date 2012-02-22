
/********************************************************************
	日期:		2012年2月22日
	文件名: 	Viewport.h
	创建者:		王延兴
	描述:		编辑器视口类	
*********************************************************************/

#pragma once

class CEditorDoc;
class CViewManager;
class CEditTool;

enum EViewportType
{
	ET_ViewportUnknown = 0,
	ET_ViewportCamera,
	ET_ViewportModel,

	ET_ViewportLast,
};

enum EStdCursor
{
	STD_CURSOR_DEFAULT,
	STD_CURSOR_HIT,
	STD_CURSOR_MOVE,
	STD_CURSOR_ROTATE,
	STD_CURSOR_SCALE,
	STD_CURSOR_SEL_PLUS,
	STD_CURSOR_SEL_MINUS,
	STD_CURSOR_SUBOBJ_SEL,
	STD_CURSOR_SUBOBJ_SEL_PLUS,
	STD_CURSOR_SUBOBJ_SEL_MINUS,

	STD_CURSOR_CRYSIS,

	STD_CURSOR_LAST,
};

#define WM_VIEWPORT_ON_TITLE_CHANGE	(WM_USER + 10)

//////////////////////////////////////////////////////////////////////////

class CViewport : public CWnd
{
	DECLARE_DYNAMIC(CViewport)

public:
	enum ViewMode
	{
		NothingMode = 0,
		ScrollZoomMode,
		ScrollMode,
		ZoomMode,
		ManipulatorDragMode,
	};

	CViewport();

	virtual					~CViewport();

public:

	virtual void			Update();

	void					SetName( const CString &name );
	
	CString					GetName() const;

	virtual void			SetType( EViewportType type ) {};

	virtual float			GetAspectRatio() const;

	virtual EViewportType	GetType() const { return ET_ViewportUnknown; }

	virtual void 			SetActive( bool bActive );
	
	virtual bool 			IsActive() const;

	virtual void 			ResetContent();

	virtual void 			UpdateContent( int flags );

	virtual void 			OnActivate();

	virtual void 			OnDeactivate();

	virtual void 			SetCursor( HCURSOR hCursor );

	virtual void			SetViewMode(ViewMode eViewMode) { m_eViewMode = eViewMode; };

	ViewMode				GetViewMode() { return m_eViewMode; };

	virtual void			OnTitleMenu( CMenu &menu ) {};

	virtual void			OnTitleMenuCommand( int id ){};

	CEditorDoc*				GetDocument();

	virtual CEditTool*		GetEditTool();

	virtual void			SetEditTool( CEditTool *pEditTool,bool bLocalToViewport=false );

protected:
	friend class			CViewManager;

	void					CreateViewportWindow();

	virtual void			PostNcDestroy();

	afx_msg void 			OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void 			OnDestroy();

	afx_msg BOOL 			OnEraseBkgnd(CDC* pDC);

	afx_msg void 			OnPaint();

	afx_msg BOOL 			OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	afx_msg void 			OnLButtonDown(UINT nFlags, CPoint point);

	afx_msg void 			OnLButtonUp(UINT nFlags, CPoint point);

	afx_msg void 			OnRButtonDown(UINT nFlags, CPoint point);

	afx_msg void 			OnRButtonUp(UINT nFlags, CPoint point);

	afx_msg void 			OnMButtonDblClk(UINT nFlags, CPoint point);

	afx_msg void 			OnMButtonDown(UINT nFlags, CPoint point);

	afx_msg void 			OnMButtonUp(UINT nFlags, CPoint point);

	afx_msg void 			OnLButtonDblClk(UINT nFlags, CPoint point);

	afx_msg void 			OnRButtonDblClk(UINT nFlags, CPoint point);

	afx_msg void 			OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	afx_msg void 			OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);

	afx_msg BOOL 			OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	DECLARE_MESSAGE_MAP()

	CMenu					m_cViewMenu;

	mutable float			m_fZoomFactor;

	bool					m_bActive;

	ViewMode				m_eViewMode;

	CPoint					m_cMouseDownPos;

	CRect					m_selectedRect;

	CRect					m_rcClient;

	HCURSOR					m_hCursor[STD_CURSOR_LAST];

	HCURSOR					m_hCurrCursor;
};