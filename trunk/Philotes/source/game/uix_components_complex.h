//----------------------------------------------------------------------------
// uix_components_complex.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_COMPONENTS_COMPLEX_H_
#define _UIX_COMPONENTS_COMPLEX_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef _UIX_COMPONENTS_H_
#include "uix_components.h"
#endif

#ifndef __RECIPE_H_
#include "recipe.h"
#endif


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	UITYPE_INVGRID = NUM_UITYPES_BASIC,
	UITYPE_INVSLOT,
	UITYPE_SKILLSGRID,
	UITYPE_UNIT_NAME_DISPLAY,
	UITYPE_ACTIVESLOT,
	UITYPE_TRADESMANSHEET,
	UITYPE_CRAFTINGGRID,

	NUM_UITYPES_COMPLEX, 
};

//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------
struct UI_INVGRID : UI_COMPONENT
{
	UI_TEXTURE_FRAME*		m_pItemBkgrdFrame;
	UI_TEXTURE_FRAME*		m_pUseableBorderFrame;
	UI_TEXTURE_FRAME*		m_pUnidentifiedIconFrame;
	UI_TEXTURE_FRAME*		m_pBadClassIconFrame;
	UI_TEXTURE_FRAME*		m_pBadClassUnIDIconFrame;
	UI_TEXTURE_FRAME*		m_pLowStatsIconFrame;
	UI_TEXTURE_FRAME*		m_pSocketFrame;
	UI_TEXTURE_FRAME*		m_pModFrame;
	UI_TEXTURE_FRAME*		m_pRecipeCompleteFrame;
	UI_TEXTURE_FRAME*		m_pRecipeIncompleteFrame;

	DWORD					m_dwModWontFitColor;
	DWORD					m_dwModWontGoInItemColor;
	DWORD					m_dwInactiveItemColor;

	float					m_fCellWidth;
	float					m_fCellHeight;
	float					m_fCellBorder;
	
	int						m_nInvLocation;				// the inv location to use

	int						m_nInvLocDefault;			// possible inv location	
	int						m_nInvLocPlayer;			// possible inv location	
	int						m_nInvLocCabalist;			// possible inv location
	int						m_nInvLocHunter;			// possible inv location
	int						m_nInvLocTemplar;			// possible inv location
	int						m_nInvLocMerchant;			// possible inv location
	
	int						m_nGridWidth;
	int						m_nGridHeight;

	UI_INV_ITEM_GFX_ADJUSTMENT*	m_pItemAdj;
	UI_INV_ITEM_GFX_ADJUSTMENT*	m_pItemAdjStart;

	UI_ANIM_INFO			m_tItemAdjAnimInfo;

	// public methods
	UI_INVGRID(void);
};

struct UI_INVSLOT : UI_COMPONENT
{
	UI_TEXTURE_FRAME*		m_pLitFrame;
	UI_TEXTURE_FRAME*		m_pSocketFrame;
	UI_TEXTURE_FRAME*		m_pModFrame;
	UI_TEXTURE_FRAME*		m_pLowStatsIconFrame;
	UI_TEXTURE_FRAME*		m_pUseableBorderFrame;
	UI_TEXTURE_FRAME*		m_pBadClassIconFrame;
	UI_TEXTURE_FRAME*		m_pBadClassUnIDIconFrame;
	UI_TEXTURE_FRAME*		m_pUnidentifiedIconFrame;
	int						m_nInvLocation;
	UNITID					m_nInvUnitId;
	BOOL					m_bUseUnitId;
	float					m_fSlotScale;
	
	DWORD					m_dwModWontFitColor;
	DWORD					m_dwModWontGoInItemColor;

	UI_INV_ITEM_GFX_ADJUSTMENT	m_ItemAdj;
	UI_INV_ITEM_GFX_ADJUSTMENT	m_ItemAdjStart;
	float					m_fModelBorderX;
	float					m_fModelBorderY;

	UI_ANIM_INFO			m_tItemAdjAnimInfo;

	// public methods
	UI_INVSLOT(void);
};

