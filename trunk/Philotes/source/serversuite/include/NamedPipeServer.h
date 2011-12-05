//----------------------------------------------------------------------------
// NamedPipeServer.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

struct PIPE_SERVER;
struct PIPE_CONNECTION;

typedef void (*FP_PIPE_SVR_ON_READ)(
					PIPE_SERVER * server,	 // server 
					void * clientContext,	 // context specified in pipe_svr_settings
					BYTE * data,			 // received data pointer
					DWORD  dataLength );	 // received data length

typedef void (*FP_PIPE_SVR_ON_DISCONNECT)(
					PIPE_SERVER * server,	 // server 
					void * clientContext );	 // context specified in pipe_svr_settings

struct pipe_svr_settings
{
	struct MEMORYPOOL *			MemPool;
	const char *				PipeName;
	LPSECURITY_ATTRIBUTES		lpPipeSecurityAttributes;
	DWORD						ReadBufferSize;
	DWORD						WriteBufferSize;
	void *						ServerContext;
	FP_PIPE_SVR_ON_READ			ReadCallback;
	FP_PIPE_SVR_ON_DISCONNECT	DisconnectCallback;
	DWORD						MaxOpenConnections;
};


//----------------------------------------------------------------------------
// PIPE SERVER METHODS
// Simple named pipe listener that accepts one connection.
// Uses completion routines so creating thread must enter an alert able wait
//   state to receive callbacks.
//----------------------------------------------------------------------------

PIPE_SERVER *
	PipeServerCreate(
		pipe_svr_settings & settings );

void
	PipeServerClose(
		PIPE_SERVER * server );

BOOL
	PipeServerOk(
		PIPE_SERVER * server );

HANDLE
	PipeServerGetServerConnectEvent(
		PIPE_SERVER * server );

BOOL
	PipeServerHandleConnect(
		PIPE_SERVER * server );

BOOL
	PipeServerSend(
		PIPE_SERVER * server,
		BYTE * data,
		DWORD  dataLength );

void *
	PipeServerGetContext(
		PIPE_SERVER * server );

void
	PipeServerReleaseContext(
		PIPE_SERVER * server );


//----------------------------------------------------------------------------
// PIPE SERVER CONTEXT SMART POINTER
//----------------------------------------------------------------------------

template<class contextType>
class PipeServerContextPtr
{
private:
	PIPE_SERVER *	m_server;
	void *			m_connectionContext;

public:
	//------------------------------------------------------------------------
	PipeServerContextPtr(
		PIPE_SERVER * server )
	{
		m_server = server;
		m_connectionContext = NULL;
	}
	//------------------------------------------------------------------------
	~PipeServerContextPtr( void )
	{
		this[0] = NULL;
	}

	//------------------------------------------------------------------------
	PipeServerContextPtr &
		operator =(void * clientContext)
	{
		if( m_server &&
			m_connectionContext )
		{
			PipeServerReleaseContext(m_server);
		}
		m_connectionContext = clientContext;
	}
	//------------------------------------------------------------------------
	contextType & operator * ( void ) {
		return *((contextType*)m_connectionContext);
	}
	contextType * operator->( void ) {
		return ((contextType*)m_connectionContext);
	}
	operator contextType * ( void ) {
		return ((contextType*)m_connectionContext);
	}
};
