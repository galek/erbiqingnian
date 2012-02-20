
/********************************************************************
	日期:		2012年2月20日
	文件名: 	RollupCtrl.h
	创建者:		王延兴
	描述:		收缩控件
*********************************************************************/

#pragma once

#include "ColorCtrl.h"
#include <afxtempl.h>

#define  ROLLUPCTRLN_EXPAND (0x0001)

struct CRollupCtrlNotify
{
	NMHDR hdr;
	int nPageId;
	bool bExpand;
};

struct RC_PAGEINFO 
{
	CWnd*					pwndTemplate;
	CButton*				pwndButton;
	CColorCtrl<CButton>*	pwndGroupBox;
	BOOL					bExpanded;
	BOOL					bEnable;
	BOOL					bAutoDestroyTpl;
	WNDPROC 				pOldDlgProc;
	WNDPROC 				pOldButProc;
	int						id;
};

#define	RC_PGBUTTONHEIGHT		18
#define	RC_SCROLLBARWIDTH		12
#define	RC_GRPBOXINDENT			6
const COLORREF	RC_SCROLLBARCOLOR = ::GetSysColor(COLOR_BTNFACE);
#define RC_ROLLCURSOR			MAKEINTRESOURCE(32649)	

//Popup Menu Ids
#define	RC_IDM_EXPANDALL		0x100
#define	RC_IDM_COLLAPSEALL		0x101
#define	RC_IDM_STARTPAGES		0x102

//////////////////////////////////////////////////////////////////////////

class CRollupCtrl : public CWnd
{
	DECLARE_DYNCREATE(CRollupCtrl)

public:

	CRollupCtrl();

	virtual 						~CRollupCtrl();

	BOOL							Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	int								InsertPage(LPCTSTR caption, CDialog* pwndTemplate, BOOL bAutoDestroyTpl=TRUE, int idx=-1,BOOL bAutoExpand=TRUE);

	int								InsertPage(LPCTSTR caption, UINT nIDTemplate, CRuntimeClass* rtc, int idx=-1);

	void							RemovePage(int idx);

	void							RemoveAllPages();

	void							ExpandPage(int idx, BOOL bExpand=TRUE,BOOL bScrool=TRUE,BOOL bFromUI=FALSE);

	void							ExpandAllPages(BOOL bExpand=TRUE,BOOL bFromUI=FALSE);

	void							EnablePage(int idx, BOOL bEnable=TRUE);

	void							EnableAllPages(BOOL bEnable=TRUE);

	int								GetPagesCount()		{ return m_PageList.size(); }

	RC_PAGEINFO*					GetPageInfo(int idx);

	void							ScrollToPage(int idx, BOOL bAtTheTop=TRUE);

	int								MovePageAt(int idx, int newidx);

	BOOL							IsPageExpanded(int idx);

	BOOL							IsPageEnabled(int idx);

	void							SetBkColor( COLORREF bkColor );

protected:

	void							RecalLayout();

	void							RecalcHeight();

	int								GetPageIdxFromButtonHWND(HWND hwnd);

	void							_RemovePage(int idx);

	void							_ExpandPage(RC_PAGEINFO* pi, BOOL bExpand,BOOL bFromUI );

	void							_EnablePage(RC_PAGEINFO* pi, BOOL bEnable);

	int								_InsertPage(LPCTSTR caption, CDialog* dlg, int idx, BOOL bAutoDestroyTpl,BOOL bAutoExpand=TRUE);

	void							DestroyAllPages();

	RC_PAGEINFO*					FindPage( int id );

	int								FindPageIndex( int id );

	CString							m_strMyClass;

	std::vector<RC_PAGEINFO*>		m_PageList;

	int								m_nStartYPos, m_nPageHeight;

	int								m_nOldMouseYPos, m_nSBOffset;

	int								m_lastId;

	bool							m_bRecalcLayout;

	std::map<CString,bool>			m_expandedMap;

	CBrush							m_bkgBrush;

	COLORREF						m_bkColor;

	static LRESULT CALLBACK 		DlgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK 		ButWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	virtual BOOL					OnCommand(WPARAM wParam, LPARAM lParam);

	afx_msg void 					OnDestroy();
	afx_msg void 					OnPaint();
	afx_msg void 					OnSize(UINT nType, int cx, int cy);
	afx_msg void 					OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void 					OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void 					OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL 					OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg int						OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void					OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg HBRUSH					OnCtlColor(CDC*, CWnd* pWnd, UINT);
	DECLARE_MESSAGE_MAP()
};
