//-------------------------------------------------------------------------------------------------
// debug.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------
#ifndef _DEBUG_H_
#define _DEBUG_H_


//-------------------------------------------------------------------------------------------------
// CONSTANTS
//-------------------------------------------------------------------------------------------------
// server is never retail but sometimes debug, server always wants to trace asserts
#if !ISVERSION(RETAIL_VERSION)
#	define DEBUG_USE_ASSERTS	1
#	define DEBUG_USE_TRACES		1
#endif


//-------------------------------------------------------------------------------------------------
// MACROS
//-------------------------------------------------------------------------------------------------
#define	FILE_LINE_ARGS_ON				, const char * file = "", unsigned int line = 0
#define	FILE_LINE_ARGS_OFF


#ifdef ASSERT
#	undef ASSERT
#endif // ASSERT

#if ISVERSION(DEVELOPMENT) && (defined(WIN32) || defined(_WIN64))
#	define IS_DEBUGGER_ATTACHED()		IsDebuggerAttached()
#else
#	define IS_DEBUGGER_ATTACHED()		(FALSE)
#endif // WIN32

#if DEBUG_USE_ASSERTS
#if ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
#define DEBUG_BREAK()					( IS_DEBUGGER_ATTACHED() ? __debugbreak() : DebugBreak() )
#else
#define DEBUG_BREAK()					( IS_DEBUGGER_ATTACHED() ? __debugbreak() : 0 )
#endif
#define ASSERTONCE_TAG					UIDEN(__ASSERTONCE_)
#define ASSERTDO_TAG					UIDEN(__ASSERTDO_)
#define ASSERT_MSG(m)					{ if (DoAssert("ERROR", m, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } }
#define ASSERTV_MSG(fmt, ...)			{ if (DoAssertEx("ERROR", __FILE__, __LINE__, __FUNCSIG__, fmt, __VA_ARGS__)) { DEBUG_BREAK(); } }
#define ErrorDoAssert(fmt, ...)			(DoAssertEx("ERROR", __FILE__, __LINE__, __FUNCSIG__, fmt, __VA_ARGS__) ? (DEBUG_BREAK(), 0) : 0)
#define ErrorDialog(fmt, ...)			(ErrorDialogEx(fmt, __VA_ARGS__) ? (DEBUG_BREAK(), 0) : 0)
#define HALT(exp)						{ if (!(exp)) {DoHalt(#exp, "", __FILE__, __LINE__, __FUNCSIG__);} }
#define FL_HALT(exp, f, l)				{ if (!(exp)) {DoHalt(#exp, "", f, l, __FUNCSIG__);} }
#define HALTX(exp, m)					{ if (!(exp)) {DoHalt(#exp, m, __FILE__, __LINE__, __FUNCSIG__);} }
#define FL_HALTX(exp, m, f, l)			{ if (!(exp)) {DoHalt(#exp, m, f, l, __FUNCSIG__);} }
#define HALTX_GS(exp, si, m)			{ if (!(exp)) {DoHalt(#exp, si, m, __FILE__, __LINE__, __FUNCSIG__);} }
#define WARN(exp)						{ if (!(exp)) {DoWarn(#exp, "", __FILE__, __LINE__, __FUNCSIG__);} }
#define FL_WARN(exp, f, l)				{ if (!(exp)) {DoWarn(#exp, "", f, l, __FUNCSIG__);} }
#define WARNX(exp, m)					{ if (!(exp)) {DoWarn(#exp, m, __FILE__, __LINE__, __FUNCSIG__);} }
#define FL_WARNX(exp, m, f, l)			{ if (!(exp)) {DoWarn(#exp, m, f, l, __FUNCSIG__);} }
#else
#define DEBUG_BREAK()					
#define ASSERTONCE_TAG					
#define ASSERT_MSG(m)					
#define ASSERTV_MSG(fmt, ...)			
#define ErrorDoAssert(fmt, ...)			
#define ErrorDialog(fmt, ...)			((void)0)
#define HALT(exp)						{ if (!(exp)) {DoHalt(#exp, "", "", 0, "");} }
#define FL_HALT(exp, f, l)				{ if (!(exp)) {DoHalt(#exp, "", "", 0, "");} }
#define HALTX(exp, m)					{ if (!(exp)) {DoHalt(#exp, m, "", 0, "");} }
#define FL_HALTX(exp, m, f, l)			{ if (!(exp)) {DoHalt(#exp, m, "", 0, "");} }
#define HALTX_GS(exp, si, m)			{ if (!(exp)) {DoHalt(#exp, si, m, "", 0, "");} }
#define WARN(exp)						{ if (!(exp)) {DoWarn(#exp, "", "", 0, "");} }
#define FL_WARN(exp, f, l)				{ if (!(exp)) {DoWarn(#exp, "", "", 0, "");} }
#define WARNX(exp, m)					{ if (!(exp)) {DoWarn(#exp, m, "", 0, "");} }
#define FL_WARNX(exp, m, f, l)			{ if (!(exp)) {DoWarn(#exp, m, "", 0, "");} }
#endif


#if ISVERSION(SERVER_VERSION)
#		undef HALT
#		undef HALTX
#		undef FL_HALT
#		undef FL_HALTX
#		undef HALTX_GS
#		define HALT(exp)						canthaltserver(exp)
#		define HALTX(exp,m)						canthaltserver(exp,m)
#		define FL_HALT(exp,f,l)					canthaltserver(exp)
#		define FL_HALTX(exp,m,f,l)				canthaltserver(exp,m)
#		define HALTX_GS(exp,si,m)				canthaltserver(exp,m)
#endif

// ------------------------------------------------------------------
// do these replace halt/warn?
#define UNIGNORABLE_ASSERTX(exp, m)								{ if (!(exp)) { DoUnignorableAssert(#exp,  m, __FILE__, __LINE__, __FUNCSIG__); } }
#define UNIGNORABLE_ASSERT(exp)									UNIGNORABLE_ASSERTX(exp, "")

