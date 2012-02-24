
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

	// �༭������һ֡�Ƿ�Ϊ�״̬�����ڼ���Ƿ�ʧ����
	bool m_bPrevActive;

	// Ϊtrue������һ��OnIdle�и���
	bool m_bForceProcessIdle;
};

extern CEditorApp theApp;

_NAMESPACE_END