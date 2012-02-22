
/********************************************************************
	����:		2012��2��22��
	�ļ���: 	ToolButton.h
	������:		������
	����:		���߰�ť
*********************************************************************/

#pragma once

#include "ColorCheckBox.h"

class CToolButton : public CColorCheckBox
{
DECLARE_DYNAMIC(CToolButton)
public:
	CToolButton();

	void 			SetToolClass( CRuntimeClass *toolClass,void *userData=0 );
	void 			SetToolName( const CString &sEditToolName,void *userData=0 );

public:
	virtual ~CToolButton();

protected:
	afx_msg void 	OnTimer(UINT_PTR nIDEvent);
	afx_msg void 	OnDestroy();
	afx_msg void 	OnClicked();
	afx_msg void 	OnPaint();

	DECLARE_MESSAGE_MAP()

	void 			StartTimer();
	void 			StopTimer();

	CRuntimeClass*	m_toolClass;
	void*			m_userData;
	int				m_nTimer;
};
