
#include "EnvironmentEditor.h"
#include "EnvironmentPanel.h"
#include "EditorRoot.h"

_NAMESPACE_BEGIN

IMPLEMENT_DYNCREATE(CEnvironmentTool,CEditTool)

CEnvironmentTool::CEnvironmentTool()
{
	SetStatusText( _T("���Ӧ������Ӧ�����޸�") );
	m_panelId = 0;
	m_panel = 0;
}

CEnvironmentTool::~CEnvironmentTool()
{
}

void CEnvironmentTool::BeginEditParams( int flags )
{
	if (!m_panelId)
	{
		m_panel = new CEnvironmentPanel(AfxGetMainWnd());
		m_panelId =	CEditorRoot::Get().AddRollUpPage( ROLLUP_TERRAIN,"����",m_panel );
		AfxGetMainWnd()->SetFocus();
	}
}

void CEnvironmentTool::EndEditParams()
{
	if (m_panelId)
	{
		CEditorRoot::Get().RemoveRollUpPage(ROLLUP_TERRAIN,m_panelId);
		m_panel = 0;
		m_panelId = 0;
	}
}

_NAMESPACE_END