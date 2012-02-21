
#pragma once

#include "XmlInterface.h"

struct IXmlStringPool
{
public:
	IXmlStringPool() 
	{ 
		m_refCount = 0; 
	}

	virtual ~IXmlStringPool() {};

	void AddRef() 
	{
		m_refCount++; 
	}

	void Release() 
	{
		if (--m_refCount <= 0) 
			delete this; 
	};

	virtual char* AddString( const char *str ) = 0;

private:
	int m_refCount;
};

//////////////////////////////////////////////////////////////////////////

class XmlParser : public IXmlParser
{
public:
	XmlParser();

	~XmlParser();

	void AddRef()
	{
		++m_nRefCount;
	}

	void Release()
	{
		if (--m_nRefCount <= 0)
			delete this;
	}

	virtual XmlNodeRef ParseFile( const char *filename,bool bCleanPools );

	virtual XmlNodeRef ParseBuffer( const char *buffer,int nBufLen,bool bCleanPools );

	XmlNodeRef	parse( const char *fileName );

	XmlNodeRef	parseBuffer( const char *buffer );

	const char* getErrorString() const { return m_errorString; }

private:
	int					m_nRefCount;

	XmlString			m_errorString;

	class XmlParserImp* m_pImpl;
};


//////////////////////////////////////////////////////////////////////////
typedef int (__cdecl *XmlStrCmpFunc)( const char *str1,const char *str2 );
extern XmlStrCmpFunc g_pXmlStrCmp;

//////////////////////////////////////////////////////////////////////////

struct XmlAttribute
{
	const char* key;
	const char* value;

	bool operator<( const XmlAttribute &attr ) const { return g_pXmlStrCmp( key,attr.key ) < 0; }
	bool operator>( const XmlAttribute &attr ) const { return g_pXmlStrCmp( key,attr.key ) > 0; }
	bool operator==( const XmlAttribute &attr ) const { return g_pXmlStrCmp( key,attr.key ) == 0; }
	bool operator!=( const XmlAttribute &attr ) const { return g_pXmlStrCmp( key,attr.key ) != 0; }
};

typedef std::vector<XmlAttribute>		XmlAttributes;
typedef XmlAttributes::iterator			XmlAttrIter;
typedef XmlAttributes::const_iterator	XmlAttrConstIter;

//////////////////////////////////////////////////////////////////////////

class CXmlNode : public IXmlNode
{
public:
	
	CXmlNode();

	CXmlNode( const char *tag );
	
	~CXmlNode();

	virtual void			DeleteThis() { delete this; };

	XmlNodeRef				createNode( const char *tag );

	const char*				getTag() const { return m_tag; };

	void					setTag( const char *tag );

	bool					isTag( const char *tag ) const;

	virtual int				getNumAttributes() const
	{
		return (int)m_attributes.size(); 
	}

	virtual bool			getAttributeByIndex( int index,const char **key,const char **value );

	virtual bool			getAttributeByIndex( int index, XmlString& key,XmlString &value );

	virtual void			copyAttributes( XmlNodeRef fromNode );

	const char*				getAttr( const char *key ) const;

	bool					getAttr(const char *key, const char **value) const;

	bool					haveAttr( const char *key ) const;

	XmlNodeRef				newChild( const char *tagName );

	void					addChild( const XmlNodeRef &node );

	void					removeChild( const XmlNodeRef &node );

	void					removeAllChilds();

	int						getChildCount() const { return (int)m_childs.size(); };

	XmlNodeRef				getChild( int i ) const;

	XmlNodeRef				findChild( const char *tag ) const;

	void					deleteChild( const char *tag );

	void					deleteChildAt( int nIndex );

	XmlNodeRef				getParent() const { return m_parent; }

	const char*				getContent() const { return m_content; };

	void					setContent( const char *str );

	XmlNodeRef				clone();

	int						getLine() const { return m_line; };

	void					setLine( int line ) { m_line = line; };

