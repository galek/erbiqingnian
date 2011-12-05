//----------------------------------------------------------------------------
// FILE: dataversion.h
//----------------------------------------------------------------------------

#ifndef __DATA_VERSION_H_
#define __DATA_VERSION_H_

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

BOOL DataVersionGet(
	struct FILE_VERSION &tDataVersion,
	BOOL bCheckPakFiles = TRUE);

BOOL DataVersionSet(
	struct FILE_VERSION &tDataVersion);

void DataVersionSetOverride(
	struct FILE_VERSION &tDataVersionOverride);

void DataVersionClearOverride(
	void);
		
#endif