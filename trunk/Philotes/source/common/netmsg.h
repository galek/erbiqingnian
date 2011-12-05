// ---------------------------------------------------------------------------
// FILE:	netmsg.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
// ---------------------------------------------------------------------------
#ifdef	_NETMSG_H_
#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _NETMSG_H_


// ---------------------------------------------------------------------------
// DISABLE WARNINGS (COMPILER SPECIFIC)
// ---------------------------------------------------------------------------
#pragma warning(disable : 4100)				// disable warning for unreferenced parameters


// ---------------------------------------------------------------------------
// INCLUDE
// ---------------------------------------------------------------------------
#ifndef _PSTRING_H_
#include "pstring.h"
#endif

// ---------------------------------------------------------------------------
// DEBUG DEFINES
// ---------------------------------------------------------------------------
#define NET_MSG_COUNTER				0		// message debugging (extra data added to msgs)


// ---------------------------------------------------------------------------
// DEFINES
// ---------------------------------------------------------------------------

#define SMALL_MSG_BUF_SIZE				2048	// size of internal message buffer
#define LARGE_MSG_BUF_SIZE				(4*1024)
	
#define MSG_BUF_HDR_SIZE				OFFSET(MSG_BUF, msg)			// size of mail msg header
#define MUX_HEADER_SIZE					12	// hack by phu because this is a value from MUX stuff

// NOTE: that MAX_SMALL_MSG_SIZE probably should have MUX_HEADER_SIZE subtracted, but it's too late to change it!
#define MAX_SMALL_MSG_SIZE				(SMALL_MSG_BUF_SIZE - MSG_BUF_HDR_SIZE)		// NOT the actual max size of messages (this includes MSG_HEADER)
#define MAX_LARGE_MSG_SIZE				(LARGE_MSG_BUF_SIZE - MSG_BUF_HDR_SIZE - MUX_HEADER_SIZE)    // actual max size of messages

#ifndef RECORD_GAMETICK
#if !NET_MSG_COUNTER
#define MSG_HDR_SIZE				(sizeof(NET_CMD) + sizeof(MSG_SIZE))
#else
#define MSG_HDR_SIZE				(sizeof(NET_CMD) + sizeof(MSG_SIZE) + sizeof(DWORD))
#endif
#else // RECORD_GAMETICK
#if !NET_MSG_COUNTER
#define MSG_HDR_SIZE				(sizeof(NET_CMD) + sizeof(MSG_SIZE) + sizeof(DWORD))
#else
#define MSG_HDR_SIZE				(sizeof(NET_CMD) + sizeof(MSG_SIZE) + 2*sizeof(DWORD))
#endif
#endif // RECORD_GAMETICK


#define MAX_SMALL_MSG_DATA_SIZE			(MAX_SMALL_MSG_SIZE - MSG_HDR_SIZE)	// NOT the maximum size of message data (not including MSG_HEADER)
#define MAX_LARGE_MSG_DATA_SIZE			(MAX_LARGE_MSG_SIZE - MSG_HDR_SIZE)	// maximum size of message data (not including MSG_HEADER)
#define MAX_SMALL_MSG_STRUCT_SIZE		(SMALL_MSG_BUF_SIZE * 8)				// NOT the maximum expanded small MSG_STRUCT size (after NetTranslateMessageForRecv)
#define MAX_LARGE_MSG_STRUCT_SIZE		( 68<<10)				// The maximum expanded MSG_STRUCT size (after NetTranslateMessageForRecv)

#define	MAX_MSG_STRUCTS				256								// max msg structs


// ---------------------------------------------------------------------------
// TYPEDEFS
// ---------------------------------------------------------------------------
typedef BYTE						NET_CMD;						// message code
typedef WORD						MSG_SIZE;						// message size
typedef void (*FP_NET_IMM_CALLBACK)(struct NETCOM *, NETCLIENTID, BYTE * data, unsigned int size);


// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
struct NET_COMMAND_TABLE;


// ---------------------------------------------------------------------------
// common message header
// ---------------------------------------------------------------------------
struct MSG_HEADER
{
	NET_CMD							cmd;							// message id
	MSG_SIZE						size;							// size of message
#ifdef RECORD_GAMETICK
    DWORD                           game_tick;
#endif
#if NET_MSG_COUNTER
	DWORD							msg_counter;					// msg_counter used for debugging

#endif

	friend BYTE_BUF_NET & operator <<(
		BYTE_BUF_NET & bbuf, 
		MSG_HEADER & hdr) 
	{ 
		bbuf << hdr.cmd << hdr.size;
#if NET_MSG_COUNTER
		bbuf << hdr.msg_counter;
#endif
#ifdef RECORD_GAMETICK
		bbuf << hdr.game_tick;
#endif
		return bbuf;
	}

	BOOL Push(
		BYTE_BUF_NET & bbuf) 
	{
		ASSERT_RETFALSE(bbuf.Push(cmd));
		ASSERT_RETFALSE(bbuf.Push(size));
#if NET_MSG_COUNTER
		ASSERT_RETFALSE(bbuf.Push(msg_counter));
#endif

#ifdef RECORD_GAMETICK
		ASSERT_RETFALSE(bbuf.Push(game_tick));
#endif
		return TRUE;
	}

