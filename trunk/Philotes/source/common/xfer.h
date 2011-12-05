//----------------------------------------------------------------------------
// FILE: xfer.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef _XFER_H_
#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _XFER_H_

#pragma warning(disable:4127)

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum XFER_MODE
{
	XFER_LOAD,
	XFER_SAVE,
};


//----------------------------------------------------------------------------
enum XFER_RESULT
{
	XFER_RESULT_OK,
	XFER_RESULT_ERROR,
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <XFER_MODE mode>
class XFER
{
private:
	BIT_BUF * m_Buf;

public:
	//------------------------------------------------------------------------
	XFER(
		BIT_BUF * buf) : m_Buf(buf)
	{
		ASSERT(buf);
	}

	//------------------------------------------------------------------------
	BIT_BUF * GetBuffer(
		void)
	{
		return m_Buf;
	}

	//------------------------------------------------------------------------
	void ZeroBuffer(
		void)
	{
		m_Buf->ZeroBuffer();
	}

	//------------------------------------------------------------------------
	BOOL IsSave(
		void) 
	{ 
		return mode == XFER_SAVE; 
	}

	//------------------------------------------------------------------------
	BOOL IsLoad(
		void) 
	{ 
		return mode == XFER_LOAD; 
	}

	//------------------------------------------------------------------------
	BOOL IsByteAligned(
		void)
	{
		return TRUE;
	}

	//------------------------------------------------------------------------
	unsigned int GetCursor(
		void)
	{
		return m_Buf->GetCursor();
	}

	//------------------------------------------------------------------------
	XFER_RESULT SetCursor(
		unsigned int pos)
	{
		if (m_Buf->SetCursor(pos) == FALSE)
		{
			return XFER_RESULT_ERROR;
		}
		return XFER_RESULT_OK;
	}

	//------------------------------------------------------------------------
	unsigned int GetLen( 
		void)
	{
		return m_Buf->GetLen();
	}

	//------------------------------------------------------------------------
	unsigned int GetAllocatedLen(
		void)
	{
		return m_Buf->GetAllocatedLen();
	}

	//------------------------------------------------------------------------
	BYTE * GetBufferPointer(
		void)
	{
		return m_Buf->GetPtr();
	}

	//------------------------------------------------------------------------
	void SetBuffer(
		BIT_BUF * buf)
	{ 
		ASSERT_RETURN(buf);
		m_Buf = buf; 
	}

