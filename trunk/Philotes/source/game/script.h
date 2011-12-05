//----------------------------------------------------------------------------
// script.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void ScriptInitEx(
	void);

void ScriptFreeEx(
	void);

DWORD ScriptGetImportCRC(
	void);

void ScriptRegisterTableNames(
	void);

BOOL ScriptRegisterStatSymbols(
	struct EXCEL_TABLE * table);

BOOL ScriptAddMarker(
	unsigned int marker);

BOOL ScriptWriteToBuffer(
	unsigned int marker,
	WRITE_BUF_DYNAMIC & fbuf);

BOOL ScriptReadFromBuffer(
	BYTE_BUF & fbuf);

BOOL ScriptWriteGlobalVMToPak(
	void);

BOOL ScriptReadGlobalVMFromPak(
	void);

struct VMGAME * VMInitGame(
	struct MEMORYPOOL * pool);

void VMFreeGame(
	struct VMGAME * vm);

void VMInitState(
	struct VMSTATE * vm);

void ByteStreamInit(
	struct MEMORYPOOL * pool,
	struct BYTE_STREAM & str,
	BYTE * buf = NULL,
	int size = 0);

BYTE_STREAM * ByteStreamInit(
	struct MEMORYPOOL * pool);

void ByteStreamFree(
	struct BYTE_STREAM * str);

BYTE * ScriptParseEx(
	struct GAME * game, 
	const char * str,
	BYTE * buf,
	int len,
	int errorlog,
	const char * errorstr);

int ScriptParseEx(
	struct VMGAME * vm,
	struct BYTE_STREAM * stream,
	const char * str,
	int errorlog,
	const char * errorstr);

int VMExecI(
	VMGAME * vm,
	struct BYTE_STREAM * stream);

int VMExecI(
	struct GAME * game,
	struct UNIT * unit,
	struct BYTE_STREAM * stream);

int VMExecI(
	struct GAME * game,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int VMExecI(
	struct GAME * game,
	struct UNIT * unit,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int VMExecI(
	struct GAME * game,
	struct UNIT * unit,
	int nParam1,
	int nParam2,
	int nSkill,
	int nSkillLevel,
	int nState,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int VMExecI(
	struct GAME * game,
	struct UNIT * unit,
	struct STATS * statslist,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int VMExecI(
	struct GAME * game,
	struct UNIT * unit,
	struct STATS * statslist,
	int nParam1,
	int nParam2,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int VMExecI(
	struct GAME * game,
	struct UNIT * unit,
	struct STATS * statslist,
	int nParam1,
	int nParam2,
	int nSkill,
	int nSkillLevel,
	int nState,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int VMExecI(
	struct GAME * game,
	struct UNIT * unit,
	struct UNIT * object,
	struct STATS * statslist,
	int nParam1,
	int nParam2,
	int nSkill,
	int nSkillLevel,
	int nState,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int VMExecI(
	struct GAME * game,
	struct UNIT * subject,
	struct UNIT * object,
	struct STATS * statslist,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int VMExecI(
	struct STATS * statslist,
	BYTE * buf,
	int len,
	const char * errorstr);

int VMExecI(
	struct STATS * statslist,
	int nParam1,
	int nParam2,
	BYTE * buf,
	int len,
	const char * errorstr);

int VMExecIUnitObjectBuffer(
	struct GAME * game,
	struct UNIT * unit,
	struct UNIT * object,
	BYTE * buf,
	int len,
	const char * errorstr = NULL);

int ClientControlUnitVMExecI(
	BYTE * buf,
	int len);

#endif // _SCRIPT_H_