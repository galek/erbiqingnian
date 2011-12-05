//----------------------------------------------------------------------------
// stdCommonIncludes.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//	COMMON INCLUDES
//----------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN

// for builds made by the build server, _WIN32_WINNT is already set to 0x0502
#ifndef _WIN32_WINNT 
#define _WIN32_WINNT 0x0501 // windows xp
#endif

#include <sal.h>
#include <ObjBase.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winperf.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <intrin.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <ws2tcpip.h>
#include <map>
#include <hash_map>

#include <common.h>
#include <commontypes.h>
#include <debug.h>
#include <presult.h>
#include <memory.h>
#include <pstring.h>
#include <id.h>
#include <log.h>
#include <critsect.h>
#include <net.h>
#include <ptime.h>

#pragma warning(push)
#pragma warning(disable : 6011)
#include <array.h>
#pragma warning(pop)
#pragma warning(default : 6011)

#include <hash.h>
#include <float.h>
#include <math.h>
#include <bitmanip.h>
#include <maths.h>
#include <freelist.h>
#include <list.h>
#include <winos.h>
#include <globals.h>
#include <vector.h>
#include <matrix.h>
#include <netmsg.h>
#include <memory>
#include <sstream>
#pragma intrinsic(memcpy,memset)
#include <hlocks.h>
// common server includes.
#if ISVERSION(SERVER_VERSION)
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#endif

#pragma warning(disable : 6262) // Can't win this one. Use all the stack you want.