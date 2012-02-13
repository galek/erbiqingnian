//------------------------------------------------------------------------------
//  string.cc
//  (C) 2006 RadonLabs GmbH
//------------------------------------------------------------------------------

#include "util/string.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

namespace Philo
{

const String::size_type String::npos = -1;
String String::BLANK = "";
//------------------------------------------------------------------------------
/**
*/
bool
String::CopyToBuffer(char* buf, SizeT bufSize) const
{
    ph_assert(0 != buf);
    ph_assert(bufSize > 1);

    #if __WIN32__
        HRESULT hr = StringCchCopy(buf, bufSize, this->AsCharPtr());
        return SUCCEEDED(hr);    
    #else
        strncpy(buf, this->AsCharPtr(), bufSize - 1);
        buf[bufSize - 1] = 0;
        return this->strLen < bufSize;
    #endif
}

//------------------------------------------------------------------------------
/**
*/
void __cdecl
String::Format(const char* fmtString, ...)
{
    va_list argList;
    va_start(argList, fmtString);
    char buf[4096]; // an 4 kByte buffer
    // need to use non-CRT thread safe function under Win32
    StringCchVPrintf(buf, sizeof(buf), fmtString, argList);

    *this = buf;
    va_end(argList);
}

//------------------------------------------------------------------------------
/**
*/
void __cdecl
String::FormatArgList(const char* fmtString, va_list argList)
{
    char buf[4096]; // an 4 kByte buffer
    // need to use non-CRT thread safe function under Win32
    StringCchVPrintf(buf, sizeof(buf), fmtString, argList);
    //*this = buf;
	this->Set(buf, strlen(buf));
}

//------------------------------------------------------------------------------
/**
    Sets a new string content. This will handle all special cases and try
    to minimize heap allocations as much as possible.
*/
void
String::Set(const char* str, SizeT length)
{
    if (0 == str)
    {
        // a null string pointer is valid as a special case
        this->Delete();
        this->localBuffer[0] = 0;
    }
    else if ((0 == this->heapBuffer) && (length < LocalStringSize))
    {
        // the new contents fits into the local buffer,
        // however, if there is already an external buffer
        // allocated, we use the external buffer!
        this->Delete();
		memcpy(this->localBuffer,str,length);
        this->localBuffer[length] = 0;
    }
    else if (length < this->heapBufferSize)
    {
        // the new contents fits into the existing buffer
		memcpy(this->heapBuffer,str, length);
        this->heapBuffer[length] = 0;
        this->localBuffer[0] = 0;
    }
    else
    {
        // need to allocate bigger heap buffer
        this->Alloc(length + 1);
		memcpy(this->heapBuffer,str,length);
        this->heapBuffer[length] = 0;
        this->localBuffer[0] = 0;
    }
    this->strLen = length;
}

//------------------------------------------------------------------------------
void
String::AppendRange(const char* append, SizeT length)
{
    ph_assert(append);
    if (length > 0)
    {
        SizeT newLength = this->strLen + length;
        
        // if there is an external buffer allocated, we need to stay there
        // even if the resulting string is smaller then LOCALSTRINGSIZE,
        // a small string in an external buffer may be the result
        // of a call to Reserve()
        if ((0 == this->heapBuffer) && (newLength < LocalStringSize))
        {
            // the result fits into the local buffer
			memcpy(this->localBuffer + this->strLen,append,length);
            this->localBuffer[newLength] = 0;
        }
        else if (newLength < this->heapBufferSize)
        {
            // the result fits into the existing buffer
			memcpy(this->heapBuffer + this->strLen,append,length);
            this->heapBuffer[newLength] = 0;
        }
        else
        {
            // need to re-allocate
            this->Realloc(newLength + newLength / 2);
			memcpy(this->heapBuffer + this->strLen,append,length);
            this->heapBuffer[newLength] = 0;
        }
        this->strLen = newLength;
    }
}

//------------------------------------------------------------------------------
/**
    Tokenize the string into a String array. 

    @param  whiteSpace      a string containing the whitespace characters
	@param  maxSplits       The maximum number of splits to perform (0 for unlimited splits). 
		If this parameters is > 0, the splitting process will stop after this many splits, left to right.
		Modified by Dinghao Li
	@return                 outTokens.Size()
*/
SizeT
String::TokenizeEx(const String& whiteSpace, Array<String>& outTokens, unsigned int maxSplits) const
{
    outTokens.Clear();
    String str(*this);
    char* ptr = const_cast<char*>(str.AsCharPtr());
    const char* token;
	unsigned int numSplits = 0;
    while ( 0 != (token = strtok(ptr, whiteSpace.AsCharPtr())) )
    {
        outTokens.Append(token);
		if (maxSplits && numSplits == maxSplits)
			break;
        ptr = 0;
		++numSplits;
    }
    return outTokens.Size();
}

//------------------------------------------------------------------------------
/**
    This is the slow-but-convenient Tokenize() method. Slow since the
    returned string array will be constructed anew with every method call.
    Consider the Tokenize() method which takes a string array as input,
    since this may allow reusing of the array, reducing heap allocations.
*/
Array<String>
String::TokenizeEx(const String& whiteSpace, unsigned int maxSplits) const
{
    Array<String> tokens;
    this->TokenizeEx(whiteSpace, tokens, maxSplits);
    return tokens;
}

//------------------------------------------------------------------------------
/**
    Tokenize a string, but keeps the string within the fence-character
    intact. For instance for the sentence:

    He said: "I don't know."

    A Tokenize(" ", '"', tokens) would return:

    token 0:    He
    token 1:    said:
    token 2:    I don't know.
*/
SizeT
String::Tokenize(const String& whiteSpace, char fence, Array<String>& outTokens) const
{
    outTokens.Clear();
    String str(*this);
    char* ptr = const_cast<char*>(str.AsCharPtr());    
    char* end = ptr + strlen(ptr);
    while (ptr < end)
    {
        char* c;

        // skip white space
        while (*ptr && strchr(whiteSpace.AsCharPtr(), *ptr))
        {
            ptr++;
        }
        if (*ptr)
        {
            // check for fenced area
            if ((fence == *ptr) && (0 != (c = strchr(++ptr, fence))))
            {
                *c++ = 0;
                outTokens.Append(ptr);
                ptr = c;
            }
            else if (0 != (c = strpbrk(ptr, whiteSpace.AsCharPtr())))
            {
                *c++ = 0;
                outTokens.Append(ptr);
                ptr = c;
            }
            else
            {
                outTokens.Append(ptr);
                break;
            }
        }
    }
    return outTokens.Size();
}

//------------------------------------------------------------------------------
/**
    Slow version of Tokenize() with fence character. See above Tokenize()
    for details.
*/
Array<String>
String::Tokenize(const String& whiteSpace, char fence) const
{
    Array<String> tokens;
    this->Tokenize(whiteSpace, fence, tokens);
    return tokens;
}

//------------------------------------------------------------------------------
/**
    Extract a substring range.
*/
String
String::ExtractRange(IndexT fromIndex, SizeT numChars) const
{
//     ph_assert(from <= this->strLen);
//     ph_assert((from + numChars) <= this->strLen);
//     const char* str = this->AsCharPtr();
//     String newString;
//     newString.Set(&(str[from]), numChars);
//     return newString;

	return this->SubString(fromIndex, numChars);
}

//------------------------------------------------------------------------------
/**
    Extract a substring until the end of the original string.
*/
String
String::ExtractToEnd(IndexT fromIndex) const
{
//    return this->ExtractRange(fromIndex, this->strLen - fromIndex);

	return this->SubString(fromIndex/*, this->strLen - fromIndex*/);
}

//------------------------------------------------------------------------------
/**
    Terminates the string at the first occurance of one of the characters
    in charSet.
*/
void
String::Strip(const String& charSet)
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char* ptr = strpbrk(str, charSet.AsCharPtr());
    if (ptr)
    {
        *ptr = 0;
    }
    this->strLen = (SizeT) strlen(this->AsCharPtr());
}

