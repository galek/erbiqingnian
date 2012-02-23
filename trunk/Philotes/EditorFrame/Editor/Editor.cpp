// Editor.cpp : Defines the class behaviors for the application.
//

#include "Editor.h"
#include "MainFrm.h"

#include "EditorDoc.h"
#include "EditorView.h"
#include "Viewport.h"
#include "ViewManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEditorApp

BEGIN_MESSAGE_MAP(CEditorApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CEditorApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
END_MESSAGE_MAP()


// CEditorApp construction

CEditorApp::CEditorApp()
{
	m_bPrevActive = false;
	m_bForceProcessIdle = false;
}


// The one and only CEditorApp object

CEditorApp theApp;


// CEditorApp initialization

BOOL CEditorApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	EditorRoot* er = new EditorRoot;

	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4); 

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CEditorDoc),
		RUNTIME_CLASS(CMainFrame), 
		RUNTIME_CLASS(CEditorView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (!ProcessShellCommand(cmdInfo))
		return FALSE;


	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

void CEditorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

BOOL CEditorApp::OnIdle( LONG lCount )
{
	if (!m_pMainWnd)
	{
		return 0;
	}

	CWnd *pWndForeground = CWnd::GetForegroundWindow();
	CWnd *pForegroundOwner = NULL;

	bool bIsAppWindow = (pWndForeground == m_pMainWnd);
	if (pWndForeground)
	{
		DWORD wndProcId = 0;
		DWORD wndThreadId = GetWindowThreadProcessId( pWndForeground->GetSafeHwnd(),&wndProcId );
		if (GetCurrentProcessId() == wndProcId)
		{
			bIsAppWindow = true;
		}
	}
	bool bActive = false;
	int res = 0;
	if (bIsAppWindow || m_bForceProcessIdle)
	{
		res = 1;
		bActive = true;
	}
	m_bForceProcessIdle = false;

	if (m_bPrevActive != bActive)
	{
		// 丢失焦点事件处理
	}
	m_bPrevActive = bActive;

	if (bActive)
	{
		// 主循环
		CViewport *pRenderViewport = EditorRoot::Get().GetViewManager()->GetGameViewport();
		if (pRenderViewport)
		{
			// 渲染视口更新
			pRenderViewport->Update();
		}
	}

	CWinApp::OnIdle(lCount);

	return res;
}
