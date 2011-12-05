//----------------------------------------------------------------------------
// e_ui.h
//
// - Header for UI functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_UI__
#define __E_UI__


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define UI_CHARACTERS_IN_FONT			128
#define UI_MAX_QUADSTRIP_POINTS			722
#define MAX_DEBUG_LABEL_DISPLAY_STRING	2048

#define UI_PRIM_ALPHABLEND		MAKE_MASK(0)
#define UI_PRIM_ZTEST			MAKE_MASK(1)
#define UI_PRIM_ZWRITE			MAKE_MASK(2)

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum SCREEN_VERTEX_TYPE
{
	SCREEN_VERTEX_XYZUV = 0,
	SCREEN_VERTEX_XYZ,
	// count
	NUM_SCREEN_VERTEX_TYPES
};

enum UI_ALIGN
{
	UIALIGN_TOPLEFT			= 0,
	UIALIGN_TOP				= 1,
	UIALIGN_TOPRIGHT		= 2,
	UIALIGN_RIGHT			= 3,
	UIALIGN_CENTER			= 4,
	UIALIGN_LEFT			= 5,
	UIALIGN_BOTTOMLEFT		= 6,
	UIALIGN_BOTTOM			= 7,
	UIALIGN_BOTTOMRIGHT		= 8,
	UIALIGN_NUM,
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef void (*PFN_UI_RENDER_ALL)();
typedef void (*PFN_UI_RESTORE)();
typedef void (*PFN_UI_DISPLAY_CHANGE)(bool bPost);	// CHB 2007.08.23
typedef void (*PFN_UI_RENDER_CALLBACK)( const class UI_RECT & tScreenRect, DWORD dwColor, float fZDelta, float fScale );

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// a location in screen space (or logical space)
class UI_POSITION
{
public:
	float					m_fX;
	float					m_fY;

	UI_POSITION() 
		: m_fX(0.0f), m_fY(0.0f) {}

	UI_POSITION(float x, float y)
		: m_fX(x), m_fY(y) {}

	bool operator== (const UI_POSITION& rhs) const { return m_fX == rhs.m_fX && m_fY == rhs.m_fY; }
	bool operator!= (const UI_POSITION& rhs) const { return m_fX != rhs.m_fX || m_fY != rhs.m_fY; }
	void operator+= (const UI_POSITION& rhs) { m_fX += rhs.m_fX; m_fY += rhs.m_fY; }
	void operator-= (const UI_POSITION& rhs) { m_fX -= rhs.m_fX; m_fY -= rhs.m_fY; }
	UI_POSITION operator+ (const UI_POSITION& rhs) const { UI_POSITION ret; ret.m_fX = m_fX + rhs.m_fX; ret.m_fY = m_fY + rhs.m_fY; return ret; }
	UI_POSITION operator- (const UI_POSITION& rhs) const { UI_POSITION ret; ret.m_fX = m_fX - rhs.m_fX; ret.m_fY = m_fY - rhs.m_fY; return ret; }
};

// basically the save as position (two floats) but renamed for clarity
class UI_SIZE
{
public:
	float					m_fWidth;
	float					m_fHeight;

	UI_SIZE() 
		: m_fWidth(0.0f), m_fHeight(0.0f) {}

	UI_SIZE(float x, float y)
		: m_fWidth(x), m_fHeight(y) {}

	// this avoids the warning C4238: nonstandard extension used : class rvalue used as lvalue
	UI_SIZE * operator&(void)
	{
		return (UI_SIZE *)this;
	}
};

// a rect in screen space
class UI_RECT
{
public:
	float m_fX1;
	float m_fY1;
	float m_fX2;
	float m_fY2;

	UI_RECT()
		: m_fX1(0.0f), m_fY1(0.0f),
		  m_fX2(0.0f), m_fY2(0.0f) {}

	UI_RECT(float x1, float y1, float x2, float y2)
		: m_fX1(x1), m_fY1(y1),
		  m_fX2(x2), m_fY2(y2) {}

