//----------------------------------------------------------------------------
//	stringhashkey.h
//  Copyright 2006, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _STRINGHASHKEY_H_
#define _STRINGHASHKEY_H_


#ifndef _PSTRING_H_
#include "pstring.h"
#endif


//----------------------------------------------------------------------------
//	NOTE: probably needs debug support...
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
template<class T, unsigned int SIZE> class CStringHashKey;
template<class T, unsigned int SIZE> class CStrRefHashKey;
template<class T, unsigned int SIZE>
	bool operator==(
		CStringHashKey<T,SIZE>&,
		CStrRefHashKey<T,SIZE>& );
template<class T, unsigned int SIZE>
	bool operator==(
		CStrRefHashKey<T,SIZE>&,
		CStringHashKey<T,SIZE>& );


//----------------------------------------------------------------------------

template<class T, unsigned int SIZE>
unsigned int StringHashKey_HashString(
	const T* str ) 
{	//	djb2 hash
	unsigned long hash = 5381;
    unsigned int  c;
	unsigned int  ii(0);
    while ( ( ii++ < SIZE ) && 
			(( c = *str++ ) != NULL) ) 
	{
			hash = ((hash << 5) + hash) + c;	//	hash * 33 + c
	}
    return hash;
}


//----------------------------------------------------------------------------
// CStringHashKey
// Designed to make using strings with hash tables very easy and efficient.
// SIZE is the maximum length of the string to be held.
// !!!Error on NULL strings!!!
// NOTE: T must be a type for which PStrCmp and PStrCopy are defined.
//----------------------------------------------------------------------------
template<class T, unsigned int SIZE>
class CStringHashKey
{
private:

	friend 	bool operator==<T,SIZE>(
		CStringHashKey<T,SIZE>&,
		CStrRefHashKey<T,SIZE>& );
	friend 	bool operator==<T,SIZE>(
		CStrRefHashKey<T,SIZE>&,
		CStringHashKey<T,SIZE>& );

	unsigned int	m_hash;				//	the hash value for the string
	T				m_str[SIZE];		//	the string

	static unsigned int HashString(
		const T* str ) 
	{
		return StringHashKey_HashString<T, SIZE>(str);
	}

public:

	//------------------------------------------------------------------------
	//	init
	//------------------------------------------------------------------------

	CStringHashKey<T,SIZE>& operator=( 
		const T* str )
	{
		m_hash = HashString( str );
		PStrCopy( m_str, str, SIZE );
		return *this;
	}

	void Init( 
		const T* str )
	{
		*this = str;
	}

	CStringHashKey(
		const T* str )
	{
		*this = str;
	}

	CStringHashKey<T,SIZE>& operator=( 
		const CStringHashKey<T,SIZE>& str )
	{
		this->m_hash = str.m_hash;
		PStrCopy( this->m_str, str.m_str, SIZE );
		return *this;
	}

	void Init(
		const CStringHashKey<T,SIZE>& str )
	{
		*this = str;
	}

	CStringHashKey(
		const CStringHashKey<T,SIZE>& str )
	{
		*this = str;
	}

	//	this is included only for compatability, there is no error checking for uninitialized string keys!
	CStringHashKey(
		void )
	{
	}

	//------------------------------------------------------------------------
	//	equality
	//------------------------------------------------------------------------

	bool operator==(
		const T* str ) const
	{
		return ( PStrCmp( m_str, str, SIZE ) == 0 );
	}

	bool operator==(
		const CStringHashKey<T,SIZE>& str ) const
	{
		return ( m_hash == str.m_hash &&
				 PStrCmp( m_str, str.m_str, SIZE ) == 0 );
	}

	//------------------------------------------------------------------------
	//	comparison
	//------------------------------------------------------------------------
	bool operator <(
		const CStringHashKey<T,SIZE> & str ) const
	{
		if(m_hash == str.m_hash)
			return (PStrCmp(m_str,str.m_str,SIZE) < 0);
		return (m_hash < str.m_hash);
	}
	bool operator >(
		const CStringHashKey<T,SIZE> & str ) const
	{
		if(m_hash == str.m_hash)
			return (PStrCmp(m_str,str.m_str,SIZE) > 0);
		return (m_hash > str.m_hash);
	}

	//------------------------------------------------------------------------
	//	data operations
	//------------------------------------------------------------------------

	unsigned int operator%(
		unsigned int mod ) const
	{
		return m_hash % mod;
	}

	T& operator[](
		unsigned int ii )
	{
		return m_str[ii];
	}

	const T & operator[](
		unsigned int ii ) const
	{
		return m_str[ii];
	}

	T* Ptr(
		void )
	{
		return m_str;
	}

	const T * Ptr(
		void ) const
	{
		return m_str;
	}

