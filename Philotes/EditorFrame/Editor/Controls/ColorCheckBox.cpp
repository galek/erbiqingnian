
#include "ColorCheckBox.h"
#include "Settings.h"

/////////////////////////////////////////////////////////////////////////////
// CColorCheckBox
IMPLEMENT_DYNCREATE( CColorCheckBox,CColoredPushButton )

CColorCheckBox::CColorCheckBox()
{
	m_nChecked = 0;
}

CColorCheckBox::~CColorCheckBox()
{

}
void CColorCheckBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	if (m_nChecked == 1)
	{
		lpDrawItemStruct->itemState |= ODS_SELECTED;
	}
	CColoredPushButton::DrawItem( lpDrawItemStruct );
}

//////////////////////////////////////////////////////////////////////////
void CColorCheckBox::SetCheck(int nCheck)
{
	__super::SetCheck(nCheck);
	if (m_nChecked != nCheck)
	{
		m_nChecked = nCheck;
		if(::IsWindow(m_hWnd))
			Invalidate();
	}
};

//////////////////////////////////////////////////////////////////////////
void CColorCheckBox::ChangeStyle()
{
	if ((EditorSettings::Get().Gui.bWindowsVista || EditorSettings::Get().Gui.bSkining) && !m_hIcon)
	{
		if (m_bOwnerDraw)
		{
			m_bOwnerDraw = false;
			ModifyStyle(0xf, BS_CHECKBOX|BS_PUSHLIKE);
		}
	}
	else
	{
		if (!m_bOwnerDraw)
		{
			m_bOwnerDraw = true;
			ModifyStyle(0xf, BS_OWNERDRAW);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CColorCheckBox::PreSubclassWindow() 
{
	CColoredPushButton::PreSubclassWindow();

	ChangeStyle();
}

//////////////////////////////////////////////////////////////////////////
BOOL CColorCheckBox::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_PAINT)
	{
		if (EditorSettings::Get().Gui.bWindowsVista || EditorSettings::Get().Gui.bSkining)
		{
			if ((m_bOwnerDraw && !m_hIcon) || (!m_bOwnerDraw && m_hIcon))
			{
				ChangeStyle();
				Invalidate(TRUE);
			}
		}
		else
		{
			if (!m_bOwnerDraw)
			{
				ChangeStyle();
				Invalidate(TRUE);
			}
		}
	}
	return __super::PreTranslateMessage(pMsg);
}
