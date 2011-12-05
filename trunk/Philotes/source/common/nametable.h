//----------------------------------------------------------------------------
// nametable.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _NAMETABLE_H_
#define _NAMETABLE_H_


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	NAMETYPE_UI,
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
struct NAMETABLE* NameTableInit(
	BOOL bFreeData = FALSE);

void NameTableFree(
	struct NAMETABLE* nametable);

void* NameTableLookup(
	struct NAMETABLE* nametable,
	const char* name,
	int* type = NULL);

BOOL NameTableRegister(	
	struct NAMETABLE* nametable,
	const char* name,
	int type,
	void* data);

void NameTableMakeUniqueName(
	struct NAMETABLE* nametable,
	char* name,
	int maxlen);


#endif // _NAMETABLE_H_