// ------------------------------------------------------------------
// base asserts
#ifdef DEBUG_USE_ASSERTS
#	define FL_ASSERTX_ACTION(exp, m, f, l, action)				{ if (!(exp)) {if (DoAssert(#exp, m, f,	l, __FUNCSIG__)) { DEBUG_BREAK(); } {action;} } }
#	define FL_ASSERTX_DO(exp, m, f, l)							BOOL ASSERTDO_TAG = (exp); if (!ASSERTDO_TAG) { if (DoAssert(#exp, m, f, l, __FUNCSIG__)) { DEBUG_BREAK(); } }; if (!ASSERTDO_TAG)
#	define FL_ASSERTXONCE_ACTION(exp, m, f, l, action)			{ static BOOL ASSERTONCE_TAG = TRUE; if (!(exp)) { if (ASSERTONCE_TAG && DoAssert(#exp, m, f, l, __FUNCSIG__)) { DEBUG_BREAK(); } {ASSERTONCE_TAG = FALSE; action;} } }
#	define FL_ASSERTXONCE_DO(exp, m, f, l)						BOOL ASSERTDO_TAG = (exp); { static BOOL ASSERTONCE_TAG = TRUE; if (!ASSERTDO_TAG) { if (ASSERTONCE_TAG && DoAssert(#exp, m, f, l, __FUNCSIG__)) { DEBUG_BREAK(); } } }; if (!ASSERTDO_TAG)
#	define FL_ASSERTV_ACTION(exp, f, l, action, fmt, ...)		{ if (!(exp)) {if (DoAssertEx(#exp, f, l, __FUNCSIG__, fmt, __VA_ARGS__)) { DEBUG_BREAK(); } {action;} } }
#	define FL_ASSERTV_DO(exp, f, l, fmt, ...)					BOOL ASSERTDO_TAG = (exp); if (!ASSERTDO_TAG) { if (DoAssertEx(#exp, f, l, __FUNCSIG__, fmt, __VA_ARGS__)) { DEBUG_BREAK(); } }; if (!ASSERTDO_TAG)
#	define FL_ASSERTVONCE_ACTION(exp, f, l, action, fmt, ...)	{ static BOOL ASSERTONCE_TAG = TRUE; if (!(exp)) { if (ASSERTONCE_TAG && DoAssertEx(#exp, f, l, __FUNCSIG__, fmt, __VA_ARGS__)) { DEBUG_BREAK(); } {ASSERTONCE_TAG = FALSE; action;} } }
#	define FL_ASSERTVONCE_DO(exp, f, l, fmt, ...)				BOOL ASSERTDO_TAG = (exp); { static BOOL ASSERTONCE_TAG = TRUE; if (!ASSERTDO_TAG) { if (ASSERTONCE_TAG && DoAssertEx(#exp, f, l, __FUNCSIG__, fmt, __VA_ARGS__)) { DEBUG_BREAK(); } } }; if (!ASSERTDO_TAG)
#	define ASSERTN_ACTION(n, exp, action)						{ if (!(exp)) {if (DoNameAssert(#exp, n, __FUNCSIG__)) { DEBUG_BREAK(); } {action;} } }
#	define ASSERTN_DO(n, exp)									BOOL ASSERTDO_TAG = (exp); if (!ASSERTDO_TAG) { if (DoNameAssert(#exp, n, __FUNCSIG__)) { DEBUG_BREAK(); } }; if (!ASSERTDO_TAG)
#	define ASSERTNONCE_ACTION(n, exp, action)					{ static BOOL ASSERTONCE_TAG = TRUE; if (!(exp)) { if (ASSERTONCE_TAG && DoNameAssert(#exp, n, __FUNCSIG__)) { DEBUG_BREAK(); } {ASSERTONCE_TAG = FALSE; action;} } }
#	define ASSERTNONCE_DO(n, exp)								BOOL ASSERTDO_TAG = (exp); { static BOOL ASSERTONCE_TAG = TRUE; if (!ASSERTDO_TAG) { if (ASSERTONCE_TAG && DoNameAssert(#exp, n, __FUNCSIG__)) { DEBUG_BREAK(); } } }; if (!ASSERTDO_TAG)
#else
#	define FL_ASSERTX_ACTION(exp, m, f, l, action)				{ if (!(exp)) {action;} }
#	define FL_ASSERTX_DO(exp, m, f, l)							if (!(exp))
#	define FL_ASSERTXONCE_ACTION(exp, m, f, l, action)			{ if (!(exp)) {action;} }
#	define FL_ASSERTXONCE_DO(exp, m, f, l)						if (!(exp))
#	define FL_ASSERTV_ACTION(exp, f, l, action, fmt, ...)		{ if (!(exp)) {action;} }
#	define FL_ASSERTV_DO(exp, f, l, fmt, ...)					if (!(exp))
#	define FL_ASSERTVONCE_ACTION(exp, f, l, action, fmt, ...)	{ if (!(exp)) {action;} }
#	define FL_ASSERTVONCE_DO(exp, f, l, fmt, ...)				if (!(exp))
#	define ASSERTN_ACTION(n, exp, action)						{ if (!(exp)) {action;} }
#	define ASSERTN_DO(n, exp)									if (!(exp))
#	define ASSERTNONCE_ACTION(n, exp, action)					{ if (!(exp)) {action;} }
#	define ASSERTNONCE_DO(n, exp)								if (!(exp))
#endif

// ------------------------------------------------------------------
// no message assert variations
#define FL_ASSERT_ACTION(exp, f, l, action)						FL_ASSERTX_ACTION(exp, "", f, l, action)
#define FL_ASSERT_DO(exp, f, l)									FL_ASSERTX_DO(exp, "", f, l)
#define FL_ASSERTONCE_ACTION(exp, f, l, action)					FL_ASSERTXONCE_ACTION(exp, "", f, l, action)
#define FL_ASSERTONCE_DO(exp, f, l)								FL_ASSERTXONCE_DO(exp, "", f, l)

// ------------------------------------------------------------------
// no file/line assert variations
#define ASSERT_ACTION(exp, action)								FL_ASSERT_ACTION(exp, __FILE__, __LINE__, action)
#define ASSERT_DO(exp)											FL_ASSERT_DO(exp, __FILE__, __LINE__)
#define ASSERTONCE_ACTION(exp, action)							FL_ASSERTONCE_ACTION(exp, __FILE__, __LINE__, action)
#define ASSERTONCE_DO(exp)										FL_ASSERTONCE_DO(exp, __FILE__, __LINE__)
#define ASSERTX_ACTION(exp, m, action)							FL_ASSERTX_ACTION(exp, m, __FILE__, __LINE__, action)
#define ASSERTX_DO(exp, m)										FL_ASSERTX_DO(exp, m, __FILE__, __LINE__)
#define ASSERTXONCE_ACTION(exp, m, action)						FL_ASSERTXONCE_ACTION(exp, m, __FILE__, __LINE__, action)
#define ASSERTXONCE_DO(exp, m)									FL_ASSERTXONCE_DO(exp, m, __FILE__, __LINE__)
#define ASSERTV_ACTION(exp, action, fmt, ...)					FL_ASSERTV_ACTION(exp, __FILE__, __LINE__, action, fmt, __VA_ARGS__)
#define ASSERTV_DO(exp, fmt, ...)								FL_ASSERTV_DO(exp, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define ASSERTVONCE_ACTION(exp, action, fmt, ...)				FL_ASSERTVONCE_ACTION(exp, __FILE__, __LINE__, action, fmt, __VA_ARGS__)
#define ASSERTVONCE_DO(exp, fmt, ...)							FL_ASSERTVONCE_DO(exp, __FILE__, __LINE__, fmt, __VA_ARGS__)

