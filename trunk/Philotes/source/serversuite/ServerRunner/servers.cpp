//----------------------------------------------------------------------------
// servers.cpp
//
// This source file programatically creates the bindings between all of the
//	server types, and the necessary external server info. found in their
//  respective libs.
// Globals created:
//			// array of connection tables for servers
//		G_SERVER_CONNECTIONS
//			// array of server specification pointers referencing macro
//			//	generated server spec names for each internal server type 
//		G_SERVERS
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"


//----------------------------------------------------------------------------
// global connection tables
//----------------------------------------------------------------------------
CONNECTION_TABLE	G_SERVER_CONNECTIONS[ TOTAL_SERVER_COUNT ];


//----------------------------------------------------------------------------
// list deliminator removal
//----------------------------------------------------------------------------
#undef  START_CLIENT_TYPES
#undef  START_COMMON_TYPES
#undef  START_PURE_TYPES
#define START_CLIENT_TYPES
#define START_COMMON_TYPES
#define START_PURE_TYPES


//----------------------------------------------------------------------------
// extern SERVER_SPECIFICATION declarations
//----------------------------------------------------------------------------
#undef START_SERVER_LIST
#undef PURE_SERVER_TYPE
#undef HYBRID_SERVER_TYPE
#undef END_SERVER_LIST

#define START_SERVER_LIST
#define PURE_SERVER_TYPE( typeName )	extern SERVER_SPECIFICATION SERVER_SPECIFICATION_NAME( typeName );
#define HYBRID_SERVER_TYPE( typeName )	extern SERVER_SPECIFICATION SERVER_SPECIFICATION_NAME( typeName );
#define END_SERVER_LIST


//----------------------------------------------------------------------------
// FIRST ROUND OF DEFINITIONS
//----------------------------------------------------------------------------
#undef _SERVER_TYPE_DEFS_
#include "servers.h"


//----------------------------------------------------------------------------
// G_SERVERS array declaration
//----------------------------------------------------------------------------
#undef START_SERVER_LIST
#undef PURE_SERVER_TYPE
#undef HYBRID_SERVER_TYPE
#undef END_SERVER_LIST

#define START_SERVER_LIST				SERVER_SPECIFICATION * G_SERVERS[] = {
#define PURE_SERVER_TYPE( typeName )	&SERVER_SPECIFICATION_NAME( typeName ),
#define HYBRID_SERVER_TYPE( typeName )	&SERVER_SPECIFICATION_NAME( typeName ),
#define END_SERVER_LIST					};


//----------------------------------------------------------------------------
// SECOND ROUND OF DEFINITIONS
//----------------------------------------------------------------------------
#undef _SERVER_TYPE_DEFS_
#include "servers.h"


//----------------------------------------------------------------------------
// SERVER NAMES
//----------------------------------------------------------------------------
#undef START_SERVER_LIST
#undef PURE_SERVER_TYPE
#undef HYBRID_SERVER_TYPE
#undef END_SERVER_LIST

#define START_SERVER_LIST				static const char * SG_SERVER_NAMES[] = {
#define PURE_SERVER_TYPE( typeName )	#typeName,
#define HYBRID_SERVER_TYPE( typeName )	#typeName,
#define END_SERVER_LIST					};


//----------------------------------------------------------------------------
// THIRD ROUND OF DEFINITIONS
//----------------------------------------------------------------------------
#undef _SERVER_TYPE_DEFS_
#include "servers.h"


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * ServerGetName(
	UINT serverType )
{
	if(serverType >= TOTAL_SERVER_COUNT)
		return "*INVALID*";
	return SG_SERVER_NAMES[serverType];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT ServerGetType(
	const WCHAR * serverName )
{
	ASSERT_RETX(serverName && serverName[0], INVALID_SVRTYPE);

	char sbServerName[MAX_PATH];
	PStrCvt(sbServerName, serverName, MAX_PATH);

	for(UINT ii = 0; ii < TOTAL_SERVER_COUNT; ++ii)
	{
		if(PStrCmp(sbServerName, SG_SERVER_NAMES[ii], MAX_PATH) == 0)
			return ii;
	}
	return INVALID_SVRTYPE;
}
