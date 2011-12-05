//----------------------------------------------------------------------------
// c_chatIgnore.h
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//---------------------------------------------------------------------------
#ifndef _C_CHATIGNORE_H_
#define _C_CHATIGNORE_H_


//----------------------------------------------------------------------------
//	CLIENT IGNORE LIST METHODS
//----------------------------------------------------------------------------

void c_IgnoreListInit(
	void );


//----------------------------------------------------------------------------
//	CLIENT IGNORE LIST MESSAGE HANDLERS
//----------------------------------------------------------------------------

void sChatCmdPlayerIgnored(
	GAME* game,
	__notnull BYTE* data);

void sChatCmdPlayerUnIgnored(
	GAME* game,
	__notnull BYTE* data);

void sChatCmdIgnoreError(
	GAME* game,
	__notnull BYTE* data);


#endif	//	_C_CHATIGNORE_H_
