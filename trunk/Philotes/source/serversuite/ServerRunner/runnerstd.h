//----------------------------------------------------------------------------
// runnerstd.h
//
// Standard header includes for all ServerRunner components.
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once
#ifndef	_RUNNERSTD_H_
#define _RUNNERSTD_H_


//----------------------------------------------------------------------------
// INCLUDES FROM COMMON
//----------------------------------------------------------------------------
#include "stdCommonIncludes.h"
#include <fileio.h>
#include <definition_common.h>
#include <definition_priv.h>
#include <appcommon.h>
#include <database.h>
#include <stlallocator.h>

#include <MuxClient.h>
#include <MuxServer.h>


//----------------------------------------------------------------------------
// STD INCLUDES
//----------------------------------------------------------------------------
#include "svrstd.h"			//	components from common and server api headers
#include "UserServerPrivateAPI.h"


//----------------------------------------------------------------------------
// SYSTEM EXTERNS
//----------------------------------------------------------------------------
extern struct SERVER_SPECIFICATION *	G_SERVERS[];
extern struct CONNECTION_TABLE			G_SERVER_CONNECTIONS[];


//----------------------------------------------------------------------------
// SERVER RUNNER INCLUDES
//----------------------------------------------------------------------------
#include "runnerdebug.h"
#include "ConnectionTable.h"

#include "ServerMailbox_priv.h"
#include "ServerRunnerNetwork.h"
#include "NetworkMap.h"

#include "runner.h"
#include "runner_priv.h"
#include "ActiveServer.h"

#include "servercontext.h"
#include "ServerManager.h"
#include "svrdebugtrace.h"
#include "playertrace.h"
#include <ehm.h>
#include <eventlog.h>

#define SERVICE_NAME  "WatchDogClient"
extern BOOL StartAsService(int argc, char **argv);
extern LONG RunnerTakeDump(EXCEPTION_POINTERS*);

#pragma warning(disable:4505)

#endif	//	_RUNNERSTD_H_
