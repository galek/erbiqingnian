//----------------------------------------------------------------------------
// XmlMessageMapper.cpp
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "XmlMessageMapper.h"


//----------------------------------------------------------------------------
//	XmlMessageMapper METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL XmlMessageMapper::Init(
	MEMORYPOOL * pool )
{
	m_hash.Init();
	m_pool = pool;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void XmlMessageMapper::Free(
	void )
{
	m_hash.Destroy(m_pool);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL XmlMessageMapper::RegisterMessageHandler(
	const WCHAR * messageName,
	FP_SERVER_XMLMESSAGE_CALLBACK handler,
	void *handlerData )
{
	ASSERT_RETFALSE(messageName && messageName[0]);
	ASSERT_RETFALSE(handler);

	CStringHashKey<WCHAR, MAX_PATH> key(messageName);
	
	_HashType * added = m_hash.Add(m_pool, key);
	
	ASSERT_RETFALSE(added);

	added->fpHandler = handler;
	added->pCallbackData = handlerData;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL XmlMessageMapper::DispatchMessage(
	LPVOID context,
	XMLMESSAGE * senderSpec,
	XMLMESSAGE * messageSpec )
{
	ASSERT_RETFALSE(senderSpec && messageSpec);

	CStringHashKey<WCHAR, MAX_PATH> key(messageSpec->MessageName);

	_HashType * handler = m_hash.Get(key);

	if(!handler)
		return FALSE;

	handler->fpHandler(context, senderSpec, messageSpec, handler->pCallbackData);

	return TRUE;
}
