//---------------------------------------------------------------------------------------------------------------------
// ServerBase.h
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

//---------------------------------------------------------------------------------------------------------------------
// CServerBase<TYPE, PERF_INSTANCE_T> Interface

template<typename TYPE, typename PERF_INSTANCE_T>
class CServerBase
{
public:
	// REQUIRED: define a public constructor with the following arguments
	// TYPE(SVRID SvrId, MEMORYPOOL* pMemoryPool, const SVRCONFIG* pSvrConfig);

	// REQUIRED: include the following line in implementation file, so the server runner can find server spec
	// SERVER_SPECIFICATION SERVER_SPECIFICATION_NAME(TYPE) = TYPE::GetServerSpecification();
	static SERVER_SPECIFICATION GetServerSpecification(void);

    SVRID GetSvrId(void) const { return SvrId; }
	
protected: // Subclass Interface
	// REQUIRED: define basic server runner configuration parameters
	static const SVRTYPE SvrType; 
	static const bool bHasEmbeddedThread;

	CServerBase(SVRID SvrId, MEMORYPOOL* pMemoryPool);
	virtual ~CServerBase(void);

	// optional: lifetime management functions.  this is generally handled automatically.
	void IncrementRefCount(void) { ::InterlockedIncrement(&iRefCount); }
	void DecrementRefCountAndDeleteThis(void);

	// REQUIRED: DefineServices() defines the services offered and required by initializing a connection table
	static void DefineServices(CONNECTION_TABLE* pConnectionTable); // must be implemented by subclass

	// optional: GetSrvConfigCreator() returns a function that creates a SVRCONFIG to read XML server configuration
	static FP_SVRCONFIG_CREATOR GetSrvConfigCreator(void) { return 0; }
	
	// optional: CreatePerfInstance() returns a PERFINSTANCE_BASE derived object for server performance counters
	static PERF_INSTANCE_T* CreatePerfInstance(SVRINSTANCE /*SvrInstance*/) { return 0; }

	// optional: ProcessXMLMessage() handles administrative messages from the WatchDogServer
	virtual void ProcessXMLMessage(XMLMESSAGE* /*pSenderSpec*/, XMLMESSAGE* /*pMsgSpec*/) { }

	// optional (only if bHasEmbeddedThread): ThreadMain() is the entry point for the server's private thread
	virtual void ThreadMain(void) { }

protected: // Implementation
	CServerBase(const CServerBase&); // non-copyable
	const CServerBase& operator=(const CServerBase&); // non-copyable

	static void* PreNetInit(MEMORYPOOL* pMemoryPool, SVRID SvrId, const SVRCONFIG* pSvrConfig);
	static BOOL InitConnectionTable(CONNECTION_TABLE* pConnectionTable);
	static void SRCommandCallback(void* pServer, SR_COMMAND SRCommand);
	static void XMLMessageCallback(void* pServer, XMLMESSAGE* pSenderSpec, XMLMESSAGE* pMsgSpec, LPVOID userData);
	static void ServerEntryPoint(void* pServer);
												   
	CCriticalSection CriticalSection; // provided for subclass.  this class is thread-safe without it.
    const SVRID SvrId;
	long iRefCount;
	bool bShutDown;

	PERF_INSTANCE_T* pPerfInstance;
	MEMORYPOOL* const pMemoryPool;
};


//---------------------------------------------------------------------------------------------------------------------
// COfferedService<SERVER_TYPE, SERVICE_ID> Interface

template<typename SERVER_TYPE, int SERVICE_ID = 0> // SERVICE_ID is a uniqueifier
class COfferedService
{
public:
	// REQUIRED: must call within server's DefineServices function to publish this service
	static void OfferTo(CONNECTION_TABLE* pConnectionTable, SVRTYPE SvrType);

	// optional: define this function to be notified of clients attaching to offered service
	static FP_NETSVR_CLIENT_ATTACHED GetClientAttachedFxn(void) { return 0; }
	// REQUIRED: you must call SvrNetDisconnectClient() after a client detaches to clean up the connection
	static FP_NETSVR_CLIENT_DETACHED GetClientDetachedFxn(void);