// ------------------------------------------------------------------
// no action assert variations
#define ASSERT_NO_ACTION
#define ASSERT(exp)												ASSERT_ACTION(exp, ASSERT_NO_ACTION)
#define FL_ASSERT(exp, f, l)									FL_ASSERT_ACTION(exp, f, l,	ASSERT_NO_ACTION)
#define ASSERTONCE(exp)											ASSERTONCE_ACTION(exp,	ASSERT_NO_ACTION)
#define FL_ASSERTONCE(exp, f, l)								FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_NO_ACTION)
#define ASSERTX(exp, m)											ASSERTX_ACTION(exp, m, ASSERT_NO_ACTION)
#define FL_ASSERTX(exp, m, f, l)								FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_NO_ACTION)
#define ASSERTXONCE(exp, m)										ASSERTXONCE_ACTION(exp, m, ASSERT_NO_ACTION)
#define FL_ASSERTXONCE(exp, m, f, l)							FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_NO_ACTION)
#define ASSERTV(exp, fmt, ...)									ASSERTV_ACTION(exp, ASSERT_NO_ACTION, fmt, __VA_ARGS__)
#define FL_ASSERTV(exp, f, l, fmt, ...)							FL_ASSERTV_ACTION(exp, f, l, ASSERT_NO_ACTION, fmt, __VA_ARGS__)
#define ASSERTVONCE(exp, fmt, ...)								ASSERTVONCE_ACTION(exp, ASSERT_NO_ACTION, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE(exp, f, l, fmt, ...)						FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_NO_ACTION, fmt, __VA_ARGS__)
#define ASSERTN(n, exp)											ASSERTN_ACTION(n, exp, ASSERT_NO_ACTION)
#define ASSERTNONCE(n, exp)										ASSERTNONCE_ACTION(n, exp, ASSERT_NO_ACTION)

// ------------------------------------------------------------------
// return assert variations
#define ASSERT_ACTION_RETURN									return
#define ASSERT_RETURN(exp)										ASSERT_ACTION(exp, ASSERT_ACTION_RETURN)
#define IF_NOT_RETURN(exp)										if (!(exp)) { ASSERT_ACTION_RETURN; }
#define FL_ASSERT_RETURN(exp, f, l)								FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETURN)
#define ASSERTONCE_RETURN(exp)									ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETURN)
#define FL_ASSERTONCE_RETURN(exp, f, l)							FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETURN)
#define ASSERTX_RETURN(exp, m)									ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETURN)
#define FL_ASSERTX_RETURN(exp, m, f, l)							FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETURN)
#define ASSERTXONCE_RETURN(exp, m)								ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETURN)
#define FL_ASSERTXONCE_RETURN(exp, m, f, l)						FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETURN)
#define ASSERTV_RETURN(exp, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_RETURN, fmt, __VA_ARGS__)
#define FL_ASSERTV_RETURN(exp, f, l, fmt, ...)					FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETURN, fmt, __VA_ARGS__)
#define ASSERTVONCE_RETURN(exp, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETURN, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETURN(exp, f, l, fmt, ...)				FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETURN, fmt, __VA_ARGS__)
#define ASSERTN_RETURN(n, exp)									ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETURN)
#define ASSERTNONCE_RETURN(n, exp)								ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETURN)

// ------------------------------------------------------------------
// return false assert variations
#define ASSERT_ACTION_RETFALSE									return FALSE
#define ASSERT_RETFALSE(exp)									ASSERT_ACTION(exp, ASSERT_ACTION_RETFALSE)
#define IF_NOT_RETFALSE(exp)									if (!(exp)) { ASSERT_ACTION_RETFALSE; }
#define FL_ASSERT_RETFALSE(exp, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETFALSE)
#define ASSERTONCE_RETFALSE(exp)								ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETFALSE)
#define FL_ASSERTONCE_RETFALSE(exp, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETFALSE)
#define ASSERTX_RETFALSE(exp, m)								ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETFALSE)
#define FL_ASSERTX_RETFALSE(exp, m, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETFALSE)
#define ASSERTXONCE_RETFALSE(exp, m)							ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETFALSE)
#define FL_ASSERTXONCE_RETFALSE(exp, m, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETFALSE)
#define ASSERTV_RETFALSE(exp, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_RETFALSE, fmt, __VA_ARGS__)
#define FL_ASSERTV_RETFALSE(exp, f, l, fmt, ...)				FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETFALSE, fmt, __VA_ARGS__)
#define ASSERTVONCE_RETFALSE(exp, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETFALSE, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETFALSE(exp, f, l, fmt, ...)			FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETFALSE, fmt, __VA_ARGS__)
#define ASSERTN_RETFALSE(n, exp)								ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETFALSE)
#define ASSERTNONCE_RETFALSE(n, exp)							ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETFALSE)

// ------------------------------------------------------------------
// return true assert variations
#define ASSERT_ACTION_RETTRUE									return TRUE
#define ASSERT_RETTRUE(exp)										ASSERT_ACTION(exp, ASSERT_ACTION_RETTRUE)
#define IF_NOT_RETTRUE(exp)										if (!(exp)) { ASSERT_ACTION_RETTRUE; }
#define FL_ASSERT_RETTRUE(exp, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETTRUE)
#define ASSERTONCE_RETTRUE(exp)									ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETTRUE)
#define FL_ASSERTONCE_RETTRUE(exp, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETTRUE)
#define ASSERTX_RETTRUE(exp, m)									ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETTRUE)
#define FL_ASSERTX_RETTRUE(exp, m, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETTRUE)
#define ASSERTXONCE_RETTRUE(exp, m)								ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETTRUE)
#define FL_ASSERTXONCE_RETTRUE(exp, m, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETTRUE)
#define ASSERTV_RETTRUE(exp, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_RETTRUE, fmt, __VA_ARGS__)
#define FL_ASSERTV_RETTRUE(exp, f, l, fmt, ...)					FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETTRUE, fmt, __VA_ARGS__)
#define ASSERTVONCE_RETTRUE(exp, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETTRUE, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETTRUE(exp, f, l, fmt, ...)				FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETTRUE, fmt, __VA_ARGS__)
#define ASSERTN_RETTRUE(n, exp)									ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETTRUE)
#define ASSERTNONCE_RETTRUE(n, exp)								ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETTRUE)

// ------------------------------------------------------------------
// return invalid assert variations
#define ASSERT_ACTION_RETINVALID								return INVALID_ID
#define ASSERT_RETINVALID(exp)									ASSERT_ACTION(exp, ASSERT_ACTION_RETINVALID)
#define IF_NOT_RETINVALID(exp)									if (!(exp)) { ASSERT_ACTION_RETINVALID; }
#define FL_ASSERT_RETINVALID(exp, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETINVALID)
#define ASSERTONCE_RETINVALID(exp)								ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETINVALID)
#define FL_ASSERTONCE_RETINVALID(exp, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETINVALID)
#define ASSERTX_RETINVALID(exp, m)								ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETINVALID)
#define FL_ASSERTX_RETINVALID(exp, m, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETINVALID)
#define ASSERTXONCE_RETINVALID(exp, m)							ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETINVALID)
#define FL_ASSERTXONCE_RETINVALID(exp, m, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETINVALID)
#define ASSERTV_RETINVALID(exp, fmt, ...)						ASSERTV_ACTION(exp, ASSERT_ACTION_RETINVALID, fmt, __VA_ARGS__)
#define FL_ASSERTV_RETINVALID(exp, f, l, fmt, ...)				FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETINVALID, fmt, __VA_ARGS__)
#define ASSERTVONCE_RETINVALID(exp, fmt, ...)					ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETINVALID, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETINVALID(exp, f, l, fmt, ...)			FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETINVALID, fmt, __VA_ARGS__)
#define ASSERTN_RETINVALID(n, exp)								ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETINVALID)
#define ASSERTNONCE_RETINVALID(n, exp)							ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETINVALID)