struct UI_SKILLSGRID : UI_COMPONENT
{
	UI_TEXTURE_FRAME*		m_pBorderFrame;
	UI_TEXTURE_FRAME*		m_pBkgndFrame;
	UI_TEXTURE_FRAME*		m_pBkgndFramePassive;
	UI_TEXTURE_FRAME*		m_pSelectedFrame;
	UI_TEXTURE_FRAME*		m_pSkillupBorderFrame;
	UI_TEXTURE_FRAME*		m_pSkillupButtonFrame;

	UI_TEXTURE_FRAME*		m_pSkillupButtonDownFrame;
	UI_TEXTURE_FRAME*		m_pSkillShiftButtonFrame;
	UI_TEXTURE_FRAME*		m_pSkillShiftButtonDownFrame;
	float					m_fCellWidth;
	float					m_fCellHeight;
	float					m_fCellBorderX;
	float					m_fCellBorderY;
	float					m_fLevelTextOffset;
	int						m_nCellsX;
	int						m_nCellsY;
	int						m_nInvLocation;
	int						m_nSkillTab;
	BOOL					m_bLookupTabNum;
	int						m_nLastSkillRequestID;
	UI_PANELEX*				m_pIconPanel;
	UI_PANELEX*				m_pHighlightPanel;
	float					m_fIconWidth;
	float					m_fIconHeight;
	UI_ANIM_INFO			m_tSkillupAnimInfo;
	int						m_nSkillupSkill;
	
	int						m_nShiftToggleOnSound;
	int						m_nShiftToggleOffSound;

	// vvv Tugboat-specific
	UI_TEXTURE_FRAME*		m_pSkillPoint;
	BOOL					m_bIsHotBar;

	// public methods
	UI_SKILLSGRID(void);
};

// vvv Tugboat-specific
struct UI_CRAFTINGGRID : UI_COMPONENT
{
	UI_TEXTURE_FRAME*		m_pBorderFrame;
	UI_TEXTURE_FRAME*		m_pBkgndFrame;
	UI_TEXTURE_FRAME*		m_pBkgndFramePassive;
	UI_TEXTURE_FRAME*		m_pSelectedFrame;
	UI_TEXTURE_FRAME*		m_pSkillupBorderFrame;
	UI_TEXTURE_FRAME*		m_pSkillupButtonFrame;

	UI_TEXTURE_FRAME*		m_pSkillupButtonDownFrame;
	UI_TEXTURE_FRAME*		m_pSkillShiftButtonFrame;
	UI_TEXTURE_FRAME*		m_pSkillShiftButtonDownFrame;
	float					m_fCellWidth;
	float					m_fCellHeight;
	float					m_fCellBorderX;
	float					m_fCellBorderY;
	float					m_fLevelTextOffset;
	int						m_nCellsX;
	int						m_nCellsY;
	int						m_nInvLocation;
	int						m_nSkillTab;
	BOOL					m_bLookupTabNum;
	int						m_nLastSkillRequestID;
	UI_PANELEX*				m_pIconPanel;
	UI_PANELEX*				m_pHighlightPanel;
	float					m_fIconWidth;
	float					m_fIconHeight;
	UI_ANIM_INFO			m_tSkillupAnimInfo;
	int						m_nSkillupSkill;

	UI_TEXTURE_FRAME*		m_pSkillPoint;

	// public methods
	UI_CRAFTINGGRID(void);
};

struct UI_TRADESMANSHEET : UI_COMPONENT
{

	UI_TEXTURE_FRAME*		m_pBkgndFrame;
	UI_TEXTURE_FRAME*		m_pCreateButtonFrame;
	float					m_fCellHeight;
	int						m_nCellsY;
	int						m_nInvLocation;

	// public methods
	UI_TRADESMANSHEET(void);
};
struct UI_UNIT_NAME_DISPLAY : UI_COMPONENT
{
	UNITID					m_idUnitNameUnderCursor;
	float					m_fUnitNameUnderCursorDistSq;
	UI_TEXTURE_FRAME*		m_pItemBorderFrame;
//	WCHAR *					m_szPickupText;
	UI_LINE*				m_pPickupLine;
	UI_TEXTURE_FRAME*		m_pUnidentifiedIconFrame;
	UI_TEXTURE_FRAME*		m_pBadClassIconFrame;
	UI_TEXTURE_FRAME*		m_pBadClassUnIDIconFrame;
	UI_TEXTURE_FRAME*		m_pLowStatsIconFrame;
	UI_TEXTURE_FRAME*		m_pHardcoreFrame;
	UI_TEXTURE_FRAME*		m_pLeagueFrame;
	UI_TEXTURE_FRAME*		m_pEliteFrame;
	UI_TEXTURE_FRAME*		m_pPVPOnlyFrame;
	DWORD					m_dwHardcoreFrameColor;
	DWORD					m_dwLeagueFrameColor;
	DWORD					m_dwEliteFrameColor;
	float					m_fPlayerInfoSpacing;
	float					m_fPlayerBadgeSpacing;
	int						m_nPlayerInfoFontsize;

