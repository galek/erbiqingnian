/********************************************************************
	����:		2012��2��20��
	�ļ���: 	ColorButton.h
	������:		������
	����:		��ɫ��ť	
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