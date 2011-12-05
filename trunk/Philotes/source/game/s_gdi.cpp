//-------------------------------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------
// Graphic core file header
//-------------------------------------------------------------------------------------------------

#include "stdafx.h"
#include "colors.h"

//-------------------------------------------------------------------------------------------------

// used for gdi creation
typedef struct
{
	BITMAPINFOHEADER	header;
	RGBQUAD				colors[256];
} mybitmapinfo;

// window and app level vars
static int window_width;
static int window_height;
static BYTE * screen_buffer = NULL;
static HBITMAP bitmap;
static HFONT font;
static int fontsizebuffer[128];

static DWORD palette[256];

static void ( * draw_function )( int x, int y, BYTE color );

//---------------------------------------------------------------------------------
// clear the offscreen buffer

void gdi_clearscreen()
{
	ZeroMemory( screen_buffer, window_width * window_height );
}

//---------------------------------------------------------------------------------
// draw a pixel at x,y with no clipping

static void drawpixel_noclip( int x, int y, BYTE color )
{
	*( screen_buffer + ( y * window_width ) + x ) = color;
}

//---------------------------------------------------------------------------------
// draw a pixel at x,y

void gdi_drawpixel( int x, int y, BYTE color )
{
	if ( ( x < 0 ) || ( x >= window_width ) || ( y < 0 ) || ( y >= window_height ) )
		return;
	*( screen_buffer + ( y * window_width ) + x ) = color;
}

//---------------------------------------------------------------------------------
// draw line

static void set_draw_function( int x1, int y1, int x2, int y2 )
{
	if ( ( x1 < 0 ) || ( x1 >= window_width ) ||
		( x2 < 0 ) || ( x2 >= window_width ) ||
		( y1 < 0 ) || ( y1 >= window_height ) ||
		( y2 < 0 ) || ( y2 >= window_height ) )
	{
		draw_function = gdi_drawpixel;
	}
	else
	{
		draw_function = drawpixel_noclip;
	}
}

//---------------------------------------------------------------------------------
// draw line

void gdi_drawline( int x1, int y1, int x2, int y2, BYTE color )
{
	set_draw_function( x1, y1, x2, y2 );

	int deltax = abs( x2 - x1 );
	int deltay = abs( y2 - y1 );

	int xadd, yadd;
	if ( x1 > x2 )
		xadd = -1;
	else
		xadd = 1;
	if ( y1 > y2 )
		yadd = -1;
	else
		yadd = 1;

	int dpr, dpru, p;
	if ( deltax >= deltay )
	{
		dpr = deltay << 1;
		dpru = dpr - ( deltax << 1 );
		p = dpr - deltax;
		while ( deltax >= 0 )
		{
			draw_function( x1, y1, color );
			if ( p > 0 )
			{
				x1 += xadd;
				y1 += yadd;
				p += dpru;
			}
			else
			{
				x1 += xadd;
				p += dpr;
			}
			deltax--;
		}		
	}
	else
	{
		dpr = deltax << 1;
		dpru = dpr - ( deltay << 1 );
		p = dpr - deltay;
		while ( deltay >= 0 )
		{
			draw_function( x1, y1, color );
			if ( p > 0 )
			{
				x1 += xadd;
				y1 += yadd;
				p += dpru;
			}
			else
			{
				y1 += yadd;
				p += dpr;
			}
			deltay--;
		}		
	}		
}

//---------------------------------------------------------------------------------
// draw a box

void gdi_drawbox( int x1, int y1, int x2, int y2, BYTE color )
{
	if ( ( x1 < 0 ) || ( x1 >= window_width ) ||
		 ( x2 < 0 ) || ( x2 >= window_width ) ||
		 ( y1 < 0 ) || ( y1 >= window_height ) ||
		 ( y2 < 0 ) || ( y2 >= window_height ) )
	{
		gdi_drawline( x1, y1, x2, y1, color );
		gdi_drawline( x1, y2, x2, y2, color );
		gdi_drawline( x1, y1, x1, y2, color );
		gdi_drawline( x2, y1, x2, y2, color );
	}
	else
	{
		BYTE * dest = screen_buffer + ( y1 * window_width );
		for ( int y = y1; y <= y2; y++ )
		{
			dest[x1] = color;
			dest[x2] = color;
			dest += window_width;
		}
		BYTE * dest_y1 = screen_buffer + ( y1 * window_width );
		BYTE * dest_y2 = screen_buffer + ( y2 * window_width );
		for ( int x = x1; x <= x2; x++ )
		{
			dest_y1[x] = color;
			dest_y2[x] = color;
		}
	}
}