	// this avoids the warning C4238: nonstandard extension used : class rvalue used as lvalue
	UI_RECT * operator&(void)
	{
		return (UI_RECT *)this;
	}

	void Normalize(void)
	{
		if (m_fX1 > m_fX2)
			SWAP(m_fX1, m_fX2);
		if (m_fY1 > m_fY2)
			SWAP(m_fY1, m_fY2);
	}

	float Width(void)	{ return m_fX2 - m_fX1; }
	float Height(void)	{ return m_fY2 - m_fY1; }
	void SetPos(float x, float y)	{	m_fX2 = x + Width(); m_fX1 = x;
										m_fY2 = y + Height(); m_fY1 = y;}	
	void Translate(float x, float y){	m_fX1 += x; m_fX2 += x;
										m_fY1 += y;	m_fY2 += y;}
	BOOL DoClip( const UI_RECT& cliprect )
	{
		if ( m_fX1 >= cliprect.m_fX2 ||
			 m_fX2 <= cliprect.m_fX1 ||
			 m_fY1 >= cliprect.m_fY2 ||
			 m_fY2 <= cliprect.m_fY1 )
		{
			return FALSE;
		}

		if ( cliprect.m_fX1 >= cliprect.m_fX2 ||
			 cliprect.m_fY1 >= cliprect.m_fY2 )
		{
			return FALSE;
		}

		if (m_fX1 < cliprect.m_fX1)
			m_fX1 = cliprect.m_fX1;
		if (m_fX2 > cliprect.m_fX2)
			m_fX2 = cliprect.m_fX2;
		if (m_fY1 < cliprect.m_fY1)
			m_fY1 = cliprect.m_fY1;
		if (m_fY2 > cliprect.m_fY2)
			m_fY2 = cliprect.m_fY2;

		return TRUE;
	}
};

// a rect in screen space
class UI_TEXRECT
{
public:
	float m_fU1;
	float m_fV1;
	float m_fU2;
	float m_fV2;

	UI_TEXRECT()
		: m_fU1(0.0f), m_fV1(0.0f),
		  m_fU2(0.0f), m_fV2(0.0f) {}

	UI_TEXRECT(float u1, float v1, float u2, float v2)
		: m_fU1(u1), m_fV1(v1),
		  m_fU2(u2), m_fV2(v2) {}

	// this avoids the warning C4238: nonstandard extension used : class rvalue used as lvalue
	UI_TEXRECT * operator&(void)
	{
		return (UI_TEXRECT *)this;
	}
};

// a tri in screen space
class UI_TEXTURETRI
{
public:
	float					x1;
	float					y1;
	float					x2;
	float					y2;
	float					x3;
	float					y3;
	float					u1;
	float					v1;
	float					u2;
	float					v2;
	float					u3;
	float					v3;
};

// a rect
class UI_TEXTURERECT
{
public:
	float					x1;
	float					y1;
	float					x2;
	float					y2;
	float					x3;
	float					y3;
	float					x4;
	float					y4;
	float					u1;
	float					v1;
	float					u2;
	float					v2;
	float					u3;
	float					v3;
	float					u4;
	float					v4;
};

// an untextured quadrilateral
class UI_QUAD
{
public:
	float					x1;
	float					y1;
	float					x2;
	float					y2;
	float					x3;
	float					y3;
	float					x4;
	float					y4;
};

// an untextured quad list
class UI_QUADSTRIP
{
public:
	VECTOR2					vPoints[ UI_MAX_QUADSTRIP_POINTS ];
	int						nPointCount;
};

// describes a single character in a font
struct UI_TEXTURE_FONT_FRAME
{
	int						nId;			//for hash
	UI_TEXTURE_FONT_FRAME	*pNext;			//for hash
	float					m_fPreWidth;
	float					m_fWidth;
	float					m_fPostWidth;
	float					m_fU1;
	float					m_fV1;
	float					m_fU2;
	float					m_fV2;
	int						m_nTextureFontRowId;
};

struct UI_TEXTURE_FRAME_CHUNK
{
	float					m_fXOffset;			// offset at which to draw this chunk
	float					m_fYOffset;
	float					m_fWidth;
	float					m_fHeight;
	float					m_fU1;
	float					m_fV1;
	float					m_fU2;
	float					m_fV2;
};

// describes a rectangle (frame) in an atlas
//  can be composed of one or more chunks
struct UI_TEXTURE_FRAME
{
	float					m_fXOffset;			// offset at which to draw the whole frame
	float					m_fYOffset;
	float					m_fWidth;
	float					m_fHeight;
	UI_TEXTURE_FRAME_CHUNK* m_pChunks;
	WORD					m_nNumChunks;
	DWORD					m_dwDefaultColor;
	BOOL					m_bHasMask;
	int						m_nMaskSize;
	BYTE*					m_pbMaskBits;