	friend BYTE_BUF_NET & operator >>(
		BYTE_BUF_NET & bbuf, 
		MSG_HEADER & hdr) 
	{ 
		bbuf >> hdr.cmd;
		bbuf >> hdr.size;
#if NET_MSG_COUNTER
		bbuf >> hdr.msg_counter;
#endif
#ifdef RECORD_GAMETICK
		bbuf >> hdr.game_tick;
#endif
		return bbuf;
	}

	BOOL Pop(
		BYTE_BUF_NET & bbuf) 
	{
		ASSERT_RETFALSE(bbuf.Pop(cmd));
		ASSERT_RETFALSE(bbuf.Pop(size));
#if NET_MSG_COUNTER
		ASSERT_RETFALSE(bbuf.Pop(msg_counter));
#endif
#ifdef RECORD_GAMETICK
		ASSERT_RETFALSE(bbuf.Pop(game_tick));
#endif
		return TRUE;
	}

	static const unsigned int GetSizeOffset(
		void)
	{
		return sizeof(NET_CMD);
	}

	void init(
		void)
	{
		cmd = (NET_CMD)0;
		size = 0;
#if NET_MSG_COUNTER
		msg_counter = 0;
#endif
#ifdef RECORD_GAMETICK
		game_tick = 0;
#endif
	}
};


