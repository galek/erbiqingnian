
/********************************************************************
	日期:		2012年2月21日
	文件名: 	RefCountBase.h
	创建者:		王延兴
	描述:		用于MFC体系的引用计数接口	
*********************************************************************/

#pragma once

class CRefCountBase : public CObject
{
public:
	CRefCountBase() { m_nRefCount = 0; };

	int AddRef()
	{
		m_nRefCount++;
		return m_nRefCount;
	};

	int Release()
	{
		int refs = --m_nRefCount;
		if (m_nRefCount <= 0)
			delete this;
		return refs;
	}

protected:
	virtual ~CRefCountBase() {};

private:
	int m_nRefCount;
};

//////////////////////////////////////////////////////////////////////////

template <class ParentClass>
class TRefCountBase : public ParentClass
{
public:
	TRefCountBase() { m_nRefCount = 0; };

	ULONG STDMETHODCALLTYPE AddRef()
	{
		m_nRefCount++;
		return m_nRefCount;
	};


	ULONG STDMETHODCALLTYPE Release()
	{
		int refs = --m_nRefCount;
		if (m_nRefCount <= 0)
			delete this;
		return refs;
	}

protected:
	virtual ~TRefCountBase() {};

private:
	int m_nRefCount;
};
