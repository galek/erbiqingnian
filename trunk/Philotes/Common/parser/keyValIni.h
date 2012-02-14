#pragma once

// ≈‰÷√Œƒº˛¿‡

#include "inParser.h"
#include "iniFileInterface.h"

namespace Philo
{
	class KvInPlaceParser
	{
	public:
		KvInPlaceParser(void);

		KvInPlaceParser(char *data,int32 len);

		KvInPlaceParser(const char *fname);

		~KvInPlaceParser(void);

		void Init(void);

		void SetFile(const char *fname); // use this file as source data to parse.

		void SetSourceData(char *data,int32 len);

		void validateMem(const char *data,int32 len);

		void SetSourceDataCopy(const char *data,int32 len);;

		int32  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

		int32 ProcessLine(int32 lineno,char *line,InPlaceParserInterface *callback);

		void SetHardSeparator(uint8 c);

		void SetHard(uint8 c);

		void SetCommentSymbol(uint8 c);

		void ClearHardSeparator(uint8 c);

		void DefaultSymbols(void); // set up default symbols for hard seperator and comment symbol of the '#' character.

		bool EOS(uint8 c);

		void SetQuoteChar(char c);

		inline bool IsComment(uint8 c) const;

	private:

		inline char*	AddHard(int32 &argc,const char **argv,char *foo);

		inline bool		IsHard(uint8 c);

		inline char*	SkipSpaces(char *foo);

		inline bool		IsWhiteSpace(uint8 c);

		inline bool		IsNonSeparator(uint8 c); // non seperator,neither hard nor soft

		bool			mMyAlloc; // whether or not *I* allocated the buffer and am responsible for deleting it.
		char*			mData;  // ascii data to parse.
		int32			mLen;   // length of data
		SeparatorType	mHard[256];
		char			mHardString[256*2];
		char			mQuoteChar;
	};

	//////////////////////////////////////////////////////////////////////////

	class KeyValue
	{
	public:
		KeyValue(const char *key,const char *value,uint32 lineno)
		{
			mKey = key;
			mValue = value;
			mLineNo = lineno;
		}

		const char * getKey(void) const { return mKey; };

		const char * getValue(void) const { return mValue; };

		uint32 getLineNo(void) const { return mLineNo; };

		void save(IniFileInterface *fph) const
		{
			IniFileInterface::kvfi_fprintf(fph,"%-30s = %s\r\n", mKey, mValue );
		}

		void setValue(const char *value)
		{
			mValue = value;
		}

	private:
		uint32 mLineNo;
		const char *mKey;
		const char *mValue;
	};

	typedef std::vector< KeyValue > KeyValueVector;

	//////////////////////////////////////////////////////////////////////////

	class KeyValueSection
	{
	public:
		KeyValueSection(const char *section,uint32 lineno)
		{
			mSection = section;
			mLineNo  = lineno;
		}

		uint32 getKeyCount(void) const { return mKeys.size(); };
		const char * getSection(void) const { return mSection; };
		uint32 getLineNo(void) const { return mLineNo; };

		const char * locateValue(const char *key,uint32 lineno) const
		{
			const char *ret = 0;

			for (uint32 i=0; i<mKeys.size(); i++)
			{
				const KeyValue &v = mKeys[i];
				if ( stricmp(key,v.getKey()) == 0 )
				{
					ret = v.getValue();
					lineno = v.getLineNo();
					break;
				}
			}
			return ret;
		}

		const char *getKey(uint32 index,uint32 &lineno) const
		{
			const char * ret  = 0;
			if ( index < mKeys.size() )
			{
				const KeyValue &v = mKeys[index];
				ret = v.getKey();
				lineno = v.getLineNo();
			}
			return ret;
		}

		const char *getValue(uint32 index,uint32 &lineno) const
		{
			const char * ret  = 0;
			if ( index < mKeys.size() )
			{
				const KeyValue &v = mKeys[index];
				ret = v.getValue();
				lineno = v.getLineNo();
			}
			return ret;
		}

		void addKeyValue(const char *key,const char *value,uint32 lineno)
		{
			KeyValue kv(key,value,lineno);
			mKeys.push_back(kv);
		}

		void save(IniFileInterface *fph) const
		{
			if ( strcmp(mSection,"@HEADER") == 0 )
			{
			}
			else
			{
				IniFileInterface::kvfi_fprintf(fph,"\r\n");
				IniFileInterface::kvfi_fprintf(fph,"\r\n");
				IniFileInterface::kvfi_fprintf(fph,"[%s]\r\n", mSection );
			}
			for (uint32 i=0; i<mKeys.size(); i++)
			{
				mKeys[i].save(fph);
			}
		}


		bool  addKeyValue(const char *key,const char *value) // adds a key-value pair.  These pointers *must* be persistent for the lifetime of the INI file!
		{
			bool ret = false;

			for (uint32 i=0; i<mKeys.size(); i++)
			{
				KeyValue &kv = mKeys[i];
				if ( strcmp(kv.getKey(),key) == 0 )
				{
					kv.setValue(value);
					ret = true;
					break;
				}
			}

			if ( !ret )
			{
				KeyValue kv(key,value,0);
				mKeys.push_back(kv);
				ret = true;
			}

			return ret;
		}

