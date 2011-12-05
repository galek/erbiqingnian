//----------------------------------------------------------------------------
// runner.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _RUNNER_H_
#define _RUNNER_H_


//----------------------------------------------------------------------------
// RUNNER VERSION
//----------------------------------------------------------------------------
extern const SVRVERSION				RunnerVersion;	// server runner version that changes w/ every re-compile
#define		 RUNNER_START_YEAR		2006			// base start year for timestamp


//----------------------------------------------------------------------------
// SERVER RUNNER CONFIG STRUCTS
//----------------------------------------------------------------------------
START_SVRCONFIG_DEF(ServerRunnerConfig)
	SVRCONFIG_INT_FIELD(0, WppTraceLevel, false, 4)
	SVRCONFIG_BIT_FIELD(1, WppTraceFlags, WPP_TRACE_FLAGS, false, 0x00FFFFFF)
	SVRCONFIG_BOL_FIELD(2, AssertSilently, false, TRUE)
	SVRCONFIG_STR_FIELD(3, CommandPipeName, true, "")
	SVRCONFIG_STR_FIELD(4, XmlMessageDefinitions, true, "")
	SVRCONFIG_BOL_FIELD(5, EnablePlayerTrace, false, FALSE)
	END_SVRCONFIG_DEF

START_SVRCONFIG_DEF(ServerRunnerDatabase)
	SVRCONFIG_STR_FIELD(0, DatabaseAddress, true, "")
	SVRCONFIG_STR_FIELD(1, DatabaseServer, false, "")
	SVRCONFIG_STR_FIELD(2, DatabaseUserId, true, "")
	SVRCONFIG_STR_FIELD(3, DatabaseUserPassword, true, "")
	SVRCONFIG_STR_FIELD(4, DatabaseName, true, "")
	SVRCONFIG_STR_FIELD(5, IsTrustedDatabase, false, "")
	SVRCONFIG_STR_FIELD(6, DatabaseNetwork, false, "")
	SVRCONFIG_INT_FIELD(7, DatabaseConnectionCount, false, 0)
	SVRCONFIG_INT_FIELD(8, RequestsPerConnectionBeforeReconnect, false, 100000)
	END_SVRCONFIG_DEF

DERIVED_SVRCONFIG_DEF(ServerRunnerLogDatabase, ServerRunnerDatabase)

START_SVRCONFIG_DEF(ServerBootstrapConfig)
	SVRCONFIG_BOL_FIELD(0, ShouldBootstrapFromConfigFile, true, TRUE)
	SVRCONFIG_STR_FIELD(1, BootstrapConfigFilePath, true, "")
	SVRCONFIG_STR_FIELD(2, BootstrapLocalIpAddress, true, "")
	END_SVRCONFIG_DEF


//----------------------------------------------------------------------------
// RUNNER PROGRAM CONTROL
//----------------------------------------------------------------------------

BOOL RunnerIsRunning(
		const char *	appMutexName );

BOOL RunnerInitSystems(
		BOOL runningAsService,
		const char * appName );

BOOL RunnerSetWorkingDirectory(
		BOOL bRunningAsService );

BOOL RunnerInitDefaultLogging(
		const char *appName);

BOOL RunnerInit(
		int							argc,
		char*						argv[],
		const char *				appName,
		class ServerRunnerConfig *	appConfig,
		ServerRunnerDatabase * gameDatabaseConfig,
		ServerRunnerLogDatabase * logDatabaseConfig );

void RunnerRun(
		void );

//	enqueues a message to shutdown the specified server
void RunnerStopServer(
		SVRTYPE type,
		SVRINSTANCE instance,
		SVRMACHINEINDEX machineIndex );

//	enqueues a message to shutdown the service
void RunnerStop(
		const char * reason );

void RunnerClose(
		void );

void RunnerCloseLogging(
		void );


#endif	//	_RUNNER_H_
