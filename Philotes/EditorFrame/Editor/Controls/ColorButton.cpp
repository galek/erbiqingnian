
#include "ColorButton.h"

_NAMESPACE_BEGIN

CColorButton::CColorButton()
: m_color( ::GetSysColor( COLOR_BTNFACE ) )
, m_showText( true )
{
}


CColorButton::CColorButton( COLORREF color, bool showText )
: m_color( color )
, m_showText( showText )
{
}


CColorButton::~CColorButton()
{
}


BEGIN_MESSAGE_MAP(CColorButton, CButton)
	
END_MESSAGE_MAP()

void CColorButton::DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct ) 
{
	ASSERT( 0 != lpDrawItemStruct );

	CDC* pDC( CDC::FromHandle( lpDrawItemStruct->hDC ) );
	ASSERT( 0 != pDC );

	DWORD style = GetStyle();

	UINT uiDrawState( DFCS_ADJUSTRECT );	
	UINT state( lpDrawItemStruct->itemState );

	CRect rc; 
	rc.CopyRect( &lpDrawItemStruct->rcItem );

	if(((state & ODS_DEFAULT) || (state & ODS_FOCUS)) )
	{
		pDC->Draw3dRect(rc, GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_WINDOWFRAME));
		rc.DeflateRect(1, 1);
	}

	if(style & BS_FLAT)
	{
		pDC->Draw3dRect(rc, GetSysColor(COLOR_WINDOWFRAME), GetSysColor(COLOR_WINDOWFRAME));
		rc.DeflateRect(1, 1);
		pDC->Draw3dRect(rc, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOW));
		rc.DeflateRect(1, 1);
	}
	else
	{
		if(state & ODS_SELECTED)
		{
			pDC->Draw3dRect(rc, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DSHADOW));
			rc.DeflateRect(1, 1);
		}
		else
		{
			pDC->Draw3dRect(rc, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DDKSHADOW));
			rc.DeflateRect(1, 1);
		}
	}

	if(state & ODS_SELECTED)
	{
		pDC->SetBkMode(TRANSPARENT);
		pDC->FillSolidRect( &rc, m_color );
	}
	else
	{
		pDC->SetBkMode(TRANSPARENT);
		pDC->FillSolidRect( &rc, m_color );
	}
	
	if( false != m_showText )
	{
		CString wndText;
		GetWindowText( wndText );

		if (!wndText.IsEmpty())
		{
			pDC->SetTextColor( m_textColor );
			pDC->DrawText( wndText, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
		}
	}
}


void CColorButton::PreSubclassWindow() 
{
	CButton::PreSubclassWindow();
	SetButtonStyle( BS_PUSHBUTTON | BS_OWNERDRAW );
}


void CColorButton::SetColor( const COLORREF &col )
{
	m_color = col;
	Invalidate();
}

void CColorButton::SetTextColor( const COLORREF &col )
{
	m_textColor = col;
	Invalidate();
}


COLORREF CColorButton::GetColor() const
{
	return( m_color );
}


void CColorButton::SetShowText( bool showText )
{
	m_showText = showText;
	Invalidate();
}


bool CColorButton::GetShowText() const
{
	return( m_showText );
}

_NAMESPACE_END