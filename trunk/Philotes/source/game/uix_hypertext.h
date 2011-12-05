//----------------------------------------------------------------------------
// uix_hypertext.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
// Date: 2/28/08
// Author: Kevin Klemmick
//
//----------------------------------------------------------------------------

#ifndef _UIX_HYPERTEXT_H_
#define _UIX_HYPERTEXT_H_

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------

class UI_HYPERTEXT;
class UI_LINE;
struct CHAT_MESSAGE;
struct GAMECLIENT;
struct GAME;

//----------------------------------------------------------------------------
// Hypertext types
//----------------------------------------------------------------------------

enum HYPERTEXT_TYPE
{
	HT_ITEM		= 0,
	HT_LINK,
	HT_LAST
};

//----------------------------------------------------------------------------
// Hypertext class (Named to match other similar UI objects)
// Very, very basic hypertext implementation which holds information on the
// location, type and data of a hypertext link within a UI_LINE object.
//----------------------------------------------------------------------------
class UI_HYPERTEXT
{
public:
	int						m_nId;
	UI_POSITION				m_UpperLeft;
	UI_POSITION				m_LowerRight;
	int						m_nDataType;				// Type of data we have
	UCHAR*					m_pDataPointer;				// Pointer to whatever data we need
	int						m_nDataSize;				// Size of data pointed to
	unsigned __int64		m_nData;					// Spot for smallish amounts of data
	BOOL					m_pDataRequiresFree;		// Set if we need to free the data pointer
	int						m_nTypeIndex;				// Index into the hypertext data table
	WCHAR*					m_szMarkupText;				// The original markup text
	int						m_nMarkupTextSize;

public:
	UI_HYPERTEXT();
	~UI_HYPERTEXT();

	BOOL				Encode(UCHAR * buffer, int * nCurPos, int nMaxSize);
	BOOL				Decode(UCHAR * buffer, int * nCurPos);
};

//----------------------------------------------------------------------------
// functions
//----------------------------------------------------------------------------

// Initial processing of text with HTML-like hypertext tags in it
BOOL UIProcessHyperText(UI_LINE* pLine);

// Reprocessing of already processed text, after word wrapping or resize
void UIReprocessHyperText(UI_LINE* pNewLine);

// Quick lookup function to find a Hypertext object by Id
UI_HYPERTEXT* UIFindHypertextById(WCHAR wc);

// Looks for any hypertext object belonging to the component passed that is under the x,y coordinates passed
UI_HYPERTEXT* UIFindHyperTextUnderMouse(	
	UI_COMPONENT* component,
	DWORD xPos,
	DWORD yPos );

// Mouse event handler functions. 
UI_MSG_RETVAL UIHypertextOnLMouseDown(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIHypertextOnMouseHover(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIHypertextOnMouseMove(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);

// CHAT_MESSAGE encode and decode functions
BOOL UICurrentConsoleLineIsHypertext(const UI_LINE * pLine);
DWORD c_UIEncodeHypertextForGameServer(const WCHAR * message, const UI_LINE * pLine, BYTE * destBuffer, DWORD destBufferLen);
BOOL s_ValididateReprocessAndForwardHypertext(GAME * pGame, GAMECLIENT * client, WCHAR * message, BYTE * dataIn, DWORD dataInLen, BYTE * dataOut, DWORD dataLenOut);
void c_UIDecodeHypertextMessageFromChatServer(WCHAR * message, UI_LINE * pLine, BYTE * data, DWORD dataLen);


#endif // _UIX_HYPERTEXT_H_