	UI_TEXTURE_FRAME*		m_pNextFrame;
	DWORD					m_dwNextFrameDelay;
};

struct CHAR_HASH : CHash<UI_TEXTURE_FONT_FRAME>
{
	int			nId;			//for hash of hashes
	CHAR_HASH	*pNext;			//for hash of hashes
};

// describes a font in an atlas
struct UIX_TEXTURE_FONT
{
	char					m_szSystemName[DEFAULT_FILE_WITH_PATH_SIZE];
	char					m_szAsciiFontName[DEFAULT_FILE_WITH_PATH_SIZE];
	char					m_szLocalPath[DEFAULT_FILE_WITH_PATH_SIZE];
	char					m_szAsciiLocalPath[DEFAULT_FILE_WITH_PATH_SIZE];
#if (ISVERSION(DEBUG_VERSION))
	char					m_szDebugName[DEFAULT_FILE_WITH_PATH_SIZE];
#endif
	BOOL					m_bBold;
	BOOL					m_bItalic;
	int						m_nWeight;
	float					m_fWidthRatio;
	float					m_fTabWidth;
	BOOL					m_bLoadedFromFile;
	int						m_nExtraSpacing;
	HANDLE					m_hInstalledFont;
	HANDLE					m_hInstalledAsciiFont;
	struct UIX_TEXTURE *	m_pTexture;
	GLYPHSET *				m_pGlyphset;

	UI_TEXTURE_FONT_FRAME * GetChar(WCHAR wc, int nSize)
	{
		ASSERT(nSize > 0);
		CHAR_HASH *pCharHash = HashGet(m_hashSizes, nSize);
		if (!pCharHash)
		{
			return NULL;
		}

		return HashGet(*pCharHash, (int)wc);
	}
	UI_TEXTURE_FONT_FRAME * AddChar(WCHAR wc, int nSize)
	{
		ASSERT(nSize > 0);
		CHAR_HASH *pCharHash = HashGet(m_hashSizes, nSize);
		if (!pCharHash)
		{
			pCharHash = HashAddSorted(m_hashSizes, nSize);
			if (!pCharHash)
			{
				return NULL;
			}
			
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
			HashInit((*pCharHash), g_StaticAllocator, 512);		// should make this number based on the size of the font (sqrt of # of characters)
#else
			HashInit((*pCharHash), NULL, 512);		// should make this number based on the size of the font (sqrt of # of characters)
#endif			
		}

		UI_TEXTURE_FONT_FRAME * pRet = HashAddSorted(*pCharHash, (int)wc);
		ASSERT_RETNULL(pRet);
		pRet->m_nTextureFontRowId = INVALID_ID;
		return pRet;
	}
	PRESULT Init(
		UIX_TEXTURE *pTexture);
	void Destroy(
		void);
	void Reset(
		void);
	void CheckRows(
		void);

protected:
//	typedef	CHash<UI_TEXTURE_FONT_FRAME>	CHAR_HASH;
	CHash<CHAR_HASH>	m_hashSizes;
};

struct UIX_TEXTURE_FONT_ROW
{
	float	m_fRowEndX;
	float	m_fRowY;
	float	m_fRowHeight;
	DWORD	m_dwLastAccess;

