///PHILOTES Source Code.  (C)2012 PhiloLabs

//------------------------------------------------------------------------------
//    @class String
//
//     PHILOTES's universal string class. An empty string object is always 32
//     bytes big. The string class tries to avoid costly heap allocations
//     with the following tactics:
// 
//     - a local embedded buffer is used if the string is short enough
//     - if a heap buffer must be allocated, care is taken to reuse an existing
//       buffer instead of allocating a new buffer if possible (usually if
//       an assigned string fits into the existing buffer, the buffer is reused
//       and not re-allocated)
// 
//     Heap allocations are performed through a local heap which should
//     be faster then going through the process heap.
// 
//     Besides the usual string manipulation methods, the String class also
//     offers methods to convert basic PHILOTES datatypes from and to string,
//     and a group of methods which manipulate filename strings.
//
// 		注意：
// 
// 		String 测试报告，整体效率远高于STL ；
// 		operator + 要少用，处理大量数据用Append ；
// 		新增hash_code_std_style()，与自带的HashCode()效率相当；
// 
// 		Added by Li
//------------------------------------------------------------------------------

#pragma once

#include "core/types.h"
#include "util/array.h"
#include "util/dictionary.h"

#include "math/vector2.h"
#include "math/vector3.h"
#include "math/vector4.h"
#include "math/quaternion.h"
#include "math/matrix3.h"
#include "math/matrix4.h"

#include "util/colourValue.h"

#include <string>//TODO: std::string  LEFT TEMPORARILY !!! by Li

//------------------------------------------------------------------------------
namespace Philo
{
class _PhiloCommonExport String
{
public:

    // new重载暂时删去

    String();
    String(const String& rhs);
    String(const char* cStr);
	String(const char* c_ptr, SizeT len);	//added by Li
    ~String();

    void operator=(const String& rhs);
    void operator=(const char* cStr);
    void operator +=(const String& rhs);

	char operator[](IndexT i) const;
	char& operator[](IndexT i);

    _PhiloCommonExport friend bool operator ==(const String& a, const String& b);
    _PhiloCommonExport friend bool operator ==(const String& a, const char* cStr);
    _PhiloCommonExport friend bool operator ==(const char* cStr, const String& a);
    _PhiloCommonExport friend bool operator !=(const String& a, const String& b);
	_PhiloCommonExport friend bool operator <(const String& a, const String& b);
	_PhiloCommonExport friend bool operator >(const String& a, const String& b);
	_PhiloCommonExport friend bool operator <=(const String& a, const String& b);
	_PhiloCommonExport friend bool operator >=(const String& a, const String& b);

    void		Reserve(SizeT newSize);																		// reserve internal buffer size to prevent heap allocs
    SizeT		Length() const;																					// return length of string
    void		Clear();																								// clear the string
    bool		IsEmpty() const;																					// return true if string object is empty
    bool		IsValid() const;																					// return true if string object is not empty
    bool		CopyToBuffer(char* buf, SizeT bufSize) const;											// copy to char buffer (return false if buffer is too small)

	void		AppendRange(const char* str, SizeT numChars);										// append a range of characters
    void		Append(const String& str);																	// append string
    void		Append(const char* str);																		// append c-string

	void		ToLower();																							// convert string to lower case
	void		ToUpper();																							// convert string to upper case
    void		FirstCharToUpper();																				// convert first char of string to upper case

	String		SubString(IndexT offset = 0, SizeT n = InvalidIndex) const;							// this substring is the character sequence that starts at character position pos and has a length of n characters   by  Li
	String		ExtractRange(IndexT fromIndex, SizeT numChars) const;								// extract substring, the same as SubString  by  Li
	String		ExtractToEnd(IndexT fromIndex) const;													// extract substring to end of this string
	void		Strip(const String& charSet);																	// terminate string at first occurence of character in set

