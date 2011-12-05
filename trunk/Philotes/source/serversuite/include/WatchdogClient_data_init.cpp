// WARNING !! only perfhelper.cpp may include this file.
/* o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o
 *     Generated Code below. Don't worry your pretty little head about it.                     *
 o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o*/
#define PERF_DATA_HEADER(ObjectType)\
	{\
		sizeof(ObjectType##_data_header),\
		sizeof(ObjectType##_data_header),\
		sizeof(PERF_OBJECT_TYPE),\
		ObjectType##_OFFSET,\
		NULL,\
		ObjectType##_OFFSET,\
		NULL,\
		PERF_DETAIL_NOVICE,\
		(sizeof(ObjectType##_data_header)- sizeof(PERF_OBJECT_TYPE))/sizeof(PERF_COUNTER_DEFINITION),\
		0,PERF_NO_INSTANCES,0\
	}

#define PERF_COUNTER_DEFINITION_INIT(Obj,ctrName,ctrType,offset)\
	{\
		sizeof(PERF_COUNTER_DEFINITION),\
		offset,\
		NULL,\
		offset,\
		NULL,\
		0,\
		PERF_DETAIL_NOVICE,\
		ctrType,\
		sizeof(sdummy_##Obj.ctrName),\
		FIELD_OFFSET(PERFCOUNTER_INSTANCE_##Obj,ctrName) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_##Obj,CounterBlock)\
	}

#define INIT_PERF_DATA_HEADER_UserServer()\
	{\
		PERF_DATA_HEADER(UserServer),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,numClients,PERF_COUNTER_LARGE_RAWCOUNT,numClients_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,numServers,PERF_COUNTER_LARGE_RAWCOUNT,numServers_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,byteRateOut,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,byteRateOut_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,byteRateIn,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,byteRateIn_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,invalidCommandCount,PERF_COUNTER_LARGE_RAWCOUNT,invalidCommandCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,incomingMessageRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,incomingMessageRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,outGoingMessageRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,outGoingMessageRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,numSessions,PERF_COUNTER_LARGE_RAWCOUNT,numSessions_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(UserServer,messageRateExceeded,PERF_COUNTER_LARGE_RAWCOUNT,messageRateExceeded_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_GameServerThread()\
	{\
		PERF_DATA_HEADER(GameServerThread),\
		PERF_COUNTER_DEFINITION_INIT(GameServerThread,GameServerThreadGameTicks,PERF_COUNTER_LARGE_RAWCOUNT,GameServerThreadGameTicks_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServerThread,GameServerThreadMaxSimultaneousTicks,PERF_COUNTER_LARGE_RAWCOUNT,GameServerThreadMaxSimultaneousTicks_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServerThread,GameServerThreadGameTickRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerThreadGameTickRate_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_GameServer()\
	{\
		PERF_DATA_HEADER(GameServer),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerClientCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerClientCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerAlltimeClientCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerAlltimeClientCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerClientDetachCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerClientDetachCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerSendErrorCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerSendErrorCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerPingBootCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerPingBootCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerIdValidateBootCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerIdValidateBootCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerMessageRateBootCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerMessageRateBootCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerCCMD_EXIT_GAMECount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerCCMD_EXIT_GAMECount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerMaximumClientCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerMaximumClientCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerGameCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerGameCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerLevelCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerLevelCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerUnitCount,PERF_COUNTER_LARGE_RAWCOUNT,GameServerUnitCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerSimulationFrames,PERF_COUNTER_LARGE_RAWCOUNT,GameServerSimulationFrames_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerSimulationFrameRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerSimulationFrameRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerGameTickRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerGameTickRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerDRLGRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerDRLGRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerUnitStepRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerUnitStepRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerUnitStepMonsterRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerUnitStepMonsterRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerUnitStepMissileRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerUnitStepMissileRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerUnitStepItemRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerUnitStepItemRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerUnitStepPlayerRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerUnitStepPlayerRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerGameEventRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerGameEventRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerGameEventHandlers,PERF_COUNTER_LARGE_RAWCOUNT,GameServerGameEventHandlers_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerAStarPathRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerAStarPathRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerLinearPathRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerLinearPathRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerPathFailureRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerPathFailureRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerMessagesReceivedPerSecond,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerMessagesReceivedPerSecond_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,GameServerMessagesSentPerSecond,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GameServerMessagesSentPerSecond_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,RoomGetNearestPathNodesInArray,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,RoomGetNearestPathNodesInArray_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,RoomGetNearestFreePathNode,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,RoomGetNearestFreePathNode_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,RoomGetNearestPathNodes,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,RoomGetNearestPathNodes_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,Ping,PERF_COUNTER_LARGE_RAWCOUNT,Ping_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameServer,ExperienceGainCount,PERF_COUNTER_LARGE_RAWCOUNT,ExperienceGainCount_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_CHAT_SERVER()\
	{\
		PERF_DATA_HEADER(CHAT_SERVER),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_SYS_MEMBER_CONNECTIONS,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_SYS_MEMBER_CONNECTIONS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_SYS_MEMBER_CONNECTION_RATE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_SYS_MEMBER_CONNECTION_RATE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_SYS_MEMBER_COMMAND_RATE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_SYS_MEMBER_COMMAND_RATE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_SYS_GAMESVR_CONNECTIONS,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_SYS_GAMESVR_CONNECTIONS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_SYS_GAMESVR_COMMAND_RATE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_SYS_GAMESVR_COMMAND_RATE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_SYS_MULTIPOINT_SENDS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_SYS_MULTIPOINT_SENDS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_SYS_CSR_COMMAND_RATE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_SYS_CSR_COMMAND_RATE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MBR_MEMBER_OBJECT_COUNT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_MBR_MEMBER_OBJECT_COUNT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MBR_DIRECT_MEMBER_MSG_RATE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_MBR_DIRECT_MEMBER_MSG_RATE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_CNL_CHANNEL_OBJECT_COUNT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_CNL_CHANNEL_OBJECT_COUNT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_CNL_CHANNEL_MEMBER_MSG_RATE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_CNL_CHANNEL_MEMBER_MSG_RATE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_CNL_CHANNEL_MEMBER_MSG_RECIPIENTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_CNL_CHANNEL_MEMBER_MSG_RECIPIENTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_CNL_CHANNEL_CHAT_MSG_RATE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_CNL_CHANNEL_CHAT_MSG_RATE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PTY_CHAT_PARTY_OBJECT_COUNT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_PTY_CHAT_PARTY_OBJECT_COUNT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PTY_GAMESVR_MSG_RATE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_PTY_GAMESVR_MSG_RATE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PTY_GAMESVR_MSG_RECIPIENTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_PTY_GAMESVR_MSG_RECIPIENTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PTY_PARTY_CHAT_CHANNEL_PERCENTILENumerator,PERF_LARGE_RAW_FRACTION,CHATPERF_PTY_PARTY_CHAT_CHANNEL_PERCENTILENumerator_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PTY_PARTY_CHAT_CHANNEL_PERCENTILEDenominator,PERF_LARGE_RAW_BASE,0),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_IM_CHAT_CHANNEL_COUNT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_IM_CHAT_CHANNEL_COUNT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_IM_CHAT_CHANNEL_PERCENTILENumerator,PERF_LARGE_RAW_FRACTION,CHATPERF_IM_CHAT_CHANNEL_PERCENTILENumerator_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_IM_CHAT_CHANNEL_PERCENTILEDenominator,PERF_LARGE_RAW_BASE,0),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PAD_ADVERTISED_PARTIES_COUNT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_PAD_ADVERTISED_PARTIES_COUNT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PAD_PARTY_CLASSIFICATIONS_COUNT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_PAD_PARTY_CLASSIFICATIONS_COUNT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PAD_PARTY_INFO_UPDATES,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_PAD_PARTY_INFO_UPDATES_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PAD_PARTY_AD_GET_REQUESTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_PAD_PARTY_AD_GET_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_PAD_PARTY_ADS_SENT,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_PAD_PARTY_ADS_SENT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_GLD_GUILD_COUNT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_GLD_GUILD_COUNT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_HAK_FAILED_LOGINS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_HAK_FAILED_LOGINS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_HAK_REJECTED_TIME_PROTECTED_OPS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_HAK_REJECTED_TIME_PROTECTED_OPS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_HAK_NOT_LOGGED_IN_COMMANDS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_HAK_NOT_LOGGED_IN_COMMANDS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_HAK_NOT_A_MEMBER_REQUESTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_HAK_NOT_A_MEMBER_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_HAK_UNSUPPORTED_REQUESTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_HAK_UNSUPPORTED_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MSGRATES_SEND_CHAT,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_MSGRATES_SEND_CHAT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MSGRATES_IGNORE_OPERATION,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_MSGRATES_IGNORE_OPERATION_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MSGRATES_BUDDY_LIST_OPERATION,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_MSGRATES_BUDDY_LIST_OPERATION_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MSGRATES_PARTY_OPERATION,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_MSGRATES_PARTY_OPERATION_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MSGRATES_GUILD_OPERATION,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_MSGRATES_GUILD_OPERATION_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MSGRATES_MISC_GETSET_INFO,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_MSGRATES_MISC_GETSET_INFO_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_MSGRATES_MISC_CHANNEL_ADDLEAVE,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,CHATPERF_MSGRATES_MISC_CHANNEL_ADDLEAVE_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_EMAIL_SEND_REQUESTS,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_EMAIL_SEND_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_EMAIL_COMPOSING_MESSAGES,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_EMAIL_COMPOSING_MESSAGES_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_EMAIL_ABORTED_COMPOSING_MESSAGES,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_EMAIL_ABORTED_COMPOSING_MESSAGES_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_EMAIL_RECEIVE_REQUESTS,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_EMAIL_RECEIVE_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_EMAIL_RECEIVED_EMAILS,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_EMAIL_RECEIVED_EMAILS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_HYPERTEXT_TOTAL_MESSAGE_COUNT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_HYPERTEXT_TOTAL_MESSAGE_COUNT_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CHAT_SERVER,CHATPERF_HYPERTEXT_TOTAL_DATA_BYTES_SENT,PERF_COUNTER_LARGE_RAWCOUNT,CHATPERF_HYPERTEXT_TOTAL_DATA_BYTES_SENT_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_DATABASE_POOL()\
	{\
		PERF_DATA_HEADER(DATABASE_POOL),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_POOLED_CONNECTIONS,PERF_COUNTER_LARGE_RAWCOUNT,DBPERF_POOLED_CONNECTIONS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_OUTSTANDING_QUERIES,PERF_COUNTER_LARGE_RAWCOUNT,DBPERF_OUTSTANDING_QUERIES_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_PROCESSED_QUERIES,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,DBPERF_PROCESSED_QUERIES_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_SINGLE_DATA_REQUESTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,DBPERF_SINGLE_DATA_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_PARTIAL_BATCH_DATA_REQUESTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,DBPERF_PARTIAL_BATCH_DATA_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_FULL_BATCH_DATA_REQUESTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,DBPERF_FULL_BATCH_DATA_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_QUERY_PROC_TIME,PERF_COUNTER_LARGE_DELTA,DBPERF_QUERY_PROC_TIME_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_RETRIEVE_RESULTS_TIME,PERF_COUNTER_LARGE_DELTA,DBPERF_RETRIEVE_RESULTS_TIME_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_FAILED_QUERIES,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,DBPERF_FAILED_QUERIES_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_WAITING_BATCH_TIMER_CALLBACKS,PERF_COUNTER_LARGE_RAWCOUNT,DBPERF_WAITING_BATCH_TIMER_CALLBACKS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_PROCESSED_BATCH_TIMER_CALLBACKS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,DBPERF_PROCESSED_BATCH_TIMER_CALLBACKS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_CANCELED_REQUESTS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,DBPERF_CANCELED_REQUESTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_ATTEMPTED_REQUEST_CANCELATIONS,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,DBPERF_ATTEMPTED_REQUEST_CANCELATIONS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_TOTAL_REQUESTS_PROCESSED_ON_ACTIVE_CONNECTIONS,PERF_COUNTER_LARGE_RAWCOUNT,DBPERF_TOTAL_REQUESTS_PROCESSED_ON_ACTIVE_CONNECTIONS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_CONNECTION_REESTABLISHMENTS,PERF_COUNTER_LARGE_RAWCOUNT,DBPERF_CONNECTION_REESTABLISHMENTS_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(DATABASE_POOL,DBPERF_LIFETIME_REQUESTS,PERF_COUNTER_LARGE_RAWCOUNT,DBPERF_LIFETIME_REQUESTS_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_CMemoryAllocator()\
	{\
		PERF_DATA_HEADER(CMemoryAllocator),\
		PERF_COUNTER_DEFINITION_INIT(CMemoryAllocator,TotalExternalAllocationSize,PERF_COUNTER_LARGE_RAWCOUNT,TotalExternalAllocationSize_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CMemoryAllocator,ExternalAllocationRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,ExternalAllocationRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CMemoryAllocator,ExternalFreeRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,ExternalFreeRate_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_LoginServer()\
	{\
		PERF_DATA_HEADER(LoginServer),\
		PERF_COUNTER_DEFINITION_INIT(LoginServer,numLoginClients,PERF_COUNTER_LARGE_RAWCOUNT,numLoginClients_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(LoginServer,loginRequestRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,loginRequestRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(LoginServer,loginRequestsPending,PERF_COUNTER_LARGE_RAWCOUNT,loginRequestsPending_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(LoginServer,loginRequestsSuccessful,PERF_COUNTER_LARGE_RAWCOUNT,loginRequestsSuccessful_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(LoginServer,loginRequestsFailed,PERF_COUNTER_LARGE_RAWCOUNT,loginRequestsFailed_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(LoginServer,usedTicketCacheSize,PERF_COUNTER_LARGE_RAWCOUNT,usedTicketCacheSize_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_MuxServer()\
	{\
		PERF_DATA_HEADER(MuxServer),\
		PERF_COUNTER_DEFINITION_INIT(MuxServer,muxServerBytesSent,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,muxServerBytesSent_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(MuxServer,muxServerBytesRecvd,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,muxServerBytesRecvd_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(MuxServer,muxServerActualClientCount,PERF_COUNTER_LARGE_RAWCOUNT,muxServerActualClientCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(MuxServer,muxServerVirtualClientCount,PERF_COUNTER_LARGE_RAWCOUNT,muxServerVirtualClientCount_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_MuxClientImpl()\
	{\
		PERF_DATA_HEADER(MuxClientImpl),\
		PERF_COUNTER_DEFINITION_INIT(MuxClientImpl,muxClientBytesSent,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,muxClientBytesSent_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(MuxClientImpl,muxClientBytesRecvd,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,muxClientBytesRecvd_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(MuxClientImpl,muxClientCount,PERF_COUNTER_LARGE_RAWCOUNT,muxClientCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(MuxClientImpl,muxClientDiscPending,PERF_COUNTER_LARGE_RAWCOUNT,muxClientDiscPending_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_HLOCK_LIB()\
	{\
		PERF_DATA_HEADER(HLOCK_LIB),\
		PERF_COUNTER_DEFINITION_INIT(HLOCK_LIB,LocksHeld,PERF_COUNTER_LARGE_RAWCOUNT,LocksHeld_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(HLOCK_LIB,LockContention,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,LockContention_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(HLOCK_LIB,LockOps,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,LockOps_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_PatchServer()\
	{\
		PERF_DATA_HEADER(PatchServer),\
		PERF_COUNTER_DEFINITION_INIT(PatchServer,numCurrentClients,PERF_COUNTER_LARGE_RAWCOUNT,numCurrentClients_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(PatchServer,numWaitingClients,PERF_COUNTER_LARGE_RAWCOUNT,numWaitingClients_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(PatchServer,numReadyClients,PERF_COUNTER_LARGE_RAWCOUNT,numReadyClients_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(PatchServer,numStalledClients,PERF_COUNTER_LARGE_RAWCOUNT,numStalledClients_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(PatchServer,UnackedBytes,PERF_COUNTER_LARGE_RAWCOUNT,UnackedBytes_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(PatchServer,numFilesPatched,PERF_COUNTER_LARGE_RAWCOUNT,numFilesPatched_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(PatchServer,numSendingBytes,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,numSendingBytes_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(PatchServer,numSendingMsgs,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,numSendingMsgs_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_GameLoadBalanceServer()\
	{\
		PERF_DATA_HEADER(GameLoadBalanceServer),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,numGameServerClients,PERF_COUNTER_LARGE_RAWCOUNT,numGameServerClients_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,numGameLoginClients,PERF_COUNTER_LARGE_RAWCOUNT,numGameLoginClients_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameLoginRequestsPending,PERF_COUNTER_LARGE_RAWCOUNT,gameLoginRequestsPending_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameLoginRequestsSuccessful,PERF_COUNTER_LARGE_RAWCOUNT,gameLoginRequestsSuccessful_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameLoginRequestsFailed,PERF_COUNTER_LARGE_RAWCOUNT,gameLoginRequestsFailed_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameSwitchRequests,PERF_COUNTER_LARGE_RAWCOUNT,gameSwitchRequests_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameServerLogins,PERF_COUNTER_LARGE_RAWCOUNT,gameServerLogins_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameServerLogouts,PERF_COUNTER_LARGE_RAWCOUNT,gameServerLogouts_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameServerDBFinishes,PERF_COUNTER_LARGE_RAWCOUNT,gameServerDBFinishes_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameTotalQueueLength,PERF_COUNTER_LARGE_RAWCOUNT,gameTotalQueueLength_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameSubscriptionPlayersInQueue,PERF_COUNTER_LARGE_RAWCOUNT,gameSubscriptionPlayersInQueue_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GameLoadBalanceServer,gameFreePlayPlayersInQueue,PERF_COUNTER_LARGE_RAWCOUNT,gameFreePlayPlayersInQueue_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_CommonNetWorker()\
	{\
		PERF_DATA_HEADER(CommonNetWorker),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetWorker,schedulingRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,schedulingRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetWorker,sendCompletionRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,sendCompletionRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetWorker,recvCompletionRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,recvCompletionRate_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_CommonNetServer()\
	{\
		PERF_DATA_HEADER(CommonNetServer),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetServer,numClientsConnected,PERF_COUNTER_LARGE_RAWCOUNT,numClientsConnected_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetServer,connectionRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,connectionRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetServer,numBytesOutstanding,PERF_COUNTER_LARGE_RAWCOUNT,numBytesOutstanding_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetServer,numSendsPending,PERF_COUNTER_LARGE_RAWCOUNT,numSendsPending_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetServer,connectionsThrottledBySendCount,PERF_COUNTER_LARGE_RAWCOUNT,connectionsThrottledBySendCount_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetServer,connectionsThrottledByByteCount,PERF_COUNTER_LARGE_RAWCOUNT,connectionsThrottledByByteCount_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_CommonNetBufferPool()\
	{\
		PERF_DATA_HEADER(CommonNetBufferPool),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetBufferPool,overlapAllocationRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,overlapAllocationRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CommonNetBufferPool,msgBufAllocationRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,msgBufAllocationRate_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_BillingProxy()\
	{\
		PERF_DATA_HEADER(BillingProxy),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,CurrentUsers,PERF_COUNTER_LARGE_RAWCOUNT,CurrentUsers_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,LifetimeClientsAttached,PERF_COUNTER_LARGE_RAWCOUNT,LifetimeClientsAttached_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,AccountsDBLinkUp,PERF_COUNTER_LARGE_RAWCOUNT,AccountsDBLinkUp_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,RequestRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,RequestRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,PendingResponses,PERF_COUNTER_LARGE_RAWCOUNT,PendingResponses_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,BillingProviderLinkFlag,PERF_COUNTER_LARGE_RAWCOUNT,BillingProviderLinkFlag_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,BillingProviderSendQueueSize,PERF_COUNTER_LARGE_RAWCOUNT,BillingProviderSendQueueSize_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,BillingProviderMessageSendRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,BillingProviderMessageSendRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,BillingProviderMessageRecvRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,BillingProviderMessageRecvRate_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,BillingProviderResyncFlag,PERF_COUNTER_LARGE_RAWCOUNT,BillingProviderResyncFlag_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,BillingProviderKicks,PERF_COUNTER_LARGE_RAWCOUNT,BillingProviderKicks_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BillingProxy,BillingProviderErrors,PERF_COUNTER_LARGE_RAWCOUNT,BillingProviderErrors_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_CSRBridgeServer()\
	{\
		PERF_DATA_HEADER(CSRBridgeServer),\
		PERF_COUNTER_DEFINITION_INIT(CSRBridgeServer,NumNamedPipesOpen,PERF_COUNTER_LARGE_RAWCOUNT,NumNamedPipesOpen_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(CSRBridgeServer,NumXmlMessagesRecvd,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,NumXmlMessagesRecvd_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_BattleServer()\
	{\
		PERF_DATA_HEADER(BattleServer),\
		PERF_COUNTER_DEFINITION_INIT(BattleServer,BattleServerSimulationFrames,PERF_COUNTER_LARGE_RAWCOUNT,BattleServerSimulationFrames_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(BattleServer,BattleServerSimulationFrameRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,BattleServerSimulationFrameRate_OFFSET)\
	}
#define INIT_PERF_DATA_HEADER_GlobalGameEventServer()\
	{\
		PERF_DATA_HEADER(GlobalGameEventServer),\
		PERF_COUNTER_DEFINITION_INIT(GlobalGameEventServer,GlobalGameEventServerSimulationFrames,PERF_COUNTER_LARGE_RAWCOUNT,GlobalGameEventServerSimulationFrames_OFFSET),\
		PERF_COUNTER_DEFINITION_INIT(GlobalGameEventServer,GlobalGameEventServerSimulationFrameRate,PERF_COUNTER_COUNTER| PERF_SIZE_LARGE,GlobalGameEventServerSimulationFrameRate_OFFSET)\
	}
static PERFCOUNTER_INSTANCE_UserServer sdummy_UserServer;
static UserServer_data_header hdr_UserServer=INIT_PERF_DATA_HEADER_UserServer();
static PERFCOUNTER_INSTANCE_GameServerThread sdummy_GameServerThread;
static GameServerThread_data_header hdr_GameServerThread=INIT_PERF_DATA_HEADER_GameServerThread();
static PERFCOUNTER_INSTANCE_GameServer sdummy_GameServer;
static GameServer_data_header hdr_GameServer=INIT_PERF_DATA_HEADER_GameServer();
static PERFCOUNTER_INSTANCE_CHAT_SERVER sdummy_CHAT_SERVER;
static CHAT_SERVER_data_header hdr_CHAT_SERVER=INIT_PERF_DATA_HEADER_CHAT_SERVER();
static PERFCOUNTER_INSTANCE_DATABASE_POOL sdummy_DATABASE_POOL;
static DATABASE_POOL_data_header hdr_DATABASE_POOL=INIT_PERF_DATA_HEADER_DATABASE_POOL();
static PERFCOUNTER_INSTANCE_CMemoryAllocator sdummy_CMemoryAllocator;
static CMemoryAllocator_data_header hdr_CMemoryAllocator=INIT_PERF_DATA_HEADER_CMemoryAllocator();
static PERFCOUNTER_INSTANCE_LoginServer sdummy_LoginServer;
static LoginServer_data_header hdr_LoginServer=INIT_PERF_DATA_HEADER_LoginServer();
static PERFCOUNTER_INSTANCE_MuxServer sdummy_MuxServer;
static MuxServer_data_header hdr_MuxServer=INIT_PERF_DATA_HEADER_MuxServer();
static PERFCOUNTER_INSTANCE_MuxClientImpl sdummy_MuxClientImpl;
static MuxClientImpl_data_header hdr_MuxClientImpl=INIT_PERF_DATA_HEADER_MuxClientImpl();
static PERFCOUNTER_INSTANCE_HLOCK_LIB sdummy_HLOCK_LIB;
static HLOCK_LIB_data_header hdr_HLOCK_LIB=INIT_PERF_DATA_HEADER_HLOCK_LIB();
static PERFCOUNTER_INSTANCE_PatchServer sdummy_PatchServer;
static PatchServer_data_header hdr_PatchServer=INIT_PERF_DATA_HEADER_PatchServer();
static PERFCOUNTER_INSTANCE_GameLoadBalanceServer sdummy_GameLoadBalanceServer;
static GameLoadBalanceServer_data_header hdr_GameLoadBalanceServer=INIT_PERF_DATA_HEADER_GameLoadBalanceServer();
static PERFCOUNTER_INSTANCE_CommonNetWorker sdummy_CommonNetWorker;
static CommonNetWorker_data_header hdr_CommonNetWorker=INIT_PERF_DATA_HEADER_CommonNetWorker();
static PERFCOUNTER_INSTANCE_CommonNetServer sdummy_CommonNetServer;
static CommonNetServer_data_header hdr_CommonNetServer=INIT_PERF_DATA_HEADER_CommonNetServer();
static PERFCOUNTER_INSTANCE_CommonNetBufferPool sdummy_CommonNetBufferPool;
static CommonNetBufferPool_data_header hdr_CommonNetBufferPool=INIT_PERF_DATA_HEADER_CommonNetBufferPool();
static PERFCOUNTER_INSTANCE_BillingProxy sdummy_BillingProxy;
static BillingProxy_data_header hdr_BillingProxy=INIT_PERF_DATA_HEADER_BillingProxy();
static PERFCOUNTER_INSTANCE_CSRBridgeServer sdummy_CSRBridgeServer;
static CSRBridgeServer_data_header hdr_CSRBridgeServer=INIT_PERF_DATA_HEADER_CSRBridgeServer();
static PERFCOUNTER_INSTANCE_BattleServer sdummy_BattleServer;
static BattleServer_data_header hdr_BattleServer=INIT_PERF_DATA_HEADER_BattleServer();
static PERFCOUNTER_INSTANCE_GlobalGameEventServer sdummy_GlobalGameEventServer;
static GlobalGameEventServer_data_header hdr_GlobalGameEventServer=INIT_PERF_DATA_HEADER_GlobalGameEventServer();
static PERF_OBJECT_TYPE* data_headers[PERFCOUNTER_OBJECT_TYPE_MAX +1 ] = {
	&hdr_UserServer.ObjectType,
	&hdr_GameServerThread.ObjectType,
	&hdr_GameServer.ObjectType,
	&hdr_CHAT_SERVER.ObjectType,
	&hdr_DATABASE_POOL.ObjectType,
	&hdr_CMemoryAllocator.ObjectType,
	&hdr_LoginServer.ObjectType,
	&hdr_MuxServer.ObjectType,
	&hdr_MuxClientImpl.ObjectType,
	&hdr_HLOCK_LIB.ObjectType,
	&hdr_PatchServer.ObjectType,
	&hdr_GameLoadBalanceServer.ObjectType,
	&hdr_CommonNetWorker.ObjectType,
	&hdr_CommonNetServer.ObjectType,
	&hdr_CommonNetBufferPool.ObjectType,
	&hdr_BillingProxy.ObjectType,
	&hdr_CSRBridgeServer.ObjectType,
	&hdr_BattleServer.ObjectType,
	&hdr_GlobalGameEventServer.ObjectType,
	NULL
};
//function tables
struct PERFCOUNTER_FUNCTION_TABLE
{
	void *(*pfnCreate)(__in __notnull WCHAR*);
	void (*UpdateOffsets)(__in __notnull void*,DWORD,DWORD);
	DWORD (*pfnGetHeaderSize)(void);
	DWORD (*pfnGetInstanceDataSize)(void);
	DWORD  (*pfnGetHeaderData)(__in __bcount(dwBufSize)BYTE *pBuffer,DWORD dwBufSize ,int );
	DWORD (*pfnGetInstanceData)(__in __notnull PERFINSTANCE_BASE *,__in __bcount(dwBufSize)BYTE*pBuffer,DWORD dwBufSize );
\
};

// Functions for UserServer
inline void *Create_PERFCOUNTER_INSTANCE_UserServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_UserServer *ptr = (PERFCOUNTER_INSTANCE_UserServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_UserServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_UserServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_UserServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_UserServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_UserServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	UserServer_data_header * pHdr = (UserServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->numClientsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numClientsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numServersDef.CounterNameTitleIndex += titleOffset;
	pHdr->numServersDef.CounterHelpTitleIndex += helpOffset;
	pHdr->byteRateOutDef.CounterNameTitleIndex += titleOffset;
	pHdr->byteRateOutDef.CounterHelpTitleIndex += helpOffset;
	pHdr->byteRateInDef.CounterNameTitleIndex += titleOffset;
	pHdr->byteRateInDef.CounterHelpTitleIndex += helpOffset;
	pHdr->invalidCommandCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->invalidCommandCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->incomingMessageRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->incomingMessageRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->outGoingMessageRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->outGoingMessageRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numSessionsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numSessionsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->messageRateExceededDef.CounterNameTitleIndex += titleOffset;
	pHdr->messageRateExceededDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_UserServer(void){
	return sizeof(UserServer_data_header);
};
inline DWORD GetInstanceDataSize_UserServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_UserServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_UserServer,instance);
};
inline DWORD GetHeaderData_UserServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_UserServer() > dataSize){
		return 0;
	};
	hdr_UserServer.ObjectType.NumInstances = instanceCount;
	hdr_UserServer.ObjectType.TotalByteLength = sizeof(UserServer_data_header) + GetInstanceDataSize_UserServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_UserServer,sizeof(hdr_UserServer));
	return sizeof(hdr_UserServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_UserServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_UserServer*ptr = (PERFCOUNTER_INSTANCE_UserServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_UserServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_UserServer,instance)));
	return dwWrote;
}
// Functions for GameServerThread
inline void *Create_PERFCOUNTER_INSTANCE_GameServerThread(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_GameServerThread *ptr = (PERFCOUNTER_INSTANCE_GameServerThread*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_GameServerThread));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_GameServerThread;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_GameServerThread) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameServerThread,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_GameServerThread(void *ptr,DWORD titleOffset,DWORD helpOffset){
	GameServerThread_data_header * pHdr = (GameServerThread_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->GameServerThreadGameTicksDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerThreadGameTicksDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerThreadMaxSimultaneousTicksDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerThreadMaxSimultaneousTicksDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerThreadGameTickRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerThreadGameTickRateDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_GameServerThread(void){
	return sizeof(GameServerThread_data_header);
};
inline DWORD GetInstanceDataSize_GameServerThread(void){
	return sizeof(PERFCOUNTER_INSTANCE_GameServerThread)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameServerThread,instance);
};
inline DWORD GetHeaderData_GameServerThread(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_GameServerThread() > dataSize){
		return 0;
	};
	hdr_GameServerThread.ObjectType.NumInstances = instanceCount;
	hdr_GameServerThread.ObjectType.TotalByteLength = sizeof(GameServerThread_data_header) + GetInstanceDataSize_GameServerThread()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_GameServerThread,sizeof(hdr_GameServerThread));
	return sizeof(hdr_GameServerThread);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_GameServerThread(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_GameServerThread*ptr = (PERFCOUNTER_INSTANCE_GameServerThread*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_GameServerThread)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameServerThread,instance)));
	return dwWrote;
}
// Functions for GameServer
inline void *Create_PERFCOUNTER_INSTANCE_GameServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_GameServer *ptr = (PERFCOUNTER_INSTANCE_GameServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_GameServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_GameServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_GameServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_GameServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	GameServer_data_header * pHdr = (GameServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->GameServerClientCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerClientCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerAlltimeClientCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerAlltimeClientCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerClientDetachCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerClientDetachCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerSendErrorCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerSendErrorCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerPingBootCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerPingBootCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerIdValidateBootCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerIdValidateBootCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerMessageRateBootCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerMessageRateBootCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerCCMD_EXIT_GAMECountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerCCMD_EXIT_GAMECountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerMaximumClientCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerMaximumClientCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerGameCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerGameCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerLevelCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerLevelCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerUnitCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerUnitCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerSimulationFramesDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerSimulationFramesDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerSimulationFrameRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerSimulationFrameRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerGameTickRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerGameTickRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerDRLGRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerDRLGRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerUnitStepRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerUnitStepRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerUnitStepMonsterRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerUnitStepMonsterRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerUnitStepMissileRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerUnitStepMissileRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerUnitStepItemRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerUnitStepItemRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerUnitStepPlayerRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerUnitStepPlayerRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerGameEventRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerGameEventRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerGameEventHandlersDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerGameEventHandlersDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerAStarPathRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerAStarPathRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerLinearPathRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerLinearPathRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerPathFailureRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerPathFailureRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerMessagesReceivedPerSecondDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerMessagesReceivedPerSecondDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GameServerMessagesSentPerSecondDef.CounterNameTitleIndex += titleOffset;
	pHdr->GameServerMessagesSentPerSecondDef.CounterHelpTitleIndex += helpOffset;
	pHdr->RoomGetNearestPathNodesInArrayDef.CounterNameTitleIndex += titleOffset;
	pHdr->RoomGetNearestPathNodesInArrayDef.CounterHelpTitleIndex += helpOffset;
	pHdr->RoomGetNearestFreePathNodeDef.CounterNameTitleIndex += titleOffset;
	pHdr->RoomGetNearestFreePathNodeDef.CounterHelpTitleIndex += helpOffset;
	pHdr->RoomGetNearestPathNodesDef.CounterNameTitleIndex += titleOffset;
	pHdr->RoomGetNearestPathNodesDef.CounterHelpTitleIndex += helpOffset;
	pHdr->PingDef.CounterNameTitleIndex += titleOffset;
	pHdr->PingDef.CounterHelpTitleIndex += helpOffset;
	pHdr->ExperienceGainCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->ExperienceGainCountDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_GameServer(void){
	return sizeof(GameServer_data_header);
};
inline DWORD GetInstanceDataSize_GameServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_GameServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameServer,instance);
};
inline DWORD GetHeaderData_GameServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_GameServer() > dataSize){
		return 0;
	};
	hdr_GameServer.ObjectType.NumInstances = instanceCount;
	hdr_GameServer.ObjectType.TotalByteLength = sizeof(GameServer_data_header) + GetInstanceDataSize_GameServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_GameServer,sizeof(hdr_GameServer));
	return sizeof(hdr_GameServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_GameServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_GameServer*ptr = (PERFCOUNTER_INSTANCE_GameServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_GameServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameServer,instance)));
	return dwWrote;
}
// Functions for CHAT_SERVER
inline void *Create_PERFCOUNTER_INSTANCE_CHAT_SERVER(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_CHAT_SERVER *ptr = (PERFCOUNTER_INSTANCE_CHAT_SERVER*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_CHAT_SERVER));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_CHAT_SERVER;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_CHAT_SERVER) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CHAT_SERVER,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_CHAT_SERVER(void *ptr,DWORD titleOffset,DWORD helpOffset){
	CHAT_SERVER_data_header * pHdr = (CHAT_SERVER_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_SYS_MEMBER_CONNECTIONSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_SYS_MEMBER_CONNECTIONSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_SYS_MEMBER_CONNECTION_RATEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_SYS_MEMBER_CONNECTION_RATEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_SYS_MEMBER_COMMAND_RATEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_SYS_MEMBER_COMMAND_RATEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_SYS_GAMESVR_CONNECTIONSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_SYS_GAMESVR_CONNECTIONSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_SYS_GAMESVR_COMMAND_RATEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_SYS_GAMESVR_COMMAND_RATEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_SYS_MULTIPOINT_SENDSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_SYS_MULTIPOINT_SENDSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_SYS_CSR_COMMAND_RATEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_SYS_CSR_COMMAND_RATEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MBR_MEMBER_OBJECT_COUNTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MBR_MEMBER_OBJECT_COUNTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MBR_DIRECT_MEMBER_MSG_RATEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MBR_DIRECT_MEMBER_MSG_RATEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_CNL_CHANNEL_OBJECT_COUNTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_CNL_CHANNEL_OBJECT_COUNTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_CNL_CHANNEL_MEMBER_MSG_RATEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_CNL_CHANNEL_MEMBER_MSG_RATEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_CNL_CHANNEL_MEMBER_MSG_RECIPIENTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_CNL_CHANNEL_MEMBER_MSG_RECIPIENTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_CNL_CHANNEL_CHAT_MSG_RATEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_CNL_CHANNEL_CHAT_MSG_RATEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PTY_CHAT_PARTY_OBJECT_COUNTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PTY_CHAT_PARTY_OBJECT_COUNTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PTY_GAMESVR_MSG_RATEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PTY_GAMESVR_MSG_RATEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PTY_GAMESVR_MSG_RECIPIENTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PTY_GAMESVR_MSG_RECIPIENTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PTY_PARTY_CHAT_CHANNEL_PERCENTILENumeratorDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PTY_PARTY_CHAT_CHANNEL_PERCENTILENumeratorDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_IM_CHAT_CHANNEL_COUNTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_IM_CHAT_CHANNEL_COUNTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_IM_CHAT_CHANNEL_PERCENTILENumeratorDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_IM_CHAT_CHANNEL_PERCENTILENumeratorDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PAD_ADVERTISED_PARTIES_COUNTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PAD_ADVERTISED_PARTIES_COUNTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PAD_PARTY_CLASSIFICATIONS_COUNTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PAD_PARTY_CLASSIFICATIONS_COUNTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PAD_PARTY_INFO_UPDATESDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PAD_PARTY_INFO_UPDATESDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PAD_PARTY_AD_GET_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PAD_PARTY_AD_GET_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_PAD_PARTY_ADS_SENTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_PAD_PARTY_ADS_SENTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_GLD_GUILD_COUNTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_GLD_GUILD_COUNTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_HAK_FAILED_LOGINSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_HAK_FAILED_LOGINSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_HAK_REJECTED_TIME_PROTECTED_OPSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_HAK_REJECTED_TIME_PROTECTED_OPSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_HAK_NOT_LOGGED_IN_COMMANDSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_HAK_NOT_LOGGED_IN_COMMANDSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_HAK_NOT_A_MEMBER_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_HAK_NOT_A_MEMBER_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_HAK_UNSUPPORTED_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_HAK_UNSUPPORTED_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MSGRATES_SEND_CHATDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MSGRATES_SEND_CHATDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MSGRATES_IGNORE_OPERATIONDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MSGRATES_IGNORE_OPERATIONDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MSGRATES_BUDDY_LIST_OPERATIONDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MSGRATES_BUDDY_LIST_OPERATIONDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MSGRATES_PARTY_OPERATIONDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MSGRATES_PARTY_OPERATIONDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MSGRATES_GUILD_OPERATIONDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MSGRATES_GUILD_OPERATIONDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MSGRATES_MISC_GETSET_INFODef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MSGRATES_MISC_GETSET_INFODef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_MSGRATES_MISC_CHANNEL_ADDLEAVEDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_MSGRATES_MISC_CHANNEL_ADDLEAVEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_EMAIL_SEND_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_EMAIL_SEND_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_EMAIL_COMPOSING_MESSAGESDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_EMAIL_COMPOSING_MESSAGESDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_EMAIL_ABORTED_COMPOSING_MESSAGESDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_EMAIL_ABORTED_COMPOSING_MESSAGESDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_EMAIL_RECEIVE_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_EMAIL_RECEIVE_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_EMAIL_RECEIVED_EMAILSDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_EMAIL_RECEIVED_EMAILSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_HYPERTEXT_TOTAL_MESSAGE_COUNTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_HYPERTEXT_TOTAL_MESSAGE_COUNTDef.CounterHelpTitleIndex += helpOffset;
	pHdr->CHATPERF_HYPERTEXT_TOTAL_DATA_BYTES_SENTDef.CounterNameTitleIndex += titleOffset;
	pHdr->CHATPERF_HYPERTEXT_TOTAL_DATA_BYTES_SENTDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_CHAT_SERVER(void){
	return sizeof(CHAT_SERVER_data_header);
};
inline DWORD GetInstanceDataSize_CHAT_SERVER(void){
	return sizeof(PERFCOUNTER_INSTANCE_CHAT_SERVER)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_CHAT_SERVER,instance);
};
inline DWORD GetHeaderData_CHAT_SERVER(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_CHAT_SERVER() > dataSize){
		return 0;
	};
	hdr_CHAT_SERVER.ObjectType.NumInstances = instanceCount;
	hdr_CHAT_SERVER.ObjectType.TotalByteLength = sizeof(CHAT_SERVER_data_header) + GetInstanceDataSize_CHAT_SERVER()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_CHAT_SERVER,sizeof(hdr_CHAT_SERVER));
	return sizeof(hdr_CHAT_SERVER);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_CHAT_SERVER(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_CHAT_SERVER*ptr = (PERFCOUNTER_INSTANCE_CHAT_SERVER*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_CHAT_SERVER)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CHAT_SERVER,instance)));
	return dwWrote;
}
// Functions for DATABASE_POOL
inline void *Create_PERFCOUNTER_INSTANCE_DATABASE_POOL(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_DATABASE_POOL *ptr = (PERFCOUNTER_INSTANCE_DATABASE_POOL*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_DATABASE_POOL));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_DATABASE_POOL;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_DATABASE_POOL) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_DATABASE_POOL,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_DATABASE_POOL(void *ptr,DWORD titleOffset,DWORD helpOffset){
	DATABASE_POOL_data_header * pHdr = (DATABASE_POOL_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->DBPERF_POOLED_CONNECTIONSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_POOLED_CONNECTIONSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_OUTSTANDING_QUERIESDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_OUTSTANDING_QUERIESDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_PROCESSED_QUERIESDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_PROCESSED_QUERIESDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_SINGLE_DATA_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_SINGLE_DATA_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_PARTIAL_BATCH_DATA_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_PARTIAL_BATCH_DATA_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_FULL_BATCH_DATA_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_FULL_BATCH_DATA_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_QUERY_PROC_TIMEDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_QUERY_PROC_TIMEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_RETRIEVE_RESULTS_TIMEDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_RETRIEVE_RESULTS_TIMEDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_FAILED_QUERIESDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_FAILED_QUERIESDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_WAITING_BATCH_TIMER_CALLBACKSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_WAITING_BATCH_TIMER_CALLBACKSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_PROCESSED_BATCH_TIMER_CALLBACKSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_PROCESSED_BATCH_TIMER_CALLBACKSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_CANCELED_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_CANCELED_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_ATTEMPTED_REQUEST_CANCELATIONSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_ATTEMPTED_REQUEST_CANCELATIONSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_TOTAL_REQUESTS_PROCESSED_ON_ACTIVE_CONNECTIONSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_TOTAL_REQUESTS_PROCESSED_ON_ACTIVE_CONNECTIONSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_CONNECTION_REESTABLISHMENTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_CONNECTION_REESTABLISHMENTSDef.CounterHelpTitleIndex += helpOffset;
	pHdr->DBPERF_LIFETIME_REQUESTSDef.CounterNameTitleIndex += titleOffset;
	pHdr->DBPERF_LIFETIME_REQUESTSDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_DATABASE_POOL(void){
	return sizeof(DATABASE_POOL_data_header);
};
inline DWORD GetInstanceDataSize_DATABASE_POOL(void){
	return sizeof(PERFCOUNTER_INSTANCE_DATABASE_POOL)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_DATABASE_POOL,instance);
};
inline DWORD GetHeaderData_DATABASE_POOL(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_DATABASE_POOL() > dataSize){
		return 0;
	};
	hdr_DATABASE_POOL.ObjectType.NumInstances = instanceCount;
	hdr_DATABASE_POOL.ObjectType.TotalByteLength = sizeof(DATABASE_POOL_data_header) + GetInstanceDataSize_DATABASE_POOL()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_DATABASE_POOL,sizeof(hdr_DATABASE_POOL));
	return sizeof(hdr_DATABASE_POOL);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_DATABASE_POOL(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_DATABASE_POOL*ptr = (PERFCOUNTER_INSTANCE_DATABASE_POOL*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_DATABASE_POOL)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_DATABASE_POOL,instance)));
	return dwWrote;
}
// Functions for CMemoryAllocator
inline void *Create_PERFCOUNTER_INSTANCE_CMemoryAllocator(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_CMemoryAllocator *ptr = (PERFCOUNTER_INSTANCE_CMemoryAllocator*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_CMemoryAllocator));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_CMemoryAllocator;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_CMemoryAllocator) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CMemoryAllocator,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_CMemoryAllocator(void *ptr,DWORD titleOffset,DWORD helpOffset){
	CMemoryAllocator_data_header * pHdr = (CMemoryAllocator_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->TotalExternalAllocationSizeDef.CounterNameTitleIndex += titleOffset;
	pHdr->TotalExternalAllocationSizeDef.CounterHelpTitleIndex += helpOffset;
	pHdr->ExternalAllocationRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->ExternalAllocationRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->ExternalFreeRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->ExternalFreeRateDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_CMemoryAllocator(void){
	return sizeof(CMemoryAllocator_data_header);
};
inline DWORD GetInstanceDataSize_CMemoryAllocator(void){
	return sizeof(PERFCOUNTER_INSTANCE_CMemoryAllocator)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_CMemoryAllocator,instance);
};
inline DWORD GetHeaderData_CMemoryAllocator(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_CMemoryAllocator() > dataSize){
		return 0;
	};
	hdr_CMemoryAllocator.ObjectType.NumInstances = instanceCount;
	hdr_CMemoryAllocator.ObjectType.TotalByteLength = sizeof(CMemoryAllocator_data_header) + GetInstanceDataSize_CMemoryAllocator()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_CMemoryAllocator,sizeof(hdr_CMemoryAllocator));
	return sizeof(hdr_CMemoryAllocator);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_CMemoryAllocator(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_CMemoryAllocator*ptr = (PERFCOUNTER_INSTANCE_CMemoryAllocator*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_CMemoryAllocator)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CMemoryAllocator,instance)));
	return dwWrote;
}
// Functions for LoginServer
inline void *Create_PERFCOUNTER_INSTANCE_LoginServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_LoginServer *ptr = (PERFCOUNTER_INSTANCE_LoginServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_LoginServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_LoginServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_LoginServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_LoginServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_LoginServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	LoginServer_data_header * pHdr = (LoginServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->numLoginClientsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numLoginClientsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->loginRequestRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->loginRequestRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->loginRequestsPendingDef.CounterNameTitleIndex += titleOffset;
	pHdr->loginRequestsPendingDef.CounterHelpTitleIndex += helpOffset;
	pHdr->loginRequestsSuccessfulDef.CounterNameTitleIndex += titleOffset;
	pHdr->loginRequestsSuccessfulDef.CounterHelpTitleIndex += helpOffset;
	pHdr->loginRequestsFailedDef.CounterNameTitleIndex += titleOffset;
	pHdr->loginRequestsFailedDef.CounterHelpTitleIndex += helpOffset;
	pHdr->usedTicketCacheSizeDef.CounterNameTitleIndex += titleOffset;
	pHdr->usedTicketCacheSizeDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_LoginServer(void){
	return sizeof(LoginServer_data_header);
};
inline DWORD GetInstanceDataSize_LoginServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_LoginServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_LoginServer,instance);
};
inline DWORD GetHeaderData_LoginServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_LoginServer() > dataSize){
		return 0;
	};
	hdr_LoginServer.ObjectType.NumInstances = instanceCount;
	hdr_LoginServer.ObjectType.TotalByteLength = sizeof(LoginServer_data_header) + GetInstanceDataSize_LoginServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_LoginServer,sizeof(hdr_LoginServer));
	return sizeof(hdr_LoginServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_LoginServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_LoginServer*ptr = (PERFCOUNTER_INSTANCE_LoginServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_LoginServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_LoginServer,instance)));
	return dwWrote;
}
// Functions for MuxServer
inline void *Create_PERFCOUNTER_INSTANCE_MuxServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_MuxServer *ptr = (PERFCOUNTER_INSTANCE_MuxServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_MuxServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_MuxServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_MuxServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_MuxServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_MuxServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	MuxServer_data_header * pHdr = (MuxServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->muxServerBytesSentDef.CounterNameTitleIndex += titleOffset;
	pHdr->muxServerBytesSentDef.CounterHelpTitleIndex += helpOffset;
	pHdr->muxServerBytesRecvdDef.CounterNameTitleIndex += titleOffset;
	pHdr->muxServerBytesRecvdDef.CounterHelpTitleIndex += helpOffset;
	pHdr->muxServerActualClientCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->muxServerActualClientCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->muxServerVirtualClientCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->muxServerVirtualClientCountDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_MuxServer(void){
	return sizeof(MuxServer_data_header);
};
inline DWORD GetInstanceDataSize_MuxServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_MuxServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_MuxServer,instance);
};
inline DWORD GetHeaderData_MuxServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_MuxServer() > dataSize){
		return 0;
	};
	hdr_MuxServer.ObjectType.NumInstances = instanceCount;
	hdr_MuxServer.ObjectType.TotalByteLength = sizeof(MuxServer_data_header) + GetInstanceDataSize_MuxServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_MuxServer,sizeof(hdr_MuxServer));
	return sizeof(hdr_MuxServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_MuxServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_MuxServer*ptr = (PERFCOUNTER_INSTANCE_MuxServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_MuxServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_MuxServer,instance)));
	return dwWrote;
}
// Functions for MuxClientImpl
inline void *Create_PERFCOUNTER_INSTANCE_MuxClientImpl(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_MuxClientImpl *ptr = (PERFCOUNTER_INSTANCE_MuxClientImpl*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_MuxClientImpl));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_MuxClientImpl;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_MuxClientImpl) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_MuxClientImpl,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_MuxClientImpl(void *ptr,DWORD titleOffset,DWORD helpOffset){
	MuxClientImpl_data_header * pHdr = (MuxClientImpl_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->muxClientBytesSentDef.CounterNameTitleIndex += titleOffset;
	pHdr->muxClientBytesSentDef.CounterHelpTitleIndex += helpOffset;
	pHdr->muxClientBytesRecvdDef.CounterNameTitleIndex += titleOffset;
	pHdr->muxClientBytesRecvdDef.CounterHelpTitleIndex += helpOffset;
	pHdr->muxClientCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->muxClientCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->muxClientDiscPendingDef.CounterNameTitleIndex += titleOffset;
	pHdr->muxClientDiscPendingDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_MuxClientImpl(void){
	return sizeof(MuxClientImpl_data_header);
};
inline DWORD GetInstanceDataSize_MuxClientImpl(void){
	return sizeof(PERFCOUNTER_INSTANCE_MuxClientImpl)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_MuxClientImpl,instance);
};
inline DWORD GetHeaderData_MuxClientImpl(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_MuxClientImpl() > dataSize){
		return 0;
	};
	hdr_MuxClientImpl.ObjectType.NumInstances = instanceCount;
	hdr_MuxClientImpl.ObjectType.TotalByteLength = sizeof(MuxClientImpl_data_header) + GetInstanceDataSize_MuxClientImpl()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_MuxClientImpl,sizeof(hdr_MuxClientImpl));
	return sizeof(hdr_MuxClientImpl);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_MuxClientImpl(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_MuxClientImpl*ptr = (PERFCOUNTER_INSTANCE_MuxClientImpl*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_MuxClientImpl)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_MuxClientImpl,instance)));
	return dwWrote;
}
// Functions for HLOCK_LIB
inline void *Create_PERFCOUNTER_INSTANCE_HLOCK_LIB(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_HLOCK_LIB *ptr = (PERFCOUNTER_INSTANCE_HLOCK_LIB*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_HLOCK_LIB));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_HLOCK_LIB;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_HLOCK_LIB) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_HLOCK_LIB,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_HLOCK_LIB(void *ptr,DWORD titleOffset,DWORD helpOffset){
	HLOCK_LIB_data_header * pHdr = (HLOCK_LIB_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->LocksHeldDef.CounterNameTitleIndex += titleOffset;
	pHdr->LocksHeldDef.CounterHelpTitleIndex += helpOffset;
	pHdr->LockContentionDef.CounterNameTitleIndex += titleOffset;
	pHdr->LockContentionDef.CounterHelpTitleIndex += helpOffset;
	pHdr->LockOpsDef.CounterNameTitleIndex += titleOffset;
	pHdr->LockOpsDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_HLOCK_LIB(void){
	return sizeof(HLOCK_LIB_data_header);
};
inline DWORD GetInstanceDataSize_HLOCK_LIB(void){
	return sizeof(PERFCOUNTER_INSTANCE_HLOCK_LIB)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_HLOCK_LIB,instance);
};
inline DWORD GetHeaderData_HLOCK_LIB(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_HLOCK_LIB() > dataSize){
		return 0;
	};
	hdr_HLOCK_LIB.ObjectType.NumInstances = instanceCount;
	hdr_HLOCK_LIB.ObjectType.TotalByteLength = sizeof(HLOCK_LIB_data_header) + GetInstanceDataSize_HLOCK_LIB()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_HLOCK_LIB,sizeof(hdr_HLOCK_LIB));
	return sizeof(hdr_HLOCK_LIB);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_HLOCK_LIB(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_HLOCK_LIB*ptr = (PERFCOUNTER_INSTANCE_HLOCK_LIB*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_HLOCK_LIB)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_HLOCK_LIB,instance)));
	return dwWrote;
}
// Functions for PatchServer
inline void *Create_PERFCOUNTER_INSTANCE_PatchServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_PatchServer *ptr = (PERFCOUNTER_INSTANCE_PatchServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_PatchServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_PatchServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_PatchServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_PatchServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_PatchServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	PatchServer_data_header * pHdr = (PatchServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->numCurrentClientsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numCurrentClientsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numWaitingClientsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numWaitingClientsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numReadyClientsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numReadyClientsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numStalledClientsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numStalledClientsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->UnackedBytesDef.CounterNameTitleIndex += titleOffset;
	pHdr->UnackedBytesDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numFilesPatchedDef.CounterNameTitleIndex += titleOffset;
	pHdr->numFilesPatchedDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numSendingBytesDef.CounterNameTitleIndex += titleOffset;
	pHdr->numSendingBytesDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numSendingMsgsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numSendingMsgsDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_PatchServer(void){
	return sizeof(PatchServer_data_header);
};
inline DWORD GetInstanceDataSize_PatchServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_PatchServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_PatchServer,instance);
};
inline DWORD GetHeaderData_PatchServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_PatchServer() > dataSize){
		return 0;
	};
	hdr_PatchServer.ObjectType.NumInstances = instanceCount;
	hdr_PatchServer.ObjectType.TotalByteLength = sizeof(PatchServer_data_header) + GetInstanceDataSize_PatchServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_PatchServer,sizeof(hdr_PatchServer));
	return sizeof(hdr_PatchServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_PatchServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_PatchServer*ptr = (PERFCOUNTER_INSTANCE_PatchServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_PatchServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_PatchServer,instance)));
	return dwWrote;
}
// Functions for GameLoadBalanceServer
inline void *Create_PERFCOUNTER_INSTANCE_GameLoadBalanceServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_GameLoadBalanceServer *ptr = (PERFCOUNTER_INSTANCE_GameLoadBalanceServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_GameLoadBalanceServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_GameLoadBalanceServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_GameLoadBalanceServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameLoadBalanceServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_GameLoadBalanceServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	GameLoadBalanceServer_data_header * pHdr = (GameLoadBalanceServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->numGameServerClientsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numGameServerClientsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numGameLoginClientsDef.CounterNameTitleIndex += titleOffset;
	pHdr->numGameLoginClientsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameLoginRequestsPendingDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameLoginRequestsPendingDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameLoginRequestsSuccessfulDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameLoginRequestsSuccessfulDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameLoginRequestsFailedDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameLoginRequestsFailedDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameSwitchRequestsDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameSwitchRequestsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameServerLoginsDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameServerLoginsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameServerLogoutsDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameServerLogoutsDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameServerDBFinishesDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameServerDBFinishesDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameTotalQueueLengthDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameTotalQueueLengthDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameSubscriptionPlayersInQueueDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameSubscriptionPlayersInQueueDef.CounterHelpTitleIndex += helpOffset;
	pHdr->gameFreePlayPlayersInQueueDef.CounterNameTitleIndex += titleOffset;
	pHdr->gameFreePlayPlayersInQueueDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_GameLoadBalanceServer(void){
	return sizeof(GameLoadBalanceServer_data_header);
};
inline DWORD GetInstanceDataSize_GameLoadBalanceServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_GameLoadBalanceServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameLoadBalanceServer,instance);
};
inline DWORD GetHeaderData_GameLoadBalanceServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_GameLoadBalanceServer() > dataSize){
		return 0;
	};
	hdr_GameLoadBalanceServer.ObjectType.NumInstances = instanceCount;
	hdr_GameLoadBalanceServer.ObjectType.TotalByteLength = sizeof(GameLoadBalanceServer_data_header) + GetInstanceDataSize_GameLoadBalanceServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_GameLoadBalanceServer,sizeof(hdr_GameLoadBalanceServer));
	return sizeof(hdr_GameLoadBalanceServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_GameLoadBalanceServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_GameLoadBalanceServer*ptr = (PERFCOUNTER_INSTANCE_GameLoadBalanceServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_GameLoadBalanceServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_GameLoadBalanceServer,instance)));
	return dwWrote;
}
// Functions for CommonNetWorker
inline void *Create_PERFCOUNTER_INSTANCE_CommonNetWorker(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_CommonNetWorker *ptr = (PERFCOUNTER_INSTANCE_CommonNetWorker*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_CommonNetWorker));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_CommonNetWorker;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_CommonNetWorker) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetWorker,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_CommonNetWorker(void *ptr,DWORD titleOffset,DWORD helpOffset){
	CommonNetWorker_data_header * pHdr = (CommonNetWorker_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->schedulingRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->schedulingRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->sendCompletionRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->sendCompletionRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->recvCompletionRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->recvCompletionRateDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_CommonNetWorker(void){
	return sizeof(CommonNetWorker_data_header);
};
inline DWORD GetInstanceDataSize_CommonNetWorker(void){
	return sizeof(PERFCOUNTER_INSTANCE_CommonNetWorker)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetWorker,instance);
};
inline DWORD GetHeaderData_CommonNetWorker(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_CommonNetWorker() > dataSize){
		return 0;
	};
	hdr_CommonNetWorker.ObjectType.NumInstances = instanceCount;
	hdr_CommonNetWorker.ObjectType.TotalByteLength = sizeof(CommonNetWorker_data_header) + GetInstanceDataSize_CommonNetWorker()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_CommonNetWorker,sizeof(hdr_CommonNetWorker));
	return sizeof(hdr_CommonNetWorker);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_CommonNetWorker(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_CommonNetWorker*ptr = (PERFCOUNTER_INSTANCE_CommonNetWorker*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_CommonNetWorker)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetWorker,instance)));
	return dwWrote;
}
// Functions for CommonNetServer
inline void *Create_PERFCOUNTER_INSTANCE_CommonNetServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_CommonNetServer *ptr = (PERFCOUNTER_INSTANCE_CommonNetServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_CommonNetServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_CommonNetServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_CommonNetServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_CommonNetServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	CommonNetServer_data_header * pHdr = (CommonNetServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->numClientsConnectedDef.CounterNameTitleIndex += titleOffset;
	pHdr->numClientsConnectedDef.CounterHelpTitleIndex += helpOffset;
	pHdr->connectionRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->connectionRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numBytesOutstandingDef.CounterNameTitleIndex += titleOffset;
	pHdr->numBytesOutstandingDef.CounterHelpTitleIndex += helpOffset;
	pHdr->numSendsPendingDef.CounterNameTitleIndex += titleOffset;
	pHdr->numSendsPendingDef.CounterHelpTitleIndex += helpOffset;
	pHdr->connectionsThrottledBySendCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->connectionsThrottledBySendCountDef.CounterHelpTitleIndex += helpOffset;
	pHdr->connectionsThrottledByByteCountDef.CounterNameTitleIndex += titleOffset;
	pHdr->connectionsThrottledByByteCountDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_CommonNetServer(void){
	return sizeof(CommonNetServer_data_header);
};
inline DWORD GetInstanceDataSize_CommonNetServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_CommonNetServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetServer,instance);
};
inline DWORD GetHeaderData_CommonNetServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_CommonNetServer() > dataSize){
		return 0;
	};
	hdr_CommonNetServer.ObjectType.NumInstances = instanceCount;
	hdr_CommonNetServer.ObjectType.TotalByteLength = sizeof(CommonNetServer_data_header) + GetInstanceDataSize_CommonNetServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_CommonNetServer,sizeof(hdr_CommonNetServer));
	return sizeof(hdr_CommonNetServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_CommonNetServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_CommonNetServer*ptr = (PERFCOUNTER_INSTANCE_CommonNetServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_CommonNetServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetServer,instance)));
	return dwWrote;
}
// Functions for CommonNetBufferPool
inline void *Create_PERFCOUNTER_INSTANCE_CommonNetBufferPool(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_CommonNetBufferPool *ptr = (PERFCOUNTER_INSTANCE_CommonNetBufferPool*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_CommonNetBufferPool));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_CommonNetBufferPool;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_CommonNetBufferPool) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetBufferPool,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_CommonNetBufferPool(void *ptr,DWORD titleOffset,DWORD helpOffset){
	CommonNetBufferPool_data_header * pHdr = (CommonNetBufferPool_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->overlapAllocationRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->overlapAllocationRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->msgBufAllocationRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->msgBufAllocationRateDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_CommonNetBufferPool(void){
	return sizeof(CommonNetBufferPool_data_header);
};
inline DWORD GetInstanceDataSize_CommonNetBufferPool(void){
	return sizeof(PERFCOUNTER_INSTANCE_CommonNetBufferPool)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetBufferPool,instance);
};
inline DWORD GetHeaderData_CommonNetBufferPool(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_CommonNetBufferPool() > dataSize){
		return 0;
	};
	hdr_CommonNetBufferPool.ObjectType.NumInstances = instanceCount;
	hdr_CommonNetBufferPool.ObjectType.TotalByteLength = sizeof(CommonNetBufferPool_data_header) + GetInstanceDataSize_CommonNetBufferPool()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_CommonNetBufferPool,sizeof(hdr_CommonNetBufferPool));
	return sizeof(hdr_CommonNetBufferPool);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_CommonNetBufferPool(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_CommonNetBufferPool*ptr = (PERFCOUNTER_INSTANCE_CommonNetBufferPool*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_CommonNetBufferPool)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CommonNetBufferPool,instance)));
	return dwWrote;
}
// Functions for BillingProxy
inline void *Create_PERFCOUNTER_INSTANCE_BillingProxy(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_BillingProxy *ptr = (PERFCOUNTER_INSTANCE_BillingProxy*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_BillingProxy));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_BillingProxy;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_BillingProxy) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_BillingProxy,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_BillingProxy(void *ptr,DWORD titleOffset,DWORD helpOffset){
	BillingProxy_data_header * pHdr = (BillingProxy_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->CurrentUsersDef.CounterNameTitleIndex += titleOffset;
	pHdr->CurrentUsersDef.CounterHelpTitleIndex += helpOffset;
	pHdr->LifetimeClientsAttachedDef.CounterNameTitleIndex += titleOffset;
	pHdr->LifetimeClientsAttachedDef.CounterHelpTitleIndex += helpOffset;
	pHdr->AccountsDBLinkUpDef.CounterNameTitleIndex += titleOffset;
	pHdr->AccountsDBLinkUpDef.CounterHelpTitleIndex += helpOffset;
	pHdr->RequestRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->RequestRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->PendingResponsesDef.CounterNameTitleIndex += titleOffset;
	pHdr->PendingResponsesDef.CounterHelpTitleIndex += helpOffset;
	pHdr->BillingProviderLinkFlagDef.CounterNameTitleIndex += titleOffset;
	pHdr->BillingProviderLinkFlagDef.CounterHelpTitleIndex += helpOffset;
	pHdr->BillingProviderSendQueueSizeDef.CounterNameTitleIndex += titleOffset;
	pHdr->BillingProviderSendQueueSizeDef.CounterHelpTitleIndex += helpOffset;
	pHdr->BillingProviderMessageSendRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->BillingProviderMessageSendRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->BillingProviderMessageRecvRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->BillingProviderMessageRecvRateDef.CounterHelpTitleIndex += helpOffset;
	pHdr->BillingProviderResyncFlagDef.CounterNameTitleIndex += titleOffset;
	pHdr->BillingProviderResyncFlagDef.CounterHelpTitleIndex += helpOffset;
	pHdr->BillingProviderKicksDef.CounterNameTitleIndex += titleOffset;
	pHdr->BillingProviderKicksDef.CounterHelpTitleIndex += helpOffset;
	pHdr->BillingProviderErrorsDef.CounterNameTitleIndex += titleOffset;
	pHdr->BillingProviderErrorsDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_BillingProxy(void){
	return sizeof(BillingProxy_data_header);
};
inline DWORD GetInstanceDataSize_BillingProxy(void){
	return sizeof(PERFCOUNTER_INSTANCE_BillingProxy)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_BillingProxy,instance);
};
inline DWORD GetHeaderData_BillingProxy(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_BillingProxy() > dataSize){
		return 0;
	};
	hdr_BillingProxy.ObjectType.NumInstances = instanceCount;
	hdr_BillingProxy.ObjectType.TotalByteLength = sizeof(BillingProxy_data_header) + GetInstanceDataSize_BillingProxy()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_BillingProxy,sizeof(hdr_BillingProxy));
	return sizeof(hdr_BillingProxy);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_BillingProxy(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_BillingProxy*ptr = (PERFCOUNTER_INSTANCE_BillingProxy*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_BillingProxy)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_BillingProxy,instance)));
	return dwWrote;
}
// Functions for CSRBridgeServer
inline void *Create_PERFCOUNTER_INSTANCE_CSRBridgeServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_CSRBridgeServer *ptr = (PERFCOUNTER_INSTANCE_CSRBridgeServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_CSRBridgeServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_CSRBridgeServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_CSRBridgeServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CSRBridgeServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_CSRBridgeServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	CSRBridgeServer_data_header * pHdr = (CSRBridgeServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->NumNamedPipesOpenDef.CounterNameTitleIndex += titleOffset;
	pHdr->NumNamedPipesOpenDef.CounterHelpTitleIndex += helpOffset;
	pHdr->NumXmlMessagesRecvdDef.CounterNameTitleIndex += titleOffset;
	pHdr->NumXmlMessagesRecvdDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_CSRBridgeServer(void){
	return sizeof(CSRBridgeServer_data_header);
};
inline DWORD GetInstanceDataSize_CSRBridgeServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_CSRBridgeServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_CSRBridgeServer,instance);
};
inline DWORD GetHeaderData_CSRBridgeServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_CSRBridgeServer() > dataSize){
		return 0;
	};
	hdr_CSRBridgeServer.ObjectType.NumInstances = instanceCount;
	hdr_CSRBridgeServer.ObjectType.TotalByteLength = sizeof(CSRBridgeServer_data_header) + GetInstanceDataSize_CSRBridgeServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_CSRBridgeServer,sizeof(hdr_CSRBridgeServer));
	return sizeof(hdr_CSRBridgeServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_CSRBridgeServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_CSRBridgeServer*ptr = (PERFCOUNTER_INSTANCE_CSRBridgeServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_CSRBridgeServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_CSRBridgeServer,instance)));
	return dwWrote;
}
// Functions for BattleServer
inline void *Create_PERFCOUNTER_INSTANCE_BattleServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_BattleServer *ptr = (PERFCOUNTER_INSTANCE_BattleServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_BattleServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_BattleServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_BattleServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_BattleServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_BattleServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	BattleServer_data_header * pHdr = (BattleServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->BattleServerSimulationFramesDef.CounterNameTitleIndex += titleOffset;
	pHdr->BattleServerSimulationFramesDef.CounterHelpTitleIndex += helpOffset;
	pHdr->BattleServerSimulationFrameRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->BattleServerSimulationFrameRateDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_BattleServer(void){
	return sizeof(BattleServer_data_header);
};
inline DWORD GetInstanceDataSize_BattleServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_BattleServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_BattleServer,instance);
};
inline DWORD GetHeaderData_BattleServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_BattleServer() > dataSize){
		return 0;
	};
	hdr_BattleServer.ObjectType.NumInstances = instanceCount;
	hdr_BattleServer.ObjectType.TotalByteLength = sizeof(BattleServer_data_header) + GetInstanceDataSize_BattleServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_BattleServer,sizeof(hdr_BattleServer));
	return sizeof(hdr_BattleServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_BattleServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_BattleServer*ptr = (PERFCOUNTER_INSTANCE_BattleServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_BattleServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_BattleServer,instance)));
	return dwWrote;
}
// Functions for GlobalGameEventServer
inline void *Create_PERFCOUNTER_INSTANCE_GlobalGameEventServer(WCHAR *instanceName){
	PERFCOUNTER_INSTANCE_GlobalGameEventServer *ptr = (PERFCOUNTER_INSTANCE_GlobalGameEventServer*)PerfHelper::Malloc(sizeof(PERFCOUNTER_INSTANCE_GlobalGameEventServer));
	PStrCopy(ptr->name,instanceName,sizeof(ptr->name)/sizeof(ptr->name[0]));
	ptr->type = PERFCOUNTER_OBJECT_TYPE_GlobalGameEventServer;
	ptr->instance.ByteLength = sizeof(ptr->instance) + sizeof(ptr->name);
	ptr->instance.NameOffset = sizeof(ptr->instance);
	ptr->instance.UniqueID = PERF_NO_UNIQUE_ID;
	ptr->instance.NameLength = PStrLen(ptr->name)*sizeof(ptr->name[0]) + sizeof(ptr->name[0]);//includes NULL terminator
	ptr->CounterBlock.ByteLength = sizeof(PERFCOUNTER_INSTANCE_GlobalGameEventServer) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_GlobalGameEventServer,CounterBlock);
	return ptr;
};
inline void UpdateOffsets_GlobalGameEventServer(void *ptr,DWORD titleOffset,DWORD helpOffset){
	GlobalGameEventServer_data_header * pHdr = (GlobalGameEventServer_data_header*)ptr;
	pHdr->ObjectType.ObjectNameTitleIndex += titleOffset;
	pHdr->ObjectType.ObjectHelpTitleIndex += helpOffset;
	pHdr->GlobalGameEventServerSimulationFramesDef.CounterNameTitleIndex += titleOffset;
	pHdr->GlobalGameEventServerSimulationFramesDef.CounterHelpTitleIndex += helpOffset;
	pHdr->GlobalGameEventServerSimulationFrameRateDef.CounterNameTitleIndex += titleOffset;
	pHdr->GlobalGameEventServerSimulationFrameRateDef.CounterHelpTitleIndex += helpOffset;
};
inline DWORD GetHeaderSize_GlobalGameEventServer(void){
	return sizeof(GlobalGameEventServer_data_header);
};
inline DWORD GetInstanceDataSize_GlobalGameEventServer(void){
	return sizeof(PERFCOUNTER_INSTANCE_GlobalGameEventServer)- FIELD_OFFSET(PERFCOUNTER_INSTANCE_GlobalGameEventServer,instance);
};
inline DWORD GetHeaderData_GlobalGameEventServer(BYTE *pBuffer, DWORD dataSize,int instanceCount){
	if(GetHeaderSize_GlobalGameEventServer() > dataSize){
		return 0;
	};
	hdr_GlobalGameEventServer.ObjectType.NumInstances = instanceCount;
	hdr_GlobalGameEventServer.ObjectType.TotalByteLength = sizeof(GlobalGameEventServer_data_header) + GetInstanceDataSize_GlobalGameEventServer()*(instanceCount==0?1:instanceCount);
	memcpy(pBuffer,&hdr_GlobalGameEventServer,sizeof(hdr_GlobalGameEventServer));
	return sizeof(hdr_GlobalGameEventServer);
};
inline DWORD GetInstanceData_PERFCOUNTER_INSTANCE_GlobalGameEventServer(PERFINSTANCE_BASE*pInstance,BYTE *pBuffer, DWORD dataSize){
	PERFCOUNTER_INSTANCE_GlobalGameEventServer*ptr = (PERFCOUNTER_INSTANCE_GlobalGameEventServer*)pInstance;
	DWORD dwWrote = 0;
	if(dataSize < sizeof(PERFCOUNTER_INSTANCE_GlobalGameEventServer)){
		return 0;
	}
	memcpy(pBuffer,&ptr->instance,dwWrote = (sizeof(*ptr) - FIELD_OFFSET(PERFCOUNTER_INSTANCE_GlobalGameEventServer,instance)));
	return dwWrote;
}
PERFCOUNTER_FUNCTION_TABLE g_PerfFuncTable[PERFCOUNTER_OBJECT_TYPE_MAX + 1]=
{
	//UserServer
	{
		Create_PERFCOUNTER_INSTANCE_UserServer,
		UpdateOffsets_UserServer,
		GetHeaderSize_UserServer,
		GetInstanceDataSize_UserServer,
		GetHeaderData_UserServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_UserServer
	},
	//GameServerThread
	{
		Create_PERFCOUNTER_INSTANCE_GameServerThread,
		UpdateOffsets_GameServerThread,
		GetHeaderSize_GameServerThread,
		GetInstanceDataSize_GameServerThread,
		GetHeaderData_GameServerThread,
		GetInstanceData_PERFCOUNTER_INSTANCE_GameServerThread
	},
	//GameServer
	{
		Create_PERFCOUNTER_INSTANCE_GameServer,
		UpdateOffsets_GameServer,
		GetHeaderSize_GameServer,
		GetInstanceDataSize_GameServer,
		GetHeaderData_GameServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_GameServer
	},
	//CHAT_SERVER
	{
		Create_PERFCOUNTER_INSTANCE_CHAT_SERVER,
		UpdateOffsets_CHAT_SERVER,
		GetHeaderSize_CHAT_SERVER,
		GetInstanceDataSize_CHAT_SERVER,
		GetHeaderData_CHAT_SERVER,
		GetInstanceData_PERFCOUNTER_INSTANCE_CHAT_SERVER
	},
	//DATABASE_POOL
	{
		Create_PERFCOUNTER_INSTANCE_DATABASE_POOL,
		UpdateOffsets_DATABASE_POOL,
		GetHeaderSize_DATABASE_POOL,
		GetInstanceDataSize_DATABASE_POOL,
		GetHeaderData_DATABASE_POOL,
		GetInstanceData_PERFCOUNTER_INSTANCE_DATABASE_POOL
	},
	//CMemoryAllocator
	{
		Create_PERFCOUNTER_INSTANCE_CMemoryAllocator,
		UpdateOffsets_CMemoryAllocator,
		GetHeaderSize_CMemoryAllocator,
		GetInstanceDataSize_CMemoryAllocator,
		GetHeaderData_CMemoryAllocator,
		GetInstanceData_PERFCOUNTER_INSTANCE_CMemoryAllocator
	},
	//LoginServer
	{
		Create_PERFCOUNTER_INSTANCE_LoginServer,
		UpdateOffsets_LoginServer,
		GetHeaderSize_LoginServer,
		GetInstanceDataSize_LoginServer,
		GetHeaderData_LoginServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_LoginServer
	},
	//MuxServer
	{
		Create_PERFCOUNTER_INSTANCE_MuxServer,
		UpdateOffsets_MuxServer,
		GetHeaderSize_MuxServer,
		GetInstanceDataSize_MuxServer,
		GetHeaderData_MuxServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_MuxServer
	},
	//MuxClientImpl
	{
		Create_PERFCOUNTER_INSTANCE_MuxClientImpl,
		UpdateOffsets_MuxClientImpl,
		GetHeaderSize_MuxClientImpl,
		GetInstanceDataSize_MuxClientImpl,
		GetHeaderData_MuxClientImpl,
		GetInstanceData_PERFCOUNTER_INSTANCE_MuxClientImpl
	},
	//HLOCK_LIB
	{
		Create_PERFCOUNTER_INSTANCE_HLOCK_LIB,
		UpdateOffsets_HLOCK_LIB,
		GetHeaderSize_HLOCK_LIB,
		GetInstanceDataSize_HLOCK_LIB,
		GetHeaderData_HLOCK_LIB,
		GetInstanceData_PERFCOUNTER_INSTANCE_HLOCK_LIB
	},
	//PatchServer
	{
		Create_PERFCOUNTER_INSTANCE_PatchServer,
		UpdateOffsets_PatchServer,
		GetHeaderSize_PatchServer,
		GetInstanceDataSize_PatchServer,
		GetHeaderData_PatchServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_PatchServer
	},
	//GameLoadBalanceServer
	{
		Create_PERFCOUNTER_INSTANCE_GameLoadBalanceServer,
		UpdateOffsets_GameLoadBalanceServer,
		GetHeaderSize_GameLoadBalanceServer,
		GetInstanceDataSize_GameLoadBalanceServer,
		GetHeaderData_GameLoadBalanceServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_GameLoadBalanceServer
	},
	//CommonNetWorker
	{
		Create_PERFCOUNTER_INSTANCE_CommonNetWorker,
		UpdateOffsets_CommonNetWorker,
		GetHeaderSize_CommonNetWorker,
		GetInstanceDataSize_CommonNetWorker,
		GetHeaderData_CommonNetWorker,
		GetInstanceData_PERFCOUNTER_INSTANCE_CommonNetWorker
	},
	//CommonNetServer
	{
		Create_PERFCOUNTER_INSTANCE_CommonNetServer,
		UpdateOffsets_CommonNetServer,
		GetHeaderSize_CommonNetServer,
		GetInstanceDataSize_CommonNetServer,
		GetHeaderData_CommonNetServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_CommonNetServer
	},
	//CommonNetBufferPool
	{
		Create_PERFCOUNTER_INSTANCE_CommonNetBufferPool,
		UpdateOffsets_CommonNetBufferPool,
		GetHeaderSize_CommonNetBufferPool,
		GetInstanceDataSize_CommonNetBufferPool,
		GetHeaderData_CommonNetBufferPool,
		GetInstanceData_PERFCOUNTER_INSTANCE_CommonNetBufferPool
	},
	//BillingProxy
	{
		Create_PERFCOUNTER_INSTANCE_BillingProxy,
		UpdateOffsets_BillingProxy,
		GetHeaderSize_BillingProxy,
		GetInstanceDataSize_BillingProxy,
		GetHeaderData_BillingProxy,
		GetInstanceData_PERFCOUNTER_INSTANCE_BillingProxy
	},
	//CSRBridgeServer
	{
		Create_PERFCOUNTER_INSTANCE_CSRBridgeServer,
		UpdateOffsets_CSRBridgeServer,
		GetHeaderSize_CSRBridgeServer,
		GetInstanceDataSize_CSRBridgeServer,
		GetHeaderData_CSRBridgeServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_CSRBridgeServer
	},
	//BattleServer
	{
		Create_PERFCOUNTER_INSTANCE_BattleServer,
		UpdateOffsets_BattleServer,
		GetHeaderSize_BattleServer,
		GetInstanceDataSize_BattleServer,
		GetHeaderData_BattleServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_BattleServer
	},
	//GlobalGameEventServer
	{
		Create_PERFCOUNTER_INSTANCE_GlobalGameEventServer,
		UpdateOffsets_GlobalGameEventServer,
		GetHeaderSize_GlobalGameEventServer,
		GetInstanceDataSize_GlobalGameEventServer,
		GetHeaderData_GlobalGameEventServer,
		GetInstanceData_PERFCOUNTER_INSTANCE_GlobalGameEventServer
	},
	{NULL,NULL,NULL,NULL,NULL}
};

