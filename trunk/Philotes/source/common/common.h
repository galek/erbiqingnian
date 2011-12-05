//----------------------------------------------------------------------------
// common.h
//
//----------------------------------------------------------------------------
#ifdef	_COMMON_H_
#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _COMMON_H_


//----------------------------------------------------------------------------
// VERSION DEFINES
//----------------------------------------------------------------------------
#define DEBUG_VERSION					0x00000001
#define RETAIL_VERSION					0x00000002
#define OPTIMIZED_VERSION				0x00000010			// optimized
#define DEBUG_ID						0x00000020			// use debug ids
#define DEVELOPMENT						0x00000100			// dev version
#define CHEATS_ENABLED					0x00000200			// development cheats enabled
#define RELEASE_CHEATS_ENABLED			0x00000400			// release cheats enabled
#define DEMO_VERSION					0x00001000			// demo w/ some features turned off
#define CLIENT_ONLY_VERSION				0x00010000			// tugboat client
#define SERVER_VERSION					0x00020000			// server
#define US_SERVER_VERSION				0x00100000			// large-realm specific server
#define ASIA_SERVER_VERSION				0x00200000			// small-realm server
#define RELEASE_VERSION					0x00400000			// release version

#if		defined(_SERVER)
#define SERVER_FLAGS					SERVER_VERSION 
#else
#define SERVER_FLAGS					0
#endif //SERVER

#define VERSION_FLAGS					SERVER_FLAGS

#if		defined(_DEBUG_OPT)
#	define	CURRENT_VERSION					(DEBUG_VERSION | DEVELOPMENT | OPTIMIZED_VERSION | CHEATS_ENABLED | VERSION_FLAGS)
//#	define	CURRENT_VERSION					(DEBUG_VERSION | DEVELOPMENT | OPTIMIZED_VERSION | DEMO_VERSION | CHEATS_ENABLED | VERSION_FLAGS)
#elif	defined(_DEBUG)
//#	define	CURRENT_VERSION					(DEBUG_VERSION | DEVELOPMENT | DEMO_VERSION | CHEATS_ENABLED | VERSION_FLAGS)
#	define	CURRENT_VERSION					(DEBUG_VERSION | DEVELOPMENT | CHEATS_ENABLED | VERSION_FLAGS | RELEASE_CHEATS_ENABLED)
//#	define	CURRENT_VERSION					(DEBUG_VERSION | DEVELOPMENT | DEMO_VERSION | CHEATS_ENABLED | VERSION_FLAGS)
#elif defined(TUGBOAT) && !defined(_SERVER) // retail tugboat client build
#	define	CURRENT_VERSION					(RETAIL_VERSION | OPTIMIZED_VERSION | CLIENT_ONLY_VERSION | VERSION_FLAGS)
#elif defined(HELLGATE) && defined(_CLIENT_ONLY) && !defined(_SERVER)
#	define	CURRENT_VERSION					(RETAIL_VERSION | OPTIMIZED_VERSION | CLIENT_ONLY_VERSION | VERSION_FLAGS)
#elif defined(HELLGATE) && defined(_DEMO) && !defined(_SERVER)
#	define	CURRENT_VERSION					(RETAIL_VERSION | OPTIMIZED_VERSION | DEMO_VERSION | VERSION_FLAGS)
#elif !defined(_SERVER)
#	define	CURRENT_VERSION					(RETAIL_VERSION | OPTIMIZED_VERSION | VERSION_FLAGS | RELEASE_VERSION)
#else	// server
#	define	CURRENT_VERSION					(OPTIMIZED_VERSION | VERSION_FLAGS | RELEASE_CHEATS_ENABLED)			// note server isn't RETAIL
#endif

#define ISVERSION(x)					((CURRENT_VERSION & (x)) == (x))


#define GLOBAL_FILE_VERSION				1



//----------------------------------------------------------------------------
// VERSIONING MACROS
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
#define SVR_VERSION_ONLY(x)				x
#define CLT_VERSION_ONLY(x)
#define IF_NOT_SERVER_ONLY(x)			
#define AUTO_VERSION(cltCode, svrCode)	svrCode
#else
#define SVR_VERSION_ONLY(x)
#define CLT_VERSION_ONLY(x)				x
#define IF_NOT_SERVER_ONLY(x)			x
#define AUTO_VERSION(cltCode, svrCode)	cltCode
#endif

#if ISVERSION(CLIENT_ONLY_VERSION)
#define IF_NOT_CLIENT_ONLY(x)
#else
#define IF_NOT_CLIENT_ONLY(x)			x
#endif

#if ISVERSION(DEBUG_VERSION)
#define DBG_VERSION_ONLY(x)				x
#else
#define DBG_VERSION_ONLY(x)				
#endif


//----------------------------------------------------------------------------
// USER MACROS
//----------------------------------------------------------------------------
#define USER_CODE(u, cmd)		USER_##u(cmd)

#if defined(USERNAME_AEbrahimi) && ISVERSION(DEBUG_VERSION)
	#define USER_AE(cmd)		cmd
