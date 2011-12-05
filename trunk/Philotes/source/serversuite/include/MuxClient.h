//----------------------------------------------------------------------------
// MuxClient.h
// 
// Modified     : $DateTime: 2006/04/23 16:39:03 $
// by           : $Author: adeshpande $
//
// Contains interface to the MuxClient object
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once
#include <autoref.h>

typedef DWORD MUXCLIENTID;


//
// This is the function that will be called when demultiplexed data is
// available. The "context" parameter is caller-specific data that is not
// interpreted by the MuxClient implementation.
//
// If dwError != NO_ERROR, the other parameters are all invalid.
//
typedef void (*FP_CLIENT_READ_COMPLETION)(DWORD dwError,
                                         void *context,
                                         MUXCLIENTID muxId,
                                         __in __bcount(dwBufSize) BYTE *pBuffer,
                                         DWORD dwBufSize);
typedef void (*FP_MULTIPOINT_READ_COMPLETION)(DWORD dwError,
                                 __in __deref __ecount(dwCount) void** contexts,
                                             DWORD      dwCount,
                                 __in __bcount(dwBufSize) BYTE *pBuffer,
                                             DWORD dwBufSize);
typedef void (*FP_CLIENT_CONNECT_COMPLETION)(DWORD status,
                                             void *context);

// The caller must fill in a MuxClientConfig structure and pass it to the
// static Create method to get a MuxClient object.
//
struct MuxClientConfig {
    WCHAR         *perfInstanceName; // instance name for perfmon.
    const char    *server_addr; //Address string for server
    unsigned short server_port; // Port on which to connect to the server.
    void          *callback_context; // context for callbacks
    NETCOM        *pNet;
    FP_CLIENT_READ_COMPLETION read_callback; //read callback pointer
    FP_CLIENT_CONNECT_COMPLETION connect_callback; 
};

class IMuxClient : public IRefCountable
{
    protected:

		IMuxClient():m_pPool(NULL){;}
		virtual ~IMuxClient(){;}
		MEMORYPOOL *m_pPool;

    public:

        __checkReturn static IMuxClient* Create(MEMORYPOOL *pool);

        __checkReturn virtual BOOL Connect(MuxClientConfig& config) = 0;


        __checkReturn virtual MUXCLIENTID Attach(
                             __in __notnull void* callback_context,
                                            FP_CLIENT_READ_COMPLETION fp_read,
                                            SOCKADDR_STORAGE& client,
             __in __bcount(dwAcceptDataLen) BYTE *pAcceptData,
                                            DWORD dwAcceptDataLEn) = 0;

        virtual void          Detach(MUXCLIENTID idMux)=0;
        virtual void          SetMultipointCallback(__in __notnull FP_MULTIPOINT_READ_COMPLETION)=0;

        __checkReturn virtual void*         GetContext(void) = 0;

        __checkReturn virtual void*         GetContext(MUXCLIENTID idMux) = 0;

        virtual int                        Send( __in_bcount(size) BYTE *data, 
                                                 DWORD size) = 0;
        virtual int                        Send(MUXCLIENTID idMux,
                               __in_bcount(size) BYTE *data, 
                                                 DWORD size) = 0;

        virtual CONNECT_STATUS GetConnectionStatus(void) = 0;
};
