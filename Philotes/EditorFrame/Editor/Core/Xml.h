
#pragma once

#include "XmlInterface.h"

_NAMESPACE_BEGIN

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

	const char*				GetTag() const { return m_tag; };

	void					SetTag( const char *tag );

	bool					IsTag( const char *tag ) const;

	virtual int				GetNumAttributes() const
	{
		return (int)m_attributes.size(); 
	}

	virtual bool			GetAttributeByIndex( int index,const char **key,const char **value );

	virtual bool			GetAttributeByIndex( int index, XmlString& key,XmlString &value );

	virtual void			CopyAttributes( XmlNodeRef fromNode );

	const char*				GetAttr( const char *key ) const;

	bool					GetAttr(const char *key, const char **value) const;

	bool					HaveAttr( const char *key ) const;

	XmlNodeRef				NewChild( const char *tagName );

	void					AddChild( const XmlNodeRef &node );

	void					RemoveChild( const XmlNodeRef &node );

	void					RemoveAllChilds();

	int						GetChildCount() const { return (int)m_childs.size(); };

	XmlNodeRef				GetChild( int i ) const;

	XmlNodeRef				FindChild( const char *tag ) const;

	void					deleteChild( const char *tag );

	void					DeleteChildAt( int nIndex );

	XmlNodeRef				GetParent() const { return m_parent; }

	const char*				GetContent() const { return m_content; };

	void					SetContent( const char *str );

	XmlNodeRef				Clone();

	int						GetLine() const { return m_line; };

	void					SetLine( int line ) { m_line = line; };

	virtual IXmlStringData* GetXMLData( int nReserveMem=0 ) const;
	XmlString				GetXML( int level=0 ) const;
	bool 					SaveToFile( const char *fileName );
	bool 					SaveToFile( const char *fileName, size_t chunkSizeBytes, FILE *file = NULL );

	void 					SetAttr( const char* key,const char* value );
	void 					SetAttr( const char* key,int value );
	void 					SetAttr( const char* key,unsigned int value );
	void 					SetAttr( const char* key,INT64 value );
	void 					SetAttr( const char* key,UINT64 value,bool useHexFormat = true  );
	void 					SetAttr( const char* key,float value );
	void 					SetAttr( const char* key,const Float2& value );
	void 					SetAttr( const char* key,const Float3& value );
	void 					SetAttr( const char* key,const Quaternion& value );

	void 					DelAttr( const char* key );
	void 					RemoveAllAttributes();

	bool 					GetAttr( const char *key,int &value ) const;
	bool 					GetAttr( const char *key,unsigned int &value ) const;
	bool 					GetAttr( const char *key,INT64 &value ) const;
	bool 					GetAttr( const char *key,UINT64 &value,bool useHexFormat = true) const;
	bool 					GetAttr( const char *key,float &value ) const;
	bool 					GetAttr( const char *key,bool &value ) const;
	bool 					GetAttr( const char *key,Float2 &value ) const;
	bool 					GetAttr( const char *key,Float3 &value ) const;
	bool 					GetAttr( const char *key,Quaternion &value ) const;
	bool 					GetAttr( const char *key,ColorValue &value ) const;
	bool 					GetAttr( const char *key,XmlString &value ) const
	{
		const char* v(NULL);
		bool  boHasAttribute(GetAttr(key,&v));
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

_NAMESPACE_END