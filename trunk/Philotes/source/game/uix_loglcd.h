//----------------------------------------------------------------------------
// uix_components.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_LOGLCD_H_
#define _UIX_LOGLCD_H_

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
BOOL UILogLCDInit(
	void);

void UILogLCDFree(
	void);

void UILogLCDUpdate(
	void);

#endif // #if !ISVERSION(SERVER_VERSION)

#endif // _UIX_LOGLCD_H_
