
#include "ToolButton.h"
#include "ClassFactory.h"
#include "EditorTool.h"
#include "EditorRoot.h"
#include "Settings.h"

#define TOOLBUTTON_TIMERID 1001

_NAMESPACE_BEGIN

// CToolButton
IMPLEMENT_DYNAMIC(CToolButton,CColorCheckBox)

CToolButton::CToolButton()
{
	m_toolClass = NULL;
	m_userData	= NULL;
	m_nTimer	= NULL;
}

CToolButton::~CToolButton()
{
	StopTimer();
}

void CToolButton::SetToolName( const CString &sEditToolName,void *userData )
{
	IClassDesc *pClass = CEditorRoot::Get().GetClassFactory()->FindClass( sEditToolName );
	if (!pClass)
	{
		//Warning( "Editor Tool %s not registered.",(const char*)sEditToolName );
		return;
	}
	if (pClass->SystemClassID() != ESYSTEM_CLASS_EDITTOOL)
	{
		//Warning( "Class name %s is not a valid Edit Tool class.",(const char*)sEditToolName );
		return;
	}
	CRuntimeClass *pRtClass = pClass->GetRuntimeClass();
	if (!pRtClass || !pRtClass->IsDerivedFrom(RUNTIME_CLASS(CEditTool)))
	{
		//Warning( "Class name %s is not a valid Edit Tool class.",(const char*)sEditToolName );
		return;
	}

	m_toolClass = pRtClass;
	m_userData	= userData;
}

void CToolButton::SetToolClass( CRuntimeClass *toolClass,void *userData )
{
	m_toolClass = toolClass;
	m_userData = userData;
}

BEGIN_MESSAGE_MAP(CToolButton, CButton)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
END_MESSAGE_MAP()

void CToolButton::OnTimer(UINT_PTR nIDEvent) 
{
	CEditTool *tool = CEditorRoot::Get().GetEditTool();
	CRuntimeClass *toolClass = 0;
	if (tool)
	{
		toolClass = tool->GetRuntimeClass();
	}

	int c = GetCheck();
		
	if (toolClass != m_toolClass)
	{
		if (GetCheck() == TRUE)
		{
			SetCheck(FALSE);
		}
		StopTimer();
	}
		
	CButton::OnTimer(nIDEvent);
}

void CToolButton::OnDestroy() 
{
	StopTimer();
	CButton::OnDestroy();
}

void CToolButton::OnPaint()
{
	CColorCheckBox::OnPaint();

	CEditTool *tool = CEditorRoot::Get().GetEditTool();
	if (tool && tool->GetRuntimeClass() == m_toolClass)
	{
		if (GetCheck() == 0)
		{
			SetCheck(1);
			StartTimer();
		}
	}
	else
	{
		if (GetCheck() == 1)
		{
			SetCheck(0);
			StopTimer();
		}
	}
}

void CToolButton::OnClicked()
{
	if (!m_toolClass)
	{
		return;
	}

	CEditTool *tool = CEditorRoot::Get().GetEditTool();
	if (tool && tool->GetRuntimeClass() == m_toolClass)
	{
		CEditorRoot::Get().SetEditTool(NULL);
		SetCheck(FALSE);

		StopTimer();
	}
	else
	{
		CEditTool *tool = (CEditTool*)m_toolClass->CreateObject();
		if (!tool)
		{
			return;
		}
		
		SetCheck(TRUE);
		StartTimer();
		
		if (m_userData)
		{
			tool->SetUserData( m_userData );
		}

		CEditorRoot::Get().SetEditTool( tool );
	}
}

void CToolButton::StartTimer()
{
	StopTimer();
	m_nTimer = SetTimer(TOOLBUTTON_TIMERID,200,NULL);
}
	
void CToolButton::StopTimer()
{
	if (m_nTimer != 0)
	{
		KillTimer(m_nTimer);
	}
	m_nTimer = 0;
}

_NAMESPACE_END