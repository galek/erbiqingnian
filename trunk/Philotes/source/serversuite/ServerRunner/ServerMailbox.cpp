//----------------------------------------------------------------------------
// ServerMailbox.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdCommonIncludes.h"
#include "servers.h"
#include "ServerRunnerAPI.h"
#include "ServerMailbox_priv.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define SVR_MAILBOX_HELD_MESSAGE_LIMIT		UINT_MAX
#define SVR_MAILBOX_FREE_MESSAGE_LIMIT		200


//----------------------------------------------------------------------------
// MAILBOX_MSG_LIST
// THREAD SAFETY: not thread safe.
//----------------------------------------------------------------------------
struct MAILBOX_MSG_LIST
{
private:
	FREELIST<RAW_MSG_ENTRY> *	m_freeList;
	RAW_MSG_ENTRY *				m_head;
	RAW_MSG_ENTRY *				m_tail;
	UINT						m_msgCount;
	struct MEMORYPOOL *			m_pool;
public:
	void Init(
		__notnull FREELIST<RAW_MSG_ENTRY> * freeList,
		struct MEMORYPOOL * pool )
	{
		DBG_ASSERT( freeList );
		m_freeList = freeList;
		m_head = NULL;
		m_tail = NULL;
		m_msgCount = 0;
		m_pool = pool;
	}
	void Free(
		void )
	{
		if( !m_freeList )
			return;
		RAW_MSG_ENTRY * itr = m_head;
		while( itr )
		{
			RAW_MSG_ENTRY * toFree = itr;
			itr = itr->next;
			if (toFree->MsgBuffSize == TINY_MSG_BUF_SIZE)
				m_freeList->Free( m_pool, toFree );
			else
				FREE(m_pool, toFree);
		}
		m_freeList = NULL;
		m_head = NULL;
		m_tail = NULL;
		m_msgCount = 0;
	}
	BOOL AddMessage(
		MEMORYPOOL *		pool,
		RAW_MSG_METADATA &  metaData,
		__notnull BYTE *	data,
		UINT				dataSize )
	{
		ASSERT_RETFALSE( m_freeList );
		ASSERT_RETFALSE( data );
		ASSERT_RETFALSE( dataSize <= RTL_FIELD_SIZE(LARGE_RAW_MSG_ENTRY, MsgBuff) );
		RAW_MSG_ENTRY * toAdd = NULL;
		
		if (dataSize <= TINY_MSG_BUF_SIZE)
		{
			toAdd = m_freeList->Get( pool, __FILE__, __LINE__ );
			ASSERT_RETFALSE( toAdd );
			toAdd->MsgBuffSize = TINY_MSG_BUF_SIZE;
			ASSERT_RETFALSE(MemoryCopy( toAdd->MsgBuff, TINY_MSG_BUF_SIZE, data, dataSize ));
		}
		else	// (dataSize <= RTL_FIELD_SIZE(LARGE_RAW_MSG_ENTRY,MsgBuff))
		{
			LARGE_RAW_MSG_ENTRY * bigToAdd = (LARGE_RAW_MSG_ENTRY*)MALLOC(m_pool, sizeof(LARGE_RAW_MSG_ENTRY));
			ASSERT_RETFALSE( bigToAdd );
			bigToAdd->MsgBuffSize = RTL_FIELD_SIZE(LARGE_RAW_MSG_ENTRY,MsgBuff);
			ASSERT_RETFALSE(MemoryCopy( bigToAdd->MsgBuff, RTL_FIELD_SIZE(LARGE_RAW_MSG_ENTRY,MsgBuff), data, dataSize ));
			toAdd = (RAW_MSG_ENTRY*)bigToAdd;
		}

		toAdd->MsgMetadata = metaData;
		toAdd->MsgSize = dataSize;
		toAdd->next = NULL;

		if( m_tail )
		{
			m_tail->next = toAdd;
		}
		m_tail = toAdd;
		if( !m_head )
		{
			m_head = toAdd;
		}

		++m_msgCount;

		return TRUE;
	}
	RAW_MSG_ENTRY * PopMessage(
		void )
	{
		RAW_MSG_ENTRY * toRet = NULL;
		if( m_head )
		{
			toRet = m_head;
			m_head = m_head->next;
			if( !m_head )
			{
				m_tail = NULL;
			}
			--m_msgCount;
		}

		return toRet;
	}
	void RecycleMessage(
		__notnull RAW_MSG_ENTRY * rawMsg )
	{
		DBG_ASSERT( rawMsg );
		ASSERT_RETURN( m_freeList );
		if (rawMsg->MsgBuffSize == TINY_MSG_BUF_SIZE)
			m_freeList->Free( m_pool, rawMsg );
		else
			FREE(m_pool, rawMsg);
	}
	UINT HeldMessageCount(
		void )
	{
		return m_msgCount;
	}
};

