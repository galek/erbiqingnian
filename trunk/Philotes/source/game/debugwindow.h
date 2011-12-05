//----------------------------------------------------------------------------
// debugwindow.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _DEBUGWINDOW_H_
#define _DEBUGWINDOW_H_

//----------------------------------------------------------------------------
// have defined to use debug window, undefined removes the code

#define DEBUG_WINDOW			1

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define DEBUG_WINDOW_WIDTH		320
#define DEBUG_WINDOW_HEIGHT		200

#define DEBUG_WINDOW_X_CENTER	( DEBUG_WINDOW_WIDTH / 2 )
#define DEBUG_WINDOW_Y_CENTER	( DEBUG_WINDOW_HEIGHT / 2 )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void DaveDebugWindowInit();
void DaveDebugWindowClose();
void DaveDebugWindowSetDrawFn( void ( *pfnDraw )( void ) );
void DaveDebugWindowUpdate( struct GAME *pGame );

BOOL DaveDebugWindowIsActive(
	void);

void DaveDebugWindowShow(
	void);

void DaveDebugWindowHide(
	void);


#endif
