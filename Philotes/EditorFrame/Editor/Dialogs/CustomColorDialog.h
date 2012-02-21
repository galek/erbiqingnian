
/********************************************************************
	日期:		2012年2月21日
	文件名: 	CustomColorDialog.h
	创建者:		王延兴
	描述:		自定义的颜色拾取对话框	
*********************************************************************/

#pragma once

#include "Functor.h"

class CCustomColorDialog : public CColorDialog
{
	DECLARE_DYNAMIC(CCustomColorDialog)

public:
	typedef Functor1<COLORREF> ColorChangeCallback;

	CCustomColorDialog(COLORREF clrInit = 0, DWORD dwFlags = 0,CWnd* pParentWnd = NULL);

	~CCustomColorDialog();

	void SetColorChangeCallback( ColorChangeCallback cb ) { m_callback = cb; };

protected:
	
	afx_msg void		OnPickColor();

	virtual BOOL		OnInitDialog();

	afx_msg void		OnLButtonDown(UINT nFlags, CPoint point);

	afx_msg void		OnMouseMove(UINT nFlags, CPoint point);

	afx_msg int			OnCreate(LPCREATESTRUCT lpCreateStruct);
	
	DECLARE_MESSAGE_MAP()

	void 				PickMode( bool bEnable );

	void 				OnColorChange( COLORREF col );

	CButton				m_pickColor;

	bool				m_bPickMode;

	HCURSOR				m_pickerCusror;

	ColorChangeCallback m_callback;
};
