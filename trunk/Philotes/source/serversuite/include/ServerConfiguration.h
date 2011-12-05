//----------------------------------------------------------------------------
// ServerConfiguration.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _SERVERCONFIGURATION_H_
#define _SERVERCONFIGURATION_H_


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _MARKUP_H_
#include "markup.h"
#endif
#ifndef _BITMAP_H_
#include "bitmanip.h"
#endif


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_SVRCONFIG_FIELDS			30	// to change this you have to update SVRCONFIG...
#define MAX_SVRCONFIG_STR_LEN			2048	// internal string buffer length.
#define MAX_SVR_CONFIG_STREAMED_SIZE	512	// max size of config when streamed.
#define MAX_SVR_CONFIG_ENUM_FIELDS		32	// max enum values in a svr config enum field type


//----------------------------------------------------------------------------
// ENUM SUPPORT
// to add a new enum type to the list of recognized enums,
//		see ServerConfiguration.cpp
//----------------------------------------------------------------------------
struct SvrConfigEnumValue
{
	char *				EnumValueName;
	DWORD				EnumValueBits;
};
struct SvrConfigEnumType
{
	char *				EnumName;
	SvrConfigEnumValue	EnumValues[MAX_SVR_CONFIG_ENUM_FIELDS+1];
};
extern SvrConfigEnumType * G_SVR_CONFIG_ENUMS[];
BOOL SvrConfigParseEnum( CMarkup *, const char *, DWORD & );



//----------------------------------------------------------------------------
// BASE TYPES
//----------------------------------------------------------------------------
class SVRCONFIG
{
protected:
	const TCHAR * m_typeName;

