
/********************************************************************
	����:		2012��2��21��
	�ļ���: 	RefCountBase.h
	������:		������
	����:		����MFC��ϵ�����ü����ӿ�	
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
