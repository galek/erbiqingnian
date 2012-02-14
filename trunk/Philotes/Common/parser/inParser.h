
#pragma once

namespace Philo
{
	#define MAXARGS 512

	class InPlaceParserInterface
	{
	public:

		// return TRUE继续parse，否则终止
		virtual int32 ParseLine(int32 lineno,int32 argc,const char **argv) =0;

		// 可以将行数据作为原始数据预先解析，返回true则跳过本行解析（由你自己解析）
		virtual bool preParseLine(int32 /* lineno */,const char * /* line */)  { return false; }; 
	};

	//////////////////////////////////////////////////////////////////////////

	enum SeparatorType
	{
		ST_DATA,        // 数据
		ST_HARD,        // 硬分隔符
		ST_SOFT,        // 软分隔符
		ST_EOS,			// 注释符号
		ST_COMMENT,
		ST_LINE_FEED,
	};

	//////////////////////////////////////////////////////////////////////////

	class InPlaceParser
	{
	public:
		InPlaceParser(void)
		{
			Init();
		}

		InPlaceParser(char *data,int32 len)
		{
			Init();
			SetSourceData(data,len);
		}

		InPlaceParser(const char *fname)
		{
			Init();
			SetFile(fname);
		}

		~InPlaceParser(void);

		void Init(void)
		{
			mQuoteChar = 34;
			mData = 0;
			mLen  = 0;
			mMyAlloc = false;
			for (int32 i=0; i<256; i++)
			{
				mHard[i] = ST_DATA;
				mHardString[i*2] = (char)i;
				mHardString[i*2+1] = 0;
			}
			mHard[0]  = ST_EOS;
			mHard[32] = ST_SOFT;
			mHard[9]  = ST_SOFT;
			mHard[13] = ST_LINE_FEED;
			mHard[10] = ST_LINE_FEED;
		}

		void SetFile(const char *fname);

		void SetSourceData(char *data,int32 len)
		{
			mData = data;
			mLen  = len;
			mMyAlloc = false;
		};

		int32  Parse(const char *str,InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

		int32  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

		int32 ProcessLine(int32 lineno,char *line,InPlaceParserInterface *callback);

		const char ** GetArglist(char *source,int32 &count); // convert source string into an arg list, this is a destructive parse.

		void SetHardSeparator(char c)
		{
			mHard[c] = ST_HARD;
		}

		void SetHard(char c) 
		{
			mHard[c] = ST_HARD;
		}

		void SetSoft(char c) 
		{
			mHard[c] = ST_SOFT;
		}


		void SetCommentSymbol(char c)
		{
			mHard[c] = ST_EOS;
		}

		void ClearHardSeparator(char c)
		{
			mHard[c] = ST_DATA;
		}


		void DefaultSymbols(void);

		bool EOS(char c)
		{
			if ( mHard[c] == ST_EOS )
			{
				return true;
			}
			return false;
		}

		void SetQuoteChar(char c)
		{
			mQuoteChar = c;
		}

		bool HasData( void ) const
		{
			return ( mData != 0 );
		}

		void setLineFeed(char c)
		{
			mHard[c] = ST_LINE_FEED;
		}

		bool isLineFeed(char c)
		{
			if ( mHard[c] == ST_LINE_FEED ) return true;
			return false;
		}

	private:

		inline char * AddHard(int32 &argc,const char **argv,char *foo);

		inline bool   IsHard(char c);

		inline char * SkipSpaces(char *foo);

		inline bool   IsWhiteSpace(char c);

		inline bool   IsNonSeparator(char c); // 无分隔符

		bool			mMyAlloc;	// 是否自己管理内存

		char*			mData;		// 待解析的ascii长度

		int32			mLen;		// data长度

		SeparatorType	mHard[256];

		char			mHardString[256*2];

		char			mQuoteChar;

		const char*		argv[MAXARGS];
	};
}