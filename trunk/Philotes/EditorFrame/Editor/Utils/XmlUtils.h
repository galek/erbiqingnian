
/********************************************************************
	日期:		2012年2月21日
	文件名: 	XmlUtils.h
	创建者:		王延兴
	描述:		XML解析加载保存工具	
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