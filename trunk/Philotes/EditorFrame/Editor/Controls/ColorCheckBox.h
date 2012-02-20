/********************************************************************
	����:		2012��2��20��
	�ļ���: 	ColorCheckBox.h
	������:		������
	����:		��ɫ��ѡ��ؼ�	
*********************************************************************/

#pragma once

#include "ColorCtrl.h"

#define STD_PUSHED_COLOR (RGB(255,255,0))

typedef CColorCtrl<CColorPushButton<CButton> > CColoredPushButton;
typedef CColorCtrl<CColorPushButton<CButton> > CCustomButton;

///////////////////////////////////////	///////////////////////////////////

class CColorCheckBox : public CColoredPushButton
{
	DECLARE_DYNCREATE( CColorCheckBox )

public:

	CColorCheckBox();

	int	GetCheck() const { return m_nChecked; };

	void SetCheck(int nCheck);

public:

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:

	virtual void PreSubclassWindow();

	virtual BOOL PreTranslateMessage(MSG* pMsg);

public:
	virtual ~CColorCheckBox();

	virtual void ChangeStyle();

protected:
	
	int m_nChecked;
};
