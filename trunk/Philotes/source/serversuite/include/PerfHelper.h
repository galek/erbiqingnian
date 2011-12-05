#pragma once

// Pretend class. Some encapsulation, but everything is static !
class PerfHelper {

    protected:
        static LIST_ENTRY         m_sListHead[PERFCOUNTER_OBJECT_TYPE_MAX];
        static CCriticalSection   m_slistCS[PERFCOUNTER_OBJECT_TYPE_MAX];
        static LONG               m_Initialized;
        static DWORD              m_dwFirstCounter;
        static DWORD              m_dwFirstHelp;

    public:
        static  BOOL  Init(void);

        __checkReturn static  void *Malloc(unsigned int);
        __checkReturn static  void *CreateInstance(
                                                unsigned int type, 
                                                __in __notnull WCHAR *name);

        static void Free(__in __notnull void *ptr);
        static void Cleanup(void);

    protected:
        static DWORD GetFakeCountersForType(__in __bcount(dwBufSize) BYTE *pBuffer,DWORD dwBufSize,unsigned int type);
        static DWORD GetCountersForType(__in __bcount(dwBufSize) BYTE *pBuffer,DWORD dwBufSize,unsigned int type);
        static BOOL  GetRegistryInformation(void);
        static BOOL  InitSharedMemory(void);
        static void  GetSD(PSECURITY_DESCRIPTOR *ppSD);
        static BOOL  StartWorker(void);
        static DWORD WINAPI sPerfHelperProc(void *ignore);
};
extern LONGLONG FakeInterlockedExchangeAdd64(LONGLONG*,LONGLONG);
#if _M_AMD64
#define NATURAL_TYPE ULONGLONG
#else
#define NATURAL_TYPE DWORD
#endif
