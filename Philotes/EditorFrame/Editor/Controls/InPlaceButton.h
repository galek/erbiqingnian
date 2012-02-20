
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	InPlaceButton.h
	������:		������
	����:		Ƕ��ʽ��ť�ؼ�	
*********************************************************************/

#pragma once

#include "ColorButton.h"
#include "Functor.h"

//////////////////////////////////////////////////////////////////////////
class CInPlaceButton : public CXTButton
{
	DECLARE_DYNAMIC(CInPlaceButton)

public:
	typedef Functor0 OnClick;

	CInPlaceButton( OnClick onClickFunctor ) { m_onClick = onClickFunctor; }

	void Click() 
	{
		OnBnClicked(); 
	}

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClicked()
	{
		if (m_onClick)
			m_onClick();
	}

	OnClick m_onClick;
};

//////////////////////////////////////////////////////////////////////////
class CInPlaceColorButton : public CColorButton
{
	DECLARE_DYNAMIC(CInPlaceColorButton)

public:
	typedef Functor0 OnClick;

	CInPlaceColorButton( OnClick onClickFunctor ) { m_onClick = onClickFunctor; }

	void Click() 
	{
		OnBnClicked();	
	}

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClicked()
	{
		if (m_onClick)
			m_onClick();
	}

	OnClick m_onClick;
};

//////////////////////////////////////////////////////////////////////////
class CInPlaceCheckBox : public CButton
{
	DECLARE_DYNAMIC(CInPlaceCheckBox)

public:
	typedef Functor0 OnClick;

	CInPlaceCheckBox( OnClick onClickFunctor ) { m_onClick = onClickFunctor; }

	void Click() 
	{
		OnBnClicked();	
	}

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClicked()
	{
		if (m_onClick)
			m_onClick();
	}

	OnClick m_onClick;
};
