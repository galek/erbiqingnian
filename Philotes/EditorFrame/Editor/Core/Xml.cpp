
#include "Xml.h"
#include "SimpleStringPool.h"
#include "StringUtils.h"

#define FLOAT_FMT	"%.8g"
#define PRIX64 "I64X"
#define PRIx64 "I64x"
#define PRId64 "I64d"
#define PRIu64 "I64u"
#define PLATFORM_I64(x) x##i64

#include "expat.h"

//////////////////////////////////////////////////////////////////////////

static int __cdecl ascii_stricmp( const char * dst, const char * src )
{
	int f, l;
	do
	{
		if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
			f -= 'A' - 'a';
		if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
			l -= 'A' - 'a';
	}
	while ( f && (f == l) );
	return(f - l);
}

XmlStrCmpFunc g_pXmlStrCmp = &ascii_stricmp;

//////////////////////////////////////////////////////////////////////////

class CXmlStringData : public IXmlStringData
{
public:
	int m_nRefCount;
	XmlString m_string;

	CXmlStringData() { m_nRefCount = 0; }
	virtual void AddRef() { ++m_nRefCount; }
	virtual void Release() { if (--m_nRefCount <= 0) delete this; }

	virtual const char* GetString() { return m_string.c_str(); };
	virtual size_t GetStringLength() { return m_string.size(); };
};

//////////////////////////////////////////////////////////////////////////

class CXmlStringPool : public IXmlStringPool
{
public:
	char* AddString( const char *str ) { return m_stringPool.Append( str,(int)strlen(str) ); }
	void Clear() { m_stringPool.Clear(); }
	void SetBlockSize(unsigned int nBlockSize) { m_stringPool.SetBlockSize(nBlockSize); }

private:
	CSimpleStringPool m_stringPool;
};

CXmlNodeReuse::CXmlNodeReuse( const char *tag, CXmlNodePool* pPool ) : m_pPool(pPool)
{
	m_pStringPool = m_pPool->GetStringPool();
	m_pStringPool->AddRef();
	m_tag = m_pStringPool->AddString(tag);
}

void CXmlNodeReuse::Release()
{
	m_pPool->OnRelease(m_nRefCount, this);
	CXmlNode::Release();
}

CXmlNodePool::CXmlNodePool(unsigned int nBlockSize)
{
	m_pStringPool = new CXmlStringPool();
	assert(m_pStringPool != 0);

	static_cast<CXmlStringPool*>(m_pStringPool)->SetBlockSize(nBlockSize);
	m_pStringPool->AddRef();
	m_nAllocated = 0;
}

CXmlNodePool::~CXmlNodePool()
{
	while (!m_pNodePool.empty())
	{
		CXmlNodeReuse* pNode = m_pNodePool.top();
		m_pNodePool.pop();
		pNode->Release();
	}
	m_pStringPool->Release();
}

XmlNodeRef CXmlNodePool::GetXmlNode(const char* sNodeName)
{
	CXmlNodeReuse* pNode = 0;

	if (!m_pNodePool.empty())
	{
		pNode = m_pNodePool.top();
		m_pNodePool.pop();

		pNode->setTag(sNodeName);

		m_nAllocated++;
	}
	else
	{
		pNode = new CXmlNodeReuse(sNodeName, this);
		assert(pNode != 0);

		pNode->AddRef();

		m_nAllocated++;
	}
	return pNode;
}