	// REQUIRED: must define command tables for offered service
	static FP_NET_REQUEST_COMMAND_HANDLER RequestHandlerArray[]; // must be implemented by specialization
	static NET_COMMAND_TABLE* BuildRequestCommandTable(void); // must be implemented by specialization
	static NET_COMMAND_TABLE* BuildResponseCommandTable(void); // must be implemented by specialization

	// optional: stub function to allow service object to implement ClientAttached with natural c++ syntax
	template<void* (SERVER_TYPE::*FXN)(SERVICEUSERID, void*, unsigned)>
	static void* ClientAttachedStub(void* pServer, SERVICEUSERID ServiceUserId, BYTE* pAcceptData, DWORD dwAcceptDataLen) 
		{ ASSERT_RETNULL(pServer); return (static_cast<SERVER_TYPE*>(pServer)->*FXN)(ServiceUserId, pAcceptData, dwAcceptDataLen); }

	// optional: stub function to allow service object to implement ClientDetached with natural c++ syntax
	template<void (SERVER_TYPE::*FXN)(SERVICEUSERID, void*)>
	static void ClientDetachedStub(void* pServer, SERVICEUSERID ServiceUserId, void* pClientContext) 
		{ ASSERT_RETURN(pServer); (static_cast<SERVER_TYPE*>(pServer)->*FXN)(ServiceUserId, pClientContext); }

	// optional: stub function to allow service object to implement ProcessRequest functions with natural c++ syntax
	template<typename MSG, void (SERVER_TYPE::*FXN)(SERVICEUSERID, const MSG&, void*)>
	static void ProcessRequestStub(void* pServer, SERVICEUSERID ServiceUserId, MSG_STRUCT* pMsg, void* pClientContext) 
		{ ASSERT_RETURN(pServer && pMsg); (static_cast<SERVER_TYPE*>(pServer)->*FXN)(ServiceUserId, *static_cast<MSG*>(pMsg), pClientContext); }
};


//---------------------------------------------------------------------------------------------------------------------
// CRequiredService<SERVER_TYPE, SERVICE_ID> Interface

template<typename SERVER_TYPE, int SERVICE_ID = 0> // SERVICE_ID is a uniqueifier
class CRequiredService
{
public:
	// REQUIRED: must call within server's DefineServices function to publish this service
	static void RequireFrom(CONNECTION_TABLE* pConnectionTable, SVRTYPE SvrType);

	// REQUIRED: must define command handlers for required service
	static FP_NET_RESPONSE_COMMAND_HANDLER ResponseHandlerArray[]; // must be implemented by specialization

	// optional: define this function to receive raw messages from required services
	// FIXME: add RawReadHandlerStub!
	static FP_NET_RAW_READ_HANDLER GetRawReadHandler(void) { return 0; }

	// optional: stub function to allow service object to implement ProcessResponse functions with natural c++ syntax
	template<typename MSG, void (SERVER_TYPE::*FXN)(SVRID, const MSG&)>
	static void ProcessResponseStub(void* pServer, SVRID SvrId, MSG_STRUCT* pMsg) { 
		ASSERT_RETURN(pServer && pMsg); 
		(static_cast<SERVER_TYPE*>(pServer)->*FXN)(SvrId, *static_cast<MSG*>(pMsg)); 
	}
};


//---------------------------------------------------------------------------------------------------------------------
// Implementation

template<typename TYPE, typename PERF_INSTANCE_T>
/*static*/ SERVER_SPECIFICATION CServerBase<TYPE, PERF_INSTANCE_T>::GetServerSpecification(void)
{
	SERVER_SPECIFICATION ServerSpecification = { 
		SvrType, typeid(TYPE).name(), 
		&PreNetInit, 
		TYPE::GetSrvConfigCreator(), 
		bHasEmbeddedThread ? &ServerEntryPoint : 0, 
		&SRCommandCallback, 
		&XMLMessageCallback, 
		&InitConnectionTable };
	return ServerSpecification;
};


template<typename TYPE, typename PERF_INSTANCE_T>
CServerBase<TYPE, PERF_INSTANCE_T>::CServerBase(SVRID sid, MEMORYPOOL* mp) :
	CriticalSection(true),
    SvrId(sid),
	iRefCount(1),
	bShutDown(false),
	pPerfInstance(TYPE::CreatePerfInstance(ServerIdGetInstance(SvrId))),
	pMemoryPool(mp)
{ }


