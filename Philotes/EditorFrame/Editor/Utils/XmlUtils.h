
/********************************************************************
	����:		2012��2��21��
	�ļ���: 	XmlUtils.h
	������:		������
	����:		XML�������ر��湤��	
*********************************************************************/

#pragma once

#include "XmlInterface.h"

class CXmlNodePool;
class CXmlUtils : public IXmlUtils
{
public:
	CXmlUtils( );

	virtual ~CXmlUtils();

	virtual IXmlParser*		CreateXmlParser();

	virtual XmlNodeRef		LoadXmlFile( const char *sFilename );

	virtual XmlNodeRef		LoadXmlFromString( const char *sXmlString );	

	virtual void			InitStatsXmlNodePool( UINT nPoolSize = 1024*1024 );

	virtual XmlNodeRef		CreateStatsXmlNode( const char *sNodeName="" );

private:

	CXmlNodePool* m_pStatsXmlNodePool;
};