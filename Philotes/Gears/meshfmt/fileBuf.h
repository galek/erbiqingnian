
#pragma once

_NAMESPACE_BEGIN

class PxFileBuf
{
public:

	enum EndianMode
	{
		ENDIAN_NONE		= 0, // do no conversion for endian mode
		ENDIAN_BIG		= 1, // always read/write data as natively big endian (Power PC, etc.)
		ENDIAN_LITTLE	= 2, // always read/write data as natively little endian (Intel, etc.) Default Behavior!
	};

	PxFileBuf(EndianMode mode=ENDIAN_LITTLE)
	{
		setEndianMode(mode);
	}

	virtual ~PxFileBuf(void)
	{

	}

	/**
	\brief Declares a constant to seek to the end of the stream.
	*
	* Does not support streams longer than 32 bits
	*/
	static const uint32 STREAM_SEEK_END=0xFFFFFFFF;

	enum OpenMode
	{
		OPEN_FILE_NOT_FOUND,
		OPEN_READ_ONLY,					// open file buffer stream for read only access
		OPEN_WRITE_ONLY,				// open file buffer stream for write only access
		OPEN_READ_WRITE_NEW,			// open a new file for both read/write access
		OPEN_READ_WRITE_EXISTING		// open an existing file for both read/write access
	};

	virtual OpenMode	getOpenMode(void) const  = 0;

	bool isOpen(void) const
	{
		return getOpenMode()!=OPEN_FILE_NOT_FOUND;
	}

	enum SeekType
	{
		SEEKABLE_NO			= 0,
		SEEKABLE_READ		= 0x1,
		SEEKABLE_WRITE		= 0x2,
		SEEKABLE_READWRITE 	= 0x3
	};

	virtual SeekType isSeekable(void) const = 0;

	void	setEndianMode(EndianMode e)
	{
		mEndianMode = e;
		if ( (e==ENDIAN_BIG && !isBigEndian() ) ||
			 (e==ENDIAN_LITTLE && isBigEndian() ) )
		{
			mEndianSwap = true;
		}
 		else
		{
			mEndianSwap = false;
		}
	}

	EndianMode	getEndianMode(void) const
	{
		return mEndianMode;
	}

	virtual uint32 getFileLength(void) const = 0;

	/**
	\brief Seeks the stream to a particular location for reading
	*
	* If the location passed exceeds the length of the stream, then it will seek to the end.
	* Returns the location it ended up at (useful if you seek to the end) to get the file position
	*/
	virtual uint32	seekRead(uint32 loc) = 0;

	/**
	\brief Seeks the stream to a particular location for writing
	*
	* If the location passed exceeds the length of the stream, then it will seek to the end.
	* Returns the location it ended up at (useful if you seek to the end) to get the file position
	*/
	virtual uint32	seekWrite(uint32 loc) = 0;

	/**
	\brief Reads from the stream into a buffer.

	\param[out] mem  The buffer to read the stream into.
	\param[in]  len  The number of bytes to stream into the buffer

	\return Returns the actual number of bytes read.  If not equal to the length requested, then reached end of stream.
	*/
	virtual uint32	read(void *mem,uint32 len) = 0;


	/**
	\brief Reads from the stream into a buffer but does not advance the read location.

	\param[out] mem  The buffer to read the stream into.
	\param[in]  len  The number of bytes to stream into the buffer

	\return Returns the actual number of bytes read.  If not equal to the length requested, then reached end of stream.
	*/
	virtual uint32	peek(void *mem,uint32 len) = 0;

	/**
	\brief Writes a buffer of memory to the stream

	\param[in] mem The address of a buffer of memory to send to the stream.
	\param[in] len  The number of bytes to send to the stream.

	\return Returns the actual number of bytes sent to the stream.  If not equal to the length specific, then the stream is full or unable to write for some reason.
	*/
	virtual uint32	write(const void *mem,uint32 len) = 0;

	/**
	\brief Reports the current stream location read aqccess.

	\return Returns the current stream read location.
	*/
	virtual uint32	tellRead(void) const = 0;