	IndexT	FindStringIndex(const String& s, IndexT startIndex = 0) const;						// return start index of substring, or InvalidIndex if not found   Modified  by  Li
	IndexT	FindStringIndex(const char* s, IndexT startIndex = 0) const;						// return start index of substring of c-style, or InvalidIndex if not found   Added  by  Li
	IndexT	FindCharIndex(char c, IndexT startIndex = 0) const;									// return index(first occurrence) of character in string, or InvalidIndex if not found
	IndexT	FindCharIndexFromSet(const String& charSet, IndexT startIndex = 0) const;	// return index of the first occurrence in the string of any character from charSet searched for    Added  by  Li
	IndexT	FindCharLastIndex(char c) const;															// return index(last occurrence) of character in string, or InvalidIndex if not found     Added  by  Li

	void		TerminateAtIndex(IndexT index);																// terminate string at given index
	bool		ContainsCharFromSet(const String& charSet) const;									// returns true if string contains any character from set
	void		TrimLeft(const String& charSet);																// delete characters from charset at left side of string
	void		TrimRight(const String& charSet);															// delete characters from charset at right side of string
	void		Trim(const String& charSet);																	// trim characters from charset at both sides of string

	void		SubstituteString(const char* matchStr, const char* substStr);					// substitute every occurance of a string with another string  TODO: needs optimization
	void		SubstituteString(const String& matchStr, const String& substStr);				// substitute every occurance of a string with another string
	void		SubstituteChar(char c, char subst);															// substiture every occurance of a character with another character
	bool		CheckValidCharSet(const String& charSet) const;										// return true if string only contains characters from charSet argument
	void		ReplaceChars(const String& charSet, char replacement);								// replace any char set character within a srtring with the replacement character

    SizeT					Tokenize(const String& whiteSpace, Array<String>& outTokens) const;												// tokenize string into a provided String array (faster if tokens array can be reused)   
    Array<String>		Tokenize(const String& whiteSpace) const;																					// tokenize string into a provided String array, SLOW since new array will be constructed
	SizeT					TokenizeEx(const String& whiteSpace, Array<String>& outTokens, unsigned int maxSplits = 0) const;		// tokenize string into a provided String array (faster if tokens array can be reused)   Added by Li
	Array<String>		TokenizeEx(const String& whiteSpace, unsigned int maxSplits = 0) const;											// tokenize string into a provided String array, SLOW since new array will be constructed    Added by Li
    SizeT					Tokenize(const String& whiteSpace, char fence, Array<String>& outTokens) const;								// tokenize string, keep strings within fence characters intact (faster if tokens array can be reused)
    Array<String>		Tokenize(const String& whiteSpace, char fence) const;																	// tokenize string, keep strings within fence characters intact, SLOW since new array will be constructed
   
	void __cdecl		Format(const char* fmtString, ...);																								// format string printf-style
	void __cdecl		FormatArgList(const char* fmtString, va_list argList);																		// format string printf-style with varargs list

    static String	Concatenate(const Array<String>& strArray, const String& whiteSpace);								// concatenate array of strings into new string
    static bool		MatchPattern(const String& str, const String& pattern);														// pattern matching, case sensitive true
	static bool		MatchPatternEx(const String& str, const String& pattern, bool caseSensitive = true);				// pattern matching extended   Added by Li
	static bool		StartsWith(const String& str, const String& pattern, bool caseSensitive = true);						// returns whether the string begins with the pattern passed in    Added by Li
	static bool		EndsWith(const String& str, const String& pattern, bool caseSensitive = true);						// returns whether the string ends with the pattern passed in   Added by Li

    IndexT	HashCode() const;					// return a 32-bit hash code for the string
	size_t		hash_code_std_style() const;	//computes a hash code for the string by pseudorandomizing transform in std style. (std::hash xhash hash_compare)      Added by Li

    void				SetCharPtr(const char* s);				// set content to char ptr
    void				Set(const char* ptr, SizeT length);		// set as char ptr, with explicit length
	const char*	AsCharPtr() const;							// return contents as character pointer
    
    void		SetInt(int val);		// set as int value
    void		SetFloat(float val);	// set as float value
    void		SetBool(bool val);		// set as bool value
    

	void SetVector2(const Vector2& v);
	void SetVector3(const Vector3& v);
	void SetVector4(const Vector4& v);
	void SetQuaternion(const Quaternion& v);
	void SetMatrix3(const Matrix3& v);
	void SetMatrix4(const Matrix4& v);
	void SetColourValue(const ColourValue& v);


