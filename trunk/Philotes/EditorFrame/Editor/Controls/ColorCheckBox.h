/********************************************************************
	日期:		2012年2月20日
	文件名: 	ColorCheckBox.h
	创建者:		王延兴
	描述:		颜色复选框控件	
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
