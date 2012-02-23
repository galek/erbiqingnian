
/********************************************************************
	日期:		2012年2月23日
	文件名: 	ViewTitleDialog.h
	创建者:		王延兴
	描述:		视图上方信息栏	
*********************************************************************/

#pragma once

#include "ColorCheckBox.h"

class CLayoutViewPane;

//////////////////////////////////////////////////////////////////////////
class CViewportTitleDlg : public CXTResizeDialog
{
	DECLARE_DYNAMIC(CViewportTitleDlg)

public:
	CViewportTitleDlg(CWnd* pParent = NULL);

	virtual ~CViewportTitleDlg();

	void 				SetViewPane( CLayoutViewPane *pViewPane );

	void 				SetTitle( const CString &title );

	void 				SetViewportSize( int width,int height );

	void 				SetActive( bool bActive );

	enum { IDD = IDD_VIEWPORT_TITLE };

protected:
	virtual BOOL 		OnInitDialog();

	virtual void 		DoDataExchange(CDataExchange* pDX);

	afx_msg void 		OnSetFocus( CWnd* );

	afx_msg void 		OnButtonSetFocus();

	afx_msg void 		OnLButtonDblClk(UINT nFlags, CPoint point);

	afx_msg void 		OnRButtonDown(UINT nFlags, CPoint point);

	afx_msg void 		OnTitle();

	afx_msg void 		OnMaximize();

	afx_msg void 		OnHideHelpers();

	afx_msg void 		OnSize( UINT nType,int cx,int cy );

	DECLARE_MESSAGE_MAP()

	CString				m_title;
	CCustomButton		m_maxBtn;
	CColorCheckBox		m_hideHelpersBtn;
	CStatic				m_sizeTextCtrl;
	CLayoutViewPane*	m_pViewPane;
	CStatic				m_titleBtn;
};