
/********************************************************************
	日期:		2012年2月20日
	文件名: 	InPlaceComboBox.h
	创建者:		王延兴
	描述:		嵌入式组合框控件	
*********************************************************************/

#pragma once

#include "Functor.h"

_NAMESPACE_BEGIN

class CInPlaceCBEdit : public CEdit
{
	CInPlaceCBEdit(const CInPlaceCBEdit& d);
	CInPlaceCBEdit& operator=(const CInPlaceCBEdit& d);

public:
	CInPlaceCBEdit();

	virtual ~CInPlaceCBEdit();

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
};

inline CInPlaceCBEdit::CInPlaceCBEdit()
{
}

inline CInPlaceCBEdit::~CInPlaceCBEdit()
{
}

/////////////////////////////////////////////////////////////////////////////

class CInPlaceCBListBox : public CListBox
{
	CInPlaceCBListBox(const CInPlaceCBListBox& d);

	CInPlaceCBListBox& operator=(const CInPlaceCBListBox& d);

public:
	CInPlaceCBListBox();

	virtual ~CInPlaceCBListBox();

	void		SetScrollBar( CScrollBar *sb ) { m_pScrollBar = sb; };

	int			GetBottomIndex();

	void		SetTopIdx(int nPos, BOOL bUpdateScrollbar=FALSE );

	int			SetCurSel(int nPos);

protected:
	void		ProcessSelected(bool bProcess = true);

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel( UINT nFlags,short zDelta,CPoint pt );
	afx_msg void OnKillFocus(CWnd* pNewWnd);

	DECLARE_MESSAGE_MAP()

	int			m_nLastTopIdx;
	CScrollBar* m_pScrollBar;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CInPlaceCBScrollBar : public CScrollBar
{
public:
	CInPlaceCBScrollBar();

	virtual ~CInPlaceCBScrollBar();

public:
	void SetListBox( CInPlaceCBListBox* pListBox );

protected:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void VScroll(UINT nSBCode, UINT nPos);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()

private:
	CInPlaceCBListBox* m_pListBox;
};

/////////////////////////////////////////////////////////////////////////////
// CInPlaceComboBox

class CInPlaceComboBox : public CWnd
{
	CInPlaceComboBox(const CInPlaceComboBox& d);
	CInPlaceComboBox operator=(const CInPlaceComboBox& d);

protected:
	DECLARE_DYNAMIC(CInPlaceComboBox)

public:
	typedef Functor0 UpdateCallback;

	CInPlaceComboBox();
	virtual ~CInPlaceComboBox();

	void SetUpdateCallback( UpdateCallback cb ) { m_updateCallback = cb; };
	void SetReadOnly( bool bEnable ) { m_bReadOnly = bEnable; };

public:
	int GetCurrentSelection() const;
	CString GetTextData() const;

public:
	int GetCount() const;
	int GetCurSel() const { return GetCurrentSelection(); };
	int SetCurSel(int nSelect, bool bSendSetData = true);
	void SelectString( LPCTSTR pStrText );
	void SelectString( int nSelectAfter,LPCTSTR pStrText );
	CString GetSelectedString();
	int AddString(LPCTSTR pStrText, DWORD nData = 0);

	void ResetContent();
	void ResetListBoxHeight();

	void MoveControl(CRect& rect);

private:
	void SetCurSelToEdit(int nSelect);

protected:
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnSelectionOk(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSelectionCancel(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNewSelection(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOpenDropDown(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditKeyDown(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditClick(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnSetFocus(CWnd* pOldWnd);
	afx_msg void	OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	void 			HideListBox();
	void 			GetDropDownRect( CRect &rect );
	void 			OpenDropDownList();
	
private:
	static int			m_nButtonDx;

	int					m_minListWidth;
	
	bool				m_bReadOnly;

	int					m_nCurrentSelection;
	
	UpdateCallback		m_updateCallback;

	CInPlaceCBEdit		m_wndEdit;
	
	CInPlaceCBListBox	m_wndList;

	CInPlaceCBScrollBar m_scrollBar;

	CWnd				m_wndDropDown;

public:
	afx_msg void OnMove(int x, int y);
};

inline CInPlaceComboBox::~CInPlaceComboBox()
{
}

inline int CInPlaceComboBox::GetCurrentSelection() const
{
	return m_nCurrentSelection;
}

_NAMESPACE_END