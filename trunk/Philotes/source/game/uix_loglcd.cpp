//----------------------------------------------------------------------------
// uix_loglcd.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "uix_loglcd.h"
#include "prime.h"
#include "game.h"
#include "stats.h"
#include "skills.h"

#if !ISVERSION(SERVER_VERSION)

// include the Logitech LCD SDK header
#include "..\..\3rd Party\Logitech G15 SDK\include\lglcd.h"

#include "uix_loglcdbmp.inl"

#define PIXEL_ON	128
#define PIXEL_OFF	0
#define BITMAP_FILEMARKER  ((WORD) ('M' << 8) | 'B')    // is always "BM" = 0x4D42


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct UILCDGlobals
{
	BOOL				m_bLCDAvail;
	lgLcdConnectContext	m_tConnectCtxt;
    lgLcdDeviceDesc		m_tDeviceDesc;
	lgLcdOpenContext	m_tOpenCtxt;
    lgLcdBitmap160x43x1	m_tBitmap;
    lgLcdBitmap160x43x1	m_tBkgdBitmap;
};

UILCDGlobals g_LCD;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL LoadBitmap(
	const char *szFilename,
	lgLcdBitmap160x43x1 &tLCDBitmap)
{
    CFile file;
    if (!file.Open(szFilename, CFile::modeRead))
        return FALSE;

    // Get the current file position.  
    DWORD dwFileStart = (DWORD)file.GetPosition();

    // The first part of the file contains the file header.
	// This will tell us if it is a bitmap, how big the header is, and how big 
    // the file is. The header size in the file header includes the color table.
	BITMAPFILEHEADER BmpFileHdr;
    int nBytes;
    nBytes = file.Read(&BmpFileHdr, sizeof(BmpFileHdr));
    if (nBytes != sizeof(BmpFileHdr)) 
    {
        TRACE0("Failed to read file header\n");
        return FALSE;
    }

    // Check that we have the magic 'BM' at the start.
    if (BmpFileHdr.bfType != BITMAP_FILEMARKER)
    {
        TRACE0("Not a bitmap file\n");
        return FALSE;
    }

    // Read the header (assuming it's a DIB). 
	BITMAPINFO	BmpInfo;
    nBytes = file.Read(&BmpInfo, sizeof(BITMAPINFOHEADER)); 
    if (nBytes != sizeof(BITMAPINFOHEADER)) 
    {
        TRACE0("Failed to read BITMAPINFOHEADER\n");
        return FALSE;
    }

    // Check that we have a real Windows DIB file.
    if (BmpInfo.bmiHeader.biSize != sizeof(BITMAPINFOHEADER))
    {
        TRACE0(" File is not a Windows DIB\n");
        return FALSE;
    }

	// So how big the bitmap surface is.
    int nBitsSize = BmpFileHdr.bfSize - BmpFileHdr.bfOffBits;

    // Allocate the memory for the bits and read the bits from the file.
    BYTE* pBits = (BYTE*) MALLOC(NULL, nBitsSize);
    if (!pBits) 
    {
        TRACE0("Out of memory for DIB bits\n");
        return FALSE;
    }

    // Seek to the bits in the file.
    file.Seek(dwFileStart + BmpFileHdr.bfOffBits, CFile::begin);

    // read the bits
    nBytes = file.Read(pBits, nBitsSize);
    if (nBytes != nBitsSize) 
    {
        TRACE0("Failed to read bits\n");
        FREE(NULL, pBits);
		return FALSE;
    }

	int nBmpHeight = ABS(BmpInfo.bmiHeader.biHeight);
	BOOL bBottomUp = BmpInfo.bmiHeader.biHeight > 0;
	BYTE byMaskBase = (1 << (BYTE)BmpInfo.bmiHeader.biBitCount) - 1;
	BYTE byMask = 0;
	BYTE *src = NULL;
	BYTE *p = NULL;
	int nWidthInBytes = (BmpInfo.bmiHeader.biWidth * BmpInfo.bmiHeader.biBitCount) / 8;
	for (int y = 0; y < LGLCD_BMP_HEIGHT; y++)
	{
		for (int x = 0; x < LGLCD_BMP_WIDTH; x++)
		{
			p = &(tLCDBitmap.pixels[(y * LGLCD_BMP_WIDTH) + x]);
			if (x < BmpInfo.bmiHeader.biWidth && y < nBmpHeight)
			{
				if (bBottomUp)
					src = pBits + ((nBmpHeight - (y+1)) * nWidthInBytes);		 
				else
					src = pBits + (y * nWidthInBytes);		 // get the right row

				if (BmpInfo.bmiHeader.biBitCount >= 8)
				{
					src += BmpInfo.bmiHeader.biBitCount * x;
				}
				else
				{
					int nParts = 8 / BmpInfo.bmiHeader.biBitCount;
					src += x / nParts;
					int nShift = ((nParts - 1) - (x % nParts)) * BmpInfo.bmiHeader.biBitCount;
					byMask = (byMaskBase << (BYTE)nShift);
				}
				*p = (*src & byMask ? PIXEL_ON : PIXEL_OFF);
			}
			else
			{
				*p = PIXEL_OFF;
			}
		}
	}

    FREE(NULL, pBits);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UILogLCDInit(
	void)
{
	DWORD dwResult = lgLcdInit();

	memclear(&g_LCD, sizeof(UILCDGlobals));

	g_LCD.m_bLCDAvail = (dwResult == ERROR_SUCCESS);

	if (!g_LCD.m_bLCDAvail)
		return FALSE;

    //// connect to LCDMon
    // set up connection context
    g_LCD.m_tConnectCtxt.appFriendlyName = AppGetName();
    g_LCD.m_tConnectCtxt.isAutostartable = FALSE;
    g_LCD.m_tConnectCtxt.isPersistent = TRUE;
    // we don't have a configuration screen
    g_LCD.m_tConnectCtxt.onConfigure.configCallback = NULL;
    g_LCD.m_tConnectCtxt.onConfigure.configContext = NULL;
    // the "connection" member will be set upon return
    g_LCD.m_tConnectCtxt.connection = LGLCD_INVALID_CONNECTION;
    // and connect
    dwResult = lgLcdConnect(&g_LCD.m_tConnectCtxt);
	if (dwResult != ERROR_SUCCESS)
	{
		g_LCD.m_bLCDAvail = FALSE;
		return FALSE;
	}

    // now we are connected (and have a connection handle returned),
    // let's enumerate an LCD (the first one, index = 0)
    dwResult = lgLcdEnumerate(g_LCD.m_tConnectCtxt.connection, 0, &g_LCD.m_tDeviceDesc);
	if (dwResult != ERROR_SUCCESS)
	{
		g_LCD.m_bLCDAvail = FALSE;
		return FALSE;
	}

    // open the context (this may need to be re-opened later
	//   for example if the user unplugs the replugs the keyboard.
	//   so we need to check this again.
    g_LCD.m_tOpenCtxt.connection = g_LCD.m_tConnectCtxt.connection;
    g_LCD.m_tOpenCtxt.index = 0;
    // we have no softbutton notification callback
    g_LCD.m_tOpenCtxt.onSoftbuttonsChanged.softbuttonsChangedCallback = NULL;
    g_LCD.m_tOpenCtxt.onSoftbuttonsChanged.softbuttonsChangedContext = NULL;
    // the "device" member will be returned upon return
    g_LCD.m_tOpenCtxt.device = LGLCD_INVALID_DEVICE;
    dwResult = lgLcdOpen(&g_LCD.m_tOpenCtxt);

	g_LCD.m_tBitmap.hdr.Format = LGLCD_BMP_FORMAT_160x43x1;

	LoadBitmap("data\\uix\\temp LCD bkgd.bmp", g_LCD.m_tBkgdBitmap);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILogLCDFree(
	void)
{
	if (g_LCD.m_bLCDAvail)
	{
	    lgLcdClose(g_LCD.m_tOpenCtxt.device);

		if (g_LCD.m_tConnectCtxt.connection != LGLCD_INVALID_CONNECTION)
		{
			lgLcdDisconnect(g_LCD.m_tConnectCtxt.connection);
		}

		lgLcdDeInit();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void DrawBitmap(
	lgLcdBitmap160x43x1 &tBitmap)
{
	memcpy(g_LCD.m_tBitmap.pixels, tBitmap.pixels, sizeof(tBitmap.pixels));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void SetPixel(
	int x,
	int y,
	BYTE val)
{
	ASSERT(x >= 0 && x < LGLCD_BMP_WIDTH);
	ASSERT(y >= 0 && y < LGLCD_BMP_HEIGHT);

	g_LCD.m_tBitmap.pixels[(y * LGLCD_BMP_WIDTH) + x] = val;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void TogglePixel(
	int x,
	int y)
{
	ASSERT(x >= 0 && x < LGLCD_BMP_WIDTH);
	ASSERT(y >= 0 && y < LGLCD_BMP_HEIGHT);

	BYTE *p = &(g_LCD.m_tBitmap.pixels[(y * LGLCD_BMP_WIDTH) + x]);
	*p = (*p != PIXEL_OFF ? PIXEL_OFF : PIXEL_ON);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int DrawNumeral(
	int x,
	int y,
	int n,
	BOOL bSwitch)
{
	if (n < 0 || n > 9)
		return 0;

	ASSERT(x >= 0 && x+scnNumeralW < LGLCD_BMP_WIDTH);
	ASSERT(y >= 0 && y+scnNumeralH < LGLCD_BMP_HEIGHT);
	
	if (!bSwitch)
	{
		for (int j=0; j<scnNumeralH; j++)
		{
			void *dst = (void *)&(g_LCD.m_tBitmap.pixels[((j+y) * LGLCD_BMP_WIDTH) + x]);
			void *src = (void *)&(sccNumeral[(j * scnNumeralScanW) + (scnNumeralW * n)]);
			memcpy(dst, src, scnNumeralW);
		}
	}
	else
	{
		for (int j=0; j<scnNumeralH; j++)
		{
			for (int i=0; i<scnNumeralW; i++)
			{
				unsigned char src = sccNumeral[(j * scnNumeralScanW) + (scnNumeralW * n) + i];
				if (src)
				{
					TogglePixel(i+x, j+y);
				}
			}
		}
	}

	return scnNumeralW;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int DrawChar(
	int x,
	int y,
	char c,
	BOOL bSwitch)
{
	if (c >= '0' && c <= '9')
		return DrawNumeral(x, y, c - '0', bSwitch);

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void DrawText(
	int x,
	int y,
	const char *str,
	BOOL bSwitch = FALSE)
{
	const char *p = str;

	int nLastWidth = 0;
	while (p && *p)
	{
		nLastWidth = DrawChar(x, y, *p, bSwitch);
		p++;
		if (nLastWidth > 0)
			x += nLastWidth + 1;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void DrawNumber(
	int x,
	int y,
	int n,
	BOOL bSwitch = FALSE)
{
	char buf[256];
	PStrPrintf(buf, arrsize(buf), "%d", n);

	DrawText(x, y, buf, bSwitch);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILogLCDDrawBar(
	int x,
	int y,
	int w,
	int h,
	int nCurVal,
	int nMaxVal)
{
	int nCutoff = x + (int)(((float)nCurVal / (float)nMaxVal) * (float)(w - 1));

	for (int i = x; i < x + w; i++)
	{
		for (int j = y; j < y + h; j++)
		{
			if (i==x || i+1==x+w || j==y || j+1==y+h)	// if this is part of the border
			{
				SetPixel(i, j, PIXEL_ON);
			}
			else
			{
				if (i <= nCutoff)
					SetPixel(i, j, PIXEL_ON);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILogLCDDrawSteppingBar(
	int x,
	int y,
	int w,
	int h,
	int nSteps,
	//int nCurVal,
	//int nMaxVal,
	float fPercent,
	BOOL bVertical,
	UINT nSmallCorner)	// clockwise - with upper left being 0
{
	if (nSteps <= 0)
		return;

	int nCutoff = (int)(fPercent * (float)(bVertical ? h : w));
	int curx = (nSmallCorner == 0 || nSmallCorner == 3 ? x : x + w);
	int dx = (nSmallCorner == 0 || nSmallCorner == 3 ? 1 :-1);
	int cury = (nSmallCorner == 0 || nSmallCorner == 1 ? y : y + h);
	int dy = (nSmallCorner == 0 || nSmallCorner == 1 ? 1 :-1);

	for (int i = 0; i <= nCutoff; i+=nSteps)
	{
		for (int j=0; j <= i / nSteps; j++)
		{
			SetPixel(curx + (bVertical * j * dx), cury + (!bVertical * j * dy), PIXEL_ON);
		}

		cury += nSteps * dy * bVertical;
		curx += nSteps * dx * !bVertical;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILogLCDUpdate(
	void)
{
	// called from UIProcess.  Update LCD screen

	if (!g_LCD.m_bLCDAvail)
		return;

	GAME *pGame = AppGetCltGame();
	if (!pGame)
		return;

	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return;

	memclear(g_LCD.m_tBitmap.pixels, sizeof(g_LCD.m_tBitmap.pixels));

	DrawBitmap(g_LCD.m_tBkgdBitmap);

#define BAR_WIDTH	(82)
#define BAR_HEIGHT	(8)
#define BAR_LEFT	((LGLCD_BMP_WIDTH - BAR_WIDTH) / 2)

	int y = 0;

	UILogLCDDrawBar(BAR_LEFT, ++y, BAR_WIDTH, BAR_HEIGHT+2, UnitGetStat(pPlayer, STATS_HP_CUR), UnitGetStat(pPlayer, STATS_HP_MAX));
	DrawNumber(BAR_LEFT + 2, y+2, UnitGetStatShift(pPlayer, STATS_HP_CUR), TRUE);
	y+= BAR_HEIGHT + 2;

	if (AppIsHellgate())
	{
		UILogLCDDrawBar(BAR_LEFT, ++y, BAR_WIDTH, BAR_HEIGHT, UnitGetStat(pPlayer, STATS_SHIELD_BUFFER_CUR), UnitGetStat(pPlayer, STATS_SHIELD_BUFFER_MAX));
		DrawNumber(BAR_LEFT + 2, y+1, UnitGetStatShift(pPlayer, STATS_SHIELD_BUFFER_CUR), TRUE);
		y += BAR_HEIGHT;
	}

	UILogLCDDrawBar(BAR_LEFT, ++y, BAR_WIDTH, BAR_HEIGHT, UnitGetStat(pPlayer, STATS_POWER_CUR), UnitGetStat(pPlayer, STATS_POWER_MAX));
	DrawNumber(BAR_LEFT + 2, y+1, UnitGetStatShift(pPlayer, STATS_POWER_CUR), TRUE);
	y += BAR_HEIGHT;

	if (AppIsHellgate())
	{
		int nLeftSkill = UnitGetStat(pPlayer, STATS_SKILL_LEFT);
		int nRightSkill = UnitGetStat(pPlayer, STATS_SKILL_RIGHT);

		float fScale = 1.0f;
		BOOL bIconIsRed = FALSE;
		int nTicksRemaining = 0;
		c_SkillGetIconInfo( pPlayer, nLeftSkill, bIconIsRed, fScale, nTicksRemaining, TRUE );

		if (!bIconIsRed)
			UILogLCDDrawSteppingBar(BAR_LEFT - 20, 1, 18, 28, 2, fScale, 1, 2);

		c_SkillGetIconInfo( pPlayer, nRightSkill, bIconIsRed, fScale, nTicksRemaining, TRUE );

		if (!bIconIsRed)
			UILogLCDDrawSteppingBar(BAR_LEFT + BAR_WIDTH + 2, 1, 18, 28, 2, fScale, 1, 3);
	}



	lgLcdUpdateBitmap(g_LCD.m_tOpenCtxt.device, &g_LCD.m_tBitmap.hdr, LGLCD_ASYNC_UPDATE(LGLCD_PRIORITY_NORMAL));
}

#endif // #if !ISVERSION(SERVER_VERSION)