//------------------------------------------------------------------------------
/**
	return start index of substring of c-style, or InvalidIndex if not found   Added  by  Li Dinghao
*/
IndexT
String::FindStringIndex(const char* s, IndexT startIndex) const
{
	ph_assert(startIndex < this->strLen);

	const char* this_ptr = this->AsCharPtr();
	int s_len = strlen(s);

	ph_assert(s_len);

	for (int i = startIndex; i < this->strLen - s_len ; ++i)
	{
		for (int idx = 0; this_ptr[i+idx] == s[idx]; ++idx)
		{
			if (idx == s_len - 1)
			{
				return i;
			}
		}
	}

	return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    return index(first occurrence) of character in string, or InvalidIndex if not found.
*/
IndexT
String::FindCharIndex(char c, IndexT startIndex) const
{
    if (this->strLen > 0)
    {
        ph_assert(startIndex < this->strLen);
        const char* ptr = strchr(this->AsCharPtr() + startIndex, c);
        if (ptr)
        {
            return IndexT(ptr - this->AsCharPtr());
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    return index of the first occurrence in the string of any character from 
	charSet searched for.  Added  by  Li Dinghao
*/
IndexT
String::FindCharIndexFromSet(const String& charSet, IndexT startIndex) const
{
	char* str = const_cast<char*>(this->AsCharPtr());
	char* ptr = strpbrk(str, charSet.AsCharPtr());
	if (ptr)
	{
		return IndexT(ptr - this->AsCharPtr());
	}
	return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
	return index(last occurrence) of character in string, or InvalidIndex if not
	found     Added  by  Li Dinghao
*/
IndexT
String::FindCharLastIndex(char c) const
{
	if (this->strLen > 0)
	{
		const char* ptr = strrchr(this->AsCharPtr(), c);
		if (ptr)
		{
			return IndexT(ptr - this->AsCharPtr());
		}
	}
	return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    Removes all characters in charSet from the left side of the string.
*/
void
String::TrimLeft(const String& charSet)
{
    ph_assert(charSet.IsValid());
    if (this->IsValid())
    {
        SizeT charSetLen = charSet.strLen;
        IndexT thisIndex = 0;
        bool stopped = false;
        while (!stopped && (thisIndex < this->strLen))
        {
            IndexT charSetIndex;
            bool match = false;
            for (charSetIndex = 0; charSetIndex < charSetLen; charSetIndex++)
            {
                if ((*this)[thisIndex] == charSet[charSetIndex])
                {
                    // a match
                    match = true;
                    break;
                }
            }
            if (!match)
            {
                // stop if no match
                stopped = true;
            }
            else
            {
                // a match, advance to next character
                ++thisIndex;
            }
        }
        String trimmedString(&(this->AsCharPtr()[thisIndex]));
        *this = trimmedString;
    }
}

//------------------------------------------------------------------------------
/**
    Removes all characters in charSet from the right side of the string.
*/
void
String::TrimRight(const String& charSet)
{
    ph_assert(charSet.IsValid());
    if (this->IsValid())
    {
        SizeT charSetLen = charSet.strLen;
        int thisIndex = this->strLen - 1;   // NOTE: may not be unsigned (thus not IndexT!)
        bool stopped = false;
        while (!stopped && (thisIndex >= 0))
        {
            IndexT charSetIndex;
            bool match = false;
            for (charSetIndex = 0; charSetIndex < charSetLen; charSetIndex++)
            {
                if ((*this)[thisIndex] == charSet[charSetIndex])
                {
                    // a match
                    match = true;
                    break;
                }
            }
            if (!match)
            {
                // stop if no match
                stopped = true;
            }
            else
            {
                // a match, advance to next character
                --thisIndex;
            }
        }
        String trimmedString;
        trimmedString.Set(this->AsCharPtr(), thisIndex + 1);
        *this = trimmedString;
    }
}

//------------------------------------------------------------------------------
/**
    Trim both sides of a string.
*/
void
String::Trim(const String& charSet)
{
    this->TrimLeft(charSet);
    this->TrimRight(charSet);
}

//------------------------------------------------------------------------------
/**
    Substitute every occurance of matchStr with substStr.
*/
void
String::SubstituteString(const char* matchStr, const char* substStr)
{
    ph_assert( matchStr && substStr);

    const char* ptr = this->AsCharPtr();
    SizeT matchStrLen = strlen(matchStr);
	SizeT substStrLen = strlen(substStr);
    String dest;

    // walk original string for occurances of str
    const char* occur;
    while (0 != (occur = strstr(ptr, matchStr )))
    {
        // append string fragment until match
        dest.AppendRange(ptr, SizeT(occur - ptr));

        // append replacement string
        if (substStrLen > 0)
            dest.Append(substStr);

        // adjust source pointer
        ptr = occur + matchStrLen;
    }
    dest.Append(ptr);
    *this = dest;
}

//------------------------------------------------------------------------------
/**
    Substitute every occurance of matchStr with substStr.
*/
void
String::SubstituteString(const String& matchStr, const String& substStr)
{
    ph_assert(matchStr.IsValid());

    const char* ptr = this->AsCharPtr();
    SizeT matchStrLen = matchStr.strLen;
    String dest;

    // walk original string for occurances of str
    const char* occur;
    while (0 != (occur = strstr(ptr, matchStr.AsCharPtr())))
    {
        // append string fragment until match
        dest.AppendRange(ptr, SizeT(occur - ptr));

        // append replacement string
        if (substStr.Length() > 0)
        {
            dest.Append(substStr);
        }

        // adjust source pointer
        ptr = occur + matchStrLen;
    }
    dest.Append(ptr);
    *this = dest;
}

//------------------------------------------------------------------------------
/**
    Return a String object containing the last directory of the path, i.e.
    a category.

    - 17-Feb-04     floh    fixed a bug when the path ended with a slash
*/
String
String::ExtractLastDirName() const
{
    String pathString(*this);
    char* lastSlash = pathString.GetLastSlash();
    
    // special case if path ends with a slash
    if (lastSlash)
    {
        if (0 == lastSlash[1])
        {
            *lastSlash = 0;
            lastSlash = pathString.GetLastSlash();
        }

        char* secLastSlash = 0;
        if (0 != lastSlash)
        {
            *lastSlash = 0; // cut filename
            secLastSlash = pathString.GetLastSlash();
            if (secLastSlash)
            {
                *secLastSlash = 0;
                return String(secLastSlash+1);
            }
        }
    }
    return "";
}

//------------------------------------------------------------------------------
/**
    Return a String object containing the part before the last
    directory separator.
    
    NOTE: I left my fix in that returns the last slash (or colon), this was 
    necessary to tell if a dirname is a normal directory or an assign.     

    - 17-Feb-04     floh    fixed a bug when the path ended with a slash
*/
String
String::ExtractDirName() const
{
    String pathString(*this);
    char* lastSlash = pathString.GetLastSlash();

    // special case if path ends with a slash
    if (lastSlash)
    {
        if (0 == lastSlash[1])
        {
            *lastSlash = 0;
            lastSlash = pathString.GetLastSlash();
        }
        if (lastSlash)
        {
            *++lastSlash = 0;
        }
    }
    pathString.strLen = (SizeT) strlen(pathString.AsCharPtr());
    return pathString;
}

//------------------------------------------------------------------------------
/**
    Pattern-matching, TCL-style.
*/
bool
String::MatchPattern(const String& string, const String& pattern)
{
    const char* str = string.AsCharPtr();
    const char* pat = pattern.AsCharPtr();

    char c2;
    bool always = true;
    while (always != false)
    {
        if (*pat == 0) 
        {
            if (*str == 0) return true;
            else           return false;
        }
        if ((*str == 0) && (*pat != '*')) return false;
        if (*pat=='*') 
        {
            pat++;
            if (*pat==0) return true;
            while (always)
            {
                if (String::MatchPattern(str, pat)) return true;
                if (*str==0) return false;
                str++;
            }
        }
        if (*pat=='?') goto match;
        if (*pat=='[') 
        {
            pat++;
            while (always) 
            {
                if ((*pat==']') || (*pat==0)) return false;
                if (*pat==*str) break;
                if (pat[1] == '-') 
                {
                    c2 = pat[2];
                    if (c2==0) return false;
                    if ((*pat<=*str) && (c2>=*str)) break;
                    if ((*pat>=*str) && (c2<=*str)) break;
                    pat+=2;
                }
                pat++;
            }
            while (*pat!=']') 
            {
                if (*pat==0) 
                {
                    pat--;
                    break;
                }
                pat++;
            }
            goto match;
        }
    
        if (*pat=='\\') 
        {
            pat++;
            if (*pat==0) return false;
        }
        if (*pat!=*str) return false;

match:
        pat++;
        str++;
    }
    // can't happen
    return false;
}

//------------------------------------------------------------------------------
/**
	pattern matching extended   Added by Dinghao Li
*/
bool
String::MatchPatternEx(const String& str, const String& pattern, bool caseSensitive)
{
	String tmpStr(str);
	String tmpPattern(pattern);

	if (!caseSensitive)
	{
		tmpStr.ToLower();
		tmpPattern.ToLower();
	}

	return MatchPattern(tmpStr, tmpPattern);
}

//------------------------------------------------------------------------------
bool
String::StartsWith(const String& str, const String& pattern, bool caseSensitive)
{
	String tmpStr(str);
	String tmpPattern(pattern);

	if (!caseSensitive)
	{
		tmpStr.ToLower();
		tmpPattern.ToLower();
	}

	uint thisLen = tmpStr.Length();
	uint patternLen = tmpPattern.Length();

	if (thisLen < patternLen || 0 == patternLen )
		return false;

	return ( tmpStr.ExtractRange(0, patternLen) == tmpPattern );
}
//------------------------------------------------------------------------------
bool
String::EndsWith(const String& str, const String& pattern, bool caseSensitive)
{
	String tmpStr(str);
	String tmpPattern(pattern);

	if (!caseSensitive)
	{
		tmpStr.ToLower();
		tmpPattern.ToLower();
	}

	uint thisLen = tmpStr.Length();
	uint patternLen = tmpPattern.Length();

	if (thisLen < patternLen || 0 == patternLen )
		return false;

	return ( tmpStr.ExtractToEnd(thisLen - patternLen) == tmpPattern );

}

//------------------------------------------------------------------------------
/**
*/
void
String::ReplaceChars(const String& charSet, char replacement)
{
    ph_assert(charSet.IsValid());
    char* ptr = const_cast<char*>(this->AsCharPtr());
    char c;
    while (0 != (c = *ptr))
    {
        if (strchr(charSet.AsCharPtr(), c))
        {
            *ptr = replacement;
        }
        ptr++;
    }
}

Vector3 
String::AsVector3() const
{
	Array<String> tokens(3, 0); 
	this->Tokenize(", \t\n", tokens);

	ph_assert (tokens.Size() == 3);

	Vector3 v(tokens[0].AsFloat(),  tokens[1].AsFloat(),  tokens[2].AsFloat());
	return v;
}

Vector2 
String::AsVector2() const
{
	Array<String> tokens(2, 0); 
	this->Tokenize(", \t\n", tokens);

	ph_assert (tokens.Size() == 2);
	
	Vector2 v(tokens[0].AsFloat(),  tokens[1].AsFloat());
	return v;
}

Vector4 
String::AsVector4() const
{
	Array<String> tokens(4, 0); 
	this->Tokenize(", \t\n", tokens);

	ph_assert (tokens.Size() == 4);

	Vector4 v(tokens[0].AsFloat(),  tokens[1].AsFloat(),
					tokens[2].AsFloat(),  tokens[3].AsFloat());
	return v;
}

Quaternion 
String::AsQuaternion() const
{
	Array<String> tokens(4, 0); 
	this->Tokenize(", \t\n", tokens);

	ph_assert (tokens.Size() == 4);

	Quaternion v(tokens[0].AsFloat(),  tokens[1].AsFloat(),
					   tokens[2].AsFloat(),  tokens[3].AsFloat());
	return v;
}

Matrix3 
String::AsMatrix3() const
{
	Array<String> tokens(9, 0); 
	this->Tokenize(", \t\n", tokens);

	ph_assert (tokens.Size() == 9);

	Matrix3 v(tokens[0].AsFloat(),  tokens[1].AsFloat(),  tokens[2].AsFloat(),  
					tokens[3].AsFloat(),  tokens[4].AsFloat(),  tokens[5].AsFloat(),
					tokens[6].AsFloat(),  tokens[7].AsFloat(),  tokens[8].AsFloat());
	return v;
}

Matrix4 
String::AsMatrix4() const
{
	Array<String> tokens(16, 0); 
	this->Tokenize(", \t\n", tokens);

	ph_assert (tokens.Size() == 16);

	Matrix4 v(tokens[0].AsFloat(),  tokens[1].AsFloat(),  tokens[2].AsFloat(),  tokens[3].AsFloat(),
					tokens[4].AsFloat(),  tokens[5].AsFloat(),  tokens[6].AsFloat(),  tokens[7].AsFloat(),
					tokens[8].AsFloat(),  tokens[9].AsFloat(),  tokens[10].AsFloat(),  tokens[11].AsFloat(),
					tokens[12].AsFloat(),  tokens[13].AsFloat(),  tokens[14].AsFloat(),  tokens[15].AsFloat());
	return v;
}

Colour 
String::AsColourValue() const
{
	Array<String> tokens(4, 0); 
	this->Tokenize(", \t\n", tokens);

	ph_assert (tokens.Size() == 4);

	Colour v(tokens[0].AsFloat(),  tokens[1].AsFloat(),  tokens[2].AsFloat(),  tokens[3].AsFloat());
	
	return v;
} 
//------------------------------------------------------------------------------
String
String::Concatenate(const Array<String>& strArray, const String& whiteSpace)
{
    String res;
    res.Reserve(256);
    IndexT i;
    SizeT num = strArray.Size();
    for (i = 0; i < num; i++)
    {
        res.Append(strArray[i]);
        if ((i + 1) < num)
        {
            res.Append(whiteSpace);
        }
    }
    return res;
}

//------------------------------------------------------------------------------
void
String::SetCharPtr(const char* s)
{
    SizeT len = 0;
    if (s)
    {
        len = (SizeT) strlen((const char*)s);
    }
    this->Set(s, len);
}

//------------------------------------------------------------------------------
/**
    Terminates the string at the given index.
*/
void
String::TerminateAtIndex(IndexT index)
{
    ph_assert(index < this->strLen);
    char* ptr = const_cast<char*>(this->AsCharPtr());
    ptr[index] = 0;
    this->strLen = (SizeT) strlen(ptr);
}

//------------------------------------------------------------------------------
void
String::StripFileExtension()
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char* ext = strrchr(str, '.');
    if (ext)
    {
        *ext = 0;
        this->strLen = (SizeT) strlen(this->AsCharPtr());
    }
}
//------------------------------------------------------------------------------
void
String::ToLower()
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char c;
    while (0 != (c = *str))
    {
        *str++ = (char) tolower(c);
    }
}

//------------------------------------------------------------------------------
void
String::ToUpper()
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char c;
    while (0 != (c = *str))
    {
        *str++ = (char) toupper(c);
    }
}

//------------------------------------------------------------------------------
void
String::FirstCharToUpper()
{
    char* str = const_cast<char*>(this->AsCharPtr());    
    *str = (char) toupper(*str);
}

//------------------------------------------------------------------------------
/**
    Returns true if string contains one of the characters from charset.
*/
bool
String::ContainsCharFromSet(const String& charSet) const
{
    char* str = const_cast<char*>(this->AsCharPtr());
    char* ptr = strpbrk(str, charSet.AsCharPtr());
    return (0 != ptr);
}

//------------------------------------------------------------------------------
/**
    @return     string representing the filename extension (maybe empty)
*/
String
String::GetFileExtension() const
{
    const char* str = this->AsCharPtr();
    const char* ext = strrchr(str, '.');
    if (ext)
    {
        ext++;
        return String(ext);
    }
    return String("");
}

//------------------------------------------------------------------------------
/**
    Get a pointer to the last directory separator.
*/
char*
String::GetLastSlash() const
{
    const char* s = this->AsCharPtr();
    const char* lastSlash = strrchr(s, '/');
    if (0 == lastSlash) lastSlash = strrchr(s, '\\');
    if (0 == lastSlash) lastSlash = strrchr(s, ':');
    return const_cast<char*>(lastSlash);
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsValidBool() const
{
    static const char* bools[] = {
        "no", "yes", "off", "on", "false", "true", 0
    };
    IndexT i = 0;
    while (bools[i] != 0)
    {
        if (0 == stricmp(bools[i], this->AsCharPtr()))
        {
            return true;
        }
        i++;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Returns content as integer. Note: this method doesn't check whether the
    contents is actually a valid integer. Use the IsValidInteger() method
    for this!
*/
int
String::AsInt() const
{
    return atoi(this->AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Returns content as unsigned long integer.      Added by Li Dinghao
*/
unsigned long
String::AsUint32() const
{
    return strtoul(this->AsCharPtr(), NULL, 10);
}

//------------------------------------------------------------------------------
/**
    Returns content as float. 
*/
float
String::AsFloat() const
{
	return	float(atof(this->AsCharPtr()));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::AsBool() const
{
    static const char* bools[] = {
        "no", "yes", "off", "on", "false", "true", "0","1",0
    };
    IndexT i = 0;
    while (bools[i] != 0)
    {
        if (0 == stricmp(bools[i], this->AsCharPtr()))
        {
            return 1 == (i & 1);
        }
        i++;
    }
    n_error("Invalid string value for bool!");
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsDigit(char c)
{
    return (0 != isdigit(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsAlpha(char c)
{
    return (0 != isalpha(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsAlNum(char c)
{
    return (0 != isalnum(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsLower(char c)
{
    return (0 != islower(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
bool
String::IsUpper(char c)
{
    return (0 != isupper(int(c)));
}

//------------------------------------------------------------------------------
/**
*/
int
String::StrCmp(const char* str0, const char* str1)
{
    ph_assert(str0 && str1);
    return strcmp(str0, str1);
}

//------------------------------------------------------------------------------
int
String::StrLen(const char* str)
{
    ph_assert(str);
    return strlen(str);
}

//------------------------------------------------------------------------------
const char*
String::StrChr(const char* str, int c)
{
    ph_assert(str);
    return strchr(str, c);
}

//------------------------------------------------------------------------------
const char*
String::StrChrEx(const char *first, SizeT n, const char& ch)
{	// look for _Ch in [_First, _First + _Count)
	ph_assert( first );
	for (; 0 < n; --n, ++first)
		if ( *first == ch )
			return (first);
	return (0);
}

//------------------------------------------------------------------------------
Dictionary<String,String>
String::ParseKeyValuePairs(const String& str)
{
    Dictionary<String,String> res;
    Array<String> tokens = str.Tokenize(" \t\n=", '"');
    ph_assert(0 == (tokens.Size() & 1)); // num tokens must be even
    IndexT i;
    for (i = 0; i < tokens.Size(); i += 2)
    {
        res.Add(tokens[i], tokens[i + 1]);
    }
    return res;
}

//------------------------------------------------------------------------------
void 
String::ChangeFileExtension(const String& newExt)
{
    this->StripFileExtension();
    this->Append("." + newExt);
}

//------------------------------------------------------------------------------
String::size_type
String::rfind(const char* ptr, size_type offset, size_type n) const
{
    ph_assert( ptr && 0<=n );

	if (n == 0)
		return (offset < this->strLen ? offset : this->strLen);	// null always matches

	if (n <= this->strLen)
	{	// room for match, look for it
		const char* u_ptr = this->AsCharPtr() + (offset < this->strLen - n ? this->strLen : this->strLen - n);
		for (; ; --u_ptr)
			if ( (*u_ptr == *ptr) && memcmp(u_ptr, ptr, n) == 0 )
				return (u_ptr - this->AsCharPtr());	// found a match
			else if (u_ptr == this->AsCharPtr())
				break;	// at beginning, no more chance for match
	}
	return (npos);	// no match
}

//------------------------------------------------------------------------------
String::size_type
String::find( const char* ptr, size_type offset, size_type n ) const
{
	ph_assert( ptr && 0<=n );

	if ( n == 0 && offset <= this->strLen )
		return (offset);	// null string always matches (if inside string)

	size_type num;
	if (offset < this->strLen  && n <= (num = this->strLen - offset))
	{	// room for match, look for it
		const char *u_ptr, *v_ptr;
		for (num -= n - 1, v_ptr = this->AsCharPtr() + offset;
			(u_ptr = StrChrEx(v_ptr, num, *ptr)) != 0;
			num -= u_ptr - v_ptr + 1, v_ptr = u_ptr + 1)
			if (memcmp(u_ptr, ptr, n)== 0)
				return (u_ptr - this->AsCharPtr());	// found a match
	}

	return (npos);	// no match
}


} // namespace System