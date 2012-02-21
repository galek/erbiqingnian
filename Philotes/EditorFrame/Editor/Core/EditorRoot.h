
/********************************************************************
	日期:		2012年2月21日
	文件名: 	EditorRoot.h
	创建者:		王延兴
	描述:		编辑器基础类	
*********************************************************************/

#pragma once

#include "XmlInterface.h"

class CXmlUtils;
class CUIEnumsDatabase;

class EditorRoot
{
public:

	EditorRoot();

	virtual ~EditorRoot();

public:

	static EditorRoot&	Get();

	static void			Destroy();

public:

	virtual XmlNodeRef			CreateXmlNode( const char *sNodeName="" );

	virtual XmlNodeRef			LoadXmlFile( const char *sFilename );

	virtual XmlNodeRef			LoadXmlFromString( const char *sXmlString );

	virtual CUIEnumsDatabase*	GetUIEnumsDatabase();

protected:

	static EditorRoot*	s_RootInstance;

	CXmlUtils*			m_pXMLUtils;

	CUIEnumsDatabase*	m_pUIEnumsDatabase;
};