//----------------------------------------------------------------------------
// mailbox locks
//----------------------------------------------------------------------------
DEF_HLOCK(SvrMailboxLock, HLCriticalSection)
	HLOCK_LOCK_RESTRICTION(ALL_LOCKS_HLOCK_NAME)
END_HLOCK

//----------------------------------------------------------------------------
// SERVER MAILBOX
// THREAD SAFETY: all methods fully thread safe
// DESC: buffed up mailbox implementation capable of holding raw message bytes
//			taken from the net stream as well as the new data required for
//			the server runner framework.
//		 Also provides a waitable semaphore that tracks the message count.
//			Semaphore only created when asked for.
//----------------------------------------------------------------------------
struct SERVER_MAILBOX
{
	//	mailbox lock and ref count
	SvrMailboxLock			m_mailboxLock;
	volatile LONG			m_mailboxRefCount;

	//	message data
	FREELIST<RAW_MSG_ENTRY> m_msgFreeList;
	MAILBOX_MSG_LIST *		m_currentList;
	MAILBOX_MSG_LIST *		m_backupList;

	//	msg count semaphore handle
	HANDLE					m_msgSemaphore;

	//	mem pool
	struct MEMORYPOOL *		m_pool;
};


//----------------------------------------------------------------------------
// PRIVATE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrMailboxFree(
	SERVER_MAILBOX * toFree )
{
	if( !toFree )
	{
		return;
	}

	//	make sure we are clear to destroy
	ASSERT_RETURN( toFree->m_mailboxRefCount == 0 );

	//	free the members
	if( toFree->m_msgSemaphore )
	{
		CloseHandle( toFree->m_msgSemaphore );
		toFree->m_msgSemaphore = NULL;
	}
	toFree->m_mailboxLock.Free();
	if( toFree->m_currentList )
	{
		toFree->m_currentList->Free();
		FREE( toFree->m_pool, toFree->m_currentList );
	}
	if( toFree->m_backupList )
	{
		toFree->m_backupList->Free();
		FREE( toFree->m_pool, toFree->m_backupList );
	}
	toFree->m_msgFreeList.Destroy( toFree->m_pool );

	//	free the box memory
	FREE( toFree->m_pool, toFree );
}


//----------------------------------------------------------------------------
// PUBLIC METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	get a new mailbox
//----------------------------------------------------------------------------
SERVER_MAILBOX * SvrMailboxNew(
	struct MEMORYPOOL * pool )
{
	//	get the memory
	SERVER_MAILBOX * toRet = (SERVER_MAILBOX*)MALLOCZ( pool, sizeof( SERVER_MAILBOX ) );
	ASSERT_RETNULL( toRet );

