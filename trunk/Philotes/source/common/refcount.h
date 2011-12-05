//-----------------------------------------------------------------------------
// refcount.h
//
// Copyright (C) 2003 Flagship Studios, All Rights Reserved.
//-----------------------------------------------------------------------------
#ifndef __REFCOUNT_H__
#define __REFCOUNT_H__


#define REFCOUNT_DEBUG					ISVERSION(DEBUG_VERSION)
#define REFCOUNT_DEVELOPMENT			ISVERSION(DEVELOPMENT)

#if REFCOUNT_DEBUG
#define RefCountInit(r, ...)			r.Init(__FILE__, __LINE__, __VA_ARGS__)
#else
#define RefCountInit(r, ...)			r.Init(__VA_ARGS__)
#endif

// Some generically-named #defines are colliding
#undef AddRef
#undef Release


struct REFCOUNT
{
private:
	int									m_nRefCount;
#if REFCOUNT_DEBUG
	const char *						m_File;
	unsigned int						m_Line;
#endif

public:
#if REFCOUNT_DEBUG
	void Init(
		const char * file,
		unsigned int line,
		int nCount = 0)
#else
	void Init(
		int nCount = 0)
#endif
	{
#if REFCOUNT_DEBUG
		m_File = file;
		m_Line = line;
#endif
		m_nRefCount = nCount;
	}

	void AddRef(
		void)
	{
		m_nRefCount++;
	}

	int Release(
		void)
	{
		m_nRefCount--;
#if REFCOUNT_DEVELOPMENT
		if ( ! AppCommonIsAnyFillpak() )			// CML 2007.08.02 - Don't whine about refcount during fillpak
		{
#	if REFCOUNT_DEBUG
			FL_ASSERT(m_nRefCount >= 0, m_File, m_Line);
#	else
			ASSERT(m_nRefCount >= 0);
#	endif	// DEBUG
		}
#endif	// DEVELOPMENT
		m_nRefCount = MAX( m_nRefCount, 0 );
		return m_nRefCount;
	}

	int GetCount(
		void) const
	{
		return m_nRefCount;
	}
};


#endif // __REFCOUNT_H__