	int						m_nDisplayArea;
	EXCELTABLE				m_eTable;
	float					m_fMaxItemNameWidth;

	// public methods
	UI_UNIT_NAME_DISPLAY(void);
};

struct UI_ACTIVESLOT : UI_COMPONENT
{
	int					m_nSlotNumber;
	int					m_nKeyCommand;
	UI_TEXTURE_FRAME*	m_pLitFrame;
	UI_COMPONENT*		m_pIconPanel;

	DWORD				m_dwTickCooldownOver;

	//public methods
	UI_ACTIVESLOT(void);
};


//----------------------------------------------------------------------------
// CASTING FUNCTIONS
//----------------------------------------------------------------------------
CAST_FUNC(UITYPE_INVGRID, UI_INVGRID, InvGrid);
CAST_FUNC(UITYPE_INVSLOT, UI_INVSLOT, InvSlot);
CAST_FUNC(UITYPE_SKILLSGRID, UI_SKILLSGRID, SkillsGrid);
CAST_FUNC(UITYPE_UNIT_NAME_DISPLAY, UI_UNIT_NAME_DISPLAY, UnitNameDisplay);
CAST_FUNC(UITYPE_ACTIVESLOT, UI_ACTIVESLOT, ActiveSlot);
CAST_FUNC(UITYPE_TRADESMANSHEET, UI_TRADESMANSHEET, TradesmanSheet);
CAST_FUNC(UITYPE_CRAFTINGGRID, UI_CRAFTINGGRID, CraftingGrid);

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
UI_TEXTURE_FRAME *UIGetStdSkillIconBackground();
UIX_TEXTURE *UIGetStdSkillIconBackgroundTexture();
UI_TEXTURE_FRAME *UIGetStdSkillIconBorder();

// Good Lord, this is madness.  Madness!
struct UI_DRAWSKILLICON_PARAMS
{
	UI_COMPONENT *		component;
	UI_COMPONENT *		pBackgroundComponent;
	UI_RECT				rect;
	UI_POSITION			pos;
	int					nSkill;
	UI_TEXTURE_FRAME *	pBackgroundFrame;
	UI_TEXTURE_FRAME *	pBorderFrame;
	UI_TEXTURE_FRAME *	pSelectedFrame;
	UIX_TEXTURE *		pTexture;
	UIX_TEXTURE *		pSkillTexture;
	BOOL				bGreyout;
	BOOL				bMouseOver;
	BYTE				byAlpha;
	BOOL				bSmallIcon;
	BOOL				bTooCostly;
	BOOL				bShowHotkey;
	BOOL				bNegateOffsets;
	float				fScaleX;
	float				fScaleY;
	float				fSkillIconWidth;
	float				fSkillIconHeight;

	UI_DRAWSKILLICON_PARAMS::UI_DRAWSKILLICON_PARAMS(
		UI_COMPONENT *component,
		UI_RECT rect,
		UI_POSITION pos,
		int nSkill,
		UI_TEXTURE_FRAME *pBackgroundFrame,
		UI_TEXTURE_FRAME *pBorderFrame,
		UI_TEXTURE_FRAME *pSelectedFrame,
		UIX_TEXTURE *pTextureIn,
		BYTE byAlpha = 255,
		BOOL bGreyout = FALSE,
		BOOL bMouseOver = FALSE,
		BOOL bSmallIcon = TRUE,
		BOOL bTooCostly = FALSE,
		BOOL bShowHotkey = TRUE,
		float fScaleX = 1.0f,
		float fScaleY = 1.0f,
		float fSkillIconWidth = -1.0f,
		float fSkillIconHeight = -1.0f
		) : component(component), 
			rect(rect),
			pos(pos),
			nSkill(nSkill),
			pBackgroundFrame(pBackgroundFrame),
			pBackgroundComponent(NULL),
			pBorderFrame(pBorderFrame),
			pSelectedFrame(pSelectedFrame),
			pTexture(pTextureIn),
			pSkillTexture(NULL),
			byAlpha(byAlpha),
			bGreyout(bGreyout),
			bMouseOver(bMouseOver),
			bSmallIcon(bSmallIcon),
			bTooCostly(bTooCostly),
			bShowHotkey(bShowHotkey),
			bNegateOffsets(FALSE),
			fScaleX(fScaleX),
			fScaleY(fScaleY),
			fSkillIconWidth(fSkillIconWidth),
			fSkillIconHeight(fSkillIconHeight)

