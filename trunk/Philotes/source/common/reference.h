//----------------------------------------------------------------------------
// reference.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#pragma once


namespace FSCommon
{

// Interface IRefCountable
// 
interface IRefCountable
{

	public:
	
		virtual int				IncRef() = 0;
		virtual int				DecRef() = 0;

};


// Class CRefCountable
//
template<bool MT_SAFE>
class CRefCountable : public IRefCountable
{

	public:
	
		CRefCountable(int startCount = 1);
		virtual ~CRefCountable();
		
	public: // IRefCountable
	
		virtual int				IncRef();
		virtual int				DecRef();
		
	protected:
	
		int						m_RefCount;
		
	private:
	
		CCritSectLite			m_RefCountCS;
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CRefCountable Constructor
//
// Parameters
//	startCount : The initial reference count
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE>
CRefCountable<MT_SAFE>::CRefCountable(int startCount) :
	m_RefCount(startCount)
{
	if(MT_SAFE)
	{
		m_RefCountCS.Init();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CRefCountable Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE>
CRefCountable<MT_SAFE>::~CRefCountable()
{
	if(MT_SAFE)
	{
		m_RefCountCS.Free();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	IncRef
//
// Returns
//	The old reference count
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE>
int CRefCountable<MT_SAFE>::IncRef()
{
	CSLAutoLock lock(MT_SAFE ? &m_RefCountCS : NULL);

	int oldRefCount = m_RefCount;
	++m_RefCount;
	
	return oldRefCount;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	DecRef
//
// Returns
//	The new reference count
//
// Remarks
//	Destroys the object if the resulting reference count after decrementing
//	is less than or equal to zero.
//
/////////////////////////////////////////////////////////////////////////////
template<bool MT_SAFE>
int CRefCountable<MT_SAFE>::DecRef()
{
	bool shouldDestroy = false;
	int newRefCount = 0;

	{
		CSLAutoLock lock(MT_SAFE ? &m_RefCountCS : NULL);

		--m_RefCount;
		newRefCount = m_RefCount;
		
		if(newRefCount <= 0)
		{
			shouldDestroy = true;
		}		
	}
	
	if(shouldDestroy)
	{
		this->~CRefCountable();
	}
	
	return newRefCount;
}



} // end namespace FSCommon