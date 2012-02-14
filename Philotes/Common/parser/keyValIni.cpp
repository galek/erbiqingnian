#include "keyValIni.h"

namespace Philo
{

	KvInPlaceParser::KvInPlaceParser( void )
	{
		Init();
	}

	KvInPlaceParser::KvInPlaceParser( char *data,int32 len )
	{
		Init();
		SetSourceData(data,len);
	}

	KvInPlaceParser::KvInPlaceParser( const char *fname )
	{
		Init();
		SetFile(fname);
	}

	void KvInPlaceParser::Init( void )
	{
		mQuoteChar = 34;
		mData = 0;
		mLen  = 0;
		mMyAlloc = false;
		for (int32 i=0; i<256; i++)
		{
			mHard[i] = ST_DATA;
			mHardString[i*2] = (char)i;
			mHardString[i*2+1] = (char)0;
		}
		mHard[0]  = ST_EOS;
		mHard[32] = ST_SOFT;
		mHard[9]  = ST_SOFT;
		mHard[13] = ST_SOFT;
		mHard[10] = ST_SOFT;
	}

	void KvInPlaceParser::SetSourceData( char *data,int32 len )
	{
		mData = data;
		mLen  = len;
		mMyAlloc = false;
	}

	void KvInPlaceParser::validateMem( const char *data,int32 len )
	{
#ifdef PH_DEBUG
		for (int32 i=0; i<len; i++)
		{
			ph_assert( data[i] );
		}
#endif
	}

	void KvInPlaceParser::SetSourceDataCopy( const char *data,int32 len )
	{
		if ( len )
		{

			//validateMem(data,len);

			free(mData);
			mData = (char *)malloc(len+1);
			memcpy(mData,data,len);
			mData[len] = 0;

			//validateMem(mData,len);
			mLen  = len;
			mMyAlloc = true;
		}
	}

	void KvInPlaceParser::SetHardSeparator( uint8 c ) /* add a hard separator */
	{
		mHard[c] = ST_HARD;
	}

	void KvInPlaceParser::SetHard( uint8 c ) /* add a hard separator */
	{
		mHard[c] = ST_HARD;
	}

	void KvInPlaceParser::SetCommentSymbol( uint8 c )
	{
		mHard[c] = ST_COMMENT;
	}

	void KvInPlaceParser::ClearHardSeparator( uint8 c )
	{
		mHard[c] = ST_DATA;
	}

	bool KvInPlaceParser::EOS( uint8 c )
	{
		if ( mHard[c] == ST_EOS || mHard[c] == ST_COMMENT )
		{
			return true;
		}
		return false;
	}

	void KvInPlaceParser::SetQuoteChar( char c )
	{
		mQuoteChar = c;
	}

	void KvInPlaceParser::SetFile( const char *fname )
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
				int32 ok = fread(mData, mLen, 1, fph);
				if ( !ok )
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

	KvInPlaceParser::~KvInPlaceParser( void )
	{
		if ( mMyAlloc )
		{
			free(mData);
		}
	}

	bool KvInPlaceParser::IsHard( uint8 c )
	{
		return mHard[c] == ST_HARD;
	}

	char * KvInPlaceParser::AddHard( int32 &argc,const char **argv,char *foo )
	{
		while ( IsHard(*foo) )
		{
			uint8 c = *foo;
			const char *hard = &mHardString[c*2];
			if ( argc < MAXARGS )
			{
				argv[argc++] = hard;
			}
			foo++;
		}
		return foo;
	}

	bool KvInPlaceParser::IsWhiteSpace( uint8 c )
	{
		return mHard[c] == ST_SOFT;
	}

	char * KvInPlaceParser::SkipSpaces( char *foo )
	{
		while ( !EOS(*foo) && IsWhiteSpace(*foo) ) foo++;
		return foo;
	}


	bool KvInPlaceParser::IsNonSeparator(uint8 c)
	{
		if ( !IsHard(c) && !IsWhiteSpace(c) && c != 0 ) return true;
		return false;
	}


