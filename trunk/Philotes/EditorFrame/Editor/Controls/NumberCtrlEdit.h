
/********************************************************************
	日期:		2012年2月20日
	文件名: 	NumberCtrlEdit.h
	创建者:		王延兴
	描述:		可拖选数字控件	
*********************************************************************/

#pragma once

#include "functor.h"

#define WMU_LBUTTONDOWN (WM_USER+100)
#define WMU_LBUTTONUP (WM_USER+101)

class CNumberCtrlEdit : public CEdit
{
	CNumberCtrlEdit(const CNumberCtrlEdit& d);

	CNumberCtrlEdit& operator=(const CNumberCtrlEdit& d);

public:
	typedef Functor0 UpdateCallback;

	CNumberCtrlEdit() {};

	void			SetText(const CString& strText);

	void			SetUpdateCallback( UpdateCallback func ) { m_onUpdate = func; }

	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	
protected:
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void	OnKillFocus(CWnd* pNewWnd);

	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);

	afx_msg void	OnSetFocus(CWnd* pOldWnd);

	afx_msg void	OnKeyDown( UINT nChar,UINT nRepCnt,UINT nFlags );

	afx_msg void	OnChar( UINT nChar,UINT nRepCnt,UINT nFlags );

	bool			IsValidChar( UINT nChar );

	DECLARE_MESSAGE_MAP()

protected:
	CString			m_strInitText;

	UpdateCallback	m_onUpdate;
};
