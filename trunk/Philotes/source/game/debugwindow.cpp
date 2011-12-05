//-------------------------------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------

#include "stdafx.h"
#include "prime.h"
#include "debugwindow.h"
#include "s_gdi.h"
#include "game.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//
// Debug window
//
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static HWND sghDebugWnd;
static char	sgszAppName[] = "Debug";

static BOOL sgbDebugInited = FALSE;
static BOOL sgbDebugVisible = FALSE;


void ( *sgpfnDraw )( void ) = NULL;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

LRESULT CALLBACK DaveDebugWindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		break;

	case WM_KEYDOWN:
		if ( wParam == VK_ESCAPE )
		{
			DestroyWindow( sghDebugWnd );
		}
		break;

	case WM_DESTROY :
		PostQuitMessage( 0 );
		return TRUE;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void DaveDebugWindowInit()
{
	if ( sgbDebugInited )
		return;

	HINSTANCE hInstance = AppGetHInstance();
	WNDCLASSEX	tWndClass;

	tWndClass.cbSize = sizeof( tWndClass );
	tWndClass.style = CS_HREDRAW | CS_VREDRAW;
	tWndClass.lpfnWndProc = DaveDebugWindowProc;
	tWndClass.cbClsExtra = 0;
	tWndClass.cbWndExtra = 0;
	tWndClass.hInstance = hInstance;
	tWndClass.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	tWndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
	tWndClass.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	tWndClass.lpszMenuName = NULL;
	tWndClass.lpszClassName = sgszAppName;
	tWndClass.hIconSm = LoadIcon( NULL, IDI_APPLICATION );

	RegisterClassEx( &tWndClass );

	int nX = GetSystemMetrics( SM_CXSCREEN ) - DEBUG_WINDOW_WIDTH;
	int nY = 0;

	int nWinWidth = DEBUG_WINDOW_WIDTH + ( GetSystemMetrics( SM_CXFIXEDFRAME ) * 2 );
	int nWinHeight = DEBUG_WINDOW_HEIGHT + ( GetSystemMetrics( SM_CYFIXEDFRAME ) * 2 ) + GetSystemMetrics( SM_CYCAPTION );

    sghDebugWnd = CreateWindowEx( 0, sgszAppName, sgszAppName, WS_POPUP | WS_CAPTION,
		nX, nY, nWinWidth, nWinHeight,
        NULL, NULL, hInstance, NULL );

	ShowWindow(sghDebugWnd, SW_HIDE);
	sgbDebugVisible = FALSE;
	
	UpdateWindow( sghDebugWnd );

	gdi_init( sghDebugWnd, DEBUG_WINDOW_WIDTH, DEBUG_WINDOW_HEIGHT );

	sgbDebugInited = TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void DaveDebugWindowClose()
{
	if (!sgbDebugInited)
		return;

	sgbDebugInited = FALSE;
	gdi_free();
	DestroyWindow(sghDebugWnd);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void DaveDebugWindowSetDrawFn( void ( *pfnDraw )( void ) )
{
	sgpfnDraw = pfnDraw;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void DaveDebugWindowUpdate( GAME *pGame )
{
	if ( GameGetState(pGame) != GAMESTATE_RUNNING )
		return;

	if (sgbDebugInited && sgbDebugVisible) 
	{
		gdi_clearscreen();

		if ( sgpfnDraw )
			sgpfnDraw();

		gdi_blit( sghDebugWnd );
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL DaveDebugWindowIsActive(
	void)
{
	if (!sgbDebugInited)
	{
		return FALSE;
	}
	return sgbDebugVisible;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void DaveDebugWindowShow(
	void)
{
	if (!sgbDebugInited)
	{
		return;
	}
	if (sgbDebugVisible)
	{
		return;
	}
	DaveDebugWindowUpdate(AppGetCltGame());
	ShowWindow(sghDebugWnd, SW_SHOWNORMAL);
	sgbDebugVisible = TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void DaveDebugWindowHide(
	void)
{
	if (!sgbDebugInited)
	{
		return;
	}
	if (!sgbDebugVisible)
	{
		return;
	}
	ShowWindow(sghDebugWnd, SW_HIDE);
	sgbDebugVisible = FALSE;
}

