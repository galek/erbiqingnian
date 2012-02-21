
#pragma once

#include "GuidUtil.h"

class XmlString : public std::string
{
public:
	XmlString() {};
	XmlString( const char *str )	: std::string(str) {};
	XmlString( const CString &str ) : std::string( (const char*)str ) {};

	operator const char*() const { return c_str(); }
};

//////////////////////////////////////////////////////////////////////////

struct IXmlStringData
{
	virtual void		AddRef()			= 0;
	virtual void		Release()			= 0;
	virtual const char* GetString()			= 0;
	virtual size_t      GetStringLength()	= 0;
};

//////////////////////////////////////////////////////////////////////////

class IXmlNode;

class XmlNodeRef 
{
public:
	XmlNodeRef() : p(NULL) {}

	explicit XmlNodeRef( int Null ) : p(NULL) {}

	XmlNodeRef( IXmlNode* p_ );

	XmlNodeRef( const XmlNodeRef &p_ );

	~XmlNodeRef();

	operator IXmlNode*() const { return p; }
	operator const IXmlNode*() const { return p; }
	IXmlNode& operator*() const { return *p; }
	IXmlNode* operator->(void) const { return p; }

	XmlNodeRef&  operator=( IXmlNode* newp );
	XmlNodeRef&  operator=( const XmlNodeRef &newp );

	operator bool() const { return p != NULL; };
	bool operator !() const { return p == NULL; };

	bool  operator == ( const IXmlNode* p2 ) const { return p == p2; };
	bool  operator == ( IXmlNode* p2 ) const { return p == p2; };
	bool  operator != ( const IXmlNode* p2 ) const { return p != p2; };
	bool  operator != ( IXmlNode* p2 ) const { return p != p2; };
	bool  operator <  ( const IXmlNode* p2 ) const { return p < p2; };
	bool  operator >  ( const IXmlNode* p2 ) const { return p > p2; };

	bool  operator == ( const XmlNodeRef &n ) const { return p == n.p; };
	bool  operator != ( const XmlNodeRef &n ) const { return p != n.p; };
	bool  operator <  ( const XmlNodeRef &n ) const { return p < n.p; };
	bool  operator >  ( const XmlNodeRef &n ) const { return p > n.p; };

	friend bool operator == ( const XmlNodeRef &p1,int null );
	friend bool operator != ( const XmlNodeRef &p1,int null );
	friend bool operator == ( int null,const XmlNodeRef &p1 );
	friend bool operator != ( int null,const XmlNodeRef &p1 );

private:
	IXmlNode* p;
};

//////////////////////////////////////////////////////////////////////////

class IXmlNode
{
protected:
	int m_nRefCount;

protected:
	virtual void			DeleteThis() = 0;

	virtual					~IXmlNode() {};

public:
	virtual XmlNodeRef		createNode( const char *tag ) = 0;

	virtual void			AddRef() { m_nRefCount++; };

	virtual void			Release() { if (--m_nRefCount <= 0) DeleteThis(); };

	virtual const char *	getTag() const = 0;

	virtual void			setTag( const char *tag ) = 0;

	virtual bool			isTag( const char *tag ) const = 0;

	virtual int				getNumAttributes() const = 0;

	virtual bool			getAttributeByIndex( int index,const char **key,const char **value ) = 0;

	virtual void			copyAttributes( XmlNodeRef fromNode ) = 0;

	virtual const char*		getAttr( const char *key ) const = 0;

	virtual bool			getAttr(const char *key, const char **value) const = 0;

	virtual bool			haveAttr( const char *key ) const = 0;

	virtual void			addChild( const XmlNodeRef &node ) = 0;

	virtual XmlNodeRef		newChild( const char *tagName ) = 0;

	virtual void			removeChild( const XmlNodeRef &node ) = 0;

	virtual void			removeAllChilds() = 0;

	virtual void			deleteChildAt( int nIndex ) = 0;

	virtual int				getChildCount() const = 0;

	virtual XmlNodeRef		getChild( int i ) const = 0;

	virtual XmlNodeRef		findChild( const char *tag ) const = 0;

	virtual XmlNodeRef		getParent() const = 0;

	virtual const char*		getContent() const = 0;

	virtual void			setContent( const char *str ) = 0;

	virtual XmlNodeRef		clone() = 0;

	virtual int				getLine() const = 0;

	virtual void			setLine( int line ) = 0;

	virtual IXmlStringData* getXMLData( int nReserveMem=0 ) const = 0;
	
	virtual XmlString		getXML( int level=0 ) const = 0;

	virtual bool			saveToFile( const char *fileName ) = 0;

	virtual bool			saveToFile( const char *fileName, size_t chunkSizeBytes, FILE *file = NULL) = 0; 

	virtual void 			setAttr( const char* key,const char* value ) = 0;
	virtual void 			setAttr( const char* key,int value ) = 0;
	virtual void 			setAttr( const char* key,unsigned int value ) = 0;
	virtual void 			setAttr( const char* key,INT64 value ) = 0;
	virtual void 			setAttr( const char* key,UINT64 value,bool useHexFormat = true ) = 0;
	virtual void 			setAttr( const char* key,float value ) = 0;
	virtual void 			setAttr( const char* key,const Float2& value ) = 0;
	virtual void 			setAttr( const char* key,const Float3& value ) = 0;
	virtual void 			setAttr( const char* key,const Quaternion& value ) = 0;

