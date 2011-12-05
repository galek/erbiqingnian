//----------------------------------------------------------------------------
// ServerMailbox_priv.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _SERVERMAILBOX_PRIV_H_
#define _SERVERMAILBOX_PRIV_H_


//----------------------------------------------------------------------------
// MESSAGE METADATA STRUCT
//----------------------------------------------------------------------------
struct RAW_MSG_METADATA
{
	BOOL					IsRequest;
	BOOL					IsFake;
	SVRTYPE					ServiceProvider;
	SVRTYPE					ServiceUser;
	union
	{
		SVRID				ServiceProviderId;
		SERVICEUSERID		ServiceUserId;
	};
};

struct DUMMY_SIZING_STRUCT : LIST_SL_NODE
{
	DUMMY_SIZING_STRUCT *   next;
	RAW_MSG_METADATA		MsgMetadata;
	UINT					MsgSize;
	UINT					MsgBuffSize;
	BYTE					MsgBuff[1];
};

#define TINY_MSG_BUF_SIZE  ((SMALL_MSG_BUF_SIZE/4) - (sizeof(DUMMY_SIZING_STRUCT) - 1))

//----------------------------------------------------------------------------
// TEMP RAW MSG HELPER STRUCTS
//----------------------------------------------------------------------------
struct RAW_MSG_ENTRY : LIST_SL_NODE
{
	//	mailbox members
	RAW_MSG_ENTRY *			next;

	//	message members
	RAW_MSG_METADATA		MsgMetadata;
	UINT					MsgSize;
	UINT					MsgBuffSize;
	BYTE					MsgBuff[TINY_MSG_BUF_SIZE];
};
struct LARGE_RAW_MSG_ENTRY : LIST_SL_NODE
{
	//	mailbox members
	RAW_MSG_ENTRY *			next;

	//	message members
	RAW_MSG_METADATA		MsgMetadata;
	UINT					MsgSize;
	UINT					MsgBuffSize;
	BYTE					MsgBuff[LARGE_MSG_BUF_SIZE];
};

//----------------------------------------------------------------------------
// INTERNAL SERVER MAILBOX INTERFACE
//----------------------------------------------------------------------------

void
	SvrMailboxIncRefCount(
		SERVER_MAILBOX *	mailbox );

BOOL
	SvrMailboxAddRawMessage(
		SERVER_MAILBOX *	mailbox,
		RAW_MSG_METADATA &	msgMetadata,
		BYTE *				msg,
		DWORD				msgLength );

RAW_MSG_ENTRY *
	SvrMailboxPopRawMessage(
		SERVER_MAILBOX *	mailbox );

void
	SvrMailboxRecycleRawMessage(
		SERVER_MAILBOX *	mailbox,
		RAW_MSG_ENTRY *		rawMsgContainer );

RAW_MSG_ENTRY *
	SvrMailboxPopRawListMessage(
		MAILBOX_MSG_LIST *	mailboxList );

void
	SvrMailboxRecycleRawListMessage(
		MAILBOX_MSG_LIST *	mailbox,
		RAW_MSG_ENTRY *		rawMsgContainer );


//----------------------------------------------------------------------------
// CONVENIENCE SMART MAILBOX POINTER
//----------------------------------------------------------------------------
struct SERVER_MAILBOX_PTR
{
private:
	SERVER_MAILBOX *	m_mailbox;

public:
	//------------------------------------------------------------------------
	void Init(
		void )
	{
		m_mailbox = NULL;
	}
	SERVER_MAILBOX_PTR(
		void )
	{
		this->Init();
	}

	//------------------------------------------------------------------------
	SERVER_MAILBOX * Set(
		SERVER_MAILBOX * mailbox )
	{
		if( m_mailbox ) {
			SvrMailboxRelease( m_mailbox );
		}
		m_mailbox = mailbox;
		if( m_mailbox )
		{
			SvrMailboxIncRefCount( m_mailbox );
		}
		return mailbox;
	}
	SERVER_MAILBOX_PTR(
		SERVER_MAILBOX * mailbox )
	{
		m_mailbox = NULL;
		this->Set( mailbox );
	}
	SERVER_MAILBOX * operator=(
		SERVER_MAILBOX * mailbox )
	{
		return this->Set( mailbox );
	}

	//------------------------------------------------------------------------
	void Free(
		void )
	{
		if( m_mailbox ) {
			SvrMailboxRelease( m_mailbox );
		}
		m_mailbox = NULL;
	}
	~SERVER_MAILBOX_PTR(
		void )
	{
		this->Free();
	}

	//------------------------------------------------------------------------
	SERVER_MAILBOX * Ptr(
		void )
	{
		return m_mailbox;
	}
	operator SERVER_MAILBOX*(
		void )
	{
		return this->Ptr();
	}
};


#endif	//	_SERVERMAILBOX_PRIV_H_