void CXmlNodePool::OnRelease(int iRefCount, void* pThis)
{
	if (2 == iRefCount)
	{
		CXmlNodeReuse* pNode = static_cast<CXmlNodeReuse*>(pThis);

		pNode->removeAllChilds();
		pNode->removeAllAttributes();

		m_pNodePool.push(pNode);

		m_nAllocated--;

		if (0 == m_nAllocated)
		{
			static_cast<CXmlStringPool*>(m_pStringPool)->Clear();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

class XmlParserImp : public IXmlStringPool
{
public:
	XmlParserImp();

	~XmlParserImp();

	void		ParseBegin( bool bCleanPools );

	XmlNodeRef	ParseFile( const char *filename,XmlString &errorString,bool bCleanPools );

	XmlNodeRef	ParseBuffer( const char *buffer,size_t bufLen,XmlString &errorString,bool bCleanPools );

	void		ParseEnd();

	char*		AddString( const char *str ) 
	{
		return m_stringPool.Append( str,(int)strlen(str) );
	}

protected:

	void		onStartElement( const char *tagName,const char **atts );

	void		onEndElement( const char *tagName );
	
	void		onRawData( const char *data );

	static void startElement(void *userData, const char *name, const char **atts) 
	{
		((XmlParserImp*)userData)->onStartElement( name,atts );
	}
	static void endElement(void *userData, const char *name ) 
	{
		((XmlParserImp*)userData)->onEndElement( name );
	}
	static void characterData( void *userData, const char *s, int len ) 
	{
		char str[32700];
		assert( len < 32700 );
		strncpy_s( str,s,len );
		str[len] = 0;
		((XmlParserImp*)userData)->onRawData( str );
	}

	void		CleanStack();

	struct SStackEntity
	{
		XmlNodeRef node;
		std::vector<CXmlNode*> childs;
	};

	std::vector<SStackEntity>	m_nodeStack;

	int							m_nNodeStackTop;

	XmlNodeRef					m_root;

	XML_Parser					m_parser;

	CSimpleStringPool			m_stringPool;
};

//////////////////////////////////////////////////////////////////////////
void XmlParserImp::CleanStack()
{
	m_nNodeStackTop = 0;
	for (int i = 0, num = m_nodeStack.size(); i < num; i++)
	{
		m_nodeStack[i].node = 0;
		m_nodeStack[i].childs.resize(0);
	}
}

void	XmlParserImp::onStartElement( const char *tagName,const char **atts )
{
	CXmlNode *pCNode = new CXmlNode;
	pCNode->m_pStringPool = this;
	pCNode->m_pStringPool->AddRef();
	pCNode->m_tag = AddString(tagName);

	XmlNodeRef node = pCNode;

	m_nNodeStackTop++;
	if (m_nNodeStackTop >= (int)m_nodeStack.size())
		m_nodeStack.resize(m_nodeStack.size()*2);

	m_nodeStack[m_nNodeStackTop].node = pCNode;
	m_nodeStack[m_nNodeStackTop-1].childs.push_back( pCNode );

	if (!m_root)
	{
		m_root = node;
	}
	else
	{
		pCNode->m_parent = (CXmlNode*)(IXmlNode*)m_nodeStack[m_nNodeStackTop-1].node;
		node->AddRef(); // Childs need to be add refed.
	}

	node->setLine( XML_GetCurrentLineNumber( (XML_Parser)m_parser ) );

	int i = 0;
	int numAttrs = 0;
	while (atts[i] != 0)
	{
		numAttrs++;
		i += 2;
	}
	if (numAttrs > 0)
	{
		i = 0;
		pCNode->m_attributes.resize(numAttrs);
		int nAttr = 0;
		while (atts[i] != 0)
		{
			pCNode->m_attributes[nAttr].key = AddString(atts[i]);
			pCNode->m_attributes[nAttr].value = AddString(atts[i+1]);
			nAttr++;
			i += 2;
		}
	}
}

void XmlParserImp::onEndElement( const char *tagName )
{
	assert( m_nNodeStackTop > 0 );

	if (m_nNodeStackTop > 0)
	{
		// Copy current childs to the parent.
		IXmlNode* currNode = m_nodeStack[m_nNodeStackTop].node;
		((CXmlNode*)currNode)->m_childs = m_nodeStack[m_nNodeStackTop].childs;
		m_nodeStack[m_nNodeStackTop].childs.resize(0);
		m_nodeStack[m_nNodeStackTop].node = 0;
	}
	m_nNodeStackTop--;
}

void	XmlParserImp::onRawData( const char* data )
{
	assert( m_nNodeStackTop >= 0 );
	if (m_nNodeStackTop >= 0)
	{
		size_t len = strlen(data);
		if (len > 0)
		{
			if (strspn(data,"\r\n\t ") != len)
			{
				CXmlNode *node = (CXmlNode*)(IXmlNode*)m_nodeStack[m_nNodeStackTop].node;
				if (*node->m_content != '\0')
				{
					node->m_content = m_stringPool.ReplaceString( node->m_content,data );
				}
				else
					node->m_content = AddString(data);
			}
		}
	}
}

XmlParserImp::XmlParserImp()
{
	m_nNodeStackTop = 0;
	m_parser = 0;
	m_nodeStack.resize(32);
	CleanStack();
}

XmlParserImp::~XmlParserImp()
{
	ParseEnd();
}

void XmlParserImp::ParseBegin( bool bCleanPools )
{
	m_root = 0;
	CleanStack();

	if (bCleanPools)
		m_stringPool.Clear();

	XML_Memory_Handling_Suite memHandler;
	memHandler.malloc_fcn = malloc;
	memHandler.realloc_fcn = realloc;

	memHandler.free_fcn = (TFreeFunc)free;
	m_parser = XML_ParserCreate_MM(NULL,&memHandler,NULL);

	XML_SetUserData( m_parser, this );
	XML_SetElementHandler( m_parser, startElement,endElement );
	XML_SetCharacterDataHandler( m_parser,characterData );
	XML_SetEncoding( m_parser,"utf-8" );
}

//////////////////////////////////////////////////////////////////////////
void XmlParserImp::ParseEnd()
{
	if (m_parser)
	{
		XML_ParserFree( m_parser );
	}
	m_parser = 0;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef XmlParserImp::ParseBuffer( const char *buffer,size_t bufLen,XmlString &errorString,bool bCleanPools )
{
	ParseBegin(bCleanPools);
	m_stringPool.SetBlockSize( (unsigned int)bufLen / 16 );

	XmlNodeRef root = 0;

	if (XML_Parse( m_parser,buffer,(int)bufLen,1 ))
	{
		root = m_root;
	} 
	else 
	{
		char str[1024];
		sprintf( str,"XML Error: %s at line %d",XML_ErrorString(XML_GetErrorCode(m_parser)),XML_GetCurrentLineNumber(m_parser) );
		errorString = str;
	}
	m_root = 0;
	ParseEnd();

	return root;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef XmlParserImp::ParseFile( const char *filename,XmlString &errorString,bool bCleanPools )
{
	XmlNodeRef root = 0;
	assert(0);
	return root;
}


//////////////////////////////////////////////////////////////////////////

XmlParser::XmlParser()
{
	m_nRefCount = 0;
	m_pImpl = new XmlParserImp;
	m_pImpl->AddRef();
}

XmlParser::~XmlParser()
{
	m_pImpl->Release();
}

XmlNodeRef XmlParser::parse( const char *fileName )
{
	m_errorString = "";
	return m_pImpl->ParseFile( fileName,m_errorString,true );
}

XmlNodeRef XmlParser::parseBuffer( const char *buffer )
{
	m_errorString = "";
	return m_pImpl->ParseBuffer( buffer,strlen(buffer),m_errorString,true );
}

XmlNodeRef XmlParser::ParseBuffer( const char *buffer,int nBufLen,bool bCleanPools )
{
	m_errorString = "";
	return m_pImpl->ParseBuffer( buffer,nBufLen,m_errorString,bCleanPools );
}

XmlNodeRef XmlParser::ParseFile( const char *filename,bool bCleanPools )
{
	m_errorString = "";
	return m_pImpl->ParseFile( filename,m_errorString,bCleanPools );
}

//////////////////////////////////////////////////////////////////////////

CXmlNode::~CXmlNode()
{
	for (int i = 0,num = m_childs.size(); i < num; ++i)
	{
		m_childs[i]->m_parent = 0;
		m_childs[i]->Release();
	}
	m_pStringPool->Release();
}

CXmlNode::CXmlNode()
{
	m_tag = "";
	m_content = "";
	m_parent = 0;
	m_nRefCount = 0;
	m_pStringPool = 0; 
}

CXmlNode::CXmlNode( const char *tag )
{
	m_content = "";
	m_parent = 0;
	m_nRefCount = 0;
	m_pStringPool = new CXmlStringPool;
	m_pStringPool->AddRef();
	m_tag = m_pStringPool->AddString(tag);
}

XmlNodeRef CXmlNode::createNode( const char *tag )
{
	CXmlNode *pNewNode = new CXmlNode;
	pNewNode->m_pStringPool = m_pStringPool;
	m_pStringPool->AddRef();
	pNewNode->m_tag = m_pStringPool->AddString(tag);
	return XmlNodeRef( pNewNode );
}

void CXmlNode::setTag( const char *tag )
{
	m_tag = m_pStringPool->AddString(tag);
}

void CXmlNode::setContent( const char *str )
{
	m_content = m_pStringPool->AddString(str);
}

bool CXmlNode::isTag( const char *tag ) const
{
	return g_pXmlStrCmp( tag,m_tag ) == 0;
}

const char* CXmlNode::getAttr( const char *key ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
		return svalue;
	return "";
}

bool CXmlNode::getAttr(const char *key, const char **value) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		*value=svalue;
		return true;
	}
	else
	{
		*value="";
		return false;
	}
}

bool CXmlNode::haveAttr( const char *key ) const
{
	XmlAttrConstIter it = GetAttrConstIterator(key);
	if (it != m_attributes.end()) {
		return true;
	}
	return false;
}

void CXmlNode::delAttr( const char *key )
{
	XmlAttrIter it = GetAttrIterator(key);
	if (it != m_attributes.end())
	{
		m_attributes.erase( it );
	}
}

void CXmlNode::removeAllAttributes()
{
	m_attributes.clear();
}

void CXmlNode::setAttr( const char *key,const char *value )
{
	XmlAttrIter it = GetAttrIterator(key);
	if (it == m_attributes.end())
	{
		XmlAttribute tempAttr;
		tempAttr.key = m_pStringPool->AddString(key);
		tempAttr.value = m_pStringPool->AddString(value);
		m_attributes.push_back( tempAttr );
	}
	else
	{
		it->value = m_pStringPool->AddString(value);
	}
}

void CXmlNode::setAttr( const char *key,int value )
{
	char str[128];
	itoa( value,str,10 );
	setAttr( key,str );
}

void CXmlNode::setAttr( const char *key,unsigned int value )
{
	char str[128];
	itoa( value,str,10 );
	setAttr( key,str );
}

void CXmlNode::setAttr( const char *key,float value )
{
	char str[128];
	sprintf( str,FLOAT_FMT,value );
	setAttr( key,str );
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::setAttr( const char *key,INT64 value )
{
	char str[32];
	sprintf( str,"%"PRId64, value );
	setAttr( key,str );
}

//////////////////////////////////////////////////////////////////////////
void CXmlNode::setAttr( const char *key,UINT64 value,bool useHexFormat)
{
	char str[32];
	if (useHexFormat)
		sprintf( str,"%"PRIX64,value );
	else
		sprintf(str, "%"PRIu64, value);
	setAttr( key,str );
}

void CXmlNode::setAttr( const char *key,const Float3& value )
{
	char str[128];
	sprintf( str,FLOAT_FMT","FLOAT_FMT","FLOAT_FMT,value.x,value.y,value.z );
	setAttr( key,str );
}
void CXmlNode::setAttr( const char *key,const Float2& value )
{
	char str[128];
	sprintf( str,FLOAT_FMT","FLOAT_FMT,value.x,value.y );
	setAttr( key,str );
}

void CXmlNode::setAttr( const char *key,const Quaternion &value )
{
	char str[128];
	sprintf( str,FLOAT_FMT","FLOAT_FMT","FLOAT_FMT","FLOAT_FMT,value.w,value.x,value.y,value.z );
	setAttr( key,str );
}

bool CXmlNode::getAttr( const char *key,int &value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		value = atoi(svalue);
		return true;
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,unsigned int &value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		value = atoi(svalue);
		return true;
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,INT64 &value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		sscanf( svalue,"%"PRId64,&value );
		return true;
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,UINT64 &value,bool useHexFormat) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		if (useHexFormat)
			sscanf( svalue,"%"PRIX64,&value );
		else
			sscanf(svalue, "%"PRIu64, &value);
		return true;
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,bool &value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		value = atoi(svalue)!=0;
		return true;
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,float &value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		value = (float)atof(svalue);
		return true;
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,Float3& value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		float x,y,z;
		if (sscanf( svalue,"%f,%f,%f",&x,&y,&z ) == 3)
		{
			value = Float3(x,y,z);
			return true;
		}
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,Float2& value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		float x,y;
		if (sscanf( svalue,"%f,%f",&x,&y ) == 2)
		{
			value = Float2(x,y);
			return true;
		}
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,Quaternion &value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		float w,x,y,z;
		if (sscanf( svalue,"%f,%f,%f,%f",&w,&x,&y,&z ) == 4)
		{
			if (fabs(w) > VEC_EPSILON || fabs(x) > VEC_EPSILON || fabs(y) > VEC_EPSILON || fabs(z) > VEC_EPSILON)
			{
				value.w = w;
				value.x = x;
				value.y = y;
				value.z = z;
				return true;
			}
		}
	}
	return false;
}

bool CXmlNode::getAttr( const char *key,ColorValue &value ) const
{
	const char *svalue = GetValue(key);
	if (svalue)
	{
		unsigned int r, g, b, a = 255;
		int numFound = sscanf( svalue,"%u,%u,%u,%u",&r,&g,&b,&a );
		if(numFound == 3 || numFound == 4)
		{
			if(r < 256 && g < 256 && b < 256 && a < 256)
			{
				value = ColorValue((float)r/255.0f,
									(float)g/255.0f,
									(float)b/255.0f,
									(float)a/255.0f);
				return true;
			}
		}
	}
	return false;
}


XmlNodeRef CXmlNode::findChild( const char *tag ) const
{
	for (int i = 0,num = m_childs.size(); i < num; ++i)
	{
		if (m_childs[i]->isTag(tag))
			return m_childs[i];
	}
	return 0;
}

void CXmlNode::removeChild( const XmlNodeRef &node )
{
	XmlNodes::iterator it = std::find(m_childs.begin(),m_childs.end(),(IXmlNode*)node );
	if (it != m_childs.end())
	{
		(*it)->m_parent = 0;
		(*it)->Release();
		m_childs.erase(it);
	}
}

void CXmlNode::removeAllChilds()
{
	for (int i = 0,num = m_childs.size(); i < num; ++i)
	{
		m_childs[i]->m_parent = 0;
		m_childs[i]->Release();
	}
	m_childs.clear();
}

void CXmlNode::deleteChild( const char *tag )
{
	for (int i = 0,num = m_childs.size(); i < num; ++i)
	{
		if (m_childs[i]->isTag(tag))
		{
			m_childs[i]->m_parent = 0;
			m_childs[i]->Release();
			m_childs.erase( m_childs.begin()+i );
			return;
		}
	}
}

void CXmlNode::deleteChildAt( int nIndex )
{
	if (nIndex >= 0 && nIndex < (int)m_childs.size())
	{
		m_childs[nIndex]->m_parent = 0;
		m_childs[nIndex]->Release();
		m_childs.erase( m_childs.begin()+nIndex );
	}
}

void CXmlNode::addChild( const XmlNodeRef &node )
{
	assert( node != 0 );
	CXmlNode *pNode = (CXmlNode*)((IXmlNode*)node);
	pNode->AddRef();
	m_childs.push_back(pNode);
	pNode->m_parent = this;
};

XmlNodeRef CXmlNode::newChild( const char *tagName )
{
	XmlNodeRef node = createNode(tagName);
	addChild(node);
	return node;
}

XmlNodeRef CXmlNode::getChild( int i ) const
{
	assert( i >= 0 && i < (int)m_childs.size() );
	return m_childs[i];
}

void CXmlNode::copyAttributes( XmlNodeRef fromNode )
{
	IXmlNode* inode = fromNode;
	CXmlNode *n = (CXmlNode*)inode;
	if (n->m_pStringPool == m_pStringPool)
		m_attributes = n->m_attributes;
	else
	{
		m_attributes.resize( n->m_attributes.size() );
		for (int i = 0; i < (int)n->m_attributes.size(); i++)
		{
			m_attributes[i].key = m_pStringPool->AddString( n->m_attributes[i].key );
			m_attributes[i].value = m_pStringPool->AddString( n->m_attributes[i].value );
		}
	}
}

bool CXmlNode::getAttributeByIndex( int index,const char **key,const char **value )
{
	XmlAttributes::iterator it = m_attributes.begin();
	if (it != m_attributes.end())
	{
		std::advance( it,index );
		if (it != m_attributes.end())
		{
			*key = it->key;
			*value = it->value;
			return true;
		}
	}
	return false;
}

bool CXmlNode::getAttributeByIndex(int index, XmlString & key,XmlString &value )
{
	XmlAttributes::iterator it = m_attributes.begin();
	if (it != m_attributes.end())
	{
		std::advance( it,index );
		if (it != m_attributes.end())
		{
			key = it->key;
			value = it->value;
			return true;
		}
	}
	return false;
}

XmlNodeRef CXmlNode::clone()
{
	CXmlNode *node = new CXmlNode;
	node->m_pStringPool = m_pStringPool;
	m_pStringPool->AddRef();
	node->m_tag = m_tag;
	node->m_content = m_content;

	CXmlNode *n = (CXmlNode*)(IXmlNode*)node;
	n->copyAttributes( this );

	node->m_childs.reserve( m_childs.size() );
	for (int i = 0,num = m_childs.size(); i < num; ++i)
	{
		node->addChild( m_childs[i]->clone() );
	}

	return node;
}

static void AddTabsToString( XmlString &xml,int level )
{
	static const char *tabs[] = {
		"",
		" ",
		"  ",
		"   ",
		"    ",
		"     ",
		"      ",
		"       ",
		"        ",
		"         ",
		"          ",
		"           ",
	};
	
	if (level < sizeof(tabs)/sizeof(tabs[0]))
	{
		xml += tabs[level];
	}
	else
	{
		for (int i = 0; i < level; i++)
			xml += "  ";
	}
}

bool CXmlNode::IsValidXmlString( const char *str ) const
{
	int len = strlen(str);

	{
		char *s = const_cast<char*>(str);
		for (int i = 0; i < len; i++)
		{
			if ((unsigned char)s[i] > 0x7F)
				s[i] = ' ';
		}
	}

	if (strcspn(str,"\"\'&><") == len)
	{
		return true;
	}
	return false;
}

IXmlStringData* CXmlNode::getXMLData( int nReserveMem ) const
{
	CXmlStringData *pStrData = new CXmlStringData;
	pStrData->m_string.reserve(nReserveMem);
	AddToXmlString( pStrData->m_string,0 );
	return pStrData;
}

XmlString CXmlNode::getXML( int level ) const
{
	XmlString xml;
	xml = "";
	xml.reserve( 1024 );

	AddToXmlString( xml,level );
	return xml;
}

//////////////////////////////////////////////////////////////////////////
XmlString CXmlNode::MakeValidXmlString( const XmlString &instr ) const
{
	XmlString str = instr;

	StringUtil::ReplaceAll(str,"&","&amp;"	);
	StringUtil::ReplaceAll(str,"\"","&quot;");
	StringUtil::ReplaceAll(str,"\'","&apos;");
	StringUtil::ReplaceAll(str,"<","&lt;"	);
	StringUtil::ReplaceAll(str,">","&gt;"	);
	StringUtil::ReplaceAll(str,"...","&gt;");

	return str;
}

void CXmlNode::AddToXmlString( XmlString &xml, int level, FILE* pFile) const
{
	AddTabsToString( xml,level );

	// Begin Tag
	if (m_attributes.empty()) 
	{
		xml += "<";
		xml += m_tag;
		if (*m_content == 0 && m_childs.empty())
		{
			// Compact tag form.
			xml += " />\n";
			return;
		}
		xml += ">";
	} 
	else 
	{
		xml += "<";
		xml += m_tag;
		xml += " ";

		// Put attributes.
		for (XmlAttributes::const_iterator it = m_attributes.begin(); it != m_attributes.end(); )
		{
			xml += it->key;
			xml += "=\"";
			if (IsValidXmlString(it->value))
				xml += it->value;
			else
				xml += MakeValidXmlString(it->value);
			it++;
			if (it != m_attributes.end())
				xml += "\" ";
			else
				xml += "\"";
		}
		if (*m_content == 0 && m_childs.empty())
		{
			xml += "/>\n";
			return;
		}
		xml += ">";
	}

	if (IsValidXmlString(m_content))
		xml += m_content;
	else
		xml += MakeValidXmlString(m_content);

	if (m_childs.empty())
	{
		xml += "</";
		xml += m_tag;
		xml += ">\n";
		return;
	}

	xml += "\n";

	for (XmlNodes::const_iterator it = m_childs.begin(); it != m_childs.end(); ++it)
	{
		IXmlNode *node = *it;
		((CXmlNode*)node)->AddToXmlString( xml,level+1,pFile);
	}

	AddTabsToString( xml,level );
	xml += "</";
	xml += m_tag;
	xml += ">\n";
}

bool CXmlNode::saveToFile( const char *fileName )
{
	return true;
}

bool CXmlNode::saveToFile( const char *fileName, size_t chunkSizeBytes, FILE *file /*= NULL */ )
{
	return true;
}