	//	init the members
	toRet->m_pool				= pool;
	toRet->m_mailboxRefCount	= 1;
	toRet->m_msgSemaphore		= NULL;
	toRet->m_mailboxLock.Init();
	toRet->m_msgFreeList.Init( SVR_MAILBOX_HELD_MESSAGE_LIMIT, SVR_MAILBOX_FREE_MESSAGE_LIMIT );
	toRet->m_currentList		= (MAILBOX_MSG_LIST*)MALLOC(pool,sizeof(MAILBOX_MSG_LIST));
	ASSERT_GOTO( toRet->m_currentList, _NEW_ERROR );
	toRet->m_currentList->Init( &toRet->m_msgFreeList, pool );
	toRet->m_backupList			= (MAILBOX_MSG_LIST*)MALLOC(pool,sizeof(MAILBOX_MSG_LIST));
	ASSERT_GOTO( toRet->m_backupList, _NEW_ERROR );
	toRet->m_backupList->Init( &toRet->m_msgFreeList, pool );

	//	return it
	return toRet;

_NEW_ERROR:
	toRet->m_mailboxLock.Free();
	if( toRet->m_currentList )
	{
		FREE( pool, toRet->m_currentList );
	}
	if( toRet->m_backupList )
	{
		FREE( pool, toRet->m_backupList );
	}
	FREE( pool, toRet );
	return NULL;
}

//----------------------------------------------------------------------------
//	release a handle to a mailbox, not destroyed until refs == 0
//----------------------------------------------------------------------------
void SvrMailboxRelease(
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETURN( mailbox );

	if( InterlockedDecrement( &mailbox->m_mailboxRefCount ) == 0 )
	{
		DBG_ASSERT( mailbox->m_mailboxLock.TryEnter() );	//	if just gave up last ref then shouldn't need to wait for others to release the object
		mailbox->m_mailboxLock.Leave();
		sSvrMailboxFree( mailbox );
		return;
	}
}

