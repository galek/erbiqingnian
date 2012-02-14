
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <assert.h>

#include "asc2bin.h"

_NAMESPACE_BEGIN


static inline bool         IsWhitespace(char c)
{
	if ( c == ' ' || c == 9 || c == 13 || c == 10 || c == ',' ) return true;
	return false;
}



static inline const char * SkipWhitespace(const char *str)
{
  if ( str )
  {
	  while ( *str && IsWhitespace(*str) ) str++;
  }
	return str;
}

static char ToLower(char c)
{
	if ( c >= 'A' && c <= 'Z' ) c+=32;
	return c;
}

static inline uint32 GetHex(uint8 c)
{
	uint32 v = 0;
	c = ToLower(c);
	if ( c >= '0' && c <= '9' )
		v = c-'0';
	else
	{
		if ( c >= 'a' && c <= 'f' )
		{
			v = 10 + c-'a';
		}
	}
	return v;
}

static inline uint8 GetHEX1(const char *foo,const char **endptr)
{
	uint32 ret = 0;

	ret = (GetHex(foo[0])<<4) | GetHex(foo[1]);

	if ( endptr )
	{
		*endptr = foo+2;
	}

	return (uint8) ret;
}


static inline unsigned short GetHEX2(const char *foo,const char **endptr)
{
	uint32 ret = 0;

	ret = (GetHex(foo[0])<<12) | (GetHex(foo[1])<<8) | (GetHex(foo[2])<<4) | GetHex(foo[3]);

	if ( endptr )
	{
		*endptr = foo+4;
	}

	return (unsigned short) ret;
}

static inline uint32 GetHEX4(const char *foo,const char **endptr)
{
	uint32 ret = 0;

	for (int32 i=0; i<8; i++)
	{
		ret = (ret<<4) | GetHex(foo[i]);
	}

	if ( endptr )
	{
		*endptr = foo+8;
	}

	return ret;
}

static inline uint32 GetHEX(const char *foo,const char **endptr)
{
	uint32 ret = 0;

	while ( *foo )
	{
		uint8 c = ToLower( *foo );
		uint32 v = 0;
		if ( c >= '0' && c <= '9' )
			v = c-'0';
		else
		{
			if ( c >= 'a' && c <= 'f' )
			{
				v = 10 + c-'a';
			}
			else
				break;
		}
		ret = (ret<<4)|v;
		foo++;
	}

	if ( endptr ) *endptr = foo;

	return ret;
}




#define MAXNUM 32

static inline scalar        GetFloatValue(const char *str,const char **next)
{
	scalar ret = 0;

	if ( next ) *next = 0;

	str = SkipWhitespace(str);

	char dest[MAXNUM];
	char *dst = dest;
	const char *hex = 0;

	for (int32 i=0; i<(MAXNUM-1); i++)
	{
		char c = *str;
		if ( c == 0 || IsWhitespace(c) )
		{
			if ( next ) *next = str;
			break;
		}
		else if ( c == '$' )
		{
			hex = str+1;
		}
		*dst++ = ToLower(c);
		str++;
	}

	*dst = 0;

	if ( hex )
	{
		uint32 iv = GetHEX(hex,0);
		scalar *v = (scalar *)&iv;
		ret = *v;
	}
	else if ( dest[0] == 'f' )
	{
		if ( strcmp(dest,"fltmax") == 0 || strcmp(dest,"fmax") == 0 )
		{
			ret = FLT_MAX;
		}
		else if ( strcmp(dest,"fltmin") == 0 || strcmp(dest,"fmin") == 0 )
		{
			ret = FLT_MIN;
		}
	}
	else if ( dest[0] == 't' ) // t or 'true' is treated as the value '1'.
	{
		ret = 1;
	}
	else
	{
		ret = (scalar)atof(dest);
	}
	return ret;
}

static inline int32          GetIntValue(const char *str,const char **next)
{
	int32 ret = 0;

	if ( next ) *next = 0;

	str = SkipWhitespace(str);

	char dest[MAXNUM];
	char *dst = dest;

	for (int32 i=0; i<(MAXNUM-1); i++)
	{
		char c = *str;
		if ( c == 0 || IsWhitespace(c) )
		{
			if ( next ) *next = str;
			break;
		}
		*dst++ = c;
		str++;
	}

	*dst = 0;

	ret = atoi(dest);

	return ret;
}