	CArrayIndexed<UI_TEXTURE_FONT_FRAME *>	m_arrFontCharacters;
};

struct UIX_TEXTURE
{
	char					m_pszFileName[DEFAULT_FILE_WITH_PATH_SIZE];

	DWORD					m_dwFlags;					// flags
	DWORD					m_dwFVF;					// vertex format

	int						m_nTextureId;
	float					m_fTextureWidth;
	float					m_fTextureHeight;
	float					m_XMLWidthRatio;
	float					m_XMLHeightRatio;
	int						m_nDrawPriority;			// this will determine the order textures are rendered within the same render section

	UI_TEXTURE_FRAME*		m_pBoxFrame;

	UI_RECT					m_rectFontArea;
	CArrayIndexed<UIX_TEXTURE_FONT_ROW>		m_arrFontRows;

	struct STR_DICTIONARY*	m_pFrames;
	struct STR_DICTIONARY*	m_pFonts;
};

struct UI_DEBUG_LABEL
{
	WCHAR wszLabel[ MAX_DEBUG_LABEL_DISPLAY_STRING ];
	VECTOR vPos;
	BOOL bWorldSpace;
	float fScale;
	DWORD dwColor;
	UI_ALIGN eAnchor;	
	UI_ALIGN eAlign;	
	BOOL bPinToEdge;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------
inline BOOL e_UIAlignIsBottom(
	int nAlign)
{
	return (nAlign == UIALIGN_BOTTOM || nAlign == UIALIGN_BOTTOMRIGHT || nAlign == UIALIGN_BOTTOMLEFT);
}

inline BOOL e_UIAlignIsTop(
	int nAlign)
{
	return (nAlign == UIALIGN_TOP || nAlign == UIALIGN_TOPRIGHT || nAlign == UIALIGN_TOPLEFT);
}

inline BOOL e_UIAlignIsRight(
	int nAlign)
{
	return (nAlign == UIALIGN_RIGHT || nAlign == UIALIGN_TOPRIGHT || nAlign == UIALIGN_BOTTOMRIGHT);
}

inline BOOL e_UIAlignIsLeft(
	int nAlign)
{
	return (nAlign == UIALIGN_LEFT || nAlign == UIALIGN_TOPLEFT || nAlign == UIALIGN_BOTTOMLEFT);
}

inline BOOL e_UIAlignIsCenterHoriz(
	int nAlign)
{
	return (nAlign == UIALIGN_TOP || nAlign == UIALIGN_BOTTOM || nAlign == UIALIGN_CENTER);
}

inline BOOL e_UIAlignIsCenterVert(
	int nAlign)
{
	return (nAlign == UIALIGN_LEFT || nAlign == UIALIGN_RIGHT || nAlign == UIALIGN_CENTER);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL e_UIElementDoClip(
	UI_RECT* drawrect,
	UI_TEXRECT* texrect,
	UI_RECT* cliprect)
{
	if (drawrect->m_fX1 >= cliprect->m_fX2 ||
		drawrect->m_fX2 <= cliprect->m_fX1 ||
		drawrect->m_fY1 >= cliprect->m_fY2 ||
		drawrect->m_fY2 <= cliprect->m_fY1)
	{
		return FALSE;
	}
	if (cliprect->m_fX1 >= cliprect->m_fX2 ||
		cliprect->m_fY1 >= cliprect->m_fY2)
	{
		return FALSE;
	}
	float width = drawrect->m_fX2 - drawrect->m_fX1;
	float height = drawrect->m_fY2 - drawrect->m_fY1;

	float texturewidth = texrect->m_fU2 - texrect->m_fU1;
	float textureheight = texrect->m_fV2 - texrect->m_fV1;

	if (drawrect->m_fX1 < cliprect->m_fX1)
	{
		texrect->m_fU1 += texturewidth * (cliprect->m_fX1 - drawrect->m_fX1)/width;
		drawrect->m_fX1 = cliprect->m_fX1;
	}
	if (drawrect->m_fX2 > cliprect->m_fX2)
	{
		texrect->m_fU2 -= texturewidth * (drawrect->m_fX2 - cliprect->m_fX2)/width;
		drawrect->m_fX2 = cliprect->m_fX2;
	}
	if (drawrect->m_fY1 < cliprect->m_fY1)
	{
		texrect->m_fV1 += textureheight * (cliprect->m_fY1 - drawrect->m_fY1)/height;
		drawrect->m_fY1 = cliprect->m_fY1;
	}
	if (drawrect->m_fY2 > cliprect->m_fY2)
	{
		texrect->m_fV2 -= textureheight * (drawrect->m_fY2 - cliprect->m_fY2)/height;
		drawrect->m_fY2 = cliprect->m_fY2;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_UIPrimitiveArcSectorGetCounts(
	float fSegmentWidthDeg,
	int * pVertexCount,
	int * pIndexCount )
{
	// return the max number of verts/inds necessary for this segment width
	// (reduces reallocs)
	*pVertexCount = 2 * (int)( CEIL( 360.f / fSegmentWidthDeg ) + 1 );
	*pIndexCount  = ( *pVertexCount - 2 ) * 3;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline const struct UI_DEBUG_LABEL * e_UIGetDebugLabels( int & nCount )
{
	extern SIMPLE_DYNAMIC_ARRAY<UI_DEBUG_LABEL> gtUIDebugLabels;
	nCount = gtUIDebugLabels.Count();
	if ( 0 == nCount )
		return NULL;
	return &gtUIDebugLabels[0];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void e_UIClearDebugLabels()
{
	extern SIMPLE_DYNAMIC_ARRAY<UI_DEBUG_LABEL> gtUIDebugLabels;
	ArrayClear(gtUIDebugLabels);
}


//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

BOOL e_UIEngineBufferIsDynamic(
	int nBufferID );

PRESULT e_UICopyToEngineBuffer(
	int nLocalBufferID,
	int & nEngineBufferID,
	BOOL bDynamic,
	BOOL bForce = FALSE );

PRESULT e_UIEngineBufferRestoreResize(
	int nEngineBufferID,
	int nLocalBufferID,
	BOOL bDynamic,
	BOOL bForce = FALSE );

PRESULT e_UITextureFontAddCharactersToTexture(
	UIX_TEXTURE_FONT* font,
	const WCHAR *szCharacters,
	int nFontSize);

float e_UIFontGetCharWidth(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio, 
	WCHAR wc);

float e_UIFontGetCharPreWidth(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio, 
	WCHAR wc);

float e_UIFontGetCharPostWidth(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio, 
	WCHAR wc);

int e_UIGetLocalBufferVertexCurCount( int nLocalBufferID );

int e_UIGetLocalBufferVertexMaxCount( int nLocalBufferID );

PRESULT e_UIGetNewEngineBuffer(
	int nLocalBufferID,
	int& nEngineBufferID,
	BOOL bDynamic );

PRESULT e_UITextureResizeLocalBuffers(
	int nLocalBufferID,
	int nVertexCount,
	int nIndexCount );

PRESULT e_UIPrimitiveResizeLocalBuffers(
	int nLocalBufferID,
	int nVertexCount,
	int nIndexCount );

PRESULT e_UITextureDrawBuffersFree(
	int nEngineBufferID,
	int nLocalBufferID );

PRESULT e_UIPrimitiveDrawBuffersFree(
	int nEngineBufferID,
	int nLocalBufferID );

PRESULT e_UITextureCreateEmpty(
	int nWidth,
	int nHeight,
	int& nTextureID,
	BOOL bMask );

PRESULT e_UITextureLoadTextureFile(
	int nDefaultId,
	const char* filename,
	int nChunkSize,
	int& nTextureID,
	BOOL bAllowAsync,
	BOOL bLoadFromLocalizedPak);

PRESULT e_UITextureAddFrame( 
	UI_TEXTURE_FRAME *pFrame, 
	int nTextureID, 
	const UI_RECT& rect );

PRESULT e_UIRestoreTextureFile(
	int nTextureID );

PRESULT e_UISaveTextureFile(
	int nTextureID, 
	const char * pszFileNameWithPath,
	BOOL bUpdatePak = TRUE );

PRESULT e_UIWriteElementFrame(
	int nLocalBufferID,
	UI_RECT & drawrect,
	UI_TEXRECT & texrect,
	float fZDelta,
	DWORD dwColor );

PRESULT e_UIWriteRotatedFrame(
	int nLocalBufferID,
	UI_RECT & drawrect,
	float fOrigWidth,
	float fOrigHeight,
	UI_TEXRECT & texrect,
	float fZDelta,
	DWORD dwColor,
	float fRotateCenterX,
	float fRotateCenterY,
	float fAngle );

PRESULT e_UIWriteElementText(
	int nLocalBufferID,
	const WCHAR wc,
	int nFontSize,
	float x1, 
	float y1, 
	float x2, 
	float y2,
	float fZDelta,
	DWORD dwColor,
	UI_RECT & cliprect,
	UIX_TEXTURE_FONT * font);

PRESULT e_UIWriteElementTri(
	int nLocalBufferID,
	const UI_TEXTURETRI & tri,
	float fZDelta,
	DWORD dwColor );

PRESULT e_UIWriteElementRect(
	int nLocalBufferID,
	const UI_TEXTURERECT & tRect,
	float fZDelta,
	DWORD dwColor );

PRESULT e_UIWriteElementQuad(
	int nLocalBufferID,
	const UI_QUAD tQuad,
	float fZDelta,
	DWORD dwColor );

PRESULT e_UIWriteElementQuadStrip(
	int nLocalBufferID,
	const UI_QUADSTRIP & tPoints,
	float fZDelta,
	DWORD dwColor );

PRESULT e_UITrashEngineBuffer(
	int & nEngineBuffer );

PRESULT e_UITextureDrawClearLocalBufferCounts(
	int nLocalBuffer );

PRESULT e_UIPrimitiveDrawClearLocalBufferCounts(
	int nLocalBuffer );

PRESULT e_UIGetNewLocalBuffer( int& nID );

PRESULT e_UIFree();

PRESULT e_UIInit();

PRESULT e_UIDestroy();

PRESULT e_UITextureRender(
	int nTextureID,
	int nEngineBuffer,
	int nLocalBuffer,
	BOOL bOpaque,
	BOOL bGrayout);

PRESULT e_UIPrimitiveRender(
	int nEngineBuffer,
	int nLocalBuffer,
	DWORD dwDrawFlags );

void e_UIInitCallbacks(
	PFN_UI_RENDER_ALL pfnRenderAll,
	PFN_UI_RESTORE pfnRestore,
	PFN_UI_DISPLAY_CHANGE pfnDisplayChange );	// CHB 2007.03.06

void e_UIRenderAll();
PRESULT e_UIRestore();
void e_UIDisplayChange(bool bPost);	// CHB 2007.08.23 - false for pre-change, true for post-change

PRESULT e_UIX_Render();

PRESULT e_UIAddLabel(
	const WCHAR * puszText,
	const VECTOR * pvPos,
	BOOL bWorldSpace,
	float fScale,
	DWORD dwColor = 0xffffffff,
	UI_ALIGN eAnchor = UIALIGN_TOP,
	UI_ALIGN eAlign = UIALIGN_CENTER,
	BOOL bPinToEdge = FALSE );

float e_UIConvertAbsoluteZToDeltaZ(
	float fAbsZ );

PRESULT e_UIElementGetCallbackFunc(
	const char * pszName,
	int nMaxLen,
	PFN_UI_RENDER_CALLBACK & pfnCallback );

void e_UIRequestFontTextureReset(BOOL bRequest);

BOOL e_UIGetFontTextureResetRequest();

#endif // __E_UI__