// ------------------------------------------------------------------
// return null assert variations
#define ASSERT_ACTION_RETNULL									return NULL
#define ASSERT_RETNULL(exp)										ASSERT_ACTION(exp, ASSERT_ACTION_RETNULL)
#define IF_NOT_RETNULL(exp)										if (!(exp)) { ASSERT_ACTION_RETNULL; }
#define FL_ASSERT_RETNULL(exp, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETNULL)
#define ASSERTONCE_RETNULL(exp)									ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETNULL)
#define FL_ASSERTONCE_RETNULL(exp, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETNULL)
#define ASSERTX_RETNULL(exp, m)									ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETNULL)
#define FL_ASSERTX_RETNULL(exp, m, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETNULL)
#define ASSERTXONCE_RETNULL(exp, m)								ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETNULL)
#define FL_ASSERTXONCE_RETNULL(exp, m, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETNULL)
#define ASSERTV_RETNULL(exp, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_RETNULL, fmt, __VA_ARGS__)
#define FL_ASSERTV_RETNULL(exp, f, l, fmt, ...)					FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETNULL, fmt, __VA_ARGS__)
#define ASSERTVONCE_RETNULL(exp, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETNULL, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETNULL(exp, f, l, fmt, ...)				FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETNULL, fmt, __VA_ARGS__)
#define ASSERTN_RETNULL(n, exp)									ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETNULL)
#define ASSERTNONCE_RETNULL(n, exp)								ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETNULL)

// ------------------------------------------------------------------
// return 0 assert variations
#define ASSERT_ACTION_RETZERO									return 0
#define ASSERT_RETZERO(exp)										ASSERT_ACTION(exp, ASSERT_ACTION_RETZERO)
#define IF_NOT_RETZERO(exp)										if (!(exp)) { ASSERT_ACTION_RETZERO; }
#define FL_ASSERT_RETZERO(exp, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETZERO)
#define ASSERTONCE_RETZERO(exp)									ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETZERO)
#define FL_ASSERTONCE_RETZERO(exp, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETZERO)
#define ASSERTX_RETZERO(exp, m)									ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETZERO)
#define FL_ASSERTX_RETZERO(exp, m, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETZERO)
#define ASSERTXONCE_RETZERO(exp, m)								ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETZERO)
#define FL_ASSERTXONCE_RETZERO(exp, m, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETZERO)
#define ASSERTV_RETZERO(exp, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_RETZERO, fmt, __VA_ARGS__)
#define FL_ASSERTV_RETZERO(exp, f, l, fmt, ...)					FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETZERO, fmt, __VA_ARGS__)
#define ASSERTVONCE_RETZERO(exp, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETZERO, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETZERO(exp, f, l, fmt, ...)				FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETZERO, fmt, __VA_ARGS__)
#define ASSERTN_RETZERO(n, exp)									ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETZERO)
#define ASSERTNONCE_RETZERO(n, exp)								ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETZERO)

// ------------------------------------------------------------------
// return fail assert variations
#define ASSERT_ACTION_RETFAIL									return E_FAIL
#define ASSERT_RETFAIL(exp)										ASSERT_ACTION(exp, ASSERT_ACTION_RETFAIL)
#define IF_NOT_RETFAIL(exp)										if (!(exp)) { ASSERT_ACTION_RETFAIL; }
#define FL_ASSERT_RETFAIL(exp, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETFAIL)
#define ASSERTONCE_RETFAIL(exp)									ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETFAIL)
#define FL_ASSERTONCE_RETFAIL(exp, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETFAIL)
#define ASSERTX_RETFAIL(exp, m)									ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETFAIL)
#define FL_ASSERTX_RETFAIL(exp, m, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETFAIL)
#define ASSERTXONCE_RETFAIL(exp, m)								ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETFAIL)
#define FL_ASSERTXONCE_RETFAIL(exp, m, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETFAIL)
#define ASSERTV_RETFAIL(exp, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_RETFAIL, fmt, __VA_ARGS__)
#define FL_ASSERTV_RETFAIL(exp, f, l, fmt, ...)					FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETFAIL, fmt, __VA_ARGS__)
#define ASSERTVONCE_RETFAIL(exp, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETFAIL, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETFAIL(exp, f, l, fmt, ...)				FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETFAIL, fmt, __VA_ARGS__)
#define ASSERTN_RETFAIL(n, exp)									ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETFAIL)
#define ASSERTNONCE_RETFAIL(n, exp)								ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETFAIL)

// ------------------------------------------------------------------
// return invalid arg assert variations
#define ASSERT_ACTION_RETINVALIDARG								return E_INVALIDARG
#define ASSERT_RETINVALIDARG(exp)								ASSERT_ACTION(exp, ASSERT_ACTION_RETINVALIDARG)
#define IF_NOT_RETINVALIDARG(exp)								if (!(exp)) { ASSERT_ACTION_RETINVALIDARG; }
#define FL_ASSERT_RETINVALIDARG(exp, f, l)						FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETINVALIDARG)
#define ASSERTONCE_RETINVALIDARG(exp)							ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETINVALIDARG)
#define FL_ASSERTONCE_RETINVALIDARG(exp, f, l)					FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETINVALIDARG)
#define ASSERTX_RETINVALIDARG(exp, m)							ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETINVALIDARG)
#define FL_ASSERTX_RETINVALIDARG(exp, m, f, l)					FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETINVALIDARG)
#define ASSERTXONCE_RETINVALIDARG(exp, m)						ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETINVALIDARG)
#define FL_ASSERTXONCE_RETINVALIDARG(exp, m, f, l)				FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETINVALIDARG)
#define ASSERTV_RETINVALIDARG(exp, fmt, ...)					ASSERTV_ACTION(exp, ASSERT_ACTION_RETINVALIDARG, fmt, __VA_ARGS__)
#define FL_ASSERTV_RETINVALIDARG(exp, f, l, fmt, ...)			FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETINVALIDARG, fmt, __VA_ARGS__)
#define ASSERTVONCE_RETINVALIDARG(exp, fmt, ...)				ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETINVALIDARG, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETINVALIDARG(exp, f, l, fmt, ...)		FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETINVALIDARG, fmt, __VA_ARGS__)
#define ASSERTN_RETINVALIDARG(n, exp)							ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETINVALIDARG)
#define ASSERTNONCE_RETINVALIDARG(n, exp)						ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETINVALIDARG)

