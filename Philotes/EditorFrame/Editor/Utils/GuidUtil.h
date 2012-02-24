
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	GuidUtil.h
	������:		������
	����:		GUID��ع���	
*********************************************************************/

#pragma once

_NAMESPACE_BEGIN

struct GuidUtil
{
	static const char*	ToString( REFGUID guid );

	static GUID			FromString( const char* guidString );

	static bool			IsEmpty( REFGUID guid );
	
	static const GUID	NullGuid;
};

// GUID�Ƚ�
struct GuidlessPredicate
{
	bool operator()( REFGUID guid1,REFGUID guid2 ) const
	{
		return memcmp(&guid1,&guid2,sizeof(GUID)) < 0;
	}
};

_NAMESPACE_END