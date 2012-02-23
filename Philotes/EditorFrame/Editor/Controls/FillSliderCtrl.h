
/********************************************************************
	日期:		2012年2月21日
	文件名: 	FillSliderCtrl.h
	创建者:		王延兴
	描述:		滑动条控件	
*********************************************************************/

#pragma once

#include "SliderCtrlEx.h"

#define WMU_FS_CHANGED          (WM_USER+100)
#define WMU_FS_LBUTTONDOWN      (WM_USER+101)
#define WMU_FS_LBUTTONUP        (WM_USER+102)

//////////////////////////////////////////////////////////////////////////

class CFillSliderCtrl : public CSliderCtrlEx
{
public:
	enum EFillStyle
	{
		eFillStyle_Vertical			= 1<<0,
		eFillStyle_Gradient			= 1<<1,
		eFillStyle_Background		= 1<<2,
		eFillStyle_ColorHueGradient = 1<<3,
	};

private:
	DECLARE_DYNCREATE(CFillSliderCtrl);
public:
	typedef Functor1<CFillSliderCtrl*>	UpdateCallback;

	CFillSliderCtrl();

	~CFillSliderCtrl();

	void 			SetFilledLook( bool bFilled );
	void 			SetFillStyle(UINT style) { m_fillStyle = style; }
	void 			SetFillColors(COLORREF start, COLORREF end) { m_fillColorStart = start; m_fillColorEnd = end; }

public:
	virtual void	SetValue( float val );

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void 	OnPaint();
	afx_msg void 	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void 	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void 	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL 	OnEraseBkgnd(CDC* pDC);

	void 			NotifyUpdate( bool tracking );
	void 			ChangeValue( int sliderPos,bool bTracking );

private:
	void 			DrawFill(CDC &dc, CRect &rect);

protected:
	bool			m_bFilled;
	UINT			m_fillStyle;
	COLORREF		m_fillColorStart;
	COLORREF		m_fillColorEnd;
	CPoint			m_mousePos;
};
