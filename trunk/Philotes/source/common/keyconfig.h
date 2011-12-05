//----------------------------------------------------------------------------
// keyconfig.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _KEYCONFIG_H_
#define _KEYCONFIG_H_


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
void KeyConfigInit(
	void);

void KeyConfigFree(
	void);

int KeyConfigGetKeyCodeFromString(
	char* str);

const WCHAR *KeyConfigGetNameFromKeyCode(
	int keycode);

#endif // _KEYCONFIG_H_