    /// generic setter
    template<typename T> void Set(const T& t);


    void AppendInt(int val);
    void AppendFloat(float val);
    void AppendBool(bool val);

	void AppendVector2(const Vector2& v);
	void AppendVector3(const Vector3& v);
	void AppendVector4(const Vector4& v);
	void AppendQuaternion(const Quaternion& v);
	void AppendMatrix3(const Matrix3& v);
	void AppendMatrix4(const Matrix4& v);
	void AppendColourValue(const ColourValue& v);

    /// generic append
    template<typename T> void Append(const T& t);

    
    int					AsInt() const;			// return contents as integer
    unsigned long	AsUint32() const;		// return contents as unsigned long integer     added by Li
    float				AsFloat() const;		// return contents as float
    bool				AsBool() const;		// return contents as bool

	Vector2			AsVector2() const;
	Vector3			AsVector3() const;
	Vector4			AsVector4() const;
	Quaternion		AsQuaternion() const;
	Matrix3			AsMatrix3() const;
	Matrix4			AsMatrix4() const;
	ColourValue		AsColourValue() const;


    /// convert to "anything"
    template<typename T> T As() const;

    /// return true if the content is a valid integer
    bool IsValidInt() const;
    /// return true if the content is a valid float
    bool IsValidFloat() const;
    /// return true if the content is a valid bool
    bool IsValidBool() const;

	bool IsValidVector2() const;
	bool IsValidVector3() const;
	bool IsValidVector4() const;
	bool IsValidQuaternion() const;
	bool IsValidMatrix3() const;
	bool IsValidMatrix4() const;
	bool IsValidColourValue() const;

    //#endif
    /// generic valid checker
    template<typename T> bool IsValid() const;

    
    static String	FromInt(int i);			// construct a string from an int
    static String	FromFloat(float f);	// construct a string from a float
    static String	FromBool(bool b);		// construct a string from a bool

	static String	FromVector2(const Vector2& v);
	static String	FromVector3(const Vector3& v);
	static String	FromVector4(const Vector4& v);
	static String	FromQuaternion(const Quaternion& v);
	static String	FromMatrix3(const Matrix3& v);
	static String	FromMatrix4(const Matrix4& v);
	static String	FromColourValue(const ColourValue& v);

    /// convert from "anything"
    template<typename T> static String From(const T& t);

    
    String		GetFileExtension() const;											// get filename extension without dot
    bool		CheckFileExtension(const String& ext) const;				// check file extension
    void		ConvertBackslashes();												// convert backslashes to slashes
    void		StripFileExtension();													// remove file extension
    void		ChangeFileExtension(const String& newExt);			// change file extension
	String		ExtractFileName() const;											// extract the part after the last directory separator
    String		ExtractLastDirName() const;										// extract the last directory of the path
    String		ExtractDirName() const;											// extract the part before the last directory separator
    String		ExtractToLastSlash() const;										// extract path until last slash
	void		SplitFilename(String& outBasename, String& outPath);	// split filename to path and basename  added by Li
    void		ReplaceIllegalFilenameChars(char replacement);				// replace illegal filename characters


    static bool				IsDigit(char c);													// test if provided character is a digit (0..9) 
    static bool				IsAlpha(char c);													// test if provided character is an alphabet character (A..Z, a..z)
    static bool				IsAlNum(char c);													// test if provided character is an alpha-numeric character (A..Z,a..z,0..9)
    static bool				IsLower(char c);													// test if provided character is a lower case character
    static bool				IsUpper(char c);													// test if provided character is an upper-case character

    static int				StrCmp(const char* str0, const char* str1);				// lowlevel string compare wrapper function
    static int				StrLen(const char* str);										// lowlevel string length function
    static const char*	StrChr(const char* str, int c);								// find character in string
	static const char*	StrChrEx(const char *first, SizeT n, const char& ch);	// find character in string, extended    added by Li


    /// parse key/value pair string ("key0=value0 key1=value1")
    static Dictionary<String,String> ParseKeyValuePairs(const String& str);


