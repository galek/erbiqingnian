#ifndef __CRC_H__
#define __CRC_H__


//----------------------------------------------------------------------------
// EXTERN
//----------------------------------------------------------------------------
extern DWORD gdwCrcTbl[256];


//----------------------------------------------------------------------------
// CRC
//----------------------------------------------------------------------------
void CRCInit(
	DWORD key);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline DWORD CRC(
	DWORD crc,
	const BYTE * buffer,
	int len)
{
	ASSERT_RETVAL(len >= 0, crc);
	ASSERT_RETVAL(buffer, crc);

	while (len--)
	{
		crc = (crc << 8) ^ gdwCrcTbl[(crc >> 24) ^ *buffer++];
	}
	return crc;
} 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
inline DWORD CRC(
	DWORD crc,
	const T & item)
{
	return CRC(crc, (const BYTE *)(&item), sizeof(item));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
DWORD CRC_STRING(
	DWORD crc,
	const T * buffer)
{
	ASSERT_RETVAL(buffer, crc);

	while (*buffer)
	{
		crc = CRC(crc, (const BYTE *)buffer, sizeof(T));
		++buffer;
	}
	return crc;
} 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T>
DWORD CRC_STRING(
	DWORD crc,
	const T * buffer,
	int len)
{
	ASSERT_RETVAL(len >= 0, crc);
	ASSERT_RETVAL(buffer, crc);

	const T * end = buffer + len;
	while (buffer < end && *buffer)
	{
		crc = CRC(crc, (const BYTE *)buffer, sizeof(T));
		++buffer;
	}
	return crc;
} 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct CRC_STATE
{
	char					filename[MAX_PATH];
	HANDLE					file;
	HANDLE					mapping;
	HANDLE					eDone;
	unsigned __int64 		bytes;
	DWORD 					checksum;
	volatile long 			progress;
	volatile long			done;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CRCFile(
	const char * filename,				// name of file (should be absolute path)
	unsigned __int64 & bytes,			// size of file (output)
	DWORD & checksum);					// 32 bit checksum (output)

BOOL CRCStart(
	CRC_STATE & state,
	const char * filename);

BOOL CRCCheckComplete(
	CRC_STATE & state,
	DWORD & progress,
	DWORD & checksum);


#endif