// ------------------------------------------------------------------
// return x assert variations
#define ASSERT_ACTION_RETX(x)									return (x)
#define ASSERT_RETX(exp, x)										ASSERT_ACTION(exp, ASSERT_ACTION_RETX(x))
#define IF_NOT_RETX(exp, x)										if (!(exp)) { ASSERT_ACTION_RETX(x); }
#define FL_ASSERT_RETX(exp, x, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETX(x))
#define ASSERTONCE_RETX(exp, x)									ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETX(x))
#define FL_ASSERTONCE_RETX(exp, x, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETX(x))
#define ASSERTX_RETX(exp, m, x)									ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETX(x))
#define FL_ASSERTX_RETX(exp, m, x, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETX(x))
#define ASSERTXONCE_RETX(exp, m, x)								ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETX(x))
#define FL_ASSERTXONCE_RETX(exp, m, x, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETX(x))
#define ASSERTV_RETX(exp, x, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_RETX(x), fmt, __VA_ARGS__)
#define FL_ASSERTV_RETX(exp, x, f, l, fmt, ...)					FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETX(x), fmt, __VA_ARGS__)
#define ASSERTVONCE_RETX(exp, x, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETX(x), fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETX(exp, x, f, l, fmt, ...)				FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETX(x), fmt, __VA_ARGS__)
#define ASSERTN_RETX(n, exp, x)									ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETX(x))
#define ASSERTNONCE_RETX(n, exp, x)								ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETX(x))

// ------------------------------------------------------------------
// return val assert variations (same as retx, except ordering in some situations)
#define ASSERT_ACTION_RETVAL(x)									return (x)
#define ASSERT_RETVAL(exp, x)									ASSERT_ACTION(exp, ASSERT_ACTION_RETX(x))
#define IF_NOT_RETVAL(exp, x)									if (!(exp)) { ASSERT_ACTION_RETX(x); }
#define FL_ASSERT_RETVAL(exp, x, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_RETX(x))
#define ASSERTONCE_RETVAL(exp, x)								ASSERTONCE_ACTION(exp,	ASSERT_ACTION_RETX(x))
#define FL_ASSERTONCE_RETVAL(exp, x, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_RETX(x))
#define ASSERTX_RETVAL(exp, x, m)								ASSERTX_ACTION(exp, m, ASSERT_ACTION_RETX(x))
#define FL_ASSERTX_RETVAL(exp, x, m, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_RETX(x))
#define ASSERTXONCE_RETVAL(exp, x, m)							ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_RETX(x))
#define FL_ASSERTXONCE_RETVAL(exp, x, m, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_RETX(x))
#define ASSERTV_RETVAL(exp, x, fmt, ...)						ASSERTV_ACTION(exp, ASSERT_ACTION_RETX(x), fmt, __VA_ARGS__)
#define FL_ASSERTV_RETVAL(exp, x, f, l, fmt, ...)				FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_RETX(x), fmt, __VA_ARGS__)
#define ASSERTVONCE_RETVAL(exp, x, fmt, ...)					ASSERTVONCE_ACTION(exp, ASSERT_ACTION_RETX(x), fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_RETVAL(exp, x, f, l, fmt, ...)			FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_RETX(x), fmt, __VA_ARGS__)
#define ASSERTN_RETVAL(n, exp, x)								ASSERTN_ACTION(n, exp, ASSERT_ACTION_RETX(x))
#define ASSERTNONCE_RETVAL(n, exp, x)							ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_RETX(x))

// ------------------------------------------------------------------
// continue assert variations
#define ASSERT_ACTION_CONTINUE									continue
#define ASSERT_CONTINUE(exp)									ASSERT_ACTION(exp, ASSERT_ACTION_CONTINUE)
#define IF_NOT_CONTINUE(exp)									if (!(exp)) { ASSERT_ACTION_CONTINUE; }
#define FL_ASSERT_CONTINUE(exp, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_CONTINUE)
#define ASSERTONCE_CONTINUE(exp)								ASSERTONCE_ACTION(exp,	ASSERT_ACTION_CONTINUE)
#define FL_ASSERTONCE_CONTINUE(exp, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_CONTINUE)
#define ASSERTX_CONTINUE(exp, m)								ASSERTX_ACTION(exp, m, ASSERT_ACTION_CONTINUE)
#define FL_ASSERTX_CONTINUE(exp, m, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_CONTINUE)
#define ASSERTXONCE_CONTINUE(exp, m)							ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_CONTINUE)
#define FL_ASSERTXONCE_CONTINUE(exp, m, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_CONTINUE)
#define ASSERTV_CONTINUE(exp, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_CONTINUE, fmt, __VA_ARGS__)
#define FL_ASSERTV_CONTINUE(exp, f, l, fmt, ...)				FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_CONTINUE, fmt, __VA_ARGS__)
#define ASSERTVONCE_CONTINUE(exp, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_CONTINUE, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_CONTINUE(exp, f, l, fmt, ...)			FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_CONTINUE, fmt, __VA_ARGS__)
#define ASSERTN_CONTINUE(n, exp)								ASSERTN_ACTION(n, exp, ASSERT_ACTION_CONTINUE)
#define ASSERTNONCE_CONTINUE(n, exp)							ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_CONTINUE)

// ------------------------------------------------------------------
// break assert variations
#define ASSERT_ACTION_BREAK										break
#define ASSERT_BREAK(exp)										ASSERT_ACTION(exp, ASSERT_ACTION_BREAK)
#define IF_NOT_BREAK(exp)										if (!(exp)) { ASSERT_ACTION_BREAK; }
#define FL_ASSERT_BREAK(exp, f, l)								FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_BREAK)
#define ASSERTONCE_BREAK(exp)									ASSERTONCE_ACTION(exp,	ASSERT_ACTION_BREAK)
#define FL_ASSERTONCE_BREAK(exp, f, l)							FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_BREAK)
#define ASSERTX_BREAK(exp, m)									ASSERTX_ACTION(exp, m, ASSERT_ACTION_BREAK)
#define FL_ASSERTX_BREAK(exp, m, f, l)							FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_BREAK)
#define ASSERTXONCE_BREAK(exp, m)								ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_BREAK)
#define FL_ASSERTXONCE_BREAK(exp, m, f, l)						FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_BREAK)
#define ASSERTV_BREAK(exp, fmt, ...)							ASSERTV_ACTION(exp, ASSERT_ACTION_BREAK, fmt, __VA_ARGS__)
#define FL_ASSERTV_BREAK(exp, f, l, fmt, ...)					FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_BREAK, fmt, __VA_ARGS__)
#define ASSERTVONCE_BREAK(exp, fmt, ...)						ASSERTVONCE_ACTION(exp, ASSERT_ACTION_BREAK, fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_BREAK(exp, f, l, fmt, ...)				FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_BREAK, fmt, __VA_ARGS__)
#define ASSERTN_BREAK(n, exp)									ASSERTN_ACTION(n, exp, ASSERT_ACTION_BREAK)
#define ASSERTNONCE_BREAK(n, exp)								ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_BREAK)