//----------------------------------------------------------------------------
//	get a handle to a wait-able semaphore object that is incremented whenever a message is added
//----------------------------------------------------------------------------
HANDLE SvrMailboxGetSemaphoreHandle(
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETNULL( mailbox );
	HANDLE toRet = NULL;

	mailbox->m_mailboxLock.Enter();

	if( !mailbox->m_msgSemaphore && mailbox->m_currentList )
	{
		mailbox->m_msgSemaphore = CreateSemaphore(
										NULL,
										mailbox->m_currentList->HeldMessageCount(),
										LONG_MAX,
										NULL );
	}
	toRet = mailbox->m_msgSemaphore;

	mailbox->m_mailboxLock.Leave();

	DBG_ASSERT( toRet );

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrMailboxHasMoreMessages(
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETFALSE( mailbox );
	BOOL toRet = FALSE;

	mailbox->m_mailboxLock.Enter();
	if( mailbox->m_currentList )
	{
		toRet = ( mailbox->m_currentList->HeldMessageCount() > 0 );
	}
	mailbox->m_mailboxLock.Leave();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrMailboxEmptyMailbox(
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETURN( mailbox );

	mailbox->m_mailboxLock.Enter();
	if( mailbox->m_currentList )
	{
		mailbox->m_currentList->Free();
		mailbox->m_currentList->Init( &mailbox->m_msgFreeList, mailbox->m_pool );
	}
	mailbox->m_mailboxLock.Leave();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrMailboxFreeMailboxBuffers(
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETURN( mailbox );

	mailbox->m_mailboxLock.Enter();
	mailbox->m_msgFreeList.ReleaseAllMemory(mailbox->m_pool);
	mailbox->m_mailboxLock.Leave();
}

//----------------------------------------------------------------------------
//	remove all net messages and place them into a mailbox message list for later processing
//----------------------------------------------------------------------------
MAILBOX_MSG_LIST * SvrMailboxGetAllMessages(
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETNULL( mailbox );
	ASSERTX_RETNULL( mailbox->m_backupList, "SvrMailboxGetAllMessages called before call to SvrMailboxReleaseMessageList" );

	mailbox->m_mailboxLock.Enter();

	//	hand over our primary mail list
	MAILBOX_MSG_LIST * toRet = mailbox->m_currentList;
	mailbox->m_currentList = mailbox->m_backupList;
	mailbox->m_backupList = NULL;

	mailbox->m_mailboxLock.Leave();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrMailboxListHasMoreMessages(
	MAILBOX_MSG_LIST * list )
{
	ASSERT_RETFALSE( list );
	return ( list->HeldMessageCount() > 0 );
}

//----------------------------------------------------------------------------
//	free the list of messages
//----------------------------------------------------------------------------
void SvrMailboxReleaseMessageList(
	SERVER_MAILBOX * mailbox,
	MAILBOX_MSG_LIST * messageList )
{
	ASSERT_RETURN( mailbox );
	ASSERT_RETURN( messageList );

	messageList->Free();
	messageList->Init( &mailbox->m_msgFreeList, mailbox->m_pool );

	mailbox->m_mailboxLock.Enter();

	if( mailbox->m_backupList )
	{
		FREE( mailbox->m_pool, messageList );
		mailbox->m_mailboxLock.Leave();
		ASSERTX_RETURN( (mailbox->m_backupList != NULL), "SvrMailboxReleaseMessageList called with message list not belonging to mailbox." );
	}
	mailbox->m_backupList = messageList;

	mailbox->m_mailboxLock.Leave();
}

//----------------------------------------------------------------------------
// INTERNAL METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrMailboxIncRefCount(
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETURN( mailbox );
	InterlockedIncrement( &mailbox->m_mailboxRefCount );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrMailboxAddRawMessage(
	SERVER_MAILBOX *	mailbox,
	RAW_MSG_METADATA &	msgMetadata,
	BYTE *				msg,
	DWORD				msgLength )
{
	ASSERT_RETFALSE( mailbox );
	ASSERT_RETFALSE( msg );

	mailbox->m_mailboxLock.Enter();

	//	add message
	BOOL success = ( mailbox->m_currentList && 
					 mailbox->m_currentList->AddMessage(
												mailbox->m_pool,
												msgMetadata,
												msg,
												msgLength ) );

	//	signal the semaphore
	if( mailbox->m_msgSemaphore && success )
	{
		ReleaseSemaphore(
			mailbox->m_msgSemaphore,
			1,	//	adding one message
			NULL );
	}

	mailbox->m_mailboxLock.Leave();

	ASSERTX( success, "Error adding raw message to server mailbox." );
	return success;
}

//----------------------------------------------------------------------------
//	!!NOTE!! assumes that the user will deal with waitable semaphore issues.
//		i.e. actually waiting on it	then retrieving only one message per wait.
//----------------------------------------------------------------------------
RAW_MSG_ENTRY * SvrMailboxPopRawMessage(
	SERVER_MAILBOX * mailbox )
{
	ASSERT_RETFALSE( mailbox );
	RAW_MSG_ENTRY * toRet = NULL;

	mailbox->m_mailboxLock.Enter();
	ONCE
	{
		ASSERT_BREAK( mailbox->m_currentList );
		toRet = mailbox->m_currentList->PopMessage();
	}	
	mailbox->m_mailboxLock.Leave();

	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrMailboxRecycleRawMessage(
	SERVER_MAILBOX *	mailbox,
	RAW_MSG_ENTRY *		rawMsg )
{
	ASSERT_RETURN( mailbox );
	ASSERT_RETURN( rawMsg );

	mailbox->m_msgFreeList.Free( mailbox->m_pool, rawMsg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
RAW_MSG_ENTRY * SvrMailboxPopRawListMessage(
	MAILBOX_MSG_LIST *	mailboxList )
{
	ASSERT_RETNULL( mailboxList );
	return mailboxList->PopMessage();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrMailboxRecycleRawListMessage(
	MAILBOX_MSG_LIST *	mailbox,
	RAW_MSG_ENTRY *		rawMsgContainer )
{
	ASSERT_RETURN( mailbox );
	ASSERT_RETURN( rawMsgContainer );
	mailbox->RecycleMessage( rawMsgContainer );
}
