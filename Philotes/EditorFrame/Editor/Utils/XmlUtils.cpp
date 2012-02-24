
#include "XmlUtils.h"
#include "Xml.h"

_NAMESPACE_BEGIN

CXmlUtils::CXmlUtils()
{
	m_pStatsXmlNodePool = 0;
}

//////////////////////////////////////////////////////////////////////////
CXmlUtils::~CXmlUtils()
{
	SAFE_DELETE(m_pStatsXmlNodePool);
}

IXmlParser* CXmlUtils::CreateXmlParser()
{
	return new XmlParser;
}

XmlNodeRef CXmlUtils::LoadXmlFile( const char *sFilename )
{
	XmlParser parser;
	XmlNodeRef node = parser.parse( sFilename );

	const char* pErrorString = parser.getErrorString();
	if (strcmp("", pErrorString) != 0)
	{
		//CryLog("Error parsing <%s>: %s", sFilename, parser.getErrorString());
	}

	return node;
}

XmlNodeRef CXmlUtils::LoadXmlFromString( const char *sXmlString )
{
	XmlParser parser;
	XmlNodeRef node = parser.parseBuffer( sXmlString );
	return node;
}

XmlNodeRef CXmlUtils::CreateStatsXmlNode( const char *sNodeName )
{
	if (0 == m_pStatsXmlNodePool)
	{
		//CryLog("[CXmlNodePool]: Xml stats nodes pool isn't initialized. Perform default initialization.");
		InitStatsXmlNodePool();
	}
	return m_pStatsXmlNodePool->GetXmlNode(sNodeName);
}

void CXmlUtils::InitStatsXmlNodePool( UINT nPoolSize )
{
	if (0 == m_pStatsXmlNodePool)
	{
		m_pStatsXmlNodePool = new CXmlNodePool(nPoolSize);
		assert(m_pStatsXmlNodePool);
	}
	else
	{
		//CryLog("[CXmlNodePool]: Xml stats nodes pool already initialized");
	}
}

_NAMESPACE_END