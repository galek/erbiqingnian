
#include "iniFileInterface.h"

namespace Philo
{
	IniFileInterface::IniFileInterface( const char *fname,const char *spec,void *mem,size_t len )
	{
		mMyAlloc = false;
		mRead = true; // default is read access.
		mFph = 0;
		mData = (char *) mem;
		mLen  = len;
		mLoc  = 0;

		if ( spec && stricmp(spec,"wmem") == 0 )
		{
			mRead = false;
			if ( mem == 0 || len == 0 )
			{
				mData = (char *)malloc(DEFAULT_BUFFER_SIZE);
				mLen  = DEFAULT_BUFFER_SIZE;
				mMyAlloc = true;
			}
		}

		if ( mData == 0 )
		{
			mFph = fopen(fname,spec);
		}

		strncpy(mName,fname,512);
	}

	IniFileInterface::~IniFileInterface( void )
	{
		if ( mMyAlloc )
		{
			free(mData);
		}
		if ( mFph )
		{
			fclose(mFph);
		}
	}

	size_t IniFileInterface::read( char *data,size_t size )
	{
		size_t ret = 0;
		if ( (mLoc+size) <= mLen )
		{
			memcpy(data, &mData[mLoc], size );
			mLoc+=size;
			ret = 1;
		}
		return ret;
	}

	size_t IniFileInterface::read( void *buffer,size_t size,size_t count )
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = fread(buffer,size,count,mFph);
		}
		else
		{
			char *data = (char *)buffer;
			for (size_t i=0; i<count; i++)
			{
				if ( (mLoc+size) <= mLen )
				{
					read(data,size);
					data+=size;
					ret++;
				}
				else
				{
					break;
				}
			}
		}
		return ret;
	}

	size_t IniFileInterface::write( const char *data,size_t size )
	{
		size_t ret = 0;

		if ( (mLoc+size) >= mLen && mMyAlloc ) // grow it
		{
			size_t newLen = mLen+DEFAULT_GROW_SIZE;
			if ( size > newLen ) newLen = size+DEFAULT_GROW_SIZE;

			char *data = (char *)malloc(newLen);
			memcpy(data,mData,mLoc);
			free(mData);
			mData = data;
			mLen  = newLen;
		}

		if ( (mLoc+size) <= mLen )
		{
			memcpy(&mData[mLoc],data,size);
			mLoc+=size;
			ret = 1;
		}
		return ret;
	}

	size_t IniFileInterface::write( const void *buffer,size_t size,size_t count )
	{
		size_t ret = 0;

		if ( mFph )
		{
			ret = fwrite(buffer,size,count,mFph);
		}
		else
		{
			const char *data = (const char *)buffer;

			for (size_t i=0; i<count; i++)
			{
				if ( write(data,size) )
				{
					data+=size;
					ret++;
				}
				else
				{
					break;
				}
			}
		}
		return ret;
	}

	size_t IniFileInterface::writeString( const char *str )
	{
		size_t ret = 0;
		if ( str )
		{
			size_t len = strlen(str);
			ret = write(str,len, 1 );
		}
		return ret;
	}

	size_t IniFileInterface::flush( void )
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = fflush(mFph);
		}
		return ret;
	}

	size_t IniFileInterface::seek( size_t loc,size_t mode )
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = fseek(mFph,loc,mode);
		}
		else
		{
			if ( mode == SEEK_SET )
			{
				if ( loc <= mLen )
				{
					mLoc = loc;
					ret = 1;
				}
			}
			else if ( mode == SEEK_END )
			{
				mLoc = mLen;
			}
			else
			{
				ph_assert(0);
			}
		}
		return ret;
	}

	size_t IniFileInterface::tell( void )
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = ftell(mFph);
		}
		else
		{
			ret = mLoc;
		}
		return ret;
	}

	size_t IniFileInterface::myputc( char c )
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = fputc(c,mFph);
		}
		else
		{
			ret = write(&c,1);
		}
		return ret;
	}

	size_t IniFileInterface::eof( void )
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = feof(mFph);
		}
		else
		{
			if ( mLoc >= mLen )
				ret = 1;
		}
		return ret;
	}

	size_t IniFileInterface::error( void )
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = ferror(mFph);
		}
		return ret;
	}

	IniFileInterface * IniFileInterface::kvfi_fopen(const char *fname,const char *spec,void *mem,size_t len)
	{
		IniFileInterface *ret = 0;

		ret = ph_new(IniFileInterface)(fname,spec,mem,len);

		if ( mem == 0 && ret->mData == 0)
		{
			if ( ret->mFph == 0 )
			{
				delete ret;
				ret = 0;
			}
		}

		return ret;
	}

	void IniFileInterface::kvfi_fclose(IniFileInterface *file)
	{
		delete file;
	}

	size_t IniFileInterface::kvfi_fread(void *buffer,size_t size,size_t count,IniFileInterface *fph)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->read(buffer,size,count);
		}
		return ret;
	}

	size_t IniFileInterface::kvfi_fwrite(const void *buffer,size_t size,size_t count,IniFileInterface *fph)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->write(buffer,size,count);
		}
		return ret;
	}

	size_t IniFileInterface::kvfi_fprintf(IniFileInterface *fph,const char *fmt,...)
	{
		size_t ret = 0;

		char buffer[2048];
		va_list arg;
		va_start( arg, fmt );
		buffer[2047] = 0;
		_vsnprintf(buffer,2047, fmt, arg);
		va_end(arg);

		if ( fph )
		{
			ret = fph->writeString(buffer);
		}

		return ret;
	}


	size_t IniFileInterface::kvfi_fflush(IniFileInterface *fph)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->flush();
		}
		return ret;
	}


	size_t IniFileInterface::kvfi_fseek(IniFileInterface *fph,size_t loc,size_t mode)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->seek(loc,mode);
		}
		return ret;
	}

	size_t IniFileInterface::kvfi_ftell(IniFileInterface *fph)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->tell();
		}
		return ret;
	}

	size_t IniFileInterface::kvfi_fputc(char c,IniFileInterface *fph)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->myputc(c);
		}
		return ret;
	}

	size_t IniFileInterface::kvfi_fputs(const char *str,IniFileInterface *fph)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->writeString(str);
		}
		return ret;
	}

	size_t IniFileInterface::kvfi_feof(IniFileInterface *fph)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->eof();
		}
		return ret;
	}

	size_t IniFileInterface::kvfi_ferror(IniFileInterface *fph)
	{
		size_t ret = 0;
		if ( fph )
		{
			ret = fph->error();
		}
		return ret;
	}

	void* IniFileInterface::kvfi_getMemBuffer(IniFileInterface *fph,size_t &outputLength)
	{
		outputLength = 0;
		void * ret = 0;
		if ( fph )
		{
			ret = fph->mData;
			outputLength = fph->mLoc;
		}
		return ret;
	}
}