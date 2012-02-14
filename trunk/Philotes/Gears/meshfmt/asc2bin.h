

#ifndef ASC2BIN_H
#define ASC2BIN_H

_NAMESPACE_BEGIN

// types:
//
//          f   : 4 byte scalar
//          d   : 4 byte integer
//          c   : 1 byte character
//          b   : 1 byte integer
//          h   : 2 byte integer
//          p   : 4 byte const char *
//          x1  : 1 byte hex
//          x2  : 2 byte hex
//          x4  : 4 byte hex (etc)
// example usage:
//
//    Asc2Bin("1 2 3 4 5 6",1,"fffff",0);

void * Asc2Bin(const char *source,const int32 count,const char *ctype,void *dest=0);

_NAMESPACE_END


#endif // ASC2BIN_H
