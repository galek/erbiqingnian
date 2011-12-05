//----------------------------------------------------------------------------
// dxC_input.h
//
// This file provides all DirectInput interface smart pointer types
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __DXC_INPUT__
#define __DXC_INPUT__

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------
#include <dinput.h>
#include <atlcomcli.h>


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define DIRECTINPUT_VERSION 0x0800

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
// DirectInput
typedef CComPtr<IDirectInput8>												SPDIRECTINPUT8;
typedef CComPtr<IDirectInputDevice8>										SPDIRECTINPUTDEVICE8;

#endif //__DXC_INPUT__