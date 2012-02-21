
/********************************************************************
	����:		2012��2��21��
	�ļ���: 	EditorRoot.h
	������:		������
	����:		�༭��������	
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