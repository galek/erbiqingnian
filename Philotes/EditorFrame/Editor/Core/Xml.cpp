
#include "Xml.h"
#include "SimpleStringPool.h"

static int __cdecl ascii_stricmp( const char * dst, const char * src )
{
	int f, l;
	do
	{
		if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
			f -= 'A' - 'a';
		if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
			l -= 'A' - 'a';
	}
	while ( f && (f == l) );
	return(f - l);
}

XmlStrCmpFunc g_pXmlStrCmp = &ascii_stricmp;

//////////////////////////////////////////////////////////////////////////

class CXmlStringPool : public IXmlStringPool
{
public:
	char* AddString( const char *str ) { return m_stringPool.Append( str,(int)strlen(str) ); }
	void Clear() { m_stringPool.Clear(); }
	void SetBlockSize(unsigned int nBlockSize) { m_stringPool.SetBlockSize(nBlockSize); }

private:
	CSimpleStringPool m_stringPool;
};

//////////////////////////////////////////////////////////////////////////
CXmlNodeReuse::CXmlNodeReuse( const char *tag, CXmlNodePool* pPool ) : m_pPool(pPool)
{
	m_pStringPool = m_pPool->GetStringPool();
	m_pStringPool->AddRef();
	m_tag = m_pStringPool->AddString(tag);
}

void CXmlNodeReuse::Release()
{
	m_pPool->OnRelease(m_nRefCount, this);
	CXmlNode::Release();
}

//////////////////////////////////////////////////////////////////////////

CXmlNodePool::CXmlNodePool(unsigned int nBlockSize)
{
	m_pStringPool = new CXmlStringPool();
	assert(m_pStringPool != 0);

	// in order to avoid memory fragmentation
	// allocates 1Mb buffer for shared string pool
	static_cast<CXmlStringPool*>(m_pStringPool)->SetBlockSize(nBlockSize);
	m_pStringPool->AddRef();
	m_nAllocated = 0;
}

CXmlNodePool::~CXmlNodePool()
{
	while (!m_pNodePool.empty())
	{
		CXmlNodeReuse* pNode = m_pNodePool.top();
		m_pNodePool.pop();
		pNode->Release();
	}
	m_pStringPool->Release();
}

XmlNodeRef CXmlNodePool::GetXmlNode(const char* sNodeName)
{
	CXmlNodeReuse* pNode = 0;

	// NOTE: at the moment xml node pool is dedicated for statistics nodes only

	// first at all check if we have already free node
	if (!m_pNodePool.empty())
	{
		pNode = m_pNodePool.top();
		m_pNodePool.pop();

		// init it to new node name
		pNode->setTag(sNodeName);

		m_nAllocated++;
		//if (0 == m_nAllocated % 1000)
		//CryLog("[CXmlNodePool]: already reused nodes [%d]", m_nAllocated);
	}
	else
	{
		// there is no free nodes so create new one
		// later it will be reused as soon as no external references left
		pNode = new CXmlNodeReuse(sNodeName, this);
		assert(pNode != 0);

		// increase ref counter for reusing node later
		pNode->AddRef();

		m_nAllocated++;
		//if (0 == m_nAllocated % 1000)
		//CryLog("[CXmlNodePool]: already allocated nodes [%d]", m_nAllocated);
	}
	return pNode;
}

void CXmlNodePool::OnRelease(int iRefCount, void* pThis)
{
	// each reusable node call OnRelease before parent release
	// since we keep reference on xml node so when ref count equals
	// to 2 means that it is last external object releases reference
	// to reusable node and it can be save for reuse later
	if (2 == iRefCount)
	{
		CXmlNodeReuse* pNode = static_cast<CXmlNodeReuse*>(pThis);

		pNode->removeAllChilds();
		pNode->removeAllAttributes();

		m_pNodePool.push(pNode);

		// decrease totally allocated by xml node pool counter
		// when counter equals to zero it means that all external
		// objects do not have references to allocated reusable node
		// at that point it is safe to clear shared string pool
		m_nAllocated--;

		if (0 == m_nAllocated)
		{
			//CryLog("[CXmlNodePool]: clear shared string pool");
			static_cast<CXmlStringPool*>(m_pStringPool)->Clear();
		}
	}
}
