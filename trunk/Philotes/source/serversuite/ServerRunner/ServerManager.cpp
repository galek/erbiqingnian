//----------------------------------------------------------------------------
// ServerManager.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "list.h"
#include "stringhashkey.h"
#include "hash.h"
#include "dbgtrace.h"
#include "NamedPipeServer.h"
#include "markup.h"
#include "markupUnicode.h"
#include "XmlMessageMapper.h"
#include <watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "version.h"

#include "ServerManager.cpp.tmh"

#if USE_MEMORY_MANAGER
#include "memoryallocator_crt.h"
#include "memoryallocator_pool.h"
using namespace FSCommon;
#endif


//----------------------------------------------------------------------------
// SVRMGR STATE ENUMERATION
//----------------------------------------------------------------------------
enum SVRMGR_STATE
{
	SVRMGR_STATE_LIVE,
	SVRMGR_STATE_DEAD,
	SVRMGR_STATE_STARTING,
	SVRMGR_STATE_QUITING
};


//----------------------------------------------------------------------------
// FORWARD COMMAND HANDLER DEFINITIONS
//----------------------------------------------------------------------------
void SVRMGR_CMD_Exit			(LPVOID, XMLMESSAGE *, XMLMESSAGE *, LPVOID );
void SVRMGR_CMD_SetLocalIp		(LPVOID, XMLMESSAGE *, XMLMESSAGE *, LPVOID );
void SVRMGR_CMD_CheckVersion	(LPVOID, XMLMESSAGE *, XMLMESSAGE *, LPVOID );
void SVRMGR_CMD_RegisterSvrType	(LPVOID, XMLMESSAGE *, XMLMESSAGE *, LPVOID );
void SVRMGR_CMD_RegisterSvr		(LPVOID, XMLMESSAGE *, XMLMESSAGE *, LPVOID );
void SVRMGR_CMD_StartSvr		(LPVOID, XMLMESSAGE *, XMLMESSAGE *, LPVOID );
void SVRMGR_CMD_StopSvr			(LPVOID, XMLMESSAGE *, XMLMESSAGE *, LPVOID );


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

struct SVRMGR_CMD
{
	//	command processing
	FP_SVRMGR_COMMAND_CALLBACK	fpCmdCompletionCallback;
	LPVOID						 pCommandContext;
	BOOL						 bCallerIsWaiting;
	volatile SVRMGR_RESULT *	 pCmdResult;
	HANDLE						 hSemaphore;

	//	command
	WCHAR					   wszCommandText[CMD_PIPE_TEXT_BUFFER_LENGTH];

	//	member void helper, does not handle member cleanup, only assigns void values
	void Clear(
		void )
	{
		wszCommandText[0]		= 0;
		fpCmdCompletionCallback	= NULL;
		bCallerIsWaiting		= FALSE;
		pCmdResult				= NULL;
		hSemaphore				= NULL;
	}
};

typedef PList<SVRMGR_CMD*>	SVRMGR_CMD_LIST;

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

struct _XMLCMD_RECIEVER
{
	XMLCMD_KEY			id;
	_XMLCMD_RECIEVER *	next;
	XMLCMD_DEF_HASH		commands;
};

typedef HASH<	_XMLCMD_RECIEVER,
				XMLCMD_KEY,
				XMLCMD_HASH_SIZE>			XMLCMD_RECIEVER_HASH;

typedef PList<CUnicodeMarkup*>				XMLCMD_LOADED_MARKUPS;


//----------------------------------------------------------------------------
// SVRMGR
// DESC: acts as a sequential command processor for the very top level commands
//			sent to the server runner process.
//			commands may be issued from multiple sources.
//----------------------------------------------------------------------------
struct SVRMGR
{
	//	message queue wait handles
	#define						SVRMGR_WAIT_HANDLE_COUNT			3
	#define						SVRMGR_MESSAGE_QUEUE_INDEX			0
	#define						SVRMGR_UPDATE_TIMER_INDEX			1
	#define						SVRMGR_CMD_PIPE_INDEX				2
	HANDLE						m_waitHandles[ SVRMGR_WAIT_HANDLE_COUNT ];

	//	command queue
	CCriticalSection			m_messageListLock;
	SVRMGR_CMD_LIST				m_messageList;

	//	free lists
	CCriticalSection			m_freeListsLock;
	SVRMGR_CMD_LIST				m_freeMessageList;
	PList<HANDLE>				m_freeSemaphores;

	//	update members
	#define						SVRMGR_UPDATE_PERIOD_MS	( 10 * 1000 )	//	TODO: come up with a reasonable update interval...
	LARGE_INTEGER				m_startTime;
	UINT64						m_lastUpdateTime;
	volatile BOOL				m_shouldExit;

	//	memory pool
	struct MEMORYPOOL *			m_pool;

	//	command pipe server
	char						m_cmdPipeName[MAX_PATH];
	PIPE_SERVER *				m_cmdPipeSvr;
	SECURITY_ATTRIBUTES			m_cmdPipeSecurityAttrs;