	bool KvInPlaceParser::IsComment(uint8 c) const
	{
		if ( mHard[c] == ST_COMMENT ) return true;
		return false;
	}

	int32 KvInPlaceParser::ProcessLine(int32 lineno,char *line,InPlaceParserInterface *callback)
	{
		int32 ret = 0;

		const char *argv[MAXARGS];
		int32 argc = 0;

		char *foo = SkipSpaces(line); // skip leading spaces...

		if ( IsComment(*foo) )  // if the leading character is a comment symbol.
			return 0;


		if ( !EOS(*foo) )  // if we are not at the end of string then..
		{
			argv[argc++] = foo;  // this is the key
			foo++;

			while ( !EOS(*foo) )  // now scan forward until we hit an equal sign.
			{
				if ( *foo == '=' ) // if this is the equal sign then...
				{
					*foo = 0; // stomp a zero byte on the equal sign to terminate the key we should search for trailing spaces too...
					// look for trailing whitespaces and trash them.
					char *scan = foo-1;
					while ( IsWhiteSpace(*scan) )
					{
						*scan = 0;
						scan--;
					}

					foo++;
					foo = SkipSpaces(foo);
					if ( !EOS(*foo) )
					{
						argv[argc++] = foo;
						foo++;
						while ( !EOS(*foo) )
						{
							foo++;
						}
						*foo = 0;
						char *scan = foo-1;
						while ( IsWhiteSpace(*scan) )
						{
							*scan = 0;
							scan--;
						}
						break;
					}
				}
				if ( *foo )
					foo++;
			}
		}

		*foo = 0;

		if ( argc )
		{
			ret = callback->ParseLine(lineno, argc, argv );
		}

		return ret;
	}