//---------------------------------------------------------------------------------
// draw a filled in box

void gdi_drawsolidbox( int x1, int y1, int x2, int y2, BYTE color )
{
	if ( ( x1 < 0 ) || ( x1 >= window_width ) ||
		( x2 < 0 ) || ( x2 >= window_width ) ||
		( y1 < 0 ) || ( y1 >= window_height ) ||
		( y2 < 0 ) || ( y2 >= window_height ) )
	{
		for ( int j = y1; j <= y2; j++ )
		{
			for ( int i = x1; i <= x2; i++ )
				draw_function( i, j, color );
		}
	}
	else
	{
		BYTE * dest = screen_buffer + ( y1 * window_width ) + x1;
		DWORD delta = window_width - ( x2 - x1 + 1 );
		for ( int j = y1; j <= y2; j++ )
		{
			for ( int i = x1; i <= x2; i++ )
				*dest++ = color;
			dest += delta;
		}
	}
}

//---------------------------------------------------------------------------------

static void drawcirclepoints( int xc, int x, int yc, int y, BYTE color )
{
	draw_function( xc + x, yc + y, color );
	draw_function( xc - x, yc + y, color );
	draw_function( xc + x, yc - y, color );
	draw_function( xc - x, yc - y, color );

	draw_function( xc + y, yc + x, color );
	draw_function( xc - y, yc + x, color );
	draw_function( xc + y, yc - x, color );
	draw_function( xc - y, yc - x, color );
}

//---------------------------------------------------------------------------------

void gdi_drawcircle( int x, int y, int radius, BYTE color )
{
	set_draw_function( x - radius, y - radius, x + radius, y + radius );

	int xx = 0;
	int yy = radius;
	int p = 3 - 2 * radius;
	while ( xx < yy ) {
		drawcirclepoints( x, xx, y, yy, color );
		if ( p < 0 )
		{
			p += 4 * xx + 6;
		}
		else
		{
			p += 4 * ( xx - yy ) + 10;
			yy--;
		}
		xx++;
	}
	if ( xx == yy )
		drawcirclepoints( x, xx, y, yy, color );
}

//---------------------------------------------------------------------------------

static void init_font( HWND window )
{
	// Load a font to work with
	font = CreateFont(  10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH, "Courier" );

	// Get the DC
	HDC hdcmain = GetDC( window );
	HDC hdc = CreateCompatibleDC( hdcmain );

	SelectObject( hdc, bitmap );
	SelectObject( hdc, font );

	ZeroMemory( &fontsizebuffer[0], 128 * sizeof( int ) );
	if ( !GetCharWidth32( hdc, 32, 127, &fontsizebuffer[32] ) )
	{
		if ( !GetCharWidth( hdc, 32, 127, &fontsizebuffer[32] ) )
		{
			ABC truetypeinfo[ 128 ];
			int ret = GetCharABCWidths( hdc, 32, 127, &truetypeinfo[32] );
			ASSERT( ret );
			for ( int i = 32; i < 128; i++ )
				fontsizebuffer[i] = truetypeinfo[i].abcA + truetypeinfo[i].abcB + truetypeinfo[i].abcC;
		}
	}

	// Release the DC
	DeleteDC( hdc );
	ReleaseDC( window, hdcmain );
}

//---------------------------------------------------------------------------------
// return the pixel length of a string using the current font

int gdi_pixel_strlen( LPTSTR string )
{
	int length = (int)strlen( string );
	int total = 0;
	for ( int i = 0; i < length ; i++ )
		total += fontsizebuffer[ string[i] ];

	return total;
}

//---------------------------------------------------------------------------------

