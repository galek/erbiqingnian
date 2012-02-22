
/********************************************************************
	����:		2012��2��22��
	�ļ���: 	TerrainPanel.h
	������:		������
	����:		�������	
*********************************************************************/

#pragma once

#include "ToolButton.h"

class CTerrainPanel : public CDialog
{
public:
	CTerrainPanel(CWnd* pParent = NULL);


	enum { IDD = IDD_TERRAIN_PANEL };
	CToolButton		m_textureBtn;
	CToolButton		m_environmentBtn;
	CToolButton		m_vegetationBtn;
	CToolButton		m_modifyBtn;
	CToolButton		m_minimapBtn;

	void			OnIdleUpdate();

public:
	virtual BOOL	OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
	virtual BOOL	OnInitDialog();

	DECLARE_MESSAGE_MAP()
};