
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	GuidUtil.h
	������:		������
	����:		GUID��ع���	
*********************************************************************/

#pragma once

struct GuidUtil
{
	static const char*	ToString( REFGUID guid );

	static GUID			FromString( const char* guidString );

	static bool			IsEmpty( REFGUID guid );
	
	static const GUID	NullGuid;
};

// GUID�Ƚ���
struct guid_less_predicate
{
	bool operator()( REFGUID guid1,REFGUID guid2 ) const
	{
		return memcmp(&guid1,&guid2,sizeof(GUID)) < 0;
	}
};