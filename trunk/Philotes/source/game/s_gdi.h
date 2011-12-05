// s_gdi.h

#define gdi_color_black			0
#define gdi_color_white			1

#define gdi_color_red			2
#define gdi_color_green			3
#define gdi_color_blue			4
#define gdi_color_yellow		5
#define gdi_color_cyan			6
#define gdi_color_purple		7
#define gdi_color_gray			8

// custom colors are from here down for any special needs
#define gdi_color_darkgray		9
#define gdi_color_verydarkgray	10
#define gdi_color_darkblue		11

//-------------------------------------------------------------------------------------------------

BOOL gdi_init( HWND window, int width, int height );
BOOL gdi_free();

//-------------------------------------------------------------------------------------------------
// screen related functions

void gdi_clearscreen();
void gdi_blit( HWND window );
void gdi_getscreensize( int * width, int * height );

//-------------------------------------------------------------------------------------------------
// drawing functions

int gdi_drawstring( int x, int y, LPTSTR string, BYTE color = gdi_color_white );
int gdi_pixel_strlen( LPTSTR string );
int gdi_wchar_drawstring( int x, int y, const WCHAR * str, BYTE color = gdi_color_white );

void gdi_drawpixel( int x, int y, BYTE color );
void gdi_drawline( int x1, int y1, int x2, int y2, BYTE color );
void gdi_drawcircle( int x, int y, int radius, BYTE color );
void gdi_drawbox( int x1, int y1, int x2, int y2, BYTE color );
void gdi_drawsolidbox( int x1, int y1, int x2, int y2, BYTE color );
