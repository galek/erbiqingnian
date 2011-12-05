//----------------------------------------------------------------------------
// XmlMessageHandler.h
// (C)Copyright 2007, Ping0 LLC. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "markupUnicode.h"
#include "stringhashkey.h"


#define MAX_XML_MESSAGE_COMMANDS			32
#define XMLCMD_HASH_SIZE					32

typedef CStringHashKey<WCHAR, MAX_PATH>		XMLCMD_KEY;

struct _XMLCMD_DEF
{
	XMLCMD_KEY			id;
	_XMLCMD_DEF *		next;

	XMLMESSAGE			command;
};

typedef HASH<	_XMLCMD_DEF,
				XMLCMD_KEY,
				XMLCMD_HASH_SIZE>			XMLCMD_DEF_HASH;

struct _XMLCMD_RECEIVER
{
	XMLCMD_KEY			id;
	_XMLCMD_RECEIVER *	next;
	XMLCMD_DEF_HASH		commands;
};

typedef HASH<	_XMLCMD_RECEIVER,
				XMLCMD_KEY,
				XMLCMD_HASH_SIZE>			XMLCMD_RECEIVER_HASH;

typedef PList<CUnicodeMarkup*>				XMLCMD_LOADED_MARKUPS;

// Misspelled data types  :)
typedef _XMLCMD_RECEIVER					_XMLCMD_RECIEVER;
typedef XMLCMD_RECEIVER_HASH				XMLCMD_RECIEVER_HASH;


class XmlMessageHandler
{

private:
	MEMORYPOOL          *m_pool;

	//	xml message dispatcher
	XMLCMD_RECEIVER_HASH		m_xmlReceiverDefHash;
	XMLCMD_LOADED_MARKUPS		m_xmlMarkupsLoaded;

	static void FP_RECEIVER_DEF_DELETE_ITR(
						MEMORYPOOL * pool,
						_XMLCMD_RECIEVER * def);
	static void FP_LOADED_MARKUP_DELETE_ITR(
						MEMORYPOOL * pool,
						CUnicodeMarkup *& markup);

protected:
	/*static*/ BOOL LoadXMLMESSAGE(
						CUnicodeMarkup * pMarkup,
						XMLMESSAGE * pMessage);
		
	/*static*/ BOOL ParseXmlDefFile(
						CUnicodeMarkup * pMarkup);

	/*static*/ BOOL LoadXmlDefFile(
						const char * filename);

	/*static*/ BOOL ValidateCommand(
						XMLMESSAGE * recipient,
						XMLMESSAGE * command);

public:
	//XmlMessageHandler();

	/*static*/ BOOL InitXmlHandling(
						MEMORYPOOL * pool,
						const char * filepaths);

	void Free(void);

	/*static*/ BOOL ParseAndValidateXmlMessages(
						const WCHAR * wszCommandText,
						CUnicodeMarkup * pMarkup,
						XMLMESSAGE * pSender,
						XMLMESSAGE * pReceiver,
						XMLMESSAGE * pCommands,
						DWORD & nCommands);
};
