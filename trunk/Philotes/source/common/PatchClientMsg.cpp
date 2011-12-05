#include <stdafx.h>
#include <net.h>

#include "PatchClientMsg.h"

// message table for responses (server to client)
NET_COMMAND_TABLE* PatchClientGetResponseTable()
{
	NET_COMMAND_TABLE * toRet = NULL;

	toRet = NetCommandTableInit(PATCH_RESPONSE_COUNT);
	ASSERT_RETNULL(toRet != NULL);

#undef  PATCH_MSG_RESPONSE_TABLE_BEGIN
#undef  PATCH_MSG_RESPONSE_TABLE_DEF
#undef  PATCH_MSG_RESPONSE_TABLE_END

#define PATCH_MSG_RESPONSE_TABLE_BEGIN(tbl)
#define PATCH_MSG_RESPONSE_TABLE_END(count)
#define PATCH_MSG_RESPONSE_TABLE_DEF(c, strct, s, r) {						\
	strct msg;																\
	MSG_STRUCT_DECL* msg_struct = msg.zz_register_msg_struct(toRet);		\
	NetRegisterCommand(toRet, c, msg_struct, s, r,							\
		static_cast<MFP_PRINT>(& strct::Print),								\
		static_cast<MFP_PUSHPOP>(& strct::Push),							\
		static_cast<MFP_PUSHPOP>(& strct::Pop));							\
	}

#undef  _PATCH_RESPONSE_TABLE_
#include "PatchServerMsg.h"

	if (NetCommandTableValidate(toRet) == FALSE) {
		NetCommandTableFree(toRet);
		return NULL;
	} else {
		return toRet;
	}
}

// message table for requests (client to server)
NET_COMMAND_TABLE* PatchClientGetRequestTable()
{
	NET_COMMAND_TABLE* toRet = NULL;

	toRet = NetCommandTableInit(PATCH_REQUEST_COUNT);
	ASSERT_RETNULL(toRet != NULL);

#undef  PATCH_MSG_REQUEST_TABLE_BEGIN
#undef  PATCH_MSG_REQUEST_TABLE_DEF
#undef  PATCH_MSG_REQUEST_TABLE_END

#define PATCH_MSG_REQUEST_TABLE_BEGIN(tbl)
#define PATCH_MSG_REQUEST_TABLE_END(count)
#define PATCH_MSG_REQUEST_TABLE_DEF(c, strct, hdl, s, r) {					\
	strct msg;																\
	MSG_STRUCT_DECL* msg_struct = msg.zz_register_msg_struct(toRet);		\
	NetRegisterCommand(toRet, c, msg_struct, s, r,							\
		static_cast<MFP_PRINT>(& strct::Print),								\
		static_cast<MFP_PUSHPOP>(& strct::Push),							\
		static_cast<MFP_PUSHPOP>(& strct::Pop));							\
	}

#undef  _PATCH_REQUEST_TABLE_
#include "PatchServerMsg.h"

	if (NetCommandTableValidate(toRet) == FALSE) {
		NetCommandTableFree(toRet);
		return NULL;
	} else {
		return toRet;
	}
}