// ------------------------------------------------------------------
// goto assert variations
#define ASSERT_ACTION_GOTO(lbl)									goto lbl
#define ASSERT_GOTO(exp, lbl)									ASSERT_ACTION(exp, ASSERT_ACTION_GOTO(lbl))
#define IF_NOT_GOTO(exp, lbl)									if (!(exp)) { ASSERT_ACTION_GOTO(lbl); }
#define FL_ASSERT_GOTO(exp, lbl, f, l)							FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_GOTO(lbl))
#define ASSERTONCE_GOTO(exp, lbl)								ASSERTONCE_ACTION(exp,	ASSERT_ACTION_GOTO(lbl))
#define FL_ASSERTONCE_GOTO(exp, lbl, f, l)						FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_GOTO(lbl))
#define ASSERTX_GOTO(exp, m, lbl)								ASSERTX_ACTION(exp, m, ASSERT_ACTION_GOTO(lbl))
#define FL_ASSERTX_GOTO(exp, m, lbl, f, l)						FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_GOTO(lbl))
#define ASSERTXONCE_GOTO(exp, m, lbl)							ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_GOTO(lbl))
#define FL_ASSERTXONCE_GOTO(exp, m, lbl, f, l)					FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_GOTO(lbl))
#define ASSERTV_GOTO(exp, lbl, fmt, ...)						ASSERTV_ACTION(exp, ASSERT_ACTION_GOTO(lbl), fmt, __VA_ARGS__)
#define FL_ASSERTV_GOTO(exp, lbl, f, l, fmt, ...)				FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_GOTO(lbl), fmt, __VA_ARGS__)
#define ASSERTVONCE_GOTO(exp, lbl, fmt, ...)					ASSERTVONCE_ACTION(exp, ASSERT_ACTION_GOTO(lbl), fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_GOTO(exp, lbl, f, l, fmt, ...)			FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_GOTO(lbl), fmt, __VA_ARGS__)
#define ASSERTN_GOTO(n, exp, lbl)								ASSERTN_ACTION(n, exp, ASSERT_ACTION_GOTO(lbl))
#define ASSERTNONCE_GOTO(n, exp, lbl)							ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_GOTO(lbl))

// ------------------------------------------------------------------
// throw assert variations
#define ASSERT_ACTION_THROW(except)								throw except
#define ASSERT_THROW(exp, except)								ASSERT_ACTION(exp, ASSERT_ACTION_THROW(except))
#define IF_NOT_THROW(exp, except)								if (!(exp)) { ASSERT_ACTION_THROW(except); }
#define FL_ASSERT_THROW(exp, except, f, l)						FL_ASSERT_ACTION(exp, f, l,	ASSERT_ACTION_THROW(except))
#define ASSERTONCE_THROW(exp, except)							ASSERTONCE_ACTION(exp,	ASSERT_ACTION_THROW(except))
#define FL_ASSERTONCE_THROW(exp, except, f, l)					FL_ASSERTONCE_ACTION(exp, f, l, ASSERT_ACTION_THROW(except))
#define ASSERTX_THROW(exp, m, except)							ASSERTX_ACTION(exp, m, ASSERT_ACTION_THROW(except))
#define FL_ASSERTX_THROW(exp, m, except, f, l)					FL_ASSERTX_ACTION(exp, m, f, l,	ASSERT_ACTION_THROW(except))
#define ASSERTXONCE_THROW(exp, m, except)						ASSERTXONCE_ACTION(exp, m, ASSERT_ACTION_THROW(except))
#define FL_ASSERTXONCE_THROW(exp, m, except, f, l)				FL_ASSERTXONCE_ACTION(exp, m, f, l,	ASSERT_ACTION_THROW(except))
#define ASSERTV_THROW(exp, except, fmt, ...)					ASSERTV_ACTION(exp, ASSERT_ACTION_THROW(except), fmt, __VA_ARGS__)
#define FL_ASSERTV_THROW(exp, except, f, l, fmt, ...)			FL_ASSERTV_ACTION(exp, f, l, ASSERT_ACTION_THROW(except), fmt, __VA_ARGS__)
#define ASSERTVONCE_THROW(exp, except, fmt, ...)				ASSERTVONCE_ACTION(exp, ASSERT_ACTION_THROW(except), fmt, __VA_ARGS__)
#define FL_ASSERTVONCE_THROW(exp, except, f, l, fmt, ...)		FL_ASSERTVONCE_ACTION(exp, f, l, ASSERT_ACTION_THROW(except), fmt, __VA_ARGS__)
#define ASSERTN_THROW(n, exp, except)							ASSERTN_ACTION(n, exp, ASSERT_ACTION_THROW(except))
#define ASSERTNONCE_THROW(n, exp, except)						ASSERTNONCE_ACTION(n, exp, ASSERT_ACTION_THROW(except))


//Extra client-server debugs theoretically to log rogue clients.  No logging put in yet.  Generated by PERL.
//Don't bother maintaining these.
//Switching to logging units as it is more convenient to the code.  Client can be
//determined by guid lookup.  likely when we actually implement logging it will
//only log the guid.
#define UNITLOG_ASSERT(exp, unit)						{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); }} }
#define UNITLOG_ASSERTX(exp, m, unit)					{ if (!(exp)) {if (DoAssert(#exp, m, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); }} }
#define UNITLOG_HALT(exp, unit)						{ if (!(exp)) {DoHalt(#exp, "", __FILE__, __LINE__, __FUNCSIG__);} }
#define UNITLOG_HALTX(exp, m, unit)					{ if (!(exp)) {DoHalt(#exp, m, __FILE__, __LINE__, __FUNCSIG__);} }
#define UNITLOG_WARN(exp, unit)						{ if (!(exp)) {DoWarn(#exp, "", __FILE__, __LINE__, __FUNCSIG__);} }
#define UNITLOG_WARNX(exp, m, unit)					{ if (!(exp)) {DoWarn(#exp, m, __FILE__, __LINE__, __FUNCSIG__);} }
#define UNITLOG_ASSERTONCE(exp, unit)					{ static BOOL ASSERTONCE_TAG = TRUE; if (ASSERTONCE_TAG && !(exp)) {ASSERTONCE_TAG = FALSE; if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); }} }
#define UNITLOG_ASSERTXONCE(exp, x, unit)				{ static BOOL ASSERTONCE_TAG = TRUE; if (ASSERTONCE_TAG && !(exp)) {ASSERTONCE_TAG = FALSE; if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); }} }

#define UNITLOG_ASSERT_RETURN(exp, unit)				{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return;} }
#define UNITLOG_ASSERT_RETFALSE(exp, unit)			{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return FALSE;} }
#define UNITLOG_ASSERT_RETTRUE(exp, unit)				{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return TRUE;} }
#define UNITLOG_ASSERT_RETINVALID(exp, unit)			{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return INVALID_ID;} }
#define UNITLOG_ASSERT_RETNULL(exp	, unit)			{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return NULL;} }
#define UNITLOG_ASSERT_RETZERO(exp, unit)				{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return 0;} }
#define UNITLOG_ASSERT_RETX(exp, x, unit)				{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return x;} }
#define UNITLOG_ASSERT_CONTINUE(exp, unit)			{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } continue;} }
#define UNITLOG_ASSERT_BREAK(exp, unit)				{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } break;} }
#define UNITLOG_ASSERT_GOTO(exp, lbl, unit)			{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } goto lbl;} }
#define UNITLOG_ASSERT_RETFAIL(exp, unit)				{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return E_FAIL;} }
#define UNITLOG_ASSERT_RETVAL(exp, rv, unit)			{ if (!(exp)) {if (DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return rv;} }
#define UNITLOG_ASSERT_DO(exp, unit)					FL_ASSERT_DO(exp, __FILE__, __LINE__)
#define UNITLOG_ASSERTONCE_RETURN(exp, unit)			{ static BOOL ASSERTONCE_TAG = TRUE; if (!(exp)) { if (ASSERTONCE_TAG && DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { ASSERTONCE_TAG = FALSE; DEBUG_BREAK(); } return;} }
#define UNITLOG_ASSERTONCE_RETVAL(exp, val, unit)			{ static BOOL ASSERTONCE_TAG = TRUE; if (!(exp)) { if (ASSERTONCE_TAG && DoAssert(#exp, "", __FILE__, __LINE__, __FUNCSIG__)) { ASSERTONCE_TAG = FALSE; DEBUG_BREAK(); } return val;} }
#define UNITLOG_ASSERTXONCE_RETURN(exp, m, unit)			{ static BOOL ASSERTONCE_TAG = TRUE; if (!(exp)) { if (ASSERTONCE_TAG && DoAssert(#exp, m, __FILE__, __LINE__, __FUNCSIG__)) { ASSERTONCE_TAG = FALSE; DEBUG_BREAK(); } return;} }