	/**
	\brief Reports the current stream location for write access.

	\return Returns the current stream write location.
	*/
	virtual uint32	tellWrite(void) const = 0;

	/**
	\brief	Causes any temporarily cached data to be flushed to the stream.
	*/
	virtual	void	flush(void) = 0;

	/**
	\brief	Close the stream.
	*/
	virtual void close(void) {}

	void release(void)
	{
		delete this;
	}

    static inline bool isBigEndian()
     {
       int i = 1;
        return *((char*)&i)==0;
    }

    inline void swap2Bytes(void* _data) const
    {
		char *data = (char *)_data;
		char one_byte;
		one_byte = data[0]; data[0] = data[1]; data[1] = one_byte;
    }

    inline void swap4Bytes(void* _data) const
    {
		char *data = (char *)_data;
		char one_byte;
		one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
		one_byte = data[1]; data[1] = data[2]; data[2] = one_byte;
    }

    inline void swap8Bytes(void *_data) const
    {
		char *data = (char *)_data;
		char one_byte;
		one_byte = data[0]; data[0] = data[7]; data[7] = one_byte;
		one_byte = data[1]; data[1] = data[6]; data[6] = one_byte;
		one_byte = data[2]; data[2] = data[5]; data[5] = one_byte;
		one_byte = data[3]; data[3] = data[4]; data[4] = one_byte;
    }


	inline void storeDword(uint32 v)
	{
		if ( mEndianSwap )
		    swap4Bytes(&v);

		write(&v,sizeof(v));
	}

	inline void storeFloat(scalar v)
	{
		if ( mEndianSwap )
			swap4Bytes(&v);
		write(&v,sizeof(v));
	}

	inline void storeDouble(double v)
	{
		if ( mEndianSwap )
			swap8Bytes(&v);
		write(&v,sizeof(v));
	}

	inline  void storeByte(uint8 b)
	{
		write(&b,sizeof(b));
	}

	inline void storeWord(uint16 w)
	{
		if ( mEndianSwap )
			swap2Bytes(&w);
		write(&w,sizeof(w));
	}

	uint8 readByte(void) 
	{
		uint8 v=0;
		read(&v,sizeof(v));
		return v;
	}

	uint16 readWord(void) 
	{
		uint16 v=0;
		read(&v,sizeof(v));
		if ( mEndianSwap )
		    swap2Bytes(&v);
		return v;
	}

	uint32 readDword(void) 
	{
		uint32 v=0;
		read(&v,sizeof(v));
		if ( mEndianSwap )
		    swap4Bytes(&v);
		return v;
	}

	scalar readFloat(void) 
	{
		scalar v=0;
		read(&v,sizeof(v));
		if ( mEndianSwap )
		    swap4Bytes(&v);
		return v;
	}

	double readDouble(void) 
	{
		double v=0;
		read(&v,sizeof(v));
		if ( mEndianSwap )
		    swap8Bytes(&v);
		return v;
	}

private:
	bool		mEndianSwap;	// whether or not the endian should be swapped on the current platform
	EndianMode	mEndianMode;  	// the current endian mode behavior for the stream
};

//////////////////////////////////////////////////////////////////////////

class PxMemoryBufferBase : public PxFileBuf
{
	void init(const void *readMem, uint32 readLen)
	{
		mReadBuffer = mReadLoc = (const uint8 *)readMem;
		mReadStop   = &mReadLoc[readLen];

		mWriteBuffer = mWriteLoc = mWriteStop = NULL;
		mWriteBufferSize = 0;
		mDefaultWriteBufferSize = 4096;

		mOpenMode = OPEN_READ_ONLY;
		mSeekType = SEEKABLE_READ;
	}

	void init(uint32 defaultWriteBufferSize)
	{
		mReadBuffer = mReadLoc = mReadStop = NULL;

		mWriteBuffer = mWriteLoc = mWriteStop = NULL;
		mWriteBufferSize = 0;
		mDefaultWriteBufferSize = defaultWriteBufferSize;

		mOpenMode = OPEN_READ_WRITE_NEW;
		mSeekType = SEEKABLE_READWRITE;
	}

public:
	PxMemoryBufferBase(const void *readMem,uint32 readLen)
	{
		init(readMem, readLen);
    }