int gdi_drawstring( int x, int y, LPTSTR string, BYTE color )
{
	// Get the DC
	HDC hdcmain = GetDC( NULL );
	HDC hdc = CreateCompatibleDC( hdcmain );

	SelectObject( hdc, bitmap );
	SelectObject( hdc, font );
	DWORD outcolor = ( palette[ color ] & 0xff0000 ) >> 16;
	outcolor += palette[ color ] & 0x00ff00;
	outcolor += ( palette[ color ] & 0x0000ff ) << 16;
	SetTextColor( hdc, outcolor );
	SetBkColor( hdc, 0 );
	SetBkMode( hdc, TRANSPARENT );

	RECT rect;
	SetRect( &rect, x, y, window_width - x, window_height - y );
	DrawText( hdc, string, -1, &rect, DT_LEFT | DT_NOCLIP );

	// Release the DC
	DeleteDC( hdc );
	ReleaseDC( NULL, hdcmain );

	return gdi_pixel_strlen( string );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int gdi_wchar_drawstring( int x, int y, const WCHAR * str, BYTE color )
{
	char buf[4096];
	PStrCvt( buf, str, 4096 );
	return gdi_drawstring( x, y, buf, color );
}

//---------------------------------------------------------------------------------
// gdi blit

void gdi_blit( HWND window )
{
	HDC dest = GetDC( window );
	HDC source = CreateCompatibleDC( dest );

	SelectObject( source, bitmap );

	BitBlt( dest, 0, 0, window_width, window_height, source, 0, 0, SRCCOPY );
	GdiFlush();

	DeleteDC( source );
	ReleaseDC( window, dest );
}

//---------------------------------------------------------------------------------

BOOL gdi_init( HWND window, int width, int height )
{
	// set clipping
	window_width = width;
	window_height = height;

	// Create Device independant bitmap (DIB)
	mybitmapinfo info;
	ZeroMemory( &info, sizeof( mybitmapinfo ) );

	// Set header
	BITMAPINFOHEADER * header = &info.header;
	header->biSize = sizeof( BITMAPINFOHEADER );
	header->biWidth = window_width;
	header->biHeight = -window_height;
	header->biPlanes = 1;
	header->biBitCount = 8;
	header->biCompression = BI_RGB;

	ZeroMemory( palette, sizeof( palette ) );
	palette[0] = GFXCOLOR_BLACK;
	palette[1] = GFXCOLOR_WHITE;
	palette[2] = GFXCOLOR_RED;
	palette[3] = GFXCOLOR_GREEN;
	palette[4] = GFXCOLOR_BLUE;
	palette[5] = GFXCOLOR_YELLOW;
	palette[6] = GFXCOLOR_CYAN;
	palette[7] = GFXCOLOR_PURPLE;
	palette[8] = GFXCOLOR_GRAY;
	palette[9] = GFXCOLOR_DKGRAY;
	palette[10] = GFXCOLOR_VDKGRAY;
	palette[11] = GFXCOLOR_DKBLUE;

	for ( int c = 0; c < 256; c++ )
	{
		info.colors[c].rgbRed = ( BYTE )( ( palette[c] & 0xff0000 ) >> 16 );
		info.colors[c].rgbGreen = ( BYTE )( ( palette[c] & 0x00ff00 ) >> 8 );
		info.colors[c].rgbBlue = ( BYTE )( palette[c] & 0x0000ff );
	}

	// Create DIB
	HDC dc = GetDC( window );
	bitmap = CreateDIBSection( dc, (tagBITMAPINFO *)&info, DIB_RGB_COLORS, (void **)&screen_buffer, NULL, 0 );
	ReleaseDC( window, dc );

	gdi_clearscreen();
	init_font( window );

	return TRUE;
}

//---------------------------------------------------------------------------------

BOOL gdi_free()
{
	DeleteObject( font );
	DeleteObject( bitmap );
	screen_buffer = NULL;
	
	return TRUE;
}

//---------------------------------------------------------------------------------

void gdi_getscreensize( int * width, int * height )
{
	if ( width ) *width = window_width;
	if ( height ) *height = window_height;
}