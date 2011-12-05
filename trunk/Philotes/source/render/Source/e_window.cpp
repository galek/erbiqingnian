//----------------------------------------------------------------------------
// e_window.cpp
//
// - Implementation for render window abstraction
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_common.h"

#include "camera_priv.h"
//


#include "e_window.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

RenderWindow gpRenderWindows[ MAX_RENDER_WINDOWS ];

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------


void RenderWindow::Reset()
{
	m_hWnd				= NULL;
	m_nWindowWidth		= 0;
	m_nWindowHeight		= 0;
	m_dwWindowFlags		= 0;
}


//void RenderWindow::ValidateRegion()
//{
//	ASSERT_RETURN( IsValidInput( m_fXMin ) );
//	ASSERT_RETURN( IsValidInput( m_fYMin ) );
//	ASSERT_RETURN( IsValidInput( m_fXMax ) );
//	ASSERT_RETURN( IsValidInput( m_fYMax ) );
//	ASSERT_RETURN( m_fXMax >= m_fXMin );
//	ASSERT_RETURN( m_fYMax >= m_fYMin );
//}
//
//BOOL RenderWindow::IsValidInput( float fInput )
//{
//	if ( fInput < 0.0f || fInput > 1.0f )
//		return FALSE;
//	return TRUE;
//}
//
//void RenderWindow::ClampInput( float & fInput )
//{
//	fInput = max( 0.0f, fInput );
//	fInput = min( 1.0f, fInput );
//}


void RenderWindow::SetFlag( RenderWindow::FLAGBIT eFlag, BOOL bValue )
{
	SETBIT( m_dwWindowFlags, eFlag, bValue );
}

BOOL RenderWindow::GetFlag( RenderWindow::FLAGBIT eFlag )
{
	return TESTBIT( m_dwWindowFlags, eFlag );
}

