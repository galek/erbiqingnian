//----------------------------------------------------------------------------
// p_windows.h
//
// windows-specific wrappers
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _P_WINDOWS_H_
#define _P_WINDOWS_H_


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// #define VK_LSHIFT         0xA0
// #define VK_RSHIFT         0xA1
// #define VK_LCONTROL       0xA2
// #define VK_RCONTROL       0xA3
//----------------------------------------------------------------------------
inline BOOL IsKeyDown(
	int vKey)
{
	// if (result & 0x0001), then key was pressed since last call to GetAsyncKeyState, 
	// or the key is toggled (VK_CAPITAL)
	return ((GetAsyncKeyState(vKey) & 0x8000) != 0);
}


#endif // _P_WINDOWS_H_