	//	xml message dispatcher
	XmlMessageMapper			m_xmlMessageMapper;
	XMLCMD_RECIEVER_HASH		m_xmlRecieverDefHash;
	XMLCMD_LOADED_MARKUPS		m_xmlMarkupsLoaded;
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static SVRMGR *			g_svrmgr	= NULL;
static volatile LONG	g_mgrState	= SVRMGR_STATE_DEAD;


//----------------------------------------------------------------------------
// PRIVATE SERVER MANAGER CONVENIENCE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static HANDLE sSvrManagerGetSemaphore(
	void )
{
	ASSERT_RETNULL( g_svrmgr );
	HANDLE toRet = NULL;

	CSAutoLock lock = &g_svrmgr->m_freeListsLock;

	//	try getting from free list
	BOOL haveFree = g_svrmgr->m_freeSemaphores.PopHead( toRet );
	if( !haveFree )
	{
		//	none in free list, need to create one
		toRet = CreateSemaphore(
					NULL,
					0,
					LONG_MAX,
					NULL );
		ASSERT_RETNULL( toRet );
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSvrManagerFreeSemaphore(
	HANDLE semaphore )
{
	ASSERT_RETURN( g_svrmgr );
	ASSERT_RETURN( semaphore );

	CSAutoLock lock = &g_svrmgr->m_freeListsLock;

	if( !g_svrmgr->m_freeSemaphores.PListPushTailPool( g_svrmgr->m_pool, semaphore ) )
	{
		TraceError("Unable to push semaphore onto free list while releasing server manager wait semaphore, closing handle." );
		CloseHandle(semaphore);
	}
}


//----------------------------------------------------------------------------
// XML MESSAGE HANDLING
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerLoadXMLMESSAGE(
	CUnicodeMarkup * pMarkup,
	XMLMESSAGE * pMessage)
{
	ASSERT_RETFALSE(pMarkup);
	ASSERT_RETFALSE(pMessage);
	structclear(pMessage[0]);

	pMessage->MessageName = pMarkup->GetTagName();

	DWORD childCount = 0;
	while(pMarkup->FindChildElem() && childCount < MAX_XMLMESSAGE_ELEMENTS)
	{
		pMessage->Elements[childCount].ElementName  = pMarkup->GetChildTagName();
		pMessage->Elements[childCount].ElementValue = pMarkup->GetChildData();
		++childCount;
	}
	pMessage->ElementCount = childCount;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerParseXmlDefFile(
	CUnicodeMarkup * pMarkup)
{
	ASSERT_RETFALSE(pMarkup);
	ASSERT_RETFALSE(pMarkup->FindElem());	//		<messages>

	while(pMarkup->FindChildElem())				//		<recipient>
	{
		ASSERT_RETFALSE(pMarkup->IntoElem());	//	->  <recipient>
		const WCHAR * recipName = pMarkup->GetAttrib(L"name");
		ASSERT_RETFALSE(recipName && recipName[0]);
		XMLCMD_KEY recipKey = recipName;

		_XMLCMD_RECIEVER * newReciever = g_svrmgr->m_xmlRecieverDefHash.Add(
												g_svrmgr->m_pool,
												recipKey);
		ASSERT_RETFALSE(newReciever);
		newReciever->commands.Init();

		while(pMarkup->FindChildElem())				//		<command>
		{
			const WCHAR * cmdName = pMarkup->GetChildTagName();
			ASSERT_RETFALSE(cmdName && cmdName[0]);
			XMLCMD_KEY cmdKey = cmdName;

			_XMLCMD_DEF * newCommand = newReciever->commands.Add(
												g_svrmgr->m_pool,
												cmdKey);
			ASSERT_RETFALSE(newCommand);

			ASSERT_RETFALSE(pMarkup->IntoElem());	//	->  <command>
			ASSERT_RETFALSE(sSvrManagerLoadXMLMESSAGE(pMarkup, &newCommand->command));
			ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <command>
		}

		ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <recipient>
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerLoadXmlDefFile(
	const char * filename)
{
	ASSERT_RETFALSE(filename && filename[0]);

	CUnicodeMarkup * pMarkup = NULL;
	HANDLE hFile = CreateFile(
					filename,
					GENERIC_READ,
					0,
					NULL,
					OPEN_EXISTING,
					0,
					NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		TraceWarn(
			TRACE_FLAG_SRUNNER,
			"XML Message Definition file missing. Filename - \"%s\"",
			filename);
		return TRUE;	//	until message config files are situated, ignore missing def files...
	}
	//	ASSERT_RETFALSE(hFile != INVALID_HANDLE_VALUE);

	char * sbBuffer = NULL;
	WCHAR * wcBuffer = NULL;

	DWORD fileSize = GetFileSize(hFile, NULL);
	ASSERT_GOTO(fileSize != INVALID_FILE_SIZE && fileSize > 0, _ERROR);

	sbBuffer = (char*)MALLOC(g_svrmgr->m_pool,fileSize);
	ASSERT_GOTO(sbBuffer, _ERROR);

	wcBuffer = (WCHAR*)MALLOC(g_svrmgr->m_pool,fileSize * sizeof(WCHAR));
	ASSERT_GOTO(wcBuffer, _ERROR);

	DWORD numRead = 0;
	ASSERT_GOTO(ReadFile(hFile,sbBuffer, fileSize, &numRead, NULL), _ERROR);
	ASSERT_GOTO(numRead == fileSize, _ERROR);

	PStrCvt(wcBuffer, sbBuffer, fileSize);

	pMarkup = MALLOC_NEW(g_svrmgr->m_pool, CUnicodeMarkup);
	ASSERT_GOTO(pMarkup, _ERROR);

	ASSERT_GOTO(pMarkup->SetDoc(wcBuffer), _ERROR);
	ASSERT_GOTO(g_svrmgr->m_xmlMarkupsLoaded.PListPushHeadPool(g_svrmgr->m_pool, pMarkup), _ERROR);
	ASSERT_GOTO(sSvrManagerParseXmlDefFile(pMarkup), _ERROR);

	FREE(g_svrmgr->m_pool, sbBuffer);
	CloseHandle(hFile);
	return TRUE;

_ERROR:
	if(sbBuffer)
		FREE(g_svrmgr->m_pool, sbBuffer);
	if(wcBuffer)
		FREE(g_svrmgr->m_pool, wcBuffer);
	if(pMarkup)
		FREE_DELETE(g_svrmgr->m_pool,pMarkup,CUnicodeMarkup);
	CloseHandle(hFile);
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerInitXmlHandling(
	const char * filepaths )
{
	g_svrmgr->m_xmlMessageMapper.Init(g_svrmgr->m_pool);
	g_svrmgr->m_xmlRecieverDefHash.Init();
	g_svrmgr->m_xmlMarkupsLoaded.Init();

	g_svrmgr->m_xmlMessageMapper.RegisterMessageHandler(L"shutdown",			SVRMGR_CMD_Exit, NULL);
	g_svrmgr->m_xmlMessageMapper.RegisterMessageHandler(L"setlocalip",			SVRMGR_CMD_SetLocalIp, NULL);
	g_svrmgr->m_xmlMessageMapper.RegisterMessageHandler(L"checkversion",		SVRMGR_CMD_CheckVersion, NULL);
	g_svrmgr->m_xmlMessageMapper.RegisterMessageHandler(L"registerservertype",	SVRMGR_CMD_RegisterSvrType, NULL);
	g_svrmgr->m_xmlMessageMapper.RegisterMessageHandler(L"registerserver",		SVRMGR_CMD_RegisterSvr, NULL);
	g_svrmgr->m_xmlMessageMapper.RegisterMessageHandler(L"startserver",			SVRMGR_CMD_StartSvr, NULL);
	g_svrmgr->m_xmlMessageMapper.RegisterMessageHandler(L"stopserver",			SVRMGR_CMD_StopSvr, NULL);

	ASSERT_RETFALSE(filepaths && filepaths[0]);
	char   buff[MAX_SVRCONFIG_STR_LEN];
	PStrCopy(buff, filepaths, MAX_SVRCONFIG_STR_LEN);
	char * start = buff;
	char * end = buff;
	BOOL isEndOfString = FALSE;

	while(!isEndOfString)
	{
		while(end[0] != 0 && end[0] != ',')
			++end;
		if(end[0] == 0)
		{
			isEndOfString = TRUE;
		}
		else
		{
			isEndOfString = FALSE;
			end[0] = 0;
			++end;
		}
		ASSERT_RETFALSE(sSvrManagerLoadXmlDefFile(start));
		start = end;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void FP_RECIEVER_DEF_DELETE_ITR(
	MEMORYPOOL * pool,
	_XMLCMD_RECIEVER * def)
{
	ASSERT_RETURN(def);
	def->commands.Destroy(pool);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void FP_LOADED_MARKUP_DELETE_ITR(
	MEMORYPOOL * pool,
	CUnicodeMarkup *& markup)
{
	ASSERT_RETURN(markup);
	markup->Free();
	FREE_DELETE(pool, markup, CUnicodeMarkup);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerFreeXmlHandling(
	void )
{
	g_svrmgr->m_xmlMessageMapper.Free();
	g_svrmgr->m_xmlRecieverDefHash.Destroy(g_svrmgr->m_pool, FP_RECIEVER_DEF_DELETE_ITR);
	g_svrmgr->m_xmlMarkupsLoaded.Destroy(g_svrmgr->m_pool, FP_LOADED_MARKUP_DELETE_ITR);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerValidateCommand(
	XMLMESSAGE * recipient,
	XMLMESSAGE * command)
{
	ASSERT_RETFALSE(recipient && recipient->MessageName && recipient->MessageName[0]);
	ASSERT_RETFALSE(command && command->MessageName && command->MessageName[0]);

	XMLCMD_KEY recipKey = recipient->MessageName;
	XMLCMD_KEY cmdKey = command->MessageName;

	_XMLCMD_RECIEVER * recipientTable = g_svrmgr->m_xmlRecieverDefHash.Get(recipKey);
	if(!recipientTable)
	{
		TraceError(
			"Server manager recieved command for unknown recipient type. Recipient - \"%S\"",
			recipient->MessageName);
		return FALSE;
	}

	_XMLCMD_DEF * cmdDef = recipientTable->commands.Get(cmdKey);
	if(!cmdDef)
	{
		TraceError(
			"Server manager recieved unknown command. Recipient - \"%S\", Command - \"%S\"",
			recipient->MessageName,
			command->MessageName);
		return FALSE;
	}

	XMLMESSAGE * def = &cmdDef->command;
	if(command->ElementCount != def->ElementCount)
	{
		TraceError(
			"Server manager recieved a command that does not match the definition loaded from file. Recipient - \"%S\", Command - \"%S\", Command Element Count - %u",
			recipient->MessageName,
			command->MessageName,
			command->ElementCount);
		return FALSE;
	}

	for(UINT ii = 0; ii < def->ElementCount; ++ii)
	{
		if(PStrCmp(command->Elements[ii].ElementName, def->Elements[ii].ElementName) != 0)
		{
			TraceError(
				"Server manager recieved a command that does not match the definition loaded from file. Recipient - \"%S\", Command - \"%S\", Element Name - \"%S\"",
				recipient->MessageName,
				command->MessageName,
				command->Elements[ii].ElementName);
			return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerParseAndValidateXmlMessages(
	const WCHAR * wszCommandText,
	CUnicodeMarkup * pMarkup,
	XMLMESSAGE * pSender,
	XMLMESSAGE * pReciever,
	XMLMESSAGE * pCommands,
	DWORD & nCommands )
{
	ASSERT_RETFALSE(wszCommandText && wszCommandText[0]);
	ASSERT_RETFALSE(pMarkup);
	ASSERT_RETFALSE(pSender);
	ASSERT_RETFALSE(pReciever);
	ASSERT_RETFALSE(pCommands);
	ASSERT_RETFALSE(nCommands == MAX_XML_MESSAGE_COMMANDS);

	//	load the markup
	ASSERT_RETFALSE(pMarkup->SetDoc((WCHAR*)wszCommandText));

	//	validate the markup
	ASSERT_RETFALSE(pMarkup->FindElem());		//	<cmd>
	ASSERT_RETFALSE(pMarkup->FindChildElem());	//		<hdr>
	ASSERT_RETFALSE(pMarkup->IntoElem());		//	->  <hdr>
		
		ASSERT_RETFALSE(pMarkup->FindChildElem());	//	<source>
		ASSERT_RETFALSE(PStrCmp(L"source", pMarkup->GetChildTagName()) == 0);
		ASSERT_RETFALSE(pMarkup->IntoElem());		//	->  <source>
	
			ASSERT_RETFALSE(pMarkup->FindChildElem());
			ASSERT_RETFALSE(pMarkup->IntoElem());
				ASSERT_RETFALSE(sSvrManagerLoadXMLMESSAGE(pMarkup, pSender));
				ASSERT_RETFALSE(pMarkup->OutOfElem());
			ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <source>

		ASSERT_RETFALSE(pMarkup->FindChildElem());	//	<dest>
		ASSERT_RETFALSE(PStrCmp(L"dest", pMarkup->GetChildTagName()) == 0);
		ASSERT_RETFALSE(pMarkup->IntoElem());		//	->  <dest>

			ASSERT_RETFALSE(pMarkup->FindChildElem());
			ASSERT_RETFALSE(pMarkup->IntoElem());
				ASSERT_RETFALSE(sSvrManagerLoadXMLMESSAGE(pMarkup, pReciever));
				ASSERT_RETFALSE(pMarkup->OutOfElem());
			ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <dest>

		ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <hdr>

	ASSERT_RETFALSE(pMarkup->FindChildElem());	//		<body>
	ASSERT_RETFALSE(pMarkup->IntoElem());		//	->  <body>

		nCommands = 0;
		while(pMarkup->FindChildElem() && nCommands < MAX_XML_MESSAGE_COMMANDS)
		{
			ASSERT_RETFALSE(pMarkup->IntoElem());
				ASSERT_RETFALSE(sSvrManagerLoadXMLMESSAGE(pMarkup, &pCommands[nCommands]));
				ASSERT_RETFALSE(pMarkup->OutOfElem());
			if(!sSvrManagerValidateCommand(pReciever, &pCommands[nCommands]))
				return FALSE;
			++nCommands;
		}
		ASSERT_RETFALSE(nCommands > 0);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerValidateXmlMessage(
	const WCHAR * wszCommandText)
{
	CUnicodeMarkup markup;
	XMLMESSAGE     sender;
	XMLMESSAGE     reciever;
	XMLMESSAGE     commands[MAX_XML_MESSAGE_COMMANDS];
	DWORD          nCommands = MAX_XML_MESSAGE_COMMANDS;
	WCHAR		   commandText[CMD_PIPE_TEXT_BUFFER_LENGTH];
	PStrCopy(commandText, wszCommandText, CMD_PIPE_TEXT_BUFFER_LENGTH);
	return sSvrManagerParseAndValidateXmlMessages(
		commandText,
		&markup,
		&sender,
		&reciever,
		commands,
		nCommands );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerFeedXmlMessageToServer(
	SVRTYPE svrType,
	SVRINSTANCE svrInst,
	SVRMACHINEINDEX svrMIndex,
	__notnull FP_SERVER_XMLMESSAGE_CALLBACK fpHandler,
	LPVOID svrData,
	__notnull XMLMESSAGE * senderSpec,
	__notnull XMLMESSAGE * command )
{
	if (ServerContextSet(svrType, svrMIndex))
	{
		__try
		{
			fpHandler(svrData, senderSpec, command, NULL);
		}
		__except( RunnerExceptFilter(
			GetExceptionInformation(),
			svrType,
			svrInst,
			"Processing server XML message handler.") )
		{
			TraceError("We should never get here but code-analysis complains if this is empty");
		}

		ServerContextSet(NULL);
	}
	else {
		TraceError("Error setting server context for server XML message recipient. Type: %!ServerType!, Inst: %hu, Machine Index: %hu", svrType, svrInst, svrMIndex);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static SVRMGR_RESULT sSvrManagerProcessXmlMessage(
	const WCHAR * wszCommandText)
{
	CUnicodeMarkup markup;
	XMLMESSAGE     sender;
	XMLMESSAGE     reciever;
	XMLMESSAGE     commands[MAX_XML_MESSAGE_COMMANDS];
	DWORD          nCommands = MAX_XML_MESSAGE_COMMANDS;
	
	if(!sSvrManagerParseAndValidateXmlMessages(
			wszCommandText,
			&markup,
			&sender,
			&reciever,
			 commands,
			nCommands ))
	{
		TraceError(
			"Error processing XML message, message structure invalid or commands unknown: \"%S\"",
			wszCommandText);
		return SVRMGR_RESULT_ERROR;
	}

	if( PStrCmp(reciever.MessageName, L"wdc", MAX_PATH) == 0 )
	{
		//	message for the server manager
		for(UINT ii = 0; ii < nCommands; ++ii)
		{
			if(!g_svrmgr->m_xmlMessageMapper.DispatchMessage(
												g_svrmgr,
												&sender,
												&commands[ii]))
			{
				TraceError(
					"No command handler found for WatchdogClient XML message: Name - %S",
					commands[ii].MessageName );
			}
		}
	}
	else if( PStrCmp(reciever.MessageName, L"server", MAX_PATH) == 0 )
	{
		//	message for a server
		FP_SERVER_XMLMESSAGE_CALLBACK fpHandler = NULL;

		//	get the server identifiers
		ASSERT_GOTO(reciever.ElementCount == 2, _INVALID_SERVER_RECIEVER);
		SVRTYPE svrType = (SVRTYPE)ServerGetType(reciever.Elements[0].ElementValue);
		ASSERT_GOTO(svrType != INVALID_SVRTYPE, _INVALID_SERVER_RECIEVER);
		int svrInst = INVALID_SVRINSTANCE;
		ASSERT_GOTO(PStrToInt(reciever.Elements[1].ElementValue, svrInst), _INVALID_SERVER_RECIEVER);
		SVRMACHINEINDEX svrMIndex = NetMapGetServerMachineIndex(
										g_runner->m_netMap,
										ServerId(svrType, (SVRINSTANCE)svrInst));
		ASSERT_GOTO(svrMIndex != INVALID_SVRMACHINEINDEX, _INVALID_SERVER_RECIEVER);

		//	get the server entry
		ACTIVE_SERVER * server = RunnerGetServer(svrType, svrMIndex);
		ASSERT_GOTO(server, _INVALID_SERVER_RECIEVER);

		//	get the handler method
		fpHandler = server->m_specs->svrXmlMessageCallback;
		if(!fpHandler)
		{
			TraceError("Server manager recieved XML message for server that does not handle XML messages: SvrType - %!ServerType!, SvrInstance - %u",
				svrType, svrInst );
			RunnerReleaseServer(svrType, svrMIndex);
			return SVRMGR_RESULT_ERROR;
		}

		//	feed it the messages
		for(UINT ii = 0; ii < nCommands; ++ii)
		{
			sSvrManagerFeedXmlMessageToServer(
				svrType,
				(SVRINSTANCE)svrInst,
				svrMIndex,
				fpHandler,
				server->m_svrData,
				&sender,
				&commands[ii]);
		}

		//	release the server
		RunnerReleaseServer(svrType, svrMIndex);
	}
	else
	{
		TraceError(
			"Server manager recieved XML message for unknown recipient: Message - \"%S\"",
			wszCommandText);
		return SVRMGR_RESULT_ERROR;
	}
	return SVRMGR_RESULT_SUCCESS;

_INVALID_SERVER_RECIEVER:
	TraceError(
		"Server manager recieved XML message with erroneous server recipient specifier: Message - \"%S\"",
		wszCommandText );
	return SVRMGR_RESULT_ERROR;
}


//----------------------------------------------------------------------------
// COMMAND QUEUE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SVRMGR_CMD * sCommandQueueNewCommand(
	void )
{
	ASSERT_RETNULL( g_svrmgr );
	SVRMGR_CMD * toRet = NULL;

	CSAutoLock lock = &g_svrmgr->m_freeListsLock;

	//	try to get one from the free list
	BOOL haveFree = g_svrmgr->m_freeMessageList.PopHead( toRet );
	if( !haveFree )
	{
		//	need to create one
		toRet = (SVRMGR_CMD*)MALLOC( g_svrmgr->m_pool, sizeof( SVRMGR_CMD ) );
		ASSERT_DO( toRet != NULL )
		{
			TraceError("Unable to allocate memory for server manager command.");
			return NULL;
		}

		//	clear the members
		toRet->Clear();
	}
	return toRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sCommandQueuePushCommand(
	SVRMGR_CMD * command )
{
	ASSERT_RETFALSE( g_svrmgr );
	ASSERT_RETFALSE( command );

	CSAutoLock lock = &g_svrmgr->m_messageListLock;

	//	inc the command count and signal the main loop
	ASSERT_RETFALSE( 
		ReleaseSemaphore(
			g_svrmgr->m_waitHandles[ SVRMGR_MESSAGE_QUEUE_INDEX ],
			1,
			NULL ) );

	//	queue the command
	ASSERT_RETFALSE(g_svrmgr->m_messageList.PListPushTailPool( g_svrmgr->m_pool, command));

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCommandQueueFreeCommand(
	SVRMGR_CMD * command )
{
	ASSERT_RETURN( g_svrmgr );
	ASSERT_RETURN( command );

	//	clear the members, any semaphore must be handled by the using thread
	command->Clear();

	CSAutoLock lock = &g_svrmgr->m_freeListsLock;

	//	add it to the free list
	if( !g_svrmgr->m_freeMessageList.PListPushHeadPool( g_svrmgr->m_pool, command ) )
	{
		TraceError("Unable to push server manager command onto the free list, freeing the command memory.");
		FREE(g_svrmgr->m_pool,command);
	}
}

//----------------------------------------------------------------------------
// DESC: optionally blocking message method, places msg in queue for main msg loop.
//----------------------------------------------------------------------------
SVRMGR_RESULT SvrManagerEnqueueMessage(
	const WCHAR *			  wszCommandText,
	BOOL						bSyncronous,
	FP_SVRMGR_COMMAND_CALLBACK fpCompletionCallback,
	LPVOID						pCallbackData)
{
	//	NOTE: not thread safe for shutdown/startup race conditions,
	//		runner starts/closes net after svrmgr init/free... avoids conditions.
	if( g_mgrState != SVRMGR_STATE_LIVE )
		return SVRMGR_RESULT_ERROR;

	ASSERT_RETX(wszCommandText && wszCommandText[0], SVRMGR_RESULT_ERROR);

	volatile SVRMGR_RESULT	toRet		= SVRMGR_RESULT_PENDING;
	HANDLE					waitHandle	= NULL;

	//	get a command
	SVRMGR_CMD * command = sCommandQueueNewCommand();
	ASSERT_RETX( command, SVRMGR_RESULT_ERROR );

	//	copy our args
	PStrCopy( command->wszCommandText, wszCommandText, CMD_PIPE_TEXT_BUFFER_LENGTH );
	command->fpCmdCompletionCallback	= fpCompletionCallback;
	command->bCallerIsWaiting			= bSyncronous;
	command->pCmdResult					= NULL;
	command->pCommandContext			= pCallbackData;
	if( bSyncronous )
	{
		//	get a semaphore to wait on, main loop responsible for signaling it, we have
		//		to be responsible for releasing it to the free list as only we know
		//		when we have acquired it
		command->hSemaphore = sSvrManagerGetSemaphore();
		waitHandle			= command->hSemaphore;
		ASSERT_GOTO( waitHandle, _ENQUEUE_ERROR );

		//	have the main loop set the return code
		command->pCmdResult = &toRet;
	}

	//	queue the command, the main loop now has responsibility for the struct memory
	ASSERT_GOTO( sCommandQueuePushCommand( command ), _ENQUEUE_ERROR);
	//	no longer safe to reference command

	//	handle synchronous wait
	if( bSyncronous )
	{
		WaitForSingleObject( waitHandle, INFINITE );
		sSvrManagerFreeSemaphore( waitHandle );
	}

	return toRet;

_ENQUEUE_ERROR:
	if(command)
	{
		sCommandQueueFreeCommand(command);
	}
	if(waitHandle)
	{
		sSvrManagerFreeSemaphore(waitHandle);
	}
	return SVRMGR_RESULT_ERROR;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerProcessCommand(
	void )
{
	ASSERT_RETURN( g_svrmgr );

	SVRMGR_CMD * command = NULL;
	HANDLE       waitingSemaphore = NULL;

	{
		//	get a command
		CSAutoLock lock = &g_svrmgr->m_messageListLock;
		
		ASSERT_RETURN( g_svrmgr->m_messageList.PopHead( command ) );
		ASSERT_RETURN( command );
		waitingSemaphore = command->hSemaphore;
	}

	//	process the command
	SVRMGR_RESULT result = SVRMGR_RESULT_NONE;
	result = sSvrManagerProcessXmlMessage(command->wszCommandText);

	//	handle user callback
	if( command->fpCmdCompletionCallback )
	{
		command->fpCmdCompletionCallback( result, command->pCommandContext );
	}

	//	handle caller notification
	if( command->pCmdResult )
	{
		(*command->pCmdResult) = result;
	}
		
	//	clean up the command
	sCommandQueueFreeCommand( command );

	//	make sure to release any waiting threads
	if( waitingSemaphore )
	{
		ASSERT( ReleaseSemaphore(
			waitingSemaphore,
			1,
			NULL ) );
	}
}


//----------------------------------------------------------------------------
// COMMAND PIPE METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrManagerSendXMLMessagingMessage(
	WCHAR * messageText )
{
	ASSERT_RETFALSE( g_svrmgr );
	ASSERT_RETFALSE(messageText && messageText[0]);

	if(!sSvrManagerValidateXmlMessage(messageText))
	{
		TraceError(
			"Server manager asked to send unknown or invalid XML message, dropping message: \"%S\"",
			messageText);
		return FALSE;
	}

	TraceVerbose(
		TRACE_FLAG_SRUNNER,
		"Server manager sending XML message: \"%S\"",
		messageText );

	return PipeServerSend(
		g_svrmgr->m_cmdPipeSvr,
		(BYTE*)messageText,
		PStrLen(messageText) * sizeof(WCHAR));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrManagerSendServerStatusUpdate(
	SVRTYPE type,
	SVRINSTANCE inst,
	const WCHAR * status)
{
	WCHAR buff[CMD_PIPE_TEXT_BUFFER_LENGTH];
	PStrPrintf(
		buff,
		CMD_PIPE_TEXT_BUFFER_LENGTH,
		L"<cmd>"
			L"<hdr>"
				L"<source>"
					L"<wdc><ip>%S</ip></wdc>"
				L"</source>"
				L"<dest>"
					L"<servermonitor><type>%S</type><instance>%u</instance></servermonitor>"
				L"</dest>"
			L"</hdr>"
			L"<body>"
				L"<serverstatus><status>%s</status></serverstatus>"
			L"</body>"
		L"</cmd>",
		RunnerNetGetLocalIp(),
		ServerGetName(type),
		inst,
		status);
	return SvrManagerSendXMLMessagingMessage(buff);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrManagerSendServerOpResult(
	SVRTYPE type,
	SVRINSTANCE inst,
	const WCHAR * opType,
	const WCHAR * opResult,
	const WCHAR * opResExplanation)
{
	WCHAR buff[CMD_PIPE_TEXT_BUFFER_LENGTH];
	PStrPrintf(
		buff,
		CMD_PIPE_TEXT_BUFFER_LENGTH,
		L"<cmd>"
			L"<hdr>"
				L"<source>"
					L"<wdc><ip>%S</ip></wdc>"
				L"</source>"
				L"<dest>"
					L"<servermonitor><type>%S</type><instance>%u</instance></servermonitor>"
				L"</dest>"
			L"</hdr>"
			L"<body>"
				L"<opresult>"
					L"<operation>%s</operation>"
					L"<result>%s</result>"
					L"<info>%s</info>"
				L"</opresult>"
			L"</body>"
		L"</cmd>",
		RunnerNetGetLocalIp(),
		ServerGetName(type),
		inst,
		opType,
		opResult,
		opResExplanation);
	return SvrManagerSendXMLMessagingMessage(buff);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerCmdPipeRead(
	PIPE_SERVER * server,
	void * clientContext,
	BYTE * data,
	DWORD  dataLength )
{
	UNREFERENCED_PARAMETER(server);
	UNREFERENCED_PARAMETER(clientContext);
	ASSERT_RETURN(dataLength > 0);
	ASSERT_RETURN(data);

	WCHAR messageText[CMD_PIPE_TEXT_BUFFER_LENGTH];
	PStrCopy(messageText, CMD_PIPE_TEXT_BUFFER_LENGTH, (WCHAR*)data, (dataLength/sizeof(WCHAR)) + 1);

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Server manager recieved XML message. Message - \"%S\"",
		messageText);

	ASSERT(SvrManagerEnqueueMessage(messageText) == SVRMGR_RESULT_PENDING);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerCmdPipeDisconnect(
	PIPE_SERVER * server,
	void * clientContext )
{
	UNREFERENCED_PARAMETER(server);
	UNREFERENCED_PARAMETER(clientContext);

	TraceWarn(
		TRACE_FLAG_SRUNNER,
		"Watchdog server disconnected from command pipe server." );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerGetCmdPipeSecurityAttrs(
	void )
{
	g_svrmgr->m_cmdPipeSecurityAttrs.bInheritHandle = FALSE;
	g_svrmgr->m_cmdPipeSecurityAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
	g_svrmgr->m_cmdPipeSecurityAttrs.lpSecurityDescriptor = NULL;

	WCHAR *szSD = L"D:P"
		L"(A;OICI;GA;;;SY)"
		L"(A;OICI;GA;;;BA)"
		L"(A;OICI;GA;;;NS)"; // Network service FULL CONTROL

	ASSERT_RETFALSE(ConvertStringSecurityDescriptorToSecurityDescriptorW(szSD,
		SDDL_REVISION_1,
		&g_svrmgr->m_cmdPipeSecurityAttrs.lpSecurityDescriptor,
		NULL));

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerFreeCmdPipeSecurityAttrs(
	void )
{
	LocalFree(g_svrmgr->m_cmdPipeSecurityAttrs.lpSecurityDescriptor);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSvrManagerInitCmdPipeServer(
	void )
{
	pipe_svr_settings setup;
	structclear(setup);

	setup.MemPool					= g_svrmgr->m_pool;
	setup.PipeName					= g_svrmgr->m_cmdPipeName;
	setup.lpPipeSecurityAttributes	= &g_svrmgr->m_cmdPipeSecurityAttrs;
	setup.ReadBufferSize			= SVR_MGR_CMD_PIPE_READ_BUFF_SIZE;
	setup.WriteBufferSize			= SVR_MGR_CMD_PIPE_WRITE_BUFF_SIZE;
	setup.ServerContext				= g_svrmgr;
	setup.ReadCallback				= sSvrManagerCmdPipeRead;
	setup.DisconnectCallback		= sSvrManagerCmdPipeDisconnect;
	setup.MaxOpenConnections		= 1;

	g_svrmgr->m_cmdPipeSvr = PipeServerCreate(setup);
	ASSERT_RETFALSE(g_svrmgr->m_cmdPipeSvr);

	return TRUE;
}


//----------------------------------------------------------------------------
// SERVER MANAGER METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSvrManagerSetExit(
						void )
{
	ASSERT_RETURN( g_svrmgr );
	g_svrmgr->m_shouldExit = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sSvrManagerShouldExit(
						   void )
{
	ASSERT_RETTRUE( g_svrmgr );
	return g_svrmgr->m_shouldExit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerProcessUpdate(
									 void )
{
	ASSERT_RETURN( g_svrmgr );
	ASSERT_RETURN( g_runner );


	//	get the delta time
	UINT64 deltaT = GetTickCount() - g_svrmgr->m_lastUpdateTime;

	//	try to connect to missing server services
	RunnerNetAttemptOutstandingConnections();

	if(!PipeServerOk(g_svrmgr->m_cmdPipeSvr))
	{
		PipeServerClose(g_svrmgr->m_cmdPipeSvr);

		TraceInfo(
			TRACE_FLAG_SRUNNER,
			"Establishing named pipe server." );

		if(!sSvrManagerInitCmdPipeServer())
		{
			TraceError(
				"Error while creating named pipe command server for server manager. Exiting..." );
			sSvrManagerSetExit();
			return;
		}

		g_svrmgr->m_waitHandles[SVRMGR_CMD_PIPE_INDEX] = PipeServerGetServerConnectEvent(g_svrmgr->m_cmdPipeSvr);
	}

	//	set the last update time
	g_svrmgr->m_lastUpdateTime += deltaT;
}

//----------------------------------------------------------------------------
//	NOTE: uses waitable timer to avoid wait-time math in main loop
//			to always run an update loop at the same interval...
//----------------------------------------------------------------------------
static BOOL sSvrManagerInitWaitTimer(
	void )
{
	ASSERT_RETFALSE( g_svrmgr );
	ASSERT_RETTRUE( !g_svrmgr->m_waitHandles[SVRMGR_UPDATE_TIMER_INDEX] );

	//	create the timer object
	g_svrmgr->m_waitHandles[SVRMGR_UPDATE_TIMER_INDEX] = CreateWaitableTimer(
																NULL,
																FALSE,
																NULL );
	ASSERT_RETFALSE( g_svrmgr->m_waitHandles[SVRMGR_UPDATE_TIMER_INDEX] );

	//	get the start time
	FILETIME filetime;
	GetSystemTimeAsFileTime(&filetime);
	g_svrmgr->m_startTime.LowPart  = filetime.dwLowDateTime;
	g_svrmgr->m_startTime.HighPart = filetime.dwHighDateTime;
 
	//	set the timer going
	ASSERT_RETFALSE(
			SetWaitableTimer(
				g_svrmgr->m_waitHandles[SVRMGR_UPDATE_TIMER_INDEX],
				&g_svrmgr->m_startTime,
				SVRMGR_UPDATE_PERIOD_MS,
				NULL,
				NULL,
				FALSE ) );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrManagerInit(
	struct MEMORYPOOL * pool,
	const char * cmdPipeName,
	const char * xmlMessagingFilePaths )
{
	SVRMGR_STATE state = (SVRMGR_STATE)InterlockedCompareExchange( &g_mgrState, SVRMGR_STATE_STARTING, SVRMGR_STATE_DEAD );
	//	make sure its not already initialized
	if( state != SVRMGR_STATE_DEAD )
	{
		ASSERT_RETFALSE(state == SVRMGR_STATE_DEAD);
	}
	ASSERT_RETTRUE( !g_svrmgr );

	//	get the singleton memory
	g_svrmgr = (SVRMGR*)MALLOCZ( pool, sizeof( SVRMGR ) );
	ASSERT_GOTO( g_svrmgr, _ERROR );

	//	init the members
	g_svrmgr->m_pool					= pool;
	g_svrmgr->m_messageListLock.Init();
	g_svrmgr->m_messageList.Init();
	g_svrmgr->m_freeListsLock.Init();
	g_svrmgr->m_freeMessageList.Init();
	g_svrmgr->m_freeSemaphores.Init();
	g_svrmgr->m_lastUpdateTime			= GetTickCount();
	g_svrmgr->m_shouldExit				= FALSE;
	PStrCopy(g_svrmgr->m_cmdPipeName, cmdPipeName, MAX_PATH);

	ASSERT_GOTO(sSvrManagerInitXmlHandling(xmlMessagingFilePaths), _ERROR);

	ASSERT_GOTO(sSvrManagerGetCmdPipeSecurityAttrs(), _ERROR);

	ASSERT_GOTO(sSvrManagerInitCmdPipeServer(), _ERROR);

	//	init the wait handles
	g_svrmgr->m_waitHandles[SVRMGR_MESSAGE_QUEUE_INDEX]	= sSvrManagerGetSemaphore();
	ASSERT_GOTO( g_svrmgr->m_waitHandles[SVRMGR_MESSAGE_QUEUE_INDEX], _ERROR );
	ASSERT_GOTO( sSvrManagerInitWaitTimer(), _ERROR );
	g_svrmgr->m_waitHandles[SVRMGR_CMD_PIPE_INDEX] = PipeServerGetServerConnectEvent( g_svrmgr->m_cmdPipeSvr );
	ASSERT_GOTO(g_svrmgr->m_waitHandles[SVRMGR_CMD_PIPE_INDEX], _ERROR);

	//	set manager state
	g_mgrState = SVRMGR_STATE_LIVE;

	TraceInfo(TRACE_FLAG_SRUNNER,"Server manager initialized.");

	return TRUE;

_ERROR:
	SvrManagerFree();
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerLocalCommandDestItr(
	struct MEMORYPOOL * pool,
	SVRMGR_CMD *& cmd )
{
	if( cmd )
	{
		FREE( pool, cmd );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSvrManagerFreeSemaphoreDestItr(
	struct MEMORYPOOL * pool,
	HANDLE &			handle )
{
	UNREFERENCED_PARAMETER(pool);
	if( handle )
	{
		CloseHandle( handle );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrManagerFree(
	void )
{
	if( !g_svrmgr ) {
		return;
	}

	g_mgrState = SVRMGR_STATE_QUITING;

	TraceInfo(TRACE_FLAG_SRUNNER,"Closing server manager.");

	//	free the pipe server
	PipeServerClose(g_svrmgr->m_cmdPipeSvr);

	//	free the XML message handling members
	sSvrManagerFreeXmlHandling();

	//	free the wait handles
	CloseHandle( g_svrmgr->m_waitHandles[SVRMGR_MESSAGE_QUEUE_INDEX] );
	CloseHandle( g_svrmgr->m_waitHandles[SVRMGR_UPDATE_TIMER_INDEX] );

	//	free the members
	g_svrmgr->m_freeListsLock.Free();
	g_svrmgr->m_freeSemaphores.Destroy( g_svrmgr->m_pool, sSvrManagerFreeSemaphoreDestItr );
	g_svrmgr->m_freeMessageList.Destroy( g_svrmgr->m_pool, sSvrManagerLocalCommandDestItr );
	g_svrmgr->m_messageListLock.Free();
	g_svrmgr->m_messageList.Destroy( g_svrmgr->m_pool, sSvrManagerLocalCommandDestItr );

	sSvrManagerFreeCmdPipeSecurityAttrs();

	//	free the singleton memory
	SVRMGR * toFree = g_svrmgr;
	g_svrmgr = NULL;
	FREE( toFree->m_pool, toFree );
	g_mgrState = SVRMGR_STATE_DEAD;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrManagerMainMsgLoop(
			void )
{
	ASSERT_RETURN( g_svrmgr );
    DWORD timeout = INFINITE;

	do
	{
		//	wait for a command or timed update
		DWORD waitResult = WaitForMultipleObjectsEx(
								SVRMGR_WAIT_HANDLE_COUNT,
								g_svrmgr->m_waitHandles,
								FALSE,
								timeout,
								TRUE );

		//	handle the wait result
		switch( waitResult )
		{
			//	process a command
			case ( WAIT_OBJECT_0 + SVRMGR_MESSAGE_QUEUE_INDEX ):
				sSvrManagerProcessCommand();
				break;

			//	do an update loop
			case ( WAIT_OBJECT_0 + SVRMGR_UPDATE_TIMER_INDEX ):
				sSvrManagerProcessUpdate();
				break;

			//	handle the command pipe connect
			case ( WAIT_OBJECT_0 + SVRMGR_CMD_PIPE_INDEX ):
				if( !PipeServerHandleConnect( g_svrmgr->m_cmdPipeSvr ) )
				{
					TraceError("Error accepting connection on command pipe for server manager.");
				}
				break;

			//	handle the execution of IO completion routines
			case WAIT_IO_COMPLETION:
				break;
			//	error
			case ( WAIT_FAILED ):
			default:
				TraceError("Error while waiting on server manager wait objects.");
				sSvrManagerSetExit();
				break;
		};
	} while( !sSvrManagerShouldExit() );
}


//----------------------------------------------------------------------------
// COMMAND HANDLERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//	<shutdown>
//		<reason/>
//	</shutdown>
//----------------------------------------------------------------------------
void SVRMGR_CMD_Exit(
	LPVOID,
	XMLMESSAGE *,
	XMLMESSAGE * msg,
	LPVOID /*userData*/)
{
	if( InterlockedCompareExchange( &g_mgrState, SVRMGR_STATE_QUITING, SVRMGR_STATE_LIVE ) != SVRMGR_STATE_LIVE )
	{
		//	already dead or exiting
		return;
	}

	TraceWarn(
		TRACE_FLAG_SRUNNER,
		"Server manager handling exit command. Explanation - \"%S\"",
		msg->Elements[0].ElementValue );

	//	sets the flag allowing the main loop to exit
	sSvrManagerSetExit();
}

//----------------------------------------------------------------------------
//	<setlocalip>
//		<ip/>
//	</setlocalip>
//----------------------------------------------------------------------------
void SVRMGR_CMD_SetLocalIp(
	LPVOID,
	XMLMESSAGE *,
	XMLMESSAGE * msg,
	LPVOID /*userData*/ )
{
	char localIp[MAX_NET_ADDRESS_LEN];
	PStrCvt( localIp, msg->Elements[0].ElementValue, MAX_NET_ADDRESS_LEN );
	ASSERT_RETURN(localIp[0]);

	if(RunnerNetGetLocalIp() && RunnerNetGetLocalIp()[0])
	{
		TraceError(
			"Server manager recieved set local IP message when local IP already set. Current IP - %s, Sent IP - %s",
			RunnerNetGetLocalIp(),
			localIp);
		return;
	}

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Server manager recieved set local IP message. IP - \"%s\"",
		localIp);

	ASSERT(RunnerNetSetLocalIp(localIp));
}

//----------------------------------------------------------------------------
//	<checkversion>
//		<version/>
//	</checkversion>
//----------------------------------------------------------------------------
void SVRMGR_CMD_CheckVersion(
	LPVOID,
	XMLMESSAGE *,
	XMLMESSAGE * msg,
	LPVOID /*userData*/ )
{
	WCHAR wszOurVersion[64];
	PStrPrintf(wszOurVersion, 64, L"%d.%d.%d.%d", 
		TITLE_VERSION,
		PRODUCTION_BRANCH_VERSION,
		RC_BRANCH_VERSION,
		MAIN_LINE_VERSION);

	if (PStrICmp(wszOurVersion, msg->Elements[0].ElementValue, 64) != 0)
	{
		TraceError(
			"!!WARNING!! WatchdogServer thinks we should be version %S, but we are really version %S.",
			msg->Elements[0].ElementValue, wszOurVersion);
	}
	else
	{
		TraceInfo(TRACE_FLAG_SRUNNER,
			"Passed version check. Version: %S", wszOurVersion);
	}

	WCHAR buff[CMD_PIPE_TEXT_BUFFER_LENGTH];
	PStrPrintf(
		buff,
		CMD_PIPE_TEXT_BUFFER_LENGTH,
		L"<cmd>"
			L"<hdr>"
				L"<source>"
					L"<wdc><ip>%S</ip></wdc>"
				L"</source>"
				L"<dest>"
					L"<wdcmonitor><ip>%S</ip></wdcmonitor>"
				L"</dest>"
			L"</hdr>"
			L"<body>"
				L"<checkversion><version>%s</version></checkversion>"
			L"</body>"
		L"</cmd>",
		RunnerNetGetLocalIp(),
		RunnerNetGetLocalIp(),
		wszOurVersion);
	ASSERT(SvrManagerSendXMLMessagingMessage(buff));
}

//----------------------------------------------------------------------------
//	<registerservertype>
//		<type/>
//		<typecount/>
//	</registerservertype>
//----------------------------------------------------------------------------
void SVRMGR_CMD_RegisterSvrType(
	LPVOID,
	XMLMESSAGE *,
	XMLMESSAGE * msg,
	LPVOID /*userData*/ )
{
	SVRTYPE svrType = (SVRTYPE)ServerGetType(msg->Elements[0].ElementValue);
	if(svrType == INVALID_SVRTYPE)
	{
		TraceError(
			"Server manager recieved register server type command with invalid server type. Type - \"%S\"",
			msg->Elements[0].ElementValue );
		return;
	}

	int     svrCount = 0;
	if(!PStrToInt(msg->Elements[1].ElementValue, svrCount))
	{
		TraceError(
			"Server manager recieved register server type command with invalid count value. Count - \"%S\"",
			msg->Elements[1].ElementValue );
		return;
	}

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Server manager handling register server type command for server %!ServerType! with instance count %u.",
		svrType, svrCount );

	NetMapSetServerInstanceCount(
		g_runner->m_netMap,
		svrType,
		svrCount );
}

//----------------------------------------------------------------------------
//	<registerserver>
//		<type/>
//		<instance/>
//		<machineindex/>
//		<ip/>
//	</registerserver>
//----------------------------------------------------------------------------
void SVRMGR_CMD_RegisterSvr(
	LPVOID ,
	XMLMESSAGE * ,
	XMLMESSAGE * msg, 
	LPVOID /*userData*/)
{
	SVRTYPE svrType = (SVRTYPE)ServerGetType(msg->Elements[0].ElementValue);
	if(svrType == INVALID_SVRTYPE)
	{
		TraceError(
			"Server manager recieved register server command with invalid server type. Type - \"%S\"",
			msg->Elements[0].ElementValue );
		return;
	}

	SVRINSTANCE svrInstance;
	int inst = 0;
	if(!PStrToInt(msg->Elements[1].ElementValue, inst))
	{
		TraceError(
			"Server manager recieved register server command with invalid server instance. Instance - \"%S\"",
			msg->Elements[1].ElementValue );
		return;
	}
	svrInstance = (SVRINSTANCE)inst;

	SVRMACHINEINDEX svrMIndex;
	int mi = 0;
	if(!PStrToInt(msg->Elements[2].ElementValue, mi))
	{
		TraceError(
			"Server manager recieved register server command with invalid server machine index. Machine Index - \"%S\"",
			msg->Elements[2].ElementValue );
		return;
	}
	svrMIndex = (SVRMACHINEINDEX)mi;

	SOCKADDR_STORAGE svrAddr;
	structclear(svrAddr);
	char sbAddr[MAX_NET_ADDRESS_LEN];
	PStrCvt(sbAddr, msg->Elements[3].ElementValue, MAX_NET_ADDRESS_LEN);
	if(!RunnerNetStringToSockAddr(sbAddr, &svrAddr))
	{
		TraceError(
			"Server manager recieved register server command with invalid server ip address. IP - \"%S\"",
			msg->Elements[3].ElementValue);
		return;
	}

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Server manager handling register cluster server command for server %!ServerType! instance %hu at ip %!IPADDR!.",
		svrType, svrInstance, ((sockaddr_in*)&svrAddr)->sin_addr.s_addr );

	//	fill out a server spec
	SERVER server;
	server.svrType			= svrType;
	server.svrInstance		= svrInstance;
	server.svrMachineIndex	= svrMIndex;
	server.svrIpAddress		= svrAddr;

	//	add the server to the map
	if(!NetMapAddServer(g_runner->m_netMap, server))
	{
		TraceError("Error while registering server with the server runner network map.");
	}
}

//----------------------------------------------------------------------------
//	<startserver>
//		<type/>
//		<instance/>
//		<machineindex/>
//		<configdata/>
//	</startserver>
//----------------------------------------------------------------------------
void SVRMGR_CMD_StartSvr(
	LPVOID context,
	XMLMESSAGE * sender,
	XMLMESSAGE * msg,
	LPVOID /*userData*/ )
{
    WCHAR szPoolDescription[MAX_PERF_INSTANCE_NAME];

	SVRTYPE svrType = (SVRTYPE)ServerGetType(msg->Elements[0].ElementValue);
	if(svrType == INVALID_SVRTYPE)
	{
		TraceError(
			"Server manager recieved start server command with invalid server type. Type - \"%S\"",
			msg->Elements[0].ElementValue );
		return;
	}

	SVRINSTANCE svrInstance;
	int inst = 0;
	if(!PStrToInt(msg->Elements[1].ElementValue, inst))
	{
		TraceError(
			"Server manager recieved start server command with invalid server instance. Instance - \"%S\"",
			msg->Elements[1].ElementValue );
		return;
	}
	svrInstance = (SVRINSTANCE)inst;

	SVRMACHINEINDEX svrMIndex;
	int mi = 0;
	if(!PStrToInt(msg->Elements[2].ElementValue, mi) ||
		mi >= MAX_SVR_INSTANCES_PER_MACHINE )
	{
		TraceError(
			"Server manager recieved start server command with invalid server machine index. Machine Index - \"%S\"",
			msg->Elements[2].ElementValue );
		return;
	}
	svrMIndex = (SVRMACHINEINDEX)mi;

	char svrConfigData[MAX_SVRCONFIG_STR_LEN];
	PStrPrintf(svrConfigData, MAX_SVRCONFIG_STR_LEN, "<conf>%S</conf>", msg->Elements[3].ElementValue);

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Server manager handling start server command for server %!ServerType! instance %hu.",
		svrType, svrInstance );

    PStrPrintf(szPoolDescription,ARRAYSIZE(szPoolDescription),L"ServerPool0x%04x",svrType);
    
#if USE_MEMORY_MANAGER
	CMemoryAllocatorCRT<true>* crtAllocator = new CMemoryAllocatorCRT<true>(szPoolDescription);
	ASSERT_GOTO( crtAllocator, _START_SVR_ERROR );
	crtAllocator->Init(NULL);
	IMemoryManager::GetInstance().AddAllocator(crtAllocator);
	
	CMemoryAllocatorPOOL<true, true, 1024>* newPool = new CMemoryAllocatorPOOL<true, true, 1024>(szPoolDescription);
	ASSERT_GOTO( newPool, _START_SVR_ERROR);
	newPool->Init(crtAllocator, 32 * KILO, false, false);
	IMemoryManager::GetInstance().AddAllocator(newPool);
#else
	MEMORYPOOL * newPool = MemoryPoolInit(szPoolDescription, DefaultMemoryAllocator );	
#endif    
	
	ASSERT_GOTO( newPool, _START_SVR_ERROR );

	//	get and load the config
	SVRCONFIG * config = NULL;
	if( G_SERVERS[svrType]->svrConfigCreator )
	{
		ASSERT_GOTO( PStrLen(svrConfigData) > 0, _START_SVR_ERROR );
		config = G_SERVERS[svrType]->svrConfigCreator( newPool );
		CMarkup markup;
		ASSERT_GOTO(markup.SetDoc(svrConfigData), _START_SVR_ERROR);
		ASSERT_GOTO(markup.FindElem("conf"), _START_SVR_ERROR);
		ASSERT_GOTO(markup.IntoElem(), _START_SVR_ERROR);
		ASSERT_GOTO(config->LoadFromMarkup(&markup), _START_SVR_ERROR);
	}

	//	try to start the requested server
#if USE_MEMORY_MANAGER
	ASSERT_GOTO(
		ActiveServerStartup(
			G_SERVERS[svrType],
			crtAllocator,
			newPool,
			svrInstance,
			svrMIndex,
			config ),
		_START_SVR_ERROR );
#else		
	ASSERT_GOTO(
		ActiveServerStartup(
			G_SERVERS[svrType],
			newPool,
			svrInstance,
			svrMIndex,
			config ),
		_START_SVR_ERROR );
#endif

	return;

_START_SVR_ERROR:
	SVRMGR_CMD_StopSvr(
		context,
		sender,
		msg,
		NULL );
}

//----------------------------------------------------------------------------
//	<stopserver>
//		<type/>
//		<instance/>
//		<machineindex/>
//	</stopserver>
//----------------------------------------------------------------------------
void SVRMGR_CMD_StopSvr(
	LPVOID,
	XMLMESSAGE *,
	XMLMESSAGE * msg,
	LPVOID /*userData*/ )
{
	SVRTYPE svrType = (SVRTYPE)ServerGetType(msg->Elements[0].ElementValue);
	if(svrType == INVALID_SVRTYPE)
	{
		TraceError(
			"Server manager recieved stop server command with invalid server type. Type - \"%S\"",
			msg->Elements[0].ElementValue );
		return;
	}

	SVRINSTANCE svrInstance;
	int inst = 0;
	if(!PStrToInt(msg->Elements[1].ElementValue, inst))
	{
		TraceError(
			"Server manager recieved stop server command with invalid server instance. Instance - \"%S\"",
			msg->Elements[1].ElementValue );
		return;
	}
	svrInstance = (SVRINSTANCE)inst;

	SVRMACHINEINDEX svrMIndex;
	int mi = 0;
	if(!PStrToInt(msg->Elements[2].ElementValue, mi) ||
		mi >= MAX_SVR_INSTANCES_PER_MACHINE )
	{
		TraceError(
			"Server manager recieved stop server command with invalid server machine index. Machine Index - \"%S\"",
			msg->Elements[2].ElementValue );
		return;
	}
	svrMIndex = (SVRMACHINEINDEX)mi;

	ACTIVE_SERVER_ENTRY & entry = g_runner->m_servers[svrType][svrMIndex];

	TraceInfo(
		TRACE_FLAG_SRUNNER,
		"Server manager handling stop server command for server %!ServerType! instance %hu.",
		svrType, svrInstance );

	entry.ServerLock.Lock();

	if( entry.Server )
	{
		//	cleanup the server
		entry.ServerLock.WaitZeroRef();
		ActiveServerShutdown( &entry );

		SvrManagerSendServerStatusUpdate(
			svrType,
			svrInstance,
			L"stopped");
	}
	else
	{
		SvrManagerSendServerOpResult(
			svrType,
			svrInstance,
			L"stopserver",
			L"ERROR",
			L"Server not running.");
	}

	entry.Server = NULL;

	entry.ServerLock.Unlock();
}
