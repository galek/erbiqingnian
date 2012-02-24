
/********************************************************************
	����:		2012��2��22��
	�ļ���: 	EnvironmentPanel.h
	������:		������
	����:		�����������	
*********************************************************************/

#pragma once

#include "PropertyCtrl.h"
#include "ColorCheckBox.h"

_NAMESPACE_BEGIN

class CEnvironmentPanel : public CDialog
{
public:
	CEnvironmentPanel(CWnd* pParent = NULL);
	~CEnvironmentPanel();

	enum { IDD = IDD_PANEL_ENVIRONMENT };
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	virtual void	OnOK() {};
	virtual void	OnCancel() {};

	void			OnPropertyChanged( XmlNodeRef node );

	afx_msg void	OnDestroy();
	virtual BOOL	OnInitDialog();
	afx_msg void	OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	CPropertyCtrl	m_wndProps;
	XmlNodeRef		m_node;
public:
	afx_msg void	OnBnClickedApply();
private:
	CCustomButton	m_applyBtn;
};

_NAMESPACE_END