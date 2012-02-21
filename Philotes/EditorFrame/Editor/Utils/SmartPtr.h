
/********************************************************************
日期:		2012年2月21日
文件名: 	SmartPtr.h
创建者:		王延兴
描述:		智能指针	
*********************************************************************/

#pragma once

#ifdef WIN64
#define TSmartPtr _smart_ptr
#else

template <class Type>
class TSmartPtr 
{
	Type* p;

public:

	TSmartPtr() : p(NULL) {}

	TSmartPtr( Type* p_ ) : p(p_)
	{
		if (p) 
		{
			(p)->AddRef();
		}
	}

	TSmartPtr( const TSmartPtr<Type>& p_ ) : p(p_.p) 
	{
		if (p)
		{
			(p)->AddRef(); 
		}
	}

	TSmartPtr( int Null ) : p(NULL) {}
	
	~TSmartPtr() 
	{
		if (p)
		{
			(p)->Release();
		}
	}

	operator Type*() const 
	{
		return p; 
	}

	operator const Type*() const 
	{ 
		return p; 
	}

	Type& operator*() const 
	{
		return *p; 
	}

	Type* operator -> (void) const
	{
		return p; 
	}

	TSmartPtr&  operator = ( Type* newp ) 
	{
		if (newp)
		{
			(newp)->AddRef();
		}
		if (p)
		{
			(p)->Release();
		}
		p = newp;
		return *this;
	}

	TSmartPtr&  operator = ( const TSmartPtr<Type> &newp ) 
	{
		if (newp.p)
		{
			(newp.p)->AddRef();
		}
		if (p)
		{
			(p)->Release();
		}
		p = newp.p;
		return *this;
	}

	operator bool() const { return p != NULL; };
	bool  operator !() const { return p == NULL; };

	bool  operator == ( const Type* p2 ) const { return p == p2; };
	bool  operator != ( const Type* p2 ) const { return p != p2; };
	bool  operator <  ( const Type* p2 ) const { return p < p2; };
	bool  operator >  ( const Type* p2 ) const { return p > p2; };

	bool operator == ( const TSmartPtr<Type> &p2 ) const { return p == p2.p; };
	bool operator != ( const TSmartPtr<Type> &p2 ) const { return p != p2.p; };
	bool operator < ( const TSmartPtr<Type> &p2 ) const { return p < p2.p; };
	bool operator > ( const TSmartPtr<Type> &p2 ) const { return p > p2.p; };

	friend bool operator == ( const TSmartPtr<Type> &p1,int null );
	friend bool operator != ( const TSmartPtr<Type> &p1,int null );
	friend bool operator == ( int null,const TSmartPtr<Type> &p1 );
	friend bool operator != ( int null,const TSmartPtr<Type> &p1 );
};
#endif //WIN64

#ifndef WIN64
template <class T>
inline bool operator == ( const TSmartPtr<T> &p1,int null )	{
	return p1.p == 0;
}

template <class T>
inline bool operator != ( const TSmartPtr<T> &p1,int null )	{
	return p1.p != 0;
}

template <class T>
inline bool operator == ( int null,const TSmartPtr<T> &p1 )	{
	return p1.p == 0;
}

template <class T>
inline bool operator != ( int null,const TSmartPtr<T> &p1 )	{
	return p1.p != 0;
}
#endif //WIN64


#define SMARTPTR_TYPEDEF(Class) typedef TSmartPtr<Class> Class##Ptr

