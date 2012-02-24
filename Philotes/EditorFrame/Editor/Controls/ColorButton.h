/********************************************************************
	日期:		2012年2月20日
	文件名: 	ColorButton.h
	创建者:		王延兴
	描述:		颜色按钮	
*********************************************************************/

#pragma once

#include <afxwin.h>

_NAMESPACE_BEGIN

class CColorButton : public CButton
{
public:

public:
	CColorButton();

	CColorButton( COLORREF color, bool showText );

	virtual ~CColorButton();

	virtual void		DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

	virtual void		PreSubclassWindow();

	void				SetColor( const COLORREF &col );

	COLORREF			GetColor() const;

	void				SetTextColor( const COLORREF &col );

	COLORREF			GetTexColor() const { return m_textColor; };

	void				SetShowText( bool showText );

	bool				GetShowText() const;

protected:

	DECLARE_MESSAGE_MAP()

	COLORREF			m_color;

	COLORREF			m_textColor;

	bool				m_showText;
};

_NAMESPACE_END