// ---------------------------------------------------------------------------
// base message struct class
// ---------------------------------------------------------------------------
struct BASE_MSG_STRUCT
{
	void zz_register_field0(NET_COMMAND_TABLE * cmd_tbl) const {} 
	void zz_register_field1(NET_COMMAND_TABLE * cmd_tbl) const {} 
	void zz_register_field2(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field3(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field4(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field5(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field6(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field7(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field8(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field9(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field10(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field11(NET_COMMAND_TABLE * cmd_tbl) const {}
	void zz_register_field12(NET_COMMAND_TABLE * cmd_tbl) const {}	
	void zz_register_field13(NET_COMMAND_TABLE * cmd_tbl) const {}	
	void zz_register_field14(NET_COMMAND_TABLE * cmd_tbl) const {}	
	void zz_register_field15(NET_COMMAND_TABLE * cmd_tbl) const {}	
	void zz_register_field16(NET_COMMAND_TABLE * cmd_tbl) const {}	
	void zz_register_field17(NET_COMMAND_TABLE * cmd_tbl) const {}	
	void zz_register_field18(NET_COMMAND_TABLE * cmd_tbl) const {}	
	void zz_register_field19(NET_COMMAND_TABLE * cmd_tbl) const {}	
	void zz_register_field20(NET_COMMAND_TABLE * cmd_tbl) const {}		
	void zz_init_field0(void) {} 
	void zz_init_field1(void) {} 
	void zz_init_field2(void) {}
	void zz_init_field3(void) {}
	void zz_init_field4(void) {}
	void zz_init_field5(void) {}
	void zz_init_field6(void) {}
	void zz_init_field7(void) {}
	void zz_init_field8(void) {}
	void zz_init_field9(void) {}
	void zz_init_field10(void) {}
	void zz_init_field11(void) {}
	void zz_init_field12(void) {}
	void zz_init_field13(void) {}
	void zz_init_field14(void) {}
	void zz_init_field15(void) {}
	void zz_init_field16(void) {}
	void zz_init_field17(void) {}
	void zz_init_field18(void) {}
	void zz_init_field19(void) {}
	void zz_init_field20(void) {}
	PStrI & zz_str_field0(PStrI & str) { return str; }
	PStrI & zz_str_field1(PStrI & str) { return str; }
	PStrI & zz_str_field2(PStrI & str) { return str; }
	PStrI & zz_str_field3(PStrI & str) { return str; }
	PStrI & zz_str_field4(PStrI & str) { return str; }
	PStrI & zz_str_field5(PStrI & str) { return str; }
	PStrI & zz_str_field6(PStrI & str) { return str; }
	PStrI & zz_str_field7(PStrI & str) { return str; }
	PStrI & zz_str_field8(PStrI & str) { return str; }
	PStrI & zz_str_field9(PStrI & str) { return str; }
	PStrI & zz_str_field10(PStrI & str) { return str; }
	PStrI & zz_str_field11(PStrI & str) { return str; }
	PStrI & zz_str_field12(PStrI & str) { return str; }
	PStrI & zz_str_field13(PStrI & str) { return str; }
	PStrI & zz_str_field14(PStrI & str) { return str; }
	PStrI & zz_str_field15(PStrI & str) { return str; }
	PStrI & zz_str_field16(PStrI & str) { return str; }
	PStrI & zz_str_field17(PStrI & str) { return str; }
	PStrI & zz_str_field18(PStrI & str) { return str; }
	PStrI & zz_str_field19(PStrI & str) { return str; }
	PStrI & zz_str_field20(PStrI & str) { return str; }
	BYTE_BUF_NET & zz_push_field0(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field1(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field2(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field3(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field4(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field5(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field6(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field7(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field8(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field9(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field10(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field11(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BYTE_BUF_NET & zz_push_field12(BYTE_BUF_NET & bbuf) { return bbuf; } 
    BYTE_BUF_NET & zz_push_field13(BYTE_BUF_NET & bbuf) { return bbuf; } 
    BYTE_BUF_NET & zz_push_field14(BYTE_BUF_NET & bbuf) { return bbuf; } 
    BYTE_BUF_NET & zz_push_field15(BYTE_BUF_NET & bbuf) { return bbuf; } 
    BYTE_BUF_NET & zz_push_field16(BYTE_BUF_NET & bbuf) { return bbuf; } 
    BYTE_BUF_NET & zz_push_field17(BYTE_BUF_NET & bbuf) { return bbuf; } 
    BYTE_BUF_NET & zz_push_field18(BYTE_BUF_NET & bbuf) { return bbuf; } 
    BYTE_BUF_NET & zz_push_field19(BYTE_BUF_NET & bbuf) { return bbuf; } 
    BYTE_BUF_NET & zz_push_field20(BYTE_BUF_NET & bbuf) { return bbuf; } 
	BOOL zz_pop_field0(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field1(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field2(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field3(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field4(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field5(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field6(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field7(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field8(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field9(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field10(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field11(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field12(BYTE_BUF_NET & bbuf) { return TRUE; } 
	BOOL zz_pop_field13(BYTE_BUF_NET & bbuf) { return TRUE; } 
    BOOL zz_pop_field14(BYTE_BUF_NET & bbuf) { return TRUE; } 
    BOOL zz_pop_field15(BYTE_BUF_NET & bbuf) { return TRUE; } 
    BOOL zz_pop_field16(BYTE_BUF_NET & bbuf) { return TRUE; } 
    BOOL zz_pop_field17(BYTE_BUF_NET & bbuf) { return TRUE; } 
    BOOL zz_pop_field18(BYTE_BUF_NET & bbuf) { return TRUE; } 
    BOOL zz_pop_field19(BYTE_BUF_NET & bbuf) { return TRUE; } 
    BOOL zz_pop_field20(BYTE_BUF_NET & bbuf) { return TRUE; } 
};


// ---------------------------------------------------------------------------
// base message class
// ---------------------------------------------------------------------------
struct MSG_STRUCT : BASE_MSG_STRUCT
{
	MSG_HEADER						hdr;
};


// ---------------------------------------------------------------------------
// TYPEDEF
// ---------------------------------------------------------------------------
typedef PStrI & (BASE_MSG_STRUCT::*MFP_PRINT)(PStrI & str);
typedef __checkReturn BOOL (BASE_MSG_STRUCT::*MFP_PUSHPOP)(BYTE_BUF_NET & bbuf);


// ---------------------------------------------------------------------------
// MESSAGE DECLARATION MACROS
// ---------------------------------------------------------------------------
#define DEF_MSG_NAME0(x, y)					x##y
#define DEF_MSG_NAME(x, y)					DEF_MSG_NAME0(x, y)
#define DEF_MSG_BLOBLEN(n)					_##n##_len

#define DEF_MSG_CONS_DECL(m)				struct MSG_STRUCT_DECL * zz_register_msg_struct(NET_COMMAND_TABLE * cmd_tbl) { BOOL bAlreadyDefined; \
												struct MSG_STRUCT_DECL * s = zz_register_msg_name(cmd_tbl, bAlreadyDefined); \
												if (!bAlreadyDefined) { \
												zz_register_field0(cmd_tbl);\
												zz_register_field1(cmd_tbl);\
												zz_register_field2(cmd_tbl);\
												zz_register_field3(cmd_tbl);\
												zz_register_field4(cmd_tbl);\
												zz_register_field5(cmd_tbl);\
												zz_register_field6(cmd_tbl);\
												zz_register_field7(cmd_tbl);\
												zz_register_field8(cmd_tbl);\
												zz_register_field9(cmd_tbl);\
												zz_register_field10(cmd_tbl);\
												zz_register_field11(cmd_tbl);\
												zz_register_field12(cmd_tbl);\
												zz_register_field13(cmd_tbl);\
												zz_register_field14(cmd_tbl);\
												zz_register_field15(cmd_tbl);\
												zz_register_field16(cmd_tbl);\
												zz_register_field17(cmd_tbl);\
												zz_register_field18(cmd_tbl);\
												zz_register_field19(cmd_tbl);\
												zz_register_field20(cmd_tbl);\
												NetRegisterMsgStructEnd(cmd_tbl); } return s; }
#define DEF_MSG_CONS_INIT(m)				void zz_init_msg_struct() { \
												zz_init_field0();\
												zz_init_field1();\
												zz_init_field2();\
												zz_init_field3();\
												zz_init_field4();\
												zz_init_field5();\
												zz_init_field6();\
												zz_init_field7();\
												zz_init_field8();\
												zz_init_field9();\
												zz_init_field10();\
												zz_init_field11();\
												zz_init_field12();\
												zz_init_field13();\
												zz_init_field14();\
												zz_init_field15();\
												zz_init_field16();\
												zz_init_field17();\
												zz_init_field18();\
												zz_init_field19();\
												zz_init_field20();}\
											m() { zz_init_msg_struct(); }

#define DEF_MSG_CONS_OP_PUSH(m)				this->zz_push_field0(bbuf);\
												this->zz_push_field1(bbuf);\
												this->zz_push_field2(bbuf);\
												this->zz_push_field3(bbuf);\
												this->zz_push_field4(bbuf);\
												this->zz_push_field5(bbuf);\
												this->zz_push_field6(bbuf);\
												this->zz_push_field7(bbuf);\
												this->zz_push_field8(bbuf);\
												this->zz_push_field9(bbuf);\
												this->zz_push_field10(bbuf);\
												this->zz_push_field11(bbuf);\
												this->zz_push_field12(bbuf);\
												this->zz_push_field13(bbuf);\
												this->zz_push_field14(bbuf);\
												this->zz_push_field15(bbuf);\
												this->zz_push_field16(bbuf);\
												this->zz_push_field17(bbuf);\
												this->zz_push_field18(bbuf);\
												this->zz_push_field19(bbuf);\
												this->zz_push_field20(bbuf);
#define DEF_MSG_CONS_OP_POP(m)				bSuccess &= this->zz_pop_field0(bbuf);\
                                            bSuccess &= this->zz_pop_field1(bbuf);\
                                            bSuccess &= this->zz_pop_field2(bbuf);\
                                            bSuccess &= this->zz_pop_field3(bbuf);\
											bSuccess &= this->zz_pop_field4(bbuf);\
                                            bSuccess &= this->zz_pop_field5(bbuf);\
                                            bSuccess &= this->zz_pop_field6(bbuf);\
                                            bSuccess &= this->zz_pop_field7(bbuf);\
											bSuccess &= this->zz_pop_field8(bbuf);\
                                            bSuccess &= this->zz_pop_field9(bbuf);\
                                            bSuccess &= this->zz_pop_field10(bbuf);\
                                            bSuccess &= this->zz_pop_field11(bbuf);\
											bSuccess &= this->zz_pop_field12(bbuf);\
											bSuccess &= this->zz_pop_field13(bbuf);\
											bSuccess &= this->zz_pop_field14(bbuf);\
											bSuccess &= this->zz_pop_field15(bbuf);\
											bSuccess &= this->zz_pop_field16(bbuf);\
											bSuccess &= this->zz_pop_field17(bbuf);\
											bSuccess &= this->zz_pop_field18(bbuf);\
											bSuccess &= this->zz_pop_field19(bbuf);\
											bSuccess &= this->zz_pop_field20(bbuf);
#if !ISVERSION(RETAIL_VERSION)
#define DEF_MSG_CONS_OP_STR(m)				str << "[" << #m; \
												this->zz_str_field0(str);\
												this->zz_str_field1(str);\
												this->zz_str_field2(str);\
												this->zz_str_field3(str);\
												this->zz_str_field4(str);\
												this->zz_str_field5(str);\
												this->zz_str_field6(str);\
												this->zz_str_field7(str);\
												this->zz_str_field8(str);\
												this->zz_str_field9(str);\
												this->zz_str_field10(str);\
												this->zz_str_field11(str);\
												this->zz_str_field12(str);\
												this->zz_str_field13(str);\
												this->zz_str_field14(str);\
												this->zz_str_field15(str);\
												this->zz_str_field16(str);\
												this->zz_str_field17(str);\
												this->zz_str_field18(str);\
												this->zz_str_field19(str);\
												this->zz_str_field20(str);\
												str << "]"; return str;
#else
#define DEF_MSG_CONS_OP_STR(m)				return str;
#endif
												
												

#define DEF_MSG_CONS_FUNC(m)				PStrI & Print(PStrI & str) { DEF_MSG_CONS_OP_STR(m) } \
											BOOL Push(BYTE_BUF_NET & bbuf) { DEF_MSG_CONS_OP_PUSH(m); return TRUE; } \
											__checkReturn BOOL Pop(BYTE_BUF_NET & bbuf) {\
                                                        BOOL bSuccess = TRUE;\
                                                        DEF_MSG_CONS_OP_POP(m);\
                                                        return bSuccess;\
                                            }

#define DEF_MSG_CONS_NOINIT(m)				DEF_MSG_CONS_DECL(m) \
											DEF_MSG_CONS_FUNC(m)

#define DEF_MSG_CONS(m)						DEF_MSG_CONS_DECL(m) \
											DEF_MSG_CONS_INIT(m) \
											DEF_MSG_CONS_FUNC(m)

#if !ISVERSION(RETAIL_VERSION)
#define DEF_MSG_GETNAME(m)					const char ** zz_getname(void) { static const char * zz_myname = #m; return &zz_myname; }
#else
#define DEF_MSG_GETNAME(m)					const char ** zz_getname(void) { static const char * zz_myname = ""; return &zz_myname; }
#endif

#define DEF_MSG_SUBSTRUCT_INHERIT_STREAM(m, i) \
											struct m : i, BASE_MSG_STRUCT { DEF_MSG_GETNAME(m);  struct MSG_STRUCT_DECL * zz_register_msg_name(NET_COMMAND_TABLE * cmd_tbl, BOOL & bAlreadyDefined) { return NetRegisterMsgStructStart(cmd_tbl, zz_getname(), FALSE, bAlreadyDefined, TRUE); }  DEF_MSG_CONS_DECL(m)		PStrI & Print(PStrI & str) { DEF_MSG_CONS_OP_STR(m) }
#define DEF_MSG_SUBSTRUCT_INHERIT(m, i) \
											struct m : i, BASE_MSG_STRUCT { DEF_MSG_GETNAME(m);  struct MSG_STRUCT_DECL * zz_register_msg_name(NET_COMMAND_TABLE * cmd_tbl, BOOL & bAlreadyDefined) { return NetRegisterMsgStructStart(cmd_tbl, zz_getname(), FALSE, bAlreadyDefined); }  DEF_MSG_CONS_NOINIT(m)
#define DEF_MSG_SUBSTRUCT(m)				struct m : BASE_MSG_STRUCT { DEF_MSG_GETNAME(m);  struct MSG_STRUCT_DECL * zz_register_msg_name(NET_COMMAND_TABLE * cmd_tbl, BOOL & bAlreadyDefined) { return NetRegisterMsgStructStart(cmd_tbl, zz_getname(), FALSE, bAlreadyDefined); }  DEF_MSG_CONS(m)
#define DEF_MSG_STRUCT(m)					struct m : MSG_STRUCT { DEF_MSG_GETNAME(m);  struct MSG_STRUCT_DECL * zz_register_msg_name(NET_COMMAND_TABLE * cmd_tbl, BOOL & bAlreadyDefined) { return NetRegisterMsgStructStart(cmd_tbl, zz_getname(), TRUE, bAlreadyDefined); }  DEF_MSG_CONS(m)


#define FLD_OFFS(f)							(unsigned int)((BYTE*)&this->f - (BYTE*)this)
#define FUNC_NAME(x, y)						x##y

#if !ISVERSION(RETAIL_VERSION)
#define FLD_REG(n, name, typestr)			void FUNC_NAME(zz_register_field, n)(NET_COMMAND_TABLE * cmd_tbl) { NetRegisterMsgField(cmd_tbl, n, #name, typestr, NULL,
#define STRUCT_REG(n, name, type)			void FUNC_NAME(zz_register_field, n)(NET_COMMAND_TABLE * cmd_tbl) { type elem; elem.zz_register_msg_struct(cmd_tbl); NetRegisterMsgField(cmd_tbl, n, #name, NULL, elem.zz_getname(), 
#else
#define FLD_REG(n, name, typestr)			void FUNC_NAME(zz_register_field, n)(NET_COMMAND_TABLE * cmd_tbl) { NetRegisterMsgField(cmd_tbl, n, typestr, NULL, 
#define STRUCT_REG(n, name, type)			void FUNC_NAME(zz_register_field, n)(NET_COMMAND_TABLE * cmd_tbl) { type elem; elem.zz_register_msg_struct(cmd_tbl); NetRegisterMsgField(cmd_tbl, n, NULL, elem.zz_getname(), 
#endif

#define FLD_INIT_BASIC(n, name, def)		void FUNC_NAME(zz_init_field, n)(void) { name = def; }
#define FLD_INIT_STR(n, type, name);		void FUNC_NAME(zz_init_field, n)(void) { name[0] = 0; }
#define FLD_INIT_BLOB(n, name)				void FUNC_NAME(zz_init_field, n)(void) { DEF_MSG_BLOBLEN(name) = 0; }

#if ISVERSION(RETAIL)
#define FLD_STRING_BASIC(n, name)
#define FLD_STRING_STRUCT(n, name)
#define FLD_STRING_ARRAY(n, name, count)	
#define FLD_STRING_STARR(n, name, count)	
#define FLD_STRING_STRING(n, name, count)	
#define FLD_STRING_BLOB(n, name)			
#else
#define FLD_STRING_BASIC(n, name)			PStrI & FUNC_NAME(zz_str_field, n)(PStrI & str_) { return str_ << "  " #name ":" << name; }
#define FLD_STRING_STRUCT(n, name)			PStrI & FUNC_NAME(zz_str_field, n)(PStrI & str_) { str_ << "  " #name ":"; name.Print(str_); return str_; }
#define FLD_STRING_ARRAY(n, name, count)	PStrI & FUNC_NAME(zz_str_field, n)(PStrI & str_) { str_ << "  " #name "[" #count "]:" << name[0]; for (unsigned int ii = 1; ii < count; ++ii) { str_ << " " << name[ii]; } return str_; }
#define FLD_STRING_STARR(n, name, count)	PStrI & FUNC_NAME(zz_str_field, n)(PStrI & str_) { str_ << "  " #name "[" #count "]:"; name[0].Print(str_); for (unsigned int ii = 1; ii < count; ++ii) { str_ << " "; name[ii].Print(str_); } return str_; }
#define FLD_STRING_STRING(n, name, count)	PStrI & FUNC_NAME(zz_str_field, n)(PStrI & str_) { name[count-1] = 0;  return str_ << "  " #name ":\"" << name << "\""; }
#define FLD_STRING_BLOB(n, name)			PStrI & FUNC_NAME(zz_str_field, n)(PStrI & str_) { str_ << "  " #name "[" << DEF_MSG_BLOBLEN(name) << "]:"; for (unsigned int ii = 0; ii < DEF_MSG_BLOBLEN(name); ++ii) { str_.printf("%02x", name[ii]); } return str_; }
#endif

#define FLD_PUSH_BASIC(n, type, name)		BYTE_BUF_NET & FUNC_NAME(zz_push_field, n)(BYTE_BUF_NET & bbuf) { return bbuf << name; }
#define FLD_PUSH_STRUCT(n, type, name)		BYTE_BUF_NET & FUNC_NAME(zz_push_field, n)(BYTE_BUF_NET & bbuf) { name.Push(bbuf); return bbuf; }
#define FLD_PUSH_ARRAY(n, name, count)		BYTE_BUF_NET & FUNC_NAME(zz_push_field, n)(BYTE_BUF_NET & bbuf) { for (unsigned int ii = 0; ii < count; ++ii) { bbuf << name[ii]; } return bbuf; }
#define FLD_PUSH_STARR(n, name, count)		BYTE_BUF_NET & FUNC_NAME(zz_push_field, n)(BYTE_BUF_NET & bbuf) { for (unsigned int ii = 0; ii < count; ++ii) { name[ii].Push(bbuf); } return bbuf; }
#define FLD_PUSH_STRING(n, name, count)		BYTE_BUF_NET & FUNC_NAME(zz_push_field, n)(BYTE_BUF_NET & bbuf) { bbuf.Push(name, count); return bbuf; }
#define FLD_PUSH_BLOB(n, name, size)		BYTE_BUF_NET & FUNC_NAME(zz_push_field, n)(BYTE_BUF_NET & bbuf) { ASSERT(DEF_MSG_BLOBLEN(name) <= size); bbuf.Push(DEF_MSG_BLOBLEN(name)); bbuf.Push(name, DEF_MSG_BLOBLEN(name)); return bbuf; }

#define FLD_POP_BASIC(n, name)				BOOL FUNC_NAME(zz_pop_field, n)(BYTE_BUF_NET & bbuf) { return bbuf.Pop(name); }
#define FLD_POP_STRUCT(n, name)				BOOL FUNC_NAME(zz_pop_field, n)(BYTE_BUF_NET & bbuf) { return name.Pop(bbuf); }
#define FLD_POP_ARRAY(n, name, count)		BOOL FUNC_NAME(zz_pop_field, n)(BYTE_BUF_NET & bbuf) { \
                                                                        BOOL bSuccess = TRUE;\
                                                                        for (unsigned int ii = 0; ii < count; ++ii)\
                                                                        { \
                                                                                bSuccess &= bbuf.Pop(name[ii]);\
                                                                         }\
                                                                         return bSuccess;\
                                            }
#define FLD_POP_STARR(n, name, count)		BOOL FUNC_NAME(zz_pop_field, n)(BYTE_BUF_NET & bbuf) {\
                                                                        BOOL bSuccess = TRUE;\
                                                                        for (unsigned int ii = 0; ii < count; ++ii)\
                                                                        {\
                                                                            bSuccess &= name[ii].Pop(bbuf);\
                                                                        }\
                                                                        return bSuccess;\
                                             }
#define FLD_POP_STRING(n, name, count)		BOOL FUNC_NAME(zz_pop_field, n)(BYTE_BUF_NET & bbuf) { return (bbuf.Pop(name, count)); }
#define FLD_POP_BLOB(n, name, size)			BOOL FUNC_NAME(zz_pop_field, n)(BYTE_BUF_NET & bbuf) {\
                                                                        BOOL bSuccess = TRUE;\
                                                                        bSuccess &= bbuf.Pop(DEF_MSG_BLOBLEN(name));\
                                                                        ASSERT_RETVAL(DEF_MSG_BLOBLEN(name) <= size, FALSE);\
                                                                        bSuccess &= (bbuf.Pop(name, DEF_MSG_BLOBLEN(name)));\
                                                                        return bSuccess;\
                                             }

#define MSG_FIELD_DEF(n, type, name, def)		FLD_REG(n, name, #type) FLD_OFFS(name), 1); }					FLD_INIT_BASIC(n, name, (type)def)			FLD_STRING_BASIC(n, name)			FLD_PUSH_BASIC(n, type, name)		FLD_POP_BASIC(n, name)
#define MSG_FLOAT_DEF(n, type, name, def)		FLD_REG(n, name, "float") FLD_OFFS(name), 1); }					FLD_INIT_BASIC(n, name, (float)def)			FLD_STRING_BASIC(n, name)			FLD_PUSH_BASIC(n, float, name)		FLD_POP_BASIC(n, name)
#define MSG_STRUCT_DEF(n, type, name)			STRUCT_REG(n, name, type) FLD_OFFS(name), 1); }																FLD_STRING_STRUCT(n, name)			FLD_PUSH_STRUCT(n, type, name)		FLD_POP_STRUCT(n, name)
#define MSG_ARRAY_DEF(n, type, name, count)		FLD_REG(n, name, #type) FLD_OFFS(name), count); }															FLD_STRING_ARRAY(n, name, count)	FLD_PUSH_ARRAY(n, name, count)		FLD_POP_ARRAY(n, name, count)
#define MSG_STARR_DEF(n, type, name, count)		STRUCT_REG(n, name, type) FLD_OFFS(name), count); }															FLD_STRING_STARR(n, name, count)	FLD_PUSH_STARR(n, name, count)		FLD_POP_STARR(n, name, count)
#define MSG_CSTR_DEF(n, name, count)			FLD_REG(n, name, "char") FLD_OFFS(name), count); }				FLD_INIT_STR(n, char, name)					FLD_STRING_STRING(n, name, count)	FLD_PUSH_STRING(n, name, count)		FLD_POP_STRING(n, name, count)
#define MSG_WSTR_DEF(n, name, count)			FLD_REG(n, name, "WCHAR") FLD_OFFS(name), count); }				FLD_INIT_STR(n, WCHAR, name)				FLD_STRING_STRING(n, name, count)	FLD_PUSH_STRING(n, name, count)		FLD_POP_STRING(n, name, count)
#define MSG_BLOB_DEF(n, ts, name, size)			FLD_REG(n, name, ts) FLD_OFFS(name), size); }					FLD_INIT_BLOB(n, name)						FLD_STRING_BLOB(n, name)			FLD_PUSH_BLOB(n, name, size)		FLD_POP_BLOB(n, name, size)
#define MSG_FIELD_DEF(n, type, name, def)		FLD_REG(n, name, #type) FLD_OFFS(name), 1); }					FLD_INIT_BASIC(n, name, (type)def)			FLD_STRING_BASIC(n, name)			FLD_PUSH_BASIC(n, type, name)		FLD_POP_BASIC(n, name)
#define MSG_FLOAT_DEF(n, type, name, def)		FLD_REG(n, name, "float") FLD_OFFS(name), 1); }					FLD_INIT_BASIC(n, name, (float)def)			FLD_STRING_BASIC(n, name)			FLD_PUSH_BASIC(n, float, name)		FLD_POP_BASIC(n, name)
#define MSG_STRUCT_DEF(n, type, name)			STRUCT_REG(n, name, type) FLD_OFFS(name), 1); }																FLD_STRING_STRUCT(n, name)			FLD_PUSH_STRUCT(n, type, name)		FLD_POP_STRUCT(n, name)
#define MSG_ARRAY_DEF(n, type, name, count)		FLD_REG(n, name, #type) FLD_OFFS(name), count); }															FLD_STRING_ARRAY(n, name, count)	FLD_PUSH_ARRAY(n, name, count)		FLD_POP_ARRAY(n, name, count)
#define MSG_STARR_DEF(n, type, name, count)		STRUCT_REG(n, name, type) FLD_OFFS(name), count); }															FLD_STRING_STARR(n, name, count)	FLD_PUSH_STARR(n, name, count)		FLD_POP_STARR(n, name, count)
#define MSG_CSTR_DEF(n, name, count)			FLD_REG(n, name, "char") FLD_OFFS(name), count); }				FLD_INIT_STR(n, char, name)					FLD_STRING_STRING(n, name, count)	FLD_PUSH_STRING(n, name, count)		FLD_POP_STRING(n, name, count)
#define MSG_WSTR_DEF(n, name, count)			FLD_REG(n, name, "WCHAR") FLD_OFFS(name), count); }				FLD_INIT_STR(n, WCHAR, name)				FLD_STRING_STRING(n, name, count)	FLD_PUSH_STRING(n, name, count)		FLD_POP_STRING(n, name, count)
#define MSG_BLOB_DEFB(n, ts, name, size)		FLD_REG(n, name, ts) FLD_OFFS(name), size); }					FLD_INIT_BLOB(n, name)						FLD_STRING_BLOB(n, name)			FLD_PUSH_BLOB(n, name, size)		FLD_POP_BLOB(n, name, size)
#define MSG_BLOB_DEFW(n, ts, name, size)		FLD_REG(n, name, ts) FLD_OFFS(name), size); }					FLD_INIT_BLOB(n, name)						FLD_STRING_BLOB(n, name)			FLD_PUSH_BLOB(n, name, size)		FLD_POP_BLOB(n, name, size)

#define MSG_FIELD(n, type, name, def)		type name;			MSG_FIELD_DEF(n, type, name, def)
#define MSG_FLOAT(n, type, name, def)		float name;			MSG_FLOAT_DEF(n, float, name, def)
#define MSG_STRUC(n, type, name)			type name;			MSG_STRUCT_DEF(n, type, name)
#define MSG_ARRAY(n, type, name, count)		type name[count];	MSG_ARRAY_DEF(n, type, name, count)
#define MSG_STARR(n, type, name, count)		type name[count];	MSG_STARR_DEF(n, type, name, count)
#define MSG_CHAR(n, name, count)			char name[count];	MSG_CSTR_DEF(n, name, count)
#define MSG_WCHAR(n, name, count)			WCHAR name[count];	MSG_WSTR_DEF(n, name, count)
#define MSG_BLOBB(n, name, size)			BYTE DEF_MSG_BLOBLEN(name); \
											BYTE name[size];	MSG_BLOB_DEFB(n, "BLOBB", name, size+sizeof(DEF_MSG_BLOBLEN(name)))
#define MSG_BLOBW(n, name, size)			WORD DEF_MSG_BLOBLEN(name); \
											BYTE name[size];	MSG_BLOB_DEFW(n, "BLOBW", name, size+sizeof(DEF_MSG_BLOBLEN(name)))

#define END_MSG_STRUCT						};

#define NET_MSG_TABLE_BEGIN					enum {
#define NET_MSG_TABLE_DEF(e, s, r)			e,
#define NET_MSG_TABLE_END(e)				e };


// ---------------------------------------------------------------------------
// MACROS
// ---------------------------------------------------------------------------
#define MSG_SET_BLOB_LEN(m, n, len)			{ unsigned int length = (unsigned int)(len);	\
											  ASSERT(length <= sizeof((m).n));				\
											  ASSERT(length < (1 << BITS_PER_WORD));	\
											  (m).DEF_MSG_BLOBLEN(n) = (WORD)length; }
											 
#define MSG_SET_BBLOB_LEN(m, n, len)		{ unsigned int length = (unsigned int)(len);	\
											  ASSERT(length <= sizeof((m).n));				\
											  ASSERT(length < (1 << BITS_PER_BYTE));	\
											  (m).DEF_MSG_BLOBLEN(n) = (BYTE)length; }
											  
#define MSG_GET_BLOB_LEN(m, n)				((m)->DEF_MSG_BLOBLEN(n))


// ---------------------------------------------------------------------------
// FUNCTIONS
// ---------------------------------------------------------------------------
struct MSG_STRUCT_DECL * NetRegisterMsgStructStart(
	NET_COMMAND_TABLE * cmd_tbl,
	const char ** name,
	BOOL isheader,
	BOOL & bAlreadyDefined,
	BOOL isStreamed = FALSE);

void NetRegisterMsgField(
	NET_COMMAND_TABLE * cmd_tbl,
	unsigned int fieldno,
#if !ISVERSION(RETAIL_VERSION)
	const char * name, 
#endif
	const char * typestr,
	const char ** structTypeStr,
	unsigned int offset,
	unsigned int count);

void NetRegisterMsgStructEnd(
	NET_COMMAND_TABLE * cmd_tbl);

void NetRegisterCommand(
	NET_COMMAND_TABLE * cmd_tbl,
	unsigned int cmd,
	MSG_STRUCT_DECL * msg,
	BOOL bLogOnSend,
	BOOL bLogOnRecv,
	MFP_PRINT mfp_print,
	MFP_PUSHPOP mfp_push,
	MFP_PUSHPOP mfp_pop);

// this is an easier way to use the above NetRegisterCommand() in the common case
template<unsigned int CMD, typename MSG_TYPE> 
inline void NetRegisterCommand(
	NET_COMMAND_TABLE* cmd_tbl, 
	BOOL bLogOnSend = false, 
	BOOL bLogOnRecv = false) 
{
	NetRegisterCommand(cmd_tbl, CMD, MSG_TYPE().zz_register_msg_struct(cmd_tbl), bLogOnSend, bLogOnRecv, 
		MFP_PRINT(&MSG_TYPE::Print), MFP_PUSHPOP(&MSG_TYPE::Push), MFP_PUSHPOP(&MSG_TYPE::Pop));
}


NET_COMMAND_TABLE * NetCommandTableInit(
	unsigned int num_commands);

void NetCommandTableRef(
	NET_COMMAND_TABLE * cmd_tbl);

void NetCommandTableFree(
	NET_COMMAND_TABLE * cmd_tbl);

BOOL NetCommandTableRegisterImmediateMessageCallback(
	NET_COMMAND_TABLE * cmd_tbl,
	NET_CMD command,
	FP_NET_IMM_CALLBACK func);

BOOL NetCommandTableValidate(
	NET_COMMAND_TABLE * cmd_tbl);

DWORD NetCommandTableGetSize(
	NET_COMMAND_TABLE * cmd_tbl);

DWORD NetCommandGetMaxSize(NET_COMMAND_TABLE *tbl, NET_CMD cmd);

BOOL NetTranslateMessageForSend(
	NET_COMMAND_TABLE * cmd_tbl,
	NET_CMD cmd,
	MSG_STRUCT * msg,
	unsigned int & size,
	BYTE_BUF_NET & bbuf);

unsigned int NetCalcMessageSize(
	NET_COMMAND_TABLE * cmd_tbl,
	NET_CMD cmd,
	MSG_STRUCT * msg);

__checkReturn BOOL NetTranslateMessageForRecv(
	NET_COMMAND_TABLE * cmd_tbl,
	BYTE * data,
	unsigned int size,
	MSG_STRUCT * msg);

BOOL NetTraceRecvMessage(
	NET_COMMAND_TABLE * cmd_tbl,
	MSG_STRUCT * msg,
	const char * label);

BOOL NetTraceSendMessage(
	NET_COMMAND_TABLE * cmd_tbl,
	MSG_STRUCT * msg,
	const char * label);

void NetCommandTraceMessageCounts(
	NET_COMMAND_TABLE * cmd_tbl);

void NetCommandResetMessageCounts(
	NET_COMMAND_TABLE * cmd_tbl);


// ---------------------------------------------------------------------------
// SOME GLOBAL MSG TYPES
// ---------------------------------------------------------------------------
DEF_MSG_SUBSTRUCT_INHERIT(MSG_VECTOR, VECTOR)
	MSG_FLOAT_DEF(0, float, x, 0.0f)
	MSG_FLOAT_DEF(1, float, y, 0.0f)
	MSG_FLOAT_DEF(2, float, z, 0.0f)

	MSG_VECTOR & operator = (const VECTOR & v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	END_MSG_STRUCT

DEF_MSG_SUBSTRUCT_INHERIT(MSG_NVECTOR, VECTOR)
	MSG_FLOAT_DEF(0, float, x, 0.0f)
	MSG_FLOAT_DEF(1, float, y, 0.0f)
	MSG_FLOAT_DEF(2, float, z, 0.0f)

	MSG_NVECTOR & operator = (const VECTOR & v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	END_MSG_STRUCT

DEF_MSG_SUBSTRUCT_INHERIT(MSG_MATRIX, MATRIX)
	MSG_ARRAY_DEF(0, float, _m, 16)

	MSG_MATRIX & operator = (const MATRIX & v)
	{
		memcpy(_m, v._m, sizeof(_m));
		return *this;
	}

	END_MSG_STRUCT

DEF_MSG_SUBSTRUCT(MSG_SOCKADDR_STORAGE)
    MSG_FIELD(0,WORD,ss_family,0)
    MSG_BLOBB(1,pad,_SS_MAXSIZE - sizeof(WORD))
    void Set(SOCKADDR_STORAGE *other)
    {
        if(other)
        {
			memcpy(&this->ss_family,other,sizeof(SOCKADDR_STORAGE));
        }
        else
        {
			ss_family = PF_UNSPEC;
            memset(pad,0,_SS_MAXSIZE- sizeof(WORD));
		}
		MSG_SET_BBLOB_LEN( (*this),pad,_SS_MAXSIZE - sizeof(WORD));
    }
    END_MSG_STRUCT

#pragma warning(default : 4100)
#endif // _NETMSG_H_
