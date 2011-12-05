//----------------------------------------------------------------------------
// uix_hypertext.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
// Date: 2/28/08
// Author: Kevin Klemmick
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "e_ui.h"
#include "uix_priv.h"
#include "uix_HyperText.h"
#include "colors.h"
#include "excel.h"
#include "prime.h"
#include "uix_graphic.h"
#include "uix_components.h"
#include "units.h"
#include "unitfile.h"
#include "c_unitnew.h"

#include "c_message.h"
#include "c_connmgr.h"
//#include "EmbeddedServerRunner.h"
//#include "UserChatCommunication.h"

//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------
#define HTDATA_NONE				0
#define HTDATA_UNITID			1
#define HTDATA_UNITGUID			2
#define HTDATA_UNITDATA			3
#define HTDATA_HTTPDATA			4

#define MAX_HYPERTEXT_IDS		1024

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------
UI_MSG_RETVAL static sHTItemOnMouseHover(UI_COMPONENT *pComponent, UI_HYPERTEXT* pHypertext);
UI_MSG_RETVAL static sHTItemOnMouseLeave(UI_COMPONENT* component, UI_HYPERTEXT* pHypertext);
UI_MSG_RETVAL static sHTItemOnMouseDown(UI_COMPONENT* component, UI_HYPERTEXT* pHypertext);
UI_MSG_RETVAL static sHTLinkOnMouseDown(UI_COMPONENT* component, UI_HYPERTEXT* pHypertext);

BOOL static sHTAddColor (UI_HYPERTEXT* pHypertext, const WCHAR* szAttributeData, WCHAR* &szTextIn, int &nDstPos);
BOOL static sHTAddItemId (UI_HYPERTEXT* pHypertext, const WCHAR* szAttributeData, WCHAR* &szTextIn, int &nDstPos);
BOOL static sHTAddText (UI_HYPERTEXT* pHypertext, const WCHAR* szAttributeData, WCHAR* &szTextIn, int &nDstPos);
BOOL static sHTAddHref (UI_HYPERTEXT* pHypertext, const WCHAR* szAttributeData, WCHAR* &szTextIn, int &nDstPos);

//----------------------------------------------------------------------------
// Hyper text tokens
//----------------------------------------------------------------------------

struct HYPERTEXT_DATA_STRUCT
{
	const WCHAR *		m_pszToken;
	HYPERTEXT_TYPE		m_eType;
	UI_MSG_RETVAL		(*m_pfnOnMouseUp)(UI_COMPONENT *pComponent, UI_HYPERTEXT*);
	UI_MSG_RETVAL		(*m_pfnOnMouseDown)(UI_COMPONENT *pComponent, UI_HYPERTEXT*);
	UI_MSG_RETVAL		(*m_pfnOnMouseHover)(UI_COMPONENT *pComponent, UI_HYPERTEXT*);
	UI_MSG_RETVAL		(*m_pfnOnMouseLeave)(UI_COMPONENT *pComponent, UI_HYPERTEXT*);
};

static HYPERTEXT_DATA_STRUCT sgtHypertextDataTable[] = 
{
	{ L"<item ",		HT_ITEM,		NULL,			sHTItemOnMouseDown,			sHTItemOnMouseHover,			sHTItemOnMouseLeave },
	{ L"<a ",			HT_LINK,		NULL,			sHTLinkOnMouseDown,			NULL,							NULL },
	{ NULL,				HT_LAST,		NULL,			NULL,						NULL,							NULL }		// must be last
};

struct HYPERTEXT_ATTRIBUTE_STRUCT
{
	const WCHAR *m_pszToken;
	BOOL		(*m_pfnProcessFunction)(UI_HYPERTEXT*, const WCHAR*, WCHAR*&, int&);
};

static HYPERTEXT_ATTRIBUTE_STRUCT sgtHypertextAttributeTable[] = 
{
	{ L"color",						sHTAddColor },
	{ L"id",						sHTAddItemId },
	{ L"text",						sHTAddText },
	{ L"href",						sHTAddHref },
	{ NULL,							NULL }			// must be last
};

//----------------------------------------------------------------------------
// Hypertext Manager Class
//----------------------------------------------------------------------------

class CHypertextManager
{
public:
	int				m_nHypertextNextId;
	UI_HYPERTEXT*	m_HypertextLookupTable[MAX_HYPERTEXT_IDS];
	int				m_nLastHypertextId;
	BOOL			m_bAddColorReturn;

public:
	CHypertextManager(void)
	{
		m_nHypertextNextId = 1;
		m_nLastHypertextId = 0;
		memset(m_HypertextLookupTable, 0, sizeof(UI_HYPERTEXT*) * MAX_HYPERTEXT_IDS);
	}

