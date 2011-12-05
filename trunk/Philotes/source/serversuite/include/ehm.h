//----------------------------------------------------------------------------
// ehm.h
// 
// Modified     : $DateTime: 2007/01/24 00:35:14 $
// by           : $Author: jlind $
//
// Error Handling Macros
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once
#include <debug.h>

//
// CBR(exp) and CBRA(exp) 
//
// Check BOOL Result, and optionally assert.
//
// Usage:
//          Declare and initialize a BOOL bRet = TRUE;
//          Have a label called "Error" where all the error handling takes
//          place.
//   e.g:
//
//          BOOL myFunction(char *foo) {
//               BOOL bRet = TRUE;
//
//               char *buffer = malloc(10000000000);
//
//               CBR(buffer != NULL);
//
//               CBRA(CreateFile(...));
//
//
//          Error:
//               if(!bRet) {
//                   if(buffer)
//                   free(buffer);
//                }
//              return bRet;
//          }
//
//
// CHR(exp) and CHRA(exp) 
//
// Same as CBR..., except with an HRESULT instead of BOOL.
//
// Usage:
//          Declare and initialize an HRESULT hr = S_OK;
//          Have a label called "Error" where all the error handling takes
//          place.
//
// CSR(exp) and CSRA(exp) 
//
// Same as CBR..., except with an SQLRETURN instead of BOOL.
//
// Usage:
//          Declare and initialize an SQLRESULT sqlRet = SQL_SUCCESS;
//          Have a label called "Error" where all the error handling takes
//          place.
//
#define CBRA(exp)        {\
                            if(!(exp)) {\
                                bRet = FALSE; \
                                if(DoAssert(#exp,"",__FILE__,__LINE__,\
                                             __FUNCSIG__)) {\
                                    DebugBreak();\
                                }\
                                goto Error; \
                            }\
                        }

#define CBR(exp)        {\
                            if(!(exp)) {\
                                bRet = FALSE; \
                                goto Error; \
                            }\
                        }

#define CHRA(exp)        {\
                            hr = (exp); \
                            if(FAILED(hr)) {\
                                if(DoAssert(#exp,"",__FILE__,__LINE__,\
                                             __FUNCSIG__)) {\
                                    DebugBreak();\
                                }\
                                goto Error; \
                            }\
                        }

#define CHR(exp)        {\
                            hr = (exp); \
                            if(FAILED(hr)) {\
                                goto Error; \
                            }\
                        }
#define CSRA(exp)        {\
                            sqlRet = (exp); \
                            if(!SQL_OK(sqlRet)) {\
                                if(DoAssert(#exp,"",__FILE__,__LINE__,\
                                             __FUNCSIG__)) {\
                                    DebugBreak();\
                                }\
                                goto Error; \
                            }\
                        }

#define CSR(exp)        {\
                            sqlRet = (exp); \
                            if(!SQL_OK(sqlRet)) {\
                                goto Error; \
                            }\
                        }


#ifndef _DEBUG
#undef CBRA
#undef CHRA
#undef CSRA
#define CBRA CBR
#define CHRA CHR
#define CSRA CSR
#endif
