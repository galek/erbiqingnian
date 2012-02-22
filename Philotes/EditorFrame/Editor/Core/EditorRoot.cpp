
#include "EditorRoot.h"
#include "Xml.h"
#include "XmlUtils.h"
#include "UIEnumsDatabase.h"
#include "EditorTool.h"
#include "ClassPlugin.h"
#include "../EditorDoc.h"

EditorRoot* EditorRoot::s_RootInstance = 0;

EditorRoot::EditorRoot()
{
	m_pXMLUtils			= new CXmlUtils();
	m_pUIEnumsDatabase	= new CUIEnumsDatabase;
	m_classFactory		= CClassFactory::Instance();
}

EditorRoot::~EditorRoot()
{
	SAFE_DELETE(m_pXMLUtils);
	SAFE_DELETE(m_pUIEnumsDatabase);
	SAFE_DELETE(m_classFactory);
}

EditorRoot& EditorRoot::Get()
{
	if (!s_RootInstance)
	{
		s_RootInstance = new EditorRoot();
	}
	return *s_RootInstance;
}

void EditorRoot::Destroy()
{
	if (s_RootInstance)
	{
		delete s_RootInstance;
		s_RootInstance = NULL;
	}
}

XmlNodeRef EditorRoot::CreateXmlNode( const char *sNodeName/*="" */ )
{
	return new CXmlNode( sNodeName );
}

XmlNodeRef EditorRoot::LoadXmlFile( const char *sFilename )
{
	return m_pXMLUtils->LoadXmlFile(sFilename);
}

XmlNodeRef EditorRoot::LoadXmlFromString( const char *sXmlString )
{
	return m_pXMLUtils->LoadXmlFromString(sXmlString);
}

CUIEnumsDatabase* EditorRoot::GetUIEnumsDatabase()
{
	return m_pUIEnumsDatabase;
}

void EditorRoot::SetEditTool( CEditTool *tool,bool bStopCurrentTool/*=true */ )
{
// 	if (tool == NULL)
// 	{
// 		if (m_pEditTool != 0 && m_pEditTool->IsKindOf(RUNTIME_CLASS(CObjectMode)))
// 		{
// 			return;
// 		}
// 		else
// 		{
// 			tool = new CObjectMode;
// 		}
// 	}

	if (tool)
	{
		if (!tool->Activate(m_pEditTool))
		{
			return;
		}
	}

	if (bStopCurrentTool)
	{
		if (m_pEditTool != tool && m_pEditTool)
		{
			m_pEditTool->EndEditParams();
			//SetStatusText( "Ready" );
		}
	}

	m_pEditTool = tool;

	if (m_pEditTool)
	{
		m_pEditTool->BeginEditParams( 0 );
	}

	if (tool != m_pickTool)
	{
		m_pickTool = NULL;
	}

	// ÊÂ¼þ
	//Notify( eNotify_OnEditToolChange );
}

void EditorRoot::SetEditTool( const CString &sEditToolName,bool bStopCurrentTool/*=true */ )
{
	CEditTool *pTool = GetEditTool();
	if (pTool && pTool->GetClassDesc())
	{
		if (stricmp(pTool->GetClassDesc()->ClassName(),sEditToolName) == 0)
			return;
	}

	IClassDesc *pClass = this->GetClassFactory()->FindClass( sEditToolName );
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
	if (pRtClass && pRtClass->IsDerivedFrom(RUNTIME_CLASS(CEditTool)))
	{
		CEditTool *pEditTool = (CEditTool*)pRtClass->CreateObject();
		this->SetEditTool(pEditTool);
		return;
	}
	else
	{
		//Warning( "Class name %s is not a valid Edit Tool class.",(const char*)sEditToolName );
		return;
	}
}

CEditTool* EditorRoot::GetEditTool()
{
	return m_pEditTool;
}

IEditorClassFactory* EditorRoot::GetClassFactory()
{
	return m_classFactory;
}

CEditorDoc* EditorRoot::GetDocument()
{
	return m_document;
}

void EditorRoot::SetDocument( CEditorDoc* doc )
{
	m_document = doc;
}

