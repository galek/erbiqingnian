#ifndef _PATCH_CLIENT_MSG_H_
#define _PATCH_CLIENT_MSG_H_

#include "PatchServerMsg.h"

// message table for responses (server to client)
NET_COMMAND_TABLE* PatchClientGetResponseTable();

// message table for requests (client to server)
NET_COMMAND_TABLE* PatchClientGetRequestTable();

#endif