	{
		if (pTexture == NULL)
		{
			pTexture = UIComponentGetTexture(component);
		}
		if (pBackgroundComponent == NULL)
		{
			pBackgroundComponent = component;
		}
	}
};

BOOL UIDrawSkillIcon(
	const UI_DRAWSKILLICON_PARAMS& tParams);

void InitComponentTypesComplex(UI_XML_COMPONENT *pComponentTypes, UI_XML_ONFUNC*& pUIXmlFunctions, int& nXmlFunctionsSize);

void UIShowWaypointsScreen( 
	void);

void UIShowAnchorstoneScreen( 
	void);

void UIShowRunegateScreen( 
	void);

BOOL UIInstanceListReceiveInstances(
	GAME_INSTANCE_TYPE eInstanceType,
	LPVOID pArray,
	DWORD townInstanceCount );

BOOL UIPartyListAddPartyDesc(
	LPVOID partyInfoMessageArray,
	DWORD partyCount );

BOOL UIPartyListAddPartyMember(
	int	nPartyID,
	const WCHAR * szName,
	int nLevel,
	int nRank,
	int nPlayerUnitClass,
	PGUID idPlayer);

BOOL UIPartyListRemovePartyDesc(
	int	nPartyID);

BOOL UIPartyPanelRemoveMyPartyDesc(
	void);

struct UI_PARTY_DESC;

void UIPartyPanelContextChange(
   UI_COMPONENT *component,
   UI_PARTY_DESC *pParty = (UI_PARTY_DESC*)(-1),
   BOOL bInParty = (BOOL)(-1),
   BOOL bIsPartyLeader = (BOOL)(-1),
   BOOL bPartyIsAdvertised = (BOOL)(-1),
   BOOL bAdvertiseSelf = (BOOL)(-1));

void UIPlayerMatchDialogContextChange(void);

enum UI_PARTY_CREATE_RESULT
{
	PCR_ERROR,
	PCR_INVALID_PARTY_NAME,
	PCR_INVALID_PARTY_LEVEL_RANGE,
	PCR_ALREADY_IN_A_PARTY,
	PCR_PARTY_NAME_TAKEN,
	PCR_SUCCESS,
};

void UIPartyCreateOnServerResult(
	UI_PARTY_CREATE_RESULT eResult);

BOOL UIInvGridGetItemSize(
	UI_INVGRID * pInvGrid,
	UNIT * pItem, 
	UI_SIZE &tSize);

BOOL UIDoMerchantAction(UNIT * pItem,
	int nSuggestedLocation, 
	int nSuggestedX,
	int nSuggestedY);

BOOL UIDoMerchantAction(
	UNIT * pItem);

void UIDoJoinParty(void);

int UIInvGetItemGridBorderColor(
	UNIT * item,
	UI_COMPONENT *pComponent,
	UI_TEXTURE_FRAME **ppFrame,
	int *pnColorIcon);

void UITradesmanSheetFree( UI_COMPONENT* component);

int UIInvGetItemGridBackgroundColor(
	UNIT * item);

UI_MSG_RETVAL UIInvGridOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIInvSlotOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIInvSlotOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

void UIInvSlotSetUnitId(
	UI_INVSLOT* invslot,
	const UNITID unitid);

UI_MSG_RETVAL UICloseParent(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

// -- Load functions ----------------------------------------------------------
BOOL UIXmlLoadInvGrid			(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadInvSlot			(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadSkillsGrid		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadUnitNameDisplay	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadCraftingGrid		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);

// -- Free functions ----------------------------------------------------------
void UIInvGridFree					(UI_COMPONENT* component);

#endif