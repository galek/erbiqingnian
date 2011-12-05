//----------------------------------------------------------------------------
// ServerManager.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _SERVERMANAGER_H_
#define _SERVERMANAGER_H_


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define SVR_MGR_CMD_PIPE_READ_BUFF_SIZE  (KILO * 6)
#define SVR_MGR_CMD_PIPE_WRITE_BUFF_SIZE (KILO * 6)
#define CMD_PIPE_TEXT_BUFFER_LENGTH ((SVR_MGR_CMD_PIPE_READ_BUFF_SIZE/sizeof(WCHAR)) + 1)
#define MAX_XML_MESSAGE_COMMANDS		  32


//----------------------------------------------------------------------------
// SERVER MANAGER RESULT VALUES
//----------------------------------------------------------------------------
enum SVRMGR_RESULT
{
	SVRMGR_RESULT_NONE,
	SVRMGR_RESULT_ERROR,
	SVRMGR_RESULT_SUCCESS,
	SVRMGR_RESULT_FAILURE,
	SVRMGR_RESULT_PENDING,

	//	KEEP AS LAST ENTRY
	SVRMGR_RESULT_COUNT
};


//----------------------------------------------------------------------------
// SERVER MANAGER CALLBACK
// DESC: optional callback called upon command completion
//----------------------------------------------------------------------------
typedef void (*FP_SVRMGR_COMMAND_CALLBACK)(
	SVRMGR_RESULT,					// the result of the command execution
	LPVOID );						// command context


//----------------------------------------------------------------------------
// SERVER MANAGEMENT METHODS
//----------------------------------------------------------------------------

BOOL SvrManagerInit(
	struct MEMORYPOOL *,
	const char * cmdPipeName,
	const char * xmlMessageDefineFilepaths );

void SvrManagerFree(
	void );

void SvrManagerMainMsgLoop(
	void );

BOOL SvrManagerSendXMLMessagingMessage(
	WCHAR * messageText );

BOOL SvrManagerSendServerStatusUpdate(
	SVRTYPE type,
	SVRINSTANCE inst,
	const WCHAR * status);

BOOL SvrManagerSendServerOpResult(
	SVRTYPE type,
	SVRINSTANCE inst,
	const WCHAR * opType,
	const WCHAR * opResult,
	const WCHAR * opResExplanation);

//----------------------------------------------------------------------------
// COMMAND PROCESSING
// DESC: optionally blocking message method, places msg in queue for main msg loop.
//----------------------------------------------------------------------------
SVRMGR_RESULT SvrManagerEnqueueMessage(
	const WCHAR *			 wszCommandText,
	BOOL						bSyncronous			= FALSE,
	FP_SVRMGR_COMMAND_CALLBACK fpCompletionCallback	= NULL,
	LPVOID						pCallbackData		= NULL );

#endif	//	_SERVERMANAGER_H_
