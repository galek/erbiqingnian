//----------------------------------------------------------------------------
// XmlMessageHandler.cpp
// (C)Copyright 2007, Ping0 LLC. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#include "XmlMessageHandler.h"
#include "XmlMessageHandler.cpp.tmh"


//----------------------------------------------------------------------------
// XML MESSAGE HANDLING
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static*/ BOOL XmlMessageHandler::LoadXMLMESSAGE(
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
/*static*/ BOOL XmlMessageHandler::ParseXmlDefFile(
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

		_XMLCMD_RECEIVER * newReceiver = m_xmlReceiverDefHash.Add(
												m_pool,
												recipKey);
		ASSERT_RETFALSE(newReceiver);
		newReceiver->commands.Init();

		while(pMarkup->FindChildElem())				//		<command>
		{
			const WCHAR * cmdName = pMarkup->GetChildTagName();
			ASSERT_RETFALSE(cmdName && cmdName[0]);
			XMLCMD_KEY cmdKey = cmdName;

			_XMLCMD_DEF * newCommand = newReceiver->commands.Add(
												m_pool,
												cmdKey);
			ASSERT_RETFALSE(newCommand);

			ASSERT_RETFALSE(pMarkup->IntoElem());	//	->  <command>
			ASSERT_RETFALSE(LoadXMLMESSAGE(pMarkup, &newCommand->command));
			ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <command>
		}

		ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <recipient>
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static*/ BOOL XmlMessageHandler::LoadXmlDefFile(
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
			TRACE_FLAG_SRUNNER, // TODO use a different trace flag
			"XML Message Definition file missing. Filename - \"%s\"",
			filename);
		return TRUE;	//	until message config files are situated, ignore missing def files...
	}
	//	ASSERT_RETFALSE(hFile != INVALID_HANDLE_VALUE);

	char * sbBuffer = NULL;
	WCHAR * wcBuffer = NULL;

	DWORD fileSize = GetFileSize(hFile, NULL);
	ASSERT_GOTO(fileSize != INVALID_FILE_SIZE && fileSize > 0, _ERROR);

	sbBuffer = (char*)MALLOC(m_pool,fileSize);
	ASSERT_GOTO(sbBuffer, _ERROR);

	wcBuffer = (WCHAR*)MALLOC(m_pool,fileSize * sizeof(WCHAR));
	ASSERT_GOTO(wcBuffer, _ERROR);

	DWORD numRead = 0;
	ASSERT_GOTO(ReadFile(hFile,sbBuffer, fileSize, &numRead, NULL), _ERROR);
	ASSERT_GOTO(numRead == fileSize, _ERROR);

	PStrCvt(wcBuffer, sbBuffer, fileSize);

	pMarkup = MALLOC_NEW(m_pool, CUnicodeMarkup);
	ASSERT_GOTO(pMarkup, _ERROR);

	ASSERT_GOTO(pMarkup->SetDoc(wcBuffer), _ERROR);
	ASSERT_GOTO(m_xmlMarkupsLoaded.PListPushHeadPool(m_pool, pMarkup), _ERROR);
	ASSERT_GOTO(ParseXmlDefFile(pMarkup), _ERROR);

	FREE(m_pool, sbBuffer);
	CloseHandle(hFile);
	return TRUE;

_ERROR:
	if(sbBuffer)
		FREE(m_pool, sbBuffer);
	if(wcBuffer)
		FREE(m_pool, wcBuffer);
	if(pMarkup)
		FREE_DELETE(m_pool,pMarkup,CUnicodeMarkup);
	CloseHandle(hFile);
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static*/ BOOL XmlMessageHandler::InitXmlHandling(
	MEMORYPOOL * pool,
	const char * filepaths )
{
	ASSERT_RETFALSE(pool);
	m_pool = pool;
	m_xmlReceiverDefHash.Init();
	m_xmlMarkupsLoaded.Init();

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
		ASSERT_RETFALSE(LoadXmlDefFile(start));
		start = end;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void XmlMessageHandler::Free(void)
{
	m_xmlReceiverDefHash.Destroy(m_pool, FP_RECEIVER_DEF_DELETE_ITR);
	m_xmlMarkupsLoaded.Destroy(m_pool, FP_LOADED_MARKUP_DELETE_ITR);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static*/ void XmlMessageHandler::FP_RECEIVER_DEF_DELETE_ITR(
	MEMORYPOOL * pool,
	_XMLCMD_RECEIVER * def)
{
	ASSERT_RETURN(def);
	def->commands.Destroy(pool);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static*/ void XmlMessageHandler::FP_LOADED_MARKUP_DELETE_ITR(
	MEMORYPOOL * pool,
	CUnicodeMarkup *& markup)
{
	ASSERT_RETURN(markup);
	markup->Free();
	FREE_DELETE(pool, markup, CUnicodeMarkup);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*static*/ BOOL XmlMessageHandler::ValidateCommand(
	XMLMESSAGE * recipient,
	XMLMESSAGE * command)
{
	ASSERT_RETFALSE(recipient && recipient->MessageName && recipient->MessageName[0]);
	ASSERT_RETFALSE(command && command->MessageName && command->MessageName[0]);

	XMLCMD_KEY recipKey = recipient->MessageName;
	XMLCMD_KEY cmdKey = command->MessageName;

	_XMLCMD_RECEIVER * recipientTable = m_xmlReceiverDefHash.Get(recipKey);
	if(!recipientTable)
	{
		TraceError(
			"XML Handler received command for unknown recipient type. Recipient - \"%S\"",
			recipient->MessageName);
		return FALSE;
	}

	_XMLCMD_DEF * cmdDef = recipientTable->commands.Get(cmdKey);
	if(!cmdDef)
	{
		TraceError(
			"XML Handler received unknown command. Recipient - \"%S\", Command - \"%S\"",
			recipient->MessageName,
			command->MessageName);
		return FALSE;
	}

	XMLMESSAGE * def = &cmdDef->command;
	if(command->ElementCount != def->ElementCount)
	{
		TraceError(
			"XML Handler received a command that does not match the definition loaded from file. Recipient - \"%S\", Command - \"%S\", Command Element Count - %u",
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
				"XML Handler received a command that does not match the definition loaded from file. Recipient - \"%S\", Command - \"%S\", Element Name - \"%S\"",
				recipient->MessageName,
				command->MessageName,
				command->Elements[ii].ElementName);
			return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*static*/ BOOL XmlMessageHandler::ParseAndValidateXmlMessages(
	const WCHAR * wszCommandText,
	CUnicodeMarkup * pMarkup,
	XMLMESSAGE * pSender,
	XMLMESSAGE * pReceiver,
	XMLMESSAGE * pCommands,
	DWORD & nCommands )
{
	ASSERT_RETFALSE(wszCommandText && wszCommandText[0]);
	ASSERT_RETFALSE(pMarkup);
	ASSERT_RETFALSE(pSender);
	ASSERT_RETFALSE(pReceiver);
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
				ASSERT_RETFALSE(LoadXMLMESSAGE(pMarkup, pSender));
				ASSERT_RETFALSE(pMarkup->OutOfElem());
			ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <source>

		ASSERT_RETFALSE(pMarkup->FindChildElem());	//	<dest>
		ASSERT_RETFALSE(PStrCmp(L"dest", pMarkup->GetChildTagName()) == 0);
		ASSERT_RETFALSE(pMarkup->IntoElem());		//	->  <dest>

			ASSERT_RETFALSE(pMarkup->FindChildElem());
			ASSERT_RETFALSE(pMarkup->IntoElem());
				ASSERT_RETFALSE(LoadXMLMESSAGE(pMarkup, pReceiver));
				ASSERT_RETFALSE(pMarkup->OutOfElem());
			ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <dest>

		ASSERT_RETFALSE(pMarkup->OutOfElem());	//	<-  <hdr>

	ASSERT_RETFALSE(pMarkup->FindChildElem());	//		<body>
	ASSERT_RETFALSE(pMarkup->IntoElem());		//	->  <body>

		nCommands = 0;
		while(pMarkup->FindChildElem() && nCommands < MAX_XML_MESSAGE_COMMANDS)
		{
			ASSERT_RETFALSE(pMarkup->IntoElem());
				ASSERT_RETFALSE(LoadXMLMESSAGE(pMarkup, &pCommands[nCommands]));
				ASSERT_RETFALSE(pMarkup->OutOfElem());
			if(!ValidateCommand(pReceiver, &pCommands[nCommands]))
				return FALSE;
			++nCommands;
		}
		ASSERT_RETFALSE(nCommands > 0);

	return TRUE;
}

// TODO implement