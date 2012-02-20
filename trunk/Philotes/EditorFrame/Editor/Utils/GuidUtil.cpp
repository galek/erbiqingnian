
#include "GuidUtil.h"

//////////////////////////////////////////////////////////////////////////
const GUID GuidUtil::NullGuid = { 0, 0, 0, { 0,0,0,0,0,0,0,0 } };

GUID GuidUtil::FromString( const char *guidString )
{
	GUID guid;
	unsigned int d[8];
	memset( &d,0,sizeof(guid) );
	guid.Data1 = 0;
	guid.Data2 = 0;
	guid.Data3 = 0;
	sscanf( guidString,"{%8X-%4hX-%4hX-%2X%2X-%2X%2X%2X%2X%2X%2X%}",
		&guid.Data1,&guid.Data2,&guid.Data3, &d[0],&d[1],&d[2],&d[3],&d[4],&d[5],&d[6],&d[7] );
	guid.Data4[0] = d[0];
	guid.Data4[1] = d[1];
	guid.Data4[2] = d[2];
	guid.Data4[3] = d[3];
	guid.Data4[4] = d[4];
	guid.Data4[5] = d[5];
	guid.Data4[6] = d[6];
	guid.Data4[7] = d[7];

	return guid;
}

const char* GuidUtil::ToString( REFGUID guid )
{
	static char guidString[64];

	sprintf( guidString,"{%.8X-%.4X-%.4X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X%}",guid.Data1,guid.Data2,guid.Data3, guid.Data4[0],guid.Data4[1],
		guid.Data4[2],guid.Data4[3],guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7] );
	return guidString;
}

bool GuidUtil::IsEmpty( REFGUID guid )
{
	static GUID sNullGuid = { 0, 0, 0, { 0,0,0,0,0,0,0,0 } };
	return memcmp(&guid,&sNullGuid,sizeof(GUID)) == 0;
}
