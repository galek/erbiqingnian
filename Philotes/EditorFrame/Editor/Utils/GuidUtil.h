
/********************************************************************
	日期:		2012年2月20日
	文件名: 	GuidUtil.h
	创建者:		王延兴
	描述:		GUID相关工具	
*********************************************************************/

#pragma once

struct GuidUtil
{
	static const char*	ToString( REFGUID guid );

	static GUID			FromString( const char* guidString );

	static bool			IsEmpty( REFGUID guid );
	
	static const GUID	NullGuid;
};

// GUID比较用
struct guid_less_predicate
{
	bool operator()( REFGUID guid1,REFGUID guid2 ) const
	{
		return memcmp(&guid1,&guid2,sizeof(GUID)) < 0;
	}
};