	virtual ~PxMemoryBufferBase(void)
	{
		reset();
	}

	void initWriteBuffer(uint32 size)
	{
		if ( mWriteBuffer == NULL )
		{
			if ( size < mDefaultWriteBufferSize ) size = mDefaultWriteBufferSize;
			mWriteBuffer = (uint8 *)malloc(size);
			ph_assert( mWriteBuffer );
    		mWriteLoc    = mWriteBuffer;
    		mWriteStop	= &mWriteBuffer[size];
    		mWriteBufferSize = size;
    		mReadBuffer = mWriteBuffer;
    		mReadStop	= mWriteBuffer;
    		mReadLoc    = mWriteBuffer;
		}
    }

	void reset(void)
	{
		free(mWriteBuffer);
		mWriteBuffer = NULL;
		mWriteBufferSize = 0;
		mWriteLoc = NULL;
		mReadBuffer = NULL;
		mReadStop = NULL;
		mReadLoc = NULL;
    }

	virtual OpenMode	getOpenMode(void) const
	{
		return mOpenMode;
	}


	SeekType isSeekable(void) const
	{
		return mSeekType;
	}

	virtual		uint32			read(void* buffer, uint32 size)
	{
		if ( (mReadLoc+size) > mReadStop )
		{
			size = (uint32)(mReadStop - mReadLoc);
		}
		if ( size != 0 )
		{
			memmove(buffer,mReadLoc,size);
			mReadLoc+=size;
		}
		return size;
	}

	virtual		uint32			peek(void* buffer, uint32 size)
	{
		if ( (mReadLoc+size) > mReadStop )
		{
			size = (uint32)(mReadStop - mReadLoc);
		}
		if ( size != 0 )
		{
			memmove(buffer,mReadLoc,size);
		}
		return size;
	}

	virtual		uint32		write(const void* buffer, uint32 size)
	{
		ph_assert( mOpenMode ==	OPEN_READ_WRITE_NEW );
		if ( mOpenMode == OPEN_READ_WRITE_NEW )
		{
    		if ( (mWriteLoc+size) > mWriteStop )
    		    growWriteBuffer(size);
    		memmove(mWriteLoc,buffer,size);
    		mWriteLoc+=size;
    		mReadStop = mWriteLoc;
    	}
    	else
    	{
    		size = 0;
    	}
		return size;
	}

	inline const uint8 * getReadLoc(void) const { return mReadLoc; };
	inline void advanceReadLoc(uint32 len)
	{
		ph_assert(mReadBuffer);
		if ( mReadBuffer )
		{
			mReadLoc+=len;
			if ( mReadLoc >= mReadStop )
			{
				mReadLoc = mReadStop;
			}
		}
	}

	virtual uint32 tellRead(void) const
	{
		uint32 ret=0;

		if ( mReadBuffer )
		{
			ret = (uint32) (mReadLoc-mReadBuffer);
		}
		return ret;
	}

	virtual uint32 tellWrite(void) const
	{
		return (uint32)(mWriteLoc-mWriteBuffer);
	}

	virtual uint32 seekRead(uint32 loc)
	{
		uint32 ret = 0;
		ph_assert(mReadBuffer);
		if ( mReadBuffer )
		{
			mReadLoc = &mReadBuffer[loc];
			if ( mReadLoc >= mReadStop )
			{
				mReadLoc = mReadStop;
			}
			ret = (uint32) (mReadLoc-mReadBuffer);
		}
		return ret;
	}

	virtual uint32 seekWrite(uint32 loc)
	{
		uint32 ret = 0;
		ph_assert( mOpenMode ==	OPEN_READ_WRITE_NEW );
		if ( mWriteBuffer )
		{
    		mWriteLoc = &mWriteBuffer[loc];
    		if ( mWriteLoc >= mWriteStop )
    		{
				mWriteLoc = mWriteStop;
			}
			ret = (uint32)(mWriteLoc-mWriteBuffer);
		}
		return ret;
	}