	//------------------------------------------------------------------------
	template <typename T>
	XFER_RESULT XferInt(
		T & value, 
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		if (mode == XFER_SAVE)
		{
			if (m_Buf->PushInt(value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			if (m_Buf->PopInt(&value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		return XFER_RESULT_OK;
	}

	//------------------------------------------------------------------------
	template <typename T>
	XFER_RESULT XferUInt(
		T & value, 
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		if (mode == XFER_SAVE)
		{
			if (m_Buf->PushUInt(value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			if (m_Buf->PopUInt(&value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		return XFER_RESULT_OK;
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferDWord(
		DWORD & value, 
		unsigned int bits
		BBD_FILE_LINE_ARGS)
	{
		if (mode == XFER_SAVE)
		{
			if (m_Buf->PushUInt(value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			if (m_Buf->PopUInt(&value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		return XFER_RESULT_OK;
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferFloat(
		float & value
		BBD_FILE_LINE_ARGS)
	{
		static const unsigned int bits = sizeof(float) * BITS_PER_BYTE;
		return XferDWord(*(DWORD *)(&value), bits  BBD_FL);
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferVector(
		VECTOR & value
		BBD_FILE_LINE_ARGS)
	{
		if (XferFloat(value.fX  BBD_FL) == XFER_RESULT_ERROR) 
		{
			return XFER_RESULT_ERROR;
		}
		if (XferFloat(value.fY  BBD_FL) == XFER_RESULT_ERROR) 
		{
			return XFER_RESULT_ERROR;
		}
		if (XferFloat(value.fZ  BBD_FL) == XFER_RESULT_ERROR) 
		{
			return XFER_RESULT_ERROR;
		}
		return XFER_RESULT_OK;
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferNVector(
		VECTOR & value
		BBD_FILE_LINE_ARGS)
	{
		DWORD dwIsZero = 0;
		if(mode == XFER_SAVE)
		{
			if(fabs(value.fX) < EPSILON && fabs(value.fY) < EPSILON)
			{
				dwIsZero = 1;
			}
			else
			{
				dwIsZero = 0;
			}
		}
		XferUInt(dwIsZero, 1  BBD_FL);
		if(mode == XFER_LOAD && dwIsZero != 0)
		{
			value.fX = 0.0f;
			value.fY = 0.0f;
			value.fZ = 1.0f;
			return XFER_RESULT_OK;
		}

		if (XferFloat(value.fX  BBD_FL) == XFER_RESULT_ERROR) 
		{
			return XFER_RESULT_ERROR;
		}
		if (XferFloat(value.fY  BBD_FL) == XFER_RESULT_ERROR) 
		{
			return XFER_RESULT_ERROR;
		}
		DWORD dwSign = 0;
		if(mode == XFER_SAVE)
		{
			if(value.fZ < 0.0f)
			{
				dwSign = 1;
			}
		}
		XferUInt(dwSign, 1  BBD_FL);

		if(mode == XFER_LOAD)
		{
			ASSERTX_RETVAL(value.fX != 0.0f || value.fY != 0.0f, XFER_RESULT_ERROR, "XFer loaded zeroes for X and Y for a normalized vector");
			float fZValueSquared = 1.0f - value.fX * value.fX - value.fY * value.fY;
			if(fZValueSquared > 0.0f)
			{
				value.fZ = sqrtf(fZValueSquared);
			}
			else
			{
				value.fZ = 0.0f;
			}

			if(dwSign != 0)
			{
				value.fZ = -value.fZ;
			}
		}
		return XFER_RESULT_OK;
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferGUID(
		PGUID & value
		BBD_FILE_LINE_ARGS)
	{
		static const unsigned int bits = sizeof(PGUID) * BITS_PER_BYTE;

		if (mode == XFER_SAVE)
		{
			if (m_Buf->PushBuf((BYTE *)&value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			if (m_Buf->PopBuf((BYTE *)&value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		return XFER_RESULT_OK;
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferTIME(
		TIME & value
		BBD_FILE_LINE_ARGS)
	{
		static const unsigned int bits = sizeof(TIME) * BITS_PER_BYTE;

		if (mode == XFER_SAVE)
		{
			if (m_Buf->PushBuf((BYTE *)&value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			if (m_Buf->PopBuf((BYTE *)&value, bits  BBD_FL) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		return XFER_RESULT_OK;
	}
};


//----------------------------------------------------------------------------
template class XFER<XFER_LOAD>;
template class XFER<XFER_SAVE>;

#define XFER_IMPL(mode, funcname, ...)	\
	template PRESULT funcname<mode>( BYTE_XFER<mode> &, __VA_ARGS__ )
#define XFER_FUNC(funcname, ...) \
	template <XFER_MODE mode> PRESULT funcname( BYTE_XFER<mode> & tXfer, __VA_ARGS__ )
#define XFER_FUNC_PROTO(funcname, ...)	\
	XFER_FUNC(funcname, __VA_ARGS__); \
	XFER_IMPL(XFER_SAVE, funcname, __VA_ARGS__); \
	XFER_IMPL(XFER_LOAD, funcname, __VA_ARGS__)

//----------------------------------------------------------------------------
#if BIT_BUF_DEBUG
#define XFER_INTX(xfer, value, bits, file, line)			if (xfer.XferInt(value, bits, file, line) == XFER_RESULT_ERROR) return FALSE;
#define XFER_INT(xfer, value, bits)							if (xfer.XferInt(value, bits, __FILE__, __LINE__) == XFER_RESULT_ERROR) return FALSE;
#define XFER_INTX_GOTO(xfer, value, bits, error, file, line)if (xfer.XferInt(value, bits, file, line ) == XFER_RESULT_ERROR) goto error;
#define XFER_INT_GOTO(xfer, value, bits, error)				if (xfer.XferInt(value, bits, __FILE__, __LINE__) == XFER_RESULT_ERROR) goto error;

#define XFER_UINTX(xfer, value, bits, file, line)				if (xfer.XferUInt(value, bits, file, line) == XFER_RESULT_ERROR) return FALSE;
#define XFER_UINT(xfer, value, bits)							if (xfer.XferUInt(value, bits, __FILE__, __LINE__) == XFER_RESULT_ERROR) return FALSE;
#define XFER_UINTX_GOTO(xfer, value, bits, error, file, line)	if (xfer.XferUInt(value, bits, file, line) == XFER_RESULT_ERROR) goto error;
#define XFER_UINT_GOTO(xfer, value, bits, error)				if (xfer.XferUInt(value, bits, __FILE__, __LINE__) == XFER_RESULT_ERROR) goto error;

#define XFER_DWORDX(xfer, value, bits, file, line)				if (xfer.XferDWord(value, bits, file, line) == XFER_RESULT_ERROR) return FALSE;
#define XFER_DWORD(xfer, value, bits)							if (xfer.XferDWord(value, bits, __FILE__, __LINE__) == XFER_RESULT_ERROR) return FALSE;
#define XFER_DWORDX_GOTO(xfer, value, bits, error, file, line)	if (xfer.XferDWord(value, bits, file, line ) == XFER_RESULT_ERROR) goto error;
#define XFER_DWORD_GOTO(xfer, value, bits, error)				if (xfer.XferDWord(value, bits, __FILE__, __LINE__) == XFER_RESULT_ERROR) goto error;

#define XFER_FLOATX(xfer, value, file, line)				if (xfer.XferFloat(value, file, line) == XFER_RESULT_ERROR) return FALSE;
#define XFER_FLOAT(xfer, value)								if (xfer.XferFloat(value, __FILE__, __LINE__) == XFER_RESULT_ERROR) return FALSE;
#define XFER_FLOATX_GOTO(xfer, value, error, file, line)	if (xfer.XferFloat(value, file, line ) == XFER_RESULT_ERROR) goto error;
#define XFER_FLOAT_GOTO(xfer, value, error)					if (xfer.XferFloat(value, __FILE__, __LINE__) == XFER_RESULT_ERROR) goto error;

#define XFER_VECTORX(xfer, value, file, line)				if (xfer.XferVector(value, file, line) == XFER_RESULT_ERROR) return FALSE;
#define XFER_VECTOR(xfer, value)							if (xfer.XferVector(value, __FILE__, __LINE__) == XFER_RESULT_ERROR) return FALSE;
#define XFER_VECTORX_GOTO(xfer, value, error, file, line)	if (xfer.XferVector(value, file, line ) == XFER_RESULT_ERROR) goto error;
#define XFER_VECTOR_GOTO(xfer, value, error)				if (xfer.XferVector(value, __FILE__, __LINE__) == XFER_RESULT_ERROR) goto error;

#define XFER_GUIDX(xfer, value, file, line)					if (xfer.XferGUID(value, file, line) == XFER_RESULT_ERROR) return FALSE;
#define XFER_GUID(xfer, value)								if (xfer.XferGUID(value, __FILE__, __LINE__) == XFER_RESULT_ERROR) return FALSE;
#define XFER_GUIDX_GOTO(xfer, value, error, file, line)		if (xfer.XferGUID(value, file, line ) == XFER_RESULT_ERROR) goto error;
#define XFER_GUID_GOTO(xfer, value, error)					if (xfer.XferGUID(value, __FILE__, __LINE__) == XFER_RESULT_ERROR) goto error;

#define XFER_TIMEX(xfer, value, file, line)					if (xfer.XferTIME(value, file, line) == XFER_RESULT_ERROR) return FALSE;
#define XFER_TIME(xfer, value)								if (xfer.XferTIME(value, __FILE__, __LINE__) == XFER_RESULT_ERROR) return FALSE;
#define XFER_TIMEX_GOTO(xfer, value, error, file, line)		if (xfer.XferTIME(value, file, line ) == XFER_RESULT_ERROR) goto error;
#define XFER_TIME_GOTO(xfer, value, error)					if (xfer.XferTIME(value, __FILE__, __LINE__) == XFER_RESULT_ERROR) goto error;

#define XFER_BOOLX(xfer, value, file, line)					XFER_UINTX(xfer, (unsigned int &)value, 1, file, line)
#define XFER_BOOL(xfer, value)								XFER_UINT(xfer, (unsigned int &)value, 1)
#define XFER_BOOLX_GOTO(xfer, value, error, file, line)		XFER_UINTX_GOTO(xfer, (unsigned int &)value, 1, error, file, line)
#define XFER_BOOL_GOTO(xfer, value, error)					XFER_UINT_GOTO(xfer, (unsigned int &)value, 1, error)

#define XFER_UNITIDX(xfer, id, file, line)					XFER_UINTX(xfer, (unsigned int &)id, STREAM_BITS_UNITID, file, line)
#define XFER_UNITID(xfer, id)								XFER_UINT(xfer, (unsigned int &)id, STREAM_BITS_UNITID)
#define XFER_UNITID_GOTO(xfer, id, error)					XFER_UINT_GOTO(xfer, (unsigned int &)id, STREAM_BITS_UNITID, error)

#define XFER_ROOMIDX(xfer, id, file, line)					XFER_UINTX(xfer, (unsigned int &)id, STREAM_BITS_ROOMID, file, line)
#define XFER_ROOMID(xfer, id)								XFER_UINT(xfer, (unsigned int &)id, STREAM_BITS_ROOMID)
#define XFER_ROOMIDX_GOTO(xfer, id, error, file, line)		XFER_UINT_GOTO(xfer, (unsigned int &)id, STREAM_BITS_ROOMID, error, file, line)
#define XFER_ROOMID_GOTO(xfer, id, error)					XFER_UINT_GOTO(xfer, (unsigned int &)id, STREAM_BITS_ROOMID, error)

#define XFER_ENUMX(xfer, value, bits, file, line)				XFER_UINTX(xfer, (unsigned int &)value, bits, file, line)
#define XFER_ENUM(xfer, value, bits)							XFER_UINT(xfer, (unsigned int &)value, bits)
#define XFER_ENUMX_GOTO(xfer, value, bits, error, file, line)	XFER_UINT_GOTO(xfer, (unsigned int &)value, bits, error, file, line)
#define XFER_ENUM_GOTO(xfer, value, bits, error)				XFER_UINT_GOTO(xfer, (unsigned int &)value, bits, error)

#else

#define XFER_INTX(xfer, value, bits, file, line)			if (xfer.XferInt(value, bits) == XFER_RESULT_ERROR) return FALSE;
#define XFER_INT(xfer, value, bits)							if (xfer.XferInt(value, bits) == XFER_RESULT_ERROR) return FALSE;
#define XFER_INTX_GOTO(xfer, value, bits, error, file, line)if (xfer.XferInt(value, bits) == XFER_RESULT_ERROR) goto error;
#define XFER_INT_GOTO(xfer, value, bits, error)				if (xfer.XferInt(value, bits) == XFER_RESULT_ERROR) goto error;

#define XFER_UINTX(xfer, value, bits, file, line)			 if (xfer.XferUInt(value, bits) == XFER_RESULT_ERROR) return FALSE;
#define XFER_UINT(xfer, value, bits)						 if (xfer.XferUInt(value, bits) == XFER_RESULT_ERROR) return FALSE;
#define XFER_UINTX_GOTO(xfer, value, bits, error, file, line)if (xfer.XferUInt(value, bits) == XFER_RESULT_ERROR) goto error;
#define XFER_UINT_GOTO(xfer, value, bits, error)			 if (xfer.XferUInt(value, bits) == XFER_RESULT_ERROR) goto error;

#define XFER_DWORDX(xfer, value, bits, file, line)				if (xfer.XferDWord(value, bits) == XFER_RESULT_ERROR) return FALSE;
#define XFER_DWORD(xfer, value, bits)							if (xfer.XferDWord(value, bits) == XFER_RESULT_ERROR) return FALSE;
#define XFER_DWORDX_GOTO(xfer, value, bits, error, file, line)	if (xfer.XferDWord(value, bits) == XFER_RESULT_ERROR) goto error;
#define XFER_DWORD_GOTO(xfer, value, bits, error)				if (xfer.XferDWord(value, bits) == XFER_RESULT_ERROR) goto error;

#define XFER_FLOATX(xfer, value, file, line)				if (xfer.XferFloat(value) == XFER_RESULT_ERROR) return FALSE;
#define XFER_FLOAT(xfer, value)								if (xfer.XferFloat(value) == XFER_RESULT_ERROR) return FALSE;
#define XFER_FLOATX_GOTO(xfer, value, error, file, line)	if (xfer.XferFloat(value) == XFER_RESULT_ERROR) goto error;
#define XFER_FLOAT_GOTO(xfer, value, error)					if (xfer.XferFloat(value) == XFER_RESULT_ERROR) goto error;

#define XFER_VECTORX(xfer, value, file, line)				if (xfer.XferVector(value) == XFER_RESULT_ERROR) return FALSE;
#define XFER_VECTOR(xfer, value)							if (xfer.XferVector(value) == XFER_RESULT_ERROR) return FALSE;
#define XFER_VECTORX_GOTO(xfer, value, error, file, line)	if (xfer.XferVector(value) == XFER_RESULT_ERROR) goto error;
#define XFER_VECTOR_GOTO(xfer, value, error)				if (xfer.XferVector(value) == XFER_RESULT_ERROR) goto error;

#define XFER_GUIDX(xfer, value, file, line)					if (xfer.XferGUID(value) == XFER_RESULT_ERROR) return FALSE;
#define XFER_GUID(xfer, value)								if (xfer.XferGUID(value) == XFER_RESULT_ERROR) return FALSE;
#define XFER_GUIDX_GOTO(xfer, value, error, file, line)		if (xfer.XferGUID(value) == XFER_RESULT_ERROR) goto error;
#define XFER_GUID_GOTO(xfer, value, error)					if (xfer.XferGUID(value) == XFER_RESULT_ERROR) goto error;

#define XFER_TIMEX(xfer, value, file, line)					if (xfer.XferTIME(value) == XFER_RESULT_ERROR) return FALSE;
#define XFER_TIME(xfer, value)								if (xfer.XferTIME(value) == XFER_RESULT_ERROR) return FALSE;
#define XFER_TIMEX_GOTO(xfer, value, error, file, line)		if (xfer.XferTIME(value) == XFER_RESULT_ERROR) goto error;
#define XFER_TIME_GOTO(xfer, value, error)					if (xfer.XferTIME(value) == XFER_RESULT_ERROR) goto error;

#define XFER_BOOLX(xfer, value, file, line)					XFER_UINTX(xfer, (unsigned int &)value, 1, file, line)
#define XFER_BOOL(xfer, value)								XFER_UINT(xfer, (unsigned int &)value, 1)
#define XFER_BOOLX_GOTO(xfer, value, error, file, line)		XFER_UINT_GOTO(xfer, (unsigned int &)value, 1, error)
#define XFER_BOOL_GOTO(xfer, value, error)					XFER_UINT_GOTO(xfer, (unsigned int &)value, 1, error)

#define XFER_UNITIDX(xfer, id, file, line)					XFER_UINTX(xfer, (unsigned int &)id, STREAM_BITS_UNITID, file, line)
#define XFER_UNITID(xfer, id)								XFER_UINT(xfer, (unsigned int &)id, STREAM_BITS_UNITID)
#define XFER_UNITIDX_GOTO(xfer, id, error, file, line)		XFER_UINT_GOTO(xfer, (unsigned int &)id, STREAM_BITS_UNITID, error)
#define XFER_UNITID_GOTO(xfer, id, error)					XFER_UINT_GOTO(xfer, (unsigned int &)id, STREAM_BITS_UNITID, error)

#define XFER_ROOMIDX(xfer, id, file, line)					XFER_UINTX(xfer, (unsigned int &)id, STREAM_BITS_ROOMID, file, line)
#define XFER_ROOMID(xfer, id)								XFER_UINT(xfer, (unsigned int &)id, STREAM_BITS_ROOMID)
#define XFER_ROOMIDX_GOTO(xfer, id, error, file, line)		XFER_UINT_GOTO(xfer, (unsigned int &)id, STREAM_BITS_ROOMID, error)
#define XFER_ROOMID_GOTO(xfer, id, error)					XFER_UINT_GOTO(xfer, (unsigned int &)id, STREAM_BITS_ROOMID, error)

#define XFER_ENUMX(xfer, value, bits, file, line)				XFER_UINTX(xfer, (unsigned int &)value, bits, file, line)
#define XFER_ENUM(xfer, value, bits)							XFER_UINT(xfer, (unsigned int &)value, bits)
#define XFER_ENUMX_GOTO(xfer, value, bits, error, file, line)	XFER_UINT_GOTO(xfer, (unsigned int &)value, bits, error)
#define XFER_ENUM_GOTO(xfer, value, bits, error)				XFER_UINT_GOTO(xfer, (unsigned int &)value, bits, error)

#endif

#define XFER_VERSION(xfer, version)							unsigned int nCurrentVersionCheck = version;	\
															XFER_UINT(xfer, version, STREAM_VERSION_BITS);	\
															ASSERTX_RETFALSE((unsigned int)version <= nCurrentVersionCheck, "Invalid future version encountered");


template <XFER_MODE mode>
class BYTE_XFER
{
private:
	BYTE_BUF * m_Buf;

public:
	//------------------------------------------------------------------------
	BYTE_XFER(
		BYTE_BUF * buf) : m_Buf(buf)
	{
		ASSERT(buf);
	}

	//------------------------------------------------------------------------
	BYTE_BUF * GetBuffer(
		void)
	{
		return m_Buf;
	}

	//------------------------------------------------------------------------
	BOOL IsSave(
		void) 
	{ 
		return (mode == XFER_SAVE); 
	}

	//------------------------------------------------------------------------
	BOOL IsLoad( 
		void) 
	{ 
		return (mode == XFER_LOAD);
	}

	//------------------------------------------------------------------------
	int GetCursor( 
		void)
	{
		return m_Buf->GetCursor();
	}

	//------------------------------------------------------------------------
	XFER_RESULT SetCursor(
		unsigned int pos)
	{
		m_Buf->SetCursor(pos);
		return XFER_RESULT_OK;
	}

	//------------------------------------------------------------------------
	unsigned int GetLen(
		void)
	{
		return m_Buf->GetLen();
	}

	//------------------------------------------------------------------------
	unsigned int GetAllocatedLen(
		void)
	{
		return m_Buf->GetAllocatedLen();
	}

	//------------------------------------------------------------------------
	BYTE * GetBufferPointer(
		void)
	{
		return m_Buf->GetPtr();
	}

	//------------------------------------------------------------------------
	void SetBuffer(
		BYTE_BUF * buf) 
	{ 
		ASSERT_RETURN(buf);
		m_Buf = buf; 
	}

	//------------------------------------------------------------------------
	BOOL ZeroBuffer( 
		void)
	{
		m_Buf->ZeroBuffer();
		return TRUE;
	}	

	//------------------------------------------------------------------------
	XFER_RESULT XferInt(
		int & value)
	{
		if (mode == XFER_SAVE)
		{
			if (m_Buf->PushInt(value) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			if (m_Buf->PopInt(&value) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		return XFER_RESULT_OK;		
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferUInt(
		unsigned int & value)
	{
		if (mode == XFER_SAVE)
		{
			if (m_Buf->PushUInt(value) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			if (m_Buf->PopUInt(&value) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		return XFER_RESULT_OK;		
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferBytes(
		void * buf, 
		int bytes)
	{
		if (mode == XFER_SAVE)
		{
			if (m_Buf->PushBuf(buf, bytes) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			if (m_Buf->PopBuf(buf, bytes) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		return XFER_RESULT_OK;		
	}

	//------------------------------------------------------------------------
	XFER_RESULT XferString(
		void * buf, 
		int nMaxStringLength )
	{
		if (mode == XFER_SAVE)
		{
			UINT nLength = PStrLen( (char *) buf, nMaxStringLength );
			if (m_Buf->PushUInt(nLength) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
			if (m_Buf->PushBuf(buf, nLength) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
		}
		else
		{
			UINT nLength;
			if (m_Buf->PopUInt(&nLength) == FALSE)
			{
				return XFER_RESULT_ERROR;
			}
			if ( nLength > (UINT)nMaxStringLength )
			{
				return XFER_RESULT_ERROR;
			}
			if ( nLength )
			{
				if (m_Buf->PopBuf(buf, nLength) == FALSE)
				{
					return XFER_RESULT_ERROR;
				}
			}
			((char*)buf)[ nLength ] = 0;
		}
		return XFER_RESULT_OK;		
	}
};


#define BYTE_XFER_ELEMENT(xfer, elem)						if ((xfer).XferBytes(&elem, sizeof(elem)) == XFER_RESULT_ERROR) return E_FAIL
#define BYTE_XFER_ELEMENTX(xfer, elem, bytes)				if ((xfer).XferBytes(&elem, bytes) == XFER_RESULT_ERROR) return E_FAIL
#define BYTE_XFER_STRING(xfer, elem, maxsize)				if ((xfer).XferString(&elem, maxsize) == XFER_RESULT_ERROR) return E_FAIL
#define BYTE_XFER_POINTER(xfer, ptr, bytes)					if ((xfer).XferBytes(ptr, bytes) == XFER_RESULT_ERROR) return E_FAIL


#endif