		void reset(void)
		{
			mKeys.clear();
		}

	private:
		uint32 mLineNo;
		const char *mSection;
		KeyValueVector mKeys;
	};

	typedef std::vector< KeyValueSection *> KeyValueSectionVector;

	//////////////////////////////////////////////////////////////////////////

	class KeyValueIni : public InPlaceParserInterface
	{
	public:
		KeyValueIni(const char *fname);

		KeyValueIni(const char *mem,uint32 len);

		KeyValueIni(void);

		virtual ~KeyValueIni(void);

		void reset(void);

		uint32 getSectionCount(void) const { return mSections.size(); };

		int32 ParseLine(int32 lineno,int32 argc,const char **argv);

		KeyValueSection * locateSection(const char *section,uint32 &keys,uint32 &lineno) const;

		const KeyValueSection * getSection(uint32 index,uint32 &keys,uint32 &lineno) const;

		bool save(const char *fname) const;

		void * saveMem(uint32 &len) const;

		KeyValueSection  *createKeyValueSection(const char *section_name,bool reset);

	private:
		int32                   mCurrentSection;
		KeyValueSectionVector	mSections;
		KvInPlaceParser         mData;
	};

	KeyValueIni* loadKeyValueIni(const char *fname,uint32 &sections)
	{
		KeyValueIni *ret = 0;

		ret = ph_new(KeyValueIni)(fname);
		sections = ret->getSectionCount();
		if ( sections < 2 )
		{
			delete ret;
			ret = 0;
		}

		return ret;
	}

	KeyValueIni* loadKeyValueIni(const char *mem,uint32 len,uint32 &sections)
	{
		KeyValueIni *ret = 0;

		ret = ph_new(KeyValueIni)(mem,len);
		sections = ret->getSectionCount();
		if ( sections < 2 )
		{
			delete ret;
			ret = 0;
		}

		return ret;
	}

	const KeyValueSection* locateSection(const KeyValueIni *ini,const char *section,uint32 &keys,uint32 &lineno)
	{
		KeyValueSection* ret = 0;

		if ( ini )
		{
			ret = ini->locateSection(section,keys,lineno);
		}

		return ret;
	}

	const KeyValueSection* getSection(const KeyValueIni *ini,uint32 index,uint32 &keycount,uint32 &lineno)
	{
		const KeyValueSection *ret = 0;

		if ( ini )
		{
			ret = ini->getSection(index,keycount,lineno);
		}

		return ret;
	}

	const char *      locateValue(const KeyValueSection *section,const char *key,uint32 &lineno)
	{
		const char *ret = 0;

		if ( section )
		{
			ret = section->locateValue(key,lineno);
		}

		return ret;
	}

	const char *      getKey(const KeyValueSection *section,uint32 keyindex,uint32 &lineno)
	{
		const char *ret = 0;

		if ( section )
		{
			ret = section->getKey(keyindex,lineno);
		}

		return ret;
	}

	const char *      getValue(const KeyValueSection *section,uint32 keyindex,uint32 &lineno)
	{
		const char *ret = 0;

		if ( section )
		{
			ret = section->getValue(keyindex,lineno);
		}

		return ret;
	}

	void              releaseKeyValueIni(const KeyValueIni *ini)
	{
		KeyValueIni *k = (KeyValueIni *)ini;
		delete k;
	}


	const char *    getSectionName(const KeyValueSection *section)
	{
		const char *ret = 0;
		if ( section )
		{
			ret = section->getSection();
		}
		return ret;
	}


	bool  saveKeyValueIni(const KeyValueIni *ini,const char *fname)
	{
		bool ret = false;

		if ( ini )
			ret = ini->save(fname);

		return ret;
	}

	void *  saveKeyValueIniMem(const KeyValueIni *ini,uint32 &len)
	{
		void *ret = 0;

		if ( ini )
			ret = ini->saveMem(len);

		return ret;
	}

	KeyValueSection  *createKeyValueSection(KeyValueIni *ini,const char *section_name,bool reset)
	{
		KeyValueSection *ret = 0;

		if ( ini )
		{
			ret = ini->createKeyValueSection(section_name,reset);
		}
		return ret;
	}

	bool  addKeyValue(KeyValueSection *section,const char *key,const char *value)
	{
		bool ret = false;

		if ( section )
		{
			ret = section->addKeyValue(key,value);
		}

		return ret;
	}


	KeyValueIni      *createKeyValueIni(void) // create an empty .INI file in memory for editing.
	{
		KeyValueIni *ret = ph_new(KeyValueIni);
		return ret;
	}

	bool              releaseIniMem(void *mem)
	{
		bool ret = false;
		if ( mem )
		{
			free(mem);
			ret = true;
		}
		return ret;
	}
}