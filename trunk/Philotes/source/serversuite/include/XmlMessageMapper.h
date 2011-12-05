//----------------------------------------------------------------------------
// XmlMessageMapper.h
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once


//----------------------------------------------------------------------------
//	INCLUDES
//----------------------------------------------------------------------------
#ifndef _STRINGHASHKEY_H_
#include "stringhashkey.h"
#endif
#ifndef _HASH_H_
#include "hash.h"
#endif


//----------------------------------------------------------------------------
//	simple command name to message handler mapping manager
//----------------------------------------------------------------------------
struct XmlMessageMapper
{
private:
	struct _HashType
	{
		CStringHashKey<WCHAR, MAX_PATH> id;
		_HashType *						next;
		FP_SERVER_XMLMESSAGE_CALLBACK	fpHandler;
		void *							pCallbackData;
	};
	HASH<_HashType,
		CStringHashKey<WCHAR, MAX_PATH>,
		64>					m_hash;
	struct MEMORYPOOL *		m_pool;

public:

	BOOL Init(
	struct MEMORYPOOL * pool );

	void Free(
		void );

	BOOL RegisterMessageHandler(
		const WCHAR * messageName,
		FP_SERVER_XMLMESSAGE_CALLBACK handler,
		void * handlerData );

	BOOL DispatchMessage(
		LPVOID context,
		XMLMESSAGE * senderSpec,
		XMLMESSAGE * messageSpec );
};
