// ---------------------------------------------------------------------------
// FILE: mailslot.h
// ---------------------------------------------------------------------------
#ifndef _MAILSLOT_H_
#define _MAILSLOT_H_
// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// DESC:	mail slot structure
// NOTE:	order of locking should always be inbox_cs : garbage_cs
// ---------------------------------------------------------------------------
struct MAILSLOT
{
	MEMORYPOOL *			mempool;			// memory pool
	unsigned int			msg_size;			// size of messages (0 == non-initialized mailslot)
	unsigned int			msg_count;

	CCritSectLite			inbox_cs;
	MSG_BUF *				inbox_head;
	MSG_BUF *				inbox_tail;
};


// ---------------------------------------------------------------------------
// EXPORTED FUNCTIONS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// DESC:	initialize a mail slot given the memory
// ---------------------------------------------------------------------------
void MailSlotInit(
	MAILSLOT & slot,
	MEMORYPOOL * pool,
	unsigned int msg_size);


// ---------------------------------------------------------------------------
// DESC:	clear a mail slot (free the memory if it was allocated)
// ---------------------------------------------------------------------------
void MailSlotFree(
	MAILSLOT & slot);


// ---------------------------------------------------------------------------
// DESC:	get a message buffer from the mailslot
// ---------------------------------------------------------------------------
MSG_BUF * MailSlotGetBuf(
	MAILSLOT * slot);


// ---------------------------------------------------------------------------
// DESC:	add a message to the mail slot inbox (FIFO)
// ---------------------------------------------------------------------------
BOOL MailSlotAddMsg(
	MAILSLOT * slot,
	MSG_BUF * msg);


// ---------------------------------------------------------------------------
// DESC:	grab the message list out of the mailslot
// ---------------------------------------------------------------------------
unsigned int MailSlotGetMessages(
	MAILSLOT * slot,
	MSG_BUF * & head,
	MSG_BUF * & tail);

//----------------------------------------------------------------------------
// DESC:	remove and return the message at the head of the list, or
//				NULL if empty
//----------------------------------------------------------------------------
MSG_BUF * MailSlotPopMessage(
	MAILSLOT * slot );

// ---------------------------------------------------------------------------
// DESC:	recycle mailslot messages
// ---------------------------------------------------------------------------
void MailSlotRecycleMessages(
	MAILSLOT * slot,
	MSG_BUF * head,
	MSG_BUF * tail);

void MailSlotPushbackMessages(
	MAILSLOT * slot,
	MSG_BUF * head,
	MSG_BUF * tail);

#endif  // _MAILSLOT_H_


