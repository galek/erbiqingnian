//----------------------------------------------------------------------------
// e_window.h
//
// - Header for render window abstraction
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_WINDOW__
#define __E_WINDOW__

#define MAX_RENDER_WINDOWS	8

struct CAMERA_INFO;

/* class RenderWindow
 *		Defines a final output for rendering.  Holds the HWND and knowledge about window size.
 */
class RenderWindow
{
public:

	enum FLAGBIT
	{
		FLAGBIT_ENABLED = 0,
		NUM_RENDER_WINDOW_FLAGBITS
	};


	void Reset();
	void SetFlag( FLAGBIT eFlag, BOOL bValue );
	BOOL GetFlag( FLAGBIT eFlag );


	HWND							m_hWnd;
	int								m_nWindowWidth;
	int								m_nWindowHeight;

private:
	DWORD							m_dwWindowFlags;

	//void ValidateRegion();
	//BOOL IsValidInput( float fInput );
	//void ClampInput( float & fInput );
};

extern RenderWindow gpRenderWindows[];

#endif // __E_WINDOW__