	virtual void flush(void)
	{

	}

	virtual uint32 getFileLength(void) const
	{
		uint32 ret = 0;
		if ( mReadBuffer )
		{
			ret = (uint32) (mReadStop-mReadBuffer);
		}
		else if ( mWriteBuffer )
		{
			ret = (uint32)(mWriteLoc-mWriteBuffer);
		}
		return ret;
	}

	uint32	getWriteBufferSize(void) const
	{
		return (uint32)(mWriteLoc-mWriteBuffer);
	}

	void setWriteLoc(uint8 *writeLoc)
	{
		ph_assert(writeLoc >= mWriteBuffer && writeLoc < mWriteStop );
		mWriteLoc = writeLoc;
		mReadStop = mWriteLoc;
	}

	const uint8 * getWriteBuffer(void) const
	{
		return mWriteBuffer;
	}

	/**
	 * Attention: if you use aligned allocator you cannot free memory with PX_FREE macros instead use deallocate method from base
	 */
	uint8 * getWriteBufferOwnership(uint32 &dataLen) // return the write buffer, and zero it out, the caller is taking ownership of the memory
	{
		uint8 *ret = mWriteBuffer;
		dataLen = (uint32)(mWriteLoc-mWriteBuffer);
		mWriteBuffer = NULL;
		mWriteLoc = NULL;
		mWriteStop = NULL;
		mWriteBufferSize = 0;
		return ret;
	}


	void alignRead(uint32 a)
	{
		uint32 loc = tellRead();
		uint32 aloc = ((loc+(a-1))/a)*a;
		if ( aloc != loc )
		{
			seekRead(aloc);
		}
	}

	void alignWrite(uint32 a)
	{
		uint32 loc = tellWrite();
		uint32 aloc = ((loc+(a-1))/a)*a;
		if ( aloc != loc )
		{
			seekWrite(aloc);
		}
	}

private:

	// double the size of the write buffer or at least as large as the 'size' value passed in.
	void growWriteBuffer(uint32 size)
	{
		if ( mWriteBuffer == NULL )
		{
			if ( size < mDefaultWriteBufferSize ) size = mDefaultWriteBufferSize;
			initWriteBuffer(size);
		}
		else
		{
			uint32 oldWriteIndex = (uint32) (mWriteLoc - mWriteBuffer);
			uint32 newSize =	mWriteBufferSize*2;
			uint32 avail = newSize-oldWriteIndex;
			if ( size >= avail ) newSize = newSize+size;
			uint8 *writeBuffer = (uint8 *)malloc(newSize);
			ph_assert( writeBuffer );
			memmove(writeBuffer,mWriteBuffer,mWriteBufferSize);
			free(mWriteBuffer);
			mWriteBuffer = writeBuffer;
			mWriteBufferSize = newSize;
			mWriteLoc = &mWriteBuffer[oldWriteIndex];
			mWriteStop = &mWriteBuffer[mWriteBufferSize];
			uint32 oldReadLoc = (uint32)(mReadLoc-mReadBuffer);
			mReadBuffer = mWriteBuffer;
			mReadStop   = mWriteLoc;
			mReadLoc = &mReadBuffer[oldReadLoc];
		}
	}

	const	uint8	*mReadBuffer;
	const	uint8	*mReadLoc;
	const	uint8	*mReadStop;

			uint8	*mWriteBuffer;
			uint8	*mWriteLoc;
			uint8	*mWriteStop;

			uint32	mWriteBufferSize;
			uint32	mDefaultWriteBufferSize;
			OpenMode	mOpenMode;
			SeekType	mSeekType;

};

//Use this class if you want to use PhysX memory allocator
class PxMemoryBuffer: public PxMemoryBufferBase
{
	typedef PxMemoryBufferBase BaseClass;

public:
	PxMemoryBuffer(const void *readMem,uint32 readLen): BaseClass(readMem, readLen) {}	
};

_NAMESPACE_END