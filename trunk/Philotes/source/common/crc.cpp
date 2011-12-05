
#include "crc.h"
#include "debug.h"

//----------------------------------------------------------------------------
// CRC
//----------------------------------------------------------------------------
DWORD gdwCrcTbl[256];


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CRCInit(
	DWORD key)
{
	for (int ii = 0; ii < 256; ii++)
	{
		DWORD val = ii << 24;
		for (int jj = 0; jj < 8; jj++)
		{
			val = (val << 1) ^ ((val & (1 << 31)) ? key : 0);
		}
		gdwCrcTbl[ii] = val;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct AutoCloseHandle
{
	HANDLE m_Handle;

	AutoCloseHandle(
		void) : m_Handle(INVALID_HANDLE_VALUE) {};

	AutoCloseHandle(
		HANDLE handle) : m_Handle(handle) {};

	void SetHandle(
		HANDLE handle)
	{
		m_Handle = handle;
	}

	void Close(
		void)
	{
		if (m_Handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_Handle);
			m_Handle = INVALID_HANDLE_VALUE;
		}
	}

	~AutoCloseHandle(
		void)
	{
		Close();
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct AutoSetEvent
{
	HANDLE m_Event;

	AutoSetEvent(
		void) : m_Event(INVALID_HANDLE_VALUE) {};

	AutoSetEvent(
		HANDLE ev) : m_Event(ev) {};

	void SetHandle(
		HANDLE ev)
	{
		m_Event = ev;
	}

	void Close(
		void)
	{
		if (m_Event != INVALID_HANDLE_VALUE)
		{
			SetEvent(m_Event);
			m_Event = INVALID_HANDLE_VALUE;
		}
	}

	~AutoSetEvent(
		void)
	{
		Close();
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD Adler32(
	BYTE * buf,
	unsigned int len)
{
	static const unsigned int MOD_ADLER = 65521;
	DWORD a = 1;
	DWORD b = 0;

	while (len) 
	{
		DWORD tlen = len > 5550 ? 5550 : len;
		len -= tlen;
		do 
		{
			a += *buf++;
			b += a;
		} while (--tlen);
		a = (a & 0xffff) + (a >> 16) * (65536 - MOD_ADLER);
		b = (b & 0xffff) + (b >> 16) * (65536 - MOD_ADLER);
	}

	// it can be shown that a <= 0x1013a here, so a single subtract will do.
	if (a >= MOD_ADLER)
	{
		a -= MOD_ADLER;
	}

	// it can be shown that b can reach 0xffef1 here.
	b = (b & 0xffff) + (b >> 16) * (65536 - MOD_ADLER);
	if (b >= MOD_ADLER)
	{
		b -= MOD_ADLER;
	}
	return (b << 16) | a;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD WINAPI CRCThread(
	LPVOID param)
{
	static unsigned int CRC_BUFFER_SIZE	=					(64 * 1024 * 1024);

	CRC_STATE * state = (CRC_STATE *)param;
	ASSERT_RETZERO(state);

	AutoSetEvent aseDone(state->eDone);

	state->file = CreateFile(state->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	ASSERT_RETZERO(state->file != INVALID_HANDLE_VALUE);
	AutoCloseHandle achFile(state->file);

	LARGE_INTEGER filesize;
	ASSERT_RETZERO(GetFileSizeEx(state->file, &filesize));
	state->bytes = filesize.QuadPart;

	state->mapping = CreateFileMapping(state->file, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
	ASSERT_RETFALSE(state->mapping);
	AutoCloseHandle achMapping(state->mapping);

	unsigned __int64 len = state->bytes;
	unsigned __int64 offset = 0;
	while (len > 0)
	{
		DWORD maplen = (DWORD)min(CRC_BUFFER_SIZE, len);
		BYTE * buf = (BYTE *)MapViewOfFile(state->mapping, FILE_MAP_READ, (DWORD)(offset >> 32), (DWORD)(offset & 0xffffffff), maplen);
		ASSERT_RETFALSE(buf);

		state->checksum ^= Adler32(buf, maplen);

		UnmapViewOfFile(buf);
		len -= maplen;
		offset += maplen;

		state->progress = (DWORD)((state->bytes - len) * 100 / state->bytes);
	}

	state->done = TRUE;

	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CRCSetup(
	CRC_STATE & crc_state,
	const char * filename,				// name of file (should be absolute path)
	BOOL bCreateEvent)
{
	ASSERT_RETFALSE(filename);

	memset(&crc_state, 0, sizeof(crc_state));
	strcpy_s(crc_state.filename, sizeof(crc_state.filename) / sizeof(crc_state.filename[0]), filename);

	crc_state.file = INVALID_HANDLE_VALUE;
	crc_state.mapping = INVALID_HANDLE_VALUE;

	crc_state.eDone = INVALID_HANDLE_VALUE;
	if (bCreateEvent)
	{
		crc_state.eDone = CreateEvent(NULL, FALSE, FALSE, NULL);
		ASSERT_RETFALSE(crc_state.eDone != INVALID_HANDLE_VALUE);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// kinda fast stand-alone threaded file checksums
//----------------------------------------------------------------------------
BOOL CRCFile(
	const char * filename,				// name of file (should be absolute path)
	unsigned __int64 & bytes,			// size of file (output)
	DWORD & checksum)					// 32 bit checksum (output)
{
	CRC_STATE state;
	ASSERT_RETFALSE(CRCSetup(state, filename, TRUE));

	DWORD dwThreadId;
	HANDLE thread = CreateThread(NULL, 0, CRCThread, (LPVOID)&state, 0, &dwThreadId);
	CloseHandle(thread);

	ASSERT_RETFALSE(WaitForSingleObject(state.eDone, INFINITE) == WAIT_OBJECT_0);
	CloseHandle(state.eDone);
	bytes = state.bytes;
	checksum = state.checksum;

	return TRUE;
}


//----------------------------------------------------------------------------
// kinda fast stand-alone threaded file checksums
//----------------------------------------------------------------------------
BOOL CRCStart(
	CRC_STATE & state,
	const char * filename)
{
	ASSERT_RETFALSE(CRCSetup(state, filename, FALSE));

	DWORD dwThreadId;
	HANDLE thread = CreateThread(NULL, 0, CRCThread, (LPVOID)&state, 0, &dwThreadId);
	CloseHandle(thread);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CRCCheckComplete(
	CRC_STATE & state,
	DWORD & progress,
	DWORD & checksum)
{
	progress = state.progress;
	checksum = 0;
	if (state.done)
	{
		progress = 100;
		checksum = state.checksum;
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CRCThreadTest(
	const char * filename,
	unsigned __int64 & bytes,			// size of file (output)
	DWORD & checksum)					// 32 bit checksum (output)
{
	CRC_STATE state;		// if you're going to declare this on the stack, allocate it instead
	ASSERT_RETFALSE(CRCStart(state, filename));

	DWORD progress;
	while (!CRCCheckComplete(state, progress, checksum))
	{
		trace("CRCProgress: [%s]  %d%%\n", state.filename, progress);
		Sleep(1000);
	}

	bytes = state.bytes;

	return TRUE;
}