#else
	#define USER_AE(cmd)
#endif


//----------------------------------------------------------------------------
// WARNING SUPPRESSION
//----------------------------------------------------------------------------
#pragma warning(disable : 4201)			// disable error for specifying structures without a declarator as members of another structure or union


//----------------------------------------------------------------------------
// GLOBAL VALUES
//----------------------------------------------------------------------------
enum			
{
	DEFAULT_INDEX_SIZE =				64,
	DEFAULT_XML_FRAME_NAME_SIZE =		64,
	DEFAULT_FILE_SIZE =					128,
	DEFAULT_DESC_SIZE =					32,

	DEFAULT_FILE_WITH_PATH_SIZE =		256,
};

#define MAX_XML_STRING_LENGTH			256
#define MAX_FILE_NAME_LENGTH			256


//----------------------------------------------------------------------------
// MACROS - misc
//----------------------------------------------------------------------------
#define STR_ARG(s)						(s ? s : "")
#define WSTR_ARG(s)						(s ? s : L"")
#define REF(v)							(v)


//----------------------------------------------------------------------------
// MACROS - logic
//----------------------------------------------------------------------------
#define BOTH(a, b)						(((a) && (b)) || (!(a) && !(b)))


//----------------------------------------------------------------------------
// MACROS - mathish
//----------------------------------------------------------------------------
#define UINT_ROTATE(cur, max, dir)	((unsigned int)((int)(cur) + (int)(max) + (dir)) % (max))


//----------------------------------------------------------------------------
// MACROS - Hash
//----------------------------------------------------------------------------
#define STR_HASH_ADDC(key, c)			((key) * 37 + (c))
#define IS_VALID_INDEX(index, nmax)		(index >= 0 && index < nmax)


//----------------------------------------------------------------------------
// MACROS - Array/Structure macros
//----------------------------------------------------------------------------
#define arrsize(x)						(unsigned int)(sizeof(x)/sizeof(x[0]))
#define OFFSET(s,m)						(unsigned int)(size_t)&(((s *)0)->m)
#define structclear(s)					memclear(&s, sizeof(s))
#define ALIGN(x)						__declspec(align(x))
#define CAST_TO_VOIDPTR(i)				((void *)((BYTE *)0 + (i)))
#define CAST_PTR_TO_INT(p)				(int)((char *)p - (char *)0)
#define CAST_PTR64_TO_INT(p)			(int)((char * POINTER_64)(p) - (char * POINTER_64)0)
#define DOWNCAST_INT_TO_BYTE(i)			(BYTE)((i) & UCHAR_MAX)
#define DOWNCAST_INT_TO_WORD(i)			(WORD)((i) & USHRT_MAX)
#define SIZE_TO_INT(s)					(int)(s)
#define SIZE_TO_UINT(s)					(unsigned int)(s)


//----------------------------------------------------------------------------
// MACROS - "Unique" identifier
//----------------------------------------------------------------------------
#define CONCAT_3_(x, y)					x##y
#define CONCAT_2_(x, y)					CONCAT_3_(x, y)
#define CONCAT(x, y)					CONCAT_2_(x, y)
#define UIDEN(x)						CONCAT(x, __LINE__)


//----------------------------------------------------------------------------
// MACROS - flow of control
//----------------------------------------------------------------------------
#define ONCE										for (int UIDEN(_once) = 0; UIDEN(_once)++ == 0;)
#define TWICE										for (int UIDEN(_twice) = 2; UIDEN(_twice)-- > 0;)
#if ISVERSION(DEVELOPMENT)
#define BOUNDED_WHILE(condition, max_iterations)	for (unsigned int UIDEN(_iden) = max_iterations; (condition) && (UIDEN(_iden) > 0 || ErrorDoAssert( "Max loop iterations exceeded:\n\nwhile (%s)\nMax iterations: %d\n\nFile: %s\nLine: %d", #condition, max_iterations, __FILE__, __LINE__ ) ); --UIDEN(_iden))
#else
#define BOUNDED_WHILE(condition, max_iterations)	for (unsigned int UIDEN(_iden) = max_iterations; (condition) && (UIDEN(_iden) > 0); --UIDEN(_iden))
#endif

// loop through all 0..count, but starting from start
#define LOOP_FROM_START(iden, count, start)			for (unsigned int iden = start; iden != NORMALIZE(start + count - 1, count); iden = NORMALIZE(iden + 1, count))


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void NULLFUNC(
	...)
{
	return;
}


//----------------------------------------------------------------------------
// MACROS - Four CC
//----------------------------------------------------------------------------
#define STRTOFOURCC(s)					(DWORD)((s)[0] == 0 ? 0 : \
										 ((s)[1] == 0 ? (s)[0] : \
										  ((s)[2] == 0 ? ((s)[0] | ((s)[1] << 8)) : \
										   ((s)[3] == 0 ? ((s)[0] | ((s)[1] << 8) | ((s)[2] << 16)) : \
										    ((s)[0] | ((s)[1] << 8) | ((s)[2] << 16) | ((s)[3] << 24))))))
