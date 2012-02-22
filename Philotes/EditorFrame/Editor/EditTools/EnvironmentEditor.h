
/********************************************************************
	日期:		2012年2月22日
	文件名: 	EnvironmentEditor.h
	创建者:		王延兴
	描述:		环境编辑	
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