	BOOL StringIsOk( const TCHAR * str )
	{
		if( !str )
			return FALSE;

		BOOL toRet = FALSE;
		UINT ii    = 0;
		while( str[ii] && !toRet )
		{
			//	string is ok as long as its not all whitespace.
			toRet = ( str[ii] != TEXT(' ') && str[ii] != TEXT('\t') );
			++ii;
		}
		return toRet;
	}
public:
	//	field methods
#pragma warning(push)
#pragma warning( disable: 4100 )
	virtual BOOL zz_load_xml_0( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_1( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_2( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_3( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_4( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_5( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_6( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_7( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_8( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_9( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_10( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_11( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_12( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_13( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_14( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_15( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_16( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_17( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_load_xml_18( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_19( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_20( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_21( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_22( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_23( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_24( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_25( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_26( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_27( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_28( CMarkup * xml ) { return TRUE; }
  virtual BOOL zz_load_xml_29( CMarkup * xml ) { return TRUE; }
	virtual BOOL zz_gen_xml_0( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_1( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_2( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_3( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_4( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_5( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_6( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_7( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_8( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_9( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_10( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_11( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_12( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_13( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_14( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_15( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_16( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_17( char * &, int & ) { return TRUE; }
	virtual BOOL zz_gen_xml_18( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_19( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_20( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_21( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_22( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_23( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_24( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_25( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_26( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_27( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_28( char * &, int & ) { return TRUE; }
  virtual BOOL zz_gen_xml_29( char * &, int & ) { return TRUE; }
	virtual BOOL zz_stream_0( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_1( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_2( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_3( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_4( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_5( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_6( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_7( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_8( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_9( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_10( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_11( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_12( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_13( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_14( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_15( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_16( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_17( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_stream_18( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_19( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_20( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_21( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_22( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_23( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_24( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_25( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_26( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_27( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_28( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_stream_29( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_0( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_1( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_2( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_3( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_4( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_5( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_6( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_7( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_8( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_9( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_10( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_11( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_12( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_13( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_14( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_15( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_16( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_17( BYTE_BUF_NET * buf ) { return TRUE; }
	virtual BOOL zz_inflate_18( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_19( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_20( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_21( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_22( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_23( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_24( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_25( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_26( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_27( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_28( BYTE_BUF_NET * buf ) { return TRUE; }
  virtual BOOL zz_inflate_29( BYTE_BUF_NET * buf ) { return TRUE; }
#pragma warning(pop)

	//	Server Config Methods
	const TCHAR * GetTypeName(
		void )
	{
		return m_typeName;
	}
	BOOL LoadFromMarkup(
		CMarkup * xml )
	{
		ASSERT_RETFALSE( xml );
		//	must be positioned at correct tag.
		if( PStrCmp( m_typeName, xml->GetTagName() ) != 0 )
		{
			char msg[512];
			PStrPrintf(msg,512,"Server config not at starting element \"%s\".",m_typeName);
			ASSERT_MSG(msg);
			return FALSE;
		}
		BOOL toRet = TRUE;
		toRet = toRet && this->zz_load_xml_0( xml );
		toRet = toRet && this->zz_load_xml_1( xml );
		toRet = toRet && this->zz_load_xml_2( xml );
		toRet = toRet && this->zz_load_xml_3( xml );
		toRet = toRet && this->zz_load_xml_4( xml );
		toRet = toRet && this->zz_load_xml_5( xml );
		toRet = toRet && this->zz_load_xml_6( xml );
		toRet = toRet && this->zz_load_xml_7( xml );
		toRet = toRet && this->zz_load_xml_8( xml );
		toRet = toRet && this->zz_load_xml_9( xml );
		toRet = toRet && this->zz_load_xml_10( xml );
		toRet = toRet && this->zz_load_xml_11( xml );
		toRet = toRet && this->zz_load_xml_12( xml );
		toRet = toRet && this->zz_load_xml_13( xml );
		toRet = toRet && this->zz_load_xml_14( xml );
		toRet = toRet && this->zz_load_xml_15( xml );
		toRet = toRet && this->zz_load_xml_16( xml );
		toRet = toRet && this->zz_load_xml_17( xml );
		toRet = toRet && this->zz_load_xml_18( xml );
    toRet = toRet && this->zz_load_xml_19( xml );
    toRet = toRet && this->zz_load_xml_20( xml );
    toRet = toRet && this->zz_load_xml_21( xml );
    toRet = toRet && this->zz_load_xml_22( xml );
    toRet = toRet && this->zz_load_xml_23( xml );
    toRet = toRet && this->zz_load_xml_24( xml );
    toRet = toRet && this->zz_load_xml_25( xml );
    toRet = toRet && this->zz_load_xml_26( xml );
    toRet = toRet && this->zz_load_xml_27( xml );
    toRet = toRet && this->zz_load_xml_28( xml );
    toRet = toRet && this->zz_load_xml_29( xml );
		return toRet;
	}
	BOOL StreamToMarkup(
		char * & dest,
		int & destLen )
	{
		ASSERT_RETFALSE(dest && destLen > 0);
		int written = PStrPrintf(
						dest,
						destLen,
						"<%s>",
						m_typeName);
		dest += written;
		destLen -= written;

		BOOL toRet = (destLen > 0);
		toRet = toRet && this->zz_gen_xml_0( dest, destLen );
		toRet = toRet && this->zz_gen_xml_1( dest, destLen );
		toRet = toRet && this->zz_gen_xml_2( dest, destLen );
		toRet = toRet && this->zz_gen_xml_3( dest, destLen );
		toRet = toRet && this->zz_gen_xml_4( dest, destLen );
		toRet = toRet && this->zz_gen_xml_5( dest, destLen );
		toRet = toRet && this->zz_gen_xml_6( dest, destLen );
		toRet = toRet && this->zz_gen_xml_7( dest, destLen );
		toRet = toRet && this->zz_gen_xml_8( dest, destLen );
		toRet = toRet && this->zz_gen_xml_9( dest, destLen );
		toRet = toRet && this->zz_gen_xml_10( dest, destLen );
		toRet = toRet && this->zz_gen_xml_11( dest, destLen );
		toRet = toRet && this->zz_gen_xml_12( dest, destLen );
		toRet = toRet && this->zz_gen_xml_13( dest, destLen );
		toRet = toRet && this->zz_gen_xml_14( dest, destLen );
		toRet = toRet && this->zz_gen_xml_15( dest, destLen );
		toRet = toRet && this->zz_gen_xml_16( dest, destLen );
		toRet = toRet && this->zz_gen_xml_17( dest, destLen );
		toRet = toRet && this->zz_gen_xml_18( dest, destLen );
    toRet = toRet && this->zz_gen_xml_19( dest, destLen );
    toRet = toRet && this->zz_gen_xml_20( dest, destLen );
    toRet = toRet && this->zz_gen_xml_21( dest, destLen );
    toRet = toRet && this->zz_gen_xml_22( dest, destLen );
    toRet = toRet && this->zz_gen_xml_23( dest, destLen );
    toRet = toRet && this->zz_gen_xml_24( dest, destLen );
    toRet = toRet && this->zz_gen_xml_25( dest, destLen );
    toRet = toRet && this->zz_gen_xml_26( dest, destLen );
    toRet = toRet && this->zz_gen_xml_27( dest, destLen );
    toRet = toRet && this->zz_gen_xml_28( dest, destLen );
    toRet = toRet && this->zz_gen_xml_29( dest, destLen );
		if(toRet)
		{
			written = PStrPrintf(
						dest,
						destLen,
						"</%s>",
						m_typeName);
			dest += written;
			destLen -= written;
		}
		return toRet;
	}
	BOOL StreamData(
		BYTE_BUF_NET * buf )
	{
		ASSERT_RETFALSE( buf );
		BOOL toRet = TRUE;
		toRet = toRet && this->zz_stream_0( buf );
		toRet = toRet && this->zz_stream_1( buf );
		toRet = toRet && this->zz_stream_2( buf );
		toRet = toRet && this->zz_stream_3( buf );
		toRet = toRet && this->zz_stream_4( buf );
		toRet = toRet && this->zz_stream_5( buf );
		toRet = toRet && this->zz_stream_6( buf );
		toRet = toRet && this->zz_stream_7( buf );
		toRet = toRet && this->zz_stream_8( buf );
		toRet = toRet && this->zz_stream_9( buf );
		toRet = toRet && this->zz_stream_10( buf );
		toRet = toRet && this->zz_stream_11( buf );
		toRet = toRet && this->zz_stream_12( buf );
		toRet = toRet && this->zz_stream_13( buf );
		toRet = toRet && this->zz_stream_14( buf );
		toRet = toRet && this->zz_stream_15( buf );
		toRet = toRet && this->zz_stream_16( buf );
		toRet = toRet && this->zz_stream_17( buf );
		toRet = toRet && this->zz_stream_18( buf );
    toRet = toRet && this->zz_stream_19( buf );
    toRet = toRet && this->zz_stream_20( buf );
    toRet = toRet && this->zz_stream_21( buf );
    toRet = toRet && this->zz_stream_22( buf );
    toRet = toRet && this->zz_stream_23( buf );
    toRet = toRet && this->zz_stream_24( buf );
    toRet = toRet && this->zz_stream_25( buf );
    toRet = toRet && this->zz_stream_26( buf );
    toRet = toRet && this->zz_stream_27( buf );
    toRet = toRet && this->zz_stream_28( buf );
    toRet = toRet && this->zz_stream_29( buf );
		return toRet;
	}
	BOOL LoadFromStream(
		BYTE_BUF_NET * buf )
	{
		ASSERT_RETFALSE( buf );
		BOOL toRet = TRUE;
		toRet = toRet && this->zz_inflate_0( buf );
		toRet = toRet && this->zz_inflate_1( buf );
		toRet = toRet && this->zz_inflate_2( buf );
		toRet = toRet && this->zz_inflate_3( buf );
		toRet = toRet && this->zz_inflate_4( buf );
		toRet = toRet && this->zz_inflate_5( buf );
		toRet = toRet && this->zz_inflate_6( buf );
		toRet = toRet && this->zz_inflate_7( buf );
		toRet = toRet && this->zz_inflate_8( buf );
		toRet = toRet && this->zz_inflate_9( buf );
		toRet = toRet && this->zz_inflate_10( buf );
		toRet = toRet && this->zz_inflate_11( buf );
		toRet = toRet && this->zz_inflate_12( buf );
		toRet = toRet && this->zz_inflate_13( buf );
		toRet = toRet && this->zz_inflate_14( buf );
		toRet = toRet && this->zz_inflate_15( buf );
		toRet = toRet && this->zz_inflate_16( buf );
		toRet = toRet && this->zz_inflate_17( buf );
		toRet = toRet && this->zz_inflate_18( buf );
    toRet = toRet && this->zz_inflate_19( buf );
    toRet = toRet && this->zz_inflate_20( buf );
    toRet = toRet && this->zz_inflate_21( buf );
    toRet = toRet && this->zz_inflate_22( buf );
    toRet = toRet && this->zz_inflate_23( buf );
    toRet = toRet && this->zz_inflate_24( buf );
    toRet = toRet && this->zz_inflate_25( buf );
    toRet = toRet && this->zz_inflate_26( buf );
    toRet = toRet && this->zz_inflate_27( buf );
    toRet = toRet && this->zz_inflate_28( buf );
    toRet = toRet && this->zz_inflate_29( buf );
		return toRet;
	}
};

typedef SVRCONFIG * (*FP_SVRCONFIG_CREATOR)( struct MEMORYPOOL * pool );

BOOL SvrConfigParseBool(const TCHAR * str);

void SvrConfigTraceError(const CHAR * typeName, const TCHAR * fieldName);


//----------------------------------------------------------------------------
// CUSTOM CONFIG MACROS
//----------------------------------------------------------------------------

#define SVRCONFIG_CREATOR_FUNC(name)	name##::NEW_##name

#define CONFIG_ASSERT(name)			SvrConfigTraceError(this->m_typeName,name)

#define START_SVRCONFIG_DEF(name)	class name : public SVRCONFIG { \
										public: \
											name( void ) { m_typeName = TEXT(#name); } \
											void * operator new ( size_t size, void * place ) { \
												UNREFERENCED_PARAMETER(size); \
												return (name*)place; } \
											void operator delete ( void *, void * ) {} \
											static SVRCONFIG * NEW_##name( \
												struct MEMORYPOOL * pool ) \
												{ name * toRet = (name*)MALLOC(pool,sizeof(name)); \
												  ASSERT_RETNULL( toRet ); \
												  new(toRet)name(); \
												  return toRet; }

#define SVRCONFIG_INT_FIELD(i, n, required, def_value) \
									int n; \
									virtual BOOL zz_load_xml_##i( CMarkup * xml ) { \
										STATIC_CHECK(i < MAX_SVRCONFIG_FIELDS, TOO_MANY_SVRCONFIG_PARAMS); \
										n = (def_value); \
										if( !xml->FindChildElem( TEXT(#n) ) ) { \
											bool bRequired = (required); /*avoid warning C4127*/ \
											if (!bRequired) return TRUE; \
											CONFIG_ASSERT(TEXT(#n)); return FALSE; } \
										BOOL toRet = PStrToInt( xml->GetChildData(), n ); \
										xml->ResetChildPos(); \
										if(!toRet) { CONFIG_ASSERT(TEXT(#n)); } \
										return toRet; } \
									virtual BOOL zz_gen_xml_##i( char * & dest, int & destLen ) { \
										int written = PStrPrintf( dest, destLen, "<%s>%d</%s>", #n, n, #n ); \
										dest += written; \
										destLen -= written; \
										return TRUE; } \
									virtual BOOL zz_stream_##i( BYTE_BUF_NET * buf ) { \
										return buf->Push<int>( n ); } \
									virtual BOOL zz_inflate_##i( BYTE_BUF_NET * buf ) { \
										return buf->Pop<int>( n ); };

#define SVRCONFIG_FLOAT_FIELD(i, n, required, def_value) \
									float n; \
									virtual BOOL zz_load_xml_##i( CMarkup * xml ) { \
										STATIC_CHECK(i < MAX_SVRCONFIG_FIELDS, TOO_MANY_SVRCONFIG_PARAMS); \
										n = (def_value); \
										if( !xml->FindChildElem( TEXT(#n) ) ) { \
											bool bRequired = (required); /*avoid warning C4127*/ \
											if (!bRequired) return TRUE; \
											CONFIG_ASSERT(TEXT(#n)); return FALSE; } \
										BOOL toRet = PStrToFloat( xml->GetChildData(), n ); \
										xml->ResetChildPos(); \
										if(!toRet) { CONFIG_ASSERT(TEXT(#n)); } \
										return toRet; } \
									virtual BOOL zz_gen_xml_##i( char * & dest, int & destLen ) { \
										int written = PStrPrintf( dest, destLen, "<%s>%f</%s>", #n, n, #n ); \
										dest += written; \
										destLen -= written; \
										return TRUE; } \
									virtual BOOL zz_stream_##i( BYTE_BUF_NET * buf ) { \
										return buf->Push<float>( n ); } \
									virtual BOOL zz_inflate_##i( BYTE_BUF_NET * buf ) { \
										return buf->Pop<float>( n ); };

#define SVRCONFIG_STR_FIELD(i, n, required, def_value) \
									TCHAR n[MAX_SVRCONFIG_STR_LEN]; \
									virtual BOOL zz_load_xml_##i( CMarkup * xml ) { \
										STATIC_CHECK(i < MAX_SVRCONFIG_FIELDS, TOO_MANY_SVRCONFIG_PARAMS); \
										PStrCopy(n, (def_value), arrsize(n)); \
										bool bRequired = (required); /*avoid warning C4127*/ \
										if( !xml->FindChildElem( TEXT(#n) ) ) { \
											if (!bRequired) return TRUE; \
											CONFIG_ASSERT(TEXT(#n)); return FALSE; } \
										BOOL toRet = ( !bRequired || StringIsOk( xml->GetChildData() ) ); \
										PStrCopy(n,MAX_SVRCONFIG_STR_LEN,xml->GetChildData(),MAX_XML_STRING_LENGTH); \
										xml->ResetChildPos(); \
										if(!toRet) { CONFIG_ASSERT(TEXT(#n)); } \
										return toRet; } \
									virtual BOOL zz_gen_xml_##i( char * & dest, int & destLen ) { \
										int written = PStrPrintf( dest, destLen, "<%s>%s</%s>", #n, n, #n ); \
										dest += written; \
										destLen -= written; \
										return TRUE; } \
									virtual BOOL zz_stream_##i( BYTE_BUF_NET * buf ) { \
										return buf->Push( n, MAX_SVRCONFIG_STR_LEN ); } \
									virtual BOOL zz_inflate_##i( BYTE_BUF_NET * buf ) { \
										return buf->Pop( n, MAX_SVRCONFIG_STR_LEN ); };

#define SVRCONFIG_BIT_FIELD(i, n, val_type, required, def_value) \
									DWORD n; \
									virtual BOOL zz_load_xml_##i( CMarkup * xml ) { \
										STATIC_CHECK(i < MAX_SVRCONFIG_FIELDS, TOO_MANY_SVRCONFIG_PARAMS); \
										n = (def_value); \
										if( !xml->FindChildElem( TEXT(#n) ) ) { \
											bool bRequired = (required); /*avoid warning C4127*/ \
											if (!bRequired) return TRUE; \
											CONFIG_ASSERT(TEXT(#n)); return FALSE; } \
										BOOL toRet = SvrConfigParseEnum( xml, #val_type, n ); \
										xml->ResetChildPos(); \
										if( !toRet ) { CONFIG_ASSERT(TEXT(#n)); } \
										return toRet; } \
									virtual BOOL zz_gen_xml_##i( char * &, int & ) { \
										return FALSE; } \
									virtual BOOL zz_stream_##i( BYTE_BUF_NET * buf ) { \
										return buf->Push<DWORD>( n ); } \
									virtual BOOL zz_inflate_##i( BYTE_BUF_NET * buf ) { \
										return buf->Pop<DWORD>( n ); };

#define SVRCONFIG_BOL_FIELD(i, n, required, def_value) \
									BOOL n; \
									virtual BOOL zz_load_xml_##i( CMarkup * xml ) { \
										STATIC_CHECK(i < MAX_SVRCONFIG_FIELDS, TOO_MANY_SVRCONFIG_PARAMS); \
										n = def_value; \
										bool bRequired = (required); /*avoid warning C4127*/ \
										if( !xml->FindChildElem( TEXT(#n) ) ) { \
											if (!bRequired) return TRUE; \
											CONFIG_ASSERT(TEXT(#n)); return FALSE; } \
										n = SvrConfigParseBool(xml->GetChildData()); \
										xml->ResetChildPos(); \
										return TRUE; } \
									virtual BOOL zz_gen_xml_##i( char * & dest, int & destLen ) { \
										int written = PStrPrintf( dest, destLen, "<%s>%s</%s>", TEXT(#n), n ? TEXT("true") : TEXT("false"), TEXT(#n) ); \
										dest += written; \
										destLen -= written; \
										return TRUE; } \
									virtual BOOL zz_stream_##i( BYTE_BUF_NET * buf ) { \
										return buf->Push<BOOL>( n ); } \
									virtual BOOL zz_inflate_##i( BYTE_BUF_NET * buf ) { \
										return buf->Pop<BOOL>( n ); };

#define END_SVRCONFIG_DEF			};

#define DERIVED_SVRCONFIG_DEF(name, base)	class name : public base { \
												public: \
													name( void ) { m_typeName = TEXT(#name); } \
											};


/*----------------------------------------------------------------------------
  ---------------------------  EXAMPLE SVRCONFIG  ----------------------------
  ----------------------------------------------------------------------------

  //	example server config definition
  START_SVRCONFIG_DEF(ExampleConfig)
	SVRCONFIG_INT_FIELD( 0, ExampleIntField )
	SVRCONFIG_STR_FIELD( 1, ExampleStrField )
	SVRCONFIG_INT_FIELD( 2, AnotherIntField )
  END_SVRCONFIG_DEF


  //	this will define an xml markup specified as such...
  //	<ExampleConfig>
  //		<ExampleIntField>123</ExampleIntField>
  //		<ExampleStrField>a string</ExampleStrField>
  //		<AnotherIntField>456</AnotherIntField>
  //	</ExampleConfig>


  //	register your config object by adding its creator method pointer
  //		to your server specification by using the naming macro.
  FP_SVRCONFIG_CREATOR * myCreator = SVRCONFIG_CREATOR_FUNC(ExampleConfig);


  //	public methods LoadFromMarkup, StreamData, and LoadFromStream
  //		will be used by the ServerRunner to initialize your fields
  //		for you at server instance startup. if config init fails
  //		the server instance will not be started.


  //	access fields normally
  void AccessExample( SVRCONFIG * bar )
  {
    ExampleConfig * foo  = (ExampleConfig*)bar;
    int theInt           = foo->ExampleIntField;
    const char * theCstr = foo->ExampleStrField;
  }


//---------------------------------------------------------------------------*/

#endif	//	_SERVERCONFIGURATION_H_