	//	useful if you can only get the string through a function call that fills a buffer for you.
	void ReHashSelf(
		void )
	{
		m_hash = HashString( m_str );
	}

	unsigned int Hash(
		void ) const
	{
		return m_hash;
	}

	operator size_t () const
	{
		return (size_t)m_hash;
	}
};


//----------------------------------------------------------------------------
// CStrRefHashKey
// Designed to make using strings with hash tables very easy and efficient.
// Safer and more memory efficient for large strings than CStringHashKey as
//		only a pointer to a string located somewhere else in memory is stored.
// SIZE is the maximum length of the string to be pointed to.
// String pointed to must remain in memory pointed to.
// !!!Error on NULL strings!!!
// NOTE: T must be a type for which PStrCmp is defined.
//----------------------------------------------------------------------------
template<class T, unsigned int SIZE>
class CStrRefHashKey
{
private:
	
	friend bool operator==<T,SIZE>(
		CStringHashKey<T,SIZE>&,
		CStrRefHashKey<T,SIZE>& );
	friend bool operator==<T,SIZE>(
		CStrRefHashKey<T,SIZE>&,
		CStringHashKey<T,SIZE>& );

	unsigned int	m_hash;				//	the hash value for the string
	const T*		m_str;				//	the string ref

	static unsigned int HashString(
		const T* str ) 
	{
		return StringHashKey_HashString<T, SIZE>(str);
	}

public:

	//------------------------------------------------------------------------
	//	init
	//------------------------------------------------------------------------

	CStrRefHashKey<T,SIZE>& operator=( 
		const T* str )
	{
		m_hash = HashString( str );
		m_str  = str;
		return *this;
	}

	void Init(
		const T* str )
	{
		*this = str;
	}

	CStrRefHashKey(
		const T* str )
	{
		*this = str;
	}

	CStrRefHashKey<T,SIZE>& operator=( 
		const CStrRefHashKey<T,SIZE>& str )
	{
		this->m_hash = str.m_hash;
		this->m_str  = str.m_str;
		return *this;
	}

	void Init(
		const CStrRefHashKey<T,SIZE>& str )
	{
		*this = str;
	}

	CStrRefHashKey(
		const CStrRefHashKey<T,SIZE>& str )
	{
		*this = str;
	}

	//	this is included only for compatability, there is no error checking for uninitialized string keys!
	CStrRefHashKey(
		void )
	{
	}

	//------------------------------------------------------------------------
	//	equality
	//------------------------------------------------------------------------

	bool operator==(
		const T* str ) const
	{
		return ( m_str == str ||
				 PStrCmp( m_str, str, SIZE ) == 0 );
	}

	bool operator==(
		const CStrRefHashKey<T,SIZE>& str ) const
	{
		return (  m_hash == str.m_hash &&
				( m_str  == str.m_str ||
				  PStrCmp( m_str, str.m_str, SIZE ) == 0 ) );
	}

	//------------------------------------------------------------------------
	//	comparison
	//------------------------------------------------------------------------
	bool operator <(
		const CStrRefHashKey<T,SIZE> & str ) const
	{
		if(m_hash == str.m_hash)
			return (PStrCmp(m_str,str.m_str,SIZE) < 0);
		return (m_hash < str.m_hash);
	}
	bool operator >(
		const CStrRefHashKey<T,SIZE> & str ) const
	{
		if(m_hash == str.m_hash)
			return (PStrCmp(m_str,str.m_str,SIZE) > 0);
		return (m_hash > str.m_hash);
	}

	//------------------------------------------------------------------------
	//	data opperations
	//------------------------------------------------------------------------

	unsigned int operator%(
		const unsigned int mod ) const
	{
		return m_hash % mod;
	}

	T& operator[](
		unsigned int ii )
	{
		return m_str[ii];
	}

	const T & operator[](
		unsigned int ii ) const
	{
		return m_str[ii];
	}

	const T * Ptr(
		void ) const
	{
		return m_str;
	}

	unsigned int Hash(
		void ) const
	{
		return m_hash;
	}
};


//----------------------------------------------------------------------------
// FRIEND FUNCTIONS
// NOTE: included so the two string key variants can be used together.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, unsigned int SIZE>
bool operator==(
		const CStringHashKey<T,SIZE>& left,
		const CStrRefHashKey<T,SIZE>& right )
{
	return( left.m_hash == right.m_hash &&
			PStrCmp( left.m_str, right.m_str, SIZE ) == 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, unsigned int SIZE>
bool operator==(
		const CStrRefHashKey<T,SIZE>& left,
		const CStringHashKey<T,SIZE>& right )
{
	return( left.m_hash == right.m_hash &&
			PStrCmp( left.m_str, right.m_str, SIZE ) == 0 );
}


#endif	//	_STRINGHASHKEY_H_
