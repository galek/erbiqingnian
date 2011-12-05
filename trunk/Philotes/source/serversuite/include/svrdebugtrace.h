//----------------------------------------------------------------------------
// svrdebugtrace.h
// 
// Modified     : $DateTime$
// by           : $Author$
//
// Component-specific stuff needed to do ETW tracing.
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef WPP_GUID_DEFINED
#define WPP_GUID_DEFINED
const GUID ServerRunner_WPPControlGuid = {
	0x47e9f13e,
	0x0403,
	0x4b67,
	{0x9d,0x96,0xad,0xee,0x23,0x5e,0xdb,0x6f} 
};
#else
extern const GUID ServerRunner_WPPControlGuid;
#endif //!WPP_GUID_DEFINED

//----------------------------------------------------------------------------
// WATCHDOG CLIENT TRACING GUID
//----------------------------------------------------------------------------
//	NOTE: make sure these are in the same order as their bits defined below!!!
#define WPP_CONTROL_GUIDS \
	WPP_DEFINE_CONTROL_GUID(\
	FlagshipServerTracing,(47e9f13e,0403,4b67,9d96,adee235edb6f), \
	WPP_DEFINE_BIT(TRACE_FLAG_ERROR) \
	WPP_DEFINE_BIT(TRACE_FLAG_DEBUG_BUILD_ONLY)\
	WPP_DEFINE_BIT(TRACE_FLAG_COMMON_NET)\
	WPP_DEFINE_BIT(TRACE_FLAG_PERFCTR_LIB)\
	WPP_DEFINE_BIT(TRACE_FLAG_MUXLIB)\
	WPP_DEFINE_BIT(TRACE_FLAG_SRUNNER)\
	WPP_DEFINE_BIT(TRACE_FLAG_SRUNNER_NET)\
	WPP_DEFINE_BIT(TRACE_FLAG_SRUNNER_DATABASE)\
	WPP_DEFINE_BIT(TRACE_FLAG_USER_SERVER)\
	WPP_DEFINE_BIT(TRACE_FLAG_CHAT_SERVER)\
	WPP_DEFINE_BIT(TRACE_FLAG_PATCH_SERVER)\
	WPP_DEFINE_BIT(TRACE_FLAG_LOGIN_SERVER)\
	WPP_DEFINE_BIT(TRACE_FLAG_GAME_MISC_LOG)\
	WPP_DEFINE_BIT(TRACE_FLAG_GAME_NET)\
	WPP_DEFINE_BIT(TRACE_FLAG_PIPE_SERVER)\
	WPP_DEFINE_BIT(TRACE_FLAG_LOADBALANCE_SERVER)\
	WPP_DEFINE_BIT(TRACE_FLAG_EVENTLOG)\
	WPP_DEFINE_BIT(TRACE_FLAG_BILLING_PROXY)\
	WPP_DEFINE_BIT(TRACE_FLAG_CSR_BRIDGE_SERVER)\
	WPP_DEFINE_BIT(TRACE_FLAG_BATTLE_SERVER)\
	WPP_DEFINE_BIT(TRACE_FLAG_MAIL)\
	WPP_DEFINE_BIT(TRACE_FLAG_EVENT_SERVER)\
	)\
	WPP_DEFINE_CONTROL_GUID(\
	PlayerActionTracing,(de1be20,8fc3,4f64,9c94,d78a966af2a5), \
	WPP_DEFINE_BIT(PLAYER_ACTION_FLAG) \
	)


//----------------------------------------------------------------------------
// TRACING DEFINES
//----------------------------------------------------------------------------
#define WPP_LEVEL_FLAGS_ENABLED(lvl,flags) WPP_LEVEL_ENABLED(flags) &&\
	WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl
#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)

#define TRACE_LEVEL_EXTRA_VERBOSE TRACE_LEVEL_RESERVED6

#ifdef _DEBUG
#ifndef NO_ETW_TRACE_TO_DEBUGGER
#define WPP_DEBUG(stuff)  server_trace stuff
#endif
#endif

//	NOTE: make sure these are in the same order as defined above!!!
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_ERROR				0x00000001
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_DEBUG_BUILD_ONLY	0x00000002
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_COMMON_NET			0x00000004
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_PERFCTR_LIB			0x00000008
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_MUXLIB				0x00000010
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_SRUNNER				0x00000020
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_SRUNNER_NET			0x00000040
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_SRUNNER_DATABASE	0x00000080
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_USER_SERVER			0x00000100
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_CHAT_SERVER			0x00000200
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_PATCH_SERVER		0x00000400
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_LOGIN_SERVER		0x00000800
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_GAME_MISC_LOG		0x00001000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_GAME_NET			0x00002000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_PIPE_SERVER			0x00004000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_LOADBALANCE_SERVER	0x00008000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_EVENTLOG 			0x00010000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_BILLING_PROXY		0x00020000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_CSR_BRIDGE_SERVER	0x00040000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_BATTLE_SERVER		0x00080000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_MAIL				0x00100000
#define WPP_ACTUAL_BIT_VALUE_TRACE_FLAG_GLOBAL_GAME_EVENT_SERVER	0x00200000

//	NOTE: make sure these are in the same order as defined above!!!
#define WPP_ACTUAL_BIT_VALUE_PLAYER_ACTION_FLAG		0x00000001

void server_trace(__in __notnull char *fmt, ...);
void server_trace(__in __notnull WCHAR *fmt, ...);