#define UNITLOG_ASSERTX_RETURN(exp, x, unit)				{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return;} }
#define UNITLOG_ASSERTX_RETFALSE(exp, x, unit)			{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return FALSE;} }
#define UNITLOG_ASSERTX_RETTRUE(exp, x, unit)				{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return TRUE;} }
#define UNITLOG_ASSERTX_RETINVALID(exp, x, unit)			{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return INVALID_ID;} }
#define UNITLOG_ASSERTX_RETNULL(exp, x, unit)				{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return NULL;} }
#define UNITLOG_ASSERTX_RETZERO(exp, x, unit)				{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return 0;} }
#define UNITLOG_ASSERTX_RETX(exp, x, y, unit)				{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return y;} }
#define UNITLOG_ASSERTX_CONTINUE(exp, x, unit)			{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } continue;} }
#define UNITLOG_ASSERTX_BREAK(exp, x, unit)				{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } break;} }
#define UNITLOG_ASSERTX_RETFAIL(exp, x, unit)				{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return E_FAIL;} }
#define UNITLOG_ASSERTX_RETVAL(exp, rv, x, unit)			{ if (!(exp)) {if (DoAssert(#exp, x, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return rv;} }
#define UNITLOG_ASSERTX_DO(exp, x, unit)					FL_ASSERTX_DO(exp, x, __FILE__, __LINE__)

#define UNITLOG_ASSERTN(n, exp, unit)						{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); }} }
#define UNITLOG_ASSERTN_RETURN(n, exp, unit)				{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return;} }
#define UNITLOG_ASSERTN_RETFALSE(n, exp, unit)			{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return FALSE;} }
#define UNITLOG_ASSERTN_RETTRUE(n, exp, unit)				{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return TRUE;} }
#define UNITLOG_ASSERTN_RETINVALID(n, exp, unit)			{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return INVALID_ID;} }
#define UNITLOG_ASSERTN_RETNULL(n, exp, unit)				{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return NULL;} }
#define UNITLOG_ASSERTN_RETZERO(n, exp, unit)				{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return 0;} }
#define UNITLOG_ASSERTN_RETX(n, exp, x, unit)				{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return x;} }
#define UNITLOG_ASSERTN_CONTINUE(n, exp, unit)			{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } continue;} }
#define UNITLOG_ASSERTN_BREAK(n, exp, unit)				{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } break;} }
#define UNITLOG_ASSERTN_GOTO(n, exp, lbl, unit)			{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } goto lbl;} }
#define UNITLOG_ASSERTN_RETFAIL(n, exp, unit)				{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return E_FAIL;} }
#define UNITLOG_ASSERTN_RETVAL(n, exp, rv, unit)			{ if (!(exp)) {if (DoNameAssert(#exp, n, __FILE__, __LINE__, __FUNCSIG__)) { DEBUG_BREAK(); } return rv;} }
//don't bother maintaining the above.  generated by perl script.  -RJD


#if ISVERSION(SERVER_VERSION) && ISVERSION(RETAIL_VERSION)
#undef DEBUG_USE_TRACES // do NOT trace to the debugger in retail versions. -amol
#endif

#if DEBUG_USE_TRACES
#	define DEBUG_TRACE(str)				trace(str)
#	define RELEASE_TRACE()					
#else
#	define DEBUG_TRACE(str)				
#	define RELEASE_TRACE()				trace("file:%s  line:%d  function: %s\n", __FILE__, __LINE__, __FUNCSIG__)
#endif

#define UNDEFINED_CODE()				ASSERTV_MSG("Reached undefined code in function:\n\n", __FUNCSIG__);
#define UNDEFINED_CODE_X(str)			ASSERTV_MSG("Reached undefined code in function:\n\n%s\n\n%s", __FUNCSIG__, str);

#define STATUS_STRING(task)				L"  [ " L#task L" ]: %S"
#define LOADING_STRING(type)			STATUS_STRING(Loading type)

#if ISVERSION(DEBUG_VERSION)
#	define DBG_ASSERT(x)				ASSERT(x);
#	define DBG_ASSERTV(x, fmt, ...)		ASSERTV(x, fmt, __VA_ARGS__);
#	define DBG_ASSERT_RETNULL(x)		ASSERT_RETNULL(x);
#	define DBG_ASSERT_RETURN(x)			ASSERT_RETURN(x);
#	define DBG_ASSERT_RETINVALID(x)		ASSERT_RETINVALID(x);
#	define DBG_ASSERT_RETFALSE(x)		ASSERT_RETFALSE(x);
#	define DBG_ASSERT_GOTO(x, y)		ASSERT_GOTO(x, y);
#	define DBG_ASSERT_RETVAL(x, y)		ASSERT_RETVAL(x, y);
#	define DBG_ASSERTX_RETFALSE(x, y)	ASSERTX_RETFALSE(x, y);
#else
#	define DBG_ASSERT(x)
#	define DBG_ASSERTV(x, fmt, ...)		
#	define DBG_ASSERT_RETNULL(x)			
#	define DBG_ASSERT_RETURN(x)			
#	define DBG_ASSERT_RETINVALID(x)
#	define DBG_ASSERT_RETFALSE(x)
#	define DBG_ASSERT_GOTO(x, y)
#	define DBG_ASSERT_RETVAL(x, y)
#	define DBG_ASSERTX_RETFALSE(x, y)
#endif


#if ISVERSION(DEBUG_VERSION) && !ISVERSION(SERVER_VERSION)
#   define keytrace(fmt, ...)			DebugKeyTrace(fmt, __VA_ARGS__)
#	define keybreak()					if (IsKeyDown(KEYTRACE_CLASS::GetKey())) { DebugBreak(); }
#else
#   define keytrace(fmt, ...)			
#	define keybreak()
#endif


