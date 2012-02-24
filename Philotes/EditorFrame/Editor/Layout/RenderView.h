
#pragma once

#include "Viewport.h"

_NAMESPACE_BEGIN

class CRenderView : public CViewport
{
	DECLARE_DYNCREATE(CRenderView)

public:

	CRenderView();

	virtual					~CRenderView();

public:

	virtual EViewportType	GetType() const { return ET_ViewportCamera; }

	virtual void			SetType( EViewportType type ) { assert(type == ET_ViewportCamera); }

	virtual float			GetAspectRatio() const;

	virtual void			Update();

	virtual void			OnRender();

	void					OnPaintSafe();

protected:

	bool					m_bUpdating;

protected:

	afx_msg int				OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void 			OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL 			OnEraseBkgnd(CDC* pDC);
	afx_msg void 			OnPaint();
	afx_msg void 			OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void 			OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void 			OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void 			OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void 			OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void 			OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void 			OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void 			OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL 			OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void 			OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL 			OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void 			OnDestroy();

	DECLARE_MESSAGE_MAP()
};

_NAMESPACE_END