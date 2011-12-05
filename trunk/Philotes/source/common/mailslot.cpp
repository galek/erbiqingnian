// ---------------------------------------------------------------------------
// FILE:	mailslot.cpp
// DESC:	locked mail slots library
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// INCLUDE
// ---------------------------------------------------------------------------

#include "mailslot.h"


// ---------------------------------------------------------------------------
// FUNCTIONS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// DESC:	mail slot free
// ---------------------------------------------------------------------------
void MailSlotFree(
	MAILSLOT & slot)
{
	MEMORYPOOL * mempool = NULL;
	MSG_BUF * inbox = NULL;

	slot.inbox_cs.Enter();
	{
		mempool = slot.mempool;
		slot.mempool = NULL;
		slot.msg_count = 0;
		slot.msg_size = 0;

		inbox = slot.inbox_head;
		slot.inbox_head = NULL;
		slot.inbox_tail = NULL;
	}
	slot.inbox_cs.Leave();

	if (inbox)
	{
		MSG_BUF * curr = NULL;
		MSG_BUF * next = inbox;
		while ((curr = next) != NULL)
		{
			next = curr->next;
			FREE(mempool, curr);
		}
	}
}


// ---------------------------------------------------------------------------
// DESC:	initialize a mail slot
// NOTE:	we trust no one is going to muck with it while we're doing this
// ---------------------------------------------------------------------------
void MailSlotInit(
	MAILSLOT & slot,
	MEMORYPOOL * pool,
	unsigned int msg_size)
{
	MailSlotFree(slot);

	memclear(&slot, sizeof(MAILSLOT));
	slot.mempool = pool;
	slot.msg_size = msg_size;
}


// ---------------------------------------------------------------------------
// DESC:	get a message buffer from the mailslot
// ---------------------------------------------------------------------------
MSG_BUF * MailSlotGetBuf(
	MAILSLOT * slot)
{
	ASSERT_RETNULL(slot);

	BOOL bAlreadyFreed = FALSE;

	slot->inbox_cs.Enter();
	{
		bAlreadyFreed = (slot->msg_size == 0);
	}
	slot->inbox_cs.Leave();
	
	if (bAlreadyFreed)
	{
		return NULL;
	}	

	MSG_BUF* msg = (MSG_BUF *)MALLOCZ(slot->mempool, MSG_BUF_HDR_SIZE + slot->msg_size);
	ASSERT_RETNULL(msg);
	
	msg->next = NULL;
	
	return msg;
}


// ---------------------------------------------------------------------------
// DESC:	add a message to the mailslot's inbox (FIFO)
// ---------------------------------------------------------------------------
BOOL MailSlotAddMsg(
	MAILSLOT * slot,
	MSG_BUF * msg)
{
	ASSERT_RETFALSE(slot);
	ASSERT_RETFALSE(msg);

	BOOL bAlreadyFreed = FALSE;

	slot->inbox_cs.Enter();
	{
		bAlreadyFreed = (slot->msg_size == 0);
		if (!bAlreadyFreed)
		{
			if (slot->inbox_tail)
			{
				slot->inbox_tail->next = msg;
			}
			else
			{
				slot->inbox_head = msg;
			}
			slot->inbox_tail = msg;
			slot->msg_count++;
		}
	}
	slot->inbox_cs.Leave();

	if (bAlreadyFreed)
	{
		return FALSE;
	}

	return TRUE;
}


// ---------------------------------------------------------------------------
// DESC:	grab the message list out of the mailslot
// ---------------------------------------------------------------------------
unsigned int MailSlotGetMessages(
	MAILSLOT * slot,
	MSG_BUF * & head,
	MSG_BUF * & tail)
{
	int count = 0;
	head = NULL;
	tail = NULL;
	ASSERT_RETZERO(slot);

	slot->inbox_cs.Enter();
	{
		if (slot->msg_size != 0)
		{
			head = slot->inbox_head;
			tail = slot->inbox_tail;
			count = slot->msg_count;
			slot->inbox_head = NULL;
			slot->inbox_tail = NULL;
			slot->msg_count = 0;
		}
	}
	slot->inbox_cs.Leave();

	return count;
}

//----------------------------------------------------------------------------
// DESC:	remove and return the message at the head of the list, or
//				NULL if empty
//----------------------------------------------------------------------------
MSG_BUF * MailSlotPopMessage(
	MAILSLOT * slot )
{
	ASSERT_RETNULL( slot );

	MSG_BUF * head = NULL;

	slot->inbox_cs.Enter();
	{
		if (slot->msg_size != 0 &&
			slot->inbox_head)
		{
			head = slot->inbox_head;
			slot->inbox_head = slot->inbox_head->next;
			if( !slot->inbox_head )
			{
				slot->inbox_tail = NULL;
			}
			--slot->msg_count;
		}
	}
	slot->inbox_cs.Leave();

	return head;
}

// ---------------------------------------------------------------------------
// DESC:	put message list back in mailbox at front
// ---------------------------------------------------------------------------
void MailSlotPushbackMessages(
	MAILSLOT * slot,
	MSG_BUF * head,
	MSG_BUF * tail)
{
	ASSERT_RETURN(slot);
	if (!head)
	{
		return;
	}
	ASSERT_RETURN(tail);

	// count the number of messages
	int count = 0;
	MSG_BUF * curr = NULL;
	MSG_BUF * next = head;
	while ((curr = next) != NULL)
	{
		++count;
		next = curr->next;
	}
	
	slot->inbox_cs.Enter();
	{
		if (slot->msg_size != 0)
		{
			if (slot->inbox_head)
			{
				tail->next = slot->inbox_head;
			}
			else
			{
				slot->inbox_tail = tail;
				tail->next = NULL;
			}
			slot->inbox_head = head;
			slot->msg_count += count;
		}
	}
	slot->inbox_cs.Leave();
}


// ---------------------------------------------------------------------------
// DESC:	recycle the message list
// ---------------------------------------------------------------------------
void MailSlotRecycleMessages(
	MAILSLOT * slot,
	MSG_BUF * head,
	MSG_BUF * tail)
{
	if (!head)
	{
		return;
	}
	ASSERT_RETURN(slot);
	ASSERT_RETURN(tail);

	slot->inbox_cs.Enter();
	{
		if (slot->msg_size != 0)
		{
			MSG_BUF* current = head;
			while(current)
			{
				MSG_BUF* next = current->next;
				
				FREE(slot->mempool, current);
				
				if(current == tail)
				{
					break;
				}
				
				current = next;
			}
		}
	}
	slot->inbox_cs.Leave();
}