	virtual IXmlStringData* getXMLData( int nReserveMem=0 ) const;
	XmlString				getXML( int level=0 ) const;
	bool 					saveToFile( const char *fileName );
	bool 					saveToFile( const char *fileName, size_t chunkSizeBytes, FILE *file = NULL );

	void 					setAttr( const char* key,const char* value );
	void 					setAttr( const char* key,int value );
	void 					setAttr( const char* key,unsigned int value );
	void 					setAttr( const char* key,INT64 value );
	void 					setAttr( const char* key,UINT64 value,bool useHexFormat = true  );
	void 					setAttr( const char* key,float value );
	void 					setAttr( const char* key,const Float2& value );
	void 					setAttr( const char* key,const Float3& value );
	void 					setAttr( const char* key,const Quaternion& value );

	void 					delAttr( const char* key );
	void 					removeAllAttributes();

	bool 					getAttr( const char *key,int &value ) const;
	bool 					getAttr( const char *key,unsigned int &value ) const;
	bool 					getAttr( const char *key,INT64 &value ) const;
	bool 					getAttr( const char *key,UINT64 &value,bool useHexFormat = true) const;
	bool 					getAttr( const char *key,float &value ) const;
	bool 					getAttr( const char *key,bool &value ) const;
	bool 					getAttr( const char *key,Float2 &value ) const;
	bool 					getAttr( const char *key,Float3 &value ) const;
	bool 					getAttr( const char *key,Quaternion &value ) const;
	bool 					getAttr( const char *key,ColorValue &value ) const;
	bool 					getAttr( const char *key,XmlString &value ) const
	{
		const char* v(NULL);
		bool  boHasAttribute(getAttr(key,&v));
		value = v;
		return boHasAttribute;
	}

private:
	void					AddToXmlString( XmlString &xml,int level,FILE* pFile=0 ) const;

	XmlString				MakeValidXmlString( const XmlString &xml ) const;

	bool					IsValidXmlString( const char *str ) const;

	XmlAttrConstIter		GetAttrConstIterator( const char *key ) const
	{
		XmlAttribute tempAttr;
		tempAttr.key = key;

		XmlAttributes::const_iterator it = std::find( m_attributes.begin(),m_attributes.end(),tempAttr );
		return it;
	}

	XmlAttrIter				GetAttrIterator( const char *key )
	{
		XmlAttribute tempAttr;
		tempAttr.key = key;

		XmlAttributes::iterator it = std::find( m_attributes.begin(),m_attributes.end(),tempAttr );
		return it;
	}

	const char*				GetValue( const char *key ) const
	{
		XmlAttrConstIter it = GetAttrConstIterator(key);
		if (it != m_attributes.end())
			return it->value;
		return 0;
	}

protected:
	IXmlStringPool*	m_pStringPool;

	const char*		m_tag;

private:

	const char*		m_content;
	CXmlNode*		m_parent;

	typedef std::vector<CXmlNode*>	XmlNodes;
	XmlNodes		m_childs;
	XmlAttributes	m_attributes;

	int				m_line;

	friend class XmlParserImp;
};

//////////////////////////////////////////////////////////////////////////

class CXmlNodePool;

class CXmlNodeReuse: public CXmlNode
{
public:
	CXmlNodeReuse(const char *tag, CXmlNodePool* pPool);
	virtual void Release();

protected:
	CXmlNodePool* m_pPool;
};

//////////////////////////////////////////////////////////////////////////

class CXmlNodePool
{
public:
	CXmlNodePool(unsigned int nBlockSize);

	virtual ~CXmlNodePool();

	XmlNodeRef GetXmlNode(const char* sNodeName);

protected:
	virtual void OnRelease(int iRefCount, void* pThis);

	IXmlStringPool* GetStringPool() { return m_pStringPool; }

private:
	friend class CXmlNodeReuse;

	IXmlStringPool* m_pStringPool;
	unsigned int m_nAllocated;
	std::stack<CXmlNodeReuse*> m_pNodePool;
};
