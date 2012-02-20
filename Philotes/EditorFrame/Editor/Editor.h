
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

//////////////////////////////////////////////////////////////////////////

class CEditorApp : public CWinApp
{
public:
	CEditorApp();

public:
	virtual BOOL InitInstance();

	afx_msg void OnAppAbout();

	DECLARE_MESSAGE_MAP()
};

extern CEditorApp theApp;