	int32  KvInPlaceParser::Parse(InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
	{
		ph_assert( callback );
		if ( !mData ) return 0;

		int32 ret = 0;

		int32 lineno = 0;

		char *foo   = mData;
		char *begin = foo;


		while ( *foo )
		{
			if ( *foo == 10 || *foo == 13 )
			{
				lineno++;
				*foo = 0;

				if ( *begin ) // if there is any data to parse at all...
				{
					int32 v = ProcessLine(lineno,begin,callback);
					if ( v ) ret = v;
				}

				foo++;
				if ( *foo == 10 ) foo++; // skip line feed, if it is in the carraige-return line-feed format...
				begin = foo;
			}
			else
			{
				foo++;
			}
		}

		lineno++; // lasst line.

		int32 v = ProcessLine(lineno,begin,callback);
		if ( v ) ret = v;
		return ret;
	}


	void KvInPlaceParser::DefaultSymbols(void)
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

	KeyValueIni::KeyValueIni( const char *fname )
	{
		mData.SetFile(fname);
		mData.SetCommentSymbol('#');
		mData.SetCommentSymbol('!');
		mData.SetCommentSymbol(';');
		mData.SetHard('=');
		mCurrentSection = 0;
		KeyValueSection *kvs = ph_new(KeyValueSection)("@HEADER",0);
		mSections.push_back(kvs);
		mData.Parse(this);
	}

	KeyValueIni::KeyValueIni( const char *mem,uint32 len )
	{
		if ( len )
		{
			mCurrentSection = 0;
			mData.SetSourceDataCopy(mem,len);

			mData.SetCommentSymbol('#');
			mData.SetCommentSymbol('!');
			mData.SetCommentSymbol(';');
			mData.SetHard('=');
			KeyValueSection *kvs = ph_new(KeyValueSection)("@HEADER",0);
			mSections.push_back(kvs);
			mData.Parse(this);
		}
	}

	KeyValueIni::KeyValueIni( void )
	{
		mCurrentSection = 0;
		KeyValueSection *kvs = ph_new(KeyValueSection)("@HEADER",0);
		mSections.push_back(kvs);
	}

	KeyValueIni::~KeyValueIni( void )
	{
		reset();
	}

	void KeyValueIni::reset( void )
	{
		KeyValueSectionVector::iterator i;
		for (i=mSections.begin(); i!=mSections.end(); ++i)
		{
			KeyValueSection *kvs = (*i);
			delete kvs;
		}
		mSections.clear();
		mCurrentSection = 0;
	}

	int32 KeyValueIni::ParseLine( int32 lineno,int32 argc,const char **argv ) /* return TRUE to continue parsing, return FALSE to abort parsing process */
	{
		if ( argc )
		{
			const char *key = argv[0];
			if ( key[0] == '[' )
			{
				key++;
				char *scan = (char *) key;
				while ( *scan )
				{
					if ( *scan == ']')
					{
						*scan = 0;
						break;
					}
					scan++;
				}
				mCurrentSection = -1;
				for (uint32 i=0; i<mSections.size(); i++)
				{
					KeyValueSection &kvs = *mSections[i];
					if ( stricmp(kvs.getSection(),key) == 0 )
					{
						mCurrentSection = (int32) i;
						break;
					}
				}
				//...
				if ( mCurrentSection < 0 )
				{
					mCurrentSection = mSections.size();
					KeyValueSection *kvs = ph_new(KeyValueSection)(key,lineno);
					mSections.push_back(kvs);
				}
			}
			else
			{
				const char *key = argv[0];
				const char *value = 0;
				if ( argc >= 2 )
					value = argv[1];
				mSections[mCurrentSection]->addKeyValue(key,value,lineno);
			}
		}

		return 0;
	}

	KeyValueSection * KeyValueIni::locateSection( const char *section,uint32 &keys,uint32 &lineno ) const
	{
		KeyValueSection *ret = 0;
		for (uint32 i=0; i<mSections.size(); i++)
		{
			KeyValueSection *s = mSections[i];
			if ( stricmp(section,s->getSection()) == 0 )
			{
				ret = s;
				lineno = s->getLineNo();
				keys = s->getKeyCount();
				break;
			}
		}
		return ret;
	}

	const KeyValueSection * KeyValueIni::getSection( uint32 index,uint32 &keys,uint32 &lineno ) const
	{
		const KeyValueSection *ret=0;
		if ( index < mSections.size() )
		{
			const KeyValueSection &s = *mSections[index];
			ret = &s;
			lineno = s.getLineNo();
			keys = s.getKeyCount();
		}
		return ret;
	}

	bool KeyValueIni::save( const char *fname ) const
	{
		bool ret = false;
		IniFileInterface *fph = IniFileInterface::kvfi_fopen(fname,"wb");
		if ( fph )
		{
			for (uint32 i=0; i<mSections.size(); i++)
			{
				mSections[i]->save(fph);
			}
			IniFileInterface::kvfi_fclose(fph);
			ret = true;
		}
		return ret;
	}

	void * KeyValueIni::saveMem( uint32 &len ) const
	{
		void *ret = 0;
		IniFileInterface *fph = IniFileInterface::kvfi_fopen("mem","wmem");
		if ( fph )
		{
			for (uint32 i=0; i<mSections.size(); i++)
			{
				mSections[i]->save(fph);
			}

			size_t tmpLen;
			void *temp = IniFileInterface::kvfi_getMemBuffer(fph,tmpLen);
			len = (uint32)tmpLen;
			if ( temp )
			{
				ret = malloc(len);
				memcpy(ret,temp,len);
			}

			IniFileInterface::kvfi_fclose(fph);
		}
		return ret;
	}

	KeyValueSection  * KeyValueIni::createKeyValueSection( const char *section_name,bool reset ) /* creates, or locates and existing section for editing. If reset it true, will erase previous contents of the section. */
	{
		KeyValueSection *ret = 0;

		for (uint32 i=0; i<mSections.size(); i++)
		{
			KeyValueSection *kvs = mSections[i];
			if ( strcmp(kvs->getSection(),section_name) == 0 )
			{
				ret = kvs;
				if ( reset )
				{
					ret->reset();
				}
				break;
			}
		}
		if ( ret == 0 )
		{
			ret = ph_new(KeyValueSection)(section_name,0);
			mSections.push_back(ret);
		}

		return ret;
	}

}