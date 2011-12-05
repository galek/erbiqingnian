//----------------------------------------------------------------------------
// ServerConfiguration.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "runnerstd.h"
#if ISVERSION(SERVER_VERSION)
#include "ServerConfiguration.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// FLAGS SPECIFIC INCLUDES
//----------------------------------------------------------------------------
#include "dbgtrace.h"


//----------------------------------------------------------------------------
// SERVER CONFIG ENUM TYPE VALUES
// put here what you will, just keep it tidy.
//----------------------------------------------------------------------------

SvrConfigEnumType WppTracingFlags = 
{
	"WPP_TRACE_FLAGS",
	{
		{ "TRACE_FLAG_ALL",					WPP_TRACE_FLAGS_ALL							},
		{ "TRACE_FLAG_DEBUG_BUILD_ONLY",	WPP_REAL_FLAG(TRACE_FLAG_DEBUG_BUILD_ONLY)	},
		{ "TRACE_FLAG_COMMON_NET",			WPP_REAL_FLAG(TRACE_FLAG_COMMON_NET)		},
		{ "TRACE_FLAG_PERFCTR_LIB",			WPP_REAL_FLAG(TRACE_FLAG_PERFCTR_LIB)		},
		{ "TRACE_FLAG_MUXLIB",				WPP_REAL_FLAG(TRACE_FLAG_MUXLIB)			},
		{ "TRACE_FLAG_SRUNNER",				WPP_REAL_FLAG(TRACE_FLAG_SRUNNER)			},
		{ "TRACE_FLAG_SRUNNER_NET",			WPP_REAL_FLAG(TRACE_FLAG_SRUNNER_NET)		},
		{ "TRACE_FLAG_SRUNNER_DATABASE",	WPP_REAL_FLAG(TRACE_FLAG_SRUNNER_DATABASE)	},
		{ "TRACE_FLAG_USER_SERVER",			WPP_REAL_FLAG(TRACE_FLAG_USER_SERVER)		},
		{ "TRACE_FLAG_CHAT_SERVER",			WPP_REAL_FLAG(TRACE_FLAG_CHAT_SERVER)		},
		{ "TRACE_FLAG_PATCH_SERVER",		WPP_REAL_FLAG(TRACE_FLAG_PATCH_SERVER)		},
		{ "TRACE_FLAG_LOGIN_SERVER",		WPP_REAL_FLAG(TRACE_FLAG_LOGIN_SERVER)		},
		{ "TRACE_FLAG_GAME_MISC_LOG",		WPP_REAL_FLAG(TRACE_FLAG_GAME_MISC_LOG)		},
		{ "TRACE_FLAG_GAME_NET",			WPP_REAL_FLAG(TRACE_FLAG_GAME_NET)			},
		{ "TRACE_FLAG_PIPE_SERVER",			WPP_REAL_FLAG(TRACE_FLAG_PIPE_SERVER)		},
		{ "TRACE_FLAG_LOADBALANCE_SERVER",	WPP_REAL_FLAG(TRACE_FLAG_LOADBALANCE_SERVER)},
		{ "TRACE_FLAG_EVENTLOG",			WPP_REAL_FLAG(TRACE_FLAG_EVENTLOG)			},
		{ "TRACE_FLAG_BILLING_PROXY",		WPP_REAL_FLAG(TRACE_FLAG_BILLING_PROXY)		},

		{ NULL,	0 }	//	must null terminate list
	}
};


//----------------------------------------------------------------------------
// ENUM TYPE LIST
//----------------------------------------------------------------------------
SvrConfigEnumType * G_SVR_CONFIG_ENUMS[] = 
{
	&WppTracingFlags,

	NULL	//	keep last
};


//----------------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// just a linear search for now...
//----------------------------------------------------------------------------
const SvrConfigEnumType * sSvrConfigFindEnumType(
	const char * enumTypeName )
{
	ASSERT_RETNULL( enumTypeName );

	UINT ii = 0;
	while( G_SVR_CONFIG_ENUMS[ii] )
	{
		if( PStrCmp( G_SVR_CONFIG_ENUMS[ii]->EnumName, enumTypeName ) == 0 )
			return G_SVR_CONFIG_ENUMS[ii];
		++ii;
	}

	return NULL;
}

//----------------------------------------------------------------------------
// just a linear search for now...
//----------------------------------------------------------------------------
const DWORD * sSvrConfigFindEnumFlag(
	const SvrConfigEnumType * enumType,
	const char * enumFlag )
{
	ASSERT_RETNULL( enumType );
	ASSERT_RETNULL( enumFlag );

	UINT ii = 0;
	while( enumType->EnumValues[ii].EnumValueName )
	{
		if( PStrCmp( enumType->EnumValues[ii].EnumValueName, enumFlag ) == 0 )
			return &enumType->EnumValues[ii].EnumValueBits;
		++ii;
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrConfigParseEnum(
	CMarkup * xml,
	const char * enumTypeName,
	DWORD & destFlags )
{
	ASSERT_RETFALSE( xml );
	ASSERT_RETFALSE( enumTypeName );
	destFlags = 0;

	const SvrConfigEnumType * enumType = sSvrConfigFindEnumType( enumTypeName );
	if( !enumType )
	{
		char msg[1024];
		PStrPrintf( msg, 1024, "Server config: unable to find enum type definition for type \"%s\".", enumTypeName );
		ASSERT_MSG( msg );
		return FALSE;
	}

	if( !xml->IntoElem() )
		return TRUE;	//	no flags specified

	while( xml->FindChildElem() )
	{
		const char * enumFlag = xml->GetChildTagName();
		ASSERT_RETFALSE( enumFlag );

		const DWORD * val = sSvrConfigFindEnumFlag( enumType, enumFlag );
		if( !val )
		{
			char msg[1024];
			PStrPrintf( msg, 1024, "Server config: unable to find enum flag \"%s\" value for enum type \"%s\".", enumFlag, enumTypeName );
			ASSERT_MSG( msg );
			return FALSE;
		}

		if( PStrToInt( xml->GetChildData() ) != 0 )
		{
			destFlags |= val[0];
		}
		else
		{
			destFlags &= (~val[0]);
		}
	}

	return xml->OutOfElem();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SvrConfigParseBool(
	const TCHAR * str)
{
	if (!str)
		return FALSE;

	if (PStrICmp(str, "1") == 0)
		return TRUE;

	if (PStrICmp(str, "true") == 0)
		return TRUE;

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SvrConfigTraceError(
	const CHAR * typeName,
	const TCHAR * fieldName)
{
	TraceError("Error while parsing server config element \"%s::%s\".", typeName, fieldName);
}
