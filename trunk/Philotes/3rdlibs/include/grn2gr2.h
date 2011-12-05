#if !defined(GRN2GR2_H)
/* ========================================================================
   $RCSfile: grn2gr2.h,v $
   $Date: 2002/06/26 03:24:01 $
   $Revision: 1.3 $
   $Creator: Casey Muratori $
   (C) Copyright 1999-2006 by RAD Game Tools, All Rights Reserved.
   ======================================================================== */

#if _WIN32
#define GRANNY_DYNEXP(ret) __declspec(dllexport) ret __stdcall
#else
#define GRANNY_DYNEXP(ret) ret
#endif

GRANNY_DYNEXP(void) GRN2GR2ConvertGRNFile(char const *InGRNFile,
                                          char const *OutGR2File,
                                          bool ShowInterface,
                                          char const *LoadSettingsFile);
GRANNY_DYNEXP(void) GRN2GR2DumpFile(char const *GRNFileName);

#define GRN2GR2_H
#endif