// STATIC_CHECK(expr, msg) - evaluates expr at compile-time. if 0, generates a compile error with msg. (from LOKI)
template<int> struct CompileTimeError;
template<> struct CompileTimeError<true> {};
#define STATIC_CHECK(expr, msg) \
	{ CompileTimeError<((expr) != 0)> ERROR_##msg; (void)ERROR_##msg; }


//-------------------------------------------------------------------------------------------------
// DEFINES
//-------------------------------------------------------------------------------------------------
#define WARNING_TYPE_BACKGROUND		MAKE_MASK(0)
#define WARNING_TYPE_STRINGS		MAKE_MASK(1)

#define DEFAULT_ERRORDLG_MB_TYPE	(MB_ABORTRETRYIGNORE | MB_DEFBUTTON4)
#define PERFORCE_ERRORDLG_MB_TYPE	(MB_YESNO | MB_DEFBUTTON2)


//-------------------------------------------------------------------------------------------------
// ENUMS
//-------------------------------------------------------------------------------------------------
enum OUTPUT_PRIORITY
{
	OP_DEBUG=0,		// Debug - debug info (always output) (room adds, messages, etc.)
	OP_LOW,			// Low - info (texture loads, model loads)
	OP_MID,			// Mid - important, special event info (texture conversions, granny file updates)
	OP_HIGH,		// High - data warnings (missing tangent/binormal, excel data errors)
	OP_ERROR,		// Error - errors (major warnings, assertions, IsComputerOnFire() returns infavorably)
	MUTE_DEBUG_OUTPUT,
	NUM_OUTPUT_PRIORITY_LEVELS = MUTE_DEBUG_OUTPUT
};


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef void (* PFN_DEBUG_STRING_CHAR)    ( OUTPUT_PRIORITY ePriority, const char  * format, va_list & args );
typedef void (* PFN_DEBUG_STRING_WCHAR)   ( OUTPUT_PRIORITY ePriority, const WCHAR * format, va_list & args );
typedef void (* PFN_CONSOLE_STRING_CHAR)  ( DWORD dwColor, const char  * str );
typedef void (* PFN_CONSOLE_STRING_WCHAR) ( DWORD dwColor, const WCHAR * str );


//----------------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------------
class KEYTRACE_CLASS
{
	static int snGlobalKey;
	int nOldKey;
public:
	KEYTRACE_CLASS( int vkKey )
	{
		nOldKey = snGlobalKey;
		snGlobalKey = vkKey;
	}
	~KEYTRACE_CLASS(void)
	{
		snGlobalKey = nOldKey;
	}
	static int GetKey() { return snGlobalKey; }
};
#define KEYTRACE_SETLOCALKEY( vkKey )				KEYTRACE_CLASS UIDEN(keytrace)(vkKey);


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
BOOL IsDebuggerAttached(
	void);


//-------------------------------------------------------------------------------------------------
// FUNCTIONS
//-------------------------------------------------------------------------------------------------
void DebugInit(
	LPSTR lpCmdLine);

void DebugFree(
	void);

#if DEBUG_USE_TRACES
void trace(
	const char * format,
	...);
#else
inline void trace(const char *, ...) {}
#endif

#if DEBUG_USE_TRACES
void vtrace(
	const char * format,
	va_list & args);
#else
inline void vtrace(const char *, va_list &) {}
#endif

#if DEBUG_USE_TRACES
void trace1(
	const char * string);
#else
inline void trace1(const char *) {}
#endif

int ErrorDialogEx(
	const char * format,
	...);

int ErrorDialogV(
	const char * format,
	va_list & args,
	UINT uType = DEFAULT_ERRORDLG_MB_TYPE);

int ErrorDialogCustom(
	UINT uType,
	const char * format,
	...);

#if DEBUG_USE_ASSERTS
int DoAssert(
	const char * expression,
	const char * message,
	const char * filename,
	int linenumber,
	const char * function);

int DoAssertEx(
	const char * szExpression,
	const char * szFile,
	int nLine,
	const char * szFunction,
	const char * szMessageFmt,
	... );

int DoAssertV(
	const char * szExpression,
	const char * szFile,
	int nLine,
	const char * szFunction,
	const char * szMessageFmt,
	va_list & args );

int DoNameAssert(
	const char * expression,
	const char * username,
	const char * filename,
	int linenumber,
	const char * function);

void DoUnignorableAssert(
	const char * szExpression,
	const char * szMessage,
	const char * szFile,
	int nLine,
	const char * szFunction );
#else
inline int DoAssert(
	const char * ,
	const char * ,
	const char * ,
	int ,
	const char *)
{
	return IDIGNORE;
}

inline int DoAssertEx(
	const char *,
	const char *,
	int,
	const char *,
	const char *,
	... )
{
	return IDIGNORE;
}

inline int DoAssertV(
	const char *,
	const char *,
	int,
	const char *,
	const char *,
	va_list &)
{
	return IDIGNORE;
}

inline int DoNameAssert(
	const char *,
	const char *,
	const char *,
	int,
	const char *)
{
	return IDIGNORE;
}

inline void DoUnignorableAssert(
	const char *,
	const char *,
	const char *,
	int,
	const char *)
{
	return;
}
#endif

void DoHalt(
	const char * expression,
	const char * message,
	const char * filename,
	int linenumber,
	const char * function);

void DoHalt(
	const char * expression,
	unsigned nStringIndex,
	const char * message,
	const char * filename,
	int linenumber,
	const char * function);

void DoWarn(
	const char * expression,
	const char * message,
	const char * filename,
	int linenumber,
	const char * function);

void ShowDataWarningV(
	const char * format,
	va_list & args,
	DWORD dwWarningType = 0);

BOOL ShowDataWarnings(
	DWORD dwWarningType = 0);

void ShowDataWarning(
	DWORD dwWarningType,
	const char * format, 
	...);

void ShowDataWarning(
	const char * format,
	...);

BOOL DataFileCheck(
	const TCHAR* filename,
	BOOL bNoDialog = FALSE );

void DebugString(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * format,
	...);

void DebugStringV(
	OUTPUT_PRIORITY ePriority,
	const WCHAR * format,
	va_list & args);

void DebugString(
	OUTPUT_PRIORITY ePriority,
	const char * format,
	...);

void DebugStringV(
	OUTPUT_PRIORITY ePriority,
	const char * format,
	va_list & args);

void ConsolePrintString(
	DWORD dwColor,
	const char * string);

void ConsolePrintString(
	DWORD dwColor,
	const WCHAR * string);

void SetDebugStringCallbackChar(
	PFN_DEBUG_STRING_CHAR pfnCallback);

void SetDebugStringCallbackWideChar(
	PFN_DEBUG_STRING_WCHAR pfnCallback);

void SetConsoleStringCallbackChar(
	PFN_CONSOLE_STRING_CHAR pfnCallback);

void SetConsoleStringCallbackWideChar(
	PFN_CONSOLE_STRING_WCHAR pfnCallback);

char * GetErrorString(
	DWORD error,
	char * str,
	int len);

#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
void DebugKeyTrace(
	const char * format,
	...);
#endif

#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
void DebugSetTimeLimiter(
	void);
#endif

#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
void DebugClearTimeLimiter(
	void);
#endif


void DebugKeyHandlerStart( int nInitValue = 0 );
void DebugKeyHandlerEnd();
void DebugKeyUp();
void DebugKeyDown();
int  DebugKeyGetValue();
void DebugKeySetValue( int nValue );
BOOL DebugKeysAreActive( int );


#endif // _DEBUG_H_
