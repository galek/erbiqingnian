
/********************************************************************
	����:		2012��2��22��
	�ļ���: 	EnvironmentEditor.h
	������:		������
	����:		�����༭	
*********************************************************************/

#pragma once

#include "EditorTool.h"

class CEnvironmentTool : public CEditTool
{
	DECLARE_DYNCREATE(CEnvironmentTool)

public:

	CEnvironmentTool();

	virtual ~CEnvironmentTool();

	virtual void	BeginEditParams( int flags );

	virtual void	EndEditParams();

	void			DeleteThis() { delete this; }

private:

	int							m_panelId;

	class CEnvironmentPanel*	m_panel;
};