	// Creates new Hypertext object. All creation should go through this function
	UI_HYPERTEXT* GetNewHypertextObject(int nType)
	{
		UI_HYPERTEXT* pNewHypertext;
		int	nNewId = m_nHypertextNextId;
		while (m_HypertextLookupTable[nNewId] != NULL && nNewId != m_nHypertextNextId-1)
			nNewId = (nNewId >= MAX_HYPERTEXT_IDS)? 1 : nNewId+1;
		ASSERTV_RETNULL (nNewId != m_nHypertextNextId-1, "Ran out of Hypertext ids (current max=%d).", MAX_HYPERTEXT_IDS);

		pNewHypertext = MALLOC_NEW(NULL, UI_HYPERTEXT);
		pNewHypertext->m_nId = nNewId;
		pNewHypertext->m_nTypeIndex = nType;
			
		m_HypertextLookupTable[nNewId] = pNewHypertext;
		m_nHypertextNextId = (nNewId >= MAX_HYPERTEXT_IDS)? 1 : nNewId+1;

		return pNewHypertext;
	}

	// Fast lookup of Hypertext pointer by id
	UI_HYPERTEXT* FindHypertextById(int nId)
	{
		return m_HypertextLookupTable[nId];
	}

	UI_HYPERTEXT* GetLastHypertextObject(void)
	{
		if (m_nLastHypertextId != 0)
			return FindHypertextById(m_nLastHypertextId);
		return NULL;
	}

	// Called on hypertext activate/deactivate
	void SetLastHypertextObjectId(int nId)
	{
		m_nLastHypertextId = nId;
	}

	// Called on object deletion
	void RemoveId(int nId)
	{
		m_HypertextLookupTable[nId] = NULL;
	}
};

// The actual manager instance, created on startup.
CHypertextManager	sHypertextManager;

//----------------------------------------------------------------------------
// Hypertext Class member functions
//----------------------------------------------------------------------------

UI_HYPERTEXT::UI_HYPERTEXT()
: m_nId (0)
, m_nDataType(0)
, m_pDataPointer(NULL)
, m_nDataSize(0)
, m_nData(0)
, m_pDataRequiresFree(FALSE)
, m_nTypeIndex(0)
, m_szMarkupText(NULL)
, m_nMarkupTextSize(0)
{
}

UI_HYPERTEXT::~UI_HYPERTEXT()
{
	sHypertextManager.RemoveId(m_nId);
	if (m_pDataRequiresFree)
		FREE(NULL, m_pDataPointer);
	m_pDataPointer = NULL;
	if (m_szMarkupText)
		FREE_DELETE_ARRAY(NULL, m_szMarkupText, WCHAR);
	m_szMarkupText = NULL;
}

static BOOL sEncodeData(UCHAR * buffer, UCHAR * data, int nSize, int * nPos, int nMaxSize)
{
	ASSERTV_RETFALSE(*nPos + nSize <= nMaxSize, "sEncodeData: Hypertext data exceeded max blob size of %d.", nMaxSize);
	if (*nPos + nSize > nMaxSize)
		return FALSE;
	memcpy(&buffer[*nPos], data, nSize);
	*nPos += nSize;
	return TRUE;
}

BOOL UI_HYPERTEXT::Encode(UCHAR * buffer, int * nCurPos, int nMaxSize)
{
	WORD		wSize = (WORD)(*nCurPos);		
	*nCurPos += sizeof(WORD);				// Reserve space for the total size
	if (!sEncodeData(buffer, (UCHAR*) &m_nId, sizeof(m_nId), nCurPos, nMaxSize))
		return FALSE;
	if (!sEncodeData(buffer, (UCHAR*) &m_nTypeIndex, sizeof(m_nTypeIndex), nCurPos, nMaxSize))
		return FALSE;
	if (!sEncodeData(buffer, (UCHAR*) &m_nDataType, sizeof(m_nDataType), nCurPos, nMaxSize))
		return FALSE;
	if (!sEncodeData(buffer, (UCHAR*) &m_nData, sizeof(m_nData), nCurPos, nMaxSize))
		return FALSE;
	if (!sEncodeData(buffer, (UCHAR*) &m_nDataSize, sizeof(m_nDataSize), nCurPos, nMaxSize))
		return FALSE;
	if (m_nDataSize > 0 && !sEncodeData(buffer, m_pDataPointer, m_nDataSize+1, nCurPos, nMaxSize))
		return FALSE;
	wSize = (WORD)(*nCurPos) - wSize;
	memcpy(buffer, &wSize, sizeof(WORD));
	return TRUE;
}

