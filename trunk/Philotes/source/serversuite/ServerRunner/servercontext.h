//----------------------------------------------------------------------------
// servercontext.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _SERVERCONTEXT_H_
#define _SERVERCONTEXT_H_


//----------------------------------------------------------------------------
//	GLOBAL TLS INDEX
//----------------------------------------------------------------------------
extern DWORD g_contextTLSIndex;


//----------------------------------------------------------------------------
// TLS SERVER CONTEXT METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL ServerContextInit(
	void )
{
	ASSERT_RETTRUE(g_contextTLSIndex == TLS_OUT_OF_INDEXES);

	g_contextTLSIndex = TlsAlloc();
	ASSERT_RETFALSE( g_contextTLSIndex != TLS_OUT_OF_INDEXES );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ServerContextFree(
	void )
{
	if(g_contextTLSIndex != TLS_OUT_OF_INDEXES)
	{
		TlsFree( g_contextTLSIndex );
		g_contextTLSIndex = TLS_OUT_OF_INDEXES;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline struct ACTIVE_SERVER * ServerContextGet(
	void )
{
	ASSERT_RETNULL( g_contextTLSIndex != TLS_OUT_OF_INDEXES );

	//	get the entry
	struct ACTIVE_SERVER_ENTRY * entry = (struct ACTIVE_SERVER_ENTRY*)TlsGetValue( g_contextTLSIndex );
	ASSERT_RETNULL( entry );

	BOOL isPrivilagedThread = ( entry->ServerThreadIds.FindExact(GetCurrentThreadId()) != NULL );
	if( isPrivilagedThread || entry->ServerLock.TryIncrement() )
	{
		if( !entry->Server )
		{
			ASSERT_RETNULL(!isPrivilagedThread);	//	should always have an active server for a privileged thread.
			entry->ServerLock.Decrement();
		}
		return entry->Server;
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ServerContextRelease(
	void )
{
	ASSERT_RETURN( g_contextTLSIndex != TLS_OUT_OF_INDEXES );

	//	get the entry
	struct ACTIVE_SERVER_ENTRY * entry = (struct ACTIVE_SERVER_ENTRY*)TlsGetValue( g_contextTLSIndex );
	ASSERT_RETURN( entry );

	if( !entry->ServerThreadIds.FindExact(GetCurrentThreadId()) )
	{
		entry->ServerLock.Decrement();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL ServerContextSet(
	SVRTYPE serverType,
	SVRMACHINEINDEX serverMachineIndex )
{
	ASSERT_RETFALSE( g_contextTLSIndex != TLS_OUT_OF_INDEXES );
	ACTIVE_SERVER_ENTRY * entry = &g_runner->m_servers[serverType][serverMachineIndex];
	ASSERT_RETFALSE( entry );
	ASSERT_RETFALSE( TlsSetValue( g_contextTLSIndex, (LPVOID)entry ) );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL ServerContextSet(
	ACTIVE_SERVER_ENTRY * entry )
{
	ASSERT_RETFALSE( g_contextTLSIndex != TLS_OUT_OF_INDEXES );
	ASSERT_RETFALSE( TlsSetValue( g_contextTLSIndex, (LPVOID)entry ) );
	return TRUE;
}

#endif	//	_SERVERCONTEXT_H_
