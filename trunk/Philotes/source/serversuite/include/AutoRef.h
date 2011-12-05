//----------------------------------------------------------------------------
// autoref.h
// 
// Modified     : $DateTime: 2008/05/02 10:04:01 $
// by           : $Author: rli $
//
// auto-ref class
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once
#pragma once

class IRefCountable {

    protected:
        long m_refCount;
        IRefCountable():
            m_refCount(1)
        {
            ;
        }
        ~IRefCountable(){;}
    public:
        virtual void AddRef(void) 
        {
            ASSERT(m_refCount > 0);
            if(InterlockedIncrement(&m_refCount) <1) {
                ASSERT(m_refCount > 100); //ASSERT(FALSE);
                m_refCount = 0;
            }
        }
        virtual void Release(void)
        {
            ASSERT(m_refCount > 0);
            if(InterlockedDecrement(&m_refCount)== 0)
            {
                DoFree();
            }
        }
        virtual void DoFree(void) = 0;
};
class AutoRef {
    private:
        IRefCountable * m_pRefObj;
    public:
        AutoRef(IRefCountable *pRefObj)
        {
            m_pRefObj = pRefObj;
            m_pRefObj->AddRef();
        }
        ~AutoRef()
        {
            m_pRefObj->Release();
        }
};

class AutoRelease
{
private:
	IRefCountable* m_pRefObj;

public:
	AutoRelease(IRefCountable *pRefObj)
	{
		m_pRefObj = pRefObj;
	}
	~AutoRelease()
	{
		if (m_pRefObj != NULL) {
			m_pRefObj->Release();
		}
	}
};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_REF_SLOTS 64

class IRefCountableDbg {
    protected:
        IRefCountableDbg():
            m_refCount(1)
        {
            memset(m_refInfo,0,sizeof(m_refInfo));
            m_curSlot = 0;
            m_lock.Init();
        }
        virtual ~IRefCountableDbg()
        {
            m_lock.Free();
        }
      
    public:
        struct RefInfo {
            const char *fileName;
            int  line;
            char opChar;
            long count;
			long opNumber;
        };

    protected:
        long             m_refCount;
        RefInfo          m_refInfo[MAX_REF_SLOTS];
        int              m_curSlot;
        CCriticalSection m_lock;

    public:
        virtual void AddRefDbg(const char *file, int line)
        {
            CSAutoLock lockme(&m_lock);

            ASSERT_RETURN(m_refCount > 0);

            m_refCount++;

			long ii = m_curSlot % MAX_REF_SLOTS;
			++m_curSlot;

            m_refInfo[ii].fileName = file;
            m_refInfo[ii].line = line;
            m_refInfo[ii].opChar = '+';
            m_refInfo[ii].count = m_refCount;
			m_refInfo[ii].opNumber = m_curSlot;
        }
        virtual void ReleaseDbg(char *file, int line)
        {
            ASSERT_RETURN(m_refCount > 0);
            long newCount = 0;

            {
                CSAutoLock lockme(&m_lock);

                newCount = --m_refCount;

                long ii = m_curSlot % MAX_REF_SLOTS;
                ++m_curSlot;

                m_refInfo[ii].fileName = file;
                m_refInfo[ii].line = line;
                m_refInfo[ii].opChar = '-';
                m_refInfo[ii].count = newCount;
                m_refInfo[ii].opNumber = m_curSlot;
            }
            if(newCount == 0)
            {
                DoFree();
            }
        }
		long GetDbgInfo( RefInfo * dest, DWORD destLen )
		{
			ASSERT_RETZERO(dest);
			for(UINT ii = 0; ii < destLen && ii < MAX_REF_SLOTS; ++ii)
			{
				dest[ii] = m_refInfo[ii];
			}
			return MAX_REF_SLOTS;
		}
		long GetRefCount( void )
		{
			return m_refCount;
		}
        virtual void DoFree(void) = 0;

};
class AutoRefDbg {
    private:
        IRefCountableDbg * m_pRefObj;
    public:
        AutoRefDbg(IRefCountableDbg *pRefObj,char *file, int line)
        {
            m_pRefObj = pRefObj;
            m_pRefObj->AddRefDbg(file,line);
        }
        ~AutoRefDbg()
        {
            m_pRefObj->ReleaseDbg(__FILE__,__LINE__);
        }
};

class AutoReleaseDbg
{
private:
	IRefCountableDbg* m_pRefObj;

public:
	AutoReleaseDbg(IRefCountableDbg *pRefObj)
	{
		m_pRefObj = pRefObj;
	}
	~AutoReleaseDbg()
	{
		if (m_pRefObj != NULL) {
			m_pRefObj->ReleaseDbg(__FILE__,__LINE__);
		}
	}
};

#if !defined(_DEBUG)
#define NO_AUTOREF_DBG
#endif

#if !defined(NO_AUTOREF_DBG)
#define IRefCountable IRefCountableDbg
#define AUTOREF(obj)      AutoRefDbg autoRef((obj),__FILE__,__LINE__)
#define AUTORELEASE(obj)  AutoReleaseDbg autoRelease(obj)
#define AddRef()		  AddRefDbg(__FILE__,__LINE__)
#define Release()		  ReleaseDbg(__FILE__,__LINE__)
#else 
#define AUTOREF(obj)      AutoRef autoRef(obj)
#define AUTORELEASE(obj)  AutoRelease autoRelease(obj)
#endif
