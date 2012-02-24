
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

//////////////////////////////////////////////////////////////////////////

_NAMESPACE_BEGIN

class CEditorApp : public CWinApp
{
public:
	CEditorApp();

public:
	virtual BOOL	InitInstance();

	virtual int		ExitInstance();

	virtual BOOL	OnIdle(LONG lCount);

	afx_msg void	OnAppAbout();

	DECLARE_MESSAGE_MAP()

protected:

	// 编辑器在上一帧是否为活动状态，用于检测是否丢失焦点
	bool m_bPrevActive;

	// 为true则在下一次OnIdle中更新
	bool m_bForceProcessIdle;
};

extern CEditorApp theApp;

_NAMESPACE_END