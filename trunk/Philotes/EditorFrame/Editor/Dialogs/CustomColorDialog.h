
/********************************************************************
	����:		2012��2��21��
	�ļ���: 	CustomColorDialog.h
	������:		������
	����:		�Զ������ɫʰȡ�Ի���	
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
