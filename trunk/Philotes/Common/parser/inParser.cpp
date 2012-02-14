
#include "inParser.h"

namespace Philo
{

	void InPlaceParser::SetFile(const char *fname)
	{
		if ( mMyAlloc )
		{
			free(mData);
		}
		mData = 0;
		mLen  = 0;
		mMyAlloc = false;

		FILE *fph = fopen(fname,"rb");
		if ( fph )
		{
			fseek(fph,0L,SEEK_END);
			mLen = ftell(fph);
			fseek(fph,0L,SEEK_SET);

			if ( mLen )
			{
				mData = (char *) malloc(sizeof(char)*(mLen+1));
				int32 read = (int32)fread(mData,mLen,1,fph);
				if ( !read )
				{
					free(mData);
					mData = 0;
				}
				else
				{
					mData[mLen] = 0; // zero byte terminate end of file marker.
					mMyAlloc = true;
				}
			}
			fclose(fph);
		}
	}

	//==================================================================================
	InPlaceParser::~InPlaceParser(void)
	{
		if ( mMyAlloc )
		{
			free(mData);
		}
	}

	//==================================================================================
	bool InPlaceParser::IsHard(char c)
	{
		return mHard[c] == ST_HARD;
	}

	//==================================================================================
	char * InPlaceParser::AddHard(int32 &argc,const char **argv,char *foo)
	{
		while ( IsHard(*foo) )
		{
			const char *hard = &mHardString[*foo*2];
			if ( argc < MAXARGS )
			{
				argv[argc++] = hard;
			}
			++foo;
		}
		return foo;
	}

	//==================================================================================
	bool   InPlaceParser::IsWhiteSpace(char c)
	{
		return mHard[c] == ST_SOFT;
	}

	//==================================================================================
	char * InPlaceParser::SkipSpaces(char *foo)
	{
		while ( !EOS(*foo) && IsWhiteSpace(*foo) ) 
			++foo;
		return foo;
	}

	//==================================================================================
	bool InPlaceParser::IsNonSeparator(char c)
	{
		return ( !IsHard(c) && !IsWhiteSpace(c) && c != 0 );
	}

	//==================================================================================
	int32 InPlaceParser::ProcessLine(int32 lineno,char *line,InPlaceParserInterface *callback)
	{
		int32 ret = 0;

		const char *argv[MAXARGS];
		int32 argc = 0;

		char *foo = line;

		while ( !EOS(*foo) && argc < MAXARGS )
		{
			foo = SkipSpaces(foo); // skip any leading spaces

			if ( EOS(*foo) ) 
				break;

			if ( *foo == mQuoteChar ) // if it is an open quote
			{
				++foo;
				if ( argc < MAXARGS )
				{
					argv[argc++] = foo;
				}
				while ( !EOS(*foo) && *foo != mQuoteChar ) 
					++foo;
				if ( !EOS(*foo) )
				{
					*foo = 0; // replace close quote with zero byte EOS
					++foo;
				}
			}
			else
			{
				foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

				if ( IsNonSeparator(*foo) )  // add non-hard argument.
				{
					bool quote  = false;
					if ( *foo == mQuoteChar )
					{
						++foo;
						quote = true;
					}

					if ( argc < MAXARGS )
					{
						argv[argc++] = foo;
					}

					if ( quote )
					{
						while (*foo && *foo != mQuoteChar ) 
							++foo;
						if ( *foo ) 
							*foo = 32;
					}

					// continue..until we hit an eos ..
					while ( !EOS(*foo) ) // until we hit EOS
					{
						if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
						{
							*foo = 0;
							++foo;
							break;
						}
						else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
						{
							const char *hard = &mHardString[*foo*2];
							*foo = 0;
							if ( argc < MAXARGS )
							{
								argv[argc++] = hard;
							}
							++foo;
							break;
						}
						++foo;
					} // end of while loop...
				}
			}
		}

		if ( argc )
		{
			ret = callback->ParseLine(lineno, argc, argv );
		}

		return ret;
	}


	int32  InPlaceParser::Parse(const char *str,InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
	{
		int32 ret = 0;

		mLen = (int32)strlen(str);
		if ( mLen )
		{
			mData = (char *)malloc(mLen+1);
			strcpy(mData,str);
			mMyAlloc = true;
			ret = Parse(callback);
		}
		return ret;
	}

	//==================================================================================
	// returns true if entire file was parsed, false if it aborted for some reason
	//==================================================================================
	int32  InPlaceParser::Parse(InPlaceParserInterface *callback)
	{
		int32 ret = 0;
		ph_assert( callback );
		if ( mData )
		{
			int32 lineno = 0;

			char *foo   = mData;
			char *begin = foo;

			while ( *foo )
			{
				if ( isLineFeed(*foo) )
				{
					++lineno;
					*foo = 0;
					if ( *begin ) // if there is any data to parse at all...
					{
						bool snarfed = callback->preParseLine(lineno,begin);
						if ( !snarfed )
						{
							int32 v = ProcessLine(lineno,begin,callback);
							if ( v )
								ret = v;
						}
					}

					++foo;
					if ( *foo == 10 )
						++foo; // skip line feed, if it is in the carraige-return line-feed format...
					begin = foo;
				}
				else
				{
					++foo;
				}
			}

			lineno++; // lasst line.

			int32 v = ProcessLine(lineno,begin,callback);
			if ( v )
				ret = v;
		}
		return ret;
	}

	//==================================================================================
	void InPlaceParser::DefaultSymbols(void)
	{
		SetHardSeparator(',');
		SetHardSeparator('(');
		SetHardSeparator(')');
		SetHardSeparator('=');
		SetHardSeparator('[');
		SetHardSeparator(']');
		SetHardSeparator('{');
		SetHardSeparator('}');
		SetCommentSymbol('#');
	}

	//==================================================================================
	// convert source string into an arg list, this is a destructive parse.
	//==================================================================================
	const char ** InPlaceParser::GetArglist(char *line,int32 &count)
	{
		const char **ret = 0;

		int32 argc = 0;

		char *foo = line;

		while ( !EOS(*foo) && argc < MAXARGS )
		{
			foo = SkipSpaces(foo); // skip any leading spaces

			if ( EOS(*foo) )
				break;

			if ( *foo == mQuoteChar ) // if it is an open quote
			{
				++foo;
				if ( argc < MAXARGS )
				{
					argv[argc++] = foo;
				}
				while ( !EOS(*foo) && *foo != mQuoteChar ) 
					++foo;
				if ( !EOS(*foo) )
				{
					*foo = 0; // replace close quote with zero byte EOS
					++foo;
				}
			}
			else
			{
				foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

				if ( IsNonSeparator(*foo) )  // add non-hard argument.
				{
					bool quote  = false;
					if ( *foo == mQuoteChar )
					{
						++foo;
						quote = true;
					}

					if ( argc < MAXARGS )
					{
						argv[argc++] = foo;
					}

					if ( quote )
					{
						while (*foo && *foo != mQuoteChar ) 
							++foo;
						if ( *foo ) 
							*foo = 32;
					}

					// continue..until we hit an eos ..
					while ( !EOS(*foo) ) // until we hit EOS
					{
						if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
						{
							*foo = 0;
							++foo;
							break;
						}
						else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
						{
							const char *hard = &mHardString[*foo*2];
							*foo = 0;
							if ( argc < MAXARGS )
							{
								argv[argc++] = hard;
							}
							++foo;
							break;
						}
						++foo;
					} // end of while loop...
				}
			}
		}

		count = argc;
		if ( argc )
		{
			ret = argv;
		}

		return ret;
	}

}