#define FOURCCTOSTR(dw, s)				((s)[0] = (char)(dw & 0xff), \
										 (s)[1] = (char)((dw >> 8) & 0xff), \
										  (s)[2] = (char)((dw >> 16) & 0xff), \
										   (s)[3] = (char)(dw >> 24), \
										    (s)[4] = 0, s)

#define STRTOTHREECC(s)					(DWORD)((s)[0] == 0 ? 0 : \
										 ((s)[1] == 0 ? (s)[0] : \
										  ((s)[2] == 0 ? ((s)[0] | ((s)[1] << 8)) : \
										    ((s)[0] | ((s)[1] << 8) | ((s)[2] << 16)))))

#define THREECCTOSTR(dw, s)				((s)[0] = (char)(dw & 0xff), \
										 (s)[1] = (char)((dw >> 8) & 0xff), \
										  (s)[2] = (char)((dw >> 16) & 0xff), \
										   (s)[3] = 0, s)

#define STRTOTWOCC(s)					(DWORD)((s)[0] == 0 ? 0 : \
										 ((s)[1] == 0 ? (s)[0] : \
										  ((s)[0] | ((s)[1] << 8))))

#define TWOCCTOSTR(w, s)				((s)[0] = (char)(w & 0xff), \
										 (s)[1] = (char)((w >> 8) & 0xff), \
										  (s)[2] = 0, s)

#define STRTOONECC(s)					(DWORD)((s)[0])

#define ONECCTOSTR(b, s)				((s)[0] = (char)(b & 0xff), \
										 (s)[1] = 0, s)

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

#ifndef MAKEFOURCCSTR
#define MAKEFOURCCSTR(str)				MAKEFOURCC(str[0],str[1],str[2],str[3])
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// CHB 2006.08.18 - Compiler hint that a pointer is not aliased.
#if _MSC_VER >= 1400
#define NOALIAS __restrict
#else
#define NOALIAS
#endif


//----------------------------------------------------------------------------
// These are all remedial math functions that everything in the world probably ought to
// have access to.  It won't hurt to put this here.
//----------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<typename T> 
inline T MAX(
	T a, 
	T b)
{ 
	return (a > b) ? a : b; 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<>
inline int MAX<int>(
	int a,
	int b)
{
	b = a - b;
	return a - (b & (b >> 31));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int MAX0(
	int a)
{
	return a & ~(a >> 31);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<typename T> 
inline T MAX(
	T a, 
	T b,
	T c)
{ 
	if (a > b)
	{
		return (a > c) ? a : c;
	}
	else
	{
		return (b > c) ? b : c;
	}
}


// ---------------------------------------------------------------------------
// return index 0-2 for which item was largest
// ---------------------------------------------------------------------------
template<typename T> 
inline T MAXIDX(
	T a, 
	T b,
	T c)
{ 
	if (a >= b)
	{
		return (a >= c) ? 0 : 2;
	}
	else
	{
		return (b >= c) ? 1 : 2;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<typename T> 
inline T MIN(
	T a,
	T b)
{	
	return (a < b) ? a : b; 
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template <>
inline int MIN<int>(
	int a,
	int b)
{
	a = a - b;
	return b + (a & (a >> 31));
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<typename T> 
inline T MIN(
	T a, 
	T b,
	T c)
{ 
	if (a < b)
	{
		return (a < c) ? a : c;
	}
	else
	{
		return (b < c) ? b : c;
	}
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<typename T> 
inline T MINIDX(
	T a, 
	T b,
	T c)
{ 
	if (a <= b)
	{
		return (a <= c) ? 0 : 2;
	}
	else
	{
		return (b <= c) ? 1 : 2;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<typename T> 
inline T ABS(
	T a)
{ 
	return (a >= (T)0) ? a : -a; 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <>
inline int ABS<int>(
	int a)
{
	return (a ^ (a >> 31)) + ((unsigned)a >> 31);
}


//----------------------------------------------------------------------------
// average even if a + b overflows
//----------------------------------------------------------------------------
inline DWORD AVERAGE(
	DWORD a,
	DWORD b)
{
	return (a & b) + ((a ^ b) >> 1);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T> 
inline T PIN(
	T n,
	T min,
	T max)	
{
	if (n <= min) 
	{ 
		return min; 
	} 
	else if (n >= max) 
	{ 
		return max; 
	} 
	else 
	{ 
		return n; 
	} 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<>
inline int PIN<int>(
	int n,
	int min,
	int max)
{
	return MAX(MIN(n, max), min);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<typename T>
inline void SWAP(
	T & a,
	T & b)
{
	T tmp = a;
	a = b;
	b = tmp;
}


// define these macros here so that we can use perf counter macros in various
// places with impunity
#if !ISVERSION(SERVER_VERSION)
#define PERF_GET_INSTANCE(o,n)  NULL
#define PERF_FREE_INSTANCE(p)   
#define PERF_INSTANCE(type)     void
#define PERF_ADD64(p,o,c,v)		{ p = p; }
#define MAX_PERF_INSTANCE_NAME 32
#endif


#endif // _COMMON_H_
