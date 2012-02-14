
#pragma once

namespace Philo
{
	#define DEFAULT_BUFFER_SIZE 1000000
	#define DEFAULT_GROW_SIZE   2000000

	class IniFileInterface
	{
	public:
		IniFileInterface(const char *fname,const char *spec,void *mem,size_t len);

		~IniFileInterface(void);

		size_t read(char *data,size_t size);

		size_t write(const char *data,size_t size);

		size_t read(void *buffer,size_t size,size_t count);

		size_t write(const void *buffer,size_t size,size_t count);

		size_t writeString(const char *str);

		size_t flush(void);

		size_t seek(size_t loc,size_t mode);

		size_t tell(void);

		size_t myputc(char c);

		size_t eof(void);

		size_t error(void);

		//////////////////////////////////////////////////////////////////////////

		static IniFileInterface * kvfi_fopen(const char *fname,const char *spec,void *mem=0,size_t len=0);

		static void kvfi_fclose(IniFileInterface *file);

		static size_t kvfi_fread(void *buffer,size_t size,size_t count,IniFileInterface *fph);

		static size_t kvfi_fwrite(const void *buffer,size_t size,size_t count,IniFileInterface *fph);

		static size_t kvfi_fprintf(IniFileInterface *fph,const char *fmt,...);

		static size_t kvfi_fflush(IniFileInterface *fph);

		static size_t kvfi_fseek(IniFileInterface *fph,size_t loc,size_t mode);

		static size_t kvfi_ftell(IniFileInterface *fph);

		static size_t kvfi_fputc(char c,IniFileInterface *fph);

		static size_t kvfi_fputs(const char *str,IniFileInterface *fph);

		static size_t kvfi_feof(IniFileInterface *fph);

		static size_t kvfi_ferror(IniFileInterface *fph);

		static void* kvfi_getMemBuffer(IniFileInterface *fph,size_t &outputLength);

		//////////////////////////////////////////////////////////////////////////

		FILE*	mFph;
		char*	mData;
		size_t  mLen;
		size_t  mLoc;
		bool	mRead;
		char	mName[512];
		bool	mMyAlloc;
	};
}