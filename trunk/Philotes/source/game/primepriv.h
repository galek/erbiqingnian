//----------------------------------------------------------------------------
// primepriv.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PRIMEPRIV_H_
#define _PRIMEPRIV_H_


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL PrimeInit(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd);

void PrimeClose(
	void);

void PrimeRun(
	void);

BOOL PrimeTimer(
	GAME * game);

enum PRIME_RUNNING
{
	PRIME_RUNNING_FIRST,
	PRIME_RUNNING_THIS_SESSION,
	PRIME_RUNNING_OTHER_SESSION,
};

PRIME_RUNNING PrimeIsRunning(
	void);

void PrimeBringForwardExistingInstance(
	void);

BOOL PrimeIsRemote(
	void);

BOOL PrimeVerifyAccess(
	const TCHAR* gdfexe );

BOOL PrimePreInitForGlobalStrings(
	HINSTANCE hInstance,
	LPSTR lpCmdLine);

BOOL PrimeDidPreInitForGlobalStrings(
	void);

#endif // _PRIMEPRIV_H_