	//--------------------------------------------------------------------------------------------------------------------------------------------
	// methods in std style, the PERFORMANCE is GOOD, but NOT RECOMMENDED in PHILOTES !! by Li
	//--------------------------------------------------------------------------------------------------------------------------------------------
	typedef SizeT size_type;
	typedef char value_type;
	static const size_type npos;

	String(const std::string& rhs);															//TODO: construct from std::string, LEFT TEMPORARILY !!! by Li
	std::string AsStdString() const{ return std::string(this->AsCharPtr()); }	//TODO: return std::string, LEFT TEMPORARILY !!! by Li

	const char*	c_str() const { return this->AsCharPtr(); }
	size_type		length() const { return this->strLen; }
	size_type		size() const { return this->strLen; }
	bool				empty () const{ return IsEmpty(); }
	String&			append ( const String& str ) { Append(str); return(*this); }
	String&			append ( const char* s, size_type n ) { AppendRange(s, n); return(*this); }
	String&			append ( const char* s ) { Append(s); return(*this); }
	// 	String&			append ( const String& str, size_type pos, size_type n );
	// 	String&			append ( size_t n, char c );
	String				substr( size_type offset = 0, size_type n = npos) const { return SubString(offset, n); }
	size_type		rfind( const char* ptr, size_type offset, size_type n) const; //BEST PERFORMANCE!
	size_type		rfind( char ch, size_type offset = npos ) const { return ( rfind((const char *)&ch, offset, 1) ); }
	size_type		rfind( const char* ptr, size_type offset = npos) const { return ( rfind(ptr, offset, strlen(ptr)) ); }
	size_type		rfind( const String& right, size_type offset = npos) const { return ( rfind(right.AsCharPtr(), offset, right.size()) ); }
	size_type		find( const char* ptr, size_type offset, size_type n) const;
	size_type		find( const char* ptr, size_type offset = 0) const { return ( find(ptr, offset, strlen(ptr)) ); }
	size_type		find( char ch, size_type offset = 0) const { return ( FindCharIndex(ch, offset) ); }
	size_type		find( const String& right, size_type offset = 0) const { return ( find(right.AsCharPtr(), offset, right.size()) ); }
	size_type		find_first_of( char ch, size_type offset = 0) const { return FindCharIndex(ch, offset); }


	//--------------------------------------------------------------------------------------------------------------------------------------------
	static String BLANK;	//TODO: Ogre uses it


private:
    
    void		Delete();							// delete contents
    char*		GetLastSlash() const;			// get pointer to last directory separator
    void		Alloc(SizeT size);					// allocate the string buffer (discards old content)
    void		Realloc(SizeT newSize);			// (re-)allocate the string buffer (copies old content)

