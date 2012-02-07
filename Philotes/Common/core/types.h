#pragma once

// fixing Windows defines...
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef CopyFile
#undef CopyFile
#endif
#ifdef GetObject
#undef GetObject
#endif

//Ogreœ∞πﬂ
typedef unsigned long		ulong;
typedef unsigned int		uint;
typedef unsigned short		ushort;
typedef unsigned char		uchar;
typedef unsigned char		ubyte;

//Ogreœ∞πﬂ
typedef unsigned __int64	uint64;
typedef unsigned int		uint32;
typedef unsigned short		uint16;
typedef unsigned char		uint8;
typedef __int64				int64;
typedef int					int32;
typedef short				int16;
typedef char				int8;

typedef int					IndexT;     // the index type
typedef int					SizeT;      // the size type

static const int InvalidIndex = -1;
