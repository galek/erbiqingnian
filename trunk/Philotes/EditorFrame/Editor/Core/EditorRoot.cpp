
#include "EditorRoot.h"
#include "Xml.h"
#include "UIEnumsDatabase.h"

EditorRoot* EditorRoot::s_RootInstance = 0;

EditorRoot::EditorRoot()
{
	m_pXMLUtils = new CXmlUtils();
	m_pUIEnumsDatabase = new CUIEnumsDatabase;
}

EditorRoot::~EditorRoot()
{
	SAFE_DELETE(m_pXMLUtils);
	SAFE_DELETE(m_pUIEnumsDatabase);
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