enum Atype {  AT_FLOAT,  AT_INT,  AT_CHAR,  AT_BYTE,  AT_SHORT,  AT_STR,  AT_HEX1,  AT_HEX2,  AT_HEX4,  AT_LAST };

#define MAXARG 64

#if 0
void TestAsc2bin(void)
{
	Asc2Bin("1 2 A 3 4 Foo AB ABCD FFEDFDED",1,"f d c b h p x1 x2 x4", 0);
}
#endif

void * Asc2Bin(const char *source,const int32 count,const char *spec,void *dest)
{

	int32   cnt = 0;
	int32   size  = 0;

	Atype types[MAXARG];

	const char *ctype = spec;

	while ( *ctype )
	{
		switch ( ToLower(*ctype) )
		{
			case 'f': size+= sizeof(scalar); types[cnt] = AT_FLOAT; cnt++;  break;
			case 'd': size+= sizeof(int32);   types[cnt] = AT_INT;   cnt++;  break;
			case 'c': size+=sizeof(char);   types[cnt] = AT_CHAR;  cnt++;  break;
			case 'b': size+=sizeof(char);   types[cnt] = AT_BYTE;  cnt++;  break;
			case 'h': size+=sizeof(short);  types[cnt] = AT_SHORT; cnt++;  break;
			case 'p': size+=sizeof(const char *);  types[cnt] = AT_STR; cnt++;  break;
			case 'x':
				{
					Atype type = AT_HEX4;
					int32 sz = 4;
					switch ( ctype[1] )
					{
						case '1':  type = AT_HEX1; sz   = 1; ctype++;  break;
						case '2':  type = AT_HEX2; sz   = 2; ctype++;  break;
						case '4':  type = AT_HEX4; sz   = 4; ctype++;  break;
					}
					types[cnt] = type;
					size+=sz;
					cnt++;
				}
				break;
		}
		if ( cnt == MAXARG ) return 0; // over flowed the maximum specification!
		ctype++;
	}

	bool myalloc = false;

	if ( dest == 0 )
	{
		myalloc = true;
		dest = (char *) malloc(sizeof(char)*count*size);
	}

	// ok...ready to parse lovely data....
	memset(dest,0,count*size); // zero out memory

	char *dst = (char *) dest; // where we are storing the results
	for (int32 i=0; i<count; i++)
	{
		for (int32 j=0; j<cnt; j++)
		{
			source = SkipWhitespace(source); // skip white spaces.

			if (source == NULL ||  *source == 0 ) // we hit the end of the input data before we successfully parsed all input!
			{
				if ( myalloc )
				{
					free(dest);
				}
				return 0;
			}

			switch ( types[j] )
			{
				case AT_FLOAT:
					{
						scalar *v = (scalar *) dst;
						*v = GetFloatValue(source,&source);
						dst+=sizeof(scalar);
					}
					break;
				case AT_INT:
					{
						int32 *v = (int32 *) dst;
						*v = GetIntValue( source, &source );
						dst+=sizeof(int32);
					}
					break;
				case AT_CHAR:
					{
						*dst++ = *source++;
					}
					break;
				case AT_BYTE:
					{
						char *v = (char *) dst;
						*v = (char)GetIntValue(source,&source);
						dst+=sizeof(char);
					}
					break;
				case AT_SHORT:
					{
						short *v = (short *) dst;
						*v = (unsigned short)GetIntValue( source,&source );
						dst+=sizeof(short);
					}
					break;
				case AT_STR:
					{
						const char **ptr = (const char **) dst;
						*ptr = source;
						dst+=sizeof(const char *);
						while ( *source && !IsWhitespace(*source) ) source++;
					}
					break;
				case AT_HEX1:
					{
						uint32 hex = GetHEX1(source,&source);
						uint8 *v = (uint8 *) dst;
						*v = (uint8)hex;
						dst+=sizeof(uint8);
					}
					break;
				case AT_HEX2:
					{
						uint32 hex = GetHEX2(source,&source);
						unsigned short *v = (unsigned short *) dst;
						*v = (unsigned short)hex;
						dst+=sizeof(unsigned short);
					}
					break;
				case AT_HEX4:
					{
						uint32 hex = GetHEX4(source,&source);
						uint32 *v = (uint32 *) dst;
						*v = hex;
						dst+=sizeof(uint32);
					}
					break;
			}
		}
	}

	return dest;
}

_NAMESPACE_END