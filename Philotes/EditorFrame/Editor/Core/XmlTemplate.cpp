
#include "XmlTemplate.h"
#include "FileUtils.h"
#include "Xml.h"

#include <io.h>

_NAMESPACE_BEGIN

void CXmlTemplate::GetValues( XmlNodeRef &node, const XmlNodeRef &fromNode )
{
	assert( node != 0 && fromNode != 0 );

	if (!node)
	{
		//gEnv->pLog->LogError("CXmlTemplate::GetValues invalid node. Possible problems with Editor folder.");
		return;
	}

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		XmlNodeRef prop = node->GetChild(i);

		if (prop->GetChildCount() == 0)
		{
			CString value;
			if (fromNode->GetAttr( prop->GetTag(),value ))
			{
				prop->SetAttr( "Value",value );
			}
		}
		else
		{
			// Have childs.
			XmlNodeRef fromNodeChild = fromNode->FindChild(prop->GetTag());
			if (fromNodeChild)
			{
				CXmlTemplate::GetValues( prop,fromNodeChild );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::SetValues( const XmlNodeRef &node,XmlNodeRef &toNode )
{
	assert( node != 0 && toNode != 0 );

	toNode->RemoveAllAttributes();
	toNode->RemoveAllChilds();

	assert(node);
	if (!node)
	{
		//gEnv->pLog->LogError("CXmlTemplate::SetValues invalid node. Possible problems with Editor folder.");
		return;
	}

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		XmlNodeRef prop = node->GetChild(i);
		if (prop)
		{
			if (prop->GetChildCount() > 0)
			{
				XmlNodeRef childToNode = toNode->NewChild(prop->GetTag());
				if (childToNode)
					CXmlTemplate::SetValues( prop,childToNode );
			}
			else
			{
				CString value;
				prop->GetAttr( "Value",value );
				toNode->SetAttr( prop->GetTag(),value );
			}
		}else
			TRACE("NULL returned from node->GetChild()");
	}
}

//////////////////////////////////////////////////////////////////////////
bool CXmlTemplate::SetValues( const XmlNodeRef &node,XmlNodeRef &toNode,const XmlNodeRef &modifiedNode )
{
	assert( node != 0 && toNode != 0 && modifiedNode != 0 );

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		XmlNodeRef prop = node->GetChild(i);
		if (prop)
		{
			if (prop->GetChildCount() > 0)
			{
				XmlNodeRef childToNode = toNode->FindChild(prop->GetTag());
				if (childToNode)
				{
					if (CXmlTemplate::SetValues( prop,childToNode,modifiedNode ))
						return true;
				}
			}
			else if (prop == modifiedNode)
			{
				CString value;
				prop->GetAttr( "Value",value );
				toNode->SetAttr( prop->GetTag(),value );
				return true;
			}
		}else
			TRACE("NULL returned from node->GetChild()");
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,bool value )
{
	XmlNodeRef param = templ->NewChild(sName);
	param->SetAttr( "type","Bool" );
	param->SetAttr( "value",value );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,int value,int min,int max )
{
	XmlNodeRef param = templ->NewChild(sName);
	param->SetAttr( "type","Int" );
	param->SetAttr( "value",value );
	param->SetAttr( "min",min );
	param->SetAttr( "max",max );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,float value,float min,float max )
{
	XmlNodeRef param = templ->NewChild(sName);
	param->SetAttr( "type","Float" );
	param->SetAttr( "value",value );
	param->SetAttr( "min",min );
	param->SetAttr( "max",max );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,const char *sValue )
{
	XmlNodeRef param = templ->NewChild(sName);
	param->SetAttr( "type","String" );
	param->SetAttr( "value",sValue );
}

//////////////////////////////////////////////////////////////////////////

CXmlTemplateRegistry::CXmlTemplateRegistry()
{}

void CXmlTemplateRegistry::LoadTemplates( const CString &path )
{
	XmlParser parser;

	m_templates.clear();

	CString dir = Path::AddBackslash(path);

	CStringArray files;
	CFileUtil::ScanFiles( dir,"*.xml",files,false );

	for (int k = 0; k < files.GetSize(); k++)
	{
		XmlNodeRef child;

		XmlNodeRef node = parser.parse( files[k] );
		if (node != 0 && node->IsTag("Templates"))
		{
			CString name;
			for (int i = 0; i < node->GetChildCount(); i++)
			{
				child = node->GetChild(i);
				AddTemplate( child->GetTag(),child );
			}
		}
	}
}

void CXmlTemplateRegistry::AddTemplate( const CString &name,XmlNodeRef &tmpl )
{
	m_templates[name] = tmpl;
}

XmlNodeRef CXmlTemplateRegistry::FindTemplate( const CString &name )
{
	XmlNodeRef node;
	std::map<CString,XmlNodeRef>::const_iterator it = m_templates.find(name);
	if (it == m_templates.end())
	{
		return 0;
	}
	else
	{
		return it->second;
	}
}

_NAMESPACE_END