BOOL UI_HYPERTEXT::Decode(UCHAR * buffer, int * nCurPos)
{
	WORD	wSize;
	int		nStart = *nCurPos;
	memcpy(&wSize, &buffer[*nCurPos], sizeof(WORD));					*nCurPos += sizeof(WORD);
	memcpy(&m_nId, &buffer[*nCurPos], sizeof(m_nId));					*nCurPos += sizeof(m_nId);
	memcpy(&m_nTypeIndex, &buffer[*nCurPos], sizeof(m_nTypeIndex));		*nCurPos += sizeof(m_nTypeIndex);
	memcpy(&m_nDataType, &buffer[*nCurPos], sizeof(m_nDataType));		*nCurPos += sizeof(m_nDataType);
	memcpy(&m_nData, &buffer[*nCurPos], sizeof(m_nData));				*nCurPos += sizeof(m_nData);
	memcpy(&m_nDataSize, &buffer[*nCurPos], sizeof(m_nDataSize));		*nCurPos += sizeof(m_nDataSize);
	if (m_nDataSize > 0)
	{
		m_pDataPointer = (UCHAR*) MALLOC(NULL, m_nDataSize + 1);
		memcpy(m_pDataPointer, &buffer[*nCurPos], m_nDataSize+1);		*nCurPos += (m_nDataSize+1);
		m_pDataRequiresFree = TRUE;
	}
	ASSERTV_RETFALSE(*nCurPos - nStart == (int)wSize, "UI_HYPERTEXT::Decode: Decoded different amount of data than size tag size: %d, decoded: %d\n", wSize, *nCurPos - nStart);
	return TRUE;
}

//----------------------------------------------------------------------------
// Hyper text processing functions
//----------------------------------------------------------------------------

// Add a color token to a string
BOOL static sHTAddColor (UI_HYPERTEXT* pHypertext, const WCHAR* szAttributeData, WCHAR* &szTextIn, int &nDstPos)
{
	int nColor = INVALID_LINK;
	if (szAttributeData[0] >= L'0' && szAttributeData[0] <= L'9')
	{
		// KCK NOTE: This is not an RGB value, it's an index into our color pallet.
		nColor = PStrToInt(szAttributeData);
	}
	else
	{
		WCHAR szColorName[256] = L"";
		char szKeyName[256] = "";
		PStrCopy(szColorName, szAttributeData, arrsize(szColorName));
		WCHAR *szEnd = PStrChr(szColorName, L' ');
		*szEnd = 0;

		PStrTrimTrailingSpaces(szColorName, arrsize(szColorName));
		PStrCvt(szKeyName, szColorName, arrsize(szColorName));
		nColor = ExcelGetLineByStringIndex(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_FONTCOLORS, szKeyName);
		if (nColor == INVALID_LINK)
		{
			ASSERTV(FALSE, "sHTAddColor: Color label ""%s"" not recognized from FontColors", szColorName );
			return FALSE;
		}
	}

	szTextIn[nDstPos++] = CTRLCHAR_COLOR;
	szTextIn[nDstPos++] = (WCHAR)(nColor & 0xFF);
	sHypertextManager.m_bAddColorReturn = TRUE;

	return TRUE;
}

// Add an item id to the hypertext data
BOOL static sHTAddItemId (UI_HYPERTEXT* pHypertext, const WCHAR* szAttributeData, WCHAR* &szTextIn, int &nDstPos)
{
	if (!pHypertext)
		return FALSE;

	UNITID nId;
	if (szAttributeData[0] >= L'0' && szAttributeData[0] <= L'9')
	{
		nId = (UNITID) PStrToInt(szAttributeData);
		if (nId > 0)
		{
			// Security check: Make sure the item exists, is an item, and belongs to the player
			UNIT* pUnit = UnitGetById(AppGetCltGame(), nId);
			ASSERTV_RETFALSE(pUnit && UnitGetGenus(pUnit) == GENUS_ITEM && UnitGetUltimateOwner(pUnit) == GameGetControlUnit(AppGetCltGame()), "Item not owned by player!");

			if (pUnit && UnitGetGenus(pUnit) == GENUS_ITEM && UnitGetUltimateOwner(pUnit) == GameGetControlUnit(AppGetCltGame()))
			{
				pHypertext->m_nDataType = HTDATA_UNITID;
				pHypertext->m_nData = nId;
			}
		}
	}

	return FALSE;
}

