//----------------------------------------------------------------------------
// MuxServer.h
// 
// Modified     : $DateTime: 2006/04/23 16:39:03 $
// by           : $Author: adeshpande $
//
// Contains interface to the MuxServer object
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once


typedef DWORD MUXCLIENTID;

static const MUXCLIENTID MUX_INVALID_ID = (DWORD)-1;
static const MUXCLIENTID MUX_MULTIPOINT_ID = MUX_INVALID_ID -1 ;

extern BOOL IsSpecialMuxClientId(MUXCLIENTID id);

class MuxServerImpl;

// accept completion.
//  If error != NO_ERROR, the other parameters are invalid and should be
//  ignored.
typedef BOOL(*FP_MUX_ACCEPT_COMPLETION)(DWORD error,
                                        MUXCLIENTID muxClientId,
										SOCKADDR_STORAGE& local,
                                        SOCKADDR_STORAGE& remote,
										void *pServerContext,
                                        void **ppNewContext,
                                        BYTE *pAcceptData,
                                        DWORD dwAcceptDataLen);

// server read completion
//  If error != NO_ERROR, pBuffer and dwBufSize are not valid.
typedef void (*FP_MUX_SERVER_READ_COMPLETION)(DWORD dwError,
                                             void *context,
                                             MUXCLIENTID muxClientId, 
                                             BYTE *pBuffer,
                                             DWORD dwBufSize);
struct MuxServerConfig {
    WCHAR                         *perfInstanceName;
    const char                    *server_addr;
    unsigned short                 server_port;
    MEMORYPOOL                    *pool;
    NETCOM                        *pNet;
    FP_MUX_ACCEPT_COMPLETION       accept_callback;
    FP_MUX_SERVER_READ_COMPLETION  read_callback;
    void                          *accept_context;
};

class MuxServer
{
    protected:
        MuxServerImpl  *m_pImpl;
        MEMORYPOOL     *m_pPool;

		template<typename T> friend T * MallocNew(MEMORY_FUNCARGS(MEMORYPOOL * pool));
		template<typename T> friend void FreeDelete(MEMORY_FUNCARGS(MEMORYPOOL * pool, const T * ptr));

protected:
        MuxServer();
        ~MuxServer();


    public:
        // server functions
        __checkReturn static MuxServer*   Create(MuxServerConfig& config);

        void                Destroy();

        int                 Send(MUXCLIENTID       muxClientId, 
                                __in_bcount(size)  BYTE *data, 
                                                   DWORD size);

        int                 SendMultipoint(
                                __in_ecount(numClients) MUXCLIENTID *clients,
                                                        DWORD numClients,
                                      __in_bcount(size) BYTE *data, 
                                                        DWORD size);


        BOOL                Disconnect(MUXCLIENTID muxClientId);

        __checkReturn void* GetReadContext(MUXCLIENTID muxClientId);

        __checkReturn void* GetAcceptContext(void);

};