template<typename TYPE, typename PERF_INSTANCE_T>
/*virtual*/ CServerBase<TYPE, PERF_INSTANCE_T>::~CServerBase(void)
{ 
	ASSERT(bShutDown); 
	ASSERT(iRefCount == 0); 
	if (pPerfInstance)
		PERF_FREE_INSTANCE(pPerfInstance);
}


template<typename TYPE, typename PERF_INSTANCE_T>
void CServerBase<TYPE, PERF_INSTANCE_T>::DecrementRefCountAndDeleteThis(void)
{
	if (::InterlockedDecrement(&iRefCount) <= 0)
		MEMORYPOOL_PRIVATE_DELETE(pMemoryPool, this, CServerBase);
}


template<typename TYPE, typename PERF_INSTANCE_T>
/*static*/ BOOL CServerBase<TYPE, PERF_INSTANCE_T>::InitConnectionTable(CONNECTION_TABLE* pConnectionTable)
{	
	SetServerType(pConnectionTable, SvrType);
	TYPE::DefineServices(pConnectionTable);
	ASSERT_RETFALSE(ValidateTable(pConnectionTable, SvrType));
	return true; 
}


template<typename TYPE, typename PERF_INSTANCE_T>
/*static*/ void* CServerBase<TYPE, PERF_INSTANCE_T>::
	PreNetInit(MEMORYPOOL* pMemoryPool, SVRID SvrId, const SVRCONFIG* pSvrConfig)
{
	try { return MEMORYPOOL_NEW(pMemoryPool) TYPE(SvrId, pMemoryPool, pSvrConfig); }
	catch (...) { return 0; } // we don't use exceptions to report errors!
}


template<typename TYPE, typename PERF_INSTANCE_T>
/*static*/ void CServerBase<TYPE, PERF_INSTANCE_T>::SRCommandCallback(void* pServer, SR_COMMAND SRCommand)
{
	ASSERT_RETURN(pServer);
	if (SRCommand != SR_COMMAND_SHUTDOWN)
		return;
	// the server is responsible for deleting itself after it receives this SR_COMMAND_SHUTDOWN
	// if there are active threads, we have to wait for them all to exit, so we use ref counting.
	CServerBase& This = *static_cast<CServerBase*>(pServer);
	This.bShutDown = true;
	This.DecrementRefCountAndDeleteThis();
}


template<typename TYPE, typename PERF_INSTANCE_T>
/*static*/ void CServerBase<TYPE, PERF_INSTANCE_T>::
	XMLMessageCallback(void* pServer, XMLMESSAGE* pSenderSpec, XMLMESSAGE* pMsgSpec, LPVOID /*userData*/)
{
	ASSERT_RETURN(pServer);
	return static_cast<CServerBase*>(pServer)->ProcessXMLMessage(pSenderSpec, pMsgSpec);
}


template<typename TYPE, typename PERF_INSTANCE_T>
/*static*/ void CServerBase<TYPE, PERF_INSTANCE_T>::ServerEntryPoint(void* pServer)
{								   
	ASSERT_RETURN(pServer);
	CServerBase& This = *static_cast<CServerBase*>(pServer);
	This.IncrementRefCount();
	This.ThreadMain();
	This.DecrementRefCountAndDeleteThis();
}


template<typename SERVER_TYPE, int SERVICE_ID>
/*static*/ void COfferedService<SERVER_TYPE, SERVICE_ID>::OfferTo(CONNECTION_TABLE* pConnectionTable, SVRTYPE SvrType) 
{
	OfferService(pConnectionTable, SvrType, RequestHandlerArray, 
		&BuildRequestCommandTable, &BuildResponseCommandTable, GetClientAttachedFxn(), GetClientDetachedFxn());
}


template<typename SERVER_TYPE, int SERVICE_ID>
/*static*/ void CRequiredService<SERVER_TYPE, SERVICE_ID>::RequireFrom(CONNECTION_TABLE* pConnectionTable, SVRTYPE SvrType) 
	{ RequireService(pConnectionTable, SvrType, ResponseHandlerArray, GetRawReadHandler()); }
