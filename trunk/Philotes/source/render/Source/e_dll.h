// e_dll.h - Engine DLL header
//
// Copyright (C) 2007 by Flagship Studios, Inc.

#ifndef __E_DLL_H__
#define __E_DLL_H__

#define ENGINE_EXPORT(rettype)		extern "C" __declspec(dllexport) rettype __stdcall

#endif // __E_DLL_H__