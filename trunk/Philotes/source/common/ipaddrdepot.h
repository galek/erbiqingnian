//----------------------------------------------------------------------------
// ipaddrdepot.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
// Desc: storage object for ip addresses
//----------------------------------------------------------------------------
#pragma once


#if ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include <memory>			//	for placement new
#include <hash_set>			//	for container
#include "stlallocator.h"	//	for MEMORYPOOL allocations and deallocations


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

inline size_t hash_value(
	const SOCKADDR_STORAGE & addr )
{
	if(addr.ss_family == AF_INET)
	{
		struct sockaddr_in * addr4 = (struct sockaddr_in*)&addr;
		return addr4->sin_addr.s_addr;
	}
	else if(addr.ss_family == AF_INET6)
	{
		struct sockaddr_in6 * addr6 = (struct sockaddr_in6*)&addr;
		size_t toRet = 0;
		UINT32 * ptr = (UINT32*)addr6->sin6_addr.u.Byte;
		toRet = STR_HASH_ADDC(ptr[0],toRet);
		toRet = STR_HASH_ADDC(ptr[1],toRet);
		toRet = STR_HASH_ADDC(ptr[2],toRet);
		toRet = STR_HASH_ADDC(ptr[3],toRet);
		return toRet;
	}
	ASSERT_MSG("Un-Supported address family sent to hash_value.");
	return (size_t)0;
}

class SOCKADDR_COMPARE 
{
public:
	bool operator()(const SOCKADDR_STORAGE& lhs, const SOCKADDR_STORAGE& rhs) const
	{
		if(lhs.ss_family != rhs.ss_family)
		{
			ASSERT(lhs.ss_family == rhs.ss_family);
			return lhs.ss_family < rhs.ss_family;
		}
		if(lhs.ss_family == AF_INET)
		{
			struct sockaddr_in	*lh4 = (struct sockaddr_in*)&lhs,
								*rh4 = (struct sockaddr_in*)&rhs;
			return lh4->sin_addr.s_addr < rh4->sin_addr.s_addr ;
		}
		if(lhs.ss_family == AF_INET6)
		{
			struct sockaddr_in6	*lh6 = (struct sockaddr_in6*)&lhs,
								*rh6 = (struct sockaddr_in6*)&rhs;
			if(lh6->sin6_scope_id != rh6->sin6_scope_id)
			{
				return (lh6->sin6_scope_id < rh6->sin6_scope_id);
			}
			return (memcmp(&lh6->sin6_addr,&rh6->sin6_addr,sizeof(lh6->sin6_addr)) < 0);
		}
		return false;
	}
};

typedef stdext::hash_compare<SOCKADDR_STORAGE, SOCKADDR_COMPARE> ADDR_COMPARE;
typedef hash_set_mp<SOCKADDR_STORAGE, ADDR_COMPARE>::type ADDR_MAP;


//----------------------------------------------------------------------------
// IPADDRDEPOT
//----------------------------------------------------------------------------
struct IPADDRDEPOT
{
private:
	ADDR_MAP            m_map;
	struct MEMORYPOOL *	m_pool;
public:
	//------------------------------------------------------------------------
	BOOL Init(
		struct MEMORYPOOL * pool )
	{
		new (&m_map) ADDR_MAP(ADDR_MAP::key_compare(), pool);
		m_pool = pool;
		return TRUE;
	}
	//------------------------------------------------------------------------
	void Free(
		void )
	{
		m_map.clear();
		m_map.~ADDR_MAP();
	}
	//------------------------------------------------------------------------
	MEMORYPOOL * GetPool(
		void )
	{
		return m_pool;
	}
	//------------------------------------------------------------------------
	BOOL Add(
		const SOCKADDR_STORAGE & toAdd )
	{
		ASSERT_RETFALSE(toAdd.ss_family==AF_INET || toAdd.ss_family==AF_INET6);
		ADDR_MAP::_Pairib inserted = m_map.insert(toAdd);
		return inserted.second;
	}
	//------------------------------------------------------------------------
	BOOL Remove(
		const SOCKADDR_STORAGE & toRemove )
	{
		return (m_map.erase(toRemove) > 0);
	}
	//------------------------------------------------------------------------
	BOOL HasAddr(
		const SOCKADDR_STORAGE & toFind )
	{
		ADDR_MAP::const_iterator itr = m_map.find(toFind);
		return (itr != m_map.end());
	}
	//------------------------------------------------------------------------
	void Clear(
		void )
	{
		m_map.clear();
	}
	//------------------------------------------------------------------------
	UINT Count(
		void )
	{
		return (UINT)m_map.size();
	}
};


#else	//	ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
// IPADDRDEPOT
// dummy client side version for net to link against.
//----------------------------------------------------------------------------
struct IPADDRDEPOT
{
	BOOL Init(struct MEMORYPOOL *)					{ return TRUE; }
	void Free(void)									{}
	MEMORYPOOL * GetPool(void)						{ ASSERT_MSG("Shouln't be using IPADDRDEPOT on client."); return NULL; }
	BOOL Add(const SOCKADDR_STORAGE &)				{ return TRUE; }
	BOOL Remove(const SOCKADDR_STORAGE &)			{ return TRUE; }
	BOOL HasAddr(const SOCKADDR_STORAGE & )			{ return FALSE; }
	void Clear(void)								{}
	UINT Count(void)								{ return 0; }
};


#endif	//	ISVERSION(SERVER_VERSION)
