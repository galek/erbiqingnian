
#include "XmlTemplate.h"
#include "FileUtils.h"
#include "Xml.h"

#include <io.h>

void CXmlTemplate::GetValues( XmlNodeRef &node, const XmlNodeRef &fromNode )
{
	assert( node != 0 && fromNode != 0 );

	if (!node)
	{
		//gEnv->pLog->LogError("CXmlTemplate::GetValues invalid node. Possible problems with Editor folder.");
		return;
	}

	for (int i = 0; i < node->getChildCount(); i++)
	{
		XmlNodeRef prop = node->getChild(i);

		if (prop->getChildCount() == 0)
		{
			CString value;
			if (fromNode->getAttr( prop->getTag(),value ))
			{
				prop->setAttr( "Value",value );
			}
		}
		else
		{
			// Have childs.
			XmlNodeRef fromNodeChild = fromNode->findChild(prop->getTag());
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

	toNode->removeAllAttributes();
	toNode->removeAllChilds();

	assert(node);
	if (!node)
	{
		//gEnv->pLog->LogError("CXmlTemplate::SetValues invalid node. Possible problems with Editor folder.");
		return;
	}

	for (int i = 0; i < node->getChildCount(); i++)
	{
		XmlNodeRef prop = node->getChild(i);
		if (prop)
		{
			if (prop->getChildCount() > 0)
			{
				XmlNodeRef childToNode = toNode->newChild(prop->getTag());
				if (childToNode)
					CXmlTemplate::SetValues( prop,childToNode );
			}
			else
			{
				CString value;
				prop->getAttr( "Value",value );
				toNode->setAttr( prop->getTag(),value );
			}
		}else
			TRACE("NULL returned from node->GetChild()");
	}
}

//////////////////////////////////////////////////////////////////////////
bool CXmlTemplate::SetValues( const XmlNodeRef &node,XmlNodeRef &toNode,const XmlNodeRef &modifiedNode )
{
	assert( node != 0 && toNode != 0 && modifiedNode != 0 );

	for (int i = 0; i < node->getChildCount(); i++)
	{
		XmlNodeRef prop = node->getChild(i);
		if (prop)
		{
			if (prop->getChildCount() > 0)
			{
				XmlNodeRef childToNode = toNode->findChild(prop->getTag());
				if (childToNode)
				{
					if (CXmlTemplate::SetValues( prop,childToNode,modifiedNode ))
						return true;
				}
			}
			else if (prop == modifiedNode)
			{
				CString value;
				prop->getAttr( "Value",value );
				toNode->setAttr( prop->getTag(),value );
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
	XmlNodeRef param = templ->newChild(sName);
	param->setAttr( "type","Bool" );
	param->setAttr( "value",value );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,int value,int min,int max )
{
	XmlNodeRef param = templ->newChild(sName);
	param->setAttr( "type","Int" );
	param->setAttr( "value",value );
	param->setAttr( "min",min );
	param->setAttr( "max",max );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,float value,float min,float max )
{
	XmlNodeRef param = templ->newChild(sName);
	param->setAttr( "type","Float" );
	param->setAttr( "value",value );
	param->setAttr( "min",min );
	param->setAttr( "max",max );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,const char *sValue )
{
	XmlNodeRef param = templ->newChild(sName);
	param->setAttr( "type","String" );
	param->setAttr( "value",sValue );
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
		if (node != 0 && node->isTag("Templates"))
		{
			CString name;
			for (int i = 0; i < node->getChildCount(); i++)
			{
				child = node->getChild(i);
				AddTemplate( child->getTag(),child );
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