// Add the displayable text for the hypertext
BOOL static sHTAddText (UI_HYPERTEXT* pHypertext, const WCHAR* szAttributeData, WCHAR* &szTextIn, int &nDstPos)
{
	if (szAttributeData[0] != L'\"')
		return FALSE;

	// KCK brute force stringcopy, but whatever..
	int nSrcPos = 1;
	while (szAttributeData[nSrcPos] != 0 && szAttributeData[nSrcPos] != L'\"')
	{
		szTextIn[nDstPos] = szAttributeData[nSrcPos];
		nDstPos++;
		nSrcPos++;
	}
	szTextIn[nDstPos] = 0;

	return TRUE;
}

// Add a http link the hypertext
BOOL static sHTAddHref (UI_HYPERTEXT* pHypertext, const WCHAR* szAttributeData, WCHAR* &szTextIn, int &nDstPos)
{
	if (szAttributeData[0] != L'\"')
		return FALSE;

	// KCK: If there is no text tag, copy in the actual data
	if (nDstPos < 4)
	{
		int nSrcPos = 1;
		while (szAttributeData[nSrcPos] != 0 && szAttributeData[nSrcPos] != L'\"')
		{
			szTextIn[nDstPos] = szAttributeData[nSrcPos];
			nDstPos++;
			nSrcPos++;
		}
		szTextIn[nDstPos] = 0;
	}

	// Add link info to the data field
	pHypertext->m_nDataSize = PStrLen(szAttributeData) * sizeof(WCHAR);
	// Reality check size
	const DWORD DWORD_MAX = static_cast<DWORD>(-1);
	if (pHypertext->m_nDataSize > 0 && pHypertext->m_nDataSize < DWORD_MAX-1)
	{
		pHypertext->m_pDataPointer = (UCHAR*) MALLOC(NULL, pHypertext->m_nDataSize + 1);
		memset(pHypertext->m_pDataPointer, HTDATA_HTTPDATA, 1);
		memcpy(pHypertext->m_pDataPointer+1, &szAttributeData[1], pHypertext->m_nDataSize);
		pHypertext->m_pDataRequiresFree = TRUE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// Hyper text event callbacks for Items
//----------------------------------------------------------------------------

UI_MSG_RETVAL static sHTItemOnMouseHover(UI_COMPONENT *pComponent, UI_HYPERTEXT* pHypertext)
{
#if !ISVERSION(SERVER_VERSION)
	UNIT *pUnit = NULL;

	if (pHypertext->m_nDataType == HTDATA_UNITID)
	{
		pUnit = UnitGetById(AppGetCltGame(), (ID)pHypertext->m_nData);
	}

	if (pHypertext->m_nDataType == HTDATA_UNITGUID)
	{
		pUnit = UnitGetByGuid(AppGetCltGame(), (PGUID)pHypertext->m_nData);
	}

	if (pHypertext->m_nDataType == HTDATA_UNITDATA)
	{
		// KCK: Little tricky here, if we have a GUID then this unit has already been created
		// once and we can look it up using that. If not or we don't find it, we'll need to
		// create it again.
		pUnit = UnitGetByGuid(AppGetCltGame(), (PGUID)pHypertext->m_nData);
		if (!pUnit)
		{
			pUnit = c_CreateNewClientOnlyUnit(pHypertext->m_pDataPointer, pHypertext->m_nDataSize);
			// Save GUID in data so we can use it next time we try to view.
			pHypertext->m_nData = UnitGetGuid(pUnit);
		}
	}

	if (pUnit)
	{
		UISetHoverTextItem( pComponent, pUnit );
		UISetHoverUnit( UnitGetId( pUnit ), UIComponentGetId( pComponent ) );
		return UIMSG_RET_HANDLED;
	}
#endif
	return UIMSG_RET_NOT_HANDLED;
}

UI_MSG_RETVAL static sHTItemOnMouseLeave(UI_COMPONENT* component, UI_HYPERTEXT* pHypertext)
{
#if !ISVERSION(SERVER_VERSION)
	if (component)
	{
		UIClearHoverText();
		UISetHoverUnit(INVALID_ID, UIComponentGetId(component));
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
#endif
	return UIMSG_RET_HANDLED;
}

UI_MSG_RETVAL static sHTItemOnMouseDown(UI_COMPONENT* component, UI_HYPERTEXT* pHypertext)
{
	// KCK TODO: Show item details
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// Hyper text event callbacks for links
//----------------------------------------------------------------------------

UI_MSG_RETVAL static sHTLinkOnMouseDown(UI_COMPONENT* component, UI_HYPERTEXT* pHypertext)
{
	// KCK TODO: Go to page or just display link? Security concern
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// UIProcessHyperText
// Note that this function will not reallocate the string - it assumes that
// any token replacements it makes will be smaller that the initial token.
// Returns TRUE if any replacements were made.
//----------------------------------------------------------------------------
BOOL UIProcessHyperText( UI_LINE *pLine )
{
	ASSERT_RETFALSE(pLine);
	if (!pLine->HasText())
		return FALSE;

	BOOL	bChangeMade = FALSE;
	int		nMaxLen = PStrLen(pLine->GetText());

	// Find if we have any hyper text tokens
	for (int i = 0; sgtHypertextDataTable[i].m_pszToken != NULL; ++i)
	{
		const HYPERTEXT_DATA_STRUCT	*pHypertextData = &sgtHypertextDataTable[i];
		const WCHAR *szStartToken = pLine->GetText();
		szStartToken = PStrStr(pLine->GetText(), pHypertextData->m_pszToken);
		while (szStartToken)
		{
			// OK, we found one, find the end token, or ignore it if none
			const WCHAR *szEndToken = PStrStr(szStartToken, L">");
			if (szEndToken)
			{
				// Find the size of the complete token
				int		nLen = SIZE_TO_INT(szEndToken - szStartToken) + 1;

				// Next create a substitution string
				WCHAR *szNewText = MALLOC_NEWARRAY(NULL, WCHAR, nLen+1);
				int	nNewEnd = 0;

				// Create the hypertext object
				UI_HYPERTEXT *pNewHypertext = sHypertextManager.GetNewHypertextObject(i);

				szNewText[nNewEnd++] = CTRLCHAR_HYPERTEXT;
				szNewText[nNewEnd++] = (WCHAR)(pNewHypertext->m_nId);

				// Now process its attributes
				sHypertextManager.m_bAddColorReturn = FALSE;
				for (int a = 0; sgtHypertextAttributeTable[a].m_pszToken != NULL; ++a)
				{
					const HYPERTEXT_ATTRIBUTE_STRUCT	*pHypertextAttribute = &sgtHypertextAttributeTable[a];
					const WCHAR *szAttribute = szStartToken;
					szAttribute = PStrStr(szStartToken, pHypertextAttribute->m_pszToken);
					if (szAttribute && pHypertextAttribute->m_pfnProcessFunction)
					{
						// Call the attribute's process function
						pHypertextAttribute->m_pfnProcessFunction(pNewHypertext, szAttribute + PStrLen(pHypertextAttribute->m_pszToken) + 1, szNewText, nNewEnd);
					}
				}

				// Now apply our changes
				if (nNewEnd > 2)
				{
					int	nDstPos = SIZE_TO_INT(szStartToken-pLine->GetText());
					int nSrcPos = 0;

					// Add a color return tag if necessary
					if (sHypertextManager.m_bAddColorReturn)
						szNewText[nNewEnd++] = CTRLCHAR_COLOR_RETURN;

					// Add the end tag
					szNewText[nNewEnd++] = CTRLCHAR_HYPERTEXT_END;

					// This is the text we're going to make the replacement in
					WCHAR*	szReplacedText = pLine->EditText();

					// Copy the new string into the text to be replaced
					while (nSrcPos < nNewEnd)
					{
						szReplacedText[nDstPos] = szNewText[nSrcPos];
						nDstPos++;
						nSrcPos++;
					}

					// Move the rest of the string forward
					szReplacedText[nDstPos] = 0;
					PStrCat(szReplacedText, szEndToken+1, nMaxLen);

					// Push the hypertext struct into the line's list.
					pLine->m_HypertextList.CItemListPushTail(pNewHypertext);

					bChangeMade = TRUE;
				}

				FREE_DELETE_ARRAY(NULL, szNewText, WCHAR);
			}

			// Keep looking for more
			szStartToken++;
			szStartToken = PStrStr(szStartToken, pHypertextData->m_pszToken);
		}
	}

	return bChangeMade;
}

//----------------------------------------------------------------------------
// UIReprocessHyperText
// This will reprocess a line with previously processed hypertext data in it.
// This is done after word wrapping is calculated or on a textbox resize
//----------------------------------------------------------------------------
void UIReprocessHyperText(
	UI_LINE* pNewLine)
{
	WCHAR	*szStart = PStrChr(pNewLine->EditText(), CTRLCHAR_HYPERTEXT);
	while (szStart)
	{
		// Check to see if this origin hypertext is in this line - if so we're reprocessing our own line
		// and don't need to do anything
		UI_HYPERTEXT*	pOriginHypertext = UIFindHypertextById(*(szStart+1));
		if (pOriginHypertext && !pNewLine->m_HypertextList.IsInList(pOriginHypertext))
		{
			// Create a copy of the previously processed hypertext object from the Origin line
			UI_HYPERTEXT* pNewHypertext = sHypertextManager.GetNewHypertextObject(pOriginHypertext->m_nTypeIndex);
			if (pOriginHypertext->m_pDataRequiresFree)
			{
				// Make a copy of the data
				pNewHypertext->m_pDataPointer = (UCHAR*) MALLOC(NULL, pOriginHypertext->m_nDataSize + 1);
				memcpy(pNewHypertext->m_pDataPointer, pOriginHypertext->m_pDataPointer, pOriginHypertext->m_nDataSize + 1);
				pNewHypertext->m_pDataRequiresFree = TRUE;
			}
			else
			{
				pNewHypertext->m_pDataPointer = pOriginHypertext->m_pDataPointer;
			}
			pNewHypertext->m_nDataSize = pOriginHypertext->m_nDataSize;
			pNewHypertext->m_nDataType = pOriginHypertext->m_nDataType;
			pNewHypertext->m_nData = pOriginHypertext->m_nData;
			*(szStart+1) = (WCHAR)(pNewHypertext->m_nId);
			pNewLine->m_HypertextList.CItemListPushTail(pNewHypertext);
		}

		// If our end token isn't on this line, we should probably add one
		WCHAR	*szEnd = PStrChr(szStart, CTRLCHAR_HYPERTEXT_END);
		if (!szEnd && pNewLine->HasText())
		{
			WCHAR	szEndToken[2] = { CTRLCHAR_HYPERTEXT_END, 0 };
			PStrCat(pNewLine->EditText(), szEndToken, pNewLine->GetSize());
		}

		if (szEnd)
			szStart = PStrChr(szEnd, CTRLCHAR_HYPERTEXT);
		else
			szStart = NULL;
	}
}

UI_HYPERTEXT* UIFindHypertextById(WCHAR wc)
{
	return sHypertextManager.FindHypertextById((int)wc);
}

//----------------------------------------------------------------------------
// Find any hypertext member of a component the mouse might be over
//----------------------------------------------------------------------------
UI_HYPERTEXT* UIFindHyperTextUnderMouse(	
	UI_COMPONENT* component,
	DWORD xPos,
	DWORD yPos )
{
#if !ISVERSION(SERVER_VERSION)
	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return NULL;
	}

	if (UIComponentIsTextBox(component))
	{
		UI_TEXTBOX*		pTextBox = UICastToTextBox(component);
		UI_POSITION		pos = UIComponentGetAbsoluteLogPosition(component);
		float			fScrollOffset = 0.0f;
		if (pTextBox->m_pScrollbar)
			fScrollOffset = -1 * pTextBox->m_pScrollbar->m_ScrollPos.m_fY;
		CItemPtrList<UI_LINE>::USER_NODE *pLineItr = pTextBox->m_LinesList.GetNext(NULL);
		while (pLineItr)
		{
			CItemPtrList<UI_HYPERTEXT>::USER_NODE *pItr = pLineItr->m_Value->m_HypertextList.GetNext(NULL);
			while (pItr)
			{
				if (pos.m_fX + pItr->m_Value->m_UpperLeft.m_fX < xPos && 
					pos.m_fX + pItr->m_Value->m_LowerRight.m_fX > xPos &&
					pos.m_fY + pItr->m_Value->m_UpperLeft.m_fY + fScrollOffset < yPos && 
					pos.m_fY + pItr->m_Value->m_LowerRight.m_fY + fScrollOffset > yPos)
				{
					return pItr->m_Value;
				}
				pItr = pLineItr->m_Value->m_HypertextList.GetNext(pItr);
			}
			pLineItr = pTextBox->m_LinesList.GetNext(pLineItr);
		}
	}
#endif
	return NULL;
}

//----------------------------------------------------------------------------
// Handler for the Left Mouse Down event. Must be specified in the component's
// xml definition.
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHypertextOnLMouseDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_HYPERTEXT*	pHypertext = UIFindHyperTextUnderMouse(component, wParam, lParam);
	if (pHypertext)
	{
		// Do Hypertext stuff here
		sHypertextManager.SetLastHypertextObjectId(pHypertext->m_nId);
		if (sgtHypertextDataTable[pHypertext->m_nTypeIndex].m_pfnOnMouseDown)
			return sgtHypertextDataTable[pHypertext->m_nTypeIndex].m_pfnOnMouseDown(component, pHypertext);
	}

	return UIMSG_RET_NOT_HANDLED;
}


//----------------------------------------------------------------------------
// Handler for the Mouse Hover event. Must be specified in the component's
// xml definition.
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHypertextOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_HYPERTEXT*	pHypertext = UIFindHyperTextUnderMouse(component, wParam, lParam);
	if (pHypertext)
	{
		// Do Hypertext stuff here
		sHypertextManager.SetLastHypertextObjectId(pHypertext->m_nId);
		if (sgtHypertextDataTable[pHypertext->m_nTypeIndex].m_pfnOnMouseHover)
			return sgtHypertextDataTable[pHypertext->m_nTypeIndex].m_pfnOnMouseHover(component, pHypertext);
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
// Handler for Mouse movement within a component. Mostly checks to see if the
// mouse has left the active hypertext object.
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHypertextOnMouseMove(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_HYPERTEXT* pLastHypertext = sHypertextManager.GetLastHypertextObject();
	if (pLastHypertext)
	{
		UI_HYPERTEXT*	pHypertext = UIFindHyperTextUnderMouse(component, wParam, lParam);
		if (!pHypertext || pHypertext != pLastHypertext)
		{
			sHypertextManager.SetLastHypertextObjectId(0);		// No last Id
			if (sgtHypertextDataTable[pLastHypertext->m_nTypeIndex].m_pfnOnMouseLeave)
				return sgtHypertextDataTable[pLastHypertext->m_nTypeIndex].m_pfnOnMouseLeave(component, pLastHypertext);
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
// Take serialized unit data and add it to the hypertext container
//----------------------------------------------------------------------------
static BOOL sHTAddUnitData (UI_HYPERTEXT* pHypertext, const WCHAR* szData, int nDataLen)
{
	if (!pHypertext)
		return FALSE;

	pHypertext->m_nDataSize = nDataLen;
	if (pHypertext->m_nDataSize <= 0)
		return FALSE;

	pHypertext->m_nDataType = HTDATA_UNITDATA;
	pHypertext->m_pDataPointer = (UCHAR*) MALLOC(NULL, pHypertext->m_nDataSize);
	memcpy(pHypertext->m_pDataPointer, szData, pHypertext->m_nDataSize);
	pHypertext->m_pDataRequiresFree = TRUE;

	return TRUE;
}

//----------------------------------------------------------------------------
//	returns TRUE if the current message on the console line needs to be
//		hypertext encoded and routed through the game server
//----------------------------------------------------------------------------
BOOL UICurrentConsoleLineIsHypertext(
	const UI_LINE * pLine)
{
	if (!pLine)
		return FALSE;

	if (pLine->m_HypertextList.Count() < 1)
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
// Used to encode hypertext data for sending with chat messages
//----------------------------------------------------------------------------
DWORD c_UIEncodeHypertextForGameServer(
	const WCHAR * message,
	const UI_LINE * pLine,
	BYTE * destBuffer,
	DWORD destBufferLen)
{
	if (!pLine)
		return 0;
	int len = 0;
	CItemPtrList<UI_HYPERTEXT>::USER_NODE *pItr = pLine->m_HypertextList.GetNext(NULL);
	while (pItr)
	{
		pItr->m_Value->Encode(destBuffer, &len, destBufferLen);
		pItr = pLine->m_HypertextList.GetNext(pItr);
	}

	return len;
}

//----------------------------------------------------------------------------
// Server side validation/repackaging function.
// Mostly this just checks if the tags are valid, but also converts a unit id
// tag into a unit data tag and packs the unit data into the message.
// returns TRUE if everything checks out
//----------------------------------------------------------------------------
BOOL s_ValididateReprocessAndForwardHypertext(
	GAME * pGame,
	GAMECLIENT * client,
	WCHAR * message,
	BYTE * dataIn,
	DWORD dataInLen,
	BYTE * dataOut,
	DWORD dataLenOut )
{
	UNIT * pControlUnit = ClientGetControlUnit(client);
	ASSERTV_RETFALSE(pControlUnit, "s_ValididateReprocessAndForwardHypertext: Expected pControlUnit from client to be non-NULL");
	
	int	nCurInPos = 0;
	int nCurOutPos = 0;

	WCHAR	*szStart = PStrChr(message, CTRLCHAR_HYPERTEXT);
	while (szStart)
	{
		int idChar = (int)(*(szStart+1));

		UI_HYPERTEXT tTempHypertext;

		// Make sure this id is the first one in the input data
		tTempHypertext.Decode(dataIn, &nCurInPos);
		if (tTempHypertext.m_nId != idChar || nCurInPos > (int)dataInLen)
			return FALSE;

		// Ok, looks like it's good. Do any reprocessing we need to.
		if (sgtHypertextDataTable[tTempHypertext.m_nTypeIndex].m_eType == HT_ITEM)
		{
			ASSERTV_RETFALSE(tTempHypertext.m_nDataType == HTDATA_UNITID, "s_ValididateReprocessAndForwardHypertext: Received data type does not match that expected for UNITID");

			UNIT * pUnit = UnitGetById(pGame, (ID)(tTempHypertext.m_nData));
			ASSERTV_RETFALSE(pUnit, "s_ValididateReprocessAndForwardHypertext: Couldn't find unit of id specified" );
			ASSERTV_RETFALSE(UnitGetUltimateOwner(pUnit) == pControlUnit, "s_ValididateReprocessAndForwardHypertext: Unit specified not owned by client" );

			// Clean up old data and replace with serialized unit data
			FREE(NULL, tTempHypertext.m_pDataPointer);
			tTempHypertext.m_pDataPointer = NULL;
			tTempHypertext.m_pDataRequiresFree = FALSE;

			// write unit to buffer
			BYTE bDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
			BIT_BUF tBitBuffer( bDataBuffer, UNITFILE_MAX_SIZE_SINGLE_UNIT );
			XFER<XFER_SAVE> tXfer( &tBitBuffer );
			UNIT_FILE_XFER_SPEC<XFER_SAVE> tSpec( tXfer );
			tSpec.pUnitExisting = pUnit;
			tSpec.eSaveMode = UNITSAVEMODE_SENDTOOTHER_NOID;
			tSpec.idClient = ClientGetId(client);
			ASSERTV_RETFALSE(UnitFileXfer( pGame, tSpec ) == pUnit, "s_ValididateReprocessAndForwardHypertext: Serializing unit failed!" );
			WORD wLen = (WORD) (tSpec.tXfer.GetLen());
			ASSERTV_RETFALSE(sHTAddUnitData(&tTempHypertext, (WCHAR*)bDataBuffer, wLen), "s_ValididateReprocessAndForwardHypertext: sHTAddUnitData Failed." );
		}

		// Now Encode into the output buffer
		ASSERTV_RETFALSE (tTempHypertext.Encode(dataOut, &nCurOutPos, dataLenOut) == TRUE, "s_ValididateReprocessAndForwardHypertext: Could no re-encode hypertext data" );

		// If our end token isn't on this line, we should probably add one
		WCHAR	*szEnd = PStrChr(szStart, CTRLCHAR_HYPERTEXT_END);
		ASSERTV_RETFALSE (szEnd, "s_ValididateReprocessAndForwardHypertext: No end token found" );

		szStart = PStrChr(szEnd, CTRLCHAR_HYPERTEXT);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// Decodes hypertext data received from the chat server.
//----------------------------------------------------------------------------
void c_UIDecodeHypertextMessageFromChatServer(
	WCHAR * message,
	UI_LINE * pLine, 
	BYTE * dataIn,	//	nothing in the code prevents a NULL pointer here
	DWORD dataInLen )	//	nothing in the code prevents a 0 length value here
{
#if !ISVERSION(SERVER_VERSION)
	if (!dataIn || dataInLen <= 0)
		return;

	int nCurInPos = 0;
	WCHAR	*szStart = PStrChr(message, CTRLCHAR_HYPERTEXT);
	while (szStart)
	{
		UI_HYPERTEXT* pNewHypertext = sHypertextManager.GetNewHypertextObject(0);
		int		nNewId = pNewHypertext->m_nId;

		pNewHypertext->Decode(dataIn, &nCurInPos);
		pNewHypertext->m_nId = nNewId;

		// Link the tags in the string to the new id
		*(szStart+1) = (WCHAR)(nNewId);

		pLine->m_HypertextList.CItemListPushTail(pNewHypertext);

		// If our end token isn't on this line, we should probably add one
		WCHAR	*szEnd = PStrChr(szStart, CTRLCHAR_HYPERTEXT_END);
		ASSERTV_RETURN (szEnd, "c_UIDecodeHypertextMessageFromChatServer: No end token found" );

		szStart = PStrChr(szEnd, CTRLCHAR_HYPERTEXT);
	}

	pLine->SetText(message);
#endif
}
