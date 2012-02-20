
/********************************************************************
	日期:		2012年2月20日
	文件名: 	InPlaceEdit.h
	创建者:		王延兴
	描述:		嵌入式编辑框	
*********************************************************************/

#pragma once

#include "Functor.h"

class CInPlaceEdit : public CEdit
{
public:
	typedef Functor0 OnChange;

	CInPlaceEdit( const CString& srtInitText,OnChange onchange );

	virtual			~CInPlaceEdit();

	void			SetText(const CString& strText);

	virtual BOOL	PreTranslateMessage(MSG* pMsg);

protected:
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void	OnKillFocus(CWnd* pNewWnd);

	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);

	afx_msg UINT	OnGetDlgCode();

	DECLARE_MESSAGE_MAP()

protected:
	CString			m_strInitText;

	OnChange		m_onChange;
};