    enum
    {
        LocalStringSize = 20,
    };
    char* heapBuffer;
    char localBuffer[LocalStringSize];
    SizeT strLen;
    SizeT heapBufferSize;
};

//------------------------------------------------------------------------------
inline
String::String() :
    heapBuffer(0),
    strLen(0),
    heapBufferSize(0)
{
    this->localBuffer[0] = 0;
}

//------------------------------------------------------------------------------
inline void
String::Delete()
{
    if (this->heapBuffer)
    {
        free((void*) this->heapBuffer);
        this->heapBuffer = 0;
    }
    this->localBuffer[0] = 0;
    this->strLen = 0;
    this->heapBufferSize = 0;
}

//------------------------------------------------------------------------------
inline 
String::~String()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
    Allocate a new heap buffer, discards old contents.
*/
inline void
String::Alloc(SizeT newSize)
{
    ph_assert(newSize > (this->strLen + 1));
    ph_assert(newSize > this->heapBufferSize);

    // free old buffer
    if (this->heapBuffer)
    {
        free((void*) this->heapBuffer);
        this->heapBuffer = 0;
    }

    // allocate new buffer
    this->heapBuffer = (char*) malloc (newSize);
    this->heapBufferSize = newSize;
    this->localBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
    (Re-)allocate external buffer and copy existing string contents there.
*/
inline void
String::Realloc(SizeT newSize)
{
    ph_assert(newSize > (this->strLen + 1));
    ph_assert(newSize > this->heapBufferSize);

    // allocate a new buffer
    char* newBuffer = (char*) malloc (newSize);

    // copy existing contents there...
    if (this->strLen > 0)
    {
        const char* src = this->AsCharPtr();
        memcpy(newBuffer, src, this->strLen);
    }
    newBuffer[this->strLen] = 0;

    // assign new buffer
    if (this->heapBuffer)
    {
        free ((void*) this->heapBuffer);
        this->heapBuffer = 0;
    }
    this->localBuffer[0] = 0;
    this->heapBuffer = newBuffer;
    this->heapBufferSize = newSize;
}

//------------------------------------------------------------------------------
/**
    Reserves internal space to prevent excessive heap re-allocations.
    If you plan to do many Append() operations this may help alot.
*/
inline void
String::Reserve(SizeT newSize)
{
    if (newSize > this->heapBufferSize)
    {
        this->Realloc(newSize);
    }
}

//------------------------------------------------------------------------------
inline void
String::SetInt(int val)
{
    this->Format("%d", val);
}

//------------------------------------------------------------------------------
inline void
String::SetFloat(float val)
{
    this->Format("%.6f", val);
}

//------------------------------------------------------------------------------
inline void
String::SetBool(bool val)
{
    if (val)
    {
        this->SetCharPtr("true");
    }
    else
    {
        this->SetCharPtr("false");
    }
}

inline void
String::SetVector2(const Vector2& v)
{
	this->Format("%.6f %.6f",v.x,v.y);
}

inline void
String::SetVector3(const Vector3& v)
{
	this->Format("%.6f %.6f %.6f",v.x,v.y,v.z);
}

inline void
String::SetVector4(const Vector4& v)
{
	this->Format("%.6f %.6f %.6f %.6f",v.x,v.y,v.z,v.w);
}

inline void
String::SetQuaternion(const Quaternion& v)
{
	this->Format("%.6f %.6f %.6f %.6f",v.w,v.x,v.y,v.z);
}

inline void
String::SetMatrix3(const Matrix3& val)
{
	this->Format("%.6f %.6f %.6f "
				 "%.6f %.6f %.6f "
				 "%.6f %.6f %.6f ",
		val[0][0],
		val[0][1],           
		val[0][2],            
		val[1][0],            
		val[1][1],            
		val[1][2],            
		val[2][0],            
		val[2][1],            
		val[2][2]);
}

inline void
String::SetMatrix4(const Matrix4& val)
{
	this->Format("%.6f %.6f %.6f %.6f "
				 "%.6f %.6f %.6f %.6f "
				 "%.6f %.6f %.6f %.6f "
				 "%.6f %.6f %.6f %.6f",
		val[0][0],
		val[0][1],
		val[0][2],
		val[0][3],
		val[1][0],
		val[1][1],
		val[1][2],
		val[1][3],
		val[2][0],
		val[2][1],
		val[2][2],
		val[2][3],
		val[3][0],
		val[3][1],
		val[3][2],
		val[3][3]);
}

inline void
String::SetColourValue(const ColourValue& v)
{
	this->Format("%.6f %.6f %.6f %.6f",v.r,v.g,v.b,v.a);
}
    
//------------------------------------------------------------------------------
inline
String::String(const char* str) :
    heapBuffer(0),
    strLen(0),
    heapBufferSize(0)
{
    this->localBuffer[0] = 0;
    this->SetCharPtr(str);
}
//------------------------------------------------------------------------------
inline
String::String(const String& rhs) :
    heapBuffer(0),
    strLen(0),
    heapBufferSize(0)
{
    this->localBuffer[0] = 0;
    this->SetCharPtr(rhs.AsCharPtr());
}
//------------------------------------------------------------------------------
inline
String::String(const std::string& rhs) :
heapBuffer(0),
strLen(0),
heapBufferSize(0)
{
	this->localBuffer[0] = 0;
	this->SetCharPtr(rhs.c_str());
}
//------------------------------------------------------------------------------
inline
String::String(const char* c_ptr, SizeT len)://added by Li
heapBuffer(0),
strLen(0),
heapBufferSize(0)
{
	this->localBuffer[0] = 0;
	this->Set(c_ptr, len);
}
//------------------------------------------------------------------------------
inline const char*
String::AsCharPtr() const
{
    if (this->heapBuffer)
    {
        return this->heapBuffer;
    }
    else
    {
        return this->localBuffer;
    }
}
//------------------------------------------------------------------------------
/**
*/
inline void
String::operator=(const String& rhs)
{
    if (&rhs != this)
    {
        this->SetCharPtr(rhs.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
String::operator=(const char* cStr)
{
    this->SetCharPtr(cStr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
String::Append(const String& str)
{
    this->AppendRange(str.AsCharPtr(), str.strLen);
}

//------------------------------------------------------------------------------
/**
*/
inline void
String::Append(const char* str)
{
	ph_assert(0 != str);
	this->AppendRange(str, (SizeT) strlen(str));
}

//------------------------------------------------------------------------------
/**
*/
inline void
String::operator += (const String& rhs)
{
    this->Append(rhs);    
}

//------------------------------------------------------------------------------
/**
*/
inline char
String::operator[](IndexT i) const
{
    ph_assert(i <= this->strLen);
    return this->AsCharPtr()[i];
}

//------------------------------------------------------------------------------
/**
    NOTE: unlike the read-only indexer, the terminating 0 is NOT a valid
    part of the string because it may not be overwritten!!!
*/
inline char&
String::operator[](IndexT i)
{
    ph_assert(i <= this->strLen);
    return (char&)(this->AsCharPtr())[i];
}

//------------------------------------------------------------------------------
inline SizeT
String::Length() const
{
    return this->strLen;
}

//------------------------------------------------------------------------------
inline void
String::Clear()
{
    this->Delete();
}

//------------------------------------------------------------------------------
inline bool
String::IsEmpty() const
{
    return (0 == this->strLen);
}

//------------------------------------------------------------------------------
inline bool
String::IsValid() const
{
    return (0 != this->strLen);
}

//------------------------------------------------------------------------------
inline String
String::SubString(IndexT offset/* = 0*/, SizeT n/* = InvalidIndex*/) const
{
	ph_assert( this->strLen >= offset && 0 <= offset && InvalidIndex <= n );

	const char* str = this->AsCharPtr();

	if (InvalidIndex == n)
		return String( &(str[offset]) );

	return String( &(str[offset]), n );
}

//------------------------------------------------------------------------------
/**
    This method computes a hash code for the string. The method is
    compatible with the HashTable class.
*/
inline IndexT
String::HashCode() const
{
    IndexT hash = 0;
    const char* ptr = this->AsCharPtr();
    SizeT len = this->strLen;
    IndexT i;
    for (i = 0; i < len; i++)
    {
        hash += ptr[i];
        hash += hash << 10;
        hash ^= hash >>  6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    hash &= ~(1<<31);       // don't return a negative number (in case IndexT is defined signed)
    return hash;
}

//------------------------------------------------------------------------------
/**
	computes a hash code for the string by pseudorandomizing transform in std style. 
	(std::hash xhash hash_compare)  Added by Li
*/
inline size_t
String::hash_code_std_style() const
{
	size_t _Val = 2166136261U;
	const char* c_ptr = this->AsCharPtr();
	const char* c_ptr_end = c_ptr + this->strLen;
	while(c_ptr != c_ptr_end)
		_Val = 16777619U * _Val ^ (size_t)*c_ptr++;

	ldiv_t _Qrem = ldiv(_Val, 127773);
	_Qrem.rem = 16807 * _Qrem.rem - 2836 * _Qrem.quot;
	if (_Qrem.rem < 0)
		_Qrem.rem += 2147483647;

	return (size_t)_Qrem.rem;
}

//------------------------------------------------------------------------------
/**
	Modified by Dinghao Li
*/
static inline String
operator+(const String& s0, const String& s1)
{
    //String newString = s0;
	String newString(s0);
    //newString.Append(s1);
	newString.AppendRange(s1.AsCharPtr(), s1.Length());
    return newString;
}

//------------------------------------------------------------------------------
/**
	Added by Dinghao Li
*/
static inline String
operator+(const char* cStr, const String& str)
{
	String newString(cStr);
	newString.AppendRange(str.AsCharPtr(), str.Length());
	return newString;
}

//------------------------------------------------------------------------------
/**
	Added by Dinghao Li
*/
static inline String
operator+(const String& str, const char* cStr)
{
	String newString(str);
	newString.AppendRange(cStr, (SizeT)strlen(cStr));
	return newString;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
String::Tokenize(const String& whiteSpace, Array<String>& outTokens) const
{
	return this->TokenizeEx(whiteSpace, outTokens);
}

//------------------------------------------------------------------------------
/**
*/
inline Array<String>
String::Tokenize(const String& whiteSpace) const
{
	return this->TokenizeEx(whiteSpace);
}

//------------------------------------------------------------------------------
/**
    Return the index of a substring, or InvalidIndex if not found   Modified  by  Li Dinghao
*/
inline IndexT
String::FindStringIndex(const String& s, IndexT startIndex) const
{
	return FindStringIndex(s.AsCharPtr(), startIndex);
}

//------------------------------------------------------------------------------
/**
    Replace character with another.
*/
inline void
String::SubstituteChar(char c, char subst)
{
    char* ptr = const_cast<char*>(this->AsCharPtr());
    IndexT i;
    for (i = 0; i <= this->strLen; ++i)
    {
        if (ptr[i] == c)
        {
            ptr[i] = subst;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Converts backslashes to slashes.
*/
inline void
String::ConvertBackslashes()
{
    this->SubstituteChar('\\', '/');
}

//------------------------------------------------------------------------------
/**
*/
inline bool
String::CheckFileExtension(const String& ext) const
{
    return (this->GetFileExtension() == ext);
}

//------------------------------------------------------------------------------
/**
    Return a String object containing the part after the last
    path separator.
*/
inline String
String::ExtractFileName() const
{
    String pathString;
    const char* lastSlash = this->GetLastSlash();
    if (lastSlash)
    {
        pathString = &(lastSlash[1]);
    }
    else
    {
        pathString = *this;
    }
    return pathString;
}

//------------------------------------------------------------------------------
/**
    Return a path string object which contains of the complete path
    up to the last slash. Returns an empty string if there is no
    slash in the path.
*/
inline String
String::ExtractToLastSlash() const
{
    String pathString(*this);
    char* lastSlash = pathString.GetLastSlash();
    if (lastSlash)
    {
        lastSlash[1] = 0;
    }
    else
    {
        pathString = "";
    }
    return pathString;
}

//------------------------------------------------------------------------------
/**
	split filename to path and basename  Added by Li Dinghao
*/
inline void
String::SplitFilename(String& outBasename, String& outPath)
{
	this->ReplaceChars("\\", '/');

	outPath = this->ExtractToLastSlash();
	outBasename = this->ExtractFileName();
}

//------------------------------------------------------------------------------
/**
    Return true if the string only contains characters which are in the defined
    character set.
*/
inline bool
String::CheckValidCharSet(const String& charSet) const
{
    IndexT i;
    for (i = 0; i < this->strLen; i++)
    {
        if (InvalidIndex == charSet.FindCharIndex((*this)[i], 0))
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
String::IsValidInt() const
{
    return this->CheckValidCharSet(" \t-+01234567890");
}

//------------------------------------------------------------------------------
/**
    Note: this method is not 100% correct, it just checks for invalid characters.
*/
inline bool
String::IsValidFloat() const
{
    return this->CheckValidCharSet(" \t-+.e1234567890");
}

inline bool
String:: IsValidVector2() const
{
	return this->CheckValidCharSet(" \t-+.,e1234567890");
}

inline bool
String:: IsValidVector3() const
{
	return this->CheckValidCharSet(" \t-+.,e1234567890");
}

inline bool
String:: IsValidVector4() const
{
	return this->CheckValidCharSet(" \t-+.,e1234567890");
}

inline bool
String:: IsValidQuaternion() const
{
	return this->CheckValidCharSet(" \t-+.,e1234567890");
}

inline bool
String:: IsValidMatrix3() const
{
	return this->CheckValidCharSet(" \t-+.,e1234567890");
}

inline bool
String:: IsValidMatrix4() const
{
	return this->CheckValidCharSet(" \t-+.,e1234567890");
}

inline bool
String:: IsValidColourValue() const
{
	return this->CheckValidCharSet(" \t-+.,e1234567890");
}


//------------------------------------------------------------------------------
/**
*/
inline void
String::ReplaceIllegalFilenameChars(char replacement)
{
    this->ReplaceChars("\\/:*?\"<>|", replacement);
}

//------------------------------------------------------------------------------
/**
*/
inline String
String::FromInt(int i)
{	
    String str;
    str.SetInt(i);
    return str;
}

//------------------------------------------------------------------------------
/**
*/
inline String
String::FromFloat(float f)
{
    String str;
    str.SetFloat(f);
    return str;
}

//------------------------------------------------------------------------------
/**
*/
inline String
String::FromBool(bool b)
{
    String str;
    str.SetBool(b);
    return str;
}

inline String
String::FromVector2(const Vector2& v)
{
	String str;
	str.SetVector2(v);
	return str;
}

inline String
String::FromVector3(const Vector3& v)
{
	String str;
	str.SetVector3(v);
	return str;
}

inline String
String::FromVector4(const Vector4& v)
{
	String str;
	str.SetVector4(v);
	return str;
}

inline String
String::FromQuaternion(const Quaternion& v)
{
	String str;
	str.SetQuaternion(v);
	return str;
}

inline String
String::FromMatrix3(const Matrix3& v)
{
	String str;
	str.SetMatrix3(v);
	return str;
}

inline String
String::FromMatrix4(const Matrix4& v)
{
	String str;
	str.SetMatrix4(v);
	return str;
}

inline String
String::FromColourValue(const ColourValue& v)
{
	String str;
	str.SetColourValue(v);
	return str;
}

//#endif
    
//------------------------------------------------------------------------------
/**
*/
inline void
String::AppendInt(int val)
{
    this->Append(FromInt(val));
}

//------------------------------------------------------------------------------
/**
*/
inline void
String::AppendFloat(float val)
{
    this->Append(FromFloat(val));
}

//------------------------------------------------------------------------------
/**
*/
inline void
String::AppendBool(bool val)
{
    this->Append(FromBool(val));
}

inline void
String::AppendVector2(const Vector2& v)
{
	this->Append(FromVector2(v));
}

inline void
String::AppendVector3(const Vector3& v)
{
	this->Append(FromVector3(v));
}

inline void
String::AppendVector4(const Vector4& v)
{
	this->Append(FromVector4(v));
}

inline void
String::AppendQuaternion(const Quaternion& v)
{
	this->Append(FromQuaternion(v));
}

inline void
String::AppendMatrix3(const Matrix3& v)
{
	this->Append(FromMatrix3(v));
}

inline void
String::AppendMatrix4(const Matrix4& v)
{
	this->Append(FromMatrix4(v));
}

inline void
String::AppendColourValue(const ColourValue& v)
{
	this->Append(FromColourValue(v));
}

//#endif
//------------------------------------------------------------------------------
/**
*/
inline bool 
operator==(const String& a, const String& b)
{
	return strcmp(a.AsCharPtr(), b.AsCharPtr()) == 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
operator==(const String& a, const char* cStr)
{
	ph_assert(0 != cStr);
	return strcmp(a.AsCharPtr(), cStr) == 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
operator==(const char* cStr, const String& b)
{
	ph_assert(0 != cStr);
	return strcmp(cStr, b.AsCharPtr()) == 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
operator != (const String& a, const String& b)
{
	return strcmp(a.AsCharPtr(), b.AsCharPtr()) != 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator < (const String& a, const String& b)
{
	return strcmp(a.AsCharPtr(), b.AsCharPtr()) < 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator > (const String& a, const String& b)
{
	return strcmp(a.AsCharPtr(), b.AsCharPtr()) > 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator <= (const String& a, const String& b)
{
	return strcmp(a.AsCharPtr(), b.AsCharPtr()) <= 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator >= (const String& a, const String& b)
{
	return strcmp(a.AsCharPtr(), b.AsCharPtr()) >= 0;
}

} // namespace Philo
//------------------------------------------------------------------------------

