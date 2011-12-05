//-----------------------------------------------------------------------------
//
// Hardware compatibility database interface.
// Facilitates enforcement of compatibility/performance policies
// based on video hardware and driver.
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef __E_COMPATDB__
#define __E_COMPATDB__

void e_CompatDB_Init(void);
void e_CompatDB_Deinit(void);
void* e_CompatDB_LoadMasterDatabaseFile( DWORD * pdwBytesRead = NULL );

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
#define E_COMPATDB_ENABLE_EDIT 1
#endif

#ifdef E_COMPATDB_ENABLE_EDIT
void e_CompatDB_DoEdit(void);
void e_CompatDB_CheckForUpdates(void);
#else
#define e_CompatDB_DoEdit()				((void)0)
#define e_CompatDB_CheckForUpdates()	((void)0)
#endif

#endif