	void 					setAttr( const char* key,unsigned long value ) { setAttr( key,(unsigned int)value ); };
	void 					setAttr( const char* key,long value ) { setAttr( key,(int)value ); };
	void 					setAttr( const char* key,double value ) { setAttr( key,(float)value ); };

	virtual void			delAttr( const char* key ) = 0;
	virtual void			removeAllAttributes() = 0;

	virtual bool 			getAttr( const char *key,int &value ) const = 0;
	virtual bool 			getAttr( const char *key,unsigned int &value ) const = 0;
	virtual bool 			getAttr( const char *key,INT64 &value ) const = 0;
	virtual bool 			getAttr( const char *key,UINT64 &value,bool useHexFormat = true ) const = 0;
	virtual bool 			getAttr( const char *key,float &value ) const = 0;
	virtual bool 			getAttr( const char *key,bool &value ) const = 0;
	virtual bool 			getAttr( const char *key,XmlString &value ) const = 0;
	virtual bool 			getAttr( const char *key,Float2& value ) const = 0;
	virtual bool 			getAttr( const char *key,Float3& value ) const = 0;
	virtual bool 			getAttr( const char *key,Quaternion &value ) const = 0;

	bool 					getAttr( const char *key,long &value ) const { int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool 					getAttr( const char *key,unsigned long &value ) const { unsigned int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool 					getAttr( const char *key,unsigned short &value ) const { unsigned int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool 					getAttr( const char *key,unsigned char &value ) const { unsigned int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool 					getAttr( const char *key,short &value ) const { int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool 					getAttr( const char *key,char &value ) const { int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool 					getAttr( const char *key,double &value ) const { float v; if (getAttr(key,v)) { value = (double)v; return true; } else return false; }

	bool					getAttr( const char *key,CString &value ) const
	{
		if (!haveAttr(key))
			return false;
		value = getAttr(key);
		return true;
	}

	void					setAttr( const char* key,REFGUID value )
	{
		const char *str = GuidUtil::ToString(value);
		setAttr( key,str );
	};

	bool					getAttr( const char *key,GUID &value ) const
	{
		if (!haveAttr(key))
			return false;
		const char *guidStr = getAttr(key);
		value = GuidUtil::FromString( guidStr );
		if (value.Data1 == 0)
		{
			memset( &value,0,sizeof(value) );
			// If bad GUID, use old guid system.
			value.Data1 = atoi(guidStr);
		}
		return true;
	}

	friend class XmlNodeRef;
};

inline XmlNodeRef::XmlNodeRef( IXmlNode* p_ ) : p(p_)
{
	if (p) p->AddRef();
}

inline XmlNodeRef::XmlNodeRef( const XmlNodeRef &p_ ) : p(p_.p)
{
	if (p) p->AddRef();
}

inline XmlNodeRef::~XmlNodeRef()
{
	if (p) p->Release();
}

inline XmlNodeRef&  XmlNodeRef::operator=( IXmlNode* newp )
{
	if (newp) newp->AddRef();
	if (p) p->Release();
	p = newp;
	return *this;
}

inline XmlNodeRef&  XmlNodeRef::operator=( const XmlNodeRef &newp )
{
	if (newp.p) newp.p->AddRef();
	if (p) p->Release();
	p = newp.p;
	return *this;
}

inline bool operator == ( const XmlNodeRef &p1,int null )	{
	return p1.p == 0;
}

inline bool operator != ( const XmlNodeRef &p1,int null )	{
	return p1.p != 0;
}

inline bool operator == ( int null,const XmlNodeRef &p1 )	{
	return p1.p == 0;
}

inline bool operator != ( int null,const XmlNodeRef &p1 )	{
	return p1.p != 0;
}

//////////////////////////////////////////////////////////////////////////

struct IXmlParser
{
	virtual void AddRef() = 0;
	virtual void Release() = 0;

	virtual XmlNodeRef ParseFile( const char *filename,bool bCleanPools ) = 0;

	virtual XmlNodeRef ParseBuffer( const char *buffer,int nBufLen,bool bCleanPools ) = 0;
};

//////////////////////////////////////////////////////////////////////////

struct IXmlUtils
{
	virtual IXmlParser*			CreateXmlParser() = 0;

	virtual XmlNodeRef			LoadXmlFile( const char *sFilename ) = 0;

	virtual XmlNodeRef			LoadXmlFromString( const char *sXmlString ) = 0;	

	virtual const char *		HashXml( XmlNodeRef node ) = 0;

	virtual bool				SaveBinaryXmlFile( const char *sFilename,XmlNodeRef root ) = 0;
	
	virtual XmlNodeRef			LoadBinaryXmlFile( const char *sFilename ) = 0;

	virtual bool 				EnableBinaryXmlLoading( bool bEnable ) = 0;

	virtual void 				InitStatsXmlNodePool( UINT nPoolSize ) = 0;

	virtual XmlNodeRef			CreateStatsXmlNode( const char *sNodeName ) = 0;

};